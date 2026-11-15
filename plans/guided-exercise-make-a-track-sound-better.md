# Guided exercise: make a track sound better

Status: Built, not committed
Idea: [#207 — Guided exercise: make a track sound better](https://github.com/geertvandamme/dawit/issues/207)
Part of: [#195](https://github.com/geertvandamme/dawit/issues/195). Same shared model as the released exercises (`src/generate/Exercises.{h,cpp}`, `ExerciseWindow`, `GuidePointer`; plans `guided-exercise-learn-the-chords-of-a-song.md`, `guided-exercise-write-lyrics-to-your-song.md`).

## Goal

A fourth guided exercise: pick one track, listen to it in the song, make room for it with DawIt EQ, even it out with DawIt Compressor, give it space with a shared reverb on a return, and compare each effect off and on at matched loudness. Each step ticks from the real song (the chain, the send, what the compressor did while playing, what was heard), names a starting point that suits the track and links to the EQ, compressor, reverb, routing and gain staging guides. A beginner learns the three everyday mixing tools and the habit that keeps them honest: switch it off and listen.

## Decisions for you

1. **Five steps, not four.** The issue lists four (EQ, compression, reverb, compare). Options: (a) add a first step, *Pick a track and listen*; (b) four steps, with the track simply the selected one. **Recommended:** (a): the steps need one named track, and "listen first, then reach for an effect" is the lesson the rest builds on (like the lyrics exercise's *Pick a section*).
   Answer: (a) five steps, with *Pick a track and listen* first. (user, 2026-11-15)
2. **When the EQ step ticks.** Options: (a) DawIt EQ on the track lowers some frequency from 40 Hz to 16 kHz by at least 1.5 dB (a cut: "make room"); (b) any change from flat. **Recommended:** (a): the step teaches cutting before boosting, and every starting point the exercise suggests has a cut that passes (tested). A boost-only EQ gets a status line, not a tick.
   Answer: (a) a cut of at least 1.5 dB between 40 Hz and 16 kHz. (user, 2026-11-15)
3. **When the compressor step ticks.** Options: (a) DawIt Compressor on the track, switched on, *heard* turning the track down by 2 dB or more while the song played; (b) its settings alone (threshold moved). **Recommended:** (a): it proves the compressor did something to this track's real level. It needs one small audio-thread addition (a reading of recent gain reduction that any number of views can take, see *Code*).
   Answer: (a) heard turning the track down by 2 dB or more while playing (the audio-thread reading). (user, 2026-11-15)
4. **Starting points.** Options: (a) the text names the starting point that suits the track (from its name, or a MIDI track's built-in sound) and the user picks it in the window; (b) the action button adds the effect with that starting point loaded. **Recommended:** (a): picking it is part of learning where the starting points are, and a step that ticks from one click teaches nothing. The *Add* buttons add DawIt EQ flat and DawIt Compressor at *Neutral start*.
   Answer: (a) the text names the starting point; the user picks it. (user, 2026-11-15)
5. **Which track.** Options: (a) the selected audio or MIDI track; when a bus, return, the master or nothing is selected (adding a return selects it), the last chosen track stays; (b) fixed at step 1 until *Start over*. **Recommended:** (a): it is what the device bar shows anyway, and every text names the track ("DawIt EQ on Bass"), so a switch is visible. Ticks follow the chosen track; the in-memory records restart when it changes.
   Answer: (a) the selected audio or MIDI track; the last one stays when a bus, return, the master or nothing is selected. (user, 2026-11-15)
6. **When "compare" ticks.** Options: (a) while the song plays, one of the exercise's effects (the track's DawIt EQ or Compressor, or DawIt Reverb on a return it sends to) has been heard at least 2 s switched off and 2 s switched on; (b) also require Match level on. **Recommended:** (a), with a tip when Match level is off: a learner who compares at all has learnt the habit; match level is taught, not enforced.
   Answer: (a) 2 s off and 2 s on while playing; Match level is a tip, not required. (user, 2026-11-15)
7. **Where it opens.** Options: Help > *Make a Track Sound Better...*; also the track header's right-click menu *Make this track sound better step by step...* (selects the track, opens the exercise); a Start panel tile. **Recommended:** Help and the track menu. An empty song has no track with sound, so no Start tile.
   Answer: Help > *Make a Track Sound Better...* and the track header menu; no Start panel tile. (user, 2026-11-15)

## What the user does and sees

**Opening.** Help > *Make a Track Sound Better...* after *Write Lyrics to Your Song...* (no shortcut; description "Pick a track, give it an EQ, a compressor and a shared reverb, then compare with them off, in five steps"); the track menu item (decision 7), just above the menu's guide item, on audio and MIDI tracks only. The shared `ExerciseWindow`, titled "Make a track sound better", about 780 high (check by shot: five rows).

**The chosen track**: the selected audio or MIDI track, else the one chosen earlier this session while it still exists (decision 5). **Exercise effects**: DawIt EQ and DawIt Compressor in the chosen track's chain, and DawIt Reverb on a return the track's send (on, above Off) goes to. Any one of each counts; a bypassed one doesn't.

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Pick a track and listen | a chosen track exists and the **listen record** has 4 s | *Play* | the chosen track's header (`ArrangementView::getTrackHeader`), else the first track with a clip |
| 2 EQ to make room | a switched-on DawIt EQ on the track lowers some frequency in 40 Hz to 16 kHz by ≥ 1.5 dB (bands that are on, summed) | *Add DawIt EQ* (adds it flat after the track's effects, opens it) or *Open DawIt EQ* | its chip in the device bar, else *+ Effect* |
| 3 Compression to even out | a switched-on DawIt Compressor on the track, and the **reduction record** ≥ 2 dB | *Add DawIt Compressor* (Neutral start, after the effects, opens it) or *Open DawIt Compressor* | its chip, else *+ Effect* |
| 4 Reverb on a send | a send of the track is on to an unmuted return with a switched-on DawIt Reverb | *Open the Sends menu* (`TrackActions::showSendsButtonMenu` at the header's Sends button) | the header's *Sends* button |
| 5 Compare with the effect off | the **compare record** (decision 6) | *Play* | the power dot of the first exercise effect's chip |

**In-memory records** (fed by `MainComponent`'s 5 Hz timer, like `feedWriteLyrics`; cleared in `afterReplace`, on *Start over* and when the chosen track changes):
- *Listen*: seconds heard while playing with a clip of the chosen track under the heard playhead and the track not silenced (`routing::getSilence`).
- *Reduction*: the most the chosen track's DawIt Compressor turned down while playing (cleared when that slot leaves the chain).
- *Compare*: per exercise effect (by slot, checked still in the song), seconds played on and off.

**Starting points named in the texts** (from `verb::kinds::getSource` on the track name; for a MIDI track on the built-in synth with an unknown name, from its sound):

| Source | EQ | Compressor |
|---|---|---|
| vocal | Vocal | Lead vocal (Backing vocals when the name has backing, bv, bvs or harmony) |
| kick / snare | Kick drum / Snare drum | Kick drum / Snare drum |
| drums (also synth drums, percussion) | Drum bus | Drum bus |
| bass (also synth bass) | Bass | Bass |
| guitar | Acoustic guitar if the name has "acoustic", else as *other* | Acoustic guitar / Electric guitar |
| keys (also synth piano, organ) | Piano, or *Make room for a vocal* when the song has a vocal track | Piano |
| ensemble, pad (also synth pad) | *Make room for a vocal* when the song has a vocal track | Pad or strings |
| other, unknown | *Make room for a vocal* when the song has a vocal track, else none (the low-cut text) | Neutral start |

**Missing prerequisites and tips** (status line; texts in *Theory*): no track with sound (fix *Import Audio...*); a bus or return selected with none remembered; the track muted or silenced by another's solo; nothing of the track under the playhead; no audio output (fix *Audio/MIDI Settings...*); an effect bypassed; EQ only boosts; EQ boosts ≥ 6 dB (tip); compressor heard but under 2 dB; compressor ≥ 10 dB (tip); the return muted, without DawIt Reverb, or its Mix under 100% (tip); a bass or kick track (tip); Match level off (tip); no exercise effect yet for step 5.

Nothing changes the song except the *Add* buttons (one plugin each, as *+ Effect* would) and the user's own clicks. **Remembered:** only `learn.soundBetter.done`. **First steps:** no change: the exercise needs a track with sound, and no command, menu or control the first steps name changes.

## Data and saving

No project data, no format change. In memory (`MainComponent`): the chosen track and the three records.

## The guide

No new guide or topics; no `gen::guides` or `TheoryGuideLinks.cpp` change. Step links:
- 1: EQ · *Common problems, in words*; EQ · *Where instruments sit*.
- 2: EQ · *Making room: masking*; EQ · *Cut narrow, boost wide*.
- 3: Compressor · *Turning down the loud parts*; Compressor · *How much, for what*.
- 4: Routing · *Sends and returns*; Reverb · *Reverb in a mix*.
- 5: Gain staging · *Louder sounds better*; Compressor · *Too much*.
- Carry-on: Gain staging · *Step by step*.

EQ, Compressor and Reverb are guides built on a plugin: their pages aren't in `gen::guides::getEntries()` but in `BuiltInGuides::getPageNames`. So `openExerciseLink` must resolve a plugin guide's page through `BuiltInGuides` (today it gets -1 and opens the guide where it was), and the engine links test checks only non-plugin links; plugin links are checked by the shots (an unknown `topic` is a shot error).

## Code

Recipes: *A guided exercise* (copy *Write Lyrics to Your Song*), *A command or menu item*, *Checking in the real app*.

- **`gen::soundbetter`** (new, `src/generate/SoundBetter.{h,cpp}`, both targets): texts, the chosen-track rule, the starting-point table, the EQ cut/boost analysis (from `eq::getResponseDb` on the bands, 1/24-octave points from 40 Hz to 16 kHz at 48 kHz), the send/return check, statuses and tips, done notes, and the three pure trackers (fed playing, heard beat, seconds since the last feed, and the facts they need). It reads DawIt EQ/Compressor/Reverb settings on the message thread through their getters.
- **`gen::exercises`**: register it (stem `learn.soundBetter`, links, carry-on, help text, height); `Context` gains the chosen track (checked with `Engine::indexOf` before use, never dereferenced otherwise), the three records and the output flag.
- **`BuiltInCompressor`**: a non-consuming reading of recent gain reduction (the block's most, falling back over time like `PluginSlot::outputLevel`), written by the audio thread as one atomic; `takePreview` and `readHistory` stay as they are. Risk: an audio-thread change in a processor that also ships as a VST3/AU: keep it allocation-free; existing compressor tests must pass unchanged.
- **UI wiring**: command, `showExercise`, track-menu item, context, timer feed, `runExerciseAction`/`showExerciseTarget` cases; getters for the header's Sends button and, on `DeviceBar`, a slot's chip and power-dot areas and the *+ Effect* button.
- **Invariants:** no tick from a click; the exercise never loads a starting point, moves a knob or sets a send itself; find slots fresh on each ask (never keep an index into a chain). Message thread only, under `Engine::getLock()` where the chain is read; no format change.

## Libraries and models

None. No `LICENSE`, `licenses/` or *License note* change.

## Theory

No new guide content: the exercise restates facts the EQ, compressor, reverb, routing and gain staging guides already give (sources in `SOURCES.md` under those guides). Two new facts were looked up (5, 14), added to `SOURCES.md` under Compressor and Gain staging.

### Facts

1. A loud sound hides a quieter one close to it in frequency (masking). Exact; EQ · *Making room: masking*.
2. Kick and bass both want 50-100 Hz; vocals and guitars meet around 2-4 kHz. Typical; same page.
3. Cut before you boost: a 3 dB cut at 300 Hz clears a voice much as a 3 dB boost at 3 kHz, without making it louder. Opinion; EQ · *Cut narrow, boost wide*.
4. Muddy is often 200-500 Hz, harsh 2-5 kHz. Typical; `eq::getProblems`.
5. Corrective cuts usually go before a compressor, tone-shaping EQ after; there is no fixed rule. Opinion; Sound On Sound (Houghton, read), Boris FX and Craig Anderton (search).
6. A compressor turns a sound down while it's loud and leaves it alone while quiet. Exact (definition); Compressor · *Turning down the loud parts*.
7. Typical reduction on the loudest parts: 3-6 dB on a vocal or bass, 1-2 dB on a mix; 10 dB and more is an effect. Typical; Compressor · *How much, for what*.
8. Each starting point's target reduction (Lead vocal 4 dB and so on). Exact (DawIt's `comp::Preset::targetReductionDb`).
9. Starting points are typical settings; every recording differs. Typical; EQ and Compressor sources.
10. A send adds a copy to a return; the track plays on as before; one shared reverb puts tracks in one room. Exact (DawIt) and typical; Routing · *Sends and returns*.
11. On a return, set the reverb to 100% wet so the send alone decides. Typical; Routing · *Insert or send?*.
12. A send switched on from a menu starts at -10 dB; muting a track silences its sends. Exact (`TrackActions`, Routing · *The inspector*).
13. Bass and kick get little or no reverb; turn reverb up till clearly heard, then back till you just stop noticing it. Typical / opinion; Reverb · *Reverb in a mix*.
14. The ear's short memory of a sound fades within a few seconds (about 3 to 4 s; estimates 1 to 10 s), so switch back and forth quickly. Typical; Wikipedia *Echoic memory* (read). Text says "within a few seconds".
15. The louder of two versions tends to sound better, even about 1 dB louder; compare at matched loudness. Typical; Gain staging · *Louder sounds better* (its sources).
16. Match level keeps DawIt EQ's and Compressor's output as loud as the input. Exact (DawIt).
17. Listen in the song, not only solo. Opinion; EQ · *Cut narrow, boost wide*.
18. DawIt's own thresholds (1.5 dB cut, 2 dB reduction, 2 s each way, 4 s listening, 6 dB boost tip, 10 dB compression tip) are DawIt's choices, not standards; the 6 dB matches the gain staging hint.

### Texts

Title "Make a track sound better". Help description as above. Intro: "Five steps to a track that sits better in your song: pick one track and listen, make room for it with EQ, even out its level with a compressor, give it space with a shared reverb, then compare it all with the effects off. Do each step in the main window: the tick appears by itself."

1. **Pick a track and listen** — "Choose one track and hear it in the song"
   - Do: "Click the name of a track with sound in it, such as a vocal, a bass or a guitar. Then press Play and listen to it with the rest of the song. What bothers you? It may be muddy, harsh or thin, loud in some places and lost in others, or dry, as if played in a cupboard. Each of the next steps works on one of these."
   - Why: "Effects are for problems you can hear, so listen before you reach for one. Listen in the song, not only on its own: a guitar that sounds thin alone may be just right with the bass. DawIt reads the track's name to suggest starting points, so a track called Vocal, Bass or Kick gets the settings that usually suit it."
   - Status, no track with sound: "Your song has no track with sound yet. Import a recording with File > Import Audio..., or make a first song with Help > First Song Exercise...." Fix: import audio.
   - Status, bus or return selected: "<Drum Bus> is a bus: it has no sound of its own. Click the name of an audio or MIDI track."
   - Status, muted: "<Bass> is muted, so you won't hear it. Click its M to unmute it." Soloed elsewhere: "Another track is soloed, so <Bass> is silent. Click S on the soloed track to turn solo off."
   - Status, nothing under the playhead while playing: "<Bass> has nothing to play here. Move the playhead to bar <9>, where its first clip starts."
   - Done: "<Bass> is your track."
   - Button "Play", tooltip: "Plays the song from the playhead. Press Space to stop." No output: fix *Audio/MIDI Settings...*, "DawIt has no audio output, so you won't hear anything. Choose an output in Audio/MIDI Settings...."
   - Show me: "Click a track's name to choose it."
2. **EQ to make room** — "Cut what's in the way"
   - Do, with a suggestion: "Press Add DawIt EQ: it goes on <Bass> and its window opens. Choose the starting point <Bass> at the top of the window, then play the song and listen. Change what doesn't suit your recording: drag a dot down to cut more, or up to cut less."
   - Do, without one: "Press Add DawIt EQ: it goes on <Lead> and its window opens. Turn on the low cut on the left and, while the song plays, drag it up until the track starts to sound thin, then back a little. If it sounds muddy, drag a bell down a few dB somewhere between 200 and 500 Hz; if harsh, between 2 and 5 kHz."
   - Why: "A loud sound hides a quieter one close to it in frequency. This is called masking, and it's why good tracks can sound muddy together: the kick and the bass both want 50 to 100 Hz, a voice and the guitars meet around 2 to 4 kHz. EQ gives each sound its own place. Most engineers cut before they boost: taking 3 dB of mud out at 300 Hz makes a voice clearer, much as adding 3 dB at 3 kHz would, but without making it louder or harsher. A low cut on a track that doesn't need the lows leaves room for the kick and the bass. Starting points are typical settings, not rules: every recording differs, so trust your ears."
   - Status, no track: "Pick a track first (step 1)."
   - Status, bypassed: "DawIt EQ on <Bass> is switched off. Click the dot on its chip in the device bar to switch it on."
   - Status, only boosts or nothing yet: "Your EQ doesn't cut anything yet. This step is about making room: turn something down, with the low cut or a dot dragged below the line."
   - Tip, big boost: "Tip: your EQ raises <3.2 kHz> by <7.5> dB. A big boost makes the track louder and pushes the next effect harder. Turn on Match level to hear whether it's really better."
   - Done: "DawIt EQ takes up to <4.0> dB off, most around <400 Hz>."
   - Button "Add DawIt EQ", tooltip: "Adds DawIt EQ to <Bass>, after its other effects, and opens it. It starts flat, so nothing changes until you move a band. Remove it with the x on its chip." With one: "Open DawIt EQ", "Opens the DawIt EQ on <Bass>."
   - Show me: "+ Effect adds an effect to <Bass>." With an EQ: "DawIt EQ on <Bass>: click to open it."
3. **Compression to even out** — "Turn down the loudest parts"
   - Do: "Press Add DawIt Compressor. Choose the starting point <Bass> at the top of its window, then play a part where <Bass> plays: the red line shows how much it turns down. Lower Threshold until the loudest parts come down by about <4> dB, or press Set it up step by step and it sets the threshold for you."
   - Why: "A compressor turns a sound down while it's loud and leaves it alone while it's quiet, like a hand on the fader that's never late. A singer is louder on some words than others, and a bass player hits some notes harder; evened out, the track sits in one place in the mix. Typical amounts for the loudest parts are 3 to 6 dB on a vocal or a bass and 1 to 2 dB on a whole mix; 10 dB or more is an effect. DawIt adds it after the EQ. Many engineers cut before the compressor, so it doesn't react to rumble or mud, and shape the tone after it, but there's no fixed rule."
   - Status, bypassed: "DawIt Compressor on <Bass> is switched off. Click the dot on its chip to switch it on."
   - Status, not enough yet: "DawIt Compressor hasn't turned <Bass> down by 2 dB yet (<0.6> dB so far). Play a part where <Bass> plays, and lower Threshold until the red line moves."
   - Tip, heavy: "Tip: it turned down up to <12.5> dB. That's a lot for evening out: the track may sound flat or pump. Raise Threshold, or lower Ratio."
   - Done: "DawIt Compressor turned the loudest parts down by up to <5.2> dB."
   - Button "Add DawIt Compressor", tooltip: "Adds DawIt Compressor to <Bass>, after its other effects, and opens it at Neutral start, which turns down what goes over -18 dB, so you may hear it at once. Remove it with the x on its chip." With one: "Open DawIt Compressor", "Opens the DawIt Compressor on <Bass>."
   - Show me: as step 2, with DawIt Compressor.
4. **Reverb on a send** — "Give it a space shared with the song"
   - Do: "Press Open the Sends menu and choose New return track > Reverb. A return track called Reverb appears, with DawIt Reverb on it, and <Bass> sends a copy of its sound to it at -10 dB. Play the song and set the amount by ear: drag the send in the device bar up or down, or turn the Reverb knob in the mixer (Cmd+2). If your song already has a reverb return, choose that one in the menu instead."
   - Why: "A reverb on a return is shared: every track that sends to it sounds as if it were played in the same room, and each track's send decides how much it gets. The track itself plays on as before; the send only adds a copy. That's why the reverb on a return is set to 100% wet. Use less than you think: turn the send up until you hear the reverb clearly, then back until you just stop noticing it."
   - Status, return muted: "The return <Reverb> is muted, so the reverb is silent. Click its M to unmute it."
   - Status, no reverb: "<Delay> has no DawIt Reverb on it. Send <Bass> to a reverb return too: in the Sends menu, choose New return track > Reverb."
   - Status, reverb bypassed: "DawIt Reverb on <Reverb> is switched off. Select the return and click the dot on its chip to switch it on."
   - Tip, bass or kick: "Tip: bass and kick usually get little or no reverb, because a tail in the low end turns to mud. Keep the send low, or try this step on another track."
   - Tip, Mix under 100%: "Tip: DawIt Reverb on <Reverb> has its Mix at <25>%, so the return also plays a dry copy of <Bass>. On a return, set Mix to 100%: then the send alone decides how much reverb a track gets."
   - Done: "<Bass> sends to <Reverb> at <-10.0> dB."
   - Button "Open the Sends menu", tooltip: "Opens the Sends menu of <Bass>, as its Sends button does. Nothing changes until you choose an item."
   - Show me: "Sends: send <Bass> to a reverb on a return track."
5. **Compare with the effect off** — "Is it really better?"
   - Do: "Turn on Match level in the EQ and compressor windows, so the track plays as loud with them as without. Then, while the song plays, click the dot on an effect's chip in the device bar to switch it off, listen, and click again to switch it on. For the reverb, select the return and do the same on DawIt Reverb. Go back and forth a few times, a few seconds each way."
   - Why: "Of two versions, the louder one tends to sound better, even when it's only about 1 dB louder, because the ear hears more bass and treble when it's loud. So a boost or a compressor's makeup can seem to help just because the track got louder. Match level evens out the loudness, so switching off and on compares only what the effect does. Switch quickly: the ear's detailed memory of a sound fades within a few seconds. If the track sounds better without an effect, leave it off: that's a good result too."
   - Status, no effect: "Add an EQ, a compressor or a reverb send first (steps 2 to 4), so there's something to switch off."
   - Status, not yet: "Play the song and switch an effect off and on while it plays: each way needs at least 2 seconds."
   - Tip, Match level off: "Tip: Match level is off in DawIt <EQ>, so switching it also changes how loud the track is. Turn it on in its window to compare fairly."
   - Done: "You compared DawIt <Compressor> off and on. It's <on> now."
   - Button "Play", as step 1.
   - Show me: "Click this dot to switch DawIt <EQ> off and on while the song plays."

Completion: "Your track has been through the main tools of mixing: EQ to make room, compression to even it out and a shared reverb for space, each checked with it off. Try it on another track, starting with the vocal or the bass, and send more tracks to the same reverb so they share one room." Carry-on "Next: the track's levels", tooltip "Opens Gain staging at Step by step: how loud a track should be going into each effect and out of its fader."

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| EQ flat (default bands) | not done | |
| Default low cut switched on (80 Hz, 12 dB/oct) | done; lowest point at 40 Hz about -12 dB | ±1 dB |
| One bell -1 dB, Q 1 | not done | |
| One bell -2 dB at 400 Hz, Q 1 | done, "2.0 dB ... 400 Hz" | ±0.1 dB, ±1/24 oct |
| One bell +3 dB only | not done, "doesn't cut" status | |
| Bell +7.5 dB | big-boost tip | ≥ 6 dB |
| Each suggested EQ preset (Vocal, Kick drum, Snare drum, Drum bus, Bass, Acoustic guitar, Piano, Make room for a vocal) | done | Drum bus lowest about -1.9 dB |
| EQ bypassed, or on another track | not done | |
| Compressor reading: steady sine peaking -6 dBFS, threshold -18, 4:1, knee 0, peak | `comp::getReductionDb` at -6 dB = 9 dB (derived: 12 dB over at 4:1 leaves 3), after 0.5 s | ±1 dB |
| Same, after 1 s of silence | < 0.5 dB | |
| Reduction record: 2.5 dB while playing / while stopped / 1.5 dB | done / not / not | |
| Reduction 12.5 dB | heavy tip | ≥ 10 dB |
| Listen: 20 feeds of 0.2 s with a clip under the playhead / 19 feeds / playhead outside clips / stopped / track muted | done / not / not / not / not | |
| Compare: 2.0 s off + 2.0 s on while playing / 1.8 s off / only on / stopped | done / not / not / not | |
| Send -10 dB to a return with DawIt Reverb | done, "-10.0 dB" | |
| Send Off (-60), return muted, reverb bypassed, return with only DawIt Delay | not done | |
| Return reverb Mix 25% | Mix tip | |
| Names: "Lead Vox", "BVs", "Bass DI", "Acoustic Gtr", "Electric guitar" (song with a vocal), "Kick In" | Vocal+Lead vocal; Backing vocals; Bass+Bass; Acoustic guitar both; Make room for a vocal+Electric guitar; Kick drum both | |
| MIDI "Synth 1" on the built-in pad | Pad or strings; EQ none without a vocal | |
| Every named preset exists in `eq::getPresets` / `comp::getPresets` | true | |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `BuiltInCompressor`: the non-consuming recent-reduction reading (*Code*). Tests: the sine numbers above; `takePreview` and history unchanged. Done when: existing compressor tests pass unchanged.
- [x] 2. `gen::soundbetter` (new files in both CMake targets): chosen track, starting-point table, EQ analysis, send check, the three trackers, every text of *Theory* with statuses, tips and done notes. Tests: the table above on made-up edits (tracks, plugins added with `Engine::addPlugin`, sends with `setSend`).
- [x] 3. Register `soundBetter` in `gen::exercises` (stem, links, carry-on, height, dispatch; `Context` fields). The links test skips plugin guides' pages (checked in step 7); stems stay distinct; every text filled with no `<` left. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 6):
- [x] 4. `MainComponent`: Help command, `showSoundBetterExercise`, the track-menu item (`TrackActions::showTrackMenu`, via a callback), context, the 5 Hz feed, resets in `afterReplace`, *Start over* and on a chosen-track change. Check: the window opens from both places.
- [x] 5. Actions and *Show me*: *Play*, *Add/Open DawIt EQ/Compressor* (`Engine::addPlugin`, `plugins::open`), *Open the Sends menu*, fixes; getters on `TrackHeader` (Sends) and `DeviceBar` (chip, power dot, *+ Effect*). `openExerciseLink` resolves plugin guides' pages through `BuiltInGuides::getPageNames`. Check: each pointer lands on its control.
- [x] 6. `ScreenshotRunner`: keys `exerciseListened`, `exerciseReduction` (dB) and `exerciseCompared` that fill the records for a shot. Done when: `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 7. Real-app check, on the demo song with `"select": "Bass"`: steps 1-5 with `exerciseShowMe` (pointer on header, *+ Effect*, chip, Sends button, power dot); step 2 after *Add DawIt EQ* (window open, text names *Bass*); steps 3 and 5 done via the record keys; step 4 after adding a reverb return with a send; one shot per plugin link (`"guide": "eq"`, `"topic": ...`, all seven plugin pages) so a wrong page name fails. Look for: no clipped text at 780 high, ticks match the song.
- [x] 8. Docs: README *Features* bullet (exercises); `docs/tests.md`, `docs/code-layout.md` (`SoundBetter`); `docs/recipes.md` *A guided exercise*: plugin guides' links resolve through `BuiltInGuides` and are checked by shots, not the engine test (the recipe now says every link is in `getEntries()`), and the third copy-from example; issue #207 progress and #195's checklist.

## Tests

All in `tests/EngineTests.cpp`, as the *Test numbers* table: the compressor reading; the EQ analysis; the listen, reduction and compare trackers; the send check; the starting-point table and that its presets exist; registration (five steps, two links each, carry-on, help description ending "in five steps", stem `learn.soundBetter`, `clampStep` 5 → 0); every text filled with no placeholder; no tick for a track that was deleted (chosen pointer no longer in the engine).

## Website hand-off

- `features.html`, the guided exercises group: one line on this exercise; a shot of the window at step 3 beside DawIt Compressor on the demo Bass.
- `learn.html`: list it with the other exercises, linking to the EQ, compressor and reverb guides.
- No Home, `for-you.html` or `about.html` change (no new library).

## Left for later

- Buses as the chosen track (a drum bus): needs a listen rule for tracks without clips.
- Third-party EQs, compressors and reverbs: the ticks read only DawIt's own.
- Delay on a send, saturation, and panning: the mix-and-export exercise (#208) covers balance and pan.
- A pointer from the EQ and compressor guides to this exercise.

## Changes while building

**Group A hand-off (engine, built and tested).**
- `BuiltInCompressor::getRecentReductionDb()` (atomic, non-consuming; the block's most, falling 10 dB/s, `recentReductionFallDbPerSecond`). Read it from the 5 Hz timer and pass it to `ReductionTracker::feed`.
- `src/generate/SoundBetter.{h,cpp}` (`gen::soundbetter`, both CMake targets). `Progress { selected, remembered, playing, heardBeat, hasOutputDevice, listened, reduction, compared }`; `ex::Context` carries the same as `soundBetterSelected`, `soundBetterRemembered`, `playing`, `heardBeat`, `listened`, `reduction`, `compared` (plus the shared `hasOutputDevice`). `MainComponent` sets `soundBetterSelected` to the selected track (any kind, so a bus can be named) and `soundBetterRemembered` to the last chosen one; `findChosenTrack (engine, selected, remembered)` gives the chosen track (null when neither is an audio/MIDI track still in the song), so after each ask `MainComponent` stores it as the remembered one and resets the three trackers when it changes.
- Trackers (feed on the 5 Hz timer, `reset()` on `afterReplace`, *Start over* and a chosen-track change): `ListenTracker::feed (playing, soundbetter::isHeardAt (engine, track, heardBeat), dt)`; `ReductionTracker::feed (playing, findEffect (track, Effect::compressor), recentDb)` (pass the compressor slot, null clears it); `CompareTracker::feed (playing, seen, dt)` with `seen` one `{ slot, ! slot->bypassed }` for each of `findExerciseEffects (track)`. Each has `getRecord()` for the context. A gap over 0.5 s counts as 0.5 s.
- `findEffect (track, Effect, preferOn)` gives the slot for the *Add/Open* buttons and *Show me* (`getActionLabel (step, hasEffect)` and `getShowMeText (step, engine, progress)` already say which); `getSends (track)` and `findExerciseEffects (track)` give the Sends button and power-dot targets (the first exercise effect is the step 5 dot).
- Registered as `Exercise::soundBetter` (stem `learn.soundBetter`, window height 780 to be checked by shot, carry-on *Next: the track's levels*). `getAction` returns Fix `importAudio` or `audioSettings` where the status names them; steps 1 and 5 own action is *Play*, step 2 and 3 *Add/Open DawIt EQ/Compressor*, step 4 *Open the Sends menu* (all `Fix::none`, dispatched on the step).
- Statuses carry the tips too, and are shown whether the step is ticked or not. `getDoneNote` is empty until ticked.

**Changes while building (Group A).**
- Decision 7 (A): the text needs a status for "nothing selected": *Click the name of a track with sound in it.* (my text), and for a track in a muted bus: *<Bass> plays into a muted bus, so you won't hear it. Click the bus's M to unmute it.* and for a clip-less track: *<Bass> has no clip to play. Click a track that has sound.* (my texts, for the UI cases the plan's texts don't cover).
- Step 3 Do text on *Neutral start* says *It starts at Neutral start.* instead of *Choose the starting point Neutral start at the top of its window*; with an effect already on the track, *Press Open DawIt EQ/Compressor to open the one on <Bass>.* replaces the *Add* sentence.
- The EQ minimum is found on the 1/24-octave grid and then refined in 1/192-octave steps, so a bell at 400 Hz reads 400 Hz.
- The mixing exercise's placeholder test checks for `<` only: the texts contain `>` in menu paths (File > Import Audio..., New return track > Reverb).
- The engine links test now skips guides built on a plugin (EQ, Compressor, Reverb): their pages are `BuiltInGuides::getPageNames`, checked by the shots.

**Group B hand-off (UI, built; app compiles with no warnings).**
- `MainComponent`: `soundBetterExerciseCommand` (Help, after Write Lyrics), `showSoundBetterExercise()`, `feedSoundBetter()` on the 5 Hz timer (sets `soundBetterRemembered`, resets the trackers when the chosen track changes), `resetSoundBetterMemory()` (called in `afterReplace`, Start over), `runSoundBetterAction`, `showSoundBetterTarget`, `setSoundBetterRecordsForShot`. Track menu item: `TrackActions::onMakeSoundBetter` (item 21, audio and MIDI tracks, single target, above the singing/guide items), which selects the track and opens the window.
- *Add* uses `plugins::add` (appends and opens, so a compressor starts at its default settings, which I took to be Neutral start: Group C should confirm by shot that it reads Neutral start). *Show me*: step 1 the chosen track's header (else the first clip track), steps 2-3 the effect's chip (`DeviceBar::getEffectChip`) else *+ Effect* (`getAddEffectButton`), step 4 `ArrangementView::getSendsButton`, step 5 the power dot (`getEffectPowerArea`; for a reverb on a return it selects that return first). Showing steps 2-5 selects the chosen track so the device bar shows its chain.
- `openExerciseLink` resolves a plugin guide's page through `BuiltInGuides::getPageNames (entry.plugin)`; not yet checked by shot for the seven plugin pages (Group C, step 7).
- Shot keys: `"exercise": "soundBetter"`, `exerciseListened` (seconds), `exerciseReduction` (dB), `exerciseCompared` (true); they act on the selected track, so put `"select": "Bass"` first. One check: step 2 *Show me* on Bass shows the pointer on *+ Effect* with the text naming Bass.
- Tests: `./build.sh --test` compiles everything without warnings; the test run currently shows 5 to 7 failures, all `mix and export:` checks from the other, unfinished plan (pan tip), none from this feature. They changed between runs, so someone is editing that work in parallel.

**Group C (real-app check and docs).**
- Shots (scale 1, `"song": "sends"`, `"select": "Bass"`; list at /private/tmp/claude-501/sb/shots.json): window at steps 1, 2, 3 (after Add, then the record keys), 4 (after `"sendsChoice": "Bass:new reverb"`) and 5; main window with *Show me* for steps 2 (*+ Effect*) and 5 (the EQ's power dot, pointer on it); the compressor's window after *Add DawIt Compressor*; and eight guide shots for every plugin link (`eq` x4, `compressor` x3, `reverb`), none errored, so every page name resolves.
- A compressor added from the exercise opens with -18 dB, 3:1, 6 dB knee, 10 ms, 150 ms, 0 dB, 100%: exactly the Neutral start values (the starting-point box still shows its placeholder, not a name). The text keeps saying "starts at Neutral start".
- 780 high holds every step with status, tip and two link chips: nothing clipped (the longest, step 5 with the Match level tip, ends 130 px above the buttons). Ticks matched the song in each shot.
- First steps: checked, no change.
- Docs: README (exercises bullet, now "All five exercises"), `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md` (copy-from example, plugin-guide links checked by shot, shot keys), `docs/ui-design.md` (height).
