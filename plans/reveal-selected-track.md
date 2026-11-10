# Reveal a track when it is created or selected from another view

Status: Released
Idea: [#191 — Reveal a track when it is created or selected from another view](https://github.com/geertvandamme/dawit/issues/191)

## Goal
When you add Keys, Bass or Voice, or pick one in the mixer, inspector or device bar, the arrangement scrolls so that track's row is on screen. A View command, Show Selected Track, does the same on demand. A beginner never has to hunt for the track the inspector is showing. Part of the usability package #175 (audit finding 15).

## Decisions for you
1. Where is Show Selected Track? Options: (a) View menu item with a shortcut, plus an entry in the arrangement's empty-area right-click menu; (b) also a toolbar button. **Recommended:** (a), because the top bar is already full and a menu item is searchable and keyboard-reachable.
   Answer: (a) View menu item with a shortcut, plus the arrangement right-click entry; no toolbar button. (user, 10 November 2026)
2. Selecting a track hidden in a folded bus. Options: (a) unfold the bus(es) so the track shows, then scroll; (b) leave folded and reveal the bus row. **Recommended:** (a), because it is explicit and never leaves the selection invisible; the unfold is a structural change and goes through the engine's bus fold function. Undo is not offered, as folding is a view choice that the user can reverse with the fold arrow.
   Answer: (a) unfold the bus(es), then scroll. (user, 10 November 2026)
3. Should reveal scroll horizontally to the selected clip? **Recommended:** no, only vertical: the issue is about rows, and horizontal moves would fight Follow Playhead.
   Answer: No, vertical only. (user, 10 November 2026)

## What the user does and sees
- Add Audio/MIDI Track (top bar, menu, shortcut): the new row scrolls into view, selected.
- Click a track in the mixer, inspector or device bar (including Return/bus/member choices): the arrangement scrolls vertically the minimum needed to show the full row. If it is already fully visible nothing moves.
- Clicking a track header or a clip in the arrangement itself never scrolls (the row is already under the pointer).
- View > Show Selected Track: reveals the current track on demand, same rules. Disabled with nothing selected. Tooltip: "Scroll the tracks to the selected track, unfolding its bus if needed".
- Playback, Follow Playhead and playhead ticks never trigger a reveal; manual scrolling is never overridden.
- Reveal changes only the scroll position (and bus folded state in case 2). Selection, clip target and notes are untouched.

## Data and saving
No new song data. Scroll position is not saved. Unfolding uses the existing saved `folded` flag.

## Code
Follows the *a command* recipe in docs/recipes.md and the song-data rule that views are `ChangeListener`s.
- `Selection` gains a reveal request: a counter bumped by a new `selectAndReveal` (same as `select`, then bump), so views can tell an explicit cross-view selection from an arrangement click. Plain `select` keeps today's behaviour. Sources outside the arrangement (`MainComponent::addTrack`, `MixerView`, `InspectorPanel`, `DeviceBar`) call `selectAndReveal`; the arrangement's own headers and clips keep `select`.
- `ArrangementView` gets a `revealTrack (Track&)`: unfold ancestor buses via the engine's bus functions, rebuild rows, then scroll `viewport` vertically with the minimum movement using the row index from `routing::getVisibleTracks` times `TrackLanes::rowHeight`. It reacts in its change callback only when the reveal counter changed since last seen, never on timers.
- `showSelectedTrackCommand` in `MainComponent` (View menu, command info with tooltip, shortcut chosen not to clash with existing ones) calls `revealTrack` for `selection.getTrack()`.
- Message thread only; no audio-thread involvement. Risk: Selection broadcasts synchronously, so reveal must run after the arrangement has rebuilt its rows (do it in the same callback after `rebuildTracks`, or defer with `MessageManager::callAsync` guarded by a SafePointer).

## Libraries and models
None.

## Theory
None.

## Steps
Single group (`./build.sh --test` at step 3):
- [x] 1. Add the reveal counter and `selectAndReveal` to `Selection`; switch `addTrack`, MixerView, InspectorPanel and DeviceBar selections to it. Tests: counter bumps only for `selectAndReveal`, not for `select`, and not when nothing changes. Done when: tests compile and pass.
- [x] 2. Add `ArrangementView::revealTrack` (unfold ancestors, minimum vertical scroll) and call it from the change callback on a new counter value; add Show Selected Track to View with tooltip and enabled state. Tests: a pure helper for the target scroll offset (row above, below, inside the view, taller-than-view), and unfolding exposes a track in `getVisibleTracks`. Done when: helper tests pass.
- [x] 3. `./build.sh --test` ends with `All checks passed` and no warnings.
- [x] 4. Real-app check: add a `ScreenshotRunner` shot-list action to select a track via the mixer path if the list cannot already; shots at small window size with 8+ tracks: (a) after selecting Voice from the mixer, its row is visible; (b) a track inside a folded bus is revealed and the bus is open; (c) View menu showing Show Selected Track. Check keyboard access of the command directly.
- [x] 5. Docs: README Features bullet, `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md` selection note, issue #191 comment with Done and Still open lists.

## Tests
New checks in `tests/EngineTests.cpp`: reveal counter behaviour, scroll-offset helper cases, fold exposure, and that reveal leaves `Selection::getTrack()/getClip()` and clip notes unchanged.

## Website hand-off
Mention under the arrangement/tracks section that the tracks list follows your selection and View > Show Selected Track finds it. No new screenshots required beyond the check shots.

## Changes while building
- Decisions answered: 1 = View menu item (Cmd+J) plus an empty-lane right-click entry, no toolbar button; 2 = unfold; 3 = vertical only.
- `Selection` lives in `src/ui`, so the tests cover the pure scroll helper `routing::getScrollToReveal` (src/engine/Routing) and the fold exposure; the reveal counter itself is checked in the real app (screenshots) only.
- Reveal runs through `MessageManager::callAsync` from `ArrangementView::changeListenerCallback`, so rows exist first. New buses, returns and the Create Bus result also use `selectAndReveal`.
- Added `"reveal": "<track>"` to `ScreenshotRunner` (documented in `DevScreenshots.h` and recipes). Shot list: list-level `"window": [1100, 560]`, scale 1; shot 1 `reveal: Bass` (row scrolled into view), shot 2 `fold: Drums`, shot 3 `reveal: Kick` (Drums bus unfolded, Kick shown at top). Both looked right.
- Not checked: keyboard path of Cmd+J and the right-click entry by hand (not covered by screenshots).
- Website hand-off: under arrangement/tracks, say the tracks follow your selection from the mixer, inspector, device bar and new tracks, and that View > Show Selected Track (Cmd+J) finds it. Reuse the shot list's first picture if a screenshot is wanted. No library, model or guide added; no theory facts added.
