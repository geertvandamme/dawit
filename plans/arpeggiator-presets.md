# An arpeggiator that plays and explains chord patterns

Status: Released
Idea: [#84 — An arpeggiator that plays and explains chord patterns](https://github.com/geertvandamme/dawit/issues/84)

Follow-up to [arpeggiator.md](arpeggiator.md) (Released): many more starting points, and a Randomise button. The 15 existing starting points, their order, names, whys and numbers stay as they are.

## Goal
A beginner who opens the Starting point menu finds a pattern for most styles (trance gates, synthwave pulses, Alberti and fingerpicking, waltz, Charleston, shuffles), each with a "why". A new **Randomise** button makes a fresh random step pattern in one click, so a producer can find a rhythm they would not have painted, and **Undo** brings the old one back.

## Decisions for you
1. **What Randomise changes.** Options: (a) the step row only (Note, Chord, Bass, Rest, Tie and accents); (b) the step row plus Order and Octaves; (c) everything, including Rate, Gate, Swing. **Recommended: (a)**, because Rate, Octaves and Gate decide the speed and character the user picked, so the result stays in the same style and never gets too fast. Shift-click for (b) is easy to add later.
   Answer: (a) the step row only.
2. **Undo.** Options: (a) an **Undo** button next to Randomise that restores the settings from before the last press (editor-only, one level, cleared when the window closes); (b) none, as the settings knobs have no undo today. **Recommended: (a)**, cheap and like "Try another" with Undo on the chord progression.
   Answer: (a) a one-level Undo button next to Randomise.
3. **Menu layout.** Options: (a) one list with section headings (Basics, Single notes, Dance and synth, Piano, guitar and harp, Chords and rhythms) via `ComboBox::addSectionHeading`; (b) sub-menus (needs a PopupMenu in place of the ComboBox in the shared `EffectEditor`, touching all 17 effect editors). **Recommended: (a)**: 51 entries scroll fine under headings and the other effects are untouched.
   Answer: (a) one list with section headings.
4. **Number of new presets.** 36 proposed below. Options: all 36, or drop the 6 marked (?) whose pattern name I could not confirm from a source. **Recommended:** keep all 36 but word the unconfirmed ones as "a figure like ..." without claiming a tradition (see *Theory*).
   Answer: keep all 36, unconfirmed ones worded loosely.
5. **Rolled chord every beat** plays 12 notes a second at 120 BPM (the guide's "too fast" line is above 12), so it warns above 120 BPM. Keep it? **Recommended:** yes; it is a roll, not a line to follow.
   Answer: yes, keep it.

## What the user does and sees
- **Starting point menu** (header of the arpeggiator window): the 15 existing entries first, in the same order, under a heading *Basics*; then 36 new ones under four headings. The shared `EffectEditor` gets an optional list of headings (name and first index); other editors pass none and look the same. Picking one applies it as today, shows its why in the *Starting points* topic, and the pattern shows in the step row.
- **Randomise** button in the arpeggiator's Pattern group (next to Latch), with **Undo** beside it, disabled until a Randomise has been pressed. Each press gives a new pattern, keeping Order, Octaves, Rate, Gate, Swing, Steps, Accent and Latch (decision 1). It sounds at once if the song plays or Test chord is held. Tooltips: Randomise "Paint a new random pattern of notes, chords, bass notes, rests and ties. Rate, octaves and everything else stay as they are. Press again for another."; Undo "Go back to the pattern before the last Randomise." Menu item *Randomise pattern* in the step row's right-click menu too, ending as today with the guide.
- Nothing new is saved: the settings and pattern are already in the project.

## Code
Recipes: *a built-in effect* (starting points, editor) in [docs/recipes.md](../docs/recipes.md).
- `arp::getPresets()` appends 36 entries after the 15 (indices 0..14 unchanged, saved nowhere by index). `Preset` gets a `group` name so the editor can build the headings; the doc comment on `getPresets` says "fifty-one".
- `arp::randomisePattern (Settings, seed)`: pure and deterministic; returns the settings with new kinds and accents only. Rules, for a pattern of n steps: step weights Note 55 %, Chord 10 %, Bass 10 %, Rest 15 %, Tie 10 %; step 1 is never Rest or Tie; a Tie never follows a Rest; never more than 3 Rests in a row; at least 3 sounding steps (Note, Chord or Bass) and at least 2 Note steps when n is 4 or more; with fewer steps, all Notes. Accents: step 1 always, each later beat-start 35 %, off-beats 10 %, at most a third of the sounding steps. Number of steps, rate and swing are never touched, so the notes a second never exceed the Rate's maximum (they can rise above the preset's own figure when rests become notes).
- The editor draws a fresh seed (time based) on each press, keeps one "before" copy of the settings for Undo, and sets the settings through `setSettings` on the message thread as the preset menu does. No audio-thread change.
- Small-feature check: one recipe, no saved data, no guide topics (one new line in the Starting points and Pattern topics). It is about eight steps, a little over the five-step size, but nothing needs theory beyond the facts below, so it stays on the small route.

## Libraries and models
None.

## Theory
Sources read (add under *Arpeggiator* in SOURCES.md): <https://en.wikipedia.org/wiki/Alberti_bass>, <https://www.musicradar.com/news/overused-production-technique-trance-gate> (gates chop a sustained sound into a stutter), <https://www.andyguitar.co.uk/videos/02-int-elec-l2-the-charleston-rhythm-aka-the-cant-buy-me-love-beat> (Charleston: hits on beat 1 and the "and" of 2), <https://www.guitarworld.com/lessons/how-fingerpick-merle-travis> (Travis picking: the thumb keeps a steady alternating bass while the fingers play higher notes), the Roger Linn page already listed (50, 54, 66 % swing). Searches: classic arpeggiator patterns trance synthwave house; Charleston rhythm and dotted-eighth delay; Travis picking and the waltz bass. Not confirmed from any source: the exact patterns of named songs (not used), the waltz oom-pah-pah, 12/8 ballad figures, piano-house stabs and the syncopations marked (?). The patterns are common figures made up for the app, so their whys describe the sound and the style loosely ("as in ...") and never claim a specific record.

### New starting points (Order, Octaves, Rate, Gate, Swing, Steps; N Note, C Chord, B Bass, R Rest, T Tie, ' accented; all Accent 20, Latch off)
**Single notes** (heading *Single notes*)
16. **Down** — Down, 1, 1/16, 80, 50, 16 × N. "The plain arpeggio turned upside down: from the top of the chord to the bottom, four a beat. Falling lines feel calmer than climbing ones."
17. **Down and up** — Down and up, 2, 1/16, 80, 50, 16 × N. "Down two octaves and back up without repeating the ends: the mirror of Up and down, starting from the top."
18. **As played** — As played, 1, 1/8, 85, 50, 8 × N. "The notes in the order you pressed them, in eighths. Press the keys in your own order to write a tune with a chord."
19. **Wide climb** — Up, 3, 1/8, 90, 50, 16 × N. "Three octaves in eighths: a slow sweep that fills the whole keyboard. Good under a held pad."
20. **Staccato eighths** — Up, 1, 1/8, 30, 50, 8 × N. "Short, dry notes with air between them: a plucked, light feel. A short gate makes the same notes sound quicker."
21. **Legato sweep** — Up and down, 2, 1/16, 120, 50, 16 × N. "Each note rings a little into the next, like a harp with the pedal down: a smooth blur rather than separate notes."
22. **Closing in** — Outside in, 2, 1/8, 90, 50, 8 × N. "Lowest, highest, then inwards, over two octaves: a figure that squeezes towards the middle."
23. **Slow tower** — Up, 4, 1/8, 100, 50, 16 × N. "Four octaves in eighths: a long climb that takes two bars before it starts again, for builds and intros."
24. **Random eighths** — Random, 2, 1/8, 70, 50, 8 × N. "Chord tones picked by chance in eighths: a calmer random line than Random sparkle. The same each time the bar plays."
25. **Random and rests** — Random, 2, 1/16, 50, 50, 16: N N R N N R N R, twice. "Random chord tones with gaps: the rests make it sound like a played line, not a stream."
**Dance and synth** (*Dance and synth*)
26. **Three and a rest** — Up, 2, 1/16, 60, 50, 16: N N N R × 4, steps 1 5 9 13 '. "Three sixteenths and a gap, four times a bar: a galloping drive. The rest gives the line room to breathe."
27. **Gated climb** — Up, 2, 1/16, 55, 50, 16: N N R N, N R N N, N N R N, N R N R. "The chord climbing through a stuttering gate: rests chop it the way a trance gate chops a pad (?)."
28. **Bass on every beat** — Up, 2, 1/16, 60, 50, 16: B N N N × 4, steps 1 5 9 13 '. "The bass note on each beat and three chord notes between: a bassline and an arpeggio from one chord."
29. **Off-beat bass** — Up, 1, 1/8, 60, 50, 8: R B R B R B R B. "The bass between the beats, leaving the beats to the kick: the off-beat bass of trance and house."
30. **Sixteenth bass pulse** — Up, 1, 1/16, 50, 50, 16 × B, steps 1 5 9 13 '. "The bass note in short sixteenths, louder on each beat: a driving, relentless pulse for dance and synthwave."
31. **Eighth bass pulse** — Up, 1, 1/8, 70, 50, 8 × B, steps 1 5 '. "The bass note in steady eighths, strongest on 1 and 3: a simple, solid synth bass for synthwave and rock."
32. **Trance gate chords** — Up, 1, 1/16, 60, 50, 16: C' C R C, C R C C, C' C R C, C R C C. "The whole chord chopped into a rhythm, as a gate chops a pad: hold a chord and it stutters in time."
33. **Pumping eighths** — Up, 1, 1/8, 50, 50, 8 × C, steps 1 3 5 7 '. "The chord on every eighth, louder on the beats: an even pump for house and pop choruses."
34. **Syncopated stabs** — Up, 1, 1/16, 50, 50, 16: C R R C, R R C R, R C R R, C R R R (?). "Short chords on unexpected sixteenths: the off-beats make it push forward against the beat."
35. **Dotted-eighth arp** — Up, 2, 1/16, 70, 50, 8: N R R N R R N R, steps 1 4 7 '. "Notes three sixteenths apart, then two: the figure a dotted-eighth delay makes over straight eighths, played by the arpeggiator itself (?)."
36. **Five against four** — Up, 2, 1/16, 70, 50, 5 × N. "A loop of five sixteenths over a bar of sixteen: it lines up with the bar only every 5 bars, so the pattern keeps shifting."
**Piano, guitar and harp** (*Piano, guitar and harp*)
37. **Alberti sixteenths** — Alberti, 1, 1/16, 90, 50, 16 × N. "Low, high, middle, high, twice as fast as Alberti bass: the classical left-hand figure with more motion."
38. **Waltz: bass and two chords** — Up, 1, 1/4, 80, 50, 3: B C' C (?). "The bass on the first beat and the chord on the next two: the oom-pah-pah of a waltz. Three beats a pattern, so it crosses the bar and lines up after 3 bars."
39. **Fingerpicking: bass twice a bar** — Up, 2, 1/8, 100, 50, 8: B N N N B N N N. "A bass note, then three higher notes, twice a bar: the plain pattern of fingerpicked folk songs."
40. **Alternating bass and notes** — Up, 1, 1/8, 100, 50, 8: B N B N B N B N. "The bass on every beat with a chord note between: the steady thumb of Travis picking, as far as one lowest note allows."
41. **12/8 ballad** — Up, 2, 1/8T, 100, 50, 12: B N N × 4. "Three to the beat with the bass on each beat: the rolling feel of a slow 12/8 ballad (?)."
42. **Rolled chord every beat** — Up, 2, 1/32, 100, 50, 8: N N N N N N T T. "The chord rolled upwards, fast, on every beat, like a harp or a strummed guitar. Fast: at 120 BPM it is 12 notes a second."
43. **Ringing harp** — Up and down, 2, 1/8, 200, 50, 8 × N. "Every note rings for two steps: a wash of overlapping notes, as a harp sounds with every string left to ring."
44. **Loose sixteenths** — Up and down, 1, 1/16, 70, 54, 16 × N. "Sixteenths with a touch of swing, 54 %, so they feel played, not programmed."
45. **Shuffle** — Up, 1, 1/8, 80, 66.67, 8 × N. "Eighths played as long, short: 66.67 % swing is the triplet shuffle of blues and jazz."
**Chords and rhythms** (*Chords and rhythms*)
46. **Backbeat chords** — Up, 1, 1/4, 80, 50, 4: R C' R C'. "The chord on beats 2 and 4, where the snare falls: a steady backbeat."
47. **Charleston** — Up, 1, 1/8, 60, 50, 8: C' R R C R R R R. "Two chords a bar, on beat 1 and the and of 2: the Charleston rhythm of jazz comping and early rock and roll."
48. **Pad chord every bar** — Up, 1, 1/4, 95, 50, 4: C T T T. "One chord held for the whole bar: the simplest backing, and the way to hear a chord before you add a pattern."
49. **Half-note chords** — Up, 1, 1/4, 95, 50, 4: C T C T. "A chord held for two beats, then another: slow, open and easy to sing over."
50. **Bass, note, chord, note** — Up, 1, 1/8, 80, 50, 8: B N C N B N C N. "Bass, a single note, the whole chord, a single note: a two-handed piano accompaniment from one held chord."
51. **3-3-2: bass and stabs** — Up, 1, 1/8, 60, 50, 8: B R R C R R C R. "The 3-3-2 rhythm with the bass on the first hit and chords on the others: the bass and the stabs in one pattern."
(Pattern 51 uses the existing *3-3-2 stabs* grid; Waltz alone is 3 steps, all others 4, 5, 8, 12 or 16.)

### Randomise: test numbers
Seeds 1 to 200 on every preset's settings: step 1 sounds; no Tie after a Rest or at step 1; at most 3 Rests in a row; at least 3 sounding steps (steps 4 and up) and at least 2 Notes; accents at most a third of the sounding steps and step 1 accented; Order, Octaves, Rate, Gate, Swing, Steps, Accent, Latch unchanged; the same seed gives the same pattern, different seeds give at least 20 different patterns of 200; a pattern survives `patternToText` and back; the notes a second after Randomise never exceed the Rate's maximum (Rate and Steps are kept).

## Steps
Group A, engine and UI (`./build.sh --test` at step 5):
- [x] 1. Add the 36 presets to `arp::getPresets()` after the 15, with a `group` name per preset (*Basics* for the first 15); update the doc comment. Done when the 15 are unchanged and the new patterns round-trip as text.
- [x] 2. `arp::randomisePattern` (*Code*) and its tests. Tests: the *Randomise: test numbers* rows.
- [x] 3. Shared `EffectEditor`: optional section headings for the Starting point menu (`ComboBox::addSectionHeading`); the arpeggiator passes its groups. Other editors unchanged.
- [x] 4. Arpeggiator editor: Randomise and Undo buttons with tooltips and the step row's right-click item; Undo keeps one copy of the settings; update the *Starting points* and *Pattern* topic lines in `ArpeggiatorGuide.cpp` to mention Randomise and the groups (check the guide's text for "fifteen").
- [x] 5. `runArpeggiatorTests`: replace the "fifteen" check (`presets.size() == 15`, 15 distinct names) with 51 names, whys over 40 characters, every setting inside its knob range, Steps matching the pattern, and spot checks (Down reads 67 64 60 for C E G; Waltz realigns after 4 patterns; Five against four after 16 patterns = 5 bars; Shuffle's second eighth starts at 2/3 beat; every preset except Rolled chord stays at or under 12 notes a second at 120 BPM and Rolled chord is exactly 12; presets 1 to 15 equal their *arpeggiator.md* numbers).

Group B, check and docs:
- [x] 6. Real-app check (`--screenshots`, scale 1, window capture): the arpeggiator open with the Starting point menu showing the headings; after Randomise with the new pattern in the step row and Undo enabled. Add a `ScreenshotRunner` setting only if the menu cannot be opened by the existing "open menu" step.
- [x] 7. Docs: README (the arpeggiator bullet: "fifteen starting points" becomes "fifty-one starting points in five groups, and Randomise with Undo"), `docs/tests.md` line for DawIt Arpeggiator ("the 15 starting points" becomes 51, add Randomise), the comment in `BuiltInArpeggiator.h`, SOURCES.md (the pages above under *Arpeggiator*), `docs/recipes.md` (starting-point headings, one line, if the recipe lists the preset menu). `website/features.html` line 499 says "Fifteen starting points": hand-off to dawWebsite. The issue gets the plan link and the new checks.

## Website hand-off
`features.html`: the starting point count and groups, and Randomise with Undo. Screenshot: the open Starting point menu.

## Changes while building
- Randomise does not keep the notes a second from rising: a pattern with many rests can come out with more sounding steps. Rate and Steps are kept, so it never exceeds Rate's maximum (the test checks that bound), not the preset's own figure. The *Randomise: test numbers* line saying otherwise is wrong.
- A pattern of fewer than 4 steps (the Waltz) randomises to all Notes with step 1 accented.
- Added a `"randomise": <seed>` setting to `ScreenshotRunner` (documented in `docs/recipes.md`); the Starting point menu and Undo cannot be opened by a shot list, so they were not photographed.
- The 36 whys are as in *Theory* except: the Waltz, Dotted-eighth arp and Gated climb texts lost their (?) markers and say "waltz-style", "a figure like"; Off-beat bass says "heard in" instead of "of".
- The Randomise button sits left of the brush buttons, after the picture's title.
- Theory check: Down says falling lines "often" feel calmer (as the guide's Order topic says); Down and up goes "down through" two octaves.
- Theory check: Wide climb no longer "fills the whole keyboard" (three octaves is under half of 88 keys); Staccato eighths "can make" notes sound quicker.
- Theory check: Legato sweep compares the overlap to harp strings left to ring, not "a harp with the pedal down" (a harp's pedals change pitch, they don't sustain).
- Theory check: Slow tower's climb is a bar and a half for a three-note chord (12 notes in eighths; the order runs on across pattern loops), not "two bars".
- Theory check: Dotted-eighth arp now says notes a dotted eighth apart, like a dotted-eighth delay's repeats (a delay over straight eighths does not make a 3-3-2 figure).
- Theory check: Charleston is named after the 1923 tune and a favourite of jazz comping; "early rock and roll" was unsourced and is gone.
- Theory check: the Waltz accents step 1 (the bass, the "oom") instead of step 2, as a waltz stresses its first beat; the step-kinds test checks it.
- Theory check: one line on Randomise and Undo in the guide's *Chords, bass notes and rests* topic (step 4 had not added it); the overview's Undo sentence reworded. Charleston source added to SOURCES.md.
- User decisions after the theory check: "Pedal harp" renamed to "Ringing harp"; the Waltz accent stays on step 1.
