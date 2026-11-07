# Keep the notebook beside the tracks with a top-bar button

Status: Released
Idea: [#162 — Keep the notebook beside the tracks with a top-bar button](https://github.com/geertvandamme/dawit/issues/162)

## Goal
Keep notes visible beside the tracks, with one top-bar button to show or hide them.

## Decisions for you
The requested implementation is approved. The button and Cmd+9 toggle the panel; track note entry points always open it at Track. The panel starts closed and keeps the last tab.

## What the user does and sees
A notebook icon in the transport bar opens or closes a panel on the right. View > Notebook and Cmd+9 do the same. Track Notes and track note icons open the Track tab. Closing saves pending App notes. The panel shares room with the inspector and arrangement, and its tabs fit a narrow panel. No song format or dependency changes.

## Steps
- [x] 1. Dock the existing Notebook component in MainComponent and wire all existing entry points.
- [x] 2. Add one top-bar icon, close control and responsive panel layout.
- [x] 3. Adapt screenshot support, check the app and run the build and tests.
- [x] 4. Update app documentation and issue progress.

## Tests
Run existing notebook and project persistence checks. Inspect screenshots of all three tabs and narrow layout in the real app.

## Website hand-off
The features page can describe the right-hand Notebook and top-bar button. Capture the main window with the panel open on a selected track. No dependency or guide change.

## Changes while building

The panel text was shortened after the narrow-window screenshot showed clipping. The Track tab keeps a short label; its description names the selected track.
