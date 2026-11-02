#pragma once

#include "Clip.h"
#include "MusicalKey.h"

#include <juce_core/juce_core.h>

#include <cstdint>
#include <vector>

/**
    Turning a melody around: its notes backwards in time, upside down in pitch (exactly or in the key),
    or with new random pitches from the song's key over the same rhythm. Pure functions on a vector of
    notes: each takes the notes, the indices to change (empty means every note) and the clip's length in
    beats, and returns new notes in the same order and the same count, so indices stay valid and notes
    never move to another place in the list. Only notes starting before the clip's end count.
*/
namespace daw::miditransform
{
using Notes = std::vector<MidiNote>;

/** The indices of the notes to change; empty means all of them. */
using Selection = juce::SortedSet<int>;

enum class Kind { reverseInTime, invertInKey, invertExactly, randomise };

/** "Reverse in time", "Reverse in pitch, in the key", "Reverse in pitch, exactly" or "Randomise pitch": the
    menu item's words, also used in "Undo ...". */
juce::String getKindName (Kind);

/** How "Randomise pitch" chooses. */
struct RandomOptions
{
    int stayNear = 0;               // at most this many scale steps from the old note; 0 = anywhere in the range
    bool avoidRepeats = false;      // a note never equals the lowest note of the group before it
    bool chordsTogether = true;     // notes starting together move by the same number of scale steps

    bool operator== (const RandomOptions&) const = default;
};

/** Notes whose starts are less than this many beats apart are a chord. */
inline constexpr double chordWindowBeats = 0.02;

//==============================================================================
// The key

/** The pitch of the key nearest to `pitch` (itself when it's in the key); halfway goes to the lower one,
    as Ableton's Fit to Scale does. Stays within 0 to 127. */
int snapToKey (int pitch, const MusicalKey&);

/** The key's pitches from `lowest` to `highest` inclusive, lowest first. */
std::vector<int> getKeyPitches (const MusicalKey&, int lowest, int highest);

/** A pitch's scale-step number: how many notes of the key lie below it from MIDI note 0 (a pitch outside
    the key is snapped first). Two pitches are n scale steps apart when their numbers differ by n. */
int getScaleStep (int pitch, const MusicalKey&);

/** The pitch of a scale-step number, limited to the key's lowest and highest note in 0 to 127. */
int getPitchOfStep (int step, const MusicalKey&);

/** The range a randomise picks from: the notes' own, widened upwards to an octave (at most 127) when smaller. */
struct Range
{
    int lowest = 0, highest = 0;
};

Range getRandomRange (int lowestPitch, int highestPitch);

//==============================================================================
// The transforms

/** The indices a transform changes: the selection (all when empty) restricted to notes starting before `clipLength`. */
std::vector<int> getAffected (const Notes&, const Selection&, double clipLength);

/** How many different pitches the notes to change have. */
int countDistinctPitches (const Notes&, const Selection&, double clipLength);

/** The lowest and highest pitch of the notes to change (both 0 when there are none). */
Range getPitchRange (const Notes&, const Selection&, double clipLength);

/** Mirrors the notes in time: each note's end becomes its start. The whole clip mirrors in [0, length]; a
    selection in [its first start, its last end]. A note ringing past the clip's end is first shortened to end
    at it. Pitch and velocity stay. Done twice it gives the notes back. */
Notes reverseInTime (const Notes&, const Selection&, double clipLength);

/** Mirrors the pitches around the middle of their range: p becomes lowest + highest - p, so every step keeps
    its size in semitones and the lowest and highest swap. Unchanged with fewer than two different pitches. */
Notes invertExactly (const Notes&, const Selection&, double clipLength);

/** Mirrors the pitches by scale steps: a note outside the key first moves to the nearest note of it, and every
    step up becomes a step down of the same number of scale notes. Unchanged with fewer than two different pitches. */
Notes invertInKey (const Notes&, const Selection&, double clipLength, const MusicalKey&);

/** Gives each note a random pitch of the key within the notes' range (see getRandomRange), keeping its start,
    length and velocity. The same seed gives the same notes on every machine. */
Notes randomise (const Notes&, const Selection&, double clipLength, const MusicalKey&, const RandomOptions&, uint32_t seed);

/** Runs the transform of a kind (`seed` and `options` only matter for randomise). */
Notes apply (Kind, const Notes&, const Selection&, double clipLength, const MusicalKey&, const RandomOptions&, uint32_t seed);

/** True when the two lists hold the same notes, exactly. */
bool sameNotes (const Notes&, const Notes&);
} // namespace daw::miditransform
