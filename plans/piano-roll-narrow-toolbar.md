# Keep piano-roll tools and the editing target visible at narrow widths

Status: Released (11 November 2026); the keyboard checks by hand are still open on #188
Idea: [#188 — Keep piano-roll tools and the editing target visible at narrow widths](https://github.com/geertvandamme/dawit/issues/188)
Umbrella: [plans/usability-improvements.md](usability-improvements.md) (package #175); audit finding 12 in [docs/usability-review-2026-11-10.md](../docs/usability-review-2026-11-10.md) (also 22 and 24 in part).

## Goal
The piano-roll header always shows which track and clip you are editing and whether you are drawing or selecting. Every other tool stays reachable at 960x600, 1280x800 and 1440x900, with or without the inspector, Notebook and Generate panel: what does not fit moves into a "More" menu instead of being cut off or overlapping. No command, shortcut or song data changes.

## Decisions for you
1. How to make room. Options: (a) priority overflow: tools that do not fit move, lowest priority first, into a "More" menu; (b) a second header row; (c) a horizontally scrolling header. **Recommended:** (a), because the editor's height stays for notes, nothing is hidden behind a scroll, and the menu doubles as a keyboard route.
   Answer: Priority overflow into a keyboard-reachable "More ..." menu (user, 11 November 2026).
2. Add a visible *Draw* button next to *Select* (audit finding 22), so the two modes are a pair and the active one is highlighted? **Recommended:** yes, a small toggle pair; *Draw* is simply "Select off". Otherwise the pair is only Select, as now.
   Answer: Yes, a visible Draw button paired with Select.
3. Label the octave buttons "Clip" scope (finding 24) now? **Recommended:** no, leave to the later clipboard/numeric package; only keep their tooltip, and in the menu say "Transpose clip up an octave".
   Answer: No; leave the clip-scope relabel to the later clipboard and numeric-editing package. Only the More menu entry reads "Transpose clip up an octave".

## What the user does and sees
The header (left to right): Piano roll/Score switch, **Draw | Select** pair, the target label, then the tools, then zoom and close. The label is the existing title ("Keys - clip name - selection"); it keeps a minimum width (about 170 px) and shortens with an ellipsis, with the full text in its tooltip. It is never the thing that shrinks first.

Tools in priority order, last to move into the menu first: Quantise, Snap (label and box), Generate, Transform, Octave down/up, Velocity, Scale, Key chords, Rhythm guide, MIDI guide. Tools that fit keep their buttons as now; a "More" button (`...`, tooltip "More piano-roll tools") appears at the right of the tools only when something moved. Its menu lists each moved tool by name, with its toggle tick or current value, and runs the same action as its button (open Quantise dialog, Transform submenu, Velocity and Scale toggles, octave, Snap choices, Generate, guides). Draw/Select, the target label, zoom and close never move. Over a Generate panel the header keeps the same rules, as the panel sits below it; the grid keeps at least its current minimum. Score mode is unchanged.

Keyboard: the shortcuts (V, Q, arrows, Cmd+...) are untouched. The More button is focusable and opens with Space/Enter. Every menu item carries the tool's tooltip text where short.

Sizes to look right: 960x600 (minimum; inspector and Notebook docked narrow, so the piano roll is about 400-500 px wide: expect Draw/Select, label and More only), 1280x800 (most tools), 1440x900 (all or nearly all). No remembered settings.

## Data and saving
None. Tool state (Velocity, Select, Scale) lives where it does now.

## Code
Follows the *A UI component* recipes in [docs/recipes.md](../docs/recipes.md); no new recipe. A small pure helper (a new header beside `NoteMenuText.h`, say `PianoRollToolbar.h`) takes the available width and an ordered list of tools with their wanted widths and gaps, and returns which stay and which overflow; `PianoRoll::resized()` uses it in place of the fixed `removeFromRight` chain, hides overflowed buttons (`setVisible (false)`, so they are neither drawn nor focusable) and shows the More button. The menu is built from the same members the buttons use, calling their existing `onClick` lambdas so semantics cannot drift. Invariants: the label gets at least its minimum; no overlap between label and tools at any width; at the very narrowest, everything but the fixed items is in the menu.
Coexistence with the uncommitted edits from [piano-roll-note-context-menu.md](piano-roll-note-context-menu.md) (`PianoRoll.cpp/.h`, `NoteMenuText.h`): build on them as they are and revert or tidy nothing. They touch the grid's right-click path (`selectForMenu`, `showNoteMenu`, `onQuantiseDialog`, `onShowMidiGuide`) and `selectAll`; this plan touches `resized()` and the header constructor block only. Reuse `onQuantiseDialog` and `onShowMidiGuide` for the menu entries rather than duplicating. If both land in one commit, nothing conflicts; if not, this one rebases trivially.

## Libraries and models
None.

## Steps
- [x] 1. Add the pure layout helper (priority order, widths, gaps, minimum label width) in its own header.
- [x] 2. Add the Draw button, wired to the Select mode flag (Decision 2), and give the label its minimum width, ellipsis and tooltip.
- [x] 3. Replace the fixed chain in `PianoRoll::resized()` with the helper; add the More button and menu built from the existing button actions, with tooltips and keyboard access.
- [x] 4. Tests, then the real-app check, then README, docs and the issue.

## Tests
In `tests/EngineTests.cpp` (a `runPianoRollToolbarTests()`): at 1440 px everything stays; at 1000 px lower-priority tools overflow while Quantise and Snap stay; at 420 px only the fixed items remain; the label keeps at least its minimum at every width from 300 to 1600; the sum of kept widths plus gaps never exceeds the space; overflow is monotonic (a tool that moved at a wider width stays moved at a narrower one).

Real-app check: shot list with `--screenshots`, scale 1, capture `"bottomPanel"` or the piano roll's area, a MIDI clip selected (Keys), in `"window"` sizes `[960, 600]`, `[1280, 800]` and `[1440, 900]`; extra shots at 960x600 and 1280x800 with Generate open (`"generatePanel": true`) and with the Notebook open. Look for: label and mode pair visible, no overlap, More present only when needed. If a shot cannot open the More menu, add a `ScreenshotRunner` setting (no patching `Main.cpp`) and shoot it open once. Then by hand: each moved tool works from the menu, Tab reaches More, shortcuts still work.

## Docs and issue notes
- README: extend the piano-roll bullet under *Features* (header keeps Draw/Select and the target; extra tools collapse into a More menu).
- `docs/code-layout.md`: the new header and the changed `PianoRoll` line; `docs/tests.md`: the new test area; `docs/ui-design.md`: one line in the toolbar/overflow rule.
- Issue #188 (per `docs/github-ideas.md`): link this plan and status, tick the done items, list what stays open (octave scope wording, Scale split, clipboard package), keep it open for the coordinator; set project status In Progress at the build start.

## Website hand-off
Screenshots at 960x600 and 1280x800 of the piano roll header, and one with the More menu open.

## Changes while building
- Decisions: 1 priority overflow, 2 Draw button added, 3 no octave relabel (menu reads "Transpose clip up/down an octave"), as approved.
- Added: below 560 px of header the zoom buttons also move into the More menu (otherwise the label shrank to "K..." at 960x600 with the Notebook and Generate open). `countKept` has an `alwaysMore` flag for that.
- Select/Draw are a radio pair; Tool enumerators are prefixed `tool` (a plain `quantise` clashed with `daw::quantise`).
- Hand-off: the real-app check ran at 960, 1280 and 1440 wide (inspector, and Notebook plus Generate); the More menu was not opened by a shot and the Tab/Space route was not exercised by hand, so a person should check those (menu items call each button's own onClick; Transform opens anchored on More). Shots in the session scratchpad `shots/pr-<width>-*.png`. Uncommitted context-menu work was left intact.
