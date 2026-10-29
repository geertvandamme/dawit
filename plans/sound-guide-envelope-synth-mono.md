# What is sound? The envelope on the clip, and a mono check

Status: Built, not committed. Theory checked, with open points
Idea: [What is sound? The waveform explained](../features_done.html#what-is-sound-the-waveform-explained)

## Goal
The *What is sound?* guide closes its three open items. First, the envelope (attack, decay, sustain, release) is measured and drawn on the note under the playhead in the user's clip. Second, a new topic, *Checking in mono*, measures how much a stereo clip loses when left and right are added together, and in which band, and plays the song in mono. Third, the guide's tone is named for what it is: the guide's own simple stand-in for the shapes, with pointers to the synth guides where real synths are heard. The user decided not to play the DawIt Synth from the guide. A beginner learns why a piano and an organ differ beyond their harmonics, and why a mix that sounds full on headphones can go thin on a phone.

## Decisions for you
Decided by the user: the guide keeps its simple tone generator and does not play the DawIt Synth's own waveforms. The text and tooltips say the tone is a stand-in and point to the synth guides, and the idea's bullet is marked as decided against.

1. **Where the mono check lives.** Options: (A) a new topic *Checking in mono* after *Phase cancellation*, with its own picture and a *Hear in mono* switch, which follows the *Taking the vocals out* pattern; (B) a button and a strip inside the *Phase cancellation* topic. **Recommended:** A. The Phase cancellation picture (comb and two wave rows) and its two control rows are already full. The vocals topic already shows the pattern of a topic that plays the song. Phase cancellation's text points to the new topic.
   Answer: A, its own topic.
2. **Where the envelope is drawn.** Options: (A) in the lower half of *The shape is the timbre*, beside the clip's spectrum, as the idea asks; (B) a topic of its own. **Recommended:** A. The timbre text already introduces the envelope, and the harmonics and the envelope of the same note side by side make the point.
   Answer: A, in the Timbre picture.
3. **The mono sum's level.** Options: (L+R)/2, or (L+R)/√2. **Recommended:** (L+R)/2. It is what DawIt Stereo's *Mono* plays and what the Stereo guide's numbers assume, so a sound in the middle keeps its level and the guides agree.
   Answer: (L+R)/2.
4. **Scope: an app-wide mono switch** (on the master strip or the transport, with a shortcut). **Recommended:** not now. The engine's speaker mode is built so that one can be added later (*Left for later*).
   Answer: Not now.

## What the user does and sees
All of this happens in the *What is sound?* window (`SoundExplainer`), which opens from the wave editor and from the Theory Guides window.

- **The shape is the timbre.** The upper half stays as it is. The lower half is split in two. The left part (about 58%) is the existing spectrum. The right part is new: *Envelope*. It shows the level of the note around the focus (playhead or mouse) as a grey line, about 3 s wide, with the A, D, S and R segments DawIt finds drawn over it in the accent colour, each labelled with its time. One line of text under it follows the *envelope rules* (in *Theory*). With no clip in the editor, the lower half uses the made-up example clip and is labelled *Example clip (made up)*. This also applies to the PDF.
- **The tone is a stand-in.** *Play tone* and the shape box keep working as they do now. Their tooltips (in *Theory*) and the Timbre text say the tone is the guide's own plain generator, not a synth, and point to *What is subtractive synthesis?* and *What is additive synthesis?*, where DawIt's synths play these shapes with filters and envelopes. No new control and no new engine path.
- **Checking in mono** is a new topic between *Phase cancellation* and *Taking the vocals out*. Like the vocals topic, it hides the tone controls and shows two controls: a *Hear in mono* toggle and a *Mono in the Stereo guide...* button. That button opens *What is DawIt Stereo?* at *Mono: phones, clubs and the sum*, found by its page name through `MainComponent::openGuide`, with a callback that MainComponent sets, like `onRemoveVocals`.
  - The picture's caption reads *Your clip in stereo and in mono*, with the correlation.
  - On the left: two bars, stereo level and mono level, with the loss in dB.
  - On the right: five bands (*Low: bass and kick, below 150 Hz*; *Low mids 150-500 Hz*; *Mids 500 Hz-2 kHz*; *Upper mids 2-6 kHz*; *Highs above 6 kHz*). Each band has a stereo bar and a mono bar with its loss under them. The band that loses most is drawn in `theme::meterYellow`.
  - Under the bands: the advice from the *mono rules*. Without a clip, the picture shows the example clip.
- **Hear in mono** plays the whole song with (L+R)/2 on both speakers. It follows the rules of *Hear left minus right*: off when the topic changes or the window closes, never in a bounce, meters unchanged. Mono and left-minus-right can never both be on.
- **Tooltips** are in *Theory*. Nothing is remembered between sessions, and nothing is added to the project.

## Data and saving
None.

## The guide
These are changes to the existing guide (recipe *Topics in an existing guide*):
- **The shape is the timbre:** the summary becomes "Harmonics, and how a note starts and ends". The text is new (in *Theory*). Picture: the shape and its harmonics (as now), the clip's spectrum, and its envelope.
- **Phase cancellation:** one bullet changes so it points to *Checking in mono*.
- **Checking in mono** (new): "What's left when left and right add up". Picture: the mono measurement of the clip, or of the example clip. This topic is the guide's "your clip" topic for stereo.
- **Theory Guides entry** (`gen::guides`, `Guide::sound`): the summary becomes "Air moving, waves, loudness in decibels, pitch in hertz, timbre and envelopes, how a sound becomes numbers, phase, and mono." Add search words: `envelope adsr attack decay sustain release mono stereo correlation`. The topics follow `allTopics`. It stays first on *Start here*.
- **Links in from other guides:** the Stereo guide's *Mono: phones, clubs and the sum* page and the Subtractive guide's *Envelopes* page each gain one sentence pointing here (text in *Theory*).

## Code
Recipes: *Topics in an existing guide*, *Checking in the real app*.

- **Envelope analysis** (`soundguide`, pure, tested). It takes audio, a rate and a focus sample, and returns the level contour in 5 ms RMS blocks (both channels' power, at clip gain) around the note, plus what it found:
  - whether a note was found;
  - the onset, peak and end;
  - attack;
  - sustained or fading;
  - for a sustained note: decay, sustain dB (relative to the peak), hold and release;
  - for a fading note: the fade in dB per second.

  How it finds them:
  - **The note:** walk back from the focus (up to 3 s) to a block below -60 dBFS or 40 dB under the later peak, or to the start of a rise of 10 dB or more within 30 ms (the same 10 dB as `describeMoment`). The end is the first block after the peak that is 40 dB under it, the next such rise, or the onset plus 6 s.
  - **Every time is a 10-90% time in amplitude:**
    - attack: from 10% of the peak to 90% of the peak;
    - decay: from 10% to 90% of the way from the peak down to the sustain;
    - release: from 90% of the sustain down to 10% of the sustain.
  - **The body** is from the peak to the release start. If a straight-line fit of its dB falls by 6 dB a second or more, the note is fading. Otherwise it is sustained, and the sustain is the median of the body's middle third.
- **Mono measurement** (`soundguide`, pure, tested), next to `measureLeftMinusRight`:
  - overall: stereo RMS (the mean of L² and R²), mono RMS ((L+R)/2), the loss (always ≤ 0) and the correlation (reuse `stereo::correlation`);
  - per band: power from Hann FFT frames of mid and side (4096 points, 50% overlap), summed over each band's bins. This is FFT rather than filters, so a 4-minute clip measures in well under 100 ms on the message thread.
  - The *most lost* band is the one with the largest loss among bands within 30 dB of the loudest. There is none when no band loses 1 dB.
  - "Stereo" uses the same test as `LeftMinusRight` (two channels, side not 60 dB under).
  - It is measured once per clip, only while the topic shows, like left minus right.
- **Advice text** for both (the rules in *Theory*), as pure functions that return strings.
- **The example clip** (engine, pure): the made-up stereo clip in *Theory*. It is used when no clip is in the editor, by the tests and by the screenshot scene.
- **Engine speaker mode** replaces the `leftMinusRight` atomic. It is one atomic mode (stereo, mono, left minus right) blended by the existing 50 ms `sideOnly`-style smoothing. Mono sends (l+r)/2 to both outputs. `setLeftMinusRight` / `isLeftMinusRight` keep working. The offline rule (`offline` forces stereo) and the untouched meters stay as they are.
- **Risks:**
  - The speaker-mode change is in the audio callback. The existing left-minus-right engine test must still pass unchanged.
  - Adding `Topic::mono` changes the `switch`es in `SoundExplainer`'s picture and the topic-count checks.
  - Moving the vocals topic's index shifts `openGuide` topic numbers. Search for callers that pass a sound-guide topic index (`gen::guides` search results, `ScreenshotRunner`) and use names where possible.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | Mono as (L+R)/2 keeps a sound that's the same in both speakers at its level | derived: (x+x)/2 = x; DawIt Stereo's Mono does the same (`BuiltInStereo.cpp`) | exact |
| 2 | Mono loss against the mean of L² and R² = 10 log10 (mid² / (mid² + side²)), with mid = (L+R)/2 and side = (L−R)/2; never above 0 dB | derived: L² + R² = 2 (mid² + side²) | exact |
| 3 | A sound on one side only loses 3.01 dB | derived: 10 log10 (1/4 ÷ 1/2) | exact |
| 4 | Unrelated left and right of equal level lose 3.01 dB (on average) | derived: the cross term averages 0 | exact |
| 5 | Equal levels with correlation −0.5 lose 6.02 dB; in general 10 log10 ((1+ρ)/2) | derived; agrees with the Stereo guide (`stereo::monoDropDb`, "6 dB at −0.5") | exact |
| 6 | Right = left flipped at half level loses 10.0 dB | derived: mid 0.25, side 0.75; 10 log10 (0.0625/0.625) = −10 | exact |
| 7 | Exactly opposite left and right cancel completely in mono | derived | exact |
| 8 | Correlation between 0 and +1 is generally safe in mono | Stereo guide ("Seeing stereo"), reused | typical |
| 9 | Much listening is mono or close to it: single-speaker phones, many Bluetooth and smart speakers, laptops, shop ceiling speakers, many club subwoofers | Stereo guide; Sound On Sound *10 essential mix checks*; Production Expert *the best way to check mono compatibility* | typical |
| 10 | Many phones now have two speakers (iPhone since the iPhone 7, 2016), close together | GSMArena, Rolling Stone (iPhone 7 launch) | exact (the date), typical (the rest) |
| 11 | A single small speaker is a truer mono check than two speakers playing the sum | Sound On Sound *10 essential mix checks*; Production Expert | opinion |
| 12 | The aim of a mono check is that nothing important vanishes, not that mono sounds as good | Stereo guide; Sound On Sound | opinion |
| 13 | The lowest notes are usually kept in the middle, below about 100-150 Hz | Stereo guide ("Why the low end stays in the middle"), reused | typical |
| 14 | ADSR: attack, decay and release are times; sustain is a level | Subtractive guide ("Envelopes"), reused; a definition | exact |
| 15 | Rise time is usually measured from 10% to 90% of the change | Wikipedia *Rise time*; JEDEC; AMS glossary | exact (a convention) |
| 16 | A straight-line 50 ms attack measures 40 ms from 10% to 90% | derived: 0.8 × 50 | exact |
| 17 | A closed hi-hat starts within a few ms; a slowly bowed violin note can be heard to start 50-100 ms after it physically begins | Wikipedia *Perceptual attack time* | typical |
| 18 | With the start of a note cut off, many instruments become hard to recognise | ASA *Auditory Demonstrations* no. 29, "Effect of tone envelope on timbre" (Houtsma, Rossing, Wagenaars); Berger, JASA 36 (1964) | typical |
| 19 | Newer research finds the attack's rapid transient matters less than once thought; it is one clue among several | Siedenburg (2019), seen only through a summary. Worded without numbers | typical (carefully worded) |
| 20 | Struck and plucked strings (piano, guitar) have no steady sustain: they fade while held | well known; the Subtractive guide's pluck example | typical |
| 21 | An exponential fade with time constant τ falls 8.69/τ dB a second | derived: 20 log10 e = 8.686 | exact |
| 22 | DawIt's synths start from these shapes and shape them with filters and envelopes (Subtractive: saw, square, triangle, sine; Additive: sums of sines) | `BuiltInSubtractive.h` (`sub::Wave`), the Subtractive and Additive guides | exact |
| 23 | The five bands and their edges (150 Hz, 500 Hz, 2 kHz, 6 kHz) | DawIt's choice; 150 Hz is the vocals topic's `lowCutHz` | opinion |

### Topic texts

**The shape is the timbre** — summary: "Harmonics, and how a note starts and ends"

> A piano and a guitar playing the same A at 440 Hz both make the air go back and forth 440 times a second, but they sound different. The difference is the shape of the wave, called the timbre or tone colour.
>
> Every shape that repeats is made of sines: one at the pitch you hear (the fundamental), plus sines at 2, 3, 4 times that frequency and more, the harmonics. The bars on the right show how loud each one is.
>
> A sine is only the fundamental: pure and soft. A triangle adds a few quiet odd harmonics. A square has every odd harmonic, hollow like a clarinet. A saw has all of them, bright and buzzy like strings or brass. Synths start from shapes like these and filter harmonics away. An EQ turns harmonics up or down, which is why it changes the colour of a sound.
>
> The tone you play here is the guide's own plain generator: the bare shape, steady, with no filter and no envelope, so you hear only what the shape does. A real synth starts from the same shapes and then shapes them. DawIt Subtractive filters a saw, square, triangle or sine and gives every note an envelope; DawIt Additive builds a sound from sines, one per harmonic. Hear them in What is subtractive synthesis? (The oscillators) and What is additive synthesis?, in the Theory Guides window.
>
> A sound also changes over the length of a note. That shape is the envelope, usually described in four parts, ADSR:
> - Attack: how long the sound takes to reach full level. A hi-hat or a plucked string gets there in a few milliseconds. You hear a slowly bowed violin note start 50 to 100 ms after it really begins.
> - Decay: how long it then takes to fall to the level it holds.
> - Sustain: the level it holds while the note goes on. A level, not a time.
> - Release: how long it takes to die away at the end.
>
> An organ note jumps to full level, holds it and stops. A piano or guitar note has no steady sustain: it starts hard and keeps fading, even while the key is held. A pad swells in and fades out slowly.
>
> The start matters to our ears. Cut off the first moment of a note and many instruments become hard to recognise, a classic listening demonstration. Newer research finds it is one clue among several, next to the harmonics and how they change during the note.
>
> The envelope picture measures the note at the playhead or the mouse in the waveform editor. The grey line is its level. The coloured lines are the attack, decay, sustain and release DawIt finds, with their times. Each time is measured from 10% to 90% of the way, as engineers measure how fast a signal rises, so it comes out a little shorter than a synth's knob says: a 50 ms straight-line attack measures 40 ms. From the audio alone DawIt can't know when a key was let go, so in a clip the release is how the sound fades out at the end. In a recording of a whole band, notes overlap and the envelope is of everything together. It's clearest on one instrument playing one note at a time.
>
> Pick a shape and click Play tone to hear the difference at the same pitch.

**Phase cancellation**: replace the bullet "- Listen in mono now and then: cancellation shows up as a sound getting thin or disappearing." with:

> - Listen in mono now and then: cancellation shows up as a sound getting thin or disappearing. Checking in mono, next, measures your clip and plays the song in mono.

**Checking in mono** — summary: "What's left when left and right add up"

> A lot of music is played in mono, with left and right added into one: from a phone with one speaker, many Bluetooth and smart speakers, a laptop, the ceiling speakers in a shop, and the subwoofers of many club systems. Phones with two speakers hold them only a hand's width apart.
>
> What's the same in both speakers adds up and stays. What's opposite cancels, as a flipped copy does in Phase cancellation. Everything in between partly stays. So a stereo clip can sound full on headphones and thin on a phone, and nothing warns you.
>
> The picture measures your clip. Stereo is the average level of left and right. Mono is left and right added and halved, as DawIt Stereo's Mono plays it, so a sound in the middle keeps its level exactly. Against that:
> - a sound in the middle loses 0 dB;
> - a sound on one side only loses 3 dB, and so do a left and right that have nothing to do with each other, like two different guitar takes or a wide reverb;
> - a left and right that partly work against each other lose more: 6 dB when they're equally loud with a correlation of -0.5, and 10 dB when one side is the other flipped at half the level;
> - exactly opposite, everything cancels.
>
> The correlation is the number DawIt Stereo's meter shows: +1 when left and right move together, 0 when they're unrelated, -1 when they're opposite. Between 0 and +1 is generally safe in mono.
>
> The bands show where the loss is. A low end that cancels in mono is the worst kind: the bass and kick get thin on phones and disappear from a club's mono subwoofers. That's one reason the lowest notes, below about 100 to 150 Hz, are usually kept in the middle. Highs that drop back in mono, like the air of a wide reverb or cymbals, are usually fine.
>
> Hear it: switch on Hear in mono and play the song. Both speakers then play left and right added together (a bounce is never changed). Listen for anything important getting quieter or vanishing: the vocal, the kick, the bass. Many engineers also check on one small speaker, which is closer to a phone. The aim isn't to make the mix sound as good in mono, only to make sure nothing important disappears.
>
> If something does, flip the polarity of one of two tracks of the same instrument, line up two microphones (Phase cancellation), turn down a stereo widener, or make the low end mono with DawIt Stereo's Mono below. Mono in the Stereo guide goes further.
>
> This measures the clip as its file holds it, not the track's pan, effects or the rest of the mix. Hear in mono plays the whole mix.

**Stereo guide** (*Mono: phones, clubs and the sum*, `StereoGuide.cpp`):
- Replace "from a phone's speaker," with "from a phone with one speaker (phones with two hold them only a hand's width apart),".
- Add a last paragraph: "To measure one clip, open What is sound? > Checking in mono: how many dB it loses in mono, and in which band."

**Subtractive guide** (*Envelopes*): add "What is sound? > The shape is the timbre measures the envelope of a note in your own clip." before "The picture shows...".

**Tooltips**
- Play tone (replaces the current one): "Play the guide's own plain tone, on top of anything else that plays: the bare shape, with no filter or envelope. Change it while you listen."
- Shape box (replaces the current one): "The shape of the wave: the same pitch, a different timbre. DawIt Subtractive plays these shapes as a real synth."
- Hear in mono: "Play the song with left and right added together on both speakers, as a phone or a club's subwoofers play it. Listen for anything that gets thin or vanishes. A bounce is never changed."
- Mono in the Stereo guide...: "Open What is DawIt Stereo? at Mono: phones, clubs and the sum: the correlation meter, what drops in mono, and keeping the low end in the middle."

### Example data
**Example clip** (8 s, 44.1 kHz, stereo). Four notes start at 0, 2, 4 and 6 s, each held 1.5 s. Every part shares one straight-line envelope: A 20 ms, D 200 ms, S 50% (−6.02 dB), R 300 ms.
- Note: a saw (`soundguide::getSample`) at 440 Hz, peak 0.4, the same in both channels. RMS 0.4/√3 = 0.231, power 0.0533.
- Bass: a sine at 55 Hz, 0.2 on the left and −0.1 on the right (flipped at half level). Mean power 0.0125. Mid 0.05, side 0.15.
- Air: noise above 6 kHz, unrelated between the channels, RMS 0.02 on each side (power 0.0004).
- Worked out:
  - stereo power = 0.0533 + 0.0125 + 0.0004 = 0.0662;
  - mono power = 0.0533 + 0.00125 + 0.0002 = 0.0548;
  - **overall loss = 10 log10 (0.827) = −0.83 dB**;
  - correlation = (0.0533 − 0.01) / √(0.0737 × 0.0587) = **+0.66**;
  - **low band −10.0 dB**, mids about 0, **highs −3.0 dB**;
  - envelope: **A 16 ms, D 160 ms, S −6.0 dB, R 240 ms, sustained**.

### "Your clip" rules
**Envelope** (one line under the picture):
- No note found (silence under −60 dBFS at the focus, or no rise and fall): "No single note here. Move the playhead or the mouse onto a note." (Silent clip: "Silence: nothing to measure.")
- A body shorter than 100 ms: "A short sound: attack {A}, gone after {T}. Like a drum hit or a click."
- Fading (fit ≤ −6 dB/s): "Attack {A}, then it keeps fading, about {X} dB a second, with no steady sustain: like a piano, a plucked string or a struck drum."
- Sustained: "Attack {A}, decay {D} down to {S} dB, it holds for {H}, then fades out in {R}." If the peak is less than 1 dB above the sustain: "Attack {A}, no decay: it holds at full level for {H}, then fades out in {R}."
- Added to either case: attack < 10 ms: " A hard start, like a pluck, a hit or an organ key."; attack ≥ 100 ms: " A slow swell, like a pad or a softly bowed string."

**Mono** (under the bands; none when there is no clip, which shows the example's numbers with the example's text):
- Not stereo: "Your clip is mono: left and right are the same, so nothing changes in mono."
- Under 0.1 s or quieter than −60 dBFS RMS: "Too short or too quiet to measure."
- Loss ≥ −1.5 dB: "Mostly in the middle: in mono it's only {L} dB quieter (correlation {ρ}). Nothing important should vanish."
- −1.5 > loss ≥ −4 dB and ρ ≥ 0: "Wide, as stereo music often is: {L} dB quieter in mono (correlation {ρ}). Unrelated left and right lose 3 dB, so this is normal for wide pads, reverbs and doubled guitars. Listen whether the important parts stay clear."
- Loss < −4 dB or ρ < 0: "Parts of this clip work against each other: in mono it loses {L} dB, more than the 3 dB of unrelated left and right (correlation {ρ}). Look for a flipped polarity, a stereo widener, or two microphones out of step (Phase cancellation)."
- Then, if the most-lost band loses ≥ 6 dB:
  - Low: "The low end loses most: {B} dB below 150 Hz. The bass and kick will sound thin on phones and club subwoofers. Try DawIt Stereo's Mono below 120 to 150 Hz on this track."
  - Low mids or Mids: "The {band} lose most ({B} dB), where vocals, guitars and keys sit. Listen in mono whether a part drops away."
  - Upper mids or Highs: "The {band} lose most ({B} dB): wide cymbals, reverbs or a widener. Usually fine; check that the vocal keeps its clarity."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Mono loss, the same signal in L and R | 0.00 dB | 0.01 |
| Mono loss, left only | −3.01 dB | 0.02 |
| Mono loss, R = −0.5 L | −10.00 dB | 0.05 |
| R = −L | stereo; loss ≤ −60 dB | — |
| Unrelated equal noise (10 s) | −3.0 dB, \|ρ\| < 0.05 | 0.2 dB |
| One-channel buffer | not stereo | — |
| 60 Hz with R = −0.5 L, plus 1 kHz the same in both | low band −10.0, mids 0.0, most lost = low | 0.3 dB |
| Example clip | overall −0.83, ρ +0.66, low −10.0, highs −3.0 | 0.2, 0.03, 1.0, 0.5 |
| Made-up envelope (sine 440, A 50 ms, D 200 ms to 0.5, hold 1 s, R 300 ms) | A 40 ms, D 160 ms, S −6.02 dB, R 240 ms, sustained | 6 ms, 10 ms, 0.5 dB, 10 ms |
| Exponential fade, τ = 0.5 s | fading, 17.4 dB/s | 1.5 |
| Example clip, focus at 1.0 s | A 16, D 160, S −6.0, R 240, sustained | 6 ms, 10 ms, 0.5 dB, 10 ms |
| Silence | no note found | — |
| Engine, Hear in mono: L sine 0.5, R silent | both outputs RMS 0.25/√2 | 0.01 |
| Engine: mono then left minus right | only the last one is on | — |
| Engine: offline render with mono on | the plain mix | exact |
| Text | every topic > 400 characters; "adsr", "mono", "envelope" find the sound guide; the Timbre text names "What is subtractive synthesis?" | — |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. Envelope analysis and its text in `SoundGuide.{h,cpp}`, as in *Code* and the *envelope rules*. Tests: the made-up ADSR, the exponential fade, silence (table). Done when the checks pass.
- [x] 2. Mono measurement (FFT bands, correlation, most-lost band) and its advice text, next to `measureLeftMinusRight`. Tests: the mono rows of the table and one advice string per rule.
- [x] 3. The example clip (in `SoundGuide` or `ExampleSong`). Tests: the example clip's mono and envelope numbers.
- [x] 4. Engine speaker mode (stereo, mono, left minus right) in place of the `leftMinusRight` atomic (*Code*). Tests: the engine rows; the existing left-minus-right test unchanged. Done when `./build.sh --test` passes with no warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 5. `Topic::mono` (between `cancellation` and `vocals`), the new and changed texts and summaries, and the `gen::guides` summary and words. The *Checking in mono* picture, the *Hear in mono* toggle (copy `leftMinusRightButton`'s show/hide and reset rules) and the Stereo guide button (a callback set in `MainComponent::showSoundGuide`). Check: the topic count and search tests.
- [x] 6. Timbre's lower half: spectrum and envelope side by side, and the example clip when there's no clip (`updateClip` measures the envelope at the focus, only for this topic). The new Play tone and shape tooltips.
- [x] 7. Stereo and Subtractive guide sentences (*Theory*). A `soundClip` scene in `ScreenshotRunner`: adds the audio track "Example clip" with the example clip and opens it in the wave editor at 1.0 s. Document it in `DevScreenshots.h` and recipes.md's list of scenes. `./build.sh --test` and `--export-guides` pass.

Group C, check and docs:
- [x] 8. Real-app check (`--screenshots`, the `soundClip` scene first with `keepWindows`):
  - (a) `guide: sound`, topic *The shape is the timbre*: the spectrum and envelope with A/D/S/R labels and the sustained text;
  - (b) topic *Phase cancellation*: the changed bullet;
  - (c) topic *Checking in mono*: −0.8 dB, Low band yellow at about −10 dB, and the low-end advice;
  - (d) *Checking in mono* without the scene: the example label.

  Look for clipped labels in the narrowed spectrum, and for the control row fitting at 1000 px.
- [x] 9. Docs:
  - README *What is sound?* bullet (the envelope on the clip, Checking in mono with Hear in mono);
  - docs/tests.md (sound guide area);
  - docs/code-layout.md (the new functions in `SoundGuide`, the speaker mode);
  - architecture.md *How a change travels* step 3: "the sound guide's tone, then the speaker mode (stereo, mono or left minus right)";
  - FEATURE_IDEAS: the envelope and mono-check items move to Done. "Playing the DawIt Synth's own waveforms" is marked as decided against: the guide keeps its own simple tone, a stand-in that points to the synth guides. The idea's status follows the remaining open items.

## Tests
As in the *Test numbers* table: in `runSoundGuideTests` (analysis, mono, example clip, text and search) and next to the existing left-minus-right engine test (speaker mode). `runTheoryGuidesTests` covers the topic count.

## Website hand-off
- Features page, *What is sound?* section: one paragraph on the envelope picture, and one on *Checking in mono* with *Hear in mono*.
- Screenshots: (a) Timbre with the example clip (spectrum and envelope); (b) *Checking in mono* on the example clip with the Low band highlighted. Both use the `soundClip` scene.
- Main page: no change. How it's made: no new library.

## Left for later
- The envelope in the wave editor's hover text ("Attack 12 ms...").
- An app-wide *Listen in mono* switch on the master strip or transport, with a shortcut (Decision 4).
- One-speaker mono (left only), the truer phone check.
- A mono check of the whole mix or a track after its effects (needs an offline render).

## Changes while building

Group A (engine) hand-off:
- `Engine::setMono (bool)` / `isMono()` next to `setLeftMinusRight`; one `speakerMode` atomic (private enum `SpeakerMode`), `sideOnly` and `monoOnly` smoothers. Switching one on switches the other off; `setLeftMinusRight (false)` leaves mono as it is (and the other way round). Bounces stay plain.
- `soundguide::measureEnvelope (audio, rate, focusSample, gain = 1)` returns `Envelope` (times are seconds from the start of the audio given; `contour` is linear 5 ms RMS blocks from `contourStartSeconds`, `blockSeconds` apart; the segments to draw are `attackStart/End`, `decayStart/End`, `releaseStart/End`, valid when `found`, `sustained`, `hasDecay`, `releaseFound` say so). `describeEnvelope (Envelope)` is the line of text. The UI should pass the clip's audio and the focus sample within it (a few seconds around the focus is enough: it looks 3 s back and 6 s forward).
- `soundguide::measureMono (audio, rate)` returns `MonoCheck` (overall, `bands[5]` with stereoDb/monoDb/lossDb, `mostLostBand`, `stereo`, `measurable`, `correlation`); `getMonoBandName (i)`; `describeMono (MonoCheck)` (mono rules, band sentence included). `numMonoBands = 5`.
- `soundguide::makeExampleClip (rate = 44100)` is the 8 s example. Its envelope at 1.0 s: A 16.9 ms, D 154 ms, S -5.9 dB, R 239 ms.
- `Topic::mono` is NOT added yet (Group B).

Changes while building:
- Example clip highs: measured loss is -0.58 dB, not the plan's -3.0 dB. The saw itself has harmonics above 6 kHz (about 0.0018 power, the same in both channels, so they lose nothing) next to the unrelated air (0.0004), so the band loses less than the air alone would. The plan's -3.0 dB (and its "highs about -3 dB" in the screenshot check) is not what a saw gives; the test checks "between -0.2 and -3.5 dB". The other numbers match the plan: overall -0.84 dB, correlation +0.65 (plan +0.66, within tolerance), low band -10.0 dB. The example's mono text therefore reads "correlation +0.65". dawTheory may want to look at the highs figure.
- The envelope contour is smoothed over three blocks (a 55 Hz bass is only a quarter cycle per 5 ms block and made the decay time wobble by about 17 ms); the attack is timed on the unsmoothed blocks so a hard start stays under 10 ms.
- Engine test for mono uses a second `Engine` with a hard-left sine track (pan -1), since the guide's tone is always in the middle.

Group B (UI) hand-off:
- Done: `Topic::mono` (texts, summaries, `gen::guides` entry and words in `SoundGuide.cpp`/`TheoryGuides.cpp`), `SoundExplainer` (`paintMono`, `paintEnvelope`, `monoButton`, `stereoGuideButton`, `onOpenStereoGuide`, example clip via `getExampleClip()` used by `updateClip` for Timbre and Checking in mono when no clip is in the editor), `MainComponent::showSoundGuide` sets `onOpenStereoGuide` (page index found by name with `BuiltInGuides::getPageNames ("DawIt Stereo")`), the Stereo and Subtractive sentences, the `soundClip` scene (`WaveEditor::showSeconds` is new, for it), docs in `DevScreenshots.h` and recipes.md. One new test in `runTheoryGuidesTests`. `./build.sh --test` passes, no warnings; `--export-guides` exits 0.
- Group C: use no `"capture": "window"` on a `guide` shot (it said "opened no window" when combined with `keepWindows` on the first shot); `{"scene":"soundClip"}` first, then `{"guide":"sound","topic":"Checking in mono"}` etc. work (topics by name). I looked at Timbre and Mono once (scratchpad shots); Group C still needs the shots (b), (d) and a check at 1000 px of the control row and the mono bands.
- Not changed: src/ui/TunerWindow.* (the user's).

Changes while building (Group B):
- Timbre's upper half is 44% of the picture, not 50%, to leave room for the envelope's text line. The envelope's plot starts 0.5 s before the onset and is 3 s wide (more for a long note, up to 9 s); the attack label sits to the left of its segment and the decay label under the sustain line, so labels do not meet.
- The spectrum's frequency labels are now "50 ... 10k" (they ran together at the narrower width).
- Example clip shown in Timbre and Mono is a real `AudioClip` ("Example clip (made up)") owned by `SoundExplainer`, so the existing clip code runs unchanged; `ClipData::example` marks it. The print source names it.
- Mono bars show "0.0 dB" for a loss under 0.05 dB (not "-0.0").

Theory check:
- Example clip highs: -0.58 dB is right, not the plan's -3.0 dB. The saw's harmonics 14-50 (6-22 kHz) carry 0.0324/n² each (an ideal saw of peak 0.4), summed, roughly 0.0012-0.0018 in power depending on how much the band-limited saw keeps near Nyquist, the same in both channels, against 0.0004 of unrelated air; that gives -0.4 to -0.6 dB. Read the plan's Example data and Test numbers "highs -3.0" as "highs about -0.6"; no shipped text states the highs figure.
- Correlation: +0.65 (measured 0.651; the plan's sum gives 0.658 with an ideal saw). The example-clip test now expects 0.65 (same tolerance) and its message says +0.65; overall -0.84 dB shows as 0.8, as planned.
- Mono advice "Mostly in the middle": "Nothing important should vanish." became ", so the clip as a whole holds up in mono." It contradicted the low-band sentence that follows on the example clip (-0.8 dB overall but -10 dB below 150 Hz). Test string updated.
- Timbre text: the bowed violin is now "can be heard to start as much as 50 to 100 ms after" (the source says "may be as much as"); "An organ note" became "An electric organ note" and the envelope advice "an organ key" became "an electric organ key" (pipe organs have a slower, breathy start); "even while the key is held" became "even while the piano key is held" (a guitar has no key).
- Phones with two speakers: "only a hand's width apart" became "only a phone's length apart", in Checking in mono and in the Stereo guide's Mono page (top and bottom speakers sit about 12-15 cm apart, more than a hand's width).
