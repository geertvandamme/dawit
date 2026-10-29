#include "ChordDiagrams.h"
#include "ChordStyles.h"

namespace daw::gen::diagrams
{
namespace
{
    /** The chord's notes as pitch classes in close order from the root: root, (2nd/9th), 3rd, 4th, 5th, 6th, 7th. */
    std::vector<int> getCloseOrder (const Chord& chord)
    {
        std::vector<int> intervals;
        for (int i : chord.intervals)
            if (std::find (intervals.begin(), intervals.end(), i % 12) == intervals.end())
                intervals.push_back (i % 12);

        std::sort (intervals.begin(), intervals.end());

        std::vector<int> pitchClasses;
        for (int i : intervals)
            pitchClasses.push_back ((chord.root + i) % 12);

        return pitchClasses;
    }

    /** The close voicing of an inversion with its lowest note at or above `lowest`. */
    std::vector<int> stack (const Chord& chord, int inversion, int lowest)
    {
        const auto order = getCloseOrder (chord);
        std::vector<int> notes;

        if (order.empty())
            return notes;

        for (size_t i = 0; i < order.size(); ++i)
        {
            const int pitchClass = order[(i + (size_t) inversion) % order.size()];
            int pitch = notes.empty() ? lowest : notes.back() + 1;
            while ((pitch % 12 + 12) % 12 != pitchClass)
                ++pitch;

            notes.push_back (pitch);
        }

        return notes;
    }

    /** How far the hand moves: each note to the nearest note of the other chord, both ways. */
    int distance (const std::vector<int>& a, const std::vector<int>& b)
    {
        auto oneWay = [] (const std::vector<int>& from, const std::vector<int>& to)
        {
            int total = 0;
            for (int p : from)
            {
                int nearest = 128;
                for (int q : to)
                    nearest = juce::jmin (nearest, std::abs (p - q));
                total += nearest;
            }
            return total;
        };

        return oneWay (a, b) + oneWay (b, a);
    }

    /** The smoothest inversion after `previous`, in whichever octave keeps it near middle C. */
    std::pair<int, std::vector<int>> smoothest (const Chord& chord, const std::vector<int>& previous)
    {
        if (previous.empty())
            return { 0, pianoInversion (chord, 0) };

        std::pair<int, std::vector<int>> best { 0, {} };
        int bestDistance = 0;

        for (int inversion = 0; inversion < getNumInversions (chord); ++inversion)
        {
            for (int lowest = 52; lowest <= 66; lowest += 12)   // the lowest note from E3 up to F#4
            {
                const auto notes = stack (chord, inversion, lowest);
                if (notes.front() > 69)
                    continue;

                const int d = distance (notes, previous);
                if (best.second.empty() || d < bestDistance)
                {
                    best = { inversion, notes };
                    bestDistance = d;
                }
            }
        }

        return best;
    }

    juce::String stringNumber (int index) { return juce::String (6 - index); }   // guitarists count from the high E

    juce::String joinStrings (const juce::StringArray& numbers)
    {
        if (numbers.size() <= 1)
            return numbers.joinIntoString ({});

        return numbers.joinIntoString (", ", 0, numbers.size() - 1) + " and " + numbers[numbers.size() - 1];
    }

    juce::String countWord (int n)
    {
        static const char* words[] = { "no", "one", "two", "three", "four", "five", "six" };
        return n >= 0 && n <= 6 ? juce::String (words[n]) : juce::String (n);
    }
}

//==============================================================================
std::vector<int> pianoInversion (const Chord& chord, int inversion)
{
    const auto order = getCloseOrder (chord);

    if (order.empty())
        return {};

    // Start each inversion's lowest note from G3 up to F#4, so it sits round middle C.
    return stack (chord, juce::jlimit (0, (int) order.size() - 1, inversion), 55);
}

int getNumInversions (const Chord& chord)
{
    return (int) getCloseOrder (chord).size();
}

juce::String getInversionName (int inversion)
{
    switch (inversion)
    {
        case 0:  return "Root position";
        case 1:  return "1st inversion";
        case 2:  return "2nd inversion";
        case 3:  return "3rd inversion";
        default: return juce::String (inversion) + "th inversion";
    }
}

int findSmoothInversion (const Chord& chord, const std::vector<int>& previous)
{
    return smoothest (chord, previous).first;
}

int pianoBass (const Chord& chord)
{
    // C3 to F#3, or G2 to B2, so it stays below the right hand.
    return chord.root <= 6 ? 48 + chord.root : 36 + chord.root;
}

std::vector<std::vector<int>> voiceProgression (const std::vector<Chord>& chords)
{
    std::vector<std::vector<int>> voicings;

    for (auto& chord : chords)
        voicings.push_back (smoothest (chord, voicings.empty() ? std::vector<int>() : voicings.back()).second);

    return voicings;
}

//==============================================================================
bool GuitarShape::isOpen() const
{
    return std::find (frets.begin(), frets.end(), 0) != frets.end();
}

int GuitarShape::getBaseFret() const
{
    int lowest = 99, highest = 0;
    for (int f : frets)
    {
        if (f > 0)
            lowest = juce::jmin (lowest, f);
        highest = juce::jmax (highest, f);
    }

    return highest <= 4 ? 1 : lowest;
}

juce::String GuitarShape::getTab() const
{
    const bool wide = std::any_of (frets.begin(), frets.end(), [] (int f) { return f >= 10; });
    juce::StringArray parts;

    for (int f : frets)
        parts.add (f < 0 ? juce::String ("x") : juce::String (f));

    return parts.joinIntoString (wide ? " " : "");
}

GuitarShape makeGuitarShape (const std::vector<int>& frets)
{
    GuitarShape shape;
    shape.frets = frets;
    shape.fingers.assign (frets.size(), 0);

    int lowestFret = 99, highestFret = 0, fretted = 0, highestString = -1;
    for (size_t s = 0; s < frets.size(); ++s)
    {
        if (frets[s] > 0)
        {
            lowestFret = juce::jmin (lowestFret, frets[s]);
            highestFret = juce::jmax (highestFret, frets[s]);
            ++fretted;
        }

        if (frets[s] >= 0)
            highestString = (int) s;
    }

    if (fretted == 0)
        return shape;

    // The index finger lies across the strings at the lowest fret, from the lowest one there up to
    // the top string played, when that top string is fretted there and nothing in between is open.
    // It is only worth it when it saves a finger: more than four notes, or two neighbouring strings.
    int barreFrom = -1, atLowest = 0;
    for (size_t s = 0; s < frets.size(); ++s)
    {
        if (frets[s] == lowestFret)
        {
            ++atLowest;
            if (barreFrom < 0)
                barreFrom = (int) s;
        }
    }

    bool canBarre = atLowest >= 2 && frets[(size_t) highestString] == lowestFret;
    for (int s = barreFrom; canBarre && s <= highestString; ++s)
        canBarre = frets[(size_t) s] != 0;

    const bool neighbours = canBarre && frets[(size_t) highestString - 1] == lowestFret;

    if (canBarre && (fretted > 4 || neighbours))
    {
        shape.barreFret = lowestFret;
        shape.barreFrom = barreFrom;
        shape.barreTo = highestString;
    }

    // The other notes get the next fingers, by fret and then from the low strings up.
    std::vector<std::pair<int, int>> notes;   // fret, string
    for (size_t s = 0; s < frets.size(); ++s)
    {
        if (frets[s] <= 0)
            continue;

        if (shape.hasBarre() && frets[s] == lowestFret)
            shape.fingers[s] = 1;
        else
            notes.emplace_back (frets[s], (int) s);
    }

    std::sort (notes.begin(), notes.end());
    int finger = shape.hasBarre() ? 2 : 1;

    for (auto& [fret, string] : notes)
        shape.fingers[(size_t) string] = juce::jmin (4, finger++);

    shape.playable = finger - 1 <= 4;

    const int fingersUsed = finger - 1;
    const int barreStrings = shape.hasBarre() ? shape.barreTo - shape.barreFrom + 1 : 0;
    const int stretch = highestFret - lowestFret;

    if (barreStrings >= 4)
        shape.difficulty = Difficulty::hard;
    else if (barreStrings > 0 || fingersUsed >= 4 || stretch >= 3 || ! shape.isOpen())
        shape.difficulty = Difficulty::medium;
    else
        shape.difficulty = Difficulty::easy;

    return shape;
}

std::vector<GuitarShape> getGuitarShapes (const Chord& chord, int maxShapes)
{
    std::vector<GuitarShape> shapes;

    auto playable = [] (const std::vector<int>& frets) { return makeGuitarShape (frets).isPlayable(); };

    for (auto& frets : styles::findGuitarShapes (chord, maxShapes, playable))
        shapes.push_back (makeGuitarShape (frets));

    return shapes;
}

std::vector<GuitarShape> shapeProgression (const std::vector<Chord>& chords)
{
    std::vector<GuitarShape> shapes;
    std::vector<int> previous;

    for (auto& chord : chords)
    {
        auto frets = styles::findGuitarShape (chord, previous.empty() ? nullptr : &previous);
        shapes.push_back (frets.empty() ? GuitarShape() : makeGuitarShape (frets));
        previous = frets;
    }

    return shapes;
}

juce::String getDifficultyName (Difficulty difficulty)
{
    switch (difficulty)
    {
        case Difficulty::easy:   return "Easy";
        case Difficulty::medium: return "Medium";
        case Difficulty::hard:   return "Hard";
    }

    return {};
}

juce::String describeGuitarShape (const GuitarShape& shape)
{
    if (shape.isEmpty())
        return "No easy shape for this chord.";

    int fingers = 0;
    juce::StringArray muted;

    for (size_t s = 0; s < shape.frets.size(); ++s)
    {
        fingers = juce::jmax (fingers, shape.fingers[s]);

        if (shape.frets[s] < 0)
            muted.add (stringNumber ((int) s));
    }

    juce::String text;

    if (shape.hasBarre())
    {
        const int strings = shape.barreTo - shape.barreFrom + 1;
        text << (strings >= 5 ? "A barre chord: the index finger holds strings " : "The index finger lies flat across strings ")
             << stringNumber (shape.barreFrom) << " to " << stringNumber (shape.barreTo)
             << " at fret " << shape.barreFret << ", and " << countWord (fingers - 1) << " more "
             << (fingers - 1 == 1 ? "finger" : "fingers") << " press the rest.";
    }
    else if (shape.isOpen())
    {
        text << "An open chord with " << countWord (fingers) << (fingers == 1 ? " finger." : " fingers.");
    }
    else
    {
        text << "A movable shape at fret " << shape.getBaseFret() << " with " << countWord (fingers)
             << " fingers: slide it up or down the neck for other chords of the same kind.";
    }

    if (! muted.isEmpty())
        text << " Don't play " << (muted.size() == 1 ? "string " : "strings ") << joinStrings (muted) << ".";

    return text;
}

//==============================================================================
juce::String explainPlaying (const Chord& chord)
{
    const auto name = chord.getName();
    const auto noteNames = juce::StringArray::fromTokens (chord.getNoteNames(), " ", {});
    const int inversions = getNumInversions (chord);
    juce::String text;

    text << "How to play " << name << "\n\n"
         << "On piano: " << name << " has " << countWord (noteNames.size()) << " notes, " << joinStrings (noteNames) << ". "
         << "With the right hand, play them as one hand shape, every other white key for most chords. "
         << "In root position the root (" << noteNames[0] << ") is the lowest note. Moving the lowest note to the top gives the 1st inversion";

    if (inversions > 1)
        text << " (" << noteNames[1 % noteNames.size()] << " at the bottom)";

    text << ", and again the 2nd inversion";

    if (inversions > 3)
        text << ", and with a seventh chord a 3rd inversion";

    text << ". All of them are still " << name << ": the same notes, stacked differently.\n\n"
         << "Why the root goes at the bottom: the lowest note you hear decides what a chord sounds like most. "
         << "So the left hand plays the root, " << noteNames[0] << ", lower down, and the chord stays "
         << name << " whichever inversion the right hand plays. "
         << "In a progression, pick the inversion that moves your hand the least: notes that two chords share stay where they are, "
         << "and the others move a step. It sounds smoother and is easier to play.\n\n";

    const auto shapes = getGuitarShapes (chord);

    if (shapes.empty())
    {
        text << "On guitar there is no comfortable shape for " << name << "; guitarists leave out a note, usually the fifth.";
        return text;
    }

    const auto& easiest = shapes.front();
    text << "On guitar (" << easiest.getTab() << ", from the low E string): " << describeGuitarShape (easiest) << " "
         << getDifficultyName (easiest.difficulty) << ". "
         << "In the chord box the strings run from the low E on the left to the high E on the right, "
         << "o is an open string, x a string you don't play, and the numbers are the fingers: 1 index, 2 middle, 3 ring, 4 little.";

    if (shapes.size() > 1)
        text << " The other boxes play the same chord further up the neck.";

    text << " Tune first: View > Tuner... (Cmd+7) checks each string, and its guide, How to tune a guitar, explains how.";

    text << "\n\n";

    if (easiest.difficulty == Difficulty::hard || ! easiest.isOpen())
    {
        text << "Why " << name << " is hard: there is no open shape for it, so one finger has to hold down several strings at once (a barre). "
             << "That takes practice and strength, so beginners usually learn the open chords first (C, A, G, E, D, Am, Em, Dm). ";

        // A capo turns an easy open shape into this chord.
        for (int capo = 1; capo <= 7; ++capo)
        {
            const Chord shapeChord { (chord.root - capo + 12) % 12, chord.intervals };
            const auto frets = styles::findGuitarShape (shapeChord);

            if (! frets.empty() && makeGuitarShape (frets).difficulty == Difficulty::easy)
            {
                text << "Or use a capo: clamp it on fret " << capo << " and play the easy " << shapeChord.getName()
                     << " shape; with the capo it sounds as " << name << ". ";
                break;
            }
        }

        text << "Players also make it easier by playing only the top three or four strings, or by leaving out the low strings.";
    }
    else
    {
        text << "This is one of the shapes to learn first. Practise changing to it from the chords around it in your song: "
                "keep the fingers that stay on the same frets down, and move the others together.";
    }

    return text;
}
} // namespace daw::gen::diagrams
