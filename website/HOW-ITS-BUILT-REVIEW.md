# How it’s built: review and redesign

This review records work before the site's promotion to `website/` on 29 October 2026. References to the original site's files describe that historical version; the maintained seven-page site has since replaced it.

## Review

The previous `about.html` had the right raw material: human and AI contributions, development tools, libraries, model credits and licences. It was already present in the redesign, but only linked in the footer. Its long sequence of similarly styled credits gave visitors little help deciding what to read first. It also moved directly from people to tool names without explaining how the pieces contribute to a music app.

The redesign keeps the established `about.html` URL and `#models` link. It adds How it’s built to the main navigation before Get started, and updates the footer label on every page.

## Changes applied

| Previous issue | Change | Why it helps |
| --- | --- | --- |
| The page is easy to miss. | Main navigation link, consistent footer link and an active state on this page. | Visitors can find the development story directly. |
| Credits arrive before the story. | Lead with a human idea and a four-step development picture: question, tool, checks, explanation. | Explains the purpose before naming the technology. |
| AI coding and AI music tools can be confused. | Separate human direction, AI-assisted development and optional models inside the app. | Shows that everyday editing, guides and built-in effects do not require running a chat model. |
| The technical pieces are presented as a catalogue. | Add a simple explanation of song data, audio tools and live feedback. | Connects the technology to what someone hears and learns. |
| Readers must infer how models are obtained. | Put models beside the task they serve, with built-in, first-use download or separate-install labels. | Makes the practical requirements visible, including English vocals and a separately supplied voice. |
| An absolute claim says every dependency is checked by a hash. | Describe recorded versions and checksums where the project specifies them. | Matches CMake, which also uses a tagged JUCE checkout. |
| Local-AI wording hides the configurable Ollama connection. | State that the default server is on the user's computer and the address can be changed. | Describes the implemented setting accurately. |
| Verification sounds like an unqualified guarantee about every change. | Explain the checks the suite and review workflow perform. | Shows concrete evidence without claiming that every revision has already passed. |
| Full credits make the page long to scan. | Group all 23 existing attributions into four native expandable sections. | Keeps versions, makers, licences and links available at the right depth. |
| The credits section skips heading levels. | Use section headings followed by credit headings, with no h2-to-h4 jump. | Gives screen-reader and keyboard users a clearer outline. |

## Evidence reviewed

- Existing `website/about.html` and the original site's `made.html`.
- `CMakeLists.txt`: C++20, JUCE 8.0.9, ONNX Runtime 1.30.0, whisper.cpp 1.9.5, Basic Pitch, CMU dictionary and Bravura downloads, versions and checksums.
- `src/engine/Stems.cpp`: the Demucs model, pinned download and size.
- `src/analysis/LyricsFinder.cpp`: Small and Medium English Whisper models.
- `src/ui/GeneratePanel.cpp`, `src/ui/LyricsEditWindow.cpp` and `src/generate/OllamaClient.h`: the qwen2.5:7b default and configurable server address.
- `docs/architecture.md`: separation of song data, audio rendering, UI, guides and optional model work.
- Targeted `tests/EngineTests.cpp` sections and project review instructions: sound measurements, app inspection and theory sources.

No screenshot or logo was created or changed. All development credits, including Bravura from the current page, are retained. The original `website/` is unchanged.

## Further improvements proposed

These need information that is not currently supplied, so they are proposals rather than invented content:

1. **A named maker introduction.** A short paragraph with the maker's preferred name and music background would make the human side more personal. Publish only details they choose to share.
2. **A public source link.** Once the intended repository URL is supplied, add a “Read the source” link near the build story and on Get started. Do not derive a guessed repository from the product name.
3. **A link to verified build results.** If a public CI run or release-check page is available, link it from The checks. Keep a dated result distinct from a permanent claim that every version passes.

The page does not need more screenshots, a second navigation system, a framework or an animation to explain this story.
