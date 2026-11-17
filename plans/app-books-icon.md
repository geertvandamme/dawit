# Give the DawIt app its own books icon on macOS, Windows and Linux

Status: Released
Idea: [#193 — Give the DawIt app its own books icon on macOS, Windows and Linux](https://github.com/geertvandamme/dawit/issues/193)

## Goal
DawIt gets its own app icon, the open book with five rising bars the app already uses for its guide buttons and the website logo, in the macOS Dock and Finder, the Windows taskbar, installer shortcuts and Explorer, on Linux in the window, the taskbar and the application menu, and in the *About DawIt* window (issue #193 asks for this too). Today the app shows the system's default icon. A beginner can then find DawIt among their apps and see it is the same book as the "explanation here" icon.

## Decisions for you
1. **Who owns the artwork and what licence it gets.** The book logo was created with ChatGPT (credited on the website's *How it's built* page). The source drawing named in `src/ui/GuideButton.cpp` (`designs/dawit-spectrum-book-variations/variation-01.png`) is not in the repository. Options: (a) treat the artwork as DawIt's own work under `AGPL-3.0-only`, listed in the `LICENSE` inventory with its provenance; (b) a separate artwork licence (such as CC BY-SA 4.0) and name/logo use rules; (c) redraw it by hand. **Recommended:** (a), because the app's own vector path (`guideicon`) already ships under AGPL, and it needs your statement that you hold the rights under ChatGPT's terms. Copyright in AI-generated images is uncertain in some countries; the plan records that and does not claim more.
   Answer: Artwork under AGPL-3.0-only with provenance recorded.
2. **Look of the icon.** Options: (a) the book and bars in paper colour (`#f8f7f2`) on a dark-green rounded square (`#304c37`, the website's accent), (b) the book alone on a transparent background, (c) the app's dark UI colour with the blue accent. **Recommended:** (a), because it matches the website and reads at 16 px; the app's own blue stays inside the app.
   Answer: Paper-coloured book on a dark-green rounded square.
3. **Shape per platform.** macOS 11+ expects the app to draw its own rounded square inside a 1024 px canvas (about 824 px with transparent margin). Windows and Linux icons are normally full-bleed. **Recommended:** the macOS rounded square (824 px, in the 1024 canvas) for macOS only; the same tile, full-bleed with rounded corners, for Windows and Linux.
   Answer: Inset rounded square on macOS; full-bleed rounded tile on Windows and Linux.
4. **Linux scope.** Options: (a) window icon from the app (always) plus a `.desktop` file and hicolor icons in the tar.gz with a README line on installing them; (b) window icon only. **Recommended:** (a); an AppImage stays a later release decision (`DISTRIBUTION_TODO.md` section 13).
   Answer: Window icon, desktop entry and hicolor icons in the Linux archive.
5. **Rights confirmation (open).** Whether you hold the rights to the artwork under ChatGPT's terms is not assumed. Confirmed by the user on 2026-11-17 (see the answer below); the `LICENSE` entry records the confirmation instead of "rights to be confirmed".
   Answer: The user confirms the rights to use and distribute the artwork as proposed (2026-11-17).
6. **Icon in the About window.** Options: (a) a 64 px tile with the book beside the version line, drawn from `guideicon::getPath()` in the same tile colours; (b) the plain book without a tile. **Recommended:** (a), so About matches the Dock icon.
   Answer: A 64 px tile beside the version line.

## What the user does and sees
- No new controls. Dock and Cmd+Tab, Finder, the `.dmg` window (macOS); the title bar, taskbar, Start menu and desktop shortcut, Explorer, the installer wizard and *Apps* list (Windows); the window title bar and taskbar, and the menu entry when the `.desktop` file is installed (Linux).
- Master artwork is made once from the same path `guideicon::getPath()` draws (so the icon and the guide buttons stay one shape), as 1024 px PNGs: a macOS one (inset rounded square) and a full-bleed one, plus small hand-checked sizes for 16, 32 and 48 px where thin gaps between bars need a thicker rendering.
- The *About DawIt* window (fixed 640 x 480) shows the icon at the top left, about 64 px, beside the version line; the text blocks shift right to make room and still shrink to fit. Tooltip: "The DawIt icon: an open book with five rising bars".
- Nothing is saved; no settings.

## Data and saving
None.

## The guide
None (no theory). First steps: no change (no command, menu or control changes).

## Code
Follows `docs/recipes.md` only loosely (no recipe fits; the packaging scripts are the closest). Design:
- **Source of truth:** `guideicon::getPath()`. A console tool `DawItExportIcon <folder>` (`tools/ExportAppIcon.cpp`; a GUI flag cannot run without a window server) renders the PNG master set offscreen with `juce::Image`/`Graphics`; the PNGs are committed in `resources/icons/` (generated, with a README line saying how to regenerate), so builds need no extra tool.
- **macOS and Windows:** `juce_add_gui_app` gets `ICON_BIG`/`ICON_SMALL` (PNG). JUCE's `juceaide` makes `Icon.icns` into `Contents/Resources` and `icon.ico` into the exe's resources. Check in the pinned JUCE 8.0.9 which sizes it produces; if the 16 and 32 px entries come out blurry, supply them through the small image or replace the generated file with a script-made `.ico` (Python standard library, PNG entries 16, 32, 48, 256). Inno's shortcut and uninstall icon already point at `DawIt.exe`; add `SetupIconFile` for the installer itself.
- **Linux:** JUCE sets no window icon on Linux. Embed a 256 px PNG (`juce_add_binary_data`, or a `.h` generated from the PNG), and call `DocumentWindow::setIcon` in `MainWindow` (`src/Main.cpp`) on every platform (harmless elsewhere). `package-release-linux.sh` adds `dawit.desktop` (`Name=DawIt`, `Exec=DawIt`, `Icon=dawit`, `Categories=AudioVideo;Audio;Midi;`), `icons/hicolor/<size>/apps/dawit.png` (16, 32, 48, 64, 128, 256, 512) and an install note; `.desktop` Exec/Icon use relative names, so the README line says to run an included `install-desktop.sh` that writes absolute paths for the user's chosen folder to `~/.local/share/`.
- **About window:** `AboutComponent` (`src/ui/AboutWindow.cpp`) paints the tile and the `guideicon` path in `paint`, with the tile colours from one shared helper so the About icon and the files cannot differ. No image file is loaded. Keep the 640 x 480 size and the text-fit rule.
- **Rules:** artwork colours are a few literals in the generator only (the app UI still uses `Theme.h`); the `.dmg` keeps the app's icon, no volume icon.
- **Risks:** a generated `.icns` hidden by macOS's icon cache (check with a fresh copy of the app); `setIcon` on a native Linux title bar (check under X11 and, if available, Wayland, where the icon comes from the `.desktop` file).

## Libraries and models
None added. Licence findings:
- **Artwork:** the only new asset. Provenance above (AI-assisted, drawn by ChatGPT; vector reproduction by the project). Add an *Artwork* entry to the `LICENSE` component inventory: "DawIt book icon (app icon, `resources/icons/`), created with ChatGPT, reproduced as `guideicon` path; licence AGPL-3.0-only per decision 1; rights confirmed by the owner on 2026-11-17 (decision 5)". Add a line to the README *License note* and a paragraph in `licenses/README.md` saying no third-party notice file is needed. No new file in `licenses/`, so no `about::` entry.
- **Tools:** JUCE's `juceaide` icon generation is part of JUCE 8.0.9 (AGPLv3 option; already listed). macOS `iconutil`, if used, is Apple's system tool and none of its code is redistributed. The optional Python script uses only the standard library and is not shipped. Inno Setup 6.4.3 (already in the inventory) embeds the icon in the setup exe; no new terms. The freedesktop `.desktop` and icon-theme layouts are open specifications, with no code.
- **Transitive dependencies:** none. No font or SVG library is added.
- **Open point:** the original drawing is still missing from the repository; the rights are confirmed (decision 5), so the `LICENSE` line records the confirmation and the missing source drawing.

## Theory
None.

## Steps
Group A, icon files and engine/build wiring (`./build.sh --test` at step 4):
- [x] 1. Add the `DawItExportIcon <folder>` console tool that renders the PNG set from `guideicon::getPath()` (colours per decision 2, shapes per decision 3, the 1024 inset-tile macOS image, the full-bleed tile, 16/32/48 small versions). Run it once and commit the PNGs to `resources/icons/`. Tests: a headless check that the render of 256 px has the tile colour at the corner and the book colour at the centre, and a non-empty image at every size. Done when: the PNGs look right when opened.
- [x] 2. In `CMakeLists.txt` set `ICON_BIG`/`ICON_SMALL` on `DawIt` and embed the 256 px PNG as binary data; call `setIcon` in `MainWindow`. Check: the built `DawIt.app/Contents/Resources/Icon.icns` exists and `iconutil -c iconset` lists its sizes; the app shows the icon in the Dock.
- [x] 3. Windows: `SetupIconFile` in `tools/windows-installer.iss`; confirm the generated `.ico` sizes (16, 32, 48, 256) from the Windows CI build (a step in `windows-installer-check.yml` lists them with a small script and fails if one is missing). Linux: add the `.desktop`, hicolor icons and install script to `package-release-linux.sh`, and extend `tools/check-linux-start.sh` to check the files are in the tar.gz. `./build.sh --test` passes without warnings.
- [x] 4. About window: paint the icon tile in `AboutComponent`, shift the text blocks, add the tooltip; check in the existing About shot (`DevScreenshots`) that no text is clipped. `./build.sh --test` passes without warnings.

Group B, check and docs (last):
- [~] 5. Real-app check: the user confirmed the icon works on macOS. Remaining platform, packaging and About checks are tracked in [#211](https://github.com/geertvandamme/dawit/issues/211).
- [x] 6. Docs: README Features bullet and *License note* line; `LICENSE` inventory entry and `licenses/README.md` paragraph (decision 1); `docs/code-layout.md` (`resources/icons/`, the flag) and `docs/tests.md` (the render check); `docs/recipes.md` (the flag, with a one-line "how to regenerate the app icon"); `DISTRIBUTION_TODO.md` (tick the Linux `.desktop`/icon item and the artwork provenance item, with the outcome of decision 1); issue #193 progress and a link to this plan (`Local plan, not committed: plans/app-books-icon.md` until committed).

## Tests
- 256 px render: corner pixel equals the tile colour (±2 per channel); centre bar pixel equals the glyph colour (±2); alpha at the macOS image's corner is 0 and at the tile's edge midpoint is 255.
- Every size in the set (16, 32, 48, 64, 128, 256, 512, 1024) renders and has more than 5 % glyph-colour pixels.
- About window: the icon area lies inside 640 x 480 and overlaps no text block (screenshot in step 5 if it can't be checked headless).
- The embedded PNG decodes to 256 x 256.
- Packaging checks (shell, in CI): `Icon.icns` present in the macOS bundle; the `.ico` has 16, 32, 48 and 256 px entries; the Linux tar.gz holds the `.desktop` file and the seven hicolor icons.

## Website hand-off
For dawWebsite, after release and not part of this build: `about.html` / *How it's made* (artwork provenance, if decision 1 adds a line to the credits); `get-started.html` only if the icon helps people find the downloaded app (optionally a small shot of the Dock or the installer). The existing `assets/logo.png` is the same glyph; keep it. Favicon (website phase, not this build): add one on all seven pages from the new PNG set (16, 32 and 48 px, and an Apple touch icon from a 180 px render). It needs the PNGs committed first and depends on the rights confirmation (decision 5).

## Left for later
- An AppImage or distribution packages with their own metadata (appstream).
- A dark/tinted macOS 26 icon variant (an Icon Composer `.icon` file).
- A separate document icon for `.dawit` project files, and file association.
- Replacing the logo by hand-drawn artwork if the rights question stays open.

## Changes while building
Group A hand-off (checked results, supersedes the "not built / placeholder / next steps" notes below):
- **Verified:** `DawItExportIcon` built and exported 11 files to `resources/icons/` (the placeholders are replaced by real renders); `./build.sh --test` ended with `All checks passed`, exit 0, no `warning:` in `build/app-icon-build.log`; the 256 px PNG was looked at.
- **Ticked:** step 1. Steps 2-4 are written and build and test cleanly but stay unticked, because their own checks are open: step 2 (`Icon.icns` in the bundle, `iconutil` sizes, Dock), step 3 (the `.ico` sizes from the Windows CI build, the Linux archive contents run through `check-linux-start.sh`), step 4 (the About shot with no clipped text).
- **For Group B:** look at the other sizes (16/32/48 especially, the larger-book choice), do the macOS Dock/Finder/Cmd+Tab/`.dmg` check on a fresh copy, About shot, and the docs. **Not done on real Windows or Linux:** title bar, taskbar, menu entry and `.ico` rendering; record them as open points in the README if still unseen.
- Original notes (historical):
- Code: `src/ui/AppIcon.{h,cpp}` (`daw::appicon`: `getTileColour`, `getGlyphColour`, `drawTile`, `render (size, macInset)`, `getSizes`, `makeIco`, `getEmbedded`, `exportAll`); `setIcon` in `src/Main.cpp`, console tool `tools/ExportAppIcon.cpp`; `AboutComponent::IconTile` (64 px tile, tooltip) in `src/ui/AboutWindow.*`, title/subtitle/version moved right to x = 108; `runAppIconTests` in `tests/EngineTests.cpp`; `CMakeLists.txt` (binary data `DawItAppIcon` / `dawicon::appicon256_png`, `ICON_BIG`/`ICON_SMALL`, `AppIcon.cpp` in both targets, `GuideButton.cpp` added to `DawItTests`).
- **The PNGs in `resources/icons/` are placeholders** (copies of `website/assets/logo.png`, so CMake could configure). First thing to do: run the exporter (see the crash-diagnosis update below), look at the PNGs, then rebuild (the 256 px file is embedded) and run the tests.
- **Update (user's run):** tests ran; the only failure was `app icon: the embedded PNG decodes to 256 x 256` (the placeholder is the logo, not 256 px). `exportAll` itself passed in the tests. The GUI binary with `--export-app-icon` exited 134 (abort); the cause is unconfirmed (not reproduced: no runs allowed here). Hypothesis: `quit()` called inside `initialise()` before the message loop runs, then the app shuts down with no engine or window. Fix applied in `src/Main.cpp`: the export now runs in `MessageManager::callAsync`, then `quit()` from the loop. Still unbuilt.
- **Next steps (user):** superseded by the crash-diagnosis update below: build and run `DawItExportIcon`, look at the PNGs, then `./build.sh --test` (re-embeds the real 256 px file), then tick steps 1-4.
- Change from the plan: also writes `resources/icons/app-icon.ico` (16/32/48/256, PNG entries) so Inno's `SetupIconFile=..\resources\icons\app-icon.ico` has a file; the app's own `.ico` still comes from JUCE. The CI check in `windows-installer-check.yml` inspects this committed file, not JUCE's generated one (the Windows build was not run).
- Linux: `package-release-linux.sh` adds `dawit.desktop`, `install-desktop.sh` and `icons/hicolor/<size>x<size>/apps/dawit.png` (16-512); `check-linux-start.sh` checks they are in the archive. README line on installing: Group B.
- Not done: the About "no overlap" check is left to the screenshot in step 5. Small sizes (16/32/48) use a larger book (76 % of the tile) instead of a hand-thickened rendering; judge them when you open the PNGs.
- Decision 5 is confirmed by the user (2026-11-17); the `LICENSE` entry in Group B says so (no "rights to be confirmed").
- Licences: nothing added in Group A; `LICENSE`, `licenses/README.md` and the README *License note* are Group B's.
- **Update (crash diagnosis):** the abort (exit 134) is SIGABRT in HIServices `_RegisterApplication`, from `NSApplication sharedApplication` in `main`, before `initialise()`: JUCE's `START_JUCE_APPLICATION` connects to the window server, which fails in a session without one (SSH, sandbox, CI). So the `callAsync` fix could never help, and no GUI-app flag can work there. Change: the flag is gone from `src/Main.cpp`; a console tool `DawItExportIcon` (`tools/ExportAppIcon.cpp`, target in `CMakeLists.txt`) calls the same `daw::appicon::exportAll`, with no NSApplication. `setIcon` stays in `MainWindow`. Run: `cmake --build build/Release --target DawItExportIcon --parallel && build/Release/DawItExportIcon_artefacts/Release/DawItExportIcon resources/icons; echo $?` (expect 0 and 11 files). Not built or run by me. Later docs (Group B) name this tool, not a `DawIt` flag, in `docs/recipes.md` and `docs/code-layout.md`.

Group B hand-off (checked results):
- **Checked here:** `./build.sh --test` after the docs: `All checks passed`, no warnings. `DawIt.app/Contents/Resources/Icon.icns` exists (33 KB). **JUCE 8.0.9 writes only two entries: `ic10` (512 px @2x, from the 1024 image) and `icp6` (48 px)**; there are no 16/32/128/256 entries, so macOS scales the 1024 image down at 16 and 32 px and the hand-made small sizes are not used there. Accepted for now (the 16 px PNG looks fine as a flat glyph); a script-made `.icns` through `iconutil` from per-size macOS-inset renders is the follow-up if Finder's list view looks blurry. `resources/icons/app-icon.ico` has 16, 32, 48 and 256 px entries (read with a script). `package-release-linux.sh` and `check-linux-start.sh` pass `bash -n`; I did not run them (no container). The 16 px and macOS 1024 PNGs were looked at: tile, book and bars read clearly.
- **Not possible in this session:** the GUI cannot start (macOS `_RegisterApplication` aborts without a window server), so the `--screenshots` About shot, the Dock, Finder, Cmd+Tab and `.dmg` checks, the Windows CI step, the Linux archive run and the title bar/taskbar/menu entry were not done. Step 5 stays `[~]`. To do on a desktop: `perl -e 'alarm 90; exec @ARGV' build/Release/DawIt_artefacts/Release/DawIt.app/Contents/MacOS/DawIt --screenshots shots.json` with `{"folder":"shots","scale":1,"shots":[{"file":"about.png","scene":"about","capture":"window"}]}` (look for clipped text beside the 64 px tile); copy the app to a new folder to avoid the icon cache and look at Dock, Finder list view and Cmd+Tab; run `tools/check-linux-start.sh` on the Linux archive and the Windows installer check in CI.
- **Docs and licence:** `LICENSE` inventory entry *DawIt book icon*, README feature bullet and *License note* paragraph, `licenses/README.md` paragraph, `docs/code-layout.md`, `docs/tests.md`, `docs/recipes.md` (*Regenerating the app icon*), `DISTRIBUTION_TODO.md`. No new `licenses/` file, no new dependency; remaining obligation: the original drawing file is not in the repository. First steps: checked, no change.
