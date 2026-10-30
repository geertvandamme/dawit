#include "MainComponent.h"
#include "HitsToMidiWindow.h"
#include "AudioSettingsPanel.h"
#include "AdditiveEditor.h"
#include "BounceActions.h"
#include "BounceDialog.h"
#include "ChordSheetExport.h"
#include "SheetMusicExport.h"
#include "GuidePrint.h"
#include "CompressorEditor.h"
#include "DelayEditor.h"
#include "DistortionEditor.h"
#include "GateEditor.h"
#include "StereoEditor.h"
#include "TuneEditor.h"
#include "DrumsEditor.h"
#include "PhysicalEditor.h"
#include "SubtractiveEditor.h"
#include "ReverbEditor.h"
#include "SaturationEditor.h"
#include "EqEditor.h"
#include "FmEditor.h"

namespace daw
{
namespace
{
    class PluginListWindow final : public juce::DocumentWindow
    {
    public:
        PluginListWindow (Engine& engine, juce::KeyListener& keys)
            : juce::DocumentWindow ("Plugins", theme::panel, juce::DocumentWindow::closeButton), keyListener (keys)
        {
            addKeyListener (&keyListener);

            auto* list = new juce::PluginListComponent (engine.pluginFormats, engine.knownPlugins,
                                                        engine.getDeadMansPedalFile(),
                                                        engine.properties.getUserSettings());
            list->setOptionsButtonText ("Scan / Options...");

            setUsingNativeTitleBar (true);
            setContentOwned (list, true);
            setResizable (true, false);
            centreWithSize (760, 520);
        }

        ~PluginListWindow() override { removeKeyListener (&keyListener); }

        void closeButtonPressed() override { setVisible (false); }

    private:
        juce::KeyListener& keyListener;
    };
}

MainComponent::MainComponent (Engine& e) : engine (e)
{
    // The built-in effects live in the engine; their windows are part of the app.
    BuiltInEq::createEditorFunction = [] (BuiltInEq& eqProcessor) { return new BuiltInEqEditor (eqProcessor); };
    BuiltInCompressor::createEditorFunction = [] (BuiltInCompressor& compressor) { return new BuiltInCompressorEditor (compressor); };
    BuiltInReverb::createEditorFunction = [] (BuiltInReverb& reverb) { return new BuiltInReverbEditor (reverb); };
    BuiltInDelay::createEditorFunction = [] (BuiltInDelay& delay) { return new BuiltInDelayEditor (delay); };
    BuiltInSaturation::createEditorFunction = [] (BuiltInSaturation& saturation) { return new BuiltInSaturationEditor (saturation); };
    BuiltInStereo::createEditorFunction = [] (BuiltInStereo& stereo) { return new BuiltInStereoEditor (stereo); };
    BuiltInTune::createEditorFunction = [] (BuiltInTune& tuner) { return new BuiltInTuneEditor (tuner); };
    BuiltInGate::createEditorFunction = [] (BuiltInGate& gate) { return new BuiltInGateEditor (gate); };
    BuiltInDistortion::createEditorFunction = [] (BuiltInDistortion& distortion) { return new BuiltInDistortionEditor (distortion); };
    BuiltInSubtractive::createEditorFunction = [] (BuiltInSubtractive& synth) { return new BuiltInSubtractiveEditor (synth); };
    BuiltInAdditive::createEditorFunction = [] (BuiltInAdditive& synth) { return new BuiltInAdditiveEditor (synth); };
    BuiltInFm::createEditorFunction = [] (BuiltInFm& synth) { return new BuiltInFmEditor (synth); };
    BuiltInDrums::createEditorFunction = [] (BuiltInDrums& machine) { return new BuiltInDrumsEditor (machine); };
    BuiltInPhysical::createEditorFunction = [] (BuiltInPhysical& synth) { return new BuiltInPhysicalEditor (synth); };

    fxui::setHostTrackLookup ([this] (const juce::AudioProcessor& processor)
    {
        const juce::ScopedLock lock (engine.getLock());
        for (const auto& track : engine.getTracks())
            for (const auto& slot : track->plugins)
                if (slot->instance.get() == &processor)
                    return fxui::HostTrack { track->name, track->isReturn(), track->isBus() };
        return fxui::HostTrack {};
    });

    tunetext::setSongAccess ({
        [this]
        {
            tunetext::SongInfo info;
            info.key = engine.getKey();
            info.sections = engine.getSections();
            info.tempo = engine.getTempoMap();
            info.numBars = juce::jmax (8, engine.getSongEndBar());
            return info;
        },
        [this] (double beat) { engine.setPositionBeats (beat); },
        [this] { return engine.properties.getUserSettings(); } });

    addAndMakeVisible (transport);
    addAndMakeVisible (inspector);
    addAndMakeVisible (inspectorEdge);
    addAndMakeVisible (arrangement);
    addAndMakeVisible (divider);
    addAndMakeVisible (deviceBar);
    addAndMakeVisible (pianoRoll);
    addChildComponent (waveEditor);
    addChildComponent (mixer);

    layout.setItemLayout (0, 150, -1.0, -0.58);
    layout.setItemLayout (1, 6, 6, 6);
    layout.setItemLayout (2, DeviceBar::preferredHeight, DeviceBar::preferredHeight, DeviceBar::preferredHeight);
    deviceBar.onPreferredHeightChanged = [this] { resized(); };
    layout.setItemLayout (3, 120, -1.0, -0.42);

    transport.onAddAudioTrack = [this] { addTrack (Track::Type::audio); };
    transport.onAddMidiTrack = [this] { addTrack (Track::Type::midi); };
    transport.onAddBus = [this] { arrangement.getTrackActions().addEmptyBus(); };
    transport.onAddReturn = [this] (juce::Component& button) { arrangement.getTrackActions().showNewReturnMenu (&button); };
    arrangement.onShowRecordingGuide = [this] (recordingguide::Topic topic) { showRecordingGuide (topic); };
    arrangement.onShowTriggerGuide = [this] (triggerguide::Topic topic) { showTriggerGuide (topic); };
    arrangement.onShowRoutingGuide = [this] (routingguide::Topic topic) { showRoutingGuide (topic); };
    pianoRoll.onShowRhythmGuide = [this] { showRhythmGuide(); };
    pianoRoll.onShowReadingGuide = [this] { showReadingGuide(); };
    pianoRoll.onShowGeneratedMusicGuide = [this]
    {
        showReadingGuide();
        if (readingGuide != nullptr) readingGuide->showTopic (gen::readmusic::Topic::generatedMusic);
    };
    pianoRoll.onExportSheetMusic = [this] { sheetmusic::showDialog (engine, selection.getTrack(), project.getProjectName(), [this] { showReadingGuide(); }); };
    pianoRoll.onClose = [this] { showBottomPanel (BottomPanel::closed); };
    waveEditor.onClose = [this] { showBottomPanel (BottomPanel::closed); };
    waveEditor.onShowSoundGuide = [this] { showSoundGuide(); };
    pianoRoll.onShowMidiGuide = [this] { showMidiGuide(); };
    pianoRoll.onModeChanged = [this] { commandManager.commandStatusChanged(); };   // the View menu's ticks
    mixer.onClose = [this] { showBottomPanel (BottomPanel::closed); };
    deviceBar.onShowMixer = [this] { showBottomPanel (BottomPanel::mixer); };
    arrangement.getTrackActions().onBounceToWav = [this] (std::vector<Track*> tracks)
    {
        bounceui::showDialog (engine, project.getProjectName(), tracks, true);
    };
    arrangement.getTrackActions().onShowSends = [this] (Track& t)
    {
        selection.select (&t);
        showBottomPanel (BottomPanel::mixer);
        mixer.showStripOf (t);
    };
    arrangement.getTrackActions().onShowSignalFlow = [this] (Track& t)
    {
        selection.select (&t);
        showSignalFlow();

        if (signalFlow != nullptr)
            signalFlow->showTrack (t);
    };
    arrangement.onOpenClipEditor = [this] { showBottomPanel (BottomPanel::clipEditor); };
    transport.onToggleMixer = [this] { commandManager.invokeDirectly (toggleMixerCommand, false); };
    arrangement.onSongRowsFoldedChanged = [this] { commandManager.commandStatusChanged(); };

    {
        auto* settings = engine.properties.getUserSettings();
        inspectorOpen = settings->getBoolValue ("view.inspectorOpen", true);
        inspectorWidth = juce::jlimit (InspectorPanel::minWidth, InspectorPanel::maxWidth, settings->getIntValue ("view.inspectorWidth", InspectorPanel::defaultWidth));
    }

    inspector.onClose = [this] { setInspectorOpen (false); };
    inspectorEdge.getPanelWidth = [this] { return inspectorWidth; };
    inspectorEdge.onResize = [this] (int w)
    {
        inspectorWidth = juce::jlimit (InspectorPanel::minWidth, InspectorPanel::maxWidth, w);
        resized();
    };
    inspectorEdge.onResizeEnd = [this] { engine.properties.getUserSettings()->setValue ("view.inspectorWidth", inspectorWidth); };
    inspector.setVisible (inspectorOpen);
    inspectorEdge.setVisible (inspectorOpen);

    const auto savedPanel = engine.properties.getUserSettings()->getValue ("ui.bottomPanel", "clip");
    showBottomPanel (savedPanel == "mixer" ? BottomPanel::mixer : savedPanel == "closed" ? BottomPanel::closed : BottomPanel::clipEditor);
    arrangement.onShowRhythmGuide = [this] { showRhythmGuide(); };
    arrangement.getProjectName = [this] { return project.getProjectName(); };

    selection.addChangeListener (this);
    pluginWindows.setKeyListener (&typing);

    typing.onOnChanged = [this] { updateTypingStrip(); };
    transport.onToggleTyping = [this] { commandManager.invokeDirectly (computerKeyboardCommand, false); };
    transport.onShowKeyGuide = [this] { openGuide (gen::guides::Guide::chords, 0); };
    transport.onShowTypingGuide = [this] { openGuide (gen::guides::Guide::midi, (int) midiguide::Topic::computerKeyboard); };
    updateTypingStrip();

    // Replacing the project deletes every track and clip, so nothing may point at them.
    project.beforeReplace = [this]
    {
        selection.select (nullptr);
        pluginWindows.closeAll();
    };
    project.afterReplace = [this]
    {
        const auto& tracks = engine.getTracks();

        if (! tracks.empty())
            selection.select (tracks.front().get(), tracks.front()->clips.empty() ? nullptr : tracks.front()->clips.front().get());
    };
    project.onTitleChanged = [this] (const juce::String& title)
    {
        if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
            window->setName (title);
    };

    commandManager.registerAllCommandsForTarget (this);
    commandManager.setFirstCommandTarget (this);
    addKeyListener (commandManager.getKeyMappings());
    setApplicationCommandManagerToWatch (&commandManager);

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #else
    addAndMakeVisible (menuBar);
   #endif

    guideprint::App printApp;
    printApp.engine = &engine;
    printApp.getProjectName = [this] { return project.getProjectName(); };
    printApp.getGuide = [this] (gen::guides::Guide guide) { return getPrintGuide (guide); };
    guideprint::setApp (std::move (printApp));

    guideprint::exportButtonFactory() = [] (guideprint::Source& source, const juce::String& plugin) -> std::unique_ptr<juce::Component>
    {
        if (const auto id = guideprint::findGuideForPlugin (plugin))
            return std::make_unique<guideprint::ExportButton> (source, *id);

        return nullptr;
    };

    setWantsKeyboardFocus (true);
    setSize (1440, 900);
}

MainComponent::~MainComponent()
{
    guideprint::setApp ({});
    guideprint::exportButtonFactory() = {};
    fxui::setHostTrackLookup ({});

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #endif

    selection.removeChangeListener (this);
    typing.onOnChanged = {};
    typing.releaseAll();
    pluginWindows.closeAll();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
   #if ! JUCE_MAC
    menuBar.setBounds (r.removeFromTop (juce::LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight()));
   #endif
    transport.setBounds (r.removeFromTop (transportHeight));

    if (typingStrip != nullptr)
        typingStrip->setBounds (r.removeFromBottom (ComputerKeyboardStrip::height));

    // The inspector takes the full height on the left; the tracks and panels get what is left.
    if (inspectorOpen)
    {
        const int width = juce::jlimit (InspectorPanel::minWidth, juce::jmax (InspectorPanel::minWidth, juce::jmin (inspectorWidth, getWidth() / 2)), inspectorWidth);
        inspector.setBounds (r.removeFromLeft (width));
        inspectorEdge.setBounds (r.removeFromLeft (InspectorPanel::edgeWidth));
    }

    if (bottomPanel == BottomPanel::closed)
    {
        // Without the panel the tracks take its room; the device bar stays at the bottom.
        deviceBar.setBounds (r.removeFromBottom (deviceBar.getPreferredHeight()));
        arrangement.setBounds (r);
        return;
    }

    const int barHeight = deviceBar.getPreferredHeight();
    layout.setItemLayout (2, barHeight, barHeight, barHeight);
    juce::Component* parts[] = { &arrangement, &divider, &deviceBar, &getBottomPanel() };
    layout.layOutComponents (parts, 4, r.getX(), r.getY(), r.getWidth(), r.getHeight(), true, true);
}

void MainComponent::setInspectorOpen (bool open)
{
    inspectorOpen = open;
    engine.properties.getUserSettings()->setValue ("view.inspectorOpen", open);
    inspector.setVisible (open);
    inspectorEdge.setVisible (open);
    resized();
    commandManager.commandStatusChanged();   // the View menu's tick
}

void MainComponent::showBottomPanel (BottomPanel panel)
{
    engine.properties.getUserSettings()->setValue ("ui.bottomPanel", panel == BottomPanel::mixer ? "mixer"
                                                                     : panel == BottomPanel::closed ? "closed" : "clip");
    bottomPanel = panel;
    updateBottomPanel();
    commandManager.commandStatusChanged();   // the View menu's ticks
}

juce::Component& MainComponent::getBottomPanel() noexcept
{
    if (bottomPanel == BottomPanel::mixer)
        return mixer;

    auto* clip = selection.getClip();
    return clip != nullptr && clip->kind == Clip::Kind::audio ? static_cast<juce::Component&> (waveEditor) : pianoRoll;
}

void MainComponent::updateBottomPanel()
{
    const bool open = bottomPanel != BottomPanel::closed;
    auto& shown = getBottomPanel();

    for (juce::Component* panel : { (juce::Component*) &pianoRoll, (juce::Component*) &waveEditor, (juce::Component*) &mixer })
        panel->setVisible (open && panel == &shown);

    divider.setVisible (open);
    transport.setMixerShown (bottomPanel == BottomPanel::mixer);
    resized();
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    return typing.handle (key) || shortcuts.handle (key);
}

bool MainComponent::keyStateChanged (bool)
{
    typing.keyStateChanged (false, this);
    return false;
}

void MainComponent::updateTypingStrip()
{
    if (typing.isOn() && typingStrip == nullptr)
    {
        typingStrip = std::make_unique<ComputerKeyboardStrip> (engine, typing);
        typingStrip->onClose = [this] { typing.setOn (false); };
        typingStrip->onShowGuide = [this] { openGuide (gen::guides::Guide::midi, (int) midiguide::Topic::computerKeyboard); };
        addAndMakeVisible (*typingStrip);
    }
    else if (! typing.isOn())
    {
        typingStrip.reset();
    }

    transport.setTypingOn (typing.isOn());
    resized();
    commandManager.commandStatusChanged();
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Held keys stop on the track they started on, before the live track changes.
    typing.releaseAll();

    // Note previews, and MIDI input while no track is armed, go to the selected track.
    engine.setLiveMidiTrack (selection.getTrack());

    // Create Bus needs something selected.
    commandManager.commandStatusChanged();

    // An audio clip shows in the wave editor, anything else in the piano roll.
    if (bottomPanel == BottomPanel::clipEditor && ! getBottomPanel().isVisible())
        updateBottomPanel();
}

void MainComponent::addTrack (Track::Type type)
{
    auto* track = engine.addTrack (type);

    // Give a new MIDI track an empty clip, ready to be drawn into.
    Clip* clip = type == Track::Type::midi ? engine.addMidiClip (*track, 0.0, 16.0) : nullptr;
    selection.select (track, clip);
}

void MainComponent::createDemoContent()
{
    auto* lead = engine.addTrack (Track::Type::midi);
    lead->name = "Lead";
    engine.setBuiltInSound (*lead, BuiltInSound::pluck);
    auto* clip = engine.addMidiClip (*lead, 0.0, 16.0);
    clip->name = "Arpeggio";

    const int chords[4][3] = { { 60, 64, 67 }, { 57, 60, 64 }, { 53, 57, 60 }, { 55, 59, 62 } };
    const juce::ScopedLock sl (engine.getLock());

    for (int bar = 0; bar < 4; ++bar)
        for (int step = 0; step < 8; ++step)
            clip->notes.push_back ({ chords[bar][step % 3] + (step >= 3 && step < 6 ? 12 : 0),
                                     bar * 4.0 + step * 0.5, 0.45, 0.55f + 0.05f * (float) (step % 4) });

    auto* bass = engine.addTrack (Track::Type::midi);
    bass->name = "Bass";
    engine.setBuiltInSound (*bass, BuiltInSound::bass);
    bass->volumeDb = -4.0f;
    bass->pan = -0.3f;
    auto* bassClip = engine.addMidiClip (*bass, 0.0, 16.0);
    bassClip->name = "Bassline";

    for (int bar = 0; bar < 4; ++bar)
        bassClip->notes.push_back ({ chords[bar][0] - 24, bar * 4.0, 3.5, 0.9f });

    engine.addTrack (Track::Type::audio)->name = "Drums";

    engine.loopStartBeat = 0.0;
    engine.loopEndBeat = 16.0;
    engine.loopEnabled = true;
    engine.notesChanged();
    selection.select (lead, clip);
}

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    commands.addArray ({ newProjectCommand, newFromStyleCommand, openProjectCommand, saveProjectCommand, saveProjectAsCommand, bounceCommand, exportChordSheetCommand, exportSheetMusicCommand, exportGuidesCommand,
                         importAudioCommand, showPluginsCommand, audioSettingsCommand,
                         showClipEditorCommand, showScoreCommand, toggleMixerCommand, signalFlowCommand, foldSongRowsCommand, playAlongCommand, singAlongCommand, tunerCommand,
                         theoryGuidesCommand, newAudioTrackCommand, newMidiTrackCommand, newBusCommand, createBusCommand, bounceInPlaceCommand,
                         newReverbReturnCommand, newDelayReturnCommand, newReturnCommand, computerKeyboardCommand, toggleInspectorCommand });
}

void MainComponent::getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info)
{
    const auto cmd = juce::ModifierKeys::commandModifier;

    switch (id)
    {
        case newProjectCommand:
            info.setInfo ("New Project", "Start an empty project", "File", 0);
            info.addDefaultKeypress ('n', cmd);
            break;
        case newFromStyleCommand:
            info.setInfo ("New Project from Style...", "Make a whole song sketch in one of fourteen styles", "File", 0);
            info.addDefaultKeypress ('n', cmd | juce::ModifierKeys::shiftModifier);
            break;
        case openProjectCommand:
            info.setInfo ("Open...", "Open a project", "File", 0);
            info.addDefaultKeypress ('o', cmd);
            break;
        case saveProjectCommand:
            info.setInfo ("Save", "Save the project", "File", 0);
            info.addDefaultKeypress ('s', cmd);
            break;
        case saveProjectAsCommand:
            info.setInfo ("Save As...", "Save the project under a new name", "File", 0);
            info.addDefaultKeypress ('s', cmd | juce::ModifierKeys::shiftModifier);
            break;
        case bounceCommand:
            info.setInfo ("Bounce to WAV...", "Render the mix, or the selected tracks as you hear them soloed, to a WAV file", "File", 0);
            info.addDefaultKeypress ('b', cmd);
            break;
        case exportChordSheetCommand:
            info.setInfo ("Export Chord Sheet (PDF)...", "Print the chords of a key and its progressions for piano or guitar", "File", 0);
            break;
        case exportSheetMusicCommand:
            info.setInfo ("Export Sheet Music (PDF)...", "Save the selected MIDI track as sheet music to print or read on a tablet", "File", 0);
            info.setActive (selection.getTrack() != nullptr && selection.getTrack()->isMidi());
            break;
        case exportGuidesCommand:
            info.setInfo ("Export Theory Guides (PDF)...", "Save guides as a booklet to print: Start here, an area or all of them", "File", 0);
            break;
        case importAudioCommand:
            info.setInfo ("Import Audio...", "Import audio files at the playhead, onto the selected audio track (or a new one)", "File", 0);
            info.addDefaultKeypress ('i', cmd);
            break;
        case showPluginsCommand:
           #if JUCE_PLUGINHOST_AU
            info.setInfo ("Plugins...", "Scan for and manage VST3 and Audio Unit plugins", "File", 0);
           #else
            info.setInfo ("Plugins...", "Scan for and manage VST3 plugins", "File", 0);
           #endif
            break;
        case audioSettingsCommand:
            info.setInfo ("Audio/MIDI Settings...", "Audio device and MIDI input settings", "File", 0);
            info.addDefaultKeypress (',', cmd);
            break;
        case showClipEditorCommand:
            info.setInfo ("Clip Editor", "Show the selected clip under the tracks, as a piano roll", "View", 0);
            info.setTicked (bottomPanel == BottomPanel::clipEditor && ! pianoRoll.isScoreMode());
            info.addDefaultKeypress ('1', cmd);
            break;
        case showScoreCommand:
            info.setInfo ("Score", "Show the selected MIDI track under the tracks as sheet music", "View", 0);
            info.setTicked (bottomPanel == BottomPanel::clipEditor && pianoRoll.isScoreMode());
            info.addDefaultKeypress ('8', cmd);
            break;
        case toggleMixerCommand:
            info.setInfo ("Mixer", "Show the volume, pan and level of every track under the tracks", "View", 0);
            info.setTicked (bottomPanel == BottomPanel::mixer);
            info.addDefaultKeypress ('2', cmd);
            break;
        case toggleInspectorCommand:
            info.setInfo ("Inspector", "Show the selected track's effects, sends, pan and volume on the left of the tracks", "View", 0);
            info.setTicked (inspectorOpen);
            info.addDefaultKeypress ('i', 0);
            break;
        case signalFlowCommand:
            info.setInfo ("Signal Flow...", "A live map of how the sound travels through the whole song: every track, effect, bus, send and return on its way to the master",
                          "View", 0);
            info.addDefaultKeypress ('6', cmd);
            break;
        case foldSongRowsCommand:
            info.setInfo ("Fold Sections, Chords and Lyrics", "Show the Sections, Chords and Lyrics rows as a thin coloured line each, "
                          "to give the tracks the room", "View", 0);
            info.setTicked (arrangement.areSongRowsFolded());
            info.addDefaultKeypress ('3', cmd);
            break;
        case playAlongCommand:
            info.setInfo ("Play Along with the Chords...", "Show the chord playing now large on piano and guitar, with the next one beside it, "
                          "to play along on a real instrument", "View", 0);
            info.addDefaultKeypress ('4', cmd);
            break;
        case singAlongCommand:
            info.setInfo ("Sing Along with the Lyrics...", "Show the line to sing now in large letters, coloured as it's sung, "
                          "with the next line under it, to read while you record vocals", "View", 0);
            info.addDefaultKeypress ('5', cmd);
            break;
        case computerKeyboardCommand:
            info.setInfo ("Play with Computer Keyboard", "Play the selected track with the letter keys, A to L and W to P", "View", 0);
            info.setTicked (typing.isOn());
            info.addDefaultKeypress ('k', cmd);
            break;
        case tunerCommand:
            info.setInfo ("Tuner...", "Listen to a guitar, bass or other instrument on an audio input and see which note it plays and how far off it is",
                          "View", 0);
            info.addDefaultKeypress ('7', cmd);
            break;
        case newAudioTrackCommand:
            info.setInfo ("New Audio Track", "Add an audio track at the end", "Track", 0);
            break;
        case newMidiTrackCommand:
            info.setInfo ("New MIDI Track", "Add a MIDI track at the end, with an empty clip to draw into", "Track", 0);
            break;
        case newBusCommand:
            info.setInfo ("New Bus", "Add an empty bus below the selected track: a track without clips that other tracks play into", "Track", 0);
            break;
        case createBusCommand:
            info.setInfo ("Create Bus", "Put the selected tracks in a new bus, to add effects to them and turn them up or down together", "Track", 0);
            info.setActive (arrangement.getTrackActions().canCreateBus());
            info.addDefaultKeypress ('g', cmd | juce::ModifierKeys::shiftModifier);
            break;
        case bounceInPlaceCommand:
            info.setInfo ("Bounce in Place", "Render the selected clip, or the selected track, through its instrument and effects onto a new audio track below, and mute the original", "Track", 0);
            info.setActive (! engine.isRecording() && selection.getTrack() != nullptr && (selection.getTrack()->isAudio() || selection.getTrack()->isMidi()));
            info.addDefaultKeypress ('b', cmd | juce::ModifierKeys::shiftModifier);
            break;
        case newReverbReturnCommand:
            info.setInfo ("Reverb", "Add a return track with DawIt Reverb: tracks send a copy of themselves to it, each at its own level", "Track", 0);
            break;
        case newDelayReturnCommand:
            info.setInfo ("Delay", "Add a return track with DawIt Delay: tracks send a copy of themselves to it, each at its own level", "Track", 0);
            break;
        case newReturnCommand:
            info.setInfo ("Empty", "Add a return track with no effect yet: tracks send a copy of themselves to it, each at its own level", "Track", 0);
            break;
        case theoryGuidesCommand:
            info.setInfo ("Theory Guides...", "Every guide in DawIt in one place: sound, MIDI, rhythm, chords, songwriting, mixing, "
                                              "effects and synthesis, with a path for beginners and a search", "View", 0);
            info.addDefaultKeypress ('g', cmd);
            break;
        default:
            break;
    }
}

bool MainComponent::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case newProjectCommand:    project.newProject(); return true;
        case newFromStyleCommand:  showNewFromStyle(); return true;
        case openProjectCommand:   project.open(); return true;
        case saveProjectCommand:   project.save(); return true;
        case saveProjectAsCommand: project.saveAs(); return true;
        case bounceCommand:        bounceui::showDialog (engine, project.getProjectName(), selection.getTracks()); return true;
        case exportChordSheetCommand: chordsheet::showDialog (engine, engine.getKey()); return true;
        case exportSheetMusicCommand: sheetmusic::showDialog (engine, selection.getTrack(), project.getProjectName(), [this] { showReadingGuide(); }); return true;
        case exportGuidesCommand:  guideprint::exportGuides (gen::guides::getStartPath(), "Start here"); return true;
        case importAudioCommand:   arrangement.importAudio(); return true;
        case showPluginsCommand:   showPluginList(); return true;
        case audioSettingsCommand: showAudioSettings(); return true;
        case showClipEditorCommand:
            pianoRoll.setScoreMode (false);
            showBottomPanel (BottomPanel::clipEditor);
            return true;
        case showScoreCommand:
            pianoRoll.setScoreMode (true);
            showBottomPanel (BottomPanel::clipEditor);
            return true;
        case toggleMixerCommand:
            // Turning the mixer off goes back to the clip editor.
            showBottomPanel (bottomPanel == BottomPanel::mixer ? BottomPanel::clipEditor : BottomPanel::mixer);
            return true;
        case toggleInspectorCommand: setInspectorOpen (! inspectorOpen); return true;
        case signalFlowCommand:    showSignalFlow(); return true;
        case foldSongRowsCommand:  arrangement.setSongRowsFolded (! arrangement.areSongRowsFolded()); return true;
        case playAlongCommand:     arrangement.showPlayAlong(); return true;
        case singAlongCommand:     arrangement.showSingAlong(); return true;
        case tunerCommand:         showTuner(); return true;
        case computerKeyboardCommand: typing.setOn (! typing.isOn()); return true;
        case theoryGuidesCommand:  showTheoryGuides(); return true;
        case newAudioTrackCommand: addTrack (Track::Type::audio); return true;
        case newMidiTrackCommand:  addTrack (Track::Type::midi); return true;
        case newBusCommand:        arrangement.getTrackActions().addEmptyBus(); return true;
        case createBusCommand:     arrangement.createBusFromSelection(); return true;
        case bounceInPlaceCommand:
            if (auto* track = selection.getTrack())
                bounceui::bounceInPlace (engine, selection, *track, selection.getClip());
            return true;
        case newReverbReturnCommand: arrangement.getTrackActions().addReturn (TrackActions::ReturnKind::reverb); return true;
        case newDelayReturnCommand:  arrangement.getTrackActions().addReturn (TrackActions::ReturnKind::delay); return true;
        case newReturnCommand:       arrangement.getTrackActions().addReturn (TrackActions::ReturnKind::empty); return true;
        default:                   return false;
    }
}

juce::PopupMenu MainComponent::getMenuForIndex (int index, const juce::String&)
{
    if (index == 1)
    {
        juce::PopupMenu track;
        track.addCommandItem (&commandManager, newAudioTrackCommand);
        track.addCommandItem (&commandManager, newMidiTrackCommand);
        track.addCommandItem (&commandManager, newBusCommand);
        track.addSeparator();
        track.addCommandItem (&commandManager, createBusCommand);
        track.addSeparator();
        track.addCommandItem (&commandManager, bounceInPlaceCommand);
        track.addSeparator();

        juce::PopupMenu returns;
        returns.addCommandItem (&commandManager, newReverbReturnCommand);
        returns.addCommandItem (&commandManager, newDelayReturnCommand);
        returns.addCommandItem (&commandManager, newReturnCommand);
        track.addSubMenu ("New Return Track", returns);
        return track;
    }

    if (index == 2)
    {
        juce::PopupMenu view;
        view.addCommandItem (&commandManager, showClipEditorCommand);
        view.addCommandItem (&commandManager, showScoreCommand);
        view.addCommandItem (&commandManager, toggleMixerCommand);
        view.addCommandItem (&commandManager, toggleInspectorCommand);
        view.addCommandItem (&commandManager, signalFlowCommand);
        view.addSeparator();
        view.addCommandItem (&commandManager, foldSongRowsCommand);
        view.addSeparator();
        view.addCommandItem (&commandManager, playAlongCommand);
        view.addCommandItem (&commandManager, singAlongCommand);
        view.addCommandItem (&commandManager, tunerCommand);
        view.addCommandItem (&commandManager, computerKeyboardCommand);
        view.addSeparator();
        view.addCommandItem (&commandManager, theoryGuidesCommand);
        return view;
    }

    if (index == 3)
    {
        juce::PopupMenu help;
        help.addCommandItem (&commandManager, theoryGuidesCommand);
        return help;
    }

    juce::PopupMenu recent;
    project.getRecentFiles().createPopupMenuItems (recent, recentProjectsBaseId, false, true);

    juce::PopupMenu menu;
    menu.addCommandItem (&commandManager, newProjectCommand);
    menu.addCommandItem (&commandManager, newFromStyleCommand);
    menu.addCommandItem (&commandManager, openProjectCommand);
    menu.addSubMenu ("Open Recent", recent, recent.containsAnyActiveItems());
    menu.addSeparator();
    menu.addCommandItem (&commandManager, saveProjectCommand);
    menu.addCommandItem (&commandManager, saveProjectAsCommand);
    menu.addSeparator();
    menu.addCommandItem (&commandManager, importAudioCommand);
    menu.addCommandItem (&commandManager, bounceCommand);
    menu.addCommandItem (&commandManager, exportChordSheetCommand);
    menu.addCommandItem (&commandManager, exportSheetMusicCommand);
    menu.addCommandItem (&commandManager, exportGuidesCommand);
    menu.addSeparator();
    menu.addCommandItem (&commandManager, showPluginsCommand);
    menu.addCommandItem (&commandManager, audioSettingsCommand);
    return menu;
}

void MainComponent::menuItemSelected (int itemId, int)
{
    if (itemId >= recentProjectsBaseId && itemId < recentProjectsBaseId + project.getRecentFiles().getNumFiles())
        project.openFile (project.getRecentFiles().getFile (itemId - recentProjectsBaseId));
}

void MainComponent::showPluginList()
{
    if (pluginListWindow == nullptr)
        pluginListWindow = std::make_unique<PluginListWindow> (engine, typing);

    pluginListWindow->setVisible (true);
    pluginListWindow->toFront (true);
}

void MainComponent::showRhythmGuide()
{
    pluginWindows.show (&rhythmGuide, "Note lengths, beats and bars", [this]
    {
        auto* guide = new RhythmExplainer (engine);
        rhythmGuide = guide;
        return guide;
    });
}

void MainComponent::showReadingGuide()
{
    pluginWindows.show (&readingGuide, "How to read music", [this]
    {
        auto* guide = new ReadingExplainer (engine, selection);
        readingGuide = guide;
        return guide;
    });
}

void MainComponent::showSoundGuide()
{
    pluginWindows.show (&soundGuide, "What is sound?", [this]
    {
        auto* guide = new SoundExplainer (engine, [this] { return SoundExplainer::ClipView { waveEditor.getClip(), waveEditor.getFocusSample() }; });
        guide->onRemoveVocals = [this] (const AudioClip& clip) { arrangement.removeVocals (clip); };
        guide->onOpenStereoGuide = [this]
        {
            // By the page's name, so the stereo guide's pages can be reordered.
            const auto page = BuiltInGuides::getPageNames ("DawIt Stereo").indexOf ("Mono: phones, clubs and the sum");
            openGuide (gen::guides::Guide::stereo, page);
        };
        soundGuide = guide;
        return guide;
    });
}

void MainComponent::showMidiGuide()
{
    pluginWindows.show (&midiGuide, "What is MIDI?", [this]
    {
        auto* guide = new MidiExplainer (engine, [this] { return MidiExplainer::ClipView { pianoRoll.getTrack(), pianoRoll.getClip() }; }, &typing);
        midiGuide = guide;
        return guide;
    });
}

void MainComponent::showTuner()
{
    pluginWindows.show (&tunerWindow, "Tuner", [this]
    {
        auto* window = new TunerWindow (engine, selection);
        window->onOpenGuide = [this] { showTuningGuide (gen::tuning::Topic::whatItShows); };
        tunerWindow = window;
        return window;
    }, { 460, 380 });
}

void MainComponent::showRecordingGuide (recordingguide::Topic topic)
{
    pluginWindows.show (&recordingGuide, "How recording works", [this]
    {
        auto* guide = new RecordingExplainer (engine);
        recordingGuide = guide;
        return guide;
    });

    if (recordingGuide != nullptr)
        recordingGuide->showTopic (topic);
}

void MainComponent::showTriggerGuide (triggerguide::Topic topic)
{
    pluginWindows.show (&triggerGuide, "How drum triggers work", [this]
    {
        auto* guide = new DrumTriggerExplainer (engine, [this] { return getTriggerGuidePart(); });
        triggerGuide = guide;
        return guide;
    });

    if (triggerGuide != nullptr)
        triggerGuide->showTopic (topic);
}

std::optional<triggerguide::Part> MainComponent::getTriggerGuidePart()
{
    // The open Hits to MIDI window's hits first, then the selected drum clip.
    if (auto* window = arrangement.getHitsToMidi().getCurrentWindow())
        if (auto part = window->getPart(); ! part.hits.empty())
            return part;

    if (selection.getTrack() != nullptr && selection.getMidiClip() != nullptr)
        return DrumTriggerExplainer::getPartForClip (engine, *selection.getTrack(), *selection.getMidiClip());

    return std::nullopt;
}

void MainComponent::showTuningGuide (gen::tuning::Topic topic)
{
    pluginWindows.show (&tuningGuide, "How to tune a guitar", [this]
    {
        auto* guide = new TuningExplainer (engine);
        tuningGuide = guide;
        return guide;
    });

    if (tuningGuide != nullptr)
        tuningGuide->showTopic (topic);
}

void MainComponent::showRoutingGuide (routingguide::Topic topic)
{
    pluginWindows.show (&routingGuide, "Buses, sends and routing", [this]
    {
        auto* guide = new RoutingExplainer (engine, selection);
        guide->onCreateBus = [this] (std::vector<Track*> tracks, juce::String name, Track* beside) { arrangement.getTrackActions().createBus (tracks, name, beside); };
        guide->onMoveIntoBus = [this] (std::vector<Track*> tracks, Track& bus) { arrangement.getTrackActions().moveIntoBus (tracks, bus); };
        guide->onAddGlue = [this] (Track& bus) { arrangement.getTrackActions().addGlueCompressor (bus); };
        guide->onShareEffect = [this] (std::vector<Track*> tracks, juce::String effect, Track* existing) { arrangement.getTrackActions().shareOnReturn (tracks, effect, existing); };
        guide->onAddReturnEffect = [this] (Track& ret, juce::String effect) { arrangement.getTrackActions().addReturnEffect (ret, effect); };
        guide->onMakeWet = [this] (Track& ret, juce::String effect) { arrangement.getTrackActions().makeReturnWet (ret, effect); };
        guide->onMoveEffect = [this] (Track& track, const PluginSlot* slot, int index) { engine.movePlugin (track, slot, index); };
        guide->onShowSignalFlow = [this] { showSignalFlow(); };
        routingGuide = guide;
        return guide;
    });

    if (routingGuide != nullptr)
        routingGuide->showTopic (topic);
}

void MainComponent::showSignalFlow()
{
    pluginWindows.show (&signalFlow, "Signal Flow", [this]
    {
        auto* view = new SignalFlowView (engine, selection);
        view->onShowGuide = [this] { showRoutingGuide (routingguide::Topic::signalFlow); };
        signalFlow = view;
        return view;
    }, { 640, 420 });
}

void MainComponent::showNewFromStyle (const juce::String& styleId)
{
    pluginWindows.show (&newFromStyle, "New Project from Style", [this]
    {
        auto* view = new NewFromStyleWindow (engine, project);
        view->onShowGuide = [this] (gen::stylesguide::Topic topic) { showStylesGuide (topic); };
        view->onShowKeyGuide = [this] { openGuide (gen::guides::Guide::chords, 0); };
        newFromStyle = view;
        return view;
    });

    if (newFromStyle != nullptr && styleId.isNotEmpty())
        newFromStyle->showStyle (styleId);
}

void MainComponent::showStylesGuide (gen::stylesguide::Topic topic)
{
    pluginWindows.show (&stylesGuide, "Styles of music", [this]
    {
        auto* guide = new StylesExplainer (engine);
        guide->onNewSong = [this] (const juce::String& styleId) { showNewFromStyle (styleId); };
        stylesGuide = guide;
        return guide;
    });

    if (stylesGuide != nullptr)
        stylesGuide->showTopic (topic);
}

void MainComponent::exportAllGuides (const juce::File& folder, std::function<void (bool)> done)
{
    folder.createDirectory();

    struct Job { juce::String name; std::vector<gen::guides::Guide> guides; };
    auto jobs = std::make_shared<std::vector<Job>>();
    auto entries = std::make_shared<std::vector<gen::guides::Entry>> (BuiltInGuides::getEntries());

    for (auto guide : gen::guides::allGuides)
        jobs->push_back ({ gen::guides::getEntry (guide).name, { guide } });

    jobs->push_back ({ "Start here", gen::guides::getStartPath() });

    auto ok = std::make_shared<bool> (true);
    auto step = std::make_shared<std::function<void (size_t)>>();
    *step = [this, folder, jobs, entries, ok, step, done] (size_t index)
    {
        if (index >= jobs->size())
        {
            done (*ok);
            return;
        }

        const auto& job = (*jobs)[index];
        std::vector<guideprint::Item> items;

        for (auto guide : job.guides)
            items.push_back (getPrintGuide (guide));

        const bool isSet = job.guides.size() > 1;
        const auto file = folder.getChildFile (juce::File::createLegalFileName (isSet ? "DawIt guides - " + job.name : job.name) + ".pdf");
        const auto started = juce::Time::getMillisecondCounterHiRes();

        guideprint::exportToFile (std::move (items), isSet ? job.name : juce::String(), isSet ? guideprint::describeSet (job.name, job.guides) : juce::String(),
                                  booklet::Options(), file,
                                  [step, index, jobs, entries, ok, file, started] (juce::Result result, const guideprint::Report& report)
                                  {
                                      const auto& finished = (*jobs)[index];
                                      int topics = 0;

                                      for (auto& names : report.topicNames)
                                          topics += names.size();

                                      bool namesRight = true;

                                      for (size_t g = 0; g < finished.guides.size() && g < report.topicNames.size(); ++g)
                                          for (auto& e : *entries)
                                          {
                                              if (e.guide != finished.guides[g] || e.topics.isEmpty())
                                                  continue;

                                              // The chord and structure guides list their series of progressions and structures as one topic.
                                              const bool grouped = report.topicNames[g].size() > e.topics.size();
                                              const int same = grouped ? e.topics.size() - 1 : e.topics.size();

                                              for (int t = 0; t < same; ++t)
                                                  namesRight = namesRight && e.topics[t] == report.topicNames[g][t];

                                              namesRight = namesRight && (grouped || report.topicNames[g].size() == e.topics.size());
                                          }

                                      const bool good = result.wasOk() && report.topicsOnOwnPages && namesRight;
                                      *ok = *ok && good;
                                      std::cout << finished.name << ": " << topics << " topics, " << report.pages << " pages, "
                                                << juce::String ((double) file.getSize() / (1024.0 * 1024.0), 1) << " MB, "
                                                << juce::String ((juce::Time::getMillisecondCounterHiRes() - started) / 1000.0, 1) << " s"
                                                << (good ? "" : "  FAILED: " + (result.failed() ? result.getErrorMessage()
                                                                                                    : juce::String (namesRight ? "a topic doesn't start its own page" : "the topics differ from the Theory Guides list")))
                                                << std::endl;
                                      (*step) (index + 1);
                                  });
    };

    (*step) (0);
}

guideprint::Item MainComponent::getPrintGuide (gen::guides::Guide guide)
{
    using G = gen::guides::Guide;
    guideprint::Item item;
    item.guide = guide;

    switch (guide)
    {
        case G::readMusic:
            if (readingGuide != nullptr)
                item.useOpen (readingGuide.getComponent());
            else
                item.adopt (std::make_unique<ReadingExplainer> (engine, selection));
            break;

        case G::sound:
            if (soundGuide != nullptr)
                item.useOpen (soundGuide.getComponent());
            else
                item.adopt (std::make_unique<SoundExplainer> (engine, [this] { return SoundExplainer::ClipView { waveEditor.getClip(), waveEditor.getFocusSample() }; }));
            break;

        case G::midi:
            if (midiGuide != nullptr)
                item.useOpen (midiGuide.getComponent());
            else
                item.adopt (std::make_unique<MidiExplainer> (engine, [this] { return MidiExplainer::ClipView { pianoRoll.getTrack(), pianoRoll.getClip() }; }));
            break;

        case G::rhythm:
            if (rhythmGuide != nullptr)
                item.useOpen (rhythmGuide.getComponent());
            else
                item.adopt (std::make_unique<RhythmExplainer> (engine));
            break;

        case G::chords:
        case G::chordShapes:
        case G::structure:
        case G::lyrics:
        case G::singing:
            return arrangement.getPrintGuide (guide);

        case G::styles:
            if (stylesGuide != nullptr)
                item.useOpen (stylesGuide.getComponent());
            else
                item.adopt (std::make_unique<StylesExplainer> (engine));
            break;

        case G::gainStaging:
            return deviceBar.getPrintGuide();

        case G::routing:
            if (routingGuide != nullptr)
                item.useOpen (routingGuide.getComponent());
            else
                item.adopt (std::make_unique<RoutingExplainer> (engine, selection));

            break;

        case G::recording:
            if (recordingGuide != nullptr)
                item.useOpen (recordingGuide.getComponent());
            else
                item.adopt (std::make_unique<RecordingExplainer> (engine));

            break;

        case G::triggers:
            if (triggerGuide != nullptr)
                item.useOpen (triggerGuide.getComponent());
            else
                item.adopt (std::make_unique<DrumTriggerExplainer> (engine, [this] { return getTriggerGuidePart(); }));

            break;

        case G::tuning:
            if (tuningGuide != nullptr)
                item.useOpen (tuningGuide.getComponent());
            else
                item.adopt (std::make_unique<TuningExplainer> (engine));

            break;

        case G::eq:
        case G::compressor:
        case G::gate:
        case G::tune:
        case G::stereo:
        case G::reverb:
        case G::delay:
        case G::saturation:
        case G::distortion:
        case G::subtractive:
        case G::additive:
        case G::fm:
        case G::drums:
        case G::physical:
            return builtInGuides.getPrintGuide (gen::guides::getEntry (guide).plugin);
    }

    return item;
}

void MainComponent::showTheoryGuides()
{
    pluginWindows.show (&theoryGuides, "Theory Guides", [this]
    {
        auto* view = new TheoryGuidesView (BuiltInGuides::getEntries());
        view->onOpen = [this] (gen::guides::Guide guide, int topic) { openGuide (guide, topic); };
        view->describeTarget = [this] (const gen::guides::Entry& entry) { return builtInGuides.describeTarget (entry.plugin); };
        theoryGuides = view;
        return view;
    });
}

void MainComponent::openGuide (gen::guides::Guide guide, int topic)
{
    using G = gen::guides::Guide;

    switch (guide)
    {
        case G::readMusic:
            showReadingGuide();

            if (readingGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (gen::readmusic::allTopics)))
                readingGuide->showTopic (gen::readmusic::allTopics[topic]);
            break;

        case G::sound:
            showSoundGuide();

            if (soundGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (soundguide::allTopics)))
                soundGuide->showTopic (soundguide::allTopics[topic]);
            break;

        case G::midi:
            showMidiGuide();

            if (midiGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (midiguide::allTopics)))
                midiGuide->showTopic (midiguide::allTopics[topic]);
            break;

        case G::rhythm:
            showRhythmGuide();

            if (rhythmGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (gen::rhythm::allTopics)))
                rhythmGuide->showTopic (gen::rhythm::allTopics[topic]);
            break;

        case G::chords:
        case G::chordShapes:
        case G::structure:
        case G::lyrics:
        case G::singing:
            arrangement.showGuide (guide, topic);
            break;

        case G::styles:
            showStylesGuide (juce::isPositiveAndBelow (topic, (int) std::size (gen::stylesguide::allTopics))
                                 ? gen::stylesguide::allTopics[topic] : gen::stylesguide::Topic::styles);
            break;

        case G::gainStaging:
            deviceBar.showGuide (juce::isPositiveAndBelow (topic, (int) std::size (gainstaging::allTopics))
                                     ? gainstaging::allTopics[topic] : gainstaging::Topic::what);
            break;

        case G::routing:
            showRoutingGuide (juce::isPositiveAndBelow (topic, (int) std::size (routingguide::allTopics)) ? routingguide::allTopics[topic] : routingguide::Topic::what);
            break;

        case G::recording:
            showRecordingGuide (juce::isPositiveAndBelow (topic, (int) std::size (recordingguide::allTopics)) ? recordingguide::allTopics[topic] : recordingguide::Topic::air);
            break;

        case G::triggers:
            showTriggerGuide (juce::isPositiveAndBelow (topic, (int) std::size (triggerguide::allTopics)) ? triggerguide::allTopics[topic] : triggerguide::Topic::whatItIs);
            break;

        case G::tuning:
            showTuningGuide (juce::isPositiveAndBelow (topic, (int) std::size (gen::tuning::allTopics)) ? gen::tuning::allTopics[topic] : gen::tuning::Topic::whatItShows);
            break;

        case G::eq:
        case G::compressor:
        case G::gate:
        case G::tune:
        case G::stereo:
        case G::reverb:
        case G::delay:
        case G::saturation:
        case G::distortion:
        case G::subtractive:
        case G::additive:
        case G::fm:
        case G::drums:
        case G::physical:
            builtInGuides.open (gen::guides::getEntry (guide).plugin, topic);
            break;
    }
}

void MainComponent::showAudioSettings()
{
    auto* panel = new AudioSettingsPanel (engine);
    panel->onShowGuide = [this] { showRecordingGuide (recordingguide::Topic::buffers); };

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (panel);
    options.dialogTitle = "Audio / MIDI Settings";
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}
} // namespace daw
