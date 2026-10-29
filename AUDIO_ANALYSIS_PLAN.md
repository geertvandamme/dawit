# Audio analysis: implementation plan

The plan for *Explore a song: analyse a WAV or MIDI file* and *Detect lyrics from audio* in [FEATURE_IDEAS.html](FEATURE_IDEAS.html).

DawIt listens to a recording and works out how the song is built:

- its tempo and beats;
- its key and chords;
- its melody and lyrics;
- its structure;
- its style, mood and instruments.

All of it is shown in one **Analyse song** window. From there you can copy the song structure, the chord progression, the melody, the lyrics or the tempo into your own project.

This document covers the design of the window, how each feature is extracted, the libraries and models (with their licences), the code layout, testing, and the order to build it in. It follows the conventions in [CLAUDE.md](CLAUDE.md).

## Contents

1. [Goals and principles](#1-goals-and-principles)
2. [The Analyse song window](#2-the-analyse-song-window)
3. [Copying into your project](#3-copying-into-your-project)
4. [The analysis pipeline](#4-the-analysis-pipeline)
5. [Feature extraction, one by one](#5-feature-extraction-one-by-one)
6. [Libraries and models](#6-libraries-and-models)
7. [Code layout](#7-code-layout)
8. [Data model and caching](#8-data-model-and-caching)
9. [Testing and evaluation](#9-testing-and-evaluation)
10. [Phases](#10-phases)
11. [Risks and open questions](#11-risks-and-open-questions)

---

## 1. Goals and principles

- **Study a song you like with the same tools you use for your own:**
  - its chords on a chord track;
  - its sections on a Sections row;
  - its melody in the piano roll;
  - its lyrics on a Lyrics row;
  - with the guides explaining what was found ("this is the pop progression, I-V-vi-IV").
- **Everything runs on the user's machine.** No audio, lyrics or results leave the computer. Models are downloaded once, after asking, and run on the CPU (and GPU where the library supports it).
- **Suggestions, not verdicts.** Every finding has a confidence, says how sure it is ("A minor, or C major"), and can be corrected in the window before it's copied.
- **Fast first, better later.** Show what's cheap right away (tempo, key, a first structure from the full mix in seconds), then refine as the slower steps finish (stems, chords, melody, lyrics).
- **Plain DSP before machine learning** where it's good enough (tempo, key, structure, a first chord recogniser). Then use small, permissively licensed models where they make the real difference (stems, notes, lyrics, tags).
- **It explains itself.** Each finding links to its guide, and the window says how it was found ("the beats are where the sound gets suddenly louder, mostly the drums").
- **Reuse what's there:**
  - Demucs stems ([Stems.h](src/engine/Stems.h));
  - Basic Pitch notes ([AudioToMidi.h](src/engine/AudioToMidi.h));
  - chord naming and numerals (`Chord`, `Harmony.h`, `ChordGuide::findProgressionName`);
  - sections and structure names (`Section.h`, `StructureGuide::findStructureName`);
  - lyrics with pinned syllables (`Lyrics.h`);
  - chord text (`ChordEditing.h`).

---

## 2. The Analyse song window

### Opening it

- **Right-click an audio clip → *Analyse song...*** in [TrackLanes.cpp](src/ui/TrackLanes.cpp), next to *Split into stems...* and *Convert to MIDI...*.
- **An *Analyse...* button in the wave editor** ([WaveEditor.cpp](src/ui/WaveEditor.cpp)), next to *Reverse*, for the open clip.
- **Later:** *File > Analyse Audio File...* opens a file without adding it to the project. It's added as a clip only when something is copied that needs alignment with it.

The first time, the window lists what it will download, with sizes, and asks before downloading:

- the Demucs model (about 84 MB), unless it's already installed;
- the Whisper model for lyrics, which is optional and can be skipped;
- the tagging model.

Without the optional models, it analyses what it can and shows *Download to find the lyrics* in their place.

One window per clip, through `PluginWindowManager::show (&clip, ...)`. Opening it again brings it to the front. The results are cached (see [8](#8-data-model-and-caching)), so reopening is instant.

### Layout

A resizable window, about 1200×820 by default:

```
┌───────────────────────────────────────────────────────────────────────────────────┐
│ "Song name.wav"  3:42                                         [▶ Play] [Loop ▢]   │
│ A minor · 92 BPM · 4/4 · Pop (indie pop) · melancholic, mid energy                │
│ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░  Finding the chords... (stems done, melody next)  [Cancel]   │
├──────────────┬────────────────────────────────────────────────────────────────────┤
│ Overview     │ 1    5    9    13   17   21   25   29   33   37   41   45   49      │ ← bars
│ Tempo & beat │ ▁▂▃▅▇█▇▅▃▂▁▂▃▅▇█▇▇▅▃▂▁▂▃▅▇█▇▅▃▁                                    │ ← waveform / energy
│ Key & chords │ [Intro][Verse 1      ][Pre ][Chorus      ][Verse 2  ]...           │ ← sections
│ Structure    │ Am  F  C  G  Am  F  C  G  F  G  C  Am ...                          │ ← chords
│ Melody       │ ─▔▔─▁▁─▔─▔▔▔─▁ ...                                                 │ ← melody (mini piano roll)
│ Lyrics       │ I walked a-long the ri-ver late | the ci-ty lights...              │ ← lyrics lines
│ Style & mood │ vocals ████  ███████  ██████                                       │
│ Instruments  │ drums  ▁▁▁▁████████████████                                        │ ← instrument rows
├──────────────┴────────────────────────────────────────────────────────────────────┤
│ (the selected tab: findings, corrections, explanation, links to guides)           │
├───────────────────────────────────────────────────────────────────────────────────┤
│ Copy to my project:  [✓] Tempo  [✓] Structure  [✓] Chords  [ ] Melody  [ ] Lyrics │
│   Where: (•) along this clip   ( ) into my song from bar [ 1 ] in my key (C maj)  │
│   [Copy chords as text]  [Copy structure as text]              [Copy to project]  │
└───────────────────────────────────────────────────────────────────────────────────┘
```

- **Header:**
  - the file name and length;
  - **the summary line**: key, tempo, metre, style and mood, each clickable to its tab;
  - a progress bar with the current step and a *Cancel* button.
- **The timeline** shows every finding over the song on one time axis, drawn the way DawIt draws them elsewhere:
  - bars from the detected downbeats;
  - the waveform with the energy curve;
  - sections in `SectionLane::getColour`;
  - chords in `ChordLane::getColour` with their names;
  - the melody as a mini piano roll;
  - the lyrics' lines;
  - a row per instrument showing where it plays.
- **Timeline interaction:**
  - click to play from there and drag to loop;
  - zoom with Cmd + scroll wheel, like the arrangement;
  - hover for what's at that moment ("Bar 17 · Chorus · F · 'hold on, hold on'");
  - while it plays, the playhead runs and the current chord, syllable and section light up.
- **Rows fill in as each step finishes.** A row still being worked on shows a soft striped placeholder with its step name.
- **The tabs on the left:** Overview, Tempo & beat, Key & chords, Structure, Melody, Lyrics, Style & mood, Instruments. Each tab shows:
  - its **findings** in words, with the confidence;
  - its **corrections** (see below);
  - **how it was found**, in two or three sentences for a beginner;
  - **links to the guides**: the chord progression guide, the circle of fifths, song structure, rhythm, writing lyrics.
- **The Overview tab** reads like a guide's "your song" topic:

  > "This is the pop progression (I-V-vi-IV) in A minor at 92 BPM. The chorus first comes at 0:48, after a 4-bar pre-chorus that builds. The bridge borrows F major's IV. The vocal range is A3 to E5."

  The local LLM can rewrite it more freely when Ollama is running; without it, the text comes from templates.
- **The copy panel** at the bottom (see [3](#3-copying-into-your-project)).

### Correcting what it found

Each correction updates the timeline and everything that depends on it. For example, moving the downbeat re-snaps the sections and chords to the new bars.

| Tab | Corrections |
|---|---|
| Tempo & beat | Halve or double the BPM (the most common mistake), tap the tempo, move the downbeat (*bar 1 starts here*, or nudge by a beat), and change the metre (4/4, 3/4, 6/8; DawIt itself is 4/4 for now) |
| Key & chords | Pick another key (the runner-up is offered first). Click a chord block for the same chord menu as the chord track. Merge or split blocks, and *Simplify* (triads only, one chord per bar) |
| Structure | Drag section edges (they snap to bars), change a section's type or name, merge or split sections, and *Label as chorus* for a repeat group |
| Melody | *Use the vocals* or *Use the lead instrument*, transpose by an octave, quantise to 1/8 or 1/16, and delete stray notes |
| Lyrics | Edit the text in the lyrics editor (`LyricsEditor`, with timing kept for unchanged words), choose the language, and *Find the words again* with the larger model |
| Style & mood | Pick from the ranked list; this only affects the template and generator settings that get copied |

The corrections are saved with the cached analysis.

---

## 3. Copying into your project

The copy panel has a checkbox per kind of finding, a choice of **where** it goes, and what happens on a conflict.

### What can be copied

| Finding | Goes to | How |
|---|---|---|
| **Tempo (BPM)** | the project tempo | Sets `Engine` tempo to the rounded BPM (or exact, if the clip is to stay in time), and moves the clip so its first downbeat lands on a bar line |
| **Key** | the song key | Sets the key, asking first as *Put on chord track* does when it differs |
| **Song structure** | the Sections row | `Section`s with their types and names, snapped to bars |
| **Chord progression** | the chord track | A `ChordMap` per bar (or half bar later), for the whole song or only the selected sections |
| **Melody** | a new MIDI track | A `MidiClip` with the melody's notes, named "Melody (from Song name)", with the DawIt Synth lead sound by default |
| **Lyrics** | the Lyrics row | `Lyrics` with each word or syllable **pinned** at its sung beat, headed by section (`[Chorus]`) |
| **Bass line** (extra) | a new MIDI track | From the bass stem's notes, like the melody |
| **Style** | the generator and template | Sets the Generate panel's style and offers the matching song template (later) |

### Where it goes

1. **Along this clip:** time is kept, so everything lines up with the recording on the timeline. This is the mode for studying and playing along.
   - Tempo first: the project tempo is set to the song's, so beats and bars match the recording.
   - The clip is shifted so its first downbeat is on a bar line. If the song has a pickup, bar 1 starts at the first downbeat and the pickup sits before it.
   - Sections, chords, melody and lyrics are written at the bars where they happen.
   - Converting seconds to beats uses the detected beats, not one constant tempo, so a band that drifts still lines up beat by beat. With *Follow the recording* in *Detect tempo*, the song has a tempo map that follows the band bar by bar (see [TEMPO_MAP_PLAN.md](TEMPO_MAP_PLAN.md)), and the finders place notes, syllables and bars through it.
2. **Into my song, from bar *N*:** the material is reused, not the recording. This is the mode for "I want this structure and these chords in my own song".
   - It's placed at the project's tempo, from bar *N*. Only the bar positions matter, not the seconds.
   - **Transposed to my key** (optional): chords move by the interval between the two keys and keep their numerals. A minor song into a major key uses its relative minor's numerals or keeps the quality, depending on a toggle.
   - The melody is transposed the same way and quantised to the chosen grid.
   - Lyrics keep their beat positions relative to their sections.
   - Optionally **only the selected sections**, such as one chorus's progression or one verse's melody.

### Conflicts and safety

- Anything that would replace existing chords, sections or lyrics asks first, listing what changes ("Replace the chords of bars 1-32?"), because DawIt has no undo yet.
- Copying never changes an existing track's instrument (see CLAUDE.md). The melody and bass always go on **new** tracks.
- **Copy as text**, for the clipboard:
  - chords as `| Am | F | C | G |`, which pastes onto the chord track through `ChordEditing.h`'s text parser;
  - the structure as `Intro 4 · Verse 8 · Chorus 8 ...`;
  - the lyrics in the lyrics editor's format, with headings and hyphens.

### A note on copyright, shown once in the window

> Chord progressions, song structures and tempos aren't owned by anyone, so use them freely. Melodies and lyrics belong to their writers: copy them to study and practise, but don't release them as your own.

---

## 4. The analysis pipeline

The analysis runs on a background thread (`juce::Thread`) owned by the window. It reports its progress per step and can be cancelled between and inside steps; the existing `ProgressFn` returns false to stop. Results are posted to the message thread as each step finishes. The edit is never touched until the user copies something.

```
audio clip ──► decode + resample ─┬─► (A) full-mix features ──► tempo, beats, downbeats ─┐
                                  │                          ──► key (rough)             │
                                  │                          ──► structure (rough)       │
                                  │                          ──► style / mood / instr. ◄─┤ (tags on full mix)
                                  │                                                      │
                                  └─► (B) stems (Demucs) ──┬─► drums  ──► refine beats ◄─┘
                                                            ├─► bass   ──┐
                                                            ├─► other  ──┴─► notes (Basic Pitch) ──► chords, key
                                                            └─► vocals ──┬─► melody (Basic Pitch / pitch tracker)
                                                                         ├─► lyrics (Whisper) ──► words → syllables → beats
                                                                         └─► vocal presence ──► structure (refine)
                                                                                              ──► summary + explanations
```

| # | Step | Input | Output | Typical time for a 4-minute song |
|---|---|---|---|---|
| 1 | Decode and resample | the clip's audio | 44.1 kHz stereo (Demucs), 22.05 kHz mono (features, Basic Pitch), 16 kHz mono (Whisper, tags) | under a second |
| 2 | Spectral features | 22.05 kHz mono | STFT, onset strength, log-mel, chroma, loudness per frame | 1-2 s |
| 3 | Tempo, beats, downbeats | onset strength (later the drums stem) | BPM, beat times, downbeats, metre, swing | 1 s |
| 4 | Rough key and structure | beat-synchronous chroma and timbre | key, sections A/B/C | 1 s |
| 5 | Tags | 16 kHz mono, full mix and stems | style, mood, instruments over time | 2-5 s |
| 6 | Stems | 44.1 kHz stereo | vocals, drums, bass, other | 1-3 min on the CPU (pieces in parallel, as now) |
| 7 | Notes | bass and other stems, vocals | Basic Pitch notes per stem | 20-40 s |
| 8 | Chords and key | notes and chroma per beat | chord per beat, then per bar, and the key | 1 s |
| 9 | Melody | vocal-stem notes or pitch | monophonic notes | 1 s |
| 10 | Lyrics | vocal stem, 16 kHz | words with times, then syllables on beats | 10 s to 2 min, depending on the model |
| 11 | Refine the structure | 4 and 8, vocal presence, lyrics repeats | named sections | 1 s |
| 12 | Summary | everything | the Overview text and the summary line | instant (or seconds with the LLM) |

The times are rough guesses to be measured. Steps 1-5 give a useful first picture in seconds. If the user already split this clip into stems (the stem tracks exist and come from this clip), step 6 reuses them instead of running Demucs again.

---

## 5. Feature extraction, one by one

All DSP runs on `juce::dsp::FFT`, with a Hann window. Frame settings are shared: 22.05 kHz, 2048-sample FFT, hop 512 (about 23 ms), unless noted.

### 5.1 Tempo, beats, downbeats, metre (BPM)

> **Built as *Detect tempo...*** on an audio clip (`src/analysis/TempoFinder`, `src/ui/TempoActions`), steps 1 to 4 for 4/4: spectral flux in 40 bands at 172 frames a second, the autocorrelation averaged over 8-second windows of slightly smoothed onsets (so a drifting tempo doesn't lose to the hi-hat's eighths), a one-octave prior around 120 BPM plus the two-beat lag, Ellis' dynamic programming for the beats with stray beats at the ends dropped, the tempo from a straight line through all the beats, drift from the tempo over 16 beats (10th to 90th percentile), and the downbeat as the beat of four with the most chord change and low-end onset. A drifting recording can be followed with a tempo per bar, measured from downbeat to downbeat ([TEMPO_MAP_PLAN.md](TEMPO_MAP_PLAN.md)). Not yet: 3/4 and 6/8, swing, the drums stem.

**First version: plain DSP.** This is the classic, well-documented pipeline:

1. **Onset strength (novelty):**
   - log-compressed mel spectrogram (about 80 bands);
   - half-wave rectified spectral flux (the positive difference between frames), summed over the bands;
   - a local mean removed, then normalised.
2. **Tempo:**
   - the autocorrelation of the onset strength over 4-8 s windows, or a Fourier tempogram;
   - weighted with a log-normal prior centred near 120 BPM (as in Ellis 2007 and librosa), so 60/120/240 ambiguities lean towards the usual range;
   - the global BPM is the peak of the averaged tempogram, with its confidence;
   - the local tempo over time shows drift.
3. **Beats:** dynamic-programming beat tracking (Ellis 2007), which picks beat times that land on strong onsets while keeping spacing close to the tempo period. It's simple and robust, about 100 lines.
4. **Downbeats and metre:**
   - beat-synchronous features: bass-band onset strength, chroma change (chords usually change on the downbeat) and loudness accent;
   - for each candidate metre (3 or 4 beats a bar) and phase, score how well downbeat positions line up with high chord change and low-frequency onsets, and pick the best;
   - 6/8 is a 3-grouping at a dotted-quarter tempo, decided from the subdivision pattern.
5. **Swing:** the ratio of onset energy at the 2/3 position against the 1/2 position between beats, from the drums stem when available, gives straight or swing and how much.
6. **Refine with the drums stem** once stems exist: run 1-3 again on the drums stem, which is cleaner, and keep whichever beat sequence scores better.

**Later, machine learning:** *Beat This!* (CPJKU, ISMIR 2024).

- Code and weights are MIT. There are small checkpoints (about 8 MB) and full ones (about 78 MB).
- It's accurate without DBN post-processing, so no madmom (whose models are non-commercial).
- Export the PyTorch model to ONNX offline with a script in `tools/` and run it in ONNX Runtime, as Basic Pitch is. It outputs frame-wise beat and downbeat activations; peak-pick them, then snap to a constant-tempo grid when the tempo is steady.
- Its README notes that some training data has restrictive licences, so check that before shipping.

**Shown in the window:**

- the BPM with *halve* / *double* buttons;
- the tempo-over-time curve, flat for a click track, wavy for a live band;
- the beat ticks on the waveform;
- the metre and the swing;
- *How it was found*: "Beats are where the sound gets suddenly louder, mostly the drums. The tempo is how often that repeats."

### 5.2 Key

> **Built, simply, inside *Detect chords...***: Krumhansl-Kessler profiles on the bars' chroma, with the first chord and the share of tonic chords deciding between relative major and minor. Not yet: a tuning estimate, other profiles, the confidence and runner-up, key changes.

1. **Chroma** (12 pitch classes per frame):
   - first version: constant-Q-like log-frequency bins folded to pitch classes from the STFT, with a tuning estimate (the median deviation of strong peaks from 440 Hz) so detuned recordings still map right;
   - from stems: chroma from the bass and other stems, leaving out drums and vocals;
   - best: pitch-class histograms from the **Basic Pitch notes** of those stems, weighted by duration and velocity.
2. **Key profiles:** correlate the song's pitch-class distribution with the 24 rotated major and minor profiles (Krumhansl-Kessler, Temperley or Albrecht-Shanahan; try all three in the evaluation and keep the best) and rank the keys.
3. **Use the chords too:** once chords exist, rescore the top 3 keys by how many chords are diatonic and whether the tonic chord starts and ends sections. This sorts out relative major and minor ("A minor, or C major").
4. **Key changes:** run the same scoring per section, and report a modulation when a section's best key differs with high confidence ("the last chorus is a step up, in B minor").

**Shown in the window:**

- the key with its runner-up and the confidence;
- the pitch-class profile drawn as a 12-bar chart over the profile shape;
- a link to the circle of fifths for this key.

### 5.3 Chords

> **Built as *Detect chords...*** on an audio clip (`src/analysis/ChordFinder`, `src/ui/ChordActions`), on the song's bars: chroma from a 4096-point STFT at 11 kHz (harmony 65 Hz to 2.1 kHz, bass 40 to 250 Hz) with a 9-frame median against drums, frames near bar lines left out, each bar's noise floor (the median of its 12 levels) taken off, cosine against major, minor, 7, maj7 and m7 templates with a bass-root bonus and a small seventh penalty, the key found, then a Viterbi pass with bonuses for chords of the key and for a chord that goes on. It checks the recording's tempo and first downbeat against the song's bars and advises *Detect tempo* first when they don't line up. Not yet: sus, dim, aug and slash chords, two chords in a bar, the stems, *N.C.* beyond silent bars, progression names in the dialog.

**First version: notes plus templates plus HMM smoothing.** This is fully under our control and has no licence questions.

1. **Per beat:** combine
   - bass notes (the lowest pitch class, from bass-stem notes, or the lowest strong chroma bin);
   - harmony notes (the duration-weighted pitch classes from other-stem and bass-stem notes, or chroma when notes are sparse).
2. **Templates:** score each chord DawIt knows (`Chord` qualities: triads, sevenths, sus, power chords) on each root:
   - cosine similarity between the beat's pitch-class vector and the chord template;
   - a bonus when the bass note is the root (or a known inversion, giving a slash chord later);
   - a penalty for chord tones that are missing.
3. **Smoothing with a Viterbi pass (HMM):**
   - states: chords;
   - emission: the template scores;
   - transitions: a strong self-transition, so chords last; higher probability for diatonic chords in the detected key and for common moves (`Harmony.h` already knows the usual next chords); a change on a downbeat is more likely than mid-bar.
4. **To bars:** merge the beats into one chord per bar, or two per bar when the second half is a confident different chord. Mark *N.C.* where nothing scores.
5. **Name and explain:**
   - `Chord::getName`, numerals from `Harmony.h`;
   - `findProgressionName` for each section's loop ("I-V-vi-IV, the pop progression");
   - the chords' families coloured as in the chord guide.

**Later, machine learning:** a chord recognition model exported to ONNX.

- Candidates: **BTC** (Bi-directional Transformer for Chord recognition, ISMIR 2019, MIT code, PyTorch, major/minor and large-vocabulary versions; check the weights' licence and training data) and **crema** (McFee, BSD-2-Clause, structured large-vocabulary chord model).
- Run it on the stems mix without drums and vocals, then use the same Viterbi and bar steps. Keep the template version as the fallback and for the tests.

**Shown in the window:**

- the chord row, with numerals under the names;
- the per-section progressions with their names;
- the less certain chords dimmed, and a click on one plays the chord.

### 5.4 Melody

1. **Source:** the vocal stem by default. Without vocals (a silent stem, as `isSilent` already finds), use the **lead instrument**: the other stem's notes in the top register (keep the highest note at each moment above a pitch floor). The user can switch between them.
2. **Notes:**
   - **First version:** Basic Pitch on the vocal stem (already built in), then `keepOneAtATime` to make it monophonic. It's already tuned for this.
   - **Better pitch:** run a monophonic pitch tracker for an accurate pitch curve with vibrato and slides, then segment it into notes:
     - **pYIN**, plain DSP (Mauch and Dixon 2014): YIN candidates with probabilities, then an HMM for the voiced pitch track. About 300 lines, no licence questions.
     - **CREPE**, as the ML option: 16 kHz, 10 ms hop, pitch and voicing confidence. Code is MIT; check the weights' licence. Export to ONNX, using the tiny or small capacity for speed.
   - **Note segmentation:** group stable runs of the pitch curve (within about ±50 cents) into notes, split at onsets (the vocal stem's spectral flux) and at voicing gaps, and round to the nearest semitone after tuning compensation. *Tony* (Mauch et al. 2015) describes this well.
3. **On the grid:** convert note times to beats with the detected beats (see 3), and quantise softly. Snap to 1/16 by default, keep the original timing when off-grid by more than 30%, and offer 1/8 / 1/16 / off.
4. **Range and info:**
   - the lowest and highest notes, so you know if you can sing it;
   - the melody's notes against the key (out-of-key notes marked);
   - the hook (the most repeated phrase in the choruses).

**Shown in the window:**

- a mini piano roll under the chords, with the pitch curve drawn faintly behind the notes;
- *Play melody* (the notes on the DawIt Synth) mixed against the vocal stem, to check them by ear.

### 5.5 Lyrics

> **Built, for English vocals only for now:** *Detect lyrics (English only)...* in an audio clip's right-click menu runs this pipeline on its own, ahead of the window (`src/analysis/LyricsFinder`, `src/ui/LyricsActions`). It uses the `.en` models (small or medium) with the language fixed to English, and the English syllabifier `lyrics::splitEnglishWord`, so other languages come out as English words that sound alike. Choices made while building it: whisper.cpp 1.9.5; DTW word times (with flash attention off) moved 0.1 s earlier, since on speech they land about that long after the word starts, and the first word of a phrase starting where the singing does; a word's end cut to the singing it starts in (Whisper stretches a phrase's last word over the pause); the singing found from the level, 30 dB below the loudest; syllables share their word's time evenly; and the project tempo instead of detected beats. Other languages, forced alignment and the window's lyrics tab are still to come (see the idea's status).

1. **Transcription with whisper.cpp** (MIT; the Whisper weights are MIT):
   - run on the **vocal stem** at 16 kHz (much better than on the full mix);
   - with `token_timestamps` and `split_on_word` / `max_len = 1` for word-level times (experimental in whisper.cpp; the DTW token timestamps option improves them where available);
   - language auto-detected, or chosen by the user.
2. **Model choice:**

   | Model | Size on disk |
   |---|---|
   | tiny | 75 MiB |
   | base | 142 MiB |
   | small | 466 MiB |
   | medium | 1.5 GiB |

   Default to **small**, which is a good balance for singing, with *Find the words again with a larger model* to download medium. Use the English-only `.en` variants when the language is English. Use Metal and Core ML on macOS (`WHISPER_COREML`, the encoder on the Neural Engine) for speed.
3. **Clean up the words:**
   - leave out non-sung segments (Whisper hallucinates on long instrumental parts), keeping only words where the vocal stem has energy;
   - drop repeated hallucinated phrases ("Thanks for watching") by a blocklist and by checking against the vocal activity;
   - sentence case, with no punctuation at line ends.
4. **Lines and verses:**
   - a new line at gaps in the singing longer than about 0.4 s, and at section boundaries;
   - a new verse at each section;
   - headings from the section names (`[Chorus]`).
5. **Syllables:**
   - split words into sung syllables with a simple English syllabifier (vowel groups, with rules for silent *e*, *-le*, common prefixes and suffixes), the same counting the lyrics guide uses;
   - spread each word's time across its syllables by the vowel onsets in the vocal stem (or evenly);
   - unsure words stay whole (one block).
6. **To DawIt's lyrics:**
   - each word or syllable becomes a **pinned** `LyricSyllable` at its beat (converted with the detected beats), so the lyrics track shows the real timing and `lyrics::spread` leaves it alone;
   - the text goes through `lyrics::toText` for the editor and *Copy as text*.
7. **Later:** better word timing by forced alignment of the corrected text to the vocal stem (re-run Whisper with the corrected text as the prompt, or a CTC aligner with a permissive licence). Check licences; Meta's MMS aligner weights are non-commercial.

**Shown in the window:**

- the lyrics row with the lines, and the text in the tab with the sung word lit while playing;
- *Edit*, which opens the lyrics editor;
- the language;
- a link to the lyrics guide (rhyme scheme and syllables of each verse, through `lyricsguide::analyse`).

### 5.6 Song structure

**First version: self-similarity plus rules.** *Built as* Detect song structure... *(`StructureFinder`): per-bar features (chroma, a 16-band balance, loudness, the share of short sounds), novelty from comparing bars a phrase apart, cuts by dynamic programming at the novelty peaks with 4, 8 and 16 bars preferred, greedy grouping, and the naming rules below without vocals or lyrics.* This is plain DSP and is understood well (Foote 2000; Serrà et al. 2014; McFee and Ellis 2014).

1. **Beat-synchronous features:** average per beat
   - chroma (harmony);
   - MFCC or log-mel (timbre);
   - loudness (energy);
   - vocal presence (from the vocal stem's energy, once stems exist).
2. **Self-similarity matrices** for chroma and for timbre, smoothed along the diagonals (repeats show up as stripes), with lag-based enhancement for repeated chord sequences.
3. **Boundaries:**
   - a novelty curve (a checkerboard kernel along the diagonal) of the combined matrix, with peaks as boundary candidates;
   - **snapped to bars**, preferring 4- and 8-bar phrases (pop sections are almost always multiples of 4 bars);
   - extra boundaries where vocal presence starts or stops and where the chord loop restarts.
4. **Grouping:** cluster segments by similarity (chroma repetition first) into groups A, B, C... (spectral clustering over the segment similarity, or agglomerative with a threshold).
5. **Naming:** score each group for each `SectionType` by:
   - **repetition**: the most repeated group with high energy is the chorus;
   - **energy**: choruses and drops are the loudest, verses lower, breakdowns lowest;
   - **vocals**: verse and chorus have vocals, intro, interlude and solo don't (a solo has a lead instrument);
   - **lyrics**: the same words each time means a chorus, new words each time means a verse;
   - **position**: the first group without vocals is the intro, the last is the outro, a short rising part before the chorus is a pre-chorus, and a once-only part after the second chorus is a bridge;
   - **the style**: EDM gets build-up and drop instead of pre-chorus and chorus.

   Then pick the best assignment and number the repeats (Verse 2) the way `Section` does.
6. **Explain:** `findStructureName` ("verse-chorus with a bridge"), the energy curve as in the structure guide, and how each label was chosen ("Chorus: comes back 3 times, the loudest part, the same words each time").

**Later, machine learning:** **All-In-One** (`allin1`, MIT code).

- It does beats, downbeats, tempo and functional section labels (intro, verse, chorus, bridge, inst, solo, break, outro) in one model from Demucs stems, which DawIt already has.
- It needs NATTEN (neighbourhood attention), so ONNX export may need work. The weights' licence isn't stated, and it's trained on the Harmonix Set.
- It's worth evaluating against the rules version once the pipeline exists.

### 5.7 Style, mood and instruments

1. **Tags with a small audio-tagging model: YAMNet** (Google, Apache 2.0):
   - about 3.7 M parameters, 521 AudioSet classes, 16 kHz input, 0.96 s frames;
   - it gives scores per frame and a 1024-dimensional embedding;
   - the AudioSet ontology includes
     - **genres** (pop, rock, hip hop, electronic, jazz, classical, country, reggae, funk, soul, metal, blues, folk...);
     - **instruments** (guitar, electric and acoustic, bass guitar, piano, organ, synthesizer, drum kit, strings, brass, singing, choir...);
     - **moods** (happy music, sad music, tender, exciting, angry, scary music).
   - Convert it to ONNX offline (tf2onnx) with a script in `tools/`, and download it on first use (pinned URL and hash) or build it in.
2. **Instruments over time:**
   - run YAMNet on each stem as well as the mix; the stem already says vocals, drums or bass, and YAMNet names what's in *other* (piano, guitar, synth, strings);
   - smooth the scores over a few seconds, then threshold them into presence bars per instrument;
   - add vocal range (male or female) from the melody's range.
3. **Style:**
   - average the genre scores over the song, then combine them with the musical features in a small hand-made scorer: tempo range, swing, drum density, distortion (from the tags), chord vocabulary (sevenths for jazz and soul, power chords for rock), and the structure (drops for EDM);
   - rank the top 3 with confidence, and map them to DawIt's style names (the Generate panel's styles, the song templates, `ChordStyles.json`'s styles).
4. **Mood:** two scales,
   - **valence** (sad to happy): major against minor, mode mixture, mood tags, brightness;
   - **arousal** (calm to energetic): tempo, loudness, onset density, mood tags;

   placed on a 2D chart, with a few words ("melancholic, mid energy") per song and per section.
5. **Later:** **CLAP** (LAION, CC0 code; the music checkpoints exist but their licence and the training data's licences need checking) for zero-shot tags from words ("a sad piano ballad", "lo-fi hip hop beat"), which gives much richer styles than AudioSet's list. **Avoid Essentia's models:** the library is AGPL and its pretrained models are CC BY-NC-SA, so they can't be used commercially.

**Shown in the window:**

- the ranked styles with what points to each ("92 BPM, swung hi-hats, sevenths, Rhodes piano → neo-soul");
- the mood chart;
- the instrument rows on the timeline, with a click on a row to solo that stem.

### 5.8 Energy and the summary

- **Loudness over time:** short-term LUFS (BS.1770; this shares code with the *LUFS explained* idea), onset density, and the brightness (spectral centroid). Shown as the energy curve under the sections, as in the structure guide.
- **The summary** (Overview tab and header line) is assembled from all findings with templates. When Ollama runs, the findings go to the local LLM as JSON, asking for a short friendly explanation, which is shown with *written by the local AI* under it.

---

## 6. Libraries and models

| Purpose | Library / model | Licence | Status | How it's used |
|---|---|---|---|---|
| FFT, filters, threads | JUCE 8 (`juce_dsp`) | AGPLv3 / commercial | have | all DSP features |
| Stems | demucs.cpp + Eigen, Demucs v4 htdemucs weights | MIT, MPL 2.0, MIT | have | vocals, drums, bass and other stems |
| Notes | Basic Pitch on ONNX Runtime | Apache 2.0, MIT | have | notes for chords, key and melody |
| Tempo, beats, key, structure, first chords, pYIN | own code (Ellis 2007, Foote 2000, Krumhansl, pYIN) | ours | new | phase 1-2 |
| Lyrics | **whisper.cpp** (ggml) + Whisper weights | MIT, MIT | new | FetchContent at a pinned commit and hash, static library, Metal and Core ML on macOS; the model is downloaded on first use |
| Tags | **YAMNet** → ONNX | Apache 2.0 | new | style, mood, instruments; about 15 MB as ONNX (to measure) |
| Beats (later) | Beat This! → ONNX | MIT (code and weights) | later | check the training data note |
| Chords (later) | BTC or crema → ONNX | MIT / BSD-2-Clause (code) | later | check the weights' licence and data |
| Pitch (later) | CREPE → ONNX | MIT (code) | later | check the weights' licence |
| Structure (later) | All-In-One | MIT (code) | later | needs NATTEN; the weights' licence isn't stated |
| Zero-shot tags (later) | LAION-CLAP | CC0 (code) | later | the checkpoints' and data licences aren't stated |

**Not to use:**

- **Essentia**: AGPL, and its models are CC BY-NC-SA.
- **madmom's models**: CC BY-NC-SA.
- **Chordino / NNLS Chroma, aubio**: GPL.
- **Meta MMS aligner weights**: CC BY-NC.

These would rule out a commercial JUCE licence later, or forbid commercial use outright.

**Rules for adding them**, as in CLAUDE.md:

- pinned URL and SHA-256;
- the licence checked for both the code and the weights (and the training data where it's stated);
- named in the README's *License note*;
- large models downloaded on first use after asking, into `~/Library/Application Support/DawIt/Models`, with the size shown;
- conversion scripts (PyTorch or TensorFlow to ONNX) in `tools/`, run by developers only and never by the app;
- the converted files hosted somewhere stable (the project's own release assets) with their hashes.

---

## 7. Code layout

A new `src/analysis/` layer: engine-level code with no UI, tested in `DawItTests`.

```
src/analysis/
  SongAnalysis.h          the result: tempo map, beats, key, chords, melody, lyrics, sections, tags, energy,
                          each with confidence; JSON to and from (for the cache); corrections applied
  AnalysisJob.{h,cpp}     the pipeline on a background thread: steps, progress, cancel, partial results,
                          reusing stems, the cache
  Features.{h,cpp}        resampling, STFT, log-mel, chroma (with tuning), onset strength, MFCC, loudness,
                          beat-synchronous averaging
  Tempo.{h,cpp}           tempogram, BPM with prior, dynamic-programming beats, downbeats and metre, swing
  KeyFinder.{h,cpp}       key profiles, ranking, per-section keys
  ChordFinder.{h,cpp}     templates, Viterbi smoothing, beats to bars, slash chords (later)
  Melody.{h,cpp}          lead source choice, Basic Pitch notes to a monophonic melody, pYIN, note segmentation
  LyricsFinder.{h,cpp}    whisper.cpp wrapper, cleaning, lines and verses, syllabifier, words to pinned syllables
  Structure.{h,cpp}       self-similarity, novelty, bar snapping, grouping, naming
  Tagger.{h,cpp}          YAMNet on ONNX Runtime: tags per frame, style and mood scoring, instrument presence
  Summary.{h,cpp}         the Overview text from templates; the prompt for the local LLM
  CopyToProject.{h,cpp}   turning findings into Sections, ChordMap, MidiClip, Lyrics and tempo, aligned or
                          transposed, with the conflicts it would cause listed first
src/ui/
  AnalysisWindow.{h,cpp}  the window: header, timeline rows, tabs, copy panel
  AnalysisTimeline.{h,cpp} the rows over time, playback, zoom, hover
  AnalysisActions.{h,cpp} the menu items, the model download question, opening the window per clip
tools/
  export_yamnet_onnx.py, export_beat_this_onnx.py, ...   developer-only conversions
  eval/                   developer-only evaluation against annotated datasets (mir_eval)
```

- **Model files** go in the same folder as the Demucs model, with one `isModelInstalled` / `downloadModel` pair per model, following `Stems.h`.
- **Playback in the window** uses a small `AudioTransportSource` on the clip's audio (and the stems for soloing), mixed into the engine's output through a new preview source (the engine's `previewNote` only plays notes today). The project's own transport isn't touched.
- **CMake:** the `src/analysis/*.cpp` files go into both `DawIt` and `DawItTests`, plus a `whisper` static library target (with `WHISPER_BUILD_EXAMPLES` and `WHISPER_BUILD_TESTS` off, and Metal and Core ML on for Apple).

---

## 8. Data model and caching

```cpp
struct SongAnalysis
{
    juce::String sourceHash;                 // SHA-256 of the audio data, the cache key
    double durationSeconds = 0.0;

    struct Beat { double seconds; int beatInBar; };          // 1 = downbeat
    std::vector<Beat> beats;
    double bpm = 0.0, bpmConfidence = 0.0;
    int beatsPerBar = 4;
    double swing = 0.0;                                      // 0 straight .. 1 full triplet swing

    MusicalKey key; std::optional<MusicalKey> runnerUpKey; double keyConfidence = 0.0;

    struct TimedChord { double startSeconds, endSeconds; Chord chord; double confidence; };
    std::vector<TimedChord> chords;                          // per beat or bar, merged

    struct Note { double startSeconds, endSeconds; int pitch; float confidence; };
    std::vector<Note> melody;
    juce::String melodySource;                               // "vocals" or "lead"

    struct Word { juce::String text; double startSeconds, endSeconds; float confidence; };
    std::vector<Word> words;                                 // as transcribed
    juce::String language;

    struct TimedSection { double startSeconds, endSeconds; SectionType type; juce::String name; double confidence; };
    std::vector<TimedSection> sections;

    struct Tag { juce::String name; float score; };
    std::vector<Tag> styles, moods;
    struct Presence { juce::String instrument; std::vector<std::pair<double, double>> ranges; };
    std::vector<Presence> instruments;
    double valence = 0.0, arousal = 0.0;

    std::vector<float> energy;                               // short-term loudness per beat
    juce::var corrections;                                   // the user's edits, applied on load
};
```

- **Times are stored in seconds**, the truth of the recording. Conversion to beats happens in `CopyToProject` through the beat list, so a corrected downbeat or tempo re-maps everything without analysing again.
- **The cache** is `~/Library/Application Support/DawIt/Analysis/<sourceHash>.json`, written after each step so a cancelled run resumes. It's versioned (`"version": 1`), and an older version is analysed again. Stems are cached as FLAC next to it (optional and size-capped), so *Find the words again* doesn't separate again.
- **The project** doesn't store the analysis; it stays with the audio file's hash. Copied results become normal project data (sections, chords, tracks, lyrics) saved as always.

---

## 9. Testing and evaluation

**Headless tests in `tests/EngineTests.cpp`** (`runAnalysisTests()`), on synthetic audio rendered by the engine or generated in the test. No model downloads needed, except the optional flags.

- **Tempo:**
  - clicks at 92, 120 and 140 BPM give the BPM within ±1;
  - beats within 20 ms;
  - downbeats from accented clicks;
  - a 3/4 waltz pattern gives 3 beats a bar;
  - a swung hi-hat pattern gives swing.
- **Key:**
  - a C major scale and I-IV-V-I in DawIt Synth piano give C major;
  - the same in A minor (with G#) gives A minor;
  - a detuned (+30 cents) version still gives the key.
- **Chords:** the DawIt Synth playing the pop progression in several keys (block chords and arpeggios) gives the chords per bar, with at least 90% bars correct on clean synthetic audio, and `findProgressionName` names it.
- **Melody:** a sine melody, and a synth lead, gives the notes back with the pitches exact and the onsets within 30 ms.
- **Structure:**
  - a synthetic song (intro A 4 bars, B 8, C 8, B 8, C 8, outro) built from different chord loops and loudness gives the boundaries on the right bars;
  - the most repeated loud group is named chorus.
- **Lyrics:**
  - the syllabifier on a word list;
  - words to pinned syllables on beats;
  - the clean-up rules (blocklist, vocal activity gating);
  - the real model only with `--whisper <model file>`, on a short recording of TTS or a sung example kept in the repo with a free licence.
- **Copying:**
  - aligned and transposed copies of sections, chords, melody and lyrics land on the expected bars and pitches;
  - conflicts are listed;
  - an existing track's instrument is never changed.
- **The cache:** save, load and correct, then re-map.
- **The tagger:** YAMNet runs on a synthetic drum loop and a sine, with the expected top classes (only with `--yamnet <model>`).

**Evaluation (developer-only, `tools/eval/`):** Python scripts with `mir_eval` against annotated datasets with permissive licences for research use (Harmonix Set annotations for beats and sections, Isophonics annotations for chords, and others after checking their terms). They run the app's analysis through a small command-line front end (`DawItTests --analyse <file> --json`). They track:

- beat F-measure;
- tempo accuracy (Acc1 / Acc2);
- chord weighted overlap (majmin and sevenths);
- segment boundary hit rate (±3 s) and pairwise label F-measure;
- melody overall accuracy;
- word error rate for lyrics.

Record the numbers in this document as the work goes on, so ML options are chosen on evidence.

---

## 10. Phases

Each phase ends usable, with tests, README, FEATURE_IDEAS status and website updates (see CLAUDE.md).

| Phase | What | Copy to project | New libraries |
|---|---|---|---|
| **1. Window and the fast picture** | `SongAnalysis`, `AnalysisJob`, features, tempo, beats, downbeats, key (chroma), structure (rules, no vocals yet), the window with the timeline, tabs, corrections and the cache. **Tempo, beats and downbeats built** as *Detect tempo...*; **structure built** (rules on bar features, no vocals yet) as *Detect song structure...* | tempo, key, structure | none |
| **2. Stems and chords** | reuse or run Demucs, Basic Pitch notes on bass and other, the chord finder with Viterbi, key refined by chords, structure refined by the chord loops and vocal presence, progression names, *Copy chords as text*. **Chords (from chroma) and key built** as *Detect chords...* | chord progression (aligned or transposed) | none |
| **3. Melody** | vocal or lead melody from Basic Pitch notes, note clean-up, range, mini piano roll, play melody | melody (and bass line) to new tracks | none |
| **4. Lyrics** | whisper.cpp, model download, words, clean-up, syllables, lines and verses, edit, the lyrics guide link. **English only built** as *Detect lyrics (English only)...*; other languages and the window's tab to come | lyrics with timing | whisper.cpp |
| **5. Style, mood, instruments** | YAMNet on mix and stems, style and mood scoring, instrument rows, the summary (templates and local LLM) | style to the generator and template | YAMNet (ONNX) |
| **6. Better models** | evaluate Beat This!, BTC or crema, CREPE or pYIN, All-In-One and CLAP against phase 1-5, and switch where clearly better and the licence is clear (the tempo map for drifting songs is built) | exact alignment with a tempo map | per evaluation |

Phase 1 alone already gives most of the "study a song" value for structure and tempo. Phase 2 gives chords, the most asked-for finding.

---

## 11. Risks and open questions

- **Chord accuracy on real mixes:** template matching on Basic Pitch notes from stems should do well on pop and rock triads, and worse on jazz voicings, distorted guitars and dense mixes. The confidence display and easy correction matter more than perfect numbers. Measure it before deciding on ML.
- **Whisper on singing** is good but not perfect, and hallucinates in instrumental parts. Run it on the vocal stem only, gated by vocal activity, and make the text easy to edit.
- **Speed:** Demucs on the CPU is the slow step. Reuse existing stems, cache them, show the fast results first, and later consider a smaller separation model for analysis only.
- **One tempo, 4/4 only in DawIt:** the aligned copy is exact only for steady 4/4 songs. Other metres are shown in the window but copied as 4/4 bars until DawIt supports them. Note it in the window.
- **Model licences and training data:** several research models publish code under MIT without saying what applies to the weights. Each needs checking before shipping (see [6](#6-libraries-and-models)). Prefer models with explicit weight licences (Whisper, YAMNet, Basic Pitch, Demucs, Beat This!).
- **Model sizes:** Whisper small (466 MiB) plus Demucs (about 84 MB) plus YAMNet. Ask before each download, allow the lyrics to be skipped, and show the disk space used in the settings with a *Remove models* button.
- **Copyright:** the window's note about melodies and lyrics (see [3](#3-copying-into-your-project)). The analysis cache stays local.
- **Windows and Linux:** whisper.cpp, ONNX Runtime and the DSP are cross-platform; Metal and Core ML are macOS-only speedups.

## References

- D. P. W. Ellis, "Beat Tracking by Dynamic Programming", *Journal of New Music Research*, 2007.
- J. Foote, "Automatic Audio Segmentation Using a Measure of Audio Novelty", ICME 2000.
- J. Serrà, M. Müller, P. Grosche, J. Ll. Arcos, "Unsupervised Music Structure Annotation by Time Series Structure Features and Segment Similarity", *IEEE Trans. Multimedia*, 2014.
- B. McFee, D. P. W. Ellis, "Analyzing Song Structure with Spectral Clustering", ISMIR 2014.
- C. L. Krumhansl, *Cognitive Foundations of Musical Pitch*, 1990 (key profiles); D. Temperley, *The Cognition of Basic Musical Structures*, 2001.
- M. Mauch, S. Dixon, "pYIN: A Fundamental Frequency Estimator Using Probabilistic Threshold Distributions", ICASSP 2014; M. Mauch et al., "Computer-aided Melody Note Transcription Using the Tony Software", 2015.
- F. Foscarin, J. Schlüter, G. Widmer, "Beat This! Accurate Beat Tracking Without DBN Postprocessing", ISMIR 2024. [github.com/CPJKU/beat_this](https://github.com/CPJKU/beat_this)
- J. Park et al., "A Bi-Directional Transformer for Musical Chord Recognition", ISMIR 2019. [github.com/jayg996/BTC-ISMIR19](https://github.com/jayg996/BTC-ISMIR19)
- B. McFee, J. P. Bello, "Structured Training for Large-Vocabulary Chord Recognition", ISMIR 2017. [github.com/bmcfee/crema](https://github.com/bmcfee/crema)
- T. Kim, J. Nam, "All-In-One Metrical and Functional Structure Analysis with Neighborhood Attentions on Demixed Audio", WASPAA 2023. [github.com/mir-aidj/all-in-one](https://github.com/mir-aidj/all-in-one)
- whisper.cpp: [github.com/ggml-org/whisper.cpp](https://github.com/ggml-org/whisper.cpp); YAMNet: TensorFlow Models, `research/audioset/yamnet`; CREPE: [github.com/marl/crepe](https://github.com/marl/crepe); LAION-CLAP: [github.com/LAION-AI/CLAP](https://github.com/LAION-AI/CLAP).
