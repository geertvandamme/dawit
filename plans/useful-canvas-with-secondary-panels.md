# Preserve usable editing space when the inspector and Notebook are open

Status: Released
Idea: [#190 — Preserve usable editing space when the inspector and Notebook are open](https://github.com/geertvandamme/dawit/issues/190)
Umbrella: [usability-improvements.md](usability-improvements.md) (package 02 slice B, #175; audit finding 13).

## Goal
At 960 x 600 with the inspector and Notebook both open, the arrangement keeps room to work (today about 214 px of timeline, roughly two bars). When the window is too narrow for both panels, the layout gives way temporarily, always shows how to get the panel back, and returns to the user's chosen layout in a wide window.

## Decisions for you
1. **How the Notebook gives way.** Options: overlay; auto-collapse; dock both, narrower. **Recommended:** overlay.
   Answer (10 November 2026, after trying it): dock both, narrower. The overlay covered the timeline and was no better than closing the Notebook. Always dock, never float or hide.
2. **Minimum useful canvas.** Options: track header (270) plus 360 px of timeline = 630 px; or 270 + 480 = 750 px. **Recommended:** 630 px, which at 960 px leaves about 8 bars at the default zoom, and still lets 1280 x 800 dock both panels (230 + 4 + 360 leaves 686).
3. **Order of giving way.** Inspector to 200 px first, then the Notebook from 360 to 240 px; below that the tracks get less. **Answer:** as recommended.
4. **Restore route.** **Answer (10 November 2026):** nothing hides, so the lit top-bar button and the panel's X are all it needs, as before; no Esc handler, no extra line, no setting.
## What the user does and sees
- The inspector (`view.inspectorOpen`, `view.inspectorWidth`) and Notebook (View > Notebook, Cmd+9, top-bar icon) open exactly as now; the user's choices are never changed by the squeeze.
- The width budget is worked out on every resize. If the tracks would get under 630 px, the inspector shrinks first (to 200 px at the narrowest), then the Notebook (from 360 px to 240 px). Both stay docked; below that the tracks get less and nothing is hidden or floated.
- Widening the window re-docks the Notebook and gives the inspector its preferred width back, with no click.
- Drag-resizing the inspector edge saves the preferred width as today; a temporarily narrowed width is never saved.
- Tooltips: the Notebook button says "Show or hide the Notebook (Cmd+9)"; while overlaid, its panel's close button says "Close the Notebook. It covers the right of the tracks because the window is narrow." The panel keeps its keyboard focus route and an accessible name.

## Data and saving
None. No project data and no new settings keys. Existing `view.inspectorWidth` and `view.inspectorOpen` are only written by the user's own actions.

## Code
Follows the *a window* and *view state* recipes in docs/recipes.md and the layout rules in docs/ui-design.md (*The inspector*, *The note icon and the Notebook*).
- `MainComponent::resized()` uses a small pure width-budget function (`panelbudget::compute`: window width, preferred inspector width, inspector and Notebook open) returning the inspector, edge, Notebook and canvas widths. It is header-only in `src/engine` so the engine tests can call it.
- The Notebook component stays a child of `MainComponent` and is carved out of the layout as before.
- Invariants: preferred widths are only changed by user drags; no component gets a zero or negative width; at least the inspector minimum and the Notebook minimum (200 px) are honoured at 960 px; the transport bar and Add Track stay untouched.
- Risk: a window narrower than about 960 px is not supported (minimum window size), so the tracks never go below about 516 px with both panels open.

## Libraries and models
None.

## Theory
None.

## Steps
Group A (single group, `./build.sh --test` at step 3):
- [x] 1. Add the pure width-budget function and apply it in `MainComponent::resized()`: shrink the inspector first, then the Notebook; keep preferred widths untouched.
- [x] 2. (Dropped with the overlay.) The Notebook keeps its plain close button and tooltip.
- [x] 3. Tests for the budget (see Tests), then `./build.sh --test`; must end `All checks passed` with no warnings.

Group C, check and docs:
- [x] 4. Real-app check: `--screenshots` at 960 x 600, 1100 x 700 and 1280 x 800 with the Notebook open, looked at.
- [x] 5. Docs: README Features line under the layout bullet, docs/tests.md, docs/code-layout.md, docs/ui-design.md (the inspector and Notebook sections), GitHub issue #190 progress and acceptance checks.

## Tests
- Budget at 960: inspector 200, Notebook 240, canvas 516; at 1100 the Notebook shrinks only as far as needed; at 1280 and 1440 preferred widths; widths add up and never fall below their minimums.
- Budget at 1280 and 1440: preferred inspector width kept, Notebook docked at 360.
- Notebook not requested: inspector keeps its preferred width at every size.
- Preferred widths passed in come back unchanged; no result below the minimums or negative at 960.

## Website hand-off
Optional: a screenshot of the 960 px window with both panels open for the layout section; no library, guide or model.

## Left for later
Focus Editor, Reset Layout, Fit Song (package 02 slice B rest), workspace presets and secondary windows (slice C), the bottom panel and mixer budget.

## Changes while building

Built in one run (steps 1 to 5), then changed after the user tried it.
- **Change (10 November 2026):** the floating overlay was rejected (it docked only from about 1225 px, Esc failed when the text box lost focus, and at 960 px it left about 126 px of timeline). Replaced by "dock both, narrower": `panelbudget::compute` shrinks the inspector to 200 px, then the Notebook from 360 to 240 px; nothing floats. Removed `NotebookWindow::setOverlay`, the extra line, the overlap tooltip and the Esc handler.
- `src/engine/PanelBudget.h` (header only), used by `MainComponent::resized()`; tests in `runPanelBudgetTests`. `./build.sh --test` ended with All checks passed and no warnings.
- At 960 px the top-bar Notebook button is under *More...* (existing behaviour); the panel's X stays visible.
- Shots: `narrow.png` (960 x 600), `mid.png` (1100 x 700), `wide.png` (1280 x 800) in the scratchpad `shots` folder.
