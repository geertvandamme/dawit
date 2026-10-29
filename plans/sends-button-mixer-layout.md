# A Sends button in the track header, and sends stacked in the mixer

Status: Built, not committed
Idea: [Aux sends and buses](../FEATURE_IDEAS.html#aux-sends-and-buses)

## Goal
A beginner who sees a track and wonders "how do I add reverb to it?" gets a small **Sends** button on the track's header that opens the mixer on that track's strip, where the sends are. In the mixer, each send becomes one row (knob on the left, the return's name to its right), so the names are readable and a long name is no longer squeezed above a knob. Builds on [aux-sends.md](aux-sends.md); no new data, no new saved settings.

## Decisions for you
1. Where does the button go in the header? The top row (name, ?, arm, M, S, x) is full; the bottom row has FX and, on MIDI tracks, Scale. Options: (a) bottom row, between the FX button and Scale (FX shrinks by about 44 px); (b) top row, replacing nothing, by shortening the name. **Recommended:** (a), because the name is more valuable than the FX label, which already shortens.
   Answer: bottom row, between FX and Scale (recommended)
2. Which tracks get the button? Options: (a) every track and bus (not returns or the master: they have no sends); (b) only when the song has a return. **Recommended:** (a), because the button is also how a beginner discovers that sends exist; with no return the mixer shows no knobs, so the mixer's hint says "Add a return track (Track > New Return Track) to send this track to a shared reverb or delay" when the song has none.
   Answer: every track and bus, not returns or master (recommended)
3. How many sends does a strip show? Rows are 22 px, so four sends take 88 px, the same as the two rows of knobs now. Options: (a) keep 4 and scroll no more; (b) show up to 6 and let the strip scroll when the mixer is short. **Recommended:** (a), because the strip has no scrolling today and the guide, device bar and docs all say four; sixth-and-later returns stay reachable through the track menu's *Sends* and the device bar chips.
   Answer: keep 4 send rows (recommended)
4. What does the button do when the mixer is already open? Options: (a) select the track, open the mixer if closed, and scroll the strip into view; (b) toggle the mixer closed if it is open on this track. **Recommended:** (a), simple and predictable; the Mixer button and Cmd+2 close it.
   Answer: select the track, open the mixer if closed, scroll the strip into view (recommended)

## What the user does and sees
- **Header:** a small *Sends* button (text "Sends", about 44 px wide, 18 px high like FX) on tracks and buses. Tooltip: "Show this track's sends in the mixer: how much of it goes to each return, such as a shared reverb or delay". Clicking selects the track (as clicking the header does), opens the mixer under the device bar if it is closed (the way `DeviceBar::onShowMixer` does), and scrolls the mixer so the strip is visible. The strip shows as selected.
- **Mixer strip:** the sends area, between the pan and the fader, becomes a stack of rows, one per return (first four), each 22 px: the 30 px dial shrinks to a 20 px dial on the left, then the return's name to its right in the return's colour (10.5 pt, shrinks to fit, then ellipsis), and PRE in yellow after the name for a pre-fader send (or on the dial's right edge if the name leaves no room; the name gives way first). With the 88 px strip that leaves about 56 px for the name; a name that does not fit is cut, and its tooltip says the full name. Rows exist on every strip (also return and master, left empty) so the faders line up. Four sends take 88 px, so the strip is no taller than today.
- Unchanged: drag to set, Off to +6 dB, double-click for Off, right-click for the send menu (Pre-fader / Post-fader, Off, the return's chain), tooltips, and the knob's value popup.
- Nothing new is remembered between sessions.

## Data and saving
None.

## Code
Recipes: *A window* is not needed; this is a row control (recipes: song row, wiring through `MainComponent`). `TrackHeader` gets a button and a callback that `ArrangementView` forwards to `MainComponent` (the pattern of `onOpenClipEditor`); `MainComponent` selects the track, calls `showBottomPanel (BottomPanel::mixer)`, and asks `MixerView` to scroll to the track's strip. `ChannelStrip` keeps `SendKnob`; only the layout in `resized`, the name drawing in `paintOverChildren` and the `sendRowHeight`/`maxSendKnobs` constants change. Invariants: strips stay `ChannelStrip::width` wide; every strip reserves the same sends height; the audio thread is untouched.

## Libraries and models
None.

## Theory
None.

## Steps
- [x] 1. Mixer layout: stack the sends one per row (knob left, name and PRE right), 22 px rows, the same reserved height on every strip; update the `MixerView.h` comment. Tests: none (layout). Done when it builds.
- [x] 2. Sends button in `TrackHeader` (bottom row, tooltip), forwarded through `ArrangementView` to `MainComponent`, which selects the track, shows the mixer and scrolls the strip into view (add a `MixerView` function for this). Also the mixer hint for a song with no return. Check: click it in the demo song with the mixer closed. Done when `./build.sh --test` ends with `All checks passed` and no warnings.
- [x] 3. Real-app check (below).
- [x] 4. Docs: README returns-and-sends bullet (stacked rows; the Sends button), `docs/ui-design.md` *Sends in the mixer and the device bar* (the dial size, row layout, button), `plans/aux-sends.md` mixer line gets a pointer to this plan, FEATURE_IDEAS.html status under *Done* for the aux-sends idea (a bullet for each).

## Tests
No engine behaviour changes, so no new checks; the existing send tests must still pass. A UI-only change is checked in the real app.

## Real-app check
Shot list for `--screenshots` (song "sends"): (1) mixer open, with the Snare, Keys and Lead vocal sends turned up, Snare pre-fader: rows read well, no clipped names; (2) arrangement with the mixer closed, showing the Sends button on a header (zoom on the headers); (3) after clicking Sends on Keys: mixer open, Keys strip selected. Step 3 needs either a new `ScreenshotRunner` action (`"click": "sends"` on a named track) or the same call the button makes; add it to `DevScreenshots.cpp` and document it in `DevScreenshots.h`. Also a song with one return and a song with four (names of mixed length, e.g. "Quarter echo") to check clipping.

## Website hand-off
`website/features.html#buses`: the screenshot `sends-mixer.png` and its alt text and caption ("two small knobs named Reverb and Delay under each pan") go out of date; retake it, and update the text near it if it says "knob per return under the pan". Check the other screenshots of the mixer (`grep mixer website/assets/screenshots`) for send knobs. Add one sentence and a header screenshot for the Sends button. No How it's made change.

## Left for later
More than four sends per strip, a *+ Send* picker; a strip scroll when the mixer is short.

## Changes while building
- The dial is drawn by `SendKnob::paint` (a 20 px default JUCE dial was almost invisible), so it fills its 20 px.
- The button callback is `TrackActions::onShowSends` (shared with the other row controls), not a new `ArrangementView` callback. The screenshot action is `"sends": "<track>"`, which runs before `"panel"` in a shot, so a shot that uses it must not also set `"panel": "closed"`.
