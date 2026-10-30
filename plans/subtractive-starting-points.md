# More useful starting points for every built-in plugin

Status: In progress
Idea: [#118 — More useful starting points for every built-in plugin](https://github.com/geertvandamme/dawit/issues/118)

## Goal

Expand DawIt Subtractive's starting points so a beginner can find distinct sounds and learn how to change them. This plan covers Subtractive alone; #118 remains open for the other plugins.

## Decisions for you

1. **Scope and selection:** Add six starting points (Soft keys, Hollow organ, Bright stab, Tremolo keys, Airy sweep and Resonant reed), keeping all 11 existing sounds unchanged. This adds keyboard, sustained, percussive, amplitude-modulated, noisy and band-pass examples. **Recommended:** accept these six and tune them by ear during implementation. Answer: approved.
2. **Existing DawIt Synth sounds:** #112 separately asks for those sounds as Subtractive presets. **Recommended:** leave that conversion for #112; these six use Subtractive's own controls and avoid claiming exact copies. Answer: approved.

## What the user does and sees

Open DawIt Subtractive and choose a name from its existing Starting points menu. Its controls and explanatory panel update as they do today; each new sound describes a use, what to hear and the controls that create it. Hovering a changed control shows its own note. Try the existing Play a note button or the track's MIDI, and adjust to taste. Check the enlarged menu fits and all entries are reachable in both the app and plugin editor; only change the shared menu if the check shows a problem.

## Data and saving

Presets are factory settings, not new song data. Preserve the original 11 names, order and settings, in particular Init at index 0. Selecting one sets the existing parameters; the project and plugin state already save those values, not a new preset index. Old projects continue to load unchanged. Test a new selection through project save and load, and the processor state round trip used by plugins.

## The guide

Use the existing *What is subtractive synthesis?* guide and its pictures and Your track topic. The new explanations live in the existing starting-point explainer, with the guide available from the editor. No new guide topic or search entry is needed. Avoid changing guide facts or text; if implementation does change them, update SOURCES.md in that change.

## Code

Follow the existing instrument recipe in `docs/recipes.md`, copying `sub::getPresets()` and its existing editor and test paths. Add entries after the present list, using `sub::Settings` defaults for omitted controls and explicit notes on the changed, meaningful controls. `BuiltInSubtractiveEditor::applyPreset` already applies settings and displays the explanation. No new processor, parameter, format, threading or routing mechanism is planned. Work within existing parameter ranges. Do not promise accurate emulation of an acoustic instrument or a particular historical device. Sound choices are starting points, and a useful result depends on the note, velocity and mix.

## Libraries and models

None. No new component is adopted; the existing licence inventory is unchanged.

## Theory

The settings and descriptions below are proposed starting values, to be auditioned and adjusted if needed. Time units are seconds, sustain is a fraction, and unspecified settings are Init defaults. These descriptions make no numerical claims beyond the settings themselves. The existing guide explains oscillators, filters, envelopes and LFOs; no extra guide facts are introduced.

**Facts:** (exact, existing implementation) A filter envelope can move the cutoff; an amp envelope shapes level; the LFO can modulate filter, volume, pitch or pulse width (`BuiltInSubtractive.h`, `SubtractiveGuide.cpp`). The available modes include poly and mono. A band-pass favours a frequency region, and 100% amp sustain holds the envelope level while the note is held (existing guide and `getEnvelopeLevel`). Sound character is a subjective starting point, not a guarantee.

**Starting points and final explanation:**

| Name | Settings from Init | Why / what to hear | Notes on changed controls |
| --- | --- | --- | --- |
| Soft keys | triangle wave 1; cutoff 1800 Hz; filter envelope amount 1 octave; filter ADSR 0.005/0.6/0/0.3; amp ADSR 0.005/1.2/0/0.4; velocity 60% | A gentle, fading keyboard sound for chords or a melody. Listen for the brightness and level falling after each note. | Triangle: a soft starting wave. Cutoff: softens the wave. Filter amount and decay: the brightness falls after the attack. Amp decay and sustain: the note fades even while held. Velocity: playing softly changes the response. |
| Hollow organ | square wave 1; cutoff 5000 Hz; amp ADSR 0.01/0.3/1/0.08; velocity 20% | A steady, hollow sound for held chords. Listen for the note staying at a level while the key is held. | Square: gives the sound its hollow character. Cutoff: leaves the tone fairly bright. Attack: softens the start. Sustain: the note holds. Release: the sound stops shortly after the key. Velocity: a light response to how hard you play. |
| Bright stab | saw wave 2; detune 7 cents; mix 50%; cutoff 1200 Hz; resonance 15%; filter amount 3 octaves; filter ADSR 0.001/0.2/0/0.12; amp ADSR 0.002/0.3/0/0.12; output -4 dB | A short, bright chord for rhythmic accents. Listen for the sharp start and quick fade. | Detune and mix: the two saws move against each other. Cutoff, resonance and filter amount: a bright attack above a lower resting cutoff. Filter decay: brightness falls quickly. Amp decay and sustain: the sound fades while held. Output: leaves more room for chords. |
| Tremolo keys | triangle wave 1; cutoff 3500 Hz; amp ADSR 0.01/0.4/0.8/0.25; LFO 4 Hz, depth 35%, target volume | A pulsing keyboard sound for a simple repeated line or chord. Listen to the level rise and fall while a note is held. | Triangle: a softer wave. Cutoff: lets the note through while softening it. Amp sustain and release: the note holds and fades after release. LFO rate: four cycles per second. LFO depth and target: move the volume. |
| Airy sweep | saw wave 1; noise 20%; cutoff 700 Hz; resonance 10%; filter amount 2 octaves; filter ADSR 1.2/1/0.7/1.5; amp ADSR 0.6/0.5/0.8/1.2; output -5 dB | A slow, airy layer for held notes. Listen for the noisy edge and the gradual rise in brightness. | Noise: adds an airy layer. Cutoff and filter amount: give the filter room to open. Filter attack: brightness rises slowly. Amp attack and release: the sound fades in and out. Output: gives held chords more room. |
| Resonant reed | square wave 1; band-pass filter; cutoff 1200 Hz; resonance 30%; key tracking 50%; amp ADSR 0.02/0.3/0.85/0.2; velocity 60% | A focused, reed-like synth tone for a melody. Listen to the narrow, coloured tone and try moving the cutoff. | Square: a hollow source. Band-pass, cutoff and resonance: emphasise a region of the sound. Key tracking: the filter follows higher notes part of the way. Amp attack and release: soften the note's edges. Velocity: changes the response to playing strength. |

Notes should be attached to every changed audible control, including individual envelope settings when relevant; the table groups related controls to keep their wording short. Do not claim these sounds recreate real instruments. The LFO timing is derived: 4 Hz = 4 cycles per second. The new presets do not add advice or Your song rules, nor new picture data.

## Steps

### Group A, engine

- [ ] 1. Add the six named settings and explanations in `src/engine/BuiltInSubtractive.cpp`; leave the existing list untouched. Audition and refine starting values and explanation wording together if a sound is silent, clips or does not match its description; record any deviations here.
- [x] 2. Extend Subtractive tests in `tests/EngineTests.cpp`: unique names, text and meaningful notes, values in range, existing presets preserved, application and representative audio. Add a new preset project/state round trip. Run `./build.sh --test` with no warnings and record the result.

### Group B, UI

- [ ] 3. Check selection applies the new settings and the panel shows the right explanation using `SubtractiveEditor.cpp` and the shared `fxui` menu. Change UI only if necessary for access or readability; preserve plugin-host compatibility. Run `./build.sh --test` and record the result.

### Group C, check and docs

- [ ] 4. Use a scratch shot list and `--screenshots` to inspect the Subtractive editor in the app, a new preset's explanation and the plugin editor. If preset selection cannot be reached, extend `ScreenshotRunner` permanently rather than patching the app; inspect each resulting image. Audition representative notes/chords, including the slow sweep.
- [ ] 5. Update the relevant README Features bullet and `docs/tests.md`/`docs/code-layout.md` where their descriptions change. Update guide sources only if guide text changes. Comment progress on #118, preserving the remaining plugins and #112 follow-up. Run `./build.sh --test` if files changed after the last passing build. Record build output, screenshots, sound check and hand-off here.

## Tests

- At least 17 entries, unique non-empty names and explanations; Init first, original 11 unchanged; settings within documented ranges; new changed controls have meaningful notes.
- Existing chord render checks remain: each preset produces peak > 0.02 and < 1.0 on its existing test input. Render held notes long enough for Airy sweep; check Soft keys' level falls while held, Hollow organ sustains, and Tremolo keys varies in level. Pick numerical thresholds from the rendering and report them; do not claim a subjective genre or instrument resemblance as a measured fact.
- Apply every new preset and compare its parameters after JUCE parameter rounding; save/reload one new sound through the project and plugin state, then compare settings. Check old project loading remains covered.
- Check every new explanation is accessible from the selected preset and changed controls, and that the menu is usable in both host modes.

## Website hand-off

The existing Subtractive section on the features page may mention its wider choice of starting sounds. A screenshot of the selected preset and explanation could illustrate it. No new homepage section, guide page or dependency notice is needed. Decide website work separately after the app is committed.

## Left for later

The rest of #118: every other built-in plugin. The original DawIt Synth sound conversions and other Subtractive features remain in #112. User preset storage is #119.

## Changes while building

- Implemented all six presets and preserved the existing eleven. The existing render check verifies that each plays a chord without clipping; added name, explanation, envelope and LFO checks, a Resonant reed project round trip and an Airy sweep plugin-state round trip. A full `./build.sh --test` passed; after the final test-only addition the test target rebuilt without warnings and the full test executable again ended `All checks passed`.
- `ScreenshotRunner` can select a Subtractive preset in both track and plugin-style windows. Inspected `/private/tmp/dawit-subtractive-shots/soft-keys.png`, `airy-sweep.png` and `plugin-editor.png`; the values, explanations and pictures were visible. The shot list is `/private/tmp/dawit-subtractive-shots.json`.
- The six new sounds now round-trip every displayed control value after application. Rendered checks confirm Soft keys fades, Hollow organ sustains, and Tremolo keys changes level half a cycle apart. The test target rebuilt with no warnings and the complete test executable ended `All checks passed` after these additions.
- Inspected all six preset panels via `/private/tmp/dawit-subtractive-all.json`; their explanations were visible. Fixed the LFO target truncating “Volume” by fitting switch-button text; `/private/tmp/dawit-lfo-check.json` confirms it in both track and plugin-style windows. App and test targets rebuilt without warnings, and the test executable ended `All checks passed`.
- Added the missing Soft keys filter-attack explanation. A test now verifies that every new preset setting differing from Init has a note and that all displayed settings survive application. The full test executable passed, and `/private/tmp/dawit-soft-check/soft.png` shows the added note in the real app.
- Still to verify: listen to the six sounds and interactively inspect the expanded drop-down menu and individual control explanations. A direct app interaction attempt was blocked because computer-use permission is not granted; the automated screenshots show each preset's explanation panel, and the tests check its per-control notes. These steps remain unticked; do not treat this plan as complete yet.
