# A singing techniques guide: measure my range from a recording

Status: Released
Idea: [#14 — A singing techniques guide](https://github.com/geertvandamme/dawit/issues/14)
Builds on: [plans/singing-techniques-guide.md](singing-techniques-guide.md) (its decision 1 (b))

## Goal
Record yourself singing your lowest and highest comfortable notes (or import such a recording), and DawIt measures the notes you held, names the closest voice type and, when you accept, keeps them as your range (`singing.lowest`, `singing.highest`), so *Your voice and your song* checks your melody against your real voice. A beginner who can't tell which key they reached by ear gets a measured range, with the pitch curve to see why.

## Decisions for you
1. **Scope: a clip only, or also live from the microphone.** Options: (a) measure an audio clip (recorded with R on an audio track, or imported); (b) also a *Listen* button that measures straight from the input through the tuner's input tap (`Engine::getTunerTap`). **Recommended:** (a). DawIt records audio now, so (a) already covers the microphone in two steps (record, then measure), and the result can be played back and checked. (b) needs an input chooser, the microphone permission and a live picture in the guide window, copied from the Tuner window: worth its own slice. A sung take against the melody in cents stays with #28 *Practise singing*, which can reuse this plan's held-note finder; the *Sing* path stays with #3.
   Answer: (a) a clip only, as recommended.
2. **Apply the measurement at once, or ask.** Options: set the range as soon as it's measured; show it with a *Use as my range* button. **Recommended:** the button: a measurement can be wrong (an octave mistake, a backing track on the recording) and it would silently replace a range set by hand.
   Answer: the *Use as my range* button, as recommended.
3. **DawIt's measuring rules** (opinion, not a standard): a note counts when its vibrato-smoothed pitch stays within 35 cents of one pitch for at least 0.5 s, rounded to the nearest note; the text warns at a span under 12 semitones (narrow) or of 30 or more (wide). **Recommended:** as written; the user is asked to hold each note about 2 s, so 0.5 s leaves room for a wobbly start.
   Answer: as written.
4. **Falsetto.** DawIt can't tell falsetto from head voice. Teachers differ on whether it counts. **Recommended:** count every held note and say so in the text: set your highest lower by hand if your songs need full voice there.
   Answer: count every held note and say so, as recommended.
5. **A new topic or a part of *Voice types and range*.** **Recommended:** a new topic *Measuring your range*, right after *Voice types and range* (14 topics; *Your voice and your song* stays last), because the measurement needs its own picture (the pitch curve).
   Answer: a new topic *Measuring your range* after *Voice types and range*, as recommended.

## What the user does and sees
- **Opens from:** *Measure singing range...* in an audio clip's right-click menu (after *Convert to MIDI...*, disabled while recording, like its neighbours); and *Measure a clip...* on the new topic of the singing guide.
- **The job:** copy `TempoActions`: a progress window "Measure singing range" ("Listening for your notes in "Range test"...") with Cancel. A 10-second clip takes well under a second. Then the singing guide opens (or comes to the front) at *Measuring your range* with the result.
- **Voice row** (now also shown on *Measuring your range*): `Your voice: [Baritone ▾] Lowest [A2 ▾] Highest [A4 ▾] [Measure a clip...]`. The button pops up a menu of the song's audio clips, "Range test (Voice, bar 3, 8.6 s)", the selected track's clips first; with none, one disabled item "No audio clips yet: record one with R on an audio track, or drag a file onto a track". Choosing one runs the job.
- **Result row** (30 px, under the voice row, only on that topic once something is measured): `Measured in "Range test": A2 to G4 · 22 semitones · closest: baritone   [Use as my range]`. *Use as my range* is disabled when nothing usable was found, and reads *Your range* (disabled) when the range already equals it. Using it sets both settings, updates the voice row and every topic's text (as the voice row does today).
- **Picture:** the clip's pitch over time (seconds across, a keyboard down the left from 3 semitones below the lowest to 3 above the highest held note), the pitch as a thin line, held stretches as thick bars, the lowest and highest labelled ("A2 · 110 Hz · held 1.6 s"), and on the right the measured range as a bar beside the closest voice type's usual range. Long clips show the stretch from the first to the last clear pitch. Without a measurement it shows the example take (Example data) with the caption "An example: a baritone's range test". Click a key or a label to hear that note (`Engine::previewNote`, 600 ms), right-click a key for *Set as my lowest/highest note* (as the other keyboards).
- **Tooltips:** *Measure a clip...* "Measure your range from an audio clip of you singing your lowest and highest notes. Choose the clip from the list."; *Use as my range* "Keep the measured notes as your range, for every song. You can still change them by hand."; the result label "What DawIt measured in the clip. Click a key in the picture to check the notes by ear."; over the curve "3.4 s · D3, 12 cents sharp"; over a held bar "Held 1.6 s · A2 (110 Hz) · your lowest"; over a gap "No clear pitch here: a breath, a consonant or silence".
- **Remembered:** only `singing.lowest`/`singing.highest`, and only on *Use as my range*. The measurement lives in the window while it's open.

## Data and saving
None in the project. The two existing app settings.

## The guide
New topic in `gen::singing`, fifth of 14: **Measuring your range** — *Record yourself low and high, and DawIt finds the notes*. Picture as above. Text in *Theory*, followed by the measurement's paragraphs (the *Your song* rules M1 to M7) when there is one. Changed texts: *Voice types and range*'s last paragraph and *Your voice and your song*'s rule 2 point to it (*Theory*). The Theory Guides entry: summary adds "measuring your range from a recording", `opensFrom` adds "an audio clip's Measure singing range...", keywords add "measure measuring measured recording held fry". Still not on *Start here*.

## Code
- Recipes: *Topics in an existing guide*, *A long job* (copy `TempoActions`), *A command or menu item* (the clip menu through a `TrackLanes` callback, as `onConvertToMidi`), *Checking in the real app*.
- **Range finder** (`src/analysis/RangeFinder.{h,cpp}`, `daw::analysis::rangefinder`, both targets), two pure stages:
  1. *Frames*: the audio as mono at its own rate (`analysis::toMono`), `pitch::Tracker` with its default clarity and loudness and the octave guard, a 40 ms window (`pitch::getWindowSize`) every 10 ms; a progress callback that can cancel.
  2. *Held notes* from frames: per run of clear frames (unclear gaps up to 50 ms bridged), the pitch smoothed with `tune::CentreFilter` (restarted at each run); a held note is a stretch of at least 0.5 s whose smoothed pitch stays within 35 cents of the stretch's median; its note is that median rounded. The result: frames, held notes (note, start, length, median cents off), lowest and highest held notes clamped to the voice row's C2 to C7 (with a flag), and a verdict (nothing held; under 5 semitones; measured).
  Stage 2 takes frames so tests can feed made-up ones; #28 can reuse it.
- **Guide content** (`gen::singing`, may include the analysis header as `KeyFindingGuide.h` does): the topic, its text, the measurement's advice (from the result, the clip name and the current range), the example take as `tune::TakeNote`s and its frames (`tune::makeFrames`, 10 ms), its example size.
- **The job** `RangeMeasurer` in `src/ui/SingingRangeActions.{h,cpp}`, owned by `ArrangementView` (which owns the singing guide): copies the clip's audio and rate under `Engine::getLock()`, runs both stages on the progress thread, then calls the explainer's "show this measurement" on the message thread. The explainer gets a callback to start it (the clip menu path) from `ArrangementView`.
- **Threading:** nothing on the audio thread; one `pitch::Tracker` per job. The window writes only the two settings and never changes the song.
- **Risks:** small. The 14th topic shifts indexes: every switch over `Topic`, the picture heights and captions, `getExampleSize`, the topic-count tests.

## Libraries and models
None. The pitch tracker is DawIt's own (`PitchTracker`, McLeod's method). Basic Pitch was considered and not used: it rounds to semitones and hears chords, where this needs one voice's fractional pitch to smooth vibrato.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | Range is "lowest to highest note a voice can produce"; teachers count only musically useful notes, usually in the modal register (falsetto and whistle notes for some voices) | Wikipedia *Vocal range* | typical |
| 2 | Range alone doesn't decide voice type; tessitura and timbre matter more | Wikipedia *Vocal range*, *Voice type* (as the guide's fact 10) | typical |
| 3 | Vocal fry is the lowest register, an irregular, rattling vibration | Wikipedia *Vocal fry register* | typical |
| 4 | Range tests: warm up, sing on "ah", count only notes you can hold cleanly without strain; range often widens with practice | Singing Carrots; voice-teaching guides (search) | opinion |
| 5 | Teachers differ on counting falsetto | range-test guides (search) | opinion |
| 6 | DawIt finds pitch with McLeod's method over about 40 ms; octave mistakes happen, an octave high or low | `PitchTracker.h`; the Tune guide's *How a tuner finds the pitch* | exact (DawIt's code) |
| 7 | Reverb, other sounds and two voices blur the pitch | the Tune guide (same text) | typical |
| 8 | A2 = 110.00 Hz, G4 = 392.00 Hz, C2 = 65.41 Hz, C7 = 2093.00 Hz | derived: 440 · 2^((n − 69)/12) | exact |
| 9 | 22 semitones = 1.8 octaves (22 / 12 = 1.83) | derived | exact |
| 10 | The smoothing (2-pole Butterworth, 1.5 Hz) passes 6-Hz-ish vibrato at 1/√(1 + (5.5/1.5)⁴) = 0.074: ±60 cents becomes ±4.5 | derived (`tune::CentreFilter`) | exact |
| 11 | Held 0.5 s within 35 cents; narrow under 12, wide 30 or more; clamp to C2 to C7 | DawIt's rules (decision 3) | opinion |
| 12 | Closest type: nearest midpoint, ties lower; example 45 to 67 has midpoint 56, baritone's 57 (1 away), bass's 52 (4) | the guide's rule; derived | exact |

**Agreement with existing guides:** the method and its octave caveat repeat the Tune guide's *How a tuner finds the pitch* (40 ms, McLeod); this measurement answers every 10 ms where DawIt Tune answers every 5 ms, and the text says "about every 10 milliseconds", which is true of this one. The *Voice types and range* ranges and the closest-type rule are reused unchanged. No contradiction found.

### Topic text

**5. Measuring your range** — *Record yourself low and high, and DawIt finds the notes*

Finding your range by ear works, but it's easy to lose track of which key you reached. DawIt can measure it for you: record yourself singing your lowest and highest comfortable notes, and it finds the notes you held.

How to record it:
1. Warm up first (Warming up). A warmed-up voice reaches its low and high notes more easily and more safely.
2. Add an audio track, choose your microphone's input, press the track's R button and then Record (How recording works explains the input and the level). Wear headphones, or mute the other tracks, so only your voice is on the recording.
3. On "ah", slide down from a comfortable middle note to the lowest note that still sounds good, and hold it for about 2 seconds.
4. Breathe, then slide up to the highest note you can sing without straining, and hold that for about 2 seconds too.
5. Stop, right-click the clip and choose Measure singing range..., or click Measure a clip... above the picture. A recording made in another app works too: drag the file onto a track first.

Sing only notes you'd use in a song. A strained shout at the top, or the creaky rattle below your lowest note (vocal fry), would make your range look bigger than it is for singing. Teachers differ on falsetto: some count it in a singer's range and some don't. If your top notes were falsetto and your songs need your full voice up there, set your highest a little lower by hand.

How DawIt measures: it finds the pitch of the recording about every 10 milliseconds, the same way DawIt Tune does (What is pitch correction? explains how), and smooths out the vibrato so it hears the middle of each note. A note counts when you held it steady, within 35 cents (about a third of a semitone) of one pitch, for at least half a second; DawIt then rounds it to the nearest note. Slides, cracks and short squeaks don't count, and neither does anything without a clear pitch: breaths, an s, a creak. Your measured range runs from the lowest to the highest note that counts.

The picture shows the recording's pitch over time against a keyboard, the held notes as thick bars, and your lowest and highest notes. Without a measurement it shows an example: a baritone who holds A2, slides up and holds G4. The squeak at A4 lasts 0.3 seconds, too short to count. Click a key to hear it, and check by ear that DawIt heard the right octave: a voice can fool a pitch tracker into hearing a note an octave too high or too low.

The closest voice type is the one whose usual range has its middle nearest the middle of yours. It's a hint, not a label: a voice type also depends on where your voice is comfortable and on its colour (Voice types and range). Click Use as my range to keep the notes. DawIt remembers them for every song, and Your voice and your song checks your melody against them. A beginner's range often grows with practice, so measure again after a few weeks.

**Changed texts.** *Voice types and range*, last paragraph, replace "Right-click a key to set it as your lowest or highest note, or pick a voice type above the picture." with "Right-click a key to set it as your lowest or highest note, or pick a voice type above the picture. Or record yourself and let DawIt find the notes (Measuring your range)." *Your voice and your song*, rule 2's last sentence becomes "Set your own range above, or measure it from a recording (Measuring your range), to check it against your voice." The intro paragraph's "(Voice types and range says how to find them)" becomes "(Voice types and range and Measuring your range say how to find them)".

### Example data
The example take, `tune::TakeNote`s (times in s, cents off the note):
- A2 (45), 0.4 to 2.6: cents 300 · (1 − t / 0.6) for t < 0.6 (a slide down from C3), then 0. Held from 1.0 to 2.6.
- Silence 2.6 to 3.2 (a breath).
- G4 (67), 3.2 to 7.6: cents −2200 · (1 − t / 3) for t < 3 (a slide up from A2), 0 to t = 3.3, then 50 · sin (2π · 5.5 · (t − 3.3)). Held from 6.2 to 7.6.
- A4 (69), 8.0 to 8.3: flat (the squeak, "0.3 s: too short to count").
- The take is 8.6 s; frames from `tune::makeFrames` every 10 ms for the picture. Expected: lowest A2 (110.00 Hz), highest G4 (392.00 Hz), 22 semitones, 1.8 octaves, closest baritone (A2 to A4).

### "Your song" rules (the measurement's paragraphs)
On *Measuring your range*, after the text. N is the clip's name, L and H the lowest and highest held notes (after clamping), S = H − L. In order:
- **M1. Nothing held:** "DawIt heard no note held for half a second in "N". Sing on "ah", hold your lowest and highest notes for about 2 seconds each, with only your voice on the recording (no backing track or reverb), and measure again." Nothing else.
- **M2. Under 5 semitones** (one held note, or S < 5): "DawIt heard only G3 to A3 held steadily in "N", 2 semitones apart. Slide all the way down to your lowest comfortable note and up to your highest, hold each for about 2 seconds, and measure again." (one note: "only G3 held steadily"). Nothing else; *Use as my range* disabled.
- **M3. Measured:** "Measured in "Range test": your lowest held note is A2 (110 Hz) and your highest is G4 (392 Hz), 22 semitones apart, about 1.8 octaves. The closest voice type is baritone (usually A2 to A4). Click Use as my range to keep these notes; Your voice and your song then checks your melody against them." (Hz to the nearest whole number; octaves S / 12 to one decimal.)
- **M4. Clamped:** "Your lowest held note, A1, is below C2, the lowest note a range can have in DawIt, so C2 is used." (or "highest ... above C7, the highest ..., so C7 is used").
- **M5. Narrow** (5 ≤ S < 12): "That's less than an octave. A beginner's comfortable range is often narrow at first and grows with practice. Check that you slid all the way down and up, and measure again in a few weeks."
- **M6. Wide** (S ≥ 30): "That's two and a half octaves or more, wider than any usual voice type. Check by ear that the top and bottom notes are ones you'd sing in a song, not a strain, a creak or falsetto, and set them by hand if not."
- **M7. Against the range set now** (a range is set): equal: "That's the range you have set now." Different: "Your range now is C3 to C5; Use as my range replaces it." Quiet when none is set.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Example take through `tune::makeTake` at 48 kHz, both stages | lowest 45, highest 67, verdict measured, closest baritone; the 0.3 s A4 not held | exact |
| Its held A2 and G4 | at least 1.0 s and 0.8 s long | ≥ |
| Note frequencies 45, 67, 36, 96 | 110.00, 392.00, 65.41, 2093.00 Hz | 0.01 Hz |
| A2 held 2 s 30 cents flat, G4 held 2 s 40 cents sharp (audio) | 45 and 67 | exact |
| C3 and C4 held 2 s each with ±75 cents vibrato at 6 Hz (audio) | 48 and 60, one held note each | exact |
| `CentreFilter` on 60 · sin (2π · 5.5 t) frames at 100 per second, after 1 s | within ±6 cents of 0 (derived ±4.5) | ≥ |
| Made-up frames steady at 50 for 0.45 s / 0.55 s | not held / held | one frame |
| A ramp 50 → 62 over 2 s alone | nothing held | exact |
| 1 s note with a 40 ms unclear gap / two 0.4 s halves with a 100 ms gap | one held note / none | exact |
| 3 s of noise (`juce::Random` seed 1, amplitude 0.3); 3 s of silence | nothing held | exact |
| Held 33 and 60 / 60 and 100 | lowest 36, clamped / highest 96, clamped | exact |
| Held 57 and 60 | under 5 semitones | exact |
| Advice: M1 has "half a second"; M2 has "only"; M3 on the example has "A2", "G4", "22 semitones", "1.8 octaves", "baritone"; M4 names "C2"; M5 at S = 7; M6 at S = 31; M7 both ways | as listed | text |

Text checks: 14 topics, the fifth *Measuring your range*, the last still *Your voice and your song*; every topic has text of 400 or more characters and an example; no "color", "center", "analyze"; search "measure range" finds the singing guide with *Measuring your range* first, and "vocal fry" finds it.

## Steps
**Group A, engine** (`./build.sh --test` at step 3)
- [x] 1. `src/analysis/RangeFinder.{h,cpp}` (both CMake targets): the two stages and the result (*Code*). Tests: a new `runRangeFinderTests` with the audio and frame rows of *Test numbers*. Done when: the example take measures A2 to G4.
- [x] 2. `gen::singing`: `Topic::measure` after `range`, name, summary, text, the M1 to M7 paragraphs, the example take and frames, `getExampleSize`; the three changed texts. Tests in `runSingingGuideTests`: 14 topics, the advice rows, the changed rule 2 text. Check: the topic's text with and without a measurement.
- [x] 3. `TheoryGuides`: the entry's summary, `opensFrom` and keywords. Tests: the two searches. Done when: the group's build passes without warnings.

**Group B, UI** (`./build.sh --test` at step 6)
- [x] 4. `src/ui/SingingRangeActions.{h,cpp}` (`RangeMeasurer`, in `DawIt` only), owned by `ArrangementView`; *Measure singing range...* in `TrackLanes::showClipMenu` through a new callback; on success the singing guide opens at the topic with the result. Check: right-click an audio clip, measure, the guide opens.
- [x] 5. `SingingExplainer`: the voice row on the new topic with *Measure a clip...* and its clip menu; the result row with *Use as my range*; the picture (curve, held bars, labels, range and closest-type bars, previews, right-click, tooltips); the example without a measurement; every switch over `Topic`. Check: each tooltip; Use updates the voice row and *Your voice and your song*; Export PDF.
- [x] 6. `ScreenshotRunner`: a scene `rangeTake` that adds an audio track "Range test" with the example take (48 kHz) and measures it without the progress window, opening the guide at *Measuring your range*; documented in `DevScreenshots.h` and recipes.md *Checking in the real app*. Check: `--export-guides` passes.

**Group C, check and docs**
- [x] 7. Real-app check, song "pop": guide "singing" at *Measuring your range* with no measurement (the example); scene `rangeTake` then the same topic (A2 to G4, baritone, Use enabled); the same with `"settings": { "singing.lowest": 48, "singing.highest": 72 }` (M7 "Your range now is C3 to C5"); the clip menu on "Range test" (the item shown); *Voice types and range* (the changed text). Look for cut-off text in the result row, an empty or misplaced curve, wrong note labels.
- [x] 8. Docs: README *Features* (the singing guide bullet: measure your range from a recording), *Tests* and *Code layout*; docs/tests.md (`runRangeFinderTests`, the singing checks); docs/code-layout.md (`RangeFinder`, `SingingRangeActions`); docs/architecture.md *Long jobs* (add `RangeMeasurer` to the examples) and the *Who owns what* tree (`RangeMeasurer` under ArrangementView's jobs); issue #14 progress (measuring from a clip done; live microphone, takes in cents (#28) and the *Sing* path (#3) open).

## Tests
In `runRangeFinderTests` (step 1) and `runSingingGuideTests` / `runTheoryGuidesTests` (steps 2 and 3): the rows of *Test numbers*, plus: a cancelled progress callback returns no result; a stereo clip measures the same as its mono copy.

## Website hand-off
- Features page, the singing guide's section: one or two sentences on measuring your range from a recording and keeping it as your range.
- Main page: nothing new.
- Screenshot: scene `rangeTake`, the guide at *Measuring your range* (the curve, A2 to G4, baritone).
- How it's made: nothing new.

## Left for later
- Measuring live from the microphone (decision 1 (b)), with the tuner's input tap.
- A sung take against the melody in cents: #28 *Practise singing* (reuse the held-note stage).
- The *Sing* path of *Guide paths by goal* (#3).
- Telling falsetto from full voice in the measurement: no reliable simple rule; the text leaves it to the singer.
- Keeping several measurements to show the range growing over time (#28's "practice over time").

## Changes while building

**Group A hand-off (engine).**
- `daw::analysis::rangefinder` (`src/analysis/RangeFinder.{h,cpp}`, both targets): `findFrames (audio, rate, progress)` (stage 1, a frame per 10 ms, `Progress` is `std::function<bool (double)>`, false cancels and the result is empty), `findHeldNotes (frames, secondsPerFrame = 0.01)` (stage 2, returns a `Result`), and `measure (audio, rate, progress)` returning `std::optional<Result>` (nullopt = cancelled). `Result` has `verdict` (`nothingHeld`, `tooNarrow`, `measured`), `frames`, `held` (`HeldNote { note, start, length, centsOff }`), `lowest`/`highest` (clamped to C2 to C7), `rawLowest`/`rawHighest`, `lowestClamped`/`highestClamped`, `firstPitchSeconds`/`lastPitchSeconds` (for "from the first to the last clear pitch") and `getSpan()`. Constants `frameSeconds`, `lowestNote`, `highestNote`, `minSpan`.
- `gen::singing` (`SingingGuide.h` now includes `engine/Tune.h` and `analysis/RangeFinder.h`): `Topic::measure` (fifth of 14), `Measurement { present, clipName, result; getRange() }`, `Context::measurement`, `getMeasureAdvice (measurement, currentRange)`, `getRangeExample()` (notes, 8.6 s, frames every 10 ms, its `result`, `previewNote` 45), and `explain (Topic::measure, context)` adds the advice paragraphs after the text. `getExampleSize (measure)` is 2 (the example's held notes).
- Held-note details: smoothed pitch per run of clear frames; the greedy window grows while max - min stays within 70 cents, is then cut to the longest stretch within 35 cents of its median. A bridged gap holds the last smoothed value.
- Left for group B: `src/ui/SingingExplainer.cpp` only has placeholders for `Topic::measure` (height 330, a one-line caption, the range picture painted, no keyboard behaviour change) so the app builds without warnings; replace them with the real picture, voice row, result row and previews.
- Not what the plan says: the search matches word beginnings, so "measure range" finds the singing guide but at *Voice types and range*; "measuring range" reaches *Measuring your range*. The test checks that, and that "measure range" and "vocal fry" find the guide. No search code was changed.

**Group B hand-off (UI).**
- `RangeMeasurer` (`src/ui/SingingRangeActions.{h,cpp}`, `DawIt` target only): `measure (Track&, AudioClip&)` runs the progress job and calls `onMeasured (Measurement)`; static `measureNow (const AudioClip&)` measures with no window (the screenshots). Owned by `ArrangementView` (`rangeMeasurer`); `showSingingMeasurement (m)` opens the guide at the topic and calls `SingingExplainer::showMeasurement (m)`; `connectSingingExplainer` sets the explainer's `onMeasureClip` at both construction sites (the window and the Theory Guides entry). The clip menu item *Measure singing range...* (menu id 50, after *Convert to MIDI...*) goes through `TrackLanes::onMeasureRange`.
- `SingingExplainer`: voice row now also on the measure topic, with *Measure a clip...* (menu of the song's audio clips, the selected track's first); result row (`resultLabel`, `useButton`: reads *Your range* and is disabled when equal, or when nothing usable); `Picture::paintMeasure` with `getMeasureLayout`, held bars and labels (click to hear), the keyboard strip (right-click sets lowest/highest), the range and closest-type bars, tooltips. The measurement stays in the explainer until replaced.
- Scene `rangeTake` in `ScreenshotRunner` (documented in `DevScreenshots.h` and recipes.md). `--export-guides` passes.
- Differs from the plan: the picture's pitch axis covers the whole curve (not only the held notes) plus 3 semitones each side, so the example's A4 squeak shows; the example's "0.3 s: too short to count" is drawn on the picture. Menu id 50 because 23 and 31 are taken. The group's `./build.sh --test` ended with All checks passed, no warnings; real-app shots (no measurement, and `rangeTake`) looked right (a.png and b.png in the scratchpad); the clip menu, the M7 settings shot and *Voice types and range* are group C's.

**Group C hand-off (check and docs).**
- Real-app shots (scale 1, song pop): the example topic with no measurement, scene `rangeTake` (A2 to G4, 22 semitones, closest baritone, *Use as my range* enabled), the same with `singing.lowest` 48 and `singing.highest` 72 set in the same shot as the scene, and *Voice types and range*. No cut-off text, the curve, held bars, labels and the range bars were right; no fixes needed. The measurement lives in the window, so a shot that needs the M7 text must set the settings in the shot that runs the scene (a later `guide` shot opens a fresh window with the example). The M7 and changed-text paragraphs sit below the fold; the tests check their text. The clip menu is a popup that a shot list cannot open: *Measure singing range...* (id 50) is in `TrackLanes::showClipMenu`, enabled when the clip is editable, and was checked by reading the code, not by sight.
- Docs: README *Learn as you go* sentence, docs/tests.md (singing guide line and `runRangeFinderTests`), docs/code-layout.md (`RangeFinder`, `SingingRangeActions`, singing entries), docs/architecture.md (jobs tree and *Long jobs*), recipes.md (scene, from group B), SOURCES.md (from group A). No new library, so no licence changes.
- Website hand-off: Features page, singing guide section: one or two sentences on measuring your range from a recording (record on an audio track, *Measure singing range...* in the clip menu, the pitch curve, held notes, closest voice type) and keeping it with *Use as my range*. Screenshot: scene `rangeTake`, `{ "scene": "rangeTake", "capture": "window" }` on song pop (the guide at *Measuring your range*, A2 to G4, baritone). Optional second: the same topic with no scene (the example take). How it's made: nothing new. Learn page: the singing guide's topic count is now 14.
