#pragma once

#include "SingPairing.h"

namespace daw::gen::singthis
{
/** Match the notes to the words: a copy of a melody with as many notes as the lyrics have syllables, so each note
    can sing one syllable. UI-free and tested. The user's own notes are never changed; this only makes a new list. */

constexpr double minSplitBeats = 0.125;   // a split never makes a note shorter than this (a 32nd note)
constexpr double minNoteBeats = 0.125;    // a note placed on a syllable is never shorter than this
constexpr double movedBeats = 0.25;       // a note start that lands further than this (a 16th) from every melody note start has moved

constexpr double syllableGapBeats = 0.125;   // legato: a note stops this long before the next syllable starts
constexpr double lastSyllableMaxBeats = 2.0; // legato: the last syllable of a phrase lasts at most this long

/** How the notes are matched: on the syllables' times with notes that run on to the next syllable (legato), on the
    syllables' times with notes no longer than the melody's own (staccato), or by merging and splitting the melody's notes. */
enum class Method { syllableTimes, mergeSplit, syllableTimesCapped };

struct MatchResult
{
    std::vector<SourceNote> notes;   // the adjusted notes, in time order, times in beats from the start of the song
    int notesBefore = 0;             // the melody's notes
    int syllables = 0;               // the target
    int merged = 0, split = 0;       // how many notes were folded into the one before, and how many were cut in two
    bool exact = false;              // the notes now number exactly the syllables
    int remaining = 0;               // notes.size() - syllables when not exact (positive: too many notes still)
    Method method = Method::mergeSplit;
    int moved = 0;                   // syllableTimes: new note starts further than a 16th from every start the melody had
    int tooShort = 0;                // syllableTimes: notes made longer than the room before the next syllable or the note's end, up to minNoteBeats

    int getNotesAfter() const { return (int) notes.size(); }
};

/** How many syllables the lyrics have over the melody when each note takes one (the One syllable per note count). */
int countSyllablesToMatch (const Lyrics&, const Melody&);

/** Makes the notes number `syllables`. Too many notes: the shortest note that is neither the first nor the last of its
    phrase (a rest of half a second or more starts a phrase) is folded into the note before it, which keeps the pitch
    of the longer of the two, until the counts match. Too few: the longest note is cut in half at the same pitch, until
    they match. Stops early, with `exact` false, when no note can be merged (every note starts or ends a phrase) or
    split (all are shorter than twice minSplitBeats). */
MatchResult matchNotesToSyllables (const Melody&, int syllables);

/** Match to the words' timing: one note on each syllable, starting at the syllable's beat. The pitch and velocity are
    those of the melody note sounding at that beat (in a rest, of the nearest earlier note, or the nearest later if none
    is earlier). The count then equals the syllable count; when the counts already match the melody is returned as it is.
    `Method::syllableTimes` (legato): a note lasts until syllableGapBeats before the next syllable, but not past the end
    of its phrase (a rest of phraseRestSeconds or more ends one) or into the next phrase. The last syllable of a phrase
    lasts the longer of its source note's remaining length and its own length in the lyrics, at most lastSyllableMaxBeats,
    and not into the next phrase or the next syllable. `Method::syllableTimesCapped`: the note stops at the end of the
    melody note it comes from (in a rest, where the rest ends) or at the next syllable, whichever is first. Either way a
    note is never shorter than minNoteBeats (counted in `tooShort`). */
MatchResult matchNotesToSyllableTimes (const Melody&, const std::vector<Syllable>&, Method = Method::syllableTimes);

/** After matching (`done`): "Merged 3 notes into the one before and split 1 note in two: 12 notes now, 12 syllables."
    Before: "14 notes, 12 syllables. Matching would merge 2 notes into the one before." Both add what is left when the
    counts could not be made equal. */
juce::String describeMatch (const MatchResult&, bool done = true);
} // namespace daw::gen::singthis
