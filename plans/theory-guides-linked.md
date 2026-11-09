# Theory guides in one place, linked to each other

Status: Released
Idea: [#2 — Theory guides in one place, linked to each other](https://github.com/geertvandamme/dawit/issues/2)

## Goal
The Theory Guides hub is built (View > Theory Guides..., Cmd+G, *Start here*, areas, a search over names and topics). This plan finishes the issue's *Still open* list: every guide page shows *See also* links to related pages in other guides, with *Back*; pages you have read are ticked and *Next page* walks on through a guide and along *Start here*; the search reads the guides' text too; the hub shows the guides still to be written; and the agents keep the links up to date. A beginner can now read the theory as one connected library, see how far they got, and find a page by a word that is only in its text ("eardrum").

## Decisions for you
1. **Links inside the text.** Options: (a) *See also* chips in a row under each page's text, from one tested link table, and inline links later; (b) real links inside the text now, which means link markup in every guide's text (and stripping it for the PDF and the plugins) and a new text view in all 20 guide windows. **Recommended: (a)**: it gives every page its links with one shared row, and the table it builds is exactly what inline links would need later. Inline links become a follow-up issue.
   Answer: (a) *See also* chips under the text now; inline links become a follow-up issue.
2. **When a page counts as read.** Options: (a) automatically after it has been on screen 10 s in the front window, or when you leave it with *Next page*, plus a *Read* toggle to tick or untick by hand; (b) only by hand; (c) as soon as it is opened. **Recommended: (a)**: ticks appear without effort, clicking through the list doesn't tick everything, and the toggle fixes mistakes. Saved in the app settings, not the project.
   Answer: (a) automatically after 10 s on screen or when leaving with *Next page*, plus a *Read* toggle.
3. **Guides still to write.** Options: (a) a short curated list in `gen::guides` (title, area, one line, issue number) shown as dim *Not written yet* cards at the end of their area and of *All guides*, found by the search after real guides, with *Follow it on GitHub* opening the issue in the browser; (b) leave it to the website; (c) defer to a follow-up issue. **Recommended: (a)**, seeded with five (below); the planning agent removes an entry when its guide ships (a test fails when a planned title equals a real guide's name). The list is yours to curate.
   Answer: (c) defer to a follow-up issue: don't build the list of guides still to write (drop the planned-guides parts of the steps, tests and hub).
4. **Where *Next page* goes.** Options: (a) the next topic of the same guide; at the end of a guide on *Start here*, the first page of the next guide on the path (skipping *Sources*); off the path, on to *Sources* and then nothing; (b) also across all guides in *All guides* order. **Recommended: (a)**; issue #3 (*Guide paths by goal*) later swaps *Start here* for the path you chose, with no change to the rule.
   Answer: (a) next topic in the same guide, then the next guide on *Start here* (skipping *Sources*).
5. **Agent edits the issue asks for** (`.claude/agents/dawTheory.md` and `dawPlanFeature.md`: add links to and from related guides, drop a shipped guide's planned entry). **Recommended:** make them in step 12 with your go-ahead, since they change agent configuration; `.claude/` stays out of commits as usual. Also `docs/recipes.md` (*A theory guide*, *Topics in an existing guide*).
   Answer: Yes: update `.claude/agents/dawTheory.md` and `dawPlanFeature.md` in step 12 (local only, not committed), plus `docs/recipes.md`.
6. **Scope, if you want it smaller.** Groups are ordered so the work can stop after any of them: links and Back (A1, B1, B2) are the core; text search and progress come in the same groups but could be cut; planned guides (3) can move to a follow-up issue with no effect on the rest. **Recommended:** build all of it; each part is small once the shared footer exists.
   Answer: Build all the rest (links, Back, read progress and *Continue*, *Next page*, text search); only the guides still to write are deferred (decision 3).

## What the user does and sees
**A footer row on every guide page** (all 19 `*Explainer` windows and the built-ins' `fxui::Guide`), 32 px under the explanation, inside the right column, on `panel`:

```
See also: [EQ · The frequency ranges] [Loudness · Peaks aren't loudness] +2   [< Back to What is sound?]   [✓ Read] [Next page: Phase >]
```
- **See also** chips: the page's links (both directions of the table, below), at most three shown, the rest under *+N* as a menu. Chip text is "<guide short name> · <page>", shrunk to fit then cut. Click: opens that guide's window (or brings it to the front) at that page. Tooltip: "Opens <guide title> at <page>. Back there brings you here." Right-click a chip: *Open*, then the guide item (`guideicon::addItem`, "Theory Guides...").
- **Back**: shown in the window a link or a cross-guide *Next page* opened, labelled with where it returns ("Back to What is sound? · Frequency is pitch", shrunk to fit). Click: opens that page again. History is the session's last 20 jumps; not saved. Cmd+[ does the same in a guide window.
- **Read** toggle (`✓ Read` / `Read`): ticks or unticks the page by hand. Tooltip: "Ticked when you've read this page: after 10 seconds on it, or when you go on with Next page. Click to tick or untick it."
- **Next page**: "Next page: <topic>" in the guide, "Next: <guide title>" at the end of a guide on *Start here* (Decision 4). Cmd+] does the same. Leaving a page with it ticks it read. Hidden on the last page off the path.
- A *Your …* page is one whose name starts with "Your".
- Pages already read show a small dim tick at the right of their row in the guide's topic list. *Sources* and the *Your song* pages are not counted (they change with the song); they show no tick and no Read toggle.
- In a plugin (`fxui::isPlugin`) the footer is hidden: there is no app to open other guides or keep progress.

**In the Theory Guides window:**
- Each card's topic chips show a tick for pages read, and the card says "4 of 11 pages read" (dim, none when 0).
- *Start here* shows "18 of 74 pages read" and a **Continue** button opening the first unread page along the path (tooltip: "Opens the first page you haven't read on this path"). When all are read: "You've read the whole path".
- **Search into the text:** a word only in a page's text also finds it, ranked below matches in a name, topic, keyword or summary. Such a topic is outlined as now and the card shows one line of the text around the first match, the word in `accent` ("… pushes and pulls your **eardrum** the same way …").
- **Not written yet** cards (Decision 3) at the end of their area and of *All guides*: dim, title, one line, "Not written yet", and a *Follow it on GitHub* link (tooltip: "Opens issue #61 on GitHub in your browser"). No topics, no export.
- Right-click on the list or a card adds *Mark all pages unread...* (asks first), before the guide item.
- Fix: the export tooltip says "Start here's seven guides"; the path has eight. Say "Start here's guides".

**Remembered between sessions:** `guides.read` in the app settings: the pages read, one "<guide key>/<topic name>" per line. Unknown pages (a renamed topic) are ignored and kept.

## Data and saving
Nothing in the project, no format change. One app setting (`guides.read`, above).

## The guide
No new guide or topic. Every guide gains links; the table is the content (below). It lives in `gen::guides`, next to the entries, and is the only place links are declared (the built-ins' pages are made in `src/ui`, so per-guide files couldn't declare them all).

**Rules of the table:** a link joins two pages, written once and shown on both. An empty topic means the guide's first page. No link to the same guide, no duplicate pair, every guide in `allGuides` in at least one link. Topic names must be names in the guide's entry (checked by tests for the guides whose topics `gen::guides` knows, and by `--export-guides` for the built-ins).

**The starting table** (keys from `gen::guides::getKey`; dawImplement checks each page name and, where one differs, picks the closest page and notes it under *Changes while building*):

| From | To |
|---|---|
| sound · Frequency is pitch | eq · The frequency ranges |
| sound · Amplitude is loudness | loudness · Peaks aren't loudness |
| sound · The shape is the timbre | additive · The harmonic series |
| sound · The shape is the timbre | subtractive · Envelopes: attack, decay, sustain, release |
| sound · Phase cancellation | stereo · Mono: phones, clubs and the sum |
| sound · Checking in mono | stereo · Mid and side |
| sound · From air to numbers | recording · From the air to a file |
| sound · From air to numbers | saturation · Aliasing |
| sound · Sound is moving air | instruments · How instruments make sound |
| spectralFreeze · A sound as a spectrum | sound · The shape is the timbre |
| eq · Where instruments sit | drums · The kick: a sine that falls |
| eq · Where instruments sit | stereo · Why the low end stays in the middle |
| eq · Louder sounds better: match level | gainStaging · Louder sounds better |
| compressor · Threshold and ratio | gainStaging · Levels into a compressor |
| compressor · How much, for what | drums · Building a kit, and recipes |
| compressor · How much, for what | routing · Glue: one compressor for the group |
| compressor · Turning down the loud parts | gate · Closing the gaps |
| compressor · Attack: how fast it grabs | triggers · Transients |
| gate · Gated reverb | reverb · Kinds of reverb |
| slicer · Mute: a trance gate | gate · Closing the gaps |
| physical · A wave running round a loop | delay · Short delays |
| physical · Karplus-Strong: noise into a note | instruments · Strings and their harmonics |
| physical · Tubes and reeds | instruments · Tubes: open and closed |
| physical · Recipes for common sounds | subtractive · Recipes for common sounds |
| subtractive · Recipes for common sounds | additive · Recipes for common sounds |
| additive · Recipes for common sounds | fm · Recipes for common sounds |
| fm · Recipes for common sounds | subtractive · Recipes for common sounds |
| subtractive · The LFO | stepFilter · Step filter, slicer and LFO |
| stepFilter · Cutoff and the three filters | subtractive · The filter |
| additive · Drawbars: the Hammond organ | instruments · Keys: piano and organ |
| saturation · Harmonics | additive · The harmonic series |
| distortion · More than saturation | saturation · Pushed too far |
| drums · Drums from oscillators and noise | instruments · Drums, bars and bells |
| drums · Velocity, accents and groove | rhythm · Feel and the grid |
| drums · Tuning drums to the song | keyFinding · What a key is |
| instruments · Synths and electronic instruments | subtractive · (first page) |
| instruments · What makes MIDI sound real | midi · Velocity |
| instruments · What makes MIDI sound real | sampler · (first page) |
| instruments · Drums and percussion | triggers · What a drum trigger is |
| midi · The sustain pedal | instruments · Keys: piano and organ |
| midi · Note numbers and pitch | readMusic · The grand staff and the piano |
| midi · Note numbers and pitch | tuning · Reference pitch: A4 = 440 Hz |
| midi · Backwards: retrograde | arpeggiator · Note order |
| rhythm · Quantising | triggers · On the grid or as played |
| rhythm · Time signatures | readMusic · Time signature, bars and tempo |
| rhythm · Note values | readMusic · Note values on the staff |
| rhythm · Note values | slicer · Note lengths and steps |
| rhythm · Tempo and time | delay · Time and tempo |
| chords · How chords work | keyFinding · Naming a chord from its notes |
| chords · How chords work | lyrics · Words and music agree |
| chords · How chords work | arpeggiator · Chord tones |
| chords · The circle of fifths | readMusic · Key signatures |
| chords · Playing along | chordShapes · (first page) |
| chords · Well-known progressions | structure · The sections |
| chords · Well-known progressions | rhythm · Bars and beats |
| keyFinding · Inversions and slash chords | chordShapes · (first page) |
| structure · How songs are built | lyrics · Song forms |
| structure · Typical structures | styles · Styles of music |
| styles · Drum bus and reverb return | routing · Typical buses in a song |
| lyrics · Stress and the beat | rhythm · Counting |
| lyrics · Singing along | singing · Vowels and words |
| singing · Singing in tune | tune · (first page) |
| singing · Singing in tune | tuning · Cents, hertz and beats |
| singing · Harmonies and doubles | harmoniser · (first page) |
| singing · The microphone | recording · Choosing an input |
| recording · Setting the level | gainStaging · Quiet, healthy, hot, over |
| gainStaging · Over 0 dB | loudness · True peak |
| gainStaging · Through a bus | routing · Levels add up |
| gainStaging · Step by step | loudness · Loudness while mixing |
| loudness · Targets and normalisation | handover · Headroom and the master |
| routing · Stems and multitracks | handover · Stems, multitracks and the mix |
| routing · Insert or send? | reverb · Reverb in a mix |
| routing · Sends and returns | delay · Delay in a mix |

**Not written yet (seed list, Decision 3):**
- *Ear training* (Reading music, #11): "Hearing intervals, chords and rhythms, and naming what you hear."
- *Releasing music* (Writing songs, #18): "Distribution, royalties, labels and what happens after the song is finished."
- *Scoring for film, TV and games* (Writing songs, #19): "Writing music to picture: timing to hits, cues and moods."
- *How podcasts are made* (Mixing, #32): "Recording and editing speech, levels and loudness for spoken audio."
- *The mastering chain* (Mixing, #61): "The steps a master goes through, from EQ and compression to the limiter and loudness."

Search words for them: ear training interval; release distribution royalties label streaming; film score picture cue game; podcast speech voice editing; mastering master limiter chain.

## Code
Recipes: *A theory guide* (step 5, the Theory Guides window; step 7, tests), *A window* (opening through `PluginWindowManager`), *A command or menu item* for the shortcuts; copy `GuideSourcesTopic` for the shared per-window helper.
- **Links** (`gen::guides`, pure, tested): the table above; a lookup giving a page's links in both directions; validation helpers the tests and `--export-guides` share.
- **Text in the entries:** `Entry` gains each topic's text, filled with the song-free text each guide's explain gives (an empty song, C major, 120 BPM where one is needed); `BuiltInGuides::getEntries` fills the built-ins' pages from `makePages` as it already does for their names. Search scores a text-only match lowest (below summary); an excerpt helper returns the line around the first match. The *Sources* topic has no text.
- **Progress and path** (`gen::guides`, pure): a page id "<key>/<topic>", read counts per guide and per path, *Next page* by Decision 4, first unread page on a path. The read set itself is kept by the UI.
- **Planned guides** (`gen::guides`): a separate list, never in `allGuides`; search returns them after real guides.
- **Guide navigator** (UI, owned by `MainComponent`): follows a link or *Next* by turning the topic name into an index with the hub's entries and calling `MainComponent::openGuide`; keeps the Back history; owns the read set (loads and saves `guides.read`) and broadcasts changes so open windows and the hub repaint ticks. Guide windows find it through one app-wide pointer set by `MainComponent`; null in plugins and tests, which hides the footer.
- **Footer helper** (UI): owned by each guide window like `GuideSourcesTopic`; the window tells it which page shows (guide key and topic name), lays it out under the text, and asks it to paint the row tick. It owns the 10 s read timer. Not printed.
- **Threading:** message thread only; no engine lock, no audio change.
- **Risk:** a wide mechanical change across 20 windows (layout, the shown-page call, the list tick). Keep each window's topic switch untouched. `MainComponent::exportAllGuides` fails (exit 1) when a link names a built-in page that doesn't exist.

## Libraries and models
None.

## Theory
None: no facts or numbers are stated. The link table and the planned guides' one-liners are editorial; `SOURCES.md` is unchanged.

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. Links in `gen::guides` (`src/generate/TheoryGuides.{h,cpp}`, or a new `TheoryGuideLinks.cpp` added to both targets): the table and its lookup. Tests: link rules (below). Done when: tests pass.
- [x] 2. Topic text in the entries and search into it, with the excerpt helper. Tests: "eardrum" finds *What is sound?* at *Sound is moving air* with an excerpt containing "eardrum"; a name match outranks a text-only match; the existing search checks still pass.
- [x] 3. Page ids, read counts, *Next page* and first unread on the path; the planned-guides list. Tests: progress, path and planned checks (below). Done when: `./build.sh --test` passes.

Group B1, UI: navigator, footer, built-ins, hub (`./build.sh --test` at step 7):
- [x] 4. The guide navigator in `MainComponent` (new `src/ui/GuideNavigator.{h,cpp}`): follow, Back history, read set in `guides.read`, change broadcast, Cmd+[ and Cmd+] routed from guide windows. Check: links open the right window at the right page.
- [x] 5. The footer helper (new `src/ui/GuideFooter.{h,cpp}`) with chips, *+N* menu, Back, Read toggle, Next, list tick, read timer; wire it into `fxui::Guide` (all built-ins at once). Extend `--export-guides` with the built-in link check. Add `"guideLink": n` (click the nth See also chip) and `"guideNext": true` to `ScreenshotRunner`.
- [x] 6. The hub (`TheoryGuidesWindow.cpp`): ticks and "n of m pages read" on cards, Start here's count and Continue, text excerpts in search results, Not written yet cards, *Mark all pages unread...*, the export tooltip fix.
- [x] 7. Check: a shot list on the EQ guide and the hub; `./build.sh --test` and `--export-guides` pass.

Group B2, UI: the explainer windows, part 1 (`./build.sh --test` at step 9):
- [x] 8. Wire the footer into Sound, MIDI, Rhythm, Chords (and chord shapes), Structure, Styles, Lyrics, Singing, Reading music, Key finding (`src/ui/*Explainer.cpp`). Their *Your …* topic gets no Read toggle.
- [x] 9. Shots of each at one linked page; `./build.sh --test` and `--export-guides` pass.

Group B3, UI: the explainer windows, part 2 (`./build.sh --test` at step 10):
- [x] 10. Wire the footer into Gain staging, Routing, Recording, Tuning, Drum triggers, Handover, Instruments and any remaining guide window (`EqExplainer`, `CompressorExplainer` if they host their own list); shots; `./build.sh --test` and `--export-guides` pass.

Group C, check and docs:
- [x] 11. Real-app check, one `--screenshots` run: (a) `guide: sound`, topic *Frequency is pitch*: footer with chips to EQ and Next; (b) next shot `"guideLink": 1`, `capture: window`: the EQ guide at *The frequency ranges* with *Back to What is sound? · Frequency is pitch*; (c) hub at *Start here* with `"settings": {"guides.read": "sound/Sound is moving air\nsound/Frequency is pitch"}`: ticks on two chips, "2 of 10 pages read" on the What is sound? card (it has no *Your …* page), the path count and Continue; (d) hub search "eardrum": the excerpt line; (e) hub *Mixing* area: the Not written yet cards; (f) a footer in a narrow built-in guide (Stereo) with *+N*. Look for cut text, overlapping chips and wrong counts.
- [x] 12. Docs: README bullet under *Features* (extend the *Find a path* bullet: links, Back, ticks, Next, text search, guides not written yet); `docs/tests.md` theory guides line; `docs/code-layout.md` (`GuideNavigator`, `GuideFooter`, links); `docs/ui-design.md` *Guide windows* (the footer row); `docs/architecture.md` *Guides* (the links table and navigator); `docs/recipes.md` *A theory guide* and *Topics in an existing guide* (add links to and from related pages, drop a planned entry); agents per Decision 5; issue #2: move the done items, update the stale "19 guides" line, link follow-ups (inline links, #3, #47, #168).

## Tests
In `runTheoryGuidesTests`:
- **Links:** every link's two guides are in `allGuides` and differ; no pair twice (either direction); every guide in at least one link; every topic named for a guide with known topics is in its entry's topics; the lookup for *sound · Frequency is pitch* includes *eq · The frequency ranges*, and the lookup for that EQ page includes the sound page (both directions); a guide's first page gets the links with an empty topic.
- **Text search:** every non-plugin topic except *Sources* has text; "eardrum" → *What is sound?* with topic *Sound is moving air*; the excerpt contains "eardrum" and is at most 160 characters; "rhyme" still ranks *How lyrics are written* first; a made-up pair of entries where one has the word in its name and the other only in text ranks the name first.
- **Progress:** page id of (rhythm, *Quantising*) is "rhythm/Quantising"; with two pages of *What is sound?* read, its count is 2 of 10; *Note lengths, beats and bars* counts 11 pages (12 less *Your song's rhythm*); an unknown id in the read set changes no count.
- **Next page:** mid-guide → next topic; last own page of *What is sound?* (on the path) → *Note lengths, beats and bars* at topic 0; last own page of a guide off the path → *Sources*; *Sources* off the path → none; the last guide on the path at its end → none. First unread on the path with nothing read → *What is sound?* topic 0; with all of *What is sound?* read → *Note lengths, beats and bars* topic 0.
- **Planned guides:** five, each with a title, summary, area and issue number above 0; no title equals a real guide's name; "royalties" finds *Releasing music* after any real guide; planned guides are not in `getEntries`.
- `--export-guides` passes (built-in link pages exist).

## Website hand-off
- `features.html`, the theory guides group: the guides now link to each other (See also, Back), tick what you've read, *Next page* along *Start here*, and search their text.
- `learn.html`: a line on links and progress near the guide cards; could list the same Not written yet guides.
- Screenshots: a guide window showing the footer (What is sound? at *Frequency is pitch*), the hub at *Start here* with ticks and Continue. No new library, so not `about.html`.

## Left for later
- The list of guides still to write, shown as *Not written yet* cards (deferred by Decision 3; seed list above).
- Inline links inside the text (follow-up issue; the link table is ready for it).
- The AI picking the right page from a question: #47.
- Paths by goal, *Next* along the chosen path: #3.
- *See also* in the PDF export and on the website's full guides (#168), from the same table.
- Progress synced to the website: not planned (nothing leaves the machine).

## Changes while building

**Group A hand-off (engine).**
- Links: `src/generate/TheoryGuideLinks.cpp` (in both targets) holds the table (`gen::guides::getLinks()`), `getLinksFor (guide, topic, isFirstPage)` (both directions; an empty topic in a returned `PageRef` means that guide's first page), `validateLinks (entries)` (returns problem lines; empty is fine; for `--export-guides` pass `BuiltInGuides::getEntries()`, whose built-in entries have their topics, so their page names get checked too). All page names in the starting table exist as written for the guides `gen::guides` knows; the built-ins' page names were only grepped for in `src/`, so `--export-guides` must confirm them.
- Text: `Entry::texts` (parallel to `topics`, empty for Sources) is filled in `getEntries()` from each guide's own explain with no song (C major, 120 BPM, empty song; styles use the new `stylesguide::explainGeneral`). **Not done:** `BuiltInGuides::getEntries()` (src/ui/TheoryGuidesWindow.cpp) must still fill `texts` for the built-ins from `makePages` (`Page::text`, with an empty string for Sources); Group B1 step 6 does that. `search` scores name 6, topic/keywords 4, summary 2, page text 1 (was 3/2/1), and puts text-only matching topics in `Match::topics`. `getExcerpt (text, words)` gives the line (at most 160 characters, with "..." at cut ends) for the card; highlight the word in the UI.
- Progress: `getPageId`, `isCountedPage` (not Sources, not "Your..."), `getProgress (entry, readIds)`, `getPathProgress`, `getFirstUnreadOnPath`, `getNextPage (entries, guide, topic)` returning `PagePosition {guide, topic}`; the read set is a `juce::StringArray` of ids. Pass UI entries (built-ins with topics), or built-in guides have no next page.
- Decision 3: the planned-guides list is left out entirely (no data, tests or hub cards).
- The stale name checks: no link page names had to change.

**Group B1 hand-off (navigator, footer, built-ins, hub).**
- `src/ui/GuideNavigator.{h,cpp}`: owned by `MainComponent` (`guideNavigator`, declared before `pluginWindows`), app-wide `GuideNavigator::getInstance()` (null in plugins and tests). Works by page *name*: `getLinks (guide, topic)`, `follow (fromGuide, fromTopic, PageRef)`, `getBack (shownGuide)` / `goBack()`, `getNext (guide, topic)` / `goNext`, `isRead` / `setRead` / `markAllUnread` (reads `guides.read` fresh from the settings each call), `setShown`. It opens pages through `MainComponent::openGuide`; `onShowGuides` opens the hub.
- `src/ui/GuideFooter.{h,cpp}`: a `juce::Component` a window owns as a member. B2/B3 wire a window with: (1) `addChildComponent (footer)`; (2) wherever the shown topic changes (and on Sources), `footer.setPage (gen::guides::Guide, topicName)` (Sources is `gen::guides::sourcesTopicName`; a window with no page calls `footer.clearPage()`); (3) in `resized()`, `if (footer.isActive()) { footer.setBounds (r.removeFromBottom (GuideFooter::height)); r.removeFromBottom (6); }` before the text, and call `resized()` when `isActive()` changed; (4) `footer.onReadChanged = [this] { list.repaint(); }` and, in the row painter, `GuideFooter::paintTick (g, w, h, guide, topicName)` (it skips Sources and *Your ...*; keep the row text 36 px clear of the right edge); (5) `bool keyPressed (const KeyPress& k) override { return footer.keyPressed (k); }` for Cmd+[ and Cmd+]. The topic name must equal the name in the entry's topics (`gen::guides::getEntry`), or Next and links are skipped for that page (Chords and Structure list their series of progressions and structures as grouped pages: check those).
- `fxui::Guide` is wired (all built-ins): it takes its guide from the `sourcesKey` (`findGuide`), so a built-in guide made with no key has no footer. `BuiltInGuides::getEntries` now fills `texts` for built-ins from `makePages` (`text`, else `liveText()`), and Sources gets an empty text.
- Hub: cards show ticks on read chips, "n of m pages read", the search excerpt (word in `accent`; only for a topic whose own name has none of the words), Start here shows "n of m pages read" and Continue, right-click *Mark all pages unread...* on a card and on the list (asks first), export tooltip fixed; the Start here subheading no longer says "Seven guides". The hub refreshes on navigator changes keeping its scroll. Not built (Decision 3): Not written yet cards.
- `--export-guides` prints "Links: 73 links between guides, all pages exist" or "Links: FAILED: ..." (exit 1) from `validateLinks (BuiltInGuides::getEntries())`.
- ScreenshotRunner: `"guideLink": n` (follow the n-th See also chip of the page last shown), `"guideNext": true`, and for `"scene": "theoryGuides"`: `"guidesRow": n` (0 Start here, 1.. areas, last All guides) and `"guidesSearch": "text"`. Shots taken (scratchpad `shots.json`): eq at *Where instruments sit* (chips Drums, Stereo width; Read; Next page), `guideLink: 1` (Drums at *The kick*, with "Back to What is EQ? · Where instruments sit"), hub on Start here with two pages read, hub search "eardrum" (excerpt with the word in accent). All looked right.
- Surprises: Group C's shot (a) uses `guide: sound`, which has no footer until B2 wires it; the 10 s read timer is untested in the app (it needs a front window).

**Group B2 hand-off (footer in the ten explainer windows).**
- Wired: Sound, MIDI, Rhythm, Chords, Structure, Styles, Lyrics, Singing, Reading music, Key finding (`src/ui/*Explainer.{h,cpp}`). Each has a `GuideFooter footer` member, `updateFooter()` (Chords, Structure, Styles and Rhythm take the row: `updateFooter (int row)`, -1 for Sources), `keyPressed` forwarding to the footer, the tick in the row painter (row text 36 px clear of the right edge) and `footer.takeSpace (r)` in `resized()` before the Sources check.
- New helpers on `GuideFooter` that B3 should use instead of the five-step recipe: `bool update (Guide, topic)` (calls `setPage` or `clearPage`; true when the footer appeared or went, so call `resized()`) and `void takeSpace (Rectangle<int>&)` (call in `resized()` before placing the text). B3 per window: include `GuideFooter.h`, `GuideFooter footer;` member, `footer.onReadChanged = [this] { list.repaint(); }; addChildComponent (footer);` in the constructor, `updateFooter()` after the topic changes and in `showSources`, `takeSpace` in `resized()`, `paintTick` in the row painter, `keyPressed`.
- Chords and Structure list their series as several rows: a static `getFooterTopic (row)` in each .cpp takes the name from `gen::guides::getEntry (...).topics` and clamps the row, so every progression is "Well-known progressions" and every structure "Typical structures". The chord shapes guide (a print mode of `ChordExplainer`, no entry topics) has no footer.
- Shots (scratchpad `shots2.json`): Sound at *Frequency is pitch* (chip EQ, Read, Next page), its first chip followed (EQ at *The frequency ranges* with Back to What is sound? / Frequency is pitch), Chords at *The circle of fifths*, Structure at *The sections*, Styles; all looked right (no cut text or overlap). `--export-guides` still passes.

**Group B3 hand-off (footer in the remaining explainer windows).**
- Wired the same way as B2: Gain staging, Routing, Recording, Tuning, Drum triggers, Handover, Instruments (`src/ui/*Explainer.{h,cpp}`): `GuideFooter footer` member, `updateFooter()` (topic name from the shown topic, Sources while the sources panel shows), `keyPressed` forwarding, `paintTick` in the row painter (text 36 px clear of the right edge), `footer.takeSpace (r)` in `resized()` before the Sources check. Their *Your ...* pages get no Read toggle (done by `GuideFooter`).
- `EqExplainer` and `CompressorExplainer` don't host a topic list (they are not guide windows with a list; the EQ and compressor guides are `fxui::Guide`, wired in B1), so they are untouched. No other `GuideSourcesTopic` user lacks a footer.
- Shots (scratchpad `shots3.json`): Gain staging (Read and Next page), Routing, Tuning, Instruments (See also chip to Sound, Read, Next page); looked right. `./build.sh --test` passes (All checks passed) and `--export-guides` passes ("Links: 73 links between guides, all pages exist").

**Group C hand-off (check and docs).**
- Real-app check (scratchpad `shotsC.json`, `shotsC2.json`, scale 1): (a) Sound at *Frequency is pitch* (chip EQ, Read, Next page); (b) `guideLink: 1`: EQ at *The frequency ranges*; (c) new runner key `"guideBack": true` (`ScreenshotRunner`, calls `GuideNavigator::goBack`): the Sound window back on *Frequency is pitch*; (d) Sound at its last counted page *Taking the vocals out*: "Next: Note lengths, beats and bars"; (e) `guideNext`: the rhythm guide opens at *Bars and beats* with "Back to What is sound? · Taking the vo..." and *Next page: Time signatures*; (f) Stereo at *Mono: phones, clubs and the sum* (one chip); Routing and Tuning at a linked page; hub at *Start here* with two pages read ("2 of 10 pages read" on the What is sound? card, "2 of 76 pages read" and Continue; the plan said 74, the real count is 76). All looked right, no cut or overlapping text. Shot (e) of the plan (Not written yet cards) is not built (Decision 3). No page has more than three links, so the *+N* menu was not seen in the app (the lookup is tested).
- Docs: README *Find a path* bullet, `docs/tests.md`, `docs/code-layout.md` (`TheoryGuideLinks`, `GuideNavigator`, `GuideFooter`), `docs/ui-design.md` (footer row), `docs/architecture.md` *Guides*, `docs/recipes.md` (*A theory guide* step 6a and *Topics in an existing guide*), `.claude/agents/dawTheory.md` and `dawPlanFeature.md` (local only; `.claude/` is ignored by git). `./build.sh --test`: All checks passed, no warnings; `--export-guides` passed in B3.
- Left out by decision 3: guides still to write (kept in *Left for later* and in the issue).
- Website hand-off: as in *Website hand-off* above, minus the Not written yet cards; screenshots worth taking: Sound at *Frequency is pitch* with the footer, the rhythm guide after Next page (Back button), the hub at *Start here* with ticks and Continue.
