# Guided exercise: find your vocal range and the right key

Status: Built, not committed; Theory checked, with open points
Idea: [#198 — Guided exercise: find your vocal range and the right key](https://github.com/geertvandamme/dawit/issues/198)
Part of: [#195 — Guided exercises that teach by doing in your own song](https://github.com/geertvandamme/dawit/issues/195). Uses the shared exercise model and window (`gen::exercises`, `ExerciseWindow`), like [the karaoke exercise](guided-exercise-sing-karaoke-to-a-song.md). Related: #26 (change the key), #14 (singing guide, range measurement), #28.

## Goal
Five steps in the user's own song: record a low-to-high take, keep the measured notes as your range, find the song's melody, compare it with your range and move the song to a key that suits you, then hear it in the new key. Every step ticks from the real song and settings. It joins three things a beginner would not connect alone (the range measurement, the singing guide's key advice and a key change) and teaches the habit singers use: move the song, not your voice.

## Decisions for you
1. **How the song moves.** No song-wide transpose exists (only clip octaves; #26 is open). Options: (a) build the core of #26 here: one action moves the key, the chord track and the notes on every pitched MIDI track by the same number of semitones, as one undo step; audio stays; no dialog or mode change; (b) the issue's fallback, the melody clip only; (c) wait for #26. **Recommended:** (a). (b) puts the melody in a different key from the chords and accompaniment, which teaches the wrong thing ("a key change keeps the song the same"). #26 later adds its *Change key...* dialog, any key, mode change and range warnings on the same function.
   Answer: Move the key, chords and pitched MIDI together as one undoable action.
2. **When the accompaniment is a recording.** DawIt can't pitch-shift audio. Options: (a) disable *Move the song* when heard audio plays under the melody and no other heard pitched MIDI track would move, and give the number to use elsewhere (karaoke key control, a band); (b) move the MIDI anyway after a confirm. **Recommended:** (a): nothing in the song would sound right after (b). When other MIDI does move, the button works and the status names each recording that stays in the old key.
   Answer: Disable the move when a recording is the only heard backing; otherwise name recordings left unchanged.
3. **Where the theory goes.** Options: a new singing-guide topic *Choosing a key for your voice*, sixth of 15, after *Measuring your range*; or #26's planned chord-guide topic *Changing the key*. **Recommended:** the singing topic now (it is about the voice); #26's topic, when built, covers roman numerals and the capo and links to it.
   Answer: Add the topic to the singing guide after Measuring your range.
4. **When step 4 counts as done.** Options: (a) every melody note is inside the range (the guide's verdicts *fits*, *near the top*, *near the bottom*), or an octave move fits; (b) only the verdict *fits*. **Recommended:** (a). The near-top/bottom advice is DawIt's rule of thumb (opinion); it stays as an optional nudge, and (b) could make the user chase it back and forth.
   Answer: Count all melody notes fitting, including an octave fit; keep near-range advice optional.
5. **A range set before the exercise.** Options: steps 1 and 2 tick at once, with a note that a new take measures again; or step 1 always needs a new take. **Recommended:** tick: the range is the real state; re-measuring stays one click away.
   Answer: Accept an existing range for steps 1 and 2.
6. **Where it opens.** Options: Help > *Find Your Range and Key...* only; also the audio clip menu (*Find my range and key...* after *Measure singing range...*). **Recommended:** both, like the karaoke exercise's Help item and clip item. An exercise list stays in #195.
   Answer: Open from Help and the audio clip menu.

## What the user does and sees
**Opening.** Help > *Find Your Range and Key...* (after *Map a Song's Structure...*); an audio clip's right-click *Find my range and key...* after *Measure singing range...* (opens the exercise only). The shared window, titled "Find your range and the right key", five rows, Back / Skip / Next, completion as in the others; height started at 700 and is 620, confirmed by shot at the tallest steps.

**The melody** is the singing guide's chosen melody track when its window is open and the user picked one, else `singing::chooseMelodyTrack` (selected track first). The exercise and *Your voice and your song* must always name the same track.

**The range take** is the newest clip the last recording made this session (reuse `karaoke::makeTakeRecord`), still in the edit and at least 3 s long.

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Record your range | a range take exists, or a range is set (`singing.lowest/highest`, `Range::isSet`) | *Record*: armed audio track: start recording; none: *Add a recording track*; unarmed: disabled, status "Press R on 'Voice' to arm it" (reuse `karaoke::findRecordTarget`) | the track's R, else Record |
| 2 Keep it as your range | a range is set | *Measure this take* (`RangeMeasurer` on the take; the guide opens at *Measuring your range*, where *Use as my range* sets it). Without a take: *Set it by hand...* opens *Voice types and range* | outlines the take |
| 3 Find the song's melody | the melody track has ≥ 4 notes | when there is no melody but an audio clip on a vocal-word track (`singing::hasVocalWord`, or named Vocals) other than the take: *Convert to MIDI...* on it (the existing dialog, which asks before a model download); else none | the melody track's header, else that clip |
| 4 Choose a key that suits you | decision 4 | *Move the song N semitones lower/higher* with the analysis' shift (not shown for an octave or no shift) | the key display in the transport bar |
| 5 Hear it in the new key | since step 4 was last done or the song last moved, the song played ≥ 4 different bars that hold melody notes (all of them if fewer), melody track heard, not recording | *Play from bar N* (first melody bar) | Play |

**Statuses (ways forward):** no input device: "No microphone found: choose an input in Audio settings." (*Audio settings...*); a measurement whose range differs from the one set: "DawIt measured C3 to G4 in 'Take 1'. Click Use as my range in the guide to keep it."; no melody and nothing to convert: "Write the tune on a MIDI track, or record yourself singing it and choose Convert to MIDI... in the clip's menu."; verdict *no key fits*: the guide's paragraph, Skip offered; move blocked by audio (decision 2): "Your backing is a recording, which DawIt can't move yet. To sing it in C major, set a karaoke player's key control to −4, or ask your band for it 4 semitones lower."; a note would leave MIDI notes 0 to 127: "A note on 'Bass' would go below the lowest MIDI note."

**Optional nudge:** with step 4 done on *near the top/bottom* and a shift, the done note adds "Optional: 2 semitones lower would be easier on your top notes." and the action stays; it is never offered opposite to the exercise's own last move.

**Tooltips:** Record "Record yourself on the armed track: slide down to your lowest comfortable note and hold it, then up to your highest and hold it."; Measure this take "Find the notes you held in the take. Nothing in the song changes."; Move the song "Move the key, the chords and every MIDI note except drums by N semitones, as one step you can undo. Recordings stay as they are."; Play from bar N "Play the song from the first bar of the melody, to hear and sing it in the new key."

**Remembered:** `learn.rangeKey.done` (settings stem `learn.rangeKey`); the step only in memory (`StepMemory`). The range stays in its two existing settings.

## Data and saving
No new project data. The move changes existing data (key, chords, notes), saved as before. Nothing for older projects.

## The guide
New topic in `gen::singing`, sixth of 15 (after *Measuring your range*; *Your voice and your song* stays last): **Choosing a key for your voice** — *Move the song, not your voice: every note the same distance*.
- **Picture:** three horizontal note-range bars on a keyboard axis: the singer's range, the melody before (the part outside the range in red) and after the move, each labelled with its notes and key. With a range set and a melody in the song, it shows the song's own bars and the analysis' suggested shift, and the text adds "In your song: " plus the analysis' first paragraph; otherwise the example (Example data) with the caption "An example: a melody too high for a baritone".
- Click a bar end to hear that note (`Engine::previewNote`, 600 ms), as the other singing pictures.
- **Links** (`TheoryGuideLinks.cpp`): singing · Choosing a key for your voice | keyfinding · What a key is; singing · Choosing a key for your voice | instruments · the bass topic (the E1 floor).
- **`gen::guides`:** the singing entry's topic list grows by itself; add search words "transpose, change key, key for my voice, karaoke key, too high, too low". Not on *Start here* (no change).
- Every existing "topic number" or `allTopics` assumption in the singing guide and its tests moves with the new topic.

## Code
- Follows recipes.md *A guide* (new topic in an existing guide) and the exercise pattern of the karaoke exercise (step content in its own `gen` namespace, behind `gen::exercises`; window, Help command, actions and Show me in `MainComponent`; `ScreenshotRunner` scenes).
- **The song move** (generate, UI-free, tested): moves `MusicalKey::tonic` by n (mod 12, scale unchanged), every chord root (mod 12) and every note on each MIDI track that isn't a drum track (`firstsong::isDrumTrack`) by n. All or nothing: if any note would leave 0 to 127, nothing changes. Audio clips, sections, tempo, instruments and plugin settings are untouched. One undo step named "Move the song 4 semitones lower (E major to C major)". Message thread, under the lock, like any edit. Anything that follows the song key (scale filter, pictures) must follow by itself: check it. #26 will reuse this function.
- **Blocked check** (pure): the heard audio clips overlapping the melody's bars, excluding the range take; blocked when there is at least one and no other heard non-drum MIDI track has notes.
- **Exercise records in `MainComponent`** (in memory, cleared on a new or opened project): the range take, the last measurement, the exercise's last move (shift and the undo step count after it, forgotten when the key no longer matches), the heard-bars counter (a `karaoke::ListenCounter`-like counter fed at 5 Hz, reset on a move).
- The comparison is `singing::analyse` on `singthis::buildMelody` of the melody track, never a second rule set.
- **Risky:** the new topic shifts the singing guide's topic indices (screenshot lists, `showTopic` callers, tests that count 14 topics).

## Libraries and models
None. Convert to MIDI uses the existing Basic Pitch model and its existing download dialog. No licence change.

## Theory

**Facts**
1. Transposing moves every note by the same number of semitones; the intervals, and so the tune and the chord qualities, stay the same. Exact (definition).
2. 12 semitones make an octave; a note an octave away has the same name. Exact.
3. The new key's tonic is the old one plus n, mod 12: E major − 4 = C major. Derived (4 − 4 = 0 = C).
4. A song can be sung when its lowest and highest notes lie inside the singer's range; the key is per singer and song, there is no one right key for a voice. Typical/opinion. audiomasterclass.com (q-what-key-should-i-sing-in).
5. Comfort depends on where most notes sit (tessitura), not only on the extremes; long stretches of high singing tire more than a few high notes. Typical. audiomasterclass.com; dummies.com (choosing the key for your audition song).
6. Singers often lower a key for live singing, professionals too. Typical. 30daysinger.com (choosing-a-key).
7. Karaoke players' key controls move in semitone steps, often 6 or 7 either way. Typical. Sony MHC-VX5 manual (13 halftone steps); other manuals in the search.
8. Men often sing songs written for women's voices an octave lower, and the reverse. Existing singing guide (octave suggestion).
9. Lowest note of a four-string bass: E1 (MIDI 28). Existing instruments guide.
10. Drums are not moved because drum notes stand for sounds, not pitches. Exact (General MIDI drum map, already used by `isDrumTrack`).
11. DawIt's rules: shifts that fit run from (range low − melody low) to (range high − melody high); outside the range, the closest shift, or an octave when that shift is 5 or more and the octave fits; near the top/bottom when ≥ 25% of the sung time is in the top/bottom 3 semitones. Opinion (DawIt's rule of thumb, as in `singing::analyse`; unchanged).
12. Low notes are quiet and easily lost in a mix. Existing *Your voice and your song* text.
13. E major is an easy guitar key; E♭ major mostly needs barre chords. Typical (common guitar knowledge).

No disagreement with existing guides. The new text uses the analysis' numbers and words.

**Topic: Choosing a key for your voice** — *Move the song, not your voice: every note the same distance*

If a song is too high or too low for you, don't change your voice: change the song. Moving a song to another key (transposing) moves every note up or down by the same number of semitones. The steps between the notes stay the same, so the tune and the chords are the same song, only higher or lower. A semitone is one key on the piano, and 12 semitones make an octave.

An example: a melody runs from E3 to A4 in E major, and a singer's range is A2 to F4. The top of the melody is 4 semitones above the singer's highest note. Moved down 4 semitones, the song is in C major and the melody runs from C3 to F4, inside the range. The picture shows both.

How to choose a key:
1. Check the edges. The melody's lowest and highest notes must fit inside your range. If the melody spans more semitones than your range, no key fits all of it.
2. Check where it sits. What tires a voice is singing near the top for a long time, more than one high note. Aim for most of the melody, especially the chorus you sing again and again, in the comfortable middle of your range. Your voice and your song warns when a quarter or more of the sung time is in the top or bottom 3 semitones of your range. That's DawIt's rule of thumb, not a standard.
3. Leave a little room at the top. A tired, cold or nervous voice reaches less, and many singers, professionals too, sing a little lower live than on the record.
4. Try it. Sing the song in two or three neighbouring keys and keep the one where you sound most like yourself. Lower isn't always better: low notes are quiet and easily lost in a mix.

There's no one right key for a voice: it depends on the song. A key that suits you in one song can be too high in the next.

An octave is a special move: the notes keep their names, so nothing in the song has to change. You just sing it lower or higher. Many men sing songs written for women's voices an octave lower, and the other way round.

What DawIt moves: the key, the chord track and the notes on every MIDI track, except drums, whose notes stand for sounds, not pitches. Recordings can't follow yet: DawIt can't change the pitch of audio. With a recorded backing, use the number elsewhere: karaoke players and apps have a key control that moves a song in semitone steps, often up to 6 or 7 either way, and a band can play it "2 semitones lower".

A move can make other parts harder. A bass line moved down may go below E1, the lowest note of a four-string bass, and guitar chords can get harder: E major is an easy guitar key, E♭ major mostly needs barre chords. Listen to the whole song after a move.

This topic doesn't change the mode: the song stays major or minor. Turning a happy song sad (C major to C minor) is a different change.

**Exercise texts**
- Title: "Find your range and the right key". Help description: "Five steps: measure your voice, compare it with your melody and move the song to a key that suits you".
- Intro: "Find out which notes you can sing, check them against your song's melody, and move the song so it fits your voice. You need a microphone, and a melody on a MIDI track (or a recording of it to turn into notes)."
- 1 *Record your range* — "Low and high, held for 2 seconds". Why: "DawIt can only fit the song to your voice once it knows your notes. Warm up first: a warmed-up voice reaches further, more safely." Links: singing · Measuring your range; singing · Warming up. Done note: "Take 1: 9.2 s." / "Your range is already set. Record a new take to measure again."
- 2 *Keep it as your range* — "Measure the take and use it". Why: "DawIt finds the notes you held steady and names the closest voice type. Check by ear that it heard the right octave before you keep it." Links: singing · Measuring your range; singing · Voice types and range. Done note: "Your range: A2 to F4, 20 semitones (closest: baritone)."
- 3 *Find the song's melody* — "The tune you'll sing, as notes". Why: "DawIt compares notes, so the tune has to be on a MIDI track. A recording of you singing it can be turned into notes with Convert to MIDI..." Link: singing · Your voice and your song. Done note: "The melody on 'Lead': E3 to A4, 17 semitones."
- 4 *Choose a key that suits you* — "Move the song, not your voice". Do text: the analysis' first paragraph. Why: "Moving every note by the same number of semitones keeps the song the same, only higher or lower." Links: singing · Choosing a key for your voice; singing · Your voice and your song. Done notes: fits: "Your melody fits: 2 semitones to spare at the top." / octave: "Sing it an octave lower: nothing in the song has to change."
- 5 *Hear it in the new key* — "Play it and sing along". Why: "A key that looks right on paper can still feel wrong. Sing a chorus: if the top notes strain, try a semitone lower." Links: singing · Choosing a key for your voice; singing · Singing in tune. Done note: "You heard 4 bars in C major."
- Completion: "Your song now sits in your voice. Your range is saved for every song, so Your voice and your song checks each new melody against it. Measure again in a few weeks: a beginner's range often grows." Carry-on: *Open Your voice and your song*.

**Example data** (picture): range A2–F4 (45–65); melody before E3–A4 (52–69), E major, 4 semitones above (69 − 65); shift −4 (closest shift 65 − 69; minimum 45 − 52 = −7; |−4| < 5, so no octave); after C3–F4 (48–65), C major.

**Test numbers**

| Check | Expected |
|---|---|
| Move E major, chord E, note 52 by −4 | C major, chord C, note 48 |
| Move A minor by +3 / B major by +2 | tonic 0 minor / tonic 1 major (named as `MusicalKey::getName` names it), scale kept |
| Note 2 moved −3 | refused, song unchanged (key, chords, notes) |
| Drum track note 36, move −4 | stays 36 |
| Move then undo | one step; key, chords, notes as before; step name has "4 semitones lower (E major to C major)" |
| Example analysis (range 45–65, melody 52–69) | verdict too high, outside 4, shift −4, new 48–65, "C major" |
| Step 4 done, melody 48–65 in range 45–65 | true (near top counts) |
| Step 4, melody 50–74 in range 45–65 | false: span 24 > 20, verdict no key, no move action, Skip offered |
| Step 4, melody 64–81, range 45–72 | octave fits (shift −12 → 52–69): done, octave note |
| Blocked: heard audio under melody, only melody MIDI | blocked; with a heard piano MIDI track: not blocked |
| Step 5 counter | 4 different melody bars after the move; repeats count once; reset on a move |
| Text | the topic has text and an example; search "transpose" finds it; every exercise link resolves |

## Steps

Group A, engine and content (`./build.sh --test` at step 4):
- [x] 1. The song move and the blocked check in generate (Code). Tests: the move rows of the table. Done when: tests pass.
- [x] 2. The new singing topic: name, summary, text, example data, "In your song" paragraph; fix indices and topic counts; links; search words; SOURCES.md is already updated. Tests: text, example, links, search. Check: `--export-guides` lists 15 singing topics.
- [x] 3. The exercise content in its own `gen` namespace and its entry in `gen::exercises` (steps, links, done rules, do texts, actions, statuses, done notes, nudge rule, `numExercises` + 1, height 700). New context fields (Code). Tests: each step's done/not-done case and status, the example analysis, octave and no-key cases, links resolve, `StepMemory`.
- [x] 4. The heard-bars counter. Tests: counter rows. `./build.sh --test`.

Group B, UI (`./build.sh --test` at step 8):
- [x] 5. Help command *Find Your Range and Key...* and the audio clip item, opening the shared window; `getExerciseContext` fills the new fields; the records kept in `MainComponent`, cleared on a new or opened project and after an undo that removes the move.
- [x] 6. Actions: Record (karaoke's path), *Add a recording track*, *Measure this take*, *Set it by hand...*, *Convert to MIDI...* on the chosen clip, *Move the song* (with the status line for recordings left behind), *Play from bar N*, *Audio settings...*. Show me targets per the table. The melody choice shared with the singing window.
- [x] 7. The topic's picture in the singing window (example and your-song modes, click to hear).
- [x] 8. `ScreenshotRunner` scenes: a range take, a set range, a MIDI melody too high (the example's numbers), an audio backing, `"exercise": "rangeKey"` with step/action/Show me keys, as for the others. `./build.sh --test`.

Group C, check and docs:
- [x] 9. Real-app check (`--screenshots`): each step before and done; step 1 with no input and unarmed; step 2 with a differing measurement; step 3 offering Convert to MIDI; step 4 too high (button "Move the song 4 semitones lower"), after the move (key display "C major", piano roll notes down 4), octave case, blocked by audio; step 5 done; completion; the new topic in both picture modes; the window height at the tallest step; Show me for steps 1, 4, 5.
- [x] 10. Docs: README Features bullet (exercises list and singing guide topic); docs/tests.md; docs/code-layout.md; recipes.md only if the exercise wiring changed; issue #198 progress and #195's checklist; note in #26 that the song move exists for its dialog. First steps: no change (a separate Help exercise; nothing the Start panel, *Get started* or First Song Exercise names changes).

## Tests
The *Test numbers* table, plus: the move never changes a track's instrument; audio clips' positions and lengths are unchanged; the exercise and the singing guide pick the same melody track for the same song and selection; every exercise link names an existing topic.

## Website hand-off
- `features.html`, the exercises group (with karaoke and the chord exercise): one short section; `learn.html`: the singing guide's new topic in its list.
- Screenshots: the exercise at step 4 with the too-high example and the move button; the new topic's picture.
- No `about.html` change (no library).

## Left for later
- #26: the *Change key...* dialog (any key, ±6, mode change, per-track ticks, range warnings) on this plan's move.
- Moving recordings needs pitch shifting (formant-preserving), not in DawIt.
- Live range measurement from the microphone without recording (#14's decision 1 (b)).
- Capo advice for guitarists (#26).
- Checking other parts' playable ranges after a move, beyond the MIDI limits.

## Changes while building

### Theory review (#198)
- **Checked:** new topic, example and live analysis, exercise instructions/statuses/done notes, picture data, tests and guide links against the Theory section. Example confirmed: 45–65 range, 52–69 melody, shift −4 to 48–65, E major to C major. Range width 20, melody width 17; 25% and the outer three pitch positions match the existing analysis rule. Both planned cross-guide links are appropriate and present.
- **Changed `src/generate/SingingGuide.cpp`:** defined a semitone as the step between adjacent piano keys, including black keys; softened universal vocal-comfort claims, replaced automatic happy-to-sad wording with the actual interval-pattern distinction, qualified bass E1 by standard tuning and guitar chord advice by standard tuning without a capo. Replaced the unsupported generalisation “often up to 6 or 7” with the Sony MHC-VX5's documented ±6-semitone example and told readers to check units and limits.
- **Changed `src/generate/RangeKey.cpp`:** highest *comfortable* note in the recording instruction; optional shift “may feel easier” rather than “would be easier”; completion no longer asserts that the song fits (steps can be skipped). `tests/EngineTests.cpp`: updated the exact expected optional sentence only.
- **Sources:** `SOURCES.md` now includes the accessible David Mellor article on choosing a song key, Sony's original manual on a second mirror (page 22, 13 settings including zero = −6 to +6), and Curtis (2013), whose discussion of emotion includes tempo, context and experience as well as mode. The planned 30 Day Singer article confirms that even professionals sometimes lower live keys. The old Audio Masterclass URL and ManualsLib page were inaccessible through the web reader; both existing citations are retained, with accessible alternatives added. Searches used for this review are recorded there.
- **Open point — picture logic, implementation required:** `src/ui/SingingExplainer.cpp`, `KeyPicture`/`getKeyPicture`/`paintChooseKey`, drops `analysis.octave` and `analysis.verdict`. For a too-wide melody (`noKey`, shift 0), it says “No move needed: the melody already fits your range.” For an octave-only suggestion it labels the third bar “Moved 12 semitones lower/higher” and the tooltip “After moving the song”, although the exercise correctly leaves the song unchanged and asks the singer to change octave. Carry these distinctions into the picture; show no-key advice instead of a fit claim, and label the octave bar as the sung melody. These need logic/UI changes beyond the theory review's permitted literals; no UI file changed here.
- **Validation needed after picture fixes:** screenshot the new topic with no-key and octave-lower/higher analyses, alongside the normal −4 example and already-fits case; add meaningful coverage of the picture's selected message/data. Re-run full build/tests and guide export after the implementation change. Live microphone and native Help-menu checks remain the implementation hand-off's unchecked items.
- **Review validation:** rebuilt `DawItTests` with `cmake --build build/Release --target DawItTests --parallel` and ran `build/Release/DawItTests_artefacts/Release/DawItTests`: exit 0, `All checks passed`, no compiler warnings. `git diff --check` passes. The app and guide export were not run in this review, as the theory-checker workflow forbids running the app. Test log: `/tmp/dawit-198-theory-tests.log`; build artifacts were updated. No commit or push.

### Group A hand-off (steps 1 to 4, built, not committed)
- **Song move:** `src/generate/SongMove.{h,cpp}`, namespace `gen::songmove`: `moveSong (Engine&, semitones)` (all or nothing, one undo step named by `getStepName`), `check`, `getMovedKey`, `getMovedTracks`, `checkBacking (engine, melodyTrack, ignoredClips)` -> `BackingCheck { blocked, recordingsLeft }`. Both files are in `CMakeLists.txt` (app and tests).
- **Topic:** `singing::Topic::chooseKey` (sixth of 15, after `measure`), text in `SingingGuide.cpp` (`chooseKeyText`), `singing::getKeyExample()` (range 45-65, melody 52-69, analysis by `analyse`), "In your song: " added by `explain` when the context has a range and a melody. Search words added in `TheoryGuides.cpp`. The two links from the plan are in `TheoryGuideLinks.cpp` (to *What a key is* and the instruments *Bass* topic).
- **Exercise:** `src/generate/RangeKey.{h,cpp}`, namespace `gen::rangekey` (steps, texts, `Progress`, `MeasureRecord`, `MoveRecord`, `HeardBarsCounter`, `findTake`, `findMelodyTrack`, `analyseMelody`, `getOfferedShift`, `isNudge`, `checkBacking`, statuses, done notes). `getAction` returns an `Op` (record, addRecordingTrack, measureTake, setByHand, convertToMidi, moveSong, playFromBar, audioSettings) with `shift` / `bar`, so group B dispatches on it by calling `rangekey::getAction (step, engine, progress)` directly. `Exercise::rangeKey` is in `gen::exercises` (`numExercises` 9, window height 700); `exercises::Context` has the new `rangeKey*` fields (take, range, chosenMelody, selected, measured, move, heardBars); `exercises::Action` has a new `enabled` flag (the window already has `actionEnabled`; step 5 in `ExerciseWindow.cpp` still needs `c.actionEnabled = action.enabled`).
- **Left for group B:** the Help command and clip menu item, filling the `rangeKey*` context fields in `getExerciseContext`, the records and the counter in `MainComponent` (feed `HeardBarsCounter` at 5 Hz, reset it on a move or when step 4 stops being done), the actions, and the topic's picture. `SingingExplainer.cpp` has stub cases for the new topic (height 250, a placeholder caption, nothing painted): step 7 replaces them.
- **Validation:** `./build.sh --test` ends with `All checks passed`, no compiler warnings; 60 new passing checks (song move and range-and-key tests) plus the updated singing, exercises and theory-guides checks. `--export-guides` exits 0: "83 links between guides, all pages exist"; the singing guide has 16 topics (15 + Sources).
- **Export failure, fixed:** the first export exited 1 with "No such page: arpeggiator / Swing". Cause: the link `rhythm Swing -> arpeggiator Swing` in `TheoryGuideLinks.cpp` named a topic the arpeggiator guide doesn't have; its topic is "Accents, swing and syncopation" (`src/ui/ArpeggiatorGuide.cpp`). It was already there before #198. Fixed the link's topic name, and the one swing-guide test that asserted the old name (`tests/EngineTests.cpp`).
- **Surprises:** a note moved with `moveSong` goes through `Engine::commitHistory` first, so anything pending is its own step. The scale filter reads the key through `Engine::setKey`, which updates the key's pitch classes, so it follows the move by itself (not checked in the real app yet: group C).

### Group B hand-off (steps 5 to 8, built, not committed)
- **Opening:** Help > *Find Your Range and Key...* (`rangeKeyExerciseCommand`, after *Map a Song's Structure...*) and the audio clip menu's *Find my range and key...* (id 51, after *Measure singing range...*; `TrackLanes::onFindRangeKey` -> `ArrangementView::onFindRangeKey` -> `MainComponent::showRangeKeyExercise`).
- **Records in `MainComponent`** (`rangeKeyTake`, `rangeKeyMeasured`, `rangeKeyMove`, `rangeKeyHeard` and its key): `getRangeKeyProgress` reads the range from the two settings and the melody choice from `ArrangementView::getChosenMelodyTrack` (new; `SingingExplainer::getChosenMelody`). `resetRangeKeyMemory` runs on a new or opened project and on Start over (counter only). `feedRangeKey` (5 Hz) takes the new take record from the engine's recording report, forgets the move after an undo that removes it (undo steps below the move's, or the key no longer matches), resets the counter when the key changes or step 4 isn't done, and feeds the heard-bars counter. The measurement comes from `ArrangementView::onRangeMeasured`.
- **Actions** (`runRangeKeyAction`, dispatched on `rangekey::getAction (...).op`): Record, Add a recording track, Measure this take (`ArrangementView::measureRange`), Set it by hand (opens the guide at *Voice types and range*), Convert to MIDI (`ArrangementView::convertToMidi`), Move the song (`songmove::moveSong`, then the move record), Play from bar N, Audio settings. `ExerciseWindow` now passes `action.enabled` to the button. **Show me** (`showRangeKeyTarget`): the track's R button or Record, the take (step 2), the melody track's header or the clip to convert, the key box (`TransportBar::getKeyBox`), Play (`TransportBar::getPlayButton`).
- **Picture:** `SingingExplainer`'s *Choosing a key for your voice* draws three bars on a keyboard (range, melody before with the part outside the range in red, melody after the move), from the song's own analysis with a range and a melody, else the example with the caption "An example: a melody too high for a baritone". Clicking a bar's end plays its note. A bar too short for its words puts them beside it. It follows the song and the range while the topic is open.
- **Scenes** (`ScreenshotRunner`, documented in `DevScreenshots.h`): `"exercise": "rangeKey"` with `rangeKeyMelody: [low, high]`, `rangeKeyTonic`, `rangeKeyBacking`, `rangeKeySungMelody`, `rangeKeyTake`, `rangeKeyMeasured`, `rangeKeyHeardBars`; the range is set with `settings` (`singing.lowest`, `singing.highest`).
- **Validation:** `./build.sh --test` ends with `All checks passed`, no compiler warnings; the test count is unchanged (Group B is UI). A smoke shot list (`/tmp/dawit-198-shots`) showed the key step with the move button, a recording left behind named in the status, the move done (step ticks, nudge shown) and the topic's picture in example and your-song mode; Show me ran without error.
- **For group C:** the `--demo` song already has MIDI tracks named *Lead vocal* and others, so the scenes' own *Lead* track is not the melody there and a recording under it isn't "blocked": use a song without them for the too-high, blocked and octave shots. The window height 700 and the step 5 and completion views are not yet looked at; the scale filter following the key is not yet checked.

### Group C hand-off (steps 9 and 10, checked and documented, not committed)
- **Real-app check** (`--screenshots`, scratch settings, `"song": "empty"`, scale 1, files in `/tmp/dawit-198-c`; shot lists `run1.json` to `run4.json` there). Looked at and found right: step 1 with no input ("No microphone found..." and *Audio/MIDI Settings...*), with no recording track (*Add a Recording track*) and done ("Take 1: 8.6 s."); step 2 with a differing measurement ("DawIt measured A2 to G4 in 'Take 1'. Click Use as my range in the guide to keep it.") and done ("Your range: A2 to F4, 20 semitones (closest: baritone)."); step 3 offering *Convert to MIDI...* on a Vocals clip, with Show me outlining that clip; step 4 too high (*Move the song 4 semitones lower*), blocked by a recording (the button off, the "-4" and "C major" status), after the move (key box "C major", the melody's ends down 4 in the piano roll, the button off, "Your melody fits: 3 semitones to spare at the bottom."), octave ("Sing it an octave lower..."), no key fits (button off, Skip), and near the top with the optional nudge; step 5 before, with Show me on Play, and done; the completion text; Show me for steps 1, 4 and 5 (R/Record, the key box, Play); the audio clip menu's *Find my range and key...* after *Measure singing range...*; the topic's picture in example and in-song mode. **Scale filter:** with the scale filter on for the melody track, the piano roll's rows and the hidden-notes count follow the key after the move (E major before, C major after), so the filter follows `Engine::setKey` by itself.
- **Changed after the shots:** the no-key status repeated the paragraph already shown as the do text; it is now "No key fits all of it, so there is nothing to move. Skip this step, or change a few of the highest or lowest notes." (test updated). The window height is 620, not 700: the tallest steps (a nudge or a status under the move button) end near y = 480 and the buttons sit at 590, so 700 left 200 px empty (tests, `ui-design.md` and `recipes.md` say 620).
- **Not checked in the real app:** the Help menu item itself (macOS native menu, not reachable by a shot; its command, description and menu position are in the code and `getCommandInfo`), and a real microphone recording (the take comes from the example range audio).
- **Validation:** `./build.sh --test` ends with `All checks passed`, no compiler warnings, 4490 passing checks, 0 failing; `--export-guides` exits 0 ("83 links between guides, all pages exist"; singing guide 16 topics: 15 and Sources).
- **Docs:** README *Features* bullet (and the singing guide sentence), `docs/tests.md` (song move, range and key, singing guide), `docs/code-layout.md` (RangeKey, SongMove, Exercises, SingingGuide, SingingExplainer), `docs/ui-design.md` (window height), `docs/recipes.md` (the guided-exercise recipe). No change to the architecture doc: nothing new is wired in a new way.
- **First steps:** checked, no update needed. The Start panel's tiles, the inspector's *Get started* and the First Song Exercise name no command, menu item or control that this feature renames, moves or adds; it is a separate Help exercise like the karaoke one, and the clip menu gains one item.
- **Theory deviations:** none from the plan's *Theory* facts, texts and numbers; the topic's text is the plan's, and the example numbers are computed by `singing::analyse` (-4, 48 to 65, C major) and tested. Text I wrote that the plan didn't give: the step do texts, the status lines not quoted in the plan (no recording track, "Stop recording first.", "Choose a key first (step 4).", "Heard: n of N bars", the no-key line, "'Backing' stays in the old key...") and the *Optional* sentence's top/bottom wording; none states a music-theory fact. The "In your song:" paragraph is added at the end of the topic. No new source, so `SOURCES.md` needs nothing beyond what the plan added; dawTheory's check is optional.
- **Website hand-off:** `features.html` exercises group (one short section: five steps, the move that keeps the song's chords and MIDI together, recordings stay); `learn.html` singing guide list (*Choosing a key for your voice*, sixth of 15). Screenshots worth taking: `"exercise": "rangeKey"`, `exerciseStep` 3 with `rangeKeyMelody [52, 69]`, `rangeKeyTonic` 4 and `settings` singing.lowest 45 / singing.highest 65 (the too-high step with the move button), and the singing guide at topic 5 (example, or with the same scene). No library, model or tool was added, so no `about.html` or `LICENSE` change.
- **Issue updates not made:** no GitHub write was made (the channel has no authorisation rule for them). Still to do by the coordinator: a progress comment on #198 (plan path `plans/guided-exercise-find-your-vocal-range-and-key.md`, *Built, not committed*, the validation above, nothing left open except commit and website), the checklist item in #195, a note on #26 that the song move (`gen::songmove::moveSong`) exists for its dialog, and the project status (set to In Progress when the build began was also not done).

### Picture fixes after theory review (built, not committed)
- **Defects (from dawTheory):** the key picture said "No move needed: the melody already fits your range." for a melody wider than the range, and labelled an octave-only suggestion "Moved 12 semitones... / After moving the song".
- **Fix** (`SingingExplainer.cpp`, `KeyPicture`, `getKeyPicture`, `paintChooseKey`): the picture now keeps the analysis' `noKey` verdict and `octave` flag. No key fits: no third bar, and the line "No key fits all of it: the melody spans N semitones and your range M. Moving the song can't help; sing the highest phrase an octave lower or change a few notes." An octave: the third bar reads "You sing it an octave lower/higher" with the notes (its tooltip says the song stays in its key), the key name stays the song's. "No move needed" is only shown when the melody fits with no shift.
- **Screenshots looked at** (`/tmp/dawit-198-d`, the topic with a range and a one-track melody): no key (24 semitones in 20, no move bar, the advice line), octave down (64-81 in 45-72: "You sing it an octave lower - E3 to A4"), octave up (48-60 in 57-81: "You sing it an octave higher - C4 to C5"), normal -4 ("Moved 4 semitones lower - C major - C3 to F4"), already fits ("No move needed...").
- **Validation:** `./build.sh --test` ends with `All checks passed`, no compiler warnings (4490 passing checks, 0 failing); `--export-guides` exits 0, "83 links between guides, all pages exist", singing guide 16 topics.

