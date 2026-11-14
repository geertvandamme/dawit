#pragma once

#include "FirstSong.h"
#include "SongChords.h"

#include <juce_core/juce_core.h>

#include <vector>

namespace daw::gen
{
/** The guided exercises as one model: each has a title, intro, steps (with one or two guide links), a completion text and
    a settings stem, and answers, from the song and a small context of app facts, whether step n is done, what to do,
    what its action button says and what is missing. UI-free. First song's content stays in `gen::firstsong`, and the
    chord exercise's in `gen::songchords`; this namespace puts them behind one set of questions for the shared window. */
namespace exercises
{
    enum class Exercise { firstSong, songChords };

    constexpr Exercise allExercises[] { Exercise::firstSong, Exercise::songChords };
    constexpr int numExercises = 2;

    using Link = firstsong::Link;
    using Fix = songchords::Fix;

    /** One step, as the window shows it. `why` is the generic text: ask `getWhy` for the one that fits the song. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<Link> links;   // none, one or two guide pages
    };

    /** An exercise's fixed content. */
    struct Info
    {
        Exercise exercise = Exercise::firstSong;
        juce::String settingsStem;        // "learn.firstSong": the keys are <stem>.step and <stem>.done
        juce::String title;
        juce::String intro;
        juce::String completion;
        juce::String carryOnLabel;        // the completion's first button
        juce::String carryOnTooltip;
        bool carryOnOpensStartHere = false;   // else it opens `carryOnLink`
        Link carryOnLink;
        std::vector<Step> steps;
    };

    /** Facts about the app that the song itself doesn't hold. The owner (`MainComponent`) fills it in on each ask. */
    struct Context
    {
        firstsong::SavedState saved;       // First song step 4
        ChordMap baseline;                 // First song step 2 and the chord exercise's step 4: the chords at the start
        int playedAlongBars = 0;           // the chord exercise's step 3
        bool hasOutputDevice = true;       // the chord exercise's step 3
    };

    /** An action button: its text, tooltip and what it does (`Fix::none`: the step's own action). */
    struct Action
    {
        juce::String label;
        juce::String tooltip;
        Fix fix = Fix::none;
    };

    /** What is missing for a step to be done: a line (empty: nothing to say) and the way forward. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Info getInfo (Exercise);

    /** The app-settings keys that remember the step and "finished once". First song's are the old `learn.firstSong.*`. */
    juce::String getStepSettingKey (Exercise);
    juce::String getDoneSettingKey (Exercise);

    /** Every guide page the exercise's steps and completion link to (for a test). */
    std::vector<Link> getAllLinks (Exercise);

    /** A step number read from a setting: anything outside the exercise's steps is 0. */
    int clampStep (Exercise, int stored) noexcept;

    bool isStepDone (Exercise, int step, const Engine&, const Context&);
    juce::String getDoText (Exercise, int step, const Engine&, const Context&);
    juce::String getWhy (Exercise, int step, const Engine&, const Context&);
    Action getAction (Exercise, int step, const Engine&, const Context&);
    Status getStatus (Exercise, int step, const Engine&, const Context&);

    /** The note under a ticked step ("32 bars long"), or empty. */
    juce::String getDoneNote (Exercise, int step, const Engine&, const Context&);
} // namespace exercises
} // namespace daw::gen
