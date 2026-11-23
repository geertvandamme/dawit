#pragma once

#include "engine/Engine.h"
#include "generate/AiSetup.h"
#include "generate/SoundDescribe.h"
#include "SubtractiveEditor.h"

namespace daw
{
/**
    The bar of "Describe a sound" in DawIt Subtractive's window (the app only). The user types what they want to
    hear; a local model through Ollama suggests the knobs to change, each with a reason. DawIt reads and clamps every
    value (`gen::sounddescribe`), shows the answer in the explainer, and then either walks through the changes one
    knob at a time (the editor rings the knob, and the step is done when the user turns it close enough) or sets them
    all at once. Nothing changes until the user chooses. Undo takes back the last answer or step for as long as the
    window is open; the app's own undo doesn't cover synth knobs. The request runs on a background thread with copies
    of everything it needs; an answer that arrives after a newer request or after the window closed is dropped.
*/
class SubtractiveDescribeBar final : public DescribeBarBase
{
public:
    SubtractiveDescribeBar (BuiltInSubtractiveEditor&, Engine&);
    ~SubtractiveDescribeBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    void tick() override;
    void opened() override;
    void closed() override;

    //==============================================================================
    // For the screenshot runner and tests: they drive the bar without Ollama.

    /** Types words into the text box. */
    void setWords (const juce::String&);

    /** Reads a canned reply as a model's reply would be read, and shows the answer. `followUp` as the switch says. */
    bool showCannedReply (const juce::String& reply, bool followUp);

    /** A canned Ollama state: unreachable, noModels or notInstalled. False for another name. */
    bool setCannedSetup (const juce::String&);

    /** Opens the Set up... steps in the explainer. */
    void showSetup (bool);

    /** Starts the walkthrough of the last answer and goes to step `step` (0 up) by setting each one before it. */
    bool startWalkthrough (int step);

    /** Sets every change of the last answer at once, as the button does. */
    void setAllNow();

private:
    enum class Phase { idle, waiting, answered, walking, finished };

    class StatusView;

    juce::PropertiesFile& getSettings() const;
    juce::String getServerUrl() const;
    juce::String getModel() const;
    juce::StringArray getRecent() const;
    void addRecent (const juce::String&);
    void showRecentMenu();

    void refreshModels();
    void applyModelList (const gen::OllamaClient::ModelList&);
    void updateSetupInfo();

    void ask (bool again);
    void cancel();
    void deliver (const gen::sounddescribe::Result&, const gen::sounddescribe::Request&);
    void showAnswer();

    void startWalk();
    void updateStep();
    void endWalk();
    void stepSet();
    void stepSkip();
    void stepBack();
    void stepFinishAll();
    void undo();
    int countMade() const;

    void updateControls();
    void setStatus (const juce::String& text, bool isError = false);
    void showMenu();
    bool isReady() const noexcept { return setupInfo.ready; }

    BuiltInSubtractiveEditor& editor;
    BuiltInSubtractive& synth;
    Engine& engine;

    juce::TextEditor text;
    juce::TextButton recentButton { "v" }, startButton, askButton { "Ask" }, closeButton { "x" };
    juce::ComboBox modelBox;
    std::unique_ptr<StatusView> status;
    juce::TextButton walkButton { "Walk me through it" }, allButton { "Set it all now" }, againButton { "Ask again" },
                     undoButton { "Undo" }, retryButton { "Retry" }, setupButton { "Set up..." },
                     backButton { "Back" }, skipButton { "Skip" }, setForMeButton { "Set it for me" }, finishButton { "Finish all" };

    Phase phase = Phase::idle;
    bool startFromCurrent = false;
    bool setupShown = false;
    juce::String statusText, cannedModel;
    bool statusIsError = false;

    gen::OllamaClient::ModelList modelList;
    bool haveModelList = false, refreshing = false;
    gen::aisetup::SetupInfo setupInfo;
    int modelRefreshId = 0;

    int requestId = 0;
    std::shared_ptr<std::atomic<bool>> stopFlag;
    gen::sounddescribe::Request lastRequest;

    std::unique_ptr<gen::sounddescribe::Walkthrough> walk;
    std::vector<char> stepMade;          // per step: set by the user or for them
    gen::sounddescribe::SettingsUndo undoList;
    juce::int64 reachedSince = 0;        // when the current step's knob got close enough
    int reachedIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubtractiveDescribeBar)
};
} // namespace daw
