# Built-in synth plays piano for drums and percussion

Status: Released
Idea: [#210 — Built-in synth plays piano for drums and percussion](https://github.com/geertvandamme/dawit/issues/210)

## Goal
On a MIDI track whose DawIt Synth sound is *Drums* or *Percussion*, every note you add, click or audition sounds as that drum or percussion instrument, never as a piano. Guide examples of drum grooves also sound as drums. A beginner building a first beat hears the kick, snare and hats they are writing, which is the point of the exercise.

## Cause (found by reading the code; not reproduced by ear)
- Song **playback is not affected**: `Track::process` feeds clip notes to the track's own `BuiltInSynth`, which reads the chosen sound at each note-on and plays drums correctly (covered by `runBuiltInSynthTests`).
- **Previews are**: `Engine::previewNote` and `previewMessage` put notes on one queue. Each audio block, `Engine::isOwnPreviewSoundNeeded` sends that queue to the engine's own `previewSynth` instead of the selected track when the track is a Drums/Percussion DawIt Synth track ("chords would play as drums"). The own synth plays `previewSound`, which defaults to piano and which most guides set back to piano. So on a drum track:
  - clicking or drawing a note in the piano roll grid, clicking the piano roll's key strip or a note in the score plays piano;
  - picking the *Drums* or *Percussion* tile in the DawIt Synth window plays its demo notes (36 + 42, 63 + 70) as piano;
  - the What is MIDI? pictures that play your clip's notes (`MidiExplainer` `playNotes`) play piano.
- Two guide examples play drum notes on whatever is selected, so they are piano too: the Styles guide's groove (`StylesExplainer::playGroove`, sets no preview sound) and, with a pitched track selected, the Instruments guide's drum-kit example.
- The rule dates from the first public commit (29 Oct). It became the main path recently: the *Start your song* drums tile and the first-song rhythm step (14 Nov, 86d2909) and the *Build a Beat* exercise (16 Nov, da32a88) open a Drums track's clip and say "Click in the grid … you hear it as you add it". That is why it reads as a regression. Computer-keyboard and MIDI-keyboard playing go through live MIDI, not previews, so they already play drums.

## Decisions for you
1. **Scope.** Options: (a) only previews of the track's own notes (piano roll, key strip, score, synth window, MIDI guide's clip playback); (b) also the guide drum examples (Styles groove, Instruments drum kit) that play piano when no drum track is selected. **Recommended:** (b): same cause, small, and both are heard by beginners.
   Answer: Defer DawIt Drums and Sampler drum-kit recognition to separate work. Keep pitched guide examples on a pitched sound. Fix track-note previews and guide drum examples.
2. **Pitched guide examples while a drum track is selected** (chord lane, Chords, Key, Rhythm, Reading, Lyrics, Singing guides). Options: keep them on DawIt's own pitched sound (today); or play them on the drum track. **Recommended:** keep today's rule: a chord played as drum hits teaches nothing.
   Answer:
3. **Other drum instruments.** The "plays drums" test only knows the DawIt Synth sounds; a DawIt Drums or DawIt Sampler drum-kit track gets pitched guide examples as drum hits. Options: widen the test now; or leave for later. **Recommended:** later (a separate small issue), to keep this fix to the reported bug.
   Answer:

## What the user does and sees
Nothing new on screen. On a Drums or Percussion track: a grid click on row 36 plays the kick, 38 the snare, 42 the closed hat; the key strip and score do the same; the synth window's *Drums* tile plays kick + closed hat, *Percussion* plays its two congas. The Styles guide's *Play* groove and the Instruments guide's drum-kit example play the kit whatever track is selected. Chords and melodies from guides still play on a pitched sound while a drum track is selected (decision 2). No settings, no tooltips change.

## Data and saving
None. No project or settings change.

## The guide
None. No guide text changes: the What is MIDI? picture already says it plays "on the clip's track", which becomes true.

## Code
Recipes: none fits (an engine routing fix). Follow *Engine and data* in CLAUDE.md.

- **A preview says what it is for.** Three kinds: *track* (notes belong to the selected track: play on its own instrument, drums included), *pitched* (a guide's chord or melody: today's rule) and *drums* (a guide's drum example: on the selected track only if it plays drums, otherwise DawIt's own synth in the Drums sound). Default stays *pitched*, so untouched callers keep today's behaviour.
- **Routing is decided once, on the message thread, when the note-on is sent**, and the matching note-off (and any `previewMessage` that follows) goes the same way. Today the choice is made per audio block from `liveMidiTrack`, so a selection change between note-on and note-off can leave a note hanging on the other synth; deciding at the call fixes that too.
- **The engine owns two own synths**: the pitched one on `previewSound` (unchanged) and a drum one fixed on Drums, so a groove and a chord preview in the same block don't fight over one sound atomic. Both are prepared with the sample rate and rendered in the same place as `previewSynth`.
- **Audio thread**: one preview queue per destination (selected track, own pitched synth, own drum synth), each a lock-free collector like `previewCollector`; the track queue goes to the live track through `ctx.previewMidi` as now. No new lock use.
- **With no MIDI track selected** (none, or an audio track) a *track* preview plays on the own pitched synth, as today.
- Callers: *track* for `PianoRollGrid::preview`, `PianoKeyboardStrip::mouseDown`, `ScoreView`, `BuiltInSynthEditor::pick`, `MidiExplainer` `playNotes` and its clip controller playback; *drums* for `StylesExplainer` groove and the `InstrumentsExplainer` drum-kit topic (replacing its `setPreviewSound (drums)` there); every other caller unchanged.
- `previewsUseOwnSound()` and `isOwnPreviewSoundNeeded` keep their meaning for *pitched*; rename or comment them so the name says so. Update the comment on `setPreviewSound` and on `BuiltInSynthEditor::pick`.
- **Risk:** an audio-thread change in `Engine::processBlock`, small. Bounce and offline renders must ignore all preview queues as today.

## Libraries and models
None. No dependency is added, changed or removed, so there are no licence obligations and `LICENSE`, `licenses/` and the README's *License note* stay as they are.

## Theory
None.

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. Add a failing test first: a Drums track selected, `previewNote (36)` as a *track* preview is heard on the track (its built-in synth peak rises) and the own synths stay silent. Done when it fails on today's code for the stated reason.
- [x] 2. Add the preview kinds, routing at the call, the per-destination queues and the own drum synth, per *Code*. Tests: see *Tests*. Check: the step-1 test and the existing "Guides can always be heard" checks pass unchanged.
- [x] 3. Update the comments on the preview functions and `isOwnPreviewSoundNeeded`. Done when `./build.sh --test` ends with `All checks passed` and no new warnings.

Group B, UI (`./build.sh --test` at step 5):
- [x] 4. Switch the *track* callers (piano roll grid and key strip, score view, DawIt Synth window tiles, What is MIDI? clip playback and its controller messages) to *track* previews.
- [x] 5. Switch the Styles guide groove and the Instruments guide drum-kit example to *drums* previews; drop the drum-kit `setPreviewSound (drums)`. Done when `./build.sh --test` passes.

Group C, check and docs:
- [x] 6. Real-app check: nothing visible changes, so no new shots. Run the existing shots for the Build a Beat exercise and the DawIt Synth window to confirm nothing moved. The listening check (add Drums from *Start your song*, click rows 36, 38, 42; pick the Drums and Percussion tiles; play the Styles groove with a Piano track selected) is for the user by ear; list it in the hand-off.
- [x] 7. Docs: `docs/architecture.md` gets one line under how a change reaches the speakers: previews have a kind and are routed when sent. `docs/tests.md`: the new preview checks on the existing previews line. No README bullet (behaviour already described). GitHub issue #210: progress comment with the cause, the fix and the tests; it stays open until the user has listened.

First steps: no change. The *Start your song* drums tile, first-song rhythm step and Build a Beat captions already promise you hear the drum you add; the fix makes that true.

## Tests
In the existing previews block of `tests/EngineTests.cpp`, with a running fake device:
- Drums track selected: a *track* preview of 36 raises the track's built-in synth peak above 0.001; both own synths stay below 1e-6.
- Same with Percussion and note 63.
- Piano track selected: a *track* preview is heard on the track (unchanged).
- Drums track selected: a *pitched* preview of 60 is heard on the own pitched synth and the track's peak stays below 1e-6 (today's rule kept).
- Piano track selected: a *drums* preview of 36 is heard on the own drum synth, not the track; with a Drums track selected it is heard on the track.
- No track selected: *track* and *pitched* previews play on the own pitched synth; *drums* on the own drum synth.
- Routing is fixed at the note-on: select another track between a *track* note-on and its note-off; after 0.5 s no voice is still active on either synth.
- A bounce rendered while previews are queued contains none of them.

## Website hand-off
None: a bug fix with no new feature or screenshot. dawWebsite needs no change.

## Left for later
- Decision 3: treat DawIt Drums and DawIt Sampler drum-kit tracks as drum tracks for pitched guide examples (separate issue if accepted).
- A test that tells a drum from a piano by sound alone (attack and decay shape), if routing tests prove not enough.

## Changes while building

### Group A hand-off (engine, built; not committed)
- `Engine::PreviewKind { track, pitched, drums }` in `src/engine/Engine.h`; `previewNote (pitch, velocity, durationMs, kind = pitched)` and `previewMessage (msg, kind = pitched)`. Group B passes `Engine::PreviewKind::track` / `::drums` at the callers listed in *Code*; untouched callers stay *pitched*.
- Routing is resolved at the call in `Engine::resolvePreview`; the note-off reuses the destination. Three queues (`previewCollector` = live track, `previewOwnCollector`, `previewDrumCollector`) and a second own synth `drumPreviewSynth` (fixed on Drums) in `Engine::processBlock`. `previewsOnOwnSynth` is gone.
- Departure from the plan: when the live track changes, `setLiveMidiTrack` queues all-notes-off for the old live track (`previewFlushTrack`), so a track-kind note cannot hang on it (the note-off alone would reach the new live track). (Checked in Group B: the count-in is not an exception, the all-notes-off reaches the old track there too.)
- Step 1 (test first) not run separately: the test was written with the fix and the fix was not built without it, so "fails on old code" is unproven; the old code has no `PreviewKind` to call.
- Tests (in the existing previews block, `tests/EngineTests.cpp`): 13 new checks (track on drums/percussion/piano, pitched on drum track, drums on piano/drums/none, track/pitched with none, selection change mid-note). The bounce check was added in Group B (see below). The drum-on-drums check allows a ringing tail from the previous drum (own < 5% of track), as the own drum synth's note rings beyond 1.5 s.
- `./build.sh --test`: `All checks passed`, no compiler warnings. Group B followed.

### Group B hand-off (UI, built; not committed)
- *track* previews: `PianoRollGrid::preview`, `PianoKeyboardStrip::mouseDown` (`PianoRoll.cpp`), `ScoreView.cpp`, `BuiltInSynthEditor.cpp` (tiles), `MidiExplainer.cpp` (`playNotes` and `playControllerView`, notes and controller messages). The MIDI keyboard picture's click (line ~225) stays *pitched*: it is a piano picture, not the clip's notes.
- *drums* previews: `StylesExplainer::timerCallback` groove; `InstrumentsExplainer` drum-kit example (new member `playsDrums`, set in `showTopic`; the picture's tile click uses *drums* when its topic is the drum kit, a small extra beyond the plan). `setPreviewSound (drums)` removed there; it stays piano.
- Count-in edge (Group A): not real. During a count-in `renderSegment` still runs from sample 0 of the block, so the all-notes-off at sample 0 reaches the old track. Hand-off line corrected.
- Bounce test added and it found a real bug: queued previews leaked into a bounce (the own synths were rendered while `offline`; track previews would have too). Fix in `Engine::processBlock`: while offline the preview buffers are dropped and both own synths are cut with `allNotesOff`. The new check failed before the fix.
- Build: `./build.sh --test` ends with `All checks passed`, no compiler warnings. Group C (real-app shots, docs) left; the by-ear check is the user's.

### Group C hand-off (check and docs; not committed)
- Real app: ran the existing `buildBeat` exercise window and `firstSong` scene shots (scale 1, window capture; `/tmp/s210/out/`, not kept). Both look as before: nothing visible changed, no new shots. The DawIt Synth window has no shot key in `DevScreenshots.h`, so it was not photographed (only the call arguments of its tiles changed). Previews are sound, so the real check is by ear (below), not done by anyone.
- Docs: `docs/architecture.md` (previews have a kind, routed once when sent, a bounce drops them), `docs/tests.md` (the new preview checks after the built-in synth line). No README change (behaviour already described), no `code-layout.md` change (no new files).
- First steps: checked, no change (texts already promise you hear the drum you add; this makes it true).
- Final `./build.sh --test` (Group B): `All checks passed`, no compiler warnings. Docs-only edits since, so no rebuild.
- GitHub #210: no comment posted (not allowed in this run). Still to do when allowed: a progress comment with cause, fix and tests; keep the issue open until the user has listened.
- **Listening check for the user:** (1) New song, *Start your song* drums tile, click grid rows 36, 38, 42: kick, snare, closed hat. (2) DawIt Synth window: pick Drums (kick + hat) and Percussion (two congas). (3) Select a Piano track, open the Styles guide, Play a groove: drums, not piano. (4) Instruments guide, Drum kit topic: example and tile clicks are drums. (5) With a Drums track selected, play a chord from the Chords guide: still pitched, by design. (6) Click a note on a Drums track, change the selected track at once: no note hangs.
- Not covered: DawIt Drums / DawIt Sampler drum-kit tracks (deferred by decision 3); a sound-only test that tells drum from piano.
