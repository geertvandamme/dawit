# DawIt Harmoniser: add pitched voices to a vocal or instrument

Status: Released
Idea: [#122 — DawIt Harmoniser: add pitched voices to a vocal or instrument](https://github.com/geertvandamme/dawit/issues/122)

## Goal
*DawIt Harmoniser*, a built-in audio effect that adds two pitched copies ("voices") of one voice or single-note line: a harmony in the song's key, an octave, a fifth, or a wide double. It shows the lead note, each voice's note and the interval between them as the song plays, and its guide teaches intervals, fixed versus in-the-key harmony, which intervals sound how, doubling, formants, and what a harmoniser can't do.

## Decisions for you
1. **Scope of "in the key".** Options: (a) Fixed only; (b) Fixed and Key (scale steps in the song's key or a key chosen in the effect); (c) also Chords (voices pick chord tones of the bar from the chord track). **Recommended:** (b): Key is how singers think of harmony; Chords needs rules for passing notes (a chord-tone harmony makes seconds on them) and bars without chords, and is easier to judge once (b) is heard. Chords goes to *Left for later*.
   Answer: (b) Fixed and Key; Chords left for later.
2. **Number of voices.** 2 or 4. **Recommended:** 2: covers a 3rd above and below, a triad, octave plus fifth and a stereo double, fits the 880 px knob area, and costs two shifters.
   Answer: 2 voices.
3. **Consonants in the voices.** (a) Voices fade out (10 ms) where there's no clear pitch, so only the lead carries s, t and breaths; (b) the voices pass them through. **Recommended:** (a): avoids the piled-up "s" the singing guide warns about, and a harmony on chords or noise goes quiet instead of copying the mix.
   Answer: (a) Voices fade out; only the lead carries consonants and breaths.
4. **Plugin build.** (a) App only for now, like DawIt Tune (`APP_ONLY`); (b) also a VST3/AU now (needs `PitchTracker`, `Psola`, `TunePictures` and `SingingGuide` in the plugin sources). **Recommended:** (a), but with the manual key from day one, so a later plugin build is a CMake change.
   Answer: (a) App only for now, with the manual key built in from the start.
5. **Name.** "Harmonizer" (with a z) is an Eventide trademark (filed 1976, still claimed; status not checked in the USPTO register). Options: keep *DawIt Harmoniser* (British spelling, descriptive); rename (*DawIt Harmony*, *DawIt Voices*). The identifier and plugin code are permanent once a project saves them. **Recommended:** keep the issue's name, but decide before the build; the plan's texts use it.
   Answer: Keep *DawIt Harmoniser*.
6. **Theory Guides area.** *Singing* (beside *What is pitch correction?*) or *Effects*. **Recommended:** *Singing*: most users will put it on a vocal, and it opens from the singing guide's *Harmonies and doubles*.
   Answer: *Singing*.
7. **Typical numbers from one read source plus a search summary** (for dawTheory): doubling "about 4 to 10 cents and 8 to 30 ms" (Sound on Sound gives only "a few cents… a few tens of ms"; the 4 to 9 cents and 8 to 20 ms page couldn't be fetched), and "PSOLA most natural within about 6 semitones" (search summaries of a lecture and a 2025 paper). **Recommended:** keep, worded as typical.
   Answer: Keep both, worded as typical.

## What the user does and sees
- **Add:** the effect list (audio effects) offers *DawIt Harmoniser*; the default starting point is *Third above*.
- **Window** (`fxui` frame, 1240 × 640): header with starting points, *Save...*, guide button, test sound *Play a sung line* (the singing guide's phrase as a made-up "ah" take through `tune::makeTake`). No match level: Mix sets the balance, as in Reverb.
- **Controls:** group *Harmony*: Harmony switch (Fixed, Key), Key menu (Song, C … B), Scale switch (Major, Minor; dimmed unless a key is chosen), Mix. Groups *Voice 1* and *Voice 2*: On, Interval, Level, Pan, Detune, Delay. The Interval knob shows Semitones in Fixed and Steps in Key (two parameters, so switching keeps both); the value text names it ("+4 · major 3rd up", "3rd up").
- **Picture:** left, a *now* column: a two-octave vertical keyboard around the lead, the key's notes shaded, the lead note as a dot in `text`, each voice in its colour with an arrow and label ("minor 3rd up · 3"), "–" with no clear note. Right, a trace of the last 8 s like DawIt Tune's (key rows shaded), the lead line plus a line per voice in its colour, gaps where no clear pitch. Level history below.
- **Lead line:** in Key with no song key: "No song key here: choose one under Key." On a bus, return or instrument track: "On a bus, DawIt Harmoniser makes everything in it 27 ms late. Put it on the audio track itself." (instrument/return wording to match).
- **Trace:** hover "Bar 9 · E4 → G4 (minor 3rd up) · C4 (major 3rd down)"; click moves the playhead; right-click *Show 4 / 8 / 16 seconds*, then *What is a harmoniser?* (`guideicon::addItem`). Knob groups have explainer topics (texts in *Theory*).
- **Device bar chip:** the lead and the voices as short bars on a mini column of rows in the voices' colours, "–" with no clear note.
- **Remembered:** `view.harmoniserTraceSeconds`.

## Data and saving
No project format change: parameters live in the plugin state (APVTS) and save with the chain; a missing parameter loads its default (recipe rule 8). The frames and the take log are session-only, as in Tune. Test: `checkThroughProject<BuiltInHarmoniser>` with every parameter off its default.

## The guide
*What is a harmoniser?*, an `fxui::Guide` (texts in `src/generate/HarmoniserGuideText.{h,cpp}`, `gen::harmoniserguide`, like `TuneGuideText`). Pages (texts in *Theory*):
1. **What a harmoniser is**: the singing guide's phrase with a 3rd up in the key (lead and voice lines, labels in semitones). Click plays it.
2. **Intervals**: a ladder from C4, 0 to 12 semitones, names and ratios; click a row to hear C4 with it.
3. **Fixed or in the key**: the C major scale C4–C5 with Fixed +4 beside Key +2 steps; out-of-key notes ringed.
4. **Which intervals sound how**: the phrase with 3rd above, 6th below, 5th above, octave below, each labelled perfect or imperfect consonance.
5. **Where the key isn't enough**: a C major bar, lead E4 D4 C4 and the 3rd up G4 F4 E4 with F marked against the chord's E; beside it B4 → F5 (6 semitones).
6. **Doubling and width**: lead in the centre, two copies at ±7 cents, 12 and 20 ms, panned ±70%, on a time-and-pan sketch; the 30 ms and 50 ms marks.
7. **Formants and how far to move**: harmonics under the "ah" envelope, up a fifth by speeding up (envelope moves 730 → 1,094 Hz) versus PSOLA (envelope stays).
8. **What it hears**: a sung "sun": the s (no pitch, voices off), the vowel (voices on), and a C major chord (no clear pitch).
9. **Using it well**: the chain Tune → Harmoniser → EQ → Compressor, reverb on a send.
10. **Your track**: the take log over the song in section colours, the lead and voice lines; the advice as live text; with no log, the made-up phrase.

**Theory Guides entry:** area *Singing*; "What is a harmoniser?"; summary "Intervals, fixed and in-the-key harmony, which intervals sound how, doubling, formants, what a harmoniser can't hear, and what the voices made of your track."; opens from "DawIt Harmoniser's window and the singing guide's Harmonies and doubles"; plugin "DawIt Harmoniser"; keywords "harmoniser harmonizer harmony harmonies backing vocals third fifth sixth octave interval diatonic scale steps fixed doubling double tracking adt detune micro pitch shift formant pitch shifter voices". Not on *Start here*. The singing guide's *Harmonies and doubles* gets one paragraph and opens it (text in *Theory*).

## Code
Recipes: *A built-in effect or instrument* (copy DawIt Tune: latency, `KeepsLatencyWhenBypassed`, `SongContextReceiver`, frame FIFO and take log, trace), *Topics in an existing guide* (the singing paragraph), *Checking in the real app*.

- **Voice:** one `tune::PsolaShifter` per voice, fed the mid (mono), with its own ratio; then detune, level, a delay line (0 to 60 ms, not latency) and constant-power pan into the stereo out. The lead is the input delayed by L, the same L as Tune (`BuiltInTune::getLatencyFor`: 1280 samples at 44.1/48 kHz).
- **Shared tracking:** one `pitch::Tracker` per block hop gives period and pitch for all voices, as in Tune's `Controls` (ratio per voice per entry).
- **Pure rules** in `src/engine` (namespace `harm::`, tested): the shift in semitones for a sung pitch, mode, interval, key mask (Key: nearest key note via `tune::chooseTarget` with its hysteresis, moved n scale steps, shift = harmony − reference); the shift is applied to the sung pitch, so voices keep the lead's cents. Fixed: the interval. Key mask with not exactly 7 notes, or no key: C major. Manual Key overrides the song. Interval name text; mix gains (dry = min(1, 2(1−m)), wet = min(1, 2m)); pan gains; starting points; the *Your track* analysis.
- **Voicing gate:** each voice's gain follows "clear pitch and period ≤ the shifter's max period" at its output time, ramped over 10 ms; so chords, noise, consonants and notes below the max period give no voice.
- **Rules:** L constant in every state; bypass and Mix 0 give the input delayed by L bit for bit; voice off or level −∞ costs no shifter output; parameter changes take effect at the next grain (no extra smoothing beyond Level/Pan gain ramps).
- **Threading:** parameters are APVTS atomics; key mask from `setSongContext` (atomic); live lead and voice notes are atomics for the chip; frames (beat, lead, voice notes, level) go through a single-producer FIFO drained by the processor's timer into recent frames and the take log, exactly as Tune.
- **Trace painter:** extend `tunepics::paintTrace` to draw extra voice lines in their colours; Tune's look must not change. One `getVoiceColour (int)` helper.
- **Risk:** CPU (two shifters plus one tracker, about 1.5× Tune); no engine or format change.

## Libraries and models
None: `juce_dsp` FFT and the existing tracker and PSOLA shifter.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | Interval sizes: unison 0, m2 1, M2 2, m3 3, M3 4, P4 5, tritone 6, P5 7, m6 8, M6 9, m7 10, M7 11, octave 12 semitones | definition (as ChordGuide) | exact |
| 2 | Ratio 2^(n/12): +1 1.0595, +3 1.1892, +4 1.2599, +7 1.4983, ±12 2 and 0.5; A4 440 Hz + 4 = 554.37 Hz | derived | exact |
| 3 | C major, 3rd up in the key (+2 steps): C+4 D+3 E+3 F+4 G+4 A+3 B+3 | derived | exact |
| 4 | C major, Fixed +4 gives E F# G# A B C# D#: 4 of 7 outside the key | derived | exact |
| 5 | Major key: every 5th in the scale is 7 semitones except on the 7th degree (B→F, 6); natural minor: on the 2nd (B→F in A minor); the 4th from F (F→B) is 6 | derived | exact |
| 6 | Sixths in a major scale are 8 or 9; thirds 3 or 4 | derived | exact |
| 7 | A 6th below = a 3rd above an octave lower (E4: G4 or G3) | derived; as singing guide | exact |
| 8 | Perfect consonances: unison, 5th, octave; imperfect: 3rds and 6ths; dissonant: 2nds, 7ths; the 4th depends on the bass | Open Music Theory 2.01 | exact (textbook convention) |
| 9 | Classical part-writing avoids parallel 5ths and octaves; voices moving in them blend | Open Music Theory 2.01 | opinion (style rule) |
| 10 | Diatonic harmonisers shift by key and scale so the result stays in key; makers treat out-of-key notes differently | Eventide PitchFactor manual; search: Lexicon, Digitech | typical |
| 11 | DawIt: an out-of-key note takes the nearest key note's interval, ties to the lower, 15-cent hysteresis | DawIt's choice (`tune::chooseTarget`) | opinion |
| 12 | Harmoniser pitch trackers are monophonic, best on single notes | Eventide PitchFactor manual | typical |
| 13 | Unvoiced sounds (s, f, sh, t) have no pitch | as Tune guide | exact |
| 14 | PSOLA keeps formants; quality falls as the shift grows; up to a factor of 2 feasible, most natural within about ±6 semitones | Moulines & Charpentier 1990 (as Tune); search: TAMU lecture, arXiv 2512.16519 | typical |
| 15 | "Ah" formants about 730 and 1,090 Hz; up a 5th by speed: 730 × 1.4983 = 1,093.8 Hz | Peterson & Barney (as Tune); derived | typical / exact |
| 16 | Two identical copies: +6.02 dB (20 log10 2) | derived; as singing guide | exact |
| 17 | Doubling by a few cents of opposing detune and a delay of a few tens of ms; about 4 to 10 cents and 8 to 30 ms | Sound on Sound MicroPitch review; search: vintagedigital.com.au | typical |
| 18 | 7 cents at 220 Hz: 220 (2^(7/1200) − 1) = 0.89 Hz | derived | exact |
| 19 | A delayed copy 5 to 30 ms late fuses with the sound; from about 50 ms (speech) it's an echo | Wikipedia *Precedence effect* | typical |
| 20 | ADT: Ken Townsend, Abbey Road, 1966, for *Revolver* | Wikipedia *Automatic double tracking* | typical |
| 21 | Eventide H910 Harmonizer, 1975, shifts up to an octave each way | Mix (mixonline.com) | typical |
| 22 | Latency 1280 samples: 26.7 ms at 48 kHz, 29.0 at 44.1 | derived; as Tune | exact |
| 23 | Lowest note with voices: max period 1280/2.9 = 441 samples: 100 Hz at 44.1 kHz, 109 Hz at 48 kHz ("about A2") | derived from `Psola.cpp`; as Tune | exact |
| 24 | Constant-power pan centre: cos 45° = 0.7071, −3.01 dB a side | derived | exact |
| 25 | Harmonies sit a few dB under the lead (starting points 6 to 8 dB) | singing guide; DawIt's choice | opinion |

Agreement: 7, 13, 15, 16, 22, 23 match the singing guide and *What is pitch correction?*. The Tune guide's "moving the formants on purpose is a job for a different effect" stays true (no formant control here). No contradiction found.

### Page texts
Name | summary, then text.

**1. What a harmoniser is** | Extra voices made from the one you recorded

A harmoniser listens to one voice, or one instrument playing one note at a time, and sings along with it: it makes copies of the line, moved up or down in pitch, and plays them with the original. From one take you get a harmony, a voice an octave lower, or a wider double of the lead.

DawIt Harmoniser has two voices. Each has its own interval (how far it's moved), level, pan, detune and delay. The original, called the lead here, plays too; Mix sets how much of each you hear.

The picture shows a line in C major, "hold on, hold on to-night", from the singing guide. The lead sings E4, E4, G4, E4, D4, C4. Voice 1, set to a 3rd up in the key, sings G4, G4, B4, G4, F4, E4. Each harmony note is two steps up the scale, so some are 3 semitones above the lead and some 4. That's the difference between Fixed and Key, which the next pages explain.

A harmoniser moves the voice you sang. It doesn't sing other words, change the rhythm or choose a harmony the way a singer would: each voice follows the lead note by note. And it needs one clear note at a time: on chords, a full mix or a noisy take it can't find the note, and the voices go quiet (What it hears).

The first harmonisers were studio boxes. Eventide's H910 Harmonizer of 1975 could move a sound up or down by as much as an octave.

**2. Intervals** | How far apart two notes are, in semitones and in names

An interval is the distance between two notes. The smallest step on a piano, from one key to the very next, is a semitone. Intervals are named by counting letter names: from C up to E is a third (C, D, E: three letters), from C up to G a fifth.

The same name comes in two sizes. C up to E is 4 semitones, a major third; D up to F is 3, a minor third. Both are thirds, because both span three letter names. The common intervals:

- unison: 0 semitones (the same note)
- minor 2nd: 1; major 2nd: 2
- minor 3rd: 3; major 3rd: 4
- perfect 4th: 5
- tritone: 6 (also called an augmented 4th or a diminished 5th)
- perfect 5th: 7
- minor 6th: 8; major 6th: 9
- minor 7th: 10; major 7th: 11
- octave: 12

Moving a note up an interval multiplies its frequency. One semitone multiplies it by 1.0595, four by 1.2599: A4 at 440 Hz becomes C#5 at 554.4 Hz. An octave doubles it, to 880 Hz; an octave down halves it, to 220 Hz. Intervals bigger than an octave sound much like the same interval within it: a third plus an octave (a tenth) still sounds like a third.

Click a row to hear C4 with that interval above it.

**3. Fixed or in the key** | The same number of semitones, or the same number of scale steps

DawIt Harmoniser moves its voices in one of two ways, set by Harmony.

Fixed moves every note by the same number of semitones. Set voice 1 to +4 and every note gets a major third above it: C gets E, D gets F#, E gets G#. That's exact and works on anything, but in a key it soon leaves the key. The picture runs the C major scale through both. With +4, four of the seven harmony notes are outside C major: F#, G#, C# and D#. Fixed suits octaves (12 semitones), fifths on a riff, doubles, and music that doesn't stay in one key.

Key moves every note a number of steps along the scale. A 3rd up is two steps up: C gets E, D gets F, E gets G, F gets A. All the harmony notes are in C major, and the third is a major third (4 semitones) on C, F and G and a minor third (3) on D, E, A and B. That's how a singer thinks of a harmony, and why the singing guide says a harmony follows the key, not a fixed number of keys. Studio harmonisers call this diatonic or scale-based harmony.

The key comes from the song, the key in the transport bar. Under Key you can choose another one for this effect, for a part in a different key from the rest.

A note outside the key, such as F# in C major, gets the interval of the nearest key note: F if it's nearer F, G if it's nearer G, and F (the lower) exactly halfway. A note that wavers near the halfway point keeps its choice until it's clearly past it, so the harmony doesn't flicker between two notes. Harmonisers handle these notes in different ways; this is DawIt's choice.

Each voice keeps the lead's tuning. If the lead sings 20 cents flat, the harmony is 20 cents flat too, so the two stay in tune with each other. Vibrato and slides come through the same way. To tune the lead itself, put DawIt Tune before DawIt Harmoniser.

**4. Which intervals sound how** | Thirds and sixths blend, fifths and octaves thicken

Not every interval makes the same kind of harmony. Music theory sorts them by how stable they sound together.

Thirds and sixths (3, 4, 8 or 9 semitones) are called imperfect consonances: pleasant together, with each voice still heard as its own line. That's why a 3rd above, a 3rd below and a 6th below are the most used harmonies in pop, country and folk. A 6th below is a 3rd above moved down an octave: the lead's E4 gets G4 a 3rd above, or G3 a 6th below.

Unisons, fifths and octaves (0, 7 and 12 semitones) are perfect consonances. They blend so well that two voices moving together in them start to sound like one thicker voice. An octave below gives a lead weight; a fifth above, with or without an octave, is the sound of a power chord on a guitar riff. Classical part-writing avoids parallel fifths and octaves to keep the parts sounding independent; pop and rock use them freely, as a colour.

Seconds and sevenths (1, 2, 10 or 11 semitones) rub: they're dissonances. A whole line harmonised in seconds sounds tense, which is why no starting point uses them. The fourth (5 semitones) sits in between: it sounds settled between two upper voices and unsettled above the bass.

Two voices are often enough. A 3rd and a 5th above make a three-note chord on every note: rich on a chorus hook, heavy across a whole verse.

**5. Where the key isn't enough** | Passing notes, the chords and the one odd fifth

Key harmony knows the scale, not the chords. Most of the time that's fine: most sung notes belong to the chord under them, and a 3rd up from a chord note is usually another chord note. Over a C major chord (C, E, G), a sung E gets G and a sung C gets E.

A passing note between chord notes gets a harmony that may not fit the chord. In the picture the lead sings E4, D4, C4 over C major, and the 3rd up sings G4, F4, E4. The F isn't in the chord and sits one semitone above the chord's E. On a short note it goes by unnoticed, as passing notes do; on a long held note it can rub. A singer would perhaps hold the G there instead. When a long note clashes, try another interval for that part, or sing that bit of harmony yourself.

Fifths have one odd note. In a major key every note of the scale has a perfect fifth (7 semitones) inside the scale except the 7th: in C major, B up a 5th in the key is F, only 6 semitones, a diminished fifth or tritone, the most tense interval in the scale. In A minor the same happens on B. If a Key voice set to a 5th up sounds harsh on one note, that's usually it. A 4th up has the same thing on F (F to B). Thirds and sixths don't: they only ever come in their major and minor sizes.

DawIt Harmoniser doesn't follow the chord track. Your track shows which sizes each voice actually made.

**6. Doubling and width** | The same note again, a little different, for a wider sound

Doubling is the lead twice: the same notes a second time. Singers do it by recording a second take, and the small differences between the takes make the voice sound thicker and smoother (the singing guide's Harmonies and doubles). A plain copy can't do that: two identical copies only make the sound 6 dB louder. A voice at 0 semitones with no detune and no delay is exactly such a copy.

To make a copy sound more like a second take, studios change it a little in two ways:
- Detune moves it a few cents. At A3 (220 Hz), 7 cents is 0.9 Hz, so the copy drifts slowly against the lead instead of locking to it.
- Delay starts it a few milliseconds later, like a second singer who's never exactly together with the first.

A common recipe is two copies, one a few cents up and one a few cents down, each with its own short delay, panned apart, with the lead in the centre. The Wide double starting point does that: +7 and −7 cents, 12 and 20 ms, panned 70% left and right, each 8 dB under the lead. Studio guides typically suggest about 4 to 10 cents and 8 to 30 ms.

Keep the delay under about 30 ms. Up to there the ear hears the copy as part of the same sound; from about 50 ms on a voice it starts to hear a separate echo.

The trick is old. In 1966 Ken Townsend at Abbey Road built ADT (artificial double tracking) for the Beatles' Revolver: the vocal played back from a second tape machine a few milliseconds late, its speed wobbling.

A made double isn't a real second take: it has the lead's breaths, consonants and small slips. For a lead that must sound truly doubled, sing the part again.

**7. Formants and how far to move** | Why the voice keeps its colour, and why small moves sound best

A voice has a pitch, how fast the vocal folds buzz, and formants: the resonances of the throat and mouth that make an "ah" sound like "ah" and your voice sound like you (What is pitch correction? explains them). An "ah" has its first two formants near 730 and 1,090 Hz, whatever note is sung.

Speeding a recording up moves both. Up a fifth (7 semitones, ×1.498), the 730 Hz formant moves to 1,094 Hz and the voice sounds smaller and younger; up an octave it's a chipmunk. DawIt Harmoniser moves its voices the way DawIt Tune corrects notes, with PSOLA: it moves the pitch and leaves the formants where they are. A harmony a fifth up still sounds like an adult singing "ah", as the picture shows.

It still sounds best on small moves. The bigger the move, the more the method shows, as a buzzy or hollow edge: typically most beyond about 6 semitones, and clearest on an octave up or down. Thirds and fourths usually sound natural; octaves are best kept quieter, under the lead. Each voice goes at most an octave each way.

Because the formants stay, a voice made this way sounds like you singing higher or lower, not like a second singer. A real second singer has other formants, which is part of why a choir sounds like many people. DawIt Harmoniser has no formant control.

**8. What it hears** | One clear note at a time, and what happens to the rest

To move a note, DawIt Harmoniser first has to find it, the same way DawIt Tune does: it compares the sound with itself a moment later and looks for the length of one cycle. That needs a sound that repeats: one voice, or one instrument playing one note at a time.

- Chords and full mixes have several notes at once, so there's no one cycle to find. The voices go quiet and only the lead is heard. Studio harmonisers have the same limit: their pitch trackers follow one note.
- Consonants and breaths (s, f, sh, t) are noise with no pitch. The voices fade out on them and back in on the next vowel, so only the lead carries them. That keeps the s sounds from piling up into a hiss, the same reason singers sing the end consonants of doubles lightly.
- Very low notes, below about A2 (110 Hz), get no voices: one cycle is too long for the short time it hears ahead. That only touches a bass line or the lowest notes of a deep voice.
- Reverb and delay before it blur the cycles. Put it before them.

It hears a little ahead, so inside it the track is about 27 milliseconds late (29 at a 44.1 kHz sample rate). On an audio track DawIt plays the clips that much earlier, so the track stays in time. On an instrument track, a bus or a return it can't, and everything on it plays late: use it on the audio track itself.

**9. Using it well** | Tune first, voices under the lead, harmony where it counts

1. Start from a clean, in-tune lead. The voices copy its tuning, so tune it first if it needs it: DawIt Tune, then DawIt Harmoniser, then EQ and compression, with reverb on a send.
2. Choose Key for harmonies, Fixed for octaves, fifths on a riff, and doubles.
3. Keep the voices under the lead. Harmonies usually sit a few dB below it; the starting points put them 6 to 8 dB down. If the harmony is easier to follow than the lead, it's too loud.
4. Spread them: two voices panned a little left and right leave the centre to the lead.
5. Use them where they count. Harmonies usually come in the chorus, on the hook or on the last words of lines. DawIt Harmoniser works on the whole track: for a chorus-only harmony, copy the chorus clips (Cmd-drag) to a second audio track, put DawIt Harmoniser there with Mix at 100%, so it plays only the voices, and leave the lead's track as it is. The harmony then has its own fader.
6. Listen with the chords. A harmony that sounds fine alone can rub with a chord (Where the key isn't enough).
7. Treat it as a sketch. It sounds like the same singer, with the same breaths. For a final record many singers learn the part the harmoniser found and sing it.

**10. Your track** | What the voices made of this track

DawIt Harmoniser listens while the song plays and keeps what it heard: the lead's note and each voice's note. This page sums that up. It only knows what has played through it, so play the song through once; playing a part again replaces what it heard there. What it heard isn't saved with the project.

**Singing guide, *Harmonies and doubles*,** add before the last paragraph: "DawIt Harmoniser can make a harmony or a double from the lead itself, to try a part before you sing it (What is a harmoniser?)."

### Controls: explainer text and tooltips
- **Harmony** (Fixed, Key): "Fixed moves every note by the same number of semitones. Key moves it a number of steps along the scale, so the voices stay in the key: a 3rd up is sometimes 3 semitones, sometimes 4."
- **Key** (Song, C … B): "The key Key harmony follows. Song uses the key in the transport bar; choose a note for a part in another key."
- **Scale** (Major, Minor): "Major or natural minor, for a key chosen here. With Song the song's own scale is used."
- **Mix** (0–100%, 50): "How much of the lead and the voices you hear. At 50% both play at full level; at 0% only the lead; at 100% only the voices, to put a harmony on a track of its own."
- **On**: "Turns this voice on or off."
- **Interval, Fixed** (−12 … +12 semitones; voice 1 +4, voice 2 +7): "How far this voice is moved, in semitones: +4 is a major 3rd up, +7 a 5th up, −12 an octave down."
- **Interval, Key** (−7 … +7 steps; voice 1 +2, voice 2 −2): "How far this voice is moved along the scale: 3rd up is two steps, 5th up four, an octave seven."
- **Level** (−30 … +6 dB, −6): "How loud this voice is. Harmonies usually sit a few dB under the lead."
- **Pan** (100% left … 100% right, 0): "Where this voice sits between the speakers. In the centre it is 3 dB quieter in each, so it sounds equally loud wherever it is."
- **Detune** (−50 … +50 cents, 0): "Moves the voice a few cents off its note. 4 to 10 cents makes a copy drift against the lead like a second take."
- **Delay** (0 … 60 ms, 0): "Starts the voice later. 8 to 30 ms makes a double; from about 50 ms it's heard as an echo."

### Starting points (each with why)
| Name | Settings | Why |
|---|---|---|
| Third above (default) | Key; V1 +2 steps, −6 dB, centre; V2 off; Mix 50% | "The most used harmony: two steps up the scale, under the lead." |
| Thirds above and below | Key; V1 +2, V2 −2 steps; −7 dB each; pan 40% left and right | "A harmony each side of the lead, for a fuller chorus." |
| Third and fifth above | Key; V1 +2 (−6 dB, 30% L), V2 +4 steps (−8 dB, 30% R) | "A three-note chord on every note. Rich on a hook; watch the 7th of the scale (Where the key isn't enough)." |
| Octave below | Fixed; V1 −12 semitones, −8 dB, centre; V2 off | "Weight under a lead without changing its notes." |
| Fifth and octave | Fixed; V1 +7 (−6 dB), V2 +12 (−10 dB), centre | "The power-chord sound, for a single-note guitar or synth riff." |
| Wide double | Fixed; V1 0, +7 cents, 12 ms, 70% L, −8 dB; V2 0, −7 cents, 20 ms, 70% R, −8 dB | "Two slightly different copies either side make the lead wider, like a second take." |

### Example data
- Page 1/4: lead E4 E4 G4 E4 D4 C4 (MIDI 64 64 67 64 62 60), one beat each at 90 BPM; 3rd up G4 G4 B4 G4 F4 E4 (67 67 71 67 65 64; +3 +3 +4 +3 +3 +4); 6th below G3 G3 B3 G3 F3 E3; 5th above B4 B4 D5 B4 A4 G4 (all +7); octave below −12.
- Page 3: C4…C5; Fixed +4: E4 F#4 G#4 A4 B4 C#5 D#5 E5 (out: F#, G#, C#, D#); Key +2: E4 F4 G4 A4 B4 C5 D5 E5.
- Page 5: C major chord bar; lead E4 D4 C4 (2 beats each), voice G4 F4 E4, F4 marked "1 semitone above E"; B4 (493.88 Hz) → F5 (698.46 Hz), 6 semitones.
- Page 6: copies at +7 cents/12 ms/−70%, −7 cents/20 ms/+70%, −8 dB; time axis 0–60 ms with marks at 30 and 50.
- Page 7: 220 Hz harmonics under an envelope with peaks 730 and 1,090 Hz; speeded up a 5th: 329.6 Hz harmonics, peaks 1,093.8 and 1,633.2 Hz; PSOLA: 329.6 Hz harmonics, peaks 730 and 1,090.
- Page 10 with no log: the page 1 phrase.

### "Your track" rules
Over the take log's frames while playing; "loud" = level above −50 dBFS; a voice note is the lead's fractional pitch plus its shift.
1. **No log:** "Play the song once with DawIt Harmoniser on: this page fills with what it heard."
2. **Few clear notes:** loud ≥ 2 s and clear pitch on < 50% of loud frames: "Only 35% of the sound had one clear note, so the voices were quiet most of the time. DawIt Harmoniser follows one voice or one single-note line; on chords, a full mix or a noisy recording it can't find the note." Quiet at ≥ 50%.
3. **Sizes (Key, information):** per voice on, voiced ≥ 2 s: "Voice 1 (3rd up) made a minor 3rd on 58% of the notes and a major 3rd on 42%: the key decides which." Always shown when the condition holds.
4. **Diminished fifth (Key):** a voice's shift is ±6 semitones on ≥ 5% of its voiced frames: "Voice 2 (5th up) made a diminished 5th (6 semitones) on 12% of the notes, on B in C major. If it sounds harsh there, try a 3rd or a 6th, or sing that line yourself." Quiet below 5%.
5. **Fixed out of key:** song key known, interval not a multiple of 12, voice notes (nearest semitone) outside the key on ≥ 20% of voiced frames: "In Fixed, voice 1 (+4) landed outside C major on 40% of the notes (F#, G#). Try Harmony: Key to keep the voices in the key, unless you want that sound." Quiet below 20%.
6. **Lead out of tune:** voiced ≥ 2 s and the lead's mean distance from the nearest semitone > 25 cents: "The lead is on average 30 cents from the nearest note, and the voices copy that. Put DawIt Tune before DawIt Harmoniser to tune the lead first." Quiet at ≤ 25.
7. **Latency:** on a bus, return or instrument track: the lead line's sentence.
8. Nothing to advise: "The voices followed the lead on 92% of the loud sound, all inside the key."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Fixed +4 on sung A3 (220 Hz), voices only | 277.18 Hz | ±5 cents |
| Fixed +7 / −12 on A3 | 329.63 / 110.00 Hz | ±5 cents |
| Key C major +2 steps: E4 / F4 sung | G4 392.00 / A4 440.00 Hz | ±5 cents |
| Key C major +4 steps on B4 | F5 698.46 Hz | ±5 cents |
| `harm` shift table C major +2 steps, C…B | 4 3 3 4 4 3 3 | exact |
| `harm` shift C major +4 steps, C…B | 7 7 7 7 7 7 6 | exact |
| F#4 exactly, Key +2 in C major | +4 (F's), A#4 466.16 Hz | exact / ±5 cents |
| A3 30 cents flat, Fixed +4 | 272.45 Hz (C#4 −30 cents) | ±5 cents |
| Unison, detune +7 cents on A3 | 220.89 Hz | ±2 cents |
| Delay 20 ms on a tone burst | voice onset L + 20 ms after input | ±3 ms |
| Reported latency at 48 kHz / 44.1 kHz | 1280 samples | exact |
| Mix 0, and bypass: output vs input delayed by L | identical | bit for bit |
| Mix gains at 0 / 50 / 100% (dry, wet) | (1,0) (1,1) (0,1) | exact |
| Pan centre: each side vs mono voice | −3.01 dB | ±0.05 dB |
| Pan 100% left: right channel | silent | ≤ −120 dBFS |
| Silence in | silence out | ≤ −120 dBFS |
| White noise, Mix 100% | voices ≥ 40 dB below input RMS | — |
| Sung 80 Hz tone (below max period) | no voice (Mix 100% ≤ −60 dB rel.) | — |
| Key with no song key | C major (E4 → G4) | ±5 cents |
| Manual A minor, +4 steps on B3 | F4 349.23 Hz | ±5 cents |
| Fixed +4 out-of-key share on C major scale | 4/7 = 57% | exact |
| Interval names | +4 "major 3rd up", −12 "octave down", +2 steps "3rd up", −5 steps "6th down" | text |
| Starting points | 6, each with why; default *Third above* | — |
| Advice (made-up logs) | rule 2 at 35% voiced; rule 4 at 12%; rule 5 at 57%; rule 6 at 30 cents; quiet cases below thresholds | — |
| Guide | 10 pages, each with text; search finds it for "harmonizer", "doubling", "diatonic" | — |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `src/engine/Harmoniser.{h,cpp}` (`harm::`): shift rules (Fixed, Key with hysteresis, C major fallback, manual key), interval names, mix and pan gains, settings, starting points, the frame type and the *Your track* analysis (rules 2 to 8). Tests: the `harm` table rows and advice rows. Check: tests pass.
- [x] 2. `BuiltInHarmoniser.{h,cpp}`: tracker, per-voice shifters, voicing gate, detune, delay, pan, mix, latency and bypass as Tune, `SongContextReceiver`, frame FIFO, timer drain, take log, test sound; `dawit_add_builtin (Harmoniser … CODE Dhrm, "Fx|Pitch Shift", APP_ONLY)`; editor function null for now. Tests: every sound row, `checkThroughProject`, registry. Check: tests pass.
- [x] 3. `src/generate/HarmoniserGuideText.{h,cpp}` (page names, summaries, texts, control texts) and the singing guide paragraph; `gen::guides` entry with keywords. Tests: 10 pages with text, search words. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 4. `tunepics`: extra voice lines in the trace and the voice colour helper; `HarmoniserEditor` on `fxui::EffectEditor` with the groups, the *now* column, the trace (hover, click, right-click, `view.harmoniserTraceSeconds`), the lead lines; set `createEditorFunction` in `MainComponent`.
- [x] 5. `HarmoniserGuide` (`fxui::Guide` pages with the pictures from *Example data*, clicks that play, *Your track* via the host-track lookup, `setPrintSource`); `BuiltInGuides` branches and `MainComponent::openGuide` case; the singing guide's *Harmonies and doubles* opens it.
- [x] 6. Device bar chip picture (`chainpictures`) and gain-staging badge as for Tune.
- [x] 7. Build; `--export-guides` passes. Done when: `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 8. Real-app check with `--screenshots`: the demo vocal (or an audio track) with DawIt Harmoniser on *Third above*, playing 4 bars: the window (now column with lead and voice, trace with two lines); *Wide double* selected; the guide at *Fixed or in the key* and *Your track*; the device bar chip. Add a `ScreenshotRunner` scene to add the effect to a track if none exists.
- [x] 9. Docs: README *Shape and mix sounds* mentions DawIt Harmoniser; `docs/tests.md` and `docs/code-layout.md` lines; `docs/architecture.md` built-ins list (Harmoniser reuses Tune's tracker and shifter; chip pictures list); issue #122 progress comment; note on #69 that the trace painter is shared.

## Tests
As the *Test numbers* table: sound measured through the processor at 48 kHz with `tune::makeTake` input and `pitch::Tracker` on the output (voices only, Mix 100%, after L), the pure `harm` rules and advice on made-up logs, latency and bypass impulse tests copied from `runTuneEngineTests`, persistence, starting points and the guide's text and search.

## Website hand-off
- `features.html`: the effects group, beside DawIt Tune (adds harmonies, octaves and doubles in the song's key).
- `learn.html`: *What is a harmoniser?* in the Singing area.
- Screenshots: the window on a vocal with *Third above* while playing; the guide at *Fixed or in the key*.
- No library: `about.html` unchanged.

## Left for later
- Chord-aware harmony from the chord track (decision 1), and MIDI-controlled voices (play the harmony notes on a keyboard; ties in with #121).
- A formant control (deeper or brighter voices), and humanised timing per voice.
- More voices (decision 2) and per-section use without a second track (automation).
- A plugin build (decision 4); latency compensation on buses and returns (#69).
- A range warning when a voice goes beyond a singer's range.

## Changes while building

Group A (engine) built. Hand-off for Group B:
- Rules and analysis: `src/engine/Harmoniser.{h,cpp}` (`harm::`): `Settings`/`Voice`, `getPresets()`, `getKeyMask (key, scale, songMask)`, `getShift`, `getIntervalName (Mode, value)` ("major 3rd up", "3rd up"), `getSizeName`, `getNoteName`, `getLevelGain`, `getMixGains`, `getPanGains`, `Frame` (beat, lead, voice[2], rmsDb), `KeyInfo {known, mask, name}`, `getAdvice (frames, settings, keyInfo, framesPerSecond, latencyLine)`, `getLatencyLine ("bus" | "return" | "instrument track")`, `getPhraseNotes()` and `phraseSeconds` (the test phrase, 90 BPM).
- Processor: `src/engine/BuiltInHarmoniser.{h,cpp}`: `getSettings/setSettings/applyPreset`, `getKnobParameter (harm::Knob)` and `(voice, harm::VoiceKnob)`, `getParameterId`, `liveLead`, `liveVoice[2]`, `getRecentFrames()`, `getTakeLog()`, `getFramesPerSecond()`, `getEffectiveKeyMask()` (key chosen here or the song's, C major fallback), `getSongKeyMask()`, `clap` (`fx::Clap::Sound::sungLine` is the new test sound: B must add its button text "Play a sung line" and width in `EffectWindow.cpp` lines near 1366 and 1477), `createEditorFunction` (null for now; CMake already names `BuiltInHarmoniserEditor` in `ui/HarmoniserEditor.h`, which B must create; APP_ONLY).
- Guide text: `src/generate/HarmoniserGuideText.{h,cpp}` (`gen::harmoniserguide`: `getPageName/Summary/Text`, `getControlText (Control)`, `getSingingParagraph()`); `gen::guides::Guide::harmoniser` is in `TheoryGuides` (area Singing, plugin "DawIt Harmoniser"); the singing guide's *Harmonies and doubles* already has the new paragraph (B makes it open the guide).
- To wire in B: `G::harmoniser` is a placeholder in the plugin-guide groups of the `switch (guide)` in `MainComponent.cpp` (two) and `ArrangementView.cpp` (two) next to `G::tune`, and returns "harmoniser" in `DevScreenshots.cpp::getGuideKey`; give them real branches.
- Decisions made while building: Level's bottom (-30 dB) means silence (so "level -inf" is a real value); a voice that is off, silent or Mix 0 only takes the input (`PsolaShifter::bypass`), no shifter output; processing runs in chunks of 1024 samples; `getAdvice` judges rules 3 to 5 only after two seconds of clear notes per voice (the plan gives that minimum only for rule 3); on rule 3 a single size reads "made a perfect 5th on every note"; the Key choice of the plan is a choice parameter (Song, C ... B).
- Texts that are mine (not in the plan's *Theory*): the parameter display names ("Voice 1 interval (semitones)" ...), the single-size sentence in rule 3 above. Nothing contradicted the plan's Theory.
- Not touched: README, docs, SOURCES.md (Group C); `Harmoniser.cpp` isn't in the plugin shared sources (app only).

Group B (UI) built; `./build.sh --test` passes with no warnings and `--export-guides` passes (What is a harmoniser.pdf made). Not yet looked at in the running app. Hand-off for Group C:
- Files: `src/ui/HarmoniserEditor.{h,cpp}` (`HarmoniserView`, `BuiltInHarmoniserEditor`), `src/ui/HarmoniserGuide.{h,cpp}` (`harmtext::makeGuide`, `getYourTrackText`); `tunepics` gained `getVoiceColour (int)`, `harmoniserColour` and `Trace::extraLines` (Tune's look is unchanged: extra lines are only drawn when set, and then `fixedLow` is ignored and the rows grow to hold every line). Wired into `MainComponent` (editor function, `arrangement.onShowHarmoniserGuide` opens the guide), `TheoryGuidesWindow.cpp` (`makePages`), `ChainPictures.cpp` and `DeviceBar.cpp` (the chip), `CMakeLists.txt` (UI_SOURCES). The `G::harmoniser` placeholder cases in `MainComponent` and `ArrangementView` were already right (they share Tune's branch, which goes through `BuiltInGuides`), so they are unchanged.
- Changes from the plan: (1) the knob area has two rows (Harmony and Voice 1 on the first, Voice 2 on the second): 16 cells in one row are about 50 px each and cannot show "+4 · major 3rd up". Voice 2's cells are therefore wider than Voice 1's; check how it looks. (2) `Guide::Picture` has no clicks, so "click plays it" and "click a row to hear C4" are not built: pages 1, 4, 8 and 10 use the guide's *Play a sung line* button (the sung line through the effect); pages 2, 3, 5, 6, 7 and 9 have no sound button. (3) The singing guide opens the harmoniser guide through a button "What is a harmoniser?" under the text of *Harmonies and doubles* (`SingingExplainer::onOpenHarmoniserGuide`, wired by `ArrangementView::onShowHarmoniserGuide`). (4) No separate gain-staging badge code was needed: the badge is generic. (5) The Listen button is hidden (no "hear only" mode) and the header button text for `Sound::sungLine` is set in `EffectWindow.cpp`.
- Texts that are mine: the explainer texts of *Now*, *Trace* and *Level* topics and the overview of the window, the singing-guide button label, and the picture captions and labels of the guide pages (taken from the plan's example data). The "no song key" lead line shows when Harmony is Key, Key is Song and the song's key does not have 7 notes.
- Group C should look at: the window (two knob rows, the now column, the trace with two voice lines), the guide pages (especially *Where the key isn't enough*, *Doubling and width* and *What it hears*, which are drawn by hand and unchecked by eye), *Your track* with a log, and the chip. `ScreenshotRunner` still has only the placeholder `getGuideKey` branch for `G::harmoniser`; a scene that adds the effect to a track is Group C's step 8.

Group C (check and docs) built. Looked at in the running app (`--screenshots`, scale 1): window on Third above and on Wide double, the guide at *Fixed or in the key*, *Where the key isn't enough*, *Doubling and width*, *What it hears*, *Your track* (with a log), the singing guide's button and the device bar chip. All fine except one fix: the time axis labels of the *Doubling and width* picture were drawn 30 px off their ticks (now centred on them). Voice 2's wider knob row reads fine and was left as it is.
- New `harmoniser` scene in `ScreenshotRunner` (the "Sung take" track with DawIt Harmoniser on Third above, played through once so its take log is full; `setUpTuneScene (…, harmoniser)`), and a plugin shot may set `"preset"` and `"clap": true` for it. Documented in `DevScreenshots.h` and `recipes.md`.
- Docs: README (*Shape and mix sounds*), `docs/tests.md`, `code-layout.md`, `architecture.md`, `ui-design.md`, `recipes.md`. SOURCES.md already lists every URL of the plan's Theory. Note for #69: the trace painter is shared (`Trace::extraLines`); the Harmoniser still has no latency compensation on buses and returns, like Tune.
- Website hand-off: `features.html` effects group beside DawIt Tune; `learn.html` *What is a harmoniser?* in Singing; screenshots worth taking: the window on Third above while playing (`plugin: DawIt Harmoniser, track: Sung take, clap: true, wait: 2500, capture: window` after `scene: harmoniser`) and the guide at *Fixed or in the key* (topic 2); no library, `about.html` unchanged.

Theory check:
- `HarmoniserGuideText.cpp` *Intervals*: removed "Click a row to hear C4 with that interval above it." (rows don't play; Group B change 2).
- *Using it well*: "the starting points put them 6 to 8 dB down" is now "6 to 10 dB" (*Fifth and octave* puts its octave at −10 dB).
- *Where the key isn't enough*: the tritone is no longer "the most tense interval in the scale" (the minor 2nd B–C is at least as harsh); now "which sounds tense and unsettled".
- *Doubling and width*: a 0-semitone voice is "close to" an identical copy, not "exactly" one (level, voicing gate and PSOLA make it not bit-identical).
- `Harmoniser.cpp` advice article: "an unison" fixed to "a unison" (vowel list without "u").
- SOURCES.md: the doubling-delay search that confirms "under about 30 ms".
- Open (code, not changed): rule 4 and rule 3 call every 6-semitone shift a "diminished 5th", but on a 4th up or 4th down in Key (F to B) it is an augmented 4th; the latency advice for an instrument track (rule 7) isn't produced by `HarmoniserGuide` (only bus and return); the Delay guide says doubling is 20–50 ms, while this guide says keep it under about 30 ms.
- Theory-check fixes (code): (1) advice rules 3 and 4 name a 6-semitone Key move by the voice's steps (3 steps: augmented 4th; 4 steps: diminished 5th; Fixed unchanged), tested with a 4th-up voice; (2) the instrument-track latency line now shows in `HarmoniserGuide` and `HarmoniserEditor` (`fxui::HostTrack::isInstrument`, set from `Track::isMidi()` in `MainComponent`); (3) `harm::getLatencyLine (kind, lateMs)` says the real value for the rate in use (29 ms at 44.1 kHz, 27 at 48) and "about 27 ms" when the rate isn't known; tested.
