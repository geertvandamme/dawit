#pragma once

#include "engine/Chord.h"
#include "engine/Section.h"

#include <vector>

namespace daw::gen
{
/** Playing along with the chord track: what the play-along window shows at a moment of the song
    (the chord now, the next one and how far away it is, the section and bar, the chords coming),
    a capo for guitar, and tips for playing along. */
namespace playalong
{
    /** A run of bars with the same chord, as the chord track draws it: one block. */
    struct Change
    {
        int startBar = 0, endBar = 0;   // endBar is the bar after it
        Chord chord;

        int getBars() const noexcept { return endBar - startBar; }
    };

    /** The chord track as runs of bars, in order. A chord repeated on the following bars is one
        run; the same chord after an empty bar is a new one. */
    std::vector<Change> getChanges (const ChordMap&);

    /** The loop range the transport plays round, in beats. */
    struct Loop
    {
        bool enabled = false;
        double startBeat = 0.0, endBeat = 0.0;
    };

    /** A chord coming up: which change and how many beats away it starts. */
    struct Coming
    {
        int change = -1;
        double inBeats = 0.0;
    };

    /** What to show at a moment of the song. Changes are indices into getChanges(). */
    struct Moment
    {
        int current = -1;          // the change playing now; -1 for a bar without a chord
        double beatsLeft = -1.0;   // until the current chord (or the silence) ends; -1 when nothing changes ahead
        double runBeats = 0.0;     // how long the current chord (or the silence) lasts in all, for the countdown

        int next = -1;             // the next chord to play; -1 when none comes
        double beatsToNext = -1.0;

        std::vector<Coming> coming;   // the chords after the current one, the next one first

        int bar = 0;               // the bar playing, 0 is the first
        int beatInBar = 0;         // 0 to 3

        int section = -1;          // index into the sections; -1 outside every section
        int barInSection = 0;      // 0 is the section's first bar
    };

    /** What plays at `beat` (a song position in beats), looking up to `numComing` chords ahead.
        While the playhead is inside the loop range, the chords after its end come from its start,
        as the transport plays them. */
    Moment findMoment (const std::vector<Change>&, const SectionList&, double beat, const Loop& = {}, int numComing = 8);

    /** When the next chord comes: "now", "in 1 beat", "in 3 beats", or in bars from two bars on:
        "in 2 bars". Rounded up, so a chord is never later than said. */
    juce::String describeWait (double beats);

    /** Where the song is: "Chorus 2 · bar 3 of 8", or "Bar 12" outside the sections. */
    juce::String describePosition (const Moment&, const SectionList&);

    /** Whether the next chord should light up: on the last beat before it. */
    bool isAboutToChange (const Moment&);

    //==============================================================================
    /** The chord whose shape is fingered with a capo on fret `capo`: the chord `capo` semitones
        lower. With a capo on fret 3, a G minor chord is played as an E minor shape. */
    Chord getShapeChord (const Chord&, int capo);

    /** "Capo 3: play the Em shape, it sounds as Gm", or empty without a capo. */
    juce::String describeCapo (const Chord&, int capo);

    /** The capo that lets the most of the song's different chords be played as open chords (in the
        first frets, without a barre); 0 when no capo does better than none. Looks at frets 0 to 7. */
    int findEasiestCapo (const std::vector<Change>&);

    //==============================================================================
    /** For beginners: how to play along with a song's chords, with tips from teachers, and a look
        at the song's own chord changes (how many different chords, the hardest one, the fastest
        change). */
    juce::String explainPlayingAlong (const std::vector<Change>&, double bpm);
} // namespace playalong
} // namespace daw::gen
