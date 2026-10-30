#pragma once

#include "engine/Engine.h"
#include "generate/NoteRealizer.h"
#include "Theme.h"

namespace daw
{
/**
    Side panel of the piano roll that asks a local LLM (through Ollama) to write a part
    into the clip being edited: chords, melody, bassline, arpeggio, drums or percussion, in the song's
    key and tempo, for a chosen style. The clip's length sets how many bars are written.
    Parts follow the chord track; chords written for bars the chord track leaves empty are added to it.
    Each click makes a few takes, scores them and puts the best in the clip; the take buttons swap in the others.
*/
class GeneratePanel final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit GeneratePanel (Engine&);
    ~GeneratePanel() override;

    void setTarget (Track*, MidiClip*);

    std::function<void()> onClose;
    std::function<void()> onShowReadingGuide;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    static constexpr int preferredWidth = 300;

private:
    friend class ScreenshotRunner;
    /** The clip's track, or null when the clip is gone (deleted, or replaced by another project). */
    Track* findTrackFor (const MidiClip*) const;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void refreshModels();
    void updatePlayingStyles();
    juce::String getPartId() const;          // "chords", "arpeggio" or "bassline": the parts with playing styles; else empty
    juce::String getPlayingStyleId() const;  // the chosen playing style, or empty for Free
    bool playsWithoutModel() const;          // a playing style as it is, over chords the chord track has
    void generate();
    void playStyle (const gen::Request&);
    void cancel();
    void takeArrived (int id, const gen::Result&, const juce::String& error, const juce::StringArray& directions);
    void allTakesArrived (int id);
    void finishGeneration (bool stoppedEarly);
    void applyTake (int index);
    void restoreClip();
    void undo();
    void updateTakeButtons();
    void generateChatMusician();
    void previewChatMusician();
    void applyChatMusician (bool chordsOnly);
    void undoChatMusician();
    bool chatSnapshotValid() const;

    gen::Request makeRequest() const;
    int getClipBars() const;
    juce::String describeClipLength() const;
    juce::String describeChordTrack() const;
    juce::StringArray collectHarmony (int bars) const;
    void setStatus (const juce::String&, bool isError = false);
    void updateControls();
    void saveSettings();
    juce::String getServerUrl() const;

    Engine& engine;
    Track* track = nullptr;
    MidiClip* clip = nullptr;

    juce::Viewport viewport;
    juce::Component form;

    juce::Label title, infoLabel;
    juce::TextButton closeButton { "x" };
    juce::Label partLabel { {}, "Part" }, styleLabel { {}, "Style" },
                promptLabel { {}, "Prompt" }, modelLabel { {}, "Model" }, variationLabel { {}, "Variation" },
                playingLabel { {}, "Playing" };
    juce::ComboBox partBox, styleBox, modelBox, takesBox, playingBox;
    juce::StringArray playingIds;   // the playing box's styles: item id 2 is the first, 1 is Free
    juce::Slider variationSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::TextEditor promptEditor;
    juce::TextButton refreshButton { "Refresh" }, generateButton { "Generate" }, undoButton { "Undo" };
    juce::ToggleButton replaceToggle { "Replace the clip's notes" }, thinkToggle { "Think first (slower)" },
                       varyToggle { "Vary it with the AI" };
    juce::Label statusLabel;

    juce::ToggleButton chatMode { "ChatMusician" }, chatSevenths { "Sevenths" },
                       chatBorrowed { "Borrowed chords" }, chatDominant { "Major dominant in minor" };
    juce::ComboBox chatTask;
    juce::TextEditor chatFirst, chatLast;
    juce::TextButton chatPreview { "Preview" }, chatApplyNotes { "Apply MIDI" },
                     chatApplyChords { "Apply chords" }, chatUndo { "Undo ChatMusician" }, chatGuide { "How to read music..." };
    MidiClip* chatClip = nullptr;
    int chatEditGeneration = -1;
    double chatStart = 0.0, chatLength = 0.0;
    MusicalKey chatKey;
    std::vector<MidiNote> chatOriginalNotes, chatNotes, chatAppliedNotes;
    ChordMap chatOriginalChords, chatAppliedChords;
    std::vector<Chord> chatChords;
    bool chatStaged = false, chatAppliedMidi = false, chatAppliedLane = false, chatAuditioning = false;

    // The running request, if any. Results from older requests are ignored.
    int generationId = 0;
    std::shared_ptr<std::atomic<bool>> cancelFlag;
    MidiClip* generatingClip = nullptr;
    int clipsEditGeneration = -1;   // the engine's edit generation when generatingClip was picked
    gen::Request generatingRequest;
    int spinnerPhase = 0;
    juce::String progressText;

    // Takes of the last generation, in the order they were made; one of them is in the clip.
    struct Take
    {
        gen::Result result;
        double score = 0.0;
        juce::StringArray directions;
    };

    std::vector<Take> takes, pendingTakes;  // pending: those of the running generation, as they arrive
    gen::Request takesRequest;
    juce::String takesClipName;
    int failedTakes = 0, shownTake = -1, bestTake = 0;
    juce::String takeError;      // why the first failed take failed
    juce::OwnedArray<juce::TextButton> takeButtons;

    // State before the last generation, for Undo and for swapping takes.
    MidiClip* undoClip = nullptr;
    std::vector<MidiNote> undoNotes;
    double undoLength = 0.0;
    juce::String undoName;
    ChordMap undoAddedChords;

    bool modelsLoaded = false;
    int modelRefreshId = 0;
};
} // namespace daw
