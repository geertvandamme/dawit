# Describe a sound for the other synths and on a MIDI track (part: DawIt Additive)

Status: Released
Idea: [#217 — Describe a sound for the other synths and on a MIDI track](https://github.com/geertvandamme/dawit/issues/217)

## Goal
*Describe a sound* comes to DawIt Additive's window: the user types "a hollow clarinet" or "a bell that rings longer", the local model (qwen2.5:7b through Ollama) picks the nearest of Additive's 12 starting points and suggests the knobs and harmonic bars to change, each with a reason, and DawIt checks, clamps and applies them or walks through them. This plan covers only the Additive part of #217; FM, Drums, Physical, the MIDI-track entry and matching a recording stay open there. It reuses the released Subtractive design ([describe-a-sound-and-ai-sets-the-synth.md](describe-a-sound-and-ai-sets-the-synth.md)) and generalises its code so the next synths are a table each, not a copy.

## Decisions for you
Carried over unchanged from #46 (agreed 2026-11-23): the bar's own Undo (20 changes); the last description not saved; nothing changes until *Walk me through it* or *Set it all now*; the shared `generator.model` and `generator.ollamaUrl`; hidden in VST3/AU; walkthrough in window order; qwen2.5:7b, never qwen2.5:3b; the evaluation gate (at least 9 of 12).

1. **What the AI may set.** Options: (a) the 17 knobs only; (b) the knobs and single harmonic bars (h1 to h32), bar changes counted as one change and walked as one step; (c) as (b) plus a "fill with a shape" pseudo-knob. **Recommended:** (b). In the prototype (below), (c) misled qwen2.5:7b ("a jazz organ" turned the organ into a sine), and the bars are what additive synthesis is about.
   Answer:
2. **Best worked example last in the prompt.** Measured on Additive: placing the closest example nearest the question raised the score. Options: do it for both synths (one code path) or only Additive. **Recommended:** both, as long as Subtractive's evaluation, re-run in Group A, stays at 9 of 12 or more; otherwise keep Subtractive's order with a table flag.
   Answer:
3. **Page names.** Use the same names as Subtractive's pages (*Words for sounds*, *Describe a sound to the AI*) or Additive-specific ones. **Recommended:** the same names; links name the guide too, and search finds both.
   Answer:
4. **Subtractive's bell note.** Its limit note sends bells to DawIt FM only. **Recommended:** add "DawIt Additive's Stretch makes bells too", so the two synths agree.
   Answer:

**Answers (user, 2026-11-23):** 1. (b) knobs and single bars. 2. Both synths, if Subtractive's re-run stays at 9 of 12 or more; otherwise only Additive. 3. The same page names as Subtractive. 4. Yes, add the Additive Stretch bell note to Subtractive's limit note.

## What the user does and sees
As in the Subtractive plan's section of the same name, in DawIt Additive's window; only the differences:
- The **Describe a sound** button sits in Additive's header right of *Save...*, app only. The 76 px bar opens across the top of the picture area; the bars picture, wave, note-over-time and history shrink to fit.
- **The answer** lists the bar changes first, as one section: "Harmonics: 3rd -9.5 dB → -20.0 dB, 8th -9.0 dB → Off", each bar with its reason, then the knobs in window order. "From Hollow reed: 3 changes" counts the bars as one.
- **Walkthrough order** follows the window: the bars, then the top knob row (Harmonics group, Movement, LFO), then the bottom row (Amp envelope, Playing).
- **The bars step:** an `accent` ring around the bars picture, and the bars to change lit (`additext::drawPartials`'s `lit`). Row 2: "Step 1 of 4 · Harmonics: set the 3rd to -20 dB and the 8th to Off (now -9.5 dB and -9.0 dB). <first reason>". Over four bars it lists four and "and 3 more (lit)". It is reached when every changed bar is within 1.2 dB (2% of the 60 dB travel) of its target, and an Off target only at Off. The line then reads "Done: all 2 bars set." *Set it for me*, *Back* and *Undo* act on all the step's bars at once.
- **Tooltips:** the Subtractive texts, except Describe a sound, Walk me through it, Set it for me and Back, which use the Additive versions in *Theory*. Each synth's table supplies its texts.
- **Settings:** `describe.additive.recent` (last 8 descriptions); the Subtractive key is unchanged.
- **First steps:** no change. The feature lives only in DawIt Additive's window, which the start panel, *Get started* and the First Song Exercise don't name.

## Data and saving
Nothing new in the project; the format doesn't change. A described sound is ordinary Additive parameters. `BuiltInAdditive` gets a `lastDescription` like Subtractive's, held on the message thread, in memory only, and not in presets.

## The guide
Two pages at the end of *What is additive synthesis?* (`additext::makeGuide`), before *Sources*:
1. **Words for sounds**: "Bright, warm, hollow, metallic: what they mean in harmonics". The picture is the current sound's bars (`drawPartials`, no movement), with a vertical line at the spectral centroid placed at its harmonic number (centroid ÷ 220 Hz) and labelled "Centre of brightness 1.73 kHz". Live.
2. **Describe a sound to the AI**: "What the AI does, what DawIt checks, and the walkthrough". The picture shows the bars of the result of the last description, with the changed bars lit and the caption "Your last description: \"a hollow clarinet\" · 3 changes" ("Describe a sound to see its changes here" when there is none). `liveText` follows Subtractive's pattern (texts in *Theory*).

- **Theory Guides entry** (`Guide::additive`): add the search words `describe ai words bright warm hollow metallic`, and end the summary with ", words for sounds, and an AI that suggests the settings". Not on *Start here*.
- **Links** (`TheoryGuideLinks.cpp`):
  - subtractive · Words for sounds | additive · Words for sounds
  - subtractive · Describe a sound to the AI | additive · Describe a sound to the AI
  - lyrics · Writing with AI | additive · Describe a sound to the AI
- **Opens from:** the guide button, the bar's right-click menu (last item, "Describing sounds guide...") and the explainer's *Read about describing sounds*.
- **Fix in Subtractive's page 2:** "it only sets DawIt Subtractive so far" becomes "it sets DawIt Subtractive and DawIt Additive so far".

## Code
Recipes: *A built-in effect or instrument* (its note on app-only parts through a hook), *Topics in an existing guide*, *Checking in the real app*. Generalise the shipped code; don't copy it.

- **A synth's describe table** (`gen::sounddescribe`). It holds:
  - the knobs, each with an id, title, range and unit, choice names, typical text, a group and the walk order;
  - the starting points, as schema-safe names (no `"` characters) with the first sentence of `why` and their values;
  - the worked examples with keywords and fallbacks, and the follow-up examples;
  - the synth-specific prompt lines;
  - the word notes, the limit notes and the centroid.
  A sound is a list of values in the knobs' own units. The prompt, schema, reading, clamping, cap, walkthrough and undo work on any table. Subtractive's table and Additive's table go in their own files in `src/generate` (both targets). Each converts its `Settings` to and from values.
- **The described-sound record** moves from `sub::` to a plain plugin-safe engine type with knob indices and value lists. Both processors hold one as `lastDescription`.
- **Additive engine:**
  - `additive::getKnobInfo`, `getKnobValue` and `setKnobValue` cover the 17 knobs and the 32 bars (indices 17 to 48). A bar runs from -60 dB, which is Off, to 0 dB, in steps of 0.1 dB.
  - `additext::getTypical` moves to `additive::` (as Subtractive's did).
  - `additive::getCentroidHz (settings, noteHz)` takes the held note with no movement at full velocity, after Brightness, Even and Stretch.
- **Groups:** all bar changes form one group. The group counts as one change against the cap of 10, holds at most 12 bars (more are dropped and counted) and walks as one step.
- **The UI is shared:**
  - `SubtractiveDescribeBar` becomes `DescribeBar` (app only).
  - The header button, bar area, highlight ring, explainer topic 130 and guide link move into a plugin-safe shared part that both editors use.
  - One factory hook is set in `MainComponent` and cleared in its destructor.
  - Each editor maps a knob index to the component to ring. Additive maps the bars to the bars picture.
- **Invariants and threading:** as in the Subtractive plan (clamp everything, drop equal values, the last duplicate wins, apply only through `setKnob`/`setSettings`/`setPartialDb` on the message thread, the Ollama call on a worker with copies, request ids, nothing touches the edit or the engine lock).
- **Risks:**
  - The refactor touches every Subtractive describe file. Its ~110 checks are the regression gate, and the Subtractive prompt must stay under 16,000 characters.
  - A starting point name with quotes breaks qwen2.5:7b's choice. With `"Ah" pad` in the schema's enum, the model never chose that starting point and fell to Plucked string; without the quotes it did.
  - The plugin targets must still link (`./build.sh --plugins` once in B1).

**Prototype (measured 2026-11-23, qwen2.5:7b on this Mac, temperature 0.3, the design above in Python, prompt 9,600 characters):** 10 of 12 on seeds 1 to 12 and 12 of 12 on seeds 101 to 112, median 3.8 s. The misses were "a jazz organ" (Amp sustain lowered to 80%) and "brass that blooms" (Swell shortened to 50 ms). The script is in the planning run's scratchpad and is not shipped; dawImplement writes the C++ evaluation from step 5's cases.

## Libraries and models
None new. Ollama (MIT) is already listed as a separate program. qwen2.5:7b (Apache-2.0, already checked for #46) is pulled by the user and never shipped. Don't suggest qwen2.5:3b (non-open `qwen-research` licence). No change to `LICENSE`, `licenses/`, packaging or the README's *License note*.

## Theory

### Facts
1. Brightness follows the spectral centroid (Schubert & Wolfe 2006). Reused from Subtractive. **Typical.**
2. Spectral centroid = Σ f·a / Σ a over the partials. Definition. **Exact.**
3. Sine on A3: centroid 220 Hz. **Exact.**
4. The 32-bar saw (bar n at 1/n) on A3: 220 × 32 / H₃₂ = 7,040 / 4.0585 = 1,734.6 Hz. Derived. **Exact.**
5. The same saw at -3 dB/oct (each bar a further × n^(−0.498)): 969.1 Hz. Derived numerically. **Exact.**
6. Square (odd bars to 31 at 1/n): 220 × 16 / Σ(1/odd n) = 3,520 / 2.3681 = 1,486.4 Hz. The saw at Even 0% gives the same. Derived. **Exact.**
7. Organ shape (bars 1 to 4 at 0 dB): (1+2+3+4)/4 × 220 = 550 Hz. Derived. **Exact.**
8. A Subtractive saw (90 harmonics to 20 kHz) on A3 has its centroid at 3,896 Hz. Reused from the Subtractive plan. The 32-bar saw is darker, which agrees with the Additive preset note "a touch softer than an analog saw". **Exact.**
9. At 100% Stretch the 8th harmonic is 193.8 cents sharp: 1200 log₂ √((1 + 0.004·64)/1.004). It agrees with the Metal bell note ("nearly 2 semitones"). Derived. **Exact.**
10. Timbre words fall into luminance, texture and mass (Zacharakis et al. 2014). Reused. **Typical.**
11. "Warm" means a richer lower middle; warm and round overlap, and people agree on them less (Rosi et al.). Reused. **Opinion.**
12. Hollow, reedy sounds have mostly odd harmonics, as in a clarinet's low register (existing Additive source). **Typical.**
13. Typical settings (Even 0–20% hollow; Stretch 60–100% bells, 5–15% strings and keys; Fade 0.2–0.5 s plucks; Swell 50–300 ms brass; attack 0.3–1 s pads; Decay 1–3 s plucks) come from the existing `getTypical`. **Typical.**
14. DawIt Additive plays the same signal in both speakers. It has no noise, no filter, no second oscillator and no pitch envelope. Phases: Spread changes the wave's shape, not its place in stereo. From the code. **Exact.**
15. A language model writes likely text and doesn't hear sound; Ollama runs it on the user's computer. Reused. **Exact.**

### Topic texts

**Words for sounds**: *Bright, warm, hollow, metallic: what they mean in harmonics*

People describe sounds with borrowed words: bright, dark, warm, hollow, metallic, thin, full. They aren't measurements, but in an additive synth most of them point at the bars and a few knobs, because they describe which harmonics are loud. A study of English and Greek listeners found the same three families of words in both languages: how bright a sound is, its texture (smooth or rough) and its mass (thick or thin).

Bright and dark: how much of the sound's energy sits high up. Listening tests find brightness follows the spectral centroid, the average frequency of the harmonics, weighted by how loud each one is. On A3 (220 Hz) a sine has it at 220 Hz, as dark as a note can be. The saw of *A saw, from sines* has it at about 1.73 kHz. DawIt Subtractive's saw, with all 90 harmonics up to 20 kHz, has it near 3.9 kHz, which is why that saw sounds a little brighter. To go brighter, turn Brightness up (towards +6 dB/oct) or draw the higher bars taller. To go darker, turn it down: at -3 dB/oct the same saw's centre falls to about 970 Hz. The picture marks the centre of the sound in the window now.

Warm: strong low harmonics (the 2nd to 4th) and a soft top. Sound engineers mostly mean that, but people agree on "warm" less than on "bright", and "warm" and "round" are often used for each other. Here, try Brightness at -2 to -4 dB/oct, with the first few bars strong.

Hollow, woody or reedy: the odd harmonics without the even ones. Turn Even down to 0 to 20%, and a saw becomes a square. Its centre on A3 moves from about 1.73 kHz to 1.49 kHz, but the change you hear is the hollow colour more than the darkness. A clarinet's low notes sound hollow for this reason.

Metallic or bell-like: partials that aren't whole-number multiples of the note. Turn Stretch up to 60 to 100%: at 100% the 8th harmonic is nearly 2 semitones sharp. A little stretch (5 to 15%) sounds like a real string or a piano rather than metal.

Full or thin: full has several low bars on and strong; thin has few, or one bar alone high up. "Fat" in the sense of two detuned oscillators isn't possible here: DawIt Additive has one set of harmonics per note. DawIt Subtractive's Detune makes that sound.

Plucky or swelling: how the harmonics move. A pluck starts with every harmonic and loses the high ones first: Fade at 0.2 to 0.5 s and Amp sustain at 0%. A brass note or a bow brings the high ones in later: Swell at 50 to 300 ms. Soft or slow means a longer Amp attack, 0.3 to 1 s for a pad. Attack is a time, so a longer attack starts more softly.

What these words can't mean here:
- **Wide:** DawIt Additive plays the same sound in both speakers, so "wide" means fuller, not wider in stereo. Phases: Spread changes the shape of the wave, not where it sits.
- **Breathy, airy, squelchy:** it has no noise for breath or air and no filter for squelch. DawIt Subtractive has both.

All of this is common practice among synth programmers, not a set of rules: try the bar or the knob, then trust your ears.

**Describe a sound to the AI**: *What the AI does, what DawIt checks, and the walkthrough*

Describe a sound, at the top of DawIt Additive's window, lets you type what you want to hear, such as "a hollow clarinet" or "a bell that rings longer". A language model then suggests the settings, with a reason for each. It's for getting started on a sound and learning which bars and knobs make it; your ears still decide.

How it works:
- DawIt sends the model the synth's 17 knobs and 32 harmonic bars with their ranges and usual settings, the 12 starting points with their settings, and your words.
- The model runs on your own computer through Ollama, so nothing leaves it.
- It picks the starting point closest to your words and answers with only what to change, each with a value and a reason. All the changes to the bars count as one change.
- A follow-up such as "a bit brighter" or "more hollow" changes the current sound and touches only what it needs.

What DawIt checks: a language model writes likely text; it doesn't hear sound. So DawIt reads every value itself and keeps it inside the range:
- A bar at +3 dB becomes 0 dB, the loudest a bar goes, and the step says so.
- It leaves out knobs the synth doesn't have.
- It measures the result: if you asked for brighter and the answer is darker on A3, it tells you.
- It says when a sound needs something this synth doesn't have: breath, noise or filter squelch (DawIt Subtractive), an 808's falling pitch (DawIt Drums), a real guitar or clarinet (DawIt Physical) or a piano (DawIt Sampler).

The reasons are the model's own words. Most are sensible, but a model can be wrong and sound sure of itself. Hover a knob or a bar to read DawIt's own explanation, and check what you hear.

Walk me through it shows the changes in the window's order: the bars first, then the knobs, top row first. It lights the bars or the knob, says the value and why, and waits for you to set it. A bar counts when it's within about 1 dB of its target, and a bar to turn off must be off. You can also click Set it for me. Back, Skip and Finish all are there too. Set it all now changes everything at once. Undo in the bar takes back the last change, whether a whole answer or one step. The app's Undo doesn't cover synth settings.

Getting the best from it: describe the sound, not the song ("a soft bell" rather than "something for my intro"). Ask for one or two changes per follow-up. Try the same words twice, because each answer can differ. Small models answer faster but miss more often; qwen2.5:7b is a good start.

What it doesn't do: it doesn't listen to a recording to copy its sound, and so far it sets only DawIt Subtractive and DawIt Additive.

*Live text:* the Subtractive page's three cases with "DawIt Additive's window". A bar change line reads "Harmonics: 3rd -9.5 dB → -20.0 dB".

**Tooltips (Additive versions)**
- Describe a sound: "Type what you want to hear, and an AI model on your computer suggests the bars and knobs, with a reason for each. Needs Ollama."
- Walk me through it: "Go through the changes one at a time: DawIt lights each knob or set of bars and waits for you to set it."
- Set it for me: "Set this knob, or these bars, to the suggested values for you, then go to the next step."
- Back: "Go back a step and put that knob, or those bars, back as they were."

**Status texts:** the Subtractive texts, plus the bar clamp note "The model asked for +3 dB; the bar stops at 0 dB." and "<n> bar changes beyond 12 were left out."

**Prompt lines for Additive** (dawImplement writes the final wording; the prototype's wording scored as above):
- The intro says that each note is 32 sines at the harmonics, h1 being the note itself. Each bar runs from 0 dB down to -60 dB, which is Off.
- The knob names carry their direction: "Brightness: tilts all harmonics; up (+) is brighter, down (-) is darker". "Stretch: pulls the harmonics sharp; more is more metallic and bell-like".
- "Pick the starting point by the kind of sound the words name (pad, bass, pluck, organ, bell, lead), not by one knob's name."
- "Change single harmonics only when the colour needs it, such as a missing octave or a vowel; most changes are knobs. Harmonic changes count as one change, at most 12 bars."
- What this synth lacks: noise or breath, a filter with resonance, a second detuned oscillator, a pitch envelope, stereo spread, effects, realistic acoustic instruments.
- A sound's bars are written as a shape name when they equal one of `getShapeLevels` (within 0.05 dB), such as "Saw shape". Otherwise they are written as the bars that are on ("h1 0 dB, h2 -8 dB, ..., the rest Off").

**Worked examples** (starting point: description → two changes):
- Drawbar organ: "a jazz organ with a bright top" → h8 0 dB, lfoDepth 6%
- Hollow reed: "a hollow woody clarinet" → brightness -4.0 dB/oct, ampAttack 50 ms
- Metal bell: "a bright metallic bell" → fade 2.00 s, brightness +2.0 dB/oct
- Plucked string: "a soft harp pluck" → fade 150 ms, brightness -5.0 dB/oct
- Sub bass: "a deep clean sub bass" → h3 Off, ampRelease 150 ms
- Shimmer pad: "a slowly shifting pad" → lfoRate 0.10 Hz, ampAttack 1.50 s
- Shimmer pad: "a warm soft pad" → brightness -4.0 dB/oct, lfoDepth 50%
- Brass swell: "a punchy brass stab" → swell 80 ms, ampSustain 60%
- Flute: "a breathy soft flute", whose summary says there is no noise → ampAttack 150 ms, h2 -12 dB
- Electric keys: "mellow electric piano" → h8 -30 dB, velocity 50%
- Ah pad: "a choir singing ah" → ampAttack 1.00 s, lfoDepth 5%
- A saw, from sines: "a bright buzzy lead" → mode Mono, glide 50 ms

Each gets one plain reason a sentence long. The fallbacks are Drawbar organ, Plucked string and Shimmer pad.

**Follow-ups:**
- "brighter", from Plucked string: brightness -3.0 → +0.0 dB/oct. Reason: "Less tilt lets the high harmonics come through louder."
- "softer start", from Drawbar organ: ampAttack 3 ms → 60 ms. Reason: "A longer attack fades each note in over 60 ms instead of starting with a click."

### "Your sound" rules (Additive)
Whole words, any case. These are advice and never block an answer. The centroid is `additive::getCentroidHz` at A3.

| Words | Condition | Advice | Quiet when |
|---|---|---|---|
| bright, brighter, brightness | follow-up, centroid after ≤ 0.95 × before | "You asked for brighter, but on A3 the answer is darker: its centre of brightness moves from <a> to <b>. Try Brightness up, or taller high bars, or ask again." | not a follow-up |
| dark, darker, dull, duller, muffled | follow-up, centroid after ≥ 1.05 × before | "You asked for darker, but on A3 the answer is brighter (<a> to <b>). Try Brightness down, or ask again." | not a follow-up |
| pluck, plucky, plucked, stab | amp sustain > 30% | "A pluck usually fades even while the key is held: try Amp sustain at 0%, Decay between 1 and 3 s, and Fade between 0.2 and 0.5 s so the high harmonics go first." | |
| pad, pads, "fade in", "fades in", "slow attack" | amp attack < 0.1 s | "A pad usually fades in: try Amp attack between 0.3 and 1 s." | |
| bass | Voices: Poly | Subtractive's text | words include chord or chords |
| "softer start", "less attack", "softer attack" | follow-up, attack after ≤ before | Subtractive's text | not a follow-up |
| hollow, woody, reedy, clarinet | Even > 20% and an even bar is on | "A hollow sound comes from the odd harmonics: try Even at 0 to 20%, so the 2nd, 4th, 6th... are quiet." | |
| metal, metallic, bell, bells, chime, chimes, gong | Stretch < 50% | "Bells and metal have partials that aren't whole-number multiples of the note: try Stretch between 60 and 100%." | |

**Limit notes (Additive)**, one per group:
- 808, kick, snare, hihat, hi-hat, drum, drums: "An 808 kick drops in pitch at the start of each note, and DawIt Additive has no pitch envelope. DawIt Drums makes 808-style drums; for a deep bass here, start from Sub bass."
- noise, breath, breathy, airy, air, whisper: "DawIt Additive makes only harmonics, so it has no noise for breath or air. DawIt Subtractive has a Noise knob; here, a slower attack and a softer top sound gentler."
- squelch, squelchy, acid, resonance, resonant, 303: "DawIt Additive has no filter, so there is no resonance to squelch. DawIt Subtractive makes acid sounds; here, the LFO on Brightness moves the colour."
- fat, phat, thick, detune, detuned, supersaw, unison: "DawIt Additive has one set of harmonics per note, with no second oscillator to detune. DawIt Subtractive's Detune makes a fat sound; here, stronger low bars (the 2nd to 4th) make it fuller."
- guitar, harp, cello, violin, clarinet: "A real guitar, harp, cello or clarinet comes from a vibrating string or tube, and DawIt Physical models those. Here you get an additive version."
- piano (not after "electric"), orchestra, orchestral: "For a real piano or orchestra, DawIt Sampler plays recordings of real instruments."
- reverb, echo, delay, distortion, distorted: Subtractive's text.
- chorus: "DawIt has no chorus effect, and DawIt Additive has no second oscillator to detune. DawIt Subtractive's Detune gives a similar thickness."
- wide, stereo, spread: "DawIt Additive plays the same sound in both speakers; Phases: Spread changes the wave's shape, not its width. A reverb after it adds space."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Centroid, Init (sine) at A3 | 220 Hz | ±0.1 Hz |
| Centroid, *A saw, from sines* | 1,734.6 Hz | ±0.5% |
| Same at Brightness -3 dB/oct | 969.1 Hz | ±0.5% |
| Square shape; saw at Even 0% | 1,486.4 Hz each | ±0.5% |
| Organ shape | 550.0 Hz | ±0.5 Hz |
| `getStretchCents (8, 100)` | 193.8 ct | ±0.5 |
| Read h3 "-12 dB" / "Off" / "+3 dB" | -12 / -60 / 0 with clamp note | exact |
| Read "-20 dB/oct" (brightness) | -12, with note | exact |
| Read fade "Off", swell "Off", "350 ms" swell | 30, 0, 0.35 | exact |
| Read "Spread", lfoTarget "even", "mono" | 1, 2, 1 | exact |
| Every Additive preset × knob and bar: read (`formatKnob`/`formatLevel`) | equals the value | as Subtractive's |
| Bar target -20 dB | -19.0 reached, -18.5 not | 1.2 dB |
| Bar target Off | -60 reached, -58 not | |
| Fade target 0.5 s | 0.57 reached, 0.60 not | 2% of ln(30 / 0.02) |
| Schema ids | 49 distinct (17 knobs, h1–h32) | |
| Schema `startFrom` | 12 names + "Current sound", none with `"`; "Ah pad" reads as `"Ah" pad` | |
| Prompt, 200-character description | < 16,000 characters | |
| 11 knob changes + 14 bars | the bar group (first 12 bars) + 9 knobs = 10; 2 bars and 2 knobs dropped and counted | |

Text checks: both pages have text and a picture; searching "hollow", "describe" and "metallic" finds the additive guide; every Additive advice and limit note is non-empty and names an existing built-in.

## Steps

Group A, engine (`./build.sh --test` at step 5):
- [x] 1. Make the described-sound record a plugin-safe engine type with knob indices and value lists, used by Subtractive's processor, editor, guide and bar. In `BuiltInAdditive`, add knob info, get and set value over the 17 knobs and 32 bars, `getTypical` (moved from `additext`), `getCentroidHz` and `lastDescription`. Tests: the centroid and stretch rows. Check: the windows' texts are unchanged.
- [x] 2. Make `gen::sounddescribe` table-driven (see *Code*), with Subtractive's table in its own file and both targets in CMake. Tests: every existing Subtractive check passes, with only its types changed. Done when the Subtractive prompt is unchanged apart from decision 2's ordering.
- [x] 3. Add Additive's table: knobs and bars, the prompt lines, the bars written as a shape or a list, the 12 worked examples, the follow-ups and schema-safe names. Tests: the read rows, the round trip, the schema rows, prompt length, "a hollow woody clarinet" picks Hollow reed first.
- [x] 4. Add groups (the bars as one change and one step, the 12-bar cap), the bars step's reached rule, Back/Set for me/Undo over a whole group, and Additive's word and limit notes. Update Subtractive's bell note (decision 4). Tests: the cap row, the bar rows, each *Your sound* row firing and staying quiet, each limit group.
- [x] 5. Add the Additive evaluation to `runSoundDescribeEval` (`--ollama <model>`). The cases, all checks on the result, from Init unless a starting point is named:
  - "a jazz organ": sustain ≥ 90%, Fade Off, attack ≤ 50 ms;
  - "a deep sub bass": Mono, centroid ≤ 400 Hz;
  - "a warm pad that swells in slowly": attack ≥ 0.3 s, Poly;
  - "a short pluck": sustain ≤ 10%, Fade ≤ 1 s;
  - "a hollow clarinet": the even bars' share of the power ≤ 10%, Mono;
  - "a metallic bell": Stretch ≥ 50%, sustain ≤ 10%;
  - "a flute with vibrato": LFO on Pitch, depth ≥ 2%, rate 4–7 Hz, centroid ≤ 600 Hz;
  - "brass that blooms at the start of each note": Swell ≥ 0.1 s;
  - "a slowly shifting shimmering pad": LFO on Brightness or Even, rate ≤ 1 Hz, depth ≥ 30%;
  - follow-ups: "brighter" from *A saw, from sines* (centroid +10%), "darker" from Electric keys (−10%), "softer start" from Drawbar organ (attack ×2).

  Re-run Subtractive's 12 too. Record both scores and medians in *Changes while building* and comment them on #216. Done when: `./build.sh --test` passes without warnings and both scores are at least 9 of 12.

Group B1, UI: the shared bar (`./build.sh --test` at step 8):
- [x] 6. Move the Describe button, bar area, highlight ring, explainer view (topic 130) and guide link out of `BuiltInSubtractiveEditor` into a plugin-safe shared part. Make one factory hook and rename `SubtractiveDescribeBar` to `DescribeBar`, which takes the table, its recent-list key and the editor's index-to-component map. Check: the Subtractive shots of #46 look the same; `./build.sh --plugins` links Subtractive and Additive.
- [x] 7. Give Additive's editor the shared part: the header button, the bar above the pictures, the ring on knobs, switches and the bars picture with the step's bars lit, the answer view with the Harmonics section, and the Additive tooltips. Check: a canned reply in a shot.
- [x] 8. Make the `ScreenshotRunner` describe keys work on `"plugin": "DawIt Additive"`, and update the key list in `DevScreenshots.h`. Done when the build passes and the plugin scene has no button.

Group B2, UI: the guide (`./build.sh --test` at step 10):
- [x] 9. Add Additive's two pages (pictures and live text), the entry's words and summary, and the three links. Fix the sentence in Subtractive's page 2. Tests: search rows, links exist, page count +2; `--export-guides` passes.
- [x] 10. Wire *Read about describing sounds* and the menu's guide item to Additive's page 2. Done when: the build passes.

Group C, check and docs:
- [x] 11. Real-app check. The Keys track with DawIt Additive, from Hollow reed, at 1400 × 780:
  - (a) the bar with text typed;
  - (b) a canned answer "a hollow clarinet" with a bar group (h2 Off, h4 Off) and two knobs, one of them clamped;
  - (c) walkthrough step 1, with the ring on the bars picture and two bars lit;
  - (d) after *Set it all now*;
  - (e) the `aiSetup` unreachable state;
  - (f) *Words for sounds* and (g) *Describe a sound to the AI* in the guide window (`"capture": "What is additive synthesis?"`);
  - (h) the plugin scene, with no button;
  - (i) one Subtractive shot, (b) of #46, which must look as before.

  Check: nothing is clipped, the ring and the lit bars are right, and the centroid label sits on the line.
- [x] 12. Docs:
  - the README Features bullet for Additive, and the Ollama line naming both synths;
  - `docs/tests.md` and `docs/code-layout.md` (the tables, `DescribeBar`, the shared part);
  - `docs/architecture.md` *Describe a sound*, generalised;
  - the note in `docs/recipes.md` *A built-in effect or instrument*: a new synth adds a describe table and an index-to-component map;
  - on #217, tick the Additive item, record the scores and leave the rest open.

## Tests
The numbers above, plus:
- the Subtractive checks unchanged;
- a group step's Back restores every bar;
- undo after *Set it all now* restores bars and knobs;
- a reply naming `h33` or `shape` is dropped and counted;
- equal bar values are dropped;
- the last of two `h3` entries wins.

## Website hand-off
- `features.html`, synths group: extend the Describe a sound section to DawIt Additive. Shots: (b) and (c).
- `learn.html`: the two new pages of *What is additive synthesis?*. Shot: (f).
- `about.html`: no new library.

## Left for later
- FM, Drums and Physical tables; *Describe a sound...* on a MIDI track; matching a recording (all #217).
- A starting point for a plain warm pad in Additive (the prototype's warm-pad case relies on *Ah pad*).
- Approaches 2 and 3 (#216); plugin knobs in the app's undo; cloud models (#41).

## Changes while building

**Group A hand-off (engine, built 2026-11-23, `./build.sh --test` passes with no warnings, `./build.sh --plugins` links):**
- `src/engine/DescribedSound.h`: plugin-safe `daw::DescribedSound` / `DescribedChange` (knob as an `int` index, `baseValues` vector). Both processors hold `lastDescription`. `sub::DescribedSound` is gone; UI casts `(sub::Knob) c.knob`.
- `src/generate/SoundDescribe.{h,cpp}` is table-driven: `Table` (knobs `KnobDesc`, `StartingPoint`, `Example`, `promptHead`, `extraKnobRows`, `followUpExamples`, `format`, `readChoice`, `describeValues`, `wordNotes`, `limitNotes`), sounds are `Values` (`std::vector<float>`). Functions take the table first: `buildSystemPrompt`, `buildUserPrompt`, `buildSchema`, `readReply`, `readValue`, `pickExamples` (names) and `pickExampleIndices`. `Walkthrough (table, sound, live)`, `Walkthrough::isReached (table, ...)`, `ValuesUndo` (was `SettingsUndo`), `areSame (table, a, b)`, `getResultValues (sound)`. `Result` has `droppedGroup` (bars beyond 12) beside `droppedExtra`. A `Walkthrough::Step` has `knob/before/target` of its first knob plus `group` and `members` (all changes of the step); a group is one step and one change against the cap.
- Tables: `getSubtractiveTable()` (`SoundDescribeSubtractive.{h,cpp}`, with `getSubtractiveValues/Settings`) and `getAdditiveTable()` (`SoundDescribeAdditive.{h,cpp}`, `getAdditiveValues/Settings`, `describeAdditiveBars`). Additive indices: 0 to 16 the knobs, 17 to 48 the bars (`additive::firstBarIndex`, `numControls`); `KnobDesc::order` gives the window order (bars first). Both files are in both CMake targets.
- `additive::getKnobInfo/getKnobValue/setKnobValue (int index, ...)`, `additive::getTypical` (moved from `additext`; the editor calls it), `additive::getCentroidHz (settings, noteHz)`.
- The Subtractive bar (`SubtractiveDescribeBar`) still exists, adapted with `getLive()`/`setLive()`/`getTable()`; B1 turns it into `DescribeBar` taking a table plus accessors. The walkthrough's bar-group step needs UI: `step.members`, `step.group`.
- A prompt line for the bars is one extra row (`h1 to h32 | ...`). Starting-point names in the table have no quotation marks (`Ah pad`); `StartingPoint::displayName` keeps `"Ah" pad`, which is what `DescribedSound::base` shows.
- Decision 2: the closest worked example now goes last for both synths (re-run kept both at 12 of 12).
- Decision 4: Subtractive's bell note now ends "and DawIt Additive's Stretch makes bells too". Subtractive's page 2 now says "it sets DawIt Subtractive and DawIt Additive so far" (that page fix belongs to step 9 but was a one-line change).
- Not yet done for later groups: the Subtractive guide page 2 text still says a bell goes to DawIt FM only in its limits sentence; the Additive guide pages; the shared UI.

**Group B1 hand-off (UI: the shared bar, built 2026-11-23; `./build.sh --test` passes with no warnings, `./build.sh --plugins` links Subtractive and Additive):**
- `src/ui/DescribeController.{h,cpp}` (plugin-safe; in `DAWIT_PLUGIN_SHARED_SOURCES` and the app): `DescribeHost` (what the bar asks of the window), `DescribeConfig` (the synth's table, recent key, colour, topic, `last` description, `getLive`/`setLive`, `getComponent` (knob index to the control to ring), `onHighlight`, `showTopic`, `openGuide` and the synth's words), `DescribeBarBase`, and `DescribeController` (button, guide link, bar space, ring, `buildText` for topic 130, the static `barFactory`). Each editor owns one `describe` and calls `layout`, `layoutBar`, `tick`, `paintRing` and `buildText` from its resized, layoutPictures, tick, paintOverChildren and buildText (all guarded against null, since the base class lays out before the member exists). `daw::describePageName` is the shared page name.
- `src/ui/DescribeBar.{h,cpp}` is the old `SubtractiveDescribeBar`, now taking `(DescribeHost&, const DescribeConfig&, Engine&)`; a group step reads "Harmonics: set the 3rd to -20 dB and the 8th to Off (now -9.5 dB and -9.0 dB). reason", with "and N more (lit)" over four, and "Done: all N bars set." `MainComponent` sets `DescribeController::barFactory`.
- Both editors expose `getDescribe()`, `hasDescribeBar()`, `isDescribeBarOpen()`, `getDescribeBar()`, `setDescribeBarOpen()` and `openDescribeGuide()`. Additive's `openDescribeGuide` looks for the page named `describePageName`: until step 9 adds it, it opens the guide at the page it shows. `AdditivePartialsView::setLitBars` lights the step's bars.
- `gen::sounddescribe`: `Table::groupTitles` and `memberLabel` ("3rd"), `countChanges (table, sound)` (a group counts as one; the bar's "n changes" and `made` use it), and `areSame` now compares within half a knob's step. Why: the synth rounds a bar to 0.1 dB, so a starting point such as Hollow reed (a bar of -9.54 dB) never equalled the live sound and the walkthrough began with a spurious "Load the starting point" step.
- CMake: the two tables moved from the app and test lists to the built-ins' `ENGINE_SOURCES` (so the plugins link them); `SoundDescribe.cpp` and `DescribeController.cpp` are in the plugin shared sources. `ScreenshotRunner`: the describe keys and a `"preset"` key work for `"plugin": "DawIt Additive"`; `DevScreenshots.h` and recipes updated. Docs updated: recipes, architecture, code-layout.
- Seen in a shot (Hollow reed, canned reply, walkthrough step 1): the ring round the bars picture with the 3rd bar lit, "Step 1 of 2" row, a Harmonics change counted as one ("2 changes"). The explainer in `--screenshots` shows the hovered control's topic, not the answer, because the real cursor rests over the window (Subtractive does the same), so the Harmonics section of the answer text is not seen in a picture yet: check it in group C.
- Left for later groups: Additive guide pages and links (B2), the real-app check and README/tests/code-layout entries for the feature (C).

**Evaluation (qwen2.5:7b, seeds 1 to 12, temperature 0.3):** DawIt Subtractive 12 of 12, median 4.4 s; DawIt Additive 12 of 12, median 4.1 s. (Several cases pass from the starting point with 0 changes, so the checks are lenient; the eval is a floor, not a proof.) To comment on #216 by the coordinator.

**Group B2 hand-off (UI: the guide, built 2026-11-23; `./build.sh --test` passes with no warnings, `./build.sh --plugins` links):**
- `additext::makeGuide` has the two pages *Words for sounds* (live: the bars with a line at the centroid's harmonic number, labelled "Centre of brightness 1.73 kHz") and *Describe a sound to the AI* (bars with the last description's changed bars lit; live text in Subtractive's three cases, bar changes on one line "Harmonics: 3rd -9.5 dB -> -20.0 dB, ..."), before *Sources*. Texts are the plan's *Theory* with small edits: the "wide" and "breathy" bullets are joined in one paragraph, and "A saw, from sines" has no italics.
- `Guide::additive` summary ends ", words for sounds, and an AI that suggests the settings." (the shared words are "describe ai words bright warm hollow metallic"); the six links are in `TheoryGuideLinks.cpp`; Subtractive's page 2 limits sentence now says "a bell (DawIt FM or DawIt Additive's Stretch)". The editor's guide link and menu item open the page by name (`describePageName`); checked in a shot (g7).
- Tests: two checks next to Subtractive's (search words and summary, links both ways). No page-count check (the pages are UI code, not linked into the tests). SOURCES.md already had the Additive paragraph from Group A, consistent with the page facts, so unchanged.
- Not run: `--export-guides` (it uses the user's settings; the auto-mode classifier refused it). The pages draw in the guide window in a shot; the PDF export of them is for the coordinator to run.
- No facts differ from the plan's *Theory*.

**Group C hand-off (check and docs, built 2026-11-23):**
- Shots (scale 1, 1400 x 780, Keys track, Hollow reed, canned replies; list in the scratchpad `c.json`): (a) text typed; (b) "a hollow clarinet" with h3 -20 dB, h5 -20 dB (a bar group), Brightness -30 dB/oct clamped to -12 and Attack 60 ms; (c) walkthrough step 1 (`"describeWalk": 0`), ring on the bars picture and the 3rd and 5th lit; (d) `describeApplyAll`; (e) `aiSetup: unreachable` with `aiSetupSteps`; (m) a reply of six bars: the step reads "... the 7th to -21 dB and 2 more (lit)"; (f) *Words for sounds* from "A saw, from sines" and (g) *Describe a sound to the AI*, both `"capture": "What is additive synthesis?"`; (h) `pluginEditor` scene of DawIt Additive, no button; (i) Subtractive "a phat lead", compared with `website/assets/screenshots/subtractive-describe-a-sound.png`.
- Departure: plan (b) said h2 Off and h4 Off, but Hollow reed has the even bars Off already, so equal values were dropped; the shots use h3 and h5.
- New for good: `ScreenshotRunner` key `"describeAnswerView": true` shows the answer in the explainer again just before the picture (the real cursor over the window otherwise makes it show the hovered control's topic). Documented in `DevScreenshots.h`.
- Fixed from the shots: Subtractive's answer headings had become the long prompt titles ("Oscillator mix (0% oscillator 1 only...)"; #46 showed "Mix"). `DescribeConfig::getTitle` (optional) gives the window's own control names for the answer and the step line; both editors set it. `additive::formatLevel` no longer says "-0.0 dB". The sentence of *Words for sounds* about the saw read "The saw of A saw, from sines" and now reads "The starting point "A saw, from sines" has it at".
- First steps: checked, no change (the feature lives only in DawIt Additive's window).
