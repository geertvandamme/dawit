# DawIt: instructions for building new features

DawIt (*DAW In Theory*) is a free desktop app for exploring music theory, trying out your own music, and growing into a full DAW. Everything in it explains itself to a beginner. These notes hold the rules every change follows. [README.md](README.md) describes what the app does, and [GitHub issues](https://github.com/geertvandamme/dawit/issues) list what could come next, with priority and status on the [DawIt project](https://github.com/users/geertvandamme/projects/2). Features are planned before they're built: each gets a plan in [plans/](plans/README.md), linked from its GitHub issue.

The how-tos are in [docs/](docs/), read when a task needs them:
- **Before website work**, read [website/MAINTAINING.md](website/MAINTAINING.md), the shared instructions for Claude, Codex and ChatGPT. Maintain the seven-page site in `website/`; the former site has been replaced.
- [docs/architecture.md](docs/architecture.md): what owns what, how a change travels from a click to the speakers and the screen, and the rules for libraries and models;
- [docs/recipes.md](docs/recipes.md): for each kind of feature (song data, a song row, a guide, a built-in effect, a command, a window, a long job), the example to copy and every file to wire it into; also checking in the real app, the ideas list and the website;
- [docs/ui-design.md](docs/ui-design.md): colours, fonts, window layouts, sizes and the interaction rules;
- [docs/code-layout.md](docs/code-layout.md) and [docs/tests.md](docs/tests.md): every file and what it holds, and what each test area checks.

## Summary

- **Stack:** C++20 and JUCE 8 (CMake, Ninja), macOS first, with Windows and Linux kept buildable through JUCE's cross-platform APIs. Other libraries are few, pinned and licence-checked: demucs.cpp with Eigen for stems, ONNX Runtime with Basic Pitch for audio to MIDI, whisper.cpp for lyrics, and Ollama over HTTP on the user's own machine for AI.
- **Layers:** `src/engine` (audio, the edit, files), `src/generate` (theory and AI) and `src/analysis` (chords, tempo, structure, lyrics in a recording) have no UI and are tested in `tests/EngineTests.cpp`; `src/ui` holds the components.
- **Every feature explains itself:** tooltips on everything, plain-language text, and, for anything with theory behind it, a guide with topics, a live picture and a "Your song" topic, listed in the *Theory Guides* window (`gen::guides`). Built-in effects and instruments are an engine processor, an `fxui` window and a "What is ...?" guide, with starting points, match level and tests that measure the sound. The parts and wiring are in [docs/recipes.md](docs/recipes.md).
- **Sources:** [SOURCES.md](SOURCES.md) lists every source of the theory guides, by guide. Any change to a guide's facts, numbers or text adds or updates its sources there in the same change.
- **Each user-visible feature also needs:** a README bullet under *Features*, its checks in [docs/tests.md](docs/tests.md), its files in [docs/code-layout.md](docs/code-layout.md), its progress in the GitHub issue and project, and a section on the website.
- **Text:** British spelling (colour, centre, analyse, normalise), short sentences, concrete examples with real numbers, and what something is for before how it works. No claims that aren't true: facts with numbers are tested or come from a standard or well-known source.

## Agents

Use the project's agents and skills rather than doing their job directly: `/daw-idea <idea>` adds an idea; `/daw-feature <idea>` (or `claude --agent dawFeature`) takes a feature from plan to release. Each agent's file in `.claude/agents/` says how it works. Follow [docs/github-ideas.md](docs/github-ideas.md) for GitHub issue search, plan links and project status. The HTML idea lists and `tools/ideas.py` are legacy references; don't maintain them as the backlog. `README.md` is large: find the needed section with Grep and read only that range. `tools/agent-usage.py` shows what the agents spent their tokens on.

## Building and testing

```bash
./build.sh --test
```

- It builds the Release app and `DawItTests` and runs the tests. It must end with `All checks passed` and print no compiler warnings.
- The flags include `-Wshadow`, `-Wswitch-enum` and `-Wfloat-equal`: don't reuse a member's name for a local or parameter; list every enum case in a switch instead of `default:`; compare floats with `juce::exactlyEqual` or a tolerance.
- New `.cpp` files go into `CMakeLists.txt`: in `DawIt`, and also in `DawItTests` for engine, generate and analysis code.
- Tests: each area has a `static void runXTests()` in `tests/EngineTests.cpp` that calls `check (condition, "what it checks")` and is called from `main`; they run headless, with the audio callback driven through a fake device.
- `./run.sh -- --demo` runs the app with example tracks; `--screenshot out.png` renders the window and quits; `--export-guides <folder>` saves every guide as a PDF (exit 1 on a failure); `--screenshots <shots.json>` takes a list of screenshots on the example song.

**Check UI work in the real app** with a shot list and `--screenshots` ([docs/recipes.md](docs/recipes.md#checking-in-the-real-app)), without patching or rebuilding. What a shot list can't reach is added to `ScreenshotRunner` for good, not patched into `src/Main.cpp`.
- `--screenshots` runs on scratch settings. For any other run (such as `--demo` by hand), copy `~/Library/Application Support/DawIt/DawIt.settings` aside before and back after. Never read its contents.
- Don't search the user's own project files or use Spotlight without asking.

## Code style

- **JUCE style:** Allman braces, 4 spaces, a space before the parentheses of calls and declarations (`setBounds (r)`), `camelCase` functions and members, `PascalCase` types, classes `final` unless meant to be subclassed, `override`, `noexcept` where it holds, `juce::` written out.
- **Namespaces:** everything in `daw`; theory and AI in `daw::gen` with a sub-namespace per guide (`daw::gen::lyricsguide`); shared effect UI in `daw::fxui`; helpers next to their data type (`daw::lyrics`).
- **Comments:** a `/** */` comment on every class and public function, saying in plain words what it's for and how it behaves for the user. Inline comments say *why*, briefly.
- **Units:** the timeline is in beats, the first bar is bar 0 (shown as 1), 4/4 for now. Audio clips keep their length in seconds. Levels are linear gain inside and dB in the text.
- **Colours** come from `Theme.h`. A colour that means something has one static helper used everywhere (`SectionLane::getColour`, `ChordLane::getColour`).
- **Non-ASCII text** goes through `juce::String::fromUTF8` or `juce::CharPointer_UTF8`. Keep a `juce::String` in a variable before taking its character pointer.

### Engine and data

- **Threading:** only the message thread changes the edit, holding `Engine::getLock()`; the audio thread only try-locks it. Mixer controls and transport state are atomics. Heavy work (loading, plugins, models) runs on a background thread with a progress window and a cancel button.
- **Tracks form a tree** (`Track::bus`, null: the master). Structure changes go through the engine's bus functions; views use `routing::getVisibleTracks` ([docs/architecture.md](docs/architecture.md#the-edit)).
- **Song data** sits in `Engine` behind a getter and a setter that tidies the value and calls `sendChangeMessage()` only on a change; views are `ChangeListener`s. Pure functions on the data live next to the type and are tested.
- **Saving:** `ProjectFile.cpp` writes indented JSON without default values; projects without the new data still load with sensible defaults; every saved feature has a save-and-load test.
- **App settings** (view state, last choices) go in `engine.properties.getUserSettings()` under keys like `view.lyricsOpen`; anything belonging to the song goes in the project.
- **Generating** notes into an existing track never changes its instrument or sound.

### UI

Follow [docs/ui-design.md](docs/ui-design.md). Always:
- click to select, double-click to edit, drag to move (Alt without snapping, Cmd to copy), Delete removes, Cmd+C/V/D where they make sense, and a right-click menu whose last item opens the guide;
- tooltips on every control and interactive area; over a timeline they say what's under the mouse ("Bar 9 · Chorus · Am");
- app-wide commands in `MainComponent`, with a shortcut when used often; menu items that open a window end with "...";
- windows open through `PluginWindowManager::show`, one per guide, coming to the front when opened again;
- text shrinks to fit before it's clipped; live pictures follow the playhead and the song, with small timer-driven repaints.

## Libraries, models and AI

JUCE is the only framework; prefer it and plain DSP over a new library. A new dependency is pinned with a hash, has a compatible licence, and is named in the README's *License note* and on the website's *How it's made* ([docs/architecture.md](docs/architecture.md#libraries-and-models)). Models and AI run on the user's machine: nothing from the user's music leaves it.

**Keep `LICENSE` current in the same change.** DawIt is `AGPL-3.0-only`. Whenever a library, model, font, dictionary, asset, SDK or tool is added, upgraded, replaced or removed, check its exact version's licence and transitive dependencies. Update the root `LICENSE` component inventory and the required complete copyright, licence and NOTICE texts in `licenses/`; update shipped copies, source obligations and the README's *License note* as applicable. This includes tools used to build or package DawIt when their terms require notices or their code/output is redistributed; merely using an editor or reading a source is not a new dependency. Keep third-party terms distinct and never alter the verbatim AGPL text or silently relicense DawIt. Follow the maintenance procedure in `docs/architecture.md` (*Libraries and models*).

Ideas and plans record the licence checks and implementation tasks for proposed dependencies; the inventory describes dependencies actually adopted. Any agent or skill that adopts one must make these updates before reporting the change complete, and include the licence files and outstanding obligations in its hand-off.

## Docs and website

- A feature that adds a new kind of thing, or changes how one is wired, updates its recipe, the architecture or the UI design doc in the same change.
- Website content and design follow [website/MAINTAINING.md](website/MAINTAINING.md). The Claude `dawWebsite` agent and Codex `daw-website` skill use that same guide. Maintain the seven-page site in `website/`; the former site has been replaced. Describe the app as it is: never call anything new or say it was added.

## Commits

- Commit only when asked. The app (code, tests, README, docs, ideas) and the website go in separate commits.
- The subject line is a sentence on what the user gets: "A lyrics track you time by dragging or tapping, and a guide to writing lyrics"; "Website: the lyrics track and the guide to writing lyrics".
- Leave `.claude/` out, and don't push unless asked.
