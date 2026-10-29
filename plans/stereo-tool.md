# A stereo tool effect: width, and mono below a frequency

Status: Built
Idea: [A stereo tool effect: width, and mono below a frequency](../FEATURE_IDEAS.html#a-stereo-tool-effect-width-and-mono-below-a-frequency)

## Goal
*DawIt Stereo*, a built-in effect that makes the low end mono below a frequency and sets the width above it, so a bass, a
pad or a whole mix keeps a solid centred low end on phones, club systems and vinyl while the highs stay wide. Its window
shows mid and side by frequency, a goniometer and a correlation meter, before and after, and *What is stereo width?*
explains stereo, mid and side, mono and why bass stays in the middle, with a *Your track* page that measures the track.

## Decisions for you

**Answered by the user, all as recommended:** the correlation meter and goniometer built now as shared parts for DawIt Meter; its own *What is stereo width?* guide, match level on by default, no pan/balance/swap, Linkwitz-Riley crossover; listen mode saved with the MONO/SIDE chip, *Your track* measured live, name DawIt Stereo in the Mixing area, the weakly sourced facts kept as typical practice.
1. **Correlation meter and goniometer now, or wait for DawIt Meter?** Options: (a) build them now as a shared engine
   measurement and a shared UI painter that DawIt Meter later reuses; (b) a private display in this window; (c) wait.
   **Recommended:** (a): the effect teaches nothing without seeing the low end close into a line, and building it shared
   now means one implementation of each (as the LUFS idea asks).
   Answer:
2. **Its own guide, or topics in the stereo image guide?** The stereo image guide isn't built. **Recommended:** its own
   *What is stereo width?* guide (like every built-in), covering what the effect needs (stereo, mid and side, the
   meters, mono, why bass stays narrow). Panning, pan laws and where instruments sit stay with the stereo image idea,
   which will link here.
   Answer:
3. **Match level on by default?** **Recommended:** on, as DawIt Saturation and Distortion: the level change of Width is
   a side effect and louder fools the ear. It never applies in the Mono and Side listen modes, which are checks.
   Answer:
4. **Pan, Balance and Swap left and right?** **Recommended:** leave out: the track's pan knob already places the sound,
   and two pan controls confuse a beginner. Noted under *Left for later*.
   Answer:
5. **Crossover kind.** Options: Linkwitz-Riley (no latency, shifts the low end's phase a little, the same in both
   sides); linear phase (no phase shift, but latency of tens of ms). **Recommended:** Linkwitz-Riley, as the idea says;
   the guide states the phase caveat.
   Answer:
6. **Is the Listen mode (Stereo, Mono, Side) saved?** **Recommended:** yes, as a parameter, so the *Mono check* starting
   point works and a project reopens as it was; the device bar chip shows MONO or SIDE so it isn't forgotten.
   Answer:
7. **"Your song" scope.** Options: the track the effect is on, measured live; or every track of the song, rendered
   offline. **Recommended:** the track, live, as the other built-ins do; the whole-song scan belongs to *Analyze the mix*.
   Answer:
8. **Name and place:** "DawIt Stereo", in the Theory Guides window under *Mixing*, not on *Start here*. **Recommended:**
   yes (the identifier is saved in projects; short).
   Answer:
9. **Facts from practitioners only** (no standard exists): club systems often run their subwoofers in mono; vinyl
   cutting engineers sum bass to mono below "about 100 to 300 Hz"; the correlation meter's third of a second is DawIt's
   choice. **Recommended:** keep them, worded as typical practice (as in *Theory*).
   Answer:

## What the user does and sees
- Added like any built-in effect (the plugin chain menu, listed after DawIt Gate) on a track or a bus; double-click its
  chip to open the window. On the whole mix: a bus that every track plays into.
- **Window** (1240 x 640, `fxui::EffectEditor`):
  ```
  [DawIt Stereo | Starting points v | What is stereo width? | Wide bass | Hear what it removes | ?]
  [ Mid and side by frequency (draggable line)   | In: goniometer  | Out: goniometer | ] [ explainer ]
  [                                              | Low [----|--] All [--|----] (each)| ]
  [ last 4 seconds of level                                                          ]
  [ MONO BELOW: Mono below, Slope | WIDTH: Width | CHECK: Listen | LEVEL: Output, Match level ]
  ```
- **Mid and side by frequency:** 20 Hz-20 kHz, -90 to 0 dB; the output's mid filled grey, the side in the effect colour
  (thin line: before, filled: after); the part below Mono below shaded amber with "mono below 120 Hz", the side filter's
  slope drawn from the line. Drag the line sideways (Alt: fine); double-click: 120 Hz; scroll: Width. Right-click:
  *Off, 80, 100, 120, 150, 200, 250 Hz*, *Reset all*, then *What is stereo width?* (at *Mono below: the crossover*).
  Tooltip under the mouse: "180 Hz · mid -24 dB · side -41 dB (after)".
- **Goniometers** In and Out (about 150 px square, `fxui::drawPanel`): dots of the last ~50 ms, the band below Mono below
  in amber, above in pink; L and R axis labels; under each, two **correlation bars** (Low and All, -1 to +1, red below 0,
  "no reading" in silence). Right-click: *What is stereo width?* at *Seeing stereo*.
- **Controls:** *Mono below* Off (bottom) then 20-500 Hz log, default 120; *Slope* switch 12 / 24 / 48 dB per octave,
  default 24; *Width* 0-200%, default 100; *Listen* switch Stereo / Mono / Side; *Output* -12 to +12 dB; *Match level*.
  Explainer texts and tooltips in *Theory*.
- **Test sound** button *Wide bass* (a new stereo test sound, in *Theory*); *Hear what it removes* plays input minus output.
- **Starting points** (six, in *Theory*), each with why.
- **Device bar chip:** a 50 px goniometer of the output (same painter), MONO or SIDE across it when Listen isn't Stereo,
  and the match-level badge like DawIt Gate's.
- Nothing new in `engine.properties`.

## Data and saving
Only the plugin's parameters, saved through its state like DawIt Gate; `ProjectFile.cpp` is unchanged, no version change.
Older projects have no DawIt Stereo; a state missing a parameter loads its default. Test: `checkThroughProject` changes
Mono below, Slope, Width, Listen, Output and Match, saves, loads and compares.

## The guide
*What is stereo width?*, an `fxui::Guide` made with the processor (as *What is a gate?*); pictures follow the window's
settings. Pages (text in *Theory*):

| # | Name | Summary | Picture |
|---|---|---|---|
| 1 | Left, right and in between | What stereo is | three sounds (vocal centred, guitar left, wide pad) as L and R waves and a dot on a speaker line |
| 2 | Mid and side | What both speakers share, and what differs | the example wave's L and R on top, M and S below; the four number pairs |
| 3 | Seeing stereo | The goniometer and the correlation meter | four goniometers with their correlation: mono, left only, wide, upside down |
| 4 | Mono: phones, clubs and the sum | What's left when the two become one | bars of five parts' level change in mono |
| 5 | Why the low end stays in the middle | Bass and kick: solid on every system | wide pad's mid and side by frequency, before and after mono below 150 Hz |
| 6 | Mono below: the crossover | Splitting the sound at a frequency | the low and high band and their flat sum for the window's slope and frequency; 12/24/48 in grey |
| 7 | Width: narrower and wider | Turning the side down or up | level and correlation against width 0-200% for the pad and the noise, window's width marked |
| 8 | Where to use it | Mix bus, bass, pads, vinyl | the six starting points as rows: mono-below span in amber, width bar |
| 9 | Too much | When narrowing or widening goes wrong | the pad's spectrum with mono below 400 Hz |
| 10 | How DawIt Stereo works | From crossover to output | the chain as boxes, the current settings in them |
| 11 | Your track | How wide your track is, down low and overall | the track's In and Out goniometers and correlation bars, the measured numbers and the advice lines |

- Opens from the window's guide button, the right-click menus of the spectrum and the scopes, and the Theory Guides window.
- **`gen::guides` entry** after `gate`: area *Mixing*; name "What is stereo width?"; summary "Mid and side, the
  goniometer and correlation meter, mono and phase, why the low end stays in the middle, mono below a frequency, width,
  and your track."; opens from "DawIt Stereo's window (What is stereo width?)"; plugin "DawIt Stereo"; keywords "stereo
  width wide narrow mono mono-compatible mono-compatibility mono-maker bass-mono mid side mid-side M/S correlation phase
  goniometer vectorscope Lissajous imager imaging widener spread low-end sub bass kick vinyl club crossover
  Linkwitz-Riley"; not on *Start here*. Mixing's area summary becomes "Levels, EQ, compression, gates, stereo width and
  buses: making the parts fit".
- **Link from** *What is sound?* > *Phase cancellation* (one sentence, in *Theory*).

## Code
Follows recipes.md *A built-in effect or instrument*, copying DawIt Gate (match level, an any-reader ring, the host
track for *Your track*, the device bar picture).
- **Stereo measurement** (new, `src/engine`, no UI, reused by DawIt Meter later): from left and right (and optionally a
  low and high split) it keeps correlation sums with a 300 ms time constant, the mid and side power per band, a ring of
  goniometer points and a 10 s history of 50 ms blocks. Every ring is atomics with an atomic write position, so the
  window, the guide, the chip and the analysis read it without taking from each other (unlike the stage peaks). No
  allocation on the audio thread.
- **Pure functions** (`daw::stereo`): mid and side, correlation from sums (none when either side is silent, 0 when one
  side is under -60 dB of the other), side-to-mid in dB, the Linkwitz-Riley band magnitudes for the drawings, the width
  formulas of the example pictures, the starting points and `analyse` (the *Your track* rules).
- **`BuiltInStereo`**: per channel a Linkwitz-Riley split (LR2: one band inverted; LR4; LR8 = two 4th-order Butterworth
  in series); low band set to its mid; high band's side times Width; added; then Listen, match level, output.
  Invariants: Mono below Off + Width 100% + Stereo + Output 0 dB + Match off is the input sample for sample (no
  filter in the path); Width alone never changes left plus right (Match off); a one-channel buffer passes untouched;
  no latency, no tail. Changing Slope or crossing Off crossfades over 20 ms; Width and the cutoff are smoothed.
- **Test sound:** `fx::Clap` gets a stereo sound; the others stay identical in every channel.
- **Shared UI painter** for the goniometer and correlation bar, plus one colour helper each for the effect (pink,
  about `#ff7fc8`) and the low band (amber, about `#ffc857`), used by the window, the guide, the chip and later DawIt Meter.
- **Threading:** parameters are APVTS atomics; filter coefficients change on the audio thread per block; the UI reads
  the rings on its timer. Nothing new under `Engine::getLock()`.
- **Risky:** the `fx::Clap` change touches every effect's test sound; the crossover's phase must be identical in both
  channels (or the image moves); clicks on slope changes.

## Libraries and models
None (JUCE IIR filters).

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| F1 | Mid = (L + R) / 2, side = (L - R) / 2; L = M + S, R = M - S. Others scale by 1/1.41 or not at all; ratios are the same | definition; DawIt Reverb's width uses it | exact |
| F2 | Mono plays L + R = 2M: the side cancels completely | derived | exact |
| F3 | Correlation r = sum(LR) / sqrt(sum(L^2) sum(R^2)): +1 when L and R have the same shape (also a panned mono sound in both sides), 0 unrelated, -1 opposite | definition; Voxengo Correlometer guide, SOS glossary | exact |
| F4 | For L and R equally loud: r = (M^2 - S^2) / (M^2 + S^2) (sums) | derived: sum(LR) = M^2 - S^2, sum(L^2) = sum(R^2) = M^2 + S^2 | exact |
| F5 | One side silent: r undefined, shown as 0; it doesn't cancel in mono | derived; meters differ | exact (DawIt's choice of display) |
| F6 | Readings from 0 to +1 are generally taken as mono-safe; below 0, the mono sum loses noticeably | SOS glossary; Voxengo | typical |
| F7 | Correlation meters differ in integration time; there is no standard. DawIt uses a 300 ms time constant | gearspace thread; x42 meter docs | opinion (DawIt's choice) |
| F8 | Goniometer: M up, S across; mono vertical; left only leans 45 degrees left; L = -R horizontal; wide = round cloud | convention (x = (R - L)/1.41, y = (L + R)/1.41) | exact |
| F9 | Mono level change against a centred sound, for equal-level sides: 10 log10((1 + r) / 2) dB: r 0.2 -2.2, 0 -3.0, -0.5 -6.0, -0.95 -16.0 | derived from F2, F4 | exact |
| F10 | Below about 100 Hz sound is hard to localise in a room; why one subwoofer works | Wikipedia *Subwoofer*; practitioner forums | typical |
| F11 | Many club and PA systems run their subwoofers in mono | Fader Pro blog; KVR forum (no standard) | typical (unconfirmed by a primary source) |
| F12 | A stereo groove moves sideways for L + R and up and down for L - R; much low side makes the groove deep and shallow and the needle can jump | Wikipedia *Phonograph record*; lathe forums; HoRNet ElliptiQ | typical |
| F13 | Vinyl cutting engineers sum bass to mono below a frequency; figures from about 100 to 300 Hz, 150 Hz common | recording.org, gearspace, emastered | opinion |
| F14 | Mono below 100-150 Hz on a mix or bus, 150-250 Hz on a bass | practice; matches *What is sound?* "below about 150 Hz" | typical |
| F15 | Linkwitz-Riley = two Butterworth filters in series; each band -6.02 dB at the crossover; LR4 and LR8 bands add to a flat magnitude; LR2 only with one band inverted | linkwitzlab.com *crossovers*; Rane Note 160; Wikipedia; derived for LR8 (n even: 1 + w^2n = abs(B)^2) | exact |
| F16 | Linkwitz and Riley described it in 1976 | Wikipedia *Linkwitz-Riley filter* (JAES 1976) | exact |
| F17 | High band one octave below the crossover: LR2 -13.98, LR4 -24.61, LR8 -48.20 dB; two octaves: -24.61, -48.20, -96.33 dB | derived: abs(H)^2 of Butterworth n | exact |
| F18 | Width scales the side: 0% mono, 200% side +6.02 dB; L + R unchanged | derived | exact |
| F19 | Level change with width w for equal unrelated sides: 10 log10((1 + w^2) / 2): 0% -3.01, 50% -2.04, 150% +2.11, 200% +3.98 dB; pad r 0.6 (M^2 = 4 S^2): 150% +0.97, 200% +2.04 dB | derived | exact |
| F20 | Correlation after width: (M^2 - w^2 S^2) / (M^2 + w^2 S^2): noise 50% 0.60, 150% -0.38, 200% -0.60; pad 0.6: 150% 0.28, 200% 0.00 | derived | exact |
| F21 | A Haas or delay widener changes the mono sum (comb filtering); mid/side width doesn't | derived; *What is sound?* cancellation | exact |
| F22 | A crossover of this kind shifts the low end's phase, the same in both sides; magnitude stays flat | derived (allpass sum) | exact |
| F23 | Plugins come before a track's fader and pan; DawIt's pan turns the far side down | `Track.cpp` | exact |
| F24 | Wide low end comes from detuned or chorused stereo synths, stereo pianos and pads, reverbs without low cut, two microphones, wideners | practice | typical |
| F25 | Width over about 150% sounds hollow or phasey; 110-140% for pads, backing vocals, returns | practice | opinion |
| F26 | Advice thresholds: side within 12 dB of mid (r under about 0.88) = wide low end; side 40 dB under = mono; low band 30 dB under the whole = little low end | DawIt's choice | opinion |
| F27 | Many phones, Bluetooth and smart speakers play mono | common knowledge; already in *What is sound?* | typical |

### Topic texts
Paste as they are (pages of *What is stereo width?*).

**1. Left, right and in between** — *What stereo is*

Stereo is two signals at once: one for the left speaker, one for the right. Your ears compare them and place each sound somewhere between the speakers. That picture is called the stereo image.

- The same sound, equally loud in both speakers, seems to come from the middle, where there is no speaker. Engineers call it the phantom centre. A lead vocal, a kick and a bass usually sit there.
- The same sound louder in one speaker moves towards it. That's what a pan knob does. In DawIt, turning the pan to the left leaves the left side as it is and turns the right side down; all the way left, the right side is silent.
- Different sounds in each speaker, like two microphones on a piano, a stereo reverb or a synth pad with chorus, seem to fill the space between the speakers. That's what we call wide.

A stereo track can hold all three at once. DawIt Stereo works on the third kind: how much of a sound differs between left and right, and at which frequencies. It doesn't move sounds left or right; the pan knob does that.

The picture shows three sounds: a vocal in the middle, a guitar panned left and a wide pad, each as its left and right wave and where you'd hear it.

**2. Mid and side** — *What both speakers share, and what differs*

Any stereo sound can be split another way than into left and right: into what both speakers share, the mid, and what differs between them, the side.

- Mid = (left + right) / 2
- Side = (left - right) / 2

And back again: left = mid + side, right = mid - side. Nothing is lost. It's the same sound, described differently.

Some numbers, for one moment of a sound:
- left 0.8, right 0.4: mid 0.6, side 0.2;
- the same in both, 0.5 and 0.5: mid 0.5, side 0. A mono sound has no side;
- on the left only, 0.6 and 0: mid 0.3, side 0.3, equally big;
- the right upside down, 0.5 and -0.5: mid 0, side 0.5. All side.

So the side holds the width. Turn the side down and the sound gets narrower; at zero, left and right are the same: mono. Turn it up and it gets wider. That's all a width control does, and it's how DawIt Stereo works inside.

Some tools define mid and side without the halves, or divide by 1.41 instead. The proportions are the same; only the numbers' size changes.

The picture shows a short stretch of a wide sound: its left and right on top, its mid and side below.

**3. Seeing stereo** — *The goniometer and the correlation meter*

Two pictures show stereo at a glance. DawIt Stereo's window has both, for the sound going in and coming out.

The goniometer (also called a vectorscope or phase scope) draws every moment of the sound as a dot. Up and down is the mid, left and right is the side. So:
- a mono sound draws a vertical line;
- a sound on the left only draws a line leaning 45 degrees to the left; on the right only, 45 degrees to the right;
- a wide sound draws a cloud: the wider, the rounder;
- a sound whose left and right are upside down to each other draws a flat line. That's the one to watch: in mono it vanishes.

In DawIt Stereo, the dots below the Mono below frequency are amber and the rest pink, so you see the low end close into a vertical line while the highs keep their cloud.

The correlation meter says much the same in one number, from -1 to +1:
- +1: left and right move together. A mono sound, also when it's panned to one side but still in both speakers.
- 0: left and right have nothing to do with each other, like two microphones far apart, or a very wide reverb.
- -1: left and right are opposite. In mono, everything cancels.

Readings between 0 and +1 are generally safe in mono. The further below 0, the more disappears when left and right are added together. Short dips below 0 in a wide reverb are usually fine; staying there is a sign of a problem.

Meters differ in how quickly they follow the sound; there is no standard. DawIt's follows over about a third of a second, and shows two bars: Low, below the Mono below frequency, and All, the whole sound.

Two caveats. A sound on one side only reads 0, because there's nothing on the other side to compare with; it doesn't cancel in mono. And silence has no reading at all.

The picture shows four sounds as a goniometer, with their correlation: mono, left only, wide, and upside down.

**4. Mono: phones, clubs and the sum** — *What's left when the two become one*

A lot of music is heard in mono: from a phone's speaker, many Bluetooth and smart speakers, ceiling speakers in shops, and the subwoofers of many club systems, which get left and right added together. Even with two speakers, someone standing near one of them hears mostly that one.

Mono adds left and right together. With mid and side: (mid + side) + (mid - side) = 2 x mid. The side drops out completely. So in mono:
- what's in the middle stays;
- a sound panned to one side stays, but gets quieter against the middle;
- what's wide loses its side, and gets quieter and duller: reverbs, wide pads and doubled guitars shrink back;
- what's upside down between the speakers cancels and disappears.

For a sound equally loud in both speakers, the correlation tells you how much it drops in mono against a sound in the middle: about 2 dB at 0.2, 3 dB at 0, 6 dB at -0.5 and 16 dB at -0.95.

That's why engineers check their mix in mono now and then. Not to make it sound good in mono, but to make sure nothing important vanishes: the vocal, the kick and the bass should be as clear in mono as in stereo.

Try it: put DawIt Stereo on a bus that every track plays into, choose the Mono check starting point and play the song. The picture shows how much five kinds of part drop in mono.

**5. Why the low end stays in the middle** — *Bass and kick: solid on every system*

In almost any pop, rock, hip-hop or dance record, the kick and the bass come from the middle, and everything below about 100 to 150 Hz is close to mono. There are good reasons.

- You can't hear where deep bass comes from. In a room, sounds below about 100 Hz are hard to place; that's why a home cinema gets away with one subwoofer anywhere in the room. Width down there adds little you hear as width.
- Mono systems lose it. In mono the side drops out, and many club systems play their subwoofers in mono. A bass that's partly side sounds full in your studio and thin in the club. With the low end mono, both are the same.
- Vinyl. A record's groove moves side to side for what both channels share and up and down for what differs. A wide or out-of-phase bass makes the groove deep and shallow in turns, and the needle can jump out. Engineers who cut records make the bass mono below a frequency; the figures they give range from about 100 to 300 Hz.
- Both speakers share the work. The low end holds most of a mix's energy. In the middle it plays equally from both speakers, so neither runs out of room first.
- A solid centre. A kick and a bass in one point sound tighter than the same notes smeared across the stereo image, and they leave the sides free for guitars, keys and reverbs.

Where does a wide low end come from? Stereo synth sounds with detuned voices or chorus, stereo pianos and pads, stereo reverbs without a low cut, a bass recorded with two microphones, and stereo wideners.

This is a habit, not a law. In a classical or acoustic recording made with two microphones in a hall, the room's low end is naturally a little wide, and many engineers leave it. For a song built from tracks, a mono low end is the safe choice.

Mono below doesn't make the mono version any louder: in mono the low side was already gone. It makes the stereo version match the mono one, so what you hear while mixing is what every system plays.

The picture shows a wide pad's mid and side by frequency, before and after Mono below 150 Hz: below the line, the side drops away.

**6. Mono below: the crossover** — *Splitting the sound at a frequency*

Mono below makes everything under a frequency mono and leaves the rest alone. To do that, DawIt Stereo first splits the sound in two with a crossover: a low band below the frequency and a high band above it. The low band's left and right are made the same (its side is removed). The high band keeps its width, set by Width. Then the two are added back together.

No filter is a brick wall. The Slope says how quickly the change happens:
- 12 dB per octave: gentle. An octave below the frequency, the side is only 14 dB down; it fades out over a few octaves.
- 24 dB per octave: the usual choice. An octave below, the side is 25 dB down; two octaves below, 48 dB.
- 48 dB per octave: sharp. An octave below, 48 dB down. For when the mono part should stop close to the frequency.

At the frequency itself, each band is at half its level (-6 dB), so the side there is half as big as it was.

The crossover is a Linkwitz-Riley design, named after the two engineers who described it in 1976 for loudspeakers. Its two bands add back to a flat response: a sound that's already mono comes out at the same level at every frequency. Like every crossover of its kind, it shifts the timing (the phase) of the low end a little, the same in both speakers. You won't hear that on its own. It matters only when the same sound also plays on another track without the effect, like two microphones on one bass: then put DawIt Stereo on the bus they share. At Off, the crossover is out of the way and the sound passes untouched.

Typical frequencies: 100 to 150 Hz on a mix or a bus, 150 to 250 Hz on a bass whose body should be mono. Above about 300 Hz you start narrowing things whose width you can hear: guitars, the left hand of a stereo piano, the body of a pad.

The picture shows the two bands for the current slope and frequency, and their sum: the straight line on top.

**7. Width: narrower and wider** — *Turning the side down or up*

Width turns the side up or down above the Mono below frequency (everywhere, when Mono below is Off). The mid is left as it is.
- 0%: no side, so left and right are the same: mono.
- 100%: as it was.
- 200%: the side twice as big (+6 dB): much wider.

Three things change with it.
- Loudness. More side adds level. For a sound whose left and right are unrelated, 150% makes it about 2 dB louder and 200% about 4 dB; 0% makes it 3 dB quieter. Louder sounds better at first hearing, so Match level, on from the start, keeps the level the same and you judge only the width.
- Correlation. A pad that reads 0.6 drops to about 0.3 at 150% and to 0 at 200%. Unrelated left and right (0) go to -0.4 at 150%.
- Stereo against mono. Left plus right stays exactly the same whatever the Width, because the side cancels whatever its size (with Match level off; with it on, the whole sound is turned down a little to keep the loudness). But the stereo version now has more side than the mono one, so a part you've widened is louder in stereo than in mono. The wider you go, the more it shrinks back on a phone.

Width only works with what already differs between left and right. A mono sound has no side, so Width does nothing to it. To make a mono sound wide, give each side something different: a stereo reverb, a short delay on one side (What is delay?), or a second take of the part panned the other way. Those change the mono sound too (a short delay added to the sound gives comb filtering), which this Width never does.

Typical settings: 80 to 100% to calm a busy stereo sound, 110 to 140% to open up pads, backing vocals or a reverb return. Engineers rarely go past 150%.

The picture shows how the level and the correlation of two sounds change as Width goes from 0 to 200%: a pad (correlation 0.6) and unrelated noise (0).

**8. Where to use it** — *Mix bus, bass, pads, vinyl*

- The whole mix, on a bus that every track plays into: Mono below 120 Hz at 24 dB per octave, Width 100%. It tidies whatever wide low end slipped in from pads, reverbs and stereo samples.
- A synth bass: Mono below 150 to 250 Hz, so the notes stay centred while the bright, chorused top keeps its width.
- A kick, a bass or a single voice that came as a stereo file: Width 0% makes it fully mono, one solid point that you then place with the pan knob.
- Pads, keys and stereo reverbs: Mono below 150 to 200 Hz, and Width 110 to 150% if they should spread out behind the vocal.
- A mix for vinyl: Mono below about 150 Hz. Ask the cutting engineer, who decides in the end.
- Checking: the Mono check starting point plays everything in mono, as a phone would. Switch Listen back to Stereo before you export.

Where it isn't needed: on a track that's already mono (nothing changes; Your track says so), on something you want wide down low on purpose, like an ambient pad or a recording of a room, and as a cure for a sound that cancels in mono because two microphones are out of step. Mono below hides that in the low end; fixing the polarity or the timing (What is sound? > Phase cancellation) fixes it everywhere.

The picture shows the starting points side by side: where their Mono below sits and how wide they leave the rest.

**9. Too much** — *When narrowing or widening goes wrong*

- Mono below set too high. Above about 300 Hz you hear width clearly, so a stereo piano, guitars or a pad lose their spread and the mix gets narrow. Turn it down until you hear the width come back, then a little lower.
- Too much width. Past about 150%, sounds get hollow and phasey and seem to come from beyond the speakers or from inside your head, and the correlation sits below 0. In mono they drop back much more than the rest.
- Widening the low end. Width over 100% with Mono below Off widens the bass too, usually the opposite of what you want. Keep Mono below on when you widen.
- Narrowing what's already mono: nothing happens.
- Mixing for mono. Make the stereo mix good, and use mono to check that nothing disappears.
- Judging by loudness. A wider sound is louder; with Match level off, compare at the same level.

The picture shows a wide pad with Mono below at 400 Hz: much more than the bass has lost its width.

**10. How DawIt Stereo works** — *From crossover to output*

The sound goes through these steps:
1. Crossover: a Linkwitz-Riley filter splits left and right at the Mono below frequency into a low and a high band (skipped when Mono below is Off).
2. Low band: left and right both become their average, the mid. The low end is now mono.
3. High band: split into mid and side; the side is multiplied by Width; then back to left and right.
4. The two bands are added together.
5. Listen: Stereo plays the result. Mono plays its mid in both speakers, as a mono system would. Side plays only what differs, in both speakers, so you hear what the width is made of.
6. Match level compares the loudness going in and coming out over about half a second and evens it out (not in Mono and Side, which are for checking).
7. Output: a last level, -12 to +12 dB.

DawIt Stereo sits in the chain before the track's fader and pan, so on a track it hears the sound before the pan knob places it: a mono track panned to one side is still mono here. On a bus it hears the tracks after their pans. It adds no delay, so the track stays in time with the others. On a track with only one channel it does nothing.

The meters measure before and after: the goniometers, the correlation of the whole sound and of the part below the Mono below frequency (below 120 Hz when it's Off), and the mid and side by frequency.

**11. Your track** — *How wide your track is, down low and overall*

Play the song where this track plays. DawIt Stereo measures what comes in over the last 10 seconds of sound (quiet moments don't count) and says what it sees: how wide the low end is, whether anything cancels in mono, and what your settings do to it.

It measures how far the side is under the mid, below the Mono below frequency (below 120 Hz when it's Off) and for the whole sound, and the correlation of both, going in and coming out. The picture shows them, with the goniometers.

Opened from the Theory Guides window without a track, this page uses a stand-in: put DawIt Stereo on a track and open the guide from its window.

### Explainer texts and tooltips
Explainer (hover) text, then the tooltip in brackets.
- **Mono below:** Everything under this frequency is made the same in both speakers: mono. Above it, the sound keeps its width. Typical: 100 to 150 Hz on a mix or a bus, 150 to 250 Hz on a bass. Off, all the way down, leaves the whole sound as it is. (Mono below: everything under this frequency is made the same in both speakers. 100 to 150 Hz keeps the kick and bass solid on small and club systems.)
- **Slope:** How quickly the sound changes from mono to stereo around the Mono below frequency. 12 dB per octave is gentle, 24 the usual choice, 48 sharp. An octave below the frequency the side is 14, 25 or 48 dB down. (Slope: how sharply mono turns into stereo at the frequency.)
- **Width:** How wide the sound is above the Mono below frequency: 0% is mono, 100% as it was, 200% the side twice as big. Turning it up makes the sound louder too; Match level evens that out. Typical: 100%, or 110 to 140% for pads and reverbs. (Width above the Mono below frequency: 0% mono, 100% as it was, 200% wider.)
- **Listen:** Stereo: the result. Mono: left and right added, in both speakers, as a phone or a club's subwoofers play it. Side: only what differs between left and right, to hear what the width is made of. Mono and Side are checks and are saved with the project: switch back to Stereo before you export. (Listen: Stereo, Mono to check as a phone hears it, or Side to hear the width alone.)
- **Output:** The level after everything else, from -12 to +12 dB. (Output level.)
- **Match level:** Keeps the sound as loud coming out as going in, so you hear the width and not a change of loudness. Not applied in Mono and Side. (Match level: same loudness in and out, for a fair comparison.)
- **Mid and side picture:** Mid and side by frequency. Grey: the mid, what both speakers share. Pink: the side, what differs; the thin line is before, the filled shape after. The amber part is made mono. Drag the line to set Mono below; double-click it for 120 Hz. (Drag to set Mono below; Alt for fine steps; double-click for 120 Hz; scroll for Width.)
- **Goniometers:** Each dot is a moment of the sound: up is the mid, sideways the side. A vertical line is mono; leaning left or right, panned; a cloud, wide; lying flat, left and right cancel in mono. Amber dots are below the Mono below frequency. (In: the sound going in. Out: coming out. Vertical is mono, a cloud is wide.)
- **Correlation bars:** From -1 to +1, over about a third of a second. Low: below the Mono below frequency. All: the whole sound. +1 is mono, 0 unrelated, below 0 parts cancel in mono. (Correlation: +1 mono, 0 unrelated, below 0 cancels in mono.)
- **Wide bass button:** (Plays a test sound: a low A at 55 Hz and a shaker, both wide. Turn Mono below up and hear the bass move to the middle while the shaker stays wide.)
- **Guide hint** (`fxui::Guide`): "Play Wide bass to hear the pictures' sound through your settings."

### Starting points (name, settings, why)
1. **Mix bus: tighten the low end** (120 Hz, 24, 100%, Stereo, match on): Whatever wide low end slipped in from pads, reverbs and stereo samples is made mono, and nothing above 120 Hz changes. The usual first step on a bus that holds the whole mix.
2. **Synth bass: mono body, wide top** (200 Hz, 24, 100%): The notes and the body stay in the middle, solid on every system; the bright, chorused top keeps its width.
3. **Fully mono** (120 Hz, 24, Width 0%): For a kick, a bass or a single voice that came as a stereo file: one solid point, which the track's pan knob then places.
4. **Vinyl-safe** (150 Hz, 24, 100%): A wide or out-of-phase bass makes a record's groove deep and shallow, and the needle can jump. Cutting engineers usually make the bass mono somewhere between 100 and 300 Hz; 150 Hz is a common middle. Ask yours.
5. **Wide pads** (200 Hz, 24, 150%, match on): The pad spreads out behind the vocal while its low end stays centred. Match level keeps the wider pad from simply sounding louder.
6. **Mono check** (Off, 100%, Listen Mono, match off): Hear the whole sound as a phone plays it. Match level is off so you hear how much quieter the wide parts get. Switch Listen back to Stereo before you export.

### Test sound "Wide bass" (1.5 s, peak about -6 dB)
Left: 0.35 x env x sin(2 pi 55 t); right: the same 90 degrees later (r = 0 in the low band). env: 5 ms rise, then
exp(-t / 0.6). Plus six shaker hits at 0, 0.25 ... 1.25 s: independent white noise per side, high-passed at about
2 kHz, peak 0.2, decaying with exp(-t / 0.03). After Mono below 120 Hz (24): the 55 Hz side is 27.5 dB down, low
correlation about 0.996.

### Example data
- Page 1: vocal L = R = 0.5 sin; guitar L = 0.6 sin, R = 0; pad L = 0.5 sin, R = 0.5 sin 90 degrees later.
- Page 2: L = 0.7 sin(2 pi f t), R = 0.7 sin(2 pi f t - 60 degrees), two cycles (r = cos 60 = 0.5); pairs (0.8, 0.4) -> (0.6, 0.2); (0.5, 0.5) -> (0.5, 0); (0.6, 0) -> (0.3, 0.3); (0.5, -0.5) -> (0, 0.5).
- Page 3: mono (L = R, r 1.00); left only (R = 0, reads 0); wide (seeded independent noise, r about 0); upside down (R = -L, r -1.00).
- Page 4 (10 log10((1 + r) / 2)): Lead vocal r 1.0: 0.0 dB; Wide pad 0.2: -2.2; Stereo reverb 0.0: -3.0; Widened synth -0.5: -6.0; Upside-down bass -0.95: -16.0.
- Pages 5 and 9: pad mid at -20 dB at 1 kHz falling 3 dB per octave, side 6 dB under the mid (r 0.60); after, the side times the LR4 high band: at 150 Hz cutoff, 37.5 Hz -48.2, 75 Hz -24.6, 150 Hz -6.0, 300 Hz -0.5, 600 Hz -0.03 dB.
- Page 7: noise (M^2 = S^2): width 0/50/100/150/200%: level -3.01/-2.04/0/+2.11/+3.98 dB, r 1/0.60/0/-0.38/-0.60. Pad (M^2 = 4 S^2): level -0.97/-0.71/0/+0.97/+2.04 dB, r 1/0.88/0.60/0.28/0.00.

### "Your track" rules (`stereo::analyse`)
Measured on the input over the last 10 s of 50 ms blocks whose input peak is above -50 dBFS; f = Mono below (120 Hz
when Off). Numbers rounded as shown. Rules in this order:
0. Under 2 s measured: only "Play the song where this track plays. DawIt Stereo needs a couple of seconds of sound to measure it."
1. Whole side 40 dB or more under the mid: only "Your track is mono: left and right are the same (the side is more than 40 dB under the mid). Mono below and Width have nothing to work on here. The pan knob places it; to make it wide, add a stereo reverb or a short delay on one side."
2. Low band (L and R power below f) 30 dB or more under the whole: "There's little below {f} Hz in this track ({x} dB under the rest), so Mono below changes little. It matters most on basses, kicks, pads, stereo pianos, reverbs and whole mixes." Rules 3 and 4 stay quiet.
3. Low side within 12 dB of the low mid, low correlation 0 or more. Off: "Below {f} Hz your track is wide: its side is only {7} dB under its mid (correlation {0.67}). In mono, or on a club's mono subwoofers, that part drops out, so the low end sounds different on every system. Try Mono below {f} Hz." On: "Mono below {f} Hz makes the low end mono: its side was {7} dB under its mid and is now {45} dB under (correlation {0.67} to {1.00})."
4. Low correlation under 0 (instead of 3): "Below {f} Hz left and right work against each other (correlation {-0.40}): in mono most of that low end cancels. This often comes from a stereo widener, two microphones on one source, or a polarity switch." then Off: "Mono below {f} Hz would fix the low end; also check the source (What is sound? > Phase cancellation)." On: "Mono below fixes the low end; also check the source (What is sound? > Phase cancellation)."
5. Width over 100% and the output's whole correlation under 0: "With Width at {180}% the whole track's correlation is {-0.20}: left and right work more against each other than with each other. It can sound hollow, and in mono only its mid is left, so it drops back much further than the rest of the mix. Try 130% or less."
6. Width 100% or less and the input's whole correlation under 0: "Your track's left and right work against each other (correlation {-0.30}): in mono parts of it cancel. Look for a polarity switch or a stereo widener earlier in the chain; Width under 100% softens it."
7. None of 2 to 6: "Below {f} Hz your track is close to mono already (side {25} dB under the mid), and the whole track's correlation is {0.45}. Mono below changes little here, and that's fine."
8. Listen not Stereo (added to any of the above): "Listen is set to {Mono}: the track plays {in mono | only its side}. Switch back to Stereo before you export."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Off, 100%, Stereo, 0 dB, match off: output vs input | identical | 0 |
| LR2, LR4, LR8 at 120 Hz, mono sine at 30, 60, 120, 240, 1000, 10000 Hz | 0.00 dB | 0.1 dB |
| Low and high band at the crossover (each slope) | -6.02 dB | 0.1 |
| High band one octave below 120 Hz: LR2 / LR4 / LR8 | -13.98 / -24.61 / -48.20 dB | 0.3 / 0.3 / 0.5 |
| 50 Hz, R 90 degrees later, mono below 250 Hz LR4: side after vs before | at least 50 dB down (calc. 55.9) | - |
| same: output correlation | 1.000 | 0.001 |
| 5 kHz on the left only, 120 Hz LR4: left level / right level | 0.00 dB / under -90 dB | 0.1 dB |
| Width 0%: L - R of output | 0 | 1e-6 |
| Width 200%, Off: output side vs 2 x input side | equal | 1e-6 |
| Any width, Off, match off: L + R out vs in | equal | 1e-6 |
| Width 0% on independent noise (power) | -3.01 dB | 0.02 dB of the value computed from the buffer's M and S |
| Listen Mono: L = R = (L + R) / 2 in; Side: L = R = (L - R) / 2 (Off, 100%) | equal | 1e-6 |
| One-channel buffer | unchanged | 0 |
| Correlation: L = R sine; R = -L; 90 degrees; independent noise 10 s; one side silent; silence | 1.000; -1.000; 0.00; abs < 0.03; 0; none | 0.001; 0.001; 0.01 |
| `stereo` formulas: width level (noise 150%, 200%), pad r at 150%, mono drop at r -0.5 | +2.11, +3.98 dB, 0.28, -6.02 dB | 0.01 |
| Slope change and Off to 120 Hz while a 0.5 1 kHz sine plays: largest sample step | under 0.1 | - |
| Match on, width 200%, wide noise, after 1 s: level change | 0 dB | 0.5 dB |
| Mono below 120 Hz on a wide mix (noise + wide 60 Hz sine), match off: level change | under 1 dB | - |
| `analyse`: each rule 0 to 8 on made-up measurements (0.5 s; side -50; low share -35; low side -7 r 0.67 Off and On; low r -0.4; width 180 r -0.2; in r -0.3; low side -25 r 0.45; Listen Mono) | that rule's text, with "120 Hz" where it says f | - |
| Other test sounds unchanged by the stereo sound: clap, note, hits in hiss | identical in every channel | 0 |
| Guide: 11 pages, each with text and a picture; search "goniometer", "mono", "vinyl", "correlation" finds it | - | - |

### Text change in an existing guide
*What is sound?* > *Phase cancellation* (`SoundGuide.cpp`), the line "- Keep the lowest notes in the middle and on one
sound: one bass, one kick, the low end of a stereo sound mono." gets " DawIt Stereo's Mono below does that last part."
No contradiction found: *What is sound?* says "nearly everything below about 150 Hz" is in the middle, DawIt Reverb's
*Bass* starting point "0%: mono, so the low end stays in the middle", DawIt Delay "Check it in mono".

## Steps
Group A, engine (build at step 4):
- [x] 1. `fx::Clap`: the *Wide bass* stereo sound (per-channel values); others unchanged. Tests: the sound's low-band correlation about 0, the other sounds identical per channel. Done when: tests pass.
- [x] 2. The shared stereo measurement and the `daw::stereo` pure functions (new engine file in both CMake targets). Tests: correlation, side-to-mid, band magnitudes, width formulas (*Test numbers*). Done when: they pass.
- [x] 3. `BuiltInStereo` (recipe step 1-2): parameters, crossover LR2/4/8, mono low band, width, Listen, match, output, smoothing and crossfades, one-channel pass; starting points; feeds for the spectrum, scopes, level history, preview; register after DawIt Gate. Tests: the sound checks, `checkThroughProject`, starting-point texts non-empty.
- [x] 4. `stereo::analyse` with the *Your track* rules. Tests: each rule on made-up measurements. Done when: `./build.sh --test` passes without warnings.

Group B, UI (build at step 9):
- [x] 5. The shared goniometer and correlation-bar painter and the two colour helpers (`src/ui`, documented in ui-design.md's colours).
- [x] 6. `StereoEditor` on `fxui`: the spectrum with the draggable line and its menu, the two scopes with bars, level history, knob groups, *Wide bass*, *Hear what it removes*, explainer texts and tooltips; `createEditorFunction` in `MainComponent`. Check: open it on Keys with DawIt Reverb before it.
- [x] 7. *What is stereo width?* pages (`src/ui/StereoGuide`), `gen::guides` entry and Mixing summary, `MainComponent::openGuide`, `BuiltInGuides` branches; the *What is sound?* sentence. Tests: the guide entry, search words, page count; `--export-guides` passes.
- [x] 8. Device bar: the chip's goniometer, MONO/SIDE label, match badge (`canMatch`, `isMatching`, `setMatching`), gain-staging stop.
- [x] 9. Build. Done when: `./build.sh --test` passes without warnings.

- [x] 10. Real-app check. `ScreenshotRunner` gains, for good: `"addPlugins"` (built-in names added to the shot's track when missing), `"preset"` (a starting point by name for the plugin opened) and `"clap"` for a plugin window; documented in `DevScreenshots.h`. Shots on Keys with `addPlugins` ["DawIt Reverb", "DawIt Stereo"], playing from beat 32: the window at *Mix bus* (amber dots in a vertical line, pink cloud, Low bar near +1 out); at *Wide pads*; the window with `clap`; the guide at *Seeing stereo*, *Mono below: the crossover*, *Width* and *Your track*; the device bar (chip goniometer); with *Mono check* (MONO on the chip). Look for cut-off text, empty scopes, wrong numbers.
- [x] 11. Docs: README (Features bullet under the built-in effects, Tests, Code layout), FEATURE_IDEAS status, docs/architecture.md (*Built-in effects*: the shared stereo measurement and painter, any-reader rings), recipes.md (screenshot keys), ui-design.md (the two colours, the scope sizes).

## Tests
`runBuiltInStereoTests` and `runStereoMeasureTests`: every row of *Test numbers*; the starting points' names and texts;
`checkThroughProject<BuiltInStereo>`; `runTheoryGuidesTests` finds "What is stereo width?" by "goniometer" and "vinyl".

## Website hand-off
- Features page: a *DawIt Stereo* section beside the other built-in effects: mono below a frequency, width, the
  goniometer and correlation meter, *What is stereo width?*.
- Screenshots: the window on Keys after DawIt Reverb at *Mix bus* while playing; the guide at *Seeing stereo* and *Your
  track*; the device bar chip.
- Guide picker: *Mixing* gains *What is stereo width?*. Main page: not needed. How it's made: no change.

## Left for later
- Pan, Balance and Swap left and right (decision 4).
- A whole-song scan of every track's low-end width (with *Analyze the mix*).
- A mono button on the master strip and effects on the master (the master track idea).
- Linear-phase crossover (decision 5). DawIt Meter reuses the stereo measurement and painter.

## Changes while building

- The spectrum uses an 8192-sample FFT, so its frequency resolution depends on the sample rate. The four-second level history shows 50 ms block peaks. The guide's width picture overlays its level and correlation curves with separate scales; its pad spectra use lines for before and after.
- The crossover uses the project's own IIR coefficient and state calculations rather than JUCE's filter classes, so all three Linkwitz–Riley slopes and their transition paths share the same processing structure. No library was added.
- ScreenshotRunner's `addPlugins` accepts all built-ins, but its plugin-window `preset` and `clap` options currently support DawIt Stereo. Guide-window `clap` remains available for all built-in guides.
- The planned advice says Mono below "makes the low end mono" even when the measured side remains audible near the crossover: in the real-app Keys check, the input side was 8 dB under the mid and the output side 9 dB under. The crossover is gradual, so this wording and its rule should be checked against the theory before release.

- Fixed after the Keys check: the processor was right (a test with a wide 60 and 90 Hz bass shows the output's low side more than 20 dB further under its mid). The Keys low band was mostly sound close to 120 Hz, where the gradual crossover leaves the side nearly untouched. The "Your track" advice now says "makes the low end mono" only when the side drops by 6 dB or more, otherwise "changes this low end little" with the reason and a suggestion (higher frequency or steeper slope), and it says "the difference between left and right (the side)" and "what they share (the mid)".
