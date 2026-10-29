# Match the notes to the words (a part of Sing this)

Status: Built, not committed
Idea: [Sing this: a sung vocal from a melody and lyrics](../FEATURE_IDEAS.html#sing-this-a-sung-vocal-from-a-melody-and-lyrics)

## Goal
When a melody has more or fewer notes than the lyrics have syllables, the Sing window offers to adjust a copy of the notes so each note sings one syllable: the shortest notes are merged into the note before, or the longest are split in two. The user's own clip is never changed (generating never changes an existing track).

## Decisions for you
1. **Where the adjusted notes go.** Options: sing from them without saving; a new MIDI track; a new clip on the same track. **Recommended:** both of the first two: *Sing* uses the adjusted notes (nothing saved), and *Make a new track with them* adds a track "<melody track> (matched)" after the melody's, so nothing of the user's changes.
   Answer: Recommended option (the user saw it).
2. **Which count.** Options: syllables as the chosen mode pairs them, or always the One syllable per note count. **Recommended:** always One syllable per note's count (the syllables from the first one at the melody to the end of the last line that starts before it ends), and switch Words to *One syllable per note* when matching, because As timed needs each syllable to start on a note, which a count can't promise.
   Answer: Recommended option.
3. **What is never touched.** The first and last note of each phrase (a rest of 0.5 s or more) are never merged away, so phrases keep their start and end.
   Answer: As asked.

## What the user does and sees
- In the Sing window, between the picture and the advice, a strip shown only when the note count differs from the syllable count (and words exist): *Match the notes to the words* and a preview, for example "88 notes, 110 syllables. Matching would split 22 notes in two."
- After pressing it: the picture and advice show the adjusted notes, Words switches to *One syllable per note*, and the strip says "Singing from adjusted notes; your clip is unchanged. Split 39 notes in two: 110 notes now, 110 syllables." with *Make a new track with them* and *Use my notes*. *Sing* sings the adjusted notes.
- *Make a new track with them* adds a MIDI track "<name> (matched)" after the melody's, with the same built-in sound and one clip from the first note to the end of the last, then the window sings that track.
- When the counts can't be made equal (every note starts or ends a phrase, or notes are shorter than 0.25 beat), the text says how many notes or syllables remain.
- Changing the melody, the clip choice or the track resets it; changing the lyrics or notes recomputes it live. Not remembered between sessions. Tooltips on the three controls and the label.

## Steps
- [x] `src/generate/SingMatch.{h,cpp}`: `countSyllablesToMatch`, `matchNotesToSyllables` (merge the shortest inner note into the note before, the longer one's pitch wins, ties go to the earlier note; else split the longest at the midpoint, minimum 0.125 beat each), `describeMatch`. In both CMake targets.
- [x] Tests in `runSingThisTests`.
- [x] `SingWindow`: strip, buttons, `matched` state, `makeMatchedTrack`.
- [x] `ScreenshotRunner` sing scene: `thinNotes`, `match`, `makeTrack` options.
- [x] README, FEATURE_IDEAS.html. The guide texts need no change.

## Tests
Exact numbers for: 6 notes to 4 (starts 0, 2.5, 3.75, 4.75, pitches 60, 64, 67, 69), 2 notes to 4 (a 4 beat note becomes 1, 1, 2), a half beat note stops at four 32nd notes, phrase ends kept (6 notes stop at 4), equal counts unchanged, 7 notes over 5 syllables then pairing with nothing left over, no words gives target 0. The new clip uses the ordinary clip saving, so no new save test.

## Website hand-off
Screenshot of the Sing window with the strip, shot list scene `sing` with `"thinNotes": 5` and `"match": true`. A sentence on the Sing this section. No new library.

## Changes while building
- Matching always switches Words to One syllable per note (decision 2), so the count the user asked for is what is sung.
- (Changed in [sing-match-timing.md](sing-match-timing.md): the notes now keep their source velocity.)
