#pragma once

#include "ChordFinder.h"
#include "engine/Section.h"

namespace daw::analysis
{
/**
    Finding a recording's song structure (intro, verse, chorus, bridge...) on the song's bars, with
    plain signal processing, no model (Foote 2000, McFee and Ellis 2014):

    1. Each bar of the song described by its notes (the chroma the chord finder uses), its sound
       (the level in 16 bands from 50 Hz to 5 kHz, bright or dull), its loudness and how much of it
       is short sounds (drums).
    2. Where a new part starts: where the four bars before a bar line differ most from the four
       after it (in chords, sound, loudness and drums), at lengths of 4, 8 or 16 bars where it can,
       because songs are built from phrases of 4 bars.
    3. Which parts are the same music: parts with the same chords in the same order and the same
       sound are one group (A, B, C...), heard again.
    4. Names from the groups: the group that comes back and is loudest is the chorus, the other one
       that comes back is the verse, a part just before each chorus is a pre-chorus, a part heard
       once after the second chorus is a bridge, and the first and last parts are the intro and outro.

    The bars are the song's, so the recording has to be at the song's tempo with its downbeats on
    the bar lines: Detect tempo does that.
*/
namespace structurefinder
{
    /** The analysis listens to mono audio at this rate, as the chord finder does. */
    constexpr double sampleRate = chordfinder::sampleRate;

    constexpr int numBands = 16;

    /** One bar of the recording, described. */
    struct Bar
    {
        int bar = 0;
        chordfinder::PitchClasses notes {};   // the level of each note name, unit length; zero when silent
        std::array<float, numBands> sound {}; // each band's level in dB against the bar's average band
        double loudness = -100.0;             // dB
        double drums = 0.0;                   // the share of short sounds, 0 to 1
        bool sounding = false;                // not silent (within 40 dB of the loudest bar)
    };

    /** A stretch of bars found to be one part, before it's named. */
    struct Segment
    {
        int startBar = 0, bars = 0;
        int group = 0;           // the same number is the same music: 0 is A, 1 is B...
        double loudness = 0.0;   // dB
    };

    /** A section as found, with what it's like and why it got its name. */
    struct FoundSection
    {
        Section section;
        int group = 0;
        int timesHeard = 1;      // how often its group's music is heard in the song
        double loudness = 0.0;   // dB
        juce::String reason;     // e.g. "heard 3 times, the loudest part that comes back"
    };

    /** The audio as mono at sampleRate. */
    juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>&, double sourceSampleRate);

    /** The song's bars that are at least half in the audio, from `startBeat`, through the song's
        tempo (and its changes). */
    std::vector<Bar> describeBars (const juce::AudioBuffer<float>& mono, double startBeat, const TempoMap&);

    /** For each bar, how much changes at its start: 0 (nothing) to 1 (everything). */
    std::vector<double> findNovelty (const std::vector<Bar>&);

    /** Where the parts start between bars `first` and `end` (indices into the novelty), the first one
        included: at strong changes, preferring parts of 4, 8 and 16 bars. */
    std::vector<int> findBoundaries (const std::vector<double>& novelty, int first, int end);

    /** How alike the music of two stretches of bars is, bar by bar from their starts: 0 to 1. */
    double similarity (const std::vector<Bar>&, int startA, int barsA, int startB, int barsB);

    /** Names the segments: neighbours of the same group join up, then each gets a type from how
        often it comes back, how loud it is and where it is, numbered when the type comes back
        (Verse 1, Verse 2). */
    std::vector<FoundSection> nameSections (const std::vector<Segment>&);

    /** The sections of mono audio at sampleRate that starts at `startBeat` of the song, through its
        tempo (and its changes). Silent bars at the start and end get no section. */
    std::vector<FoundSection> analyse (const juce::AudioBuffer<float>& mono, double startBeat, const TempoMap&);

    /** The same at one tempo all through. */
    inline std::vector<FoundSection> analyse (const juce::AudioBuffer<float>& mono, double startBeat, double bpm)
    {
        return analyse (mono, startBeat, TempoMap (bpm));
    }

    /** "A", "B"... for a group. */
    juce::String getGroupName (int group);

    /** The sections as text: "Intro (4) | Verse 1 (8) | Chorus 1 (8)". */
    juce::String toText (const std::vector<FoundSection>&);

    /** Which music comes back: "A B C B C D C E". */
    juce::String getPattern (const std::vector<FoundSection>&);
} // namespace structurefinder
} // namespace daw::analysis
