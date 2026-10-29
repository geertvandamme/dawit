#include "GuideButton.h"
#include "MixerView.h"
#include "engine/Routing.h"

namespace daw
{
namespace
{
    juce::String formatPan (double v)
    {
        const int percent = juce::roundToInt (std::abs (v) * 100.0);
        return percent == 0 ? juce::String ("C") : juce::String (percent) + (v < 0 ? " L" : " R");
    }
}

ChannelStrip::ChannelStrip (Engine& e, Selection& s, TrackActions& a, Track* t) : engine (e), selection (s), actions (a), track (t)
{
    // The same range and feel as the track header's volume slider.
    fader.textFromValueFunction = [] (double v) { return v <= -60.0 ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; };
    fader.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
    fader.setScrollWheelEnabled (false);   // scrolling never changes a value, so the mixer scrolls
    fader.setRange (-60.0, 6.0, 0.1);
    fader.setSkewFactorFromMidPoint (-12.0);
    fader.setDoubleClickReturnValue (true, 0.0);
    // A click on the fader only selects the strip; dragging moves the volume from where it is.
    fader.setSliderSnapsToMousePosition (false);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, width - 12, 18);
    fader.setTooltip (track != nullptr ? "Volume (double-click resets to 0 dB)"
                                       : "Master volume: the whole mix, as in the transport bar (double-click resets to 0 dB)");
    fader.onValueChange = [this]
    {
        if (! isAlive())
            return;

        if (track != nullptr)
            track->volumeDb = (float) fader.getValue();
        else
            engine.masterVolumeDb = (float) fader.getValue();
    };
    fader.addMouseListener (this, false);
    addAndMakeVisible (fader);

    if (track != nullptr)
    {
        // The same buttons and colours as the track header's.
        muteButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0a030));
        muteButton.setTooltip (track->isReturn() ? "Mute: take this return's effect out of the mix. The tracks keep playing, dry."
                               : track->isBus() ? "Mute: silence every track in this bus" : "Mute: silence this track, and its sends");
        muteButton.onClick = [this] { if (isAlive()) track->muted = muteButton.getToggleState(); };

        soloButton.setClickingTogglesState (true);
        soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4fc06f));
        soloButton.setTooltip (track->isReturn() ? "Solo: hear only this return. The tracks stay silent but keep sending to it."
                               : track->isBus() ? "Solo: hear this bus, with all its tracks" : "Solo: hear only the soloed tracks");
        soloButton.onClick = [this] { if (isAlive()) track->soloed = soloButton.getToggleState(); };

        for (auto* b : { &muteButton, &soloButton })
        {
            b->setWantsKeyboardFocus (false);
            b->addMouseListener (this, false);
            addAndMakeVisible (b);
        }

        panKnob.setScrollWheelEnabled (false);
        panKnob.setRange (-1.0, 1.0, 0.01);
        panKnob.setDoubleClickReturnValue (true, 0.0);
        panKnob.textFromValueFunction = formatPan;
        panKnob.setPopupDisplayEnabled (true, true, nullptr);
        panKnob.setTooltip ("Pan: where the track sits between the left and right speaker (double-click to centre)");
        panKnob.onValueChange = [this]
        {
            if (isAlive())
                track->pan = (float) panKnob.getValue();

            panLabel.setText (formatPan (panKnob.getValue()), juce::dontSendNotification);
        };
        panKnob.addMouseListener (this, false);
        addAndMakeVisible (panKnob);

        panLabel.setFont (juce::FontOptions (11.0f));
        panLabel.setColour (juce::Label::textColourId, theme::textDim);
        panLabel.setJustificationType (juce::Justification::centred);
        panLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (panLabel);

        outputButton.setTooltip ("Where this track's sound goes: straight to the master, or into a bus. Click to change it.");
        outputButton.onClick = [this] { if (isAlive()) actions.showOutputMenu (*track, &outputButton); };

        if (track->isReturn())
        {
            outputButton.setTooltip ("A return plays into the master.");
            outputButton.setEnabled (false);
        }
        else
        {
            // A knob for each of the first four returns.
            const auto returns = routing::getReturns (engine.getTracks());

            for (size_t i = 0; i < returns.size() && i < (size_t) maxSendKnobs; ++i)
            {
                auto* knob = sendKnobs.add (new SendKnob (engine, actions, *track, *returns[i], SendKnob::Look::compact));
                knob->addMouseListener (this, false);
                addAndMakeVisible (knob);
            }
        }
    }
    else
    {
        outputButton.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x92 Out")));
        outputButton.setTooltip ("The master goes to your speakers, and into a bounced WAV.");
        outputButton.setEnabled (false);

        // The master is the mix bus: the ? (like a bus header's) opens the guide at that topic.
        guideButton.setTooltip ("The mix bus: what the master is for, how much headroom to leave, and a gentle compressor to glue the mix (opens Buses, sends and routing)");
        guideButton.setWantsKeyboardFocus (false);
        guideButton.setMouseClickGrabsKeyboardFocus (false);
        guideButton.onClick = [this] { if (actions.onShowGuide) actions.onShowGuide (routingguide::Topic::mixBus); };
        addAndMakeVisible (guideButton);
    }

    outputButton.setWantsKeyboardFocus (false);
    outputButton.addMouseListener (this, false);
    addAndMakeVisible (outputButton);

    update();
}

ChannelStrip::~ChannelStrip() = default;

int ChannelStrip::getNumSendKnobs() const
{
    return juce::jmin ((int) routing::getReturns (engine.getTracks()).size(), maxSendKnobs);
}

bool ChannelStrip::isAlive() const
{
    // A track can go before the mixer catches up; never touch one that is gone.
    return track == nullptr || engine.indexOf (track) >= 0;
}

void ChannelStrip::update()
{
    if (! isAlive())
        return;

    const float volume = track != nullptr ? track->volumeDb.load() : engine.masterVolumeDb.load();

    if (! fader.isMouseButtonDown() && ! juce::approximatelyEqual ((float) fader.getValue(), volume))
        fader.setValue (volume, juce::dontSendNotification);

    if (track != nullptr)
    {
        if (! panKnob.isMouseButtonDown() && ! juce::approximatelyEqual ((float) panKnob.getValue(), track->pan.load()))
            panKnob.setValue (track->pan.load(), juce::dontSendNotification);

        panLabel.setText (formatPan (panKnob.getValue()), juce::dontSendNotification);

        // The track header and the mixer change the same flags.
        muteButton.setToggleState (track->muted.load(), juce::dontSendNotification);
        soloButton.setToggleState (track->soloed.load(), juce::dontSendNotification);

        // Muted, a bus above it muted, or another track soloed: the strip dims.
        const auto silence = routing::getSilence (*track, engine.getTracks());

        if (silence != shownSilence)
        {
            shownSilence = silence;
            shownSilenced = silence != routing::Silence::none;
            repaint();
        }

        if (track->isBus() && track->folded != shownFolded)
        {
            shownFolded = track->folded;
            repaint();
        }

        for (auto* knob : sendKnobs)
            knob->refresh();

        const auto output = juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x92 ")) + TrackActions::getOutputName (*track);

        if (output != shownOutput)
        {
            shownOutput = output;
            outputButton.setButtonText (output);
        }
    }

    if (track != nullptr && ! sendKnobs.isEmpty())
        repaint (sendArea);

    if (track != nullptr && track->name != shownName)
    {
        shownName = track->name;
        repaint();
    }

    const bool armedAudio = track != nullptr && track->isAudio() && track->armed.load();

    if (armedAudio != shownInput)
    {
        shownInput = armedAudio;
        inputMeter = {};
        repaint();
    }

    if (shownInput)
    {
        if (inputMeter.update (track->inputMeterLevel.load()))
            repaint (meterArea.expanded (3, 0));

        return;
    }

    const float left = track != nullptr ? track->peakLeft.load() : engine.masterPeakLeft.load();
    const float right = track != nullptr ? track->peakRight.load() : engine.masterPeakRight.load();

    if (! juce::approximatelyEqual (left, meterLeft) || ! juce::approximatelyEqual (right, meterRight))
    {
        meterLeft = left;
        meterRight = right;
        repaint (meterArea);
    }
}

juce::String ChannelStrip::getTooltip()
{
    if (shownInput && meterArea.expanded (4, 0).contains (getMouseXYRelative()))
        return recordingui::getInputMeterTooltip();

    return {};
}

void ChannelStrip::paint (juce::Graphics& g)
{
    const bool selected = selection.isSelected (track);
    g.fillAll (selected ? theme::panelLight : theme::panel);

    // Colour, name and type.
    const auto colour = track != nullptr ? track->colour : theme::text;
    g.setColour (colour);
    g.fillRect (0, 0, getWidth(), 4);

    g.setColour (shownSilenced ? theme::textDim : theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    const bool isBus = track != nullptr && isAlive() && track->isBus();
    g.drawText (track != nullptr && isAlive() ? track->name : juce::String ("Master"), isBus ? 20 : 6, 12, getWidth() - (isBus ? 26 : 12), 18,
                juce::Justification::centred, true);

    // A bus folds its tracks' strips away with the arrow in its top left.
    if (isBus)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (track->folded ? juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb8")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbe")),
                    getFoldArea(), juce::Justification::centred);
    }

    const bool warn = shownSilence == routing::Silence::busMuted;
    g.setColour (warn ? theme::meterYellow : theme::textDim);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (track == nullptr ? "MIX" : shownSilence == routing::Silence::busMuted ? "BUS MUTED" : shownSilenced ? "SILENT"
                : track->isReturn() ? "RETURN" : track->isBus() ? "BUS" : track->isMidi() ? "MIDI"
                : shownInput ? "REC " + recordingui::getInputChipText (*track) : "AUDIO", 6, 29, getWidth() - 12, 13,
                juce::Justification::centred);

    if (selected)
    {
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawRect (getLocalBounds(), 1);
    }

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (getWidth() - 1, 0, 1, getHeight());

    if (shownInput)
    {
        recordingui::drawInputMeter (g, meterArea.toFloat(), inputMeter, true);
        return;
    }

    drawStereoMeter (g, meterArea.toFloat(), meterLeft, meterRight);

    // The 0 dB mark on the meter.
    g.setColour (theme::textDim.withAlpha (0.7f));
    const float zeroY = (float) meterArea.getBottom() - (float) meterArea.getHeight() * meterProportion (1.0f);
    g.fillRect ((float) meterArea.getX() - 3.0f, zeroY, 3.0f, 1.0f);
}

void ChannelStrip::resized()
{
    auto r = getLocalBounds().reduced (6, 0);
    r.removeFromTop (44);

    // The master has no mute, solo or pan; it keeps the space so the faders line up.
    auto buttons = r.removeFromTop (22).withSizeKeepingCentre (62, 20);
    guideButton.setBounds (buttons.withSizeKeepingCentre (29, 20));
    muteButton.setBounds (buttons.removeFromLeft (29));
    soloButton.setBounds (buttons.removeFromRight (29));
    r.removeFromTop (2);

    auto pan = r.removeFromTop (54);
    panKnob.setBounds (pan.removeFromTop (38).withSizeKeepingCentre (38, 38));
    panLabel.setBounds (pan.removeFromTop (14));

    // Room for the sends when the song has a return, a row each, on every strip so the faders line up.
    sendArea = r.removeFromTop (getNumSendKnobs() * sendRowHeight);

    for (int i = 0; i < sendKnobs.size(); ++i)
        sendKnobs[i]->setBounds (sendArea.getX() + 4, sendArea.getY() + i * sendRowHeight, sendArea.getWidth() - 4, sendRowHeight);

    outputButton.setBounds (r.removeFromBottom (20));
    r.removeFromBottom (6);
    auto faderArea = r;
    meterArea = faderArea.removeFromRight (12).withTrimmedBottom (24).withTrimmedTop (4);
    fader.setBounds (faderArea);
}

juce::Rectangle<int> ChannelStrip::getFoldArea() const
{
    return { 2, 10, 18, 20 };
}

void ChannelStrip::mouseDown (const juce::MouseEvent& e)
{
    if (track == nullptr)
    {
        if (e.eventComponent == this && e.mods.isPopupMenu())
        {
            juce::PopupMenu menu;
            guideicon::addItem (menu, "About the mix bus (the master)...", [this] { if (actions.onShowGuide) actions.onShowGuide (routingguide::Topic::mixBus); });
            menu.showMenuAsync ({});
        }

        return;
    }

    if (! isAlive())
        return;

    const bool onStrip = e.eventComponent == this;

    if (onStrip && e.mods.isPopupMenu())
    {
        // A right-click on a selected strip keeps the selection, so the menu acts on all of them.
        if (! selection.isSelected (track))
            selection.select (track);

        actions.showTrackMenu (*track);
        return;
    }

    if (onStrip && track->isBus() && getFoldArea().contains (e.getPosition()))
    {
        engine.setFolded (*track, ! track->folded);
        return;
    }

    if (onStrip && e.mods.isShiftDown())
    {
        selection.extendTo (track, routing::getVisibleTracks (engine.getTracks()));
        return;
    }

    if (onStrip && e.mods.isCommandDown())
    {
        selection.toggle (track);
        return;
    }

    // As clicking the track header: keep the selected clip if it is on this track.
    auto* clip = selection.getClip();
    const bool clipIsOnThisTrack = std::any_of (track->clips.begin(), track->clips.end(), [clip] (auto& c) { return c.get() == clip; });
    selection.select (track, clipIsOnThisTrack ? clip : nullptr);
}

//==============================================================================
MixerView::MixerView (Engine& e, Selection& s, TrackActions& a) : engine (e), selection (s), actions (a)
{
    title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (title);

    hint.setColour (juce::Label::textColourId, theme::textDim);
    hint.setFont (juce::FontOptions (12.5f));
    addAndMakeVisible (hint);

    closeButton.setWantsKeyboardFocus (false);
    closeButton.setTooltip ("Close the mixer. Open it again with the Mixer button or Cmd+2.");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    stripHolder.onPaintOverChildren = [this] (juce::Graphics& g) { paintBrackets (g); };
    viewport.setViewedComponent (&stripHolder, false);
    viewport.setScrollBarsShown (false, true);
    addAndMakeVisible (viewport);
    addAndMakeVisible (master);

    syncStrips();
    startTimerHz (30);
}

void MixerView::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::panel);
    g.fillRect (getLocalBounds().removeFromTop (headerHeight));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, headerHeight - 1, getWidth(), 1);

    if (strips.isEmpty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Add a track to see its channel here.", viewport.getBounds(), juce::Justification::centred);
    }
}

void MixerView::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (8, 4);
    closeButton.setBounds (header.removeFromRight (24));
    title.setBounds (header.removeFromLeft (60));
    hint.setBounds (header);

    master.setBounds (r.removeFromRight (ChannelStrip::width));
    r.removeFromRight (4);
    viewport.setBounds (r);

    // Leave room for the scroll bar only when it shows.
    const int stripsWidth = strips.size() * ChannelStrip::width;
    const int height = r.getHeight() - (stripsWidth > r.getWidth() ? viewport.getScrollBarThickness() : 0);
    stripHolder.setSize (stripsWidth, height);

    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds (i * ChannelStrip::width, 0, ChannelStrip::width, height);
}

void MixerView::showStripOf (const Track& t)
{
    syncStrips();

    for (auto* strip : strips)
        if (strip->getTrack() == &t)
            viewport.setViewPosition (juce::jlimit (0, juce::jmax (0, stripHolder.getWidth() - viewport.getWidth()),
                                                    strip->getX() - (viewport.getWidth() - ChannelStrip::width) / 2), 0);
}

void MixerView::syncStrips()
{
    const auto rows = routing::getVisibleTracks (engine.getTracks());
    bool same = (int) rows.size() == strips.size();

    for (size_t i = 0; same && i < rows.size(); ++i)
        same = strips[(int) i]->getTrack() == rows[i];

    const bool hasReturn = ! routing::getReturns (engine.getTracks()).empty();
    const juce::String hintText = juce::String ("Click a strip to select its track. M mutes it, S solos it. Double-click a fader or knob to reset it. "
                                                "Shift- or Cmd-click strips to select several; right-click to put them in a bus. ")
                                  + (hasReturn ? "Turn up a send (a knob with a return's name) to send the track to the shared reverb or delay."
                                               : "Add a return track (Track > New Return Track) to send a track to a shared reverb or delay.");

    if (hint.getText() != hintText)
        hint.setText (hintText, juce::dontSendNotification);

    if (same)
        return;

    strips.clear();

    for (auto* t : rows)
        stripHolder.addAndMakeVisible (strips.add (new ChannelStrip (engine, selection, actions, t)));

    resized();
    repaint();
}

void MixerView::paintBrackets (juce::Graphics& g)
{
    // A bracket in the bus's colour over its strip and the strips of its tracks, one line lower for each level.
    for (int i = 0; i < strips.size(); ++i)
    {
        auto* track = strips[i]->getTrack();

        if (track == nullptr || engine.indexOf (track) < 0)
            continue;

        const auto bounds = strips[i]->getBounds();

        for (auto* bus = track->isBus() ? track : track->bus; bus != nullptr; bus = bus->bus)
        {
            const int level = routing::getDepth (*bus);

            if (level > 3)
                continue;

            g.setColour (bus->colour);
            g.fillRect (bounds.getX(), 4 + level * 3, bounds.getWidth(), 3);
        }
    }
}

void MixerView::timerCallback()
{
    if (! isShowing())
        return;

    syncStrips();

    for (auto* strip : strips)
        strip->update();

    master.update();

    // Follow the selection made elsewhere, and renamed tracks.
    if (selection.getTracks() != shownSelection)
    {
        shownSelection = selection.getTracks();
        stripHolder.repaint();
    }
}
} // namespace daw
