# Show each guide's sources in its last topic

Status: Released
Idea: [#6 — Show each guide's sources in its last topic](https://github.com/geertvandamme/dawit/issues/6)

## Goal
Every theory guide ends with a *Sources* topic: a short note on how far its numbers can be trusted, then the pages read for it as links that open in the browser, and the searches that shaped a fact. A curious or sceptical beginner can check a claim and read further without leaving the app, and the list can't drift from `SOURCES.md` because it is read from it.

## Decisions for you
1. **One source of truth.** Options: (a) embed `SOURCES.md` in the app as binary data and parse it at run time with a tested parser; (b) generate a C++ table from it in CMake at build time; (c) move the lists into each guide's data in `src/generate` and have a test check `SOURCES.md` agrees. **Recommended: (a)**: agents and people keep editing one Markdown file exactly as now, the parser is plain tested C++ (no CMake text processing), and the app shows the change at the next build. (c) doubles every edit; (b) puts a Markdown parser in CMake.
   Answer: (a) embed `SOURCES.md` and parse it at run time.
2. **Guides with no recorded sources yet** (13: Chords, Chord shapes, Structure, Gain staging, EQ, Compressor, Saturation, Distortion, Subtractive, Additive, FM, Drums, Physical; Rhythm and Delay are partial). Options: (a) every guide gets a heading in `SOURCES.md`, empty ones say so, and a test requires a heading per guide; the topic shows the "none recorded yet" text; (b) guides without a heading just show that text, no test; (c) research their sources first (a dawTheory pass per guide) before shipping. **Recommended: (a)**, with (c) as a follow-up issue: the gap stays visible in the app and the file, and a new guide can't be added without a heading.
   Answer: (a) a heading per guide, empty ones say so, a test requires a heading per guide; (c) as a follow-up issue.
3. **Agent and skill updates the issue asks for.** With 1(a) nothing extra must be copied into the guide: the Sources topic *is* `SOURCES.md`. The agents only need the new heading convention (a `Guide key:` line, below) and to know that notes under a heading are shown to readers. Files: `.claude/agents/dawPlanFeature.md`, `dawImplement.md`, `dawTheory.md`; `.codex/skills/daw-plan-feature` and `daw-implement` (the Codex/ChatGPT side; `website/MAINTAINING.md` needs nothing); `AGENTS.md`, `CLAUDE.md` (*Sources* bullet), `docs/recipes.md`. **Recommended:** make these edits in step 13, but only with your explicit go-ahead, since they change agent configuration and `CLAUDE.md`; `.claude/` stays out of commits as usual. If you choose 1(c) instead, the agents must also edit the guide's sources list in `src/generate` on every new source.
   Answer: yes, make the agent, skill, `AGENTS.md`, `CLAUDE.md` and `docs/recipes.md` edits in step 13 (the user gave the go-ahead); `.claude/` stays out of commits.
4. **Plain `http://` links.** One page (the CMU Pronouncing Dictionary, Singing) has no https version (checked: port 443 refuses). Options: allow `http://` only for links listed in the test, or drop the link. **Recommended:** allow it through a one-entry list in the test, so the issue's "every link starts with `https://`" holds for everything else.
   Answer: drop the CMU Pronouncing Dictionary link; every link must start with `https://`, no exception list.

## What the user does and sees
- In every guide window, after *Your song*, a last row **Sources** with the summary "12 pages read, 5 searches" (or "No sources recorded yet"). Same for the built-ins' "What is ...?" guides, in the app and in the plugin overlay.
- Selecting it replaces the picture and the explanation with one scrolling panel (same `panelLight` box, same margins, ui-design.md's guide window layout): the intro text (*Theory*), any notes from `SOURCES.md` for that guide, then **Pages read**: one link per line, shown without `https://`, shrunk to fit then cut with an ellipsis, its note (the text after the link in `SOURCES.md`) dim after it; then **Searched for**: one search per line in `textDim`, any result links indented under it.
- Click a link: opens in the default browser (`juce::URL::launchInDefaultBrowser`); the app sends nothing itself. Hover: underline in `accent` and the tooltip "Opens <full url> in your browser. Right-click to copy the link." Right-click: *Open in browser*, *Copy link*. A search line's tooltip: "Searched for this while writing the guide; the links under it are results that were used."
- The Theory Guides window lists *Sources* as each guide's last topic and opens a guide at it; the PDF export ends with a Sources page (text only, URLs written out).
- Nothing is remembered between sessions.

## Data and saving
None in the project or settings. `SOURCES.md` gains one machine-readable line under each guide heading, `Guide key: <key>`, using the guide keys `--screenshots` already uses (`sound`, `readMusic`, `stepFilter`, ...). Headings without it (*Loudness*, whose guide isn't built yet, and *Not for a guide*) are not shown anywhere. Mapping of the existing headings: Sound→sound, Recording→recording, Tuning→tuning, Singing→singing, Pitch correction→tune, Gate, Stereo, Routing, Reverb, Styles, Lyrics, MIDI→midi, Drum triggers→triggers, Reading music→readMusic, Slicer, Step filter→stepFilter, Key finding→keyFinding, Spectral freeze→spectralFreeze, Arpeggiator, Harmoniser, Delay, Rhythm.

## The guide
No new guide; one topic added to all 35 by shared code.
- **Sources**: "Where this guide's facts come from". Picture: none; the panel above takes the picture's and the text's place.
- Not a "your song" topic; it comes after *Your song*. Search finds it by "sources", "references", "where does this come from".

## Code
Recipes: *A theory guide* (steps 5 to 7, the shared parts) and *Topics in an existing guide*, but the topic is added by shared code, not to each guide's `Topic` enum.
- **The data:** `SOURCES.md` embedded with `juce_add_binary_data` (a `DawItSources` target), linked into DawIt, DawItTests and every built-in plugin (`cmake/DawItBuiltIns.cmake`).
- **The parser:** `gen::guidesources` in `src/generate/GuideSources.{h,cpp}`, depending on `juce_core` only, so the plugins can use it (they never include `TheoryGuides.h`). For a guide key it returns the notes, the pages (url, note) and the searches (text, result urls from `(results: <...>)`), plus the topic's summary line and its full text for the PDF. Pure, tested on a small made-up Markdown string and on the real file. Tolerates the existing variants (`**Searches**`, `**Pages read (the inspector)**`) and skips the file's own introduction.
- **Keys:** the `Guide`→key function moves from `DevScreenshots.cpp` into `gen::guides` so the shot runner and the windows share it. `guides::getEntries` appends "Sources" to every guide's topics; plugin-built guides get it from their window as now.
- **The panel:** one shared `src/ui` component shows a guide's sources, plus a small helper each guide window owns: it adds the last list row, swaps the picture and explanation for the panel on that row, opens at it for an index one past the guide's own topics, and gives the print topic. `fxui::Guide` takes a sources key and appends the page itself, so the 19 built-in guides only pass their key; the 15 `*Explainer` windows wire the helper in. `ChordExplainer` uses `chordShapes` when it shows chord shapes, `chords` otherwise.
- **Risk:** a wide, mechanical change across 15 windows' list models (row counts, selection, `showPrintTopic`, `getNumPrintTopics`). Keep each window's own topic switch untouched: the helper handles the extra row before it. `MainComponent::exportAllGuides` fails (exit 1) when a guide's last print topic isn't "Sources", so `--export-guides` checks every window.
- No audio-thread or lock involvement; parse once, lazily, on the message thread.

## Libraries and models
None.

## Theory
The only text is the topic's own; it states no external facts.

**Facts**
- Doubling the power adds 10 × log10 2 = 3.01 dB. Derived. Exact (used as the intro's example).

**Topic text: Sources**
- Name: `Sources`
- Summary (list row): "<n> pages read, <m> searches" (singular "1 page read", "1 search"; a part with 0 is left out), or "No sources recorded yet".
- Intro (shown first, and first in the PDF page):

  "Where the numbers in this guide come from.

  Worked out: some numbers follow from a formula. Doubling the power of a sound adds 3 dB, because 10 × log10 2 = 3.01. DawIt's own tests check many of these.

  Measured: what the pictures show about your song or an effect's sound is measured by DawIt as you watch. Its tests check those measurements on known test sounds.

  From the sources below: definitions, standards, history and typical values. A typical value is a usual range, not a rule: engineers and musicians often work outside it.

  The pages were read when the guide was written or checked. The searches are what was looked up when no single page was used. A page can move or change after it was read. Click a link to open it in your browser, or right-click it to copy it."
- With no sources recorded (replaces the last two paragraphs' list): "No sources have been recorded for this guide yet. It was written before DawIt began keeping a list. They will be added here when the guide is next checked."
- Headings in the panel: "Pages read", "Searched for".

**Notes in SOURCES.md, rewritten for readers** (step 1):
- Delay: "Only the doubling range on *Short delays* has sources so far; the guide's other topics were written before DawIt began keeping a list."
- Rhythm: "These sources cover the time-signature topics; the guide's older topics were written before DawIt began keeping a list."
- Each empty heading (decision 2a): "No sources recorded yet."

**Test numbers**
| Check | Expected |
|---|---|
| Guides with a `Guide key:` heading | 35 of 35 (decision 2a) |
| Unknown keys in `SOURCES.md` | 0 |
| Routing pages read (count the `- <https` lines under its heading) | equal to the file's count |
| Links not starting `https://` | 0, except the listed CMU link |
| Made-up text: 2 pages, 1 search with 2 results | 2 pages, 1 search, 2 result links, summary "2 pages read, 1 search" |
| Headings without a key (Loudness, Not for a guide) | not returned for any key |
| `getEntries`: every guide's last topic | "Sources" (plugin guides: checked by `--export-guides`) |
| Search "sources" | every guide |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. `SOURCES.md`: add `Guide key:` under each guide heading (mapping above), add a heading with "No sources recorded yet." for each of the 13 guides without one (named as their window titles), rewrite the Delay and Rhythm notes as in *Theory*, and update the file's introduction (the key convention; notes are shown in the app). Check: every guide key appears once.
- [x] 2. Embed `SOURCES.md` as the `DawItSources` binary data and link it into DawIt, DawItTests and the plugins. Check: `./build.sh --plugins` still builds.
- [x] 3. `gen::guidesources` parser, summary and print text (`src/generate/GuideSources.{h,cpp}`, in DawIt, DawItTests and `DAWIT_PLUGIN_SHARED_SOURCES`). Tests: the made-up text and the real-file checks in *Test numbers*.
- [x] 4. Move the guide key function into `gen::guides`, use it in `DevScreenshots.cpp`, append "Sources" in `getEntries`, add "references" to the search. Tests: every key round-trips, every entry ends with "Sources", the search check; update `runTheoryGuidesTests`' topic counts. Done when: `./build.sh --test` passes without warnings.

Group B1, UI: the panel and the built-in guides (`./build.sh --test` at step 7):
- [x] 5. The shared sources panel and the guide-window helper (`src/ui`): layout, link rows, tooltips, click to open, right-click menu, print topic. Check: in step 7's build.
- [x] 6. `fxui::Guide` takes a sources key and appends the Sources page (no picture, no test-sound button); every built-in guide maker passes its key. Check: DawIt Reverb's guide ends with Sources.
- [x] 7. The plugin overlay: the panel scrolls and links open from a plugin window. Done when: `./build.sh --test` and `./build.sh --plugins` pass without warnings.

Group B2, UI: the guide windows (`./build.sh --test` at step 10):
- [x] 8. Wire the helper into Sound, MIDI, Rhythm, Routing, Recording, Tuning, Drum triggers, Reading music (`*Explainer.{h,cpp}`), including opening at the topic index one past their own in `MainComponent::openGuide`.
- [x] 9. The same for Chords (and Chord shapes), Structure, Styles, Lyrics, Singing, Gain staging, Key finding (`ArrangementView::showGuide`, `DeviceBar::showGuide`, `MainComponent`).
- [x] 10. `MainComponent::exportAllGuides` fails when a guide's last print topic isn't "Sources". Check: `--export-guides <scratch folder>` exits 0 and a PDF's last page lists its links. Done when: `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 11. Real-app check, one `--screenshots` run: `guide: routing, topic: Sources`; `guide: reverb, topic: Sources`; `guide: chords, topic: Sources` (no-sources text); `guide: rhythm, topic: Sources` (the note and partial list); `pluginEditor` with `builtIn: DawIt Reverb, overlay: true` at the last page. Each must show the intro, readable links without overlap, the searches indented under their heading.
- [x] 12. Docs: README *Features* bullet in the guides section; `docs/tests.md` (theory guides area); `docs/code-layout.md` (`GuideSources`, the panel); `docs/ui-design.md` (the last topics are *Your song*, then *Sources*; the panel's look); `docs/recipes.md` *A theory guide* step 8 and *Topics in an existing guide* step 4 (the `Guide key:` line; the topic comes for free); `docs/architecture.md` *Guides* (one line on `guidesources`); GitHub #6 progress and the follow-up issue of decision 2(c).
- [x] 13. With the user's go-ahead (decision 3): one paragraph each in `.claude/agents/dawPlanFeature.md`, `dawImplement.md`, `dawTheory.md`, `.codex/skills/daw-plan-feature/SKILL.md`, `daw-implement/SKILL.md`, `AGENTS.md` and `CLAUDE.md`'s *Sources* bullet: a new guide's heading gets its `Guide key:` line; notes under a heading are shown in the app, so write them for readers; the Sources topic follows `SOURCES.md` by itself, so a new source only goes there.

## Tests
In `runTheoryGuidesTests` (or a new `runGuideSourcesTests`), the rows of *Test numbers*: the made-up Markdown parses into 2 pages, 1 search with 2 result links and the summary "2 pages read, 1 search"; a page's note is kept; `**Searches**` and `**Pages read (the inspector)**` are read as their lists; the real file has a heading for each of the 35 keys and no unknown key; Routing's page count matches the file; every link starts with `https://` except the one listed CMU link; Loudness and *Not for a guide* belong to no key; every `getEntries` entry ends with "Sources"; the summary of a guide with no sources is "No sources recorded yet"; the intro text contains "3.01". `--export-guides` covers the windows.

## Website hand-off
- `learn.html`: one sentence in the guides introduction: every guide ends with its sources, as links.
- `features.html`: a line in the theory guides group; no Home or `for-you.html` change.
- Screenshot worth taking: the Routing guide at *Sources* (from step 11). No `about.html` change.

## Left for later
- Researching sources for the 13 guides with none (a dawTheory pass each; a follow-up issue).
- Per-fact markers in the topic texts (which sentence comes from which source): needs a citation format in every guide.
- Checking that links still work (would need network access the app doesn't use).

## Changes while building

**Group A hand-off (engine)**
- `SOURCES.md`: 35 `Guide key:` headings (the 13 new ones say "No sources recorded yet."), the CMU `http://` line is gone, the introduction explains the convention. Embedded as `DawItSources` (`dawsources::SOURCES_md`), linked into DawIt, DawItTests and every plugin (`cmake/DawItBuiltIns.cmake`).
- `gen::guidesources` (`src/generate/GuideSources.{h,cpp}`, in DawIt, DawItTests and `DAWIT_PLUGIN_SHARED_SOURCES`): `getSources (key)` returns `Sources { found, notes, pages {url, note}, searches {text, resultUrls} }`; `getSummary (sources)` is the row text; `getIntro (hasSources)` the panel's intro (paragraphs separated by a blank line); `getPrintText (sources)` the PDF text. `parse (markdown, key)` and `getKeys` are the pure parts. A note that only says "No sources recorded yet." is dropped, so `isEmpty()` decides which intro and summary to show.
- `gen::guides` (`TheoryGuides.h`): `getKey (Guide)`, `findGuide (key)` and `sourcesTopicName` ("Sources"). `ScreenshotRunner::getGuideKey` is gone; `DevScreenshots.cpp` calls `gen::guides::getKey`.
- `getEntries()` appends "Sources" to every entry that has topics, and adds the keywords "sources references reference citations where this come from" to all. Guides with empty topics there (the plugin guides, and Chords and Chord shapes, whose window builds its own list) get the row from their window: group B must add it for those.
- `getEntry (g)` itself is unchanged (no Sources topic), so a guide window's own topic count still matches its `allTopics`.
- Tests: `runGuideSourcesTests` in `tests/EngineTests.cpp`. All pass, no warnings.
- Surprise: `./build.sh --plugins` stops at the link of DawIt Gate (AU): undefined `daw::harm::getPhraseNotes()` used from `BuiltInGate.cpp` (`daw::fx::Clap::getSungLineSample`). It is from the uncommitted Harmoniser work, not from this group; the app and tests build fine. The plugin builds otherwise compile with the new data (`libDawItSources.a` links). Re-check `--plugins` in group B1 once that is fixed.

**Group B1 hand-off (panel and built-in guides)**
- `src/ui/GuideSourcesPanel.{h,cpp}` (`daw::`; in DawIt and `DAWIT_PLUGIN_SHARED_SOURCES`; needs only `GuideSources.h`, not `TheoryGuides.h`). `GuideSourcesPanel` is the scrolling panel (`setKey`). `GuideSourcesTopic (parent, key)` is the helper for B2's windows: `GuideSourcesTopic::isRow (row, ownTopics)` (the row one past the guide's own topics), `paintRow (g, w, h, selected)` (same look as the other rows, 46 px), `show (bool, area)` (shows or hides the panel in `area`, a hidden child of `parent`), `getSummary()`, `getPrintTopic()` (no picture) and `getName()`. The window hides its own picture and explanation while `isShown()`; see `fxui::Guide` (`onSources()`, `showPage`, `resized`) for the pattern, and clamp `showPage` indexes to rows + 1.
- `fxui::Guide` takes a last constructor argument `sourcesKey`; with one, its list, print topics and `showPage (pages.size())` include Sources. Built-in guides get the key from `gen::guidesources::getKeyForPlugin (title)` (new: a table of the 19 window titles, tested), EQ and Compressor pass "eq" and "compressor" themselves.
- `DevScreenshots.cpp`: a guide shot's `"topic": "Sources"` now resolves for plugin-built guides too (`addIfNotAlreadyThere`). B2 must make `main.openGuide (guide, index)` open the other guides at the Sources row (`index == their own topic count`).
- Checked: `--screenshots` of `guide: reverb, topic: Sources` and of the plugin overlay (`pluginEditor`, `overlay`, `page: 99`) show the intro, links and searches, and the overlay scrolls. Not checked yet: link clicks and the right-click menu in the real app (not scripted), the empty-sources text.
- `./build.sh --test`: All checks passed, no warnings. `./build.sh --plugins`: only the known link error (`daw::harm::getPhraseNotes()` in the Gate, Stereo, Reverb and Delay AU/VST3 targets); everything else, including the new files, compiles.

**Group B2 hand-off (guide windows)**
- `GuideSourcesTopic` gained `select (host, list, ownTopics, keep)` and `close()`: Sources hides every visible child of the window except `keep` (list, title, export button, sometimes a hint label), restores them on `close()` (children are `SafePointer`s, so tiles a window deletes meanwhile are safe), and the window's `resized()` returns early with `sources.show (true, area)` after placing the list.
- All 15 Explainers (Sound, Midi, Rhythm, Routing, Recording, Tuning, DrumTrigger, Reading, Chord, Structure, Styles, Lyrics, Singing, GainStaging, KeyFinding) have a `sources` member, one more list row and print topic, a public `showSources()`, `sources.close()` at the top of their show function, and the timer/change callbacks that re-show the current row are skipped while Sources is shown. Chord also has `shapeSources` (key `chordShapes`) for the chord shapes booklet.
- Opening at Sources: `MainComponent::openGuide`, `ArrangementView::showGuide` and `DeviceBar::showGuide (topic, atSources)` call `showSources()` when `topic` is one past the guide's own topics; Chords and Structure (variable counts) use `gen::guides::sourcesTopicIndex` (1000), and `showTopic (row)` there clamps to the last row, which is Sources.
- `BuiltInGuides::getPrintGuide` passes the guide key to `fxui::Guide` and `BuiltInGuides::getEntries` adds "Sources" to the plugin guides' topics, so the Theory Guides chips and the PDFs match.
- `exportAllGuides` also fails ("a guide's last topic isn't Sources") when a guide's last print topic isn't Sources; its topic-name comparison now leaves Sources out.

**Group C hand-off (check and docs)**
- Real-app check (`--screenshots`, scale 1, window captures; scratchpad `shots/`): Routing, Reverb, Chords (no-sources text), Rhythm (note and partial list), Sound at Sources then at topic 0 in the same window (Play tone, Level and Frequency came back), and the plugin overlay of DawIt Reverb at its last page. Each shows the intro, readable links and searches indented under their heading. Link clicks and the right-click menu are not scripted; not clicked.
- `DevScreenshots.cpp`: `"topic": "Sources"` for Chords and Structure now maps to `gen::guides::sourcesTopicIndex` (their windows build their own lists).
- PDF check: `pdftotext` is missing; the PDFs were checked by `--export-guides` (exit 1 when a last print topic isn't Sources).
- Docs: README bullet, `docs/tests.md`, `code-layout.md`, `ui-design.md`, `recipes.md`, `architecture.md`; agents, Codex skills, `AGENTS.md` and `CLAUDE.md` got the `Guide key:` paragraph.
- The follow-up issue for decision 2(c) (sources for the 13 guides) could not be created by the agent (the creation was denied): create it with the text in this hand-off. #6 has a progress comment.
