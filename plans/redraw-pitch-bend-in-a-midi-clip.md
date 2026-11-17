# Bug: redrawing the pitch bend in a MIDI clip does not work properly

Status: Released
Idea: [#192 — Bug: redrawing the pitch bend in a MIDI clip does not work properly](https://github.com/geertvandamme/dawit/issues/192)

## Goal
Drawing a bend (or any controller) over an existing curve replaces exactly the beats the drag covered, in either direction, so the lane, playback and undo agree. Small fix: one recipe, no saved data, no theory.

## Cause (found by reading `ControllerLaneArea::updateDrag`, `Mode::draw`, and `controllers::getLinePoints`; traced with a Python mirror of the logic; confirmed by the fix and the real-app check below)
1. **Dragged backwards:** `getLinePoints` swaps its ends and returns the points in ascending beat order, so `pointsBefore.back()` is the far end, not the cursor. The next step then starts from the wrong place and the path zigzags (duplicate beats, values from two paths mixed).
2. **Flat stretch:** `getLinePoints` and the `drawn` filter skip repeated values, so the cursor's beat is never recorded. The next step starts a line from the start of the flat stretch (a ramp instead of flat then a rise), and old points under the flat stretch beyond the last recorded point are not removed (stale).
3. **Replaced range** `lo..hi` comes from the recorded points only, so it stops short of the cursor (same stale-point effect). One gesture is one undo step (checked).

## Decisions for you
1. How a drawn range ends. Options: (a) replace from the first to the last beat the cursor reached, with the cursor's value written as the last point; (b) keep today's range. **Recommended:** (a), it is what the issue's acceptance checks say.
   Answer: Replace the whole range the cursor reached and record its final value.
2. Bend value snapping to centre and semitones without Alt stays as it is (looks like design, not a cause). **Recommended:** keep.
   Answer: keep, approved. Keep existing centre and semitone snapping.

## What the user does and sees
No new controls. Click-drag in the Bend lane (either direction, back and forth, flat then up, over old points) shows the curve just drawn, nothing older inside the covered beats, and the first and last drawn points on the cursor. Shift-line and Alt behave as before.

## Steps
1. Keep the draw path as a time-ordered list that tracks the cursor (beat and value) separately from the recorded points; each step draws from the previous cursor to the new one, in the drag's direction, so a backward step is a line from the old cursor, not from the far end. In `ControllerLane.cpp`. Done when the path has no duplicate beats and is ordered.
2. Replace `lo..hi` using the whole range the cursor has covered (earliest to latest beat reached), and always write the cursor's current value as a point (so a flat stretch has an end point; later repeats of the same value inside a stretch may still be dropped). Move this to a pure helper in `MidiControllers.h` (`drawPath`-style, name to choose) that the UI calls, so it can be tested without the UI.
3. Make sure one drag is one undo step (check how other lane edits group; reuse it). Tests: `tests/EngineTests.cpp`, in `runControllerLaneTests`: flat then rise stays flat until the rise; flat over old points leaves none in the range; backward drag equals the same curve drawn forward; zigzag (right, back, right) ends with the last path; other kinds and notes untouched; one undo restores. Add to `docs/tests.md`. `./build.sh --test` passes.
4. Real-app check and docs (below). Docs: update the Draw line in `ControllerLane.h` comment and `docs/tests.md`; `docs/code-layout.md` only if a file's role changes.

## Real-app and engine validation
- Engine: the tests above, via `./build.sh --test`; also playback: a clip with a redrawn bend gives the drawn values through `controllers::getSpanMessages` (bend at start, mid, end).
- Real app: a `--screenshots` shot list (recipes.md, "checking in the real app") on the demo song, piano roll open with the Bend lane: draw a bend; redraw over part of it forward; redraw backwards; flat then rise; Shift-line; Alt. Before/after images must show no stale points or flat gaps; then undo once and capture to show the original bend. If the shot runner cannot do a drag, add the smallest scene key to `ScreenshotRunner` (named in the plan's notes at build) or do the drags by hand in the app and say so.
- First steps: checked at build; no change expected (no start panel, name or menu changes).

## Dependencies and licence
None. No new library, no `LICENSE`, `licenses/` or `SOURCES.md` change. Runs fully local.

## Website hand-off
None (a bug fix); website proposal to follow release.

## Open
Nothing beyond #163. Not done: the new tests were not run against the old code, and the failure in the old code is shown only by the Python trace, not in the app.

## Changes while building
- `MidiControllers.h`: new `extendDrawPath` (path of cursor points in drag order, repeats kept, either direction) and `applyDrawPath` (replaces the kind's events between the first and last beat reached; later pass wins; repeats dropped but first and latest-beat points kept; tidied). `ControllerLane.cpp` `Mode::draw` now uses them (`pointsBefore` is the path).
- Tests in `runControllerLaneTests` (flat then rise, flat over old points, other kinds untouched, backward equals forward, ordered, zigzag later pass wins, single point); `docs/tests.md` line added.
- Real-app check: new `checkRedraw` key on the `controllerLanes` scene (`DevScreenshots.{h,cpp}`, `docs/recipes.md`) sends mouse events to the lane via new `ControllerLaneArea::getPositionForCheck`. Undo was already one step per gesture (commit per drag), checked by two undos restoring the original.

## Validation
- `./build.sh --test`: All checks passed, no warnings (three -Wshadow warnings from my first test draft fixed). 4,498 PASS.
- Engine: also a playback check, `getSpanMessages` of a flat-then-rise redraw (8192 flat to beat 2, 16,000 at the end).
- Real app (`--screenshots` with `"controllerLanes": "Kick", "checkRedraw": true`, mouse events sent to the lane; scratch settings, no desktop control). Logged results:
  - flat 0.5 to 1.5 then rise: 1.25 and 1.5 = 8192, 2.0 = 12288;
  - backward flat 3.0 to 2.0: 2.25 and 2.75 = 4096, points ordered;
  - Shift line (1.0, 8192 to 3.0, 16383): 1.0 = 8192, 2.0 = 12288, 3.0 = 16383; one undo restored the bend as before;
  - Alt (draw at 8500): 8500 with Alt, 8192 without (centre snapping kept); one undo restored the bend;
  - pedal lane and notes unchanged throughout; two undos restored the original bend after the first two redraws.
  Screenshot checked by eye: `/tmp/s192/redraw.png` (not kept in the repo).
- Limits: the old code was not run against the new checks; the mouse events are sent in-process, not by a real pointer.
- First steps: checked, no change. Theory: none. Website: none. Dependencies and licence: none.
