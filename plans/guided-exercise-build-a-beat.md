# Guided exercise: build a beat

Status: Released
Idea: [#203 — Guided exercise: build a beat](https://github.com/geertvandamme/dawit/issues/203)

## Goal

A guided exercise that teaches rhythm by building one drum beat in your own song: kick and snare, then hi-hats, then velocity so it grooves, then swing. Each step ticks from the notes really in the drum clip, points at the control, names the drum notes and links to the guide topic behind it. Swing needs a tool DawIt lacks, so Quantise gets a **Swing** setting, and the rhythm guide gets a *Swing* topic. A beginner ends with a beat of their own and knows what the backbeat, accents and swing are.

## Decisions for you

1. **Where swing comes from.** Options: (a) a *Swing* setting in the Quantise dialog (50 to 75%, the scale of the MPC, Logic and DawIt Arpeggiator), for the 1/8 and 1/16 grids; (b) a *Swing* item in the piano roll's Transform menu; (c) leave the swing step out (three steps). **Recommended:** (a). Quantise already has the grid, strength, a picture and Cmd+Z in one step. It also fixes the rhythm guide's "DawIt's Quantise has no swing setting yet". Dragging notes with Alt also works, and also ticks the step.
   Answer: Update R4 and add R5 to recognise steady swing. Help and the drum track header menu. Tick from the song as it is. Any one drum, with a difference of at least 16 velocity steps. Swing in Quantise, 50–75% for eighth and sixteenth grids.
2. **What counts as "it grooves" (step 3).** Options: one drum's hits in one bar differ by at least 16 velocity steps (of 127); or hi-hats only; or a ghost note (a snare at 40 or less). **Recommended:** any one drum, 16 or more. This covers hi-hat accents and ghost notes alike. The First song beat (hi-hats 89 and 70) passes, which is honest: it has accents.
   Answer:
3. **When a step is already done.** A beat written by First song ticks steps 1 to 3 at once. Options: tick from the song as it is (done notes explain what is there); or compare with a baseline taken when the window opens. **Recommended:** as it is, like the other exercises. The issue asks for ticks "from the real state of the song", and the done notes still teach.
   Answer:
4. **Where it opens.** Options: Help > *Build a Beat...*; also *Build a beat step by step...* in a drum track's header right-click menu; also a Start panel tile. **Recommended:** Help and the drum track's menu (just above the guide item). The Start panel already has a Drums tile, and #195's list of exercises will offer it later.
   Answer:
5. **The rhythm guide's advice for a swung part.** Today its *Your song's rhythm* rule R4 calls swung notes "between the grid lines" and suggests Quantise. **Recommended:** R4 skips notes that sit at a steady swing, and a new rule R5 names the swing ("Drums swings its sixteenths at about 60%").
   Answer:

## What the user does and sees

**Opening.** Help > *Build a Beat...* (after *Mix and Export Your Song...*, no shortcut, description "Kick and snare, hi-hats, accents and swing: build a drum beat in four steps"). The drum track's header menu has *Build a beat step by step...*. Both open the shared `ExerciseWindow`, "Build a beat", 360 × about 640 (fix it by shot at the longest step).

**The beat clip.** This is the selected MIDI clip if it is on a drum track (`firstsong::isDrumTrack`). Otherwise it is the first clip with notes on the selected drum track, then on the first drum track, then that track's first clip. The **groove bar** is the first bar of the clip that has a kick and a snare. Steps 2 to 4 look there first, then at any bar.

**Drum roles, by General MIDI note:** kick 35, 36; snare 37 (side stick), 38, 39 (clap), 40; hi-hat 42, 44, 46, and the ride 51, which keeps time the same way. Other notes play no role in the checks.

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Kick and snare | a bar of the beat clip has a kick and a snare | no drum track: *Add drums* (as the Start panel's Drums tile: `addInstrumentTrack (BuiltInSound::drums)`); a drum track: *Open the beat* (select the clip, open the clip editor; a track with no clip first gets an empty 4-bar clip at bar 1) | the piano roll's grid |
| 2 Hi-hats | the groove bar (else any bar with a kick and a snare) has 4 or more hi-hat or ride notes | *Open the beat* | the piano roll's Snap box |
| 3 Make it groove | in some bar, one drum's hits differ by 16 or more velocity steps (rounded `velocity × 127`) | *Open the beat* | the piano roll's *Velocity* button |
| 4 Add swing | the swing measure (below) finds swing in some bar | *Quantise with swing...*: the Quantise dialog on the whole beat clip, starting at grid 1/8 when the groove bar's hi-hats move in eighths, else 1/16, swing 58%, strength 100%; nothing moves until the user presses Quantise | the dialog's Swing slider while it is open, else the piano roll's *Quantise...* button |

**Swing measure** (pure, shared with Quantise's tests and the rhythm guide). For one drum in one bar, and for a pair of 1 beat (swung eighths) or 0.5 beats (swung sixteenths), counted from the bar start: a note is **on** when it starts within 0.02 beats of a pair's start. Otherwise its place in the pair (0 to 1) makes it **swung** in 0.53 to 0.72, **straight** at 0.5 ± 0.03, or **other**. Swing is found when 2 or more notes are swung and they outnumber the straight and other notes. The percentage is the mean place of the swung notes. 0.72 is the upper limit so that a dotted figure (0.75) is not called swing.

**Done notes** (one line, from the song): step 1 names where the snare falls by count ("Snare on 2 and 4: the backbeat"; "Snare on 3: a half-time feel"; else "Snare on 2 &"). Step 2 names the hi-hat spacing ("8 hi-hats: eighth notes, 1 & 2 & 3 & 4 &"). Step 3 names the drum and its range ("Hi-hat from 70 to 100"). Step 4 names the swing ("Hi-hats swing their eighths at about 60%").

**Status lines (missing prerequisites and tips):**
- No drum track: "A drum track plays the Drums sound or DawIt Drums." Its action is *Add drums*.
- A drum track whose notes include no kick or snare note number (a kit with its own map): name the notes to use.
- Step 4 with the hi-hats on eighths and the dialog at 1/16: Swing at 1/16 moves only sixteenths, so the eighths stay put. Choose 1/8.
- Swing found above 0.72 (dotted): "75% is a dotted rhythm, sharper than swing: try 55 to 70%."
- No audio output: fix *Audio/MIDI Settings...* (existing `Fix::audioSettings`), on every step.

Nothing changes the song without the user's action. *Add drums* and *Open the beat* (empty clip) are labelled actions. Quantise only applies on its own button.

**Quantise dialog.** A **Swing** slider under Strength runs 50 to 75% in 1% steps, with 50% meaning off. It is enabled for the 1/8 and 1/16 grids. For triplet and 1/4 grids it is greyed, and its tooltip says why. The picture draws the swung lines. The summary line adds "with swing 60%". The setting is remembered as `quantise.swing` with the others, and Q repeats it.

**Remembered:** `learn.buildBeat.done` only (the step is in memory, as for every exercise). `quantise.swing` in app settings.

**First steps:** no change. First song's step 3 still writes its basic beat, and no command or control it names changes. The Start panel's Drums tile is unchanged; this exercise opens from Help and the drum track's menu.

## Data and saving

There is no project data and no format change. Quantise with swing changes note starts only, as Quantise already does (undone in one step). The beat clip is not stored; it is found again on each ask. `exercises::Context` gains the selected track and clip, which are compared with the engine's tracks before use, as for the mixing exercise.

## The guide

**New topic in the rhythm guide** (`gen::rhythm`): **Swing**, after *Feel and the grid* and before *Your song's rhythm*. Summary: "Long-short pairs, 50 to 75%". The picture uses the explainer's playable rows: one bar of hi-hat eighths at 50%, 58%, 66% and 75%, and one bar of sixteenths at 60%. Each row's caption gives its percentage and the delay at the song's tempo, in ms.

**Changed text** (*Theory* has the new paragraphs): *Feel and the grid* (Swing on a straight grid; the LM-1 paragraph's last sentence), *Quantising* (the "Triplets and swing" sentence and the dialog description), and the Quantise grid tooltip.

**Your song's rhythm:** R4 skips notes at a steady swing; a new rule R5 is in *Theory*.

**Links** (`TheoryGuideLinks.cpp`):
- rhythm · Swing | arpeggiator · Swing (the knob page, same scale)
- rhythm · Swing | instruments · Drums and percussion
- rhythm · Swing | drums · Velocity, accents and groove

**gen::guides entry:** the rhythm guide's search words gain "swing, shuffle, groove, MPC swing". Its `opensFrom` gains "the Quantise dialog's Swing" and "Build a Beat". Not on *Start here* (the rhythm guide's place is unchanged).

**Exercise links** (one or two per step): 1 instruments · Drums and percussion, rhythm · Counting; 2 rhythm · Counting, rhythm · Reading DawIt; 3 midi · Velocity, drums · Velocity, accents and groove; 4 rhythm · Swing, rhythm · Quantising. Carry on: rhythm · Swing.

## Code

Follows recipes.md *A guided exercise* (copy *Write Lyrics to Your Song*: no recording, a pure model on the `Engine`), *Topics in an existing guide*, and *A command or menu item*.

- `gen::buildbeat` (`src/generate/BuildBeat.{h,cpp}`, both targets): content, the beat-clip choice, drum roles, the four done rules, done notes and statuses. All pure on `Engine` and `Context`. It is registered in `gen::exercises` with stem `learn.buildBeat`.
- **Swing grid** next to `quantise::` (`MidiQuantise`): `Settings` gains a swing (0.5 to 0.75; 0.5 = off). With a 1/8 or 1/16 grid, every second line, counted from each bar's start, moves from `(k)·g` to `(k−1)·g + 2·swing·g`; notes go to the nearest swung line, then strength applies as now. Triplet and 1/4 grids ignore it. `describe` names it. An odd line count in a bar (5/8) leaves the last line straight.
- **Swing measure:** a pure function in the same place. It takes note starts of one pitch in one bar and returns swung, straight and other counts and the mean percentage for each pair size. `gen::buildbeat` and `gen::rhythm` (R4, R5) use it; `gen::rhythm`'s `SongRhythm` gains per-clip notes (name, starts by pitch) for R5.
- `MainComponent`: command, `showBuildBeat`, context, `runExerciseAction` (add drums, open the beat, open Quantise with a start state), `showExerciseTarget`. `QuantiseDialog` takes an optional start state (grid, swing) without saving it until applied, defaulting to today's behaviour. Add getters for the Snap box, Velocity and Quantise buttons and the dialog's Swing slider, if they are missing.
- **Threading:** all on the message thread under the usual lock. Quantise already edits through the undo history; nothing touches the audio thread.
- **Risk:** low. R4's change alters existing advice; keep its numbers for unswung parts identical (test).

## Libraries and models

None.

## Theory

### Facts

| # | Fact | Kind | Source |
|---|---|---|---|
| 1 | Swing as a percentage is the share of each pair of notes taken by the first. At 50% both are equal (straight); at 66.7% it is 2 : 1, a triplet feel. | exact | Roger Linn, Attack Magazine interview; Logic Pro Q-Swing |
| 2 | Only the second note of each pair moves later; the first stays on the grid. | exact | Linn ("delay the second 16th note within each 8th"); Logic ("every second point in the grid") |
| 3 | Linn's machines swing sixteenths; DAWs such as Logic swing whichever grid is chosen, eighths or sixteenths. | exact | Linn; Logic manual |
| 4 | The useful range is about 50 to 70%. Linn's examples: 54% loosens straight sixteenths without sounding swung; at 90 BPM, 62% feels looser than the 66% triplet. Logic calls 50 to 75% the most useful. | typical | Linn; Logic manual |
| 5 | At 75% the pair is 3 : 1, a dotted eighth and a sixteenth (or a dotted sixteenth and a thirty-second). | exact | derived: 0.75 / 0.25 = 3 |
| 6 | The delay is (swing − 50%) × the pair's length. Sixteenths at 60%: 0.10 × 0.5 beats = 0.05 beats = 25 ms at 120 BPM. Eighths at 66%: 0.16 × 1 beat = 0.16 beats = 80 ms at 120 BPM. | exact | derived (a beat is 500 ms at 120 BPM) |
| 7 | Programs use different scales: in FL Studio and Cubase 0% is straight, and their 100% matches about 66.6% on the MPC and Logic scale. | exact | Attack Magazine, *Swing in DAWs* |
| 8 | Swing on drum machines started as "shuffle" on Linn's LM-1. | exact | Attack Magazine; already in the rhythm guide (Wikipedia, Linn LM-1) |
| 9 | Jazz players swing eighths near 2 : 1, wider at slow tempos and nearer even at fast ones. | typical | already in the rhythm guide (Wikipedia, Swing) |
| 10 | Shuffle and swing are used loosely; "shuffle" usually means a steady triplet long-short feel, as in blues. | typical | Soundbrenner, *Swing*; Benny Sutton glossary; blues sources already in Rhythm |
| 11 | The backbeat is the accent on beats 2 and 4 of 4/4, usually the snare or a clap. A basic rock beat: kick on 1 and 3, snare on 2 and 4, closed hi-hat on every eighth. | exact | Wikipedia, *Beat (music)*; Soundbrenner, *Backbeat*; already in Instruments |
| 12 | Half time: the snare on beat 3 only, so the same tempo feels slower. | typical | already in the rhythm guide (*Tempo and time*); Soundbrenner |
| 13 | Four on the floor: the kick on every beat (house, disco). | exact | already in Styles (Wikipedia, *Four on the floor*) |
| 14 | General MIDI drum notes: 35 and 36 kick, 37 side stick, 38 and 40 snare, 39 clap, 42 closed hi-hat, 44 pedal hi-hat, 46 open hi-hat, 51 ride. | exact | already in MIDI and Drums sources (Wikipedia, *General MIDI*) |
| 15 | Velocity runs 0 to 127. As a rule of thumb, hi-hats about 90 to 110 on the beat and 60 to 80 between, ghost notes about 20 to 40. | exact / opinion | MIDI 1.0; Instruments guide's rule of thumb |
| 16 | Quantising swung eighths to a straight 1/16 grid moves each short note from 0.67 to 0.75 of the beat. | exact | already in the rhythm guide |

The exercise's thresholds (16 velocity steps, a swung place of 0.53 to 0.72) are DawIt's choices, not facts. No text states them as rules.

### Topic text: Swing

**Name:** Swing  **Summary:** Long-short pairs, 50 to 75%

Swing

Play a run of eighth notes evenly and they sound straight: 1 & 2 & 3 & 4 &. Make the first of each pair a little longer and the second a little shorter, and the same notes start to bounce. That long-short feel is swing, and it's in jazz, blues, funk, hip-hop, house and a lot of pop.

Drum machines and DAWs measure swing as a percentage: the share of each pair that the first note takes.
  50%: both halves equal. Straight, no swing.
  54%: barely late. Straight sixteenths feel looser without sounding swung.
  58 to 62%: a clear bounce that still feels tight.
  66%: two thirds and one third, the triplet feel of a shuffle (1 trip let, playing the 1 and the let).
  75%: three quarters and one quarter: a dotted rhythm, sharper than swing.
Only the second note of each pair moves; the first stays on the grid. The useful range is about 50 to 70%, and the right amount is a matter of taste and tempo.

How far does a note move? Take the swing above 50% and multiply it by the length of the pair. Sixteenths at 60% at 120 BPM: a pair of sixteenths lasts half a beat, 250 ms, and 10% of that is 25 ms. Eighths at 66%: 16% of a 500 ms beat is 80 ms. Small numbers, but you hear them.

Eighths or sixteenths? Swing works on pairs of whichever note value you choose. Swing the eighths and the "&"s move late; that's the jazz and blues shuffle. Swing the sixteenths and the "e"s and "a"s move late, while every eighth stays where it was; that's the feel of a lot of funk, hip-hop and house. Swinging sixteenths does nothing to a hi-hat that only plays eighths.

In DawIt: open a drum clip in the piano roll, click Quantise..., choose 1/8 or 1/16, and set Swing. The picture shows where every note will go before you apply it, and Cmd+Z undoes it in one step. DawIt Arpeggiator's Swing knob uses the same scale.

Watch out:
  Other programs use other scales. In FL Studio and Cubase 0% means straight, and their 100% is about 66% here. Compare by ear, not by number.
  Swing a whole beat at once. Parts that play the same note values, such as the hi-hat and a bass line, usually sound together when they swing by the same amount.
  Swing isn't the same as loose timing. Swung notes are late by the same amount every time; a part played in by hand varies from note to note (Feel and the grid).

The picture plays one bar of hi-hat eighths at 50%, 58%, 66% and 75%, and sixteenths at 60%, with how late the second note of each pair is at your tempo.

### Changed texts

- *Feel and the grid*, Swing on a straight grid (replace the item): "Swing on a straight grid. Swung eighths are played long-short, close to the first two thirds and the last third of a beat. Quantise them to 1/16 and each short note jumps from 0.67 to 0.75 of the beat: a stiffer, dotted rhythm. Use the 1/8 triplet grid, or 1/8 with Swing at 66%, to keep the swing (see Swing)."
- *Feel and the grid*, last sentence of the LM-1 paragraph: replace "DawIt's Quantise has no swing setting yet: for a swung part, use a triplet grid." with "DawIt's Quantise has a Swing setting that does the same (see Swing)."
- *Quantising*: "Triplets and swing need a triplet grid (see Feel and the grid)." becomes "Triplets need a triplet grid; for swing, use a triplet grid or the Swing setting (see Swing)." In the "In DawIt" paragraph, after "Choose the grid and the strength;" insert "with 1/8 or 1/16, Swing moves every second line later;".
- Quantise grid tooltip: "...a triplet grid for triplets, or Swing for a swung part."

### Tooltips

- Swing slider: "Swing moves every second grid line later, so pairs of notes play long-short. 50% is straight, 66% the triplet feel of a shuffle, 75% a dotted rhythm. Works with the 1/8 and 1/16 grids."
- Swing greyed: "Swing works with the 1/8 and 1/16 grids. A triplet grid already swings."
- Help item: "Kick and snare, hi-hats, accents and swing: build a drum beat in four steps."
- Drum track menu item: "Build a beat on this track, step by step."

### Exercise texts

**Title:** Build a beat
**Intro:** A beat in four steps, on a drum track of your own song. Each step ticks when the notes are really there. Press Space at any point to hear where you are.

1. **Kick and snare**, subtitle "The heartbeat and the backbeat".
   Do: "In the drum clip, put the kick (C2, note 36) on beats 1 and 3, and the snare (D2, note 38) on 2 and 4. Click in the piano roll's grid to add a note."
   Why: "The kick and the snare carry the beat. The snare on 2 and 4 is the backbeat, the accent almost every rock and pop song has."
   Action: *Add drums* (tooltip "Add a track with the Drums sound and an empty clip, open in the piano roll") / *Open the beat* (tooltip "Select the drum clip and open it in the piano roll").
2. **Hi-hats**, subtitle "Keeping time".
   Do: "Add the closed hi-hat (F#2, note 42) on every eighth note: 1 & 2 & 3 & 4 &. Set Snap to 1/8 to place them easily."
   Why: "The hi-hat fills the space between the kick and snare and tells everyone how fast the beat moves. Eighths sound steady; sixteenths sound busier."
3. **Make it groove**, subtitle "Loud and soft".
   Do: "Turn on Velocity (V) and drag the hi-hats on the &s down, to about 70, and leave the ones on the beat near 100."
   Why: "A drummer never hits twice the same way. Louder hits on the beat and softer ones between give the beat a pulse. A very soft snare between the backbeats, a ghost note at 20 to 40, adds more."
4. **Add swing**, subtitle "Long-short pairs".
   Do: "Click Quantise with swing..., choose 1/8 if your hi-hats play eighths (1/16 for sixteenths), set Swing to about 58% and press Quantise. Play it, then try 66%."
   Why: "Swing makes the first note of each pair longer and the second shorter. 50% is straight; 66% is the triplet feel of a shuffle. A little swing makes a stiff beat bounce."
   Action: *Quantise with swing...* (tooltip "Open Quantise on the whole drum clip with Swing set. Nothing moves until you press Quantise; Cmd+Z undoes it.")

**Completion:** "You built a beat: kick and snare, hi-hats, accents and swing. Copy the clip to fill the song (Cmd+D), change it for the chorus, and add a fill on the toms at the end of a section." Carry on: *Read about swing* (rhythm · Swing).

### "Your song" rules (rhythm guide)

- **R4 change:** before counting a note as loose, leave out notes the swing measure calls swung in their clip, bar and pitch. Unswung parts give exactly the numbers they give today.
- **R5 (new):** a MIDI clip with swing in at least half the bars that have notes on a pitch. Advice: "<Clip track> swings its <eighths|sixteenths> at about <n>% (50% is straight, 66% a triplet feel). Parts that play the same note values usually sound together at the same swing (Swing)." One line per clip, at most three. It stays quiet when nothing swings.

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| Quantise 1/16, swing 60%: note at 0.25 | 0.30 | 1e-9 |
| Quantise 1/16, swing 60%, strength 50%: note at 0.25 | 0.275 | 1e-9 |
| Quantise 1/16, swing 60%: note at 0.27 | 0.30 | 1e-9 |
| Quantise 1/8, swing 66%: note at 0.5 | 0.66 | 1e-9 |
| Quantise 1/8, swing 75%: note at 0.5 | 0.75 | 1e-9 |
| Quantise 1/16, swing 50%: same as today on a mixed part | identical notes | exact |
| Triplet 1/8 grid with swing 60% | same as without swing | exact |
| Delay of sixteenths at 60%, 120 BPM | 25 ms | 0.01 |
| Delay of eighths at 66%, 120 BPM | 80 ms | 0.01 |
| Measure: hi-hats 0, 0.6, 1, 1.6, 2, 2.6, 3, 3.6 | eighths swung, 60% | 0.5% |
| Measure: hi-hats every 0.25 with 0.30, 0.80, ... | sixteenths swung, 60% | 0.5% |
| Measure: straight eighths; straight sixteenths | no swing | — |
| Measure: kick 0, 0.75, 2, 2.75 (dotted) | no swing | — |
| Step 3: hi-hats 100 / 70 | done | — |
| Step 3: First song beat (0.7 / 0.55 → 89 / 70) | done | — |
| Step 3: every note 0.8 | not done | — |
| R4: 4 bars, hi-hat eighths swung 60%, kick and snare straight (48 notes) | R4 silent, R5 says "eighths at about 60%" | ±1% |
| R4 on today's played-by-hand example | same count as before | exact |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. Swing in `quantise::Settings` and `apply`, and the swing measure, next to `MidiQuantise`. `describe` names swing. Tests: the Quantise and measure rows of *Test numbers*. Done when: the tests pass.
- [x] 2. Rhythm guide: the *Swing* topic (text from *Theory*), the changed texts, R4's change and R5 (per-clip notes in `SongRhythm`, filled where it is built today). Add the four links and the search words and `opensFrom`. Tests: topic has text, "swing" found by search, R4/R5 rows, links resolve. Done when: the tests pass.
- [x] 3. `gen::buildbeat`: beat clip, drum roles, done rules, done notes, statuses, all exercise texts. Register it in `gen::exercises` (enum, `allExercises`, `numExercises`, `getInfo`, stem `learn.buildBeat`, links, carry-on). Tests: below. Done when: the tests pass.
- [x] 4. `SOURCES.md` already has this run's lines (Rhythm). Check that nothing else needs adding. Run `./build.sh --test`. Done when: All checks passed, no warnings.

Group B, UI (`./build.sh --test` at step 8):
- [x] 5. Quantise dialog: the Swing slider under Strength, greyed for triplet and 1/4 grids, the picture's swung lines, `quantise.swing`, an optional start state. Check: open it on the example song's Drums clip at 1/16, 60%.
- [x] 6. Rhythm explainer: the *Swing* picture rows, using the song's tempo for the ms captions.
- [x] 7. `MainComponent`: Help command, the drum track header's menu item, `showBuildBeat`, the context (selected track and clip, output device), actions (add drums, open the beat, Quantise with start state) and *Show me* targets (grid, Snap box, Velocity button, Quantise button or the dialog's Swing slider), with getters where missing. Reset with the other exercises in `afterReplace`.
- [x] 8. `ScreenshotRunner`: `"exercise": "buildBeat"` (steps 0 to 3), and a `drumBeat` scene with `"beatStage"`: `"kickSnare"`, `"hats"`, `"accents"` or `"swing"`. It writes that stage of a 1-bar beat into a new "Drums" track's clip and selects it. `./build.sh --test`.

Group C, check and docs:
- [x] 9. Real-app check (`--screenshots`, `"song": "empty"` and the pop song): (a) the exercise at step 1 with no drums (*Add drums* shown), (b) after *Add drums* with *Show me* (pointer on the grid, `"capture": "main"`), (c) each `beatStage` with the matching step open (done notes "Snare on 2 and 4: the backbeat", "8 hi-hats", "Hi-hat from 70 to 100", "about 58%"), (d) step 4's action (the Quantise dialog at 1/8, 58%, swung lines; `"capture": "window"`), (e) `"guide": "rhythm"` at *Swing* with `"play": true`, and at *Your song's rhythm* after the swing stage (R5 line, no R4 line), (f) the longest step for the window height, (g) the existing `"quantiseDialog"` shot at a triplet grid (Swing greyed). Look for clipped text and wrong numbers.
- [x] 10. Docs: README bullet under Features (the exercise, Quantise's Swing, the Swing topic); `docs/tests.md` (Quantise swing, swing measure, Build a Beat, R4/R5); `docs/code-layout.md` (`BuildBeat.{h,cpp}`); recipes.md *A guided exercise* (name the fourth model and the new screenshot keys) and *Checking in the real app*; ui-design.md's exercise window height list. Issue #203 progress (keep it open), and a line in #195 and #9 (Quantise swing done; triplet Snap still open).

## Tests

- Quantise and measure: every Quantise and measure row in *Test numbers*. Swing ignored on triplet and 1/4 grids. 5/8 bar: the last line stays straight.
- Build a Beat model on made-up edits: no drum track gives the *Add drums* action and no tick. A non-drum MIDI clip is never the beat clip. Selected drum clip beats the first one. Step 1 kick + clap ticks, kick alone doesn't. Step 2 needs 4 hi-hats or rides in the groove bar or another kick-and-snare bar; 3 don't tick. Step 3 rows from *Test numbers*. Step 4 ticks on the measure's rows and not on straight or dotted parts. Done notes: "backbeat" for snares on 2 and 4, "half-time" for 3 only. A deleted selected track in the context is ignored.
- Registry: links name existing topics (`drums · Velocity, accents and groove` is a built-in page: skip in the engine test, check by shot). Stems are distinct. Texts are non-empty for all four steps. `getStaleSettingKeys` covers the new stem.
- Rhythm guide: *Swing* has text and an example; the search for "swing" and "shuffle" finds it; *Feel and the grid* no longer contains "no swing setting"; R4/R5 rows.

## Website hand-off

- `features.html`: the exercises group (beside the other guided exercises), and one line under editing MIDI for Quantise's Swing.
- `learn.html`: the rhythm guide's *Swing* topic.
- Screenshots: the exercise window at step 4 beside the piano roll with a swung hi-hat (pop song, Drums clip); the Quantise dialog at 1/8, 58% with swung lines.
- No library, so no `about.html` change.

## Left for later

- A drum lane view and a step editor to place hits (#52, #51); the exercise uses the piano roll.
- Triplet Snap in the piano roll (#9).
- A swing setting in the Generate panel's built-in patterns, and groove templates taken from a recording.
- Fills, open hi-hats and choke, and styles beyond the rock beat (four on the floor, half time, dembow): a later step or exercise, with the Styles guide.
- A Start panel tile and the #195 exercise list.

## Changes while building

**Group A, engine (built; hand-off for Group B).**

- *Quantise swing.* `quantise::Settings` has a fourth member `swing` (0.5 to 0.75; default 0.5, so every existing `{ grid, strength, ends }` still means today's quantise). `quantise::isSwingGrid (gridBeats)` (1/8 and 1/16 only) and `quantise::isSwinging (settings)` say when it applies; use them to grey the dialog's Swing slider. `describe` adds ", with swing 60%." to the summary line (and "... on the 1/16 grid with swing 60%." when nothing moves). `quantise::getSwingDelayMs (swing, pairBeats, bpm)` gives the ms for the captions. Constants `quantise::minSwing` and `maxSwing`. The Quantise grid tooltip in `QuantiseDialog.cpp` is already changed (the only dialog edit: no slider yet). Group A's UI edits are three: that tooltip and the two in `RhythmExplainer.cpp`.
- *Swing measure* (in `MidiQuantise`): `quantise::classifySwing`, `countSwing`, `findSwing (startsInBar)` returning `SwingFound { found, pairBeats (1 or 0.5), percent, swung }`.
- *Rhythm guide.* `Topic::swing` sits between `feelAndGrid` and `yourSong` (13 topics). `getQuantiseExample (Topic::swing, bpm)` returns the five rows (Eighths 50%, 58%, 66%, 75% and Sixteenths 60%), made with `quantise::apply`; each `QuantiseExampleRow` now also has `swing` and `pairBeats` (0 on the old rows), and its `detail` already holds the caption with the delay at `bpm` (default 120). `RhythmExplainer::getRows` already draws the Swing rows (the `T::swing` case beside *Quantising* and *Feel and the grid*, passing the song's tempo, so the ms captions follow it; added so the switch is complete and the build has no warning). Step 6 is left to check it by shot. `SongRhythm` has a fifth member `midiClips` (`ClipNotes { track name, startBeat, notes }`), filled in `RhythmExplainer::getSongRhythm`. R4 skips notes in steady swing; R5 is in `getAdvice`. The Swing links, search words (`swing swung shuffle mpc`), `opensFrom` and the changed texts are in.
- *`gen::buildbeat`* (`BuildBeat.{h,cpp}`, both targets; `Exercise::buildBeat`, `numExercises` 6). Everything the UI needs: `findBeat (engine, Progress)` (track and clip, either null), `isStepDone`, `getAction (step, ...)` (labels *Add drums*, *Open the beat*, *Quantise with swing...*, all `Fix::none`, so `runExerciseAction` dispatches on the label's state: no drum track means add drums; steps 1 to 3 with a drum track mean select the clip and open it, and a track with no clip first gets an empty 4-bar clip at bar 1; step 4 with notes means open Quantise), `getStatus` (audio output gives `Fix::audioSettings`, which `exercises::getAction` already turns into the audio settings button), `getDoneNote`, `getQuantiseStart (engine, progress)` (the `quantise::Settings` for the dialog's start state), `getShowMeText (step, quantiseDialogOpen)`, `getTrackMenuTooltip()`, `getHelpDescription()`, `windowHeight` 640 (fix by shot). `exercises::Context` gained `beatSelectedTrack`, `beatSelectedClip` and `beatQuantiseGrid` (the dialog's grid in beats while it is open, else the last used; 0 unknown: it only drives the "Choose 1/8" tip), all to be filled by `MainComponent::getExerciseContext`. Nothing in `MainComponent` or `ExerciseWindow` knows the exercise yet.
- *Half-done:* nothing. No `Exercise::buildBeat` UI is wired, so the app builds and behaves as before.

**Things that differed from the plan or surprised me**

- A steady 53 to 72% off-beat delay counts as swing: a hand-played part that is late by a constant 0.05 beats on every off-beat (55%) is measured as swing at 55%, so R4 leaves it out and R5 names it. The plan's *played-by-hand* example (varied lateness) gives R4's numbers exactly as before (tested).
- Step 4's "kit with its own map" status shows whenever the clip has notes but none is a kick or snare, as the plan says; it also shows while the user has only placed hi-hats so far. Its wording ("This clip has no kick or snare yet...") reads correctly then too.
- Step 3 and step 4 only look at kick, snare and hi-hat/ride notes (the plan's "other notes play no role"), per pitch (a closed and an open hi-hat are two drums).
- `Fix` was not extended: the three own actions use `Fix::none`.
- The theory guides test "Sources and Your song's rhythm are not counted" went from 11 to 12 pages, the rhythm topic-count test from 12 to 13, the done-keys test from 5 to 6.

**Group B, UI (built; hand-off for Group C).**

- *Quantise dialog* (`QuantiseDialog.{h,cpp}`): a **Swing** slider under Strength (50 to 75%, 1% steps), greyed with the plan's tooltip for triplet, 1/4 and 1/32 grids (`quantise::isSwingGrid`); the picture draws every second line later, in the accent colour; the window is 412 px high (was 380). `quantise.swing` is remembered with the other settings (`getLastSettings`/`saveLastSettings`, so Q repeats it). `showDialog (..., const quantise::Settings* start = nullptr)` opens with a start state that is saved only when Quantise is pressed. `quantiseui::getOpenGridBeats()` gives the open window's grid (0 when none).
- *Rhythm explainer:* the Swing rows were drawn in Group A; the clip signature now includes the swing so the Quantising picture refreshes.
- *`MainComponent`:* Help > *Build a Beat...* (`buildBeatExerciseCommand`, after *Mix and Export Your Song...*), the drum track header menu's *Build a beat step by step...* (`TrackActions::onBuildBeat`, drum tracks only, just above the guide items), `showBuildBeatExercise`, `getExerciseContext` fills `beatSelectedTrack`, `beatSelectedClip` and `beatQuantiseGrid` (the open dialog's grid, else 0), `runBuildBeatAction` (no drum track: `addInstrumentTrack (drums)`; else `openBeatClip (true)` selects the beat clip, adds an empty 4-bar clip if the track has none and opens the clip editor; step 4 with notes opens Quantise on the whole clip with `buildbeat::getQuantiseStart`), `showBuildBeatTarget` (grid, Snap box, Velocity button, Quantise... button through new `PianoRoll::getSnapBox/getVelocityButton/getQuantiseButton`). No extra memory to reset: the step lives in `exerciseSteps`, which `afterReplace` already resets.
- *Screenshots:* `"exercise": "buildBeat"` (`exerciseStep` 0 to 3); scene `drumBeat` with `"beatStage"` `kickSnare`, `hats` (eighths, all velocity 100), `accents` (the &s at 70) or `swing` (accents quantised to 1/8 at 58%): a one-bar "Beat" clip on a new track "Drums" (a second one next to the pop song's), selected and open in the piano roll; `"quantiseDialog"`/`"quantise"` take `"swing"`. As for `playedClip`, a scene runs after the shot's `quantiseDialog`, so make the clip in an earlier shot.
- *Checked in the app (not the Group C check):* the exercise window at step 4 and the Quantise window at 1/8, 58% on the hats stage (4 of 12 notes move, 0.08 beats, 40 ms, "with swing 58%"), the step-4 action opening the dialog with grid 1/8 and swing 58%. Looked right.
- *For Group C:* `windowHeight` is still 640 and is clearly too tall for steps 1 to 4 (about a third is empty): fix by shot at the longest step with a status line. A stage that completes all four steps shows the finished page, so use the stage before the step you want open.

**Where Group B differed from the plan**

- *Show me* for step 4 always points at the piano roll's *Quantise...* button: the Quantise window is modal and a separate window, and the pointer is an overlay of the main window, so it cannot outline the dialog's Swing slider. `getShowMeText (step, true)` is unused for now.
- The track menu item has no tooltip: JUCE popup menu items cannot carry one, so `getTrackMenuTooltip()` is unused (the Help command's description is shown).
- The drum track menu item and Help command were not added to First steps (no start panel change, as the plan says).

**Group C, check and docs (done).**

- *Real-app check* (`--screenshots`, scale 1, shot list in `/tmp/bbc/shots*.json`, images in `/tmp/bbc/out`): (a) step 1 with no drums: the *Add drums* button and the status "A drum track plays the Drums sound or DawIt Drums."; (b) after *Add drums* the pointer outlines the piano roll's grid; *Show me* at steps 2 and 4 outlines the Snap box and the Quantise... button with their sentences; (c) each stage with its step open: hats "8 hi-hats: eighth notes, 1 & 2 & 3 & 4 &", accents "Hi-hat from 70 to 100", and kick and snare (ticked); (d) step 4's action opens the Quantise window at 1/8, swing 58%, swung lines, "4 of 12 notes move, by up to 0.08 beats (40 ms at 120 BPM), with swing 58%."; (e) the rhythm guide at *Swing* (five playable rows) and at *Your song's rhythm* after the swing stage: "Drums swings its eighths at about 58% ..." and no R4 line; (f) the longest step (step 4 with a two-line audio status, two chips): 540 px high fits with the buttons in view; (g) the Quantise window on the 1/8 triplet grid with Swing greyed and the summary without "swing". `--export-guides` still passes (40 PDFs).
- *Fixed after looking:* the Swing rows' captions were cut off ("Eighths at 58%: the second note of each p...") so the delay was hidden; they now read "Late by 0.08 beats, 40 ms" (50%: "Straight: nothing is late"; the name holds the percentage and the delay follows the song's tempo), and the test checks them. The window height went from 640 to 540 (a third of 640 was empty).
- *Not seen in a picture:* the swing step's done note "Hi-hats swing their eighths at about 58%": the stage that ticks it finishes the exercise, so the window shows the finished page. The note is checked by the tests (`build a beat: pressing Quantise at the starting settings ticks step 4 ...`), and the same words appear in the guide's R5 line.
- *First steps:* checked, no change. The Start panel (its Drums tile adds a drum track and is unchanged), the inspector's *Get started* and the First Song Exercise (step 3 still writes its basic beat; nothing it names was renamed) need no update.
- *Docs:* README (the exercise under the guided exercises, Quantise's Swing and the Swing topic), `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md` (*A guided exercise*, *Checking in the real app*) and `docs/ui-design.md` (the window height list). `SOURCES.md` already holds this feature's sources; no library, model or font was added, so no licence file changed.
- *Final validation:* `./build.sh --test` ended with `All checks passed` (4326 PASS, 0 FAIL, 0 warnings).

**Website hand-off**

- `features.html`: the exercises group (beside the other guided exercises), and one line under editing MIDI for Quantise's Swing. `learn.html`: the rhythm guide's *Swing* topic. No library, so no `about.html` change.
- Shots worth taking: the exercise window at step 4 beside the piano roll with a swung hi-hat (pop song: use the `drumBeat` scene with `"beatStage": "accents"` and `"exercise": "buildBeat"`, `"exerciseStep": 3`, then `"capture": "main"`), and the Quantise window at 1/8, 58% (`"quantiseDialog": {"grid": "1/8", "swing": 58}` in the shot after a `drumBeat` shot, `"capture": "window"`). The rhythm guide at *Swing* with `"play": true` shows the five rows.
