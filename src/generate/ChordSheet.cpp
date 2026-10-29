#include "ChordSheet.h"

namespace daw::gen::sheet
{
namespace
{
    const char* letters = "CDEFGAB";
    constexpr int naturals[] = { 0, 2, 4, 5, 7, 9, 11 };

    bool usesFlats (const MusicalKey& key)
    {
        const auto signature = guide::getKeySignature (guide::getCirclePosition (key));
        return ! signature.isEmpty() && signature[0].endsWithChar ('b');
    }

    /** The letter (0 = C ... 6 = B) of the key's tonic. */
    int getTonicLetter (const MusicalKey& key)
    {
        static const char* sharps[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        static const char* flats[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        const auto name = juce::String ((usesFlats (key) ? flats : sharps)[key.tonic]);
        return juce::String (letters).indexOfChar (name[0]);
    }

    /** A pitch class written with this letter: 1 with D is "Db", with C "C#". */
    juce::String spellNote (int pitchClass, int letter)
    {
        const int accidental = ((pitchClass - naturals[letter]) % 12 + 18) % 12 - 6;
        return juce::String::charToString (letters[letter]) + juce::String::repeatedString (accidental > 0 ? "#" : "b", std::abs (accidental));
    }

    /** A chord spelled from the letter (0 = C ... 6 = B) of its root. */
    Entry spellEntry (const MusicalKey& key, int rootLetter, const Chord& chord, const juce::String& role)
    {
        const auto suffix = chord.getName().substring (MusicalKey::getTonicName (chord.root).length());
        juce::StringArray notes;

        for (int i : chord.intervals)
        {
            // Letters above the root: 2nds/9ths 1, 3rds 2, 4ths 3, 5ths 4, 7ths 6 (a dim7's 9 semitones is a bb7).
            static const int steps[] = { 0, 1, 1, 2, 2, 3, 4, 4, 4, 6, 6, 6 };
            notes.addIfNotAlreadyThere (spellNote ((chord.root + i) % 12, (rootLetter + steps[i % 12]) % 7));
        }

        return { chord, spellNote (chord.root, rootLetter) + suffix, notes.joinIntoString (" "), guide::getNumeral (key, chord), role,
                 guide::getFamily (key, chord) };
    }

    /** A chord whose root is on scale degree `degree` (1-7) of the key, spelled from that degree's letter. */
    Entry makeEntry (const MusicalKey& key, int degree, const Chord& chord, const juce::String& role)
    {
        return spellEntry (key, (getTonicLetter (key) + degree - 1) % 7, chord, role);
    }

    /** How hard the chords are on guitar: a total of 0 for each easy shape, 1 for medium and 2 for hard
        (or none at all), and how many are easy. */
    std::pair<int, int> rateShapes (const std::vector<Chord>& chords)
    {
        int total = 0, easy = 0;

        for (auto& chord : chords)
        {
            const auto shapes = diagrams::getGuitarShapes (chord, 1);
            const int level = shapes.empty() ? 2 : (int) shapes.front().difficulty;
            total += level;
            easy += level == 0 ? 1 : 0;
        }

        return { total, easy };
    }

    std::vector<Chord> getTriads (const MusicalKey& key)
    {
        std::vector<Chord> chords;

        for (int degree = 1; degree <= 7; ++degree)
            chords.push_back (harmony::triad (key, degree));

        return chords;
    }
}

Sheet make (const MusicalKey& key)
{
    Sheet s;
    s.key = key;
    s.keyName = getKeyName (key);

    const int position = guide::getCirclePosition (key);
    const auto signature = guide::getKeySignature (position);
    s.keySignature = guide::describeKeySignature (position) + (signature.isEmpty() ? juce::String() : ": " + signature.joinIntoString (" "));

    for (int degree = 1; degree <= 7; ++degree)
    {
        const auto triad = harmony::triad (key, degree);
        s.chords.push_back (makeEntry (key, degree, triad, harmony::getFunction (degree)));

        // A seventh on the half-diminished chord is written with a slashed circle: Bm7b5 is viiø7.
        Entry seventh = makeEntry (key, degree, { triad.root, getChordIntervals (key, degree, "seventh") }, harmony::getFunction (degree));
        seventh.numeral = harmony::getRomanNumeral (degree, seventh.chord) + "7";

        if (seventh.chord.intervals.contains (6) && seventh.chord.intervals.contains (10))
            seventh.numeral = seventh.numeral.replace (juce::CharPointer_UTF8 ("\xc2\xb0"), juce::CharPointer_UTF8 ("\xc3\xb8"));

        s.sevenths.push_back (seventh);
    }

    if (const auto dominant = harmony::getBorrowedDominant (key))
    {
        // In a minor key, the dominant is usually made major, from harmonic minor.
        s.borrowed.push_back (makeEntry (key, 5, *dominant, "from harmonic minor"));
        auto seventh = makeEntry (key, 5, { dominant->root, { 0, 4, 7, 10 } }, "from harmonic minor");
        seventh.numeral = "V7";
        s.borrowed.push_back (seventh);
    }
    else if (key.scale == MusicalKey::Scale::major)
    {
        // The chords a major key most often borrows from its minor: the minor four, and the major chords on the flat six and seven.
        s.borrowed.push_back (makeEntry (key, 4, { (key.tonic + 5) % 12, { 0, 3, 7 } }, "from the minor key"));
        s.borrowed.push_back (makeEntry (key, 6, { (key.tonic + 8) % 12, { 0, 4, 7 } }, "from the minor key"));
        s.borrowed.push_back (makeEntry (key, 7, { (key.tonic + 10) % 12, { 0, 4, 7 } }, "from the minor key"));
    }

    for (auto& progression : guide::getCommonProgressions (key))
    {
        ProgressionRow row { progression.name, progression.summary, {}, {} };
        juce::StringArray numerals;

        for (auto& step : progression.steps)
        {
            row.chords.push_back (makeEntry (key, step.degree, guide::buildChord (key, step), {}));
            numerals.add (row.chords.back().numeral);
        }

        row.numerals = numerals.joinIntoString (" - ");
        s.progressions.push_back (std::move (row));
    }

    return s;
}

Entry describe (const MusicalKey& key, const Chord& chord)
{
    if (const int degree = harmony::findDegree (key, chord); degree > 0)
        return makeEntry (key, degree, chord, harmony::getFunction (degree));

    // Outside the key: flats unless the key is written with sharps.
    static const char* flats[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    static const char* sharps[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const bool sharpKey = ! usesFlats (key) && ! guide::getKeySignature (guide::getCirclePosition (key)).isEmpty();
    const auto name = juce::String ((sharpKey ? sharps : flats)[chord.root]);
    return spellEntry (key, juce::String (letters).indexOfChar (name[0]), chord, {});
}

juce::String getKeyName (const MusicalKey& key)
{
    return spellNote (key.tonic, getTonicLetter (key)) + " " + MusicalKey::getScaleName (key.scale);
}

Capo findCapo (const MusicalKey& key)
{
    const auto [totalWithout, easyWithout] = rateShapes (getTriads (key));
    Capo best { 0, key, easyWithout, easyWithout };
    int bestTotal = totalWithout;

    for (int fret = 1; fret <= 7; ++fret)
    {
        // With a capo on this fret, every shape sounds `fret` semitones higher, so the shapes are those of the key that much lower.
        MusicalKey shapesKey = key;
        shapesKey.tonic = ((key.tonic - fret) % 12 + 12) % 12;
        const auto [total, easy] = rateShapes (getTriads (shapesKey));

        if (total < bestTotal)
        {
            best = { fret, shapesKey, easyWithout, easy };
            bestTotal = total;
        }
    }

    // Only worth it when it makes clearly more of the chords easy: a capo changes how the guitar sounds too.
    if (totalWithout - bestTotal < 3)
        return { 0, key, easyWithout, easyWithout };

    return best;
}
} // namespace daw::gen::sheet
