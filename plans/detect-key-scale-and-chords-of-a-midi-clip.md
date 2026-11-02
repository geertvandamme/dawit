# Detect the key, scale and chords of a MIDI clip

Status: Released
Idea: [#38 — Detect the key, scale and chords of a MIDI clip](https://github.com/geertvandamme/dawit/issues/38)

## Goal
Right-click a MIDI clip (or press *Key and chords* in the piano roll) and DawIt names the part's key and scale (major, minor, a mode, harmonic or melodic minor, pentatonic or blues) with how well it fits and where notes fall outside it, and a chord per bar or half bar with inversions, Roman numerals and the progression's name. Each result says why, with the runner-up when it was close. One click puts the chords on the chord track, sets the song's key or switches on the scale filter. A new guide, *Finding the key and chords*, explains scales, modes and naming chords from notes, so a beginner learns how the answer is reached, not just the answer.

## Decisions for you
1. **Where the result shows.** Options: a message box like the audio *Detect chords...*; or the *Your clip* topic of the new guide, with a picture, the reasons and the action buttons. **Recommended:** the guide topic. The reasons need room and a picture, and the topic follows the selection, so you can click through clips.
   Answer: the recommended option (user, 2026-11-02).
2. **Chords faster than a bar.** The chord track holds one chord per bar. Options: show half-bar chords in the guide and write the chord on the downbeat; or let the chord track hold two chords per bar (a format change touching the chord row, chord styles, play-along, sheets). **Recommended:** the first; the guide says which bars changed halfway. Beat-level changes are left for later.
   Answer: the recommended option (user, 2026-11-02).
3. **Slash chords on the chord track.** `Chord` has no bass note. Options: write C/E as C and show the bass only in the guide; or add a bass note to `Chord` (format change, chord row, sheet export). **Recommended:** the first, and a follow-up issue for a bass note.
   Answer: the recommended option (user, 2026-11-02).
4. **Pentatonic and blues as the song key.** The song key has nine scales; adding pentatonic and blues would break the seven-chord harmony code. **Recommended:** find and explain them, but *Set the song's key* sets the related seven-note key: major pentatonic → major; minor pentatonic and blues → minor.
   Answer: the recommended option (user, 2026-11-02).
5. **Bars where the clip names no chord** (rests, a melody). Options: leave the chord track's chord there; or clear it, as audio *Detect chords* does. **Recommended:** leave it: a MIDI part is often one of several, and silence isn't a chord.
   Answer: the recommended option (user, 2026-11-02).
6. **The automatic suggestion on MIDI file import** (in the issue) depends on *MIDI file import* (#49), which isn't built. **Recommended:** leave it for #49 to call this analysis.
   Answer: the recommended option (user, 2026-11-02).
7. **The guide's name.** **Recommended:** *Finding the key and chords*. Other options: *Keys, scales and modes*; *Scales and chords from notes*.
   Answer: the recommended option (user, 2026-11-02).

## What the user does and sees
- **Opening:** *Detect key and chords...* in a MIDI clip's right-click menu (after *Sing this melody...*), and a `GuideButton` *Key and chords* in the piano roll header next to *Scale*. Both select the clip and open the guide at *Your clip*. The clip menu gets a last item *About keys and scales...* (`guideicon::addItem`) opening the guide at *What a key is*. On a track playing the built-in Drums or Percussion sound with no plugin instrument, both are disabled; the button's tooltip says "Drum notes pick drum sounds, not pitches, so they have no key."
- **The guide window** (ui-design's guide window layout, 1000 × 780): topics on the left, picture, text. *Your clip* analyses the selected MIDI clip; with only a MIDI track selected, its clip under the playhead, else its first clip. It follows selection and edits (Selection and engine listener).
- ***Your clip* picture:** the clip's bars across the width; above each bar (or half bar) the chord name, with its bass as a slash ("C/E"), and the numeral below it ("vi"); the notes as small bars (pitch up, time across) in the chord's colour (`ChordLane::getColour`) when in the chord, grey when not, orange when outside the scale; a 12-bar strip on the right with the note-name profile and the scale's notes marked. Tooltip over a bar: "Bar 3 · Am7 (vi7) · or C/A". Click a chord to hear it on the clip's track (copy `ChordExplainer::playChord`).
- **Actions under the picture on *Your clip*** (copy ChordExplainer's *Put on chord track* placement):
  - *Put chords on chord track*: writes the downbeat chord of each of the clip's bars that has one (decisions 2, 3, 5). If any of those bars already has a different chord, asks "Replace the chords in bars 1 to 8? 3 of them already have other chords (bar 2: Dm, ...)." with *Replace* and *Cancel*. Tooltip: "Writes one chord per bar over this clip's bars on the chord track, without the bass note. Asks before replacing chords already there."
  - *Set the song's key to A minor* (hidden when it already is). Tooltip: "The chord menu, the Roman numerals, the scale filter and Generate follow the song's key."
  - *Show only notes in A minor* (toggle, `Engine::setScaleFilter` on the clip's track; disabled until the song is in the found key). Tooltip: "The piano roll of this track shows only the notes of the song's key. Notes already in clips play as they are."
- **Other topics' pictures:** clicking a scale plays it, clicking a chord plays it (on the selected MIDI track, as the chord guide does).
- **Settings:** none new.

## Data and saving
None. The results write only existing song data (chord map, key, a track's scale filter) through `Engine::setChords`, `setKey` and `setScaleFilter`, which already save. No format change.

## The guide
New guide *Finding the key and chords* (`gen::keyfinding`, `src/generate/KeyFindingGuide`; window `src/ui/KeyFindingExplainer`). Topics (text in *Theory*):

| # | Topic | Summary | Picture |
|---|---|---|---|
| 1 | What a key is | The notes a part uses, and the one that feels like home. | The example tune's 12-note profile (beats per note name), C major's notes lit, C marked home. |
| 2 | Major and minor scales | The two most common scales, and why C major and A minor share their notes. | Two keyboard octaves: C major and A minor, with the W/H steps written between keys. |
| 3 | The modes | Dorian, Phrygian, Lydian, Mixolydian and Locrian: the same seven notes with another home. | Seven rows, one per mode from its white key, the characteristic note highlighted. |
| 4 | Harmonic and melodic minor | Minor scales with a raised seventh (and sixth), for a stronger pull home. | Rows A natural, harmonic, melodic minor, raised notes lit; Em (E G B) beside E (E G# B). |
| 5 | Pentatonic and blues scales | Five-note scales, and the blues scale's extra note. | Rows C major pentatonic, A minor pentatonic, A blues with the blue note marked. |
| 6 | How DawIt finds the key | Counting how long each note sounds and comparing it with every key. | The example tune's profile beside C major's key profile, with the top three keys and their scores. |
| 7 | Naming a chord from its notes | Collect the notes, stack them in thirds, and the bottom of the stack is the root. | E G C B in a bar → note names → stacked C E G B → "Cmaj7". |
| 8 | Inversions and slash chords | The bass note: C, C/E and C/G are the same chord over different notes. | Keyboards for C, C/E, C/G and A C E G over A (Am7, or C/A), bass key lit. |
| 9 | Sus, diminished and added notes | Chords with a note swapped, squeezed or added. | Keyboards: Csus2, Csus4, Bdim, Bdim7, Caug, Cadd9. |
| 10 | Notes between the chords | Passing and neighbour notes: why not every note belongs to the chord. | One bar: held C chord and the tune C D E F G; weight bars 3.2 for chord notes, 0.4 for D and F. |
| 11 | Your clip | The key, scale and chords of the selected MIDI clip, and what to do with them. | The clip picture above. |

**Theory Guides entry:** area harmony; summary "Keys, scales and modes, how a chord is named from its notes, and the key and chords of your own MIDI clip."; opens from "A MIDI clip's menu (Detect key and chords...) and the piano roll's Key and chords button"; search words: key scale mode modes ionian dorian phrygian lydian mixolydian aeolian locrian pentatonic blues harmonic melodic minor inversion slash bass sus suspended diminished augmented add9 passing neighbour detect analyse find. Not on *Start here* (Chord progressions is). The chord guide's *Your chord track* text, when the chord track is empty, adds: "Or right-click a MIDI clip and choose Detect key and chords... to read its chords."

## Code
Recipes: *A theory guide* (copy the lyrics guide; chord playback and the action button from ChordExplainer). Not a long job: a clip's notes analyse in milliseconds on the message thread, which may read the edit without the lock (it is the only writer).

- **The finder** `analysis::midiharmony` (`src/analysis/MidiHarmony.{h,cpp}`), pure: the clip's notes in song beats (cut at the clip's end), 4 beats a bar, in; ranked keys with fit and reasons, outside notes, segments with chord, bass, runner-up and notes between, single-line flag and profiles, out.
- **Scales found:** `MusicalKey`'s nine plus major pentatonic, minor pentatonic and blues (finder-only; decision 4 maps them to a key to set).
- **Weights:** a note counts its length in beats inside the segment × velocity (0 to 1). The bass of a segment is the note name that is lowest for the longest time.
- **Key:** correlate the whole clip's profile with the 24 Krumhansl-Kessler profiles (share `chordfinder`'s profiles and correlation rather than copying them). The best home note, with hints when the top two are within 0.1 (the last bass note, the first segment's chord root, the share of time on each home chord, as `findKey` does), sets the tonic. Then the scale on that tonic is the one with the highest fit; within 3 points the simpler wins, in this order: major, minor, Mixolydian, Dorian, Lydian, Phrygian, harmonic minor, melodic minor, Locrian. Pentatonic or blues only when all its notes sound and no other note has over 2% of the weight. **Relative keys (user, 2026-11-02):** when the top key and its relative major or minor hold the part's notes equally well (both plain scales fit the same), the part's home decides between them (the first chord counts double, then the first bass note, the last bass note and chord, and the time on the home chord), even when the correlation puts the other key ahead by more than 0.1: an Am F C G loop starting and ending on Am is A minor, not C major; the *Your clip* text then says it is close, that they share all their notes, and why. Fewer than 8 notes or 3 note names: no key.
- **Outside-note reasons:** in a minor key, the raised 7th → harmonic minor, raised 6th and 7th → melodic minor, the flat 5th → the blue note; in a major key, the flat 7th → Mixolydian or borrowed, flat 3rd or 6th → borrowed from the parallel minor, the raised 4th → Lydian; a note of half a beat or less that steps between neighbours → a passing note; else "outside the key".
- **Chords:** score every root × all 16 `Chord::getQualities()` against the segment profile: share of the weight in the chord's notes, minus a little per missing note (the fifth may be missing), minus a little per note over three; the bass decides the slash. Ties go to: explains all notes, root in the bass, fewer notes, in the key, same as the previous segment. Root and fifth only: the key's major or minor triad on that root, flagged "no third". A runner-up within 0.05 is reported. A bar splits into halves when the halves' best chords differ, each half has at least 25% of the bar's weight, and their mean score beats the whole bar's by 0.1. A single-line segment (no two notes overlap) gets a chord only when it has 3+ note names of that chord and the chord holds at least 85% of its weight (an arpeggio); a part where no segment does is a melody: key only.
- **Text and actions** in `gen::keyfinding`: topic texts, example data, the *Your clip* text from a result (numerals with `gen::guide::getNumeral`, the name with `findProgressionName` over the merged bar chords), and the pure chord-track merge (which bars get which chord, which existing chords would be replaced).
- **Risk:** threshold tuning against the made-up parts; keep constants together and named, as in ChordFinder.cpp.

## Libraries and models
None.

## Theory

### Facts
1. There are 12 note names (pitch classes); a note name in any octave counts as the same one. Exact (definition).
2. Major scale steps W W H W W W H; C major is the white keys C D E F G A B. Exact. Wikipedia *Mode (music)*.
3. Natural minor steps W H W W H W W; A minor uses C major's notes (relative keys, home three semitones lower). Exact. Wikipedia *Mode (music)*; derived (9 = 0 − 3 mod 12).
4. The seven modes are the white keys from C (Ionian), D (Dorian), E (Phrygian), F (Lydian), G (Mixolydian), A (Aeolian), B (Locrian). Exact. Wikipedia *Mode (music)*.
5. Characteristic notes: Dorian = minor with a major 6th; Phrygian = minor with a minor 2nd; Lydian = major with an augmented 4th; Mixolydian = major with a minor 7th; Locrian has a diminished 5th and a diminished home chord. Exact. Wikipedia *Mode (music)*.
6. Locrian is treated as more theoretical than practical. Typical. Wikipedia *Mode (music)*.
7. Harmonic minor raises the 7th (leading note), making the five chord major (E G# B in A minor), with an augmented second (3 semitones) F–G#. Exact. Wikipedia *Minor scale*; derived (8 − 5 = 3).
8. Melodic minor raises the 6th and 7th going up and returns to natural minor coming down in classical use; jazz uses the raised form both ways. Exact. Wikipedia *Minor scale*. DawIt's melodic minor is 0 2 3 5 7 9 11 (`MusicalKey`).
9. Major pentatonic = 1 2 3 5 6 (no 4th, 7th); minor pentatonic = 1 b3 4 5 b7; C major and A minor pentatonic share notes; no half steps. Exact. Wikipedia *Pentatonic scale*.
10. The black keys F# G# A# C# D# form F# major pentatonic. Exact, derived from 9.
11. The six-note blues scale = minor pentatonic + b5; other "blues scales" exist (major blues 1 2 b3 3 5 6, a nine-note one); no single agreed definition. Exact. Wikipedia *Blues scale*.
12. Blue notes are often played between semitones; MIDI notes are whole semitones. Exact. Wikipedia *Blues scale*; MIDI 1.0 note numbers.
13. Krumhansl and Kessler (1982) key profiles from probe-tone ratings: major 6.35 2.23 3.48 2.33 4.38 4.09 2.52 5.19 2.39 3.66 2.29 2.88; minor 6.33 2.68 3.52 5.38 2.60 3.53 2.54 4.75 3.98 2.69 3.34 3.17. Exact. Searched (fastled, partitura), as in ChordFinder.cpp.
14. Krumhansl-Schmuckler: the total duration of each note name is correlated with the 24 key profiles; the highest wins. Exact. Searched (Robine et al. ICMC 2008, Temperley).
15. Key finders' usual near misses are a fifth, the relative and the parallel key; the MIREX score gives 1, 0.5, 0.3 and 0.2. Exact (weights). Searched (arXiv 1706.02921, mir_eval).
16. Root position, first (3rd in bass), second (5th) and third inversion (7th, seventh chords only); the lowest note decides. Exact. Wikipedia *Inversion (music)*.
17. A slash chord is chord/bass, "over", not "or"; F/D has Dm7's notes. Exact. Wikipedia *Slash chord*. A C E G over A = Am7 = C/A, derived likewise.
18. Sus2/sus4 replace the third with a 2nd/4th; Csus2 and Gsus4 share C D G. Exact. Wikipedia *Suspended chord*; derived.
19. A dim7's notes are 3 semitones apart; only three different dim7 chords; Cdim7 = Ebdim7 = F#dim7 = Adim7. Exact. Wikipedia *Diminished seventh chord*.
20. An augmented triad's notes are 4 semitones apart: Caug, Eaug, G#aug share notes. Exact, derived (3 × 4 = 12).
21. add9 adds the 9th without a 7th; with a 7th it is a ninth chord. Exact (definition; `Chord::getName`).
22. Passing note: a step between chord notes in one direction; neighbour: a step away and back; suspension and appoggiatura fall on the beat. Exact. Wikipedia *Nonchord tone*.
23. A C E G over C (C6) has the same notes as Am7. Exact, derived.
24. The example tune's profile (C 6, D 2, E 3, F 1, G 3, A 1 beats): C major correlation 0.948, A minor 0.586. Exact, derived (Pearson correlation with fact 13).
25. Passing-note weights: a 4-beat note at velocity 0.8 = 3.2; an eighth note (0.5 beat) at 0.8 = 0.4; ratio 8. Exact, derived.
26. Moods of modes (bright, dark, floating) are descriptions players differ on. Opinion; the text says "often heard as".

### Topic texts

**1. What a key is** — *The notes a part uses, and the one that feels like home.*

A key is two things: a set of notes that a piece mostly uses, and one note among them that feels like home. A song in C major uses mainly the seven notes C, D, E, F, G, A and B (the white keys of a piano), and C is home: tunes come to rest on it, and songs often end on it.

Music has 12 different note names: C, C#, D, Eb, E, F, F#, G, Ab, A, Bb and B. After B comes C again, an octave higher. For finding a key the octave doesn't matter: a low C and a high C count as the same note name.

The picture shows a short tune: C E G E | F A G | E D C D | C. Each bar is how long one note name sounds, in beats: C sounds for 6 beats, E and G for 3, D for 2, F and A for 1. Every note is one of C major's seven, and C sounds longest and comes last. That's why the tune is in C major.

A key isn't a rule. Songs use notes from outside their key all the time, for colour or tension, and some change key halfway. A key describes what a part mostly does; it doesn't forbid anything.

This guide doesn't explain how the chords of a key work together (I, IV, V and so on): that's the *Chord progressions* guide.

**2. Major and minor scales** — *The two most common scales, and why C major and A minor share their notes.*

A scale is the notes of a key in order, from home up to home an octave higher. What makes scales differ is the pattern of steps between the notes. A whole step (W) is two semitones, like C to D. A half step (H) is one semitone, like E to F: two piano keys right next to each other.

The major scale's pattern is W W H W W W H. From C it gives C D E F G A B C, all white keys. From G the same pattern needs one black key: G A B C D E F# G. Every major key uses this pattern, so they all sound alike, just higher or lower.

The natural minor scale's pattern is W H W W H W W. From A it gives A B C D E F G A, again all white keys. Its third note is a semitone lower than in A major (C instead of C#), and that gives minor its darker sound.

C major and A minor use exactly the same seven notes. They're called relative keys. Every major key has a relative minor whose home is three semitones lower: C major and A minor, G major and E minor, F major and D minor. What tells them apart is which note feels like home: where the tune rests, which chord the song starts and ends on, which note the bass keeps coming back to. That's also why a key finder mixes them up more than anything else.

Click a scale to hear it.

**3. The modes** — *Dorian, Phrygian, Lydian, Mixolydian and Locrian: the same seven notes with another home.*

Play the white keys from C to C and you hear the major scale. Play the same white keys from D to D and it sounds different: minor, but with one brighter note near the top. That's the Dorian mode. Each white key can be home, which gives seven modes:
• Ionian, from C: the major scale.
• Dorian, from D: minor with a major sixth (B in D Dorian, where D minor has Bb).
• Phrygian, from E: minor with a minor second (F, just a semitone above home E).
• Lydian, from F: major with a raised fourth (B in F Lydian, where F major has Bb).
• Mixolydian, from G: major with a minor seventh (F in G Mixolydian, where G major has F#).
• Aeolian, from A: the natural minor scale.
• Locrian, from B: its home chord (B D F) is diminished, so it hardly feels at rest. It's more theory than practice.

Players often hear Dorian as minor but warmer, Lydian as bright and floating, Mixolydian as major but relaxed, and Phrygian as dark. Those are impressions, not rules.

A mode can start on any note. D Dorian has the notes of C major, but A Dorian has those of G major: A B C D E F# G. To tell a mode, compare it with the major or minor scale on the same home note and look for the one note that differs, its characteristic note. A part in D that uses B, never Bb, is D Dorian rather than D minor.

Modes only show when home is clear. White-key notes that keep landing on C are C major; the same notes landing on D are D Dorian. When home is unclear, DawIt says so and names both.

**4. Harmonic and melodic minor** — *Minor scales with a raised seventh (and sixth), for a stronger pull home.*

In natural minor, the seventh note is a whole step below home: G in A minor. The five chord is then minor (E G B), and it pulls home only softly. Raise the seventh by a semitone, to G#, and the five chord becomes major (E G# B). G# sits a semitone below A and leans towards it. It's called the leading note.

The harmonic minor scale is natural minor with that raised seventh: A B C D E F G# A. Between F and G# there's now a gap of three semitones (an augmented second), which gives the scale its exotic sound.

The melodic minor scale also raises the sixth, to smooth that gap: A B C D E F# G# A. In classical music it's used like this going up, and coming down it returns to natural minor (A G F E D C B A). Jazz uses the raised form both ways, and that's the form DawIt shows and the song key uses.

Most minor songs mix these: natural minor for most of the tune, and the raised seventh near the end of a phrase, in the five chord. So a G# in a part in A minor doesn't mean a different key. DawIt names the key A minor and says where the G# comes from.

**5. Pentatonic and blues scales** — *Five-note scales, and the blues scale's extra note.*

A pentatonic scale has five notes. The major pentatonic is the major scale without its fourth and seventh: C D E G A. The minor pentatonic is natural minor without its second and sixth: A C D E G. They share their notes, just as C major and A minor do.

They have no half steps, so no two of their notes rub against each other. That's why many guitarists and singers improvise with them first, and why so many folk tunes and riffs fit in them. The black keys of a piano form one: F# G# A# C# D#, the F# major pentatonic.

The blues scale is the minor pentatonic with one note added, the flattened fifth: Eb in A, so A C D Eb E G. It's the "blue note", usually a quick passing note between D and E. On a guitar or a voice, blue notes are often bent somewhere between two notes; MIDI notes are whole semitones, so DawIt sees the nearest one. There's no single agreed blues scale: some books use a major blues scale (C D Eb E G A) or a nine-note one. DawIt looks for the six-note one.

A part that uses just these five or six notes is shown as pentatonic or blues. The song's key can only be one of the seven-note scales, so *Set the song's key* sets A minor for A minor pentatonic or A blues, and C major for C major pentatonic.

**6. How DawIt finds the key** — *Counting how long each note sounds and comparing it with every key.*

DawIt adds up how long each of the 12 note names sounds in the clip, in beats, times its velocity. A whole note played firmly counts for more than a quick, soft grace note. That gives a profile like the bars in the picture.

It compares that profile with one for each of the 24 major and minor keys. Those come from listening tests by Carol Krumhansl and Edward Kessler (1982): listeners heard a key set up, then rated how well each of the 12 notes fitted it. Home scored highest (6.35 in major), then the other notes of the home chord (the fifth 5.19, the third 4.38), then the rest of the scale, and the notes outside the scale lowest (2.2 to 2.5). The key whose profile rises and falls most like the clip's wins. This is the Krumhansl-Schmuckler method, and the score is a correlation from -1 to 1.

For the example tune, C major scores 0.95 and the runner-up, A minor, 0.59: a clear result. When the top two are close, DawIt also looks at where the part starts and ends, because the first chord and the last bass note usually sit on home.

Then it checks the scale. Of the scales on that home note (major, minor, the modes, harmonic and melodic minor, the pentatonics and blues), it picks the one that holds the most of the notes played. The fit is that share, by length and velocity. When two scales fit within 3 points of each other, the simpler one wins (major or minor before a mode), and the few notes outside it are explained instead, like a G# in A minor.

It isn't perfect. The usual mistakes of key finders are a key a fifth away (G major for C major), the relative key (A minor for C major) and the key on the same home note (C minor for C major). Research tests even give part marks for them: half a point for a fifth, 0.3 for the relative key, 0.2 for the parallel one. A short clip, a part with only a few notes, or a riff over one chord gives little to go on. Your ears decide: if the music feels at home on another note, trust that.

**7. Naming a chord from its notes** — *Collect the notes, stack them in thirds, and the bottom of the stack is the root.*

A chord is named after its root and its kind: Am is a minor chord on A. To name a chord from the notes played, DawIt does what a musician does:
1. Collect the note names that sound in the bar (or half bar), in any octave, weighted by how long and how firmly they sound.
2. Find the chord whose notes match them best. A chord's notes stack in thirds from its root: C, a third up E, a third up G, a third up B.
3. Look at the lowest note, the bass. It often decides between chords that share notes.

In the picture a bar plays E, G, C and B in different octaves. As note names that's C E G B, and stacked in thirds from C it's C E G B: Cmaj7.

A chord doesn't need all its notes. The fifth is often left out because it adds little colour: C and E with a C in the bass is still C. The third decides major or minor, so with only a root and a fifth (a power chord) DawIt picks the major or minor chord that's in the key and says the third is missing.

DawIt names the chords the chord track can hold: major, m, 7, maj7, m7, m7b5, dim, dim7, aug, sus2, sus4, add9, m(add9), 9, maj9 and m9. A sixth chord (C E G A) has the same notes as Am7, so it's named Am7 with C in the bass, Am7/C. Other notes it can't place are listed as notes between the chords.

**8. Inversions and slash chords** — *The bass note: C, C/E and C/G are the same chord over different notes.*

A chord's notes can be stacked in any order. What counts is the lowest one, the bass:
• root position: the root in the bass (C E G over C);
• first inversion: the third in the bass (C over E), written C/E;
• second inversion: the fifth in the bass, C/G;
• third inversion, for seventh chords only: the seventh in the bass, C7/Bb.
The slash means "over". C/E is a C chord with E in the bass, not "C or E".

Inversions make a bass line smoother. C, G/B, Am walks the bass down C, B, A while the chords stay simple.

A slash chord's bass can also be a note that isn't in the chord at all: C/D is C E G over a D.

Sometimes the bass decides the name. A, C, E and G over an A are Am7. You could also read them as a C chord over A, C/A: the same notes. Because A is Am7's root, DawIt says Am7 and mentions C/A as the other reading. F/D and Dm7 are another such pair.

Some chords are symmetrical. A diminished seventh's four notes are each three semitones apart, so Cdim7, Ebdim7, F#dim7 and Adim7 are the same notes, and there are only three different dim7 chords. An augmented chord's notes are four semitones apart: Caug, Eaug and G#aug share their notes. For these, DawIt names the chord on the bass note.

The chord track holds one chord per bar without its bass, so C/E goes on it as C. *Your clip* shows the bass.

**9. Sus, diminished and added notes** — *Chords with a note swapped, squeezed or added.*

Most chords are major or minor, but a MIDI part shows other kinds clearly:
• sus2 and sus4: the third is swapped for the second or the fourth. Csus2 is C D G, Csus4 is C F G. With no third they're neither major nor minor, and they sound open, waiting to move. Csus2 and Gsus4 have the same notes (C D G), so the bass decides again.
• diminished (dim): a minor third and a fifth a semitone lower, B D F. Tense; in C major it's the seven chord.
• half-diminished (m7b5): a diminished chord with a minor seventh, B D F A.
• diminished seventh (dim7): three minor thirds stacked, B D F Ab. Very tense; it often leads to the chord a semitone above its root (Bdim7 to C).
• augmented (aug): two major thirds stacked, C E G#. Dreamy and unresolved.
• add9: a major chord plus the ninth (the second, an octave up) without a seventh: Cadd9 is C E G D. With a seventh as well, it's a ninth chord (C9, Cmaj9).

**10. Notes between the chords** — *Passing and neighbour notes: why not every note belongs to the chord.*

A tune moves between the notes of its chords. Hold a C chord for a bar while the tune runs C D E F G: D and F aren't in the chord. They're passing notes, stepping from one chord note to the next in the same direction. A neighbour note steps away and comes back (E F E). These usually fall between the beats and are short.

Some outside notes land on the beat on purpose. A suspension holds a note over from the previous chord, then falls a step to a chord note (Csus4 becoming C). An appoggiatura leaps to an outside note on the beat, then steps to a chord note. Both add tension that then resolves.

That's why DawIt weighs notes by length and velocity. In the picture, each note of the held C chord (4 beats at velocity 80%) counts 3.2, while each eighth-note D and F counts 0.4 (half a beat at 80%). The chord's notes outweigh the passing notes eight to one, so the bar is C, and D and F are listed as notes between the chords.

When outside notes are long and loud, they probably belong to the chord, and DawIt picks a richer one: Cadd9 for a held D.

**11. Your clip** — *The key, scale and chords of the selected MIDI clip, and what to do with them.*

Built from the result, in this order (the rules are below): the key and fit; how close the runner-up is; the notes outside the scale; the song's key; the chords with numerals and the close calls; the progression's name; half-bar changes; what the buttons do. It ends: "Check by ear: play the clip with the chord track and listen whether each chord feels right."

### Example data
- Tune (topics 1, 6): quarter notes at velocity 1, bars C E G E | F A G(2 beats) | E D C D | C (4 beats). Profile in beats: C 6, D 2, E 3, F 1, G 3, A 1; total 16. Scores: C major 0.948, A minor 0.586, C minor 0.560, G major 0.551.
- Topic 7: E3 G3 C4 B4 for 4 beats → C E G B → Cmaj7.
- Topic 8: C (C3 E3 G3), C/E (E2 + G3 C4 E4), C/G (G2 + C4 E4), A2 + C4 E4 G4 → Am7, other reading C/A.
- Topic 9: Csus2 C D G, Csus4 C F G, Bdim B D F, Bdim7 B D F Ab, Caug C E G#, Cadd9 C E G D.
- Topic 10: C3 E3 G3 for 4 beats at 0.8 (3.2 each) plus eighths C4 D4 E4 F4 G4 at 0.8 (0.4 each).

### "Your clip" rules
| Condition | Text | Quiet when |
|---|---|---|
| No MIDI clip found | "Select a MIDI clip (click it in the arrangement), and this topic shows its key, scale and chords." | a clip is found |
| Drum sound track | "\<Track\> plays drum sounds: its notes pick drums, not pitches, so it has no key." | otherwise |
| < 8 notes or < 3 note names | "Only \<n\> notes: too few to tell the key. Chords are still named where notes sound together." | enough notes |
| Key found | "\<Key\> fits best: \<fit\>% of the notes, by length and velocity, are in it." (pentatonic or blues adds "On the chord track and in the song's key this counts as \<key to set\>.") | — |
| Top two within 0.1 | "It's close: \<runner-up\> fits almost as well. \<Relative: "They share all their notes."\> \<Winner\> wins because \<the part ends on X / starts on an X chord / spends most time on X\>." | gap ≥ 0.1 |
| Fit < 85% | "Only \<fit\>% of the notes fit any one key. The part may change key, or use many notes from outside it. Try splitting the clip where it changes and detecting each half." | fit ≥ 85% |
| Notes outside the scale | per note name: "\<G#\> in bar 7: \<reason\>.", reasons from *Code* ("from harmonic minor: it makes the five chord major", "a passing note", "borrowed from A minor"...), at most five names | none outside |
| Song key differs | "The song is set to \<key\>. Set it to \<found\> so the chord menu, the numerals and the scale filter follow this part." | same key ("The song is already in \<key\>.") |
| Chords | "Chords: C | Am | F | G (I vi IV V)." with bars; slash: "Bar 1 is C/E: a C chord with its third, E, in the bass." | melody |
| Close chord | "Bar 3: Am7, or C/A: the same notes; the bass, A, decides." | runner-up gap ≥ 0.05 |
| No third | "Bar 5: G5 played as G and D only; G is the chord in the key." | — |
| Progression name | "This is the \<name\>." | not found |
| Half bars | "Bar 9 changes chord halfway (G, then C). The chord track holds one chord per bar, so it gets G." | none |
| Melody | "This part plays one note at a time and outlines no chords, so DawIt names its key but not chords: a tune's notes alone don't fix the harmony. Detect the chords of the chord part instead." | chords found |

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| KK major profile [0], [4], [7] | 6.35, 4.38, 5.19 | exact |
| Tune: key, score, runner-up | C major 0.948; A minor 0.586 | ±0.005 |
| Block chords C Am F G (whole-note triads, vel 0.8) | C major (0.935 vs A minor 0.830); chords C, Am, F, G; numerals I vi IV V; "Fifties progression" | ±0.005 |
| Same as eighth-note arpeggios | same chords and key | — |
| Walking bass (C2 E2 G2 A2...) under half-note chords | same chords | — |
| Stepwise melody (C D E F E D C...) | C major; no chords; single line | — |
| A minor tune with G# (bars A C E C, D F A F, E G B G, A E C A, F A C A, D F A D, E G# B E, A whole; quarters vel 1) | A minor (0.941); fit 31/32 = 96.9%; harmonic minor fit 93.75%; G# in bar 7 → harmonic minor | ±0.1% |
| D Dorian part (profile D 10, A 5, G 4, B 4, C 3, E 3, F 3 beats) | D dorian, fit 100%; D minor 87.5% | ±0.1% |
| A minor pentatonic part (A C D E G only) | A minor pentatonic, key to set A minor | — |
| A blues part (adds a short Eb) | A blues, key to set A minor | — |
| Inversions: E2+C E G; B1+G B D; A2+C E G; G2+C E | C/E, G/B, Am7 (runner-up C/A), C/G | — |
| Sus, dim, aug: C F G; B D F; B D F Ab; C E G# | Csus4, Bdim, Bdim7, Caug | — |
| dim7 and aug symmetry | Cdim7, Ebdim7, F#dim7, Adim7 same pitch classes; Caug = Eaug = G#aug | exact |
| Half-bar split: C for beats 1-2, G for 3-4 | two segments C, G; chord-track merge writes C | — |
| Passing notes (topic 10 example) | chord C; D and F as notes between; weights 3.2 and 0.4 | ±0.001 |
| Power chord G2 D3 in C major | G, flagged no third | — |
| Merge with existing chords | bars with a different chord reported for the question; bars without a found chord untouched | — |
| Scale sets | C major = A minor notes; C major pent = A minor pent; A blues = {A C D Eb E G}; melodic minor 0 2 3 5 7 9 11 | exact |

## Steps

Group A, engine (`./build.sh --test` at step 3):
- [x] 1. `src/analysis/MidiHarmony.{h,cpp}` (both CMake targets): profiles and bass, key ranking with hints, scale choice incl. pentatonic and blues, fit, outside notes with reasons (*Code*). Share the KK profiles and correlation with ChordFinder. Tests: `runMidiHarmonyTests` for the key, scale and fit rows of *Test numbers*. Check: the tune gives C major 0.948.
- [x] 2. Chords per bar and half bar, slash bass, runner-up, power chord, single-line and arpeggio rules, notes between the chords. Tests: the chord rows of *Test numbers*.
- [x] 3. `src/generate/KeyFindingGuide.{h,cpp}` (`gen::keyfinding`): topics, names, summaries, texts and example data from *Theory*, the *Your clip* text from a result with numerals and progression name, the pure chord-track merge. Tests: every topic has text and example; the *Your clip* rules on made-up parts (melody, close relative, outside note, half bar, drums, too few notes); merge. Done when: `./build.sh --test` passes without warnings.

Group B, UI (`./build.sh --test` at step 6):
- [x] 4. `src/ui/KeyFindingExplainer.{h,cpp}`: the guide window with pictures for topics 1 to 10 (click plays a scale or chord), PDF source (recipes *A theory guide* step 3). Add it to `gen::guides` (entry in *The guide*), `ArrangementView::showGuide` / `getPrintGuide`, `MainComponent::openGuide` / `getPrintGuide`. Tests: `runTheoryGuidesTests` search finds it for "dorian", "pentatonic", "slash chord", "inversion".
- [x] 5. *Your clip*: follows the selection (clip, else the track's clip at the playhead, else its first), its picture with tooltips and chord playback, and the three actions with the replace question.
- [x] 6. Opening: the clip menu items *Detect key and chords...* and *About keys and scales...* (last, `guideicon::addItem`) in `TrackLanes::showClipMenu`, the piano roll's *Key and chords* `GuideButton`, both disabled on drum tracks; the chord guide's empty-track line from *The guide*. Done when: `./build.sh --test` passes and `--export-guides` writes the new guide.

Group C, check and docs:
- [x] 7. Real-app check with `--screenshots`: (a) `select` "Keys", `guide` the new guide at "Your clip" (chords Am F C G with numerals, the key named, buttons visible); (b) same with "Lead vocal" (melody: key only); (c) "Bass"; (d) topics "The modes" and "Inversions and slash chords" (pictures fill, no clipped text); (e) the piano roll on "Keys" with the *Key and chords* button. If "Your clip" can't find a clip from a track selection, add a clip selection to `ScreenshotRunner` for good.
- [x] 8. Docs: README *Features* bullet (where, the clicks, what it finds, the guide); `docs/code-layout.md` (MidiHarmony, KeyFindingGuide, KeyFindingExplainer); `docs/tests.md` (MIDI key and chords area); `docs/architecture.md` layer table: `src/analysis` also reads MIDI notes (MidiHarmony), and the guides list; issue #38 progress comment (built, still open: import suggestion, beat-level chords, bass on the chord track); a follow-up issue for decision 3 if accepted.

## Tests
As *Test numbers*, in `runMidiHarmonyTests` (steps 1 and 2) and `runKeyFindingGuideTests` (step 3): key and scale results with scores and fits; chord names on every made-up part; the *Your clip* text contains the expected phrases ("fits best", "It's close", "harmonic minor", "halfway", "names its key but not chords", "plays drum sounds", "too few to tell"); every topic has non-empty text and example data; merge keeps bars without a found chord and reports replaced ones; Theory Guides search for the guide's words.

## Website hand-off
- `features.html`: the *Analysing songs* group, beside *Detect chords* on audio: "Key and chords of a MIDI clip".
- `learn.html`: a card for *Finding the key and chords* (harmony area).
- Screenshots: the guide at *Your clip* on the pop song's Keys track (Am F C G, numerals, key), and *The modes* topic.
- No new library: `about.html` unchanged.

## Left for later
- The suggestion on MIDI file import (#49 should call this).
- Chords per beat, and more than one chord per bar on the chord track (decision 2).
- A bass note on the chord track for slash chords (decision 3).
- Key changes inside a clip (today: the advice to split the clip).
- Several clips or tracks analysed together (the whole song's MIDI), and *Play chords in a style* reading chords from a clip (#25).
- 6, 11 and 13 chords, power chords as their own symbol: the chord track's qualities don't have them.
- Time signatures other than 4/4 (#137).

## Changes while building

### Group A hand-off (engine, built; `./build.sh --test` passes, no warnings)
- **Finder:** `src/analysis/MidiHarmony.{h,cpp}`, `daw::analysis::midiharmony`. `makeInput (const MidiClip&)` gives the notes in song beats, cut at the clip's end; `analyse (Input)` returns a `Result` (`tooFew`, `key` (`FoundKey`: tonic, `Scale` incl. pentatonic and blues, `fit`, `songKey` = what *Set the song's key* sets, `name`), `ranking` (top 3 `KeyScore`), `close`, `relative`, `closeReason`, `runnerUp`, `outside` (`OutsideNote`: pitch class, `firstBar` 0-based, `share`, `reason`), `segments` (`Segment`: bar 0-based, `split`/`secondHalf`, `weights`, `bass`, `chord`, `chordBass` (-1 = root), `score`, `noThird`, `otherChord`/`otherBass`/`otherGap`, `between`), `singleLine`, `melody`, `profile`). `findChords (Input, optional<MusicalKey>)` gives only the segments. Helpers: `getChordName (chord, bass)` ("C/E"), `getSpelledName (pc, tonic)` ("G#" in A minor), `getKeyName`, `getScaleIntervals`, `getSongKey`, `getWeight`.
- **Shared profiles:** `chordfinder::getKeyProfile (minor)` and `correlateKey (profile, minor, tonic)` added to `ChordFinder.h`.
- **Guide content:** `src/generate/KeyFindingGuide.{h,cpp}`, `gen::keyfinding`: `Topic` / `allTopics` (11, `yours` last), `getTopicName`, `getTopicSummary`, `getText (Topic)` (topics 1-10 verbatim from *Theory*; `yours` gives the "Select a MIDI clip" prompt), `getExample (Topic)` (`Example`: `rows` of `Row` (label, steps, pitchClasses, pitches to play, marked, home, bass, note), `notes` for topics 1, 6, 7, 10, `caption`), `getExampleTune()`, `ClipView` + `explainYourClip (ClipView)` (the *Your clip* text; pass `found`, `drums`, `trackName`, the song's key and `analyse` result), `getBarChords`, `mergeIntoChordTrack (segments, existing ChordMap)` -> `Merge` (`toWrite`, `replaced`, `firstBar`, `lastBar`), `applyMerge (merge, existing)` -> new `ChordMap` for `Engine::setChords`, `getReplaceQuestion (merge)` (empty when nothing is replaced). Bars in the text are 1-based; in the structs 0-based.
- **Not yet done (Group B):** the `gen::guides` entry, `KeyFindingExplainer`, `showGuide`/`getPrintGuide` wiring, Your clip picture and actions, menu items, piano roll button, the chord guide's empty-track line. Tests for the guides search ("dorian", "pentatonic", "slash chord", "inversion") go with the entry.
- **Surprises:** see *Changes while building*.

## Changes while building (Group A)
- **Bass of a single-line segment (arpeggio)** is the lowest note, not "lowest for the longest time": in A C E C the note C comes round most often and would have made every arpeggio an inversion (Am/C). Polyphonic segments keep the plan's rule (ties go to the lower note).
- **Topic 7 example:** E3 G3 C4 B4 names the chord Cmaj7 with E in the bass, so the finder says Cmaj7/E (first inversion). The topic text speaks of the chord (Cmaj7), as the plan has it; the picture should not label the bar plain "Cmaj7" if it shows the bass.
- **Chord scoring numbers (my choice, not in *Theory*):** missing chord note -0.08 (missing fifth -0.01), each note over three -0.05; a tie is within 0.005. With -0.03 per extra note the passing-note example came out as Cadd9, so it is 0.05. A half-bar split needs a gain of 0.1 (compared with a 1e-6 tolerance: Cmaj9 over C then G scores exactly 0.9 against 1.0).
- **Single-line chords** are named only for the first nine qualities (no sus, add or ninth chords), so a stepwise C D E F is not read as Cadd9.
- **The finder tells the first-bar arpeggio**: the guide's tune (C E G E | ...) names a C chord in bar 1 because that bar is an arpeggio of C; it is not a pure melody. The tests use a stepwise melody for "key only".
- **Spelling:** `getSpelledName` writes the outside note of a key by its degree (G# not Ab in A minor), used in the *Your clip* text.
- The "A minor tune with G#" is not a close call between A minor and C major (clear win); the close-call test uses a part of C 3, E 2, G 3, A 3, D, F, B 1 beat each ending on A (C major 0.873, A minor 0.818, relative, "the part ends on A").

### Group B hand-off (UI, built; `./build.sh --test` passes, no warnings, `--export-guides` writes the guide: 11 topics, 12 pages)
- **Window:** `src/ui/KeyFindingExplainer.{h,cpp}` (`KeyFindingExplainer (Engine&, Selection&)`, `showTopic (Topic)`, 1000 x 780, PDF source). Pictures for topics 1 to 10 (keyboard rows with note names and W/H steps, profile charts, example notes; click plays), *Your clip* picture (bars, chords with numerals, coloured notes, 12-bar profile strip, tooltip, click plays the chord), three buttons and a status line. `analyseSelection()` finds the clip: selected MIDI clip, else the selected MIDI track's clip under the playhead, else its first. It follows `Selection` and `Engine` changes. A clip over 16 bars shows its first 16 ("Showing bars 1 to 16").
- **Wiring:** `Guide::keyFinding` in `gen::guides` (entry per *The guide*), `MainComponent::showKeyGuide (Topic)` / `openGuide` / `getPrintGuide`, `ArrangementView` (cases + `onShowKeyGuide (bool yourClip)` forwarded from `TrackLanes::onShowKeyGuide`), `ScreenshotRunner::getGuideKey` gives `"keyFinding"`, CMake (`DawIt` only). Clip menu: *Detect key and chords...* (id 23, after *Sing this melody...*) and *About keys and scales...* (id 24, last, `guideicon::addItem`); piano roll: `keyChordsButton` (`GuideButton "Key and chords"`, after *Scale*, `PianoRoll::updateKeyChordsButton` from the timer). Both are disabled on a built-in Drums or Percussion track; the button's tooltip then says why. The chord guide's empty-track text has the new line.
- **Tests:** `runTheoryGuidesTests`: entry (area, name, 11 topics, last "Your clip", not on Start here) and searches "dorian", "pentatonic", "slash chord", "inversion", "detect key", "blues scale".
- **Changes from the plan:** the *Your clip* buttons show only when they apply (*Put chords* when the clip is a MIDI clip on a pitched track, disabled for a melody; *Set key* only when the song's key differs; *Show only notes* when a key was found, disabled until the song is in it). The topic 7 picture's second row says "Cmaj7, with E in the bass: Cmaj7/E" (from `findChords`), not plain "Cmaj7". Notes in the chord take the chord's colour even when outside the scale; orange is for notes outside the scale that are not in the chord. A "Note names" row (topic 7) and the other chord rows lit on a one-octave keyboard; the bass is a green dot (the root a white one).
- **For Group C:** I looked at "Your clip" on Keys, "The modes", "How DawIt finds the key", "Naming a chord", "Inversions" and "Notes between the chords" in the real app (shots in the scratchpad, scale 1); they filled correctly. Surprise: on the pop song's Keys clip the finder reads bars 2 and 6 as F/A and bars 4 and 8 as G/B (the left hand has A and B in the bass), so the shot-list expectation "chords Am F C G" gives Am F/A C G/B; C major fit 100% while the song is A minor. Not changed: it follows the plan's rules.

### Group C hand-off (check and docs, built; `./build.sh --test` passes)
- **Real-app check:** shots (scale 1) for Keys, Lead vocal, Bass and Drums at *Your clip*, *The modes*, *Inversions and slash chords*, and the main window with the piano roll open on Keys. The *Key and chords* button shows beside *Scale* and is enabled on Keys; *Your clip* for Drums says to select a MIDI clip (a drum selection has no pitched clip). Pictures fill, no clipped text. `select` of a track already finds its clip, so no ScreenshotRunner change.
- **Fix:** on the Lead vocal the text said "E phrygian fits best" then "It's close: G major ... They share all their notes. E minor wins ...". Now "They share all their notes" only for plain major/minor, and the winner is named with the found scale ("E phrygian wins because ...").
- **Open (not changed):** the pop song's Keys clip (Am F/A C G/B, starts and ends on Am) reads as C major 100%, while the song is A minor. The two keys differ by more than 0.1 in the Krumhansl-Kessler correlation, so the plan's rule (hints only inside 0.1) does not apply. The text then tells the user to set the song to C major, without saying A minor uses the same notes. A fix would change the key rule in *Theory*, so it is left for the user: either widen the hint window for relative keys or add a line that the relative minor fits equally and the part starts and ends on its chord.
- **Docs:** README bullet, `docs/code-layout.md`, `docs/tests.md`, `docs/architecture.md` (layer table), issue #38 comment, follow-up issue #140 (bass note on the chord track).
- **Relative-key rule (user, 2026-11-02):** after the 24 keys are ranked, if the top key's relative (major or minor) fits the notes equally (plain major and minor scale fit equal) and has a strictly higher home score (last bass note on its tonic, first chord its tonic chord, share of time on its tonic chord), it becomes the winner whatever the gap, and `close` and `relative` are set so the text names the other key and the reason. Existing test numbers are unchanged. New tests: Am F C G (8 bars, starting and ending on Am, also with F/A and G/B in the bass) is A minor; the same chords starting and ending on C are C major.

### Theory check
- Topic 2: "a key finder mixes them up more than anything else" softened to "one of the commonest mix-ups of a key finder" (fact 15 lists the fifth first; no source ranks the relative key first).
- Topic 6: the hints paragraph now gives the 0.1 gap and the relative-key rule (user, 2026-11-02): equal scale fit, start counted double, Am F C G starting and ending on Am is A minor although C major correlates 0.93 against 0.83 (derived from fact 13, as the block-chord test).
- Topic 7: the example says its lowest note is E, so DawIt writes Cmaj7/E, matching the picture; a new paragraph explains the arpeggio rule (3+ note names of a plain chord, 85% of the weight, bass the lowest note) and why a scale-wise tune gets no chords.
- `MidiHarmony.cpp`: the flat-seventh reason now names what it is borrowed from ("from Mixolydian, or borrowed from C minor: the flat seventh").
- Open, code: outside-note reasons treat only plain minor (and pentatonic/blues) as a minor key, so in a part found as harmonic or melodic minor, Dorian or Phrygian a flat 7th gets the Mixolydian reason and a flat 5th the Lydian one; the minor branch should follow the found scale's third.
- Open, code: `describeBass` calls any bass 3 or 4 semitones above the root "its third" and 7 above "its fifth" without checking the chord holds that note (an Eb bass under C, a G bass under Caug).
- Open, for the user: the chord guide calls vi IV I V "Sad pop" in a major key, while this finder calls the same loop starting and ending on Am A minor (i VI III VII). Both readings are common; the guides don't contradict each other outright, but a user may see both.
- **Text fixes after the theory check:** the outside-note reasons now take the major or minor branch from the found scale's third (a minor third means minor), so a part found as harmonic or melodic minor, Dorian or Phrygian no longer gets the Mixolydian or Lydian reasons; a flat seventh or sixth missing from such a scale is "from natural minor: the flat seventh/sixth". `describeBass` names the bass the chord's third or fifth only when the chord holds that interval, else "X in the bass" or "X, which isn't in the chord, in the bass" (C over Eb). Tests: A harmonic minor with a G; C over Eb. The Lydian-before-passing-note order is unchanged.
