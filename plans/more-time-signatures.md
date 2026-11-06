# More time signatures: write songs in 3/4, 6/8, 5/4 and more, not only 4/4

Status: Released
Idea: [#137 — More time signatures: write songs in 3/4, 6/8, 5/4 and more, not only 4/4](https://github.com/geertvandamme/dawit/issues/137)

## Goal
A song gets one time signature (3/4, 6/8, 5/4, 7/8, 12/8, 2/2 and others) that the ruler, grid, Snap, metronome, count-in, score, MIDI export, plugins and generators follow, instead of a fixed 4/4. A beginner can write a waltz or a 6/8 ballad, hear its bars clicked, read it in the score, and learn from the rhythm guide what the two numbers mean and how the song they are writing is counted.

## Decisions for you
1. **Scope of the first version.** Options: (a) one signature for the whole song; (b) a signature per bar, stored like the tempo map. **Recommended:** (a). It covers waltzes, ballads, shuffles and odd metres; every bar helper gets the signature through one place, so (b) can follow later (Left for later) without touching the callers again.
   Answer: (a) one signature for the whole song. (user, 2026-11-06)
2. **What BPM counts.** Options: (a) always quarter notes; (b) the felt beat (a dotted quarter in 6/8). **Recommended:** (a), as MIDI files (tempo per quarter note) and DAWs such as Logic Pro do, so the tempo map, audio clips and plugins stay unchanged. The tooltip and the guide give the felt beat: 6/8 at 120 BPM is 80 dotted quarters a minute.
   Answer: (a) always quarter notes. (user, 2026-11-06)
3. **Metronome in 6/8, 7/8 and other /8 metres.** Options: (a) click every eighth, loud on 1 and medium where each group starts; (b) click only the big beats (two a bar in 6/8). **Recommended:** (a): it makes the grouping audible (2+2+3 in 7/8), and in x/4 and x/2 nothing changes (a click per bottom note, loud on 1).
   Answer: (a) click every eighth, loud on 1, medium on group starts. (user, 2026-11-06)
4. **Changing the signature of a song with content.** Options: (a) a dialog: *Keep each part in its bar* (default), *Keep the timing*, *Cancel*; (b) always keep bars. **Recommended:** (a). Keeping bars matches the issue's acceptance check; keeping the timing is what someone needs whose recording was barred in the wrong metre. One undo step either way.
   Answer: (a) the dialog, *Keep each part in its bar* as default. (user, 2026-11-06)
5. **Grouping of odd /8 and /16 metres.** Options: (a) fixed for now, twos and a final three (5/8 = 2+3, 7/8 = 2+2+3); (b) a grouping menu now. **Recommended:** (a), said in the guide and the tooltip; a grouping choice is Left for later.
   Answer: (a) fixed grouping for now. (user, 2026-11-06)
6. **Which signatures.** **Recommended:** a menu of nine (2/4, 3/4, 4/4, 5/4, 6/8, 7/8, 9/8, 12/8, 2/2) and *Other...* for any top number 1 to 15 over 2, 4, 8 or 16, as the issue proposes.
   Answer: As recommended: nine common ones plus *Other...*. (user, 2026-11-06)
7. **Generators outside 4/4.** Options: (a) generators follow the bar length; built-in 4/4 patterns (chord styles, song-template drums) are cut at the bar line and say so; ChatMusician (a fixed 4/4 format) is offered only in 4/4; (b) disable all generating outside 4/4. **Recommended:** (a). New from Style always makes a 4/4 song.
   Answer: (a) generators follow the bar length and say so. (user, 2026-11-06)
8. **Older DawIt versions.** **Recommended:** a song not in 4/4 saves as format version 4, so an older DawIt says "saved by a newer version" instead of playing it in 4/4 with every bar wrong. A 4/4 song keeps the version it had (1, 2 or 3).
   Answer: As recommended: format version 4 for songs not in 4/4. (user, 2026-11-06)

## What the user does and sees
- **Transport bar:** a small button showing the signature ("3/4") right of BPM. Click: a menu of the nine with a hint each ("3/4  three beats: waltzes", "6/8  two beats of three", "7/8  2+2+3", "12/8  four beats of three: slow blues", "2/2  cut time: two half notes"), *Other...*, and last *Time signatures guide* (`guideicon::addItem`, opens the rhythm guide at *Time signatures*). *Other...* opens a small dialog with two number boxes (top 1 to 15; bottom 2, 4, 8, 16). Tooltip in *Theory*.
- **Ruler:** draws the signature at its left edge; clicking it opens the same menu; the ruler's right-click menu gets a *Time signature* submenu above its guide item. Bar numbers and lines follow the bar length.
- **Change dialog** (only when the song has clips, chords, sections, lyrics or tempo changes): *Keep each part in its bar*, *Keep the timing*, *Cancel*; text in *Theory*. One undo step, "Time signature 3/4".
- **Grid everywhere** (arrangement lanes, chord, section, lyrics and tempo rows, piano roll, wave editor): three line weights, bar, beat (group start) and pulse (the bottom note, only where it differs from the beat: eighths in 6/8 and 7/8, quarters in 2/2). Snap's *1 bar* is a whole bar of the signature.
- **Position display:** bar.beat.sixteenth where beat counts the bottom note (6/8: 1 to 6) and the third number the sixteenths in it (x/4: 1–4, x/8: 1–2, x/2: 1–8, x/16: always 1). Timeline tooltips follow ("Bar 9, beat 4").
- **Metronome and count-in:** one bar of the signature; three click levels (see Code). The metronome menu's tooltip says how it clicks in this signature.
- **Score:** the signature at the start of the first line (two-digit tops such as 12), bars of the right length, beams and rests by beat group; the time-signature hover tooltip describes it.
- **Generate panel, chord styles:** in a song not in 4/4 a line: "This pattern is written in 4/4; in 3/4 each bar plays its first 3 beats." ChatMusician says "ChatMusician writes bars of 4/4; switch the song to 4/4 to use it" and its Generate button is off.
- No app settings: the signature belongs to the song.

## Data and saving
- `TimeSignature` (numerator, denominator) on `Engine`, default 4/4, cleared to 4/4 by `resetEdit`. Part of undo (`undo::Snapshot`, `undo::describe` "Time signature 6/8").
- `ProjectFile.cpp`: a `timeSignature` key with numerator and denominator, left out at 4/4; a missing or invalid key loads as 4/4. A song with any other signature saves as version 4 (`formatVersion` becomes 4; the lower versions stay for songs without it).
- Tests: 6/8 save-and-load; an old project loads as 4/4; version 4 only when not 4/4; undo round trip of a *keep bars* change.

## The guide
New topics in the rhythm guide (`gen::rhythm`, `RhythmExplainer`), following recipes.md *Topics in an existing guide*. Its explain functions take the song's signature. Order and pictures:

| Topic | Summary | Picture |
|---|---|---|
| Bars and beats (changed) | 4/4, the beat, and the time signature | as now (4/4, 3/4, 6/8 rows) |
| Time signatures (new) | 2/4, 3/4, 4/4 and 2/2: the simple metres | a bar each of 2/4, 3/4, 4/4, 2/2; loud, medium and soft hits; playable |
| Compound and odd metres (new) | 6/8, 12/8, 5/4, 7/8: beats of three and uneven bars | a bar each of 6/8, 9/8, 12/8, 5/4, 7/8 with group labels; playable |
| Note values, Rests, Dotted notes and triplets, Counting, Tempo and time, Reading DawIt (changed text) | as now | as now; Tempo's seconds use the song's signature |
| Your song's time signature (new, last) | What your song's bars and clicks are | one bar of the song's signature: clicks as big, medium and small dots, groups bracketed, the bar's seconds; Play plays it at the song's tempo |

Theory Guides entry (`Guide::rhythm`): summary "Bars and beats, time signatures from 3/4 to 7/8, whole to sixteenth notes, rests, dotted notes and triplets, counting, tempo, and your song's metre."; add search words "waltz metre meter compound simple odd shuffle common cut alla breve 3/4 6/8 12/8 5/4 7/8"; opens-from adds "the time signature next to the tempo". Already on *Start here*. The reading guide's *Time signature, bars and tempo*, *Note values*, *Ties* and *Rests* texts change (no new topics); its header picture uses the song's signature.

## Code
Follows recipes.md *Song data that's saved*, *Topics in an existing guide* and *A command or menu item* (no new command; a transport control).
- **`TimeSignature`** (`src/engine/TimeSignature.{h,cpp}`, `daw::timesig` helpers): tidies to top 1–15 and bottom 2/4/8/16. Everything is in quarter notes (the timeline's beats): bar length = top × 4 / bottom; pulse = 4 / bottom; beat groups as in *Theory* F8; kind simple, compound or odd; bar ↔ beat; position text; `describe`; MIDI clocks per click. **One rule:** song code never writes a bar length; it asks the song's `TimeSignature`. Guide examples drawn in 4/4 keep 4/4 through a named example constant and say so.
- **Notes and beats never scale.** A quarter note is one beat in every signature; MIDI notes keep their beats inside their clip. The timeline, tempo map, audio clips (seconds) and plugin ppq are unchanged.
- **Changing the signature** is one Engine function under one lock, one undo step. *Keep the timing* only sets it. *Keep bars* maps every bar-anchored beat position (clip starts, MIDI clip ends, tempo changes, lyrics syllable starts and ends, loop points, playhead) as bar + offset × new bar / old bar, a pure tested function; chords and sections are by bar and don't move; audio clip lengths stay in seconds.
- **Audio thread:** the signature is read in `processBlock` under the try-lock it already holds (metronome, count-in, `getPosition`); set only under `getLock()`. No new atomics. Metronome: a click per pulse; bar start loud (0.5, 1760 Hz as now); group start medium (0.35, 1320 Hz) only where groups differ from pulses; other pulses soft (0.2, 1320 Hz); x/4 sounds as today. Count-in clicks the same pattern over one bar (negative beats, proper modulo).
- **MIDI export** writes its own FF 58 event (top, power of two, clocks per click, 8): JUCE's `timeSignatureMetaEvent` writes cc 1, bb 96. Plugin playhead: the signature and `ppqPositionOfLastBarStart` = whole bars.
- **Notation:** units per bar from the bar length (12 per quarter stays); R9 splits at beat groups (and at the bar's middle only when it has four equal groups: 4/4, 12/8); R10 one rest per empty group (dotted-quarter rest in compound); R11 beams within a group; R7 uses sixteenths only in compound metres (no duplets yet); `Score` carries the signature for the painter.
- **Risky:** the refactor is wide (about 60 files mention a bar of 4). Do it area by area with a 3/4 and 6/8 test per area; the Group B check greps for leftovers. The format version bump. The metronome change on the audio thread.

## Libraries and models
None. JUCE and existing code only; `LICENSE`, `licenses/` and the README *License note* don't change.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| F1 | Top number: how many of the bottom note fill a bar; bottom: which note (2 half, 4 quarter, 8 eighth, 16 sixteenth), a power of two. | Wikipedia *Time signature* | exact |
| F2 | Bar length in quarter notes = top × 4 / bottom: 3/4 = 3, 6/8 = 3, 7/8 = 3.5, 9/8 = 4.5, 12/8 = 6, 5/4 = 5, 2/2 = 4, 7/16 = 1.75. | derived | exact |
| F3 | Simple metres: top 2, 3, 4; the beat is the bottom note and splits in two. | Wikipedia *Time signature* | exact |
| F4 | Compound metres: top 6, 9, 12; the beat is a dotted note of three bottom notes; 6/8 is two dotted-quarter beats of three eighths. | Wikipedia; OpenLearn 3.9 | exact |
| F5 | 6/4 is usually compound duple: six quarters in two groups of three. | OpenLearn 3.8 (search) | typical |
| F6 | C means 4/4 (common time); C with a line means 2/2 (cut time, alla breve). | Wikipedia *Time signature* | exact |
| F7 | Odd metres (5, 7, 11...) group in twos and threes, and the grouping depends on the piece: 7 as 2+2+3, 2+3+2 or 3+2+2; Blue Rondo à la Turk's 9/8 as 2+2+2+3. | Wikipedia *Time signature* | typical |
| F8 | DawIt's groups, in this order: top 1 one group; top 6, 9, 12, 15 groups of three bottom notes (6/4 too); top 2, 3, 4, or any other top over 2 or 4, one group per bottom note (5/4 five); other tops over 8 or 16 in twos with a final three when odd (5/8 = 2+3, 7/8 = 2+2+3, 8/8 = 2+2+2+2). | DawIt's choice | opinion |
| F9 | Take Five is in 5/4; Paranoid Android has passages in 7/8; slow blues shuffles are in 12/8; waltzes, minuets and mazurkas are in 3/4; jigs and tarantellas in 6/8. | Wikipedia *Time signature* | exact |
| F10 | In compound metre eighths are beamed in threes, one group per dotted-quarter beat; beaming in twos would suggest 3/4. | OpenLearn 3.9 | exact |
| F11 | How to write rests in compound metre varies; DawIt writes a dotted-quarter rest for an empty beat. | MuseScore discussion (search) | opinion |
| F12 | A whole rest stands for a whole empty bar in any time signature. | Wikipedia *Rest* (in SOURCES, reading music) | typical |
| F13 | MIDI files give tempo per quarter note; Logic Pro shows tempo as quarter notes a minute whatever the signature. | SMF 1.0 (FF 51); Logic Pro 9 manual p. 176 | exact / typical |
| F14 | Felt beat in compound metre = BPM ÷ 1.5: 6/8 at 120 → 80; 12/8 at 90 → 60. | derived (dotted quarter = 1.5 quarters) | exact |
| F15 | Seconds per bar = bar length × 60 / BPM: 3/4 at 120 → 1.5 s; 6/8 at 120 → 1.5 s; 7/8 at 120 → 1.75 s; 4/4 at 120 → 2.0 s. | derived | exact |
| F16 | SMF time signature FF 58 04 nn dd cc bb: dd the power of two (3 = eighth), cc MIDI clocks per click (24 per quarter), bb 32nds per quarter (8); 6/8 clicking dotted quarters has cc = 36. No event means 4/4. | Glatt's SMF page; spec example (search) | exact |
| F17 | A shuffle in 4/4 with triplet eighths is often written in 12/8 instead, slow blues especially; both sound the same. | Wikipedia; bluesharmonica.com (search) | typical |
| F18 | Beat 1 is the strongest; in 4/4 beat 3 is next; in 6/8 beat 4 carries the second accent. | existing rhythm and lyrics guides | typical |
| F19 | In 4/4 notation the middle of the bar stays visible (ties at beat 3); DawIt applies the same to 12/8. | Gould via MuseScore (in SOURCES); 12/8 by analogy | opinion |

Clocks per click (derived from F16, a click per group when groups are equal, else per pulse): x/4 24, x/2 48, 6/8 9/8 12/8 36, other x/8 12, 6/16 12/16 18, other x/16 6, 6/4 72.

### Topic texts

**Bars and beats** (changed: the last paragraph replaces "DawIt works in 4/4..."):
> Your song is in {sig}: {describe}. To choose another, click the time signature next to the tempo; the topics *Time signatures* and *Compound and odd metres* explain the common ones. The ruler numbers the bars; the lines between them are the beats.

**Time signatures** · "2/4, 3/4, 4/4 and 2/2: the simple metres"
> Time signatures
>
> A time signature is two numbers, one above the other. The top number counts; the bottom number says what it counts: 2 is a half note, 4 a quarter note, 8 an eighth note and 16 a sixteenth. So 3/4 is three quarter notes in a bar, and 6/8 is six eighth notes.
>
> When the top number is 2, 3 or 4, each beat is one of the bottom notes, and it splits into two halves. These are the simple metres:
>   2/4: two beats, ONE two. Marches and polkas.
>   3/4: three beats, ONE two three. Waltzes, minuets and mazurkas.
>   4/4: four beats, ONE two THREE four, with beat 3 a little stronger than 2 and 4. Most pop, rock, hip-hop and dance music. It is also written as a C, for common time.
>   2/2: two half-note beats, each split into two quarters. It holds as much as a bar of 4/4, but you feel two big beats instead of four, which suits fast music. It is also written as a C with a line through it: cut time, or alla breve.
>
> The first beat of the bar is the strongest. It is where a chord usually changes and a section starts, which is why chords and sections in DawIt belong to bars.
>
> The time signature doesn't change how long a note is. At 120 BPM a quarter note lasts 0.5 seconds in 4/4 and in 3/4; a bar of 3/4 just ends a beat sooner, after 1.5 seconds instead of 2.
>
> Each row in the picture is one bar. Play a row to hear it: the loudest note is beat 1.

**Compound and odd metres** · "6/8, 12/8, 5/4, 7/8: beats of three and uneven bars"
> Compound and odd metres
>
> When the top number is 6, 9 or 12, you count the bar in big beats of three. 6/8 has six eighth notes, but you feel two beats: ONE two three FOUR five six. Each big beat is a dotted quarter note (three eighths), and it splits into three, not two. These are the compound metres:
>   6/8: two beats of three. Irish jigs, tarantellas and many ballads.
>   9/8: three beats of three.
>   12/8: four beats of three. Slow blues: it sounds like 4/4 with every beat split into three.
>   6/4: usually two beats of three quarter notes.
>
> 3/4 and 6/8 both hold six eighth notes, grouped differently. 3/4 is three beats of two eighths (ONE and TWO and THREE and); 6/8 is two beats of three (ONE two three FOUR five six). Written music shows which by beaming the eighths in twos or in threes.
>
> Odd metres have a top number such as 5 or 7, which doesn't split into equal beats. Their beats are a mix of twos and threes, so some beats are longer than others:
>   5/4: five beats, usually felt as 3 + 2 or 2 + 3. Dave Brubeck's Take Five is in 5/4.
>   7/8: seven eighths, grouped 2+2+3, 2+3+2 or 3+2+2, depending on the music. Radiohead's Paranoid Android has passages in 7/8.
>   9/8 can be odd too: Brubeck's Blue Rondo à la Turk groups it 2+2+2+3.
>
> DawIt groups 5/8 as 2+3 and 7/8 as 2+2+3. The metronome clicks every eighth, loud on beat 1 and medium where each group starts, and the stronger grid lines show the same groups.
>
> Play a row and count along: the loudest note is beat 1, the medium ones start each group.

**Note values** (changed lines): "Whole note: 4 beats, a whole bar of 4/4. Snap: 1 bar in 4/4." and, after the fraction paragraph: "Snap's *1 bar* is always a whole bar of your song: 3 beats in 3/4, 3.5 in 7/8."

**Dotted notes and triplets** (added after the swing paragraph): "A dotted quarter is also the beat of 6/8 and 12/8. A shuffle in 4/4, with its eighths played as triplets, is often written in 12/8 instead, slow blues especially: each of its beats becomes a dotted quarter of three eighths. It sounds the same; only the counting on paper changes."

**Counting** (added lines after Triplets): "  In 3/4: 1 & 2 & 3 &.\n  In 6/8: 1 2 3 4 5 6, with the weight on 1 and 4."

**Tempo and time** (changed): the seconds list says "a bar of {sig} lasts {s}" (bar length × 60 / BPM), and a paragraph before *Typical tempos*:
> In DawIt the BPM always counts quarter notes, whatever the time signature, as MIDI files and DAWs such as Logic Pro do; lengths in beats (a clip of 12 beats, a note of 0.5 beats) are quarter notes too. In 6/8 and 12/8 the beat you feel is a dotted quarter, one and a half quarter notes, so at 120 BPM it goes at 80 a minute. Sheet music often writes such a tempo as a dotted quarter = 80.

**Reading DawIt** (changed first and third paragraphs):
> The ruler at the top numbers the bars and shows the time signature at its left. In the arrangement and the piano roll, the strong lines are bars, the lighter ones beats, and the faint ones the Snap grid. In 6/8 the lighter lines are the two big beats, and the eighths between them are fainter.
>
> The position display next to the transport shows where the playhead is as bar.beat.sixteenth, all counted from 1, and the time in minutes and seconds. The middle number counts the bottom note of the time signature. In 4/4, 1.1.1 is the very start, 1.3.1 is beat 3 of bar 1, and 5.3.2 is bar 5, beat 3, the second sixteenth: in counting, the "e" of 3 in bar 5. In 6/8 the middle number counts eighths, 1 to 6, so 2.4.1 is bar 2's fourth eighth: its second big beat.

**Your song's time signature** · "What your song's bars and clicks are" (built from the song):
> Your song
>
> Your song is in {sig}: {describe}. At {bpm} BPM a bar lasts {seconds} s.
> [compound] DawIt's BPM counts quarter notes. In {sig} the beat you feel is a dotted quarter, which goes at {felt} a minute ({bpm} ÷ 1.5).
> The metronome clicks {pulses} times a bar: {clicks}.
> [advice, see rules]
> To change the time signature, click {sig} next to the tempo. If the song already has clips, chords or lyrics, DawIt asks whether each part keeps its bar or its timing.

`{clicks}` examples: 4/4 "loud on 1, softer on 2, 3 and 4"; 3/4 "loud on 1, softer on 2 and 3"; 6/8 "loud on 1, medium on 4, soft on the others"; 7/8 "loud on 1, medium on 3 and 5, soft on the others". `{describe}` examples: 3/4 "three quarter-note beats in each bar"; 2/2 "two half-note beats in each bar"; 6/8 "two beats in each bar, each three eighth notes long (six eighths in all)"; 12/8 "four beats in each bar, each three eighth notes long (twelve eighths in all)"; 7/8 "seven eighth notes in each bar, grouped 2+2+3"; 5/4 "five quarter-note beats in each bar".

**Reading guide** (`ReadingGuide.cpp`):
- *Time signature, bars and tempo*: first paragraph ends "...it's sometimes written as a C, for common time, and 2/2 as a C with a line through it, cut time. The rhythm guide explains the others. Your song is in {sig}, written at the start of the first line." Second: "In every bar the notes and rests add up to exactly what the time signature says: four quarter notes in 4/4, six eighths in 6/8." Tempo mark, added: "In 6/8 and 12/8 DawIt still writes ♩ = {bpm}, because its BPM counts quarter notes; printed music often writes the dotted quarter instead, ♩. = {felt}." (only in a compound song.)
- *Note values*: replace the beaming sentences' ending with "...don't usually cross the middle of a 4/4 bar. In 6/8 and 12/8 the beat is a dotted quarter, so eighths are beamed in threes; beamed in twos, the bar would look like 3/4."
- *Ties*: add "In 6/8 a note that starts between the big beats is split at the next one, so both dotted-quarter beats stay visible."
- *Rests* (its whole-rest sentence): "A whole rest also stands for a whole empty bar, in any time signature. In 6/8 DawIt writes an empty big beat as a dotted quarter rest; printed music doesn't always agree on this."

**Tooltips and dialogs**
- Signature button: "Time signature {sig}: {describe}. Click to choose another. The tempo always counts quarter notes{compound: ; you feel the dotted quarter at {felt} a minute}."
- Ruler signature mark: "Time signature {sig}: {describe}. Click to change it."
- *Other...* boxes: "Top: how many notes fill a bar, 1 to 15." / "Bottom: which note is counted: 2 half, 4 quarter, 8 eighth, 16 sixteenth."
- Change dialog title "Change to {new}?", text: "A bar of {new} lasts {L new} beats instead of {L old}. Keep each part in its bar: clips, lyrics and tempo changes move with their bars, and chords and sections stay on theirs. The notes inside a clip keep their rhythm, so a {old} pattern now runs across the bar lines, and notes past a clip's new end are hidden, not deleted. Keep the timing: everything stays where it sounds and the bar lines move under it, so chords and sections move with the bars. Undo (Cmd+Z) puts it back."
- Metronome button, appended: "In {sig} it clicks {pulses} times a bar: {clicks}."
- Score time-signature hover: "Time signature {sig}: {describe}."
- Detect tempo result, in a compound song: "In 6/8 and 12/8 a song may be heard at its big beat: if the bars come out too slow, try 1.5 times the tempo."

### Example data
- *Time signatures* rows (bar length in quarters; velocities loud 1.0, medium 0.8, soft 0.6): 2/4 two quarters (1.0, 0.6), length 2; 3/4 three quarters (1.0, 0.6, 0.6), 3; 4/4 four quarters (1.0, 0.6, 0.8, 0.6), 4; 2/2 two halves (1.0, 0.6), 4.
- *Compound and odd* rows, labels 1..n: 6/8 six eighths, loud 1, medium 4, length 3; 9/8 nine eighths, medium 4 and 7, 4.5; 12/8 twelve eighths, medium 4, 7, 10, 6; 5/4 five quarters, medium 4 (3+2), 5; 7/8 seven eighths, medium 3 and 5, 3.5. Row length becomes a double.
- *Your song*: the song's bar, one hit per pulse with the metronome's levels; at 3/4 120 BPM: 3 hits, 1.5 s.

### "Your song" rules
1. **Clip doesn't fill whole bars.** A MIDI or audio clip longer than one bar whose length in beats isn't a whole number of bars (tolerance 0.01 beats). Text for the first one, plus "and {n} more" when several: "'{clip}' is {beats} beats long: {bars, one decimal} bars of {sig}. A part that doesn't fill whole bars drifts against the bar lines when it repeats; try {floor × L} or {ceil × L} beats ({floor} or {ceil} bars)." Quiet when every clip fills whole bars. Example: 16 beats in 3/4 → "5.3 bars", "15 or 18 beats (5 or 6 bars)".
2. **Shuffle in 4/4.** Signature 4/4; of the note starts in the song's MIDI clips, those not within 0.03 beats of a whole beat are off-beat; fires when there are at least 16 off-beat starts, at least 60 % of them within 0.03 of 1/3 or 2/3 of a beat, and at most 20 % within 0.03 of 1/2. Text: "Many of your notes fall on triplets ({t} of the {o} notes between the beats): a shuffle or swing feel. Sheet music often writes this feel in 12/8, where every beat splits into three eighths. To try it, start a song in 12/8 at {bpm × 1.5} BPM, so its dotted-quarter beat stays at {bpm}." Quiet otherwise.
3. **Odd grouping.** In an odd /8 or /16 metre always: "DawIt groups {sig} as {groups}; music in {sig} may also use another grouping."

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Bar length 4/4, 3/4, 6/8, 7/8, 9/8, 12/8, 5/4, 2/2, 7/16 | 4, 3, 3, 3.5, 4.5, 6, 5, 4, 1.75 | 1e-9 |
| Group starts 6/8; 12/8; 7/8; 5/8; 2/2; 6/4 | {0, 1.5}; {0, 1.5, 3, 4.5}; {0, 1, 2}; {0, 1}; {0, 2}; {0, 3} | 1e-9 |
| Pulse x/2, x/4, x/8, x/16 | 2, 1, 0.5, 0.25 | 1e-9 |
| Tidy 0/4, 16/4, 4/3 | 1/4, 15/4, 4/4 | exact |
| Position 4/4 18.25; 3/4 18.25; 6/8 4.75 | "5.3.2"; "7.1.2"; "2.4.2" | exact |
| Felt BPM 6/8 at 120; 12/8 at 90 | 80; 60 | 1e-9 |
| Bar seconds 3/4, 6/8, 7/8, 4/4 at 120 | 1.5, 1.5, 1.75, 2.0 | 1e-9 |
| Keep bars 4/4→3/4: beat 16; 18 | 12; 13.5 | 1e-9 |
| Keep bars 6/8→4/4: beat 4.5 | 6.0 | 1e-9 |
| 4/4→3/4→4/4 round trip of 20 positions | unchanged | 1e-9 |
| MIDI clip start 8 length 8, 4/4→3/4 keep bars | start 6, length 6, notes unchanged | 1e-9 |
| Song end: clip ending at beat 10 in 3/4 | bar 4 | exact |
| Metronome 3/4, beats 0–9 | 3 loud (0, 3, 6), 6 soft | exact |
| Metronome 6/8, beats 0–6 | 12 clicks: loud 0, 3; medium 1.5, 4.5; 8 soft | exact |
| Count-in 3/4 | 3 clicks, the first loud, over 3 beats | 1 sample |
| Plugin playhead 3/4 at beat 7.5 | {3, 4}, last bar start 6 | 1e-9 |
| MIDI export FF 58 data 4/4; 3/4; 6/8; 7/8; 2/2 | 04 02 18 08; 03 02 18 08; 06 03 24 08; 07 03 0C 08; 02 01 30 08 (hex) | exact |
| Score 6/8 bar of six eighths | two beam groups of three | exact |
| Score 3/4 bar of six eighths | three beam groups of two | exact |
| Score 6/8 eighths on 1 2 3, beat 2 empty | a dotted-quarter rest at unit 18 | exact |
| Rule 1: 16-beat clip in 3/4 | fires, "5.3 bars", "15 or 18" | text |
| Rule 2: 32 notes at 0 and 2/3 of each beat in 4/4 | fires, "16 of the 16" | text |
| Rule 2: same at 0 and 1/2 | quiet | text |
| Every rhythm topic has text; *Your song* text names the signature | non-empty | — |
| Theory Guides search "waltz", "compound" | finds the rhythm guide | — |

## Steps

Group A1, engine core (`./build.sh --test` at step 3):
- [x] 1. `TimeSignature` and `timesig::` helpers in `src/engine/TimeSignature.{h,cpp}` (both CMake targets): tidy, bar length, pulse, groups (F8), kind, bar ↔ beat, position text, `describe`, felt BPM, clocks per click, the *keep bars* mapping. Tests: the table's first nine rows and the keep-bars rows. Check: tests pass.
- [x] 2. Engine getter and setter (recipe *Song data that's saved*), the change function with both modes as one undo step, undo snapshot and describe, `resetEdit`, `ProjectFile` key and version 4. Tests: save and load, old project, version, undo round trip, a keep-bars change moving a clip, lyrics, tempo change and loop.
- [x] 3. Engine users of the bar: song end bar, bounce range, count-in, metronome levels, recorded clip lengths, held-note shift, the one-bar MIDI clip, chord at a beat, plugin playhead, MIDI export FF 58. Tests: song end, metronome, count-in, playhead, export bytes. Done when `./build.sh --test` passes.

Group A2, generate and analysis (`./build.sh --test` at step 7):
- [x] 4. `gen::rhythm`: `formatPosition` and `countAt` take the signature; `explain` takes it; the three new topics and changed texts from *Theory*; the *Your song* analysis (rules 1–3) as a pure function. Tests: positions, topic texts, rules 1–3.
- [x] 5. `gen::notation`: bar units, groups for R9–R11, compound grid, signature in `Score`, tempo-mark bars. Tests: the three score rows and an existing 4/4 example unchanged.
- [x] 6. Lyrics, PlayAlong, SingPairing, Takes, NoteRealizer, ChordStyles (cut at the bar), SongTemplates (sets 4/4), PromptBuilder (states the signature and bar length), ChatMusician (4/4 only, with its reason); ExampleSong stays 4/4 explicitly. Tests: in 3/4, lyrics spread over a section and a chord style's spans per bar; a prompt names 6/8.
- [x] 7. Analysis takes the bar length: ChordFinder, StructureFinder, MidiHarmony per bar; TempoFinder's downbeats every bar when it is a whole number of quarters, otherwise tempo only with its note; reading guide texts from *Theory*. Tests: chords per bar in 3/4; MidiHarmony bars in 6/8; reading texts name the signature. Done when `./build.sh --test` passes.

Group B1, UI: the control and the timeline (`./build.sh --test` at step 10):
- [x] 8. Transport-bar signature button, menu, *Other...* dialog and change dialog; ruler mark, click and submenu; tooltips from *Theory*; a `"timeSignature": "3/4"` shot key in `ScreenshotRunner` (keep bars). Check: the menu changes the ruler.
- [x] 9. Ruler, lanes, song rows, TrackLanes, piano roll grid and Snap, wave editor, position display and timeline tooltips, ScoreView and ScorePainter (two-digit signatures, header width, hover), Generate panel and chord-style notes, sing-along and play-along dots, metronome tooltip, Detect tempo note.
- [x] 10. Leftover sweep: grep `src/` for bar lengths of 4 (`4.0`, `* 4`, `/ 4` with bar or beat nearby); song code uses the signature, guide examples use the named 4/4 constant. Done when `./build.sh --test` passes without warnings.

Group B2, UI: the guide (`./build.sh --test` at step 11):
- [x] 11. RhythmExplainer rows for the new topics (double bar lengths, three velocities), the *Your song* picture and Play, `showTopic` for the new topics, the ruler and button guide items opening *Time signatures*; Theory Guides entry text and words; ReadingExplainer header with the song's signature. Done when `./build.sh --test` and `--export-guides` pass.

Group C, check and docs:
- [x] 12. Real-app check, one `--screenshots` run: (a) `"timeSignature": "3/4"`, arrangement with chords and lyrics open: bars of three in the ruler, lanes and chord row, "3/4" on the ruler and button; (b) same, piano roll of Bass zoomed in: bar and beat lines; (c) `"timeSignature": "6/8"`, Lead in score mode: 6/8 at the start, eighths in threes; (d) guide rhythm, topic *Compound and odd metres*; (e) guide rhythm, *Your song's time signature* in 6/8 (felt 80 at 120 BPM if the song is at 120); (f) the change dialog if reachable (else add a scene). Look for cut-off text and wrong bar numbers.
- [x] 13. Docs: README *Features* bullet (transport signature, what follows it) and remove "The song timeline currently uses 4/4"; README ChatMusician lines say "4/4 songs only"; docs/tests.md (time signature area); docs/code-layout.md (`TimeSignature`); docs/architecture.md (*The edit*: positions in quarter-note beats, bars from the song's signature; *Saving*: version 4); recipes.md *Song data*: bar math goes through the signature; issue #137 progress, and notes on #102 (chords split at beats use the bar's beats) and #49 (import reads FF 58).

## Tests
In `tests/EngineTests.cpp`, a `runTimeSignatureTests()` plus additions to the rhythm, notation, project, undo, export, lyrics, generation and analysis tests: every row of *Test numbers*, and per area a 3/4 and a 6/8 case: section-spread lyrics land in bars of 3 beats; a chord style plays one span of 3 beats per bar in 3/4; ChordFinder returns one chord per 3-beat bar; the prompt text contains "6/8" and "3 beats"; New from Style leaves 4/4. `--export-guides` still passes.

## Website hand-off
- features.html, *Write & compose*: a short section on the time signature (button, ruler, metronome, score), near the rhythm item.
- learn.html: the rhythm guide card mentions time signatures; a screenshot of *Compound and odd metres* would show it.
- Screenshots: the arrangement in 3/4 with the chord row; the score in 6/8. No library, so no about.html change.

## Left for later
- Signature changes inside a song (a signature map like the tempo map, #37).
- Choosing the grouping of odd metres (3+2+2 instead of 2+2+3).
- Common-time and cut-time symbols in the score; duplets in compound metre.
- ChatMusician and the built-in chord styles and drum templates in other metres (they are 4/4 patterns).
- *Detect tempo* finding bars in 5/8, 7/8 and other bars that aren't whole quarters, and offering ×1.5 in compound songs.
- MIDI import reading the signature (#49); several chords in a bar (#102).
- A rule on very slow felt beats in compound songs: left out because the lower limit of a felt beat wasn't checked.

## Changes while building

### Group A1 hand-off (engine core, built)
- `src/engine/TimeSignature.{h,cpp}`: `daw::TimeSignature { numerator, denominator }` and `daw::timesig::` (`getCommon`, `tidy`, `getBarLength`, `getPulseLength`, `getKind`, `getGroups`, `getGroupStarts`, `hasLongGroups`, `getClick`, `getBarAt`, `getBarStart`, `getBarStartAtOrBefore`, `formatPosition`, `toString`, `describe`, `getFeltBpm`, `getBarSeconds`, `getMidiClocksPerClick`, `mapBeatKeepingBars`, and `timesig::fourFour` for 4/4 examples). Both CMake targets.
- Inside `Engine` the bare name `TimeSignature` is `juce::AudioPlayHead::TimeSignature` (Engine inherits it): write `daw::TimeSignature` in Engine code.
- Engine: `getTimeSignature()`, `getBarLength()` (use this instead of 4), `setTimeSignature()` (only sets it), `changeTimeSignature (sig, keepBars)` (one undo step "Time signature 3/4", commits earlier edits first; keep bars maps clips, MIDI clip lengths, tempo changes, lyrics, loop and playhead; returns false if unchanged). `resetEdit` goes back to 4/4. Not blocked while recording: the UI should disable it then.
- Undo: `Snapshot::timeSignature` (compared) and the loop points (carried, not compared; `restore` puts the loop back only when crossing a signature change). `describe` names a signature change first.
- Saving: `timeSignature { numerator, denominator }` key, left out at 4/4; invalid or missing loads as 4/4; `formatVersion` is now 4 (newest read, written only when not 4/4), `formatVersionWithReturns` = 3 added.
- Audio thread: metronome clicks per pulse (bar 0.5 at 1760 Hz, group start 0.35, other pulses 0.2 where groups are longer than a pulse, else 0.3 as before) via `clickPattern` filled in `applyTimeSignature`; count-in is one bar; chord at a beat, plugin playhead (`getPosition`) and recording clip lengths use the signature. Song end bar and bounce song range use `getBarLength()`.
- MIDI export: `midiexport::SongInfo` has a third member `timeSignature`; `createTimeSignatureEvent` writes FF 58 with the plan's clocks. `MidiExportActions.cpp` passes the song's signature. `MidiGuide.cpp` (guide example) and two tests pass `timesig::fourFour`.
- Not done, left to later groups as planned: the remaining `4.0` bar sites in `Lyrics.cpp`, `ExampleSong.cpp`, `DrumTriggerGuide.cpp`, `MidiGuide.cpp` (formatPosition), analysis, notation and the UI.
- Test note: the existing "version 4 is refused as newer" test now uses version 5. `runTimeSignatureTests()` is called after `runMetronomeTests()`.

### Group A2 hand-off (generate and analysis, built)
- **Rhythm guide** (`gen::rhythm`): `formatPosition (sig, beats)`, `countAt (sig, beat)`, `describeClicks (sig)` ("loud on 1, medium on 4, soft on the others"), `explain (topic, bpm, sig, advice = {})`. `Topic` now: barsAndBeats, timeSignatures, compoundAndOdd, noteValues, rests, dottedAndTriplets, counting, tempo, readingDawIt, yourSong (the order of `allTopics`; topic names "Time signatures", "Compound and odd metres", "Your song's time signature"). *Your song*: `rhythm::SongRhythm { signature, bpm, clips (name, beats), noteStarts }` and `rhythm::getAdvice (song)` give the advice lines (rules 1 to 3); pass them as `explain (Topic::yourSong, bpm, sig, advice)`. B2 must build the `SongRhythm` from the engine (every MIDI and audio clip with its length in beats; note starts of MIDI clips in song beats). `RhythmExplainer::getRows` returns no rows yet for the three new topics (`break;`), and it still draws its pictures in 4/4 (`timesig::fourFour` in its `countAt` and `formatPosition` calls); `explain` call there already passes `engine.getTimeSignature()`.
- **Notation**: `Input::timeSignature`, `Score::timeSignature`, `Score::unitsPerBar` (replaces the removed constant `unitsPerBar`; `ScorePainter` uses `score.unitsPerBar` for the whole-bar rest), `notation::getUnitsPerBar (sig)`. Nothing in the UI sets `Input::timeSignature` yet (ScoreView, ReadingExplainer, SheetMusicExport, DevScreenshots build `Input` without it, so they still write 4/4): B1 step 9. An empty bar is a whole rest of `unitsPerBar` units in every signature. The painter's time-signature header still draws "4/4".
- **Generators**: `gen::Request::timeSignature` and `getBarLength()` (set in `GeneratePanel::makeRequest`); `buildSystemPrompt (sig)` (the panel passes the request's); user prompt names the signature and "one bar is N beats". `styles::play (..., barLength = 4.0)` and `spansFromChordTrack (..., barLength)` cut the 4/4 patterns at the bar (`ChordStyleActions.cpp` passes the engine's bar length). `lyrics::place` and `setText` take `barLength = 4.0` (UI callers `LyricsEditWindow.cpp:777` and `LyricsLane.cpp:723` and `LyricsActions` still pass nothing: B1). `Takes`, `NoteRealizer` use `r.getBarLength()`. `SongTemplates` builds with an explicit `setTimeSignature (fourFour)`; `ExampleSong` too. `chatmusician::isAvailable (sig)` and `getUnavailableReason()` ("ChatMusician writes bars of 4/4; switch the song to 4/4 to use it"): the panel (B1) must use them to turn its Generate button off.
- **Play-along and sing-along**: `playalong::findMoment (..., numComing, barLength = 4.0)`, `describeWait (beats, barLength = 4.0)`, `explainPlayingAlong (changes, bpm, barLength = 4.0)`; `singthis::Melody::timeSignature` (set by `buildMelody (..., sig)`, `SingWindow` passes the engine's), `getBar (sig, beat)`. UI callers of the play-along functions still use the default 4 (`ChordPlayAlong.cpp`, `LyricsSingAlong.cpp`), and so does the 4-dot beat row: B1 step 9. In 7/8 the play-along's beat walk is by whole quarter beats, so "beats to next chord" can be up to half a beat off there.
- **Analysis**: `chordfinder::analyse (..., barLength = 4.0)`, `structurefinder::describeBars` and `analyse` (same), `midiharmony::Input::timeSignature` and `makeInput (clip, sig)` (halves of a bar only where the signature has two or four groups and a bar of 3 beats or more: 4/4, 2/2, 6/8, 12/8), `lyricsfinder::toLyrics (..., barLength = 4.0)`, `tempofinder::followRecording (tempo, clipStart, barLength = 4.0)` (a bar per `barLength` detected beats when it is a whole number of quarters, else the one tempo). The UI actions that call them (`TempoActions`, the chord, structure and lyrics detect actions, `KeyFindingExplainer`) don't pass the bar length yet: B1 step 9/10.
- **Reading guide**: its texts (time signature and tempo, note values, ties) take the signature from `part.input.timeSignature`; its bar numbers use `timesig::getBarAt`.
- `timesig::getFeltBpm` now divides by the dotted beat (3 pulses): 6/8 and 12/8 as before, 6/4 BPM / 3, 6/16 BPM / 0.75 (A1 had BPM / 1.5 for every compound metre). The *Your song* text names the dotted note by denominator (dotted eighth, quarter or half).
- Left on 4: `SingWindow.cpp` grid and bar numbers (`st::getBar (timesig::fourFour, ...)`), and the `* 4.0` sites in `StylesGuide.cpp`, `StructureGuide.cpp` (bar seconds), `LyricsGuide.cpp:818`, `SingingGuide.cpp:353`, `TuneAnalysis.cpp:142`, `DrumTriggerExplainer`, `MidiExplainer` for the step 10 sweep.

### Changes while building (Group A2)
- Text of mine, not in *Theory*: the *Your song's time signature* heading (the plan's text started "Your song"; the topic name is used so every topic text starts with its name); the "And {n} more." ending of rule 1; "(quarter notes a minute)" and "one bar is N beats (quarter notes), counted as ..." in the prompts; the sentence "It writes bars of 4/4, so it is offered only in 4/4 songs." in the reading guide's generated-music topic. The guide texts use plain words without the plan's asterisks (Time signatures, Compound and odd metres).
- In 6/8 the "In 6/8 DawIt still writes ..." sentence of the reading guide also shows in 9/8 and 12/8 (any compound /8 metre), named by the song's signature.
- Notation splitting rule (R9) as built: in simple metres a piece starting off a group start is split at the next group start (2/2 splits a note from the second quarter at the half-note beat); the middle of a bar is kept only with four equal groups (4/4, 12/8, 8/8); where groups are longer than a bottom note (6/8, 7/8) a piece never crosses a group start except from a group start to another one.

### Group B1 hand-off (UI: the control and the timeline, built)
- **`src/ui/TimeSignatureUi.{h,cpp}`** (`daw::timesigui`, in the DawIt target): `showMenu (engine, target, onGuide, atMouse)`, `addItems (menu, engine, owner)` (the nine plus *Other...*, disabled while recording), `choose (engine, sig, owner)` (asks *Keep each part in its bar / Keep the timing / Cancel* only when `hasPartsToMove`), `showOtherDialog` (two combo boxes, 1 to 15 and 2/4/8/16), tooltips `getButtonTooltip`, `getMarkTooltip`, `getMetronomeSentence`, `formatBeats`, and the grid helpers `getLineKind (sig, beat)` (bar, beat = group start, pulse, other) and `getLineStep (sig)` that every grid now uses.
- **Transport bar**: `timeSignatureButton` right of BPM (disabled while recording, tooltip follows the tempo for the felt beat); the position display uses `timesig::formatPosition`; the metronome tooltip ends with the click sentence. `TransportBar::onShowTimeSignatureGuide` is wired in `MainComponent` to `showRhythmGuide()` (the whole guide). B2: make it, the ruler's menu item (`Ruler::onShowRhythmGuide`) and the *Time signatures guide...* item open the topic *Time signatures* (`RhythmExplainer::showTopic`), and rename the ruler's old item if wanted.
- **Ruler**: draws the signature in a box at the left edge (hides the first bar's number), click opens the menu, right-click has a *Time signature* submenu above *Note lengths, beats and bars...*; beat ticks are taller than pulse ticks; tooltip over the mark.
- **Grids and bar maths** now ask `engine.getBarLength()` or the signature: ArrangementView (ruler, folded rows), TrackLanes (grid, snap, tooltip), ChordLane, SectionLane, LyricsLane, TempoLane, PianoRoll (grid by kind, chord rows, "1 bar" snap follows the signature through `PianoRoll::updateSnap`), WaveEditor, HitsToMidiWindow, SingWindow, ScoreView (`Input::timeSignature` set; `getBarLength()`; "bar 3, beat 2.5" counts the bottom note; hover text), ReadingExplainer and SheetMusicExport (set `part.input.timeSignature`), ScorePainter (`drawNumber` draws two-digit numbers; header width via new `notation::getTimeSignatureWidth`: 3.0 for one digit, 4.7 for two).
- **Generators and analysis callers** now pass the bar length: GeneratePanel (`songBarOf (barLength, ...)`, a note line under the key/tempo: the "written in 4/4" sentence, or ChatMusician's reason, and its Generate button off in other signatures via `getInfoNote()`), LyricsEditWindow, LyricsLane, LyricsActions, ChordActions, StructureActions, TempoActions (`adviseLiningUp (..., barLength)`; *Detect tempo* compound-song note), ChordPlayAlong (one dot per beat of the bar), LyricsSingAlong (`singalong::getCountIn (lyrics, m, barLength)`: new third parameter, count-in of a bar's beats), ChordExplainer, KeyFindingExplainer (`makeInput (clip, sig)`), Tune and Harmoniser pictures (`tunetext::SongInfo::timeSignature`, `Trace::barLength`, `tunecheck::analyse (..., barLength)`), BounceDialog, DevScreenshots.
- **Shot key** `"timeSignature": "3/4"` in `ScreenshotRunner` (keep bars), applied before `select`, so it stays for the later shots of a list.
- **Left on 4 on purpose** (examples written in 4/4, not the song's bars): the `*Guide.cpp` and `*Explainer.cpp` picture code (Arpeggiator, Slicer, StepFilter, Delay, Reverb, Harmoniser, DrumTrigger, Midi, Lyrics, Singing, KeyFinding, Styles pictures), `MainComponent` demo song, `GeneratePanel` ChatMusician range check (4/4 only), `BuiltInArpeggiator` message. `RhythmExplainer` rows for the new topics and its 4/4 pictures are B2's.
- Also: removed the unused `latencyText` from `HarmoniserEditor.cpp` (a warning from the last commit).
- Check: one shot with `"timeSignature": "3/4"` shows bars of three in the ruler and lanes, "3/4" on the ruler and button, position 1.1.1. Two tests added (count-in in 3/4, header width for 12/8). Not tried by hand: the menu clicks and dialogs (Group C's step 12 (f)).

### Group B2 hand-off (UI: the guide, built)
- `RhythmExplainer` (`src/ui/RhythmExplainer.{h,cpp}`): `Row::beatsPerBar` is now a double (bar length in quarter notes); rows of the metre topics set `bySignature` and `signature` (grid lines strong at the signature's group starts, a bracket over each group, "Hear the beat" clicks at group starts via `Row::getBeatStarts()`). `getRows (topic, bpm, sig)` has rows for *Time signatures* (2/4, 3/4, 4/4 with a stronger 3, 2/2), *Compound and odd metres* (6/8, 9/8, 12/8, 5/4 as 3+2, 7/8), *Your song* (one hit per click at the metronome's levels, bar seconds in the detail) and *Tempo* (song's bar, seconds). Every row of a topic shares one width, set by the longest bar (at least 4 quarters). *Bars and beats* and *Reading DawIt* examples stay in 4/4 on purpose (the text says so).
- `RhythmExplainer::getSongRhythm (engine)` builds `rhythm::SongRhythm` (every clip's length in beats, MIDI note starts in song beats); `showRow` passes `getAdvice` for *Your song*. The guide refreshes on engine changes for *Tempo*, *Your song* and *Bars and beats*. The guide opens on *Note values* (found by topic, not index 1).
- `MainComponent::showTimeSignatureGuide()` opens the rhythm guide on *Time signatures*; used by the transport button's and the ruler mark's menu guide items (`TransportBar::onShowTimeSignatureGuide`, new `Ruler::` and `ArrangementView::onShowTimeSignatureGuide`). The ruler's right-click item "Note lengths, beats and bars..." still opens the whole guide.
- Theory Guides entry for the rhythm guide: summary, opens-from and search words from the plan. `ReadingExplainer` needed no change (B1 already sets `part.input.timeSignature`, so the score header and bars follow the song).
- Texts of mine, not in *Theory*: row names and details in the new pictures ("Two beats: marches and polkas", "Four beats, 3 a little stronger", "Five beats, here 3 + 2", "A bar lasts 1.88 s at 96 BPM" and so on).
- Checked: `./build.sh --test` passes with no warnings, `--export-guides` exits 0, and two shots (6/8 *Compound and odd metres* and *Your song*) look right; the *Your song* text and 6/8 felt 64 at 96 BPM match the rules. For Group C step 12 (d), (e): the shot key `"timeSignature": "6/8"` with `"guide": "rhythm"` and the topic name works.

### Group C hand-off (real-app check and docs, built)
- Shots (scratch folder, scale 1) in `/private/tmp/claude-501/-Users-geertvandamme-prj-dawit-public/a7ab4073-f889-480b-b2e2-7e5006cd91ce/scratchpad/shots/`: `a-arrangement-34.png` (3 beats per bar in the ruler, chord row, lyrics, sections; "3/4" on the ruler mark and button, position 1.1.1), `b-roll-34.png` (piano roll grid in 3/4), `c-score-68.png` (6/8 header, eighths beamed in threes, bars 5 to 10 numbered right), `d-guide-compound.png`, `e-guide-song.png` (6/8, 96 BPM: bar 1.88 s, felt 64), `change-dialog.png`, `other-dialog.png`. No cut-off text or wrong bar numbers; nothing needed fixing. The signature menu itself (a native pop-up) was not captured.
- Added to `ScreenshotRunner` for good: scenes `timeSignatureChange` (with `"signature"`, default 3/4) and `timeSignatureOther`, shot key `selectClip` (a track's first MIDI clip), and alert windows opened by a shot are cancelled when the next shot starts.
- Docs done: README (*Write in any metre* bullet, ChatMusician 4/4 only, limitations), docs/tests.md, code-layout.md, architecture.md (positions, saving version 4), recipes.md (bar maths in *Song data*).
- Website hand-off: features.html *Write & compose*: a section on the time signature button, ruler, metronome and score, near the rhythm item; learn.html: the rhythm guide card mentions time signatures and metres (screenshot `d-guide-compound.png`); screenshots worth taking: the arrangement in 3/4 with chords open (shot `a`), the score in 6/8 with Lead vocal (shot `c`), with `"timeSignature"` and `"selectClip"` keys. No library added.
- Test pass of the piano-roll shot: the Bass pattern is a 4-beat pattern kept as is in 3/4 (by design: notes keep their rhythm).

### Coordinator fix after Group C
- `ScorePainter.cpp` drew dotted rests without their dot, so 6/8's dotted-quarter rest for an empty big beat looked like a quarter rest. It now draws the augmentation dot in the space above the middle line. Checked in a re-shot of the 6/8 score (bar 7); `./build.sh --test` passes without warnings.

### Theory check
- `RhythmGuide.cpp` *Bars and beats*: "the top number is how many beats a bar has" became "how many notes fill a bar" (6/8 has two beats, not six); "marches and polka" became "polkas".
- `TimeSignatureUi.cpp` menu hint for 5/4: "five beats: uneven bars" became "five beats: 3 + 2 or 2 + 3" (the bar isn't uneven; its beats group unevenly, as the guide says).
- `GeneratePanel.cpp` chord-style note: "each bar plays its first N beats" was wrong for bars longer than 4 (5/4, 9/8, 12/8 repeat the pattern's start); now "each bar of {sig} (N beats) plays one bar of it, cut or repeated to fit."
- `PromptBuilder.cpp`: ", counted as {describe}" became ", felt as {describe}", so "one bar is 3 beats (quarter notes)" and "two beats in each bar" don't read as a contradiction.
- README *Write in any metre*: chord styles are offered outside 4/4 (cut or repeated per bar), New from Style starts a 4/4 song, ChatMusician 4/4 only; the metronome's medium click is only in metres with groups (6/8, 7/8).
- Checked and kept: the *Your song's time signature* heading, "(quarter notes a minute)", `getFeltBpm` (6/4 ÷ 3, 6/16 ÷ 0.75), the 2/2 split rule, the other menu hints, the picture rows, the Detect tempo note. SOURCES.md: one search on 2/2 notation added under *Rhythm*.
- Open: the comment above `GeneratePanel::getInfoNote` still says "plays the first beats of each bar"; the *Your song* text prints "÷ 3.0" for 6/4 and "2.0 s" for 4/4 (number formatting in code). The longer Generate note was not seen in the real app.
- The Generate panel's info label had a fixed height (48 px, +34 with the note), so a long chord list plus the 4/4-pattern note cut off its last line. It is now as tall as its wrapped text (`getInfoHeight`). Checked in 3/4 and 5/4 with the new shot keys `"generatePanel": true` and `"generatePlaying": 2` (ScreenshotRunner, documented in recipes.md). The stale comment above `getInfoNote` now matches dawTheory's corrected note.
- Kept as is: "2.0 s" and "÷ 3.0" in *Your song*, since other guides write seconds the same way ("2.0 seconds", "2.00 s").
