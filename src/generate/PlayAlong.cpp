#include "PlayAlong.h"
#include "engine/Tempo.h"
#include "ChordDiagrams.h"

#include <set>

namespace daw::gen::playalong
{
namespace
{
    /** The change playing in a bar, or -1. */
    int changeAtBar (const std::vector<Change>& changes, int bar)
    {
        const auto it = std::upper_bound (changes.begin(), changes.end(), bar, [] (int b, const Change& c) { return b < c.endBar; });
        return it != changes.end() && it->startBar <= bar ? (int) (it - changes.begin()) : -1;
    }

    int barOf (double beat)
    {
        return (int) std::floor (beat / 4.0 + 1.0e-9);
    }

    /** The easiest guitar shape a chord can be played with. */
    diagrams::GuitarShape easiestShape (const Chord& chord)
    {
        const auto shapes = diagrams::getGuitarShapes (chord, 1);
        return shapes.empty() ? diagrams::GuitarShape() : shapes.front();
    }

    /** An open chord: in the first frets, without a barre. */
    bool isOpenShape (const diagrams::GuitarShape& shape)
    {
        return ! shape.isEmpty() && ! shape.hasBarre() && shape.getBaseFret() == 1;
    }

    std::vector<Chord> getDifferentChords (const std::vector<Change>& changes)
    {
        std::vector<Chord> chords;
        for (auto& c : changes)
            if (std::find (chords.begin(), chords.end(), c.chord) == chords.end())
                chords.push_back (c.chord);

        return chords;
    }

    int countOpenShapes (const std::vector<Chord>& chords, int capo)
    {
        return (int) std::count_if (chords.begin(), chords.end(), [capo] (const Chord& c) { return isOpenShape (easiestShape (getShapeChord (c, capo))); });
    }

    juce::String formatSeconds (double seconds)
    {
        return juce::String (seconds, seconds < 10.0 ? 1 : 0) + " seconds";
    }
}

std::vector<Change> getChanges (const ChordMap& chords)
{
    std::vector<Change> changes;

    for (auto it = chords.begin(); it != chords.end();)
    {
        Change change;
        change.startBar = it->first;
        change.chord = it->second;
        change.endBar = change.startBar + 1;

        for (++it; it != chords.end() && it->first == change.endBar && it->second == change.chord; ++it)
            ++change.endBar;

        changes.push_back (change);
    }

    return changes;
}

Moment findMoment (const std::vector<Change>& changes, const SectionList& sections, double beat, const Loop& loop, int numComing)
{
    Moment m;
    beat = juce::jmax (0.0, beat);
    m.bar = barOf (beat);
    m.beatInBar = juce::jlimit (0, 3, (int) std::floor (beat - m.bar * 4.0 + 1.0e-9));
    m.current = changeAtBar (changes, m.bar);

    for (size_t i = 0; i < sections.size(); ++i)
    {
        if (m.bar >= sections[i].startBar && m.bar < sections[i].getEndBar())
        {
            m.section = (int) i;
            m.barInSection = m.bar - sections[i].startBar;
            break;
        }
    }

    // Walk on beat by beat, from the next beat, as the transport will play: round the loop
    // when the playhead is before its end.
    const bool wraps = loop.enabled && loop.endBeat - loop.startBeat > 0.01 && beat < loop.endBeat;
    const bool inLoop = wraps && beat >= loop.startBeat;
    const double songEndBeat = changes.empty() ? 0.0 : changes.back().endBar * 4.0;
    constexpr int maxSteps = 4096;

    double t = std::floor (beat + 1.0e-9) + 1.0, d = t - beat;
    int steps = 0;

    const auto wrap = [&]
    {
        if (wraps && t >= loop.endBeat - 1.0e-9)
        {
            d -= t - loop.endBeat;
            t = loop.startBeat;
        }
    };

    const auto advance = [&]
    {
        t += 1.0;
        d += 1.0;
        ++steps;
        wrap();
    };

    // Past the last chord nothing changes any more, unless the loop goes back.
    const auto pastTheEnd = [&] { return ! wraps && t >= songEndBeat; };
    const auto at = [&] { return changeAtBar (changes, barOf (t)); };

    wrap();

    while (steps < maxSteps && ! (m.current < 0 && pastTheEnd()) && at() == m.current)
        advance();

    if (steps >= maxSteps || (m.current < 0 && pastTheEnd()))
        return m;

    m.beatsLeft = d;

    // How long the current chord (or the silence) has been going: from its first bar, or the
    // loop's start when it plays round the loop.
    double runStart = 0.0;

    if (m.current >= 0)
        runStart = changes[(size_t) m.current].startBar * 4.0;
    else if (const auto it = std::upper_bound (changes.begin(), changes.end(), m.bar, [] (int b, const Change& c) { return b < c.endBar; });
             it != changes.begin())
        runStart = std::prev (it)->endBar * 4.0;

    if (inLoop)
        runStart = juce::jmax (runStart, loop.startBeat);

    m.runBeats = beat - runStart + m.beatsLeft;

    // The next chord: the first bar with one after the current chord ends.
    while (steps < maxSteps && ! pastTheEnd() && at() < 0)
        advance();

    if (steps >= maxSteps || pastTheEnd())
        return m;

    m.next = at();
    m.beatsToNext = d;
    m.coming.push_back ({ m.next, d });

    for (int last = m.next; (int) m.coming.size() < numComing;)
    {
        advance();

        if (steps >= maxSteps || pastTheEnd())
            break;

        if (const int c = at(); c != last)
        {
            if (c >= 0)
                m.coming.push_back ({ c, d });

            last = c;
        }
    }

    return m;
}

juce::String describeWait (double beats)
{
    if (beats < 0.05)
        return "now";

    if (beats < 8.0 - 1.0e-6)
    {
        const int n = juce::jmax (1, (int) std::ceil (beats - 1.0e-6));
        return n == 1 ? "in 1 beat" : "in " + juce::String (n) + " beats";
    }

    return "in " + juce::String ((int) std::ceil (beats / 4.0 - 1.0e-6)) + " bars";
}

juce::String describePosition (const Moment& m, const SectionList& sections)
{
    if (m.section >= 0 && m.section < (int) sections.size())
    {
        const auto& s = sections[(size_t) m.section];
        return s.getDisplayName() + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 bar ")) + juce::String (m.barInSection + 1) + " of " + juce::String (s.bars);
    }

    return "Bar " + juce::String (m.bar + 1);
}

bool isAboutToChange (const Moment& m)
{
    return m.next >= 0 && m.beatsToNext > 0.0 && m.beatsToNext <= 1.0 + 1.0e-6;
}

//==============================================================================
Chord getShapeChord (const Chord& chord, int capo)
{
    return { ((chord.root - capo) % 12 + 12) % 12, chord.intervals };
}

juce::String describeCapo (const Chord& chord, int capo)
{
    if (capo <= 0)
        return {};

    return "Capo " + juce::String (capo) + ": play the " + getShapeChord (chord, capo).getName() + " shape, it sounds as " + chord.getName();
}

int findEasiestCapo (const std::vector<Change>& changes)
{
    const auto chords = getDifferentChords (changes);
    int best = 0, bestOpen = countOpenShapes (chords, 0);

    for (int capo = 1; capo <= 7; ++capo)
        if (const int open = countOpenShapes (chords, capo); open > bestOpen)
        {
            best = capo;
            bestOpen = open;
        }

    return best;
}

//==============================================================================
juce::String explainPlayingAlong (const std::vector<Change>& changes, double bpm)
{
    juce::String text =
        "Playing along with a song is how most musicians learn: the band keeps going, and you fit in. The play-along window "
        "shows the chord being played right now, large enough to read from across the room, with the next chord beside it. "
        "The next chord lights up on the last beat before the change, and a bar under the chord runs out over the beats until "
        "it, with the beats of the bar as dots (1 2 3 4), so you know when to move.\n\n"
        "Tips from teachers:\n"
        "- Look at the next chord, not the one you're playing. Your hand already knows the current one.\n"
        "- Move only the fingers that change. Keys and fingers that stay are ringed green: keep them down (anchor fingers).\n"
        "- Keep the rhythm going, even when a change is late. Strum the open strings or play just the bass note rather than "
        "stopping: the band doesn't wait, and the next bar comes anyway.\n"
        "- Practise the hardest change on its own first, slowly, back and forth. Loop the bars it's in (drag along the ruler).\n"
        "- Start with songs of three or four chords, such as the I-V-vi-IV pop progression (C G Am F), before ones with barre chords.\n"
        "- Mute the track you're playing yourself, and play its part with the rest of the band.\n";

    if (changes.empty())
        return text + "\nYour song has no chords yet. Put chords on the Chords row, pick a progression here and put it on the chord "
                      "track, or detect the chords of a recording (right-click an audio clip, Detect chords...).";

    const auto chords = getDifferentChords (changes);
    juce::StringArray names;
    for (auto& c : chords)
        names.add (c.getName());

    text << "\nYour song: " << juce::String ((int) chords.size()) << (chords.size() == 1 ? " chord, " : " different chords, ")
         << names.joinIntoString (" ") << ".";

    // The quickest change, where you have least time to move.
    const auto quickest = std::min_element (changes.begin(), changes.end(), [] (const Change& a, const Change& b) { return a.getBars() < b.getBars(); });
    const double seconds = quickest->getBars() * 4.0 * 60.0 / juce::jmax (1.0, bpm);
    text << " The quickest change comes after " << juce::String (quickest->getBars()) << (quickest->getBars() == 1 ? " bar" : " bars")
         << " of " << quickest->chord.getName() << " in bar " << juce::String (quickest->startBar + 1) << ": " << formatSeconds (seconds)
         << " at " << formatBpm (bpm) << " BPM.";

    // On guitar: the barre chords, and a capo that makes them open chords.
    juce::StringArray barres;
    for (auto& c : chords)
        if (const auto shape = easiestShape (c); ! isOpenShape (shape))
            barres.add (c.getName());

    if (barres.isEmpty())
    {
        text << " On guitar, every chord is an open chord, without a barre: a good song to start with.";
    }
    else
    {
        text << " On guitar, " << barres.joinIntoString (", ") << (barres.size() == 1 ? " needs" : " need")
             << " a barre or a shape higher up the neck.";

        if (const int capo = findEasiestCapo (changes); capo > 0)
        {
            text << " With a capo on fret " << juce::String (capo) << ", " << juce::String (countOpenShapes (chords, capo)) << " of the "
                 << juce::String ((int) chords.size()) << " chords are open shapes (" << describeCapo (chords.front(), capo)
                 << "). Pick the capo in the play-along window and it shows the shapes to play.";
        }
    }

    return text;
}
} // namespace daw::gen::playalong
