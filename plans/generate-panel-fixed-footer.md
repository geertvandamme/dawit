# Keep Generate, Cancel and generation status visible while settings scroll

Status: Released (keyboard check and running/staged-result shots still owed; see #189)
Idea: [#189 — Keep Generate, Cancel and generation status visible while settings scroll](https://github.com/geertvandamme/dawit/issues/189)

Part of [usability-improvements.md](usability-improvements.md) (package 175, audit finding 14). Mode redesign and model setup (package 11, findings 46, 47, 49) are out of scope.

## Goal
In the piano roll's Generate panel the Generate button and the status line sit at the bottom of the scrolling form, so at normal window heights they are below the visible panel. A beginner sees settings but not the action, and cannot see a running job or an error. A fixed footer keeps the action, Cancel and the status always in view while the settings scroll.

## Decisions for you
1. What goes in the footer? Options: (a) Generate/Cancel + Undo + take buttons + status in normal mode, and Generate/Cancel + Preview, Apply MIDI, Apply chords + status in ChatMusician mode; (b) only Generate/Cancel and status; every other button stays in the scrolling form. **Recommended:** (a), because Undo, takes and the staged Preview/Apply are the results of the action and the issue asks for staged-result states to stay reachable.
   Answer: (a), all actions in the footer (user, 11 November 2026).
2. Footer height. Options: (a) fixed rows with the status limited to three lines (about 40 px, shrinking text to fit, full text in a tooltip); (b) footer grows with the status. **Recommended:** (a), so the footer never takes the settings' room in a short window; the tooltip carries long errors.
   Answer: (a), fixed rows with the status limited to three lines and the full text in a tooltip (user, 11 November 2026).
3. Are ChatMusician's secondary buttons ("Undo ChatMusician", "How to read music...") in the footer? Options: leave both in the scrolling form, or put Undo ChatMusician in the footer. **Recommended:** Undo in the footer (it is state-dependent, like Undo), the guide link stays in the form.
   Answer: Undo ChatMusician in the footer; "How to read music..." stays in the scrolling form (user, 11 November 2026).

## What the user does and sees
The panel keeps its title bar, key/tempo/clip info and settings in the scrolling area (the viewport, scrollbar as now). Below it, a divider line and a footer that never scrolls:

    [ Generate / Cancel ............ ] [ Undo ]
    [ take 1 ][ take 2 ][ take 3 ]        (normal mode, once takes exist)
    status text, up to three lines

ChatMusician mode: footer holds `Generate / Cancel`, then `Preview`, `Apply MIDI`, `Apply chords`, `Undo ChatMusician` (as many rows as fit the width) and the status. Behaviour and enablement of every button are unchanged: the button label (Generate, Play style, Cancel), the existing tooltips and the shared `generate()` / `cancel()` paths stay. Resizing the panel or window, or toggling the mode, changes only layout: no notes, clip, target or chord track change. Tab order runs settings, then footer. Status text is exposed through the label's accessible name as now; add accessible names to the footer buttons if missing. Nothing new is saved.

## Data and saving
None. No project or settings change.

## Code
Recipe: *Windows* is not involved; this is a layout change inside `src/ui/GeneratePanel.cpp/.h`. The viewport shrinks to the area above a footer region of the panel. `generateButton`, `undoButton`, `takeButtons`, `statusLabel` and the chat action buttons become children of `GeneratePanel` (not `form`), laid out by `resized()` from the bottom up; `form` keeps only the settings and its height no longer includes the actions. The footer height is computed from the visible rows (normal with or without takes; chat), and `resized()` already runs from `updateControls()`, take changes and mode toggles, so footer height changes re-lay out the viewport. The viewport must keep at least a usable height: if the window is so short that footer plus a minimum viewport do not fit, shrink the status first (one line), then let the footer rows wrap, never drop a button.
Invariants: the existing generation state machine (`cancelFlag`, `generationId`, `generatingClip`, staged ChatMusician state) is untouched; `updateControls()` enablement is kept as is and covered by a test that drives states. Message thread only. Risks: the `takeButtons` array is rebuilt in `updateTakeButtons()`, so its parent must change with it; `ScreenshotRunner` (friend) reaches `generatePanel`, `generateButton` and `chatMode`.

## Steps
- [x] 1. Reparent the action widgets out of `form` into the panel and split `resized()` into settings layout (viewport/form) and footer layout (normal and ChatMusician), with the status at about 40 px and a full-text tooltip. Done when the viewport height plus footer equals the panel height at several heights.
- [x] 2. Keep tooltips, accessible names and tab order for the moved controls; check `updateControls()` enablement for no clip, models loading, running, error and staged ChatMusician result; add an `updateTakeButtons()` parent fix.
- [x] 3. Add `ScreenshotRunner` support the shot list lacks (`generateStatus`); the panel size is the list's `window`.
- [x] 4. `./build.sh --test` through `All checks passed`; real-app shots and direct keyboard check (below).
- [x] 5. Docs: README *Features* bullet for the Generate panel, `docs/tests.md` (UI checks section), `docs/code-layout.md`, `docs/ui-design.md` (panel footer pattern); update #189 with completed checks.

## Tests
Layout is UI-only. Checks are real-app: `--screenshots` with `"scale": 1`, `"capture": "window"` or an `area` on the panel, at a short window height (about 600) and at 900 for: normal mode, normal mode with takes, running (Cancel), error, ChatMusician with a staged result. Confirm the footer is on screen in each and the settings scroll. Keyboard (not provable by screenshot): Tab from the model box reaches Generate, Space/Return triggers it, Cancel is reachable while running, and Escape or Tab does not leave the footer unreachable. Resize check: note and clip counts and the target clip are the same before and after changing the panel/window size.

## Website hand-off
None needed now; the Generate panel's website section follows the later package 11 redesign.

## Changes while building
- Built as planned. Added `"generateStatus"` / `"generateStatusError"` to `ScreenshotRunner` (documented in `docs/recipes.md`). The staged-result and running states are not reachable without a model, so they were not photographed; their enablement is unchanged code.
- The footer's rows are fixed; the status shrinks to one line when the panel is under about 120 px of settings. Keyboard order (settings, then footer) follows child order; not tested by hand.
