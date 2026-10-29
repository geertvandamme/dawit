# Score view: MIDI as sheet music

Status: Built, not committed. Theory checked, with open points
Idea: [Score view: MIDI as sheet music](../FEATURE_IDEAS.html#score-view-midi-as-sheet-music)

## Goal
A *Score* switch in the clip editor writes the selected MIDI track out as sheet music: clef, key and time signature, notes, rests, ties and beams, chord symbols and section names. It is read-only, and it exports to a PDF a musician can play from. A new guide, *How to read music*, teaches a beginner to read what they see, using their own part as the last example.

**Scope (the user's choice):** read-only view and PDF export only. No editing in the score and no MusicXML.

**Ties with other ideas:**
- *Export to PDF: sheet music and guitar chord sheets*: this plan builds the **sheet music PDF for one track** (title, tempo, key, time signature, bar numbers, chord symbols, section names as rehearsal marks, A4 or Letter). It leaves out several parts or a full score, repeats, transposing parts (B♭ trumpet) and drum notation.
- *Learn to read notes on the piano*: this plan builds the **shared staff drawing** (`gen::notation` for spelling, staff positions, key signatures and clefs, and `ScorePainter` for the drawing). It also writes the teaching of landmarks and mnemonics as guide topics. It leaves out the exercises, levels, scoring and progress.
- *Reading music and ear training*: this plan covers the **reading** half (staff, clefs, key signatures, accidentals). It leaves out dynamics, articulation and ear training.

## Decisions for you
1. **What the score shows.** Options: the selected clip only; or the selected clip's whole track (every MIDI clip on it, from bar 1). **Recommended:** the whole track, scrolled to the selected clip. Bar numbers then match the ruler, and the PDF is a whole part.
   Answer: Whole track, scrolled to the selected clip (recommended).
2. **Voices.** Options: one line of notes per staff, where a note still held when the next note starts is written up to that next note; or two voices (stems up and down) for held notes. **Recommended:** one voice per staff for now. Splitting voices is the hardest part of reading MIDI, and the tooltip shows the real length. Two voices are listed under *Left for later*.
   Answer: One voice per staff (recommended).
3. **Music font.** Options: Bravura (SIL OFL 1.1, the reference font of the SMuFL standard, a new pinned dependency); or clefs and rests drawn as hand-made paths. **Recommended:** Bravura. Clefs, rests and accidentals drawn by hand look amateur, and OFL allows bundling the font in an app.
   Answer: Bravura (recommended).
4. **Saving the clef choice.** Options: per track in the project (a missing key loads as Auto, so no version change); or not saved. **Recommended:** in the project. It belongs to the part, the way a printed part keeps its clef.
   Answer: Saved per track in the project (recommended).
5. **Where the guide sits.** Options: a new *Reading music* area in the Theory Guides window (the reading-notes exercises and ear training would join it later); or under *Rhythm*. **Recommended:** a new area. Leave it off *Start here*, since DawIt's start path is about making music.
   Answer: A new *Reading music* area (recommended).
6. **Splitting a grand staff.** Options: at middle C (C4 and up go to the treble staff); or by hand position. **Recommended:** at middle C for now. Splitting by hand is left for later.
   Answer: At middle C (recommended).
7. **Spelling notes outside the key.** Options: follow the chord sheet (flats, unless the key signature has sharps), but take chord tones from the bar's chord and write the raised 6th and 7th in minor as sharps or naturals; or spell by melodic direction (sharps going up, flats going down), as engravers do. **Recommended:** the first. It agrees with the chord sheet and chord guide, and it is testable. Melodic direction is left for later.
   Answer: Follow the chord sheet (recommended).
8. **Syncopation.** Options: always tie at the beat and at the middle of a 4/4 bar; or allow the short forms (eighth, quarter, eighth, or a half note on beat 2). **Recommended:** always tie. Beginners find it easiest to count, and it is what Gould and the Berklee article teach. The guide says the short forms exist.
   Answer: Always tie (recommended).
9. **The six-sharp/six-flat key.** DawIt's chord guide writes position 6 of the circle as 6 sharps (F♯ major, D♯ minor). The score follows it, so the idea's example "G♭ in E♭ minor" comes out as F♯ in D♯ minor. **Recommended:** stay consistent with the chord guide.
   Answer: Stay consistent with the chord guide (recommended).

## What the user does and sees
- **The switch.** The clip editor's header (`PianoRoll`) gets two linked toggle buttons, *Piano roll | Score*, at its left. The choice is remembered in `view.clipEditorMode`. View > *Score* (Cmd+8, ticked) shows the clip editor in score mode. When an audio clip is selected, the wave editor still shows.
- **The header in score mode.** The title (track name and bars) is kept. Snap, Octave, Select, Scale and Generate are hidden. In their place: *Clef* (a menu), zoom − / +, *Sheet music (PDF)...*, a `GuideButton` "How to read music", and ×.
- **The body (`ScoreView`).** One long line of music that scrolls horizontally. The clef and key signature stay pinned at the left edge while you scroll. On top, from the top down:
  - section names in boxes and the tempo mark (♩ = 120, and again where the rounded tempo changes by 2 BPM or more);
  - chord symbols where the chord changes, and at each section start;
  - bar numbers above every bar;
  - the staff, or a grand staff with a brace.

  Everything is light ink (`theme::text`) on `theme::panel`. The bars of the selected clip get a faint `accent` wash. The playhead is a red line, and notes that are sounding now light up in `accent`. While playing, the view follows the playhead as the piano roll does.
- **Zoom.** The staff space is 6 to 16 px (default 9), saved in `view.scoreZoom`. Cmd+scroll zooms; scroll and Shift+scroll move. If the grand staff doesn't fit vertically, it scrolls vertically.
- **Interaction (read-only):**
  - Click a note to hear it with the track's sound. Click anywhere else to move the playhead to that beat.
  - Double-click a note to switch to the piano roll with that note selected. Double-click elsewhere to open the piano roll at that bar.
  - Right-click opens a menu: *Clef* ▸ (Auto, Treble, Bass, Grand staff, Guitar: treble an octave up, Bass guitar: bass an octave up), *Show chord symbols* (ticked, `view.scoreChords`), *Export Sheet Music (PDF)...*, *Show in Piano Roll*, and last *How to read music* (`guideicon::addItem`).
  - There is no Delete and no copy or paste: nothing in the score changes the song.
- **Tooltips.** Over the score they say what's under the mouse:
  - a note: "E4 · quarter note · bar 3, beat 2 · in the chord Am · played 0.03 beats late, 0.94 beats long";
  - a tied note: "... tied: one sound of 1.5 beats";
  - a rest: "Quarter rest · bar 3, beat 4";
  - the clef: "Treble clef: its curl circles the line for G4, the G above middle C";
  - the key signature: "Key signature of D major: F♯ and C♯ in every octave";
  - the time signature: "4/4: four quarter-note beats in a bar";
  - a chord symbol: "Am, from the chord track";
  - a triplet bracket: "Triplet: three notes in the time of two".

  Every button has a tooltip too, for example *Clef*: "Which staff the part is written on. Auto picks the one that needs the fewest ledger lines".
- **Empty states.** With no track selected: "Select a MIDI track or clip to see its part written out." With a track that has no notes: "This track has no notes yet. Draw some in the piano roll, then come back to Score."
- **PDF.** File > *Export Sheet Music (PDF)...* (disabled unless a MIDI track is selected) and the header button both open a small dialog: the paper (A4 or Letter, `export.sheetPaper`), then where to save. The default name is "<song> - <track>.pdf".

  The pages are portrait. Page 1 has the song title at the top (the project's name, or "Untitled"), the part (track name) on the left, and the tempo mark and key below them. The lines of music are justified to the page width, and a bar is never split across lines. Every line starts with the clef, the key signature and the bar number. Only the first line has the time signature. The last bar ends with a final bar line. Every page has its number and "Made with DawIt" at the foot. Ink is black on white.

## Data and saving
- `Track` gets a **score clef**: Auto (the default), Treble, Bass, Grand staff, Guitar or Bass guitar. It is saved as `"scoreClef"` (left out when Auto). There is no version change, and older projects load as Auto. It is set through an engine setter under the lock that calls `sendChangeMessage()`. It is not structural and the audio thread never reads it.
- App settings: `view.clipEditorMode`, `view.scoreZoom`, `view.scoreChords`, `export.sheetPaper`.
- Test: save and load *Bass guitar*; Auto is not written; a project without the key loads as Auto.

## The guide
A new guide, **How to read music** (`gen::readmusic`, window `ReadingExplainer`, copying the lyrics guide), in the new *Reading music* area. Every picture is drawn by `ScorePainter` from an example score made by `gen::notation`. Clicking a note in a picture plays it, and *Play* plays the topics that have a rhythm.

| # | Topic | Summary | Picture |
|---|---|---|---|
| 1 | The staff | Five lines: higher on the page is higher in pitch | Treble staff, C4 up to G5 step by step, letters under, lines and spaces in two shades |
| 2 | Treble and bass clefs | The clef says which line is which note | Treble staff with G4 on its line (accent), bass staff with F3, middle C between |
| 3 | Notes in the treble clef | E G B D F on the lines, F A C E in the spaces | The nine notes named, with a keyboard below (`keyboardpicture`) lighting them |
| 4 | Notes in the bass clef | G B D F A on the lines, A C E G in the spaces | The same for the bass staff |
| 5 | Ledger lines and middle C | Short extra lines above and below the staff | C4 and A3 under the treble staff, C4 and E4 over the bass staff, C6 and C2 |
| 6 | The grand staff and the piano | Two staves joined: right hand above, left below | Grand staff C3 G3 C4 E4 G4 C5, keyboard lit, landmarks F3, C4, G4 |
| 7 | Sharps, flats and naturals | Up or down a semitone, until the bar line | The two accidental bars (example data) |
| 8 | Key signatures | The sharps or flats at the start of every line | The song's key signature in both clefs, then G, D, F and B♭ major |
| 9 | Time signature, bars and tempo | 4/4, bar lines, bar numbers and ♩ = 120 | A line start with the song's tempo, bars 1 and 2, final bar line |
| 10 | Note values on the staff | Stems, flags, beams, dots, rests and triplets | The two note-value bars |
| 11 | Ties and the beat | Why one MIDI note can be written as two | MIDI blocks on top, the tied score below |
| 12 | Chord symbols above the staff | Am over the bar: what a band plays from | Two bars with Am and F, a boxed "Chorus" |
| 13 | How DawIt writes your MIDI | Rounded for reading; the MIDI stays as played | A played rhythm slightly off the grid on top, written below, move arrows |
| 14 | Your part | The selected track, written out and explained | The four bars round the playhead of the selected (or first) MIDI track |

- **Your part** follows the selected MIDI track (or else the first MIDI track with notes) and the song's changes.
- **Opens from:** the score header's guide button, the score's right-click menu, and the sheet-music export dialog. The rhythm guide's *Note values* text gets one closing sentence: "The guide How to read music shows them on the staff."
- **The `gen::guides` entry:**
  - area: *Reading music*, summary "Notes on the staff, clefs, keys and the score", with its own colour from `TheoryGuidesView::getColour`;
  - title: "How to read music";
  - summary: "Read sheet music: the staff, the treble and bass clefs, note names, ledger lines, sharps and flats, key signatures, and how DawIt writes your MIDI as a score.";
  - opensFrom: "The Score view's guide button and right-click menu, and File > Export Sheet Music (PDF)...";
  - keywords: sheet music, notation, score, staff, stave, clef, treble clef, bass clef, G clef, F clef, note names, ledger line, middle C, grand staff, sharp, flat, natural, accidental, key signature, read music, sight-reading, "Do I need to read music";
  - not on *Start here*;
  - `showTopic`, a case in `MainComponent::openGuide`, and `getPrintGuide`.

## Code
Recipes: *A theory guide*, *A window* (the guide), *A command or menu item* (View > Score, File > Export Sheet Music), *Song data that's saved* (only the per-track clef), and *Checking in the real app*.

- **`gen::notation`** (`src/generate/Notation.{h,cpp}`, pure and tested) turns notes, the key, the chords, the sections, the tempo map and a clef choice into a **Score**. A Score has staves, and each staff has bars. A bar holds events (a chord of written pitches, or a rest) with a written value (whole to 32nd), a dot, a triplet flag, a tie to the next event, beam groups and a stem direction. It also holds the source note indices, so tooltips can give the played start and length.
  - A written pitch is a letter, an alteration, an octave and a staff step (0 = bottom line), plus whether an accidental is shown.
  - Engraving is also pure: x positions in staff spaces per event, and line breaks for a given width.
  - The `ScorePainter` (UI) only scales and draws. The rules it keeps are in *Theory* (rules R1 to R14).
- **Invariants:**
  - written durations in every bar of every staff add up to exactly 4 beats;
  - a tie only joins equal pitches;
  - x grows with the beat inside a line;
  - a bar is never split across lines;
  - the MIDI is never changed.
- **`gen::readmusic`** (`src/generate/ReadingGuide.{h,cpp}`) holds the topics, the texts, the example scores (built through `gen::notation`) and `analysePart` for *Your part*.
- **UI:**
  - `ScorePainter` (`src/ui`): draws a Score line (or a guide example) with given ink colours at a staff-space size, using SMuFL glyphs from Bravura through `juce::Typeface::createSystemTypefaceFor` on embedded binary data. Note heads, stems, beams, ledger lines, ties and bar lines are paths. Clefs, accidentals, rests, flags, time digits and the brace are glyphs. One painter serves the panel, the guide and the PDF.
  - `ScoreView`: the panel body.
  - `SheetMusicExport`: the dialog and the pages, through `pdf::Document`. Pages are rendered at 300 dpi, because notation needs crisper lines than the chord sheet's 200.
- **Threading:** `ScoreView` copies the track's notes, key, chords, sections and tempo under `Engine::getLock()` on a change message, then builds the Score on the message thread outside the lock. A track of a few thousand notes takes milliseconds. Repaints come from the playhead timer, in small regions. Nothing touches the audio thread.
- **Read-only rule (as the Signal Flow window):** `ScoreView` calls no engine function that changes the song. Its only writes are the selection, the playhead, note previews, its settings and the track's score clef.
- **Risks:** glyph rendering of Private Use Area code points through JUCE's typeface (check it early, in step 7). The notation rules are the bulk of the work: steps 2 and 3 carry most of the tests.

## Libraries and models
- **Bravura** (Steinberg), SIL Open Font License 1.1, the reference font of SMuFL (Standard Music Font Layout).
  - Take only `redist/otf/Bravura.otf` from <https://github.com/steinbergmedia/bravura>, pinned to a commit with `file (DOWNLOAD ... EXPECTED_HASH SHA256=...)` in CMake (fill in the hash when adding).
  - Embed it with `juce_add_binary_data`. It runs on the user's machine.
  - Ship its `LICENSE.txt` in the app's Resources, and name it in the README's *License note* and on *How it's made*.
  - Note its size in the README when adding it (not confirmed here).
  - Glyph code points come from SMuFL's `glyphnames.json` (for example gClef U+E050, fClef U+E062).

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| F1 | A staff has 5 lines and 4 spaces; each line or space is one letter step; letters A–G repeat | Open Music Theory 1.03 | exact |
| F2 | Treble (G) clef: its curl circles line 2 = G4. Bass (F) clef: its dots surround line 4 = F3 | Wikipedia *Clef*; OMT 1.03 | exact |
| F3 | G4 is a fifth above middle C and F3 a fifth below; 67 − 60 = 7, 60 − 53 = 7 semitones | Wikipedia *Clef*; derived | exact |
| F4 | Middle C is C4 = MIDI note 60 (DawIt's convention, as the MIDI guide states) | MIDI guide | exact |
| F5 | Treble lines E4 G4 B4 D5 F5, spaces F4 A4 C5 E5. Bass lines G2 B2 D3 F3 A3, spaces A2 C3 E3 G3 | OMT 1.03 | exact |
| F6 | The mnemonics Every Good Boy Deserves Football/Fudge, FACE, Good Boys Deserve Fudge Always, All Cows Eat Grass are common, and there are many variants | search (music-theory-practice.com, Dummies) | typical |
| F7 | Ledger lines continue the staff. C4 is 1 ledger line below the treble and above the bass staff; A3 is 2 below the treble; E4 is 2 above the bass; C6 is 2 above the treble; C2 is 2 below the bass | OMT 1.03; derived from F5 | exact |
| F8 | Grand staff: treble over bass, joined by a line and a brace, used for piano; usually the left hand plays the bass staff | OMT 1.04 | exact (the hand: typical) |
| F9 | An accidental lasts until the bar line, for the same staff position; a tie carries it over the bar line; another octave is not affected (many publishers repeat it anyway) | Wikipedia *Accidental* | exact (the reminders: opinion) |
| F10 | Sharps come in the order F C G D A E B, flats B E A D G C F. Major tonic = a semitone above the last sharp, or the second-last flat; 1 flat = F major; a relative minor is 3 semitones below and shares the signature | Wikipedia *Key signature* | exact |
| F11 | The key signature is on every line; the time signature only at the start (or where it changes); the order is clef, key signature, time signature | Schmidt-Jones (LibreTexts) via search | exact |
| F12 | Stems: below the middle line go up (right of the head), on or above go down (left); a middle-line note may go either way (DawIt: down); about an octave long; in a beamed group the extreme notes decide | Wikipedia *Stem* | exact (convention) |
| F13 | Beams group notes by beat and don't cross major subdivisions of the bar; rests usually break them | Wikipedia *Beam* | typical |
| F14 | In 4/4 the middle of the bar stays visible: only notes starting on beat 1 (whole, half, dotted half) cross beat 3; others are tied there; a note that starts off a beat is tied at the next beat; some writers allow the short forms | Berklee *imaginary barlines*; Gould *Behind Bars* p.167 (quoted on musescore.org) | opinion (DawIt follows the strict form) |
| F15 | A whole rest hangs from line 4, a half rest sits on line 3; a whole rest fills an empty bar in any time signature | Wikipedia *Rest*; search (Dummies) | exact |
| F16 | Guitar is written an octave above its sound in treble clef; bass guitar an octave above in bass clef; the 8 under the clef is often left out | Wikipedia *Clef*; search (Dummies, bass) | exact (leaving out the 8: typical) |
| F17 | Bass guitar open strings sound E1 A1 D2 G2 and are written E2 A2 D3 G3 | standard tuning + F16; derived | exact |
| F18 | ♩ = 120 means 120 quarter notes a minute = 120 BPM in 4/4 | definition (rhythm guide) | exact |
| F19 | A dot adds half the value; a triplet is 3 in the time of 2; the eighth triplet is 1/3 beat | rhythm guide | exact |
| F20 | A quarter of a sixteenth is 0.0625 beats = 31 ms at 120 BPM (0.0625 × 0.5 s) | derived | exact |
| F21 | A tie joins equal pitches into one sound; a slur joins different notes (play smoothly) | common knowledge | exact |
| F22 | Order-of-sharps mnemonic: Father Charles Goes Down And Ends Battle (backwards for flats) | common knowledge, one of several | typical |

### Notation rules (`gen::notation`; each is tested)
- **R1 Staff step.** Step = (octave × 7 + letter index) − bottom-line step. The bottom line is E4 (treble) or G2 (bass). The middle line is step 4.
- **R2 Ledger lines.** Above the staff: (step − 8) / 2 for step ≥ 10. Below: −step / 2 for step ≤ −2 (integer division).
- **R3 Auto clef.** Treble if the lowest note is ≥ F3 (53). Bass if the highest is ≤ G4 (67). If both are true, choose by the median (≥ 60: treble). Otherwise a grand staff, split at C4 (60 and up go to the treble staff). No notes: treble. Guitar and Bass guitar write the notes 12 semitones higher, with the clef's 8 below.
- **R4 Spelling.**
  - In the key's scale: the scale-degree letter.
  - Else a tone of the bar's chord: as `chordsheet::describe` spells it.
  - Else in a minor-type key, the natural minor's 6th or 7th degree + 1: the raised degree (F♯/G♯ in A minor, B♮/C♯ in D minor).
  - Else: flats, unless the key signature has sharps.
  - The octave comes from the spelled letter (E♯4 is pitch 65).
- **R5 Key signature.** As `guide::getKeySignature (getCirclePosition)`. Treble steps:
  - sharps F5 8, C5 5, G5 9, D5 6, A4 3, E5 7, B4 4;
  - flats B4 4, E5 7, A4 3, D5 6, G4 2, C5 5, F4 1;
  - bass: the same minus 2 (bass flat F2 at −1).
- **R6 Accidentals shown.** An accidental is shown when the alteration differs from the state of that staff step in this bar (the key signature first, then earlier notes in the bar). Every bar line resets the state. A tied continuation shows none.
- **R7 Display grid.** For each beat, collect the starts and ends inside it. Use eighth-note triplets (steps of 1/3) if their summed rounding error is less than half the sixteenths' (steps of 1/4) and some point is off the eighth grid. Otherwise use sixteenths. Round starts and ends to the beat's grid. A note is at least one grid step long.
- **R8 One voice.** Notes on a staff with the same rounded start form a chord. The chord ends at the earlier of its longest note's end and the next start on that staff.
- **R9 Splitting.** Split at bar lines; at the next beat when the note starts off a beat; and at beat 3 unless it starts on beat 1. Write each piece as the largest value (with at most one dot) that fits, then tie the rest. Triplet notes stay inside their beat.
- **R10 Rests.**
  - Inside a beat, rests fill to the next grid point, then to the beat.
  - Whole empty beats are quarter rests; a half rest only on beat 1 or 3 when both beats of that half are empty.
  - A whole empty bar is one whole rest, centred.
  - No dotted rests.
- **R11 Beams.** Eighths and shorter are beamed within one beat. A rest or a quarter breaks the group, and a group of one gets a flag.
- **R12 Stems.** Single notes: step ≤ 3 goes up, ≥ 4 goes down. A chord or beamed group follows the note furthest from step 4 (a tie goes down).
- **R13 Tempo marks.** One at bar 1 with the rounded BPM. Another where the rounded BPM differs from the last mark by ≥ 2.
- **R14 Chord symbols.** Shown where the chord track's chord changes, and at each section start, spelled by `chordsheet::describe`. Section names are boxed at the bar where the section starts.

### Example data for the pictures
- **The staff:** C4 D4 E4 F4 G4 A4 B4 C5 D5 E5 F5 G5 (60 62 64 65 67 69 71 72 74 76 77 79), whole notes.
- **Treble notes:** lines 64 67 71 74 77, spaces 65 69 72 76. **Bass notes:** lines 43 47 50 53 57, spaces 45 48 52 55.
- **Ledger lines:** treble 60, 57, 81, 84; bass 60, 64, 40, 36.
- **Grand staff:** 48 55 | 60 | 64 67 72; landmarks 53, 60, 67.
- **Accidentals** (C major, quarters):
  - bar 1: 65 (no sign), 66 (♯), 66 (no sign), 65 (♮);
  - bar 2: 70 (♭), 71 (♮), then 65 as a half note (no sign: the bar line cleared the ♯).
- **Key signatures:** the song's key, then G (F♯), D (F♯ C♯), F (B♭), B♭ (B♭ E♭).
- **Note values:** treble, positions in beats:
  - bar 1: half 67 at 0 (stem up); quarter 74 at 2 (down); eighths 72 at 3 and 71 at 3.5, beamed, stems down (72 is step 5, 71 step 4);
  - bar 2: dotted quarter 64 at 0; eighth 65 at 1.5 (flag); triplet 67 69 71 at 2, 2⅓, 2⅔; quarter rest at 3.
- **Ties and the beat:** MIDI notes (start, length):
  - (0, 0.5), (0.5, 1.0), (1.5, 0.5), (2.0, 1.5), (3.5, 1.0); pitches 69 72 71 69 67.
  - Written: eighth, eighth tied to eighth (at beat 2), eighth, dotted quarter on beat 3, eighth tied over the bar line to an eighth.
  - Bar 2 continues with an eighth rest at 0.5, a quarter rest at 1 and a half rest at 2.
- **Chord symbols:** A minor. Bar 1 "Chorus", Am: quarters 69 72 76 72. Bar 2, F: quarters 65 69 72 69.
- **How DawIt writes your MIDI:** played (start, length):
  - (0.03, 0.46), (0.52, 0.41), (0.98, 0.95), (2.04, 0.47), (2.49, 1.40); pitches 64 67 69 67 64.
  - The sixteenths win in every beat (beat 1: error 0.15 against 0.42 for triplets).
  - Rounded: 0–0.5, 0.5–1, 1–2, 2–2.5, 2.5–4 (3.89 rounds to 4.0).
  - Written: eighth, eighth, quarter, eighth, eighth tied to quarter.
  - Starts moved: 0.03, 0.02, 0.02, 0.04, 0.01 beats.

### Topic texts
*{key}*, *{signature}* and *{bpm}* come from the song.

**1. The staff** — *Five lines: higher on the page is higher in pitch*

Written music is drawn on a staff (in Britain often called a stave): five lines with four spaces between them. Each line and each space is one note. A note sits either on a line (the line runs through it) or in a space (between two lines).

Higher on the staff means higher in pitch. From a line to the space just above it is one step up the white keys of a piano: E to F, F to G. From a line to the next line skips a note: E to G. So you can see a tune's shape before you know a single note name: up, down, a leap, a repeated note.

Music uses seven letters, A B C D E F G, and after G they start again at A. The black keys have no letters of their own: they're written with sharps and flats (see *Sharps, flats and naturals*).

The staff alone doesn't say which line is which note. That's the job of the clef, the sign at the start of every line. Notes too high or too low for the five lines get short lines of their own, called ledger lines.

In the picture, each note is one step higher than the one before, from a line to a space and on. Click a note to hear it.

**2. Treble and bass clefs** — *The clef says which line is which note*

The clef at the start of a staff says which notes the lines stand for. Without it, the same dot could be any note.

The treble clef is also called the G clef. It grew out of a letter G, and its curl wraps round the second line from the bottom. That line is G4, the G just above middle C. High instruments and voices read it: the piano's right hand, singers, guitar, violin, flute, trumpet.

The bass clef is the F clef. It grew out of a letter F, and its two dots sit either side of the second line from the top. That line is F3, the F just below middle C. Low instruments read it: the piano's left hand, bass guitar, cello, trombone, and low voices.

G4 is seven semitones (a fifth) above middle C, and F3 seven semitones below it, so the two clefs sit either side of middle C. The number after a note counts octaves from C. Middle C is C4, MIDI note 60 in DawIt (see *What is MIDI?*), and every note from C4 up to the B above it has a 4.

You may meet a treble clef with a small 8 under it: the music sounds an octave lower than written. Guitar music is written an octave higher than it sounds, so it fits on the treble staff; bass guitar music is written an octave higher in the bass clef. Many printed parts leave the 8 off, because guitarists and bassists know.

Other clefs exist (the viola's alto clef puts middle C on the middle line), but almost all the music you'll meet uses treble and bass.

**3. Notes in the treble clef** — *E G B D F on the lines, F A C E in the spaces*

From the bottom line up, the five lines of the treble staff are E G B D F: E4, G4, B4, D5 and F5. A sentence helps you remember them: Every Good Boy Deserves Football (or Fudge; there are many versions).

The four spaces, from the bottom up, are F A C E: F4, A4, C5 and E5. They spell FACE.

Mnemonics are a start, but fluent readers don't spell out a sentence for every note. They know a few landmark notes by heart and count steps from them:
  G4, the line the clef curls round;
  middle C (C4), on one ledger line below the staff;
  C5, in the third space, an octave above middle C;
  F5, the top line.
A note one step higher is the next letter; a note on the next line up skips a letter.

Try it: the second space is A4, the A that orchestras tune to (440 Hz). Two spaces higher is E5.

In the picture, the lines and spaces are named and the keyboard below lights each note. Click one to hear it.

**4. Notes in the bass clef** — *G B D F A on the lines, A C E G in the spaces*

From the bottom line up, the five lines of the bass staff are G B D F A: G2, B2, D3, F3 and A3. A sentence to remember them: Good Boys Deserve Fudge Always.

The four spaces, from the bottom up, are A C E G: A2, C3, E3 and G3. All Cows Eat Grass.

The bass clef is not the treble clef moved down: the same line is a different letter. The treble staff's bottom line is E, the bass staff's is G. That's why people who read only treble at first misread the bass clef by two letters.

Landmarks to read from: F3, the line between the clef's dots; middle C (C4), on one ledger line above the staff; and C3, in the second space, an octave below middle C.

A bass guitar's open strings sound E1, A1, D2 and G2. They are written an octave higher, as E2, A2, D3 and G3, so they sit on and just below the staff instead of far under it.

**5. Ledger lines and middle C** — *Short extra lines above and below the staff*

When a note is too high or too low for the staff, short lines are drawn just for it: ledger lines. They work like the staff's own lines, so the letters carry on in order.

Below the treble staff: D4 hangs just under the bottom line, middle C (C4) sits on one ledger line, B3 hangs under that, and A3 sits on two ledger lines.
Above the bass staff: B3 sits on the top line, middle C (C4) on one ledger line, D4 on top of it, and E4 on two ledger lines.

So middle C is written in both clefs, on one ledger line: below the treble staff and above the bass staff. It's the same key on the piano, the C nearest the middle of the keyboard.

Ledger lines are easy to miscount. Learn two more landmarks: two ledger lines above the treble staff is C6, and two below the bass staff is C2.

A part that keeps needing three or more ledger lines is usually written in the other clef, or with 8va over it (play an octave higher than written). DawIt picks the clef that needs the fewest ledger lines; it doesn't write 8va yet.

**6. The grand staff and the piano** — *Two staves joined: right hand above, left below*

Piano music needs high and low notes at once, so it uses two staves joined by a brace, the curly bracket at the start: the treble staff on top and the bass staff below. Together they are the grand staff (the great stave in Britain).

Usually the top staff is for the right hand and the bottom staff for the left. That's a habit, not a law: pianists play what's written with whichever hand suits, and composers move notes between the staves.

Middle C sits between the two staves, on one ledger line under the treble staff or one above the bass staff. It's the same note either way; where it's written usually says which hand plays it.

Bar lines run through both staves, and notes lined up above each other are played together: a chord in the left hand and a melody note in the right hand at the same place sound at the same moment.

When DawIt writes a part on a grand staff, it puts every note from middle C up in the treble staff and every note below in the bass staff. Real piano music splits by hand, not at a fixed note, so a melody that dips below middle C may jump between staves.

In the picture, the notes on the grand staff light up on the keyboard below.

**7. Sharps, flats and naturals** — *Up or down a semitone, until the bar line*

A sharp (♯) raises a note by a semitone, the next key up on the piano: F♯ is the black key just above F. A flat (♭) lowers it by a semitone: B♭ is the black key just below B. A natural (♮) cancels a sharp or flat and takes the note back.

These signs are called accidentals. How to read them:
  They're written before the note but said after it: "F sharp".
  An accidental lasts until the next bar line, for every later note on the same line or space in that bar. After the bar line it's gone.
  It only counts for that line or space, not the same letter in another octave. Many publishers print it again in the other octave anyway, to be clear.
  A note tied over the bar line keeps its accidental; a new note in the next bar doesn't.

One key can have two names. The black key between F and G is F♯ and also G♭. Which is written depends on the key: in D major it's F♯, because a scale uses each letter once (D E F♯ G A B C♯). DawIt picks the spelling from the song's key and the bar's chord.

Rarer: a double sharp (𝄪) raises a note by two semitones and a double flat (♭♭) lowers it by two.

In the picture, the first bar has a sharp that lasts until a natural cancels it. In the second, the bar line has cleared it.

**8. Key signatures** — *The sharps or flats at the start of every line*

Most songs use the seven notes of one key. Instead of writing the same sharps or flats on every note, they're written once at the start of every line, after the clef: the key signature. A sharp on the F line means every F is F♯, in every octave, unless an accidental says otherwise.

Sharps always come in the same order, F C G D A E B, and flats in the opposite order, B E A D G C F. So two sharps are always F♯ and C♯, and three flats are always B♭, E♭ and A♭. (A sentence for the order: Father Charles Goes Down And Ends Battle; read it backwards for flats.)

To find the key:
  With sharps, the major key is a semitone above the last sharp: F♯ and C♯ mean D major.
  With flats, the major key is the second-last flat: B♭, E♭, A♭ mean E♭ major. One flat (B♭) is F major.
  No sharps or flats is C major.

Each key signature also fits one minor key, the relative minor, three semitones below the major: D major's signature is also B minor's, and C major's (none) is A minor's. The music tells you which. A song in A minor usually starts and ends on A, and often has G♯ written as an accidental: the raised seventh, which the key signature leaves out.

Your song is in {key}: {signature}. In the score, the key signature is on every line; the time signature only on the first. The circle of fifths in the chord guide shows all twelve keys and their signatures.

**9. Time signature, bars and tempo** — *4/4, bar lines, bar numbers and ♩ = 120*

After the clef and key signature comes the time signature: two numbers stacked like a fraction. 4/4 means four beats in a bar, each a quarter note; it's sometimes written as a C, for common time. The rhythm guide explains other time signatures; DawIt works in 4/4.

Bar lines divide the music into bars (measures in America). In 4/4 every bar adds up to exactly four beats, notes and rests together. A double bar line, thin then thick, marks the end.

Bar numbers are printed small above the staff so musicians can say "from bar 17". In DawIt's score they match the ruler.

The tempo mark ♩ = {bpm} at the top means {bpm} quarter notes a minute: the same as {bpm} BPM. Older music often uses Italian words instead, such as Andante (at a walking pace) or Allegro (fast).

Section names such as Verse and Chorus are printed in boxes above the bar where they start. They're rehearsal marks: places a band can start from.

In the picture: the start of a line with your song's tempo, two bars and the end.

**10. Note values on the staff** — *Stems, flags, beams, dots, rests and triplets*

The rhythm guide says how long each note value lasts. On the staff, you read the length from the note's shape and the pitch from where its head sits.

Stems: notes below the middle line have their stem going up, on the right of the head; notes on or above the middle line have it going down, on the left. It changes nothing in the sound: it keeps the stems inside the staff so they don't collide with other lines. A stem is about an octave long.

Flags and beams: an eighth note has one flag and a sixteenth two. Eighths and sixteenths in the same beat have their flags joined into beams, so you can see each beat as a group: four sixteenths under a double beam make one beat. DawIt beams by the beat. Some printed music beams four eighths across beats 1 and 2 or 3 and 4, but beams never cross the middle of a 4/4 bar.

Dots: a dot after the head makes the note half as long again. A dotted quarter is 1.5 beats.

Rests: a whole rest hangs from the fourth line and a half rest sits on the middle line, like a hat. Quarter, eighth and sixteenth rests have shapes of their own. A whole rest also stands for a whole empty bar.

Triplets: three notes with a 3 over them fill the time of two. Three eighth-note triplets fill one beat.

In the picture: a half note, a quarter and two beamed eighths in the first bar; a dotted quarter, an eighth, a triplet and a quarter rest in the second. Press Play to hear it.

**11. Ties and the beat** — *Why one MIDI note can be written as two*

A tie is a curve joining two notes of the same pitch: play the first and hold it through the second, without playing again. A slur looks similar but joins different notes, and means "play them smoothly". DawIt writes ties but not slurs, because MIDI doesn't record phrasing.

A note that crosses a bar line needs a tie, because a 4/4 bar holds four beats and no more. A note that starts on beat 4 and lasts two beats is written as a quarter tied to a quarter in the next bar.

Ties are also used inside a bar, so you can see where the beats are:
  A note that starts between beats and lasts past the next beat is split at that beat. An eighth note on the "and" of 1 that lasts a whole beat is written as two tied eighths, so beat 2 is visible.
  In 4/4 the middle of the bar, beat 3, stays visible. A half note on beat 2 is written as two tied quarters. Only notes that start on beat 1 (a half, a dotted half or a whole note) hold through beat 3 without a tie.

This is a convention, not a law. Many writers use short forms for common syncopations (eighth, quarter, eighth), and some allow a half note on beat 2. DawIt writes the strict form, which is the easiest to count when you're learning.

In the picture, the top shows the MIDI notes as the piano roll does; below is how the score writes the same rhythm, with ties at the beat and over the bar line.

**12. Chord symbols above the staff** — *Am over the bar: what a band plays from*

Chord names above the staff, such as Am, F, C or G7, tell a guitarist or pianist which chord to play from that point until the next one. They're the same names as on DawIt's chord track: a letter for the root, m for minor, 7 for a seventh (the chord guide explains them).

A page with a melody and chord symbols, often with the lyrics under the notes, is a lead sheet. It's how most pop and jazz songs are written down for a band: the singer reads the melody, and the others make their parts from the chords.

Chord symbols are spelled in the song's key, like the notes: B♭ in F major, A♯m in F♯ major. DawIt takes them from the chord track and shows a symbol where the chord changes and at the start of each section, not on every bar.

Melody notes that belong to the chord sound settled; others add colour or lead to the next note. Hover a note in the score to see whether it's in the bar's chord.

**13. How DawIt writes your MIDI** — *Rounded for reading; the MIDI stays as played*

MIDI records when each key went down and came up, very precisely. Nobody plays exactly on the beat, so a note played "on beat 2" may start 0.03 beats late and last 0.94 beats. Written exactly, that would need tiny rests and odd ties nobody could read. So the score rounds, the way a musician writing a part down by ear would:
  Each beat is read either as sixteenths or as eighth-note triplets, whichever fits its notes better, and every start and end moves to the nearest of those points.
  Notes that start together on one staff become a chord. The score writes one line of notes per staff, so a note still held when the next one starts is written up to that next note.
  Long notes are split with ties at bar lines and beats (see *Ties and the beat*).
  Sharps and flats come from the song's key and the bar's chord: G♯ in A minor, not A♭.
  The clef is the one that needs the fewest ledger lines: treble when the lowest note is F3 or higher, bass when the highest is G4 or lower, and a grand staff, split at middle C, when the part needs both.

The rounding is only for the score. The MIDI isn't changed and plays exactly as recorded. Hover a note to see how far it moved.

DawIt doesn't write everything a printed part has yet. Swung eighths are written as triplets (a quarter and an eighth in each beat), and velocity isn't written as dynamics such as p (soft) and f (loud).

In the picture: a rhythm as it was played, slightly off the grid, and how the score writes it.

**14. Your part** — *The selected track, written out and explained*: built by `analysePart` (rules below).

### Tooltips and texts elsewhere
- **Clef menu:**
  - Auto: "Picks the clef that needs the fewest ledger lines: treble, bass, or both for a wide part";
  - Grand staff: "Treble and bass staves joined, as piano music is written; split at middle C";
  - Guitar: "Treble clef, written an octave higher than it sounds, as guitar music is";
  - Bass guitar: "Bass clef, written an octave higher than it sounds, as bass guitar music is".
- **Sheet music (PDF)...:** "Save this part as sheet music to print or read on a tablet".
- **Score toggle:** "Show the part as sheet music: notes, rests and ties on a staff. Read-only: the MIDI isn't changed".

### "Your part" rules (`analysePart`)
- **Always:** "{track} has {n} notes in bars {first}–{last}." Then "It's written in the {clef} because ...":
  - treble: "its lowest note, {low}, is F3 or higher";
  - bass: "its highest note, {high}, is G4 or lower";
  - grand staff: "it runs from {low} to {high}, too wide for one staff: {t} notes from middle C up are in the treble staff and {b} in the bass staff";
  - chosen: "you chose it in the Clef menu".
- **Ledger lines:** when any note needs them, "The highest note, {high}, needs {k} ledger lines; the lowest, {low}, needs {m}." (Leave out a half that is 0.)
- **Key:** "The key is {key}, with {signature} at the start of every line."
  - When {a} > 0: "{a} of the {n} notes are outside the key, so they carry a sharp, flat or natural." In a minor key, when the raised 7th occurs: "{g} of them are {raised 7th}, the raised seventh that minor keys borrow to lead home."
  - When {a} = 0: "Every note is in the key, so the score needs no accidentals."
- **Rounding:** "{moved} notes start more than a quarter of a sixteenth (0.0625 beats) away from where they're written; the most is {max} beats, in bar {bar}." When {moved} = 0: "Every note starts on the grid, so the score shows exactly what plays." When there are triplet beats: "{tb} beats are written as triplets."
- **Held notes:** when {h} > 0: "{h} notes are still held when the next one starts; the score writes them up to that next note."
- **Advice** (each only when its condition holds, otherwise quiet):
  - more than 25% of notes moved and n ≥ 8: "This part was played freely, so the score had to round a lot. That's fine for listening. For a cleaner score, put the notes on the grid in the piano roll with Snap at 1/16, but listen first: the feel changes too."
  - Auto bass clef, the lowest note needs ≥ 4 ledger lines, below E2 (40), and the track name contains "bass": "The lowest notes need {m} ledger lines. Bass guitar parts are written an octave higher than they sound: choose Clef > Bass guitar in the right-click menu to read it as bass players do."
  - Auto grand staff, lowest ≥ E2 (40), and the name contains "guitar" or "gtr": "Guitar music is written on one treble staff, an octave higher than it sounds: choose Clef > Guitar to read it that way."
  - treble, highest note needs ≥ 4 ledger lines: "The highest notes need {k} ledger lines. Printed music would use 8va (play an octave higher) here; DawIt doesn't write it yet, so count the ledger lines in pairs."
- **Empty:** no MIDI track: "Select a MIDI track to see its part written out here." No notes: "{track} has no notes yet. Draw some in the piano roll and they'll appear here."

### Test numbers
| Check | Expected |
|---|---|
| Spell 66 in D major / 68 in A minor, no chord / 61 in D minor / 70 in D minor | F♯4 / G♯4 / C♯4 / B♭4 |
| Spell 70 in C major, no chord / 66 in C major with chord D / 66 in C major, no chord | B♭4 / F♯4 / G♭4 |
| Spell 65 in F♯ major | E♯4, treble step 0 |
| Treble steps of 64, 71, 77, 60 / bass steps of 43, 50, 57, 60 | 0, 4, 8, −2 / 0, 4, 8, 10 |
| Ledger lines: treble 60, 57, 53, 84 / bass 64, 40, 28 | 1, 2, 3, 2 / 2, 1, 4 |
| Auto clef {57..72} / {36..60} / {48..76} / {55,60,62} / {53,55,57} / none | treble / bass / grand / treble / bass / treble |
| D major treble sharp steps / B♭ major treble flat steps / D major bass sharp steps | 8, 5 / 4, 7 / 6, 3 |
| Split 1–3 / 0–3 / 0.5–1.5 / 3–5 / 0–1.75 / 2–4 / 0–4 | ¼ tie ¼ / dotted ½ / ⅛ tie ⅛ / ¼ tie ¼ over the bar line / dotted ¼ tie 1/16 / ½ / whole |
| Rests: only 0–0.5 / empty bar / notes 0–1 and 2.5–3 | ⅛ rest at 0.5, ¼ at 1, ½ at 2 / one whole rest / ¼ rest at 1, ⅛ rest at 2, ¼ rest at 3 |
| Triplets: 0, ⅓, ⅔ (⅓ each) / 0 (⅔) and ⅔ (⅓) / 0, 0.25, 0.5, 0.75 | 3 eighth-triplets, one bracket / quarter-triplet + eighth-triplet / sixteenths |
| The "How DawIt writes your MIDI" example | values ⅛ ⅛ ¼ ⅛ ⅛⌒¼; 4.0 beats; beat 1 errors 0.15 vs 0.42 (± 0.005) |
| Accidentals example | bar 1: none, ♯, none, ♮; bar 2: ♭, ♮, none |
| Stems: 67 / 74 / 71 / group 72+71 / group 64+74 | up / down / down / down / up |
| Beams: 4 sixteenths in a beat / 4 eighths over 2 beats / eighth + eighth rest | 1 group / 2 groups / flag, no beam |
| 200 seeded random parts | every bar of every staff = 4.0 beats (± 1e-9); ties join equal pitches; x grows with the beat |
| Line breaks for an A4 width | no bar split; every line ≤ width |
| Tempo marks for 120 → 121 → 125 | "♩ = 120" at bar 1, one more at the 125 change |
| 0.0625 beats at 120 BPM | 31.25 ms |
| Save and load | scoreClef bass guitar kept; Auto not written; a missing key → Auto |
| Guide | 14 topics, each with text and an example score; search finds it for "clef", "key signature", "sheet music", "ledger line" |
| Your part | a "Bass" part E1–G2 → the bass guitar advice; 10 notes, 4 moved > 0.0625 → the freely-played advice; all in key → "no accidentals" |

## Steps
**Group A1, engine: notation** (`./build.sh --test` at step 3)
- [x] 1. `src/generate/Notation.{h,cpp}` (`daw::gen::notation`): the Score types, spelling (R4), staff steps and ledger lines (R1, R2), auto clef (R3), key-signature positions (R5). Add it to both CMake targets. Tests: the spelling, step, ledger, clef and key-signature rows. Done when they pass.
- [x] 2. Rhythm: the display grid (R7), one voice (R8), splitting and ties (R9), rests (R10), triplets, accidentals shown (R6). Tests: the split, rest, triplet, accidental and example rows, and the random-parts check.
- [x] 3. Beams, stems (R11, R12), tempo marks and chord and section symbols (R13, R14), and engraving: x positions in staff spaces and line breaks for a width. Tests: the stem, beam, tempo and line-break rows. `./build.sh --test` passes.

**Group A2, engine: data and guide content** (`./build.sh --test` at step 6)
- [x] 4. The track's score clef: an engine setter, saving in `ProjectFile.cpp`. Tests: the save-and-load row.
- [x] 5. `src/generate/ReadingGuide.{h,cpp}` (`daw::gen::readmusic`): topics, the texts from *Theory*, the example scores, `analysePart`. The rhythm guide's one added sentence. Tests: the guide and *Your part* rows.
- [x] 6. `gen::guides`: a new `Area` *Reading music* (name, summary) and the entry. `runTheoryGuidesTests` search checks. `./build.sh --test` passes.

**Group B1, UI: the score view** (`./build.sh --test` at step 9)
- [x] 7. Bravura: pinned download, binary data, licence file and `ScorePainter` (`src/ui`). Check early that the Private Use Area glyphs render: draw a treble clef in the score's empty state and capture it with a `panel: clipEditor` shot on a track with no notes.
- [x] 8. `ScoreView` and the header switch in `PianoRoll`, `view.clipEditorMode`, View > Score (Cmd+8), zoom, the playhead, follow while playing, the sounding-note highlight.
- [x] 9. Interaction: click to hear or move the playhead, double-click to the piano roll, tooltips, the right-click menu (the clef through the engine setter), empty states. Add `"clipEditorMode": "score"` to `ScreenshotRunner` as a shot key. `./build.sh --test` passes.

**Group B2, UI: the guide and the PDF** (`./build.sh --test` at step 12)
- [x] 10. `ReadingExplainer` (the lyrics guide's layout): pictures through `ScorePainter` plus `keyboardpicture` for topics 3, 4 and 6, click to hear, Play for topics 10, 11 and 13, PDF export (`guideprint::Source`). Wire up `openGuide`, `getPrintGuide`, the score's buttons and the area colour.
- [x] 11. `SheetMusicExport`: the dialog, the File menu command, the header button, the pages at 300 dpi.
- [x] 12. `--export-guides` passes with the new guide. `./build.sh --test` passes.

**Group C, check and docs**
- [x] 13. Real-app check (pop song), each shot with `"clipEditorMode": "score"`:
  - `score-keys.png`: select Keys, `panel: clipEditor`, `capture: bottomPanel`. A grand staff, A minor (no key signature), chord symbols, bar numbers matching the ruler.
  - `score-bass.png`: Bass. The bass clef.
  - `score-lead.png`: Lead vocal at beat 32 with `play`. Treble, the playhead and lit notes.
  - The guide at *Notes in the treble clef*, *Ties and the beat* and *Your part* (Keys selected).
  - Export the Keys PDF by hand and look at page 1: header, systems, foot.

  Look for clipped glyphs, collisions of chord symbols with ledger lines, and wrong bar numbers.
- [x] 14. Docs:
  - README *Features* bullet (Score switch, Cmd+8, the PDF, the guide), and the *License note* for Bravura.
  - `docs/code-layout.md` (Notation, ReadingGuide, ScorePainter, ScoreView, SheetMusicExport) and `docs/tests.md` (notation, reading guide).
  - `docs/ui-design.md`: the score view and the notation ink colours.
  - `docs/architecture.md`: the clip editor's two modes, and the one painter shared by the panel, guide and PDF.
  - FEATURE_IDEAS: this idea *Partly done* (read-only view and PDF); *Export to PDF* gets "sheet music for one track" done; *Reading music and ear training* is partly done (the reading guide); *Learn to read notes* gets a status note that the staff drawing is built.

## Tests
See *Test numbers* above, in `runNotationTests` (steps 1 to 3), `runReadingGuideTests` (step 5) and the project round trip (step 4).

## Website hand-off
- **Features page:** a "Score view and sheet music" section: the switch, read-only rounding explained, the PDF, the guide.
- **Main page:** the guide in the guide picker under a new *Reading music* area chip.
- **Screenshots** (shot list keys: `clipEditorMode`, `scoreClef`, `sheetMusic`):
  - the Keys grand staff in the bottom panel under the arrangement;
  - the guide at *Notes in the treble clef*;
  - page 1 of the Keys PDF.
- **How it's made:** Bravura (OFL 1.1, the SMuFL reference font): what it draws, its version and a link.

## Left for later
- Editing in the score, step input, and MusicXML export (the user's scope).
- Two voices per staff; splitting a grand staff by hand.
- 8va and 8vb lines; courtesy accidentals.
- Spelling by melodic direction; dynamics from velocity.
- A swing marking instead of triplets; quarter-note triplets across two beats.
- Several parts or a full score in the PDF; repeats; transposing instruments; drum notation (the drum tracks idea).
- Lyrics under the notes.
- The reading-notes exercises, which reuse `gen::notation` and `ScorePainter`.

## Changes while building
- **Group A1 hand-off (steps 1 to 3 built, 44 checks in `runNotationTests`).** `src/generate/Notation.{h,cpp}`, namespace `daw::gen::notation`, in both CMake targets.
  - Entry point: `notation::build (const Input&)` with `Input { notes (MidiNote, beats from bar 0), key, ChordMap chords, sections, TempoMap tempo, ClefChoice clef }` returns a `Score` (`staves` -> `bars` -> `events`, plus `barWidths`, `tempoMarks`, `chordSymbols`, `sectionMarks`, `clefUsed`, `keyPosition`). Rhythm is in units of 1/12 beat (`unitsPerBeat` 12, `unitsPerBar` 48); `Event::start` and `length` are units, `getBeats()` converts. `Event::sourceNotes` indexes `Input::notes` (for tooltips and the "moved" counts in `analysePart`).
  - Also public: `spell`, `getStaffStep`, `getLedgerLines`, `chooseAutoClef`, `getKeySignature (position, clef)`, `fitBeat (points)` (the R7 errors), `getHeaderWidth`, `layoutLines (score, width, stretchLast)` returning `Line`s, and `getEventX (line, barInLine, event)` (staff spaces from the line's left edge).
  - Chords come from the per-bar `ChordMap`: a chord applies to its own bar only, a symbol is shown where it differs from the previous bar's.
  - Deviations: (1) the accidentals example needs a D chord in bar 1 to spell 66 as F#, since the table says 66 in C major without a chord is G-flat; the test supplies it. (2) A written piece may not end on a sixteenth point inside a beat whose note ends on a triplet point (otherwise a 2-unit remainder appears): a small addition to R9. (3) Engraving constants (spacing `2.2 + 3.2 * sqrt (beats)`, accidental 1.4, dot 0.8, header 3.6 + 1.0 per sign + 0.8 + 3.0 for the time signature on line 1) are first guesses for `ScorePainter` to tune. (4) Chord heads that touch (seconds) are not displaced yet: left to the painter.
- **Group A2 hand-off (steps 4 to 6 built; all tests pass).**
  - Clef: `enum class ScoreClef` (same order as `notation::ClefChoice`) and `Track::scoreClef` in `src/engine/Track.h`; `Engine::setScoreClef (track, clef)`; saved as `"scoreClef"` (ids auto, treble, bass, grand, guitar, bassGuitar; Auto not written; MIDI tracks only). The UI casts `(notation::ClefChoice) (int) track.scoreClef`.
  - `src/generate/ReadingGuide.{h,cpp}` (`gen::readmusic`): `Topic`/`allTopics`/`getTopicName`/`getTopicSummary`; `getExample (Topic, const Part&)` returns `Example { panels, playNotes }`, each `Panel` having `label`, a built `notation::Score`, `accentNotes`, `keyboardNotes`, `showNoteNames`, `played` (`MidiBlock`s for the ties and rounding pictures), and `firstBar`/`numBars` (-1: all; the "Your part" panel shows 4 bars round the playhead); `explain (Topic, Part)` (title line, blank line, text); `analysePart (Part)` returns a `PartReport` (numbers and `text`). `Part { hasTrack, trackName, input (notes, key, chords, sections, tempo, clef), playheadBeat, autoClef }`: the UI fills it from the selected MIDI track, with `autoClef` true when `Track::scoreClef` is Auto.
  - `gen::guides`: `Area::readingMusic` (after Rhythm, colour 0xffe8d36a in `TheoryGuidesView::getColour`) and `Guide::readMusic`. `MainComponent::openGuide`, `getPrintGuide` and `ReadingExplainer` are still to do (Group B2). `--export-guides` and `DevScreenshots` loop over `allGuides`, so they need the guide wired before they run cleanly.
  - Deviations: (1) the rhythm guide's added sentence sits at the end of the paragraph on beams ("... instead of flags. The guide How to read music shows them on the staff."), not after the Play line. (2) Grand-staff picture: the plan's landmark F3 is not among its six notes, so it is only listed in `accentNotes`. (3) Texts say "see Sharps, flats and naturals" etc. without italics, as plain text.
- **Group B1 hand-off (steps 7 to 9 built; `./build.sh --test` passes, no warnings).**
  - Bravura: `BRAVURA_COMMIT` 37b194378b710cc40e406ab6c4b07608bb9548ae of steinbergmedia/bravura, `redist/otf/Bravura.otf`, 889,228 bytes, SHA-256 cdf0f893ee1fdb64b7f6713d71ee0dcfc349c0ac01429a8e451b01a9e79f5f3b; `LICENSE.txt` (4,420 bytes) SHA-256 bdca252186865f3b2c0e76da4a4e335c14ba1d37f80194fca254ddfa88e84fbf. Both pinned with `file (DOWNLOAD ... EXPECTED_HASH)` in `CMakeLists.txt`; the font is binary data target `DawItFonts` (`dawfonts::Bravura_otf`), the licence is copied to the bundle's `Contents/Resources/Bravura-LICENSE.txt` (next to the executable elsewhere) by a POST_BUILD command on `DawIt`. The README *License note* and *How it's made* are still to do (Group C).
  - `src/ui/ScorePainter.{h,cpp}` (`daw::scorepaint`): `drawSystem (g, score, line, x, yTop, space, ink, options, hits*, lit)` draws one `notation::Line` (staves, brace, bar lines, header, notes, rests, ties, beams, triplet brackets, bar numbers, chord symbols, section boxes, tempo marks); `drawHeader`, `drawClef`, `describeEvent`, `getTopMargin`, `getBottomMargin`, `getStaffBlockHeight`, `Ink` (light-on-dark by default; B2 passes black on white). `Options::firstBar/lastBar` draw only some bars, `Options::header` is off for the scrolling score. `Hit` lists what was drawn (notes, rests, clef, key and time signature, chord symbols, triplet brackets) for tooltips and clicks.
  - `src/ui/ScoreView.{h,cpp}`; `PianoRoll` owns it (`scoreView`), with `setScoreMode`, `isScoreMode`, `openFromScore`, and the callbacks `onShowReadingGuide`, `onExportSheetMusic` (the PDF button and the guide button are disabled until B2 sets them; `MainComponent` sets only `onModeChanged`). View > Score (Cmd+8, `showScoreCommand`) is in; Clip Editor (Cmd+1) now means the piano roll. `ScreenshotRunner` has the shot key `"clipEditorMode": "score" | "pianoRoll"`.
  - Deviations: (1) note heads, the augmentation dot and the tuplet 3 are Bravura glyphs, not paths (the SMuFL stem anchors then fit exactly); stems, ledger lines, beams, ties, bar lines and the brace are paths. (2) The score is one `Line` from `layoutLines (score, 1e9)`; its first line's header (with time signature) is drawn pinned. (3) `MainComponent::openGuide`, `getPrintGuide`, `ArrangementView::getPrintGuide` and `showGuide` and `ScreenshotRunner::getGuideKey` got an empty `case G::readMusic:` (or its key) so the build has no -Wswitch warning; B2 fills them. (4) The clef menu items have no per-item tooltips (JUCE's PopupMenu has none); the Clef button's tooltip says what Auto does. (5) The brace is a hand-drawn path, not the glyph, so it scales to any height; look at it again in the Group C shots.
  - Checked in the real app: Keys (treble, whole-note chords, Intro/Verse/Chorus boxes, tempo, chord symbols, bar numbers), Bass (bass clef, whole rests, ties, beams), Lead vocal playing at beat 32 (triplet bracket, a lit note, the playhead, follow-scroll), and the two empty states (the clef glyph renders). Not seen: a grand staff (no example track needs one) and the PDF.
- **Group B2 hand-off (steps 10 to 12 built; `./build.sh --test` passes, no warnings; `--export-guides` passes, "How to read music: 14 topics, 15 pages").**
  - `src/ui/ReadingExplainer.{h,cpp}` (`ReadingExplainer (Engine&, Selection&)`, `showTopic`, `guideprint::Source`); `MainComponent::showReadingGuide`, `readingGuide`, and the `readMusic` cases of `openGuide` and `getPrintGuide` (removed from `ArrangementView`, where it now sits with the "other guides"). `PianoRoll::onShowReadingGuide` and `onExportSheetMusic` are set in `MainComponent`; the export dialog has a *How to read music* button too.
  - `src/ui/SheetMusicExport.{h,cpp}` (`daw::sheetmusic`): `getPart (Engine&, Track&, playheadBeat)` (also used by the guide), `renderPages`, `exportPdf`, `showDialog (engine, track, projectName, onShowGuide)`, `Paper`, `Header`. File > Export Sheet Music (PDF)... (`exportSheetMusicCommand`, after the chord sheet; active only with a MIDI track selected). A4 (default) or Letter in `export.sheetPaper`; file name "<song> - <track>.pdf"; 300 dpi, staff space 5.6 pt.
  - `ScorePainter` gained `getLineTopMargin (score, line, space)` (tight room over a line: 9.4, 6.6 or 4.6 spaces by what it draws), and the brace is now a proper curly path (the B1 one rendered as a small arrowhead).
  - `ScreenshotRunner` shot key `"sheetMusic": "a4" | "letter"` writes the selected MIDI track's PDF and `<file stem>-page<n>.png` next to the shot (give it `"area": [0,0,10,10]`). Group C should add it to the recipe.
  - Deviations: (1) the keyboard under the notes is `KeyboardDiagram` (the chord guide's piano), not `keyboardpicture` (that is the computer-keyboard keys). (2) The pictures drop the tempo mark except in *Time signature, bars and tempo* and *Your part* (it crowded every example). (3) Three or more panels are laid out in three columns; the staff shrinks (to 60%) when the pictures would not fit the height. (4) The *How DawIt writes your MIDI* picture has no move arrows: a thin line from each played block down to the staff shows where it starts, and the block is placed through the written events' x so the offset can be seen. (5) The PDF page 1 says "Tempo mark = bpm   Key: ..." as plain text under the part name; the clef, key and bar number start every line. (6) No automated test for the dialog or pages (UI); checked in the real app.
  - Checked in the real app (shots in the scratchpad): the guide at Notes in the treble clef (keyboard lit under named notes), Ties and the beat (blocks over the score), Key signatures (6 pictures), Treble and bass clefs, The grand staff (brace, both staves named), How DawIt writes your MIDI, and Your part (Keys); page 1 of the Keys PDF (title, part, tempo and key, 5 justified systems with chord symbols and section boxes, final bar line, foot). Not looked at: Letter paper, a multi-page PDF, Play running, the dialog window itself.

- **Group C (steps 13, 14 built).** Checked in the real app: Keys as a grand staff (forced with the new shot key `"scoreClef": "grand"`; no example track needs one by range), Lead vocal playing at beat 32, Letter PDF pages 1 and 2 (3 systems on page 1, 3 on page 2, bars 25 to 36, "Page 2 of 2", final bar line), the A4 Bass PDF (2 pages), the guide at Treble and bass clefs and at Your part. Bar numbers match the ruler; no clipped glyphs or chord symbol collisions seen. Added to `ScreenshotRunner`: `"scoreClef"`; all three new keys (`clipEditorMode`, `scoreClef`, `sheetMusic`) are in `DevScreenshots.h` and `docs/recipes.md`.
  - **Known gap:** in the scrolling panel the grand staff's brace is not visible (it is drawn left of x = 0, off the view); the PDF and the guide's pictures show it. Fixing it needs a left margin in `ScoreView`'s coordinates. The export dialog window itself cannot be reached by a shot key; the PDF it writes was checked.
  - Docs done: README bullet and License note (Bravura), code-layout, tests, ui-design, architecture, recipes; four ideas set to Partly done.
- **Fix: the brace in the score panel.** `ScoreView` now keeps a left margin of 2.2 staff spaces (`getMargin()`, `getHeaderEnd()`): the system, the pinned header, bar positions (`getBarX`, `beatToX`, `xToBeat`), the hit boxes, the header hit area, the clip scroll and the follow-while-playing limit all start after it, so a grand staff's brace shows. Checked with `"scoreClef": "grand"` on Keys (brace and joined bar line visible) and with Bass (single staff, playing, header and playhead right).
- **Theory check.** Text and number literals only; `./build.sh --test` passes, no warnings.
  - Bass-line mnemonic: "Good Boys Do Fine Always (or Deserve Fudge Always)"; the Do Fine form is the more common one (music-theory-practice.com, barrypopik.com). Treble: "Football (or Fudge, or Does Fine ...)". Father Charles Goes Down And Ends Battle confirmed (MacEwan open book).
  - Ledger lines: "B3 sits on the top line" was wrong (A3 is the bass staff's top line, and topic 1 says "on a line" means the line runs through it): now "B3 sits just above the top line ... D4 just above that".
  - Softened overclaims: beams "don't usually cross" the middle of a 4/4 bar (was "never"); a lead sheet is "a common way" to write a pop or jazz song (was "how most"); "A song in A minor often ends on A" (was "usually starts and ends"); swung eighths are "usually" written as triplets (lighter swing rounds to sixteenths under R7).
  - Tooltips: clef menu "Guitar: treble, written an octave up" / "Bass guitar: bass, written an octave up"; paper box "US Letter mainly in the United States and Canada".
  - Open (needs code): for a key at circle position 6 (F♯ major, D♯ minor) `guide::describeKeySignature` says "6 sharps or 6 flats", so *Key signatures* and *Your part* say that while the score draws 6 sharps. Use the score's own signature (`notation::getKeySignature`) for the count in `ReadingGuide.cpp`.
  - SOURCES.md: the mnemonic searches and pages added under *Reading music*.
- **Fix (key signature wording).** `ReadingGuide.cpp` no longer uses `guide::describeKeySignature`, which says "6 sharps or 6 flats" at the top of the circle. A local `describeSignature` counts the marks from `notation::getKeySignature`, so the Key signatures topic and Your part say "6 sharps" for F# major / D# minor, as the score draws. Test in `runReadingGuideTests`.
