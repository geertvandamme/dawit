# Recipes: the files each kind of feature touches

For each kind of feature: the existing one to copy, and every place it has to be wired in. Read only the recipe you need; [architecture.md](architecture.md) says how the parts fit together, and [ui-design.md](ui-design.md) how they look. The rules in CLAUDE.md still apply (style, threading, text, tests). When a feature adds a new kind of thing or changes how one is wired, update its recipe here in the same change.

Every recipe ends the same way: new `.cpp` files in `CMakeLists.txt` (`DawIt`, and also `DawItTests` for engine, generate and analysis code), a `run<X>Tests()` in `tests/EngineTests.cpp` called from `main`, and the docs ([Docs](#docs-readme-and-the-ideas-list)).

## Contents

- [Song data that's saved](#song-data-thats-saved)
- [A song row above the tracks](#a-song-row-above-the-tracks)
- [A theory guide](#a-theory-guide)
- [Topics in an existing guide](#topics-in-an-existing-guide)
- [A built-in effect or instrument](#a-built-in-effect-or-instrument)
- [A command or menu item](#a-command-or-menu-item)
- [A window](#a-window)
- [A song template](#a-song-template)
- [A long job](#a-long-job)
- [Checking in the real app](#checking-in-the-real-app)
- [Docs: README and the ideas list](#docs-readme-and-the-ideas-list)
- [The website](#the-website)

## Song data that's saved

Copy: the lyrics (`Lyrics.h`, `Lyrics.cpp`, `Engine::getLyrics` / `setLyrics`).

1. **The type** in `src/engine/<X>.h` (and `.cpp`), with `operator==`, and pure edit functions in a namespace next to it (`lyrics::setText`, `lyrics::tap`, `lyrics::tidy`).
2. **Engine:** a member, a getter, and a setter that tidies the value and calls `sendChangeMessage()` only when it changed. Clear it in `resetEdit()`.
3. **Saving** in `ProjectFile.cpp`: write it in the JSON (leave defaults out), read it back with a default when the key is missing.
4. **Tests:** the pure functions, and a save-and-load round trip; an older project without the key loads with the default.

## A song row above the tracks

Copy: the Lyrics row (`LyricsLane`, `LyricsTrackHeader`), or the Sections row for a row without a header of its own (`SectionLane`, `RowLabel`).

1. **The lane** `src/ui/<X>Lane.{h,cpp}`: a `juce::Component` and `ChangeListener` on the engine, drawing on the shared `TimelineView`; `updatePlayhead()` repaints only the playhead's strip. A row that opens has `setExpanded`, `getPreferredHeight()`, an `onLayoutChanged` callback and remembers it in `view.<x>Open`. A `static` colour helper if the row's colour means something (`LyricsLane::colour`).
2. **Interaction** as in [ui-design.md](ui-design.md#interaction): click, double-click to edit, drag (Alt without snapping), right-click menu ending with the guide, Delete, copy and paste; a tooltip naming what's under the mouse.
3. **The header** in `ArrangementView.h`: a `RowLabel` that opens the guide, or a header class with an arrow and buttons (`LyricsTrackHeader`).
4. **ArrangementView:** members for the lane and header, their callbacks in the constructor, `layoutContent()` (header and lane bounds), `getSongRowsHeight()`, the list of rows hidden when folded (`setSongRowsFolded`), the playhead timer, and a `show<X>Guide()`.
5. **Folded rows:** a line in `FoldedSongRows` (`getLine` index, `paint`, the tooltip) and in `FoldedSongRowsHeader`'s legend.
6. **The data** if it's new: see [Song data that's saved](#song-data-thats-saved).

## A theory guide

Copy: the lyrics guide (`src/generate/LyricsGuide`, `src/ui/LyricsExplainer`), or the routing guide for a mixing topic (`src/engine/RoutingGuide`, `src/ui/RoutingExplainer`).

1. **Content** in `src/generate/<X>Guide.{h,cpp}` (`daw::gen::<x>`), or `src/engine` for sound and DSP topics, with no UI code:
   - `enum class Topic` and `allTopics[]`, with `getTopicName` and `getTopicSummary` (one line for the list);
   - `explain (Topic, song state...)`, which returns the text for a topic and may use the song's key, tempo, sections and so on;
   - example data for the pictures as plain structs (see `lyricsguide::Example`: tokens with beats, pitches, stress and rhyme);
   - the analysis of the user's own song (see `lyricsguide::analyse`, `structure`'s advice, the chord guide's own-track explanation), for the "your song" topic, which every guide has.
2. **Opening it:** a button that opens the guide is a `GuideButton` (icon only for "?", else icon then label) and its menu item is added with `guideicon::addItem`, never a plain text button.
2. **Window** in `src/ui/<X>Explainer.{h,cpp}`: the layout in [ui-design.md](ui-design.md#guide-windows); a `Picture` drawn with `juce::Graphics` in the app's visual language; a click plays a sound where that helps (a chord, a rhythm, a tone); a `ChangeListener` so "your song" follows changes; a public `showTopic (Topic)`. A guide that can act on the song offers it (*Put on chord track*, *Set it up step by step*).
3. **PDF export:** the window is a `guideprint::Source` with a `guideprint::ExportButton` in its title row: `getPrintTitle`, `getPrintSource` (what the pictures follow), `getNumPrintTopics`, `showPrintTopic`, `paintPrintPicture`, `endPrinting`.
4. **Opening it:** from where its theory applies (a row label, a *?* button, the last item of the right-click menu) and from related guides, through `PluginWindowManager::show`, with a `SafePointer` to the window in the view that opens it.
5. **The Theory Guides window**, in `src/generate/TheoryGuides.{h,cpp}`: add it to `Guide` and `allGuides`, and write its `Entry`: its area (or a new `Area`), window title, a one-sentence summary, `opensFrom`, search `keywords`, and its topics from its own list (`getTopicName` over `allTopics`) so they can't drift apart. A guide for beginners in general may go on the *Start here* path (`getStartPath()`).
6. **Opening at a topic and exporting:** a case in `MainComponent::openGuide` and `MainComponent::getPrintGuide` (or `ArrangementView::showGuide` / `getPrintGuide` for the song rows' guides, `DeviceBar` for gain staging).
7. **Tests:** names, schemes and numbers; every topic has text and an example; the advice on a made-up song. `runTheoryGuidesTests` checks every guide has an entry with its topics; add a search check for the words it should be found by. `--export-guides <folder>` must still pass.
8. **Sources:** every page and search behind the guide's facts goes under its heading in [SOURCES.md](../SOURCES.md).
9. **CLAUDE.md** isn't touched for a guide: the guides are listed in `gen::guides`.

**The text** is for a beginner: what it is, what it's for, why it works, and typical values ("a vocal usually sits about 3 dB above the band"); examples before rules, short paragraphs, terms explained the first time they appear. The "Your song" topic says concretely what's good and what to try ("the chorus lines are 3 syllables longer than the verse's") and follows changes. No claims that aren't true: facts with numbers are checked in the tests or come from a standard or a well-known source.

## Topics in an existing guide

1. Add the topic to the guide's `Topic` and `allTopics` (the order is the reading order), with its name, summary, `explain` text and example data.
2. Draw its picture in the window's `Picture` (a `switch` over every `Topic`, no `default:`).
3. The Theory Guides entry and the PDF follow by themselves; the tests' topic count changes.
4. Add the sources of any new or changed fact to the guide's heading in [SOURCES.md](../SOURCES.md).

## A built-in effect or instrument

Copy: DawIt Reverb for an effect (`BuiltInReverb`, `ReverbEditor`, `ReverbGuide`), DawIt Subtractive for an instrument. An instrument plays a test note instead of a clap and is listed by `BuiltInFormat::getInstruments()`.

1. **`src/engine/BuiltIn<X>.{h,cpp}`:**
   - a `juce::AudioProcessor` with an `identifier` ("DawIt X"), a `getDescription()` and parameters in an `AudioProcessorValueTreeState`;
   - starting points (presets) per instrument or use, each with why;
   - match level where appropriate, so before and after are compared at the same loudness; Reverb and Delay use Mix instead and have no match-level switch;
   - a level and history feed for the graphs, and the test sound (a clap or a note) from `EffectTools.h`;
   - pure functions for the curves and numbers the window draws, so they can be tested;
   - a `static std::function<juce::AudioProcessorEditor* (BuiltIn<X>&)> createEditorFunction`, with `hasEditor()` true when it's set.
2. **Register it** in `BuiltInFormat.cpp` (its description in the list, and a branch in `createPluginInstance`), and with one `dawit_add_builtin (...)` call at the end of `CMakeLists.txt` (`cmake/DawItBuiltIns.cmake`): name, display name, kind (effect or instrument), a four-letter plugin code with one capital first (permanent once anyone saves a session with it), VST3 category, processor and editor classes and headers, engine and UI sources (`APP_ONLY` keeps it out of the plugins). That one call adds the sources to `DawIt` and `DawItTests`, lists it in the generated `DawItBuiltInList.h` (the registry tests check it) and makes its VST3 and AU plugin. Rules for the plugin: its editor, explainer and guide must not include `engine/Engine.h`, `generate/TheoryGuides.h` or the app's UI (the plugin link fails with a missing symbol if one does: cut the dependency, don't add app files); the editor and guide host members (`fxui::PluginMode`, `fxui::GuideHost`) go before other members as in `ReverbEditor`; the overview calls `fxui::addPluginParagraph`. Build the plugins with `./build.sh --plugins` (several minutes: each target compiles JUCE) and copy them to the DAWs' folders with `./build.sh --install-plugins` (this changes what the user's DAWs see: ask first). Check with `auval -a` and `auval -v aufx <code> Dwit` (effects) or `aumu` (instruments), and with pluginval on the `.vst3`, or a shot of the editor with the `pluginEditor` scene.
3. **`src/ui/<X>Editor.{h,cpp}`**, a subclass of `fxui::EffectEditor` (`EffectWindow.h`): knob groups (`EffectKnob`), each with a topic the explainer shows when the mouse rests on it; the effect's own picture, draggable where it makes sense (a curve, dots on a response); the last 4 seconds of level; *Hear only what it adds* or *what it removes* where possible; `buildText`, `layoutPictures`, `applyPreset`, `createGuidePages`. Set `createEditorFunction` in `MainComponent`'s constructor.
4. **`src/ui/<X>Guide.{h,cpp}`:** the "What is X?" pages (`fxui::Guide::Page`) with live numbers for the current settings and drawings, and `setPrintSource` for the PDF. A page may supply its own clap callback and button text to audition an example without changing the processor's settings; pages without them use the guide's usual test sound. For *Your track*, use the shared `fxui` host-track lookup: `MainComponent` sets it beside the editor factories, finding the processor in a track's plugins under `Engine::getLock()` on the message thread. It returns the name and whether it is a track, bus or return; no host means the stand-in processor.
5. **The Theory Guides window:** an entry in `gen::guides` with `plugin` set to the identifier, a case in `MainComponent::openGuide` (`builtInGuides.open`), and branches for the processor in `BuiltInGuides` (`TheoryGuidesWindow.cpp`: opening the guide and `getPageNames`).
6. **The device bar:** correct loudness badges for gain staging (`GainStaging`); a small picture in the chip where it says something at a glance (only EQ and Compressor have one so far).
   - **An effect that holds the sound back** reports a constant latency (`setLatencySamples` in `prepareToPlay`, the same in every mode), implements `KeepsLatencyWhenBypassed` and delays in `processBlockBypassed`, so the track stays in time whether it works or not (copy `BuiltInTune`, or the dry delay line in `BuiltInSaturation`). The engine makes it up for audio tracks; add an impulse test as in `runTuneEngineTests`.
   - **An effect that follows the song** implements `SongContextReceiver` (`setSongContext` gets the key's and the bar's chord's pitch classes on the audio thread). The engine finds the interface when the plugin is added.
7. **Tests:** measure the sound against expected numbers (levels, times, harmonics, curves); check the starting points and the text of the controls; `checkThroughProject<BuiltIn<X>>` saves it in a project and loads it back.

## A command or menu item

1. A `CommandIDs` value in `MainComponent.h`, then `getAllCommands`, `getCommandInfo` (name, description, a shortcut when it's used often, ticked state), `perform`, and the item in `getMenuForIndex`. Items that open a window end with "...".
2. Call `commandManager.commandStatusChanged()` when a ticked state changes (see `foldSongRowsCommand` and `onSongRowsFoldedChanged`).
3. Track commands that the headers, mixer and device bar share go in `TrackActions`, not in each view.

## A window

1. Open it with `pluginWindows.show (key, title, [] { return new <X> (...); })`, where `key` is the object it belongs to (a guide's view, a plugin), and keep a `juce::Component::SafePointer` to it so the next open brings it to the front.
2. A resizable window passes a `minimumSize`; guides have a fixed size ([ui-design.md](ui-design.md#windows)).
3. Transport keys keep working through the manager's key listener (`TransportShortcuts`).
4. A zoomable, scrollable map or diagram window copies `SignalFlowView`: a pure graph in `src/engine`, one painter in graph units that the window, a guide's picture and the PDF share, its own zoom, Fit and pan, and settings under `view.<name>...`. A view that must stay read-only takes the tracks as `const` and reads the falling levels, not the stage peaks.

## A song template

Copy `gen::templates` in `SongTemplates.{h,cpp}` and `NewFromStyleWindow`. Keep the style table and the deterministic plan UI-free and test them in `DawItTests`. Turning a plan into a song uses only `Engine` functions for tempo, key, sections, chords, tracks, clips, plugins, buses, returns and sends; it adds no template-only project data. A File-menu template goes through `ProjectManager::newProjectFrom`, so the old project gets the usual save question and the result is an edited, untitled project. Draw previews with a shared UI painter that the chooser, guide and PDF use, and add `style:<id>` plus the chooser's scene to `ScreenshotRunner`.

## A long job

Copy: `TempoActions` (`TempoDetector`) for an analysis, `StemActions` for one that needs a model.

1. The work as a pure function in `src/analysis` or `src/engine` that takes audio and returns plain data, and checks a "should exit" flag; tested on made-up audio.
2. A `<X>Actions.{h,cpp}` class in `src/ui`, owned by `ArrangementView`: copy what it needs from the edit under the lock, run on a `juce::ThreadWithProgressWindow` with a cancel button, then show the result and change the edit on the message thread when the user accepts.
3. A model folder the user installs themselves (a DiffSinger voice) is checked before it's used: the check names the missing file, and a folder that fails it is removed again (`sing::loadVoice`, `sing::installVoice`). The window says how to get one when there is none.
4. A model that's downloaded goes through `ModelDownload` (fixed revision, size and SHA-256, asked before the first download); see [architecture.md](architecture.md#libraries-and-models) for the rules on new libraries.

## Checking in the real app

UI work is checked in the real app, not only in tests, with a shot list and the `--screenshots` developer flag (`ScreenshotRunner`, `src/ui/DevScreenshots.{h,cpp}`). No code is patched and nothing is rebuilt for it:

1. Write a shot list in your scratchpad (the format is in `DevScreenshots.h`): the song (`"pop"`, the example song from `examples::buildPopSong`, is the default), then a shot per picture, each saying what to select, open or zoom to, and what to capture. A shot may also set app settings first (`"settings": { "singing.lowest": 45, "singing.highest": 69 }`, written to the scratch settings; `null` removes one). For example:

   ```json
   { "folder": "shots", "shots": [
       { "file": "arrangement.png", "select": "Kick", "chordsOpen": true, "lyricsOpen": true, "height": 520 },
       { "file": "mixer.png", "panel": "mixer", "fold": "Vocals", "capture": "bottomPanel" },
       { "file": "guide.png", "guide": "routing", "topic": "Your song's routing", "play": true, "beat": 32 },
       { "file": "compressor.png", "plugin": "DawIt Compressor", "track": "Drums" } ] }
   ```

2. Run it under a timeout, all the shots in one run: `perl -e 'alarm 120; exec @ARGV' build/Release/DawIt_artefacts/Release/DawIt.app/Contents/MacOS/DawIt --screenshots shots.json`. It prints a line per PNG and exits with 1 when a shot failed, saying why (an unknown track, guide or topic, with the names it knows).

   `"clipEditorMode": "score"` (or `"pianoRoll"`) switches the clip editor; `"scoreClef": "grand"` (auto, treble, bass, grand, guitar, bassGuitar) sets the selected MIDI track's clef; `"sheetMusic": "a4"` (or `"letter"`) writes the selected track's PDF and `<file stem>-page<n>.png` next to the shot (give it `"area": [0,0,10,10]`).

   `"chatMusician": "melody"` or `"chords"` opens that Generate mode on the selected MIDI track. `"chatFourBars": true` adds a scratch four-bar clip, `"chatClearChords": true` empties the scratch chord lane, and `"chatGenerate": true` with `"chatModel"` starts a local request. Later shots can use `"chatApply": "midi"`, `"chords"` or `"undo"`.

   A guide shot may set `"clap": true` to play its selected page's test sound before `"wait"`; use this to capture moving pictures at a repeatable moment.
   The `pluginEditor` scene (`"builtIn": "DawIt Reverb"`, optional `"overlay": true` and `"page": n`; use `"capture": "window"`) opens a built-in's editor as a plugin host would, in a plain window with no track, and its guide overlay at a page.
   Use `"addPlugins": ["DawIt Reverb", "DawIt Stereo"]` with `"track"` to add built-ins that are missing. A DawIt Stereo or DawIt Tune plugin shot may set `"preset"` to a starting point's name and `"clap": true` to play its test sound (Wide bass, an off note). The `recording` scene adds an audio track "Voice" armed on Input 1 with a stack of three takes (the newest playing); a scene runs after the shot's `select`, `fold` and `panel`, so give it a shot of its own and select the track in the next; the `audioSettings` scene opens the Audio / MIDI Settings panel. The `soundClip` scene adds an audio track "Example clip" with the sound guide's made-up example clip, selected and open in the wave editor zoomed to its first two seconds, so What is sound? measures it at 1.0 s (use it before a `guide: sound` shot with `keepWindows`). The `tune` scene adds a "Sung take" track with DawIt Tune on it and plays the song through once offline, so the tuner has heard it. The `tuner` scene opens the Tuner on Guitar: standard with a made-up reading (`"hz"`, default 109.24: an A string 12 cents flat), so no microphone is needed; set `tuner.tuning` (for example `"bass4"`) with `"settings"`. A built-in guide shot with `"track"` opens the guide on that track's instance when present.

3. Look at every screenshot: text cut off or overlapping, wrong numbers, empty pictures.

The run uses settings of its own (`DawIt/Scratch`, emptied each time), so the user's `DawIt.settings` is neither read nor changed. When a shot needs a state the list can't express, add it to `ScreenshotRunner` for good (a setting, or a scene in `getSceneNames()`) rather than patching `src/Main.cpp`: the next feature can use it too. A new window that opens in its own way gets a scene; a new guide works by its key once it's in `gen::guides`.

## Docs: README and the ideas list

- **README.md:** a bullet under *Features* saying how to use it (where, which click, which keys) and what it shows; its checks in [tests.md](tests.md) under its area; its files in [code-layout.md](code-layout.md). The README is about 110 KB: find the section with Grep and edit it in place.
- **GitHub issues and the DawIt project:** follow [github-ideas.md](github-ideas.md) for searching open and closed ideas, adding or refining Markdown issues, linking plans and updating progress. An issue holds scope, decisions, acceptance checks and related issues; the project holds priority and status. Keep partly built work open, and record *Built, not committed* until the coordinator commits and completes it. The HTML idea lists and `tools/ideas.py` are legacy references and are not maintained by feature work.
- **CLAUDE.md** only when a feature changes a rule everyone follows; new patterns go in these docs.

## The website

Before website work, read [website/MAINTAINING.md](../website/MAINTAINING.md). It is the shared instruction source for Claude, Codex and ChatGPT: page placement, feature/guide markup, screenshots, styling, navigation and verification.

Maintain the seven-page site in `website/`. The former site has been replaced. The site has seven static HTML pages, `assets/site.css` and `assets/site.js`; it has no build step. Its guides are native cards in `learn.html`; its credits page is `about.html`. Preserve those current patterns rather than restoring the former overview's JavaScript guide picker.

Preview with the `website` configuration in `.claude/launch.json`, or `python3 -m http.server 8766 --bind 127.0.0.1 --directory website`. Reuse existing screenshots under the current instruction; the real-app screenshot recipe above applies only when the user authorises captures.
