# Match to the words' timing (a part of Sing this)

Status: Built, not committed
Idea: [Sing this: a sung vocal from a melody and lyrics](../FEATURE_IDEAS.html#sing-this-a-sung-vocal-from-a-melody-and-lyrics)
Builds on: [plans/sing-match-notes.md](sing-match-notes.md)

## Goal
Merging and splitting puts new note starts where the merge or split lands, not at the times of the syllables, so matched notes can sound out of time with the words (the lyrics often come from audio detection, with rough times). A second way of matching puts one note on each syllable, at the syllable's own beat. It is the default; merging and splitting stays as the choice that keeps the melody's own rhythm. The user's clip is never changed.

## Decisions for you
1. **Default way.** Timing-based, as asked ("yes implement that"). Answer: as asked.
2. **Equal counts.** When the counts already match nothing is changed, even if the syllables start away from the notes (the strip only shows when the counts differ). Recommended and built so; the *As timed* mode and the advice already cover that case.

## What the user does and sees
- The strip in the Sing window (shown when the counts differ) gains a choice beside *Match the notes to the words*: **Match to the words' timing** (default) or **Match by merging and splitting**. Both have tooltips with the trade-offs. The choice is remembered (`sing.matchMethod`).
- Preview, timing: "88 notes, 110 syllables. Matching would move 22 note starts by more than a 16th." After: "Put a note on each syllable: 110 notes now, 110 syllables. 22 note starts moved by more than a 16th." Merge and split texts are as before.
- *Make a new track with them*, *Use my notes* and the switch of Words to *One syllable per note* work as before. The matched notes (both ways) now keep the velocity of the note they come from; the new track's clip uses it too.

## The rule (timing)
For each syllable of the One syllable per note count, in time order, one note at the syllable's beat. Pitch and velocity come from the melody note sounding at that beat; in a rest the nearest earlier note, and when none is earlier the nearest later. A note start counts as moved when it is more than 0.25 beat (a 16th) from every start the melody had. Never shorter than 0.125 beat (made 0.125 and counted in the text when the room is shorter). Three choices, remembered as `sing.matchMethod` (0, 1, 2):
- **Match to the words' timing** (default, legato): a note lasts until 0.125 beat before the next syllable, even across the gap between notes, but never past the end of its phrase (a rest of 0.5 s or more, the phrase rule of SingScore) or into the next phrase. The last syllable of a phrase lasts the longer of its source note's remaining length and its own length in the lyrics, at most 2 beats, and stops before the next phrase and before the next syllable's gap.
- **Match by merging and splitting**: as in sing-match-notes.md.
- **Notes as long as the melody's**: the same starts, but a note stops at the end of its source note (in a rest, where the rest ends) or at the next syllable: staccato, with gaps where the melody had them. This was the first version of the timing rule.

## Steps
- [x] `SourceNote` and `MelodyNote` get `velocity` (default 0.8); `buildMelody`, merge/split and the window carry it.
- [x] `SingMatch`: `Method`, `matchNotesToSyllableTimes`, `moved`, `tooShort`, `describeMatch` for both methods.
- [x] `SingWindow`: the choice (`matchMethodBox`), tooltips, setting, preview text.
- [x] Tests in `runSingThisTests`: exact starts, lengths, pitches, velocities, rests, a syllable before the melody, two syllables on one note, minimum length, equal counts, the descriptions.
- [x] README, FEATURE_IDEAS.html (part link, idea stays Partly done).

## Tests
Legato: the same example gives lengths 0.875, 1.375, 1.275, 0.475, 1, 2; a 0.5 beat gap between words is filled; a 9 beat word from beat 4.5 stops at the next phrase (6); the last syllable stops at 2 beats; 0.125 beat room gets the minimum. Melody lengths (the first version): 3 notes (60 [0,2), 64 [2,4), 67 [6,8) with a rest from 4 to 6) over syllables at 0, 1, 2.5, 3.9, 4.5, 6: starts as the syllables, lengths 1, 1, 1.4, 0.125 (raised from 0.1), 1.5, 2, pitches 60, 60, 64, 64, 64, 67, 4 starts moved. A syllable before the melody; two syllables on a 4 beat note give two 2 beat notes; equal counts unchanged; velocities kept.

## Website hand-off
Screenshot of the Sing window's strip with the choice: shot list scene `sing`, `"voices": "none"`, `"thinNotes": 5`, with and without `"match": true` (one shot per run: thinning repeats in a later shot). A sentence on the Sing this section. No new library.

## Changes while building
- Fixes the earlier limitation: matched notes (both ways) keep the source note's velocity; the new clip no longer uses 0.8 throughout (it still is the default for notes with none).
- Follow-up: the first version capped notes at the source note's end, which made them too short; that is now the third choice and legato is the default. The setting `sing.matchMethod` keeps its meaning for 0 and 1.
- Not listened to: whether the timing-based notes sound better with a real voice is unchecked.
