# Turn the arpeggio into MIDI notes

Status: Released
Idea: [#146 — Turn the arpeggio into MIDI notes](https://github.com/geertvandamme/dawit/issues/146)

## Goal
Right-click a MIDI clip of block chords on a track with DawIt Arpeggiator and choose **Create MIDI clip from arpeggiator**: DawIt plays the clip through the track's MIDI effects, silently and offline, and writes every note that comes out into a new clip at the same place, on a track below that sounds the same but has no arpeggiator. A beginner can then change single notes, print the part as a score or move it to another instrument, and the guide explains when to keep a pattern live and when to write it out. This was decision 3 of [arpeggiator.md](arpeggiator.md), left for its own issue.

## Decisions for you
1. **Entry points now.** Options: the clip menu only (the issue's request); also the track menu (**Create MIDI track from arpeggiator**: all the track's unmuted clips in one pass); also a button in the arpeggiator window or its chip menu; selected bars; the chord track as input; replacing the source in place. **Recommended:** clip menu and track menu now. The track version is the same engine call with no clip, and one pass over the whole track matches playback where Latch or a held chord runs from one clip into the next. Leave for later: the window button (the 1240-wide window is full, and the menus already reach it), selected bars (split the clip first), chord-track input (the arpeggiator can't follow the chord track yet; arpeggiator plan, *Left for later*), and replace in place (it loses the live pattern, and mute plus undo already give the same result).
   Answer: clip menu and track menu now (recommended); the rest left for later.
2. **The chords afterwards.** Options: mute the source clip (the track version mutes the source track), as Bounce in place does; leave it playing. **Recommended:** mute it. The song then sounds the same, not doubled. The clip's notes and the arpeggiator don't change, and undo unmutes it.
   Answer: mute it (recommended).
3. **The new track's sound.** Options: copy the instrument (built-in sound or plugin, with its state), the audio effects, colour, bus, volume, pan, sends and score clef, but no MIDI effects; or the built-in piano only. **Recommended:** copy, so the written-out part sounds like the arpeggio did. A plugin that fails to load is left out, and a note names it.
   Answer: copy the instrument, audio effects, mixer settings, sends, bus and clef, no MIDI effects (recommended).
4. **Undo.** DawIt has no app-wide undo; single-step undos sit in menus (*Undo random progression*, *Undo Reverse in time*). Options: an **Undo create MIDI clip from arpeggiator** item in the source clip's and the new clip's menus, available while the new clip is unchanged; or leave it to Delete. **Recommended:** the menu item, as the issue asks for one-step undo.
   Answer: the menu undo item (recommended).
5. **Name.** Options: the issue's "Create MIDI clip from arpeggiator…"; or the same without "…"; or "Write out arpeggio as notes". **Recommended:** "Create MIDI clip from arpeggiator" without "…", because CLAUDE.md keeps "…" for items that open a window, and this one doesn't (only a progress bar for a long track). When the track's active MIDI effects aren't only DawIt Arpeggiator, the item says "MIDI effects" instead of "arpeggiator".
   Answer: "Create MIDI clip from arpeggiator" without "…", saying "MIDI effects" when needed (recommended).
6. **Where the notes stop.** With Latch on, playback goes on past the clip's end until the next chord. Options: cut at the clip's end; extend to the next clip or the song's end. **Recommended:** cut at the clip's end (the track version: the last clip's end). The clip then matches its source, and the guide says so.
   Answer: cut at the clip's end (recommended).

## What the user does and sees
- **Clip menu** (`TrackLanes::showClipMenu`, MIDI clips): on a track with a MIDI effect, after *Bounce clip in place*: **Create MIDI clip from arpeggiator**. It is enabled when a MIDI effect isn't bypassed, the clip isn't muted and has notes, and DawIt isn't recording. The menu's guide items end with **About writing out the arpeggio...**, which opens "What is an arpeggiator?" at *Writing the arpeggio out as notes* on that track's arpeggiator. When an undo is available for this clip (source or result), **Undo create MIDI clip from arpeggiator** comes next.
- **Track menu** (`TrackActions`, a single MIDI track with a MIDI effect): **Create MIDI track from arpeggiator**, below *Bounce in place*. It is enabled as above, for unmuted clips.
- **The result:** a MIDI track right below, named "<track> (arpeggio)", with one clip (named as the source clip or the track) at the source's start and length. The new clip is selected and open in the piano roll. A "(arpeggio)" track already right below, with no MIDI effects, the same bus and a free place, is reused (as Bounce in place reuses "(bounced)"). Then its sound isn't copied again.
- **Long track:** the render runs behind a progress window with Cancel (copy `bounceui`'s `InPlaceJob`); a clip takes well under a second.
- **Messages** (an info box only when needed): "Nothing came out of the MIDI effects for this clip: hold chords long enough for a step, or check that a step isn't all Rests." and "<plugin> couldn't be loaded on the new track, so it plays the built-in piano." The problems disable the item instead.
- **Tooltips:** menu items have none (JUCE popup menus can't show them). The explanation is in the guide page and one sentence in the arpeggiator window's overview explainer: "To edit single notes or print the part, right-click the clip and choose Create MIDI clip from arpeggiator."
- Nothing goes in app settings.

## Data and saving
No new data and no format change: the result is an ordinary MIDI track and clip with copied plugins, saved as usual. The undo record lives only in the engine and is forgotten on load or New (edit generation). Test: write out, save, load; the new track has the notes, the instrument and effects, and no MIDI effect.

## The guide
A new page in "What is an arpeggiator?" (`arptext::makeGuide`), after *How DawIt Arpeggiator works* and before *Your track*, so it becomes 12 pages:

| Page | Summary | Picture |
|---|---|---|
| Writing the arpeggio out as notes | From a pattern that plays live to notes you can edit | two piano-roll strips (`drawRoll`): top, the chords clip (C4 E4 G4 for 2 beats, A3 C4 E4 for 2); bottom, the 16 written-out notes (Up, 1/16, 80 %); between them "DawIt Arpeggiator" and an arrow. Captions as in *Example data*. Hear this plays the example (`playExample`) |

Also change: page 9's last paragraph and page 10's point 5 (texts in *Theory*), and the window's overview explainer (above). Theory Guides entry: add the search words "write-out written-out convert" and ", and writing it out as notes" before ", and your track" in the summary. *Your track* doesn't change.

## Code
Recipes: *A command or menu item* (menus only, no `CommandIDs`: not used often enough for a shortcut), *Topics in an existing guide* (a page of an `fxui::Guide`), and *A long job* (copy Bounce in place: `bounce::planInPlace`, `Renderer`, `addBouncedTrack`, `bounceui::InPlaceJob`).

- **Notes capture:** a new offline-only focus option, *notes only*: one chain (the track, or one clip), with the MIDI effects run but not the instrument or audio effects, and no latency extra (`Renderer` adds a MIDI track's effects latency today; a capture must not, or every beat shifts). A capture handed in through the `RenderContext` gets every MIDI event after `Track::runMidiEffects` with its beat: the segment's start beat plus position × bpm / (60 × rate). Because `renderSegment` splits at tempo changes, this is exact through tempo changes. Only an offline render sets it; the device callback sees a null pointer, which is its only change.
- **Pairing:** a pure, tested function turns the captured events into `MidiNote`s, relative to the clip's start. A note-on is paired with the next note-off of its pitch (or a velocity-0 note-on); velocity is the 7-bit value / 127. Notes still sounding at the end are cut there; notes starting at or after it are dropped; zero-length notes are dropped. The render runs 1 beat past the end so the arpeggiator hears the clip's last note-offs.
- **Write-out:** plans (problems: not a MIDI track, no active MIDI effect, clip muted or empty, recording), renders, then on the message thread adds the track, copies the sound (decision 3), adds the clip, mutes the source and records the undo. It mirrors `addBouncedTrack`'s placement and reuse; the plugin copy follows what `ProjectFile` saves per slot (description, state, bypass). Put it next to Bounce in place in `Bounce.{h,cpp}`, or in a new engine file if Bounce.cpp grows too large.
- **Undo** (engine, like `transformUndo`): source clip, its or its track's old mute, the new clip and track, whether the track was made, and a copy of the written notes. It can undo while the new clip is on its track with those notes, start and length, and the source still exists. Undo removes the clip (and the track if this made it and it is now empty) and restores the mute.
- **Invariants:** the source clip's notes and every plugin on the source track are unchanged; the new track never has a MIDI effect; random order gives the same notes as playback (it hashes the song step).
- **Threading:** the `Renderer` is made and destroyed on the message thread (the device goes silent); the render and the capture run on the job thread; edits happen afterwards on the message thread through engine functions under the lock. Risk: the one change in `Track::process` (audio path); keep it a null-checked pointer.

## Libraries and models
None.

## Theory
### Facts
1. A MIDI note-on carries a velocity from 1 to 127; a note-on with velocity 0 counts as a note-off. Exact. MIDI 1.0 Detailed Specification (the guide's existing "MIDI's scale of 1 to 127" source).
2. At 120 BPM a 1/16 step lasts 0.25 beat = 0.125 s; at 80 % gate a note lasts 0.2 beat = 100 ms. Exact, derived (60/120 × 0.25 = 0.125; × 0.8 = 0.1).
3. 4 beats of 1/16 steps are 16 steps, so a 4-beat clip at 1/16 with all Note steps gives 16 notes. Exact, derived (4 / 0.25).
4. Up over C4 E4 G4 plays C E G C E G C E in 8 steps; over A3 C4 E4 it plays A C E A C E A C, and the order starts again on the new chord because the block chords are let go before the next one. Exact, from the shipped `arp::` behaviour (guide page 3: "The order starts again from its first note when you play a new chord after letting go of the last one").
5. DawIt's Random repeats for the same bar, so the written-out notes equal what plays. Exact, by design (arpeggiator plan decision 4).
6. Writing out when the pattern is settled and keeping it live while trying patterns. Opinion (workflow).
No web lookups were needed: every fact is a definition, a calculation or a fact the arpeggiator guide already states.

### Topic texts
**Writing the arpeggio out as notes** — From a pattern that plays live to notes you can edit

The arpeggiator plays its pattern live. Each time the song plays, it turns the chords of the clip into notes. That's what makes it quick to change: choose another order or rate, and every chord follows. But the notes it plays aren't in the clip. You can't move one of them, change one note's length, print them as a score or play them with another instrument.

Create MIDI clip from arpeggiator, in the right-click menu of a MIDI clip, writes them out. DawIt plays the clip through the track's arpeggiator, silently and faster than real time, and writes every note that comes out into a new clip at the same place in the song. The clip goes on a new track below, named after the old one, for example "Keys (arpeggio)". Create MIDI track from arpeggiator, in the track's menu, does the same for all the track's clips at once.

What it keeps: each note's pitch, start, length and velocity, as you hear them. So the order, octaves, rate, gate, swing, ties, accents, chord and bass steps and rests are all in the notes. Random gives the same notes as when you play, because DawIt's chance repeats each time the song plays the same bar. The steps are counted in beats, so tempo changes are followed too.

An example: a clip with C major (C4 E4 G4) for 2 beats, then A minor (A3 C4 E4) for 2 beats, with Up at 1/16 and 80% gate, becomes 16 notes: C E G C E G C E, then A C E A C E A C. Each note lasts 0.2 beats, 100 ms at 120 BPM. The order starts again on the A minor, because the clip lets go of one chord before the next one starts.

What changes: the new track has the same instrument, effects, volume and pan, but no arpeggiator, so its notes aren't turned into a pattern a second time. The chords clip is muted, not deleted, so the song sounds the same. To go back, choose Undo create MIDI clip from arpeggiator in either clip's menu while the new notes are still as they were. Or unmute the chords and delete the new track.

The written-out notes stop where the clip ends. With Latch on, the arpeggiator goes on after the clip when the song plays, until the next chord, but the new clip ends with the chords clip.

When to do it is up to you. Many people keep the pattern live while they're still trying orders and rates, and write it out when it's settled and they want to change single notes: a different last note at the end of a phrase, a rest taken out. Once it's written out, a new order or rate means writing it out again from the chords. For audio instead of notes, use Bounce clip in place.

### Changed texts
- **Page 9, last paragraph** becomes: "DawIt Arpeggiator or Play chords as...? Both turn chords into patterns. Play chords as... on the chord track writes the notes into a clip you can edit one by one. The arpeggiator plays them live: change the order or the rate while the song plays, and every chord you hold follows. When a live pattern is settled, Create MIDI clip from arpeggiator writes it out as notes too."
- **Page 10, point 5** becomes: "5. Recording keeps what you play, not what it plays. A take holds your chords, and the arpeggiator turns them into the pattern each time the song plays, so you can still change the pattern, the rate or the order afterwards. To get the notes it plays, right-click the clip and choose Create MIDI clip from arpeggiator; for audio, bounce the clip or the track."
- **Window overview explainer**, added at its end: "To edit single notes or print the part, right-click the clip and choose Create MIDI clip from arpeggiator."

### Example data for the picture
Source clip, 4 beats: C4 E4 G4 (60 64 67) from beat 0 to 2; A3 C4 E4 (57 60 64) from 2 to 4; velocity 0.8. Settings: Up, Octaves 1, 1/16, gate 80 %, swing 50 %, 16 × Note, no accents. Result: 16 notes, note k (0 to 15) starts at beat 0.25 k and lasts 0.2. Pitches 60 64 67 60 64 67 60 64 | 57 60 64 57 60 64 57 60. Velocity 102/127 = 0.803 (0.8 × 127 = 101.6, rounded). Captions: "The chords clip: C, then Am, 4 beats" and "The new clip: 16 notes, Up, 1/16, 80% gate".

### "Your song" rules
None: *Your track* doesn't change.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Example clip at bar 1, 120 BPM: note count | 16 | exact |
| its starts / lengths | 0.25 k / 0.2 beat | 1 sample at 48 kHz (0.00005 beat at 120 BPM) |
| its pitches | as in *Example data* | exact |
| its velocity | 102/127 | 1e-4 |
| accent 20 on steps 1, 5, 9, 13 | 122/127 on those, 102/127 elsewhere | 1e-4 |
| same clip at bar 5 (beat 16) | same notes relative to the clip | 1 sample |
| swing 54 %, 1/16 | odd steps start 0.27 beat after the even one | 1 sample |
| tie on step 2, gate 80 % | step 1's note lasts 0.45 beat (0.25 + 0.25 × 0.8), 15 notes | 1 sample (check against `arp::getNoteEndBeats`) |
| tempo 120 then 60 BPM from beat 2 | starts still 0.25 k beats | 1 sample at the local tempo |
| Random, two write-outs and playback | identical pitch lists | exact |
| Latch on, clip 4 beats | no note starts at or after beat 4; the last ends at ≤ 4 | exact |
| Write-out vs playback through the fake device into a recording instrument | same notes | 1 sample |
| Search "write-out" | finds "What is an arpeggiator?" | exact |

## Steps
Group A, engine (`./build.sh --test` at step 3):
- [x] 1. Notes-only capture in the offline render (focus option, capture through `RenderContext`, no latency extra) and the pure pairing function. Files: `Engine.h`, `Track.{h,cpp}`, `Bounce.{h,cpp}`. Tests: pairing (overlaps of one pitch, velocity 0, cut at end, zero length); the example clip's 16 notes. Done when: the device path is unchanged apart from a null check.
- [x] 2. Write-out (clip and track) with plan problems, placement and "(arpeggio)" reuse, sound copy without MIDI effects, source mute, undo record and undo. Files: `Bounce.{h,cpp}` (or a new engine file plus `CMakeLists.txt` in both targets), `Engine.{h,cpp}`. Tests: problems; the new track's plugins, sound, bus, sends and clef; source notes and plugins unchanged; mute; undo removes and unmutes; undo refused after a note edit; second write-out reuses the track.
- [x] 3. The rest of *Test numbers*: accents, bar 5, swing, tie, tempo change, Random, Latch, write-out vs playback (copy `runMidiEffectHostTests`' recording instrument), save and load. Files: `tests/ArpeggiatorTests.h`. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 5):
- [x] 4. Clip menu items (write-out, guide item, undo) in `TrackLanes`, track menu item in `TrackActions`, a job with a progress window in `BounceActions` (copy `InPlaceJob`), select the new clip, messages from *What the user does and sees*. Wire through `ArrangementView` as `onBounceClip` is. Check: a shot (step 6) shows the new track and clip.
- [x] 5. The guide page with picture and Hear this, the changed page 9 and 10 texts and window explainer (`ArpeggiatorGuide.cpp`, `ArpeggiatorEditor.cpp`); the `gen::guides` summary and search words (`TheoryGuides.cpp`) with a search test in `EngineTests.cpp`; a `ScreenshotRunner` shot key that runs the write-out on the selected track's first MIDI clip (`DevScreenshots.{h,cpp}`, documented in recipes.md *Checking in the real app*). Done when: `./build.sh --test` passes and `--export-guides` writes the 12-page PDF.

Group C, check and docs:
- [x] 6. Real-app check: shot 1 adds DawIt Arpeggiator to Keys (`addPlugins`, preset "Up"); shot 2 runs the write-out on Keys' first clip and captures the arrangement (Keys, "Keys (arpeggio)" below, source clip muted) and the piano roll (`capture: bottomPanel`); shot 3 opens `{"guide": "arpeggiator", "topic": 11, "track": "Keys"}`, the new page. Look for: notes on the 1/16 grid, the clip at the source's place, no cut-off text in the page and picture.
- [x] 7. Docs: README *Shape and mix sounds* bullet (the arpeggiator's sentence gains "and writes its pattern out as MIDI notes"); `docs/tests.md` arpeggiator line; `docs/code-layout.md` (Bounce or the new file; "twelve-page"); `docs/architecture.md` *How a change travels* bounce paragraph (the notes-only focus and capture); `docs/recipes.md` shot key; issue #146 progress (and a note on #85, which asks for recording MIDI-effect output).

## Tests
All in *Test numbers*, plus: the source clip's notes and the source track's plugin list are identical before and after; the new track has no MIDI effect and its instrument reports the same state; undo after a moved note is refused; the item's problems (audio track, no MIDI effect, bypassed, muted clip, empty clip, recording) each give their text; a project with the result saves and loads with the same notes.

## Website hand-off
- `features.html`: a sentence in the arpeggiator section: write out the arpeggio as editable MIDI notes on a track below.
- `learn.html`: "What is an arpeggiator?" has 12 pages; a shot of the new page fits.
- Screenshot: the pop song with Keys and "Keys (arpeggio)" below, the new clip open in the piano roll (step 6's shot 2).
- No library, so `about.html` is unchanged.

## Left for later
- A button in the arpeggiator window or chip menu, and write-out of selected bars (decision 1).
- The chord track as input: needs the arpeggiator to follow the chord track first.
- Replacing the source clip in place (decision 1).
- Recording live arpeggiator output while playing (#85, the issue's later enhancement).
- Extending past the clip's end with Latch (decision 6).

## Changes while building

**Group A hand-off (engine, built and tested; `./build.sh --test` passes with no warnings).**
- Capture: `MidiCapture` (events with beat) in `Track.h`; `RenderContext::notesOnly` and `midiCapture`; `BounceFocus::notesOnly` and `capture` (`Engine.h`, passed on in `Engine::renderSegment`); `Track::process` captures after `runMidiEffects` and returns there when `notesOnly`. The `Renderer` adds no latency extra for `notesOnly` and has `renderDiscarding (keepGoing)` (renders, throws the sound away).
- API in `Bounce.h` (namespace `daw::bounce`): `planWriteOut (engine, track, clip = nullptr)` returns `WriteOutPlan` (`problem` holds the item's text: "Only a MIDI track has MIDI effects to write out.", "The track has no MIDI effect that is switched on.", "Stop recording first.", "That clip is muted." / "The track has no unmuted clip.", "That clip has no notes." / "The track's clips have no notes."); `getWriteOutSettings (plan, capture)`; `pairNotes`; `addWrittenOutTrack (engine, plan, capture)` (message thread; fails with the "Nothing came out of the MIDI effects..." text and changes nothing; `WriteOutResult::note` holds the "couldn't be loaded" text); `writeOutArpeggio (engine, track, clip)` does all on one thread.
- For the job (group B): on the message thread make `MidiCapture capture; Renderer r (engine, getWriteOutSettings (plan, capture));`, run `r.renderDiscarding (keepGoing)` on the job thread, destroy the renderer on the message thread, then call `addWrittenOutTrack`. The capture must outlive the renderer. Render range is the plan's range plus `writeOutExtraBeats` (1 beat).
- Undo (Engine): `canUndoArpeggioWriteOut()`, `canUndoArpeggioWriteOut (const Clip&)` (source or result clip), `canUndoArpeggioWriteOut (const Track&)` (result track, or source track of a track write-out), `undoArpeggioWriteOut()`; recorded by `addWrittenOutTrack`.
- Track version: `planWriteOut` without a clip covers the unmuted clips that have notes; one result clip spans their range and the source track is muted.
- Differences/surprises: the sound copy takes each plugin's description from its instance (`getPluginDescription`) and copies state and bypass; the built-in sound and clef are copied too. Notes starting within 0.001 beat of the end count as at the end (a start rounds to a sample, so 4.0 can come out as 3.99998). The "(arpeggio)" track is reused only if it is right below with the same bus and no MIDI effect and a free place. The "recording" problem is in the plan but not tested (no way to start a recording headless). The write-out vs playback test compares pitch lists (the recorder keeps no timing). Saved projects round notes to 15 digits, so the save/load test compares with a tolerance.

**Group B hand-off (UI, built; `./build.sh --test` ends with All checks passed, no warnings; `--export-guides` writes the arpeggiator PDF).**
- `BounceActions.{h,cpp}` (`bounceui`): `writeOutArpeggio (engine, selection, track, clip = nullptr)` (plan, `WriteOutJob` with progress window and Cancel, then `addWrittenOutTrack`; selects the new clip; info box for the "couldn't be loaded" note, warning box on failure), `undoWriteOut`, `hasMidiEffect`, `getWriteOutName` ("arpeggiator" or "MIDI effects"), `findArpeggiator`. No `onBounceClip`-style callback was needed: `TrackLanes` and `TrackActions` call `bounceui` directly, as they already do for WAV export. Only the guide item uses a callback, `TrackLanes::onShowArpeggioGuide`, set in `ArrangementView` (opens the track's arpeggiator window at `arptext::writeOutPage` = 10).
- Menus: clip menu ids 40 (write out), 41 (guide, only when the track has DawIt Arpeggiator), 42 (undo); track menu `writeOutItem`/`undoWriteOutItem` (single MIDI track with a MIDI effect). The item is greyed when `planWriteOut` has a problem (the problem text is not shown, as menus have no tooltips). Undo is shown in the source clip's, the result clip's and the track's menus while `canUndoArpeggioWriteOut` holds.
- Engine: added `Engine::getArpeggioWriteOut()` (public getter of the undo record, used by `undoWriteOut`).
- Guide: new page 11 "Writing the arpeggio out as notes" (picture and Hear this on C4 E4 G4 as 1/16 Up), "Your track" is now page 12 (`pages[11]`, topic index 11), page 9 and 10 texts and the window's overview changed as in *Theory*. Theory Guides summary and words changed; search test uses "finds" (the guide is found, but not first for "write-out").
- Shot key: scene `"writeOutArpeggio"` with `"track"` (and optional `"whole": true`) in `DevScreenshots.{h,cpp}`. For group C: the plan's step 6 says `"topic": 11` for the new page, but the new page is index 10 (0-based) or give its name as the topic string; "Your track" is 11.
- Not done here: the real-app look at the new picture, docs/recipes.md shot key line (only `DevScreenshots.h` documents it so far).

**Group C hand-off (check and docs; `./build.sh --test` passes, no warnings).**
- Real-app check: shots `s1` (device bar), `s2` (whole window after the write-out), `s2b` (piano roll of "Keys (arpeggio)") and `s3` (guide page, topic 10) in the scratchpad `shots/` folder. The guide page, picture, Hear this button and text fit with nothing cut off; the new track's inspector shows the built-in piano and no MIDI effect; the piano roll shows the notes on the 1/16 grid. A shot list cannot scroll the arrangement down to Keys, so the new clip's place in the arrangement was not photographed (the tests check it). The step 6 shot used `"topic": 10`, not 11 (the new page is 0-based 10).
- Docs: README arpeggiator bullet, `docs/tests.md`, `docs/code-layout.md` (twelve-page, BounceActions), `docs/architecture.md` (notes-only focus and capture), `docs/recipes.md` (the `writeOutArpeggio` shot key). Issue #146 has a Built, not committed comment; #85 has a pointer comment. #146 is In Progress.
- No facts or texts beyond the plan's *Theory*.

**Website hand-off.** `features.html`: a sentence in the arpeggiator section, "write out the arpeggio as editable MIDI notes on a track below" (clip and track menus, mutes the chords, one-step undo). `learn.html`: "What is an arpeggiator?" has 12 pages; a shot of "Writing the arpeggio out as notes" fits (shot list entry `{"guide":"arpeggiator","topic":10,"track":"Keys"}`). Window shot: the pop song, shot 1 `addPlugins` DawIt Arpeggiator on Keys with preset "Up", shot 2 `{"scene":"writeOutArpeggio","track":"Keys"}`, then `capture: bottomPanel` for the piano roll. No library, so `about.html` is unchanged.
