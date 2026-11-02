# An arpeggiator that plays and explains chord patterns

Status: Released
Idea: [#84 — An arpeggiator that plays and explains chord patterns](https://github.com/geertvandamme/dawit/issues/84)

## Goal
DawIt Arpeggiator is a built-in MIDI effect: hold a chord (keyboard, computer keyboard or a MIDI clip of block chords) and it plays the chord as a pattern in time with the song: single notes in an order, over octaves, plus whole-chord, bass, rest and tie steps. Fifteen starting points go from a plain climbing arpeggio to Alberti bass, off-beat stabs and the 3-3-2. Its guide, "What is an arpeggiator?", explains chord tones, note order, octaves, note length, accents, swing and syncopation, patterns that cross the bar and how a pattern fits the song, with a *Your track* page that checks the arpeggio against the chord track. DawIt has no MIDI effects yet, so this also adds host support: MIDI effects run before the instrument on MIDI tracks.

## Decisions for you
1. **Plugin build.** Options: app only for now; or also an AU MIDI effect (`aumi`) and a VST3, which needs a new kind in `dawit_add_builtin` and host checks in Logic, Live and Reaper. **Recommended:** app only (`APP_ONLY`), with the plugin build a follow-up issue, because every DAW already has an arpeggiator and hosts treat MIDI-effect plugins differently (AU has a MIDI-processor type Logic hosts; VST3 support varies by host).
   Answer (user, 2026-11-02): app only (`APP_ONLY`); plugin build is a follow-up issue.
2. **Third-party MIDI effects.** Options: the host support places any plugin that reports itself a MIDI effect before the instrument, but the add menu lists only DawIt's own; or also list scanned MIDI-effect plugins. **Recommended:** DawIt's own only now; listing scanned ones needs the scan to record which are MIDI effects, and testing with real plugins.
   Answer (user, 2026-11-02): DawIt's own MIDI effects only in the add menu.
3. **Turning the arpeggio into notes** (a command that writes what it plays into a MIDI clip). Options: now; later. **Recommended:** later (own issue): recording keeps the chords, which stays editable, and *Play chords as...* already writes out arpeggio parts.
   Answer (user, 2026-11-02): later, as its own issue.
4. **Random order.** Options: repeatable (chosen from the step's place in the song, so a bar plays the same each time); new on every pass. **Recommended:** repeatable, so a part you like stays, bounces match playback and tests are exact.
   Answer (user, 2026-11-02): repeatable.
5. **Sustain pedal.** Options: the arpeggiator treats the pedal as holding the keys and doesn't pass it on; or passes it to the instrument. **Recommended:** holds the keys, because a pedal passed on makes every arpeggio note ring into the next, which a beginner can't tell from a bug.
   Answer (user, 2026-11-02): holds the keys, not passed on.
6. **Guide area.** Options: MIDI ("Notes as numbers, and how they reach an instrument"); Effects. **Recommended:** MIDI: it changes notes, not sound; not on *Start here*.
   Answer (user, 2026-11-02): MIDI area.

## What the user does and sees
- **Adding it:** on a MIDI track, *+ Effect* (device bar, inspector) gets a first section **Before the instrument** with "DawIt Arpeggiator  -  turns held chords into patterns". Not offered on audio tracks, buses or returns. Its chip sits left of the instrument chip in the device bar and above the instrument row in the inspector; it can be dragged only among MIDI effects. Its chip shows the step row, the playing step lit.
- **The window** (1240 × 620, `fxui::EffectEditor`): header with starting points, *?* guide button and **Test chord** (holds the chord track's chord of the current bar, its notes from C4 up, or C4 E4 G4 without a chord, for 2 bars). Knob groups: *Notes* (Order switch, Octaves 1–4), *Rhythm* (Rate 1/32 … 1/4 from `stepgrid::stepLengths`, Gate 10–200 %, Swing 50–75 %), *Pattern* (Steps 1–16, Accent 0–40, Latch switch). Middle: a keyboard strip (the held keys outlined and labelled R, 3, 5, 7…, the playing note lit, the held chord's name above, e.g. "Am7"); below it the step row; at the bottom, in place of the level history, the last 2 bars of notes it played as a small piano roll against the beats.
- **Step row** (copy `SlicerPatternView`): brushes Note, Chord, Bass, Rest, Tie (keys 1–5). Click gives a step the brush (a step already in it becomes Note), drag paints, Shift-click toggles its accent, double-click or Delete makes it Note, Cmd+C/V copy and paste the pattern as a line ("DawIt Arpeggiator pattern: 1/16 16 NNcR…", accented steps in lower case), right-click: the five kinds, Accent, Copy, Paste, Clear, then the guide. Tooltip: "Step 3 of 16 · Chord · accented. Click …".
- **Order switch** opens a menu with each order, a one-line description and the guide last.
- Tooltips on every control (texts in *Theory*). Nothing is kept in app settings.

## Data and saving
All settings, the 16 step kinds and accents are `AudioProcessorValueTreeState` parameters, saved in the plugin state like any built-in. No project format change. Plugins load in their saved order and the engine puts MIDI effects first anyway. An older DawIt reports the unknown plugin and loads the rest. Tests: `checkThroughProject<BuiltInArpeggiator>` and the chain order (arpeggiator, instrument, effect) after save and load.

## The guide
"What is an arpeggiator?" (`fxui::Guide` pages in `src/ui/ArpeggiatorGuide`), opened from the window's *?*, the chip's right-click menu and the Theory Guides window. Each page except the last has a listening example: its button asks the processor to play a fixed example (worked out by the `arp::` functions, without changing the settings) into the instrument; with no host track the button is disabled and says "Put DawIt Arpeggiator on a MIDI track to hear this".

| Page | Summary | Picture |
|---|---|---|
| Chords, one note at a time | What an arpeggio is, and what an arpeggiator does | C4 E4 G4 as a block chord, then as 1/16 Up, in two piano-roll strips |
| Chord tones | The notes of the chord: root, third, fifth | keyboard with the held keys labelled R 3 5 (7), live; C major when nothing is held |
| Note order | Up, down, and the patterns in between | the current order over the held notes (or C E G) as numbered dots on a pitch × step grid |
| Octaves | Spreading the chord over more of the keyboard | the sequence at 1, 2 and 3 octaves, the current setting highlighted |
| Rate and note length | How fast it steps, and how long each note lasts | notes at the current rate with gate 50, 100 and 150 %, ms per step at the song tempo |
| Chords, bass notes and rests | The steps of the pattern | the current step row above the notes it makes |
| Accents, swing and syncopation | Where the weight falls | 8 eighths with the 3-3-2 hits, and a pair of 1/16 at 50, 54 and 67 % swing |
| Patterns that cross the bar | When the notes and the steps don't line up | 3 bars of 3-note Up over 16 steps, the C marked each time it comes round |
| How a pattern fits the song | Chord changes, range and space | 4 bars of the song's chord track with the arpeggio's notes and the bar's chord tones |
| How DawIt Arpeggiator works | Before the instrument, on the song's clock | the chain: notes in → arpeggiator → instrument → effects, the playhead's step |
| Your track | What it played on this track | the last 2 bars from the step record, notes outside the bar's chord marked |

Theory Guides entry: `Guide::arpeggiator`, Area MIDI, title "What is an arpeggiator?", summary "Turning held chords into patterns: chord tones, note order, octaves, note length, accents, swing and syncopation, and your track.", `opensFrom` "DawIt Arpeggiator's window (What is an arpeggiator?)", `plugin` "DawIt Arpeggiator", keywords: arpeggiator arpeggio arpeggios arp arps broken-chord broken chord chord-trigger alberti pattern order up down latch hold gate swing shuffle accent syncopation off-beat tresillo 3-3-2 octaves rate midi-effect. Not on *Start here*.

## Code
Recipes: *A built-in effect or instrument* (copy DawIt Slicer: steps, `stepgrid::Clock`, step records, pattern view), plus the host support below.

- **A MIDI effect** is a plugin slot whose instance says `isMidiEffect()`: it changes the track's MIDI buffer and never its audio. The slot remembers this when it's made.
- **Chain order invariant:** on a MIDI track, MIDI effects come first, then the instrument, then audio effects. `Engine::addPlugin` inserts a MIDI effect after the existing ones whatever index it's given, and refuses it on audio tracks, buses and returns ("A MIDI effect changes notes: put it on a MIDI track."); `movePlugin` keeps MIDI effects among themselves and other plugins out of their places.
- **Render order in `Track::process`:** clip, live, preview and flush events into `midi`, then every MIDI effect in order, then the built-in synth or instrument, then the audio effects. Today the built-in synth renders before `runPlugins`, so this is the change that makes it hear the arpeggiator. MIDI effects run also in `instrumentOnly` bounces (they make the notes) and whether or not the song plays (live playing).
- **Bypass and removal:** a bypassed MIDI effect is still called, through `processBlockBypassed`, so it ends its own notes and passes MIDI through. Removing or bypassing one bumps `midiEditVersion` so the instrument's notes are released.
- **Every `! isInstrument` that means "an audio effect"** (latency sum `getCompensation`/`getEffectsLatency`, gain staging badges, stage meters in `runPlugins`, `SignalFlow`, `RoutingGuide`'s chain checks, device bar and inspector rows; grep `isInstrument`, about 25 files) skips MIDI effects. This is the wide part; each site gets a line of thought, not a blanket replace.
- **`arp::`** (in `BuiltInArpeggiator.{h,cpp}`): pure functions on the settings: the note sequence for held notes, order and octaves; what a step plays; step start times with swing; note length with gate and ties; the repeat length of notes against steps; notes per second; the pattern text; the starting points; `analyse`. Tested.
- **The processor** (audio thread only): the held notes (pitch, velocity, press order, up to 16), latch and pedal state, the notes it has started, the order counter (reset when a chord starts from nothing). It walks each block sample by sample against `stepgrid::Clock` (song steps from the play head; own clock at the song tempo, restarted at a new chord, while stopped), handles input events before a step starting on the same sample, and never sends a note-on for a pitch it already sounds (it ends it first). All-notes-off (a flush: stop, seek, loop wrap, note edit) forgets held and latched notes and ends its own. It implements `SongContextReceiver` for the bar's chord (test chord, *Your track*). Step records (step, kind, held and played notes, chord pitch classes, beat) go to the UI through a lock-free FIFO, as in DawIt Slicer. Test chord and page examples are requested by atomics and played by the audio thread.
- Register in `BuiltInFormat` as a separate MIDI-effects list (the add menu uses it) and with `dawit_add_builtin (Arpeggiator … KIND effect … APP_ONLY)`, code `Arpg`.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | An arpeggio is a chord's notes played one after another; from Italian *arpeggiare*, to play on a harp | Wikipedia *Arpeggio* | exact |
| 2 | Arpeggios open Beethoven's "Moonlight" Sonata and run through it | Wikipedia *Arpeggio* | exact |
| 3 | Arpeggiators have controls for speed, range and mode; trance leads use fast 1/16 arpeggios | Wikipedia *Synthesizer* (Arpeggiator) | typical |
| 4 | Roland Juno-60 (1982): arpeggio modes Up, Up and down, Down; range 1–3 octaves; Hold | Roland Juno-60 specifications; JU-06A manual (search) | exact |
| 5 | Alberti bass: low, high, middle, high; after Domenico Alberti (1710–1740/46), who used but didn't invent it; Mozart K. 545 opening | Wikipedia *Alberti bass* | exact |
| 6 | Chord Trigger repeats the held notes as a block chord; Play Order follows the order played; Gate above 100 % overlaps (legato); Hold keeps the pattern after release | Ableton Live manual, MIDI effects | typical (how DAWs do it) |
| 7 | Major third 4 semitones, minor third 3, fifth 7, minor seventh 10, major seventh 11 | common knowledge; agrees with the chords and singing guides | exact |
| 8 | An octave up doubles the frequency: A4 440 Hz, A5 880 Hz | common knowledge (ISO 16 for A4) | exact |
| 9 | MIDI velocity 1–127; middle C is note 60 (C4) | MIDI 1.0; as in "What is MIDI?" | exact |
| 10 | Step lengths: 1/4 = 1 beat, 1/8 = 0.5, 1/16 = 0.25, 1/32 = 0.125, 1/8T = 1/3, 1/16T = 1/6 | `stepgrid`, Slicer guide | exact |
| 11 | 120 BPM: 1 beat 0.5 s, 1/16 125 ms, 8 notes/s; 140 BPM: 1/16 9.33/s, 1/32 18.67/s | derived (140/60 × 4, × 8) | exact |
| 12 | Syncopation: weight moved from strong beats to weak ones: off-beat accents, rests on strong beats, ties across the beat, anticipation | Wikipedia *Syncopation* | exact |
| 13 | A producer (Rick Snoman) says all dance music uses syncopation | Wikipedia *Syncopation* | opinion |
| 14 | Tresillo 3+3+2: hits on pulses 1, 4, 7 of 8 | Wikipedia *Tresillo*; derived positions | exact |
| 15 | Swing (Linn): 50 % straight; 66 % triplet swing, the second 16th of each pair on the eighth-note triplet; only the even steps move; 54 % loosens the feel without sounding like swing; MPC range 50–75 % | Roger Linn (brettworks interview); search results | exact (50, 66.7 %); opinion (54 %) |
| 16 | 3 notes over 16 steps line up every lcm(3,16) = 48 steps = 3 bars of 1/16 | derived | exact |
| 17 | Polymetre: cycles of different lengths over a constant beat, realigning after a common multiple | Wikipedia *Polymeter* | exact |
| 18 | 6 steps of 1/16 = 1.5 beats; realigns with the bar after 12 beats (8 patterns, 3 bars) | derived | exact |
| 19 | Up and down without repeating the ends: cycle 2m − 2 for m notes (3 → 4, 6 → 10) | derived | exact |
| 20 | Fast low notes (below about C3) blur and fight the bass; above about 12 notes a second notes are hard to follow one by one; leave the vocal's range free; arpeggios work as contrast between sections | common production advice; DawIt's thresholds | opinion |

### Topic texts

**Chords, one note at a time** — What an arpeggio is, and what an arpeggiator does

An arpeggio is a chord played one note after another instead of all at once. Hold C, E and G on a piano together and you hear a block chord, C major. Play them C, E, G, C, E, G in a steady rhythm and you hear the same chord as a moving line. The word comes from the Italian arpeggiare, to play on a harp, where chords are naturally played this way.

An arpeggiator does it for you. You hold a chord, or a MIDI clip holds it, and the arpeggiator plays its notes in turn, in time with the song: four notes a beat at 1/16, in the order you choose, over one or more octaves. You choose the chord; the arpeggiator chooses the rhythm and the order.

Arpeggios are everywhere: the opening of Beethoven's "Moonlight" Sonata, a fingerpicked guitar ballad, the fast sixteenths of trance. Synthesisers have had arpeggiators for a long time. Roland's Juno-60, from 1982, had three orders (Up, Up and down, Down), a range of 1 to 3 octaves and a Hold switch, and those are still the basic controls.

DawIt Arpeggiator goes further: a step of its pattern can also play the whole chord, the bass note, a rest or a longer note, so the same chord can become a broken chord, a stab or a piano part.

It's a MIDI effect: it changes notes, not sound. It sits before the instrument, so what you hear depends on the instrument after it. A short pluck makes each note clear; a slow pad blurs them into a shimmer.

**Chord tones** — The notes of the chord: root, third, fifth

The notes an arpeggiator plays are the notes you hold, so the first thing that decides how it sounds is the chord.

A triad, the most common chord, has three notes. The root names it. The third is 3 or 4 semitones above the root: 4 for a major chord (C to E), 3 for a minor one (A to C). The fifth is 7 semitones above the root (C to G, A to E). So C major is C E G and A minor is A C E. Add the note 10 or 11 semitones above the root and you have a seventh chord: Am7 is A C E G, Cmaj7 is C E G B.

Because every note it plays is a chord tone, an arpeggio fits wherever its chord fits. That's why it's an easy way to fill a song: hold the chord of the bar and its notes agree with the harmony. They clash when the chord you hold isn't the one the rest of the song plays; the Your track page checks that against the chord track.

Which notes you hold, and where, is called the voicing, and it changes the sound. C4 E4 G4 ripples close together; C3 G3 E4, spread over more than an octave, sounds wider and calmer. A note more makes the cycle longer: a 4-note chord in Up comes round every 4 steps, a 3-note one every 3.

The picture shows the keys you hold, each named by its job in the chord (R for the root, then 3, 5 and 7), and the note playing now, lit.

**Note order** — Up, down, and the patterns in between

The order is the path the arpeggiator takes through the notes you hold. With C4 E4 G4 held:

- Up: C E G, C E G. The classic climbing arpeggio.
- Down: G E C, G E C. Falling, often more relaxed.
- Up and down: C E G E, C E G E. Up, then back down without playing the top and the bottom twice, so 3 notes come round every 4 steps.
- Down and up: G E C E, G E C E.
- Outside in: C G E. The lowest, the highest, then inwards. With four notes, C E G B, it plays C B E G.
- As played: in the order you pressed the keys. Press E, then C, then G, and it plays E C G. Notes of a clip that start together count from low to high.
- Alberti: low, high, middle, high: C G E G. It's named after Domenico Alberti, an Italian composer of the 18th century who used it a great deal, though he didn't invent it. It's the left hand at the start of Mozart's Piano Sonata in C, K. 545.
- Random: one of the held notes, picked by chance for each step; the same note can come twice. DawIt's chance is the same each time the song plays the same bar, so a part you like stays the way it is.

The order starts again from its first note when you play a new chord after letting go of the last one. Add or lift a note while others stay held and it carries on where it was.

Up and Down sound like a direction; Up and down, Outside in and Alberti like a figure that circles. Try one chord in each: the harmony stays the same, only the shape of the line changes.

**Octaves** — Spreading the chord over more of the keyboard

Octaves sets how many octaves the arpeggio climbs through. At 1 it plays only the notes you hold. At 2 it plays them, then the same notes 12 semitones (an octave) higher: C4 E4 G4 C5 E5 G5. At 3 it adds another octave, up to G6.

A note an octave higher has the same name at twice the frequency: A4 is 440 Hz, A5 880 Hz. So the harmony doesn't change, only the range. More octaves make a longer, sweeping line: 3 notes over 2 octaves come round every 6 steps, over 3 octaves every 9.

More range isn't always better. A line that climbs into the top of the keyboard gets thin and piercing, and one that dives low gets muddy and gets in the way of the bass. 1 or 2 octaves is the usual choice, 3 or 4 for big sweeping trance or film arpeggios.

Hold one note and set 2 octaves, and it jumps between the note and its octave, C3 C4 C3 C4: the octave bounce of disco and synth-pop basslines.

**Rate and note length** — How fast it steps, and how long each note lasts

Rate is the length of a step, as a note value, the same as in the piano roll: 1/4 is one beat, 1/8 half a beat, 1/16 a quarter of a beat and 1/32 an eighth of a beat. Triplets fit three in the time of two: 1/8T is a third of a beat, 1/16T a sixth.

At 120 BPM a beat lasts 0.5 seconds, so 1/16 steps come every 125 ms, 8 notes a second. At 140 BPM 1/16 is 9.3 notes a second and 1/32 is 18.7. The rate follows the song's tempo, so the arpeggio stays on the beat when the tempo changes.

Gate is how long each note lasts, as a share of its step. At 100% a note lasts until the next one starts: joined notes, called legato. At 50% it lasts half the step and leaves a gap: short, detached notes, called staccato, which keep a fast pattern clear. Above 100% the notes overlap, each still sounding when the next starts, like harp strings ringing on or a pianist holding the sustain pedal. An instrument that plays one note at a time may glide from one to the next instead, depending on how it's set.

How long you hear a note also depends on the instrument. A pluck that dies away in 200 ms sounds short at any gate; a pad with a slow release goes on sounding after the note ends. If the gate seems to do nothing, look at the instrument's release.

**Chords, bass notes and rests** — The steps of the pattern

Each step of the pattern says what to play when its turn comes:

- Note: the next note of the order.
- Chord: the whole chord you hold, at once. A pattern of only Chord steps turns a held chord into a rhythm of block chords; other arpeggiators call this a chord trigger.
- Bass: the lowest note you hold, an octave lower, as a pianist's left hand would play it.
- Rest: nothing. A gap is rhythm too.
- Tie: the step before goes on sounding through this one, for a longer note.

Chord, Bass, Rest and Tie don't move the order on: Note, Chord, Note plays the order's first note, the chord, then its second note.

Mixing them is how an arpeggiator plays like a person. Bass, Note, Note, Note in eighths is a ballad piano's left hand. Bass, Chord, Bass, Chord in quarters is the oom-pah of stride piano or a march. Rest, Chord, Rest, Chord in eighths puts the chord on the off-beats, as in ska and house. Chord, Tie, then notes is a hit followed by a ripple.

Steps sets how many steps the pattern has before it starts again, from 1 to 16. It counts its steps from the start of the song, so it stays in place when you play from the middle or loop a section. 16 steps of 1/16, 12 of 1/8T, 8 of 1/8 or 4 of 1/4 fill exactly one bar, so step 1 is always on a bar line; other lengths cross the bar line (see Patterns that cross the bar).

**Accents, swing and syncopation** — Where the weight falls

An accented step plays louder: its velocity goes up by the Accent amount (20 at the start, on MIDI's scale of 1 to 127). Shift-click a step to accent it. What louder means is up to the instrument: most get louder, many also brighter.

Accents say where the weight of the bar is. In 4/4 the beats, 1, 2, 3 and 4, are the strong places, and 1 the strongest. Accent steps 1, 5, 9 and 13 of a 1/16 pattern and the arpeggio marks the beat, as trance arpeggios do.

Syncopation puts the weight where it isn't expected: on the off-beats, the "and"s between the beats, or on a weak beat while the strong one stays silent. A rest on beat 1 with a chord just after it, or an accent on the last sixteenth before a beat, pushes the rhythm forward. Dance producers count it among the things that make a track move.

The best-known syncopated pattern is the 3-3-2. Of the 8 eighth notes in a bar, play the 1st, 4th and 7th: on 1, on the "and" of 2, and on 4. The middle hit falls between the beats, and the gaps are 3, 3 and 2 eighths long. It's the tresillo of Cuban music, and pop and EDM drops use it everywhere. The 3-3-2 stabs starting point plays it.

Swing delays every second step, so pairs of steps become long-short. At 50% both steps are the same length: straight. At about 67% the first of each pair takes two thirds of the time and the second falls on a triplet: a full shuffle. Roger Linn, who designed the swing of the Linn and MPC drum machines, describes small settings such as 54% as loosening the feel without it sounding like swing. Swing works on pairs of steps, so at 1/16 it swings the sixteenths, at 1/8 the eighths.

**Patterns that cross the bar** — When the notes and the steps don't line up

Hold three notes, set Up and 16 steps of 1/16, and listen to where the C falls. The notes come round every 3 steps and the bar every 16, so the C is on steps 1, 4, 7, 10, 13 and 16, and in the next bar on step 3. The line shifts against the beat, which makes even a plain arpeggio sound restless.

They meet again after the smallest number of steps that both 3 and 16 divide: 48 steps, 3 bars of 1/16. With four notes the cycle is 4, which divides 16, so the C is on every beat and the arpeggio sounds square and settled.

The pattern's own length does the same. A pattern of 6 steps of 1/16 is a beat and a half long, so it starts at a different place in each bar, and lines up with the bar again only after 12 beats: 8 patterns, 3 bars. Cycles of different lengths over the same steady pulse are called polymetre.

Neither is wrong. Lined-up patterns sound steady; crossing ones restless and hypnotic. That's an opinion, but a common one. To make a crossing pattern line up, hold one note more or less, or choose Up and down: 3 notes then come round every 4 steps.

**How a pattern fits the song** — Chord changes, range and space

An arpeggio is one part of the arrangement, so it shares the song with everything else.

Follow the chords. The arpeggiator plays what you hold, so when the song moves from C to Am, hold Am. With a MIDI clip on the track, write the chord track's chords into it as block chords; the arpeggiator turns each into the pattern. Change chord on a bar line or a beat: the order starts again on the new chord, so a change right on step 1 sounds cleanest.

Give it a range of its own. Busy parts in the same range get in each other's way. If the vocal sits around C4 to C5, an arpeggio an octave above or below leaves it room. Keep fast arpeggios out of the bass's range, below about C3, where quick notes tend to blur.

Leave space. A fast arpeggio fills every gap. Many producers bring one in for the build-up or the chorus and leave it out of the verse, or make it sparser there with rests or slower steps. That's taste, not a rule.

Velocity and length change it as much as speed. The same 1/16 pattern sounds gentle with a 50% gate on a soft pluck, and hard at 100% on a bright saw wave.

DawIt Arpeggiator or Play chords as...? Both turn chords into patterns. Play chords as... on the chord track writes the notes into a clip you can edit one by one. The arpeggiator plays them live: change the order or the rate while the song plays, and every chord you hold follows.

**How DawIt Arpeggiator works** — Before the instrument, on the song's clock

1. It's a MIDI effect. On a MIDI track it sits before the instrument, first in the device bar. The notes of the clips, your MIDI keyboard and the computer keyboard go into it, and what comes out goes to the instrument: the DawIt Synth, a built-in instrument or a plugin. It only goes on MIDI tracks, because an audio track has no notes.

2. It follows the song's clock. DawIt tells it the tempo and where the playhead is for every block of sound, and it counts its steps from the start of the song. While the song is stopped and you play, it runs a clock of its own at the song's tempo, starting at step 1 with your first chord.

3. A new chord starts a step. Notes held together make the chord. When a chord arrives a little after a step began, up to a quarter of a step late, that step plays at once, so a chord played or recorded slightly late isn't skipped.

4. It ends every note it starts: when you let go, stop, jump to another place or switch it off. Latch keeps the chord going after you let go, until you play the next one. The sustain pedal holds the keys you let go of while it's down.

5. Recording keeps what you play, not what it plays. A take holds your chords, and the arpeggiator turns them into the pattern each time the song plays, so you can still change the pattern, the rate or the order afterwards. For a written-out part, use Play chords as... on the chord track; for audio, bounce the track.

6. Switched off, it lets the notes through unchanged: you hear the chord you hold.

**Your track** — What it played on this track

This page looks at the last 2 bars the arpeggiator played on this track, and at the chord track, and says what to try. (Then the advice lines below.)

### Tooltips
- Rate: "How long a step lasts, as a note value: 1/16 is four steps a beat. Follows the song's tempo."
- Order: "The path through the notes you hold: Up, Down, Up and down, Down and up, Outside in, As played, Alberti or Random."
- Octaves: "How many octaves it climbs through: 1 plays the notes you hold, 2 adds them an octave higher."
- Gate: "How long each note lasts, as a share of its step: 50% short and detached, 100% joined, above 100% overlapping."
- Swing: "Delays every second step: 50% is straight, about 67% a full triplet shuffle."
- Steps: "How many steps the pattern has before it starts again. 16 steps of 1/16 fill one bar."
- Accent: "How much louder an accented step plays, in MIDI velocity (1 to 127). Shift-click a step to accent it."
- Latch: "Keep playing the chord after you let go of the keys, until you play the next one."
- Test chord: "Play the chord track's chord of this bar (C major where there's none) through the arpeggiator for 2 bars."
- Keyboard picture: "The keys you hold, named by their place in the chord, and the note playing now."
- Notes picture: "The last 2 bars of notes it played, against the song's beats."
- Brushes: Note "The next note of the order"; Chord "The whole chord at once"; Bass "The lowest held note, an octave lower"; Rest "Nothing: a gap"; Tie "The step before goes on sounding".
- Order menu: Up "Lowest to highest"; Down "Highest to lowest"; Up and down "Up, then back, without repeating the ends"; Down and up "Down, then back up"; Outside in "Lowest, highest, then inwards"; As played "The order you pressed the keys"; Alberti "Low, high, middle, high"; Random "Any held note, the same each time the bar plays".

### Starting points (Order, Octaves, Rate, Gate, Swing, Steps and pattern; N Note, C Chord, B Bass, R Rest, T Tie, ' accented)
1. **Up** — Up, 1, 1/16, 80%, 50%, 16 × N. "The plain arpeggio: each note of the chord in turn, from the bottom up, four a beat. Start here and change one thing at a time."
2. **Trance sixteenths** — Up, 2, 1/16, 50%, 50%, 16 × N, steps 1 5 9 13 accented. "Short sixteenths climbing two octaves, louder on each beat: the driving trance arpeggio. The 50% gate keeps the fast notes apart."
3. **Up and down** — Up and down, 2, 1/16, 80%, 50%, 16 × N. "Up two octaves and back without repeating the ends: a rolling line that circles round the chord."
4. **Falling eighths** — Down, 1, 1/8, 90%, 54%, 8 × N. "Down the chord in eighths, slightly loosened with 54% swing: relaxed, for synthwave and lo-fi."
5. **Alberti bass** — Alberti, 1, 1/8, 100%, 50%, 8 × N. "Low, high, middle, high, as in the left hand of Mozart's Sonata K. 545. Hold a chord in the lower middle of the keyboard."
6. **Ballad: bass and broken chord** — Up, 2, 1/8, 150%, 50%, 8: B N N N N N N N. "The bass note, then the chord rising in eighths, each note ringing into the next as with the sustain pedal."
7. **Chord on every beat** — Up, 1, 1/4, 90%, 50%, 4 × C, step 1 accented. "The chord trigger: the whole chord on each beat, the first one strongest. A pulse for verses and sketches."
8. **Off-beat stabs** — Up, 1, 1/8, 40%, 50%, 8: R C R C R C R C. "Short chords between the beats, as in ska, reggae and house: the off-beats carry the weight."
9. **3-3-2 stabs** — Up, 1, 1/8, 60%, 50%, 8: C' R R C R R C R. "Three hits a bar on 1, the and of 2, and 4: the syncopated 3-3-2 of pop and EDM drops."
10. **Stride: bass and chord** — Up, 1, 1/4, 70%, 50%, 4: B C B C. "Bass on 1 and 3, the chord on 2 and 4: the oom-pah of stride piano and marches."
11. **Hit and ripple** — Up, 2, 1/16, 80%, 50%, 8: C' T N N N N N N. "The whole chord, held for a moment, then its notes ripple upwards: chords and single notes in one pattern."
12. **Triplet waves** — Up and down, 2, 1/8T, 80%, 50%, 12 × N. "Three notes a beat, up and down two octaves: rolls against straight drums, for film and ballads."
13. **Octave bounce** — Up, 2, 1/8, 60%, 50%, 8 × N, steps 1 3 5 7 accented. "Hold a single note: it jumps between it and its octave, the disco and synth-pop bass trick."
14. **Random sparkle** — Random, 2, 1/16, 40%, 50%, 16 × N. "Short notes picked by chance from the chord over two octaves: always chord tones, and the same each time the bar plays."
15. **Outside in** — Outside in, 1, 1/16, 70%, 50%, 16 × N. "Lowest, highest, then inwards: a figure that circles the chord instead of climbing it."

All have Accent 20 and Latch off.

### Example data for the pictures
- C4 E4 G4 = 60 64 67. Up 1 oct: 60 64 67; 2 oct: 60 64 67 72 76 79; 3 oct ends at 91 (G6).
- Up and down 1 oct: 60 64 67 64 (cycle 4); 2 oct: 60 64 67 72 76 79 76 72 67 64 (cycle 10).
- Down: 67 64 60. Down and up: 67 64 60 64. Outside in: 60 67 64; C E G B: 60 71 64 67. Alberti: 60 67 64 67. As played (E, C, G): 64 60 67.
- Bass step on C4 E4 G4: 48.
- 3-3-2: eighths 1, 4, 7 of 8 = beats 0, 1.5, 3.
- Swing pairs at 120 BPM, 1/16: second step at 0.25 beat (50 %), 0.27 (54 %), 0.333 (66.7 %): 125, 135, 166.7 ms.
- Crossing: C of 3-note Up on steps 1, 4, 7, 10, 13, 16, then 3 of the next bar; lines up after 48 steps.
- Test chord: Am (bits A C E) → C4 E4 A4 (60 64 69); none → 60 64 67.

### "Your track" rules (last 2 bars of step records)
1. **Nothing held** (no step played a note): "Nothing reached the arpeggiator in the last 2 bars. Hold a chord on your keyboard, or put a MIDI clip of chords on this track, and play the song." Ends there.
2. **Chord fit**, over steps in bars with a chord on the chord track (at least 4 notes): more than 25 % of played notes outside the bar's chord: "{x} of the {y} notes it played aren't in the chord track's chord (bar {n}: you hold {held}, the chord track has {chord}). Notes outside the chord can add colour or clash. If it sounds muddy, hold the chord track's chord, or change the chord track." At most 25 % (and some notes outside): "Most notes it played are in the chord track's chords; the {x} that aren't are passing colour." None outside: "Every note it played is in the chord track's chord of its bar, so the arpeggio follows the harmony." Quiet without a chord track.
3. **Crossing** (order not Random, pattern has Note steps): with cycle n and k Note steps a pattern, if n doesn't divide k: "Your {n}-note cycle on a pattern with {k} Note steps comes round at a different step each time; it lines up again every {patterns} patterns ({bars} bars). Hold {n±1} notes or try Up and down to line it up, or keep it for the restless feel." Quiet when it divides.
4. **Fast** (notes a second above 12, from Note and Chord steps at the song tempo): "At {bpm} BPM, {rate} steps are {x} notes a second, too fast to follow one by one; they blur into a shimmer. If you want notes you can follow, try {rate×2} ({y} a second)." Quiet at or below 12.
5. **Low and fast** (lowest played note below C3 = 48, and steps 1/16 or shorter): "The arpeggio goes down to {note}. Fast low notes tend to sound muddy and get in the way of the bass; try it an octave higher, or Octaves 1." Quiet otherwise.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Sequence Up, C E G, 2 oct | 60 64 67 72 76 79 | exact |
| Up and down cycle, 3 notes 1 oct / 2 oct | 4 / 10 | exact |
| Outside in C E G B | 60 71 64 67 | exact |
| Alberti C E G | 60 67 64 67 | exact |
| As played E C G | 64 60 67 | exact |
| Bass step on C4 E4 G4 | 48 | exact |
| Notes > 127 dropped (G9 held, 2 oct) | no note > 127 | exact |
| Step start, 120 BPM, 48 kHz, 1/16 | every 6000 samples | ±1 sample |
| Gate 50 % at 1/16, 120 BPM | note-off 3000 samples after on | ±1 |
| Gate 150 % | next note starts before the previous ends; same pitch ended first | exact |
| Swing 66.7 % / 75 %, 1/16, 120 BPM | second step at 8000 / 9000 samples | ±1 |
| Chord step | all held notes on at one sample | exact |
| Tie after Note at 80 % | one note lasting 1.8 steps (10 800 samples) | ±1 |
| Accent 20 on velocity 100 / 120 | 120 / 127 | exact |
| Note index after Note, Chord, Note | first, chord, second | exact |
| New chord after all released | order restarts at the first note | exact |
| Chord 20 ms into a 125 ms step | that step plays at arrival | ±1 sample |
| Chord 60 % into a step | waits for the next step | exact |
| Step index at ppq 1.0, 1/16, 16 steps | step 5 (index 4) | exact |
| Random: same bar played twice | same notes; all from the held set | exact |
| Flush, stop, seek, bypass, removal | no note left sounding | exact |
| Latch on, keys released | pattern continues; next chord replaces it | exact |
| Crossing 3 vs 16 / 4 vs 16 | 3 patterns, 3 bars / lines up | exact |
| Notes a second, 140 BPM 1/32 / 1/16 | 18.67 / 9.33 | ±0.01 |
| Pattern text round trip | identical settings | exact |
| Guide | 11 pages, each with text; every page but *Your track* has an example; search "tresillo", "alberti" and "arp" find the guide | exact |

## Steps

Group A1, engine: MIDI effect host support (`./build.sh --test` at step 2):
- [x] 1. MIDI effects in the chain: the slot knows it's a MIDI effect; the order invariant in `addPlugin`/`movePlugin`; refusal on audio tracks, buses, returns; `Track::process` runs MIDI effects before the built-in synth and instrument (also in `instrumentOnly` bounces), bypassed ones through `processBlockBypassed`; removal and bypass release the instrument's notes. Tests: with a test-only MIDI effect in `tests/` that transposes by +12, a note 60 on a built-in-synth track and on a plugin-instrument track arrives as 72; order after add, move and project round trip; refusal; bypass passes 60.
- [x] 2. Audit every `isInstrument` site (see *Code*) so MIDI effects don't count as audio stages: latency, gain staging, metering, `SignalFlow`, `RoutingGuide`. Tests: a MIDI effect adds no latency and no gain-staging stage; Signal Flow and routing guide checks unchanged. Done when the group's build passes.

Group A2, engine: the arpeggiator (`./build.sh --test` at step 5):
- [x] 3. `arp::` pure functions and the starting points (*Code*). Tests: the sequence, swing, gate, tie, accent, crossing, notes-a-second and pattern-text rows of *Test numbers*; 15 presets with names and whys.
- [x] 4. `BuiltInArpeggiator` processor: parameters, held notes, clock, latch, pedal, flush, bypass, late-chord catch, test chord and page examples, step records, `SongContextReceiver`; registered in `BuiltInFormat` (MIDI-effects list) and `dawit_add_builtin … APP_ONLY`. Tests: the timing, chord, index, random, flush and latch rows, driven through a track with the fake device; `checkThroughProject<BuiltInArpeggiator>`; registry test sees it.
- [x] 5. `arp::analyse` for *Your track*, the five rules. Tests: a made-up record for each rule, and quiet cases.

Group B1, UI: the window and the chain (`./build.sh --test` at step 8):
- [x] 6. Add menus (`plugins::showAddMenu`): the *Before the instrument* section on MIDI tracks only, inserting before the instrument. Device bar chip left of the instrument, drag limits, chip picture (`chainpictures`); inspector row above the instrument. Check: shot of the device bar and inspector on Keys.
- [x] 7. `ArpeggiatorEditor` (*What the user does and sees*): knobs, Order menu, keyboard picture with chord name (named as *Detect key and chords* does, `MidiHarmony`), step row copied from `SlicerPatternView`, last-2-bars notes view, Test chord, starting points; `createEditorFunction` in `MainComponent`.
- [x] 8. Tooltips and explainer texts from *Theory*. Check: window shot with *Hit and ripple* playing the test chord.

Group B2, UI: the guide (`./build.sh --test` at step 10):
- [x] 9. `ArpeggiatorGuide` pages and pictures (*The guide*), page examples, *Your track* from `arp::analyse`, `setPrintSource`.
- [x] 10. `gen::guides` entry, `MainComponent::openGuide` case, `BuiltInGuides` branches; fix `RoutingGuide`'s chain texts ("the instrument first on a MIDI track" and "the instrument that plays its MIDI notes") to say MIDI effects such as DawIt Arpeggiator come before the instrument. Tests: `runTheoryGuidesTests` count and the search words; `--export-guides` passes.

Group C, check and docs:
- [x] 11. Real-app check: shots on the pop song: Keys with `addPlugins: ["DawIt Arpeggiator"]`, its window with preset *3-3-2 stabs* and `clap` (wait 1500) capturing the window; the device bar and inspector on Keys; the guide at *Note order*, *Patterns that cross the bar* and *Your track* while playing at bar 9. Look for: chip order, lit step, held keys and chord name, notes view filling, no cut-off text, *Your track* advice matching the song.
- [x] 12. Docs: README Features bullet; `docs/code-layout.md` (`BuiltInArpeggiator`, `ArpeggiatorEditor`, `ArpeggiatorGuide`); `docs/tests.md` (MIDI effect host, arpeggiator); `docs/architecture.md` (*The edit*'s plugins and *Built-in effects*: MIDI effects first, run before the synth); `docs/recipes.md` (*A built-in effect or instrument*: a MIDI effect's differences); `docs/ui-design.md` (chip order); issue #84 progress, and a follow-up issue for the plugin build and *turn into notes* if chosen; note in #85 that MIDI effect hosting exists.

## Tests
In `tests/EngineTests.cpp`: `runMidiEffectHostTests` (step 1–2 checks) and `runArpeggiatorTests` (every row of *Test numbers*, the five *Your track* rules and their quiet cases, the starting points' names and whys, project round trip with chain order).

## Website hand-off
- `features.html`: the MIDI or "make music" group, near Play chords as..., with a window shot (pop song, Keys, *3-3-2 stabs*, test chord playing).
- `learn.html`: "What is an arpeggiator?" in the MIDI area, with the *Patterns that cross the bar* picture.
- Home and `for-you.html`: not needed. `about.html`: nothing (no library).

## Left for later
- DawIt Arpeggiator as an AU/VST3 plugin (decision 1).
- Listing third-party MIDI effects in the add menu (decision 2).
- Writing the arpeggio into a MIDI clip (decision 3).
- Following the chord track without held notes (a "Chord track" source): needs the chord's root and voicing, not only its pitch classes.
- Velocity patterns beyond on/off accents, per-step octave or transpose, pattern offset, and Ableton-style repeats: more controls than a beginner needs at first.
- A link from the styles guide and *Play chords as...* to the arpeggiator guide.

## Changes while building

**Group A1 hand-off (MIDI effect host, engine side done)**
- `PluginSlot::isMidiEffect` (set in `Engine::addPlugin` from `instance->isMidiEffect()`, never for instruments) and `PluginSlot::isAudioEffect()` (neither instrument nor MIDI effect); `Track::getNumMidiEffects()`. Use `isAudioEffect()` wherever a site means "an audio effect".
- `Engine::addPlugin` refuses a MIDI effect off MIDI tracks ("A MIDI effect changes notes: put it on a MIDI track."), inserts it after the existing MIDI effects whatever the index, and never inserts another plugin before them (the UI's `description.isInstrument ? 0 : -1` still works). `movePlugin` clamps both ways. Removing or bypassing a MIDI effect bumps `midiEditVersion`. Because the instance is created before the check, the description alone can't tell a MIDI effect: the add menu must offer only DawIt's own MIDI effects on MIDI tracks (`BuiltInFormat` list, step 4 and 6).
- `Track::runMidiEffects` runs after clip, live, preview and flush events and before the built-in synth; a bypassed one goes through `processBlockBypassed`. `runPlugins` skips MIDI effects (so also no metering). Latency (`getEffectsLatency`), `SignalFlow` effect nodes and `RoutingGuide` stages/`firstEffectIndex` use `isAudioEffect()`.
- Tests: `runMidiEffectHostTests` with `TestMidiPlugin`/`TestMidiFormat` in `tests/EngineTests.cpp` (a transposing MIDI effect and a recording instrument, format "TestMidi"); copy it for the arpeggiator's chain tests.
- Left for Group B1 (UI sites, they need the new chip layout): `DeviceBar.cpp` lines using `isInstrument` (chip stop kind, drag/power/remove, stage energy at ~341, the instrument count ~1489), `InspectorPanel.cpp` rows (~59-170, 528-532, 899-904), `PluginActions.cpp` (add menu filter `d.isInstrument != instruments`, ~41-113), `TrackHeader.cpp` add menu and FX button text. Until then a MIDI effect would show as an effect row there, but none can be added from the menus yet.

**Group A2 hand-off (the arpeggiator engine, done; `./build.sh --test` passes, 99 new checks)**
- Files: `src/engine/BuiltInArpeggiator.{h,cpp}` (namespace `daw::arp` and the processor `daw::BuiltInArpeggiator`), `tests/ArpeggiatorTests.h` (included by `EngineTests.cpp` after the MIDI effect host tests; `runArpeggiatorTests`, `runArpeggiatorAnalysisTests`). Registered with `dawit_add_builtin (Arpeggiator ... APP_ONLY)`, code `Darp`, no `UI_SOURCES` yet: **Group B1 must add `src/ui/ArpeggiatorEditor.cpp` (and B2 `ArpeggiatorGuide.cpp`) to `UI_SOURCES` in CMakeLists.txt**, and set `BuiltInArpeggiator::createEditorFunction` in `MainComponent`.
- `BuiltInFormat::getMidiEffects()` lists it (not in `getEffects()`): the add menu's *Before the instrument* section reads this on MIDI tracks only. The plugin registry test now expects two app-only built-ins (Tune, Arpeggiator) and has a MIDI-effect branch for the bus checks.
- `arp::Settings` (order, octaves, rate index into `stepgrid::stepLengths`, gatePercent, swingPercent, steps, accent, latch, `kinds[16]`, `accents[16]`); `arp::Knob`, `getOrderName/getOrderHint/getKindName/getKindHint/getKindLetter` (tooltip texts from the plan); `arp::getPresets()` (15, "Up" first); `patternToText/patternFromText`; `getSequence/buildSequence`, `getCycleLength`, `getStepStartBeats`, `getNoteEndBeats`, `getTiedSteps`, `getAccentedVelocity`, `getBassPitch`, `getNotesPerSecond`, `getCrossingPatterns`, `getBarRealignPatterns` (for the *Patterns that cross the bar* and *How a pattern fits* pictures), `nameChord (pitchClassBits, lowestPitchClass)` (for the chord name above the keyboard; pass -1 for the lowest to try every root).
- Processor: `getSettings/setSettings/setKnob/setKind/setAccented`, `getKnobParameter/getKindParameter/getAccentParameter`; `playTestChord()` (any thread), `playExample (arp::Example)` (message thread; an `Example` is settings + up to 16 pitches + length in beats, the parameters are untouched), `stopExample()`, `isExamplePlaying()`. For pictures: atomics `playingStep`, `clockBeat`, `usingSongClock`, `tempoBpm`, `getUsedBpm()`, `heldKeys[2]` and `soundingNotes[2]` (bit p % 64 of word p / 64), `chordPitchClasses` (the bar's chord, 0 for none), and `history.getRecent (StepRecord*, n)` (one record per step while keys are held, rests and ties too: step, kind, played pitches, held mask, chord, beat; `clear()` on the message thread). `arp::analyse ({settings, records of the last 2 bars, bpm})` gives the *Your track* lines; the UI builds the records from `history` (records whose `beat` is within 2 bars of `clockBeat`).
- A MIDI effect has no audio buses (`isMidiEffect()`); while a test chord or example plays, live keys and pedal are ignored and the example's settings are used; when it ends, keys still held are forgotten until pressed again.
- Departures from the plan (all text and numbers of *Theory* are used as written):
  - Swing's parameter step is 0.01 (not 0.1) so the triplet shuffle can be set to 66.67 and land on 8000 samples; the starting points use 50 and 54 only.
  - A note's gate is a share of the *actual* length of its step (after swing), so at 100 % notes still join under swing; with no swing it is the plan's number.
  - Alberti runs per octave: low, high, middle, high over the held notes, then again an octave up (a lone key plays itself; two keys low, high, low, high). Cycle 4 per octave.
  - *Your track* rule 3 says "Hold {m-1} or {m+1} notes" with m the number of held keys (not the cycle length n), which stays right with 2 or more octaves; one held key says "Hold 2 notes". "Or try Up and down" is left out when the order already is Up and down. The bars are shown with up to two decimals ("0.75 bars"), "1 bar" for one.
  - Rule 4's "try {rate x 2}" is the next longer rate when there isn't a double (1/8T goes to 1/8).
  - Random's pick uses a hash of the song's absolute step number, so it needs no stored state.

**Group B1 hand-off (menus, chain UI and the window; `./build.sh --test` passes, no warnings)**
- Add menus: `plugins::showAddMenu` (effect menu, MIDI tracks only) starts with a *Before the instrument* section from `BuiltInFormat::getMidiEffects()`; `TrackHeader`'s FX menu has an "Add MIDI effect (before the instrument)" submenu. `plugins::add` is unchanged (the engine inserts it first).
- Device bar (`DeviceBar.cpp`): MIDI effect chips are built before the synth/instrument chip; the chip has no meters or badge (`Chip::isMidiEffect`, `getMeterWidth`), label "MIDI EFFECT", stop kind `instrument` with new `gainstaging::Stop::isMidiEffect` (`findHints` takes the first stop after MIDI effects as the source); dragging is limited to the same group (MIDI effects among themselves, audio effects after them); adding a MIDI effect scrolls the bar to the start. Inspector rows: MIDI effects first, then the instrument, then effects; drag limited to the group.
- Chip picture: `chainpictures::paintCurve` draws the step row (bar per step by kind, accent taller, playing step white); also used by Signal Flow's `hasPicture`.
- New files: `src/ui/ArpeggiatorGuide.{h,cpp}` (namespace `arptext`: topics, tooltip and explainer texts, `getKindColour`, `layoutSteps/drawSteps/getStepDescription`, `getKeyboardState/getHeldChordName/drawKeyboard`, `getLastBars/drawLastBars`, and `makeGuide`) and `src/ui/ArpeggiatorEditor.{h,cpp}` (`BuiltInArpeggiatorEditor`, `ArpeggiatorStepView`, keyboard and notes views); both in `UI_SOURCES`, editor set in `MainComponent`.
- **Group B2 must replace `arptext::makeGuide`**, which is a one-page placeholder (page 1's first paragraph with the live keyboard); the editor's `?`/guide button and the step view's right-click menu already call `openGuide()`. The frame's `EffectEditor` Guide still shows its own clap button on pages unless a page sets `noClap` (set it on all pages; the examples use `Page::ownClap` with `BuiltInArpeggiator::playExample`). The window's clap button is the Test chord button; the listen button is hidden (the frame needs a `std::atomic<bool>` and `fx::Clap`, so the editor passes unused statics).
- `DevScreenshots` has an Arpeggiator branch (`"preset"` by name, `"clap": true` presses Test chord), and `addPlugins` now also finds MIDI effects.
- Departures: none from *Theory*. Texts I wrote that aren't in *Theory* (explainer paragraphs for the knobs, keyboard, step row, notes view and the overview, the step tooltip "Step 3 of 16 · Chord · accented. ...", the menu line "turns held chords into patterns") only reuse its facts and numbers. Value texts "80%", "66.67%" and "+20" are set on the knobs in the editor.

**Group B2 hand-off (the guide; `./build.sh --test` passes, no warnings; `--export-guides` writes "What is an arpeggiator.pdf")**
- `arptext::makeGuide` has the 11 pages with the texts of *Theory* as written (generated from the plan), a picture per page (`drawRoll`, `drawDots`, `simulate` in `ArpeggiatorGuide.cpp` work out the notes with the `arp::` functions), a listening example on pages 1 to 10 (`addExample`: `noClap`, `ownClap` with `playExample`, button "Hear this"; with no host track the button does nothing and its tooltip says "Put DawIt Arpeggiator on a MIDI track to hear this"), and *Your track* with `arp::analyse` as `liveText`. `setPrintSource` is done by the frame and `BuiltInGuides::getPrintGuide`.
- `Guide::arpeggiator` is last in `gen::guides::allGuides` (so the slicer/step-filter order test stays), area MIDI, plugin "DawIt Arpeggiator"; `MainComponent::openGuide` and `getPrintGuide` send it to `builtInGuides`, `BuiltInGuides` also lists `BuiltInFormat::getMidiEffects()` and makes the pages; `ArrangementView` and `DevScreenshots` (`"guide": "arpeggiator"`) have the case. Tests: the hub test counts MIDI effects as built-ins; new checks for the entry and the search words "arpeggiator", "arpeggio", "alberti", "tresillo".
- `RoutingGuide` chain texts now say MIDI effects such as DawIt Arpeggiator come before the instrument.
- Departures: page 9's picture uses a made-up progression (C, Am, F, G) because a guide can't see the chord track; the real check is *Your track*. Pages with a Hear this example use their own settings (page 3, 5, 6 and 10 take the window's order, rate and gate, or pattern). Example pitches C4 E4 G4 (Am7, A3 C4 E4 for pages 2 and 9) are mine; the text and numbers of *Theory* are unchanged. "Play the track, or hold a chord, and the picture follows." is my line on *Your track*.
- Shots looked at (scratchpad `shots/g1,g3,g7,g9,g11.png`, Keys track with the arpeggiator, pop song): pictures fit, no cut-off text. Group C's shot list can reuse `{"guide": "arpeggiator", "topic": N, "track": "Keys"}`.

**Group C hand-off (real-app check and docs; `./build.sh --test` ends with All checks passed, no warnings)**
- Shots looked at (scratchpad `shots/`): `c1.png` (window, *3-3-2 stabs*, Test chord playing: Am named, keys 3, 5 and R lit, step row with the lit step, notes view filling with Bar 1 and Bar 2 labels), `c3.png` (*Hit and ripple*, two octaves, notes view full), `c2.png` (Keys: device bar chip order MIDI EFFECT, instrument, fader; inspector row), `g11.png` (*Your track* at bar 9: the 3-note cycle on 16 Note steps advice). No cut-off text; no fixes needed. The "Bar 0" labels are gone.
- Shot list for dawWebsite: `{"addPlugins": ["DawIt Arpeggiator"], "select": "Keys", "track": "Keys"}` in a first shot, then `{"track": "Keys", "plugin": "DawIt Arpeggiator", "preset": "3-3-2 stabs", "clap": true, "wait": 3000, "capture": "window"}` (the add and the Test chord press must be in separate shots), the device bar `{"select": "Keys", "track": "Keys", "area": [230, 515, 1210, 50]}`, and `{"guide": "arpeggiator", "topic": 7, "track": "Keys", "capture": "window"}` for *Patterns that cross the bar*.
- Noticed: the inspector lists a MIDI effect under its *Effects* heading (above the instrument row); acceptable, left as is.
- GitHub: #84 commented (Built, not committed; In Progress kept); follow-ups #145 (AU/VST3 build) and #146 (turn into notes) created with Feature Idea and midi labels, project status Todo, verified; #85 noted.
- Docs: README Features bullet (under *Shape and mix sounds*), `docs/code-layout.md`, `docs/tests.md`, `docs/architecture.md`, `docs/recipes.md`, `docs/ui-design.md`.

**Theory check**
- Juno-60 year confirmed: September 1982 (Wikipedia *Roland Juno-60*); modes, range 1-3 and Hold agree with Roland's specification. Added to SOURCES.md.
- Page 6 and the Steps explainer: lengths that divide the bar (8 or 4 steps of 1/16) also stay on the bar line; only other lengths cross it (the text said every other length crosses).
- Page 7: the tresillo came from African music into Cuban and Latin American music (Wikipedia *Tresillo*); "pop and EDM drops use it everywhere" softened to "common in pop and in EDM drops".
- Page 8: "Up and down: 3 notes come round every 4 steps" now says "at 1 octave" (at 2 octaves it is 10).
- Page 3: Random picks from the held notes in every octave Octaves sets, as the code does.
- *Your track* rule 5: "or Octaves 1" removed (Octaves only climbs, so it never raises the lowest note); now "try holding the chord an octave higher". Test expectation changed to match.
- Open (code, for dawImplement): rule 3 suggests "Hold {m-1} or {m+1} notes" and "try Up and down" without checking that they line up (5 notes on 16 steps suggests 6; 4 notes on 6 steps suggests 5; Up and down with 4 notes is a 6-note cycle). It should only offer counts and orders whose `getCycleLength` divides the Note steps.

- Rule 3 fixed (dawImplement): *Your track* now offers only note counts (m-1, m+1) and Up and down whose `getCycleLength` divides the Note steps, dropping a clause when nothing fits ("Keep it for the restless feel."); tests added.
