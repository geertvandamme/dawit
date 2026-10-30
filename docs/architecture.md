# How DawIt's code fits together

What owns what, how a change travels from a click to the speakers and back to the screen, and where each kind of code lives. Read it before planning a feature, instead of tracing the code. It names files and classes; the code itself is the reference for the details. For step-by-step lists of the files a kind of feature touches, see [recipes.md](recipes.md); for how the UI looks, see [ui-design.md](ui-design.md).

## Layers

| Folder | What's in it | May use |
|---|---|---|
| `src/engine` | The edit (tracks, clips, song data), audio, plugins, saving, built-in effects and instruments, the sound and mixing guides' content, models (stems, audio to MIDI) | JUCE, no UI |
| `src/generate` | Music theory and AI: chords, keys, harmony, chord styles, deterministic song templates, the song guides' content, play-along and sing-along timing, Ollama | engine types, no UI |
| `src/analysis` | Finding things in a recording with plain signal processing: chords (`ChordFinder`), tempo (`TempoFinder`), structure (`StructureFinder`), lyrics (`LyricsFinder`, with Whisper) | engine types, no UI |
| `src/ui` | Components and windows | everything |
| `src/Main.cpp` | The app: makes the `Engine` and the main window, handles the developer flags | |
| `tests/EngineTests.cpp` | Headless tests of engine, generate and analysis code | not `src/ui` |

Engine, generate and analysis code goes in both CMake targets (`DawIt` and `DawItTests`); UI code only in `DawIt`.

## Who owns what

```
DawItApplication (Main.cpp)
├── Engine                      the edit, audio, plugins, settings
└── MainWindow
    └── MainComponent           commands, menus, the guides opened from menus
        ├── Selection           selected tracks and clip (a ChangeBroadcaster)
        ├── PluginWindowManager one window per key (plugin, guide, editor)
        ├── BuiltInGuides       opens built-ins' "What is ...?" guides from the Theory Guides window
        ├── TransportBar
        ├── ArrangementView     ruler, song rows, track headers and lanes
        │   ├── TimelineView    beats ↔ pixels, shared by the ruler and lanes
        │   ├── Ruler, TempoLane, SectionLane, ChordLane, LyricsLane, FoldedSongRows (+ their headers)
        │   ├── TrackHeader ×n, TrackLanes, TrackActions (the track menu, shared with mixer and device bar)
        │   ├── background jobs: StemSplitter, AudioToMidiConverter, LyricsDetector, TempoDetector,
        │   │   ChordDetector, StructureDetector, ChordStylePlayer
        │   └── SafePointers to its guide windows: chords, structure, lyrics, play-along, sing-along
        ├── InspectorPanel (+ InspectorEdge)   the selected track's effects, sends, pan and volume, left of everything under the transport bar
        ├── DeviceBar           the selected track's chain as chips, gain staging badges and guide
        ├── PianoRoll (+ GeneratePanel, ScoreView), WaveEditor, MixerView   the bottom panel shows one of these; the clip editor has two modes, piano roll and score
        ├── ProjectManager      New, New from Style, Open, Save, recent files, ask to save
        └── SafePointers to guide windows: sound, MIDI, rhythm, styles, routing, Theory Guides
```

`Engine` (`src/engine/Engine.h`) also holds the app-wide services: `deviceManager`, `knownPlugins` and `properties` (the app settings, `engine.properties.getUserSettings()`).

## The edit

- **Tracks** (`Track.h`): `Type` audio, midi, bus or fxReturn (`hasInput()`: a bus or a return); name, colour, atomics for volume, pan, mute, solo and arm; `clips`; `plugins` (`PluginSlot`: an instance, bypass and level atomics). A MIDI track without an instrument plays the built-in synth (`builtInSound`).
- **Buses** form a tree: `Track::bus` is the bus a track plays into (null: the master). `Engine::getTracks()` is the edit order, each bus followed by everything in it. `routing::` (`Routing.h`) answers questions about the tree: visible rows (`getVisibleTracks`), depth, why a track is silent (`getSilence`), where a drag lands (`Place`). Changes go through the engine: `addTrack`, `addBus`, `createBus`, `moveTrack`, `removeFromBus`, `ungroup`, `setFolded`, `removeTrack`.
- **Returns and sends**: a return (`Type::fxReturn`) hears the sends of other tracks and plays into the master only; returns sit last in the edit order and in `getRenderOrder`. `Track::sends` (`Send`: target, `levelDb`, `preFader`, peak) is structural, written through `Engine::addReturn`, `setSend` and `removeSend`; the level and pre-fader flag are atomics the UI writes directly. In `Track::process` a send is taken after the plugins (pre-fader) or after the track's smoothed fader gain but before the pan, and added into the return's `busInput`; it is silent when the track or a bus above it is muted (`mutedAbove`) or another track is soloed and neither the track's solo path nor the return is. `renderSegment` clears every `busInput`, and a return is on the solo path when a sender on it is. Saved as `"type": "return"` and `"sends"` (version 3).
- **Clips** (`Clip.h`): `AudioClip` (audio in memory, peaks, gain, length in seconds) and `MidiClip` (notes in beats). Engine functions add, move, split, join, duplicate and remove them.
- **Song data**, which makes no sound but guides the tools and views: `MusicalKey`, `TempoMap` (tempo changes by beat), `ChordMap` (a chord per bar, `Chord.h`), `SectionList` (`Section.h`), `Lyrics` (lines of timed syllables, `Lyrics.h`). Each has a getter and a setter on `Engine`; the setter tidies the value (`tidySections`, `lyrics::tidy`) and calls `sendChangeMessage()`. Pure edits live next to the type: `lyrics::`, `chordEdits::` (`ChordEditing.h`).
- **Positions** are in beats, bar 0 first, 4/4. Audio clips keep seconds, so where they end in beats follows the tempo map.

## How a change travels

1. **A click** in a view calls an engine function on the message thread. The engine takes `getLock()`, changes the edit, and calls `sendChangeMessage()`.
2. **Views** are `juce::ChangeListener`s on the engine (and on `Selection`) and repaint. When the whole edit is replaced (load, New), `getEditGeneration()` changes and views rebuild their rows. Note edits call `notesChanged()`, which tells the audio thread to release held notes.
3. **The audio thread** (`Engine::audioDeviceIOCallbackWithContext` → `processBlock`) only try-locks; if the UI holds the lock it skips that block. It splits the block at tempo changes (`renderSegment`) and renders each track in `renderOrder` (every track before the buses it plays into) with a `RenderContext`: audio clips, MIDI clips into the built-in synth or the plugin chain, then its sends, volume and pan, into its bus's `busInput` or the master mix, and last the returns. Then the master volume, the metronome, the sound guide's tone and the Hits to MIDI audition buffer (`renderAudition`, ended by the transport playing or a bounce), then the speaker mode (stereo, mono or left minus right) and the meters.
4. **Meters and graphs** read atomics the audio thread writes (`Track::peakLeft`, `PluginSlot::inputPeak`, `masterPeakLeft`...) on a UI timer, and repaint only the small areas that change (`updatePlayhead()`). Two kinds of level: the stage peaks (`PluginSlot::inputPeak/outputPeak`, `Track::faderInputPeak`, `Send::peak`, `masterInputPeak`) are swapped to 0 by the device bar when it reads them, so only one view may read them; the falling levels (`updateMeterLevel`: `peakLeft/Right`, `busInputLevel`, `Track::chainInputLevel`, `PluginSlot::outputLevel`, `Send::level`, `masterPeakLeft/Right`) take nothing away when read, so any number of views can read them. The Signal Flow window uses only the falling levels.
5. **Computer keyboard notes** enter as MIDI input: `ComputerKeyboardTyping` (message thread) turns plain letters into note messages and sends them with `Engine::addMidiInput`, like a MIDI keyboard's, so previewing, the scale filter and recording need nothing of their own.
6. **Transport and mixer controls** are atomics set straight from the UI (`play()`, `setPositionBeats`, `Track::volumeDb`).

Bouncing renders the same path offline: `beginOffline`, `renderOffline`, `endOffline` (`Bounce.cpp`, `BounceDialog`). A `BounceFocus` (set under the lock before `beginOffline`, cleared by `endOffline`, honoured only offline) says what plays: the mix; the selected tracks *heard as soloed*, where a per-block "counts as soloed" flag (`Track::countsAsSoloed`) replaces the solo buttons, which are never written; or *one chain*, one track's clips (or one clip) through its instrument and effects at unity gain, straight out, which is bounce in place (`BounceActions`). A MIDI track's effects latency is rendered extra and dropped.

**Recording audio.** The callback hands the input buffers to `processBlock`. An armed audio track records `Track::inputChannel` (mono or the stereo pair) into a per-track FIFO that `Engine::serviceRecording()` (the 30 Hz timer) drains to a crash-safe raw WAV through a `ThreadedWriter`; capture starts after the count-in. On stop the take is cut, moved earlier by `getLatencyShiftSamples()` (the device's reported latency plus the measured extra), trimmed of silence, saved as a 24-bit WAV in `getRecordingFolder()` and added as a clip; a loop pass that wraps starts a new take, stacked on the others, with only the newest playing (`daw::takes` in `Clip.h`). **Inputs on demand:** the device opens input channels only while `areInputsWanted()` (an audio track armed, or the settings panel's meter), and the start-up state has none, so a microphone is never opened for a bounce or a test. Monitoring runs the input through the track's chain in the callback; it is never saved. `LatencyMeasure.cpp` plays eight clicks through the device and finds them again (`findRoundTrip`) to measure the latency the driver leaves out, kept per device pair in the app settings. The UI side is in `RecordingActions` and `AudioSettingsPanel`. **The Tuner** uses the same inputs: while its window is open `setTunerListening (true)` makes `areInputsWanted()` true, and the callback copies the chosen channel into `tuner::InputTap`; `tuner::Listener` (in `Tuner.cpp`, driven by the window's timer) reads it with the pitch tracker's long window. Its mute-while-tuning flag silences only the armed track's monitoring, and only while listening.

## Saving

- **The project** (`.dawit`) is JSON written by `daw::project` in `ProjectFile.cpp`: tracks, clips, plugin states, key, tempo map, chords, sections and lyrics. Audio is copied into the project's audio folder and referenced by relative path. Default values are left out; a missing key loads as its default, so older projects load. `ProjectManager` (UI) does New, Open, Save and asks to save changes.
- **App settings** (view state, last choices) go in `engine.properties.getUserSettings()` under keys like `view.songRowsFolded` or `export.sheetInstrument`.
- **Song templates** (`gen::templates`, `SongTemplates.{h,cpp}`) are deterministic plans made from a style, key, tempo, seed and whether to write notes. Building one writes only ordinary edit data through `Engine`; `ProjectManager::newProjectFrom` keeps the empty untitled edit as the saved baseline so the made song is unsaved.

## Guides

- **Content** is UI-free and tested: `src/generate/<X>Guide.{h,cpp}` for the song's theory (`gen::guide` chords, `gen::structure`, `gen::stylesguide`, `gen::lyricsguide`, `gen::singing`, `gen::rhythm`) and `src/engine` for sound and mixing (`soundguide`, `midiguide`, `gainstaging`, `routingguide`). Each has `Topic`, `allTopics`, `getTopicName`, `getTopicSummary`, `explain (...)`, example data for the pictures and an analysis of the user's song.
- **Windows** are `src/ui/<X>Explainer.{h,cpp}`: a topic list, a live picture and the text (see [ui-design.md](ui-design.md#guide-windows)). The built-ins' "What is ...?" guides are pages of an `fxui::Guide` made in `src/ui/<X>Guide.{h,cpp}`.
- **The list of all guides** is `gen::guides` (`src/generate/TheoryGuides.{h,cpp}`): `Area`, `Guide`, `allGuides`, an `Entry` per guide, search and the *Start here* path. `TheoryGuidesView` (`TheoryGuidesWindow.cpp`) shows it.
- **Opening a guide at a topic** goes through `MainComponent::openGuide`: the sound, MIDI, rhythm and routing guides are MainComponent's own; the drum triggers guide is `MainComponent::showTriggerGuide`; chords, chord shapes, structure, lyrics and singing go to `ArrangementView::showGuide`; gain staging to `DeviceBar::showGuide`; built-ins to `BuiltInGuides::open` with the plugin's identifier.
- **PDF export:** each window is a `guideprint::Source` (`GuidePrint.h`) with an `ExportButton`. `getPrintGuide` in MainComponent, ArrangementView, DeviceBar or BuiltInGuides makes a guide for export when its window is closed; `GuideBooklet` lays out the pages and `PdfWriter` writes the PDF.

Where each guide opens from in the app is in its entry's `opensFrom` in `TheoryGuides.cpp`.

## Built-in effects and instruments

- **Processors** are `src/engine/BuiltIn<X>.{h,cpp}`: a `juce::AudioProcessor` with an `identifier` ("DawIt Reverb") and an `AudioProcessorValueTreeState`. `BuiltInFormat` is a plugin format that lists them (`getEffects`, `getInstruments`) and creates them, so they sit in plugin chains and save like any plugin.
- **Their windows** live in `src/ui` but the engine can't know the UI, so each processor has a static `createEditorFunction` that `MainComponent`'s constructor sets.
- **The shared frame** is `daw::fxui` in `EffectWindow.h`: `EffectEditor` (knob groups, the explainer panel on the right, starting points, the level history, the guide button), `EffectKnob`, `LevelHistoryView`, `Guide` and `GuideWindow`. DawIt EQ and Compressor are older and have their own windows (`EqEditor`, `CompressorEditor`) and explainers.
- **As plugins:** every built-in except Tune also builds as a VST3 and AU (`./build.sh --plugins`). The registry is the `dawit_add_builtin` calls in `CMakeLists.txt` (`cmake/DawItBuiltIns.cmake`), which also write `DawItBuiltInList.h` for the tests. Plugin mode is `processor.wrapperType != wrapperType_Undefined` (`fxui::isPlugin`), read by the fxui frame: the editor owns a tooltip window and DawIt's look and feel (`fxui::PluginMode`), `getHostTrack` answers "This track", the overview gets the "Part of DawIt" paragraph, and the guide opens as an overlay (`fxui::GuideHost`). Plugin sources never include `Engine.h`, `TheoryGuides.h` or the app's UI. DawIt's add menus hide its own copies (`plugins::isDawItPluginCopy`).
- **Levels:** effects that change tone or dynamics offer match level for a fair comparison. Reverb and Delay have no match-level switch: their Mix sets how much wet sound is added. Reverb's types are level-matched for comparison.
- **Shared sound tools** are in `EffectTools.h`: the level history and the hand clap.
- **Stereo measurements** are in `StereoMeasure.{h,cpp}`. The audio thread writes atomic history and sample rings; the editor, guide and device chip can each read them without consuming another reader's data. `StereoScope` shares the goniometer and correlation-bar painters.
- **Latency is made up for.** A plugin that holds the sound back reports it with `setLatencySamples`. An audio track reads its clips that much earlier (`Track::getCompensation`, the sum over its plugins; bypassed ones count when they implement `KeepsLatencyWhenBypassed` and delay in `processBlockBypassed`, as DawIt Tune, Saturation and Distortion do). After a start, seek, loop wrap or offline start (`RenderContext::restarted`, not after note edits) the track first feeds its chain the skipped samples and drops the output (`Track::preroll`), so what the plugins hold is what comes next. Buses, returns, MIDI tracks and live MIDI aren't compensated.
- **The song reaches plugins.** A plugin that implements `SongContextReceiver` gets the key's pitch classes and those of the chord in the bar at the start of the block before each `processBlock` (`RenderContext::chordPitchClasses`, read under the lock the audio thread holds, which is why `Engine::setChord` and `setChords` take it). The slot caches the interface when the plugin is added.
- **DawIt Tune** (`BuiltInTune`) finds the pitch with `pitch::Tracker`, decides the move with `tune::Corrector` and moves it with `tune::PsolaShifter`; it hands what it heard (`tune::Frame`) to the message thread through a FIFO that its own timer drains into the recent frames and the take log, which `gen::tunecheck` turns into advice.
- **The device bar** shows a chip per plugin; DawIt EQ, Compressor, Gate and Tune draw a small picture in theirs (`chainpictures`; DawIt Tune shows the note being sung and a needle). `GainStaging` gives the loudness badges.

## The score

`gen::notation::build` turns a MIDI track, the key, chords, sections and tempo into a `Score` (staves, bars, events, rests and ties, beams, line breaks) without any UI, and is tested. `scorepaint::drawSystem` (`ScorePainter`, Bravura glyphs) is the one painter: `ScoreView` (the clip editor's second mode, `view.clipEditorMode`), the pictures in `ReadingExplainer` and the PDF pages of `SheetMusicExport` all call it, with different ink and sizes. The score is read-only; the only saved score data is the track's clef (`Track::scoreClef`).

## Long jobs and models

- **The pattern:** an `<X>Actions` or `<X>Detector` class in `src/ui` (owned by `ArrangementView`) runs the work on a `juce::ThreadWithProgressWindow` with a cancel button, outside the engine's lock, then changes the edit on the message thread. Examples: `StemActions`, `AudioToMidiActions`, `HitsToMidiActions`, `ChordActions`, `TempoActions`, `StructureActions`, `LyricsActions`.
- **Models:** Basic Pitch is built in (`juce_add_binary_data`) and run by ONNX Runtime (`AudioToMidi`). Demucs (`Stems`, through demucs.cpp) and Whisper (`LyricsFinder`, through whisper.cpp) are downloaded once after asking, at a fixed revision checked by size and SHA-256 (`ModelDownload`), to `~/Library/Application Support/DawIt/Models`.
- **Voices (Sing this):** DiffSinger voices are not shipped. The user adds one (a .zip, a folder or a .oudep vocoder) to the **Voices** folder next to Models (`sing::getVoicesFolder()`, vocoders in `Voices/Vocoders`). `Voice.cpp` reads and checks a voice in OpenUtau's layout before it is used and names the file that is missing; `VoiceInstall.cpp` unpacks it. `DiffSinger` (a `Singer`) runs its ONNX models in order (duration, pitch, acoustic, vocoder) on the CPU with the threads set to the core count minus 2, and all models share `onnx::getEnvironment()` (`OnnxEnvironment.h`). The score (`generate/SingScore`) and the pairing of words to notes (`generate/SingPairing`) are UI-free; the tests drive them with a fake Singer, and `DawItTests --voice <folder>` runs a real voice. Words come from the voice's dictionary, then the CMU Pronouncing Dictionary embedded in the app (`DawItDictionary`).
- **AI generation:** `GeneratePanel` (beside the piano roll) builds a `GenerationRequest`; `PromptBuilder` asks the user's local Ollama (`OllamaClient`) for JSON; `NoteRealizer` turns the reply into notes. Writing notes into a track never changes its instrument or sound. `LyricsEditWindow` does the same for words: `gen::lyricsai` builds the request, schema and checks and `Thread::launch` runs the `OllamaClient` call off the message thread, but the window only changes the text box, which `Apply` then puts into the song.
- **ChatMusician:** the Generate panel snapshots a whole-bar MIDI clip and chord map, then `gen::chatmusician` makes and validates bounded JSON requests on a background thread. A take is staged until the user previews or applies it; chord-track and MIDI application are separate, and undo checks that the affected song state has not changed. Raw non-streaming Ollama requests can finish after cancellation; their results are discarded.

## Libraries and models

- **JUCE** (8.0.9, from `external/JUCE` or fetched) is the only framework: UI, audio, plugin hosting (VST3, and AU on macOS), files, JSON, HTTP and graphics. Prefer JUCE and plain DSP over a new library.
- **Bravura** (SIL OFL 1.1) is the font for sheet music, downloaded at a pinned commit with a hash and embedded as binary data (`DawItFonts`); its licence is shipped beside the app.
- **The others**, each fetched in `CMakeLists.txt` at a pinned URL: demucs.cpp with Eigen (stems), ONNX Runtime (Basic Pitch), whisper.cpp (lyrics from audio).
- **A new dependency:**
  - is fetched with `FetchContent` at a pinned URL with a `URL_HASH SHA256`;
  - has terms compatible with DawIt's `AGPL-3.0-only` release; check the exact code/model/asset licence and provenance rather than assuming the runtime's licence covers its content. GPLv3 code can be combined under the licences' compatibility provisions; non-commercial restrictions are not an open-source licence for DawIt's code;
  - updates the component inventory in the root `LICENSE`, with its version/revision, purpose, source, licence and the corresponding complete texts in `licenses/`, in the same change;
  - is named in the README's *License note*, and on the website's *How it's made* page with its version, licence, what it does in DawIt and a link (a model also with its size).
- **Models run locally:** small ones built in, large ones downloaded on first use after asking; nothing from the user's music leaves their machine.

**Licence maintenance applies to changes as well as additions.** For libraries, SDKs, models, fonts, dictionaries, assets and tools, inspect the exact revision's licence, copyright, NOTICE files and relevant transitive components. Record whether it is embedded, linked, downloaded, separately installed, or used only to build/package. Development tools need a licence entry when their terms require attribution or their code/output is redistributed, not merely because a maintainer uses them. A proposed dependency stays in its idea/plan until adopted.

Update only the project declaration or component inventory around the verbatim AGPL text in `LICENSE`; never edit the standard licence text or relicense a third-party component. Preserve upstream notices in `licenses/` with their provenance in `licenses/README.md`. On an upgrade, refresh notices from the actual version; on removal, remove only entries no remaining component needs. Update the README's *License note*, applicable binary/source packaging and the website hand-off together. Source availability obligations (including AGPL/GPL and MPL) are separate from supplying licence texts. If release packaging does not yet exist, record its remaining work in `DISTRIBUTION_TODO.md`; do not claim that checking in notices completes distribution compliance. Report the licence files changed and unresolved obligations with the feature.
- **AI:** prompts ask for JSON with a schema; replies become notes through tested code; checks that need no model run by default, and real model runs are optional (`--ollama <model>`); cloud models would be opt-in only.

## Developer flags and tests

- `--demo` fills the edit with example tracks; `--screenshot <file.png>` renders the window and quits; `--export-guides <folder>` saves every guide as a PDF and exits with 1 on a failure (in `Main.cpp`, done by `MainComponent`).
- `--screenshots <shots.json>` takes the screenshots in a list (`ScreenshotRunner` in `src/ui/DevScreenshots.{h,cpp}`, a friend of `MainComponent` and `ArrangementView` so it can set up their views), on its own scratch settings (`Engine (true)`). Its default song is `examples::buildPopSong` (`src/engine/ExampleSong.{h,cpp}`), the A minor pop song with sections, chords, lyrics and buses.
- `tests/EngineTests.cpp`: a `static void run<X>Tests()` per area, called from `main`, each calling `check (condition, "what it checks")`. The audio callback is driven by hand through a fake device. `checkThroughProject<Plugin>` saves a built-in in a project and loads it back.
