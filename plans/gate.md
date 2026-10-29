# DawIt Gate

Status: Built, not committed; Theory checked, with open points
Idea: [Built-in effects and synths that teach while you use them](../FEATURE_IDEAS.html#built-in-effects-and-synths-that-teach-while-you-use-them) (the "Later: gate" bullet)

## Goal
A built-in noise gate and downward expander that opens for the notes and closes in the gaps, to remove hiss, hum and the
bleed of other drums, or to tighten drums and make gated reverb. Like DawIt Compressor it explains every knob, shows what it
does (the curve with its hysteresis loop, the last 4 seconds with when it was open, the key filters) and comes with
*What is a gate?*, a guide that also looks at the user's own track and suggests a threshold.

## Decisions for you
1. **Gate and expander in one effect?** Options: a pure gate; a gate with a Ratio knob whose far end is *Gate*; two effects.
   **Recommended:** one effect, Ratio 1:1 to 10:1 with *Gate* at the end, because a gate is an expander with an endless
   ratio, and a beginner learns that by turning one knob (FabFilter Pro-G works the same way).
   Answer: One effect, Ratio 1:1 to 10:1 with *Gate* at the end (recommendation accepted).
2. **Lookahead?** Options: none (no delay); 1-10 ms lookahead with reported latency. **Recommended:** none for now: attack
   goes down to 0.01 ms, the guide says what lookahead is, and lookahead would delay the track (it needs the engine's
   latency compensation checked first).
   Answer: None for now; noted under *Left for later* (recommendation accepted).
3. **Where in the Theory Guides window?** Options: *Mixing* (next to compression); *Effects*. **Recommended:** Mixing, as a
   dynamics tool beside the compressor; not on *Start here*.
   Answer: Mixing, next to compression; not on *Start here* (recommendation accepted).
4. **What attack, hold and release mean.** Makers differ (Logic and FabFilter: release = time to close fully; FabFilter: hold
   counts from opening). **Recommended:** attack and release as time constants (two thirds of the way, as DawIt Compressor
   says), hold counted from when the level falls under the closing level (Logic, Sound On Sound). The text says makers differ.
   Answer: As recommended: time constants, hold counted from falling under the closing level (recommendation taken).
5. **Name:** "DawIt Gate" or "DawIt Gate/Expander". **Recommended:** DawIt Gate (the identifier is saved in projects; short).
   Answer: DawIt Gate (recommendation taken).

## What the user does and sees
- Added like any built-in effect: the plugin chain menu (from `BuiltInFormat::getEffects()`, listed right after DawIt
  Compressor), saved and loaded like a plugin. Double-click its chip in the device bar to open the window.
- **Window** (1400 x 780, on `fxui::EffectEditor`, two knob rows like DawIt Subtractive):
  ```
  [DawIt Gate | Starting points v | What is a gate? | Hits and hiss | Hear what it removes | ?]
  [ curve (square)            | last 4 seconds                        ] [ explainer ]
  [  OPEN / CLOSED -80 dB     | key filters (2/5 wide) |              ]
  [ WHEN: Threshold Hysteresis | HOW FAR: Range Ratio | HOW FAST: Attack Hold Release ]
  [ KEY: Low cut High cut Key listen | LEVEL: Match level ]
  ```
- **Curve** (`GateCurveView`): level in across, out up, -80 to 0 dB; the open line (diagonal), the closed line (range or
  ratio), dashed opening level (threshold) and closing level (threshold minus hysteresis) with arrows showing the loop,
  the track's dot with a fading trail (newest moment), and an OPEN / CLOSED light with the current reduction. Drag the
  threshold dot and the closing-level dot sideways, the range dot (left end of the closed line) up and down; Alt drags in
  0.1 dB steps (otherwise 1 dB); scroll changes the ratio; double-click a dot resets it. Right-click menu: *Set the threshold
  from the track (-49 dB)* (from `gate::analyse`, greyed with "play the track first" when there's no suggestion),
  *Reset all*, then *What is a gate?* (opens at *Threshold*). Tooltips on each dot: "Threshold -40 dB: drag sideways",
  "Closes at -44 dB: drag to set the hysteresis", "Range -80 dB: drag up or down".
- **Last 4 seconds** (`GateHistoryView`, from `getRecent()`): in, out, reduction, threshold and closing lines, an open bar,
  beat lines. Tooltip under the mouse: "-2.4 s · in -45 dB · closed, -80 dB".
- **Key filters** (`KeyFilterView`): the key response, 20 Hz-20 kHz, labelled *kick and bass*, *snare body*, *cymbals and
  hi-hat*. Drag the two handles; double-click turns one off; right-click: *Key listen*, *Turn the key filters off*,
  *What is a gate?* (at *The key filter*).
- Knobs are `fxui::EffectKnob`s, with the explainer on hover as in the other effect windows. *Key listen* and *Match level*
  are `EffectEditor::Switch`es. *Hits and hiss* plays the new test sound; *Hear what it removes* plays input minus output.
- **Device bar:** a tiny curve (-80 to 0 dB) with the track's dot and a match-level badge, as DawIt Compressor's chip.
- Nothing new in `engine.properties`; key listen and hear-what-it-removes are not saved.

## Data and saving
Only the plugin's parameters, through `getStateInformation` like DawIt Compressor; `ProjectFile.cpp` is unchanged. Older
projects have no gate. Test: `checkThroughProject<BuiltInGate>` changes threshold, ratio, key low cut and match level, saves,
loads and compares.

## The guide
*What is a gate?* (`gatetext::makeGuide`, an `fxui::Guide`, like *What is compression?*). Pictures follow the window's
settings through `gate::simulate`, the same code the processor runs. Topics (text in *Theory*):

| # | Name | Summary | Picture |
|---|---|---|---|
| 1 | Closing the gaps | What a gate does | `makeDrumHits` in (grey) and out (colour), open bar, threshold line |
| 2 | Threshold: when it opens | Above the noise, below the notes | the same hits with the window's threshold and closing line, the hiss level and the hits' peak labelled |
| 3 | Hysteresis: open high, close low | Why a gate chatters, and the cure | `makeFadingNote` twice: top with hysteresis 0 and hold 0 (open bar flickers), bottom with the window's hysteresis; closes counted on each |
| 4 | Range: how far it closes | Silence, or just quieter | the curve for ranges 10, 20, 40, 80 dB (grey) and the window's |
| 5 | Gate or expander: the ratio | Cut the gaps, or turn them down gently | the curve for 2:1, 4:1 and Gate (grey) and the window's |
| 6 | Attack: how fast it opens | Keeping the first click | `makeHitStart`: the gain over the first 20 ms for 0.05, 1 and 5 ms, and the window's |
| 7 | Hold and release: how it closes | The end of every note | `makeTomHit` out with release 30 ms and 400 ms (grey) and the window's; hold marked |
| 8 | The key filter: what it listens to | Not letting the kick open the snare | the key response with the window's filters and the three labelled ranges |
| 9 | Where gates are used | Drums, guitars, voices, and when not to | four small plots: gain after a hit for the Kick, Toms, Lead vocal and Gated reverb starting points |
| 10 | Gated reverb | The big drum sound of the 1980s | `makeReverbSnare` in (grey) and out through the Gated reverb starting point |
| 11 | Too much | Chopped, choppy, clicking | `makeTomHit` through the window's settings and with the threshold 15 dB higher (ending gone) |
| 12 | How DawIt Gate works | From key to gain | the signal chain as boxes: key filters, detector, open/closed, how far, smoothing, gain, match |
| 13 | Your track | What the gate makes of your track | the gate's last 4 seconds (`getRecent`): level, floor and loud lines, threshold and closing line, open bar, and `analyse`'s advice lines drawn under it |

- Opens from the window's *What is a gate?* button, the curve's and key view's right-click menus, and the Theory Guides window.
- **`gen::guides` entry:** `Guide::gate` after `compressor`; area *Mixing*; name "What is a gate?"; summary "Closing the gaps
  between notes: threshold and hysteresis, range and ratio, attack, hold and release, the key filter, and gated reverb.";
  opens from "DawIt Gate's window (What is a gate?)"; plugin "DawIt Gate"; keywords "gate gating noise-gate noise hiss hum
  bleed spill expander expansion downward chatter hysteresis hold key sidechain kick snare toms drums gated reverb";
  topics come from the plugin (empty list, as for the other built-ins). Not on *Start here*. Mixing's area summary becomes
  "Levels, EQ, compression, gates and buses: making the parts fit".
- PDF export comes with `fxui::Guide::setPrintSource`; `BuiltInGuides` makes stand-ins and pages.

## Code
**New `src/engine/BuiltInGate.{h,cpp}`** (namespace `daw::gate`, then `daw::BuiltInGate`; reuse `comp::getCoefficient`,
`eq::makeStages`/`eq::getResponseDb` for the 12 dB/oct key filters, as DawIt Compressor's sidechain filter does):
- `enum class Knob { threshold, hysteresis, range, ratio, attack, hold, release, keyLow, keyHigh }`, `numKnobs = 9`;
  `Settings` (defaults -40 dB, 4 dB, 80 dB, 10 = Gate, 0.5 ms, 20 ms, 100 ms, 20 Hz = off, 20 kHz = off); ranges:
  threshold -80..0, hysteresis 0..12, range 0..80 (stored positive, shown "-80 dB"), ratio 1..10 log, attack 0.01..50 ms log,
  hold 0..1000 ms (skewed), release 5..2000 ms log, key low cut 20..4000 Hz log, key high cut 200..20000 Hz log.
  Constants `detectorMs = 10`, `flickerMs = 20`, `momentMs = 2`, `recentSeconds = 4`.
- `isGate (ratio)` (>= 9.95), `isKeyLowOn`, `isKeyHighOn`.
- `getReductionDb (Settings, levelDb, bool open)`: 0 when open; closed: the range for a gate, else
  `min (range, (threshold - level) * (ratio - 1))`, never below 0.
- `Detector` (key peak: instant rise, falls with `detectorMs` time constant), `OpenState` (opens at level >= threshold;
  reloads the hold while level >= threshold - hysteresis; closes when the hold has run out; counts closes and flickers,
  a reopen within `flickerMs` of a close), `Smoother` (linear gain one-pole: attack coefficient while rising, release while
  falling; starts at the range's gain, closed).
- `simulate (Settings, levelsDb, rate) -> Run { gainDb, open, closes, flickers }`: the same three structs on a level curve.
- Examples at `exampleRate = 10000` (values in *Theory*): `makeDrumHits (seconds)`, `makeFadingNote()`, `makeHitStart()`,
  `makeTomHit()`, `makeReverbSnare()`.
- `formatKnob (Knob, Settings)`, as the knobs show them (see *Test numbers*).
- `Preset { name, why, followTrack, settings, notes[numKnobs] }`, `getPresets()` (eight, in *Theory*).
- `Moment { inputDb, outputDb, detectorDb, reductionDb, open, flickers }`; `Analysis { heard, silentGaps, floorDb, loudDb,
  flickers, suggestedDb (optional), advice }`; `analyse (moments, settings, matchGainDb)` (rules in *Theory*).

**`BuiltInGate`** (`juce::AudioPluginInstance`, identifier "DawIt Gate", category "Dynamics", uid "DGt1" `0x44477431`),
following `BuiltInCompressor`'s shape: parameters in an APVTS (ids `threshold hysteresis range ratio attack hold release
keyLow keyHigh match`), `getSettings`/`setSettings`/`setKnob`, `applyPreset (Preset, std::optional<float> trackThreshold)`
(uses the track's threshold when `followTrack`), match level (the compressor's half-second power method), `listenToRemoved`
and `listenToKey` atomics, `tempoBpm`, `fx::Clap clap`, `takePreview()` for the device bar, and
`std::vector<gate::Moment> getRecent() const`: the last 4 s, one moment per 2 ms, from a ring of atomics with an atomic
write position, so the window, the guide and the analysis can all read it (a torn moment is harmless). Stereo channels share
one key and one gain. No latency, no tail.

**`src/engine/EffectTools.h`:** `fx::Clap::Sound::hitsInHiss`: the clap at 0, 250, 500 and 750 ms over white noise of peak
0.003 (-50.5 dB) for 1.0 s.

**New `src/ui/GateGuide.{h,cpp}`** (namespace `daw::gatetext`, like `disttext`): `enum Topic { curve = 50, key, match,
keyListen }`, `colour` (teal `0xff2ec4b6`), `getTitle`, `describe`, `getTypical`, `describeSettings (topic, Settings, recent
moments, bpm)`, drawing helpers `drawCurve`, `drawLevels`, `drawKeyFilters`, and `makeGuide (BuiltInGate&)`.

**New `src/ui/GateEditor.{h,cpp}`:** `GateCurveView`, `GateHistoryView`, `KeyFilterView` and `BuiltInGateEditor final :
fxui::EffectEditor` (`buildText`, `layoutPictures`, `applyPreset` with the track's suggested threshold, `createGuidePages`,
`tick`), as `BuiltInDistortionEditor`.

**Changes:** `BuiltInFormat.{h,cpp}` (register, after the compressor), `Engine.cpp` comment, `EffectWindow.cpp` (button text
"Hits and hiss", width 110, for the new sound), `MainComponent.cpp` (`createEditorFunction`, `openGuide` and
`getPrintGuide` cases), `ArrangementView.cpp` (both switches), `TheoryGuides.{h,cpp}`, `TheoryGuidesWindow.cpp`
(`makePages`), `DeviceBar.cpp` (`getGate`, `canMatch`/`isMatching`/`setMatching`, `hasPicture`, `updatePicture`,
`updateSignal` via `takePreview`, `paintPicture` branch on the -80..0 dB curve), `CMakeLists.txt` (engine file in both
targets, UI files in `DawIt`). Mentions of a gate in `DistortionGuide.cpp` and `ReverbGuide.cpp` (texts in *Theory*).
Threading: parameters and atomics only; the audio thread never locks.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | A gate passes a sound unchanged above a threshold and turns it down (or off) below it. | [Wikipedia: Noise gate](https://en.wikipedia.org/wiki/Noise_gate); [SOS](https://www.soundonsound.com/techniques/advanced-gating-techniques-part-1) | exact |
| 2 | A downward expander turns down below the threshold by a ratio: at 2:1, 10 dB under comes out 20 dB under (10 dB of reduction); reduction = (threshold - level) x (ratio - 1). At 4:1: 30 dB. | [ProSoundWeb](https://www.prosoundweb.com/signal-processing-fundamentals-dynamic-controllers/2/); derived | exact |
| 3 | A gate is an expander with a very high ratio; above about 5:1 an expander acts more and more like a gate. | [FabFilter Pro-G help](https://www.fabfilter.com/help/pro-g/using/dynamiccontrols) | typical |
| 4 | Range (floor) is the most it turns down; full range mutes. | SOS; FabFilter | exact |
| 5 | Hysteresis: opens at the threshold, closes a few dB lower; stops chatter. | SOS; [Logic Pro Noise Gate](https://support.apple.com/guide/logicpro-ipad/lpipbafe55dc/ipados) | exact (definition), typical ("a few dB") |
| 6 | Chatter is rapid opening and closing near the threshold; hold and hysteresis cure it. | SOS | exact |
| 7 | Gates without a hold knob often have a fixed hold of about 20-30 ms, against triggering on single cycles of low notes. | SOS | typical |
| 8 | The fastest gate attacks are a few tens of microseconds, for percussive sounds; slow-starting sounds suit 10 ms or more. | SOS | typical |
| 9 | Makers define attack, hold and release differently (FabFilter and Logic: release = time to reach full reduction; FabFilter: hold = minimum time fully open). | [FabFilter time controls](https://www.fabfilter.com/help/pro-g/using/timecontrols); Logic | exact |
| 10 | Ultra-fast gain changes can distort or click. | FabFilter (lookahead text); SOS | typical |
| 11 | DawIt's attack: after one attack time 63% open (1 - 1/e), after three 95%. | derived | exact |
| 12 | Closing: the gain falls 20 log10 e = 8.69 dB per release time while far above the range: 8.7 dB at 1x, 26.1 dB at 3x, 43.4 dB at 5x; 20 dB at 2.30x (ln 10), 40 dB at 4.61x. | derived | exact |
| 13 | 5 ms attack: first millisecond at 20 log10 (1 - e^-0.2) = -14.8 dB. | derived | exact |
| 14 | Detector falls 8.69 dB per 10 ms; -20 to -40 dB takes 10 ln 10 = 23.0 ms; -30 to -40 dB 11.5 ms. | derived | exact |
| 15 | Key filters are 12 dB/oct Butterworth: low cut 150 Hz at 60 Hz = -16.0 dB; low cut 200 Hz at 50 Hz = -24.1 dB; high cut 2 kHz at 8 kHz = -24.1 dB; high cut 150 Hz at 200 Hz = -6.2 dB; low cut 1 kHz at 100 Hz = -40.0 dB. | derived (`eq::getResponseDb`, flatQ 0.7071) | exact |
| 16 | A low cut on a snare gate's key stops the kick opening it; a high cut stops hi-hat spill; key listen lets you hear the key. | SOS; Logic; [Shure](https://www.shure.com/en-US/insights/recording-drums-part-3-gates) | typical |
| 17 | Heavy high cut on the key can make the gate open slightly later. | SOS | typical |
| 18 | Kick weight around 60 Hz, snare body around 200 Hz, cymbals 6-10 kHz. | DawIt's EQ guide (`eq::getRegions`, presets) | typical |
| 19 | -20 dB is a tenth of the level, and sounds about a quarter as loud (10 dB per doubling of loudness). | DawIt's sound guide | typical |
| 20 | Noise 40 dB under the notes adds 10 log10 (1 + 10^-4) = 0.0004 dB of power. | derived | exact |
| 21 | Gated reverb: found around 1979 by Steve Lillywhite and Hugh Padgham on Peter Gabriel's 1980 album, with Phil Collins on drums; best known from Collins' "In the Air Tonight". One method: room mics through a gate opened by the close mics, hold about half a second, fast release. | [Wikipedia: Gated reverb](https://en.wikipedia.org/wiki/Gated_reverb) | exact (history), typical (hold) |
| 22 | Overheads and room mics are usually left ungated; toms are among the drums gated most often. | [mastering.com](https://mastering.com/noise-gate/); Wikipedia: Noise gate; Shure | opinion |
| 23 | Some engineers set hold and release in time with the song; at 120 BPM a sixteenth is 125 ms, a beat 500 ms. | [Nail The Mix](https://nailthemix.com/sam-pura-tempo-synced-kick-snare-gates); derived | opinion; exact (numbers) |
| 24 | 2:1 to 4:1 with 10-20 dB of range on vocals and speech; a full close sounds less natural in a mix than 10-20 dB. | common practice, no single source | opinion |
| 25 | A gate before or after distortion: before follows the playing, after also catches the distortion's hiss; both are used. | agrees with DawIt's distortion guide ("before or after") | opinion |
| 26 | A gate usually goes first in a chain, before compression (which brings the noise up). | common practice; derived (makeup raises the floor) | opinion |
| 27 | Software instruments are silent between notes (DawIt's synths output digital silence). | DawIt | exact (for DawIt's synths) |
| 28 | Threshold about 6 dB above the noise floor; under 12 dB between noise and notes, a gate separates them poorly. | DawIt's rule of thumb | opinion |

The guides agree: the distortion guide says "before or after the distortion" in *Sustain and noise* but "before it" in the
drive text and *Distortion in a mix*; step 8 makes those two say "before or after". The compressor guide's time-constant
wording (two thirds) is reused.

### Topic texts
**1. Closing the gaps** — *What a gate does*
A gate lets a sound through while it's loud and shuts while it's quiet, like a door that opens when someone knocks hard enough. Between the notes the door is closed, and whatever quietly goes on there is gone.

What goes on between the notes is usually something you didn't want: the hiss of an amp, the hum of a guitar's pickups, a fridge in the next room, or the other drums leaking into a drum's microphone (called bleed or spill). Under the notes you don't hear it, because the notes are much louder. In the gaps you do. A gate removes it there.

A gate can also shape a sound on purpose: it can make a drum shorter and tighter, or cut a big reverb off abruptly, the gated drum sound of the 1980s.

What a gate can't do is take noise out from under the notes: while it's open, the hiss plays along. That needs a noise reduction tool, which DawIt doesn't have yet.

The picture shows made-up drum hits over steady hiss at -55 dB, through the gate with the settings in its window: in grey what comes in, in colour what comes out, and along the bottom when the gate is open. The hits pass; the hiss between them is gone.

**2. Threshold: when it opens** — *Above the noise, below the notes*
The threshold is the level the sound must reach to open the gate. Anything louder passes. When the sound falls back under it, the gate closes (a little lower, with hysteresis: see the next topic).

Set it between two levels: above the loudest noise, so the noise alone never opens it, and below the quietest note you want to keep, so every note does. In the picture the hiss sits at -55 dB and the hits start at -6 dB: anything from about -50 to -20 dB works, and -40 dB leaves room on both sides.

The threshold depends on how loud the track is, so set it by ear and by the picture, not by a number. A good way: start with it at the bottom, play a part with gaps, and raise it until the gaps are quiet. Then check that the softest notes still come through whole. The Your track topic suggests a threshold about 6 dB above your track's noise.

When the noise and the quietest notes are close together, less than about 12 dB apart, no threshold separates them well. Then a gentle expander works better than a gate (see Gate or expander).

The picture shows the hits with the window's threshold and closing level: the gate opens where a hit crosses the threshold.

**3. Hysteresis: open high, close low** — *Why a gate chatters, and the cure*
A note doesn't fade smoothly: its level wobbles a little as it dies away. If the gate opened and closed at exactly the same level, a note fading past the threshold would cross it several times, and the gate would flick open and shut. You hear it as a buzz or a stutter, called chatter.

Hysteresis is the cure: the gate opens at the threshold but closes only once the sound has fallen a few dB under it. With a threshold of -40 dB and 4 dB of hysteresis, it opens at -40 dB and closes at -44 dB. A wobble smaller than 4 dB can't open it again.

A thermostat works the same way: it might switch the heating on at 19 degrees and off at 21, instead of on and off around 20 all day.

A few dB is usual; DawIt starts at 4 dB. More hysteresis also keeps the gate open longer as a note dies away, a little like a longer hold. Hold is the other cure for chatter (see Hold and release).

The picture shows a note fading from -20 dB to -60 dB over two seconds with a slight wobble, through a threshold of -40 dB. Top: no hysteresis and no hold, and the gate closes, opens again and closes. Bottom: the window's hysteresis; with 4 dB or more, it closes once.

**4. Range: how far it closes** — *Silence, or just quieter*
The range is how much the gate turns the sound down while it's closed. At -80 dB the gaps are as good as silent. At -20 dB they're at a tenth of their level, which sounds about a quarter as loud: faint, but there. At 0 dB the gate does nothing.

A full close sounds clean on its own, but in a mix it can sound unnatural: the room and the bleed vanish between hits and come back with every note, so you hear the background switch on and off. A smaller range turns the gaps down far enough that they stop being a problem, while the sound still has air around it. Many engineers use 10-20 dB on vocals and on drums with a lot of room sound, and the full range for the hiss between riffs on a distorted guitar.

The picture shows what comes out for each level that goes in, with ranges of 10, 20, 40 and 80 dB in grey and the window's in colour.

**5. Gate or expander: the ratio** — *Cut the gaps, or turn them down gently*
A gate is open or closed. An expander is gentler: under the threshold it turns the sound down more the quieter it gets, by a ratio.

At 2:1, every dB the sound falls under the threshold becomes 2 dB in the output. A sound 10 dB under the threshold comes out 20 dB under it: turned down by 10 dB. At 4:1 the same sound is turned down by 30 dB. The range is still the most it turns down.

It's the mirror image of a compressor, which turns down what's above its threshold. That's why it's also called a downward expander: it makes the difference between loud and quiet bigger, and pushes the quiet parts further down.

Why use it: quiet sounds near the threshold, like a breath, a soft word or the end of a note, are turned down a little instead of cut off. So an expander sounds natural where a gate would chop. 2:1 to 4:1 is a common choice on vocals and speech. Above about 5:1 an expander already behaves much like a gate.

In DawIt Gate the Ratio knob runs from 1:1 (nothing happens) to 10:1, and its far end is Gate: closed means turned down by the whole range at once.

The picture shows what comes out for each level that goes in, from -80 to 0 dB, for 2:1, 4:1 and a gate, and the window's settings in colour. Above the threshold every line follows the diagonal: nothing changes there.

**6. Attack: how fast it opens** — *Keeping the first click*
When the sound crosses the threshold the gate opens, but not all at once. The attack time is how fast: after the attack time it's about two thirds of the way open (63%), and after three times as long 95%.

For drums the attack must be fast, because the most important part of a hit is its first few milliseconds: the click of a kick's beater, the crack of a snare. A gate with a slow attack is still opening while they pass. With a 5 ms attack, the first millisecond of a hit comes out about 15 dB quieter than it went in, and the hit loses its edge. The fastest gates open in a few hundredths of a millisecond for this reason.

Faster isn't always better: a gate that jumps open in the middle of a wave can click, mostly on low notes. If you hear a click at the start of every note, try 0.5-2 ms. Sounds that start softly, like a voice, strings or a pad, are fine with 1-10 ms or more.

Makers measure attack in different ways, so the same number can behave differently in another gate.

The picture shows the start of a hit and how far the gate is open over the first 20 ms, with attacks of 0.05, 1 and 5 ms and the window's in colour.

**7. Hold and release: how it closes** — *The end of every note*
When the sound falls under the closing level, two times decide what happens.

Hold keeps the gate fully open a little longer. Low notes need it: a bass note or a floor tom swings slowly, and between the peaks of one wave the level can dip under the threshold. Without hold, the gate could try to close inside a single note. Many gates without a hold knob have a fixed hold of about 20-30 ms for this reason. DawIt starts at 20 ms.

Release is how fast the gate then closes. After the release time it has closed about two thirds of the way; with a large range, the sound falls about 8.7 dB every release time. With a 100 ms release the tail is 8.7 dB down after 100 ms, 26 dB after 300 ms and 43 dB after half a second: it fades rather than stops.

Release shapes the end of every note. A short release (20-80 ms) makes drums short and tight. A longer one (200-500 ms) lets toms ring out. Too short, and notes are chopped off, with the reverb on them; too long, and the noise fades out in every gap instead of disappearing.

Some engineers set hold and release in time with the song, so every hit lasts a note value: at 120 BPM a sixteenth note is 125 ms.

Makers define these differently too: in some gates release is the time to close completely, and hold counts from the moment the gate opens.

The picture shows one tom hit dying away over bleed at -50 dB, through a short release (30 ms), a long one (400 ms) and the window's settings in colour.

**8. The key filter: what it listens to** — *Not letting the kick open the snare*
The gate decides when to open by listening to a copy of the sound, called the key or sidechain. That copy can be filtered without changing the sound itself.

Why: a snare microphone also picks up the kick and the hi-hat. The kick is often loud enough in it to open the gate, so the snare's gate opens on every kick, bleed and all. The kick's weight is around 60 Hz and the snare's body around 200 Hz, so a low cut on the key at about 150 Hz makes the kick count far less: at 60 Hz it's 16 dB quieter to the gate. A high cut does the same for the hi-hat and cymbals, which are mostly above 6 kHz.

It works the other way round too: a high cut on the key of a kick drum's gate, at about 150 Hz, keeps the snare from opening it.

Key listen lets you hear what the gate listens to, so you can set the filters by ear: you should hear the drum you want, with the others small.

Don't overdo it. The sharp start of a hit is mostly highs, so cutting the highs out of the key makes the gate open a little later.

DawIt Gate listens to its own track. Opening it from another track, so a kick opens the gate on a bass, isn't possible yet.

The picture shows what the gate hears of each frequency with the window's key filters, 12 dB per octave each.

**9. Where gates are used** — *Drums, guitars, voices, and when not to*
- Drums with a microphone each: toms especially, because they play least and pick up the whole kit the rest of the time; kick and snare often too. The overhead and room microphones usually stay ungated, because they hold the kit together.
- Distorted guitars: high gain turns up the hiss and hum between riffs. A gate before the distortion follows the playing; after it, it also catches the hiss the distortion adds. Many players use one in each place.
- Vocals and speech: an expander (2:1 to 4:1, a range of 10-20 dB) turns down the room, headphone spill and noise between phrases. Leave the breaths audible: a vocal without any sounds edited.
- Bass and keyboards with hum: a gentle expander between the notes.
- Effects: gated reverb (see that topic).

When not to:
- Tracks from software instruments, such as DawIt's own synths, are already silent between the notes: there's nothing to remove, and a gate would only cut the notes' tails.
- Sounds that fade slowly, like piano and pads: the gate cuts the fade. Use an expander, or edit the gaps.
- Noise under the notes: a gate only helps in the gaps.

In the chain a gate usually goes first: before compression, whose makeup turns the quiet parts up and so brings the noise closer to the notes, and before delay and reverb. The exception is gated reverb, where the gate comes after the reverb.

The picture shows how four starting points close after a hit: Kick drum, Toms, Lead vocal and Gated reverb.

**10. Gated reverb** — *The big drum sound of the 1980s*
Gated reverb is a big reverb or room sound that stops abruptly instead of fading: a snare that sounds huge for a moment and then is gone, leaving space before the next hit.

It was found around 1979 by producer Steve Lillywhite and engineer Hugh Padgham, recording Phil Collins' drums for Peter Gabriel's third album (1980). It became the sound of Collins' own 'In the Air Tonight' and of much of the 1980s. One way studios made it: the room microphones went through a gate that the close microphones opened, held for about half a second and then closed fast.

To make it in DawIt: put DawIt Reverb on the drum track with a large room or hall and plenty of reverb, and DawIt Gate after it with the Gated reverb starting point. Its threshold sits high, just under the hits, so the gate starts to close soon after each hit. The hold (350 ms) sets how long the reverb lasts, and a very short release (20 ms) cuts it off. At 120 BPM a beat is 500 ms, so the tail is gone before the next beat.

Because DawIt Gate listens to its own track, the length also depends on how loud the reverb is: raise the threshold for a shorter sound.

The picture shows a snare hit with a long reverb tail, and what's left after the gate.

**11. Too much** — *Chopped, choppy, clicking*
Signs a gate is doing harm, and what to try:
- Notes cut short: the ends of notes, cymbal tails or the reverb vanish. Lower the threshold, or lengthen the hold or the release.
- Quiet notes missing: ghost notes on a snare, soft words. Lower the threshold, or use an expander so they're turned down rather than lost.
- Chatter: a buzz as the gate flickers at the end of notes. More hysteresis or hold.
- Clicks at the start of notes: the attack is too fast for the sound. Try 0.5-2 ms.
- The background switching on and off: the room or the noise comes back with every note. A smaller range (10-20 dB) usually sounds more natural than silence.
- Breaths gone from a vocal: it sounds edited. An expander, or a lower threshold.

Match level helps you check. A gate that only removes noise hardly changes how loud a track is: noise 40 dB under the notes adds only 0.0004 dB. If match level has to add more than about 1 dB, the gate is taking away parts of the notes.

The picture shows a tom hit through the window's settings, and with the threshold 15 dB higher: its ending is gone.

**12. How DawIt Gate works** — *From key to gain*
For every sample:
1. The key: a copy of the sound through the key filters (a low cut and a high cut, 12 dB per octave), with both channels together, so the stereo image doesn't shift.
2. The detector follows the key's peaks. It rises at once and falls back by 8.7 dB every 10 ms, so the moments a wave passes through zero don't count as silence.
3. Open or closed: the gate opens when the detector reaches the threshold. It closes when the detector has stayed under the threshold minus the hysteresis for longer than the hold.
4. How far: open, nothing is turned down. Closed, the sound is turned down by the whole range (Gate), or by the ratio for how far it is under the threshold, at most the range.
5. Smoothing: the gain moves there with the attack time while opening and the release time while closing; after one of them, it's two thirds of the way.
6. The sound is turned by that gain, and match level makes it as loud as it came in.

DawIt Gate has no lookahead: it reacts to what is playing now, so it adds no delay. A gate with lookahead listens a few milliseconds ahead and opens before a hit arrives, at the cost of delaying the sound by as much.

Hear what it removes plays only what the gate takes away. You should hear the noise and bleed in the gaps; if you hear the starts or ends of notes, the gate is cutting into them.

The picture shows these steps as a chain.

**13. Your track** — *What the gate makes of your track*
This topic looks at the last 4 seconds of the track DawIt Gate is on, as the gate heard them, and says what to try.

It measures two levels: the noise floor, the level the track stays under for its quietest tenth of the time (the gaps), and the loud parts, the level it reaches in its loudest twentieth. A threshold about 6 dB above the floor keeps the noise out and lets the notes through, as long as the notes are far enough above it.

The picture shows the track's level in grey with the floor and the loud parts marked, the threshold and the closing level, when the gate was open, and what that suggests.

Play the track, or click Hits and hiss, and the picture follows.

### Explainer texts (knobs, pictures, common topics)
Per knob: title / what it does / when and how it's set / live lead.
- **Threshold: when it opens** / The level the sound must reach to open the gate. Louder passes; when it falls back under the threshold minus the hysteresis, the gate closes. Drag the threshold dot on the curve sideways. / Above the loudest noise, below the quietest note you want to keep. Start low, play a part with gaps and raise it until the gaps are quiet, then check the soft notes still come through. Right-click the curve to set it from the track. / "Opens at {T} dB, closes at {T-H} dB. The track reached {peak} dB in the last 4 seconds."
- **Hysteresis: open high, close low** / How far under the threshold the sound must fall before the gate closes. It stops the gate flickering open and shut (chatter) when a fading note wobbles around the threshold. / A few dB: 3-6 dB suits most sounds. More if the gate chatters at the ends of notes; 0-2 dB for gated reverb, where it should close exactly. / "Opens at {T} dB, closes at {T-H} dB."
- **Range: how far it closes** / How much the gate turns the sound down while it's closed. -80 dB is as good as silence, -20 dB leaves the gaps faint, 0 dB does nothing. / The full range (-60 to -80 dB) for hiss between riffs and for gated reverb. 10-20 dB on vocals and on drums with room sound, so the background is turned down rather than switched off. / "Closed, it turns the sound down by {R} dB, to {100 x 10^(-R/20)}% of its level."
- **Ratio: gate or expander** / At its far end, Gate: closed means turned down by the whole range at once. Below that it's an expander: under the threshold, every dB the sound falls becomes more in the output, so quiet sounds are turned down gently instead of cut. / Gate for drums, distorted guitars and effects. 2:1 to 4:1 for vocals, speech and anything with soft endings. / "A sound 10 dB under the threshold is turned down by {min (R, 10 x (ratio - 1))} dB." (Gate: "by the whole range, {R} dB.")
- **Attack: how fast it opens** / How fast the gate opens once the sound crosses the threshold: after the attack time it's two thirds open, after three times as long 95%. / 0.01-0.1 ms for drums, so the first click gets through. 0.5-2 ms if a fast attack clicks on low notes. 1-10 ms for voices, strings and pads. / "The first millisecond of a hit comes out {x} dB quieter." with x = -20 log10 (1 - (1 - g) e^(-1/attackMs)), g the range's gain; "comes through whole" when x < 0.1.
- **Hold: open a little longer** / How long the gate stays fully open after the sound falls under the closing level, before it starts to close. It keeps the gate from closing between the waves of a low note, and lets a tail ring a moment longer. / 10-50 ms for most sounds, 50-150 ms for toms, 250-500 ms for gated reverb, where hold sets the length. / "{hold} ms. At {bpm} BPM a sixteenth note is {15000/bpm} ms." (tempo part only when known)
- **Release: how fast it closes** / How fast the gate closes after the hold. After the release time it's two thirds closed: the sound falls about 8.7 dB every release time, so it fades instead of stopping. / 20-80 ms for tight kicks and snares, 200-500 ms for toms and ringing sounds, 100-300 ms for vocals, 10-30 ms to cut gated reverb off. / "After the hold, 20 dB down in {2.30 x release} ms and 40 dB down in {4.61 x release} ms." (when R < 20: "it reaches the range, {R} dB down, ...": skip the numbers past R)
- **Key low cut: ignore the lows** / Takes the lows out of what the gate listens to (the key), 12 dB per octave, without changing the sound. Off at its lowest. / About 150 Hz on a snare, so the kick doesn't open it; 80-100 Hz on vocals, against rumble and plosives. Off on kick, bass and toms. / "A 60 Hz kick counts {x} dB quieter to the gate." ("Off: it hears all the lows.")
- **Key high cut: ignore the highs** / Takes the highs out of what the gate listens to, 12 dB per octave. Off at its highest. It makes the gate open a little later, so set it no lower than needed. / About 150 Hz on a kick, so the snare doesn't open it; 2-5 kHz on snare and toms, against hi-hat and cymbals. Off on most other sounds. / "An 8 kHz hi-hat counts {x} dB quieter to the gate."
- **curve: The curve** / Level in across, out up, from -80 to 0 dB. Above the threshold the line follows the diagonal: nothing changes. The lower line is the gate closed (the range, or the ratio). The dashed lines are where it opens (the threshold) and where it closes (minus the hysteresis): it jumps up at one and down at the other. The dot is the track as it plays. Drag the dots, scroll for the ratio, double-click a dot to reset it, right-click for more.
- **history (common): The last 4 seconds** / The level coming in (grey), what comes out (colour), how much it turned down (red, from the top), the threshold and closing level, and a bar along the bottom while the gate is open. Lead: "Open {p}% of the time; {n} flickers." or "Play the track, or click Hits and hiss, to see it here."
- **key: The key filters** / What the gate hears of each frequency. Drag the left handle for the low cut and the right one for the high cut; double-click one to turn it off.
- **match: Match level** / Keeps the output as loud as the input. A gate that only removes noise hardly changes the loudness, so match level adds less than 1 dB; when it adds more, the gate is cutting into the notes. Lead as in DawIt Distortion's.
- **keyListen: Key listen** / Plays what the gate listens to, through the key filters, instead of the track. Set the filters so you hear the drum you want with the others small. Not saved with the song.
- **listen (common): Hear what it removes** / Plays only what the gate takes away. You should hear the noise and bleed in the gaps; if you hear the starts or ends of notes, the gate is cutting into them.
- **clap (common): Hits and hiss** / Four claps a quarter of a second apart over steady hiss at about -50 dB, through the gate with the window's settings, even while the song is stopped. With the threshold at -40 dB the hiss between the claps disappears.
- **overview:** lead "DawIt Gate turns a sound down while it's quiet: it opens for the notes and closes in the gaps, to remove hiss, hum and the bleed of other instruments, or to make drums tighter." When to use it: "On drum microphones (toms, kick, snare), distorted guitars, and voices with noise between phrases; and for gated reverb. Not on software instruments, which are already silent between notes." How it works: "The key filters decide what it listens to; threshold and hysteresis when it opens and closes; range and ratio how far; attack, hold and release how fast." How it's usually set: "Attack as fast as the sound allows, hold 20-50 ms, release by ear to the end of the notes, threshold above the noise. First in the chain, before compression." Reading the window: "Left, the curve with the track's dot and the open light. Right, the last 4 seconds and the key filters. Hover anything to read about it here." Getting started: "Pick a starting point; each tells why every knob is set the way it is. Click Hits and hiss to hear the gate at work, even while the song is stopped. Read What is a gate? for how it works and when it's used."

### Starting points
Settings as threshold, hysteresis, range, ratio, attack, hold, release, key low, key high; *follow* = threshold set from the
track when `analyse` has a suggestion. Notes in knob order.
1. **Neutral start** (-40, 4, 80, Gate, 0.5, 20, 100, off, off; no follow). Why: "A plain gate, middle of the road: a place to start turning knobs and hearing what each does." Notes: "-40 dB: above typical hiss, below most notes. Raise it until the gaps are quiet." / "It opens at -40 dB and closes at -44 dB, so fading notes don't make it flicker." / "-80 dB: closed is as good as silent." / "Gate: open or closed, nothing in between." / "0.5 ms: fast enough for most sounds, without clicks." / "20 ms, like the fixed hold many gates have: enough to ride over the waves of low notes." / "100 ms: the tail fades over a few hundred milliseconds instead of stopping." / "Off: it listens to the whole sound." / "Off."
2. **Kick drum** (-30, 4, 40, Gate, 0.02, 30, 80, off, 150; follow). Why: "A close kick microphone also hears the snare, the toms and the cymbals. The gate opens only for the kick, so the rest of the kit leaves the kick's track, and the kick sounds shorter and tighter." Notes: "Set from the track once it has played: just above the bleed between the kicks." / "4 dB: a clean close as the kick dies away." / "-40 dB: the bleed is gone in the mix, without the hard silence of a full close." / "Gate: a kick should be there or not." / "0.02 ms: open before the beater's click passes; the click is what makes a kick heard on small speakers." / "30 ms: rides over the slow waves of the kick's body, around 60 Hz." / "80 ms: tight. Longer for a boomy kick that should ring." / "Off: the kick's lows are what should open it." / "150 Hz: the snare's body (around 200 Hz) counts 6 dB less, its crack and the cymbals far less, so they don't open it."
3. **Snare drum** (-30, 4, 30, Gate, 0.02, 40, 150, 150, 5000; follow). Why: "A snare microphone sits right next to the hi-hat and hears the kick too. The gate keeps the snare and drops most of the rest, so the snare's EQ and compression work on the snare alone." Notes: "Just above the loudest bleed (usually the hi-hat), below the softest hits you want. Ghost notes are quiet: check they still come through." / "4 dB." / "-30 dB: turned down, not silent. The overheads fill the gaps with the same kit, so a full close isn't needed." / "Gate." / "0.02 ms: the crack is in the first milliseconds." / "40 ms." / "150 ms: lets the snare's ring and wires fade. Shorter for a dry, tight snare." / "150 Hz: the kick (around 60 Hz) counts 16 dB less, so it doesn't open the snare's gate." / "5 kHz: the hi-hat and cymbals, mostly above 6 kHz, count less."
4. **Toms** (-30, 6, 30, Gate, 0.05, 100, 350, off, 2000; follow). Why: "Toms play only now and then, and their microphones hear the whole kit the rest of the time. Gating them is one of the most common uses of a gate on drums: it takes a lot of bleed and ringing out of the mix." Notes: "Above the kit's bleed, below the softest tom hit." / "6 dB: toms ring long and unevenly; more hysteresis keeps the gate from flickering at the end." / "-30 dB." / "Gate." / "0.05 ms." / "100 ms: a low tom swings slowly." / "350 ms: lets the tom ring out. Too short and it sounds choked." / "Off: a floor tom's lows are what should open it." / "2 kHz: cymbals count much less; at 8 kHz, 24 dB less."
5. **Distorted guitar** (-50, 6, 80, Gate, 0.5, 10, 60, off, off; follow). Why: "High gain turns up the hiss, hum and string noise between riffs as much as the notes. A gate shuts it off the moment you stop playing, which tight riffs need. Put it before DawIt Distortion to follow the playing, or after it to catch the hiss it adds." Notes: "Above the hiss, set while nothing plays; check the quietest palm mutes still open it." / "6 dB: distorted notes hold and then drop fast, with noise riding on them; a wide gap closes cleanly." / "-80 dB: silent between riffs." / "Gate: stops should be stops." / "0.5 ms: fast, without a click." / "10 ms: short, so the gate keeps up with fast riffs." / "60 ms: closes quickly after a stop, the way a player mutes the strings." / "Off." / "Off."
6. **Lead vocal** (-45, 3, 12, 2:1, 2, 50, 200, 100, off; follow). Why: "Between phrases, a vocal microphone hears the room, the headphones and the singer moving. An expander turns that down by up to 12 dB, gently enough that breaths and soft endings stay natural." Notes: "Just under the quietest words, above the room and headphone spill." / "3 dB." / "-12 dB: the gaps quieter, not silent; a vocal with no air around it sounds edited." / "2:1: sounds near the threshold are turned down only a little, so soft endings fade instead of stopping." / "2 ms: a voice starts softly; slower than for drums avoids clicks." / "50 ms: rides over short pauses inside words." / "200 ms: words end naturally." / "100 Hz: rumble and the thump of p and b don't open it." / "Off."
7. **Spoken voice** (-50, 3, 15, 3:1, 1, 100, 250, 100, off; follow). Why: "A podcast or voice-over is all voice and pauses, and in the pauses the room, a computer fan or the street come forward. A firmer expander turns them down by up to 15 dB." Notes: "Above the room noise in the pauses, below the quietest words." / "3 dB." / "-15 dB: the pauses clearly quieter, still not dead." / "3:1: firmer than for singing, still not a hard cut." / "1 ms." / "100 ms: stays open over the short gaps between words, so it doesn't pump within a sentence." / "250 ms." / "100 Hz: desk bumps and rumble don't open it." / "Off."
8. **Gated reverb** (-20, 2, 80, Gate, 0.02, 350, 20, off, off; no follow). Why: "The 1980s drum sound: a big reverb that stops dead. Put DawIt Reverb before this gate on the drum track, with a large room or hall and plenty of reverb; the gate cuts the tail off after each hit." Notes: "High, just under the hits: the gate starts to close as soon as the tail falls under it. Raise it for a shorter sound." / "2 dB: a sharp, exact close." / "-80 dB: the tail stops dead." / "Gate." / "0.02 ms: the hit comes through whole." / "350 ms: how long the reverb lasts. At 120 BPM a beat is 500 ms, so it's gone before the next one." / "20 ms: the cut-off itself. Fast, but not so fast that it clicks." / "Off." / "Off."

### Example data (at 10 000 values per second, levels in dB)
- `makeDrumHits (4 s)`: a hit every 0.5 s from 0.1 s, each -6 dB falling 200 dB/s, power-summed with hiss at -55 dB (10 log10 (10^(hit/10) + 10^-5.5)). A hit reaches -44 dB after 0.19 s and the hiss level after 0.245 s.
- `makeFadingNote()`: 2 s of -20 - 20t + 1.5 sin (2 pi 6 t). Threshold -40, hysteresis 0, hold 0: closes at about 0.94 s, opens at 1.00 s, closes at about 1.06 s (two closes). Hysteresis 4: closes once, at about 1.23 s (after 1.075 s the level can't reach -40 again).
- `makeHitStart()`: 30 ms, -60 dB until 10 ms, then -6 dB.
- `makeTomHit()`: 1.5 s of bleed at -50 dB with a hit at 0.1 s of -6 dB falling 40 dB/s, power-summed (the hit reaches -30 dB at 0.7 s).
- `makeReverbSnare()`: 1.5 s, a hit at 0.1 s: -6 dB falling 200 dB/s, plus a tail starting at -12 dB falling 30 dB/s (60 dB in 2 s), power-summed with -60 dB hiss.

### "Your track" rules (`gate::analyse`)
Over the moments of the last 4 s (2000 at 2 ms): `floor` = 10th percentile of `detectorDb`, `loud` = 95th percentile,
`spread` = loud - floor, `flickers` = sum, suggestion S = floor + 6 (only when heard, not silent, spread >= 12). Numbers are
rounded to whole dB. Rules in order; A0 and A1 end the list; then one of A2-A5; then A6 and A7 when they apply.
- **A0** fewer than 500 moments above -90 dB: "Play the track, or click Hits and hiss, to see what the gate makes of it."
- **A1** floor < -90: "Between the notes this track is silent (under -90 dB), as software instruments usually are. There's no noise for a gate to remove: here it would only shorten the notes. Leave it off, or use it to shape the sound on purpose."
- **A2** spread < 12: "The quiet parts ({floor} dB) are only {spread} dB under the loud parts ({loud} dB), so there's little room for a threshold between them. Try a ratio of 2:1 to 3:1 with a range of 6-10 dB, so the quiet parts are turned down rather than cut."
- **A3** threshold < floor + 3: "The gate opens on the noise: the quiet parts are at {floor} dB and the threshold at {T} dB. Try about {S} dB."
- **A4** threshold > loud - 6: "The threshold ({T} dB) is within 6 dB of the loud parts ({loud} dB), so quieter notes may not open it and others get cut short. Try about {S} dB."
- **A5** otherwise: "The threshold ({T} dB) sits {T - floor} dB above the noise ({floor} dB) and {loud - T} dB under the loud parts ({loud} dB), so it should open on the notes and close in the gaps."
- **A6** flickers >= 3: "It flickered open and shut {n} times in the last 4 seconds (chatter). Try more hysteresis (6 dB) or a longer hold (30-50 ms)."
- **A7** matchGainDb > 1.0: "Match level would add {x} dB, so the gate is taking away parts of the notes, not only noise. Try a lower threshold or a longer release."

### Other texts
- Distortion drive text (`DistortionGuide.cpp`, "that's why high-gain players use a noise gate before it") and *Distortion in a mix* ("A noise gate before it...") become: "...use a noise gate, such as DawIt Gate, before or after it." and "- A noise gate (DawIt Gate) before or after it, or tidy edits, to keep the hiss between notes down."
- Reverb *Kinds of reverb*: "...gated reverb, a big reverb cut off abruptly (the 1980s drum sound of Phil Collins' 'In the Air Tonight'; DawIt Gate's Gated reverb starting point makes it), ..."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| `getReductionDb` gate closed, range 80 | 80 dB | exact |
| expander 2:1, 10 dB under, range 80 / 4:1 / 4:1 range 20 | 10 / 30 / 20 dB | 1e-6 |
| open, any level | 0 dB | exact |
| processor attack 10 ms, DC 0.1 from silence: gain at 10 ms | 0.632 | 0.002 |
| release 100 ms, hold 0, hysteresis 0, T -40: step 0.1 to 0.001; gain at 20 ms / at 123.0 ms after step | 1.0 / 0.368 | 0.001 / 0.01 |
| same with hold 50 ms: gain at 70 ms / at 173.0 ms | 1.0 / 0.368 | 0.001 / 0.01 |
| step 0.1 to -42 dB, T -40: hysteresis 4 gain at 500 ms / hysteresis 0 | 1.0 / under 0.02 | 0.001 / - |
| steady -60 dB DC, T -40: range 20 / range 80 output | -80 dB / -140 dB | 0.1 / 1 dB |
| steady -50 dB, T -40, H 0: 2:1 / 4:1 / 4:1 range 20 output after 1 s | -60 / -80 / -70 dB | 0.1 dB |
| 50 Hz sine -20 dB (50 ms fade-in), T -30: key low cut 200 Hz output peak after 1 s / no filter | under -90 dB / -20 dB | - / 0.2 dB |
| key listen, low cut 1 kHz, 100 Hz sine -20 dB: output peak | -60.0 dB | 0.3 dB |
| left DC 0.1, right DC 0.001, T -40: right output after 0.5 s | 0.001 | 1% |
| hear what it removes: closed on 0.001 DC / open on 0.1 DC | 0.001 / under 1e-5 | 1e-6 / - |
| match level: 1 kHz bursts -10 dB, 100 ms on / 400 ms off, noise peak 0.001, 3 s: `matchGainDb` / with T -5 | 0 to 0.3 dB / over 1 dB | - |
| flickers: -30/-50 dB alternating every 15 ms, T -40, H 0, hold 0, 1 s / hold 20 ms | at least 30 / 0 | - |
| `simulate (makeFadingNote)`: closes with H 0 hold 0 / H 4 | 2 / 1 | exact |
| `simulate (makeHitStart)`, gain 1 ms after the jump: attack 5 ms / 0.05 ms | -14.8 dB / above -0.1 dB | 0.3 dB / - |
| `simulate (makeDrumHits)` defaults: gain at 0.45 s | under -60 dB | - |
| `simulate` matches the processor on steady -50 dB, 2:1 | same gain | 0.1 dB |
| `formatKnob`: ratio 10, 2; range 80; key low 20; key high 20000; attack 0.05; release 1500 | "Gate", "2.0:1", "-80 dB", "Off", "Off", "0.05 ms", "1.50 s" | exact |
| presets: 8, Neutral start first, each with a why and 9 notes, settings in range; Gated reverb hold >= 250 and release <= 30; Lead vocal and Spoken voice not Gate, range <= 20 | - | - |
| `analyse`: 1400 moments at -55, 600 at -10, T -60 / -40 / -12 | floor -55, loud -10, S -49; A3 with "-49" / A5 / A4 with "-49" | 0.01 dB |
| `analyse`: 1400 at -120 + 600 at -10 / 1400 at -30 + 600 at -22 / only 100 above -90 | A1, no S / A2, no S / A0 | - |
| `analyse`: flickers summing 5; matchGainDb 2.0 | A6 with "5 times"; A7 with "2.0 dB" | - |
| theory guides: "noise gate", "gated reverb", "expander", "bleed", "chatter" find "What is a gate?" first; it's in Mixing, not on *Start here* | - | - |

## Steps
Group A (engine), build and test at step 3.
- [x] 1. `gate::` pure functions, structs, examples, `formatKnob`, presets and `analyse` in `src/engine/BuiltInGate.{h,cpp}`; add to both targets in `CMakeLists.txt`. Files: those. Done when: it compiles.
- [x] 2. `BuiltInGate` processor, `fx::Clap::Sound::hitsInHiss`, register in `BuiltInFormat` after the compressor, `Engine.cpp` comment. Files: `BuiltInGate.*`, `EffectTools.h`, `BuiltInFormat.*`, `Engine.cpp`.
- [x] 3. `runBuiltInGateTests()` with every check in *Tests* except the guide search, and `checkThroughProject<BuiltInGate>`; call it from `main`. Done when: `./build.sh --test` passes without warnings.

Group B (UI), build and test at step 8.
- [x] 4. `src/ui/GateGuide.{h,cpp}`: `gatetext` texts and the 13 pages with their pictures, from *Theory*. Check: later, in step 9.
- [x] 5. `src/ui/GateEditor.{h,cpp}`: the three views and `BuiltInGateEditor` (layout, groups, presets with the track's threshold, explainer texts, interactions and tooltips as above); `EffectWindow.cpp` button text. Add both to `DawIt` in `CMakeLists.txt`.
- [x] 6. Wiring: `MainComponent.cpp` (editor function, `openGuide`, `getPrintGuide`), `ArrangementView.cpp` switches, `TheoryGuides.{h,cpp}` (enum, `allGuides`, entry, Mixing summary), `TheoryGuidesWindow.cpp` (`makePages`); search checks in `runTheoryGuidesTests`.
- [x] 7. Device bar: the gate's chip picture, dot and match badge in `DeviceBar.cpp`.
- [x] 8. The distortion and reverb guide texts from *Other texts*. Done when: `./build.sh --test` passes without warnings and `--export-guides` exports *What is a gate?* with 13 topics.
- [x] 9. Real-app check: hook in `src/Main.cpp` adds DawIt Gate to a demo track, opens its window, requests `hitsInHiss`, and after 1.5 s snapshots the window; then the guide at *Hysteresis*, *Gated reverb* and *Your track*, and the device bar. Look for: the hiss gone between claps in the history, the open bar, the dot on the curve, a readable explainer, advice text on *Your track*, the chip's curve. Restore `Main.cpp` and rebuild.
- [x] 10. Docs: README (Features bullet after DawIt Compressor's, Tests line, Code layout lines for `BuiltInGate`, `GateEditor`, `GateGuide`), FEATURE_IDEAS (a *Done: DawIt Gate.* paragraph in the idea, "Later: chorus, limiter", drop "a noise gate" from DawIt Distortion's still-open list, and *What is mixing?*'s Done line names *What is a gate?*), CLAUDE.md (guide table row "What is a gate? | DawIt Gate's window"; DawIt Gate in the built-in effects list).

## Tests
In `runBuiltInGateTests()`: every row of *Test numbers* except the last, plus: every knob's `formatKnob` is non-empty for its default; `getRecent()` returns 2000 moments after 4 s of play and their flickers add up to the processor's count. In `runTheoryGuidesTests()`: the last row. The built-in plugin count check already requires the gate's entry.

## Website hand-off
- Features page: a section "A gate that opens for the notes and closes in the gaps", next to DawIt Compressor's: the window and the guide, with list items for threshold and hysteresis, range and ratio, attack/hold/release, the key filters and key listen, the starting points, *Your track* and gated reverb.
- Main page: *What is a gate?* as a tab in the guide picker's *Mixing* area (not *Start here*).
- Screenshots: the window on the demo song's drum track right after *Hits and hiss* (hiss gone between the claps in the history, the open bar); the guide at *Hysteresis* (the two fading notes); maybe *Gated reverb*.
- How it's made: nothing new.

## Left for later
- Lookahead (needs latency compensation checked); an external key from another track (a kick opening the bass) and a duck mode; tempo-synced hold and release; *Set it up step by step* as in the compressor; gain reduction in the mixer; noise reduction under the notes (spectral), which a gate can't do.

## Changes while building
- **Drum-hits test number changed.** `simulate (makeDrumHits)` with the defaults: the gain at 0.45 s is about -12.0 dB, not under -60 dB (the gate closes at 0.31 s and a 100 ms release has only run 1.4 times by 0.45 s; 80 dB needs about 0.92 s). The test now checks the gate is open at 0.15 s and closed at 0.45 s, the gain under -10 dB at 0.45 s and about -24 dB at 0.59 s. The guide text "the hiss between them is gone" holds only roughly with the default 100 ms release (the hiss ends 24 dB down before the next hit).
- **Key high cut range** is 100 Hz to 20 kHz (the plan said 200 Hz up), because the Kick drum starting point uses 150 Hz.
- **Key response numbers** (fact 15) are those of the ideal 12 dB/oct Butterworth filter: `getKeyResponseDb` computes at 192 kHz by default (at 48 kHz a 2 kHz high cut at 8 kHz is -25.7 dB, not -24.1). The processor's filters run at the real rate, so they are slightly lower near the top.
- **Hit-start test** reads the gain at the end of the first millisecond (index 100 + 9 at 10 kHz): -14.8 dB for a 5 ms attack, as fact 13.
- **Range dot** of the curve sits at the threshold on the closed line (a gate), or where the expander's closed line reaches the range; it is pinned to the bottom edge when below -80 dB, and dragging is relative to where it started (the plan put it at the left end of the line, which is off the chart for any range above 0).
- **Closing-level dot** coincides with the threshold dot at hysteresis 0; a click left of it picks the closing dot.
- **Analysis** uses `detectorDb` for "heard" (above -90 dB), the floor and the loud parts; the threshold suggestion is rounded to whole dB. "Hits and hiss" lasts one second, so the *Your track* topic reads it as silent gaps (A1) once the claps have scrolled past; with continuous hiss it gives A3-A5 (tested on made-up moments).
- Added `fx::Clap::Sound::hitsInHiss` and `BuiltInGate::flickerTotal` (a count the tests compare with the history).
- **Theory check: *Closing the gaps*** no longer says the hiss "is gone": with a 100 ms release it is about 24 dB down by the next hit (-12 dB at 0.45 s, -24.2 dB at 0.59 s, as tested), with 30 ms gone well before it.
- **Theory check: *Hits and hiss* explainer** no longer says the hiss "disappears" at -40 dB: about 15 dB down by the next clap with 100 ms, gone with 30 ms.
- **Theory check: attack** (topic 6 and the knob's live lead) says "a millisecond into a hit" instead of "the first millisecond": -14.8 dB is the gain at the end of that millisecond, less before it.
- **Theory check: gated reverb** (topic 10, the starting point's threshold and hold notes, the hold's typical text): hold counts from falling under the closing level, so the gate doesn't "start to close soon after each hit" and the tail isn't gone within a 500 ms beat; the picture's snare closes about 0.68 s after the hit (0.33 s to fall under -22 dB plus 350 ms hold).
- **Theory check: *Your track*:** A0 asks only to play the track; A1 says the last 4 s were silent more than a tenth of the time, which also happens after Hits and hiss (1 s) with the song stopped, and the topic text says so.
- **Theory check: curve explainer** says which dot drags which way, and that with no hysteresis the left half of the shared dot picks the closing level.
- **Theory check: Toms note** "at 8 kHz, about 24 dB less" (ideal filter -24.1 dB, the processor at 44.1-48 kHz -25.7 to -26.0 dB).
- **Fix: *Your track* ignores digital silence.** `gate::analyse` finds the noise floor and the loud parts over the audible moments only (detector above -90 dB), so gaps between clips, or Hits and hiss after it scrolls out of the 4 s, no longer give a false "silent" verdict. A1 now means the quietest audible parts are under -80 dB (software instruments falling to silence), and its text and the topic's text say digital silence isn't counted. New test: 1000 moments of digital silence plus noise at -55 dB and notes at -10 dB give floor -55 dB and a suggestion of -49 dB; the old "1400 at -120" case became "1400 at -85".
- **Fix: range dot at range 0.** It sits on the threshold dot; the threshold is now picked there.
- **Fix: gated reverb picture.** The label reads "cut off N ms after the hit", counted from the hit at 0.1 s.
