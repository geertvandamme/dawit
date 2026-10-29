# Tempo map: implementation plan

The plan for *A tempo map: songs whose tempo changes* in [FEATURE_IDEAS.html](FEATURE_IDEAS.html).

A band that plays without a click doesn't keep one tempo: a rock song can start at 96 BPM, push to 104 in the chorus and slow down at the end. With one tempo for the whole song, DawIt's bars drift away from the recording: after a minute the chords, sections, lyrics and the metronome are a beat or more off. A **tempo map** gives every bar its own tempo, so the song's bars follow the band from the first bar to the last.

## Contents

1. [What the user does and sees](#1-what-the-user-does-and-sees)
2. [The data: TempoMap](#2-the-data-tempomap)
3. [The engine](#3-the-engine)
4. [Everything that turns beats into seconds](#4-everything-that-turns-beats-into-seconds)
5. [Detect tempo: following the recording](#5-detect-tempo-following-the-recording)
6. [The Tempo row](#6-the-tempo-row)
7. [Saving and exporting](#7-saving-and-exporting)
8. [Tests](#8-tests)
9. [Steps](#9-steps)
10. [Open questions and later](#10-open-questions-and-later)

---

## 1. What the user does and sees

- **Detect tempo on a drifting recording:** right-click the clip, *Detect tempo...*. When the tempo moves (the dialog already says "its tempo moves between 94 and 106 BPM"), the first choice is **Follow the recording, bar by bar**, next to the steady tempo, half and double. *Set tempo* then:
  - gives every bar of the recording its own tempo, measured from one detected downbeat to the next, so each bar line lands on the band's downbeat;
  - moves the clip so its first downbeat starts a bar (needed to follow it);
  - replaces the song's tempo.
- **What follows the band:** the bars and beats, so everything that sits on them: chords, sections, lyrics, MIDI clips, Play along, the metronome and the count-in. The recording itself plays at its own speed, untouched: nothing is stretched.
- **The Tempo row:** a row under the ruler, shown while the tempo changes. It draws the tempo as a line over the bars, with the BPM at the slowest and fastest points, and says what's under the mouse ("Bar 12 · 101.37 BPM"). Right-click: *Make the tempo steady* (the average over the song), and the guide to note lengths and tempo. It folds with the other song rows (Cmd+3) as a line of its own.
- **The transport:** the BPM shows the tempo at the playhead and follows it while the song plays. While the tempo changes, the BPM can't be dragged or typed (that would throw the band's timing away); its tooltip says why and how to make the tempo steady again.
- **Everything else stays the same** for a song with one tempo, which is what every song has until a recording is followed.

## 2. The data: TempoMap

`src/engine/TempoMap.{h,cpp}`, plain C++ with no UI, tested on its own.

- **A list of tempo changes:** from `beat` on the song plays at `bpm`, until the next change. The first change is always at beat 0, so there's always a tempo; one change means one tempo for the whole song. The tempo is constant between changes (steps, no ramps yet).
- **Tidied when made:** sorted, the first moved to beat 0, BPM kept within 20 to 300 like the transport, and a change to the tempo already playing dropped.
- **Seconds and beats:**
  - `beatToSeconds (beat)` adds up each stretch's beats × 60 / BPM from the start of the song (beat 0 is 0 s; before it, the first tempo carries on);
  - `secondsToBeat (seconds)` is the reverse;
  - `getSecondsBetween (from, to)` and `getBeatAfter (fromBeat, seconds)`, which is what an audio clip needs: it starts at a beat and lasts a number of seconds;
  - the seconds at each change are kept, so each lookup is a binary search.
- **Questions:** the tempo at a beat, the next change after a beat (for the audio thread), whether it's steady, the slowest, fastest and average tempo over a stretch.

## 3. The engine

- **`Engine` holds a `TempoMap`** instead of the `std::atomic<double> tempo`. Like the chords and sections, it's song data: changed on the message thread, under `getLock()`, with a change message.
  - `getTempoMap()` and `setTempoMap (TempoMap)`;
  - `setTempo (bpm)` stays, and sets one tempo for the whole song;
  - `getTempo()` becomes the tempo at the playhead, for the transport and the guides that say "at your song's 96 BPM".
- **Playback:** `processBlock` already renders under the try-lock and splits each block at the loop end. It now also splits at every tempo change, and takes the tempo of each piece from the map. Inside a piece the tempo is constant, so the metronome, MIDI and recording code keep their simple samples-per-beat sums.
- **Audio clips:** the read position of a clip is the seconds between its start and the playhead, taken from the map (`RenderContext` carries the map), so a recording plays straight through any number of tempo changes.
- **Plugins** get the tempo of the piece being rendered from the play head, and the time in seconds from the map.
- **The heard position** (latency taken off, for tapping lyrics and Play along) goes back the latency's seconds through the map.

## 4. Everything that turns beats into seconds

An audio clip keeps its length in seconds, so its length in beats depends on where it starts: `Clip::getLengthBeats (const TempoMap&)` and `getEndBeat (const TempoMap&)` replace the BPM versions (a MIDI clip ignores the map). The places that use them, or multiply by 60 / BPM:

| Where | What changes |
|---|---|
| `Engine` | duplicating a clip after itself, splitting and joining audio clips (the cut in seconds), the song's end |
| `TrackLanes` | an audio clip's width and its waveform, column by column through the map |
| `WaveEditor` | the bar and beat lines, the playhead and the clip's length in bars |
| `ArrangementView` | the length of the timeline |
| `TransportBar` | the time display (minutes and seconds) |
| `Bounce`, `BounceDialog` | the length of the range in seconds |
| `ChordFinder`, `StructureFinder` | where each bar is in the recording |
| `LyricsFinder` | the beat of each sung word |
| `AudioToMidi` | the beat of each found note |
| `MidiExport` | a tempo event per change |

## 5. Detect tempo: following the recording

`TempoFinder` already returns every beat's time and which beat is the first downbeat. `tempofinder::toTempoMap` turns them into the song's map:

- the downbeats are every fourth beat from the first one; bar *k* lasts from downbeat *k* to downbeat *k + 1*, and its tempo is 240 / its length in seconds (4 beats);
- the first downbeat goes on the bar line nearest to where it is now (as *Move it so its first downbeat starts a bar* does), and the clip moves with it; the tempo before it is the first bar's, so the clip's start lands right;
- after the last whole bar the tempo stays at the last bar's;
- the tempos aren't smoothed or rounded: each bar line has to land on its downbeat, and smoothing would add up to a drift. A bar that measures a little fast because a hit came early is still drawn as it is.

The dialog offers it first when the tempo isn't steady, with the range ("Follow the recording, bar by bar: 94 to 106 BPM"), and explains what it does. The tempo found as one number stays available, for a song that drifts only a little.

Chords, sections and lyrics detected after this land on the band's bars, because the finders take the map.

## 6. The Tempo row

`src/ui/TempoLane.{h,cpp}`, a row above the Sections row, with its header in `ArrangementView`:

- shown only while the tempo changes (`! isSteady()`), and as a fourth line in the folded rows;
- the tempo as a line, scaled between the song's slowest and fastest tempo (with a few BPM of margin, so small changes don't look huge), with the BPM written at the start and at each change where there's room;
- the tooltip says the bar and its tempo; a click moves the playhead, as on the other rows;
- the right-click menu: *Make the tempo steady (98.4 BPM, the average)* and *Note lengths, beats and tempo...* last, which opens the rhythm guide;
- read-only for now: dragging the points and drawing ramps comes later.

## 7. Saving and exporting

- **Projects:** `"tempo"` stays (the first tempo), and a song whose tempo changes adds `"tempoChanges": [[beat, bpm], ...]`. Older projects have no `"tempoChanges"` and load with one tempo, as before; a project with changes opened in an older DawIt plays at its first tempo.
- **MIDI files:** a tempo event at each change, so a DAW plays the exported notes in time with the band. A single clip's file starts at the tempo where the clip starts.
- **Bounce:** the range's length in seconds comes from the map; the rendering follows it through `processBlock`.

## 8. Tests

- **TempoMap:** tidying (sorting, the first at beat 0, limits, repeats dropped), beats to seconds and back across changes and before beat 0, the seconds between two beats, the next change, slowest, fastest and average.
- **Playback:** an audio clip's samples played back through a tempo change come out at the right sample (a click in the recording heard at its own time), and MIDI notes after a change start at the time the map gives; the plugin play head's tempo per piece.
- **Clips:** an audio clip's length in beats at different starts, splitting it at a beat after a change, duplicating it.
- **Detection:** the test band speeding up from 100 to 110 BPM turned into a map whose bar lines are within 25 ms of the band's downbeats all the way through, while one steady tempo would be off by more than a beat at the end.
- **Projects:** a map saved and loaded back, and an old project without changes.
- **MIDI export:** the tempo events of a map.

## 9. Steps

1. This plan. **Done.**
2. `TempoMap`, the engine, playback, clips, the views, saving and exporting, with tests. A song with one tempo behaves exactly as before. **Done.**
3. *Detect tempo* following the recording, and the Tempo row (read-only). **Done**, checked in the app on a band speeding up from 96 to 106 BPM: the bars stay on its kicks for the whole minute, and *Detect chords* afterwards puts its chords on the right bars.
4. Later: editing the Tempo row (drag a change, add or remove one, ramps from one tempo to another), tempo changes in a song you write yourself, and a tempo guide topic on why bands drift and how a click track works.

## 10. Open questions and later

- **Ramps:** a gradual change (a ritardando at the end) needs a tempo that changes inside a stretch; beats to seconds then integrates 60 / BPM over the ramp. Steps per bar are close enough for following a band.
- **Beats, not only bars:** a map per beat would follow a band even more closely, but every beat's small error would show as a jumpy tempo line. Per bar is the usual compromise.
- **Time signatures:** the bars are 4 beats, as everywhere in DawIt for now.
- **Stretching instead:** stretching the recording to one tempo (warping) changes its sound and needs a time-stretching library with a licence that fits; it's the opposite of studying the song as it was played, so it isn't planned.
- **Undo:** setting the tempo replaces the map; with *Undo and redo*, the old map comes back.
