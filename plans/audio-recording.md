# Audio recording

Status: Done, Theory checked
Idea: [Audio recording](../FEATURE_IDEAS.html#audio-recording)

## Goal
Record a voice, a guitar or any sound from an audio input onto an audio track, the way MIDI recording already works: arm the track with R, press Record, and the take lands on the track in time. A singer or guitarist no longer needs another program. A new guide, *How recording works*, explains interfaces, input levels, latency, monitoring and takes with the user's own device as the example.

## Decisions for you
1. **Where the guide lives.** Options: a new guide in the *Sound* area; a new *Recording* area; topics added to *Singing techniques*. **Recommended:** a new guide *How recording works* in *Sound*, because it is about any instrument, not only voice, and one guide per area keeps the picker short. Not on *Start here*.
   Answer: Recommended option accepted.
2. **Loop recording.** Options: every pass a take, stacked as clips at the same place, the newest complete one playing and the rest muted; add every pass on top (as MIDI does); keep only the last pass. **Recommended:** stacked takes with a *Takes* submenu to choose, because it needs no new saved data (muted clips already save) and loses nothing. Comping by parts is left for later.
   Answer: Stacked takes (recommended).
3. **Where a track's input is saved.** Options: in the project (left out when it is Input 1, mono); in the app settings, as the idea says. **Recommended:** the project, because tracks have no stable identity across projects to key a setting on, and a vocal track belongs to its socket. A missing channel on another interface falls back to Input 1.
   Answer: The project (recommended).
4. **Monitoring through DawIt.** Options: the input through the track's effects and fader; dry. **Recommended:** through the effects (a singer hears the reverb), at the cost of the effects' latency. Monitor is never saved and starts off.
   Answer: Recommended option accepted.
5. **Detecting a refused microphone permission.** Options: notice input that is exactly zero for 1 s while armed, then explain (cross-platform, no new code language); ask macOS through AVFoundation in an Objective-C++ file. **Recommended:** the zero-input check, because it also catches a wrong or dead input, and JUCE's `RuntimePermissions` is a no-op on macOS.
   Answer: Recommended option accepted.
6. **File format.** Options: 24-bit WAV; 32-bit float WAV. **Recommended:** 24-bit, the studio norm and what the guide's numbers use; an input can't exceed 0 dBFS, so float gains nothing.
   Answer: Recommended option accepted.
7. **Recording over existing clips.** Options: mute the old clips the take overlaps; leave them playing; cut them. **Recommended:** mute them, so the old clip becomes another take to go back to (DawIt has no undo).
   Answer: Recommended option accepted.
8. **Measure latency in this plan.** Options: now; later. **Recommended:** now: it is small, testable with the fake device, and the guide's timing topic depends on it.
   Answer: Recommended option accepted.

## What the user does and sees
- **Arming:** audio tracks get the R button MIDI tracks have. Armed, the header shows an input chip after R ("In 1", "In 1+2"); click it (or right-click R, or the track menu's *Input*) for a menu of the device's inputs: each channel mono, then each stereo pair, with the device's channel names. A *Monitor* toggle (headphone glyph) appears on armed audio tracks. Arming opens the device's inputs; disarming the last audio track closes them, so the macOS microphone dot isn't on while nobody records. R and the input can't change while recording.
- **Input meter:** an armed audio track's header meter and mixer strip show the input level, with marks at -6 and 0 dB and `gainstaging::getZone`'s zones; a red over-0 mark holds 2 s. Tooltip: "Input level: aim for peaks around -18 to -10 dB while you play the loudest part; red means it clipped".
- **Monitor:** the first time it's turned on, a one-off message: "Use headphones. On speakers the microphone hears itself and howls. Monitor plays the input through this track's effects, about 6.4 ms late now; your interface's direct monitoring has no delay." (the real round trip). Remembered as `recording.monitorWarned`.
- **Record (R):** records every armed track, MIDI and audio together, after the count-in, with the metronome as now. Nothing armed and no MIDI track selected: "Nothing to record on. Arm a track with its R button: a MIDI track records your keyboard, an audio track your microphone or instrument."
- **While recording:** each armed audio track's lane shows a red region growing from where recording started, with the waveform drawn as it comes in and the name "Vocals take 3".
- **After stopping:** the take is a clip starting where recording started (not snapped to the bar), named "<track> take <n>". Overlapped old clips are muted. A take quieter than -60 dB throughout is removed with a message: "Nothing came in on Input 2 for Guitar: check the cable, the input chosen next to R, and the gain knob." Exactly-zero input for 1 s while armed: "DawIt hears nothing at all from the input. If macOS asked for the microphone and you said no, allow DawIt in System Settings > Privacy & Security > Microphone, then quit and reopen DawIt." with a button that opens that settings pane. Lost samples (message thread stalled more than 10 s): "1.2 s of the take was lost and is silent."
- **Takes:** right-click an audio clip: *Takes* submenu listing the clips on the track that overlap it ("Take 2 (playing)"); choosing one unmutes it and mutes the others that overlap it; *Delete other takes*. Muted takes are drawn under the playing one. The menu ends with *About recording audio...*. Tooltip over a take: "Vocals take 2 (muted) · 1 of 3 takes here: right-click > Takes to choose".
- **Audio / MIDI Settings** becomes a small panel: the device selector now with the input device, an input meter, the latency line ("Round trip 6.4 ms: 3.1 in, 3.3 out, reported" or "... measured, 1.5 ms more than reported"), *Measure latency* and a *?* that opens the guide's timing topic. Measure tooltip: "Plays eight clicks and listens for them coming back, to find your interface's real delay. Connect an output to an input with a cable, or put the microphone near a speaker".
- **Remembered:** `recording.monitorWarned`, `recording.extraLatencyMs.<input device>|<output device>`; the device itself as now.

## Data and saving
- An audio track's input: its first channel (0-based) and whether it is a stereo pair; saved on audio tracks only when not Input 1 mono. No format version change: older builds ignore the keys. Older projects load as Input 1, mono. Arm and monitor aren't saved.
- Takes are ordinary audio clips and WAV files; muted takes use the existing clip `muted`. Takes go to the project's `Audio Files` folder when the project has a file, else to `~/Library/Application Support/DawIt/Recordings`, which `collectAudio` copies in on save. `ProjectManager` tells the engine the folder on New, Open and Save.
- Tests: input choice round trip; default left out; a project without it loads Input 1 mono; a saved take's file sits in `Audio Files`.

## The guide
New guide `recordingguide` (`src/engine/RecordingGuide`, engine because it reads the device), window `RecordingExplainer` copying `RoutingExplainer`, owned by `MainComponent` like the routing guide. Topics (texts in *Theory*):

| Topic | Summary | Picture |
|---|---|---|
| From the air to a file | What happens between your voice and the take | Five boxes: microphone, interface (preamp, converter), buffer, DawIt, WAV file; the live input level in the converter box; the device's name and rate on it |
| The audio interface | Mic, instrument and line inputs, and 48 V phantom power | Bars of the three levels on one volt scale (mic about 2.5 mV, line 1.23 V), with the gain needed between them |
| Choosing an input | Which socket a track records, mono or stereo | The device's input channels as sockets, a line from each to the armed track recording it |
| Setting the level | Loudest peaks around -18 to -10 dB, never at 0 | One sung phrase three times: peaks at -36 dB, -12 dB and clipped flat at 0, with the meter zones; the armed track's live meter |
| Buffers and latency | Why sound through the computer comes out a little late | A time line: in-buffer, DawIt, out-buffer, with ms per stage from the current device; a table of buffer sizes at its rate |
| Hearing yourself | Direct monitoring, the Monitor button, and headphones | Two paths to the headphones: direct (0 ms) and through DawIt (the round trip); a speaker-to-mic loop marked "howl" |
| Why takes are moved earlier | Latency correction and Measure latency | A click on the beat, the same click recorded a round trip later, an arrow moving it back by the reported and measured amounts |
| Takes and loop recording | Several passes, keep the best | The loop range with three stacked takes and a cut-short fourth, the newest complete one playing |
| Your session | Your device, its latency and your last take | The topic 1 chain with this session's numbers and every armed track's meter; the advice below it |

*Your session* reads a snapshot (device names, rate, buffer, reported latencies, measured extra, number of inputs, armed tracks with input and monitor, last take's peak, zero-input flag) through a pure analysis in the guide content; rules in *Theory*.

Opens from: the R button's and the track's right-click menu (*About recording audio...*), a take's clip menu, the settings panel's *?* (timing topic), the Monitor warning (*Hearing yourself*), the Theory Guides window. The singing guide's *The microphone* topic mentions it. Entry: area *Sound*, title "How recording works", summary "Recording a voice or instrument: the audio interface, input levels, latency, monitoring and takes, with your own device as the example.", search words "record recording audio interface microphone mic input preamp gain phantom 48v condenser dynamic hi-z instrument line level latency buffer delay monitoring monitor headphones feedback howl take takes comp comping loop clipping clip level xlr usb". Not on *Start here*.

## Code
Recipes: *A theory guide* (routing guide as the model), *A command or menu item* (track commands in `TrackActions`), *A window*, *Checking in the real app*.
- **Track input:** an audio track's input choice is two atomics (first channel, stereo); the audio thread reads them.
- **Inputs on demand:** the engine opens the input device's channels (up to 8) while any audio track is armed or the settings panel shows its meter, and none otherwise; at start-up a saved device state is restored without inputs (tests and `--screenshots` must never open a microphone).
- **The callback** passes the device's input buffers to `processBlock`; offline bounces get none. Recording stops before `beginOffline`, and when the device stops or changes rate (what was recorded is kept).
- **Capture (audio thread):** for each armed audio track, copies its channels into a per-track lock-free FIFO (10 s at the device rate, made under the lock when recording starts) once the count-in is over; writes the input level atomics (falling level, any number of readers) whether recording or not; notes the beat of the first captured sample and the stream positions of loop wraps; counts dropped samples. Never allocates or locks.
- **Recorder (message thread, the existing 30 Hz recording timer):** drains the FIFOs into each take's growing buffer and a `juce::AudioFormatWriter::ThreadedWriter` on a raw WAV (crash safety). Takes are not in the edit while recording; the lane draws them from the recorder's state. On stop: cut passes, drop the latency samples, write each take's 24-bit WAV, delete the raw file, add the clips under the lock.
- **Latency rule:** shift = reported input + output latency (JUCE's CoreAudio figures each include one buffer) + the measured extra for this device pair. The first `shift` samples of the stream are dropped, so sample 0 of a take is the capture beat; pass cuts are at wrap position + shift.
- **Takes rule:** each pass is a clip; the newest pass that reached the loop end plays, all others muted; a pass cut short by stopping is kept muted unless it is the only one; a take whose peak is under -60 dBFS (`gainstaging::silentBelowDb`) is removed with its file.
- **Monitoring:** with Monitor on, the track's input is added into its chain where clips play, so it passes the effects, sends, fader and pan, and follows mute and solo like the clips. Never recorded.
- **Measure latency:** a pure onset finder (click times and the recorded input in, round trip out) plus an engine routine that plays 8 clicks 0.25 s apart at -12 dBFS outside the song.
- **Risky:** an audio-thread change (input path, FIFOs); the device reopening when arming (a short dropout, acceptable); the microphone permission (CMake `MICROPHONE_PERMISSION_ENABLED` and text; a hardened-runtime build would also need `com.apple.security.device.audio-input`).

## Libraries and models
None. JUCE's `WavAudioFormat`, `ThreadedWriter`, `AbstractFifo` and `AudioDeviceSelectorComponent`.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| F1 | One buffer lasts size ÷ rate: 64/48 kHz 1.333 ms, 128/48 kHz 2.667 ms, 256/48 kHz 5.333 ms, 512/44.1 kHz 11.61 ms, 1024/48 kHz 21.33 ms | derived | exact |
| F2 | A round trip passes at least two buffers: 128 at 48 kHz is at least 5.333 ms | derived | exact |
| F3 | Measured round trips at 128 samples, 48 kHz are about 6 to 12 ms (RME Digiface USB 6.8 ms, ESI Maya44 11.8 ms) | RTL Utility reports on the RME forum and Thomann reviews | typical |
| F4 | Sound travels 0.343 m per ms; 5.333 ms is 1.83 m | derived from `soundguide::speedOfSound` | exact |
| F5 | Shure recommends 5 ms or less of latency for in-ear monitoring | Shure, *Digital wireless latency explained* | typical |
| F6 | Accepted latency ranged from 42 ms to under 1.4 ms depending on instrument and monitoring; in-ears tolerate less than wedges | Lester and Boley, AES Convention Paper 7198 (2007), abstract | typical |
| F7 | A smaller buffer gives the CPU less time per batch, so heavy sessions click and drop out; 64 to 128 for recording, 512 to 1024 for mixing | Sound On Sound *Q: What buffer size should I use?*; Focusrite support | opinion |
| F8 | 0 dBFS is the largest value a converter or fixed-point file can hold; louder is clipped | definition (as the routing guide states) | exact |
| F9 | 24-bit range 20 log10 2^24 = 144.5 dB; 16-bit 96.3 dB | derived; matches the sound guide's 96 dB | exact |
| F10 | Record with loudest peaks around -18 to -10 dBFS and under -6; SOS: an average around -18 dBFS RMS, and -6 dBFS peaks are safe but more than 24-bit needs | singing guide's *The microphone*; Sound On Sound *Q: Should I gain-stage to -6 or -18 dBFS?* | opinion |
| F11 | +4 dBu = 0.775 × 10^(4/20) = 1.228 V; -10 dBV = 0.316 V | definition (Wikipedia *Line level*) | exact |
| F12 | Microphones give a few millivolts (about 2.5 mV, -50 dBu, as a nominal figure) | Wikipedia *Line level*; sources vary | typical |
| F13 | Mic preamps give about 30 to 60 dB of gain: 31.6 to 1000 times the voltage | derived ratio; range typical | typical |
| F14 | Passive guitar pickups need a high-impedance (Inst, Hi-Z) input; on a line input they sound weak and dull | Wikipedia *Line level*; Gearank | typical |
| F15 | Phantom power: 48 V (P48, IEC 61938) for condenser mics; dynamic mics don't need it and a balanced one isn't affected; ribbons: follow the maker | Wikipedia *Phantom power*; DPA *Know the basics about phantom power* | exact (voltage) / typical (care) |
| F16 | Twice the distance from a mic is about 6 dB quieter | singing guide (inverse square law) | exact in open air |
| F17 | macOS asks the first time an app opens the microphone; refused, the input gives only zeros with no error; reopening the app may be needed after allowing | developer reports (dev.to *2000 buffers of nothing*, Apple developer forums) | typical, unconfirmed by Apple |
| F18 | 24-bit WAV at 48 kHz: 144,000 bytes/s mono, 8.64 MB a minute; 17.28 MB stereo | derived | exact |
| F19 | A loop from speaker to microphone with a gain of 1 or more howls (feedback) | acoustics, the Larsen effect | exact |
| F20 | Comping is building one performance from the best parts of several takes | common studio practice | typical |

### Topic texts

**From the air to a file** — *What happens between your voice and the take*

Recording turns sound in the room into a clip on a track. On the way it passes five stops, the boxes in the picture.

1. The microphone turns the moving air into a tiny electrical signal, a few thousandths of a volt. An electric guitar's pickups do the same from the strings.
2. The audio interface's preamp makes that signal bigger, often by 30 to 60 dB (30 to 1,000 times); its gain knob sets how much. Its converter then measures the signal 48,000 times a second at 48 kHz and turns it into numbers (What is sound? explains samples). A laptop's built-in microphone has all of this inside the computer.
3. The numbers arrive in small batches, the buffer, for example 128 samples at a time. DawIt takes a batch in and sends the next batch of the song out, hundreds of times a second.
4. DawIt writes what comes in to a WAV file while you play, so a crash doesn't lose the take. A minute of mono at 48 kHz and 24 bits is about 8.6 MB.
5. When you stop, the take becomes a clip on the track, moved earlier by the delay of the trip (Why takes are moved earlier says why).

What is saved is the input as it came in. The track's effects and fader change what you hear, not the take, so you can change them afterwards without recording again.

To record: arm an audio track with its R button, choose its input in the chip next to R, and press Record (or R). The count-in and metronome work as for MIDI. Press Record again, or Stop, to end the take.

**The audio interface** — *Mic, instrument and line inputs, and 48 V phantom power*

An audio interface is the box between your instruments and the computer: inputs with gain knobs, converters both ways, and outputs for speakers and headphones. The Mac's built-in microphone is fine for trying ideas out; an interface sounds cleaner, takes proper microphones and instruments, and lets you hear yourself without delay.

Three kinds of signal come in, at very different strengths:
- Mic level: a few thousandths of a volt (about 2.5 mV is a usual figure). It needs the most gain.
- Instrument level: an electric guitar or bass straight from its jack. Stronger than a microphone, weaker than line level, and different for every guitar. Use the input marked Inst or Hi-Z: it's made for pickups, which sound thin and dull on an ordinary input.
- Line level: keyboards, drum machines, a mixer's output. Studio gear works at +4 dBu, about 1.23 volts; home gear at -10 dBV, about 0.32 volts. It needs little or no gain.

Most interfaces have combo sockets that take a microphone's XLR plug or a jack.

Phantom power, the 48V button, sends 48 volts up the microphone cable to run a condenser microphone's electronics (the standard, IEC 61938, calls it P48). Dynamic microphones such as the Shure SM58 don't need it, and a normal balanced cable keeps it away from them. Ribbon microphones are the exception: leave it off unless the maker says yours needs it.

DawIt records at the sample rate set in Audio / MIDI Settings and saves 24-bit WAV files. 44.1 or 48 kHz is fine for music.

This topic doesn't say which interface or microphone to buy; the number of inputs you need at once is the first thing to decide.

**Choosing an input** — *Which socket a track records, mono or stereo*

Each socket on the interface is a numbered input: Input 1, Input 2 and so on. A track records the one shown next to its R button; click it to choose.

Mono or stereo? One microphone or one guitar is one channel: record it mono, on Input 1 or Input 2. Mono isn't worse. It's one sound from one place, and the pan knob puts it where you want it. A stereo pair (Inputs 1+2) is for sources that really are two channels: a keyboard's left and right outputs, or two microphones set up as a pair. A single microphone recorded as stereo gives one silent side, or two identical ones if both inputs hear it: twice the file for nothing.

Arm several tracks to record several inputs at once: a singer on Input 1 and a guitar on Input 2 become two takes, mixed separately later. Two tracks on the same input record the same sound twice.

The picture shows your interface's inputs and which armed track takes each. DawIt opens the inputs only while an audio track is armed; on a Mac the microphone dot in the menu bar goes away when you disarm.

The first time, macOS asks whether DawIt may use the microphone. If you said no, DawIt hears only silence: allow it in System Settings > Privacy & Security > Microphone, then quit and reopen DawIt.

**Setting the level** — *Loudest peaks around -18 to -10 dB, never at 0*

The gain knob on the interface sets how loud the take is. Set it before recording, while you sing or play the loudest part of the song: an armed track's meter shows the input.

A common aim is loudest peaks around -18 to -10 dB, and always under -6 dB. The meter marks -6 and 0 dB, as the gain staging badges do.

Why not as loud as possible? The converter has a hard ceiling, 0 dBFS (full scale): the biggest number it can write. Anything louder has the top of its wave cut flat, the right of the picture, and sounds like crackle and buzz. Inside DawIt a track can go over 0 dB and be turned down again; at the converter it can't, and no effect repairs it. Singers get louder in the chorus than in the sound check, so leave room.

Why not very quiet? A 24-bit file has about 144 dB between its smallest step and its top, so a take that peaks at -30 dB has lost nothing you'd hear. But a quiet take is hard to see and judge, and effects' starting points expect a normal level (a vocal compressor's threshold of -18 dB does nothing to a voice that never reaches it). Below about -24 dB, turn the gain up.

Engineers differ on the exact aim. Some say peaks near -6 dB, Sound On Sound an average around -18 dB. At 24 bits any of these is fine; what matters is staying clear of 0.

The level also changes with distance: twice as far from the microphone is about 6 dB quieter (The microphone, in Singing techniques). So set the gain where you'll really stand.

**Buffers and latency** — *Why sound through the computer comes out a little late*

Latency is the delay between a sound going in and coming out again. The computer can't handle one sample at a time, so it works in batches, the buffer. While it fills one batch from the input and plays the one before, time passes.

One buffer lasts its size divided by the sample rate:
- 64 samples at 48 kHz: 1.3 ms
- 128 samples at 48 kHz: 2.7 ms
- 256 samples at 48 kHz: 5.3 ms
- 512 samples at 44.1 kHz: 11.6 ms
- 1024 samples at 48 kHz: 21.3 ms

A sound passes at least two buffers, one in and one out, plus a little for the converters and the driver. So the round trip at 128 samples and 48 kHz is at least 5.3 ms, and interfaces measure about 6 to 12 ms there, depending on the interface and its driver.

How much is too much? Sound travels about 34 cm in a millisecond, so 5.3 ms is like standing 1.8 m from a speaker, which guitarists do all the time. Singers notice most, because they also hear their own voice inside their head with no delay at all. Shure recommends 5 ms or less for singers' in-ear monitors, and a study for Shure (Lester and Boley, 2007) found that what players accept ranges from about 42 ms down to under 1.4 ms, depending on the instrument and how they listen.

Smaller isn't free. With a smaller buffer the computer has less time for each batch, and with many effects it runs out: you hear clicks and dropouts. A common way of working is 64 or 128 samples while recording and 512 or 1024 while mixing. Change it in Audio / MIDI Settings.

Latency matters for what you hear while you play. The take itself is put back in time afterwards (Why takes are moved earlier).

**Hearing yourself** — *Direct monitoring, the Monitor button, and headphones*

Monitoring is hearing what you record while you record it. There are two ways, the two paths in the picture.

Direct monitoring: most interfaces can send the input straight to the headphones before it reaches the computer, with a knob or button marked Direct Monitor or Mix. There's no delay you'd notice, so it's the best way for singers. You hear yourself dry, without DawIt's effects.

Through DawIt: the Monitor button on an armed audio track plays the input through the track's effects and fader, so you can hear your voice with its reverb. It comes back a round trip late (Your session shows how late), plus whatever delay the track's effects add. Use one way, not both: with both on you hear yourself twice a few milliseconds apart, which sounds hollow (What is sound?, Phase cancellation).

Use headphones, not speakers. A microphone that hears the speakers playing itself makes a loop, and once the loop gives back as much as it gets, it howls: feedback. That's why Monitor is off whenever DawIt starts. Even without monitoring, speakers leak the song and the metronome into the take; closed-back headphones keep them out.

What you hear isn't what's recorded: the take is always the input as it came in, without the effects you monitor through.

**Why takes are moved earlier** — *Latency correction and Measure latency*

You play along with what you hear, and what you hear left the computer a moment ago. What you play then takes a moment to come back in. So a note played exactly on the beat arrives in DawIt late by the whole round trip: the output's latency plus the input's.

DawIt moves each take earlier by that much, so it lands where you played it. For example, a device reporting 3.1 ms in and 3.3 ms out has a take moved 6.4 ms earlier. Uncorrected, 6.4 ms doesn't sound like an echo, but a tight part feels a little behind the drums, and a second take played along with the first ends up later still.

DawIt uses the latency the device reports. Some drivers leave part out, such as the converters' own delay. Measure latency, in Audio / MIDI Settings, finds the real round trip: it plays eight clicks and listens for them coming back. Connect an output to an input with a cable (the most exact), or put the microphone close to a speaker, and set the level so the clicks are clear but not clipping. DawIt remembers the difference for that interface and adds it to every take. Measure again when you change the interface or its sample rate; a buffer change is already in the reported part.

Correction fixes the trip, not the playing: a note you played late stays late.

**Takes and loop recording** — *Several passes, keep the best*

A take is one recording of a part. Few performances are right the first time, so record a few and keep the best.

With the loop on (the loop range in the ruler), Record goes round and round, and every pass becomes its own take at the same place on the track. The newest complete pass plays; the older ones stay underneath, muted. Right-click a take and choose Takes to hear another one: it plays and the others there are muted. Delete other takes keeps only the one you chose. A pass you stopped half-way through is kept too, muted.

MIDI recording works differently: with the loop on, each pass adds its notes to the same clip, because notes can be layered and deleted one by one. Audio can't.

Recording over a clip already on the track mutes the old clip instead of deleting it, so it becomes one more take to go back to.

Comping, short for compiling, is the studio way of building one performance from the best parts of several takes: the first line from take 2, the chorus from take 4. DawIt doesn't comp yet. Split the takes (right-click, Split at playhead) and mute the parts you don't want, or choose a whole take.

A take with nothing in it, quieter than -60 dB from start to end, is removed, and DawIt says so. It usually means the wrong input was chosen.

**Your session** — *Your device, its latency and your last take*

Built from the snapshot, then the advice. Example with numbers:
"Input: Scarlett 2i2 USB, 2 inputs. Output: Scarlett 2i2 USB. 48 kHz, buffer 128 samples (2.7 ms).
Round trip 6.4 ms: 3.1 ms in, 3.3 ms out, as the device reports.
Armed: Vocals on Input 1 (mono), Monitor off.
Last take: Vocals take 3, loudest peak -12.4 dB."

### Your session rules
| Condition | Advice | Quiet when |
|---|---|---|
| No input channels | "DawIt can't find an audio input. Plug in an audio interface, or choose an input device in Audio / MIDI Settings (the Mac's built-in microphone counts)." | inputs exist |
| Exactly zero input for 1 s while armed | the permission message from *What the user does and sees* | not armed, or any non-zero sample |
| Round trip ≥ 12 ms and Monitor on | "You hear yourself 14.2 ms late through DawIt. Try a buffer of 128 or 64 samples, or turn Monitor off and use your interface's direct monitoring." | Monitor off everywhere |
| Buffer ≥ 512 and a track armed | "A buffer of 512 samples suits mixing. For recording, 128 or 64 makes playing and monitoring feel immediate; raise it again if you hear clicks." | nothing armed |
| Last take peak ≥ -0.1 dB | "Your last take reached 0 dB: its loudest moments were cut off and can't be repaired. Turn the gain down and record it again." | no take this session |
| -6 ≤ peak < -0.1 dB | "Your last take peaked at -3.2 dB, close to the top. A little lower, peaks around -18 to -10 dB, leaves room for a louder moment." | no take |
| -24 ≤ peak < -6 dB | "Your last take peaked at -12.4 dB: a good level." | no take |
| -60 ≤ peak < -24 dB | "Your last take peaked at -31.0 dB: quiet but not lost, since 24 bits has room. Turn the gain up so peaks reach about -18 to -10 dB; then the meters and effects' starting points match it." | no take |
| No measured latency | "Takes are timed by the latency your device reports (6.4 ms). Measure it once in Audio / MIDI Settings so takes land exactly in time." | no input device |
| Measured extra ≥ 1 ms (either sign) | "Measuring found 1.5 ms more than the device reports; takes move by that too." | not measured, or under 1 ms |

### Example data
- Latency table (F1) at the current rate, round trip = 2 buffers + reported extras.
- Level picture: a 1 s phrase with peaks at -36, -12 and clipped at 0 dBFS (amplitudes 0.0158, 0.251, 1.0 with the wave scaled ×2 and cut at 1).
- Takes picture: loop bars 9 to 12; pass 1 from bar 10 (started mid-loop), passes 2 and 3 full, pass 4 cut at bar 11; pass 3 playing.
- Interface picture: 2.5 mV, 0.316 V, 1.228 V on a log scale (gain from mic to line: 20 log10 (1.228 / 0.0025) = 53.8 dB).

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| buffer ms 64/48000, 128/48000, 256/48000, 512/44100, 1024/48000 | 1.333, 2.667, 5.333, 11.610, 21.333 | 0.001 |
| minimum round trip 128/48000 | 5.333 ms | 0.001 |
| distance for 5.333 ms | 1.83 m | 0.01 |
| 24-bit and 16-bit range | 144.5 dB, 96.3 dB | 0.1 |
| +4 dBu, -10 dBV | 1.228 V, 0.316 V | 0.001 |
| mic to line gain | 53.8 dB | 0.1 |
| 24-bit mono 48 kHz per minute | 8.64 MB | 0.01 |
| advice thresholds | -0.1, -6, -24, -60 dB, 12 ms, 512, 1 ms as in the rules | exact |

## Steps

Group A1, engine: input and takes (`./build.sh --test` at step 4):
- [x] 1. Track input choice (atomics), saved on audio tracks when not Input 1 mono; `setArmed` accepts audio tracks. CMake: `MICROPHONE_PERMISSION_ENABLED` with the text "DawIt records your voice or instrument onto a track. Nothing leaves your computer." Tests: round trip, default left out, old project loads mono Input 1. Done when the tests pass.
- [x] 2. Inputs to the engine: the callback passes input buffers to `processBlock`; input level atomics for armed audio tracks; Monitor through the chain; the "inputs wanted" state (armed audio track or settings meter) that opens and closes inputs; start-up restores the device without inputs. Give `FakeDevice` input channels and settable latencies. Tests: monitoring checks below.
- [x] 3. The recorder: per-track FIFOs, capture after the count-in, raw `ThreadedWriter`, latency shift, take files and clips on stop, silence removal, overlapped clips muted, recording folder setter, stop on bounce or device change, dropped-sample count. `startRecording` records armed audio and MIDI together and returns false only when nothing at all can record. Tests: the recording checks.
- [x] 4. Loop passes as takes, and pure take helpers next to the clip type (the takes overlapping a clip; choose one; delete the others). Tests: the loop and takes checks. Done when `./build.sh --test` passes with no warnings.

Group A2, engine: latency and guide content (`./build.sh --test` at step 7):
- [x] 5. Measure latency: the pure onset finder and the engine routine, plus storing the extra per device pair in settings. Tests: the latency checks.
- [x] 6. `recordingguide` content in `src/engine/RecordingGuide.{h,cpp}`: topics, texts from *Theory*, example data, the formulas (buffer ms, round trip, distance, ranges, file size) and the *Your session* analysis on a plain snapshot. Tests: the guide checks.
- [x] 7. Singing guide *The microphone*: replace "DawIt doesn't record audio yet: ... Cmd+I)." with "Record on an audio track with its R button; How recording works explains the interface, levels and latency." Check its level advice still says -18 to -10 dB. Done when the build passes.

Group B1, UI: tracks, transport and settings (`./build.sh --test` at step 11):
- [x] 8. Track header and mixer: R on audio tracks, the input chip and menu, Monitor with the one-off warning, the input meter; `TrackActions` items *Input*, *Monitor input*, *About recording audio...*. Check: arm a track in `--demo` (settings copied aside), see the meter move.
- [x] 9. Recording in the lanes: the growing red region with live peaks; after-stop messages (silent take, zero input with the Privacy button opening `x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone`, lost samples); the new "Nothing to record on" text in `TransportShortcuts`.
- [x] 10. Takes in the clip menu (*Takes* submenu, *Delete other takes*, guide last), playing take drawn on top, the take tooltip.
- [x] 11. Audio / MIDI Settings panel: the selector with the input device, input meter, latency line, *Measure latency* (a short progress window with Cancel, then the result or the "couldn't hear the clicks" message), *?* button. Done when the build passes.

Group B2, UI: the guide window (`./build.sh --test` at step 12):
- [x] 12. `RecordingExplainer` with the nine pictures, PDF export, the `gen::guides` entry, cases in `MainComponent::openGuide` and `getPrintGuide`, opened from every place in *The guide*. Done when the build passes and `--export-guides` succeeds.

Group C, check and docs:
- [x] 13. Real-app check: add a `recording` scene to `ScreenshotRunner` (a "Vocals" audio track armed on Input 1 with a 3-take stack built from generated audio, the newest playing; no device input opened). Shots: the arrangement with the stack and the input chip; the clip menu can't be shot, so the Takes tooltip; the guide at every topic; the settings panel. Look for cut text, wrong numbers, empty pictures.
- [x] 14. Docs: README *Features* bullet next to MIDI recording, and remove "audio/MIDI recording" from the *not yet* line; docs/tests.md, docs/code-layout.md; architecture.md (the input path, the recorder, inputs on demand) and recipes.md *Checking in the real app* (the scene); FEATURE_IDEAS status for this idea and a partly-done note on *Recording, the home studio and other areas*.

## Tests
- **Saving:** input choice saved and loaded; default omitted; old project Input 1 mono.
- **Monitoring:** armed, Monitor off: a -12 dBFS 440 Hz input gives silent output; on: the output correlates > 0.99 with the input, and the fader at -6 dB lowers it 6.0 ± 0.1 dB; muted track silent; unarmed track never hears input. Input level atomic reads -12 ± 0.2 dB.
- **Inputs on demand:** inputs wanted false at start, true after arming an audio track, false after disarming; arming a MIDI track doesn't change it.
- **Recording:** with reported input 200 and output 300 samples, a recorded sine take's sample 0 equals input sample 500 (exact within 1e-6 before the file, 2^-23 after reading the WAV back); the clip starts at the capture beat; mono input gives a 1-channel clip, Inputs 1+2 a 2-channel one with each side matching; count-in of one bar records nothing until it ends; a take of silence, and one peaking at -61 dB, leave no clip and no file, one at -59 dB stays; an overlapped old clip ends muted; deleting the track while recording leaves no file; MIDI and audio armed together give both takes; nothing armed and no MIDI track: `startRecording` false.
- **Loop takes:** loop of 2 bars at 120 BPM (4 s), recording from 1 bar before the loop end for 2.5 further loops: 4 clips; passes 2 and 3 start at the loop start and last 4.0 s ± 1 sample; pass 3 unmuted, the others muted; choosing pass 2 unmutes it and mutes 1, 3 and 4; *delete others* leaves one clip.
- **Measure latency:** fake device that returns the output 437 samples later: round trip 437 ± 1; with reported 200 + 200, extra = 37 samples ± 1; no echo (silent input): the routine reports failure; clicks found in 5 of 8 accepted, 4 of 8 failure.
- **Guide:** the test numbers table; every topic has a name, summary, text and example; each *Your session* rule fires at its threshold and stays quiet just past it; search "phantom power", "latency" and "comping" find the guide; `runTheoryGuidesTests` counts the new entry.

## Website hand-off
- Features page: a *Recording audio* section next to MIDI recording: arm, choose the input, record with count-in, takes on a loop, latency correction and Measure latency, and the guide.
- Main page: the *How recording works* guide in the guide picker under *Sound*; worth a line in step 3 *Take it further*.
- Screenshots: the `recording` scene's arrangement (armed Vocals with its input chip and a take stack); the guide at *Buffers and latency* and at *Your session*; the settings panel with the latency line.
- How it's made: nothing new.

## Left for later
- Comping by parts (swipe across takes), take lanes, and punch-in and punch-out ranges.
- Recording through effects (printing them), and more than two input channels on one track.
- Latency correction for MIDI recording, and a low-latency mode that bypasses effects while monitoring.
- Removing unused take files and the unsaved *Recordings* folder.
- The native macOS permission check (AVFoundation), and controlling an interface's direct monitoring.
- The tuner (features_done.html#a-tuner-for-guitar-bass-and-other-instruments) and *Practise singing*, which build on this input path.

## Changes while building

### Group A1 hand-off (engine: input and takes), built and tested

Differences from the plan:
- The track's input choice also lives on `Track` as `inputChannel`, `inputStereo`, `monitor` and `inputMeterLevel` (atomics); the setters are `Engine::setInput (track, firstChannel, stereo)` and `Engine::setMonitor (track, on)`. Both ignore non-audio tracks; `setInput` and an audio track's `setArmed` are ignored while recording. Saved keys are `inputChannel` and `inputStereo`, on audio tracks only, left out for Input 1 mono.
- The level atomic is `Track::inputMeterLevel` (not `inputLevel`: a plugin-slot local of that name shadowed it). It falls back like `updateMeterLevel`; the red "over 0 dB holds 2 s" mark is the UI's to keep.
- Inputs on demand: `Engine::areInputsWanted()`, `setInputMeterWanted (bool)` (for the settings panel). The engine only reopens the device when it is the one that last called `audioDeviceAboutToStart` (so tests with a `FakeDevice` and a bounce never open a microphone). The start-up device state is stripped of its input channels (`audioDeviceInChans`).
- `Engine::isInputAllZero()` (zero-input flag after 1 s armed), `getNumInputChannels()`, `getLatencyShiftSamples()`, `setMeasuredLatencyExtraSamples (int)` / `getMeasuredLatencyExtraSamples()` (step 5 fills it in and stores it per device pair in settings), `setRecordingFolder (File)` / `getRecordingFolder()` (an empty folder means `<settings folder>/Recordings`). `ProjectManager::markSaved` sets the folder to `Audio Files` next to the project file (or empty for an unsaved project), which covers New, Open and Save.
- Recording: `startRecording()` records armed MIDI and audio tracks together; it returns false only when nothing is armed and there is no live MIDI track. `Engine::serviceRecording()` is what the 30 Hz timer does (tests call it between blocks). After stopping, `getLastRecordingReport()` (one `TakeReport` per armed audio track: `numTakes`, `removedAsSilent`, `peakDb`, `lostSeconds`, `error`, `inputChannel`, `stereo`) and `getRecordingReportVersion()` (goes up with each recording) give the UI its messages. `getLiveTakes()` gives what the lane draws while recording (`startBeat`, `seconds`, `peaks` per 512 samples).
- Raw crash-safety file: 32-bit float WAV `<track> raw.wav` in the recording folder, deleted when recording ends. Samples lost at the very end are added to the take as silence too.
- Loop passes: a pass starts at the first captured block and at every block that doesn't continue the last one (a loop wrap, or the playhead moved), so a seek during recording also starts a new take. The newest pass that reached a wrap plays; a lone pass plays.
- Takes helpers are in `src/engine/Clip.h` (`daw::takes::findOverlapping`, `choose`, `findOthers`) with `Engine::chooseTake (Track&, AudioClip&)` and `Engine::deleteOtherTakes (Track&, AudioClip&)` for the clip menu. Stopping is also done by `beginOffline()`, and by the device stopping or changing its rate.
- `MICROPHONE_PERMISSION_ENABLED` and its text are set in `juce_add_gui_app`.
- Tests: `runAudioRecordingTests` in `tests/EngineTests.cpp` with a `RecordingRig` helper; `FakeDevice` has `numInputs`, `inputLatency` and `outputLatency`.
- Not done here (later groups): the UI, the lanes, the messages, step 5 onward.

### Group A2 hand-off (engine: latency and guide content), built and tested

- Measuring: `src/engine/LatencyMeasure.{h,cpp}` (`daw::latency`: `makeClick`, `getClickStart`, `getTotalSamples`, pure `findRoundTrip (heard, rate)` returning `Result { ok, clicksFound, roundTripSamples }`; at least 5 of 8 clicks within 2 ms of each other are needed; windows quieter than -54 dB or a fifth of the loudest are ignored). Engine API: `startLatencyMeasurement()` (false without inputs, while recording or bouncing, or already measuring; the settings panel must have its input meter on first so the device has inputs), `isMeasuringLatency()`, `getLatencyMeasurementProgress()` (0 to 1, about 2.3 s), `cancelLatencyMeasurement()`, and `serviceLatencyMeasurement()` which the UI calls from a timer: it returns true once when the result is ready, then `getLastLatencyMeasurement()` gives `LatencyMeasurement { ok, clicksFound, roundTripSamples, reportedSamples, extraSamples, sampleRate, problem }` (`problem` holds the "couldn't hear the clicks" text). While measuring the callback plays only the clicks. A good result sets the extra, `hasMeasuredLatency()`, and stores it in settings as `recording.extraLatencyMs.<input device>|<output device>` (`getLatencySettingsKey()`, ms); the engine reloads it when the device changes and at start-up (`loadMeasuredLatency`).
- The settings key is empty without a device, so the storing is not covered by a test; the clicks are on all output channels and the loudest input channel is listened to.
- Guide content: `src/engine/RecordingGuide.{h,cpp}`, namespace `daw::recordingguide`: `Topic`/`allTopics`, `getTopicName/Summary`, `explain`, the formulas, picture data (`getBufferTable`, `getSignalLevels`, `getLevelExamples` + `getPhrase`, `getTakesExample`, `exampleLoopStartBar/EndBar`), `Snapshot`/`ArmedTrack`, `describeSession`, `describeLatency` (the settings panel's line), `describeInput`, `getRoundTripMs`, `getZeroInputMessage`, `analyse (Snapshot)` returning `Advice { text, topic }` in the plan's rule order. The group-B code fills the `Snapshot` from the engine (device names from `deviceManager`, latencies from the device's reported samples / rate, `getMeasuredLatencyExtraSamples`, `getLastRecordingReport`, `isInputAllZero`, `Track::inputMeterLevel`).
- The plan gave no general text for *Your session*; `explain (Topic::yourSession)` is a short intro I wrote (UI-style text, to check). The search words, the `gen::guides` entry, and the "search finds the guide / theory guides count" tests belong to step 12 and are not done.
- Singing guide *The microphone* now says "Record on an audio track with its R button; How recording works explains the interface, levels and latency."; its level advice already said -18 to -10 dB.
- Tests: `runLatencyMeasureTests` and `runRecordingGuideTests` in `tests/EngineTests.cpp` (`measureWithEcho` helper on `RecordingRig`).

### Group B1 hand-off (UI: tracks, transport and settings), built and tested

- Shared UI helpers are in `src/ui/RecordingActions.{h,cpp}` (`daw::recordingui`): `getInputChipText`, `makeInputMenu` / `applyInputChoice` / `showInputMenu`, `setMonitor (engine, track, on, showGuide)` (the one-off warning, `recording.monitorWarned`; its second button "About hearing yourself..." calls `showGuide (Topic::monitoring)`), `makeSnapshot (Engine&)` (fills `recordingguide::Snapshot` from the device and engine: use it for the guide's *Your session* and its live meters), `InputMeter` + `drawInputMeter` (zones, marks at -6 and 0 dB, red hold 2 s), `showReportMessages` (after-stop messages, polled by `ArrangementView::timerCallback`).
- The guide window is not wired yet. These callbacks exist and `MainComponent` must set them in group B2: `ArrangementView::onShowRecordingGuide (recordingguide::Topic)` (track menu *About recording audio...* opens `Topic::air`, a clip's *About recording audio...* opens `Topic::takes`, the Monitor warning opens `Topic::monitoring`) and `AudioSettingsPanel::onShowGuide` (the ? button; should open `Topic::buffers`; `MainComponent::showAudioSettings` builds the panel and doesn't set it yet, so the ? is greyed out). The track menu's item is greyed until `onShowRecordingGuide` is set; the clip menu item is always on and does nothing until it is.
- Track header: R on audio tracks (right-click R opens the input menu); armed, the bottom-left shows the input chip ("In 1", "In 1+2") instead of the AUDIO text, a *Mon* button sits before R, and the header meter shows the input. The mixer strip shows "REC In 1" and the input meter, with a tooltip. R and the chip are disabled while recording. Track menu: *Input* submenu, *Monitor input*, and *About recording audio...* last.
- Lanes: `TrackLanes::updateRecording()` (30 Hz, from the arrangement timer) and `drawLiveTakes` draw the red growing take with live peaks and the name "<track> take <n>"; muted overlapped takes are drawn under the playing one and a click reaches the playing one; the take tooltip is as in the plan; the clip menu has *Takes* (ticked = playing), *Delete other takes* and *About recording audio...*.
- Settings: `AudioSettingsPanel` (`src/ui/AudioSettingsPanel.{h,cpp}`) replaces the bare selector (now with up to 8 inputs), with the input meter, the latency line (`describeLatency`), *Measure latency* (progress window with Cancel; failure shows `LatencyMeasurement::problem`) and ?. New engine call for its meter: `Engine::getDeviceInputLevel()` (loudest input channel, falling; updated in `meterInputs`).
- Differences: the Monitor button says "Mon" (no headphone glyph). The zero-input message is shown after stopping (when `isInputAllZero()` is still set), with an *Open Privacy settings* button on macOS, in place of the other messages. The record button tooltip and the "Nothing to record on" text were updated (`TransportShortcuts::toggleRecording`, `TransportBar.h`).
- Not checked in a real app with a microphone (the scene is group C's). The panel's device selector may itself open input channels when an input device is chosen; the engine closes them again when the panel closes and nothing is armed.

### Group B2 hand-off (UI: the guide window), built and tested

- `src/ui/RecordingExplainer.{h,cpp}`: the window (1000 x 740, picture 330 high), nine pictures in one `Picture` class, a 15 Hz timer that refreshes the picture from `recordingui::makeSnapshot` and, for *Your session*, the text (`explain` + `describeSession` + every `analyse` line with its topic name in brackets, or "Nothing needs changing right now."). `MainComponent::showRecordingGuide (Topic)` opens it; `ArrangementView::onShowRecordingGuide` and `AudioSettingsPanel::onShowGuide` (-> *Buffers and latency*) are set; `openGuide` and `getPrintGuide` have the `G::recording` cases. PDF export works (`--export-guides`: 9 topics, 10 pages).
- `gen::guides::Guide::recording` is last in `allGuides` (area Sound, not on the start path). Search words are the plan's plus "power" (so "phantom power" matches: the search looks at words, not phrases). Tests added to `runTheoryGuidesTests`.
- Screenshot runner: `"guide": "recording"` works with topic names. I looked at three topics (air, interface, takes) in a window shot; the other six pictures and *Your session* with an armed track are for group C's shots.
- Without a device the pictures use 128 samples at 48 kHz; *Choosing an input* shows two example sockets and tracks, labelled as an example.

### Group C hand-off (real-app check and docs), built and tested

- Scenes added to `ScreenshotRunner`: `recording` (an audio track "Voice", not "Vocals" because the pop song already has a bus of that name, armed on Input 1 with three stacked takes of 4 s at bar 9, the newest playing) and `audioSettings` (opens the settings panel). A scene runs after the shot's `select`/`fold`/`panel`, so it gets a shot of its own and the next shot selects "Voice".
- Looked at: the arrangement row, the settings panel, every guide topic's picture (air, interface and takes in B2; choose input, level, buffers, hearing yourself, moved, *Your session* with Voice armed here). Fixed two things: the playing take of a stack was drawn see-through over the muted ones, so their names showed through (now filled with the lane colour first, `TrackLanes::paint`); the five-box *Monitor in DawIt* path in *Hearing yourself* overlapped (boxes narrower, `RecordingExplainer::paintMonitoring`).
- Docs: README bullet and the limitations line, `docs/tests.md`, `docs/code-layout.md`, `docs/architecture.md` (the recording paragraph), `docs/recipes.md` (the scenes), FEATURE_IDEAS statuses (this idea and *Recording, the home studio and other areas*, both Partly done).
- Could not be verified without a real microphone: the macOS permission prompt and the zero-input message with the Privacy button, a real interface's reported latency against *Measure latency*, live input levels and the red growing take while recording, Monitor through headphones, and a stereo-pair input. The engine paths are covered by tests with a fake device.
- Website hand-off: the plan's, and shots: the arrangement row (Voice, `In 1`, take stack, from the `recording` scene with `"zoom":40,"viewStart":28`), the guide at *Buffers and latency* and *Your session*, the settings panel with its latency line. Nothing new for How it's made.

### Theory check
- `RecordingGuide.cpp` *Setting the level*: the vocal compressor example said a threshold of -18 dB; the compressor's Lead vocal starting point uses -20 dB, so the text now names it with -20 dB.
- *Buffers and latency*: "a study for Shure (Lester and Boley, 2007)" became "a 2007 study (Lester and Boley, AES paper 7198)", since only Boley was at Shure (Lester was at Purdue).
- *The audio interface*: "a normal balanced cable keeps it away from them" became "on a normal balanced cable it does them no harm" (phantom reaches a dynamic mic but, on both wires equally, passes no current through it; F15).
- *Takes and loop recording* and the takes picture caption: "the newest complete pass plays" became "the newest pass that reached the end of the loop plays" (the engine's rule; a pass started mid-loop counts), and a cut-short pass is "muted, unless it's the only one".
- *Your session* intro (written while building): "how your last take was" became "how loud your last take was". Other numbers, advice rules, thresholds, tests and picture data match *Theory*.
