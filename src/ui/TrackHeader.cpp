#include "TrackHeader.h"
#include "PluginActions.h"
#include "engine/BuiltInFormat.h"
#include "engine/Routing.h"

namespace daw
{
TrackHeader::TrackHeader (Engine& e, Track& t, Selection& s, PluginWindowManager& w, TrackActions& a)
    : engine (e), track (t), selection (s), pluginWindows (w), actions (a)
{
    setWantsKeyboardFocus (true);              // for Escape while dragging
    setMouseClickGrabsKeyboardFocus (false);

    nameLabel.setText (track.name, juce::dontSendNotification);
    nameLabel.setEditable (false, true, false);
    nameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    nameLabel.setTooltip ("Double-click to rename");
    nameLabel.onTextChange = [this]
    {
        track.name = nameLabel.getText();
        engine.sendChangeMessage();
    };
    nameLabel.addMouseListener (this, false);
    addAndMakeVisible (nameLabel);

    // Record arm: a MIDI track plays what comes in from the MIDI keyboard, an audio track hears its input; Record records it here.
    armButton.setClickingTogglesState (true);
    armButton.setColour (juce::TextButton::buttonOnColourId, theme::playhead);
    armButton.setTooltip (track.isAudio() ? "Arm for recording: Record (R) then records this track's audio input, chosen next to R. Right-click to choose the input"
                                          : "Arm for recording: play this track from your MIDI keyboard, and record into it with Record (R)");
    armButton.onClick = [this] { engine.setArmed (track, armButton.getToggleState()); };
    armButton.setVisible (track.isMidi() || track.isAudio());
    armButton.setWantsKeyboardFocus (false);
    armButton.addMouseListener (this, false);   // a right-click opens the input menu
    addChildComponent (armButton);

    // An armed audio track: which input it records, and Monitor to hear it through the track's effects.
    inputChip.setTooltip ("The audio input this track records: a channel of your audio interface or microphone. Click to choose another");
    inputChip.onClick = [this] { recordingui::showInputMenu (engine, track, &inputChip); };
    inputChip.setWantsKeyboardFocus (false);
    addChildComponent (inputChip);

    monitorButton.setClickingTogglesState (true);
    monitorButton.setColour (juce::TextButton::buttonOnColourId, theme::accent);
    monitorButton.setTooltip ("Monitor: hear the input through this track's effects and fader, about one round trip late. Use headphones, or the microphone hears itself and howls");
    monitorButton.onClick = [this]
    {
        recordingui::setMonitor (engine, track, monitorButton.getToggleState(), actions.onShowRecordingGuide);
    };
    monitorButton.setWantsKeyboardFocus (false);
    addChildComponent (monitorButton);

    // Scale filter: show only the song key's notes, and only let those through from the MIDI keyboard.
    scaleButton.setClickingTogglesState (true);
    scaleButton.onClick = [this] { engine.setScaleFilter (track, scaleButton.getToggleState()); };
    scaleButton.setVisible (track.isMidi());
    scaleButton.setWantsKeyboardFocus (false);
    addChildComponent (scaleButton);

    muteButton.setClickingTogglesState (true);
    muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0a030));
    muteButton.setTooltip ("Mute");
    muteButton.onClick = [this] { track.muted = muteButton.getToggleState(); };

    soloButton.setClickingTogglesState (true);
    soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff4fc06f));
    soloButton.setTooltip ("Solo");
    soloButton.onClick = [this] { track.soloed = soloButton.getToggleState(); };

    muteButton.setTooltip (track.isReturn() ? "Mute: take this return's effect out of the mix. The tracks keep playing, dry."
                           : track.isBus() ? "Mute: silence every track in this bus" : "Mute: silence this track, and its sends");
    soloButton.setTooltip (track.isReturn() ? "Solo: hear only this return (the reverb or echo on its own). The tracks stay silent but keep sending to it."
                           : track.isBus() ? "Solo: hear this bus, with all its tracks" : "Solo");

    fxButton.setTooltip (track.isMidi() ? "Instrument and effect plugins" : track.isReturn() ? "Effects on the sum of the sends into this return"
                         : track.isBus() ? "Effects on the sum of this bus's tracks" : "Effect plugins");
    fxButton.onClick = [this] { showPluginMenu(); };

    deleteButton.setTooltip (track.isReturn() ? "Delete this return track. The sends to it go too; the tracks keep playing without it"
                             : track.isBus() ? "Remove this bus. Its tracks stay and play into where the bus played" : "Delete track");
    deleteButton.onClick = [this]
    {
        if (track.isReturn())
            actions.deleteReturn (track);
        else if (track.isBus())
            actions.ungroup (track);
        else if (actions.onDelete)
            actions.onDelete ({ &track }, true);
    };

    // A bus: the arrow folds its tracks into its row, and ? opens the guide.
    foldButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    foldButton.setTooltip ("Hide this bus's tracks (or show them again). While they're hidden, the bus's row still shows where they play.");
    foldButton.onClick = [this] { engine.setFolded (track, ! track.folded); };
    foldButton.setVisible (track.isBus());
    addChildComponent (foldButton);

    guideButton.setTooltip (track.isReturn() ? "Sends and returns: how a shared reverb or delay works, and how much of each track to send to it"
                                             : "Buses, sends and routing: where each track's sound goes, what a bus is for, and how levels add up");
    guideButton.onClick = [this] { if (actions.onShowGuide) actions.onShowGuide (track.isReturn() ? routingguide::Topic::sends : routingguide::Topic::why); };
    guideButton.setVisible (track.hasInput());
    addChildComponent (guideButton);

    sendsButton.setTooltip ("Send this track to a shared reverb or delay, and show it in the mixer");
    sendsButton.onClick = [this] { actions.showSendsButtonMenu (track, &sendsButton); };
    sendsButton.setVisible (! track.isReturn());
    addChildComponent (sendsButton);

    for (auto* b : std::initializer_list<juce::TextButton*> { &muteButton, &soloButton, &fxButton, &deleteButton, &foldButton, &guideButton, &sendsButton })
        b->setWantsKeyboardFocus (false);

    for (auto* b : { &muteButton, &soloButton, &fxButton, &deleteButton })
        addAndMakeVisible (b);

    volumeSlider.setScrollWheelEnabled (false);   // scrolling never changes a value, so the track list scrolls
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 18);
    volumeSlider.setRange (-60.0, 6.0, 0.1);
    volumeSlider.setSkewFactorFromMidPoint (-12.0);
    volumeSlider.setDoubleClickReturnValue (true, 0.0);
    volumeSlider.textFromValueFunction = [] (double v) { return v <= -60.0 ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; };
    volumeSlider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
    volumeSlider.setValue (track.volumeDb.load(), juce::dontSendNotification);
    volumeSlider.setTooltip ("Volume (double-click resets to 0 dB)");
    volumeSlider.onValueChange = [this] { track.volumeDb = (float) volumeSlider.getValue(); };
    volumeSlider.updateText();
    addAndMakeVisible (volumeSlider);

    panSlider.setScrollWheelEnabled (false);
    panSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange (-1.0, 1.0, 0.01);
    panSlider.setDoubleClickReturnValue (true, 0.0);
    panSlider.setValue (track.pan.load(), juce::dontSendNotification);
    panSlider.setPopupDisplayEnabled (true, true, nullptr);
    panSlider.textFromValueFunction = [] (double v)
    {
        const int percent = juce::roundToInt (std::abs (v) * 100.0);
        return percent == 0 ? juce::String ("C") : juce::String (percent) + (v < 0 ? " L" : " R");
    };
    panSlider.setTooltip ("Pan (double-click to centre)");
    panSlider.onValueChange = [this] { track.pan = (float) panSlider.getValue(); };
    panSlider.updateText();
    addAndMakeVisible (panSlider);

    updateFxButton();
    updateStructure();
    updateInputControls();
    startTimerHz (30);
}

TrackHeader::~TrackHeader()
{
    nameLabel.removeMouseListener (this);
    armButton.removeMouseListener (this);
}

std::vector<Track*> TrackHeader::getBusChain() const
{
    std::vector<Track*> chain;

    for (auto* b = track.bus; b != nullptr; b = b->bus)
        chain.insert (chain.begin(), b);

    return chain;
}

int TrackHeader::getIndent() const
{
    return juce::jmin (routing::getDepth (track), maxGutters) * gutterWidth;
}

juce::String TrackHeader::getBottomText() const
{
    switch (routing::getSilence (track, engine.getTracks()))
    {
        case routing::Silence::busMuted: return "BUS MUTED";
        case routing::Silence::solo:     return "SILENT";
        case routing::Silence::none:
        case routing::Silence::muted:    break;
    }

    if (track.isReturn())
    {
        const int count = (int) routing::getSenders (track, engine.getTracks()).size();
        return juce::String (juce::CharPointer_UTF8 ("RETURN \xc2\xb7 ")) + juce::String (count) + (count == 1 ? " send" : " sends");
    }

    if (track.isBus())
    {
        const int count = (int) std::count_if (engine.getTracks().begin(), engine.getTracks().end(),
                                               [this] (auto& t) { return ! t->hasInput() && routing::playsInto (*t, track); });
        return juce::String (juce::CharPointer_UTF8 ("BUS \xc2\xb7 ")) + juce::String (count) + (count == 1 ? " track" : " tracks");
    }

    return track.isMidi() ? "MIDI" : "AUDIO";
}

void TrackHeader::updateStructure()
{
    const int depth = routing::getDepth (track);

    if (depth != shownDepth)
    {
        shownDepth = depth;
        resized();
    }

    if (track.isBus())
        foldButton.setButtonText (track.folded ? juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb8")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbe")));

    shownBottomText = getBottomText();
    repaint();
}

void TrackHeader::editName()
{
    nameLabel.showEditor();
}

juce::String TrackHeader::getTooltip()
{
    const int x = getMouseXYRelative().x;
    const auto chain = getBusChain();

    if (shownInput && meterArea.expanded (3, 0).contains (getMouseXYRelative()))
        return recordingui::getInputMeterTooltip();

    if (x < getIndent() && ! chain.empty())
        return "In the " + chain[(size_t) juce::jlimit (0, (int) chain.size() - 1, x / gutterWidth)]->name + " bus";

    if (track.isReturn())
    {
        juce::StringArray names;

        for (auto* sender : routing::getSenders (track, engine.getTracks()))
            names.add (sender->name);

        const auto list = names.isEmpty() ? juce::String ("Nothing sends to it yet.")
                                          : names.joinIntoString (", ") + (names.size() == 1 ? " sends" : " send") + " a copy of itself to it.";
        return track.name + " is a return track: " + list + " It plays into the master. Turn up a track's " + track.name
               + " knob in the mixer to send it here. Right-click for more.";
    }

    if (track.isBus())
    {
        juce::StringArray names;

        for (auto* m : routing::getMembers (track, engine.getTracks()))
            names.add (m->name);

        const auto where = track.bus != nullptr ? track.bus->name : juce::String ("master");
        auto list = names.isEmpty() ? juce::String ("No track plays into it yet") : names.joinIntoString (", ") + (names.size() == 1 ? " plays" : " play") + " into it";
        return track.name + " is a bus: " + list + ", and it plays into the " + where + ". Drag a track's header onto it to add the track. Right-click for more.";
    }

    return track.name + (track.isMidi() ? ": a MIDI track" : ": an audio track") + ". Click to select it, Shift-click or Cmd-click to select several. "
           "Drag it by its name to move it up or down or into a bus. Right-click for more.";
}

void TrackHeader::paint (juce::Graphics& g)
{
    const bool selected = selection.isSelected (&track);
    g.fillAll (selected ? theme::panelLight : theme::panel);

    // The buses this track plays into, as coloured gutters.
    const auto chain = getBusChain();

    for (size_t level = 0; level < chain.size() && level < (size_t) maxGutters; ++level)
    {
        const int x = (int) level * gutterWidth;
        g.setColour (chain[level]->colour.withAlpha (0.25f));
        g.fillRect (x, 0, gutterWidth, getHeight());
        g.setColour (chain[level]->colour);
        g.fillRect (x, 0, 2, getHeight());
    }

    const int indent = getIndent();
    g.setColour (track.colour);
    g.fillRect (indent, 0, 4, getHeight());

    const bool warn = shownBottomText == "BUS MUTED" || shownBottomText == "SILENT";
    g.setColour (warn ? theme::meterYellow : theme::textDim);
    g.setFont (juce::FontOptions (11.0f));
    if (! shownInput)
        g.drawText (shownBottomText, indent + 12, getHeight() - 22, track.hasInput() ? 86 : 52, 16, juce::Justification::centredLeft);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
    g.fillRect (getWidth() - 1, 0, 1, getHeight());

    // An armed audio track shows what comes in, with the zones that tell whether the level is right.
    if (shownInput)
        recordingui::drawInputMeter (g, meterArea.toFloat(), inputMeter, true);
    else
        drawStereoMeter (g, meterArea.toFloat(), meterLeft, meterRight);
}

void TrackHeader::resized()
{
    auto r = getLocalBounds().withTrimmedLeft (8 + getIndent()).reduced (4, 5);
    meterArea = r.removeFromRight (8);
    r.removeFromRight (4);

    auto top = r.removeFromTop (22);
    deleteButton.setBounds (top.removeFromRight (22));
    top.removeFromRight (4);
    soloButton.setBounds (top.removeFromRight (24));
    top.removeFromRight (2);
    muteButton.setBounds (top.removeFromRight (24));
    top.removeFromRight (2);

    if (track.isMidi() || track.isAudio())
    {
        armButton.setBounds (top.removeFromRight (24));
        top.removeFromRight (4);
    }

    if (shownInput)
    {
        monitorButton.setBounds (top.removeFromRight (34));
        top.removeFromRight (4);
    }

    if (track.hasInput())
    {
        guideButton.setBounds (top.removeFromRight (22));
        top.removeFromRight (4);

        if (track.isBus())
            foldButton.setBounds (top.removeFromLeft (20));
    }

    nameLabel.setBounds (top);

    r.removeFromTop (4);
    auto middle = r.removeFromTop (30);
    panSlider.setBounds (middle.removeFromRight (34));
    middle.removeFromRight (4);
    volumeSlider.setBounds (middle);

    r.removeFromTop (2);
    auto bottom = r.removeFromTop (18);
    inputChip.setBounds (bottom.getX() + 4, bottom.getY(), 46, bottom.getHeight());
    bottom.removeFromLeft (track.hasInput() ? 84 : 50);

    if (track.isMidi())
    {
        scaleButton.setBounds (bottom.removeFromRight (46));
        bottom.removeFromRight (4);
    }

    if (! track.isReturn())
    {
        sendsButton.setBounds (bottom.removeFromRight (44));
        bottom.removeFromRight (4);
    }

    fxButton.setBounds (bottom);
}

void TrackHeader::mouseDown (const juce::MouseEvent& e)
{
    collapseOnMouseUp = false;

    // The R button: only its right-click is ours.
    if (e.eventComponent == &armButton)
    {
        if (e.mods.isPopupMenu() && track.isAudio())
            recordingui::showInputMenu (engine, track, &armButton);

        return;
    }

    if (e.mods.isPopupMenu())
    {
        // A right-click on a selected track keeps the selection, so the menu acts on all of them.
        if (! selection.isSelected (&track))
            selection.select (&track);

        actions.showTrackMenu (track);
        return;
    }

    if (e.mods.isShiftDown())
    {
        selection.extendTo (&track, routing::getVisibleTracks (engine.getTracks()));
    }
    else if (e.mods.isCommandDown())
    {
        selection.toggle (&track);
    }
    else if (selection.isSelected (&track) && selection.getTracks().size() > 1)
    {
        // It may be the start of dragging all of them: only a click without a drag selects this one alone.
        collapseOnMouseUp = true;
    }
    else
    {
        // Keep the selected clip (and so the piano roll) if it lives on this track.
        auto* clip = selection.getClip();
        const bool clipIsOnThisTrack = std::any_of (track.clips.begin(), track.clips.end(),
                                                    [clip] (auto& c) { return c.get() == clip; });

        selection.select (&track, clipIsOnThisTrack ? clip : nullptr);
    }
}

void TrackHeader::setDragging (bool shouldDrag)
{
    dragging = shouldDrag;
    setAlpha (dragging ? 0.55f : 1.0f);
    setMouseCursor (dragging ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void TrackHeader::mouseDrag (const juce::MouseEvent& e)
{
    if (e.eventComponent == &armButton || e.mods.isPopupMenu())
        return;

    if (! dragging && e.getDistanceFromDragStart() > 5)
    {
        collapseOnMouseUp = false;

        if (! selection.isSelected (&track))
            selection.select (&track);

        setDragging (true);
        grabKeyboardFocus();
    }

    if (dragging && onDragMove != nullptr)
        onDragMove (*this, e.getEventRelativeTo (this));
}

void TrackHeader::mouseUp (const juce::MouseEvent& e)
{
    if (e.eventComponent == &armButton)
        return;

    if (dragging)
    {
        setDragging (false);

        if (onDragEnd != nullptr)
            onDragEnd (*this);

        return;
    }

    if (collapseOnMouseUp)
    {
        collapseOnMouseUp = false;
        selection.select (&track);
    }
}

bool TrackHeader::keyPressed (const juce::KeyPress& key)
{
    if (dragging && key == juce::KeyPress::escapeKey)
    {
        setDragging (false);

        if (onDragCancel != nullptr)
            onDragCancel();

        return true;
    }

    return false;
}

void TrackHeader::timerCallback()
{
    // Recording arms the selected track by itself, so follow the engine. Loading a project and
    // splitting into stems can mute a track.
    armButton.setToggleState (track.armed.load(), juce::dontSendNotification);
    updateInputControls();
    muteButton.setToggleState (track.muted.load(), juce::dontSendNotification);
    soloButton.setToggleState (track.soloed.load(), juce::dontSendNotification);

    // The built-in synth's sound can change in its window.
    updateFxButton();

    // The words under the name follow the bus it is in, the tracks in it, and what is muted or soloed.
    if (const auto text = getBottomText(); text != shownBottomText)
    {
        shownBottomText = text;
        repaint();
    }

    // The scale filter can also change from the piano roll, and the key from the transport.
    scaleButton.setToggleState (track.scaleFilter.load(), juce::dontSendNotification);

    if (const auto tip = "Show only the notes of " + engine.getKey().getName() + " in this track's clips, and let only "
                         "those keys through from your MIDI keyboard. Notes already in clips still play.";
        scaleButton.getTooltip() != tip)
        scaleButton.setTooltip (tip);

    // Volume and pan can also change in the mixer.
    if (! volumeSlider.isMouseButtonDown() && ! juce::approximatelyEqual ((float) volumeSlider.getValue(), track.volumeDb.load()))
        volumeSlider.setValue (track.volumeDb.load(), juce::dontSendNotification);

    if (! panSlider.isMouseButtonDown() && ! juce::approximatelyEqual ((float) panSlider.getValue(), track.pan.load()))
        panSlider.setValue (track.pan.load(), juce::dontSendNotification);

    if (shownInput)
    {
        inputMeter.update (track.inputMeterLevel.load());
        meterLeft = meterRight = 0.0f;
    }
    else
    {
        meterLeft = track.peakLeft.load();
        meterRight = track.peakRight.load();
    }

    repaint (meterArea);
}

void TrackHeader::updateInputControls()
{
    if (! track.isAudio())
        return;

    const bool show = track.armed.load();

    if (show != shownInput)
    {
        shownInput = show;
        inputChip.setVisible (show);
        monitorButton.setVisible (show);

        if (! show)
            inputMeter = {};

        resized();
        repaint();
    }

    if (shownInput)
    {
        if (const auto text = recordingui::getInputChipText (track); inputChip.getButtonText() != text)
            inputChip.setButtonText (text);

        monitorButton.setToggleState (track.monitor.load(), juce::dontSendNotification);
    }

    // The arm and the input can't change while recording.
    const bool canChange = ! engine.isRecording();
    armButton.setEnabled (canChange);
    inputChip.setEnabled (canChange);
}

void TrackHeader::updateFxButton()
{
    juce::StringArray names;
    for (auto& slot : track.plugins)
        names.add (slot->instance->getName() + (slot->bypassed.load() ? " (off)" : ""));

    if (names.isEmpty())
        fxButton.setButtonText (track.isMidi() ? "FX: DawIt Synth (" + builtInSounds::getName (track.builtInSound) + ")" : "FX: none");
    else
        fxButton.setButtonText ("FX: " + names.joinIntoString (" > "));
}

void TrackHeader::showPluginMenu()
{
    juce::PopupMenu menu;
    constexpr int openBuiltInSynth = 1, useBuiltInSynth = 2, openBase = 1000, addBase = 10000;

    if (track.usesBuiltInSynth())
        menu.addItem (openBuiltInSynth, "Open DawIt Synth (" + builtInSounds::getName (track.builtInSound) + ")");

    for (int i = 0; i < (int) track.plugins.size(); ++i)
    {
        auto& slot = *track.plugins[(size_t) i];
        menu.addItem (openBase + i, "Open " + slot.instance->getName() + (slot.bypassed.load() ? " (bypassed)" : ""));
    }

    if (! track.plugins.empty() || track.usesBuiltInSynth())
        menu.addSeparator();

    juce::Array<juce::PluginDescription> choices;

    if (track.isMidi())
    {
        // DawIt's own instruments first, then the plugins by vendor.
        juce::PopupMenu instruments;
        instruments.addItem (useBuiltInSynth, "DawIt Synth (built-in, " + juce::String ((int) builtInSounds::all.size()) + " sounds)", true,
                             track.usesBuiltInSynth());

        for (auto& d : BuiltInFormat::getInstruments())
        {
            choices.add (d);
            instruments.addItem (addBase + choices.size() - 1, d.name + " (built-in)");
        }

        instruments.addSeparator();
        const auto pluginInstruments = plugins::createAddMenu (engine.knownPlugins, true, addBase, choices);

        for (juce::PopupMenu::MenuItemIterator it (pluginInstruments); it.next();)
            instruments.addItem (it.getItem());

        menu.addSubMenu (track.hasInstrument() ? "Replace instrument" : "Add instrument", instruments);
    }

    // DawIt's own effects first, then the plugins by vendor.
    juce::PopupMenu effects;

    for (auto& d : BuiltInFormat::getEffects())
    {
        choices.add (d);
        effects.addItem (addBase + choices.size() - 1, d.name + " (built-in)");
    }

    effects.addSeparator();

    const auto pluginEffects = plugins::createAddMenu (engine.knownPlugins, false, addBase, choices);

    for (juce::PopupMenu::MenuItemIterator it (pluginEffects); it.next();)
        effects.addItem (it.getItem());

    menu.addSubMenu ("Add effect", effects);

    if (engine.knownPlugins.getNumTypes() == 0)
        menu.addItem (-1, "No plugins found yet - use File > Plugins... to scan", false);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&fxButton),
                        [this, choices, safeThis = juce::Component::SafePointer<TrackHeader> (this)] (int result)
                        {
                            if (safeThis == nullptr)
                                return;

                            if (result == openBuiltInSynth)
                                plugins::openBuiltInSynth (engine, pluginWindows, track);
                            else if (result == useBuiltInSynth)
                                plugins::useBuiltInSynth (engine, pluginWindows, track);
                            else if (result >= openBase && result < openBase + (int) track.plugins.size())
                                plugins::open (pluginWindows, track, *track.plugins[(size_t) (result - openBase)]);
                            else if (result >= addBase && result < addBase + choices.size())
                                plugins::add (engine, pluginWindows, track, choices.getReference (result - addBase));
                        });
}
} // namespace daw
