#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "generate/LyricsAi.h"
#include "generate/LyricsGuide.h"
#include "Theme.h"

namespace daw
{
/**
    The Lyrics window: the whole lyrics as text (Edit... in the Lyrics header, or a double-click on the row), with a
    Write with AI panel beside it.

    Nothing reaches the song until Apply (`lyrics::setText` keeps the timing of the words that didn't change). The
    panel asks the user's local Ollama model, on a worker thread, for the whole song, a section, the next line, a
    rewrite of the selection, or rhymes for a word; DawIt counts every suggestion's syllables and rhymes itself.
    *Use* replaces exactly the range a request was made for, through the text box, so Cmd+Z takes it back; it is off
    when the text in that range has changed. The window never changes tracks or instruments.

    The window is non-modal: the owner opens it through `PluginWindowManager`, so the song keeps playing and the
    guide can be opened from it.
*/
class LyricsEditWindow final : public juce::Component,
                               private juce::ChangeListener,
                               private juce::Timer
{
public:
    /** `firstBar` is where lines go when the song has no sections (the playhead's bar). */
    LyricsEditWindow (Engine&, int firstBar);
    ~LyricsEditWindow() override;

    /** Set by the owner: opens the lyrics guide at a topic. */
    std::function<void (gen::lyricsguide::Topic)> onShowGuide;

    /** For the --screenshots developer flag: shows a made-up answer without a model. `state` is "rewrite", "song", "rhymes"
        or "noOllama"; the selection is line 3 of the first verse. */
    void showCanned (const juce::String& state);

    /** Set by the owner: closes this window (Apply and Cancel call it). */
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;

private:
    class TextBox;
    class ResultsView;
    struct Chip;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void apply();
    void close();
    void setPanelOpen (bool open);
    void layoutPanel (juce::Rectangle<int> area);

    // The brief and the asks.
    void briefEdited();
    void loadBriefFromEngine();
    gen::lyricsai::Asks getAsks() const;
    void saveAsks();

    // Models and requests.
    juce::String getServerUrl() const;
    void refreshModels();
    void setStatus (const juce::String& message, bool isError = false);
    void startRequest (gen::lyricsai::Kind, const juce::String& addHeading = {}, int addBars = 8);
    void launch (gen::lyricsai::Request);
    void runAgain();
    void cancelRequest();
    void deliver (int id, gen::lyricsai::Result result, juce::String chatError, gen::rhyme::RhymeList dictionary);
    void showResult (const juce::String& chatError);
    void selectResult (int row, int chip);
    void useSelected();
    void applyText (const gen::lyricsai::Applied&, const juce::String& replacement);
    void updateUseState();
    void updateActionLabels();
    juce::String getWordAtCaret() const;

    void showSectionMenu();
    void addPopupItems (juce::PopupMenu&);
    void performPopupAction (int id);

    Engine& engine;
    const int startBar;
    const juce::String openedText;

    std::unique_ptr<TextBox> text;
    juce::Label hint, outsideNote;
    juce::ToggleButton keepTiming;
    juce::TextButton applyButton { "Apply" }, cancelButton { "Cancel" };

    // The panel.
    bool panelOpen = true;
    juce::TextButton foldButton;
    GuideButton helpButton { "?" };
    juce::Label modelLabel, aboutLabel, moodLabel, viewLabel, asksLabel, wordsLabel;
    juce::ComboBox modelBox, moodBox, viewBox;
    juce::TextButton reloadButton { juce::String::fromUTF8 ("\xe2\x86\xbb") };
    juce::TextEditor aboutEditor, wordsEditor;
    juce::TextButton songButton { "Write the song" }, sectionButton { "Section" }, selectionButton { "Rewrite selection" }, rhymesButton { "Rhymes" };
    std::vector<std::unique_ptr<juce::TextButton>> askButtons;

    juce::Label resultHeading, statusLabel;
    juce::TextButton useButton { "Use" }, againButton { "Try again" }, stopButton { "Cancel" };
    GuideButton resultHelp { "?" };
    juce::Viewport resultsViewport;
    std::unique_ptr<ResultsView> results;

    // Models and the request in flight.
    bool modelsLoaded = false;
    int modelRefreshId = 0;
    juce::String modelError;
    int requestId = 0;
    bool running = false;
    std::shared_ptr<std::atomic<bool>> stopFlag;
    std::shared_ptr<std::atomic<int>> receivedChars;
    int spinnerStep = 0;

    // The last request, kept for Use and Try again.
    gen::lyricsai::Request lastRequest;
    gen::lyricsai::Context lastContext;
    gen::lyricsai::Result lastResult;
    std::vector<gen::rhyme::DictionaryRhyme> lastDictionary;
    bool lastGuessed = false, haveResult = false, haveLastRequest = false, staleShown = false;
    int selectedRow = -1, selectedChip = -1;
    bool refreshingBrief = false;

    static constexpr int panelWidth = 360;
};
} // namespace daw
