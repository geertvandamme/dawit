# Bug: scrolling while the song plays jumps back to the playhead

Status: Released
Idea: [#96 — Bug: scrolling while the song plays jumps back to the playhead](https://github.com/geertvandamme/dawit/issues/96)

## Goal

Let people inspect another part of the arrangement during playback without the view immediately jumping back. Make following easy to resume or turn off across sessions.

## Decisions for you

1. Should following resume when the playhead naturally comes into the visible area after a manual scroll? **Recommended:** Yes, as the issue proposes. Alternatively, resume only on Play, an explicit seek or the menu command.
   Answer: Yes, resume on natural re-entry.
2. Should this include a continuously centred scrolling mode? **Recommended:** No; retain the existing page-by-page behaviour and leave centred scrolling for later. Alternatively, include it in this feature with a mode choice.
   Answer: Retain page-by-page movement; leave centred scrolling for later.

## What the user does and sees

View > Follow playhead is a ticked command with a shortcut, preferably F if it does not conflict with existing shortcuts or text entry. Its tick represents the persistent preference, not temporary suspension. It defaults on; turning it on immediately resumes following and brings an off-screen playhead into view during playback. Turning it off prevents automatic following until explicitly enabled again, including after Play, seeking and restart.

While enabled, a manual horizontal scroll (wheel, trackpad or scrollbar) or zoom suspends following when it changes the view, including while stopped. Programmatic scrolling must not be mistaken for a manual gesture. A new Play action or explicit playhead seek from the ruler or a lane resumes it; while suspended, the playhead naturally entering the visible area also resumes it. Resuming does not change the preference. Normal following retains the existing page jump and margin. Explain the temporary pause and how to resume in the ruler tooltip; distinguish the permanent menu setting. Selecting or editing a lane without moving the playhead must not resume following.

## Data and saving

Store the preference as `view.followPlayhead` in app settings, default true when absent. Temporary suspension is session state, not a song property or setting. No project format changes; old projects are unaffected.

## The guide

None: this is navigation behaviour, not music theory.

## Code

Follow the *A command or menu item* and *Checking in the real app* recipes. `ArrangementView` owns the preference and temporary follow state; `MainComponent` exposes the command and updates its tick through command status notifications. Identify all actual manual horizontal navigation, seek and Play paths, including transport and keyboard shortcuts, and signal intent explicitly where observing position or playing state alone would confuse a seek, loop wrap or rapid stop/start. Do not infer a manual scroll from the timer's own view changes or scrollbar updates. Playback, looping, recording and the existing playhead positions remain unchanged. All follow and UI state lives on the message thread; the audio thread and song data need no changes. A small UI-independent transition model may be used for meaningful headless tests, without moving UI into the engine.

## Libraries and models

None. No dependency or licence inventory change is proposed.

## Theory

None.

## Steps

### Group A, navigation and command

- [x] 1. Implement preference and temporary follow transitions in `src/ui/ArrangementView.{h,cpp}` and any appropriate UI-independent helper. Distinguish actual manual scroll/zoom from programmatic view updates; preserve page-follow behaviour. Tests: transition sequences and view bounds. Check: playback does not pull a manually moved view back.
- [x] 2. Wire explicit Play and seek intent across arrangement, transport and shortcuts as needed, and add the ticked View command, conflict-free shortcut, setting and ruler tooltip in `src/ui/MainComponent.{h,cpp}`, `src/ui/TransportBar.*` and relevant lane code. Tests: off stays off across Play, seeks and restart; enabled follow resumes as specified. Check: the menu tick reflects the saved preference.
- [x] 3. Cover edge cases in `tests/EngineTests.cpp` using the UI-independent rules where appropriate, including loop wrap, no-op gestures, zoom limits, scrolling while stopped, and repeated Play/seek. Run `./build.sh --test`; it must end with `All checks passed` and no warnings.

### Group C, check and docs

- [x] 4. Check the real app with a scratch-settings `--screenshots` shot list: playing and scrolled away, resumed follow, and the View menu enabled and disabled. Extend `ScreenshotRunner` if necessary to exercise and capture the interactions; inspect each shot and verify motion and persistence with reproducible checks, not screenshots alone.
- [x] 5. Update the Features bullet in `README.md`, `docs/tests.md`, `docs/code-layout.md` and any changed navigation/command documentation. Record completed and deferred scope and verification on issue #96; leave it open until committed. Run `./build.sh --test` with no warnings and record the result and website hand-off.

## Tests

- Default enabled; preference survives restart; disabled remains disabled through playback, seeks, visibility changes and restart.
- Manual horizontal scrolling and effective zoom suspend following; a no-op or internal update does not. Stopped scrolling remains suspended until an explicit resume condition.
- When enabled, Play and actual user seek resume; natural re-entry resumes; scrolling away does not immediately resume merely because the playhead was already visible before the gesture.
- A loop wrap or ordinary position update is not mistaken for a user seek; a real seek is recognised even if the resulting position is unchanged. Boundary and empty/zero-width views do not cause spurious jumps.
- Follow uses the existing page threshold and margin, and menu state tracks the persistent preference rather than suspension. Verify all input paths and shortcuts in the running app.

## Website hand-off

Consider a short mention in the arrangement or navigation section of `features.html`; no guide, home-page feature or dependency notice. A useful screenshot would show the arrangement and View menu; motion and temporary suspension need text rather than a still image.

## Left for later

Continuously centred scrolling and a follow-mode choice, if the recommended scope is accepted. Keep deferred scope visible in an open issue before closing #96.

## Changes while building

Group A: `FollowPlayhead` tracks temporary suspension and visibility transitions. `Engine::getTransportIntentVersion()` signals explicit Play and seek across all transport paths, including recording start, without treating audio-thread loop wraps as seeks. `ArrangementView` owns the preference and view changes; the View command uses Cmd/Ctrl+Shift+F to avoid the computer keyboard. Six headless transition checks passed. The first sandboxed test run could not write its MIDI fixture to the test cache; the full rerun with cache access ended in `All checks passed` without compiler warnings. Group C must exercise the remaining UI and preference cases in the real app.

Group C: `/private/tmp/dawit-follow-shots.json`, `/private/tmp/dawit-follow-checks.json` and `/private/tmp/dawit-follow-zoom.json` checked scroll, scrollbar, zoom, no-op gestures, stopped scroll and resuming Play; every screenshot was inspected. The runner inspected the actual View menu's item and tick, checked its registered shortcut, invoked the command and reopened the scratch settings file to check the saved value. This verifies menu data and persistence without desktop control; it does not photograph the native menu or restart the app. The final `./build.sh --test` passed without warnings. README, code layout and test documentation were updated. No theory was added and no dependency changed.
