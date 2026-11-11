# DawIt usability implementation plan

Status: Planned
Idea: [#173 — Make DawIt easier to learn and use](https://github.com/geertvandamme/dawit/issues/173)
Source: [Usability review of 10 November 2026](../docs/usability-review-2026-11-10.md), findings 1–60.
Related issue: [#151 — A/B settings and undo for plugins, and presets for a whole effects chain](https://github.com/geertvandamme/dawit/issues/151).

## Goal

Make DawIt safe to explore, usable at supported window sizes and understandable without prior DAW experience. Preserve music and unblock everyday tasks first; then improve discovery, editing and advanced tools.

This umbrella plan maps all 60 findings to 12 ordered packages. Each package becomes a bounded child plan before coding; large packages split into the slices below. It does not replace existing feature plans or approve implementation.

## Decisions for you

1. **Navigation:** incremental repairs or a wholesale redesign. **Recommended:** incremental packages, retaining the arrangement and familiar menus/shortcuts.
2. **Window sizes:** increase the minimum or adapt. **Recommended:** retain 960 × 600, using overflow and focus views; also verify 1280 × 800 and 1440 × 900.
3. **Effect recovery:** global snapshots or explicit processor history. **Recommended:** built-in gesture Undo/Redo, Undo Load and A/B under #151 first; hosted plugins need separate compatibility design. Preserve accepted mute/solo exclusions in [the Undo plan](undo-and-redo.md).
4. **Crash recovery:** **Recommended:** local snapshots after edits settle, targeting at most 60 seconds of lost completed edits; initially retain five valid generations per project. Package 03 must settle storage limits and unsaved-recording ownership before implementation.
5. **Generation:** **Recommended:** built-in patterns by default; optional AI uses explicit preview/application and preserves the instrument.

Child plans use these recommendations unless revised. No new dependency or model is proposed.

## What the user does and sees

S/M/L describe relative scope, not elapsed time: local, several views, or shared behaviour/persistence.

| Order | Package | Findings | Scope | Dependency |
| --- | --- | --- | --- | --- |
| 01 | [Immediate safeguards #174](https://github.com/geertvandamme/dawit/issues/174) | 20, 33, 53 interim | M | None |
| 02 | [Adaptive workspace and visible selection #175](https://github.com/geertvandamme/dawit/issues/175) | 11–19 | L | Independent; follows 01 in release priority |
| 03 | [Resource preservation and crash recovery #176](https://github.com/geertvandamme/dawit/issues/176) | 52, 53 complete | L | 01 save guard |
| 04 | [Recoverable effect experiments #151](https://github.com/geertvandamme/dawit/issues/151) | 51 | L | Existing #151; independent of 03 |
| 05 | [Accessible interaction foundations #177](https://github.com/geertvandamme/dawit/issues/177) | 57–60 | L | Start during 01; integrate with 02 |
| 06 | [First sound and guided starting tasks #159](https://github.com/geertvandamme/dawit/issues/159) | 1–5, 43, 45 | M | 02; basic 05 patterns |
| 07 | [Recording preparation and silence help #178](https://github.com/geertvandamme/dawit/issues/178) | 29–32, 34 | M | 01 monitoring guard; 02 and 05 |
| 08 | [Find tools by intent #179](https://github.com/geertvandamme/dawit/issues/179) | 6–10, 27, 35, 44 | L | Shared commands; 05 |
| 09 | [Clear export and portable projects #180](https://github.com/geertvandamme/dawit/issues/180) | 54–56 | M | 03 for unresolved resources |
| 10 | [Predictable note and clip editing #181](https://github.com/geertvandamme/dawit/issues/181) | 21–26, 28 | L | 01 note menu; 02 and 05 |
| 11 | [Clear pattern and AI workflows #182](https://github.com/geertvandamme/dawit/issues/182) | 46–50 | L | 02 footer; 04 recovery conventions; 05 |
| 12 | [Understand effects and routing #183](https://github.com/geertvandamme/dawit/issues/183) | 36–42 | M | 02, 04 and 05 |

Finding 53 has an immediate safeguard and a complete fix. Accessibility is a completion condition from package 01, not work postponed until package 05. Independent slices can ship without waiting for unrelated packages.

### Package 01 Immediate safeguards

**Three small slices:** replace piano-roll right-click deletion with a context menu ending in the guide; show first-use monitoring advice before enabling Monitor; prevent overwrite of a source project loaded with omitted audio/plugins.

Until package 03 preserves unavailable objects, offer Save a Copy with the omissions listed. The source-path guard also applies to Save As and save-before-close/replace. A preserved missing SoundFont reference does not trigger it.

**Acceptance:** opening the note menu changes no notes; cancelling monitoring leaves it off; cancelling a reduced save preserves original bytes. Reproduce the missing-resource risk with scratch fixtures first. Each slice ships independently.

### Package 02 Adaptive workspace and visible selection

**A:** transport overflow, grouped editor tools, persistent target label and fixed Generate/Cancel/status footer. **B:** panel width budgeting, Focus Editor, Reset Layout and Fit Song/Selection. **C:** secondary windows fit the available display; optional workspace presets reduce repeated panel setup.

Keep Add Track and transport reachable. On narrow screens, an explicit Notebook overlay or temporary secondary-panel collapse preserves canvas space and a visible restore action. Temporary constraints never overwrite preferred widths. Reveal newly created tracks and explicit selection from another view; avoid automatic scrolling on every playhead update.

**Acceptance:** at all three sizes, exercise inspector, Notebook, editor/mixer and generator combinations. Every displaced action remains reachable; no zero-width hit targets; target names stay visible. Layout actions preserve song data. Check secondary windows on a display smaller than their former defaults.

### Package 03 Resource preservation and crash recovery

**A:** retain missing audio clips and unavailable plugin slots with identity, original state, placement and chain order. Named placeholders offer Relink/Rescan. Missing instruments produce silence; missing effects are explicitly bypassed. **B:** local recovery generations for saved and untitled projects preserve referenced unsaved audio; startup offers Recover, Keep for Later or Discard.

**Acceptance:** remove resource availability in a scratch project, save/reload, restore availability and recover the same arrangement and state. Test failed relinks, interrupted/disk-full writes, corrupt-newest recovery fallback and forced termination after an edit. The ordinary saved project remains unchanged. Retention never deletes audio referenced by another candidate.

**Design gate:** settle placeholder rendering, format compatibility, crash detection, recording ownership and retention in a dedicated plan. Capture a coherent edit on the message thread; write immutable copies atomically on a worker. Remove package 01's overwrite guard only after preservation passes.

### Package 04 Recoverable effect experiments

Reuse #151 and existing preset/state handling. **A:** Undo Load, named initial settings and a changed indicator. **B:** built-in parameter gesture Undo/Redo and A/B; one drag or preset application is one step. Label the affected processor/track and define shortcut focus rules.

Start with explicit processor history, preserving global structural Undo and the accepted mute/solo exclusions. Design project-persistent A/B before slice B ships. Chain presets are separate #151 scope; hosted-plugin parity follows a compatibility plan. Never poll arbitrary plugin state in the audio callback.

**Acceptance:** restore exact state after preset loading and multi-control edits; independently edit/switch A/B; verify window close and project reload. Cover EQ, compressor and the shared frame, playback continuity and text-box Undo routing.

### Package 05 Accessible interaction foundations

**A:** shared focus traversal, visible focus, names/roles/values and keyboard context actions, starting with toolbars, guide topics and musical selection. **B:** interface/text scaling, readable secondary text, higher contrast and non-colour state cues. Apply these patterns in every subsequent package.

**Acceptance:** keyboard-only first-note, note-edit, guide-open, recording-setup and export tasks, with musical typing both on and off. VoiceOver can inspect a note, clip, guide topic, value and selected track. Verify default/enlarged text at supported sizes and recognisable clipping/active sends without hue. Update UI rules that currently make focus avoidance and text shrinking unconditional; record platform gaps explicitly.

### Package 06 First sound and guided starting tasks

Replace passive empty-state text with Start from a Style, Add an Instrument or Drums, Record Voice or Instrument, Import Audio and Learn. Reuse existing commands; explain MIDI at the choice and select the next useful control. Show task guidance before Master in the initial inspector.

Offer an optional exercise: hear a note, change a chord, add rhythm and save. Reuse the guide library, Continue and progress; do not require the full reading path. Simplify unexpanded topic cards while retaining direct search matches.

**Acceptance:** templates work from the workspace and File; existing-song replacement still asks to save; guide progress survives. Observe beginners attempting first sound in about two minutes without coaching. This is a target to measure, not a promised result.

### Package 07 Recording preparation and silence help

Provide target track, input/device, live meter, monitoring and visible count-in in one short preparation panel. Reuse recording services and audio settings; missing-input states include an enabled Open Audio Settings action. Explain current latency conditionally using existing guidance, without inventing a universal threshold.

Add Why Can't I Hear This? using actual device, instrument, mute/solo, levels and clip/input state, including `routing::getSilence`. Distinguish verified causes from suggestions; open relevant controls without silently changing the mix.

**Acceptance:** record/play/re-record a scratch take; diagnose no input, unarmed track, another soloed track, missing instrument and muted clip. Manually test permission denial and device disconnection. Observe a basic take within about five minutes as an initial study target.

### Package 08 Find tools by intent

**A:** selected-audio Actions shared by arrangement/waveform editor, purpose-led effect descriptions and labelled Learn entries. **B:** Find Action with synonyms, shortcuts, requirements and keyboard activation through existing commands/`TrackActions`. **C:** menu regrouping and guide-search changes after task evidence.

Include karaoke, find tempo, record guitar, shared reverb, export and bounce. Disabled results explain prerequisites and offer a route forward. Guide search already removes question words and has keywords/prefix matching: measure failed queries before adding partial matches.

**Acceptance:** each goal reaches the same action through search and object UI; enablement follows selection; text focus retains shortcuts. Preserve working guide questions, topic links and reading progress. Keep familiar menu routes until replacements are usable.

### Package 09 Clear export and portable projects

Lead with Export Audio; retain bounce in search. Explain Whole Song, Selected Tracks, Stems and Individual Tracks, with names, range, duration and included processing. Flag definite empty/silent conditions and unresolved resources before rendering; offer measurement for final peak information rather than inferring it from live meters. Retain clipping feedback; add Play, Reveal and Export Again.

Explain `.dawit` plus `Audio Files`; Collect for Sharing lists dependencies and copies only appropriate project content.

**Acceptance:** render scope matches the summary for full mix, selection, loop and stems. Cancel/failure preserves the song. Collected audio reopens elsewhere; missing plugins/SoundFonts are reported without claiming they were bundled. Package 03 placeholders survive sharing/export. Wording can ship before full collection support.

### Package 10 Predictable note and clip editing

**A:** Cut/Copy/Paste/Duplicate with explicit target/position and one Undo step. Preserve applicable controller events, clip gain, duration and mute data; respect time signature, tempo map and audio seconds. **B:** explicit Draw/Select, numeric selection inspector, clear transpose scope, separate display filtering from input pitch constraints. **C:** visible take chooser and Score Preview with Edit in Piano Roll.

**Acceptance:** cross-clip note and compatible cross-track clip paste; cut/paste/undo restores the original music and instrument. Check text clipboard routing, lasso/draw, selected/all transpose and take switching. Old projects retain their combined Scale behaviour until explicitly changed. Reject incompatible targets helpfully.

### Package 11 Clear pattern and AI workflows

Offer Built-in Pattern, AI Part and ChatMusician modes. Patterns need no Ollama. Distinguish server unavailable, no model and incompatible/unverified model, with Retry and copyable setup instructions. A model name alone cannot prove compatibility; installation remains explicit.

AI stages Preview, Apply and Cancel with target, replace/add scope and proposed chord changes visible. Reject stale results after target edits/deletion. Preserve instruments; use package 02's footer. Lyrics uses Use This Line and Apply Lyrics to Song with an uncommitted indicator.

**Acceptance:** offline patterns; deterministic setup/cancellation/stale-result fixtures; Apply is reversible and Cancel changes no music. Test ChatMusician prerequisites and unsupported output against shipped behaviour. Reconcile [the existing plan](chatmusician-generation.md) before expanding scope; supplement fixtures with live smoke tests.

### Package 12 Understand effects and routing

Put a short Try This entry above dense explanations; optionally focus controls for the starting point. Persistently distinguish Editing This Track from Practice Example. Reuse package 04's preset state display.

Preserve sender/return context with a breadcrumb and send-level access after creating shared reverb. Keep unselected signal-flow labels readable, make Focus Selected explicit and provide Open Mixer. The map remains read-only for routing.

**Acceptance:** practice examples cannot accidentally edit song processors; guides identify their actual target. Keys → Reverb exposes sender amount and return effect. Map labels remain readable when another track is selected, with keyboard and enlarged-text checks.

## Data and saving

Layout, text/contrast and onboarding preferences belong in app settings; ordinary task flows use existing project data. Package 03 defines placeholder format compatibility and recovery ownership; 04 defines A/B persistence; 10 migrates combined Scale behaviour. Child plans specify defaults, old-reader behaviour and save/load failure tests before implementation. Recovery is separate from the saved project and retains its required audio references.

## The guide

Reuse MIDI, recording, routing, reading, effect and handover guides. No replacement library. Child plans changing explanations supply final text, picture examples, advice conditions and `SOURCES.md` updates in the same change.

## Code

Build on `ProjectManager`/`ProjectFile`, `Engine`/`UndoHistory`, `Selection`, `MainComponent`, `TrackActions`, `GeneratePanel`, `fxui` and `gen::guides`. Follow Commands, Windows, Saved Song Data and Long Jobs recipes. Preserve engine/UI separation and message-thread edit ownership; state capture, serialisation and restoration must not block the audio callback. Workers consume immutable data and accepted results apply on the message thread.

## Libraries and models

None. Use JUCE and existing services. Any child plan introducing a dependency must check exact-version/transitive licences and plan root `LICENSE`, `licenses/`, README and packaging obligations before adoption. AI setup work does not authorise installation or a default-model change.

## Theory

None added by this umbrella plan. Reuse sourced explanations; child plans must verify any changed factual claims. In particular, avoid unverified latency thresholds or musical-quality guarantees.

## Steps and implementation groups

For **each slice**, write three to six bounded steps per applicable group:

- **Group A, behaviour and engine:** reproduce the finding, define compatibility and implement behaviour with meaningful headless tests in `tests/EngineTests.cpp`. Run engine checks; omit for purely visual work.
- **Group B, UI:** share commands/state, add tooltips and keyboard/semantic access, then permanently extend `ScreenshotRunner` if needed. End with `./build.sh --test` and `All checks passed`; review warnings under project policy.
- **Group C, check and docs:** inspect real-app captures and perform interaction checks screenshots cannot establish. Update README, `docs/tests.md`, `docs/code-layout.md` and changed architecture/recipes/UI guidance. Record resolved findings, evidence and remaining scope in the implementation issue and this plan.

## Tests and release gates

1. **First release:** 01 plus essential toolbar/selection slices of 02. No destructive popup gesture, monitoring-before-consent or reduced overwrite; essential controls reachable at supported sizes. Do not wait for onboarding redesign.
2. **Reliability releases:** 03 and 04 independently. Resource round trips, fault injection and processor recovery pass, with accessible new UI.
3. **Beginner workflow releases:** 06–09 using 05 patterns. Observe first sound, recording, tool search and whole-song export without coaching; fix blocking failures before more navigation changes.
4. **Editing and advanced releases:** 10–12 independently. Compare wrong turns and task completion against the audit baseline; validate hypotheses before replacing working behaviour.

Carry 05 checks through every release. Reuse [audit captures and shot lists](../docs/usability-review-2026-11-10/); add failure and interaction fixtures. Verify supported sizes at default/enlarged text. Keyboard, VoiceOver and crash recovery require direct checks. Run Windows/Linux builds for affected shared code and platform checks for file handling, modifiers and accessibility; state any unverified behaviour.

## Website hand off

After implementation and commit, offer a separate daw-website hand-off for the relevant feature, learning and audience pages. Describe shipped behaviour, following website maintenance rules; keep website edits in their own commit. No website changes belong to this planning task.

## Backlog and left for later

The umbrella is #173. Packages 04 and 06 reuse #151 and #159; the first release also reuses #172. Their original issue bodies and wider scope are preserved. Broader recording (#33), MIDI expression (#163), routing (#68), role-based views (#100) and guide questions (#47) are related work rather than duplicate implementation tasks. Before later child plans, search open/closed GitHub issues and read existing decisions. Do not use the legacy HTML backlog.

The [first-release milestone](https://github.com/geertvandamme/dawit/milestone/1) groups these tasks under packages 01 and 02:

- [#184 — Note context menu](https://github.com/geertvandamme/dawit/issues/184).
- [#185 — Monitoring advice before enabling input](https://github.com/geertvandamme/dawit/issues/185).
- [#186 — Protect a project loaded with omitted resources](https://github.com/geertvandamme/dawit/issues/186).
- [#187 — Reachable transport and track actions](https://github.com/geertvandamme/dawit/issues/187).
- [#188 — Reachable piano-roll tools and target](https://github.com/geertvandamme/dawit/issues/188).
- [#189 — Fixed generation action and status footer](https://github.com/geertvandamme/dawit/issues/189).
- [#190 — Useful canvas with secondary panels](https://github.com/geertvandamme/dawit/issues/190).
- [#191 — Reveal a created or explicitly selected track](https://github.com/geertvandamme/dawit/issues/191).
- [#172 — Inspector toggle](https://github.com/geertvandamme/dawit/issues/172), reused.

No release version/date is assigned. The plan and audit remain local and uncommitted; issue descriptions identify that explicitly rather than linking to nonexistent GitHub files.

Verified on 10 November 2026: the umbrella, 12 package issues and nine first-release tasks are on the [DawIt project](https://github.com/users/geertvandamme/projects/2), with 21 native parent links and the `usability` label. The project has a Priority field with P1/P2/P3 options; all first-release tasks are P1. New issues are Todo; reused #151/#159 remain Todo and #172 remains Next. There are 19 new issues and three reused issues. No implementation is claimed.

Defer wholesale navigation redesign, new DSP/models, automatic installations, arbitrary hosted-plugin Undo guarantees, chain presets and notation editing. Unconfirmed usability hypotheses need participant checks before larger redesigns.

## Changes while building

For each package record its child plan/issue, completed slices, resolved finding IDs, build/interaction evidence and next dependency. Keep the original audit as the baseline.
