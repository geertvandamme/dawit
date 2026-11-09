#include "InstrumentsGuide.h"

#include "SingingGuide.h"
#include "engine/MidiGuide.h"

#include <algorithm>
#include <cmath>

namespace daw::gen::instruments
{
namespace
{
    juce::String utf8 (const char* text) { return juce::String::fromUTF8 (text); }

    //==============================================================================
    // The texts are the ones in the plan's Theory section; each family's ends with what the picture shows.

    const char* const textMakeSound =
        "Every acoustic instrument does three things. Something vibrates: a string, the air in a tube, a reed or the player's lips, a drum skin, a wooden or metal bar, "
        "or a singer's vocal folds. Something makes it louder: a string on its own moves hardly any air, so a guitar passes its vibration through the bridge into a wooden "
        "body, and a piano into its soundboard, a large thin board that moves far more air than a string can. And something shapes the colour: the body, the tube and the "
        "player's mouth have resonances of their own that make some frequencies stronger than others.\n\n"
        "Electric and electronic instruments end the same way. An electric guitar's pickup turns the string's movement into a signal, an amplifier makes it bigger, and a "
        "speaker cone moves the air. A synth skips the vibrating object and makes the signal directly.\n\n"
        "The pitch comes from how many times a second the vibration repeats, and the colour from the mix of harmonics and from how a note starts and ends (What is sound? "
        "explains both). The next three topics show why strings, tubes and drums give such different mixes.\n\n"
        "Every family topic after them answers the same questions, so you can compare: what vibrates, what makes it louder, its range, how it's played, and how to write it "
        "in MIDI so it sounds played rather than typed. DawIt plays and shows notes as they sound; where an instrument's music is written at another pitch, the topic says so.\n\n"
        "The picture shows each kind of thing that vibrates, with an instrument that uses it.";

    const char* const textStrings =
        "A string is fixed at both ends, so it can only vibrate in shapes that fit exactly between them: the whole length swinging as one, two halves swinging in opposite "
        "directions, three thirds, and so on. A real string does all of these at once. Each shape vibrates at a whole-number multiple of the lowest: an A string at 110 Hz "
        "also sounds 220, 330, 440 and 550 Hz. These are the harmonics, and their mix is a big part of the instrument's colour.\n\n"
        "Three things set the pitch: length, tension and weight. A shorter, tighter or lighter string sounds higher. Stop a string at half its length and its pitch doubles, "
        "an octave up: 440 Hz becomes 880 Hz. Two thirds of the length gives a fifth up, 660 Hz. That's what a guitarist's frets and a violinist's fingers do. A tuning peg "
        "changes the tension, and the low strings are thicker and heavier.\n\n"
        "Where the string is plucked, struck or bowed changes the mix. Near the middle gives a round, soft tone with few high harmonics; near the end, close to a guitar's "
        "bridge, a bright, thin one. A harmonic that has a still point (a node) where you pluck can't sound at all: pluck exactly at the middle and the even harmonics are "
        "missing. What is physical modelling? lets you hear this on DawIt Physical's string.\n\n"
        "Piano, guitar, harp, violin, cello and bass are all string instruments. They differ in how the string is set going (a hammer, a finger, a pick, a bow) and in what "
        "makes it louder.\n\n"
        "The picture shows a string's first four shapes with their frequencies on A2, and an A4 string stopped at half and at two thirds of its length.";

    const char* const textTubes =
        "In wind and brass instruments, the air inside a tube vibrates. Something at one end keeps it going: breath blown across an edge (the flute), one cane reed "
        "(clarinet and saxophone), two reeds against each other (oboe and bassoon), or the player's lips buzzing in a mouthpiece (every brass instrument). The length of "
        "the tube sets the pitch: a longer tube sounds lower. Keys, holes, valves and a slide change how much of the tube vibrates.\n\n"
        "The ends decide which harmonics fit. A tube open at both ends, like a flute, holds every harmonic, as a string does. A tube closed at one end, like a clarinet "
        "(the mouthpiece and reed close it), holds only the odd ones, 1, 3, 5 and so on, and its lowest note is an octave below an open tube of the same length. Worked out "
        "for a 66 cm tube, with sound travelling at 343 metres a second: open at both ends it sounds about 260 Hz; closed at one end, about 130 Hz, with 390 and 650 Hz above. "
        "Real instruments come out a little different, because of the mouthpiece, the holes and the bell.\n\n"
        "That's why a clarinet's low notes sound hollow and woody, and why it jumps a twelfth (to the 3rd harmonic) when the player opens the register key, where a flute "
        "jumps an octave. A real clarinet has some even harmonics, but much weaker ones. Oboes, bassoons and saxophones have cone-shaped tubes, which act like open ones: "
        "every harmonic, and an octave jump.\n\n"
        "Brass players use the harmonics directly, changing their lips to move from one to the next (see Brass).\n\n"
        "The picture shows the waves that fit in an open and a closed tube of 66 cm, and the harmonics each gives.";

    const char* const textDrumsBars =
        "A drum skin is held all round its edge, so it vibrates in rings and slices, not halves and thirds, and the frequencies of those shapes aren't whole-number "
        "multiples. On an ideal round skin, the shapes above the lowest vibrate at 1.59, 2.14, 2.30, 2.65 and 2.92 times its frequency: for a drum at 100 Hz, 159, 214, "
        "230, 265 and 292 Hz. That mix doesn't add up to one clear note, which is why most drums sound like a thud or a crack rather than a pitch you could sing. Drummers "
        "still tune them higher or lower.\n\n"
        "Timpani are the exception. The kettle under the skin and the air inside it pull the overtones close to a harmonic series, so timpani play real notes, and the "
        "player retunes them with a pedal.\n\n"
        "A bar that's free at both ends, like a xylophone's, has overtones at about 2.76 and 5.40 times the lowest: for a bar at 440 Hz, about 1,213 and 2,378 Hz. Makers "
        "cut an arch out of the underside to move them: a marimba bar's first overtone is tuned two octaves up, 4 times the lowest, and a xylophone's a twelfth up, 3 times. "
        "That's what makes them sound like notes. Cymbals and gongs have so many unrelated overtones that they sound like a wash of noise.\n\n"
        "In MIDI, drums don't play pitches: each note number picks a different drum (see Drums and percussion).\n\n"
        "The picture puts the overtones of a drum skin, a free bar and a marimba bar against a string's harmonics.";

    const char* const textKeys =
        "What vibrates: steel strings, one to three for each note. A key throws a felt hammer at them, and the hammer falls straight back so the strings ring. The faster "
        "the key goes down, the faster the hammer, and the louder and brighter the note: a hard hit makes more high harmonics. That speed is what MIDI velocity measures.\n\n"
        "What makes it louder: the strings press on a bridge glued to the soundboard, a big, thin wooden board. A string alone moves very little air; the board moves a lot.\n\n"
        "Range: 88 keys, from A0 (27.5 Hz, MIDI note 21) to C8 (4,186 Hz, note 108). A piano note starts dying away at once: the low notes ring for many seconds, the top "
        "ones for less than one. Letting go of the key drops a felt damper on the strings, unless the sustain pedal holds all the dampers up.\n\n"
        "How it's played: with two hands. A common split is the left hand on the bass note, or the root and fifth, and the right hand on the chord or the melody. Each hand "
        "reaches about an octave.\n\n"
        "In MIDI:\n\n"
        "Vary the velocity. Played by hand, a melody note is usually louder than the chord under it, and the first beat of a bar a little stronger. Every note at 100 sounds typed.\n\n"
        "Change the sustain pedal at each chord change: up just after the new chord starts, then down again, so the old chord doesn't blur into the new one (What is MIDI?, "
        "The sustain pedal).\n\n"
        "Keep each hand to what it can reach: about an octave, and five fingers.\n\n"
        "Notes of a chord played by hand don't all start on the same instant; a few milliseconds apart is enough to sound played.\n\n"
        "The organ is a keyboard too, but it doesn't fade: a note sounds as long as the key is down, and velocity usually changes nothing. A pipe organ's pipes are tubes "
        "(Tubes: open and closed). Organists swell the sound with an expression pedal; in MIDI that's usually controller 11, expression.\n\n"
        "The picture shows the piano's range on its keyboard and a two-handed C major chord: C2 and G2 in the left hand, E4, G4 and C5 in the right.";

    const char* const textGuitar =
        "What vibrates: six strings, tuned E2, A2, D3, G3, B3 and E4 in standard tuning, plucked with a finger or a pick. Each fret shortens the string and raises it a "
        "semitone.\n\n"
        "What makes it louder: on an acoustic guitar, the bridge passes the vibration to the wooden top, and the hollow body adds warmth to the low notes. An electric "
        "guitar's solid body is nearly silent: a magnetic pickup turns the string's movement into a signal, and the amplifier shapes much of the tone.\n\n"
        "Range: from E2 (82 Hz, MIDI note 40) to about D6 on a guitar with 22 frets (E6 with 24; a classical guitar stops lower). Guitar music is written an octave higher "
        "than it sounds, to keep it on the treble staff; DawIt's piano roll shows the notes as they sound.\n\n"
        "How it's played: strummed chords, single picked notes, fingerpicking (the thumb on the bass strings, the fingers on the top ones), palm muting (the side of the hand "
        "resting on the strings by the bridge, for a short, chunky sound), slides, bends and hammer-ons.\n\n"
        "In MIDI:\n\n"
        "Six notes at most, one per string, in shapes a hand can hold. An eight-note piano chord is unplayable on a guitar; Piano and guitar chords shows real shapes.\n\n"
        "A strum isn't a block: the pick crosses the strings one after another. A downstroke starts on the low string, an upstroke on the high one. As a rule of thumb, start "
        "each string about 10 to 30 milliseconds after the one before. At 120 BPM, 15 ms is 0.03 of a beat, and a six-string strum then takes 75 ms.\n\n"
        "Upstrokes are usually lighter and often catch only the top three or four strings: give them less velocity.\n\n"
        "A strummed chord rings until the next one, unless it's muted.\n\n"
        "Play chords as... on the chord track writes strummed and picked parts this way.\n\n"
        "The picture shows the six open strings on the keyboard, and a down and an up strum of an E chord (E2 B2 E3 G\xE2\x99\xAF" "3 B3 E4), each string 15 ms after the one before.";

    const char* const textBass =
        "What vibrates: four strings tuned E1, A1, D2 and G2, an octave below a guitar's lowest four. Many players use five strings, adding a low B0 (31 Hz). The electric "
        "bass has pickups and an amplifier, like an electric guitar; the double bass, its acoustic relative, is in Strings.\n\n"
        "Range: from E1 (41 Hz, MIDI note 28) to about G4 on a bass with 24 frets (D\xE2\x99\xAF" "4 with 20). Like the guitar, bass is written an octave higher than it sounds.\n\n"
        "How it's played: with the fingers (round and warm), a pick (a sharper start to each note), or slapped (the thumb bounces off the low strings and the fingers pop the "
        "high ones, the sound of funk). Ghost notes are muted, almost pitchless hits between the real notes that add rhythm.\n\n"
        "What a bass line does: it joins the drums' rhythm to the chords, and holds the band together.\n\n"
        "Start with the root of each chord on its first beat: under C, a C.\n\n"
        "Add the fifth (G under C) and the octave for movement: root, fifth, octave works in many styles.\n\n"
        "Lead into the next chord with an approach note a semitone or a tone away, on the last beat: C, G, C, C\xE2\x99\xAF, then D.\n\n"
        "The octave pattern, the root low and high on alternate eighth notes, drives disco and dance music.\n\n"
        "Play with the kick drum: bass notes on the kick's beats make the groove tight.\n\n"
        "In MIDI:\n\n"
        "One note at a time. Two notes together low down sound muddy; when you do play two, keep them wide apart, a fifth or a tenth.\n\n"
        "Mix short and long notes, and leave small gaps: they let the groove breathe.\n\n"
        "Vary the velocity a little, strongest on the main beats.\n\n"
        "The picture shows the bass's range and a root, fifth, octave line with an approach note: C2, G2, C3, C\xE2\x99\xAF" "3, then D3, A2, D2.";

    const char* const textBowed =
        "What vibrates: four strings, set going by a bow or plucked with a finger. Rosin makes the bow's hair grip the string, which sticks and slips over and over and keeps "
        "sounding as long as the bow moves (What is physical modelling? shows how). The bridge passes the vibration into a hollow wooden body.\n\n"
        "Ranges and open strings, as they sound:\n\n"
        "Violin: G3 to about E7 (196 to 2,637 Hz); strings G3, D4, A4, E5.\n\n"
        "Viola: C3 to about C6; strings C3, G3, D4, A4.\n\n"
        "Cello: C2 to about E5; strings C2, G2, D3, A3.\n\n"
        "Double bass: E1 to about B3; strings E1, A1, D2, G2, written an octave higher than it sounds. Some have an extension down to C1.\n\n"
        "The top notes depend on the player, and most parts stay well below them.\n\n"
        "How it's played:\n\n"
        "Legato: notes joined smoothly, several in one bow.\n\n"
        "Spiccato: short notes with the bow bouncing off the string, for fast, rhythmic parts.\n\n"
        "Tremolo: the bow moving back and forth as fast as it can on one note, for tension or a shimmer.\n\n"
        "Pizzicato: plucked with a finger, short and soft.\n\n"
        "Vibrato: the finger rocking on the string, a small swing in pitch on long notes.\n\n"
        "Divisi: a section splits, some players taking one note and the rest another.\n\n"
        "A string section is first violins, second violins, violas, cellos and double basses; the basses often play the cello part an octave lower.\n\n"
        "In MIDI:\n\n"
        "Shape long notes with a swell. Many sampled strings change loudness and tone with the mod wheel (controller 1) or expression (controller 11) while a note sounds, "
        "not only with velocity: draw a rise and fall in the Mod wheel or Expression lane.\n\n"
        "For a legato line, overlap each note a little with the next. Legato patches play one note at a time and use the overlap to play the join.\n\n"
        "Sample libraries switch articulations with keyswitches: notes below the instrument's range that change it to, say, pizzicato. Put a keyswitch a few ticks before "
        "the note it changes, and check the library's manual, as layouts differ.\n\n"
        "Orchestral samples often start slowly: moving a part 10 to 40 milliseconds earlier can put it on the beat.\n\n"
        "A single violin, viola or cello plays one or two notes at once; chords of three or four are spread quickly across the strings. For full chords, use a section.\n\n"
        "The picture shows the four instruments' ranges and open strings on the keyboard.";

    const char* const textBrass =
        "What vibrates: the player's lips, buzzing in a cup-shaped mouthpiece, set the air in a long metal tube going. The bell at the end lets the sound out and gives it "
        "its brightness.\n\n"
        "Why the pitch is what it is: brass players pick notes from the harmonics of the tube, tightening the lips and blowing faster to go up. A B\xE2\x99\xAD trumpet with no "
        "valves pressed gives B\xE2\x99\xAD" "3, F4, B\xE2\x99\xAD" "4, D5, F5 and B\xE2\x99\xAD" "5, harmonics 2 to 6 and 8 of B\xE2\x99\xAD" "2 (116.5 Hz). The 7th, near A\xE2\x99\xAD" "5, is 31 cents flat, "
        "so it's avoided. Valves add extra tube: the first lowers the pitch by two semitones, the second by one, the third by three, and together they fill the gaps between "
        "harmonics. A trombone's slide does the same with seven positions, each a semitone lower.\n\n"
        "Ranges, as they sound:\n\n"
        "Trumpet in B\xE2\x99\xAD: E3 to about B\xE2\x99\xAD" "5, written a tone higher than it sounds.\n\n"
        "French horn in F: B1 to about F5, written a fifth higher.\n\n"
        "Trombone: E2 to about B\xE2\x99\xAD" "4, with low pedal notes below.\n\n"
        "Tuba: comfortable from F1 to C4; pedal notes reach B\xE2\x99\xAD" "0, and strong players go to C5 and above.\n\n"
        "How it's played: everything comes from the breath and the lips, so loudness, tone and phrase length depend on breathing. Brass can swell from very soft to very loud, "
        "and it gets brighter as it gets louder. Falls (the pitch dropping off at the end of a note) and doits (a slide up) are common in jazz and pop. A mute in the bell "
        "makes it quieter and changes the colour: a straight mute is thin and bright, a cup mute soft and round, a harmon mute the buzzy, distant sound of Miles Davis.\n\n"
        "Sections: a pop horn section is often trumpet, saxophone and trombone; a big band has sections of trumpets, trombones and saxophones, four or five each. Close "
        "voicings, with the chord's notes within an octave, give the punchy section sound.\n\n"
        "In MIDI:\n\n"
        "One note at a time per player: for a chord, a track per instrument, or a brass section patch.\n\n"
        "Leave room to breathe: a rest every few bars.\n\n"
        "Velocity sets the attack; the Expression lane shapes swells and fades on long notes.\n\n"
        "Short stabs need short notes; long chords need a swell, not a flat level.\n\n"
        "Don't keep a part in its top few notes for long: they tire the player and sound strained.\n\n"
        "The picture shows the brass ranges on the keyboard and a B\xE2\x99\xAD trumpet's harmonics with no valve pressed.";

    const char* const textWoodwinds =
        "What vibrates: the air in a tube, kept going by the breath. A flute player blows across an edge, like across the top of a bottle. A clarinet and a saxophone have "
        "one cane reed against a mouthpiece; an oboe and a bassoon have two reeds against each other, which gives their nasal, reedy sound. Keys and holes shorten the "
        "vibrating tube for higher notes. The saxophone is made of brass, but it's a woodwind because it uses a reed.\n\n"
        "Why they sound different: the flute's tube is open at both ends and gives every harmonic, a pure, breathy tone. The clarinet's is a cylinder closed at the reed, so "
        "its low notes are mostly odd harmonics, the hollow sound in Tubes: open and closed. Oboes, bassoons and saxophones have cone-shaped tubes, which give every harmonic.\n\n"
        "Ranges, as they sound:\n\n"
        "Flute: C4 to about C7. The piccolo plays an octave higher, and is written an octave lower than it sounds.\n\n"
        "Oboe: B\xE2\x99\xAD" "3 to about F6.\n\n"
        "Clarinet in B\xE2\x99\xAD: D3 to about B\xE2\x99\xAD" "6, written a tone higher than it sounds.\n\n"
        "Bassoon: B\xE2\x99\xAD" "1 to about E5; most parts stay at or below C5.\n\n"
        "Alto saxophone: D\xE2\x99\xAD" "3 to A\xE2\x99\xAD" "5, written a major sixth higher.\n\n"
        "Tenor saxophone: A\xE2\x99\xAD" "2 to E\xE2\x99\xAD" "5, written a ninth higher.\n\n"
        "Many saxophones have a high F\xE2\x99\xAF key, one semitone more, and skilled players go higher still.\n\n"
        "How it's played: one note at a time, on the breath. Phrases are shaped like sung lines: they grow and fade, and end where the player breathes. Notes are tongued "
        "(each started with the tongue, a clear start) or slurred (joined, only the first tongued).\n\n"
        "In MIDI:\n\n"
        "One note at a time: a chord needs a track per instrument.\n\n"
        "Breathe: a short rest every few bars. As a rule of thumb, a phrase that runs on for 8 seconds or more with no gap sounds like a machine.\n\n"
        "Shape long notes in the Expression lane, and give the notes inside a slurred phrase slightly less velocity than its first.\n\n"
        "Use each instrument's best register: the flute is soft low down and bright up high; the clarinet dark and rich at the bottom; the oboe cuts through in its middle.\n\n"
        "The picture shows the woodwind ranges on the keyboard.";

    const char* const textVoice =
        "What vibrates: two small folds in the throat, the vocal folds, set going by the breath. The throat, mouth and nose shape the sound: their resonances, called "
        "formants, make the vowels. Singing techniques explains how the voice works and how to sing.\n\n"
        "Ranges: voice teachers give each voice type about two octaves: bass E2 to E4, baritone A2 to A4, tenor C3 to C5, alto F3 to F5, mezzo-soprano A3 to A5, soprano "
        "C4 to C6. Most pop singers use less, and the comfortable part is the middle. A tenor part is usually written in the treble clef, an octave higher than it sounds.\n\n"
        "How voices are used:\n\n"
        "The lead vocal carries the words and the melody.\n\n"
        "Backing vocals sing harmonies (often a third above or below the melody), oohs and aahs behind the lead, or answers to it.\n\n"
        "A choir usually sings in four parts, SATB: soprano, alto, tenor and bass, each in its own range.\n\n"
        "Vocal chops are short pieces of a recorded voice, cut up and played as an instrument, common in dance music.\n\n"
        "In MIDI: a MIDI note can't sing words, so a MIDI voice is a choir or \"aah\" sample, a vocal chop in a sampler, or a sketch of a melody someone will sing later.\n\n"
        "Keep every part inside its voice type's range, and leave breaths as a singer would: as a rule of thumb, a line that runs 8 seconds or more with no rest needs one.\n\n"
        "One note at a time per part: for a choir, a track for each of the four parts, or a choir patch.\n\n"
        "Choir pads sound more real with the Expression lane rising into each chord and falling away.\n\n"
        "Sing this melody... on a MIDI clip lets you hear a melody sung, and Singing techniques checks a melody against your range.\n\n"
        "The picture shows the six voice types' ranges on the keyboard.";

    const char* const textSynths =
        "What vibrates: in the end, a speaker cone. A synth makes the signal itself, from oscillators and filters or from recordings, and an amplifier and a speaker turn it "
        "into moving air. So a synth has no fixed range and never needs a breath: how it plays is up to you.\n\n"
        "How synth players play, with one hand on the keys and one on the controls:\n\n"
        "Pitch bend: a wheel that slides the pitch up and down, usually by up to 2 semitones, for bends like a guitarist's or a singer's.\n\n"
        "The mod wheel: usually adds vibrato, or opens a filter, while a note sounds.\n\n"
        "Glide (portamento): the pitch slides from one note to the next instead of jumping. On DawIt's synths in mono mode, only overlapping notes slide, so you choose which "
        "notes glide by how you play them.\n\n"
        "Mono and poly: a mono synth plays one note at a time, for leads and basses; a poly synth plays chords, for pads and keys.\n\n"
        "The arpeggiator turns a held chord into a pattern of single notes (What is an arpeggiator?).\n\n"
        "Knobs: players turn the filter's cutoff by hand during a part, for a build-up.\n\n"
        "In MIDI:\n\n"
        "Use the Bend lane for a slide into a note, and the Mod wheel lane for vibrato on long notes only.\n\n"
        "For a mono lead with glide, overlap the notes you want to slide to, and leave a gap before the ones you want to jump to.\n\n"
        "Velocity may change nothing on a synth sound, or may open its filter: listen before drawing it.\n\n"
        "A synth doesn't have to imitate anything, but the same habits make it musical: phrases with rests, accents on the strong beats, long notes that change.\n\n"
        "The picture shows a synth lead: a bend up 2 semitones into a note, and a glide between two overlapping notes.";

    const char* const textDrumKit =
        "What vibrates: skins (the drums), metal plates (the cymbals), wood and metal, and seeds inside a shaker. Most make a noise more than a note (Drums, bars and bells).\n\n"
        "The kit: a kick drum played with a foot pedal, a snare with wires under it that buzz, toms, a hi-hat (two cymbals the other foot opens and closes), a ride and a "
        "crash. In General MIDI the drums are on channel 10, and each note number is an instrument: 36 is the kick, 38 the snare, 42 the closed hi-hat, 46 the open hi-hat, "
        "49 a crash and 51 a ride. DawIt Synth's Drums sound follows this map, from note 35 to 81, and so do most drum samplers.\n\n"
        "How drummers play: two hands and two feet, so four hits at the same moment at most. A basic rock beat has the hi-hat on every eighth note, the kick on 1 and 3, and "
        "the snare on 2 and 4. Drummers accent the hi-hat (louder on the beat, softer between) and add ghost notes: very soft snare hits between the main ones, felt more "
        "than heard. A fill, a short busy run on the toms or the snare, marks the end of a section.\n\n"
        "Percussion adds a layer on top: a shaker or tambourine on eighths or sixteenths, congas and bongos in short repeating patterns, a cowbell or claves on a fixed "
        "rhythm. Many musical traditions have their own: the tabla in Indian music, the steel pan from Trinidad and Tobago, the kalimba, a thumb piano with metal tines, "
        "from Africa.\n\n"
        "In MIDI:\n\n"
        "Vary the velocity: as a rule of thumb, hi-hats about 90 to 110 on the beat and 60 to 80 between, a strong backbeat snare, and ghost notes very soft, about 20 to 40.\n\n"
        "No more than four hits at once, and none a drummer couldn't reach.\n\n"
        "On a real hi-hat, closing it stops the open sound: end an open hi-hat note where the next closed one starts.\n\n"
        "Don't quantise every hit to 100%: small timing differences sound played (Note lengths, beats and bars, Quantising and feel).\n\n"
        "Change the pattern for each section, and add a fill at the end of a phrase.\n\n"
        "The picture shows one bar of a rock beat on the drum map, with its velocities: hi-hat accents, the backbeat and two ghost notes.";

    const char* const textOrchestration =
        "Orchestration is choosing who plays what. The same few ideas work for a band and an orchestra.\n\n"
        "One part carries the melody, and it has to be easy to hear: high, or in a colour that stands out. In an orchestra the first violins, the flute, the oboe or the "
        "trumpet often have it; in a band, the voice or a lead instrument.\n\n"
        "Give each part its own register: the bass at the bottom, the chords in the middle, the melody on top. Two parts in the same few notes blur into each other.\n\n"
        "Low notes need space. Notes close together sound clear high up but muddy low down: as a rule of thumb, below about C3 keep to octaves and fifths.\n\n"
        "Doubling at the octave makes a line stronger without thickening it: violins with a flute an octave above, cellos with double basses an octave below. Doubling in "
        "unison (two instruments on the same notes) blends their colours into a new one.\n\n"
        "Take turns. Strings, then woodwinds answering, then everyone at the climax. A full sound all the time leaves nowhere to grow.\n\n"
        "Write for what each instrument does well: long lines for strings and horns, quick figures for woodwinds and pizzicato, punch for brass.\n\n"
        "Film, TV and game music lean on all of these.\n\n"
        "The picture stacks every family's range on one keyboard, and shows a melody on the violins doubled by a flute an octave higher.";

    const char* const textRealMidi =
        "A part sounds typed when every note has the same strength and length, sits exactly on the grid, and does things the instrument can't. Players do none of that. A "
        "checklist for every family:\n\n"
        "1. Range: keep each part inside its instrument's range, and mostly in the middle of it. A cello note below C2 doesn't exist.\n\n"
        "2. Notes at once: one for wind, brass and voice and most bass lines; six on a guitar; five fingers a hand on a piano; four limbs on a drum kit.\n\n"
        "3. Breath: wind, brass and voices need a rest every few bars.\n\n"
        "4. Velocity: vary it. Strong beats a little louder, the top of a phrase loudest, repeated notes never all the same. In a sampled instrument, velocity often picks a "
        "different recording, so it changes the tone as well as the level (What is a sampler?).\n\n"
        "5. Note lengths: mix short and long, and leave gaps where a player would lift a finger or the bow. Legato lines overlap a little; staccato notes are short.\n\n"
        "6. Timing: quantise, but not always fully. A strum spreads its notes; a rolled chord on a harp or piano starts its notes a few milliseconds apart.\n\n"
        "7. Change inside a note: a held note on a real instrument isn't flat. Draw a swell in the Expression or Mod wheel lane, add vibrato to long notes, bend into a note "
        "on a guitar or synth.\n\n"
        "8. Articulation: switch between joined, short and accented notes, with keyswitches or a track for each.\n\n"
        "The picture shows the same string line twice. Typed: every note at velocity 100 and 0.9 beats long, no swell. Played: velocities from 84 to 104, each note overlapping "
        "the next by 0.05 beats for legato, and expression rising from 70 to 110 on the long last note.";

    // Not in the plan: a short introduction to the analysis (the page names what it does and what it never does).
    const char* const textYoursIntro =
        "This page checks the notes of the selected MIDI track against the instrument they're for: its range, how many notes start together, whether a wind, brass or vocal "
        "part leaves room to breathe, how much the velocity varies, and whether long notes swell. DawIt guesses the instrument from the track's DawIt Sampler preset, then its "
        "name, then its DawIt sound; choose another above if the guess is wrong. Nothing is saved, and your track is never changed.";

    const char* const textNoTrack = "Select a MIDI track with notes, and this topic checks them against the instrument they're for.";

    //==============================================================================
    Info makeInfo (Instrument i, Family f, const char* name, int low, int high, int atOnce, bool breathes, bool swells, int written,
                   const char* gm, const char* words, int five = -1)
    {
        Info info;
        info.instrument = i;
        info.family = f;
        info.name = name;
        info.lowest = low;
        info.highest = high;
        info.atOnce = atOnce;
        info.breathes = breathes;
        info.swells = swells;
        info.writtenHigher = written;
        info.fiveStringLowest = five;
        info.generalMidi = gm;
        info.nameWords = juce::StringArray::fromTokens (words, ",", "");
        return info;
    }

    struct ProgramRange
    {
        Instrument instrument;
        int from, to;   // General MIDI programs as shown, 1 to 128
    };

    constexpr ProgramRange programRanges[] {
        { Instrument::piano, 1, 8 }, { Instrument::organ, 17, 24 }, { Instrument::guitar, 25, 32 }, { Instrument::bassGuitar, 33, 40 },
        { Instrument::violin, 41, 41 }, { Instrument::viola, 42, 42 }, { Instrument::cello, 43, 43 }, { Instrument::doubleBass, 44, 44 },
        { Instrument::stringSection, 45, 46 }, { Instrument::harp, 47, 47 }, { Instrument::stringSection, 49, 52 },
        { Instrument::choir, 53, 55 }, // 54, Voice Oohs, is a choir in most SoundFonts
        { Instrument::trumpet, 57, 57 }, { Instrument::trombone, 58, 58 }, { Instrument::tuba, 59, 59 }, { Instrument::trumpet, 60, 60 },
        { Instrument::frenchHorn, 61, 61 }, { Instrument::brassSection, 62, 64 },
        { Instrument::altoSax, 66, 66 }, { Instrument::tenorSax, 67, 67 }, { Instrument::oboe, 69, 69 }, { Instrument::bassoon, 71, 71 },
        { Instrument::clarinet, 72, 72 }, { Instrument::flute, 74, 74 }, { Instrument::synth, 81, 96 }
    };

    /** The words of a track name: lower case letters and digits, with hyphens kept inside a word ("hi-hat"). */
    juce::StringArray getWords (const juce::String& text)
    {
        juce::StringArray words;
        juce::String word;

        for (auto c : text.toLowerCase())
        {
            if (juce::CharacterFunctions::isLetterOrDigit (c) || c == '-')
            {
                word += juce::String::charToString (c);
            }
            else if (word.isNotEmpty())
            {
                words.add (word);
                word.clear();
            }
        }

        if (word.isNotEmpty())
            words.add (word);

        return words;
    }

    bool containsPhrase (const juce::StringArray& words, const juce::StringArray& phrase)
    {
        if (phrase.isEmpty() || phrase.size() > words.size())
            return false;

        for (int start = 0; start + phrase.size() <= words.size(); ++start)
        {
            bool all = true;

            for (int k = 0; k < phrase.size() && all; ++k)
                all = words[start + k] == phrase[k];

            if (all)
                return true;
        }

        return false;
    }

    std::optional<Instrument> guessFromName (const juce::String& trackName)
    {
        const auto words = getWords (trackName);

        struct Candidate
        {
            juce::StringArray phrase;
            int length;
            Instrument instrument;
        };

        std::vector<Candidate> candidates;

        for (auto& info : getInstruments())
            for (auto& w : info.nameWords)
                candidates.push_back ({ getWords (w), w.length(), info.instrument });

        // The longest phrase first, so "double bass" and "bassoon" win over "bass".
        std::stable_sort (candidates.begin(), candidates.end(), [] (auto& a, auto& b) { return a.length > b.length; });

        for (auto& c : candidates)
            if (containsPhrase (words, c.phrase))
                return c.instrument;

        return std::nullopt;
    }

    juce::String capitalised (const juce::String& s) { return s.substring (0, 1).toUpperCase() + s.substring (1); }

    /** Seconds as the text shows them: "12" for 12.0, "8.4" otherwise. */
    juce::String describeSeconds (double seconds)
    {
        const auto rounded = std::round (seconds * 10.0) / 10.0;
        return std::abs (rounded - std::round (rounded)) < 0.01 ? juce::String ((int) std::round (rounded)) : juce::String (rounded, 1);
    }

    juce::String countNotes (int n, const juce::String& verbSingular, const juce::String& verbPlural)
    {
        return n == 1 ? "1 note " + verbSingular : juce::String (n) + " notes " + verbPlural;
    }

    RangeBar makeBar (Instrument i, std::vector<int> openStrings = {})
    {
        const auto& info = getInfo (i);
        return { info.name, info.lowest, info.highest, info.family, std::move (openStrings) };
    }

    ExampleNote note (int pitch, double start, double length, int velocity) { return { pitch, start, length, velocity }; }
} // namespace

//==============================================================================
juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::makeSound:      return "How instruments make sound";
        case Topic::strings:        return "Strings and their harmonics";
        case Topic::tubes:          return "Tubes: open and closed";
        case Topic::drumsBars:      return "Drums, bars and bells";
        case Topic::keys:           return "Keys: piano and organ";
        case Topic::guitar:         return "Guitar";
        case Topic::bass:           return "Bass";
        case Topic::bowed:          return "Strings";
        case Topic::brass:          return "Brass";
        case Topic::woodwinds:      return "Woodwinds";
        case Topic::voice:          return "Voice and choirs";
        case Topic::synths:         return "Synths and electronic instruments";
        case Topic::drumKit:        return "Drums and percussion";
        case Topic::orchestration:  return "Orchestration basics";
        case Topic::realMidi:       return "What makes MIDI sound real";
        case Topic::yours:          return "Your track";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::makeSound:      return "Something vibrates, something makes it louder, something shapes it";
        case Topic::strings:        return "Half the length, twice the pitch: 440 Hz becomes 880 Hz";
        case Topic::tubes:          return "Why a clarinet sounds hollow, and an octave lower than a flute of the same length";
        case Topic::drumsBars:      return "Why a drum's overtones aren't harmonics";
        case Topic::keys:           return "88 keys, hammers, a soundboard and the sustain pedal";
        case Topic::guitar:         return "Six strings, frets, strums, and written an octave up";
        case Topic::bass:           return "Roots and fifths, approach notes, one note at a time";
        case Topic::bowed:          return "Violin, viola, cello and double bass: bowing, plucking and swells";
        case Topic::brass:          return "Buzzing lips, the harmonic series, valves and the slide";
        case Topic::woodwinds:      return "Flute, clarinet, oboe, bassoon and saxophone: edges, reeds and breath";
        case Topic::voice:          return "Voice types, breath, choirs, backing vocals and vocal chops";
        case Topic::synths:         return "Pitch bend, the mod wheel, glide and the arpeggiator";
        case Topic::drumKit:        return "The kit, the drum map, four limbs, accents and ghost notes";
        case Topic::orchestration:  return "Who carries the tune, doubling, and giving each part room";
        case Topic::realMidi:       return "Range, breath, velocity, length, timing and change inside a note";
        case Topic::yours:          return "Your track's notes against its instrument";
    }

    return {};
}

juce::String explain (Topic t, const juce::String& yourTrackText)
{
    switch (t)
    {
        case Topic::makeSound:      return utf8 (textMakeSound);
        case Topic::strings:        return utf8 (textStrings);
        case Topic::tubes:          return utf8 (textTubes);
        case Topic::drumsBars:      return utf8 (textDrumsBars);
        case Topic::keys:           return utf8 (textKeys);
        case Topic::guitar:         return utf8 (textGuitar);
        case Topic::bass:           return utf8 (textBass);
        case Topic::bowed:          return utf8 (textBowed);
        case Topic::brass:          return utf8 (textBrass);
        case Topic::woodwinds:      return utf8 (textWoodwinds);
        case Topic::voice:          return utf8 (textVoice);
        case Topic::synths:         return utf8 (textSynths);
        case Topic::drumKit:        return utf8 (textDrumKit);
        case Topic::orchestration:  return utf8 (textOrchestration);
        case Topic::realMidi:       return utf8 (textRealMidi);
        case Topic::yours:          return utf8 (textYoursIntro) + "\n\n" + (yourTrackText.isEmpty() ? juce::String (textNoTrack) : yourTrackText);
    }

    return {};
}

//==============================================================================
juce::String getFamilyName (Family f)
{
    switch (f)
    {
        case Family::keys:      return "Keys";
        case Family::guitar:    return "Guitar";
        case Family::bass:      return "Bass";
        case Family::strings:   return "Strings";
        case Family::brass:     return "Brass";
        case Family::woodwinds: return "Woodwinds";
        case Family::voice:     return "Voice";
        case Family::synths:    return "Synths";
        case Family::drums:     return "Drums and percussion";
    }

    return {};
}

Topic getFamilyTopic (Family f)
{
    switch (f)
    {
        case Family::keys:      return Topic::keys;
        case Family::guitar:    return Topic::guitar;
        case Family::bass:      return Topic::bass;
        case Family::strings:   return Topic::bowed;
        case Family::brass:     return Topic::brass;
        case Family::woodwinds: return Topic::woodwinds;
        case Family::voice:     return Topic::voice;
        case Family::synths:    return Topic::synths;
        case Family::drums:     return Topic::drumKit;
    }

    return Topic::makeSound;
}

const std::vector<Info>& getInstruments()
{
    using I = Instrument;
    using F = Family;

    static const std::vector<Info> table {
        makeInfo (I::piano,         F::keys,      "Piano",            21, 108, 10, false, false,  0, "1-8", "piano,keys,rhodes"),
        makeInfo (I::organ,         F::keys,      "Organ",            -1,  -1, 12, false, true,   0, "17-24", "organ"),
        makeInfo (I::guitar,        F::guitar,    "Guitar",           40,  86,  6, false, false, 12, "25-32", "guitar,gtr"),
        makeInfo (I::bassGuitar,    F::bass,      "Bass guitar",      28,  67,  2, false, false, 12, "33-40", "bass,bassline", 23),
        makeInfo (I::violin,        F::strings,   "Violin",           55, 100,  2, false, true,   0, "41", "violin,fiddle"),
        makeInfo (I::viola,         F::strings,   "Viola",            48,  84,  2, false, true,   0, "42", "viola"),
        makeInfo (I::cello,         F::strings,   "Cello",            36,  76,  2, false, true,   0, "43", "cello"),
        makeInfo (I::doubleBass,    F::strings,   "Double bass",      28,  59,  2, false, true,  12, "44", "double bass,contrabass,upright"),
        makeInfo (I::stringSection, F::strings,   "String section",   28, 100,  0, false, true,   0, "45, 46, 49-52", "strings,string"),
        makeInfo (I::harp,          F::strings,   "Harp",             23, 104,  8, false, false,  0, "47", "harp"),
        makeInfo (I::trumpet,       F::brass,     "Trumpet",          52,  82,  1, true,  true,   2, "57, 60", "trumpet,tpt"),
        makeInfo (I::frenchHorn,    F::brass,     "French horn",      35,  77,  1, true,  true,   7, "61", "horn,french horn"),
        makeInfo (I::trombone,      F::brass,     "Trombone",         40,  70,  1, true,  true,   0, "58", "trombone,tbn"),
        makeInfo (I::tuba,          F::brass,     "Tuba",             22,  72,  1, true,  true,   0, "59", "tuba"),
        makeInfo (I::brassSection,  F::brass,     "Brass section",    35,  82,  0, true,  true,   0, "62-64", "brass"),
        makeInfo (I::flute,         F::woodwinds, "Flute",            60,  96,  1, true,  true,   0, "74", "flute"),
        makeInfo (I::oboe,          F::woodwinds, "Oboe",             58,  89,  1, true,  true,   0, "69", "oboe"),
        makeInfo (I::clarinet,      F::woodwinds, "Clarinet",         50,  94,  1, true,  true,   2, "72", "clarinet"),
        makeInfo (I::bassoon,       F::woodwinds, "Bassoon",          34,  76,  1, true,  true,   0, "71", "bassoon"),
        makeInfo (I::altoSax,       F::woodwinds, "Alto saxophone",   49,  80,  1, true,  true,   9, "66", "sax,saxophone,alto sax"),
        makeInfo (I::tenorSax,      F::woodwinds, "Tenor saxophone",  44,  75,  1, true,  true,  14, "67", "tenor sax"),
        makeInfo (I::voice,         F::voice,     "Voice",            40,  84,  1, true,  true,   0, "", "vocal,vocals,vox,voice,singer,bv,backing"),
        makeInfo (I::choir,         F::voice,     "Choir",            40,  84,  0, true,  true,   0, "53-55", "choir"),
        makeInfo (I::synth,         F::synths,    "Synth",            -1,  -1,  0, false, false,  0, "81-96", "synth,lead,pad,pluck"),
        makeInfo (I::drumKit,       F::drums,     "Drum kit",         35,  81,  4, false, false,  0, "bank 128", "drum,drums,kick,snare,hats,hi-hat,perc,percussion")
    };

    return table;
}

const Info& getInfo (Instrument i)
{
    return getInstruments()[(size_t) i];
}

bool isDrumKit (Instrument i) { return i == Instrument::drumKit; }

double getNoteHz (int note) { return 440.0 * std::pow (2.0, (note - 69) / 12.0); }

juce::String getSentenceName (const Info& info)
{
    return info.name.toLowerCase().replace ("french", "French");
}

//==============================================================================
Guess guessInstrument (const Hints& h)
{
    Guess g;

    // 1. The DawIt Sampler preset: bank 128 is a drum kit; bank 0 programs are General MIDI.
    if (h.hasSampler)
    {
        if (h.samplerBank == 128)
        {
            g = { Instrument::drumKit, GuessSource::sampler, h.samplerPreset };
            return g;
        }

        for (auto& r : programRanges)
        {
            if (h.samplerProgram >= r.from && h.samplerProgram <= r.to)
            {
                g = { r.instrument, GuessSource::sampler, h.samplerPreset };
                return g;
            }
        }
    }

    // 2. The track's name.
    if (const auto byName = guessFromName (h.trackName))
    {
        g = { *byName, GuessSource::name, {} };
        return g;
    }

    // 3. The DawIt sound: a DawIt instrument, or the built-in synth's sound.
    if (h.dawItInstrument.isNotEmpty())
    {
        const auto name = h.dawItInstrument.toLowerCase();

        if (name == "dawit drums")
            g = { Instrument::drumKit, GuessSource::plugin, h.dawItInstrument };
        else if (name == "dawit subtractive" || name == "dawit additive" || name == "dawit fm")
            g = { Instrument::synth, GuessSource::plugin, h.dawItInstrument };

        return g;
    }

    if (h.builtInSound.isNotEmpty())
    {
        const auto sound = h.builtInSound.toLowerCase();
        std::optional<Instrument> instrument;

        if (sound == "piano")
            instrument = Instrument::piano;
        else if (sound == "organ")
            instrument = Instrument::organ;
        else if (sound == "bass")
            instrument = Instrument::bassGuitar;
        else if (sound == "lead" || sound == "pluck" || sound == "pad")
            instrument = Instrument::synth;
        else if (sound == "drums" || sound == "percussion")
            instrument = Instrument::drumKit;

        if (instrument.has_value())
            g = { *instrument, GuessSource::builtInSound, capitalised (sound) };
    }

    return g;
}

//==============================================================================
std::vector<double> getHarmonics (double fundamental, int count)
{
    std::vector<double> result;

    for (int n = 1; n <= count; ++n)
        result.push_back (fundamental * n);

    return result;
}

double getOpenTubeHz (double lengthMetres, int harmonic, double speedOfSound)
{
    return harmonic * speedOfSound / (2.0 * lengthMetres);
}

double getClosedTubeHz (double lengthMetres, int oddHarmonic, double speedOfSound)
{
    return oddHarmonic * speedOfSound / (4.0 * lengthMetres);
}

std::vector<double> getMembraneRatios()
{
    // Zeros of the Bessel functions J0, J1, J2, J0, J3 and J1 (the first of each order, then the second of J0 and J1).
    static constexpr double zeros[] { 2.404826, 3.831706, 5.135622, 5.520078, 6.380162, 7.015587 };
    std::vector<double> ratios;

    for (auto z : zeros)
        ratios.push_back (z / zeros[0]);

    return ratios;
}

std::vector<double> getFreeBarRatios()
{
    // The roots of the free-free beam equation: 4.730041, 7.853205 and 10.995608; the frequencies go with their squares.
    static constexpr double roots[] { 4.730041, 7.853205, 10.995608 };
    std::vector<double> ratios;

    for (auto r : roots)
        ratios.push_back ((r * r) / (roots[0] * roots[0]));

    return ratios;
}

double getHarmonicCentsFromEqual (int harmonic)
{
    const double cents = 1200.0 * std::log2 ((double) harmonic);
    return cents - 100.0 * std::round (cents / 100.0);
}

double getTrumpetHarmonicHz (int harmonic) { return 116.54 * harmonic; }

double getMillisecondsAsBeats (double ms, double bpm) { return ms / 1000.0 * bpm / 60.0; }

//==============================================================================
Example getExample (Topic t)
{
    Example e;

    switch (t)
    {
        case Topic::makeSound:
            e.labels = { "String", "Air column", "Reed", "Lips", "Membrane", "Bar", "Vocal folds", "Speaker cone" };
            e.details = { "Guitar, violin, piano", "Flute, organ pipe", "Clarinet, oboe, saxophone", "Trumpet, trombone, tuba",
                          "Drum, timpani", "Xylophone, marimba, vibraphone", "Voice", "Synth, electric guitar through an amp" };
            break;

        case Topic::strings:
            e.labels = { "A2 string: its first four shapes", "A4 string: whole, half, two thirds" };
            e.numbers = { getHarmonics (110.0, 4), { 440.0, 880.0, 660.0 } };
            break;

        case Topic::tubes:
        {
            const double length = 0.66;
            e.labels = { "Open at both ends", "Closed at one end" };
            e.numbers = { { getOpenTubeHz (length, 1), getOpenTubeHz (length, 2), getOpenTubeHz (length, 3) },
                          { getClosedTubeHz (length, 1), getClosedTubeHz (length, 3), getClosedTubeHz (length, 5) } };
            break;
        }

        case Topic::drumsBars:
            e.labels = { "String harmonics", "Drum skin", "Free bar", "Marimba bar" };
            e.numbers = { { 1.0, 2.0, 3.0, 4.0 }, getMembraneRatios(), getFreeBarRatios(), { 1.0, 4.0 } };
            break;

        case Topic::keys:
        {
            e.ranges = { makeBar (Instrument::piano) };
            e.numbers = { { getNoteHz (21), getNoteHz (108) } };
            ExampleLine chord;
            chord.label = "C major, two hands";

            for (int p : { 36, 43, 64, 67, 72 })
                chord.notes.push_back (note (p, 0.0, 4.0, 90));

            e.lines = { chord };
            break;
        }

        case Topic::guitar:
        {
            e.strumMs = 15.0;
            e.ranges = { makeBar (Instrument::guitar, { 40, 45, 50, 55, 59, 64 }) };
            const double gap = getMillisecondsAsBeats (e.strumMs, e.bpm);
            ExampleLine strum;
            strum.label = "Down and up strum";
            int i = 0;

            for (int p : { 40, 47, 52, 56, 59, 64 })
            {
                strum.notes.push_back (note (p, gap * i, 2.5, 96 - 2 * i));
                ++i;
            }

            i = 0;

            for (int p : { 64, 59, 56, 52 })
            {
                strum.notes.push_back (note (p, 2.5 + gap * i, 1.5, 72));
                ++i;
            }

            e.lines = { strum };
            break;
        }

        case Topic::bass:
        {
            e.ranges = { makeBar (Instrument::bassGuitar, { 28, 33, 38, 43 }) };
            ExampleLine line;
            line.label = "Root, fifth, octave and an approach note";
            line.chordLabels = { { 0.0, "C" }, { 4.0, "D" } };
            const int pitches[] { 36, 43, 48, 49, 50, 45, 38 };
            const int velocities[] { 100, 84, 92, 80, 100, 84, 92 };

            for (int i = 0; i < 7; ++i)
                line.notes.push_back (note (pitches[i], (double) i, i == 6 ? 2.0 : 1.0, velocities[i]));

            e.lines = { line };
            break;
        }

        case Topic::bowed:
            e.ranges = { makeBar (Instrument::violin, { 55, 62, 69, 76 }), makeBar (Instrument::viola, { 48, 55, 62, 69 }),
                         makeBar (Instrument::cello, { 36, 43, 50, 57 }), makeBar (Instrument::doubleBass, { 28, 33, 38, 43 }) };
            break;

        case Topic::brass:
        {
            e.ranges = { makeBar (Instrument::trumpet), makeBar (Instrument::frenchHorn), makeBar (Instrument::trombone), makeBar (Instrument::tuba) };
            std::vector<double> hz, cents;

            for (int n = 2; n <= 8; ++n)
            {
                hz.push_back (getTrumpetHarmonicHz (n));
                cents.push_back (getHarmonicCentsFromEqual (n));
            }

            e.labels = { utf8 ("B\xE2\x99\xAD trumpet harmonics 2 to 8 (Hz)"), "Cents from equal temperament" };
            e.numbers = { hz, cents };
            break;
        }

        case Topic::woodwinds:
            e.ranges = { makeBar (Instrument::flute), makeBar (Instrument::oboe), makeBar (Instrument::clarinet), makeBar (Instrument::bassoon),
                         makeBar (Instrument::altoSax), makeBar (Instrument::tenorSax) };
            break;

        case Topic::voice:
            for (auto& v : singing::getVoiceTypes())
                e.ranges.push_back ({ v.name, v.lowest, v.highest, Family::voice, {} });

            break;

        case Topic::synths:
        {
            ExampleLine lead;
            lead.label = "Lead: a bend into a note, and a glide";
            lead.notes = { note (69, 0.0, 2.0, 90), note (72, 2.5, 1.0, 90), note (74, 3.4, 1.0, 90) };
            lead.bend = { { 0.0, 8192 }, { 1.0, 8192 }, { 1.25, 16383 }, { 2.0, 16383 }, { 2.25, 8192 } };
            e.lines = { lead };
            e.glideMs = 100;
            break;
        }

        case Topic::drumKit:
        {
            ExampleLine beat;
            beat.label = "Rock beat";

            for (int i = 0; i < 8; ++i)
                beat.notes.push_back (note (42, 0.5 * i, 0.25, i % 2 == 0 ? 105 : 70));

            beat.notes.push_back (note (36, 0.0, 0.25, 110));
            beat.notes.push_back (note (36, 2.0, 0.25, 110));
            beat.notes.push_back (note (38, 1.0, 0.25, 115));
            beat.notes.push_back (note (38, 3.0, 0.25, 115));
            beat.notes.push_back (note (38, 1.75, 0.25, 30));
            beat.notes.push_back (note (38, 3.75, 0.25, 30));
            e.lines = { beat };
            break;
        }

        case Topic::orchestration:
        {
            for (auto i : { Instrument::piano, Instrument::guitar, Instrument::bassGuitar, Instrument::violin, Instrument::viola, Instrument::cello,
                            Instrument::doubleBass, Instrument::harp, Instrument::trumpet, Instrument::frenchHorn, Instrument::trombone,
                            Instrument::tuba, Instrument::flute, Instrument::oboe, Instrument::clarinet, Instrument::bassoon,
                            Instrument::altoSax, Instrument::tenorSax, Instrument::voice })
                e.ranges.push_back (makeBar (i));

            ExampleLine violins, flute;
            violins.label = "Violins";
            flute.label = "Flute, an octave above";
            const int melody[] { 76, 74, 72, 74 };

            for (int i = 0; i < 4; ++i)
            {
                violins.notes.push_back (note (melody[i], (double) i, 1.0, 90));
                flute.notes.push_back (note (melody[i] + 12, (double) i, 1.0, 90));
            }

            e.lines = { violins, flute };
            break;
        }

        case Topic::realMidi:
        {
            e.bpm = 90.0;
            ExampleLine typed, played;
            typed.label = "Typed";
            played.label = "Played";
            const int pitches[] { 72, 74, 76, 79, 76 };
            const int velocities[] { 84, 90, 96, 104, 92 };

            for (int i = 0; i < 5; ++i)
            {
                typed.notes.push_back (note (pitches[i], (double) i, i == 4 ? 3.6 : 0.9, 100));
                played.notes.push_back (note (pitches[i], (double) i, i == 4 ? 4.0 : 1.05, velocities[i]));
            }

            played.expression = { { 4.0, 70 }, { 6.0, 110 }, { 8.0, 80 } };
            e.lines = { typed, played };
            break;
        }

        case Topic::yours:
            break;
    }

    return e;
}

//==============================================================================
Report analyse (const TrackPart& part, std::optional<Instrument> chosen, const Hints& hints)
{
    Report r;
    r.numNotes = (int) part.notes.size();
    r.hasNotes = ! part.notes.empty();

    const auto finish = [&r]
    {
        r.text = r.paragraphs.joinIntoString ("\n\n");
        return r;
    };

    // 1. No track with notes.
    if (! part.hasTrack || part.notes.empty())
    {
        r.paragraphs.add (textNoTrack);
        return finish();
    }

    Guess guess;

    if (chosen.has_value())
        guess = { chosen, GuessSource::chosen, {} };
    else
        guess = guessInstrument (hints);

    // 2. Unknown.
    if (! guess.instrument.has_value())
    {
        r.paragraphs.add ("DawIt can't tell which instrument \"" + part.trackName + "\" is for. Choose one above. A DawIt Sampler preset, or a track name such as Cello, "
                          "lets DawIt guess.");
        return finish();
    }

    r.instrument = guess.instrument;
    r.source = guess.source;
    const auto& info = getInfo (*guess.instrument);
    const auto name = getSentenceName (info);
    const bool drums = isDrumKit (info.instrument);

    // 3. Identity.
    juce::String how;

    switch (guess.source)
    {
        case GuessSource::sampler:      how = "from its DawIt Sampler preset, " + guess.detail; break;
        case GuessSource::name:         how = "from its name"; break;
        case GuessSource::builtInSound:
        case GuessSource::plugin:       how = "from its DawIt sound, " + guess.detail; break;
        case GuessSource::chosen:       how = "chosen above"; break;
        case GuessSource::none:         break;
    }

    r.paragraphs.add ("Checking \"" + part.trackName + "\" as " + name + " (" + how + "). " + juce::String (r.numNotes) + (r.numNotes == 1 ? " note." : " notes."));

    auto sorted = part.notes;
    std::stable_sort (sorted.begin(), sorted.end(), [] (auto& a, auto& b) { return a.startBeat < b.startBeat; });

    int problems = 0;

    // 4. Below the lowest note (for drums: outside the drum map, either side).
    if (info.lowest >= 0)
    {
        int firstBar = 0, firstPitch = 0, lowestPitch = 128;

        for (auto& n : sorted)
        {
            const bool outside = drums ? (n.pitch < info.lowest || n.pitch > info.highest) : n.pitch < info.lowest;

            if (outside)
            {
                if (r.below == 0)
                {
                    firstBar = n.bar;
                    firstPitch = n.pitch;
                }

                ++r.below;
                lowestPitch = juce::jmin (lowestPitch, n.pitch);
            }
        }

        r.firstBelowBar = firstBar;

        if (r.below > 0)
        {
            ++problems;

            if (drums)
            {
                r.paragraphs.add (countNotes (r.below, "is", "are") + " on numbers with no General MIDI drum (the first in bar " + juce::String (firstBar) + ", note "
                                  + juce::String (firstPitch) + "): most drum sounds play nothing there.");
            }
            else
            {
                auto text = countNotes (r.below, "is", "are") + " below the " + name + "'s lowest note, " + midiguide::getNoteName (info.lowest) + " (the first in bar "
                            + juce::String (firstBar) + ", " + midiguide::getNoteName (firstPitch) + "). Try them an octave up, or give them to a lower instrument.";

                if (info.fiveStringLowest >= 0 && lowestPitch >= info.fiveStringLowest)
                    text += " A five-string bass reaches down to " + midiguide::getNoteName (info.fiveStringLowest) + ".";

                r.paragraphs.add (text);
            }
        }
    }

    // 5. Above the top of the range (drums are covered by the rule above).
    if (info.lowest >= 0 && ! drums)
    {
        int firstBar = 0, firstPitch = 0;

        for (auto& n : sorted)
        {
            if (n.pitch > info.highest)
            {
                if (r.above == 0)
                {
                    firstBar = n.bar;
                    firstPitch = n.pitch;
                }

                ++r.above;
            }
        }

        r.firstAboveBar = firstBar;

        if (r.above > 0)
        {
            ++problems;
            r.paragraphs.add (countNotes (r.above, "is", "are") + " above the top of the " + name + "'s range, " + midiguide::getNoteName (info.highest) + " (the first in bar "
                              + juce::String (firstBar) + ", " + midiguide::getNoteName (firstPitch) + "). Try them an octave lower, or give them to a higher instrument.");
        }
    }

    // 6. Too many at once: notes starting within 0.05 beats of the one before count as one moment (a strum is one).
    if (info.atOnce > 0)
    {
        constexpr double together = 0.05 + 1.0e-9;
        int firstBar = 0;
        size_t i = 0;

        while (i < sorted.size())
        {
            size_t end = i + 1;

            while (end < sorted.size() && sorted[end].startBeat - sorted[end - 1].startBeat <= together)
                ++end;

            if ((int) (end - i) > info.atOnce)
            {
                if (r.togetherMoments == 0)
                    firstBar = sorted[i].bar;

                ++r.togetherMoments;
            }

            i = end;
        }

        if (r.togetherMoments > 0)
        {
            ++problems;
            juce::String reason;

            switch (info.instrument)
            {
                case Instrument::guitar:        reason = "A guitar has six strings."; break;
                case Instrument::piano:         reason = "Two hands play ten notes at most."; break;
                case Instrument::organ:         reason = "Two hands play ten notes, and the feet one or two pedal notes: twelve at once at most."; break;
                case Instrument::drumKit:       reason = "A drummer has two hands and two feet: four hits at once at most."; break;
                case Instrument::harp:          reason = "A harpist plucks with the thumb and three fingers of each hand: eight notes at once at most."; break;
                case Instrument::bassGuitar:
                case Instrument::violin:
                case Instrument::viola:
                case Instrument::cello:
                case Instrument::doubleBass:    reason = "A " + name + " plays one or two notes at a time."; break;
                case Instrument::stringSection:
                case Instrument::trumpet:
                case Instrument::frenchHorn:
                case Instrument::trombone:
                case Instrument::tuba:
                case Instrument::brassSection:
                case Instrument::flute:
                case Instrument::oboe:
                case Instrument::clarinet:
                case Instrument::bassoon:
                case Instrument::altoSax:
                case Instrument::tenorSax:
                case Instrument::voice:
                case Instrument::choir:
                case Instrument::synth:         reason = "A " + name + " plays one note at a time: give the others to another track."; break;
            }

            r.paragraphs.add ("In " + juce::String (r.togetherMoments) + (r.togetherMoments == 1 ? " place, " : " places, ") + juce::String (info.atOnce + 1)
                              + " or more notes start together (the first in bar " + juce::String (firstBar) + "). " + reason);
        }
    }

    // 7. Breath: the longest run of notes with no gap of 0.3 seconds or more.
    if (info.breathes)
    {
        auto bySeconds = part.notes;
        std::stable_sort (bySeconds.begin(), bySeconds.end(), [] (auto& a, auto& b) { return a.startSeconds < b.startSeconds; });

        double runStart = bySeconds[0].startSeconds, runEnd = runStart + bySeconds[0].lengthSeconds;
        int runBar = bySeconds[0].bar, longestBar = runBar;
        double longest = 0.0;

        const auto closeRun = [&]
        {
            if (runEnd - runStart > longest)
            {
                longest = runEnd - runStart;
                longestBar = runBar;
            }
        };

        for (size_t k = 1; k < bySeconds.size(); ++k)
        {
            auto& n = bySeconds[k];

            if (n.startSeconds - runEnd >= 0.3 - 1.0e-9)
            {
                closeRun();
                runStart = n.startSeconds;
                runEnd = runStart + n.lengthSeconds;
                runBar = n.bar;
            }
            else
            {
                runEnd = juce::jmax (runEnd, n.startSeconds + n.lengthSeconds);
            }
        }

        closeRun();
        r.longestPhraseSeconds = longest;
        r.longestPhraseBar = longestBar;

        if (longest > 8.0)
        {
            ++problems;
            r.breathMissing = true;
            r.paragraphs.add ("The longest phrase, from bar " + juce::String (longestBar) + ", runs " + describeSeconds (longest) + " seconds with no gap to breathe. Leave a short "
                              "rest, a third of a second or more, every few bars.");
        }
    }

    // 8. Velocity: eight or more notes that all have about the same velocity.
    if (r.numNotes >= 8)
    {
        int lowestVelocity = 127, highestVelocity = 0;
        double sum = 0.0;

        for (auto& n : part.notes)
        {
            lowestVelocity = juce::jmin (lowestVelocity, n.velocity);
            highestVelocity = juce::jmax (highestVelocity, n.velocity);
            sum += n.velocity;
        }

        if (highestVelocity - lowestVelocity <= 4)
        {
            ++problems;
            r.flatVelocity = true;
            r.paragraphs.add ("All " + juce::String (r.numNotes) + " notes have about the same velocity (" + juce::String ((int) std::round (sum / r.numNotes)) + "). Vary it: "
                              + (drums ? "accents on the hi-hat, soft ghost notes on the snare" : "stronger on the main beats and at the top of a phrase, softer between") + ".");
        }
    }

    // 9. Swell: long notes that stay at one level, with no mod wheel, volume or expression in the track.
    if (info.swells && ! part.hasLoudnessController)
    {
        for (auto& n : part.notes)
            if (n.lengthSeconds >= 2.0 - 1.0e-9)
                ++r.longFlatNotes;

        if (r.longFlatNotes >= 3)
        {
            ++problems;
            r.paragraphs.add (juce::String (r.longFlatNotes) + " notes last 2 seconds or more and stay at one level. Draw a swell in the Expression lane (or the Mod wheel lane, "
                              "which many sampled strings and brass use for loudness).");
        }
        else
        {
            r.longFlatNotes = 0;
        }
    }

    // 10. All well.
    if (problems == 0)
    {
        r.nothingToPoint = true;
        r.paragraphs.add ("Nothing to point out: every note is in the " + name + "'s range and playable. What makes MIDI sound real has more to try.");
    }

    return finish();
}
} // namespace daw::gen::instruments
