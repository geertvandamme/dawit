# Use built-in patterns and optional AI with clear preview and apply steps

Status: Released

> Note (12 November 2026): [remove-chatmusician.md](remove-chatmusician.md) (#194) removed the *ChatMusician* mode described below; the Generate panel has two modes. The text below is what was true when this was built.
Idea: [#182 — Use built-in patterns and optional AI with clear preview and apply steps](https://github.com/geertvandamme/dawit/issues/182)

Package 11 of 12 of [usability-improvements.md](usability-improvements.md) (#173), audit findings 46–50. Builds on the released [Generate panel footer](generate-panel-fixed-footer.md) (package 02). Package 04 (#151) has no child plan yet; this plan follows its stated conventions: one user action is one named history step, the affected target is labelled, and an undo for the action sits next to it.

## Goal
The Generate panel offers three clear modes: **Built-in pattern** (no AI, works offline), **AI part** and **ChatMusician**, each with its own prerequisites in plain words. Every mode follows the same promise: Generate, then Preview on the track's own instrument, then Apply (one undo step) or Discard (nothing changes), with the target clip, the replace/add scope and any proposed chords always visible. When the AI is not ready, the panel says exactly why (Ollama not running, no model, model not installed, ChatMusician model unchecked) and offers Retry and copyable setup steps. In the Lyrics window, *Use this line* and *Apply lyrics to song* separate trying a line from changing the song, with a "Not applied yet" mark.

## Reconciling the ChatMusician plan
[chatmusician-generation.md](chatmusician-generation.md) is *In progress*; #103 is closed. Verified in the code: melody and chord tasks, chunking, validation, staging, a sine-tone preview limited to two minutes (`startAudition`), separate *Apply MIDI* / *Apply chords* and a snapshot-checked *Undo ChatMusician* are built. This plan **keeps** its engine (`gen::chatmusician`, `OllamaClient::generateRaw`) and scope (natural quarter notes, 4/4 whole bars, no harmonisation), and **replaces** only its panel workflow: the sine preview, its two-minute limit and its own undo. Its open checks (audible preview, save/reload after apply, cancel while responding) are covered here. Harmonising a melody and ABC parsing stay out (*Left for later*). Group C marks that plan as superseded in its UI parts.

Verified current behaviour that this plan changes:
- AI part puts the best take straight into the clip; Cancel during a run applies the takes that already arrived; a result is applied even if the clip was edited meanwhile (only deletion is checked).
- "Play style" (a playing style without the AI) applies at once; built-in patterns only exist for Chords, Arpeggio and Bassline and need a chord on every bar.
- With no model, the status says "In a terminal, run: ollama pull qwen2.5:7b"; unreachable server and empty list are told apart only in the status text; ChatMusician mode shows the general model (`generator.model`, finding 48).
- The Lyrics window's *Cancel* closes without asking, even with typed changes.

## Decisions for you
1. **Stage built-in patterns too.** Options: patterns apply at once as now; patterns stage like the AI. **Recommended:** stage them, so every mode has one promise; changing the playing style restages instantly, which makes trying styles quicker than now.
   Answer: Stage them (as recommended).
2. **Preview sound.** Options: keep the sine-tone audition; play the staged take on the track's own instrument through a transient overlay on the clip. **Recommended:** the overlay: the user hears the real sound in the song, with no length limit. It is an audio-thread change (see *Code*).
   Answer: The overlay on the track's own instrument (as recommended).
3. **Fast workflow.** Options: none; an app setting *Apply straight away (skip preview)*, default off. **Recommended:** include it: Generate then applies the best take as one history step, with no take buttons.
   Answer: None: no skip-preview setting. Every mode always stages first (the user chose against the recommendation).
4. **One-step undos (#152, Generate part).** Options: keep *Undo* and *Undo ChatMusician*; fold them into the history. **Recommended:** fold: Apply is one named history step, and the footer's single *Undo apply* runs the app's Undo while that step is the latest. The other three one-step undos of #152 stay for #152.
   Answer: Fold them into the history, with one *Undo apply* button (as recommended).
5. **ChatMusician compatibility.** Options: warn only; require a passed one-bar format check before the first Generate with a model, run automatically and remembered per model digest. **Recommended:** require it; it takes about one short reply and is the only evidence DawIt can get. A pass is not proof that the model is ChatMusician.
   Answer: Require the one-bar format check, run automatically and remembered per model digest (as recommended).
6. **ChatMusician download.** Options: name no download; name the locally tested `hf.co/mradermacher/ChatMusician-GGUF:Q4_K_M`. **Recommended:** name none until its provenance, base-model (Llama 2) terms and hash are verified (open in the ChatMusician plan).
   Answer: Name no download yet (as recommended).
7. **Button names.** *Cancel* stays on the Generate button while a request runs; the staged take's button is *Discard*. **Recommended:** as written, so two Cancel buttons never show together.
   Answer: As written: *Cancel* while running, *Discard* for the staged take.
8. **Lyrics window closing with unapplied text.** Options: close silently; ask "Apply your changes to the song?" with *Apply lyrics to song*, *Discard changes*, *Keep editing*. **Recommended:** ask.
   Answer: Ask, with *Apply lyrics to song*, *Discard changes*, *Keep editing* (as recommended).

## What the user does and sees
**Piano roll Generate panel** (opened as now: *Generate...* in the piano-roll header or its *More...* menu). Tooltip of *Generate...*: "Make a part for this clip: a built-in pattern (no AI needed) or a part written by a local AI model."

```
Generate                                     x
A minor   4/4   96 BPM
[ Built-in pattern | AI part | ChatMusician ]
One line on what this mode needs
---- settings (scroll) ----
Pattern:      Part (Chords, Arpeggio, Bassline) · Style · Playing
AI part:      AI status line [Retry] [Set up...] · Model · Part · Style · Playing (Free) · Vary · Prompt · Variation, takes · Think
ChatMusician: AI status line [Retry] [Set up...] · ChatMusician model · check line · Task · chord choices · How to read music...
Apply straight away (skip preview)
---- footer (fixed) ----
Into: Keys · "Chords" · bars 1–4        (Replace notes | Add to notes)
Chords: adds Am F C G to empty bars 1–4 (bar 3 keeps F)
[ Make pattern | Generate | Cancel ]
Take 1 (82)  Take 2 (75)  Take 3 (61)
[Preview] [Apply] [Discard]  Apply: (Notes and chords | Notes only | Chords only)
[Undo apply]
status (up to three lines)
```

- **Modes:** a three-button radio group, Tab to reach, arrow keys to move, each with an accessible title. Mode lines: *Built-in pattern* "DawIt plays a pattern from its own library over the chord track. No AI needed; works offline." *AI part* "A local AI model on your computer writes the part. Needs Ollama and a model." *ChatMusician* "A separately installed ChatMusician model writes a simple melody or chord progression. 4/4 only." Main button: *Make pattern*, *Generate*, *Generate* (*Cancel* while a request runs).
- **Built-in pattern** never contacts Ollama and shows no model controls. Melody, drums and percussion are offered in AI part only (part box note: "Melody, drums and percussion need AI part."). Bars without a chord: the button is disabled and the line says "Bars 3–4 have no chord. Add chords on the Chords row (right-click selected bars for a random progression), or use AI part."
- **Staging:** a result is never written into the clip by Generate. The piano roll shows staged notes as outlines over the clip's notes, the footer says "Take ready, not applied. Preview, then Apply or Discard." *Preview* plays the song from the clip's start through the clip's end and stops (press again to stop); while a take is staged, any playback of that clip plays the take (replace) or both (add). Take buttons choose which take is staged. *Apply* writes it as one undo step named "Generate melody in 'Keys'" (part name, track) and ends the staging; the status says "Applied to Keys, bars 1–4. Cmd+Z or Undo apply takes it back." *Discard* removes it; the song never changed.
- **Proposed chords** (AI part asking for bar chords, ChatMusician chord task) are listed in the footer; only empty chord-track bars are ever filled, and kept bars are named. The *Apply* choice appears only when chords are proposed.
- **Stale results:** a change to the target clip's notes, start or length, its deletion, another project, the key, the time signature or a chord in its bars ends the staging (or rejects a result as it arrives): "The clip or its chords changed while this take was waiting, so it was set aside. Generate again." Closing the panel or switching clip or mode discards a staged take.
- **Cancel** during a request stops it and stages nothing, even takes that already arrived ("Cancelled. The song is unchanged."). A raw ChatMusician request may still finish inside Ollama; its reply is ignored.
- **AI status line** (AI part, ChatMusician): "Connected to Ollama · 3 models"; "Ollama isn't running at http://localhost:11434."; "Ollama is running but has no models installed."; "'llama3:8b' isn't installed in Ollama." Each problem state shows *Retry* (replaces *Refresh*), *Set up...* and "Or use Built-in pattern, which needs no AI" as a link button.
- **Set up...** opens a non-modal callout with steps and a *Copy* button per command: 1. Install Ollama from ollama.com (*Open ollama.com*). 2. Start it: open the Ollama app, or run `ollama serve`. 3. Get a model: `ollama pull qwen2.5:7b` (a 4.7 GB download; any chat model works). 4. Press Retry. In ChatMusician mode step 3 reads: "Install a ChatMusician model in Ollama yourself. DawIt doesn't name a download yet, because its licence and origin haven't been checked. Then choose it here; DawIt tries it with a one-bar test before first use."
- **ChatMusician model list** is separate from the AI part's (`generator.chatMusicianModel`). Models with "chatmusician" in the name come first; the others sit under "Other models (not ChatMusician by name)". Check line: "Not checked yet: DawIt will ask it for one test bar first." / "Passed the one-bar test on 11 Nov 2026. This shows it follows the format, not that its music is good." / "Failed the one-bar test: its reply didn't follow the format. It may not be a ChatMusician model." with *Test again*. Generate with an unchecked model runs the check first ("Checking the model with one test bar...").
- **Right-click** on the panel's background: the footer's actions, then the guide (*How to read music...* in ChatMusician, *Styles of music...* otherwise).

**Lyrics window:** *Use* becomes *Use this line* (*Use these lines* for several lines), tooltip "Put this alternative in the text box. The song changes only when you choose Apply lyrics to song." *Apply* becomes *Apply lyrics to song*. Beside it, "Not applied yet" (yellow, `theme::meterYellow`) shows whenever the text differs from the song's lyrics as opened; the existing "changed outside this window" note stays. Closing with unapplied text asks (decision 8). Syllable and rhyme feedback is unchanged.

**App settings:** `generator.mode` (`pattern`, `ai`, `chatMusician`; migrated once from `generator.chatMusician`: true gives `chatMusician`, else `ai` when `generator.model` is set, else `pattern`), `generator.chatMusicianModel`, `generator.applyAtOnce` (false), `generator.modelChecks` (per digest: passed or failed and the date).

## Data and saving
No project data and no format change. The staged take, the overlay and the model checks are never saved, exported, bounced or in the undo history. Applied notes and chords are ordinary clip notes and chord-track chords. Tests: saving while a take is staged writes the clip's own notes; after Apply, save and load give the applied notes and chords.

## The guide
No new guide or topic. The panel's guide entries stay: the reading guide's *ABC and generated music* (ChatMusician) and the Styles of music guide (patterns, AI part). The reading guide's ChatMusician text stays true ("Preview the result before applying it"). Add "pattern generate preview apply" to the Styles of music guide's search words and "Generate panel" to its "also opens from".

## Code
Recipes: *A command or menu item* (right-click menu), *Checking in the real app*; ui-design.md *The Generate panel*.
- **Staged-take overlay:** a MIDI clip can carry transient preview notes, replacing or adding to its own, set and cleared by the message thread under `Engine::getLock()`; `Track`'s note rendering plays them; bounce, freeze, export, `ProjectFile`, undo snapshots and the clip's own `notes` ignore them. Setting or clearing goes through `notesChanged()` so hanging notes are released. Cleared on project load, clip removal and panel close.
- **Target snapshot** (UI-free, `src/generate`): edit generation, clip identity, start, length, notes, the chords in its bars, key and time signature, with a staleness check; generalises `chatSnapshotValid`. A result becomes notes fitted to the clip's length (replace or add) and chord proposals split into added and kept bars.
- **Apply** is one engine call: under the lock, notes (and chosen chords, empty bars only) and the clip name as today, then `nameNextStep` and `commitHistory`, so it is exactly one step. *Undo apply* is enabled only while `getUndoName()` and the step count match what Apply recorded; it runs MainComponent's Undo command.
- **AI setup state** (UI-free): `OllamaClient::listModels` returns name, digest and family, and says whether the server answered; a pure function maps that plus the chosen model, mode and check record to a state and its text. The ChatMusician check asks one bar over C with the existing prompt and `parseReply`.
- **Invariants:** only Apply changes the song; Built-in pattern makes no network call; no path changes an instrument; results with an old request id are dropped.
- **Risks:** the audio-thread overlay (keep it a pointer read under the lock the callback already holds; nothing allocates on the audio thread); removing two shipped undo buttons; settings migration.

## Libraries and models
None. Qwen2.5 7B (Apache 2.0) is named only as a model the user installs; DawIt neither ships nor downloads it. No ChatMusician model is named (decision 6).

## Theory
None. The feature adds UI text, not musical facts.

## Steps
Group A, engine (`./build.sh --test` at step 4):
- [x] 1. Staged-take overlay on `MidiClip` and its rendering in `Track` (replace and add); bounce, save, snapshot ignore it. Tests: overlay 1–6 in *Tests*. Done when the tests pass through the fake device.
- [x] 2. Target snapshot, staleness and result fitting in `src/generate` (new `.cpp` in both CMake targets). Tests: staging 1–6.
- [x] 3. Engine apply as one named history step with notes, chosen chords in empty bars and clip name; undo restores exactly. Tests: apply 1–6.
- [x] 4. `listModels` entries and reachability, the setup-state function and texts, ChatMusician model ordering, the one-bar check and the per-digest record. Tests: setup 1–7. Run `./build.sh --test`.

Group B1, UI: Generate panel (`./build.sh --test` at step 8):
- [x] 5. Mode radio group, mode lines, per-mode controls, button names, settings migration, *Generate...* tooltip; Built-in pattern never calls Ollama; restage on playing-style change. Check: Built-in pattern makes a take with Ollama stopped.
- [x] 6. Footer: Into line with Replace/Add, chord line, take buttons, Preview / Apply / Discard, Apply choice, *Undo apply*; ghost notes in `PianoRoll`; stale handling through the change listener; Cancel stages nothing; *Apply straight away*. Remove *Undo*, *Undo ChatMusician* and the sine preview. Check: Discard leaves notes and chords equal; Cmd+Z undoes one Apply.
- [x] 7. AI status line, *Retry*, *Set up...* callout with Copy buttons, link to Built-in pattern; ChatMusician model list, check line and automatic check. Check: with Ollama stopped the line and Retry appear.
- [x] 8. Right-click menu, tab order, accessible titles; `ScreenshotRunner` keys `generateMode` (`pattern`, `ai`, `chatMusician`), `generateStage: true` (stage a pattern take without applying), `aiSetup` (`unreachable`, `noModels`, `notInstalled`, `unchecked`, `failed`, a canned state without Ollama), `chatApply` gains `apply` and `discard`. Run `./build.sh --test`.

Group B2, UI: Lyrics window (`./build.sh --test` at step 9):
- [x] 9. *Use this line* / *Use these lines*, *Apply lyrics to song*, "Not applied yet" mark, close question (decision 8); `lyricsEditor` scene shows the mark when `"lyricsTyped"` sets text. Run `./build.sh --test`.

Group C, check and docs:
- [x] 10. Real-app check, `--screenshots` on the pop song, widths 1440×900 and 960×600: each mode on Keys; a staged pattern (ghost notes, Into line, footer); a staged AI take with three takes and a chord line (canned); after Apply and after Undo apply; each `aiSetup` state with the Set up callout open; ChatMusician unchecked and failed; the lyrics window with "Not applied yet". Look for cut text, the footer fully visible, and the song unchanged before Apply. Keyboard: reach every mode, Preview, Apply and Discard with Tab; VoiceOver names spoken (direct check, state anything unverified).
- [x] 11. Live smoke test where Ollama is available: Built-in pattern with Ollama quit; AI part with `qwen2.5:7b` (Preview heard on Keys, Apply, Cmd+Z); ChatMusician model check and a four-bar melody over Am–F–C–G; Cancel during a request. Record model, digest, Ollama version and results in #182, not as quality claims.
- [x] 12. Docs: README Features (replace the Generate footer bullet; lyrics Write with AI wording) and *Optional local AI setup* (modes, no preview limit, check); docs/tests.md; docs/code-layout.md (`GeneratePanel`, the new generate file); ui-design.md *The Generate panel* (modes, staging footer, Discard/Undo apply) and architecture.md *Long jobs and models* (overlay, staging, ChatMusician paragraph); chatmusician-generation.md status note; #182 progress, #152 (Generate part done), #173 row.

## Tests
Overlay:
1. A clip with 4 notes and an overlay of 2 replacing notes plays 2 note-ons over its bars through the fake device; with add, 6.
2. Clearing the overlay plays the 4 again; no note is left hanging after clearing mid-note.
3. A bounce of the track ignores the overlay (4 note-ons' worth, same as without).
4. Saving with an overlay writes the clip's 4 notes; `commitHistory` records no step for setting or clearing it.
5. Removing the clip or loading a project clears it.
6. The instrument and built-in sound are unchanged throughout.

Staging:
1. A fresh snapshot is valid; each of note change, start, length, deletion, new project, key, time signature and a chord change in its bars makes it stale; a tempo change or a chord change outside its bars does not.
2. Replace gives the result's notes; add appends them to the clip's.
3. Notes starting at or after the clip's end are dropped; a note crossing it is shortened to end there.
4. Proposed chords Am F C G over bars where bar 3 has F: added bars 1, 2, 4, kept bar 3.
5. A result with an old request id is rejected.
6. A cancelled run yields no staged take, even with takes already arrived.

Apply:
1. Apply adds exactly one history step, named "Generate melody in 'Keys'".
2. Undo after Apply restores the notes, the chords and the clip name exactly; redo puts them back.
3. "Chords only" changes no note; "Notes only" changes no chord.
4. Discard (no Apply) leaves the edit equal to its snapshot before Generate.
5. Save and load after Apply give the applied notes and chords.
6. A Built-in pattern request over chords on every bar realises notes with an unreachable server URL (`http://127.0.0.1:1`).

Setup:
1. An unreachable URL gives the not-running state and its text with the URL.
2. A reply with no models gives the no-models state; a chosen name not in the list gives not-installed.
3. Model entries keep digest and family; embedding models are still left out.
4. ChatMusician ordering puts names containing "chatmusician" (any case) first.
5. A canned valid one-bar reply passes the check; a malformed one fails; the record is keyed by digest, so a new digest is unchecked.
6. An unchecked or failed ChatMusician model is not ready; a passed one is.
7. The migration of `generator.chatMusician` and `generator.model` gives the modes listed in *App settings*.

## Website hand-off
- `features.html`, the songwriting / generation group: three modes, offline patterns, Preview on the track's instrument, Apply as one undo step.
- Screenshots: a staged Built-in pattern take; the AI status line with *Set up...* open; the Lyrics window with "Not applied yet". No Home, `learn.html` or `about.html` change.

## Left for later
- Harmonising a supplied melody and ABC parsing (#103 deferred scope; needs an open issue, since #103 is closed).
- A downloadable, verified ChatMusician model (provenance and Llama 2 terms unverified).
- Built-in patterns for melody, drums and percussion.
- An arrangement-view mark on a clip with a staged take (the panel and piano roll show it).
- The other one-step undos of #152 (transform, random progression, arpeggiator write-out).
- A UI for the Ollama server address (`generator.ollamaUrl`).

## Changes while building

### Hand-off from Group A (engine)
Built and tested (`./build.sh --test`: All checks passed, no warnings; new checks are in `runGenerateStagingTests`).
- **Overlay:** `MidiClip::preview` (`StagedPreview`: `active`, `replacesNotes`, `notes`; a copy of a clip carries none). Set with `Engine::setClipPreview (clip, notes, replacesNotes)` and clear with `Engine::clearClipPreview (clip)`; both go through `notesChanged()`, which releases held notes, and neither records a history step. `Track::collectMidiClipEvents` plays it; offline renders (bounce, freeze) set `RenderContext::ignorePreview`. `removeClip` and `resetEdit` clear previews. For add mode, pass only the take's notes with `replacesNotes = false`.
- **Staging** (`src/generate/GenerateStaging.h/.cpp`, namespace `daw::gen::staging`): `Target::capture/isValid`, `fitNotes`, `splitChordProposal` (bars are song bars from 0) and the `Staging` class: `begin (engine, clip)` returns a request id (and discards any staged take), `cancel()`, `deliver (engine, id, notes, chords, replace)` returns `Outcome::{staged, oldRequest, cancelled, stale}`, `getTake()` (notes fitted to the clip, one chord per bar), `getNotesAfterApply()`, `setReplace()`, `isStale (engine)`, `discard()`. The panel still owns the overlay: call `setClipPreview (clip, staging.getTake().notes, staging.isReplacing())` after `staged`, and `clearClipPreview` on discard, apply, close or stale. `Target::isValid` also compares the edit generation, so a new project is stale.
- **Apply:** `Engine::applyGenerated (clip, finalNotes, addedChordBars, scope, clipName, stepName)` returns `GenerateApplied { applied, undoSteps, stepName, chordsAdded, chordsKept }`. It commits pending edits first, writes notes (`finalNotes` = `staging.getNotesAfterApply()`), only empty chord bars, the clip name (skipped when empty), ends the preview, then records one named step. `Engine::GenerateScope` is `notesAndChords`, `notesOnly`, `chordsOnly`. For *Undo apply*: keep `undoSteps` and `stepName` and enable the button while `engine.getNumUndoSteps() == undoSteps && engine.getUndoName() == stepName`. Build `addedChordBars` from `splitChordProposal (engine.getChords(), firstBar, take.chords).added` (the engine skips occupied bars anyway). The step name is passed in; the panel builds "Generate melody in 'Keys'".
- **AI setup** (`src/generate/AiSetup.h/.cpp`, namespace `daw::gen::aisetup`): `Mode` with `getModeId/getModeFromId/migrateMode (oldChatMusicianOn, oldModel)`; `OllamaClient::listModelInfo()` returns `ModelList { reachable, models (name, digest, family), error }` (`parseModelList` for canned JSON; `listModels` still works for the Lyrics window); `describe (mode, list, url, chosenModel, checks)` returns `SetupInfo { state, status, checkLine, ready, offersRetry, offersTestAgain, canStart() }`; `orderForChatMusician`; `ModelChecks` (`get/set/toString/fromString`, for `generator.modelChecks`, keyed by digest); `getSetupSteps (mode)`; `runOneBarCheck (request, error)`, where `request` has the same shape as the one `generateMelody` takes (prompt, schema, error out), so the panel wraps `generateRaw` with `maxTokens` about 384 and its cancel flag. An unchecked model has `state == needsCheck`, `ready == false` and `canStart() == true`: Generate runs the check first, stores the result with `ModelChecks::set (digest, passed)`, and only continues on a pass.
- **Surprises:** none in the design. Notes: `Staging::deliver` rejects a result as `stale` when the target changed in the meantime; the Cancel rule of the plan (stage nothing) is `cancel()`. The unreachable-server test for patterns (apply 6) checks that `gen::realize` needs no client and that `127.0.0.1:1` is unreachable; the structural guarantee that Built-in pattern never calls Ollama is for group B1 to keep (do not call `OllamaClient` in that mode).
- Nothing in `src/ui` was touched; `GeneratePanel` still uses its old flow until group B1.

### Hand-off from Group B2 (Lyrics window)
`src/ui/LyricsEditWindow.h/.cpp`: buttons are *Use this line* (*Use these lines* when the selected alternative has several lines, *Use this word* for rhymes), *Apply lyrics to song*; the yellow "Not applied yet" label (`unappliedMark`) sits left of Apply and shows while the text differs from `openedText`. Closing: the title-bar X goes through a new `daw::CloseGuard` (in `PluginWindows.h`; `PluginWindowManager::Window::closeButtonPressed` asks the content first); `LyricsEditWindow::requestClose` asks with *Apply lyrics to song* / *Discard changes* / *Keep editing*. *Cancel* (the window's own button) still closes without asking, since it says so. `ScreenshotRunner` `lyricsEditor` scene takes `"lyricsTyped": "<text>"` (via `setTypedText`) to show the mark. For Group C: the close question itself was not clicked in the real app; docs (README lyrics wording, `CloseGuard` in code-layout, recipes `lyricsTyped`) are still to do.

### Hand-off from Group B1 (Generate panel)
Built (`./build.sh --test`: All checks passed, no warnings). `src/ui/GeneratePanel.h/.cpp` is rewritten; nothing else in `src/generate` or the engine changed.
- **Panel:** `GeneratePanel` has the three-mode radio group (`setMode (aisetup::Mode, save)`, saved as `generator.mode`, migrated from `generator.chatMusician` / `generator.model` when unset), per-mode controls, the footer (Into line with *Replace notes* / *Add to notes*, chord line, *Make pattern* / *Generate* / *Cancel*, take buttons, *Preview* / *Apply* / *Discard*, the Apply choice box shown only when chords would be added, *Undo apply*), the AI status line with *Retry* (*Refresh* when all is well), *Set up...* callout (Open ollama.com, Copy buttons), the link button to Built-in pattern, ChatMusician model box (`generator.chatMusicianModel`, ordered by `orderForChatMusician`), check line and *Test again*; the automatic one-bar check runs inside `generateChatMusician (false)` before the first request (a request that could not be made is not recorded as a failed check). Right-click on the panel or its form background opens the menu (footer actions, then the guide). Tab order is set with `setExplicitFocusOrder`; every control has a title.
- **Staging:** every mode stages through `gen::staging::Staging`; the overlay is `engine.setClipPreview` (`showOverlay`/`clearOverlay`), Apply is `engine.applyGenerated` with the step name "Generate <part> in '<track>'", Discard/close/clip switch/mode switch/stale clear the overlay. *Undo apply* calls `onUndoApply` (PianoRoll -> MainComponent `stepHistory (false)`), enabled while `getNumUndoSteps()` and `getUndoName()` still match. The old Undo, Undo ChatMusician and sine preview are gone. Preview plays from the clip start to its end (the timer stops it).
- **PianoRoll:** `PianoRollGrid::paint` draws the staged take as white outlines over the clip's notes (the clip's own notes are dimmed in replace mode); new callbacks `onShowStylesGuide`, `onUndoApply` on `PianoRoll`, wired in `MainComponent`; the *Generate...* tooltip is the plan's.
- **ScreenshotRunner keys:** `generateMode` (`pattern`, `ai`, `chatMusician`; also opens the panel), `generateStage: true` (pattern only: stages a take), `aiSetup` (`unreachable`, `noModels`, `notInstalled`, `unchecked`, `failed`) with `aiSetupSteps: true` to open the callout, `chatApply` is now `apply`, `midi` (notes only), `chords` (chords only), `discard` or `undo`. `generatePanel: true` still works (pattern mode); `chatMusician: "melody"|"chords"` now sets ChatMusician mode. `docs/recipes.md` still describes the old keys: Group C.
- **Departures from the plan:** (1) No *Apply straight away* setting (decision 3). (2) No *Vary it with the AI* toggle: Built-in pattern plays a style as it is, AI part always varies a chosen style (Free lets the model write all of it); `generator.varyStyle` is no longer read. (3) The footer's chord line shows only the missing-chords message (Built-in pattern) or the proposed chords of a staged take; the plain chord list is no longer shown. (4) The ChatMusician model box is not editable (choose from the list). (5) Canned AI takes cannot be staged by a shot (the AI needs Ollama); for a chord line or three takes in Group C, stage a pattern take (pattern takes have one take and no chords) or add a canned-take scene to `ScreenshotRunner`.
- **For Group C:** in the 338-402 px high bottom panel the footer (about 170 px with a staged take) leaves little room for the settings, which scroll; check this at 960x600. Real-app checks, docs (README, tests.md, code-layout.md for `GeneratePanel`, recipes `generatePanel` keys, ui-design.md, architecture.md) and the Styles guide search words ("pattern generate preview apply", "Generate panel") are still to do. The panel's staged-take, Apply, Discard, Cancel and Undo apply flow was only exercised through screenshots (pattern take staged with outlines, canned setup states), not by clicking: the live smoke test (step 11) covers Preview, Apply and Cmd+Z.

### Hand-off from Group C (check and docs)
**Real-app check** (`--screenshots`, scratch lists in the session scratchpad; windows 1440x900 and 960x600; all looked at). Seen working: each mode, a staged pattern take (outlines, Into line, footer), three canned AI takes with a chord line and the Apply choice, Apply (one step, status "Applied to Keys", Undo apply shown) and Undo apply (notes, chords and the clip name back), each `aiSetup` state with the Set up callout, ChatMusician unchecked and failed with *Test again*, the Lyrics window with "Not applied yet" (980 and 600 px wide). The song was unchanged before Apply in every shot.
**Fixed while checking:**
- At 960x600 the bottom panel is only 183 px: the footer with takes, chords and the Apply choice (about 190 px) pushed the buttons off the top and the status off the bottom. `GeneratePanel::resized` now tightens its rows below 280 px, shrinks the status to one line (14 px, so no half line peeks) and, in a very short panel, drops the chord line (its text stays the tooltip). All buttons and one status line now fit; the settings above get little room and scroll.
- The staged and applied status texts now start with a short first sentence ("Take ready, not applied." / "Applied to Keys, bars 1-4.") so the one-line status is meaningful.
- A mode switch clears the previous mode's status ("Undone...", an apply message).
- `NoteRealizer`'s pattern summary names only the first eight chords then "..." (a 36-bar clip filled the status with chords).
**ScreenshotRunner additions:** `generateCannedTakes` (three canned AI melody takes with chords Am F C G, via `GeneratePanel::stageCannedTakes`), `chatClearChords` for it, `generateScroll`, `generateRun` / `generateModel` / `generateTakes` (a live Generate against Ollama), and `aiSetup` now closes the callout unless `aiSetupSteps` is true. `docs/recipes.md` lists all the keys.
**Live smoke test (step 11)**: see the issue comment and the report; Ollama 0.40.8 was running with qwen2.5:7b (digest 845dbda0ea48) and the ChatMusician GGUF (digest ae24476b37a1). AI part with qwen2.5:7b staged a take, Apply and Undo apply restored the song exactly. Not run: Built-in pattern with Ollama quit (the user's Ollama was left running; the unreachable-URL test covers it), the audible Preview, Cancel during a request in the live app.
**Not verified:** VoiceOver names (set in code, not spoken), Tab reaching every mode/Preview/Apply/Discard (set with `setExplicitFocusOrder`, not driven), the Lyrics close question (not clicked), the Lyrics window at exactly 560 px.
**Docs done:** README (Generate bullet, Optional local AI setup, Lyrics wording), docs/tests.md, docs/code-layout.md, docs/ui-design.md, docs/architecture.md, docs/recipes.md, the Styles guide's search words and "also opens from", the superseded note in chatmusician-generation.md. #152 and the #173 row need no text change: they stay open for the other undos.

### Website hand-off
- `features.html`, songwriting / generation group: three Generate modes (Built-in pattern offline, AI part, ChatMusician), Preview on the track's own instrument with no limit, Apply as one undo step with Discard and Undo apply, the readiness line with *Retry* and *Set up...*, and the Lyrics window's *Apply lyrics to song* with "Not applied yet". No Home, `learn.html` or `about.html` change; no library or model added.
- Screenshots to take: a staged Built-in pattern take (`generateMode: pattern`, `generateStage: true`, `selectClip: Keys`, `capture: bottomPanel`); the AI status line with *Set up...* open (`generateMode: ai`, `aiSetup: unreachable`, `aiSetupSteps: true`, `generateScroll: 60`); canned AI takes (`generateCannedTakes: true`, `chatClearChords: true`); the Lyrics window (`scene: lyricsEditor`, `lyricsTyped`, `capture: window`).
