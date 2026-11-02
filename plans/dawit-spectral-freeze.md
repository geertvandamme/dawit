# DawIt Spectral Freeze: hold a sound and turn it into a texture

Status: Released
Idea: [#123 — DawIt Spectral Freeze: hold a sound and turn it into a texture](https://github.com/geertvandamme/dawit/issues/123)

## Goal
DawIt Spectral Freeze is a built-in effect that takes the last moment of a track's sound and holds it for as long as you like: a piano chord becomes a pad, a breath a soft wind, a cymbal a steady wash. Its window shows the live spectrum and the held snapshot, and its guide *What is a spectral freeze?* explains spectra, frequency bins, the trade between time and frequency resolution, and why a frozen spectrum sounds unlike a looped sample. A beginner learns that a sound is a list of frequencies, and what is lost when time is taken out of it.

## Decisions for you
1. **When the song stops.** Options: (a) the hold carries on through stop and play, like a held pedal, until Freeze is clicked off; (b) stopping the song lets go of the hold. **Recommended: (a)**: a freeze is for holding a sound after it ends, also on live input with the song stopped; (b) would leave Freeze lit with nothing heard. The window, chip and tooltip say "Holding" so it's never a mystery.
   Answer: (a) keep holding through stop and play until Freeze is clicked off.
2. **Save the hold in the project.** Options: (a) save the snapshot with the plugin's state (about 40 KB per held instance, nothing when nothing is held); (b) save only the settings, so a reopened project holds nothing until the next capture. **Recommended: (a)**: reopening sounds as it did when saved, and a bounce is repeatable.
   Answer: (a) save the snapshot with the plugin's state.
3. **Resolution.** Options: (a) fixed at about 93 ms (4,096 samples at 44.1/48 kHz, 8,192 at 88.2/96 kHz, 16,384 above); (b) a switch (Smooth 23 ms / Normal 93 ms / Detailed 372 ms). **Recommended: (a)**: one fewer control; the guide's *Time or frequency* picture shows the three lengths on the user's own sound. The switch is in *Left for later*.
   Answer: (a) fixed at about 93 ms.
4. **Test sound.** Options: (a) a plucked A minor chord that is captured 0.25 s in (turning Freeze on), so one click demonstrates the effect; (b) the chord only, the user clicks Freeze. **Recommended: (a)**, with the tooltip saying it replaces the hold.
   Answer: (a) the A minor chord, captured 0.25 s in.
5. **Movement's randomness on stereo.** Options: (a) the same random values on both sides, so a mono sound stays mono; (b) independent, which also widens. **Recommended: (a)**: predictable and mono-safe; a Width control is in *Left for later*.
   Answer: (a) the same random values on both sides.
6. **Name and plugin code.** *DawIt Spectral Freeze*, code `Dfrz` (permanent once saved), VST3 category `Fx|Spectral`. **Recommended:** as written. The guide notes it's not the "Freeze track" of other DAWs.
   Answer: As written: DawIt Spectral Freeze, `Dfrz`, `Fx|Spectral`.

## What the user does and sees
Added like any effect (device bar *+*, the inspector, the mixer). Window: the `fxui::EffectEditor` frame ([ui-design.md](../docs/ui-design.md#effect-and-instrument-windows)), 1240 × 620.

```
[DawIt Spectral Freeze] [Starting points v] [? What is a spectral freeze?] [Test sound]           Holding · A3
┌ Hold ──────────────────────────┐ ┌ Colour ─────────────┐ ┌ Output ──────────┐ ┌ explainer ┐
│ [ FREEZE ] [Capture again] Fade│ │ Brightness Movement │ │ Mix     Output   │ │           │
└────────────────────────────────┘ └─────────────────────┘ └──────────────────┘ │           │
┌ Spectrum: 20 Hz … 20 kHz, log; live input (dim line), the hold (filled, accent), ┐│           │
│ tallest hold peak labelled "220 Hz · A3"                                         ││           │
└──────────────────────────────────────────────────────────────────────────────────┘│           │
┌ Level, last 4 s (LevelHistoryView) ──────────────────────────────────────────────┐└───────────┘
```

- **Freeze** (a large switch, lit while holding): on captures the last 0.15 s or so and fades the hold in; off fades it out. **Capture again** (enabled while holding) takes a new snapshot that crossfades with the old over Fade.
- **Knobs:** Fade 10 ms–10 s (log, default 300 ms); Brightness −6…+6 dB/octave (default 0); Movement 0–100% (default 20%); Mix 0–100% (default 100%); Output −24…+12 dB (default 0).
- **Spectrum picture:** hover shows "1,250 Hz (near D#6) · hold −32 dB · live −40 dB"; click plays nothing (it's a measurement); right-click menu: *Capture again*, *Let go*, then the guide.
- **Header status:** "Not holding", "Holding · A3" (nearest note of the tallest peak) or "Holding · noise".
- **Starting points** with why (*Theory*). Explainer shows the topic of the control under the mouse.
- **Device bar chip:** a small hold spectrum when holding (accent), the live one dim otherwise.
- Tooltips: *Theory*, *Tooltips*. No app settings; everything is plugin state.

## Data and saving
Plugin state only: the parameters plus, when held, the snapshot (per channel and bin: level and frequency, with its sample rate and size) as a base64 child of the state XML. No project format change; projects without the plugin are unaffected. Loading with Freeze on and a snapshot fades the hold in over Fade; Freeze on without a snapshot (a host that dropped it) holds silence until *Capture again*. `checkThroughProject<BuiltInSpectralFreeze>` saves a held 1 kHz sine and checks it sounds after loading.

## The guide
*What is a spectral freeze?*, an `fxui::Guide` in `src/ui/SpectralFreezeGuide.{h,cpp}` (copy `StepFilterGuide`). Pages (text in *Theory*):

| # | Page | Summary | Picture |
|---|---|---|---|
| 1 | Holding a moment | A sound that never ends | the test chord's waveform dying away (grey) and its hold carrying on (accent); live hold level |
| 2 | A sound as a spectrum | How loud each frequency is | the live input spectrum and the hold, log axis with octave marks and note names, tallest peak labelled |
| 3 | Frequency bins | The FFT's slices of the spectrum | the hold around its tallest peak, one bar per bin, bin frequencies under them |
| 4 | Time or frequency | You can't have both sharp | two notes a semitone apart (A3 + A#3 test tones, or the input ring) analysed at 1,024 / 4,096 / 16,384 samples side by side, with each window's length drawn as a bar |
| 5 | What a snapshot keeps | Level and pitch, not time | a made-up snare hit over 0.5 s (attack, then decay) above its hold, flat in time |
| 6 | Freeze or loop? | Why a hold isn't a repeat | a 0.15 s loop of the snare repeating with seams marked, above the freeze's even hold |
| 7 | Movement and brightness | Bring a still hold to life | the hold with the tilt line through 1 kHz and a band for Movement's ±dB |
| 8 | Fade, mix and letting go | How the hold starts and stops | timeline: Freeze on, Fade, holding, Freeze off; the dry and hold gain curves for the current Fade and Mix |
| 9 | Make a pad from your sound | Step by step, and what it can't do | four chords (Am F C G, 2.5 s a bar) with a capture on each and the holds crossfading |
| 10 | How DawIt Spectral Freeze works | Pieces, spectra, new pieces | boxes: last second → 4 windows → FFT → snapshot → new piece every 23 ms → inverse FFT → overlap → fade/mix/output |
| 11 | Your track | What the hold is made of | the hold's spectrum with flatness and nearest note, the advice lines |

Pages 1 and 4 have a clap callback: page 1 *Hear a freeze* (the test chord, captured); page 4 *Play two notes* (A3 + A#3, 1 s, not captured). *Your track* uses `fxui::getHostTrack` and `sfz::analyse`.

**Theory Guides entry:** area *Effects*, title "What is a spectral freeze?", summary "Holding a moment of sound as a texture: spectra, frequency bins, time against frequency, why a freeze isn't a loop, and your track.", opens from "DawIt Spectral Freeze's window (What is a spectral freeze?)", plugin "DawIt Spectral Freeze", words: `spectral freeze spectral-freeze hold sustain infinite texture pad drone spectrum fft fourier bins frequency-bins resolution window hann phase-vocoder snapshot ambient`. Not on *Start here*. Topics from `BuiltInGuides::getPageNames`.

## Code
Follow recipes.md *A built-in effect or instrument*, copying DawIt Step Filter (`BuiltInStepFilter`, `StepFilterEditor`, `StepFilterGuide`); pure maths in `sfz::` beside the processor and tested.
- **STFT.** Size N as in decision 3, hop N/4, periodic Hann for analysis and synthesis, `juce::dsp::FFT`. Output normalised by the window-squared overlap sum (1.5).
- **Capture** (on the audio thread, at the first sample where Freeze goes on or *Capture again* is asked): from a 1 s input ring per channel, the newest 4 frames; level per bin = RMS over the 4 frames; frequency per bin from the phase step between frames 3 and 4 (the phase-vocoder estimate, wrapped). Also stored: the loudest hop's level in the second before (for advice).
- **Hold voice:** each hop, every bin's phase advances by its frequency, plus Movement's jitter (uniform ±Movement·π); its level wobbles (smoothed random, ±Movement·6 dB, new target about once a second, average power kept) and takes Brightness's tilt (dB/octave about 1 kHz), then the whole is scaled back to the untilted hold's total power. Random phases lose overlap coherence, so correct the level for Movement (the measured tests set the curve).
- **Fade and mix:** angle θ goes 0 → Mix·π/2 over Fade on capture and back on release; dry × cos θ, hold × sin θ (equal power, same as `fx::getMixGains` at the end). With nothing held the output equals the input exactly. *Capture again*: the new voice rises as the old falls; at most two voices.
- **Rules:** latency 0 (the hold is made from the past; dry is untouched), so no `KeepsLatencyWhenBypassed`. Stop/play don't touch the hold (decision 1). `prepareToPlay` keeps the snapshot, remaps it by frequency if the rate changed, and reseeds the random generator, so two bounces are identical. Same random values on both channels. Mono and stereo layouts. `getTailLengthSeconds` returns Fade.
- **Threading:** parameters are atomics. Only the audio thread writes a snapshot, into a free slot published by an atomic index and generation; message-thread readers (editor, guide, chip, `getStateInformation`) copy and re-check the generation. `setStateInformation` fills an incoming slot the audio thread adopts at the next block. No locks or allocation on the audio thread. The test chord's capture is counted on the audio thread; the processor's timer then sets Freeze on the message thread.
- **Live spectrum:** a non-consuming input ring like `BuiltInEq::readPreview`; editor, guide and chip do their own display FFT.
- **Risk:** the level normalisation (do the measured level tests first) and the snapshot hand-off between threads.

## Libraries and models
None. `juce::dsp::FFT` and plain DSP.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | Bin spacing = sample rate ÷ FFT size: 44,100 ÷ 4,096 = 10.77 Hz; 48,000 ÷ 4,096 = 11.72 Hz; 96,000 ÷ 8,192 = 11.72 Hz | derived (DFT definition) | exact |
| 2 | Window length = size ÷ rate: 4,096 ÷ 44,100 = 92.9 ms; ÷ 48,000 = 85.3 ms; 1,024 → 23.2 ms; 16,384 → 371.5 ms; bins 43.1 / 2.69 Hz | derived | exact |
| 3 | Bin spacing × window length = 1 (finer frequency means longer time) | derived (fs/N · N/fs) | exact |
| 4 | A real FFT of N samples has N/2 + 1 bins from 0 Hz to half the rate: 2,049 bins, 0 to 22,050 Hz | derived | exact |
| 5 | Bins between 20 and 40 Hz at 10.77 Hz: 2 (21.5, 32.3); between 10 and 20 kHz: 929 | derived (k = 929 … 1,857) | exact |
| 6 | Hann main lobe is 4 bins wide at its base | Smith, SASP, *Resolving Sinusoids* | exact |
| 7 | Hann −6 dB width 2.0 bins, −3 dB 1.44 bins, highest side lobe −31.5 dB | window table (metricgate, after Harris 1978) | exact |
| 8 | Sinusoids are reliably separated about one main-lobe width apart; less is possible without noise | Smith, SASP | typical |
| 9 | Semitone ratio 2^(1/12) = 1.0595: A1 55 → 58.27 Hz (3.27 Hz, 0.30 bin); A3 220 → 233.08 (13.1 Hz, 1.2 bins); A5 880 → 932.33 (52.3 Hz, 4.9 bins); semitones clear 4 bins (43.1 Hz) from about 725 Hz | derived | exact |
| 10 | Periodic Hann squared, overlapped at a quarter of its length, sums to 1.5 everywhere | derived (mean of w² is 3/8, × 4) | exact |
| 11 | A bin's exact frequency comes from how far its phase moved between two frames, minus the bin centre's expected advance, wrapped, divided by the hop; with hop N/4 it's unambiguous within ±2 bins | Dudas & Lippe, *The Phase Vocoder – Part I* (Cycling '74); Dolson 1986 | exact |
| 12 | Holding one frame (speed zero) sounds static and mechanical; a small random phase deviation breaks that | Dudas & Lippe, *Part II* | opinion (widely used practice) |
| 13 | PaulStretch randomises the phases and uses long windows, giving a smooth, smeared sound | Csound manual, *paulstretch* | exact (method), opinion (sound) |
| 14 | Loss of phase coherence between bins gives phase-vocoder sound a "phasy", less present quality | Laroche & Dolson 1999 | typical |
| 15 | A magnitude snapshot has no time order: attacks and rhythm inside it are lost | derived (definition) | exact |
| 16 | A 0.15 s loop repeats 6.7 times a second (0.1625 s: 6.15) | derived (1 ÷ 0.15) | exact |
| 17 | A loop clicks at the seam unless cut at matching points (zero crossings) or crossfaded; zero crossings alone don't make different moments match | *The Art of Looping* (muzines); HISE forum | typical |
| 18 | Spectral flatness = geometric ÷ arithmetic mean of the power spectrum, 0 (pure tones) to 1 (white noise); also called tonality coefficient or Wiener entropy; an MPEG-7 descriptor | Wikipedia, *Spectral flatness* (Johnston 1988) | exact |
| 19 | Equal-power mix at 50%: cos 45° = 0.707, −3.01 dB each; uncorrelated sounds then add to the level of either | derived; DawIt's `fx::getMixGains` | exact |
| 20 | Tilt +3 dB/octave about 1 kHz: 8 kHz +9 dB, 125 Hz −9 dB (before level match) | derived (3 × log2 (f/1000)) | exact |
| 21 | Capture span: 4,096 + 3 × 1,024 = 7,168 samples = 162.5 ms at 44.1 kHz, 149.3 ms at 48 kHz (and at 96 kHz with 8,192 + 3 × 2,048); the text says 0.15 s | derived | exact |
| 22 | Ableton Live's Spectral Time (FFT-based) has a freeze section | Ableton blog | exact |
| 23 | "Freeze" in a DAW's track menu renders a track to save processing; unrelated | common knowledge | exact |
| 24 | Granular synthesis plays many short overlapping grains of a recording | common knowledge (issue #83) | exact |

Facts 7, 8 and 14 rest on secondary pages; the text uses 7's 2-bin figure only, and 14 only as "can".

### Topic texts

**1. Holding a moment** — *A sound that never ends*

A spectral freeze takes one moment of a sound and keeps it sounding for as long as you like. Strike a piano chord, click Freeze while it rings, and the chord stays: no fading, no new attack, just the chord, held still. A breath becomes a soft wind; a cymbal becomes a steady wash.

It's for pads, drones and textures made from sounds you already have. Hold the last chord of a song under a spoken line. Hold a sung "ah" as a bed under a verse. Or stop a sound dead for a break.

It's called spectral because it doesn't keep the wave itself. It keeps the sound's spectrum: how loud each frequency was in the last 0.15 seconds before you clicked. From that list it builds new sound, over and over, so the hold has no beginning, no end and no loop that repeats.

What happens:
- Freeze on: DawIt takes the snapshot and fades the hold in over Fade (0.3 s to start with).
- While it holds: the hold plays with your track by Mix. At Mix 100% you hear only the hold.
- Freeze off: the hold fades out over Fade, and your track plays on as before.
- Capture again: a new snapshot fades in while the old one fades out.

This is not the Freeze in other DAWs' track menus, which renders a track to audio to save computer power. Spectral effects such as Ableton Live's Spectral Time have a freeze like this one.

Click Hear a freeze: a plucked A minor chord plays and is held 0.25 s after it starts. The picture shows the chord dying away in grey and the hold carrying on in colour.

**2. A sound as a spectrum** — *How loud each frequency is*

Any sound can be described as many sine waves at once, each at its own frequency and level. That list is its spectrum. What is sound? shows it for steady shapes: a note at 220 Hz has its fundamental at 220 Hz and harmonics at 440, 660 and 880 Hz and up.

The picture is the spectrum of what comes into DawIt Spectral Freeze right now: low frequencies on the left (20 Hz), high on the right (20 kHz), louder higher up. Every octave gets the same width, as you hear them.

- A held note shows a row of peaks, one per harmonic.
- A chord shows several rows mixed together.
- A breath or a cymbal shows a broad hill with no clear peaks. That's noise: every frequency at once.

The coloured shape is the hold, the snapshot DawIt Spectral Freeze plays. Its tallest peak is labelled with its frequency and nearest note. The tallest peak isn't always the note you played: it can be a harmonic, an octave or a fifth above it. Move the mouse over the picture to read the frequency, the hold's level and the live sound's level there.

**3. Frequency bins** — *The FFT's slices of the spectrum*

To find a spectrum, the computer takes a short piece of the sound, here 4,096 samples, and runs a fast Fourier transform (FFT) on it. The FFT answers one question for many frequencies at once: how much of this frequency is in the piece?

The frequencies it answers for are evenly spaced, like the rungs of a ladder. Each rung is a frequency bin. The spacing is the sample rate divided by the length of the piece: 44,100 ÷ 4,096 = 10.8 Hz. So the bins sit at 0, 10.8, 21.5, 32.3 Hz and so on, up to 22,050 Hz, half the sample rate: 2,049 bins in all.

Before the FFT, the piece is faded in and out with a smooth curve called a Hann window, so its hard edges don't add frequencies of their own. The price: one pure tone lights up a few neighbouring bins, not one. Its peak is 4 bins wide at the base (43 Hz here) and about 2 bins wide at half its height.

Even spacing means the bins are sparse at the bottom and crowded at the top. Between 20 and 40 Hz, the lowest octave you hear, there are only 2 bins. Between 10 and 20 kHz there are 929. That's why a freeze holds the detail of high sounds better than of low ones.

A note rarely sits exactly on a bin: 220 Hz falls between 215.3 and 226.1 Hz. The freeze still holds 220 Hz, because it also measures where in its cycle each bin's wave is (its phase) and how far that moves in 23 ms, which gives the exact frequency. See What a snapshot keeps.

The picture zooms in on the hold around its tallest peak, with one bar per bin.

**4. Time or frequency** — *You can't have both sharp*

A longer piece gives bins that are closer together, but it blurs time. A shorter piece follows quick changes, but its bins are further apart. You can't have both: the bin spacing times the length of the piece is always 1.

At 44.1 kHz:
- 1,024 samples: 23 ms long, bins 43.1 Hz apart.
- 4,096 samples, as DawIt Spectral Freeze uses: 93 ms, bins 10.8 Hz apart.
- 16,384 samples: 372 ms, bins 2.7 Hz apart.

Why it matters for a freeze. Two notes a semitone apart are about 6% apart in frequency. At A5 (880 and 932 Hz) that's 52 Hz, about 5 bins: the freeze holds them apart. At A3 (220 and 233 Hz) it's 13 Hz, about 1 bin. At A1 (55 and 58 Hz) it's 3.3 Hz, a third of a bin: they land in the same bins and come out blurred, as one rough tone. So low chords turn muddy in a freeze sooner than high ones. For a clearer hold, freeze a chord played higher up, or with its low notes spread apart.

A longer piece would separate low notes, but it would take in more of what changes: freeze a quick melody and you hold several notes at once. DawIt uses about 85 to 93 ms (4,096 samples at 44.1 and 48 kHz, 8,192 at 88.2 and 96 kHz), a common middle choice.

Click Play two notes to hear A3 and A#3 together. The picture shows them analysed at the three lengths: at 1,024 samples one hump, at 16,384 two clear peaks.

**5. What a snapshot keeps** — *Level and pitch, not time*

For each bin the snapshot keeps how loud it was, averaged over the last 0.15 seconds, and its exact frequency. That's why a held A stays an A. It also keeps where in its cycle each bin's wave was at the end (its phase), so the hold starts with a note's harmonics lined up as they were; from there each one simply carries on at its own frequency.

It keeps no order in time. Those 0.15 seconds become one still picture. So:
- Attacks are lost. The knock of a piano hammer, the crack of a snare and the start of a consonant are spread over the snapshot and lose their edge. A frozen snare becomes a steady "shhh" with the drum's colour; a frozen "k" becomes a hiss.
- Rhythm is lost. Freeze a drum loop and you hold a blend of whatever was in those 0.15 seconds.
- Vibrato stops. A singer's wobble becomes one steady pitch, or a slightly blurred one if the snapshot caught both ends of it.
- The moment matters. Click Freeze just after a piano note starts and you hold the bright beginning; click late and you hold the darker, quieter end.

The best sounds to freeze already hold still for a moment: a sung vowel, a held chord, a pad, a breath, the ring after a hit.

The picture shows a snare hit over half a second, and below it the hold made from it: the same colours, with no hit.

**6. Freeze or loop?** — *Why a hold isn't a repeat*

You could also hold a sound by looping a short piece of it, as a sampler does. A loop sounds different, for three reasons.
- A loop repeats. Loop 0.15 seconds and it plays about 6 or 7 times a second: you hear a flutter, a rhythm. A freeze builds each new piece from the snapshot with every wave carried on where it was, so there's no point where it starts over.
- A loop has a seam. Where the end jumps back to the start, the wave jumps too, and that clicks unless the loop is cut at matching points or crossfaded. Even then, two different moments of a sound don't always join smoothly.
- A loop keeps time. Everything in the piece comes back each time: the attack, the vibrato, a change of note. A freeze has no time in it.

A loop is better when you want the real sound, changes and all: a drum loop, a riff, a sampled note with its attack. A freeze is better when you want a still colour: a pad, a drone, a wash.

Granular synthesis sits between the two: it plays many short, overlapping grains of a recording, and can wander through it.

The picture shows a loop of about 0.15 s of the snare repeating, with its seams marked, above the freeze's even hold.

**7. Movement and brightness** — *Bring a still hold to life*

A hold made of perfectly steady frequencies can sound static and mechanical, most of all on noise such as a breath. Movement brings it to life with a little randomness, as many freezes do.
- Movement 0%: every bin keeps its level and its frequency. The cleanest hold of a chord or a vowel.
- Movement 20 to 50%: each bin's level drifts slowly up and down, and its phase is nudged at random. The hold breathes.
- Movement 100%: levels wander by up to 6 dB, about once a second, and each bin's phase is random every 23 ms. The hold shimmers and smears into an airy wash, like the extreme stretching of PaulStretch, which randomises phases on purpose. The pitches stay put, but a pure tone grows rough.

The level stays the same whatever Movement is set to.

Brightness tilts the hold's spectrum around 1 kHz, by up to 6 dB per octave. At +3 dB per octave, 8 kHz (three octaves above 1 kHz) comes up 9 dB and 125 Hz goes down 9 dB. DawIt then sets the hold back to the level it had, so Brightness changes the colour, not the loudness. A single pure tone has nothing to tilt and stays as it was.

A darker hold tends to sit behind other parts, such as under a vocal; a brighter one comes forward and adds air. That's a mixing habit, not a rule.

The picture shows the hold with Brightness's tilt line through 1 kHz, and a band for how far Movement lets each level wander.

**8. Fade, mix and letting go** — *How the hold starts and stops*

Fade sets how long the hold takes to come in when you click Freeze, and to go when you click it off: from 10 ms, a sudden stop, to 10 seconds, a slow swell. 0.3 s comes in smoothly without sounding slow. Even 10 ms is a short fade, not a jump, so it doesn't click.

Mix sets what you hear while it holds:
- 100%: only the hold. Your track goes quiet under it, which is how a freeze stops a sound in its tracks.
- 50%: both, each 3 dB down, so together they're about as loud as either alone.
- 0%: only your track.
When nothing is held, your track passes through untouched, whatever Mix says; only Output still sets its level.

What happens when:
- The sound stops: the hold goes on. That's the point: a breath keeps sounding after it ends.
- You click Freeze on silence: the hold is silent too. Click Capture again while the sound plays.
- The song stops or starts: the hold carries on, like a held pedal. Click Freeze off to let go.
- You save the project: the hold is saved with it and sounds the same when you open it again.
- You bypass it: your track plays untouched at once, and the hold stops.
- Timing: no delay. The hold is made from sound already heard, so your track stays in time.

DawIt can't yet switch Freeze on at a point in the song. To keep a hold, play to the moment, click Freeze, set Mix to 100% and bounce the track in place: the result is an audio clip of the hold.

**9. Make a pad from your sound** — *Step by step, and what it can't do*

Here is how to turn a sound of your own into a pad under a song, with the Am F C G of the example song (96 BPM, so a bar lasts 2.5 s).

1. Choose a sound that lasts a moment: a keys chord, a sung "ah", a breath or a cymbal. Put it on its own track, or add DawIt Spectral Freeze to the track that has it.
2. Add DawIt Spectral Freeze (the + Effect button in the inspector or the device bar). Press play.
3. Click Freeze while the sound rings, not on its first moment. The snapshot is the 0.15 s before your click, and the start of a note is noisy and still changing. A plucked chord also fades (the test chord loses 20 dB a second), so click about 0.25 s after it starts, as Test sound does. If the status says Holding but the hold is quiet, Your track says so: click Capture again.
4. Pick a starting point to begin from. Pad under the playing (Fade 1.5 s, Brightness -1 dB/oct, Movement 30%, Mix 50%) sits under the part you play. Held chord (Fade 300 ms, Movement 10%, Mix 100%) gives the clean hold. Breath texture (Fade 2 s, Movement 60%) suits a breath or a cymbal, which are noise.
5. Follow the chords. A hold stays on the chord it was taken from, so on each chord change click Capture again about 0.25 s after the new chord starts: then the 0.15 s it takes holds the new chord, not the end of the old one. The new hold crossfades with the old one over Fade: at 1.5 s that is the first 1.5 s of a 2.5 s bar.
6. Make it sit. Lower Mix so your track is heard through the hold. A darker hold tends to sit behind a vocal: try Brightness at -2 dB/oct or lower. On a noisy hold such as a breath, Movement of 20% or more keeps it from sounding static and mechanical; 30 to 60% is a good start.
7. Keep it. Set Mix to 100% and use Track > Bounce in Place: you get an audio clip of the hold. A bounce plays only the hold you have now, because DawIt can't yet capture at a point in the song by itself. For a pad that follows the chords, capture and bounce one chord at a time (Bounce in Place mutes the original track; unmute it to capture the next), then trim each clip to its bar. Copy a clip with Cmd-drag to use it under other sections.

What it is, honestly: an effect, not an instrument. It takes no notes. Playing a MIDI key doesn't change what it holds, and a hold stays at the pitch it was taken at: an A3 stays an A3. DawIt has no sampler yet, so there is no way yet to play a bounced hold across the keys; a held clip can only be placed on the timeline like any audio. The picture shows the four chords, a capture on each, and each hold crossfading into the next.

**10. How DawIt Spectral Freeze works** — *Pieces, spectra, new pieces*

1. It keeps the last second of your track.
2. When you click Freeze, it takes the newest 0.15 s as 4 overlapping pieces of 4,096 samples, 1,024 samples (23 ms) apart, each faded with a Hann window.
3. An FFT turns each piece into 2,049 bins. The snapshot keeps each bin's level over the four pieces, its exact frequency, from how far the bin's phase moved from the third piece to the fourth, and its phase in the fourth piece, where the hold starts from.
4. Every 23 ms it builds a new piece: each bin at its level, its phase carried on by its frequency, with Movement's randomness, tilted by Brightness and set back to the snapshot's level.
5. An inverse FFT turns the piece back into sound. It's faded with the same window and laid over the pieces before it, four overlapping at a time, so the joins can't be heard.
6. Fade brings the hold in and out; Mix sets it against your track with equal power; Output sets the level of everything.

Each side of a stereo track has its own snapshot. Movement uses the same randomness on both, so a mono sound stays mono. At 88.2 and 96 kHz the pieces are 8,192 samples, so they last as long and the bins stay about as far apart.

**11. Your track** — *What the hold is made of*

This topic looks at the hold on the track DawIt Spectral Freeze is on: how loud it is, whether it's mostly notes or mostly noise, its tallest peak and nearest note, and, in DawIt, whether that note is in the song's key. Play the track, click Freeze while a sound rings, and the advice follows.

(Then the `sfz::analyse` lines.)

### Tooltips
- Freeze: "Freeze: hold the last 0.15 s of sound for as long as you like. Click again to let go; the hold fades in and out over Fade."
- Capture again: "Take a new snapshot of the sound now. The new hold fades in as the old one fades out."
- Fade: "How long the hold takes to come in and to go: 10 ms stops a sound at once, 2 s or more swells in like a pad."
- Brightness: "Tilts the hold around 1 kHz: up makes it brighter and airier, down darker. The level stays the same."
- Movement: "0%: the hold is perfectly still. Higher: each frequency's level and phase wander at random, so the hold breathes and shimmers."
- Mix: "How much of the hold you hear against your track while it holds. 100%: only the hold. With nothing held, Mix leaves your track untouched."
- Output: "The level of everything that comes out."
- Spectrum picture: "Live sound (thin line) and the hold (filled), 20 Hz to 20 kHz. Move over it to read the frequency and levels."
- Test sound: "Plays a plucked A minor chord and holds it 0.25 s in, replacing any hold."
- Chip: "DawIt Spectral Freeze: holding A3" / "… not holding".

### Starting points
| Name | Fade | Bright. | Move. | Mix | Why |
|---|---|---|---|---|---|
| Held chord | 300 ms | 0 | 10% | 100% | Only the hold, still and clean, like a piano chord with the pedal down for ever. |
| Pad under the playing | 1.5 s | −1 dB/oct | 30% | 50% | The hold swells in slowly under what you play, a little darker so it sits behind. |
| Breath texture | 2 s | +1 dB/oct | 60% | 100% | A breath or cymbal becomes a moving wash; Movement keeps noise from sounding mechanical. |
| Dark drone | 3 s | −3 dB/oct | 20% | 70% | The highs rolled away: a low bed under a verse that doesn't fight the vocal. |
| Shimmer | 1 s | +2 dB/oct | 100% | 60% | Every frequency wanders: a glittering, restless hold above the track. |
| Sudden stop | 10 ms | 0 | 0% | 100% | The sound stops dead on one held moment, an effect for a break; 10 ms is fast but doesn't click. |

### Example data
- Test chord: A3 220 Hz, C4 261.63, E4 329.63, each with harmonics 2–6 at 1/n level, plucked (5 ms attack, decaying 20 dB/s), peak about −12 dBFS, 2 s; captured at 0.25 s.
- Two notes (page 4): A3 220 Hz + A#3 233.08 Hz, equal level, −12 dBFS together.
- Snare (pages 5, 6): noise burst plus 190 Hz body, 1 ms attack, noise decaying 60 dB in 150 ms, body in 80 ms.
- Brightness picture: tilt at ±3 dB/oct: 125 Hz ∓9, 1 kHz 0, 8 kHz ±9 dB.

### "Your track" rules (`sfz::analyse`)
Flatness over the band that holds the sound: bins from 50 Hz to 16 kHz whose 41-bin neighbourhood (power averaged) is within 20 dB of the strongest neighbourhood, powers floored at −120 dB. So a cymbal's 2–8 kHz wash counts as noise however empty the rest is. Thresholds 0.1 and 0.3 are DawIt's own. Levels in dBFS of the hold's RMS.
| # | Condition | Advice | Quiet when |
|---|---|---|---|
| Z1 | Freeze off | "Nothing is held. Play your track and click Freeze while the sound you want rings." | holding |
| Z2 | hold below −60 dBFS | "The hold is almost silent (−66 dBFS): the 0.15 s before you clicked were quiet. Click Capture again while the sound plays." | ≥ −60 |
| Z3 | hold ≥ 10 dB below the loudest 23 ms of the second before capture (and not Z2) | "You caught the sound as it faded: the hold is 14 dB quieter than the loudest moment just before. For a fuller hold, click Capture again just after the note starts." | < 10 dB |
| Z4 | flatness ≥ 0.3 and Movement < 20% | "Your hold is mostly noise (like a breath, a cymbal or a drum). At Movement 5% it can sound static or mechanical: try 30 to 60%." | flatness < 0.3 or Movement ≥ 20% |
| Z4b | flatness ≥ 0.3 and Movement ≥ 20% | "Your hold is mostly noise (like a breath, a cymbal or a drum). At Movement 25% its levels and phases keep wandering, so it moves like a soft wash instead of sounding still and mechanical." | flatness < 0.3 or Movement < 20% |
| Z5 | flatness < 0.1 | "Your hold is mostly notes. Its tallest peak is 220 Hz, nearest A3 (+4 cents); it may be a harmonic of the note played." | flatness ≥ 0.1 |
| Z6 | Z5 and the song's key known: peak's pitch class in key | "A is in the song's key (A minor), so the hold can stay under most chords as a drone." | no key (plugin in another host) |
| Z7 | Z5, key known, pitch class not in key | "A# is not in the song's key (A minor): it will clash with some chords. Capture a note from the key, or hold it only where it fits." | as Z6 |
| Z8 | 0.1 ≤ flatness < 0.3 | "Your hold is a mix of notes and noise (−18 dBFS)." | otherwise |

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| bin spacing 44.1 k/4,096; 48 k/4,096; 96 k/8,192 | 10.7666, 11.7188, 11.7188 Hz | 0.001 |
| window length at 44.1 k, 48 k | 92.88, 85.33 ms | 0.01 |
| bins 20–40 Hz; 10–20 kHz (44.1 k) | 2; 929 | exact |
| semitone gap at A1 / A5 in bins | 0.304 / 4.86 | 0.01 |
| Hann² overlap sum at hop N/4 | 1.5 at every sample | 1e-6 |
| tilt +3 dB/oct at 8 kHz, 125 Hz, 1 kHz | +9, −9, 0 dB | 0.01 |
| fade at Fade/2, Mix 100% | hold −3.01, dry −3.01 dB | 0.1 |
| capture span 44.1 k | 7,168 samples | exact |
| flatness: white-noise hold; test-chord hold | > 0.6; < 0.05 | — |
| flatness: 2–8 kHz noise hold; pink-noise hold | ≥ 0.3; ≥ 0.3 (measured 0.44, 0.51) | — |
| nearest note 220 Hz; 233.08 Hz | A3 0 ct; A#3 0 ct | 1 ct |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. `src/engine/BuiltInSpectralFreeze.{h,cpp}` (`sfz::` part): bin and window maths, Hann² overlap sum, tilt gain, fade/mix gains, phase-vocoder frequency estimate, spectral flatness, nearest note (via `soundguide::getNoteName`, cents derived). Tests: the *Test numbers* table.
- [x] 2. The processor: input ring, capture, two hold voices, Movement, Brightness, Fade/Mix, Output, parameters, test chord with sample-accurate capture, live feed, published snapshot; register with `dawit_add_builtin` (`Dfrz`, `Fx|Spectral`) and in `BuiltInFormat.cpp`. Tests: measured checks in *Tests*. Check: the level tests pass at three rates.
- [x] 3. Snapshot in the plugin state, rate remap, reseeding in `prepareToPlay`, six starting points with why. Tests: save/load, remap, determinism, presets.
- [x] 4. `sfz::analyse` (Z1–Z8) from a plain input struct. Tests: each rule both sides of its threshold. `./build.sh --test`. Done when: all checks pass, no warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 5. `src/ui/SpectralFreezeEditor.{h,cpp}`: knob groups, Freeze switch and Capture again, spectrum picture with hover and right-click menu, header status, level history, explainer texts, starting points; factory in `MainComponent`; device bar chip picture (`chainpictures`). Check: add to Keys, Test sound, the hold appears.
- [x] 6. `src/ui/SpectralFreezeGuide.{h,cpp}`: the ten pages with pictures, the two clap callbacks, *Your track*, `setPrintSource`; `BuiltInGuides` branches; `gen::guides` entry and `MainComponent::openGuide` case. Tests: Theory Guides entry with 10 topics; searches.
- [x] 7. Plugin check: `./build.sh --plugins`, `auval -v aufx Dfrz Dwit`, a `pluginEditor` shot with the overlay. `./build.sh --test`. Done when: all pass, no warnings, `--export-guides` passes.

Group C, check and docs:
- [x] 8. Real-app check: add DawIt Spectral Freeze to `ScreenshotRunner`'s `preset` and `clap` branches (clap plays the test chord and captures). Shots: Keys with `addPlugins`, preset *Held chord*, `clap: true`, `wait` 1500 (hold in the spectrum, "Holding · A3"); preset *Breath texture* on Vocals while playing; the guide at *Time or frequency* and *Your track* after a capture; the device bar chip; `pluginEditor` with overlay. Look for cut text, wrong Hz or notes, empty pictures.
- [x] 9. Docs: README Features bullet (the *Shape and mix sounds* line); docs/tests.md and docs/code-layout.md lines; architecture.md *Built-in effects* (a snapshot published between threads; the chip list); recipes.md's chip list; SOURCES.md checked; issue #123 progress.

## Tests
- `sfz::` maths: the *Test numbers* table.
- Held sine: 1 kHz at −12 dBFS, Movement 0, Brightness 0, Mix 100, held 10 s: −12 dBFS ±0.5 dB, frequency 1,000 ±1 Hz, RMS over every 50 ms within 0.5 dB; also 220 Hz (between bins) stays 220 ±0.5 Hz. At 44.1, 48 and 96 kHz and blocks of 17, 512 and 4,096.
- Noise: white noise at −20 dBFS RMS held at Movement 0 and 100: RMS within 1.5 dB of the input.
- Brightness: 250 Hz + 4 kHz at equal level, +3 dB/oct: 4 kHz over 250 Hz +12 ±0.5 dB, total power ±0.2 dB.
- Movement 0 is repeatable; Movement 100 after the same `prepareToPlay` gives identical output twice (bounce determinism).
- Fade 300 ms: hold at 150 ms −3.01 ±0.5 dB, full by 300 ms + one hop; after Freeze off, below −100 dBFS by Fade + one hop. Fade 10 ms: no sample step larger than twice the steady signal's largest.
- Capture again: level never dips more than 3.5 dB nor rises more than 1 dB during the crossfade (same sine).
- Not held: output equals input within 1e-6 at any Mix. Bypass: input unchanged. Latency 0.
- Silence: no hold, zero in → zero out (< 1e-9); hold of silence → below −100 dBFS; hold carries on after input stops.
- Stereo: L = R input stays L = R at Movement 100; left-only input gives a silent right hold.
- Rate remap: hold captured at 44.1 kHz, prepared at 96 kHz: still 1 kHz ±2 Hz, level ±0.5 dB.
- Save/load: `checkThroughProject<BuiltInSpectralFreeze>` with a held 1 kHz sine: Freeze on and the same output level after loading.
- Presets: six, each with a why, values in range.
- Analysis: Z2 at −61 fires, −59 doesn't; Z3 at 11 dB fires, 9 doesn't; Z4 at flatness 0.35 with Movement 10 fires, flatness 0.25 or Movement 25 doesn't; Z5 names A3 for 220 Hz; Z7 fires for A#3 in A minor, Z6 for A3; no key sentence without a key.
- Theory Guides: entry with 10 pages; "spectral freeze", "drone" and "FFT" find it.

## Website hand-off
- `features.html`, the effects group: DawIt Spectral Freeze, with a shot of the window holding the test chord (Keys, preset *Held chord*).
- `learn.html`: *What is a spectral freeze?* under Effects; the *Time or frequency* page is the picture worth showing.
- Not on Home or `for-you.html`; no `about.html` change (no library).

## Left for later
- A Resolution switch (23 / 93 / 372 ms), decision 3.
- Freezing on the beat or on each new note (Ableton's Sync and Retrigger modes); needs automation (#87) or a clock-driven capture.
- A Width control (independent randomness per side), decision 5.
- Playing the hold from MIDI notes at other pitches (spectral sampler; see #82, #83).
- Morphing between two holds belongs to #132.

## Changes while building

**Group A (engine), hand-off for Group B.** Built: `src/engine/BuiltInSpectralFreeze.{h,cpp}`, registered in `BuiltInFormat.cpp` and with `dawit_add_builtin(SpectralFreeze ...)` in `CMakeLists.txt` (engine sources only; `EDITOR BuiltInSpectralFreezeEditor` / `ui/SpectralFreezeEditor.h` are named but not yet written, so add `UI_SOURCES src/ui/SpectralFreezeEditor.cpp src/ui/SpectralFreezeGuide.cpp` there in Group B). Tests are in `tests/SpectralFreezeTests.h` (included by `EngineTests.cpp`; `runBuiltInSpectralFreezeTests`, `runSpectralFreezeAnalysisTests`).
- **What Group B calls** (all in `daw::BuiltInSpectralFreeze`, `daw::sfz`): `getSettings()/setSettings()/setKnob (sfz::Knob, v)/setFreeze (bool)` (Freeze is parameter `"freeze"`, a bool, outside `sfz::Knob`); `requestCapture()` (Capture again); `playTestChord()` (the clap callback: plays the chord, captured 0.25 s in on the audio thread, the processor's own timer then switches Freeze on); `isHolding()` / `getHoldAmount()` (header status, Freeze lit); `getSnapshot (sfz::Snapshot&)` (a copy; message thread; false when nothing was captured) and `hasSnapshot()`; `readInput (channel, dest, count)` (the live input ring, 1 s, not consuming); `getRate()`, `getFftSize()`; `levels` (`fx::LevelHistory`: input peak and the hold's peak as mixed in); `getKeyPitchClasses()` (from `SongContextReceiver`, 0 when unknown; the key's name is the UI's to supply); `createEditorFunction`.
- **Pure functions** (`sfz::`) the pictures can use: `getFftSize/getHopSize/getNumBins/getBinSpacingHz/getWindowMs/countBins/getSemitoneGapBins/makeHann/getTiltDb/getFadeGains/getCaptureSpan/getPhaseVocoderHz/getSpectralFlatness/getNearestNote/getNoteName/getPitchClass`; on a snapshot: `getSnapshotRms/getSnapshotFlatness (snap, channel)/getSnapshotPeakHz`; `formatKnobValue/formatKnob`; `getPresets()` (six; they leave Freeze alone, so the window's `applyPreset` sets the five knobs only); `analyse (sfz::AnalysisInput)` returns the Z1 to Z8 lines (fill `holdDb`, `priorPeakDb` from `20 log10 (snap.priorPeakRms)`, `flatness`, `peakHz`, `movementPercent`, the key).
- **Plugin build:** `SoundGuide.cpp` is not needed (see below); the engine file has no dependency outside `EffectTools.h` and `SongContext.h`.
- The **two failing checks in the full test run are the Theory Guides hub checks** ("every built-in effect and instrument has one guide in the hub", "no guide names a plugin that isn't built in"): they need the guide entry of step 6.

**Changes while building (Group A).**
- **Snapshot also stores each bin's phase** (newest piece), not only level and frequency, so the held sound keeps the harmonics' phase relations of the capture (about 64 KB base64 per held instance, not 40 KB). After a sample-rate remap the phases are the synthetic "sine peaking mid-piece" ones.
- **Level normalisation is measured, not a fixed curve.** Random phase and partial coherence make the overlap-added level depend on the sound (a sine is 1.0, noise is lower), so each voice measures its own output power (smoothed over about 0.25 s, seeded from the first hop) and scales it to the snapshot's power (Parseval over the bins, a sine of amplitude A is A squared over 2). This covers Movement, Brightness (the tilt changes the colour only) and noise with one mechanism; the gain is clamped to -12 to +18 dB. Measured: a held sine is within 0.01 dB of -12 dBFS over 10 s, noise within 0.1 dB at Movement 0 and 100%.
- **Capture again crossfades linearly (amplitude), not equal power**: two holds of the same sine are nearly in phase, and equal power would give a 3 dB bump (the plan's test allows at most +1 dB). A second capture inside a running crossfade drops the older voice at once.
- **Output applies to everything**, also with nothing held (at the default 0 dB the track is bit-identical to the input, which is what the test checks); Mix never touches the unheld track.
- **Own note naming** (`sfz::getNoteName`, middle C = C4, same as `soundguide::getNoteName`) instead of calling `soundguide`, so the plugin links without `SoundGuide.cpp` (which pulls in the pitch tracker).
- The hold's random values (jitter, level wander) are drawn per bin per piece for both channels together, three draws per bin whatever Movement is, so Movement 0 and 100 renders repeat exactly.
- First pieces: three virtual earlier pieces are laid in at the start of a hold so the first samples are as loud as the rest; they cost three extra inverse FFTs in the block where Freeze is clicked.

**Group B (UI), hand-off for Group C.** Built: `src/ui/SpectralFreezeEditor.{h,cpp}` (`BuiltInSpectralFreezeEditor`, `SpectralFreezeSpectrumView`), `src/ui/SpectralFreezeGuide.{h,cpp}` (`daw::sfztext`: texts, the spectrum helpers `computeSpectrum/getLiveSpectrum/getHoldSpectrum/getExampleSpectrum/drawSpectrum/getHoverText`, `paintMiniSpectrum` for the chip, `makeGuide` with the ten pages). Wired: `CMakeLists.txt` `UI_SOURCES`, `MainComponent` (factory, `openGuide` and `getPrintGuide` cases), `BuiltInGuides::makePages` (`TheoryGuidesWindow.cpp`), `gen::guides::Guide::spectralFreeze` (after the step filter; entry as in *The guide*), `ChainPictures.cpp` (`hasPicture`, `paintCurve`) and `DeviceBar.cpp` (`getSpectralFreeze` in the chip's three checks), the guide-switch cases in `ArrangementView.cpp` and `DevScreenshots.cpp` (`"guide": "spectralFreeze"` works in a shot list). `./build.sh --test` ends with All checks passed, no warnings; `--export-guides` passes (10 topics, 11 pages).
- **Still for Group C (`ScreenshotRunner`):** `preset` and `clap` branches for DawIt Spectral Freeze (`applyPreset` leaves Freeze alone, so a shot with `clap: true` calls `playTestChord()` and waits for Freeze to switch on; preset by name from `sfz::getPresets()` via `setSettings`). A plain `plugin` + `addPlugins` shot and the guide shots already work: shots from this group were looked at (editor not holding, guide pages 1 to 10).
- **Plugin check:** `./build.sh --plugins` links (no app symbols). `auval -v aufx Dfrz Dwit` and `--install-plugins` were not run: they need the component copied into the user's `~/Library/Audio/Plug-Ins`, which changes what their DAWs see; ask first. The `pluginEditor` shot is below.

**Changes while building (Group B).**
- **`fxui::Guide::Page` got `bool noClap`** (last member, so older positional initialisers still work): the guide's usual test-sound button is hidden on a page that has none. Spectral Freeze shows a button on pages 1 (*Hear a freeze*) and 4 (*Play two notes*) only.
- **`BuiltInSpectralFreeze::playTwoNotes()`** (engine, small): the page 4 sound, A3 + A#3 at -12 dBFS together for 1 s with 5 ms edges, never captured (`pairRequested`, `pairMode`).
- **`fxui::HostTrack` got `keyName`** ("A minor", empty when unknown), filled by `MainComponent`'s lookup from `Engine::getKey().getName()`, so *Your track* can say Z6 and Z7 with the key's name; a plugin in another host has none and the key sentence is left out.
- **Header:** the frame needs a "hear only" switch and a `fx::Clap`; a freeze has neither, so the editor gives it stand-ins (`SpectralFreezeStand`), hides that button, repurposes the clap button as *Test sound* (`playTestChord`), puts the status ("Not holding", "Holding . A3", "Holding . noise", "Holding . silence") where the switch was, and lays the header out again in `resized()` (the title is wider than the frame's 130 px).
- **The editor has no *Hear only* and its explainer topic for it is the title only;** nothing else of the window's content is left out.
- **Theory Guides test:** "freeze" now also finds the spectral freeze guide, so the routing guide's search check uses "freeze track" (the routing guide's own wording) instead. Added: the entry and three searches (spectral freeze, drone, FFT). The 10-topic count comes from `BuiltInGuides::getPageNames` (UI), not testable in `DawItTests`; `--export-guides` prints it.
- **Not in the plan's text:** the explainer text for each knob (what it does) is assembled from the guide's own sentences (pages 7 and 8) and the plan's tooltips; the example hold (test chord's spectrum) is drawn when nothing is held; *Your track* has no extra sentences beyond `sfz::analyse`.


**Group C (check and docs), hand-off.** `ScreenshotRunner` (`DevScreenshots.cpp`) now has a DawIt Spectral Freeze branch in the plugin-shot code: `"preset"` by name (`sfz::getPresets()`, sets the five knobs only) and `"clap": true` (`playTestChord()`; give `"wait": 1500` so the hold shows). Docs done: README bullet, tests.md, code-layout.md, architecture.md (snapshot published between threads, `HostTrack::keyName`, chip list), recipes.md (chip list, shot list). SOURCES.md already held the guide's sources.
- **Shots looked at** (scratchpad `shots/`): `held.png` (Keys, Held chord, clap, wait 1500: "Holding . C4", spectrum and level history right), `g-time.png`, `g-track.png` (Your track after a capture on Keys: mostly notes, 262 Hz C4, "C is in the song's key (A minor)"), `host.png` (pluginEditor with overlay), `breath.png` (Breath texture preset on "Lead vocal", not holding: a preset leaves Freeze alone). No text cut off or empty pictures.
- **Things found:** the tallest peak of the test chord's snapshot is 262 Hz (C4), not A3 as the plan's shot note guessed; the chord holds A3, C4 and E4, so this is right. The device bar chip in a track row is the text "FX: DawIt Spectral Freeze"; its small picture was checked in Group B only by `hasPicture`, not seen in a Group C shot.
- **Outstanding:** `auval -v aufx Dfrz Dwit` and `--install-plugins` not run (need the user's approval to copy into `~/Library/Audio/Plug-Ins`).
- **Website hand-off:** `features.html` effects group: DawIt Spectral Freeze with `held.png`'s state (Keys, preset *Held chord*, clap, wait 1500, `"capture": "window"`); `learn.html`: *What is a spectral freeze?* under Effects, the *Time or frequency* page (`g-time.png`) is the picture. No library, so no `about.html` change.

**Theory check.**
- Guide page 5 (*What a snapshot keeps*): it now also says the snapshot keeps each bin's phase, so the hold starts with a note's harmonics lined up as they were (Group A's change); it no longer says "two things".
- Guide page 9 step 3 and its block diagram: the snapshot keeps level, frequency and phase (the newest piece's), where the hold starts from.
- Mix's tooltip, its explainer and guide page 8: with nothing held, Mix leaves your track untouched but Output still sets its level (Group A made Output apply to everything).
- SOURCES.md: the Ableton Live manual's Spectral Time section (the Freezer section, Manual and Retrigger modes) backs fact 22, which used to rest on the blog only.
- Closed (was open): the flatness thresholds 0.1 and 0.3 are DawIt's own and were measured over 50 Hz to 16 kHz as a whole. Smooth spectra fit them (white about 0.9 after 4-piece averaging, pink about 0.45), but band-limited noise such as a cymbal's 2 to 8 kHz wash can score under 0.1 and then be called "mostly notes" and "Holding . <note>". Check on real breath and cymbal captures, or measure flatness per band. That's a logic change.
- Closed (was open): "0.16 s" is the 44.1 kHz figure (7,168 samples = 162.5 ms); at 48 and 96 kHz a capture spans 149 ms.


**Changes while building (after the theory check).**
- **Notes or noise:** `sfz::getSnapshotFlatness` now measures flatness over the band that holds the sound (bins whose 41-bin neighbourhood is within 20 dB of the strongest), inside 50 Hz to 16 kHz. Thresholds stay 0.1 and 0.3. Measured: test chord 0.0006, white noise above 0.6, 2 to 8 kHz noise 0.44, pink 0.51. Test added for the two band-limited/pink noises.
- **Z4 gap:** a noisy hold (flatness 0.3 or more) with Movement 20% or more now says "Your hold is mostly noise (...). At Movement N% it keeps changing, so it should sound like a soft wash rather than a tone." (new wording, for dawTheory to check). Tested.
- **"0.16 s" is now "about 0.15 s"** in all shipped text (guide, tooltips, explainer, Z2, captions, architecture.md).

**Theory check, second pass.** Both open points are closed by the builder's changes above, checked:
- Flatness over the band that holds the sound: the method is described nowhere in the guide text (only "mostly notes or mostly noise"), so no text needed changing; the band-limited and pink noise tests back it.
- The new Z4b line now reads "... At Movement N% its levels and phases keep wandering, so it moves like a soft wash instead of sounding still and mechanical." (it said "rather than a tone", but a noise hold isn't a tone at any Movement; the point, after Dudas & Lippe, is that random movement keeps a hold from sounding static). Test expectation updated.
- Page 6's loop picture now draws a 0.15 s loop and says "about 6.7 repeats a second" (it drew 0.1625 s and said 6.15, against the text's "0.15 seconds ... about 6 or 7 times a second"); page 1's hold band uses the same 0.15 s.
- *Theory* above updated to what shipped: facts 16 and 21, topic texts (0.15 s, the phase in pages 5 and 9, Output in page 8), the Freeze and Mix tooltips, the flatness band, Z2, Z4b and the flatness test numbers.


**Guide page added (user's request): "Make a pad from your sound"** (page 9 of 11, after *Fade, mix and letting go*; "How it works" is now page 10 and "Your track" page 11). A hands-on page: choose a sound, add the effect, click Freeze while it rings (not on the attack; 0.25 s after a plucked chord, as Test sound does), pick a starting point (Pad under the playing: 1.5 s, -1 dB/oct, 30%, 50%; Held chord; Breath texture), click Capture again on each chord change (crossfade over Fade; at 1.5 s that is 1.5 s of a 2.5 s bar at 96 BPM), set Mix and Brightness to sit under a vocal, then Track > Bounce in Place at Mix 100% and Cmd-drag the clip to copy it. It says plainly that this is an effect, not an instrument: no MIDI notes, the hold stays at its captured pitch, and DawIt has no sampler yet. Picture: Am F C G with a capture at each chord and the holds crossfading (shapes not to scale). No clap button. Numbers come from the plan's presets, test chord (20 dB/s) and the example song (96 BPM); no new sources. `--export-guides` now reports 11 topics.

**Theory check, page 9 (*Make a pad from your sound*).** Checked against the engine and the app: 96 BPM and Am F C G, one chord a bar (`ExampleSong.cpp`), so a bar is 2.5 s; Capture again crossfades over Fade (linear, `crossfade` steps by 1/Fade); the quoted starting points match `sfz::getPresets()`; "+ Effect" is the inspector's and device bar's button and Bounce in Place is in the Track menu; the test chord loses 20 dB a second and is captured 0.25 s in. Text changed:
- Step 5: Capture again "about 0.25 s after the new chord starts", so the 0.15 s snapshot holds the new chord, not the end of the old one (it said "just after", which would catch the old chord's tail and the new attack, against step 3's own advice).
- Step 6: Brightness is a tendency ("a darker hold tends to sit behind a vocal", as page 7 says it's a habit, not a rule), and Movement's 20% is tied to noisy holds, as in page 7 and the Z4 advice (it said Movement above 20% stops any hold sounding mechanical).
- Step 7: a bounce is an offline render with the one current snapshot, so a bounce plays only the hold you have now; for a pad that follows the chords, capture and bounce one chord at a time (Bounce in Place mutes the original track), then trim each clip to its bar. It had suggested copying one bounced clip under the other sections right after capturing on every chord.
- The topic's final text is in *Theory* as topic 9 (How it works and Your track renumbered 10 and 11). No new sources: every number comes from the app.

