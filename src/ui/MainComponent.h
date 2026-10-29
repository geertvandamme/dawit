#pragma once

#include "GuidePrint.h"
#include "ArrangementView.h"
#include "ComputerKeyboardStrip.h"
#include "ComputerKeyboardTyping.h"
#include "DeviceBar.h"
#include "InspectorPanel.h"
#include "MixerView.h"
#include "NewFromStyleWindow.h"
#include "PianoRoll.h"
#include "ProjectManager.h"
#include "ReadingExplainer.h"
#include "RhythmExplainer.h"
#include "RecordingExplainer.h"
#include "DrumTriggerExplainer.h"
#include "TuningExplainer.h"
#include "RoutingExplainer.h"
#include "SignalFlowView.h"
#include "SoundExplainer.h"
#include "StylesExplainer.h"
#include "TheoryGuidesWindow.h"
#include "MidiExplainer.h"
#include "TransportBar.h"
#include "TransportShortcuts.h"
#include "TunerWindow.h"
#include "WaveEditor.h"

namespace daw
{
class MainComponent final : public juce::Component,
                            public juce::ApplicationCommandTarget,
                            public juce::MenuBarModel,
                            private juce::ChangeListener
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

    juce::StringArray getMenuBarNames() override { return { "File", "Track", "View", "Help" }; }
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
        foldSongRowsCommand,
        playAlongCommand,
        singAlongCommand,
        tunerCommand,
        theoryGuidesCommand,
        newAudioTrackCommand,
        newMidiTrackCommand,
        newBusCommand,
        createBusCommand,
        bounceInPlaceCommand,
        newReverbReturnCommand,
        newDelayReturnCommand,
        newReturnCommand,
        computerKeyboardCommand,
        toggleInspectorCommand
    };

    enum class BottomPanel { clipEditor, mixer, closed };

    static constexpr int recentProjectsBaseId = 100;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void addTrack (Track::Type);
    void showPluginList();
    void showAudioSettings();
    void showReadingGuide();
    void showRhythmGuide();
    void showSoundGuide();
    void showMidiGuide();

    /** Shows or hides the computer keyboard strip to match `typing`, and updates the button and menu. */
    void updateTypingStrip();

    /** Opens the Tuner window, or brings it to the front. */
    void showTuner();

    /** Opens the style chooser, optionally with one style selected. */
    void showNewFromStyle (const juce::String& styleId = {});

    /** Opens Styles of music at one of its topics. */
    void showStylesGuide (gen::stylesguide::Topic);

    /** Opens the How recording works guide at a topic. */
    void showRecordingGuide (recordingguide::Topic);
    void showTriggerGuide (triggerguide::Topic);
    std::optional<triggerguide::Part> getTriggerGuidePart();
    void showTuningGuide (gen::tuning::Topic);

    /** Opens the Buses, sends and routing guide at a topic. */
    void showRoutingGuide (routingguide::Topic);

    /** Opens the Signal Flow window: the whole song as a live flow diagram (it only shows the song). */
    void showSignalFlow();

    /** A guide for the PDF export: its open window, or one made for the export. */
    guideprint::Item getPrintGuide (gen::guides::Guide);

    /** The Theory Guides window: every guide in one place. */
    void showTheoryGuides();

    /** Opens a guide from the Theory Guides window, at a topic of its entry in gen::guides or -1 at its start. */
    void openGuide (gen::guides::Guide, int topic);

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
    juce::Component::SafePointer<RhythmExplainer> rhythmGuide;   // its window, while open
    juce::Component::SafePointer<SoundExplainer> soundGuide;     // its window, while open
    juce::Component::SafePointer<MidiExplainer> midiGuide;       // its window, while open
    juce::Component::SafePointer<StylesExplainer> stylesGuide;   // its window, while open
    juce::Component::SafePointer<NewFromStyleWindow> newFromStyle; // its window, while open
    juce::Component::SafePointer<RecordingExplainer> recordingGuide; // its window, while open
    juce::Component::SafePointer<DrumTriggerExplainer> triggerGuide; // its window, while open
    juce::Component::SafePointer<TuningExplainer> tuningGuide;       // its window, while open
    juce::Component::SafePointer<TunerWindow> tunerWindow;       // its window, while open
    juce::Component::SafePointer<RoutingExplainer> routingGuide; // its window, while open
    juce::Component::SafePointer<SignalFlowView> signalFlow;     // its window, while open
    juce::Component::SafePointer<TheoryGuidesView> theoryGuides; // its window, while open
    std::unique_ptr<ComputerKeyboardStrip> typingStrip;   // docked at the bottom while typing is on
    juce::TooltipWindow tooltips { this, 600 };

    ProjectManager project { engine };
    juce::ApplicationCommandManager commandManager;
   #if ! JUCE_MAC
    juce::MenuBarComponent menuBar { this };
   #endif

    static constexpr int transportHeight = 46;
};
} // namespace daw
