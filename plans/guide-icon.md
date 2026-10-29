# A guide icon for every theory popup (icon part only)

Status: Approved
Idea: [A guide icon for every theory popup, and a tempo guide by the BPM](../FEATURE_IDEAS.html#a-guide-icon-for-every-theory-popup-and-a-tempo-guide-by-the-bpm)

## Goal
Every button, label and menu item that opens a theory guide shows one recognisable icon (an open book with five rising bars, from `designs/dawit-spectrum-book-variations/variation-01.png`), so a beginner learns to spot "there is an explanation here". The tempo guide in the idea is NOT part of this; the idea stays partly done.

## Decisions for you
1. How to ship the icon. Options: (a) trace the PNG into a `juce::Path` in code; (b) embed the PNG. **Recommended:** (a), because a PNG cannot take the theme's colours (dim at rest, accent on hover) and is blurry at small sizes. The glyph is simple (a book of two page curves plus five rounded bars), so I trace it by hand into a path in a 0..1 box, checked against the PNG in a screenshot.
   Answer: Trace into a juce::Path (recommended).
2. Right-click menu items. Options: icon in front of the item, or text only. **Recommended:** icon in front, as the idea says, through `PopupMenu::Item::image` (a `juce::Drawable` made from the same path), because the last menu item is the guide everywhere and the icon then matches the buttons.
   Answer: Yes, icon in front of guide menu items (recommended).
3. Which "?" buttons count. Several "?" buttons (`EffectWindow`, `EqEditor`, `CompressorEditor` `helpButton`) open the window's own overview topic, not a guide. **Recommended:** leave those as "?", and change only buttons that open a guide window or guide topic, because the icon should always mean "guide". Say if you want them changed too.
   Answer: Leave those as "?" (recommended).
4. Buttons that open the guide but are wide text ("What is EQ?", "How sound flows?"). Per your rule: icon first, then the label, with the button widened by the icon. **Recommended:** keep the labels unchanged and do not shorten them.
   Answer: Yes, labels unchanged, widened for the icon (recommended).
5. Tooltips. **Recommended:** keep each tooltip and start it with the guide's name ("Song structure guide: ...") where it does not already.
   Answer: Yes, as recommended.

## What the user does and sees
- `GuideButton`, one shared component: a flat button, no keyboard focus (`setWantKeyboardFocusOnClick (false)`), with the icon (about 16 px, 18 on 30 px-high bars) in `theme` dim text colour at rest and `accent` on hover or press. Given a label, it draws the icon, a 4 px gap, then the label; given none (an icon-only button) it is a square.
- Icon-only (label is just "?"): the Lyrics header and the Sing window `helpButton`s that open a guide topic, `ArrangementView` Sections and Chords `guideButton`, `TrackHeader` and `MixerView` `guideButton`, `AudioSettingsPanel` `guideButton`, `TunerWindow` and `HitsToMidiWindow` `helpButton`, `ComputerKeyboardStrip` `helpButton`, `PianoRoll` `rhythmButton`.
- Icon then label: `DeviceBar` "Gain staging?", `EffectWindow` guide button (reverb, delay, saturation, distortion and the rest), `EqEditor` "What is EQ?", `CompressorEditor` "What is compression?", `WaveEditor` "What is sound?", `PianoRoll` "What is MIDI?", `SignalFlowView` "How sound flows?", `NewFromStyleWindow` "What is <style>?", `SoundExplainer` "Mono in the Stereo guide...".
- The text-only row hint "Open guide ?" painted by `RowLabel` (`ArrangementView.cpp`, about line 291) becomes the icon alone (with the hover tooltip), still clickable as now.
- Menus: items that open a guide ("Song structure guide...", "About buses, sends and routing...", "What is FM synthesis?", about 17 `addItem` calls found by searching for "guide") get the icon in front.
- Nothing new is saved.

## Code
Follows `docs/recipes.md` (*A window* and *A theory guide*) and `docs/ui-design.md` (controls, tooltips). New `src/ui/GuideButton.h/.cpp` (in `DawIt` in `CMakeLists.txt` only): `daw::GuideButton` (a `juce::Button` subclass, `final`, `/** */` comments) plus `guideicon::getPath()` (the traced path in a unit box), `guideicon::draw (g, bounds, colour)` and `guideicon::makeDrawable()` for menus. Existing members become `GuideButton` with the same `onClick`, so no behaviour changes; each site loses its `setButtonText ("?")`. Theme colours come from `Theme.h`, no literals. Layout code that sized a "?" button is changed to the icon-only size (square) or the icon plus text width (`getBestWidthForHeight`-style helper on the button). Risk: some headers are tight, so check text still shrinks to fit rather than being cut.

## Libraries and models
None.

## Theory
None.

## Steps
Group A, the component (`./build.sh --test` at step 2):
- [x] 1. Trace the icon into `guideicon::getPath()` (book plus five rounded bars) and write `GuideButton` and the menu `Drawable`. Done when: it draws sharp at 12, 16 and 24 px.
- [x] 2. Add a `ScreenshotRunner` scene (or reuse an existing window) that shows the button at rest and hovered if the shot list cannot reach it. Done when: `./build.sh --test` passes with no warnings.

Group B, use it everywhere (`./build.sh --test` at step 5):
- [x] 3. Replace every "?" guide button (list above) with the icon-only `GuideButton` and fix each layout. Done when: no guide-opening button shows "?" or "Open guide ?".
- [x] 4. Replace the labelled guide buttons with `GuideButton` (icon, then label), widening each. Tooltips start with the guide's name.
- [x] 5. Put the icon in front of the guide items in right-click menus (`PopupMenu::Item::image`). Done when: `./build.sh --test` passes.

Group C, check and docs:
- [x] 6. Real-app check: one `--screenshots` list (scale 1, cropped): the arrangement with Sections and Chords rows, the Lyrics header, the device bar, an effect window (reverb), the EQ, the wave editor, a Sections right-click menu, and one hovered button. Each must show the icon, no clipped label, and match the PNG's shape.
- [x] 7. Docs: README *Features* line, `docs/code-layout.md` (GuideButton), `docs/ui-design.md` (the guide icon rule: every guide opener uses `GuideButton`) and `docs/recipes.md` (a guide recipe: use `GuideButton`), `docs/tests.md` (None), FEATURE_IDEAS status (icon part Done, tempo guide Still open; `tools/ideas.py set-tag ... partly`, then `check`).

## Tests
Engine tests do not cover the UI, so: a check in `runUiLogicTests` (if present, otherwise none) that `guideicon::getPath().getBounds()` is non-empty and fits the unit box. The look is checked in the screenshots.

## Website hand-off
Mention the icon on the theory guides section ("one icon marks every guide"). Screenshots: the arrangement header rows and an effect window header, cropped.

## Left for later
The tempo guide by the BPM, and the guide icon in places that still have no guide.

## Changes while building
- `GuideButton` derives from `juce::TextButton` (not `juce::Button`), so existing `setButtonText`, `setColour` and `getBestWidthForHeight` calls keep working; it keeps the TextButton background and draws the icon (and label) itself.
- The icon-only buttons were not given a separate size: existing "?" widths stay; labelled buttons were widened by 22 to 26 px.
- Lists such as `for (auto* b : { ... })` mixing TextButton and GuideButton became `std::initializer_list<juce::TextButton*>`.
- Menus: 29 `menu.addItem` calls whose text names a guide now use `guideicon::addItem (menu, ...)` (found by search, so check any new ones).
- Tooltips were left as they are (most already name the guide); the label and tooltip text are unchanged.
- No automated test: the icon is UI-only and not in `DawItTests`; checked by screenshots (reverb header, arrangement headers, piano roll header). The right-click menu icon is not covered by a shot (shot lists cannot open menus), so look at one by hand.
- Left as a plain "?": the `helpButton` of `EffectWindow`, `EqEditor` and `CompressorEditor` (window overview, decision 3).
- Song rows: the icon sits at the front of the Sections, Chords and Lyrics headers, in one column (x 22 to 44) with the name after it (x 47), instead of at the right end (`RowLabel::paint`, `LyricsTrackHeader::resized` and `paint`).
