#include "TransportBar.h"
#include "TransportShortcuts.h"
#include "engine/Tempo.h"

namespace daw
{
void GlyphButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    const auto onColour = glyph == Glyph::record ? theme::playhead : theme::accent;

    g.setColour (on ? onColour.withAlpha (0.35f) : theme::panelLight.brighter (isHighlighted ? 0.15f : 0.0f));
    g.fillRoundedRectangle (bounds, 4.0f);

    if (isDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRoundedRectangle (bounds, 4.0f);
    }

    const auto icon = bounds.withSizeKeepingCentre (14.0f, 14.0f);
    juce::Path p;

    switch (glyph)
    {
        case Glyph::play:
            p.addTriangle (icon.getX() + 2.0f, icon.getY(), icon.getX() + 2.0f, icon.getBottom(),
                           icon.getRight(), icon.getCentreY());
            break;
        case Glyph::pause:
            p.addRectangle (icon.getX() + 1.0f, icon.getY(), 4.5f, icon.getHeight());
            p.addRectangle (icon.getRight() - 5.5f, icon.getY(), 4.5f, icon.getHeight());
            break;
        case Glyph::stop:
            p.addRectangle (icon.reduced (1.0f));
            break;
        case Glyph::toStart:
            p.addRectangle (icon.getX(), icon.getY(), 2.5f, icon.getHeight());
            p.addTriangle (icon.getRight(), icon.getY(), icon.getRight(), icon.getBottom(),
                           icon.getX() + 3.0f, icon.getCentreY());
            break;
        case Glyph::loop:
        {
            juce::Path arc;
            arc.addCentredArc (icon.getCentreX(), icon.getCentreY(), 6.0f, 6.0f, 0.0f, 0.6f, 5.6f, true);
            g.setColour (on ? theme::accent : theme::text);
            g.strokePath (arc, juce::PathStrokeType (2.0f));
            const float tipX = icon.getCentreX() + 6.0f * std::sin (0.6f);
            const float tipY = icon.getCentreY() - 6.0f * std::cos (0.6f);
            p.addTriangle (tipX - 4.0f, tipY - 3.0f, tipX + 3.0f, tipY - 3.5f, tipX, tipY + 3.0f);
            break;
        }
        case Glyph::metronome:
        {
            // A tapered case with the pendulum leaning right.
            juce::Path body;
            body.startNewSubPath (icon.getX() + 4.0f, icon.getY());
            body.lineTo (icon.getRight() - 4.0f, icon.getY());
            body.lineTo (icon.getRight(), icon.getBottom());
            body.lineTo (icon.getX(), icon.getBottom());
            body.closeSubPath();

            g.setColour (on ? onColour : theme::text);
            g.strokePath (body, juce::PathStrokeType (1.5f));
            g.drawLine (icon.getCentreX(), icon.getBottom() - 2.0f, icon.getRight() + 1.0f, icon.getY() + 2.0f, 1.5f);
            return;
        }
        case Glyph::keyboard:
        {
            // A piece of piano: an outline, three white-key dividers and two black keys.
            g.setColour (on ? onColour : theme::text);
            g.drawRoundedRectangle (icon.getX(), icon.getY() + 1.0f, icon.getWidth(), icon.getHeight() - 2.0f, 1.5f, 1.4f);

            for (int i = 1; i < 4; ++i)
                g.drawLine (icon.getX() + icon.getWidth() * (float) i / 4.0f, icon.getY() + 1.0f, icon.getX() + icon.getWidth() * (float) i / 4.0f, icon.getBottom() - 1.0f, 1.0f);

            for (int i = 1; i < 4; i += 2)
                g.fillRect (icon.getX() + icon.getWidth() * (float) i / 4.0f - 1.5f, icon.getY() + 1.0f, 3.0f, icon.getHeight() * 0.5f);

            return;
        }
        case Glyph::record:
            p.addEllipse (icon.reduced (1.5f));
            g.setColour (on ? theme::playhead : theme::playhead.withAlpha (0.75f));
            g.fillPath (p);
            return;
    }

    g.setColour (on ? onColour : theme::text);
    g.fillPath (p);
}

//==============================================================================
TransportBar::TransportBar (Engine& e) : engine (e)
{
    toStartButton.onClick = [this] { engine.setPositionBeats (0.0); };
    playButton.onClick = [this] { engine.togglePlay(); };
    stopButton.onClick = [this]
    {
        // First press stops, a second press returns to the start.
        if (engine.isPlaying())
            engine.stop();
        else
            engine.setPositionBeats (0.0);
    };
    recordButton.onClick = [this] { TransportShortcuts::toggleRecording (engine); };
    loopButton.setClickingTogglesState (true);
    loopButton.onClick = [this] { engine.loopEnabled = loopButton.getToggleState(); };

    // The metronome settings are remembered between sessions.
    auto& settings = *engine.properties.getUserSettings();
    engine.metronomeEnabled = savedMetronome = settings.getBoolValue ("metronome.enabled", false);
    engine.metronomeOnlyWhileRecording = settings.getBoolValue ("metronome.onlyWhileRecording", false);
    engine.countInEnabled = settings.getBoolValue ("metronome.countIn", false);

    metronomeButton.onClick = [this]
    {
        engine.metronomeEnabled = ! engine.metronomeEnabled.load();
        saveMetronomeSettings();
    };
    metronomeButton.onPopupMenu = [this] { showMetronomeMenu(); };
    keyboardButton.onClick = [this] { if (onToggleTyping) onToggleTyping(); };
    keyboardButton.onPopupMenu = [this] { if (onShowTypingGuide) onShowTypingGuide(); };

    for (auto* b : { &toStartButton, &playButton, &stopButton, &recordButton, &loopButton, &metronomeButton, &keyboardButton })
        addAndMakeVisible (b);

    tempoSlider.setScrollWheelEnabled (false);   // scrolling never changes a value
    tempoSlider.setSliderStyle (juce::Slider::LinearBar);
    tempoSlider.setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    tempoSlider.setColour (juce::Slider::backgroundColourId, juce::Colours::black.withAlpha (0.35f));
    // Typed tempos keep two decimals, as detected ones do (96.84); dragging moves in whole BPM.
    tempoSlider.setRange (20.0, 300.0, 0.01);
    tempoSlider.textFromValueFunction = [] (double bpm) { return formatBpm (bpm); };
    tempoSlider.valueFromTextFunction = [this] (const juce::String& text) { return parseBpm (text).value_or (engine.getTempo()); };
    tempoSlider.setValue (engine.getTempo(), juce::dontSendNotification);
    tempoSlider.setTooltip (steadyTempoTooltip);
    tempoSlider.onValueChange = [this]
    {
        auto bpm = tempoSlider.getValue();

        if (tempoSlider.isMouseButtonDown())
        {
            bpm = std::round (bpm);
            tempoSlider.setValue (bpm, juce::dontSendNotification);
        }

        engine.setTempo (bpm);
    };
    addAndMakeVisible (tempoSlider);

    tempoLabel.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (tempoLabel);

    keyLabel.setColour (juce::Label::textColourId, theme::textDim);
    keyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (keyLabel);

    for (int pc = 0; pc < 12; ++pc)
        keyTonicBox.addItem (MusicalKey::getTonicName (pc), pc + 1);

    for (int i = 0; i < MusicalKey::numScales; ++i)
        keyScaleBox.addItem (MusicalKey::getScaleName ((MusicalKey::Scale) i), i + 1);

    keyTonicBox.setTooltip ("Key of the song: tonic");
    keyScaleBox.setTooltip ("Key of the song: scale");

    for (auto* box : { &keyTonicBox, &keyScaleBox })
    {
        box->setWantsKeyboardFocus (false);
        box->onChange = [this] { setKeyFromBoxes(); };
        addAndMakeVisible (box);
    }

    keyTonicBox.setSelectedId (engine.getKey().tonic + 1, juce::dontSendNotification);
    keyScaleBox.setSelectedId ((int) engine.getKey().scale + 1, juce::dontSendNotification);

    positionLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 15.0f, juce::Font::plain));
    positionLabel.setJustificationType (juce::Justification::centred);
    positionLabel.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (0.35f));
    addAndMakeVisible (positionLabel);

    addAudioButton.onClick = [this] { if (onAddAudioTrack) onAddAudioTrack(); };
    addMidiButton.onClick = [this] { if (onAddMidiTrack) onAddMidiTrack(); };
    addBusButton.onClick = [this] { if (onAddBus) onAddBus(); };
    addBusButton.setTooltip ("Add a bus: a track without clips that other tracks play into, so you can add effects to them and turn them up or down together. "
                             "To put tracks in a new bus at once, select them and press Cmd+Shift+G.");
    addReturnButton.onClick = [this] { if (onAddReturn) onAddReturn (addReturnButton); };
    addReturnButton.setTooltip ("Add a return track: a shared reverb or delay that tracks send a copy of themselves to, each at its own level. "
                                "Then turn up the send knobs in the mixer.");
    mixerButton.setTooltip ("Show the mixer under the tracks: volume, pan and level of every track (Cmd+2)");
    mixerButton.setColour (juce::TextButton::buttonOnColourId, theme::accent.withAlpha (0.6f));
    mixerButton.onClick = [this] { if (onToggleMixer) onToggleMixer(); };

    for (auto* b : { &addAudioButton, &addMidiButton, &addBusButton, &addReturnButton, &mixerButton })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    masterLabel.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (masterLabel);

    masterSlider.setScrollWheelEnabled (false);
    masterSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
    masterSlider.setRange (-60.0, 6.0, 0.1);
    masterSlider.setSkewFactorFromMidPoint (-12.0);
    masterSlider.setDoubleClickReturnValue (true, 0.0);
    masterSlider.setTextValueSuffix (" dB");
    masterSlider.setValue (engine.masterVolumeDb.load(), juce::dontSendNotification);
    masterSlider.onValueChange = [this] { engine.masterVolumeDb = (float) masterSlider.getValue(); };
    addAndMakeVisible (masterSlider);

    startTimerHz (30);
}

TransportBar::~TransportBar() = default;

void TransportBar::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (getLocalBounds().removeFromBottom (1));

    drawStereoMeter (g, meterArea.toFloat(), meterLeft, meterRight);
}

void TransportBar::resized()
{
    auto r = getLocalBounds().reduced (8, 7);

    for (auto* b : { &toStartButton, &playButton, &stopButton, &recordButton, &loopButton, &metronomeButton, &keyboardButton })
    {
        b->setBounds (r.removeFromLeft (34));
        r.removeFromLeft (4);
    }

    r.removeFromLeft (10);
    tempoSlider.setBounds (r.removeFromLeft (64));
    tempoLabel.setBounds (r.removeFromLeft (34));
    keyLabel.setBounds (r.removeFromLeft (30));
    r.removeFromLeft (4);
    keyTonicBox.setBounds (r.removeFromLeft (58));
    r.removeFromLeft (4);
    keyScaleBox.setBounds (r.removeFromLeft (118));
    r.removeFromLeft (12);
    positionLabel.setBounds (r.removeFromLeft (176));
    r.removeFromLeft (12);

    meterArea = r.removeFromRight (12);
    r.removeFromRight (6);
    masterSlider.setBounds (r.removeFromRight (130));
    masterLabel.setBounds (r.removeFromRight (50));
    r.removeFromRight (10);

    // Buttons get their natural width, shrinking evenly when the window is narrow.
    juce::TextButton* buttons[] = { &addAudioButton, &addMidiButton, &addBusButton, &addReturnButton, &mixerButton };
    constexpr int gap = 6;
    int wanted = 0;

    for (auto* b : buttons)
        wanted += b->getBestWidthForHeight (r.getHeight()) + 12;

    const int available = r.getWidth() - gap * ((int) std::size (buttons) - 1);
    const float scale = wanted > available ? (float) juce::jmax (0, available) / (float) wanted : 1.0f;

    for (auto* b : buttons)
    {
        b->setBounds (r.removeFromLeft ((int) ((float) (b->getBestWidthForHeight (r.getHeight()) + 12) * scale)));
        r.removeFromLeft (gap);
    }
}

void TransportBar::setKeyFromBoxes()
{
    MusicalKey key;
    key.tonic = juce::jlimit (0, 11, keyTonicBox.getSelectedId() - 1);
    key.scale = (MusicalKey::Scale) juce::jlimit (0, MusicalKey::numScales - 1, keyScaleBox.getSelectedId() - 1);
    engine.setKey (key);
}

void TransportBar::showMetronomeMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Metronome");
    menu.addItem ("Click", true, engine.metronomeEnabled.load(), [this]
    {
        engine.metronomeEnabled = ! engine.metronomeEnabled.load();
        saveMetronomeSettings();
    });
    menu.addItem ("Click only while recording", true, engine.metronomeOnlyWhileRecording.load(), [this]
    {
        engine.metronomeOnlyWhileRecording = ! engine.metronomeOnlyWhileRecording.load();
        saveMetronomeSettings();
    });
    menu.addSeparator();
    menu.addItem ("Count in one bar before recording", true, engine.countInEnabled.load(), [this]
    {
        engine.countInEnabled = ! engine.countInEnabled.load();
        saveMetronomeSettings();
    });

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (metronomeButton));
}

void TransportBar::saveMetronomeSettings()
{
    auto& settings = *engine.properties.getUserSettings();
    savedMetronome = engine.metronomeEnabled.load();
    settings.setValue ("metronome.enabled", savedMetronome);
    settings.setValue ("metronome.onlyWhileRecording", engine.metronomeOnlyWhileRecording.load());
    settings.setValue ("metronome.countIn", engine.countInEnabled.load());
    settings.saveIfNeeded();
}

void TransportBar::timerCallback()
{
    // Follow key changes made elsewhere (e.g. by future composition tools).
    const auto key = engine.getKey();

    if (! keyTonicBox.isPopupActive() && ! keyScaleBox.isPopupActive())
    {
        keyTonicBox.setSelectedId (key.tonic + 1, juce::dontSendNotification);
        keyScaleBox.setSelectedId ((int) key.scale + 1, juce::dontSendNotification);
    }

    const bool playing = engine.isPlaying();
    playButton.setGlyph (playing ? GlyphButton::Glyph::pause : GlyphButton::Glyph::play);
    loopButton.setToggleState (engine.loopEnabled.load(), juce::dontSendNotification);
    recordButton.setToggleState (engine.isRecording(), juce::dontSendNotification);
    metronomeButton.setToggleState (engine.metronomeEnabled.load(), juce::dontSendNotification);

    if (engine.metronomeEnabled.load() != savedMetronome) // toggled with the K key
        saveMetronomeSettings();

    const double beat = engine.getPositionBeats();
    const double seconds = juce::jmax (0.0, engine.getTempoMap().beatToSeconds (beat));
    const int bar = (int) (beat / 4.0) + 1;
    const int beatInBar = (int) std::fmod (beat, 4.0) + 1;
    const int sixteenth = (int) ((beat - std::floor (beat)) * 4.0) + 1;
    const int minutes = (int) (seconds / 60.0);

    if (const double countIn = engine.getCountInBeatsLeft(); countIn > 0.0)
        positionLabel.setText ("Count-in   " + juce::String ((int) std::ceil (countIn - 1.0e-6)), juce::dontSendNotification);
    else
        positionLabel.setText (juce::String::formatted ("%3d.%d.%d   %02d:%06.3f", bar, beatInBar, sixteenth,
                                                        minutes, seconds - minutes * 60.0),
                               juce::dontSendNotification);

    if (! tempoSlider.isMouseButtonDown())
        tempoSlider.setValue (engine.getTempo(), juce::dontSendNotification);

    // While the tempo follows a recording bar by bar, the BPM shows the tempo at the playhead and
    // can't be changed: one number would throw the band's timing away.
    if (const bool steady = engine.getTempoMap().isSteady(); tempoSlider.isEnabled() != steady)
    {
        tempoSlider.setEnabled (steady);
        tempoSlider.setTooltip (steady ? steadyTempoTooltip
                                       : "The tempo at the playhead. The song's tempo follows a recording bar by bar (the Tempo row), "
                                         "so it changes as the song plays. Right-click the Tempo row to make it steady again");
    }

    if (! masterSlider.isMouseButtonDown()) // an opened project brings its own master volume
        masterSlider.setValue (engine.masterVolumeDb.load(), juce::dontSendNotification);

    meterLeft = engine.masterPeakLeft.load();
    meterRight = engine.masterPeakRight.load();
    repaint (meterArea);
}
} // namespace daw
