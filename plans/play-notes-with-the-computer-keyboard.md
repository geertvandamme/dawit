# Play notes with the computer keyboard

Status: Built, not committed
Idea: [Play notes with the computer keyboard](../FEATURE_IDEAS.html#play-notes-with-the-computer-keyboard)

## Goal
With no MIDI keyboard at hand, the letter keys play the selected track: A to L are the white keys, W E T Y U O P the black ones, Z and X move the octave, C and V the velocity. A strip at the bottom of the window shows which letter plays which note. Notes go in exactly as a MIDI keyboard's would, so previewing, playing along, recording and the scale filter all work. A beginner can start playing and recording melodies straight away, and the "What is MIDI?" guide explains what the computer keyboard can and can't do compared with a real one.

## Decisions for you
1. **Where the on-screen keyboard lives.** Options: a strip docked at the bottom of the main window, shown while typing is on; a small floating window, like Logic's Musical Typing. **Recommended:** the docked strip. It is always in view while you play, it doesn't cover the arrangement, and it goes away when typing is turned off.
   Answer: Docked strip.
2. **The scale filter.** Options: keys outside the song's key stay silent, as with a MIDI keyboard, and the strip greys them out; or, with the filter on, the rows are remapped so every letter plays a note of the key. **Recommended:** keep them silent. That is what the idea asks ("the same way as a MIDI keyboard") and it is what the filter already does. Remapping is under *Left for later*.
   Answer: Keys outside the key stay silent and are greyed out.
3. **K and the metronome while typing is on.** Options: K plays C5 and the metronome stays on its button, as in Logic and Live; or K is skipped. **Recommended:** K plays a note. Skipping it would leave a hole in the white keys. The button's tooltip and the guide say so.
   Answer: K plays C5.
4. **Shortcut and memory.** Options: Cmd+K (Logic's shortcut for Musical Typing) and remembered between sessions; no shortcut; not remembered. **Recommended:** Cmd+K, remembered, along with the octave and the velocity. The strip makes it obvious that it is on.
   Answer: Cmd+K, remembered with octave and velocity (recommendation, stated to the user before they approved the build).
5. **Velocity steps.** Options: steps of 20, as in Ableton Live (20, 40 … 120, 127); finer steps of 10. **Recommended:** steps of 20, with a start of 100. Seven steps can be told apart by ear and are quick to move through.
   Answer: Steps of 20, start 100 (recommendation, stated to the user before they approved the build).

## What the user does and sees
- **Turning it on:** a keyboard glyph button in the transport bar, right of the metronome (a new `GlyphButton` glyph), and *View > Play with Computer Keyboard* (ticked, Cmd+K). The strip's close button turns it off too.
- **The strip** (docked under the device bar, about 64 px high, full width, on `theme::panel`):
  ```
  [Z −] Octave: A plays C4 [+ X]   [C −] Velocity 100 [+ V]   | 16 piano keys with letters and note names |   Plays: Bass  [?] [×]
  ```
  The keys are C to D♯ an octave up: 9 white keys (letter at the top, note name such as "C4" at the bottom) and 7 black keys (letter). A held key is lit in `accent`, whether it is held from the keyboard or the mouse. A key the scale filter silences is drawn at 35% with no note name. When the target can't play notes (no track, or an audio track selected and nothing armed), the keys are dimmed and "Plays:" reads "Select a MIDI track to hear the keys".
- **Keys:** with typing on, a plain letter (no Cmd, Ctrl or Alt; Shift and Caps Lock are ignored) that is in the map plays or changes the octave or velocity. Letters outside the map (R, I, M, Q …) keep their usual meaning. Space, Return, Home and R keep their transport meaning, and Cmd shortcuts work as always. A focused text box, and the Lyrics row while tapping, still get their letters first. Holding a key doesn't retrigger the note on auto-repeat. Changing the octave while a key is held leaves that note sounding until its key comes up.
- **Mouse on the strip:** click a key to play it for as long as the button is held. Dragging across the keys moves the note from key to key. The − and + buttons step the octave and the velocity. Double-clicking a readout resets it (A plays C4; velocity 100). Right-click: *Reset octave (A plays C4)*, *Reset velocity (100)*, *Hide the computer keyboard*, separator, *What is MIDI? The computer keyboard...*. The strip's buttons don't take keyboard focus (ui-design.md *Focus*).
- **Where its notes go:** through `Engine::addMidiInput`, exactly like a MIDI keyboard: to the armed MIDI tracks, or to the selected (live) track when none is armed. R records them.
- **Tooltips** are listed in *Theory*.
- **Settings** (`engine.properties`): `typing.on`, `typing.octave` (the note A plays: 12 to 108 in steps of 12, default 60), `typing.velocity` (default 100). Nothing goes in the project.

## Data and saving
No song data and no format change. The three app settings above are read at start-up and written on change. Out-of-range stored values are clamped to the range and snapped to the nearest step.

## The guide
A new topic in "What is MIDI?" (`midiguide`, recipes.md *Topics in an existing guide*), placed after *Velocity*:
- **The computer keyboard:** "Play notes with the letter keys when there's no MIDI keyboard". The picture is the strip's keyboard, drawn by the same painter, with the current octave, velocity, held keys and scale-filter greying, and the target ("Plays: Bass"). Clicking a key plays it.
- It opens from the strip's *?* button, the strip's right-click menu and the transport button's right-click (`MainComponent::openGuide` at that topic).
- The guide's entry in `gen::guides`: add the search words "computer keyboard", "musical typing", "typing", "QWERTY" and "play without a keyboard", and add "the computer keyboard strip" to its opens-from. It already exists, so there is no *Start here* change.
- *Your clip* is unchanged. The new topic's live picture is its "your song" part; there is no advice.

## Code
Follow recipes.md *A command or menu item* (the toggle), *Topics in an existing guide* (the topic) and ui-design.md for the strip.
- **`ComputerKeyboard`** (src/engine, no UI, tested): the pure state of musical typing. It holds the letter-to-semitone map, the octave (the note A plays), the velocity ladder and the held keys. Pressing a letter gives a note-on, or nothing for a repeat, an unmapped key or a note above 127. Releasing gives the note-off for the note that key started, even if the octave has changed since. *Release all* gives note-offs for every held key. It also answers which of its 16 keys the scale filter silences for given key pitch classes. MIDI channel 1.
- **The typing handler** (src/ui, owned by `MainComponent`) turns key presses into `ComputerKeyboard` calls and sends the messages with `Engine::addMidiInput`. It is consulted *before* `TransportShortcuts` in `MainComponent::keyPressed`, and in the key listener the plugin windows share, so a synth window in front can be played too. Only one place decides what a letter does.
- **Key-up is the risky part.** JUCE has no key-up event, only `keyStateChanged`, and a child can swallow it. So: release on `keyStateChanged` by checking `juce::KeyPress::isKeyCurrentlyDown` for each held key; also check every held key on a 30 Hz timer (the safety net against stuck notes); release all when typing is turned off, when DawIt loses focus to another app, and *before* the live track changes in `MainComponent::changeListenerCallback` (otherwise the note-off would go to the new track and the old one would hang). Check whether `isKeyCurrentlyDown` needs the upper- or lower-case code on macOS.
- **Threading:** only the message thread touches `ComputerKeyboard`. The audio thread sees ordinary MIDI input through the existing `midiCollector`. No engine or audio-thread change.
- **The strip painter** is one function (in the strip's file) that the strip and the guide picture share (ui-design.md: one painter for a meaning). The strip finds its target the way the engine does (armed MIDI tracks, otherwise the live track) and repaints on a short timer. It reads `Track::scaleFilter` and the song key for the greying.

## Libraries and models
None.

## Theory

### Facts
1. An octave on a piano has 12 semitones: 7 white keys and 5 black. There is no black key between E and F or between B and C. **Exact** (common knowledge).
2. The map: A S D F G H J K L play C D E F G A B C D; W E T Y U O P play C♯ D♯ F♯ G♯ A♯ C♯ D♯ (semitones 0, 2, 4, 5, 7, 9, 11, 12, 14 and 1, 3, 6, 8, 10, 13, 15). R and I fall over the E–F and B–C gaps. This is the layout of Ableton Live's computer MIDI keyboard (middle row white keys, upper row black, Ableton manual 17.3.2) and Logic's Musical Typing. **Exact** (DawIt's definition; the convention is from the sources).
3. Z and X move the octave, and C and V the velocity, in both Logic (Apple Logic Pro help, *Play software instruments*) and Live (Ableton manual). **Exact.**
4. Live steps velocity in twenties (20, 40, 60 …), per the Ableton manual. DawIt's ladder is 20, 40, 60, 80, 100, 120, 127, starting at 100. **Exact** (DawIt's choice).
5. By default A plays note 60, middle C, called C4 in DawIt. This agrees with the existing MIDI guide's *Note numbers* topic. **Exact.**
6. The octave range for A is C0 (12) to C8 (108). That is 9 positions. At the top, P plays 108 + 15 = 123 ≤ 127. One octave lower than C0 would be C−1 (note 0), which no piano or GM drum needs. **Exact** (derived).
7. A real keyboard measures velocity from the time between two contacts under a key. This is already stated in the MIDI guide's *Velocity* topic. Most computer keys are plain on/off switches and can't. **Typical** (common knowledge; analogue gaming keyboards are the exception, so the text says "most").
8. Keyboards without a diode per key can reliably detect only two keys at once. A third may be dropped (blocking) or may add a key that wasn't pressed (ghosting). Common sequences often get 4 to 5 keys. N-key rollover keyboards register every key. **Typical** (Wikipedia *Key rollover*; Deskthority *NKRO*).
9. General MIDI drums: kick 36 (C2), snare 38 (D2), closed hi-hat 42 (F♯2). This matches `BuiltInDrums::getNote` and the MIDI guide. With A at C2 (two presses of Z), A plays the kick, S the snare and T the closed hi-hat. **Exact** (derived: 36 + 0, 2 and 6).
10. With the scale filter on, note-ons outside the key don't sound, but note-offs always pass (`Track.cpp`). In C major the black keys W E T Y U O P are silent. In G major (G A B C D E F♯), F is silent and only T sounds among the black keys. **Exact** (derived).
11. On an AZERTY keyboard A and Q, and Z and W, swap places, so the letters of the map no longer form two straight rows. **Exact** (common knowledge of the layout).
12. Velocity 100 is recorded as 100/127 = 0.787 of full. **Exact** (derived; `midiguide::toMidiVelocity` turns it back into 100).

### Topic text: The computer keyboard
**Name:** The computer keyboard
**Summary:** Play notes with the letter keys when there's no MIDI keyboard

**Text:**

No MIDI keyboard at hand? The letter keys of your computer can play one. Turn it on with the keyboard button in the transport bar, or Cmd+K. A strip at the bottom of the window then shows which letter plays which note.

The middle row of letters is the white keys: A plays C, S plays D, D plays E, and so on up to L, the D an octave above. The row above is the black keys, each sitting between the two white keys it lies between on a piano: W is C♯, between A and S, and E is D♯. R and I play nothing, because a piano has no black key between E and F or between B and C. That's 16 notes, an octave and a bit: enough for most melodies and bass lines.

At first A plays middle C (C4, note 60). Z and X move every key down or up an octave, from C0 to C8, so the same letters play a bass line two octaves down or a tune one octave up. C and V make the notes softer or harder in steps: 20, 40, 60, 80, 100, 120 and 127. It starts at 100.

Why steps? A real keyboard measures how fast each key goes down (see Velocity). Most computer keys are plain switches, on or off, so they can't tell a soft touch from a hard one. Every note gets the same velocity until you change it. Vary it afterwards in the piano roll, or with Humanize, to make the part sound played.

The keys play exactly as a MIDI keyboard would. They play the selected track, or the armed tracks when any are armed, and R records what you play. With the scale filter on, keys outside the song's key stay silent, and the strip greys them out. On a drum track each key is a drum. The kick is note 36 (C2), so press Z twice: then A plays the kick, S the snare and T the closed hi-hat.

Chords are harder. Many computer keyboards can only be sure of two keys at once. Hold three, and one may not sound, or a key you didn't press may play. Keyboards sold with "n-key rollover" (often gaming keyboards) don't have this limit. If a chord loses a note, draw it in the piano roll or put it on the chord track instead.

While it's on, the letters play notes instead of being shortcuts: K plays C, so turn the metronome on and off with its button. Space, Return and R still play, go to the start and record, and shortcuts with Cmd work as always. Typing in a text box still types. The notes follow the letters printed on the keys, so on a keyboard with another layout, such as a French AZERTY one, the two rows aren't straight.

The picture shows the keys as they are now: the letter on each, the note it plays, what you're holding down, and which track hears it. Click a key to hear it.

### Tooltips
- **Transport button:** "Play notes with the computer keyboard (Cmd+K): A to L are the white keys, W E T Y U O P the black ones. Z and X change the octave, C and V how hard. While it's on, K plays a note instead of the metronome. Right-click for the guide."
- **A key** (live values): "S plays D4 (note 62) at velocity 100. Click to hear it." A silenced key: "E plays D♯4 (note 63). It's not in C major, so the scale filter on Bass keeps it silent."
- **Octave − / +:** "Octave down (Z): A plays C3" / "Octave up (X): A plays C5". The lowest and highest positions say "A already plays C0, the lowest" and "A already plays C8, the highest".
- **Octave readout:** "A plays C4 (note 60), L plays D5 (note 74). Z and X move it an octave. Double-click for C4."
- **Velocity − / +:** "Softer (C): velocity 80" / "Harder (V): velocity 120".
- **Velocity readout:** "Every note gets velocity 100 of 127. A computer key can't tell how hard you press, so set it here. Double-click for 100."
- **Plays:** "The keys play the selected track, or the armed tracks when any are armed, as a MIDI keyboard does." With no target: "Select a MIDI track to hear the keys."
- **?:** "What is MIDI? The computer keyboard: what it can and can't do."
- **×:** "Stop playing with the computer keyboard (Cmd+K): the letters go back to being shortcuts."
- **Menu item description:** "Play the selected track with the letter keys, A to L and W to P".

### Example data for the picture
At the default octave: A 60 C4, W 61 C♯4, S 62 D4, E 63 D♯4, D 64 E4, F 65 F4, T 66 F♯4, G 67 G4, Y 68 G♯4, H 69 A4 (440 Hz), U 70 A♯4, J 71 B4, K 72 C5, O 73 C♯5, L 74 D5, P 75 D♯5. With no song state, the PDF shows this at velocity 100 with nothing held.

### "Your song" rules
None (no advice). The picture follows the live state.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| a, w, s, k, l, p at the default octave | 60, 61, 62, 72, 74, 75 | exact |
| r, i, q, m | no note | exact |
| "A" (upper case) | 60 | exact |
| X then a | 72 | exact |
| Z four times then a; a fifth Z | 12; stays 12 | exact |
| X four times then p; a fifth X | 123; A stays 108 | exact |
| velocity default; C once; C five times; V from 100 twice; once more | 100; 80; 20 (stays); 120 then 127; stays 127 | exact |
| hold a, X, release a | note-off for 60 | exact |
| a pressed twice without release | one note-on | exact |
| release all with a, e, g held | three note-offs: 60, 63, 67 | exact |
| silenced keys in C major | w e t y u o p | exact |
| silenced keys in G major | f, w, e, y, u, o, p (t sounds) | exact |
| recorded note on an armed track | pitch 60, velocity 0.787 | ±0.01 |
| MIDI guide topic count | 11 | exact |
| new topic text, summary, name | not empty | n/a |
| search "musical typing" | finds What is MIDI? | n/a |

## Steps

Group A, engine (`./build.sh --test` at step 2):
- [x] 1. `ComputerKeyboard` in `src/engine/ComputerKeyboard.{h,cpp}` (both CMake targets), as in *Code*. Tests: `runComputerKeyboardTests`, covering every row of the table except the guide rows. Also feed the messages through a `RecordingRig` with an armed MIDI track (copy the existing `addMidiInput` recording test) and check the recorded note. Done when the tests pass.
- [x] 2. The *Computer keyboard* topic in `MidiGuide` after `velocity`: name, summary and text from *Theory*. Give it an empty case in `MidiExplainer`'s picture switch (to be filled in B2). Add the search words and opens-from to the guide's entry in `TheoryGuides.cpp`. Tests: topic count 11, the text is not empty, "musical typing" finds the guide. Done when `./build.sh --test` passes without warnings.

Group B1, UI: typing and the strip (`./build.sh --test` at step 5):
- [x] 3. The typing handler in `src/ui`, owned by `MainComponent`, put ahead of `TransportShortcuts` and in the plugin windows' key listener. Add the key-up rules, the 30 Hz watchdog, and release all on turning off, on losing focus and before the live track changes (*Code*). Read and write the `typing.*` settings. Check: with typing on, A sounds on a selected synth track, K doesn't toggle the metronome, and R still records.
- [x] 4. The strip: a component with the shared keyboard painter, docked under the device bar in `MainComponent::resized` while typing is on. Give it the readouts, the − and + buttons, mouse playing with drag, the right-click menu, every tooltip from *Theory*, the target label and the scale-filter greying.
- [x] 5. The toggle: a new keyboard glyph button in `TransportBar` (its right-click opens the guide topic) and *View > Play with Computer Keyboard* with Cmd+K, ticked (recipes.md *A command or menu item*, with `commandStatusChanged` when it flips). Add `typing.on` to what `ScreenshotRunner` settings can turn on, if a setting doesn't already reach it at start-up. Done when `./build.sh --test` passes without warnings.

Group B2, UI: the guide picture (`./build.sh --test` at step 6):
- [x] 6. Fill the topic's picture in `MidiExplainer` with the strip painter (live state, click to play, the default state for the PDF), and add the `openGuide` route at the topic from the strip and the transport button. Check: `--export-guides` still passes.

Group C, check and docs:
- [x] 7. Real-app check, with a shot list:
  - (a) the pop song, Bass selected, `settings: {"typing.on": true}`, capturing the window. The strip should be under the device bar, nothing clipped, "Plays: Bass", and the octave reading C4.
  - (b) the same with `typing.octave: 36` and Bass's scale filter on (via its menu, or a setting/scene added to `ScreenshotRunner` for good). The black keys should be greyed and the note names should read C2 to D♯3.
  - (c) `guide: midi, topic: "The computer keyboard"`.
  - (d) a drum track selected. The keys should still be shown, and "Plays:" should name the drums.
- [x] 8. Docs:
  - A README bullet under *Features* (playing and recording).
  - docs/tests.md: the computer keyboard checks and the MIDI guide's count.
  - docs/code-layout.md: `ComputerKeyboard`, the handler and the strip.
  - docs/ui-design.md: the strip's size and place under *The arrangement*, and the rule that typing takes plain letters ahead of `TransportShortcuts`.
  - docs/architecture.md *How a change travels*: the computer keyboard enters as MIDI input.
  - FEATURE_IDEAS status (and the MIDI guide's idea, if it has one).

## Tests
Everything in the *Test numbers* table, in `runComputerKeyboardTests` and the MIDI guide and Theory Guides tests. Also:
- an octave change while a key is held doesn't strand the note;
- releasing a key that was never pressed gives nothing;
- the settings clamp: an octave of 200 becomes 108, and a velocity of 90 snaps to 100 (the nearest step; a tie goes up).

## Website hand-off
- Features page: a short section in *Playing and practising*: "No MIDI keyboard? Play with the letters." It covers the map, octave and velocity, the strip and recording with R, with a screenshot of the strip on the pop song (Bass selected, a few keys held if a scene allows, otherwise none).
- Main page: not needed, or one line in the "play and record" step.
- A screenshot of the guide topic picture (shot c).
- How it's made: no change.

## Left for later
- Rows remapped to the song key's notes with the scale filter on (decision 2).
- Sustain (Tab), pitch bend (1, 2) and modulation (3 to 8) keys, as Logic has.
- Layout-aware keys (by physical position, for AZERTY and QWERTZ). JUCE gives characters, not key positions.
- Drum names on the keys when a drum track is the target.
- ; and ' for E and F above L, as in Logic, which depend on the layout.

## Changes while building
- Group A hand-off: `daw::ComputerKeyboard` (`src/engine/ComputerKeyboard.h`) is a plain class, one instance for the handler. `press (char)` returns `Press { handled, optional<MidiMessage> noteOn }` (`handled` is true for every mapped key, including Z X C V and auto-repeats, so the caller consumes it); `release (char)` gives the Note Off for the note that key started; `releaseAll()`; `isHeld`, `isKeyHeld (index)`, `getHeldLetters()`. Keys are indexed 0 to 15 (semitones above A): `getLetter (i)`, `getKeyIndex (c)`, `isBlackKey (i)`, `getNote (i)` (-1 above 127). `getOctaveNote/setOctaveNote/moveOctave (steps)`, `getVelocity/setVelocity/moveVelocity (steps)` (setters clamp and snap, ties up), `isMapped (c)`. `getSilencedLetters (keyPitchClasses)` takes the same bitmask as `Track` (`ctx.keyPitchClasses`) and returns the letters of silenced keys in key order. The caller must lower-case nothing: both cases work. Tests are `runComputerKeyboardTests`, including a recording through `RecordingRig`.
- Group A hand-off, guide: `midiguide::Topic::computerKeyboard` sits after `velocity` (index 6, 11 topics); its picture case in `MidiExplainer::Picture::paint` is an empty `break` with a comment, to fill in B2 (the live-state setPlayhead repaint condition near it may need the topic too). The search words and the opens-from text are in `TheoryGuides.cpp`.
- Note: the plan's test row "V from 100 twice; once more" is read as 120 after the first press, 127 after the second, 127 after the third.
- Group B1 hand-off: `ComputerKeyboardTyping` (`src/ui/ComputerKeyboardTyping.{h,cpp}`, member `typing` of `MainComponent`, declared after `shortcuts`) is the one place letters are decided: `handle (KeyPress)`, a `KeyListener` (typing first, then `TransportShortcuts`) given to the plugin windows and the plugin list; `MainComponent::keyPressed/keyStateChanged` call it. It owns the `ComputerKeyboard`, reads/writes `typing.on/octave/velocity`, polls held keys at 30 Hz (also releases all when DawIt is not the foreground app), `releaseAll()` is called in `changeListenerCallback` before `setLiveMidiTrack`, in the destructor and on `setOn (false)`. `isKeyCurrentlyDown` on macOS tries both letter cases itself. Mouse notes go through `mouseKeyDown (index)` / `mouseKeyUp()` (the watchdog skips the mouse's letter).
- Group B1 hand-off, strip: `ComputerKeyboardStrip` (`src/ui/ComputerKeyboardStrip.{h,cpp}`) with `onClose` and `onShowGuide`; the shared painter is `keyboardpicture::paint (g, area, keyboard, silencedLetters, dimmed)`, plus `keyboardpicture::getKeyAt (area, point)` and `getNoteName` (with a real sharp sign). For the guide picture (B2) use `ComputerKeyboardStrip::getSilencedLetters (engine, keyboard)` and `getTargets (engine)`; the keyboard comes from `typing.getKeyboard()` (MainComponent has it; `MidiExplainer` will need a reference or a callback, and for the PDF a default `ComputerKeyboard` with nothing held). The strip sits at the very bottom of the window (below the bottom panel), not under the device bar, so it stays at the piano's place; 64 px, in `MainComponent::resized`.
- Group B1: the `openGuide (midi, computerKeyboard)` route is already wired from the strip's ? and the transport button's right-click (B2 needs only the picture). Menu command `computerKeyboardCommand` (Cmd+K, View menu, ticked); transport button `GlyphButton::Glyph::keyboard`. `ScreenshotRunner` calls `main.typing.reloadSettings()` after the shot's settings when they contain `typing.on` (so `settings: {"typing.on": true}` works). Shot checked once: the strip shows, nothing clipped, "Plays: Lead vocal", octave reads C4. Not done: the typed-key check in the live app (A sounding, K not toggling the metronome); the strip's mouse and keys were not exercised by hand.
- Group B2: `MidiExplainer` takes an optional third constructor argument `ComputerKeyboardTyping*` (MainComponent passes `&typing` where it opens the guide window at line ~669; the Start here/PDF-list instance at ~867 passes none). The topic's picture (`Picture::paintComputerKeyboard`) uses `keyboardpicture::paint` with the live keyboard, `getTargets` ("Plays: ..." caption) and `getSilencedLetters`; it repaints from the guide's timer when a signature of octave, velocity, held letters, silenced letters and target changes (`Picture::refreshLive`). Click and drag play through `typing->mouseKeyDown/mouseKeyUp`. For the PDF, `paintPrintPicture` sets `setPrinting (true)`: a default `ComputerKeyboard` (C4, velocity 100, nothing held, no target, no greying). `--export-guides` ran to exit 0 and `What is MIDI.pdf` was written; the picture itself was not looked at (Group C's shot c covers the live one). The picture's three text lines under the keys (octave, velocity, click hint) are mine, not from Theory.
- Group C: added a `"scaleFilter"` shot key (a track name or list) to `ScreenshotRunner`, documented in `DevScreenshots.h`. Shots a to d looked at: strip at the bottom, nothing clipped, "Plays: Bass" and A = C4; with octave 36 and Bass's filter the black keys are greyed and the names read C2 to D3; Kick selected shows "Plays: Kick" (the Drums bus selected shows the keys dimmed with "Select a MIDI track to hear the keys"); the guide picture shows keys, letters, note names, "Plays: Bass", the filter greying and the three text lines. Not checked by hand: real key presses (A sounding, K not toggling the metronome, key-up).
