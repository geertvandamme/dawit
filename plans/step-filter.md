# A step filter: make a rhythmic pattern of filter movements

Status: Released (auval not yet run; see Changes while building)
Idea: [#131 — A step filter: make a rhythmic pattern of filter movements](https://github.com/geertvandamme/dawit/issues/131)

## Goal
DawIt Step Filter is a built-in effect that gives each step of a repeating pattern its own filter cutoff, in time with the song: a held chord that pulses between dark and bright on eighth notes, a squelchy acid line, a talking band-pass, a high-pass that rises over four bars. Its guide *What is a step filter?* explains cutoff, resonance, the three filter types, steps, glide and mix, and how filtering differs from DawIt Slicer's mutes and repeats. It teaches a beginner that tone (which frequencies are heard) can make rhythm, not only level.

## Decisions for you
1. **A separate effect, or a filter lane in DawIt Slicer.** Options: (a) a separate effect, *DawIt Step Filter*, that can sit before or after Slicer; (b) a cutoff lane in Slicer beside its modes. **Recommended: (a)**: each window stays readable, the two patterns can have different lengths and step lengths, Slicer's saved parameters and its plugin code (`Dslc`) stay untouched, and both share the same step grid and clock code, so they line up when chained. A combined window is in *Left for later*.
   Answer: (a) a separate effect, DawIt Step Filter. (user, 2026-11-02)
2. **What a step stores.** Options: (a) a height from 0 to 100% between two knobs, *Low cutoff* and *High cutoff*, spaced in octaves; (b) a cutoff in Hz per step; (c) a base cutoff plus an offset per step (Steinberg's StepFilter). **Recommended: (a)**: turning one knob moves or widens the whole pattern, equal heights are equal musical intervals, and the tooltip still shows each step in Hz.
   Answer: (a) a 0–100% height between Low and High cutoff, in octaves. (user, 2026-11-02)
3. **Per-step resonance.** Options: one Resonance knob for all steps; or a second lane. **Recommended: one knob**; a resonance lane is in *Left for later*.
   Answer: One Resonance knob for all steps. (user, 2026-11-02)
4. **Glide's unit.** Options: % of a step (the same feel at any tempo) or ms. **Recommended: % of a step**, with the ms shown under the knob.
   Answer: % of a step, 2 ms minimum. (user, 2026-11-02)
5. **Level.** Options: no match level and an *Output* knob (−24 to +6 dB); or match level that evens out the pattern. **Recommended: Output, no match level**: the level change between steps is part of the effect (as with Slicer's mutes), and resonance can add up to 26 dB, which Output and the *Your track* advice deal with.
   Answer: Output knob, no match level. (user, 2026-11-02)
6. **Slope.** Options: 12 and 24 dB per octave as in DawIt Subtractive (default 24); or 12 only. **Recommended: both**, the same filter and resonance scale as Subtractive, so a setting means the same in both.
   Answer: Both 12 and 24 dB per octave (default 24). (user, 2026-11-02)
7. **While the song is stopped**: its own clock at the song's tempo, as Slicer decided; *Play a bar* starts it on step 1. **Recommended**, for consistency.
   Answer: Its own clock at the song's tempo, as Slicer. (user, 2026-11-02)

## What the user does and sees
- **Adding it:** *DawIt Step Filter* in the effect menus like any built-in; also a VST3/AU (code `Dstf`, category `Fx|Filter`).
- **The window** (an `fxui::EffectEditor`, copying DawIt Slicer's): header with starting points, **Play a bar**, **Hear only what it removes**, the guide button. Knob groups *Grid* (Step length, Steps, Tempo Song/Own with BPM), *Filter* (Type, Slope, Low cutoff, High cutoff, Resonance), *Sound* (Glide, Mix, Output). Explainer on the right.
- **Pictures:** *The pattern*: up to 16 bars over beat and bar lines, each as high as its cutoff on an octave scale from Low to High (labelled in Hz at both ends), with the cutoff's path over time drawn as a line through them (slopes where Glide slides); the playing step lit, following the clock at 30 Hz; cells past Steps dimmed. Beside it *The filter now*: the response from 20 Hz to 20 kHz at the cutoff playing now, the Low to High range shaded, a dot at the cutoff. Below, the level history (`LevelHistoryView`).
- **Interaction on the pattern:** click sets a step to the mouse's height; drag across paints several; values snap to 5%, Alt drags without snapping; double-click or Delete over a step sets it to 100%; Cmd+C / Cmd+V copy and paste the pattern (step length, Steps, 16 heights) as a line of text, as Slicer's; right-click a step: *Lowest*, *Halfway*, *Highest*, *Make every step the same as this one*, *Copy pattern*, *Paste pattern*, then *What is a step filter?...* (`guideicon::addItem`).
- **Tooltips:** texts in *Theory*; over a step: "Step 5 of 16 · beat 2, 1st sixteenth · 1.0 kHz (50%)".
- **Hear only what it removes:** plays the input minus the filtered sound (Mix 100%), so you hear the part the filter takes away. Not saved.
- **Device bar chip:** the 16 bars, the playing one lit (`chainpictures`; also in Signal Flow).
- **Remembered:** nothing outside the plugin state.

## Data and saving
All settings are plugin parameters in the processor's `AudioProcessorValueTreeState`, saved in the project's plugin state like every built-in: step length (Slicer's six choices, default 1/16), Steps (1 to 16, default 16), Tempo (Song/Own), Own BPM (20 to 300, default 120), Type (low-pass, band-pass, high-pass; default low-pass), Slope (12/24 dB, default 24), Low cutoff (20 Hz to 20 kHz, log, default 300 Hz), High cutoff (default 20 kHz), Resonance (0 to 100%, default 0), Glide (0 to 100% of a step, default 10%), Mix (0 to 100%, default 100), Output (−24 to +6 dB, default 0), and 16 step heights (0 to 100%, default 100%). No `ProjectFile` change, no format version. Test: `checkThroughProject<BuiltInStepFilter>` with a non-default pattern.

## The guide
*What is a step filter?*, pages of an `fxui::Guide` (`src/ui/StepFilterGuide`), pictures following the processor's settings and tempo, *Play a bar* as its test sound:
1. **A pattern of tone**: what it does. Picture: one bar of the test chord's harmonics (bars at 110 Hz × n) with each step's cutoff line and the harmonics it keeps lit.
2. **Cutoff and the three filters**: low-, band-, high-pass. Picture: the three responses at 1 kHz, Slope as set.
3. **Resonance**: the peak at the cutoff. Picture: low-pass at 1 kHz at 0, 25, 50, 75 and 100%, peaks labelled in dB.
4. **Steps, Low and High**: timing and heights. Picture: the pattern with each step's Hz, and ms per step at the tempo.
5. **Glide: jump or slide**: Picture: the cutoff over two steps at Glide 0, 25, 50 and 100%.
6. **Step filter, slicer and LFO**: Picture: a held chord's level per step through Slicer's *Trance gate* and through *Eighth-note pulse*, and an LFO's sine against a step pattern.
7. **Mix, level and clipping**: Picture: low-pass 24 dB at 1 kHz at Mix 100% and 50% (the dip), and the resonance peak against Output.
8. **How DawIt Step Filter works**: block diagram (clock → step height → glide → cutoff → filter → mix with dry → output).
9. **Step filters in a mix**: Picture: the starting points' patterns side by side.
10. **Your track**: Picture: the last 2 bars, per step input and output level and the cutoff, with the advice lines.

**Theory Guides entry:** `Guide::stepFilter` after `slicer`, area *Effects*; summary "Moving a filter in a rhythm: cutoff and resonance, low-, band- and high-pass, steps, glide, mix, and your track."; opens from "DawIt Step Filter's window (What is a step filter?)"; plugin "DawIt Step Filter"; words: step-filter stepfilter filter filters filtering cutoff resonance low-pass lowpass high-pass highpass band-pass bandpass sequencer filter-sequencer pattern steps glide acid wah sweep rhythmic tone. Area summary becomes "Space, echoes, colour and rhythm: reverb, delay, slicing, step filters, saturation, distortion". Not on *Start here*. Slicer's *Mute: a trance gate* page gets one closing sentence (text in *Theory*).

## Code
Follow recipes.md *A built-in effect or instrument*, copying DawIt Slicer (`BuiltInSlicer`, `SlicerEditor` with `SlicerPatternView`, `SlicerGuide`), and *A theory guide* step 5.
- **Shared filter.** Move DawIt Subtractive's 2-pole TPT state-variable filter, the resonance-to-Q scale (`sub::getQ`) and the response maths (`sub::getFilterResponse`) into a small header-only engine file both use; Subtractive's functions forward to it. The 24 dB slope stays "a flat stage, then the resonant one". Subtractive must sound the same: its existing tests pass unchanged.
- **Shared grid and clock.** Move Slicer's step-length table, step index and clock (song ppq when playing, own clock otherwise, own-tempo fallback, jump detection) into a header-only helper both use, so the two count steps identically. Slicer's tests guard the move.
- **Cutoff:** step height v → Low × (High/Low)^v, with the lower knob as Low whichever is set lower; clamped to 20 Hz … 45% of the sample rate. Step = floor(beats / step beats) mod Steps.
- **Glide:** at each step boundary the cutoff moves from where it is to the new step's value, straight in octaves, over Glide × step length, never under 2 ms (so Glide 0% doesn't click). The filter coefficient is updated every sample (or every 8 samples at most).
- **Jumps** (seek, loop wrap, start): the cutoff goes to the new step's value over the 2 ms minimum; filter states are kept (no reset, which would click).
- **Mix** linear (dry 1−m, wet m), as Slicer, then Output gain on the whole. No latency (`setLatencySamples (0)`).
- **Test sound:** the clap request plays one bar of a held A minor chord of saw waves (A2, E3, A3, C4, E4; peak about −12 dBFS) at the tempo in use, restarting the own clock at step 1.
- **History and advice:** per step (index, cutoff, input and output peak and RMS, beat) in a non-consuming atomic ring as `slc::StepHistory`; `stf::analyse` (pure, tested) gives the *Your track* lines.
- **Threading:** parameters are atomics; nothing allocates on the audio thread; the editor paints from settings, atomics and the history.
- **Risk:** the two shared-code moves touch two shipped effects; do them first, alone, with the existing tests unchanged.

## Libraries and models
None. No licence change: no `LICENSE`, `licenses/` or *License note* update.

## Theory

### Facts
1. A beat lasts 60,000 / BPM ms; at 120 BPM a 1/16 step is 125 ms, a 1/8 250 ms. Exact; Slicer and Rhythm guides.
2. An octave up doubles the frequency. Exact; the Sound guide.
3. Cutoff = Low × (High/Low)^v: Low 250 Hz, High 4 kHz is 4 octaves; 0/25/50/75/100% give 250/500/1,000/2,000/4,000 Hz. Exact, derived.
4. A filter's cutoff (corner) without resonance is where it is 3 dB down. Exact; EQ guide (*Slopes*).
5. One octave above the cutoff, a 12 dB low-pass is −12.3 dB and a 24 dB one −24.6 dB (1/√(1+2⁴), squared). Exact, derived; agrees with EQ and Subtractive guides ("about 24 dB").
6. A 2-pole low- or high-pass is Q times as loud at its cutoff; DawIt's Resonance 0 to 100% is Q 0.71 to 20 (DawIt Subtractive). 12 dB at the cutoff: 0% −3.0, 25% +4.2, 50% +11.5, 75% +18.8, 100% +26.0 dB; 24 dB: 3.0 dB less (−6.0 … +23.0). Exact, derived (20 log10 Q).
7. DawIt's band-pass peaks at 0 dB at the cutoff and is cutoff/Q wide at −3 dB: at 1 kHz and 50% (Q 3.76), 266 Hz. Exact, derived.
8. A saw's harmonics fall as 1/n (Subtractive guide); keeping only the first 4 keeps 86.5% of its power, 0.6 dB less (Σ1/n² to 4 over π²/6). Exact, derived: a low-pass changes tone far more than level.
9. A 110 Hz saw (A2) has harmonics every 110 Hz; a cutoff of 440 Hz sits on the 4th. Exact, derived.
10. Sudden cutoff jumps can click; step filters smooth them with an envelope, attack or glide: Logic Pro's Step FX (Sound On Sound, 2018), Roland SH-4d's Step Filter (Attack), Steinberg StepFilter (Glide). Typical; manufacturers.
11. Glide straight in octaves: halfway in time is the geometric mean; 250 Hz → 4 kHz passes 1 kHz at half the glide. Exact, derived.
12. At the cutoff, a 24 dB low-pass (no resonance) is −6 dB and half a cycle behind, so at Mix 50% the sum is 0.5 − 0.25 = 0.25: −12.0 dB, a dip; with 12 dB (a quarter cycle behind) −4.3 dB. Exact, derived.
13. Step filters are a known effect: Steinberg StepFilter (16 steps of cutoff and resonance, tempo-synced, glide, mix), Roland SH-4d Step Filter (16 steps, slopes −12/−24/−36 dB), Logic Pro Step FX (step modulators to a filter's cutoff, 24 dB low-pass by default). Exact (manufacturer).
14. Sample and hold on a cutoff gives random, held filter steps, a classic analogue synth sound. Typical; search results (Sweetwater, Cherry Audio).
15. French or filter house (mid-1990s Paris) runs disco loops through moving low-pass filters. Typical; Wikipedia *French house*.
16. Resonance 60–80% for acid squelch; cutoffs basses 150–500 Hz, pads 500 Hz–2 kHz. Typical; Subtractive guide.
17. Highest cutoff 45% of the sample rate: 19.8 kHz at 44.1 kHz. Exact, derived (a design limit).
18. A high-pass rising over the bars before a drop makes the drop's bass hit harder. Opinion.
19. Filter movement works best on steady, bright sounds. Opinion.
20. The *Your track* thresholds (1.5 dB, 6 dB, 30 dB, 1/12 octave). Opinion.

Exact 13, typical 4, opinion 3.

### Topic texts (name — summary — text)

**1. A pattern of tone** — *Moving a filter in a rhythm*

A step filter changes how bright a sound is, step by step, in time with the song. Each step of a repeating pattern has its own filter setting: dark on one eighth note, bright on the next. A held chord that would just sit there starts to pulse and move, while it keeps playing all the time.

A filter takes frequencies away. A low-pass, the most used, lets the low ones through and turns down the high ones above its cutoff. Every pitched sound is made of harmonics: a saw wave at A2 (110 Hz) has one at 110 Hz, 220, 330, 440 and so on up. With the cutoff at 440 Hz only the first four pass at full strength and the sound is dark and round; at 4 kHz dozens pass and it is bright and buzzy.

That's why a step filter changes tone much more than level. Most of a saw's power is in its lowest harmonics: keeping only the first four keeps 87% of it, just 0.6 dB less. You hear the change as darker or brighter, not quieter. DawIt Slicer's Mute steps work on level, leaving gaps.

Like a slicer, a step filter works on the sound itself and doesn't know where the notes are: each step starts on the grid, even in the middle of a note.

The picture shows the harmonics of the test chord, with each step's cutoff and the harmonics it lets through.

**2. Cutoff and the three filters** — *Low-pass, band-pass, high-pass*

The cutoff is where the filter starts working. A filter doesn't stop at its cutoff like a wall: it turns things down more and more past it, along a slope. With no resonance the level at the cutoff itself is 3 dB down, and one octave past it a 12 dB per octave filter is about 12 dB down, a 24 dB one about 25 dB.

- Low-pass keeps what's below the cutoff. Low steps sound dark and muffled, high steps bright. The usual choice for a step filter.
- High-pass keeps what's above. Low steps leave the sound whole, high steps thin it out to fizz. Good for drums and for build-ups.
- Band-pass keeps a band around the cutoff and takes away both sides: nasal and vocal. Moving it from step to step sounds like a voice changing its vowel, or a wah pedal played in rhythm.

Slope: 24 dB per octave is the stronger, more obvious movement; 12 dB is gentler and keeps more of the sound at every step. On a band-pass, 24 dB makes the band's sides steeper.

The same filter is in DawIt Subtractive: What is subtractive synthesis? > The filter shows it on a synth's harmonics.

The picture shows the three types with the cutoff at 1 kHz.

**3. Resonance** — *The peak at the cutoff*

Resonance boosts the frequencies right at the cutoff while those past it are still cut: the filter gets a peak. At 0% there is none; at 50% a low-pass at 12 dB per octave is 11.5 dB louder at the cutoff; at 100% 26 dB, and it nearly rings on its own, a whistle at the cutoff.

On a step filter, resonance is what makes the pattern sing. Each step moves the peak to a new frequency, so the harmonic nearest the cutoff stands out on every step and the pattern sounds almost like a melody. That is the squelchy, talking sound of acid house basslines: high resonance, usually 60 to 80%, and a cutoff that moves on every sixteenth.

Two cautions. Resonance makes the track louder, by up to 26 dB at the peak, so turn Output down as you turn Resonance up, or the next effect or the master can clip. And a peak low down, under about 150 Hz, booms on a bass.

On a band-pass, Resonance makes the band narrower instead: at 1 kHz and 50% it is 266 Hz wide.

The picture shows a low-pass at 1 kHz at 0, 25, 50, 75 and 100%.

**4. Steps, Low and High** — *When each step plays, and where its cutoff is*

Step length is a note value: at 120 BPM a 1/16 step lasts 125 ms and a 1/8 step 250 ms. Steps sets how many steps the pattern has, from 1 to 16; 16 steps of 1/16 make a bar of 4/4, 8 of 1/8 too. The pattern is counted from the start of the song, so step 1 is always on the first beat of bar 1. These work exactly as in DawIt Slicer, so the two line up when you use both.

Each step's height sets its cutoff between two knobs: Low cutoff at the bottom, High cutoff at the top. The heights are spaced in octaves, the way we hear pitch: with Low at 250 Hz and High at 4 kHz (4 octaves apart), 0% is 250 Hz, 25% 500 Hz, 50% 1 kHz, 75% 2 kHz and 100% 4 kHz. Equal steps up the bars are equal musical steps.

Turning Low and High moves the whole pattern at once. Move them apart for a bigger movement, close together for a subtle one. If they are equal, every step has the same cutoff and nothing moves.

Typical ranges: basses 150 to 500 Hz at the bottom; pads and chords from about 500 Hz to 2 to 5 kHz; drums with a high-pass from about 30 Hz to 2 kHz.

The picture shows your pattern with each step's cutoff in Hz and how long each step lasts.

**5. Glide: jump or slide** — *How the cutoff gets to the next step*

Glide sets how long the cutoff takes to move to the next step's value, as a share of a step. At 0% it jumps; at 50% it slides through the first half of the step and holds; at 100% it slides the whole step, so a pattern turns into a smooth curve.

A jump is crisp and rhythmic, but a sudden change of cutoff can make a small click, most of all with high resonance. Step filters in other programs have an attack or glide control for the same reason. DawIt Step Filter always takes at least 2 ms, so even 0% doesn't click; a little Glide, 5 to 20%, softens the edges further.

The slide is even in octaves, as we hear it: from 250 Hz to 4 kHz it passes 1 kHz halfway through the glide.

Longer glides make the filter sound like a sweep. A pattern of 16 quarter-note steps rising from 0% to 100% with Glide 100% is a smooth four-bar rise.

The picture shows the cutoff over two steps at Glide 0, 25, 50 and 100%.

**6. Step filter, slicer and LFO** — *Three ways to make a sound move*

DawIt Slicer and DawIt Step Filter both follow a pattern on the beat, but they change different things. A Mute step on the slicer takes the sound away: silence, a trance gate. A low step on the step filter takes away only the high frequencies: the sound keeps playing, darker. A trance gate chops; a step filter pulses. They work well together: Slicer first for the gaps, the step filter after it for the tone.

An LFO (low-frequency oscillator), as on DawIt Subtractive's filter, moves the cutoff along a repeating smooth wave, such as a sine. That's a wobble or a sweep, always the same shape. A step filter can make any shape: two steps high, one low, a jump at the end of the bar. Old analogue synths had a stepped version too, sample and hold, which set the cutoff to a random value on every step.

An auto-wah follows how hard the part is played rather than a pattern. Automation draws one movement across a whole song. Neither is in DawIt yet.

The picture shows a held chord's level through Slicer's Trance gate and through Eighth-note pulse, and an LFO's sine against a step pattern.

**7. Mix, level and clipping** — *What happens around the filter*

Mix blends the filtered sound with the untouched track. 100% is the filter alone. Lower values keep some of the original under the movement, but not exactly half a filter: the filtered sound lags a little behind the original near the cutoff, so the two partly cancel there. A 24 dB low-pass at Mix 50% is 12 dB down right at its cutoff, deeper than at 100% (6 dB down). It sounds like a soft notch that moves with the pattern. If that's not what you want, keep Mix at 100% and move Low and High closer together for a gentler effect.

Output sets the level after the filter, from −24 to +6 dB. Lower it when you add resonance: the peak can make the track up to 26 dB louder at the cutoff.

A low-pass that goes very low on a step, say 50 Hz on a pad, leaves almost nothing; that's a gap like a slicer's Mute, with a muffled edge.

Hear only what it removes plays the part the filter takes away, so you can hear which frequencies each step removes.

The picture shows a low-pass at 1 kHz at Mix 100% and 50%, and the resonance peak against Output.

**8. How DawIt Step Filter works** — *From the song's clock to the sound*

1. It follows the song's clock. DawIt tells every effect the tempo and where the playhead is. From that, DawIt Step Filter works out which step it's in, counted from the start of the song, so step 1 is on the first beat of bar 1 wherever you start, loop or change the tempo.
2. It finds the step's cutoff from its height between Low and High.
3. It glides there over Glide's share of the step, at least 2 ms, evenly in octaves.
4. It filters the sound with the type, slope and resonance you set, each side of a stereo track on its own, both at the same cutoff. The cutoff stays below 45% of the sample rate: 19.8 kHz at 44.1 kHz.
5. It mixes the result with the untouched track by Mix, then sets the level with Output.
6. When the song is stopped, it keeps its own clock at the song's tempo, so you hear it on Play a bar and on what you play live. Tempo Own uses the BPM you set instead. As a plugin in another program, it follows that program's tempo and position when it gives them, and its own tempo when not.
7. It holds nothing back: no latency, so the track stays in time whether it's on or bypassed.

The picture follows the sound through these parts.

**9. Step filters in a mix** — *Where it works*

A step filter works best on steady, bright sounds, where there are harmonics to take away and time to hear them change: pads, held chords, saw basses, noise, drum loops. On a sine or a soft flute there is little above the first harmonic, so a low-pass has little to do. That's an opinion with a reason behind it.

- Pads and chords: an eighth-note pulse between dark and bright, the starting point Eighth-note pulse.
- Basslines and leads: high resonance and a different cutoff on every sixteenth, the acid sound (Acid steps).
- Drum loops: a high-pass on some steps thins the loop out without leaving gaps. Producers of French and filter house in the 1990s built whole tracks on disco loops under a moving low-pass filter.
- Build-ups: a high-pass rising over the bars before a drop takes the bass away, so the drop lands harder when it comes back (Build-up rise).
- Vocals: a band-pass pattern on a copy of a vocal for a talking, radio-like effect, with the main vocal left clear.

How to use it:
- Put it on one part, not the whole mix, so the rest keeps the song steady.
- DawIt can't yet switch an effect on at a point in the song. To filter one section, put that clip on a track of its own with the step filter.
- Watch the level when you add resonance; Your track says when it gets loud.

The picture shows the starting points' patterns side by side.

**10. Your track** — *What the pattern does to this track*

This topic looks at the last 2 bars of the track DawIt Step Filter is on, step by step: each step's cutoff and how loud the track came in and went out. It says whether the pattern is heard, whether resonance makes it too loud and whether a step leaves almost nothing.

Play the track, or click Play a bar, and the picture follows.

### The window's texts (describe / typical / tooltip)
- **Step length** — "How long each step lasts, as a note value of the song's tempo, 1/32 to 1/4 with triplets. At 120 BPM a 1/16 step is 125 ms." Typical: "1/16 for acid and fast pulses, 1/8 for chords and pads, 1/4 for slow movements and rises." Tooltip: "How long a step lasts, as a note value".
- **Steps** — "How many steps the pattern has before it starts again, counted from the start of the song. 16 steps of 1/16 make one bar." Typical: "16 for a bar of 1/16 or two of 1/8; 8 for a bar of 1/8." Tooltip: "How many steps before the pattern repeats".
- **Tempo** — as DawIt Slicer's. Tooltip: "Follow the song's tempo, or a tempo of its own".
- **Type** — "Low-pass keeps the lows: low steps are dark. High-pass keeps the highs: high steps are thin. Band-pass keeps a band around the cutoff: nasal and vocal." Typical: "Low-pass nearly always; high-pass for drums and rises; band-pass for talking effects." Tooltip: "Which frequencies the filter keeps".
- **Slope** — "How steeply the filter turns things down past the cutoff: 12 or 24 dB per octave. 24 makes a stronger movement." Typical: "24 dB for basses, acid and obvious pulses; 12 dB for pads and gentle movement." Tooltip: "12 or 24 dB per octave".
- **Low cutoff** — "The cutoff of a step at the bottom (0%). Turn it to move the low end of the whole pattern." Typical: "Basses 150-500 Hz, pads 400-800 Hz, high-pass rises 30 Hz." Tooltip: "The cutoff of the lowest steps".
- **High cutoff** — "The cutoff of a step at the top (100%). Steps in between are spaced evenly in octaves." Typical: "2-5 kHz for an obvious movement; 20 kHz for steps that open fully." Tooltip: "The cutoff of the highest steps".
- **Resonance** — "A peak at the cutoff. It makes each step's cutoff sing; at 50% a 12 dB low-pass is 11.5 dB louder at the cutoff, at 100% 26 dB." Typical: "0-20% for natural movement, 30-50% for character, 60-80% for acid. Lower Output as you raise it." Tooltip: "A peak at the cutoff: makes the steps sing".
- **Glide** — "How long the cutoff takes to reach each step's value, as a share of the step (ms below). 0% jumps (still over 2 ms, so it doesn't click); 100% slides the whole step." Typical: "0-10% for crisp steps, 20-50% for a softer pulse, 100% for sweeps." Tooltip: "Jump or slide between steps".
- **Mix** — "How much of what you hear is the filtered sound. Below 100% the filtered sound partly cancels the original near the cutoff, a moving dip." Typical: "100% nearly always." Tooltip: "The filtered sound against the untouched track".
- **Output** — "The level after the filter, from −24 to +6 dB. Turn it down when resonance makes the track louder." Typical: "0 dB without resonance; −6 to −12 dB with 50% or more." Tooltip: "The level after the filter".
- **The pattern** — "Each bar is a step; its height sets its cutoff between Low and High. Click or drag to set steps, Alt for exact values, double-click for the top. The lit step is playing now." Tooltip per step as in *What the user does and sees*.
- **The filter now** — "The filter at the cutoff playing now: what passes (above the 0 dB line is boosted by resonance). The shaded band is Low to High."
- **Play a bar** — "Plays one bar of a held A minor chord of saw waves through the filter, from step 1, at the tempo it uses. It works while the song is stopped." Tooltip: "Play a held chord through the step filter".
- **Hear only what it removes** — "Plays what the filter takes away, so you hear which frequencies each step removes."
- **Overview** — "DawIt Step Filter gives each step of a pattern its own filter cutoff, in time with the song, so a steady sound moves in tone: dark and bright, thin and full."
- **Slicer guide addition** (end of *Mute: a trance gate*): "To make a held sound pulse in tone instead of in level, use DawIt Step Filter: see What is a step filter?"

### Starting points (name — settings — why)
Heights are listed step 1 to 16; Mix 100%, Output 0 dB unless given.
1. **Neutral: every step open** — 1/16, 16, low-pass 24, Low 300 Hz, High 20 kHz, Res 0, Glide 10%, all 100%. "Every step is open, so you hear the track almost unchanged. Drag a step down to hear it darken."
2. **Eighth-note pulse** — 1/8, 8, low-pass 24, Low 400 Hz, High 4 kHz, Res 20%, Glide 15%, 0, 100, 0, 100, 0, 100, 0, 100. "Low and high cutoffs take turns on eighth notes, so a held chord pulses between dark and bright while it keeps sounding. A little Resonance makes the bright steps shine; Glide 15% takes the edge off each change."
3. **Acid steps** — 1/16, 16, low-pass 24, Low 200 Hz, High 3.2 kHz, Res 65%, Glide 5%, Output −9 dB, 0, 50, 25, 75, 0, 100, 25, 50, 0, 75, 25, 100, 0, 50, 75, 25. "High resonance makes each sixteenth's cutoff sing like a note: the squelch of an acid bassline. Best on a saw bass or lead. Output −9 dB makes up for the resonance peak."
4. **Talking band-pass** — 1/8, 8, band-pass 12, Low 400 Hz, High 2.5 kHz, Res 40%, Glide 50%, 0, 60, 30, 100, 0, 60, 30, 100. "A band moving between 400 Hz and 2.5 kHz sounds like a voice changing vowels. Glide 50% makes it talk rather than click. Try it on a copy of a vocal or a guitar."
5. **Build-up rise** — 1/4, 16, high-pass 12, Low 30 Hz, High 2 kHz, Res 25%, Glide 100%, heights rising evenly 0, 6.7, 13.3 … 100. "Over four bars the high-pass climbs from 30 Hz to 2 kHz and the bass drains out, so the next section lands harder. The pattern restarts every four bars from bar 1, so start the build-up on bar 1, 5, 9 and so on."
6. **Soft pad movement** — 1/8, 8, low-pass 12, Low 600 Hz, High 3 kHz, Res 15%, Glide 60%, 100, 40, 70, 20, 100, 50, 80, 30. "A gentle, breathing change of tone under a pad. 12 dB keeps it open; long Glide makes the steps flow into each other."

### Example data for the pictures
- Test chord: saws at 110, 164.8, 220, 261.6, 329.6 Hz; harmonics n × each up to 20 kHz, amplitude 1/n.
- Resonance page: 12 dB low-pass at 1 kHz, peaks −3.0, +4.2, +11.5, +18.8, +26.0 dB.
- Glide page: 1/16 at 120 BPM, 250 Hz → 4 kHz; Glide 50% reaches 4 kHz at 62.5 ms, passes 1 kHz at 31.25 ms.
- Mix page: 24 dB low-pass at 1 kHz: −6.0 dB at Mix 100%, −12.0 dB at 50%.

### "Your track" rules (last 2 bars of step records; F0 ends, the rest list in order)
- **F0, nothing heard:** every input peak below −60 dBFS → "Play the track, or click Play a bar, to see it here."
- **F1, no song tempo** (Song source, host gave no BPM) → "No song tempo reaches DawIt Step Filter here, so it steps at its own 120 BPM. If the track has a tempo, choose Own and set it, or the steps will drift against the beat."
- **F2, nothing moves:** every used step's cutoff within 1/12 octave of the others → "Every step has the same cutoff (1.2 kHz), so the tone doesn't move. Drag some steps up or down, or turn Low cutoff and High cutoff apart."
- **F3, hard to hear** (not F2): the steps' output-minus-input level spans less than 1.5 dB → "The steps change this track's level by at most 1.1 dB, so the pattern may be hard to hear. The track may have little sound between 300 Hz and 5.0 kHz: try a lower Low cutoff (low-pass) or a higher High cutoff (high-pass), or add Resonance."
- **F4, louder:** output peak more than 6 dB above input peak → "At its loudest the filter makes the track 9 dB louder than it came in, from the resonance peak. Turn Output down by about 9 dB, or lower Resonance, so nothing after it clips."
- **F5, almost silent step:** a step's output RMS 30 dB or more below its input → "Step 3 takes the track down by 34 dB, almost to silence. That works as a gate-like pulse; for a softer one, raise that step or Low cutoff."
- **F6, summary**, always after F0: "The last 2 bars: 16 steps of 1/16 (125 ms at 120 BPM), a low-pass moving between 300 Hz and 5.0 kHz; the steps change the level by 2 to 14 dB."

### Test numbers
| Checked | Expected | Tolerance |
|---|---|---|
| Q at 0 / 50 / 100% | 0.71 / 3.76 / 20.0 | ±0.01 |
| 12 dB LP at cutoff, Res 0 / 50 / 100% | −3.0 / +11.5 / +26.0 dB | ±0.1 |
| 24 dB LP at cutoff, Res 0 | −6.0 dB | ±0.1 |
| LP 12 / 24 one octave above | −12.3 / −24.6 dB | ±0.1 |
| BP at cutoff; BP −3 dB width at 1 kHz, 50% | 0.0 dB; 266 Hz | ±0.1 dB; ±3% |
| Heights 0–100%, Low 250, High 4000 (and swapped) | 250, 500, 1000, 2000, 4000 Hz | ±0.5% |
| Glide 50%, 1/16, 120 BPM: cutoff at 31.25 ms / 62.5 ms | 1000 / 4000 Hz | ±2% / ±1 block |
| Glide 0%: change complete | within 2 ms | +1 block |
| Measured 1 kHz sine, LP 12 at 1 kHz; HP 12 at 1 kHz on 250 Hz; at 44.1, 48, 96 kHz | as the response maths (−3.0; −24.1 dB) | ±0.5 dB |
| Mix 50%, LP 24 at 1 kHz, 1 kHz sine; LP 12 | −12.0 / −4.3 dB | ±0.3 |
| Mix 0% | output = input | 1e-6 |
| Output −6 dB | ×0.501 | ±0.5% |
| Step at ppq 9.0, 1/16, 16 steps | step 5 (index 4) | exact |
| Cutoff clamp at 44.1 kHz | ≤ 19,845 Hz | exact |
| Saw first 4 harmonics' power | 86.5% (−0.63 dB) | ±0.1% |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. Move Subtractive's state-variable filter, resonance scale and response maths into a shared header-only engine file; Subtractive forwards to it. Tests: Q and response numbers from the table; all Subtractive tests unchanged. Done when: tests pass, Subtractive's sound unchanged.
- [x] 2. Move Slicer's step-length table, step index and clock into a shared header-only helper; Slicer uses it. Tests: Slicer's tests unchanged; the ppq 9.0 step check. Done when: tests pass.
- [x] 3. `src/engine/BuiltInStepFilter.{h,cpp}` (`stf::`): parameters, cutoff mapping, glide, filter, mix, Output, test chord, presets, pattern text, step history; register with `dawit_add_builtin` (`Dstf`, `Fx|Filter`, engine sources plus the two shared headers) and in `BuiltInFormat.cpp`. Tests: mapping, glide, measured responses at three rates, stability, clicks, silence, stereo, mix, output, bypass, presets, pattern text, save/load.
- [x] 4. `stf::analyse` (F0 to F6). Tests: each rule on made-up records, thresholds both sides. `./build.sh --test`. Done when: all checks pass, no warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 5. `src/ui/StepFilterEditor.{h,cpp}`: the knob groups, the pattern view (copy `SlicerPatternView`'s painting, menu and copy/paste), *The filter now*, the level history, texts and starting points; editor factory set in `MainComponent`; the device bar chip picture. Check: open on Keys, paint steps, play.
- [x] 6. `src/ui/StepFilterGuide.{h,cpp}`: the ten pages with pictures and Your track (`fxui::getHostTrack`), `setPrintSource`; `BuiltInGuides` branches; `gen::guides` entry, area summary, `MainComponent::openGuide`; the Slicer guide sentence. Tests: Theory Guides entry, topics, search words.
- [x] 7. Plugin check: `./build.sh --plugins`, `auval -v aufx Dstf Dwit`, a `pluginEditor` shot. `./build.sh --test`. Done when: all checks pass, no warnings, `--export-guides` passes.

Group C, check and docs:
- [x] 8. Real-app check: add a `preset` branch for DawIt Step Filter in `ScreenshotRunner` (as Slicer's). Shots: Keys with `addPlugins: ["DawIt Step Filter"]`, preset *Eighth-note pulse*, `play: true` at beat 12 (lit step, response moving); Bass with *Acid steps*; the guide at *Resonance* and *Your track* after playing; the device bar chip; `pluginEditor` with overlay. Look for cut text, wrong Hz, empty pictures.
- [x] 9. Docs: README Features bullet (effects); docs/tests.md and docs/code-layout.md lines (also the shared filter and clock files); architecture.md *Built-in effects* (shared filter and step clock; Step Filter follows the play head and has a chip picture) and recipes.md's chip list; SOURCES.md checked; issue #131 progress.

## Tests
- Shared filter: the Q, response, octave and band-pass numbers in the table.
- Subtractive and Slicer: existing checks pass unchanged after the moves.
- Mapping and glide: the table's values; Low above High gives the same cutoffs.
- Measured: sine levels through each type at 44.1, 48 and 96 kHz against the response maths (±0.5 dB).
- Stability: Resonance 100%, cutoff alternating 20 Hz and the clamp on every 1/32, 10 s of white noise at each rate: all samples finite, peak below +40 dBFS.
- Clicks: 110 Hz sine, Glide 0%, low-pass 12, steps 0/100% (Low 200 Hz, High 5 kHz): no sample-to-sample change across a step boundary more than twice the steady output's largest one.
- Silence: zero in, zero out (< 1e-9); after loud input then 1 s of silence, output below 1e-6.
- Stereo: left-only input leaves the right output silent; equal inputs give equal outputs.
- Clock: step at ppq 9.0; after a seek, a loop wrap and a tempo change; own clock when stopped; own BPM when the host gives none.
- Mix, Output, bypass (input unchanged), latency 0.
- Presets: six, each with a why, values in range; Eighth-note pulse alternates 0/100.
- Pattern text round trip; bad text leaves the settings untouched.
- Analysis: F0 to F6; F3 at 1.1 dB fires, 2 dB doesn't; F4 at 9 dB fires, 5 doesn't; F5 at 34 dB fires, 25 doesn't; F2 at 1/24 octave fires, 1 octave doesn't.
- Save and load: `checkThroughProject<BuiltInStepFilter>` with a non-default pattern.
- Theory Guides: entry with its 10 pages; searching "step filter" and "filter sequencer" finds it; "cutoff" finds it and Subtractive.

## Website hand-off
- `features.html`, the effects group, beside DawIt Slicer: a pattern of filter cutoffs on the beat.
- `learn.html`: a card for *What is a step filter?* in Effects.
- Screenshot: the window on Keys with *Eighth-note pulse* while playing (lit step, response). No `about.html` change.

## Left for later
- A filter lane inside Slicer, or one window for both (decision 1).
- A per-step resonance lane, per-step glide or tied steps (Steinberg and Logic offer them).
- More than 16 steps, swing, a 36 dB slope, a Randomise button (as #126 asks for Slicer).
- Restarting the pattern on a MIDI note, and stereo offsets between left and right.
- Starting the effect at a point in the song, which needs automation (#87).

## Changes while building

**Group A, engine: built.** `./build.sh --test` ends with All checks passed, no warnings. Hand-off for Groups B and C:
- **Shared filter:** `src/engine/StateVariableFilter.h`, namespace `daw::filt`: `FilterType`, `flatQ`, `getQ`, `getStage`, `getFilterResponse`, `getFilterDb` and the `Svf` struct. `sub::FilterType` is now an alias of `filt::FilterType`; `sub::getQ`, `sub::getFilterResponse` and `sub::getFilterDb` forward to it. Inside namespace `sub`, call `sub::getFilterResponse` qualified (ADL on `filt::FilterType` makes the bare call ambiguous).
- **Shared grid and clock:** `src/engine/StepGrid.h`, namespace `daw::stepgrid`: `stepLengths`, `getStepBeats`, `getStepMs`, `getStepIndex`, `clampBpm`, and `Clock` (`begin`, `restartOwn`, `commit`, `finish`). `slc::` and `stf::` re-export the names with `using`, so Slicer's code and tests are unchanged.
- **Engine:** `src/engine/BuiltInStepFilter.{h,cpp}`: `daw::BuiltInStepFilter` (`identifier` "DawIt Step Filter", code `Dstf`, parameter ids `step steps tempo ownbpm type slope low high resonance glide mix output height1..height16`), `createEditorFunction`, `getSettings` / `setSettings` / `setKnob (stf::Knob, value)` / `setHeight (step, percent)` / `getHeight`, `getKnobParameter`, `getHeightParameter`. Atomics for the window: `playingStep`, `clockBeat`, `usingSongClock`, `currentCutoffHz` (the cutoff at the end of the last block, glide included), `tempoBpm`, `getUsedBpm()`, `listenToEffect`. `clap.request()` plays the test chord (the clap's request flag only). `history` is an `stf::StepHistory` (`getRecent`, `clear`) of `stf::StepRecord` (step, cutoffHz = where the step's cutoff was heading, in and out peak and RMS, beat). `levels` is an `fx::LevelHistory` (per 64 samples: input peak dB, and the peak of what the filter changed, output minus input) for `LevelHistoryView`.
- **`stf::` helpers for the window and guide:** `getHeightCutoffHz`, `getStepCutoffHz`, `clampCutoff (hz, rate)`, `getGlideMs`, `formatHz`, `formatKnob`, `formatKnobValue`, `patternToText` / `patternFromText`, `getPresets()` (six, the plan's names and why texts), `getTypeName`, `analyse (AnalysisInput)` (F0 to F6).
- **Registered:** `dawit_add_builtin (StepFilter ...)` in `CMakeLists.txt` with `ENGINE_SOURCES` only; Group B adds `UI_SOURCES src/ui/StepFilterEditor.cpp src/ui/StepFilterGuide.cpp`. The editor name `BuiltInStepFilterEditor` / `ui/StepFilterEditor.h` is already recorded for the plugin build. `BuiltInFormat.cpp` creates it and lists it after Slicer. Set `BuiltInStepFilter::createEditorFunction` in `MainComponent`.
- **Group B must remove a temporary exemption** in `tests/EngineTests.cpp` (marked `TEMPORARY (step filter, Group A)`, in the Theory Guides hub checks): the two "every built-in has one guide in the hub" checks skip DawIt Step Filter until its `gen::guides` entry exists.
- Tests are in `tests/StepFilterTests.h` (`runSharedFilterAndGridTests`, `runBuiltInStepFilterTests`, `runStepFilterAnalysisTests`, called from `main`).

Departures from the plan:
- **Clicks test threshold is 3x, not 2x.** With Glide 0% (the 2 ms minimum), going from the low step (200 Hz) to the high one (5 kHz) makes the output catch up with the input: the largest sample-to-sample change is 0.0172 against 0.0072 for the steady sine (2.4x; a plain Python model of the same filter gives the same). That is 3% of the peak per sample, not a click you can hear, and an instant jump would be worse; the test checks 3x. If dawTheory or the user wants 2x, the minimum glide would need to be longer.
- **F6's level clause** ("the steps change the level by 2 to 14 dB") uses the smallest and largest per-step level change (output RMS over input RMS, in dB, magnitude) over the steps that had sound; F3's "at most N dB" is the spread of those changes. The plan didn't define either more exactly.
- **F2 and F6 take their cutoffs from the settings** (the steps in use), not from the records.
- The test chord is band-limited saws (polyBLEP) at 0.05 each, so its peak is at most 0.25 (about -12 dBFS) with a 5 ms fade at each end.
- Cutoff limits: the 19,845 Hz test uses a 0.0001 tolerance because 0.45 x 44100 is not exact in floating point.

**Group B, UI: built.** `./build.sh --test` ends with All checks passed, no compiler warnings; `./build.sh --plugins` builds the VST3/AU; `--export-guides` passes (What is a step filter.pdf written). The Group A temporary exemption in `tests/EngineTests.cpp` is removed. Hand-off for Group C:
- **Files:** `src/ui/StepFilterEditor.{h,cpp}` (`BuiltInStepFilterEditor`, `StepFilterPatternView`, `StepFilterResponseView`) and `src/ui/StepFilterGuide.{h,cpp}` (`stftext::`: `getTitle`, `describe`, `getTypical`, `getTooltip`, `describeSettings`, `drawPattern`, `drawFilterNow`, `drawLastBars`, `getLastBars`, `makeGuide`, `colour`). Both are in `UI_SOURCES` of `dawit_add_builtin (StepFilter ...)`; the editor factory is set in `MainComponent`.
- **Wiring done:** `Guide::stepFilter` after `slicer` in `TheoryGuides.{h,cpp}` (entry, words, area summary "... delay, slicing, step filters, saturation, distortion"); `G::stepFilter` in the switches of `MainComponent` (2), `ArrangementView` (2) and `DevScreenshots` (guide key "stepFilter"); `BuiltInGuides::makePages` branch in `TheoryGuidesWindow.cpp`; the Slicer guide's *Mute: a trance gate* page has the closing sentence; the device bar chip and Signal Flow picture (`ChainPictures`, `DeviceBar`: bars as high as each step, the playing one white, repaint every timer tick).
- **Tests added** (EngineTests, Theory Guides block): entry in Effects after the slicer, "step filter", "filter sequencer", "acid" find it first, "cutoff" finds it and Subtractive, area summary.
- **Left for Group C:** the `preset` (and `clap`) branch for `BuiltInStepFilter` in `ScreenshotRunner` (`DevScreenshots.cpp`, next to Slicer's) and its line in `DevScreenshots.h`; the real-app shots; docs (README, tests.md, code-layout.md, architecture.md, recipes.md chip list: now Slicer and Step Filter). `auval -v aufx Dstf Dwit` was not run: the AU is only in the build folder, and registering it needs `./build.sh --install-plugins`, which changes what the user's DAWs see (ask first). The `pluginEditor` scene shots of the window and the guide overlay (pages 1, 3, 5 to 9) were looked at and show the editor and pictures correctly.
- **Window:** 1400 x 640 (twelve controls in one row); pictures: pattern (left), filter now (right, 30%), `LevelHistoryView` below it (the legend reads "the track" and "what the filter changed").

Departures from the plan (Group B):
- The guide's picture for page 6 draws the Trance gate lane as an every-other-1/8-step Mute pattern rather than reading Slicer's preset, and the Eighth-note pulse lane from the step filter's own preset with the chord's power through the filter (level nearly unchanged, cutoff alternating).
- Pictures with a fixed example (Glide, Resonance, Mix, the three filters) use the plan's example numbers; those that follow the settings (harmonics, steps and Hz, Your track, the resonance peaks and slope) use the processor's.

**Group C, check and docs: built.** `./build.sh --test` ends with All checks passed (see the report). Hand-off:
- Added a `preset` / `clap` branch for DawIt Step Filter in `ScreenshotRunner` (`src/ui/DevScreenshots.cpp`). Shot list used (scale 1, `"wait"` is in ms): Keys + `addPlugins` + preset *Eighth-note pulse*, `clap`, `play`, beat 12, wait 3000, window; Bass + *Acid steps*; guide `stepFilter` at *Resonance*, *Mix, level and clipping*, *Your track* (after playing, `track: Keys`); `deviceBar` chip; `pluginEditor` scene `builtIn: "DawIt Step Filter"` with `overlay`.
- Found and fixed: on the guide's Resonance page the percentage labels sat on the curves; they are now a column at the top right. The Mix and Output page is clean.
- Docs: README bullet, docs/tests.md, code-layout.md, architecture.md, recipes.md (chip list, preset shots). SOURCES.md already had the Step filter section from planning; unchanged.
- Website hand-off: see *Website hand-off* above. Best screenshot: Keys with *Eighth-note pulse* playing (window, 3 s of play, beat 12); the chip picture is a nice detail for the effects group.

**Theory check (dawTheory).** Derived numbers re-computed (Q, peaks, slopes, band-pass width, saw power, Mix 50% dips, glide, clamp, chord notes): all as shipped. `./build.sh --test` ends with All checks passed, no warnings. Changed:
- Guide page 1: a 440 Hz cutoff no longer passes "the first four at full strength"; the fourth sits at the cutoff (3 or 6 dB down) and the fifth is still partly heard.
- Guide page 3: the band-pass is 266 Hz wide at 1 kHz and 50% "at 12 dB per octave" (257 Hz at 24 dB).
- Guide page 5: the 2 ms minimum "keeps even 0% from clicking", and with high resonance a jump can still start with a short ping (modelled: 6x the steady change at Resonance 65%).
- Guide page 7: the filtered sound lags "by half a cycle at the cutoff of a 24 dB low-pass", not "a little".
- Guide page 6 picture: the Trance gate lane now mutes every other 1/16 step (Slicer's preset), not every other 1/8; label to match.
- Advice F3 (engine and test): "The steps differ in what they do to this track's level by at most N dB", since N is the spread between steps, not the change itself.
- Advice F5: "raise that step or Low cutoff on a low-pass, or lower that step or High cutoff on a high-pass" (raising helps only on a low-pass).
- Verdicts: the 3x clicks threshold is kept and the 2 ms minimum stays (a model of the test gives 17x with no minimum, 3.5x at 1 ms, 2.4x at 2 ms, so 3x still catches a missing minimum); F2/F6 from the settings and the F3/F6 level definitions are sound; the polyBLEP chord at 0.05 each matches "about -12 dBFS" (0.25 is the worst case).
- Open: the Resonance knob's band-pass line (`describeSettings`) gives cutoff/Q for both slopes, 3% too wide at 24 dB and more at low resonance (code, not a literal); F6 takes magnitudes, so a +3 dB step and a -3 dB step both read 3 dB.

**Fixes after the theory check (done).** (1) The Resonance knob's band-pass line now uses the new `filt::getBandWidthHz (steep, cutoff, resonance)` (the -3 dB points of the shared response, either slope): 266 Hz at 12 dB and 257 Hz at 24 dB for 1 kHz at 50%; tested. (2) F6 keeps the direction of each step's level change and reads "from -14 to -2 dB" (with a minus sign), or "by -3 dB" when the rounded ends agree; tests for a +3/-3 mix and equal steps. `./build.sh --test`: All checks passed, no warnings. `auval` was skipped by the user and stays open.
