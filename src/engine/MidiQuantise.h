#pragma once

#include "Clip.h"

#include <juce_core/juce_core.h>

#include <vector>

/**
    Quantising: moving notes towards the nearest line of a grid, all the way or part of it, so a part played in by
    hand sits tighter and keeps as much of its feel as you want. Pure functions on a vector of notes: apply() takes
    the notes, the indices to change (empty means every note), where the clip starts in the song and how long it is,
    and the bar length, and returns new notes in the same order and the same count, so indices stay valid. Only notes
    starting before the clip's end count. The grid lines are each bar's start plus whole grid steps inside the bar.
*/
namespace daw::quantise
{
using Notes = std::vector<MidiNote>;

/** The indices of the notes to change; empty means all of them. */
using Selection = juce::SortedSet<int>;

/** One choice in the grid menu. */
struct Grid
{
    juce::String key;       // "1/16", "1/8t": what the settings file stores
    juce::String name;      // "1/16 (sixteenth note)": what the menu shows
    double beats = 0.25;    // the step in beats
};

/** The grids on offer: 1/4, 1/8, 1/16, 1/32 and the triplets 1/4, 1/8 and 1/16 (2/3, 1/3 and 1/6 of a beat). */
const std::vector<Grid>& getGrids();

/** The index of the grid with this settings key; the 1/16 grid's when the key is unknown. */
int findGrid (const juce::String& key);

/** The key of the grid whose step is `beats` ("1/16"), or an empty string when none is. */
juce::String getGridKey (double beats);

/** What a quantise does. */
struct Settings
{
    double gridBeats = 0.25;    // the step, in beats
    double strength = 1.0;      // 0 (nothing moves) to 1 (onto the line)
    bool ends = false;          // also move each note's end, so lengths change

    bool operator== (const Settings&) const = default;
};

/** Notes whose starts are less than this many beats after the first of them move together, as a chord. */
inline constexpr double chordWindowBeats = 0.02;

/** The notes after a quantise. Each start moves towards its nearest grid line (exactly halfway goes to the later
    line; a start within 1e-6 of a line stays exactly where it is) by `strength` of the way; notes of a chord take their
    first note's line. A line outside the clip becomes the nearest one inside it. Lengths, pitches and velocities stay
    (a note that would cross the clip's end is shortened to end at it); with `ends` the end moves too, to the nearest line
    but at least a grid step after the start's line. Quantising again at full strength changes nothing more. */
Notes apply (const Notes&, const Selection&, double clipStartBeat, double clipLength, double barLength, const Settings&);

/** What a quantise did, for the dialog's summary line. */
struct Summary
{
    int inScope = 0;                // notes the quantise looks at
    int moved = 0;                  // of those, how many changed
    double largestMove = 0.0;       // beats: the biggest change of a start or an end
    int collidingNotes = 0;         // notes of the same pitch that now start together but didn't before
};

/** Compares the notes before and after apply(). */
Summary summarise (const Notes& before, const Notes& after, const Selection&, double clipLength);

/** The summary line: "5 of 24 notes move, by up to 0.10 beats (50 ms at 120 BPM)." or why nothing moves, with a
    warning added when notes of the same pitch now start together. */
juce::String describe (const Summary&, const Settings&, double bpm);

/** True when the two lists hold the same notes, exactly. */
bool sameNotes (const Notes&, const Notes&);
} // namespace daw::quantise
