# Bounce to WAV (audio export): selected tracks, bounce in place, stems and dragging audio out

Status: In progress
Idea: [Bounce to WAV (audio export)](../FEATURE_IDEAS.html#bounce-to-wav-audio-export)

## Goal
The open parts of Bounce to WAV: bounce only the selected tracks; *Bounce in place*, which prints a track or a clip through its instrument and effects onto a new audio track and mutes the original; one WAV per track (multitracks) or per bus (stems) in a folder, to mix or master elsewhere; and an audio clip saved or dragged out as a WAV. A beginner learns what each file holds (what you hear when you solo it), why files must line up, and when parts add up to the mix.

## Decisions for you
1. **Build it in two goes.** Options: one build; or Go 1 (the engine's bounce focus, *Selected tracks*, *Bounce in place*, audio clips out as WAV, two guide topics: steps 1 to 8) then Go 2 (multitracks and stems, the third topic and the "your song" rules: steps 9 to 14). **Recommended:** two goes, Go 1 first: it is what a beginner uses inside DawIt (reverse a synth, audio to MIDI, save CPU), and it builds the engine part Go 2 needs. Go 2 is mostly UI and file handling on top.
   Answer: Two goes, as recommended. Build Go 1 now (Groups A and B, steps 1 to 8, then the Go 1 part of 13 and 14); Go 2 (Groups C and D) later.
2. **What a multitrack or stem file holds.** Options: (a) what you hear when you solo it: its effects, bus (multitracks), its share of the reverb returns, fader, pan and master volume, plus a *Dry* choice for multitracks; (b) Ableton's default: each track's own processing only, returns as files of their own. **Recommended:** (a): the files at 0 dB play back the mix, and it reuses the solo rule the routing guide teaches. Returns as their own files go under *Left for later*.
   Answer: (a), as recommended.
3. **Bounce in place prints the effects.** Options: instrument and effects printed (Logic's default); or instrument only, with the effects copied live onto the new track. **Recommended:** printed, the new track with no effects; *instrument only* later.
   Answer: Printed, as recommended.
4. **Where the text goes.** Options: three topics in the *Buses, sends and routing* guide; or a new *Bouncing and exporting* guide. **Recommended:** the routing guide: each file is defined by routing (solo path, buses, returns), and its picture already draws that.
   Answer: The routing guide, as recommended.
5. **Shortcut for Bounce in place:** Cmd+Shift+B. Logic uses Ctrl+B, which is Cmd+B (Bounce to WAV) on Windows and Linux. **Recommended:** Cmd+Shift+B.
   Answer: Cmd+Shift+B, as recommended.
6. **An audio clip dragged out** is its own audio with its clip gain, not through the track's effects (bounce it in place first for that). **Recommended:** as stated.
   Answer: As stated.

## What the user does and sees
**Bounce to WAV window** (Cmd+B) gets a first row, *What*, above *Range*:
- *The mix* (as today);
- *Selected tracks: Kick, Snare* ("4 tracks" past three names), disabled with "(select them in the track headers)" when none; the outermost selected tracks (`routing::getOutermost`), so a bus means everything in it;
- Go 2: *Every track, one file each (multitracks, 13 files)* and *Every bus and lone track, one file each (stems, 5 files)*, with a *Sound* row for multitracks: *As you hear it* / *Dry: instrument and clips only*.

The hint under the rows changes with *What* (texts in *Theory*, tooltips too). One file: the save dialog as today, named "<Project> (Kick, Snare).wav". Several files: the save dialog names a new folder ("<Project> stems", "<Project> multitracks") that is created; files are "01 Kick.wav", numbered in edit order (three digits past 99). The report lists the files that peak over 0 dB with by how much, and those left out and why ("Pad: muted", "Strings: no clips"). Cancel deletes the files this export wrote. Remembered: `export.bounceWhat` (mix, tracks, stems; never *selected*), `export.bounceDry`.

**Track right-click menu** (audio and MIDI tracks, before the guide item): *Bounce to WAV...* (opens the window with *Selected tracks* picked) and *Bounce in place*, disabled when the track has no unmuted clip. **Clip right-click menu** (audio and MIDI): *Bounce clip in place*; audio clips also *Export clip as WAV file...*. **Track menu:** *Bounce in Place* (Cmd+Shift+B) on the selected clip, or the selected track when no clip is selected.

**Bounce in place** runs with a progress window and Cancel, like Bounce. Afterwards:
- a new audio track right below the original, in the same bus, named "Lead (bounced)", same colour, with the original's volume, pan and sends (level and pre/post), no plugins; the new track is selected;
- track: one clip from the first unmuted clip's start to the last one's end plus a tail of up to 4 s, trimmed where it falls below -80 dB; the original track is muted;
- clip: that clip alone (other clips on the track are not heard), from its start, plus the tail; the clip is muted. It goes on the track right below when that is an audio track named "<name> (bounced)" with no clip in the way, else a new track;
- the original's instrument, notes and effects never change;
- a result that peaks over 0 dBFS is scaled to peak at -1 dBFS and its clip gain raised by the same dB (at most +12 dB, saying so when that limit cuts it), because projects save clips as 24-bit WAV.

**Dragging an audio clip out** of the window gives a WAV, exactly like MIDI clips today (same gesture, temporary folder cleaned after an hour). The clip's audio times its clip gain at its own rate and channels, 24-bit, or 32-bit float when that peaks over 0 dB. *Export clip as WAV file...* writes the same through a save dialog in `export.audioFolder`. The audio clip tooltip adds "drag it out of the window to save it as a WAV file".

## Data and saving
None new. Bounce in place makes ordinary tracks and in-memory audio clips (`Engine::addAudioClip` with a buffer), which saving writes to the project's audio folder as for split audio. No format version change. Tests check a bounced-in-place project saves and loads with the new track's volume, pan, sends and clip gain.

## The guide
Three topics in `routingguide`, after *Signal flow*, before *Your song's routing* (Go 1: the first two; Go 2: the third). Texts in *Theory*.
1. **Bouncing to audio** — "Turn the mix, or a few tracks, into a WAV file". Picture: `makePopExample` with Kick and Snare as the bounced tracks: their path (Drums bus, master) lit, the rest dimmed (`onSelectedPath`), and a file tag "Kick + Snare.wav" by the master.
2. **Bounce in place** — "Print a track's instrument and effects to audio, and keep the original". Picture: a small example: Lead (muted, dimmed) and "Lead (bounced)" below it, both in a Synths bus, both sending to Reverb.
3. **Stems and multitracks** — "One file per bus or per track, to mix or master somewhere else". Picture: `makePopExample` with a numbered file tag on each of the 5 stems (Drums, Bass, Guitars, Keys, Vocals); a node gets an optional file tag drawn at its right.

**Your song's routing** (Go 2): `explainSong` adds the stem and multitrack counts; `analyse` adds the advice in *Theory*, last in the list, with no action. Theory Guides entry: add search words "bounce export render print freeze stems multitracks wav"; topics follow. Not on *Start here*. The Bounce window gets no guide button (it's a dialog); the track menu's last item already opens this guide.

## Code
- Recipes: *Topics in an existing guide*, *A command or menu item*, *A long job* (copy `BounceJob`), *Checking in the real app*.
- **Bounce focus** (new, in `bounce::Settings`, applied by `bounce::Renderer` under `Engine::getLock()` and cleared in its destructor; only honoured while offline): *mix* (today); *heard as soloed* (a set of tracks stands in for the solo buttons); *one chain* (one track's clips through its instrument and, unless *instrument only*, its effects; optionally one clip only).
- **Solo set:** every read of the solo buttons on the audio path (`renderSegment`'s solo pass and the send check against `send.target->soloed` in `Track::process`) goes through one per-block "counts as soloed" flag, which is the focus set during such a bounce. Mute still wins. The user's solo atomics are never written. Normal playback must stay bit-identical (the existing solo tests).
- **One chain:** only that track renders, at unity gain, no pan, no sends, its mute ignored, straight to the output, with no master volume; buses and returns don't render. The clip filter lets only that clip's audio or notes play. For a MIDI track the plugins' latency (as `Track::getCompensation` sums it, over what is rendered) is rendered extra and dropped from the start; audio tracks are compensated already.
- **Renderer** can render into memory as well as to a file, and can keep the tail whole (multi-file exports: every file the same length).
- **Engine-side, tested:** making the bounced track (placement, copied volume, pan, sends, clip gain scaling, muting) is a function in `daw::bounce` the UI calls on the message thread once rendering is done; so is the plan of a multi-file export (which tracks, names, what's skipped and why, the about text) and writing a clip to WAV.
- **Threading:** the focus is plain data written on the message thread under the lock before `beginOffline` and read only by `renderOffline` (which holds the lock); the device callback plays silence while offline.
- **Risks:** the audio-thread change to solo (do it first, alone, with tests); MIDI-track latency; tracks deleted while a modal job runs (check the track still exists, as `StemActions` does, before adding the result).

## Libraries and models
None.

## Theory

### Facts
1. A **stem** is a group of related tracks mixed and printed together as one file; **multitracks** are the individual, unprocessed tracks a mix is made from. iZotope, *Stems and multitracks* (izotope.com/en/learn/stems-and-multitracks); Boris FX blog. Definition, exact. Agrees with the routing guide's *What a bus is for* ("A stem is a bus saved as its own audio file").
2. Every exported file should start at the same point (bar 1) so they line up when imported. Sonarworks, *How to prepare your session for a mixer*; Dan Nielsen's stems checklist; Apple's Logic support note on adding a region at bar 1. Typical (universal practice).
3. Stems set to 0 dB should recreate the rough mix. Sonarworks. Typical.
4. Non-linear processing (compression, saturation, limiting) on a shared bus stops parts summing back to the mix. iZotope. Exact (superposition: a linear process f has f(a)+f(b)=f(a+b); a compressor's gain depends on the input level).
5. Faders, pan, EQ, reverb and delay are linear, so parts through them add up to the whole. Derived (superposition); DawIt Reverb and Delay have no random modulation (checked in code). Exact for DawIt's built-ins; other plugins may differ.
6. Whether a stem includes its reverb, and whether mixers want plugins printed or dry, varies. iZotope (wet or dry, artist's choice); Sonarworks ("many mixers prefer plugins running live"); Ableton's *Include return and master effects* option (help.ableton.com, Importing and exporting stems). Opinion.
7. Tempo should go with the files (in the name or as a tempo map MIDI file). Sonarworks (tempo in the name); Dan Nielsen's checklist (tempo map as MIDI). Typical.
8. Send at least 24-bit. Dan Nielsen's checklist and mixbutton guide; agrees with *What is sound?* ("44.1 kHz and 24-bit is a good choice"). Typical.
9. Bounce in place renders the track's instrument and effect plug-ins, saving CPU; volume and pan are copied, not performed, by default; the new audio goes on a new track below. Apple, Logic Pro user guide: *Bounce a track in place*, *Bouncing Regions and Tracks in Place* (Logic 9 manual). Manufacturer.
10. Freeze keeps the original so it can be undone; Logic's bounce in place replaces it. Apple, Logic Pro for iPad guide. Manufacturer.
11. 16- and 24-bit WAV can't hold peaks over 0 dBFS; 32-bit float can. As in *What is sound?* and *The mix bus*. Exact.
12. -80 dB = 10^(-80/20) = 0.0001 (the tail cut). Derived, exact.
13. -1 dBFS = 10^(-1/20) = 0.8913. A peak of 2.0 scaled to 0.8913 needs 20 log10 (2 / 0.8913) = +7.02 dB of clip gain. Derived, exact.
14. The pop example: 13 tracks (Kick, Snare, Hats, Overheads, Bass, Guitar L, Guitar R, Piano, Pad, Lead vocal, BV 1 to 3), 5 stems (Drums, Bass, Guitars, Keys, Vocals; Backing vocals is inside Vocals). Derived, exact.
15. "Bounce" comes from tape, where tracks were mixed down ("bounced") onto a free track. Common knowledge in recording texts; stated without a date.

### Topic texts

**Bouncing to audio** — *Turn the mix, or a few tracks, into a WAV file*

Bouncing turns the sound DawIt makes while it plays into a WAV file: a recording of it. The instruments, effects and faders do their work once, and the result is saved. The word comes from tape, where engineers mixed several tracks down onto a free one: they bounced them.

What goes in depends on what you bounce:
- The mix (File > Bounce to WAV..., Cmd+B): every track through its bus and its sends, then the master volume. This is the file you play to other people.
- Selected tracks: select them in the track headers (Shift-click, Cmd-click), then pick Selected tracks in the same window. You get what you hear when you solo them: each one through its buses and the reverb it sends to, then the master volume. Your solo buttons aren't touched.
- One track, as audio in the song: see Bounce in place.

The picture shows a bounce of Kick and Snare. The lit boxes are in the file. Kick and Snare still go through the Drums bus, so its compressor and fader shape the file, but Hats and Overheads are left out.

A bounce runs offline: DawIt renders as fast as your computer can, usually much faster than the song plays, and plugins are told they may use their best quality. It doesn't change your song. Muted tracks stay silent, and the metronome is left out.

Every bounce starts exactly at the start of its range. Two files bounced from the same range line up sample for sample when you place them side by side in another program.

The format choices, 44.1 or 48 kHz and 16, 24 or 32-bit float, are in What is sound?, under From air to numbers. One rule matters here: a 16- or 24-bit file can't hold a peak over 0 dB. The loudest moments are cut off (clipped), which sounds harsh. A 32-bit float file keeps them, so they can still be turned down later.

**Bounce in place** — *Print a track's instrument and effects to audio, and keep the original*

Bounce in place turns a track, or one clip, into audio on a new track right below it, and mutes the original. Right-click the track's header or a clip and choose Bounce in place, or press Cmd+Shift+B.

Why you'd want it:
- To work with the sound as audio: reverse it, split it into stems, convert it to MIDI for another instrument, or cut and move it like a recording.
- To keep a sound you like, such as a synth patch and its effects, safe from a change by accident later.
- To save computer power. Instruments and effects work all the time the song plays; an audio clip only has to be read. Six tracks with heavy synths and reverbs can make playback stutter; bounced, they play without effort.

What goes in: the track's clips through its instrument and its effects, up to its fader. What stays live: the fader, the pan, the sends and the bus. The new track gets the original's fader level, pan, sends and bus, and no effects (they are in the audio now), so the song sounds the same. Logic Pro's Bounce in Place works the same way by default: the volume and pan are copied, not printed.

The original stays in the song, muted, with its instrument, notes and effects as they were. To go back, unmute it and delete the bounced track. Other programs call this kind of reversible bounce freezing.

Things to know:
- The audio starts where the first clip starts and runs on for up to 4 seconds after the last one ends, while a reverb or a released note dies away. It is cut where the sound has fallen below -80 dB.
- A bounce takes the sound as it is now. Later changes to the notes or the effects don't reach the audio; bounce again after changing them.
- Bouncing one clip plays only that clip: a note still ringing from the clip before it is left out.
- A bounce that peaks over 0 dB is turned down to peak at -1 dB, and its clip gain is raised by the same amount: a peak of 2.0 (+6 dB) gets +7.0 dB of clip gain. It sounds the same and saves without clipping.
- Effects that hold the sound back a little, such as DawIt Tune, are allowed for, so the audio lands in time.

**Stems and multitracks** — *One file per bus or per track, to mix or master somewhere else*

Sometimes a song goes on to someone else: a friend who mixes in another program, a mastering engineer, a remixer. Then it leaves DawIt as several files instead of one. Bounce to WAV makes them, all into one new folder.

Multitracks are one file per track: Kick, Snare, Bass, Lead vocal, each on its own. A mixing engineer works from these. The song in the picture has 13 tracks, so it makes 13 files.

Stems are one file per group: everything in the Drums bus as one file, then the bass, the guitars, the keys and the vocals. In DawIt a stem is each bus at the top level, and each track that isn't in a bus. The song in the picture makes 5 stems; Backing vocals is inside Vocals, so it is part of the Vocals stem. Stems are for mastering in parts, a remix, backing tracks for a live show, or a version without the vocals.

Each file is what you hear when you solo that track or bus: its instrument and effects, the bus it plays into, its share of the reverb on the returns, its fader and pan, and the master volume. Put all the files in another program with their faders at 0 dB, and they play back your mix.

Dry multitracks are the other choice: only the clips and the instrument, no effects, the fader at 0 dB, the pan in the middle and no reverb. They are for an engineer who wants to make those choices again. Engineers differ: many like to hear your effects, some want everything dry, and some want both. Ask before you send.

Three things make the files work in another program:
- They all start at the same moment, the start of the range (usually bar 1), and they are all the same length, even a track that only plays in the chorus. A file that starts late is easy to put in the wrong place.
- The tempo goes with them. A WAV file has no tempo, so DawIt writes the tempo, key and sample rate into About these files.txt, and the tempo and its changes into Tempo map.mid, which most programs can import to set their grid.
- Every file has the same sample rate and bit depth. Send 24-bit or 32-bit float; most engineers ask for at least 24-bit.

When do the files add up to exactly the mix? A fader, a pan, an EQ, a reverb or a delay treats each part the same whether it plays alone or with the others, so the parts add up to the whole. A compressor, gate, saturation or distortion on a bus doesn't. It reacts to how loud everything in it is together. Kick alone pushes the drum bus compressor less than the whole kit does, so the drum multitracks add up to drums a little louder and less squashed than in your mix. The Drums stem keeps the whole bus in one file, so it is exactly as you hear it. A compressor on a return works the same way: it hears all the sends together.

What it leaves out: muted tracks, tracks with no clips, and the notes themselves (Export track as MIDI file... saves those).

### Tooltips and window texts
- *What*: "What to bounce: the mix, the tracks selected in the headers, or one file per track or per bus."
- *Sound* (multitracks): "As you hear it: with its effects, bus, reverb and fader, so the files play back your mix. Dry: only the clips and the instrument at 0 dB, for an engineer who mixes it again."
- Hint, selected: "What you hear when you solo Kick and Snare: through their buses and the reverb they send to, with the master volume. Your solo buttons aren't changed."
- Hint, multitracks: "One WAV per track, all starting at the start of the range and the same length, in a new folder with the tempo and key. Muted tracks and tracks with no clips are left out."
- Hint, stems: "One WAV per bus at the top and per track outside a bus, each as you hear it when soloed. Together they play back the mix."
- *Bounce in Place* command: "Render the selected clip, or the selected track, through its instrument and effects onto a new audio track below, and mute the original."
- *Bounce to WAV...* command: "Render the mix, the selected tracks, or every track or bus, to WAV files."
- About these files.txt (example): "Song: Pop song. Tempo: 96 BPM (changes in Tempo map.mid). Key: A minor. 48 kHz, 24-bit WAV. Every file starts at bar 1 and lasts 3:12.4. Each file is the track as heard when soloed, with its effects, bus, reverb and fader." (Dry: "...only its clips and instrument, no effects, fader at 0 dB.")

### Example data
- Bouncing to audio: `makePopExample`, Kick and Snare lit with Drums and the master; file tag "Kick + Snare.wav".
- Bounce in place: Synths bus; Lead (muted, -8 dB), Lead (bounced) (-8 dB), both sending -12 dB to Reverb.
- Stems: `makePopExample`, tags 1 Drums, 2 Bass, 3 Guitars, 4 Keys, 5 Vocals.

### "Your song" rules (Go 2)
1. Always when there is at least one track with an unmuted clip: `explainSong` adds "Bounced as stems, your song makes 3 files: Drums, Bass and Vocals. As multitracks, 7 files." Quiet for an empty song.
2. A bus with at least two tracks playing into it (directly or through buses) and an unbypassed DawIt Compressor, Gate, Saturation, Distortion or Tune: "Drums has DawIt Compressor, which reacts to the whole group. Multitracks of Kick, Snare and Hats won't add up exactly to your mix. Send the Drums stem as well, or send them dry and say which compressor you used." Once per bus, last in the list.
3. A return with sends from at least two tracks and one of those effects: "Reverb has DawIt Compressor, which hears all the sends together, so the reverb in each file is a little different from your mix's. Send the files as you hear them and check the sum, or send the returns' effects as notes." Quiet otherwise.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Tail cut level | 0.0001 (-80 dB) | exact |
| Peak 2.0 bounced in place: stored peak | 0.8913 | ±0.001 |
| ... clip gain | +7.02 dB | ±0.05 dB |
| Selected {A} in a bus with fader +6 dB | peak = A's × 1.995 | ±0.5 % |
| Sum of multitracks (or stems) vs the mix | difference | ≤ 0.0001 per sample |
| Pop-shaped song: stems / multitracks | 5 / 13 | exact |
| Dry file of a mono clip vs clip audio × clip gain | equal, both channels | ≤ 1e-6 |
| Clip exported as 24-bit WAV vs clip audio × gain | equal | ≤ 1e-6 |
| MIDI track with bypassed DawIt Tune, bounced in place, vs without Tune | equal | ≤ 1e-5 per sample |

## Steps
**Group A (engine, Go 1); `./build.sh --test` at step 3.**
- [x] 1. The bounce focus in `Bounce.h` / `Engine` / `Track`: heard-as-soloed sets, one chain (clip filter, instrument only), rendering into memory, whole tail. Tests: solo-set and chain checks below; the existing solo tests unchanged. Done when the full suite passes.
- [x] 2. In `daw::bounce`: bounce in place (track and clip: render, MIDI latency drop, peak scaling, the new track's placement, copies and muting, reuse of the "(bounced)" track); writing a clip to WAV. Tests: below, and a save-and-load round trip.
- [x] 3. Routing guide topics 1 and 2 (`RoutingGuide.{h,cpp}`: names, summaries, texts, examples, search words in `TheoryGuides.cpp`). Tests: topic count, texts, search.

**Group B (UI, Go 1); `./build.sh --test` at step 8.**
- [x] 4. Bounce window: *What* with *The mix* and *Selected tracks* (`BounceDialog`, MainComponent passes the selection); track menu *Bounce to WAV...* in `TrackActions`. Check: Kick and Snare selected, bounce, the file holds only them.
- [x] 5. *Bounce in place*: Track menu command (Cmd+Shift+B), track and clip right-click items, the progress job (copy `BounceJob`). Check: Lead bounced plays the same with the original muted.
- [x] 6. Audio clips: *Export clip as WAV file...* and the drag out (copy `midiactions::dragClipOut`, in `TrackLanes`' drag), tooltip.
- [x] 7. `RoutingExplainer` pictures for the two topics (file tag on a node).
- [x] 8. `ScreenshotRunner` scenes: `bounce` (opens the Bounce window with the shot's `select` tracks) and `bounceInPlace` (bounces the shot's track in place).

**Group C (engine, Go 2); `./build.sh --test` at step 10.**
- [ ] 9. Multi-file export in `daw::bounce`: the plan (tracks or stems, names, skipped and why), rendering each with its focus (or chain for dry), same length, cancel removes written files, About these files.txt and Tempo map.mid (with `midiexport`). Tests below.
- [ ] 10. Topic 3 and the "your song" rules in `RoutingGuide`. Tests below.

**Group D (UI, Go 2); `./build.sh --test` at step 12.**
- [ ] 11. Bounce window: multitracks, stems, *Sound*, the folder save, the report.
- [ ] 12. Stems picture tags.

- [ ] 13. Real-app check (Go 1 part done: bounce scene, bounceInPlace, the two topics; Go 2 part open) (after each go): shots: `bounce` scene with Kick and Snare selected (window shows *Selected tracks: Kick, Snare* and the hint, nothing cut off); `bounceInPlace` on the example song's lead MIDI track (a "(bounced)" track right below with a waveform, the original muted, same bus); routing guide on *Bouncing to audio*, *Bounce in place*, and in Go 2 *Stems and multitracks* and *Your song's routing* (counts and advice); Go 2: `bounce` scene with *What* on stems.
- [ ] 14. Docs (Go 1 part done: README bullets, tests, code-layout, architecture, ideas status; Go 2 part open): README *Bounce to WAV* bullet (What, selected, multitracks/stems, Sound) and a *Bounce in place* bullet, audio clip drag out in the clip bullet; docs/tests.md (bounce and routing guide lines); docs/code-layout.md (Bounce, BounceDialog); docs/architecture.md's bouncing line (the focus: heard as soloed, one chain); FEATURE_IDEAS status (move done parts; after Go 2 the idea is done and moves to features_done.html).

## Tests
In `runBounceTests` (and `runRoutingGuideTests`), on songs of audio clips with made-up buffers (deterministic):
- Selected {A} equals the mix with B muted (≤ 1e-6); the user soloing B first changes nothing in it; solo atomics unchanged afterwards. A in a bus at +6 dB: peak ×1.995. A sending to an empty return: the file includes the return share (equals the mix with B muted).
- Chain: an audio track with DawIt EQ +6 dB, fader -10 dB, pan hard left: equals the clip through the EQ alone, both channels, no fader. Clip filter: bouncing the second of two clips equals bouncing it with the first deleted (≤ 1e-5). Instrument only: equals the clip × clip gain (≤ 1e-6).
- Bounce in place: new track right below, same bus, volume, pan, sends (level and pre-fader); original muted; instrument and `builtInSound` unchanged; clip starts at the first clip's start; the mix after equals the mix before (≤ 1e-4, audio-clip track with an EQ). Clip mode mutes only the clip and reuses an existing "(bounced)" track with no overlap. Peak 2.0: stored peak 0.8913, gain +7.02 dB. MIDI latency check with bypassed DawIt Tune (≤ 1e-5). Save and load keeps it all.
- Clip to WAV: 24-bit equals audio × gain (≤ 1e-6), its rate and channels; 32-bit float when the peak is over 1.
- Go 2: multitracks of 3 tracks (one bus with DawIt EQ, an empty return with sends) sum to the mix (≤ 1e-4); stems likewise; all files the same length; names "01 Kick.wav"; muted and clipless tracks skipped with the reason; a pop-shaped song plans 5 stems and 13 multitracks; cancel leaves no files; About text holds the tempo, key, rate and bit depth; Tempo map.mid has the tempo changes.
- Routing guide: 17 topics after Go 1, 18 after Go 2; each new one has text and an example; "bounce in place", "stems" and "multitracks" find their topics; rule 2 fires with DawIt Compressor on a 2-track bus and not with DawIt EQ; rule 1's counts. `--export-guides` still passes.

## Website hand-off
- Features page: extend the Bounce section: the *What* choices, Bounce in place (before/after shot of a MIDI lead and its bounced track), stems and multitracks for another program.
- Main page: not needed.
- Screenshots: the Bounce window with *Selected tracks: Kick, Snare*; the arrangement after Bounce in place on the example song's lead; the routing guide on *Stems and multitracks*.

## Left for later
- Returns exported as their own files, with the tracks' sends left out (Ableton's default).
- Bounce in place with only the instrument, the effects copied live to the new track; bouncing buses and returns in place; replacing the original instead of muting it.
- Dragging a MIDI clip out as rendered audio; a full mix file in the stems folder.
- Master effects (DawIt has none yet); stems through them would need the "include master effects" choice.

## Changes while building

**Group A hand-off (engine, Go 1; built and tested, `./build.sh --test` passed with no warnings).**
- `BounceFocus` (in `Engine.h`: modes `mix`, `heardAsSoloed` with `tracks`, `oneChain` with `track`, `clip`, `instrumentOnly`); `Engine::setBounceFocus` / `clearBounceFocus`; honoured only while offline, cleared by `endOffline`. `Track::countsAsSoloed` replaces reads of `soloed` on the audio path (sends included); `RenderContext` gained `unityOutput`, `instrumentOnly`, `onlyClip`. `Track::getEffectsLatency()` is public.
- `bounce::Settings` gained `keepWholeTail` and `focus`. `Renderer::renderToBuffer (AudioBuffer&, keepGoing)` renders to memory (stereo, settings' rate); `render (file, keepGoing)` as before. A MIDI track's effects latency is rendered extra and dropped by the Renderer itself.
- In `Bounce.h`: `InPlacePlan planInPlace (engine, track, clip*)` (checks, range), `getInPlaceSettings (plan)` (one chain, 48 kHz, 4 s tail), `addBouncedTrack (engine, plan, buffer)` (message thread, after rendering: placement, copies, muting, peak scaling, `InPlaceResult::note` says what to tell the user, `track`/`clip` for selecting), `bounceInPlace (engine, track, clip*)` (synchronous, for tests), `writeClipToWav (clip, file, int* bits)`. The UI job (step 5) should: plan, build a `Renderer` with `getInPlaceSettings`, `renderToBuffer` on a thread with progress, then `addBouncedTrack` on the message thread (it checks the track and clip still exist).
- Routing guide: `Topic::bounce`, `Topic::inPlace` (before `yourSong`; 17 topics); `getExample (bounce)` is the pop example with Kick, Snare and Drums `onSelectedPath` and all 15 others `silent`; `getExample (inPlace)` is Synths bus, Lead (muted, silent), Lead (bounced), Reverb return. `getExampleFileTag (Topic)` returns "Kick + Snare.wav" for `bounce`, to be drawn by the master in `RoutingExplainer` (step 7). For Go 2's stems, per-node tags are still to be added as a `Node` field.
- Changed on purpose: a bounce now starts at its gains with no 20 ms fade-in (`Track::snapGains`, set by `beginOffline`; the master volume likewise); this only affects offline renders.
- Surprise: the built-in piano and organ start from a random phase, so two bounces of them are not bit-equal; the MIDI latency test uses the pluck sound. DawIt EQ's match level learns as it plays, so chain tests compare first renders of fresh songs.
- Tests: `runBounceFocusTests`, `runBounceInPlaceTests` (new, after `runBounceTests`), and the routing guide checks (topic count 17 in three places, texts, examples, search).


**Group B hand-off (UI, Go 1; `./build.sh --test` passed, no warnings; shots looked at).**
- New `src/ui/BounceActions.{h,cpp}` (`bounceui::bounceInPlace (Engine&, Selection&, Track&, Clip*)`: plan, `Renderer::renderToBuffer` on a progress thread, `addBouncedTrack`, selects the result; `exportClipAsWav`; `dragClipOut` for audio, to a "DawIt Audio" temp folder). `BounceDialog` has a *What* row (`whatBox`: The mix / Selected tracks: ...), `showDialog (engine, name, selected, pickSelected)`; the hint changes with it; `chosen` (the outermost selected) feeds `settings.focus`. Go 2 adds ids and rows next to `mixId`/`selectedId`, and `export.bounceWhat` is not yet remembered (nothing but the mix and selected exist).
- Menus: Track menu *Bounce in Place* (Cmd+Shift+B, `bounceInPlaceCommand` in MainComponent); track right-click *Bounce to WAV...* (`TrackActions::onBounceToWav`) and *Bounce in place*; clip menu *Bounce clip in place* (`TrackLanes::onBounceClip`) and for audio *Export clip as WAV file...*; the clip drag out now works for audio clips too. The lanes' hint line says "Drag a clip out of the window ... as a MIDI or WAV file". The plan's "audio clip tooltip" has no clip tooltip to extend, so the hint line carries it.
- Routing explainer draws the file tag under the master (`RoutingPicture::setFileTag`, from `getExampleFileTag`). Go 2 still needs per-node tags.
- `ScreenshotRunner`: scenes `bounce` and `bounceInPlace`; `"select"` takes a list.
- Real-app shots (scratchpad, scale 1): `bounce.png` (window, "Selected tracks: Kick, Snare" and the hint, nothing cut), `inplace.png` (capture main after `bounceInPlace` on "Lead vocal": the new "Lead vocal (bounced)" track is selected in the clip editor with a waveform, bus Vocals, 48 kHz stereo, peak -6.8 dB; its lane is below the visible rows), `g1.png`, `g2.png` (the two guide topics; the file tag "Kick + Snare.wav" sits under the master). Not looked at in the real app: the menus and the actual drag out of a window (they need a mouse).
- Changed: the Bounce window is 256 high; the hint says "Kick and Snare" (and "the 4 selected tracks" past three).
