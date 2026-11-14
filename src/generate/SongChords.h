#pragma once

#include "FirstSong.h"
#include "engine/Chord.h"
#include "engine/Engine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <set>
#include <vector>

namespace daw::gen
{
/** The "Learn the chords of a song" exercise: bring in a recording, let DawIt find its chords, play along with them
    and swap one. UI-free: the texts, the live "is it done" rules, the played-along counter, the swap suggestion and a
    made-up example recording for tests and screenshots. */
namespace songchords
{
    constexpr int numSteps = 4;

    /** How many bars a song clip must span, how many of them need a chord, and how many bars must be played along. */
    constexpr int barsNeeded = 4;

    /** What the way forward is when a step can't be done yet (the action button turns into it). */
    enum class Fix { none, importAudio, detectChords, audioSettings };

    /** One step: heading, one-line summary, why and one or two guide pages. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();   // "Learn the chords of a song"
    juce::String getIntro();
    juce::String getCompletionText();

    //==============================================================================
    /** The song clip: the longest unmuted audio clip in the edit (ties go to the earliest start), or null. Find it fresh
        each time: it is never kept. */
    const AudioClip* findSongClip (const Engine&);

    /** The clip's length in bars at the song's tempo. */
    double getClipBars (const Engine&, const AudioClip&);

    /** The whole bars inside the clip: `firstBar` to `endBar` (exclusive), bar 0 first. */
    struct Span
    {
        int firstBar = 0, endBar = 0;

        bool contains (int bar) const noexcept { return bar >= firstBar && bar < endBar; }
        int size() const noexcept { return endBar - firstBar; }
    };

    Span getSpan (const Engine&, const AudioClip&);

    /** How many bars of the span have a chord. */
    int countChords (const ChordMap&, Span);

    /** The clip's name for the texts ("your recording" when it has none). */
    juce::String getClipName (const AudioClip&);

    //==============================================================================
    /** Step 1: an unmuted audio clip spans at least 4 bars. */
    bool isStep1Done (const Engine&);

    /** Step 2: at least 4 bars inside the song clip's span have a chord. */
    bool isStep2Done (const Engine&);

    /** Step 3: the song played through 4 different bars with a chord while Play along was open. */
    bool isStep3Done (int playedAlongBars) noexcept;

    /** Step 4: a bar in the span had a chord in `baseline` and now has a different one (removing one doesn't count). */
    bool isStep4Done (const Engine&, const ChordMap& baseline);

    bool isStepDone (int step, const Engine&, const ChordMap& baseline, int playedAlongBars);

    /** A step number read from a setting: anything outside 0 to 3 is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** Counts the different bars of the song the user has heard with a chord while Play along was open. */
    class PlayedAlongCounter
    {
    public:
        /** Feeds the bar under the playhead. Counts it only when it has a chord, the transport is playing and Play along
            is showing; the same bar twice counts once. */
        void feed (int bar, bool hasChord, bool playing, bool windowOpen)
        {
            if (hasChord && playing && windowOpen)
                bars.insert (bar);
        }

        int getCount() const noexcept { return (int) bars.size(); }
        void reset() { bars.clear(); }

    private:
        std::set<int> bars;
    };

    //==============================================================================
    /** A suggested swap: the chord in `bar` (0 first) replaced by its relative, and the notes they share. */
    struct Swap
    {
        bool valid = false;
        int bar = 0;
        Chord from, to;
        juce::Array<int> shared;   // pitch classes in both triads, in the order of `from`

        bool isMajor() const { return from.intervals.contains (4); }
        juce::String getSharedText() const;   // "C and E"
    };

    /** The relative of `chord`: the minor triad 3 semitones lower for a major chord, the major triad 3 semitones higher
        for a minor one (C to Am, G7 to Em, Am to C). Invalid for other chords (sus, diminished, augmented). */
    Swap getSwapFor (int bar, const Chord& chord);

    /** The swap suggested for the song: the first chord in the span, taken from `baseline` when it has one there (so the
        suggestion stays put once the user has swapped), else from the current chords. */
    Swap getSwap (const Engine&, const ChordMap& baseline);

    //==============================================================================
    /** What to do in step `step`, with the song clip's name and the swap. Out-of-range steps give an empty string. */
    juce::String getDoText (int step, const juce::String& clipName, const Swap&);

    /** The step's "why": step 4's depends on the chord (relative minor or major). */
    juce::String getWhy (int step, const Swap&);

    /** The action button text and tooltip of a step, with its normal action. */
    juce::String getActionLabel (int step);
    juce::String getActionTooltip (int step, const juce::String& clipName, const Swap&);

    /** What stops a step being done now: a line for the status area and what the button should become. Empty text and
        `Fix::none`: nothing to say. `playedAlongBars` is the counter, `hasOutputDevice` whether audio can play. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const ChordMap& baseline, int playedAlongBars, bool hasOutputDevice);

    /** The line under a ticked step ("Song is 32 bars long at the song's tempo."), or empty. */
    juce::String getDoneNote (int step, const Engine&, const ChordMap& baseline);

    /** A made-up recording of the example song: C G Am F twice, a bar each at `bpm` in 4/4 (8 bars; 16 s at 120 BPM),
        mono. Each chord's notes sound around middle C with four overtones plus the root two octaves lower, with 25 ms fades. */
    juce::AudioBuffer<float> makeExampleRecording (double sampleRate, double bpm);
} // namespace songchords
} // namespace daw::gen
