#include "Takes.h"
#include "ChordStyles.h"

#include <map>
#include <set>

namespace daw::gen
{
namespace
{
    /** 1 inside [low, high], falling to 0 at `softness` outside it. */
    double band (double x, double low, double high, double softness)
    {
        if (x < low)
            return juce::jmax (0.0, 1.0 - (low - x) / softness);

        if (x > high)
            return juce::jmax (0.0, 1.0 - (x - high) / softness);

        return 1.0;
    }

    struct Check { double score, weight; };

    double combine (std::initializer_list<Check> checks)
    {
        double total = 0.0, weights = 0.0;

        for (auto& c : checks)
        {
            total += c.score * c.weight;
            weights += c.weight;
        }

        return weights > 0.0 ? total / weights : 0.0;
    }

    int stepOf (double beats) { return juce::roundToInt (beats * 4.0); }

    /** 0 when all velocities are the same, 1 from a spread of about 30 (of 127): accents and ghost notes. */
    double velocitySpread (const std::vector<MidiNote>& notes)
    {
        float lowest = 1.0f, highest = 0.0f;

        for (auto& n : notes)
        {
            lowest = juce::jmin (lowest, n.velocity);
            highest = juce::jmax (highest, n.velocity);
        }

        return juce::jlimit (0.0, 1.0, (highest - lowest) * 127.0 / 30.0);
    }

    /** Average overlap (0 to 1) of each bar's notes with the first bar's, by pitch and 16th step; -1 for one bar. */
    double barSimilarity (const std::vector<MidiNote>& notes, int bars)
    {
        if (bars < 2)
            return -1.0;

        std::vector<std::set<std::pair<int, int>>> perBar ((size_t) bars);

        for (auto& n : notes)
            perBar[(size_t) juce::jlimit (0, bars - 1, (int) (n.start / 4.0))].insert ({ n.pitch, stepOf (std::fmod (n.start, 4.0)) });

        double total = 0.0;

        for (int bar = 1; bar < bars; ++bar)
        {
            const auto& a = perBar[0];
            const auto& b = perBar[(size_t) bar];
            size_t shared = 0;

            for (auto& item : b)
                shared += a.count (item);

            const auto all = a.size() + b.size() - shared;
            total += all > 0 ? (double) shared / (double) all : 1.0;
        }

        return total / (bars - 1);
    }

    /** Weight 0 for a pattern of one bar, which has nothing to compare. */
    Check repetition (double similarity, double low, double high)
    {
        return similarity < 0.0 ? Check { 0.0, 0.0 } : Check { band (similarity, low, high, 0.3), 1.0 };
    }

    std::optional<Chord> chordOf (const Request& r, const Result& result, int bar)
    {
        if (auto chord = r.getTrackChord (bar))
            return chord;

        return bar < (int) result.newChords.size() ? result.newChords[(size_t) bar] : std::nullopt;
    }

    double scorePitched (const Request& r, const Result& result, const std::vector<MidiNote>& notes, int bars)
    {
        int strong = 0, fitting = 0, leaps = 0, lowest = 127, highest = 0;
        std::set<int> pitches, onsets, lengths;
        double sounding = 0.0;

        for (size_t i = 0; i < notes.size(); ++i)
        {
            const auto& n = notes[i];

            // Chord tones on the beat (key tones where there is no chord); anything goes in between.
            if (stepOf (n.start) % 4 == 0)
            {
                const auto chord = chordOf (r, result, (int) (n.start / 4.0));
                ++strong;
                fitting += (chord.has_value() ? chord->getPitchClasses().contains (n.pitch % 12) : r.key.contains (n.pitch)) ? 1 : 0;
            }

            pitches.insert (n.pitch);
            onsets.insert (stepOf (n.start) % 4);
            lengths.insert (juce::jmax (1, stepOf (n.length)));
            lowest = juce::jmin (lowest, n.pitch);
            highest = juce::jmax (highest, n.pitch);
            sounding += n.length;

            if (i > 0 && std::abs (n.pitch - notes[i - 1].pitch) > 7)
                ++leaps;
        }

        const double fit = strong > 0 ? (double) fitting / strong : 0.5;
        const double perBar = (double) notes.size() / bars;
        const double distinct = (double) pitches.size();
        const double span = highest - lowest;
        const double similarity = barSimilarity (notes, bars);

        if (r.part == Part::bassline)
            return combine ({ { fit, 3.0 }, { band (distinct, 2, 7, 2), 1.5 }, { band (span, 0, 15, 7), 1.0 },
                              { band (perBar, 1, 16, 3), 1.5 }, repetition (similarity, 0.3, 1.0) });

        if (r.part == Part::arpeggio)
            return combine ({ { fit, 3.0 }, { band (distinct, 3, 10, 3), 1.5 }, { band (span, 5, 24, 7), 1.0 },
                              { band (perBar, 4, 16, 4), 1.5 }, repetition (similarity, 0.3, 1.0) });

        // Melody: a hook needs a few pitches, varied rhythm, room to breathe and mostly small steps.
        const double leapShare = notes.size() > 1 ? leaps / (notes.size() - 1.0) : 0.0;
        const double filled = sounding / (bars * 4.0);

        return combine ({ { fit, 3.0 }, { band (distinct, 4, 10, 3), 2.0 }, { band (span, 5, 19, 7), 1.0 },
                          { band (perBar, 2, 10, 3), 2.0 }, { band ((double) onsets.size(), 2, 4, 1), 1.5 },
                          { band ((double) lengths.size(), 2, 6, 1), 1.0 }, { band (filled, 0.4, 0.95, 0.3), 1.0 },
                          { band (leapShare, 0.0, 0.25, 0.3), 1.0 }, repetition (similarity, 0.2, 0.85) });
    }

    double scoreHits (const Request& r, const std::vector<MidiNote>& notes, int bars)
    {
        std::map<int, std::vector<const MidiNote*>> byInstrument;

        for (auto& n : notes)
            byInstrument[n.pitch].push_back (&n);

        // An instrument on (nearly) every 16th at one velocity sounds like a machine stuck on repeat.
        int walls = 0;
        const int steps = bars * 16;

        for (auto& [pitch, hits] : byInstrument)
        {
            std::set<int> hitSteps;
            float lowest = 1.0f, highest = 0.0f;

            for (auto* hit : hits)
            {
                hitSteps.insert (stepOf (hit->start));
                lowest = juce::jmin (lowest, hit->velocity);
                highest = juce::jmax (highest, hit->velocity);
            }

            if ((double) hitSteps.size() >= steps * 0.9 && (highest - lowest) * 127.0 < 10.0)
                ++walls;
        }

        const double perBar = (double) notes.size() / bars;
        const double instruments = (double) byInstrument.size();
        const double spread = velocitySpread (notes);
        const double noWalls = walls == 0 ? 1.0 : 0.2;
        const double similarity = barSimilarity (notes, bars);

        if (r.part == Part::percussion)
            return combine ({ { band (instruments, 2, 4, 1), 2.0 }, { noWalls, 2.5 }, { band (perBar, 6, 32, 6), 1.0 },
                              { spread, 1.5 }, repetition (similarity, 0.4, 0.95) });

        const auto has = [&byInstrument] (std::initializer_list<int> pitches)
        {
            for (int p : pitches)
                if (byInstrument.count (p) > 0)
                    return 1.0;

            return 0.0;
        };

        return combine ({ { has ({ 35, 36 }), 3.0 }, { has ({ 37, 38, 39, 40 }), 2.0 }, { has ({ 42, 44, 46, 51, 54, 70, 82 }), 1.0 },
                          { noWalls, 1.0 }, { band (perBar, 8, 40, 8), 1.5 }, { band (instruments, 3, 8, 2), 1.0 },
                          { spread, 1.5 }, repetition (similarity, 0.4, 0.95) });
    }

    /** A playing style varied by the model: close enough to the style's own pattern to keep its feel,
        different enough to be a variation, with bars that answer each other. */
    double scoreStyle (const Request& r, const Result& result, const std::vector<MidiNote>& notes, int bars)
    {
        const auto* base = styles::findStyle (r.playingStyle);

        if (base == nullptr)
            return 0.0;

        // The style's own pattern over the same chords, for comparison.
        std::vector<styles::ChordSpan> spans;

        for (int bar = 0; bar < bars; ++bar)
            if (const auto chord = chordOf (r, result, bar))
                spans.push_back ({ bar * 4.0, bar * 4.0 + 4.0, *chord });

        const auto example = styles::play (*base, spans, bars * 4.0, r.bpm);

        if (example.empty())
            return 0.5;

        std::set<std::pair<int, int>> ours, theirs;

        for (auto& n : notes)
            ours.insert ({ stepOf (n.start), n.pitch });

        for (auto& n : example)
            theirs.insert ({ stepOf (n.start), n.pitch });

        size_t shared = 0;
        for (auto& item : ours)
            shared += theirs.count (item);

        const double likeness = (double) shared / (double) (ours.size() + theirs.size() - shared);
        const double density = (double) notes.size() / (double) example.size();

        return combine ({ { band (density, 0.6, 1.6, 0.6), 2.0 }, { band (likeness, 0.3, 0.85, 0.3), 2.0 },
                          { velocitySpread (notes), 0.5 }, repetition (barSimilarity (notes, bars), 0.3, 0.9) });
    }

    double scoreChords (const Request& r, const std::vector<MidiNote>& notes, int bars)
    {
        // Notes struck together are one chord.
        std::map<int, std::set<int>> strikes;
        std::set<int> lengths;

        for (auto& n : notes)
        {
            strikes[stepOf (n.start)].insert (n.pitch % 12);
            lengths.insert (juce::jmax (1, stepOf (n.length)));
        }

        const double perBar = (double) strikes.size() / bars;

        // Over a chord track the chords are given: only the rhythm is the model's.
        if (r.isChordRhythm())
            return combine ({ { band (perBar, 1, 8, 3), 2.0 }, { velocitySpread (notes), 1.0 }, { band ((double) lengths.size(), 1, 4, 2), 0.5 } });

        std::set<std::set<int>> distinct;

        for (auto& [step, pitchClasses] : strikes)
            distinct.insert (pitchClasses);

        return combine ({ { band ((double) distinct.size(), juce::jmin (bars, 3), 8, 2), 2.0 }, { band (perBar, 0.5, 2, 1), 1.0 } });
    }
}

double scoreTake (const Request& r, const Result& result)
{
    // A style played as it is: the takes differ only in the chords the model wrote for empty bars.
    if (r.usesPlayingStyle() && ! r.varyStyle)
    {
        std::set<juce::String> chords;
        for (auto& chord : result.newChords)
            if (chord.has_value())
                chords.insert (chord->getName());

        return result.notes.empty() ? 0.0 : band ((double) chords.size(), juce::jmin (r.bars, 3), 8, 2);
    }

    const int bars = juce::jmax (1, r.usesPlayingStyle() ? r.getStyleBars() : r.getPatternBars());
    std::vector<MidiNote> pattern;

    for (auto& n : result.notes)
        if (n.start < bars * 4.0 - 1.0e-6)
            pattern.push_back (n);

    if (pattern.empty())
        return 0.0;

    if (r.usesPlayingStyle())
        return scoreStyle (r, result, pattern, bars);

    switch (r.part)
    {
        case Part::chords:     return scoreChords (r, pattern, bars);
        case Part::drums:
        case Part::percussion: return scoreHits (r, pattern, bars);
        case Part::melody:
        case Part::bassline:
        case Part::arpeggio:   break;
    }

    return scorePitched (r, result, pattern, bars);
}
} // namespace daw::gen
