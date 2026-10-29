# A singing techniques guide

Status: Built, not committed. Theory checked
Idea: [A singing techniques guide](../FEATURE_IDEAS.html#a-singing-techniques-guide)

## Goal
A guide for people who sing on their own songs and have never had a lesson: how the voice works, breathing, warming up, voice types and range, registers, singing in tune, vowels, vibrato, loud singing, harmonies, looking after the voice and the microphone. Its *Your voice and your song* topic takes the singer's range (a voice type, or their own lowest and highest notes, found by ear on a keyboard that plays) and checks the song's melody against it: "the chorus goes 2 semitones above your range: try the song in G minor instead of A minor". It's honest about its limits: a start, not a teacher or a doctor.

## Decisions for you
1. **How much of "your voice" goes in this first slice.** Options: (a) the guide, its pictures, and a range set by hand (a voice type, or two notes, found by ear on a clickable keyboard), checked against the song's MIDI melody; (b) (a) plus *Measure my range from this clip*: pitch-track an imported recording of the user singing low and high; (c) (a) plus live measurement from the microphone, and a sung take against the melody in cents. **Recommended:** (a). DawIt opens the audio device with no inputs (`deviceManager.initialise (0, 2, ...)`) and records no audio, so (c) first needs audio input, a microphone permission and recording. A take in cents is the idea *Practise singing*, which needs the same pitch tracker as (b); build (b) with it.
   Answer: (a) by hand only, as recommended.
2. **Where the range is kept.** Options: the app settings (it's your voice, the same for every song), or the project (the singer of this song). **Recommended:** app settings (`singing.lowest`, `singing.highest`), because most users sing their own songs; a per-song singer can come later.
   Answer: app settings, as recommended.
3. **The Theory Guides area.** Options: a new area *Singing*, or under *Writing songs*. **Recommended:** a new *Singing* area ("Your voice: how it works, singing in tune, and recording it"), where *Practise singing* would go too, and the *Sing* path of *Guide paths by goal* starts.
   Answer: a new *Singing* area, as recommended.
4. **Sing along's *Tips...*.** It opens the lyrics guide's *Singing along* today. Options: keep it and add a second button, point it at the new guide, or make it a small menu. **Recommended:** a menu: *Reading the words while you record* (lyrics guide, *Singing along*) and *Singing techniques* (new guide, *Posture and breathing*).
   Answer: a menu, as recommended.
5. **When to see a doctor.** The idea says "hoarseness that lasts more than two weeks". The NHS (laryngitis page) and Mayo Clinic say 2 weeks; the American otolaryngologists' clinical guideline (AAO-HNS, 2018) tells *doctors* to look at the folds after 4 weeks. **Recommended:** the text says 2 weeks, as the NHS does, with its urgent signs, and doesn't mention the 4.
   Answer: "still hoarse after 2 weeks", as recommended.
6. **DawIt's own rules in the range check** (opinion, not a standard): a melody "sits near the top" when 25% or more of its sung time is in the top 3 semitones of the range; when the smallest key change that fits is 5 semitones or more and an octave also fits, it suggests the octave (nothing in the song changes). **Recommended:** as written.
   Answer: as written, as recommended.
7. **Numbers not read in a primary source** (kept, worded carefully; dawTheory should check them): the SM58's "6 to 10 dB at 100 Hz" (found in several copies of Shure's manual, not the current PDF); Larrouy-Maestri's tolerance "half of a quarter tone" read from the abstract (about 25 cents); Prame's vibrato figures from a summary. **Recommended:** keep them.
   Answer: keep them and have dawTheory check them, as recommended.

## What the user does and sees
- **Opens from:** the Theory Guides window (area *Singing*); Sing along's *Tips...* menu; a button *Singing techniques...* under the picture of the lyrics guide's *Singing along* topic; *Singing techniques...* in the track menu (just above the routing guide's item) of a track whose name has a vocal word (vocal, vox, voice, sing, singer, lead, melody, bv, backing, harmony, choir).
- **Window:** title *Singing techniques*, 1000 × 740, ui-design.md's guide window layout, *Export PDF...* in the title row.
- **Voice row** (a 30 px row above the picture, shown on *Voice types and range* and *Your voice and your song*): `Your voice: [Tenor ▾] Lowest [C3 ▾] Highest [C5 ▾]`, and on *Your voice and your song* also `Melody: [Lead vocal ▾]`. The type menu has *Not set*, the six types and *Custom*; picking a type fills both notes with its usual range; changing a note shows *Custom (closest: tenor)*. The note menus run from C2 to C7; the highest stays at least 5 semitones above the lowest. The melody menu lists MIDI tracks; it starts on the selected track if it's a MIDI track with notes, else the first MIDI track with notes whose name has a vocal word, else the first MIDI track with notes that doesn't play drums.
- **Clicks:** a key on any keyboard picture plays that note for 600 ms (`Engine::previewNote`); right-click a key: *Set as my lowest note*, *Set as my highest note*. A click on the warm-up, in-tune or harmony picture plays its notes. While the song plays, the melody note at the playhead lights its key on the *Your song* keyboard.
- **Tooltips:** each combo ("Your voice type: fills in its usual range. Set your own notes if they differ."), each key ("C4 (middle C) · 261.6 Hz · click to hear it, right-click to set it as your lowest or highest"), the *Your song* bars ("Chorus · B4 to C4 · 2 semitones above your range").
- **Remembered:** `singing.lowest`, `singing.highest` (MIDI note numbers; missing means not set). The melody track isn't remembered.

## Data and saving
None in the project. The range is two app settings (decision 2).

## The guide
New guide `gen::singing`, 13 topics in reading order: How the voice works, Posture and breathing, Warming up, Voice types and range, Registers and the break, Singing in tune, Vowels and words, Vibrato and straight tone, Loud, soft and belting, Harmonies and doubles, Looking after your voice, The microphone, Your voice and your song. Summaries and texts are in *Theory*, each picture in *Example data*, drawn in the app's visual language (piano-roll notes and grid, keyboards, section colours, `theme::`).

**Theory Guides entry:** area *Singing* (new), name "Singing techniques", summary "How the voice works, breathing, warming up, voice types and your range, registers, singing in tune, vowels, vibrato, belting, harmonies, looking after your voice, the microphone, and your melody against your range.", opens from "Sing along's Tips..., the lyrics guide's Singing along, and a vocal track's menu", keywords "sing singer singing voice vocal vocals vox range tenor soprano alto contralto mezzo baritone bass breath breathing diaphragm warm-up warmup trill pitch tune cents flat sharp vibrato belt belting falsetto register chest head mix break passaggio hoarse hoarseness microphone mic pop filter plosive proximity harmony double key suits", topics from `singing::allTopics`. Not on *Start here*.

## Code
- Recipes: *A theory guide* (copy the lyrics guide), *A command or menu item* (the track menu item through `TrackActions`), *Checking in the real app*.
- **`gen::singing`** (`src/generate/SingingGuide.{h,cpp}`): topics, names, summaries, `explain (Topic, context)` where the context holds the range (optional), the analysis, the key, tempo and lyrics; the voice-type table; the closest type and the types a melody fits; example data for every picture; the default melody-track rule (from names, "has notes" and "plays drums" flags); and the range analysis.
- **The range analysis** takes the melody's top line (`singthis::buildMelody`, so overlapping notes count once), the sections, key, lyrics, tempo and the optional range, and returns the melody's lowest and highest notes with where they are, per-section lowest and highest for the picture, the shift it suggests and the advice list (the *Your song* rules in *Theory*). Pure and tested.
- **Window** `src/ui/SingingExplainer.{h,cpp}`: a `ChangeListener` on the engine (tracks, notes, sections, key and lyrics change the *Your song* text and picture); writes only its two settings and plays previews. It never changes the song.
- **Opening:** the guide belongs to `ArrangementView` (as the lyrics guide: `showGuide`, `getPrintGuide`, a `SafePointer`), so Sing along and the lyrics guide reach it; `MainComponent::openGuide` and `getPrintGuide` route `Guide::singing` there. Adding `Guide::singing` and `Area::singing` means a case in every switch over them (the hub's area colour too: a hue distinct from the other eight).
- **Threading:** nothing new on the audio thread. The analysis copies the melody track's notes under `Engine::getLock()` and runs on the message thread (a song's notes are a few thousand at most).
- **Risks:** small. The new `Area` changes the hub's list; `runTheoryGuidesTests` already checks every area is used.

## Libraries and models
None.

## Theory

### Facts
| # | Fact | Source | Kind |
|---|---|---|---|
| 1 | MIDI 60 is middle C, named C4; A4 = MIDI 69 = 440 Hz; f = 440 · 2^((n − 69) / 12) | ISO 16; MIDI 1.0 | exact |
| 2 | E2 = 82.41 Hz, A3 = 220 Hz, C4 = 261.63 Hz, C6 = 1046.50 Hz | derived from 1 | exact |
| 3 | A semitone is 100 cents, an octave 1200 | definition (Ellis) | exact |
| 4 | The pitch of a sung note is how many times a second the folds open and close | Titze, *Principles of Voice Production* (1994) | exact |
| 5 | Vocal folds about 1.75 to 2.5 cm long in men, 1.25 to 1.75 cm in women | Titze 1994, via Wikipedia *Vocal cords* | typical |
| 6 | Speaking pitch about 90 to 155 Hz in men, 165 to 255 Hz in women | Baken, *Clinical Measurement of Speech and Voice* (2000), via Wikipedia *Voice frequency* | typical |
| 7 | Formants of a man's "ah" about 730 and 1090 Hz, "ee" about 270 and 2290 Hz; women's higher | Peterson & Barney (1952) | typical |
| 8 | Voice ranges: soprano C4–C6, mezzo A3–A5, alto F3–F5, tenor C3–C5, baritone A2–A4, bass E2–E4 | McKinney; Boldrey (bass); via Wikipedia *Voice type*. Lumen and MasterClass agree within 2 semitones; Wikipedia *Vocal range* gives narrower | typical |
| 9 | Each range in that table spans 24 semitones | derived | exact |
| 10 | Baritone is the most common man's voice; most people have medium voices; voice type also depends on tessitura and timbre | Wikipedia *Voice type* (Vennard, Smith) | typical |
| 11 | Breaks (passaggi), Miller: men's upper break C4 (basso profondo) to A4 (light tenor), lyric baritone E4, lyric tenor G4; women's lower break soprano E♭4, mezzo E4, contralto G♯4/A4; upper soprano F♯5, mezzo E5/F5 | Miller's books, via Wikipedia *Passaggio* | typical |
| 12 | "Mixed voice" is described differently by teachers | voice-teaching practice | opinion |
| 13 | The diaphragm contracts and moves down on a breath in; belly and lower ribs move out | anatomy (any physiology text) | exact |
| 14 | Raising the shoulders to breathe takes in less air and adds neck tension | voice-teaching consensus | opinion |
| 15 | A line over 6 s needs a deep breath first | DawIt's rule in `singalong::explainSingingAlong` | opinion |
| 16 | At 96 BPM a beat is 0.625 s: 2 beats 1.25 s, 0.5 beat 0.31 s, 10 beats 6.25 s | derived (60 / 96) | exact |
| 17 | Warm-ups of 5 to 10 minutes are common advice; no study sets the best length (3 to 5 minutes of straw-type exercises in voice patients) | teachers; scoping review, *Audiol Commun Res* | opinion |
| 18 | Humming, lip trills and straws (semi-occluded vocal tract) make the folds vibrate more easily | Titze, *J Speech Lang Hear Res* 49 (2006) 448–459 | typical |
| 19 | Listeners judge a sung note out of tune at about 25 cents ("half a quarter tone"); intervals about 20 cents | Larrouy-Maestri, *Music & Science* (2018); Vurma & Ross (2006) | typical |
| 20 | Two tones beat as many times a second as their difference in Hz | physics | exact |
| 21 | Congenital amusia ("tone deafness") about 1.5% (older estimate 4%) | Peretz & Vuvan, *Eur J Hum Genet* (2017); Kalmus & Fry (1980) | typical |
| 22 | About 10 to 15% sing a semitone or more off, and most of them hear pitch as well as good singers | Pfordresher & Brown, *Music Perception* 25 (2007) 95–115 | typical |
| 23 | Vibrato about 5 to 7 Hz (5.5 to 6.7 Hz in Prame's ten singers), extent about ±60 cents in classical singing, under ±100 for soloists | Prame (1994), via summaries; Wikipedia *Vibrato* | typical |
| 24 | Pop singers often start vibrato late in a note; opera singers through it | Wikipedia *Vibrato* | typical |
| 25 | Louder singing comes from more air pressure under the folds and firmer closure | Titze 1994 | typical |
| 26 | A diatonic third is 3 or 4 semitones: E4–G4 3, G4–B4 4, D4–F4 3, C4–E4 4; a sixth below is a third above an octave lower | interval definitions | exact |
| 27 | Two identical copies add up to 6.02 dB louder, nothing more | derived: 20 log10 2 | exact |
| 28 | What you drink doesn't touch the vocal folds | University of Minnesota Lions Voice Clinic | exact |
| 29 | Hydration works through the body over hours; steam moistens the surface briefly; the evidence is thin | Voice Foundation newsletter (2013) | typical |
| 30 | See a GP if laryngitis hasn't improved after 2 weeks or keeps coming back; 111 if very painful to swallow, 999 if hard to breathe; whispering strains the voice; limit caffeine and alcohol | NHS *Laryngitis*; Mayo Clinic (2 weeks) | exact (as guidance) |
| 31 | Studio vocal distance about 15 to 23 cm (6 to 9 in); a handheld SM58 within about 15 cm | Sound On Sound; Shure SM58 user guide | typical |
| 32 | Twice the distance is 6.02 dB quieter (free field; less in a reflective room) | derived: inverse-square law | exact |
| 33 | Proximity effect: directional mics gain bass close up, the SM58 6 to 10 dB at 100 Hz within about 6 mm; omnidirectional mics don't | Shure SM58 user guide; acoustics | exact (manufacturer) |
| 34 | Pop filter about 5 to 8 cm in front of the mic | Sound On Sound (about 7.5 cm); Soundtrap (2 to 4 in) | typical |
| 35 | Low cut around 80 to 100 Hz on vocals; peaks around −18 to −10 dB coming in | DawIt EQ's Vocal starting point; gain staging guide | typical |
| 36 | Range check thresholds (3 semitones, 25%, octave at 5 or more) | DawIt's rules | opinion |

**Agreement with existing guides:** the table's extremes E2 and C6 are `singthis::lowestMidi` and `highestMidi` ("nearly every singer"); the Singing along tips (stand, breathe low, sing in a key that suits you, an octave lower) agree. **One fix:** DawIt EQ's *Male voice* says "Sung notes from about F2 to C5, speech lower", but men's speech (90 to 155 Hz) sits in the low part of that range, not below it (step 3).

### Topic texts

**1. How the voice works** — *Breath, two small folds, the throat and mouth*

Your voice is an instrument with three parts: a power source, something that vibrates, and a body that shapes the sound. A guitar has your fingers, the strings and its wooden body. Your voice has your breath, the vocal folds and your throat and mouth.

The power is the air from your lungs. The vocal folds (also called vocal cords) are two small bands of muscle and soft tissue in your larynx, the voice box behind your Adam's apple. They're small: roughly 1.7 to 2.5 cm long in adult men and 1.2 to 1.7 cm in adult women. When you sing, they close, the air pushes them apart, and they spring back, over and over. Each time they open, a puff of air goes up into your throat, and the puffs make a buzz.

The number of puffs a second is the pitch. To sing the A below middle C (A3, 220 Hz), your folds open and close 220 times a second; for the A above middle C (A4, 440 Hz), 440 times. When we speak, most men's voices sit around 90 to 155 Hz and most women's around 165 to 255 Hz. Longer, heavier folds vibrate more slowly, which is why men's voices are usually lower. For a higher note, small muscles stretch the folds, like tightening a guitar string.

The buzz on its own sounds thin, like a reed without its instrument. Your throat, mouth and nose are a resonator: they make some of the buzz's harmonics louder and others softer. Their shape, set by your tongue, jaw and lips, is what makes one vowel different from another. In the picture, the same 220 Hz buzz is shaped two ways. For "ah" the strong areas (formants) are both fairly low, around 700 and 1,100 Hz; for "ee" one is very low and one high, around 300 and 2,300 Hz. These are a man's; a woman's are a little higher. Same note, same folds, a different shape: a different vowel.

So singing well is three things working together: a steady flow of air, folds that vibrate freely without squeezing, and a relaxed, open throat and mouth. The next topics take them one at a time. Click the picture to hear the A3.

**2. Posture and breathing** — *Stand tall, breathe low, plan your breaths*

Singing runs on breath, so it starts with how you stand and how you breathe.

Stand (or sit up) tall and loose: feet about hip-width apart, knees not locked, shoulders down and relaxed, chin level. Don't push your chin forward or up to reach a high note. Slumping squeezes the room your lungs need, and stiffness, in the neck and jaw most of all, reaches the folds.

Breathe low. The main breathing muscle is the diaphragm, a dome under your lungs. When it tightens, it flattens and moves down, the lungs fill, and your belly and lower ribs move out. That's why teachers say "breathe into your belly": the air goes into your lungs, but the belly is where you see it. If your shoulders rise when you breathe in, you're breathing high, into the top of your chest, which takes in less air and tightens your neck. Try it: put a hand on your belly and breathe in slowly through your nose. The hand should move out and the shoulders stay still.

Breath support is what singers call letting the air out steadily rather than in one rush. To practise, breathe in low and let the air out on a long "sss", as evenly as you can, without forcing it at the end. With practice it lasts longer and stays even, and a long note needs exactly that.

Plan your breaths. A line is easier when you know where you'll breathe: in the rests between lines, not in the middle of a word. In the picture, at 96 BPM, the first line has a 2-beat rest after it, 1.25 seconds: time for a full breath. The second has half a beat, about 0.3 seconds: a quick, small breath through the mouth. The third runs for 10 beats, about 6.3 seconds, so breathe in deeply before it, or take a quick breath after "head". Mark your breaths on the lyrics and practise them like the words.

Running out of breath often shows as the end of a line going flat or quiet. If that happens, breathe earlier, before you need to. The lyrics guide's Singing along looks at your song's longest line and its shortest rest.

**3. Warming up** — *Hum, lip trill and slide, a few minutes first*

Runners warm up before they run, and singers before they sing: a few minutes of gentle exercises get the breath going and the folds moving freely, so the first take isn't the warm-up. Many teachers suggest 5 to 10 minutes. Research hasn't settled the best length, so go by feel: start easy, and stop if anything hurts.

A simple routine, from low and soft to higher and louder:
1. Loosen up: roll your shoulders, yawn, and rub your jaw muscles.
2. Hum on a comfortable note, lips together and teeth apart, until you feel a buzz on your lips and nose.
3. Lip trills: blow through loose lips so they flutter, like a child making an engine sound, and sing through it. If your lips won't flutter, press a fingertip into each cheek, or hum through a drinking straw instead.
4. Five-note scales on a trill or a hum: up five notes and back down (1-2-3-4-5-4-3-2-1), then start a semitone higher and do it again, for as long as it stays easy. {start}
5. Sirens: slide smoothly from low to high and back, like a siren, on a trill, on "ng" (as at the end of "sing") or on "oo". Let the voice change gears on its own; don't push through the change.
6. Last, sing a few lines of the song itself, at half volume.

Humming, lip trills and singing through a straw all partly close the mouth. That holds back a little of the air, and the gentle pressure helps the folds vibrate with less effort, which is why voice teachers and voice clinics use them.

Short practice on most days does more than a long session once a week, and a warmed-up voice is also the one to find your range with (Voice types and range).

`{start}` is: with a range set, "The picture's scale starts on F3, a fourth above your lowest note (C3); click it to hear it." (lowest + 5, its name); without one, "The picture's scale starts on C4 (middle C); click it to hear it, and start an octave lower if that's high for you."

**4. Voice types and range** — *Bass to soprano, and finding your own range*

Your range is the span from the lowest to the highest note you can sing well: not just reach, but sing with a usable tone. Singers are grouped into voice types by where their voices sit. These are the usual ranges of trained classical singers, as voice-teaching books give them:
  Soprano: C4 to C6 (from middle C up two octaves)
  Mezzo-soprano: A3 to A5
  Alto (contralto): F3 to F5
  Tenor: C3 to C5
  Baritone: A2 to A4
  Bass: E2 to E4

Other books give ranges a few notes different, so take these as a guide, not a rule. Each spans about two octaves. Men's voices are usually bass, baritone or tenor, and women's alto, mezzo-soprano or soprano, and the two groups overlap: a tenor and an alto share most of their notes. Baritone is the most common man's voice, and most people have a medium voice rather than the highest or the lowest.

Range isn't everything. A voice type also depends on where your voice is comfortable most of the time (its tessitura, Italian for "texture") and on its colour: a tenor and a baritone can both sing an A3, but it sounds lighter from one and darker from the other. A beginner's comfortable range is often narrower than these, and it grows with practice, so don't rush to give yourself a label. Teachers often wait months before they decide.

Find your range by ear, warmed up. Click a key in the picture to hear it and sing it on "ah". Go down a note at a time until your voice gets breathy, croaky or very quiet: the last note that still sounded good is your lowest. Then go up from the middle until you'd have to strain or shout: the last easy note is your highest. Don't push past discomfort. Right-click a key to set it as your lowest or highest note, or pick a voice type above the picture. DawIt remembers your range for every song, and Your voice and your song checks your melody against it.

**5. Registers and the break** — *Chest voice, head voice, the mix, falsetto*

As you sing from low to high, your voice changes gears. Each gear is a register: a group of notes that feel and sound alike, because the folds vibrate in a similar way.

Chest voice is the full, low register you speak in. You feel it buzz in your chest, which gives it its name, but the sound is made by the folds, which vibrate thick and with firm closure. Head voice is the higher, lighter register: the folds are stretched thinner, and you feel it more in your face and head. Falsetto is lighter and breathier still, with the folds barely closing: the soft high notes of many soul and pop singers.

The break (in Italian, passaggio) is where one register hands over to the next. Sing a slow siren from low to high and your voice may crack or flip there, or suddenly get lighter. For many men the main break comes somewhere between C4 and A4: around E4 for a baritone and G4 for a tenor. For many women, chest voice gives way somewhere around E4 to A4, with another change higher up, around E5 to F#5. These are typical places, not rules: yours may be a few notes away. The picture shows a tenor and a mezzo-soprano.

Mixed voice, or the mix, is what singers call blending the registers around the break, so the voice carries on up without a crack or a sudden change of colour. Teachers explain it in different ways, and some avoid the word, but the goal is the same: one even voice from bottom to top. It comes with practice: sirens across the break, five-note scales that cross it, and singing a little lighter as you go up, rather than pushing chest voice higher.

Pushing chest voice up and louder is the most common way beginners strain: it feels powerful, but the throat tightens and the voice tires within minutes. If a note above your break only works by shouting, sing it lighter, or move the song's key down (Your voice and your song suggests how far).

**6. Singing in tune** — *Hear the note first, then land on it*

Singing in tune means your note has the pitch it should have. Differences this small are measured in cents: a semitone, one key on the piano, is 100 cents. Many listeners start to hear a sung note as out of tune when it's about 20 to 25 cents off, a fifth to a quarter of a semitone; trained musicians hear smaller differences.

The picture shows three notes as their pitch over time, against the target (the line at 0):
- Sliding in by accident: the note starts 150 cents low and slides up for a quarter of a second. A slide on purpose is a style (a scoop, in soul and country); one by accident sounds unsure.
- In tune, with vibrato: the pitch swings evenly around the target. That's fine: the ear hears the middle of the swing.
- Drifting flat: the note starts right and sinks 30 cents by its end, often from running out of breath or energy at the end of a line.

How to sing more in tune:
1. Hear the note before you sing it. Play it (click the picture), hear it in your head, then sing.
2. Match a steady reference, such as a held piano or synth note. Slide slowly around it until the two sound like one. Close but not matched, you hear a slow "wah-wah" called beating: two notes 2 Hz apart beat twice a second. It stops when you match.
3. Aim at the middle of the note from the start, rather than sliding up to it.
4. Keep the breath going to the end of the line, and sing the last note as carefully as the first.
5. Record yourself and listen back. You hear pitch far better afterwards than while you sing.

"I'm tone deaf" is rarely true. Real tone deafness (congenital amusia) affects about 1.5% of people. More people sing out of tune, about 10 to 15% in one study, but most of them hear pitch differences as well as good singers do. Their ears are fine; it's the link from hearing a note to singing it that needs practice, and it gets better with it.

**7. Vowels and words** — *Open vowels on long notes, consonants quick*

The vowels carry your voice; the consonants carry the words. Good singing gives each its job.

Long notes go on vowels. Sing "night" on a long note and the "ay" gets nearly all of it: start the n just before the beat, hold the vowel, and put the t at the very end. Holding a consonant instead, such as the "ng" of "sing" or the "l" of "feel", makes a hum that's hard to sing and hard to hear. In the picture, the red line holds "sing" on its "ng"; the line under it holds the "ih" and closes with the "ng" only at the end.

Open vowels such as "ah" (father), "oh" and "ay" (day) are the easiest to hold and to sing high. On high notes, singers open a closed vowel a little: "ee" towards "ih", "oo" towards "oh". The word still comes across, and the throat stays open. This is called vowel modification.

Keep consonants short, clear and a little early: the consonant just before the beat, the vowel on it. Then the word lands in time, as the lyrics guide's Words on a melody and How a computer sings show. End consonants, the t of "night" and the d of "hold", should be crisp but quick, and at the same moment when several voices sing.

Many vowels are two in one (diphthongs): the "ay" of "night" is "ah" sliding to "ee". Hold the first part and move to the second only at the end of the note: "naaah-eet", not "na-eeee".

Speak the line first, then sing it the way you'd say it, a little exaggerated. A word that's still hard to sing on its note can often be changed (the lyrics guide's Writing tips).

**8. Vibrato and straight tone** — *A gentle swing in pitch, or none*

Vibrato is a regular swing in pitch around a held note. In trained singers it swings about 5 to 7 times a second, in classical singing typically about half a semitone (50 to 60 cents) above and below the note. The ear doesn't hear two notes: it hears one note, at the middle of the swing, with a warmer, living tone.

A straight tone is a held note without vibrato. Pop, folk, early music and many choirs use a lot of it, and pop singers often start a long note straight and let the vibrato come in towards its end. Classical singers usually keep it through the whole note. The picture shows all three on a 2-second note.

Vibrato isn't made by shaking. In a relaxed, well-supported voice it comes by itself, from small natural pulses in the larynx. Forcing it by moving the jaw, the belly or the throat sounds mechanical. A tight voice often has none, or a fast, nervous flutter; a tired or pushed one can get a slow, wide wobble.

To practise, hold a comfortable note straight for 4 beats, then let it relax into vibrato for 4 more, without pushing. Both are tools: a straight tone sounds plain and close, vibrato warm and open, and moving between them is part of a style.

**9. Loud, soft and belting** — *Power from the breath, not the throat*

Singing louder comes from more air pressure under the folds and folds that close more firmly, not from a tighter throat. When you get louder by squeezing, the neck tightens, the chin pushes forward, and the voice soon gets hoarse.

Soft singing is harder than it sounds. It needs the same steady breath with less pressure, or the note goes breathy and flat. A classic exercise is a note from soft to loud and back (messa di voce, Italian for "placing of the voice") on a comfortable pitch: the pitch should stay put while the level changes.

Belting is the loud, bright, speech-like sound of musical theatre, rock, soul and gospel, carried higher than chest voice usually goes. Done well, it's a skill like any other, built up over months, ideally with a teacher. Done by pushing chest voice up and shouting, it's the quickest way to strain your voice. Signs you're pushing: the veins in your neck stand out, your chin lifts, it scratches or hurts, or you're hoarse afterwards. Then sing it lighter, or lower the key.

Loudness is part of the song, too. Verses are usually softer and lower, choruses louder and higher; in the picture, the thicker a note, the louder it's sung, and the chorus line is about 12 dB louder than the verse. Save your loudest for the moments that need it, so they stand out.

On a microphone you don't have to be loud: the mic and the fader make you loud. On big notes, lean back from the mic a little, and on soft ones come in a little (The microphone says why).

**10. Harmonies and doubles** — *A third above, and the same part twice*

Backing vocals make a voice sound bigger. Two kinds are used most.

A harmony is a second melody that moves with the lead, on other notes of the same chords. The most common is a third above. In the picture the lead sings E4, E4, G4, E4, D4, C4 on "hold on, hold on to-night", and the harmony sings G4, G4, B4, G4, F4, E4: each note two steps up the scale. A third is 3 or 4 semitones (a minor or a major third), so a harmony follows the key, not a fixed number of keys. A third below works too, and so does a sixth below, which is the same notes as a third above, an octave lower. Harmonies usually come in the chorus, on the hook, or on the last words of lines.

A double is the lead sung again, same notes and words, as a second take on its own track. No two takes are exactly alike, and the tiny differences in timing and pitch make the sound thicker and smoother. A copy of the same take can't do that: two identical copies only make it 6 dB louder. Doubles usually sit a little quieter than the lead, or come in pairs panned left and right.

How to sing them:
- Learn the part on its own first: solo the harmony's track, or play its notes, and sing along.
- Match the lead exactly: the same vowels, the consonants at the same moments. Keep the end consonants (s, t) light on the doubles, or they pile up into a hiss or a click.
- Sing a harmony a little softer than the lead, so it blends under it.

Put the backing parts in a bus (the routing guide shows how), so one fader sets their level. Click the picture to hear the lead with its harmony.

**11. Looking after your voice** — *Water, rest, and when to see a doctor*

Your vocal folds are small and soft, and they tire like any muscle. Most singers' voice problems come from using the voice too much or too hard, and most go away with rest.

Every day:
- Drink water through the day. What you drink never touches your vocal folds: it goes down behind them. Water keeps them moist by keeping your whole body hydrated, which takes hours, not minutes, so a glass just before you sing does little. Breathing in steam, over a bowl of hot water or in the shower, moistens them from the surface for a while. Research on both is thin, but neither does harm.
- Alcohol and a lot of caffeine dry you out; smoke and dusty air irritate the throat.
- Rest your voice before a session and after a long one. Shouting, cheering at a gig or talking over loud music the night before can cost you the session.
- Whispering doesn't rest your voice: it strains it too. Speak quietly instead, or not at all.
- Take breaks while you sing, and stop when your voice starts to tire, not long after.

Stop when it hurts. Pain, a scratchy throat, cracks on notes that are usually easy, or hoarseness after singing are signs to rest, not to push through.

See a doctor:
- if your voice is still hoarse after 2 weeks, or voice problems keep coming back;
- at once if it's very painful to swallow, or hard to breathe (in the UK, call 111 or 999).
Your GP or an ENT doctor (ear, nose and throat) can look at your vocal folds, and a speech and language therapist who works with singers can help you back.

This guide is a start, not a replacement for a singing teacher or a doctor. A good teacher hears what you can't hear yourself.

**12. The microphone** — *Distance, the pop filter, a quiet room*

A good home vocal recording comes down mostly to three things: how far you are from the microphone, what stops the pops, and how quiet and dead the room is. DawIt doesn't record audio yet: record in your audio interface's software or another app, then drag the file onto a track (or File > Import Audio..., Cmd+I).

Distance. A good starting point with a studio microphone is about 15 to 23 cm (6 to 9 inches) from your mouth, about a hand's span. Closer sounds bigger and more intimate, with less of the room; further sounds more natural but picks up more of the room and its noise. A handheld stage mic such as the Shure SM58 is made to be sung into closer, within about 15 cm.

Keep the distance steady. Sound gets quieter with distance: twice as far is about 6 dB quieter, and twice as close about 6 dB louder (a little less in a room full of echoes). In the picture, moving in from 20 cm to 10 cm makes the voice 6 dB louder, and to 5 cm 12 dB. That's why moving about while you sing makes the level jump. Use it on purpose: lean back a little on loud notes, and come in a little on soft ones.

The proximity effect. Most vocal mics are directional (cardioid: they hear the front more than the back and sides), and they add bass as you get close. Right up against it, a mic like the SM58 adds about 6 to 10 dB at 100 Hz. A little gives a warm, radio-like voice; too much sounds boomy. Omnidirectional mics, which hear all round, don't do this.

Plosives. "P" and "b", and a strong "t", push out a puff of air that hits the mic as a low thump. A pop filter, a mesh screen about 5 to 8 cm in front of the mic, breaks up the puff and keeps you from getting too close. Singing just past the mic rather than straight into it helps too. A low cut around 80 to 100 Hz when mixing removes what's left: DawIt EQ's Vocal starting point does it at 90 Hz.

The room. A small bare room adds a boxy sound that can't be taken out afterwards. Record in a room with soft things in it (a sofa, curtains, a bed), face away from bare walls, or hang a duvet behind the mic; a wardrobe full of clothes is a well-known home vocal booth. Switch off fans and anything else you can hear, and wear closed headphones so the backing track doesn't leak into the mic.

Level. Set the input so your loudest notes peak around -18 to -10 dB, never near 0: a clipped vocal can't be repaired. The gain staging guide explains why.

**13. Your voice and your song** — *Your range against your melody, a key to try*

The text starts with this paragraph, then the advice of the *Your song* rules below, one paragraph each:

Your voice and your song checks the melody you sing against your range. Pick your voice type above the picture, or set your own lowest and highest notes (Voice types and range says how to find them). DawIt remembers your range for every song. In the picture, your range is the bar at the bottom, with its top and bottom 3 semitones hatched, and each section of the song has a bar from its lowest to its highest melody note.

### Example data
- **Voice:** three boxes Breath → Vocal folds → Throat and mouth; under them two spectra of one buzz: harmonics of 220 Hz up to 3,960 Hz (18 lines), "ah" envelope peaks 730 and 1,090 Hz, "ee" 270 and 2,290 Hz, as smooth humps (an illustration, not a measured spectrum). Click plays A3.
- **Breathing** (96 BPM): line 1 "I walked along the river late" beats 0 to 6, rest 6 to 8 ("2 beats · 1.25 s: a full breath"); line 2 "the city lights behind me" 8 to 13.5, rest 13.5 to 14 ("½ beat · 0.3 s: a quick breath"); line 3 "your voice still ringing in my head, the way it used to find me" 14 to 24 ("10 beats · 6.3 s: breathe deeply before it"), a dotted breath mark after "head" at beat 19.
- **Warm-up:** 60 62 64 65 67 65 64 62 60, half a beat each, the last held 1 beat; the faint repeat a semitone up (61 to 68); siren a smooth glide 55 → 72 → 55 over 4 beats. With a range set the scale starts at lowest + 5.
- **Range:** a keyboard C2 to C7 with six bars (MIDI bass 40–64, baritone 45–69, tenor 48–72, alto 53–77, mezzo 57–81, soprano 60–84) and *You* on top from the settings.
- **Your song:** a keyboard around the notes used, your range as a bar (top and bottom 3 semitones hatched), and a bar per section from its lowest to highest melody note in the section's colour, notes outside your range in red.
- **Registers:** tenor 48–72: chest 48–62, middle 62–67, head 67–72, breaks at D4 (62) and G4 (67); mezzo 57–81: chest 57–64, middle 64–76, head 76–81, breaks at E4 (64) and E5 (76). Caption: "Typical places; yours may be a few notes away."
- **In tune** (target A4, three 1.5 s notes): slide −150 → 0 cents over 0.25 s, then 0; vibrato 0 until 0.3 s, then 40 · sin (2π · 5.5 · (t − 0.3)); drift 0 until 0.5 s, then linear to −30 at 1.5 s. Dashed lines at ±25 labelled "about where listeners hear it".
- **Vowels** (beats, 96 BPM, a consonant 0.08 s = 0.128 beat): Right "to-night": "to" at 0, 1 beat (t −0.13 to 0, uw 0 to 0.9); "night" at 1, 3 beats (n 0.87 to 1.0, ay 1.0 to 3.87, t 3.87 to 4.0). Wrong (red) "sing" at 0, 4 beats: s −0.13 to 0, ih 0 to 0.3, ng 0.3 to 4.0. Fixed: s −0.13 to 0, ih 0 to 3.85, ng 3.85 to 4.0.
- **Vibrato** (2 s, cents): straight 0; late: 0 until 1.0 s, then ±30 at 5.5 Hz (illustrative); through: ±60 at 6 Hz from the start.
- **Loud and soft:** verse 64 64 62 60, 1 beat each, at −12 dB; chorus 67 69 72 (2 beats, "belt") 69 at −3, −3, 0, −3 dB; label "12 dB louder".
- **Harmonies:** lead 64 64 67 64 62 60 at beats 0, 1, 2, 3, 4, 4.5, lengths 1, 1, 1, 1, 0.5, 3.5 ("hold on, hold on to-night"); harmony 67 67 71 67 65 64; the double the lead's notes 0.03 beat later (about 20 ms), faint.
- **Care:** Every day: water through the day; steam; rest before and after; no whispering; breaks. Stop and see a doctor: hoarse after 2 weeks; keeps coming back; very painful to swallow or hard to breathe: at once.
- **Microphone:** level against distance, relative to 20 cm, 20 log10 (20 / d): 5 cm +12.0, 10 cm +6.0, 15 cm +2.5, 20 cm 0, 30 cm −3.5, 40 cm −6.0, 60 cm −9.5 dB; the 15 to 23 cm zone green; under 10 cm "more bass (directional mics)"; the pop filter 5 to 8 cm in front of the mic.

### "Your song" rules
L and H are the melody's lowest and highest notes, lo and hi your range, "sung time" the notes' length in beats. Shifts that fit are the whole numbers from lo − L to hi − H. Bars count from 1; a note's section is the one at its bar. Key names: the song's key moved by the shift (same scale, `MusicalKey::getName`). The closest voice type is the one whose range's midpoint is nearest the midpoint of lo and hi; a tie goes to the lower type. In order; the first of rules 3 to 8 that applies is the range paragraph:

1. **No melody** (no MIDI track with notes): "Your song has no melody to check yet. Write or record the tune on a MIDI track, or turn a sung recording into notes with Convert to MIDI... in the clip's menu, then choose it as the melody above." Nothing else.
2. **No range set:** "The melody (Lead vocal) goes from C4 to B4, 11 semitones. In the usual ranges that fits a tenor, an alto, a mezzo-soprano or a soprano. Set your own range above to check it against your voice." (the types whose range holds L to H; none: "That's wider than any one voice type's usual range, so most singers would find part of it hard."). Then rule 9 only.
3. **Doesn't fit in any key** (H − L > hi − lo): "The melody spans 30 semitones (D3 to G#5), and your range spans 24 (C3 to C5), so no key fits all of it. Try singing the highest phrase an octave lower, change a few of the highest or lowest notes, or give that part to a second voice."
4. **Too high** (H > hi; shifts that fit exist): "The chorus goes 2 semitones above your range: its highest note is B4 (bar 14), and your highest is A4. Try the song 2 semitones lower: in G minor instead of A minor. The melody then runs from A#3 to A4." The shift is the fitting one closest to 0; when that's 5 or more semitones away and −12 fits: "Try singing it an octave lower: then it runs from E3 to E4, inside your range, and nothing in the song has to change. Many men sing songs written for women's voices an octave lower, and the other way round."
5. **Too low** (L < lo): the same with "below", "lowest", "higher" and +12.
6. **Near the top** (fits; 25% or more of the sung time in hi − 2 to hi): "The melody fits your range (C3 to C5), but stays near the top: 27% of the sung time is within 3 semitones of your highest note, most of it in the chorus. That's tiring over a whole song. Try it 2 semitones lower: in G minor instead of A minor." (−2 when L − 2 ≥ lo, else −1, else "It already uses the bottom of your range, so a key change won't help: warm up well, and sing the high notes lighter.")
7. **Near the bottom** (fits; 25% or more in lo to lo + 2): "...stays near the bottom: 67% of the sung time is within 3 semitones of your lowest note, most of it in the verse. Low notes are quiet and easily lost in a mix. Try it 2 semitones higher: in B minor instead of A minor." (+2, +1 or "...uses the top of your range...")
8. **Fits well:** "The melody, from C4 to A4, sits inside your range (C3 to C5), with 3 semitones to spare at the top and 12 at the bottom. A good key for you."
9. **The longest line** (lyrics exist and `singalong::findLongestLine` is over 6 s at the song's tempo): "Your longest line, "your voice still ringing in my head", lasts 6.3 seconds: plan a deep breath before it (Posture and breathing)." Quiet otherwise.

### Test numbers
| Check | Expected | Tolerance |
|---|---|---|
| Note names 40, 60, 69, 84 | E2, C4, A4, C6 | exact |
| Frequencies of 40, 57, 69, 84 | 82.41, 220.00, 440.00, 1046.50 Hz | 0.01 Hz |
| Voice table spans | 24 semitones each, 6 types, in order bass to soprano | exact |
| Closest type: 43–62, 48–72, 55–74, 62–79, 60–84 | bass, tenor, alto, mezzo-soprano (tie goes lower), soprano | exact |
| Types that fit melody 60–71 | tenor, alto, mezzo-soprano, soprano | exact |
| M1 = 60 62 64 65 (1 beat each), 67 69 71 (2 each), range 45–69, key A minor | too high by 2, shift −2, "G minor", new range 58–69 | exact |
| M1, range 48–72 | fits well; top share 20% (below 25%) | exact |
| M1 with 71 lasting 3 beats, range 48–72 | near the top, 27%, shift −2 | 1% |
| 64 67 72 76 (1 each), range 45–69 | closest shift −7, octave chosen, new range 52–64 | exact |
| 50 54 57 62, range 57–81 | too low, octave up, new range 62–74 | exact |
| Notes 50 and 80, range 48–72 | doesn't fit any key, span 30 | exact |
| 48 (2) 50 (2) 52 (1) 55 (1), range 48–72, A minor | near the bottom, 67%, shift +2, "B minor" | 1% |
| Breathing example at 96 BPM | 1.25 s, 0.3125 s, 6.25 s | 0.001 s |
| Microphone curve | +12.04, +6.02, +2.50, 0, −3.52, −6.02, −9.54 dB | 0.01 dB |
| Vibrato example | through: 60 cents peak, period 0.1667 s; late: 30 cents, period 0.1818 s | 0.5 cent, 0.001 s |
| Harmony thirds | 3 3 4 3 3 4 semitones | exact |
| Melody-track default | "Lead vocal" in the pop song's names; the selected MIDI track with notes wins; drums skipped | exact |

Text checks: 13 topics, each with a name, summary, text of 400 or more characters and an example; the last is *Your voice and your song*; the text has no "color", "center" or "analyze"; with a range set, the warm-up text names lowest + 5; search finds the guide for "hoarse", "vibrato", "voice range" and "pop filter", with the matching topic first for "vibrato".

## Steps
**Group A: content and analysis** (`./build.sh --test` at step 3)
- [x] 1. `src/generate/SingingGuide.{h,cpp}` (both CMake targets): topics, names, summaries, texts from *Theory*, the voice table, closest and fitting types, example data, the melody-track rule. Tests: `runSingingGuideTests` with the table's rows above (names, frequencies, types, examples, text checks). Done when: every topic has text and an example.
- [x] 2. The range analysis and the *Your song* text (rules 1 to 9). Tests: the made-up melodies of the table; rule 1 and 2 texts; rule 9 on a made-up 10-beat line at 96 BPM, quiet on a 4-beat one. Done when: every rule has a test.
- [x] 3. DawIt EQ's *Male voice* (`BuiltInEq.cpp`): "Sung notes from about F2 to C5, with speech low in that range (about 90 to 155 Hz). The words, though, are understood at 2-5 kHz, far above the notes." Done when: the group's build passes.

**Group B: the window and its links** (`./build.sh --test` at step 7)
- [x] 4. `TheoryGuides`: `Area::singing` and `Guide::singing` with the entry above; every switch over them (hub colour, `openGuide`, `getPrintGuide`, `ArrangementView::showGuide` / `getPrintGuide`). Tests: the entry lists its topics; the four searches. Check: the hub shows *Singing*.
- [x] 5. `src/ui/SingingExplainer.{h,cpp}`: layout, voice row, the 13 pictures, previews, right-click on keys, the playhead key, `guideprint::Source`, `showTopic`. Check: each topic in the app; Export PDF.
- [x] 6. Opening: `ArrangementView::showSingingGuide`; Sing along's *Tips...* menu (decision 4); the button on the lyrics guide's *Singing along*; *Singing techniques...* in `TrackActions`' menu for vocal names. Check: each path opens the window at the right topic.
- [x] 7. `ScreenshotRunner`: a shot key `"settings"` (an object of app-setting keys and values, written to the scratch settings before the shot), documented in `DevScreenshots.h`. Check: `--export-guides` passes.
- [x] 8. Real-app check, song "pop": guide "singing" at *How the voice works*, *Voice types and range*, *Registers and the break*, *Singing in tune*, *The microphone*, and *Your voice and your song* with `"settings": { "singing.lowest": 45, "singing.highest": 69 }` (a baritone: the melody goes above, a key 2 or more semitones lower is suggested) and once with none set; scene sing along to see *Tips...*. Look for cut-off text, empty pictures and wrong notes.
- [x] 9. Docs: README (*Features* bullet under the guides, *Tests*, *Code layout* for `SingingGuide` and `SingingExplainer`); FEATURE_IDEAS status (*Partly done*: the guide and the hand-set range; open: measuring the range, takes in cents, the *Sing* path); docs/architecture.md *Guides* (`gen::singing`, opened by `ArrangementView`); recipes.md *Checking in the real app* (the `"settings"` key).

## Tests
In `runSingingGuideTests` (steps 1 and 2) and `runTheoryGuidesTests` (step 4), the checks of *Test numbers*, plus: the default melody track for the pop song's track list is "Lead vocal"; a range with lowest above highest isn't accepted by the analysis (treated as not set).

## Website hand-off
- Features page: a section on the singing guide in the guides part: what it covers, finding your range by ear, and the key check on your own song; plainly a start, not a teacher.
- Main page: in the guide picker, the *Singing* area.
- Screenshots (song "pop"): *Your voice and your song* with a baritone range (the chorus above it and the key to try), *Singing in tune*, *Registers and the break*, *The microphone*.
- How it's made: nothing new.

## Left for later
- Measuring the range from a recording (decision 1 (b)), then from the microphone, once DawIt records audio.
- A sung take against the melody in cents: *Practise singing: record yourself and get advice*.
- The *Sing* path of *Guide paths by goal*.
- Links with *Pitch correction and pitch check effect* once it exists; its cents numbers must agree with *Singing in tune* (out of tune heard from about 20 to 25 cents; its "about 5 are hard to hear" is the smallest difference heard side by side, a different thing).
- A *Transpose the song* button on the advice, once *Change the key* exists.
- Hearing the pictures' pitch curves (slide, vibrato, drift) as sound, not only the target note.
- A range per song (decision 2), and checking the backing-vocal tracks too.
- Topics on recording takes and comping, and on a teacher's lesson types: no recording in DawIt yet.

## Changes while building
- Step 8 on the pop song: its melody runs D4 to A4, so a baritone range (A2 to A4) fits but sits near the top (58%, "try it 2 semitones lower: G minor"); the "too high" text is shown with a range of A2 to G4 instead (checked with 45 and 67).
- *Near the bottom* with no key change possible has no text in *Theory*; I wrote: "It already uses the top of your range, so a key change won't help: warm up well, and sing the low notes with a steady, supported breath."
- Topic 3's text in *Theory* ended with a note on `{start}`; that note is not shown, only its two sentences are used.
- Besides the plan: `Area::singing` sits after *Writing songs* and has hue 0xffc6e05a; the Melody menu lists all MIDI tracks; the hub's *Singing* entry has opensFrom "Sing along's Tips..., the lyrics guide's Singing along, and a vocal track's menu"; the track-menu item shows for any single track (not a return) with a vocal word.
- The *Singing techniques...* button on the lyrics guide's *Singing along* opens at *Posture and breathing*, as does Sing along's menu item; the track menu opens at *How the voice works*.
- dawTheory: SM58 "6 to 10 dB at 100 Hz" confirmed in several printings of Shure's SM58 user guide (lips about 6 mm away; the current guide says "about 6 mm" and "lips less than 15 cm" for vocals); text unchanged.
- dawTheory: Larrouy-Maestri (2018) "half of a quarter tone" confirmed in the abstract, which also stresses large individual differences; *Singing in tune* now adds "People differ a lot".
- dawTheory: Prame's 1994 abstract (JASA 96, 1979) gives a mean rate of 6.0 Hz (5.5 to 6.7 only in a secondary summary); his 1997 paper (JASA 102, 616) gives a mean extent of ±71 cents (±34 to ±123). *Vibrato* now says "about half to three quarters of a semitone (50 to 75 cents)" instead of "half a semitone (50 to 60 cents)"; the ±60 picture stays.
- dawTheory: *Loud, soft and belting* text and picture caption say "9 to 12 dB louder" (the example's chorus is −3 and 0 dB against −12), not "about 12 dB".
- dawTheory: rules 6 and 7 say "in the top (bottom) 3 semitones of your range" instead of "within 3 semitones of your highest (lowest) note", which would count one note more than the rule does; tests updated. Rule 7's no-key-change wording checked: true (it only shows when the melody's highest note is the top of the range).
