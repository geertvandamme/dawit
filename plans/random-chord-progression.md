# Generate a random chord progression in the selected key and bars

Status: Released
Idea: [#129 — Generate a random chord progression in the selected key and bars](https://github.com/geertvandamme/dawit/issues/129)

## Goal
Select some bars in the chord lane and get a starting progression in the song's key, one chord per bar, with no AI model. Try again gives another one. A beginner hears a real progression at once and edits it.

## Decisions for you
1. Selection rules. Options: (a) uniform random in-key triads; (b) weighted by common harmony. **Recommended:** (b): the first bar is the home chord (degree 1) with 70% chance, else degree 4 or 6; the last bar of a run of 4 or more bars is degree 5 (60%) or 1; middle bars pick among degrees 1 to 6 by fixed weights (4 and 5 likeliest, 6 and 2 next, 1 and 3 least), never the same chord twice in a row, never degree 7 (diminished). Fixed numbers in the plan, so tests can check them; it still sounds random.
   Answer: Weighted, as recommended.
2. Where it lives. Options: (a) chord lane right-click menu only; (b) also a Generate panel mode. **Recommended:** (a) only: it is the chord lane's job, and Generate already holds the separate ChatMusician chords. Menu items "Random progression in <key> for bars 3-6" and, after one, "Try another progression".
   Answer: Chord lane right-click menu only, as recommended.
3. Replacement and undo. The chord setters have no undo. Options: (a) reuse the existing "Replace chords?" question (`applyAsking`) when bars already hold chords, plus a one-step "Undo random progression" menu item that restores the previous chord map; (b) question only. **Recommended:** (a), like the transform undo, because Try again would otherwise lose the user's old chords for good.
   Answer: The existing replace question plus a one-step "Undo random progression" item, as recommended.
4. Which bars when nothing is selected. Options: (a) menu item disabled, its tooltip and the lane tooltip say "Select bars first (click, or Shift-drag)"; (b) use the clicked bar plus 3. **Recommended:** (a), as the issue asks for an explanation, not a guess.
   Answer: Disabled with the explanation, as recommended.
5. Guide link. **Recommended:** a last menu item "Explain chord progressions..." already exists; the new item's tooltip names it. No new guide text, so no new facts.
   Answer: Reuse "Explain chord progressions..." and name it in the tooltip, as recommended.

## What the user does and sees
Right-click a bar in a selection (or Shift-drag bars 3 to 6, then right-click): the menu gets "Random progression in C major..." below the type/clear items. Choosing it fills only the selected bars with in-key triads; chords outside stay. If any selected bar already has a chord, the existing question appears ("Bars 3 to 6 already have 2 other chords. Replace them?"). Afterwards the bars stay selected, so the same item, now "Try another progression", rerolls. "Undo random progression" puts back the chords from before the first try. Menu item tooltips: "Picks one chord from the key for each selected bar. Random choices can be edited like any chord." Not saved beyond the chords themselves; nothing new in the project.

## Code
Recipes: *A command* and the chord rows in `docs/recipes.md`. UI-free `gen::chordroll` in `src/generate` (new file, also in `DawItTests`): takes a `MusicalKey`, a bar count and a seed (`std::mt19937`), returns triads from `harmony::triad`. `ChordLane` calls it for `selectionStart`/`selectionBars`, builds the new `ChordMap` over a copy of `engine.getChords()` and goes through `applyAsking` (so conflict question and selection are reused). It remembers the map before the first try for the undo item; the memory is dropped when the selection moves or the song's chords change otherwise. Seed from `juce::Random`; tests pass a fixed seed. Invariants: bars outside the range unchanged; exactly one chord per selected bar; every chord is a degree 1 to 6 triad of the key.

## Libraries and models
None.

## Theory
None new: weights are a design choice, not a claim (the menu says "starting point"). No guide text changes, so nothing in SOURCES.md.

## Steps
Group A (one run, `./build.sh --test` at step 3):
- [x] 1. Add `gen::chordroll` (generate, CMake in both targets): rules of decision 1, seeded. Tests: major and minor keys, 1, 2, 4 and 8 bars, all chords in key and never degree 7, no repeat in a row, first bar home or 4/6, last bar of 4+ is 5 or 1, same seed same result, different seeds differ over 20 tries.
- [x] 2. Add the menu items, tooltips and one-step undo to `ChordLane` via `applyAsking`; disabled with the explanation when nothing is selected. Tests: a headless test of the map built over a copy (outside bars preserved, replacement count right) in `tests/EngineTests.cpp`.
- [x] 3. Real-app check by `--screenshots`: select bars 2 to 5 on the example song, right-click is not reachable by shot list, so add a `ScreenshotRunner` setting `"chordRandom": {"seed": 1}` that runs the action; shot of the filled lane (window crop). Docs: README *Features* bullet, `docs/tests.md`, `docs/code-layout.md`, issue #129 Done/Still open. `./build.sh --test` passes with no warnings.

## Tests
As in steps 1 and 2; the question dialog and menu are checked in the real app.

## Website hand-off
One sentence on the chord lane section: select bars and get a random progression in the key, with undo. One screenshot of the lane after generating.

## Left for later
More than one chord per bar (#102); sevenths or style-aware progressions; a Generate panel mode.

## Changes while building
- Menu items have no tooltip in JUCE's PopupMenu, so the explanation is in the chord lane's own tooltip (random progression, edit or undo) and, with nothing selected, in the disabled item's text. The item has no "..." as it opens no window.
- Test of the map-building (outside bars kept) uses `chordEdits::clearBars` plus the generator in `tests/EngineTests.cpp`; the lane's function itself is UI and was checked in the real app.
- Scene setting `"chordRandom": {"start", "bars", "seed", "clear"}` in `ScreenshotRunner`. Try another progression restarts from the chords before the first try and asks no new question.
- Files: `src/generate/ChordRoll.{h,cpp}`, `src/ui/ChordLane.{h,cpp}`, `src/ui/DevScreenshots.cpp`, CMakeLists, tests.
