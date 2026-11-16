#pragma once

#include "ExerciseFix.h"
#include "FirstSong.h"
#include "engine/Engine.h"
#include "engine/MidiQuantise.h"

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace daw::gen
{
/** The "Build a beat" exercise, in four steps on a drum track of the user's own song: kick and snare, hi-hats, velocity so
    it grooves, and swing. UI-free: the texts, the choice of the drum clip (the beat clip), the General MIDI drum roles, the
    live "is it done" rules that read the notes really in the clip, the statuses, tips and done notes, and the Quantise
    settings the last step opens. Nothing here changes the song: the user's own actions do.

    Step numbers (0 first): 0 kick and snare, 1 hi-hats, 2 make it groove, 3 add swing. */
namespace buildbeat
{
    constexpr int numSteps = 4;

    /** The step numbers. */
    constexpr int stepKickSnare = 0, stepHats = 1, stepGroove = 2, stepSwing = 3;

    using Fix = exercises::Fix;

    /** The app-settings keys' stem: the key is `learn.buildBeat.done`. */
    inline const char* const settingsStem = "learn.buildBeat";

    /** The window's height in pixels (to be fixed by a screenshot at the longest step). */
    constexpr int windowHeight = 540;

    // DawIt's own thresholds (not standards, and no text states them as rules).
    constexpr int minHats = 4;                 // hi-hat or ride notes in a bar for step 2
    constexpr int minVelocitySteps = 16;       // of 127: one drum's loudest and softest hit in a bar for step 3
    constexpr double startSwing = 0.58;        // the swing Quantise opens with in step 4
    constexpr double dottedLimit = 0.72;       // a pair's second note later than this is a dotted rhythm, not swing

    /** One step: heading, one-line summary, why and one or two guide pages. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();             // "Build a beat"
    juce::String getIntro();
    juce::String getHelpDescription();   // the Help menu item's one-line description
    juce::String getCompletionText();

    /** The drum track's header menu item's tooltip. */
    juce::String getTrackMenuTooltip();

    /** A step number read from a setting: anything outside the four steps is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** What a General MIDI drum note does in the checks: kick 35 and 36; snare 37 (side stick), 38, 39 (clap) and 40; hi-hat
        42, 44 and 46, and the ride 51, which keeps time the same way. Other notes play no role. */
    enum class Role { none, kick, snare, hat };

    Role getRole (int pitch) noexcept;

    /** "Kick", "Side stick", "Snare", "Clap", "Hi-hat" or "Ride" for those notes, else an empty string. */
    juce::String getDrumName (int pitch);

    //==============================================================================
    /** What the exercise needs from the app besides the song. */
    struct Progress
    {
        const Track* selectedTrack = nullptr;   // the selected track and clip, only compared with the engine's tracks and clips
        const Clip* selectedClip = nullptr;     // before use (they may have been deleted since)
        bool hasOutputDevice = true;
        double quantiseGridBeats = 0.0;         // the grid the Quantise dialog is on (open) or was last used with; 0 when unknown
    };

    /** The beat clip and its track. `track` is null when the song has no drum track; `clip` is null when the drum track has no
        MIDI clip yet. */
    struct Beat
    {
        const Track* track = nullptr;
        const MidiClip* clip = nullptr;
    };

    /** Whether the song has a drum track (the Drums or Percussion sound, or DawIt Drums). */
    bool hasDrumTrack (const Engine&);

    /** Finds the beat clip: the selected MIDI clip if it is on a drum track; otherwise the first clip with notes on the
        selected drum track, then on the first drum track that has one, and then the first clip of the selected (else the first)
        drum track, even when it is empty. */
    Beat findBeat (const Engine&, const Progress&);

    //==============================================================================
    /** One drum hit as the checks see it: where it starts in its bar (beats from the bar's start), and its velocity (0 to 127). */
    struct Hit
    {
        int pitch = 36;
        double offset = 0.0;
        int velocity = 100;
    };

    /** The notes of `clip` that start inside it, grouped by the song's bars: one entry per bar that has notes, in order. */
    struct Bar
    {
        int index = 0;               // 0 for the first bar of the song (shown as 1)
        std::vector<Hit> hits;
    };

    std::vector<Bar> getBars (const Engine&, const MidiClip&);

    /** Whether a bar has a note of this role. */
    bool hasRole (const Bar&, Role);

    /** How many hi-hat or ride notes a bar has. */
    int countHats (const Bar&);

    /** The groove bar: the first bar that has a kick and a snare; null when none has. */
    const Bar* findGrooveBar (const std::vector<Bar>&);

    /** Step 1: some bar of the beat clip has a kick and a snare. */
    bool isKickAndSnare (const std::vector<Bar>&);

    /** Step 2: the groove bar, else any bar with a kick and a snare, has `minHats` or more hi-hat or ride notes. */
    bool hasHats (const std::vector<Bar>&);

    /** One drum's loudest and softest hit in one bar, for step 3. */
    struct Spread
    {
        int pitch = 42;
        int lowest = 0, highest = 0;
    };

    /** The kick, snare or hi-hat of some bar whose hits differ by `minVelocitySteps` or more (the widest first); none when no
        drum does. Rounded `velocity x 127`. */
    std::optional<Spread> findVelocitySpread (const std::vector<Bar>&);

    /** Swing found in a bar by `quantise::findSwing`, for a kick, snare or hi-hat: which drum, and what it found. */
    struct DrumSwing
    {
        int pitch = 42;
        quantise::SwingFound found;
    };

    /** The first bar's swing, the hi-hat first, then the snare, then the kick; none when no bar swings. */
    std::optional<DrumSwing> findDrumSwing (const std::vector<Bar>&);

    /** A dotted rhythm (a second note at about 75% of its pair): the mean place as a percentage, when 2 or more notes sit there. */
    std::optional<double> findDotted (const std::vector<Bar>&);

    /** Whether the hi-hats of the groove bar move in eighths (or slower): every start is a multiple of half a beat. */
    bool hatsMoveInEighths (const std::vector<Bar>&);

    bool isStepDone (int step, const Engine&, const Progress&);

    /** What to do (the plan's text; it names the notes). */
    juce::String getDoText (int step);

    /** The step's own action button, from the song: *Add drums* (no drum track), *Open the beat*, or *Quantise with swing...*. */
    struct Action
    {
        juce::String label;
        juce::String tooltip;
        Fix fix = Fix::none;    // the step's own action; Fix::audioSettings is never the action (it is the status's)
    };

    Action getAction (int step, const Engine&, const Progress&);

    /** What is missing for a step to be done, or a tip: a line (empty: nothing to say) and the way forward. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step, or empty, from the song: "Snare on 2 and 4: the backbeat", "8 hi-hats: eighth notes,
        1 & 2 & 3 & 4 &", "Hi-hat from 70 to 100", "Hi-hats swing their eighths at about 60%". */
    juce::String getDoneNote (int step, const Engine&, const Progress&);

    /** What *Quantise with swing...* opens the dialog with: the 1/8 grid when the groove bar's hi-hats move in eighths, else
        1/16, swing 58% and strength 100%. */
    quantise::Settings getQuantiseStart (const Engine&, const Progress&);

    /** The *Show me* pointer's sentence for a step. For the swing step it depends on whether the Quantise dialog is open (its
        Swing slider is pointed at) or not (the piano roll's *Quantise...* button is). */
    juce::String getShowMeText (int step, bool quantiseDialogOpen = false);
} // namespace buildbeat
} // namespace daw::gen
