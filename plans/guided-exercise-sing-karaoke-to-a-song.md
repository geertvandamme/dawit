# Guided exercise: sing karaoke to a song

Status: Released
Idea: [#197 — Guided exercise: sing karaoke to a song](https://github.com/geertvandamme/dawit/issues/197)
Part of: [#195 — Guided exercises that teach by doing in your own song](https://github.com/geertvandamme/dawit/issues/195). Uses the shared exercise model and window (`gen::exercises`, `ExerciseWindow`) that #196 built from #159's First song exercise.

## Goal

An exercise that turns a song the user likes into a karaoke practice session: bring in the recording, take the vocals out, let DawIt find the words, sing along while they light up, record a take and listen back. Every step ticks from the real song (tracks, lyrics, a recorded clip, what was played), points at the control, links to the guide page behind it and says what is missing (a model to download, no microphone, no armed track) with a way forward. It strings together four features a beginner would never find in this order, and ends with the one thing that teaches most: hearing yourself.

## Decisions for you

1. **Where it opens.** Options: Help > *Sing Karaoke to a Song...* only; also an audio clip's right-click *Sing karaoke to this song...* (after *Remove vocals (karaoke version)...*); also the Start panel. **Recommended:** Help and the clip item, like *Learn the Chords of a Song*. The Start panel tile (#209) and an exercise list stay in #195.
   Answer: Help and the audio clip menu.
2. **Five steps or six.** The issue has "record a take and listen back" as one step. Options: one step that ticks when a take exists; two steps, *Record a take* and *Listen back*, each ticking from the song. **Recommended:** six steps: listening back is where a singer learns, and a separate tick makes sure it happens.
   Answer: Six steps, with a separate listen-back step.
3. **When the vocals count as taken out.** The Remove vocals dialog offers the vocals off (track muted), quiet (−12 dB) or softer (−6 dB) as a guide. Options: only off/muted; off or at most −6 dB. **Recommended:** muted or at most −6 dB: a quiet guide voice is a real karaoke choice for learning a tune; the done note says how to mute it when the tune is known.
   Answer: Muted or at most −6 dB.
4. **What step 5's button does.** Options: start recording itself, one bar before the first line of words (the count-in setting applies as usual); only move the playhead there and point at Record. **Recommended:** start recording: it is the user's own click, Monitor stays off, and it removes the hardest part of a first take (finding where to start). Stop is the usual transport Stop.
   Answer: Start recording one bar before the first words on the user's click.
5. **Model downloads.** Options: the exercise downloads Demucs and Whisper itself; the existing dialogs do it (they already ask first and show a progress window with Cancel) and the exercise says beforehand what will be downloaded, how big, and whether it is there already. **Recommended:** the existing dialogs: one download path, already tested; the issue's "progress window" requirement is met by them.
   Answer: Use the existing download dialogs.
6. **Finding the karaoke tracks after a restart.** In memory the exercise knows which tracks Remove vocals made; after reopening a project it doesn't. Options: in memory only (step 2 unticks after a restart); also recognise an audio track named "Vocals" (the name Remove vocals and Split into stems give it). **Recommended:** both: the name rule also ticks *Split into stems* followed by muting Vocals, which the issue names as a way to do step 2.
   Answer: Also recognise an audio track named Vocals after restart.

## What the user does and sees

**Opening.** Help > *Sing Karaoke to a Song...* (new command, no shortcut, after the other exercises); an audio clip's right-click menu, *Sing karaoke to this song...* after *Remove vocals (karaoke version)...* (opens the exercise only). The shared exercise window (360 wide, always on top), titled "Sing karaoke to a song", six rows, "N of 6 done", Back / Skip / Next and completion as in the other exercises. Height starts at 740 (`Info::windowHeight`), confirmed by shot at the tallest step.

**The song span:** the bars of the karaoke source clip while the karaoke record (below) exists, else of the song clip (`songchords::findSongClip`), ignoring clips the take record names, so a long take never becomes "the song".

| Step | Done when (live) | Action | Show me |
|---|---|---|---|
| 1 Bring in a song | an unmuted audio clip spans ≥ 4 bars (as the chord exercise) | *Import audio...* | Import tile if shown, else the track lanes |
| 2 Take the vocals out | the **vocals track** is muted (track or its clips, or `routing::getSilence` not none) or at most −6 dB, and another audio clip covering ≥ 4 bars of the span is heard | *Remove vocals...* (the existing dialog on the song clip) | outlines the song clip |
| 3 Find the words | ≥ 2 timed lines (`writelyrics::isLineTimed`) start inside the span | *Detect lyrics...* (the existing dialog on the vocals track's clip when there is one, else the song clip) | outlines that clip |
| 4 Sing along | the song played through 4 different bars of the span that hold a syllable while Sing along was showing | *Open Sing along* (`singAlongCommand`) | the Lyrics header's Sing along button |
| 5 Record a take | a clip the last recording made is still in the song, at least 2 bars long, and overlaps a bar with words | *Record from bar N* (decision 4) | the armed track's R button, else the transport's Record |
| 6 Listen back | the song played (not recording) through min(4, take's bars) different bars of that take while it was heard (clip unmuted, track not silenced) | *Play your take* (playhead to the take's start, play) | outlines the take clip |

**The vocals track:** the karaoke record's Vocals track if still in the edit, else the first audio track named "Vocals" (case-insensitive) with a clip overlapping the span (decision 6).

**Missing prerequisites** (status line in `textDim`; the button becomes the fix):
- Steps 2–6 without a song clip of 4 bars: *Import audio...*.
- Step 2 or 3, Demucs not downloaded: a line with its size (no fix; the dialog asks). Step 3 says the Whisper model's size, or "(downloaded)", and that Demucs isn't needed again when the Vocals track exists.
- Step 2 with vocals louder than −6 dB: line saying how to turn them down or mute them; button stays.
- Steps 4–6 with no timed lines in the span: *Detect lyrics...*. Step 4 with no output device: *Audio/MIDI Settings...*.
- Step 5, no input channels: *Audio/MIDI Settings...*; no audio track armed and no empty audio track: *Add a Recording track* (new `Fix`, runs `MainComponent::addRecordingTrack`, which already points at R); an unarmed audio track exists: line "Press R on <track> to arm it", button disabled, Show me points at R.
- Step 5, the last recording came in silent (report `removedAsSilent`): line pointing at the input chip and *Setting the level*.
- Step 6 while recording: line "Stop recording first".
- Counting lines: "Sung along: 2 of 4 bars", "Heard 1 of 4 bars of your take".

Nothing changes the song without a click: Remove vocals, Detect lyrics, Add a Recording track, Record and Play are the user's own actions through existing code.

**Remembered:** `learn.karaoke.done` only, like every exercise (the step is in `StepMemory`).

**First steps: no change.** A separate Help exercise; no renamed command or control.

## Data and saving

No project data, no format change. In memory, owned by `MainComponent`:
- **karaoke record:** the tracks Remove vocals added (from `StemSplitter::onStemsAdded`, forwarded by `ArrangementView`), which of them is Vocals, and the source clip's span in beats. Cleared in `afterReplace`.
- **take record:** the clips the last recording made (from the engine's report, below), compared by address with the engine's clips, never dereferenced unless found. Cleared in `afterReplace`.
- **sung-along bars** and **listened bars**: distinct-bar counters like `songchords::PlayedAlongCounter`. Cleared in `afterReplace` and on *Start over*; a new take clears the listened bars.

## The guide

No new guide or topic, no `gen::guides`, `TheoryGuideLinks.cpp` or guide-text change. Step links (the links test checks each names an existing topic):
- 1: MIDI · *What MIDI is* (audio versus MIDI).
- 2: What is sound? · *Taking the vocals out*.
- 3: How lyrics are written · *Your lyrics*.
- 4: How lyrics are written · *Singing along*; Singing techniques · *Posture and breathing*.
- 5: How recording works · *Hearing yourself*; How recording works · *Setting the level*.
- 6: How recording works · *Takes and loop recording*; Singing techniques · *Singing in tune*.
- Completion: Singing techniques · *Your voice and your song* (carry-on button *Open the singing guide*).

## Code

Recipes: *A guided exercise* (copy *Write Lyrics to Your Song* for the 5 Hz tracker and *Learn the Chords of a Song* for the song clip and its fixes), *A command or menu item*, *Checking in the real app*.

- **`gen::karaoke`** (new, UI-free, both CMake targets): texts, the song span, the vocals track rule, the six done rules, statuses and done notes, and the two bar counters. Pure functions on `Engine` plus a small progress value (karaoke record, take record, counters, Sing along showing, output/input present, recording, last report silent).
- **`gen::exercises`:** `Exercise::karaoke`, stem `learn.karaoke`; `Context` gains the karaoke progress. `Fix` gains `addRecordingTrack`; step 2, 3, 5 and 6's own actions are dispatched on the step in `runExerciseAction`.
- **Engine:** a finished recording's report also says which clips it made (identities only), so the take record never guesses from track names. Tested with the fake device.
- **`ArrangementView`** gains entry points to open Remove vocals and Detect lyrics on a given clip (like `detectChords`) and forwards `onStemsAdded`.
- **Feeding:** the existing 5 Hz timer (`feedWriteLyrics`' heard beat) feeds both counters; a bar lasts ≥ 1 s up to 240 BPM, so none is missed.
- **Invariants:** no tick from a button press; nothing reads a deleted clip or track (find fresh, compare addresses, `engine.indexOf`); the exercise never writes notes, lyrics or levels itself.
- **Threading:** message thread only; no audio-thread or format change. Risk: low; the engine report change touches recording, so the existing recording tests must pass unchanged.

## Libraries and models

None new. It uses Demucs (htdemucs through demucs.cpp, MIT; weights under `licenses/Demucs-weights-LICENSE.txt`) and Whisper small/medium English (through whisper.cpp v1.9.5, MIT; weights under `licenses/Whisper-weights-LICENSE.txt`), already in `LICENSE`, `licenses/` and the README's *License note*, downloaded through the existing pinned `ModelDownload` paths. No `LICENSE`, `licenses/` or packaging change.

## Theory

No guide content changes. The exercise texts state these facts; all but fact 13 are DawIt behaviour or already in a guide with its source.

### Facts

1. Remove vocals splits the recording with Demucs into drums, bass, other and vocals; the four add up to the mix, so the three without vocals are the song without its singer. Exact (DawIt, `Stems.h`; What is sound? · *Taking the vocals out*).
2. Some reverb on the voice and a melody an instrument plays with the singer can stay faintly; backing vocals go with the lead. Typical (same topic).
3. Guide vocals: off (muted), quiet −12 dB, softer −6 dB. Exact (`stems::getGuideDb`).
4. Demucs is downloaded once (its size from `stems::getModelDownloadSize`, about 84 MB) and separates in about the song's length on a fast computer. Exact (size, code) / typical (time, `Stems.h`: "roughly as long as the audio on an M1 Max").
5. Whisper small is 487,614,201 bytes, medium 1,533,774,781 bytes; medium hears more words and is about 3 times slower. Exact (pinned sizes in `LyricsFinder.cpp`) / typical (speed, existing dialog text). Shown with `models::formatSize`.
6. Detect lyrics is English only; it snaps each word to where the voice begins, up to 0.15 s. Exact (DawIt).
7. Whisper can mishear sung words; corrections go in the lyrics editor. Typical (existing dialog text).
8. Everything runs on this computer; the music isn't sent anywhere. Exact (DawIt).
9. Lyrics of other people's songs belong to them: use them to learn and practise, not to publish. Opinion/caution (existing Detect lyrics dialog text).
10. Use closed headphones: speakers leak the song into the take, and a microphone hearing itself can howl. Typical (How recording works · *Hearing yourself*; Singing techniques · *The microphone*).
11. Loudest sung notes should peak around −18 to −10 dB, never near 0. Typical (How recording works · *Setting the level*; Singing techniques · *The microphone*).
12. A take quieter than −60 dB throughout is removed as silent. Exact (DawIt, `TakeReport::removedAsSilent`).
13. Your recorded voice sounds higher and thinner than you expect: while you sing you hear yourself through the air and through the bones of your head, which adds low end; a recording has only the air part, which is closer to what others hear. Typical, well established (JAIST study reported by Medical Xpress 2021; Irish Examiner *Appliance of science* 2019). New: sources added to `SOURCES.md` under *Not for a guide*.
14. 4 bars of 4/4 at 120 BPM last 8 s; 2 bars, 4 s. Exact (derived: 16 × 0.5 s; 8 × 0.5 s).
15. Sing along opens with Cmd+5; Import audio with Cmd+I. Exact (command shortcuts).

### Texts

Final; `{...}` is filled in live. British spelling.

**Title:** Sing karaoke to a song
**Help description:** Take the vocals out of a song you like, see its words light up, sing along, then record yourself and listen back.
**Intro:** Pick a song you like and sing it yourself. DawIt takes the singer out, finds the words and shows them as they're sung. Then you record a take and hear how you sound. Everything runs on this computer. Use headphones from step 4 on.

**Step 1. Bring in a song** — *An audio recording of a song with singing*
- Do: Import a recording of a song with a singer, at least 4 bars long (8 seconds at 120 BPM).
- Why: A recording is audio: the sound itself, not notes you can edit. The next steps split that sound into parts and listen to the words in it.
- Action: *Import audio...* — tooltip: Choose an audio file and put it on a new track (Cmd+I)
- Done note: "{clip name}", {n} bars long.

**Step 2. Take the vocals out** — *Make a karaoke version*
- Do: Click Remove vocals. In the window, choose whether the singer stays faintly as a guide, then let it run. It adds a Karaoke track without the singer and a Vocals track under it, and mutes your original clip.
- Why: Demucs, a model that has learned what voices, drums and bass sound like, splits the song into four parts that add up to the mix. Without the vocals you hear the whole band, low end included. A little reverb from the voice can stay, and backing vocals go with the lead.
- Action: *Remove vocals...* — tooltip: Open Remove vocals (karaoke version) for "{clip name}"
- Status, Demucs not downloaded: The first time, Demucs ({size}) is downloaded from Hugging Face. Remove vocals asks before it downloads and shows its progress. Splitting takes about as long as the song on a fast computer, and longer on an older one.
- Status, vocals still loud: The Vocals track is louder than −6 dB. Turn its fader down, or press M on it to mute it.
- Done note (muted): The vocals are muted. (Guide): The vocals play at {n} dB as a guide. Press M on Vocals when you know the tune.

**Step 3. Find the words** — *Let DawIt write down what is sung*
- Do: Click Detect lyrics. It listens to the Vocals track, so the singer is already on their own, and puts the words on the Lyrics row, each syllable where it's sung.
- Why: Whisper, a speech model, hears the words; DawIt then moves each word to where the voice begins and splits it into syllables. It works for English only, and it can mishear a sung word: double-click the Lyrics row to correct the words. The words of someone else's song belong to them: use them to practise, not to publish.
- Action: *Detect lyrics...* — tooltip: Open Detect lyrics for "{clip name}"
- Status, model sizes: Whisper small is a {size} download the first time ({downloaded} when it's there). Demucs isn't needed again: the Vocals track has the singer on their own. (Without a Vocals track: Detect lyrics finds the vocals first with Demucs.)
- Status, no song: Bring in a song first (step 1).
- Done note: {n} lines of words, from bar {a} to bar {b}.

**Step 4. Sing along** — *Read the words as they light up*
- Do: Open Sing along (Cmd+5), press Play and sing. The colour runs over each syllable while it's sung; after a rest, four dots count you in. Sing through 4 bars with words.
- Why: Reading the words in time frees you to listen to the band and your pitch. Stand or sit up straight and breathe in before the count-in ends. Wear headphones so you can hear the song over yourself.
- Action: *Open Sing along* — tooltip: Show the words in large letters, coloured as they're sung (Cmd+5)
- Counting: Sung along: {n} of 4 bars.
- Status, no words: Find the words first (step 3). No output: No audio output is chosen. Choose one in Audio/MIDI Settings to hear the song.
- Done note: You sang along through 4 bars.

**Step 5. Record a take** — *Record yourself singing over the karaoke version*
- Do: Arm a track with R, then click Record from bar {n}. DawIt starts one bar before the first words, so you have time to come in. Sing at least 2 bars, then press Stop.
- Why: Wear closed headphones, not speakers: speakers leak the song into your take, and a microphone that hears itself can howl. Set the input so your loudest notes peak around −18 to −10 dB, never near 0 dB: a clipped take can't be repaired. Monitor stays off; most audio interfaces let you hear yourself directly without delay.
- Action: *Record from bar {n}* — tooltip: Start recording on the armed track one bar before the first line of words. Press Stop when you're done.
- Status, no input: No audio input was found. Connect a microphone or an audio interface, then choose it in Audio/MIDI Settings. No track: Add an audio track to record on. Unarmed: Press R on {track} to arm it. Silent: The last take came in silent, so it was removed. Check the input chip next to R and the level (Setting the level).
- Fix: *Add a Recording track* — tooltip: Add an audio track called Recording and show its R button
- Done note: Your take is {n} bars long, on {track}.

**Step 6. Listen back** — *Hear how you really sound*
- Do: Click Play your take and listen through it, with the karaoke version. Listen once just for the timing, once for the pitch.
- Why: Your voice on a recording sounds higher and thinner than in your head, and almost everyone finds that strange at first. While you sing you hear yourself through the air and through the bones of your head, which adds low end; a recording has only the air part, closer to what others hear. Listen for where you came in late or drifted off the note, then record another take and keep the best.
- Action: *Play your take* — tooltip: Move the playhead to the start of your take and play
- Counting: Heard {n} of {m} bars of your take. Status, recording: Stop recording first.
- Done note: You listened back to your take.

**Completion:** You made a karaoke version of a song, sang it from the words and heard yourself back. Record a few more takes and keep the best one (How recording works · Takes and loop recording). The singing guide shows how to breathe, warm up and sing in tune.
**Carry-on:** *Open the singing guide* — tooltip: Open Singing techniques at Your voice and your song.

### Test numbers

| Check | Expected | Tolerance |
|---|---|---|
| Song clip of 3.9 bars / 4 bars | not done / done | — |
| Vocals at −6 dB / −5.9 dB / muted | done / not / done | 0.05 dB |
| Timed lines in span: 1 / 2 | not / done | — |
| Sung-along distinct bars: same bar 4 times / 4 bars | 1 / done | — |
| Take of 1.9 bars / 2 bars overlapping words | not / done | — |
| Take of 2 bars: listened bars needed | 2 | — |
| 4 bars at 120 BPM | 8 s | 0.001 s |

## Steps

Group A, engine (`./build.sh --test` at step 4):
- [x] 1. The engine's recording report says which clips the recording made. Tests: a fake-device recording on one armed track reports its one clip; a loop of two passes reports two; a silent take reports none. Done when: existing recording tests pass unchanged.
- [x] 2. `gen::karaoke`: texts from *Theory*, the song span, the vocals-track rule, steps 1–3's done rules, statuses, done notes. Tests: the step 1–3 rows of *Test numbers*, the name rule, a renamed karaoke Vocals track still found through the record.
- [x] 3. The two bar counters, the take record and steps 4–6's rules and statuses (no input, no track, unarmed, silent, recording). Tests: the step 4–6 rows; a deleted take clip unticks steps 5 and 6; a take never becomes the song clip.
- [x] 4. Register in `gen::exercises` (`Exercise::karaoke`, info, links, `Fix::addRecordingTrack`). Tests: every link names an existing topic; distinct settings stems; `numExercises` 8. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 7):
- [x] 5. `MainComponent`: Help command and clip menu item, `showKaraoke`, context, karaoke record from `onStemsAdded`, counters fed from the 5 Hz timer, resets in `afterReplace` and Start over. Check: the window opens at step 1 and reopens at its place.
- [x] 6. Actions and Show me per the table: `ArrangementView` entry points for Remove vocals and Detect lyrics on a clip, Record from bar N, Play your take; pointers on the clip areas, Sing along button, R and Record. Check: each button opens the right dialog on the right clip.
- [x] 7. `ScreenshotRunner`: `"exercise": "karaoke"` and a `karaokeSong` scene built without models (the example recording, a Karaoke track and a muted Vocals track, two timed lyric lines, optionally a take clip), keys `exerciseSungAlong`, `exerciseTakeRecorded`, `exerciseListened`, `exerciseNoInput`. Done when: `./build.sh --test` passes without warnings.

Group C, check and docs:
- [x] 8. Real-app check: shots of each step before and after (karaoke scene), step 2 with Demucs missing, step 5 with no input, no track and unarmed, Show me on steps 2, 4, 5, 6, the clip menu item, the longest step at 740 high (adjust `windowHeight` if clipped). Each must show the right tick, status line and link chips.
- [x] 9. Docs: README Features bullet (exercises), `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md` *A guided exercise* (karaoke as the example of an exercise reading a job's result tracks and a recording), issue #197 progress and #195's checklist.

## Tests

As *Test numbers* and the step tests above; plus: no done rule ticks from an action alone (step 4 with Sing along showing but not playing stays at 0), every text is non-empty, and step 3's status names the Whisper model's size.

## Website hand-off

- `features.html`, the guided-exercises group: a short section; `learn.html`'s exercise list.
- Screenshot: the exercise window at step 4 beside Sing along over the karaoke scene (Karaoke and muted Vocals tracks, Lyrics row).
- No *How it's made* change (no new library or model).

## Left for later

- Advice on the take's pitch and timing (#28, Practise singing: record yourself and get advice).
- Choosing a key that suits the voice and transposing the karaoke version (#198).
- Lyrics in other languages (Whisper's multilingual models).
- The Start panel tile and an exercise list (#209, #195).

## Changes while building

**Group A (engine) built; `DawItTests` builds with no warnings and "All checks passed".** Hand-off for Group B:

- **Engine:** `Engine::TakeReport::clips` (`std::vector<const AudioClip*>`, identities only) lists the clips a recording made, in order (`src/engine/Engine.h`, filled in `finishAudioRecordings`). Tests added to the existing recording blocks (one clip, four loop passes, silent: none); the existing recording tests pass unchanged.
- **`gen::karaoke`** (`src/generate/Karaoke.{h,cpp}`, both CMake targets): `Progress` (karaoke record, take record, `sungBars`, `listenedBars`, `hasOutputDevice`, `hasInput`, `recording`, `demucsInstalled`, `whisperInstalled`), `KaraokeRecord` and `TakeRecord` with `makeKaraokeRecord (engine, added, vocals)` and `makeTakeRecord (engine)`, the counters `SungAlongCounter::feed (bar, holdsSyllable, playing, singAlongShowing)` and `ListenCounter::feed (bar, insideTake, playing, recording, heard)`, and finders for Group B's actions and Show me: `findSongClip`, `findRemoveClip`, `findDetectClip`, `findVocalsTrack`, `findVocalsClip`, `findTake (engine, progress, preferHeard)`, `findTrackOf`, `findRecordTarget` (armed / unarmed track to press R on / none), `getRecordStartBar` (0-based; the label shows +1), `getSongSpan`, `isSungBar`, `getBarOf`, `isClipHeard`, `getListenBarsNeeded`, `isInEdit`.
- **`gen::exercises`:** `Exercise::karaoke` (stem `learn.karaoke`, `numExercises` 8, window height 740 via `karaoke::windowHeight`, to confirm by shot in Group C). `Context` gained `karaokeRecord`, `karaokeTake`, `karaokeSungBars`, `karaokeListenedBars`, `hasInputDevice`, `recording`, `demucsInstalled`, `whisperInstalled` for `getExerciseContext` to fill (`hasOutputDevice` is the existing field).
- **`Fix`** gained `addRecordingTrack` and `detectLyrics` (steps 4 and 5 without timed lines offer Detect lyrics). Group B's `runFix` must handle both; steps 2, 3, 5 and 6's own actions (`Fix::none`) are dispatched on the step in `runExerciseAction`. The Record button is disabled in Group B when `findRecordTarget` is `unarmed` (status "Press R on X to arm it").
- **Surprise:** `StemSplitter::onStemsAdded` passes only the instrumental tracks (`karaoke.instrumental`); the Vocals track (`karaoke.vocals`) is not in it. Group B has to forward it too (for example `onStemsAdded` extended or a second callback) and call `makeKaraokeRecord (engine, instrumental, vocals)`. Split into stems passes all stems: call it with `vocals` null and the Vocals track found by name, or the name rule applies later.
- **Departure from the plan:** `Fix::detectLyrics` is new (the plan named only `addRecordingTrack`), so the status line can turn the button into Detect lyrics.
- Step 5's text shows "Record from bar N" with the bar one before the first words (never before bar 1); an unarmed empty audio track counts as "press R on it", a track that already holds clips does not.
- No theory, library, model or licence change; `SOURCES.md` edit is the planner's. First steps: no change.

**Group B (UI) built; full `./build.sh --test` passed ("All checks passed", no warnings).** Hand-off for Group C:

- **Wiring (`MainComponent`):** Help > *Sing Karaoke to a Song...* (`karaokeExerciseCommand`, after Map a Song's Structure), `showKaraokeExercise`, `getExerciseContext` fills the karaoke fields, `getKaraokeProgress`, records `karaokeRecord` / `karaokeTake` and counters `karaokeSung` / `karaokeListen` (fed at 5 Hz by `feedKaraoke`, which also adopts a new recording report as the take record and clears the listened bars), cleared by `resetKaraokeMemory` in `afterReplace` (everything) and on Start over (counters only). `runKaraokeAction` and `showKaraokeTarget` hold the actions and Show me per the plan's table.
- **Forwarding the Vocals track:** `StemSplitter::onStemsAdded` is now `(tracks, Track* vocals)` (null for Split into stems; `noteKaraokeTracks` then takes the track named Vocals); `ArrangementView::onKaraokeAdded` forwards it. New in `ArrangementView`: `detectLyrics (Track&, AudioClip&)`, `onSingKaraoke`; `TrackLanes`: menu item 25 *Sing karaoke to this song...* after *Remove vocals (karaoke version)...*; `TransportBar::getRecordButton`.
- **Departures:** (1) the button of step 5 with an unarmed track, and step 6 with no take or while recording, is disabled in `ExerciseWindow` (its tooltip stays the step's); clicking an unarmed step 5 never happens, but the code also points at R. (2) The screenshot key for step 6 is `exerciseListenedBars`, not `exerciseListened` (that one already sets the Sound Better record). (3) *Record from bar N* uses `TransportShortcuts::toggleRecording` after setting the playhead, so the count-in applies as usual and a failed start shows the usual message.
- **Screenshot runner:** `"exercise": "karaoke"`; `"karaokeSong": true` (muted `Song` track, `Karaoke` track, muted `Vocals` track, two timed lines at beats 8 and 20, records set; works on its own or after `songRecording`), `"karaokeTake": true` (2-bar take on an armed `Recording` track at beat 8, over words), `"karaokeVocalsDb"`, `"exerciseSungAlong": n`, `"exerciseListenedBars": n`, `"exerciseNoInput": true`, with `"exerciseStep"`, `"exerciseAction"`, `"exerciseShowMe"` as for the others. Documented in `DevScreenshots.h`. Only one shot was taken, to check the scene works (step 4, 2 of 4 bars: `/tmp/kashots/k4.png`, texts and chips fit at 740 high); the real-app check is Group C's.
- **Not done (Group C):** screenshots of every step, Show me checks, the clip menu item, window height at the tallest step, docs.
- First steps: checked, no change.

**Group C (real-app check and docs) done; full `./build.sh --test` passed ("All checks passed", 4417 checks, no warnings).**

- **Shots** (screenshot runner, scale 1, 1500 x 940 window, files in `/tmp/kashots/s/`, shot lists `A.json` to `H.json` there; `"song": "empty"` with the `songRecording` scene plus `karaokeSong`): step 1 empty (before), step 2 before (song only), step 2 done (muted vocals), step 3 before (Whisper size line, "Demucs isn't needed again") and done ("2 lines of words, from bar 3 to bar 7."), step 4 at 0 and 4 bars, step 5 with no input, no track (*Add a Recording track*), an unarmed track (*Press R on Recording to arm it.*, button disabled) and done (take), step 6 before (disabled with "Record a take first (step 5).") and at 2 of 4 bars. Show me: step 2 (outline on the song clip), 4 (Sing along button), 5 (R button of the unarmed track) and 6 (the take clip). Each showed the right tick, status line and link chips. The window at 740 high has about 190 px spare at the tallest step, so `windowHeight` stays.
- **Fixed while checking:** (1) the step 5 *why* text, the completion text and step 2's loud-vocals status showed mojibake (UTF-8 escapes in plain string literals): now `juce::String::fromUTF8`. (2) `exerciseSungAlong` / `exerciseListenedBars` without `karaokeSong` wiped the records: new `MainComponent::setKaraokeCountsForShot` sets only the counters. (3) The headless runner has no audio input, so `exerciseNoInput` is now tri-state (`true` none, `false` present, absent: the real device); new key `karaokeUnarmedTrack` adds an empty unarmed Recording track.
- **Follow-up checks (added after Group C):** the runner gained permanent keys, none of which touch installed models: `"clipMenu": "Track"` with `"capture": "popupMenu"` (the clip's right-click menu), `exerciseDemucsMissing` / `exerciseWhisperMissing` (pretend a model is missing), `karaokeArmedTrack`, `karaokeSilentTake`, and each shot now dismisses an open menu. Shots (`/tmp/kashots/s/`, lists `M` to `T`): `m1` the clip menu with *Sing karaoke to this song...* right after *Remove vocals (karaoke version)...*; `n1` step 2 with Demucs missing ("Demucs (84 MB) is downloaded from Hugging Face..."); `n2` step 3 with both models missing and no Vocals track (Whisper 488 MB, Demucs 84 MB); `o1` step 1 done ("Song", 8 bars long.); `p1` vocals loud (status), `p2` vocals at -6 dB (guide note); `q1` the last take came in silent; `r1`/`r2` step 5 armed and its Show me; `t1` step 6 done ("You listened back to your take."); `s1` all six done: the completion text with *Open the singing guide* and *Start over*. All as expected. Remaining limitation: step 6 while recording ("Stop recording first.") has a unit test but no shot, as the runner cannot record.
- **Docs:** README Features bullet, `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md` (*A guided exercise*), `docs/ui-design.md` (window height). #197 has a progress comment; #195's checklist reads "(built, not committed)". Project status stays In Progress for the coordinator.
- **For dawWebsite:** `features.html` (guided exercises: a short section), `learn.html`'s exercise list; shots to take with `{"song":"empty","scene":"songRecording","karaokeSong":true,"karaokeTake":true,"exercise":"karaoke","exerciseStep":3,"exerciseSungAlong":2}` (window) and the same main window; no library, model or guide added, so no *How it's made* change.
- **Theory:** no fact or text outside the plan's *Theory* section. First steps: checked, no change (no renamed command or control; the start panel, inspector welcome and First Song Exercise untouched, so no new shots).
