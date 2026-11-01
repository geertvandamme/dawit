# DawIt Slicer: repeat, reverse and gate audio in time with the song

Status: Released
Idea: [#126 — DawIt Slicer: repeat, reverse and gate audio in time with the song](https://github.com/geertvandamme/dawit/issues/126)

## Goal
A built-in effect that cuts a track into steps on the song's beat grid and, per step, plays it through, repeats the step before, plays it backwards or mutes it. It makes stutters, swells, trance gates and rolls that stay in time, shows the pattern against the beats with the step that is playing, and its guide *What is a slicer?* explains note lengths, the memory, clicks and the difference between a repeat and an echo.

## Decisions for you
1. **The silent step's name.** Options: *Mute*, *Gate* (the issue's word), *Gap*. **Recommended: Mute**, because DawIt Gate is a level-following noise gate; the guide explains the term *trance gate* and the difference.
   Answer: Mute.
2. **Scope of the pattern.** Options: one pattern of 1 to 16 steps, four modes and one Roll for all Repeat steps; or also per-step roll, chance and pitch (as Ableton's Beat Repeat). **Recommended: the first**: it fits a beginner's grid; the rest is listed in *Left for later*.
   Answer: The first: one pattern, four modes, one Roll for all Repeat steps.
3. **While the song is stopped.** Options: keep slicing on its own clock at the song's tempo; or pass the sound through. **Recommended: its own clock**, so the test beat and live playing can be heard sliced; *Play a bar* starts that clock on step 1.
   Answer: Its own clock (keep slicing).
4. **After a loop wrap.** Options: start empty, as after a jump (the first Repeat or Reverse step plays through); or keep the slice heard just before the wrap. **Recommended: start empty**: the same rule for every jump, and each loop pass sounds like the first.
   Answer: Start empty.
5. **Device bar chip.** Options: a picture of the 16 steps coloured by mode, or none. **Recommended: the picture** (it also shows in the Signal Flow window, through `chainpictures`).
   Answer: The picture.

## What the user does and sees
- **Adding it:** *DawIt Slicer* in the effect menus like any built-in; also a VST3/AU (code `Dslc`, category `Fx`).
- **The window** (an `fxui::EffectEditor`, copying DawIt Delay): header with starting points, **Play a bar** (the clap button relabelled), **Hear only what it changes**, the guide button; knob groups *Grid* (Step length, Steps, Tempo with its Song/Own switch and BPM) and *Sound* (Roll, Smooth, Mix); explainer on the right.
- **Pictures:** *The pattern*: a row of up to 16 cells over beat and bar lines, labelled with the beat they fall on; each cell drawn by mode (Through plain, Repeat a loop arrow with an arc from the slice it plays, Reverse a left arrow, Mute empty); the playing step lit, following the clock at 30 Hz. Above it four mode buttons (*Through, Repeat, Reverse, Mute*), the brush. Below: *The last 2 bars*: input (grey) and output (colour) levels against the same grid.
- **Interaction on the pattern:** click a cell to paint the brush's mode (clicking a cell already in that mode makes it Through); drag across cells to paint several; double-click or Delete over a cell makes it Through; keys 1 to 4 choose the brush; Cmd+C / Cmd+V copy and paste the pattern (modes, step length, Steps) as a line of text; right-click a cell: the four modes (ticked), *Make every step Through*, *Copy pattern*, *Paste pattern*, then *What is a slicer?...* (`guideicon::addItem`). Cells past Steps are dimmed and can't be painted.
- **Tooltips** on every control (texts in *Theory*); over a cell: "Step 6 of 16 · beat 2, 2nd sixteenth · Repeat: plays step 5 again (125 ms)"; over a Repeat/Reverse cell whose slice isn't ready: "... · plays through: the slicer hasn't heard step 5 yet".
- **Hear only what it changes:** Through steps go silent, so only repeats and reverses are heard.
- **Remembered:** nothing outside the plugin state (the brush resets to Repeat; the plugin build must not read app settings).

## Data and saving
All settings are plugin parameters in the processor's `AudioProcessorValueTreeState`, so they save in the project's plugin state like every built-in and are automatable in other hosts: step length (choice: 1/32, 1/16T, 1/16, 1/8T, 1/8, 1/4; default 1/16), Steps (1 to 16, default 16), Roll (1 to 4, default 1), Smooth (0.5 to 30 ms, default 3), Mix (0 to 100%, default 100), Tempo source (Song/Own, default Song), Own BPM (20 to 300, default 120), and 16 step modes (default Through). No `ProjectFile` change, no format version; projects without it are unaffected. Test: `checkThroughProject<BuiltInSlicer>` with a non-default pattern.

## The guide
*What is a slicer?*, pages of an `fxui::Guide` (`src/ui/SlicerGuide`), pictures following the processor's settings and tempo, *Play a bar* as its test sound:
1. **Slices of time**: cutting sound on the beat grid. Picture: a bar of the test beat's waveform cut into the steps, ms per step.
2. **Note lengths and steps**: how long a step lasts. Picture: rows 1/4 to 1/32 against one bar at the tempo, with ms.
3. **Repeat: a stutter**: playing the step before again. Picture: input slices above, output below, arcs from each slice to its repeats; Roll shown.
4. **Repeat or echo?**: why a stutter isn't a delay. Picture: one hit through a 1/16 delay at 50% feedback (each echo 6 dB down) and through three Repeat steps (all at 0 dB).
5. **Reverse**: backwards slices. Picture: a hit's envelope forwards and the same slice backwards in the next step.
6. **Mute: a trance gate**: rhythm from silence. Picture: a steady chord before and after the pattern, fades at Smooth's length.
7. **Clicks and smoothing**: why a cut needs a fade. Picture: 10 ms of a 110 Hz sine cut at a step boundary, without and with the fade.
8. **How DawIt Slicer works**: clock, memory, the empty start, tempo source. Picture: block diagram (input → step memory → choose by step → fade → mix with dry), with the held slice marked.
9. **Slicing in a mix**: where stutters work. Picture: an 8-bar strip, slicing only in bar 8 before a chorus.
10. **Your track**: what the pattern does to this track. Picture: the last 2 bars in and out per step, with the advice lines (rules in *Theory*).

**Theory Guides entry:** `Guide::slicer` after `delay`, area *Effects*; summary "Cutting a track into steps on the beat: repeats and stutters, reverse, trance gates, clicks and smoothing, and your track."; opens from "DawIt Slicer's window (What is a slicer?)"; plugin "DawIt Slicer"; words: slicer slice slices slicing stutter stutters repeat beat-repeat reverse backwards trance-gate gating chop chops chopping glitch roll rolls step steps pattern. Area summary becomes "Space, echoes, colour and rhythm: reverb, delay, slicing, saturation, distortion". Not on *Start here*. The Delay guide's *Delay in a mix* gets one sentence pointing to DawIt Slicer for repeats that replace the sound (text in *Theory*).

## Code
Follow recipes.md *A built-in effect or instrument*, copying DawIt Delay (processor, `DelayEditor`, `DelayGuide`), and *A theory guide* step 5 for the entry.
- **The clock.** Engine is every plugin's `AudioPlayHead` (`Engine::getPosition`): BPM, `ppqPosition` at the start of each `renderSegment`, `isPlaying`, loop points, 4/4. Segments split at tempo changes and loop wraps, so within a call the tempo is constant. Built-ins so far read only the BPM (Delay, Gate) or ppq too (Tune). The slicer re-reads the position every block: playing with ppq → song clock; otherwise its own clock at the host BPM (or Own BPM). Step = floor(beats / step beats) mod Steps.
- **Jumps.** A block whose ppq differs from the previous block's end by more than 4 samples' worth, or a start, clears both slices. `Track::preroll` (latency compensation) calls the chain with the same position twice; that reads as a jump and is harmless.
- **Slices.** Two buffers of one longest step (1 beat at 20 BPM, 3 s) per channel, made in `prepareToPlay`. Each step's input is recorded into the current buffer; at the start of a run of Repeat/Reverse steps the buffers swap and the held one stays for the whole run. A slice is ready only if its whole step was recorded since the last jump; otherwise the step plays through. A held slice keeps its length in samples when the tempo changes; step boundaries follow the beats.
- **Roll** plays the first 1/Roll of the slice Roll times per Repeat step; Reverse ignores Roll.
- **Fades.** Every change of source (step boundary, each restart of a repeat piece, into and out of Mute, both ends of a reverse) is a raised-cosine crossfade of Smooth ms (at most a quarter of the piece), starting at the boundary, gains summing to 1. No latency: `setLatencySamples (0)`, no `KeepsLatencyWhenBypassed`.
- **Mix** is linear (dry 1−m, wet m), not `fx::getMixGains`, so Through steps are unchanged at any Mix. No match level: Mix sets the effect, as in Delay.
- **Test sound:** the processor takes the clap request (`fx::Clap::takeRequest`) and plays its own one bar at the tempo in use (kick on 1 and 3, snare on 2 and 4, closed hat on every eighth, over a quiet held A minor chord), restarting its own clock at step 1.
- **History:** per step (index, mode, input and output peak and RMS, beat), in a non-consuming atomic ring like `StereoMeasure`, read by the pictures and `slc::analyse` (pure, in the engine file, tested), which returns the *Your track* advice.
- **Threading:** parameters are atomics; the audio thread allocates nothing; the editor paints from settings and the history only.
- **Risk:** a plugin with latency after the slicer (DawIt Tune) makes it cut slightly ahead of the beat; the guide says to put it after Tune.

## Libraries and models
None.

## Theory

### Facts
1. A beat (quarter note) lasts 60,000 / BPM ms; at 120 BPM 500 ms. Exact; the Rhythm and Delay guides.
2. At 120 BPM: 1/4 500 ms, 1/8 250, 1/8T 166.7, 1/16 125, 1/16T 83.3, 1/32 62.5. Exact, derived (500 × 1, ½, ⅓, ¼, ⅙, ⅛).
3. 16 steps of 1/16 = 4 beats = one bar of 4/4; 12 of 1/16T and 6 of 1/8T = 2 beats; 3 of 1/16 meet the bar again after 12 beats = 3 bars. Exact, derived.
4. At 90 BPM a 1/16 lasts 166.7 ms; at 140 BPM 107.1 ms. Exact, derived.
5. Roll 4 on 1/16 at 120 BPM: 31.25 ms pieces, 32 a second. Exact, derived.
6. Repetitions above about 20 a second are heard as a buzz with a pitch; the lower limit of hearing pitch is about 20 Hz. Typical; the Sound guide's 20 Hz.
7. A cut that jumps the wave's level makes a click; editors cut at zero crossings or fade: fades about 1 to 3 ms, crossfades about 2 to 4 ms. Typical; Sound On Sound (Removing clicks with crossfades).
8. Clicks are heard most on low, smooth sounds. Typical; agrees with the Gate guide's *Attack* topic.
9. Raised-cosine gains w and 1−w sum to 1, so |w·a + (1−w)·b| ≤ max(|a|, |b|): a join is never louder than the louder sound. Exact, derived.
10. Linear mix: (1−m)·x + m·x = x, so Through steps are unchanged at any Mix; a Mute step at Mix 50% is 20 log10 0.5 = −6.0 dB. Exact, derived.
11. Half the steps muted on a steady sound: 10 log10 0.5 = −3.0 dB average; 12 of 16 muted: 10 log10 0.25 = −6.0 dB. Exact, derived.
12. Ableton's Beat Repeat captures a tempo-synced piece and repeats it; it can add repeats to the sound (Mix) or play them instead (Insert); small grids give artefacts, large ones loops. Exact (manufacturer); Ableton Live manual.
13. A delay at 50% feedback: each echo 6 dB quieter than the one before. Exact; the Delay guide.
14. The Beatles used reversed tape in 1966: the backwards guitar solo of *I'm Only Sleeping* and backwards vocals at the end of *Rain*. Exact (history); Beatles Bible, Wikipedia *Backmasking*. Who was "first" is disputed: not claimed.
15. A trance gate chops a sustained sound (often a pad) into rhythm with a step pattern; named after 1990s trance. Typical; Soundbridge, search results (Elektronauts).
16. Trance is usually 136 to 140 BPM. Typical; the Rhythm guide.
17. Trap hi-hat rolls are usually 1/32 or 1/64 notes, often before a snare or at the end of a bar. Typical; search results (eMastered, Unison).
18. Longest slice: 1 beat at 20 BPM (DawIt's slowest, `TempoMap::minBpm`) = 3 s = 144,000 samples at 48 kHz. Exact, derived.
19. Slicing works best as a contrast, kept for fills and the bar before a new section. Opinion.
20. The *Your track* thresholds (30 dB, 6 dB, 12 of 16 steps). Opinion.

Exact 13, typical 5, opinion 2.

### Topic texts (the guide's pages: name, summary, text)

**1. Slices of time** — *Cutting sound on the beat grid*

A slicer cuts a track into short pieces of time, called steps, that line up with the song's beats, and does something with each one: plays it as it is, plays the step before it again, plays it backwards, or leaves a gap. Out of one steady part it makes stutters, swells and choppy rhythms that stay in time with the song.

The steps follow the beat. At 120 BPM a beat lasts 500 ms, so with a step length of 1/16 each step is 125 ms and 16 of them make one bar of 4/4. Change the tempo and the steps change with it: at 90 BPM a sixteenth lasts 167 ms.

A slicer works on the sound itself, from a recording, a loop or an instrument. It doesn't know where the notes are: it cuts where the grid says, even in the middle of a note. That is part of its sound, and the reason it matters where a part's notes fall against the grid.

Producers use it for the stutter in the bar before a chorus, the choppy pads of trance, a hit played backwards into the next bar, and drum rolls. The next topics take these one by one.

The picture shows one bar of the test beat cut into DawIt Slicer's steps, with how long each lasts at the tempo.

**2. Note lengths and steps** — *How long a step lasts*

The step length is a note value, as in the piano roll. A quarter note (1/4) is one beat, an eighth (1/8) half a beat, a sixteenth (1/16) a quarter of a beat and a thirty-second (1/32) an eighth of a beat. Triplets fit three in the time of two: a sixteenth triplet (1/16T) is a sixth of a beat, an eighth triplet (1/8T) a third.

A beat lasts 60,000 / BPM milliseconds. At 120 BPM:
- 1/4: 500 ms
- 1/8: 250 ms
- 1/8T: 167 ms
- 1/16: 125 ms
- 1/16T: 83 ms
- 1/32: 62.5 ms

Steps sets how many steps the pattern has before it starts again, from 1 to 16. 16 steps of 1/16 make one bar; 16 of 1/8 make two. With triplets, 12 steps of 1/16T or 6 of 1/8T fill two beats, so the pattern meets the bar line again. Other numbers drift across the bar and come back later: a pattern of 3 sixteenths only starts on the bar line again every 3 bars.

The pattern is counted from the start of the song, so step 1 is always on the first beat of bar 1, wherever you start playing.

Shorter steps cut more finely: 1/32 steps sound like a buzz or a drum roll. 1/4 steps repeat whole beats, which sounds less like an effect and more like rearranging the part.

The picture shows each step length against one bar at the tempo the slicer is using.

**3. Repeat: a stutter** — *Playing the step before again*

A Repeat step plays the step before it again, instead of what comes in now. Set step 2 to Repeat and you hear step 1 twice; what came in during step 2 is left out.

Several Repeat steps in a row all play the same slice: the one just before the row. Through, Repeat, Repeat, Repeat plays step 1 four times. When the row ends, the next Through step plays the track where the song is by then, so the part stays in time: the repeats replace a stretch of it, they don't push it later.

Roll plays a shorter piece more often within each Repeat step. With Roll 2 a step plays the first half of the slice twice; with 3, its first third three times; with 4, its first quarter four times. On 1/16 steps at 120 BPM, Roll 4 repeats pieces of 31 ms, 32 times a second. Roll 3 gives triplets inside the step.

Very short repeats stop sounding like repeats. Above about 20 a second the ear hears them as a buzz with a low pitch, for the same reason a vibration 20 or more times a second is heard as a note. Glitch music and build-ups use this on purpose.

A Repeat step needs something to repeat. Right after you press play, jump or loop back, the slicer hasn't heard the step before yet, so that step plays the track through as it is.

The picture shows the steps that came in above and what comes out below, with an arc from each repeated slice to where it plays again.

**4. Repeat or echo?** — *Why a stutter isn't a delay*

A repeat and an echo both play again something you've just heard, but they work differently and sound different.

An echo, from a delay, comes on top of the sound. The sound carries on, and a copy follows a set time later, usually quieter and fading with every repeat. Nothing is taken away: you hear more.

A repeat, from a slicer, replaces the sound. During a Repeat step you hear the step before at its full level, and what came in during that step is left out (at a Mix of 100%). Nothing fades: the fourth repeat is as loud as the first. When the repeats end, the track comes back where the song is.

So a delay fills the gaps and makes a part bigger; a slicer rearranges it. A delay on a vocal leaves the words whole with echoes after them; a slicer makes the same vocal stammer: "to-to-to-tonight".

Some effects can do both: Ableton Live's Beat Repeat can add its repeats to the sound or play them instead of it. DawIt Slicer always plays them instead, and its Mix knob blends the result with the untouched track.

The picture shows one hit through each: DawIt Delay at 1/16 with 50% feedback, where each echo is 6 dB quieter than the one before, and DawIt Slicer with three Repeat steps, where every repeat is as loud as the hit.

**5. Reverse** — *Backwards slices*

A Reverse step plays the step before it backwards. A drum hit, which starts loud and dies away, becomes a sound that swells up from nothing and stops dead: the reverse cymbal or reverse snare that pulls into a new section. A sung word becomes the strange, sucking sound of tape played backwards.

The slicer can't play backwards what hasn't happened yet, so it always reverses the step that has just passed. A row of Reverse steps plays that same slice backwards each time.

Backwards sound began with tape: thread a reel the other way round and the music plays backwards. The Beatles made it famous in 1966, with the backwards guitar solo of I'm Only Sleeping and the backwards vocals at the end of Rain.

A reversed hit ends with what was its start, so its loudest moment comes at the very end of the step, just before the next one. That pull towards the next beat is what makes it useful: a Reverse step on the last step of a bar swells into the downbeat. It works best when the step before starts with a hit; a step that is quiet at its start gives a swell that hardly arrives.

The picture shows a hit as it came in and the same slice played backwards in the next step.

**6. Mute: a trance gate** — *Rhythm from silence*

A Mute step lets nothing through: a short gap. A pattern of Through and Mute steps chops a long, steady sound into rhythm. On a held pad chord, Through on every other 1/16 step turns one long chord into a pulse of sixteenth notes.

This is called a trance gate, after the trance music of the 1990s, where it was everywhere on pads and held synth chords, usually at 136 to 140 BPM. It works best on steady sounds: a pad, a held chord, a long note, noise. On drums, a Mute step only takes hits away, which you can do more simply by deleting notes.

DawIt Gate is a different effect with a similar name: it opens and closes by itself, following how loud the track is, to keep noise out between notes. A trance gate ignores the level and follows a pattern on the beat.

Smooth sets how quickly each step fades in and out. 1 to 5 ms gives crisp cuts without clicks; 10 to 30 ms gives a softer pulse that breathes. On short steps a long fade fills the gaps back in: on 1/16 steps at 140 BPM (107 ms each), a 30 ms fade takes up more than a quarter of every gap.

A Mix below 100% turns the gaps into dips: at 50% a Mute step is half as loud, 6 dB down, instead of silent.

The picture shows a steady chord before and after the pattern, with the fades drawn at Smooth's length.

**7. Clicks and smoothing** — *Why a cut needs a fade*

Sound is a wave that swings smoothly up and down. Cut it at a random moment and jump to another piece of sound, and the wave jumps too, from one level to another between two samples. The speaker follows that jump, and you hear a click. A slicer cuts at every step, so without care it would click up to 16 times a bar.

Audio editors avoid it in two ways: by cutting where the wave crosses zero, or with a short fade. Sound On Sound suggests fades of about 1 to 3 ms at the edges of a clip, or crossfades of about 2 to 4 ms where two pieces join.

A slicer can't wait for the wave to cross zero, because the step must start on the beat, so it fades. At every change (from the track to a repeat, at each start of a repeated piece, into and out of a gap, at both ends of a reverse) DawIt Slicer fades the old sound out while it fades the new one in, over the Smooth time, starting on the step's boundary. The two always add up to 100%, so a join is never louder than the louder of the two sounds.

3 ms, where it starts, keeps a drum's attack sharp and takes the click out of most sounds. Clicks are easiest to hear on low, smooth sounds such as a bass or a sine wave, where nothing else covers them; raise Smooth a little if you still hear them. On very short pieces (Roll on 1/32 steps) the fade is kept to a quarter of the piece, so there's still sound in it.

The picture shows 10 ms of a low note cut at a step: without a fade the wave jumps; with Smooth it bends over to the new sound.

**8. How DawIt Slicer works** — *The song's clock and a memory*

1. It follows the song's clock. DawIt tells every effect on a track the tempo and where the playhead is, for every block of sound. From that, DawIt Slicer works out which step it's in: the steps since the start of the song, counted round the pattern. So step 1 is on the first beat of bar 1 whether you play from the start or from bar 9, loop a section or change the tempo.

2. It records every step. While a step plays, what comes in is written into a memory. When the next step is a Repeat or Reverse, that recording becomes the slice it plays from, and stays so for the whole row. The memory holds two of the longest steps: a 1/4 at 20 BPM, DawIt's slowest tempo, is 3 seconds, which at 48 kHz is 144,000 samples for each side. It is made when playback is prepared, never while the sound runs, and never grows.

3. It starts empty. After you press play, jump or loop back, it hasn't heard the step before, so a Repeat or Reverse step right then plays the track through. From the next step on it has a slice.

4. When the song is stopped, it keeps its own clock at the song's tempo, so you still hear it on the test beat and on what you play live. Play a bar starts that clock on step 1.

5. Tempo: Song follows the song's tempo and position. Own uses the BPM you set, for a part recorded at another tempo, or in a program that doesn't tell plugins its tempo. As a plugin in another program, DawIt Slicer follows that program's tempo and position when it gives them, and its own tempo (120 BPM to start with) when it doesn't.

6. It holds nothing back: there's no latency, so the track stays in time whether DawIt Slicer is on or bypassed. Put it after DawIt Tune in a chain: DawIt Tune holds the sound back a little, and a slicer before it would cut slightly ahead of the beat.

7. The output is mixed with the untouched track by Mix, in straight proportion, so a Through step sounds exactly like the track at any Mix.

The picture follows the sound from the input through the memory, the choice for each step and the fades, to the mix.

**9. Slicing in a mix** — *Where stutters work*

Slicing works best as a contrast. A part that stutters all the time stops sounding like an effect; a stutter in the last bar before the chorus makes the chorus land harder. That's an opinion, but a widely shared one.

Where it's used:
- Build-ups and fills: Repeat steps on the last beat of a bar or section (Stutter into the next bar).
- Pads and held chords: a Mute pattern, the trance gate.
- Drums and loops: a Reverse step at the end of a bar for a swell; Repeat with Roll for a drum roll.
- Hi-hats: rolls of 1/32 or 1/64 notes, often just before a snare or at the end of a bar, as in trap.
- Vocals: one word chopped into a stammer, a common hook in dance and pop. Use a copy of the vocal, so the main one stays clear.

How it's used:
- DawIt can't yet switch an effect on and off at a point in the song. To slice just one bar, copy that bar's clip to a track of its own with DawIt Slicer, and leave the bar out of the original track.
- Keep Mix at 100% for clean stutters and gaps; lower it to keep some of the original underneath.
- Check that the slices start on a note. A Repeat after a silent step repeats silence; a Reverse after a quiet step barely swells. Your track points these out.
- Clicks: if you hear them, raise Smooth a millisecond or two.

The picture shows eight bars with slicing only in the bar before the chorus.

**10. Your track** — *What the pattern does to this track*

This topic looks at the last 2 bars of the track DawIt Slicer is on, step by step, as the slicer heard them, and says what to try.

For each step it compares how loud the track came in and how loud it came out. It points out repeats and reverses of slices that were almost silent, how much quieter the pattern makes the track on average, and a pattern that changes most of its steps.

Play the track, or click Play a bar, and the picture follows.

### The window's texts (describe / typical / tooltip)
- **Step length** — "How long each step lasts, as a note value of the song's tempo: from 1/32 to 1/4, with triplets (1/16T, 1/8T). At 120 BPM a 1/16 step is 125 ms." Typical: "1/16: stutters, gates and fills. 1/8: reverses and slower chops. 1/32: rolls and buzzes. Triplets: rolling, swung chops." Tooltip: "How long a step lasts, as a note value".
- **Steps** — "How many steps the pattern has before it starts again, counted from the start of the song. 16 steps of 1/16 make one bar; other numbers drift across the bar line." Typical: "16 for a bar of 1/16 or two bars of 1/8. 12 with 1/16T, 6 with 1/8T. 3, 5 or 7 for patterns that cross the bar." Tooltip: "How many steps before the pattern repeats".
- **Tempo** — "Song: the steps follow the song's tempo and position. Own: they follow the BPM set here, on the slicer's own clock (also used as a plugin when the program gives no tempo)." Typical: "Song, nearly always. Own for a part recorded at another tempo." Tooltip: "Follow the song's tempo, or a tempo of its own".
- **Roll** — "How many times a Repeat step plays its slice: 1 plays it once; 2, 3 or 4 play its first half, third or quarter that many times, for rolls. Reverse steps ignore it." Typical: "1: plain repeats. 2 or 4: drum rolls. 3: triplet rolls." Tooltip: "Repeat steps play a shorter piece this many times".
- **Smooth** — "How long each cut fades, in ms: the old sound fades out while the new one fades in, so the jump doesn't click." Typical: "1-5 ms: crisp. 10-30 ms: soft, breathing gates. Raise it if a bass clicks." Tooltip: "Fade at every cut, so it doesn't click".
- **Mix** — "How much of what comes out is the sliced sound. 100%: only the slicer; 50%: Mute steps are 6 dB dips and repeats sit on top of the track. Through steps are the same at any mix." Typical: "100% for stutters and gates. 30-60% for a gentler pulse." Tooltip: "The sliced sound against the untouched track".
- **The pattern** — "The steps over the song's beats. Click a step to give it the chosen mode, drag to paint several, double-click to make it Through. The lit step is playing now." Tooltip per cell as in *What the user does and sees*.
- **Mode buttons** — Through: "Plays the track as it is." Repeat: "Plays the step before again, at full level." Reverse: "Plays the step before backwards." Mute: "Silence for this step: a trance gate."
- **The last 2 bars** — "Grey: the track coming in; colour: what comes out, against the steps. Repeats copy an earlier step's shape; gaps are Mute steps."
- **Play a bar** — "Plays one bar of drums over a held chord through the slicer, at the tempo it uses, from step 1. It works while the song is stopped." Tooltip: "Play a bar of drums and a chord through the slicer".
- **Hear only what it changes** — "Silences the Through steps, so you hear only the repeats and reverses."
- **Overview** — "DawIt Slicer cuts the track into steps on the beat and plays each one through, repeats the step before, plays it backwards, or mutes it, in time with the song."
- **Delay guide addition** (end of *Delay in a mix*): "To repeat a piece of a part instead of adding echoes to it, for a stutter, use DawIt Slicer: see What is a slicer?, Repeat or echo?"

### Starting points (name — settings — why)
1. **Neutral: every step plays** — 1/16, 16 steps, Roll 1, Smooth 3 ms, Mix 100%, all Through. "Nothing changes until you click a step. Start here and paint your own pattern."
2. **Stutter into the next bar** — 1/16, 16, Roll 1, 3 ms, 100%; steps 14 to 16 Repeat. "The first sixteenth of beat 4 plays four times before the next bar: the stutter before a new section. 1/16 keeps it tight, Smooth 3 ms keeps the cuts clean, Mix 100% so the repeats replace the track."
3. **Trance gate** — 1/16, 16, Roll 1, 3 ms, 100%; even steps Mute. "Every other sixteenth is silent, so a held chord pulses in sixteenths. Best on pads and long notes; 3 ms keeps the edges crisp without clicks."
4. **Soft pulse** — as Trance gate, Smooth 15 ms, Mix 50%. "The gaps become 6 dB dips with soft edges: a gentle, breathing movement under a mix rather than a chop."
5. **Reverse swell** — 1/8, 8 steps, Roll 1, 3 ms, 100%; step 8 Reverse. "The last eighth of every bar plays the eighth before it backwards: a hit there turns into a swell into the downbeat. 8 steps of 1/8 make one bar."
6. **Triplet rolls** — 1/8, 8 steps, Roll 3, 3 ms, 100%; steps 2 and 6 Repeat. "For a hi-hat track: steps 2 and 6, the second halves of beats 1 and 3, play the start of the eighth before them three times, as sixteenth triplets, rolling into the snares on beats 2 and 4. Roll 3 makes the triplets."

### Example data
- Step ms at 120 BPM: see fact 2. At 90 BPM: 1/4 666.7, 1/8 333.3, 1/16 166.7. At 140: 1/16 107.1.
- Echo vs repeat picture: delay echoes at 0, 125, 250, 375 ms at 0, −6.0, −12.0, −18.1 dB (20 log10 0.5ⁿ); slicer repeats at 125, 250, 375 ms all 0 dB.
- Clicks picture: 110 Hz sine (one cycle 9.09 ms), cut at 5 ms to a piece starting at its opposite phase; fade 3 ms.
- Test bar at 120 BPM: 2.0 s; kick at 0 and 1.0 s, snare at 0.5 and 1.5 s, hats every 0.25 s; A minor chord (A3, C4, E4) held for the bar about 18 dB under the drums.

### "Your track" rules (over the last 2 bars of step records; first match of S0 ends, the others list in order)
- **S0, nothing heard:** every step's input peak below −60 dBFS → "Play the track, or click Play a bar, to see it here." Nothing else.
- **S1, no song tempo** (Song source, host gave no BPM) → "No song tempo reaches DawIt Slicer here, so it slices at its own 120 BPM. If the track has a tempo, choose Own and set it, or the steps will drift against the beat." (with the own BPM).
- **S2, all Through** → "Every step plays through, so DawIt Slicer leaves the track as it is. Click a step to make it repeat, reverse or mute."
- **S3, near-silent slice:** a Repeat or Reverse step whose slice peaked more than 30 dB below the loudest step → "Step 6 repeats a slice that was 35 dB quieter than the loudest step, so you hear a gap. Move the repeat to just after a note, or make it a Mute step if a gap is what you want." (Reverse: "plays backwards a slice that was ..."). One line per step, at most three.
- **S4, level drop:** output RMS over the 2 bars at least 6 dB under the input RMS → "The pattern makes the track about 7 dB quieter on average, mostly from its 10 Mute steps. Fine for a fill; for longer, raise the track's fader or use fewer Mute steps."
- **S5, busy:** 12 or more of the pattern's steps are not Through → "12 of 16 steps change the sound. Slicing tends to work best as a contrast: try keeping it for the bar before a new section."
- **S6, summary**, always after S0: "The last 2 bars: 16 steps of 1/16 (125 ms at 120 BPM): 3 repeats, 1 reverse, 4 mutes."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| 1/16 at 120 BPM | 125.0 ms | 0.01 |
| 1/8T at 120 BPM; 1/16T | 166.67 ms; 83.33 ms | 0.01 |
| 1/4 at 20 BPM | 3000 ms | 0.01 |
| Slice buffer per channel at 48 kHz | 144,000 to 150,000 samples | — |
| Step index: ppq 3.75, 1/16, 16 steps; ppq 4.0; ppq 1.5 with 6 steps | 16; 1; 1 | exact |
| Repeat: impulse at sample 500 of step 1 (past the fade), step 2 Repeat (1/16, 120 BPM, 48 kHz) | peak at 6500 | ±1 sample |
| Roll 2; Roll 3 (same impulse) | peaks at 6500, 9500; 6500, 8500, 10500 | ±1 |
| Run: Through, Repeat ×3, the same impulse | peaks at 6500, 12500, 18500, each within 0.1 dB of the input | ±1 |
| Reverse: impulse at sample 1000 of step 1, step 2 Reverse | peak at 10999 | ±1 |
| Mute step past the fade, Mix 100% | 0 | exact |
| Mute step at Mix 50%, steady sine | −6.02 dB | 0.05 |
| Through steps at Mix 0, 37, 100% | out equals in | 1e−6 |
| Output peak vs input peak of the last 2 steps | not above | 1e−6 |
| 1 kHz sine at 0.5, mixed pattern, Smooth 3 ms | max sample-to-sample step ≤ 0.1 (the sine's own is 0.065) | — |
| Silence in, every mode | exactly 0 out | exact |
| First step after start or seek is Repeat | out equals in for that step | 1e−6 |
| Tempo 120 → 60 at beat 4 | next boundary at beat 4.25 = 250 ms after the change | ±1 sample |
| No playhead, Own 100 BPM, the same impulse | 1/16 = 150 ms; repeat peak at 7700 | ±1 |
| Song source, playhead without BPM | uses Own BPM | exact |
| Stopped (isPlaying false) | own clock at the host BPM; Play a bar starts step 1 | exact |
| Buffers after processing at 20 BPM, 1/4 | sizes unchanged (no allocation) | exact |
| Play a bar at 120 BPM | 2.0 s | 1 block |
| Latency | 0 samples | exact |
| Mean level with 8 of 16 / 12 of 16 steps Muted, steady sine | −3.01 / −6.02 dB | 0.1 |
| Analysis: S0 to S6 each on made-up records; S3 at 35 dB fires, 25 dB doesn't; S4 at 7 dB fires, 5 doesn't | | |
| Theory Guides search "stutter", "trance gate", "slicer" | finds *What is a slicer?* | |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. `src/engine/BuiltInSlicer.{h,cpp}`: the `slc` namespace (step values, Settings, step index, step ms, formatting) and the processor's parameters, `getDescription`, presets with whys from *Theory*; register in `BuiltInFormat.cpp` and `dawit_add_builtin (Slicer ... CODE Dslc ... "Fx")` (editor files may start as stubs, as other built-ins). Tests: step ms, step index, presets present, registry. Check: tests pass.
- [x] 2. The DSP: clock (song, own, stopped), jumps, the two slice buffers, Repeat with Roll, Reverse, Mute, raised-cosine fades, linear mix, *Hear only what it changes*, the test bar via `takeRequest`. Tests: every timing, level, click, silence, jump, tempo and no-playhead row of the table, driving `processBlock` with a fake `AudioPlayHead`.
- [x] 3. Engine check through DawIt: the slicer on an audio track in a fake-device render, playing from bar 2, with a tempo change and a loop wrap (repeat lands on the grid; first step after the wrap plays through). `checkThroughProject<BuiltInSlicer>` with a non-default pattern.
- [x] 4. The step history ring and `slc::analyse` (rules S0 to S6). Tests: each rule on made-up records, thresholds both sides. `./build.sh --test`. Done when: all checks pass, no warnings.

Group B, UI (`./build.sh --test` at step 8):
- [x] 5. `src/ui/SlicerEditor.{h,cpp}`: the fxui frame, knob groups, the pattern picture with brush buttons and every interaction and tooltip, the last-2-bars picture, the explainer texts; set `createEditorFunction` in `MainComponent`. Check: opens from a track and as `pluginEditor`.
- [x] 6. `src/ui/SlicerGuide.{h,cpp}`: the ten pages with pictures and Your track (`fxui::getHostTrack`), `setPrintSource`; `BuiltInGuides` branches (`TheoryGuidesWindow.cpp`); `gen::guides` entry, area summary, `MainComponent::openGuide` case; the Delay guide sentence. Tests: `runTheoryGuidesTests` entry and the three searches.
- [x] 7. Device bar chip: `chainpictures` draws the steps coloured by mode (settings only). Update `hasPicture`'s comment.
- [x] 8. `ScreenshotRunner`: allow `"preset"` for DawIt Slicer, as for Stereo and Tune. `./build.sh --test`; `--export-guides` passes.

Group C, check and docs:
- [x] 9. Real-app check (`--screenshots`): Drums track, `addPlugins: ["DawIt Slicer"]`, preset *Stutter into the next bar*, `play: true` at beat 12 (lit step, last-2-bars filled); a Pad or Keys track with *Trance gate*; the guide at *Repeat or echo?*, *Clicks and smoothing* and *Your track* after playing; `pluginEditor` scene for DawIt Slicer with the overlay; the device bar chip. Look for clipped text, wrong ms, empty pictures.
- [x] 10. Docs: README *Features* bullet (effects list); `docs/tests.md` (built-in effects area and the registry line); `docs/code-layout.md` (the four files); `docs/architecture.md`: a line under *Built-in effects* that Engine is every plugin's `AudioPlayHead` (BPM, ppq at each segment's start, playing, loop points; preroll repeats the position), and add Slicer to the device bar pictures; `docs/recipes.md` step 6: replace the stale "only EQ and Compressor have one so far" with the current list. GitHub #126: progress and what's left; `SOURCES.md` is already updated.

## Tests
As in *Test numbers*; plus: every parameter's text non-empty (registry test covers it), every page has text and a picture, every preset has a name and a why and loads its 16 modes, and `Hear only what it changes` silences a Through step.

## Website hand-off
- `features.html`: the effects group, next to DawIt Delay: stutters, reverse swells and trance gates on the beat, with *What is a slicer?*.
- `learn.html`: *What is a slicer?* in Effects.
- Screenshot: the editor on the example song's Drums with *Stutter into the next bar*, playing; optionally the *Repeat or echo?* page. No library: `about.html` unchanged.

## Left for later
- Per-step roll, chance, pitch and level (Beat Repeat-style variation): scope kept to a beginner's grid.
- Switching it on for one bar only needs automation (#87).
- Keeping the slice across a loop wrap (decision 4).
- Time signatures other than 4/4 (DawIt is 4/4 for now).
- Compensating latency of plugins after it in the chain.
- A shared step grid with the step editor (#51).

## Changes while building

### Group A hand-off (engine, built)
- **Files:** `src/engine/BuiltInSlicer.{h,cpp}` (namespace `slc` and `BuiltInSlicer`), stub editor `src/ui/SlicerEditor.{h,cpp}` (`BuiltInSlicerEditor`, a plain `juce::AudioProcessorEditor`: Group B replaces it with an `fxui::EffectEditor`; `createEditorFunction` is not yet set in `MainComponent`), registered in `BuiltInFormat.cpp` and `CMakeLists.txt` (`UI_SOURCES` has only `SlicerEditor.cpp`: Group B adds `SlicerGuide.cpp`). Tests: `tests/SlicerTests.h` (`runBuiltInSlicerTests`, `runSlicerAnalysisTests`, `runSlicerEngineTests`), included before `runBuiltInSaturationTests` in `EngineTests.cpp` and called after `runBuiltInDelayTests`.
- **For the UI (all in `slc::` / on the processor):** `slc::Settings`, `getPresets()` (name, why, settings; no per-knob notes: the plan gave only a why), `formatKnob`, `getModeName`, `getStepMs`, `getStepBeats`, `getStepIndex`; processor: `getSettings/setSettings/setKnob/setMode/getMode`, `getKnobParameter`, `getModeParameter (step)`, `getParameters()`, `listenToEffect`, `clap.request()` (Play a bar: the processor takes it and plays its own bar), `playingStep` (0-based, -1 none), `playingSliced` (the playing step is a Repeat or Reverse step that has a slice), `clockBeat` (position in quarter notes at the last block's start, for the lit step's phase), `usingSongClock`, `tempoBpm` (host's, 0 unknown), `getUsedBpm()`. History: `history.getRecent (records, max)` (non-consuming, oldest first, ring of 256), `history.clear()`; advice: `slc::analyse ({ settings, records, bpm, hostHasTempo })` returns the lines (S0 alone, or S1 to S6 in order, the summary last). The last 2 bars are `8 / getStepBeats` records (up to 64): ask for that many.
- **TEMPORARY exemption in `runTheoryGuidesTests`** (`tests/EngineTests.cpp`, "every built-in effect and instrument has one guide in the hub" and "no guide names a plugin that isn't built in"): DawIt Slicer is skipped because its guide comes in step 6. Group B deletes the `slicerWithoutGuide` line and its two uses.
- **Deviations / choices:** (1) Fades: when a source ends (a slice past its end) it holds its last sample while it fades out, so every join is continuous; the plan's "gains summing to 1" holds. A new voice's fade is at most a quarter of its piece (Repeat: the piece, Reverse: the slice, others: the step). (2) With Tempo = Own while the song plays, the steps count from the song's seconds at the Own BPM (so they still start at the song's start and follow seeks); stopped or without a play head, the free-running own clock. (3) The own clock starts at beat 0 on prepare, when the song stops, and at *Play a bar*. (4) The test bar is kick 0.65 on 1 and 3, snare on 2 and 4, hat 0.15 on every eighth, A minor chord 3 x 0.035; it peaks at about 0.9 (-1 dB) and lasts 4 beats at the tempo in use. (5) Record levels are the mixed output (not the listen output). (6) Advice wording for Reverse S3: "Move the reverse to just after a note, ..." (the plan only gave the Repeat wording); S4 drops "mostly from its N Mute steps" and "or use fewer Mute steps" when there are no Mute steps. (7) Parameter ids: `step`, `steps`, `roll`, `smooth`, `mix`, `tempo`, `ownbpm`, `mode1` to `mode16` (all version 1). (8) Plugin description category is "Rhythm" (the VST3 category stays "Fx"). Slice memory: 144,064 samples per channel at 48 kHz.
- **Engine finding:** the engine fades the master in when playback starts, so the first sound after `play()` is quieter than the next; engine tests locate peaks by position with a hundredth-of-maximum threshold, and compare the repeat's distance to the original (a single-sample impulse can read 1 to 2 samples off through the clip player).
- **No facts, numbers or texts were changed** from *Theory*; the code uses the plan's step table, presets, whys, thresholds and advice texts as written.

### Group B hand-off (UI, built)
- **Files:** `src/ui/SlicerGuide.{h,cpp}` (namespace `slctext`: texts, `PatternGrid`/`drawPattern`/`getCellDescription`/`drawLastBars`/`getLastBars`, the ten pages from *Theory* verbatim, `getSliceSource`), `src/ui/SlicerEditor.{h,cpp}` (`SlicerPatternView`, `SlicerLastBarsView`, `BuiltInSlicerEditor` as an `fxui::EffectEditor`, header button `Sound::note` relabelled "Play a bar"). Wired: `CMakeLists.txt` (`SlicerGuide.cpp`), `MainComponent` (editor factory, `G::slicer` cases), `TheoryGuides.{h,cpp}` (`Guide::slicer`, entry, area summary), `TheoryGuidesWindow.cpp`, `ArrangementView.cpp`/`DevScreenshots.cpp` switches, the Delay guide sentence, `ChainPictures` + `DeviceBar` (steps coloured by mode; Signal Flow gets it through `hasPicture`), `ScreenshotRunner` `"preset"`/`"clap"` for DawIt Slicer. The `slicerWithoutGuide` exemption is removed. `docs/` and README are Group C's.
- **Added to the engine file:** `slc::patternToText` / `slc::patternFromText` ("DawIt Slicer pattern: 1/16 16 TTRRVVMM...", T R V M) for Cmd+C / Cmd+V, tested in `runBuiltInSlicerTests`. New tests: slicer guide entry, three searches, Effects area summary.
- **Choices not in the plan (UI text and layout only):** brush buttons sit in the pattern panel's top right; the BPM knob shares the Tempo topic and is dimmed while Song has a tempo; the window's overview, "Reading the window" and "Shortcuts" texts and the guide button's text are mine; the starting-point explainer lists each knob's value (no per-knob notes exist) and the preset's *why*. Pictures use a made-up level shape for the test beat (`getTestLevel`: kick 0.65 / snare 0.5 / hat 0.15 over a quiet chord, decays 120 / 80 / 20 ms), only to draw; nothing is claimed from it. Pages 3, 5 and 6 draw your pattern, or a small example (named in the caption) when it has no Repeat, Reverse or Mute step. Page 9 draws one bar of your pattern in bar 8. The lit step is the processor's `playingStep` (it stays lit on the last step when no sound flows).
- **No facts, numbers or texts were changed** from *Theory*; page texts were copied from the plan by script. The tooltip for a missing slice shows only on the playing step.
- **For Group C:** the quick shot with `Drums`, "Stutter into the next bar", `play` at beat 12 showed an empty *last 2 bars* picture: check the run really plays through the slicer (and wait for it) before judging. The guide's page 4 labels were fixed after a first look; the other pages have not been looked at yet. `--export-guides` passes (slicer PDF written).

### Group C hand-off (check and docs, built)
- **Real-app check done** (`--screenshots`, scratchpad shots): editor on Drums (*Stutter into the next bar*, playing from beat 12) and on Keys (*Trance gate*), the guide's ten pages, the `pluginEditor` scene with the overlay, and the device bar chip. The empty last-2-bars picture was not a bug: the run only has records from beat 12 on, and bar 4 of the example drums is nearly silent, so only the hit at beat 16 shows; with a wait of 3 s the picture fills (Keys/gate shows all 16 steps).
- **Two real bugs fixed:** (1) `DeviceBar.cpp` `paintPicture` returned early for the slicer, so the chip showed no picture (added `getSlicer() == nullptr` to the early-return test); (2) the guide's *How DawIt Slicer works* page: the *Step memory* box's subtitle overlapped the recording/held labels (box taller, subtitle now "two slices"). The *Repeat or echo?* ms labels (0, 156, 312, 469 ms) are clean.
- **Docs:** README *Features* (Shape and mix sounds), `docs/tests.md`, `docs/code-layout.md`, `docs/architecture.md` (the play head line and the device bar pictures), `docs/recipes.md` step 6. `./build.sh --test` ends "All checks passed", no warnings.
- No facts, numbers or texts from *Theory* were changed.

### Theory check
- *Mute: a trance gate* page: "a 30 ms fade takes up more than a quarter of every gap" was wrong, since the engine caps every fade at a quarter of its piece (`startFade`); now says 30 ms would be more than a quarter, so the fade is kept to a quarter of the step, 27 ms (107.1 / 4).
- *Clicks and smoothing* page: the quarter-of-the-piece limit now reads as applying to every fade (short Roll pieces, or a long Smooth on short steps), not only to Roll on 1/32.
- *How DawIt Slicer works* item 4: the stopped clock runs at the tempo in use (song's or Own), not always the song's; item 5: with Own, the steps still count from the start of the song while it plays (Group A deviation 2).
- Smooth's explainer text adds "A fade is never longer than a quarter of its piece."
- *Slices of time* picture note: "Taller lines are the beats" became "Brighter lines" (the beat lines are the same height, drawn in the stronger grid colour).
- Re-checked Sound On Sound's 1 to 3 ms fades and 2 to 4 ms crossfades (agrees; already in SOURCES.md). Test expected values match *Test numbers*.
- Open (code, not text): the Smooth readout (`describeSettings`) and the *Mute* page's note and fade drawing use Smooth uncapped, so above a quarter of the step they show e.g. "48% of a step" and a longer fade than the slicer makes.
