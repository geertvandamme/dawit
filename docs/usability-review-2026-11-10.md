# DawIt usability review and improvement proposals

DawIt makes a coherent promise: learn music by making and hearing it. Its guides, live diagrams, practice views and explanatory effects support that promise particularly well. The main workspace asks for considerably more DAW knowledge than the learning material does. A beginner must identify the right object, discover an interaction, and often navigate several panels before reaching the helpful explanation.

My assessment is that DawIt is **strong at explaining a tool once someone has found it, and substantially weaker at helping someone find the right tool and take their first steps**. For someone who already understands tracks, clips, MIDI and buses, the structure is reasonably familiar. For a complete beginner, the starting screen and the number of equally prominent controls create avoidable uncertainty.

This is an expert usability inspection, not a measured study with participants. The priorities below reflect likely user impact; they are not measured failure rates or a usability score.

## Scope and evidence

Reviewed on 10 November 2026 using the existing macOS Release executable and source checkout `a719f98`. The working tree was clean at the start. Captured and visually inspected 37 real-app images, including the empty project, example song, 1440 × 900 workspace and supported 960 × 600 minimum size. The screenshot runner uses scratch settings and synthetic or example content. The app code and the user's settings and projects were not changed.

Coverage includes arrangement, track selection, piano roll, controller lanes, score, audio editor, mixer, sends, signal flow, Notebook, theory library, chord guide, effect guide, style templates, recording setup, audio settings, export dialogue, generation panels, lyrics editing, play along, sing along, tuner, drum conversion, EQ, compressor, reverb, subtractive synth and drum instrument. Menu contents and editing behaviours were also inspected in source.

Screenshots establish visible state and layout. Source establishes the specified interaction paths. Direct native UI automation timed out, so pointer and keyboard behaviours described from source are labelled accordingly. Recording hardware, perceived sound quality, full AI requests and downloads, save/reload recovery scenarios, screen-reader output, and Windows/Linux behaviour were not exercised end to end. Some scenes use deliberately prepared data: the lyrics AI alternatives and tuner reading are demonstration fixtures, not measured model quality or microphone results. Sequential shots preserve panel state, so an open Generate panel in a later image is not evidence that it opens by default.

Evidence labels used below:

- **V**: visually observed in a real-app capture.
- **S**: confirmed in the current source.
- **H**: a usability hypothesis or design opportunity to validate with people.

Priority definitions:

- **P1**: address first because it blocks an important path, hides essential controls, surprises users with destructive behaviour, or threatens confidence in preserving work.
- **P2**: meaningful friction in discovery, understanding or repeated work.
- **P3**: refinement or an opportunity to improve an already usable screen.

## What already works well

1. **The guides are integrated with the music.** “Your chord track” explains the actual progression and shows playable piano or guitar diagrams. This is much more useful than an isolated manual. [Chord guide image](usability-review-2026-11-10/07-chords-guide.png).
2. **The guide library already solves much of the content-navigation problem.** It has categories, full-text word search, topic links, a beginner path, Continue, reading progress and PDF export. The task is to expose it more effectively, not build another library. [Guide library image](usability-review-2026-11-10/06-guides.png).
3. **Effect and instrument interfaces explain cause and effect.** Reverb connects decay to a room picture; compression connects threshold and ratio to a curve; synthesis connects controls to harmonics and envelopes. [Reverb](usability-review-2026-11-10/15-reverb.png), [compressor](usability-review-2026-11-10/33-compressor.png), [synth](usability-review-2026-11-10/34-synth.png).
4. **Purpose-first wording is often excellent.** “Hear what it removes”, “Hear only the reverb”, “Play a note” and “Find a bad frequency” encourage listening and experimentation.
5. **The style builder offers a concrete musical starting point.** A genre list, arrangement picture, key, tempo and optional notes make a whole-song sketch approachable. [Style image](usability-review-2026-11-10/09-style.png).
6. **Practice views have a clear visual hierarchy.** Play along shows the current chord, next chord, fingerings and countdown. Sing along provides large words and preparation time. [Play along](usability-review-2026-11-10/18-play-along.png), [sing along](usability-review-2026-11-10/37-sing-along.png).
7. **The tuner gives an actionable instruction.** “12 cents flat, tighten it a little” translates a measurement into a useful next step. It also provides note buttons and an input-level display. [Tuner image](usability-review-2026-11-10/36-tuner.png).
8. **The quantise dialogue explains scope and a no-op.** It explicitly says all 12 notes are affected when none are selected, previews the grid, and disables Quantise with “Nothing to move”. This is a good model for other tools. [Quantise image](usability-review-2026-11-10/29-quantise.png).
9. **Several recovery and safety basics exist.** Unsaved-change prompts, named Undo/Redo, cancellation of long jobs, recording input meters, monitoring advice, missing-resource warnings and export clipping explanations are present. The proposals below build on them.
10. **The sound path is represented consistently.** Track colours, instrument/effect chips, inspector, mixer and signal-flow map give users several ways to understand the same song.

## How easy the main tasks are to discover

These are inspection judgements about the routes, not observed participant success rates.

| User goal | Existing route | Assessment | Main improvement |
| --- | --- | --- | --- |
| Start making a first song | Add Audio/MIDI, or File → New Project from Style | Hard for someone unfamiliar with track types | Put task choices and templates in the empty workspace |
| Hear a first note | Add MIDI track, use its clip and sound, draw or enable computer keyboard | Several concepts before the reward | Offer a guided first-note action |
| Learn what a chord means | Chords guide icon, chord menus, or Theory Guides | Strong once located | Label the learning entry point and expose it on first launch |
| Edit notes | Select a MIDI clip and use the piano roll | Familiar to DAW users; drawing and selection need teaching | Make tool mode explicit and preserve controls at narrow widths |
| Record a voice | Add audio track, arm, choose input, optionally monitor, record | Fragmented setup | A short recording preparation flow with a live meter |
| Fix a silent track | Inspect sound, mute/solo, routing, levels, device | High diagnostic burden | Contextual “Why can't I hear this?” help |
| Add reverb to one track | Add Effect in the device bar or inspector | Fairly visible | Explain insert effect versus shared reverb at the choice |
| Share reverb between tracks | Sends menu or return track, mixer send controls | Requires routing vocabulary | Provide a named “Shared reverb” route and retain sender context |
| Find tempo or chords in audio | Audio clip right-click menu | Powerful but easy to miss | Selected-clip action area and searchable commands |
| Practise chords or sing lyrics | Buttons on song rows or View menu | Relatively clear once song data exists | Explain prerequisites and offer setup actions when empty |
| Generate a part | Generate in piano roll, then part/style/playing/model settings | High setup and choice burden | Separate built-in patterns from optional AI |
| Export a listenable song | File → Bounce to WAV | Clear to experienced users; terminology barrier for beginners | “Export audio” wording with an explicit whole-song summary |
| Return to a clean workspace | Close panels and choose views individually | No obvious single recovery route | Reset layout and task-based workspace presets |

## First launch and finding tools

**1. P1 · The empty project does not offer a complete first task. V/H.** It tells users to add an Audio or MIDI track or drop audio, while the rest of the screen shows an empty editor, master controls and song rows. It does not visibly offer “Make a song from a style”, “Record my voice” or “Learn the basics”. Add those actions directly in the empty workspace, including a small playable example. Success means a beginner can get a first sound without opening a menu or reading a guide. [Empty project](usability-review-2026-11-10/20-empty.png).

**2. P2 · Track creation starts with technical categories. V/H.** Audio, MIDI, Bus and Return appear together in the top bar. Beginners usually think “record a guitar”, “play a piano” or “add drums”. Keep the technical types, but offer a task-based Add Track picker with short explanations and sensible defaults. The picker should create the correct track and select its next useful control.

**3. P2 · Empty-state instructions are text rather than actionable controls. V.** The piano roll asks for a MIDI clip; the arrangement asks for a track. Replace passive guidance with “Add an instrument track”, “Import audio” and, where appropriate, “Create a clip here”. Disabled actions should explain what is missing. [Empty project](usability-review-2026-11-10/20-empty.png).

**4. P2 · The master is the first prominent inspector object. V/H.** In an empty project the inspector shows Master, “THE MIX BUS” and a volume fader. That is a difficult introduction before there are parts to mix. Show a welcome or contextual inspector state until a user selects something, with a clear way to inspect Master later.

**5. P1 · The most approachable creation tool is hidden behind File. V/S/H.** New Project from Style already produces a concrete arrangement and sounds, but it is absent from the empty-state choices. Promote it to “Start from a style” on the opening workspace. Retain the File command and shortcut. [Style builder](usability-review-2026-11-10/09-style.png), [menu source](../src/ui/MainComponent.cpp#L755).

**6. P2 · There is no visible search for app actions. S/H.** Guide search is good, but the main command/menu structure has no equivalent way to find “split”, “karaoke”, “reverb” or “export”. Add a Find Action command with synonyms, shortcuts, prerequisites and the selected target. Search results should execute the action or show the exact place it belongs. [Command definitions](../src/ui/MainComponent.cpp#L519).

**7. P2 · The menu categories mix tasks and configuration. S/H.** File includes loudness measurement, plugin management, audio/MIDI settings and several kinds of PDF export; View includes practice tools and the tuner. Add coherent task groupings or a small Tools menu, while keeping useful existing routes as aliases. A beginner should be able to predict where recording setup, analysis and export belong. [Menu source](../src/ui/MainComponent.cpp#L755).

**8. P1 · Major audio tools depend on discovering the clip context menu. S/H.** Stem separation, karaoke, audio-to-MIDI, hits-to-MIDI, tempo/chord/structure detection and lyric detection are clustered in one audio-clip menu. Provide a visible Actions or Analyse area for the selected audio clip, with short outcome descriptions. Keep the context menu as a fast route. [Clip menu source](../src/ui/TrackLanes.cpp#L844).

**9. P2 · Some tooltips carry the main instructions. V/S/H.** Tooltips explain important modifier keys, keyboard modes and recording behaviour, sometimes in long paragraphs. Put the essential action and current mode on screen; use tooltips for the shortcut and one useful detail. An unfamiliar user should not have to hover over every control to learn a workflow. [Transport tooltips](../src/ui/TransportBar.h), [piano-roll tooltips](../src/ui/PianoRoll.cpp#L773).

**10. P2 · The guide symbol is hard to identify at small sizes. V/H.** The distinctive book/spectrum icon is recognisable after learning it, but tiny standalone versions compete with audio and waveform imagery. Use “Learn” or “Guide” labels at key entry points and teach the symbol once. Preserve the compact symbol where repeated context makes its meaning clear. [Arrangement](usability-review-2026-11-10/01-arrangement.png).

## Workspace layout and orientation

**11. P1 · Essential top-bar controls disappear at the supported minimum size. V/S.** At 960 × 600, Add Audio Track, Add MIDI Track, Bus, Return and Mixer are no longer visible in the top bar. The sizing code can shrink the button group to zero available width. Introduce an overflow menu or a second row, prioritising transport and Add Track; ensure every action remains reachable through a visible control. [Minimum-size image](usability-review-2026-11-10/23-small.png), [layout source](../src/ui/TransportBar.cpp#L287).

**12. P1 · The piano-roll toolbar loses controls as width decreases. V/S.** The minimum-size image loses Scale, Velocity, Select, Transform and octave controls; even the wider workspace gives the track title and additional help controls little room. Replace fixed-width packing with grouped tools, overflow and a persistent target label. Verify with inspector, Notebook and Generate open in combinations. [Minimum-size image](usability-review-2026-11-10/23-small.png), [layout source](../src/ui/PianoRoll.cpp#L1121).

**13. P1 · The Notebook can leave almost no usable timeline. V/S.** At 960 × 600, the inspector, track headers and Notebook consume most of the width. The remaining arrangement shows roughly two bars at the captured zoom. Collapse a secondary panel automatically, offer an overlay Notebook, or enforce a useful minimum canvas width. [Minimum-size Notebook](usability-review-2026-11-10/25-small-notebook.png), [panel layout](../src/ui/MainComponent.cpp#L350).

**14. P2 · The Generate action can sit below the visible panel. V/S.** In the normal generator at 1440 × 900, the first view shows configuration while the main Generate button and lower status require scrolling. Keep Generate/Cancel and its current status in a persistent footer, and put advanced settings in a collapsible section. [Generator image](usability-review-2026-11-10/05-generate.png), [form layout](../src/ui/GeneratePanel.cpp#L290).

**15. P1 · The selected track can be outside the visible arrangement. V/S.** Bass, Keys or Voice can be selected in the inspector and device bar while the arrangement still displays only Drums, Kick, Snare and Hats. Automatically reveal a track after explicit selection from another view or track creation, and provide “Show selected track”. Avoid forcing scroll changes during playback unless requested. [Keys selected off screen](usability-review-2026-11-10/03-piano.png), [selection callback](../src/ui/MainComponent.cpp#L447).

**16. P2 · Much of the opening example is outside the current view. V/H.** At the captured normal size, only about nine bars and four track rows are visible in a 36-bar example. An overview strip and Fit Song/Fit Selection controls would make the overall structure easier to grasp. Offer a sensible initial fit for an example, rather than making people infer the song's extent from scrollbars. [Arrangement](usability-review-2026-11-10/01-arrangement.png).

**17. P2 · The default layout spends substantial space on overlapping controls. V/H.** The inspector repeats selected-track controls also available in the track header, mixer and device bar, while a large empty piano roll can remain open. Offer Learn, Compose, Record and Mix workspace presets, with a quick way to customise them. Hide an unused bottom editor on a fresh empty project or give it an actual starting task.

**18. P2 · There is no obvious single layout recovery action. S/H.** Panels can be closed, resized and reopened individually, and some visibility persists between sessions. Add Reset Layout plus a temporary “Focus on editor” command. A user who closes several panels should be able to return to a known state without remembering individual shortcuts. [Panel handling](../src/ui/MainComponent.cpp#L375).

**19. P2 · Some secondary windows start larger than a small laptop workspace. V/S/H.** The subtractive and drum editors are 1400 × 780, while the guide hub is 1040 × 760. These are default content sizes, so fit to an actual display still needs live testing. Make editors adapt to the available work area through responsive grouping or scrolling, and keep close/back controls visible. [Synth](usability-review-2026-11-10/34-synth.png), [drums](usability-review-2026-11-10/35-drums.png).

## Editing and interaction consistency

**20. P1 · Right-clicking a piano-roll note deletes it immediately. S.** The note-grid handler removes the clicked note on a popup-menu gesture, whereas clips and many other objects open menus. A user looking for help or note properties can therefore delete a note. Open a note context menu with Delete, Duplicate, Quantise, Properties and the relevant guide; if fast erase is retained, make it an explicit tool or modifier. Verify Undo restores an accidental deletion. [Handler](../src/ui/PianoRoll.cpp#L301).

**21. P1 · Standard clipboard editing is incomplete in the inspected handlers. S.** Clip handling supports Cmd+D and copy-drag; the note grid supports selection, deletion and stepping, but these handlers provide no Cmd+C/Cmd+V workflow. The Edit menu contains only Undo/Redo. Add Cut/Copy/Paste and Duplicate for selected notes and clips, with clear paste position and cross-track behaviour. Verify focused text editors retain their own clipboard commands. [Clip keys](../src/ui/TrackLanes.cpp#L787), [note keys](../src/ui/PianoRoll.cpp#L557), [Edit menu](../src/ui/MainComponent.cpp#L755).

**22. P2 · Drawing and selecting use different rules on the same empty grid. S/H.** A plain press on empty piano-roll space creates a note; selection requires Select mode or modifiers. This is efficient after learning it, but accidental notes are likely during exploration. Show explicit Draw and Select tools, a clear active cursor, and a short first-use hint. Keep fast modifier gestures for experienced users. [Grid interaction](../src/ui/PianoRoll.cpp#L301).

**23. P2 · Note manipulation lacks a visible precision-editing surface. V/S/H.** Dragging and keyboard nudging are available, but the captured editor offers no obvious selected-note fields for pitch, start, duration and velocity. Add a compact selection inspector with musical units and multi-selection behaviour. It should make “start this note on beat 3 and hold it for two beats” possible without pixel-precise dragging.

**24. P2 · Transpose scope is easy to misunderstand. V/S/H.** The toolbar's Octave buttons transpose the whole clip, while arrow keys act on selected notes. Label this scope explicitly or make the buttons follow the selection and say when they affect the whole clip. Show a preview or named Undo step. [Toolbar setup](../src/ui/PianoRoll.cpp#L850), [note keyboard handling](../src/ui/PianoRoll.cpp#L557).

**25. P2 · Scale combines a visual filter with input restriction. S/H.** The same feature hides notes outside the song key and restricts incoming MIDI notes. A user seeking a clearer display may not expect a keyboard-performance change. Separate “Show notes in key” from “Constrain played notes”, or expose both consequences in a clear popover before enabling it. [Scale tooltip](../src/ui/PianoRoll.cpp#L1324).

**26. P2 · Score looks like an editing mode but is read-only. V/S/H.** Piano roll and Score appear as neighbouring switches; the read-only explanation is in a tooltip. Put “Score preview” or a short read-only label in the score header, and make “Edit in piano roll” visible on note selection. Preserve the existing double-click route. [Score](usability-review-2026-11-10/04-score.png), [mode tooltips](../src/ui/PianoRoll.cpp#L805).

**27. P2 · Important audio editing actions live outside the waveform editor. V/S/H.** The waveform view has Reverse and Gain, while Split, Join, conversion and detection live on the arrangement clip menu. Add a contextual audio toolbar or Actions menu in the editor, with the current clip named. A user already looking closely at a waveform should not have to return to its arrangement rectangle to discover what can be done. [Audio editor](usability-review-2026-11-10/27-audio-editor.png), [clip menu](../src/ui/TrackLanes.cpp#L844).

**28. P2 · Recorded takes are mainly managed through a context submenu. S/H.** Overlapping takes have a Takes submenu and “Delete other takes”. Make the active take and total take count visible on the clip, with a small chooser and audition controls. Preserve muted alternatives and make any permanent removal explicit. [Takes menu](../src/ui/TrackLanes.cpp#L873).

## Recording and understanding silence

**29. P1 · Recording preparation is spread across several places. S/H.** Users must connect the audio settings window, track arm button, input chip, monitor, count-in and transport Record. Add a “Record voice or instrument” preparation view with device, input, live level, monitoring advice and count-in, then create or select the target track. Keep this flow short enough to reach recording without a theory lesson. [Track controls](../src/ui/TrackHeader.cpp#L28), [audio settings](usability-review-2026-11-10/11-settings.png).

**30. P2 · The no-input message directs users elsewhere without taking them there. S.** The input menu can show a disabled “No audio input found: choose one in Audio / MIDI Settings” item. Replace it or supplement it with an enabled Open Audio Settings action. Retain the selected track/input context when the settings window closes. [Input menu](../src/ui/RecordingActions.cpp#L50).

**31. P1 · The latency display needs a practical interpretation. V/H.** The captured setup reports a 104.3 ms round trip, but the visible line does not say what that means for playing or singing. Add conditional advice such as “You may hear an echo when monitoring through DawIt”, with buffer-size and direct-monitoring guidance appropriate to the device. Do not infer the same latency on other machines. [Audio settings](usability-review-2026-11-10/11-settings.png).

**32. P2 · Count-in options are hidden behind a metronome right-click. S/H.** A count-in is especially valuable to a beginner preparing a first take. Give the metronome a visible menu affordance or put Count-in next to Record in the recording preparation view. Show its enabled state before recording. [Metronome tooltip](../src/ui/TransportBar.h).

**33. P1 · First-use monitoring advice appears after monitoring is enabled. S.** `setMonitor` changes the engine state before displaying its first-use headphones/feedback message. Present that choice before turning monitoring on, or gate the first enablement behind the advice. The wording should explain monitoring versus the interface's direct monitoring, then provide a clear Enable button. [Monitoring handler](../src/ui/RecordingActions.cpp#L102).

**34. P1 · Silence has too many possible causes for a beginner to diagnose. V/S/H.** Instrument selection, track/clip mute, solo elsewhere, master level, routing, MIDI arming and device configuration can all affect what is heard. Add “Why can't I hear this?” to a selected track and the empty output state, using actual engine state to offer ordered checks. Link directly to the relevant control and avoid claiming a diagnosis when the app cannot determine it.

## Effects and mixing

**35. P2 · The effect chooser explains that tools teach, but not enough about which tool solves a problem. S/H.** Built-in menu descriptions commonly say “explains itself as you use it”. Add concise purposes such as “Reverb — put a sound in a space” and “Compressor — even out loud and quiet moments”, plus search and categories when the list grows. Keep exact plugin names available. [Plugin menu](../src/ui/PluginActions.cpp#L140).

**36. P2 · Large instruments expose many controls with similar prominence. V/H.** The subtractive synth combines oscillator, filter, envelope, LFO and playing controls in one dense view. Add an optional guided view that highlights the few controls useful for the selected starting point, while retaining the complete instrument. Help a beginner hear a useful change before requiring them to understand every section. [Synth](usability-review-2026-11-10/34-synth.png).

**37. P2 · Explanation panes are informative but visually dense. V/H.** Reverb, EQ and compressor place several paragraphs of small text alongside the controls. Lead with a short “Try this” instruction and a compact explanation; allow the rest to expand or use larger reading text. Keep the live diagrams and full guide. [EQ](usability-review-2026-11-10/32-eq.png), [compressor](usability-review-2026-11-10/33-compressor.png).

**38. P2 · “Starting point...” does not name the current default sound. V/H.** In the captured effect/instrument editors, the preset control initially shows a prompt while parameter values already define a sound. Give the initial state a meaningful name, and mark subsequent manual changes visibly. Make reverting to that sound straightforward. [Reverb](usability-review-2026-11-10/15-reverb.png), [drums](usability-review-2026-11-10/35-drums.png).

**39. P2 · Learning can open a real processor or a processor on no track. S/H.** The guide system already describes its target and looks for an instance in the song, preferring the selected track. Still, users should not have to rely on a title or tooltip to know whether experimenting changes their music. Persistently label “Editing Keys” versus “Practice example”, and offer an explicit choice when multiple relevant instances exist. [Target resolution](../src/ui/TheoryGuidesWindow.cpp#L140).

**40. P2 · Adding a shared effect switches the user's selection to the return. V/S/H.** After creating a Keys → Reverb send, the inspector and mixer move to Reverb. That is useful for editing the effect but obscures the sender whose amount may need adjustment. Show a persistent “Keys → Reverb” breadcrumb with a sender-level control and a way back to Keys. [Send result](usability-review-2026-11-10/31-sends.png), [selection wiring](../src/ui/MainComponent.cpp#L200).

**41. P2 · The signal-flow map makes unselected tracks too faint. V/S/H.** With Keys selected, the rest of the song is strongly dimmed; the tiny legend is difficult to read. Retain readable labels and distinguish the selected path with outlines or line emphasis. Offer Focus Selected as an explicit mode instead of making reduced legibility the normal selected state. [Signal flow](usability-review-2026-11-10/08-routing.png).

**42. P2 · The signal-flow map needs clearer action expectations. V/S/H.** It looks like a diagram of controllable effects and connections, but selection is its principal interaction and changes happen elsewhere. Put a short “Select a track here; edit it in the mixer” explanation near the toolbar, and provide an explicit Open Mixer action for the selection. Keep accidental routing changes impossible. [Signal flow](usability-review-2026-11-10/08-routing.png), [interaction specification](ui-design.md#signal-flow-window).

## Learning and generation

**43. P2 · The beginner path starts with a substantial reading commitment. V/H.** “0 of 76 pages read” is accurate, but can make getting started feel like a course requirement. Keep the full path while offering a five-minute first exercise: hear a note, change a chord, add a rhythm and save. Give the exercise its own clear completion state. [Guide library](usability-review-2026-11-10/06-guides.png).

**44. P2 · Guide search could be more forgiving of everyday questions. S/H.** Search already removes common question words, uses curated keywords and supports word-prefix matching. It still requires every remaining word to match a guide. Validate longer phrases and unfamiliar vocabulary, then add partial-match suggestions and “Try fewer words” recovery where needed. Test phrases such as “why is my voice muddy?” and “how do I record a guitar?” before changing the existing search, since they may already work well. [Search implementation](../src/generate/TheoryGuides.cpp#L562), [matching rules](../src/generate/TheoryGuides.cpp#L685).

**45. P3 · Guide cards expose many topic chips at once. V/H.** The cards are useful but create a dense page of similar small targets. Show a few likely topics, a topic count and an Expand action, while preserving direct topic matches in search results. Keep the current category structure and Continue action. [Guide library](usability-review-2026-11-10/06-guides.png).

**46. P2 · Built-in patterns and AI share a setup-heavy generator. V/S/H.** The generator presents model-related choices even though some playing styles work without a model. Offer clear modes: Built-in Pattern, AI Part and ChatMusician, each with its own prerequisites and a plain description. Change the Generate tooltip so it does not imply that every generation path requires AI. [Generate](usability-review-2026-11-10/05-generate.png), [availability logic](../src/ui/GeneratePanel.cpp#L470).

**47. P1 · AI setup recovery can require a terminal command. S/H.** When no models are installed, the panel tells users to run `ollama pull ...`. Provide an in-app setup explanation with server connection state, model detection, copyable instructions and a Retry action; keep built-in patterns available from the same empty state. Distinguish “server unavailable” from “server running, no model installed”. [Model refresh](../src/ui/GeneratePanel.cpp#L494).

**48. P1 · ChatMusician mode can display an ordinary general model. V/S/H.** The captured mode shows `qwen2.5:7b` while requesting a ChatMusician task. The README requires a compatible separately installed model or alias. Explain compatibility before generation and offer a tested setup route; do not imply that every model in the generic Ollama list is suitable. Verify unsupported choices produce helpful guidance rather than repeated invalid results. [ChatMusician](usability-review-2026-11-10/30-chat.png), [setup requirements](../README.md#optional-local-ai-setup).

**49. P2 · Generation paths have different commitment models. S/H.** ChatMusician has staged Preview/Apply actions; the normal generator can replace or add notes and offers takes and Undo. Standardise the visible promise as Generate → Preview → Apply where practical, with the target clip and replace/add behaviour always visible. Preserve deliberate fast workflows behind an explicit preference. [Generator controls](../src/ui/GeneratePanel.cpp#L78), [Generate image](usability-review-2026-11-10/05-generate.png).

**50. P2 · Lyrics editing has two levels of applying changes. V/H.** Use inserts a candidate into the dialogue text; Apply commits the dialogue to the song. Rename these “Use this line” and “Apply lyrics to song”, and show whether the text contains uncommitted changes. Preserve the useful syllable-count and rhyme feedback. [Lyrics editor](usability-review-2026-11-10/17-lyrics.png).

## Recovery and sharing

**51. P1 · Undo excludes settings inside an effect. S.** The global Undo command explicitly says effect settings, mute and solo are not part of undo. The effect-setting gap is particularly significant for learning: a user can lose a sound while experimenting with several knobs. Add parameter/preset changes to an appropriate undo history or provide reliable per-processor Undo and A/B snapshots. Make mute/solo exclusions explicit without obscuring the more consequential parameter behaviour. [Undo definition](../src/ui/MainComponent.cpp#L671).

**52. P1 · Automatic crash recovery is not evident in the inspected application source. S/H.** The project manager provides manual saves and save-before-replace prompts, but no autosave/recovery mechanism was found in the source search. Add periodic recovery snapshots, atomic replacement and a recover/discard choice after an unclean exit. Validate by terminating a scratch session and recovering recent notes, clips and settings; normal Save prompts are not sufficient for this scenario. [Project manager](../src/ui/ProjectManager.cpp).

**53. P1 · Missing audio and unavailable plugins are skipped rather than preserved as placeholders. S.** The loader warns, then omits those clips or processors. The loaded edit is marked saved; a later save serialises that reduced edit. This creates a source-indicated risk of losing the references and plugin state when saving over a project opened on an incomplete setup. Preserve unavailable objects, state and placement; provide Relink/Rescan and explicit warnings before a save that would discard them. The sampler already preserves its missing SoundFont reference, which is a useful precedent. [Plugin loading](../src/engine/ProjectFile.cpp#L339), [audio loading](../src/engine/ProjectFile.cpp#L372), [loaded baseline](../src/ui/ProjectManager.cpp#L174). This scenario needs an end-to-end scratch-project regression test.

**54. P2 · Project portability is not obvious from Save. S/H.** Saving collects audio into a sibling `Audio Files` folder, so sharing only the `.dawit` file can be misleading. Explain the project file plus `Audio Files` folder relationship after first save, and offer “Collect project for sharing” with a dependency summary. Include external plugins and sampled instruments in that summary without promising they can all be bundled. [Audio collection](../src/engine/ProjectFile.cpp#L527).

**55. P1 · “Bounce to WAV” is an avoidable export vocabulary barrier. V/S/H.** Keep the production term in explanatory text, but lead with “Export Audio...” and “Export WAV”. Separate Whole Song, Selected Tracks, Stems and Individual Tracks with one-sentence descriptions. Make the current scope prominent before the file chooser. [Export dialogue](usability-review-2026-11-10/10-bounce.png), [File command](../src/ui/MainComponent.cpp#L545).

**56. P2 · Export problems are explained after the file is rendered. S/H.** The completion message can report clipping or a silent file, which is helpful but late. Add a preflight summary of obvious empty/muted ranges and known level concerns, and offer measurement where required rather than guessing final render peaks. After export, offer reveal, play and export-again actions with the resulting file's name. [Render reporting](../src/ui/BounceDialog.cpp#L45).

## Accessibility and sustained use

**57. P1 · Custom painted interactive content needs an accessibility audit and semantic controls. S/H.** Guide cards/chips and several musical surfaces are mouse-driven painted components; no custom `AccessibilityHandler` implementation was found under `src/ui`. Standard JUCE widgets may already expose useful semantics, so this does not establish that the entire app is inaccessible. Test VoiceOver and add names, roles, values, selection state and actions for notes, clips, guide topics and routing objects. A blind user should be able to select and inspect a note and open a guide topic.

**58. P1 · Keyboard reachability conflicts with the policy of keeping focus away from buttons. S/H.** Many controls deliberately disable keyboard focus to preserve musical shortcuts. That helps performance but needs an alternative route for keyboard-only operation. Define a consistent focus traversal model, visible focus indication and keyboard invocation of context actions; verify it with computer-keyboard playing both on and off. Guide cards especially need a keyboard route beyond mouse clicks. [Guide cards](../src/ui/TheoryGuidesWindow.cpp#L333), [track controls](../src/ui/TrackHeader.cpp#L130).

**59. P2 · Small text and low-emphasis labels make sustained reading harder. V/S/H.** Mixer instructions, strip types, graph legends, track metadata and explanatory panels use small or dim text. Add a scalable interface/text preference and a higher-contrast option, with responsive layout rather than merely shrinking text to fit. Measure contrast on actual enabled, disabled, selected and dimmed states; the screen captures alone do not establish accessibility conformance. [Mixer](usability-review-2026-11-10/02-mixer.png), [signal flow](usability-review-2026-11-10/08-routing.png), [theme](../src/ui/Theme.h).

**60. P2 · Colour should reinforce musical state without carrying it alone. V/H.** Chord families, track identity, meter zones and some graph distinctions use colour prominently. Preserve existing labels, add patterns/shape or text where needed, and test greyscale and common colour-vision variations. In particular, ensure selected notes, bypassed processors, clipping and active sends remain distinguishable without hue. [Chord guide](usability-review-2026-11-10/07-chords-guide.png), [EQ](usability-review-2026-11-10/32-eq.png), [sends](usability-review-2026-11-10/31-sends.png).

## Recommended order of work

| Order | Package of work | Findings | Completion evidence |
| --- | --- | --- | --- |
| 1 | Preserve work and make exploration recoverable | 20, 51–53 | Right-click opens a menu; effect experiments undo; crash recovery and missing-resource round trips preserve data |
| 2 | Make the supported window sizes usable | 11–15, 19 | No unreachable controls at 960 × 600, 1280 × 800 and 1440 × 900; useful canvas with secondary panels |
| 3 | Give beginners a first successful task | 1–5, 29, 34, 43 | A new user creates sound, records a first take and understands the next action without coaching |
| 4 | Make tools findable | 6–10, 27–28, 35, 55 | Actions searchable by common language and available from the selected object's editor |
| 5 | Simplify optional generation | 14, 46–50 | Offline patterns work without AI; setup failures are actionable; generated changes can be previewed |
| 6 | Improve repeated editing and workspace clarity | 16–18, 21–26, 36–42, 54, 56 | Faster note/clip editing, clear scope and selection, readable routing, predictable sharing/export |
| Across every package | Accessibility and reading comfort | 44–45, 57–60 | Keyboard and VoiceOver acceptance checks, text scaling and contrast checks accompany the affected UI |

These packages are suggested planning boundaries, not implementation plans or time estimates. Accessibility checks belong in the first changed screens, rather than waiting for a final polish phase.

## Proposed navigation and wording

Keep the working arrangement central. Add a small set of visible task entries that map to existing capabilities: **Make music**, **Record**, **Practise**, **Learn**, **Export**. They can open focused panels or menus rather than separate application modes. Add **Find Action** as a universal route for people who do not know a tool's location.

| Current wording or presentation | Proposed wording or presentation | Reason |
| --- | --- | --- |
| + MIDI Track | Instrument or drums — MIDI | Starts with the user's intent while teaching the term |
| New Project from Style | Start from a style | Easier to scan in the empty workspace |
| Bounce to WAV | Export audio — WAV | Connects an unfamiliar term to the expected task |
| + Bus | Group tracks — bus | Explains its purpose before its type |
| + Return | Shared effect — return | Gives a reason to choose it |
| Scale | Notes in key, with separate input constraint | Makes the two effects of the current option explicit |
| Starting point... on an initialised processor | Named initial sound | Identifies what is currently heard |
| Use / Apply in lyrics | Use this line / Apply lyrics to song | Separates candidate insertion from song commitment |
| Model / Refresh alone | AI connection status and model / Retry | Explains what is being connected and repaired |
| Tiny guide icon at a primary entry point | Learn, with the guide icon | Teaches the icon's meaning |

Validate these terms in context. Experienced users should still find “MIDI”, “bus”, “return” and “bounce” through menus, search and explanations.

## Usability study to validate the proposals

Recruit a small formative group covering complete beginners, musicians who have not used a DAW, and experienced DAW users. Five or six people across these groups can reveal useful problems but cannot establish reliable population-wide scores. Include keyboard-only and screen-reader sessions separately, or recruit participants who normally use those methods.

Give participants goals without naming controls:

1. “Make a short piece that has chords and drums. Play it.”
2. “Change one note, copy a short phrase, and undo a change you dislike.”
3. “Record a spoken line. Listen back and make another take.”
4. “Your instrument is silent. Find the cause.” Use several controlled causes across sessions.
5. “Make two parts sound as though they are in the same room.”
6. “Find an explanation of why this chord feels tense, then try a different chord.”
7. “Use this recording to find its tempo or make a karaoke version.”
8. “Make a bass pattern without installing anything.” Then separately test optional AI setup.
9. “Export a file you can send to a friend.” Check that it contains the intended whole song.
10. “Save the project so another computer can reopen it.” Introduce a deliberately missing audio file or plugin in a scratch copy and ask for recovery.

Record independent completion, requests for help, wrong turns, accidental edits, the first place searched, recovery success and time to first sound. Ask “What do you expect this to do?” before ambiguous actions, and “How easy was that?” after each task. Separate finding the tool from using it: the guides may make use easy even when discovery is hard.

Suggested initial acceptance targets are: no essential control disappears at supported sizes; all destructive musical edits have a clear recovery route; first sound is possible in about two minutes; a basic take is possible in about five minutes; and whole-song export is found without coaching. These are proposed product targets, not results of this review. Set final targets after the first baseline sessions.

## Evidence files and reproduction

The [evidence folder](usability-review-2026-11-10/) contains all 37 inspected PNGs. `26-interactive.png` is an additional empty-workspace capture; direct native automation did not complete. The four shot lists reproduce the scripted states: [normal workspace](usability-review-2026-11-10/pop.json), [empty project](usability-review-2026-11-10/empty.json), [minimum size](usability-review-2026-11-10/small.json) and [additional tools](usability-review-2026-11-10/more.json).

Run one list at a time from the repository root with the existing build:

```bash
perl -e 'alarm 120; exec @ARGV' build/Release/DawIt_artefacts/Release/DawIt.app/Contents/MacOS/DawIt --screenshots docs/usability-review-2026-11-10/pop.json
```

The initial normal-workspace run captured its first 18 shots and then failed a final ChatMusician fixture selection. The corrected normal-workspace list omits that final shot and was rerun successfully; ChatMusician was successfully captured as `30-chat.png` in the additional-tools run. All four saved shot lists completed successfully. This runner setup failure is not classified as a product usability defect. Report links, finding numbering and the presence of all referenced capture files were checked.

This review adds documentation and evidence only. It does not implement the proposals, create backlog issues, change the website or commit changes.
