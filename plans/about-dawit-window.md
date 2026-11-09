# An About DawIt window with the version, licence, source link and every third-party notice, readable offline

Status: Released
Idea: [#170 — An About DawIt window with the version, licence, source link and every third-party notice, readable offline](https://github.com/geertvandamme/dawit/issues/170)

## Goal
An *About DawIt* window says what the user is running (version and build commit), that DawIt is free software under `AGPL-3.0-only` with no warranty, what that lets them do, and where the matching source is. A *Licences...* window shows the full text of `LICENSE` and every file in `licenses/`, compiled into the app, so it works with the network off. It closes the open item in DISTRIBUTION_TODO.md section 3.1 and blocks the v0.1.4 tag. The wider scope of #99 (per-library table with versions, website page, theory hub link) stays out.

## Decisions for you
1. **One window or two.** Options: (a) a small About window with a *Licences...* button that opens a second, resizable window; (b) one window with a side list whose first entry is the About page. **Recommended:** (a), because the About page stays small and readable, and "Licences..." follows the "..." rule for menu items and buttons that open a window.
   Answer:
2. **Where the source link points.** Options: always the tag `v<version>`; or the build's exact state. **Recommended:** a build made at a tagged commit links to `https://github.com/geertvandamme/dawit/tree/v<version>`; a clean build at another commit links to `.../tree/<full commit>`; a build with local changes, or without git (built from an archive), links to the tag and says so in a line under it. One CMake cache variable, `DAWIT_SOURCE_URL`, overrides it all: the single place to switch to the complete-source archive later.
   Answer:
3. **Names and groups for the notices.** Options: show bare file names; or keep a short table in the engine that gives each file a readable name and a group. **Recommended:** the table, with a test that fails when a file in `licenses/` has no entry, so a newly added notice (for example a downloaded model) can't appear without saying it's a download. A file missing from the table still shows, under *Other notices*, with its file name.
   Answer:
4. **macOS menus.** **Recommended:** *About DawIt...* only in the application menu on macOS (the platform's convention) and as the last item of *Help* on Windows and Linux, as the issue says.
   Answer:
5. **A Copy button for the version line.** **Recommended:** yes: it copies "DawIt 0.1.4 (commit 1a2b3c4)" for bug reports. Small, and the labels themselves can't be selected.
   Answer:

**Answered (9 November 2026):** the user took all five recommendations: (1) two windows; (2) the tag, commit or tag-with-note link, overridden by `DAWIT_SOURCE_URL`; (3) the table of names and groups with its test; (4) the app menu on macOS, *Help* elsewhere; (5) a Copy button for the version line.

## What the user does and sees
- **Opening:** macOS: *DawIt > About DawIt...* (JUCE's `setMacMainMenu` takes extra Apple-menu items; pass a menu holding the command). Windows and Linux: *Help > About DawIt...*, last in `getMenuForIndex` index 4, after a separator. Both through `PluginWindowManager::show`, one window per key, coming to the front when opened again. No shortcut (rarely used).
- **About window**, fixed, about 640 × 480, `Theme` colours, text that shrinks to fit:

  ```
  DawIt                                   (large, accent)
  DAW In Theory
  Version 0.1.4 · commit 1a2b3c4   [Copy]
  Copyright (C) 2026 DawIt contributors.
  <licence paragraph>
  <rights paragraph>
  <source line>      [Source code]   [Licences...]
  <third-party line>
  ```
- **Licences window**, resizable (minimum 720 × 480, opens at 960 × 660): a list on the left with group headings, a read-only, selectable, scrolling monospaced text view on the right (Cmd+A, Cmd+C work). Opens on *DawIt's licence and component list* (`LICENSE`). Up and Down move through the list. A one-line header above the text names the entry and its group.
- **Groups and order:** *DawIt* (`LICENSE`; `licenses/README.md` as "Where these texts come from"); *Built into DawIt* (everything not listed below, alphabetical by name, `GPL-3.0.txt` named "GNU GPL v3 (the VST3 SDK's licence as DawIt uses it)", the five Eigen files as "Eigen: COPYING.MPL2" and so on); *Downloaded when you ask* (Demucs weights, Whisper weights, MuseScore General); *Used to make the Windows installer* (Inno Setup); *Separate programs* (Ollama, a fixed text, no file).
- **Tooltips:** Copy: "Copy the version and commit, for a bug report". Source code: "Open the source code of this version on GitHub". Licences...: "Read the licence of DawIt and of every part it's built from. Works offline". The list: "Choose a licence or notice to read". The text view: "The full text, as its makers wrote it. Select and copy it with Cmd+C".
- **No right-click menu and no guide:** this window has no theory behind it (CLAUDE.md's guide rule doesn't apply).
- **Nothing remembered** between sessions; nothing saved in the project.

### Text of the About window (final, British spelling)
- Licence paragraph: "DawIt is free software, licensed under the GNU Affero General Public License, version 3 only (AGPL-3.0-only). It comes with no warranty: not even the implied warranty that it is fit for sale or for a particular purpose."
- Rights paragraph: "You may run DawIt for any purpose, study how it works, change it, and share it, changed or not. When you pass it on, you must pass on these same rights and make its source code available. The full terms are under Licences..."
- Source line, exact tag: "Source code of this version: github.com/geertvandamme/dawit, tag v0.1.4". Other commit: "... at commit 1a2b3c4". Local changes or no git: "This build has local changes or no record of its commit. The link opens the source of DawIt 0.1.4 as released."
- Third-party line: "DawIt is built with JUCE, demucs.cpp and Eigen, ONNX Runtime with Basic Pitch, whisper.cpp, TinySoundFont and others. Each keeps its own licence: see Licences... Ollama, which DawIt can talk to for AI, is a separate program with its own licence and is not part of DawIt."
- Header of a *Downloaded when you ask* entry: "Not in the app: DawIt downloads this after asking you, and keeps this notice beside it." *Separate programs* entry text: "Ollama runs on your own computer as a separate program. DawIt sends it requests over a local connection. Ollama and each language model you install have their own licences; DawIt does not include or distribute them."
- Commit unknown: "commit unknown".

## Data and saving
None in the project or the settings.

## The guide
None: not a theory guide, no `gen::guides` entry, no `SOURCES.md` heading.

## Code
- **Recipes:** recipes.md *A command or menu item* and *A window* (key: a member `SafePointer`, as `tunerWindow`).
- **Embedded notices:** a new binary-data target (copy `DawItSources`) holding `LICENSE` and every file under `licenses/`, collected with `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`, linked into `DawIt` and `DawItTests`. JUCE keeps only the base name, so base names must stay unique (a test checks); Eigen's are identified by their `COPYING.` prefix.
- **Build info:** version from `PROJECT_VERSION`; short and full commit, "has local changes" and "is exactly tag v<version>" from git, written by a small CMake script run at every build into a generated header that is only rewritten when its content changes (so an unchanged tree doesn't recompile). No git, or not a git checkout: commit empty, shown as "unknown". `DAWIT_SOURCE_URL` (cache, empty by default) overrides the link. App target only; the logic it feeds is tested with plain inputs.
- **`src/engine/AboutInfo` (namespace `daw::about`), pure, no UI:** the notice list from the embedded data (name, group, file name, text, in the order above, using the name table of decision 3); the version line; the source URL and source line from (version, commit, dirty, exact tag, override); the fixed texts above. Text is read with `juce::String::fromUTF8`.
- **`src/ui/AboutWindow` (About and Licences components):** only lays out what `about::` returns. Opening the link uses `juce::URL::launchInDefaultBrowser`.
- **Command:** `aboutCommand` in `MainComponent` ("About DawIt...", description "The version, the licence, where to get the source, and every third-party notice"). Added to Help on non-Mac only, and to the Apple menu on Mac.
- **Threading:** message thread only; no lock, no audio-thread change.
- **Risks:** the build-info step must not break builds without git or rebuild everything each time; binary size grows by about 0.7 MB of text.
- **Shared files with the uncommitted theory-guides work:** `CMakeLists.txt` (source lists and a new data target), `MainComponent.{h,cpp}` (a command, menu items, a `SafePointer`, a show function), `DevScreenshots.cpp` (two scenes), `tests/EngineTests.cpp` (a new `runAboutTests`). Add lines next to, never inside, the hunks for `TheoryGuideLinks`, `GuideNavigator`, `GuideFooter` and `validateLinks`; don't stage or revert them.

## Libraries and models
None adopted. The window embeds the existing `LICENSE` and `licenses/` texts; the release packages keep copying them beside the app (DISTRIBUTION_TODO.md 3.2). The ONNX Runtime per-platform notices and the curl notice gap (3.2) stay open: the window shows what `licenses/` holds and picks up a fix by itself. The in-app link is a convenience, not a substitute for the complete-source item in 3.1, which stays open.

## Theory
None.

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. CMake: the notices binary-data target (all of `licenses/` plus `LICENSE`), linked to both targets; a `DAWIT_SOURCE_DIR` compile definition for `DawItTests` only; the build-info script, generated header and `DAWIT_SOURCE_URL` cache variable for `DawIt`. Check: a configure and build passes; touching no file and building again recompiles nothing.
- [x] 2. `src/engine/AboutInfo.{h,cpp}` (in both targets' sources): the notice list with the name and group table, the version line, the source URL and line rules of decision 2, and the fixed texts. Tests: see *Tests*.
- [x] 3. `runAboutTests` in `tests/EngineTests.cpp`, called from `main`. Done when `./build.sh --test` passes with no warnings.

Group B, UI (`./build.sh --test` at step 6):
- [x] 4. `src/ui/AboutWindow.{h,cpp}`: the About component (layout and texts above, Copy, Source code, Licences...) and the Licences component (grouped list, header line, read-only selectable text view). Check: both lay out at their sizes with no clipped text.
- [x] 5. `MainComponent`: `aboutCommand`, show functions for both windows through `pluginWindows.show` (Licences with its minimum size), Help item on Windows and Linux, the extra Apple-menu item on macOS (set on `setMacMainMenu`, cleared with it).
- [x] 6. `ScreenshotRunner`: scenes `about` and `licences` (with optional `"notice": "<file name>"` selecting an entry), listed in `getSceneNames`. Done when `./build.sh --test` passes.

Group C, check and docs:
- [x] 7. Real-app check, one run: `{ "scene": "about", "capture": "window" }`; `{ "scene": "licences", "capture": "window" }`; the same with `"notice": "Demucs-weights-LICENSE.txt"` and with `"notice": "COPYING.MPL2"`. Look for: the version matching `CMakeLists.txt`, a commit or "commit unknown", every paragraph uncut, the list's group headings, the download header line, the text scrolled to the top. Open *DawIt > About DawIt...* twice by hand to see one window come forward (copy the settings aside first, per CLAUDE.md).
- [x] 8. Docs: README *Features* bullet (the app itself) and a sentence in *License note* that the texts are also readable in *About DawIt > Licences...*; `docs/code-layout.md` (`AboutInfo`, `AboutWindow`, the build-info script); `docs/tests.md` (the about checks); `docs/architecture.md` *Libraries and models* and `licenses/README.md` *Updating*: a new notice file is embedded by itself but needs a name and group in `about::`, and replace "The existing CMake build only copies Bravura's licence into the app" with the embedding fact; DISTRIBUTION_TODO.md 3.1: tick the About item, keep the complete-source item open and note the `DAWIT_SOURCE_URL` switch; issue #170 progress comment with the checklist ticked.

## Tests
`runAboutTests`:
- Every file under `DAWIT_SOURCE_DIR/licenses` (recursively, today 41 including `README.md`) and `LICENSE` is embedded, byte for byte equal; the embedded count equals the file count on disk (catches a stale glob or a base-name clash); no embedded entry lacks a file.
- Every file has a name-table entry (decision 3), and each name is non-empty and unique.
- Groups: `Demucs-weights-LICENSE.txt`, `Whisper-weights-LICENSE.txt`, `MuseScore-General-LICENSE.md` are *Downloaded when you ask*; `Inno-Setup-LICENSE.txt` is the installer group; `LICENSE` is first; the Ollama entry exists, has no file, and its text contains "separate program".
- `LICENSE`'s text contains "AGPL-3.0-only" and "GNU AFFERO GENERAL PUBLIC LICENSE"; every entry's text is non-empty.
- Version line: ("0.1.4", "1a2b3c4") gives "Version 0.1.4 · commit 1a2b3c4"; an empty commit gives "commit unknown".
- Source URL: exact tag gives `https://github.com/geertvandamme/dawit/tree/v0.1.4`; clean, other commit gives `.../tree/<full 40-character commit>`; dirty or empty commit gives the tag URL and the "local changes" line; a non-empty override wins in every case.
- The licence paragraph contains "AGPL-3.0-only" and "no warranty".

## Website hand-off
- `about.html` (*How it's made*): one line that the app shows its licence, the source link and every third-party notice in *About DawIt*, offline.
- `features.html`: not needed beyond, at most, a line in the group about the app itself; no Home or `for-you.html` change.
- Screenshot only if dawWebsite wants one: the Licences window on *Downloaded when you ask*.

## Left for later
- #99's per-library table (version, purpose, link) from one data file checked against `CMakeLists.txt`, the website page and the theory hub link.
- The complete-source archive (DISTRIBUTION_TODO.md 3.1): set `DAWIT_SOURCE_URL` to it once the archive script exists.
- Search in the Licences window.

## Changes while building

**Hand-off from Group A (engine):**
- `CMakeLists.txt`: binary-data target `DawItNotices` (namespace `dawnotices`, header `DawItNotices.h`; `LICENSE` plus a recursive glob of `licenses/*`, dot-files excluded) linked to `DawIt` and `DawItTests`; `DAWIT_SOURCE_DIR` define for `DawItTests`; cache variable `DAWIT_SOURCE_URL`; custom target `DawItBuildInfo` (ALL, a dependency of `DawIt`) runs `cmake/BuildInfo.cmake` and writes `build/Release/generated/BuildInfo.h` only when its content changes (a second build recompiles nothing); that directory is on `DawIt`'s include path only.
- `BuildInfo.h` gives `daw::buildinfo::version`, `commit` (full hash, empty without git), `hasLocalChanges`, `isExactTag`, `sourceUrlOverride` (all constants). "Local changes" means tracked files differ from HEAD (untracked files are ignored). Note: the project version in `CMakeLists.txt` is 0.1.3 today, not the 0.1.4 of the plan's examples.
- `src/engine/AboutInfo.{h,cpp}` (`daw::about`, in both targets): `getNotices()` (ordered `Notice {name, group, fileName, text}`, Ollama last in its group, no file), `getGroupName (Group)`, `getNoticeHeader (notice)` (name, group in brackets, plus the "Not in the app..." sentence for downloads), `getVersionLine (version, commit)`, `getCopyLine (version, commit)`, `getSourceInfo (version, commit, hasLocalChanges, isExactTag, urlOverride)` giving `{url, line, note}`, and `getLicenceText()`, `getRightsText()`, `getThirdPartyText()`. Group B calls them with the `buildinfo::` values.
- `runAboutTests` is in `tests/EngineTests.cpp` (22 checks, all pass). 42 notice files are embedded (41 in `licenses/` plus `LICENSE`).
- Surprise: the plan's "text of a Downloaded entry header" is the sentence only; `getNoticeHeader` puts it after "<name> (<group>)." so the header names the entry as the plan's layout asks.
- `./build.sh --test` ends with `All checks passed`. Three warnings remain, none in this group's code: `-Wshadow` in `src/ui/ChordExplainer.cpp:1177` and `src/ui/DrumTriggerExplainer.cpp:771`, and two in `tests/EngineTests.cpp` around lines 13974 and 13978 (theory-guides progress tests). All pre-date this work.

**Hand-off from Group B (UI):**
- `src/ui/AboutWindow.{h,cpp}` (in the `DawIt` sources only): `AboutComponent` (640 x 480, paint-based text blocks that shrink from 14 pt down to 9 pt to fit; `onShowLicences`) and `LicencesComponent` (960 x 660, `ListBox` with group-heading rows that can't be selected, header label, read-only monospaced `TextEditor`; `selectNotice (fileName)`). Both read `buildinfo::` (from the generated `BuildInfo.h`) and `about::`.
- `MainComponent`: `aboutCommand` ("About DawIt...", Help menu on Windows/Linux after a separator; on macOS `appleMenuExtras` passed to `setMacMainMenu`), `showAbout()`, `showLicences (fileName = {})` (minimum size 720 x 480), `aboutWindow` / `licencesWindow` SafePointers.
- `ScreenshotRunner`: scenes `about` and `licences` (`"notice": "<file name>"`), both in `getSceneNames`.
- Not looked at in the real app yet: Group C should check the About text fits (the buttons row sits below the source line and note), the header line of the Licences window (44 px high, may wrap), and Up/Down in the list with a heading between groups.

**Hand-off from Group C (check and docs):**
- Real-app shots (`about`, `licences`, Demucs and `COPYING.MPL2` entries) all looked right: version 0.1.3 with the commit, every paragraph and the local-changes note uncut, group headings, the one-line download header (did not wrap), buttons row below the note. One fix: a newly chosen text could start scrolled a few pixels (Eigen's first line was clipped); `AboutWindow.cpp` now calls `text.scrollEditorToPositionCaret (0, 0)` after `setCaretPosition (0)`.
- Checked by hand on macOS on 9 November 2026: *DawIt > About DawIt...* chosen twice brings the same window forward, and Up/Down moves across the Licences list headings.
- Docs updated: README (Features bullet, *License note*), `docs/code-layout.md`, `docs/tests.md`, `docs/architecture.md`, `licenses/README.md`, DISTRIBUTION_TODO.md 3.1 (About ticked, complete-source item left open).
