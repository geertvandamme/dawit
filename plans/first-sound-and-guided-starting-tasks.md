# Learn composing and recording through guided steps in DawIt

Status: Released
Idea: [#159 — Learn composing and recording through guided steps in DawIt](https://github.com/geertvandamme/dawit/issues/159)
Scope: usability roadmap package 06, *First sound and guided starting tasks* (audit findings 1–5, 43 and 45), a bounded child of [usability-improvements.md](usability-improvements.md) (#173). The rest of #159 (the Help button and full composing and recording journeys) stays open; see *Left for later*.

## Goal

An empty song stops being a blank screen: five large choices (Start from a style, Add an instrument or drums, Record voice or instrument, Import audio, Learn) each start a real task, explain MIDI and audio at the moment of choosing, and point at the next control to use. A short, optional first-song exercise (hear a note, change a chord, add a rhythm, save) teaches by doing in the user's own song, with live ticks, without asking anyone to read the 76-page path first. The guide library's cards get lighter: a few topics each, with every search match still shown.

## Decisions for you

1. **Scope.** Options: this package only; or also the Help button and full journeys of #159. **Recommended:** this package only. It is the roadmap's bounded slice; the Help button and recording journey build on it (its pointer and step model) and on #178 and #179.
   Answer: the recommendation (user, 2026-11-14).
2. **Where the exercise lives.** Options: a small separate window; inside the Theory Guides window; a docked panel. **Recommended:** a small window (360 × 500), because the user acts in the main window while reading it and can move it off the control being taught. The guides window and the inspector open it.
   Answer: the recommendation (user, 2026-11-14).
3. **When a step counts as done.** Options: live from the current song (a tick can go away again after Undo or in another song); or remembered once reached. **Recommended:** live, so a tick never claims something the song doesn't have. Only "finished the exercise once" and the current step are remembered.
   Answer: the recommendation (user, 2026-11-14).
4. **Buttons that write music** (*Put in four chords*, *Write a basic beat*). Options: offer them as explicit buttons; or teach only. **Recommended:** offer them, one undo step each, beside the instructions for doing it by hand: a beginner who gets stuck still reaches the next step. They write only into an empty chord track or an empty drum clip and never change an instrument.
   Answer: the recommendation (user, 2026-11-14).
5. **The inspector's welcome state.** Options: only while the song has no tracks; whenever nothing is selected. **Recommended:** only with no tracks, with a *Show the master* button; songs with tracks keep today's behaviour, so experienced users lose nothing.
   Answer: the recommendation (user, 2026-11-14).
6. **Topic chips on a card.** Options: the first 6 in reading order plus *+N more*; as many as fit one line. **Recommended:** the first 6, plus every search match beyond them, plus *+N more*: the count is predictable and testable, and a match is never hidden.
   Answer: the recommendation (user, 2026-11-14).
7. **Computer keyboard.** Options: turn it on whenever an instrument is added; only when asked. **Recommended:** only when asked: the exercise's *Add a piano and play it* says so on its button; the start tile's pointer suggests Cmd+K. Typing mode takes the letter keys, which would surprise someone who didn't ask.
   Answer: the recommendation (user, 2026-11-14).
8. **Opening the exercise by itself on first launch.** **Recommended:** no. The Learn tile, the inspector's welcome and Help > First Song Exercise... offer it.
   Answer: the recommendation (user, 2026-11-14).

## What the user does and sees

**Start panel (empty song).** While the song has no tracks, the lane area below the song rows (spanning the track-header column and the lanes) shows the heading "Start your song", a dim line under it and five tiles in one row, then the existing line about dropping audio files. Tiles are real buttons (keyboard focus, Tab order left to right, Return or Space, accessible title and description from the tooltip). At least 120 px each; below 640 px they wrap into rows of 3 and 2, and below 120 px a tile drops its subtitle (kept in the tooltip). Text shrinks to fit before cutting. Check at 960 × 600 with the inspector open.

| Tile | Does |
|---|---|
| Start from a style | `newFromStyleCommand` (the window asks to save first, as today) |
| Add an instrument or drums | a menu: a section header, the 8 built-in sounds, then *What is MIDI?* last (`guideicon::addItem`, topic *What MIDI is*). Adds a MIDI track named after the sound with an empty 4-bar clip (`4 × getBarLength()` beats), selects and reveals it, shows the clip editor, then the pointer on the piano-roll grid |
| Record voice or instrument | adds an audio track "Recording", selects and reveals it, pointer on its R button; with no input channels on the device, the caption says so and offers *Audio/MIDI Settings...*. Never arms or records |
| Import audio | `importAudioCommand` |
| Learn | Theory Guides at *Start here* (`theoryGuidesCommand`, `showRow (0)`) |

**The pointer.** A click-through overlay on `MainComponent`: a 2 px `accent` outline around a target component and a caption bubble (`panelLight`, 13 pt, at most 300 px wide) beside it, placed on the side with room. The bubble alone takes clicks (its optional action button, ×). It goes on the next mouse-down outside the bubble, on Esc, after 20 s, or when the target goes away. One at a time. Used by the start tiles and the exercise's *Show me*.

**The inspector's welcome.** With no tracks and nothing selected the inspector shows, instead of Master: "Get started", a short text, *First song exercise...* (with "2 of 4 done" under it once started) and *Show the master*. *Show the master* shows today's master view for the rest of this empty song; adding a track or selecting one ends the welcome.

**The piano roll with no clip.** Under the existing text, when the song has no MIDI track, an *Add an instrument...* button opens the same menu as the tile (finding 3).

**The exercise window** ("First song exercise", `PluginWindowManager::show`, fixed 360 × 500, first placed at the main window's top right). Opens from Help > *First Song Exercise...* (new command, no shortcut), the inspector welcome, the strip in Theory Guides and the Learn path. Sketch:

```
First song exercise                                     [guide ?]
Four short steps in your own song ...
 ✓ 1 Hear a note
 ● 2 Change a chord                    (current step, open)
     what to do · why
     [Put in four chords]  [Show me]  [Chords guide · Your chord track]
 ○ 3 Add a rhythm
 ○ 4 Save your song
 [Back]            2 of 4 done            [Skip] [Next]
```

- Click a step row to open it; Back, Next and Skip move between steps (Skip and Next are the same move; Next is the accent button once the step is done). Esc closes. All buttons focusable with tooltips.
- Ticks follow the song live (Decision 3), checked on engine changes and on a 2 Hz timer for the saved state. A step being done never moves the user on by itself.
- Each step's action button changes with the song (the texts are in *Theory*): step 1 *Add a piano and play it* or *Open <track> and play it*; step 2 *Put in four chords* (only with an empty chord track) else *Open the Chords row*; step 3 *Add drums*, then *Write a basic beat* (only in 4/4 with an empty drum clip selected; otherwise disabled with the reason in its tooltip); step 4 *Save...* (the Save command, which asks for a name the first time).
- *Show me* points at the step's control: the piano-roll grid (else the transport's keyboard button); the Chords row; the drum track's lane (else the piano-roll grid); step 4 has none.
- All four done: the list is replaced by the completion text with *Carry on with Start here* and *Start over* (back to step 1; the song is untouched).

**Theory Guides.** On *Start here*, above the first card, a strip: one line of text and *Start exercise*, *Continue exercise (2 of 4)* or "Done" with *Do it again*. Path progress and Continue are unchanged. Cards show at most 6 topic chips, plus any matched topic beyond those, then a *+N more* chip; clicking it shows them all and a *Show fewer* chip. Expansion lasts while the window is open.

**Remembered** in app settings: `learn.firstSong.step` (0 to 3) and `learn.firstSong.done` (true once all four were ticked at the same time). Nothing else.

## Data and saving

No project data and no format change. The exercise's step and done flag are app settings; a missing or out-of-range value reads as step 0 and not done. Tracks, clips and chords the actions add are ordinary edit data, saved and undone as usual. The chord step's baseline (the chords when the step was opened, or the example just written) lives only in memory.

## The guide

No new guide or topics. The exercise and pointers link to existing pages: MIDI · *What MIDI is* and *The computer keyboard*; Chord progressions · *Your chord track*; Note lengths, beats and bars · *Bars and beats*; How recording works · *Choosing an input*. The step definitions name a `gen::guides::Guide` and a topic name; a test checks each exists in `getEntries()`. No `gen::guides` entry, no `TheoryGuideLinks.cpp` change, no `SOURCES.md` change (every fact reuses an existing guide's source).

## Code

Recipes: *A command or menu item* (First Song Exercise...), *A window* (the exercise), *Checking in the real app*. No song-data recipe.

- **`gen::firstsong`** (`src/generate/FirstSong.{h,cpp}`, UI-free): the four steps with their texts and guide links; a pure check per step on the `Engine` plus a "saved" flag and a chord baseline passed in; "is a drum track" (the existing convention `usesBuiltInSynth() && builtInSounds::isPercussive`, or DawIt Drums as the instrument, which style songs use); the example chords (`chordguide::getCommonProgressions (key)[0]` over bars 1 to 4); the basic beat (pure notes for N bars of 4/4); and the two writers, which go through `Engine` under its lock and name their undo step with `nameNextStep`.
- **Done rules** (exact): step 1, a non-drum MIDI track has a note; step 2, the chord track has a chord and differs from the baseline; step 3, a drum track has at least 4 notes; step 4, the project has a file and no unsaved changes (`ProjectManager` exposes this; today `hasUnsavedChanges` is private).
- **Invariants:** nothing here arms, records, monitors or replaces a song without the existing save question; writers only fill an empty chord track or an empty clip on a drum track; no instrument is ever changed.
- **`gen::guides::getShownTopics`** (pure): which chip indexes a card shows, given the topic count, the matched indexes, expanded or not, and the limit 6.
- **UI:** the start panel is a child of `ArrangementView` shown only with no tracks (rebuilt on edit changes); the pointer and the exercise window belong to `MainComponent`, which also wires the tile actions to existing commands and `addTrack`. `InspectorPanel` gets the welcome state; `TheoryGuidesView` the strip and the card collapse. All message thread; no audio-thread or format change. Risk is low; the widest change is `InspectorPanel`'s null-selection meaning.

## Libraries and models

None.

## Theory

No new guide content. Every fact the new texts state already appears in an existing guide with its source in `SOURCES.md`; no new searches or pages were needed.

### Facts

1. MIDI carries instructions (which note, how hard, how long), not sound; the track's instrument turns them into sound. Exact (definition). MIDI 1.0; MIDI guide *What MIDI is*.
2. An audio clip is the recorded sound itself; its notes can't be edited like MIDI notes. Exact (definition). Same topic.
3. Note 60 is middle C; DawIt calls it C4 (some makers say C3). Exact (convention, stated with its caveat in MIDI · *Note numbers and pitch*).
4. With the computer keyboard on, A plays middle C at first; Z and X move an octave; T plays the F♯ above. Exact (DawIt behaviour; MIDI · *The computer keyboard*).
5. General MIDI Level 1 percussion: 36 Bass Drum 1 (kick), 38 Acoustic Snare, 42 Closed Hi-Hat. Exact. GM1 key map (`SOURCES.md`, Drum triggers heading); Instruments guide.
6. With C4 = 60: 36 = C2, 38 = D2, 42 = F♯2. Derived: 60 − 24 = 36 is two octaves below C4; 36 + 2 = D; 36 + 6 = F♯.
7. The backbeat is the snare on beats 2 and 4, the base of most rock and pop. Typical (Styles guide; Rhythm guide example).
8. A basic rock beat: kick on 1 and 3, snare on 2 and 4, hi-hat on every eighth note. Typical; drummers vary it endlessly (Rhythm guide's backbeat example).
9. I–V–vi–IV ("Pop progression") is one of the most used progressions in pop. Typical (Chord guide, *Well-known progressions*).
10. In C major I–V–vi–IV is C, G, Am, F. Derived (degrees 1, 5, 6, 4 of C major).
11. In a minor key the first common progression is "Epic minor", i–VI–III–VII: in A minor Am, F, C, G. Derived (Chord guide's list).
12. The Chords row makes no sound by itself; the chord diagrams in the opened row play when clicked. Exact (DawIt behaviour, `ChordLane::playBlock`).
13. Clicking an empty spot of the piano-roll grid writes a note and plays it. Exact (DawIt behaviour, not in Select mode).
14. A `.dawit` project keeps the tracks, notes, chords and instrument and effect settings; audio is copied into the project's audio folder. Exact (`ProjectFile.cpp`).
15. New Project from Style offers 14 styles. Exact (`gen::templates::getStyles()`, tested).
16. One bar of the basic beat is 2 kicks + 2 snares + 8 hi-hats = 12 notes; 4 bars are 48. Derived.

### Texts

**Start panel.** Heading: "Start your song". Line: "Pick one way to begin. You can add the others at any time."

Tiles (title · subtitle · tooltip):
- **Start from a style** · "A whole song sketch in one of 14 styles" · "Opens New Project from Style (Cmd+Shift+N). Choose a style, key and tempo, and DawIt builds the sections, chords and tracks of a song, with notes to start from if you want them. It replaces this empty song and asks to save first if you changed anything."
- **Add an instrument or drums** · "MIDI: notes you can change, played by a sound you choose" · "Adds a MIDI track. MIDI isn't sound: it's the notes to play, and the track's instrument turns them into sound. So you can change any note, or the whole instrument, later. Choose a sound from the list."
- **Record voice or instrument** · "Audio: the sound itself, from a microphone or an instrument" · "Adds an audio track for a recording. Audio is the sound itself: you can cut it, move it and add effects, but not change its notes the way you can with MIDI. Nothing records until you press Record."
- **Import audio** · "Bring in a recording or a loop, such as a WAV file" · "Choose audio files to put on a new audio track at the playhead (Cmd+I). You can also drop files here."
- **Learn** · "The guides, and a five-minute first exercise" · "Opens Theory Guides at Start here (Cmd+G). The first thing there is a short exercise in your own song: hear a note, change a chord, add a rhythm and save."

Instrument menu header: "A MIDI track: notes played by this sound". Item tooltips are not shown in menus; the header carries the point.

**Pointer captions.**
- After an instrument: "Your <Piano> track has an empty clip, open here. Click in the grid to write a note: you hear it as you add it. Or press Cmd+K to play it with your letter keys: A is middle C." (Drums or Percussion: "Your Drums track has an empty clip, open here. On a drum track each note is a drum: C2 (note 36) is the kick, D2 (38) the snare. Click in the grid to add one.")
- After Record: "Your Recording track is ready. Press R to arm it: the chip after R shows which input it records, such as In 1. Then press the red Record button, and Stop when you're done."
- No input: "No audio input was found on this audio device. Connect a microphone or an audio interface, then choose it in Audio/MIDI Settings." Button: *Audio/MIDI Settings...*

**Inspector welcome.** Heading "Get started". Text: "The inspector shows the selected track: its sound, effects, sends, pan and volume. Your song has no tracks yet. Pick a way to start in the middle, or try the first-song exercise." Buttons: *First song exercise...* (tooltip "Four short steps in your own song: hear a note, change a chord, add a rhythm and save"); *Show the master* (tooltip "The master is the sum of every track. Its volume sets how loud the whole song plays.").

**Piano roll button.** *Add an instrument...*, tooltip "Adds a MIDI track with an empty clip to write notes into. Choose its sound from the list."

**Theory Guides strip.** "New here? Try the first-song exercise: hear a note, change a chord, add a rhythm and save, in your own song. It takes about five minutes, and you don't need to read anything first." Buttons as above; tooltip "Opens the exercise window. It works in the song that's open now and adds nothing until you press a button." Chips: *+N more* (tooltip "Show all <n> topics of <guide>"), *Show fewer*.

**Exercise.** Title "First song exercise". Intro: "Four short steps in your own song, about five minutes in all. Each one says what to do and why. Do it in the main window: the tick appears by itself."

1. **Hear a note** — "Play middle C, then write a note"
   - Do: "Press Add a piano and play it. DawIt adds a Piano track with an empty clip, opens it in the piano roll at the bottom and turns on the computer keyboard. Press A: you hear middle C (C4, note 60). Then click anywhere in the piano roll's grid to write a note. You hear it as you add it, and it stays in the song. Press Space to play it back."
   - Why: "A MIDI note is an instruction, not a sound: which key, how hard and how long. Middle C is note 60. The track's instrument turns the instruction into sound, which is why you can change the note, or the whole instrument, at any time."
   - With a non-drum MIDI track already there, the button is *Open <name> and play it* and the first sentence reads "Press Open <name> and play it: DawIt opens its first clip in the piano roll and turns on the computer keyboard."
   - Button tooltip: "Adds a Piano track (or opens your MIDI track), opens its clip and turns on Play with Computer Keyboard (Cmd+K turns it off again)."
2. **Change a chord** — "Pick another chord for one bar"
   - Do: "The Chords row above the tracks holds one chord per bar. If it's empty, press Put in four chords: DawIt writes the <Pop progression> in your key, <C, G, Am and F> in <C major>, in bars 1 to 4. Click a bar on the Chords row and choose another chord, for example <Dm> in place of <F>. Open the row with its arrow to see each chord as piano keys: click a chord to hear it."
   - Why: "The Chords row is the song's plan. It makes no sound by itself, but it shows what each chord is, guides the generator and fills Play along. Changing one chord changes the feeling of the whole bar."
   - The angle-bracket parts come from the song's key and `getCommonProgressions (key)[0]` (in a minor key, Epic minor: "Am, F, C and G in A minor", example "Dm in place of G"). The suggested swap replaces the 4th chord with the key's degree-2 triad in major (F → Dm in C) or degree-4 triad in minor (G → Dm in A minor).
   - Put in four chords tooltip: "Writes four chords from the key into bars 1 to 4 of the empty Chords row. One undo step."
3. **Add a rhythm** — "A beat of kick, snare and hi-hat"
   - Do: "Press Add drums: DawIt adds a Drums track with an empty four-bar clip and opens it in the piano roll. On a drum track each note is a drum, not a pitch: C2 (note 36) is the kick, D2 (38) the snare and F♯2 (42) the closed hi-hat. Write the kick on beats 1 and 3, the snare on 2 and 4 and a hi-hat on every eighth note. Or press Write a basic beat and change what it writes. Press Space to hear it."
   - Why: "This is the backbeat that most pop and rock rests on: the snare on beats 2 and 4 gives the bar its push, the kick marks beats 1 and 3, and the hi-hat keeps time in between. Drummers change it endlessly, but it's a good first beat."
   - Add drums tooltip: "Adds a MIDI track with the built-in Drums sound and an empty four-bar clip, open in the piano roll." Write a basic beat tooltip: "Writes kick, snare and hi-hat into the empty drum clip: one bar, repeated. One undo step." Disabled: "Only for an empty drum clip in 4/4" (or "Select the drum clip first").
4. **Save your song** — "Keep it as a project file"
   - Do: "Press Save... (Cmd+S) and choose a name and a folder. DawIt writes a .dawit project with everything in the song: tracks, notes, chords, sounds and effects. Audio you record or import is copied into the project's audio folder. Open it again with File > Open Recent."
   - Why: "Saving often means a crash or a closed window costs you little. A project keeps every part separate, so you can change any of them later."
   - Done but changed afterwards: the tick goes, and the line reads "Changed since you saved: press Save again."

Completion: "Done. You heard a note, changed a chord, added a rhythm and saved your song. Next, try Start from a style for a whole song sketch, or carry on with Start here in Theory Guides." Buttons: *Carry on with Start here*, *Start over* (tooltip "Back to step 1. Your song stays as it is.").

### Example data

- Basic beat, one 4/4 bar (beats from the clip start, velocity): kick 36 at 0 and 2 (0.9); snare 38 at 1 and 3 (0.85); hi-hat 42 at 0, 0.5 … 3.5 (0.7 on the beat, 0.55 off it); each note 0.25 beats long. Repeated for each bar of the clip.
- Example chords: C major → C G Am F, swap F → Dm; A minor → Am F C G, swap G → Dm; G major → G D Em C, swap C → Am.

### "Your song" rules

None (no guide topic). The step checks are under *Code*.

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| Basic beat, 1 bar | 12 notes: 2 × 36, 2 × 38, 8 × 42 | exact |
| Basic beat, 4 bars (16 beats) | 48 notes, last hi-hat at 15.5 | 1e-9 beats |
| Kick starts | 0, 2 (bar 1) | 1e-9 |
| Example chords C major | C, G, Am, F at bars 0–3 | exact |
| Example chords A minor | Am, F, C, G | exact |
| Swap suggestion C major / A minor / G major | F → Dm / G → Dm / C → Am | exact |
| Instrument clip length in 4/4 | 16 beats | 1e-9 |
| Shown chips, 17 topics, none matched, collapsed | 0–5, hidden 11 | exact |
| Shown chips, 17 topics, match 12, collapsed | 0–5 and 12, hidden 10 | exact |
| Shown chips, 4 topics | all 4, hidden 0 | exact |
| Style count in the tile text | 14 = `getStyles().size()` | exact |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `gen::firstsong` content and checks: step list, texts (from *Theory*), guide links, drum-track rule, done rules, settings reading (clamped). Tests: each done rule true and false on made-up edits, including Undo-like removal; each link resolves in `getEntries()`; every text non-empty. Done when the tests pass.
- [x] 2. The example chords, the swap suggestion and the basic beat as pure functions, then the two writers through `Engine` (empty chord track only; empty drum clip only, 4/4 only; named undo step). Tests: the *Test numbers* rows; a writer refuses a filled target and leaves the edit unchanged; the drum track's sound is the same after writing. Done when they pass.
- [x] 3. `gen::guides::getShownTopics` with its tests (table rows). Done when `./build.sh --test` passes without warnings.

Group B1, UI: start panel, pointer, inspector (`./build.sh --test` at step 6):
- [x] 4. The pointer overlay on `MainComponent` (outline, bubble with optional button, dismiss rules), and the instrument menu plus the tile actions in `MainComponent` (reuse `addTrack`, commands). Check: by hand, the pointer on the piano roll after adding a Piano.
- [x] 5. The start panel in `ArrangementView` (layout and wrapping rules, keyboard focus, tooltips, accessible titles), replacing the empty-song line of `TrackLanes::paint`; the piano roll's *Add an instrument...*. Check: 960 × 600 and 1440 × 900.
- [x] 6. `InspectorPanel`'s welcome state and *Show the master*; expose "has a file and no unsaved changes" on `ProjectManager`. Done when `./build.sh --test` passes without warnings.

Group B2, UI: exercise and guides (`./build.sh --test` at step 9):
- [x] 7. The exercise window and Help > *First Song Exercise...* command; live ticks, step actions, *Show me*, the settings keys, completion. Opened also from the inspector welcome.
- [x] 8. `TheoryGuidesView`: the strip on *Start here*, card chips through `getShownTopics` with *+N more* and *Show fewer*.
- [x] 9. `ScreenshotRunner`: scene `firstSong` (opens the exercise; `"firstSongStep": n`), key `"startChoice"` (`"instrument:piano"`, `"instrument:drums"`, `"record"`, `"recordNoInput"`) that runs a tile's action and shows its pointer, key `"firstSongAction": true` that presses the open step's action button. Done when `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 10. Real-app check, `"song": "empty"`: the start panel at 1440 × 900 and 960 × 600 (`"window"`), with the inspector welcome; `startChoice` instrument:piano (pointer on the grid), instrument:drums, record (pointer on R), recordNoInput; `firstSong` at steps 1–4 before and after `firstSongAction` (ticks appear; chords C G Am F written; 48 drum notes); the Theory Guides scene on row 0 (strip, collapsed cards) and with `guidesSearch` "triplet" (match chip shown past the limit). By hand: Tab through tiles and exercise buttons; Start from a style on an empty song with edited chords asks to save; File > New Project from Style still works; guide read ticks survive; Esc dismisses the pointer. Look for cut text, overlap and a pointer covering its own target.
- [x] 11. Docs: README *Features* bullet (start panel and exercise); `docs/tests.md` (firstsong, getShownTopics); `docs/code-layout.md` (FirstSong, the pointer, the exercise window, start panel); `docs/ui-design.md` (start panel, pointer, exercise window, inspector welcome, card chips; fix "the master when null"); `docs/architecture.md` (ownership tree, `gen::firstsong`); `docs/recipes.md` *Checking in the real app* (new scene and keys). Update #159 (done, still open) and #173's package 06 line.

## Tests

In `tests/EngineTests.cpp`, `runFirstSongTests()`:
- Step 1 false on an empty edit, false with only a drum track's notes, true with one note on a Piano track.
- Step 2 false with no chords; false when equal to the baseline; true after one bar changes; false again when changed back.
- Step 3 false with 3 drum notes, true with 4; a track with DawIt Drums counts; a Bass track with 10 notes doesn't.
- Step 4 follows the two flags passed in (file and unsaved).
- Settings: missing, -1 and 9 read as step 0; done reads false unless true.
- The *Test numbers* table; writers refuse filled targets; the instrument is unchanged after a write; each guide link names an existing topic.

In `runTheoryGuidesTests()`: the `getShownTopics` rows.

## Website hand-off

- `features.html`: the learning group, "Start your song" choices and the first-song exercise.
- `learn.html`: the exercise as the first thing on *Start here*, before the reading path.
- Home or `for-you.html` (beginners): worth a line, if the user agrees.
- Screenshots: the empty song's start panel at 1440 × 900; the exercise window on step 2 beside the Chords row. No `about.html` change.

## Left for later

- The large Help button and goal journeys (singing, recording, creating) of #159; they reuse the pointer and step model. Kept open in #159.
- A full recording journey (input, level, monitoring, take): #178's preparation panel comes first.
- Find Action and task-based Add Track for songs that already have tracks: #179.
- Keyboard access to guide-card chips and screen-reader checks beyond the new buttons: #177.
- The exercise found by guide search ("first", "beginner") and more exercises (a melody, a recording): after observing beginners.
- The two-minute first-sound target: measured with participants, not promised.

## Changes while building

**Group A hand-off (engine, built and tested).**
- `src/generate/FirstSong.{h,cpp}`, namespace `daw::gen::firstsong` (in `CMakeLists.txt`, both targets). `getSteps()` gives title, subtitle, why, guide link; `getDoText (step, key, midiTrackName)` the "what to do" text (step 2 names the key's example chords and swap); `getActionLabel (step, midiTrackName, chordTrackEmpty, hasDrumTrack)` and `getActionTooltip (step, chordTrackEmpty, hasDrumTrack)` the button; `getTitle/getIntro/getCompletionText`; `getAllLinks()` (every guide page used, including the tile ones: *What MIDI is*, *Note numbers and pitch*, *Choosing an input*).
- Checks: `isStep1Done/2/3 (engine[, baseline])`, `isStep4Done (SavedState {hasFile, unsaved})`, or `isStepDone (step, engine, baseline, saved)`. `isDrumTrack (track)` (built-in Drums or Percussion sound, or DawIt Drums instrument), `findMidiTrack (engine)` (first non-drum MIDI track with a note, else first without). Settings keys `stepSettingKey` and `doneSettingKey`; `clampStep (int)`, `readDone (juce::var)`.
- Data and writers: `getExampleChords (key)`, `getSwapSuggestion (key)` (bar 3 and the chord), `getBasicBeat (bars)`, `canWriteChords`, `canWriteBeat (engine, clip)`, `writeExampleChords (engine)`, `writeBasicBeat (engine, clip)`; each writer commits pending edits first and is one named undo step; each returns false and changes nothing on a filled target.
- `gen::guides::getShownTopics (topicCount, matched, expanded, limit = maxShownTopics)` in `TheoryGuides.{h,cpp}` returns `ShownTopics { shown, hidden }`.
- Tests: `runFirstSongTests()` and four rows in `runTheoryGuidesTests()`.
- Not done by Group A and needed by B1: `ProjectManager` must expose "has a file and no unsaved changes" (step 6) to fill `SavedState`. Step 1's *Add a piano* path is the UI's: Group A has no "add instrument track" helper (use `Engine::addTrack`, `addMidiClip (track, 0, 4 * getBarLength())`). The DawIt Drums instrument branch of `isDrumTrack` is not covered by a test (it needs a plugin instance).

**Group B1 hand-off (start panel, pointer, inspector; built, `./build.sh --test` passes with no warnings).**
- `src/ui/GuidePointer.{h,cpp}` (the pointer; `MainComponent::pointer`, a child laid over the whole window). `pointer.show (findTarget, { caption, buttonText, onButton })`: `findTarget` is a `std::function<juce::Component*()>` asked every 100 ms for up to 1 s, so a control built a moment later (a new track's R button) can be pointed at; then the pointer follows the component and ends on its going away, 20 s, any mouse-down outside the bubble (global mouse listener), Esc (`MainComponent::keyPressed`) or its x. `dismiss()`, `isActive()`, `getCaption()`, `getTarget()`, `getBubbleBounds()` for the screenshots. Bubble goes below the target, else above, right, left, else inside its top left corner.
- `src/ui/StartPanel.{h,cpp}` (`StartPanel::Choice { style, instrument, record, importAudio, learn }`, `getTile (Choice)`), child of `ArrangementView` (`getStartPanel()`, `onStartChoice`, shown over the viewport area while `engine.getTracks().empty()`, set in `rebuildTracks`). `TrackLanes::paint` no longer draws the empty-song line (the panel has its own drop line). `ArrangementView::getArmButton (const Track&)` and `TrackHeader::getArmButton()` give the R button.
- `MainComponent` (private, so `ScreenshotRunner` can call them): `handleStartChoice (choice, tile)`, `showAddInstrumentMenu (target)`, `addInstrumentTrack (BuiltInSound)` (named step "Add instrument"; empty clip `4 * getBarLength()`; selects and reveals; `showBottomPanel (clipEditor)`; pointer on `pianoRoll.getGridArea()`), `addRecordingTrack()` (named step "Add recording track"; pointer on R; the no-input caption and *Audio/MIDI Settings...* button when `getNumInputChannels() == 0`). The Learn tile runs `theoryGuidesCommand` then `theoryGuides->showRow (0)`. Neither arms, records nor turns the computer keyboard on.
- `PianoRoll::onAddInstrument` / `updateAddInstrumentButton()` / `getGridArea()`: the *Add an instrument...* button under the "Select a MIDI clip" text, shown only with no MIDI track in the song.
- `InspectorPanel`: welcome state (no tracks and *Show the master* not pressed; ends when a track exists). For B2: set `inspector.onFirstSongExercise` (the *First song exercise...* button is hidden until it is set) and `inspector.getFirstSongProgress` (returns "2 of 4 done" or empty; polled at 30 Hz while the welcome shows). Both are NOT set yet in `MainComponent`.
- `ProjectManager::hasFile()` and `isEdited()` (public, cheap; `isEdited` follows the once-a-second check) fill `firstsong::SavedState { hasFile, unsaved }` for step 4.
- Differences: tiles wrap into 3 and 2 when they would be under 120 px wide (about 664 px of area with 16 px margins), not at exactly 640. On short areas (960 x 600 with the inspector open the area is about 160 px high) the tiles shrink to about 56 px, a tile then drops its subtitle (kept in its tooltip) and the dim line or footer may go first. Not yet looked at: the pointer bubble on screen (shot keys `startChoice` are step 9); the start panel at 960 x 600 and 1440 x 900 and the welcome were checked once (`empty960.png`, `empty1440.png` in the scratchpad) and show no cut text.

**Group B2 hand-off (exercise window, guide cards, screenshot support; built, `./build.sh --test` passes with no warnings).**
- `src/ui/FirstSongExercise.{h,cpp}` (`FirstSongExercise`, 360 x 500, in `CMakeLists.txt`): rows as focusable buttons, the open step's body in a viewport, Back/Skip/Next and the completion state. Owned by `MainComponent` through `pluginWindows.show (&exercise, ...)`; `MainComponent::showFirstSongExercise()`, `runExerciseAction (step)`, `showExerciseTarget (step)`, `openFirstSongLink`, `showStartHere()`, `getExerciseState()`. New command `firstSongExerciseCommand` (Help > First Song Exercise..., no shortcut). The chord baseline is `MainComponent::exerciseChordBaseline` (set when the exercise is first opened and after the example chords are written or *Start over*), so ticks survive closing the window. `inspector.onFirstSongExercise` and `getFirstSongProgress` are set ("N of 4 done" once started).
- `TheoryGuidesView`: `ExerciseState`, `getExerciseState`, `onExercise (restart)` and the strip (`ExerciseStrip`, Start here only, polled at 2 Hz); `Card` chips go through `getShownTopics` with *+N more* / *Show fewer*; expansion is kept in `expandedCards` while the window is open.
- New getters: `ArrangementView::getTrackHeader`, `getChordLane`, `showChordsRow` (unfolds the song rows and expands the Chords row); `TransportBar::getTypingButton`. `addInstrumentTrack (sound, typingIsOn)` has a caption for when the computer keyboard is on; `addRecordingTrack (pretendNoInput)`.
- ScreenshotRunner: scene `firstSong` (+ `"firstSongStep"`), key `"startChoice"` (`instrument:<sound>`, `record`, `recordNoInput`), key `"firstSongAction": true`; documented in `DevScreenshots.h`.
- Differences: *Show me* for step 3 points at the drum track's header (not its lane: lanes are one component). Step 1's button turns the computer keyboard on (Decision 7 reading: only through this button). The window opens at the main window's top right on first show only. Esc closes the window. Not yet looked at on screen (Group C): the exercise window at each step, the strip and the chips.

**Group C hand-off (real-app check and docs; `./build.sh --test` passes, no warnings).**
- Bugs found in the real app and fixed: (1) `FirstSongExercise` crashed on opening (`setSize` in the constructor called `resized()` before the rows and body existed); `setSize` is now the last line before `refresh()`. (2) Step 3's text showed "Fâ ˝2" for F sharp 2: the string went through `const char*`, now `juce::String::fromUTF8`. (3) The Start here strip never showed when the window opened from the menu (the view's first build ran before `getExerciseState` was set): `TheoryGuidesView::refreshCards()` is called in `MainComponent::showTheoryGuides`.
- Looked at (scratchpad `c/`): start panel at 1440 x 900 and 960 x 600 with the inspector welcome; `startChoice` piano (pointer below the grid, not over it), drums, record and recordNoInput (the machine has no input, so both showed the no-input caption with its Audio/MIDI Settings button; pointer under the R button); the exercise at steps 1-4 before and after `firstSongAction` (labels turn into *Open Piano...*, *Open the Chords row*, *Write a basic beat*); Theory Guides at row 0 (strip, collapsed cards with +N more) and "triplet" (the match chip is outlined past the limit). No cut text or overlap found. Tile subtitles shrink individually, so *Import audio* and *Learn* have smaller subtitles than the others at 1440 px: left as is.
- Not done (needs hands, no way to drive keys in the shot runner): Tab through tiles and exercise buttons, Esc dismissing the pointer, Start from a style asking to save, File > New Project from Style, guide read ticks surviving. Also not shown: a ticked step (needs a note, an edited chord, or a saved file) and the finished state.
- Docs updated: README bullet, `docs/tests.md`, `docs/code-layout.md`, `docs/ui-design.md` (inspector "master when null" and a first-song section), `docs/architecture.md`, `docs/recipes.md`. GitHub #159 and #173 not touched, as asked.
- **Website hand-off:** features: the Start your song panel and its pointer, the First song exercise (four steps, ticks by themselves, Show me), the inspector welcome and the Start here strip with card chips. Screenshots worth taking (`"song": "empty"`, scale 2): the panel at 1440 x 900 (`start1440`), `startChoice instrument:piano` with the pointer, and the exercise window at step 2 (`"scene": "firstSong", "firstSongStep": 1`, `"capture": "window"`), plus `theoryGuides` row 0. No library, model or tool added; no new guide.

**New Project crash (coordinator's report), checked and extended.**
- The `arrangement.rebuildTracks()` in `project.afterReplace` is right, but not enough: `Selection` and the engine also tell listeners only by async message, so the inspector, mixer, device bar, piano roll and score view could paint or tick on deleted tracks until it arrives. `afterReplace` now also calls `engine.dispatchPendingMessages()` (after the rebuild), selects the first track, then `selection.dispatchPendingMessages()`; `replaceEdit` runs synchronously with no message loop between `beforeReplace` and `afterReplace`, so this covers New, Open and `newProjectFrom`.
- Feature code checked: `FirstSongExercise` keeps no track or clip pointers (it reads the engine fresh); `GuidePointer` follows a `SafePointer` and ends when its target goes. `exerciseChordBaseline` was stale after a replace: `afterReplace` now resets it to the new song's chords once the exercise has begun.
- Shot key `"replaceProject": "new"` (or a `.dawit` path, via the new `ProjectManager::openFileWithoutAsking`) in `DevScreenshots.{h,cpp}` and `docs/recipes.md`. The Open path is untested (no .dawit file to hand).
- Proved: pop song, Bass selected, inspector and mixer open, 1.5 s of ticks, then `replaceProject: "new"` with a 2 s wait (no crash; start panel, welcome and empty mixer shown), then a piano tile, then New Project again. Shots `n1` to `n4` in the scratchpad `c/`.

**After the build (user feedback).** File > New Project crashed after adding tracks: the old track headers' timers and paints read deleted tracks before the engine's async change message rebuilt them. `project.afterReplace` now rebuilds the arrangement and dispatches the engine's and selection's pending messages at once (covers New, Open, Open Recent and style templates); checked with the `replaceProject` shot key. The exercise window is now always on top (user's request). Still to check by hand: Tab and Return/Space on tiles and exercise buttons, Esc on the pointer, Start from a style asking to save; the two-minute first-sound target is to be measured with beginners.
