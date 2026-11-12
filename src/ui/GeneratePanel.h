#pragma once

#include "engine/Engine.h"
#include "generate/AiSetup.h"
#include "generate/GenerateStaging.h"
#include "generate/NoteRealizer.h"
#include "Theme.h"

namespace daw
{
/**
    Side panel of the piano roll that makes a part for the clip being edited, in one of two modes: a built-in pattern
    (DawIt's own library over the chord track; no AI, works offline) or an AI part written by a local model through Ollama.

    Every mode makes the same promise: Generate stages a take (it shows as outlines over the clip's notes and plays on the
    track's own instrument when you preview it), then Apply writes it as one undo step, or Discard leaves the song as it
    was. The target clip, the replace/add choice and any proposed chords are always shown in the footer.
*/
class GeneratePanel final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit GeneratePanel (Engine&);
    ~GeneratePanel() override;

    void setTarget (Track*, MidiClip*);

    std::function<void()> onClose;
    std::function<void()> onShowStylesGuide;

    /** Runs the app's Undo (the owner wires it to its Undo command): the footer's *Undo apply* uses it. */
    std::function<void()> onUndoApply;

    /** Called when the staged take changes, so the piano roll repaints its outlines. */
    std::function<void()> onStagedChanged;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void mouseDown (const juce::MouseEvent&) override;

    static constexpr int preferredWidth = 300;

private:
    friend class ScreenshotRunner;

    /** The clip's track, or null when the clip is gone (deleted, or replaced by another project). */
    Track* findTrackFor (const MidiClip*) const;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void updateTimer();

    // Modes and settings
    void setMode (gen::aisetup::Mode, bool save = true);
    bool isMode (gen::aisetup::Mode m) const noexcept { return mode == m; }
    void refreshModels();
    void applyModelList (const gen::OllamaClient::ModelList&);
    void updateSetup();
    void updatePlayingStyles();
    void updatePartBox();
    juce::String getPartId() const;          // "chords", "arpeggio" or "bassline": the parts with playing styles; else empty
    juce::String getPlayingStyleId() const;  // the chosen playing style, or empty for Free
    juce::String getAiModel() const;         // the model the current mode uses
    void saveSettings();
    juce::String getServerUrl() const;

    // Making a take
    void generate();
    void generatePattern();
    void generateAi();
    void cancel();
    void takeArrived (int id, const gen::Result&, const juce::String& error, const juce::StringArray& directions);
    void allTakesArrived (int id);
    void finishAi();
    void stageTake (int index);
    /** Stages a result that arrived for request `id`; says why when it can't. Returns true when staged. */
    bool stageResult (int id, const std::vector<MidiNote>& notes, const std::vector<Chord>& chords, const juce::String& summary,
                      const juce::String& clipName);
    void restageIfShowing();

    // The staged take
    void previewStaged();
    void stopPreview();
    void applyStaged (Engine::GenerateScope);
    void discardStaged (const juce::String& message = {}, bool isError = false);
    void undoApply();
    void clearOverlay();
    void showOverlay();
    bool canUndoApply() const;
    void updateTakeButtons();
    void showMenu();

    gen::Request makeRequest() const;
    int getClipBars() const;
    int getFirstBar() const;
    juce::String describeBars() const;
    juce::String describeMissingChords() const;
    juce::String describeProposedChords() const;
    juce::StringArray collectHarmony (int bars) const;
    void setStatus (const juce::String&, bool isError = false);
    juce::String getInfoNote() const;
    bool hasChordOnEveryBar() const;

    void updateControls();
    void showSetup (bool show);

    /** For the screenshot runner: shows a made-up Ollama state (unreachable, noModels, notInstalled) without
        contacting Ollama. Returns false for an unknown name. */
    bool setCannedSetup (const juce::String& name);

    /** For the screenshot runner: stages three made-up melody takes with proposed chords Am F C G, as if an AI request had
        just finished, without contacting Ollama. Returns false when no clip is chosen. */
    bool stageCannedTakes();

    Engine& engine;
    Track* track = nullptr;
    MidiClip* clip = nullptr;

    gen::aisetup::Mode mode = gen::aisetup::Mode::pattern;

    juce::Viewport viewport;
    int footerTop = 0;   // where the fixed footer (actions and status) starts
    juce::Component form;

    /** A mode button that moves to the next or previous mode with the arrow keys, like a radio group. */
    struct ModeButton final : juce::TextButton
    {
        using juce::TextButton::TextButton;
        std::function<void (int)> onArrow;
        bool keyPressed (const juce::KeyPress&) override;
    };

    /** The Set up... steps, with the buttons that copy their commands. */
    struct SetupCallout final : juce::Component
    {
        void paint (juce::Graphics&) override;
    };

    juce::Label title, infoLabel, modeLine, partNote;
    juce::TextButton closeButton { "x" };
    ModeButton modeButtons[2] { ModeButton ("Built-in pattern"), ModeButton ("AI part") };

    // AI status and setup
    juce::Label aiStatus;
    juce::TextButton retryButton { "Retry" }, setupButton { "Set up..." }, usePatternButton { "Or use Built-in pattern, which needs no AI" };
    SetupCallout setupCallout;
    juce::Label setupText;
    juce::TextButton openOllamaButton { "Open ollama.com" }, copyServeButton { "Copy: ollama serve" },
                     copyPullButton { "Copy: ollama pull qwen2.5:7b" };
    bool setupShown = false;

    juce::Label partLabel { {}, "Part" }, styleLabel { {}, "Style" },
                promptLabel { {}, "Prompt" }, modelLabel { {}, "Model" }, variationLabel { {}, "Variation" },
                playingLabel { {}, "Playing" };
    juce::ComboBox partBox, styleBox, modelBox, takesBox, playingBox;
    juce::StringArray playingIds;       // the playing box's styles: item id 2 is the first, 1 is Free
    juce::Slider variationSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::TextEditor promptEditor;
    juce::ToggleButton thinkToggle { "Think first (slower)" };

    // Footer
    juce::Label intoLabel, chordLine, statusLabel;
    juce::TextButton replaceButton { "Replace notes" }, addButton { "Add to notes" }, generateButton { "Generate" },
                     undoApplyButton { "Undo apply" }, previewButton { "Preview" }, applyButton { "Apply" }, discardButton { "Discard" };
    juce::ComboBox scopeBox;
    juce::OwnedArray<juce::TextButton> takeButtons;

    // The running request, if any. Results from older requests are ignored.
    gen::staging::Staging staging;
    std::shared_ptr<std::atomic<bool>> cancelFlag;
    gen::Request generatingRequest;
    int spinnerPhase = 0;
    juce::String progressText;

    // Takes of the last AI generation, in the order they were made; one of them is staged.
    struct Take
    {
        gen::Result result;
        std::vector<Chord> chords;   // one per bar of the clip, when the take proposes chords
        double score = 0.0;
        juce::StringArray directions;
    };

    std::vector<Take> takes, pendingTakes;  // pending: those of the running generation, as they arrive
    juce::String takesClipName;             // what Apply names the clip
    gen::Part takesPart = gen::Part::chords;
    int failedTakes = 0, shownTake = -1, bestTake = 0;
    juce::String takeError;      // why the first failed take failed

    // The overlay on the engine: which clip shows the staged take, in which edit.
    MidiClip* overlayClip = nullptr;
    int overlayGeneration = -1;
    int clipsEditGeneration = -1;   // the engine's edit generation when the clip was picked

    bool previewing = false;
    double previewEnd = 0.0;

    // What the last Apply recorded, so Undo apply knows whether it is still the latest step.
    int appliedSteps = -1;
    juce::String appliedName;

    gen::OllamaClient::ModelList modelList;
    gen::aisetup::SetupInfo setupInfo;
    bool modelsLoaded = false;
    int modelRefreshId = 0;
};
} // namespace daw
