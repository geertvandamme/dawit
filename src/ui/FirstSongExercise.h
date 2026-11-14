#pragma once

#include "engine/Engine.h"
#include "generate/FirstSong.h"
#include "GuideButton.h"
#include "Selection.h"
#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
/**
    The First song exercise window (Help > First Song Exercise...): four short steps in the user's own song, with a
    tick for each that follows the song live. A step shows what to do and why, a button that does it, *Show me*
    (points at the control) and a link to the guide page. A step being done never moves the user on by itself;
    when all four are done together the list gives way to the completion text. The step and the finished flag are
    remembered in the app settings. Message thread only; what the buttons do is up to `MainComponent`.
*/
class FirstSongExercise final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    /** `baseline` is the chord track as it was when the exercise began (step 2 is done once the chords differ from it);
        it belongs to the owner so a closed and reopened window keeps its ticks. */
    FirstSongExercise (Engine&, Selection&, ChordMap& baseline);
    ~FirstSongExercise() override;

    /** The step's action button was pressed (0 to 3). */
    std::function<void (int step)> onAction;

    /** *Show me* was pressed on a step: point at the control it teaches. */
    std::function<void (int step)> onShowMe;

    /** A guide link was clicked. */
    std::function<void (const gen::firstsong::Link&)> onOpenLink;

    /** *Carry on with Start here* on the completion text. */
    std::function<void()> onCarryOn;

    /** *Start over* was pressed: the owner resets the chord baseline. */
    std::function<void()> onStartOver;

    /** Esc: close the window. */
    std::function<void()> onClose;

    /** The saved state of the project, for step 4. */
    std::function<gen::firstsong::SavedState()> getSavedState;

    /** How many steps are ticked now (0 to 4). */
    int getDoneCount() const noexcept;

    /** The step shown open (0 to 3), and whether the completion text is shown. */
    int getStep() const noexcept { return step; }
    bool isFinished() const noexcept { return finished; }

    /** The action button of the open step, for the screenshots. */
    juce::TextButton& getActionButton() noexcept;

    /** Opens a step (0 to 3), as clicking its row does. */
    void setStep (int newStep);

    /** Brings the ticks, the texts and the buttons up to date with the song now, as the timer does twice a second. */
    void refresh();

    /** Goes back to step 1 and clears the finished flag; the song is untouched. */
    void startOver();

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int windowWidth = 360, windowHeight = 500;

private:
    class StepRow;
    class Body;
    /** What the open step's body shows. */
    struct BodyContent
    {
        juce::String subtitle, doText, why, status;
        juce::String actionLabel, actionTip;
        bool actionEnabled = true, hasShowMe = true, hasLink = true;
        juce::String linkText;
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override { refresh(); }

    gen::firstsong::SavedState getSaved() const;
    bool isDone (int index) const;
    juce::String getMidiTrackName() const;
    bool hasDrumTrack() const;
    void saveSettings();

    Engine& engine;
    Selection& selection;
    ChordMap& baseline;

    int step = 0;
    bool finished = false;
    bool done[gen::firstsong::numSteps] {};

    std::unique_ptr<StepRow> rows[gen::firstsong::numSteps];
    juce::Viewport viewport;
    std::unique_ptr<Body> body;
    juce::TextButton backButton { "Back" }, skipButton { "Skip" }, nextButton { "Next" };
    juce::TextButton carryOnButton { "Carry on with Start here" }, startOverButton { "Start over" };
    juce::TextLayout introLayout, finishedLayout;
    juce::Rectangle<int> introArea, finishedArea, countArea;
    juce::String bodySignature;
    BodyContent bodyContent;
};
} // namespace daw
