# Transform MIDI notes: reverse in time, reverse in pitch, randomise pitch in the song key

Status: Released; Theory checked
Idea: [#141 — Transform MIDI notes: reverse in time, reverse in pitch, randomise pitch in the song key](https://github.com/geertvandamme/dawit/issues/141)

## Goal
One menu turns a MIDI clip, or the selected notes, into a variation: backwards (retrograde), upside down (inversion, exact or in the key) or with new random pitches from the song's key over the same rhythm. A beginner gets quick ideas and hears that a melody is two separate things, rhythm and pitch, with three new topics in *What is MIDI?* that explain the old composing tricks behind them.

## Decisions for you
1. **Where the topics go.** Options: three topics in *What is MIDI?* (as the issue says); a new guide "Turning a melody around" with its own window. **Recommended:** the MIDI guide: its pictures already use the clip in the piano roll, and the piano roll opens it. Its summary and search words grow to cover them.
   Answer: the MIDI guide, as recommended.
2. **How *Reverse in time* places notes.** Options: (a) mirror in time, the issue's rule (`start' = S + E - end`): no new overlaps, exact for legato lines, but a short note with a rest after it lands off the beat by that rest; (b) mirror the starts, as Ableton Live's *Reverse*: attacks stay on the grid, but a long note can overlap the next (C D E as 1, 1, 2 beats gives a 2-beat E at 0 over D at 1). Both undo themselves when done twice. **Recommended:** (a), with the caveat in the topic text.
   Answer: (a) mirror in time, as recommended.
3. **Preview and undo.** DawIt has no undo yet (#92). Options: (a) apply at once, with *Randomise again* (a new roll from the original notes) and a one-step *Undo <transform>* in the menu; (b) a *Randomise* window with live preview, range boxes, *Try again*, *Keep* and *Cancel*. **Recommended:** (a): hearing the clip is the preview, and #42 or #92 can add a window or full undo later.
   Answer: (a) apply at once, with *Randomise again* and a one-step undo, as recommended.
4. **Inversion as two items** (*in the key*, *exactly*) or one item with a ticked *Keep in the key* option. **Recommended:** two items: both are one click, and the difference is the lesson.
   Answer: two items, as recommended.
5. **Randomise options and defaults:** *Stay near the original* (Any, 1, 2 or 3 scale steps; default Any), *Avoid repeating the same pitch* (default off), *Chords move together* (default on), as ticked items in an *Options* submenu, remembered in the app settings. The range is the notes' own, widened to an octave when smaller; a range the user types is left for later. **Recommended:** as written.
   Answer: as written, as recommended.
6. **Drum sounds.** The pitch items are disabled on a track playing a percussive built-in sound (its notes pick drums), as *Detect key and chords...* is; *Reverse in time* stays. Plugin instruments can't be told apart and are allowed. **Recommended:** as written.
   Answer: as written, as recommended.

## What the user does and sees
- **Piano roll toolbar:** a *Transform* button between the octave buttons and *Select* (about 84 px). Its tooltip: "Transform the selected notes, or the whole clip when none are selected: reverse in time (the last note plays first), reverse in pitch (upside down), or new random pitches from the song's key over the same rhythm. Reverse twice to get back." Disabled without a clip.
- **MIDI clip right-click menu (`TrackLanes`):** a *Transform* submenu after the transpose items, acting on the whole clip.
- **The menu** (one shared builder for both places; nothing opens a window, so no "..."):
  ```
  Reverse in time
  Reverse in pitch, in the key
  Reverse in pitch, exactly
  ─────
  Randomise pitch in A minor          (the song's key by name)
  Randomise again                     (only right after a randomise of this clip, unchanged since)
  Randomise options ▸  Stay near: Any / 1 / 2 / 3 scale steps (radio) │ Avoid repeating the same pitch ✓ │ Chords move together ✓
  ─────
  Undo Reverse in time                (names the last transform; disabled when there's none to undo)
  ─────
  [guide icon] About reversing and randomising notes...   (MIDI guide at the topic of the last transform, else *Backwards: retrograde*)
  ```
- **Disabled when nothing would change:** no notes; *Reverse in time* when the result equals the notes; the reverse in pitch items with fewer than two distinct pitches; the pitch items on a drum sound (Decision 6). JUCE menu items have no tooltips, so the button's tooltip and the guide carry the explanation.
- **Scope:** with notes selected in the piano roll, only those change and stay selected (transforms change notes in place, never reorder or add them); otherwise the whole clip. Only notes starting before the clip's end count; a note ringing past the end is first shortened to end at it.
- **After a transform** the clip plays the new notes at once (`notesChanged()`); the track's instrument and sound are untouched.
- **Settings:** `transform.stayNear` (0 to 3, 0 = any), `transform.avoidRepeats`, `transform.chordsTogether`. Nothing else is remembered.

## Data and saving
No new project data and no format change: the transform edits `MidiClip::notes`, which are saved already. The undo snapshot and the seed live only in memory and are dropped by `resetEdit()`. One save-and-load test of a transformed clip.

## The guide
Three topics in *What is MIDI?* (`midiguide`), before *Your clip*:

| Topic | Summary | Picture |
|---|---|---|
| Backwards: retrograde | The last note plays first | Two small piano rolls, *Before* and *After*, of the clip in the piano roll (first 8 bars), else the example; click one to hear it |
| Upside down: inversion | Up a step becomes down a step | *Before*, *In the key* and *Exactly*, with the mirror line dashed across; notes outside the key outlined |
| New notes, same rhythm | Random pitches from the key | *Before* and one roll at seed 1 with the default options; the key's notes in range as lit rows |

- **Your clip** gains a paragraph, *Turning it around* (rules in *Theory*); `explain` learns whether the clip plays a drum sound.
- **Theory Guides entry:** summary becomes "What MIDI is and isn't, its history, the messages, note numbers, velocity, controllers, files, and turning a melody backwards, upside down or into new notes."; `opensFrom` adds "the Transform menu"; keywords add "retrograde inversion invert reverse backwards upside down mirror random randomise transform variation". Not on *Start here* (unchanged). Topics follow from `allTopics`.
- Opens at a topic through `MainComponent::openGuide (Guide::midi, topic)`; the PDF follows.

## Code
- Follows recipes.md *Topics in an existing guide* and *A command or menu item* (the menu lives in views, not `CommandIDs`: it acts on a clip).
- **`miditransform`** (`src/engine/MidiTransforms.{h,cpp}`, both CMake targets): pure functions on a note vector, the indices to change (empty = all) and the clip length, returning new notes in the same order: reverse in time; invert exactly or in a key; randomise in a key with options and a seed; and the key helpers they share (snap a pitch to the key, ties to the lower note as Ableton's *Fit to Scale*, never leaving 0 to 127; a pitch's scale-step number; the key's pitches in a range). A chord is notes whose starts are under 0.02 beats apart.
- **Rules:** reverse in time mirrors in `[S, E]`: whole clip `[0, length]`, a selection `[first start, last end]`. Exact inversion `p' = lo + hi - p`; in-key inversion mirrors scale-step numbers between those of the snapped lowest and highest notes. Randomise keeps start, length and velocity; range `[lo, hi]`, widened to `lo + 12` (at most 127) when `hi - lo < 12`; uniform choice among allowed pitches; *stay near n* allows at most n scale steps from the snapped old pitch; *avoid repeats* excludes the previous onset group's new lowest pitch; when nothing is allowed, drop *avoid repeats*, then take the in-key pitch in range nearest the old one. *Chords together* moves a group by one random number of scale steps that keeps all of it in range.
- **Random numbers:** `std::mt19937` with its raw output mapped by our own code (as `SongTemplates`), not `std::uniform_int_distribution`, which differs between standard libraries.
- **Engine:** one function applies a transform result to a clip under `getLock()`, calls `notesChanged()` only when something changed, and keeps a one-step snapshot (clip, edit generation, indices, notes before and after, which transform, seed). Undo and *Randomise again* work only while `findTrackFor (clip)` finds it, the generation matches and the notes equal the snapshot's result; *again* re-randomises the snapshot's original notes with seed + 1. The first seed comes from the time. Model on `setNoteVelocities`.
- **Threading and risk:** message thread only, under the lock like any note edit; no audio-thread change. Selected indices stay valid because note order and count never change.

## Libraries and models
None.

## Theory

**Facts**

| # | Fact | Kind | Source |
|---|---|---|---|
| 1 | A retrograde is a melody played backwards; an exact retrograde reverses pitches and rhythm. | exact | Wikipedia, *Retrograde (music)* |
| 2 | Machaut's *Ma fin est mon commencement* has a retrograde canon; in Bach's *Musical Offering* one canon voice is the other backwards; the minuet of Haydn's Symphony No. 47 is a palindrome. | exact | Wikipedia, *Retrograde (music)* |
| 3 | Machaut (14th century, died 1377) is over 600 years ago. | exact | derived: 2026 - 1377 = 649 |
| 4 | Listeners recognise an inverted or backwards short melody better than chance, but not reliably. | typical | Dowling (1972), *Perception & Psychophysics* 12(5), 417-421 (first page); Mongoven and Carbon's summary |
| 5 | Melodic inversion turns each interval upside down; intervals may stay exact or, more often in tonal music, become their scale equivalents (C D E to C B A rather than C B♭ A♭). | exact / typical | Harvard Dictionary of Music, quoted in Wikipedia, *Inversion (music)* |
| 6 | The D minor fugue of *The Well-Tempered Clavier*, Book 1, inverts its theme by scale steps. | exact | Wikipedia, *Inversion (music)* |
| 7 | A twelve-tone row has four standard forms: prime, retrograde, inversion, retrograde inversion. | exact | Wikipedia, *Inversion (music)* and *Retrograde (music)* |
| 8 | Exact inversion of C E G around its middle gives G E♭ C (a C minor chord): 4 + 3 semitones become 3 + 4. | exact | derived: 127 - 60 = 67, 127 - 64 = 63, 127 - 67 = 60 |
| 9 | Mirroring by `lo + hi - p` always lands on a whole note and swaps the lowest and highest. | exact | derived |
| 10 | C4 to C5 holds 13 notes, 8 of them in C major. | exact | derived |
| 11 | 8 choices for each of 7 notes: 8^7 = 2,097,152 melodies. | exact | derived |
| 12 | With 8 equally likely choices, a note repeats the one before 1 time in 8. | exact | derived |
| 13 | Musical dice games (Kirnberger, 1757) chose each bar by dice from bars that fit; the 1792 Simrock print credited to Mozart is unauthenticated. | exact | Wikipedia, *Musikalisches Würfelspiel* |
| 14 | An F held over a C chord often sounds unsettled: F is a semitone above the chord's E. | opinion (interval derived) | derived; common arranging practice |
| 15 | Ableton Live's Reverse swaps the first and last notes' positions; its Invert is scale-aware with a scale; Fit to Scale goes to the lower degree on a tie. | exact | Ableton Live manual, *Editing MIDI* (design reference only) |
| 16 | Reversing MIDI notes keeps each note's attack; reversing audio makes notes swell and stop. | exact | derived from MIDI being instructions (existing *What MIDI is* topic) |

No contradiction with an existing guide. "Inversion" already means a chord inversion in the chord and key-finding guides; the inversion topic says they differ.

**Topic texts**

*Backwards: retrograde* — "The last note plays first"

Play a melody from its last note to its first and you get its retrograde (Latin for "going backwards"). It's one of the oldest ways to make a new line out of one you already have.

Take the start of Frère Jacques: C D E C, E F G, where the G lasts twice as long. Backwards it's G (long) F E C E D C. The same notes with the same lengths, in the opposite order, so the rhythm runs backwards too: the long note now comes first.

Composers have done this for over 600 years. In Guillaume de Machaut's song *Ma fin est mon commencement* ("my end is my beginning") one part sings another part backwards. In Bach's *Musical Offering* one voice of a canon plays the other backwards, and the minuet of Haydn's Symphony No. 47 sounds the same played from either end. In twelve-tone music, the retrograde is one of a row's four standard forms.

Don't expect to recognise it. Music runs forwards, and a melody backwards rarely sounds like itself: in listening tests (Dowling, 1972), people spotted a backwards melody more often than by guessing, but far from always. That's what makes it useful. It sounds new, yet it has the same notes, the same length and the same feel as the original, so it still fits the song.

In DawIt: *Transform > Reverse in time*, on the piano roll's toolbar or a MIDI clip's right-click menu. With no notes selected, the whole clip turns around, rests included: a note 1 beat long at the start of a 4-beat clip moves to beat 4. With notes selected, only they turn around, between the first one's start and the last one's end. Each note keeps its length and velocity; its end becomes its start. Do it again and you're back where you started.

One thing to watch: the silence after a note now comes before it. A short note on the beat with a rest after it lands after the beat. Notes that each last until the next one starts keep their rhythm exactly. Move short ones back onto the beat if you want them there.

This reverses the notes, not the sound: every note still starts with its attack. Playing a recording backwards (an audio clip's *Reverse*) is a different effect, where every note swells up and stops dead.

*Upside down: inversion* — "Up a step becomes down a step"

Turn a melody upside down and every step up becomes a step down, and every step down a step up. That's its inversion. The rhythm stays exactly as it was; only the pitches change.

Frère Jacques again: C D E C, E F G goes up, up, down, up, up, up. Mirrored around the middle of its range, it becomes G F E G, E D C: down, down, up, down, down, down. The highest and lowest notes swap places: the low C becomes a G and the high G a C.

There are two ways to mirror. **Exactly** keeps every step's size in semitones: up 4 semitones (C to E) becomes down 4 semitones. Mirrored exactly, C D E C, E F G becomes G F E♭ G, E♭ D C. E♭ isn't in C major, so the result sounds darker, partly in another key. **In the key** counts notes of the scale instead: up two scale notes becomes down two scale notes. Every note stays in the key, though some steps change size: C up to E is 4 semitones, G down to E is 3. Tonal music usually mirrors in the key: in the D minor fugue of Bach's *Well-Tempered Clavier*, Book 1, the theme comes back upside down that way. Twelve-tone music mirrors exactly, and the inversion is one of a row's four standard forms, with the retrograde and the retrograde inversion (backwards and upside down).

Mirroring exactly turns a major chord into a minor one. C E G is 4 semitones and then 3 up from C. Mirrored around its middle it becomes G E♭ C, which from the bottom is C E♭ G: 3 and then 4, C minor. Mirrored in the key, C E G becomes G E C, still C major.

This isn't a chord inversion. A chord inversion puts another of a chord's notes at the bottom (C/E is C major with E lowest). Mirroring a melody is a different thing with the same name.

In DawIt: *Transform > Reverse in pitch*, in the key or exactly. The mirror is the middle of the notes' range, so the line stays where it is and its lowest and highest notes swap. It needs at least two different pitches. Do the same one again and you're back where you started. In the key, a note outside the key first moves to the nearest note of the key (the lower one when it's halfway), so that note doesn't come back exactly.

*New notes, same rhythm* — "Random pitches from the key"

A melody is two things at once: when the notes play (the rhythm) and which notes they are (the pitches). Keep the rhythm, change the pitches, and you have a new melody that still moves like the old one. *Randomise pitch* does exactly that: every note keeps its start, length and velocity, and gets a new pitch picked at random from the song's key.

Why the key? It limits the choice to notes that belong. In C major from C4 to C5 that's 8 notes (C D E F G A B C) instead of 13. With 8 choices, a 7-note phrase can come out 8 × 8 × 8 × 8 × 8 × 8 × 8 = 2,097,152 ways, and none has a note outside the key. Most won't be good; some will give you an idea you wouldn't have had. Roll again until one does, keep the parts you like and fix the rest by hand.

Chance in music isn't new. In the 18th century, musical dice games let anyone put together a minuet or a waltz by rolling dice to pick each bar from a table of bars that all fitted. The best-known one was printed in 1792 under Mozart's name, though that has never been confirmed. The trick is the same as here: the dice choose, but only among choices that already fit.

What makes random notes sound more like a tune:
- **Stay near the original:** each note moves at most 1, 2 or 3 notes of the scale from its old pitch, so the shape of the line survives.
- **Avoid repeating the same pitch:** with 8 notes to choose from, about 1 note in 8 would otherwise repeat the one before.
- **A narrow range:** the notes' own range, widened to an octave when it's smaller.
- **Chords move together:** notes that start together move by the same number of scale notes, so a three-note chord stays a chord of the key. C E G moved up one scale note becomes D F A: it can turn from major to minor.

What it doesn't do: it doesn't know your chords. A note can be in the key and still rub against the chord under it: an F held over a C chord often sounds unsettled, because F is a semitone above the chord's E. It doesn't shape phrases either, so it won't end a line on the home note. Those choices are still yours; *Chord progressions* and *Finding the key and chords* help with them.

In DawIt: *Transform > Randomise pitch in* (the song's key), then *Randomise again* to roll another from the same original notes until you like one. *Undo* puts the notes back. Drum sounds are left out: their notes choose a drum, not a pitch.

**Menu and tooltip texts:** the Transform button's tooltip and the menu items as in *What the user does and sees*.

**Example data** (Frère Jacques, bars 1 and 3, public domain; `{pitch, start, length}`, 8-beat clip, C major):
- Before: {60,0,1} {62,1,1} {64,2,1} {60,3,1} {64,4,1} {65,5,1} {67,6,2}.
- Reverse in time (`8 - end`): G {67,0,2}, F {65,2,1}, E {64,3,1}, C {60,4,1}, E {64,5,1}, D {62,6,1}, C {60,7,1}.
- Exactly (`127 - p`): 67 65 63 67 63 62 60. In the key (steps C0 D1 E2 F3 G4, `4 - step`): 67 65 64 67 64 62 60.
- Randomise: range 60 to 67 is under 12, so 60 to 72: candidates 60 62 64 65 67 69 71 72 (8). The picture draws seed 1.

**"Your song" rules** (*Turning it around* paragraph in *Your clip*; quiet when the clip has no notes):
- Drum sound: "It plays a drum sound, so only Reverse in time fits: its notes pick drums, not pitches." Nothing else in the paragraph.
- One distinct pitch: "All its notes are {C4}, so there's nothing to mirror. Randomise pitch would give it a tune from the {8} notes of {C major} between {C4} and {C5}."
- Otherwise: "Reversed in time, its last note, {G4}, would play first. Reversed in pitch, {C4} and {G4} swap places. Randomise pitch would pick from the {8} notes of {C major} between {C4} and {C5}." The last two notes come from the range rule.
- Notes outside the key (count ≥ 1): "{2} of its notes are outside {C major}: reversing in the key or randomising moves them into it."
- At least 8 notes and 3 distinct pitches: "Try it on a copy: duplicate the clip and transform the copy, then play them one after the other. A phrase answered by its own inversion is an old trick."

**Test numbers**

| Check | Expected | Tolerance |
|---|---|---|
| Note {start 0, length 1} in a 4-beat clip, reversed | start 3, length 1, velocity unchanged | 1e-9 |
| Frère Jacques reversed: starts by original order | 7, 6, 5, 4, 3, 2, 0 | 1e-9 |
| Frère Jacques inverted exactly | 67 65 63 67 63 62 60 | exact |
| Frère Jacques inverted in C major | 67 65 64 67 64 62 60 | exact |
| C E G (60 64 67) exactly / in the key | 67 63 60 / 67 64 60 | exact |
| Key pitches in 60..72: C major, A minor | 8 each | exact |
| E major (sharps) in 64..76 | 64 66 68 69 71 73 75 76 | exact |
| Range of 60..67 widened | 60..72 | exact |
| Snap 61 into C major (tie) | 60 | exact |
| Reverse twice, invert twice (each way, in-key notes) | original notes | 1e-9 |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `MidiTransforms.{h,cpp}` with the three transforms and key helpers (*Code*), in both CMake targets. Tests: `runMidiTransformTests` with the table's numbers and the checks under *Tests*. Done when they pass.
- [x] 2. The engine function that applies a result, with the one-step snapshot, undo and *Randomise again*; cleared in `resetEdit()`. Tests: undo, again, unchanged-only, selection-only, save and load. Check: a transform then undo gives the notes back.
- [x] 3. The MIDI guide: three topics with texts, example data, *Turning it around* in *Your clip* (with the drum flag), the Theory Guides entry's summary, `opensFrom` and keywords. Tests: topic count 14, texts and examples, the clip rules, search. Done when `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 6):
- [x] 4. A shared *Transform* menu builder in `src/ui` (items, enable rules, the options submenu on the settings keys, the guide item) and the piano roll's *Transform* button with its tooltip, acting on `getSelectedNotes()` or the clip. Check: reversing a clip in the app moves its notes and keeps them selected.
- [x] 5. The *Transform* submenu in `TrackLanes`' MIDI clip menu (whole clip, drum rule as *Detect key and chords...*), with the guide item opening `openGuide (Guide::midi, topic)` through a callback.
- [x] 6. `MidiExplainer` pictures for the three topics (before/after rolls, the mirror line, lit key rows, a click plays a roll at the song's tempo, copying `KeyFindingExplainer`'s `onPlayNote` with `callAfterDelay`). Done when `./build.sh --test` passes and `--export-guides` still succeeds.

Group C, check and docs:
- [x] 7. Real-app check: add a `"transform"` shot setting to `ScreenshotRunner` (`"reverse"`, `"invertInKey"`, `"invertExact"`, `"randomise"` with seed 1) applied to the selected track's first MIDI clip. Shots on the pop song: piano roll of *Lead vocal* (Transform button between the octave buttons and Select, nothing cut off); the same after `"transform": "reverse"` and after `"randomise"` (notes moved, rhythm kept); `guide: midi` at each new topic and *Your clip* with *Lead vocal* selected (pictures drawn from the clip, mirror line, readable text).
- [x] 8. Docs: README bullet under *Features*; `docs/tests.md` (transform and guide checks); `docs/code-layout.md` (`MidiTransforms`, the menu builder); `ScreenshotRunner`'s `transform` in recipes.md *Checking in the real app*; issue #141 progress (and a note on #42 and #92 about sharing the snapshot); SOURCES.md is done by the plan.

## Tests
- Reverse in time: the table's checks; selection only mirrors inside its span and leaves others untouched; a note past the clip end is shortened first; notes never start before 0 or at or after the clip's end.
- Invert: the table's checks; lowest and highest swap; results stay in 0..127; fewer than two distinct pitches changes nothing.
- Randomise: every start, length and velocity unchanged; every pitch in the key and in range (C major, A minor, E major); seed 7 twice gives the same notes, seeds 1 and 2 differ on a 16-note clip; *stay near 1* keeps every in-key note within 1 scale step; *avoid repeats* gives no equal consecutive single notes over 50 seeds; *chords together* keeps C E G as three notes 2 scale steps apart; a one-pitch clip changes.
- Engine: undo restores exactly; undo and *again* refuse after the notes change, after the clip is deleted and after `resetEdit()`; *again* starts from the original notes; nothing is announced when nothing changes; the instrument and built-in sound are unchanged; a transformed clip saves and loads.
- Guide: 14 topics, each with text and an example; the three topic texts contain "retrograde", "inversion" and "2,097,152"; *Your clip* says "drum sound" for a drum clip and "nothing to mirror" for one pitch; searching "retrograde", "inversion" and "randomise" finds *What is MIDI?*.

## Website hand-off
- `features.html`: the editing and arranging group, a short item on *Transform* (reverse in time, reverse in pitch, random pitches in the key).
- `learn.html`: the MIDI guide card gains "backwards, upside down and new notes".
- Screenshots: the piano roll of *Lead vocal* before and after *Reverse in time*, and the guide at *Upside down: inversion*. No new library for `about.html`.

## Left for later
- A range the user types, and a preview window with Keep and Cancel (Decision 3), shared with #42's *Amount* design.
- Full undo and redo (#92), which would replace the one-step snapshot.
- Retrograde inversion as one item (do both in turn), and an inversion axis the user picks.
- Chord-aware randomising (notes from the chord at each beat) and phrase endings on the home note (#85).
- Per-item tooltips: JUCE popup menus have none.

## Changes while building

**Group A hand-off (engine and guide, built; `./build.sh --test` passes with no warnings)**
- `src/engine/MidiTransforms.{h,cpp}`, namespace `daw::miditransform`, in both CMake targets. Types: `Notes` (vector of `MidiNote`), `Selection` (`juce::SortedSet<int>`, empty = all), `Kind` (`reverseInTime`, `invertInKey`, `invertExactly`, `randomise`), `RandomOptions { stayNear, avoidRepeats, chordsTogether }` (defaults 0, false, true). Functions: `getKindName (Kind)` (menu words, "Reverse in time" ... "Randomise pitch", for "Undo ..."), `reverseInTime`, `invertExactly`, `invertInKey`, `randomise`, `apply (Kind, ...)`, `sameNotes`, `getAffected`, `countDistinctPitches`, `getPitchRange` (for the menu's enable rules: disable reverse in pitch when `countDistinctPitches < 2`; disable reverse in time when `sameNotes (notes, reverseInTime (...))`), `snapToKey`, `getKeyPitches`, `getScaleStep`, `getPitchOfStep`, `getRandomRange`.
- `Engine` (`Engine.h` near `setNoteVelocities`): `transformNotes (MidiClip&, Kind, const juce::SortedSet<int>& selection, RandomOptions = {}, uint32_t seed = 0)` (seed 0 = from the time; returns false and announces nothing when nothing changes), `canUndoTransform (clip)`, `getLastTransformKind()` (for "Undo <name>" and the guide topic), `undoTransform (clip)`, `canRandomiseAgain (clip)`, `randomiseAgain (clip, options)`. The snapshot is cleared in `resetEdit()`. The engine uses its own key (`getKey()`) and the clip's `lengthBeats`.
- Guide: `midiguide::Topic` gained `retrograde`, `inversion`, `newNotes` (before `yourClip`; 14 topics). `midiguide::explain (topic, clip, bpm, key, drumSound = false)`: the new last parameter says the clip plays a drum sound; `MidiExplainer::showTopic` already passes it. `midiguide::getTurnAroundExample()` and `turnAroundExampleLength` (8 beats) are the Frere Jacques example for the pictures (named so, because `MidiExplainer.cpp` has its own anonymous `getExampleNotes`).
- For Group B: `MidiExplainer::paint` has an empty `case` for the three new topics (marked "pictures come with the UI group"); the three topic texts end with a sentence describing the picture (before/after rolls, mirror line dashed and notes outside the key outlined, the key's notes lit), which the pictures must match. The Theory Guides entry (summary, `opensFrom`, keywords) is done. The guide item's topic index: `Topic::retrograde` is index 10, `inversion` 11, `newNotes` 12.

**Group B hand-off (UI, built; `./build.sh --test` passes with no warnings, `--export-guides` exits 0)**
- `src/ui/MidiTransformMenu.{h,cpp}` (in `CMakeLists.txt`), namespace `daw::transformmenu`: `build`, `makeSubMenu`, `handle (result, engine, clip, selection, onShowGuide)`, `showFor (target, ...)`, `getButtonTooltip()`, and the guide topic constants. Menu ids are 9001 and up, so `TrackLanes` tries `handle` first for a MIDI clip. Randomise options are read from and written to `transform.stayNear`, `transform.avoidRepeats` and `transform.chordsTogether` in the user settings.
- Piano roll: `transformButton` (84 px, between the octave buttons and Select), enabled with a clip, acting on `grid.getSelectedNotes()`. `PianoRoll::onShowTransformGuide`, `TrackLanes::onShowTransformGuide` and `ArrangementView::onShowTransformGuide` take the topic number; `MainComponent` sends them to `openGuide (Guide::midi, topic)`.
- Clip menu: a *Transform* submenu after *Detect key and chords...* (whole clip; disabled while recording; the pitch items are off on a drum sound).
- `MidiExplainer::Picture::paintTurnAround` draws the three topics (2 or 3 stacked small rolls, the first 8 bars of the clip or the Frere Jacques example, seed 1 and default options for the roll); a click on a roll plays it at the song tempo. Looked at the inversion picture in the app (guide `midi`, topic 11, example clip): the others are not yet viewed, for Group C.
- Not done here: no tests for the menu (UI), no real-app shots, and the "transform" screenshot setting is Group C.

**Group C hand-off (check and docs, done)**
- `ScreenshotRunner` has `"transform"` (`reverse`, `invertInKey`, `invertExact`, `randomise` seed 1; needs `"select"` naming a track; opens the clip editor). Shots looked at: piano roll after reverse and after randomise (Transform button between the octave buttons and Select, nothing cut off, rhythm kept, notes moved), the guide at the three topics with the example and with the clip, and *Your clip* with its turn-it-around line. All fine; nothing needed fixing. A selection kept across a transform was not shown in a shot (the engine keeps note order, so indices stay valid).
- Docs: README bullet, `docs/tests.md`, `docs/code-layout.md`, `docs/recipes.md`. Website hand-off is above.

**Changes while building**
- Random pitches draw one number per group of notes from `std::mt19937` (raw output times the count, shifted right 32 bits); the stated rules otherwise hold. Reverse in time shortens a note ringing past the clip's end only in that transform (the pitch transforms leave lengths alone).
- Texts: the plan's markdown (italics, bold, bullet dashes) became plain text: bullets are "• ", and the *Transform > ...* names appear without italics. Two small wording changes for the rendered text: "the guides to chord progressions and to finding the key and chords help with them" (the plan names them in italics), and each of the three topics ends with one sentence about its picture (not in the plan's Theory).
- The saved velocity is quantised to 1 to 127 on save, so the save-and-load test compares velocity within half a step and notes in order of start (the file lists them that way).


**Theory check**
- Topic texts, *Turning it around* rules, tooltip, menu names, example data and test values match *Theory*; the derived numbers (inversions 67 65 63/64 67 63/64 62 60, C E G to G E♭ C, 8 key notes, 8^7 = 2,097,152, 1 in 8) re-derived and correct.
- `MidiGuide.cpp`: the retrograde picture sentence now says it shows the start of Frère Jacques when no clip is open and names Reverse in time; the inversion and new-notes picture sentences say each roll has its own dashed mirror line, that the roll uses the default options, and that a click plays a roll (as the picture does).
- `MidiGuide.cpp`: the in-key snapping rule gains a concrete example ("C♯ becomes C in C major"), matching the test that snaps 61 to 60.
- `MidiExplainer.cpp`: the inversion picture's "notes outside the key are outlined" label moved from the *In the key* roll (which never has any) to the *Exactly* roll; the *In the key* label is now "In the key: every note stays in the key".
