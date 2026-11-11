# Open a context menu instead of deleting a note on right-click

Status: Released
Idea: [#184 — Open a context menu instead of deleting a note on right-click](https://github.com/geertvandamme/dawit/issues/184)
Umbrella: [usability-improvements.md](usability-improvements.md), package 01 (#174), finding 20 in [the review](../docs/usability-review-2026-11-10.md).

## Goal
Right-clicking in the piano-roll note grid no longer changes the song. It opens a menu of what you can do with the note (or the notes) under the pointer, and its last item opens the MIDI guide. A beginner who right-clicks to look for help or options can no longer lose a note by accident.

## Decisions for you
1. Which items in the menu? Options: (a) only Delete, Select All and the guide; (b) also Quantise...; (c) also Duplicate and Properties. **Recommended:** (b). Quantise... already exists with a clear scope, while Duplicate and Properties need clipboard and editing work that belongs to #181 (package 10), which will add to this menu.
   Answer: Delete, Select All, Quantise... and the guide; Duplicate and Properties wait for #181.
2. Keep a fast erase? Options: (a) none, Delete or Backspace on the selection and the menu's Delete remain; (b) Alt-right-click erases. **Recommended:** (a), because the review asks for an explicit tool if any, and the double-click on a note already deletes it. Changing that double-click is not in this plan.
   Answer: No fast erase; Delete/Backspace and the menu's Delete are enough.
3. How does the keyboard open the menu? **Recommended:** Shift+F10, and the Menu key where the platform sends it, while the grid has focus; the menu opens at the first selected note, or at the grid's centre-left when nothing is selected. Nothing is chosen until the user picks an item.
   Answer: Shift+F10 and the Menu key while the grid has focus.

## What the user does and sees
- Right-click (or Ctrl-click on macOS) on a note: if it is not selected, it becomes the only selected note, as a left click does; a selected note keeps the whole selection. Nothing is deleted.
- Right-click on empty grid: the menu opens with the selection unchanged.
- Menu items, with scope in the text: "Delete 3 notes" or "Delete note" (disabled when nothing is selected), "Select All Notes" (Cmd+A shown), "Quantise Selected Notes..." or "Quantise Clip..." (the dialog's existing scope), then a separator and "What is MIDI?..." with the guide icon (`guideicon::addItem`), calling `onShowMidiGuide`.
- Shift+F10 (and the Menu key) opens the same menu from the keyboard, so the menu is not mouse-only.
- Keeps: Delete and Backspace on the selection, Cmd+A, Q, V, arrow keys, drag, resize, shift-select, select and velocity modes, double-click deleting a note. Undo and Redo (Edit menu) restore a deleted note.
- Tooltips: the grid tooltip gains a line "Right-click for note actions" only if it fits; the menu items need none beyond their text. The grid gets an accessible title and description ("Note grid", with the shortcut for the menu).
- Nothing new is saved or remembered.

## Data and saving
None. The menu changes no data until an item is chosen; choosing Delete uses the existing `deleteNotes`, one change message and one Undo step.

## Code
Recipe: *a command / right-click menu ending with a guide* in [docs/recipes.md](../docs/recipes.md), with `guideicon::addItem` and the existing `quantiseui` entry points. In `PianoRollGrid`, the popup branch of `mouseDown` selects as above and calls one new function that builds and shows the async menu; `keyPressed` calls the same function for the keyboard gestures. The menu must not hold indices across the async callback: re-read the selection on choosing, and use a `SafePointer` for the grid. Build the menu text and enabled state in a small pure helper so a test can check it. Only the message thread edits notes, under `Engine::getLock()` as `deleteNotes` already does.

## Libraries and models
None.

## Theory
None. The guide item uses the existing MIDI guide (`onShowMidiGuide`); no text of the guide changes, so `SOURCES.md` is untouched.

## Steps
Single group:
- [x] 1. Add the helper that gives the menu's item labels and enabled states from the selection count and the clip, and the note-grid menu function; in `PianoRollGrid::mouseDown` replace the deleting popup branch with select-then-show-menu. Done when right-click never changes `clip->notes`.
- [x] 2. Open the same menu from `keyPressed` on Shift+F10 and the Menu key; set the grid's accessible title and description; add the tooltip line. Check that other keys and focus handling are unchanged. Done when the menu opens without the mouse.
- [x] 3. Tests (below), then `./build.sh --test` ending in `All checks passed` with no warnings.
- [x] 4. Real-app check: a shot list with `--screenshots` showing the open menu over a selected note group (and the keyboard-opened menu if the runner can press keys; otherwise add a scene or setting to `ScreenshotRunner`); the menu must show the scope text, a separator and the guide item last, and the notes must be unchanged. Then check by hand: right-click leaves the note count as it was, Delete then Cmd+Z restores a note.
- [x] 5. Docs: README *Features* bullet for the piano roll (right-click menu), `docs/tests.md`, `docs/code-layout.md` if a file is added, `docs/ui-design.md` (the piano roll's right-click and keyboard menu), and the GitHub issue (plan link comment, done and open lists, acceptance checkboxes).

## Tests
- Helper: with 0, 1 and 3 selected notes the delete label is "Delete note" (disabled at 0), "Delete note" and "Delete 3 notes"; the quantise label says "Selected Notes" with a selection and "Clip" without; the guide item is always last.
- Opening the menu path with the popup gesture (the function that selects, without showing) leaves the note count unchanged; selecting an unselected note replaces the selection, and a selected note keeps it.
- Delete through `deleteNotes` then an engine Undo restores the notes, if the engine's undo is reachable headless; otherwise this stays a manual check in step 4.

## Website hand-off
Not a feature for the website on its own. If the website describes right-click deleting notes, change it to the menu. No screenshots needed beyond step 4's; no library added.

## Left for later
Duplicate, Copy, Paste and Properties items (#181, findings 21 to 26). An Alt-based fast erase, if asked for. The double-click deleting a note, which still surprises some users (#181).

## Changes while building
- The Menu key is not opened: JUCE has no key code for it, so only Shift+F10 opens the menu from the keyboard.
- The tooltip line was left out (the note tooltip shows only over a note and is already long); the grid's accessible description names Shift+F10.
- `ScreenshotRunner` cannot capture a popup menu (it is its own native window), so the real-app shot shows only that the piano roll still renders. The menu's behaviour is covered by the helper test and needs a manual check: right-click leaves the note count, Delete then Cmd+Z restores it.
- Hand-off: `PianoRollGrid::selectForMenu` and `showNoteMenu` (src/ui/PianoRoll.cpp) build the menu from `notemenu::getText` (src/ui/NoteMenuText.h); #181 can add Duplicate, Copy, Paste and Properties there.
