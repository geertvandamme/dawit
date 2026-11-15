#pragma once

#include "GuidePointer.h"
#include "GuidePrint.h"
#include "ArrangementView.h"
#include "ComputerKeyboardStrip.h"
#include "ComputerKeyboardTyping.h"
#include "ExerciseWindow.h"
#include "DeviceBar.h"
#include "InspectorPanel.h"
#include "MixerView.h"
#include "MeterGuide.h"
#include "NewFromStyleWindow.h"
#include "PianoRoll.h"
#include "ProjectManager.h"
#include "KeyFindingExplainer.h"
#include "InstrumentsExplainer.h"
#include "ReadingExplainer.h"
#include "RhythmExplainer.h"
#include "HandoverExplainer.h"
#include "RecordingExplainer.h"
#include "DrumTriggerExplainer.h"
#include "TuningExplainer.h"
#include "RoutingExplainer.h"
#include "SignalFlowView.h"
#include "NotebookWindow.h"
#include "SoundExplainer.h"
#include "StylesExplainer.h"
#include "TheoryGuidesWindow.h"
#include "GuideNavigator.h"
#include "MidiExplainer.h"
#include "TransportBar.h"
#include "TransportShortcuts.h"
#include "TunerWindow.h"
#include "AboutWindow.h"
#include "WaveEditor.h"

namespace daw
{
class MainComponent final : public juce::Component,
                            public juce::ApplicationCommandTarget,
                            public juce::MenuBarModel,
                            private juce::ChangeListener,
                            private juce::Timer
{
public:
    explicit MainComponent (Engine&);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

    /** Fills the edit with a couple of example tracks (used by the --demo flag). */
    void createDemoContent();

    /** The developer flag --export-guides <folder>: exports every guide to <folder>/<name>.pdf, then Start here,
        one after another, printing a line for each, and calls back with whether every export worked and every topic
        started its own page with the name the Theory Guides list gives it. */
    void exportAllGuides (const juce::File& folder, std::function<void (bool ok)> done);

    /** Opens a project file, after offering to save unsaved changes. */
    void openProject (const juce::File& file) { project.openFile (file); }

    /** Offers to save unsaved changes, then calls `close` unless the user cancelled. */
    void askToClose (std::function<void()> close) { project.askToSaveChanges (std::move (close)); }

    // Commands (with their shortcuts) and the File menu.
    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands (juce::Array<juce::CommandID>&) override;
    void getCommandInfo (juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo&) override;

    juce::StringArray getMenuBarNames() override { return { "File", "Edit", "Track", "View", "Help" }; }
    juce::PopupMenu getMenuForIndex (int index, const juce::String& name) override;
    void menuItemSelected (int itemId, int topLevelMenuIndex) override;

private:
    friend class ScreenshotRunner;   // the --screenshots developer flag sets up views directly

    enum CommandIDs
    {
        newProjectCommand = 0x2000,
        newFromStyleCommand,
        openProjectCommand,
        saveProjectCommand,
        saveProjectAsCommand,
        bounceCommand,
        measureLoudnessCommand,
        exportChordSheetCommand,
        exportSheetMusicCommand,
        exportGuidesCommand,
        importAudioCommand,
        showPluginsCommand,
        audioSettingsCommand,
        showClipEditorCommand,
        showScoreCommand,
        toggleMixerCommand,
        signalFlowCommand,
        notebookCommand,
        foldSongRowsCommand,
        followPlayheadCommand,
        showSelectedTrackCommand,
        playAlongCommand,
        singAlongCommand,
        tunerCommand,
        aboutCommand,
        theoryGuidesCommand,
        firstSongExerciseCommand,
        songChordsExerciseCommand,
        writeLyricsExerciseCommand,
        soundBetterExerciseCommand,
        mixExportExerciseCommand,
        newAudioTrackCommand,
        newMidiTrackCommand,
        newBusCommand,
        createBusCommand,
        bounceInPlaceCommand,
        newReverbReturnCommand,
        newDelayReturnCommand,
        newReturnCommand,
        computerKeyboardCommand,
        toggleInspectorCommand,
        undoCommand,
        redoCommand
    };

    enum class BottomPanel { clipEditor, mixer, closed };

    static constexpr int recentProjectsBaseId = 100;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    /** The quiet-moment commit: five times a second, when no mouse button is down and no menu or dialog is up, whatever
        changed since the last step becomes a step of the undo history. */
    void timerCallback() override;

    /** True while an edit must not be committed or undone: a mouse button is down (a drag is one step), or a modal window is up. */
    static bool isHistoryBusy();

    /** Undo (or redo) the latest step: commits what is pending, restores it, then forgets what left the edit (selection,
        plugin windows, the live MIDI track, the piano roll's note selection). A focused text box takes the key for its own text instead. */
    void stepHistory (bool redo);
    void addTrack (Track::Type);

    /** The start panel's tiles (the song has no tracks): each starts a real task, using the same commands as the menus. */
    void handleStartChoice (StartPanel::Choice, juce::Component& tile);

    /** The menu of built-in sounds (a header saying what a MIDI track is, the eight sounds, then *What is MIDI?*), shown
        beside `target`. A choice runs `addInstrumentTrack`. */
    void showAddInstrumentMenu (juce::Component& target);

    /** Adds a MIDI track named after the sound, with an empty four-bar clip, selects and reveals it, shows the clip
        editor and points at the piano roll's grid. Keeps the computer keyboard as it is. One undo step. */
    void addInstrumentTrack (BuiltInSound, bool typingIsOn = false);

    /** Opens an exercise's window (first time at the main window's top right); reopening brings it to the front.
        One window per exercise. */
    void showExercise (gen::exercises::Exercise);

    /** Help > First Song Exercise...: `showExercise (firstSong)`. */
    void showFirstSongExercise() { showExercise (gen::exercises::Exercise::firstSong); }

    /** The exercise's window while it is open, or null. */
    ExerciseWindow* getExerciseWindow (gen::exercises::Exercise exercise) const noexcept
    {
        return exerciseWindows[(size_t) exercise].getComponent();
    }

    /** What an exercise step's button does (`fix` none: the step's own action), and what its *Show me* points at. */
    void runExerciseAction (gen::exercises::Exercise, int step, gen::exercises::Fix fix);
    void showExerciseTarget (gen::exercises::Exercise, int step);
    void runSongChordsAction (int step, gen::exercises::Fix fix);
    void showSongChordsTarget (int step);
    void runWriteLyricsAction (int step, gen::exercises::Fix fix);
    void showWriteLyricsTarget (int step);
    void runSoundBetterAction (int step, gen::exercises::Fix fix);
    void showSoundBetterTarget (int step);
    void runMixExportAction (int step, gen::exercises::Fix fix);
    void showMixExportTarget (int step);

    /** The app facts an exercise's steps ask about besides the song itself. */
    gen::exercises::Context getExerciseContext (gen::exercises::Exercise which = gen::exercises::Exercise::firstSong) const;

    /** Help > Learn the Chords of a Song...: `showExercise (songChords)`. */
    void showSongChordsExercise() { showExercise (gen::exercises::Exercise::songChords); }

    /** Help > Write Lyrics to Your Song... and the Lyrics row's menu: `showExercise (writeLyrics)`. */
    void showWriteLyricsExercise() { showExercise (gen::exercises::Exercise::writeLyrics); }

    /** Help > Make a Track Sound Better... and the track menu's item: `showExercise (soundBetter)`. */
    void showSoundBetterExercise() { showExercise (gen::exercises::Exercise::soundBetter); }

    /** Help > Mix and Export Your Song...: `showExercise (mixExport)`. */
    void showMixExportExercise() { showExercise (gen::exercises::Exercise::mixExport); }

    /** For screenshots: sets a track's fader (dB) or pan (-1 to 1) the way the mixer does, and fills the exercise's
        loudness and export records without writing a file. */
    void setTrackMixForShot (const juce::String& trackName, std::optional<float> volumeDb, std::optional<float> pan);
    void setMixExportMeasuredForShot();
    void setMixExportExportedForShot (const gen::mixexport::ExportRecord&);

    /** For screenshots: fills the make-a-track-sound-better records for the chosen track: seconds listened, the most dB the
        compressor turned it down, and whether each exercise effect was heard off and on. */
    void setSoundBetterRecordsForShot (double listenedSeconds, double reductionDb, bool compared);

    /** For screenshots: marks the song as played through the exercise's line with Sing along open. */
    void setWriteLyricsSungForShot();

    /** For screenshots: counts `bars` bars as played along, and pretends there is no audio output device. */
    void setExercisePlayedAlongForShot (int bars);
    void setExerciseNoOutputForShot (bool noOutput) noexcept { exerciseNoOutput = noOutput; }

    /** Opens the guide page an exercise link names. */
    void openExerciseLink (const gen::exercises::Link&);

    /** Opens Theory Guides at Start here. */
    void showStartHere();

    /** Where the exercise stands, for the inspector's welcome and the strip in Theory Guides. */
    TheoryGuidesView::ExerciseState getExerciseState();

    /** Adds an audio track "Recording", selects and reveals it and points at its R button; with no audio input on the
        device the hint says so and offers Audio/MIDI Settings. It never arms or records. One undo step. */
    void addRecordingTrack (bool pretendNoInput = false);
    void showPluginList();
    void showAudioSettings();
    void showReadingGuide();
    void showKeyGuide (gen::keyfinding::Topic);
    void showRhythmGuide();
    void showTimeSignatureGuide();   // the rhythm guide, on the topic Time signatures
    void showSoundGuide();
    void showMidiGuide();

    /** Shows or hides the computer keyboard strip to match `typing`, and updates the button and menu. */
    void updateTypingStrip();

    /** Opens the Tuner window, or brings it to the front. */
    void showTuner();

    /** Opens the About DawIt window (or brings it forward). */
    void showAbout();

    /** Opens the Licences window (or brings it forward); a notice's file name selects that entry. */
    void showLicences (const juce::String& fileName = {});

    /** Opens the style chooser, optionally with one style selected. */
    void showNewFromStyle (const juce::String& styleId = {});

    /** Opens Styles of music at one of its topics. */
    void showStylesGuide (gen::stylesguide::Topic);

    /** Opens the How recording works guide at a topic. */
    void showRecordingGuide (recordingguide::Topic);
    void showHandoverGuide (handoverguide::Topic);
    void showInstrumentsGuide (gen::instruments::Topic);
    void showTriggerGuide (triggerguide::Topic);
    std::optional<triggerguide::Part> getTriggerGuidePart();
    void showTuningGuide (gen::tuning::Topic);

    /** Opens the Buses, sends and routing guide at a topic. */
    void showRoutingGuide (routingguide::Topic);

    /** Opens the Signal Flow window: the whole song as a live flow diagram (it only shows the song). */
    void showSignalFlow();

    /** Opens the docked Notebook at a tab (none: the one it was left on). */
    void showNotebook (std::optional<NotebookWindow::Tab> tab = std::nullopt);

    /** A guide for the PDF export: its open window, or one made for the export. */
    guideprint::Item getPrintGuide (gen::guides::Guide);

    /** The Theory Guides window: every guide in one place. */
    void showTheoryGuides();

    /** Opens a guide from the Theory Guides window, at a topic of its entry in gen::guides or -1 at its start. */
    void openGuide (gen::guides::Guide, int topic);
    /** File > Measure Loudness...: measures the song offline, then opens the loudness guide at Your track. */
    void measureLoudness (bool wholeSong = false);

    /** Shows the clip editor or the mixer under the device bar, or closes that panel, and remembers it. */
    void showBottomPanel (BottomPanel);

    /** What the panel shows: the mixer, or the clip editor for the selection (the wave editor for an
        audio clip, otherwise the piano roll). */
    juce::Component& getBottomPanel() noexcept;
    void updateBottomPanel();

    /** Shows or hides the inspector on the left, and remembers it (`view.inspectorOpen`). */
    void setInspectorOpen (bool);

    Engine& engine;
    Selection selection;
    TransportShortcuts shortcuts { engine };   // declared before the windows that listen to it
    ComputerKeyboardTyping typing { engine, shortcuts };   // ahead of the shortcuts: a mapped letter plays
    /** Follows links between guides, keeps Back and the pages read. Declared before the windows whose footers listen to it. */
    GuideNavigator guideNavigator { engine.properties.getUserSettings(), [] { return BuiltInGuides::getEntries(); },
                                    [this] (gen::guides::Guide guide, int topic) { openGuide (guide, topic); } };
    PluginWindowManager pluginWindows;
    BuiltInGuides builtInGuides { engine, selection, pluginWindows };   // after the windows it opens

    TransportBar transport { engine };
    ArrangementView arrangement { engine, selection, pluginWindows };
    juce::StretchableLayoutManager layout;
    juce::StretchableLayoutResizerBar divider { &layout, 1, false };
    DeviceBar deviceBar { engine, selection, pluginWindows, arrangement.getTrackActions() };
    PianoRoll pianoRoll { engine, selection };
    WaveEditor waveEditor { engine, selection };
    MixerView mixer { engine, selection, arrangement.getTrackActions() };
    BottomPanel bottomPanel = BottomPanel::clipEditor;
    InspectorPanel inspector { engine, selection, pluginWindows, arrangement.getTrackActions() };
    InspectorEdge inspectorEdge;
    bool inspectorOpen = true;
    int inspectorWidth = InspectorPanel::defaultWidth;

    std::unique_ptr<juce::DocumentWindow> pluginListWindow;
    juce::Component::SafePointer<ReadingExplainer> readingGuide;   // its window, while open
    juce::Component::SafePointer<KeyFindingExplainer> keyGuide;    // its window, while open
    juce::Component::SafePointer<RhythmExplainer> rhythmGuide;   // its window, while open
    juce::Component::SafePointer<SoundExplainer> soundGuide;     // its window, while open
    juce::Component::SafePointer<MidiExplainer> midiGuide;       // its window, while open
    juce::Component::SafePointer<StylesExplainer> stylesGuide;   // its window, while open
    juce::Component::SafePointer<NewFromStyleWindow> newFromStyle; // its window, while open
    juce::Component::SafePointer<HandoverExplainer> handoverGuide; // its window, while open
    juce::Component::SafePointer<InstrumentsExplainer> instrumentsGuide;   // its window, while open
    juce::Component::SafePointer<RecordingExplainer> recordingGuide; // its window, while open
    juce::Component::SafePointer<DrumTriggerExplainer> triggerGuide; // its window, while open
    juce::Component::SafePointer<TuningExplainer> tuningGuide;       // its window, while open
    juce::Component::SafePointer<TunerWindow> tunerWindow;       // its window, while open
    juce::Component::SafePointer<AboutComponent> aboutWindow;    // its window, while open
    juce::Component::SafePointer<LicencesComponent> licencesWindow; // its window, while open
    juce::PopupMenu appleMenuExtras;                              // About DawIt... in the macOS application menu
    juce::Component::SafePointer<RoutingExplainer> routingGuide; // its window, while open
    juce::Component::SafePointer<SignalFlowView> signalFlow;     // its window, while open
    std::unique_ptr<NotebookWindow> notebookWindow; // the panel, while open
    /** Closes the Notebook and saves pending App notes. */
    void closeNotebook();
    juce::Component::SafePointer<TheoryGuidesView> theoryGuides; // its window, while open
    juce::Component::SafePointer<ExerciseWindow> exerciseWindows[gen::exercises::numExercises];   // each exercise's window, while open
    ChordMap exerciseChordBaseline;   // the chords when the exercise began: step 2 is done once they differ
    gen::exercises::StepMemory exerciseSteps;   // each exercise's open step, this session and project only
    bool exerciseBaselineSet = false;
    ChordMap songChordsBaseline;   // the chords the song chords exercise compares with: step 4 is done once one has changed
    gen::songchords::PlayedAlongCounter playedAlong;   // bars heard with a chord while Play along was open
    bool songChordsBaselineSet = false;
    gen::songchords::TempoRecord songTempoRecord;   // the song clip and tempo as Detect tempo (or a lined-up result) left them; cleared when another song opens
    bool exerciseNoOutput = false;   // screenshots only
    /** Starts the song chords exercise's memory afresh: the baseline is now, nothing played along. */
    void resetSongChordsMemory();
    /** Feeds the played-along counter from the heard playhead (5 Hz). */
    void feedPlayedAlong();

    int writeLyricsSectionBar = -1;   // the write-lyrics exercise's section chosen earlier this session (its start bar); -1 none
    gen::writelyrics::SungTracker writeLyricsSung;   // the song played through the exercise's line with Sing along open
    /** Forgets the write-lyrics exercise's remembered section and sung record (another song, or Start over). */
    void resetWriteLyricsMemory();
    /** Remembers the chosen section while the loop is on, and feeds the sung tracker (5 Hz). */
    void feedWriteLyrics();
    gen::mixexport::Baseline mixBaseline;   // the mix and export exercise: each track's fader and pan when it opened (valid once taken)
    gen::mixexport::MeasureRecord mixMeasured;   // the last loudness measurement, and the last finished bounce
    gen::mixexport::ExportRecord mixExported;
    /** Forgets the baseline and both records (another song, or Start over): the baseline is taken afresh when `takeBaseline` is true. */
    void resetMixExportMemory (bool takeBaseline);
    /** Remembers the song measurement that has just finished (the range it covered comes from `loudnessui::getLastRange`). */
    void recordMeasurement (const metertext::SongResult&);
    const Track* soundBetterRemembered = nullptr;   // the make-a-track-sound-better exercise's track, chosen earlier this session (compared with the engine's tracks before use)
    gen::soundbetter::ListenTracker soundBetterListen;
    gen::soundbetter::ReductionTracker soundBetterReduction;
    gen::soundbetter::CompareTracker soundBetterCompare;
    double soundBetterLastFeedMs = 0.0;
    /** Forgets the exercise's chosen track and records (another song, or Start over). */
    void resetSoundBetterMemory();
    /** Follows the chosen track (the records start afresh when it changes) and feeds the three trackers (5 Hz). */
    void feedSoundBetter();
    std::unique_ptr<ComputerKeyboardStrip> typingStrip;   // docked at the bottom while typing is on
    GuidePointer pointer;   // the hint over everything: where to click next
    juce::TooltipWindow tooltips { this, 600 };

    ProjectManager project { engine };
    juce::ApplicationCommandManager commandManager;
   #if ! JUCE_MAC
    juce::MenuBarComponent menuBar { this };
   #endif

    static constexpr int transportHeight = 46;
};
} // namespace daw
