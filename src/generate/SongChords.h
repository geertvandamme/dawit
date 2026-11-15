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
/** The "Learn the chords of a song" exercise, in six steps: bring in a recording, find its tempo, its chords and its
    sections, play along with the chords and swap one. UI-free: the texts, the live "is it done" rules, the tempo
    record, the played-along counter, the swap suggestion and made-up example recordings for tests and screenshots.

    Step numbers (0 first): 0 bring in, 1 tempo, 2 chords, 3 sections, 4 play along, 5 swap. */
namespace songchords
{
    constexpr int numSteps = 6;

    /** The step numbers. */
    constexpr int stepImport = 0, stepTempo = 1, stepChords = 2, stepSections = 3, stepPlayAlong = 4, stepSwap = 5;

    /** How many bars a song clip must span, how many of them need a chord, and how many bars must be played along. */
    constexpr int barsNeeded = 4;

    /** What the way forward is when a step can't be done yet (the action button turns into it), and what a step's own
        action is for the tempo and sections steps (`detectTempo`, `detectSections`). */
    enum class Fix { none, importAudio, detectTempo, detectChords, detectSections, audioSettings };

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
    juce::String getHelpDescription();   // the Help menu item's one-line description
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
    /** The song's tempo as DawIt last set or confirmed it for the song clip (in memory, kept by the owner, cleared only
        when another song opens). Step 2 is done while the song clip is that clip, its start beat is the recorded one and
        the song's tempo map is unchanged. `noClearBeat` records that Detect tempo gave up on the clip. */
    struct TempoRecord
    {
        const AudioClip* clip = nullptr;   // identity only: never dereferenced unless it is the current song clip
        double startBeat = 0.0;
        TempoMap tempoMap { 120.0 };
        bool noClearBeat = false;

        bool isEmpty() const noexcept { return clip == nullptr; }
    };

    /** A record of the song clip and tempo map as they are now (Set tempo finished, or a result lined up). Empty when
        the engine has no song clip or `clip` is not it. */
    TempoRecord makeTempoRecord (const Engine&, const AudioClip& clip);

    /** A record that Detect tempo found no clear beat in `clip`. */
    TempoRecord makeNoBeatRecord (const Engine&, const AudioClip& clip);

    /** What the exercise knows beyond the song itself: the chords at the start, the played-along bars, whether audio
        can play, and the tempo record. */
    struct Progress
    {
        ChordMap baseline;
        int playedAlongBars = 0;
        bool hasOutputDevice = true;
        TempoRecord tempo;

        Progress (ChordMap chords = {}, int playedAlong = 0, bool output = true, TempoRecord record = {})
            : baseline (std::move (chords)), playedAlongBars (playedAlong), hasOutputDevice (output), tempo (std::move (record)) {}
    };

    /** How many bars of `span` the sections cover. */
    int countSectionBars (const SectionList&, Span);

    /** Step 1: an unmuted audio clip spans at least 4 bars. */
    bool isStep1Done (const Engine&);

    /** Step 2: the tempo record is for the song clip and matches its start and the song's tempo map. */
    bool isStepTempoDone (const Engine&, const TempoRecord&);

    /** Step 3: at least 4 bars inside the song clip's span have a chord. */
    bool isStepChordsDone (const Engine&);

    /** Step 4: the sections cover at least 4 bars inside the song clip's span. */
    bool isStepSectionsDone (const Engine&);

    /** Step 5: the song played through 4 different bars with a chord while Play along was open. */
    bool isStepPlayAlongDone (int playedAlongBars) noexcept;

    /** Step 6: a bar in the span had a chord in `baseline` and now has a different one (removing one doesn't count). */
    bool isStepSwapDone (const Engine&, const ChordMap& baseline);

    bool isStepDone (int step, const Engine&, const Progress&);

    /** A step number read from a setting: anything outside 0 to 5 is 0. */
    int clampStep (int stored) noexcept;

    /** The version of the step numbering saved beside the step: 2 now (absent or older: the four-step version). */
    constexpr int stepsVersion = 2;

    /** A saved step in the numbering of `version`: in the four-step version 0, 1, 2, 3 are steps 0, 2, 4, 5 now (anything
        else is 0); in the current version 0 to 5 are kept (anything else is 0). */
    int migrateStep (int stored, int version) noexcept;

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

    /** The step's "why": the swap step's depends on the chord (relative minor or major). */
    juce::String getWhy (int step, const Swap&);

    /** The action button text and tooltip of a step, with its normal action. */
    juce::String getActionLabel (int step);
    juce::String getActionTooltip (int step, const juce::String& clipName, const Swap&);

    /** What stops a step being done now: a line for the status area and what the button should become. Empty text and
        `Fix::none`: nothing to say. `Progress` holds the counter, whether audio can play and the tempo record. Steps 3 and 4 add a tip
        to find the tempo first while step 2 isn't done. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step ("Song is 32 bars long at the song's tempo."), or empty. */
    juce::String getDoneNote (int step, const Engine&, const Progress&);

    /** A made-up recording of the example song: C G Am F twice, a bar each at `bpm` in 4/4 (8 bars; 16 s at 120 BPM),
        mono. Each chord's notes sound around middle C with four overtones plus the root two octaves lower, with 25 ms fades. */
    juce::AudioBuffer<float> makeExampleRecording (double sampleRate, double bpm);

    /** The example song with a beat, for the tempo and structure steps: 16 bars of 4/4 at `bpm`, a verse (C G Am F) and a
        chorus (F G C G) of 4 bars each, verse chorus verse chorus, the verse 10 dB softer, with a short low kick on beats
        1 and 3 and a short high tick on every beat all through (40 s at 96 BPM), mono. */
    juce::AudioBuffer<float> makeExampleSongWithBeat (double sampleRate, double bpm);
} // namespace songchords
} // namespace daw::gen
