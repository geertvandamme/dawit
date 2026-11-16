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
    double swing = 0.5;         // 0.5 (straight) to 0.75: the share of each pair of lines the first takes (see `isSwingGrid`)

    bool operator== (const Settings&) const = default;
};

/** The swing scale of the MPC, Logic and DawIt Arpeggiator: 50% is straight, 66% a triplet feel, 75% a dotted rhythm. */
inline constexpr double minSwing = 0.5, maxSwing = 0.75;

/** Whether swing applies to a grid: only the 1/8 (0.5 beats) and 1/16 (0.25 beats) grids. A triplet grid already swings and
    the 1/4 and 1/32 grids have no pairs worth swinging. */
bool isSwingGrid (double gridBeats);

/** Whether `settings` swing the lines: a swing grid and a swing above 50%. */
bool isSwinging (const Settings&);

/** Notes whose starts are less than this many beats after the first of them move together, as a chord. */
inline constexpr double chordWindowBeats = 0.02;

/** The notes after a quantise. Each start moves towards its nearest grid line (exactly halfway goes to the later
    line; a start within 1e-6 of a line stays exactly where it is) by `strength` of the way; notes of a chord take their
    first note's line. A line outside the clip becomes the nearest one inside it. Lengths, pitches and velocities stay
    (a note that would cross the clip's end is shortened to end at it); with `ends` the end moves too, to the nearest line
    but at least a grid step after the start's line. Quantising again at full strength changes nothing more.

    Swing (`settings.swing` above 0.5, on the 1/8 and 1/16 grids only): in every bar, counted from its start, each second
    line moves later, from k x grid to (k - 1) x grid + 2 x swing x grid, so the first line of each pair stays and the pair
    splits swing : (1 - swing). Notes go to the nearest swung line, and strength applies as before. A bar with an odd number of
    lines (5/8) leaves its last line straight. */
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
    warning added when notes of the same pitch now start together. A swinging quantise adds "with swing 60%". */
juce::String describe (const Summary&, const Settings&, double bpm);

/** How late the second note of each pair is at `swing` (0.5 to 0.75), in milliseconds: (swing - 0.5) x the pair's length in
    beats x the length of a beat. Sixteenths (a pair of 0.5 beats) at 60% and 120 BPM: 25 ms; eighths (1 beat) at 66%: 80 ms. */
double getSwingDelayMs (double swing, double pairBeats, double bpm);

/** Where one note start falls in its pair (see `countSwing`). */
enum class SwingPlace { onPair, swung, straight, other };

/** Classifies a note start, in beats from the start of its bar, against pairs of `pairBeats` counted from the bar's start:
    within 0.02 beats of a pair's start is `onPair`; otherwise its place in the pair (0 to 1) is `swung` from 0.53 to 0.72,
    `straight` at 0.5 plus or minus 0.03, and `other` anywhere else. `place`, when given, receives that place. */
SwingPlace classifySwing (double startInBar, double pairBeats, double* place = nullptr);

/** What one drum's notes in one bar say about swing, for one size of pair. */
struct SwingCount
{
    int on = 0;           // starts within 0.02 beats of the start of a pair
    int swung = 0;        // starts at 0.53 to 0.72 of the pair
    int straight = 0;     // starts at 0.5 of the pair, plus or minus 0.03
    int other = 0;        // every other start, such as a dotted rhythm (0.75)
    double percent = 50.0;   // the mean place of the swung notes in their pair, as a percentage (50 when none is swung)
};

/** Counts note starts, in beats from the start of their bar, against pairs of `pairBeats` (1 for swung eighths, 0.5 for
    swung sixteenths) counted from the bar's start. 0.72 is the upper limit so that a dotted figure (0.75) is not swing. */
SwingCount countSwing (const std::vector<double>& startsInBar, double pairBeats);

/** The swing found in one bar of one drum, by the best-fitting pair size. */
struct SwingFound
{
    bool found = false;
    double pairBeats = 1.0;     // 1 (eighths) or 0.5 (sixteenths)
    double percent = 50.0;      // the mean swing, such as 60
    int swung = 0;              // how many notes are swung
};

/** Swing is found when 2 or more notes are swung and they outnumber the straight and other notes (the notes on the pair
    starts don't count against it). Tries pairs of 1 beat and 0.5 beats and takes the one with more swung notes (the eighths
    on a tie). Whether a dotted rhythm at 0.75 is "swing" is no: it is `other`. */
SwingFound findSwing (const std::vector<double>& startsInBar);

/** True when the two lists hold the same notes, exactly. */
bool sameNotes (const Notes&, const Notes&);
} // namespace daw::quantise
