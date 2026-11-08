#include "MidiQuantise.h"

#include "MidiTransforms.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>

namespace daw::quantise
{
const std::vector<Grid>& getGrids()
{
    static const std::vector<Grid> grids {
        { "1/4",   "1/4 (quarter note)",         1.0 },
        { "1/8",   "1/8 (eighth note)",          0.5 },
        { "1/16",  "1/16 (sixteenth note)",      0.25 },
        { "1/32",  "1/32 (thirty-second note)",  0.125 },
        { "1/4t",  "1/4 triplet",                2.0 / 3.0 },
        { "1/8t",  "1/8 triplet",                1.0 / 3.0 },
        { "1/16t", "1/16 triplet",               1.0 / 6.0 },
    };

    return grids;
}

int findGrid (const juce::String& key)
{
    const auto& grids = getGrids();

    for (int i = 0; i < (int) grids.size(); ++i)
        if (grids[(size_t) i].key == key)
            return i;

    return 2;
}

juce::String getGridKey (double beats)
{
    for (auto& g : getGrids())
        if (std::abs (g.beats - beats) < 1.0e-9)
            return g.key;

    return {};
}

//==============================================================================
namespace
{
    struct Lines
    {
        double lower = 0.0, upper = 0.0;
    };

    /** The grid lines either side of the song position `s`: the bar's start plus whole steps, the next bar's start closing the bar. */
    Lines findLines (double s, double step, double barLength)
    {
        const double barStart = std::floor (s / barLength + 1.0e-12) * barLength;
        const double r = std::max (0.0, s - barStart);
        const double k = std::floor (r / step + 1.0e-12);
        Lines lines { barStart + k * step, std::min (barStart + (k + 1.0) * step, barStart + barLength) };

        if (lines.lower > barStart + barLength - 1.0e-9)
            lines = { barStart + barLength, barStart + barLength + std::min (step, barLength) };

        return lines;
    }

    /** The nearest line to `s` (halfway goes to the later one), or nothing when `s` is already on one. */
    std::optional<double> findNearest (double s, double step, double barLength)
    {
        const auto lines = findLines (s, step, barLength);
        const double down = s - lines.lower, up = lines.upper - s;

        if (std::min (down, up) < 1.0e-6)
            return std::nullopt;

        if (std::abs (down - up) <= 1.0e-9)
            return lines.upper;

        return down < up ? lines.lower : lines.upper;
    }

    /** The smallest line at or after `s`. */
    double findLineAtOrAfter (double s, double step, double barLength)
    {
        const auto lines = findLines (s, step, barLength);
        return std::abs (lines.lower - s) < 1.0e-9 ? lines.lower : lines.upper;
    }

    /** A grid target in the clip's own beats for a song position, kept inside the clip; nothing when the note stays. */
    std::optional<double> findTarget (double local, double clipStart, double clipLength, double step, double barLength)
    {
        const auto nearest = findNearest (clipStart + local, step, barLength);

        if (! nearest)
            return std::nullopt;

        const double end = clipStart + clipLength;
        double target = *nearest;

        if (target >= end - 1.0e-9)
        {
            target = findLines (end - 1.0e-9, step, barLength).lower;

            if (target - clipStart < -1.0e-9)
                return std::nullopt;
        }
        else if (target - clipStart < -1.0e-9)
        {
            target = findLineAtOrAfter (clipStart, step, barLength);

            if (target >= end - 1.0e-9)
                return std::nullopt;
        }

        return target - clipStart;
    }
}

Notes apply (const Notes& notes, const Selection& selection, double clipStart, double clipLength, double barLength, const Settings& settings)
{
    auto result = notes;
    const double step = settings.gridBeats;
    const double strength = juce::jlimit (0.0, 1.0, settings.strength);
    auto affected = miditransform::getAffected (notes, selection, clipLength);

    if (affected.empty() || step <= 0.0 || barLength <= 0.0 || strength <= 0.0)
        return result;

    std::stable_sort (affected.begin(), affected.end(), [&notes] (int a, int b) { return notes[(size_t) a].start < notes[(size_t) b].start; });

    size_t i = 0;

    while (i < affected.size())
    {
        const double groupStart = notes[(size_t) affected[i]].start;
        const double lineOfGroup = findTarget (groupStart, clipStart, clipLength, step, barLength).value_or (groupStart);

        for (; i < affected.size() && notes[(size_t) affected[i]].start - groupStart < chordWindowBeats; ++i)
        {
            const auto& old = notes[(size_t) affected[i]];
            auto& n = result[(size_t) affected[i]];
            const double newStart = old.start + strength * (lineOfGroup - old.start);
            const double oldEnd = old.start + old.length;
            const bool endedInside = oldEnd <= clipLength + 1.0e-9;
            double length = old.length;

            if (settings.ends)
            {
                double endLine = findTarget (oldEnd, clipStart, clipLength, step, barLength).value_or (oldEnd);
                endLine = std::max (endLine, lineOfGroup + step);

                if (endedInside)
                    endLine = std::min (endLine, clipLength);

                const double newLength = oldEnd + strength * (endLine - oldEnd) - newStart;

                if (newLength > 1.0e-9)
                    length = newLength;
            }
            else if (endedInside && newStart + length > clipLength)
            {
                length = clipLength - newStart;
            }

            if (! juce::exactlyEqual (newStart, old.start))
                n.start = newStart;

            if (! juce::exactlyEqual (length, old.length))
                n.length = length;
        }
    }

    return result;
}

Summary summarise (const Notes& before, const Notes& after, const Selection& selection, double clipLength)
{
    Summary s;

    if (before.size() != after.size())
        return s;

    for (int i : miditransform::getAffected (before, selection, clipLength))
    {
        const auto& a = before[(size_t) i];
        const auto& b = after[(size_t) i];
        const double dStart = std::abs (b.start - a.start), dEnd = std::abs (b.getEnd() - a.getEnd());
        ++s.inScope;

        if (dStart > 1.0e-9 || dEnd > 1.0e-9)
        {
            ++s.moved;
            s.largestMove = std::max ({ s.largestMove, dStart, dEnd });
        }
    }

    // Notes of one pitch that now share a start but didn't.
    std::vector<int> order (after.size());
    std::iota (order.begin(), order.end(), 0);
    std::sort (order.begin(), order.end(), [&after] (int a, int b)
    {
        const auto& x = after[(size_t) a];
        const auto& y = after[(size_t) b];
        return x.pitch != y.pitch ? x.pitch < y.pitch : x.start < y.start;
    });

    std::vector<bool> colliding (after.size(), false);

    for (size_t k = 1; k < order.size(); ++k)
    {
        const int a = order[k - 1], b = order[k];

        if (after[(size_t) a].pitch == after[(size_t) b].pitch && std::abs (after[(size_t) a].start - after[(size_t) b].start) < 1.0e-9
            && std::abs (before[(size_t) a].start - before[(size_t) b].start) >= 1.0e-9)
            colliding[(size_t) a] = colliding[(size_t) b] = true;
    }

    s.collidingNotes = (int) std::count (colliding.begin(), colliding.end(), true);
    return s;
}

juce::String describe (const Summary& summary, const Settings& settings, double bpm)
{
    auto plain = [] (double v, int places)
    {
        auto text = juce::String (v, places);

        while (text.contains (".") && (text.endsWithChar ('0') || text.endsWithChar ('.')))
            text = text.dropLastCharacters (1);

        return text;
    };

    if (summary.moved == 0)
    {
        if (settings.strength <= 0.0)
            return "At 0% nothing moves.";

        const auto key = getGridKey (settings.gridBeats);
        return key.isEmpty() ? juce::String ("Nothing to move: every note is already on the grid.")
                             : "Nothing to move: every note is already on the " + key + " grid.";
    }

    juce::String text = juce::String (summary.moved) + " of " + juce::String (summary.inScope)
                        + (summary.inScope == 1 ? " note moves" : " notes move") + ", by up to " + juce::String (summary.largestMove, 2)
                        + " beats (" + juce::String (juce::roundToInt (summary.largestMove * 60000.0 / juce::jmax (1.0, bpm))) + " ms at "
                        + plain (bpm, 2) + " BPM).";

    if (summary.collidingNotes > 0)
        text << " " << summary.collidingNotes << " notes of the same pitch will start together; you may want to delete one.";

    return text;
}

bool sameNotes (const Notes& a, const Notes& b)
{
    return miditransform::sameNotes (a, b);
}
} // namespace daw::quantise
