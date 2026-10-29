# Sing this: a sung vocal from a melody and lyrics

Status: Built, not committed; Theory checked, with open points
Idea: [Sing this: a sung vocal from a melody and lyrics](../FEATURE_IDEAS.html#sing-this-a-sung-vocal-from-a-melody-and-lyrics)

## Goal
*Sing this...* turns a MIDI melody and the words on the Lyrics row into a sung vocal on a new audio track. It uses a DiffSinger voice the user adds, and everything runs on their own machine. A writer who can't sing hears the song with a voice on it. Before rendering, the window shows which syllable goes on which note. A lyrics guide topic explains how a computer sings: phonemes, vowels on the beat, and a voice made from recordings.

This first slice covers English only, voices the user adds from a file, and rendering only. No project data changes.

## Decisions for you
1. **Scope.** Options: the full idea (one-click voice downloads, more languages, re-rendering in place), or this slice. **Recommended:** this slice. Voice formats vary, and the speed is unknown until step 1 measures it.
   Answer: This slice (English, render-only).
2. **Getting a voice.** Options:
   (a) *Add a voice...* takes a zip or folder the user downloaded, and *Get a voice...* links to the suggested voice's page;
   (b) a one-click download from the maker's release, pinned by size and SHA-256;
   (c) ship a voice with the app.
   **Recommended:** (a) now, and ask Lotte V before building (b). Her v1.0 is only on MediaFire (no stable URL to pin; GitHub has only the betas). Her licence forbids redistributing the original audio files. And (c) adds hundreds of MB.
   Answer: (a) now: the user adds a zip or folder; ask Lotte V before building (b).
3. **Vocoders under CC BY-NC-SA 4.0.** Nearly every DiffSinger voice uses OpenVPI's NSF-HiFiGAN or a fine-tuned copy of it. Hanami's own vocoder is one of these (her `LICENSE-vocoder` is CC BY-NC-SA 4.0). OpenVPI's maintainer wrote on 4 Oct 2026 (openvpi/vocoders issue #4) that the licence constrains redistributing and modifying the weights, "not the data that flows through it". **Recommended:** allow such voices: the user installs them, DawIt shows the terms, and DawIt never redistributes them. *Unconfirmed:* that is a maintainer's comment, not legal advice.
   Answer: Allowed, installed by the user with the terms shown; never redistributed.
4. **YAML** (`dsconfig.yaml`, `dsdict-en.yaml`). Options: a small reader for the subset these files use, or yaml-cpp (MIT, pinned). **Recommended:** the subset reader (block maps, block and flow lists, scalars, comments, quotes). It is tested on the suggested voice's files and fails clearly on anything else.
   Answer: Subset reader (recommended; not asked, default taken).
5. **Words not in the dictionary.** Options: guess from the spelling and flag them, skip them, or use OpenUtau's G2P ONNX model. **Recommended:** guess and flag now; the model can come later.
   Answer: Guess and flag (recommended; not asked, default taken).
6. **Writing the timing back to the Lyrics row** (*Put the words on these notes*). **Recommended:** later. There's no undo, and it would overwrite the user's timing.
   Answer: Later (recommended; not asked, default taken).

## What the user does and sees
- **Opening:**
  - *Sing this melody...* in a MIDI clip's right-click menu, after the transpose items. It sings that clip.
  - *Sing the lyrics with a computer voice...* in the Lyrics row's menu, after *Sing along*. It sings the whole melody track.
  - Both open the one **Sing** window through `PluginWindowManager::show`. No shortcut.
- **The window** (about 960 × 640, fixed):
  ```
  Sing "Lead vocal" with a computer voice                                 [?]
  Voice [Hoshino Hanami ▾] [Add a voice...] [Get a voice...]    by Lotte V · Terms...
  Melody [Lead vocal ▾]   Sing (•) this clip ( ) the whole track   Words [As timed ▾]
  Pitch [Natural ▾]  [x] Breaths  Quality [Good ▾]  [x] Mute the melody track afterwards
  ┌ picture: the notes as in the piano roll; each note shows its syllable, with its ─┐
  │ sounds below it (n ay t); a held note gets a line; notes without words are grey; │
  └ syllables that are left out are red above the grid ──────────────────────────────┘
  Advice: one line per rule, the good news first                    [Cancel] [Sing]
  ```
- **Without a voice:** the picture and advice still work, *Sing* is disabled, and a line says "Add a voice to sing this: Get a voice... shows where".
- **Sing:**
  - A progress window ("Singing phrase 3 of 12...") with Cancel.
  - Then a new audio track, "Hanami · Lead vocal", is added right after the melody track (in the same bus) and selected. Its one clip starts 0.5 s before the first note.
  - The melody track is muted if the box is ticked. Its notes and instrument never change.
- **The voice menu:** the installed voices, then *Show in Finder* and *Remove this voice...* (asks first).
- **Terms...** opens the voice's readme or licence file.
- **Add a voice...**
  - Takes a `.zip`, a folder or a `.oudep` vocoder.
  - Unpacks it into `~/Library/Application Support/DawIt/Voices`, then checks it.
  - Names any missing file, and refuses a voice with no English.
- **The picture:**
  - It follows changes to the lyrics and the notes.
  - A click on a note plays it with the built-in synth.
  - Over a note, the tooltip reads "Bar 9 · E4 · "night" (n ay t)" or "Bar 9 · E4 · no words: left out".
- **Tooltips** are in *Theory*.
- **App settings:** `sing.voice`, `sing.pitch`, `sing.breaths`, `sing.quality`, `sing.muteMelody`.

## Data and saving
None in the project; the format is unchanged.
- The vocal is an ordinary audio clip. Add it the way `stems::addStemTracks` adds a stem.
- Voices live in the Voices folder, next to `models::getFolder()`.
- A project still plays after its voice is removed.

## The guide
- **A new lyrics guide topic, *How a computer sings*,** between *Singing along* and *Your lyrics* (recipes.md *Topics in an existing guide*).
  - Its picture is a fourth example kind, *sounds*: the notes as in *Words on a melody*, with each phoneme as a block under its note.
  - Vowels are drawn in the section colour and consonants dimmer. Leading consonants start left of the note line.
  - Example: "feel the night". A click plays it.
- **"Your song"** is the Sing window's advice. *Your lyrics* is unchanged.
- **It opens from** the window's ? button and the Lyrics menu (which already ends with the guide).
- **The `gen::guides` lyrics entry:**
  - summary: "What each part says, lines, rhyme, stress on the beat, words on a melody, the hook, how a computer sings, and your lyrics.";
  - add "the Sing window's ?" to `opensFrom`;
  - keywords: add "voice vocal synthesis synthesiser diffsinger phoneme voicebank vocoder".
  - It is not on *Start here*.

## Code
- **Recipes:** *A long job* (copy `StemActions`), *A window*, *Topics in an existing guide*, *A command or menu item*.
- **New concepts**, in `src/generate`, UI-free and tested:
  - **Sung melody:** the top note wherever notes overlap, with times in seconds from the `TempoMap`.
  - **Pairing:** gives each note a syllable to start, one to hold, or nothing, in two modes. It also produces the advice (the rules are in *Theory*).
  - **Pronunciation:** a word becomes phonemes. It tries, in order:
    1. the voice's `dsdict-en.yaml`;
    2. the bundled CMUdict (stress digits removed, lower case, with the voice's language prefix such as `en/`);
    3. a guess from the spelling, flagged.
  - **Sung score:** phrases split at rests of 0.5 s or more. Each phrase holds phonemes with durations in frames, and an f0 per frame.
    - Vowels start on their note; leading consonants end at the note start; word-final consonants close the note.
    - 0.5 s of padding before each phrase, with SP (silence) and AP (breath) tokens.
  - **Voice:** a folder in OpenUtau's DiffSinger layout.
    - `dsconfig.yaml` names the acoustic model, the phonemes and the vocoder.
    - Optional sub-folders: `dsdur`, `dspitch`, `dsvariance`, `dsvocoder`.
    - A vocoder the voice doesn't contain is looked up in `Voices/Vocoders`.
  - **Singer:** an interface that renders a phrase's score to audio.
    - The real one runs the ONNX models in turn: duration, pitch, variance, acoustic (with steps), vocoder.
    - The tests use a fake that returns a sine at the f0.
    - Follow OpenUtau's MIT code for the tensor names and the call order (`DiffSingerBasePhonemizer.cs`, `DiffSingerPitch.cs`, `DiffSingerVariance.cs`, `DiffSingerRenderer.cs`).
    - Check each model's input names when it loads, and fail clearly on any it doesn't know.
- **Invariants:**
  - The output is the melody's span plus 0.5 s before and 0.5 s after, as one 44.1 kHz mono buffer.
  - The melody track changes only by the optional mute.
  - A broken voice fails before rendering, naming the file.
- **Threading:**
  - Notes, lyrics and the tempo map are copied under the lock; everything else runs on the job thread.
  - Cancel through `Ort::RunOptions::SetTerminate`.
  - CPU only, with intra-op threads set to the core count minus 2 so playback keeps going. CoreML can't run DiffSinger's dynamic shapes (OpenUtau PR #2553).
  - The track is added on the message thread. The audio thread is untouched.
- **Shared `Ort::Env`:** move it out of `AudioToMidi.cpp` into one engine helper.
- **Risks:** voice layouts that vary, the speed (step 1 is a gate), the subset YAML reader.

## Libraries and models
- **ONNX Runtime 1.30.0:** already in the app; no change.
- **CMU Pronouncing Dictionary** (`cmusphinx/cmudict`, `cmudict.dict`):
  - more than 134,000 words, 39 phonemes;
  - its README calls use "for any research or commercial purpose ... completely unrestricted" and asks for its origin to be acknowledged;
  - pin a commit with `URL_HASH SHA256` and embed it with `juce_add_binary_data` (about 3.6 MB; check when pinning). It runs locally.
- **DiffSinger** (openvpi, Apache 2.0): no code linked; DawIt runs exported ONNX models.
- **OpenUtau** (MIT): the reference. Add its notice if any code is translated closely.
- **No voice ships.** The suggested one is *Hoshino Hanami ~AI❤dol~ for DiffSinger v1.0* by Lotte V:
  - Team L❤VE Voicebank License: creative and commercial use, credit required, no redistribution of the original audio files;
  - its vocoder is CC BY-NC-SA 4.0;
  - *unconfirmed:* its size, and whether it loads as it is (step 1 finds out).
- All of these go in the README *License note* and on *How it's made*.

## Theory
**Facts** (E exact, T typical, O opinion or design value):
1. "night" is N AY T: 3 phonemes from 5 letters. E (CMUdict).
2. though DH OW, through TH R UW, tough T AH F. E (CMUdict).
3. CMUdict 0.7b: more than 134,000 entries and 39 phonemes (15 vowels and 24 consonants), with stress 0/1/2 on the vowels. E (Wikipedia *CMU Pronouncing Dictionary*; OpenUtau's English phonemizer lists 15 + 24).
4. One vowel sound per syllable: together is T AH G EH DH ER, 3 vowels. E (CMUdict).
5. "Consonants before the beat, vowels on the beat" is taught to choirs; recordings show initial consonants are nearly always early. T (GIA Quarterly 30/4, 2019; Univ. Bern, boris.unibe.ch/149910).
6. OpenUtau's DiffSinger starts the vowel on the note, puts leading consonants before it, and pads phrases by 500 ms. E (`DiffSingerBasePhonemizer.cs`).
7. Singing stretches vowels much more than consonants, compared with speech. T (NHSS, arXiv 2012.00337).
8. At 120 BPM a beat is 60/120 = 0.5 s. E (derived).
9. f = 440 · 2^((n − 69)/12): C4 261.63, D4 293.66, E4 329.63 Hz. E (equal temperament).
10. A frame is 512 / 44,100 s = 11.61 ms (86.13 per second). E (derived; openvpi/vocoders spec and OpenUtau defaults).
11. OpenVPI's vocoders: 44.1 kHz, 128 mel bands from 40 to 16,000 Hz, hop 512, window 2048. E (openvpi/vocoders releases).
12. Mel bands are spaced the way pitch is heard: narrow low, wide high. E (Stevens, Volkmann & Newman 1937).
13. DiffSinger removes noise step by step, guided by the score. Shallow diffusion starts from a rough guess and was 45.1% faster. E (Liu et al., AAAI 2022, arXiv 2105.02446).
14. OpenUtau's default speedup is 50 (1000/50 = 20 steps); a smaller speedup is slower and can sound better. E (OpenUtau wiki *DiffSinger support*).
15. Hanami has 3 h 53 min of data (about 2 h in English) and a range of about F3–A5. T (release notes; DiffSinger wiki).
16. DiffSinger's makers forbid making anyone's voice without their consent. E (openvpi/DiffSinger README).
17. Voice types run from bass E2–E4 to soprano C4–C6, so E2 (MIDI 40) to C6 (MIDI 84) covers nearly every singer. T (Wikipedia *Voice type*; other sources differ by a few notes).
18. Terms differ: Hanami allows commercial use with credit, Canary needs permission, Miyo needs a bought licence. E (DiffSinger wiki, Oct 2026).
19. A melisma is one syllable over several notes. E (as in the lyrics guide's *Words on a melody*).

Design values (O):
- pairing tolerance 0.125 beat (`lyrics::minGap`);
- *As timed* is the default when at least 75% of the syllables start on a note;
- phrases split at rests of 0.5 s or more, and get a breath of min(0.3 s, half the rest);
- without a duration model, consonants last 0.08 s each, and leading ones take at most half the previous note;
- *Exact notes* slide over 0.05 s;
- Quality: Draft 10, Good 20 (OpenUtau's default), Best 50 steps.

**Topic: How a computer sings** (summary "Sounds of words, notes and a voice")

> A singing voice synthesiser turns three things into a sung vocal: the words, the notes and a voice. Sing this... uses DiffSinger, an open-source singing synthesiser, with a voice you add.
>
> First the words become phonemes. A phoneme is one sound of a language: "night" has five letters but three sounds, n, ay and t. English spelling is a poor guide to sound: though, through and tough all end in "ough" and sound different. So DawIt looks every word up in a pronouncing dictionary, the CMU Pronouncing Dictionary, which lists more than 134,000 English words using 39 sounds: 15 vowels and 24 consonants. A word it can't find is guessed from its spelling, and the Sing window lists it so you can check how it sounds.
>
> Every syllable has one vowel sound at its centre, and in singing the vowel carries the note. "Together" has three vowel sounds, so it takes three notes, or one note held. Consonants are short noises around the vowel. Singers start the vowel on the beat and slip the consonant in just before it, so the word lands on time. Choirs are taught the same rule: consonants before the beat, vowels on the beat. DawIt places the sounds this way. In the picture, "feel the night" is sung at 120 BPM. The f of "feel" starts just before beat 1, and the n of "night" is sung at the end of the note before it, so the "ay" starts exactly on beat 3.
>
> This is why long notes want open vowels. At 120 BPM a beat lasts half a second, so "night" on a 2-beat note lasts 1 second. If the t takes about a tenth of that, the "ay" gets about 0.9 seconds. A word packed with consonants on a short note, such as "strengths" on a 16th note, leaves almost no time for its vowel and sounds rushed, just as it does when a person sings it.
>
> The voice is a model trained on recordings of one singer, often a few hours of them, called a voicebank. Its tone and the range where it sounds best come from that singer: the voice DawIt suggests, Hoshino Hanami, sings best from about F3 to A5. Several models work in turn. One decides how long each sound lasts. One draws the pitch curve with the slides and vibrato a singer adds, unless you choose Exact notes. One paints the sound as a spectrogram, a picture of how much energy there is at each pitch, about 86 times a second for most voices. Last, a vocoder turns that picture into audio. DiffSinger paints the spectrogram by starting from noise and removing it step by step. More steps take longer and can sound cleaner.
>
> What it doesn't do: it sings the notes and words you give it, and nothing more. It doesn't write the melody or the lyrics, it adds breaths only in the rests, and it won't act the song the way a singer would. Every voice comes with its maker's terms. Many let you use it in your own released songs with credit; some ask permission or a fee first. Read them before you publish, and credit the voice. Only use voices whose singer agreed to become a voicebank: DiffSinger's makers ask that nobody's voice is made without their consent.

**Tooltips:**
- Voice: "The voice that sings: a model trained on recordings of one singer. Add voices with Add a voice..."
- Add a voice...: "Add a DiffSinger voicebank you downloaded (a .zip or a folder), or a vocoder (.oudep) that a voice needs."
- Get a voice...: "Opens a page with English DiffSinger voices you can download, and the one DawIt suggests."
- Terms...: "The voice maker's terms. Many voices ask for credit; some ask permission or a fee before you release music."
- Melody: "The MIDI track whose notes are sung, one note at a time."
- This clip / the whole track: "Sing only the clip you picked, or every clip on the melody track."
- Words: "As timed: each note sings the syllable that starts on it, and a note inside a syllable holds it. One syllable per note: the syllables in order, one on each note, whatever their timing."
- Pitch: "Natural: the voice adds its own slides and vibrato. Exact notes: the pitch stays on each note, with a short slide between notes."
- Breaths: "Adds a short breath before a phrase that follows a rest of half a second or more."
- Quality: "How many steps the voice takes to paint its sound. Draft is quickest; Best is slowest and can sound cleaner."
- Mute the melody track afterwards: "Mutes the melody's own instrument so you hear the voice alone. The notes stay."
- Picture: "The melody with the syllable each note sings and its sounds below. Click a note to hear it."
- Sing: "Renders the vocal to a new audio track, on this computer."
- ?: "How a computer sings: phonemes, notes and voices."

**Example** ("feel the night" at 120 BPM; the consonants are an illustrative 0.08 s, or 0.16 beat):

| Note | Beat, length | Pitch | Sounds (seconds from beat 1) |
|---|---|---|---|
| feel | 0, 1 | D4 (62), 293.66 Hz | f −0.08–0.00, iy 0.00–0.34, l 0.34–0.42 |
| the | 1, 1 | C4 (60), 261.63 Hz | dh 0.42–0.50, ah 0.50–0.92 |
| night | 2, 2 | E4 (64), 329.63 Hz | n 0.92–1.00, ay 1.00–1.92, t 1.92–2.00 |

The vowels take 0.34 + 0.42 + 0.92 = 1.68 s of the 2.08 s, which is 80.8%.

**"Your song" rules** (the window's advice; bars counted from 1):
1. **No words in range:** "There are no words on the Lyrics row over bars 5–12. Type them with Edit... in the Lyrics header first." Sing is disabled.
2. **Notes without words** (As timed; the note starts outside every lyric line): "4 notes in bars 5–6 have no words, so they're left out. Add words there, or try One syllable per note."
3. **Syllables without a note** (As timed; no note starts within 0.125 beat of it): ""ge" in bar 9 starts between notes, so it isn't sung. Drag it onto a note on the Lyrics row, or try One syllable per note."
4. **Counts differ** (One syllable per note): "12 notes and 14 syllables: the last 2 syllables ("to-night") aren't sung." or "12 notes and 10 syllables: the last 2 notes aren't sung. To hold a syllable over several notes, time the words and use As timed."
5. **Guessed words:** ""yeah-eh" isn't in the dictionary, so its sounds were guessed from the spelling. Listen to it, or spell it the way it sounds."
6. **Overlapping notes:** "Notes overlap in 3 places. A voice sings one note at a time, so the top one is sung."
7. **Out of range** (below E2 or above C6): "2 notes go above C6, higher than nearly every singer. Try them an octave lower."
8. **Crowded** (a note under 0.15 s whose syllable has 3 or more consonants): ""strengths" in bar 3 has many consonants on a 0.13 s note and may sound rushed. Give it a longer note or a shorter word."
9. **Melisma** (for information): ""night" is held over 3 notes: the vowel slides from note to note."
10. **All good** (none of rules 1–8 apply): "Every note has a syllable and every syllable a note: ready to sing."

**Test numbers:**

| Check | Expected | Tolerance |
|---|---|---|
| f0 for MIDI 69, 60, 62, 64 | 440.00, 261.63, 293.66, 329.63 Hz | 0.01 |
| frame | 11.61 ms (86.13 per s) | 0.01 |
| "feel the night", fallback durations: vowel starts | iy 0.00, ah 0.50, ay 1.00 s | 1 frame |
| ... first start, last end | f −0.08 s, t 2.00 s | 1 frame |
| clip for 8 beats at 120 / 60 BPM | 5.0 / 9.0 s | 0.02 s |
| syllable 0.10 / 0.20 beat after a note | paired / not paired | exact |
| range warning for MIDI 39, 40, 84, 85 | yes, no, no, yes | exact |
| a rest of 0.5 s / 0.25 s | new phrase / SP in the same phrase | exact |
| CMUdict | 39 phonemes, 15 vowels; night = n ay t; together has 3 vowels; strengths has 1 vowel and ≥ 5 consonants | exact |

## Steps
**Group A: engine and generate** (build at step 6)
- [x] 1. **Spike and gate.**
  - Build: the shared `Ort::Env`; the voice reader (subset YAML, `dsconfig`, phonemes, sub-folders); the real Singer.
  - Add an optional `DawItTests --voice <folder>` that sings "feel the night" to a WAV in the temp folder and prints the time it took. The user first puts Hanami v1.0 in the Voices folder by hand.
  - Tests: the YAML cases; a missing `acoustic.onnx` gives an error that names it.
  - **Gate:** if Good takes more than twice the sung length on this Mac, stop and report the numbers.
- [x] 2. **Pronunciation:** fetch and embed CMUdict; the lookup and the spelling guess. Tests: the CMUdict row; "zorbly" is guessed and flagged.
- [x] 3. **Pairing and advice:** the top-note melody, both modes, the default mode, rules 1–10. Tests: the pairing, range and mode rows; every rule's text on made-up songs.
- [x] 4. **Sung score:** phrases, phoneme timing (from the model or the fallback), breaths, f0 (exact with slides, or the pitch model). Tests: the "feel the night", f0, frame and phrase rows.
- [x] 5. **Render core:** phrases into one buffer with the lead-in and tail; cancel; progress. Tests: the clip-length row with the fake Singer; a cancelled render returns nothing.
- [x] 6. **Guide content:** the topic, the *sounds* example, the `gen::guides` text. Tests: the topic has text and an example; "voicebank" and "phoneme" find the lyrics guide; one more topic than before. Done when `./build.sh --test` passes without warnings.

**Group B: UI** (build at step 10)
- [x] 7. **Voices:** install (zip, folder, `.oudep`, with `juce::ZipFile`), list, remove, check, the terms and credit line.
- [x] 8. **The Sing window:** layout, picture, advice, tooltips, settings, the ? button.
- [x] 9. **`SingActions`** (owned by `ArrangementView`): the job, adding the track, the mute, the two menu items.
- [x] 10. **The *sounds* picture** in `LyricsExplainer`, with click to play. Done when `./build.sh --test` and `--export-guides` pass.
- [x] 11. **Real-app check.**
  - Add a `sing` scene to `ScreenshotRunner` that opens the window on Lead vocal's Melody clip.
  - Shots: the window without a voice (the hint, Sing disabled); with a voice when one is installed (the syllables on the notes, "ready to sing"); the lyrics guide at *How a computer sings*.
  - By hand with a voice: sing the clip. The new track sits under Lead vocal and lines up with the Lyrics row.
- [x] 13. **Getting a voice is explained in the app** (added at the user's request):
  - the Sing window without a voice, or with a voice that fails the check, shows a help panel with the steps and this computer's Voices folder, a *Step by step...* button, and the check's error that names the missing file;
  - *Get a voice...* is a menu: Hanami's page, the DiffSinger wiki's list of English voices, and the step-by-step guide;
  - a lyrics guide topic, *Getting a voice*: what a voice is, where to download one, what to download, how to add it, what the folder holds, how to tell it worked, what the terms mean and what to do when it fails;
  - tests: the topic's text mentions each of those, it has an example, and the search finds it.
- [x] 12. **Docs:**
  - README: Features, Tests, Code layout, License note.
  - FEATURE_IDEAS: the status block, the label and the counts.
  - docs/architecture.md *Long jobs and models*: the Voices folder, the shared ONNX environment, the Singer.
  - docs/recipes.md *A long job*: a model folder the user installs is checked before it's used.

## Tests
Add to `tests/EngineTests.cpp`:
- every row of *Test numbers*;
- the YAML reader: nested maps, flow lists, comments, quotes, and an error on a tab or an anchor;
- the voice check: a missing file is named, and a voice with no English is refused;
- *As timed* holds a syllable over the notes inside it (melisma);
- *One syllable per note* reports what's left over on both sides;
- the melody track's notes and instrument are unchanged after a fake render;
- every advice rule on made-up songs;
- the new topic's text and example;
- the search words.

The `--voice` run is optional and not part of `./build.sh --test`.

## Website hand-off
- **Features page:** "Sing this: a computer voice sings your melody". Say that the user adds the voices and that each voice has its own terms.
- **Screenshots:** the Sing window on the example song's Lead vocal; the arrangement with the sung track and the Lyrics row open; the guide at *How a computer sings*.
- **Main page:** ask the user.
- **How it's made:** CMUdict, DiffSinger's model format, OpenUtau as the reference; no voice is shipped.

## Left for later
- One-click voice download (after asking the maker).
- Other languages: OpenUtau has phonemizers for about 15, but DawIt splits syllables for English only.
- Re-rendering one phrase, or in place.
- Expression curves: gender, velocity, breathiness, tension.
- *Put the words on these notes* (no undo yet).
- A G2P model for unknown words.
- GPU rendering.
- A topic in the singing techniques guide once that guide exists.
- Harmonies with several voices.

## Changes while building

- **Step 1 (the gate) passed.** The user placed Hoshino Hanami v1.0 in the Voices folder (one folder deeper than expected, with .url files beside it). `DawItTests --voice` sings "feel the night" (2 s) on this Mac (Apple silicon, CPU, threads = cores - 2) with the duration and pitch models used: Draft 1.28 s, Good 1.69 s, Best 3.33 s, that is 0.64x, 0.85x and 1.67x the sung length, so Good is well under the gate of 2x. The pitch measured on the result was D4 296 Hz, C4 262.5 Hz and E4 331.6 Hz (expected 293.66, 261.63, 329.63), the clip is 3.0 s (2 s + 1 s), and the peak is 0.20, so the vocal is quiet. The models' input names matched what was written from OpenUtau's code (acoustic: tokens, durations, f0, gender, velocity, spk_embed, depth, steps; vocoder: mel, f0; dur: encoder_out, x_masks, ph_midi, spk_embed; pitch: encoder_out, ph_dur, note_midi, note_rest, note_dur, pitch, expr, retake, spk_embed, steps), no warnings.
- **The real voice's layout differs from the plan's idea of it**, so the reader is more general: `dsconfig.yaml` names files in sub-folders (`dsmain/acoustic.onnx`, `dsmain/phonemes.txt`); its speakers are `.emb` files whose first speaker (Root) is used; the vocoder is `dsvocoder/vocoder.yaml` found by its `name`; the duration and pitch models are in `dsdur` and `dspitch`, each with a linguistic encoder and its own speaker file; phonemes carry no `en/` prefix (a shared set, "ay" is English as well); the English dictionary `dsdict-en.yaml` is in `dsdur`; the terms are `Terms of Use.pdf` (shown by *Terms...*), with licences in `__License`.
- **The optional duration and pitch models are left out, with a note, when they can't be understood** (unknown input names, a missing speaker file, or a failure while singing), and the sound lengths and exact pitch are used instead. The acoustic model and the vocoder must be understood completely, or loading fails naming the input. The variance model (breathiness, tension, voicing) is not used: the acoustic model gets zeros for those curves.
- **Added:** `Voices/Vocoders`, `sing::overrideVoicesFolder` (the screenshot runner's *sing* scene can show the window with no voice), the scene's `sing` and `voice` options, `Singer`/`addVocalTrack` in the engine, and a second new guide topic, *Getting a voice* (the user's request), so the lyrics guide has two more topics, not one.
- **CMUdict:** the text said "more than 134,000 words"; the file has 135,166 lines, 126,052 different words (the rest are other ways to say a word). The topic now says "more than 134,000 entries for about 126,000 English words". dawTheory should check this. The embedded file is pinned at commit 74790861f652b15e4ac49015a90074ad62a27690, SHA-256 81917843c7f44ce2b094ac63873c2c7a4cf802040792c455ba3ca406891c3d22, with its licence ("any research or commercial purpose is completely unrestricted", origin to be acknowledged, and a BSD-style licence file).
- **Facts in *Getting a voice* that the plan's Theory did not have** (written from the voice's own licence files and docs/interesting-projects.md; dawTheory should check them): Hanami's terms (credit Lotte V; commercial and non-commercial use without asking; no re-uploading the voice's files, no selling them; no political, hateful or illegal use), the vocoder's CC BY-NC-SA 4.0, the size "about 570 MB unpacked" (measured), the list of other English voices and their terms (from docs/interesting-projects.md, read in October 2026), and the error texts.
- *Words* use the syllables' own text, so a word split by hyphens in the lyrics is looked up whole ("to-night" is "tonight").
- The *Natural* pitch model, the duration model and the vocoder were verified only with Hanami's models.
- **Real-app check done with the real voice:** pressing Sing on Lead vocal's whole Melody clip (about 21 s of the 96 BPM song's first 14 bars shown; the clip is the full song) added the vocal track in the Vocals bus right after Lead vocal, muted the melody, and the vocal lines up with the Lyrics row. The vocal is quiet (peak about 0.2, meters at -14 dB), so users may raise the fader; a level match was not added.
- **dawTheory:** CMUdict counted at the pinned commit: 135,166 entries, 126,052 words, 39 phonemes (15 vowels); *How a computer sings* now says "about 135,000 entries for about 126,000 English words", and the test checks both counts within 1,000.
- **dawTheory:** *How a computer sings*: "together ... takes three notes, or one note held" became "three notes, one for each syllable (a syllable can also be held over several notes)" (a note can't carry three syllables in DawIt); "long notes want open vowels" became "the vowel gets nearly all of a long note" (what the paragraph shows); the spectrogram shows energy "at each frequency", not "at each pitch".
- **dawTheory:** *Getting a voice*: Hanami's terms checked against her Terms of Use.pdf and `__License` files (added "or claim the voice as your own"; credit "for example" in the description, since the terms don't say where); the OpenVPI comment checked against openvpi/vocoders issue #4 (yqzhishen, 4 Oct 2026: redistribution and modification of the weights), now said to be about OpenVPI's vocoder, one person's reading, not legal advice and not Lotte V's; the other voices' terms are now called community-wiki summaries from October 2026, with "read each maker's own terms"; the vocoder may be shared and named only; a measured speed (Good a little under the clip's length on the test Mac); the quoted error texts match the code's wording.
- **dawTheory:** the Sing window's *Get a voice...* tooltip and help panel now describe a menu (the suggested voice's page, the wiki's list, the steps), not one page.
- **Bug hunt: "the sung words differ from the lyrics" (real voice, Hoshino Hanami).** Findings: (1) The acoustic, duration and pitch models of this voice have **no `languages` input** (their inputs are tokens, durations, f0, ... and tokens, word_div, word_dur), so the language id is irrelevant here (`englishLanguageId` -1 is harmless; the shared bare phoneme set needs no id). (2) The tokens for "feel the night" are right (f iy l dh ah n ay t, ids 58 74 84 43 23 88 28 117); CMUdict sounds map to the voice's own symbols. (3) A real typo: the word inputs were given under the name `word_wordDiv` instead of `word_div`, so the duration model always failed and fixed sound lengths were used; fixed. (4) With `DawItTests --voice <folder> --whisper <ggml-medium.en.bin>`, whisper.cpp hears the rendering as "Feel the night" and the first 52 beats of the example song as the lyrics' words (only "late" heard as "lane"), with and without the typo. So the voice and the sound pipeline sing the right words for correctly paired material. (5) Pairing changes: lines are now dealt out in the order they are heard, not the order of the list (tapping a line's first word can leave a line before or over its neighbour), and a line that runs over the next one stops its last word where the next line starts, so it no longer holds notes that belong to the next line's words. Fewer or more notes than syllables never invents words: As timed leaves out the syllables with no note (advice rule 3) and notes with no syllable (rule 2); One syllable per note sings the first syllables in order and leaves the rest (rule 4); only words missing from both dictionaries are guessed from the spelling, and rule 5 says so. `--voice` now prints the language id, the tokens with their ids and frames, the example song's pairing (each word with its source) and the advice, and what Whisper hears when `--whisper` is given.
- **Bug fix: fast singing lands words in the wrong place (real voice, short notes).** Measured with the new `st::measureTiming` (the vowel start of every syllable against its note, after rounding to frames; `Score::syllables`) on a made-up tune of 16 words with clusters ("strengths next light street and glimpsed ...", one word per note, 140 BPM): the vowels *did* start on their notes already (mean error 2.7 ms, max 5.6 ms, half a frame), except at 32nd notes (max 28 ms, drifting later). The real fault was **the vowels being crushed**: the old rule gave the next word's consonants up to half the previous note and then the previous word's closing consonants 60% of what was left, so on 16th notes (107 ms) the vowel kept as little as 1 frame (11.6 ms, 11% of its note; with the real voice's duration model "eh2 ng1 k1 th1 s1 n3" and "ay1"), which the acoustic model paints as a missing or neighbouring vowel. Also a sound squeezed under a frame was forced to 1 frame and pushed every later sound (up to +28 ms at 32nd notes). Fix in `SingScore.cpp`: (1) for each note the closing consonants and the next lead are shortened together so the vowel keeps max(40% of the note, 3 frames), at most 80% of the note; (2) frames come from rounding each boundary separately, and a sound that would get under one frame takes it from the sounds before it (not by moving later ones), so the phrase length stays exact. After: 16th notes at 140 BPM shortest vowel 34.8 ms (3 frames, 32% of the note) instead of 11.6 ms, 8th notes 81.3 ms instead of 34.8 ms, 120 BPM 16ths 46.4 ms; onset error unchanged (max 5.6 ms); 32nd notes (53 ms) cannot hold the clusters: max error 25.5 ms, and early (-7.6 ms mean) instead of late (+7.6 ms), with no drift; the example song is unchanged (34 vowels, max 5.5 ms, shortest vowel 232 ms). The duration model's output is used sensibly (only consonants, each 20 to 250 ms, then shortened as above); the rendering is placed by the same frame grid (`startSeconds + cutFrames * hop`), so there is no latency in `renderPhrases`. Not verified: how it sounds. Whisper's word times were of no use at these speeds (it hears 12 to 13 of 16 words, in the wrong words), so there is no acoustic measurement; the measurement is of the frame plan. `--voice` now prints this timing for the example song and for the fast tune at 8th and 16th notes with the real models.
