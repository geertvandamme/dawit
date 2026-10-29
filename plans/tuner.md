# A tuner for guitar, bass and other instruments

Status: Built, not committed. Theory checked, with open points
Idea: [A tuner for guitar, bass and other instruments](../features_done.html#a-tuner-for-guitar-bass-and-other-instruments)

## Goal
A Tuner window (View > Tuner..., Cmd+7) listens to an instrument on an audio input and shows the note, how many cents sharp or flat it is and which string it is, with tunings for guitar, bass and ukulele. A string button plays the string's note for tuning by ear. A new guide, *How to tune a guitar*, explains cents and beats, the tunings, the reference pitch, tuning up to the note, why strings drift, tuning by ear, harmonics and low notes, and says whether the tuning suits the song.

## Decisions for you
1. **How close counts as in tune (green).** Options: within 3 cents (yellow to 10, red beyond); within 5 cents, as the idea says. **Recommended:** 3 cents. A string is tuned once and played for the whole song, and its error adds to the other strings' in every chord. A steady plucked string reaches 3 cents easily with a tuner. The guide explains why this is stricter than DawIt Tune's 10 cents for a voice.
   Answer: Recommended option, as asked.
2. **Where the command lives.** Options: View menu, next to Play Along and Sing Along; a new Tools menu. **Recommended:** View, with Cmd+7 (it's free). It's the same kind of practice window as those two, and a menu with one item isn't worth it.
   Answer: Recommended option, as asked.
3. **Where the tuning and reference pitch are saved.** Options: app settings; the project. **Recommended:** app settings for now. A guitarist uses one tuning across songs, and nothing in the edit has to change. Saving a song's tuning with the project is under *Left for later*.
   Answer: Recommended option, as asked.
4. **Mute while tuning.** Options: a switch, on by default, that silences monitored input while the tuner is open; muting the track itself; no muting. **Recommended:** the switch. Monitoring is the only way the input reaches the speakers, and the track's Mute button stays as the user set it.
   Answer: Recommended option, as asked.
5. **The guide's name.** Options: *How to tune a guitar* (the idea); *How to tune an instrument*. **Recommended:** *How to tune a guitar*: that's what a beginner searches for. Bass and ukulele have their own topic and search words.
   Answer: Recommended option, as asked.

## What the user does and sees
**Opening:** View > Tuner... (Cmd+7). It opens through `PluginWindowManager::show` and comes to the front if it's already open. It is resizable: 560 × 440 by default, 460 × 380 at least.

```
┌ Tuner ────────────────────────────────────────────────────────────────┐
│ [Guitar: standard ▾] [Listen: Voice · Input 1 ▾] A4 = [440] Hz [✓ Mute while tuning] [?] │  32 px row
│                                                                         │
│                                A2                    (big, ~72 pt)      │
│          50 flat ────────────────|──▲────────────── 50 sharp            │  needle, zone colour
│               A string: 12 cents flat, tighten it a little              │
│                          in A minor                                     │
│                                                                         │
│      [E2]   [A2]   [D3]   [G3]   [B3]   [E4]     string buttons         │
│  ▮▮▮▮▮▮▯▯▯ input    Play one string at a time and let it ring.          │  level bar + status line
└─────────────────────────────────────────────────────────────────────────┘
```

- **Tuning menu:** Guitar: standard, Drop D, DADGAD, Half a step down, Bass: 4 strings, Bass: 5 strings, Ukulele, Chromatic. There are no string buttons in Chromatic.
- **Listen menu:** first *Selected track's input* (named "Voice · Input 1" from the track's `inputChannel` and `inputStereo`) when the selected track is an audio track, then every device input (*Input 1*, *Input 2*...; a stereo pair is summed to mono). If no audio track is selected, it falls back to the last device input chosen.
- **Reference:** a number box (drag up and down, double-click to type) from 415 to 466 Hz in steps of 0.5 Hz. Double-click it with Alt held to go back to 440.
- **The picture:** the note name, the needle from -50 to +50 cents, the cents in words with what to do, the string, and whether the note is in the song's key. A note that hasn't been heard for a while shows as a dash. **Hold:** the last note stays for 2 s after the sound ends, fading, then turns to a dash.
- **String buttons:** a click locks the tuner to that string and plays its note (a saw tone at -20 dB for 3 s, at the reference pitch, through `Engine::setTone`). The locked button is lit. While locked, the cents are measured against that string even when it's far off ("A string: 2 semitones low, tighten it"). A second click unlocks it, and the tuner goes back to the nearest string. Keys 1 to 6 do the same, counting from the lowest string. Each button's edge has the colour of the string's last reading (green, yellow or red), so you can see which strings are done.
- **Status line** (when there is no note): the four messages in *Theory > Tooltips and messages*, chosen in this order: no inputs open, input exactly zero (`Engine::isInputAllZero`-style, a second of zeros), too quiet (below -50 dB), no clear note.
- **Right-click on the picture:** *Play the reference tone* (the locked or nearest string), *Unlock the string*, then *How to tune a guitar...* last. The **?** button opens the guide too.
- **Mute while tuning:** while the window is open and this is on, monitored input is silent (see *Code*). Recording still captures.
- Space plays and pauses as everywhere (`TransportShortcuts`).
- **Remembered** in app settings: `tuner.tuning`, `tuner.input`, `tuner.referenceHz`, `tuner.mute`. The locked string isn't remembered.

The tooltips are listed in *Theory*.

## Data and saving
Nothing is added to the project, and the format version stays the same. The tuner's settings are app settings (above). The last reading of each string is kept for the session only, so the guide's *Your tuning* topic can list it.

## The guide
**How to tune a guitar**, a new guide following recipes.md *A theory guide* (content `gen::tuning` in `src/generate/TuningGuide.{h,cpp}`, window `TuningExplainer`). Topics, in reading order:

| # | Name | Summary | Picture |
|---|---|---|---|
| 1 | What a tuner shows | The note, how many cents off, and which way to turn | The tuner painter on the live reading, or the example: A string 12 cents flat. |
| 2 | Cents, hertz and beats | Why tuners count in cents, and what two nearly equal notes sound like | 440 Hz plus 442 Hz over 1.5 s: the summed wave with its two swells a second, labelled "2 beats a second". |
| 3 | Standard tuning and other tunings | E A D G B E, and drop D, DADGAD, half a step down, bass and ukulele | The chosen tuning's strings as horizontal lines (thick to thin), each with its note and Hz; strings that differ from standard are marked with the change ("-200 cents"). Clicking a string plays its tone. |
| 4 | Reference pitch: A4 = 440 Hz | What counts as A, and when to change it | A scale from 415 to 466 Hz with marks at 415, 432, 440, 442 and 443, the current reference highlighted and its cents from 440. |
| 5 | How to tune a string | Pluck, read, turn, and why you tune up to the note | Pitch over 6 s: a string starts 30 cents sharp, is loosened to 20 flat, and is brought up to 0. The "from below" part is drawn green, and an arrow says "up to the note". |
| 6 | Why strings drift | New strings stretch, warmth and damp move the wood, and a hard pluck starts sharp | The pitch of one hard pluck over 2 s, starting a little sharp and gliding to the note, labelled "drawn to show the shape, not measured". |
| 7 | Tuning by ear: the fifth fret | Matching each string to the one below, and listening for beats | A neck with the five fretted notes ringed (5th, 5th, 5th, 4th, 5th) and an arrow to the next open string. |
| 8 | Harmonics at the 12th, 7th and 5th fret | Bell-like notes that show a string's octave, and why tuning with them can mislead | The A string vibrating in 2, 3 and 4 parts, with the nodes at the 12th, 7th and 5th fret, labelled 220, 330 and 440 Hz. |
| 9 | Bass, ukulele and low notes | Why a bass's low E takes the tuner longer, and how a ukulele is tuned | The same 85 ms listening window over a 41.2 Hz wave (3.5 cycles) and a 329.6 Hz wave (28 cycles). |
| 10 | Your tuning and your song | Whether the open strings fit the key, low notes, the reference, and each string's last reading | The tuning's strings, each green when its note is in the key and grey when not, with its last reading in cents; below them the capo suggestion when there is one. |

- **Opens from:** the tuner's **?** button and the last item of its right-click menu. It's MainComponent's own guide (like the recording guide): a case in `MainComponent::openGuide` and `getPrintGuide`.
- **Linked from:** the *Piano and guitar chords* guide's guitar text gets one sentence: "Tune first: View > Tuner... (Cmd+7) checks each string, and its guide, How to tune a guitar, explains how."
- **Theory Guides entry:** area `sound` (beside *How recording works*), title "How to tune a guitar", summary "Tuning a guitar, bass or ukulele: cents and beats, the tunings, the reference pitch, tuning by ear and with harmonics, and whether your tuning suits the song.", opens from "The Tuner window (View > Tuner..., Cmd+7)", search words: `tuner tuning tune guitar bass ukulele uke string strings standard drop dadgad half step down eb flat sharp cents hz 440 442 432 415 reference concert pitch harmonic harmonics fret fifth beats peg capo intonation stretch`. Not on *Start here*: not every beginner plays a stringed instrument.

## Code
Recipes: *A window*, *A command or menu item*, *A theory guide*, *Checking in the real app*.

- **Pitch tracker, low notes:** `pitch::Tracker` gets a lowest frequency, set in `prepare`. The default stays 50 Hz (`rate / 50`), so DawIt Tune, `soundguide::estimatePitch` and their tests don't change. The tuner uses 30 Hz with a window of the first power of two of at least 85 ms (4096 at 44.1 and 48 kHz, 8192 at 88.2 and 96 kHz), so the longest lag (30 Hz) is under half the window, which McLeod's method needs. The clarity threshold starts at Tune's 0.93. Lower it only if the decaying-pluck test fails, and never to 0.92 or below: two notes a tritone apart score 0.915 and must stay "no note".
- **Engine, the input tap** (`src/engine/Tuner.{h,cpp}`): while the tuner listens, the audio thread copies one input channel (or the mono sum of a pair) into a lock-free single-reader FIFO. The channel and whether it's listening are atomics. Listening counts in `areInputsWanted()`, like `setInputMeterWanted`, so the device opens its inputs only then. A **mute monitoring** atomic is checked where `Track::process` plays monitored input (`armed && monitor`). The input is silenced there only; capture for recording is untouched.
- **Engine, the listener:** a plain class, with no UI, that drains the FIFO on the message thread (the window's 30 Hz timer), analyses the newest window, and keeps the smoothed reading. Smoothing is the median of the last 5 voiced readings, so a single octave slip never moves the needle. Hold: the last reading stays for 2 s. It also reports level and silence for the status line. All its memory is allocated before listening starts. Tests drive it straight from sample buffers, and through the fake device.
- **`gen::tuning`** (pure, tested): the tunings, each with its strings as MIDI notes and their names (the guitar's standard strings must equal `ChordStyles.cpp`'s `openStrings`); a reading from a frequency, the reference and an optional locked string (note, cents, nearest string, zone, the words); the zones 3 and 10 cents; the guide's topics, texts, example data, and `analyse` for *Your tuning*. Name a string by its note; where a note repeats in a tuning, the lowest copy is "low", the highest "high" and any other gets its octave ("D3 string"). The nearest string is the one nearest in cents; on a tie, the lower one.
- **UI:** `TunerWindow` (`src/ui`), owned through a `SafePointer` in MainComponent. The needle is `tunepics::paintTuner`, extended to take the zone bands and two extra text lines, so the tuner, DawIt Tune and the guide share one look. `TuningExplainer` follows `RecordingExplainer`.
- **Threading:** nothing structural changes. The audio thread only writes the FIFO and reads two atomics. The tracker runs on the message thread, an FFT of 8192 points 30 times a second.
- **Risks:** the change in `Track::process` (audio thread) and in the tracker that DawIt Tune shares. Run `runTuneEngineTests` and the pitch tests unchanged before and after.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | f = 440 × 2^((n − 69) / 12), MIDI note 69 = A4; middle C = C4 = 60 | MIDI 1.0 note numbers; ISO 16 | exact |
| 2 | A4 = 440 Hz is the standard concert pitch | ISO 16 ([A440](https://en.wikipedia.org/wiki/A440_(pitch_standard))) | exact |
| 3 | Some US orchestras (New York Philharmonic, Boston) tune to 442 Hz; most German and Austrian orchestras to 443 Hz; period-instrument baroque groups to 415 Hz | Wikipedia, A440 | typical |
| 4 | 415.30 = 440 × 2^(−1/12); 466.16 = 440 × 2^(1/12) | derived | exact |
| 5 | Cents = 1200 log2(f2 / f1); 100 to a semitone | Ellis; already in DawIt Tune's guide | exact |
| 6 | Standard tuning E2 A2 D3 G3 B3 E4 = 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 Hz; fourths except G to B, a major third | derived from 1; agrees with `ChordStyles.cpp` `openStrings` (40 45 50 55 59 64) | exact |
| 7 | Drop D: D2 A2 D3 G3 B3 E4; DADGAD: D2 A2 D3 G3 A3 D4 (open strings make Dsus4); half a step down: Eb2 Ab2 Db3 Gb3 Bb3 Eb4 | [Drop D](https://en.wikipedia.org/wiki/Drop_D_tuning); [Guitar Player, DADGAD](https://www.guitarplayer.com/lessons/dadgad-for-dummies-you-already-know-50-percent-of-this-tuning) | exact |
| 8 | Bass E1 A1 D2 G2 = 41.20, 55.00, 73.42, 98.00 Hz; 5-string adds B0 = 30.87 Hz | standard bass tuning; Hz derived | exact |
| 9 | Ukulele G4 C4 E4 A4 (392.00, 261.63, 329.63, 440.00 Hz), re-entrant; low G is G3 | [GretschTech: reentrant tuning](https://blog.gretschguitars.com/2014/01/gretschtech-reentrant-tuning/) | exact |
| 10 | Beats a second = the difference in Hz: 440 vs 442 Hz = 2 a second | physics (Rossing, *The Science of Sound*) | exact |
| 11 | 5 cents at 110 Hz = 0.318 Hz (a beat every 3.1 s); at 440 Hz = 1.27 Hz; 1 cent at 440 Hz = 0.254 Hz (one every 3.9 s) | derived: f × (2^(c/1200) − 1) | exact |
| 12 | 10 cents is 0.47 Hz at 82.41 Hz and 1.90 Hz at 329.63 Hz | derived | exact |
| 13 | A string's frequency is proportional to √tension, so 1% more tension = 600 log2 1.01 = 8.61 cents | Mersenne's law (Rossing) | exact |
| 14 | Harmonics: touched at 1/2, 1/3, 1/4 of the length (12th, ~7th, ~5th fret): 2×, 3×, 4× the open frequency | ideal string (Rossing) | exact |
| 15 | Tuning by matching 5th- and 7th-fret harmonics leaves each string 1.955 cents flat (a pure fourth is 498.045 cents); A −1.96, D −3.91, G −5.87; the B from G's 4th-fret harmonic (5:4, 386.31 cents) is 13.69 cents flat of G, so −19.55 overall | derived | exact |
| 16 | 5th-fret method (4th fret on G for B); its errors build up | [Wikibooks, Tuning the Guitar](https://en.wikibooks.org/wiki/Guitar/Tuning_the_Guitar) | exact (method), typical (errors) |
| 17 | E2 × 2^(5/12) = 110.00 Hz (5th fret of the low E = open A) | derived | exact |
| 18 | Two steady tones one after the other can be told apart at roughly 3 to 10 cents | as DawIt Tune's guide (*Cents: how far out is out*); they agree | typical |
| 19 | Tune up to the note: gear backlash and slack at the nut can let a string tuned down drift flat | Wikibooks; [Sweetwater tuning tip](https://www.sweetwater.com/insync/guitar-tuning-tip/) | opinion (common practice) |
| 20 | New strings stretch and keep going flat until they settle; pull them gently and retune | Wikibooks | typical |
| 21 | Temperature and humidity change the strings and the wood; the pitch can drift either way | guitar forums and makers' advice, which disagree on the direction | typical |
| 22 | A hard pluck starts sharp and the pitch glides down (tension modulation) | [Bank et al., DAFx 2009](https://home.mit.bme.hu/~bank/publist/dafx09/index.html) | exact (effect); size not stated |
| 23 | 432 Hz is 31.77 cents below 440; claims that it is healthier or more natural rest on few, small studies | derived; [Padua systematic review](https://thesis.unipd.it/bitstream/20.500.12608/59910/1/Marmolaro%20Mattia.pdf) | exact / opinion |
| 24 | 442 Hz reference = 7.85 cents above 440 | derived | exact |
| 25 | Listening window 4096 at 48 kHz = 85.3 ms (92.9 at 44.1 kHz); E1 period 24.27 ms (3.5 cycles); B0 32.40 ms (2.6 cycles); DawIt Tune's 2048 = 42.7 ms, from 50 Hz | derived from DawIt's tracker | exact |
| 26 | Green within 3 cents, yellow within 10 | DawIt's choice | opinion |
| 27 | On a floating tremolo bridge, retuning one string moves the bridge and the others | mechanics (the bridge balances strings against springs) | typical |
| 28 | Comparing the 12th-fret note with the 12th-fret harmonic checks intonation | common setup practice, not separately sourced: worded as a check, not a rule | typical |
| 29 | Capo on fret 1 in standard tuning: open strings F Bb Eb Ab C F, all in Eb major | derived | exact |

### Topic texts

**1. What a tuner shows**: *The note, how many cents off, and which way to turn*

A tuner listens to one note and tells you three things: which note it is, how far it is from that note, and which way to go. Play the A string of a guitar and the tuner shows A2. If it says 12 cents flat, the string is a little low: tighten it. If it says 8 cents sharp, it's a little high: loosen it.

A cent is a hundredth of a semitone, the step from one fret to the next. The needle runs from 50 cents flat on the left to 50 cents sharp on the right. Past 50 cents the note is nearer the next semitone, so the tuner shows that note instead.

DawIt's tuner turns green within 3 cents of the note, yellow within 10 and red beyond. These bands are DawIt's choice, not a standard. They're stricter than the 10 cents DawIt Tune allows a singer, because a string is tuned once and then played for a whole song, and its error adds to the other strings' in every chord.

With a tuning chosen, such as Guitar: standard, the tuner also names the nearest string: "A string: 12 cents flat, tighten it a little". Click a string's button to tune only that string. The tuner then measures against that string even when it's far off, and plays its note so you can also tune by ear.

Play one string at a time and let it ring. A chord, or two strings ringing together, has no single pitch, and the tuner shows a dash. A tuner checks the open strings only. Whether the guitar is still in tune higher up the neck depends on its setup: see *Why strings drift*.

**2. Cents, hertz and beats**: *Why tuners count in cents, and what two nearly equal notes sound like*

Pitch is how many times a second a string vibrates, counted in hertz (Hz). A guitar's A string vibrates 110 times a second: 110 Hz. The A an octave up is 220 Hz, and A4, the A above middle C, is 440 Hz. Every octave doubles the number.

A semitone is split into 100 cents, and an octave is 1,200. Cents measure a ratio, so a cent is the same small step at any pitch. That's why tuners count in cents. 10 cents flat is 0.47 Hz at the low E (82.41 Hz) and 1.90 Hz at the high E (329.63 Hz), and it sounds just as far out on both.

When two notes are almost the same, you hear beats: the sound swells and fades as the two waves fall in and out of step. The number of beats a second is the difference in hertz. 440 Hz against 442 Hz beats twice a second. As the two notes come closer the beats slow down, and when they stop, the notes match.

Low notes beat slowly. Two A strings 5 cents apart, at 110 and 110.3 Hz, beat once every 3 seconds, which is easy to miss. Their higher harmonics beat faster: at 440 Hz the same 5 cents beats 1.3 times a second. So when you tune by ear, listen to the bright, high part of the sound.

How small a difference can you hear? Two steady notes played one after the other can be told apart when they're roughly 3 to 10 cents apart, depending on the pitch, the loudness and the listener. Held together it's easier: even 1 cent at 440 Hz makes a slow beat, one every 4 seconds.

**3. Standard tuning and other tunings**: *E A D G B E, and drop D, DADGAD, half a step down, bass and ukulele*

A guitar in standard tuning is tuned, from the thickest string to the thinnest, E2 A2 D3 G3 B3 E4: 82.4, 110.0, 146.8, 196.0, 246.9 and 329.6 Hz. Each string is a fourth (5 semitones) above the one before, except G to B, a major third (4 semitones). That one shorter step is what lets the common chord shapes fit under the fingers. A way to remember the strings: Eddie Ate Dynamite, Good Bye Eddie.

Other tunings change some of the strings:
- **Drop D:** the low E goes down a whole step to D2 (73.4 Hz): D A D G B E. A power chord is then one finger across the lowest three strings, and the guitar reaches a whole step lower. It's common in rock and metal.
- **DADGAD:** D2 A2 D3 G3 A3 D4. Three strings come down a whole step, and the open strings ring as a Dsus4 chord. It's heard a lot in folk and Celtic music.
- **Half a step down:** every string a semitone lower: Eb Ab Db Gb Bb Eb. The same shapes sound a semitone lower, which can suit a singer, and the slacker strings are a little easier to bend.
- **Bass:** E1 A1 D2 G2 (41.2, 55.0, 73.4 and 98.0 Hz), an octave below the guitar's four lowest strings. A five-string bass adds a low B0 at 30.9 Hz.
- **Ukulele:** G4 C4 E4 A4 (392.0, 261.6, 329.6 and 440.0 Hz). The G string is higher than the C next to it. This is called re-entrant tuning, and it gives the ukulele its bright, close sound. A low-G ukulele has a thicker string tuned an octave lower, to G3.
- **Chromatic:** no strings. The tuner names any note, for any instrument.

Tuning a string down a step or two is safe. Don't tune a string far above its usual note: the extra tension can break it.

**4. Reference pitch: A4 = 440 Hz**: *What counts as A, and when to change it*

Before a tuner can say a note is in tune, it needs a starting point: the pitch of A4, the A above middle C. Every other note is worked out from it. Almost all music recorded today uses A4 = 440 Hz, the concert pitch set as a standard in ISO 16. DawIt's instruments play at 440 Hz too, so leave the tuner at 440 Hz to play along with your song.

Some music is tuned differently. Many orchestras tune a little higher: some American orchestras to 442 Hz, and most in Germany and Austria to about 443 Hz. Groups playing baroque music on old instruments often use 415 Hz, a semitone below 440. An old recording that was sped up or slowed down, or a band that tuned to its own piano, can sit anywhere in between.

To play along with such a recording, set the reference to match it. Moving it from 440 to 442 Hz raises every note by 7.9 cents. The tuner's range, 415 to 466 Hz, runs from a semitone below 440 to a semitone above.

You may read that 432 Hz sounds more natural or is better for you. The studies behind such claims are few and small, and there's no established reason to prefer it. It is simply 31.8 cents lower than 440 Hz. If you tune to it, your guitar will sound flat against DawIt's instruments and most recordings.

**5. How to tune a string**: *Pluck, read, turn, and why you tune up to the note*

1. Play the open string once, at the strength you normally play, and let it ring.
2. Read the tuner once the needle settles. A string plucked hard sounds slightly sharp at first and settles as it rings (see *Why strings drift*).
3. If it's flat, tighten the string by turning its tuning peg. If it's sharp, loosen it.
4. Tune up to the note. If the string is sharp, loosen it until it's a little flat, then tighten it up to the note. Coming from below, the peg's gears and the string over the nut settle under tension. Coming down from above, a little slack can stay caught and let go later, and the string drifts flat. Many players tune this way; with very good tuning pegs it matters less.
5. Turn a little at a time. A tiny turn makes a big change: the pitch follows the square root of the string's tension, so just 1% more tension raises the note by 8.6 cents.
6. Go round all the strings, then go round again. Tuning one string changes the pull on the neck a little. On a guitar with a floating bridge (a tremolo arm that can move the note up as well as down) it moves the bridge, and the other strings shift with it. The second round is quick.

Hold the guitar as you play it while you tune. At the end, play a chord you know, such as an open E major or G major: it should sound clear, without a slow wobble.

**6. Why strings drift**: *New strings stretch, warmth and damp move the wood, and a hard pluck starts sharp*

A guitar that was in tune yesterday is often out today. The usual reasons:
- **New strings stretch.** For the first hours of playing, and sometimes a few days, new strings stretch and settle around the pegs and the bridge, so they keep going flat. Tune them, gently pull each string away from the fretboard along its length, and tune again. Repeat until the pitch stops dropping.
- **Temperature and humidity.** Steel strings, and the wooden neck and body, grow and shrink with warmth and damp. A guitar brought in from a cold car, or played under hot stage lights, drifts as it warms up, flat or sharp depending on the guitar. Let it reach the room's temperature, then tune.
- **Playing.** Big string bends, heavy strumming and a tremolo arm all pull on a string, and a string can slip a little on its peg or catch in the nut.
- **A hard pluck starts sharp.** Plucking hard stretches the string a little more while it swings wide, so the first moment of a note is slightly sharp, and the pitch glides down as the note fades. Pluck at the strength you play, and read the tuner once the note has settled.

When the open strings are in tune but chords high up the neck sound out, the strings may not be the problem. Compare the note pressed at the 12th fret with the harmonic at the 12th fret (see *Harmonics*). They should be the same note. If the pressed note is sharp or flat, the guitar's intonation needs adjusting at the bridge. That's a setup job, for a guitar technician or for a careful evening with a setup guide.

**7. Tuning by ear: the fifth fret**: *Matching each string to the one below, and listening for beats*

Without a tuner, or to check one, you can tune the strings to each other. First tune the low E to something you trust: a tuner, a piano, or the note the tuner's E2 button plays. Then:
- Press the low E string at the 5th fret. That's A2, 110 Hz. Tune the open A string to it.
- A string, 5th fret: D3. Tune the open D string to it.
- D string, 5th fret: G3. Tune the open G string to it.
- G string, **4th** fret: B3. Tune the open B string to it. G to B is a major third, one fret less than the other steps.
- B string, 5th fret: E4. Tune the open high E string to it.

Play both notes together and listen for beats, the slow wobble of two nearly equal notes. Turn the open string until the wobble slows down and stops.

Your guitar is then in tune with itself, which is all you need when you play alone. To play with a recording or with other people, the low E must match their pitch too. Each step can add a small error, and over five steps the errors add up. So check the high E against the low E: two octaves apart, they should sound as one.

**8. Harmonics at the 12th, 7th and 5th fret**: *Bell-like notes that show a string's octave, and why tuning with them can mislead*

Touch a string lightly, right above a fret wire, without pressing it down. Pluck it, then lift your finger: you hear a soft, bell-like note, a harmonic. Your finger stops the string moving at that point, so it can only vibrate in equal parts.
- At the 12th fret, halfway along, it vibrates in two halves: twice the open string's frequency, an octave up. On the A string that's 220 Hz.
- At the 7th fret, a third of the way along, in three parts: three times, an octave and a fifth up. On the A string, 330 Hz.
- At the 5th fret, a quarter of the way along, in four parts: four times, two octaves up. On the A string, 440 Hz.

A harmonic rings long and clear, which makes it easy to hear and easy for a tuner to read.

A popular shortcut tunes neighbouring strings by matching harmonics: the low E's 5th-fret harmonic (4 × 82.41 = 329.6 Hz) against the A string's 7th-fret harmonic (3 × 110 = 330 Hz). On a correctly tuned guitar these two aren't quite equal. A guitar is tuned in equal temperament, where every semitone is the same size, while harmonics give pure intervals, which are slightly different. Matching them leaves the A string 2 cents flat, the D 4 cents and the G 6. Tuning the B from the G string's 4th-fret harmonic is worse: that harmonic is a pure major third, which is 14 cents smaller than the guitar's, so the B ends up about 20 cents flat in all.

So use harmonics to check octaves at the 12th fret, or to give a tuner a clean note. Tune the strings themselves to a tuner or with fretted notes, and the B string always that way.

**9. Bass, ukulele and low notes**: *Why a bass's low E takes the tuner longer, and how a ukulele is tuned*

A bass's lowest string, E1, vibrates 41.2 times a second, so one vibration takes 24.3 milliseconds. DawIt's way of finding a pitch compares the sound with itself one vibration later, so it needs at least two whole vibrations in what it hears. The tuner listens to about 85 milliseconds at a time (4,096 samples at 48 kHz): three and a half vibrations of the low E, and more than two and a half of a five-string bass's low B (30.9 Hz). DawIt Tune, made for voices, listens to about 43 ms and stops at 50 Hz. That's why a bass needs the tuner.

Low strings are also hard to hear. Laptop microphones and small speakers barely pick up 41 Hz, so what reaches the tuner is mostly the string's harmonics. The tuner still finds the right note, because the harmonics repeat at the string's own rate. Still, plugging the bass straight into the audio interface's instrument input gives the steadiest reading. Tuning a bass by ear is easier with the harmonics at the 12th fret: they're an octave higher, so the beats come twice as fast.

A ukulele's strings are G4 C4 E4 A4, and the G is higher than the C beside it. Played in order from the G string, they sound the tune of "My Dog Has Fleas". To tune them to each other, start from the C string: its 4th fret is E (tune the E string to it), the E string's 5th fret is A (the A string), and the C string's 7th fret is G4, the high G string's note.

**10. Your tuning and your song**: *Whether the open strings fit the key, low notes, the reference, and each string's last reading*

This page compares the tuning chosen in the tuner with your song. It says whether the open strings are in the song's key, whether your bass or guitar part goes lower than the lowest string, whether the reference pitch matches DawIt's instruments, and how far each string was off when the tuner last heard it.

Open strings matter because they ring: in open chords, and when a string is left ringing by accident. In A minor, all six open strings of standard tuning (E A D G B E) are in the key, which is one reason A minor and E minor are so comfortable on a guitar. In Eb major only D and G are. Many players then use a capo: clamped on the 1st fret, it makes the open strings F, Bb, Eb, Ab, C and F, all in Eb major, and you play the shapes of D major.

A deliberate drop tuning and a string that has slipped can look the same on a tuner. If the tuner says your low string is D and your song is in D, that's probably drop D on purpose. If it's somewhere between D and E, it slipped.

(The analysis follows, from the rules below.)

### Tooltips and messages
- **Tuning menu:** "Which instrument and tuning to tune to. In a tuning, the tuner names the nearest string; Chromatic names any note."
- **Listen menu:** "The input the tuner hears: the selected audio track's input, or any input of your audio interface. Plug your instrument into it."
- **Reference:** "The pitch of A4 that every note is measured from. 440 Hz is the standard, and what DawIt's instruments play. Drag, or double-click to type, from 415 to 466 Hz."
- **Mute while tuning:** "Silences the input you hear through Monitor while the tuner is open, so tuning noise and feedback stay out of the speakers. The track's own Mute button isn't touched, and recording still records."
- **String button:** "E string, E2 (82.4 Hz). Click to tune only this string and hear its note; click again to let the tuner pick the nearest string. Keys 1 to 6 do the same."
- **Picture:** "The note you play and how far off it is: green within 3 cents, yellow within 10, red beyond. Right-click for the reference tone and the guide."
- **Level bar:** "How loud the input is. Below the mark (-50 dB) it's too quiet for the tuner to hear a note."
- **? button:** "How to tune a guitar: cents and beats, tunings, the reference pitch, tuning by ear and with harmonics."
- **Command:** "Tuner...": "Listen to a guitar, bass or other instrument on an audio input and see which note it plays and how far off it is."
- **Status messages:** "No audio input is open. Choose your audio interface in Audio / MIDI Settings (Cmd+,)." · "The input is silent. Is the instrument plugged in, and may DawIt use the microphone? (System Settings, Privacy & Security, Microphone)" · "Too quiet to hear a note. Turn up the input gain on your audio interface, or play closer to the microphone." · "No clear note. Play one string at a time and let it ring."
- **Reading words:** within 3 cents, "in tune"; up to 15, "N cents flat, tighten it a little" or "N cents sharp, loosen it a little"; up to 50, without "a little"; beyond 50 (locked string only), "N semitones low, tighten it" or "N semitones high, loosen it", rounded to whole semitones. One cent is "1 cent".

### Example data
- Topic 1: A2 at 109.24 Hz = 12 cents flat (110 × 2^(−12/1200) = 109.239).
- Topic 2: 440 + 442 Hz, 1.5 s, envelope |2 cos(π × 2 × t)|: loudest at 0, 0.5, 1.0 and 1.5 s, silent at 0.25, 0.75 and 1.25 s (two swells a second).
- Topic 5: pitch path (s, cents): (0, +30) (1.5, +30) (2.5, −20) (3.5, −20) (5, 0) (6, 0).
- Topic 6: glide cents(t) = 6 × e^(−t / 0.3) for 0 to 2 s, labelled as drawn, not measured.
- Topic 8: A string 110 Hz: 220, 330, 440 Hz; nodes at 1/2, 1/3, 1/4.
- Topic 9: 85.3 ms over 41.20 Hz (3.51 cycles) and 329.63 Hz (28.1 cycles).
- Topic 10 on the example song (A minor, Guitar: standard): 6 of 6 strings in the key.

### "Your tuning" rules (in this order)
1. **Key fit** (always, except in Chromatic): "{n} of the {m} open strings ({list}) are in {key}." When n < m, add "{others} aren't: let them ring only when you mean to." When n = m, add "Open chords and ringing strings will fit."
2. **Capo** (guitar and ukulele tunings only): when n ≤ m / 2 and a capo on fret k (1 to 5, the lowest k with the most strings in the key) gives at least m − 1: "Try a capo on fret {k} and play the shapes of {key moved down k semitones}: the open strings become {notes}, {n'} of them in {key}." Quiet otherwise. Eb major, standard tuning: n = 2, capo 1 gives 6 ("the shapes of D major").
3. **Low notes:** a MIDI track whose name has a bass word (bass tunings) or a guitar word (guitar tunings), with its lowest note below the lowest string: "The {track} track goes down to {note} ({Hz} Hz), below the lowest string, {string}. Tune that string down to {note} ({d} semitones) or use {a five-string bass / a drop tuning}." Quiet when there's no such track, or nothing lower.
4. **Reference:** when it's not 440 Hz: "Your reference is A4 = {f} Hz, {c} cents {above/below} DawIt's instruments, which play at A4 = 440 Hz. Tuned to it, your instrument will sound {sharp/flat} against the song. Use 440 Hz unless you play along with a recording or musicians tuned differently." Quiet at 440.
5. **Last readings:** "Last heard: E 2 cents flat, A in tune, ..." for the strings heard this session; when none: "Open the tuner (Cmd+7) and play each string once: this page then shows how far each one was."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Standard tuning Hz | 82.4069, 110.000, 146.832, 195.998, 246.942, 329.628 | 0.001 |
| Guitar standard strings = `openStrings` | 40 45 50 55 59 64 | exact |
| Bass E1, B0; ukulele G4 | 41.2034, 30.8677, 391.995 | 0.001 |
| Sine 110 Hz (pure function and through the listener) | A2, 0 cents | 1 cent |
| Sine 81.4604 Hz, Guitar: standard | low E string, −20 cents, "flat, tighten it" | 1 cent |
| Sine 41.2 Hz, Bass: 4 strings | E1, E string | 2 cents |
| Sine 30.87 Hz, Bass: 5 strings | B0 | 2 cents |
| 440 Hz with reference 442 | A4, −7.85 cents | 0.05 (function), 1 (listener) |
| Decaying pluck (harmonics 1 to 8 at 1/n, 1.5 s decay) at 82.4069 Hz, read after 150 ms | E2 | 2 cents |
| Silence; white noise at −20 dB; E2 + F2 together; E2 + A#2 (a tritone) together | no note | — |
| Locked A string, sine 98 Hz | "2 semitones low" | exact text |
| Zones: 2.9 / 3.1 / 9.9 / 10.1 cents | in tune / close / close / out | — |
| Harmonic method | 1.955 cents per step; B −19.55 overall | 0.01 |
| 1% more tension | 8.61 cents | 0.01 |
| Beats 440 vs 442 Hz; 5 cents at 110 Hz | 2.0 Hz; 0.318 Hz | 0.001 |
| 432 Hz vs 440 | −31.77 cents | 0.01 |
| Key fit: standard in A minor; standard in Eb major; capo | 6 of 6; 2 of 6; capo on fret 1, 6 of 6 | exact |
| Low notes: a "Bass" MIDI track down to D1, Bass: 4 strings | advice names D1 and E string | text |
| Text | every topic has a name, summary, text and example; search "drop d", "ukulele", "432" and "tuner" find the guide | — |

## Steps

Group A, engine (`DawItTests` build at step 3):
- [x] 1. `pitch::Tracker`: a lowest frequency set in `prepare` (default 50 Hz) and a tuner window size (≥ 85 ms). Tests: 41.2 and 30.87 Hz are found with the long window; DawIt Tune's and the pitch tests pass unchanged. Done when Tune's tests are untouched and green.
- [x] 2. `src/engine/Tuner.{h,cpp}`: the input tap (atomics, FIFO, counted in `areInputsWanted`), the mute-monitoring flag in `Track::process`, and the listener (median of 5, 2 s hold, level and silence). Tests through the fake device: an input sine at 110 Hz reads A2; inputs are wanted only while listening; a monitored armed track is silent with mute on and heard with it off, and still records.
- [x] 3. `src/generate/TuningGuide.{h,cpp}` (`gen::tuning`): the tunings, readings and words, zones, topic texts and examples, `analyse` with the five rules. Tests: the *Test numbers* table. Add both files to both CMake targets. Done when `./build.sh --test` passes without warnings.

Group B1, UI: the tuner window (`./build.sh --test` at step 6):
- [x] 4. Extend `tunepics::paintTuner` with the bands and the two text lines (DawIt Tune keeps its 10/25 bands and looks the same). Build `TunerWindow`: the controls, string buttons with the reference tone, keys 1 to 6, the status line, the level bar, the right-click menu, the settings keys. Check: DawIt Tune's window shot unchanged.
- [x] 5. The command: View > Tuner... with Cmd+7 (recipe *A command or menu item*), opening through `pluginWindows.show` with a `SafePointer`; listening starts when the window opens and stops when it closes.
- [x] 6. `ScreenshotRunner`: a `tuner` scene that opens the Tuner on Guitar: standard and feeds its listener a made-up A string 12 cents flat (109.24 Hz), so the shot shows a reading without a microphone.

Group B2, UI: the guide (`./build.sh --test` at step 8):
- [x] 7. `TuningExplainer` (copy `RecordingExplainer`): the ten pictures (the tuner topic uses the shared painter), clickable strings in topic 3, *Your tuning* following the engine, the tuner's settings and its session readings, and PDF export.
- [x] 8. Its `gen::guides` entry (area `sound`, not on *Start here*), cases in `MainComponent::openGuide` and `getPrintGuide`, the **?** button and menu item in the tuner, and the sentence in the chord shapes guide's guitar text. Tests: `runTheoryGuidesTests` sees the entry; search checks. `--export-guides` passes.

Group C, check and docs:
- [x] 9. Real-app check, in one `--screenshots` run: `tuner.png` (scene `tuner`: the note A2, a red needle left of centre (12 cents is beyond the 10-cent yellow band), "A string: 12 cents flat, tighten it a little", "in A minor", six string buttons); `tuner-bass.png` (setting `tuner.tuning` to Bass: 4 strings, four buttons); every guide topic (`"guide": "tuning"`, each topic by name), checking that no text is cut off, that the numbers match *Theory* and that *Your tuning* says "6 of the 6 open strings" on the pop song; `tune.png` (scene `tune`) to see that DawIt Tune's tuner hasn't changed.
- [x] 10. Docs: README Features bullet (the tuner and its guide); `docs/code-layout.md` (Tuner, TuningGuide, TunerWindow, TuningExplainer); `docs/tests.md` (the tuner tests); FEATURE_IDEAS status of this idea, and the *Recording, the home studio* idea's note that tuning is covered; `docs/architecture.md`'s *Recording audio* paragraph gets one line on the tuner's input tap and that it opens the inputs; the `tuner` scene in recipes.md *Checking in the real app*.

## Tests
The checks in *Test numbers*, in `runTunerTests` (engine: tracker, listener, input tap, mute) and `runTuningGuideTests` (generate: tunings, readings, words, zones, rules, text). The existing `runTuneEngineTests` and pitch tests must pass unchanged.

## Website hand-off
- Features page: a section "A tuner for guitar, bass and ukulele": the note and needle, the tunings, string buttons that play the note, the reference pitch, and the guide *How to tune a guitar*.
- Main page: one line in the guide picker's *Sound* area. Not a main step.
- Screenshots: `tuner.png` from the `tuner` scene (example pop song, A string 12 cents flat); the guide at *Harmonics at the 12th, 7th and 5th fret* and at *Your tuning and your song*.
- How it's made: nothing new (no library).

## Left for later
- Saving the song's tuning with the project and showing it on the track (a guitar track "in drop D").
- A strobe view, and more tunings (open G, open D, 7-string, baritone, mandolin, violin family) with user-made tunings.
- Polyphonic tuning (strum all strings at once) needs several pitches at once, which the tracker can't do.
- An intonation helper that compares the 12th-fret note with the harmonic, string by string.
- Sweetened tunings (offsets per string) and stretch tuning.

## Changes while building
- **Group A hand-off.**
  - Engine: `src/engine/Tuner.{h,cpp}` holds `tuner::InputTap` (FIFO; owned by `Engine`, reached with `engine.getTunerTap()`), `tuner::Reading` and `tuner::Listener` (`prepare (rate)`, `service (tap)` to drain it from the 30 Hz timer, `feed (samples, n)`, `reset()`, `getReading()`: `hasNote`, `midi`, `hz`, `ageSeconds`, `fade`, `levelDb`, `tooQuiet`, `exactlyZero`). Window: call `engine.setTunerInput (channel, stereo)`, `engine.setTunerMuteMonitoring (bool)`, `engine.setTunerListening (true)` on open and `false` on close (this opens the device inputs). The engine's mute is only in force while listening. "No inputs open" for the status line is `engine.getNumInputChannels() == 0`. Listener's rate is `engine.getSampleRate()` (call `prepare` again when it changes). `pitch::getTunerWindowSize` and `Tracker::prepare (rate, window, lowestHz = 50)` added; Tune is unchanged.
  - `Tuner.h` forward-declares `pitch::Tracker` (the listener holds it by `unique_ptr`) because `Engine.h` includes it and pulling `juce_dsp` into `Engine.h` broke `Singer.cpp`.
  - Theory: `gen::tuning` in `src/generate/TuningGuide.{h,cpp}`: `getStrings`, `getReading (hz, ref, tuning, locked)` (`note`, `cents`, `stringIndex`, `stringName`, `words`, `text`, `zone`), `describeCents`, `getZone`, `getNearestString`, topic names/summaries/`explain`/`getExampleCaption`, example data (`exampleHz`, `getBeatEnvelope`, `getTuningPath`, `getDriftGlide`, `getFifthFretSteps`, `getHarmonicHz`, `getCyclesInWindow`, `getReferenceCents`), `getKeyFit`, `getBestCapoFret`, and `analyse (tuning, ref, key, trackLows, lastReadings)` returning a `StringArray` of paragraphs. The setting id for a tuning is `getTuningId` / `getTuningFromId`. `getNoteName` uses the key's spelling (C#, Eb), but a reading on a string shows the string's own spelling (Db3).
  - Departures:
    - **E2 + F2 together.** A semitone apart, two notes beat slowly enough (4.9 Hz) that one 85 ms window scores up to 0.998 clarity at the pitch in between, so the plan's "no note" can't come from the tracker alone. The listener now also watches the loudness of the last 16 hops: a short dip of 6 dB or more that comes back up is a beat, and while it's seen there is no note and a held note is dropped. The first ~100 ms of such a sound can still flash a note. The tritone (E2 + A#2) scores 0.91 and stays "no note" by the clarity threshold, which is unchanged at 0.93.
    - **Last readings wording.** The plan's example is "E 2 cents flat, A in tune", but 2 cents is inside the green band. Rule 5 therefore shows the cents always and says "in tune" only when they round to 0; the tuner's own words keep "in tune" within 3 cents.
    - **Low-notes rule wording.** "below the lowest string, the E string" (with "the"); "or use a five-string bass" is added only for Bass: 4 strings, and "or use a drop tuning" only for standard and half a step down.
    - **Tuning names.** Menu names are "Guitar: standard", "Guitar: Drop D", "Guitar: DADGAD", "Guitar: half a step down", "Bass: 4 strings", "Bass: 5 strings", "Ukulele", "Chromatic".
- **Group B1 hand-off.**
  - `src/ui/TunerWindow.{h,cpp}`: `TunerWindow (Engine&, Selection&)`, opened by `MainComponent::showTuner()` (View > Tuner..., Cmd+7, `tunerCommand`, `SafePointer<TunerWindow> tunerWindow`). It has `std::function<void()> onOpenGuide`: the **?** button and the menu item "How to tune a guitar..." show only when it's set, and the button is placed in `resized()`, so set it in `showTuner` before the window is shown (group B2 does this and adds the `openGuide` case). `TunerWindow::getZoneColour (gen::tuning::Zone)` and `getTunerLastReadings (Tuning)` (a session store of each string's last cents, as `gen::tuning::LastReading`) are for the guide's *Your tuning* topic. Settings keys: `tuner.tuning` (id), `tuner.input` ("track", "m<ch>" or "s<ch>"), `tuner.referenceHz`, `tuner.mute`; the guide can read the first and third.
  - `tunepics::TunerPicture` and `paintTuner (g, area, TunerPicture)` in `TunePictures.{h,cpp}` take the bands, note text, lines and fade; the old `paintTuner (..., sungMidi, KeyInfo)` builds one with 10/25 cents, so DawIt Tune is unchanged (tests green; its window shot not re-taken yet: group C's `tune.png`).
  - `ScreenshotRunner` scene `tuner` (optional `"hz"`, default 109.24) calls `TunerWindow::showExample (hz)`, which shows a made-up reading instead of listening. Use shot settings for `tuner.tuning` etc.
  - Departure: the plan's step 9 expects a *yellow* needle for A string 12 cents flat, but with the chosen bands (green 3, yellow 10) 12 cents is red. The shot shows red text and needle, which is right; the plan's wording should say red. Also the top controls wrap to two rows below 780 px wide (the default 560 px), so the 32 px row of the sketch is two rows there.
  - Checked one `tuner.png` (560 x 440): note A2, needle left of centre, "A string: 12 cents flat, tighten it a little", "in A minor", six buttons E2 to E4, level bar and hint. Mute-while-tuning, the string buttons' tone and keys 1 to 6 were not exercised in the app (no input device in the scene).
- **Group B2 hand-off.**
  - `src/ui/TuningExplainer.{h,cpp}` (`TuningExplainer (Engine&)`, `showTopic (gen::tuning::Topic)`); `gen::guides::Guide::tuning` added (area `sound`, not on *Start here*), cases in `MainComponent::openGuide` / `getPrintGuide` / `showTuningGuide`, `ArrangementView` switches, `ScreenshotRunner`'s guide key `"tuning"`. The tuner's `onOpenGuide` is set in `MainComponent::showTuner`, so the **?** button and the menu item now show. The chord shapes guide's guitar paragraph (`ChordDiagrams.cpp`) ends with the "Tune first" sentence. Tests added in `runTheoryGuidesTests`.
  - The guide reads the Tuner's saved settings (`tuner.tuning`, `tuner.referenceHz`) and `getTunerLastReadings` at 10 Hz; clicking a string in topic 3 plays its saw tone (3 s) at the saved reference.
  - Departures: (1) topic 1's picture always shows the example (A string, 12 cents flat), not the live reading. (2) The plan's `**bold**` marks are stripped for display and PDF. (3) A search for "drop d" finds the guide but the structure guide (keyword "drop") ties and comes first; "dadgad", "ukulele", "432" and "guitar tuner" find it first, and plain "tuner" ranks DawIt Tune first. The test checks those. (4) Topic 2's picture draws the swells with 40 wiggles a second, not the real 440 (the caption says so).
  - Checked (scale 1, `"guide": "tuning"`): topics 3 (Drop D), 5, 7, 8, 10 by eye; `--export-guides` passes ("How to tune a guitar: 10 topics, 11 pages"). Topics 1, 2, 4, 6, 9 not yet looked at: group C's topic walk should.
- **Group C notes.** Checked `tuner.png`, `tuner-bass.png`, `tune.png` (DawIt Tune unchanged: 10/25 bands) and every guide topic at scale 1; the needle for 12 cents flat is red as the plan now says. Found and fixed: the guide text showed the plan's `*italic*` marks (for example "see *Why strings drift*"); `TuningExplainer` now strips every `*`. The scratch settings persist inside one run, so a guide shot after a `tuner.tuning` setting shows that tuning in *Your tuning* (set it back with `"settings"` for the standard-tuning check). Docs done: README bullet, code-layout, tests, architecture (Recording audio), recipes (`tuner` scene), FEATURE_IDEAS (done; the home studio idea notes it). Left for later stays as listed above.
- **Theory check.**
  - Topic 1 (`TuningGuide.cpp`): "a chord ... the tuner shows a dash" became "usually shows a dash, and a note it does show then can't be trusted". The beat check only covers close pairs (a semitone; the tritone falls to clarity); an octave or a fifth has one common period and can read as a note.
  - String button tooltip (`TunerWindow.cpp`): "Keys 1 to 6" became "The number keys, 1 for the first string, do the same", since a bass or ukulele has 4 or 5 strings.
  - Topic 3 picture caption for bass and ukulele (`TuningExplainer.cpp`): "Thick strings at the top" became "the order you see them on the instrument, the top one first"; a ukulele's top string (G4) is thin.
  - Test (`EngineTests.cpp`): the 0.47 / 1.90 Hz check now uses 10 cents *flat* (0.475 and 1.898 Hz, tolerance 0.005), as the text says; 10 cents sharp is 0.477 Hz.
  - Accepted as built: rule 5's "in tune" only at 0 cents, the low-notes wording, topic 2's 40 wiggles (labelled), topic 1's fixed example (labelled). Open: the topic 3 picture still draws the ukulele's G string thickest (code, not text).
