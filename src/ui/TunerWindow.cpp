#include "GuideButton.h"
#include "TunerWindow.h"

#include <map>

namespace daw
{
using namespace gen;

namespace
{
    constexpr int tuningIdBase = 1, trackInputId = 1, monoBase = 100, pairBase = 200, maxInputs = 8;
    constexpr int toneMs = 3000;

    /** Last reading of each string this session, per tuning. */
    std::map<tuning::Tuning, std::map<int, double>>& lastReadingStore()
    {
        static std::map<tuning::Tuning, std::map<int, double>> store;
        return store;
    }

    juce::String inputWords (int channel, bool stereo)
    {
        return stereo ? "Inputs " + juce::String (channel + 1) + "+" + juce::String (channel + 2) : "Input " + juce::String (channel + 1);
    }

    int getNumDeviceInputs (Engine& engine)
    {
        if (auto* device = engine.deviceManager.getCurrentAudioDevice())
            return juce::jmin (maxInputs, device->getInputChannelNames().size());

        return 0;
    }
}

std::vector<tuning::LastReading> getTunerLastReadings (tuning::Tuning t)
{
    std::vector<tuning::LastReading> out;

    for (const auto& [index, cents] : lastReadingStore()[t])
        out.push_back ({ index, cents });

    return out;
}

//==============================================================================
/** A string's button: lit while locked, with an edge in the colour of the string's last reading. */
class TunerWindow::StringButton final : public juce::TextButton
{
public:
    juce::Colour edge;
    bool hasEdge = false;

    void paintOverChildren (juce::Graphics& g) override
    {
        if (hasEdge)
        {
            g.setColour (edge);
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
        }
    }
};

juce::Colour TunerWindow::getZoneColour (tuning::Zone zone)
{
    switch (zone)
    {
        case tuning::Zone::inTune: return theme::meterGreen;
        case tuning::Zone::close:  return theme::meterYellow;
        case tuning::Zone::out:    return theme::meterRed;
    }

    return theme::meterGreen;
}

//==============================================================================
TunerWindow::TunerWindow (Engine& e, Selection& s)
    : engine (e), selection (s)
{
    setWantsKeyboardFocus (true);

    if (auto* settings = engine.properties.getUserSettings())
    {
        tuning = tuning::getTuningFromId (settings->getValue ("tuner.tuning", "guitarStandard"));
        referenceHz = juce::jlimit (415.0, 466.0, settings->getDoubleValue ("tuner.referenceHz", 440.0));
        const auto input = settings->getValue ("tuner.input", "track");

        if (input.startsWithChar ('m') || input.startsWithChar ('s'))
        {
            inputFollowsTrack = false;
            deviceStereo = input.startsWithChar ('s');
            deviceChannel = juce::jlimit (0, maxInputs - 1, input.substring (1).getIntValue());
        }

        muteToggle.setToggleState (settings->getBoolValue ("tuner.mute", true), juce::dontSendNotification);
    }
    else
        muteToggle.setToggleState (true, juce::dontSendNotification);

    for (const auto t : tuning::allTunings)
        tuningBox.addItem (tuning::getTuningName (t), (int) t + tuningIdBase);

    tuningBox.setSelectedId ((int) tuning + tuningIdBase, juce::dontSendNotification);
    tuningBox.setTooltip ("Which instrument and tuning to tune to. In a tuning, the tuner names the nearest string; Chromatic names any note.");
    tuningBox.onChange = [this]
    {
        setTuning ((tuning::Tuning) (tuningBox.getSelectedId() - tuningIdBase));
        saveSetting ("tuner.tuning", tuning::getTuningId (tuning));
    };
    addAndMakeVisible (tuningBox);

    inputBox.setTooltip ("The input the tuner hears: the selected audio track's input, or any input of your audio interface. Plug your instrument into it.");
    inputBox.onChange = [this]
    {
        const int id = inputBox.getSelectedId();

        if (id == trackInputId)
            inputFollowsTrack = true;
        else if (id >= pairBase)
        {
            inputFollowsTrack = false;
            deviceChannel = id - pairBase;
            deviceStereo = true;
        }
        else if (id >= monoBase)
        {
            inputFollowsTrack = false;
            deviceChannel = id - monoBase;
            deviceStereo = false;
        }

        saveSetting ("tuner.input", inputFollowsTrack ? juce::String ("track") : (deviceStereo ? "s" : "m") + juce::String (deviceChannel));
        applyInput();
    };
    addAndMakeVisible (inputBox);

    referenceLabel.setJustificationType (juce::Justification::centredRight);
    referenceLabel.setTooltip ("The pitch of A4 that every note is measured from. 440 Hz is the standard, and what DawIt's instruments play. "
                               "Drag, or double-click to type, from 415 to 466 Hz.");
    addAndMakeVisible (referenceLabel);

    referenceSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    referenceSlider.setRange (415.0, 466.0, 0.5);
    referenceSlider.setNumDecimalPlacesToDisplay (1);
    referenceSlider.setTextValueSuffix (" Hz");
    referenceSlider.setTextBoxIsEditable (true);
    referenceSlider.setColour (juce::Slider::backgroundColourId, theme::panelLight);
    referenceSlider.setColour (juce::Slider::trackColourId, theme::accent.withAlpha (0.25f));
    referenceSlider.setColour (juce::Slider::textBoxTextColourId, theme::text);
    referenceSlider.setValue (referenceHz, juce::dontSendNotification);
    referenceSlider.setTooltip (referenceLabel.getTooltip());
    referenceSlider.addMouseListener (this, false);
    referenceSlider.onValueChange = [this]
    {
        referenceHz = referenceSlider.getValue();
        saveSetting ("tuner.referenceHz", referenceHz);
        rebuildStringButtons();
        repaint();
    };
    addAndMakeVisible (referenceSlider);

    muteToggle.setTooltip ("Silences the input you hear through Monitor while the tuner is open, so tuning noise and feedback stay out of the speakers. "
                           "The track's own Mute button isn't touched, and recording still records.");
    muteToggle.onClick = [this]
    {
        engine.setTunerMuteMonitoring (muteToggle.getToggleState());
        saveSetting ("tuner.mute", muteToggle.getToggleState());
    };
    muteToggle.setWantsKeyboardFocus (false);
    addAndMakeVisible (muteToggle);

    helpButton.setTooltip ("How to tune a guitar: cents and beats, tunings, the reference pitch, tuning by ear and with harmonics.");
    helpButton.setWantsKeyboardFocus (false);
    helpButton.onClick = [this] { if (onOpenGuide != nullptr) onOpenGuide(); };
    addChildComponent (helpButton);

    tuningBox.setWantsKeyboardFocus (false);
    inputBox.setWantsKeyboardFocus (false);
    setTooltip ("The note you play and how far off it is: green within 3 cents, yellow within 10, red beyond. Right-click for the reference tone and the guide.");

    rebuildStringButtons();
    rebuildInputMenu();

    if (engine.getSampleRate() > 0.0)
        listener.prepare (engine.getSampleRate());

    engine.setTunerMuteMonitoring (muteToggle.getToggleState());
    applyInput();
    engine.setTunerListening (true);
    selection.addChangeListener (this);
    setSize (560, 440);
    startTimerHz (30);
}

TunerWindow::~TunerWindow()
{
    stopTimer();
    selection.removeChangeListener (this);
    stopTone();
    engine.setTunerListening (false);
}

void TunerWindow::saveSetting (const juce::String& key, const juce::var& value)
{
    if (auto* settings = engine.properties.getUserSettings())
    {
        if (value.isBool())
            settings->setValue (key, (bool) value);
        else if (value.isDouble())
            settings->setValue (key, (double) value);
        else
            settings->setValue (key, value.toString());
    }
}

void TunerWindow::showExample (double hz)
{
    exampleHz = hz;
    repaint();
}

void TunerWindow::parentHierarchyChanged()
{
    if (isShowing())
        grabKeyboardFocus();
}

//==============================================================================
void TunerWindow::setTuning (tuning::Tuning t)
{
    tuning = t;
    lockedString = -1;
    stopTone();
    rebuildStringButtons();
    resized();
    repaint();
}

void TunerWindow::rebuildStringButtons()
{
    strings = tuning::getStrings (tuning);
    stringButtons.clear();
    const auto last = getTunerLastReadings (tuning);

    for (int i = 0; i < (int) strings.size(); ++i)
    {
        auto* b = stringButtons.add (new StringButton());
        const auto& s = strings[(size_t) i];
        b->setButtonText (s.note);
        b->setClickingTogglesState (false);
        b->setWantsKeyboardFocus (false);
        b->setColour (juce::TextButton::buttonOnColourId, theme::accent);
        b->setToggleState (i == lockedString, juce::dontSendNotification);
        b->setTooltip (s.label + " string, " + s.note + " (" + juce::String (tuning::getHz (s.midi, referenceHz), 1) + " Hz). "
                       "Click to tune only this string and hear its note; click again to let the tuner pick the nearest string. The number keys, 1 for the first string, do the same.");
        b->onClick = [this, i] { lockString (i); };

        for (const auto& l : last)
            if (l.stringIndex == i)
            {
                b->hasEdge = true;
                b->edge = getZoneColour (tuning::getZone (l.cents));
            }

        addAndMakeVisible (b);
    }
}

void TunerWindow::lockString (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) strings.size()))
        return;

    if (lockedString == index)
    {
        lockedString = -1;
        stopTone();
    }
    else
    {
        lockedString = index;
        playTone (index);
    }

    for (int i = 0; i < stringButtons.size(); ++i)
        stringButtons[i]->setToggleState (i == lockedString, juce::dontSendNotification);

    repaint();
}

void TunerWindow::playTone (int stringIndex)
{
    if (! juce::isPositiveAndBelow (stringIndex, (int) strings.size()))
        return;

    Engine::Tone tone;
    tone.on = true;
    tone.shape = soundguide::Shape::saw;
    tone.frequency = (float) tuning::getHz (strings[(size_t) stringIndex].midi, referenceHz);
    tone.levelDb = -20.0f;
    engine.setTone (tone);
    toneEndsAt = juce::Time::getMillisecondCounter() + (juce::uint32) toneMs;
}

void TunerWindow::stopTone()
{
    if (toneEndsAt != 0)
    {
        engine.setTone ({});
        toneEndsAt = 0;
    }
}

//==============================================================================
void TunerWindow::rebuildInputMenu()
{
    inputBox.clear (juce::dontSendNotification);
    auto* track = selection.getTrack();
    const bool audioSelected = track != nullptr && track->type == Track::Type::audio;
    lastTrackLabel = audioSelected ? track->name + inputWords (track->inputChannel.load(), track->inputStereo.load()) : juce::String();
    lastNumInputs = getNumDeviceInputs (engine);

    if (audioSelected)
        inputBox.addItem (track->name + " \xc2\xb7 " + inputWords (track->inputChannel.load(), track->inputStereo.load()), trackInputId);

    if (lastNumInputs == 0)
        inputBox.addItem ("No audio input found", 9999);

    for (int c = 0; c < lastNumInputs; ++c)
        inputBox.addItem ("Input " + juce::String (c + 1), monoBase + c);

    if (lastNumInputs > 1)
    {
        inputBox.addSeparator();

        for (int c = 0; c + 1 < lastNumInputs; c += 2)
            inputBox.addItem ("Inputs " + juce::String (c + 1) + "+" + juce::String (c + 2) + " (stereo)", pairBase + c);
    }

    if (inputFollowsTrack && audioSelected)
        inputBox.setSelectedId (trackInputId, juce::dontSendNotification);
    else
        inputBox.setSelectedId ((deviceStereo ? pairBase : monoBase) + deviceChannel, juce::dontSendNotification);

}

void TunerWindow::applyInput()
{
    int channel = deviceChannel;
    bool stereo = deviceStereo;

    if (auto* track = selection.getTrack(); inputFollowsTrack && track != nullptr && track->type == Track::Type::audio)
    {
        channel = track->inputChannel.load();
        stereo = track->inputStereo.load();
    }

    if (channel != lastAppliedChannel || stereo != lastAppliedStereo)
    {
        engine.setTunerInput (channel, stereo);
        lastAppliedChannel = channel;
        lastAppliedStereo = stereo;
        listener.reset();
    }
}

void TunerWindow::changeListenerCallback (juce::ChangeBroadcaster*)
{
    rebuildInputMenu();
    applyInput();
}

//==============================================================================
juce::String TunerWindow::getStatusText (const tuner::Reading& r) const
{
    if (exampleHz > 0.0)
        return "Play one string at a time and let it ring.";

    if (engine.getNumInputChannels() == 0)
        return "No audio input is open. Choose your audio interface in Audio / MIDI Settings (Cmd+,).";

    if (r.exactlyZero)
        return "The input is silent. Is the instrument plugged in, and may DawIt use the microphone? (System Settings, Privacy & Security, Microphone)";

    if (r.tooQuiet)
        return "Too quiet to hear a note. Turn up the input gain on your audio interface, or play closer to the microphone.";

    return "No clear note. Play one string at a time and let it ring.";
}

void TunerWindow::timerCallback()
{
    if (toneEndsAt != 0 && (juce::int32) (juce::Time::getMillisecondCounter() - toneEndsAt) >= 0)
        stopTone();

    if (const double rate = engine.getSampleRate(); rate > 0.0 && ! juce::exactlyEqual (rate, listener.getSampleRate()))
        listener.prepare (rate);

    if (exampleHz <= 0.0 && listener.getSampleRate() > 0.0)
        listener.service (engine.getTunerTap());

    if (lastNumInputs != getNumDeviceInputs (engine)
        || lastTrackLabel != (selection.getTrack() != nullptr && selection.getTrack()->type == Track::Type::audio
                                  ? selection.getTrack()->name + inputWords (selection.getTrack()->inputChannel.load(), selection.getTrack()->inputStereo.load())
                                  : juce::String()))
        rebuildInputMenu();

    applyInput();

    const auto& r = listener.getReading();
    hasCurrent = exampleHz > 0.0 || r.hasNote;

    if (hasCurrent)
    {
        current = tuning::getReading (exampleHz > 0.0 ? exampleHz : r.hz, referenceHz, tuning, lockedString);

        if (exampleHz <= 0.0 && r.ageSeconds <= 0.0 && current.stringIndex >= 0)
        {
            auto& store = lastReadingStore()[tuning];
            const bool first = store.find (current.stringIndex) == store.end();
            store[current.stringIndex] = current.cents;

            if (auto* b = stringButtons[current.stringIndex])
            {
                const auto edge = getZoneColour (current.zone);
                b->hasEdge = true;

                if (first || b->edge != edge)
                {
                    b->edge = edge;
                    b->repaint();
                }
            }
        }
    }

    repaint();
}

//==============================================================================
void TunerWindow::resized()
{
    auto r = getLocalBounds().reduced (10, 8);
    const bool narrow = getWidth() < 780;
    auto row1 = r.removeFromTop (28);
    helpButton.setVisible (true);
    helpButton.setBounds (row1.removeFromRight (28));

    tuningBox.setBounds (row1.removeFromLeft (180));
    row1.removeFromLeft (8);

    if (narrow)
    {
        inputBox.setBounds (row1);
        r.removeFromTop (6);
        row1 = r.removeFromTop (28);
    }
    else
    {
        inputBox.setBounds (row1.removeFromLeft (230));
        row1.removeFromLeft (8);
    }

    referenceLabel.setBounds (row1.removeFromLeft (42));
    referenceSlider.setBounds (row1.removeFromLeft (84));
    row1.removeFromLeft (12);
    muteToggle.setBounds (row1.removeFromLeft (170));

    statusArea = r.removeFromBottom (34);
    levelArea = statusArea.removeFromLeft (110).withSizeKeepingCentre (110, 10);
    statusArea.removeFromLeft (10);

    auto buttons = r.removeFromBottom (52).reduced (0, 6);
    r.removeFromBottom (4);

    if (! stringButtons.isEmpty())
    {
        const int gap = 8, n = stringButtons.size();
        const int w = juce::jmin (84, (buttons.getWidth() - gap * (n - 1)) / n);
        auto strip = buttons.withSizeKeepingCentre (w * n + gap * (n - 1), buttons.getHeight());

        for (auto* b : stringButtons)
        {
            b->setBounds (strip.removeFromLeft (w));
            strip.removeFromLeft (gap);
        }
    }

    pictureArea = r.removeFromTop (r.getHeight());
}

void TunerWindow::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    const auto& r = listener.getReading();

    tunepics::TunerPicture p;
    p.greenCents = tuning::inTuneCents;
    p.yellowCents = tuning::closeCents;
    p.maxNoteSize = 72.0f;
    p.has = hasCurrent;
    p.line = " ";

    if (hasCurrent)
    {
        p.note = current.note;
        p.cents = current.cents;
        p.line = current.text.isNotEmpty() ? current.text : current.words;
        p.fade = exampleHz > 0.0 ? 1.0f : r.fade;
        const auto& key = engine.getKey();
        p.extraLine = (key.contains (current.midi) ? "in " : "not in ") + key.getName();
    }

    tunepics::paintTuner (g, pictureArea.toFloat(), p);

    // The level: -70 dB to 0, with a mark at -50 dB where the tuner starts to hear.
    const float level = exampleHz > 0.0 ? -28.0f : r.levelDb;
    auto bar = levelArea.toFloat();
    g.setColour (theme::grid);
    g.fillRoundedRectangle (bar, 2.0f);
    const float share = juce::jlimit (0.0f, 1.0f, (level + 70.0f) / 70.0f);
    g.setColour (level < -50.0f ? theme::textDim : theme::meterGreen);
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * share), 2.0f);
    g.setColour (theme::text.withAlpha (0.6f));
    g.fillRect (bar.getX() + bar.getWidth() * (20.0f / 70.0f) - 0.5f, bar.getY() - 2.0f, 1.0f, bar.getHeight() + 4.0f);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.5f));
    auto text = statusArea.toFloat();
    g.drawFittedText (hasCurrent ? juce::String ("Play one string at a time and let it ring.") : getStatusText (r), text.toNearestInt(),
                      juce::Justification::centredLeft, 2, 0.8f);
}

//==============================================================================
void TunerWindow::mouseDown (const juce::MouseEvent& e)
{
    if (e.originalComponent == &referenceSlider)
        return;

    grabKeyboardFocus();
}

void TunerWindow::mouseUp (const juce::MouseEvent& e)
{
    if (e.originalComponent == &referenceSlider)
    {
        // Alt and a double-click go back to the standard pitch.
        if (e.mods.isAltDown() && e.getNumberOfClicks() == 2)
        {
            referenceSlider.hideTextBox (false);
            referenceSlider.setValue (440.0, juce::sendNotificationSync);
        }

        return;
    }

    if (e.mods.isPopupMenu() && pictureArea.contains (e.getPosition()))
        showMenu();
}

void TunerWindow::showMenu()
{
    enum { playId = 1, unlockId, guideId };
    juce::PopupMenu menu;
    int target = lockedString;

    if (target < 0 && hasCurrent)
        target = current.stringIndex;

    menu.addItem (playId, "Play the reference tone", target >= 0);
    menu.addItem (unlockId, "Unlock the string", lockedString >= 0);

    if (onOpenGuide != nullptr)
    {
        menu.addSeparator();
        guideicon::addItem (menu, guideId, "How to tune a guitar...");
    }

    menu.showMenuAsync ({}, [safe = juce::Component::SafePointer<TunerWindow> (this), target] (int result)
    {
        if (safe == nullptr)
            return;

        if (result == playId)
            safe->playTone (target);
        else if (result == unlockId && safe->lockedString >= 0)
            safe->lockString (safe->lockedString);
        else if (result == guideId && safe->onOpenGuide != nullptr)
            safe->onOpenGuide();
    });
}

bool TunerWindow::keyPressed (const juce::KeyPress& key)
{
    if (! key.getModifiers().isAnyModifierKeyDown())
    {
        const int c = key.getKeyCode();

        if (c >= '1' && c <= '6')
        {
            if (c - '1' < (int) strings.size())
                lockString (c - '1');

            return true;
        }
    }

    return false;
}
} // namespace daw
