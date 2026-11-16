#include "MainComponent.h"
#include "engine/Routing.h"
#include "PluginActions.h"
#include "engine/PanelBudget.h"
#include "engine/Stems.h"
#include "engine/ModelDownload.h"
#include "analysis/LyricsFinder.h"
#include "HitsToMidiWindow.h"
#include "AudioSettingsPanel.h"
#include "AdditiveEditor.h"
#include "BounceActions.h"
#include "BounceDialog.h"
#include "QuantiseDialog.h"
#include "LoudnessActions.h"
#include "ChordSheetExport.h"
#include "SheetMusicExport.h"
#include "GuidePrint.h"
#include "CompressorEditor.h"
#include "DelayEditor.h"
#include "ArpeggiatorEditor.h"
#include "SlicerEditor.h"
#include "SpectralFreezeEditor.h"
#include "StepFilterEditor.h"
#include "DistortionEditor.h"
#include "GateEditor.h"
#include "StereoEditor.h"
#include "HarmoniserEditor.h"
#include "TuneEditor.h"
#include "MeterEditor.h"
#include "DrumsEditor.h"
#include "PhysicalEditor.h"
#include "SamplerEditor.h"
#include "SamplerActions.h"
#include "SamplerGuide.h"
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
    // Older versions kept each exercise's step in the settings; it is in memory now, so drop the stale keys.
    for (auto exercise : gen::exercises::allExercises)
        for (auto& key : gen::exercises::getStaleSettingKeys (exercise))
            engine.properties.getUserSettings()->removeValue (key);

    // The built-in effects live in the engine; their windows are part of the app.
    BuiltInEq::createEditorFunction = [] (BuiltInEq& eqProcessor) { return new BuiltInEqEditor (eqProcessor); };
    BuiltInCompressor::createEditorFunction = [] (BuiltInCompressor& compressor) { return new BuiltInCompressorEditor (compressor); };
    BuiltInReverb::createEditorFunction = [] (BuiltInReverb& reverb) { return new BuiltInReverbEditor (reverb); };
    BuiltInDelay::createEditorFunction = [] (BuiltInDelay& delay) { return new BuiltInDelayEditor (delay); };
    BuiltInSlicer::createEditorFunction = [] (BuiltInSlicer& slicer) { return new BuiltInSlicerEditor (slicer); };
    BuiltInArpeggiator::createEditorFunction = [] (BuiltInArpeggiator& arpeggiator) { return new BuiltInArpeggiatorEditor (arpeggiator); };
    BuiltInStepFilter::createEditorFunction = [] (BuiltInStepFilter& filter) { return new BuiltInStepFilterEditor (filter); };
    BuiltInSpectralFreeze::createEditorFunction = [] (BuiltInSpectralFreeze& freeze) { return new BuiltInSpectralFreezeEditor (freeze); };
    BuiltInSaturation::createEditorFunction = [] (BuiltInSaturation& saturation) { return new BuiltInSaturationEditor (saturation); };
    BuiltInStereo::createEditorFunction = [] (BuiltInStereo& stereo) { return new BuiltInStereoEditor (stereo); };
    BuiltInTune::createEditorFunction = [] (BuiltInTune& tuner) { return new BuiltInTuneEditor (tuner); };
    BuiltInMeter::createEditorFunction = [] (BuiltInMeter& meter) { return new BuiltInMeterEditor (meter); };
    BuiltInHarmoniser::createEditorFunction = [] (BuiltInHarmoniser& harmoniser) { return new BuiltInHarmoniserEditor (harmoniser); };
    BuiltInGate::createEditorFunction = [] (BuiltInGate& gate) { return new BuiltInGateEditor (gate); };
    BuiltInDistortion::createEditorFunction = [] (BuiltInDistortion& distortion) { return new BuiltInDistortionEditor (distortion); };
    BuiltInSubtractive::createEditorFunction = [] (BuiltInSubtractive& synth) { return new BuiltInSubtractiveEditor (synth); };
    BuiltInAdditive::createEditorFunction = [] (BuiltInAdditive& synth) { return new BuiltInAdditiveEditor (synth); };
    BuiltInFm::createEditorFunction = [] (BuiltInFm& synth) { return new BuiltInFmEditor (synth); };
    BuiltInDrums::createEditorFunction = [] (BuiltInDrums& machine) { return new BuiltInDrumsEditor (machine); };
    BuiltInPhysical::createEditorFunction = [] (BuiltInPhysical& synth) { return new BuiltInPhysicalEditor (synth); };
    BuiltInSampler::createEditorFunction = [this] (BuiltInSampler& synth) { return new BuiltInSamplerEditor (synth, engine); };

    guideNavigator.onShowGuides = [this] { showTheoryGuides(); };
    bounceui::setGuideOpener ([this] (bool files) { openGuide (gen::guides::Guide::handover, files ? 2 : 1); });
    bounceui::setFinishedListener ([this] (const gen::mixexport::ExportRecord& record) { mixExported = record; });
    sampleractions::setGuideOpener ([this] { openGuide (gen::guides::Guide::sampler, 0); });

    fxui::setHostTrackLookup ([this] (const juce::AudioProcessor& processor)
    {
        const juce::ScopedLock lock (engine.getLock());
        for (const auto& track : engine.getTracks())
            for (const auto& slot : track->plugins)
                if (slot->instance.get() == &processor)
                    return fxui::HostTrack { track->name, track->isReturn(), track->isBus(), engine.getKey().getName(), track->isMidi() };
        return fxui::HostTrack {};
    });

    samplertext::setTrackLookup ([this] (const juce::AudioProcessor& processor)
    {
        samplertext::TrackNotes found;
        const juce::ScopedLock lock (engine.getLock());

        for (const auto& track : engine.getTracks())
            for (const auto& slot : track->plugins)
                if (slot->instance.get() == &processor)
                {
                    found.found = true;
                    found.trackName = track->name;

                    for (const auto& clip : track->clips)
                        if (clip->kind == Clip::Kind::midi && ! clip->muted)
                            for (const auto& n : static_cast<const MidiClip&> (*clip).notes)
                                found.notes.push_back ({ n.pitch, juce::jlimit (1, 127, juce::roundToInt (n.velocity * 127.0f)) });

                    return found;
                }

        return found;
    });

    tunetext::setSongAccess ({
        [this]
        {
            tunetext::SongInfo info;
            info.key = engine.getKey();
            info.sections = engine.getSections();
            info.tempo = engine.getTempoMap();
            info.timeSignature = engine.getTimeSignature();
            info.numBars = juce::jmax (8, engine.getSongEndBar());
            return info;
        },
        [this] (double beat) { engine.setPositionBeats (beat); },
        [this] { return engine.properties.getUserSettings(); } });

    metertext::setSongAccess ({
        [] { return loudnessui::getLast(); },
        [this] { measureLoudness(); } });

    addAndMakeVisible (transport);
    addAndMakeVisible (inspector);
    addAndMakeVisible (inspectorEdge);
    addAndMakeVisible (arrangement);
    addAndMakeVisible (divider);
    addAndMakeVisible (deviceBar);
    addAndMakeVisible (pianoRoll);
    addChildComponent (waveEditor);
    addChildComponent (mixer);
    addChildComponent (pointer);

    layout.setItemLayout (0, 150, -1.0, -0.58);
    layout.setItemLayout (1, 6, 6, 6);
    layout.setItemLayout (2, DeviceBar::preferredHeight, DeviceBar::preferredHeight, DeviceBar::preferredHeight);
    deviceBar.onPreferredHeightChanged = [this] { resized(); };
    layout.setItemLayout (3, 120, -1.0, -0.42);

    transport.onAddAudioTrack = [this] { addTrack (Track::Type::audio); };
    transport.onAddMidiTrack = [this] { addTrack (Track::Type::midi); };
    transport.onAddBus = [this] { arrangement.getTrackActions().addEmptyBus(); };
    transport.onAddReturn = [this] (juce::Component& button) { arrangement.getTrackActions().showNewReturnMenu (&button); };
    arrangement.onShowHandoverGuide = [this] { showHandoverGuide (handoverguide::Topic::stems); };
    arrangement.onShowInstrumentsGuide = [this] (Track& t) { showInstrumentsGuide (instrumentsui::getTopicFor (engine, t)); };
    arrangement.onShowRecordingGuide = [this] (recordingguide::Topic topic) { showRecordingGuide (topic); };
    arrangement.onShowTriggerGuide = [this] (triggerguide::Topic topic) { showTriggerGuide (topic); };
    arrangement.onShowHarmoniserGuide = [this] { openGuide (gen::guides::Guide::harmoniser, 0); };
    arrangement.onShowRoutingGuide = [this] (routingguide::Topic topic) { showRoutingGuide (topic); };
    arrangement.onShowKeyGuide = [this] (bool yourClip) { showKeyGuide (yourClip ? gen::keyfinding::Topic::yours : gen::keyfinding::Topic::key); };
    arrangement.onShowTransformGuide = [this] (int topic) { openGuide (gen::guides::Guide::midi, topic); };
    pianoRoll.onShowTransformGuide = [this] (int topic) { openGuide (gen::guides::Guide::midi, topic); };
    {
        const auto showQuantising = [this]
        {
            const auto* first = std::begin (gen::rhythm::allTopics);
            openGuide (gen::guides::Guide::rhythm, (int) (std::find (first, std::end (gen::rhythm::allTopics), gen::rhythm::Topic::quantising) - first));
        };
        arrangement.onShowQuantiseGuide = showQuantising;
        pianoRoll.onShowQuantiseGuide = showQuantising;
    }
    pianoRoll.onShowKeyGuide = [this] { showKeyGuide (gen::keyfinding::Topic::yours); };
    pianoRoll.onShowRhythmGuide = [this] { showRhythmGuide(); };
    pianoRoll.onShowReadingGuide = [this] { showReadingGuide(); };
    pianoRoll.onShowStylesGuide = [this] { showStylesGuide (gen::stylesguide::Topic::styles); };
    pianoRoll.onUndoApply = [this] { stepHistory (false); };
    pianoRoll.onExportSheetMusic = [this] { sheetmusic::showDialog (engine, selection.getTrack(), project.getProjectName(), [this] { showReadingGuide(); }); };
    pianoRoll.onClose = [this] { showBottomPanel (BottomPanel::closed); };
    waveEditor.onClose = [this] { showBottomPanel (BottomPanel::closed); };
    waveEditor.onShowSoundGuide = [this] { showSoundGuide(); };
    pianoRoll.onShowMidiGuide = [this] { showMidiGuide(); };
    pianoRoll.onAddInstrument = [this] (juce::Component& button) { showAddInstrumentMenu (button); };
    pianoRoll.updateAddInstrumentButton();
    arrangement.onStartChoice = [this] (StartPanel::Choice choice, juce::Component& tile) { handleStartChoice (choice, tile); };
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
    arrangement.getTrackActions().onShowNotes = [this] (Track& t)
    {
        selection.select (&t);
        showNotebook (NotebookWindow::Tab::track);
    };
    arrangement.onOpenClipEditor = [this] { showBottomPanel (BottomPanel::clipEditor); };
    transport.onToggleNotebook = [this] { commandManager.invokeDirectly (notebookCommand, false); };
    transport.onToggleInspector = [this] { commandManager.invokeDirectly (toggleInspectorCommand, false); };
    transport.onToggleMixer = [this] { commandManager.invokeDirectly (toggleMixerCommand, false); };
    arrangement.onSongRowsFoldedChanged = [this] { commandManager.commandStatusChanged(); };
    arrangement.onLearnChords = [this] { showSongChordsExercise(); };
    arrangement.onWriteLyrics = [this] { showWriteLyricsExercise(); };
    arrangement.onSingKaraoke = [this] { showKaraokeExercise(); };
    arrangement.onFindRangeKey = [this] { showRangeKeyExercise(); };
    arrangement.onRangeMeasured = [this] (const gen::singing::Measurement& m) { rangeKeyMeasured = { m.present, m.getRange(), m.clipName }; };
    arrangement.onKaraokeAdded = [this] (const std::vector<Track*>& added, Track* vocals) { noteKaraokeTracks (added, vocals); };
    arrangement.getTrackActions().onMakeSoundBetter = [this] (Track& t)
    {
        selection.select (&t);
        showSoundBetterExercise();
    };
    arrangement.getTrackActions().onBuildBeat = [this] (Track& t)
    {
        selection.select (&t);
        showBuildBeatExercise();
    };
    arrangement.onChordsDetected = [this]
    {
        songChordsBaseline = engine.getChords();   // the detected chords are the starting point of the swap, not the swap
        songChordsBaselineSet = true;
    };
    // The tempo step ticks while the song clip, its place and the tempo are as the detectors left them.
    arrangement.onTempoSetFromClip = [this] (const AudioClip& clip)
    {
        if (auto record = gen::songchords::makeTempoRecord (engine, clip); ! record.isEmpty())
            songTempoRecord = std::move (record);
    };
    arrangement.onNoClearBeat = [this] (const AudioClip& clip)
    {
        if (auto record = gen::songchords::makeNoBeatRecord (engine, clip); ! record.isEmpty())
            songTempoRecord = std::move (record);
    };
    arrangement.onDetectionLinedUp = [this] (const AudioClip& clip, bool linedUp)
    {
        if (linedUp)
            if (auto record = gen::songchords::makeTempoRecord (engine, clip); ! record.isEmpty())
                songTempoRecord = std::move (record);
    };

    {
        auto* settings = engine.properties.getUserSettings();
        inspectorOpen = settings->getBoolValue ("view.inspectorOpen", true);
        inspectorWidth = juce::jlimit (InspectorPanel::minWidth, InspectorPanel::maxWidth, settings->getIntValue ("view.inspectorWidth", InspectorPanel::defaultWidth));
    }

    inspector.onClose = [this] { setInspectorOpen (false); };
    inspector.onFirstSongExercise = [this] { showFirstSongExercise(); };
    inspector.getFirstSongProgress = [this]
    {
        const auto state = getExerciseState();
        return state.started ? juce::String (state.done) + " of 4 done" : juce::String();
    };
    inspectorEdge.getPanelWidth = [this] { return inspectorWidth; };
    inspectorEdge.onResize = [this] (int w)
    {
        inspectorWidth = juce::jlimit (InspectorPanel::minWidth, InspectorPanel::maxWidth, w);
        resized();
    };
    inspectorEdge.onResizeEnd = [this] { engine.properties.getUserSettings()->setValue ("view.inspectorWidth", inspectorWidth); };
    inspector.setVisible (inspectorOpen);
    transport.setInspectorShown (inspectorOpen);
    inspectorEdge.setVisible (inspectorOpen);

    const auto savedPanel = engine.properties.getUserSettings()->getValue ("ui.bottomPanel", "clip");
    showBottomPanel (savedPanel == "mixer" ? BottomPanel::mixer : savedPanel == "closed" ? BottomPanel::closed : BottomPanel::clipEditor);
    arrangement.onShowRhythmGuide = [this] { showRhythmGuide(); };
    arrangement.onShowTimeSignatureGuide = [this] { showTimeSignatureGuide(); };
    arrangement.getProjectName = [this] { return project.getProjectName(); };

    selection.addChangeListener (this);
    pluginWindows.setKeyListener (&typing);

    typing.onOnChanged = [this] { updateTypingStrip(); };
    transport.onToggleTyping = [this] { commandManager.invokeDirectly (computerKeyboardCommand, false); };
    transport.onShowKeyGuide = [this] { openGuide (gen::guides::Guide::chords, 0); };
    transport.onShowTimeSignatureGuide = [this] { showTimeSignatureGuide(); };
    transport.onShowTypingGuide = [this] { openGuide (gen::guides::Guide::midi, (int) midiguide::Topic::computerKeyboard); };
    updateTypingStrip();

    // Replacing the project deletes every track and clip, so nothing may point at them.
    project.beforeReplace = [this]
    {
        selection.select (nullptr);
        pluginWindows.closeAll();
        loudnessui::clearLast();   // a different project is a different song
    };
    project.afterReplace = [this]
    {
        // The engine's change message comes later, and the old headers' timers and paints would read deleted tracks
        // before then, so the rows are rebuilt now.
        arrangement.rebuildTracks();
        engine.dispatchPendingMessages();   // the inspector, mixer, device bar and clip editors drop the old tracks now

        const auto& tracks = engine.getTracks();

        if (! tracks.empty())
            selection.select (tracks.front().get(), tracks.front()->clips.empty() ? nullptr : tracks.front()->clips.front().get());

        selection.dispatchPendingMessages();

        // The exercise's chord tick compares with the song as it was when it began, which is now a different song.
        if (exerciseBaselineSet)
            exerciseChordBaseline = engine.getChords();

        resetSongChordsMemory();
        resetWriteLyricsMemory();
        resetKaraokeMemory (false);
        resetRangeKeyMemory (false);
        resetSoundBetterMemory();
        resetMixExportMemory (false);
        exerciseSteps.reset();   // a new, opened or restored project starts every exercise at step 1

        for (auto& window : exerciseWindows)
            if (window != nullptr)
                window->returnToFirstStep();
        songTempoRecord = {};   // it describes the song, so only another song clears it (Start over keeps it)
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
    appleMenuExtras.addCommandItem (&commandManager, aboutCommand);
    juce::MenuBarModel::setMacMainMenu (this, &appleMenuExtras);
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

    transport.onUndo = [this] { stepHistory (false); };
    transport.onRedo = [this] { stepHistory (true); };
    startTimerHz (5);

    setWantsKeyboardFocus (true);
    setSize (1440, 900);
}

MainComponent::~MainComponent()
{
    guideprint::setApp ({});
    guideprint::exportButtonFactory() = {};
    fxui::setHostTrackLookup ({});
    sampleractions::setGuideOpener ({});
    samplertext::setTrackLookup ({});
    metertext::setSongAccess ({});

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #endif

    stopTimer();
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
    pointer.setBounds (getLocalBounds());

    if (typingStrip != nullptr)
        typingStrip->setBounds (r.removeFromBottom (ComputerKeyboardStrip::height));

    // The inspector takes the full height on the left; the tracks and panels get what is left. In a narrow window the
    // inspector shrinks first, then the Notebook, so the tracks keep a useful width (preferred widths stay as chosen).
    const auto budget = panelbudget::compute (getWidth(), inspectorOpen, inspectorWidth, notebookWindow != nullptr);

    if (inspectorOpen)
    {
        inspector.setBounds (r.removeFromLeft (budget.inspectorWidth));
        inspectorEdge.setBounds (r.removeFromLeft (budget.edgeWidth));
    }

    if (notebookWindow != nullptr)
        notebookWindow->setBounds (r.removeFromRight (budget.notebookWidth));

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
    transport.setInspectorShown (open);
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
    if (key == juce::KeyPress::escapeKey && pointer.isActive())
    {
        pointer.dismiss();
        return true;
    }

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
    selection.selectAndReveal (track, clip);
}

void MainComponent::handleStartChoice (StartPanel::Choice choice, juce::Component& tile)
{
    switch (choice)
    {
        case StartPanel::Choice::style:        commandManager.invokeDirectly (newFromStyleCommand, false); break;
        case StartPanel::Choice::instrument:   showAddInstrumentMenu (tile); break;
        case StartPanel::Choice::record:       addRecordingTrack(); break;
        case StartPanel::Choice::importAudio:  commandManager.invokeDirectly (importAudioCommand, false); break;

        case StartPanel::Choice::learn:        showStartHere(); break;
        case StartPanel::Choice::learnSong:    showExercise (gen::exercises::Exercise::songChords); break;
    }
}

void MainComponent::showAddInstrumentMenu (juce::Component& target)
{
    constexpr int firstSoundId = 1, midiGuideId = 100;
    juce::PopupMenu menu;
    menu.addSectionHeader ("A MIDI track: notes played by this sound");
    int id = firstSoundId;

    for (auto sound : builtInSounds::all)
        menu.addItem (id++, builtInSounds::getName (sound));

    menu.addSeparator();
    guideicon::addItem (menu, midiGuideId, "What is MIDI?");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                        [safe = juce::Component::SafePointer<MainComponent> (this)] (int result)
                        {
                            if (safe == nullptr || result <= 0)
                                return;

                            if (result == midiGuideId)
                                safe->openGuide (gen::guides::Guide::midi, (int) midiguide::Topic::whatItIs);
                            else if (result - firstSoundId < (int) builtInSounds::all.size())
                                safe->addInstrumentTrack (builtInSounds::all[(size_t) (result - firstSoundId)]);
                        });
}

void MainComponent::addInstrumentTrack (BuiltInSound sound, bool typingIsOn)
{
    engine.nameNextStep ("Add instrument");
    auto* track = engine.addTrack (Track::Type::midi);
    const auto name = builtInSounds::getName (sound);
    track->name = name;
    engine.setBuiltInSound (*track, sound);
    auto* clip = engine.addMidiClip (*track, 0.0, 4.0 * engine.getBarLength());
    selection.selectAndReveal (track, clip);
    showBottomPanel (BottomPanel::clipEditor);

    const auto caption = builtInSounds::isPercussive (sound)
        ? "Your " + name + " track has an empty clip, open here. On a drum track each note is a drum: C2 (note 36) is the kick, "
                           "D2 (38) the snare. Click in the grid to add one."
        : typingIsOn ? "Your " + name + " track has an empty clip, open here. The computer keyboard is on: press A to hear middle C. "
                                                                    "Click in the grid to write a note (Cmd+K turns the keyboard off)."
                     : "Your " + name + " track has an empty clip, open here. Click in the grid to write a note: you hear it as you add it. "
                       "Or press Cmd+K to play it with your letter keys: A is middle C.";

    pointer.show ([safe = juce::Component::SafePointer<PianoRoll> (&pianoRoll)] () -> juce::Component*
                  {
                      return safe != nullptr && safe->isShowing() ? &safe->getGridArea() : nullptr;
                  },
                  { caption, {}, {} });
}

void MainComponent::addRecordingTrack (bool pretendNoInput)
{
    engine.nameNextStep ("Add recording track");
    auto* track = engine.addTrack (Track::Type::audio);
    track->name = "Recording";
    selection.selectAndReveal (track, nullptr);

    GuidePointer::Options options;

    if (engine.getNumInputChannels() > 0 && ! pretendNoInput)
        options.caption = "Your Recording track is ready. Press R to arm it: the chip after R shows which input it records, such as In 1. "
                          "Then press the red Record button, and Stop when you're done.";
    else
    {
        options.caption = "No audio input was found on this audio device. Connect a microphone or an audio interface, then choose it in Audio/MIDI Settings.";
        options.buttonText = "Audio/MIDI Settings...";
        options.onButton = [safe = juce::Component::SafePointer<MainComponent> (this)] { if (safe != nullptr) safe->showAudioSettings(); };
    }

    // The track's header is built a moment after the edit changes, so the pointer asks for the R button until it is there.
    pointer.show ([this, track] () -> juce::Component*
                  {
                      return engine.indexOf (track) >= 0 ? arrangement.getArmButton (*track) : nullptr;
                  },
                  std::move (options));
}

void MainComponent::showStartHere()
{
    commandManager.invokeDirectly (theoryGuidesCommand, false);

    if (theoryGuides != nullptr)
        theoryGuides->showRow (0);   // Start here
}

void MainComponent::resetSongChordsMemory()
{
    songChordsBaseline = engine.getChords();
    playedAlong.reset();
}

void MainComponent::resetWriteLyricsMemory()
{
    writeLyricsSectionBar = -1;
    writeLyricsSung.reset();
}

void MainComponent::resetSoundBetterMemory()
{
    soundBetterRemembered = nullptr;
    soundBetterListen.reset();
    soundBetterReduction.reset();
    soundBetterCompare.reset();
}

void MainComponent::resetMixExportMemory (bool takeBaseline)
{
    mixBaseline = takeBaseline ? gen::mixexport::takeBaseline (engine) : gen::mixexport::Baseline();
    mixMeasured = {};
    mixExported = {};
}

void MainComponent::recordMeasurement (const metertext::SongResult& result)
{
    const auto range = loudnessui::getLastRange().value_or (loudnessui::MeasuredRange());
    mixMeasured = {};
    mixMeasured.valid = true;
    mixMeasured.wholeSong = range.wholeSong;
    mixMeasured.startBeat = range.startBeat;
    mixMeasured.endBeat = range.endBeat;
    mixMeasured.readings = result.readings;
}

void MainComponent::feedSoundBetter()
{
    namespace sb = gen::soundbetter;

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double seconds = soundBetterLastFeedMs > 0.0 ? (now - soundBetterLastFeedMs) / 1000.0 : 0.2;
    soundBetterLastFeedMs = now;

    const auto* chosen = sb::findChosenTrack (engine, selection.getTrack(), soundBetterRemembered);

    if (chosen != soundBetterRemembered)
    {
        // Another track: its records start afresh.
        soundBetterListen.reset();
        soundBetterReduction.reset();
        soundBetterCompare.reset();
        soundBetterRemembered = chosen;
    }

    if (chosen == nullptr)
        return;

    const bool playing = engine.isPlaying();
    soundBetterListen.feed (playing, sb::isHeardAt (engine, *chosen, engine.getHeardPositionBeats()), seconds);

    auto* compressor = sb::findEffect (*chosen, sb::Effect::compressor);
    float recentDb = 0.0f;

    if (compressor != nullptr)
        if (auto* plugin = dynamic_cast<BuiltInCompressor*> (compressor->instance.get()))
            recentDb = plugin->getRecentReductionDb();

    soundBetterReduction.feed (playing, compressor, recentDb);

    std::vector<sb::CompareTracker::Seen> seen;

    for (auto* slot : sb::findExerciseEffects (*chosen))
        seen.push_back ({ slot, ! slot->bypassed.load() });

    soundBetterCompare.feed (playing, seen, seconds);
}

void MainComponent::setSoundBetterRecordsForShot (double listenedSeconds, double reductionDb, bool compared)
{
    namespace sb = gen::soundbetter;
    const auto* chosen = sb::findChosenTrack (engine, selection.getTrack(), soundBetterRemembered);

    if (chosen != soundBetterRemembered)
    {
        soundBetterListen.reset();
        soundBetterReduction.reset();
        soundBetterCompare.reset();
        soundBetterRemembered = chosen;
    }

    if (chosen == nullptr)
        return;

    for (double heard = 0.0; heard < listenedSeconds; heard += sb::longestFeedSeconds)
        soundBetterListen.feed (true, true, sb::longestFeedSeconds);

    if (reductionDb > 0.0)
        soundBetterReduction.feed (true, sb::findEffect (*chosen, sb::Effect::compressor), (float) reductionDb);

    if (compared)
    {
        const auto effects = sb::findExerciseEffects (*chosen);

        for (int round = 0; round < 6; ++round)
            for (const bool on : { true, false })
            {
                std::vector<sb::CompareTracker::Seen> seen;

                for (auto* slot : effects)
                    seen.push_back ({ slot, on });

                soundBetterCompare.feed (true, seen, sb::longestFeedSeconds);
            }
    }
}

void MainComponent::setWriteLyricsSungForShot()
{
    namespace wl = gen::writelyrics;
    wl::Progress progress;
    progress.rememberedBar = writeLyricsSectionBar;

    if (const auto section = wl::findChosenSection (engine, progress))
        if (const int index = wl::findLine (engine.getLyrics(), *section, engine.getBarLength()); index >= 0)
        {
            const auto& line = engine.getLyrics().lines[(size_t) index];
            const wl::LineSpan span { line.startBeat, line.endBeat, wl::getLineText (line) };
            const double secondsPerBeat = 60.0 / engine.getTempo();

            // Two samples in one pass, one in each half of the line.
            writeLyricsSung.feed (line.startBeat + 0.1, true, true, &span, secondsPerBeat);
            writeLyricsSung.feed (line.endBeat - 0.1, true, true, &span, secondsPerBeat);
        }
}

void MainComponent::feedWriteLyrics()
{
    namespace wl = gen::writelyrics;
    wl::Progress progress;
    progress.rememberedBar = writeLyricsSectionBar;
    const auto section = wl::findChosenSection (engine, progress);

    if (section.has_value() && wl::getLoop (engine).on)
        writeLyricsSectionBar = section->startBar;

    std::optional<wl::LineSpan> span;

    if (section.has_value())
        if (const int index = wl::findLine (engine.getLyrics(), *section, engine.getBarLength()); index >= 0)
        {
            const auto& line = engine.getLyrics().lines[(size_t) index];
            span = wl::LineSpan { line.startBeat, line.endBeat, wl::getLineText (line) };
        }

    writeLyricsSung.feed (engine.getHeardPositionBeats(), engine.isPlaying(), arrangement.isSingAlongShowing(),
                          span.has_value() ? &*span : nullptr, 60.0 / engine.getTempo());
}

void MainComponent::setExercisePlayedAlongForShot (int bars)
{
    playedAlong.reset();

    for (int bar = 0; bar < bars; ++bar)
        playedAlong.feed (bar, true, true, true);
}

void MainComponent::feedPlayedAlong()
{
    if (! engine.isPlaying() || ! arrangement.isPlayAlongShowing())
        return;

    const int bar = (int) std::floor (engine.getHeardPositionBeats() / engine.getBarLength());
    const auto& chords = engine.getChords();
    bool inSong = true;

    if (const auto* clip = gen::songchords::findSongClip (engine))
        inSong = gen::songchords::getSpan (engine, *clip).contains (bar);

    playedAlong.feed (bar, inSong && chords.count (bar) > 0, true, true);
}

void MainComponent::showExercise (gen::exercises::Exercise which)
{
    if (which == gen::exercises::Exercise::mixExport && ! mixBaseline.valid)
        resetMixExportMemory (true);

    if (which == gen::exercises::Exercise::songChords && ! songChordsBaselineSet)
    {
        resetSongChordsMemory();
        songChordsBaselineSet = true;
    }

    if (! exerciseBaselineSet)
    {
        exerciseChordBaseline = engine.getChords();
        exerciseBaselineSet = true;
    }

    auto& slot = exerciseWindows[(size_t) which];
    const bool fresh = slot == nullptr;

    pluginWindows.show (&slot, gen::exercises::getInfo (which).title, [this, which, &slot]
    {
        const auto remembered = exerciseSteps.get (which);
        auto* view = new ExerciseWindow (engine, selection, which, [this, which] { return getExerciseContext (which); }, remembered.step, remembered.finished);
        view->onProgressChanged = [this, which] (int step, bool finished) { exerciseSteps.set (which, step, finished); };
        view->onAction = [this, which] (int step, gen::exercises::Fix fix) { runExerciseAction (which, step, fix); };
        view->onShowMe = [this, which] (int step) { showExerciseTarget (which, step); };
        view->onOpenLink = [this] (const gen::exercises::Link& link) { openExerciseLink (link); };
        view->onCarryOn = [this, which]
        {
            const auto info = gen::exercises::getInfo (which);

            if (info.carryOnOpensStartHere)
                showStartHere();
            else
                openExerciseLink (info.carryOnLink);
        };
        view->onStartOver = [this, which]
        {
            if (which == gen::exercises::Exercise::songChords)
                resetSongChordsMemory();
            else if (which == gen::exercises::Exercise::writeLyrics)
                resetWriteLyricsMemory();
            else if (which == gen::exercises::Exercise::karaoke)
                resetKaraokeMemory (true);
            else if (which == gen::exercises::Exercise::rangeKey)
                resetRangeKeyMemory (true);
            else if (which == gen::exercises::Exercise::soundBetter)
                resetSoundBetterMemory();
            else if (which == gen::exercises::Exercise::mixExport)
                resetMixExportMemory (true);
            else
                exerciseChordBaseline = engine.getChords();
        };
        view->onClose = [this, which]
        {
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<MainComponent> (this), which]
                                             { if (safe != nullptr) safe->pluginWindows.closeFor (&safe->exerciseWindows[(size_t) which]); });
        };
        slot = view;
        return view;
    });

    if (slot != nullptr)
        if (auto* window = slot->getTopLevelComponent(); window != nullptr && window != getTopLevelComponent())
        {
            // The user works in the main window while following the steps, so the exercise stays in front of it.
            window->setAlwaysOnTop (true);

            if (fresh)
            {
                const auto main = getTopLevelComponent()->getBounds();
                window->setTopLeftPosition (main.getRight() - window->getWidth() - 24, main.getY() + 60);
            }
        }
}

gen::exercises::Context MainComponent::getExerciseContext (gen::exercises::Exercise which) const
{
    gen::exercises::Context context;
    context.saved = { project.hasFile(), project.isEdited() };
    const bool chordsExercise = which == gen::exercises::Exercise::songChords;
    context.baseline = chordsExercise ? songChordsBaseline : exerciseChordBaseline;
    context.playedAlongBars = playedAlong.getCount();
    context.tempo = songTempoRecord;
    context.writeLyricsSectionBar = writeLyricsSectionBar;
    context.sung = writeLyricsSung.getRecord();
    context.singAlongOpen = arrangement.isSingAlongShowing();
    context.soundBetterSelected = selection.getTrack();
    context.soundBetterRemembered = soundBetterRemembered;
    context.mixBaseline = mixBaseline;
    context.mixSelected = selection.getTrack();
    context.mixMeasured = mixMeasured;
    context.mixExported = mixExported;
    context.mixerShowing = bottomPanel == BottomPanel::mixer;
    context.playing = engine.isPlaying();
    context.heardBeat = engine.getHeardPositionBeats();
    context.listened = soundBetterListen.getRecord();
    context.reduction = soundBetterReduction.getRecord();
    context.compared = soundBetterCompare.getRecord();
    context.beatSelectedTrack = selection.getTrack();
    context.beatSelectedClip = selection.getMidiClip();
    context.beatQuantiseGrid = quantiseui::getOpenGridBeats();
    context.hasOutputDevice = ! exerciseNoOutput && engine.deviceManager.getCurrentAudioDevice() != nullptr;
    context.karaokeRecord = karaokeRecord;
    context.karaokeTake = karaokeTake;
    context.karaokeSungBars = karaokeSung.getCount();
    context.karaokeListenedBars = karaokeListen.getCount();
    {
        const auto rk = getRangeKeyProgress();
        context.rangeKeyTake = rk.take;
        context.rangeKeyRange = rk.range;
        context.rangeKeyChosenMelody = rk.chosenMelody;
        context.rangeKeySelected = rk.selectedTrack;
        context.rangeKeyMeasured = rk.measured;
        context.rangeKeyMove = rk.lastMove;
        context.rangeKeyHeardBars = rk.heardBars;
    }
    context.hasInputDevice = exerciseNoInput.has_value() ? ! *exerciseNoInput : engine.getNumInputChannels() > 0;
    context.recording = engine.isRecording();
    context.demucsInstalled = exerciseDemucsMissing.has_value() ? ! *exerciseDemucsMissing : stems::isModelInstalled();
    context.whisperInstalled = exerciseWhisperMissing.has_value() ? ! *exerciseWhisperMissing
                                                                  : models::isInstalled (analysis::lyricsfinder::getModel (analysis::lyricsfinder::ModelSize::small));
    return context;
}

TheoryGuidesView::ExerciseState MainComponent::getExerciseState()
{
    TheoryGuidesView::ExerciseState state;
    auto& settings = *engine.properties.getUserSettings();
    const bool finishedBefore = gen::firstsong::readDone (juce::var (settings.getBoolValue (gen::firstsong::doneSettingKey, false)));
    const gen::firstsong::SavedState saved { project.hasFile(), project.isEdited() };

    for (int i = 0; i < gen::firstsong::numSteps; ++i)
        if (gen::firstsong::isStepDone (i, engine, exerciseChordBaseline, saved))
            ++state.done;

    state.finished = finishedBefore;
    state.started = finishedBefore || exerciseBaselineSet || exerciseSteps.get (gen::exercises::Exercise::firstSong).step > 0;

    if (state.finished)
        state.done = gen::firstsong::numSteps;

    return state;
}

void MainComponent::openExerciseLink (const gen::exercises::Link& link)
{
    const auto& entry = gen::guides::getEntry (link.guide);

    // A guide built on a plugin (EQ, Compressor, Reverb) has its pages in the plugin, not in the entry.
    const int topic = entry.plugin.isNotEmpty() ? BuiltInGuides::getPageNames (entry.plugin).indexOf (link.topic)
                                                : entry.topics.indexOf (link.topic);
    openGuide (link.guide, topic);
}

void MainComponent::runSongChordsAction (int step, gen::exercises::Fix fix)
{
    using gen::exercises::Fix;
    namespace sc = gen::songchords;

    // Each detector runs on the song clip found fresh: a clip the user deleted since the window was drawn is never read.
    const auto detectWith = [this] (void (ArrangementView::*detector) (Track&, AudioClip&))
    {
        const auto* clip = sc::findSongClip (engine);
        auto* track = clip != nullptr ? engine.findTrackFor (clip) : nullptr;

        if (track != nullptr)
            (arrangement.*detector) (*track, *const_cast<AudioClip*> (clip));
        else
            commandManager.invokeDirectly (importAudioCommand, false);
    };

    if (fix == Fix::importAudio) { commandManager.invokeDirectly (importAudioCommand, false); return; }
    if (fix == Fix::detectTempo) { detectWith (&ArrangementView::detectTempo); return; }
    if (fix == Fix::detectChords) { detectWith (&ArrangementView::detectChords); return; }
    if (fix == Fix::detectSections) { detectWith (&ArrangementView::detectStructure); return; }
    if (fix == Fix::audioSettings) { showAudioSettings(); return; }

    switch (step)
    {
        case sc::stepImport: commandManager.invokeDirectly (importAudioCommand, false); break;
        case sc::stepTempo: detectWith (&ArrangementView::detectTempo); break;
        case sc::stepChords: detectWith (&ArrangementView::detectChords); break;
        case sc::stepSections: detectWith (&ArrangementView::detectStructure); break;
        case sc::stepPlayAlong: arrangement.showPlayAlong(); break;

        case sc::stepSwap:
        {
            arrangement.showChordsRow();
            const auto swap = sc::getSwap (engine, songChordsBaseline);

            if (swap.valid)
                pointer.show ([this] () -> juce::Component* { return &arrangement.getChordLane(); },
                              [this, bar = swap.bar] { return arrangement.getChordLane().getBarArea (bar); },
                              { "Bar " + juce::String (swap.bar + 1) + ": click it and choose " + swap.to.getName() + " in place of "
                                    + swap.from.getName() + ".", {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getChordLane(); },
                              { "The Chords row: click a bar and choose another chord.", {}, {} });
            break;
        }

        default: break;
    }
}

void MainComponent::runExerciseAction (gen::exercises::Exercise which, int step, gen::exercises::Fix fix)
{
    if (which == gen::exercises::Exercise::mapStructure)
    {
        if (step == 3 && fix == gen::exercises::Fix::none)
            showExerciseTarget (which, step);
        else
            runSongChordsAction (step == 2 ? gen::songchords::stepSections : step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::songChords)
    {
        runSongChordsAction (step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::writeLyrics)
    {
        runWriteLyricsAction (step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::karaoke)
    {
        runKaraokeAction (step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::rangeKey)
    {
        runRangeKeyAction (step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::soundBetter)
    {
        runSoundBetterAction (step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::mixExport)
    {
        runMixExportAction (step, fix);
        return;
    }

    if (which == gen::exercises::Exercise::buildBeat)
    {
        runBuildBeatAction (step, fix);
        return;
    }

    const auto pointAt = [this] (std::function<juce::Component*()> target, const juce::String& caption)
    {
        pointer.show (std::move (target), { caption, {}, {} });
    };

    switch (step)
    {
        case 0:
        {
            if (const auto* found = gen::firstsong::findMidiTrack (engine))
            {
                auto* track = const_cast<Track*> (found);
                MidiClip* clip = nullptr;

                for (auto& c : track->clips)
                    if (clip == nullptr)
                        clip = dynamic_cast<MidiClip*> (c.get());

                if (clip == nullptr)
                    clip = engine.addMidiClip (*track, 0.0, 4.0 * engine.getBarLength());

                selection.selectAndReveal (track, clip);
                showBottomPanel (BottomPanel::clipEditor);

                if (! typing.isOn())
                    commandManager.invokeDirectly (computerKeyboardCommand, false);

                pointAt ([safe = juce::Component::SafePointer<PianoRoll> (&pianoRoll)] () -> juce::Component*
                         { return safe != nullptr && safe->isShowing() ? &safe->getGridArea() : nullptr; },
                         "Your " + track->name + " clip is open here. The computer keyboard is on: press A to hear middle C. "
                         "Click in the grid to write a note (Cmd+K turns the keyboard off).");
            }
            else
            {
                if (! typing.isOn())
                    commandManager.invokeDirectly (computerKeyboardCommand, false);

                addInstrumentTrack (BuiltInSound::piano, true);
            }
            break;
        }

        case 1:
        {
            if (gen::firstsong::canWriteChords (engine) && gen::firstsong::writeExampleChords (engine))
                exerciseChordBaseline = engine.getChords();   // the example itself isn't the user's change

            arrangement.showChordsRow();
            pointAt ([this] () -> juce::Component* { return &arrangement.getChordLane(); },
                     "Click a bar on the Chords row and choose another chord. Open the row with its arrow to see each chord as piano keys, "
                     "and click a chord to hear it.");
            break;
        }

        case 2:
        {
            bool hasDrums = false;

            for (auto& t : engine.getTracks())
                hasDrums = hasDrums || gen::firstsong::isDrumTrack (*t);

            if (! hasDrums)
            {
                addInstrumentTrack (BuiltInSound::drums);
            }
            else if (auto* clip = selection.getMidiClip(); clip != nullptr && gen::firstsong::canWriteBeat (engine, *clip))
            {
                gen::firstsong::writeBasicBeat (engine, *clip);
                pointAt ([safe = juce::Component::SafePointer<PianoRoll> (&pianoRoll)] () -> juce::Component*
                         { return safe != nullptr && safe->isShowing() ? &safe->getGridArea() : nullptr; },
                         "A basic beat: kick on beats 1 and 3, snare on 2 and 4, a hi-hat on every eighth. Press Space to hear it, and change any note.");
            }
            break;
        }

        case 3:
            commandManager.invokeDirectly (saveProjectCommand, false);
            break;

        default:
            break;
    }
}

void MainComponent::showSongChordsTarget (int step)
{
    namespace sc = gen::songchords;

    const auto clipArea = [this]
    {
        const auto* clip = sc::findSongClip (engine);
        return clip != nullptr ? arrangement.getLanes().getClipArea (*clip) : juce::Rectangle<int>();
    };

    switch (step)
    {
        case sc::stepImport:
            if (arrangement.getStartPanel().isShowing())
                pointer.show ([this] () -> juce::Component* { return &arrangement.getStartPanel().getTile (StartPanel::Choice::importAudio); },
                              { "Drop a song file here, or choose File > Import Audio... (Cmd+I).", {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); },
                              { "Drop a song file here, or choose File > Import Audio... (Cmd+I).", {}, {} });
            break;

        case sc::stepTempo:
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); }, clipArea,
                          { "Your recording. Right-click it and choose Detect tempo... to find its beat.", {}, {} });
            break;

        case sc::stepChords:
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); }, clipArea,
                          { "Your recording. Right-click it and choose Detect chords... to find its chords.", {}, {} });
            break;

        case sc::stepSections:
        {
            // The span's sections when there are some, else the recording itself, where the menu item is.
            const auto* clip = sc::findSongClip (engine);
            const auto span = clip != nullptr ? sc::getSpan (engine, *clip) : sc::Span();

            if (clip != nullptr && sc::countSectionBars (engine.getSections(), span) > 0)
            {
                arrangement.setSongRowsFolded (false);   // the Sections row is among the song rows
                pointer.show ([this] () -> juce::Component* { return &arrangement.getSectionLane(); },
                              [this, span] { return arrangement.getSectionLane().getBarsArea (span.firstBar, span.endBar); },
                              { "Your song's sections. Click one to loop it or change its type from its menu.", {}, {} });
            }
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); }, clipArea,
                              { "Your recording. Right-click it and choose Detect song structure... to find its parts.", {}, {} });
            break;
        }

        case sc::stepPlayAlong:
            pointer.show ([this] () -> juce::Component* { return &arrangement.getPlayAlongButton(); },
                          { "Play along: the chord playing now, large, with the next one beside it.", {}, {} });
            break;

        case sc::stepSwap:
        {
            arrangement.showChordsRow();
            const auto swap = sc::getSwap (engine, songChordsBaseline);

            if (swap.valid)
                pointer.show ([this] () -> juce::Component* { return &arrangement.getChordLane(); },
                              [this, bar = swap.bar] { return arrangement.getChordLane().getBarArea (bar); },
                              { "Bar " + juce::String (swap.bar + 1) + ": click it and choose " + swap.to.getName() + ".", {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getChordLane(); },
                              { "The Chords row: click a bar and choose another chord.", {}, {} });
            break;
        }

        default: break;
    }
}

//==============================================================================
gen::karaoke::Progress MainComponent::getKaraokeProgress() const
{
    gen::karaoke::Progress p;
    p.karaoke = karaokeRecord;
    p.take = karaokeTake;
    p.sungBars = karaokeSung.getCount();
    p.listenedBars = karaokeListen.getCount();
    p.hasOutputDevice = ! exerciseNoOutput && engine.deviceManager.getCurrentAudioDevice() != nullptr;
    p.hasInput = exerciseNoInput.has_value() ? ! *exerciseNoInput : engine.getNumInputChannels() > 0;
    p.recording = engine.isRecording();
    return p;
}

void MainComponent::noteKaraokeTracks (const std::vector<Track*>& added, Track* vocals)
{
    // Split into stems has no separate Vocals track: its Vocals stem is among the tracks.
    if (vocals == nullptr)
        for (auto* t : added)
            if (t != nullptr && t->name.trim().equalsIgnoreCase ("Vocals"))
                vocals = t;

    karaokeRecord = gen::karaoke::makeKaraokeRecord (engine, added, vocals);
}

void MainComponent::resetKaraokeMemory (bool countersOnly)
{
    karaokeSung.reset();
    karaokeListen.reset();

    if (countersOnly)
        return;

    karaokeRecord = {};
    karaokeTake = {};
    karaokeReportVersion = engine.getRecordingReportVersion();   // an earlier project's recording is not this song's take
}

void MainComponent::setKaraokeForShot (Track* karaokeTrack, Track* vocals, const AudioClip* take, int sungBars, int listenedBars)
{
    karaokeRecord = gen::karaoke::makeKaraokeRecord (engine, karaokeTrack != nullptr ? std::vector<Track*> { karaokeTrack } : std::vector<Track*> {}, vocals);
    karaokeTake = {};

    if (take != nullptr)
        karaokeTake.clips = { take };

    setKaraokeCountsForShot (sungBars, listenedBars);
}

void MainComponent::setKaraokeCountsForShot (int sungBars, int listenedBars)
{
    karaokeSung.reset();
    karaokeListen.reset();

    for (int bar = 0; bar < sungBars; ++bar)
        karaokeSung.feed (bar, true, true, true);

    for (int bar = 0; bar < listenedBars; ++bar)
        karaokeListen.feed (bar, true, true, false, true);
}

void MainComponent::feedKaraoke()
{
    namespace ka = gen::karaoke;

    // A new recording report replaces the take record; one that made no clip and wasn't silent (a MIDI recording) leaves it.
    if (const int version = engine.getRecordingReportVersion(); version != karaokeReportVersion)
    {
        karaokeReportVersion = version;

        if (const auto record = ka::makeTakeRecord (engine); ! record.isEmpty())
        {
            karaokeTake = record;
            karaokeListen.reset();
        }
    }

    if (! engine.isPlaying())
        return;

    const auto progress = getKaraokeProgress();
    const double beat = engine.getHeardPositionBeats();
    const int bar = ka::getBarOf (engine, beat);
    karaokeSung.feed (bar, ka::isSungBar (engine, progress, bar), true, arrangement.isSingAlongShowing());

    if (const auto* take = ka::findTake (engine, progress, true))
        karaokeListen.feed (bar, beat >= take->startBeat && beat < take->getEndBeat (engine.getTempoMap()), true, engine.isRecording(), ka::isClipHeard (engine, *take));
}

void MainComponent::runKaraokeAction (int step, gen::exercises::Fix fix)
{
    using gen::exercises::Fix;
    namespace ka = gen::karaoke;

    const auto progress = getKaraokeProgress();

    // The clip is found fresh: the user may have deleted it since the window was drawn.
    const auto withClip = [this] (const AudioClip* clip, void (ArrangementView::*run) (Track&, AudioClip&))
    {
        auto* track = clip != nullptr ? engine.findTrackFor (clip) : nullptr;

        if (track != nullptr)
            (arrangement.*run) (*track, *const_cast<AudioClip*> (clip));
        else
            commandManager.invokeDirectly (importAudioCommand, false);
    };

    const auto detectLyrics = [&] { withClip (ka::findDetectClip (engine, progress), &ArrangementView::detectLyrics); };

    if (fix == Fix::importAudio) { commandManager.invokeDirectly (importAudioCommand, false); return; }
    if (fix == Fix::audioSettings) { showAudioSettings(); return; }
    if (fix == Fix::detectLyrics) { detectLyrics(); return; }
    if (fix == Fix::addRecordingTrack) { addRecordingTrack (exerciseNoInput.value_or (false)); return; }

    switch (step)
    {
        case ka::stepImport: commandManager.invokeDirectly (importAudioCommand, false); break;

        case ka::stepVocals:
            if (const auto* clip = ka::findRemoveClip (engine, progress))
                arrangement.removeVocals (*clip);
            else
                commandManager.invokeDirectly (importAudioCommand, false);
            break;

        case ka::stepWords: detectLyrics(); break;
        case ka::stepSingAlong: arrangement.showSingAlong(); break;

        case ka::stepRecord:
        {
            const auto target = ka::findRecordTarget (engine);

            if (target.kind == ka::RecordTarget::Kind::none) { addRecordingTrack (exerciseNoInput.value_or (false)); break; }
            if (target.kind == ka::RecordTarget::Kind::unarmed) { showKaraokeTarget (step); break; }

            // One bar before the first words, on the user's click; the count-in setting applies as usual.
            if (! engine.isRecording())
            {
                engine.setPositionBeats (ka::getRecordStartBar (engine, progress) * engine.getBarLength());
                TransportShortcuts::toggleRecording (engine);
            }
            break;
        }

        case ka::stepListen:
        {
            if (engine.isRecording())
                break;

            if (const auto* take = ka::findTake (engine, progress, true))
            {
                engine.setPositionBeats (take->startBeat);
                engine.play();
            }
            break;
        }

        default: break;
    }
}

void MainComponent::showKaraokeTarget (int step)
{
    namespace ka = gen::karaoke;
    const auto progress = getKaraokeProgress();

    arrangement.setSongRowsFolded (false);   // the Lyrics row may be folded

    // Each outline asks for its clip again when drawn, so a deleted clip is never read.
    const auto outline = [this] (std::function<const AudioClip*()> find, const juce::String& caption)
    {
        pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); },
                      [this, find]
                      {
                          const auto* clip = find();
                          return clip != nullptr ? arrangement.getLanes().getClipArea (*clip) : juce::Rectangle<int>();
                      },
                      { caption, {}, {} });
    };

    switch (step)
    {
        case ka::stepImport:
            if (arrangement.getStartPanel().isShowing())
                pointer.show ([this] () -> juce::Component* { return &arrangement.getStartPanel().getTile (StartPanel::Choice::importAudio); },
                              { "Drop a song file here, or choose File > Import Audio... (Cmd+I).", {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); },
                              { "Drop a song file here, or choose File > Import Audio... (Cmd+I).", {}, {} });
            break;

        case ka::stepVocals:
            outline ([this] { return ka::findRemoveClip (engine, getKaraokeProgress()); },
                     "Your song. Right-click it and choose Remove vocals (karaoke version)... to make the karaoke version.");
            break;

        case ka::stepWords:
            outline ([this] { return ka::findDetectClip (engine, getKaraokeProgress()); },
                     "The Vocals clip. Right-click it and choose Detect lyrics (English only)... to find the words.");
            break;

        case ka::stepSingAlong:
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLyricsSingAlongButton(); },
                          { "Sing along: the words in large letters, coloured as they're sung.", {}, {} });
            break;

        case ka::stepRecord:
        {
            const auto target = ka::findRecordTarget (engine);

            if (target.track != nullptr)
            {
                selection.selectAndReveal (const_cast<Track*> (target.track), nullptr);
                pointer.show ([this, track = target.track] () -> juce::Component* { return engine.indexOf (track) >= 0 ? arrangement.getArmButton (*track) : nullptr; },
                              { target.kind == ka::RecordTarget::Kind::armed ? "Armed: press the red Record button, and Stop when you're done."
                                                                              : "Press R on " + target.track->name + " to arm it. The chip after R shows which input it records.", {}, {} });
            }
            else
                pointer.show ([this] () -> juce::Component* { return &transport.getRecordButton(); },
                              { "Add an audio track and arm it with R, then press Record here.", {}, {} });
            break;
        }

        case ka::stepListen:
            outline ([this] { return ka::findTake (engine, getKaraokeProgress(), true); }, "Your take. Play it with the karaoke version and listen.");
            break;

        default: break;
    }
}

//==============================================================================
gen::singing::Range MainComponent::getSingerRange() const
{
    auto* settings = engine.properties.getUserSettings();

    if (settings == nullptr || ! settings->containsKey (SingingExplainer::lowestKey) || ! settings->containsKey (SingingExplainer::highestKey))
        return {};

    gen::singing::Range r { settings->getIntValue (SingingExplainer::lowestKey, -1), settings->getIntValue (SingingExplainer::highestKey, -1) };
    r.lowest = juce::jlimit (gen::singing::lowestChoice, gen::singing::highestChoice, r.lowest);
    r.highest = juce::jlimit (gen::singing::lowestChoice, gen::singing::highestChoice, r.highest);
    return r.isSet() ? r : gen::singing::Range();
}

gen::rangekey::Progress MainComponent::getRangeKeyProgress() const
{
    gen::rangekey::Progress p;
    p.take = rangeKeyTake;
    p.range = getSingerRange();
    p.chosenMelody = arrangement.getChosenMelodyTrack();
    p.selectedTrack = selection.getTrack();
    p.measured = rangeKeyMeasured;
    p.lastMove = rangeKeyMove;
    p.heardBars = rangeKeyHeard.getCount();
    p.hasOutputDevice = ! exerciseNoOutput && engine.deviceManager.getCurrentAudioDevice() != nullptr;
    p.hasInput = exerciseNoInput.has_value() ? ! *exerciseNoInput : engine.getNumInputChannels() > 0;
    p.recording = engine.isRecording();
    return p;
}

void MainComponent::resetRangeKeyMemory (bool countersOnly)
{
    rangeKeyHeard.reset();
    rangeKeyHeardKey = engine.getKey();

    if (countersOnly)
        return;

    rangeKeyTake = {};
    rangeKeyMeasured = {};
    rangeKeyMove = {};
    rangeKeyReportVersion = engine.getRecordingReportVersion();   // an earlier project's recording is not this song's take
}

void MainComponent::setRangeKeyForShot (const AudioClip* take, bool measured, int heardBars)
{
    rangeKeyTake = {};
    rangeKeyMeasured = {};

    if (take != nullptr)
    {
        rangeKeyTake.clips = { take };

        if (measured)
        {
            const auto m = RangeMeasurer::measureNow (*take);
            rangeKeyMeasured = { m.present, m.getRange(), take->name };
        }
    }

    rangeKeyHeard.reset();
    rangeKeyHeardKey = engine.getKey();

    for (int bar = 0; bar < heardBars; ++bar)
        rangeKeyHeard.feed (bar, true, true, false, true);
}

void MainComponent::feedRangeKey()
{
    namespace rk = gen::rangekey;

    // A new recording report replaces the take record; one that made no clip and wasn't silent leaves it.
    if (const int version = engine.getRecordingReportVersion(); version != rangeKeyReportVersion)
    {
        rangeKeyReportVersion = version;

        if (const auto record = gen::karaoke::makeTakeRecord (engine); ! record.isEmpty())
            rangeKeyTake = record;
    }

    // After an undo that takes the exercise's move back, the move is forgotten.
    if (rangeKeyMove.present && (engine.getNumUndoSteps() < rangeKeyMove.undoSteps || rangeKeyMove.keyAfter != engine.getKey()))
        rangeKeyMove = {};

    // The song moved (or was moved back): the bars heard count again from nothing.
    if (engine.getKey() != rangeKeyHeardKey)
    {
        rangeKeyHeardKey = engine.getKey();
        rangeKeyHeard.reset();
    }

    const auto progress = getRangeKeyProgress();

    if (! rk::isStepKeyDone (engine, progress))
    {
        rangeKeyHeard.reset();
        return;
    }

    if (! engine.isPlaying())
        return;

    const auto* melody = rk::findMelodyTrack (engine, progress);
    const double beat = engine.getHeardPositionBeats();
    const int bar = gen::karaoke::getBarOf (engine, beat);
    const bool holds = rk::getMelodyBars (engine, melody).count (bar) > 0;
    const bool heard = melody != nullptr && ! melody->muted && routing::getSilence (*melody, engine.getTracks()) == routing::Silence::none;
    rangeKeyHeard.feed (bar, holds, true, engine.isRecording(), heard);
}

void MainComponent::runRangeKeyAction (int step, gen::exercises::Fix fix)
{
    using gen::exercises::Fix;
    namespace rk = gen::rangekey;

    if (fix == Fix::audioSettings) { showAudioSettings(); return; }
    if (fix == Fix::addRecordingTrack) { addRecordingTrack (exerciseNoInput.value_or (false)); return; }

    const auto progress = getRangeKeyProgress();
    const auto action = rk::getAction (step, engine, progress);

    if (! action.enabled)
        return;

    // The clip is found fresh: the user may have deleted it since the window was drawn.
    const auto withClip = [this] (const AudioClip* clip, void (ArrangementView::*run) (Track&, AudioClip&))
    {
        if (auto* track = clip != nullptr ? engine.findTrackFor (clip) : nullptr)
            (arrangement.*run) (*track, *const_cast<AudioClip*> (clip));
    };

    switch (action.op)
    {
        case rk::Op::record:
            if (! engine.isRecording())
                TransportShortcuts::toggleRecording (engine);
            break;

        case rk::Op::addRecordingTrack: addRecordingTrack (exerciseNoInput.value_or (false)); break;
        case rk::Op::audioSettings: showAudioSettings(); break;
        case rk::Op::measureTake: withClip (rk::findTake (engine, progress), &ArrangementView::measureRange); break;
        case rk::Op::setByHand: arrangement.showSingingTopic (gen::singing::Topic::range); break;
        case rk::Op::convertToMidi: withClip (rk::findConvertClip (engine, progress), &ArrangementView::convertToMidi); break;

        case rk::Op::moveSong:
            if (const auto result = gen::songmove::moveSong (engine, action.shift); result.moved)
            {
                rangeKeyMove = { true, action.shift, engine.getKey(), result.undoSteps };
                rangeKeyHeard.reset();
                rangeKeyHeardKey = engine.getKey();
                commandManager.commandStatusChanged();
            }
            break;

        case rk::Op::playFromBar:
            if (! engine.isRecording())
            {
                engine.setPositionBeats (action.bar * engine.getBarLength());
                engine.play();
            }
            break;

        case rk::Op::none: break;
    }
}

void MainComponent::showRangeKeyTarget (int step)
{
    namespace rk = gen::rangekey;
    const auto progress = getRangeKeyProgress();
    arrangement.setSongRowsFolded (false);

    switch (step)
    {
        case rk::stepRecord:
        {
            const auto target = gen::karaoke::findRecordTarget (engine);

            if (target.track != nullptr)
            {
                selection.selectAndReveal (const_cast<Track*> (target.track), nullptr);
                pointer.show ([this, track = target.track] () -> juce::Component* { return engine.indexOf (track) >= 0 ? arrangement.getArmButton (*track) : nullptr; },
                              { target.kind == gen::karaoke::RecordTarget::Kind::armed ? "Armed: press the red Record button, slide low and hold, then high and hold, and press Stop."
                                                                                        : "Press R on " + target.track->name + " to arm it.", {}, {} });
            }
            else
                pointer.show ([this] () -> juce::Component* { return &transport.getRecordButton(); },
                              { "Add an audio track and arm it with R, then press Record here.", {}, {} });
            break;
        }

        case rk::stepKeep:
        {
            // Outline the take while it exists; the guide shows the rest.
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); },
                          [this]
                          {
                              const auto* clip = rk::findTake (engine, getRangeKeyProgress());
                              return clip != nullptr ? arrangement.getLanes().getClipArea (*clip) : juce::Rectangle<int>();
                          },
                          { "Your take. Right-click it and choose Measure singing range..., or use the button in the exercise.", {}, {} });
            break;
        }

        case rk::stepMelody:
        {
            if (const auto* track = rk::findMelodyTrack (engine, progress); track != nullptr && rk::countNotes (*track) > 0)
                pointer.show ([this, track] () -> juce::Component* { return engine.indexOf (track) >= 0 ? arrangement.getTrackHeader (*track) : nullptr; },
                              { "The melody track: its notes are what your range is checked against.", {}, {} });
            else if (const auto* clip = rk::findConvertClip (engine, progress))
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); },
                              [this, clip]
                              {
                                  return gen::karaoke::isInEdit (engine, clip) ? arrangement.getLanes().getClipArea (*clip) : juce::Rectangle<int>();
                              },
                              { "Right-click this recording and choose Convert to MIDI... to turn the tune into notes.", {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); },
                              { "Write the tune on a MIDI track, or record yourself singing it.", {}, {} });
            break;
        }

        case rk::stepKey:
            pointer.show ([this] () -> juce::Component* { return &transport.getKeyBox(); },
                          { "The song's key. After Move the song it shows the new key.", {}, {} });
            break;

        case rk::stepHear:
            pointer.show ([this] () -> juce::Component* { return &transport.getPlayButton(); },
                          { "Press Play and sing the melody in the new key.", {}, {} });
            break;

        default: break;
    }
}

void MainComponent::runWriteLyricsAction (int step, gen::exercises::Fix fix)
{
    using gen::exercises::Fix;
    namespace wl = gen::writelyrics;

    if (fix == Fix::audioSettings) { showAudioSettings(); return; }
    if (fix == Fix::openSections) { showWriteLyricsTarget (wl::stepSection); return; }
    if (fix == Fix::openLyrics) { arrangement.showLyricsEditor(); return; }

    switch (step)
    {
        case wl::stepSection: showWriteLyricsTarget (wl::stepSection); break;
        case wl::stepWrite: arrangement.showLyricsEditor(); break;

        case wl::stepTap:
        {
            // The line is found fresh: the lyrics may have changed since the window was drawn.
            wl::Progress progress;
            progress.rememberedBar = writeLyricsSectionBar;
            const auto section = wl::findChosenSection (engine, progress);
            const int line = section.has_value() ? wl::findLine (engine.getLyrics(), *section, engine.getBarLength()) : -1;

            if (line < 0)
            {
                arrangement.showLyricsEditor();
                break;
            }

            // Tapping needs the keyboard focus, which the exercise window has: bring the main window forward first.
            if (auto* top = getTopLevelComponent())
                top->toFront (true);

            arrangement.showSectionsRow();
            arrangement.getLyricsLane().startTappingLine (line);
            break;
        }

        case wl::stepSing: arrangement.showSingAlong(); break;
        default: break;
    }
}

void MainComponent::runSoundBetterAction (int step, gen::exercises::Fix fix)
{
    using gen::exercises::Fix;
    namespace sb = gen::soundbetter;

    if (fix == Fix::importAudio) { commandManager.invokeDirectly (importAudioCommand, false); return; }
    if (fix == Fix::audioSettings) { showAudioSettings(); return; }

    // The chosen track is found fresh: the song may have changed since the window was drawn.
    auto* track = const_cast<Track*> (sb::findChosenTrack (engine, selection.getTrack(), soundBetterRemembered));

    switch (step)
    {
        case sb::stepListen:
        case sb::stepCompare:
            if (! engine.isPlaying())
                engine.play();
            break;

        case sb::stepEq:
        case sb::stepCompressor:
        {
            if (track == nullptr)
            {
                showSoundBetterTarget (sb::stepListen);
                break;
            }

            const auto effect = step == sb::stepEq ? sb::Effect::eq : sb::Effect::compressor;

            if (auto* slot = sb::findEffect (*track, effect))
                plugins::open (pluginWindows, *track, *slot);
            else
                plugins::add (engine, pluginWindows, *track, step == sb::stepEq ? BuiltInEq::getDescription() : BuiltInCompressor::getDescription());
            break;
        }

        case sb::stepReverb:
            if (track == nullptr)
                showSoundBetterTarget (sb::stepListen);
            else if (auto* button = arrangement.getSendsButton (*track))
                arrangement.getTrackActions().showSendsButtonMenu (*track, button);
            break;

        default: break;
    }
}

void MainComponent::runMixExportAction (int step, gen::exercises::Fix fix)
{
    using gen::exercises::Fix;
    namespace mx = gen::mixexport;

    if (fix == Fix::importAudio) { commandManager.invokeDirectly (importAudioCommand, false); return; }
    if (fix == Fix::audioSettings) { showAudioSettings(); return; }

    switch (step)
    {
        case mx::stepBalance:
        case mx::stepPan:
            showBottomPanel (BottomPanel::mixer);
            showMixExportTarget (step);
            break;

        case mx::stepLoudness: measureLoudness (true); break;
        case mx::stepExport: bounceui::showDialog (engine, project.getProjectName(), {}, false, true); break;
        default: break;
    }
}

/** The beat clip as the exercise's steps see it, from the selection and the song. */
static gen::buildbeat::Progress getBeatProgress (const gen::exercises::Context& context)
{
    return { context.beatSelectedTrack, context.beatSelectedClip, context.hasOutputDevice, context.beatQuantiseGrid };
}

bool MainComponent::openBeatClip (bool addClipIfMissing)
{
    namespace bb = gen::buildbeat;
    const auto beat = bb::findBeat (engine, getBeatProgress (getExerciseContext (gen::exercises::Exercise::buildBeat)));

    if (beat.track == nullptr)
        return false;

    auto* track = const_cast<Track*> (beat.track);
    auto* clip = const_cast<MidiClip*> (beat.clip);

    if (clip == nullptr && addClipIfMissing)
    {
        engine.nameNextStep ("Add drum clip");
        clip = engine.addMidiClip (*track, 0.0, 4.0 * engine.getBarLength());
    }

    selection.selectAndReveal (track, clip);
    showBottomPanel (BottomPanel::clipEditor);
    return clip != nullptr;
}

void MainComponent::runBuildBeatAction (int step, gen::exercises::Fix fix)
{
    namespace bb = gen::buildbeat;

    if (fix == gen::exercises::Fix::audioSettings)
    {
        showAudioSettings();
        return;
    }

    if (! bb::hasDrumTrack (engine))
    {
        addInstrumentTrack (BuiltInSound::drums);
        return;
    }

    if (! openBeatClip (true))
        return;

    const auto progress = getBeatProgress (getExerciseContext (gen::exercises::Exercise::buildBeat));
    const auto beat = bb::findBeat (engine, progress);

    if (step == bb::stepSwing && beat.clip != nullptr && ! beat.clip->notes.empty())
    {
        // The whole clip, with the starting settings; nothing moves until the user presses Quantise.
        const auto start = bb::getQuantiseStart (engine, progress);
        const auto showQuantising = [this]
        {
            const auto* first = std::begin (gen::rhythm::allTopics);
            openGuide (gen::guides::Guide::rhythm, (int) (std::find (first, std::end (gen::rhythm::allTopics), gen::rhythm::Topic::quantising) - first));
        };
        quantiseui::showDialog (engine, *const_cast<MidiClip*> (beat.clip), {}, showQuantising, &start);
        return;
    }

    showBuildBeatTarget (step);
}

void MainComponent::showBuildBeatTarget (int step)
{
    namespace bb = gen::buildbeat;
    openBeatClip (false);

    const auto text = bb::getShowMeText (step, false);
    const auto safe = juce::Component::SafePointer<PianoRoll> (&pianoRoll);

    pointer.show ([safe, step] () -> juce::Component*
                  {
                      if (safe == nullptr || ! safe->isShowing())
                          return nullptr;

                      switch (step)
                      {
                          case bb::stepHats:   return &safe->getSnapBox();
                          case bb::stepGroove: return &safe->getVelocityButton();
                          case bb::stepSwing:  return &safe->getQuantiseButton();
                          default:             break;
                      }

                      return &safe->getGridArea();
                  }, { text, {}, {} });
}

void MainComponent::showMixExportTarget (int step)
{
    namespace mx = gen::mixexport;
    showBottomPanel (BottomPanel::mixer);

    const auto* shown = mx::findShownTrack (engine, selection.getTrack());
    const auto* candidate = step == mx::stepPan ? mx::findPanCandidate (engine) : nullptr;
    const auto* track = candidate != nullptr ? candidate : shown;
    const auto text = mx::getShowMeText (step, track != nullptr ? track->name : juce::String());

    if (step == mx::stepLoudness)
    {
        pointer.show ([this] () -> juce::Component* { return &mixer.getMasterStrip(); },
                      [this] { return mixer.getMasterStrip().getMeterArea(); }, { text, {}, {} });
        return;
    }

    if (step == mx::stepExport || track == nullptr)
    {
        pointer.show ([this] () -> juce::Component* { return &mixer.getMasterStrip(); }, { text, {}, {} });
        return;
    }

    mixer.showStripOf (*track);
    const bool pan = step == mx::stepPan;
    pointer.show ([this, track, pan] () -> juce::Component*
                  {
                      if (engine.indexOf (track) < 0)
                          return nullptr;

                      return pan ? mixer.getPanKnobOf (*track) : mixer.getFaderOf (*track);
                  }, { text, {}, {} });
}

void MainComponent::setTrackMixForShot (const juce::String& trackName, std::optional<float> volumeDb, std::optional<float> pan)
{
    for (auto& t : engine.getTracks())
        if (t->name == trackName)
        {
            if (volumeDb.has_value())
                t->volumeDb.store (*volumeDb);

            if (pan.has_value())
                t->pan.store (*pan);

            engine.sendChangeMessage();
        }
}

void MainComponent::setMixExportMeasuredForShot()
{
    const auto result = loudnessui::measureSongNow (engine);

    if (result.has_value())
        recordMeasurement (*result);
}

void MainComponent::setMixExportExportedForShot (const gen::mixexport::ExportRecord& record)
{
    mixExported = record;
}

void MainComponent::showSoundBetterTarget (int step)
{
    namespace sb = gen::soundbetter;
    sb::Progress progress;
    progress.selected = selection.getTrack();
    progress.remembered = soundBetterRemembered;
    progress.compared = soundBetterCompare.getRecord();
    auto* track = const_cast<Track*> (sb::findChosenTrack (engine, progress));
    const auto text = sb::getShowMeText (step, engine, progress);

    if (step == sb::stepListen)
    {
        // The chosen track's header, else the first track with a clip.
        if (track == nullptr)
            for (auto& t : engine.getTracks())
                if (track == nullptr && (t->isAudio() || t->isMidi()) && ! t->clips.empty())
                    track = t.get();

        if (track != nullptr)
            pointer.show ([this, track] () -> juce::Component* { return engine.indexOf (track) >= 0 ? arrangement.getTrackHeader (*track) : nullptr; }, { text, {}, {} });
        else
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLanes(); }, { text, {}, {} });

        return;
    }

    if (track == nullptr)
        return;

    // The device bar shows the selected track's chain.
    if (selection.getTrack() != track)
        selection.selectAndReveal (track);

    switch (step)
    {
        case sb::stepEq:
        case sb::stepCompressor:
        {
            const auto* slot = sb::findEffect (*track, step == sb::stepEq ? sb::Effect::eq : sb::Effect::compressor);

            if (slot != nullptr)
                pointer.show ([this, slot] () -> juce::Component* { return deviceBar.getEffectChip (slot); }, { text, {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &deviceBar.getAddEffectButton(); }, { text, {}, {} });
            break;
        }

        case sb::stepReverb:
            pointer.show ([this, track] () -> juce::Component* { return engine.indexOf (track) >= 0 ? arrangement.getSendsButton (*track) : nullptr; },
                          { text, {}, {} });
            break;

        case sb::stepCompare:
        {
            const auto effects = sb::findExerciseEffects (*track);

            if (effects.empty())
            {
                pointer.show ([this] () -> juce::Component* { return &deviceBar.getAddEffectButton(); }, { text, {}, {} });
                break;
            }

            // The first effect on the chosen track, else a reverb on a return (select that return to see its chip).
            const PluginSlot* target = nullptr;

            for (auto* slot : effects)
                if (target == nullptr && plugins::contains (*track, slot))
                    target = slot;

            if (target == nullptr)
            {
                target = effects.front();

                for (auto& t : engine.getTracks())
                    if (t->isReturn() && plugins::contains (*t, target))
                        selection.selectAndReveal (t.get());
            }

            pointer.show ([this, target] () -> juce::Component* { return deviceBar.getEffectChip (target); },
                          [this, target] { return deviceBar.getEffectPowerArea (target); },
                          { text, {}, {} });
            break;
        }

        default: break;
    }
}

void MainComponent::showWriteLyricsTarget (int step)
{
    namespace wl = gen::writelyrics;
    wl::Progress progress;
    progress.rememberedBar = writeLyricsSectionBar;
    const auto section = wl::findChosenSection (engine, progress);
    const auto sungSection = section.has_value() ? section : wl::findFirstSungSection (engine.getSections());
    const auto name = sungSection.has_value() ? sungSection->getDisplayName() : juce::String();
    const auto text = wl::getShowMeText (step, name);

    arrangement.showSectionsRow();   // the song rows may be folded

    switch (step)
    {
        case wl::stepSection:
        {
            if (sungSection.has_value())
            {
                pointer.show ([this] () -> juce::Component* { return &arrangement.getSectionLane(); },
                              [this, first = sungSection->startBar, end = sungSection->getEndBar()] { return arrangement.getSectionLane().getBarsArea (first, end); },
                              { text, {}, {} });
                break;
            }

            // No sung section: the first free bars of the row.
            int bar = 0;

            for (bool moved = true; moved;)
            {
                moved = false;

                for (auto& s : engine.getSections())
                    if (bar >= s.startBar && bar < s.getEndBar())
                    {
                        bar = s.getEndBar();
                        moved = true;
                    }
            }

            pointer.show ([this] () -> juce::Component* { return &arrangement.getSectionLane(); },
                          [this, bar] { return arrangement.getSectionLane().getBarsArea (bar, bar + 4); },
                          { text, {}, {} });
            break;
        }

        case wl::stepWrite:
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLyricsEditButton(); }, { text, {}, {} });
            break;

        case wl::stepTap:
        {
            const int line = section.has_value() ? wl::findLine (engine.getLyrics(), *section, engine.getBarLength()) : -1;

            if (line >= 0)
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLyricsLane(); },
                              [this, line]
                              {
                                  const auto& lines = engine.getLyrics().lines;
                                  return (size_t) line < lines.size() ? arrangement.getLyricsLane().getBeatRangeArea (lines[(size_t) line].startBeat, lines[(size_t) line].endBeat)
                                                                      : juce::Rectangle<int>();
                              },
                              { text, {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &arrangement.getLyricsLane(); }, { text, {}, {} });
            break;
        }

        case wl::stepSing:
            pointer.show ([this] () -> juce::Component* { return &arrangement.getLyricsSingAlongButton(); }, { text, {}, {} });
            break;

        default: break;
    }
}

void MainComponent::showExerciseTarget (gen::exercises::Exercise which, int step)
{
    if (which == gen::exercises::Exercise::mapStructure)
    {
        if (step == 3)
        {
            arrangement.setSongRowsFolded (false);
            pointer.show ([this] () -> juce::Component* { return &arrangement.getSectionLane(); },
                          { "Click a chorus and choose Loop this section. If none is marked, click a section and choose Chorus.", {}, {} });
        }
        else
            showSongChordsTarget (step == 2 ? gen::songchords::stepSections : step);
        return;
    }

    if (which == gen::exercises::Exercise::songChords)
    {
        showSongChordsTarget (step);
        return;
    }

    if (which == gen::exercises::Exercise::writeLyrics)
    {
        showWriteLyricsTarget (step);
        return;
    }

    if (which == gen::exercises::Exercise::karaoke)
    {
        showKaraokeTarget (step);
        return;
    }

    if (which == gen::exercises::Exercise::rangeKey)
    {
        showRangeKeyTarget (step);
        return;
    }

    if (which == gen::exercises::Exercise::soundBetter)
    {
        showSoundBetterTarget (step);
        return;
    }

    if (which == gen::exercises::Exercise::mixExport)
    {
        showMixExportTarget (step);
        return;
    }

    if (which == gen::exercises::Exercise::buildBeat)
    {
        showBuildBeatTarget (step);
        return;
    }

    const auto gridTarget = [safe = juce::Component::SafePointer<PianoRoll> (&pianoRoll)] () -> juce::Component*
    {
        return safe != nullptr && safe->isShowing() ? &safe->getGridArea() : nullptr;
    };

    switch (step)
    {
        case 0:
            if (pianoRoll.isShowing())
                pointer.show (gridTarget, { "Click in the grid to write a note: you hear it as you add it.", {}, {} });
            else
                pointer.show ([this] () -> juce::Component* { return &transport.getTypingButton(); },
                              { "Turn on the computer keyboard here (Cmd+K), then press A to hear middle C.", {}, {} });
            break;

        case 1:
            arrangement.showChordsRow();
            pointer.show ([this] () -> juce::Component* { return &arrangement.getChordLane(); },
                          { "The Chords row: one chord per bar. Click a bar to choose another chord.", {}, {} });
            break;

        case 2:
        {
            const Track* drums = nullptr;

            for (auto& t : engine.getTracks())
                if (drums == nullptr && gen::firstsong::isDrumTrack (*t))
                    drums = t.get();

            if (drums != nullptr)
                pointer.show ([this, drums] () -> juce::Component*
                              { return engine.indexOf (drums) >= 0 ? arrangement.getTrackHeader (*drums) : nullptr; },
                              { "Your drum track. Each note in its clip is a drum: kick, snare or hi-hat.", {}, {} });
            else
                pointer.show (gridTarget, { "Click in the grid to write drum notes once you have a drum track.", {}, {} });
            break;
        }

        default:
            break;
    }
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
    engine.clearHistory();   // the example song is the start, not a step
}

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    commands.addArray ({ newProjectCommand, newFromStyleCommand, openProjectCommand, saveProjectCommand, saveProjectAsCommand, bounceCommand, measureLoudnessCommand, exportChordSheetCommand, exportSheetMusicCommand, exportGuidesCommand,
                         importAudioCommand, showPluginsCommand, audioSettingsCommand,
                         showClipEditorCommand, showScoreCommand, toggleMixerCommand, signalFlowCommand, notebookCommand, foldSongRowsCommand, followPlayheadCommand, showSelectedTrackCommand, playAlongCommand, singAlongCommand, tunerCommand, aboutCommand,
                         theoryGuidesCommand, firstSongExerciseCommand, songChordsExerciseCommand, writeLyricsExerciseCommand, soundBetterExerciseCommand, mixExportExerciseCommand, buildBeatExerciseCommand, mapStructureExerciseCommand, karaokeExerciseCommand, rangeKeyExerciseCommand, newAudioTrackCommand, newMidiTrackCommand, newBusCommand, createBusCommand, bounceInPlaceCommand,
                         newReverbReturnCommand, newDelayReturnCommand, newReturnCommand, computerKeyboardCommand, toggleInspectorCommand,
                         undoCommand, redoCommand });
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
        case measureLoudnessCommand:
            info.setInfo ("Measure Loudness...", "Measure how loud the whole song (or the loop) is, in LUFS and true peak, without playing it", "File", 0);
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
        case notebookCommand:
            info.setInfo ("Notebook", "Write notes for yourself, about this song or about one track", "View", 0);
            info.setTicked (notebookWindow != nullptr);
            info.addDefaultKeypress ('9', cmd);
            break;
        case foldSongRowsCommand:
            info.setInfo ("Fold Sections, Chords and Lyrics", "Show the Sections, Chords and Lyrics rows as a thin coloured line each, "
                          "to give the tracks the room", "View", 0);
            info.setTicked (arrangement.areSongRowsFolded());
            info.addDefaultKeypress ('3', cmd);
            break;
        case followPlayheadCommand:
            info.setInfo ("Follow Playhead", "Keep the playhead in view while the song plays", "View", 0);
            info.setTicked (arrangement.isFollowPlayheadEnabled());
            info.addDefaultKeypress ('f', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case showSelectedTrackCommand:
            info.setInfo ("Show Selected Track", "Scroll the tracks to the selected track, unfolding its bus if needed", "View", 0);
            info.setActive (selection.getTrack() != nullptr);
            info.addDefaultKeypress ('j', juce::ModifierKeys::commandModifier);
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
        case aboutCommand:
            info.setInfo ("About DawIt...", "The version, the licence, where to get the source, and every third-party notice", "Help", 0);
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
        case firstSongExerciseCommand:
            info.setInfo ("First Song Exercise...", "Four short steps in your own song: hear a note, change a chord, add a rhythm and save", "Help", 0);
            break;
        case songChordsExerciseCommand:
            info.setInfo ("Learn the Chords of a Song...", gen::songchords::getHelpDescription(), "Help", 0);
            break;
        case writeLyricsExerciseCommand:
            info.setInfo ("Write Lyrics to Your Song...", gen::writelyrics::getHelpDescription(), "Help", 0);
            break;
        case soundBetterExerciseCommand:
            info.setInfo ("Make a Track Sound Better...", gen::soundbetter::getHelpDescription(), "Help", 0);
            break;
        case mixExportExerciseCommand:
            info.setInfo ("Mix and Export Your Song...", gen::mixexport::getHelpDescription(), "Help", 0);
            break;
        case buildBeatExerciseCommand:
            info.setInfo ("Build a Beat...", gen::buildbeat::getHelpDescription(), "Help", 0);
            break;
        case karaokeExerciseCommand:
            info.setInfo ("Sing Karaoke to a Song...", gen::karaoke::getHelpDescription(), "Help", 0);
            break;
        case rangeKeyExerciseCommand:
            info.setInfo ("Find Your Range and Key...", gen::rangekey::getHelpDescription(), "Help", 0);
            break;
        case mapStructureExerciseCommand:
            info.setInfo ("Map a Song's Structure...", gen::exercises::getInfo (gen::exercises::Exercise::mapStructure).helpDescription, "Help", 0);
            break;
        case theoryGuidesCommand:
            info.setInfo ("Theory Guides...", "Every guide in DawIt in one place: sound, MIDI, rhythm, chords, songwriting, mixing, "
                                              "effects and synthesis, with a path for beginners and a search", "View", 0);
            info.addDefaultKeypress ('g', cmd);
            break;
        case undoCommand:
            info.setInfo (engine.canUndo() ? "Undo " + engine.getUndoName() : juce::String ("Undo"),
                          "Take back the latest step: notes, clips, tracks, effects in a chain, sends, volume, pan and the song rows. Mute, solo and the settings inside an effect aren't part of undo",
                          "Edit", 0);
            info.addDefaultKeypress ('z', cmd);
            info.setActive (engine.canUndo());
            break;
        case redoCommand:
            info.setInfo (engine.canRedo() ? "Redo " + engine.getRedoName() : juce::String ("Redo"),
                          "Put back the step you took back", "Edit", 0);
            info.addDefaultKeypress ('z', cmd | juce::ModifierKeys::shiftModifier);
           #if ! JUCE_MAC
            info.addDefaultKeypress ('y', cmd);
           #endif
            info.setActive (engine.canRedo());
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
        case measureLoudnessCommand: measureLoudness(); return true;
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
        case notebookCommand:      if (notebookWindow != nullptr) closeNotebook(); else showNotebook(); return true;
        case foldSongRowsCommand:  arrangement.setSongRowsFolded (! arrangement.areSongRowsFolded()); return true;
        case showSelectedTrackCommand:
            if (auto* t = selection.getTrack())
                arrangement.revealTrack (*t);
            return true;
        case followPlayheadCommand:
            arrangement.setFollowPlayhead (! arrangement.isFollowPlayheadEnabled());
            commandManager.commandStatusChanged();
            return true;
        case playAlongCommand:     arrangement.showPlayAlong(); return true;
        case singAlongCommand:     arrangement.showSingAlong(); return true;
        case tunerCommand:         showTuner(); return true;
        case aboutCommand:         showAbout(); return true;
        case computerKeyboardCommand: typing.setOn (! typing.isOn()); return true;
        case theoryGuidesCommand:  showTheoryGuides(); return true;
        case firstSongExerciseCommand: showFirstSongExercise(); return true;
        case songChordsExerciseCommand: showSongChordsExercise(); return true;
        case writeLyricsExerciseCommand: showWriteLyricsExercise(); return true;
        case soundBetterExerciseCommand: showSoundBetterExercise(); return true;
        case mixExportExerciseCommand: showMixExportExercise(); return true;
        case buildBeatExerciseCommand: showBuildBeatExercise(); return true;
        case mapStructureExerciseCommand: showExercise (gen::exercises::Exercise::mapStructure); return true;
        case karaokeExerciseCommand: showKaraokeExercise(); return true;
        case rangeKeyExerciseCommand: showRangeKeyExercise(); return true;
        case newAudioTrackCommand: addTrack (Track::Type::audio); return true;
        case newMidiTrackCommand:  addTrack (Track::Type::midi); return true;
        case newBusCommand:        arrangement.getTrackActions().addEmptyBus(); return true;
        case createBusCommand:     arrangement.createBusFromSelection(); return true;
        case bounceInPlaceCommand:
            if (auto* track = selection.getTrack())
                bounceui::bounceInPlace (engine, selection, *track, selection.getClip());
            return true;
        case undoCommand:          stepHistory (false); return true;
        case redoCommand:          stepHistory (true); return true;
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
        juce::PopupMenu edit;
        edit.addCommandItem (&commandManager, undoCommand);
        edit.addCommandItem (&commandManager, redoCommand);
        return edit;
    }

    if (index == 2)
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

    if (index == 3)
    {
        juce::PopupMenu view;
        view.addCommandItem (&commandManager, showClipEditorCommand);
        view.addCommandItem (&commandManager, showScoreCommand);
        view.addCommandItem (&commandManager, toggleMixerCommand);
        view.addCommandItem (&commandManager, toggleInspectorCommand);
        view.addCommandItem (&commandManager, signalFlowCommand);
        view.addCommandItem (&commandManager, notebookCommand);
        view.addSeparator();
        view.addCommandItem (&commandManager, foldSongRowsCommand);
        view.addCommandItem (&commandManager, followPlayheadCommand);
        view.addCommandItem (&commandManager, showSelectedTrackCommand);
        view.addSeparator();
        view.addCommandItem (&commandManager, playAlongCommand);
        view.addCommandItem (&commandManager, singAlongCommand);
        view.addCommandItem (&commandManager, tunerCommand);
        view.addCommandItem (&commandManager, computerKeyboardCommand);
        view.addSeparator();
        view.addCommandItem (&commandManager, theoryGuidesCommand);
        return view;
    }

    if (index == 4)
    {
        juce::PopupMenu help;
        help.addCommandItem (&commandManager, theoryGuidesCommand);
        help.addCommandItem (&commandManager, firstSongExerciseCommand);
        help.addCommandItem (&commandManager, songChordsExerciseCommand);
        help.addCommandItem (&commandManager, writeLyricsExerciseCommand);
        help.addCommandItem (&commandManager, soundBetterExerciseCommand);
        help.addCommandItem (&commandManager, mixExportExerciseCommand);
        help.addCommandItem (&commandManager, buildBeatExerciseCommand);
        help.addCommandItem (&commandManager, mapStructureExerciseCommand);
        help.addCommandItem (&commandManager, rangeKeyExerciseCommand);
        help.addCommandItem (&commandManager, karaokeExerciseCommand);
       #if ! JUCE_MAC
        help.addSeparator();
        help.addCommandItem (&commandManager, aboutCommand);   // on macOS it sits in the application menu
       #endif
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
    menu.addCommandItem (&commandManager, measureLoudnessCommand);
    menu.addCommandItem (&commandManager, exportChordSheetCommand);
    menu.addCommandItem (&commandManager, exportSheetMusicCommand);
    menu.addCommandItem (&commandManager, exportGuidesCommand);
    menu.addSeparator();
    menu.addCommandItem (&commandManager, showPluginsCommand);
    menu.addCommandItem (&commandManager, audioSettingsCommand);
    return menu;
}

bool MainComponent::isHistoryBusy()
{
    return juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown()
        || juce::Component::getCurrentlyModalComponent() != nullptr;
}

void MainComponent::timerCallback()
{
    if (! isHistoryBusy() && engine.commitHistory())
        commandManager.commandStatusChanged();

    feedPlayedAlong();
    feedWriteLyrics();
    feedKaraoke();
    feedRangeKey();
    feedSoundBetter();
}

void MainComponent::stepHistory (bool redo)
{
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
    {
        // A text box keeps its own undo; a plugin or guide window leaves Cmd+Z to its own content.
        if (auto* text = dynamic_cast<juce::TextEditor*> (focused))
        {
            const auto mods = juce::ModifierKeys::commandModifier | (redo ? juce::ModifierKeys::shiftModifier : 0);
            text->keyPressed (juce::KeyPress ('z', mods, 0));
            return;
        }

        if (focused->getTopLevelComponent() != getTopLevelComponent())
            return;
    }

    if (isHistoryBusy())
        return;

    Engine::HistoryChange change;

    if (! (redo ? engine.redo (&change) : engine.undo (&change)))
        return;

    for (auto* track : change.tracksGone)
    {
        selection.forget (track);
        pluginWindows.closeFor (static_cast<const void*> (track));

        if (engine.getLiveMidiTrack() == track)
            engine.setLiveMidiTrack (nullptr);
    }

    for (auto* clip : change.clipsGone)
        selection.forget (nullptr, clip);

    for (auto* processor : change.processorsGone)
        pluginWindows.closeFor (*processor);

    pianoRoll.clearNoteSelection();
    commandManager.commandStatusChanged();
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
        auto* guide = new RhythmExplainer (engine, [this] { return RhythmExplainer::ClipView { pianoRoll.getTrack(), pianoRoll.getClip() }; });
        rhythmGuide = guide;
        return guide;
    });
}

void MainComponent::showTimeSignatureGuide()
{
    showRhythmGuide();

    if (rhythmGuide != nullptr)
        rhythmGuide->showTopic (gen::rhythm::Topic::timeSignatures);
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

void MainComponent::showKeyGuide (gen::keyfinding::Topic topic)
{
    pluginWindows.show (&keyGuide, "Finding the key and chords", [this]
    {
        auto* guide = new KeyFindingExplainer (engine, selection);
        keyGuide = guide;
        return guide;
    });

    if (keyGuide != nullptr)
        keyGuide->showTopic (topic);
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

void MainComponent::showAbout()
{
    pluginWindows.show (&aboutWindow, "About DawIt", [this]
    {
        auto* about = new AboutComponent();
        about->onShowLicences = [this] { showLicences(); };
        aboutWindow = about;
        return about;
    });
}

void MainComponent::showLicences (const juce::String& fileName)
{
    pluginWindows.show (&licencesWindow, "Licences", [this]
    {
        auto* licences = new LicencesComponent();
        licencesWindow = licences;
        return licences;
    }, { 720, 480 });

    if (fileName.isNotEmpty())
        if (auto* licences = licencesWindow.getComponent())
            licences->selectNotice (fileName);
}

void MainComponent::showHandoverGuide (handoverguide::Topic topic)
{
    pluginWindows.show (&handoverGuide, "Sending stems to an engineer", [this]
    {
        auto* guide = new HandoverExplainer (engine);
        guide->onShowBounce = [this] { bounceui::showDialog (engine, project.getProjectName(), selection.getTracks()); };
        handoverGuide = guide;
        return guide;
    });

    if (handoverGuide != nullptr)
        handoverGuide->showTopic (topic);
}

void MainComponent::showInstrumentsGuide (gen::instruments::Topic topic)
{
    pluginWindows.show (&instrumentsGuide, "Instruments in MIDI", [this]
    {
        auto* guide = new InstrumentsExplainer (engine, selection);
        instrumentsGuide = guide;
        return guide;
    });

    if (instrumentsGuide != nullptr)
        instrumentsGuide->showTopic (topic);
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

void MainComponent::showNotebook (std::optional<NotebookWindow::Tab> tab)
{
    if (notebookWindow == nullptr)
    {
        notebookWindow = std::make_unique<NotebookWindow> (engine, selection);
        notebookWindow->onClose = [safe = juce::Component::SafePointer<MainComponent> (this)]
        {
            juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->closeNotebook(); });
        };
        addAndMakeVisible (*notebookWindow);
    }

    if (tab.has_value())
        notebookWindow->showTab (*tab);

    transport.setNotebookShown (true);
    commandManager.commandStatusChanged();
    resized();
}

void MainComponent::closeNotebook()
{
    notebookWindow.reset();
    transport.setNotebookShown (false);
    commandManager.commandStatusChanged();
    resized();
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

    // The links between guides must name pages that exist, including the built-ins' pages.
    const auto linkProblems = gen::guides::validateLinks (*entries);

    for (auto& problem : linkProblems)
        std::cout << "Links: FAILED: " << problem << std::endl;

    if (linkProblems.isEmpty())
        std::cout << "Links: " << gen::guides::getLinks().size() << " links between guides, all pages exist" << std::endl;
    else
        *ok = false;

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

                                      bool namesRight = true, sourcesLast = true;

                                      for (size_t g = 0; g < finished.guides.size() && g < report.topicNames.size(); ++g)
                                      {
                                          // Every guide's last topic is Sources.
                                          const auto& names = report.topicNames[g];
                                          sourcesLast = sourcesLast && ! names.isEmpty() && names[names.size() - 1] == gen::guides::sourcesTopicName;

                                          for (auto& e : *entries)
                                          {
                                              if (e.guide != finished.guides[g] || e.topics.isEmpty())
                                                  continue;

                                              // The chord and structure guides list their series of progressions and structures as one topic.
                                              // The comparison leaves Sources out: it was checked above.
                                              const int ownTopics = e.topics.size() - 1, ownNames = names.size() - 1;
                                              const bool grouped = ownNames > ownTopics;
                                              const int same = grouped ? ownTopics - 1 : ownTopics;

                                              for (int t = 0; t < same && t < ownNames; ++t)
                                                  namesRight = namesRight && e.topics[t] == names[t];

                                              namesRight = namesRight && (grouped || ownNames == ownTopics);
                                          }
                                      }

                                      const bool good = result.wasOk() && report.topicsOnOwnPages && namesRight && sourcesLast;
                                      *ok = *ok && good;
                                      std::cout << finished.name << ": " << topics << " topics, " << report.pages << " pages, "
                                                << juce::String ((double) file.getSize() / (1024.0 * 1024.0), 1) << " MB, "
                                                << juce::String ((juce::Time::getMillisecondCounterHiRes() - started) / 1000.0, 1) << " s"
                                                << (good ? "" : "  FAILED: " + (result.failed() ? result.getErrorMessage()
                                                                                                    : juce::String (! sourcesLast ? "a guide's last topic isn't Sources"
                                                                                                                  : namesRight ? "a topic doesn't start its own page" : "the topics differ from the Theory Guides list")))
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
        case G::keyFinding:
            if (keyGuide != nullptr)
                item.useOpen (keyGuide.getComponent());
            else
                item.adopt (std::make_unique<KeyFindingExplainer> (engine, selection));
            break;

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
                item.adopt (std::make_unique<RhythmExplainer> (engine, [this] { return RhythmExplainer::ClipView { pianoRoll.getTrack(), pianoRoll.getClip() }; }));
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

        case G::handover:
            if (handoverGuide != nullptr)
                item.useOpen (handoverGuide.getComponent());
            else
                item.adopt (std::make_unique<HandoverExplainer> (engine));

            break;

        case G::instruments:
            if (instrumentsGuide != nullptr)
                item.useOpen (instrumentsGuide.getComponent());
            else
                item.adopt (std::make_unique<InstrumentsExplainer> (engine, selection));

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
        case G::harmoniser:
        case G::stereo:
        case G::reverb:
        case G::delay:
        case G::slicer:
        case G::arpeggiator:
        case G::stepFilter:
        case G::spectralFreeze:
        case G::saturation:
        case G::distortion:
        case G::subtractive:
        case G::additive:
        case G::fm:
        case G::drums:
        case G::physical:
        case G::sampler:
        case G::loudness:
            return builtInGuides.getPrintGuide (gen::guides::getEntry (guide).plugin);
    }

    return item;
}

void MainComponent::measureLoudness (bool wholeSong)
{
    // The result is kept; the guide opens at its last page, "Your track", where the numbers and the advice are.
    loudnessui::measureSong (engine, [this] (const metertext::SongResult& result)
                             {
                                 recordMeasurement (result);
                                 openGuide (gen::guides::Guide::loudness, 10);
                             }, wholeSong);
}

void MainComponent::showTheoryGuides()
{
    pluginWindows.show (&theoryGuides, "Theory Guides", [this]
    {
        auto* view = new TheoryGuidesView (BuiltInGuides::getEntries());
        view->onOpen = [this] (gen::guides::Guide guide, int topic) { openGuide (guide, topic); };
        view->describeTarget = [this] (const gen::guides::Entry& entry) { return builtInGuides.describeTarget (entry.plugin); };
        view->getExerciseState = [this] { return getExerciseState(); };
        view->onExercise = [this] (bool restart)
        {
            showFirstSongExercise();

            if (restart)
                if (auto* window = getExerciseWindow (gen::exercises::Exercise::firstSong))
                    window->startOver();
        };
        view->refreshCards();   // the first build ran before the strip's callbacks were set
        theoryGuides = view;
        return view;
    });
}

void MainComponent::openGuide (gen::guides::Guide guide, int topic)
{
    using G = gen::guides::Guide;

    switch (guide)
    {
        case G::keyFinding:
            showKeyGuide (juce::isPositiveAndBelow (topic, (int) std::size (gen::keyfinding::allTopics)) ? gen::keyfinding::allTopics[topic] : gen::keyfinding::Topic::key);

            if (keyGuide != nullptr && topic == (int) std::size (gen::keyfinding::allTopics))
                keyGuide->showSources();
            break;

        case G::readMusic:
            showReadingGuide();

            if (readingGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (gen::readmusic::allTopics)))
                readingGuide->showTopic (gen::readmusic::allTopics[topic]);
            else if (readingGuide != nullptr && topic == (int) std::size (gen::readmusic::allTopics))
                readingGuide->showSources();
            break;

        case G::sound:
            showSoundGuide();

            if (soundGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (soundguide::allTopics)))
                soundGuide->showTopic (soundguide::allTopics[topic]);
            else if (soundGuide != nullptr && topic == (int) std::size (soundguide::allTopics))
                soundGuide->showSources();
            break;

        case G::midi:
            showMidiGuide();

            if (midiGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (midiguide::allTopics)))
                midiGuide->showTopic (midiguide::allTopics[topic]);
            else if (midiGuide != nullptr && topic == (int) std::size (midiguide::allTopics))
                midiGuide->showSources();
            break;

        case G::rhythm:
            showRhythmGuide();

            if (rhythmGuide != nullptr && juce::isPositiveAndBelow (topic, (int) std::size (gen::rhythm::allTopics)))
                rhythmGuide->showTopic (gen::rhythm::allTopics[topic]);
            else if (rhythmGuide != nullptr && topic == (int) std::size (gen::rhythm::allTopics))
                rhythmGuide->showSources();
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

            if (stylesGuide != nullptr && topic == (int) std::size (gen::stylesguide::allTopics))
                stylesGuide->showSources();
            break;

        case G::gainStaging:
            deviceBar.showGuide (juce::isPositiveAndBelow (topic, (int) std::size (gainstaging::allTopics))
                                     ? gainstaging::allTopics[topic] : gainstaging::Topic::what,
                                 topic == (int) std::size (gainstaging::allTopics));
            break;

        case G::routing:
            showRoutingGuide (juce::isPositiveAndBelow (topic, (int) std::size (routingguide::allTopics)) ? routingguide::allTopics[topic] : routingguide::Topic::what);

            if (routingGuide != nullptr && topic == (int) std::size (routingguide::allTopics))
                routingGuide->showSources();
            break;

        case G::handover:
            showHandoverGuide (juce::isPositiveAndBelow (topic, (int) std::size (handoverguide::allTopics)) ? handoverguide::allTopics[topic] : handoverguide::Topic::stems);

            if (handoverGuide != nullptr && topic == (int) std::size (handoverguide::allTopics))
                handoverGuide->showSources();
            break;

        case G::instruments:
            showInstrumentsGuide (juce::isPositiveAndBelow (topic, (int) std::size (gen::instruments::allTopics)) ? gen::instruments::allTopics[topic] : gen::instruments::Topic::makeSound);

            if (instrumentsGuide != nullptr && topic == (int) std::size (gen::instruments::allTopics))
                instrumentsGuide->showSources();
            break;

        case G::recording:
            showRecordingGuide (juce::isPositiveAndBelow (topic, (int) std::size (recordingguide::allTopics)) ? recordingguide::allTopics[topic] : recordingguide::Topic::air);

            if (recordingGuide != nullptr && topic == (int) std::size (recordingguide::allTopics))
                recordingGuide->showSources();
            break;

        case G::triggers:
            showTriggerGuide (juce::isPositiveAndBelow (topic, (int) std::size (triggerguide::allTopics)) ? triggerguide::allTopics[topic] : triggerguide::Topic::whatItIs);

            if (triggerGuide != nullptr && topic == (int) std::size (triggerguide::allTopics))
                triggerGuide->showSources();
            break;

        case G::tuning:
            showTuningGuide (juce::isPositiveAndBelow (topic, (int) std::size (gen::tuning::allTopics)) ? gen::tuning::allTopics[topic] : gen::tuning::Topic::whatItShows);

            if (tuningGuide != nullptr && topic == (int) std::size (gen::tuning::allTopics))
                tuningGuide->showSources();
            break;

        case G::eq:
        case G::compressor:
        case G::gate:
        case G::tune:
        case G::harmoniser:
        case G::stereo:
        case G::reverb:
        case G::delay:
        case G::slicer:
        case G::arpeggiator:
        case G::stepFilter:
        case G::spectralFreeze:
        case G::saturation:
        case G::distortion:
        case G::subtractive:
        case G::additive:
        case G::fm:
        case G::drums:
        case G::physical:
        case G::sampler:
        case G::loudness:
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
