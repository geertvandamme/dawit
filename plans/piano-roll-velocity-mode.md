# A MIDI editor window: notes, velocity, sustain pedal, pitch bend and controllers

Status: Released
Idea: [#56 — A MIDI editor window: notes, velocity, sustain pedal, pitch bend and controllers](https://github.com/geertvandamme/dawit/issues/56)

Small first slice of #56 (its "first step, velocity in the piano roll", without the lane): a velocity mode in the piano roll.

## Goal
In the piano roll, a *Velocity* mode where dragging up or down changes how hard the selected notes play, with notes coloured dark (soft) to light (loud) so the loudness can be seen. Velocity is already stored, saved, played and exported; only the editing is missing (a drawn note is always 0.8).

## Decisions for you
1. How is the mode entered and left? Options: (a) a *Velocity* toggle in the piano roll header beside *Select*, plus key V while the grid has focus; (b) hold a modifier key. **Recommended:** (a), because it matches the *Select* button, shows the state and is discoverable by tooltip. Velocity mode never draws or moves notes: click selects, drag changes velocity.
   Answer: (a) Velocity toggle beside Select, plus the V key (user chose this).
2. How does a drag map to velocity? Options: (a) relative: every selected note moves by the same amount, 1 step per 2 px up (Shift: 1 step per 8 px for fine, Alt: none), clamped to 1-127 per note, so differences are kept (a note at 127 stays there while the others rise); (b) absolute: all selected notes take the mouse's height. **Recommended:** (a), because it keeps a phrase's shape. A slight clamp loss at the ends is accepted.
   Answer: (a) relative, as recommended (user accepted the recommendations).
3. What does dragging on an unselected note do in velocity mode? Options: (a) it selects that note alone and changes it; (b) it does nothing until selected. **Recommended:** (a), as in move mode. Dragging on empty space draws a selection rectangle.
   Answer: (a) as recommended.
4. Colour ramp? Options: (a) the track colour from dark (velocity 1: darker by 0.7) to light (127: lighter by 0.5), also used outside velocity mode so notes always show loudness; (b) a fixed grey-to-white ramp; (c) ramp only in velocity mode. **Recommended:** (a), because the track keeps its identity. It replaces today's darkening only (today: darker by up to 0.9, no lightening). Selected notes keep their white outline.
   Answer: (a) track colour dark to light, shown in every mode (user chose this).
5. Does a drawn note take the last velocity set? Options: (a) yes, per piano roll window, starting at 0.8; (b) no, always 0.8. **Recommended:** (a), as #56 asks. Not saved; app-lifetime only, so no setting.
   Answer: (a) as recommended.
6. Show the value while dragging? **Recommended:** yes, a small "Velocity 96" label by the pointer (range shown when the notes differ: "Velocity 60 to 110"). The hover tooltip already says "velocity N"; add the loudness word only if #56's pp-ff mapping is approved (not in this plan).
   Answer: Yes, as recommended.
7. Undo: the app has no undo, so none is built; the drag is one change (one `notesChanged` per drag step, a note previews once when the drag starts and again when its value changes by 8 or more). Say if undo should come first.
   Answer: No undo; build as described (user chose this).

## What the user does and sees
- Header: a *Velocity* toggle (tooltip: "Velocity mode: drag up or down to change how hard the selected notes play. Click Velocity or press V to go back to editing notes.") next to *Select*. Cursor in the mode: up-down arrow over notes.
- Select notes as usual, turn it on, drag a note: up = louder, down = softer, for all selected notes. Notes redraw lighter or darker live. On the first drag step and every 8 steps the dragged note plays at its new velocity.
- Hover tooltip keeps "velocity N" (1 to 127). A note's rectangle gets lighter as it gets louder, in every mode.
- The mode is not remembered; it starts off.

## Data and saving
None. `MidiNote::velocity` (0 to 1) is already saved. Values are stored as n/127 so 1-127 round-trips.

## Code
Recipe: song data (the setter), then a piano roll control. `Engine::setNotesVelocity (MidiClip&, indices, offsets or new values)` next to `transposeClip`: takes the original values plus a delta in steps, clamps each note to 1/127 to 1, writes under the lock, calls `notesChanged()` and sends a change message only if some value changed; returns whether it did. `PianoRollGrid` gets a `velocityMode` flag set by the header toggle and key V (like `selectMode`), a drag mode `velocity` using `dragOriginal`, a shared `velocityColour (base, velocity)` helper (also usable by the score and tooltips later), a `lastVelocity` used when drawing a note, and mouse cursor and label painting. Threading: edit under `engine.getLock()` on the message thread, as the move drag does.

## Libraries and models
None.

## Theory
None. (Loudness words pp to ff are left for the velocity lane / guide work of #56.)

## Steps
- [x] 1. Engine: the velocity setter (clamp, no-op sends no change). Tests in `EngineTests.cpp` (see Tests). Done when `DawItTests` builds and passes.
- [x] 2. Grid: `velocityMode`, the velocity drag (select on press, relative change with the setter, Shift fine, preview rule, label), cursor, `velocityColour` ramp in `paint`, `lastVelocity` for new notes.
- [x] 3. Header: the *Velocity* toggle with tooltip, key V in `keyPressed`, kept in sync, right-click menu unchanged. `./build.sh --test` passes with no warnings.
- [x] 4. Real-app check: shot list with a clip of mixed velocities selected, velocity mode on (add a `ScreenshotRunner` setting `pianoRollVelocityMode` if the list cannot reach it), cropped to the piano roll at scale 1; notes must run visibly dark to light and the toggle must show as on.
- [x] 5. Docs: README piano roll bullet, `docs/tests.md`, `docs/code-layout.md`, issue #56 Done/Still open, and a plan comment on the issue.

## Tests
- Setter: +20 steps on notes at 40, 63, 127 gives 60, 83, 127 (127 stays clamped); -100 gives 1 for all; a delta of 0 or all already at the limit returns false and sends no change message.
- Differences kept: three notes at 50, 70, 90 raised by 10 are 60, 80, 100.
- Save and load keeps a changed velocity (n/127 within 1e-4).
- Rendered audio: a note at 127 is louder than one at 40 (peak ratio above 2).

## Website hand-off
`website/features.html`, the piano roll section: a sentence on Velocity mode and the dark-to-light notes; one screenshot of the piano roll in velocity mode. No How it's made change.

## Left for later
From #56, not in this plan: the velocity lane (per-note bars, ramps, double-click number, Alt-scale), Set velocity... / Soft / Medium / Loud menu items, the loudness words and velocity guide picture, controllers lanes (sustain, pitch bend, mod wheel, expression), the MIDI editor window, release velocity, and live wheels. The issue stays open.

## Changes while building
- Tests: the setter checks only (no save-and-load or rendered-audio check: velocity saving and playback are existing, tested features).
- Setter is `Engine::setNoteVelocities (clip, original, indices, steps)`. Screenshot key `"velocityMode": "<track>"` spreads the first clip's velocities, selects all and turns the mode on.
- Cursor in velocity mode over a note is the up-down resize cursor; Alt does nothing special.
