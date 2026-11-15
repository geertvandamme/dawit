# Guided exercise: mix and export

Status: Released
Idea: [#208 — Guided exercise: mix and export](https://github.com/geertvandamme/dawit/issues/208)
Part of: [#195](https://github.com/geertvandamme/dawit/issues/195). Same shared model as the released exercises (`src/generate/Exercises.{h,cpp}`, `ExerciseWindow`, `GuidePointer`; plans `guided-exercise-learn-the-chords-of-a-song.md`, `guided-exercise-write-lyrics-to-your-song.md`). Built beside [#207](https://github.com/geertvandamme/dawit/issues/207) (`guided-exercise-make-a-track-sound-better.md`, in progress): see *Shared with #207*.

## Goal

An exercise that finishes a song: balance the faders, place the parts left and right, measure how loud the whole song is, and export it as a WAV. Each step ticks from the real song (faders and pans compared with where they were, a whole-song loudness measurement, a finished bounce of the mix), points at the mixer and links to the guides. Two new Gain staging topics, *Balancing the tracks* and *Panning the parts*, give the balance and pan steps a page of their own, with live pictures of the song. A beginner ends with a file to play to other people and knows the order of a mix: balance first, then place, then check.

## Decisions for you

1. **Guide content for balance and pan.** Nothing in DawIt explains how to set a balance or where parts usually go (Gain staging touches the fader, the Stereo guide what a pan knob does). Options: (a) two new topics at the end of Gain staging, *Balancing the tracks* and *Panning the parts*, each with a live picture of the song; (b) links to existing pages only, with the method in the step texts. **Recommended:** (a): the steps need a page that says how and why, and the pictures show the user's own faders and pans. #64 (the stereo image guide) can later take the pan topic over and extend it.
   Answer: (a) two new Gain staging topics.
2. **When steps 1 and 2 tick.** Options: (a) changed from where they were when the exercise opened: a track's fader by 1 dB or more, a track's pan by 10% or more; (b) the same, moved while the song plays; (c) "levels differ": not all faders at the same value. **Recommended:** (a): a real action, no tick from a song that came balanced (the example song does), and it works with the mixer, the headers or the inspector.
   Answer: (a) changed since the exercise opened (fader ≥ 1 dB, pan ≥ 10%).
3. **When "check the loudness" ticks.** Options: (a) a measurement of the whole song (*Measure Loudness...*), with the step's button measuring the whole song even when the loop is on; (b) a DawIt Meter heard on a bus that holds the whole mix; (c) the meter window open (the issue's wording). **Recommended:** (a): integrated loudness only means something for the whole song, and the master has no effects, so (b) needs a bus most songs don't have.
   Answer: (a) a whole-song loudness measurement.
4. **When "export" ticks.** Options: (a) a finished bounce of *The mix*, *Whole song*, that doesn't clip (a 16- or 24-bit file peaking over 0 dB doesn't tick; a 32-bit float one ticks, with a tip); (b) any finished bounce. **Recommended:** (a): the step is about a finished song file, and a clipped file is the one mistake that can't be undone in the file.
   Answer: (a) a finished whole-song mix bounce that doesn't clip (32-bit float over 0 dB ticks with a tip).
5. **Where it opens.** Options: Help > *Mix and Export Your Song...* only; also an item in File after *Measure Loudness...*; also the master strip. **Recommended:** Help only, like the lyrics exercise; an empty song has nothing to mix, so no Start panel tile. #195's list of exercises will add more places.
   Answer: Help only.

## What the user does and sees

**Opening.** Help > *Mix and Export Your Song...*, after the last exercise item (after *Make a Track Sound Better...* when #207 is in, else after *Write Lyrics to Your Song...*); no shortcut; description "Balance the faders, pan the parts, check the loudness and export a WAV of your song, in four steps". The shared `ExerciseWindow`, titled "Mix and export your song", about 700 high (check by shot: four rows).

**Baseline.** When the exercise first opens in a session and project, and on *Start over*, it remembers each track's fader and pan (every track except the master; returns and buses count). Tracks added later are not in it and never tick; a deleted track is skipped (checked with `Engine::indexOf`, never dereferenced). `afterReplace` clears it and the two records below.

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Balance the levels | a baseline track's fader differs from its baseline by ≥ 1.0 dB | *Open the mixer* (shows the mixer panel, then *Show me*) | the fader of the **shown track** in its mixer strip (opens the mixer first) |
| 2 Pan the parts | a baseline audio or MIDI track's pan differs from its baseline by ≥ 0.10 (10%) | *Open the mixer* | the pan knob of the **pan candidate**, else the shown track |
| 3 Check the loudness | the **measure record** covers the whole song | *Measure the whole song* (the whole song even with the loop on; opens the loudness guide at *Your track*, as *Measure Loudness...* does) | the master strip's meter |
| 4 Export | the **export record**: the mix, whole song, not clipped (decision 4) | *Export the song...* (the Bounce to WAV window, started on *The mix* and *Whole song*) | the master strip |

- **Shown track**: the selected audio or MIDI track, else the first with a clip.
- **Pan candidate**: the first audio or MIDI track that the name rule (below) calls a guitar, keys, pad/ensemble or backing vocal; none: the generic texts.
- **Name rule**: use `soundbetter::getSource` (and its backing-vocal words) when #207 is in the tree; else `verb::kinds::getSource` on the name, with backing, bv, bvs and harmony meaning a backing vocal. **Centre parts**: kick, snare, drums, bass and lead (not backing) vocal.
- **Measure record** (in memory): the readings of the last song measurement made through `MainComponent::measureLoudness` (the File command, the guide's *Measure the song* and the step's button all go there), and whether it covered the whole song or the loop (with its bars).
- **Export record** (in memory): the last bounce that finished (not cancelled, not failed) through the Bounce to WAV window: what (mix, selected tracks, multitracks or stems), whole song or loop (bars), bit depth, sample rate, peak, length and file name.

**Statuses and tips**: all in *Theory* with their conditions. Steps 3 and 4 render offline and need no output.

Nothing changes the song: the buttons open the mixer, measure and open the export window; the exercise never moves a fader or pan itself. **Remembered:** only `learn.mixExport.done`. **First steps:** no change: the exercise needs a song with tracks, and no command, menu or control the first steps name changes.

## Data and saving

No project data, no format change. In memory (`MainComponent`): the baseline and the two records.

## The guide

Two topics appended to **Gain staging** (`gainstaging::Topic`, after *Step by step*, whose step 5 "balance the tracks with their faders" leads into them; appending keeps the existing topic indices). Texts in *Theory*.

- **Balancing the tracks** — "How loud each part is against the others". Picture: a row per track (`routing::getVisibleTracks`, master last) in its colour: its live level as a bar and its fader setting as a marker with the dB, on a -30 to +6 dB scale. A fader below -20 dB is marked; the master's row has a -6 dB line, its peak amber above -6 dB and red above 0. With no tracks: the example song's faders, captioned "An example, not a rule".
- **Panning the parts** — "A place for each part, left to right". Picture: a line from L to R with a dot per audio and MIDI track at its pan, in its colour, named; behind it a centre band labelled "kick, snare, bass, lead vocal" and side zones labelled "guitars, keys, pads, backing vocals". A centre part panned ≥ 25% gets an amber ring and "usually in the middle"; with 3 or more tracks all centred, the caption suggests panning one. With no tracks: the example song's pans.

These pictures are the topics' "your song" part (Gain staging has no *Your song* topic). The `gen::guides` entry: summary gains "and how to balance and pan the tracks"; search words add "balance balancing static mix pan panning left right"; not on *Start here* (unchanged).

**Links** (`TheoryGuideLinks.cpp`):
- gainStaging · Balancing the tracks | loudness · Loudness while mixing
- gainStaging · Balancing the tracks | eq · Making room: masking
- gainStaging · Panning the parts | stereo · Left, right and in between
- gainStaging · Panning the parts | stereo · Why the low end stays in the middle
- gainStaging · Panning the parts | sound · Checking in mono

**Exercise links:** 1: Gain staging · *Balancing the tracks*, *Effects, fader and master*. 2: Gain staging · *Panning the parts*; Stereo · *Why the low end stays in the middle*. 3: Loudness · *Loudness while mixing*, *Targets and normalisation*. 4: Routing · *Bouncing to audio*; Gain staging · *Over 0 dB*. Carry-on: Handing over · *Mixing or mastering?*.

Loudness and Stereo are guides built on a plugin: their pages come from `BuiltInGuides::getPageNames`, not `getEntries()`. #207's step 5 makes `openExerciseLink` resolve them and its step 3 makes the engine links test skip them (shots check them). If #207 isn't built yet, do both here.

## Code

Recipes: *A guided exercise* (copy *Write Lyrics to Your Song*), *Topics in an existing guide*, *A command or menu item*, *Checking in the real app*.

- **`gen::mixexport`** (new, `src/generate/MixExport.{h,cpp}`, both targets): texts, the baseline type, the name rule and centre parts, the pan candidate, the four done rules, statuses, tips and done notes, and the two record types (the measure record holds `loudness::Readings`). Pure, on the `Engine` and the context. Pan text as the headers show it ("C", "35 L").
- **`gen::exercises`**: register `mixExport` (stem `learn.mixExport`, links, carry-on, help text, height); `Context` gains the baseline, the two records, *mixer showing*. All four buttons are the steps' own actions (`Fix::none`, dispatched on the step); statuses use the existing `importAudio` and `audioSettings` fixes. No new `Fix` values.
- **`bounceui`**: a listener set once by `MainComponent` (like `setGuideOpener`), called on the message thread after a bounce finishes, with the facts of the export record; `showDialog` gains a way to start on *The mix* and *Whole song*.
- **`loudnessui::measureSong`**: an option to measure the whole song whatever the loop; the result says which range it measured.
- **`MixerView`**: areas for a track strip's fader and pan knob and the master strip and its meter, for the pointer.
- **`GainStagingExplainer`**: shows a song picture in place of the chain picture for the two new topics; both print in `--export-guides`.
- **Invariants:** no tick from a click; read volumes and pans from the tracks' atomics on the message thread; find tracks fresh, never keep an index. No audio-thread or format change. Risk: low; the Bounce and Measure changes must leave their default behaviour exactly as it is (existing tests unchanged).

**Shared with #207** (build in either order; the second merges, not replaces): `Exercises.{h,cpp}` (enum, `allExercises`, `numExercises`, `getInfo`, dispatch, `Context`), `MainComponent.{h,cpp}` (command IDs, Help menu order, `showExercise`'s *Start over*, `afterReplace`, `getExerciseContext`, `runExerciseAction`, `showExerciseTarget`, `openExerciseLink`), `DevScreenshots`, `CMakeLists.txt`, `tests/EngineTests.cpp` (`main`, the links test), `SOURCES.md` (Gain staging), `docs/recipes.md` *A guided exercise* (heights), `docs/tests.md`, `docs/code-layout.md`, the README exercises bullet. If #180 renames *Bounce to WAV...* first, the step 4 texts follow the new names.

## Libraries and models

None. No `LICENSE`, `licenses/` or *License note* change.

## Theory

### Facts

1. A static mix: a first balance with the faders and pan only, before EQ or compression; it shows what needs fixing. Opinion (common practice); iZotope static mix (read), LANDR and Violet Recording (search).
2. Start in the busiest part, usually a chorus; a balance there carries to quieter parts. Typical; iZotope (read).
3. Start with the foundation (drums and bass) or the lead vocal, then add parts one at a time. Opinion; iZotope (read), Violet (search).
4. EQ and compression shift the balance, so come back to the faders. Opinion; Violet (search).
5. The ear's balance of bass and treble changes with level (equal-loudness contours, ISO 226); louder seems better. Exact (the curves) / typical; EQ guide, Gain staging · *Louder sounds better*.
6. Mix at one moderate level; quoted levels vary (about 70 to 85 dB SPL), so the text gives none. Opinion; search (Sonarworks, Production Expert).
7. Peaks aren't loudness: a snare can peak high and sound quieter than a pad. Typical; Gain staging · *Quiet, healthy, hot, over*.
8. Faders within about 10 dB of 0 dB are easiest to fine-tune. Opinion; Gain staging · *Effects, fader and master*.
9. Master peaks around -6 dB while mixing. Typical; Gain staging · *Step by step*.
10. Kick, snare, bass and lead vocal usually sit in the middle. Typical; Stereo · *Left, right and in between*, iZotope panning (read), MusicRadar (search).
11. Low sounds stay in the middle: both speakers share them; solid in mono and on club systems. Typical; Stereo · *Why the low end stays in the middle*.
12. A part played twice (a doubled guitar) panned hard left and right sounds wide and full. Typical; iZotope panning (read).
13. Two different parts in one range go to opposite sides, often only part of the way (about 20%). Opinion; iZotope panning (read).
14. Backing vocals go off centre so the lead keeps the middle. Typical; search (MusicRadar, musicguymixing).
15. DawIt's pan keeps the near side and turns the far side down by cos(pan × 90°); all the way out the far side is silent. Exact (`Track.cpp`); Stereo guide says the same.
16. A one-channel part panned all the way out keeps half its power: -3.0 dB (10 log10 0.5); halfway, -1.25 dB (10 log10 0.75). Exact (derived).
17. Added to mono, a part panned all the way out is 6.0 dB quieter than in the middle (20 log10 0.5). Exact (derived).
18. Programs pan differently (pan laws). Typical; Wikipedia *Panning law* (search).
19. Check on headphones, a phone or a car too. Opinion; iZotope panning (read: headphones, car).
20. Integrated loudness is the whole song's; measure from start to end. Exact; ITU-R BS.1770, loudness guide.
21. No loudness target while mixing; keep the true peak below about -1 dBTP: encoding to AAC or Ogg can push peaks closer to 0 over it. Typical; loudness guide (*Loudness while mixing*, *True peak*).
22. Spotify plays songs at about -14 LUFS; turns louder ones down, quieter ones up only while the true peak stays at -1 dBTP or lower (its example: -20 LUFS, -5 dBTP → +4 dB). Typical (service policy, changes); Spotify (in SOURCES under Loudness).
23. Not every service turns quiet songs up; normalisation can be off. Typical; loudness guide.
24. Making a song loud is mastering's job, at the end. Typical; loudness guide, Gain staging · *Over 0 dB*.
25. A 16- or 24-bit file can't hold a peak over 0 dB (clips); 32-bit float keeps it. Exact; Gain staging · *Over 0 dB*.
26. 44.1 kHz for CD and streaming, 48 kHz for video; 24-bit the usual choice. Typical; the Bounce window, Sound · *From air to numbers*.
27. Distributors accept 44.1 kHz WAV; CD Baby asks for 16-bit, DistroKid takes most. Typical; search (both support pages; fetch refused). Text says "if a distributor asks for 16-bit at 44.1 kHz, export that".
28. Turn down by ceil(peak + 1) dB to fix a clipped bounce. Exact (DawIt's Bounce window rule).
29. *Let the end ring out* keeps up to 4 s after the end; it is on for the whole song. Exact (Bounce window).
30. A bounce renders offline, faster than the song plays, and changes nothing in the song. Exact; Routing · *Bouncing to audio*.
31. DawIt doesn't move faders over time (no automation yet). Exact.
32. DawIt's thresholds (1 dB, 10%, 25% centre tip, -20 dB fader tip) are its own choices. Opinion.

### Topic texts (Gain staging)

**Balancing the tracks** — "How loud each part is against the others"

"A mix starts with the balance: how loud each part is against the others. Set it with the faders alone, before any EQ or compression. Engineers call this first balance the static mix. It shows what really needs fixing, and often that's less than you thought.

How to set one:
1. Play the busiest part of the song, usually a chorus, where the most parts play at once. A balance that works there usually works in the quieter parts too.
2. Start with the part the song is built on: in most pop and rock that's the drums and the bass, or the lead vocal. Mute the rest for a moment.
3. Unmute the other parts one at a time. Bring each fader up until you hear the part clearly, then back a little.
4. Play the whole song and fix what gets lost or sticks out.

Habits that help:
- Listen at a moderate level, one you could talk over, and keep it there. The ear hears bass and treble differently at different volumes, and louder always seems better.
- Judge by ear, not by the meters. A snare can peak high and still sound quieter than a pad.
- Keep the master's peaks below about -6 dB. If they go higher, pull all the faders down a little, together.
- Faders within about 10 dB of 0 dB are easiest to fine-tune. If a part needs its fader far down, turn down its source instead: the instrument's volume, or the clip's gain.
- Come back to the balance after EQ and compression, which change how loud a part seems. Check it on headphones and a phone too.

The picture shows your tracks: each fader as a mark with its setting, and the level coming through it as a bar. With no tracks, it shows the example song's faders, as an example, not a rule.

This page doesn't cover changing a level over the song. A part that's right in the verse may be too loud in the chorus; DawIt can't move a fader over time yet."

**Panning the parts** — "A place for each part, left to right"

"The pan knob places a part between the left and the right speaker. Parts that all sit in the middle compete to be heard; panning gives each a place of its own, so each is clearer and the mix gets wider.

Where parts usually go (habits, not rules):
- In the middle: the kick, the snare, the bass and the lead vocal. They carry the song, and the lowest sounds are best shared by both speakers: they stay solid on every system, even in mono.
- To the sides: guitars, keys, pads, backing vocals and percussion. Backing vocals off centre leave the middle to the lead.
- A part played twice, such as a guitar recorded two times, sounds wide and full with one take hard left and the other hard right.
- Two different parts in the same range, such as a rhythm guitar and a piano, get on better on opposite sides, often only part of the way, such as 20% left and 20% right.

How DawIt pans: turning the knob to the left leaves the left side as it is and turns the right side down; all the way left, the right side is silent. So a part recorded in one channel, such as a voice or a guitar, gets up to 3 dB quieter as you pan it from the middle all the way out (about 1.25 dB halfway). Check its level again after panning. Other programs pan in slightly different ways, called pan laws.

Check in mono now and then. In mono both sides are added, so the places disappear, and with DawIt's pan a part panned all the way out comes out 6 dB quieter than one in the middle. If a part almost vanishes in mono, pan it less far. Checking in mono, in What is sound?, plays the song in mono.

The picture shows your tracks along the line from left to right, with the usual places behind them. A kick, snare, bass or lead vocal panned a quarter or more of the way out is ringed. With no tracks, it shows the example song.

This page doesn't cover making one part itself wider (DawIt Stereo does) or placing parts front to back with reverb."

### Exercise texts

Title "Mix and export your song". Intro: "Four steps to a finished file of your song: balance the levels, place the parts left and right, check how loud it is, then export it as a WAV you can play anywhere. Do each step in the main window: the tick appears by itself."

1. **Balance the levels** — "How loud each part is against the others"
   - Do: "Press Open the mixer (Cmd+2) and play the busiest part of your song, usually a chorus. Start with the part the song is built on, the drums and bass or the lead vocal. Then set each other part's fader until you hear it clearly, and back a little. The tick comes when a fader has moved 1 dB or more."
   - Why: "Balance is most of a mix. Engineers set it with the faders alone, before any EQ or compression, because it shows what really needs fixing. A balance that works where the most parts play usually works in the quieter parts too. Listen at a moderate level, one you could talk over, and keep it there: at different volumes the ear hears bass and treble differently, and louder always seems better. Judge by ear, not by the meters: a snare can peak high and still sound quieter than a pad."
   - Status, no tracks with sound: "Your song has no tracks with sound yet. Make some with Help > First Song Exercise..., or import a recording with File > Import Audio...." Fix: import audio.
   - Status, no output: "DawIt has no audio output, so you won't hear the balance. Choose an output in Audio/MIDI Settings...." Fix: audio settings.
   - Status, only the master moved: "You moved the master, which turns the whole song up or down. The balance is between the tracks: move a track's fader instead."
   - Tip, far down: "Tip: <Pad>'s fader is at <-26.0> dB. Faders within about 10 dB of 0 dB are easiest to fine-tune. If a part is far too loud, turn down its instrument or its clip's gain, then bring the fader back up."
   - Done: "<Bass> moved from <-4.0> to <-6.5> dB." (the largest move)
   - Button "Open the mixer", tooltip: "Shows the mixer under the tracks (Cmd+2): a fader, a pan knob and a meter for every track."
   - Show me: "<Bass>'s fader: drag it up or down while the song plays."
2. **Pan the parts** — "A place for each part, left to right"
   - Do, with a candidate: "Turn the pan knob of <Keys>, above its fader, to the left or right while the song plays. Keep the kick, snare, bass and lead vocal in the middle, and send parts like guitars, keys and backing vocals to the sides. A part played twice sounds wide with one take on each side."
   - Do, without: "Turn a track's pan knob in the mixer to the left or right while the song plays. Keep the kick, snare, bass and lead vocal in the middle, and send parts like guitars, keys and backing vocals to the sides. A part played twice sounds wide with one take on each side."
   - Why: "Parts that all sit in the middle compete to be heard. Panning gives each one a place of its own, so each is clearer, and the mix gets wider. The lowest and most important parts usually stay in the middle: both speakers share them, so they stay solid on every system, even in mono. In DawIt, panning a part all the way out makes it up to 3 dB quieter, so check its level again after."
   - Status, no tracks / no output: as step 1.
   - Tip, centre part panned: "Tip: <Bass> is panned <40 R>. The kick, snare, bass and lead vocal usually sit in the middle, so the base of the song is solid in both speakers. Double-click its pan knob to centre it, unless you want it there."
   - Done: "<Keys> is now <35 L> (it was <20 L>)."
   - Button "Open the mixer", as step 1.
   - Show me: "<Keys>'s pan knob: drag it to move the part left or right. Double-click centres it."
3. **Check the loudness** — "How loud is the whole song?"
   - Do: "Press Measure the whole song. DawIt plays the song into a loudness meter, faster than real time and without a sound, and shows the numbers in What is loudness?. File > Measure Loudness... does the same, but with the loop on it measures only the loop."
   - Why: "Peaks show whether something clips, not how loud a song sounds: the ear follows how much sound arrives over time. A loudness meter measures that in LUFS, the unit streaming services use. Integrated loudness is the average of the whole song, so it's measured from the start to the end. While you mix there's no target to hit: just leave room, with the true peak below about -1 dBTP. Streaming services play every song at about the same loudness. Spotify, for one, plays songs at about -14 LUFS: it turns louder ones down, and quieter ones up only as far as their peaks allow. Making a song loud is a job for mastering, at the end."
   - Status, empty song: "Your song has nothing to measure yet. Add a clip to a track first."
   - Status, loop only: "That measured only the loop, bars <13> to <20> (<0:20>). Press Measure the whole song for the song's loudness."
   - Done: "Integrated <-18.4> LUFS, true peak <-3.2> dBTP: <3.2> dB of headroom." Then one of: " Spotify would turn it up <2.2> dB, as far as its peaks allow, to about <-16.2> LUFS." (quieter than -15); " Spotify would play it <4.0> dB quieter." (louder than -13); " That's about where Spotify plays songs." (within 1 LU); " Its peaks leave Spotify no room to turn it up." (quieter, turn-up ≤ 0.05 dB). Not every service turns songs up; the step's *Why* says so through "quieter ones up only as far as their peaks allow".
   - Tip, over 0 dBTP: "Tip: the true peak reaches <+1.3> dBTP, over 0, so the song can clip once exported or played. Turn the tracks down by about <3> dB (the master works too) and measure again."
   - Tip, -1 < true peak ≤ 0: "Tip: the true peak is <-0.4> dBTP, less than 1 dB below 0. Streaming services turn songs into smaller files (AAC or Ogg), which can push peaks this close over 0. Leave at least 1 dB."
   - Button "Measure the whole song", tooltip: "Renders the whole song, even with the loop on, without playing it, and measures its loudness the way DawIt Meter does. Nothing in the song changes."
   - Show me: "The master's meter shows peaks as the song plays. For loudness, measure the whole song: press Measure the whole song."
4. **Export** — "A WAV file of your song"
   - Do: "Press Export the song.... The Bounce to WAV window opens on The mix and Whole song, with Let the end ring out on, so the last reverb isn't cut off. 44.1 kHz and 24-bit suit most uses; if a distributor asks for 16-bit at 44.1 kHz, choose that. Press Bounce..., choose where to save, and DawIt writes the file, faster than the song plays."
   - Why: "Exporting, or bouncing, records what DawIt plays into a file: every track through its effects, buses and faders, then the master. Your song doesn't change, and muted tracks stay silent. A 16- or 24-bit file can't hold a peak over 0 dB: the loudest moments are cut off, which sounds harsh. That's why the loudness step checks the peaks first. A 32-bit float file keeps them, but most players and services need a fixed-point file."
   - Status, selected tracks: "That file holds only the selected tracks. For the song, choose The mix under What."
   - Status, files: "That made one file per <track> (<multitracks>). For the song as one file, choose The mix under What."
   - Status, loop: "That file is only the loop, bars <13> to <20>. Choose Whole song under Range."
   - Status, clipped: "The mix peaks at <+1.4> dB, so the <24>-bit file clips on its loudest moments. Turn the tracks down by about <3> dB (the master works too) and export again."
   - Tip, 32-bit over 0: "Tip: the file peaks at <+1.4> dB. 32-bit float keeps that, but played as it is, or turned into another format, it clips. Turn the tracks down by about <3> dB for a file to share."
   - Done: "<My Song.wav>: <3:12>, <24>-bit, <44.1> kHz, peak <-2.1> dB."
   - Button "Export the song...", tooltip: "Opens Bounce to WAV on The mix and Whole song. Nothing is written until you press Bounce... and choose where to save."
   - Show me: "The master: everything you hear goes through it into the file. Press Export the song..., or choose File > Bounce to WAV... (Cmd+B)."

Completion: "Done. Your song is balanced, placed, measured and exported: a file you can play to anyone. Listen to it somewhere else, on headphones, a phone or in a car, and note what you'd change: then change it and export again. To make it as loud as released songs, or to hand it to an engineer, read on." Carry-on "Next: mastering or an engineer", tooltip "Opens Sending stems to an engineer at Mixing or mastering?: what mastering does, and which files an engineer wants."

### Example data

- Example song (`buildPopSong`): Kick -2 dB C, Snare -4 C, Hats -9 25 R, Bass -4 C, Keys -7 20 L, Lead vocal 0 C, BV 1 -9 40 L, BV 2 -9 40 R, Drums bus. Pan candidate: Keys. 96 BPM; Chorus 1 bars 13 to 20.
- Spotify turn-up = min(-14 - I, -1 - TP): I -18.4, TP -3.2 → min(4.4, 2.2) = 2.2 dB, to -16.2; I -20, TP -5 → min(6, 4) = 4.0 (the loudness guide's example).
- Fix amount = ceil(peak + 1): +1.3 → 3; +1.4 → 3.
- DawIt pan, one-channel part: centre L = R = 1 (power 2); 50% L: R = cos 45° = 0.7071, power 1.5 → -1.25 dB; 100% L: power 1 → -3.01 dB; mono sum 1 vs 2 → -6.02 dB.

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| Step 1, Bass baseline -4.0: set -4.5 / -5.0 / -6.5 | not / done / done, "-4.0 to -6.5 dB" | 0.01 |
| Master moved 3 dB only | not done, master status | |
| Track added after baseline at -6, deleted baseline track | not done, no crash | |
| Fader tip at -26.0 / -20.0 | tip / none | |
| Step 2, Keys baseline -0.20: set -0.25 / -0.35 | not / done, "35 L (it was 20 L)" | |
| Pan text 0 / -0.35 / 0.4 | "C" / "35 L" / "40 R" | |
| Centre tip: Bass 0.40 / Bass 0.20 / BV 1 -0.40 / Lead vocal -0.30 | tip / none / none / tip | |
| Pan candidate, example song / tracks Kick, Bass only | Keys / none | |
| DawIt pan gains (from `Track`'s rule): far side at 50%; power at 100%, 50%; mono sum at 100% | 0.7071; -3.01, -1.25 dB; -6.02 dB | 0.01 |
| Step 3, whole song I -18.4 TP -3.2 | done; "3.2 dB of headroom", "up 2.2 dB", "-16.2" | 0.05 |
| I -20 TP -5 / I -10 TP -0.3 / I -14.5 / I -16 TP -0.5 | up 4.0 / 4.0 quieter + 1 dB tip / "about where" / "no room" + 1 dB tip | 0.05 |
| TP +1.3 | over-0 tip, "about 3 dB" | |
| Loop-only record | not done, "bars 13 to 20" | |
| Step 4: mix whole 24-bit peak 0.79 / loop / selected / stems / 24-bit peak 1.17 / 32-bit peak 1.17 | done "peak -2.0 dB" / not / not / not / not, "about 3 dB" / done + tip | 0.1 dB |
| Registration | 4 steps, 2 links each, carry-on, stem `learn.mixExport` distinct, `clampStep` 4 → 0, help text ends "in four steps", no `<` left in any text | |
| Gain staging | 10 topics; the two new names; each has text; search "panning" finds Gain staging | |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `gen::mixexport` (`src/generate/MixExport.{h,cpp}`, both CMake targets): baseline, name rule and centre parts, pan candidate, the four done rules, the two record types, every text of *Theory* with statuses, tips and done notes. Tests: the step rows of *Test numbers* on made-up edits (example song via `buildPopSong`, records filled by hand). Check: placeholders filled for the example song.
- [x] 2. Register `mixExport` in `gen::exercises` (see *Shared with #207*: merge with what's there). Links test: plugin guides' pages skipped if #207 hasn't done it. Tests: the registration row.
- [x] 3. Gain staging: the two topics and texts (`GainStaging.{h,cpp}`), the `gen::guides` summary and words, the five links. Tests: the Gain staging row, the DawIt pan gains row against `Track`'s rule, links name existing topics. Done when `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 4. `bounceui` listener and *Mix + Whole song* start; `loudnessui::measureSong` whole-song option and measured range. Check: File > Bounce to WAV... and Measure Loudness... behave as before (existing tests unchanged).
- [x] 5. `MainComponent`: Help command, `showExercise (mixExport)`, baseline on first open and *Start over*, clears in `afterReplace`, records from the bounce listener and `measureLoudness`, context, actions and *Show me* per the table (open the mixer first); `MixerView` areas; `openExerciseLink` plugin pages if #207 hasn't. Check: each pointer lands on its control.
- [x] 6. `GainStagingExplainer`: the two song pictures (example song with no tracks). Check: `--export-guides` writes the Gain staging PDF with both pictures.
- [x] 7. `ScreenshotRunner`: `"exercise": "mixExport"`; keys `"trackVolume": {"Bass": -6.5}` and `"trackPan": {"Keys": -0.35}` (set like the mixer does, after the baseline), `"exerciseMeasured": true` (whole-song measurement now, recorded), `"exerciseExported": {"what": "mix", "range": "song", "bits": 24, "peakDb": -2.1}` (fills the record, writes no file). Add them to `docs/recipes.md` *Checking in the real app*. Done when `./build.sh --test` passes without warnings and the other exercises' shots look as before.

Group C, check and docs:
- [x] 8. Real-app check (example song; `"capture": "window"` for the exercise, `"main"` for pointers): steps 1–4 with `exerciseShowMe` (fader of Bass after `"select": "Bass"`, Keys' pan knob, master meter, master strip); step 1 after `trackVolume`, step 2 after `trackPan` and with Bass at 0.4 (tip); step 3 after `exerciseMeasured` (done note) and with the loop on via `loopSection` then File measure (loop status); step 4 after each `exerciseExported` variant (done, loop, clipped); completion; empty song (no tracks status); `guide: gainStaging` at both new topics, with the example song and `"song": "empty"`; one shot per plugin link (`loudness` and `stereo` topics) so a wrong page name fails. Look for: cut text at 700 high, pointers not covering their target, numbers matching *Test numbers*.
- [x] 9. Docs: README *Features* (exercises bullet; Gain staging's two topics); `docs/tests.md` (mixexport, Gain staging topics); `docs/code-layout.md` (`MixExport`); `docs/recipes.md` *A guided exercise* (height, export and measure records as a pattern for exercises that read a finished job); issue #208 progress (Done / Still open) and #195's checklist; a line on #64 that *Panning the parts* exists for it to build on.

## Tests

All in `tests/EngineTests.cpp`, `runMixExportTests()`: the *Test numbers* rows; every text non-empty with no placeholder; links resolve (non-plugin in `getEntries()`); a deleted track in the baseline neither ticks nor crashes. In the Gain staging tests: ten topics, texts, the pan gain numbers. The other exercises' tests unchanged.

## Website hand-off

- `features.html`, guided exercises group: one line on this exercise; a shot of the mixer with the exercise at step 2 (Keys' pan knob pointed at).
- `learn.html`: list it with the other exercises; mention Gain staging's balance and pan pages, with a shot of *Panning the parts* on the example song.
- No Home, `for-you.html` or `about.html` change.

## Left for later

- A full stereo image guide (drummer's or audience's view, depth, width, mono button): #64.
- Bus-by-bus balance advice against a style: #66; the mix analysed with fixes: #67.
- Fader moves over the song (automation): #87.
- Mastering (a limiter to a target): #61; an MP3 or FLAC export and *Play*, *Reveal*, *Export again* after a bounce: #180.
- A pointer to the exercise from the Gain staging and loudness guides.

## Changes while building

**Group A (engine), built.**
- `gen::mixexport` (`src/generate/MixExport.{h,cpp}`, both CMake targets): `Progress { Baseline baseline; const Track* selected; MeasureRecord measured; ExportRecord exported; bool hasOutputDevice; }`; `takeBaseline (engine)` (every track plus `masterDb`, `valid`); `MeasureRecord { valid, wholeSong, startBeat, endBeat, loudness::Readings readings }`; `ExportRecord { valid, What what (mix, selectedTracks, multitracks, stems), wholeSong, startBeat, endBeat, bitDepth, sampleRate, peak (linear), seconds, fileName }`. Group B fills the records (the bar numbers come from the beats, 4/4). Also: `isStepDone/getDoText/getStatus/getDoneNote`, `getActionLabel/Tooltip (step)`, `getFixLabel/Tooltip (fix)` (importAudio, audioSettings), `getShowMeText (step, trackName)`, `findShownTrack (engine, selected)`, `findPanCandidate`, `findFaderMove/findPanMove`, `formatPan`, `formatLength`, `getPanGains/getPanPowerDb/getMonoSumDb`, `clips (bits, peak)`.
- `gen::exercises`: `Exercise::mixExport` registered (5 exercises); `Context` gained `mixBaseline`, `mixSelected`, `mixMeasured`, `mixExported`, `mixerShowing` (the last unused by the rules, for the pointer). The step's own buttons return `Fix::none`; `MainComponent` dispatches on the step.
- Gain staging: `Topic::balance`, `Topic::pan` appended (10 topics); texts, summary, search words and the five links are in. The song pictures are group B (`GainStagingExplainer` still shows the chain picture for them).
- Help menu order, `exerciseWindows` size and `openExerciseLink` plugin pages: group B (the `MainComponent` loop uses `numExercises`, so the array already has 5 slots). #207 already makes the links test skip plugin guides.
- Tests: `runMixExportTests()` after `runSoundBetterTests()`; the exercises done-keys test now expects 5 keys (#207 had it at 4).

**Differences from *Theory*:**
- Centre parts: a track on the drum sound or named hats, cymbals or percussion is not a centre part (the name rule calls those drums, and the example song's Hats at 25 R would get a wrong tip); a drum track counts only when its name has drum, drums or kit. Kick, snare, bass and a lead vocal as planned.
- Step 3, done note: when the song would reach -14 LUFS without hitting the peak limit, the line says "Spotify would turn it up X dB, to about -14 LUFS." (without "as far as its peaks allow"); the planned wording is used when the peaks limit it.
- Step 4 files status: "one file per track (multitracks)" or "one file per bus or track outside a bus (stems)".
- The no-output status of step 2 says "balance" as in step 1 (the plan says "as step 1").
- The DawIt pan test checks the formula (`getPanGains`, copied from `Track::process`'s rule), not a render through a `Track`.

**Group B (UI), built.** `./build.sh --test` ends with `All checks passed`, no warnings; `--export-guides` writes the Gain staging PDF.
- `bounceui` (`BounceDialog.{h,cpp}`): `setFinishedListener (std::function<void (const gen::mixexport::ExportRecord&)>)` (set in the `MainComponent` constructor; fills `mixExported`; called on the message thread after a finished, not cancelled or failed, mix, selected-tracks or files bounce, with the peak, length and file or folder name) and `showDialog (..., bool pickMixSong)` (starts on *The mix* and *Whole song*). Defaults unchanged.
- `loudnessui` (`LoudnessActions.{h,cpp}`): `measureSong (engine, onFinished, bool wholeSong = false)` and `getLastRange()` (`MeasuredRange { wholeSong, startBeat, endBeat }`, cleared by `clearLast`; `measureSongNow` sets it too). `MainComponent::measureLoudness (bool wholeSong = false)` records every measurement through `recordMeasurement`.
- `MainComponent`: Help > *Mix and Export Your Song...* (`mixExportExerciseCommand`, after *Make a Track Sound Better...*); `mixBaseline` (taken when the window first opens and on *Start over*), `mixMeasured`, `mixExported`; `resetMixExportMemory (bool takeBaseline)` (called with false from `afterReplace`); `runMixExportAction` and `showMixExportTarget` (steps 1-2 open the mixer and point at the fader or pan knob of the shown track or pan candidate, step 3 at the master meter, step 4 at the master strip); `MixerView::getFaderOf/getPanKnobOf/getMasterStrip` and `ChannelStrip::getFader/getPanKnob/getMeterArea`. `openExerciseLink` already handled plugin guides (#207), so no change there.
- `GainStagingExplainer`: `SongPicture` (balance and pan modes) with `gainstaging::getSongSnapshot (const Engine*)`; the constructor takes the engine (DeviceBar passes it, both sites); the example song's rows are used with no tracks. Difference: the pan picture is a row per audio or MIDI track (name left, dot at its pan) over the centre band and side zones, not one shared line, so names never overlap.
- `ScreenshotRunner`: `"exercise": "mixExport"`, `trackVolume`, `trackPan`, `exerciseMeasured`, `exerciseExported`, run after the exercise window opens (before it when no exercise is named). Documented in `docs/recipes.md` (*Checking in the real app*) and `DevScreenshots.h`.
- For group C: the shots in step 8 can all be taken with these keys; not yet looked at in the real app (pointers, window height 700, picture layout at 320 high with the example song's 10 rows are unchecked).

**Group C (check and docs), built.** Shots (scratch `shots.json`, three runs, window 360 x 700): steps 1-4 plain, `exerciseShowMe` on Bass's fader, Keys' pan knob, the master meter and the master strip; step 1 after `trackVolume`, step 2 after `trackPan`, step 3 after `exerciseMeasured`, step 4 after the song, loop and clipped (16-bit, +0.3 dB) records; completion; empty song (no tracks status); `gainStaging` at *Balancing the tracks*, *Panning the parts* (live, example song and empty), the five plugin-link pages (all names resolve); the Gain staging PDF pictures. Nothing needed fixing.
- Seen: every step fits at 700 high (the tallest, step 3 with its tip and done note, ends at about 590); each pointer lands on its control. The Bass fader pointer's caption covers the neighbouring strips' pan knobs, and the master strip caption covers the *+ Instrument* chip; both go with the pointer's next click.
- Not reachable by shot: the step 3 loop status (the `loudness` scene measures the whole song and does not go through `MainComponent::measureLoudness`); it is covered by the engine tests. The Bass-at-0.4 tip shows as the done note once the fader has moved 1 dB, so the tip itself only shows for a small move.
- Docs: README (the exercise and Gain staging's two topics, in the guided-steps bullet), `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md` (*A guided exercise*: records of a finished job).

**Theory check.**
- `GainStagingExplainer.cpp` example rows: "Drums bus" at -3 dB became "Drums" at 0 dB, as in `buildPopSong` (buses start at 0 dB).
- Balance caption for a far-down fader: now "is below -20 dB. Turn down its source instead (its instrument or clip's gain, or for a bus the tracks in it), then bring the fader back up.", agreeing with the topic and step 1's tip (it said to turn the others down).
- Balance caption: "The master's line marks -6 dB: keep its peaks below it." (was "The master's line is -6 dB.").
- *Panning the parts*: "If a part almost vanishes in mono" became "gets lost in mono" (a part panned with DawIt's pan only drops 6 dB in mono; vanishing comes from phase, not panning); the ringed sentence names a whole drum kit too, as `isCentrePart` does.
- *Balancing the tracks* and step 1's *Why*: "louder always seems better" became "almost always", as in *Louder sounds better*.
- `SOURCES.md` (Gain staging): the Spotify loudness page, re-read for step 3's done note.
- Open points fixed afterwards: (1) step 3's done note drops the headroom phrase when the true peak is over 0 ("Integrated -12.0 LUFS, true peak +1.3 dBTP." and the over-0 tip follows); (2) step 1's far-down tip for a bus or return says "If it is far too loud, turn down the tracks in it, then bring the fader back up." and the balance picture's caption names "the tracks in it" for a row that is not an audio or MIDI track; (3) the example balance picture has the example song's Backing vocals and Vocals buses at 0 dB (11 rows plus the master, fits the 320 high picture). Tests: bus tip, over-0 note.
