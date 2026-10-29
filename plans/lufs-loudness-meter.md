# LUFS explained: what loudness is and how it's measured

Status: Approved
Idea: [LUFS explained: what loudness is and how it's measured](../FEATURE_IDEAS.html#lufs-explained-what-loudness-is-and-how-its-measured)

## Goal
A built-in metering effect, **DawIt Meter**, that shows loudness the way streaming services and broadcasters measure it (momentary, short-term and integrated LUFS, loudness range, true peak) against a target, and passes the sound through untouched. It is also built as a VST3 and Audio Unit plugin like the other built-ins. Its guide, **What is loudness? LUFS explained**, explains why peaks aren't loudness, K-weighting, gating, true peak and normalisation, and reads your song's numbers against the target. *File > Measure Loudness...* measures the whole song offline, since the master has no effects chain yet.

## Decisions for you
1. **What goes in the first version.** Options: (a) the effect and its guide only; (b) also *Measure Loudness...*, an offline measurement of the whole song (or the loop) that opens the guide's *Your song* page; (c) also a live loudness meter built into the engine's master output. Included either way: the correlation bar (it reuses `stereo::Measurement` and the `StereoScope` painter). Left for later either way: the goniometer and spectrum in the meter, and the guide's *Try it* buttons. **Recommended:** (b). Without it, DawIt can only measure the whole song by putting every track on one bus, because the master has no chain. (b) reuses the bounce renderer and doesn't touch the audio thread. (c) changes `Engine::processBlock` and is better done with *A master track in the arrangement and the mixer*, where DawIt Meter can simply sit last on the master.
   Answer: (b) the effect and guide plus *Measure Loudness...* (accepted).
2. **Hear it normalised** (a switch that plays the song turned by the gap to the target, so a loud master can be heard as Spotify would play it). It is the only control that would change the sound, so the meter would no longer be a pure pass-through while it's on. **Recommended:** later, with the *Try it* buttons. The guide shows the numbers (how much each service turns the song down) instead.
   Answer: Later, with the *Try it* buttons (accepted).
3. **One guide or two.** Options: one guide built on the plugin (`fxui::Guide` pages, which also work inside the VST3/AU and from the Theory Guides window), or a separate app guide (`src/engine/LoudnessGuide` and an explainer window) plus a short "What is a loudness meter?". **Recommended:** one guide, *What is loudness? LUFS explained*, under Mixing, not on *Start here* (the path stays short and ends with EQ and compression; the gain staging guide links to it).
   Answer: One guide, *What is loudness? LUFS explained* (accepted).
4. **The Club starting point** (-8 LUFS, -1 dBTP). Club masters are often -9 to -6 LUFS, but that is common practice, not a published target. **Recommended:** keep it, with a *why* text that says it's a choice and what it costs.
   Answer: Keep the Club starting point (accepted).
5. **Mono channels.** BS.1770 measures a mono channel as one channel, so a mono track in another DAW reads 3 LU lower than the same sound on both sides of a stereo track. Some meters offer "dual mono" (+3 dB). **Recommended:** measure as the standard says, and say so in the tooltip and on *How DawIt Meter works*. Dual mono goes under *Left for later*.
   Answer: Measure as the standard says (accepted).
6. **Name and plugin code.** "DawIt Meter", code `Dmtr`, VST3 category `Fx|Analyzer`. The code is permanent once a session is saved with it. **Recommended:** as given.
   Answer: DawIt Meter / `Dmtr` (accepted).

## What the user does and sees
- **Adding it:** *+ Effect* lists DawIt Meter after DawIt Tune. It goes anywhere in a chain: after the last effect of a track, on a bus, or on a bus that holds the whole mix. It has no latency and no change of level, so the gain staging badge reads ±0 (check `GainStaging` treats it so).
- **The window** (`fxui::EffectEditor`, 1240 × 640):
  ```
  [DawIt Meter | Starting points: Streaming ▾ | What is loudness? | Play the calibration tone | ?]
  ┌ readings ───────────────┐┌ the last 2 minutes ─────────────────────────────┐┌ explainer ┐
  │ M  ▮▮▮▮▮▮▯▯  -12.4 LUFS ││ momentary (thin) and short-term (thick) lines,   │           │
  │ S  ▮▮▮▮▮▯▯▯  -13.1 LUFS ││ the target as a dashed line, integrated as a     │           │
  │ Integrated   -13.8 LUFS ││ dotted one; -48 to 0 LUFS                        │           │
  │ to target +0.2 LU       ││                                                  │           │
  │ LRA 5.2 LU  Max S -11.0 │└──────────────────────────────────────────────────┘           │
  │ True peak -0.8 dBTP     │ Levels: L/R bars with sample peak hold, true-peak mark, dB scale│
  │ Sample peak, RMS, crest │ Correlation bar (the StereoScope painter)                      │
  └─────────────────────────┘                                                               │
  [Target: Target, Ceiling, Show target] [Measure: Reset, Reset on play]                     │
  ```
  The M and S bars run from -60 to 0 LUFS with a tick at the target. The integrated number and its "to target" line take the meter's colour helper: green within ±1 LU of the target, yellow outside it, and grey with *Show target* off. Louder than the target isn't red: it isn't a fault. True peak and the levels turn red above the ceiling. A reading with no data shows "–". Bypassed, it stops measuring.
- **Interaction:** a click on the integrated number or the true peak resets the measurement (the same as *Reset*). Hovering over the history shows "1:12 ago · M -10.8 · S -12.1 LUFS". A right-click on the history or the readings opens *Reset*, *Reset on play*, then *What is loudness?...* last. The explainer tells about whatever the mouse rests on. *Play the calibration tone* (the frame's clap button) plays the EBU check tone. There is no match level and no *Hear only what it adds* (hide the frame's listen button).
- **Reset on play** (on by default): when the play head goes from stopped to playing, a new measurement starts. While a host reports itself stopped, integrated, LRA and the maxima don't count; with no play-head information the meter always measures.
- **The device bar chip** (`chainpictures`): the short-term LUFS as a number, a thin bar with the target tick, and a red dot while the true peak is over the ceiling. Tooltip: "DawIt Meter: short-term -13.1 LUFS, integrated -13.8 LUFS, true peak -0.8 dBTP. Double-click for the meter."
- ***File > Measure Loudness...*** (after *Bounce to WAV...*, no shortcut): renders the loop range when the loop is on, otherwise the whole song, through a progress window with *Cancel*. Then it opens the guide at *Your song* with the result: integrated, LRA, max short-term, true peak, sample peak, the range measured and its length. The result is kept in memory until the next measurement and isn't saved.
- **Settings:** nothing in the app settings. Everything is the plugin's state.

## Data and saving
Plugin parameters in the `AudioProcessorValueTreeState`, saved with the project like any plugin: target (-30 to -5 LUFS, default -14), ceiling (-3 to 0 dBTP, default -1), show target (default on), reset on play (default on). Measurements are never saved. There is no project format change. Test with `checkThroughProject<BuiltInMeter>` on non-default values.

## The guide
A built-in's guide ([recipes.md](../docs/recipes.md) *A built-in effect or instrument*, step 4), in `src/ui/MeterGuide.{h,cpp}`, titled **What is loudness? LUFS explained**. Its pictures are drawn by the meter's own code on made-up signals, so their numbers are computed, not typed in. Pages (the text is in *Theory*):

| # | Page | Summary | Picture |
|---|---|---|---|
| 1 | Peaks aren't loudness | Same peak, 11 dB apart | A steady tone and the same tone in 40 ms bursts, both peaking at -6 dBFS, with their measured short-term LUFS (-6.0 and -17.0) |
| 2 | What LUFS means | Loudness units, relative to full scale | A LUFS scale from -60 to 0 with -23 (broadcast), -16 (Apple Music), -14 (streaming), -9 to -6 (club) marked, and the meter's short-term as a moving dot |
| 3 | K-weighting | The filter that makes the meter hear like an ear | The K-weighting curve from 20 Hz to 20 kHz (the meter's filter, computed), labelled at 20 Hz -13.3, 50 Hz -3.9, 100 Hz -1.1, 1 kHz +0.7, 4 kHz +4.0 dB |
| 4 | Momentary, short-term, integrated | 0.4 s, 3 s and the whole song, with gating | The example song (intro -40, verse -16, chorus -11 LUFS) as M and S lines, the gated-out intro shaded, integrated -12.8 drawn across. The live history when the meter has 30 s or more |
| 5 | Loudness range | How far the loudness moves, in LU | A histogram of the example's short-term values with the 10th and 95th percentiles marked: LRA 5 LU. Live when measured for 60 s or more |
| 6 | True peak | The peaks between the samples | Tech 3341's signal 19: samples at exactly 0 dBFS, and the wave through them reaching +3.0 dBTP, with both meters' readings |
| 7 | Targets and normalisation | What each service turns up or down | Bars per target with the gain each would apply to the integrated loudness (live, or a master at -9 LUFS) |
| 8 | Loud masters and the loudness war | Squashed masters lose, once normalised | Two masters before and after normalisation to -14 LUFS: dynamic (-14 LUFS, -1.0 dBTP) and squashed (-7 LUFS, -0.3 dBTP, peaking at -7.3 dBTP after) |
| 9 | Loudness while mixing | No target, but fair comparisons | An effect off and on: short-term -17.0 against -15.2 LUFS, so turn it down 1.8 dB to compare |
| 10 | How DawIt Meter works | The steps from samples to numbers | The block diagram: K-filter, mean square, 400 ms and 3 s windows, gates, histogram; the 4× true-peak path |
| 11 | Your track | Your numbers against the target | The integrated and true-peak numbers against the target and ceiling, the M/S history, and the advice |

*Your track* follows the *"Your track" rules* in *Theory*. It uses the instance's own readings when the meter is in the song (the `fxui` host-track lookup says track, bus or return). Opened without an instance (from the Theory Guides window or *Measure Loudness...*), it shows the last *Measure Loudness* result, or a *Measure the song* button. In a plugin there is no song measurement: the page reads the instance as "This track". The page's name in the list is *Your track*, as in the other built-ins.

**Theory Guides entry:** `Guide::loudness`, area Mixing, name "What is loudness? LUFS explained", summary "Why peaks aren't loudness, LUFS and K-weighting, momentary, short-term and integrated loudness, loudness range, true peak, streaming targets and normalisation, the loudness war, and your song's numbers.", opensFrom "DawIt Meter's window (What is loudness?), File > Measure Loudness..., and the gain staging guide", plugin "DawIt Meter", keywords "loudness LUFS LKFS LU loud quiet meter metering loudness-meter integrated short-term momentary gating LRA loudness-range true-peak dBTP inter-sample intersample peak K-weighting BS.1770 R128 EBU ATSC A/85 normalisation normalization Spotify Apple YouTube Tidal Amazon podcast broadcast streaming target mastering limiter loudness-war crest PLR headroom". Not on *Start here*. **Linked from** the gain staging guide's *Louder sounds better* page (one sentence, in *Theory*).

## Code
Follows recipes.md *A built-in effect or instrument* (copy DawIt Stereo, also a measuring effect with any-reader rings), *A command or menu item*, and *A long job* (copy how `BounceDialog` runs `bounce::Renderer` on a progress thread).

- **`loudness::Meter`** (`src/engine/LoudnessMeter.{h,cpp}`, no UI, no JUCE UI types): BS.1770-5 loudness and true peak for 1 or 2 channels at any sample rate. The K-weighting biquads are designed per sample rate from the stage parameters in *Theory* (they reproduce the standard's 48 kHz table). Momentary uses a 400 ms sliding window, short-term a 3 s one. Every 100 ms the meter adds a 400 ms block (integrated) and a short-term value (LRA) to fixed **histograms** of 0.1 LU bins from -70 to +10 LUFS, with a count and an energy sum per bin. Integrated and LRA are recomputed from them every 100 ms with both gates, so memory doesn't grow however long it runs. True peak uses the standard's 48-tap, 4-phase FIR, at every sample rate. It also gives the sample peak, the 3 s RMS and the crest factor, the maxima, the measured time and a "has reading" flag. Pure helpers sit next to it: K response, gain to target, Spotify-style up-gain, the advice (`loudness::advise`). The same class serves the effect and the offline measurement, so both show the same numbers.
- **`BuiltInMeter`** (`src/engine/BuiltInMeter.{h,cpp}`, identifier "DawIt Meter"): output = input, bit for bit, with no latency. It feeds a `loudness::Meter` and a `stereo::Measurement`. It publishes the readings as atomics and a 2-minute history ring at 10 Hz (M and S) that any number of views can read without taking from each other (like `stereo::Measurement`). The UI asks for a reset through an atomic flag, and the audio thread carries it out. Reset on play reads `getPlayHead()`. Starting points are the targets. The test sound is a new `fx::Clap` sound: the calibration tone.
- **Threading:** the audio thread owns the meter's state and never allocates or locks. The UI reads only atomics and the rings. *Measure Loudness* makes the `Renderer` on the message thread and renders on the job's thread into its own `loudness::Meter`, through a public render-to-callback on `bounce::Renderer` (it has `run` with a sink, but it's private). The result goes back to the message thread.
- **Plugin build:** one `dawit_add_builtin (Meter DISPLAY_NAME "DawIt Meter" KIND effect CODE Dmtr VST3_CATEGORY "Fx|Analyzer" ...)` with `LoudnessMeter.cpp` and `StereoMeasure.cpp` as engine sources, and `MeterEditor`, `MeterGuide` and `StereoScope` as UI sources. It isn't `APP_ONLY`. The editor and guide must not include `Engine.h`, `TheoryGuides.h` or app UI. The song measurement reaches the guide through an `fxui` hook that `MainComponent` sets, next to the host-track lookup, and that stays empty in plugins.
- **Risks:** the K filter and gating details (a 3 dB error is the classic bug, from one channel against two); covered by the Tech 3341/3342 tests at 48 and 44.1 kHz. A built-in whose output must equal its input exactly: test it with random noise.

## Libraries and models
None. Plain DSP. The design constants for other sample rates come from libebur128 (MIT), cited as a source, with no code taken.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | LUFS = Loudness Units relative to Full Scale; LKFS (BS.1770's name) is the same; 1 LU = 1 dB | EBU Tech 3341 §2.5; ITU-R BS.1770-5 | exact |
| 2 | Loudness = -0.691 + 10 log10 Σ G_i z_i (mean squares of K-weighted channels); G = 1.0 for L, R, C; 1.41 for surrounds; LFE excluded | BS.1770-5 eq. 2, Table 3 | exact |
| 3 | A 0 dBFS 997 Hz sine in one front channel reads -3.01 LKFS; -0.691 cancels K's +0.691 dB at 997 Hz | BS.1770-5 Notes 1–2 | exact |
| 4 | So the same sine in both channels reads 0.00 LUFS (10 log10 (0.5 + 0.5) = 0) | derived from 2–3 | exact |
| 5 | K stage 1: shelf modelling the head as a rigid sphere, 48 kHz b = 1.53512485958697, -2.69169618940638, 1.19839281085285; a = -1.69065929318241, 0.73248077421585 | BS.1770-5 Table 1 | exact |
| 6 | K stage 2 (RLB high-pass), 48 kHz b = 1, -2, 1; a = -1.99004745483398, 0.99007225036621 | BS.1770-5 Table 2 | exact |
| 7 | Other rates: shelf f0 1681.974450955533 Hz, gain 3.999843853973347 dB, Q 0.7071752369554196, Vb = Vh^0.4996667741545416; high-pass f0 38.13547087602444 Hz, Q 0.5003270373238773 (bilinear with tan(π f0 / fs)); these reproduce 5–6 to 1e-11 | libebur128 `ebur128_init_filter`; checked numerically | exact |
| 8 | K response at 48 kHz: 20 Hz -13.3, 30 Hz -8.3, 50 Hz -3.9, 100 Hz -1.1, 997 Hz +0.69, 2 kHz +3.1, 4 kHz +4.0, 10 kHz +4.0 dB | derived from 5–6 | exact |
| 9 | Momentary: 0.4 s sliding window, ungated; short-term: 3 s; integrated: gated as BS.1770; update rate ≥ 10 Hz | EBU Tech 3341 §2.2 | exact |
| 10 | Gating: 400 ms blocks, 75% overlap; absolute gate -70 LUFS; relative gate 10 LU below the absolute-gated loudness; incomplete last block dropped | BS.1770-5 eq. 5–7; Tech 3341 §2.3 | exact |
| 11 | LRA: short-term values (3 s, ≥ 10 Hz), absolute gate -70, relative gate -20 LU, LRA = 95th − 10th percentile; not stable in the first 60 s | EBU Tech 3342; Tech 3341 §2.4 | exact |
| 12 | True peak: 4× oversampling (48-tap, 4-phase FIR in BS.1770 Annex 2), result in dBTP | BS.1770-5 Annex 2 | exact |
| 13 | 4× oversampling under-reads by at most 0.55 dB (content up to 0.45 fs), 0.69 dB (0.5 fs) | BS.1770-5 Annex 2, Attachment 1 table | exact |
| 14 | A sample-peak meter can under-read by 3 dB on a quarter-sample-rate tone; on transients under-reads of several dB are common | BS.1770-5 Annex 2, Attachment 1 | exact |
| 15 | Tech 3341 test 19: fs/4 sine, amplitude 1.41, phase 45° → samples at 0 dBFS, true peak +3.0 (+0.2/−0.4) dBTP; tests 15–18 → -6.0 dBTP | EBU Tech 3341 Table 1 | exact |
| 16 | Alignment: 1 kHz stereo sine at -18 dBFS reads -18.0 LUFS | EBU Tech 3341 §2.9 | exact |
| 17 | Tech 3341 tests 1–5, 9, 12 (levels, durations, expected ±0.1 LU) | EBU Tech 3341 Table 1 | exact |
| 18 | Tech 3342 tests 1–4: -20/-30 → 10, -20/-15 → 5, -40/-20 → 20, -50/-35/-20/-35/-50 → 15 LU, ±1 | EBU Tech 3342 Table 1 | exact |
| 19 | Tech 3342 example programmes: commercial (narrow) 5 LU, movie (wide) 15 LU | EBU Tech 3342 Table 1 | exact |
| 20 | EBU R 128: -23.0 LUFS, ±1.0 LU where exact isn't practical (live); true peak ≤ -1 dBTP | EBU R 128 (2023) | exact |
| 21 | ATSC A/85 (US TV): -24 LKFS, true peak -2 dBTP | Flux and Lawo manuals (searched) | typical (secondary sources agree) |
| 22 | Spotify: normalises to -14 LUFS (BS.1770); turns louder songs down; turns quieter ones up only while keeping 1 dB of true-peak headroom (-20 LUFS at -5 dBTP → +4 dB → -16 LUFS); Loud -11, Quiet -19; asks for ≤ -1 dBTP, ≤ -2 dBTP for masters louder than -14 | Spotify for Artists, loudness normalisation | exact (the vendor's own page) |
| 23 | Apple Music Sound Check: about -16 LUFS, on by default on new devices | MeterPlugs (Ian Shepherd), 2022 | typical |
| 24 | YouTube, Tidal, Amazon Music: about -14 LUFS | MeterPlugs; several guides (searched) | typical (not officially published) |
| 25 | Podcasts: often -16 LUFS (Apple among those using it); AES TD1008 suggests -16 for music streaming | Production Expert, Telos (searched) | typical |
| 26 | Lossy encoding can raise peaks; lower true-peak limits for data-reduced distribution | Spotify page; EBU R 128 item m | exact (as stated) |
| 27 | Loud pop and EDM masters are often about -10 to -6 LUFS integrated | Mastering the Mix, ask.audio (searched) | typical, varies by source |
| 28 | Dense masters often LRA 3–6 LU | violetrecording (searched); Tech 3342 NLR example 5 LU | typical |
| 29 | PLR = peak minus integrated loudness: a heavily limited remaster PLR 7 sounds feeble when normalised, a modern pop track 9, dynamic jazz 20; Dynameter uses the sample peak | Sound On Sound, Dynameter review (searched) | typical |
| 30 | A peak-to-loudness gap under about 8 dB tends to sound flat once normalised | Sound On Sound (Shepherd, said of PSR) | opinion |
| 31 | A sine's crest factor (peak − RMS) is 3.01 dB | derived: 20 log10 √2 | exact |
| 32 | Same peak, bursts on 8% of the time read 10 log10 0.08 = -11.0 LU lower | derived | exact |
| 33 | BS.1770 isn't meant to judge the loudness of pure tones | BS.1770-5 Annex 1 | exact |
| 34 | About 10 dB more sounds roughly twice as loud | the sound guide (existing) | typical |

The guides agree: the sound guide (sine RMS 3 dB under its peak, 10 dB ≈ twice as loud), the routing and gain staging guides (peaks around -6 dBFS on the master before mastering, -6 to -3 dB) and the stereo guide (correlation) say the same. No guide states a LUFS number yet, so none has to change.

### Topic texts

**1. Peaks aren't loudness** — *Same peak, 11 dB apart*

A peak meter shows the biggest swing of the wave. Your ears don't follow that. They follow how much sound arrives over a moment, not the single highest point.

The picture shows two sounds with the same peak, -6 dBFS. The first is a steady tone. The second is the same tone in short bursts, 40 ms on and then silence until the next one half a second later, like a hit on every beat at 120 BPM. A peak meter shows them as equal. DawIt Meter measures the steady tone at -6.0 LUFS and the bursts at -17.0 LUFS. The bursts sound for only 8% of the time, and 10 × log10 (0.08) is -11 dB.

The same happens in a mix. A drum loop and a pad can peak at the same level while the pad sounds much louder. A master squashed by a limiter peaks at -1 dB, just like a dynamic one, and sounds far louder.

That's why streaming services, broadcasters and mastering engineers measure loudness in LUFS, not in peaks. You still need the peaks, because they tell you whether something clips. DawIt Meter shows both.

**2. What LUFS means** — *Loudness units, relative to full scale*

LUFS stands for Loudness Units relative to Full Scale. It comes from the international standard ITU-R BS.1770, which broadcasters and streaming services use. You'll also see LKFS (Loudness, K-weighted, relative to Full Scale). It's the same unit under another name.

The reference is a full-scale sine wave. A 1 kHz sine peaking at 0 dBFS in both channels measures about 0 LUFS, and in one channel only, -3.0 LUFS. Music is far quieter than that steady, full-scale tone, so its loudness is almost always a negative number. Closer to 0 is louder: -8 LUFS is louder than -14 LUFS.

One LU (loudness unit) is one decibel. Turn a song up by 3 dB and it measures 3 LU higher, from -17 to -14 LUFS. A difference is written in LU ("the chorus is 4 LU louder than the verse"), and a level in LUFS.

Some numbers to place it. European television aims at -23 LUFS. Spotify plays songs at about -14 LUFS. A loud club master is often between -9 and -6 LUFS.

What LUFS doesn't tell you: whether the song is good, or how loud it is in your room (that depends on your volume knob and speakers). It measures the level in the file, weighted the way the ear hears, so that two songs at the same LUFS sound about equally loud. It works well for music and speech. The standard itself says it isn't meant for judging pure tones.

**3. K-weighting** — *The filter that makes the meter hear like an ear*

Our ears don't hear all frequencies equally. Deep bass needs much more energy to sound as loud as the middle, and the range from about 2 to 5 kHz stands out. So before measuring, the meter passes the sound through a filter called K-weighting.

It has two parts. The first raises everything above about 2 kHz, by up to 4 dB. It models how the head itself boosts those frequencies on their way into the ear. The second turns the low end down: about 1 dB at 100 Hz, 4 dB at 50 Hz, 8 dB at 30 Hz and 13 dB at 20 Hz. Around 1 kHz it changes almost nothing (+0.7 dB, which the formula takes off again).

What this means in a mix:
- A bass-heavy mix measures quieter than a peak or RMS meter suggests, because its energy sits where K-weighting counts less.
- A bright mix measures louder. A boost at 3 kHz raises the LUFS more than the same boost at 60 Hz.
- Sub-bass below 40 Hz hardly counts, yet it still takes up headroom. It can push the peaks up without making the song measure louder.

K-weighting is a simple model. It doesn't change with the playback volume the way hearing does (we hear more bass when it's loud). It was chosen because it matched listening tests on real music and speech well, not because it copies the ear exactly.

**4. Momentary, short-term, integrated** — *0.4 s, 3 s and the whole song*

A loudness meter gives three readings over three lengths of time.

**Momentary:** the last 0.4 seconds. It jumps with every beat and word. Use it to see single hits and words.

**Short-term:** the last 3 seconds. It follows a phrase, or a bar or two, and moves smoothly. This is the one to watch while you mix, to compare a verse with a chorus or an effect on and off.

**Integrated:** the whole song, or everything since you pressed Reset. It's the one number streaming services and broadcasters use, so it's the one a target means.

Integrated leaves out the quiet parts. The song is cut into 400 ms blocks, a new one every 100 ms. Blocks below -70 LUFS (near silence) are dropped. The rest are averaged, then blocks more than 10 LU below that average are dropped too, and the average is taken again. This is called gating. It means a quiet intro or a pause doesn't pull the number down.

An example: a song with a 10-second intro at -40 LUFS, a 40-second verse at -16 and a 40-second chorus at -11. Its integrated loudness is -12.8 LUFS. The intro is more than 10 LU under the average, so it's left out. Counted in, the result would be -13.3. Notice that -12.8 is closer to the chorus than to the verse. The average is taken over the sound's energy, not over the numbers, so loud parts count for more.

In DawIt Meter, integrated starts again when you press Reset, or when playback starts if *Reset on play* is on. For the real number, play the whole song from start to end. A loop of the chorus measures the chorus.

**5. Loudness range** — *How far the loudness moves, in LU*

Loudness range (LRA) says how much the loudness moves between the quieter and louder parts of a song, in LU. The EBU defines it in Tech 3342.

It's worked out from the short-term readings (3-second windows, ten a second). The quietest 10% and the loudest 5% are ignored, so a single bang or a fade-out doesn't count. LRA is the distance between what's left at the bottom and at the top. Like integrated loudness, it leaves out near-silence and very quiet parts (here, more than 20 LU below the average).

Typical values vary a lot by style:
- about 3 to 6 LU: a dense pop, EDM or rock master, where verse and chorus are close;
- more: songs that move between quiet and loud sections;
- 15 LU or more: film soundtracks and much classical music.

The example song (verse at -16, chorus at -11) has an LRA of 5 LU.

LRA isn't a score: a ballad and a club track should differ. It isn't the crest factor either. The crest factor is about peaks against the average from moment to moment; LRA is about the song's sections. Measure at least a minute of the song: the EBU says the reading hasn't settled during the first 60 seconds.

**6. True peak** — *The peaks between the samples*

A digital recording is a row of samples. The real sound wave, the one your converter or headphones make from them, is a smooth curve through those samples. Between two samples it can swing higher than either of them. These are inter-sample peaks.

A sample-peak meter looks only at the samples, so it can miss them. The picture shows a high tone, a quarter of the sample rate (12 kHz at 48 kHz), whose samples all sit at exactly 0 dBFS. The sample-peak meter says 0.0 dB. The wave between them reaches +3.0 dBTP, 3 dB over full scale. That's an extreme case, but on short, bright hits a sample meter often reads several dB low.

True peak, in dBTP (decibels true peak), is measured by working out the curve between the samples. DawIt Meter computes four points for every sample (4× oversampling), as ITU-R BS.1770 describes. That finds the peak to within about half a decibel.

Why it matters:
- A converter, or a phone playing the file, can clip on a peak that the sample meter never showed.
- Encoding to MP3, AAC or Ogg changes the wave slightly, and its peaks can come out higher than the original's.

That's why masters for streaming usually stop at -1 dBTP rather than 0 dBFS. Spotify asks for -1 dBTP or lower, and -2 dBTP for masters louder than -14 LUFS. European broadcast (EBU R 128) allows at most -1 dBTP, and US television (ATSC A/85) -2 dBTP. A limiter with true-peak detection, set to -1 dB, does the job.

DawIt Meter holds the highest true peak since Reset, and turns it red above the ceiling of your target.

**7. Targets and normalisation** — *What each service turns up or down*

Streaming services don't play every song as loud as it was mastered. They measure each song's integrated loudness and turn it up or down to one level, so you don't have to reach for the volume between songs. This is loudness normalisation.

Typical playback levels (each service sets its own, and they change now and then):
- Spotify, YouTube, Tidal, Amazon Music: about -14 LUFS.
- Apple Music, with Sound Check (on by default on new devices): about -16 LUFS.
- Podcasts: often -16 LUFS.
- European television and radio (EBU R 128): -23 LUFS, true peak at most -1 dBTP.
- US television (ATSC A/85): -24 LUFS, true peak at most -2 dBTP.

Turning down is simple: a master at -8 LUFS plays 6 dB quieter on Spotify. Turning up has a limit. Spotify turns a quiet song up only as far as its peaks allow, keeping the true peak at -1 dBTP or lower. Its own example: a song at -20 LUFS with a true peak of -5 dBTP is turned up 4 dB, to -16 LUFS, not to -14. Not every service turns quiet songs up at all.

Normalisation isn't everywhere. Listeners can switch it off, Spotify Premium also has a Loud (-11 LUFS) and a Quiet (-19 LUFS) setting, and a downloaded file, a CD or a DJ set plays the song as it is.

So a streaming target isn't a rule you must hit. It tells you how loud your song will be played. Master near it and what you hear is what listeners get. Broadcast is different: there, the number is a delivery requirement.

The picture shows how much each would turn your song up or down: the meter's integrated loudness when it has one, otherwise a master at -9 LUFS.

**8. Loud masters and the loudness war** — *Squashed masters lose, once normalised*

From the 1990s on, masters got louder and louder, as each record tried to jump out on the radio. A limiter makes a song louder at the same peak by flattening the peaks, so the average rises. This race is called the loudness war.

What you lose: the punch of the drums (their attacks are cut off), the difference between a quiet verse and a big chorus, and in the end clean sound. What you gain, where nothing is normalised, is a louder song, and louder sounds better at first.

Normalisation changes the sum. Take two masters:
- a dynamic one at -14 LUFS, peaking at -1.0 dBTP: 13 dB between its peaks and its loudness;
- a squashed one at -7 LUFS, peaking at -0.3 dBTP: only 6.7 dB between them.

On Spotify the squashed one is turned down 7 dB, so both are equally loud. Now the dynamic one's drums still rise 13 dB above the loudness, while the squashed one's peaks reach only -7.3 dBTP. Next to the other it sounds flatter and smaller, not louder.

To judge a master, look at the gap between the peaks and the loudness. The meter shows the crest factor (the peak minus the RMS over the last 3 seconds; a sine wave's is 3 dB), and *Your track* compares the peak with the integrated loudness. In one well-known comparison, a heavily limited remaster had a 7 dB gap and sounded feeble once normalised, a modern pop song 9 dB, and a dynamic jazz recording 20 dB.

Loud still has its place. Club and EDM masters are often -9 to -6 LUFS, because clubs and DJ software play files as they are, and the density is part of the sound. It's a choice, not a mistake. Make it on purpose, and compare at the same loudness.

**9. Loudness while mixing** — *No target, but fair comparisons*

While you mix, you don't need a loudness target. A mix keeps room for mastering, with peaks around -6 dBFS on the master as in the Gain staging guide, whatever its LUFS. The final loudness is set at the end, by the limiter.

Where a loudness meter helps while mixing:
- **Fair comparisons.** Louder sounds better, even by 1 dB. Put DawIt Meter after an effect and note the short-term loudness with the effect off. Switch it on, and turn the effect's output until the short-term reads the same. In the picture the effect adds 1.8 dB (-17.0 against -15.2 LUFS), so turn its output down 1.8 dB. Now you hear only what the effect does.
- **A reference song.** Import a finished song you like onto a track, put DawIt Meter on it, and turn the track down until its short-term matches your mix's. At the same loudness you hear the real differences in tone and punch.
- **Sections.** Watch the short-term in the verse and the chorus. If they read the same, the lift has to come from the arrangement.
- **Tracks and buses.** A meter on the drum bus or the vocals shows how loud they sit. Peaks can't tell you that, because a snare's peak is far above its loudness.

DawIt Meter passes the sound through untouched and adds no delay, so it can sit anywhere: after the last effect of a track, on a bus, or on a bus that holds the whole mix. *File > Measure Loudness...* measures the whole song at once.

**10. How DawIt Meter works** — *From samples to numbers*

1. **K-weighting.** Each channel goes through the two filters of ITU-R BS.1770, the head shelf and the low cut, set for the sample rate.
2. **Mean square.** The filtered samples are squared and averaged over 0.4 seconds (momentary) and 3 seconds (short-term), and left and right are added with the same weight. -0.691 + 10 × log10 of that gives LUFS.
3. **Every 100 ms** a new 400 ms block is counted for integrated loudness, and a new short-term value for the loudness range. They're counted in steps of 0.1 LU, so the meter can run for hours without needing more memory. Then the gates are applied: -70 LUFS and 10 LU below for integrated, and -70 LUFS and 20 LU below for the range, taken between the 10th and 95th percentiles.
4. **True peak.** Four points are worked out for every sample (BS.1770's 48-tap filter), and the highest is kept.
5. **Levels.** Sample peak, RMS and crest factor are measured on the sound as it is (not K-weighted) over the last 3 seconds. Correlation is measured as in DawIt Stereo.

The sound itself passes through unchanged, bit for bit, with no delay. The readings follow the EBU's Tech 3341 ("EBU Mode") and Tech 3342. DawIt's tests check them with the EBU test signals: a 1 kHz tone at -23 dBFS in both channels reads -23.0 LUFS, a tone at -20 dBFS followed by one at -30 dBFS gives a range of 10 LU, and a tone whose samples sit at 0 dBFS reads +3.0 dBTP.

A mono channel is measured as one channel, as BS.1770 says. So a mono track in another DAW reads 3 LU lower than the same sound in both channels of a stereo track.

**11. Your track** — *Your numbers against the target*: the text is made by `loudness::advise` (below).

**Gain staging guide, *Louder sounds better*** (one sentence appended): "To compare by loudness rather than by peaks, put DawIt Meter after the effect and match its short-term LUFS: *What is loudness? LUFS explained* tells how."

### Tooltips and explainer leads
- **Momentary:** "Momentary loudness: the last 0.4 seconds, K-weighted (ITU-R BS.1770). It jumps with every beat. In LUFS, closer to 0 is louder."
- **Short-term:** "Short-term loudness: the last 3 seconds. Watch it while mixing, to compare a verse with a chorus or an effect on and off. Streaming services play songs at about -14 LUFS on average."
- **Integrated:** "Integrated loudness: everything since Reset, leaving out near-silence and quiet parts (gating). The number streaming services use: Spotify about -14 LUFS, Apple Music about -16. Click to reset."
- **To target:** "How far the integrated loudness is from the target: +3.0 LU means a service at the target turns it down 3 dB."
- **LRA:** "Loudness range (EBU Tech 3342): how much the loudness moves between quieter and louder parts, in LU. A dense pop master is often 3 to 6 LU, a film 15 or more. Settles after a minute."
- **Max S:** "The loudest 3 seconds since Reset."
- **True peak:** "True peak: the highest point of the wave since Reset, also between the samples (4× oversampled). Streaming masters usually stop at -1 dBTP. Red above the target's ceiling. Click to reset."
- **Sample peak:** "Sample peak: the highest sample since Reset. It can read lower than the true peak, by 3 dB on some high tones."
- **RMS:** "RMS: the average level over the last 3 seconds, not weighted. A sine wave's RMS is 3 dB under its peak."
- **Crest factor:** "Crest factor: the peak minus the RMS over the last 3 seconds. Large for drums and dynamic mixes, small for squashed ones. A sine wave's is 3 dB."
- **Correlation:** DawIt Stereo's tooltip, unchanged.
- **History:** "The last 2 minutes: momentary (thin) and short-term (thick), the target dashed, integrated dotted. Hover for the time and the readings."
- **Target:** "Target loudness: the integrated loudness you aim for. The starting points set it (Streaming: -14 LUFS)."
- **Ceiling:** "True-peak ceiling: the highest true peak the target allows. -1 dBTP for streaming and European broadcast, -2 for US television."
- **Show target:** "Show the target line and colour the numbers by it. Off while mixing, when there's no target yet."
- **Reset:** "Start measuring again: integrated loudness, loudness range, the loudest values and the peaks."
- **Reset on play:** "Start a new measurement each time playback starts, so Play from bar 1 measures the song from its beginning. Off: keep adding up across plays."
- **Calibration tone:** "Plays a 1 kHz tone at -18 dBFS in both channels for 3 seconds. A correct meter reads -18.0 LUFS: the EBU's check. Turn your volume down first."
- **Overview (explainer):** "DawIt Meter measures loudness the way streaming services and broadcasters do (ITU-R BS.1770, EBU R 128), and the peaks, including those between the samples. It doesn't change the sound. Put it last on a track or a bus, play, and read the integrated loudness against a target."

### Starting points (target, ceiling, show target; why)
1. **Streaming (Spotify, YouTube, Tidal, Amazon)**: -14, -1, on. "These services play songs at about -14 LUFS. A master near it plays almost unchanged, and a louder one is turned down. Spotify asks for a true peak of -1 dBTP or lower (-2 dBTP for masters louder than -14 LUFS), because encoding can raise the peaks."
2. **Apple Music**: -16, -1, on. "Apple Music's Sound Check plays songs at about -16 LUFS. A master made for -14 LUFS plays 2 dB quieter there, which is fine."
3. **Podcast**: -16, -1, on. "Many podcast platforms ask for about -16 LUFS for a stereo show, with peaks at -1 dBTP or lower. Speech varies less than music, so it's easy to keep close to the target."
4. **Broadcast, Europe (EBU R 128)**: -23, -1, on. "European television and radio deliver every programme at -23.0 LUFS (±1 LU where that's not practical, as in live shows), with a true peak of -1 dBTP or lower."
5. **Broadcast, US (ATSC A/85)**: -24, -2, on. "US television asks for -24 LKFS (the same as LUFS) and a true peak of -2 dBTP or lower."
6. **Club and DJ**: -8, -1, on. "Clubs and DJ software usually play files as they are, without normalisation, so club and EDM masters are often loud: about -9 to -6 LUFS. That's a choice, not a rule, and it costs punch."
7. **Mixing (no target)**: -14, -1, off. "While mixing there's no loudness target. Keep headroom (peaks around -6 dBFS on the master), and use the short-term loudness to compare effects and sections at the same level."

The calibration tone: a 997 Hz sine at -18 dBFS in both channels, 3 s, with 10 ms fades.

### Example data
- **Same peak:** a stereo 997 Hz sine at -6 dBFS peak, 6 s → short-term -6.0 LUFS. The same with 40 ms bursts every 500 ms (2 ms fades) → short-term -6 + 10 log10 0.08 = -17.0 LUFS.
- **Example song:** stereo 997 Hz sines: 10 s at -40, 40 s at -16, 40 s at -11 dBFS. Abs-gated mean -13.33, relative gate -23.33, so the intro is gated out → integrated -12.8 LUFS. LRA: gate -33.3; 10th percentile -16, 95th -11 → 5 LU.
- **True peak:** fs/4 sine, amplitude 1.41, phase 45°: samples ±1.0 (0.0 dBFS), true peak +3.0 dBTP (20 log10 1.41 = 2.99).
- **Targets for a master at -9 LUFS:** streaming -5.0 dB, Apple and podcast -7.0, EBU -14.0, ATSC -15.0, club -1.0.
- **Loudness war:** dynamic -14.0 LUFS / -1.0 dBTP (gap 13.0) → unchanged; squashed -7.0 / -0.3 (gap 6.7) → -7.0 dB → peaks -7.3 dBTP.
- **Mixing:** effect off -17.0, on -15.2 LUFS short-term → +1.8 dB.

### "Your track" rules (`loudness::advise`)
Inputs: integrated I, short-term now, max short-term, max true peak TP, max sample peak P, LRA, measured seconds t, target T and ceiling C (with the target's name, or "your target of -12 LUFS" when no starting point matches), show target, host kind (track, bus or return; unknown in a plugin, or for a song measurement). Each item is one paragraph, in this order. Gaps are rounded to 0.1.
1. **No reading** (no block above -70 LUFS): "Nothing measured yet. Play the song: DawIt Meter measures what passes through it." Stop.
2. **t < 30 s:** "Measured over 0:24 so far. For the song's integrated loudness, play it from the start to the end." Continue, but skip 7 and 8.
3. **Host is a track or a return:** "DawIt Meter is on the track Lead vocal, not on the whole mix, so a target doesn't apply here. Short-term now -21.3 LUFS, loudest 3 seconds -18.9 LUFS. Use it to compare the track with an effect on and off at the same loudness." Stop. On a bus, start with "If the bus Drums holds the whole mix:" and go on.
4. **Show target off:** "No target while mixing. Integrated -18.2 LUFS, true peak -5.9 dBTP: 5.9 dB of headroom." Then 8 only.
5. **TP > C:** "The true peak reaches +0.4 dBTP, above the -1.0 dBTP ceiling for Streaming. Encoding to AAC or Ogg can make it clip. Set your limiter's ceiling to -1.0 dB with true-peak detection, or lower the level going into it."
6. **I vs T:**
   - |I − T| ≤ 1: "Integrated -14.3 LUFS: within 1 LU of the -14 LUFS target, so it plays almost unchanged."
   - I > T + 1, normalised targets (Streaming, Apple Music, own targets): "Integrated -8.2 LUFS: Spotify plays it 5.8 dB quieter, at -14 LUFS. If you like how dense it sounds, keep it. If you made it loud to sound louder on streaming, it won't: try backing off the limiter by 3 to 4 dB and compare both at the same loudness." (For Apple Music, "Apple Music"; for an own target, "a service at -12 LUFS".)
   - I > T + 1, delivery targets (Podcast, both Broadcast): "Integrated -19.6 LUFS: 3.4 LU louder than the -23 LUFS that European broadcast asks for. Turn the whole mix down 3.4 dB."
   - I < T − 1, Streaming: up = min (T − I, −1 − TP). "Integrated -19.0 LUFS, 5.0 LU under -14. Spotify turns quieter songs up only while the true peak stays at -1 dBTP or lower: with -4.0 dBTP it adds 3.0 dB, to -16.0 LUFS, so it plays 2.0 dB quieter than most songs. Other services may not turn it up at all. To sit with them, raise the level into a limiter." When up = T − I: "...it can turn it up the full 5.0 dB."
   - I < T − 1, other targets: "Integrated -26.1 LUFS, 3.1 LU under the -23 LUFS target. Raise the whole mix 3.1 dB, and keep the true peak under -1.0 dBTP."
   - Club, I < T − 1: "Integrated -11.2 LUFS, quieter than a typical club master (-9 to -6 LUFS). A DJ can turn it up, but next to other tracks it may sound smaller. A limiter brings it up."
7. **LRA** (t ≥ 60): below 3: "Loudness range 2.4 LU: the quiet and loud parts are very close. If the chorus should lift, try less compression or limiting on the whole mix, or more contrast in the arrangement." Above 15: "Loudness range 17.0 LU: as wide as a film. On a phone or in a car the quiet parts may get lost. Gentle compression on the whole mix narrows it." Otherwise quiet.
8. **Peak to loudness** (t ≥ 30, P − I < 8 dB): "The peaks are only 6.7 dB above the integrated loudness. Masters this dense often sound flat next to dynamic ones once they're normalised. For more punch, let the limiter do less." Otherwise quiet.
9. **Nothing from 5–8:** "Nothing stands out: the loudness, the peaks and the range all suit Streaming."

A song measurement (from *Measure Loudness...*) starts with "The whole song (bar 1 to bar 37, 3:12):" or "The loop (bars 9 to 17, 0:16):", and t is the rendered length.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| K coefficients designed at 48 kHz vs BS.1770 Tables 1–2 | equal | 1e-9 |
| K response, 48 kHz: 20 Hz / 50 / 100 / 997 / 4000 Hz | -13.28 / -3.93 / -1.13 / +0.69 / +3.97 dB | 0.02 |
| 997 Hz stereo sine at 0 dBFS, both channels / left only (48 and 44.1 kHz) | 0.00 / -3.01 LUFS | 0.05 |
| Tech 3341 #1 (-23 dBFS, 20 s) M, S, I; also at 44.1 kHz | -23.0 | 0.1 |
| Tech 3341 #2 (-33) M, S, I | -33.0 | 0.1 |
| Tech 3341 #3, #4, #5 integrated | -23.0 | 0.1 |
| Tech 3341 #9 short-term after 3 s; #12 momentary after 1 s | -23.0 | 0.1 |
| Tech 3341 #15–18 / #19 max true peak | -6.0 / +3.0 dBTP | +0.2 / −0.4 |
| Tech 3342 #1 / #2 / #3 / #4 LRA | 10 / 5 / 20 / 15 LU | 1 |
| Example song integrated; with only the absolute gate; LRA | -12.8 / -13.3 / 5 | 0.1 / 0.1 / 1 |
| Same peak: steady / bursts short-term | -6.0 / -17.0 LUFS | 0.1 / 0.3 |
| Calibration tone at the end, short-term | -18.0 LUFS | 0.1 |
| Sine: crest factor; RMS of a -6 dBFS sine | 3.01 dB; -9.01 dBFS | 0.05 |
| Silence or -80 dBFS only | integrated has no reading | — |
| Correlation, same in both / inverted | +1 / −1 | 0.01 |
| Pass-through: noise in, out (gain 1, any target) | bit-identical; latency 0 | exact |
| Histogram: 10 min of -23 dBFS tone, integrated | -23.0, no allocation after prepare | 0.1 |
| Reset flag; reset on play (play head stopped → playing) | integrated cleared | — |
| Spotify up-gain: -20 LUFS, -5 dBTP | +4.0 dB → -16.0 LUFS | 0.05 |
| Gain to target: -9 LUFS vs each starting point | -5, -7, -7, -14, -15, -1 dB | 0.05 |
| Advice rules 1–9 on made-up readings | each text appears exactly when its condition holds | — |
| Starting points: 7, each with a why; target and ceiling as listed | — | — |
| Save and load (`checkThroughProject`) target -16, ceiling -2, show target off, reset on play off | same | exact |
| Guide: entry with plugin "DawIt Meter", 11 pages each with text; search finds it by "LUFS", "true peak", "loudness war", "normalisation", "LKFS", "R128" | — | — |

## Steps
Group A, engine (`./build.sh --test` at step 4):
- [ ] 1. `loudness::Meter` in `src/engine/LoudnessMeter.{h,cpp}` (both CMake targets) with the pure helpers. Tests: `runLoudnessTests` with the K, Tech 3341 (1–5, 9, 12, 15–19), Tech 3342 (1–4), example-song, same-peak, crest, histogram and silence rows. Done when: they pass at 48 and 44.1 kHz.
- [ ] 2. `fx::Clap` gets the calibration tone (3 s), other sounds unchanged. `BuiltInMeter` (recipe step 1): parameters, pass-through, readings and history ring, reset flag, reset on play, `stereo::Measurement`, starting points. Register it in `BuiltInFormat` and with `dawit_add_builtin (Meter ...)` (UI sources added in group B; until then leave the editor out). Tests: pass-through, latency, correlation, reset, calibration tone, starting points, `checkThroughProject`.
- [ ] 3. `loudness::advise` with the *"Your track"* rules, and the public render-to-callback on `bounce::Renderer` plus `loudness::measureSong` (renders a range into a meter, checks a cancel flag). Tests: every rule; a measured 4-bar song with one steady tone matches the live meter within 0.1 LU.
- [ ] 4. Build. Done when: `./build.sh --test` passes without warnings and the registry test lists DawIt Meter.

Group B1, UI: the window and the chip (`./build.sh --test` at step 7):
- [ ] 5. `MeterEditor` on `fxui` (`src/ui/MeterEditor.{h,cpp}`): the readings, M/S bars, levels, correlation bar (`StereoScope` painter), history with hover, the knob groups, the explainer texts, tooltips, the right-click menu, the hidden listen button. Add the meter's colour helper next to the editor and name it in ui-design.md's colours. Set `createEditorFunction` in `MainComponent`. Complete the `dawit_add_builtin` call with the UI sources.
- [ ] 6. Device bar: the chip's picture and tooltip (`chainpictures`), and the ±0 badge.
- [ ] 7. Build. Done when: it passes without warnings and the window opens from the chip.

Group B2, UI: the guide and Measure Loudness (`./build.sh --test` at step 11):
- [ ] 8. `MeterGuide` (`src/ui/MeterGuide.{h,cpp}`): the 11 pages with pictures and texts from *Theory*, `liveText` for the live pages, `setPrintSource`, and *Your track* via `fxui::getHostTrack`.
- [ ] 9. Theory Guides: `Guide::loudness` entry, `MainComponent::openGuide`, `BuiltInGuides` branches. Append the sentence to gain staging's *Louder sounds better*. Tests: entry, page count, the search words.
- [ ] 10. *File > Measure Loudness...* (`CommandIDs`, a `LoudnessActions` on a progress window copying `BounceDialog`'s use of `Renderer`). It keeps the last result and opens the guide at *Your track* through the `fxui` song-measurement hook (empty in plugins).
- [ ] 11. Build, `--export-guides` into the scratchpad, `./build.sh --plugins`. Check `auval -v aufx Dmtr Dwit`. Don't install the plugins without asking. Done when: all pass without warnings.

Group C, check and docs:
- [ ] 12. Real-app check. Add for good to `ScreenshotRunner`: a `meter` scene that puts DawIt Meter on the Drums bus and plays the song through once offline (like `tune`), and a `loudness` scene that runs *Measure Loudness* on the whole song without the progress window. Shots: the meter window on Drums after the `meter` scene (numbers filled in, history drawn, nothing cut off); the device bar with the chip; the guide at *True peak*, *Momentary, short-term, integrated* and *Your track* after the `loudness` scene (the song's numbers and advice); the `pluginEditor` scene for DawIt Meter with `"overlay": true` at page 1.
- [ ] 13. Docs: README (a Features bullet for DawIt Meter and *Measure Loudness...*, the plugins bullet's count 13 → 14), docs/tests.md (the loudness tests), docs/code-layout.md (`LoudnessMeter`, `BuiltInMeter`, `MeterEditor`, `MeterGuide`, `LoudnessActions`), architecture.md *Built-in effects* (the shared loudness measurement), recipes.md (the screenshot scenes), FEATURE_IDEAS: this idea partly done (what's left from *Left for later*); the mastering chain and gain staging ideas note the meter exists. Add a line to the plan's *Changes while building*.

## Tests
As in *Test numbers*, in `runLoudnessTests` (the meter and helpers), `runMeterEffectTests` (the processor, starting points, saving) and `runTheoryGuidesTests` (entry and search). The EBU signals are made in the test as stereo 1000 Hz sines (997 Hz for the 0 dBFS checks) with the levels and durations of Tech 3341/3342; the meter is reset before each.

## Website hand-off
- Features page: a *DawIt Meter* section under the built-in effects, with LUFS, true peak, the targets, *Measure Loudness* and the guide; one line in the plugins section (now 14).
- Main page: worth a mention under mixing ("How loud is my song? DawIt Meter and the LUFS guide").
- Screenshots: the meter window on the Drums bus of the example song after a full play; the guide's *True peak* page; *Your track* after *Measure Loudness* on the example song; the plugin editor with the guide overlay.
- How it's made: no new library. A line that the meter is checked against the EBU Tech 3341 and 3342 test signals.

## Left for later
- A live loudness meter on the master and in the mixer's master strip (with *A master track in the arrangement and the mixer*).
- *Hear it normalised* and the guide's *Try it* buttons (turn up, add a compressor, limit, *Match loudness*).
- A goniometer and spectrum in the meter, and drawing K-weighting over the song's spectrum.
- A loudness check in *Bounce to WAV*, and short-term LUFS beside the gain staging arrows.
- Dual-mono measurement, the EBU +9/+18 scales and an LU (relative) display, surround and per-channel loudness.
- PSR (peak to short-term loudness) as a separate reading, and loudness per track in *Analyze the mix*.

## Changes while building

- Started the engine meter: K-weighting, sliding windows and gated histograms are in place. Step 1 remains open: true peak, remaining readings, helpers and the full conformance tests are still needed.
