# Show monitoring advice before turning on the input monitor

Status: Released
Idea: [#185 — Show monitoring advice before turning on the input monitor](https://github.com/geertvandamme/dawit/issues/185)

## Goal
The first time someone turns Monitor on, DawIt explains headphones, the real delay and the interface's direct monitoring
first, and only turns the input on when they choose Enable. Cancel, closing the prompt or opening the guide leave it off, so
a microphone never howls before the beginner has been warned. Audit finding 33.

## Decisions for you
1. When does the advice count as seen (`recording.monitorWarned`)? Options: when shown / only when Enable is chosen.
   **Recommended:** only on Enable, because someone who cancels or reads the guide has not yet chosen to monitor, and sees the
   short prompt again next time. Answer: only on Enable (user, 2026-11-11).
2. Buttons. **Recommended:** "Enable monitoring" (default), "Cancel" (Escape), "About hearing yourself..." (opens the guide
   topic, monitoring stays off). Answer: the three recommended buttons (user, 2026-11-11).

## What the user does and sees
- Monitor button on an armed audio track header, or Monitor in the track's right-click menu. Turning it **off** is always
  immediate. Turning it **on** after the advice was accepted once is immediate too (later use).
- First use: the button springs back to off and a prompt shows the existing text (headphones, "about N ms late now" from
  `recordingguide::getRoundTripMs`, direct monitoring has no delay) with the three buttons above. Enable turns Monitor on for
  that track and the button shows it; the other two leave it off.
- Keyboard: Return enables, Escape cancels, the guide button is reachable with Tab. Tooltips of the button stay as they are.
- If, while the prompt is open, the track is deleted or disarmed, or the audio device goes away, Enable does nothing.
  No audio device: the prompt still shows (the round trip reads "a little late") and Enable only turns the flag on as today.

## Data and saving
None. Monitor is not saved. The `recording.monitorWarned` user setting keeps its key and is written only per decision 1.

## Code
Recipe: *a command* (app-wide behaviour in the UI layer) in [docs/recipes.md](../docs/recipes.md); example is the existing
`recordingui::setMonitor`.
- `recordingui::setMonitor` stops calling `engine.setMonitor` before the advice. When turning on with the advice unseen, it
  shows the prompt and returns without changing the engine; on Enable it re-checks `engine.indexOf (&track) >= 0`,
  that the track is audio and armed, then calls `engine.setMonitor` and saves the setting.
- The async callback captures the engine and track the way `showInputMenu` does (index check, no raw use after deletion);
  the caller's toggle button is refreshed from the engine afterwards. Guard any captured component with a
  `juce::Component::SafePointer`.
- Callers (`TrackHeader` monitor button, `TrackActions` menu item) set the toggle back to `track.monitor.load()` when
  the monitor did not turn on, and the header's existing refresh keeps it in sync after Enable. `setMonitor` returns
  whether it changed the engine at once, so callers know.
- Threading: all on the message thread; `engine.setMonitor` keeps its own locking.
- Risk: the `ToggleButton` flips itself before `onClick`, so the spring-back must not trigger another `onClick`
  (`dontSendNotification`).

## Libraries and models
None.

## Theory
None. The prompt text is the existing text, unchanged apart from the button labels (UI text is ours); no guide fact changes,
so `SOURCES.md` is untouched.

## Steps
Single group.

- [ ] 1. Rework `recordingui::setMonitor` in `src/ui/RecordingActions.cpp/.h` as above: advice first, three-button prompt,
  Enable-only turn-on with the track-still-valid check, setting written per decision 1, doc comment updated.
  Done when: off is immediate, first-use on defers, guide route and Cancel change nothing.
- [ ] 2. Update the callers in `TrackHeader.cpp` and `TrackActions.cpp` to spring the button back and refresh. Done when the
  header button and the menu item both show the true state after Cancel, guide and Enable.
- [ ] 3. Tests in `tests/EngineTests.cpp` only where logic can be headless: a small pure helper for "may Monitor be turned
  on now" (track still present, audio, armed) if split out of the UI function; otherwise the UI is checked in step 4.
  `./build.sh --test` must end with `All checks passed`, no warnings.
- [ ] 4. Real-app check: shot list with `--screenshots` on scratch settings (first use is the default there): arm an audio
  track, click Monitor, capture the prompt (crop to the dialog) and the header after Cancel (button off). Add a scene to
  `ScreenshotRunner` if the shot list cannot click Monitor. Check by keyboard (Tab, Return, Escape) by hand, and later use,
  deleting the track during the prompt and no device in a quick manual run, noting results in the issue.
- [ ] 5. Docs: README Features bullet on Monitor, `docs/tests.md` (audio recording line), `docs/code-layout.md` if a
  helper is added, GitHub issue acceptance checkboxes and the progress note.

## Tests
- The helper refuses a deleted, MIDI or unarmed track and accepts an armed audio track.
- Through the fake device: after a simulated first-use Cancel the track's `monitor` stays false and the monitored input is
  silent; after Enable it is on and the input is heard (reuse the existing monitoring test).

## Website hand-off
Small safeguard; at most a line in the features page's recording section saying Monitor asks first and explains
headphones. Screenshot: the prompt over a track header.

## Left for later
The wider recording-preparation flow (audit finding 29) and latency advice (finding 31) stay in their issues.

## Changes while building
- Tests: `src/ui` is not linked into `DawItTests`, so `recordingui::canMonitor` and the prompt cannot be tested headlessly (steps 3 is
  covered by the existing monitoring tests and by the real-app check). No new checks.
- Button order is Enable monitoring, About hearing yourself..., Cancel: JUCE gives the last button result 0, which is also
  Escape and closing the window, so Cancel had to be last to make those leave Monitor off.
- Added the `monitorAdvice` ScreenshotRunner scene (armed Voice track, Monitor asked on).
- The prompt's callback holds `juce::WeakReference`s to the engine and the track (added `JUCE_DECLARE_WEAK_REFERENCEABLE (Track)`), so a track deleted
  and a new one allocated at its address cannot pass; `canMonitor` still checks it is in the song, audio and armed.
- The `monitorAdvice` scene clicks the Voice header's real Monitor button (`TrackHeader` got `friend class ScreenshotRunner`) and,
  with `"resolve": "enable"` or `"cancel"`, answers the prompt. Real-app result: after Cancel the header's Mon is off, after Enable it is on.
