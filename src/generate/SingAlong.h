#pragma once

#include "engine/Lyrics.h"
#include "PlayAlong.h"

namespace daw::gen
{
/** Singing along with the lyrics track while recording vocals: which line is sung at a moment of
    the song, how much of each syllable has been sung (the colour that runs over the words), the
    count-in before a line that comes after a rest, and tips for singing along with a look at the
    song's own lyrics. */
namespace singalong
{
    /** What the sing-along window shows at a moment of the song. Lines are indices into the lyrics. */
    struct Moment
    {
        int line = -1;              // the line sung now, or between lines the next one; -1 when no line comes
        bool singing = false;       // the playhead is between the line's first syllable and its end
        double beatsToLine = 0.0;   // until `line` starts, while it isn't sung yet
        int previous = -1;          // the line sung before `line`
        int next = -1;              // the line after `line`
        int syllable = -1;          // the syllable of `line` sung now; -1 when none is
    };

    /** What's sung at `beat` (a song position in beats). While the playhead is inside the loop range,
        only the lines that start in it count, and after its last line the first one comes again,
        as the transport plays them. */
    Moment findMoment (const Lyrics&, double beat, const playalong::Loop& = {});

    /** The colour runs over a syllable in at most this many beats, so one sung before a rest fills
        while it's sung, not over the rest. */
    inline constexpr double maxFillBeats = 2.0;

    /** How much of a syllable has been sung at `beat`, from 0 to 1: it fills from its beat to where
        the next one starts (or the line ends), in at most maxFillBeats. */
    float getSung (const LyricLine&, size_t syllable, double beat);

    /** A line comes after a rest when at least this many beats go by without singing before it. */
    inline constexpr double restBeats = 2.0;

    /** The count-in before a line that comes after a rest (or starts the song): 4, 3, 2 or 1, the
        beats left (rounded up) over the last 4 beats before it. 0 while singing, further away, or
        when the line follows the one before closely. */
    int getCountIn (const Lyrics&, const Moment&);

    /** The line's words as they're written: the syllables of a word joined, a space between words. */
    juce::String getText (const LyricLine&);

    //==============================================================================
    /** The shortest rest between two lines, where a singer has the least time to breathe: before
        `line`, `beats` long. line is -1 when there are fewer than two lines. */
    struct Breath
    {
        int line = -1;
        double beats = 0.0;
    };

    Breath findShortestBreath (const Lyrics&);

    /** The line sung the longest without a rest, and how long in beats; -1 without lines. */
    std::pair<int, double> findLongestLine (const Lyrics&);

    /** For beginners: how to sing along, with tips from singing teachers, and a look at the song's
        own lyrics (how many lines, the longest one to sing in one breath, the shortest rest). */
    juce::String explainSingingAlong (const Lyrics&, double bpm);
} // namespace singalong
} // namespace daw::gen
