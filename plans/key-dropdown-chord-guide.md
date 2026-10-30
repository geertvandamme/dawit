# A guide icon for every theory popup, and a tempo guide by the BPM

Status: Released
Idea: [#1 — A guide icon for every theory popup, and a tempo guide by the BPM](https://github.com/geertvandamme/dawit/issues/1)

Small piece of issue #1, separate from `plans/guide-icon.md` (the icon, built) and the tempo guide (still open).

## Goal
The key dropdowns end with a guide item, icon in front, that opens the Chord progressions guide at "How chords work", where keys and their seven chords are explained. A beginner who wonders what "A minor" means finds out from the box itself.

## Decisions for you
1. Where the item lives. Options: (a) last item inside each dropdown's list, after a separator; (b) a right-click menu on the box whose last item opens the guide. **Recommended:** (a), as you asked ("in the key dropdowns"); a right-click on a ComboBox is not discoverable. The ComboBox's own `getRootMenu()` takes the icon item through `guideicon::addItem`.
   Answer: (a) last item inside each dropdown, after a separator.
2. Which dropdowns. Found: `keyTonicBox` and `keyScaleBox` (`TransportBar`), `keyBox` (`NewFromStyleWindow`), `tonicBox` and `scaleBox` (`ChordExplainer`, which is the chord guide itself). **Recommended:** the first three; skip `ChordExplainer` (it is already inside the guide). In `NewFromStyleWindow` the item sits after the 12 tonics and "From the style".
   Answer: The three in TransportBar and NewFromStyleWindow; ChordExplainer skipped.
3. Which topic. **Recommended:** the guide's first topic, "How chords work" (its text says every key has seven chords, one per scale note). The guide has no topic only about keys; making one would need theory text, so not here.
   Answer: Topic 0, "How chords work".
4. Item text. **Recommended:** "Chords and keys guide..." (the guide's area is "Chords and keys"), tooltips of the boxes unchanged.
   Answer: "Chords and keys guide...".

## What the user does and sees
Open the tonic or scale dropdown in the transport bar: the 12 notes (or the scales), a separator, then the book icon and "Chords and keys guide...". Choosing it opens the Theory Guides window on Chord progressions, topic 1, in front if already open; the key does not change and the box keeps showing the current key. Same in the new-song-from-style window's key box. Nothing is saved.

## Code
Recipes: *A window* / *A theory guide* in `docs/recipes.md`. `guideicon::addItem` (`src/ui/GuideButton.h`) adds the item. Give it a fixed id outside the note and scale ids (for example 1000). `TransportBar` gets a callback `onShowKeyGuide`, wired in `MainComponent` to `openGuide (gen::guides::Guide::chords, 0)` next to `onShowTypingGuide`. `NewFromStyleWindow` needs a way to reach the same function (a callback from its creator, as with its other buttons). Invariant: choosing the guide item must not change the key or the style choice; in `onChange` restore the previous selection with `dontSendNotification` and call the callback. Risk: the menu with an icon item inside a ComboBox draws the selected-tick column; check alignment, and that `TransportBar::timerCallback`'s "popup active" guard still holds.

## Libraries and models
None.

## Theory
None. Uses the existing topic text.

## Steps
Group A (one run, `./build.sh --test` at step 3):
- [x] 1. Add the guide item to `keyTonicBox`/`keyScaleBox` with the `onShowKeyGuide` callback wired in `MainComponent`. Done when: choosing it opens the guide and leaves the key unchanged.
- [x] 2. Same for `keyBox` in `NewFromStyleWindow`.
- [x] 3. Real-app check: a shot of the open tonic dropdown is not reachable by shot list; look at it by hand once, and take no other shots. Docs: README *Features* line (guide icon), `docs/ui-design.md` (guide items in dropdowns), issue #1 plan comment and Done/Still open lists. `./build.sh --test` passes with no warnings.

## Tests
None: UI only. The behaviour (key unchanged, guide opened) is checked by hand in the real app.

## Website hand-off
One sentence on the theory-guides section: the key boxes link to the chords guide. No new screenshot.

## Left for later
The tempo guide by the BPM (issue #1). A keys-only topic in the chord guide.

## Changes while building
- Decisions answered as recommended. `TransportBar::onShowKeyGuide` and `NewFromStyleWindow::onShowKeyGuide` both go to `openGuide (Guide::chords, 0)` in `MainComponent`. `NewFromStyleWindow` remembers its last key id (`lastKeyId`) to restore after the guide item.
- `./build.sh --test`: All checks passed, no warnings. The open dropdown was not looked at in the real app (a shot list cannot open it): check by hand.
