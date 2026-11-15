# Guided exercise: learn the chords of a song

Status: Released (six-step amendment, theory checked)
Idea: [#196 — Guided exercise: learn the chords of a song](https://github.com/geertvandamme/dawit/issues/196)
Part of: [#195 — Guided exercises that teach by doing in your own song](https://github.com/geertvandamme/dawit/issues/195) (this plan also does #195's first item, the shared step model).
Depends on: the First song exercise of #159 (`plans/first-sound-and-guided-starting-tasks.md`, *Built, not committed*): `src/generate/FirstSong.{h,cpp}`, `src/ui/FirstSongExercise.{h,cpp}`, `src/ui/GuidePointer.{h,cpp}`, `src/ui/StartPanel.{h,cpp}` exist only in the working tree. Commit that work first, or build this on top of it in the same tree.

## Goal

A second guided exercise that teaches chords with a song the user likes: bring in a recording, let DawIt find its chords, play along with them, then swap one and hear what changes. Each step ticks from the real state of the song, points at the control, links to the guide page behind it and says what's missing when a step can't be done yet. To build it, the First song exercise's step list and window become one shared model and one window that run any exercise.

**Amendment (2026-11-15):** finding the song's tempo and its sections become their own steps, so the exercise runs: bring in a song, find its tempo, its chords and its sections, play along, swap a chord. The tempo lines the song's bars up with the recording, which the chords and sections need; the sections show which parts repeat, so the learner knows what to practise.

## Decisions for you

1. **Shared model now, or a second window.** Options: generalise `gen::firstsong` and `FirstSongExercise` into one exercise model and one window (First song becomes its first user); or copy them for this exercise. **Recommended:** generalise. #195 asks for one window and one model, and a copy would have to be merged before #197 to #209.
   Answer: the recommendation (user, 2026-11-15).
2. **Where it opens.** Options: Help > *Learn the Chords of a Song...*; also an audio clip's right-click *Learn this song's chords...*; also a list of exercises. **Recommended:** the Help item and the clip item. A list of all exercises, and the Start panel tile (#209), stay in #195.
   Answer: the recommendation (user, 2026-11-15), and also the Start panel tile, #209 (user, 2026-11-15).
3. **How playing along is confirmed.** Options: tick on opening Play along (the issue's first version); tick once the song has played through 4 different bars with chords while Play along is open; listen to MIDI input for the right notes. **Recommended:** 4 bars while open: still simple and honest ("the song played with the chords in front of you"), and it can't tick from a click alone. Listening to what the user plays is later.
   Answer: the recommendation (user, 2026-11-15).
4. **Remembering "played along".** Options: in memory for the open song (gone after a restart or another song); in app settings. **Recommended:** memory, like First song's chord baseline. Only the step and "finished once" go in settings; the step is quick to redo.
   Answer: the recommendation (user, 2026-11-15).
5. **Step 1 needs a recording of at least 4 bars.** **Recommended:** yes: chords come one per bar, and step 2 asks for 4 of them. A shorter clip gets a status line saying so.
   Answer: the recommendation (user, 2026-11-15).
6. **The swap step's suggestion.** Options: the relative minor or major of the first chord (C to Am); the chord that most often follows; no suggestion. **Recommended:** relative: it shares two notes, so it fits the song and sounds different, and in a major key it stays in the key.
   Answer: the recommendation (user, 2026-11-15).
7. **Point to the exercise from the chord guide.** **Recommended:** one sentence in *Playing along* (no new fact, no source change).
   Answer: the recommendation (user, 2026-11-15).

**Amendment (2026-11-15): tempo and sections.**

8. **Detect tempo and detect song structure become steps of this exercise.** Answer (user, 2026-11-15): yes, as their own steps, in this order: 1 Bring in a song, 2 Find its tempo, 3 Find its chords, 4 Find its sections, 5 Play along, 6 Swap a chord.
9. **Keep the name** "Learn the chords of a song" (Help item, clip item *Learn this song's chords...*, start tile "Learn from a song you like" / "Find its chords and play along"). Options: keep; rename to "Learn a song you like". **Recommended:** keep: chords are still what it teaches and plays along with; tempo and sections serve them. Only the Help description and the intro change.
   Answer: the recommendation (user, 2026-11-15).
10. **What marks the tempo as found.** Options: (a) in memory for this song: Detect tempo's *Set tempo* on the song clip, or Detect chords / Detect song structure putting their result on a row while reporting the clip lined up with the song's bars; done while the song's tempo map and the clip's start are unchanged since; (b) the same, saved in the project (a format change); (c) only *Set tempo*. **Recommended:** (a): it follows the real state (Undo, a tempo edit or moving the clip untick it), needs no format change, and like "played along" (decision 4) it is quick to redo after a restart. The lined-up path ticks it for a song already at the recording's tempo, and after typing a tempo by hand.
   Answer: the recommendation (user, 2026-11-15).
11. **When the sections step ticks.** Options: sections cover at least 4 bars inside the song clip's span; at least 2 sections start in the span. **Recommended:** 4 bars covered, matching the chord step's rule; a 4- to 8-bar clip often gives one section, which is the honest answer.
   Answer: the recommendation (user, 2026-11-15).
12. **Tempo before chords and sections: required or advised.** **Recommended:** advised: a `textDim` tip in steps 3 and 4 while step 2 isn't ticked, the button stays the step's own; both dialogs already check the bars and offer *Detect tempo first*. A song without a clear beat can then still be done.
   Answer: the recommendation (user, 2026-11-15).
13. **Users part-way through the 4-step version.** **Recommended:** their saved step maps to the same step's new number (0, 1, 2, 3 to 0, 2, 4, 5); "finished" stays finished (*Start over* shows the six steps).
   Answer: the recommendation (user, 2026-11-15).
14. **A second example recording for tests and shots**: 16 bars with a beat and two parts, so tempo and structure detection have something real to find. **Recommended:** yes; the 8-bar example stays for the existing tests.
   Answer: the recommendation (user, 2026-11-15).

## What the user does and sees

**Opening.** Help > *Learn the Chords of a Song...* (new command, no shortcut, under *First Song Exercise...*); an audio clip's right-click menu, *Learn this song's chords...* (after *Detect chords...*; opens the exercise only). The window is the shared exercise window (360 × 500, always on top, first at the main window's top right), titled "Learn the chords of a song", with the same rows, body, Back / "N of 4 done" / Skip / Next and completion as First song. It reopens at the remembered step; reopening brings it to the front.

**Changes to the shared window** (First song looks as before):
- A step may link to two guide pages (two link chips).
- The body's status line explains a missing prerequisite in `textDim` and the action button turns into the way forward (texts in *Theory*).
- The completion's first button is per exercise: First song *Carry on with Start here*; this one *Open the chord guide* (Chord progressions · *Playing along*).

**Steps** (texts in *Theory*; the *song clip* is the longest unmuted audio clip, ties to the earliest):

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Bring in a song | an unmuted audio clip spans ≥ 4 bars of the song | *Import audio...* (`importAudioCommand`) | the start panel's Import tile if shown, else the track lanes |
| 2 Find its chords | ≥ 4 bars inside the song clip's span have a chord | *Detect chords* (`ChordDetector::detect` on the song clip: the existing dialog, with *Detect tempo first*) | outlines the song clip |
| 3 Play along | the song played through 4 different bars with a chord while Play along was open (this song, this session) | *Open Play along* (`showPlayAlong`) | the Chords header's *Play along* button |
| 4 Swap a chord | a bar in the span had a chord in the baseline and now has a different chord (removing one doesn't count) | *Open the Chords row* (unfold, pointer on the suggested bar) | the suggested bar on the Chords row |

**Missing prerequisites** (status line and button):
- Step 2 without a song clip of 4 bars: button *Import audio...*.
- Step 3 or 4 with no chords in the span: button *Detect chords* (or *Import audio...* with no song clip).
- Step 3 with no audio output device: button *Audio/MIDI Settings...*.
- No model is needed anywhere: step 2's text says so.
- Step 3 shows "Played along: 2 of 4 bars" while counting.

Nothing changes the song except the user's own choices in the existing dialogs and rows: *Detect chords* only opens the dialog, whose *Put on chord track* writes the chords.

**Remembered** in app settings: `learn.songChords.step` (0 to 3) and `learn.songChords.done`. First song keeps `learn.firstSong.step` and `learn.firstSong.done`.

### Amendment: six steps

The exercise grows to six steps; everything above holds unless said here. Help description, intro, completion and the changed texts are in *Theory* (*Amendment texts*). The window shows six rows and "N of 6 done", 360 × 740 (was 680; the viewport still scrolls on a short screen).

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Bring in a song | as before | as before | as before |
| 2 Find its tempo | the **tempo record** matches: the song clip is the recorded clip, its start beat and the song's tempo map are unchanged since | *Detect tempo* (`TempoDetector::detect` on the song clip: the existing dialog) | outlines the song clip |
| 3 Find its chords | as old step 2 | as before | as before |
| 4 Find its sections | the Sections row's sections cover ≥ 4 bars inside the song clip's span | *Detect song structure* (`StructureDetector::detect` on the song clip: the existing dialog) | no sections in the span: outlines the song clip; else the Sections row over the span |
| 5 Play along | as old step 3 | as before | as before |
| 6 Swap a chord | as old step 4 | as before | as before |

**The tempo record** (in memory, `MainComponent`, like the swap baseline): the song clip's identity, its start beat and the song's tempo map, taken when (a) Detect tempo's *Set tempo* finishes for that clip, or (b) Detect chords' *Put on chord track* or Detect song structure's *Put on Sections row* finishes for that clip while its dialog found the recording lined up with the song's bars (`adviseLiningUp` empty). A failed Detect tempo ("No clear beat") on the song clip is remembered too, for the status line. Cleared in `afterReplace` only: *Start over* keeps it, since it describes the song, not practice.

**Missing prerequisites and tips** (status line; the button changes only where a fix is named):
- Steps 2 and 4 without a song clip of 4 bars: their own "needs a recording" line, button *Import audio...*.
- Step 2, record taken but no longer matching: "changed since" line; button stays *Detect tempo*.
- Step 2, no clear beat found in the song clip: line with the way forward (type the tempo, then Skip); button stays.
- Steps 3 and 4 while step 2 isn't ticked: a tip to find the tempo first; button stays.
- Step 4 with a span under 16 bars and no sections yet: a line that a short recording may give one or two sections.
- Steps 5 and 6 point at "step 3" for chords (was step 2).

**Remembered:** the step key holds 0 to 5. A steps-version key beside it (absent: the 4-step version) tells old saves apart; an old step maps 0, 1, 2, 3 to 0, 2, 4, 5 and the version is written. `learn.songChords.done` is kept.

**First steps:** no change to `StartPanel` (the tile's "Find its chords and play along" stays true), the inspector's *Get started* or the First song exercise.

## Data and saving

No project data and no format change. In memory only, owned by `MainComponent` and reset when the song is replaced (`afterReplace`) or on *Start over*: the swap baseline (taken when step 4 first opens in this session, and again each time *Put on chord track* writes detected chords) and the played-along bars.

**Amendment:** still no project data. In memory: the tempo record (above). In settings: the steps-version key and the step migration (above), tested as a pure function.

## The guide

No new guide, no new topics, no `gen::guides` entry, no `TheoryGuideLinks.cpp` or `SOURCES.md` change: every fact is DawIt behaviour, derived, or already in the Chord progressions guide with its sources. Links (each checked by a test to name an existing topic):
- Step 1: MIDI · *What MIDI is* (audio versus MIDI).
- Step 2: Finding the key and chords · *Naming a chord from its notes*; Note lengths, beats and bars · *Tempo and time*.
- Step 3: Chord progressions · *Playing along*.
- Step 4: Chord progressions · *Your chord track*; Chord progressions · *How chords work*.

One sentence is added to `gen::playalong::explainPlayingAlong` (the *Playing along* text, after the tips): see *Theory*.

**Amendment links** (no guide text, `gen::guides`, `TheoryGuideLinks.cpp` or `SOURCES.md` change; the links test covers them):
- Step 2, tempo: Note lengths, beats and bars · *Tempo and time*; Note lengths, beats and bars · *Bars and beats*.
- Step 3, chords: Finding the key and chords · *Naming a chord from its notes* only (*Tempo and time* moves to step 2).
- Step 4, sections: Song structure · *How songs are built*; Song structure · *Your song's structure*.

## Code

Recipes: *A command or menu item*, *A window*, *Checking in the real app*. Adds a new recipe, *A guided exercise*.

- **`gen::exercises`** (new, UI-free): the exercise list (First song, Song chords), each with a settings-key stem, title, intro, completion text, carry-on link and steps (title, subtitle, why, one or two links), and one way to ask, given the engine and a **context** of app facts the owner fills in (saved state, chord baselines, played-along bars, has an output device): is step *n* done, its "do" text, and its action (label, tooltip, enabled) and status line. First song's content stays in `gen::firstsong`, reached through it unchanged.
- **`gen::songchords`** (new, UI-free): the steps and texts, the song clip and its bar span (from `Clip::getEndBeat`), the four done rules, the played-along counter (pure: fed bar, has-chord, playing, window open; counts distinct bars), the swap suggestion, and a made-up example recording (C G Am F twice, at a given tempo) for tests and screenshots.
- **`ExerciseWindow`** replaces `FirstSongExercise` (rename, take an exercise); one window per exercise, keyed in `PluginWindowManager`. `MainComponent` dispatches action and *Show me* by exercise and step.
- **`GuidePointer`** gains an optional area inside the target (asked on each follow), to outline one clip in `TrackLanes` or one bar in `ChordLane`.
- **Played along:** while Play along is showing, a message-thread timer at 4 Hz feeds the counter the bar under the heard playhead (a bar lasts at least 1 s up to 240 BPM, so none is missed).
- **Invariants:** no tick from a button press; no exercise writes notes, chords or instruments; nothing reads a deleted clip (find the song clip fresh each time; `SafePointer` for components).
- Threading: message thread only; reads the edit as the views do. No audio-thread or format change. Risk: the rename touches `MainComponent`, `InspectorPanel`, `TheoryGuidesView` and `ScreenshotRunner`; First song's tests and shots must pass unchanged.

**Amendment code.**
- `gen::songchords`: named steps instead of bare numbers (the old 0 to 3 are spread through `MainComponent`, `Exercises.cpp` and the shot keys: renumber all of them); two new `Fix` values (detect tempo, detect song structure); the tempo record as a plain value in `exercises::Context` (clip identity compared by address to a fresh `findSongClip`, never dereferenced; start beat; `TempoMap`, which has `operator==`) plus "no beat found for this clip"; pure rules for steps 2 and 4, statuses and done notes; the step migration.
- Detectors report what the exercise needs: `TempoDetector` says which clip it set the tempo from and which had no clear beat; `ChordDetector` and `StructureDetector` say which clip they wrote from and whether it lined up. `ArrangementView` forwards them (as it does `onChordsDetected`) and gains *detect tempo* and *detect structure* entry points like `detectChords`, a `SectionLane` getter and the area of a bar range on it (like `ChordLane::getBarArea`).
- Invariants as before: no tick from a button press (a dialog's Cancel records nothing), nothing written but by the dialogs' own buttons.
- Risk: renumbering. Every `case` on a song chords step and every shot with `exerciseStep` for `songChords` changes; First song is untouched.

## Libraries and models

None. Chord detection is DawIt's own signal processing (`analysis::chordfinder`), with no model or download. No `LICENSE`, `licenses/` or *License note* change.

Amendment: none. Tempo and structure detection are DawIt's own signal processing (`analysis::tempofinder`, `analysis::structurefinder`), no model, download or new dependency. No `LICENSE`, `licenses/` or *License note* change.

## Theory

No new guide content. The exercise's texts state these facts; none needed a new search (all are DawIt behaviour, derived, or in an existing guide with its source in `SOURCES.md`).

### Facts

1. An imported recording is audio: the sound itself; its notes can't be edited like MIDI. Exact (definition); MIDI · *What MIDI is*.
2. Detect chords finds one chord per bar of the song. Exact (DawIt behaviour, `ChordFinder.h`; README limit "a bar holds one chord").
3. It measures the 12 note names in any octave per bar, filters short sounds (drums) out, and compares them with major, minor, 7, maj7 and m7 chords on every root (60 chords); the bass note decides between close ones. Exact (`ChordFinder.h`, tested: vocabulary size 60).
4. It can't name sus, diminished, augmented or slash chords; it gives the nearest of the 60. Exact (the vocabulary).
5. It needs the recording's bars on the song's bars; the dialog offers *Detect tempo first* otherwise. Exact (`adviseLiningUp`).
6. A busy bass, passing notes or a sung melody can fool it. Typical (DawIt's existing dialog text).
7. It needs no download and runs on the computer. Exact (no model).
8. Playing along is how most musicians learn; look at the next chord; keep the rhythm going. Opinion, already in Chord progressions · *Playing along* (teachers' tips).
9. Playing just the root (bass note) is an easier start than whole chords. Opinion (same topic's tip "play just the bass note rather than stopping").
10. A major chord's relative minor has its root 3 semitones lower and shares two notes: C (C E G) and Am (A C E) share C and E. Exact (derived; Chord progressions states "shares two notes with the home chord"; Finding the key and chords states "three semitones lower").
11. In a major key, swapping any major or minor chord of the key for its relative stays in the key: C↔Am, F↔Dm, G↔Em. Exact (derived from the C major triads).
12. The relative minor often sounds darker than its major. Opinion (Chord progressions: "home's sad twin ... with a darker feel").
13. 4 bars of 4/4 at 120 BPM last 8 s. Exact (derived: 16 beats × 0.5 s).
14. Play along opens with Cmd+4; Import Audio with Cmd+I. Exact (command shortcuts).
15. The Chords row makes no sound by itself; a recording doesn't change when its chords on the row change. Exact (DawIt behaviour).

### Texts

Title "Learn the chords of a song". Help item *Learn the Chords of a Song...*, description "Find the chords of a recording you like, play along with them and swap one, in four steps". Clip menu item *Learn this song's chords...*.

Intro: "Four steps with a song you like: bring it in, let DawIt find its chords, play along with them, then swap one and hear what changes. Do each step in the main window: the tick appears by itself."

1. **Bring in a song** — "Import a recording you like"
   - Do: "Press Import audio... (Cmd+I) and choose a song file on your computer, such as an MP3 or WAV file, or drag the file onto the tracks. DawIt puts it on an audio track at the playhead. A song with a clear guitar or piano part and three or four chords is the easiest to start with."
   - Why: "A recording is audio: the sound itself, not the notes. DawIt can't read its chords the way it reads a MIDI clip, so in the next step it listens for them."
   - Status, clip under 4 bars: "Your longest recording, <name>, lasts <2.5> bars at the song's tempo. Chords are found one per bar, so this exercise needs at least 4: import a longer part of the song."
   - Done: "<name> is <32> bars long at the song's tempo."
   - Button tooltip: "Opens Import Audio (Cmd+I): choose a song file to put on an audio track. Nothing else in the song changes."
   - Show me: "Drop a song file here, or choose File > Import Audio... (Cmd+I)."
2. **Find its chords** — "Let DawIt listen for one chord per bar"
   - Do: "Press Detect chords. DawIt listens to <name> and shows the chords it hears, one per bar of the song, and the key they sound like. If it says the bars don't line up, choose Detect tempo first, set the song's tempo from the recording, then press Detect chords again. When the chords look right, press Put on chord track: they appear on the Chords row above the tracks."
   - Why: "In each bar DawIt measures how strongly each of the 12 note names sounds, in any octave, leaving out short sounds such as drums. Then it picks the major, minor or seventh chord that fits best, and the lowest note decides between close ones. That's how you find chords by ear too: which notes ring, and which is lowest. It needs no download. Check the result by ear: a busy bass, passing notes or a sung melody can fool it, and it can't name sus, diminished or slash chords, so it gives the nearest chord it knows."
   - Status, no song clip: "Detect chords needs a recording of at least 4 bars: step 1 brings one in."
   - Done: "<12> bars of <name> have a chord."
   - Button tooltip: "Listens to <name> and shows the chords it finds. Nothing changes until you press Put on chord track."
   - Show me: "Your recording. Right-click it and choose Detect chords... to find its chords."
3. **Play along** — "Play the chords as the song plays"
   - Do: "Press Open Play along (Cmd+4). It shows the chord playing now, large, as piano keys and a guitar chord box, with the next one beside it. Press Space to play the song and play the chords on a piano, a keyboard or a guitar. If whole chords are too much at first, play only the root, the note the chord is named after, then add the rest. The tick comes once you have played along through 4 bars with chords."
   - Why: "Playing along is how most musicians learn songs: the recording keeps going and you fit in. Watch the next chord, not the one you're playing, and keep the rhythm going even when a change comes late. DawIt doesn't listen to what you play: the tick only says the song played with the chords in front of you."
   - Counting: "Played along: <2> of 4 bars."
   - Status, no chords: "Play along shows the chords on the Chords row, and there are none on your recording yet. Find them first (step 2)."
   - Status, no output: "No audio output is chosen, so you won't hear the song. Choose one in Audio/MIDI Settings."
   - Button tooltip: "Opens Play along with the chords (Cmd+4). Press Space to play; the window follows the song."
   - Show me: "Play along: the chord playing now, large, with the next one beside it."
4. **Swap a chord** — "Change one and hear the difference"
   - Do: "Click bar <1> on the Chords row and choose <Am> in place of <C>. The recording doesn't change: it still plays <C>. Play your new chord along with it in Play along, or click it in the open Chords row to hear it on its own, and listen: does it fit, clash, or change the mood? Then try a swap of your own. Undo (Cmd+Z) brings the found chord back."
   - Why, major chord: "<Am> is the relative minor of <C>: its root is 3 semitones lower, and they share two of their three notes, <C and E>. Chords that share notes can often stand in for each other, which musicians call a substitution. That's why the swap still fits the song, and why it often sounds darker."
   - Why, minor chord: "<C> is the relative major of <Am>: its root is 3 semitones higher, and they share two of their three notes, <C and E>. Chords that share notes can often stand in for each other, which musicians call a substitution. That's why the swap still fits the song, and why it often sounds brighter."
   - Status, no chords: as step 3's.
   - Done: "Bar <1>: <C> is now <Am>."
   - Button tooltip: "Unfolds the Chords row and points at bar <1>. Click a bar to choose its chord."
   - Show me: "Bar <1>: click it and choose <Am>."

Completion: "Done. You found the chords of a song, played along with them and heard what one swap does. Next, try another song, or loop a hard change and practise it slowly: drag along the ruler to loop those bars." Buttons *Open the chord guide* (tooltip "Opens Chord progressions at Playing along") and *Start over* (tooltip "Back to step 1. Your song stays as it is.").

Sentence added to *Playing along* (after the tips): "To learn a song you like step by step, from its recording to playing along, use Help > Learn the Chords of a Song...."

### Example data

- Example recording: C G Am F C G Am F, a bar each at the song's tempo (120 BPM: 2 s a bar, 16 s, 8 bars, 32 beats), made as in the chord finder test (each chord's notes around middle C with 4 overtones, plus the root two octaves lower; 25 ms fades). Detected: the same 8 chords; swap suggestion bar 1, C → Am, shared C and E.
- Swap table: C→Am (C, E); G→Em (G, B); F→Dm (F, A); Am→C (C, E); Dm→F (F, A); G7→Em (triad G B D, shares G, B); Cmaj7→Am.

### "Your song" rules

None (no guide topic). The step rules are under *What the user does and sees*.

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| Example recording, 120 BPM | 16 s, spans 32 beats = 8 bars | 1e-6 beats |
| Example recording detected | C, G, Am, F, C, G, Am, F | exact |
| Step 1, clip 7.9 s at 120 BPM (3.95 bars) / 8.0 s (4 bars) | false / true | exact |
| Step 2, 3 / 4 chords in span; 4 outside span | false / true; false | exact |
| Counter: bars 0,0,1,2 / 0,1,2,3; chordless bar, stopped or window closed | false / true; not counted | exact |
| Step 4: bar 0 C→Am / C removed / bar outside span changed | true / false / false | exact |
| Swap C, G, F, Am, Dm, G7 | Am, Em, Dm, C, F, Em; shared notes 2 each | exact |
| Settings step -1, 4, 9 | 0 | exact |
| Bar at 240 BPM in 4/4 vs 4 Hz sampling | 1.0 s ≥ 0.25 s | exact |

### Amendment: facts

No guide's facts change, so no `SOURCES.md` change; the one fact searched is cited here.

16. BPM is beats per minute; at 120 BPM a beat lasts 0.5 s, at 96 BPM 0.625 s. Exact (definition; derived 60 / 96); Rhythm · *Tempo and time*.
17. The first beat of a bar is the downbeat. Exact (definition).
18. Detect tempo marks where the sound suddenly gets louder (mostly drums), finds how often that repeats, places the beats on it, and takes as each bar's first beat the one where chords change and kick and bass hit hardest (a good guess). Exact (DawIt behaviour, `TempoFinder.h`, after Ellis 2007).
19. It needs a clear beat and enough of it: it gives up on audio shorter than 4 of its longest beat periods (4 × 60 / 40 BPM = 6 s, `tempofinder::findPeriod`), on a weak repeat, or with fewer than 8 beats; the dialog says "shorter than about 6 seconds". Exact (`tempofinder::findPeriod` and `analyse`; derived 4 × 1.5 s). (Theory check: the 6 s comes from the 4-period minimum, not from 8 beats at 80 BPM.)
20. Tempo detectors often report half or double the tempo a listener taps (the "octave error"). Typical: tempo-estimation research calls it an octave error and scores answers within a factor of 2 or 3 leniently for that reason (Schreiber, *Data-Driven Approaches for Tempo and Key Estimation of Music Recordings*, PhD thesis, FAU Erlangen-Nürnberg 2020, section 2.4: estimates that are integer multiples or fractions of the true tempo are called octave errors, and ACC2 counts 1/3, 1/2, 2 and 3 times the tempo as right, https://www.audiolabs-erlangen.com/content/05_fau/professor/00_mueller/01_group/2020_Schreiber_TempoKeyEstimation_ThesisPhD.pdf; Levy, "Improving perceptual tempo estimation with crowd-sourced annotations", ISMIR 2011: tempo estimators frequently suffer from octave error, and listeners tap at different metrical levels of the same piece but agree which pieces are fast, https://archives.ismir.net/ismir2011/paper/000015.pdf). Both read in the theory check. Searches: "tempo estimation octave error half double tempo beat tracking Ellis 2007"; "octave errors tempo estimation half or double perceived tempo listeners tap". DawIt's dialog says the same.
21. Setting a new tempo keeps MIDI clips, chords, sections and lyrics on their bars; audio keeps its speed, so audio clips change length on the timeline. Exact (DawIt behaviour, the Detect tempo dialog).
22. A band playing without a click drifts; when the tempo is not steady (beyond 2.5% or 2 BPM) and there are at least two bars to measure, Detect tempo offers *Follow the recording*, which gives every bar its own tempo. Typical (drift), exact (DawIt behaviour, `TempoDetector::offer`).
23. Detect song structure compares the four bars before and after each bar line (chords, bright or dull sound, loudness, drums), prefers parts of 4, 8 or 16 bars, groups parts with the same chords and sound, and calls the loudest music that comes back the chorus. Exact (DawIt behaviour, `StructureFinder.h`, after Foote 2000 and McFee and Ellis 2014).
24. Songs are mostly built from phrases of 4 bars; sections are usually 4, 8 or 16 bars. Typical; Song structure · *How songs are built* (its sources).
23a. Parts may be any length up to 16 bars (an intro of 2 bars, for one); 4, 8 and 16 cost nothing, other lengths are only taken when the change is strong, so a short recording can still be cut. Exact (`structurefinder` `lengthCost`, `findBoundaries`); the reason the short-span status no longer says "phrases of 4 bars or more".
25. A verse and a chorus with the same chords and sound can't be told apart by it. Exact (the dialog's caveat).
26. Most pop and rock songs repeat their parts, so knowing the verse and chorus covers most of the song. Opinion (follows from the verse-chorus form in Song structure; worded as "most pop and rock songs").
27. Clicking a section on the Sections row opens its menu, with *Type* and *Loop this section*. Exact (`SectionLane` menu and tooltip).
28. 16 bars of 4/4 at 96 BPM last 40 s (64 × 0.625); a bar 2.5 s. Exact (derived).

### Amendment texts

Help description: "Find the tempo, chords and sections of a recording you like, play along with its chords and swap one, in six steps".

Intro (replaces): "Six steps with a song you like: bring it in, let DawIt find its tempo, chords and sections, play along with the chords, then swap one and hear what changes. Do each step in the main window: the tick appears by itself."

Step 1 Why (replaces): "A recording is audio: the sound itself, not the notes or the beat. DawIt can't read its tempo or chords the way it reads a MIDI clip, so in the next steps it listens for them."

2. **Find its tempo** — "Line the song's bars up with the recording"
   - Do: "Press Detect tempo. DawIt listens to <name> and shows its tempo in BPM (beats per minute) and whether it stays steady. Press Set tempo: the song takes that tempo, and <name> moves so its first downbeat, the first beat of a bar, starts a bar. If the tempo feels twice as fast or as slow as you would tap your foot, choose half or double before you press Set tempo."
   - Why: "The bars and beats on the ruler are only a grid until the song has the recording's tempo. Then every bar of the grid is a bar of the song, so the chords in step 3 fall one per bar and the sections in step 4 start on the right bars. DawIt marks the moments where the sound suddenly gets louder, mostly the drums, finds how often that pattern repeats and places the beats on them. It needs no download, but it needs a clear beat, such as drums, and at least about 6 seconds of it. Tempo detectors often find half or double the tempo you would tap along to, so tap along to check. A band playing without a click often drifts: when it does, Detect tempo also offers Follow the recording, which gives every bar its own tempo."
   - Status, no song clip: "Detect tempo needs a recording of at least 4 bars: step 1 brings one in."
   - Status, changed since: "The song's tempo or the place of <name> has changed since its tempo was found. Detect it again so the bars line up."
   - Status, no clear beat: "No clear beat was found in <name>: it may have no drums, or play freely. If you know the song's tempo, double-click the tempo next to the transport and type it, then press Skip: Detect chords checks whether the bars line up."
   - Done, one tempo: "The song is at <96> BPM, the tempo of <name>." Following: "The song's tempo follows <name> bar by bar, from <88> to <95> BPM."
   - Button tooltip: "Listens to <name> and shows its tempo. Nothing changes until you press Set tempo."
   - Show me: "Your recording. Right-click it and choose Detect tempo... to find its beat."
3. **Find its chords** (old step 2) — Do (replaces its third sentence): "If it says the bars don't line up, choose Detect tempo first and set the tempo as in step 2, then press Detect chords again."
   - Tip, step 2 not ticked: "Tip: find the tempo first (step 2), so each chord falls on its own bar. Detect chords checks this too."
4. **Find its sections** — "Mark its intro, verses and choruses"
   - Do: "Press Detect song structure. DawIt listens to <name> and shows the parts it hears, such as intro, verse and chorus, with why each got its name and which parts are the same music (A, B, C). When they look right, press Put on Sections row: they appear on the Sections row above the chords. Click a section there and choose Loop this section to practise it, or Type to change it if DawIt guessed wrong."
   - Why: "Most pop and rock songs repeat their parts: once you can play the verse and the chorus, you know most of the song, and the sections show where each comes back. DawIt compares the four bars before and after every bar line: their chords, their sound (bright or dull), how loud they are and how much drums there are. A new part starts where these change, after 4, 8 or 16 bars where it can, because songs are mostly built from phrases of 4 bars. Parts with the same chords and sound are the same music heard again, and the loudest music that comes back is taken as the chorus. Check it by ear: a verse and a chorus with the same chords and sound can't be told apart."
   - Status, no song clip: "Detect song structure needs a recording of at least 4 bars: step 1 brings one in."
   - Tip, step 2 not ticked: "Tip: find the tempo first (step 2), so the sections start on the right bars. Detect song structure checks this too."
   - Status, span under 16 bars and no sections: "<name> is <8> bars long. DawIt finds where parts start by comparing the 4 bars on each side of a bar line, so a short recording may give only one or two sections: a whole song shows its structure best."
   - Done: "<name> has <4> sections: <Verse 1, Chorus 1, Verse 2, Chorus 2>." (at most four names, then "and <n> more").
   - Button tooltip: "Listens to <name> and shows the parts it finds. Nothing changes until you press Put on Sections row."
   - Show me, no sections: "Your recording. Right-click it and choose Detect song structure... to find its parts." With sections: "Your song's sections. Click one to loop it or change its type."
5. **Play along** (old 3): status, no chords: "... Find them first (step 3)."
6. **Swap a chord** (old 4): unchanged.

Completion (replaces): "Done. You found the tempo, chords and sections of a song, played along with its chords and heard what one swap does. Next, try another song, or practise one part at a time: click its section on the Sections row and choose Loop this section."

### Amendment example data

- Example song with a beat (new; the 8-bar example stays): 16 bars of 4/4 at a given tempo, verse C G Am F and chorus F G C G alternating every 4 bars (V C V C), chords made as the 8-bar example; the verse 10 dB softer than the chorus; a beat all through: a short low kick on beats 1 and 3 and a short high noise tick on every beat (30 ms). At 96 BPM: 0.625 s a beat, 2.5 s a bar, 40 s. If the kick changes the detected bass notes, keep the ticks only and note it under *Changes while building*.

### Amendment test numbers

| Check | Expected | Tolerance |
|---|---|---|
| Steps, links | 6 steps; every link names an existing topic | exact |
| Migration, old version: 0, 1, 2, 3; -1, 4, 9 | 0, 2, 4, 5; 0 | exact |
| Migration, new version: 0 to 5; 6 | kept; 0 | exact |
| Step 2: record matches / clip start +0.5 beat / tempo 120 to 121 / a longer clip becomes the song clip / no record | true / false / false / false / false | exact |
| Step 4: sections over 3 bars of the span / 4 bars / only outside / one half overlapping (2 of its 4 bars in span) plus 2 more in span | false / true / false / true | exact |
| Step 3 and 4 tip shown iff step 2 not done; step 4 short-span line at 15 / 16 bars | shown / not shown | exact |
| Example song at 96 BPM: length | 40 s, 64 beats, 16 bars | 1e-6 |
| Tempo found on it at 96 / 120 BPM | 96 / 120 | ±0.5 BPM |
| Its first downbeat vs the clip's start | within a third of a beat (0.208 s at 96) | exact bound |
| Chords found on it | C G Am F F G C G twice | exact |
| Structure on it | ≥ 2 sections, covering ≥ 4 bars; a boundary at bar 4 | exact (report in *Changes* if the finder joins differently) |
| Done notes: following tempo 88 to 95; five sections | "from 88 to 95 BPM"; four names and "and 1 more" | exact |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `gen::exercises` (`src/generate/Exercises.{h,cpp}`, both CMake targets): the exercise list, step descriptors with one or two links, the context, and done / do text / action / status per step; First song reached through it. Tests: the First song checks give the same answers through it; every link of every exercise names an existing topic; settings stems differ. Done when the tests pass.
- [x] 2. `gen::songchords` (`src/generate/SongChords.{h,cpp}`): texts from *Theory*, song clip and span, the four rules with baseline and counter, the swap suggestion, the example recording. Tests: the *Test numbers* rows. Done when they pass.
- [x] 3. The *Playing along* sentence in `PlayAlong.cpp`. Tests: the chord guide text contains "Learn the Chords of a Song". Done when `./build.sh --test` passes without warnings.

Group B1, UI: shared window and pointer (`./build.sh --test` at step 5):
- [x] 4. Rename `FirstSongExercise` to `ExerciseWindow` (`src/ui/ExerciseWindow.{h,cpp}`) running any exercise: link chips, status line, per-exercise carry-on, settings by stem; `MainComponent` keeps one `SafePointer` per exercise and dispatches by exercise; update `InspectorPanel`, `TheoryGuidesView`, `ScreenshotRunner` uses. Check: the `firstSong` shots look as before.
- [x] 5. `GuidePointer` optional area; getters for the song clip's bounds in the lanes, a bar's bounds on the Chords row, the Chords header's *Play along* button and whether Play along is showing. Done when `./build.sh --test` passes without warnings.

Group B2, UI: the song chords exercise (`./build.sh --test` at step 7):
- [x] 6. Help > *Learn the Chords of a Song...* and the clip menu item; actions, *Show me* and prerequisites per the table; the 4 Hz counter while Play along shows; baselines (step 4 first opened; `ChordDetector::onChordsSet` also tells `MainComponent`); reset in `afterReplace` and *Start over*. Check: by hand on a song with a recording.
- [x] 7. `ScreenshotRunner`: scene `exercise` (`"exercise": "firstSong"|"songChords"`, `"exerciseStep"`), keys `"exerciseAction"`, `"exerciseShowMe"`, `"exercisePlayedAlong": n`, `"exerciseNoOutput"`; scene `songRecording` (audio track "Song" with the example recording at the song's tempo) and key `"detectChordsNow"` (puts the detected chords on the chord track with no dialog); keep `firstSong`, `firstSongStep`, `firstSongAction` as aliases. Done when `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 8. Real-app check, `"song": "empty"`, `"capture": "window"` for the exercise: songChords at step 1 with no recording, then `songRecording`; step 2 before and after `detectChordsNow` (ticked; C G Am F twice on the row; "8 bars of Song have a chord"); step 3 with no chords, with `exerciseNoOutput`, at `exercisePlayedAlong` 2 and 4; step 4 with `exerciseShowMe` (pointer on bar 1) and after a chord change; completion; `exerciseShowMe` on steps 1 to 3 (Import tile, the clip outline, the Play along button); the clip menu item; the four `firstSong` shots unchanged. Look for cut text, a pointer covering its target, the second link chip fitting. By hand: Detect chords from step 2 through the dialog, Detect tempo first, play 4 bars with Play along open.
- [x] 9. Docs: README *Features* bullet (or extend the first-song bullet: exercises, this one); `docs/tests.md` (exercises, songchords); `docs/code-layout.md` (Exercises, SongChords, ExerciseWindow replaces FirstSongExercise); `docs/recipes.md` new *A guided exercise* (model, window, settings stem, links test, screenshot keys) and the *Checking in the real app* keys; `docs/ui-design.md` and `docs/architecture.md` (the window's new name, status line, link chips, pointer area). GitHub: #196 progress; tick #195's "shared step model" item.
- [x] 10. #209: a sixth *Learn from a song you like* tile in `StartPanel` (`Choice::learnSong`), opening `showExercise (songChords)`; checked at 1440 x 900 and 960 x 600 and that step 1's *Show me* still outlines the Import tile.

**Amendment: tempo and sections steps.** Each group is a fresh dawImplement run; read *Amendment* under *What the user does and sees*, *Code* and *Theory* first.

Group D, engine (`./build.sh --test` at step 13):
- [x] 11. `gen::songchords` and `gen::exercises` to six named steps: renumber every use, new step texts, links and done notes from *Amendment texts*, `Fix` values for detect tempo and detect song structure, the tempo record and "no clear beat" in `Context`, the step 2 and step 4 rules, statuses and tips, `clampStep` 0 to 5, and the pure step migration with the version key. Tests: the *Amendment test numbers* rows for steps, migration, rules, tips and done notes; the old step tests under their new numbers.
- [x] 12. The example song with a beat (*Amendment example data*) beside `makeExampleRecording`. Tests: its length; tempo at 96 and 120 BPM; first downbeat; chords; structure (the table's rows).
- [x] 13. Help description string moves into `gen::songchords` (or `exercises::Info`) so a test checks it says "six steps"; done when `./build.sh --test` passes without warnings and First song's checks are unchanged.

Group E, UI (`./build.sh --test` at step 16):
- [x] 14. Detector reports and wiring: `TempoDetector` reports the clip it set the tempo from and a clip with no clear beat; `ChordDetector` and `StructureDetector` report the clip and whether it lined up when they write; `ArrangementView` forwards them and gains detect-tempo and detect-structure entry points, a `SectionLane` getter and a bar-range area on it. `MainComponent` keeps the tempo record (cleared in `afterReplace` only) and fills `Context`. Check: by hand, Detect tempo on a recording ticks step 2; moving the clip unticks it.
- [x] 15. `MainComponent`'s song chords actions and *Show me* for six steps (`runSongChordsAction`, `showSongChordsTarget`), the new `Fix` values; `ExerciseWindow` at 360 × 740 for song chords and reading the step through the migration (writing the version key); the Help description. Check: an old `learn.songChords.step` of 2 opens at *Play along* (5th row) on scratch settings.
- [x] 16. `ScreenshotRunner`: the `songRecording` scene can use the beat example, at its own tempo and an offset (so tempo detection has work to do); `detectChordsNow` analyses the song clip's own audio (it now regenerates the 8-bar example); new keys `detectTempoNow` (sets the tempo and moves the clip as *Set tempo* does, then records it) and `detectSectionsNow` (puts the found sections on the row as *Put on Sections row* does); `exerciseStep` takes 0 to 5. Update `docs/recipes.md` *Checking in the real app* keys. Done when `./build.sh --test` passes without warnings.

Group F, check and docs:
- [x] 17. Real-app check, `"song": "empty"`, `"capture": "window"` for the exercise and `"main"` where the row matters: beat example at 96 BPM on a 120 BPM song, step 2 before (`exerciseShowMe`: clip outline) and after `detectTempoNow` (ticked, "The song is at 96 BPM"); step 3 with the tip, then after `detectChordsNow`; step 4 before (`exerciseShowMe`), after `detectSectionsNow` (Sections row filled, done note) and its *Show me* on the row; step 4 on the 8-bar example (short-span line); step 2 with no recording; steps 5 and 6 as before under their new numbers; completion; the four `firstSong` shots unchanged; start panel unchanged. Look for cut text, six rows and "N of 6 done" fitting at 740, the pointer not covering its target. By hand: Detect tempo and Detect song structure from the window through their dialogs; Undo after Set tempo unticks step 2.
- [x] 18. Docs: README's exercise sentence (tempo, chords and sections; *Show me* targets); `docs/tests.md` (songchords: tempo record, sections rule, migration, beat example); `docs/code-layout.md` (SongChords line); `docs/ui-design.md` (Exercise window 740, six rows); `docs/recipes.md` *A guided exercise*: renumbering steps needs a settings migration; GitHub #196 progress comment (Done / Still open), plan `Status` per the workflow.

## Tests

`runSongChordsTests()` in `tests/EngineTests.cpp`: the *Test numbers* rows on made-up edits (an audio track with a silent clip of the given length for steps 1 to 4; the example recording through `chordfinder::analyse` for detection); the song clip is the longest unmuted one; every text non-empty and placeholders filled for C major and A minor. In `runFirstSongTests()`: the same checks through `gen::exercises`, and every exercise's links resolve in `getEntries()`. In the chord guide tests: the *Playing along* sentence.

Amendment, in `runSongChordsTests()`: the *Amendment test numbers* rows (made-up edits with a silent clip and a set or changed tempo map for step 2; sections lists for step 4; the beat example through `tempofinder::analyse`, `chordfinder::analyse` and `structurefinder::analyse`); every new text non-empty with placeholders filled; the Help description says "six steps"; First song's checks unchanged.

## Website hand-off

- `features.html`, start panel: six tiles now, the last *Learn from a song you like*, opening the song chords exercise (screenshot of the panel at 1440 x 900 from `"song": "empty"`).
- `features.html`, learning group: guided exercises, now two; this one with a song the user likes.
- `learn.html`: beside the First song exercise, "Learn the chords of a song".
- `for-you.html`: worth a line for people learning an instrument, if the user agrees.
- Screenshots: the exercise at step 2 beside the song clip and the Chords row with C G Am F (`songRecording`, `detectChordsNow`); Play along open with the exercise at step 3. No `about.html` change.
- Amendment: `features.html` and `learn.html` texts say six steps (tempo, chords, sections, play along, swap); a new shot of step 4 ticked with the Sections row and Chords row filled over the beat example (from the Group F list) can replace or join the step 2 shot. No `about.html` change.

## Left for later

- Checking what the user plays (MIDI input or microphone against the chord): #196's open item, after this.
- A list of all exercises: #195. #209's tile is built here and opens this exercise; its menu of the other song exercises and its own file chooser wait for those exercises.
- More than one chord per bar and sus, diminished and slash chords in detection: #102 and the chord finder.
- Played-along progress kept across restarts: only if users ask.
- Splitting stems to hear the chords better (needs the Demucs model): a later step for #199.
- Amendment: the tempo record kept across restarts (decision 10 (b)); a tap-tempo button for songs with no clear beat; lyrics as a step (Detect lyrics needs the whisper model): only if users ask.

## Changes while building

### Group A hand-off (engine)

- `src/generate/SongChords.{h,cpp}` (`gen::songchords`, in both CMake targets): `getSteps`, `getTitle/Intro/CompletionText`; `findSongClip`, `getClipBars`, `getSpan` (whole bars: `firstBar`, `endBar` exclusive), `countChords`, `getClipName`; `isStep1Done`, `isStep2Done`, `isStep3Done (playedBars)`, `isStep4Done (engine, baseline)`, `isStepDone`; `PlayedAlongCounter` (`feed (bar, hasChord, playing, windowOpen)`, `getCount`, `reset`); `Swap` with `getSwapFor`, `getSwap (engine, baseline)` (taken from the baseline's first chord in the span, so it stays put after a swap); `getDoText`, `getWhy`, `getActionLabel/Tooltip`, `getStatus` (text plus `Fix` none / importAudio / detectChords / audioSettings), `getDoneNote`; `makeExampleRecording (rate, bpm)` (mono, C G Am F twice).
- `src/generate/Exercises.{h,cpp}` (`gen::exercises`): `Exercise { firstSong, songChords }`, `allExercises`, `getInfo` (settings stem, title, intro, completion, carry-on label/tooltip, `carryOnOpensStartHere` or `carryOnLink`, steps with a `links` vector), `getStepSettingKey/getDoneSettingKey`, `clampStep`, `getAllLinks`, and per step `isStepDone`, `getDoText`, `getWhy`, `getAction` (label, tooltip, `Fix`; the prerequisite fix replaces the step's own action), `getStatus`, `getDoneNote`, all taking `(Exercise, step, const Engine&, const Context&)`. `Context` holds `saved` (First song step 4), `baseline` (First song step 2 and chord step 4), `playedAlongBars`, `hasOutputDevice`. First song's content is still in `gen::firstsong`, unchanged; `firstsong::stepSettingKey/doneSettingKey` equal the exercises keys.
- UI to do for the `Fix` values: `importAudio` runs `importAudioCommand`, `detectChords` opens the detect-chords dialog on the song clip, `audioSettings` opens Audio/MIDI Settings. Step 1's status is shown with `Fix::none`. Step 3's status shows "Played along: n of 4 bars." while counting.
- The *Playing along* sentence is added in `PlayAlong.cpp` (`explainPlayingAlong`, after the tips, in all cases).
- Departures: no-recording status for steps 3 and 4 reuses step 2's text ("Detect chords needs a recording of at least 4 bars: step 1 brings one in."), since the plan gave no separate text. The status's bar count is rounded down to one decimal (3.95 reads 3.9). A swap is offered for major chords (relative minor) and for minor ones that are not diminished or augmented; sus, diminished and augmented give no suggestion. New tests are in `runSongChordsTests()` (it also holds the First song through-the-model checks, not `runFirstSongTests()`).

### Group B1 hand-off (shared window and pointer)

- `src/ui/ExerciseWindow.{h,cpp}` replaces `FirstSongExercise` (CMake updated). `ExerciseWindow (engine, selection, Exercise, std::function<Context()>)`; callbacks `onAction (step, Fix)`, `onShowMe (step)`, `onOpenLink (Link)`, `onCarryOn`, `onStartOver`, `onClose`; `getExercise()`, `getStep()`, `setStep`, `getActionButton()`, `startOver()`. Two link chips (side by side if they fit), status line (`textDim` for the chord exercise, yellow for First song's save note as before), a green done note from `getDoneNote`, carry-on button text and tooltip from `Info`, settings via `getStepSettingKey/getDoneSettingKey`.
- `MainComponent`: `showExercise (Exercise)` (`showFirstSongExercise()` calls it), `getExerciseWindow (Exercise)`, `exerciseWindows[numExercises]` (SafePointers, keyed in `PluginWindowManager`), `getExerciseContext()` (saved state and baseline filled; `playedAlongBars` 0 and `hasOutputDevice` true are for B2 to fill), `openExerciseLink`. `runExerciseAction (Exercise, step, Fix)` and `showExerciseTarget (Exercise, step)` return at once for `songChords`: B2 fills them (the `Fix` values per the Group A hand-off). `onStartOver` only resets `exerciseChordBaseline`; B2 adds the played-along reset. No Help item or clip menu item yet.
- Pointer: `GuidePointer::show (findTarget, findArea, Options)` (area in the target's coordinates, asked on each follow; empty outlines the whole target). Getters: `TrackLanes::getClipArea (const Clip&)`, `ChordLane::getBarArea (bar)`, `ArrangementView::getLanes()`, `getPlayAlongButton()`, `isPlayAlongShowing()`.
- `ScreenshotRunner` uses `getExerciseWindow (firstSong)`; the `firstSong`, `firstSongStep`, `firstSongAction` keys work as before (checked: the step 2 shot looks as before). B2 adds the new scene keys and keeps these as aliases.

### Group B2 hand-off (UI: the song chords exercise)

- `MainComponent`: command `songChordsExerciseCommand` (Help, after First Song Exercise, no shortcut), `showSongChordsExercise()`, `runSongChordsAction (step, fix)` and `showSongChordsTarget (step)` (called from `runExerciseAction` / `showExerciseTarget`), `getExerciseContext (which)` (fills `playedAlongBars` and `hasOutputDevice`), own `songChordsBaseline` (separate from First song's `exerciseChordBaseline`; set when the window first opens, in `afterReplace`, on Start over and when `ArrangementView::onChordsDetected` fires), and `playedAlong` (`PlayedAlongCounter`, fed by `feedPlayedAlong()` from the existing 5 Hz `timerCallback` while the transport plays and Play along shows; counts only bars with a chord inside the song clip's span; reset with the baseline). Departure: 5 Hz rather than 4 Hz (reused the timer; a bar still lasts at least 1 s).
- `ArrangementView`: `detectChords (Track&, AudioClip&)`, `onChordsDetected`, `onLearnChords`; `TrackLanes::onLearnChords` and clip menu item *Learn this song's chords...* (id 19, after *Detect chords...*, always enabled).
- Screenshots: `"exercise": "firstSong"|"songChords"` is a key (works with any scene) with `exerciseStep`, `exercisePlayedAlong`, `exerciseNoOutput`, `exerciseAction`, `exerciseShowMe`; scene `songRecording` (track "Song", example recording at the song's tempo) and key `detectChordsNow`. `firstSong`, `firstSongStep`, `firstSongAction` unchanged. Order within a shot: scene, detectChordsNow, exercise window, then step, action, show me. Checked one shot (songChords step 2 after detectChordsNow: step 1 and 2 ticked, texts fit).
- Not yet checked by eye (Group C): pointer shots, the clip menu item, the by-hand dialog and play-along runs.

### Group C hand-off (real-app check and docs)

- Text change: step 1's *Do* text no longer says streaming-app songs can't be imported (unverified). It now reads "choose a song file on your computer, such as an MP3 or WAV file, or drag the file onto the tracks", with the sentence removed in `SongChords.cpp` and in this plan's *Texts*.
- Fix found by eye: the song chords window (500 high) pushed the action and *Show me* buttons out of view when step 3 had a status line (the viewport scrolled). `ExerciseWindow` is now 360 x 680 for song chords (`tallWindowHeight`); First song stays 360 x 500 (its shot looks as before).
- Checked by eye (shots in the scratchpad `shots/`): steps 1 to 4 windows, no recording, after `detectChordsNow` (8 bars have a chord, C G Am F twice), no output, played along 2 and 4, *Show me* for the Import tile, the clip outline, Play along button and bar 1 ("click it and choose Am"), First song step 1 unchanged, start panel. Not checked: the completion state, the clip menu item by eye, and by-hand detect-chords via the dialog and a real 4-bar play-along (needs a person with audio). Known gap: the pointer outlines a clip that is scrolled out of view without scrolling to it.
- Docs done: README bullet (extends the first-song bullet), `docs/tests.md`, `docs/code-layout.md`, `docs/ui-design.md`, `docs/architecture.md`, `docs/recipes.md` (new *A guided exercise* and screenshot keys). First steps: the exercise window rename touched no start-panel or First song text; no change.

### Start panel tile hand-off (#209)

- `StartPanel::Choice::learnSong` (last tile; title "Learn from a song you like", subtitle "Find its chords and play along"), handled in `MainComponent::handleStartChoice` with `showExercise (songChords)`. The exercise's step 1 does the file choosing, so the tile has no chooser or menu of its own yet (#209's menu waits for the other song exercises). Layout: 6 across down to about 792 px, then two rows of 3. Docs: README bullet, `ui-design.md`. No new tests (UI only, checked by screenshots).

### Group D hand-off (engine, six steps)

- `gen::songchords` is six steps (constants `stepImport`, `stepTempo`, `stepChords`, `stepSections`, `stepPlayAlong`, `stepSwap` = 0 to 5). New: `TempoRecord` (`clip` pointer for identity, `startBeat`, `tempoMap`, `noClearBeat`), `makeTempoRecord (engine, clip)` (empty unless `clip` is the song clip) and `makeNoBeatRecord`; `Progress` (baseline, playedAlongBars, hasOutputDevice, tempo; a constructor, so `{}` works) replaces the loose parameters: `isStepDone (step, engine, progress)`, `getStatus (step, engine, progress)`, `getDoneNote (step, engine, progress)`. Rules: `isStepTempoDone (engine, record)`, `isStepChordsDone`, `isStepSectionsDone` (`countSectionBars`), `isStepPlayAlongDone`, `isStepSwapDone`. `migrateStep (stored, version)` and `stepsVersion = 2`; `getHelpDescription()`; `makeExampleSongWithBeat (rate, bpm)`.
- `Fix` gains `detectTempo` and `detectSections`. `exercises::getAction` returns them as the *own* action of steps 2 and 4 (`Fix::none` for the other own actions as before); statuses never return them. `exercises::Context` gains `tempo` (the record). `exercises::getStepsVersionKey` ("learn.songChords.stepsVersion"), `getStepsVersion`, `migrateStep (Exercise, stored, version)`; `Info::helpDescription` (MainComponent's Help item now uses `gen::songchords::getHelpDescription()`).
- UI NOT yet updated (Group E): `runSongChordsAction` / `showSongChordsTarget` / `feedPlayedAlong` / `ExerciseWindow` still use the old four-step numbers and ignore the new Fix values; `ExerciseWindow` must read the step through `migrateStep` and write the version key; the tempo record is not yet kept or filled in `MainComponent`.
- Departures: when both the tip and the short-span line apply to step 4 they are joined in one status text. The step 2 own-action tooltip, step 3 and 4 tooltips follow *Amendment texts*. Links: step 2 *Tempo and time* (rhythm), step 3 both as before, step 4 Song structure *How songs are built*. Step 5 and 6 no-chords line says "(step 3)"; their no-recording line reuses "Detect chords needs a recording...".
- The 16-bar example detects as planned except one point: tempo 96.00 and 120.00 BPM, chords C G Am F F G C G twice, structure at least 2 sections with a boundary at bar 4 all pass; the kick did not disturb the chords (no ticks-only fallback). The finder's `firstDownbeat` is beat 4 (2.496 s at 96 BPM), i.e. a bar line but one bar after the clip start, not within a third of a beat of 0; the test accepts any bar line within a third of a beat (bars 0, 1 or 2). Group E's `detectTempoNow` will therefore move the clip when *Set tempo* is applied, as the dialog does.

### Group E hand-off (UI, six steps)

- Detector reports: `TempoDetector::onTempoSetFromClip (const AudioClip&)` (after Set tempo and the clip move) and `onNoClearBeat (const AudioClip&)`; `ChordDetector::onChordsSetFromClip` and `StructureDetector::onSectionsSetFromClip` (`const AudioClip&, bool linedUp`; `linedUp` is true only when a tempo was found and `adviseLiningUp` was empty). The older `onTempoSet`, `onChordsSet`, `onSectionsSet` stay. `ArrangementView` forwards them as `onTempoSetFromClip`, `onNoClearBeat`, `onDetectionLinedUp`, and has `detectTempo`, `detectStructure`, `getSectionLane()`; `SectionLane::getBarsArea (firstBar, endBar)`.
- `MainComponent`: `songTempoRecord` (filled by those three callbacks through `makeTempoRecord` / `makeNoBeatRecord`, put into `Context::tempo`, cleared in `afterReplace` only, so Start over keeps it). `runSongChordsAction` / `showSongChordsTarget` use the `songchords::step*` names and the new `Fix` values; step 4 *Show me* outlines the span on the Sections row when it has sections, else the clip.
- `ExerciseWindow` reads the step through `exercises::migrateStep` with the stored version and writes `learn.songChords.stepsVersion`; 360 x 740 for song chords. Not checked by hand: an old saved step of 2 opening at *Play along* (the migration is covered by the Group D tests; no run on scratch settings with a saved step).
- `ScreenshotRunner`: `songRecording` takes `songBeat`, `songBpm`, `songStartBeat`; `detectTempoNow`, `detectChordsNow` (now analyses the clip's own audio), `detectSectionsNow`, applied in that order; the last two also tell the exercise "lined up" as the dialogs do (so a lined-up result ticks step 2). `exerciseStep` takes 0 to 5. Note for Group F: `"song"` is a list-level key (a per-shot `"song": "empty"` is ignored, and the example song's chords and sections then tick steps 3 and 4). Checked by eye: beat example at 96 BPM, clip at beat 2: step 2 ticked after `detectTempoNow` (song at 96 BPM, clip moved to a bar line), six rows and "N of 6 done" fit at 740, step 4 *Show me* outlines the span on the Sections row. No existing shot list used song chords `exerciseStep` (none in the repo), so none needed updating.
- Docs done: `docs/recipes.md` shot keys and `DevScreenshots.h`. Still for Group F: the other docs and the full real-app check.

### Group F hand-off (check and docs)

- Checked with shot lists on scratch settings (`"song": "empty"`, scale 1, in the scratchpad `shots/`): beat example at 96 BPM on 120, clip at beat 2. Step 2 with no recording (needs-a-recording line, Import button); step 2 *Show me* outlines the clip; after `detectTempoNow` step 2 ticks ("The song is at 96 BPM, the tempo of Song.") and the toolbar reads 96; step 3 with the find-the-tempo tip; step 4 *Show me* on the Sections row and its done note ("Song has 4 sections..."); steps 5 and 6; completion text (`"settings": {"learn.songChords.done": true}`); an old saved step 2 with no version opens at *Play along*; `"undo": 1` after `detectTempoNow` unticks step 2 and shows the changed-since line; the 8-bar example at step 4 shows the tip joined with the short-span line. Six rows and "N of 6 done" fit at 740. The Sections pointer's caption sits over the Chords row's first bars, not over its target (the Sections row), so left as is.
- Observed: step 4 with both the tip and the short-span line makes the body scroll a little at 740 (buttons stay visible); not changed.
- Not checked by hand: Detect tempo and Detect song structure through their real dialogs from the window; the clip's right-click *Learn this song's chords...* item (wired in `ArrangementView`, unchanged by this amendment).
- Docs updated: README bullet, `docs/tests.md`, `docs/code-layout.md`, `docs/ui-design.md`, `docs/recipes.md` (migration note). `docs/architecture.md` needed no change. First steps: checked, no change.

### Theory check (amendment)

- Read Schreiber 2020 (section 2.4) and Levy 2011 (ISMIR): both confirm fact 20 (octave errors; listeners tap at different metrical levels); fact 20 now names them properly. `SOURCES.md` *Not for a guide* lists both.
- Fact 19 corrected: the "about 6 seconds" comes from `findPeriod` needing 4 of its longest beat periods (4 × 1.5 s at 40 BPM), not from 8 beats at 80 BPM. The text was right.
- Step 2 Why: "It needs no download, but it needs a clear beat..."; the last sentence now reads "A band playing without a click often drifts: when it does, Detect tempo also offers Follow the recording, which gives every bar its own tempo." (the option only appears when the tempo isn't steady; fact 22 says so).
- Step 4 short-span status: "Parts are found in phrases of 4 bars or more" was wrong (the finder allows parts of 1 to 16 bars and only prefers 4, 8 and 16); now "DawIt finds where parts start by comparing the 4 bars on each side of a bar line". The test string in `runSongChordsTests` follows. New fact 23a.
- Step 4 Do and the completion text: clicking a section opens a menu, so they now say "choose Loop this section" (and "Type" in step 4); fact 27 follows.
- Checked against the code without change: 4 bars each side, lengths 4, 8, 16 preferred, same chords and sound grouped (similarity at least 0.8), the chorus the loudest group heard at least twice (more often heard breaks near ties), half/double choices in the dialog, Set tempo moving the clip to a bar line, the Skip button and the tempo's double-click.
- Open, beyond text: `TempoFinder.h` says `analyse` gives up on "less than 4 seconds"; the code's limit is 6 s (a comment for dawImplement).

