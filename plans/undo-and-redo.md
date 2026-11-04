# Undo and redo

Status: Released
Idea: [#92 — Undo and redo](https://github.com/geertvandamme/dawit/issues/92)

## Goal
Cmd+Z undoes the last edit and Cmd+Shift+Z redoes it, for every edit that makes up the song: notes, clips, tracks and buses, the effects in a chain, sends, volume and pan, and the song rows (key, tempo, chords, sections, lyrics). The Edit menu and two transport-bar buttons name the step ("Undo Move notes in 'Lead'"). A beginner can try anything and get back, which is the point of an app made for trying things.

## Decisions for you
1. **How steps are made.** Options: (a) as the issue suggests, JUCE's `UndoManager` with an `UndoableAction` per edit, which means wrapping every edit site (the piano roll, lanes, mixer and many windows change notes and faders directly: about 100 places), and an edit someone forgets is silently not undoable; (b) snapshots: when the app is quiet (no mouse button down), DawIt compares the song with the last saved step and, when it differs, records a step; undo puts the earlier snapshot back. **Recommended:** (b). It covers every edit, today's and future ones, without touching edit sites, and a test can check it generically (the project text before an edit equals the text after undo).
   Answer: (b) snapshots, as recommended. (user, 2026-11-03)
2. **Mute and solo.** Options: part of undo; not. **Recommended:** not. They are listening tools, toggled while listening; undoing a note edit should never unmute a track. Clip mute (and choosing a take) *is* undone: it changes what the song plays.
   Answer: Not part of undo, as recommended. (user, 2026-11-03)
3. **Settings inside effects and instruments** (knobs, presets, starting points). **Recommended:** not in this release; #151 (A/B and undo for plugins) covers them. Adding, removing, moving and bypassing a plugin are undone, and a removed plugin comes back with its settings.
   Answer: Not in this release; left to #151, as recommended. (user, 2026-11-03)
4. **Undo and redo buttons in the transport bar.** Options: menu and shortcuts only; also two buttons. **Recommended:** also buttons, so a beginner sees that undo exists and what it will undo (the tooltip names the step).
   Answer: Also two buttons, as recommended. (user, 2026-11-03)
5. **The existing one-step undos** (Generate panel's Undo and Undo ChatMusician, *Undo transform*, *Undo random progression*, *Undo create MIDI clip from arpeggiator*). Options: keep them for now; replace them in this release. **Recommended:** keep them; they still work (they check the notes are unchanged) and each of their actions is also a history step. Folding them in goes to a follow-up issue.
   Answer: Keep them for now; fold them in through a follow-up issue, as recommended. (user, 2026-11-03)
6. **How many steps.** Options: 50, 100, unlimited. **Recommended:** 100. The history is in memory only and a deleted audio clip's samples are kept while a step needs them.
   Answer: 100 steps, as recommended. (user, 2026-11-03)
7. **Where Cmd+Z works.** **Recommended:** in the main window. A focused text box (a name, the lyrics editor) keeps Cmd+Z for its own text; plugin windows and guide windows leave it to the plugin.
   Answer: The main window; a focused text box keeps Cmd+Z, as recommended. (user, 2026-11-03)

## What the user does and sees
- **Edit menu** (new, between File and Track): *Undo Move notes in 'Lead'* (Cmd+Z) and *Redo Move notes in 'Lead'* (Cmd+Shift+Z; also Ctrl+Y on Windows and Linux). Greyed as plain *Undo* / *Redo* when there is nothing.
- **Transport bar:** two small curved-arrow buttons, left of the transport buttons. Tooltips: "Undo Move notes in 'Lead' (Cmd+Z)" / "Nothing to undo. Every edit to notes, clips, tracks, effects in a chain, sends, volume, pan and the song rows can be undone. Mute, solo, the loop and the settings inside an effect aren't part of undo." and the same for redo.
- A **step** is everything done between two quiet moments: a whole drag of a note or fader is one step; keys held down to nudge notes give a step about every 0.2 s. Undo while dragging does nothing.
- After undo, the selection stays where its tracks and clip still exist; a track or clip that is gone is unselected, and windows of plugins that are gone close. The piano roll drops its note selection when the clip's notes changed.
- Undoing back to what was saved clears "(edited)" in the title (the existing text compare does that).
- New, Open, New from Style and `--demo` start an empty history. Nothing is remembered between sessions; no settings keys.
- Not undone (said in the tooltip): mute, solo, arm, monitor, input, bus fold, scale filter, loop, playhead, plugin settings, view settings.

## Data and saving
No project change; the history is never saved. What a step holds is the song's undoable state (below). Test: for each kind of edit, `project::toText` without plugin state is equal before the edit and after undo, and after the edit and after redo.

## The guide
None: no theory. The tooltips explain what undo covers; the README bullet says it too.

## Code
Follows recipes.md *A command or menu item* (Undo, Redo) and the engine's song-data rules.
- **Shared edit objects.** Tracks, clips and plugin slots become shared-owned (`Tracks`, `Track::clips`, `Track::plugins`), so the edit and history snapshots can both hold them. Pointers stay valid: undo puts back the *same* objects, so `Selection`, open windows and `ArrangementView::rebuildTracks`'s pointer compare keep working.
- **Audio samples never change** once a clip is in the edit: `reverseClip`, `splitClip` and `joinClips` on audio make new clip objects in place of the old ones (callers reselect the result). MIDI notes are copied into snapshots, so they may change in place.
- **A snapshot** (`daw::undo`, new `src/engine/UndoHistory.{h,cpp}`): song data (key, tempo map, chords, sections, lyrics, brief, master volume); per track in edit order, the object, its bus, name, colour, volume, pan, built-in sound, clef, sends (target, level, pre-fader), its plugin slots in order with bypass, and its clips (object, start, name, muted, audio gain, MIDI length and notes). Unchanged note lists and lyrics are shared with the previous snapshot, so a step costs about what it changed. Pure functions: capture, compare, describe.
- **History:** a list of snapshots with a cursor, at most 101 (100 steps), names per step. A commit compares the edit with the snapshot at the cursor and records a step only on a difference, dropping the redo side. Undo or redo first commits any pending change, then restores. An object no snapshot or edit holds is freed outside the lock.
- **Names:** an engine edit function may name the next step ("Split clip", "Reverse in time", "Record"); otherwise `describe` names it from the difference, most structural first: tracks added or removed, tracks moved, plugins, clips, notes (add, delete, move, length, velocity), song rows, mixer, names and colours. One track or clip in quotes: "Delete track 'Bass'", "Change chords".
- **Restore** (Engine, message thread): plugins and tracks re-entering the edit are prepared as `addPlugin` prepares them (outside the lock); then, under `getLock()`, the track list, buses, sends, chains, clips and fields are written back, `updateRouting()` runs and held notes are released (`midiEditVersion`); song rows go through their setters; then one change message. Fields outside the snapshot are left as they are. It returns the tracks, clips and processors that left the edit, for the UI to forget.
- **Refusals:** no commit and no undo while recording, offline (bounce), a modal window is up, or a mouse button is down. `resetEdit` and `project::load` clear the history and take the loaded song as the start.
- **UI:** `MainComponent` owns the quiet-moment timer (5 Hz) and the commands; the timer's compare is cheap (no copies unless something differs).
- **Risks:** the ownership change touches `Engine.cpp`, `Track.h`, `ProjectFile.cpp` and tests (about 15 explicit sites, compile errors find the rest); restore is an audio-thread structural change (keep it to one lock); revived hosted plugins must be prepared again at the current rate.

## Libraries and models
None.

## Theory
None.

## Steps

Group A1, engine: shared objects and snapshots (tests target at step 3):
- [x] 1. Make tracks, clips and plugin slots shared-owned; keep every removal path's behaviour (plugins released and destroyed outside the lock). Tests: all existing pass. Done when: `DawItTests` passes with no warnings.
- [x] 2. Audio `reverseClip`, `splitClip`, `joinClips` replace clips with new objects and return them; update `TrackLanes` and `WaveEditor` callers to select the result, and existing tests that kept a pointer to the left part. Tests: reverse returns a new clip, the old object's first sample is unchanged; split and join leave the source objects' samples unchanged.
- [x] 3. `daw::undo` snapshot: capture (sharing unchanged notes and lyrics with a previous snapshot), compare with the edit, describe. Add both CMake targets. Tests: *Tests* items 1 to 4. Done when: the tests target builds and passes.

Group A2, engine: history and restore (`./build.sh --test` at step 6):
- [x] 4. History in `Engine`: commit, undo, redo, names, limit, refusals while recording or offline; cleared by `resetEdit` and at the end of `project::load`. Tests: items 5 to 8.
- [x] 5. Restore under one lock as in *Code*, returning what left the edit; prepare revived plugins and tracks. Tests: items 9 to 16 (the `toText` round trips and sound through the fake device).
- [x] 6. Step names from engine edit functions (split, join, reverse, transpose, transform, add or remove track, bus, return, plugin, take choice, recording as one "Record" step). Tests: item 17. Done when: `./build.sh --test` ends with `All checks passed`, no warnings.

Group B, UI (`./build.sh --test` at step 10):
- [x] 7. `MainComponent`: Edit menu (shift `getMenuForIndex`'s indices), Undo and Redo commands with names and shortcuts, a focused `juce::TextEditor` gets the undo instead; the 5 Hz quiet-moment commit, `commandStatusChanged()` after each commit, undo and redo. After undo or redo: `Selection::forget` what left, close its plugin windows (`PluginWindowManager::closeFor`), reset the live MIDI track, clear the piano roll's note selection. Check: menu items enable and rename.
- [x] 8. Transport bar undo and redo buttons with the tooltips above (if decision 4 stays). Check: buttons grey out with no history.
- [x] 9. `ProjectManager` (New, Open, New from Style) and `--demo` start an empty history after the song is built; a song template's build is not a step.
- [x] 10. `ScreenshotRunner`: shot keys `"undo": n`, `"redo": n` and `"deleteTrack": "<name>"`, each committing first as the timer would; document them in recipes.md *Checking in the real app*. Done when: `./build.sh --test` passes, no warnings.

Group C, check and docs:
- [x] 11. Real-app check, one run on the pop song: (a) `chordRandom` bars 2 to 5 seed 5, capture the chord lane; (b) `"undo": 1`, the chord lane is as in the plain song; (c) `"redo": 1`, as (a); (d) `transform: reverse` on Lead, then `"undo": 1` with the clip editor open: notes as before; (e) `"deleteTrack": "Bass"` then `"undo": 1`: Bass back in its bus with its effects in the device bar; (f) the transport bar area: buttons enabled after an edit, greyed on a fresh song. Look for clipped text and stale faders.
- [x] 12. Docs: README *Features* bullet (Cmd+Z, Cmd+Shift+Z, what is and isn't undone); docs/tests.md (the undo area); docs/code-layout.md (`UndoHistory`); docs/architecture.md (*The edit*: shared objects, immutable audio samples, the history and restore; *How a change travels*: the quiet-moment commit); recipes.md *Song data that's saved*: new song data goes into the snapshot and `describe`; ui-design.md *Interaction*: Cmd+Z and Cmd+Shift+Z. GitHub #92 progress; open the follow-up issues in *Left for later*.

## Tests
`runUndoTests` in `tests/EngineTests.cpp` (fake device where sound is checked):
1. Capturing the same edit twice compares equal; changing one note's velocity, one chord, a track's volume by 0.5 dB, a send level or a clip's start each compares different.
2. A snapshot after a one-note change shares the other clips' note lists and the lyrics with the previous one (same storage).
3. Mute, solo, arm, fold, scale filter, loop and a built-in plugin's parameter don't make a difference.
4. `describe`: one note added → "Add note in 'Lead'"-style name naming the track; a track removed → "Delete track 'Bass'"; chords changed → "Change chords"; a track removed and notes changed → the track name wins.
5. No step when nothing changed; one step after an edit; undo and redo flags and names.
6. A new edit after undo drops redo.
7. 105 commits keep 100 steps: 100 undos work, the 101st returns false.
8. Undo is refused while recording and while offline; `resetEdit` and loading a project leave no steps, and loading isn't undoable.
9. For each: add and delete a note, move a MIDI clip, delete an audio clip, split and join (audio and MIDI), reverse audio, add and delete a track, create a bus from two tracks and ungroup, move a track into a bus, add a return with two sends then delete it, add, move, bypass and remove a plugin (a MIDI effect stays first), change key, tempo, chords, sections, lyrics, volume and pan: `toText` without plugin state equals the text before after undo, and the text after after redo.
10. Undo of a deleted track gives back the same `Track*` with the same `PluginSlot*` objects.
11. Undo of a deleted audio clip gives back the same object with the same samples (first, middle and last sample equal).
12. Undo of reverse plays the original: the first sample of a rendered block through the fake device matches the original clip's start within 1e-6.
13. A deleted MIDI track with a DawIt Subtractive comes back and is heard: peak above -60 dBFS over 0.5 s of a held note.
14. Undoing a deleted bus puts its tracks back inside it, and the render order has every track before its bus.
15. A built-in plugin's parameter set after its removal and undo keeps its value (settings aren't undone).
16. Restore reports the tracks, clips and processors that left the edit; mute and solo set after the edit survive undo.
17. Engine-named steps: split → "Split clip", reverse → "Reverse clip", transform → its kind's name, recording a MIDI take → one "Record" step.

## Website hand-off
- features.html: in the group for working with the arrangement or the app itself, a short line: undo and redo for every edit, with what isn't undone.
- No new screenshot needed; if one is wanted, the transport bar with the undo button's tooltip on the pop song.
- Not for Home or for-you.html; no How it's made change.

## Left for later
- Plugin settings, presets and A/B (#151).
- Folding the one-step undos (Generate panel, transform, random progression, arpeggio write-out) into the history and removing their buttons (new follow-up issue).
- An undo history window that lists the steps to jump back several at once (new follow-up issue).
- A short "Undid Move notes" message on screen after undo.
- A memory cap for audio kept only by old steps (now bounded by the 100 steps).
- Undo inside plugin and guide windows.
- Mute and solo, if decision 2 is answered differently later.

## Changes while building

**Group A1 hand-off (built, tests pass, no warnings; the DawIt app target also builds).**
- `Tracks`, `Track::clips`, `Track::plugins` and `takes::ClipList` are now `std::shared_ptr` (a type swap in `Track.h`, `Clip.h`, `Engine.{h,cpp}`; `Track::sends` stays `unique_ptr<Send>`, snapshots hold sends by value). Removal paths are unchanged: they already `reset()`/`clear()` their local owners outside the lock. Group A2 must keep that when the history also holds the objects (free the last reference outside the lock).
- Audio clips are replaced, not changed: `Engine::replaceClip (Track&, const Clip& old, shared_ptr<Clip> fresh)` (private; swaps in place under the lock, bumps `midiEditVersion`, returns the old one for the caller to free). `reverseClip` now returns `AudioClip*` (the new object, null when off a track); `splitClip (track, clip, beat, Clip** leftPart = nullptr)` returns the right part and gives the new left part through `leftPart` (the same clip for MIDI); `joinClips` returns a new clip for audio (MIDI still in place). `TrackLanes` (reverse, split) and `WaveEditor` (reverse button) reselect the result. Other UI holders of a `Clip*` (not found any that matter) would need the same if they appear.
- `src/engine/UndoHistory.{h,cpp}` (both CMake targets): `daw::undo::Snapshot` (key, tempo, chords, sections, `shared_ptr<const Lyrics>`, brief, master volume, `TrackState` per track holding the `shared_ptr<Track>`, bus, name, colour, volume, pan, built-in sound, clef, `SendState`s, `PluginState`s (`shared_ptr<PluginSlot>` + bypass), `ClipState`s (`shared_ptr<Clip>`, name, start, muted, gain, MIDI length and shared `NoteList`)). Functions: `capture (const Engine&, const Snapshot* previous = nullptr)` shares unchanged note lists and lyrics; `matches (const Snapshot&, const Engine&)` compares without copies (for the 5 Hz timer); `equal (Snapshot, Snapshot)`; `describe (before, after)` gives the names ("Delete track 'Bass'", "Ungroup 'Bus 1'", "Add note in 'Lead'", "Move notes in 'Lead'", "Change chords", ...; empty when equal). Clip-level differences where audio clips were replaced describe as "Change clips on 'T'"; step 6's engine-named steps ("Split clip", "Reverse clip") should override that.
- Tests: `runUndoSnapshotTests` (items 1 to 4) in `tests/EngineTests.cpp`; the split/join/reverse tests now check the source objects' samples stay unchanged.
- Snapshot sends hold the target `Track*`; restore (A2) rebuilds `Send` objects from values. A snapshot does not compare mute, solo, arm, monitor, input, fold, scale filter, loop or plugin parameters.
- Note: `build/Release` had no tests target configured; I ran `cmake -B build/Release -DDAWIT_BUILD_TESTS=ON` once.

**Group A2 hand-off (built; `./build.sh --test` result below).**
- API on `Engine` (message thread): `commitHistory()` (records a step when the edit differs from the cursor snapshot; false while recording or bouncing; cheap when nothing changed, so the 5 Hz timer can call it), `undo (HistoryChange* = nullptr)`, `redo (HistoryChange* = nullptr)` (undo commits a pending change first; both false while recording or bouncing), `canUndo()`, `canRedo()`, `getUndoName()`, `getRedoName()` (empty when none), `getNumUndoSteps()`, `getNumRedoSteps()`, `clearHistory()` (takes the current edit as the start; called at the end of the constructor, `resetEdit()` and `project::load`), `nameNextStep (name)`, `Engine::maxHistorySteps` (100).
- `Engine::HistoryChange` has `tracksGone` (`const Track*`), `clipsGone` (`const Clip*`) and `processorsGone` (`const juce::AudioProcessor*`, the plugin instances) for the UI to forget (`Selection::forget`, `PluginWindowManager::closeFor`). The objects stay alive while any step holds them, but must not be used. Restore also resets nothing in the UI: Group B must clear the live MIDI track (the engine already nulls `liveMidiTrack` when its track left), the piano roll's note selection, and call `commandStatusChanged()`.
- Engine does not check mouse buttons or modal windows: Group B's timer must skip the commit (and refuse undo/redo) while `juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown()` or a modal component is up.
- Restore (`Engine::restore`, private): revived tracks go through `Track::prepare`, revived plugins on kept tracks through `prepareToPlay` (outside the lock); one lock writes back track order, buses, fields, sends (existing `Send` objects are reused so gains don't glitch), plugin chains with bypass, clips (same objects, fields and MIDI notes), master volume, then `updateRouting()` and `++midiEditVersion`; song rows go through their setters after. Departed tracks and plugins get `releaseResources()`; everything is freed outside the lock. Mute, solo, arm, monitor, fold, scale filter, loop and plugin parameters are left as they are.
- Step names: `describe` names most steps; engine functions name the rest through `nameNextStep`: "Split clip", "Join clips", "Reverse clip", "Transpose notes", the transform's kind name, "Choose take", "Delete other takes" and "Record" (`startRecording` first commits what was done before, then names the next step, so a take is one step; commits are refused during recording). Add/delete track, bus, return and plugin steps are named by `describe` ("Delete track 'Bass'" and so on), not by the engine functions. The latest name before a commit wins, and a commit clears it.
- Surprise: `startRecording` creates its empty take clips at once, so the pre-recording commit has to come before them (it does).
- Tests: `runUndoHistoryTests` (items 5 to 17) in `tests/EngineTests.cpp`. Audio reverse can't be checked by project text (it holds no samples): that test checks the object and the rendered sound instead.

**Group B hand-off (built; `./build.sh --test` ends with `All checks passed`, no warnings).**
- `MainComponent`: Edit menu (menu indices now File 0, Edit 1, Track 2, View 3, Help 4; the shot runner's `getMenuForIndex` call was moved to 3), `undoCommand`/`redoCommand` (Cmd+Z, Cmd+Shift+Z, Ctrl+Y off Mac; names from `engine.getUndoName()`/`getRedoName()` in `getCommandInfo`), `stepHistory (bool redo)` (private; focused `TextEditor` gets the key, a focused component in another window is left alone, refuses while `isHistoryBusy()`), 5 Hz `timerCallback` commit then `commandStatusChanged()`. After a step: `Selection::forget`, `closeFor` on gone tracks and processors, live MIDI track nulled, `PianoRoll::clearNoteSelection()` (new).
- `TransportBar`: two `GlyphButton`s (new glyphs undo/redo) left of the transport buttons, `onUndo`/`onRedo`; its 30 Hz timer refreshes enabled state and tooltips (`updateUndoButtons`).
- `--demo` (`createDemoContent`) and `ProjectManager::newProjectFrom` call `engine.clearHistory()` after building; New and Open already clear through `resetEdit`/`project::load`.
- `ScreenshotRunner` keys `"deleteTrack"`, `"undo": n`, `"redo": n` (applied after `transform`/`chordRandom`, before the scroll keys); documented in recipes.md. Each commits first.

**Group C hand-off (built; `./build.sh --test` ends with `All checks passed`, no warnings).**
- Real-app check on the pop song (`--screenshots`, scale 1, 1300 x 760): (a) a random progression on bars 2 to 5 changed the chord lane; (b) `undo: 1` brought back Am F C G; (c) `redo: 1` gave (a) pixel for pixel; (d) with the clip editor open, `transform: reverse` then `undo: 1` put the Lead vocal's notes back (the editor stays open on the same clip) and `redo: 1` was identical to the reversed shot; (e) `deleteTrack: Bass` then `undo: 1` brought Bass back with its Synth Bass instrument in the device bar (the pop song's Bass has no effects); (f) the transport bar's two buttons are greyed on the fresh song and enabled after an edit. Nothing clipped, no stale fader seen.
- Fixed: `ScreenshotRunner` now calls `engine.clearHistory()` after building the song, so a shot list starts with an empty history (the undo button showed enabled on a fresh song before).
- The "premature end of file" Ninja warning came from a damaged `.ninja_deps`/`.ninja_log` in `build/Release`; deleting both and rebuilding cleared it.
- Not checked by hand: Cmd+Z in a focused text box (a track name, the lyrics editor) giving the box its own undo, which a headless run can't type. Worth one manual try in the real app.
- Docs: README bullet *Undo and redo*, `docs/tests.md`, `docs/code-layout.md`, `docs/architecture.md` (*The edit*, *How a change travels* item 6), `docs/recipes.md` (song data step 5; shot keys), `docs/ui-design.md` (*Interaction*).
- Follow-up issues: #152 (fold the one-step undos into the history), #153 (an undo history window, with the on-screen "Undid ..." message).
- Website hand-off: see *Website hand-off* above. features.html: one line on undo and redo for every edit and what isn't undone; no new screenshot needed (if wanted: the transport bar with the undo tooltip on the pop song); no Home, for-you or How it's made change.

**Fix after Group B (track rename and undo).** Cause: `TrackHeader` set its name label's text only in its constructor, and undo keeps the same `Track` objects, so `ArrangementView::rebuildTracks` kept the headers: the name was restored in the song but the header kept showing the new name (the fx chain's name box reads the track again, which is why it looked right there). Not a focus problem: the focused `TextEditor` path in `stepHistory` is unchanged, and a Label's editor is a `juce::TextEditor`. Fix: `TrackHeader::timerCallback` follows `track.name` unless the label is being edited. It is the only editable `juce::Label` in `src/ui`. Test: renaming a track is a step that undo and redo carry back and forth (the label is UI and not headlessly testable).
