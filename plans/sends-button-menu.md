# The Sends button opens a menu of sends

Status: Built, not committed
Idea: [Aux sends and buses](../FEATURE_IDEAS.html#aux-sends-and-buses)

## Goal
Clicking *Sends* on a track header now opens the same menu as the right-click *Sends* item: pick an existing return to send to, or make a new one (Reverb, Delay, Empty). Either way the mixer opens on the track, so a beginner can add a send and see its knob in one click. Follows [sends-button-mixer-layout.md](sends-button-mixer-layout.md); no new data.

## Decisions for you
1. After choosing an existing return, which track is selected? Options: (a) the track you clicked from, with the send on and the mixer on its strip (where the new knob appears); (b) the return itself, as you wrote ("select the send track"). **Recommended:** (b) for a new return (that is what `addReturn` already does, and its strip shows its chain), and for an existing return also (b), so both cases behave the same; the mixer scrolls to the return's strip and the send knob on the source track is one click back. Say if you would rather stay on the source track.
   Answer: (b) the return, for both a new and an existing return (recommended)
2. What does choosing a return that is already on do? Options: (a) keep the menu's toggle (turns the send off, check mark shows it is on); (b) leave it on and just open the mixer. **Recommended:** (a), because it is the same menu as the right-click one and the only way to switch a send off without the mixer; the mixer still opens. Turning off does not select the return; it shows the source track's strip.
   Answer: (a) keep the menu's toggle (recommended)
3. Extra menu entries? Options: (a) none beyond the right-click menu; (b) add "About sends and returns..." as the last item. **Recommended:** (b), because the click menu is the one place a beginner meets sends and CLAUDE.md wants the guide as the last item of such menus; it opens the existing sends guide, and the mixer is not opened for it. No "Show mixer" entry: every other choice opens it.
   Answer: (b) add the guide item, no Show mixer entry (recommended)

## What the user does and sees
- Click *Sends* on a track or bus header (not returns or master, as now): the menu opens under the button, with the returns (ticked when on, with their dB), a separator, *New return track >* Reverb, Delay, Empty, and the guide item. With no return the menu holds only *New return track >* and the guide item.
- Existing return, off: the send turns on at -10 dB (as the menu does), the return is selected, the mixer opens and scrolls to the return's strip.
- Existing return, on: the send turns off; the mixer opens on the clicked track's strip.
- New return: created with the send from the clicked track (starting points as now), selected, mixer opened on its strip.
- Dismissing the menu changes nothing and does not open the mixer. The tooltip becomes "Send this track to a shared reverb or delay, and show it in the mixer".
- Right-click menu unchanged, except that it also gains none of the new behaviour (it keeps not opening the mixer), so the two differ only in that.

## Data and saving
None.

## Code
Recipe: song row (a header control through `TrackActions`). `TrackHeader::sendsButton` calls a new `TrackActions` function that builds `makeSendsMenu` for the one track (ids as `applySendsChoice` expects, plus the guide item), shows it at the button, and on a result calls `applySendsChoice` and then `onShowSends` with the track to show. `applySendsChoice` should report which track to show (the return when it switched a send on or made a return, else the source), so the right-click path stays as it is. `MainComponent::onShowSends` keeps selecting the track it is given, opening the mixer and calling `showStripOf`. Invariants: guard with `isAlive` after the async menu, as `showSendMenu` does; the menu's changes go through `engine.setSend`/`removeSend`/`addReturn` on the message thread; the guide item uses `guideicon::addItem`.

## Libraries and models
None.

## Theory
None.

## Steps
- [x] 1. `TrackActions`: add `showSendsMenu (Track&, juce::Component* target)` (menu, guide item, the choice, then `onShowSends` with the track to show); make `applySendsChoice` return the `Track*` to show. Tests: none (UI). Done when it builds.
- [x] 2. `TrackHeader`: the button calls it instead of `onShowSends` directly; new tooltip. Add a `ScreenshotRunner` shot key for it only if the existing `"sends"` action cannot reach a state (the menu itself is a native popup and cannot be captured; shoot the result instead).
- [x] 3. Real-app check: demo song with the mixer closed; use `"sends": "<track>"` is the old path, so add `"sendsChoice": "<track>:<return name or New reverb>"` to `ScreenshotRunner`, which calls the same code as the menu result. Shots: (1) existing return chosen, mixer open on the return; (2) New reverb on a track with no returns, mixer open on the new Reverb strip. Scale 1, mixer area crop.
- [x] 4. Docs: README returns-and-sends bullet (the button opens the menu), `docs/ui-design.md` *Sends in the mixer and the device bar*, FEATURE_IDEAS.html *Done* bullet for the idea (the existing Sends-button bullet), `./build.sh --test` ends with `All checks passed`.

## Tests
No engine change; existing send tests must pass. If `applySendsChoice`'s return value is separable from the UI, one check: switching a send on returns the return, off returns the source.

## Website hand-off
`website/features.html#buses`: the sentence about the Sends button now says it opens a menu to pick or make a return, then shows the mixer. Optionally a screenshot of the mixer after "New reverb". No How it's made change.

## Left for later
Send level in the menu; sends for several selected tracks from the button (it uses the clicked track only).

## Changes while building
- Menu ids: returns from 1, new Reverb/Delay/Empty 101 to 103, guide 1000 (`TrackActions::sendsButtonChoice`, also used by the screenshot key `"sendsChoice": "Track:Return"` or `"Track:new reverb"`). No engine test added (UI only).
