#pragma once

#include "engine/Engine.h"
#include "generate/Exercises.h"
#include "GuideButton.h"
#include "Selection.h"
#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
/**
    The window of a guided exercise (Help > First Song Exercise..., Learn the Chords of a Song...): four short steps in
    the user's own song, with a tick for each that follows the song live. A step shows what to do and why, a button that
    does it (or, when something is missing, the way forward), *Show me* (points at the control), one or two links to
    guide pages and, when something is missing, a line saying what. A step being done never moves the user on by itself;
    when all are done together the list gives way to the completion text. The step and the finished flag are remembered
    in the app settings under the exercise's key stem. One window class runs any `gen::exercises::Exercise`.
    Message thread only; what the buttons do is up to `MainComponent`.
*/
class ExerciseWindow final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    /** `getContext` gives the app facts the song doesn't hold (saved state, chord baseline, played-along bars, output
        device); the owner keeps them, so a closed and reopened window keeps its ticks. */
    ExerciseWindow (Engine&, Selection&, gen::exercises::Exercise, std::function<gen::exercises::Context()> getContext);
    ~ExerciseWindow() override;

    /** The step's action button was pressed; `fix` says which way forward it is (`none`: the step's own action). */
    std::function<void (int step, gen::exercises::Fix fix)> onAction;

    /** *Show me* was pressed on a step: point at the control it teaches. */
    std::function<void (int step)> onShowMe;

    /** A guide link was clicked. */
    std::function<void (const gen::exercises::Link&)> onOpenLink;

    /** The completion text's first button (the exercise's carry-on). */
    std::function<void()> onCarryOn;

    /** *Start over* was pressed: the owner resets its baselines. */
    std::function<void()> onStartOver;

    /** Esc: close the window. */
    std::function<void()> onClose;

    gen::exercises::Exercise getExercise() const noexcept { return exercise; }

    /** How many steps are ticked now. */
    int getDoneCount() const noexcept;

    /** The step shown open, and whether the completion text is shown. */
    int getStep() const noexcept { return step; }
    bool isFinished() const noexcept { return finished; }

    /** The action button of the open step, for the screenshots. */
    juce::TextButton& getActionButton() noexcept;

    /** Opens a step, as clicking its row does. */
    void setStep (int newStep);

    /** Brings the ticks, the texts and the buttons up to date with the song now, as the timer does twice a second. */
    void refresh();

    /** Goes back to step 1 and clears the finished flag; the song is untouched. */
    void startOver();

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int windowWidth = 360, windowHeight = 500, tallWindowHeight = 680;

private:
    class StepRow;
    class Body;
    /** What the open step's body shows. */
    struct BodyContent
    {
        juce::String subtitle, doText, why, status, doneNote;
        juce::String actionLabel, actionTip;
        bool actionEnabled = true, hasShowMe = true, statusDim = false;
        juce::StringArray linkTexts;   // one chip each (none, one or two)
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override { refresh(); }

    gen::exercises::Context getContext() const;
    bool isDone (int index) const;
    void saveSettings();

    Engine& engine;
    Selection& selection;
    gen::exercises::Exercise exercise;
    std::function<gen::exercises::Context()> contextProvider;
    gen::exercises::Info info;
    int numSteps = 0;

    int step = 0;
    bool finished = false;
    std::vector<bool> done;
    gen::exercises::Fix currentFix = gen::exercises::Fix::none;

    std::vector<std::unique_ptr<StepRow>> rows;
    juce::Viewport viewport;
    std::unique_ptr<Body> body;
    juce::TextButton backButton { "Back" }, skipButton { "Skip" }, nextButton { "Next" };
    juce::TextButton carryOnButton, startOverButton { "Start over" };
    juce::TextLayout introLayout, finishedLayout;
    juce::Rectangle<int> introArea, finishedArea, countArea;
    juce::String bodySignature;
    BodyContent bodyContent;
};
} // namespace daw
