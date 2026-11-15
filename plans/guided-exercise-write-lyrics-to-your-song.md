# Guided exercise: write lyrics to your song

Status: Released
Idea: [#204 — Guided exercise: write lyrics to your song](https://github.com/geertvandamme/dawit/issues/204)
Part of: [#195 — Guided exercises that teach by doing in your own song](https://github.com/geertvandamme/dawit/issues/195). Builds on the shared exercise model of #196 (`plans/guided-exercise-learn-the-chords-of-a-song.md`, released): `src/generate/Exercises.{h,cpp}`, `src/ui/ExerciseWindow.{h,cpp}`, `GuidePointer`.

## Goal

A third guided exercise: pick a part of your song, write one line for it, time it by tapping along and sing it with the words in front of you. Each step ticks from the real state of the song (the loop, the Lyrics row, the timing of the line, the song playing with Sing along open), points at the control, links to the lyrics guide's pages and says what's missing. A beginner ends with one sung line in their own song and knows the four tools that make the rest: sections, the Lyrics window, tapping and Sing along.

## Decisions for you

1. **What "pick a section" means.** Options: (a) loop it: the loop is on and lies inside one sung section (*Loop this section* in the section's menu); remembered for the session when the loop goes off; (b) DawIt picks the first sung section without words and step 1 ticks when one exists; (c) the playhead stopped in a sung section. **Recommended:** (a): it is a real choice, and the loop then repeats the part for writing, tapping and singing without rewinding. The loop is saved in the project, so a reopened song still has it.
   Answer: the recommendation (user, 2026-11-15).
2. **When "sing it" ticks.** Options: on opening Sing along; once the song has played through the line with Sing along open (in one pass); once an audio recording covers the line. **Recommended:** played through with Sing along open, like the chord exercise's play-along rule: it can't tick from a click, and needs no microphone. Recording a take is the completion's suggestion and a later step (*Left for later*).
   Answer: the recommendation (user, 2026-11-15).
3. **Tapping a single line, and tapping across a loop.** Today tapping goes on to the next line, and at a loop wrap it would put the next line's syllables back at the loop's start. Options: (a) *Tap this line* from the exercise taps only that line and stops after its last syllable; and in all tapping, a loop wrap sends the next tap back to the first line whose middle lies in the loop (in single-line mode, that line again); (b) only the single-line mode. **Recommended:** (a): the wrap rule fixes a real fault for everyone who taps with the loop on, and lets the learner tap the line again each pass.
   Answer: the recommendation (user, 2026-11-15).
4. **Where it opens.** Options: Help > *Write Lyrics to Your Song...*; also the Lyrics row's right-click *Write lyrics step by step...*; also a section's menu or a Start panel tile. **Recommended:** Help and the Lyrics row menu. An empty song has no sections or music to write to, so no Start tile; a section-menu entry waits for #195's list of exercises.
   Answer: the recommendation (user, 2026-11-15).
5. **The hyphen tip.** When a word of the line sounds like 2 or more syllables (by `rhyme::countSyllables`, the pronouncing dictionary) but is typed as one block, step 3 names it and suggests hyphens. **Recommended:** yes: unsplit words are the commonest reason a tapped line feels wrong.
   Answer: the recommendation (user, 2026-11-15).
6. **Point to the exercise from the lyrics guide.** **Recommended:** one sentence at the end of *Your lyrics* when the song has no lyrics (no new fact, no source change).
   Answer: the recommendation (user, 2026-11-15).

## What the user does and sees

**Opening.** Help > *Write Lyrics to Your Song...* (new command after *Learn the Chords of a Song...*, no shortcut, description "Pick a section, write a line for it, time it by tapping and sing it, in four steps"); the Lyrics row's right-click menu *Write lyrics step by step...* (just above the guide item; opens the exercise only). The shared `ExerciseWindow`, titled "Write lyrics to your song", 360 × 680 (check by shot; four rows with long texts), always on top, reopening at the remembered step.

**Steps** (texts in *Theory*). The **chosen section** is the sung section the loop lies in (loop on, start and end inside one section of a sung type, `lyrics::isSung`), else the one remembered from earlier this session if a sung section still starts at that bar. A line **counts for** the section when the middle of its span (start to end beat) lies inside the section's bars, so a pickup before the bar line still counts. **The line** is the first counting line with every syllable placed, else the first counting line.

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Pick a section | a chosen section exists | *Open the Sections row* (unfold song rows if folded, then *Show me*) | the chosen or first sung section on the Sections row (`SectionLane::getBarsArea`); with none, the row's first free bars |
| 2 Write a line | a line counts for the chosen section | *Open Lyrics...* (`ArrangementView::showLyricsEditor`) | the Lyrics header's *Edit...* button |
| 3 Time it by tapping | a counting line has ≥ 2 syllables and every syllable after its first is pinned (tapped, dragged or detected) | *Tap this line*: brings the main window to the front, then `LyricsLane::startTapping` from the line's start in single-line mode (decision 3) | the line's span on the Lyrics row (new area getter, like `ChordLane::getBarArea`) |
| 4 Sing it | the **sung record** matches the line (below) | *Open Sing along* (`ArrangementView::showSingAlong`, Cmd+5) | the Lyrics header's *Sing along* button |

**Sung record** (in memory): while the transport plays and Sing along is showing, `MainComponent`'s existing 5 Hz timer feeds a pure tracker the heard beat and the line's span. Done when, in one pass, a sample falls in the line's first half and a later one in its second half; a stop, Sing along closing or the beat going backwards (a loop wrap) starts a new pass. A line under 0.5 s needs one sample inside. The record keeps the line's text; step 4 is done while the line has that text.

**Missing prerequisites and tips** (status line; the button turns into the fix only where one is named):
- Step 1: no sections; only unsung sections (names them); loop on across two sections. Fix: *Open the Sections row*.
- Steps 2 to 4 with no chosen section: "pick a section first", fix *Open the Sections row*. Steps 3 and 4 with no counting line: fix *Open Lyrics...*.
- Step 2: lines exist but none counts for the section (drag hint). Done note warns when the line has more than 2.5 syllables a beat (the *Your lyrics* rule).
- Step 3: partly placed ("2 of 8 not placed"); one-block line; hyphen tip (decision 5).
- Steps 3 and 4: nothing sounds in the section (no unmuted clip on an unmuted track overlaps its bars): tip, button stays.
- Step 4: no audio output: fix *Audio/MIDI Settings...*; Sing along open and not yet done: a "press Space" line.
- No model is needed; Write with AI is mentioned as optional (needs Ollama).

Nothing changes the song except the user's own actions: the Lyrics window changes it only on *Apply lyrics to song*, tapping only the tapped line. *Tap this line* starts playback, as the Tap button does.

**Remembered** in app settings: `learn.writeLyrics.step` (0 to 3) and `learn.writeLyrics.done`. No steps-version key (new exercise).

**First steps:** no change: the exercise needs a song with sections and music, so it isn't a Start panel choice, and no command or control the beginner's first steps name changes.

## Data and saving

No project data and no format change. In memory, owned by `MainComponent`, cleared in `afterReplace` and on *Start over* (both describe practice): the remembered section (its start bar) and the sung record.

## The guide

No new guide or topics, no `gen::guides` entry, no `TheoryGuideLinks.cpp` change. Links (the links test checks each names an existing topic):
- Step 1: Lyrics · *What each part says*; Song structure · *How songs are built*.
- Step 2: Lyrics · *Lines and phrases*; Lyrics · *Writing tips*.
- Step 3: Lyrics · *Stress and the beat*; Rhythm · *Bars and beats*.
- Step 4: Lyrics · *Singing along*; Singing · *Posture and breathing*.
- Completion carry-on: Lyrics · *Your lyrics*.

One sentence is added to the lyrics guide's *Your lyrics* text when the song has no lyrics (see *Theory*).

## Code

Recipes: *A guided exercise* (copy the chord exercise), *A command or menu item*, *Checking in the real app*.

- **`gen::writelyrics`** (new, `src/generate/WriteLyrics.{h,cpp}`, both CMake targets): steps, texts, the chosen-section rule (pure: sections, loop on/start/end, bar length, remembered bar), the counts-for rule, the line, the four done rules, statuses with fixes, done notes, the hyphen tip and the sung tracker (pure: fed heard beat, playing, Sing along showing, line span, seconds a beat).
- **`gen::exercises`**: register `writeLyrics` (stem `learn.writeLyrics`, links, carry-on, help description); `Context` gains the remembered section bar and the sung record. `Fix` moves out of `songchords` to the shared model (songchords keeps using it) and gains *open the Sections row* and *open Lyrics*; the window height becomes part of `Info` instead of the songchords special case.
- **`lyrics::isSung`** leaves Lyrics.cpp's anonymous namespace so the exercise uses the same rule as `lyrics::place`.
- **`LyricsLane`**: single-line tapping (stops after the line's last syllable, playback goes on) and the loop-wrap rule of decision 3; an area for a beat range for the pointer. The tap needs keyboard focus in the main window, so the action brings it to the front before `grabKeyboardFocus`.
- **UI wiring** as the recipe: `MainComponent` command, `showExercise`, `runExerciseAction`/`showExerciseTarget` cases, timer feed, context; `ArrangementView` gains `isSingAlongShowing`, getters for the Lyrics header's *Edit...* and *Sing along* buttons and the menu callback.
- **Invariants:** no tick from a button press; the exercise writes no lyrics, sections or notes itself; find the line fresh each time (never keep an index into `Lyrics`).
- Threading: message thread only; the loop is read from the engine's atomics. No audio-thread or format change. Risk: the tapping change touches a shipped interaction; the existing tap tests must pass unchanged with the loop off.

## Libraries and models

None. No `LICENSE`, `licenses/` or *License note* change. `rhyme::countSyllables` uses the CMU Pronouncing Dictionary DawIt already ships.

## Theory

No new guide content. One new fact was looked up (13); the rest are DawIt behaviour, derived, or in the lyrics guide with its sources in `SOURCES.md`.

### Facts

1. A line is usually sung over a phrase of 2 bars, sometimes 1 or 4. Typical; Lyrics · *Lines and phrases*.
2. A line usually ends before its phrase does, leaving room to breathe. Typical; same topic.
3. A verse tells the story with new words; a chorus says what the song is about with the same words each time; intros, solos and drops usually have no words. Typical; Lyrics · *What each part says*.
4. DawIt treats verse, pre-chorus, chorus, post-chorus, bridge and phrase sections as sung. Exact (`lyrics::isSung`).
5. Show, don't tell; everyday words over clever ones. Opinion; Lyrics · *Writing tips*.
6. Count syllables, not words: "together" is three. Exact; Lyrics · *Lines and phrases*.
7. Stressed syllables belong on strong beats; in 4/4 beat 1 is strongest and beat 3 next. Typical (the rule) and exact (metre); Lyrics · *Stress and the beat*.
8. Hyphens give each syllable its own place in time; unplaced syllables are spread evenly between placed ones. Exact (`lyrics::parse`, `lyrics::spread`).
9. Each tap places the next syllable at the heard playhead (output latency taken off); tapping a line's first syllable moves the whole line. Exact (`lyrics::tap`, `getHeardPositionBeats`).
10. A tap snaps to 0.25, 0.5 or 1 beat, finer as you zoom in. Exact (`LyricsLane::snap`).
11. A quarter beat at 96 BPM lasts 0.156 s, so a tap up to 0.078 s early still snaps to the right quarter. Exact (derived: 60 / 96 / 4 = 0.15625 s; half of it 0.078 s).
12. A line may start just before the bar line on a small word (a pickup). Typical; Lyrics · *Words on a melody*, *Singing along*.
13. People tapping along to a beat usually tap a little early, by some tens of milliseconds (the negative mean asynchrony). Typical; van der Steen and Keller 2013 (Frontiers in Human Neuroscience, read); Repp and Su 2013 (Psychonomic Bulletin and Review, from the search). Text says "a few hundredths of a second".
14. Breathe in the rest before a line; come in when the colour starts. Opinion (teachers' tips); Lyrics · *Singing along*.
15. Phrases come in pairs and fours; XAXA, lines 2 and 4 rhyming, is the most common scheme in pop and folk. Typical; Lyrics · *Lines and phrases*, *Rhyme*.
16. More than 2.5 syllables a beat is fast to sing. Opinion (DawIt's existing *Your lyrics* threshold).
17. *Loop this section* loops exactly the section's bars; Sing along follows the loop. Exact (`SectionLane`, `LyricsSingAlong`).
18. Write with AI needs Ollama on the computer. Exact (existing panel).

### Texts

Title "Write lyrics to your song". Intro: "Four steps to a first sung line: pick a part of your song, write a line for it, time it by tapping along, then sing it with the words in front of you. Do each step in the main window: the tick appears by itself."

1. **Pick a section** — "Choose the part your line is for"
   - Do: "Press Open the Sections row, click a verse or a chorus there and choose Loop this section. The loop plays that part over and over, so you can listen, write, tap and sing without rewinding. A chorus is a good place to start: its few lines say what the song is about."
   - Why: "Each part of a song has its own job, and the words follow it. A verse tells the story, with new words each time. A chorus says what the song is about, with the same words every time it comes back. Intros, solos and drops usually have no words, so DawIt looks for a part that is usually sung: a verse, pre-chorus, chorus, post-chorus, bridge or phrase."
   - Status, no sections: "Your song has no sections yet. Click an empty part of the Sections row, above the chords, and choose Verse or Chorus, or start from a song sketch with File > New Project from Style...."
   - Status, no sung sections: "Your song's sections, <Intro and Solo>, usually have no words. Click one, choose Type and make it a Verse or a Chorus, or add one in an empty part of the row."
   - Status, loop across sections: "The loop runs across more than one section. Click a section and choose Loop this section."
   - Done: "<Chorus>, bars <13> to <20>, is your section."
   - Button tooltip: "Shows the Sections row and points at <Chorus>. Click a section and choose Loop this section."
   - Show me: "<Chorus>: click it and choose Loop this section." With no sung section: "Click here to add a section, then choose Verse or Chorus."
2. **Write a line** — "One sung line for your section"
   - Do: "Press Open Lyrics... and type one line for <Chorus>, the way you would sing it. Put hyphens between the syllables of longer words, ri-ver, to-ge-ther, so each gets its own place in time. Above it, type the part's name in brackets, such as [Chorus], to put it on that kind of section. Then press Apply lyrics to song. If Ollama is installed, the window's Write with AI panel can suggest lines, but your own words are the point here."
   - Why: "A line of lyrics is sung over a phrase of the music, usually 2 bars, sometimes 1 or 4. Leave room to breathe: a line usually ends before its phrase does. Everyday words, said naturally, work better than clever ones, and a concrete picture, two cups on the table, one still full, says more than naming a feeling. Count syllables, not words: \"together\" is three."
   - Status, no section: "Pick a section first (step 1): the line is written for it."
   - Status, no line in the section: "Your lyrics have <12> lines, but none is sung in <Chorus>, bars <13> to <20>. On the Lyrics row, drag the bar above a line's words into the section, or type a new line."
   - Done: "\"<hold on, hold on tonight>\": <6> syllables over <8> beats." Added when over 2.5 a beat: " At <96> BPM that's fast to sing: give it more beats, or cut words."
   - Button tooltip: "Opens the Lyrics window to type your line. Nothing in the song changes until you press Apply lyrics to song."
   - Show me: "Edit...: type your line, then press Apply lyrics to song."
3. **Time it by tapping** — "Put each syllable where it's sung"
   - Do: "Press Tap this line. The song plays from a bar before your line. Sing it in your head with the music and press any letter key the moment each syllable comes: each tap puts the next syllable there, and the first moves the whole line. Tapping stops after the last syllable, and with the loop on you can tap it again on the next pass. You can also drag a syllable on the Lyrics row; Alt drags without snapping."
   - Why: "Until you place them, DawIt spreads a line's syllables evenly, which is rarely how words are sung. Words have a stressed syllable, to-GE-ther, and stressed syllables belong on strong beats: in 4/4, beat 1 is the strongest and beat 3 the next. Tapping is the quickest way to put each syllable where you hear it. DawIt takes your sound card's delay off each tap, and snaps it to the nearest quarter, half or whole beat, finer as you zoom in. Most people tap a little early, a few hundredths of a second ahead of the beat, and the snapping usually puts those taps back on it. A line may start just before the bar line on a small word, a pickup: it still belongs to its section."
   - Status, no section: as step 2. No line: "Write a line first (step 2)."
   - Status, partly placed: "<2> of the line's <8> syllables aren't placed yet. Tap the line again from its start."
   - Status, one block: "Your line is a single syllable block. Write a longer line, with hyphens between syllables, so there's something to tap."
   - Tip, hyphens: "Tip: <along> and <river> sound like more than one syllable but are one block each, so they get one tap. In the Lyrics window, put hyphens between their syllables (ri-ver) to tap each one."
   - Tip, nothing sounds: "Nothing plays in <Chorus> yet, so there's nothing to tap or sing to. Add chords and a beat first (Help > First Song Exercise...), or turn on the metronome in the transport bar and tap to its click."
   - Done: "All <6> syllables of your line are placed."
   - Button tooltip: "Plays from a bar before your line. Press any letter key the moment each syllable is sung. Tapping stops after the last syllable; Esc stops it sooner."
   - Show me: "Your line. Press Tap this line and tap a letter key on each syllable, or drag a syllable to place it."
4. **Sing it** — "Sing your line with the words in front of you"
   - Do: "Press Open Sing along (Cmd+5). It shows the line to sing now in large letters, and the colour runs over each syllable as it's sung, just as you tapped it. Press Space and sing when your line comes round: with the section looping, you get another go each pass. The tick comes once the song has played through your line with Sing along open."
   - Why: "Singing a line out loud is its real test. A word that's hard to sing, or a stress on the wrong beat, shows at once: go back to step 3, or change a word. Breathe in the rest before the line, not in the middle of a word, and come in when the colour starts, which may be just before the bar line. DawIt doesn't listen to you: the tick only says the song played your line with the words in front of you."
   - Status, no section / no line: as step 3. Nothing sounds: step 3's tip.
   - Status, no output: "No audio output is chosen, so you won't hear the song. Choose one in Audio/MIDI Settings."
   - Status, Sing along open, not yet done: "Sing along is open: press Space and sing when your line comes round."
   - Done: "The song played through your line with Sing along open."
   - Button tooltip: "Opens Sing along (Cmd+5): your words in large letters as the song plays. Press Space to play."
   - Show me: "Sing along (Cmd+5): your words, large, as the song plays."

Completion: "Done. You wrote a line for your song, put each syllable in time and sang it. Next, write the line that answers it: lines come in pairs, and in many songs line 2 rhymes with line 4. Then fill the section, and record your voice on an audio track while Sing along shows the words." Buttons *Open the lyrics guide* (tooltip "Opens How lyrics are written at Your lyrics: rhymes, syllables and gaps in your words") and *Start over* (tooltip "Back to step 1. Your song stays as it is.").

Lyrics row menu item tooltip-free (menu); Help description as under *Opening*.

Sentence added to *Your lyrics* when the song has no lyrics (after the existing text): "To write, time and sing a first line step by step, use Help > Write Lyrics to Your Song...."

### Example data (tests and shots)

- The example song's sections (`buildPopSong`): Intro bars 0–4, Verse 4–12, Chorus 12–20, Verse 20–28, Chorus 28–36 (0-based), 4/4, 96 BPM (0.625 s a beat, 2.5 s a bar). Chorus 1 spans beats 48–80 and shows as "bars 13 to 20".
- Line "hold on, hold on to-night": 6 blocks (hold, on, hold, on, to, night), 6 syllables by sound. Unhyphenated "I walked along the river late": 6 blocks, 8 by sound; the tip names "along" and "river".
- Tapped line for shots: the 6 syllables pinned at beats 48, 49, 50, 51, 52, 53 (end 55): midpoint 51.5 inside 48–80.

### "Your song" rules

None beyond the step statuses above (no guide topic).

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| `isSung` per type | verse, preChorus, chorus, postChorus, bridge, phrase true; intro, solo, interlude, breakdown, buildUp, drop, outro false | exact |
| Chosen section, loop on 48–80 / 52–64 / 0–16 / 40–56 / off | Chorus at 12 / Chorus at 12 / none (intro) / none (two sections) / none | exact |
| Remembered bar 4, loop off / that section retyped Solo / loop on 48–80 | Verse at 4 / none / Chorus at 12 | exact |
| Counts for Chorus: line 47.5–54 (midpoint 50.75) / 76–84 (80) / 44–50 (47) | yes / no / no | exact |
| Step 3: 6 blocks, 1–5 pinned / 1–3 pinned / a 1-block line | done / "2 of the line's 6" / one-block status | exact |
| Hyphen tip: unhyphenated / hyphenated line | names along, river / none | exact |
| Fast line: 6 syllables in 2 beats (3.0 a beat) / in 8 beats (0.75) | warning / none | exact |
| Sung tracker, line 48–56 at 96 BPM (5 s): samples 49, 53 / 49 only / 49, stop, 53 / 53, 49, 50 / then 53 / 53 with Sing along closed | done / not / not / not / done / not | exact |
| Short line 48–48.75 at 96 BPM (0.47 s): one sample at 48.4 | done | exact |
| Half of a 0.5 s line vs the 5 Hz period | 0.25 s ≥ 0.2 s | exact |
| Quarter beat at 96 BPM; half of it | 0.15625 s; 0.078 s | 1e-6 |
| Steps, links, stems | 4 steps; every link names a topic; stems distinct | exact |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. Shared model: move `Fix` to `gen::exercises` with the two new values, window height into `Info`, `lyrics::isSung` public. Tests: songchords and First song checks unchanged; `isSung` row. Done when they pass.
- [x] 2. `gen::writelyrics` (`src/generate/WriteLyrics.{h,cpp}`, both CMake targets): texts from *Theory*, the chosen-section, counts-for and line rules, done rules, statuses, tips, done notes, sung tracker; registered in `gen::exercises` with `Context` fields. Tests: the *Test numbers* rows on made-up edits (example sections, made-up lyrics). Check: texts have placeholders filled for the example song.
- [x] 3. The *Your lyrics* sentence in `LyricsGuide.cpp`. Tests: the empty-lyrics text contains "Write Lyrics to Your Song". Done when `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 4. `LyricsLane`: single-line tapping and the loop-wrap rule (decision 3); a beat-range area getter. Tests: a pure helper for "which line the next tap goes to after a wrap" in `lyrics::` with checks (loop 48–80, tapping line 2 at 76 then wrap to 48 → line 1 again). Check by hand: tap with the loop on.
- [x] 5. `MainComponent` and `ArrangementView`: Help command, Lyrics row menu item, `showExercise (writeLyrics)`, actions and *Show me* per the table (bring the main window forward before tapping), the remembered section and sung tracker fed from the 5 Hz timer, `isSingAlongShowing`, header button getters, reset in `afterReplace` and *Start over*.
- [x] 6. `ScreenshotRunner`: `"exercise": "writeLyrics"` with the existing step, action and show-me keys; new keys `"clearLyrics"`, `"loopSection": n` (loops the nth section as its menu does), `"lyricsLine": "text"` (applies text as *Apply lyrics to song* does), `"tapLineNow"` (pins the line's syllables a beat apart from its start, as tapping would), `"exerciseSung"` (marks the sung record). Update `docs/recipes.md` *Checking in the real app* keys.
- [x] 7. Done when `./build.sh --test` passes without warnings and the `firstSong` and `songChords` shots look as before.

Group C, check and docs:
- [x] 8. Real-app check (`"capture": "window"` for the exercise, `"main"` where a row matters; example song with `clearLyrics`): step 1 with no loop, *Show me* on Chorus 1; after `loopSection` 2 (ticked, "Chorus, bars 13 to 20"); step 2 before and after `lyricsLine` "[Chorus]\nhold on, hold on to-night" (line on the Lyrics row in bars 13 to 20); step 2 with the unhyphenated verse line (hyphen tip at step 3); step 3 *Show me* (outline on the line), after `tapLineNow`; step 4 with `exerciseNoOutput`, with Sing along open, after `exerciseSung`; completion; step 1 on an empty song (no sections line). Look for cut text, four rows fitting at 680, the pointer not covering its target. By hand: *Tap this line* with the loop on, two passes; Sing along through the line.
- [x] 9. Docs: README (extend the exercises bullet); `docs/tests.md` (writelyrics, isSung, tap wrap); `docs/code-layout.md` (WriteLyrics); `docs/ui-design.md` (exercise window heights per exercise, *Tap this line*); `docs/recipes.md` *A guided exercise* (`Fix` and height now in `gen::exercises`); GitHub #204 progress (Done / Still open), #195's list, plan status per the workflow.

## Tests

`runWriteLyricsTests()` in `tests/EngineTests.cpp`: the *Test numbers* rows on made-up edits (the example song's sections without lyrics, lines set with `lyrics::setText` or built with pins); every text non-empty with placeholders filled; the exercise's links resolve in `getEntries()` and its stem differs from the others; First song and song chords checks unchanged after the `Fix` move. In the lyrics guide tests: the *Your lyrics* sentence. In the lyrics tests: the wrap helper.

## Website hand-off

- `features.html`, learning group: guided exercises, now three; this one writes, taps and sings a first line.
- `learn.html`: beside the other two exercises, "Write lyrics to your song", linking to the lyrics guide.
- Screenshots: the main window with Chorus 1 looping, the line on the open Lyrics row and the exercise at step 3 ticked; Sing along with the exercise at step 4. No `about.html` change.

## Left for later

- Listening to the singing (microphone level or pitch against the line), and a step that records a vocal take over the line: after this, in #204.
- A list of all exercises and a section-menu entry: #195.
- Syllables following a melody clip, and stresses checked against the beat with the dictionary: #24.
- Keeping the remembered section and the sung record across restarts: only if users ask.

## Changes while building

Group A (engine), hand-off for Group B:
- `src/generate/WriteLyrics.{h,cpp}` (`gen::writelyrics`) holds the rules. Registered as `Exercise::writeLyrics` in `gen::exercises` (`numExercises` is 3). `Info::windowHeight` is 500 / 740 / 680 and `ExerciseWindow` uses it (the `windowHeight`/`tallWindowHeight` constants are gone). `Fix` now lives in `src/generate/ExerciseFix.h` (`gen::exercises::Fix`, `songchords::Fix` is an alias) with the new `openSections` and `openLyrics`; `runExerciseAction` must handle both (nothing in `MainComponent` handles them yet).
- `ex::Context` gained `writeLyricsSectionBar` (-1 none), `sung` (`writelyrics::SungRecord`) and `singAlongOpen`. `MainComponent` owns a `writelyrics::SungTracker` and feeds it from the 5 Hz timer: `tracker.feed (engine.getHeardPositionBeats(), playing, singAlongShowing, &span, secondsPerBeat)`, with `span` from `writelyrics::findLine (engine.getLyrics(), *section, engine.getBarLength())` and `getLineText`; `context.sung = tracker.getRecord()`. Call `tracker.reset()` in `afterReplace` and *Start over*. Remember the chosen section's start bar in `writeLyricsSectionBar` whenever `writelyrics::findChosenSection (engine, progress)` has a value and the loop is on.
- Helpers for the UI: `writelyrics::getShowMeText (step, sectionName)` (the pointer sentences), `findChosenSection (engine, progress)`, `findFirstSungSection`, `findLine`, `getLoop`, `getBarsText`. `lyrics::isSung` is public in `Lyrics.h`.
- Step 1's action label/tooltip come from `ex::getAction`; with `Fix::none` the button is the step's own (step 3's *Tap this line* and step 4's *Open Sing along* are `Fix::none`, so `runExerciseAction` must dispatch on step for the writeLyrics exercise).
- The *Your lyrics* sentence is in `LyricsGuide.cpp`; its tests are in the lyrics guide check.

Changes while building (Group A):
- Texts the plan lacked: step 1 with the loop inside an unsung section: "The loop is in <Intro>, which usually has no words. Click a verse or a chorus and choose Loop this section." (fix: open the Sections row). Step 3's hyphen tip has a one-word form: "Tip: along sounds like more than one syllable but is one block, so it gets one tap. In the Lyrics window, put hyphens between its syllables to tap each one." The words in tips and notes are shown without angle brackets.
- `Progress`/`Context` also carry `singAlongOpen` (step 4's "press Space" line needs it).
- Status lines for steps 3 and 4 join their messages and tips with a newline.

Group B (UI), hand-off for Group C:
- `lyrics::getWrapTarget` (Lyrics.h) is the pure wrap rule; `LyricsLane::startTapping (fromBeat, onlyLine)` and `startTappingLine (line)` do single-line tapping (stops after the line's last syllable, playback goes on); `LyricsLane::getBeatRangeArea (start, end)` is the pointer area; the Lyrics row's menu item *Write lyrics step by step...* (`onWriteLyrics`, above the guide item) opens the exercise. The loop-wrap rule applies to all tapping (loop on, playing, heard beat before the last tap: next tap goes to the loop's first line, or the single line); the existing tap tests pass unchanged.
- `ArrangementView`: `getLyricsLane`, `getLyricsEditButton`, `getLyricsSingAlongButton`, `isSingAlongShowing`, `showSectionsRow`, `showLyricsEditor` (now public), `onWriteLyrics`.
- `MainComponent`: `writeLyricsExerciseCommand` (Help, after *Learn the Chords of a Song...*), `runWriteLyricsAction`, `showWriteLyricsTarget`, `feedWriteLyrics` (5 Hz timer), `resetWriteLyricsMemory` (afterReplace and Start over), `setWriteLyricsSungForShot`.
- `ScreenshotRunner`: `"exercise": "writeLyrics"` plus `clearLyrics`, `loopSection` (0-based: the example song's Chorus 1 is 2), `lyricsLine`, `tapLineNow`, `exerciseSung`, applied in that order before the window opens. Documented in `DevScreenshots.h`; the plan's *Checking in the real app* doc in `docs/recipes.md` is NOT yet updated (Group C).
- Not done in Group B: the `firstSong` and `songChords` shot comparison (step 7) and the manual tap/sing checks; `./build.sh --test` passed. *Show me* for step 1 with no sung section points at the first free 4 bars; the Sections row is unfolded by every *Show me* and by *Tap this line*.

Group C (check and docs):
- Real-app check done with `--screenshots` (window captures for the exercise, main crops for the Lyrics row with Chorus 1 looping): every step, the loop-on state, the hyphen tip, completion, an empty song, and `firstSong` / `songChords` as before. Change: the window is 360 x 780, not 680: step 3 with a status line, the hyphen tip and two links scrolled and squeezed its buttons at 680 (`writelyrics::windowHeight`, test updated).
- Step 1's *Show me* points at the first sung section (Verse, bars 5 to 12 in the example song), not Chorus 1, as the rule says when nothing is looped.
- Not done by hand (needs a person at the keyboard): *Tap this line* with the loop on over two passes, and Sing along through the line with real playback; both are covered by the tests (`getWrapTarget`, the sung tracker) and the shots with `tapLineNow` / `exerciseSung`.
- First steps: checked, no change.

Theory check:
- All shipped texts match *Theory* (and the three texts added while building); the early-tap fact (13) re-checked: taps lead a metronome by some tens of milliseconds on average, so "a few hundredths of a second" stands; snapping is to the nearest grid line (`std::round`), so early taps do go back on the beat. The completion's "line 2 rhymes with line 4" agrees with Lyrics · *Rhyme* (XAXA the most common). No text or test changed.
- `SOURCES.md` (*Not for a guide*): the proxy link for the musicians/non-musicians paper replaced by its DOI link, a stray blank line in the list removed, and the check's search added.
- Open (needs code): step 2's done note and the fast-line rule count syllable blocks, while *Your lyrics* counts syllables by sound (`rhyme::countSyllables`), so "I walked along the river late" shows 6 here and 8 in the guide. Step 3 also ticks for that line once its 6 blocks are tapped, and the hyphen tip then disappears.

dawTheory fixes (user's choices):
- Step 2's done note and `isLineFast` count syllables by sound, as the lyrics guide's *Your lyrics* topic does (`writelyrics::countSungSyllables`: `rhyme::countSyllables` on the hyphenated text), so "I walked along the river late" is 8, not 6 blocks. Step 3's "All N syllables ... placed" still counts blocks, since it is about taps.
- Step 3 still ticks from the real timing (every block after the first pinned). Its done note now repeats the hyphen tip on a second line while a multi-syllable word is one block (`getHyphenTip`, shared with the status line). Tests added for both.

Step memory (user's change, 2026-11-15, all exercises): this exercise's step is no longer in app settings (`learn.writeLyrics.step` is not written); it is in memory (`MainComponent::exerciseSteps`) and starts at step 1 after a new, opened or restored project or a restart. Only `learn.writeLyrics.done` stays. See the note at the end of `plans/guided-exercise-learn-the-chords-of-a-song.md`.
