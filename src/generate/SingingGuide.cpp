#include "SingingGuide.h"
#include "SingAlong.h"

namespace daw::gen::singing
{
namespace
{
    const char* const voiceText = R"txt(Your voice is an instrument with three parts: a power source, something that vibrates, and a body that shapes the sound. A guitar has your fingers, the strings and its wooden body. Your voice has your breath, the vocal folds and your throat and mouth.

The power is the air from your lungs. The vocal folds (also called vocal cords) are two small bands of muscle and soft tissue in your larynx, the voice box behind your Adam's apple. They're small: roughly 1.7 to 2.5 cm long in adult men and 1.2 to 1.7 cm in adult women. When you sing, they close, the air pushes them apart, and they spring back, over and over. Each time they open, a puff of air goes up into your throat, and the puffs make a buzz.

The number of puffs a second is the pitch. To sing the A below middle C (A3, 220 Hz), your folds open and close 220 times a second; for the A above middle C (A4, 440 Hz), 440 times. When we speak, most men's voices sit around 90 to 155 Hz and most women's around 165 to 255 Hz. Longer, heavier folds vibrate more slowly, which is why men's voices are usually lower. For a higher note, small muscles stretch the folds, like tightening a guitar string.

The buzz on its own sounds thin, like a reed without its instrument. Your throat, mouth and nose are a resonator: they make some of the buzz's harmonics louder and others softer. Their shape, set by your tongue, jaw and lips, is what makes one vowel different from another. In the picture, the same 220 Hz buzz is shaped two ways. For "ah" the strong areas (formants) are both fairly low, around 700 and 1,100 Hz; for "ee" one is very low and one high, around 300 and 2,300 Hz. These are a man's; a woman's are a little higher. Same note, same folds, a different shape: a different vowel.

So singing well is three things working together: a steady flow of air, folds that vibrate freely without squeezing, and a relaxed, open throat and mouth. The next topics take them one at a time. Click the picture to hear the A3.)txt";

    const char* const breathingText = R"txt(Singing runs on breath, so it starts with how you stand and how you breathe.

Stand (or sit up) tall and loose: feet about hip-width apart, knees not locked, shoulders down and relaxed, chin level. Don't push your chin forward or up to reach a high note. Slumping squeezes the room your lungs need, and stiffness, in the neck and jaw most of all, reaches the folds.

Breathe low. The main breathing muscle is the diaphragm, a dome under your lungs. When it tightens, it flattens and moves down, the lungs fill, and your belly and lower ribs move out. That's why teachers say "breathe into your belly": the air goes into your lungs, but the belly is where you see it. If your shoulders rise when you breathe in, you're breathing high, into the top of your chest, which takes in less air and tightens your neck. Try it: put a hand on your belly and breathe in slowly through your nose. The hand should move out and the shoulders stay still.

Breath support is what singers call letting the air out steadily rather than in one rush. To practise, breathe in low and let the air out on a long "sss", as evenly as you can, without forcing it at the end. With practice it lasts longer and stays even, and a long note needs exactly that.

Plan your breaths. A line is easier when you know where you'll breathe: in the rests between lines, not in the middle of a word. In the picture, at 96 BPM, the first line has a 2-beat rest after it, 1.25 seconds: time for a full breath. The second has half a beat, about 0.3 seconds: a quick, small breath through the mouth. The third runs for 10 beats, about 6.3 seconds, so breathe in deeply before it, or take a quick breath after "head". Mark your breaths on the lyrics and practise them like the words.

Running out of breath often shows as the end of a line going flat or quiet. If that happens, breathe earlier, before you need to. The lyrics guide's Singing along looks at your song's longest line and its shortest rest.)txt";

    const char* const warmupText = R"txt(Runners warm up before they run, and singers before they sing: a few minutes of gentle exercises get the breath going and the folds moving freely, so the first take isn't the warm-up. Many teachers suggest 5 to 10 minutes. Research hasn't settled the best length, so go by feel: start easy, and stop if anything hurts.

A simple routine, from low and soft to higher and louder:
1. Loosen up: roll your shoulders, yawn, and rub your jaw muscles.
2. Hum on a comfortable note, lips together and teeth apart, until you feel a buzz on your lips and nose.
3. Lip trills: blow through loose lips so they flutter, like a child making an engine sound, and sing through it. If your lips won't flutter, press a fingertip into each cheek, or hum through a drinking straw instead.
4. Five-note scales on a trill or a hum: up five notes and back down (1-2-3-4-5-4-3-2-1), then start a semitone higher and do it again, for as long as it stays easy. {start}
5. Sirens: slide smoothly from low to high and back, like a siren, on a trill, on "ng" (as at the end of "sing") or on "oo". Let the voice change gears on its own; don't push through the change.
6. Last, sing a few lines of the song itself, at half volume.

Humming, lip trills and singing through a straw all partly close the mouth. That holds back a little of the air, and the gentle pressure helps the folds vibrate with less effort, which is why voice teachers and voice clinics use them.

Short practice on most days does more than a long session once a week, and a warmed-up voice is also the one to find your range with (Voice types and range).)txt";

    const char* const rangeText = R"txt(Your range is the span from the lowest to the highest note you can sing well: not just reach, but sing with a usable tone. Singers are grouped into voice types by where their voices sit. These are the usual ranges of trained classical singers, as voice-teaching books give them:
  Soprano: C4 to C6 (from middle C up two octaves)
  Mezzo-soprano: A3 to A5
  Alto (contralto): F3 to F5
  Tenor: C3 to C5
  Baritone: A2 to A4
  Bass: E2 to E4

Other books give ranges a few notes different, so take these as a guide, not a rule. Each spans about two octaves. Men's voices are usually bass, baritone or tenor, and women's alto, mezzo-soprano or soprano, and the two groups overlap: a tenor and an alto share most of their notes. Baritone is the most common man's voice, and most people have a medium voice rather than the highest or the lowest.

Range isn't everything. A voice type also depends on where your voice is comfortable most of the time (its tessitura, Italian for "texture") and on its colour: a tenor and a baritone can both sing an A3, but it sounds lighter from one and darker from the other. A beginner's comfortable range is often narrower than these, and it grows with practice, so don't rush to give yourself a label. Teachers often wait months before they decide.

Find your range by ear, warmed up. Click a key in the picture to hear it and sing it on "ah". Go down a note at a time until your voice gets breathy, croaky or very quiet: the last note that still sounded good is your lowest. Then go up from the middle until you'd have to strain or shout: the last easy note is your highest. Don't push past discomfort. Right-click a key to set it as your lowest or highest note, or pick a voice type above the picture. DawIt remembers your range for every song, and Your voice and your song checks your melody against it.)txt";

    const char* const registersText = R"txt(As you sing from low to high, your voice changes gears. Each gear is a register: a group of notes that feel and sound alike, because the folds vibrate in a similar way.

Chest voice is the full, low register you speak in. You feel it buzz in your chest, which gives it its name, but the sound is made by the folds, which vibrate thick and with firm closure. Head voice is the higher, lighter register: the folds are stretched thinner, and you feel it more in your face and head. Falsetto is lighter and breathier still, with the folds barely closing: the soft high notes of many soul and pop singers.

The break (in Italian, passaggio) is where one register hands over to the next. Sing a slow siren from low to high and your voice may crack or flip there, or suddenly get lighter. For many men the main break comes somewhere between C4 and A4: around E4 for a baritone and G4 for a tenor. For many women, chest voice gives way somewhere around E4 to A4, with another change higher up, around E5 to F#5. These are typical places, not rules: yours may be a few notes away. The picture shows a tenor and a mezzo-soprano.

Mixed voice, or the mix, is what singers call blending the registers around the break, so the voice carries on up without a crack or a sudden change of colour. Teachers explain it in different ways, and some avoid the word, but the goal is the same: one even voice from bottom to top. It comes with practice: sirens across the break, five-note scales that cross it, and singing a little lighter as you go up, rather than pushing chest voice higher.

Pushing chest voice up and louder is the most common way beginners strain: it feels powerful, but the throat tightens and the voice tires within minutes. If a note above your break only works by shouting, sing it lighter, or move the song's key down (Your voice and your song suggests how far).)txt";

    const char* const tuneText = R"txt(Singing in tune means your note has the pitch it should have. Differences this small are measured in cents: a semitone, one key on the piano, is 100 cents. Many listeners start to hear a sung note as out of tune when it's about 20 to 25 cents off, a fifth to a quarter of a semitone. People differ a lot, and trained musicians hear smaller differences.

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

DawIt Tune, a built-in effect, shows how many cents each sung note is off while the song plays, and can pull notes into tune. Its guide, What is pitch correction?, explains how.)txt";

    const char* const vowelsText = R"txt(The vowels carry your voice; the consonants carry the words. Good singing gives each its job.

Long notes go on vowels. Sing "night" on a long note and the "ay" gets nearly all of it: start the n just before the beat, hold the vowel, and put the t at the very end. Holding a consonant instead, such as the "ng" of "sing" or the "l" of "feel", makes a hum that's hard to sing and hard to hear. In the picture, the red line holds "sing" on its "ng"; the line under it holds the "ih" and closes with the "ng" only at the end.

Open vowels such as "ah" (father), "oh" and "ay" (day) are the easiest to hold and to sing high. On high notes, singers open a closed vowel a little: "ee" towards "ih", "oo" towards "oh". The word still comes across, and the throat stays open. This is called vowel modification.

Keep consonants short, clear and a little early: the consonant just before the beat, the vowel on it. Then the word lands in time, as the lyrics guide's Words on a melody and How a computer sings show. End consonants, the t of "night" and the d of "hold", should be crisp but quick, and at the same moment when several voices sing.

Many vowels are two in one (diphthongs): the "ay" of "night" is "ah" sliding to "ee". Hold the first part and move to the second only at the end of the note: "naaah-eet", not "na-eeee".

Speak the line first, then sing it the way you'd say it, a little exaggerated. A word that's still hard to sing on its note can often be changed (the lyrics guide's Writing tips).)txt";

    const char* const vibratoText = R"txt(Vibrato is a regular swing in pitch around a held note. In trained singers it swings about 5 to 7 times a second, in classical singing typically about half to three quarters of a semitone (50 to 75 cents) above and below the note. The ear doesn't hear two notes: it hears one note, at the middle of the swing, with a warmer, living tone.

A straight tone is a held note without vibrato. Pop, folk, early music and many choirs use a lot of it, and pop singers often start a long note straight and let the vibrato come in towards its end. Classical singers usually keep it through the whole note. The picture shows all three on a 2-second note.

Vibrato isn't made by shaking. In a relaxed, well-supported voice it comes by itself, from small natural pulses in the larynx. Forcing it by moving the jaw, the belly or the throat sounds mechanical. A tight voice often has none, or a fast, nervous flutter; a tired or pushed one can get a slow, wide wobble.

To practise, hold a comfortable note straight for 4 beats, then let it relax into vibrato for 4 more, without pushing. Both are tools: a straight tone sounds plain and close, vibrato warm and open, and moving between them is part of a style.)txt";

    const char* const loudText = R"txt(Singing louder comes from more air pressure under the folds and folds that close more firmly, not from a tighter throat. When you get louder by squeezing, the neck tightens, the chin pushes forward, and the voice soon gets hoarse.

Soft singing is harder than it sounds. It needs the same steady breath with less pressure, or the note goes breathy and flat. A classic exercise is a note from soft to loud and back (messa di voce, Italian for "placing of the voice") on a comfortable pitch: the pitch should stay put while the level changes.

Belting is the loud, bright, speech-like sound of musical theatre, rock, soul and gospel, carried higher than chest voice usually goes. Done well, it's a skill like any other, built up over months, ideally with a teacher. Done by pushing chest voice up and shouting, it's the quickest way to strain your voice. Signs you're pushing: the veins in your neck stand out, your chin lifts, it scratches or hurts, or you're hoarse afterwards. Then sing it lighter, or lower the key.

Loudness is part of the song, too. Verses are usually softer and lower, choruses louder and higher; in the picture, the thicker a note, the louder it's sung, and the chorus line is 9 to 12 dB louder than the verse. Save your loudest for the moments that need it, so they stand out.

On a microphone you don't have to be loud: the mic and the fader make you loud. On big notes, lean back from the mic a little, and on soft ones come in a little (The microphone says why).)txt";

    const char* const harmonyText = R"txt(Backing vocals make a voice sound bigger. Two kinds are used most.

A harmony is a second melody that moves with the lead, on other notes of the same chords. The most common is a third above. In the picture the lead sings E4, E4, G4, E4, D4, C4 on "hold on, hold on to-night", and the harmony sings G4, G4, B4, G4, F4, E4: each note two steps up the scale. A third is 3 or 4 semitones (a minor or a major third), so a harmony follows the key, not a fixed number of keys. A third below works too, and so does a sixth below, which is the same notes as a third above, an octave lower. Harmonies usually come in the chorus, on the hook, or on the last words of lines.

A double is the lead sung again, same notes and words, as a second take on its own track. No two takes are exactly alike, and the tiny differences in timing and pitch make the sound thicker and smoother. A copy of the same take can't do that: two identical copies only make it 6 dB louder. Doubles usually sit a little quieter than the lead, or come in pairs panned left and right.

How to sing them:
- Learn the part on its own first: solo the harmony's track, or play its notes, and sing along.
- Match the lead exactly: the same vowels, the consonants at the same moments. Keep the end consonants (s, t) light on the doubles, or they pile up into a hiss or a click.
- Sing a harmony a little softer than the lead, so it blends under it.

Put the backing parts in a bus (the routing guide shows how), so one fader sets their level. Click the picture to hear the lead with its harmony.)txt";

    const char* const careText = R"txt(Your vocal folds are small and soft, and they tire like any muscle. Most singers' voice problems come from using the voice too much or too hard, and most go away with rest.

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

This guide is a start, not a replacement for a singing teacher or a doctor. A good teacher hears what you can't hear yourself.)txt";

    const char* const microphoneText = R"txt(A good home vocal recording comes down mostly to three things: how far you are from the microphone, what stops the pops, and how quiet and dead the room is. Record on an audio track with its R button; How recording works explains the interface, levels and latency.

Distance. A good starting point with a studio microphone is about 15 to 23 cm (6 to 9 inches) from your mouth, about a hand's span. Closer sounds bigger and more intimate, with less of the room; further sounds more natural but picks up more of the room and its noise. A handheld stage mic such as the Shure SM58 is made to be sung into closer, within about 15 cm.

Keep the distance steady. Sound gets quieter with distance: twice as far is about 6 dB quieter, and twice as close about 6 dB louder (a little less in a room full of echoes). In the picture, moving in from 20 cm to 10 cm makes the voice 6 dB louder, and to 5 cm 12 dB. That's why moving about while you sing makes the level jump. Use it on purpose: lean back a little on loud notes, and come in a little on soft ones.

The proximity effect. Most vocal mics are directional (cardioid: they hear the front more than the back and sides), and they add bass as you get close. Right up against it, a mic like the SM58 adds about 6 to 10 dB at 100 Hz. A little gives a warm, radio-like voice; too much sounds boomy. Omnidirectional mics, which hear all round, don't do this.

Plosives. "P" and "b", and a strong "t", push out a puff of air that hits the mic as a low thump. A pop filter, a mesh screen about 5 to 8 cm in front of the mic, breaks up the puff and keeps you from getting too close. Singing just past the mic rather than straight into it helps too. A low cut around 80 to 100 Hz when mixing removes what's left: DawIt EQ's Vocal starting point does it at 90 Hz.

The room. A small bare room adds a boxy sound that can't be taken out afterwards. Record in a room with soft things in it (a sofa, curtains, a bed), face away from bare walls, or hang a duvet behind the mic; a wardrobe full of clothes is a well-known home vocal booth. Switch off fans and anything else you can hear, and wear closed headphones so the backing track doesn't leak into the mic.

Level. Set the input so your loudest notes peak around -18 to -10 dB, never near 0: a clipped vocal can't be repaired. The gain staging guide explains why.)txt";

    const char* const yoursIntro = R"txt(Your voice and your song checks the melody you sing against your range. Pick your voice type above the picture, or set your own lowest and highest notes (Voice types and range says how to find them). DawIt remembers your range for every song. In the picture, your range is the bar at the bottom, with its top and bottom 3 semitones hatched, and each section of the song has a bar from its lowest to its highest melody note.)txt";
} // namespace

//==============================================================================
juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::voice:      return "How the voice works";
        case Topic::breathing:  return "Posture and breathing";
        case Topic::warmup:     return "Warming up";
        case Topic::range:      return "Voice types and range";
        case Topic::registers:  return "Registers and the break";
        case Topic::tune:       return "Singing in tune";
        case Topic::vowels:     return "Vowels and words";
        case Topic::vibrato:    return "Vibrato and straight tone";
        case Topic::loud:       return "Loud, soft and belting";
        case Topic::harmony:    return "Harmonies and doubles";
        case Topic::care:       return "Looking after your voice";
        case Topic::microphone: return "The microphone";
        case Topic::yours:      return "Your voice and your song";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::voice:      return "Breath, two small folds, the throat and mouth";
        case Topic::breathing:  return "Stand tall, breathe low, plan your breaths";
        case Topic::warmup:     return "Hum, lip trill and slide, a few minutes first";
        case Topic::range:      return "Bass to soprano, and finding your own range";
        case Topic::registers:  return "Chest voice, head voice, the mix, falsetto";
        case Topic::tune:       return "Hear the note first, then land on it";
        case Topic::vowels:     return "Open vowels on long notes, consonants quick";
        case Topic::vibrato:    return "A gentle swing in pitch, or none";
        case Topic::loud:       return "Power from the breath, not the throat";
        case Topic::harmony:    return "A third above, and the same part twice";
        case Topic::care:       return "Water, rest, and when to see a doctor";
        case Topic::microphone: return "Distance, the pop filter, a quiet room";
        case Topic::yours:      return "Your range against your melody, a key to try";
    }

    return {};
}

//==============================================================================
juce::String getNoteName (int midiNote)
{
    static const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((midiNote % 12) + 12) % 12]) + juce::String (midiNote / 12 - 1);
}

double getFrequency (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

const std::vector<VoiceType>& getVoiceTypes()
{
    // The usual ranges of trained classical singers (McKinney; Boldrey for the bass), 24 semitones each.
    static const std::vector<VoiceType> types { { "Bass", 40, 64 },          // E2 to E4
                                                { "Baritone", 45, 69 },      // A2 to A4
                                                { "Tenor", 48, 72 },         // C3 to C5
                                                { "Alto", 53, 77 },          // F3 to F5
                                                { "Mezzo-soprano", 57, 81 }, // A3 to A5
                                                { "Soprano", 60, 84 } };     // C4 to C6
    return types;
}

int getClosestType (Range range)
{
    if (! range.isSet())
        return -1;

    const auto& types = getVoiceTypes();
    int best = 0, bestDistance = 1 << 20;

    // Twice the middle, to stay in whole numbers; the first (lowest) of equals wins.
    for (int i = 0; i < (int) types.size(); ++i)
    {
        const int distance = std::abs ((range.lowest + range.highest) - (types[(size_t) i].lowest + types[(size_t) i].highest));

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }

    return best;
}

std::vector<int> getTypesThatFit (int lowest, int highest)
{
    std::vector<int> fits;
    const auto& types = getVoiceTypes();

    for (int i = 0; i < (int) types.size(); ++i)
        if (types[(size_t) i].lowest <= lowest && types[(size_t) i].highest >= highest)
            fits.push_back (i);

    return fits;
}

int getExactType (Range range)
{
    const auto& types = getVoiceTypes();

    for (int i = 0; i < (int) types.size(); ++i)
        if (types[(size_t) i].lowest == range.lowest && types[(size_t) i].highest == range.highest)
            return i;

    return -1;
}

juce::String getRangeLabel (Range range)
{
    if (! range.isSet())
        return "Not set";

    if (const int exact = getExactType (range); exact >= 0)
        return getVoiceTypes()[(size_t) exact].name;

    return "Custom (closest: " + getVoiceTypes()[(size_t) getClosestType (range)].name.toLowerCase() + ")";
}

//==============================================================================
bool hasVocalWord (const juce::String& trackName)
{
    static const char* vocalWords[] { "vocal", "vox", "voice", "sing", "singer", "lead", "melody", "bv", "backing", "harmony", "choir" };

    for (auto& word : juce::StringArray::fromTokens (trackName.toLowerCase(), " -_.,()/1234567890", {}))
        for (auto* v : vocalWords)
            if (word.startsWith (v))
                return true;

    return false;
}

int chooseMelodyTrack (const std::vector<TrackInfo>& tracks, int selected)
{
    if (juce::isPositiveAndBelow (selected, (int) tracks.size()) && tracks[(size_t) selected].hasNotes)
        return selected;

    for (int i = 0; i < (int) tracks.size(); ++i)
        if (tracks[(size_t) i].hasNotes && hasVocalWord (tracks[(size_t) i].name))
            return i;

    for (int i = 0; i < (int) tracks.size(); ++i)
        if (tracks[(size_t) i].hasNotes && ! tracks[(size_t) i].playsDrums)
            return i;

    return -1;
}

//==============================================================================
namespace
{
    juce::String semitones (int n)
    {
        return juce::String (n) + (n == 1 ? " semitone" : " semitones");
    }

    juce::String withArticle (const juce::String& name)
    {
        return (juce::String ("aeiou").containsChar (name.toLowerCase()[0]) ? "an " : "a ") + name.toLowerCase();
    }

    int getBarOf (double beat) { return (int) std::floor (beat / 4.0 + 1.0e-9); }

    const Section* findSection (const SectionList& sections, double beat)
    {
        const int bar = getBarOf (beat);

        for (auto& s : sections)
            if (bar >= s.startBar && bar < s.getEndBar())
                return &s;

        return nullptr;
    }

    juce::String shiftedKeyName (MusicalKey key, int shift)
    {
        key.tonic = (((key.tonic + shift) % 12) + 12) % 12;
        return key.getName();
    }

    /** The section's name for the text: "chorus", "verse 2". */
    juce::String sectionWord (const Section& s) { return s.getDisplayName().toLowerCase(); }

    /** "Try the song 2 semitones lower: in G minor instead of A minor." */
    juce::String keySuggestion (const Analysis& a, const MusicalKey& key, const juce::String& lead)
    {
        const int n = std::abs (a.shift);
        return lead + " " + semitones (n) + (a.shift < 0 ? " lower" : " higher") + ": in " + a.newKeyName + " instead of " + key.getName() + ".";
    }

    juce::String octaveSuggestion (const Analysis& a)
    {
        return juce::String ("Try singing it an octave ") + (a.shift < 0 ? "lower" : "higher") + ": then it runs from " + getNoteName (a.newLowest) + " to "
               + getNoteName (a.newHighest) + ", inside your range, and nothing in the song has to change. Many men sing songs written for women's voices an "
               "octave lower, and the other way round.";
    }

    juce::String roundedSeconds (double seconds)
    {
        // Halves round up (6.25 is 6.3), whatever the last bit of the sum.
        return juce::String (std::floor (seconds * 10.0 + 0.5 + 1.0e-6) / 10.0, 1);
    }
} // namespace

Analysis analyse (const singthis::Melody& melody, const juce::String& trackName, const SectionList& sections, const MusicalKey& key,
                  const Lyrics& lyrics, double bpm, Range range)
{
    Analysis a;
    a.trackName = trackName;
    a.range = range.isSet() ? range : Range();

    if (melody.notes.empty())
    {
        a.verdict = Analysis::Verdict::noMelody;
        a.paragraphs.add ("Your song has no melody to check yet. Write or record the tune on a MIDI track, or turn a sung recording into notes with "
                          "Convert to MIDI... in the clip's menu, then choose it as the melody above.");
        return a;
    }

    a.hasMelody = true;
    a.lowest = 127;
    a.highest = 0;
    double totalBeats = 0.0;

    for (auto& n : melody.notes)
    {
        const double length = n.endBeat - n.startBeat;
        totalBeats += length;

        if (n.pitch < a.lowest)
        {
            a.lowest = n.pitch;
            a.lowestBar = getBarOf (n.startBeat) + 1;
        }

        if (n.pitch > a.highest)
        {
            a.highest = n.pitch;
            a.highestBar = getBarOf (n.startBeat) + 1;
        }

        if (auto* s = findSection (sections, n.startBeat))
        {
            // The sections of the song in order, told apart by their place in the list.
            const auto index = (size_t) (s - sections.data());

            if (a.sections.size() <= index)
                a.sections.resize (index + 1, SectionRange { {}, SectionType::verse, 128, -1, 0.0 });

            auto& r = a.sections[index];
            r.name = s->getDisplayName();
            r.type = s->type;
            r.lowest = juce::jmin (r.lowest, n.pitch);
            r.highest = juce::jmax (r.highest, n.pitch);
            r.beats += length;
        }
    }

    // Sections with no melody in them are left out; the rest stay in song order.
    a.sections.erase (std::remove_if (a.sections.begin(), a.sections.end(), [] (const SectionRange& r) { return r.highest < 0; }), a.sections.end());

    const juce::String lowName = getNoteName (a.lowest), highName = getNoteName (a.highest);

    // The rule that comes last: the longest line, when it needs a deep breath.
    auto addLongestLine = [&]
    {
        if (const auto [index, beats] = singalong::findLongestLine (lyrics); index >= 0 && bpm > 0.0 && beats * 60.0 / bpm > 6.0)
            a.paragraphs.add ("Your longest line, \"" + singalong::getText (lyrics.lines[(size_t) index]) + "\", lasts " + roundedSeconds (beats * 60.0 / bpm)
                              + " seconds: plan a deep breath before it (Posture and breathing).");
    };

    if (! range.isSet())
    {
        a.verdict = Analysis::Verdict::noRange;
        juce::String text = "The melody (" + trackName + ") goes from " + lowName + " to " + highName + ", " + semitones (a.highest - a.lowest) + ". ";
        const auto fits = getTypesThatFit (a.lowest, a.highest);

        if (fits.empty())
            text << "That's wider than any one voice type's usual range, so most singers would find part of it hard.";
        else
        {
            juce::StringArray names;

            for (int i : fits)
                names.add (withArticle (getVoiceTypes()[(size_t) i].name));

            text << "In the usual ranges that fits ";

            for (int i = 0; i < names.size(); ++i)
                text << (i == 0 ? "" : i == names.size() - 1 ? " or " : ", ") << names[i];

            text << ". Set your own range above to check it against your voice.";
        }

        a.paragraphs.add (text);
        addLongestLine();
        return a;
    }

    const int lo = range.lowest, hi = range.highest;
    const juce::String rangeNames = getNoteName (lo) + " to " + getNoteName (hi);

    // The shifts that fit are the whole numbers from lo - L to hi - H.
    if (a.highest - a.lowest > hi - lo)
    {
        a.verdict = Analysis::Verdict::noKey;
        a.paragraphs.add ("The melody spans " + juce::String (a.highest - a.lowest) + " semitones (" + lowName + " to " + highName + "), and your range spans "
                          + juce::String (hi - lo) + " (" + rangeNames + "), so no key fits all of it. Try singing the highest phrase an octave lower, "
                          "change a few of the highest or lowest notes, or give that part to a second voice.");
        addLongestLine();
        return a;
    }

    const int minShift = lo - a.lowest, maxShift = hi - a.highest;
    a.sparesTop = hi - a.highest;
    a.sparesBottom = a.lowest - lo;

    auto setShift = [&] (int shift, bool asOctave)
    {
        a.shift = shift;
        a.octave = asOctave;
        a.newLowest = a.lowest + shift;
        a.newHighest = a.highest + shift;
        a.newKeyName = shiftedKeyName (key, shift);
    };

    auto sectionOf = [&] (double beat) -> juce::String
    {
        if (auto* s = findSection (sections, beat))
            return sectionWord (*s);

        return {};
    };

    if (a.highest > hi || a.lowest < lo)
    {
        const bool high = a.highest > hi;
        a.verdict = high ? Analysis::Verdict::tooHigh : Analysis::Verdict::tooLow;
        a.outside = high ? a.highest - hi : lo - a.lowest;

        // The note that goes outside, and where it is.
        for (auto& n : melody.notes)
            if (n.pitch == (high ? a.highest : a.lowest))
            {
                a.sectionName = sectionOf (n.startBeat);
                break;
            }

        const int closest = high ? maxShift : minShift;
        const int octaveShift = high ? -12 : 12;
        const bool useOctave = std::abs (closest) >= 5 && octaveShift >= minShift && octaveShift <= maxShift;
        setShift (useOctave ? octaveShift : closest, useOctave);

        juce::String text = (a.sectionName.isNotEmpty() ? "The " + a.sectionName : juce::String ("The melody")) + " goes " + semitones (a.outside)
                            + (high ? " above" : " below") + " your range: its " + (high ? "highest" : "lowest") + " note is "
                            + (high ? highName : lowName) + " (bar " + juce::String (high ? a.highestBar : a.lowestBar) + "), and your "
                            + (high ? "highest" : "lowest") + " is " + getNoteName (high ? hi : lo) + ". ";

        if (useOctave)
            text << octaveSuggestion (a);
        else
            text << keySuggestion (a, key, "Try the song") << " The melody then runs from " << getNoteName (a.newLowest) << " to " << getNoteName (a.newHighest) << ".";

        a.paragraphs.add (text);
        addLongestLine();
        return a;
    }

    // It fits. How much of the sung time is near the top, and near the bottom?
    double topBeats = 0.0, bottomBeats = 0.0;
    std::vector<std::pair<juce::String, double>> topIn, bottomIn;

    auto add = [] (std::vector<std::pair<juce::String, double>>& list, const juce::String& name, double beats)
    {
        for (auto& p : list)
            if (p.first == name)
            {
                p.second += beats;
                return;
            }

        list.push_back ({ name, beats });
    };

    for (auto& n : melody.notes)
    {
        const double length = n.endBeat - n.startBeat;

        if (n.pitch >= hi - 2)
        {
            topBeats += length;
            add (topIn, sectionOf (n.startBeat), length);
        }

        if (n.pitch <= lo + 2)
        {
            bottomBeats += length;
            add (bottomIn, sectionOf (n.startBeat), length);
        }
    }

    auto mostly = [] (const std::vector<std::pair<juce::String, double>>& list)
    {
        juce::String best;
        double most = 0.0;

        for (auto& p : list)
            if (p.second > most + 1.0e-9 && p.first.isNotEmpty())
            {
                most = p.second;
                best = p.first;
            }

        return best;
    };

    const double topShare = topBeats / totalBeats, bottomShare = bottomBeats / totalBeats;
    constexpr double nearShare = 0.25 - 1.0e-9;
    const juce::String fitsText = "The melody fits your range (" + rangeNames + "), but stays near the ";

    if (topShare >= nearShare)
    {
        a.verdict = Analysis::Verdict::nearTop;
        a.share = topShare;
        a.sectionName = mostly (topIn);
        juce::String text = fitsText + "top: " + juce::String (juce::roundToInt (topShare * 100.0)) + "% of the sung time is in the top 3 semitones of your range"
                            + (a.sectionName.isNotEmpty() ? ", most of it in the " + a.sectionName : juce::String()) + ". That's tiring over a whole song. ";

        if (a.lowest - 2 >= lo)
            setShift (-2, false);
        else if (a.lowest - 1 >= lo)
            setShift (-1, false);

        text << (a.shift != 0 ? keySuggestion (a, key, "Try it")
                              : juce::String ("It already uses the bottom of your range, so a key change won't help: warm up well, and sing the high notes lighter."));
        a.paragraphs.add (text);
    }
    else if (bottomShare >= nearShare)
    {
        a.verdict = Analysis::Verdict::nearBottom;
        a.share = bottomShare;
        a.sectionName = mostly (bottomIn);
        juce::String text = fitsText + "bottom: " + juce::String (juce::roundToInt (bottomShare * 100.0)) + "% of the sung time is in the bottom 3 semitones of your range"
                            + (a.sectionName.isNotEmpty() ? ", most of it in the " + a.sectionName : juce::String())
                            + ". Low notes are quiet and easily lost in a mix. ";

        if (a.highest + 2 <= hi)
            setShift (2, false);
        else if (a.highest + 1 <= hi)
            setShift (1, false);

        text << (a.shift != 0 ? keySuggestion (a, key, "Try it")
                              : juce::String ("It already uses the top of your range, so a key change won't help: warm up well, and sing the low notes with a steady, supported breath."));
        a.paragraphs.add (text);
    }
    else
    {
        a.verdict = Analysis::Verdict::fits;
        a.share = topShare;
        a.paragraphs.add ("The melody, from " + lowName + " to " + highName + ", sits inside your range (" + rangeNames + "), with " + semitones (a.sparesTop)
                          + " to spare at the top and " + juce::String (a.sparesBottom) + " at the bottom. A good key for you.");
    }

    addLongestLine();
    return a;
}

//==============================================================================
int getWarmupStart (Range range)
{
    return range.isSet() ? range.lowest + 5 : 60;
}

juce::String explain (Topic t, const Context& context)
{
    juce::String text;

    switch (t)
    {
        case Topic::voice:      text = voiceText; break;
        case Topic::breathing:  text = breathingText; break;
        case Topic::warmup:     text = warmupText; break;
        case Topic::range:      text = rangeText; break;
        case Topic::registers:  text = registersText; break;
        case Topic::tune:       text = tuneText; break;
        case Topic::vowels:     text = vowelsText; break;
        case Topic::vibrato:    text = vibratoText; break;
        case Topic::loud:       text = loudText; break;
        case Topic::harmony:    text = harmonyText; break;
        case Topic::care:       text = careText; break;
        case Topic::microphone: text = microphoneText; break;
        case Topic::yours:
            text = yoursIntro;

            for (auto& paragraph : context.analysis.paragraphs)
                text << "\n\n" << paragraph;

            return text;
    }

    if (t == Topic::warmup)
    {
        const int start = getWarmupStart (context.range);
        const juce::String sentence = context.range.isSet()
            ? "The picture's scale starts on " + getNoteName (start) + ", a fourth above your lowest note (" + getNoteName (context.range.lowest) + "); click it to hear it."
            : juce::String ("The picture's scale starts on C4 (middle C); click it to hear it, and start an octave lower if that's high for you.");
        text = text.replace ("{start}", sentence);
    }

    return text;
}

//==============================================================================
double VoiceExample::getEnvelope (const double (&formants)[2], double hz)
{
    // Two smooth humps on a log-frequency axis: an illustration, not a measured spectrum.
    double h = 0.04;

    for (int i = 0; i < 2; ++i)
    {
        const double octaves = std::log2 (juce::jmax (1.0, hz) / formants[i]);
        h = juce::jmax (h, (i == 0 ? 1.0 : 0.8) * std::exp (-0.5 * octaves * octaves / (0.35 * 0.35)));
    }

    return juce::jlimit (0.0, 1.0, h);
}

VoiceExample getVoiceExample()
{
    return {};
}

BreathingExample getBreathingExample()
{
    BreathingExample e;
    e.lines.push_back ({ "I walked along the river late", 0.0, 6.0, 8.0, juce::String::fromUTF8 ("2 beats \xc2\xb7 1.25 s: a full breath"), -1.0 });
    e.lines.push_back ({ "the city lights behind me", 8.0, 13.5, 14.0, juce::String::fromUTF8 ("\xc2\xbd beat \xc2\xb7 0.3 s: a quick breath"), -1.0 });
    e.lines.push_back ({ "your voice still ringing in my head, the way it used to find me", 14.0, 24.0, 24.0,
                         juce::String::fromUTF8 ("10 beats \xc2\xb7 6.3 s: breathe deeply before it"), 19.0 });
    return e;
}

WarmupExample getWarmupExample (Range range)
{
    WarmupExample e;
    e.startNote = getWarmupStart (range);
    static const int steps[] { 0, 2, 4, 5, 7, 5, 4, 2, 0 };
    double beat = 0.0;

    for (size_t i = 0; i < std::size (steps); ++i)
    {
        const double length = i + 1 == std::size (steps) ? 1.0 : 0.5;
        e.scale.push_back ({ e.startNote + steps[i], beat, length, 0.0f, {} });
        e.repeat.push_back ({ e.startNote + steps[i] + 1, beat + 5.0, length, 0.0f, {} });
        beat += length;
    }

    return e;
}

std::vector<RegisterExample> getRegisterExamples()
{
    return { { "Tenor", 48, 72, 62, 67, 62, 67 }, { "Mezzo-soprano", 57, 81, 64, 76, 64, 76 } };
}

double TuneCurve::getCents (double t) const
{
    switch (kind)
    {
        case Kind::slide:   return t < 0.25 ? -150.0 * (1.0 - t / 0.25) : 0.0;
        case Kind::vibrato: return t < 0.3 ? 0.0 : 40.0 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * (t - 0.3));
        case Kind::drift:   return t < 0.5 ? 0.0 : -30.0 * (t - 0.5) / 1.0;
    }

    return 0.0;
}

std::vector<TuneCurve> getTuneExamples()
{
    return { { TuneCurve::Kind::slide, "Sliding in by accident", "Starts 150 cents low and slides up in a quarter of a second.", 1.5 },
             { TuneCurve::Kind::vibrato, "In tune, with vibrato", "The pitch swings evenly around the target: fine.", 1.5 },
             { TuneCurve::Kind::drift, "Drifting flat", "Starts right and sinks 30 cents by its end.", 1.5 } };
}

std::vector<VowelLine> getVowelExamples()
{
    constexpr double c = 0.128;   // a consonant of 0.08 s at 96 BPM, in beats
    std::vector<VowelLine> lines;

    VowelLine right;
    right.label = "Right";
    right.note = "Vowels carry the notes; the n and t are quick.";
    right.words = { { "to", 0.0, 1.0 }, { "night", 1.0, 4.0 } };
    right.parts = { { "t", -c, 0.0, false }, { "uw", 0.0, 0.9, true }, { "n", 0.87, 1.0, false }, { "ay", 1.0, 3.87, true }, { "t", 3.87, 4.0, false } };
    lines.push_back (right);

    VowelLine wrong;
    wrong.label = "Wrong";
    wrong.note = "The ng is held instead of the vowel: a hum.";
    wrong.wrong = true;
    wrong.words = { { "sing", 0.0, 4.0 } };
    wrong.parts = { { "s", -c, 0.0, false }, { "ih", 0.0, 0.3, true }, { "ng", 0.3, 4.0, false } };
    lines.push_back (wrong);

    VowelLine fixed;
    fixed.label = "Fixed";
    fixed.note = "Hold the ih; close with the ng at the very end.";
    fixed.words = { { "sing", 0.0, 4.0 } };
    fixed.parts = { { "s", -c, 0.0, false }, { "ih", 0.0, 3.85, true }, { "ng", 3.85, 4.0, false } };
    lines.push_back (fixed);

    return lines;
}

double VibratoCurve::getCents (double t) const
{
    return t < startSeconds ? 0.0 : extentCents * std::sin (juce::MathConstants<double>::twoPi * rateHz * (t - startSeconds));
}

VibratoExample getVibratoExample()
{
    VibratoExample e;
    e.curves = { { "Straight tone", 0.0, 0.0, 0.0 }, { "Late vibrato", 30.0, 5.5, 1.0 }, { "Vibrato all through", 60.0, 6.0, 0.0 } };
    return e;
}

std::vector<NoteBlock> getLoudExample()
{
    std::vector<NoteBlock> notes;
    double beat = 0.0;

    for (int pitch : { 64, 64, 62, 60 })
        notes.push_back ({ pitch, beat++, 1.0, -12.0f, {} });

    notes.push_back ({ 67, beat, 1.0, -3.0f, {} });
    notes.push_back ({ 69, beat + 1.0, 1.0, -3.0f, {} });
    notes.push_back ({ 72, beat + 2.0, 2.0, 0.0f, "belt" });
    notes.push_back ({ 69, beat + 4.0, 1.0, -3.0f, {} });
    return notes;
}

HarmonyExample getHarmonyExample()
{
    HarmonyExample e;
    static const int lead[] { 64, 64, 67, 64, 62, 60 }, harmony[] { 67, 67, 71, 67, 65, 64 };
    static const double starts[] { 0.0, 1.0, 2.0, 3.0, 4.0, 4.5 }, lengths[] { 1.0, 1.0, 1.0, 1.0, 0.5, 3.5 };
    static const char* words[] { "hold", "on", "hold", "on", "to", "night" };

    for (size_t i = 0; i < 6; ++i)
    {
        e.lead.push_back ({ lead[i], starts[i], lengths[i], 0.0f, words[i] });
        e.harmony.push_back ({ harmony[i], starts[i], lengths[i], 0.0f, {} });
    }

    return e;
}

double getMicLevelDb (double distanceCm)
{
    return 20.0 * std::log10 (20.0 / juce::jmax (0.1, distanceCm));
}

std::vector<double> getMicDistances()
{
    return { 5.0, 10.0, 15.0, 20.0, 30.0, 40.0, 60.0 };
}

CareExample getCareExample()
{
    CareExample e;
    e.everyDay = { "Drink water through the day", "Steam, now and then", "Rest before and after a long session", "Don't whisper", "Take breaks" };
    e.doctor = { "Hoarse after 2 weeks", "Problems that keep coming back", "Very painful to swallow, or hard to breathe: at once" };
    return e;
}

size_t getExampleSize (Topic t, Range range)
{
    switch (t)
    {
        case Topic::voice:      return (size_t) getVoiceExample().harmonics;
        case Topic::breathing:  return getBreathingExample().lines.size();
        case Topic::warmup:     { const auto e = getWarmupExample (range); return e.scale.size() + e.repeat.size() + 1; }
        case Topic::range:      return getVoiceTypes().size() + (range.isSet() ? 1 : 0);
        case Topic::registers:  return getRegisterExamples().size();
        case Topic::tune:       return getTuneExamples().size();
        case Topic::vowels:     return getVowelExamples().size();
        case Topic::vibrato:    return getVibratoExample().curves.size();
        case Topic::loud:       return getLoudExample().size();
        case Topic::harmony:    return getHarmonyExample().lead.size() + getHarmonyExample().harmony.size();
        case Topic::care:       return (size_t) (getCareExample().everyDay.size() + getCareExample().doctor.size());
        case Topic::microphone: return getMicDistances().size();
        case Topic::yours:      return 1;
    }

    return 0;
}
} // namespace daw::gen::singing
