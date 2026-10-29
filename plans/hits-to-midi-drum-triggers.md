# Trigger MIDI from audio: drum triggers and drum replacement

Status: Built, not committed; Theory checked
Idea: [Trigger MIDI from audio: drum triggers and drum replacement](../FEATURE_IDEAS.html#trigger-midi-from-audio-drum-triggers-and-drum-replacement)

## Goal
Right-click an audio clip with drums (a recording, a loop, a drum stem) and choose *Hits to MIDI...*: DawIt finds every hit, shows them on the waveform, and makes a MIDI track below with a note per hit, at the same time and with a velocity from how loud it was, played by DawIt Drums. A beginner can turn a loop into a part they can read and edit, replace a weak snare, or layer a kick, and a new guide, *How drum triggers work*, explains transients, thresholds, bleed, velocity, the grid and phase.

## Decisions for you
1. **The live Trigger device** (a device on an audio track that sends notes to another track during playback). Options: (a) leave it for later; (b) add it to this plan as Group D. **Recommended: (a)**, because DawIt has no way yet for one track to send notes to another (a new routing concept, with latency to make up), which is a feature of its own; the offline tool covers replace, layer and loop-to-part now.
   Answer: (a) Leave it for later.
2. **Where the theory goes.** Options: (a) a new guide *How drum triggers work* in the MIDI area; (b) pages in *What is DawIt Drums?*; (c) a topic in *What is MIDI?*. **Recommended: (a)**: eight topics plus *Your clip* is too much for one topic, and it isn't about DawIt Drums' sound.
   Answer: (a) A new guide in the MIDI area.
3. **Two drums at once.** With *Sort by drum*, each hit becomes one note (the band with most energy wins), so a hi-hat played with a kick is lost. Options: (a) one note per hit now, per-band finding later; (b) find hits in each band separately now (needs bleed rules between bands, harder to make reliable). **Recommended: (a)**; the guide says so plainly and markers can be fixed by hand.
   Answer: (a) One note per hit now.
4. **Minimum gap and Dead zone** in the idea are the same thing (time after a hit in which nothing new counts). **Recommended:** one control, *Minimum gap*.
   Answer: One control, Minimum gap.
5. **Listen** needs a small new engine player (a buffer played on top of everything, like the sound guide's tone): an audio-thread change. Options: (a) build it; (b) no Listen: make the track, then play the song. **Recommended: (a)**: hearing hits against the original before making the track is the point of the window.
   Answer: (a) Build Listen.
6. **The new track's instrument:** (a) DawIt Drums as a plugin (recommended: the kit with knobs and its own guide, as song templates use); (b) the built-in synth's Drums sound (lighter, no window).
   Answer: (a) DawIt Drums as a plugin.

## What the user does and sees
**Opening.** An audio clip's right-click menu gets *Hits to MIDI...* right after *Convert to MIDI...* (in `TrackLanes`). It finds the candidates behind a progress window with Cancel (a second or two for a song), then opens the window (one per clip, through `PluginWindowManager`; it closes when the clip is deleted). The *Convert to MIDI* dialog's text gets: "For drums, use Hits to MIDI... instead."

**The window** (resizable, 1000 × 620, minimum 760 × 480):
```
┌ Hits to MIDI: "Drum loop" ─────────────────────────────── [?] ┐
│ bars ruler                                                     │
│ waveform; a marker per hit in its drum's colour; faint ticks   │
│ for rises below Sensitivity; the Sensitivity line; playhead    │
├────────────────────────────────────────────────────────────────┤
│ Find:    Sensitivity [-30 dB]   Minimum gap [30 ms]            │
│ Notes:   Write as [Sort by drum ▾]   Velocity [Natural ▾]      │
│ Timing:  (•) Keep feel  ( ) Snap to [1/16 ▾] Strength [100 %]  │
│ Original:(•) Keep playing (layer)  ( ) Mute the clip (replace) │
│ 18 hits in 2 bars (9 a bar): 5 kick, 5 snare, 8 hi-hat         │
│ [▶ Listen]  Hear [Both ▾]              [Cancel] [Make MIDI track] │
└────────────────────────────────────────────────────────────────┘
```
- **Sensitivity** −6 to −60 dB (default −30): the quietest hit that counts, below the loudest. **Minimum gap** 5 to 250 ms (default 30). Both re-pick instantly from the candidates; no new analysis.
- **Write as:** Sort by drum, Kick (36), Snare (38), Hi-hat (42), guessed from the track and clip names (kick, snare, hat/hihat/hi-hat; otherwise Sort by drum). **Velocity:** Natural, Gentle, Strong, Same for all (100).
- **Timing:** Keep feel (default) or Snap to 1/4, 1/8, 1/16, 1/32, 1/8 triplet, 1/16 triplet with Strength 0 to 100% (step 5).
- **Listen** plays the clip from the start (or from a clicked spot) through the engine's audition player and toggles to Stop; **Hear**: Original, Both, Hits. Listening stops the transport; pressing Play stops listening.
- **Make MIDI track** adds the track and closes the window; the new track is selected. **Cancel** or closing makes nothing.
- **Markers:** click selects (and shows its line in the status: "Hit 7 · 1.52 s · bar 1, beat 4 · Snare · velocity 87"); drag moves it to the strongest rise within 10 ms of the mouse (Alt: exactly there); click an empty spot adds a hit the same way; double-click plays 0.4 s from that hit; Delete removes the selected one. Right-click a marker: Kick, Snare, Hi-hat (ticked), Delete hit, separator, *How drum triggers work...*; right-click empty: Add hit here, Undo hand edits, separator, the guide. Cmd+scroll or pinch zooms, scroll pans.
- Hand edits are kept as a list over the automatic hits and re-applied after each re-pick (a remove or move matches the nearest hit within 10 ms); *Undo hand edits* clears them.
- The **?** button and the menus' last item open the guide at the topic the mouse was over (the waveform: *Finding the hits*; Velocity: *How hard*; Timing: *On the grid*; Original: *Replace or layer*).
- Tooltips: in *Theory*. Over the waveform: "1.52 s · Bar 1, beat 4 · Snare, velocity 87" or "2.10 s · Bar 2, beat 1 · Click to add a hit here".
- **Remembered** in app settings: `hits.sensitivityDb`, `hits.gapMs`, `hits.velocity`, `hits.snap`, `hits.grid`, `hits.strength`, `hits.original`, `hits.hear`. *Write as* is guessed per clip.

**Afterwards:** a MIDI track "<source name> hits" right below the source track, with DawIt Drums, one clip at the audio clip's place and length; the source track is untouched (only the clip's mute when *Mute the clip* was chosen).

## Data and saving
Nothing new in the project: the result is an ordinary MIDI track, clip and plugin, and the replace choice is the existing clip mute. No format version change. Test: a made track (notes 36, 38, 42 with their velocities, DawIt Drums, the muted source clip) saves and loads unchanged.

## The guide
New guide **How drum triggers work** (`gen::guides` area MIDI; not on *Start here*: it's for a task, not a first step). Content `src/engine/DrumTriggerGuide.{h,cpp}` (`daw::triggerguide`), window `src/ui/DrumTriggerExplainer.{h,cpp}`, owned by `MainComponent` like the recording guide. Topics, in order:

| Topic | Summary | Picture |
|---|---|---|
| What a drum trigger is | Hits in a recording become notes | The example loop's waveform with markers, its notes in kick, snare and hi-hat lanes below |
| Transients | The fast rise at the start of a hit | The first 60 ms of the example snare hit, the peak marked, the marker at the start of the rise |
| Finding the hits | A sensitivity and a minimum gap | The loop's level in dB with the Sensitivity line (the user's setting), the rise curve under it, hits as dots, the double hit with its gap shaded |
| How hard: velocity | Louder hits become higher velocities | The four curves, 0 to −40 dB against 1 to 127, the chosen one bright, the example points |
| Kick, snare or hi-hat | Telling drums apart by their frequencies | The spectra of the example kick, snare and hi-hat, 30 Hz to 11 kHz, the borders at 150 Hz and 6 kHz, each band's share in % |
| False and missed hits | Bleed, cymbals, rolls and flams | The example snare track with bleed and ghost notes, lines at −30, −24 and −18 dB and which markers each keeps |
| On the grid or as played | Snap with a strength, or keep the feel | Eight played hits on a 1/16 grid, at 0%, 50% and 100% |
| Replace or layer | And why the two must line up | Two 60 Hz waves in line and 8.3 ms apart, with their sums |
| Your clip | What was found, and what to try | The user's hits on the bar grid in drum lanes, with the counts |

*Your clip* follows the open Hits to MIDI window's current hits; with none open, the selected MIDI clip on a track that plays drums (DawIt Drums, or the Drums or Percussion sound), its notes mapped by `drums::getDrumForNote`. Clicking a picture plays it (the loop, the snare hit, the two kicks).

Entry: summary "Turning drum hits in a recording into MIDI notes: transients, sensitivity and gap, velocity, telling kick, snare and hi-hat apart, false hits, the grid, and replacing or layering with the phase lined up."; opens from "An audio clip's right-click menu (Hits to MIDI...), the ? in the Hits to MIDI window"; keywords: "trigger triggers drum replacement replace replacer layer layering sample samples augment hits hit onset onsets transient transients threshold sensitivity retrigger mask gap bleed spill ghost flam roll kick snare hihat hi-hat velocity quantise quantize snap groove feel phase polarity comb audio to midi drums to midi stem loop".

## Code
Follows recipes.md *A long job* (copy `TempoActions`/`AudioToMidiActions`), *A theory guide* (copy the recording guide), *A command or menu item* (the clip menu), *A window*.

- **Hit finding** (`src/analysis/HitFinder`, `daw::analysis::hits`), pure and tested. Two stages so the controls are instant: *candidates* (one long-job pass) and *picking* (on every change).
  - Candidates: mono at 22.05 kHz through `tempofinder::prepareInput`, the 40-band rise of `tempofinder::findOnsets` (factor out the band flux if it can't be reused as is) with hop 128 (5.8 ms); every local peak of the rise above a small floor (tuned in the tests) is a candidate. Its time is refined on the source audio to the sample: the earliest sample in the 10 ms before the peak sample (and after the previous candidate) whose absolute value reaches 10% of the hit's peak. Each candidate keeps its time in seconds, its rise, its peak level in dB (the largest sample in the 30 ms after the time) and its energy below 150 Hz, 150 Hz to 6 kHz and above 6 kHz over the first 50 ms. Checks a "should exit" flag and reports progress.
  - Picking: keep candidates whose level is within *Sensitivity* dB of the loudest candidate; walk in time order and drop any within *Minimum gap* after the last kept one (first wins). Drum: the band with the most energy (kick, snare, hi-hat), or the chosen single drum. Velocity: `127 × 10^(L / k)` with L the level in dB below the loudest kept hit *of the same drum*, k = 40 (Natural), 60 (Gentle), 20 (Strong), clamped to 1..127; Same for all is 100. Hand edits (add, move, remove, set drum) re-applied last.
- **Hits to MIDI** (`src/engine/HitsToMidi`, `daw::hitstomidi`): hits to clip notes through the tempo map (`getBeatAfter` from the clip start); snapping is to the song's grid (absolute beats, not the clip start), `start + strength × (nearest − start)`; note length 1/16 (0.25 beat), shortened to end before the next note of the same pitch; notes past the clip end dropped. Making the track: new MIDI track at `indexOf (source) + 1`, `engine.addPlugin` with `BuiltInDrums::getDescription()` as `SongTemplates` does, the clip under the lock, `notesChanged()`, and the source clip muted only on *replace*. Never touches the source track's instrument or effects. Also the Listen render: the clip (at its gain), the hits through `drums::render` with DawIt Drums' starting kit and velocity scaling, or both, at the device rate; and the example loop for the guide, the tests and the screenshot scene (rendered from the guide's example data).
- **Audition player** in `Engine`, played right after `renderTone` in `processBlock`, never offline: a buffer set and cleared under `getLock()` (the audio thread only reads it while it holds the try-lock), the old buffer freed on the message thread after the lock is released; read position and "playing" are atomics the window reads for its playhead. Starting the transport stops it.
- **Risky:** the audio-thread change (no allocation, no freeing on the audio thread; never heard in a bounce), and the candidate floor and band edges, which must be tuned on the rendered example loop, not only on clicks.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | A transient is the fast rise at the start of a sound; a drum hit's most important part is its first few ms | gate guide (already states it); Wikipedia *Onset (audio)* | typical |
| 2 | Not every onset has a sharp transient (bowed, sung, pads) | Wikipedia *Onset (audio)* | exact (definition) |
| 3 | Onsets are found with a detection function (rise of energy per band, spectral flux) then peak picking with a threshold | Bello et al. 2005, IEEE TSAP 13(5) (via search); Wikipedia | exact (method) |
| 4 | Simple level-only detection gives many false or missed onsets | Wikipedia *Onset (audio)* | typical |
| 5 | A mask/retrigger time ignores triggers just after a hit (kick beater bounce); Roland's runs 0–64 ms; keep it low or fast notes drop out | Roland SPD-SX PRO manual | exact (that product) / typical |
| 6 | 1/16 notes at 120 BPM are 125 ms apart; 1/32 at 240 BPM 31.25 ms | derived: 60/120/4, 60/240/8 | exact |
| 7 | A hit 5.8 ms step: 128 / 22050 = 5.80 ms; a sample at 44.1 kHz = 0.0227 ms | derived | exact |
| 8 | GM drum map: 36 kick (Bass Drum 1), 38 snare (Acoustic Snare), 42 closed hi-hat | General MIDI 1 (DawIt Drums already uses it) | exact |
| 9 | Kick tone ~40–70 Hz; snare body ~150–250 Hz plus wire noise above; hi-hats above ~6–7 kHz | Drums and gate guides (DrumsGuide.cpp knob texts, GateGuide bands) | typical |
| 10 | DLS maps velocity to attenuation as 40 log10(127/v) dB; MIDI 1.0 only says an exponential mapping is preferred; GM2 leaves velocity's effect undefined | midi.org forum thread quoting DLS, GM2 and MIDI 1.0 | exact (the curve), with the caveat |
| 11 | Velocity's effect is up to the instrument | MIDI guide (same words) | exact |
| 12 | Quantise strength moves a note that fraction towards the nearest grid line; 100% full, 0% none | Logic Pro help (Q-Strength) | exact |
| 13 | 50–90% strength tightens while keeping feel; the amount is taste | Logic tutorials (search) | opinion |
| 14 | Replace mutes the original; double/layer keeps it; the trigger notes go on a new instrument track below | Logic Pro help, drum replacement | exact (that tool) |
| 15 | A timing offset is normally 0 ms; change it for an instrument that answers late | Logic Pro help | typical |
| 16 | Two equal waves in line sum +6.02 dB; τ apart they cancel at 1/(2τ) and odd multiples: 60 Hz half period 8.33 ms; 1 ms → 500, 1500, 2500 Hz | derived: 20 log10 2; f = (2n+1)/(2τ) | exact |
| 17 | Layered drums must start together and move the same way, or the drum thins; check by zooming, flip polarity if needed | Sound On Sound, *How to replace & reinforce recorded drums* | typical |
| 18 | Bleed from other drums is the main cause of false triggers and can be as loud as ghost notes | Sound On Sound (same); recording.org thread (search) | typical |
| 19 | Engineers rarely trigger hi-hats: keep the recording or program them | Sound On Sound (same) | opinion |
| 20 | Snare rolls and flams are hard to trigger | Sound On Sound (same) | typical |
| 21 | One sample on every hit sounds machine-like ("machine-gunning"); several samples per velocity help | Sound On Sound (same) | typical |
| 22 | At 120 BPM a beat is 0.5 s; 1.5 s from beat 8 is beat 11; 30 ms = 0.06 beat | derived | exact |
| 23 | Kick and snare make the backbeat with the snare on 2 and 4 | common knowledge; the styles guide | typical |

No disagreement with existing guides: the bands match the drums and gate guides; velocity matches the MIDI guide.

### Topic texts

**What a drum trigger is** — *Hits in a recording become notes*

A drum trigger listens to a recording of drums and writes a note every time something is hit. Producers use it for three things:

• Replace a sound: a snare that was recorded thin or dull plays a better snare instead, hit for hit.
• Layer a sound: a second drum plays under the original, such as a deep kick under a kick recorded in a small room. The room stays; the new kick adds weight.
• Turn a loop into a part: a drum loop or a drum stem becomes MIDI notes you can see, edit and play with any kit.

In DawIt, right-click an audio clip and choose Hits to MIDI.... The hits become notes in a new MIDI track below, at the same times, with the velocity from how loud each hit was, played by DawIt Drums. The original audio stays where it is, so you can keep it, mute it, or play both.

A drum trigger only asks when something was hit and how hard. It doesn't hear pitch. For a bass, a voice or a melody, use Convert to MIDI... instead: it finds pitched notes, and drums have no clear pitch, so it isn't made for them.

The picture shows a two-bar drum loop at 120 BPM: the waveform with a marker on every hit, and below it the notes the hits become.

**Transients** — *The fast rise at the start of a hit*

A transient is the fast rise at the start of a sound. When a stick hits a snare, the level jumps from almost nothing to its loudest within a few milliseconds, then dies away over a tenth of a second or more. A kick's beater, a hand clap and a plucked string do the same.

That rise is what a trigger looks for. It's the clearest sign that something new has started: a sound that's dying away only gets quieter, and a held note stays about level.

A hit's time is the start of its rise, not its peak. DawIt first finds the hit in steps of 5.8 ms, then looks back for the first sample that reaches a tenth of the hit's peak (20 dB under it). At 44.1 kHz one sample lasts 0.023 ms, far finer than anyone can hear.

Not every sound has a sharp transient. A bowed cello, a pad or a softly sung note fades in, and where it starts is a matter of opinion. Triggers work best on drums and other things that are struck.

The picture shows the first 60 ms of a snare hit: the rise, the peak, and the marker at the start of the rise.

**Finding the hits** — *A sensitivity and a minimum gap*

DawIt listens to the clip in 40 frequency bands and asks, every 5.8 ms, how much each band got louder. Added up, that's the rise curve in the picture: flat while a sound holds or fades, with a spike at every hit. Finding note starts this way is called spectral flux, and it's the usual first step in onset detection.

Two settings decide which spikes count as hits:

• Sensitivity is how quiet a hit may be, measured from the loudest hit in the clip. At −30 dB a hit counts if its peak is no more than 30 dB under the loudest. Raise it (towards −12 dB) when markers appear on bleed or noise; lower it (towards −45 dB) when soft hits are missed.
• Minimum gap is how long after a hit nothing new counts. At 30 ms, a second spike 12 ms after a hit is taken as part of it: a kick's beater bouncing back, or a rattle of the snare. Drum modules have the same setting, often called mask time or retrigger: Roland's runs from 0 to 64 ms. Keep it shorter than the fastest real notes: sixteenth notes at 120 BPM are 125 ms apart, and 32nd notes at 240 BPM are 31 ms apart.

Any marker can be fixed by hand: drag it to move it, click an empty spot to add a hit, or select one and press Delete.

The picture shows the example loop's level with the sensitivity line, the rise curve under it, the hits that count, and a double hit 12 ms apart where the gap keeps only the first.

**How hard: velocity** — *Louder hits become higher velocities*

Every MIDI note has a velocity from 1 to 127: how hard it's played. A trigger takes it from how loud each hit was, compared with the loudest hit of the same drum. The loudest snare gets 127; quieter snares get less.

How much less is the velocity curve:

• Natural: a hit 12 dB quieter gets velocity 64, and one 20 dB quieter gets 40. This is the reverse of the curve the DLS standard for sample players uses to turn velocity into level (40 × log10 (127 ÷ velocity) dB quieter), so a player that follows it plays each hit back about as loud as it was.
• Gentle: softer hits keep more velocity. 20 dB quieter gets 59.
• Strong: the differences grow. 20 dB quieter gets 13, so ghost notes stand clearly apart.
• Same for all: every hit gets 100, the way a drum machine plays.

Velocity isn't loudness itself: what it does is up to the instrument. MIDI only asks that it be applied smoothly, and few instruments follow the DLS curve exactly. DawIt Drums turns velocity into level with its own Velocity knob; a sampled kit may switch to a recording of a softer hit. So the same notes can sound more or less even on different kits.

The picture shows the four curves from 0 to −40 dB, with the example's hits as dots.

**Kick, snare or hi-hat** — *Telling drums apart by their frequencies*

With Sort by drum, each hit becomes the drum whose part of the spectrum holds most of its energy in its first 50 ms:

• Kick: below 150 Hz. Its tone usually sits between 40 and 70 Hz, below most bass notes. Written as note 36.
• Snare: 150 Hz to 6 kHz. A body around 150 to 250 Hz and the noise of the snare wires above it. Written as note 38.
• Hi-hat: above 6 kHz, where a hi-hat's metal is. Written as note 42, the closed hi-hat.

These are General MIDI's drum notes, so the part plays a kick, a snare and a hi-hat on DawIt Drums and on almost any drum kit.

It's a rule of thumb, and it has limits. One hit gets one note: when the kick and the hi-hat hit together on beat 1, the one with more energy (usually the kick) wins, and that hi-hat is missing from the part. Toms, claps, rides and crashes have no note of their own here: a tom usually sorts as a kick or a snare, a crash as a hi-hat. Fix them with a right-click on the marker before you make the track, or later in the piano roll.

Without Sort by drum every hit becomes the one drum you choose, which is right for a single drum such as a snare recording.

The picture shows the spectrum of a kick, a snare and a hi-hat from the example loop, with the borders at 150 Hz and 6 kHz and how much of each hit's energy falls in each part.

**False and missed hits** — *Bleed, cymbals, rolls and flams*

A trigger hears level, not drums. Anything loud enough that rises counts as a hit; anything too quiet doesn't.

• Bleed: every microphone on a kit also hears the other drums (this is also called spill). In a snare microphone the kick can be as loud as the snare's own ghost notes, the very soft strokes between the backbeats. Raise Sensitivity until the bleed's markers go, and add ghost notes back by hand if they matter. A stem from Split into stems... usually has less bleed than a microphone, but rarely none.
• Cymbals and hi-hats ring for a long time and wash over everything. Their ring doesn't rise, so it seldom makes a hit by itself, but it can hide quiet hits under it. Many engineers don't trigger hi-hats at all: they keep the recording, or program the part.
• Rolls and flams: a flam is two strokes so close together they sound like one fat hit; a roll is a fast run of soft strokes. A long Minimum gap merges them into one; a short one finds each stroke, but may also find a kick's bounce. Snare rolls are among the hardest things to trigger well.
• All the same: when every hit plays the same sample, fast runs can sound like a machine gun. Velocity helps; kits that switch between several recordings of each hit help more.

Where the detector can't tell, look at the waveform and move, add or delete markers yourself.

The picture shows a snare recording with snare hits at 0 dB, two ghost notes at −22 dB and the kick bleeding through at −26 dB. At −30 dB everything counts, the bleed too; at −24 dB the bleed goes and the ghost notes stay; at −18 dB only the backbeats are left.

**On the grid or as played** — *Snap with a strength, or keep the feel*

Hits are found to the sample, in seconds. DawIt's notes sit on the timeline in beats, so each time goes through the song's tempo: at 120 BPM a beat lasts 0.5 s, so a hit 1.5 s into a clip that starts on bar 3 (beat 8) lands on beat 11. Tempo changes are followed.

Then you choose:

• Keep feel leaves every hit where it was played. A drummer is never exactly on the grid, and the small pushes and drags are part of the groove. This is where DawIt starts.
• Snap to the grid moves each hit towards the nearest grid line (1/16 is a sixteenth note). Strength says how far: 100% puts it on the line, 50% halfway. At 120 BPM a hit 30 ms late (0.06 beats) ends up 15 ms late at 50%, and on the line at 100%. Many producers use 50 to 90% to tidy a loose part and keep some of its life; how much is a matter of taste.

Snapping only helps when the song's tempo matches the recording. If the hits drift further from the grid bar after bar, set the tempo first (Detect tempo... in the clip's menu).

The picture shows eight played hits and where they go at 0%, 50% and 100%.

**Replace or layer** — *And why the two must line up*

The new track can be used in two ways:

• Layer: the original keeps playing and the new drum plays with it. The room, the player's touch and the new drum's weight all stay. A common use: a deep kick under a kick recorded in a small room.
• Replace: the original is muted and only the new drum plays. Choose Mute the clip in Hits to MIDI, or mute it later from the clip's right-click menu.

When you layer, the two must line up. A kick's 60 Hz wave rises and falls every 16.7 ms. Play two copies exactly together and they add up: twice the swing, 6 dB louder. Move one by half a cycle, 8.3 ms, and one pushes while the other pulls: the low end cancels and the kick goes thin. Smaller shifts cancel higher frequencies: two copies 1 ms apart cancel at 500 Hz, 1.5 kHz, 2.5 kHz and so on, a hollow sound called comb filtering.

DawIt puts every note at the start of the hit's rise, so a drum that sounds at once lines up with the original. Some samples begin with a little silence, and some instruments answer a few milliseconds late. To check, zoom in on both: they should start together and swing the same way first. If the layer sounds thinner than either drum alone, move the notes by a millisecond or two, or flip the polarity of one with a plugin that can.

The picture shows two 60 Hz kick waves: in line, and 8.3 ms apart, with what you hear when they add up.

**Your clip** — *What was found, and what to try*: the general text is the rules below, joined; with nothing to look at: "Right-click an audio clip with drums and choose Hits to MIDI..., or select a drum part, and this page looks at it."

### Tooltips
- Sensitivity: "How quiet a hit may be and still count, in dB below the loudest hit. Raise it (towards −12 dB) if markers appear on bleed or noise; lower it (towards −45 dB) if soft hits are missed."
- Minimum gap: "After a hit, nothing new counts for this long, so one hit isn't found twice (a beater bouncing back, a rattle). 30 ms suits most parts; shorten it for fast rolls."
- Write as: "Sort by drum: kick (36), snare (38) or hi-hat (42) by where each hit's energy is. Or every hit as one drum, for a recording of a single drum."
- Velocity: "How loudness becomes velocity. Natural: 12 dB quieter is velocity 64. Gentle keeps soft hits louder, Strong makes them softer, Same for all gives every hit 100."
- Keep feel: "Leaves every hit where it was played, so the groove stays."
- Snap to: "Moves the hits towards the nearest line of this grid."
- Strength: "How far each hit moves towards the grid: 100% onto the line, 50% halfway, 0% not at all."
- Keep playing: "Layer: the original audio keeps playing with the new drums."
- Mute the clip: "Replace: mutes the original clip, so only the new drums play. Unmute it any time from its right-click menu."
- Listen: "Plays the clip with the hits on top, from the start or from where you clicked. Click again to stop."
- Hear: "What Listen plays: the original, the hits as DawIt Drums will play them, or both."
- Make MIDI track: "Puts the hits in a new MIDI track below, with DawIt Drums. The original stays."
- ?: "How drum triggers work: transients, sensitivity, velocity, bleed and lining up a layer."

### Example data
- **Loop:** 2 bars 4/4 at 120 BPM (4.0 s), from DawIt Drums' starting kit. Kick at beats 0, 2, 4, 6, 6.75, level 0 dB; snare at 1, 3, 5, 7, −2 dB, played 6 ms late; ghost snare at 3.75, −24 dB; closed hi-hat on the off-beats 0.5, 1.5 ... 7.5 (8 hits), alternating −14 and −16 dB, 0 to 8 ms late; a kick bounce 12 ms after beat 4 at −12 dB. Expected at −30 dB and 30 ms: 18 hits (5 kick, 5 snare, 8 hi-hat; the bounce merged). Natural velocities: kicks 127; snares 127, ghost 127 × 10^(−22/40) = 35.8 → 36; hats 127 and 127 × 10^(−2/40) = 113.2 → 113.
- **Velocity curves** at 0, −6, −12, −20, −30, −40 dB: Natural 127, 90, 64, 40, 23, 13; Gentle 127, 101, 80, 59, 40, 27; Strong 127, 64, 32, 13, 4, 1.
- **Snare track:** snare 0 dB at beats 1, 3; ghosts −22 dB at 1.75, 3.5; kick bleed −26 dB at 0, 2, 2.5. Kept: −30 dB: 7; −24 dB: 4; −18 dB: 2.
- **Grid:** eight 1/16 hits at 120 BPM, off by +12, −8, +30, +4, −20, +16, −4, +24 ms; at 50% each is half that; at 100% 0.
- **Phase:** 60 Hz, period 16.67 ms; in line peak ×2 (+6.02 dB); 8.33 ms apart: silence; 1 ms apart: notches at 500, 1500, 2500 Hz.

### "Your clip" rules
Input: hits (beat, drum, velocity 1–127), the clip's length in bars, the tempo. Each line is said only when its condition holds.
1. Always: "18 hits in 2 bars (9 a bar): 5 kick, 5 snare, 8 hi-hat." (the drum counts only when sorted or drum notes).
2. Average a bar within 10% of 4, 8 or 16: "That's 4 hits a bar on average, as many as there are quarter notes (8: eighth notes; 16: sixteenth notes): a steady pulse." Said for all hits and for the hi-hats alone ("The hi-hat plays 4 a bar...").
3. At least 4 snare hits and 75% or more within 1/8 beat of beats 2 and 4: "Most snare hits land on beats 2 and 4: the backbeat of rock and pop."
4. Two hits of the same drum less than 30 ms apart (N pairs): "N pairs of hits are less than 30 ms apart. That's a flam if the drummer played one, or one hit found twice: try a Minimum gap of 50 ms."
5. Velocities spread less than 8 (max − min): "Every hit has about the same velocity (95 to 100). If the drummer played accents and ghost notes, try the Strong curve; for a machine-like part this is right." Quiet when Same for all is chosen.
6. Snare hits at velocity 40 or less (N ≥ 1): "N quiet snare hits (velocity 40 or less): ghost notes, the soft strokes between the backbeats. Keep them if you can hear them in the original."
7. Mean distance from the 1/16 grid, in ms at the song's tempo, with at least 8 hits: under 3 ms: "The hits sit on the 1/16 grid (on average 2 ms off): played to a click, programmed or snapped."; 3 to 15 ms: "On average 9 ms off the 1/16 grid: tight, with a human feel. Keep feel keeps it."; over 15 ms: "On average 24 ms off the 1/16 grid: a loose part, a shuffle or triplet feel, or a song tempo that doesn't match the recording. Check the tempo (Detect tempo...) before snapping."

### Test numbers
| What | Expected | Tolerance |
|---|---|---|
| 8 clicks 0.5 s apart (44.1 kHz) | 8 hits at 0.0, 0.5 ... 3.5 s | ±0.5 ms |
| Clicks at 120 BPM, clip at beat 8 | notes at beats 8–15 (clip beats 0–7) | ±0.002 beat |
| Double hit 12 ms apart, gap 30 ms / 5 ms | 1 hit / 2 hits | exact |
| Hits at 0, −12, −20 dB, Natural | 127, 64, 40 | ±1 |
| Gentle / Strong at −20 dB | 59 / 13 | ±1 |
| Same for all | 100 | exact |
| 60 Hz burst / 8 kHz noise burst / 200 Hz tone + 1–4 kHz noise | 36 / 42 / 38 | exact |
| Sensitivity on the snare track: −30, −24, −18 dB | 7, 4, 2 hits | exact |
| Snap: hit at beat 4.06, 1/16, 0 / 50 / 100% | 4.06 / 4.03 / 4.0 | ±1e-9 |
| Example loop, −30 dB, 30 ms | 18 hits: 5 kick, 5 snare, 8 hi-hat, each within 2 ms of its beat time | counts exact |
| 60 Hz sum in line / 8.33 ms apart | +6.02 dB / below −40 dB | ±0.05 dB |
| 1 ms comb notches | 500, 1500, 2500 Hz | exact (formula) |
| 30 ms at 120 BPM | 0.06 beat | ±1e-9 |

Text checks: every topic has a name, summary, text and example; the guide is found by "drum replacement", "bleed" and "transient"; every *Your clip* rule fires on a made-up part and stays quiet otherwise.

## Steps

Group A1, engine: finding hits (`./build.sh --test` at step 4)
- [x] 1. `src/analysis/HitFinder.{h,cpp}`: candidates (Code), in both CMake targets. Tests: `runHitFinderTests` with clicks (times ±0.5 ms), silence (none), cancel. Done when: the tests pass.
- [x] 2. Picking: sensitivity, gap, drum by band, velocity curves, hand edits. Tests: the gap, velocity, sorting and sensitivity rows of *Test numbers*; a moved, an added and a removed hit survive a re-pick.
- [x] 3. `src/engine/HitsToMidi.{h,cpp}`: notes through the tempo map with song-grid snap and strength, note lengths, the example loop render, the Listen render. Tests: the beat, snap and example-loop rows.
- [x] 4. Making the track: below the source, DawIt Drums, mute only on replace, source track's plugins and sound unchanged. Tests: those checks, plus save and load of the result (*Data and saving*). Done when: `./build.sh --test` passes without warnings.

Group A2, engine: audition and guide content (`./build.sh --test` at step 6)
- [x] 5. The audition player in `Engine` (Code). Tests through the fake device: plays the buffer sample for sample, advances the position, stops at the end and when the transport starts, silent in `renderOffline`.
- [x] 6. `src/engine/DrumTriggerGuide.{h,cpp}` (`daw::triggerguide`): topics, names, summaries, texts and tooltips from *Theory*, the example data, the *Your clip* analysis (also used for the window's status line). Tests: the text checks, the curve and phase numbers, each rule on a made-up part.

Group B1, UI: the Hits to MIDI window (`./build.sh --test` at step 9)
- [x] 7. `src/ui/HitsToMidiActions.{h,cpp}` (owned by `ArrangementView`) runs the candidates on a progress window and opens the window; *Hits to MIDI...* in the clip menu; the line in the Convert to MIDI dialog. Check: the menu item finds hits on a demo audio clip.
- [x] 8. `src/ui/HitsToMidiWindow.{h,cpp}`: waveform, markers in drum colours (one static colour helper for kick, snare, hi-hat), the controls, the status line, every interaction and tooltip from *What the user does and sees*, settings under `hits.*`.
- [x] 9. Listen and Hear through the audition player, with the window's playhead; Make MIDI track selects the new track. Done when: `./build.sh --test` passes without warnings.

Group B2, UI: the guide (`./build.sh --test` at step 11)
- [x] 10. `src/ui/DrumTriggerExplainer.{h,cpp}` (copy `RecordingExplainer`): the pictures from *The guide*, clicks that play, PDF export; *Your clip* from the open Hits window or the selected drum clip. `Guide::triggers` in `gen::guides` with its entry, `MainComponent::openGuide` and `getPrintGuide` cases, opening from the window's ? and menus. Tests: `runTheoryGuidesTests` search words.
- [x] 11. `ScreenshotRunner`: a `drumLoop` scene that adds an audio track "Drum loop" with the example loop and opens Hits to MIDI on it, sorted by drum. `--export-guides` still passes. Done when: `./build.sh --test` passes without warnings.

Group C, check and docs
- [x] 12. Real-app check (one `--screenshots` run): (a) `drumLoop` scene, capture the window: 18 markers in three colours, status "18 hits in 2 bars"; (b) the same with `hits.sensitivityDb` −18 in `settings`: fewer markers, faint ticks for the rest; (c) after Make MIDI track (add to the scene as an option): the arrangement with "Drum loop hits" below, DawIt Drums in the device bar; (d) guide shots at *Kick, snare or hi-hat*, *Finding the hits*, *Replace or layer* and *Your clip* with the window open. Look for clipped text, wrong numbers, empty pictures.
- [x] 13. Docs: README Features bullet (near Convert to MIDI), docs/tests.md (hit finder, hits to MIDI, audition, drum trigger guide), docs/code-layout.md (the new files), FEATURE_IDEAS status (Partly done → Done or Partly done if the Trigger device stays open), architecture.md (the audition player beside the tone; `HitsToMidiActions` among the long jobs; the guide in *Opening a guide at a topic*).

## Tests
In `tests/EngineTests.cpp`: `runHitFinderTests`, `runHitsToMidiTests`, the audition checks in the engine tests, `runDrumTriggerGuideTests`; every row of *Test numbers*; a click track with a 2% noise floor still gives 8 hits; the made track saves and loads; a clip with no hits gives no track and an empty status ("No hits at this sensitivity. Try a lower value, such as −45 dB.").

## Website hand-off
- Features page: a section "Drums from a recording": right-click, the window, sort by drum, keep feel or snap, layer or replace, and the guide.
- Main page: not needed; the guide picker's MIDI area lists the new guide.
- Screenshots: the Hits to MIDI window on the example drum loop sorted by drum; the arrangement with the new track below the loop; the guide at *Kick, snare or hi-hat* and at *Replace or layer*.
- How it's made: nothing new (plain DSP, no library).

## Left for later
- The live Trigger device (Decision 1): a note output from an audio track to another track, replace/layer switch, time offset, band filters against bleed.
- Two drums at once (per-band finding), and toms, rides and crashes as their own notes.
- A timing offset control and the user's own samples (a sampler) for replacement.
- The drum view and notation (their own idea) to show the result by drum.
- Convert to MIDI's grid is relative to the clip's start, not the song's grid (seen while planning; not changed here).

## Changes while building

**Group A1 (engine: finding hits), built and tested (`./build.sh --test` passes, no warnings).**
- `analysis::hits` (`src/analysis/HitFinder.{h,cpp}`): `findCandidates (audio, rate, shouldExit, progress)`, `pick (candidates, PickSettings, edits)` -> `Hit {time, drum, velocity, levelDb, rise, drumFixed}`, `classify`, `getVelocity (curve, dbBelow)`, `getNoteForDrum`, `HandEdit {kind add/move/remove/setDrum, time, target (a Candidate), drum}` (matched within `matchSeconds` = 10 ms), and `measureCandidate (audio, rate, time, searchSeconds, candidates, exact)` for the window's add and drag (strongest candidate within the search range unless `exact`, which measures level and bands at the time given). Its own `analysis::hits::Drum` (kick, snare, hihat), not `drums::Drum`.
- `hitstomidi` (`src/engine/HitsToMidi.{h,cpp}`): `toClipNotes (hits, tempo, clipStartBeat, clipLengthBeats, Timing {snap, gridBeats, strength 0..1})`; `addDrumTrack (engine, source, audioClip, notes, muteOriginal)` (null and no track when no notes); `renderListen (audioClip, notes, tempo, clipStartBeat, rate, Hear::original/both/hits)` (stereo, as long as the clip, from the notes so snapping is heard); `getExampleLoop()` and `renderExampleLoop (rate)` (mono, 4 s) for the guide, tests and the `drumLoop` scene.
- Not yet written: the candidates are not cached anywhere; the window (group B1) holds them and calls `pick` on every change, and the status text "No hits at this sensitivity..." belongs to the UI.
- Tests: `runHitFinderTests`, `runHitsToMidiTests` in `tests/EngineTests.cpp`; the two files are in both CMake targets.

**Changes while building (A1).**
- Candidates use their own 256-sample (11.6 ms) FFT with lag 2 and hop 128, not `tempofinder::findOnsets` (that one normalises away the absolute rise, has no cancel flag and uses 46 ms windows, which merge hits 12 ms apart). It reuses `tempofinder::prepareInput`, the hop and the band edges.
- Time refinement works on the *first difference* of the source (change per sample), not the sample value: a kick still ringing at 60 Hz would otherwise hide a hi-hat on top of it (the earliest sample over 10% of the peak was the start of the search range). Reference = the largest change within 15 ms before to 30 ms after the flux peak.
- Two extra filters, both needed on the example loop: (1) a candidate's change in the 10 ms after must be at least 1.4 times that in the 8 ms before, which removes the second bump of a snare's ring (36 ms after it); (2) a second pass `addSharpAttacks` adds hits the spectral rise cannot see, a millisecond whose change is 3 times that of the 8 ms before and at least 6 ms from a candidate (the double hit 12 ms apart, and a kick bounce).
- The band energies are the spectrum of the 50 ms from the hit *minus* that of the 50 ms before (never below 0), so a hi-hat or snare over a ringing kick is not sorted as a kick. This is a fact the plan's *Theory* did not have; it only changes how the sort is done (the bands, 150 Hz and 6 kHz, are as planned).
- A hit on the very first sample is found (silence is assumed before the start).
- A candidate's level is the peak sample in the 30 ms after its time, as planned; under a ringing drum it reads up to about 1 dB high, so the example loop's velocities are 127 and 113 only to within a few: the test allows 3 for kicks and the ghost and 10 for the other snares and hi-hats. Level readings, not velocity maths, cause the spread (`getVelocity` is exact against the curves). The example loop's hits are each scaled so their first 30 ms peak at the stated level.
- Test signals are synthetic (noise clicks, tone bursts); the real-drum behaviour of the thresholds (`riseFloor` 8 dB, ratio 1.4, sharp ratio 3) is for the real-app check to confirm on recorded drums if any are at hand.

**Group A2 (engine: audition player and guide content), built and tested (`./build.sh --test` passes, no warnings).**
- Audition player in `Engine`: `startAudition (juce::AudioBuffer<float>, int startSample = 0)`, `stopAudition()`, `isAuditioning()`, `getAuditionPosition()` (samples; the buffer's length once ended). Mono or stereo, already at the device rate (`renderListen` at `engine.getSampleRate()`). Played by `renderAudition` right after `renderTone`. It ends by itself when the transport plays or a bounce runs (so the window must stop the transport before `startAudition`, and a Play press ends Listen: the window can poll `isAuditioning()` to flip its button). The buffer is swapped under `lock` and the old one freed outside it.
- `daw::triggerguide` (`src/engine/DrumTriggerGuide.{h,cpp}`, in both CMake targets): `Topic`/`allTopics`, `getTopicName`, `getTopicSummary`, `explain (Topic, const Part* = nullptr)`, `getTooltip (Tip)` (every control of the window), the numbers (`getBeatsFromMs`, `getMsFromBeats`, `getPeriodMs`, `getSumGainDb`, `getNotchHz`, `getCurveVelocity`, `getSnareTrack`, `countKept`, `getGridOffsetsMs`, `getSnappedOffsetMs`), and the *Your clip* rules: `Part {hits (PartHit beat from the clip start, drum, velocity), lengthBars, bpm, showDrums, sameForAll}`, `analyse (Part)` (lines), `describeCounts (Part)` (rule 1 alone: the window's status line, e.g. "18 hits in 2 bars (9 a bar): 5 kick, 5 snare, 8 hi-hat."), `getPartFromNotes (DrumNote list, lengthBeats, bpm)` for a selected drum clip (kick/snare/hi-hat notes only, open hats count as hi-hat).
- The window (B1) builds a `Part` from its current hits: `beat` = hit time through the tempo map from the clip start (after snapping if on), `sameForAll` when Velocity is Same for all.
- Texts use ASCII "-" for minus and "- " for bullets, as the other guides do, not the plan's "−" and "•".
- Rule 5 (same velocity) is only said with at least 4 hits (the plan gave no minimum; one hit would be "all the same"). Rule 2's hi-hat line needs at least 4 hi-hats. Rule 4's pair count is per drum, over neighbours in time.
- No example `Part` function: pictures use `hitstomidi::getExampleLoop()` (the UI maps it to hits with Natural velocities).
- Tests: `runAuditionTests`, `runDrumTriggerGuideTests` (called after `runHitsToMidiTests`).

**Group B1 (UI: the Hits to MIDI window), built; `./build.sh --test` passes, no warnings. Not yet seen in the real app (the `drumLoop` scene is step 11).**
- `HitsToMidiActions` (`src/ui/HitsToMidiActions.{h,cpp}`, member `hitsToMidi` of `ArrangementView`, getter `getHitsToMidi()`): `open (Track&, AudioClip&)` (progress window, then the window; a second call brings the window to the front), `openWith (Track&, AudioClip&, candidates)` (no job: for the screenshot scene, returns the window), `getWindowFor (clip)`, `getCurrentWindow()` (the most recent still open: what *Your clip* should read), `onShowGuide (triggerguide::Topic)`. `ArrangementView::onShowTriggerGuide` is the callback the owner (`MainComponent`) must set in group B2 to open the guide at a topic; until then the window's ? does nothing. The clip menu item is id 22 in `TrackLanes` (`onHitsToMidi`); the Convert to MIDI dialog has the new line.
- `HitsToMidiWindow` (`src/ui/HitsToMidiWindow.{h,cpp}`): `getDrumColour (Drum)` static (kick orange, snare blue, hi-hat green), `getPart()` (a `triggerguide::Part` from the notes after snapping, for *Your clip*), `getHits()`, `getStatusText()`, `makeTrack()` public (the scene option (c) calls it), `onShowGuide`, `onClose`, `onMade`. The window caches the clip's length, rate and start; it reads the clip only after `findTrackFor` says it is alive, and closes itself when it is not. The window is keyed in `PluginWindowManager` by the clip's address; closing is `onClose` -> `closeFor (key)` asynchronously.
- Differences from the plan: the "clicked spot" for Listen is a click on the **ruler** (a click on the waveform adds a hit); dragging previews the raw mouse position and the strongest rise within 10 ms is found on mouse-up (finding it needs `measureCandidate`'s whole-clip pass for a spot with no candidate nearby, too slow per mouse move); the **?** takes the topic of the control the mouse last entered (waveform, Sensitivity and gap: *Finding the hits*; Write as: *Kick, snare or hi-hat*; Velocity: *How hard*; Timing: *On the grid*; Original: *Replace or layer*; none yet: *What a drum trigger is*). Changes made while Listen plays restart it from the current position after about 130 ms. Make MIDI track is off when there are no hits; the status then reads "No hits at this sensitivity. Try a lower value, such as -45 dB."
- Settings are saved on every change under `hits.*` (`hear` default Both, `grid` default 1/16 as an index into 1/4, 1/8, 1/16, 1/32, 1/8 triplet, 1/16 triplet).

**Group B2 (UI: the guide window and the `drumLoop` scene), built; `./build.sh --test` passes, no warnings; `--export-guides` passes (the PDF "How drum triggers work.pdf" is made). Not yet seen in the real app (group C).**
- `DrumTriggerExplainer` (`src/ui/DrumTriggerExplainer.{h,cpp}`, `Guide::triggers` in `gen::guides`, area MIDI, last in `allGuides`): constructor `(Engine&, PartProvider)`, `showTopic`, static `getPartForClip (engine, track, midiClip)` (DawIt Drums plugin or the Drums/Percussion built-in sound). `MainComponent::showTriggerGuide (Topic)`, `getTriggerGuidePart()` (the open Hits window's `getPart()` when it has hits, else the selected drum MIDI clip), `triggerGuide` SafePointer, `openGuide` and `getPrintGuide` cases; `arrangement.onShowTriggerGuide` is set. The audio clip's right-click menu has *About drum triggers...* (id 32, `TrackLanes::onShowTriggerGuide`). `ScreenshotRunner::getGuideKey` is "triggers" (`"guide": "triggers", "topic": "Kick, snare or hi-hat"` etc.).
- Pictures are drawn from the example loop rendered at 44.1 kHz (spectra from the first kick, snare and hi-hat: the 2048 samples from the hit minus the 2048 before; shares computed from them). Clicks play: the loop (*What a drum trigger is*, *Finding the hits*), the snare hit (*Transients*), two 60 Hz kicks, left half in line and right half 8.33 ms apart (*Replace or layer*), through `engine.stop()` and `startAudition`. The picture follows `hits.sensitivityDb`, `hits.gapMs` and `hits.velocity` from the user settings, and *Your clip* is refreshed 10 times a second.
- Differences: *Finding the hits* shows the level curve, the Sensitivity line, the hits as dots and the shaded gap, but not the separate "rise curve" under it. Tooltips on the picture are one general sentence.
- Scene `drumLoop` (`DevScreenshots.cpp`): audio track "Drum loop" with `hitstomidi::renderExampleLoop (44100)` at beat 0, selected, then `getHitsToMidi().openWith` with `findCandidates` (sorted by drum is the default); `"makeTrack": true` calls `window->makeTrack()` (step 12c). Sensitivity etc. come from the shot's `settings` (`hits.sensitivityDb`). Added to the scene list in the runner's error text.
- Tests: `runTheoryGuidesTests` checks the entry (area, name, nine topics, not on the start path) and that "drum replacement", "transient", "drum trigger" and "layering" find the guide first and "bleed" finds it among its matches (the gate guide stays first for "bleed"). The window itself has no test.

**Group C (real-app check and docs), done; `./build.sh --test` passes, no warnings.**
- Seen in the real app (`--screenshots`, scale 1): the Hits window on the `drumLoop` scene (18 markers in three colours, "18 hits in 2 bars (9 a bar): 5 kick, 5 snare, 8 hi-hat."), with Sensitivity -18 dB (17 hits: the ghost snare drops out; the faint ticks show for rises below the line), after Make MIDI track (the "Drum loop hits" track with DawIt Drums in the device bar and the notes in the piano roll), and the guide at *Kick, snare or hi-hat* (shares: kick 100% below 150 Hz, snare 96% in the middle, hi-hat 98% above 6 kHz), *Replace or layer* (+6.02 dB against cancelling) and *Your clip* (the same 18 hits; ghost snare, grid 1/16 within 3 ms). No clipped text or empty pictures.
- Fixed: the `drumLoop` scene now sets the tempo to 120 BPM when it makes the track (the pop song's 96 BPM showed "1.6 bars").
- Shot-list notes: a guide shot must not use `keepWindows` with `"capture": "window"` (it opens no new window to capture); capture a guide by its window name (`"capture": "How drum triggers work"`). `"scene": "drumLoop"` with a `guide` shot opens both, for *Your clip* with the window open.
- Docs: README (a bullet after Convert to MIDI), docs/tests.md, docs/code-layout.md, docs/architecture.md, the idea's status block (Partly done: the live Trigger device, per-band finding, offset and own samples stay open), SOURCES.md already had the guide's section. No new library.

**Theory check.**
- *Transients*: the hit time is now described as the built code finds it (the first sample whose sample-to-sample change reaches a tenth of the biggest, so a hi-hat over a ringing kick gets its own start), not a tenth of the peak level.
- *Finding the hits*: no longer says the picture shows a rise curve (it doesn't); adds the second, millisecond-by-millisecond pass for hits a few ms apart; the picture sentence names the beater bounce the gap keeps out.
- *Kick, snare or hi-hat*: sorts by the energy a hit *adds* (the 50 ms after minus the 50 ms before), with a short paragraph on why; the picture sentence and the spectrum picture's heading (DrumTriggerExplainer.cpp) say the same.
- *How hard*: "MIDI only asks that it be applied smoothly" became "The MIDI standard leaves the curve to each instrument" (MIDI 1.0 only prefers an exponential curve, which is not the DLS one). The DLS curve 40 log10 (127/v) is confirmed by a second, independent source (OpenJDK's DLS-style concave transform, 20 log10 (127²/v²)); the DLS document itself is still unread.
- Listen tooltip: "from where you clicked on the ruler" (a click on the waveform adds a hit).
- SOURCES.md: the OpenJDK page and the searches for DLS and MIDI 1.0 velocity added under the guide.
