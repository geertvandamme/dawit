# Realistic instruments: SoundFont and SFZ player

Status: Released
Idea: [#58 — Realistic instruments: SoundFont and SFZ player](https://github.com/geertvandamme/dawit/issues/58)

## Goal
A second built-in instrument, **DawIt Sampler**, plays recorded instruments from SoundFont files (.sf2, .sf3): a real grand piano, nylon guitar, strings or drum kit instead of the DawIt Synth's synthetic sounds. A free General MIDI set (MuseScore General) is downloaded once after asking, and users can add their own SoundFonts. Its "What is a sampler?" guide explains root notes, key zones, velocity layers, loops, SoundFonts and General MIDI, so a beginner learns why a sampled sound sounds real and how to program it so it stays convincing.

## Decisions for you
1. **Player library.** TinySoundFont v0.9 (MIT, one header) with stb_vorbis for .sf3, or FluidSynth (LGPL-2.1+, combinable with AGPL; reads modulators and reverb/chorus amounts, but a larger build, GLib until about 2.5). **Recommended:** TinySoundFont: small and audio-thread friendly. Catch: it ignores modulators (MuseScore General uses 2.01 modulators), so the mod wheel does nothing and velocity changes brightness only through layers; the guide says so.
   Answer: TinySoundFont v0.9 with stb_vorbis (user, 2026-11-08).
2. **SFZ now or later.** sfizz (BSD-2, several bundled dependencies) now, or SoundFonts only. **Recommended:** later, own issue; *Add SoundFont...* refuses .sfz with one sentence.
   Answer: Later, in its own issue; Add SoundFont... refuses .sfz with one sentence (user).
3. **Download flow.** Ask when a sound from a set that isn't downloaded is picked (every time, nothing nags otherwise), or a first-run offer. **Recommended:** the first; "Not now" keeps the DawIt Synth and the menu shows *(not downloaded)*.
   Answer: Ask when a sound from a set that isn't downloaded is picked; "Not now" keeps the DawIt Synth (user accepted recommendation).
4. **Copy or link your own SoundFonts.** Always copy, always link, or ask. **Recommended:** ask, *Copy* the default: a copy survives moving the original; a link saves space for big files (paths kept in app settings).
   Answer: Ask, with Copy as the default (user accepted recommendation).
5. **Name, and #82** (a sampler for your own recordings). **Recommended:** *DawIt Sampler*; #82 later adds "load a recording" to the same instrument.
   Answer: DawIt Sampler; #82 later adds loading a recording to the same instrument (user accepted recommendation).
6. **VST3/AU build.** **Recommended:** `APP_ONLY` for now: the shared library and download are app services.
   Answer: APP_ONLY for now (user accepted recommendation).
7. **.sf3 decoder.** stb_vorbis (MIT or public domain, supported by TinySoundFont, unchanged since 2021, past fuzzing CVEs fixed) or a shim on JUCE's libvorbis, already shipped. **Recommended:** stb_vorbis, but only on files that pass DawIt's structure check, on a background thread.
   Answer: stb_vorbis, only on files that pass DawIt's structure check, on a background thread (user accepted recommendation).
8. **Generator picks presets** (bass: fingered bass, chords: piano). **Recommended:** later (templates and New from Style could offer it); generating notes never changes an instrument.
   Answer: Later; generating notes never changes an instrument (user accepted recommendation).

## What the user does and sees
- **Choosing a sound.** The *Add/Replace instrument* menus (track header, `PluginActions` add menu, device bar) get **DawIt Sampler** under *Built-in*, with a submenu **Sampled sounds**:
  - first MuseScore General, then each added SoundFont by file name;
  - in a font with more than 16 presets in bank 0, bank 0 is grouped by the 16 General MIDI families (*Piano*, *Bass*, ...), other banks go under *More banks*, and bank 128 under *Drum kits*;
  - then *Add SoundFont...*, and last the guide (`guideicon::addItem`, "What is a sampler?...").
  Picking a preset on a track that plays the DawIt Synth adds DawIt Sampler set to it. On a track that already has DawIt Sampler, it only changes the preset. A set that isn't downloaded shows *MuseScore General (not downloaded)*; picking from it asks first.
- **DawIt Synth window:** a button **Use a sampled sound** switches the track to the matching preset (mapping in *Theory*). Choosing *DawIt Synth* in the instrument menu switches back.
- **Download question:** "Download MuseScore General? DawIt Sampler's free sound set: a full General MIDI set of 128 sounds and drum kits, about 38 MB, MIT licence. It is downloaded once to DawIt's SoundFonts folder. Until then the track plays the DawIt Synth." [Download] [Not now]. Progress window with cancel (`ModelDownload` pattern).
- **Add SoundFont...:** file chooser (.sf2, .sf3; .sfz refused with "DawIt Sampler plays SoundFonts (.sf2, .sf3); SFZ libraries aren't supported yet."). The file is checked first, then a choice: *Copy into DawIt's SoundFonts folder* (default) or *Use it where it is*. A failed check names the problem ("not a SoundFont", "the file is cut short").
- **The DawIt Sampler window** (`fxui::EffectEditor`, 1400 × 780): header with starting points, *Save...*, the guide button and *Test note*; left, the SoundFont box over a family list and a preset list ("001 Acoustic Grand Piano"); middle, the key-zone picture (keyboard 0-127, a coloured band per zone, a dot on each root) over the velocity layers of the last key and the level history; knobs Level, Transpose, Fine tune, Bend range; the explainer on the right. A banner over the zones when the file is missing ("... isn't on this computer: the DawIt Synth's Bass plays instead", [Add SoundFont...] or [Download]) or loading.
- **Interaction.** Click a preset to choose it; Up/Down step through presets; click a key in the zone picture to hear it; hover shows its zone ("E2 · zone C2-F#2 · root D2 · velocity 1-127"). The right-click menu on the zone picture has *Copy preset name* and ends with the guide.
- **Elsewhere:** the device bar chip reads "DawIt Sampler - Acoustic Grand Piano" and shows a tiny zone strip (`chainpictures`). It reads "(missing)" or "(loading)" when that applies. The inspector and track header show the same name.
- **Tooltips:** in *Theory*.
- **Settings:** `soundfonts.linked` (linked paths). The SoundFonts folder sits next to Models (`~/Library/Application Support/DawIt/SoundFonts`), with an override for tests and `ScreenshotRunner`.

## Data and saving
- No project format change. The sampler's choice lives in its plugin state, saved like every built-in: the SoundFont reference (file name, size and, for a linked file, its full path), bank, program (0-127), and the four parameters. Default values are left out.
- **Finding the file on load:** look in the SoundFonts folder by name and size, then at the linked path. When it isn't found, the sampler keeps the reference, plays the fallback sound, and `project::load` adds a warning: "Piano plays the DawIt Synth: the SoundFont "X.sf2" isn't on this computer. Add it with Add SoundFont... and the sampled sound comes back."
- Older projects are unaffected. An older DawIt opening a project with DawIt Sampler reports it as a missing plugin, as for any unknown plugin.
- Test: `checkThroughProject<BuiltInSampler>` keeps file name, bank 128, program 0 and Transpose +5. A project whose font is missing loads with the reference intact and one warning.

## The guide
"What is a sampler?", an `fxui::Guide` (built-in recipe step 4), guide key `sampler`. Pages, in order, with the text and what each picture shows in *Theory* (each page's last paragraph): 1 A sampler plays recordings, 2 Pitch from playback speed (+/- interval buttons, click plays both), 3 Key zones, 4 Velocity layers, 5 Loops and the envelope, 6 Inside a SoundFont, 7 General MIDI, 8 Memory and loading, 9 Sampled or synthesised? (two play buttons), 10 Your track (the track's notes against the preset's zones, out-of-range notes in `meterRed`), 11 Sources. Pictures 1 and 3 to 8 follow the sampler's current font, preset and last note.

Theory Guides entry: area `synthesis`; title "What is a sampler?"; summary "Real instruments from recordings: root notes, key zones, velocity layers, loops, SoundFonts and General MIDI."; opens from "DawIt Sampler's window (What is a sampler?), the Sampled sounds menu"; plugin "DawIt Sampler"; words "sampler sample sampled soundfont sf2 sf3 sfz multisample zone velocity layer loop round robin general midi gm program bank drum kit musescore realistic piano strings". Not on *Start here*. Also fix `SpectralFreezeGuide`'s "DawIt has no sampler yet" sentence (Group B2).

## Code
Follows recipes.md *A built-in effect or instrument* (copy DawIt Subtractive), *A long job* (the download and font loading, `StemActions`'s download part), *A command or menu item* (track menus in `TrackActions`/`PluginActions`).
- **`soundfonts::` (engine, juce_core plus TinySoundFont; no `Engine.h`):** the folder, a structure check, a header-only preset list (read only the `phdr` records, so menus never decode samples), the zone map of a preset (key and velocity ranges, root, loop, envelope: for pictures, tests and *Your track*), and a **Library**: one per process, loading each file once on its own background thread and sharing it. Each sampler holds a share; the font is freed on the message thread when the last share goes. Copying, linking and the linked list also live here.
- **`BuiltInSampler`:** a JUCE instrument processor that owns its own TinySoundFont copy (`tsf_copy` of the shared font: own voices, shared samples). It also owns a `BuiltInSynth` with its own `std::atomic<BuiltInSound>`, the **fallback**, which plays while the font is missing or loading. The fallback sound comes from the program's General MIDI family (table in *Theory*).
- **Threading:** every TinySoundFont call for a sampler's copy is made on the audio thread inside `processBlock`, except while it is being built. The message thread builds a copy (under the Library's mutex: `tsf_copy` isn't thread-safe), calls `tsf_set_max_voices` (64) and creates channel 0, so the audio thread never allocates. It hands the copy over through a single atomic slot. The audio thread swaps it in at a block start and puts the old one in a retire slot. The message thread's timer closes retired copies; the audio thread never frees. Bank, program, transpose, tune and bend range are atomics, applied at block start. MIDI is split at event times for sample-accurate notes. CC 7, 10, 11, 64, 121 and RPN 0 go through `tsf_channel_midi_control`; pitch bend through `tsf_channel_set_pitchwheel`.
- **Invariants:** program is stored 0-127 and shown 1-128. A missing or loading font never means silence while the fallback exists. A bounce waits (in its progress window) until every sampler's font has loaded, and lists tracks playing a fallback before it starts. Generating notes never changes the instrument.
- **Risks:** untrusted user files reach a C parser and stb_vorbis. The structure check runs first (chunk sizes inside the file, every index and sample offset in range), and loading runs on a background thread. Memory: TinySoundFont stores floats, so a font takes about twice its .sf2 size (about 400 MB for MuseScore General). The audio-thread handover is new: test it.

## Libraries and models
| Component | Version / pin | Licence | Use |
|---|---|---|---|
| TinySoundFont | v0.9, commit `853a0a171759f1ddba0de1442133a75912bbeffa`; FetchContent tarball with `URL_HASH SHA256` (compute when adding) | MIT, © 2017-2025 Bernhard Schelling (LICENSE file says 2017-2023), based on SFZero © 2012 Steve Folta (MIT) | compiled in; `tml.h` not used |
| stb_vorbis | v1.22 (`stb_vorbis.c` last changed in commit `1ee679ca2ef753a528db5ba6801e1067b40481b8`); pin the stb repo tarball at that commit with SHA-256 | MIT or public domain (Unlicense), at our choice: use MIT, © 2017 Sean Barrett | compiled in, .sf3 only |
| MuseScore General | 0.2.0 (`VERSION`), `MuseScore_General.sf3` (38 MB) and `MuseScore_General_License.md` from `https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/`; size and SHA-256 of both recorded when adding | MIT, with copyright notices for Frank Wen (FluidR3), Michael Cowgill (FluidR3Mono), S. Christian Collins, Ethan Winer (Temple Blocks) and Michael Schorsch (cymbals) that must go with derivative works; parts public domain or CC0 (VCSL) per its sample-sources CSV | downloaded after asking, never shipped or built in |

All run on the user's machine. **Obligations** (Group A steps 1 and 4):
- the root `LICENSE` inventory gets TinySoundFont and stb_vorbis (compiled in) and MuseScore General (separately downloaded, like the Whisper weights);
- `licenses/` gets `TinySoundFont-LICENSE.txt` (with the SFZero line), `stb_vorbis-LICENSE.txt` (the MIT alternative, verbatim) and `MuseScore-General-LICENSE.md` (verbatim), with provenance in `licenses/README.md`;
- the downloaded `MuseScore_General_License.md` is saved beside the .sf3 and shown from the sampler window (*About this sound set...*);
- the README *License note* gets a paragraph, release packaging ships the two code notices (`DISTRIBUTION_TODO.md` if packaging isn't ready), and `about.html` goes to the website hand-off.

**Unresolved:** the CSV gives no licence of their own for Ethan Winer's and Amir Oosman's samples. We rely on the set's overall MIT statement (low risk, since DawIt doesn't redistribute the file). User-added SoundFonts carry their own terms; the Add dialog says so.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| F1 | A sampler records and plays back recordings, pitch-shifted to play scales; samples are mapped to keyzones | Wikipedia *Sampler* | exact |
| F2 | Mellotron 1963 (tape); Fairlight CMI 1979, first commercial polyphonic digital sampler, 8-bit, ~US$25,000; E-mu Emulator 1981, 128 kB | Wikipedia *Sampler* | exact |
| F3 | n semitones = speed × 2^(n/12): +1 → 1.0595, +7 → 1.4983 (length 0.667), +12 → 2.000 (length 0.5), −12 → 0.5 | derived | exact |
| F4 | Resampling speeds up envelope and vibrato and moves formants (chipmunk up, giant down) | SOS *Lost art of sampling 6*; Steinberg forum | exact (qualitative) |
| F5 | Keygroups of 3-4 semitones common; a sample often covers its root ± about 5 semitones | SOS; SynthFont tutorial | typical |
| F6 | Velocity layers capture timbre change; more layers cost memory | SOS | typical |
| F7 | Detailed piano libraries can have 8 or more layers | KVR forum (an 88-key, 8-layer example) | typical |
| F8 | Round robin rotates recordings to avoid the machine-gun effect; SFZ has `seq_length` and `lorand`/`hirand`; SoundFonts have no round robin | sfzformat.com; TinySoundFont generator list | exact |
| F9 | SoundFont samples can play once, loop continuously, or loop while held and then play to the end | TinySoundFont header (continuous and sustain loops) | exact |
| F10 | SoundFont: E-mu/Creative, AWE32 1994; 2.0 public 1996 (.sf2); 2.01 1998; 2.04 2005, 24-bit | Wikipedia *SoundFont* | exact |
| F11 | Presets in banks of up to 128; bank 128 is percussion; preset → instruments → zones with key and velocity ranges → mono samples; stereo is an L and R pair; samples are shared | SynthFont tutorial; midi.org forum | exact |
| F12 | .sf3: unofficial, by Werner Schweer for MuseScore, Ogg Vorbis (lossy) compression | Wikipedia *SoundFont* | exact |
| F13 | MuseScore General 0.2.0: .sf2 206 MB, .sf3 38 MB (38/206 = 0.18, "about a fifth"); MIT; uses 2.01 modulators | osuosl listing, licence, readme | exact |
| F14 | Lossy .sf3 compression is usually hard to hear in a mix | (practice) | opinion |
| F15 | SFZ: plain text with regions, groups and global settings, pointing to separate sample files | sfzformat.com | exact |
| F16 | GM 1991: 128 programs, 16 families of 8 (names and ranges in topic 7), channel 10 drums, 47 drum sounds on notes 35-81, 24 voices | Wikipedia *General MIDI* | exact |
| F17 | Program 1 Acoustic Grand Piano, 25 Nylon guitar, 34 Electric Bass (finger), 49 String Ensemble 1, 17 Drawbar Organ, 81 Lead 1 (square), 90 Pad 2 (warm); kick 36, snare 38, closed hat 42, open hat 46, crash 49, ride 51 | Wikipedia *General MIDI*; agrees with the MIDI guide (1 piano, 25 nylon; 36 kick, 38 snare) | exact |
| F18 | Programs shown 1-128, sent as 0-127 | Wikipedia *General MIDI* | exact |
| F19 | GM bend range starts at 2 semitones | MIDI guide's source (midi.org defaults) | exact |
| F20 | 16-bit 44.1 kHz mono: 44,100 × 2 × 60 = 5,292,000 B = 5.3 MB/min; stereo 10.6 MB | derived | exact |
| F21 | DawIt Sampler holds samples as 32-bit floats → about 2 × .sf2 size: MuseScore General about 400 MB (2 × 206 = 412) | TinySoundFont header; derived | typical ("about") |
| F22 | TinySoundFont ignores modulators and the reverb and chorus send amounts | TinySoundFont header | exact |
| F23 | Choose sampled for realism and synth for shaping; mixing both is common | (practice) | opinion |

No disagreement with existing guides. The MIDI guide's GM facts match. `SpectralFreezeGuide` says "DawIt has no sampler yet": reworded in step 8. The issue's "about 200 MB in memory" becomes about 400 MB here (F21).

### Page texts (paste as they are)

**1. A sampler plays recordings** · *What a sampler is, and what it's for*
A sampler is an instrument that plays recordings. Someone recorded a real piano, note by note and at several strengths. When you press middle C, the sampler plays the recording of middle C. That's why a sampled piano sounds like a piano: it is one, recorded.

A synthesiser such as the DawIt Synth or DawIt Subtractive builds its sound from scratch with oscillators and filters. That is light and flexible, but an acoustic instrument's hammer noise, ringing strings and breath are very hard to build that way. A sampler sidesteps the problem by playing back the real thing.

Samplers began with tape: the Mellotron (1963) had a strip of tape behind each key. The first polyphonic digital sampler on sale was the Fairlight CMI (1979), at 8 bits and about US$25,000; the E-mu Emulator (1981) had 128 kB of memory. Today a sampled instrument is a set of recordings plus a map that says which recording plays for which key, and for how hard you play.

DawIt Sampler plays such sets in the SoundFont format (.sf2 and .sf3). Use it for pianos, guitars, strings, brass and drum kits when a part should sound like real players, for example before you bounce it to a WAV.

What it doesn't do yet: load a single recording of your own and play it across the keys, or play SFZ libraries.

The picture shows the recording that played for the last note, with its name.

**2. Pitch from playback speed** · *Root notes, and why higher means shorter*
Every recording in a sampled instrument has a root note: the note it was recorded at. To play another note, the sampler plays the recording faster or slower, the way a record sounds higher when it spins faster. One semitone up is 1.0595 times faster (2 to the power 1/12). Twelve semitones, an octave, is exactly twice as fast: A3 at 220 Hz becomes A4 at 440 Hz, and the recording lasts half as long.

That's the catch: speed changes more than pitch. A recording of A3 played as E4, 7 semitones up, runs 1.498 times faster. Its attack, its fade and any vibrato all go 1.5 times faster, and it is over in two thirds of the time. The body resonances that give an instrument or a voice its character, its formants, move up with it. A voice pushed up an octave sounds like a chipmunk, and pushed down, like a giant. A step or two is hard to hear; big steps sound unnatural.

Some samplers can change the pitch without the speed (time-stretching). A SoundFont player such as DawIt Sampler changes the speed, as samplers have done from the start.

The picture shows one recording at its root, and the same recording played higher: the same wave, squeezed. Each click on Hear both plays the note and then the same note moved by the interval shown, and the next click moves on to the next interval, so you can see and hear how much faster and shorter it gets.

**3. Key zones** · *Multisampling: a recording every few notes*
Because a recording sounds less natural the further it's moved, good sampled instruments are multisampled: recorded every few notes, each recording covering a small range of keys, its key zone (also called a keygroup or split). A recording every 3 or 4 semitones is common, so no key is more than a semitone or two from its root; a zone often reaches up to about 5 semitones either side of its root. Some piano libraries record every key.

When you play a key, the sampler finds the zone it falls in and plays that zone's recording, sped up or slowed down by the distance to its root. Where two zones meet, the change of recording can be heard as a small jump in tone. Good libraries hide it with careful recording; some overlap neighbouring zones and crossfade them.

A key outside every zone plays nothing. A violin preset may stop at G3, the violin's lowest note, so a bass line played on it stays silent below that.

The picture is the keyboard with the zones of your preset, one colour each, a dot on each root. The last key you played is lit, with the zone it used.

**4. Velocity layers** · *Why a hard hit sounds brighter, not only louder*
Play a piano key softly and the note is dark and round; play it hard and it is bright, with more hammer. That isn't only a change of level: the timbre changes. A sampler captures this with velocity layers, recordings of the same note at different strengths, each played over a range of MIDI velocities (1 to 127). A preset might use one recording for velocities 1 to 63 and another for 64 to 127; detailed piano libraries can have 8 layers or more, at the cost of memory.

So in a layered preset, velocity picks the recording as well as the level. A part drawn with every note at velocity 100 always plays the same layer, one reason programmed parts can sound mechanical. Vary the velocities and the soft notes really sound soft.

Many presets, including many in free SoundFonts, have only one layer. Velocity then changes only the level.

Some libraries go further with round robin: several recordings of the same note at the same strength, played in turn, so a fast repeated snare doesn't sound like a machine gun. SoundFonts have no round robin; many SFZ libraries do.

The picture shows the layers under the last key you played: velocity 1 at the bottom, 127 at the top, each layer a band, and a line at the last note's velocity.

**5. Loops and the envelope** · *How a short recording holds a long note*
An organ note lasts as long as you hold the key, but a recording has an end. Samplers solve this with a loop: two points in the recording, the loop start and end, between which the sampler plays round and round while the note sounds. Placed where the wave matches, a loop can't be heard, and 2 seconds of recording can hold a note for a minute.

In a SoundFont each zone says what its recording does: play once to its end (a piano or a drum, which fade by themselves); loop for as long as the note sounds; or loop while the key is held and play on to the end when you let go, so the natural release is heard.

On top of the recording sits a volume envelope: the same attack, decay, sustain and release as on the synths (see What is sound?). It fades a looped note out after you let go, so it ends smoothly instead of stopping dead. Each zone has its own envelope, so a piano's low notes can ring longer than its high ones.

The picture shows the recording for the last key, with its loop marked if it has one and the play position running through it. Under it is the zone's envelope, with its times in seconds.

**6. Inside a SoundFont** · *Presets, instruments, samples and banks*
A SoundFont is one file that holds the recordings and the map. E-mu and Creative made the format for the Sound Blaster AWE32 sound card (1994). SoundFont 2.0 (1996) was published for anyone to use, and its files end in .sf2. Version 2.04 (2005) added 24-bit samples.

Inside, from the top down:
- A preset is what you choose, such as Acoustic Grand Piano. Presets are numbered within banks of up to 128, and bank 128 is kept for drum kits.
- A preset is made of one or more instruments, and an instrument of zones. Each zone has a key range, a velocity range, a recording, and settings such as tuning, loop, envelope and filter.
- The samples are the recordings themselves, always mono: a stereo sound is a left and a right sample played together. One sample can serve many zones and presets.

An .sf3 is the same thing with its recordings compressed with Ogg Vorbis, the way an MP3 is compressed. MuseScore made it, and the files are about a fifth of the size: MuseScore General is 38 MB as .sf3 and 206 MB as .sf2. The compression throws a little detail away, usually too little to matter in a mix. DawIt decodes an .sf3 when it loads it.

SFZ is another format: a plain text file that lists the zones and points to separate audio files (WAV or FLAC). Many free libraries come as SFZ. DawIt Sampler doesn't play SFZ yet.

DawIt Sampler reads the zones, loops, envelopes and filter. It doesn't read a SoundFont's modulators, so the mod wheel does nothing in it, or its reverb and chorus amounts: add DawIt Reverb for space.

The picture shows the file you chose, top to bottom: its presets, instruments and samples, and how many of each.

**7. General MIDI** · *Why program 1 is a piano everywhere*
General MIDI (1991) fixed a numbered list of 128 sounds, so a MIDI file plays with roughly the right instruments on any device. They come in 16 families of 8: Piano 1-8, Chromatic Percussion 9-16, Organ 17-24, Guitar 25-32, Bass 33-40, Strings 41-48, Ensemble 49-56, Brass 57-64, Reed 65-72, Pipe 73-80, Synth Lead 81-88, Synth Pad 89-96, Synth Effects 97-104, Ethnic 105-112, Percussive 113-120 and Sound Effects 121-128. So 1 is the Acoustic Grand Piano, 25 the nylon guitar, 34 the fingered electric bass and 49 the string ensemble.

The numbers are shown from 1 to 128, as here and in most software. Inside a MIDI message they run from 0 to 127, so program 1 is sent as 0. Some SoundFont editors show 0 to 127: the same sounds, counted from 0.

Drums have a map of their own: each note is a drum, not a pitch. General MIDI puts drums on channel 10 and names 47 sounds on notes 35 to 81: 36 is the kick, 38 the snare, 42 the closed hi-hat, 46 the open hi-hat, 49 a crash and 51 a ride. In a SoundFont the drum kits live in bank 128. DawIt's drum patterns write this map, so they play on a sampled kit unchanged.

General MIDI fixes the names, not the sound. Program 1 is a piano on every device, but each SoundFont's piano is its own recording.

The picture shows the 128 sounds as 16 rows of 8, with your preset lit; for a drum kit, the drum map with each note's name.

**8. Memory and loading** · *Why a sampled sound takes a moment and a lot of memory*
Recordings are big. One minute of mono CD-quality audio (16 bits, 44,100 samples a second) is 44,100 × 2 bytes × 60 = 5.3 MB; in stereo, 10.6 MB. A piano recorded on every key, at several strengths, each note ringing for seconds, adds up fast: big commercial piano libraries fill many gigabytes.

A SoundFont is loaded into memory whole, so its notes start instantly. DawIt Sampler keeps each sample as a 32-bit number, twice the 16 bits in the file, so a SoundFont takes about twice its .sf2 size in memory: MuseScore General, 206 MB as an .sf2, takes about 400 MB. An .sf3 is decoded to full size first, which takes a moment; until it is ready the track plays the DawIt Synth.

A SoundFont is loaded once, however many tracks use it: ten tracks on MuseScore General presets cost the memory of one. It is freed when no track uses it any more.

The DawIt Synth needs no recordings, which is why it starts instantly and stays the default.

The picture compares, for the SoundFont you chose, the file on disk, the decoded samples and the memory used.

**9. Sampled or synthesised?** · *Which to pick for a part*
Neither is better; they are good at different things, and engineers choose differently.

A sampled sound suits a part that should sound like real players: a piano ballad, a nylon guitar, a string section, a drum kit for a rock or pop song. It brings detail a synth can't fake. It costs memory and a moment to load, and it plays as recorded: you choose a preset more than you shape it.

A synth suits a sound you want to shape, a sound that is electronic anyway (basses, leads and pads in dance music), or a part you want light and instant. DawIt Subtractive, Additive, FM, Drums and Physical let you turn every knob.

A sampled part still has to be played like the real instrument to convince: notes in the instrument's range, varied velocities, a sustain pedal for a piano, chords a guitarist could finger. A string section playing a fast bass line won't sound like strings, however good the recordings.

Many songs mix both: a sampled piano and drums with a synth bass and pad. Try a part on each: Use a sampled sound in the DawIt Synth window switches a track to the matching sampled preset, and choosing DawIt Synth in the instrument menu switches back.

The button plays a short tune of four notes through your track's own sound, so you can hear it on the DawIt Synth and then on a sampled sound.

**10. Your track** · *What this track's sampler is doing*: text from the rules below.

### Tooltips
- SoundFont box: "Which SoundFont this track plays. MuseScore General is DawIt's free General MIDI set; add your own .sf2 or .sf3 with Add SoundFont..."
- Preset list: "The sounds in this SoundFont, by bank and program (programs shown 1 to 128, as in General MIDI). Click to choose; Up and Down step through them."
- Level: "How loud the sampler plays, from -24 to +6 dB. Presets differ in loudness: even them out here, then mix with the fader."
- Transpose: "Moves every note by -24 to +24 semitones before the zone is chosen, so other recordings play, not just other pitches."
- Fine tune: "Detunes by -100 to +100 cents (100 cents is a semitone), to match a recording or instrument slightly off A = 440 Hz."
- Bend range: "How far the pitch wheel bends, 1 to 24 semitones each way. General MIDI starts at 2."
- Zone picture: "Key zones of this preset: each colour is one recording and the dot its root note. Hover a key for its zone; click to hear it."
- Missing banner: "This SoundFont isn't on this computer. The DawIt Synth plays instead; the project remembers the file and the preset, so the sound comes back once you add it."

### Starting points (MuseScore General, bank 0 unless noted; program shown 1-128)
| Name | Preset | Why |
|---|---|---|
| Grand piano for chords | 1 | The most used sampled sound: it carries chords and melody alike, and its recordings change from soft to bright with velocity. |
| Fingered bass | 34 | A round electric bass played with the fingers that sits under chords without covering them; the usual choice for pop. |
| String section pad | 49 | A string ensemble that swells in slowly; hold whole-bar chords under a chorus. |
| Nylon guitar | 25 | Plucked strings for arpeggios and soft chords; keep chords to shapes a guitarist could finger. |
| Drawbar organ | 17 | A held organ tone that loops for as long as you hold the keys; good for gospel, rock and soul chords. |
| Standard drum kit | bank 128, 1 | A kit that follows the General MIDI drum map, so DawIt's drum patterns play on real drums unchanged. |

### Example data
- DawIt Synth → sampled (Use a sampled sound): piano → 1, organ → 17, bass → 34, lead → 81, pluck → 25, pad → 49, drums and percussion → bank 128 program 1.
- Fallback (program's family → DawIt Synth): Piano → piano; Chromatic Percussion, Guitar, Ethnic → pluck; Organ → organ; Bass → bass; Strings, Ensemble, Synth Pad, Synth Effects, Sound Effects → pad; Brass, Reed, Pipe, Synth Lead → lead; Percussive → percussion; bank 128 → drums.
- Speed picture: intervals −12, −7, −5, +5, +7, +12 → speed 0.500, 0.667, 0.749, 1.335, 1.498, 2.000; length the inverse.
- Memory picture, MuseScore General: 38 MB on disk → 206 MB decoded (16-bit) → about 412 MB held (32-bit).

### "Your track" rules (first matching wins for the first line; later rules add lines)
1. Font missing: "This track's SoundFont, "X", isn't on this computer, so the DawIt Synth's {sound} plays instead. Add it with Add SoundFont... (or download MuseScore General) and the sampled sound comes back; the project still remembers it." Nothing else.
2. Loading: "Loading X: the DawIt Synth plays until it's ready." Nothing else.
3. No notes: "There are no notes on this track yet. Play the test note or draw notes in the piano roll, and this page shows which zones they use."
4. Notes outside every zone (n > 0): "{n} of {m} notes (lowest {note}, highest {note}) are outside the keys {preset} plays ({low} to {high}), so they make no sound. Move them an octave towards the middle, or pick a preset that reaches them."
5. At least 8 notes, the preset has 2 or more layers on the keys played, and every velocity falls in one layer: "All {m} notes fall in the velocity layer for {lo}-{hi}, so you hear only one of this preset's {L} recordings per key. Vary the velocities (the piano roll's velocity mode) and soft notes will use the soft recordings, not only play quieter."
6. Any note 12 or more semitones from its zone's root: "{note} plays a recording made at {root}, {k} semitones away, at {speed}× its speed; stretched that far a note can sound thin or unnatural. That is how this preset was made; another preset may cover that range better."
7. None of 4-6: "All {m} notes are inside {preset}'s range ({low} to {high}) and use {z} of its {Z} zones. Nothing to change."
Silent on rules 5-6 for drum kits (layers and roots mean little there).

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Test font, zone 1 (keys 0-71, root 69, sample of 441 Hz = 100 samples a period at 44.1 kHz): note 69 | 441 Hz | ±1 Hz |
| note 76 (+7) | 660.7 Hz (441 × 1.4983) | ±1.5 Hz |
| zone 2 (keys 72-127, root 81, 882 Hz): note 81 | 882 Hz | ±2 Hz |
| note 84 (+3) | 1048.9 Hz | ±2.5 Hz |
| velocity 40 (soft layer, pure sine) vs 100 (loud layer, sine + 2nd harmonic at −6 dB) | 2nd harmonic < −40 dB at vel 40; −6 dB ±1 at vel 100 | as stated |
| held 3 s on a sustain loop | RMS at 2.0-2.1 s within 1 dB of 0.2-0.3 s | 1 dB |
| same zone without loop, sample 0.5 s | silent (< −80 dB) after 0.6 s | |
| drum preset bank 128: note 36 sounds, note 60 silent | > −30 dB / < −80 dB | |
| two samplers on one file | 1 load; memory = sample count × 4 bytes | exact |
| preset list (header only) | names, banks, programs as written | exact |
| speed factors | 2^(n/12) for n = ±5, ±7, ±12 | 1e-4 |
| GM family of program 34 (shown) | Bass; 0-127 storage 33 | exact |
| fallback(sampled(s)) == s for every DawIt Synth sound except percussion (→ drums) | true | |
| memory estimate of MuseScore General | 2 × 206 = 412 MB | exact |
| Text: every page has a name, summary and text; search "soundfont", "sf2", "velocity layer" and "general midi" finds the guide | | |

## Steps

Group A1, engine: library and player (`./build.sh --test` at step 3):
- [x] 1. Fetch TinySoundFont and stb_vorbis in `CMakeLists.txt` (pinned tarballs with SHA-256), compiled in one engine source with their warnings silenced locally. Add the licence files, `licenses/README.md` provenance, `LICENSE` inventory and README *License note*. Done when: both targets build without warnings.
- [x] 2. `soundfonts::` in `src/engine/SoundFonts.{h,cpp}`: folder (with override), structure check, header-only preset list, zone map, the shared Library (background loading, shares, freeing on the message thread), plus a test-font writer in engine code (the shot runner uses it too): two zones, two velocity layers, a sustain-looped zone, an unlooped zone, a bank 128 kit, and an .sf3 variant written with JUCE's Ogg Vorbis writer. Tests: preset list, zone map, check rejects truncated, zero-length and non-RIFF files without crashing, one load for two users, memory figure.
- [x] 3. `BuiltInSampler` (instrument recipe, `APP_ONLY`, code `Dsmp` after checking it's unused): state, four parameters, atomics, copy handover and retire, the fallback `BuiltInSynth` with the family table, MIDI and controllers, test note. Register it in `BuiltInFormat` and `dawit_add_builtin`. Tests: the frequency, layer, loop, drum, fallback and handover checks from *Test numbers*; .sf3 plays the same pitch as .sf2; `checkThroughProject`; bend of +2 semitones with the default range.

Group A2, engine: downloads, files, guide content (`./build.sh --test` at step 6):
- [x] 4. MuseScore General download: generalise `models::Model` with a target folder, add the .sf3 and its licence file at a pinned URL with size and SHA-256 (download once by hand, `shasum -a 256`). Add copy or link of a user file, `soundfonts.linked`, the missing-file warning in `project::load`, and bounce waiting for loads plus listing fallbacks. Tests: copy and link find the file again; a missing font gives one warning and keeps the reference; a bounce right after load renders the sampled sound, not the fallback.
- [x] 5. Guide content in `src/engine/SamplerGuide.{h,cpp}` (`daw::samplerguide`): page names, summaries and texts from *Theory*, GM family names and ranges, speed factors, both mapping tables, starting points with their why, and `analyse` (zones + notes → the *Your track* lines). Tests: the text, mapping and speed rows of *Test numbers*; each advice rule on a made-up track, including quiet cases.
- [x] 6. Build and run `./build.sh --test`. Optional: `DawItTests --soundfont <file>` loads a real font and plays its first preset, for a by-hand check with MuseScore General.

Group B1, UI: menus and window (`./build.sh --test` at step 9):
- [x] 7. Sampled sounds menus in `PluginActions` and `TrackHeader` (shared through `TrackActions`), *Add SoundFont...* flow, download question, *Use a sampled sound* in `BuiltInSynthEditor`. Check: picking a preset on a DawIt Synth track adds the sampler; picking again changes only the preset.
- [x] 8. `SamplerEditor` (`fxui::EffectEditor`): font box, family and preset lists with Up/Down, zone picture with hover and click, velocity layers, knobs, starting points, missing and loading banner, *About this sound set...*. Set `createEditorFunction` in `MainComponent`. Device bar chip name and zone strip; inspector and track header names.
- [x] 9. Build and run `./build.sh --test`.

Group B2, UI: the guide (`./build.sh --test` at step 12):
- [x] 10. `src/ui/SamplerGuide.{h,cpp}` pages with the live pictures above and `setPrintSource`; *Your track* through the `fxui` host-track lookup.
- [x] 11. `gen::guides` entry (key `sampler`), `MainComponent::openGuide`, `BuiltInGuides` branches, and the guide as the last item of every Sampled sounds menu. Reword `SpectralFreezeGuide`'s sentence to: "DawIt Sampler plays SoundFonts, but it can't yet load a recording of your own, so there is no way yet to play a bounced hold across the keys; ...". Tests: `runTheoryGuidesTests` search words.
- [x] 12. Build and run `./build.sh --test`; `--export-guides` passes.

Group C, check and docs:
- [x] 13. Real-app check: `ScreenshotRunner` writes the test font into its scratch SoundFonts folder (add a scene if needed). Shots: (a) the track header instrument menu open on Sampled sounds; (b) the DawIt Sampler window on the test font's piano with the zone picture; (c) the same window with a missing font (banner, fallback name); (d) the device bar with the sampler chip; (e) guide pages 3, 7 and 10 on the pop song's bass track. Each must show readable text, no clipping, and the zones and colours as described. If MuseScore General is already downloaded on the machine, note its load time in *Changes while building* (it updates "takes a moment").
- [x] 14. Docs: README *Features* bullet (Mixing, effects and synths); `docs/code-layout.md` (new files); `docs/tests.md` (sampler and SoundFont checks); `docs/architecture.md` (the SoundFont Library, downloaded content beside Models, the audio-thread handover); `docs/recipes.md` (*A long job*: downloaded content that isn't a model). Issue #58 progress comment with done and still open; link #82.

## Tests
As in *Test numbers*, plus:
- the structure check rejects a file whose `phdr` index points past the end and a sample offset beyond `smpl`;
- a preset change on the audio thread allocates nothing (voices preallocated: 64 notes at once still play, the 65th takes the oldest releasing voice);
- the handover: swapping the copy mid-playback doesn't crash and frees nothing on the audio thread (retired copy closed by the timer);
- a project with two sampler tracks on the same font reloads with one Library load.

## Website hand-off
- `features.html`, *Mixing, effects and synths*: DawIt Sampler and sampled sounds; screenshot of the sampler window with the zone picture on MuseScore General (piano preset).
- `learn.html`: "What is a sampler?" under synthesis, screenshot of the Key zones page.
- `about.html`: TinySoundFont (MIT), stb_vorbis (MIT or public domain) and MuseScore General 0.2.0 (MIT, downloaded on request, 38 MB).
- Home or `for-you.html` only if the user wants "realistic instruments" highlighted.

## Left for later
- SFZ libraries through sfizz (decision 2).
- SoundFont modulators (mod wheel vibrato, velocity to brightness) and reverb and chorus amounts: needs another player or own code.
- A decoded cache of .sf3 files for faster loads; lower memory than 32-bit samples.
- The generator and templates choosing presets per part (decision 8).
- VST3/AU build of DawIt Sampler (decision 6).
- Loading your own single recording (#82), round robin, AI rendering (MIDI-DDSP, timbre transfer).

## Changes while building

### Group A1 (library and player), built

**Departures from the plan**
- **Libraries:** TinySoundFont and stb_vorbis are compiled together in `src/engine/SoundFontCore.cpp`, a static library `soundfontcore` (warnings off there only). `SoundFontCore.h` re-exports `tsf.h` and adds read-only accessors for what `struct tsf` hides (`dawit_tsf_get_zone` and friends). Pins: TinySoundFont tarball SHA-256 `76c356df...fa57`, stb tarball `be9842d2...dd7`. Licence work done: root `LICENSE` inventory (TinySoundFont with SFZero, stb_vorbis), `licenses/TinySoundFont-LICENSE.txt` (verbatim, SFZero line included), `licenses/stb_vorbis-LICENSE.txt` (the MIT alternative, verbatim), `licenses/README.md` provenance, README build and *License note* paragraphs, `DISTRIBUTION_TODO.md` rows, `docs/architecture.md` library list. **Left to step 4:** the MuseScore General entry in `LICENSE`, `MuseScore-General-LICENSE.md` and its provenance (its URL, size and SHA-256 are only fixed there).
- **Registered everywhere now:** `BuiltInFormat::getInstruments()` lists DawIt Sampler, so it already shows in the Add instrument menus (no window yet: `hasEditor()` is false until Group B1). That made two tests wait for later groups: the theory-guides check that every built-in has a guide exempts DawIt Sampler, and the guide-sources check ignores the `sampler` key (SOURCES.md already has its heading). Both exemptions switch themselves off when the guide exists (Group B2); no manual removal.
- `soundfonts::Reference` (file name, size, linked path, `find()`, `forFile`) is already in `SoundFonts.h`; step 4 only adds copy/link functions, `soundfonts.linked` and the warning.
- `sampler::getSampledPreset` and `getFallbackSound` (the two mapping tables) are in `BuiltInSampler.h`; step 5's guide text can read them.
- The sample count of an .sf3 is the highest zone end (TinySoundFont doesn't expose the decoded count); exact for an .sf2.
- Not tested: "a preset change on the audio thread allocates nothing" (no allocation hook in the tests); it holds by construction (only `tsf_channel_set_*`, no allocation, after the message thread made the voices and channel 0).

**Hand-off for the next groups**
- `soundfonts::` (`src/engine/SoundFonts.h`): `getFolder()`, `setFolderOverride()`, `check (file)` (plain-words `problem`), `readPresets (file)` (header only, `PresetInfo::getLabel()` "001 Name"), `Reference`, `Zone`/`LoopMode`, `Font` (`getZones (bank, program)`, `getPresets()`, `getSamples()`, `getMemoryBytes()`, `waitUntilLoaded()`, `getOpenCopyCount()`), `Library::get().acquire (file)` / `getLoadCount()`, and `testfont::make/write (compressed)` for the shot runner (presets: Test Piano, Test Layers, Test Organ, Test Pluck at bank 0 programs 0-3; Test Kit at bank 128 program 0).
- `BuiltInSampler` (`src/engine/BuiltInSampler.h`, identifier "DawIt Sampler", code `Dsmp`, `APP_ONLY`): `setFont (Reference)`, `setPreset (bank, program)`, `getStatus()` (noFont, missing, loading, failed, noPreset, ready), `getFailure()`, `getFont()`, `getFallbackSound()`, `isPlayingFallback()`, knobs by `sampler::Knob` (`setKnob`, `getSettings`), `clap.request()` for the test note, `history`, `lastNote`/`lastVelocity`/`activeVoices` for pictures. It is a `ChangeBroadcaster` (status and choice changes) and runs `service()` on a 200 ms timer; a window that has no timer should not call `service()` itself. State lives in `parameters.state` properties `font`, `size`, `link`, `bank`, `program` plus the four parameters.
- A reference that isn't found gives `Status::missing`; call `setFont` again with the same reference after the file is added or downloaded to retry. `project::load` does not warn yet (step 4).
- `sampler::getFamily (program)` gives the 0-15 family index; the family names and ranges belong to `SamplerGuide` (step 5).

### Group A2 (downloads, files, guide content), built

**Departures and facts beyond the plan**
- **MuseScore General pins:** `MuseScore_General.sf3` is 39,900,972 bytes (39.9 MB decimal, 38.05 MiB, which is the plan's "38 MB"), SHA-256 `5b85b6c2c61d10b2b91cddd41efcce7b25cd31c8271d511c73afafbef20b6fa3`; `MuseScore_General_License.md` is 3,419 bytes, SHA-256 `5ad8d737e13c7f01f5b9674872a82a92b4ba253603e8ed14b9db12293550b4b9`; `VERSION` reads 0.2.0. `models::formatSize` prints the set as "40 MB", so the download question (Group B1) should say "about 40 MB" (or the text keep "38 MB" as MiB); the guide's "38 MB" is the listing's figure and stays. The real font held 430 MB of samples (the highest zone end, so about, and the guide says "about 400 MB"); load took 1.7 s on this Mac; 309 presets; preset 1 is named "Grand Piano".
- **Licence work done:** `LICENSE` inventory entry (URL, size, SHA-256, the five named notices, the unresolved per-sample point), `licenses/MuseScore-General-LICENSE.md` (verbatim `MuseScore_General_License.md`), `licenses/README.md` provenance row, README *License note*, `DISTRIBUTION_TODO.md` (table row and a section 7 checkbox), `docs/architecture.md` (downloaded content beside Models). **Release obligations left:** ship `licenses/MuseScore-General-LICENSE.md` with every binary; verify the download from a packaged app; decide whether to keep relying on the overall MIT statement (Ethan Winer's and Amir Oosman's samples have no licence of their own in the sample-sources CSV).
- **`models::Model`** gained `getTargetFolder` (a function pointer, null means Models) and `source` (names the server in the error, default "Hugging Face"); existing `Model { ... }` initialisers are unchanged. The licence file is a second `Model`; `soundfonts::downloadMuseScoreGeneral` fetches it first, then the set.
- **Bounce:** `Renderer` finds the song's samplers when made and, at the start of `run()` (the render thread), waits for their fonts to load, checks `keepGoing` (so cancel works), then calls `sampler->service()` to hand each copy over. So no UI change is needed for the wait: every bounce site (`BounceDialog`, `BounceActions`, `LoudnessActions`) gets it. `BuiltInSampler::service()` now holds a mutex because the render thread and the timer can both call it. **Not wired to a UI yet:** `bounce::getFallbackTracks (const Engine&)` returns the lines ("Piano: the SoundFont "X.sf2" isn't on this computer") for the bounce dialogs to show before starting; do that in Group B1 or C.
- **Rule texts:** rule 4 reads "1 of 2 notes (E2) is outside the keys ... so it makes no sound. Move it ..." for a single stray note (the plan's plural text is used for two or more); rule 5 counts the notes that sound ("All {s} notes ...", s = notes inside a zone), and it needs 8 such notes; rule 6 shows the speed with 3 decimals. A preset with no zones gets "{preset} has no zones to play, so this track makes no sound." (mine). The Sources page summary, "Where the facts in this guide come from", and the "Your track" text without a track ("Choose a track that plays DawIt Sampler and ...") are mine.
- **Test-only:** `DawItTests --soundfont <file>` (step 6, optional): structure check, preset count, load time, memory, and plays the first preset.

**Hand-off for the next groups**
- `soundfonts::` additions (`SoundFonts.h`): `getMuseScoreGeneral()`, `getMuseScoreGeneralLicence()` (`models::Model`s for `models::download`), `getMuseScoreGeneralReference()`, `isMuseScoreGeneralInstalled()`, `getMuseScoreGeneralLicenceFile()` (for *About this sound set...*), `downloadMuseScoreGeneral (progress)` (background thread; "" for success or cancel), `isSoundFontFile`, `addFile (file, copy)` returning `AddResult { ok, reference, problem }` (the .sfz sentence and the structure-check problems are in `problem`; with link, call `addLinked (engine.properties.getUserSettings(), file)` yourself after the sampler takes the reference), `getLinked`, `addLinked`, `removeLinked` (take a `juce::PropertiesFile`), `getAddedFonts (const juce::PropertiesFile*)` for the Sampled sounds menu (copies in the folder by name, then existing linked files, never MuseScore General).
- After a download, call `sampler->setFont (soundfonts::getMuseScoreGeneralReference())` again to start the load (it was `missing` until then).
- `project::load` adds the missing-file warning for a sampler whose reference isn't found.
- `daw::samplerguide` (`src/engine/SamplerGuide.h`): `Page`/`allPages`, `getPageName`, `getPageSummary`, `explain (Page, const TrackView* = nullptr)` (the text, with the picture paragraph last), `TrackView` (`font` ready/missing/loading, `fontName`, `presetName`, `isDrumKit`, `fallbackSound`, `zones` from `Font::getZones`, `notes` as key after Transpose and velocity), `analyse (TrackView)` (lines), `getFamilyName/Range/First/LastProgram`, `getFamilyOfShownProgram`, `getSampledPreset (BuiltInSound)` and `getFallbackSound (SampledPreset)` (programs shown 1-128), `getStartingPoints()`, `getSpeed`, `getLengthFactor`, `getPictureIntervals`, `getBytesPerMinuteMono16`, `getMemoryBytes`, `museScore*Mb`, `getNoteName` (C4 = 60).
- The guide's page enum names: recordings, pitch, zones, velocity, loops, soundFont, generalMidi, memory, sampledOrSynth, yourTrack, sources.

### Group B1 (menus and window), built

**Departures and texts of mine (beyond the plan's Theory)**
- **Menus** live in `src/ui/SamplerActions.{h,cpp}` (`daw::sampleractions`), not in `TrackActions`: `createSampledSoundsMenu (engine, currentSampler, pick, openGuide)` builds the submenu (MuseScore General by General MIDI family when bank 0 has more than 16 sounds, other banks under *More banks*, bank 128 under *Drum kits*; added fonts by file name; *Add SoundFont...*; an optional last item for the guide), `createTrackMenu (engine, windows, track)` wraps it for a track, and `useSampledSound (...)` adds DawIt Sampler (replacing the instrument) or only changes the font and sound. It is in the add menu (`PluginActions::showAddMenu`, so also the device bar and inspector) and in the track header's Add/Replace instrument menu. While MuseScore General isn't downloaded its submenu lists the six starting points under a header "Picking one asks to download it (about 40 MB)". The menu's last item "What is a sampler?..." appears once a caller passes `openGuide` (Group B2).
- **Download question** (`whenMuseScoreGeneralIsHere`): "Download MuseScore General?", body "DawIt Sampler's free sound set: a full General MIDI set of 128 sounds and drum kits, about 40 MB, MIT licence. It is downloaded once to DawIt's SoundFonts folder. Until then the track plays the DawIt Synth." [Download] [Not now]; progress window with Cancel; after a download every sampler waiting for the set retries loading it.
- **Add SoundFont...** (`addSoundFont`): the chooser also lists *.sfz so that the refusal sentence is reachable; the check runs on the message thread (it only streams the tables), the copy too (a 200 MB .sf2 takes about a second). Buttons: "Copy into DawIt's SoundFonts folder" (default), "Use it where it is", "Cancel". After adding, the first sound (bank 0 program 0 when present) is chosen.
- **BuiltInSampler** gained `playKey (key)` (the window's keyboard), `getDisplayName()` ("DawIt Sampler - Acoustic Grand Piano", with "(missing)", "(loading)", "(unreadable)" or "- no such sound" in the other states), `getPresetName()` and a cached `getZones()`. Tests added in `runSamplerTests`.
- **Window** (`src/ui/SamplerEditor.{h,cpp}`, `BuiltInSamplerEditor (BuiltInSampler&, Engine&)`, 1400 x 780, wired in `MainComponent`): SoundFont box (menu: MuseScore General, added fonts, Add SoundFont..., About this sound set...), family list, preset list (clicking or Up/Down chooses and plays the test note), zone picture and keyboard (`src/ui/SamplerPictures.{h,cpp}`: x is key, y is velocity, one colour per distinct key/velocity range, a dot on each root, a line for the last note; hover text "E2 - zone C2-F#2 - root D2 - velocity 1-127" in the heading and as the tooltip; click or drag plays keys; right-click: Copy preset name, guide), velocity layers of the last key, the last 4 seconds, four knobs, six starting points. The header button reads "Play a note" (the shared label for instruments), not "Test note". There is no *Hear only the effect* switch (hidden, as in DawIt Meter). The banner over the zone picture covers: no font, missing, loading, unreadable, no such sound, with [Add SoundFont...] and [Download MuseScore General]. *About this sound set...* opens a read-only window: for MuseScore General the licence file beside it (or a sentence saying it isn't there yet), for other fonts the path, size, sound count and memory.
- **Explainer texts are mine** (the plan has only the tooltips): the overview, the topics for the SoundFont box, the lists, the zone picture, the layers and the banner, built from facts F1, F5, F6, F11 and F13 and the plan's tooltips, which are used as they are for the four knobs.
- **Names:** device bar chip, inspector and track header FX button show `getDisplayName()`; the chip gets a zone strip (`chainpictures`, `samplerpics::paintMiniStrip`).
- **Use a sampled sound** button in the DawIt Synth window's footer (`BuiltInSynthEditor`), through `useSampledSound` on MuseScore General with `sampler::getSampledPreset`.
- **Bounce:** `BounceDialog` now lists tracks from `bounce::getFallbackTracks` and asks "Bounce anyway" / "Cancel" before starting (both the mix and the multitrack bounce). The in-place and arpeggio bounces of `BounceActions` and the loudness render are not covered.
- `createGuidePages()` of the window returns the guide's text pages with empty pictures; Group B2 replaces it with `SamplerGuide` in `src/ui`. The guide's *Sources* page isn't in the list yet either (the sources key has no `gen::guides` entry).
- Not checked in the real app (Group C): no shot list ran for this group, only `./build.sh --test` (All checks passed, no warnings).

**Hand-off for Group B2**
- Open the guide from the window with `openGuide (page)` (an `EffectEditor` function, protected: `BuiltInSamplerEditor::createGuidePages()` is where `src/ui/SamplerGuide` pages go). `ZoneView::onOpenGuide` and the menu's `openGuide` argument are the other entry points; pass a `std::function` to `createTrackMenu`/`createSampledSoundsMenu` (add the parameter to `createTrackMenu`) so the Sampled sounds menus end with the guide.
- Pictures can use `samplerpics::` (`getZoneColour`, `getDistinctZones`, `paintZoneMap`, `paintKeyboard`, `describeKey`) and `BuiltInSampler::getZones()`/`getFont()`; for *Your track* find the track through `fxui::getHostTrack` (it gives only the name: extend it or look the processor up in the engine, as `MainComponent` does).

### Group B2 (the guide), built

**Departures and texts of mine (beyond the plan's Theory)**
- **Guide menu item:** instead of passing `openGuide` through every caller, `sampleractions::setGuideOpener (fn)` (installed by `MainComponent` beside the host lookup, cleared in its destructor) gives `createTrackMenu` the guide item, so the track header, add menu, device bar and inspector all end their Sampled sounds menu with "What is a sampler?...". `createSampledSoundsMenu` still takes `openGuide` for other callers.
- **Your track** uses its own lookup, `samplertext::setTrackLookup` (in `SamplerGuide.h`, installed by `MainComponent`): it finds the processor's track under the lock and returns the unmuted MIDI clips' notes (velocity as 1-127). `fxui::HostTrack` is unchanged. Two lines of mine for states the plan's rules don't cover: no SoundFont chosen ("This track has no SoundFont chosen yet, so the DawIt Synth's {sound} plays. Choose one ..."), and a SoundFont that couldn't be read ("This track's SoundFont couldn't be read ({problem}), so the DawIt Synth plays instead."). A sampler on no track shows the "Choose a track..." text.
- **Buttons:** the guide frame gives each page one button, so page 2 has "Hear both" (A3, then A3 moved by the interval shown; each click steps to the next interval, so the six intervals are chosen by clicking, not by +/- buttons) and page 9 has "Hear a short tune" (four notes through the track's own sound) instead of two buttons for the synth and the sampled piano. The final paragraph of page 9 in `samplerguide` still says "The buttons play the same chord on the DawIt Synth's piano and on the sampled piano" and page 2's says "Change the interval ...; click to hear both": **dawTheory or the owner should reword these two paragraphs to match**, or a later group should give the frame two buttons and a chord.
- **No SoundFont ready:** pictures 1, 3 to 8 draw a made-up example (a recording every 6 keys in a soft and a loud layer; MuseScore General's 38/206/412 MB for the memory picture) with the line "An example: choose a SoundFont in DawIt Sampler's window to see yours." Page 6 shows presets, zones and recordings of the chosen preset (the count of instruments isn't available from TinySoundFont, so the middle level is zones); page 7 for a drum kit lights the notes 35-81 that the kit plays and names only the six drums of the plan (36, 38, 42, 46, 49, 51). Page 1's caption uses the zone's note range, as samples have no names in TinySoundFont.
- `fxui` untouched. `gen::guides::Guide::sampler` is in `Area::synthesis` (its summary "five ways" is the area's, not changed). The plan's step-11 tests are in `runTheoryGuidesTests` (entry, key, sources key, not on Start here, and the four search words) and the plugin-key count is 21.
- Removed the two A1 test exemptions (guide-sources ignoring `sampler`, theory-guides exempting DawIt Sampler); the guide-count text no longer says 36. `SpectralFreezeGuide`'s sentence now reads "DawIt Sampler plays SoundFonts, but it can't yet load a recording of your own, so there is no way yet to play a bounced hold across the keys; ...". `--export-guides` passes (the sampler guide is 11 topics, 12 pages).

**Hand-off for Group C**
- Shots: the guide on every page (`"guide": "sampler"` and `"topic"` with the page name) with a sampler on a track holding a font (the test fonts of `soundfonts::testfont` or MuseScore General), one without a font (example pictures), one with a drum kit for page 7, and "Your track" with notes outside the zones. Not looked at in the app yet.
- Docs still to do: README bullet and *License note* check, `docs/tests.md`, `docs/code-layout.md` (`src/ui/SamplerGuide.{h,cpp}`), recipes only if wiring changed (the guide-opener hook is new), the issue, and `SOURCES.md` already has its heading.

### Group C (real-app check and docs), built

- **Guide texts reworded** (for dawTheory): page 2's last paragraph now reads "... the same wave, squeezed. Each click on Hear both plays the note and then the same note moved by the interval shown, and the next click moves on to the next interval, so you can see and hear how much faster and shorter it gets." Page 9's last paragraph now reads "The button plays a short tune of four notes through your track's own sound, so you can hear it on the DawIt Synth and then on a sampled sound." The *Theory* page texts above are updated to match.
- **ScreenshotRunner:** writes the test font ("Test Sounds.sf2") to a scratch SoundFonts folder (`dawit-screenshot-soundfonts`, set with `setFolderOverride`) at the start of every run, and has a shot key `"sampler": {"font": "test|missing|none", "bank", "program"}` with `"track"` (documented in `DevScreenshots.h` and `recipes.md`). No scene was needed.
- **Shots looked at** (scale 1, scratchpad `shots/`): the sampler window on the test piano (two zones, dots on the roots, layers, knobs), the same with a missing font (banner text, "(missing)", Add SoundFont... button), the device bar chip (name and zone strip), guide pages 1 to 11 on the pop song's Bass (page 3 zones, page 6 the three-level tree, page 7 the drum map with the kit's 36 and 38 lit, page 9 side by side, page 10 "Your track"), page 10 on the test kit with 108 of 108 notes outside the zones (red dots, the plan's rule 4 text), and page 3 with no SoundFont (made-up example and its caption). Nothing was cut off or overlapping; no fixes were needed.
- **Not captured:** the track header's Sampled sounds menu (a popup can't be photographed by the shot list; its contents are covered by tests of `createSampledSoundsMenu`). MuseScore General was not on this machine for the shots; its load measured in group A2 was 1.7 s (309 presets), so "takes a moment" stands.
- **Docs:** README feature bullet (the instruments paragraph of *Learn by making music*; the README has no *Features* heading), `docs/code-layout.md`, `docs/tests.md`, `docs/architecture.md` (the sampler, the Library, the handover), `docs/recipes.md` (downloaded content; the shot key). Licence files were checked: `LICENSE` inventory, `licenses/` (TinySoundFont, stb_vorbis, MuseScore General), the README *License note* and `DISTRIBUTION_TODO.md` are complete.
- **Release obligations left:** ship `licenses/MuseScore-General-LICENSE.md` with every binary; verify the MuseScore General download from a packaged app; decide whether to rely on the overall MIT statement for the two sample authors without their own licence; include TinySoundFont and stb_vorbis in source packaging (`DISTRIBUTION_TODO.md`).

### dawTheory check
- Sizes in decimal MB, as DawIt shows them everywhere (download question "about 40 MB", the memory picture of a real font): page 6 now says MuseScore General is 40 MB as .sf3 and 216 MB as .sf2 (39,900,972 and 215,614,036 bytes; the listing's 38M and 206M are MiB), page 8 says about 430 MB in memory (2 x 216 = 432, matching the 430 MB measured); `samplerguide::museScore*Mb` are 40, 216 and 432, and the test's expected values follow.
- Page 1's picture paragraph no longer promises the recording's name (TinySoundFont gives none): it shows the keys it covers and its root.
- Page 2's picture paragraph says "higher or lower ... squeezed or stretched" (the intervals include -12, -7 and -5).
- Page 5 adds the SoundFont envelope's hold between attack and decay (the zone picture draws it).
- Page 6's picture paragraph describes what is drawn: presets in the file, zones of the chosen preset, recordings they use (not instruments); the picture's "thousand samples" reads "thousand sample values in the whole file", so it isn't confused with recordings.
- Page 7's picture paragraph: the drum map 35 to 81 with the kit's notes lit and the six main drums named, not every note's name.
- Rule 3 says "Click Play a note in DawIt Sampler's window" (the button's real label), not "Play the test note".
- MuseScore General is described as "all 128 General MIDI sounds, drum kits and more" (it has 309 presets) in the download question, the window's overview and About this sound set.
- Confirmed with sources (added to SOURCES.md): no round robin in SoundFont 2 (Polyphone forum, the unadopted SoundFont 3 proposal); 8+ velocity layers (Salamander Grand Piano, 16 layers, every minor third).
- **Fallback warning on every bounce path:** the "Some tracks play a stand-in sound" question ("Bounce anyway" / "Cancel", lines from `bounce::getFallbackTracks`) is now `bounceui::goUnlessFallbacks` in `BounceDialog.{h,cpp}`, called by the Bounce to WAV dialog, *Bounce in place*, *Create MIDI clip from arpeggiator* (`BounceActions`) and *Measure Loudness...* (`LoudnessActions::measureSong`). `measureSongNow` (synchronous, for scripts and shots) doesn't ask. The question is UI, so no new test: the lines themselves are covered by the existing `getFallbackTracks` checks.
