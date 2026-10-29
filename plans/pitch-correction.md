# Pitch correction and pitch check effect

Status: Built, not committed; Theory checked, with open points
Idea: [Pitch correction and pitch check effect](../FEATURE_IDEAS.html#pitch-correction-and-pitch-check-effect)

## Goal
*DawIt Tune*, a built-in effect for one voice or one single-note line. In *Check* mode it only measures: the note, how many cents sharp or flat, and whether it's in the song's key. In *Correct* mode it also moves each note towards the nearest allowed note, lightly or as the hard-tuned pop sound. It teaches the difference between a note a few cents out and a wrong note, and its *Your track* page says where a take is out ("the verse sits 18 cents flat on average").

## Decisions for you

**Answered by the user, all as recommended:** Check and Correct with the Key, Chords and Chromatic targets (Melody, a Formants switch and Check pitch... later); latency reported and compensated per track in the engine; formants always kept, default Check only, guide in the Singing area, the two Auto-Tune numbers kept as typical and checked by dawTheory. Build: not yet.
1. **Scope.** Options: (a) Check mode only; (b) Check and Correct with Key, Chords and Chromatic targets, Speed, Amount, Tolerance, Keep vibrato, formants always kept, and the engine's latency compensation; (c) also the Melody target, a Formants switch and *Check pitch...* on a clip. **Recommended:** (b): (a) is a corrector that can't correct; Melody needs a track chooser and MIDI on the audio thread, and *Check pitch...* is a long job *Practise singing* also wants. Both reuse what (b) builds.
   Answer:
2. **Latency.** The tracker and shifter hear about 27 ms ahead. Options: (a) report it, and the engine reads that track's clips earlier, with a pre-roll after a jump (also helps Saturation, Distortion and third-party plugins); (b) no compensation, the track plays 27 ms late; (c) a low-latency design that corrects each note's first 20 to 40 ms late and moves formants. **Recommended:** (a), the riskiest step (an audio-thread change in `Track::process`), but 27 ms is a fifth of a 16th at 120 BPM. Buses and returns stay uncompensated; the window says so.
   Answer:
3. **Formants.** Options: (a) always kept (PSOLA does it), no control; (b) a Formants switch that can move them. **Recommended:** (a): corrections are under a semitone, where kept formants are what everyone wants.
   Answer:
4. **Default when added.** *Check only* or *Natural fix*. **Recommended:** *Check only*: measuring first is what the guide teaches, and the lead line says how to correct.
   Answer:
5. **Theory Guides area.** *Singing* or *Effects*. **Recommended:** *Singing* ("singing in tune, and recording it").
   Answer:
6. **Numbers read only in summaries** (worded as typical; for dawTheory): Auto-Tune's "10 to 50 ms for natural correction, 0 for the hard effect" (Antares blog, Antelope's guide; the manuals' PDFs couldn't be read) and its 1997 release (one source says 1996). **Recommended:** keep as worded.
   Answer:

## What the user does and sees
- **Device bar:** DawIt Tune is in the effect list; its chip (`ChainPictures`) shows the current note and a small cents needle in the zone colour, "–" with no clear pitch.
- **Window** (`fxui` frame, 1240 × 640): header with the starting points, guide button, test sound *Play an off note* and toggle *Hear the original*. Controls: Mode switch (Check, Correct), Target Type switch (Key, Chords, Chromatic), Speed (0 to 400 ms, default 50), Amount (0 to 100%, default 100), Tolerance (0 to 50 cents, default 10), Keep vibrato (on). In Check mode the correction controls are dimmed but usable.
- **Picture:** the **tuner** (big name of the nearest semitone, needle −50 to +50 cents in the zone colour, "18 cents flat", "in A minor" or "not in A minor: F#") and the **trace**: the last 8 s on piano-roll rows (the key's rows shaded), centred on the recent notes ± 7 semitones, the sung line coloured by zone, the corrected line in `accent`, gaps for no clear pitch; the frame's level history below. Lead line: in Check "Only measuring: switch Mode to Correct to pull notes into tune."; on a bus or return "On a bus, DawIt Tune makes everything in it 27 ms late. Put it on the vocal track itself."
- **Trace interaction:** hover "Bar 9 · A3 · 18 cents flat · in G major" (no bar while stopped); click moves the playhead there; right-click *Show 4 / 8 / 16 seconds*, then *What is pitch correction?*. Tooltips and explainer texts are in *Theory*.
- **Remembered:** `view.tuneTraceSeconds`.

## Data and saving
No project data or format change: the parameters save in the plugin state like DawIt Gate's, and a missing one loads its default. *Hear the original* is an unsaved atomic; the take log lives in the processor for the session. Test: `checkThroughProject<BuiltInTune>` with every parameter off its default.

## The guide
*What is pitch correction?*, an `fxui::Guide` in `src/ui`; page names, summaries and texts in *Theory*, picture data in *Example data*. Pages: 1 What pitch correction is (the example phrase, run through `tune::`); 2 Cents (A4 to A#4 as 100 cents, the 10 and 25 bands, the Hz marked); 3 Finding the pitch (a buzz with its period, the tracker's match curve, noise beside it); 4 Target notes (C4 to C5 rows, C major shaded, the F# arrows and the hysteresis band); 5 Speed, amount and tolerance (the scoop at three speeds); 6 Vibrato (Keep vibrato on and off); 7 Formants (harmonics under the envelope: played faster versus PSOLA); 8 Hard tune (the staircase); 9 Using it well (the chain Tune → EQ → Compressor → reverb send); 10 Your track (the take log over the song in section colours, each section's mean cents as a bar; the advice as `liveText`; with no log, the made-up take).

**Theory Guides entry:** area *Singing*; "What is pitch correction?"; summary "Cents and what ears notice, how a tuner finds the pitch, target notes, speed and amount, vibrato, formants, hard tune, using it well, and how in tune your track is."; opens from "DawIt Tune's window and the singing guide's Singing in tune"; plugin "DawIt Tune"; keywords "tune tuner tuning autotune auto-tune pitch correction intonation flat sharp cents retune speed humanize vibrato formant chipmunk psola melodyne t-pain cher believe robot hard scale snap". Not on *Start here*. The singing guide's *Singing in tune* gets one paragraph (in *Theory*).

## Code
Recipes: *A built-in effect or instrument* (copy DawIt Gate: `gate::`, `gatetext`, `liveText`, the host lookup), *Topics in an existing guide*, *Checking in the real app*.

- **Pitch tracker** (`src/engine`): McLeod's method as in `soundguide::estimatePitch`, through `juce::dsp::FFT`, allocation-free after prepare. Window W = power of two ≥ 40 ms (2048 at 44.1/48 kHz, 4096 at 88.2/96), hop W/8, 50 Hz to 2 kHz; gives pitch (fractional MIDI) and clarity. Voiced at clarity ≥ 0.8 and RMS ≥ −50 dBFS (estimates). Octave guard: a lone hop ~12 semitones from the last two is dropped. `estimatePitch` becomes a wrapper; its tests pass unchanged.
- **`tune::`** (`src/engine`, the processor needs it): cents, target from a 12-bit mask with 15-cent hysteresis (ties to the lower note), tolerance, amount, glide (one pole, τ = Speed/3), vibrato centre (2-pole low-pass at 1.5 Hz, restarted after a gap or a jump over 100 cents within 20 ms), starting points, a made-up sung take generator ("ah" buzz with a cents curve per note) for tests, the test sound and the scene, and the simulator the pictures run.
- **Shifter:** TD-PSOLA on the mid, the same grains applied to both channels: marks one period apart nudged to the largest sample within ±¼ period, Hann grains two periods long, output marks period / ratio apart. Unvoiced or |shift| < 1 cent: the delayed dry signal bit for bit; 5 ms crossfades.
- **Rules:** L = W/2 + hop (1280 samples at 44.1/48 kHz, 2560 at 96), constant in every mode; Check, *Hear the original* and bypass give the input delayed by L (`processBlockBypassed`, also in Saturation and Distortion). No match level (level unchanged). Chords uses Key in bars without a chord.
- **Song context** (generic): `RenderContext` already carries the key mask; add the chord mask of the bar at the segment start (read under the lock the audio thread holds). Plugins opt in through a small engine interface, cast once into `PluginSlot`; `Track::process` passes the masks before `processBlock`.
- **Latency compensation** (risky): a track with clips reads its clips L_track samples ahead (sum of its plugins' `getLatencySamples()`, bypassed included), through the tempo map and the loop. After a start, seek, loop wrap or offline start (not note edits) it first feeds its chain the skipped L_track samples, discarded, in chunks ≤ the block size. Buses, returns and live MIDI aren't shifted.
- **Threading:** parameters and the live reading are atomics; trace frames go through a single-producer FIFO that a message-thread timer in the processor drains into the take log, so *Your track* fills with no window open.
- **Take analysis** in `src/generate` beside `gen::singing` (its `getNoteName`), reused by *Practise singing*.
- **UI:** one tuner painter and one trace painter with one zone-colour helper, shared by window, guide, chip and later *Check pitch...*.

## Libraries and models
None: JUCE's FFT (`juce_dsp`, linked) and plain DSP. Basic Pitch (`AudioToMidi`) is a polyphonic model for whole clips, too slow for the audio thread.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | A semitone is 100 cents, an octave 1,200; cents = 1200 log2 (f2/f1) | Ellis (1885), definition | exact |
| 2 | A4 = 440 Hz | ISO 16 | exact |
| 3 | 30 cents below A4: 440 · 2^(−30/1200) = 432.44 Hz; 20 below: 434.95 Hz; 50 above: 452.89 Hz | derived | exact |
| 4 | 20 cents at A2 (110 Hz): 110 (1 − 2^(−20/1200)) = 1.26 Hz | derived | exact |
| 5 | Steady tones one after the other: differences of a few cents (about 3 to 10) can be heard, depending on pitch, level and listener | Moore, *An Introduction to the Psychology of Hearing*; Micheyl et al. (2012); about 0.2% ≈ 3.5 cents at best | typical |
| 6 | A sung note is heard as out of tune by many listeners from about 20 to 25 cents | Larrouy-Maestri (2018); Vurma & Ross (2006); as the singing guide | typical |
| 7 | Short notes are heard less exactly than long ones | Micheyl et al. (2012), duration effect | typical |
| 8 | DawIt's bands: in tune ≤ 10 cents, close ≤ 25, out > 25 | DawIt's choice, built on 6 | opinion |
| 9 | Vibrato about 5 to 7 Hz, classical extent about 50 to 75 cents each way; the ear hears the middle | Prame (1994, 1997); as the singing guide | typical |
| 10 | Period of 220 Hz = 4.545 ms; 2 and 3 periods 9.09 and 13.64 ms | derived | exact |
| 11 | McLeod's method (normalised square difference), 2005; YIN (de Cheveigné & Kawahara, 2002) is a relative | McLeod & Wyvill, ICMC 2005; JASA 111 (2002) | exact |
| 12 | Unvoiced sounds (s, f, sh, t) are noise with no repeating cycle, so no pitch | phonetics, any textbook | exact |
| 13 | Trackers make octave errors, especially with a weak fundamental | de Cheveigné & Kawahara (2002) | typical |
| 14 | "Ah" formants about 730 and 1,090 Hz (adult male averages) | Peterson & Barney (1952); as the singing guide | typical |
| 15 | Playing faster by a semitone multiplies every frequency by 2^(1/12) = 1.0595: 730 → 773 Hz; an octave: 1,460 Hz | derived | exact |
| 16 | TD-PSOLA moves pitch while keeping the spectral envelope, best for small shifts of one voice | Moulines & Charpentier, *Speech Communication* 9 (1990) | typical |
| 17 | Speed = time to 95%: 1 − e^−3 = 0.950; after Speed/3, 1 − e^−1 = 0.632 | derived | exact |
| 18 | 2-pole Butterworth low-pass at 1.5 Hz passes 1/√(1 + (5.5/1.5)^4) = 0.074 (−22.6 dB) at 5.5 Hz | derived | exact |
| 19 | Latency 1280 samples = 26.7 ms at 48 kHz, 29.0 ms at 44.1 kHz; a 16th at 120 BPM is 125 ms | derived | exact |
| 20 | Antares Auto-Tune, by Andy Hildebrand, released 1997, based on autocorrelation | Britannica *Auto-Tune*; Antares | typical (year varies by source) |
| 21 | Cher's *Believe* (1998) made the hard effect famous, retune speed at zero | Antares blog; Britannica | typical |
| 22 | Auto-Tune users often start natural correction at about 10 to 50 ms; 0 to a few ms gives the hard effect | Antares blog; Antelope Audio guide (manuals unread) | typical |
| 23 | Put correction first, on the dry vocal | Antares blog | opinion |
| 24 | A minor's natural scale has G; minor songs often use G# (harmonic minor's raised 7th) | any harmony textbook; `MusicalKey` scales | exact |
| 25 | In C major, F# lies between F and G (C major has F) | definition | exact |

Agreement: facts 6, 9 and 14 match the singing guide's *Singing in tune*, *Vibrato* and voice picture. The idea's "about 5 cents are hard to hear" is fact 5 (side by side) and must not be stated as the in-song threshold. The idea's example "an F in C major" is wrong (F is in C major); the plan uses F#.

### Page texts

**1. What pitch correction is** — *Measuring each sung note, and moving it when asked*

Pitch correction listens to one voice or instrument, finds the pitch of each note as it's sung, and can move it to the note it should be. DawIt Tune does both. In Check mode it only measures: the note, how many cents it's sharp or flat, and whether it belongs to the song's key. In Correct mode it also moves each note towards its target.

The picture shows a short line sung in A minor: E4, G4 and A4. The sung line is green where it's within 10 cents of a note, yellow within 25 and red beyond; the shaded rows are the notes of the key; the blue line is what DawIt Tune makes of it with the Natural fix starting point. The E starts 150 cents low and slides up: the slide is kept, and the note settles on E. The G sits 35 cents flat and is pulled up to G, with its vibrato kept. The A starts right and drifts 30 cents flat; the correction follows it back up.

A note can be wrong in two ways. It can be a little out, sharp or flat by some cents, on the right note. Or it can be the wrong note. Pitch correction fixes the first. It can't know which note you meant: it moves each note to the nearest target, so a note sung nearer the wrong note ends up on the wrong note, in tune.

Light correction is common in today's pop production, often so light that nobody notices it. Used hard, it becomes a sound of its own (Hard tune). It's no substitute for singing in tune: a weak take that's been corrected still sounds weak. The Singing techniques guide's Singing in tune shows how to practise.

**2. Cents: how far out is out** — *100 cents to a semitone, and what ears notice*

A semitone, one key on the piano, is split into 100 cents; an octave is 1,200. Cents measure a ratio, not a number of hertz, so a cent is the same small step at any pitch. A4 is 440 Hz. 30 cents below it is 432.4 Hz and 20 cents below it 434.9 Hz. At A2, 110 Hz, the same 20 cents is only 1.3 Hz. That's why tuners count in cents.

How much can you hear? Played one after the other, two steady tones a few cents apart, roughly 3 to 10 depending on the pitch, the loudness and the listener, can be told apart. In a song it's different: a sung note usually has to be about 20 to 25 cents off before many listeners hear it as out of tune. Trained musicians notice less, and people differ a lot. Short, quick notes are heard less exactly than long held ones.

DawIt Tune colours notes with that in mind: green within 10 cents of a note (in tune), yellow within 25 (close: some listeners will hear it on a long note), red beyond 25 (out). These bands are DawIt's choice, not a standard.

A sung note never sits on one exact number. Even a good singer's pitch moves by a few cents, slides into notes and swings with vibrato. What counts is where the note settles, its middle, and whether it stays there on a long note.

**3. Finding the pitch** — *How DawIt Tune hears which note you sing*

A sung vowel repeats itself: the vocal folds open and close, and each cycle of the wave looks almost like the one before. The time one cycle takes is the period, and the pitch is how many periods fit in a second. A3, 220 Hz, repeats every 4.5 milliseconds.

DawIt Tune finds the period by comparing the sound with itself a moment later, for every delay from 0.5 ms (2,000 Hz) to 20 ms (50 Hz). When the delay is exactly one period, the two line up and the match is nearly perfect. The picture shows the match for a sung A3: a peak at 4.5 ms, and more at 9.1 and 13.6 ms, two and three periods. This is McLeod's method (2005), the one DawIt also uses for the pitch in the wave editor. It looks at about 40 milliseconds at a time and gives a new answer about every 5 ms.

Sounds without a cycle have no pitch. The s of "sun", f, sh and t are noise, made by air rushing past the tongue and teeth: the match never peaks, so DawIt Tune lets them through untouched. The same goes for breaths and silence.

Three things make pitch hard to find:
- Octave mistakes. A voice with a weak lowest harmonic can look as if it repeats every two periods, an octave low, or every half period, an octave high. DawIt Tune compares each answer with the ones before it, and ignores a sudden octave jump that lasts only a moment.
- Chords and harmonies. Two notes at once don't make one repeating cycle, and DawIt Tune shows "no clear pitch". It follows one voice at a time.
- Reverb, delay and other sounds in the recording blur the cycle. Put DawIt Tune on the dry vocal, before reverb.

**4. Target notes** — *Key, chords or every semitone*

In Correct mode every note is moved towards a target: the nearest note DawIt Tune is allowed to use. Target chooses which notes those are.

Key: the notes of the song's key, set in the transport bar. In C major those are C, D, E, F, G, A and B. A note sung between two of them goes to the nearer one: an F# in C major is pulled down to F when it's sung a little flat, and up to G when it's a little sharp. This is the usual choice: a note that drifts can never be moved to a note outside the key.

Chords: only the notes of the chord in that bar on the chord track, such as A, C and E under Am. That's stricter: a passing note between chord notes is pulled onto one of them. Use it for long held notes over clear chords, not for a busy melody. Bars without a chord use the key.

Chromatic: every semitone. Nothing is moved by more than half a semitone, and notes from outside the key stay what they are. Use it when the song borrows notes from outside its key, or when you're not sure of the key.

Halfway between two targets is the point where a note changes target. A note that hovers right on it would flip between the two, so DawIt Tune keeps a note on its target until the pitch is 15 cents past the halfway point.

Mind the key's own notes. A minor's natural scale has G, but many songs in A minor sing G#, the raised seventh. With the key set to A minor, a sung G# is pulled to G or A. Set the key to A harmonic minor, or choose Chromatic, and it stays a G#.

**5. Speed, amount and tolerance** — *How fast and how far notes are moved*

Speed is how quickly a note is moved to its target, in milliseconds: the time it takes to get 95% of the way there. At 0 ms the move is instant. At 50 ms a note that starts low is still heard sliding up, then settles on the note. At 200 ms only long notes get there at all, and quick notes pass almost as they were sung. The picture shows the same scooped E corrected at 0, 50 and 200 ms.

Fast speeds sound more corrected. A real voice glides between notes and wavers a little; take that away and it sounds mechanical. Slow speeds sound natural but leave short notes as they were. With Antares Auto-Tune, the best-known tuner, many engineers start a natural fix somewhere between 10 and 50 ms and adjust by ear. Each tuner measures speed in its own way, DawIt Tune included, so the same number can sound different from one to the next.

Amount is how much of the error is removed. At 100% the note goes all the way to its target. At 60% a note 30 cents flat ends up 12 cents flat: closer, and still human.

Tolerance is a band around each target, in cents, where nothing is touched. With 10 cents, a note 8 cents flat is left exactly as it was, and a note 30 cents flat is corrected.

Start gentle, with Natural fix or Gentle polish, and listen with Hear the original on and off. If you can hear the correction working on a natural vocal, slow it down or lower the amount.

**6. Vibrato and the human touch** — *Moving the middle of the note, not the swing*

Vibrato is a regular swing in pitch around a held note, about 5 to 7 times a second; in classical singing it typically swings 50 to 75 cents above and below. The ear hears one note, at the middle of the swing. So a note with vibrato isn't out of tune: only its middle can be.

A fast correction that follows every moment of the pitch fights the vibrato and flattens it into a straight, buzzy tone. Keep vibrato stops that. With it on, DawIt Tune first smooths the pitch to find the note's middle, letting through only changes slower than about one and a half times a second. A 5.5 Hz vibrato keeps less than a tenth of its swing in that smoothed line. Then it moves the middle to the target, and the swing rides on top, unchanged. The picture shows a note 30 cents flat with a 40-cent vibrato: with Keep vibrato on, the middle moves to the note and the swing stays; off, at 0 ms, the swing is gone.

The same goes for the other things that make a voice sound alive: a scoop into a note, a fall at its end, a bend on an emotional word. They are style, not mistakes. A slower speed and a tolerance leave them in; Hard tune takes them out on purpose.

Finding the middle takes a moment, so with Keep vibrato on the correction of a new note starts a little later. DawIt Tune starts afresh at each new note to keep that short.

**7. Formants** — *Moving the pitch without moving the voice's colour*

A voice has two parts that a tuner must keep apart. The pitch is how fast the vocal folds buzz. The formants are the resonances of the throat and mouth: they make an "ah" sound like "ah", and your voice sound like you. An "ah" has its first two formants near 730 and 1,090 Hz, whatever note it's sung on.

The simplest way to raise a pitch is to play the sound faster, like a tape at a higher speed. That moves everything up together, formants too. Up a semitone, the "ah" formant at 730 Hz moves to 773 Hz; up an octave, to 1,460 Hz, and a voice turns into a chipmunk. The picture shows an octave, so the move is easy to see.

DawIt Tune keeps the formants where they are. It cuts the voice into small overlapping pieces, each about two cycles of the folds long, and lays them closer together to raise the pitch or further apart to lower it. Each piece keeps the shape of its cycle, and with it the colour of the vowel; only how often the pieces repeat changes. This is called PSOLA (pitch-synchronous overlap-add, Moulines and Charpentier, 1990). It works best on one voice and on small moves, which is what pitch correction makes: usually less than a semitone.

DawIt Tune has no formant control. Moving the formants on purpose, for a deeper voice or a cartoon one, is a job for a different effect.

**8. Hard tune** — *When the steps are the point*

Set Speed to 0 ms and Amount to 100% and the voice jumps from note to note with no glide in between. Vibrato vanishes, slides turn into steps, and the voice gets a synthetic, warbling edge where it changes note. It's the sound of Cher's Believe (1998), often called the first big hit to use it on purpose, and of a lot of pop, hip-hop and R&B since.

It came from a correction tool. Antares Auto-Tune, made by the engineer Andy Hildebrand, came out in 1997 to fix notes quietly. The producers of Believe turned its speed to zero and made the steps the hook.

For the sound: use Key rather than Chromatic, so the steps land on the key's notes. Sing clear notes and move between them decisively; the warble is strongest on slides between notes, so slide on purpose where you want it. Hard tune shows every wrong note plainly: a note sung nearer the wrong target jumps there completely.

It's a style. Some listeners love it and some can't stand it, and on a quiet ballad it can pull attention away from the words. Choose it; don't fall into it.

**9. Using it well** — *Check first, fix lightly, put it first*

Pitch correction works best as a light last step on a good take, not as a cure.

1. Check first. Put DawIt Tune on the vocal in Check mode and play the song. Your track lists where the notes are out. A few notes out is normal. A whole section flat often means tiredness, a key that doesn't suit your voice, or a headphone mix where you can't hear yourself well.
2. Fix what singing can fix. Another take, or a key that suits your voice (Singing techniques: Your voice and your song), beats correcting a strained one.
3. Put it first in the chain, on the dry vocal: before EQ, compression and above all reverb and delay, which blur the cycles DawIt Tune listens to. Antares gives the same advice for Auto-Tune.
4. Fix lightly. Try Natural fix or Gentle polish, then compare with Hear the original. Stop when a held note sounds right; you don't need every note at 0 cents.
5. Mind the wrong notes. Correction moves a note to the nearest target, not to the one you meant. A note sung nearer the wrong note needs another take, or Chromatic and a careful ear.
6. One voice at a time: put it on each vocal track, not on a bus of harmonies.

DawIt Tune has to hear a little ahead, so inside it the track is about 27 milliseconds late (29 at a 44.1 kHz sample rate). DawIt plays that track's clips the same amount earlier, so it stays in time with the rest. On a bus or a return it can't do that, and everything on it plays late: use DawIt Tune on a track. When you play the track live from a keyboard, you hear it that much later.

**10. Your track** — *How in tune this track is, from what it heard*

DawIt Tune listens while the song plays and keeps what it heard: each note, the nearest note to it, and how many cents it was off. This page sums that up per section. It only knows what has played through it, so play the song through once; playing a part again replaces what it heard there. What it heard isn't saved with the project. Notes are measured against the nearest semitone, so a note outside the key counts as a different note, not as out of tune.

(Then the advice paragraphs from the rules below, as `liveText`.)

**Singing guide, *Singing in tune*, last paragraph added:** DawIt Tune, a built-in effect, shows how many cents each sung note is off while the song plays, and can pull notes into tune. Its guide, What is pitch correction?, explains how.

### Controls: explainer text, typical values and tooltips
- **Mode.** Check only measures; you hear the track as it is. Correct also moves each note towards its target. Both make the track 27 ms late inside, which DawIt makes up for, so switching never moves the track in time. *Typical:* Check first, Correct only if the take needs it. *Tooltip:* "Check: only measure. Correct: also pull notes into tune."
- **Target.** Key: the notes of the song's key (C major: C, D, E, F, G, A, B). Chords: the notes of the chord in that bar; bars without a chord use the key. Chromatic: every semitone, nothing moves more than half a semitone. *Typical:* Key; Chromatic when the song borrows notes. *Tooltip:* "Which notes a sung note can be pulled to."
- **Speed.** How fast a note reaches its target: the time to get 95% of the way, from 0 ms (instant, the hard-tuned sound) to 400 ms (only long notes get there). *Typical:* about 50 ms for a natural fix, 100 to 150 ms for a gentle polish, 0 for hard tune. *Tooltip:* "Time to get 95% of the way to the target note. Double-click for 50 ms."
- **Amount.** How much of the error is removed. 100%: all of it; 60%: a note 30 cents flat ends 12 cents flat. *Typical:* 100% for a fix, 50 to 70% to keep it human. *Tooltip:* "How much of the error is removed."
- **Tolerance.** A band around each target, in cents, where nothing is touched: with 10, a note 8 cents flat is left as it was. *Typical:* 5 to 15 cents; 0 for hard tune. *Tooltip:* "Notes closer than this to their target are left alone."
- **Keep vibrato.** Moves the middle of a note, not its swing, so vibrato, scoops and bends stay. *Typical:* on, except for hard tune. *Tooltip:* "On: correct the middle of the note and keep its vibrato."
- **Hear the original.** The track without correction, at the same moment and level, to compare. *Tooltip:* "Hold the correction back to compare (not saved)."
- **Play an off note.** A sung "ah" on A3 that starts 150 cents low, slides up to 30 cents flat and holds there with a gentle vibrato, through the effect. *Tooltip:* "Play a slightly out-of-tune sung note through DawIt Tune."
- **Tuner.** The nearest note to what's sung now, how many cents sharp or flat, and whether it's in the song's key. Green within 10 cents, yellow within 25, red beyond. "–" means no clear pitch: silence, a consonant, a breath or several notes at once.
- **Trace.** The pitch of the last seconds on the piano-roll's rows: the key's notes shaded, the sung line coloured by how far out it is, the corrected line in blue. Hover for the note; click to move the playhead there; right-click to show more or fewer seconds.
- **Starting points and why:**
  - *Check only* (Check): "Changes nothing. Shows each note and how many cents it's out, so you learn how the take sits before fixing anything."
  - *Natural fix* (Correct, Key, 50 ms, 100%, 10 cents, Keep vibrato on): "Pulls notes into tune within about 50 ms, so slides and scoops still sound sung, leaves notes within 10 cents alone and keeps the vibrato. The usual light fix for a lead vocal."
  - *Gentle polish* (Correct, Key, 120 ms, 60%, 15 cents, Keep vibrato on): "Removes a little over half of what's out, slowly, so the singer's character stays. For a good take with a few loose notes."
  - *Hard tune* (Correct, Key, 0 ms, 100%, 0 cents, Keep vibrato off): "Jumps to the nearest note of the key at once, so slides become steps: the robotic sound of Cher's Believe and much pop and hip-hop since. A sound, not a fix."

### Example data
- **Example phrase** (A minor, 3 s, frames every 5 ms): E4 (64) 0.0 to 0.9 s: −150 cents rising linearly to −10 at 0.2 s, then −10. G4 (67) 1.0 to 1.9 s: −35, plus 20 · sin (2π · 5.5 · (t − 1.3)) from 1.3 s. A4 (69) 2.0 to 3.0 s: 0 until 2.4 s, then linear to −30 at 3.0 s. Unvoiced gaps 0.9 to 1.0 and 1.9 to 2.0 s. The corrected line comes from the `tune::` simulator with Natural fix.
- **Cents:** A4 440.00, −30 → 432.44, −20 → 434.95, +50 → 452.89 Hz; A2 110 Hz, −20 cents → 108.74 Hz (1.26 Hz lower).
- **Finding the pitch:** 220 Hz buzz, harmonics 1 to 16 under the "ah" envelope; period 4.545 ms; match peaks at 4.545, 9.091, 13.636 ms (from the tracker itself); white noise at the same RMS, no peak above 0.5.
- **Targets:** C major; F# +10 cents (66.10) → G (67); F# −10 cents (65.90) → F (65); midpoint 66.00, band 65.85 to 66.15.
- **Speed:** E4 scoop −150 → −10 over 0.2 s; corrected at 0 ms (on E from the first frame), 50 ms (95% after 50 ms), 200 ms.
- **Vibrato:** centre −30 cents, ±40 at 5.5 Hz, 2 s; Keep vibrato on at 50 ms (centre → 0, swing ±40 kept), off at 0 ms (flat at 0).
- **Formants:** 220 Hz, harmonics up to 4 kHz, envelope peaks 730 and 1,090 Hz; faster by an octave: 440 Hz harmonics, peaks 1,460 and 2,180 Hz; PSOLA: 440 Hz harmonics under the original envelope.
- **Hard tune:** G4 for 0.6 s with ±30 cents vibrato, a 150 ms slide to A4, A4 for 0.6 s; corrected at 0 ms in A minor.

### "Your track" rules
A **note** is a run of voiced frames whose vibrato centre keeps the same nearest semitone, with gaps under 50 ms bridged, at least 150 ms long; its **error** is the median cents to that semitone over its frames after the first 80 ms. **Phrases** are split by 0.5 s without a voiced frame. Bars are shown from 1. Rules in order (all thresholds are DawIt's choice):
1. Fewer than 8 notes: "Play the song with DawIt Tune on this track. It listens while the song plays, then shows how in tune each section is." Nothing else.
2. Always (8 or more): "Of 46 notes, 38 (83%) are within 25 cents of a note, and 29 (63%) within 10."
3. Each section with 5 or more notes whose mean error is 12 cents or more either way (up to three, largest first): "The verse sits 18 cents flat on average. A whole section flat often comes from tiredness, low energy or notes that sit awkwardly in your voice; try it with a little more energy, or check the key in Singing techniques: Your voice and your song." (sharp: "...sharp on average. Singing sharp often comes from pushing; try it a little lighter.") Quiet under 12.
4. At least 4 phrases of 2 or more notes whose last notes average 15 cents or more flatter than the other notes: "The last notes of your lines are 22 cents flatter than the rest, which often means running out of breath. Take a breath before each line rather than during it (Singing techniques: Posture and breathing)."
5. Notes of 250 ms or more outside the song key, per section and pitch name (up to three): "3 notes in the chorus are F#, outside C major (bars 17, 18 and 21): wrong notes, or on purpose? Listen to them against the chords." When the note is the raised 7th of a minor key, add: "G# is the raised seventh that harmonic minor uses, common in minor songs. If it's meant, set the key to A harmonic minor so DawIt Tune pulls to it."
6. The three notes with the largest error beyond 25 cents: "Furthest out: bar 9, A4, 38 cents flat; bar 12, C5, 31 cents sharp; ...". Quiet when none.
7. Correct mode and mean |correction| over voiced frames above 40 cents: "DawIt Tune is moving notes by 46 cents on average. That much correction tends to sound processed; another take may sound better."
8. Over 40% of frames louder than −40 dBFS have no clear pitch, with at least 2 s of them: "Much of this track has no clear pitch: is it a chord, a stack of harmonies or drums? DawIt Tune follows one note at a time."
9. 90% or more within 25 cents and none of 3 to 6 said: "Nicely in tune. Correction would change little; if you use it, Gentle polish is enough."

### Test numbers
| Check | Expected | Tolerance | Kind |
|---|---|---|---|
| cents of 432.44 Hz against A4 | −30.00 | 0.05 | exact |
| frequency of 69 − 0.30 | 432.44 Hz | 0.01 | exact |
| ratio for +100 cents | 1.059463 | 1e−6 | exact |
| Key C major: 66.10 → 67; 65.90 → 65; 66.00 with no previous → 65 | as listed | — | exact |
| hysteresis, previous 65: 66.10 → 65; 66.20 → 67 | as listed | — | exact |
| Chromatic: 66.40 → 66; 66.60 → 67 | as listed | — | exact |
| A minor: 68.00 → 67; A harmonic minor: 68.00 → 68; Am chord (A C E): 71.00 → 72 | as listed | — | exact |
| tolerance 10: error 8 → 0, error 30 → 30; amount 60%, error 30 → 18 | as listed | 0.001 | exact |
| glide, 60 ms, step 100 cents: at 20 ms / 60 ms | 63.2 / 95.0 | 0.1 | exact |
| glide at 0 ms: first sample | 100 | 0 | exact |
| centre filter gain at 5.5 Hz | 0.074 | 0.005 | exact |
| latency at 44.1 / 48 / 96 kHz | 1280 / 1280 / 2560 | 0 | exact |
| Check mode, bypass, Hear the original: output vs input delayed by L | max diff < 1e−6 | — | exact |
| silence in | max abs out < 1e−7 | — | exact |
| tracker: 55, 110, 440, 1760 Hz sines; 110 Hz saw; buzz with fundamental 20 dB under harmonic 2 | true pitch | 3 cents | estimate |
| tracker: white noise; equal sines 220 + 311.13 Hz | no clear pitch | — | estimate |
| 432.44 Hz sine and the "ah" buzz at A3 − 30 cents, Hard tune, C major, over 0.2 to 1.0 s | 0 cents | 5 cents | estimate |
| 440 Hz sine, Natural fix, C major | output = input delayed by L, max diff < 1e−6 | — | estimate |
| speed 0: output within 10 cents of target by | 30 ms after the note starts | — | estimate |
| centre −30, ±40 at 5.5 Hz, Natural fix: centre / depth | 0 / 40 cents | 5 / 10 cents | estimate |
| same, Keep vibrato off, 0 ms: depth | < 12 cents | — | estimate |
| PSOLA, 150 Hz "ah" buzz up 4 semitones: strongest harmonic in 500 to 1,500 Hz | within 10% of 730 Hz (resampling: ≈ 920 Hz) | — | estimate |
| noise burst between voiced notes: output in the burst (5 ms edges excluded) vs delayed input | below −60 dB of the burst | — | estimate |
| off-note test sound: peak out vs in; RMS out vs in | ≤ +1 dB; within 0.5 dB | — | estimate |
| 10 s at 48 kHz, Correct mode | processed in under 1 s | — | estimate |

Text checks: 10 pages, each with a name, a summary, text of 300 or more characters and a picture; no "color", "center", "analyze"; search finds the guide for "autotune", "auto-tune", "pitch correction", "formant" and "cents", first for "autotune"; four starting points with a "why"; the singing guide's *Singing in tune* names DawIt Tune.

## Steps
**Group A: pure DSP and theory (engine, generate).**
- [x] 1. Pitch tracker in `src/engine` (FFT McLeod, allocation-free, the octave guard) and `soundguide::estimatePitch` rebuilt on it. Tests: the tracker rows; estimatePitch's tests unchanged. Done when: they pass.
- [x] 2. `tune::` functions in `src/engine`: cents, targets with hysteresis, tolerance, amount, glide, vibrato centre, starting points, the made-up take generator, the guide simulator and example data. Tests: the exact rows. Done when: they pass.
- [x] 3. The PSOLA shifter in `src/engine` (mono mid, grains applied to both channels, dry path, crossfades). Tests: the PSOLA row, a ratio-1 run equals the delayed input. Done when: `./build.sh --test` passes.

**Group B: the processor and the engine.**
- [x] 4. `BuiltInTune` following the recipe (parameters, modes, latency, `processBlockBypassed`, *Hear the original*, live atomics, frame FIFO and take log, the off-note test sound added to `fx::Clap` beside the stereo tool's `wideBass`, which is being added in parallel), registered in `BuiltInFormat`. Tests: the sound rows, starting points, `checkThroughProject`.
- [x] 5. Song context to plugins (key and chord masks through `RenderContext`, opt-in interface, cached in `PluginSlot`). Tests: `setKey` (G major) reaches the processor's mask after one block; a bar with Am gives A C E while it plays; a bar with no chord falls back to the key.
- [x] 6. Latency compensation in `Track::process` (read-ahead, pre-roll after a jump, loop, offline) and `processBlockBypassed` with the delay in Saturation and Distortion. Tests: an impulse at beat 4 on two tracks, one with DawIt Tune, lands on the same sample (exact) when played, when started at beat 3.95 (25 ms before it at 120 BPM, inside the 26.7 ms at 48 kHz), on each pass of a loop from 4 to 8, bypassed, and in a bounce; Saturation's and Distortion's tests still pass. Done when: the group's `./build.sh --test` passes.
- [x] 7. Take analysis in `src/generate` with the "your track" rules. Tests: made-up logs giving rules 1, 3 ("verse sits 20 cents flat" from 8 notes at −20), 4, 5 (3 F# notes in the chorus in C major at bars 17, 18, 21; a G# in A minor adds the harmonic-minor sentence), 6, 9, and "83%" for 38 of 46.

**Group C: the UI.**
- [x] 8. Shared painters (tuner, trace, zone colour helper) and `TuneEditor` on the `fxui` frame with the controls, texts, tooltips, hover, click and right-click; `createEditorFunction` in `MainComponent`. Check: the window with the off note playing.
- [x] 9. The guide (10 pages, `liveText` for *Your track*, PDF), its `gen::guides` entry, `BuiltInGuides` branches, the `openGuide` case, the chip picture in `ChainPictures`, the singing guide's added paragraph. Tests: the text checks; `runTheoryGuidesTests`; `--export-guides` passes. Done when: `./build.sh --test` passes with no warnings.
- [x] 10. Real-app check: add a `tune` scene to `ScreenshotRunner` that adds an audio track "Sung take" with a clip from the take generator on the pop song's lead melody (verse 20 cents flat, 3 F# in the chorus, flat line ends), puts DawIt Tune on it (*Natural fix*), and renders the song once so the log fills. Shots: the window during the chorus (tuner, trace with red and yellow notes, corrected line); the window in Check mode; the device bar chip; guide pages 1, 3, 4, 7 and 10 (`"clap": true` where the picture moves); the singing guide's *Singing in tune*. Look for: cut-off text, the trace on the right rows, the advice naming the verse and the F# bars.
- [x] 11. Docs: README (Features bullet, Tests, Code layout for the tracker, `tune::`, `BuiltInTune`, the take analysis, `TuneEditor`, `TuneGuide`, the painters); FEATURE_IDEAS status *Partly done* (open: the later parts), correcting its example to F# and saying the hover pitch comes from `soundguide::estimatePitch`; the singing guide's idea mentions the link; `docs/architecture.md` (latency compensation and song context under *Built-in effects*), `docs/recipes.md` (a built-in with latency: constant latency, `processBlockBypassed`; opting into the song context), `docs/ui-design.md` (the zone colours and the trace and tuner painters).

## Tests
All checks are in *Theory: Test numbers* and steps 5 to 7, in `runTuneTests` (tracker, `tune::`, PSOLA, processor, engine) and the take analysis's own run function. Estimates may be adjusted once measured; dawImplement records the measured value and the new tolerance under *Changes while building*.

## Website hand-off
- Features page, *Effects*: a DawIt Tune section (Check and Correct, the tuner and trace, starting points, *Your track*), with screenshots of the window during the `tune` scene's chorus and the guide's *Target notes* and *Your track* pages.
- Main page: worth a line where singing is mentioned; the guide picker's *Singing* area gets *What is pitch correction?*.
- How it's made: no new library; a sentence that pitch is found with McLeod's method and moved with PSOLA, plain DSP.

## Left for later
- *Melody* target (notes of a chosen MIDI clip); *Check pitch...* on a clip with a summary window; *Practise singing* reusing the tracker and take analysis.
- A *Formants* control for creative shifts; pitch-shifting audio clips for *Change the key*.
- Dry monitoring while recording: DawIt records no audio yet; when it does, monitoring must bypass the read-ahead.
- Latency compensation on buses and returns (delaying the other tracks), and a song-wide view of every plugin's latency.
- Saving the take log with the project.

## Changes while building

- **Measured test numbers (the plan's estimates):** the tracker finds 55, 110, 440 and 1,760 Hz sines within 0.02 cents, a 110 Hz saw within 0.09, "ah" buzzes at 98, 220 and 392 Hz within 0.01, and a buzz with the fundamental 20 dB under harmonic 2 at the fundamental (0.00 cents; a check was added that prefers twice the delay when it matches at least 0.008 better). Hard tune on a 432.44 Hz sine is within 1.17 cents of A4 from 0.2 to 1.0 s (the "ah" at A3 minus 30 cents: 0.59); a note starting from silence is within 10 cents 30 ms after it starts (-0.51 cents by zero crossings); Natural fix on the vibrato example gives centre -2.5 cents and depth 42.6 (frames at 200 per second), Hard tune with Keep vibrato off depth 0.0; PSOLA up four semitones puts the strongest harmonic in 500 to 1,500 Hz at 756 Hz (resampling would give about 945); a noise burst comes out 221 dB below the burst (the dry path is exact); the off note keeps its level (peak 0.00 dB, RMS -0.02 dB) and peaks at -6.0 dB; ten seconds at 48 kHz take 0.06 s. The tolerances in the table stay as planned.
- **Clarity threshold 0.93, not 0.8:** two notes a tritone apart (220 and 311.13 Hz) scored 0.915 and would have been read as a pitch of 83 Hz; made-up sung vowels, with vibrato of 40 cents and slides of 700 cents a second, scored 0.955 and up. `pitch::Tracker::minClarity` is 0.93; `estimatePitch` keeps its old 0.75, so the wave editor's hover is unchanged.
- **Lowest note it moves is about 100 Hz** (G2): the shifter needs 2.9 periods of look-ahead inside the latency, so a period over latency / 2.9 (441 samples at 44.1 and 48 kHz, 882 at 96 kHz) is passed through as the held-back input. The tuner and the trace still measure such notes. This isn't in the plan.
- **Scoops start on the nearest note:** the plan's picture data say the corrected scoop is "on E from the first frame" at 0 ms. The nearest allowed note to the first frames (62.5, 150 cents low) is D, so at 0 ms the corrected line sits on D until the sung pitch passes the halfway point plus 15 cents, then jumps to E; with Keep vibrato on, the lagging centre does the same more gently. The pictures show what the simulator does; the texts needed no change.
- **Only audio tracks are compensated:** MIDI tracks with an instrument followed by a plugin with latency are not read earlier (`Track::getCompensation` is 0 for them). A bypassed plugin that doesn't implement `KeepsLatencyWhenBypassed` (third-party plugins) counts 0, as it is skipped as before. A change of the track's compensation while playing (a plugin added or bypassed) pre-rolls once. The interface lives in the new `SongContext.h` with `SongContextReceiver`.
- **`Engine::setChord`, `setChords` and `resetEdit` now take the engine's lock** around the chord map, because the audio thread reads it to pass the bar's chord to plugins.
- **Page texts and example notes live in `src/generate/TuneGuideText` and `tune::` (UI-free)** so the tests can read them; the window's `TuneGuide.cpp` draws the pictures. Two sections with the same name in "Your track" are told apart by their bars ("The verse (bars 5 to 12) sits ...").
- **A fact for dawTheory:** the page 2 text, as planned, says 20 cents below A4 is 434.9 Hz while fact 3 gives 434.95 Hz (which rounds to 435.0 Hz at one decimal); the "432.4 Hz" and "1.3 Hz" in the text are rounded the usual way.
- **Not in the plan:** `Clap::Sound::offNote` (A3 sung "ah", level scaled so it peaks at -6 dB), `BuiltInTune::clearRecentFrames` and the `tune` scene of `ScreenshotRunner`; DawIt Tune's own colour is violet `#a78bfa`; the chip picture is in `ChainPictures`, so the Signal Flow window shows it too; the clap button of the shared frame is 120 px wide for "Play an off note".
- **dawTheory, 434.9 Hz:** 440 · 2^(−20/1200) = 434.946 Hz, so "434.9" (page 2) and "434.95" (picture, fact 3, test) are both right; no change.
- **dawTheory, Auto-Tune numbers:** the 1997 release (Wikipedia gives 19 September 1997; Britannica, Mix agree) and "many engineers start between 10 and 50 ms" (editorial guides; Auto-Tune Access's own presets are 3, 25 and 65 ms) are kept as worded, as typical.
- **dawTheory, page 1:** the scoop sentence now says the start, nearer D, is held towards D at first and moves to E past the halfway point (what the simulator draws), not "the slide is kept".
- **dawTheory, page 2:** "Trained musicians notice less" became "notice smaller differences" (it said the opposite of the research).
- **dawTheory, page 3:** octave mistakes: the octave-high error is the one a weak lowest harmonic causes; reworded.
- **dawTheory, page 5:** one sentence explaining why the picture's corrected scoop sits towards D, then moves to E (a jump at 0 ms).
- **dawTheory, page 9:** only audio tracks are compensated (instrument tracks, buses and returns play late), and notes below about A2 (110 Hz; 108.8 Hz at 48 and 96 kHz, 100 Hz at 44.1 kHz) are measured but not moved; the live-keyboard sentence folded into the instrument-track case.
- **dawTheory, TuneEditor:** lead line "Hold Hear the original" became "Turn on" (it's a toggle); the Mode explainer says the latency is made up for "on an audio track".
- **dawTheory, open:** the window shows the bus/return lead line but nothing on an instrument track, where DawIt Tune also plays late (code change, not made).
