# Save and load your own plugin presets

Status: Released
Idea: [#119 — Save and load your own plugin presets](https://github.com/geertvandamme/dawit/issues/119)

## Goal
Save the settings of any effect or instrument as a named preset of your own, then load it on another track, in another song, or in DawIt's VST3/AU plugin in another DAW. Built-ins list your presets under their starting points. Third-party plugins get Save and Load in their chip's right-click menu. A beginner can keep a sound they built and learn from it later, without rebuilding it each time.

## Decisions for you
1. **Hosted third-party plugins.** Options: (a) built-ins only for now; (b) built-ins, plus hosted plugins through the chip and inspector right-click menu, matched to the same plugin in the same format, with a note that DawIt can't check what a plugin restores. **Recommended: (b).** The code is the same (an identity and the plugin's state), and the issue asks for it.
   Answer: (b) built-ins plus hosted plugins through the chain menu. (user's choice)
2. **Preset file format.** Options: (a) DawIt's own versioned JSON file (`.dawitpreset`): the plugin identity, the name, the date, DawIt's version and the plugin's state in base64; (b) the host formats (`.vstpreset`, `.aupreset`); (c) the raw state bytes. **Recommended: (a).** One readable format works for every plugin, in the app and in both plugin formats, and can say why it was refused. A host format belongs to one plugin format, and DawIt's built-ins in the app aren't VST3 or AU.
   Answer: (a) `.dawitpreset`. (recommendation accepted)
3. **Where presets are stored.** Options: (a) DawIt's data folder, next to Models and Voices: `~/Library/Application Support/DawIt/Presets/<plugin>/` (on Windows and Linux, the same sibling of the settings file); (b) `~/Library/Audio/Presets/DawIt/<plugin>/`, where hosts look for DawIt plugins' `.vstpreset` and `.aupreset` files; (c) a folder the user picks. **Recommended: (a).** It's DawIt's own folder, hosts won't list foreign files in their preset browsers, and *Show presets folder* finds it.
   Answer: (a) `~/Library/Application Support/DawIt/Presets/<plugin>/`. (user's choice)
4. **How DawIt's VST3/AU plugins share presets with the app.** Options: (a) one library: the app and every DawIt plugin, in either format and any host, read and write the same folder, and a built-in is identified by its identifier ("DawIt Reverb"), so a preset saved in the AU in Logic loads in DawIt and in the VST3 in Reaper; (b) separate libraries per host. **Recommended: (a).** The processors and their state are the same in all three. Caveat (unconfirmed): a sandboxed host may stop a plugin from reaching the folder. The plugin then says it can't read or write there, and import and export still work.
   Answer: (a) one shared library. (user's choice)
5. **The hosts' own preset formats.** Options: (a) keep them separate and say so in the text: the DAW's preset menu saves `.vstpreset`/`.aupreset` files that DawIt doesn't list, and DawIt's presets don't appear in the DAW's menu; (b) also import `.vstpreset` into hosted VST3 plugins (JUCE has `VST3PluginFormat::setStateFromVSTPresetFile`) and DawIt plugins' own host presets; (c) also export them. **Recommended: (a) now.** (b) and (c) go under *Left for later*.
   Answer: (a) keep them separate; (b) and (c) left for later. (recommendation accepted)
6. **Undo after loading.** Options: (a) none: loading replaces the settings, as picking a starting point does now; (b) a one-level *Undo load*. **Recommended: (a).** It matches the starting points. A/B snapshots (#59) are the better answer later.
   Answer: (a) no undo. (recommendation accepted)

## What the user does and sees
**In a built-in's window** (every `fxui::EffectEditor`, and the EQ and Compressor windows, which have their own frames). The header keeps the *Starting point...* dropdown, and a **Save...** button (about 64 px) sits right after it. The dropdown lists:
- the factory starting points, as now;
- a heading *Your presets*, with your presets for this plugin in alphabetical order (case-insensitive). If there are none, a disabled item says *None yet: use Save... to keep these settings*. A file that can't be read shows disabled as *Name (can't be read)*;
- a separator, then *Import preset file...*, *Export these settings...*, a *Manage your presets* submenu (one submenu per preset: *Rename...*, *Delete...*, *Export...*) and *Show presets folder*;
- last, after a separator, *About starting points and presets*. It shows the presets topic in the explainer.

Picking one of your presets loads it, shows its name in the dropdown as `showPresetName` does, and the explainer shows the user-preset text (below).

**Save...** and **Rename...** open an **inline bar** in the header, in place of the dropdown and button. It holds a text box (prefilled with the last picked name, or "My <plugin name without DawIt>" plus a number, such as "My Reverb 1"), then *Save* and *Cancel*. Enter saves and Esc cancels. The bar isn't a separate window, so it works inside a host too. When the name is already taken, the bar asks *Replace "Warm room"?* with *Replace* and *Cancel*. *Delete...* asks in the same bar: *Delete "Warm room"? This can't be undone.* with *Delete* and *Cancel*. Errors show in the bar in `meterRed` for 5 s, for example *Couldn't save "Warm room": DawIt can't write to its presets folder.* Import and export use `juce::FileChooser` (async). It starts in the user's Documents folder, with the `*.dawitpreset` filter.

**In the chain** (right-click a device bar chip or an inspector effect row: `plugins::showSlotMenu`, for built-ins and hosted plugins alike). A **Presets** submenu sits after *Bypass*. It lists your presets for that plugin, then *Save settings as preset...*, *Import preset file...* and *Export settings...*. The name prompt here is an `AlertWindow` with a text box, which is fine in the app. A hosted plugin whose `getStateInformation` returns nothing gets *Save* disabled, with the item text *This plugin reports no settings to save*.

**Rules**
- Factory starting points are code, never files, so nothing can overwrite them. A user preset may share a factory name, because it sits under its own heading.
- Names are trimmed and 1 to 60 characters long. The characters `/ \ : * ? " < > |` become `-`. Names are compared case-insensitively. An import whose name is taken gets " (2)", " (3)" and so on.
- Loading a preset that doesn't fit (another plugin, a damaged file, a newer format) changes nothing and says why. Example: *"Warm room" is a preset for DawIt Delay, not DawIt Reverb.*
- No shortcut: saving presets isn't done often enough to need one.

**Tooltips**
- *Starting point* dropdown: "Starting points made for this effect, then your own presets. Pick one to load its settings; the menu also saves, imports, exports and renames your presets."
- *Save...*: "Save these settings as a preset of your own, to load on any track, in any song, and in DawIt's plugins in other DAWs. Saving the project keeps this track's settings too, but only in this song."
- Chain *Presets* submenu: no tooltips, because menu items have none. The item names say what each does.

**Explainer, presets topic.** Each editor's existing text gets this common paragraph, added by the frame: "Starting points come with DawIt and never change. Your presets are settings you saved yourself. Pick one in the same list to load it, on any track and in any song, and in DawIt's plugins in other DAWs. A preset keeps the settings, not the sound: the same reverb preset sounds different on a voice and on a drum. Saving the project already keeps every track's settings, so save a preset when you want to use them again somewhere else. Your DAW's own preset menu is separate: it saves its own files, which this list doesn't show."

**When one of your presets was just picked**, the frame writes the topic itself. The title is the name. The lead is "Your own preset, saved on 3 November 2026." Then "Loading it set every control to what you saved; controls that didn't exist when it was saved are at their defaults." When the preset was saved with a different plugin version, the frame adds "Saved with version 0.1.0 of this plugin; it may sound a little different now."

Nothing new goes in the app settings.

## Data and saving
- Nothing changes in the project, and its format version stays. A loaded preset is ordinary plugin state, which `ProjectFile.cpp` already saves.
- **A preset file** is UTF-8 JSON, one per preset, in `<data folder>/Presets/<plugin folder>/<name>.dawitpreset`. The plugin folder is the built-in's identifier, or for a hosted plugin "<manufacturer> - <name> (<format>)", tidied as names are. The file holds a format tag and version 1, the name, the save date (ISO 8601), DawIt's version, the plugin identity and the state in base64. Its contents are checked on load: the folder is only for tidiness and is never trusted.
- **Identity.** A built-in is matched by its identifier alone, so the app's copy and the VST3/AU plugins match. A hosted plugin is matched by format name plus unique id, with name and manufacturer as a fallback when the id is 0. A VST3 preset doesn't load into the AU of the same plugin.
- **Versions.** A file with a newer format version is refused ("made by a newer DawIt"). A different plugin version loads, with the note above.

## The guide
No new guide or guide topics: there's no music theory here. The explanation lives in each built-in's explainer (the presets topic above) and in the tooltips.

## Code
Recipes: none fits exactly. It touches *A built-in effect or instrument* (the shared frame) and *A command or menu item* (the slot menu), and follows the folder pattern of `sing::getVoicesFolder`/`overrideVoicesFolder`.
- **`presets::`** (new, `src/engine/UserPresets.{h,cpp}`): UI-free and free of `Engine.h`. It goes into `DawIt`, `DawItTests` and `DAWIT_PLUGIN_SHARED_SOURCES`, so the plugins share it. It holds the preset (identity, name, date, versions, state), JSON in and out with validation that returns plain error texts, the identity of a processor or `PluginDescription` and the matching rule, the library (the folder with a test/screenshot override, list, save, rename, delete, import, export) and *apply*.
- **Apply** decodes and checks everything first, and only then calls `setStateInformation`. On any failure it changes nothing. For a built-in, parameters missing from the preset get their default values before applying. `AudioProcessorValueTreeState::replaceState` otherwise keeps the current value of a missing parameter, so an old preset would sound different depending on what was set before.
- **Writes** go through a temporary file that replaces the target (`juce::TemporaryFile`), so a failed write or a reader in another host never sees half a file.
- **Threading.** File work runs on the message thread (the files are small), never on the audio thread. Built-ins apply on the message thread without the engine lock, as starting points do: their parameters are atomics. Hosted plugins apply under `Engine::getLock()`, so the audio thread skips blocks while a third-party plugin changes state. A large state may give a short gap in the sound.
- **The shared preset picker** (new, in `fxui`): the dropdown, *Save...* and the inline bar. `EffectEditor`, `EqEditor` and `CompressorEditor` all use it in place of their `presets` combo box, and `setPresetNames`/`applyPreset`/`showPresetName` keep working. It needs the plugin identity from the processor and works the same in plugin mode.
- **Risky parts:** the header change touches the 16 frame-based editors at once plus the EQ and Compressor windows (check that each header fits), and the plugin link (the new file must not pull in app code).

## Libraries and models
None. JUCE only.

## Theory
None: no music theory. The text states four technical facts:
- The VST3 user presets on macOS live in `~/Library/Audio/Presets/<company>/<plugin>/`, and on Windows in `Documents/VST3 Presets/<company>/<plugin>/`. Source: Steinberg VST3 developer portal, *Preset Locations*. Exact.
- AU presets are `.aupreset` files in `~/Library/Audio/Presets/<manufacturer>/<plugin>/`. Source: Apple TN2157. Exact. Neither path is printed in the app's text: they back decision 3 only.
- A plugin's settings are what it returns from `getStateInformation`. Some plugins keep part of their sound outside it (samples, libraries). Source: JUCE `AudioProcessor` docs. Typical: it varies by plugin, so the text says "some".
- `replaceState` keeps current values for parameters missing from a state. Source: JUCE 8.0.9 source, `juce_AudioProcessorValueTreeState.cpp`. Exact, and the reason for the defaults rule.

Hosted-plugin note, shown in an `AlertWindow` the first time a hosted preset is saved, and in the chain's Presets submenu header text: "DawIt saves what the plugin reports as its settings. Most plugins restore all of it. Some keep part of their sound elsewhere (samples, a library, a licence) and may not sound the same. Listen after loading."

## Steps
Group A, engine (tests target at step 3):
- [x] 1. Add `presets::` with the preset, its JSON, validation, identity and matching (`src/engine/UserPresets.{h,cpp}`). Add it to `CMakeLists.txt` (`DawIt`, `DawItTests`) and to `DAWIT_PLUGIN_SHARED_SOURCES` in `cmake/DawItBuiltIns.cmake`. Tests: `runUserPresetTests` file and identity checks. Done when the file round trip and the refusals pass.
- [x] 2. Add the library: the folder (a `Presets` sibling of the settings file, plus an override), list, save (with replace), rename, delete, import (with " (2)"), export, the name rules and atomic writes. Tests: library checks, with the override pointing at a temporary folder. Done when the write-failure checks give their error texts.
- [x] 3. Add apply: check first, defaults for missing built-in parameters, nothing changed on failure. Tests: every built-in from `DawItBuiltInList.h`, the defaults rule, and the wrong-plugin and damaged-file refusals. Done when the tests target builds without warnings and passes.

Group B, UI (`./build.sh --test` at step 7):
- [x] 4. Make the shared preset picker in `src/ui/EffectWindow.{h,cpp}` (dropdown sections, *Save...*, inline bar, import/export choosers, *Show presets folder*, the frame's presets text and the user-preset text) and use it in `EffectEditor`. Check: the Reverb window saves, lists and loads a preset.
- [x] 5. Use the picker in `EqEditor` and `CompressorEditor` (their header layout), and add the common paragraph to their presets text. Check: the EQ saves and loads.
- [x] 6. Add the *Presets* submenu to `plugins::showSlotMenu` (`PluginActions.cpp`), with the AlertWindow name prompt, hosted loads under the lock, alerts for errors, Save disabled when the state is empty, and the hosted note.
- [x] 7. In `ScreenshotRunner`, point the presets folder at a scratch folder every run. Add the shot keys `userPresets` (save the named factory starting points of the shot's plugin as user presets under given names), `userPreset` (load one) and `presetBar` (`"save"`, `"replace"` or `"delete"`: open the inline bar), and document them in `DevScreenshots.h`. Done when `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 8. Real-app check. Shots: DawIt Reverb on Vocals with `userPresets` "Warm room" and "Big hall" and `userPreset` "Warm room" (the header shows the name, *Save...* fits, the explainer shows the user-preset text with the date); the same with `presetBar` "save" and "replace" (text box and buttons fit, nothing clipped); DawIt EQ and DawIt Compressor with a loaded user preset; DawIt Subtractive (instrument window header); the `pluginEditor` scene for DawIt Reverb (the picker in plugin mode). Then run `./build.sh --plugins` (don't install) to prove the plugins link with `UserPresets.cpp`.
- [x] 9. Docs. Add a README Features bullet near the built-in effects (save, load, import, export, shared with the plugins) and a *License note* check (unchanged). Add the `UserPresets` and picker lines to `docs/code-layout.md` and the checks to `docs/tests.md`. In `docs/architecture.md`, add a *Saving* bullet on the presets library and identity, and in *Built-ins* note that the plugins share it. In `docs/recipes.md` (*A built-in effect*), say that presets come with the frame and that a new state must stay restorable with defaults. In `docs/ui-design.md` (*Effect windows*), describe the header with *Save...* and the inline bar. Post #119 progress, and add a note on #91 (presets shared with the plugins; host formats still open).

## Tests
In `runUserPresetTests` (`tests/EngineTests.cpp`), all on a temporary library folder:
- Save three distinct DawIt Reverb presets (Mix 10, 30 and 60 %). The list has 3 in alphabetical order. Each loads into a fresh instance with those Mix values (within 0.001).
- A second library object on the same folder lists the same three, as a new session would.
- For every built-in in `DawItBuiltInList.h`, save the state of a changed instance and apply it to a fresh one: the state bytes are equal after `stabiliseState`-style settling.
- A Reverb preset applied to DawIt Delay is refused with a message naming both plugins, and Delay's state bytes are unchanged.
- Refused, with the state unchanged: an empty file, non-JSON, a missing state, bad base64, format version 99 and another format tag.
- The defaults rule: remove one parameter from a saved Reverb state, set that parameter away from its default on the target, apply, and it reads its default.
- Hosted identity, using a made-up `PluginDescription` for VST3 and AU with the same name: the VST3 preset matches the VST3 and not the AU. A unique id of 0 falls back to name and manufacturer.
- Names: "  Warm/room:1 " becomes "Warm-room-1", "" is refused, 61 characters are refused, and "warm room" is the same as "Warm Room" (replace needed). Importing "Big hall" twice gives "Big hall (2)".
- Rename and delete work. Renaming to a taken name is refused. The factory preset counts (such as `eq::getPresets()`) are unchanged.
- Write failure: with the library folder set to the path of an existing ordinary file, save returns an error text and writes nothing. A failed export leaves no partial file.
- A plugin version differing from the saved one still loads and reports the difference.

## Website hand-off
- `features.html`: a line in the effects and synths group (save your own presets, use them in any song and in the VST3/AU plugins).
- No new guide, so nothing for `learn.html`. Not for Home.
- Screenshot: the DawIt Reverb window with *Your presets* loaded (shot list: `"addPlugins": ["DawIt Reverb"], "track": "Lead vocal", "plugin": "DawIt Reverb", "userPresets": {"Warm room": "Small room", "Big hall": "Drum room"}, "userPreset": "Warm room", "capture": "window"`), if the features page shows that window. `"presetBar": "save"` shows the inline name bar.
- Facts for the text: presets are `.dawitpreset` files in `DawIt/Presets/<plugin>/`; DawIt's VST3/AU plugins share that folder with the app (a preset saved in the AU loads in the app and the VST3); hosted plugins are saved from the chip's right-click menu and DawIt can't check what a plugin restores; a DAW's own preset menu stays separate. No new library or model (nothing for `about.html`).

## Left for later
- Importing `.vstpreset`/`.aupreset` (hosted VST3 through JUCE, and DawIt plugins' own host presets) and exporting them (decision 5).
- A preset browser shared across all plugins, with tags and search (#91).
- Saving a whole chain as one preset, and duplicating a track (#130).
- A/B snapshots and undoing a load (#59).
- Presets for the DawIt Synth's fixed sounds: they have no settings to save.
- Remembering which preset a slot came from in the project.

## Changes while building

**Group A hand-off (engine, built and tested).** `src/engine/UserPresets.{h,cpp}`, namespace `daw::presets`, in `DawIt`, `DawItTests` and `DAWIT_PLUGIN_SHARED_SOURCES`. Tests: `runUserPresetTests`, all passing, no warnings.
- Identity: `builtInIdentity (identifier)`, `hostedIdentity (PluginDescription)`, `identityOf (AudioPluginInstance&)` (built-in when the description's format is empty or "DawIt", by `getName()`, so it also works inside a plugin), `matches`, `Identity::getDisplayName()/getFolderName()`.
- File: `makePreset (name, identity, state)`, `toJson`, `fromJson`/`readFile` (a `ParseResult` with a plain `error`), `writeFile` (atomic), `tidyName`, `checkName` (empty string when fine, otherwise the reason).
- Library: `presets::Library` (default constructor uses `getPresetsFolder()`, which honours `overridePresetsFolder (File)` for ScreenshotRunner; `Library (File)` for tests). Methods: `list` (entries with `name`, `file`, `readable`, `error`, `saved`, `dawItVersion`, `pluginVersion`; unreadable files are listed with their error, files for another plugin are left out), `find`, `save (id, name, state, replace)` (`Result::nameTaken` when taken and not replacing; failure `message` is the full sentence for the bar), `rename`, `remove`, `load`, `importFile (targetIdentity, file)` (refuses another plugin, " (2)" for a taken name), `exportTo (id, name, file)` and static `exportState (id, name, state, file)` (for "Export these settings...").
- Apply: `presets::apply (const Preset&, AudioPluginInstance&)` returns `ApplyResult {ok, message, versionDiffers, savedVersion}`. It does not lock: Group B must hold `Engine::getLock()` for a hosted plugin. For a built-in with APVTS-style state it adds missing `PARAM`s with defaults first.
- Differences from the plan: the "same state bytes" test for every built-in compares parameter values within 1e-5 instead of bytes, because logarithmic parameters drift by a few millionths (Reverb low cut) and never reach byte equality; `stabiliseState` is in `SongTemplates.cpp`, not shared. A listed preset's name comes from the name inside the file (keeps its case on a case-insensitive disk). The plugin version for a built-in is DawIt's version (`JUCE_APPLICATION_VERSION_STRING`, or `JucePlugin_VersionString` in a plugin).
- For the UI: the date for the user-preset text is `Entry::saved` (ISO 8601; parse with `juce::Time::fromISO8601`). Messages for refusals from `apply`/`importFile` are already whole sentences.

**Group B hand-off (UI, built; `./build.sh --test` passes with no warnings).**
- `fxui::PresetPicker` (`src/ui/EffectWindow.{h,cpp}`) is the dropdown plus *Save...* plus the inline bar (kinds `Bar::save/rename/replace/deleteIt/error`; the bar is 540 px wide, drawn over the guide button and what follows while open, and errors close it after 5 s). `EffectEditor::presets` is now a `PresetPicker` (same name), so `setPresetNames`/`showPresetName`/`applyPreset` work as before. The header now gives it 270 px (`setNormalBounds`); `BuiltInEqEditor` 260, `BuiltInCompressorEditor` 250, `BuiltInSpectralFreezeEditor` 250 (it lays out its own header). Group C must look at all headers (title and guide button positions moved right by 70 or 60 px).
- Frame text: `addPresetsParagraph` and `addUserPresetText` (in `EffectWindow.cpp`) give the common paragraph and the user-preset topic. `EffectEditor::getText` appends the paragraph to every editor's presets topic and writes the user-preset topic itself; a new `CommonTopic::presetsAbout` is the "About starting points and presets" item. `EqExplainer` and `CompressorExplainer` got `showUserPreset` and `showAboutPresets` and add the same paragraph to their presets text. A user preset's explainer text stays until a starting point is picked.
- Loading a user preset: `PresetPicker::loadUserPreset` (library load, then `presets::apply`, no lock, since built-ins). Import also loads the imported preset. Save replaces only after the *Replace "x"?* bar. Rename keeps the shown name in step.
- Slot menu (`PluginActions.cpp`): *Presets* submenu after *Bypass* with your presets, *Save settings as preset...* (AlertWindow with a text box; disabled with the "reports no settings" text when the state is empty), *Import preset file...*, *Export settings...*. Hosted loads and state reads hold `engine.getLock()`. Replace in the chain asks through an alert. The hosted note shows once per run after a hosted save, and the submenu has a header line for hosted plugins ("Saves what the plugin reports as its settings").
- Differences: the picker is a component of its own, not code in `EffectEditor`, so Eq and Compressor share it; `fxui::PresetHost` (`getPresetPicker`, `applyStartingPoint`) lets the shot runner reach it. Starting points are found by name ignoring a menu suffix, so "Small room" finds "Small room (Room)". A plugin-mode editor needs keyboard focus in the name box: not checked in a host.
- For Group C, shot keys (documented in `DevScreenshots.h`; every run points the presets folder at an empty scratch folder `dawit-screenshot-presets`): on a shot with `"plugin"` + `"track"` (or the `pluginEditor` scene with `"builtIn"`): `"userPresets": {"Warm room": "Small room", "Big hall": "Drum room"}` (new name: factory starting point to save), `"userPreset": "Warm room"` (load it), `"presetBar": "save"|"replace"|"delete"` with `"presetBarName"` (default "Warm room"). Reverb starting points on the Lead vocal track work (`"addPlugins": ["DawIt Reverb"]` first). One shot already looked at: the Reverb header with "Warm room" and the user-preset text fit. Not yet looked at: the bar shots, EQ, Compressor, Spectral Freeze, Subtractive (1400 wide), `pluginEditor`.

**Group C hand-off (real-app check and docs, built).** All shots looked at (scratch `g1`/`g2` folders): the Reverb header with a user preset, the inline *save*, *replace* and *delete* bars, DawIt EQ, Compressor, Subtractive (1400 wide), Spectral Freeze and the `pluginEditor` scene for Reverb: every header fits, nothing clipped or overlapping, the explainer shows the user-preset topic with the date and the common paragraph. No layout fixes were needed. The only change: the `DevScreenshots.h` example named a starting point that doesn't exist ("Cathedral"); it now says "Small room". Starting point names that work in shots: Reverb "Small room", "Drum room"; Compressor "Neutral start"; EQ "Vocal"; Subtractive "Bass"; Spectral Freeze "Dark drone". Subtractive and Spectral Freeze need `"addPlugins"` first. Docs done: README bullet (*Keep your own presets*), `docs/code-layout.md`, `docs/tests.md`, `docs/architecture.md` (Saving and Built-ins), `docs/recipes.md` (item 8) and `docs/ui-design.md`. SOURCES.md already had the four preset sources. Not checked: keyboard focus in the name box inside a real host, and a sandboxed host writing to the folder (the plugins only link here, they weren't installed or loaded).
