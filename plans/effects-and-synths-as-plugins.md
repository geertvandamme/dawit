# DawIt's effects and synths as VST3 and AU plugins

Status: Built, not committed. Theory checked, with open points
Idea: [DawIt's effects and synths as VST3 and AU plugins](../FEATURE_IDEAS.html#dawits-effects-and-synths-as-vst3-and-au-plugins)

## Goal
DawIt's built-in effects and synths also build as VST3 plugins (Reaper, Live, Bitwig, Cubase, FL Studio) and AU plugins (Logic, GarageBand, which load only Audio Units), with the same window, knobs, starting points and "What is ...?" guide as in DawIt. Someone who learned with DawIt keeps the same tools and explanations in the DAW they grow into. Above all, every built-in is registered once in CMake, so a new effect or synth becomes a plugin by the same one call that adds it to the app.

## Decisions for you
1. **Which plugins first.** Options: all 14 built-ins; the 11 in the idea; all but DawIt Tune (13). **Recommended:** 13, all but Tune, because Tune corrects to the song's key and chords, which a host doesn't give; it needs its own key picker first. Tune stays app-only by one flag in its registration.
   Answer: 13, all but Tune (user).
2. **Where the guide opens in a plugin.** Options: a page that slides over the controls inside the plugin window; a side panel that widens the window; a separate window as in DawIt. **Recommended:** the page over the controls, with *Back to the controls* and Esc, because hosts treat extra windows differently (Logic keeps them behind its own) and the window size stays fixed. DawIt itself keeps its separate guide windows.
   Answer: Page over the controls (recommended, user did not object).
3. **Standalone apps.** Options: none; for the synths; for all. **Recommended:** none for now; DawIt is the standalone. The registration names the formats, so adding Standalone later is one word.
   Answer: None for now (user).
4. **Building and installing.** Options: plugins built by every `./build.sh`; only with `./build.sh --plugins`. **Recommended:** `--plugins` (13 plugin targets each compile JUCE, several minutes), plus `--install-plugins`, which also copies them to `~/Library/Audio/Plug-Ins/VST3` and `Components`. Agents never run `--install-plugins` without your OK, since it changes what your DAWs see.
   Answer: `--plugins` and `--install-plugins` (recommended, user did not object).
5. **Codes, permanent once anyone saves a session with them.** Manufacturer `Dwit`; plugins `Dequ` EQ, `Dcmp` Compressor, `Dgat` Gate, `Dstr` Stereo, `Drvb` Reverb, `Ddly` Delay, `Dsat` Saturation, `Ddst` Distortion, `Dsub` Subtractive, `Dadd` Additive, `Dfmx` FM, `Ddrm` Drums, `Dphy` Physical, `Dtun` Tune (later). Each has one capital, first, as AU and GarageBand need. Bundle ids `com.dawit.plugin.<name>`. **Recommended:** these.
   Answer: As recommended.
6. **Names in the host.** Options: "DawIt Reverb" (as in DawIt's own menus); "Reverb" under the maker DawIt. **Recommended:** "DawIt Reverb": the same name everywhere, and a project that names it finds it.
   Answer: "DawIt Reverb" (recommended).
7. **The VST3 SDK licence.** JUCE 8.0.9, which DawIt uses, bundles a VST3 SDK under Steinberg's licence or GPLv3; the MIT SDK 3.8 comes with JUCE 8.0.11. Options: stay on 8.0.9 and use the SDK under GPLv3, which may be combined with AGPLv3 code; upgrade JUCE first. **Recommended:** stay on 8.0.9 for this plan and upgrade JUCE as its own change, since an upgrade touches the whole app. The idea's line "MIT since 3.8" gets corrected.
   Answer: Stay on 8.0.9, GPLv3 SDK (recommended).
8. **Signing.** **Recommended:** ad-hoc only (the linker already signs on Apple Silicon); Developer ID and notarisation stay in DISTRIBUTION_TODO.md. Downloaded plugins will need Gatekeeper's Open Anyway until then.
   Answer: Ad-hoc only (recommended).
9. **Export PDF in a plugin's guide.** **Recommended:** left out (it needs the app's export dialog and guide list); the guide says DawIt can export it.
   Answer: Left out (recommended).

## What the user does and sees
- **In DawIt:** nothing changes, except that DawIt's add-plugin menus hide DawIt's own VST3 and AU copies (manufacturer DawIt), so "DawIt Reverb" isn't listed twice; the built-in is used.
- **In a host:** the plugins appear under the maker *DawIt* as effects (VST3 category per kind: EQ, Dynamics, Reverb, Delay, Distortion, Spatial; synths Instrument|Synth, Drums Instrument|Drum) and instruments. Opening one shows the same window as in DawIt at the same size: header with starting points, guide button, Clap or Play a note, *Hear only what it adds*, match level; knobs; the picture; the explainer on the right. Parameters are automatable with the names and value texts DawIt shows ("350 ms").
- **The guide** (decision 2) slides over the controls and fills the window: topic list, picture and text as in DawIt's guide window, with a *Back to the controls* button top left; Esc and the guide button also go back. It reopens on the page it showed.
- **What's different from DawIt:** the overview text ends with a paragraph saying it is part of DawIt and what the app adds; *Your track* pages work from the settings alone and call the track "This track"; there's no device bar chip, gain staging badge or Export PDF. Tempo comes from the host's play head, as it already does.
- Tooltips: the guide button "Opens the guide to <effect> inside this window"; Back "Back to the knobs and pictures (Esc)". Everything else keeps its tooltip.
- Nothing is remembered in app settings; the plugin's state is its normal state blob, saved by the host.

## Data and saving
None in the project. A plugin's state is exactly the built-in's `getStateInformation`, so a sound's state loads the same in DawIt and in a host (tested: the same blob into both). DawIt's add menus filtering its copies is not saved.

## The guide
No new guide or topics. The existing "What is ...?" guides open inside the plugin window. Text changes: the overview paragraph and the *Your track* name in plugins (in *Theory*).

## Code
Recipes: *A built-in effect or instrument* (changed: step 2 becomes the CMake registration) and *A window* for the overlay.
- **The registry** is one CMake function in `cmake/DawItBuiltIns.cmake` (`dawit_add_builtin`), one call per built-in with: its name, processor class and header, editor class and header, kind (effect or instrument), plugin code, VST3 category, engine sources, UI sources, and `APP_ONLY` (Tune). It adds the sources to `DawIt` (and engine ones to `DawItTests`), appends to a generated header `DawItBuiltInList.h` (name, identifier, code, kind, app-only) for the tests, and, when `DAWIT_BUILD_PLUGINS` is on and not `APP_ONLY`, makes a `juce_add_plugin` target (VST3, plus AU on Apple) from a template entry file (`src/plugin/PluginEntry.cpp.in`) whose `createPluginFilter` sets the editor function and returns the processor. The shared plugin sources (fxui frame, `EffectTools`, `BuiltInSynth`, `Theme`, ...) are one list beside it. Manufacturer, company and version are set once for the directory.
- **Rule:** plugin sources never include `engine/Engine.h`, `generate/TheoryGuides.h` or app UI (`SectionLane`, `PluginWindows`). The seam: `guideprint::Source` moves to a header of its own; `fxui::Guide` gets its Export button through a hook `MainComponent` installs (no hook: no button).
- **Plugin mode** is `processor.wrapperType != wrapperType_Undefined`, read by the fxui frame: no global flag. In it the editor owns a `TooltipWindow` and DawIt's look and feel, the guide opens as the overlay, the host-track lookup answers "This track", and the overview gets the shared paragraph. One fxui helper opens a guide (window or overlay) for `EffectEditor`, `EqEditor` and `CompressorEditor`.
- **Threading:** unchanged; processors are the same code. JUCE modules compile once per plugin target (JUCE's module rule); shared DawIt code is compiled per plugin, accepted for an opt-in build.
- **Risks:** the CMake refactor of every built-in's source lines (the app must build the same); hidden dependencies of fxui on app code (found by the plugin link); hosts that don't run a plugin's audio while stopped, so Clap may be silent (checked in a real host, not claimed in text).

## Libraries and models
No new library: JUCE's plugin client (VST3 SDK bundled with JUCE 8.0.9, Steinberg licence or GPLv3; AudioUnitSDK, Apache 2.0). Tools for checking, not shipped: `auval` (part of macOS) and pluginval (Tracktion, GPLv3), used if installed; pluginval goes on *How it's made* if used.

## Theory
No music theory. The facts the README, idea and texts state:

**Facts**
- Logic Pro hosts Audio Units only, not VST (Apple's Logic Pro manual; support.apple.com/102082 lists AUv2 and AUv3). Exact.
- VST3 folders: macOS `~/Library/Audio/Plug-Ins/VST3` (user), `/Library/Audio/Plug-Ins/VST3` (all users); Windows `C:\Program Files\Common Files\VST3` (Steinberg VST 3 developer portal, *Plug-in Locations*). Exact.
- AU folder `~/Library/Audio/Plug-Ins/Components` (JUCE's default AU copy dir, JUCEUtils.cmake; Apple). Exact.
- AU plugin code: exactly one capital; GarageBand 10.3 wants the first letter capital, the rest lower case; manufacturer code at least one capital (JUCE *CMake API.md*). Exact.
- VST3 SDK 3.8.0 (October 2025) is MIT; earlier SDKs are Steinberg licence or GPLv3; JUCE 8.0.11 bundles 3.8 (Steinberg forum announcement; JUCE 8.0.11 release notes). The SDK in JUCE 8.0.9 states the dual licence in its LICENSE.txt (checked in `external/JUCE`). Exact.
- AudioUnitSDK in JUCE is Apache 2.0 (its LICENSE.txt). Exact.
- GPLv3 §13 and AGPLv3 §13 allow combining code under the two licences in one work (GNU licences). Exact.
- `auval -v aufx <subtype> <manufacturer>` validates an installed effect AU (`aumu` for an instrument) (Apple's auval). Exact.

**Texts** (ready to paste)
- Overview paragraph in plugins, heading "Part of DawIt": "This is <name> from DawIt, a free app for learning music theory while you make music. Here it works like any plugin: the knobs, the starting points and the guide are the same as in the app. In DawIt it also sees the song around it: it names the track it's on, checks the level going in and out, and the guide's last page looks at your track."
- *Your track* in a plugin: the track is called "This track"; the page's advice works from the settings, as for a stand-in.
- Guide button tooltip: "Opens the guide to <name> inside this window, with pictures that follow your settings." Back: "Back to the knobs and pictures (Esc)."
- README *License note* replacement for its first paragraph's last sentence: "DawIt's effects and synths also build as VST3 and AU plugins. The VST3 SDK bundled with JUCE 8.0.9 is licensed under Steinberg's VST3 licence or GPLv3, and DawIt uses it under GPLv3, which may be combined with AGPLv3 code; from version 3.8 (bundled with JUCE 8.0.11) it is MIT. The Audio Unit SDK is Apache 2.0."

**Test numbers**
| Check | Expected |
|---|---|
| Plugin codes | 4 characters, first a capital, the other three lower case letters, all different |
| Manufacturer code | `Dwit` |
| Registry vs `BuiltInFormat` | every effect and instrument registered once; kind matches `isInstrument` |
| App-only | exactly DawIt Tune |
| Parameter text | for 5 values per parameter (0, 0.25, 0.5, 0.75, 1): text not empty, and text of value-for-text equals the text |
| Parameter ids | version hint at least 1, unique per processor |
| Layouts | effects accept mono→mono and stereo→stereo; instruments stereo out with no input and accept MIDI |
| State | blob from one instance loads into a fresh one with every parameter within 1e-6 |
| DawIt copies | a VST3 or AU description with manufacturer "DawIt" is hidden from the add menus; DawIt built-ins and other makers' plugins are not |

## Steps
Group A, registry and seams (`./build.sh --test` at step 4):
- [x] 1. Move `guideprint::Source` into its own header with no `Engine.h`/`TheoryGuides.h`; `fxui::Guide` gets its Export button from a hook `MainComponent` installs. Check: the app's effect guides still show Export PDF. Done when: no file in the fxui frame or the built-ins' editors and guides includes `Engine.h` (except Tune's, app-only).
- [x] 2. Write `cmake/DawItBuiltIns.cmake` with `dawit_add_builtin` and the shared plugin source list, and replace every built-in's lines in `DawIt` and `DawItTests` with one call each (codes from decision 5; Tune `APP_ONLY`). Generate `DawItBuiltInList.h`. Done when: the app and tests build the same as before.
- [x] 3. A pure function next to the plugin add menus that says whether a description is one of DawIt's own plugin copies (VST3 or AU, manufacturer DawIt); `plugins::createAddMenu` skips them. Tests: the *DawIt copies* row.
- [x] 4. Registry tests (`runPluginRegistryTests`): every row of *Test numbers* except DawIt copies, over the generated list. Fix any parameter text that fails the round trip. Done when: `./build.sh --test` passes.

Group B1, plugin targets (`./build.sh --test --plugins` at step 6):
- [x] 5. `src/plugin/PluginEntry.cpp.in` and the `juce_add_plugin` part of `dawit_add_builtin` behind `DAWIT_BUILD_PLUGINS`; `build.sh` gets `--plugins` and `--install-plugins` (copy after build). Check: 13 `.vst3` and 13 `.component` bundles in the build folder. Don't run `--install-plugins`.
- [x] 6. Plugin mode in the fxui frame (and `EqEditor`, `CompressorEditor`): tooltip window, look and feel, "This track", the overview paragraph. Done when: the plugin build links with no warnings (a link error names a hidden app dependency: cut it, don't add app files).

Group B2, the guide inside the window (`./build.sh --test --plugins` at step 8):
- [x] 7. The guide overlay through one fxui helper, used by all three editor kinds in plugin mode: fills the window, Back button, Esc, reopens on its page. DawIt's windows unchanged.
- [x] 8. `ScreenshotRunner`: a shot option that opens a built-in's editor as a plugin would (a processor with a plugin wrapper type, in a plain window) and optionally its guide overlay at a page.

Group C, check and docs:
- [x] 9. Real-app check: shots of DawIt Reverb and DawIt Subtractive as plugins (window; guide overlay at the first page and at *Your track*), and DawIt's add-effect menu. Must show: the "Part of DawIt" paragraph, Back button, no Export PDF, "This track"; the menu without duplicates. Run `auval`/pluginval only if the user has agreed to install; otherwise list the commands for the user in the report.
- [x] 10. Docs: README Features bullet and License note; docs/recipes.md *A built-in effect or instrument* (step 2 is now one `dawit_add_builtin` call, which also makes the plugin; how to build and install plugins); docs/architecture.md (built-ins: plugin mode, the registry); docs/ui-design.md (the guide overlay); docs/code-layout.md (`cmake/`, `src/plugin/`); docs/tests.md; DISTRIBUTION_TODO.md section 6 (what's done); FEATURE_IDEAS status and its licence line corrected.

## Tests
In `tests/EngineTests.cpp`, the table in *Theory*: codes, manufacturer, registry against `BuiltInFormat`, Tune the only app-only, parameter text round trip at 5 values, parameter ids, bus layouts, the state blob round trip for every registered built-in, and the DawIt-copies filter (hides a VST3 "DawIt Reverb" by DawIt; keeps the built-in and a VST3 by another maker).

## Website hand-off
- Features page: a section "Take DawIt's effects and synths into your DAW": the formats, which hosts were tested (only those the user tested), that the guide opens inside the plugin window; screenshot of DawIt Reverb's plugin window with the guide overlay (from step 9's shots) and, if the user provides one, a host screenshot.
- Main page: a list item under *3 Take it further*.
- How it's made: pluginval if it was used. Downloads wait for DISTRIBUTION_TODO.md.

## Left for later
- DawIt Tune as a plugin (needs a key and chord picker when there's no song).
- The host's track name in *Your track* (JUCE's `updateTrackProperties`, VST3 hosts).
- Mono-to-stereo layouts (Logic's Mono→Stereo slot), Standalone apps, CLAP, LV2, AUv3.
- Installer, installing from inside DawIt, Developer ID signing and notarisation (DISTRIBUTION_TODO.md).
- Exporting a track's chain as plugin presets; Export PDF from a plugin's guide.
- Upgrading JUCE to 8.0.11+ for the MIT VST3 SDK.

## Changes while building
- Step 3: the pure function is in `src/engine/OwnPluginCopies.h` (inline, header only), not in `PluginActions.cpp`, because the tests don't compile UI sources.
- Step 4: a real bug the round-trip test found: DawIt Drums' "bend time" parameter (held in ms) parsed "500 ms" as 0.5. Fixed in `BuiltInDrums.cpp` (`knobText`). The state test sets stepped parameters to a snapped value first, since a host would.
- Test row *Plugin codes*: "all different" is read as no two plugins share a code (Dadd has repeated letters within itself).

Group A hand-off (for Group B1):
- `cmake/DawItBuiltIns.cmake`: `dawit_add_builtin (<Name> DISPLAY_NAME KIND CODE VST3_CATEGORY PROCESSOR HEADER EDITOR EDITOR_HEADER ENGINE_SOURCES UI_SOURCES [APP_ONLY])` is called 14 times at the end of `CMakeLists.txt`, then `dawit_finish_builtins()`. It already stores every field per built-in as global properties `DAWIT_BUILTIN_<Name>_<FIELD>` (list in `DAWIT_BUILTINS`); step 5 adds the `juce_add_plugin` target there, reading those. It also defines `DAWIT_PLUGIN_MANUFACTURER_CODE` ("Dwit") and `_NAME` ("DawIt"). The shared plugin source list is not written yet (step 5).
- Not moved into a registration (still in the main lists, shared with other code): `BuiltInFormat.cpp`, `BuiltInSynth.cpp`, `StereoMeasure.cpp`, `ReverbKinds.cpp`, `Tune.cpp`, `Psola.cpp`, `Tuner.cpp`, `PitchTracker.cpp`, `EffectWindow.cpp`, `BuiltInSynthEditor.cpp`, `StereoScope` went with Stereo. Tune's UI sources `TunePictures/TuneGuide/TuneEditor` are in its (APP_ONLY) call.
- `DawItBuiltInList.h` is generated into `build/<cfg>/generated/` (`daw::builtinlist::entries`, `manufacturerCode`).
- `src/ui/GuideSource.h` holds `guideprint::Source` and `guideprint::exportButtonFactory()` (an inline hook, empty in a plugin; `MainComponent` sets it). `EffectWindow.h` now includes `GuideSource.h`, not `GuidePrint.h`; no fxui frame file or built-in editor/guide includes `Engine.h` directly. Transitive check by the plugin link is still to come (step 6): `EffectWindow.cpp` includes `engine/Tempo.h`, `engine/EffectTools.h`.
- `src/engine/OwnPluginCopies.h`: `plugins::isDawItPluginCopy` (VST3/AudioUnit with maker "DawIt"), used by `createAddMenu`.

- Step 5: the plugin target is made by `dawit_add_plugin_target` (called from `dawit_add_builtin`), target names `DawItPlugin<Name>`, plus a `DawItPlugins` target. The shared plugin list `DAWIT_PLUGIN_SHARED_SOURCES` also holds the engine files of built-ins that others call into (`BuiltInEq`, `BuiltInCompressor`, `BuiltInSaturation`, `BuiltInSubtractive`, `Tune`) and `SubtractiveGuide.cpp` (its envelope picture is used by the other synths' guides); found by link errors. `dawit_add_builtin` also takes `PLUGIN_EXTRA_SOURCES` (unused for now). `--install-plugins` copies `.vst3` to `~/Library/Audio/Plug-Ins/VST3` and `.component` to `Components` (Linux: `~/.vst3`); not run. A clean `--plugins` build takes about 6 minutes.
- Step 6: `fxui::isPlugin`, `fxui::PluginMode` (look and feel and tooltip window; first member of `EffectEditor`, `BuiltInEqEditor`, `BuiltInCompressorEditor`) and `fxui::addPluginParagraph` are in `EffectWindow.h`. `getHostTrack` answers "This track" when no lookup is installed and the processor is a plugin. Departures: the paragraph says "This is Reverb from DawIt" (the name without its "DawIt " prefix, as "This is DawIt Reverb from DawIt" reads badly). `verb::kinds::analyse` knows the name `trackname::pluginTrackName` and gives one sentence instead of name-based advice ("This plugin can't see the name of the track it is on, so there is no advice for the kind of sound here. In DawIt, this page goes by the track's name: Vocal, Snare, Guitar and so on.") since "This track" would otherwise say it can't tell what the track holds: new text for dawTheory to see. The guide still opens in a window in a plugin until step 7.

Group B1 hand-off (for Group B2):
- Editors in a plugin: `fxui::isPlugin (processor)` says plugin mode. `EffectEditor::openGuide`, `BuiltInEqEditor::openGuide` and `BuiltInCompressorEditor::openGuide` each build a `fxui::GuideWindow` themselves (step 7's one helper replaces that). Plugin entry files are generated from `src/plugin/PluginEntry.cpp.in` into `build/<cfg>/plugin/`.
- `./build.sh --test --plugins` passes: All checks passed, no warnings, 13 `.vst3` and 13 `.component` bundles in `build/Release/DawItPlugin*_artefacts/Release/`. Nothing is installed.
- Not yet checked in a real host or window (step 9).

Group B2 hand-off (for Group C):
- Step 7: `fxui::GuideHost` (in `EffectWindow.h/.cpp`) is a member of `EffectEditor`, `BuiltInEqEditor` and `BuiltInCompressorEditor` (after `PluginMode`); `guideHost.show (title, makeGuide, printText, page)`. In DawIt it opens the old `GuideWindow`; in a plugin an overlay (`GuideHost::Overlay`: a 38 px bar with "Back to the controls", then the `Guide`) that fills the editor, follows its resizes, closes on Back or Esc, and is kept so it reopens on its page. The guide button can't toggle it back, since the overlay covers the button; Back and Esc do. The editors' `guideWindow` members are gone.
- Step 8: shot scene `"scene": "pluginEditor"` with `"builtIn": "DawIt Reverb"` (name or identifier), optional `"overlay": true` and `"page": n` (note: `"guide"` is the existing Theory Guides key, so it isn't used). It makes the processor with `setTypeOfNextNewPlugin (wrapperType_VST3)` and shows its editor in a plain window (`PluginEditorWindow` in `DevScreenshots.cpp`); capture with `"capture": "window"`. It switches the Export PDF hook off while the guide is built, so shots show no Export PDF as in a plugin.
- The editor window is about 1240 x 620, shorter than the guide's 1080 x 800 layout, so in the overlay the explanation text is short and scrolls (seen in Reverb's first page). Step 9 should check it reads acceptably; Guide's layout may need a shorter picture in a small window.
- `./build.sh --test --plugins` passes (All checks passed, no warnings).

Group C (steps 9-10):
- Shots (reverb window, guide at page 0 and at *Your track* page 16, Subtractive window and guide) looked at. Fixes: `Guide::resized` shrinks the picture in a short window (180 to 360 px) so the text keeps room; `getHostTrack` now answers "This track" for any plugin-mode processor before the app's lookup (the app's lookup, installed in a screenshot run, hid it).
- Not shown in a shot: the "Part of DawIt" paragraph (it ends the overview, below the scroll, and a shot's mouse position often hovers a topic instead) and DawIt's add-effect menu (no scene opens it; the test covers the filter). Not run: `auval`, pluginval (nothing installed). Commands: `auval -a`, then `auval -v aufx Drvb Dwit` (use `aumu` for Dsub, Dadd, Dfmx, Ddrm, Dphy), and `pluginval --validate-in-process <path>.vst3`, after `./build.sh --install-plugins`.
- Idea status: Partly done (no host test, no installer or signing, no Tune).

Theory check:
- `src/ui/EffectWindow.cpp` (`addPluginParagraph`): "the guide's last page looks at your track" was true only for Reverb, Gate and Stereo, and the track-name advice only for Reverb; now "its window names the track it's on, the track shows how loud the sound is at each step, and some guides, such as Reverb's, give advice by the track's name", plus "There you can also save a guide as a PDF" (decision 9).
- `src/engine/ReverbKinds.cpp`: "This plugin can't see the name of the track" became "As a plugin, DawIt Reverb doesn't know the name of the track it's on" (VST3 hosts can pass a track name; DawIt doesn't read it yet, see *Left for later*).
- README Features bullet: the *Your track* sentence now names Reverb's page only and says the plugins don't read the host's track name yet, rather than that a host doesn't tell it; Gate's and Stereo's pages measure the sound and work as in DawIt.
- README License note checked against `external/JUCE` (8.0.9, VST 3.7.12, LICENSE.txt Steinberg or GPLv3; AudioUnitSDK Apache 2.0) and the JUCE 8.0.11 / VST 3.8 MIT reports: correct, unchanged. SOURCES.md updated.
- Open: in a plugin, Reverb's `analyse` returns before the settings advice (mix, low cut, tempo), so the page doesn't "work from the settings" as *Theory* planned; the plugin's guide button keeps DawIt's tooltip, not the planned "Opens the guide to <name> inside this window".
