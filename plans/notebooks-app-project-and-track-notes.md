# Notebooks: app, project and track notes

Status: Released
Idea: [#95 — Notebooks: app, project and track notes](https://github.com/geertvandamme/dawit/issues/95)

## Goal
Somewhere to write things down while making music, at three levels: your own notebook, the same in every song; notes
about this song, saved in the `.dawit` file; and notes about one track, saved with that track. A beginner learns faster
by writing down what worked ("ratio 3:1 on the vocal"), what to try next and what still needs fixing, next to the song.

## Decisions for you
1. **Where the App notebook is kept.** Options: a plain UTF-8 file `Notebook.txt` next to the settings file
   (`~/Library/Application Support/DawIt/`); or a key in the app settings. **Recommended:** the file, because it can be
   long, survives a settings reset, can be opened or backed up outside DawIt, and follows the presets folder's pattern
   (with the same kind of override for tests and the shot runner).
   Answer:
2. **Undo.** Options: keep notes out of the undo history (typing has the text box's own Cmd+Z); or put them in
   `undo::Snapshot` like the lyrics brief. **Recommended:** out of the history, because the history commits at every
   quiet moment (5 Hz), so typing a paragraph would make dozens of steps and push real edits past the 100-step limit.
   Undoing a track's deletion still brings its notes back (undo restores the same `Track` object).
   Answer:
3. **Tab names.** Options: *App · Project · Track* (as in the issue); *All songs · This song · This track*.
   **Recommended:** *App · Project · Track*, with the line under the tabs saying in plain words what each holds and
   where it's saved, and the Track tab named after the track ("Track: Bass").
   Answer:
4. **Format version.** Options: no bump (an older DawIt opens the project and ignores the notes, dropping them if it
   saves); or bump to 5 so an older DawIt refuses it. **Recommended:** no bump, as with the lyrics brief: notes make no
   sound, and refusing a whole song over them helps nobody.
   Answer:
5. **Window or panel.** Options: a separate resizable window; a docked panel. **Recommended:** a window
   (*View > Notebook...*, Cmd+9, the first free number), so it can sit beside the arrangement or on a second screen
   without taking room from the tracks.
   Answer:

**Answers (2026-11-07):** all recommendations taken: 1. `Notebook.txt` next to the settings file; 2. notes kept out of the undo history; 3. tabs *App · Project · Track*; 4. no format version bump; 5. a separate resizable window (Cmd+9). Build now.

## What the user does and sees
**Opening it:** *View > Notebook...* (Cmd+9); *Track Notes...* in the track right-click menu (headers, mixer, device
bar: shared in `TrackActions`), which selects that track and opens the Track tab; a click on a track's note icon does
the same. Opening again brings the window to the front at the asked tab.

**The window** (resizable, default 560 × 480, minimum 420 × 320, `theme::panel`):
```
[ App ][ Project ][ Track: Bass ]                    (radio toggle buttons, 28 px, accent when chosen)
Notes about Bass: how it was recorded, why these...  (13 pt, textDim, one or two lines)
+-------------------------------------------------+
| multi-line text box, 15 pt, panelLight,          |
| word-wrapped, scrolls; placeholder when empty    |
+-------------------------------------------------+
```
The line under the tabs (final text):
- App: "Your own notes, the same in every song: things you learned, settings that worked, ideas to try. Kept on this computer, not in the project."
- Project: "Notes about this song: what it's about, the plan, what still needs doing, its versions. Saved in the .dawit file when you save the project."
- Track: "Notes about <name>: how it was recorded, why these settings, which take is best. Saved with the track in the project."
- Track, nothing selected: "Select a track to write notes about it." (the text box is disabled and empty).

Placeholders (shown when empty, `textDim`):
- App: "For example: \"Ratio 3:1 on the vocal compressor sounded natural.\" \"Try Am F C G in the next song.\" A to-do list."
- Project: "For example: what the song is about, the plan (verse, chorus, verse, chorus, bridge, chorus), what to fix next, \"v3: new bridge, louder vocal\"."
- Track: "For example: \"Recorded with the microphone 20 cm from the guitar.\" \"Take 3 has the best chorus.\" \"Fix: the snare is late in bar 12.\""

**Typing:** plain text; Return starts a new line, Tab is typed rather than moving focus. A focused text box keeps
Space, R, K, Return and the computer-keyboard letters for its own text (ui-design.md *Interaction*), and Cmd+Z/Shift+Z
for its own typing. The right-click menu is JUCE's text menu (cut, copy, paste, select all); there is no guide to end
it with, since the notebook has no theory. The Track tab follows `Selection::getTrack()` (the master counts as no
track); switching track, tab, opening or loading a project, New, or an undo that removes the shown track refreshes it.

**The note icon:** a 12 × 12 px page with three lines in `textDim` (`text` on hover), shown only on a track that has
notes: in the track header's top row left of the name (the name gives up 14 px), on the mixer strip next to the name,
and in the inspector right of the name. Tooltip: "Notes: <first line> · Click to open them in the Notebook". Click: select
the track and open the Notebook at its Track tab. One painter shared by all three.

**Tooltips:**
- App tab: "Your own notebook, the same in every song (kept on this computer)".
- Project tab: "Notes about this song (saved in the project)".
- Track tab: "Notes about the selected track (saved with the track in the project)".
- Text box: "Type your notes. Cmd+Z undoes typing. The App notebook is kept as you type; project and track notes are saved when you save the project."
- Command: "Notebook..." / "Write notes for yourself, about this song or about one track". Track menu: "Track Notes...".

**Remembered between sessions:** the last tab in `view.notebookTab` (app, project, track; default project).

## Data and saving
- **Project notes:** one string on `Engine` (song data: getter, setter that sends a change only when it changed,
  cleared in `resetEdit`). Saved as a top-level `"projectNotes"` string.
- **Track notes:** one string per track (audio, MIDI, bus, return), changed through an `Engine` setter that sends a
  change. Saved as `"trackNotes"` in the track's object. The key names avoid plain "notes", which clips already use
  for MIDI notes; code names should avoid it too.
- Both are saved exactly as typed (line breaks as `\n`, CRLF read as LF) and left out when they hold no visible
  characters. No format version bump (decision 4): a song with only notes still saves as version 1; an older project
  without the keys loads with empty notes.
- The unsaved-changes check compares `project::toText`, so typing notes marks the project edited by itself.
- **The App notebook:** `Notebook.txt` (UTF-8) next to the settings file, read when the window opens, written through
  a temporary file 1 s after typing stops, when the window closes and when the app quits. A missing file is an empty
  notebook; a failed write shows "Couldn't save the notebook to <path>" in the line under the tabs. Tests and
  `ScreenshotRunner` point it at a scratch folder through an override, like `presets::overridePresetsFolder`.

## The guide
None: the notebook has no theory. It explains itself through the line under each tab, the placeholders and tooltips.

## Code
- Follows recipes.md *Song data that's saved* (copy the lyrics brief: `Engine::setLyricsBrief`, its `ProjectFile.cpp`
  keys, `resetEdit`) except step 6: notes stay out of `undo::Snapshot` (decision 2), and out of `matches` and `equal`,
  so typing never records a step. *A command or menu item* for View > Notebook... and *A window* for the window
  (`PluginWindowManager::show`, one key, a `minimumSize`, a `SafePointer` in `MainComponent`). The track menu item and
  its callback go in `TrackActions`, like `onShowSends`.
- New: `daw::notebook` (`src/engine/Notebook.{h,cpp}`, no UI): the App notebook's file, load and atomic save, the folder
  override, and pure helpers: *has notes* (any non-whitespace), *first line* (first non-blank line, trimmed, cut to
  80 characters with "..."), and line-ending normalising. `NotebookWindow` (`src/ui`): tabs, text box, refresh rules.
  A shared note-icon painter for header, strip and inspector.
- **Rules:** notes are written to the engine on every text change, so Save and the unsaved-changes check always see
  them; the window reloads its text box from the engine only when what it shows changes (tab, track, edit generation,
  or the engine's text differs from the box's), never in reply to its own write, so the caret doesn't jump. The window
  never keeps a `Track*` the edit no longer holds: on each engine change it checks the shown track is still in
  `getTracks()`. Generating, bouncing in place, stems and templates make tracks without notes.
- **Threading:** notes are message-thread data; the setters take `getLock()` like other song data; the audio thread
  never reads them. The App notebook file is written on the message thread (small text, temporary file).
- **Risks:** small. A per-keystroke `sendChangeMessage` repaints listeners (coalesced, as the lyrics brief does today);
  the icon's visibility must follow it.

## Libraries and models
None.

## Theory
None.

## Steps
Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `src/engine/Notebook.{h,cpp}` in both CMake targets: the helpers, the file location beside the settings file,
  load, atomic save, the override. Tests: the helper and file checks below. Done when they pass.
- [x] 2. Project notes on `Engine` and track notes on `Track` with an `Engine` setter; `resetEdit` clears the project
  notes; `ProjectFile.cpp` writes `projectNotes` and `trackNotes` (left out when blank) and reads them with empty
  defaults. Tests: save-and-load, older project, version, unsaved text. Check: a saved test project holds the keys.
- [x] 3. Undo invariants: setting notes records no step; deleting a track with notes then Undo brings them back.
  Done when `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 4. `src/ui/NotebookWindow.{h,cpp}`: tabs, the line under them, text box, placeholders, tooltips, refresh rules,
  the App notebook's 1 s save, save on close and on quit, `view.notebookTab`. Check: type in each tab, reopen the window.
- [x] 5. *View > Notebook...* (Cmd+9) in `MainComponent`, opening at a given tab; *Track Notes...* in the track menu
  through a `TrackActions` callback that selects the track and opens the Track tab.
- [x] 6. The note icon in `TrackHeader`, `ChannelStrip` and `InspectorPanel` with its tooltip and click; shown and
  hidden as notes come and go. Check: type a note on Bass, the icon appears in all three.
- [x] 7. `ScreenshotRunner`: a `notebook` key (app, project or track) that opens the window at that tab, and a
  `notebookText` object setting the App, project and named tracks' notes before the shot (App into the scratch
  notebook folder). Done when `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 8. Real-app check, one `--screenshots` run on the pop song: (a) notes set on Bass and the project, arrangement
  with Bass selected: the icon left of "Bass" and the name still fits; (b) `panel: mixer`, `capture: bottomPanel`: the
  icon on Bass's strip; (c) `notebook: track`, `capture: window`: "Track: Bass", the line under the tabs, the text;
  (d) `notebook: project`; (e) `notebook: app` with empty text: the placeholder, not cut off; (f) inspector open on
  Bass: the icon beside the name. Look for clipped text and overlap.
- [x] 9. Docs: README bullet under *Learn by making music* (the "Take what you learn..." area: the notebook, where it
  opens, what each tab keeps and where); docs/tests.md (a Notebook area); docs/code-layout.md (`Notebook.{h,cpp}`,
  `NotebookWindow.{h,cpp}`); docs/architecture.md *Saving* (project and track notes, `Notebook.txt`, kept out of undo:
  add to the "Not in a snapshot" list); docs/ui-design.md (the note icon); recipes.md *Checking in the real app* (the
  `notebook` and `notebookText` keys); issue #95 progress comment, keeping the *Later* items open.

## Tests
In `tests/EngineTests.cpp`, a `runNotebookTests()`:
- *Has notes*: "" and " \n\t " are false; "x" is true.
- *First line*: "\n\n  Take 3 is best  \nmore" gives "Take 3 is best"; a 100-character line gives 80 characters
  plus "..."; "" gives "".
- Line endings: "a\r\nb\rc" becomes "a\nb\nc".
- App notebook file (override to a temporary folder): missing file loads ""; save then load returns the same text,
  including "café · ♪" and 10,000 characters; the save leaves no temporary file behind.
- Project round trip: project notes "Plan:\nverse, chorus\n\"v3\"" and Bass's track notes "Take 3 has the best chorus"
  survive save and load exactly; a bus and a return keep theirs too.
- Blank notes ("  \n") are not written: the text has no `projectNotes` or `trackNotes` key.
- A project text without the keys (as saved before this feature) loads with empty notes.
- A 4/4 song with no buses but with notes saves as version 1.
- `project::toText` differs after setting project notes and after setting a track's notes (the unsaved-changes check).
- The project-notes setter sends no change when given the same text; `resetEdit` empties the project notes.
- Undo: after `commitHistory`, setting notes and calling `commitHistory` again records no step; deleting a track with
  notes, then Undo, restores the track with the same notes.

## Website hand-off
- `features.html`, *Work & export*: a short item on the Notebook (your own notes in every song, song notes, track notes
  with the icon on the track).
- Not for Home or `learn.html`. Screenshot worth taking: shot (c) beside the arrangement with the Bass icon visible.
- No library, so `about.html` is unchanged.

## Left for later
- Notes pinned to a moment (a ruler marker: "the snare is late here") and notes linked to a section: need new song data and a row.
- Searching all notes, and simple headings and to-do checkboxes: plain text first.
- Exporting the project notes with a bounce.
- Letting the local LLM read project and track notes when generating, or suggest what to note after a guide.
- Notes on the master, and editing track notes directly inside the inspector.

## Changes while building
- Group A hand-off (engine, built and tested): `daw::notebook` in `src/engine/Notebook.{h,cpp}` (both CMake targets;
  `Engine.h` includes it): `hasNotes`, `getFirstLine`, `normaliseLineEndings`, `getNotebookFile`,
  `overrideNotebookFolder (File)`, `loadAppNotebook()`, `saveAppNotebook (text)` (temporary file, then rename).
  Project notes: `Engine::getProjectNotes()` / `setProjectNotes (text)`. Track notes: public member `Track::trackNotes`,
  changed through `Engine::setTrackNotes (Track&, text)`; both setters normalise line endings and send a change only on
  a difference. Saved as `projectNotes` and `trackNotes` (left out when blank; read in `ProjectFile.cpp`). Notes stay
  out of `undo::Snapshot` as planned; undoing a track deletion restores them (tested). Tests: `runNotebookTests()`.
  Nothing is half-done; the UI groups need only call these.
- Group B hand-off (UI, built; `./build.sh --test` passes with no warnings): `NotebookWindow` (`src/ui/NotebookWindow.{h,cpp}`, `Tab { app, project, track }`,
  `showTab`, `flush`; saves the App notebook 1 s after typing and in its destructor, so on close and quit) and the shared `NoteIcon` (`src/ui/NoteIcon.{h,cpp}`,
  `update (const Track*)`), both in the `DawIt` target only. `MainComponent::showNotebook (std::optional<Tab> = nullopt)` (none: the remembered tab; `notebookCommand`,
  Cmd+9 in View, after Signal Flow); `TrackActions::onShowNotes` and the *Track Notes...* item (after *Show in Signal Flow...*, in the return's menu too) select the
  track and open the Track tab. The icon is in `TrackHeader` (left of the name; the name gives up 14 px), `ChannelStrip` (left of the name) and `InspectorPanel`
  (right of the name), each updated from its own timer. `ScreenshotRunner`: `notebookText` { app, project, tracks: { Bass: "..." } } (closes an open Notebook window
  first so it reloads; the App notebook is a scratch folder, `dawit-screenshot-notebook`) and `notebook` (app, project, track); documented in `DevScreenshots.h`.
  Departure: the placeholder is drawn by the window (wrapped) because `TextEditor`'s own is one line and would be cut off; unlike JUCE's it also shows while focused.
  A quick run on the pop song showed the Track tab and the inspector icon fine; the formal shots (a to f) are Group C's. `docs/recipes.md` still needs the two keys.
- Group C hand-off (done): real-app shots a to f looked at, no clipped text or overlap (icon beside "Bass" in the header, left of the name in the mixer strip, right of it in the inspector; empty App placeholder wraps in two lines). Shot (a) needs `"fold": "Drums"` to bring Bass into view. Docs done: README bullet (before *Take what you learn...*), tests.md, code-layout.md, architecture.md (*Saving*, not in a snapshot), ui-design.md (*The note icon and the Notebook*), recipes.md (the two shot keys). Issue #95 updated as Built, not committed.
