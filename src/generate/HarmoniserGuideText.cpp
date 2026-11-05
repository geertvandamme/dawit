#include "HarmoniserGuideText.h"

namespace daw::gen::harmoniserguide
{
namespace
{
    const char* const names[numPages][2] = {
        { "What a harmoniser is", "Extra voices made from the one you recorded" },
        { "Intervals", "How far apart two notes are, in semitones and in names" },
        { "Fixed or in the key", "The same number of semitones, or the same number of scale steps" },
        { "Which intervals sound how", "Thirds and sixths blend, fifths and octaves thicken" },
        { "Where the key isn't enough", "Passing notes, the chords and the one odd fifth" },
        { "Doubling and width", "The same note again, a little different, for a wider sound" },
        { "Formants and how far to move", "Why the voice keeps its colour, and why small moves sound best" },
        { "What it hears", "One clear note at a time, and what happens to the rest" },
        { "Using it well", "Tune first, voices under the lead, harmony where it counts" },
        { "Your track", "What the voices made of this track" } };

    const char* const texts[numPages] = {
        R"harm(A harmoniser listens to one voice, or one instrument playing one note at a time, and sings along with it: it makes copies of the line, moved up or down in pitch, and plays them with the original. From one take you get a harmony, a voice an octave lower, or a wider double of the lead.

DawIt Harmoniser has two voices. Each has its own interval (how far it's moved), level, pan, detune and delay. The original, called the lead here, plays too; Mix sets how much of each you hear.

The picture shows a line in C major, "hold on, hold on to-night", from the singing guide. The lead sings E4, E4, G4, E4, D4, C4. Voice 1, set to a 3rd up in the key, sings G4, G4, B4, G4, F4, E4. Each harmony note is two steps up the scale, so some are 3 semitones above the lead and some 4. That's the difference between Fixed and Key, which the next pages explain.

A harmoniser moves the voice you sang. It doesn't sing other words, change the rhythm or choose a harmony the way a singer would: each voice follows the lead note by note. And it needs one clear note at a time: on chords, a full mix or a noisy take it can't find the note, and the voices go quiet (What it hears).

The first harmonisers were studio boxes. Eventide's H910 Harmonizer of 1975 could move a sound up or down by as much as an octave.)harm",
        R"harm(An interval is the distance between two notes. The smallest step on a piano, from one key to the very next, is a semitone. Intervals are named by counting letter names: from C up to E is a third (C, D, E: three letters), from C up to G a fifth.

The same name comes in two sizes. C up to E is 4 semitones, a major third; D up to F is 3, a minor third. Both are thirds, because both span three letter names. The common intervals:

- unison: 0 semitones (the same note)
- minor 2nd: 1; major 2nd: 2
- minor 3rd: 3; major 3rd: 4
- perfect 4th: 5
- tritone: 6 (also called an augmented 4th or a diminished 5th)
- perfect 5th: 7
- minor 6th: 8; major 6th: 9
- minor 7th: 10; major 7th: 11
- octave: 12

Moving a note up an interval multiplies its frequency. One semitone multiplies it by 1.0595, four by 1.2599: A4 at 440 Hz becomes C#5 at 554.4 Hz. An octave doubles it, to 880 Hz; an octave down halves it, to 220 Hz. Intervals bigger than an octave sound much like the same interval within it: a third plus an octave (a tenth) still sounds like a third.)harm",
        R"harm(DawIt Harmoniser moves its voices in one of two ways, set by Harmony.

Fixed moves every note by the same number of semitones. Set voice 1 to +4 and every note gets a major third above it: C gets E, D gets F#, E gets G#. That's exact and works on anything, but in a key it soon leaves the key. The picture runs the C major scale through both. With +4, four of the seven harmony notes are outside C major: F#, G#, C# and D#. Fixed suits octaves (12 semitones), fifths on a riff, doubles, and music that doesn't stay in one key.

Key moves every note a number of steps along the scale. A 3rd up is two steps up: C gets E, D gets F, E gets G, F gets A. All the harmony notes are in C major, and the third is a major third (4 semitones) on C, F and G and a minor third (3) on D, E, A and B. That's how a singer thinks of a harmony, and why the singing guide says a harmony follows the key, not a fixed number of keys. Studio harmonisers call this diatonic or scale-based harmony.

The key comes from the song, the key in the transport bar. Under Key you can choose another one for this effect, for a part in a different key from the rest.

A note outside the key, such as F# in C major, gets the interval of the nearest key note: F if it's nearer F, G if it's nearer G, and F (the lower) exactly halfway. A note that wavers near the halfway point keeps its choice until it's clearly past it, so the harmony doesn't flicker between two notes. Harmonisers handle these notes in different ways; this is DawIt's choice.

Each voice keeps the lead's tuning. If the lead sings 20 cents flat, the harmony is 20 cents flat too, so the two stay in tune with each other. Vibrato and slides come through the same way. To tune the lead itself, put DawIt Tune before DawIt Harmoniser.)harm",
        R"harm(Not every interval makes the same kind of harmony. Music theory sorts them by how stable they sound together.

Thirds and sixths (3, 4, 8 or 9 semitones) are called imperfect consonances: pleasant together, with each voice still heard as its own line. That's why a 3rd above, a 3rd below and a 6th below are the most used harmonies in pop, country and folk. A 6th below is a 3rd above moved down an octave: the lead's E4 gets G4 a 3rd above, or G3 a 6th below.

Unisons, fifths and octaves (0, 7 and 12 semitones) are perfect consonances. They blend so well that two voices moving together in them start to sound like one thicker voice. An octave below gives a lead weight; a fifth above, with or without an octave, is the sound of a power chord on a guitar riff. Classical part-writing avoids parallel fifths and octaves to keep the parts sounding independent; pop and rock use them freely, as a colour.

Seconds and sevenths (1, 2, 10 or 11 semitones) rub: they're dissonances. A whole line harmonised in seconds sounds tense, which is why no starting point uses them. The fourth (5 semitones) sits in between: it sounds settled between two upper voices and unsettled above the bass.

Two voices are often enough. A 3rd and a 5th above make a three-note chord on every note: rich on a chorus hook, heavy across a whole verse.)harm",
        R"harm(Key harmony knows the scale, not the chords. Most of the time that's fine: most sung notes belong to the chord under them, and a 3rd up from a chord note is usually another chord note. Over a C major chord (C, E, G), a sung E gets G and a sung C gets E.

A passing note between chord notes gets a harmony that may not fit the chord. In the picture the lead sings E4, D4, C4 over C major, and the 3rd up sings G4, F4, E4. The F isn't in the chord and sits one semitone above the chord's E. On a short note it goes by unnoticed, as passing notes do; on a long held note it can rub. A singer would perhaps hold the G there instead. When a long note clashes, try another interval for that part, or sing that bit of harmony yourself.

Fifths have one odd note. In a major key every note of the scale has a perfect fifth (7 semitones) inside the scale except the 7th: in C major, B up a 5th in the key is F, only 6 semitones, a diminished fifth or tritone, which sounds tense and unsettled. In A minor the same happens on B. If a Key voice set to a 5th up sounds harsh on one note, that's usually it. A 4th up has the same thing on F (F to B). Thirds and sixths don't: they only ever come in their major and minor sizes.

DawIt Harmoniser doesn't follow the chord track. Your track shows which sizes each voice actually made.)harm",
        R"harm(Doubling is the lead twice: the same notes a second time. Singers do it by recording a second take, and the small differences between the takes make the voice sound thicker and smoother (the singing guide's Harmonies and doubles). A plain copy can't do that: two identical copies only make the sound 6 dB louder. A voice at 0 semitones with no detune and no delay is close to such a copy.

To make a copy sound more like a second take, studios change it a little in two ways:
- Detune moves it a few cents. At A3 (220 Hz), 7 cents is 0.9 Hz, so the copy drifts slowly against the lead instead of locking to it.
- Delay starts it a few milliseconds later, like a second singer who's never exactly together with the first.

A common recipe is two copies, one a few cents up and one a few cents down, each with its own short delay, panned apart, with the lead in the centre. The Wide double starting point does that: +7 and −7 cents, 12 and 20 ms, panned 70% left and right, each 8 dB under the lead. Studio guides typically suggest about 4 to 10 cents and 8 to 30 ms.

Keep the delay under about 30 ms. Up to there the ear hears the copy as part of the same sound; from about 50 ms on a voice it starts to hear a separate echo.

The trick is old. In 1966 Ken Townsend at Abbey Road built ADT (artificial double tracking) for the Beatles' Revolver: the vocal played back from a second tape machine a few milliseconds late, its speed wobbling.

A made double isn't a real second take: it has the lead's breaths, consonants and small slips. For a lead that must sound truly doubled, sing the part again.)harm",
        R"harm(A voice has a pitch, how fast the vocal folds buzz, and formants: the resonances of the throat and mouth that make an "ah" sound like "ah" and your voice sound like you (What is pitch correction? explains them). An "ah" has its first two formants near 730 and 1,090 Hz, whatever note is sung.

Speeding a recording up moves both. Up a fifth (7 semitones, ×1.498), the 730 Hz formant moves to 1,094 Hz and the voice sounds smaller and younger; up an octave it's a chipmunk. DawIt Harmoniser moves its voices the way DawIt Tune corrects notes, with PSOLA: it moves the pitch and leaves the formants where they are. A harmony a fifth up still sounds like an adult singing "ah", as the picture shows.

It still sounds best on small moves. The bigger the move, the more the method shows, as a buzzy or hollow edge: typically most beyond about 6 semitones, and clearest on an octave up or down. Thirds and fourths usually sound natural; octaves are best kept quieter, under the lead. Each voice goes at most an octave each way.

Because the formants stay, a voice made this way sounds like you singing higher or lower, not like a second singer. A real second singer has other formants, which is part of why a choir sounds like many people. DawIt Harmoniser has no formant control.)harm",
        R"harm(To move a note, DawIt Harmoniser first has to find it, the same way DawIt Tune does: it compares the sound with itself a moment later and looks for the length of one cycle. That needs a sound that repeats: one voice, or one instrument playing one note at a time.

- Chords and full mixes have several notes at once, so there's no one cycle to find. The voices go quiet and only the lead is heard. Studio harmonisers have the same limit: their pitch trackers follow one note.
- Consonants and breaths (s, f, sh, t) are noise with no pitch. The voices fade out on them and back in on the next vowel, so only the lead carries them. That keeps the s sounds from piling up into a hiss, the same reason singers sing the end consonants of doubles lightly.
- Very low notes, below about A2 (110 Hz), get no voices: one cycle is too long for the short time it hears ahead. That only touches a bass line or the lowest notes of a deep voice.
- Reverb and delay before it blur the cycles. Put it before them.

It hears a little ahead, so inside it the track is about 27 milliseconds late (29 at a 44.1 kHz sample rate). On an audio track DawIt plays the clips that much earlier, so the track stays in time. On an instrument track, a bus or a return it can't, and everything on it plays late: use it on the audio track itself.)harm",
        R"harm(1. Start from a clean, in-tune lead. The voices copy its tuning, so tune it first if it needs it: DawIt Tune, then DawIt Harmoniser, then EQ and compression, with reverb on a send.
2. Choose Key for harmonies, Fixed for octaves, fifths on a riff, and doubles.
3. Keep the voices under the lead. Harmonies usually sit a few dB below it; the starting points put them 6 to 10 dB down. If the harmony is easier to follow than the lead, it's too loud.
4. Spread them: two voices panned a little left and right leave the centre to the lead.
5. Use them where they count. Harmonies usually come in the chorus, on the hook or on the last words of lines. DawIt Harmoniser works on the whole track: for a chorus-only harmony, copy the chorus clips (Cmd-drag) to a second audio track, put DawIt Harmoniser there with Mix at 100%, so it plays only the voices, and leave the lead's track as it is. The harmony then has its own fader.
6. Listen with the chords. A harmony that sounds fine alone can rub with a chord (Where the key isn't enough).
7. Treat it as a sketch. It sounds like the same singer, with the same breaths. For a final record many singers learn the part the harmoniser found and sing it.)harm",
        R"harm(DawIt Harmoniser listens while the song plays and keeps what it heard: the lead's note and each voice's note. This page sums that up. It only knows what has played through it, so play the song through once; playing a part again replaces what it heard there. What it heard isn't saved with the project.)harm" };
}

juce::String getPageName (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (names[page][0]) : juce::String(); }
juce::String getPageSummary (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (names[page][1]) : juce::String(); }
juce::String getPageText (int page) { return juce::isPositiveAndBelow (page, numPages) ? juce::String::fromUTF8 (texts[page]) : juce::String(); }

juce::String getControlText (Control control)
{
    switch (control)
    {
        case Control::harmony:        return juce::String::fromUTF8 (R"harm(Fixed moves every note by the same number of semitones. Key moves it a number of steps along the scale, so the voices stay in the key: a 3rd up is sometimes 3 semitones, sometimes 4.)harm");
        case Control::key:            return juce::String::fromUTF8 (R"harm(The key Key harmony follows. Song uses the key in the transport bar; choose a note for a part in another key.)harm");
        case Control::scale:          return juce::String::fromUTF8 (R"harm(Major or natural minor, for a key chosen here. With Song the song's own scale is used.)harm");
        case Control::mix:            return juce::String::fromUTF8 (R"harm(How much of the lead and the voices you hear. At 50% both play at full level; at 0% only the lead; at 100% only the voices, to put a harmony on a track of its own.)harm");
        case Control::on:             return juce::String::fromUTF8 (R"harm(Turns this voice on or off.)harm");
        case Control::intervalFixed:  return juce::String::fromUTF8 (R"harm(How far this voice is moved, in semitones: +4 is a major 3rd up, +7 a 5th up, −12 an octave down.)harm");
        case Control::intervalKey:    return juce::String::fromUTF8 (R"harm(How far this voice is moved along the scale: 3rd up is two steps, 5th up four, an octave seven.)harm");
        case Control::level:          return juce::String::fromUTF8 (R"harm(How loud this voice is. Harmonies usually sit a few dB under the lead.)harm");
        case Control::pan:            return juce::String::fromUTF8 (R"harm(Where this voice sits between the speakers. In the centre it is 3 dB quieter in each, so it sounds equally loud wherever it is.)harm");
        case Control::detune:         return juce::String::fromUTF8 (R"harm(Moves the voice a few cents off its note. 4 to 10 cents makes a copy drift against the lead like a second take.)harm");
        case Control::delay:          return juce::String::fromUTF8 (R"harm(Starts the voice later. 8 to 30 ms makes a double; from about 50 ms it's heard as an echo.)harm");
    }

    return {};
}

juce::String getSingingParagraph()
{
    return "DawIt Harmoniser can make a harmony or a double from the lead itself, to try a part before you sing it (What is a harmoniser?).";
}
}
