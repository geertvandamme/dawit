# Keep transport and track actions reachable in a narrow window: verification

Status: Released
Idea: [#187 — Keep transport and track actions reachable in a narrow window](https://github.com/geertvandamme/dawit/issues/187)
Related: [plans/inspector-top-bar-button.md](inspector-top-bar-button.md) (built and released the overflow with #172; not rebuilt here)

## Goal

Close #187's open acceptance checks as far as possible without a person at the keyboard: show by an automated check that at the three supported window sizes every transport and track action keeps a usable route, and that the More... menu is complete and mirrors the buttons.

## Decisions for you

1. **How far to go.** Answer: *Verify and close* (the user's choice): add checks and fix defects, change no design.

## What the user does and sees

Nothing new. Below 1550 pt the bar shows a More... button holding Undo, Redo, Go to start, Loop, Metronome (and options), Computer keyboard (and guide), Notebook, the four Add buttons and Mixer. Play, Stop, Record and the Inspector button always stay on the bar.

## Steps

- [x] 1. Make the menu testable: `TransportBar` gets `getActions`, `getOverflowItems`, `activateOverflowItem` and `refreshButtonStates`; the menu and its dispatch use them, so the check tests the real list.
- [x] 2. Add the shot key `checkTransport` to `ScreenshotRunner` (permanent): at 960 x 600, 1280 x 800 and 1440 x 900 no visible control has zero size, every action has its button or its menu item, items mirror enabled and toggled state, items run (Loop, Metronome, Add MIDI track, Undo), and Return on More... opens the menu.
- [x] 3. Run it in the real app; fix any defect; update `docs/tests.md` and `docs/code-layout.md`; run `./build.sh --test`.

## Tests

The check lives in the real app (the bar is UI, outside `DawItTests`): `--screenshots` with `"checkTransport": true`, which fails the run (exit 1) on a problem. Not covered: arrow-key movement inside the open menu (JUCE's own), real keyboard focus order, and the inspector menu/close/restart paths of #172.

## Website hand-off

None.

## Changes while building

- The menu items now act at once (`activateOverflowItem` presses the button's handler; `triggerClick` is delivered on the next message). Same result for the user.
- No defect found in the layout: all three sizes pass. The undo step is made by the main window's timer, so the menu's Undo enables a moment after an edit (as the button does).
- Keyboard: Return on More... opens the menu (checked in two shots, because it opens on the next message). Still unverified without a person: real focus order, arrow keys inside the menu, the inspector menu/close/restart paths of #172.
