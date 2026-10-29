# Snap detected lyrics to the singing

Status: Built, not committed
Idea: [Detect lyrics from audio](../FEATURE_IDEAS.html#detect-lyrics-from-audio) (a part of it)

## Goal
Whisper's word times on singing are often a little off. After Whisper, each word's start is moved to the nearest onset of the vocals (where the voice begins), and syllables inside a word start on the voice's own onsets where it has them. The words on the lyrics track then line up with the singing.

## Decisions for you
1. How far a word may move. Options: 0.1 s, 0.15 s, 0.25 s. **Recommended:** 0.15 s: Whisper's error on singing is typically 0.1 to 0.2 s, and at 120 BPM an eighth note is 0.25 s, so a bigger reach would hop to the next word's onset. Answer: user said "implement option two"; 0.15 s used.
2. The setting. Options: always on, or a choice in the dialog. **Recommended:** a "Word timing" drop-down (Snap words to the singing / As Whisper heard them), default snap, remembered as `analysis.lyricsSnap`. Answer: as recommended (a drop-down, like the dialog's other choices, instead of a checkbox).

## What the user does and sees
In *Detect lyrics (English only)...* a new *Word timing* choice with a tooltip. Nothing else changes; the found words sit closer to where they are sung. A word with no clear onset within 0.15 s stays where Whisper put it.

## Code
- `src/analysis/OnsetFinder.{h,cpp}` (new, no UI): `onsets::findOnsets` on mono audio: spectral flux of 32 ms Hann windows every 10 ms, 100 Hz to 5 kHz, log-compressed; peaks above the local average, not in parts more than 40 dB under the loudest, not where the level falls, at least 60 ms apart; time corrected by the measured 4 ms bias.
- `lyricsfinder::snapToOnsets` moves word starts (nearest onset within `longestSnap`, not nearer the next word, not past neighbours: 20 ms between starts, at least 50 ms long) and fills `Word::onsets`; `getSyllableTimes` places syllables on a word's own onsets within 40% of a syllable (at most 0.15 s), else evenly; `toLyrics` uses it. No onsets: identical to before.
- `LyricsActions`: the choice, the setting, and the call after `cleanUp` on the muted mono Whisper heard. No saved data changes.

## Libraries and models
None (juce_dsp's FFT).

## Theory
None.

## Steps
- [x] 1. OnsetFinder, snapping, syllable times, CMake in both targets.
- [x] 2. Tests in `runLyricsFinderTests`.
- [x] 3. Dialog choice and setting.
- [x] 4. Docs: README, FEATURE_IDEAS.

## Tests
Six synthetic notes with known starts: onsets within one 10 ms hop (3 ms measured); silence none; words 80 ms early or late snap to within a hop; a word 0.3 s from any onset stays; order, spacing and length kept; syllables land on a word's onset when near the even split, else even; `toLyrics` puts a syllable at beat 3.2 instead of 3.0.

## Website hand-off
A sentence on the lyrics feature: words are fitted to where the voice begins. No new screenshot beyond the dialog's *Word timing* choice. No library added.

## Left for later
Checking accuracy against hand-timed lyrics on real recordings; vowel onsets in legato singing; the lyrics guide has no accuracy section, so none was added.

## Changes while building
- The choice is a drop-down rather than a checkbox (the alert window has no checkbox).
