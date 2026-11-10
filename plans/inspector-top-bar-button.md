# A top-bar button to show or hide the track inspector

Status: Released
Idea: [#172 — A top-bar button to show or hide the track inspector](https://github.com/geertvandamme/dawit/issues/172)
Related: [#187 — Keep transport and track actions reachable in a narrow window](https://github.com/geertvandamme/dawit/issues/187), [usability package 02](https://github.com/geertvandamme/dawit/issues/175).

## Goal

Make the inspector easy to discover and restore from the top bar while keeping transport and track actions reachable at supported window sizes.

## Decisions for you

1. **Narrow bar:** implement the transport overflow in #187 with this button, or defer this button until #187 is built. **Recommended:** implement the bounded #187 transport overflow together, so adding the button does not make the existing narrow-width problem worse. Keep both issues' acceptance checks and record their individual progress.
   Answer: Build the inspector toggle and the bounded #187 overflow work together.

## What the user does and sees

A left-column panel icon beside the Notebook button toggles the inspector; its tooltip says “Show or hide the inspector (I)”. Its highlighted state matches the inspector after every entry point, including View > Inspector, I, the panel close button and its menu. The existing `view.inspectorOpen` setting remains the preference and is restored on startup.

At 960 × 600, 1280 × 800 and 1440 × 900, give controls enough room; place displaced actions in an explicit, keyboard-accessible overflow menu with meaningful names and correct enabled and toggled states. Playback, Record and Add Track retain a visible route. Inspector and Notebook remain reachable, including when both panels are open. Reuse existing callbacks and commands; do not duplicate their behaviour. Preserve existing shortcuts and song data.

## Data and saving

No song data or project format changes. The existing app setting remains authoritative; older projects are unaffected.

## The guide

None: this control names an existing view and teaches no theory.

## Code

Follow [A command or menu item](../docs/recipes.md#a-command-or-menu-item) and the UI design guidance. Use the existing inspector command and state as the single source of truth; initialise the button from the restored preference and update it on all changes. Use the Notebook toggle as the visual precedent. Keep overflow state and action dispatch aligned with the existing controls. Layout and callbacks operate on the message thread; no audio-thread or engine changes are needed. The fixed-width transport layout currently leaves the track buttons with zero width at narrow sizes, so check all controls rather than just the new icon.

## Libraries and models

None proposed. If implementation changes a dependency, follow the exact-version, transitive licence, inventory, notices and distribution procedure in `docs/architecture.md` before completion.

## Theory

None.

## Steps and implementation groups

### Group B: UI

- [ ] 1. In `TransportBar` and `MainComponent`, add the inspector glyph and accessible, tooltip-labelled toggle using the existing inspector command and restored state. Check all existing entry points keep its state in sync.
- [ ] 2. Adapt the transport layout and provide explicit overflow for displaced actions, with existing callbacks, enabled and toggle states; preserve visible playback, Record and Add Track routes. Cover supported widths and keyboard access, including Inspector and Notebook.
- [ ] 3. Add meaningful automated checks for layout or state where practical and permanent `ScreenshotRunner` support for missing capture states. Run `./build.sh --test` to `All checks passed`; assess warnings under project policy.

### Group C: real-app check and docs

- [ ] 4. Capture the example song at 960 × 600, 1280 × 800 and 1440 × 900, with inspector open and closed and Notebook open; inspect the bar and overflow. Directly exercise button, View, I, panel close/menu, overflow and keyboard access; verify restart restores both inspector states using isolated settings.
- [ ] 5. Update README Features, `docs/ui-design.md`, `docs/tests.md` and `docs/code-layout.md`; report progress and verification on #172 and #187, preserving any unfinished scope. Run `./build.sh --test` to `All checks passed` after changes.

## Tests

Check that all actions retain nonzero reachable targets at the three widths, overflow reflects enabled and toggled state, and each entry point changes the same inspector state. Verify persistence and keyboard interaction in the real app; screenshots alone do not establish either.

## Website hand-off

Consider a brief mention in the features page's workspace description and a main-window screenshot showing the inspector and its top-bar button. No guide or dependency is proposed. Website changes are separate after commit.

## Left for later

Other package 02 workspace changes remain in their own issues. Any #187 acceptance item not completed remains open and is reported explicitly.

## Changes while building

The inspector toggle and compact transport overflow are implemented. `./build.sh --test` passed after rerunning with access to the test cache. Scratch-settings captures at 960 × 600, 1280 × 800 and 1440 × 900 show the open and closed inspector indicator; the Notebook is open in the full-window captures. The user validated the feature and reported that it works. Specific direct keyboard, menu and restart checks were not independently verified because computer-use permission was unavailable; no new automated interaction checks were added. Remaining verification is tracked under #187.
