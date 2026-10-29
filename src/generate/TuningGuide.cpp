#include "TuningGuide.h"

#include <cmath>

namespace daw::gen::tuning
{
namespace
{
struct Definition
{
    Tuning tuning;
    const char* id;
    const char* name;
    std::vector<std::pair<int, const char*>> strings;   // MIDI note and spelling
};

const std::vector<Definition>& getDefinitions()
{
    static const std::vector<Definition> all {
        { Tuning::guitarStandard, "guitarStandard", "Guitar: standard", { { 40, "E2" }, { 45, "A2" }, { 50, "D3" }, { 55, "G3" }, { 59, "B3" }, { 64, "E4" } } },
        { Tuning::dropD,          "dropD",          "Guitar: Drop D",   { { 38, "D2" }, { 45, "A2" }, { 50, "D3" }, { 55, "G3" }, { 59, "B3" }, { 64, "E4" } } },
        { Tuning::dadgad,         "dadgad",         "Guitar: DADGAD",   { { 38, "D2" }, { 45, "A2" }, { 50, "D3" }, { 55, "G3" }, { 57, "A3" }, { 62, "D4" } } },
        { Tuning::halfStepDown,   "halfStepDown",   "Guitar: half a step down", { { 39, "Eb2" }, { 44, "Ab2" }, { 49, "Db3" }, { 54, "Gb3" }, { 58, "Bb3" }, { 63, "Eb4" } } },
        { Tuning::bass4,          "bass4",          "Bass: 4 strings",  { { 28, "E1" }, { 33, "A1" }, { 38, "D2" }, { 43, "G2" } } },
        { Tuning::bass5,          "bass5",          "Bass: 5 strings",  { { 23, "B0" }, { 28, "E1" }, { 33, "A1" }, { 38, "D2" }, { 43, "G2" } } },
        { Tuning::ukulele,        "ukulele",        "Ukulele",          { { 67, "G4" }, { 60, "C4" }, { 64, "E4" }, { 69, "A4" } } },
        { Tuning::chromatic,      "chromatic",      "Chromatic",        {} },
    };

    return all;
}

const Definition& getDefinition (Tuning tuning)
{
    for (const auto& d : getDefinitions())
        if (d.tuning == tuning)
            return d;

    return getDefinitions().front();
}

juce::String letterOf (const TuningString& s)
{
    return s.note.trimCharactersAtEnd ("0123456789");
}

juce::String plural (int n, const juce::String& singular)
{
    return juce::String (n) + " " + singular + (n == 1 ? "" : "s");
}

juce::String joinList (const juce::StringArray& items)
{
    if (items.size() <= 1)
        return items.joinIntoString ("");

    return items.joinIntoString (", ", 0, items.size() - 1) + " and " + items[items.size() - 1];
}

/** A frequency without trailing zeros: "442", "441.5". */
juce::String describeHz (double hz)
{
    const auto text = juce::String (hz, 2);
    return text.contains (".") ? text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".") : text;
}
}

//==============================================================================
juce::String getTuningName (Tuning tuning) { return getDefinition (tuning).name; }
juce::String getTuningId (Tuning tuning) { return getDefinition (tuning).id; }

Tuning getTuningFromId (const juce::String& id)
{
    for (const auto& d : getDefinitions())
        if (id == d.id)
            return d.tuning;

    return Tuning::guitarStandard;
}

bool isFrettedTuning (Tuning t)
{
    return t == Tuning::guitarStandard || t == Tuning::dropD || t == Tuning::dadgad || t == Tuning::halfStepDown || t == Tuning::ukulele;
}

bool isBassTuning (Tuning t) { return t == Tuning::bass4 || t == Tuning::bass5; }

std::vector<TuningString> getStrings (Tuning tuning)
{
    std::vector<TuningString> strings;

    for (const auto& [midi, note] : getDefinition (tuning).strings)
        strings.push_back ({ midi, note, {} });

    for (auto& s : strings)
    {
        const auto letter = letterOf (s);
        int copies = 0, lower = 0, higher = 0;

        for (const auto& other : strings)
            if (letterOf (other) == letter)
            {
                ++copies;
                lower += other.midi < s.midi ? 1 : 0;
                higher += other.midi > s.midi ? 1 : 0;
            }

        s.label = copies == 1 ? letter : lower == 0 ? "low " + letter : higher == 0 ? "high " + letter : s.note;
    }

    return strings;
}

double getHz (double midi, double referenceHz)
{
    return referenceHz * std::pow (2.0, (midi - 69.0) / 12.0);
}

juce::String getNoteLetter (int midi)
{
    return MusicalKey::getTonicName (midi);
}

juce::String getNoteName (int midi)
{
    return getNoteLetter (midi) + juce::String (midi / 12 - 1);
}

double getCents (double hz, double baseHz)
{
    return 1200.0 * std::log2 (hz / baseHz);
}

//==============================================================================
Zone getZone (double cents)
{
    const double c = std::abs (cents);
    return c <= inTuneCents ? Zone::inTune : c <= closeCents ? Zone::close : Zone::out;
}

juce::String describeCentsShort (double cents)
{
    if (getZone (cents) == Zone::inTune)
        return "in tune";

    const double c = std::abs (cents);

    if (c > 50.0)
        return plural ((int) std::lround (c / 100.0), "semitone") + (cents < 0.0 ? " low" : " high");

    return plural ((int) std::lround (c), "cent") + (cents < 0.0 ? " flat" : " sharp");
}

/** The "Your tuning" wording: the cents even when within the green band, "in tune" only when they round to 0. */
static juce::String describeLastReading (double cents)
{
    if (std::abs (cents) < 0.5)
        return "in tune";

    const double c = std::abs (cents);
    return c > 50.0 ? plural ((int) std::lround (c / 100.0), "semitone") + (cents < 0.0 ? " low" : " high")
                    : plural ((int) std::lround (c), "cent") + (cents < 0.0 ? " flat" : " sharp");
}

juce::String describeCents (double cents)
{
    if (getZone (cents) == Zone::inTune)
        return "in tune";

    const double c = std::abs (cents);
    const juce::String what = describeCentsShort (cents);

    if (c > 50.0)
        return what + (cents < 0.0 ? ", tighten it" : ", loosen it");

    const juce::String little = std::lround (c) <= 15 ? " a little" : "";
    return what + (cents < 0.0 ? ", tighten it" : ", loosen it") + little;
}

int getNearestString (double midi, Tuning tuning)
{
    const auto strings = getStrings (tuning);
    int best = -1;
    double bestDistance = 1.0e9;

    for (int i = 0; i < (int) strings.size(); ++i)
    {
        const double d = std::abs (midi - strings[(size_t) i].midi);

        // On a tie the lower string wins.
        if (d < bestDistance - 1.0e-9 || (std::abs (d - bestDistance) <= 1.0e-9 && strings[(size_t) i].midi < strings[(size_t) best].midi))
        {
            best = i;
            bestDistance = d;
        }
    }

    return best;
}

Reading getReading (double hz, double referenceHz, Tuning tuning, int lockedString)
{
    Reading r;
    const auto strings = getStrings (tuning);
    const double midiF = 69.0 + 12.0 * std::log2 (hz / referenceHz);
    r.midi = (int) std::lround (midiF);
    r.note = getNoteName (r.midi);
    r.cents = (midiF - r.midi) * 100.0;

    if (lockedString >= 0 && lockedString < (int) strings.size())
    {
        r.locked = true;
        r.stringIndex = lockedString;
        r.cents = (midiF - strings[(size_t) lockedString].midi) * 100.0;
    }
    else if (const int nearest = getNearestString (midiF, tuning); nearest >= 0 && std::abs (midiF - strings[(size_t) nearest].midi) <= 0.5 + 1.0e-9)
    {
        r.stringIndex = nearest;
        r.note = strings[(size_t) nearest].note;
        r.midi = strings[(size_t) nearest].midi;
        r.cents = (midiF - r.midi) * 100.0;
    }

    r.zone = getZone (r.cents);
    r.words = describeCents (r.cents);

    if (r.stringIndex >= 0)
    {
        r.stringName = strings[(size_t) r.stringIndex].label + " string";
        r.text = r.stringName + ": " + r.words;
    }
    else
    {
        r.text = r.words;
    }

    return r;
}

//==============================================================================
double getBeatsPerSecond (double hz1, double hz2) { return std::abs (hz1 - hz2); }

double getHzForCents (double hz, double cents) { return hz * (std::pow (2.0, cents / 1200.0) - 1.0); }

double getCentsForTensionPercent (double percent) { return 600.0 * std::log2 (1.0 + percent / 100.0); }

double getReferenceCents (double referenceHz) { return getCents (referenceHz, 440.0); }

std::vector<double> getHarmonicTuningErrors()
{
    const double pureFourth = getCents (4.0, 3.0);     // 498.045
    const double pureThird = getCents (5.0, 4.0);      // 386.314
    const double step = 500.0 - pureFourth;
    const double third = 400.0 - pureThird;
    return { -step, -2.0 * step, -3.0 * step, -(3.0 * step + third) };
}

double getCyclesInWindow (double hz, double ms) { return ms * hz / 1000.0; }

double getBeatEnvelope (double t)
{
    return std::abs (2.0 * std::cos (juce::MathConstants<double>::pi * 2.0 * t));
}

double getTuningPath (double t)
{
    static constexpr double points[][2] { { 0.0, 30.0 }, { 1.5, 30.0 }, { 2.5, -20.0 }, { 3.5, -20.0 }, { 5.0, 0.0 }, { 6.0, 0.0 } };

    if (t <= points[0][0])
        return points[0][1];

    for (size_t i = 1; i < std::size (points); ++i)
        if (t <= points[i][0])
        {
            const double f = (t - points[i - 1][0]) / (points[i][0] - points[i - 1][0]);
            return points[i - 1][1] + f * (points[i][1] - points[i - 1][1]);
        }

    return points[std::size (points) - 1][1];
}

double getDriftGlide (double t) { return 6.0 * std::exp (-t / 0.3); }

std::vector<int> getFifthFretSteps() { return { 5, 5, 5, 4, 5 }; }

std::vector<double> getHarmonicHz (double hz) { return { 2.0 * hz, 3.0 * hz, 4.0 * hz }; }

//==============================================================================
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItShows:    return "What a tuner shows";
        case Topic::centsAndBeats:  return "Cents, hertz and beats";
        case Topic::tunings:        return "Standard tuning and other tunings";
        case Topic::reference:      return "Reference pitch: A4 = 440 Hz";
        case Topic::howToTune:      return "How to tune a string";
        case Topic::drift:          return "Why strings drift";
        case Topic::byEar:          return "Tuning by ear: the fifth fret";
        case Topic::harmonics:      return "Harmonics at the 12th, 7th and 5th fret";
        case Topic::bassAndUkulele: return "Bass, ukulele and low notes";
        case Topic::yourTuning:     return "Your tuning and your song";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItShows:    return "The note, how many cents off, and which way to turn";
        case Topic::centsAndBeats:  return "Why tuners count in cents, and what two nearly equal notes sound like";
        case Topic::tunings:        return "E A D G B E, and drop D, DADGAD, half a step down, bass and ukulele";
        case Topic::reference:      return "What counts as A, and when to change it";
        case Topic::howToTune:      return "Pluck, read, turn, and why you tune up to the note";
        case Topic::drift:          return "New strings stretch, warmth and damp move the wood, and a hard pluck starts sharp";
        case Topic::byEar:          return "Matching each string to the one below, and listening for beats";
        case Topic::harmonics:      return "Bell-like notes that show a string's octave, and why tuning with them can mislead";
        case Topic::bassAndUkulele: return "Why a bass's low E takes the tuner longer, and how a ukulele is tuned";
        case Topic::yourTuning:     return "Whether the open strings fit the key, low notes, the reference, and each string's last reading";
    }

    return {};
}

juce::String getExampleCaption (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItShows:    return "The A string, 12 cents flat";
        case Topic::centsAndBeats:  return "440 Hz plus 442 Hz: two swells a second";
        case Topic::tunings:        return "The strings of the chosen tuning, with their notes and Hz";
        case Topic::reference:      return "Reference pitches from 415 to 466 Hz";
        case Topic::howToTune:      return "A string tuned from 30 cents sharp, down to 20 flat, and up to the note";
        case Topic::drift:          return "One hard pluck over 2 seconds, drawn to show the shape, not measured";
        case Topic::byEar:          return "The fifth fret on each string gives the next open string";
        case Topic::harmonics:      return "The A string in two, three and four parts: 220, 330 and 440 Hz";
        case Topic::bassAndUkulele: return "The same 85 ms over a 41.2 Hz and a 329.6 Hz wave";
        case Topic::yourTuning:     return "Your tuning's strings: green when in the key, with their last readings";
    }

    return {};
}

juce::String explain (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItShows:
            return juce::String::fromUTF8 (R"T(A tuner listens to one note and tells you three things: which note it is, how far it is from that note, and which way to go. Play the A string of a guitar and the tuner shows A2. If it says 12 cents flat, the string is a little low: tighten it. If it says 8 cents sharp, it's a little high: loosen it.

A cent is a hundredth of a semitone, the step from one fret to the next. The needle runs from 50 cents flat on the left to 50 cents sharp on the right. Past 50 cents the note is nearer the next semitone, so the tuner shows that note instead.

DawIt's tuner turns green within 3 cents of the note, yellow within 10 and red beyond. These bands are DawIt's choice, not a standard. They're stricter than the 10 cents DawIt Tune allows a singer, because a string is tuned once and then played for a whole song, and its error adds to the other strings' in every chord.

With a tuning chosen, such as Guitar: standard, the tuner also names the nearest string: "A string: 12 cents flat, tighten it a little". Click a string's button to tune only that string. The tuner then measures against that string even when it's far off, and plays its note so you can also tune by ear.

Play one string at a time and let it ring. A chord, or two strings ringing together, has no single pitch: the tuner usually shows a dash, and a note it does show then can't be trusted. A tuner checks the open strings only. Whether the guitar is still in tune higher up the neck depends on its setup: see *Why strings drift*.)T");

        case Topic::centsAndBeats:
            return juce::String::fromUTF8 (R"T(Pitch is how many times a second a string vibrates, counted in hertz (Hz). A guitar's A string vibrates 110 times a second: 110 Hz. The A an octave up is 220 Hz, and A4, the A above middle C, is 440 Hz. Every octave doubles the number.

A semitone is split into 100 cents, and an octave is 1,200. Cents measure a ratio, so a cent is the same small step at any pitch. That's why tuners count in cents. 10 cents flat is 0.47 Hz at the low E (82.41 Hz) and 1.90 Hz at the high E (329.63 Hz), and it sounds just as far out on both.

When two notes are almost the same, you hear beats: the sound swells and fades as the two waves fall in and out of step. The number of beats a second is the difference in hertz. 440 Hz against 442 Hz beats twice a second. As the two notes come closer the beats slow down, and when they stop, the notes match.

Low notes beat slowly. Two A strings 5 cents apart, at 110 and 110.3 Hz, beat once every 3 seconds, which is easy to miss. Their higher harmonics beat faster: at 440 Hz the same 5 cents beats 1.3 times a second. So when you tune by ear, listen to the bright, high part of the sound.

How small a difference can you hear? Two steady notes played one after the other can be told apart when they're roughly 3 to 10 cents apart, depending on the pitch, the loudness and the listener. Held together it's easier: even 1 cent at 440 Hz makes a slow beat, one every 4 seconds.)T");

        case Topic::tunings:
            return juce::String::fromUTF8 (R"T(A guitar in standard tuning is tuned, from the thickest string to the thinnest, E2 A2 D3 G3 B3 E4: 82.4, 110.0, 146.8, 196.0, 246.9 and 329.6 Hz. Each string is a fourth (5 semitones) above the one before, except G to B, a major third (4 semitones). That one shorter step is what lets the common chord shapes fit under the fingers. A way to remember the strings: Eddie Ate Dynamite, Good Bye Eddie.

Other tunings change some of the strings:
- **Drop D:** the low E goes down a whole step to D2 (73.4 Hz): D A D G B E. A power chord is then one finger across the lowest three strings, and the guitar reaches a whole step lower. It's common in rock and metal.
- **DADGAD:** D2 A2 D3 G3 A3 D4. Three strings come down a whole step, and the open strings ring as a Dsus4 chord. It's heard a lot in folk and Celtic music.
- **Half a step down:** every string a semitone lower: Eb Ab Db Gb Bb Eb. The same shapes sound a semitone lower, which can suit a singer, and the slacker strings are a little easier to bend.
- **Bass:** E1 A1 D2 G2 (41.2, 55.0, 73.4 and 98.0 Hz), an octave below the guitar's four lowest strings. A five-string bass adds a low B0 at 30.9 Hz.
- **Ukulele:** G4 C4 E4 A4 (392.0, 261.6, 329.6 and 440.0 Hz). The G string is higher than the C next to it. This is called re-entrant tuning, and it gives the ukulele its bright, close sound. A low-G ukulele has a thicker string tuned an octave lower, to G3.
- **Chromatic:** no strings. The tuner names any note, for any instrument.

Tuning a string down a step or two is safe. Don't tune a string far above its usual note: the extra tension can break it.)T");

        case Topic::reference:
            return juce::String::fromUTF8 (R"T(Before a tuner can say a note is in tune, it needs a starting point: the pitch of A4, the A above middle C. Every other note is worked out from it. Almost all music recorded today uses A4 = 440 Hz, the concert pitch set as a standard in ISO 16. DawIt's instruments play at 440 Hz too, so leave the tuner at 440 Hz to play along with your song.

Some music is tuned differently. Many orchestras tune a little higher: some American orchestras to 442 Hz, and most in Germany and Austria to about 443 Hz. Groups playing baroque music on old instruments often use 415 Hz, a semitone below 440. An old recording that was sped up or slowed down, or a band that tuned to its own piano, can sit anywhere in between.

To play along with such a recording, set the reference to match it. Moving it from 440 to 442 Hz raises every note by 7.9 cents. The tuner's range, 415 to 466 Hz, runs from a semitone below 440 to a semitone above.

You may read that 432 Hz sounds more natural or is better for you. The studies behind such claims are few and small, and there's no established reason to prefer it. It is simply 31.8 cents lower than 440 Hz. If you tune to it, your guitar will sound flat against DawIt's instruments and most recordings.)T");

        case Topic::howToTune:
            return juce::String::fromUTF8 (R"T(1. Play the open string once, at the strength you normally play, and let it ring.
2. Read the tuner once the needle settles. A string plucked hard sounds slightly sharp at first and settles as it rings (see *Why strings drift*).
3. If it's flat, tighten the string by turning its tuning peg. If it's sharp, loosen it.
4. Tune up to the note. If the string is sharp, loosen it until it's a little flat, then tighten it up to the note. Coming from below, the peg's gears and the string over the nut settle under tension. Coming down from above, a little slack can stay caught and let go later, and the string drifts flat. Many players tune this way; with very good tuning pegs it matters less.
5. Turn a little at a time. A tiny turn makes a big change: the pitch follows the square root of the string's tension, so just 1% more tension raises the note by 8.6 cents.
6. Go round all the strings, then go round again. Tuning one string changes the pull on the neck a little. On a guitar with a floating bridge (a tremolo arm that can move the note up as well as down) it moves the bridge, and the other strings shift with it. The second round is quick.

Hold the guitar as you play it while you tune. At the end, play a chord you know, such as an open E major or G major: it should sound clear, without a slow wobble.)T");

        case Topic::drift:
            return juce::String::fromUTF8 (R"T(A guitar that was in tune yesterday is often out today. The usual reasons:
- **New strings stretch.** For the first hours of playing, and sometimes a few days, new strings stretch and settle around the pegs and the bridge, so they keep going flat. Tune them, gently pull each string away from the fretboard along its length, and tune again. Repeat until the pitch stops dropping.
- **Temperature and humidity.** Steel strings, and the wooden neck and body, grow and shrink with warmth and damp. A guitar brought in from a cold car, or played under hot stage lights, drifts as it warms up, flat or sharp depending on the guitar. Let it reach the room's temperature, then tune.
- **Playing.** Big string bends, heavy strumming and a tremolo arm all pull on a string, and a string can slip a little on its peg or catch in the nut.
- **A hard pluck starts sharp.** Plucking hard stretches the string a little more while it swings wide, so the first moment of a note is slightly sharp, and the pitch glides down as the note fades. Pluck at the strength you play, and read the tuner once the note has settled.

When the open strings are in tune but chords high up the neck sound out, the strings may not be the problem. Compare the note pressed at the 12th fret with the harmonic at the 12th fret (see *Harmonics*). They should be the same note. If the pressed note is sharp or flat, the guitar's intonation needs adjusting at the bridge. That's a setup job, for a guitar technician or for a careful evening with a setup guide.)T");

        case Topic::byEar:
            return juce::String::fromUTF8 (R"T(Without a tuner, or to check one, you can tune the strings to each other. First tune the low E to something you trust: a tuner, a piano, or the note the tuner's E2 button plays. Then:
- Press the low E string at the 5th fret. That's A2, 110 Hz. Tune the open A string to it.
- A string, 5th fret: D3. Tune the open D string to it.
- D string, 5th fret: G3. Tune the open G string to it.
- G string, **4th** fret: B3. Tune the open B string to it. G to B is a major third, one fret less than the other steps.
- B string, 5th fret: E4. Tune the open high E string to it.

Play both notes together and listen for beats, the slow wobble of two nearly equal notes. Turn the open string until the wobble slows down and stops.

Your guitar is then in tune with itself, which is all you need when you play alone. To play with a recording or with other people, the low E must match their pitch too. Each step can add a small error, and over five steps the errors add up. So check the high E against the low E: two octaves apart, they should sound as one.)T");

        case Topic::harmonics:
            return juce::String::fromUTF8 (R"T(Touch a string lightly, right above a fret wire, without pressing it down. Pluck it, then lift your finger: you hear a soft, bell-like note, a harmonic. Your finger stops the string moving at that point, so it can only vibrate in equal parts.
- At the 12th fret, halfway along, it vibrates in two halves: twice the open string's frequency, an octave up. On the A string that's 220 Hz.
- At the 7th fret, a third of the way along, in three parts: three times, an octave and a fifth up. On the A string, 330 Hz.
- At the 5th fret, a quarter of the way along, in four parts: four times, two octaves up. On the A string, 440 Hz.

A harmonic rings long and clear, which makes it easy to hear and easy for a tuner to read.

A popular shortcut tunes neighbouring strings by matching harmonics: the low E's 5th-fret harmonic (4 × 82.41 = 329.6 Hz) against the A string's 7th-fret harmonic (3 × 110 = 330 Hz). On a correctly tuned guitar these two aren't quite equal. A guitar is tuned in equal temperament, where every semitone is the same size, while harmonics give pure intervals, which are slightly different. Matching them leaves the A string 2 cents flat, the D 4 cents and the G 6. Tuning the B from the G string's 4th-fret harmonic is worse: that harmonic is a pure major third, which is 14 cents smaller than the guitar's, so the B ends up about 20 cents flat in all.

So use harmonics to check octaves at the 12th fret, or to give a tuner a clean note. Tune the strings themselves to a tuner or with fretted notes, and the B string always that way.)T");

        case Topic::bassAndUkulele:
            return juce::String::fromUTF8 (R"T(A bass's lowest string, E1, vibrates 41.2 times a second, so one vibration takes 24.3 milliseconds. DawIt's way of finding a pitch compares the sound with itself one vibration later, so it needs at least two whole vibrations in what it hears. The tuner listens to about 85 milliseconds at a time (4,096 samples at 48 kHz): three and a half vibrations of the low E, and more than two and a half of a five-string bass's low B (30.9 Hz). DawIt Tune, made for voices, listens to about 43 ms and stops at 50 Hz. That's why a bass needs the tuner.

Low strings are also hard to hear. Laptop microphones and small speakers barely pick up 41 Hz, so what reaches the tuner is mostly the string's harmonics. The tuner still finds the right note, because the harmonics repeat at the string's own rate. Still, plugging the bass straight into the audio interface's instrument input gives the steadiest reading. Tuning a bass by ear is easier with the harmonics at the 12th fret: they're an octave higher, so the beats come twice as fast.

A ukulele's strings are G4 C4 E4 A4, and the G is higher than the C beside it. Played in order from the G string, they sound the tune of "My Dog Has Fleas". To tune them to each other, start from the C string: its 4th fret is E (tune the E string to it), the E string's 5th fret is A (the A string), and the C string's 7th fret is G4, the high G string's note.)T");

        case Topic::yourTuning:
            return juce::String::fromUTF8 (R"T(This page compares the tuning chosen in the tuner with your song. It says whether the open strings are in the song's key, whether your bass or guitar part goes lower than the lowest string, whether the reference pitch matches DawIt's instruments, and how far each string was off when the tuner last heard it.

Open strings matter because they ring: in open chords, and when a string is left ringing by accident. In A minor, all six open strings of standard tuning (E A D G B E) are in the key, which is one reason A minor and E minor are so comfortable on a guitar. In Eb major only D and G are. Many players then use a capo: clamped on the 1st fret, it makes the open strings F, Bb, Eb, Ab, C and F, all in Eb major, and you play the shapes of D major.

A deliberate drop tuning and a string that has slipped can look the same on a tuner. If the tuner says your low string is D and your song is in D, that's probably drop D on purpose. If it's somewhere between D and E, it slipped.)T");
    }

    return {};
}

//==============================================================================
KeyFit getKeyFit (Tuning tuning, const MusicalKey& key, int capoFret)
{
    KeyFit fit;
    const auto strings = getStrings (tuning);

    for (int i = 0; i < (int) strings.size(); ++i)
        (key.contains (strings[(size_t) i].midi + capoFret) ? fit.inKey : fit.outOfKey).push_back (i);

    return fit;
}

int getBestCapoFret (Tuning tuning, const MusicalKey& key)
{
    int best = 1, bestCount = -1;

    for (int fret = 1; fret <= 5; ++fret)
    {
        const int count = (int) getKeyFit (tuning, key, fret).inKey.size();

        if (count > bestCount)
        {
            best = fret;
            bestCount = count;
        }
    }

    return best;
}

juce::StringArray analyse (Tuning tuning, double referenceHz, const MusicalKey& key,
                           const std::vector<TrackLow>& tracks, const std::vector<LastReading>& lastReadings)
{
    juce::StringArray advice;
    const auto strings = getStrings (tuning);
    const int m = (int) strings.size();

    // 1. Key fit.
    if (m > 0)
    {
        const auto fit = getKeyFit (tuning, key);
        const int n = (int) fit.inKey.size();
        juce::StringArray inNames, outNames;

        for (const int i : fit.inKey)
            inNames.add (letterOf (strings[(size_t) i]));

        for (const int i : fit.outOfKey)
            outNames.add (letterOf (strings[(size_t) i]));

        auto text = juce::String (n) + " of the " + juce::String (m) + " open strings (" + inNames.joinIntoString (", ") + ") are in " + key.getName() + ".";

        if (n < m)
            text << " " << joinList (outNames) << (outNames.size() == 1 ? " isn't" : " aren't") << ": let them ring only when you mean to.";
        else
            text << " Open chords and ringing strings will fit.";

        advice.add (text);

        // 2. Capo.
        if (isFrettedTuning (tuning) && 2 * n <= m)
        {
            const int fret = getBestCapoFret (tuning, key);
            const auto capoFit = getKeyFit (tuning, key, fret);
            const int n2 = (int) capoFit.inKey.size();

            if (n2 >= m - 1)
            {
                MusicalKey shapes = key;
                shapes.tonic = ((key.tonic - fret) % 12 + 12) % 12;
                juce::StringArray notes;

                for (const auto& s : strings)
                    notes.add (getNoteLetter (s.midi + fret));

                advice.add ("Try a capo on fret " + juce::String (fret) + " and play the shapes of " + shapes.getName() + ": the open strings become "
                            + notes.joinIntoString (", ") + ", " + juce::String (n2) + " of them in " + key.getName() + ".");
            }
        }

        // 3. Low notes.
        const bool bass = isBassTuning (tuning), guitar = tuning == Tuning::guitarStandard || tuning == Tuning::dropD
                                                          || tuning == Tuning::dadgad || tuning == Tuning::halfStepDown;
        const auto lowest = *std::min_element (strings.begin(), strings.end(), [] (auto& a, auto& b) { return a.midi < b.midi; });

        for (const auto& track : tracks)
        {
            const auto name = track.name.toLowerCase();
            const bool matches = (bass && name.contains ("bass")) || (guitar && (name.contains ("guitar") || name.contains ("gtr")));

            if (! matches || track.lowestMidi < 0 || track.lowestMidi >= lowest.midi)
                continue;

            const int down = lowest.midi - track.lowestMidi;
            const auto note = getNoteName (track.lowestMidi);
            auto line = "The " + track.name + " track goes down to " + note + " (" + juce::String (getHz (track.lowestMidi), 1) + " Hz), below the lowest string, the "
                        + lowest.label + " string. Tune that string down to " + note + " (" + plural (down, "semitone") + ")";

            if (tuning == Tuning::bass4)
                line << " or use a five-string bass";
            else if (tuning == Tuning::guitarStandard || tuning == Tuning::halfStepDown)
                line << " or use a drop tuning";

            advice.add (line + ".");
        }
    }

    // 4. Reference.
    if (std::abs (referenceHz - 440.0) > 1.0e-6)
    {
        const double cents = getReferenceCents (referenceHz);
        advice.add ("Your reference is A4 = " + describeHz (referenceHz) + " Hz, " + juce::String (std::abs (cents), 1) + " cents "
                    + (cents > 0.0 ? "above" : "below") + " DawIt's instruments, which play at A4 = 440 Hz. Tuned to it, your instrument will sound "
                    + (cents > 0.0 ? "sharp" : "flat") + " against the song. Use 440 Hz unless you play along with a recording or musicians tuned differently.");
    }

    // 5. Last readings.
    if (m > 0)
    {
        juce::StringArray heard;

        for (int i = 0; i < m; ++i)
            for (const auto& r : lastReadings)
                if (r.stringIndex == i)
                    heard.add (strings[(size_t) i].label + " " + describeLastReading (r.cents));

        advice.add (heard.isEmpty() ? juce::String ("Open the tuner (Cmd+7) and play each string once: this page then shows how far each one was.")
                                    : "Last heard: " + heard.joinIntoString (", ") + ".");
    }

    return advice;
}
}
