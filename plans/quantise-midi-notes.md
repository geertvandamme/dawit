# Quantise MIDI notes to a grid

Status: Released
Idea: [#155 — Quantise MIDI notes to a grid](https://github.com/geertvandamme/dawit/issues/155)

## Goal
*Quantise...* in the piano roll and a MIDI clip's menu moves the selected notes (or the whole clip) towards the nearest lines of a grid, all the way or part of it, so a part played in by hand sits tighter and keeps as much of its feel as you want. A picture shows where each note goes before you apply it, and two new topics in the rhythm guide explain grid, strength, snapping versus quantising, and why perfectly in time isn't always better.

## Decisions for you
1. **How it's offered.** Options: (a) a small *Quantise notes* dialog (grid, strength, *Quantise ends too*, a before/after picture, a summary line, Cancel and Quantise), plus Q to repeat the last settings; (b) a menu only (grid and strength as submenus), no picture. **Recommended:** (a): the issue asks for the scope to be clear before applying and the result to be shown; the picture does both, and Q keeps it quick.
   Answer: (a) a dialog with a picture, plus Q to repeat the last settings. (user, 2026-11-08)
2. **Which guide.** Options: two topics in *Note lengths, beats and bars* (rhythm); topics in *What is MIDI?*. **Recommended:** rhythm: the piano roll's Snap ? already opens it, its rows are playable one-bar rhythms that suit before/after examples, and the MIDI guide's *Time in MIDI* already points there for quantising.
   Answer: the rhythm guide. (user, 2026-11-08)
3. **Chords.** Options: notes starting within 0.02 beats of each other (`miditransform::chordWindowBeats`) go to the same line, chosen by the earliest; or every note on its own (a spread chord straddling the halfway point splits across two lines). **Recommended:** together.
   Answer: chords move together. (user, 2026-11-08)
4. **Two notes of the same pitch landing together** (a flam at 100%). Options: keep both (count and order never change, so selection and undo stay simple) and say so in the summary line; delete the quieter one. **Recommended:** keep and say so.
   Answer: keep both and say so in the summary line. (user, 2026-11-08)
5. **Where the grid lines are.** Options: every grid step from each bar line (restarting at every bar); plain multiples from the song's start, as Hits to MIDI does. They agree in 4/4 and wherever the step divides the bar; in 7/8 (3.5 beats) only bar-based lines land on bar 2's downbeat. **Recommended:** from each bar line; aligning Hits to MIDI goes to *Left for later*.
   Answer: from each bar line. (user, 2026-11-08)
6. **Rename the rhythm guide's last topic** *Your song's time signature* to *Your song's rhythm* (summary "Your song's bars, clicks and timing"), since it gains timing advice. **Recommended:** yes.
   Answer: yes, rename it to *Your song's rhythm*. (user, 2026-11-08)

## What the user does and sees
- **Piano roll toolbar:** a *Quantise...* button (about 84 px) just left of the Snap label. Tooltip: "Quantise: move the selected notes (or the whole clip when none are selected) towards the nearest lines of a grid, to tighten a part played in by hand. Q quantises again with your last settings." Disabled without a clip.
- **Q** in the piano roll quantises the selection (or clip) at once with the last settings, no dialog. Plain Q is free: not used by the piano roll or by `ComputerKeyboardTyping`'s letter map.
- **MIDI clip right-click menu (`TrackLanes`):** *Quantise notes...* after the *Transform* submenu, for the whole clip; disabled where *Transform* is (not editable).
- **The dialog** (about 560 × 380, modal, opened like `BounceDialog`, title "Quantise notes"):
  ```
  All 24 notes of 'Lead' (none are selected)        [guide icon]
  Grid      [1/16 (sixteenth note) v]
  Strength  [=========o--]  75%
  [ ] Quantise ends too (lengths change)
  ┌ picture: up to 4 bars from the first note in scope; grid lines; notes as outlines (now) and filled (after), joined ┐
  5 of 24 notes move, by up to 0.10 beats (50 ms at 120 BPM).
                                                 [Cancel] [Quantise]
  ```
  Scope line with a selection: "The 6 selected notes of 'Lead'". Grid choices: 1/4 (quarter note), 1/8 (eighth note), 1/16 (sixteenth note), 1/32 (thirty-second note), 1/4 triplet, 1/8 triplet, 1/16 triplet (2/3, 1/3 and 1/6 beat). Strength slider 0 to 100% in 1% steps. The picture, summary and button follow every change. *Quantise* is disabled when nothing would move (summary says why). The guide icon opens the rhythm guide at *Quantising*. Return applies, Escape cancels.
- **After applying:** the notes move, the selection stays as it was, the track's instrument is untouched, and the step is "Quantise notes" in the undo history (Cmd+Z restores the exact notes).
- **Settings:** `quantise.grid` (the choice, default 1/16), `quantise.strength` (0 to 100, default 100), `quantise.ends` (default off). Nothing in the project.
- Texts of the tooltips and summary lines are in *Theory*.

## Data and saving
No new project data and no format change: quantising edits `MidiClip::notes`, already saved. One save-and-load test of a quantised clip (notes equal to within 1e-9).

## The guide
Two topics in *Note lengths, beats and bars* (`gen::rhythm`), after *Reading DawIt*, before the last topic:

| Topic | Summary | Picture (rows of `RhythmExplainer`, playable) |
|---|---|---|
| Quantising | Moving played notes onto the grid, all or part of the way | *As played*, *50%*, *100%* on 1/16 (example A); with a clip open in the piano roll, two more rows first: its first bar with a note off the last-used grid, as it is and with the last Quantise settings |
| Feel and the grid | Why perfectly in time isn't always better | *Swung eighths*, *Swung, quantised to 1/16*, *Swung, quantised to 1/8 triplet*, *Snare 20 ms late*, *Snare quantised* (example B) |

- **Your song (renamed *Your song's rhythm*, Decision 6)** gains advice R4 (*Theory*). `SongRhythm` already holds every MIDI note start.
- `RhythmExplainer` gets the clip open in the piano roll as `MidiExplainer` does (a `getClipView`-style callback from `MainComponent`), and repaints when it changes.
- **Theory Guides entry:** summary becomes "Bars and beats, time signatures from 3/4 to 7/8, whole to sixteenth notes, rests, dotted notes and triplets, counting, tempo, quantising and feel, and your song's metre."; `opensFrom` adds "the Quantise notes window"; words add "quantise quantize quantising quantizing quantisation quantization strength groove feel tight loose timing humanise humanize laid-back". Stays on *Start here*. Topics follow `allTopics`.
- Opens at a topic through `MainComponent::openGuide (Guide::rhythm, topic)`; the PDF follows.
- **MIDI guide fix:** *Time in MIDI* and *Your clip* write "Quantize"/"quantized" in American spelling; make them British, and *Your clip*'s timing line, when notes are off the sixteenths, adds "Quantise... (Q in the piano roll) can move them onto the grid."

## Code
- Follows recipes.md *Topics in an existing guide*, *A window* only for the dialog's look (it's a modal dialog like `BounceDialog`, not a `PluginWindowManager` window), and *A command or menu item* in the views (it acts on a clip, like the Transform menu: nothing in `CommandIDs`).
- **`quantise`** (`src/engine/MidiQuantise.{h,cpp}`, both CMake targets): pure functions. Settings: grid in beats, strength 0 to 1, ends on or off. One function returns new notes in the same order and count from the notes, selection (empty = all), the clip's start beat and length, and the bar length; one returns the summary (notes moved, largest move in beats, same-pitch pairs now starting together); the grid choices with their names and settings keys.
- **Rules:** only notes starting before the clip's end count (reuse `miditransform::getAffected`). Lines are each bar's start plus whole grid steps inside the bar, in song beats (clip start + note start), one signature per song. Target = nearest line; exactly halfway (within 1e-9) goes to the later line, as `snapNearest` does; a start within 1e-6 of a line is left exactly as it is. A chord (Decision 3: starts under 0.02 beats after the group's first) takes its first note's target. A target at or past the clip's end becomes the last line before it, one before the clip's start the first line inside it; with neither, the note stays. New start = start + strength × (target − start). Starts only: length, pitch, velocity kept, except a note that ended inside the clip and would now cross its end is shortened to end at it. Ends too: the end's target is its nearest line but at least one grid step after the start's target and at most the clip's end (if the note ended inside it); new end = end + strength × (target − end), so the length stays above 0. At 100% quantising twice changes nothing more.
- **Engine:** one function applies it under `getLock()`, names the step "Quantise notes" (`nameNextStep`), calls `notesChanged()`, and changes and announces nothing when no note moves. No one-step undo of its own: the history does it (undo plan, Decision 5).
- **UI:** one shared helper opens the dialog or applies the last settings for a clip and selection, used by `PianoRoll` and `TrackLanes`.
- **Threading and risk:** message thread only, a note edit under the lock; no audio-thread change. Selected indices stay valid because order and count never change.

## Libraries and models
None.

## Theory

**Facts**

| # | Fact | Kind | Source |
|---|---|---|---|
| 1 | Quantising moves a note towards the nearest grid line; a strength percentage says how far: 100% all the way, 0% not at all. | exact (definition) | Logic Pro help, *Quantize* (Q-Strength); Ableton Live manual, *Quantizing Notes* (Amount) |
| 2 | A start at 1.10 beats on a 1/4 grid goes to 1.00 at 100% and 1.05 at 50%. | exact | derived: 1.10 + 0.5 × (1.00 − 1.10) = 1.05 |
| 3 | At 120 BPM a beat lasts 500 ms, 0.10 beats is 50 ms, 0.04 beats 20 ms, 0.02 beats 10 ms, a sixteenth 125 ms. | exact | derived: 60 000 / 120 = 500 |
| 4 | Triplet grids: 1/4 triplet 2/3 beat, 1/8 triplet 1/3, 1/16 triplet 1/6. | exact | derived; rhythm guide (*Dotted notes and triplets*) |
| 5 | Swung eighths are played long-short, near 2:1 like a triplet, wider at slow tempos and nearer even at fast ones. | typical | already in the rhythm guide; Wikipedia, *Swing (jazz performance style)* |
| 6 | A swung note at 2/3 beat on a 1/16 grid goes to 0.75 (0.083 away, against 0.167 to 0.5). | exact | derived |
| 7 | Triplets at 0, 1/3, 2/3 of a beat become 0, 0.25, 0.75 on a 1/16 grid. | exact | derived |
| 8 | Skilled players' timing differs from the grid by small amounts, often within ±50 ms; measured spreads were 0.026 beats in a funk excerpt at 100 BPM (about 16 ms) and 0.068 beats in a swing excerpt at 150 BPM (about 27 ms). | typical | Senn, Kilchenmann, von Georgi and Bullerjahn (2016), *Frontiers in Psychology*; ms derived |
| 9 | Listeners rated recordings with the players' own timing and fully quantised versions about equally groovy, exaggerated timing lower, and liked best a point between the two. | typical (one study) | Senn et al. (2016) |
| 10 | Davies et al. (2013) and Frühauf, Kopiez and Platz (2013) found quantised patterns rated grooviest. | typical | Senn et al. (2016), summarising them; Frühauf et al., *Musicae Scientiae* 17(2) abstract |
| 11 | Many producers quantise at 50 to 90% to tidy a part and keep some life; a matter of taste. | opinion | as the drum triggers guide (*Snap with a strength, or keep the feel*) says |
| 12 | Roger Linn's LM-1 (1980) had "timing correct" (quantising to the nearest sixteenth) and a shuffle made by delaying every second sixteenth. | exact | Wikipedia, *Linn LM-1* |
| 13 | Quantising only helps when the song's tempo matches the playing. | exact (derived) | as the drum triggers guide says |
| 14 | Two hits a few hundredths of a beat apart land on the same line at 100%. | exact | derived |

No contradiction with an existing guide: Fact 11 and the strength example agree with the drum triggers guide (30 ms late becomes 15 ms at 50%). The MIDI guide's American "Quantize" is a spelling fix, not a fact.

**Topic texts**

*Quantising* — "Moving played notes onto the grid, all or part of the way"

Quantising

Play a part on a keyboard and your notes land a little early or a little late: a few hundredths of a beat either side of where you meant them. Quantising moves them towards the nearest line of a grid you choose, so the part sits tighter on the beat.

Take a note you meant on beat 2 that starts at 1.10 beats (positions count from 0, so beat 2 is at 1.00). On a 1/4 grid the nearest line is 1.00, 0.10 beats earlier. At 120 BPM a beat lasts 0.5 s, so the note was 50 ms late.

Strength says how far each note goes:
  100%: all the way, onto the line (1.10 becomes 1.00).
  50%: halfway (1.10 becomes 1.05, now 25 ms late).
  0%: nowhere.
A note already on a line stays where it is. Below 100% every note keeps part of its own timing, smaller but in the same direction, so a loose part gets tighter and still sounds played. Many producers use 50 to 90% for that; how much is a matter of taste.

Choose the grid from the smallest note value the part really plays. Eighth notes need 1/8 or 1/16. Quantise sixteenths to 1/8 and every other one jumps to a neighbouring line, often on top of the note already there. Triplets and swing need a triplet grid (see *Feel and the grid*).

Snap and quantise both use a grid, at different moments. Snap works while you draw, move or stretch a note with the mouse: the note goes where you put it, a grid step at a time. Quantise works afterwards, on notes that are already there, usually a part you played in or imported, and moves them all at once, each to its own nearest line.

In DawIt: select notes in the piano roll, or none for the whole clip, and click Quantise... (or press Q to use your last settings again). Choose the grid and the strength; the picture shows where each note will go before you apply it. Only the starts move: every note keeps its length, pitch and velocity, unless you tick Quantise ends too. Notes that start within 0.02 beats of each other, as in a chord, move together, and every note stays inside the clip. A note exactly halfway between two lines goes to the later one. Cmd+Z undoes the whole quantise in one step.

Quantising moves MIDI notes, not the sound of an audio recording. And it only helps when the song's tempo matches the playing: if the notes drift further from the grid bar after bar, set the tempo first.

When a MIDI clip is open in the piano roll, the picture starts with a bar of it that has notes off the grid, as it is and as your last Quantise settings would leave it.

*Feel and the grid* — "Why perfectly in time isn't always better"

Feel and the grid

Good players are never exactly on the grid. A drummer may play the snare a touch late for a relaxed, laid-back feel, or push the hi-hat ahead for urgency, and a pianist spreads the notes of a chord a little. In recordings these differences are small: from a few milliseconds to a few tens of milliseconds, mostly under 50 ms. At 120 BPM, 20 ms is 0.04 beats, a third of a sixteenth note.

Part of a performance's feel lives in those differences, and quantising at 100% takes them out. That isn't always a loss. In one listening study (Senn and colleagues, 2016), people rated funk and swing recordings with the players' own timing and the same recordings fully quantised about equally groovy; versions with the differences made larger scored lower, and the best-liked lay between the two. Other studies found perfectly quantised drum patterns the grooviest of all, and a lot of dance music is programmed exactly on the grid on purpose. So there's no rule: listen to both, and lower the strength when 100% sounds stiff.

Where quantising goes wrong:
  Swing on a straight grid. Swung eighths are played long-short, close to the first two thirds and the last third of a beat. Quantise them to 1/16 and each short note jumps from 0.67 to 0.75 of the beat: a stiffer, dotted rhythm. Use the 1/8 triplet grid to keep the swing.
  Triplets on a straight grid. Eighth-note triplets start at 0, 0.33 and 0.67 of a beat. On a 1/16 grid they become 0, 0.25 and 0.75, a different rhythm.
  Flams and grace notes. Two hits a few hundredths of a beat apart land on the same line at 100%: the flam becomes one louder hit, and two notes of the same pitch play on top of each other. Select only the notes that need it, or lower the strength.
  Timing on purpose. A laid-back snare, a chord pushed onto the "a" of 4, a pick-up before the bar: quantise them and the part loses what made it move. Select only the notes that are really out.

Quantising and swing came into music production with drum machines. Roger Linn's LM-1 (1980) could put a part played in by hand onto the nearest sixteenth, which he called timing correct, and add a shuffle by delaying every second sixteenth. DawIt's Quantise has no swing setting yet: for a swung part, use a triplet grid.

The picture plays swung eighths as played, on a 1/16 grid and on a 1/8 triplet grid, and a backbeat with the snare 20 ms late, before and after quantising.

**Tooltips and dialog texts**
- Grid: "The grid the notes move to. Choose the smallest note value the part really plays: 1/16 for sixteenth notes, a triplet grid for triplets and swing."
- Strength: "How far each note moves towards its grid line: 100% onto the line, 50% halfway, 0% not at all. Below 100% the part keeps some of its feel."
- Quantise ends too: "Also move each note's end towards the nearest grid line, so lengths change. Off: every note keeps its length."
- Picture: "Where each note goes: outlines are where it is now, filled notes where Quantise puts it. The lines are the chosen grid."
- Quantise button (dialog): "Move the notes as the picture shows. Cmd+Z undoes it in one step."
- Clip menu item: none (JUCE menu items have no tooltips); the dialog explains.
- Summary lines: "5 of 24 notes move, by up to 0.10 beats (50 ms at 120 BPM)." / "Nothing to move: every note is already on the 1/16 grid." / "At 0% nothing moves." / added when it happens: " 2 notes of the same pitch will start together; you may want to delete one." The ms use the tempo at the clip's start.

**Example data** (4/4, 120 BPM, one bar, lengths 0.4 beats, velocities 0.8 on beats and 0.6 between)
- A, *As played* (eighth notes on 1/16): starts 0.04, 0.47, 1.06, 1.55, 1.98, 2.44, 3.03, 3.58; offsets +20, −15, +30, +25, −10, −30, +15, +40 ms.
- A, *50%*: 0.02, 0.485, 1.03, 1.525, 1.99, 2.47, 3.015, 3.54 (offsets halved).
- A, *100%*: 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5.
- B, *Swung eighths*: 0, 2/3, 1, 5/3, 2, 8/3, 3, 11/3, lengths 0.3. *On 1/16*: 0, 0.75, 1, 1.75, 2, 2.75, 3, 3.75. *On 1/8 triplet*: unchanged.
- B, *Snare 20 ms late*: kick 0 and 2, snare 1.04 and 3.04 (labels "kick", "snare"), hi-hat on every eighth; *Snare quantised*: snare 1 and 3.
- The rows are made with the `quantise` functions, so the guide shows what the tool does.

**"Your song" rule** (R4, in `rhythm::getAdvice`)
- For each MIDI note start, take its place in its bar and its distance to the nearest multiple of 0.125 or of 1/6 beat (so sixteenths, thirty-seconds and triplets count as on the grid). Loose: more than 0.02 beats.
- Says, when at least 16 notes and at least 25% of them are loose: "<n> of your <m> MIDI notes start between the grid lines, more than 0.02 beats (<ms> ms at <bpm> BPM) from the nearest sixteenth, thirty-second or triplet, as when a part is played in by hand. If it sounds right, that's feel: leave it. If it sounds loose, select the notes in the piano roll and try Quantise... at 50 to 90% to tighten it and keep some of the feel." (ms = 0.02 × 60 000 / BPM: 10 ms at 120.)
- Quiet with fewer than 16 notes or fewer than 25% loose.

**Test numbers**

| Check | Expected | Tolerance |
|---|---|---|
| 1.10 on 1/4 grid, 100% / 50% / 0% | 1.00 / 1.05 / 1.10 | 1e-9 |
| 2.00 on 1/16, 100% | 2.00 exactly | exact |
| 1.125 on 1/16 (halfway) | 1.25 | 1e-9 |
| 2/3 on 1/16 / on 1/8 triplet | 0.75 / unchanged | 1e-9 |
| Chord 1.12 and 1.13 on 1/16 | both 1.00 (1.13 alone: 1.25) | 1e-9 |
| Clip 8 beats, start 7.90 on 1/16 | 7.75 | 1e-9 |
| Clip at song beat 0.1, note at 0.0, 1/16 | 0.15 (song 0.25) | 1e-9 |
| Starts only: start 3.4, length 0.6, clip 4, grid 1/8 | start 3.5, length 0.5 | 1e-9 |
| Ends too: start 1.10, length 0.10, 1/16, 100% | start 1.00, length 0.25 | 1e-9 |
| Ends too: start 1.02, length 0.05, 100% / 50% | length 0.25 / start 1.01, length 0.15 | 1e-9 |
| 7/8, grid 1/4: song beat 3.4 / 4.4 | 3.5 / 4.5 (not 4.0) | 1e-9 |
| Example A at 50% and 100% | the rows in *Example data* | 1e-9 |
| Summary: Example A at 100% | 8 moved, largest 0.08 beats, 40 ms at 120 BPM | 1e-9 / 0.5 ms |
| R4: 16 notes, 8 off by 0.05 | fires, "8 of your 16" | text |
| R4: 15 notes all loose; 16 on 1/32 or triplets | quiet | text |
| ms text at 120 BPM | "10 ms" | text |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `quantise` in `src/engine/MidiQuantise.{h,cpp}` (both targets): grid choices, settings, the transform and the summary, with the rules in *Code*. Tests: `runQuantiseTests` with every row of the test table but R4. Done when: the tests pass.
- [x] 2. The engine function (lock, step name, `notesChanged`, no change no announce) next to `transformNotes`. Tests: returns false and records no history step on a no-op; one `undo` restores the exact notes; selection only moves selected notes; pitch, velocity, length kept; count and order kept; save and load of a quantised clip. Done when: tests pass.
- [x] 3. Rhythm guide content: the two topics (texts, names, summaries) and example rows' data built with `quantise`; rename the last topic (Decision 6) and update its uses (`RhythmExplainer`, tests, plan references stay); R4 in `getAdvice`; MIDI guide spelling and *Your clip* sentence; Theory Guides entry. Tests: R4 rows of the table; topic count +2; each new topic has text and rows; search "quantize" and "quantise" find the rhythm guide. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 6):
- [x] 4. `src/ui/QuantiseDialog.{h,cpp}` (DawIt target): the layout, picture, summary and settings in *What the user does and sees*, plus the shared helper that opens it or applies the last settings. Check: opens on the example song's Lead clip.
- [x] 5. Wire it: piano roll button and Q key (`PianoRoll`), *Quantise notes...* in the MIDI clip menu (`TrackLanes`), the dialog's guide icon to `openGuide (Guide::rhythm, Quantising)`. Check: the button, Q and the menu all quantise; Cmd+Z undoes.
- [x] 6. `RhythmExplainer`: pictures of the two topics (rows; the *Your clip* rows from the open clip and last settings) and the clip callback from `MainComponent`. `ScreenshotRunner`: a `playedClip` scene (a MIDI track "Played keys" with Example A over two bars, selected and open in the piano roll) and shot keys `"quantiseDialog": {"grid": "1/16", "strength": 50}` (opens the dialog for the selected clip) and `"quantise"` (same keys, applies). Done when: `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 7. Real-app check, one run: (a) `playedClip` scene; (b) `"quantiseDialog"` at 50%, capture the dialog: scope line, outlines and filled notes, summary "16 of 16 notes move, by up to 0.04 beats (25 ms at 96 BPM)" (the example song is at 96 BPM); (c) piano roll with Snap visible: toolbar fits, title shrinks; (d) `"quantise"` at 100%, `capture: "bottomPanel"`: notes on the lines; (e) `"undo": 1`: back as played; (f) rhythm guide at *Quantising* with the clip open (your-clip rows) and at *Feel and the grid*; (g) *Your song's rhythm* after (a) shows R4. Look at every PNG for clipped text and wrong numbers.
- [x] 8. Docs: README rhythm bullet (or the editing bullet) mentions Quantise with grid and strength; `docs/tests.md` (quantise area, R4), `docs/code-layout.md` (`MidiQuantise`, `QuantiseDialog`), `docs/recipes.md` *Checking in the real app* (the `playedClip` scene and the two shot keys), `docs/architecture.md` *Guides* line naming the rhythm guide's clip callback; #155 progress comment (and #9: triplet grids exist in Quantise, not yet in Snap). `SOURCES.md` already has the sources.

## Tests
As in *Test numbers*, in `runQuantiseTests` (engine function, undo, save and load included) and `runRhythmTests` (R4, topics, search).

## Website hand-off
- `features.html`: the MIDI editing group (next to Transform): Quantise with grid, strength and the before/after picture.
- `learn.html`: the rhythm guide gains *Quantising* and *Feel and the grid*.
- Screenshots: the Quantise dialog at 50% on the `playedClip` scene; the rhythm guide at *Feel and the grid*. Not for Home or `about.html` (no library).

## Left for later
- Swing setting in Quantise (and triplet Snap): #9 tracks swing and triplet Snap.
- Audible preview before applying: shared with #42 (Humanize); hearing the result and Cmd+Z covers it now.
- Logic-style Q-Range (only move notes nearer or further than a distance), velocity or length quantise, groove templates: not asked for.
- Hits to MIDI uses plain multiples from the song start, not bar-based lines; align it with `quantise` in a follow-up (only odd metres differ).
- Quantise as you record (input quantise): needs MIDI recording first.

**Group A hand-off (engine and guide content, built; `./build.sh --test` passes, no warnings)**
- `daw::quantise` (`src/engine/MidiQuantise.{h,cpp}`, both CMake targets): `getGrids()` (key, menu name, beats; keys `1/4 1/8 1/16 1/32 1/4t 1/8t 1/16t`), `findGrid (key)` (1/16's index when unknown), `getGridKey (beats)`, `Settings { gridBeats, strength 0..1, ends }`, `apply (notes, selection, clipStartBeat, clipLength, barLength, settings)`, `summarise (before, after, selection, clipLength)` giving `Summary { inScope, moved, largestMove, collidingNotes }`, `describe (summary, settings, bpm)` (the summary line) and `sameNotes`.
- `Engine::quantiseNotes (MidiClip&, selection, settings)` in `Engine.{h,cpp}`: uses the engine's bar length, names the step "Quantise notes", returns false and records nothing when no note changes. It has no one-step undo of its own and doesn't touch `transformUndo`.
- Guide: `rhythm::Topic::quantising` and `feelAndGrid` sit before `yourSong` (now 12 topics; `yourSong` is index 11 and named *Your song's rhythm*). `rhythm::getQuantiseExample (Topic)` returns the rows (`name`, `detail`, `notes`, `labels` per note, empty label = show the count) for the two topics, 4/4 at 120 BPM, made with `quantise::apply`. The *Your clip* rows (clip open in the piano roll, last settings) are NOT built: they need the callback and are Group B's. `RhythmExplainer::getRows` has a placeholder `case` for the two new topics that returns no rows: Group B fills it from `getQuantiseExample`.
- R4 is in `rhythm::getAdvice`; Theory Guides entry updated; MIDI guide spelling and *Your clip* sentence done.
- Tests: `runQuantiseTests` in `tests/EngineTests.cpp` (all table rows, engine function, undo, save and load, topics, search, R4); R4 and the topic checks live there rather than in `runRhythmTests`.

## Changes while building
- The chord window is a second constant, `quantise::chordWindowBeats` (0.02), equal to `miditransform::chordWindowBeats`, so `quantise` doesn't depend on the transform header for it.
- Same-pitch collisions are counted as notes (a pair is 2), matching the summary text "2 notes of the same pitch will start together".

**Group B hand-off (UI, built)**
- `src/ui/QuantiseDialog.{h,cpp}` (`daw::quantiseui`): `getLastSettings (Engine&)` / `saveLastSettings` (keys `quantise.grid`, `quantise.strength` as 0 to 100, `quantise.ends`), `applyLast (Engine&, MidiClip&, selection)` (the Q key), `showDialog (Engine&, MidiClip&, selection, onShowGuide)` (modal `DialogWindow`, 560 x 380, title "Quantise notes"), `getButtonTooltip()`.
- Wiring: `PianoRoll::quantiseButton` (84 px, left of the Snap label, enabled with a clip), `PianoRollGrid::onQuantiseKey` (plain Q), `PianoRoll::onShowQuantiseGuide`; the clip menu item 43 *Quantise notes...* in `TrackLanes` (whole clip, enabled when editable and the clip has notes), `TrackLanes::onShowQuantiseGuide` forwarded by `ArrangementView::onShowQuantiseGuide`; both set in `MainComponent` to open the rhythm guide at *Quantising*.
- `RhythmExplainer (Engine&, std::function<ClipView()>)`: `ClipView { track, clip }` like `MidiExplainer`; `getRows` builds the two topics from `getQuantiseExample`; *Quantising* gets two "Your clip" rows in front (first bar with a note off the last grid, as it is and with the last settings; none when no note is off the grid or no clip is open), redrawn by a 4 Hz `clipWatch` poll when the clip or settings change.
- `ScreenshotRunner`: scene `playedClip` (track and clip "Played keys", example A over two bars, selected and open in the piano roll), shot keys `"quantiseDialog": {"grid","strength","ends"}` (opens the dialog, whole clip) and `"quantise"` (same keys, applies). Both keys run before a shot's `scene`, so use the scene in an earlier shot.
- Not yet looked at in the real app (Group C): toolbar fit with the new button, dialog layout, the picture, clip rows.

**Group C hand-off (check and docs, built)**
- Real-app check done. Found and fixed: the dialog's content had no background of its own, so a window capture showed pale text on white (the window colour hid it on screen): `Content::paint` in `QuantiseDialog.cpp` now fills `theme::panel`. The *Snare 20 ms late* rows of *Feel and the grid* drew the hi-hat's count ("1", "2") over the drum names; the hi-hat hits on a kick or snare now carry a blank label (`" "`) in `getQuantiseExample`.
- On the default pop song the R4 advice does not appear after (a): the song's other MIDI clips make the 16 loose notes under a quarter of all notes, as the rule intends. Use `"song": "empty"` with the `playedClip` scene to see it (it then reads "14 of your 16 MIDI notes ... (10 ms at 120 BPM)": two of the 16 happen to sit within 0.02 beats of a line), and gone after quantising at 100%.
- The dialog summary at 50% reads "16 of 16 notes move, by up to 0.04 beats (25 ms at 96 BPM)" on the pop song, as planned.
- Website hand-off: show the piano roll toolbar with *Quantise...* (`playedClip` scene, `capture: bottomPanel`), the dialog at 1/16 and 50% (`quantiseDialog`, `capture: window`), the piano roll after `quantise` at 100%, and the rhythm guide at *Quantising* with the clip open and at *Feel and the grid*. Add a feature entry (grid with triplets, strength, chords together, one undo, Q) and mention the two new rhythm topics for `learn.html`. No library, model or tool was added.
