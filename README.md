# DawIt - DAW In Theory

**Website: [dawit.studio](https://dawit.studio)**

DawIt (*DAW In Theory*) is a free, open-source desktop music sketchpad that helps you learn while making music. Write and record parts, try out chords and sounds, and explore music theory through playable examples and guides that follow your song.

Built with C++20 and [JUCE](https://juce.com). macOS is the primary target; Windows and Linux build instructions are below.

![DawIt's arrangement showing song sections, guitar chords, lyrics and audio tracks](website/assets/screenshots/karaoke-main.png)

*The app: follow chords and lyrics alongside the audio tracks in an arrangement.*

![The What is sound? guide showing a waveform, the clip's spectrum and its envelope](website/assets/screenshots/sound-guide-envelope.png)

*A theory guide: explore timbre, harmonics and the shape of a sound.*

![DawIt EQ showing its frequency curve over typical instrument ranges, with six filter controls](website/assets/screenshots/eq-instruments.png)

*A built-in effect: shape a sound with EQ and see where instruments sit in the frequency range.*

## Learn by making music

DawIt is for learning about music and sound while making music: from your first beat or chord to singing, recording, synthesis and mixing. Its [theory guides](https://dawit.studio/learn.html) put explanations, playable examples, live pictures and advice about your own song beside the tools you use. Explore the full catalogue on the [features page](https://dawit.studio/features.html).

- **Find a path through music and sound:** the Theory Guides window offers a beginner path and searchable topics across sound, MIDI, rhythm, reading music, harmony, songwriting, singing, mixing, effects and synthesis. Each guide ends with its sources. You can try the effects and synth guides with test sounds before making a track.
- **Understand what sound is:** explore moving air, waveforms, pitch, loudness, harmonics, envelopes, phase and digital audio in the sound guide. Play and change a tone, or watch the pictures follow your own recording, from its overall shape down to individual samples.
- **Learn how songs fit together:** guides to chords, the circle of fifths, song structure, styles and lyrics explain harmony, repetition, contrast, rhyme and how words fit a melody. Try progressions, arrange sections and time lyrics in your own song. Optional local AI gives you musical parts and lyrics to explore.
- **Learn rhythm by hearing and making it:** playable examples explain note lengths, rests, triplets, swing and time signatures, including compound and odd metres. Try them with the piano roll, metronome and count-in, or use the arpeggiator and its guide to hear how note order, accents and swing turn chords into patterns.
- **Quantise MIDI notes to a grid:** in the piano roll (the *Quantise...* button, or Q to repeat the last settings) and in a MIDI clip's right-click menu (*Quantise notes...*), a dialog moves the selected notes, or the whole clip, towards the nearest line of a grid (1/4 down to 1/32, with triplets), all the way or by a strength from 0 to 100%, with *Quantise ends too* as an option. A picture shows each note's outline where it is now and the filled note where it will go, with a line such as "16 of 16 notes move, by up to 0.04 beats (25 ms at 96 BPM)". Chords move together, pitch, velocity and (unless ends are quantised) length stay, and Cmd+Z undoes it in one step. The rhythm guide's *Quantising* and *Feel and the grid* topics explain grid, strength, why perfectly in time isn't always better, and its *Your song's rhythm* topic notices notes played between the grid lines.
- **Learn to read music and understand MIDI:** the reading guide explains your own part as sheet music, with notes lighting as they sound. The MIDI guide connects notes, velocity, pedals and pitch bend to the messages behind them. Hear a melody backwards or upside down, and find a clip's key and chords with reasons for the results.
- **Learn by playing, singing and recording:** chord diagrams show which notes to play on piano, how inversions work and where to put your fingers on guitar. The singing guide explains the voice, breathing, tuning and harmonies, measures your range from a recording and checks a melody against it. Record yourself, follow chords and lyrics, or explore how a computer sings English lyrics with a separately supplied voice.
- **Learn from a recording's parts:** separate instruments and vocals into stems to hear how an arrangement works, or remove the vocals to practise singing. Explore detected tempo, chords, sections and English lyrics, and turn pitched audio or drum hits into MIDI to examine the notes and rhythms. Analysis runs on your computer.
- **Understand mixing and effects:** guides explain gain staging, routing, EQ, compression, gates, stereo width, pitch correction, harmonies, reverb, delay, distortion and more. Change a control and hear and see its effect. Follow your song through buses, sends and effect chains, learn why effect order matters, and use the loudness guide to understand the meter readings. The *Sending stems to an engineer* guide (the "?" in the Bounce to WAV window, or *Sending stems to an engineer...* in a track's right-click menu) explains stems, multitracks and the mix, what a mixing or a mastering engineer wants, same-start-same-length 24-bit files, headroom, printed or dry effects, a null test, what to send with the files and working through revisions. Its *Your song's stems* topic draws your song as the files Bounce to WAV would make, with the measured peak, the file sizes at your chosen rate and bit depth, and advice such as tracks named "Audio 2", a muted track, or a bus compressor that stops multitracks adding up to your mix.
- **Learn how instruments make their sounds:** explore subtractive, additive, FM, drum and physical modelling synthesis through guides and playable instruments. Hear how harmonics, filters, envelopes, vibrating strings and other sound sources shape a note. Start with an explained sound, change it and save your settings to revisit the experiment. DawIt Sampler plays real recordings from a SoundFont (.sf2 or .sf3): pick a sound from the Sampled sounds menu in a track's instrument menu (the free MuseScore General set, about 40 MB, is downloaded after asking) or add your own file, then see its key zones and velocity layers and read the *What is a sampler?* guide.
- **Write things down as you work (Notebook):** The notebook icon in the top bar, *View > Notebook* or Cmd+9 opens or closes a panel on the right of the tracks, with three tabs. *App* is your own notebook, the same in every song, kept on this computer in `Notebook.txt` next to the settings and saved as you type. *Project* holds notes about this song and *Track* notes about the selected track; both are saved in the `.dawit` file. *Track Notes...* in a track's right-click menu (or a click on the small note icon that appears on a track with notes, in its header, mixer strip and the inspector) opens the Track tab, and the icon's tooltip shows the first line. Each tab says in plain words what it holds, with an example when empty. Notes stay out of Undo; the text box has its own Cmd+Z.
- **Take what you learn into other music making:** save projects to return to your experiments, export audio and MIDI to another DAW (Bounce to WAV makes the mix, the selected tracks, or one file per track or per bus, with a Dry choice for multitracks, in a new folder with the tempo and key, and a "?" that opens the guide to handing over stems; Bounce in place prints a track or clip to a new audio track; an audio clip can be dragged out as a WAV), and print parts, chord sheets and guides as PDFs for practice. DawIt's exported effects and synth plugins keep their explanations and guides inside the plugin window, so you can keep learning in another DAW.

## Optional local AI setup

AI generation uses a separately installed [Ollama](https://ollama.com) server. Install a model, for example:

```bash
ollama pull qwen2.5:7b
```

Keep Ollama running, using its app or `ollama serve`, and choose an installed model in DawIt's Generate panel. The default server address is `http://localhost:11434`; generation runs on your computer. The editor, built-in sounds and theory guides work without Ollama.

For ChatMusician, install a compatible model separately in Ollama and select its name or alias in Generate. Choose a MIDI clip starting on a bar line and spanning whole 4/4 bars (the song must be in 4/4; the Generate button says so otherwise). Melody needs a chord on every bar; chord generation offers explicit harmony choices and optional first and last chords. Preview uses a simple tone and is limited to two minutes. The initial melody format uses only natural notes, even when the key contains sharps or flats; it does not support accidentals, rests or varied rhythms. Harmonising an existing melody is not supported. A valid response is not a guarantee of musical quality. If the model is missing, refresh the list or check the alias; if its response is invalid, try again. Cancel discards the result, though the non-streaming local request may finish in Ollama in the background.

## Building

Requirements: CMake ≥ 3.22, a C++20 compiler and Git. JUCE 8.0.9 comes from `external/JUCE` if that folder exists, otherwise CMake downloads it automatically. CMake also downloads demucs.cpp and Eigen at pinned versions (checked by hash) for stem separation. On macOS their matrix maths runs through Apple's Accelerate framework. For audio to MIDI development builds download Microsoft's prebuilt ONNX Runtime 1.30.0 for the platform (macOS on Apple Silicon, Windows x64/ARM64, Linux x64/ARM64). Release candidates build the same version from pinned source for macOS Apple Silicon, Windows x64 and Linux x64. The build copies the runtime into the app (`Contents/Frameworks` on macOS, next to the executable elsewhere) and compiles in the Basic Pitch model (230 KB). For detecting lyrics it downloads whisper.cpp 1.9.5 (with ggml) and builds it as static libraries, on Metal and Accelerate on macOS. For sampled instruments it downloads TinySoundFont 0.9 and the stb repository's `stb_vorbis.c` at pinned commits (checked by hash) and compiles them into one small static library.

### Scripts (macOS / Linux / Git Bash on Windows)

```bash
./build.sh                 # Release build into build/Release
./build.sh debug --test    # Debug build, then run the engine tests
./build.sh --clean         # wipe build/Release and rebuild

./run.sh                   # build if needed, then run (log output in this terminal)
./run.sh debug -- --demo   # run a Debug build with example tracks
./run.sh --no-build        # run the existing build as-is
```

Only one copy of DawIt can run at a time, so quit the running one before using `run.sh`. The sections below show the equivalent plain CMake commands.

### macOS

Requires macOS 14.0 or later and the Xcode Command Line Tools or Xcode. The full app's minimum macOS version still needs clean-machine testing.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
open build/DawIt_artefacts/Release/DawIt.app
```

To get an Xcode project instead, use `-G Xcode`.

### Windows

Requires Visual Studio 2022 with the "Desktop development with C++" workload.

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The app is written to `build/DawIt_artefacts/Release/DawIt.exe`. Plugins are scanned from `C:\Program Files\Common Files\VST3`.

### Linux

Install JUCE's dependencies (Debian/Ubuntu names):

```bash
sudo apt install build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev libcurl4-openssl-dev
```

Then build the same way as on macOS. VST3 plugins are scanned from `~/.vst3` and `/usr/lib/vst3`. Linux needs curl because the AI generator uses HTTP; macOS and Windows use their built-in networking.

## Tests

`DawItTests` is a headless test of the engine. It drives the audio callback through a fake device; what each area checks is listed in [docs/tests.md](docs/tests.md).

Optionally, pass an instrument and an effect to test real plugin hosting. Each is a `.vst3` or `.component` path, or an Audio Unit id such as `AudioUnit:Effects/aufx,rvb2,appl`:

```bash
cmake --build build --target DawItTests
./build/DawItTests_artefacts/Debug/DawItTests /Library/Audio/Plug-Ins/VST3/SomeSynth.vst3 /Library/Audio/Plug-Ins/VST3/SomeEffect.vst3
./build/DawItTests_artefacts/Debug/DawItTests "AudioUnit:Synths/aumu,dls ,appl" "AudioUnit:Effects/aufx,rvb2,appl"
```

The tests also check how model replies become notes: chord building, naming, parsing and the chord track, key mapping, timing, looping and the drum note numbers. These checks need no model. To run one real generation per part type against Ollama, add `--ollama <model>`:

```bash
./build/Debug/DawItTests_artefacts/Debug/DawItTests --ollama qwen2.5:7b
```

The rhythm checks cover naming note values, dotted notes and triplets, splitting other lengths into tied notes, positions and counts, and the guide's topics. The section checks cover tidying overlapping sections, saving and loading them, every structure being whole 4-bar phrases that is explained and recognised, numbering repeated sections, and the advice on a song's own structure. The playing-style checks cover the Generate panel's requests (no model needed for a style played as it is, only chords asked for when bars have none), the prompt and schema, varied patterns with unusable events left out, and scoring a variation above a copy of the style. With `--ollama`, four styles are varied by the real model. The chord style checks play every built-in style over C, Am, F, G and check that it plays only the chords' notes, that each strum runs the right way, that guitar shapes are the usual open shapes (and playable for every chord quality on every root), pushed chords, empty bars, and the walking bass's approach notes.

Audio to MIDI runs the real Basic Pitch model on a synthesized melody and chord, checks that every note is found at its time, and checks the note-finding rules, *One at a time*, quantising and the new track. The C++ port gives the same notes as the basic-pitch Python package on the same audio.

Stem separation is tested without the model too: preparing the input, separating in pieces and cross-fading them back together, cancelling, leaving out silent stems, adding the stem tracks, and putting one stem on its own track. The karaoke checks mix made-up stems back into the mix and into the mix minus the vocals, put the Karaoke and Vocals tracks right below the clip with its position and gain, mute the vocals with the guide off and play them at -12 dB when quiet, make the instrumental as stems, add nothing for a clip without singing, and save and load the new tracks with the project. The sound guide's checks measure left minus right: a sound in the middle cancels, one on a single side keeps its level, a centred bass loses more than 30 dB below 150 Hz, and a mono clip leaves nothing; the engine plays a centred tone as silence while *Hear left minus right* is on. To split a made-up kick-and-bass mix with the real model, add `--demucs <model file>`:

```bash
./build/Release/DawItTests_artefacts/Release/DawItTests --demucs ~/Library/Application\ Support/DawIt/Models/ggml-model-htdemucs-4s-f16.bin
```

To check the lyrics detection with a real Whisper model, add `--whisper <model file>`. On macOS it makes speech with the system's voice (`say`), with a two-second pause in it, and checks the words, where the phrases start and end, and the two lines:

```bash
./build/Release/DawItTests_artefacts/Release/DawItTests --whisper ~/Library/Application\ Support/DawIt/Models/ggml-small.en.bin
```
- Signal flow: the falling levels (into the chain, out of an effect, out of a send) stay when the device bar reads the peaks and fall below 0.001 in a second; the graph of a made-up song (20 boxes, 19 lines and a -12 dB send, the columns and rows, the bypassed box, the lit path of Kick, Vocal and Drums, mute and solo silence, 0.5 shows as -6.02 dB, the example's -9.88 dB and -7.48 dB, and that building the graph changes nothing in the song); a 4:1 compressor's 4.5 dB difference between 6 dB before it and after it; two EQs in either order; the Signal flow topic's text and search words; and the five effect-order advice rules, their buttons, what they leave alone and that moving the compressor removes the advice

To build without tests, pass `-DDAWIT_BUILD_TESTS=OFF`.

The app also has developer flags. `--demo` fills the edit with example tracks. `--screenshots shots.json` takes the screenshots in a list on the example song (an A minor pop song with sections, chords, lyrics, and Drums and Vocals buses): each shot says what to select, open or zoom to (a guide at a topic, a plugin's window, the mixer) and what to capture, and the run uses settings of its own, so it starts the same every time; the format is in `src/ui/DevScreenshots.h`. `--screenshot out.png` renders the window to a PNG and quits. `--export-guides <folder>` saves every guide as a PDF in the folder, then *Start here*, printing the pages, size and time of each, and exits with 1 if an export failed or a guide's topics differ from the Theory Guides list.

## Code layout

How the parts fit together is in [docs/architecture.md](docs/architecture.md), the files each kind of feature touches in [docs/recipes.md](docs/recipes.md), and the UI's look and interaction rules in [docs/ui-design.md](docs/ui-design.md).

The file-by-file list is in [docs/code-layout.md](docs/code-layout.md).

### Threading model

Only the message thread changes the edit (tracks, clips, notes, plugin chains), and it always holds `Engine::getLock()` while doing so. The audio thread only *try-locks* that lock. If the UI happens to hold it, the audio thread outputs one block of silence instead of waiting. Mixer controls and transport state are atomics. Heavy work (file loading, plugin creation and destruction) happens outside the lock.

Positions on the timeline are in beats. Audio clips keep their length in seconds, so their length in beats follows the tempo.

## Known limitations / next steps

- The time signature is one for the whole song: it can change inside a song only by splitting it into separate projects (a signature map is a later idea). MIDI import does not yet read the signature (#49), and a bar holds one chord (#102).
- Plugins are scanned in-process. A plugin that crashes during a scan is skipped on the next scan (dead man's pedal), but it takes the app down that one time.
- Audio clips are fully loaded into RAM (up to 30 minutes per file) and resampled with linear interpolation.

## License note

DawIt is fully open source under the GNU Affero General Public License, version 3 only (`AGPL-3.0-only`). [LICENSE](LICENSE) contains the complete licence and the current component inventory; [licenses/](licenses/README.md) preserves third-party copyright, licence and notice texts with their provenance. JUCE 8.0.9 is used under its AGPLv3 option. The VST3 SDK bundled with it is offered under Steinberg's licence or the GPLv3; DawIt uses it under the GPLv3, which may be combined with AGPLv3 code. (Only VST3 SDK 3.8 and later, bundled from JUCE 8.0.11, is MIT.) The Audio Unit plugins use Apple's AudioUnitSDK (Apache 2.0), which JUCE bundles. The binary notice packaging and complete-source release checks remain in [DISTRIBUTION_TODO.md](DISTRIBUTION_TODO.md).

Windows draft installers are made with Inno Setup 6.4.3; its copyright and licence are in [licenses/Inno-Setup-LICENSE.txt](licenses/Inno-Setup-LICENSE.txt).

The bundled VST3 SDK also contains BSD-style licensed base and public SDK code. JUCE's HarfBuzz uses Unicode data (Unicode License V3). ggml contains rotary embedding code attributed to YaRN (MIT) and vector routines attributed to Arm. The ONNX Mac library includes zlib and nlohmann/json; their exact-version notices, the other notices and Basic Pitch's upstream NOTICE are in [licenses/](licenses/README.md).

The score view and its PDF draw clefs, rests, flags and note heads with Bravura (Steinberg, the SMuFL reference font), which is under the SIL Open Font License 1.1. It is downloaded at a pinned commit and a SHA-256 hash when building, embedded in the app, and its licence text is shipped with the app (`Bravura-LICENSE.txt`).

Stem separation uses demucs.cpp (MIT), Eigen (MPL 2.0, built with `EIGEN_MPL2_ONLY`), and the Demucs v4 `htdemucs` weights (MIT), downloaded from the [Retrobear/demucs.cpp](https://huggingface.co/datasets/Retrobear/demucs.cpp) dataset on Hugging Face. Audio to MIDI uses Spotify's Basic Pitch model (Apache 2.0) and ONNX Runtime (MIT), whose library ships inside the app; its `ThirdPartyNotices.txt` lists the components it contains. Detecting lyrics uses whisper.cpp and ggml (MIT) and OpenAI's English Whisper weights `small.en` and `medium.en` (MIT), downloaded from the [ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp) repository on Hugging Face at a fixed revision.

DawIt Sampler plays SoundFont files (.sf2 and .sf3) with TinySoundFont (MIT, by Bernhard Schelling, based on SFZero by Steve Folta, MIT) and decodes the compressed samples of .sf3 files with stb_vorbis (Sean Barrett; MIT or public domain, DawIt uses the MIT terms). Both are compiled into the app and their notices are in [licenses/](licenses/README.md). The free sound set it offers to download, MuseScore General 0.2.0 (`MuseScore_General.sf3`, 39,900,972 bytes, fetched from [ftp.osuosl.org](https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/) after asking, SHA-256 checked), is under the MIT licence with copyright notices for Frank Wen, Michael Cowgill, S. Christian Collins, Ethan Winer and Michael Schorsch; DawIt does not ship it, and saves its licence file beside the download (full text in [licenses/MuseScore-General-LICENSE.md](licenses/MuseScore-General-LICENSE.md)). SoundFonts you add are your own files with their own terms.

Sing this uses DiffSinger models (openvpi, Apache 2.0; no code is linked, DawIt runs exported ONNX models with ONNX Runtime) and follows OpenUtau (MIT) as its reference for how a voice is laid out and the models are called. The CMU Pronouncing Dictionary (Carnegie Mellon University, `cmudict.dict` at a pinned commit of cmusphinx/cmudict, SHA-256 checked, built into the app, about 3.6 MB) permits redistribution under its BSD-style terms. Its full copyright, conditions and disclaimer are preserved in [licenses/CMUdict-LICENSE.txt](licenses/CMUdict-LICENSE.txt); they must accompany binary distributions, rather than only a one-line acknowledgement. No voice ships with DawIt: the user adds one, and each voice has its own terms, which the Sing window shows (Terms...). The voice DawIt suggests, Hoshino Hanami ~AI❤dol~ for DiffSinger by Lotte V, has a voicebank licence that allows creative and commercial use with credit and forbids redistributing the original audio files; its terms of use also forbid selling the files and political or hateful use. Its vocoder file is licensed CC BY-NC-SA 4.0, as nearly every DiffSinger voice's vocoder is (they derive from OpenVPI's NSF-HiFiGAN). DawIt never redistributes a vocoder or a voice; the user installs them, and the window shows the vocoder's licence. A maintainer of OpenVPI's vocoders wrote on 4 October 2026 that the licence constrains redistributing and modifying the weights, not the data that flows through them; that is a comment, not legal advice, so anyone releasing music commercially should read the terms.
