# A MIDI editor window: notes, velocity, sustain pedal, pitch bend and controllers

Status: Planned
Idea: [#56 — A MIDI editor window: notes, velocity, sustain pedal, pitch bend and controllers](https://github.com/geertvandamme/dawit/issues/56)

Second slice of #56: **controller data and lanes**. Velocity mode in the piano roll is built (`plans/piano-roll-velocity-mode.md`); the separate window and live wheels stay open.

## Goal
A MIDI clip keeps the sustain pedal, pitch bend, mod wheel, expression, volume, channel pressure and any other controller, with its notes. They are saved, played, bounced, exported in MIDI files and kept when recording, and drawn and edited in lanes under the piano roll, beside a velocity lane. A beginner can see what a recorded performance holds, draw a bend or a pedal, and learn in *What is MIDI?* what each one does.

## Decisions for you
1. **Where the lanes live.** Options: (a) under the bottom-panel piano roll now, the window later hosting the same piano roll; (b) build the window first. **Recommended:** (a): one editor, no second note editor to keep in step, and the window becomes a small later slice.
   Answer:
2. **Recording the pedal.** Today a pedalled note is recorded longer. Options: (a) record pedal events and the keys at their real lengths, as DAWs do; (b) keep the longer notes, drop the pedal. **Recommended:** (a): the pedal becomes editable and the score shows the keys as played. An instrument that ignores the pedal then plays shorter notes; *Your clip* says so.
   Answer:
3. **What DawIt's instruments respond to.** Options: (a) the built-in synth and DawIt Subtractive, FM, Additive and Physical take the pedal, pitch bend (±2 semitones), volume and expression (GM curve); mod wheel and pressure wait for the live-wheels slice; DawIt Drums ignores all; (b) only the built-in synth. **Recommended:** (a): a lane drawn on DawIt's own sound must be heard. Volume and expression start at 127, so existing songs sound the same.
   Answer:
4. **Playback rules (chase and reset).** **Recommended:** on stop, jump, loop wrap and at the end of a clip that used them, DawIt centres the bend, lifts the pedal, sets mod wheel and pressure to 0 and expression to 127 (the controllers a *Reset All Controllers* puts back); then it sends each lane's last value before the new position. Volume, pan and other controllers keep their last value. A part then sounds the same wherever playback starts, and no bend or pedal hangs. The alternative, no chase, is simpler but plays wrongly from the middle of a bend.
   Answer:
5. **Steps or curves.** Options: (a) events whose value holds until the next, as on a cable; lines and freehand write a point every 1/32 beat at most, skipping repeats; (b) points joined by lines, the engine making steps. **Recommended:** (a): what you see is what is sent, recorded and exported.
   Answer:
6. **Project format.** **Recommended:** a song with any controller event saves as version 4, so an older DawIt refuses it instead of silently dropping bends and pedals (as returns did for version 3). Songs without them keep their old version.
   Answer:

## What the user does and sees
- **Lane area** under the note grid in the piano roll (not in score mode), sharing its horizontal scroll, zoom and Snap. A divider above it drags to share height with the grid (`view.pianoRollLanesHeight`, default 2 lanes, 64 px each; more lanes scroll). Left column (the keyboard's 56 px): each lane's short name ("Velocity", "Pedal", "Bend", "Mod", "Expr", "Vol", "Press", "CC 74"), shrinking to fit, a click opening the lane menu; at the bottom a **+ Lane** button.
- **+ Lane** menu: Velocity, Sustain pedal (64), Pitch bend, Mod wheel (1), Expression (11), Volume (7), Pan (10), Channel pressure, *Other controller >* (0 to 119 with names from `midiguide::getCommonControllers`, others "Controller N"), then the guide. The shown lanes are remembered (`view.pianoRollLanes`); opening a clip also shows, for that clip, every lane it has data in.
- **Lane menu** (click the name or right-click the lane): Change to > (the same list), Thin out points, Select all in lane, Remove all points in lane, Hide lane, then *About <lane>...* opening its guide topic.
- **Drawing** (controller lanes): click adds a point; drag a point to move it (time snaps to the grid, Alt no snap); drag across empty space draws freehand; Shift+drag draws a straight line; in the piano roll's Select mode a drag draws a selection rectangle instead. Click selects, Shift-click adds; Delete removes; Cmd+C/V/D copy, paste at the playhead into the same kind of lane, duplicate after the selection; Cmd-drag copies.
- **Sustain lane:** values as filled steps, a press (64 and up) as a solid bar in the track colour, in-between values as lower bars. Drag across empty space draws a press (127 at the start, 0 at the end, both snapped).
- **Pitch bend lane:** centred, with lines at ±1 and ±2 semitones labelled "+1", "+2"; a dragged point catches the centre and the semitone lines within 4 px (Alt off). Before its first point a lane at rest draws its rest value dimmed.
- **Velocity lane:** a bar per note at its start (selected notes outlined). Drag a bar: that note, or every selected note if it is selected, changes by the same steps (as velocity mode, clamped 1 to 127). Drag across empty space: notes under the line take its height. Double-click a bar to type a number.
- **Hover:** "Bar 9 · beat 2 · Pitch bend · +0.5 semitone (10240)", "Bar 3 · Sustain pedal · down (127)", "Bar 2 · beat 1 · C4 · velocity 96".
- **Tooltips** (headers): see *Theory, tooltips*.

## Data and saving
- A MIDI clip gains **controller events**: kind (a controller number 0 to 119, pitch bend or channel pressure), beat from the clip start, and value in the message's own range (0 to 127; bend 0 to 16,383, 8,192 the middle). Kept sorted by beat and kind; one value per kind per beat (a later write replaces); events past the clip end are kept but not played, like notes.
- Saved in `ProjectFile.cpp` per clip as lanes (one per kind used, events as `[beat, value]` pairs), left out when empty. Version 4 only when a clip has events (Decision 6); version-3 and older projects load with none.
- Undo: the events join the clip's state in `undo::Snapshot`, shared when unchanged like `NoteList`; `undo::describe` names "Change pitch bend in 'Lead'".
- Tests: save and load, the version rule, an older project, undo round trip.

## The guide
New topics in *What is MIDI?* (`midiguide`, `MidiExplainer`), after *Pedals, wheels and knobs*:
1. **Pedals, wheels and knobs** (rewritten): the controller messages and how DawIt's lanes play them. Picture: as today.
2. **The sustain pedal**: controller 64, keys versus sound, changing it with each chord. Picture: the clip's sustain lane under its notes, each note's key length solid and its pedalled ring lighter up to the lift; the example (four chords) when the clip has no pedal. Click plays it.
3. **Pitch bend**: 14 bits, range, semitones and Hz. Picture: the clip's bend lane with a semitone scale, the bent pitch as a line over the note; the example (a guitar-style bend on A4) when it has none. Click plays it.
4. **Mod wheel, expression, volume**: what each is for and the dB curve. Picture: the clip's expression or volume lane with a dB scale (0, -6, -12, -20, -40); the example swell when it has none. Click plays it.
5. **Velocity** (existing): one sentence on the velocity lane.
6. **Your clip** (existing): a controller paragraph (*Theory, "your clip" rules*), and the byte counts include controller messages.

Opens from: each lane's menu (*About <lane>...*) at its topic, the + Lane menu's last item at *Pedals, wheels and knobs*. Theory Guides entry: summary adds "pedals and pitch bend"; search words add "pitch bend sustain pedal damper mod wheel modulation expression volume controller lane CC aftertouch pressure". Not on *Start here* (unchanged).

## Code
Recipes: *Song data that's saved* (clip data, with undo), *Topics in an existing guide*, *Checking in the real app*.
- **Controller events** live in `MidiClip` next to `notes`; pure helpers next to the type: tidy, value of a kind at a beat, the kinds used, thin out, points along a line, split at a beat. They are structural: written under `Engine::getLock()` through one engine setter per lane that calls `notesChanged()` (so playing notes are flushed and the lanes re-chased) and names the undo step.
- **Every place that copies or cuts a MIDI clip** carries them: `duplicateClip`, `splitClip` (the right part starts with each continuous lane's value at the cut, so it sounds the same), `joinClips`, copy and paste, Cmd-drag, the undo snapshot. Transforms (reverse, invert, randomise) leave them alone. Risky: grep every `MidiClip` copy.
- **Playback:** a pure function returns a clip's controller messages for a span of beats, with the start-of-span chase and the end-of-clip resets (Decision 4), tested without audio. `Track::collectMidiClipEvents` adds them; at the same sample, controllers go before Note Ons and resets after Note Offs. On a flush the track sends the resets for kinds it sent (audio-thread state on the track) and chases at the next block. MIDI effects get them (the arpeggiator already passes non-pedal events on). Bounces follow the same path.
- **Instruments:** a small shared controller state (pedal-held notes, bend factor `2^(semitones/12)`, smoothed gain `(cc7/127)² × (cc11/127)²`) used by the built-in synth (pedal is native to `juce::Synthesiser`; bend and gain in its voice) and the four DawIt synths. Bend moves sounding notes too.
- **Recording:** `Engine::captureInput` keeps CC, pitch bend and channel pressure; `handleRecordedEvent` puts them in the take; the pedal no longer lengthens notes (Decision 2).
- **Export:** `midiexport` writes the events (and the end-of-clip resets); `midiguide::getEvents` lists them, so the byte counts follow.

## Libraries and models
None. Licence check: no new library, model, asset or tool; JUCE's MIDI classes only. No `LICENSE` or `licenses/` change.

## Theory

### Facts
1. CC64 sustain: 0 to 63 up, 64 to 127 down. Exact. MIDI 1.0 (already in the guide).
2. Most pedals send only 0 and 127; some digital pianos send values between (half-pedalling); instruments without it treat 64+ as down. Typical. Wikipedia *Sustain pedal*; forum reports (SOURCES.md).
3. The pedal lifts all dampers; notes ring until they die away or it lifts; other strings resonate. Exact. Wikipedia *Sustain pedal*.
4. Lift as the new chord sounds, press just after with keys down (legato/syncopated pedalling); late changes blur. Typical. Yamaha *Piano pedagogy: pedaling*.
5. Pitch bend: 14 bits, 0 to 16,383, centre 8,192; low 7 bits first. Exact. MIDI 1.0; recordingblogs.
6. GM default bend range ±2 semitones (RPN 0). Typical. Wikipedia *Pitch wheel*; midi.org defaults thread.
7. semitones = (value − 8192) ÷ 8192 × range: 12,288 → +1, 4,096 → −1, 0 → −2, 16,383 → +1.99976. Exact, derived.
8. One step at range 2: 200 ÷ 8192 = 0.0244 cents; 7 bits: 400 ÷ 128 = 3.125 cents. Exact, derived.
9. ×2^(1/12) = 1.05946 a semitone; A4 → 466.16 Hz (+1), 493.88 Hz (+2), 415.30 Hz (−1). Exact, derived.
10. Guitar bends: mostly a semitone or a whole tone, sometimes a tone and a half. Typical. Guitar Player string-bending lessons.
11. CC1's job is up to the sound: vibrato on most synths, dynamics crossfade in many orchestral libraries. Typical. cmtext MIDI ch. 3; Orchestral Tools *Common MIDI CCs*.
12. GM: dB = 40 log10(cc7/127) + 40 log10(cc11/127), gain (cc7/127)² × (cc11/127)². Exact. GM2 spec and GM1 Developer Guidelines, quoted on midi.org.
13. 100 → −4.15 dB, 64 → −11.90, 40 → −20.07, 60 → −13.03, 84 → −7.18, 100 with 64 → −16.06, 0 → silent. Exact, derived.
14. GM modules usually start volume at 100 (some 90), expression 127. Typical. midi.org defaults thread; DTM wiki; Ardour.
15. Volume once to balance, expression for swells; libraries differ. Opinion. Orchestral Tools; forums.
16. Reset All Controllers: bend centre, mod 0, expression 127, sustain off; volume, pan, RPNs kept. Typical (Yamaha, Roland manuals).
17. CC 120 to 127 are channel mode messages. Exact. MIDI 1.0.
18. CC and Pitch Bend 3 bytes, Channel Pressure 2. Exact. MIDI 1.0.
19. Bend acts on every note of the channel. Exact. MIDI 1.0.
20. At 120 BPM a beat is 0.5 s. Exact, derived.

### Topic texts

**Pedals, wheels and knobs** · *Sustain, mod wheel, pitch bend*
> Besides notes, MIDI carries what the pedals, wheels and knobs do.
>
> Control Change messages (status B0 for channel 1) carry a controller number and a value from 0 to 127. Some numbers have a fixed meaning everywhere: 1 is the modulation wheel, 7 the volume, 10 the pan, 11 the expression, 64 the sustain pedal (64 or more is down). Others are free for a synth to use for its knobs. Numbers 120 to 127 aren't knobs at all: they are channel messages such as All notes off.
>
> Pitch Bend has its own message and twice the detail: 14 bits, 0 to 16,383, with 8,192 in the middle for no bend. How far the full bend goes (often 2 semitones) is set on the instrument.
>
> Program Change picks a sound (in General MIDI, 1 is the grand piano and 25 the nylon guitar). Aftertouch, or pressure, says how hard a key is pressed while it's held, for keyboards that can feel it, and System Exclusive lets a maker send anything, such as a whole synth patch.
>
> In DawIt a clip keeps its controllers in lanes under the piano roll: press + Lane to add one. Each point is one message, and its value holds until the next point, just as on a cable: a smooth curve is many small steps. When you stop or jump, DawIt centres the bend, lifts the pedal, sets the mod wheel and pressure to 0 and the expression to 127, then sends what the lanes say at the new place. So a part sounds the same wherever you start it, and no note is left bent or held. Volume, pan and the other knobs keep the last value they got, as they do on a synth.

**The sustain pedal** · *Controller 64: notes ring on after the keys come up*
> On a piano, the right-hand pedal lifts every damper off the strings. A note keeps ringing after you let go of its key, until it dies away or the pedal comes up, and the other strings ring along quietly, which makes the sound fuller.
>
> On a keyboard the pedal is sent as controller 64: 0 to 63 is up, 64 to 127 is down. Most pedals are switches and send only 0 and 127. Some digital pianos send the values in between for half-pedalling, and some piano instruments use them; others treat anything from 64 up as down.
>
> The pedal doesn't make the notes longer in the MIDI: the keys still come up when they do, and it's the instrument that keeps them sounding. So a pedalled note looks short in the piano roll, and the Sustain pedal lane under it shows the press as a bar.
>
> Pianists lift the pedal and press it again with each new chord. Lift it as the new chord sounds and press it again just after, while the keys are still down: the old chord stops, the new one is caught, and there's no gap. This is called legato or syncopated pedalling. Hold the pedal through a chord change and both chords ring together, which blurs them.
>
> The picture shows four chords, C, Am, F and G, each key held for one beat. The pedal goes down a quarter beat after each chord and comes up as the next one starts, so each chord rings for the whole bar: 2 seconds at 120 BPM, though the keys were down for half a second.
>
> The instrument decides what the pedal does. DawIt's instruments hold notes while it's down; drum sounds ring out as they would anyway, and some synth sounds ignore it.

**Pitch bend** · *Slide the pitch: 8,192 is the middle*
> Pitch bend slides the pitch up or down while a note plays, like a guitarist pushing a string or a singer sliding into a note. On a keyboard it's the wheel or lever left of the keys, and it springs back to the middle when you let go.
>
> It has its own message, with 14 bits for its value: 0 to 16,383, with 8,192 in the middle for no bend. The extra detail is for smooth slides: with 7 bits, a bend of 2 semitones each way would move in steps of about 3 cents (a cent is a hundredth of a semitone); with 14 bits a step is 0.02 cents.
>
> The message doesn't say how many semitones it means. The instrument decides, with its bend range. General MIDI instruments start at 2 semitones each way, and so do DawIt's. Then 12,288 is 1 semitone up, 4,096 is 1 down and 0 is 2 down; the top, 16,383, is a hair under 2 up. Many synths let you set 12 or 24 for big dives. If a plugin uses another range, the same lane bends further or less, so check the range in the plugin.
>
> A semitone up makes the frequency 1.0595 times higher. Bend A4 (440 Hz) up 1 semitone and it plays 466.2 Hz, the B flat; up 2 and it plays 493.9 Hz, the B.
>
> A bend moves every note sounding on the channel, so bending a chord bends all of it. For a guitar-style bend, bend one note on its own. Guitarists most often bend by a semitone (one fret) or a whole tone (two frets), sometimes a tone and a half.
>
> The picture shows a guitar-style bend on A4: the note starts, rises a whole tone to B4 in half a beat, holds, and comes back down before it ends. The scale on the left is in semitones at a range of 2.

**Mod wheel, expression, volume** · *Vibrato, swells and the part's level*
> Three controllers shape a sound while it plays.
>
> The mod wheel (controller 1) sits next to the pitch wheel but stays where you leave it. What it does is up to the sound: on most synths it adds vibrato, and on many orchestral sample libraries it fades between soft and loud recordings. DawIt's own instruments don't use it yet; a plugin's manual says what it does there.
>
> Volume (controller 7) is the part's level, and expression (controller 11) a second level inside it, for swells. A common way to work: set the volume once to balance the part, then draw crescendos and fades with expression. Some sample libraries use controllers 1, 7 and 11 differently, so check the manual.
>
> In General MIDI both follow the same curve: each lowers the level by 40 × log10(127 ÷ value) dB, and the two add up. At 127 nothing changes; at 100 the part is 4.2 dB quieter; at 64, 11.9 dB; at 0 it's silent. Volume 100 with expression 64 is 16.1 dB down. Sound modules usually start a part at volume 100; DawIt's own instruments start at 127, so a volume lane can only turn them down.
>
> Unlike the mixer's fader, these travel with the notes: a swell drawn in the clip moves when you move the clip, and goes into an exported MIDI file.
>
> The picture shows a string chord held for two bars, with expression rising from 40 (20 dB down) to 127 over the first bar and falling to 60 (13 dB down) over the second.

**Velocity** (existing), replace its last sentence with:
> The picture shows your clip's notes by their velocity: taller and brighter is harder. In the piano roll, the Velocity lane under the notes shows the same bars: drag one up or down to change it.

### Tooltips
- **+ Lane:** "Add a lane under the notes: velocity, sustain pedal, pitch bend, mod wheel, expression, volume or any controller."
- **Velocity:** "Velocity: how hard each note is played, 1 to 127. Drag a bar up or down; drag across empty space to draw a line over several notes."
- **Sustain pedal:** "Sustain pedal (controller 64): while it's down, notes ring on after their keys come up. Drag across the lane to draw a press."
- **Pitch bend:** "Pitch bend: slides the pitch, 2 semitones up or down on DawIt's instruments (a plugin may use another range). Click to add a point, drag to draw."
- **Mod wheel:** "Mod wheel (controller 1): often vibrato, but it's up to the instrument. DawIt's own instruments don't use it yet."
- **Expression:** "Expression (controller 11): a level inside the volume, for swells and fades within a phrase. 127 is full, 64 is 12 dB down."
- **Volume:** "Volume (controller 7): the part's level, 0 to 127. Set it once to balance the part; draw swells with expression."
- **Pan:** "Pan (controller 10): left (0), middle (64) or right (127), if the instrument uses it."
- **Channel pressure:** "Channel pressure (aftertouch): how hard keys are pressed while held, 0 to 127. Only some keyboards send it, and DawIt's own instruments don't use it yet."
- **Other controller N:** "Controller N: what it does depends on the instrument; its manual lists the numbers it uses."
- **Thin out points:** "Removes points that lie on a straight line between their neighbours, so a recorded wheel takes less room. The sound stays the same within 1 step (32 for pitch bend)."

### Example data
- **Sustain:** chords C (60 64 67) at beat 0, Am (57 60 64) at 4, F (53 57 60) at 8, G (55 59 62) at 12; keys 1 beat, velocity 100. Pedal 127 at 0.25, 4.25, 8.25, 12.25; 0 at 4, 8, 12, 16. Each chord sounds 4 beats = 2.0 s at 120 BPM; keys 0.5 s.
- **Bend:** A4 (69) beats 0 to 2, velocity 100. Bend rises linearly 8,192 → 16,383 from 0.25 to 0.75 (a point every 1/32 beat, 17 points; 12,288 at 0.5 = +1 semitone), holds to 1.5, falls to 8,192 by 1.75 (9 points).
- **Swell:** C4 E4 G4 (60 64 67) beats 0 to 8, velocity 90. Expression 40 → 127 from 0 to 4 and 127 → 60 from 4 to 8, a point every 1/8 beat; at beat 2 it is 84 (−7.18 dB).

### "Your clip" rules (a *Controllers* paragraph, after velocity)
- **None:** clip has notes and no events: "No pedal, bends or controllers in this clip. Press + Lane under the piano roll to draw some." Quiet without notes.
- **Pedal summary:** "The sustain pedal goes down N times, for X% of the clip."
- **Pedal across a chord change** (the song has chords and a press covers a chord-track change more than 1/4 beat after it went down): "The pedal is held through the change from C to F at bar 5, so both chords ring together. Try lifting it as the F starts and pressing again just after." First case only. Without chords, a press of 16 beats or more: "The pedal stays down for N bars from bar B, so the notes pile up. Pianists usually lift it and press again at each chord change."
- **Pedal down at the clip end:** "The pedal is still down at the end of the clip. DawIt lifts it there, so notes stop at the clip's end."
- **Bend summary:** "Pitch bend goes from −1 to +2 semitones (at the usual range of 2)."; not centred at the end: "The bend ends at +1.5 semitones. DawIt centres it at the clip's end; add a point at 0 where the bend should finish."
- **Silent:** volume or expression at 0 while a note plays: "Expression is at 0 from bar 5 while notes play, so they are silent there."
- **Ignored:** for each lane used that the instrument ignores (from what the track plays: built-in synth and DawIt synths use pedal, bend, volume, expression; DawIt Drums none; a plugin unknown): "DawIt Subtractive doesn't use the mod wheel, so the Mod wheel lane changes nothing here; a plugin instrument may." For a plugin: "What <plugin> does with controllers is up to it; its manual lists the ones it uses."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| semitones of 12,288 / 4,096 / 0 / 8,192 at range 2 | +1 / −1 / −2 / 0 | exact |
| semitones of 16,383 | +1.99976 | 1e-5 |
| gain dB for 100 / 64 / 40 | −4.15 / −11.90 / −20.07 | 0.01 |
| 100 and 64 together | −16.06 dB | 0.01 |
| 0 | gain 0 | exact |
| built-in synth and each DawIt synth, A4 with bend 12,288 / 4,096 | 466.16 / 415.30 Hz | 5 cents |
| expression 64 on a held note, steady level | −11.9 dB | 0.5 dB |
| pedal down 0.5 to 3, key up at 1: level beat 1.5–2.5 vs no pedal | ≥ 20 dB more | |
| thin a 33-point straight ramp | 2 points | exact |
| beat 0.25 in an exported file | tick 240 | exact |
| Your clip byte count: 2 notes, 2 CC, 3 bends, 1 pressure | 4×3 + 2×3 + 3×3 + 2 = 29 without running status | exact |

## Steps
Group A1, engine: the data and playback (`./build.sh --test` at step 5):
- [ ] 1. Controller events in `MidiClip` with the pure helpers (*Code*). Tests: tidy, value at a beat, thin, line points, split. Check: tests pass.
- [ ] 2. Engine setter per lane (lock, `notesChanged`, step name); carry events through duplicate, split (chased value at the cut), join, copy and paste, Cmd-drag; undo snapshot, `matches`, `equal`, `describe`, restore. Tests: each clip edit keeps them; undo round trip.
- [ ] 3. Saving: lanes per clip, version 4 only with events. Tests: round trip, version rule, an older project loads with none.
- [ ] 4. Playback: the span function (chase, end-of-clip resets), `collectMidiClipEvents`, flush resets per track. Tests: chase at beat 2 sends bend 12,288 before the note; resets at clip end; ordering at one sample; a notes-only bounce capture holds them.
- [ ] 5. MIDI export and `midiguide::getEvents`; recording keeps CC, bend and pressure and stops lengthening pedalled notes (update the old pedal test). Tests: export ticks, byte counts, a recorded take's events. `./build.sh --test` passes without warnings.

Group A2, engine: instruments and the guide's content (`./build.sh --test` at step 8):
- [ ] 6. The shared controller state; the built-in synth's bend and volume and expression. Tests: the sound measurements in the table.
- [ ] 7. DawIt Subtractive, FM, Additive and Physical: pedal, bend (sounding notes too), volume and expression. Tests: the same measurements per instrument; DawIt Drums unchanged.
- [ ] 8. `midiguide`: the three topics, the rewritten and the velocity texts, the example data, the controller paragraph of *Your clip* (pass the song's chords and what the track's instrument uses), Theory Guides summary and keywords. Tests: text and example per topic, the advice on made-up clips, search words. Add the sources to SOURCES.md if any changed. `./build.sh --test` passes.

Group B, UI (`./build.sh --test` at step 12):
- [ ] 9. The lane area in `PianoRoll` (`src/ui/ControllerLane.{h,cpp}`): layout, divider, shared scroll and zoom, + Lane and lane menus, settings, drawing steps, bend scale, pedal bars, hover text, tooltips. Check: open a MIDI clip, add the Bend and Pedal lanes, scroll and zoom: the lanes follow the grid.
- [ ] 10. Editing controller lanes: add, move, freehand, Shift line, Select-mode rectangle, Delete, Cmd+C/V/D, Cmd-drag, snapping and catching, Thin out, Remove all, Hide.
- [ ] 11. The velocity lane: bars, relative drag of the selection, line across empty space, double-click number.
- [ ] 12. `MidiExplainer` pictures for the three topics (the clip's lanes or the example, a click plays it), *About <lane>...* opening the topic, PDF export. `./build.sh --test` passes; `--export-guides` passes.

Group C, check and docs:
- [ ] 13. Real-app check: add a `ScreenshotRunner` key `"controllerLanes": "<track>"` that writes the bend and sustain examples into the track's first MIDI clip and shows the Velocity, Pedal and Bend lanes. Shots: the piano roll with those lanes (`capture: bottomPanel`); the MIDI guide at *The sustain pedal*, *Pitch bend*, *Mod wheel, expression, volume* and *Your clip* with that clip. Look for: steps and scale labels readable, pedal bars under the notes, text not cut, numbers as in *Theory*.
- [ ] 14. Docs: README *Features* bullet (lanes, recording, export); docs/tests.md; docs/code-layout.md (`ControllerLane`); architecture.md *The edit* (clips hold controller events; the chase and reset rule in *How a change travels*); ui-design.md (the lane area under the piano roll); recipes.md *Song data* (clip-level data copied by every clip edit); issue #56 Done and Still open.

## Tests
The numbers are in *Theory, test numbers*; each step names its checks. Besides those: a setter sends no change message when nothing changed; a version-3 file loads with no events; a fake-device take with the pedal down, a key released under it and a bend keeps the key's own length and the events; *Your clip* advice on made-up clips (a press across a chord change, a bend ending at +1.5, expression 0 under a note, mod wheel on DawIt Subtractive); search for "pitch bend", "sustain pedal", "mod wheel" and "expression" finds *What is MIDI?*.

## Website hand-off
- `features.html`, the piano roll section: a short paragraph on lanes (velocity, pedal, bend, mod wheel, expression, volume, any controller), kept when recording and exported; one screenshot of the piano roll with the Velocity, Pedal and Bend lanes (step 13's shot).
- `learn.html`: *What is MIDI?* now covers the sustain pedal, pitch bend and expression; optionally the *Pitch bend* topic's picture.
- No How it's made change.

## Left for later
- The MIDI editor window (Cmd+E, *Open in MIDI editor...*), hosting this piano roll and lanes.
- Live wheels: on-screen wheels, mod wheel vibrato and channel pressure on DawIt's instruments, a bend range per instrument (and sending RPN 0 to plugins).
- Line, ramp, sine and random shapes; starting points (pedal per chord, bend into a note, string swell, vibrato).
- Loudness words for velocity (pp to ff), release velocity, per-note bend for MPE.
- 14-bit controllers (CC 1 with 33) shown as one lane; RPN and NRPN lanes; poly aftertouch.
- MIDI file import keeping the events (#49); controllers in *Create MIDI clip from arpeggiator*; a mark in the arrangement for clips with controllers.

## Changes while building
