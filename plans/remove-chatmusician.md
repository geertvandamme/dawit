# Remove ChatMusician from DawIt and keep the simpler Generate panel

Status: Released
Idea: [#194 — Remove ChatMusician from DawIt and keep the simpler Generate panel](https://github.com/geertvandamme/dawit/issues/194)

## Goal
The Generate panel offers two modes, *Built-in pattern* and *AI part*, instead of three. ChatMusician's mode, task box, model list, chord choices, one-bar format check and 4/4 rule go, with the code, tests, guide text and docs behind them. A beginner sees fewer choices and no mode that rarely gives musical results; staging (Preview, Apply, Discard, Undo apply) and the Lyrics window's *Write with AI* stay as they are.

## State of the working tree (checked 12 November 2026)
#182 ([patterns-and-ai-preview-and-apply.md](patterns-and-ai-preview-and-apply.md)) is *Built, not committed*: all groups done, `./build.sh --test` passed, real-app shots looked at. Its uncommitted files hold most of what this plan changes: `AiSetup.*` (new), `GenerateStaging.*` (new), `GeneratePanel.*`, `OllamaClient.*`, `DevScreenshots.cpp`, `EngineTests.cpp`, the docs, and a superseded note in `chatmusician-generation.md`. `ChatMusician.*` and `OllamaClient::generateRaw` were committed earlier (#103). `generateRaw` is used only by the panel's ChatMusician request; `gen::voiceChord` (`NoteRealizer`) is also used by `ChordStyles` and stays, while `chatmusician::voiceChords` goes. `ProjectFile.cpp` saves nothing of ChatMusician or of a staged take (the overlay is transient), so projects are unaffected. `LICENSE` (line 223) and `licenses/` name no ChatMusician code, weights or notice.

## Decisions for you
**Answered by the user, 12 November 2026:** all six recommendations accepted. 1: commit #182 first (done before building), build this as a second commit, one website pass after both. 2: an old mode opens as *AI part*. 3: remove the *ABC and generated music* topic. 4 and 5: mark the old plan superseded and keep dated history. 6: rename the screenshot keys.

1. **How this relates to #182.** Options: (a) commit #182 as it is (its own commit, already verified), then build this on top as a second commit; (b) fold this removal into #182 before its first commit, so ChatMusician's three-mode panel never lands; (c) wait. **Recommended:** (a): #182 is complete and checked, two commits keep each change reviewable and revertible, and this plan's steps are the same either way. Hold #182's website hand-off and do one website update after this plan, so the site never describes three modes. With (b), the steps still apply to the working tree, and Group C updates #182's plan and issue instead of adding notes.
   Answer:
2. **An old `generator.mode` of `chatMusician`.** Options: open as *AI part*; open as *Built-in pattern*. **Recommended:** *AI part*: the user chose a local model, Ollama is set up, and the AI status line explains anything missing. The old `generator.chatMusician` switch (true) also gives *AI part*. The unused keys `generator.chatMusicianModel`, `generator.chatTask` and `generator.modelChecks` are removed from the settings once, when the panel loads.
   Answer:
3. **The reading guide's topic *ABC and generated music*.** It exists only for ChatMusician (the panel's guide button opens it), and nothing else in DawIt reads or writes ABC. Options: remove the topic with its picture; keep it with only the ABC paragraph. **Recommended:** remove it: a topic about a notation DawIt doesn't use, with no way to reach it from the app, teaches nothing the user can try. The guide goes from 15 topics to 14; its search words lose "abc generated musician" and its "also opens from" loses the Generate panel; `SOURCES.md` loses the abcnotation.com line under *Reading music*, which supported only that topic.
   Answer:
4. **The old plan.** Options: delete `plans/chatmusician-generation.md`; mark it superseded. **Recommended:** `Status: Superseded` with a one-line note at the top pointing to this plan and #194 (replacing #182's "panel workflow superseded" note), and add *Superseded* to the status list in `plans/README.md`. Released plans that mention ChatMusician (`more-time-signatures`, `generate-panel-fixed-footer`, `random-chord-progression`, `undo-and-redo`, `usability-improvements`) describe what was true when built and stay as they are; `patterns-and-ai-preview-and-apply.md` gets a one-line note under its status.
   Answer:
5. **What may still say "ChatMusician".** Options: nothing outside the superseded plan; also dated history. **Recommended:** allow dated history: the superseded plan and the notes in other plans, the settings migration (the literal `chatMusician` and the old keys), the search line under *Not for a guide* in `SOURCES.md` (a record of research done, not shown in the app), and the dated audit `docs/usability-review-2026-11-10.md` with its `more.json`. Group C's grep check lists exactly these.
   Answer:
6. **Screenshot keys named after ChatMusician that the AI part still uses** (`chatApply`, `chatClearChords`). Options: keep the names; rename to `generateApply` and `generateClearChords`. **Recommended:** rename, no aliases: shot lists live in scratch folders, and `docs/recipes.md` is updated with them.
   Answer:

## What the user does and sees
- The Generate panel's radio group: `[ Built-in pattern | AI part ]`, Tab to reach, arrow keys to move, the two mode lines and tooltips unchanged. Main button *Make pattern* / *Generate*.
- Gone: the ChatMusician mode line, Model and Task boxes, *Sevenths*, *Borrowed chords*, *Major dominant in minor*, first/last chord fields, *How to read music...*, the one-bar check line and *Test again*, the "Checking the model with one test bar..." progress, and ChatMusician's 4/4 note (the "This pattern is written in 4/4..." note for patterns stays).
- *Set up...* has one set of steps for the AI part (install, start, `ollama pull qwen2.5:7b` with its Copy button, Retry); the AI status line has its unreachable, no-models, not-installed and ready states.
- Right-click on the panel: the footer's actions, then *Styles of music...* always.
- Proposed chords stay: AI part can still propose bar chords, shown in the footer with the Apply choice.
- A user who last used ChatMusician opens the panel in *AI part* (decision 2), with no error.
- The reading guide has 14 topics; *How DawIt writes the score* is followed by *Your part*.

## Data and saving
No project change and no format version change: `ProjectFile.cpp` holds no ChatMusician data, and staged takes are never saved. The existing save-and-load tests cover older projects. App settings: `generator.mode` is `pattern` or `ai`; `chatMusician` reads as `ai`; the three obsolete keys are removed on load (decision 2); `generator.ollamaUrl`, `generator.model` and the other `generator.*` keys are untouched.

## The guide
No new guide or topic. The reading guide (`gen::readmusic`) loses *ABC and generated music* with its picture and text (decision 3); `MainComponent`'s jump to that topic and `PianoRoll::onShowGeneratedMusicGuide` go. Its `gen::guides` entry: "also opens from" becomes "The Score view's guide button, its right-click menu, and File > Export Sheet Music (PDF)..."; search words drop "abc generated musician". No `TheoryGuideLinks` entry points at the topic (checked). The Styles of music guide stays the Generate panel's guide.

## Code
A removal; no recipe adds anything. Where a recipe's example or wiring mentions ChatMusician, the docs are corrected in Group C.
- **Modes:** `gen::aisetup::Mode` has two values. `getModeFromId` maps "chatMusician" to the AI part and anything unknown to the pattern; `migrateMode`'s old switch gives the AI part. A pure helper in `AiSetup` tidies an app-settings `PropertySet` (maps the mode, removes the three obsolete keys) so it is tested headless; the panel calls it on load.
- **Setup state:** drop `noChoice`, `needsCheck`, `checkFailed`, the check line, *Test again*, `ModelChecks`, `formatCheckDate`, `orderForChatMusician` and `runOneBarCheck`. Ready means a chosen, installed model; `describe` no longer takes check records. Keep `ModelInfo`'s digest and family (parsed and tested; harmless).
- **Panel:** remove the third mode and its controls, `generateChatMusician` and the ChatMusician branches in layout, `updateControls`, `getInfoNote`, `getAiModel`, `applyModelList`, the right-click menu, `setCannedSetup` (`unchecked` and `failed` go) and the tab order. Keep `generationId`, `cancelFlag`, staging and the footer exactly as #182 left them.
- **Delete** `src/generate/ChatMusician.h/.cpp` (both `CMakeLists.txt` targets) and `OllamaClient::generateRaw` (only ChatMusician used it).
- **Threading:** unchanged; nothing touches the audio thread or the edit lock.
- **Risk:** low. A wide but shallow removal in one large file (`GeneratePanel.cpp`, about 1,900 lines); the footer layout must still fit at 960 x 600. Removing a reading-guide topic shifts the topic indices that `MainComponent::openGuide` and the PDF export use: they come from `allTopics`, so check that nothing hard-codes an index past *How DawIt writes the score*.

## Libraries and models
None added or removed. `LICENSE` and `licenses/` have no ChatMusician entry (DawIt never shipped or downloaded a ChatMusician model). Group C re-reads the Ollama paragraph in `LICENSE` and the README's *License note* and leaves them unchanged, since the AI part and *Write with AI* still use Ollama.

## Theory
None new. The only theory change is removing the reading guide's *ABC and generated music* topic (decision 3) and its one `SOURCES.md` line; no remaining text states a fact that changes. The Styles of music and reading guides then say nothing about ChatMusician, ABC or a 4/4-only generator.

## Steps
Ordered so each group's build passes: the panel stops using ChatMusician first, then the engine code it used goes.

Group A, UI: the Generate panel with two modes (`./build.sh --test` at step 4):
- [x] 1. `GeneratePanel`: two mode buttons; remove every ChatMusician control, branch, setting read and write, `generateChatMusician`, the reading-guide callback and the ChatMusician 4/4 note; right-click ends with *Styles of music...*; tab order and accessible titles for what's left. Check: the panel shows two modes; Built-in pattern and AI part unchanged.
- [x] 2. `PianoRoll` and `MainComponent`: remove `onShowGeneratedMusicGuide` and the jump to the topic (the Score view's own reading-guide button stays). `setCannedSetup`: drop `unchecked` and `failed`.
- [x] 3. `DevScreenshots.cpp`: remove the `chatMusician` block and its keys (`chatFourBars`, `chatScroll`, `chatGenerate`, `chatModel`), `generateMode` accepts `pattern` or `ai`, rename `chatApply` to `generateApply` (its `midi` and `chords` actions stay: the Apply choice still offers notes only and chords only) and `chatClearChords` to `generateClearChords` (decision 6).
- [x] 4. Build. Done when: `./build.sh --test` passes with no warnings; `grep -i chatmusician src/ui` finds nothing.

Group B, engine and the guide (`./build.sh --test` at step 8):
- [x] 5. `AiSetup`: two modes, the migration and the settings tidy (decision 2), the simplified states and `describe`, one set of setup steps; the panel calls the tidy on load and the new `describe`. Tests: setup 1–3 kept; setup 4–6 and the ChatMusician steps check removed; new checks below.
- [x] 6. Delete `ChatMusician.h/.cpp`, their two `CMakeLists.txt` lines, `runChatMusicianRangeTests` with its include and call, and `OllamaClient::generateRaw`.
- [x] 7. Reading guide: remove *ABC and generated music* (enum, list, name, summary, text, picture); the `gen::guides` entry's "also opens from" and search words (see *The guide*); `SOURCES.md` loses the abcnotation.com line under *Reading music*. Tests: the reading guide's count 15 becomes 14, the example check drops `generated`.
- [x] 8. Build. Done when: `./build.sh --test` ends with `All checks passed`, no new warnings; `grep -ri chatmusician src tests CMakeLists.txt` finds only the migration in `AiSetup` and its test.

Group C, check and docs:
- [x] 9. Real-app check, `--screenshots` on the example song at 1440 x 900 and 960 x 600, Keys clip selected: `generateMode: pattern`; `generateMode: pattern` with `generateStage: true`; then `generateApply: apply`; then `generateApply: undo`; `generateMode: ai` with `generateCannedTakes: true` and `generateClearChords: true` (three takes, a chord line, the Apply choice); each `aiSetup` state (`unreachable`, `noModels`, `notInstalled`) with `aiSetupSteps: true`; the reading guide's topic list. Look for: two modes only, no gap where controls were, the footer fully visible at 960 x 600, Undo apply restoring the clip, the callout with the pull step, 14 reading topics. Also run once with settings holding `generator.mode = chatMusician` (a scratch settings file, if the runner supports one; otherwise state it is covered by the test).
- [x] 10. Docs: README (*Generate* bullet: two modes, drop "ChatMusician model not checked"; delete the ChatMusician paragraph under *Optional local AI setup*; re-read the *License note*, unchanged); `docs/tests.md` (drop the ChatMusician area and the ChatMusician parts of the footer and AI setup lines; add the migration); `docs/code-layout.md` (drop `ChatMusician`; `AiSetup` and `GeneratePanel` lines say two modes); `docs/architecture.md` (staging line two modes; delete the ChatMusician paragraph); `docs/ui-design.md` (*The Generate panel*: two modes); `docs/recipes.md` (the screenshot keys of step 3).
- [x] 11. Plans and issues: `chatmusician-generation.md` to `Status: Superseded` with its note (decision 4); *Superseded* in `plans/README.md`'s status list; a note under the status of `patterns-and-ai-preview-and-apply.md`; this plan to *Built, not committed*. Run the grep of decision 5 over `src tests CMakeLists.txt README.md docs SOURCES.md` and record what it finds. Update #194 (progress, checks) and #182 (its body says three modes and "Test ChatMusician prerequisites": add a comment that #194 removed them, or edit with decision 1 (b)); #103 stays closed.

## Tests
- Modes: `getModeFromId ("chatMusician")` is the AI part; "pattern" and "ai" round-trip through `getModeId`; an unknown id is the pattern; `migrateMode (true, "")` and `migrateMode (false, "qwen2.5:7b")` are the AI part, `migrateMode (false, "")` the pattern.
- Settings tidy: a `PropertySet` with `generator.mode = chatMusician`, `generator.chatMusicianModel`, `generator.chatTask`, `generator.modelChecks` and `generator.model = qwen2.5:7b` ends with `generator.mode = ai`, the three keys gone and `generator.model` unchanged; one with `generator.mode = pattern` and none of them is unchanged.
- Setup: unreachable, no-models, not-installed and ready keep their texts (setup 1–3 as now); an installed chosen model is ready; Built-in pattern is not-needed and can start; the setup steps have four lines and step 3 names `ollama pull qwen2.5:7b`.
- Reading guide: 14 topics, each with a summary, a text and a picture; *How DawIt writes the score* is followed by *Your part*; no topic text or summary contains "ABC" or "ChatMusician"; the catalogue's reading guide search words don't contain "musician".
- Kept: the staging tests (`runGenerateStagingTests`), the Ollama and Lyrics AI tests, the save-and-load tests, unchanged and passing.

## Website hand-off
- After this lands (and with #182's pending website update folded in, decision 1): `features.html` songwriting / generation group describes two modes (Built-in pattern offline, AI part), staging and the readiness line; remove "and ChatMusician is offered in 4/4 songs only" from the time-signature *Limits* line (`features.html` line 363). Search every page for "ChatMusician" and "ABC".
- Screenshot worth taking: the Generate panel on Keys with a staged pattern take (1440 x 900). No Home, `learn.html` or `about.html` change; no library or model changes.

## Left for later
- A music-specific local model, if a better one appears: a new issue with its own licence check.
- Harmonising a melody (was open in #103 and #182).

## Changes while building

**Group A hand-off (12 November 2026).** Built; `./build.sh --test` ends with `All checks passed`, no warnings.
- `GeneratePanel` has two modes only (`modeButtons[2]`); all ChatMusician controls, `generateChatMusician`, `onShowReadingGuide` (panel member), `modelChecks`, `checkLine`, `testAgainButton`, `chatModelNames` and the 4/4 note are gone. Right-click ends with *Styles of music...*. `PianoRoll::onShowGeneratedMusicGuide` and the `MainComponent` jump are gone (the Score view's own `onShowReadingGuide` stays). `setCannedSetup` knows `unreachable`, `noModels`, `notInstalled`. `DevScreenshots.cpp`: `generateMode` is `pattern` or `ai`, the `chatMusician` block and its keys are gone, `chatApply` is `generateApply`, `chatClearChords` is `generateClearChords`.
- Deliberately left for Group B (the panel still compiles against the old `AiSetup`): `GeneratePanel.cpp` line ~312 still calls `ai::migrateMode (s.getBoolValue ("generator.chatMusician", false), ...)`, so `grep -i chatmusician src/ui` finds that one literal until step 5 replaces it with the settings tidy. A small guard after it maps any mode above `ai` (the old third value, which `getModeFromId` still returns for "chatMusician") to `ai`; delete the guard when `Mode` has two values.
- Panel calls `ai::describe (mode, modelList, url, model, {})` with an empty `ModelChecks`; when step 5 drops that parameter, change that one call in `updateSetup()`. `setupInfo.canStart()` is still used for the AI part. `getSetupSteps (mode)` is still called with the panel's mode; the callout always shows the pull button now, so step 5's single set of steps (four lines) must keep step 3 naming `ollama pull`.
- `aiSetupSteps` shots: the callout height is now fixed at three buttons.
- Not yet run in the real app (Group C).

**Group B hand-off (12 November 2026).** Built; `./build.sh --test` ends with `All checks passed`, no warnings.
- `AiSetup`: `Mode` has `pattern` and `ai`; `getModeFromId ("chatMusician")` is `ai`; new `tidySettings (juce::PropertySet&)` maps the saved mode and removes `generator.chatMusicianModel`, `generator.chatTask` and `generator.modelChecks`; `describe (mode, list, url, model)` has no check records; `getSetupSteps()` takes no mode (four lines, step 3 names `ollama pull qwen2.5:7b`). `SetupInfo` lost `checkLine` and `offersTestAgain`; `State` lost `noChoice`, `needsCheck`, `checkFailed`. `GeneratePanel` calls `tidySettings` on load; the temporary mode guard is gone. `migrateMode (true, "")` is `ai`.
- Deleted `ChatMusician.h/.cpp` (both `CMakeLists.txt` targets), `OllamaClient::generateRaw`, `runChatMusicianRangeTests`. Reading guide has 14 topics (`generatedMusic` removed); `TheoryGuides` "also opens from" and search words updated; `SOURCES.md` lost the abcnotation.com line (the *Reading music* heading now goes straight to *Pages read*).
- Tests: setup 4-6 removed; new checks for mode ids, migration, settings tidy and four setup steps; reading guide count 14.
- `grep -ri chatmusician src tests CMakeLists.txt` finds only: the migration literals in `AiSetup.h/.cpp` and `GeneratePanel.cpp` (the old `generator.chatMusician` switch read), and in `EngineTests.cpp` the migration test and the `ChatMusician-Q4` model-list fixture name (harmless; Group C may rename it if it wants an exact grep).
- Group C: `docs/tests.md`, `docs/code-layout.md` and others still describe the removed code; SOURCES.md line 1156 (research record) stays by decision 5.

**Group C hand-off (12 November 2026).** Built; `./build.sh --test` ends with `All checks passed`, no warnings (0 lines containing `warning:`).
- Real-app check (`--screenshots`, scale 1, Keys clip): at 960 x 600 `generateMode: pattern`, staged pattern, after `generateApply: apply` and `undo`, `ai` with `generateCannedTakes` and `generateClearChords`; at 1440 x 900 staged pattern, canned AI takes, the reading guide's topic list, and `aiSetup` `unreachable`, `noModels`, `notInstalled` with `aiSetupSteps` (needs `generateScroll: 130` to bring the callout into view in the short settings viewport). Seen: two modes only, no gap, footer fully visible at 960 x 600, *Undo apply* restores the clip (status "Undone. The song is as it was before Apply."), the callout has four steps with `ollama pull qwen2.5:7b`, the reading guide lists 14 topics (*How DawIt writes your MIDI* then *Your part*, then Sources). The old `generator.mode = chatMusician` start-up is covered by the settings-tidy test: the panel loads its settings when the piano roll is built, before a shot's `settings` apply, so a shot can't show it.
- Docs: README (*Generate* bullet two modes; ChatMusician paragraph removed; *License note* and the `LICENSE` Ollama paragraph re-read, unchanged), `docs/tests.md`, `docs/code-layout.md`, `docs/architecture.md`, `docs/ui-design.md`, `docs/recipes.md` (old `chat*` shot keys removed, `generateApply`, `generateClearChords`).
- Plans: `chatmusician-generation.md` is `Superseded` with its note; *Superseded* is in `plans/README.md`'s status list; note under `patterns-and-ai-preview-and-apply.md`'s status.
- Decision 5 grep over `src tests CMakeLists.txt README.md docs SOURCES.md` finds only: the migration literals (`AiSetup.h/.cpp`, `GeneratePanel.cpp` old `generator.chatMusician` switch), the migration tests and the `ChatMusician-Q4` model-list fixture name in `EngineTests.cpp`, one line in `docs/tests.md` naming the migration, the research line in `SOURCES.md` (1154), and `docs/usability-review-2026-11-10.md` with its `more.json`. All allowed.
- Website hand-off: as under *Website hand-off* above, once, after both #182 and #194 are committed. `features.html` generation group: two modes, staging, readiness line; remove "and ChatMusician is offered in 4/4 songs only" from the time-signature *Limits* line; search every page for "ChatMusician" and "ABC". Screenshot worth taking: the Generate panel on Keys with a staged pattern take at 1440 x 900 (`generateMode: pattern`, `generateStage: true`). No library, model or guide change for `about.html` or `learn.html`.
- No fact, number or text beyond the plan was needed.
