# Describe a sound, and AI sets the synth

Status: Released
Idea: [#46 — Describe a sound, and AI sets the synth](https://github.com/geertvandamme/dawit/issues/46)

## Goal
In DawIt Subtractive's window the user types what they want to hear ("a phat lead", "a warm pad that swells in slowly"). A local model through Ollama picks the nearest starting point and suggests only the knobs to change, each with a reason. DawIt reads and clamps every value with tested code, checks the result against the words, then sets it all at once or walks through the changes one knob at a time. A beginner gets a sound and learns which knobs make it. This plan covers DawIt Subtractive only, using approach 1 from the issue (an existing model, a JSON schema, the starting points as examples).

## Decisions for you
1. **Undo.** Plugin knobs are not in the app's undo history (`undo::Snapshot` leaves plugin settings out), and plugin windows leave Cmd+Z alone. Options: (a) the bar has its own **Undo**, which takes back the last answer or step for as long as the window is open (up to 20 changes); (b) add plugin parameters to the app's history, which is a wide change to undo for every plugin. **Recommended:** (a), because it is small and safe. (b) belongs in its own issue.
   Answer: the recommendation (agreed 2026-11-23).
2. **Keep the reasons?** The last description and its reasons live on the processor while the app runs and are not saved. Saving them in the plugin state would also put them into user presets. **Recommended:** don't save them.
   Answer: the recommendation (agreed 2026-11-23).
3. **When does the sound change?** After an answer nothing changes until the user clicks *Walk me through it* or *Set it all now*. **Recommended:** yes, because the walkthrough needs the old values and the user chooses how to learn.
   Answer: the recommendation (agreed 2026-11-23).
4. **Model choice.** Share `generator.model` and `generator.ollamaUrl` with the Generate panel and the Lyrics window, or give it a setting of its own. **Recommended:** share them, so there is one choice everywhere.
   Answer: the recommendation (agreed 2026-11-23).
5. **Plugin builds.** Hide *Describe a sound* in the VST3/AU, which don't link Ollama code. **Recommended:** hide it. The guide pages say it works in the DawIt app.
   Answer: the recommendation (agreed 2026-11-23).
6. **Walkthrough order.** Follow the window order (oscillators, mixer, filter, filter envelope, amp envelope, LFO, playing) or the order of the guide's *Building a sound* page. **Recommended:** window order, so the eye moves left to right.
   Answer: the recommendation (agreed 2026-11-23).
7. **The next approach (decision rule).** Run the opt-in evaluation (step 5) with qwen2.5:7b. If it scores at least 9 of 12, stop at approach 1. Below that, or when the median answer takes over 15 s on an M1 with 16 GB, re-open #216 (a fine-tuned 0.5 to 3 B model, compared with the same evaluation) before trying approach 3 (embedding plus rules, which needs no Ollama). Use approach 3 first only if you want it to work without Ollama. **Recommended:** this rule.
   Answer: the recommendation (agreed 2026-11-23).

## What the user does and sees
- **Opening it.** A **Describe a sound** button sits in Subtractive's header, right of *Save...*. It is shown only in the app. It opens a bar, 76 px high, across the top of the picture area, and the pictures shrink to fit. The bar's × closes it and leaves the sound as it is.
- **The bar, row 1:** a text box ("Describe a sound: a warm pad that swells in slowly"; Return asks), a *Start from* switch (*Best starting point* / *Current sound*), the model menu and **Ask** (**Cancel** while waiting). After any answer the switch moves to *Current sound*, so the next words are a follow-up. The text box's drop-down offers the last 8 descriptions.
- **Row 2** is the status line. While waiting: "Asking qwen2.5:7b...". When Ollama or the model isn't ready, it shows `aisetup::describe`'s status with *Retry* and *Set up...*; *Set up...* puts `aisetup::getSetupSteps()` in the explainer. After an answer it shows "From Warm pad: 6 changes." with **Walk me through it**, **Set it all now**, **Ask again** (same words, new seed) and **Undo**. Any notes on words or limits come first, in `meterYellow` (see *Theory*).
- **The explainer** shows the answer the way it shows a starting point: the description as the title, the model's summary, "Starting from: Warm pad", then one section per change ("Detune: 9.0 ct → 18.0 ct" and the reason). Above them is the line "Reasons written by the AI model; hover a knob for DawIt's own explanation." DawIt's own notes, such as a clamped value, are marked as DawIt's.
- **The walkthrough** goes in the window's knob order. When the base is a different starting point, the first step is "Load the starting point Warm pad". Each step puts an `accent` ring around the knob or switch and replaces row 2 with "Step 3 of 6 · Detune: turn it to 18.0 ct (now 9.0 ct). <reason>", with **Back**, **Skip**, **Set it for me** and **Finish all**.
  - The user turns the knob. Within 2% of its travel (normalised on the parameter's own range; a choice must be exact), the line shows "Done: you set 17.6 ct" and moves on after 0.8 s. The user's value stays as it is.
  - **Back** returns to the step before and puts its knob back as it was.
  - At the end the line reads "All 6 changes made. Play a note and listen; type a change to refine it."
- **Set it all now** sets every change at once as one Undo step.
- **Tooltips** on every control use the texts in *Theory*. The bar's controls don't take keyboard focus, except the text box. Esc in the text box cancels a request, and otherwise closes the bar.
- **Settings:** `describe.subtractive.recent` (the last 8 descriptions), plus the shared `generator.model` and `generator.ollamaUrl`. The bar is closed each time the window opens.

## Data and saving
Nothing new goes in the project, and the format doesn't change. A sound made this way is ordinary Subtractive parameters, saved as before. The last description (words, base, changes with before and after values, reasons, notes) is kept on the processor, message thread only, and is not saved (decision 2). The guide's live text reads it.

## The guide
New pages go at the end of *What is subtractive synthesis?* (`subtext::makeGuide`), before *Sources*:
1. **Words for sounds**: "Bright, warm, fat, punchy: what they mean on the knobs". The picture is the current sound's harmonics at A3 (`drawSpectrum`), with a vertical line at the spectral centroid labelled "Centre of brightness 1.2 kHz". It is live.
2. **Describe a sound to the AI**: "What the AI does, what DawIt checks, and the walkthrough". The picture is `drawFlow` with the parts the last description changed lit, captioned "Your last description: \"a phat lead\" · 6 changes" ("Describe a sound to see its changes here" when there is none). `liveText` gives the "your sound" part (see *Theory*).

- **Theory Guides entry:** add search words `describe ai words bright warm fat phat punchy` and end the summary with ", words for sounds, and an AI that suggests the knobs". It does not go on *Start here*.
- **Links** in `TheoryGuideLinks.cpp`:
  - sound · The shape is the timbre | subtractive · Words for sounds
  - lyrics · Writing with AI | subtractive · Describe a sound to the AI
- The pages open from the guide button, from the bar's right-click menu (last item, `guideicon::addItem`, "Describing sounds guide...") and from the explainer's answer view.

## Code
Recipes: *A built-in effect or instrument* (the Subtractive editor and guide), *Topics in an existing guide* (pages of an `fxui::Guide`), *Checking in the real app*. For the AI, copy `LyricsEditWindow` + `gen::lyricsai`: the pure request, schema and checks in `src/generate`, the call on `juce::Thread::launch` with a `SafePointer` and `callAsync`, and the result applied on the message thread.

- **`gen::sounddescribe`** (`src/generate/SoundDescribe.{h,cpp}`, both targets) builds the prompts and the schema, picks the examples and reads the reply into a described sound. It also reads values (number plus unit, or a choice name, then clamped to the range with a note), runs the word checks and limit notes, and holds the walkthrough model: steps, target reached, Back, Finish, and an undo list of settings.
- **Knob reference:** `subtext::getTypical` moves to `sub::` (engine), so the window and the prompt share one text. A new `sub::getCentroidHz` (amplitude-weighted, after the filter at the sustain cutoff, or before it) serves the checks and the picture.
- **The described-sound record** is a plain `sub::` type in `BuiltInSubtractive.h`, safe for the plugin build. It holds words, base, changes (knob, before, after, reason, DawIt note), notes and model.
- **Editor:** a static factory hook on `BuiltInSubtractiveEditor`, set in `MainComponent` beside `createEditorFunction`. When the hook is null (in a plugin), there is no button. The editor offers a knob highlight, the bar's area and the described-sound explainer view. The bar itself is app-only (`src/ui/SubtractiveDescribeBar.{h,cpp}`, `DawIt` target only), so the plugin targets never compile `OllamaClient`.
- **Invariants:**
  - Every knob a reply names resolves to one `sub::Knob` or is dropped and counted.
  - Values are always clamped.
  - A change equal to the base value is dropped.
  - When a knob is named twice, the last one wins.
  - At most 10 changes.
  - Applying changes uses `setKnob` or `setSettings` only.
  - Nothing touches the edit or the engine lock.
- **Threading:** the Ollama call runs on a background thread with copies of everything it needs (settings, words, model). A reply arriving after the window closed or a newer request is dropped (request id). Knob changes run on the message thread, as `applyPreset` does: the parameters are atomics the audio thread already reads.
- **Prompt:** temperature 0.3, think off, `num_ctx` as `OllamaClient` sets it. The system prompt holds the rules and the knob table: name, id, range with unit, and the typical text. The user prompt holds the 17 starting points (name, `why`, the values that differ from Init), three full worked examples, two follow-up examples and the current sound. It must stay under 16,000 characters.
- **Risks:**
  - Model quality varies, so the evaluation in step 5 is the gate (decision 7).
  - The `getTypical` move touches the Subtractive UI files.
  - The plugin targets must still link, so `./build.sh --plugins` runs once in Group B1.

## Libraries and models
None new. Ollama is already listed as a separate program (MIT) in `LICENSE` and *About > Licences*. Models are pulled by the user and never shipped. The model DawIt names in its texts and `aisetup` steps, `qwen2.5:7b`, is Apache-2.0 (checked on its Hugging Face card). Don't suggest `qwen2.5:3b` in app text: it is under the non-open `qwen-research` licence. No change to `LICENSE`, `licenses/`, packaging or the README's *License note*. A later fine-tune or embedding model (decision 7) needs its own licence check of the exact weights and of any training tool before it is adopted.

## Theory

### Facts
1. Brightness follows the spectral centroid much better than the centroid divided by F0 (r = 0.51 against 0.03). Schubert & Wolfe 2006, Acta Acustica 92. **Typical** (empirical).
2. Spectral centroid = Σ f·a / Σ a over the harmonics (amplitude-weighted). Definition. **Exact.**
3. An unfiltered saw on A3 up to 20 kHz has 90 harmonics (90 × 220 = 19,800 Hz). Its centroid is 90 × 220 / H₉₀ = 19,800 / 5.0826 = 3,896 Hz. Derived. **Exact.**
4. The centroid of a sine on A3 is 220 Hz. **Exact.**
5. Doubling the cutoff raises it one octave (900 Hz → 1.8 kHz). Definition. **Exact.**
6. Two saws 12 cents apart on A3 beat 220 × (2^(12/1200) − 1) = 1.53 times a second; 9 cents gives 1.15. Derived, and agrees with the guide's 7 cents = 0.89. **Exact.**
7. English and Greek listeners' timbre words fall into three groups: luminance, texture and mass. Zacharakis, Pastiadis & Reiss 2014. **Typical.**
8. "Warm" to experts means a lower, richer middle. Warm and round overlap, and people agree on them less. Rosi et al. (IRCAM; search results in SOURCES). **Opinion.**
9. Fat or thick sounds come from detuned oscillators (5–10 ct gentle, 10–20 ct thick) or an octave layer. Existing `getTypical` and Sound On Sound. **Typical.**
10. Attack is a time, so a longer attack gives a softer start. ADSR definition, as in the existing guide. **Exact.**
11. Pluck: amp sustain 0%, decay 0.2–1 s. Pad: attack 0.3–1 s, release 0.5–2 s. Wobble: 2 or 4 Hz at 120 BPM, depth 50–80%. Vibrato: 5–6 Hz, depth 3–10%. Breath: noise 5–15%. Squelch: resonance 60–80%. All from the existing `getTypical`. **Typical.**
12. DawIt Subtractive plays the same signal in both speakers and has no pitch envelope. From the code. **Exact.**
13. Saw, square, triangle and sine have whole-number harmonics only. A bell's partials are not whole-number multiples. Existing guides (`getHarmonic`, DawIt FM's Chowning bell). **Exact.**
14. A language model writes likely text and does not hear sound. Agrees with the lyrics guide's *Writing with AI*. **Exact** (by construction).
15. The model runs on the user's computer through Ollama at `http://localhost:11434` by default. From the code. **Exact.**
16. Ollama makes the answer follow a JSON schema sent in `format`. Its docs suggest a low temperature and the schema in the prompt too. **Exact.**

### Topic texts

**Words for sounds**: *Bright, warm, fat, punchy: what they mean on the knobs*

People describe sounds with borrowed words: bright, dark, warm, fat, thin, punchy, soft. They aren't measurements, but most of them point at a few knobs. Knowing which ones turns "make it warmer" into something you can do. A study of English and Greek listeners found the same three families of words in both languages: how bright a sound is, its texture (smooth or rough) and its mass (thick or thin).

Bright and dark: how much of the sound's energy sits high up. Listening tests find brightness follows the spectral centroid, the average frequency of the harmonics, weighted by how loud each one is. A plain saw on A3 (220 Hz) has its centroid near 3.9 kHz. A sine on the same note has it at 220 Hz, as dark as a note can be. To go brighter, turn Cutoff up (doubling it, say from 900 Hz to 1.8 kHz, moves it up an octave), use a saw or a square instead of a triangle or a sine, or add filter-envelope Amount. Darker is the opposite. The picture marks the centroid of the sound in the window now.

Warm: a full lower middle and a soft top. Sound engineers mostly mean that, but people agree on "warm" less than on "bright", and "warm" and "round" are often used for each other. On this synth, try a low-pass at about 500 Hz to 2 kHz, little resonance and two saws.

Fat, thick or phat: more than one oscillator at once. A second oscillator detuned by 5 to 20 cents beats against the first, which sounds bigger and alive. At 12 cents on A3 it beats about 1.5 times a second. An octave below adds weight. Thin is the opposite: one oscillator, a narrow pulse or a high-pass.

Punchy, snappy, plucky: how a note starts and ends. Use a fast attack (1 to 5 ms), a short decay, and a filter envelope that opens for the first 100 to 300 ms. A pluck also has Amp sustain at 0%, so it fades even while you hold the key.

Soft, slow or swelling: a longer attack. Attack is a time, so "less attack" usually means a longer attack: 0.3 to 1 s for a pad. A long tail is the release.

Squelchy: high resonance (60 to 80%) with a filter envelope, as in acid bass. Wobbly: the LFO on the filter, on the beat (2 or 4 Hz at 120 BPM). Vibrato: the LFO on the pitch, at about 5 to 6 Hz and small. Airy or breathy: 5 to 15% noise. Hollow or woody: a square wave.

What these words can't mean here: DawIt Subtractive plays the same sound in both speakers, so "wide" means fuller, not wider in stereo. A reverb after it adds space. Reverb, echo and distortion are effects that go after the synth in the track's chain. All of this is common practice among synth programmers, not a set of rules: try the knob, then trust your ears.

**Describe a sound to the AI**: *What the AI does, what DawIt checks, and the walkthrough*

Describe a sound, at the top of DawIt Subtractive's window, lets you type what you want to hear, such as "a warm pad that swells in slowly". A language model then suggests the knob settings, with a reason for each. It's for getting started on a sound and learning which knobs make it; your ears still decide.

How it works: DawIt sends the model the synth's 28 knobs with their ranges and usual settings, the starting points with their settings, and your words. The model runs on your own computer through Ollama, so nothing leaves it. It picks the starting point closest to your words and answers with only the knobs to change, each with a value and a reason. A follow-up such as "a bit brighter" or "softer start" changes the current sound and touches only the knobs it needs.

What DawIt checks: a language model writes likely text; it doesn't hear sound. So DawIt reads every value itself and keeps it inside the knob's range: a cutoff of 40 kHz becomes 20 kHz, and the step says so. It leaves out knobs the synth doesn't have, and it measures the result: if you asked for brighter and the answer is darker on A3, it tells you. It also says when a sound needs something this synth doesn't have: an 808's falling pitch (DawIt Drums), a bell (DawIt FM), a real guitar or clarinet (DawIt Physical) or a piano (DawIt Sampler).

The reasons are the model's own words. Most are sensible, but a model can be wrong and sound sure of itself. Hover a knob to read DawIt's own explanation, and check what you hear.

Walk me through it shows the changes one knob at a time, in the window's order. It lights the knob, says the value and why, and waits for you to turn it (close is fine) or to click Set it for me. Back, Skip and Finish all are there too. Set it all now changes every knob at once. Undo in the bar takes back the last change, whether a whole answer or one step. The app's Undo doesn't cover synth knobs.

Getting the best from it: describe the sound, not the song ("a short bright pluck" rather than "something for my chorus"). Ask for one or two changes per follow-up. Try the same words twice, because each answer can differ. Small models answer faster but miss more often; qwen2.5:7b is a good start.

What it doesn't do: it doesn't listen to a recording to copy its sound, and it only sets DawIt Subtractive so far.

*Live text* (the "your sound" part, after the explanation):
- None yet: "Your last description: none yet. Click Describe a sound at the top of DawIt Subtractive's window."
- In a plugin: "Describe a sound works in the DawIt app, with Ollama on your computer."
- Otherwise, first the line "Your last description: \"<words>\", from <base>: <n> changes, <m> made so far." Then one line per change ("Cutoff: 900 Hz → 1.80 kHz"), then any word notes.

**Tooltips**
- Describe a sound: "Type what you want to hear, and an AI model on your computer suggests the knobs, with a reason for each. Needs Ollama."
- Text box: "What you want to hear, in your own words. After an answer, type a change such as \"a bit brighter\" or \"softer start\". Press Return to ask."
- Start from: "Best starting point: the AI picks the closest starting point and changes as little as possible. Current sound: it changes the sound in the window now, for follow-ups."
- Model: "The Ollama model that answers. The same choice as in the Generate panel and the Lyrics window."
- Ask: "Ask the model. It takes a few seconds; nothing changes until you choose how to apply the answer."
- Cancel: "Stop waiting for the answer. Nothing changes."
- Ask again: "Ask the same words again for a different answer."
- Walk me through it: "Go through the changes one knob at a time: DawIt lights each knob and waits for you to turn it."
- Set it all now: "Change every knob at once. The panel on the right lists each change and why."
- Set it for me: "Turn this knob to the suggested value for you, then go to the next one."
- Skip: "Leave this knob as it is and go to the next one."
- Back: "Go back a step and put that knob back as it was."
- Finish all: "Set every knob that's left now and end the walkthrough."
- Undo: "Take back the last change made here: a whole answer or one step. The app's Undo doesn't cover synth knobs."
- Step text: "The model's reason. Hover the knob for DawIt's own explanation."
- ×: "Close the bar. The sound stays as it is."

**Status texts**
- "Asking <model>... The first answer can take longer while the model loads."
- "From <base>: <n> changes. Walk me through it, or set it all now."
- "The model suggests no changes from <base>. Try other words."
- "The model's answer couldn't be read. Ask again, or pick another model."
- "<n> suggestions named knobs this synth doesn't have and were left out."
- Clamped value: "The model asked for 40 kHz; the knob stops at 20 kHz."
- Unknown base: "The model named no starting point DawIt knows, so it starts from the current sound."

**Prompt rules** (system prompt; dawImplement writes the final wording):
- Use only the listed knobs.
- Give every value with its unit, as the examples show.
- Start from the closest starting point, or from the current sound when asked.
- Change as few knobs as the sound needs, at most 10.
- Give one sentence per change for a beginner: what you'll hear and why.
- If the request needs something this synth lacks (pitch envelope, stereo spread, effects, bell-like inharmonic tones, realistic acoustic instruments), say so in the summary.

**Follow-up examples** for the prompt:
- "brighter", from Warm pad: Cutoff 900 Hz → 1.80 kHz. Reason: "Twice the cutoff, one octave up, lets more of the saws' upper harmonics through."
- "softer start", from Lead: Amp attack 5.0 ms → 60 ms. Reason: "A longer attack fades each note in over 60 ms instead of starting with a click."

### "Your sound" rules
Words are matched as whole words, ignoring case. Notes are advice and never block an answer.

| Words | Condition | Advice | Quiet when |
|---|---|---|---|
| bright, brighter, brightness | follow-up and centroid at A3 after ≤ 0.95 × before | "You asked for brighter, but on A3 the answer is darker: its centre of brightness moves from 1.40 kHz to 1.10 kHz. Try Cutoff up, or ask again." | not a follow-up |
| dark, darker, dull, duller, muffled | follow-up and centroid after ≥ 1.05 × before | "You asked for darker, but on A3 the answer is brighter (<a> to <b>). Try Cutoff down, or ask again." | not a follow-up |
| pluck, plucky, plucked, stab | amp sustain > 30% | "A pluck usually fades even while the key is held: try Amp sustain at 0% and Amp decay between 0.2 and 1 s." | |
| pad, swell, swells, swelling, "fade in", "slow attack" | amp attack < 0.1 s | "A pad usually fades in: try Amp attack between 0.3 and 1 s." | |
| bass | Voices: Poly | "A bass usually plays one note at a time: try Voices: Mono, so overlapping notes slide." | words include chord or chords |
| "softer start", "less attack", "softer attack" | follow-up and amp attack after ≤ before | "You asked for a softer start, but Amp attack didn't get longer. Attack is a time: a longer attack starts more softly." | not a follow-up |

**Limit notes**, one per matching group:
- 808, kick, snare, hihat, hi-hat, drum, drums: "An 808 kick drops in pitch at the start of each note, and DawIt Subtractive has no pitch envelope. DawIt Drums makes 808-style drums; for an 808-style bass here, start from Sub bass with a long Amp decay."
- bell, bells, chime, chimes, glass, glassy, marimba, "electric piano": "Bells and chimes have partials that aren't whole-number multiples of the note, and this synth's oscillators make whole-number harmonics only. DawIt FM makes bells, electric pianos and marimbas."
- guitar, harp, cello, violin, clarinet: "A real guitar, harp, cello or clarinet comes from a vibrating string or tube, and DawIt Physical models those. Here you get a synth version."
- piano (not after "electric"), orchestra, orchestral, choir: "For a real piano or orchestra, DawIt Sampler plays recordings of real instruments."
- reverb, echo, delay, distortion, distorted: "Reverb, echo and distortion are effects: add DawIt Reverb, DawIt Delay or DawIt Distortion after the synth in the track's chain."
- chorus: "DawIt has no chorus effect. For a chorus-like movement here, try a square with the LFO on Width."
- wide, stereo, spread: "DawIt Subtractive plays the same sound in both speakers. Detune makes it sound fuller; a reverb after it adds space."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Centroid, Init saw at A3, before the filter | 3,895.7 Hz | ±0.5% |
| Centroid, sine at A3 | 220 Hz | ±0.1 Hz |
| Warm pad, cutoff 900 → 1,800 Hz | filtered centroid rises | strict |
| `getBeatHz (220, 12)` / `(220, 9)` | 1.530 / 1.147 Hz | ±0.002 |
| Read "1.2 kHz" (cutoff), "800 ms" (amp attack), "60%" (amp sustain) | 1200 Hz, 0.8 s, 0.6 | exact |
| Read "40 kHz" (cutoff), "-30 dB" (output) | 20000 and -24, each with a clamp note | exact |
| Read "lowpass", "pulse", "Off" (glide), "legato", "+7 st", "12 cents", "2.5 oct" | Low-pass, Square, 0, Mono, 7, 12, 2.5 | exact |
| Every preset × knob: read (`formatKnob`) | equals the value | Hz and times 1%, % 0.5, ct/dB/oct 0.05, choices exact |
| Walkthrough target 1,200 Hz cutoff | 1,300 Hz reached; 1,500 Hz not | 2% of travel (ln ratio / ln 1000) |
| Detune target 18 ct | 17.1 reached, 16.9 not | ±1.0 ct |
| Schema knob ids | 28, distinct, one per `sub::Knob` | |
| Schema `startFrom` | 17 preset names + "Current sound" | |
| Prompt length, 200-character description | < 16,000 characters | |

Text checks:
- both new pages have text and a picture, and their names match the links;
- searching "bright", "describe" and "phat" finds the subtractive guide;
- every limit note and advice text is non-empty and names an existing built-in.

## Steps

Group A, engine (`./build.sh --test` at step 5):
- [x] 1. Move `subtext::getTypical` to `sub::` in `BuiltInSubtractive` and update its callers in `SubtractiveGuide.cpp` and `SubtractiveEditor.cpp`. Add `sub::getCentroidHz` (before or after the filter, at the sustain cutoff) and the described-sound record type. Tests: centroid rows, beat rows. Check: the window's text is unchanged.
- [x] 2. Create `src/generate/SoundDescribe.{h,cpp}` (`gen::sounddescribe`; add to both targets) with the knob ids and value reading: units, choice names, clamping with a note. Tests: the read rows, and the round trip over every preset and knob.
- [x] 3. Build the request: the system and user prompts (*Prompt rules*, knob table, starting points, three examples picked by word overlap with the description, defaulting to Bass, Warm pad and Pluck, the two follow-up examples), the schema, and reading a reply into a described sound (base, diff, drops, cap). Tests: the schema rows, prompt length, "a fat acid bass with squelch" picks Acid bass first, equal values dropped, duplicates and caps, an unreadable reply.
- [x] 4. Add the word checks, limit notes and walkthrough model (steps, a load-base step when needed, target reached, Back, Skip, Finish all, undo list of 20). Tests: each *Your sound* row firing and staying quiet, each limit group, the walkthrough rows, Back restoring the value.
- [x] 5. Add the opt-in evaluation, run with `--ollama <model>`. Twelve cases: bass → mono; "deep sub bass" → filtered centroid ≤ 600 Hz; "warm pad that swells in slowly" → attack ≥ 0.3 s and poly; "short pluck" → sustain ≤ 10% and decay ≤ 1 s; "acid squelch bass" → resonance ≥ 50% and amount ≥ 1 oct; "dubstep wobble" → LFO on filter, depth ≥ 30%; "lead with vibrato and glide" → mono, glide > 0, LFO on pitch; "phat lead" → mono and (detune ≥ 5 ct with mix 20–80%, or pitch2 ±12 with mix ≥ 20%); "airy breathy pad" → noise ≥ 5%; follow-ups from Warm pad "brighter" (centroid +10%), from Bass "darker" (−10%), from Lead "softer start" (attack ×2). Print the score and the median time; check ≥ 9 of 12. Done when: `./build.sh --test` passes without warnings.

Group B1, UI: the bar and walkthrough (`./build.sh --test` at step 9):
- [x] 6. Editor: the factory hook (set in `MainComponent`), the header button (hidden when the hook is null), the bar area above the pictures, the knob and switch highlight ring, and the described-sound view in `buildText`. Check: `./build.sh --plugins` still links Subtractive.
- [x] 7. `SubtractiveDescribeBar` (app only): the text box with the recent list, *Start from*, the model menu, Ask/Cancel/Ask again, AI status with *Set up...*, the background request with request ids, and the right-click menu ending with the guide.
- [x] 8. Answer actions: *Walk me through it* (done detection on the editor's tick), *Set it all now*, Undo, the record saved on the processor, every tooltip and status text from *Theory*.
- [x] 9. Add `ScreenshotRunner` keys for a Subtractive plugin shot: `describeText`, `describeReply` (a canned JSON reply, parsed as a model's would be; no Ollama), `describeWalk` (step n), `describeApplyAll`, and the existing `aiSetup` states. Done when: the build passes and the hook is null in the plugin.

Group B2, UI: the guide (`./build.sh --test` at step 11):
- [x] 10. Add the two guide pages with their pictures and live text, the Theory Guides entry's words and summary, and the two links. Tests: the search and link rows; `--export-guides` passes.
- [x] 11. Wire the explainer's "Read about describing sounds" link and the bar menu's guide item to page 2. Done when: the build passes.

Group C, check and docs:
- [x] 12. Real-app check (on the Keys track with DawIt Subtractive added, from preset Warm pad):
  - (a) the bar open with text typed;
  - (b) a canned answer "a phat lead" from Lead with 5 changes, one of them clamped, showing the status, the yellow notes and the explainer;
  - (c) walkthrough step 2 with the ring on Detune;
  - (d) after *Set it all now*;
  - (e) the `aiSetup` unreachable state;
  - (f) guide page *Words for sounds* and (g) *Describe a sound to the AI* after (d);
  - (h) the `pluginEditor` scene, which has no Describe button.

  Check: nothing is clipped at 1400 × 780, the ring sits on the right knob, and the texts fit.
- [x] 13. Docs and tracking:
  - a README Features bullet (Subtractive) and a line in the Ollama section;
  - `docs/tests.md` and `docs/code-layout.md` lines;
  - `docs/architecture.md` *AI generation*: one line on Describe a sound (app-only bar, hook, nothing in undo);
  - `docs/recipes.md` *A built-in effect or instrument*: a note on app-only parts through a hook.
  - Issue #46: tick the walkthrough items and record the scope left. Comment on #216 with the evaluation score.

## Tests
The numbers above, plus:
- an unknown knob is dropped and counted;
- a reply missing `changes` reads as unreadable;
- an unknown `startFrom` falls back to the current sound with the note;
- Finish all sets the remaining targets;
- undo restores the settings from before the answer;
- the guide pages count rises by 2, and every page name in the links exists.

## Website hand-off
- `features.html`, in the synths group: Describe a sound in DawIt Subtractive, the walkthrough and the AI's limits. Shots: (b) and (c).
- `learn.html`: the two new pages of *What is subtractive synthesis?*. Shot: (f).
- `about.html`: no new library. Ollama is already listed.

## Left for later
- Other synths (Additive, FM, Drums, Physical) and *Describe a sound...* on a MIDI track that picks the synth (#46).
- Matching a sound from a recording by rendering and comparing (#46).
- Approaches 2 and 3 under decision 7 (#216).
- Plugin knobs in the app's undo history (its own issue).
- Cloud models (#41).
- A "make it more like a Juno" style knowledge base: models guess at gear names.

## Changes while building

### Group A (engine) hand-off
Built: steps 1 to 5. `./build.sh --test` passes (All checks passed, no warnings). Issue #46 is In Progress on the board (verified).

**Files.** `src/engine/BuiltInSubtractive.{h,cpp}`: `sub::getTypical (int)` (moved from `subtext`, text unchanged; `SubtractiveGuide.{h,cpp}` lost it, `SubtractiveEditor.cpp` calls `sub::getTypical`), `sub::KnobInfo` / `getKnobInfo`, `getKnobValue`, `setKnobValue` (clamps), `getCentroidHz (settings, noteHz, filtered)` (after the filter at the sustain cutoff, or before it), `sub::DescribedChange` and `sub::DescribedSound` (plain data), and a public `BuiltInSubtractive::lastDescription` member (message thread, not saved). New `src/generate/SoundDescribe.{h,cpp}` in both targets. Tests: `runSoundDescribeTests` (about 110 checks) and `runSoundDescribeEval` (only with `--ollama <model>`) in `tests/EngineTests.cpp`.

**For Group B1 (use these by name, namespace `daw::gen::sounddescribe`):**
- Request: fill `Request { words, followUp, current (= processor.getSettings()), seed, model }`, send `buildSystemPrompt()`, `buildUserPrompt (request)`, `buildSchema()` with `temperature` (0.3) and `think` false to `OllamaClient::chat`; read with `readReply (request, reply)`. `Result { ok, error, sound, unknownKnobs, droppedEqual, droppedExtra }`. `result.sound` is a `sub::DescribedSound` (words, base, model, summary, followUp, baseSettings, changes in window order with before/after in the knob's own units, reason and DawIt note; `notes` = status notes in the order: unknown base, unknown knobs, clamp notes, word advice, limit notes; `made` is for the UI to count). The status texts "From <base>: <n> changes...", "no changes", "couldn't be read" are not made here: `Result.error` holds the unreadable text, the rest is the bar's.
- `getResultSettings (sound)` is the base with all changes: "Set it all now" is `setSettings (getResultSettings (sound))` after pushing the live settings on a `SettingsUndo` (max 20; `push`, `pop`, `canUndo`, `clear`).
- `Walkthrough (sound, live)`: `getNumSteps/getIndex/getStep(i)/isFinished`, `Step { loadsBase, knob, before, target, reason, note }` (a `loadsBase` step's `reason` is "Load the starting point <base>"); `isCurrentReached (live)` for the editor's tick, then `markDone (live)` (keeps the user's value), `setForUser (live)`, `skip (live)`, `back (live)`, `finishAll (live)`. They change the `Settings` you hold; call `processor.setSettings (live)` afterwards (or `setKnob` for the one knob). Push to `SettingsUndo` before each step the walkthrough sets.
- `formatKnobValue (knob, value)` shows a number as the knob does; `getKnobId`, `getKnobFromId`, `getKnobTitle`. `pickExamples (words)` is only for tests.
- Walkthrough order is the `sub::Knob` enum order = the window's order (oscillators, mixer, filter, filter envelope, amp envelope, LFO, playing).

**Deviations and details.**
- Singular note: "1 suggestion named a knob this synth doesn't have and was left out." for one; the plan's text for 2 or more.
- The clamp note is built as "The model asked for <what it wrote>; the knob stops at <value>." with ".0" trimmed ("20 kHz", "-24 dB"), as the plan's example.
- A start name is matched case-insensitively, and "Init" matches "Init: a plain saw". In a follow-up `startFrom` is ignored. The base name shown is the starting point's name before the colon ("Init").
- The 16 worked examples (description, summary, two changes each) and their keywords are mine, in `SoundDescribe.cpp`; their reasons are DawIt prompt text, not guide facts. Examples are picked by words in common with the keywords, ties by list order, defaulting to Bass, Warm pad and Pluck.
- The prompt keeps only the first sentence of each starting point's `why` to stay small: 13,376 characters with a 200-character description (limit 16,000).
- Bare JSON numbers in a value are read in the knob's usual unit (Hz, seconds, percent, st, ct, oct, dB).
- No new facts, numbers or theory text were needed beyond the plan's. Not touched: SOURCES.md, LICENSE, guides.

**Evaluation (decision 7), qwen2.5:7b through the local Ollama on this Mac, seed 1 to 12:** 12 of 12 pass, median 4.4 s per answer (range 2.1 to 13.4 s; the first call includes loading the model). So the plan stops at approach 1; no #216 reopening needed on this evidence. Not measured on an M1 with 16 GB, so the 15 s rule is only checked on this machine. Caveat: several cases pass with 0 or 1 changes because the starting point alone already satisfies the check ("bass", "dubstep wobble", "lead with vibrato and glide" had 0 changes), so the 12 of 12 says the model picks the right starting point and keeps it valid, and says little about how well it tweaks. The follow-up cases did change the knobs as asked.

### Group B1 (UI: the bar and walkthrough) hand-off
Built: steps 6 to 9. `./build.sh --test` passes (All checks passed, no new warnings); `./build.sh --plugins` compiled Subtractive's plugin sources without the bar; the VST3/AU link step fails for every plugin (DawIt EQ too) on undefined `GuideFooter` and `gen::guides::sourcesTopicName` symbols from `EffectWindow.cpp`, none from this feature (no symbol of the bar, `OllamaClient` or `gen::sounddescribe` is undefined), so it is a failure that was already there and needs its own fix. Checked in the real app with a shot list (bar with text, answer, walkthrough step with the ring, after Set it all now, the unreachable state, the plugin scene).

**Files.** `src/ui/SubtractiveEditor.{h,cpp}`: `DescribeBarBase` (the plugin-safe base the editor knows: `barHeight` 76, `tick`, `opened`, `closed`), `BuiltInSubtractiveEditor::describeBarFactory` (static hook, null in a plugin), the header button `Describe a sound` (between Save... and the guide button; hidden when the hook is null or the processor runs as a plugin), `setDescribeBarOpen`, `isDescribeBarOpen`, `getDescribeBar`, `setHighlight (knob)` (accent ring drawn in `paintOverChildren`), `showDescription (setupSteps)` (explainer topic 130: the intro when there is no description, the answer, or the Ollama set-up steps), `openDescribeGuide()` (B2: it only calls `openGuide()` for now; point it at the *Describe a sound to the AI* page), `resized` override (moves the guide button right) and the bar area taken from the top of `layoutPictures`. New `src/ui/SubtractiveDescribeBar.{h,cpp}` (DawIt target only, in CMake after `MainComponent.cpp`): text box with the recent list (`describe.subtractive.recent`, 8), Start from switch, model menu (shares `generator.model` and `generator.ollamaUrl`), Ask/Cancel, status view (DawIt's notes in `meterYellow` first, shrinks to fit), Walk me through it / Set it all now / Ask again / Undo / Retry / Set up..., the walkthrough buttons (Back, Skip, Set it for me, Finish all), request ids and stop flag, right-click menu ending with the guide item. `MainComponent` sets the factory and clears it in its destructor. `ScreenshotRunner` keys (plugin shots with `"plugin": "DawIt Subtractive"`): `describeText` (opens the bar; "" is fine), `describeReply` (canned JSON reply as a string) with `describeFollowUp`, `describeWalk` (step 0 up, the earlier steps are set first), `describeApplyAll`, and the existing `aiSetup` (unreachable, noModels, notInstalled) with `aiSetupSteps`.

**Deviations and details.**
- The explainer shows a clamp note once, under its change (not again in the closing list).
- The explainer's answer view has no "Read about describing sounds" link yet: that is step 11 (B2).
- Undo during a walkthrough ends the walkthrough and goes back to the answer (Walk me through it is offered again); `made` is reset to 0 by Undo.
- A request has one attempt (no retry on an unreadable reply); the status then says the answer couldn't be read.
- Set it for me and Finish all push the live settings on the undo list before each change; Set it all now pushes once. Skip and Back do not.
- The bar's own Esc: in the text box it cancels a request, otherwise it closes the bar.
- No new facts, numbers or texts beyond the plan's *Theory*. The editor's intro text for the empty answer view (title "Describe a sound" and two short paragraphs) and the "Set up Ollama" lead line are mine (UI text, no facts beyond the plan); "Cancelled. Nothing changes.", "Undone. ..." and "Hide steps" are mine too.

**For B2.** Add the two pages at the end of `subtext::makeGuide` before Sources; the guide's live text reads `synth.lastDescription` (`words`, `base`, `changes`, `made`, `notes`) and must say "Describe a sound works in the DawIt app, with Ollama on your computer." in a plugin (`fxui::isPlugin`). Wire `BuiltInSubtractiveEditor::openDescribeGuide()` to page 2 (it is called from the bar's right-click menu); add the explainer's link in `buildText` topic 130 (`describeTopic`). `getCentroidHz (settings, noteHz, filtered)` is in `sub::` for the picture of page 1.

### Group B2 (UI: the guide) hand-off
Built: steps 10 and 11. `./build.sh --test` passes (All checks passed, no warnings).

**Files.** `src/ui/SubtractiveGuide.{h,cpp}`: two pages before Sources in `subtext::makeGuide`, *Words for sounds* (spectrum of the sound in the window at A3 with a pink vertical line and label "Centre of brightness <hz>" from `sub::getCentroidHz (s, testNoteHz, true)`; the label flips to the left of the line past the middle) and `subtext::describePageName` = *Describe a sound to the AI* (`drawFlow` lit through the parts the last description changed, one every 1.5 s, caption "Your last description: \"<words>\"" with "<n> changes" on the right, or "Describe a sound to see its changes here"; `liveText` per the plan, with the plugin text). The texts are the plan's, word for word. `src/generate/TheoryGuides.cpp` (words and summary), `TheoryGuideLinks.cpp` (sound · The shape is the timbre | subtractive · Words for sounds; lyrics · Writing with AI | subtractive · Describe a sound to the AI). `SubtractiveEditor`: `openDescribeGuide()` finds the page by name and calls `openGuide (index)`; a flat button *Read about describing sounds* (`describeGuideLink`) sits over the bottom of the explainer on the Describe a sound topic (intro and answer view, hidden for the Ollama set-up lines), shown by `tick()`. `ScreenshotRunner`: `"describeGuide": true` opens the guide at that page. Tests: two rows in the theory-guides block (search "bright", "describe", "phat" and the summary end; the four link directions).

**Deviations.**
- A page's picture lights one part at a time, cycling, because `drawFlow` takes one highlight. Knob to part: oscillator knobs, glide and voices -> oscillators; mix and noise -> mixer; filter knobs -> filter; both envelopes -> envelopes; LFO -> LFO; velocity and output -> amp.
- The live text lists a DawIt note only when it isn't already a change's clamp note (it is shown there).
- The link button is my UI text and tooltip, no facts. No numbers or theory beyond the plan's *Theory*; SOURCES.md already had the section text for both pages (from the planning run); not touched.

- The page's `liveText` returns the explanation followed by the "your sound" part (a guide's `liveText` replaces the page text).
- Checked in the real app: *Describe a sound to the AI* and *Words for sounds* in the `pluginEditor` overlay (pages 12 and 11, 0-based; plugin text shown), and the explainer's *Read about describing sounds* button (`g_explainer.png`). The app's own guide window can't be captured by a shot (`"describeGuide": true` opens it; `capture` can't name it), so the live "your sound" lines with a description are not seen in a picture; they are plain string building from the same data as the explainer. For group C: if you want that shot, add a capture for the guide window.
- `--export-guides` passes (all guides, exit 0).

### Group C (check and docs) hand-off
Built: steps 12 and 13. Status is Built, not committed.

**Real-app check** (shot list `shots4.json` in the scratchpad, 1400 x 780 window, scale 1): the bar with text typed (nothing clipped; the Start from button reads "Best starting point"), the walkthrough step 1 of 5 with the ring on Oscillator 2: Detune and the Back/Skip/Set it for me/Finish all buttons, after *Set it all now* (clamp note in yellow, Ask again and Undo, every knob at its target), the plugin editor (no Describe button), and the app's own guide window on *Describe a sound to the AI* with "Your last description" and "5 changes" (the live "your sound" lines sit below the visible part of the text and were not seen). An earlier run covered the answer, the unreachable Ollama state and the explainer's link. The guide window is captured with `"capture": "What is subtractive synthesis?"` (the window name already worked, so no new ScreenshotRunner code); its keys are now listed in `DevScreenshots.h` and `docs/recipes.md`.

**Docs:** README Features bullet and the Ollama section line, `docs/tests.md`, `docs/code-layout.md` (SoundDescribe, SubtractiveDescribeBar), `docs/architecture.md` (Describe a sound line), `docs/recipes.md` (app-only parts through a hook, the screenshot keys).

**First steps:** checked, no change (the feature is only in DawIt Subtractive's window; the start panel, the inspector's Get started and the First Song Exercise don't mention it).

**Not built / known:** `./build.sh --plugins` still fails to link on main (GuideFooter, sourcesTopicName from EffectWindow.cpp), unrelated and being fixed elsewhere.

**Website hand-off:** `features.html` (synths group): Describe a sound in DawIt Subtractive, the walkthrough, the clamps and word checks, local Ollama; shots: the answer (`b_answer.png` style, from Lead, "a phat lead") and the walkthrough step (`c_walk.png`). `learn.html`: the two new pages of *What is subtractive synthesis?* (*Words for sounds*, *Describe a sound to the AI*); shot: the guide window. `about.html`: no new library (Ollama already listed). Nothing needed beyond the plan's Theory.
