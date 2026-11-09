# Instruments in MIDI: one page per family

Status: Released
Idea: [#13 — Instruments in MIDI: one page per family](https://github.com/geertvandamme/dawit/issues/13)

## Goal
A guide, *Instruments in MIDI*, with one topic per family of instruments: what vibrates, what makes it louder, why its pitch is what it is, how it's played, its range, and how to write it in MIDI so it sounds played rather than typed. Three short physics topics (strings, tubes, drums and bars) come first so families can be compared, and *Your track* checks the selected MIDI track's notes against the instrument it's for (range, notes at once, breaths, velocity, swells). A beginner who has only ever typed notes into a piano roll learns why a flute can't play a chord and how to make strings swell.

## Decisions for you
1. **Scope of the first build.** The issue lists ten areas. Options: (a) physics topics + one topic each for keys, guitar, bass, strings, brass, woodwinds, voice, synths, drums and percussion, + orchestration basics, *What makes MIDI sound real* and *Your track* (16 topics); (b) only what nothing else covers: physics, bass, strings, brass, woodwinds, realistic MIDI, *Your track* (9 topics); (c) everything, with a page per instrument (violin and viola apart) and world instruments in depth. **Recommended:** (a): a "one page per family" guide needs all families, and keys, guitar, voice and drums are short pages that link to the guides already covering them (Chord shapes, Singing techniques, What is MIDI?, What is drum synthesis?). World percussion gets a paragraph; pitched-percussion ranges, per-instrument pages and choir writing in depth go to *Left for later*.
   Answer: (a) all families, 16 topics, as recommended.
2. **Name and key.** Options: the issue's title *Instruments in MIDI*; *How instruments work and how to write for them*; *Writing for instruments*. **Recommended:** *Instruments in MIDI*, key `instruments`: short, matches the issue, and the summary and search words carry the physics.
   Answer: *Instruments in MIDI*, key `instruments`.
3. **Area and Start here.** Options: area MIDI; Songwriting; a new area *Instruments*. **Recommended:** MIDI (next to What is MIDI?), not on *Start here* (it's long and best read a family at a time).
   Answer: Area MIDI, not on *Start here*.
4. **Which instrument *Your track* checks.** Options: guessed each time, with a dropdown in the topic to choose another, not saved (recommended); or the choice saved on the track in the project (a format change for one guide). **Recommended:** guessed, not saved: no format change, and the guess (sampler preset, then track name, then DawIt sound) is right for most tracks.
   Answer: Guessed each time with a dropdown, not saved.
5. **Which range counts.** Options: the full range a skilled player reaches (one table, from HyperPhysics and Wikipedia); a "comfortable" range as well. **Recommended:** the full range only, with the text saying top notes are hard and parts usually stay below them. Comfortable ranges disagree between sources; later.
   Answer: The full range only.
6. **Unconfirmed or disputed numbers.** The bassoon's top: HyperPhysics gives B♭5, Wikipedia E5 (A♭5 extreme) with "rarely above C5": planned with E5. The tuba: Wikipedia's "easily F1 to C4", pedal B♭0, top "at least C5": planned as B♭0 to C5 for the check, F1 to C4 in the text. Velocities for hi-hat accents (90 to 110 / 60 to 80) and ghost notes (20 to 40), the 10 to 30 ms strum spacing, the 8-second breath rule and "below about C3 keep to octaves and fifths" are rules of thumb (opinion), worded "about" and "a rule of thumb". **Recommended:** keep as written.
   Answer: Keep as written (bassoon top E5, tuba B♭0 to C5, rules of thumb worded as such).

## What the user does and sees
- **Opens from:** Theory Guides (MIDI area); a MIDI clip's right-click menu, **About writing for this instrument...** (`guideicon::addItem`, after *About keys and scales...*), opening at the guessed family's topic (or *Your track* when unknown); the end of a MIDI track's **FX** menu, the same item (`guideicon::addItem`, after a separator).
- **Window:** ui-design.md *Guide windows*, 1000 × 760, 16 topics then *Sources*. Pictures follow the selected MIDI track for *Your track*; the rest are fixed examples. A click on a note or the end of a range bar plays it with `Engine::previewNote` (the selected track's instrument, else the built-in piano); a *Play* button on topics with an example line (guitar strum, bass line, drum beat, realistic MIDI before and after, synth lead) plays it with its velocities and timing.
- ***Your track*:** a dropdown above the picture, "Instrument: Guess (Cello, from its DawIt Sampler preset)" then every instrument of the table by family; the picture is the track's notes as a pitch strip over its bars against the instrument's range band (notes outside in `meterRed`, chord moments on a one-note instrument marked), and the text from the analysis. Follows selection and edits (`ChangeListener`).
- **Tooltips:** topic rows give their summary; range bar: "Cello · C2 to E5 · 65 to 659 Hz · click an end to hear it"; picture notes: "E3 · velocity 84 · 0.05 beats overlap"; dropdown: "Which instrument Your track checks against. Guess uses the track's DawIt Sampler preset, then its name, then its DawIt sound."; menu item: "Opens Instruments in MIDI at this instrument's family."
- **Remembered:** nothing. The dropdown resets to Guess when the selected track changes.

## Data and saving
None: no project data, format change or settings key.

## The guide
New guide `gen::instruments`, key `instruments`, area MIDI, not on *Start here*. Topics (texts in *Theory*):
1. **How instruments make sound**: seven tiles, each a thing that vibrates (string, air column, reed, lips, membrane, bar, vocal folds) plus the speaker cone, with an instrument name under each.
2. **Strings and their harmonics**: a string with its first four shapes and their frequencies on A2 (110, 220, 330, 440 Hz); below, the half and two-thirds lengths of an A4 string (880, 660 Hz).
3. **Tubes: open and closed**: two 66 cm tubes, open-open and closed-open, with the standing waves that fit and harmonic bars (260, 520, 780 Hz / 130, 390, 650 Hz).
4. **Drums, bars and bells**: four rows of partial lines on one frequency axis: string harmonics 1 to 4, ideal drum skin 1 to 2.92, free bar 1, 2.76, 5.40, marimba bar 1 and 4.
5. **Keys: piano and organ**: the 88-key range with A0 and C8 in Hz; a two-handed C major chord (C2 G2 | E4 G4 C5).
6. **Guitar**: the six open strings on the keyboard; a down and an up strum of open E, 15 ms per string.
7. **Bass**: the bass's range; the line C2 G2 C3 C♯3 | D3 A2 D2 under the chords C and D.
8. **Strings**: violin, viola, cello and double bass ranges with their open strings marked.
9. **Brass**: trumpet, horn, trombone and tuba ranges; a B♭ trumpet's harmonics 2 to 8 with their cents from equal temperament.
10. **Woodwinds**: flute, oboe, clarinet, bassoon, alto and tenor saxophone ranges.
11. **Voice and choirs**: the six voice types' ranges (from `gen::singing::getVoiceTypes`, so the two guides agree).
12. **Synths and electronic instruments**: a lead line with a 2-semitone bend into a note and a glide between two overlapping notes.
13. **Drums and percussion**: one bar of a rock beat on the drum map (42, 38, 36) with velocity bars.
14. **Orchestration basics**: every family's ranges stacked on one keyboard; a four-note melody on violins with a flute an octave above.
15. **What makes MIDI sound real**: the same string line twice, typed and played, with velocity bars and the expression lane under each.
16. **Your track**: above.
Then *Sources*. Entry: name "Instruments in MIDI"; summary "How each family of instruments makes its sound, its range and how it's played, and how to write it in MIDI so it sounds real: keys, guitar, bass, strings, brass, woodwinds, voice, synths and drums."; opensFrom "A MIDI clip's right-click menu and a MIDI track's FX menu (About writing for this instrument...)"; keywords: instrument instruments orchestra orchestration family range ranges violin viola cello double bass contrabass strings bow bowing legato spiccato pizzicato tremolo divisi vibrato keyswitch articulation brass trumpet trombone horn tuba mute valve slide woodwind flute piccolo clarinet oboe bassoon saxophone sax reed breath guitar strum strumming fingerpicking palm mute bassist slap ghost note approach note root fifth pianist soundboard hammer organ choir satb backing vocals chops synth portamento glide drummer groove percussion shaker conga tabla steel pan kalimba timpani marimba xylophone membrane harmonics odd standing wave realistic humanise humanize transposing.

Related guides gain one sentence each pointing here (a step): What is a sampler? (*General MIDI* page), What is physical modelling? (*Tubes and reeds*), Singing techniques (*Voice types and range*).

## Code
Follow recipes.md *A theory guide*, copying the reading guide (`ReadingGuide`, `ReadingExplainer`: owned by `MainComponent`, takes `Engine` and `Selection`, *Your part* from the selected MIDI track).
- **Content** `src/generate/InstrumentsGuide.{h,cpp}`, `daw::gen::instruments`: topics, names, summaries, `explain`, the example data, and:
  - **the instrument table**: per instrument its family, sounding lowest and highest note, how many notes it plays at once, whether it breathes, whether it sustains (can swell), how much higher it's written, its General MIDI programs and name words (*Theory*, Ranges table);
  - **the guess**: from the hints (DawIt Sampler bank and program, track name, built-in sound or DawIt instrument) in that order; name words match longest first, so "double bass" and "bassoon" beat "bass";
  - **the analysis**: of a plain struct (notes with start in beats and seconds, length in seconds, velocity, bar; whether the clips have mod wheel, expression or volume events), the rules in *Theory*, each a paragraph.
- **Window** `src/ui/InstrumentsExplainer.{h,cpp}`, a `guideprint::Source` with `GuideSourcesTopic`; the UI turns the track into the struct (notes of all its MIDI clips, through the tempo map) and reads the sampler's `getBank`/`getProgram`.
- **Threading:** message thread only; the track's clips are read under `Engine::getLock()` and copied; the sampler's bank and program are atomics. Nothing on the audio thread changes.
- **Rules:** drums are checked against the drum map, not a pitch range; synths have no range; the guide never changes a track (CLAUDE.md: generating never changes the instrument; this doesn't generate).
- **Risk:** small; the size is in the text. `Guide` gains a case: every switch over it (`MainComponent::openGuide`, `getPrintGuide`, `TheoryGuides`) must list it.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | String harmonics are whole multiples: A2 110 → 220, 330, 440, 550 Hz | derived (f·n) | exact |
| 2 | Half the length doubles the pitch (440 → 880 Hz); two thirds gives 660 Hz | derived (f ∝ 1/L) | exact |
| 3 | Plucking at a harmonic's node removes it; at the middle, the even harmonics | physical guide's sources (derived) | exact |
| 4 | Open tube f = c/2L, closed f = c/4L, odd harmonics only; 66 cm, c 343 m/s: 259.8 and 129.9 Hz; 3rd and 5th 389.8, 649.6 Hz | derived; UNSW via physical guide's sources | exact |
| 5 | Clarinet overblows a twelfth, flute an octave; real clarinet has weak even harmonics low down | physical guide's sources | exact / typical |
| 6 | Conical bores (oboe, bassoon, sax) give all harmonics and overblow at the octave | Wikipedia *Bore*; Montagu; mozart.co.uk | exact |
| 7 | Ideal membrane modes 1, 1.593, 2.136, 2.295, 2.653, 2.917 (Bessel zeros 2.405, 3.832, 5.136, 5.520, 6.380, 7.016) | derived; VSL forum summary, Springer chapter | exact |
| 8 | Timpani: air loading pulls overtones near harmonic | Springer (Rossing) chapter, HyperPhysics | typical |
| 9 | Free bar modes 1, 2.756, 5.404 (from 4.730, 7.853, 10.996 squared ratios) | derived (beam theory) | exact |
| 10 | Marimba first overtone tuned to 4×, xylophone to 3× | HyperPhysics *xylo*, CCRMA percussion page | typical |
| 11 | A string alone radiates little; the bridge drives a soundboard or top | KTH, search results | exact (qualitative) |
| 12 | Piano A0 27.5 Hz (21) to C8 4186 Hz (108) | What is MIDI? (`midiguide`) | exact |
| 13 | Ranges in the Ranges table | HyperPhysics *orchins*; Wikipedia *Bassoon*, *Tuba*; derived for guitar, bass, sax | typical (tops vary) |
| 14 | Transpositions: B♭ clarinet, trumpet M2; horn P5; alto sax M6; tenor sax M9; guitar, bass, double bass octave; piccolo octave up | intmus *Instrument transpositions* | exact |
| 15 | Guitar tuning E2 A2 D3 G3 B3 E4; bass E1 A1 D2 G2; 5-string low B0 | tuning guide (common) | exact |
| 16 | Violin G3 D4 A4 E5, viola C3 G3 D4 A4, cello C2 G2 D3 A3, double bass E1 A1 D2 G2 | common (tuned in fifths, bass in fourths) | exact |
| 17 | Valves lower 2, 1, 3 semitones; trombone 7 positions a semitone apart | brass search (Hugill, Britannica) | exact |
| 18 | B♭ trumpet open harmonics of B♭2 116.54 Hz; 5th −13.7 cents, 7th −31.2 cents from equal temperament | derived (1200 log2) | exact |
| 19 | Keyswitches below the range, a few ticks early; legato patches mono, overlap notes; samples speak late, try 10 to 40 ms earlier | Sound On Sound *Sampled orchestra part 3* | typical |
| 20 | CC1 dynamics crossfade in orchestral libraries; CC11 for swells; bend range 2 semitones by default | MIDI guide's sources (Orchestral Tools, midi.org) | typical / exact |
| 21 | Strum: down from the low string, up from the high; about 10 to 30 ms per string | SFZ strum tutorial (20 ms), forums | opinion |
| 22 | 15 ms at 120 BPM = 0.03 beat; six strings = 75 ms | derived (15/500) | exact |
| 23 | Root, fifth, octave, chromatic approach on beat 4 | guitar-pro.com, Yamaha Hub | typical |
| 24 | GM drums channel 10; 36 kick, 38 snare, 42 closed hat, 46 open hat, 49 crash, 51 ride; map 35 to 81 | Wikipedia *General MIDI* (sampler's sources) | exact |
| 25 | GM programs: 1–8 piano, 17–24 organ, 25–32 guitar, 33–40 bass, 41 violin, 42 viola, 43 cello, 44 contrabass, 45–46 tremolo, pizzicato strings, 47 harp, 49–52 string ensembles, 53–55 choir and voice, 57 trumpet, 58 trombone, 59 tuba, 60 muted trumpet, 61 French horn, 62–64 brass sections, 65–68 soprano, alto, tenor, baritone sax, 69 oboe, 70 English horn, 71 bassoon, 72 clarinet, 73 piccolo, 74 flute, 81–96 synth lead and pad | Wikipedia *General MIDI* | exact |
| 26 | Voice types (bass E2–E4 ... soprano C4–C6) | singing guide (McKinney, Boldrey) | typical |
| 27 | Glide in mono only on overlapping notes | DawIt Subtractive (its guide) | exact for DawIt |
| 28 | Hi-hat accents ~90–110 / 60–80, ghost notes ~20–40; breath every 8 s; close chords below ~C3 muddy | rules of thumb | opinion |

### Ranges table (sounding MIDI notes; the check uses these)
| Instrument | Family | Low–high | At once | Breathes | Swells | Written higher | GM |
|---|---|---|---|---|---|---|---|
| Piano | keys | 21–108 | 10 | no | no | 0 | 1–8 |
| Organ | keys | none | 12 | no | yes | 0 | 17–24 |
| Guitar | guitar | 40–86 | 6 | no | no | 12 | 25–32 |
| Bass guitar | bass | 28–67 (5-string from 23) | 2 | no | no | 12 | 33–40 |
| Violin | strings | 55–100 | 2 | no | yes | 0 | 41 |
| Viola | strings | 48–84 | 2 | no | yes | 0 | 42 |
| Cello | strings | 36–76 | 2 | no | yes | 0 | 43 |
| Double bass | strings | 28–59 | 2 | no | yes | 12 | 44 |
| String section | strings | 28–100 | none | no | yes | 0 | 45, 46, 49–52 |
| Harp | strings | 23–104 | 8 | no | no | 0 | 47 |
| Trumpet | brass | 52–82 | 1 | yes | yes | 2 | 57, 60 |
| French horn | brass | 35–77 | 1 | yes | yes | 7 | 61 |
| Trombone | brass | 40–70 | 1 | yes | yes | 0 | 58 |
| Tuba | brass | 22–72 | 1 | yes | yes | 0 | 59 |
| Brass section | brass | 35–82 | none | yes | yes | 0 | 62–64 |
| Flute | woodwinds | 60–96 | 1 | yes | yes | 0 | 74 (73 piccolo: 74–108, written 12 lower) |
| Oboe | woodwinds | 58–89 | 1 | yes | yes | 0 | 69 |
| Clarinet | woodwinds | 50–94 | 1 | yes | yes | 2 | 72 |
| Bassoon | woodwinds | 34–76 | 1 | yes | yes | 0 | 71 |
| Alto saxophone | woodwinds | 49–80 | 1 | yes | yes | 9 | 66 |
| Tenor saxophone | woodwinds | 44–75 | 1 | yes | yes | 14 | 67 |
| Voice | voice | 40–84 | 1 | yes | yes | 0 | none |
| Choir | voice | 40–84 | none | yes | yes | 0 | 53–55 |
| Synth | synths | none | none | no | no | 0 | 81–96 |
| Drum kit | drums | 35–81 (map) | 4 | no | no | 0 | bank 128 |

Name words (longest match first): piano, keys, rhodes, organ; guitar, gtr; bass, bassline (bass guitar); double bass, contrabass, upright; violin, fiddle; viola; cello; strings, string; harp; trumpet, tpt; horn, french horn; trombone, tbn; tuba; brass; flute; piccolo; oboe; clarinet; bassoon; sax, saxophone, alto sax, tenor sax; vocal, vocals, vox, voice, singer, bv, backing; choir; synth, lead, pad, pluck; drum, drums, kick, snare, hats, hi-hat, perc, percussion. Built-in sound: piano → piano; organ → organ; bass → bass guitar; lead, pluck, pad → synth; drums, percussion → drum kit. DawIt Drums → drum kit; DawIt Subtractive, Additive, FM → synth; DawIt Physical and other plugins: no guess.

### Topic texts

**1. How instruments make sound** — *Something vibrates, something makes it louder, something shapes it*

Every acoustic instrument does three things. Something vibrates: a string, the air in a tube, a reed or the player's lips, a drum skin, a wooden or metal bar, or a singer's vocal folds. Something makes it louder: a string on its own moves hardly any air, so a guitar passes its vibration through the bridge into a wooden body, and a piano into its soundboard, a large thin board that moves far more air than a string can. And something shapes the colour: the body, the tube and the player's mouth have resonances of their own that make some frequencies stronger than others.

Electric and electronic instruments end the same way. An electric guitar's pickup turns the string's movement into a signal, an amplifier makes it bigger, and a speaker cone moves the air. A synth skips the vibrating object and makes the signal directly.

The pitch comes from how many times a second the vibration repeats, and the colour from the mix of harmonics and from how a note starts and ends (What is sound? explains both). The next three topics show why strings, tubes and drums give such different mixes.

Every family topic after them answers the same questions, so you can compare: what vibrates, what makes it louder, its range, how it's played, and how to write it in MIDI so it sounds played rather than typed. DawIt plays and shows notes as they sound; where an instrument's music is written at another pitch, the topic says so.

The picture shows each kind of thing that vibrates, with an instrument that uses it.

**2. Strings and their harmonics** — *Half the length, twice the pitch: 440 Hz becomes 880 Hz*

A string is fixed at both ends, so it can only vibrate in shapes that fit exactly between them: the whole length swinging as one, two halves swinging in opposite directions, three thirds, and so on. A real string does all of these at once. Each shape vibrates at a whole-number multiple of the lowest: an A string at 110 Hz also sounds 220, 330, 440 and 550 Hz. These are the harmonics, and their mix is a big part of the instrument's colour.

Three things set the pitch: length, tension and weight. A shorter, tighter or lighter string sounds higher. Stop a string at half its length and its pitch doubles, an octave up: 440 Hz becomes 880 Hz. Two thirds of the length gives a fifth up, 660 Hz. That's what a guitarist's frets and a violinist's fingers do. A tuning peg changes the tension, and the low strings are thicker and heavier.

Where the string is plucked, struck or bowed changes the mix. Near the middle gives a round, soft tone with few high harmonics; near the end, close to a guitar's bridge, a bright, thin one. A harmonic that has a still point (a node) where you pluck can't sound at all: pluck exactly at the middle and the even harmonics are missing. What is physical modelling? lets you hear this on DawIt Physical's string.

Piano, guitar, harp, violin, cello and bass are all string instruments. They differ in how the string is set going (a hammer, a finger, a pick, a bow) and in what makes it louder.

The picture shows a string's first four shapes with their frequencies on A2, and an A4 string stopped at half and at two thirds of its length.

**3. Tubes: open and closed** — *Why a clarinet sounds hollow, and an octave lower than a flute of the same length*

In wind and brass instruments, the air inside a tube vibrates. Something at one end keeps it going: breath blown across an edge (the flute), one cane reed (clarinet and saxophone), two reeds against each other (oboe and bassoon), or the player's lips buzzing in a mouthpiece (every brass instrument). The length of the tube sets the pitch: a longer tube sounds lower. Keys, holes, valves and a slide change how much of the tube vibrates.

The ends decide which harmonics fit. A tube open at both ends, like a flute, holds every harmonic, as a string does. A tube closed at one end, like a clarinet (the mouthpiece and reed close it), holds only the odd ones, 1, 3, 5 and so on, and its lowest note is an octave below an open tube of the same length. Worked out for a 66 cm tube, with sound travelling at 343 metres a second: open at both ends it sounds about 260 Hz; closed at one end, about 130 Hz, with 390 and 650 Hz above. Real instruments come out a little different, because of the mouthpiece, the holes and the bell.

That's why a clarinet's low notes sound hollow and woody, and why it jumps a twelfth (to the 3rd harmonic) when the player opens the register key, where a flute jumps an octave. A real clarinet has some even harmonics, but much weaker ones. Oboes, bassoons and saxophones have cone-shaped tubes, which act like open ones: every harmonic, and an octave jump.

Brass players use the harmonics directly, changing their lips to move from one to the next (see Brass).

The picture shows the waves that fit in an open and a closed tube of 66 cm, and the harmonics each gives.

**4. Drums, bars and bells** — *Why a drum's overtones aren't harmonics*

A drum skin is held all round its edge, so it vibrates in rings and slices, not halves and thirds, and the frequencies of those shapes aren't whole-number multiples. On an ideal round skin, the shapes above the lowest vibrate at 1.59, 2.14, 2.30, 2.65 and 2.92 times its frequency: for a drum at 100 Hz, 159, 214, 230, 265 and 292 Hz. That mix doesn't add up to one clear note, which is why most drums sound like a thud or a crack rather than a pitch you could sing. Drummers still tune them higher or lower.

Timpani are the exception. The kettle under the skin and the air inside it pull the overtones close to a harmonic series, so timpani play real notes, and the player retunes them with a pedal.

A bar that's free at both ends, like a xylophone's, has overtones at about 2.76 and 5.40 times the lowest: for a bar at 440 Hz, about 1,213 and 2,378 Hz. Makers cut an arch out of the underside to move them: a marimba bar's first overtone is tuned two octaves up, 4 times the lowest, and a xylophone's a twelfth up, 3 times. That's what makes them sound like notes. Cymbals and gongs have so many unrelated overtones that they sound like a wash of noise.

In MIDI, drums don't play pitches: each note number picks a different drum (see Drums and percussion).

The picture puts the overtones of a drum skin, a free bar and a marimba bar against a string's harmonics.

**5. Keys: piano and organ** — *88 keys, hammers, a soundboard and the sustain pedal*

What vibrates: steel strings, one to three for each note. A key throws a felt hammer at them, and the hammer falls straight back so the strings ring. The faster the key goes down, the faster the hammer, and the louder and brighter the note: a hard hit makes more high harmonics. That speed is what MIDI velocity measures.

What makes it louder: the strings press on a bridge glued to the soundboard, a big, thin wooden board. A string alone moves very little air; the board moves a lot.

Range: 88 keys, from A0 (27.5 Hz, MIDI note 21) to C8 (4,186 Hz, note 108). A piano note starts dying away at once: the low notes ring for many seconds, the top ones for less than one. Letting go of the key drops a felt damper on the strings, unless the sustain pedal holds all the dampers up.

How it's played: with two hands. A common split is the left hand on the bass note, or the root and fifth, and the right hand on the chord or the melody. Each hand reaches about an octave.

In MIDI:
- Vary the velocity. Played by hand, a melody note is usually louder than the chord under it, and the first beat of a bar a little stronger. Every note at 100 sounds typed.
- Change the sustain pedal at each chord change: up just after the new chord starts, then down again, so the old chord doesn't blur into the new one (What is MIDI?, The sustain pedal).
- Keep each hand to what it can reach: about an octave, and five fingers.
- Notes of a chord played by hand don't all start on the same instant; a few milliseconds apart is enough to sound played.

The organ is a keyboard too, but it doesn't fade: a note sounds as long as the key is down, and velocity usually changes nothing. A pipe organ's pipes are tubes (Tubes: open and closed). Organists swell the sound with an expression pedal; in MIDI that's usually controller 11, expression.

The picture shows the piano's range on its keyboard and a two-handed C major chord: C2 and G2 in the left hand, E4, G4 and C5 in the right.

**6. Guitar** — *Six strings, frets, strums, and written an octave up*

What vibrates: six strings, tuned E2, A2, D3, G3, B3 and E4 in standard tuning, plucked with a finger or a pick. Each fret shortens the string and raises it a semitone.

What makes it louder: on an acoustic guitar, the bridge passes the vibration to the wooden top, and the hollow body adds warmth to the low notes. An electric guitar's solid body is nearly silent: a magnetic pickup turns the string's movement into a signal, and the amplifier shapes much of the tone.

Range: from E2 (82 Hz, MIDI note 40) to about D6 on a guitar with 22 frets (E6 with 24; a classical guitar stops lower). Guitar music is written an octave higher than it sounds, to keep it on the treble staff; DawIt's piano roll shows the notes as they sound.

How it's played: strummed chords, single picked notes, fingerpicking (the thumb on the bass strings, the fingers on the top ones), palm muting (the side of the hand resting on the strings by the bridge, for a short, chunky sound), slides, bends and hammer-ons.

In MIDI:
- Six notes at most, one per string, in shapes a hand can hold. An eight-note piano chord is unplayable on a guitar; Piano and guitar chords shows real shapes.
- A strum isn't a block: the pick crosses the strings one after another. A downstroke starts on the low string, an upstroke on the high one. As a rule of thumb, start each string about 10 to 30 milliseconds after the one before. At 120 BPM, 15 ms is 0.03 of a beat, and a six-string strum then takes 75 ms.
- Upstrokes are usually lighter and often catch only the top three or four strings: give them less velocity.
- A strummed chord rings until the next one, unless it's muted.
- Play chords as... on the chord track writes strummed and picked parts this way.

The picture shows the six open strings on the keyboard, and a down and an up strum of an E chord (E2 B2 E3 G♯3 B3 E4), each string 15 ms after the one before.

**7. Bass** — *Roots and fifths, approach notes, one note at a time*

What vibrates: four strings tuned E1, A1, D2 and G2, an octave below a guitar's lowest four. Many players use five strings, adding a low B0 (31 Hz). The electric bass has pickups and an amplifier, like an electric guitar; the double bass, its acoustic relative, is in Strings.

Range: from E1 (41 Hz, MIDI note 28) to about G4 on a bass with 24 frets (D♯4 with 20). Like the guitar, bass is written an octave higher than it sounds.

How it's played: with the fingers (round and warm), a pick (a sharper start to each note), or slapped (the thumb bounces off the low strings and the fingers pop the high ones, the sound of funk). Ghost notes are muted, almost pitchless hits between the real notes that add rhythm.

What a bass line does: it joins the drums' rhythm to the chords, and holds the band together.
- Start with the root of each chord on its first beat: under C, a C.
- Add the fifth (G under C) and the octave for movement: root, fifth, octave works in many styles.
- Lead into the next chord with an approach note a semitone or a tone away, on the last beat: C, G, C, C♯, then D.
- The octave pattern, the root low and high on alternate eighth notes, drives disco and dance music.
- Play with the kick drum: bass notes on the kick's beats make the groove tight.

In MIDI:
- One note at a time. Two notes together low down sound muddy; when you do play two, keep them wide apart, a fifth or a tenth.
- Mix short and long notes, and leave small gaps: they let the groove breathe.
- Vary the velocity a little, strongest on the main beats.

The picture shows the bass's range and a root, fifth, octave line with an approach note: C2, G2, C3, C♯3, then D3, A2, D2.

**8. Strings** — *Violin, viola, cello and double bass: bowing, plucking and swells*

What vibrates: four strings, set going by a bow or plucked with a finger. Rosin makes the bow's hair grip the string, which sticks and slips over and over and keeps sounding as long as the bow moves (What is physical modelling? shows how). The bridge passes the vibration into a hollow wooden body.

Ranges and open strings, as they sound:
- Violin: G3 to about E7 (196 to 2,637 Hz); strings G3, D4, A4, E5.
- Viola: C3 to about C6; strings C3, G3, D4, A4.
- Cello: C2 to about E5; strings C2, G2, D3, A3.
- Double bass: E1 to about B3; strings E1, A1, D2, G2, written an octave higher than it sounds. Some have an extension down to C1.
The top notes depend on the player, and most parts stay well below them.

How it's played:
- Legato: notes joined smoothly, several in one bow.
- Spiccato: short notes with the bow bouncing off the string, for fast, rhythmic parts.
- Tremolo: the bow moving back and forth as fast as it can on one note, for tension or a shimmer.
- Pizzicato: plucked with a finger, short and soft.
- Vibrato: the finger rocking on the string, a small swing in pitch on long notes.
- Divisi: a section splits, some players taking one note and the rest another.
A string section is first violins, second violins, violas, cellos and double basses; the basses often play the cello part an octave lower.

In MIDI:
- Shape long notes with a swell. Many sampled strings change loudness and tone with the mod wheel (controller 1) or expression (controller 11) while a note sounds, not only with velocity: draw a rise and fall in the Mod wheel or Expression lane.
- For a legato line, overlap each note a little with the next. Legato patches play one note at a time and use the overlap to play the join.
- Sample libraries switch articulations with keyswitches: notes below the instrument's range that change it to, say, pizzicato. Put a keyswitch a few ticks before the note it changes, and check the library's manual, as layouts differ.
- Orchestral samples often start slowly: moving a part 10 to 40 milliseconds earlier can put it on the beat.
- A single violin, viola or cello plays one or two notes at once; chords of three or four are spread quickly across the strings. For full chords, use a section.

The picture shows the four instruments' ranges and open strings on the keyboard.

**9. Brass** — *Buzzing lips, the harmonic series, valves and the slide*

What vibrates: the player's lips, buzzing in a cup-shaped mouthpiece, set the air in a long metal tube going. The bell at the end lets the sound out and gives it its brightness.

Why the pitch is what it is: brass players pick notes from the harmonics of the tube, tightening the lips and blowing faster to go up. A B♭ trumpet with no valves pressed gives B♭3, F4, B♭4, D5, F5 and B♭5, harmonics 2 to 6 and 8 of B♭2 (116.5 Hz). The 7th, near A♭5, is 31 cents flat, so it's avoided. Valves add extra tube: the first lowers the pitch by two semitones, the second by one, the third by three, and together they fill the gaps between harmonics. A trombone's slide does the same with seven positions, each a semitone lower.

Ranges, as they sound:
- Trumpet in B♭: E3 to about B♭5, written a tone higher than it sounds.
- French horn in F: B1 to about F5, written a fifth higher.
- Trombone: E2 to about B♭4, with low pedal notes below.
- Tuba: comfortable from F1 to C4; pedal notes reach B♭0, and strong players go to C5 and above.

How it's played: everything comes from the breath and the lips, so loudness, tone and phrase length depend on breathing. Brass can swell from very soft to very loud, and it gets brighter as it gets louder. Falls (the pitch dropping off at the end of a note) and doits (a slide up) are common in jazz and pop. A mute in the bell makes it quieter and changes the colour: a straight mute is thin and bright, a cup mute soft and round, a harmon mute the buzzy, distant sound of Miles Davis.

Sections: a pop horn section is often trumpet, saxophone and trombone; a big band has sections of trumpets, trombones and saxophones, four or five each. Close voicings, with the chord's notes within an octave, give the punchy section sound.

In MIDI:
- One note at a time per player: for a chord, a track per instrument, or a brass section patch.
- Leave room to breathe: a rest every few bars.
- Velocity sets the attack; the Expression lane shapes swells and fades on long notes.
- Short stabs need short notes; long chords need a swell, not a flat level.
- Don't keep a part in its top few notes for long: they tire the player and sound strained.

The picture shows the brass ranges on the keyboard and a B♭ trumpet's harmonics with no valve pressed.

**10. Woodwinds** — *Flute, clarinet, oboe, bassoon and saxophone: edges, reeds and breath*

What vibrates: the air in a tube, kept going by the breath. A flute player blows across an edge, like across the top of a bottle. A clarinet and a saxophone have one cane reed against a mouthpiece; an oboe and a bassoon have two reeds against each other, which gives their nasal, reedy sound. Keys and holes shorten the vibrating tube for higher notes. The saxophone is made of brass, but it's a woodwind because it uses a reed.

Why they sound different: the flute's tube is open at both ends and gives every harmonic, a pure, breathy tone. The clarinet's is a cylinder closed at the reed, so its low notes are mostly odd harmonics, the hollow sound in Tubes: open and closed. Oboes, bassoons and saxophones have cone-shaped tubes, which give every harmonic.

Ranges, as they sound:
- Flute: C4 to about C7. The piccolo plays an octave higher, and is written an octave lower than it sounds.
- Oboe: B♭3 to about F6.
- Clarinet in B♭: D3 to about B♭6, written a tone higher than it sounds.
- Bassoon: B♭1 to about E5; most parts stay at or below C5.
- Alto saxophone: D♭3 to A♭5, written a major sixth higher.
- Tenor saxophone: A♭2 to E♭5, written a ninth higher.
Many saxophones have a high F♯ key, one semitone more, and skilled players go higher still.

How it's played: one note at a time, on the breath. Phrases are shaped like sung lines: they grow and fade, and end where the player breathes. Notes are tongued (each started with the tongue, a clear start) or slurred (joined, only the first tongued).

In MIDI:
- One note at a time: a chord needs a track per instrument.
- Breathe: a short rest every few bars. As a rule of thumb, a phrase that runs on for 8 seconds or more with no gap sounds like a machine.
- Shape long notes in the Expression lane, and give the notes inside a slurred phrase slightly less velocity than its first.
- Use each instrument's best register: the flute is soft low down and bright up high; the clarinet dark and rich at the bottom; the oboe cuts through in its middle.

The picture shows the woodwind ranges on the keyboard.

**11. Voice and choirs** — *Voice types, breath, choirs, backing vocals and vocal chops*

What vibrates: two small folds in the throat, the vocal folds, set going by the breath. The throat, mouth and nose shape the sound: their resonances, called formants, make the vowels. Singing techniques explains how the voice works and how to sing.

Ranges: voice teachers give each voice type about two octaves: bass E2 to E4, baritone A2 to A4, tenor C3 to C5, alto F3 to F5, mezzo-soprano A3 to A5, soprano C4 to C6. Most pop singers use less, and the comfortable part is the middle. A tenor part is usually written in the treble clef, an octave higher than it sounds.

How voices are used:
- The lead vocal carries the words and the melody.
- Backing vocals sing harmonies (often a third above or below the melody), oohs and aahs behind the lead, or answers to it.
- A choir usually sings in four parts, SATB: soprano, alto, tenor and bass, each in its own range.
- Vocal chops are short pieces of a recorded voice, cut up and played as an instrument, common in dance music.

In MIDI: a MIDI note can't sing words, so a MIDI voice is a choir or "aah" sample, a vocal chop in a sampler, or a sketch of a melody someone will sing later.
- Keep every part inside its voice type's range, and leave breaths as a singer would: as a rule of thumb, a line that runs 8 seconds or more with no rest needs one.
- One note at a time per part: for a choir, a track for each of the four parts, or a choir patch.
- Choir pads sound more real with the Expression lane rising into each chord and falling away.
- Sing this melody... on a MIDI clip lets you hear a melody sung, and Singing techniques checks a melody against your range.

The picture shows the six voice types' ranges on the keyboard.

**12. Synths and electronic instruments** — *Pitch bend, the mod wheel, glide and the arpeggiator*

What vibrates: in the end, a speaker cone. A synth makes the signal itself, from oscillators and filters or from recordings, and an amplifier and a speaker turn it into moving air. So a synth has no fixed range and never needs a breath: how it plays is up to you.

How synth players play, with one hand on the keys and one on the controls:
- Pitch bend: a wheel that slides the pitch up and down, usually by up to 2 semitones, for bends like a guitarist's or a singer's.
- The mod wheel: usually adds vibrato, or opens a filter, while a note sounds.
- Glide (portamento): the pitch slides from one note to the next instead of jumping. On DawIt's synths in mono mode, only overlapping notes slide, so you choose which notes glide by how you play them.
- Mono and poly: a mono synth plays one note at a time, for leads and basses; a poly synth plays chords, for pads and keys.
- The arpeggiator turns a held chord into a pattern of single notes (What is an arpeggiator?).
- Knobs: players turn the filter's cutoff by hand during a part, for a build-up.

In MIDI:
- Use the Bend lane for a slide into a note, and the Mod wheel lane for vibrato on long notes only.
- For a mono lead with glide, overlap the notes you want to slide to, and leave a gap before the ones you want to jump to.
- Velocity may change nothing on a synth sound, or may open its filter: listen before drawing it.
- A synth doesn't have to imitate anything, but the same habits make it musical: phrases with rests, accents on the strong beats, long notes that change.

The picture shows a synth lead: a bend up 2 semitones into a note, and a glide between two overlapping notes.

**13. Drums and percussion** — *The kit, the drum map, four limbs, accents and ghost notes*

What vibrates: skins (the drums), metal plates (the cymbals), wood and metal, and seeds inside a shaker. Most make a noise more than a note (Drums, bars and bells).

The kit: a kick drum played with a foot pedal, a snare with wires under it that buzz, toms, a hi-hat (two cymbals the other foot opens and closes), a ride and a crash. In General MIDI the drums are on channel 10, and each note number is an instrument: 36 is the kick, 38 the snare, 42 the closed hi-hat, 46 the open hi-hat, 49 a crash and 51 a ride. DawIt Synth's Drums sound follows this map, from note 35 to 81, and so do most drum samplers.

How drummers play: two hands and two feet, so four hits at the same moment at most. A basic rock beat has the hi-hat on every eighth note, the kick on 1 and 3, and the snare on 2 and 4. Drummers accent the hi-hat (louder on the beat, softer between) and add ghost notes: very soft snare hits between the main ones, felt more than heard. A fill, a short busy run on the toms or the snare, marks the end of a section.

Percussion adds a layer on top: a shaker or tambourine on eighths or sixteenths, congas and bongos in short repeating patterns, a cowbell or claves on a fixed rhythm. Many musical traditions have their own: the tabla in Indian music, the steel pan from Trinidad and Tobago, the kalimba, a thumb piano with metal tines, from Africa.

In MIDI:
- Vary the velocity: as a rule of thumb, hi-hats about 90 to 110 on the beat and 60 to 80 between, a strong backbeat snare, and ghost notes very soft, about 20 to 40.
- No more than four hits at once, and none a drummer couldn't reach.
- On a real hi-hat, closing it stops the open sound: end an open hi-hat note where the next closed one starts.
- Don't quantise every hit to 100%: small timing differences sound played (Note lengths, beats and bars, Quantising and feel).
- Change the pattern for each section, and add a fill at the end of a phrase.

The picture shows one bar of a rock beat on the drum map, with its velocities: hi-hat accents, the backbeat and two ghost notes.

**14. Orchestration basics** — *Who carries the tune, doubling, and giving each part room*

Orchestration is choosing who plays what. The same few ideas work for a band and an orchestra.
- One part carries the melody, and it has to be easy to hear: high, or in a colour that stands out. In an orchestra the first violins, the flute, the oboe or the trumpet often have it; in a band, the voice or a lead instrument.
- Give each part its own register: the bass at the bottom, the chords in the middle, the melody on top. Two parts in the same few notes blur into each other.
- Low notes need space. Notes close together sound clear high up but muddy low down: as a rule of thumb, below about C3 keep to octaves and fifths.
- Doubling at the octave makes a line stronger without thickening it: violins with a flute an octave above, cellos with double basses an octave below. Doubling in unison (two instruments on the same notes) blends their colours into a new one.
- Take turns. Strings, then woodwinds answering, then everyone at the climax. A full sound all the time leaves nowhere to grow.
- Write for what each instrument does well: long lines for strings and horns, quick figures for woodwinds and pizzicato, punch for brass.
Film, TV and game music lean on all of these.

The picture stacks every family's range on one keyboard, and shows a melody on the violins doubled by a flute an octave higher.

**15. What makes MIDI sound real** — *Range, breath, velocity, length, timing and change inside a note*

A part sounds typed when every note has the same strength and length, sits exactly on the grid, and does things the instrument can't. Players do none of that. A checklist for every family:
1. Range: keep each part inside its instrument's range, and mostly in the middle of it. A cello note below C2 doesn't exist.
2. Notes at once: one for wind, brass and voice and most bass lines; six on a guitar; five fingers a hand on a piano; four limbs on a drum kit.
3. Breath: wind, brass and voices need a rest every few bars.
4. Velocity: vary it. Strong beats a little louder, the top of a phrase loudest, repeated notes never all the same. In a sampled instrument, velocity often picks a different recording, so it changes the tone as well as the level (What is a sampler?).
5. Note lengths: mix short and long, and leave gaps where a player would lift a finger or the bow. Legato lines overlap a little; staccato notes are short.
6. Timing: quantise, but not always fully. A strum spreads its notes; a rolled chord on a harp or piano starts its notes a few milliseconds apart.
7. Change inside a note: a held note on a real instrument isn't flat. Draw a swell in the Expression or Mod wheel lane, add vibrato to long notes, bend into a note on a guitar or synth.
8. Articulation: switch between joined, short and accented notes, with keyswitches or a track for each.

The picture shows the same string line twice. Typed: every note at velocity 100 and 0.9 beats long, no swell. Played: velocities from 84 to 104, each note overlapping the next by 0.05 beats for legato, and expression rising from 70 to 110 on the long last note.

**16. Your track** — *Your track's notes against its instrument*: the analysis paragraphs (below).

### Example data
- Strings topic 2: A2 harmonics 110, 220, 330, 440 Hz; A4 440, half 880, two thirds 660.
- Tubes: L 0.66 m, c 343 m/s: open 259.8, 519.7, 779.5 Hz; closed 129.9, 389.8, 649.6 Hz.
- Membrane 100 Hz: 100, 159.3, 213.6, 229.5, 265.3, 291.7 Hz. Free bar 440 Hz: 440, 1212.9, 2377.7 Hz. Marimba 440 → 1760; xylophone 440 → 1320.
- Piano chord: 36, 43 | 64, 67, 72.
- Strum (120 BPM): down 40, 47, 52, 56, 59, 64 at 0, 15, 30, 45, 60, 75 ms, velocity 96 to 86 falling by 2; up on beat 2.5: 64, 59, 56, 52 at 0, 15, 30, 45 ms, velocity 72.
- Bass (beats, 1 each, last 2): 36, 43, 48, 49 | 50, 45, 38; velocities 100, 84, 92, 80 | 100, 84, 92.
- Trumpet harmonics on 116.54 Hz: n = 2, 3, 4, 5, 6, 7, 8: 233.1, 349.6, 466.2, 582.7, 699.2, 815.8, 932.3 Hz; cents from equal temperament 0, +2.0, 0, −13.7, +2.0, −31.2, 0.
- Drum bar (eighths, beats 0 to 3.5): hats 42 at 105, 70 alternating; kick 36 at 0 and 2, velocity 110; snare 38 at 1 and 3, velocity 115; ghosts 38 at 1.75 and 3.75, velocity 30.
- Synth: A4 (1 beat) bends to +2 semitones (value 16383 at range 2) over 0.25 beat into a held B4 pitch; C5 overlapping D5 by 0.1 beat (glide 100 ms).
- Orchestration: melody E5 D5 C5 D5 (violins 76, 74, 72, 74), flute 88, 86, 84, 86.
- Realistic line (90 BPM, beats 1, 1, 1, 1, 4): C5 D5 E5 G5 E5 (72, 74, 76, 79, 76). Typed: velocity 100, lengths 0.9, 0.9, 0.9, 0.9, 3.6. Played: velocities 84, 90, 96, 104, 92; lengths 1.05, 1.05, 1.05, 1.05, 4.0; expression 70 at beat 4 rising to 110 at beat 6, falling to 80 at beat 8.

### "Your track" rules
In this order, a paragraph each; bars as the ruler shows them (1 is the first); note names as `midiguide::getNoteName`.
1. **No track:** no MIDI track selected and none with notes: "Select a MIDI track with notes, and this topic checks them against the instrument they're for."
2. **Unknown:** "DawIt can't tell which instrument "<track>" is for. Choose one above. A DawIt Sampler preset, or a track name such as Cello, lets DawIt guess." Nothing else.
3. **Identity** (always first otherwise): "Checking "<track>" as <instrument> (from its DawIt Sampler preset, <preset> / from its name / from its DawIt sound, <sound> / chosen above). <N> notes."
4. **Below:** any note below the lowest: "<n> notes are below the <instrument>'s lowest note, <low> (the first in bar <b>, <name>). Try them an octave up, or give them to a lower instrument." Bass guitar adds, when all are at or above B0: "A five-string bass reaches down to B0." Drums: any note outside 35 to 81: "<n> notes are on numbers with no General MIDI drum (the first in bar <b>, note <k>): most drum sounds play nothing there."
5. **Above:** "<n> notes are above the top of the <instrument>'s range, <high> (the first in bar <b>, <name>). Try them an octave lower, or give them to a higher instrument."
6. **Too many at once:** notes starting within 0.05 beats count as one moment; moments with more than the table's "at once": "In <m> places, <k+1> or more notes start together (the first in bar <b>). <Reason>" with: one-note instruments "A <instrument> plays one note at a time: give the others to another track."; bass and solo strings "A <instrument> plays one or two notes at a time."; guitar "A guitar has six strings."; piano "Two hands play ten notes at most."; drums "A drummer has two hands and two feet: four hits at once at most."
7. **Breath** (breathes): the longest run with no gap of 0.3 s or more, when longer than 8 s: "The longest phrase, from bar <b>, runs <s> seconds with no gap to breathe. Leave a short rest, a third of a second or more, every few bars."
8. **Velocity:** 8 or more notes, highest minus lowest velocity 4 or less: "All <N> notes have about the same velocity (<v>). Vary it: <family hint>." Hints: drums "accents on the hi-hat, soft ghost notes on the snare"; others "stronger on the main beats and at the top of a phrase, softer between".
9. **Swell** (swells): 3 or more notes of 2 s or longer and no mod wheel, expression or volume events in the track's clips: "<n> notes last 2 seconds or more and stay at one level. Draw a swell in the Expression lane (or the Mod wheel lane, which many sampled strings and brass use for loudness)."
10. **All well:** none of 4 to 9: "Nothing to point out: every note is in the <instrument>'s range and playable. What makes MIDI sound real has more to try."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Membrane ratios | 1.593, 2.136, 2.295, 2.653, 2.917 | 0.002 |
| Free bar ratios | 2.756, 5.404 | 0.002 |
| Open, closed tube 0.66 m | 259.8, 129.9 Hz | 0.1 |
| Trumpet harmonic 5, 7 cents | −13.7, −31.2 | 0.1 |
| Strum 15 ms at 120 BPM | 0.03 beat; 6 strings 75 ms | exact |
| Note numbers of table ranges | as the Ranges table | exact |
| Written higher | clarinet 2, trumpet 2, horn 7, alto 9, tenor 14, guitar, bass, double bass 12 | exact |
| Guess | program 43 → cello; 34 → bass guitar; bank 128 → drum kit; 74 → flute; name "Double Bass" → double bass; "Bassoon 2" → bassoon; "Bass" → bass guitar; "Lead vocal" (lead sound) → voice; "BV 1" → voice; "Track 3" with lead sound → synth; "Track 3" with a third-party plugin → unknown | exact |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `src/generate/InstrumentsGuide.{h,cpp}` (both CMake targets): the topics, names, summaries and `explain` texts from *Theory*, and the example data. Add `Guide::instruments` to `gen::guides` with its entry (*The guide*), because `SOURCES.md` already has the `Guide key: instruments` heading (written with this plan) and the guide-sources test fails until the key is a guide; give `MainComponent::openGuide` and `getPrintGuide` a case that does nothing yet, so the app builds. Tests: `runInstrumentsGuideTests` (every topic has a name, summary, text of 300+ characters and example; the example numbers); `runTheoryGuidesTests` search checks (cello, strum, keyswitch, clarinet, ghost note, transposing find it). Done when the tests pass.
- [x] 2. The instrument table, the guess and the physics numbers (membrane and bar ratios, tube frequencies, trumpet cents, strum beats). Tests: the *Test numbers* table. Check: the guess table passes.
- [x] 3. The *Your track* analysis and its text. Tests: each rule on a made-up track (below), and the quiet cases. Done when `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 4. `InstrumentsExplainer` with the list, text, Sources topic and PDF export, pictures blank; `MainComponent`'s show function, and its `openGuide` and `getPrintGuide` cases filled in. Check: the guide opens from Theory Guides at any topic.
- [x] 5. The pictures of topics 1 to 15 in the app's visual language (ui-design.md *Live pictures*), clicks that play a note, *Play* for the example lines, tooltips on range bars and notes.
- [x] 6. *Your track*: the selected MIDI track (or the first with notes) turned into the analysis struct, the dropdown, its picture; follows selection and edits.
- [x] 7. Entry points: *About writing for this instrument...* in the MIDI clip menu (`TrackLanes`) and at the end of a MIDI track's FX menu (`TrackHeader`), opening at the guessed family's topic. One-sentence pointers in the sampler guide's *General MIDI* page, the physical guide's *Tubes and reeds* and the singing guide's *Voice types and range*. `--export-guides` passes.

Group C, check and docs:
- [x] 8. Real-app check, one `--screenshots` run: `guide: instruments` at *Tubes: open and closed*, *Guitar* (`clap`/Play), *Strings*, *Brass*, *Drums and percussion*, *What makes MIDI sound real*; *Your track* with `select: "Bass"` (guess from the built-in sound), `select: "Lead vocal"` (from its name) and `select: "Kick"`. Look for clipped labels, range bars at the right keys, the advice text. Nothing new in `ScreenshotRunner`.
- [x] 9. Docs: README *Features* bullet (guides list); `docs/tests.md` (instruments guide area); `docs/code-layout.md` (two files pairs); `docs/architecture.md` *Guides* list gains `gen::instruments` and its opener; issue #13 progress (what's built, what stays open from *Left for later*).

## Tests
In `runInstrumentsGuideTests`:
- every topic: name, summary, text over 300 characters, example data; 16 topics; the Theory Guides entry lists them all;
- the *Test numbers* table;
- analysis: a cello clip with A1 in bar 5 and G5 in bar 12 → below and above paragraphs naming bars 5 and 12; a flute clip with 3 two-note chords → "In 3 places"; a 6-note strum on guitar → nothing, 7 notes → the guitar line; a drum bar with 5 hits at beat 0 → the four-limbs line; a note 30 on drums → the drum-map line; a trumpet phrase of 12 s at 120 BPM without gaps → breath line with "12" (and with a 0.3 s gap at 6 s → quiet); 16 notes all velocity 100 → velocity line, velocities 98 to 103 → still (spread 5) quiet; strings with 3 notes of 2.5 s and no controllers → swell line, with an expression event → quiet; a synth with notes 0 to 127 → no range line; a clean cello part → the all-well line only;
- search: "cello", "strum", "keyswitch", "clarinet", "ghost note", "transposing" find the guide.

## Website hand-off
- `learn.html`: a card for *Instruments in MIDI* in the MIDI area; screenshot of *Brass* (ranges and harmonics) or *Your track* on the Bass track.
- `features.html`: one line in the guides group: "Instruments in MIDI: how each family makes its sound and how to write it so it sounds played, with a check of your own track."
- Home or `for-you.html`: no. `about.html`: nothing (no library).

## Left for later
- Pages per instrument (violin and viola apart), piccolo, English horn, baritone sax, euphonium in the table, and pitched percussion ranges (marimba, xylophone, glockenspiel, timpani): sources disagree or weren't checked.
- World percussion in depth (tabla bols, steel pan, kalimba tunings and patterns).
- Comfortable ranges per instrument, beside the full ones (sources vary).
- Choir writing (voice leading, SATB spacing) and vocal chops in depth: with Singing techniques.
- The spectrum of each instrument's note from the sampled sound; previews on a sampled instrument rather than the track's or the piano.
- Saving the chosen instrument on the track; commands that fix what *Your track* finds (move out-of-range notes, spread a strum): with #42 *Humanize MIDI with AI* and #25 *Play chords in an instrument's style*.
- Guitar playability (whether a chord shape fits one hand), keyswitch lanes.

## Changes while building

### Group A hand-off (engine, built; `./build.sh --test` passes with no warnings)
- **Code:** `src/generate/InstrumentsGuide.{h,cpp}` (`daw::gen::instruments`, in both CMake targets). `Guide::instruments` is added at the end of `Guide` and `allGuides`, with its key, entry (Area::midi, not on Start here) and 16 topics in `gen::guides`. Tests: `runInstrumentsGuideTests` (36 checks).
- **API for the UI (Group B):** `Topic`/`allTopics`/`getTopicName`/`getTopicSummary`/`explain (Topic, yourTrackText)` (for `Topic::yours` it adds a short intro before the analysis text, and says to select a track when the text is empty); `getExample (Topic)` returns an `Example` (lines of notes with bend and expression points, `ranges` as `RangeBar`s with open strings, `numbers` and `labels` rows, `details` for the tiles, `glideMs`, `strumMs`, `bpm`); `getInstruments()`/`getInfo`/`Family`/`getFamilyTopic` for the dropdown "by family" and the menu's topic; `guessInstrument (Hints)` and `analyse (TrackPart, optional<Instrument> chosen, Hints)` for *Your track*. The UI fills `Hints` with: `hasSampler`, `samplerBank`, `samplerProgram` (the sampler's own `getProgram()` plus 1), `samplerPreset` name, `trackName`, `builtInSound` (the sound's id: piano, organ, bass, lead, pluck, pad, drums, percussion) when no plugin is the track's instrument, and `dawItInstrument` (the plugin's name; the guess matches "DawIt Drums", "DawIt Subtractive", "DawIt Additive", "DawIt FM", case-insensitively, so check these are the plugins' real display names). `TrackNote::bar` is the bar as shown (1 is the first).
- **MainComponent:** `openGuide` and `getPrintGuide` have empty `G::instruments` cases (and `ArrangementView`'s two switches list it), to be filled in step 4. Until then `--export-guides` has no source for the guide.
- **Changes from the plan:** (1) piccolo (program 73, the word "piccolo") gives no guess, because the table has no piccolo and mapping it to Flute would flag its notes above C7; GM programs 65, 68, 70 give none for the same reason. (2) The guess takes the program as shown (1 to 128), so "program 43" is the cello. (3) Notes starting within 0.05 beats of the one before count as one moment (a chain), so a spread strum of up to six notes is one moment. (4) The harp's too-many-at-once reason ("A harp is played with eight fingers: eight notes at most.") and the *Your track* introduction paragraph are mine, not in the plan's Theory. (5) The synth example bends A4 up 2 semitones over the quarter beat from beat 1 to 1.25 and holds, the glide pair C5 then D5 overlap by 0.1 beat at beats 2.5 and 3.4; the strum's up stroke starts at beat 2.5 (0-based).
- **Theory discrepancy:** the 7th harmonic's distance from equal temperament works out at -31.17 cents (-31.2 to one decimal), not -31.1 as the plan's table says; the test uses the table's tolerance of 0.1 and the texts say "31 cents flat". dawTheory may want to correct the plan table.
- **Left for later groups:** nothing half-done. Sources: `SOURCES.md` already has the guide's heading; no new sources were needed.

### Group B hand-off (UI, built; `./build.sh --test` passes with no warnings, `--export-guides` writes "Instruments in MIDI.pdf", 17 topics, 19 pages)
- **Code:** `src/ui/InstrumentsExplainer.{h,cpp}` (in `DawIt` only). Free helpers `instrumentsui::getHints`, `getTopicFor (Engine&, Track&)` and `getFamilyColour`. `MainComponent::showInstrumentsGuide (Topic)`, `openGuide` and `getPrintGuide` cases filled. Plugin display names `DawIt Drums/Subtractive/Additive/FM` match `identifier` in the engine.
- **Entry points:** clip menu item 44 in `TrackLanes` (`onShowInstrumentsGuide`), FX menu item 3 in `TrackHeader` (via `TrackActions::onShowInstrumentsGuide`, MIDI tracks only, after a separator), both forwarded by `ArrangementView::onShowInstrumentsGuide (Track&)`. One-sentence pointers added to the sampler guide's *General MIDI* text (`engine/SamplerGuide.cpp`), the physical guide's *Tubes and reeds* and the singing guide's `rangeText`... in `SingingGuide.cpp` (the measure-by-ear paragraph).
- **Not looked at yet (Group C):** nothing was screenshotted, so label clipping, range-bar positions and the *Your track* picture are unchecked. Pictures are drawn in `Picture::paint*` (tiles, strings, tubes, partials, ranges, lines, brass cents, yours). The strum overlap tooltip is hidden above 0.5 beats. Playback uses the example's own tempo; the synth lead's bend and the realistic line's expression are sent with `previewMessage`; the glide is not audible.
- **Text changes outside the plan:** the three pointer sentences above (wording mine). Choice dropdown, *Your track* key line and picture labels are UI text.

### Group C hand-off (real-app check and docs, built; `./build.sh --test` ends with All checks passed, no warnings)
- **Shots looked at** (`/private/tmp/claude-501/-Users-geertvandamme--claude-spares-spare-PS62f1/604211f2-24ce-4f49-be11-30cf010ec9b4/scratchpad/shots/`, 1000 x 760, window capture): `tubes.png`, `guitar.png` (with `clap`), `strings.png`, `brass.png`, `drums.png`, `real.png`, `orch.png`, `yours-bass.png`, `yours-vocal.png`, `yours-kick.png`; list in `scratchpad/shots.json`. Range bars sit on the right keys (bass E1 to G4, voice E2 to C6, drum kit B1 to A5); the 19-bar orchestration picture and the brass columns fit.
- **Fixed in `InstrumentsExplainer.cpp`:** (1) the *Your track* dropdown showed no text, because selecting the same id again didn't refresh the changed item text: it now goes through "none" first; (2) the open-string dots covered the range label on string bars: dots now sit on the bar's bottom edge; (3) the tube picture's third harmonic label ran off the right edge: bars are shorter.
- **Notes:** the Bass track's guess says "from its name" (name beats the built-in sound in the guess order). The flat sign renders with spaces around it ("B ♭") in the system font; left as it is. Tube labels show Hz to three decimals (259.848), the text rounds to 260.
- **Docs:** README *Features* bullet, `docs/tests.md`, `docs/code-layout.md` (two files), `docs/architecture.md` (`gen::instruments` and its opener). Issue #13 has a *Built, not committed* comment, still open, project status In Progress (verified).
- **Website hand-off:** as under *Website hand-off*; best shots `brass.png` (ranges and harmonics) or `yours-bass.png`; no library or model added, so nothing for `about.html`.

### Theory check
- Plan *Theory*: the 7th trumpet harmonic is −31.17 cents, so −31.2 (facts row 18, example data, test numbers); a free bar at 440 Hz has overtones at 1212.9 and 2377.7 Hz, not 1212.6 and 2377.8 (the text's 1,213 and 2,378 were right).
- `tests/EngineTests.cpp`: the 7th harmonic's expected value is −31.2 (was −31.1, inside the tolerance only by 0.07).
- `InstrumentsGuide.cpp`: the strum spacing (Guitar), the 8-second phrase (Woodwinds) and the 8-second line (Voice) are now worded "as a rule of thumb", as decided; the same three sentences in the plan's topic texts.
- `InstrumentsGuide.cpp`: the harp's too-many-at-once reason says what harpists do: "A harpist plucks with the thumb and three fingers of each hand: eight notes at once at most." (the little fingers aren't used; Wikipedia *Pedal harp*).
- `InstrumentsGuide.cpp`: the *Your track* introduction says "a wind, brass or vocal part" leaves room to breathe, since the breath rule also checks voice and choir.
- `InstrumentsExplainer.cpp`: the pitch-bend tooltip no longer says the glide comes from "the instrument's glide time, 100 ms" (100 ms is the example's, and Play doesn't glide); the *Your track* key says a yellow triangle "marks more notes at once than it can play" rather than "a chord on an instrument that can't play it" (a cello's double stop isn't marked).
- `SingingGuide.cpp`: the pointer says Instruments in MIDI shows "the voice's range next to every other instrument's" (its orchestration picture has one Voice bar; the six types are on the Voice topic alone).
- Checked and right: every range in the table against its note names and Hz, transpositions, GM programs, drum map 35 to 81 (also DawIt Synth's Drums), membrane and bar ratios, tube frequencies, trumpet harmonics, the example data (strum up stroke at beat 2.5, the A4 bend to +2 over beats 1 to 1.25, the C5/D5 overlap of 0.1 beat), the voice types against Singing techniques, and the sampler and physical guides' pointers.
- Fixed after the theory check (user's choice): the organ now has its own reason and a limit of 12 (ten fingers and one or two pedal notes), and GM 54 (Voice Oohs) is guessed as Choir. Was open: "Two hands play ten notes at most" was also the organ's reason, though organists add the pedals with their feet (a separate reason needs a code change); GM 54 (Voice Oohs) is guessed as a solo voice, one note at a time, though many SoundFonts' Voice Oohs are choirs; GM 17 to 24 includes accordion and harmonica, guessed as Organ.
