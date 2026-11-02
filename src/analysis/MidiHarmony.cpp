#include "MidiHarmony.h"
#include "ChordFinder.h"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace daw::analysis::midiharmony
{
namespace
{
    // All the tuning of the finder is here, so a made-up part that goes wrong is fixed in one place.
    constexpr int minNotes = 8;                    // fewer notes than this: no key
    constexpr int minNames = 3;                    // fewer note names than this: no key
    constexpr double closeKeyGap = 0.1;            // top two keys this close: the hints decide
    constexpr double hintWeight = 0.1;             // each hint (ends on, starts on, most time on home)
    constexpr double scaleTie = 0.03;              // scales within this much of the best fit: the simpler wins
    constexpr double pentatonicOtherShare = 0.02;  // a pentatonic or blues part has no other note over this share

    constexpr double missingNotePenalty = 0.08;    // a chord note that doesn't sound
    constexpr double missingFifthPenalty = 0.01;   // ... the fifth is often left out
    constexpr double extraNotePenalty = 0.05;      // per chord note over three
    constexpr double chordTieGap = 0.005;          // chords this close in score are a tie
    constexpr double otherReadingGap = 0.05;       // a runner-up chord this close is reported
    constexpr double splitMinShare = 0.25;         // each half of a split bar holds this much of the bar's weight
    constexpr double splitGain = 0.1;              // and the halves' mean score beats the whole bar's by this
    constexpr double arpeggioShare = 0.85;         // one note at a time: the chord holds this much of the weight
    constexpr int arpeggioNames = 3;               // ... with this many of its note names sounding
    constexpr int arpeggioQualities = 9;           // ... and is one of the first nine qualities (no sus, add or ninths)
    constexpr double passingLength = 0.5;          // a passing note lasts at most this many beats
    constexpr double epsilon = 1.0e-9;

    int mod12 (int v) { return ((v % 12) + 12) % 12; }

    const Scale simplestFirst[] { Scale::major, Scale::minor, Scale::mixolydian, Scale::dorian, Scale::lydian,
                                  Scale::phrygian, Scale::harmonicMinor, Scale::melodicMinor, Scale::locrian };

    juce::String withArticle (const juce::String& noteName)
    {
        return (juce::String ("AEFHILMNORSX").containsChar (noteName[0]) ? "an " : "a ") + noteName;
    }

    /** The notes cut to the clip's stretch of the song. */
    std::vector<Note> clipNotes (const Input& in)
    {
        std::vector<Note> notes;

        for (auto n : in.notes)
        {
            const double s = juce::jmax (n.start, in.startBeat), e = juce::jmin (n.getEnd(), in.endBeat);

            if (e - s > epsilon)
            {
                n.start = s;
                n.length = e - s;
                notes.push_back (n);
            }
        }

        return notes;
    }

    bool isSingleLine (const std::vector<Note>& notes, double a, double b)
    {
        std::vector<std::pair<double, double>> spans;

        for (auto& n : notes)
        {
            const double s = juce::jmax (n.start, a), e = juce::jmin (n.getEnd(), b);

            if (e - s > epsilon)
                spans.push_back ({ s, e });
        }

        std::sort (spans.begin(), spans.end());
        double latestEnd = -1.0e9;

        for (auto& [s, e] : spans)
        {
            if (s < latestEnd - 1.0e-6)
                return false;

            latestEnd = juce::jmax (latestEnd, e);
        }

        return true;
    }

    Segment makeSegment (const std::vector<Note>& notes, int bar, double a, double b)
    {
        Segment seg;
        seg.bar = bar;
        seg.startBeat = a;
        seg.endBeat = b;

        std::vector<double> times { a, b };

        for (auto& n : notes)
        {
            const double w = getWeight (n, a, b);

            if (w > 0.0 || (juce::jmin (n.getEnd(), b) - juce::jmax (n.start, a)) > epsilon)
            {
                seg.weights[(size_t) mod12 (n.pitch)] += w;
                seg.total += w;
                times.push_back (juce::jlimit (a, b, n.start));
                times.push_back (juce::jlimit (a, b, n.getEnd()));
            }
        }

        seg.singleLine = isSingleLine (notes, a, b);

        // The bass: the note name that is the lowest note for the longest time.
        std::sort (times.begin(), times.end());
        std::array<double, 12> lowestFor {};
        std::array<int, 12> lowestPitch;
        lowestPitch.fill (1000);

        for (size_t i = 0; i + 1 < times.size(); ++i)
        {
            const double t0 = times[i], t1 = times[i + 1];

            if (t1 - t0 <= epsilon)
                continue;

            const double mid = 0.5 * (t0 + t1);
            int lowest = 1000;

            for (auto& n : notes)
                if (n.start <= mid && mid < n.getEnd())
                    lowest = juce::jmin (lowest, n.pitch);

            if (lowest < 1000)
            {
                lowestFor[(size_t) mod12 (lowest)] += t1 - t0;
                lowestPitch[(size_t) mod12 (lowest)] = juce::jmin (lowestPitch[(size_t) mod12 (lowest)], lowest);
            }
        }

        double longest = 0.0;

        for (int pc = 0; pc < 12; ++pc)
        {
            // A tie (an arpeggio) goes to the lower note.
            const bool tie = std::abs (lowestFor[(size_t) pc] - longest) <= epsilon && seg.bass >= 0 && lowestPitch[(size_t) pc] < lowestPitch[(size_t) seg.bass];

            if (lowestFor[(size_t) pc] > longest + epsilon || tie)
            {
                longest = lowestFor[(size_t) pc];
                seg.bass = pc;
            }
        }

        // One note at a time (an arpeggio): the lowest note is the bass, not the note that comes round most often.
        if (seg.singleLine)
        {
            int lowest = 1000;

            for (auto& n : notes)
                if (juce::jmin (n.getEnd(), b) - juce::jmax (n.start, a) > epsilon)
                    lowest = juce::jmin (lowest, n.pitch);

            seg.bass = lowest < 1000 ? mod12 (lowest) : -1;
        }

        return seg;
    }

    //==============================================================================
    const std::vector<Chord>& getAllChords()
    {
        static const std::vector<Chord> chords = []
        {
            std::vector<Chord> all;

            for (auto& q : Chord::getQualities())
                for (int root = 0; root < 12; ++root)
                    all.push_back ({ root, q.intervals });

            return all;
        }();

        return chords;
    }

    int getQualityIndex (const Chord& c)
    {
        const auto& qualities = Chord::getQualities();

        for (size_t i = 0; i < qualities.size(); ++i)
            if (qualities[i].intervals == c.intervals)
                return (int) i;

        return -1;
    }

    bool isMinorTriad (const Chord& c) { return c.intervals.contains (3); }

    bool isTonicChord (const Chord& c, int tonic, bool minorKey)
    {
        return c.root == tonic && isMinorTriad (c) == minorKey;
    }

    struct Pick
    {
        Chord chord;
        int bass = -1;
        double score = 0.0;
        bool noThird = false;
        std::optional<Chord> other;
        int otherBass = -1;
        double otherGap = 0.0;
    };

    struct Scored
    {
        Chord chord;
        double score = 0.0, share = 0.0;
        int sounding = 0, numNotes = 0;
        bool explainsAll = false, rootInBass = false, inKey = false, samePrevious = false;
        juce::Array<int> pcs;
    };

    std::optional<Pick> pickChord (const Segment& seg, const std::optional<MusicalKey>& key, const std::optional<Chord>& previous)
    {
        if (seg.total <= epsilon)
            return std::nullopt;

        std::vector<int> sounding;

        for (int pc = 0; pc < 12; ++pc)
            if (seg.weights[(size_t) pc] > epsilon)
                sounding.push_back (pc);

        if (sounding.size() < 2)
            return std::nullopt;

        const auto bassOf = [&seg] (const Chord& c) { return seg.bass >= 0 && seg.bass != c.root ? seg.bass : -1; };

        // Only a root and a fifth: the key's major or minor chord on that root.
        if (sounding.size() == 2)
        {
            for (int i = 0; i < 2; ++i)
            {
                const int root = sounding[(size_t) i], fifth = sounding[(size_t) (1 - i)];

                if (mod12 (fifth - root) == 7)
                {
                    if (seg.singleLine)
                        return std::nullopt;

                    bool minor = false;

                    if (key)
                    {
                        const auto intervals = key->getIntervals();
                        minor = intervals.contains (mod12 (root + 3 - key->tonic)) && ! intervals.contains (mod12 (root + 4 - key->tonic));
                    }

                    Pick pick;
                    pick.chord = { root, minor ? juce::Array<int> { 0, 3, 7 } : juce::Array<int> { 0, 4, 7 } };
                    pick.bass = bassOf (pick.chord);
                    pick.score = 1.0 - missingNotePenalty;
                    pick.noThird = true;
                    return pick;
                }
            }
        }

        juce::Array<int> keyPcs;

        if (key)
            for (int i : key->getIntervals())
                keyPcs.add (mod12 (key->tonic + i));

        std::vector<Scored> all;

        for (auto& chord : getAllChords())
        {
            Scored s;
            s.chord = chord;
            s.pcs = chord.getPitchClasses();
            double inWeight = 0.0, penalty = 0.0;
            bool inKeyAll = key.has_value();

            for (int pc : s.pcs)
            {
                const double w = seg.weights[(size_t) pc];
                inWeight += w;

                if (w > epsilon)
                    ++s.sounding;
                else
                    penalty += mod12 (pc - chord.root) == 7 ? missingFifthPenalty : missingNotePenalty;

                if (key && ! keyPcs.contains (pc))
                    inKeyAll = false;
            }

            if (s.sounding < 2)
                continue;

            s.numNotes = s.pcs.size();
            s.share = inWeight / seg.total;
            s.score = s.share - penalty - extraNotePenalty * (double) juce::jmax (0, s.numNotes - 3);
            s.explainsAll = s.share >= 1.0 - 1.0e-6;
            s.rootInBass = seg.bass == chord.root;
            s.inKey = inKeyAll;
            s.samePrevious = previous && *previous == chord;
            all.push_back (std::move (s));
        }

        if (all.empty())
            return std::nullopt;

        double top = -1.0e9;

        for (auto& s : all)
            top = juce::jmax (top, s.score);

        const Scored* best = nullptr;

        const auto rank = [] (const Scored& s) { return std::make_tuple (s.explainsAll, s.rootInBass, -s.numNotes, s.inKey, s.samePrevious); };

        for (auto& s : all)
            if (s.score >= top - chordTieGap && (best == nullptr || rank (s) > rank (*best)))
                best = &s;

        // One note at a time: only an arpeggio of a plain chord names a chord.
        if (seg.singleLine
            && (getQualityIndex (best->chord) < 0 || getQualityIndex (best->chord) >= arpeggioQualities
                || best->sounding < arpeggioNames || best->share < arpeggioShare))
            return std::nullopt;

        Pick pick;
        pick.chord = best->chord;
        pick.bass = bassOf (best->chord);
        pick.score = best->score;

        // The same notes read another way: Am7 over A is also C over A.
        if (best->numNotes == 4 && seg.bass == best->chord.root)
        {
            for (int p : best->pcs)
            {
                if (p == best->chord.root || pick.other)
                    continue;

                for (auto& q : { juce::Array<int> { 0, 4, 7 }, juce::Array<int> { 0, 3, 7 } })
                {
                    const Chord triad { p, q };
                    const auto triadPcs = triad.getPitchClasses();

                    if (! pick.other && std::all_of (triadPcs.begin(), triadPcs.end(), [best] (int pc) { return best->pcs.contains (pc); }))
                    {
                        pick.other = triad;
                        pick.otherBass = best->chord.root;
                    }
                }
            }
        }

        if (! pick.other)
        {
            const Scored* second = nullptr;

            for (auto& s : all)
            {
                auto a = s.pcs, b = best->pcs;
                a.sort();
                b.sort();

                if (s.score >= best->score - otherReadingGap && ! (a == b) && (second == nullptr || s.score > second->score))
                    second = &s;
            }

            if (second != nullptr)
            {
                pick.other = second->chord;
                pick.otherBass = bassOf (second->chord);
                pick.otherGap = best->score - second->score;
            }
        }

        return pick;
    }

    void fill (Segment& seg, const Pick& pick)
    {
        seg.chord = pick.chord;
        seg.chordBass = pick.bass;
        seg.score = pick.score;
        seg.noThird = pick.noThird;
        seg.otherChord = pick.other;
        seg.otherBass = pick.otherBass;
        seg.otherGap = pick.otherGap;
        const auto pcs = pick.chord.getPitchClasses();

        for (int pc = 0; pc < 12; ++pc)
            if (seg.weights[(size_t) pc] > epsilon && ! pcs.contains (pc))
                seg.between.push_back (pc);
    }

    //==============================================================================
    bool contains (const juce::Array<int>& intervals, int tonic, int pc) { return intervals.contains (mod12 (pc - tonic)); }

    double getFit (const std::array<double, 12>& weights, double total, int tonic, const juce::Array<int>& intervals)
    {
        double in = 0.0;

        for (int i : intervals)
            in += weights[(size_t) mod12 (tonic + i)];

        return total > 0.0 ? in / total : 0.0;
    }

    /** Pentatonic and blues parts: every note of the scale sounds and nothing else counts for much. */
    bool isFivePart (const std::array<double, 12>& weights, double total, int tonic, const juce::Array<int>& intervals)
    {
        for (int pc = 0; pc < 12; ++pc)
        {
            const double w = weights[(size_t) pc];

            if (contains (intervals, tonic, pc) ? w <= epsilon : w > pentatonicOtherShare * total)
                return false;
        }

        return true;
    }

    std::pair<Scale, double> chooseScale (const std::array<double, 12>& weights, double total, int tonic, bool minorKey)
    {
        const Scale fives[] { Scale::blues, minorKey ? Scale::minorPentatonic : Scale::majorPentatonic };

        for (auto s : fives)
        {
            if (s == Scale::blues && ! minorKey)
                continue;

            const auto intervals = getScaleIntervals (s);

            if (isFivePart (weights, total, tonic, intervals))
                return { s, getFit (weights, total, tonic, intervals) };
        }

        double bestFit = 0.0;

        for (auto s : simplestFirst)
            bestFit = juce::jmax (bestFit, getFit (weights, total, tonic, getScaleIntervals (s)));

        for (auto s : simplestFirst)
        {
            const double fit = getFit (weights, total, tonic, getScaleIntervals (s));

            if (fit >= bestFit - scaleTie)
                return { s, fit };
        }

        return { Scale::major, bestFit };
    }

    bool isPassing (const std::vector<Note>& sorted, int pc)
    {
        bool any = false;

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            if (mod12 (sorted[i].pitch) != pc)
                continue;

            any = true;

            if (sorted[i].length > passingLength + epsilon || i == 0 || i + 1 == sorted.size()
                || std::abs (sorted[i].pitch - sorted[i - 1].pitch) > 2 || std::abs (sorted[i].pitch - sorted[i + 1].pitch) > 2)
                return false;
        }

        return any;
    }

    std::vector<OutsideNote> findOutside (const std::vector<Note>& notes, const Result& r)
    {
        const auto intervals = getScaleIntervals (r.key.scale);
        const int tonic = r.key.tonic;
        // The branch follows the found scale's third: a minor third means minor (harmonic, Dorian, Phrygian and the blues too).
        const bool minorKey = intervals.contains (3);

        std::vector<Note> sorted = notes;
        std::sort (sorted.begin(), sorted.end(), [] (const Note& a, const Note& b) { return std::tie (a.start, a.pitch) < std::tie (b.start, b.pitch); });

        const auto isOutside = [&] (int d) { return r.profile[(size_t) mod12 (tonic + d)] > epsilon && ! intervals.contains (d); };
        std::vector<OutsideNote> found;

        for (int pc = 0; pc < 12; ++pc)
        {
            if (r.profile[(size_t) pc] <= epsilon || contains (intervals, tonic, pc))
                continue;

            OutsideNote o;
            o.pitchClass = pc;
            o.share = r.profile[(size_t) pc] / r.totalWeight;
            o.firstBar = 1000000;

            for (auto& n : notes)
                if (mod12 (n.pitch) == pc)
                    o.firstBar = juce::jmin (o.firstBar, (int) std::floor (n.start / 4.0 + epsilon));

            const int d = mod12 (pc - tonic);
            const auto parallel = MusicalKey::getTonicName (tonic) + " minor";

            if (minorKey && d == 11)
                o.reason = isOutside (9) ? "from melodic minor: the raised sixth and seventh" : "from harmonic minor: it makes the five chord major";
            else if (minorKey && d == 9)
                o.reason = isOutside (11) ? "from melodic minor: the raised sixth and seventh" : "from melodic minor or Dorian: a raised sixth";
            else if (minorKey && d == 6)
                o.reason = "the blue note, a flattened fifth";
            else if (minorKey && d == 10)
                o.reason = "from natural minor: the flat seventh";
            else if (minorKey && d == 8)
                o.reason = "from natural minor: the flat sixth";
            else if (! minorKey && d == 10)
                o.reason = "from Mixolydian, or borrowed from " + parallel + ": the flat seventh";
            else if (! minorKey && (d == 3 || d == 8))
                o.reason = "borrowed from " + parallel;
            else if (! minorKey && d == 6)
                o.reason = "from Lydian: the raised fourth";
            else if (isPassing (sorted, pc))
                o.reason = "a passing note";
            else
                o.reason = "outside the key";

            found.push_back (o);
        }

        std::sort (found.begin(), found.end(), [] (const OutsideNote& a, const OutsideNote& b) { return a.share > b.share; });
        return found;
    }
} // namespace

//==============================================================================
Input makeInput (const MidiClip& clip)
{
    Input in;
    in.startBeat = clip.startBeat;
    in.endBeat = clip.startBeat + clip.lengthBeats;

    for (auto& n : clip.notes)
        in.notes.push_back ({ n.pitch, clip.startBeat + n.start, n.length, n.velocity });

    return in;
}

juce::Array<int> getScaleIntervals (Scale s)
{
    switch (s)
    {
        case Scale::major:           return MusicalKey::getIntervals (MusicalKey::Scale::major);
        case Scale::minor:           return MusicalKey::getIntervals (MusicalKey::Scale::minor);
        case Scale::dorian:          return MusicalKey::getIntervals (MusicalKey::Scale::dorian);
        case Scale::phrygian:        return MusicalKey::getIntervals (MusicalKey::Scale::phrygian);
        case Scale::lydian:          return MusicalKey::getIntervals (MusicalKey::Scale::lydian);
        case Scale::mixolydian:      return MusicalKey::getIntervals (MusicalKey::Scale::mixolydian);
        case Scale::locrian:         return MusicalKey::getIntervals (MusicalKey::Scale::locrian);
        case Scale::harmonicMinor:   return MusicalKey::getIntervals (MusicalKey::Scale::harmonicMinor);
        case Scale::melodicMinor:    return MusicalKey::getIntervals (MusicalKey::Scale::melodicMinor);
        case Scale::majorPentatonic: return { 0, 2, 4, 7, 9 };
        case Scale::minorPentatonic: return { 0, 3, 5, 7, 10 };
        case Scale::blues:           return { 0, 3, 5, 6, 7, 10 };
    }

    return {};
}

juce::String getScaleName (Scale s)
{
    switch (s)
    {
        case Scale::major:           return MusicalKey::getScaleName (MusicalKey::Scale::major);
        case Scale::minor:           return MusicalKey::getScaleName (MusicalKey::Scale::minor);
        case Scale::dorian:          return MusicalKey::getScaleName (MusicalKey::Scale::dorian);
        case Scale::phrygian:        return MusicalKey::getScaleName (MusicalKey::Scale::phrygian);
        case Scale::lydian:          return MusicalKey::getScaleName (MusicalKey::Scale::lydian);
        case Scale::mixolydian:      return MusicalKey::getScaleName (MusicalKey::Scale::mixolydian);
        case Scale::locrian:         return MusicalKey::getScaleName (MusicalKey::Scale::locrian);
        case Scale::harmonicMinor:   return MusicalKey::getScaleName (MusicalKey::Scale::harmonicMinor);
        case Scale::melodicMinor:    return MusicalKey::getScaleName (MusicalKey::Scale::melodicMinor);
        case Scale::majorPentatonic: return "major pentatonic";
        case Scale::minorPentatonic: return "minor pentatonic";
        case Scale::blues:           return "blues";
    }

    return {};
}

juce::String getNoteName (int pitchClass)
{
    return MusicalKey::getTonicName (pitchClass);
}

juce::String getSpelledName (int pitchClass, int tonic)
{
    static const char* letters = "CDEFGAB";
    static const int naturals[] = { 0, 2, 4, 5, 7, 9, 11 };
    static const int steps[] = { 0, 1, 1, 2, 2, 3, 4, 4, 4, 6, 6, 6 };
    const int tonicLetter = juce::String (letters).indexOfChar (getNoteName (tonic)[0]);
    const int d = mod12 (pitchClass - tonic);
    const int letter = (tonicLetter + steps[d]) % 7;
    const int accidental = (mod12 (pitchClass - naturals[letter]) + 6) % 12 - 6;
    return juce::String::charToString (letters[letter]) + juce::String::repeatedString (accidental > 0 ? "#" : "b", std::abs (accidental));
}

juce::String getKeyName (int tonic, Scale s)
{
    return getNoteName (tonic) + " " + getScaleName (s);
}

MusicalKey getSongKey (int tonic, Scale s)
{
    switch (s)
    {
        case Scale::major:           return { tonic, MusicalKey::Scale::major };
        case Scale::minor:           return { tonic, MusicalKey::Scale::minor };
        case Scale::dorian:          return { tonic, MusicalKey::Scale::dorian };
        case Scale::phrygian:        return { tonic, MusicalKey::Scale::phrygian };
        case Scale::lydian:          return { tonic, MusicalKey::Scale::lydian };
        case Scale::mixolydian:      return { tonic, MusicalKey::Scale::mixolydian };
        case Scale::locrian:         return { tonic, MusicalKey::Scale::locrian };
        case Scale::harmonicMinor:   return { tonic, MusicalKey::Scale::harmonicMinor };
        case Scale::melodicMinor:    return { tonic, MusicalKey::Scale::melodicMinor };
        case Scale::majorPentatonic: return { tonic, MusicalKey::Scale::major };
        case Scale::minorPentatonic: return { tonic, MusicalKey::Scale::minor };
        case Scale::blues:           return { tonic, MusicalKey::Scale::minor };
    }

    return {};
}

juce::String getChordName (const Chord& chord, int bass)
{
    return chord.getName() + (bass >= 0 && bass != chord.root ? "/" + getNoteName (bass) : juce::String());
}

double getWeight (const Note& n, double from, double to)
{
    const double overlap = juce::jmin (n.getEnd(), to) - juce::jmax (n.start, from);
    return overlap > 0.0 ? overlap * (double) n.velocity : 0.0;
}

//==============================================================================
std::vector<Segment> findChords (const Input& in, const std::optional<MusicalKey>& key)
{
    const auto notes = clipNotes (in);
    std::vector<Segment> segments;

    if (notes.empty() || in.endBeat <= in.startBeat)
        return segments;

    std::optional<Chord> previous;
    const int firstBar = (int) std::floor (in.startBeat / 4.0 + epsilon);
    const int lastBar = (int) std::ceil (in.endBeat / 4.0 - epsilon) - 1;

    for (int bar = firstBar; bar <= lastBar; ++bar)
    {
        const double a = juce::jmax ((double) bar * 4.0, in.startBeat), b = juce::jmin ((double) bar * 4.0 + 4.0, in.endBeat);
        auto whole = makeSegment (notes, bar, a, b);

        if (whole.total <= epsilon)
        {
            // Silence: no segment would show anything, but the bar still exists for the chord track.
            segments.push_back (whole);
            continue;
        }

        const auto wholePick = pickChord (whole, key, previous);

        if (b - a >= 4.0 - epsilon)
        {
            auto h1 = makeSegment (notes, bar, a, a + 2.0), h2 = makeSegment (notes, bar, a + 2.0, b);
            const auto p1 = pickChord (h1, key, previous);
            const auto p2 = p1 ? pickChord (h2, key, p1->chord) : std::nullopt;

            if (p1 && p2 && ! (p1->chord == p2->chord)
                && h1.total >= splitMinShare * whole.total && h2.total >= splitMinShare * whole.total
                && 0.5 * (p1->score + p2->score) - (wholePick ? wholePick->score : 0.0) >= splitGain - 1.0e-6)
            {
                h1.split = h2.split = true;
                h2.secondHalf = true;
                fill (h1, *p1);
                fill (h2, *p2);
                segments.push_back (h1);
                segments.push_back (h2);
                previous = p2->chord;
                continue;
            }
        }

        if (wholePick)
        {
            fill (whole, *wholePick);
            previous = wholePick->chord;
        }

        segments.push_back (whole);
    }

    return segments;
}

Result analyse (const Input& in)
{
    Result r;
    const auto notes = clipNotes (in);

    for (auto& n : notes)
    {
        const double w = getWeight (n, in.startBeat, in.endBeat);
        r.profile[(size_t) mod12 (n.pitch)] += w;
        r.totalWeight += w;
    }

    r.numNotes = (int) notes.size();

    for (double w : r.profile)
        if (w > epsilon)
            ++r.numNames;

    r.singleLine = isSingleLine (notes, in.startBeat, in.endBeat);
    r.tooFew = r.numNotes < minNotes || r.numNames < minNames || r.totalWeight <= epsilon;
    r.segments = findChords (in, std::nullopt);

    if (! r.tooFew)
    {
        // Rank the 24 keys by the Krumhansl-Kessler correlation.
        std::vector<KeyScore> keys;

        for (int tonic = 0; tonic < 12; ++tonic)
            for (bool minor : { false, true })
                keys.push_back ({ tonic, minor, chordfinder::correlateKey (r.profile, minor, tonic) });

        std::stable_sort (keys.begin(), keys.end(), [] (const KeyScore& a, const KeyScore& b) { return a.score > b.score; });
        r.close = keys[0].score - keys[1].score < closeKeyGap;

        // The hints, from a first pass over the chords: where the part ends and starts, and how long it stays on home.
        int lastBass = -1, firstBass = -1;
        std::optional<Chord> firstChord, lastChord;
        double chordTime = 0.0;

        for (auto& s : r.segments)
        {
            if (s.bass >= 0)
            {
                lastBass = s.bass;

                if (firstBass < 0)
                    firstBass = s.bass;
            }

            if (s.chord)
            {
                lastChord = s.chord;

                if (! firstChord)
                    firstChord = s.chord;

                chordTime += s.endBeat - s.startBeat;
            }
        }

        const auto homeShare = [&] (const KeyScore& k)
        {
            double t = 0.0;

            for (auto& s : r.segments)
                if (s.chord && isTonicChord (*s.chord, k.tonic, k.minor))
                    t += s.endBeat - s.startBeat;

            return chordTime > 0.0 ? t / chordTime : 0.0;
        };

        const auto hinted = [&] (const KeyScore& k)
        {
            return k.score + hintWeight * ((lastBass == k.tonic ? 1.0 : 0.0) + (firstChord && isTonicChord (*firstChord, k.tonic, k.minor) ? 1.0 : 0.0)
                                           + homeShare (k));
        };

        if (r.close)
        {
            const double top = keys[0].score;
            auto closeEnd = std::find_if (keys.begin(), keys.end(), [top] (const KeyScore& k) { return top - k.score >= closeKeyGap; });
            std::stable_sort (keys.begin(), closeEnd, [&] (const KeyScore& a, const KeyScore& b) { return hinted (a) > hinted (b); });
        }

        // The relative major or minor of the top key that holds the part's notes just as well: where the part starts and
        // ends decides, even when the correlation put the other key far ahead (an Am F C G loop is A minor, not C major).
        {
            const auto& top = keys[0];
            const int relTonic = top.minor ? mod12 (top.tonic + 3) : mod12 (top.tonic + 9);
            const auto fitOf = [&] (const KeyScore& k) { return getFit (r.profile, r.totalWeight, k.tonic, getScaleIntervals (k.minor ? Scale::minor : Scale::major)); };
            const auto homeScore = [&] (const KeyScore& k)
            {
                // Where a part starts says more than where it stops (a loop is often cut, a song may end on its relative key).
                return 2.0 * (firstChord && isTonicChord (*firstChord, k.tonic, k.minor) ? 1.0 : 0.0) + (firstBass == k.tonic ? 1.0 : 0.0)
                       + (lastBass == k.tonic ? 1.0 : 0.0) + (lastChord && isTonicChord (*lastChord, k.tonic, k.minor) ? 1.0 : 0.0) + homeShare (k);
            };

            const auto rel = std::find_if (keys.begin(), keys.end(), [&] (const KeyScore& k) { return k.tonic == relTonic && k.minor != top.minor; });

            if (rel != keys.end() && std::abs (fitOf (*rel) - fitOf (top)) < 1.0e-6 && homeScore (*rel) > homeScore (top) + 1.0e-9)
            {
                std::rotate (keys.begin(), rel, rel + 1);
                r.close = true;
            }
        }

        r.ranking.assign (keys.begin(), keys.begin() + 3);
        r.runnerUp = keys[1];
        const auto& win = keys[0];
        const auto& other = keys[1];

        r.relative = (win.minor && ! other.minor && other.tonic == mod12 (win.tonic + 3))
                     || (! win.minor && other.minor && other.tonic == mod12 (win.tonic + 9));

        if (r.close)
        {
            if (lastBass == win.tonic && lastBass != other.tonic)
                r.closeReason = "the part ends on " + getNoteName (lastBass);
            else if (firstChord && isTonicChord (*firstChord, win.tonic, win.minor) && ! isTonicChord (*firstChord, other.tonic, other.minor))
                r.closeReason = "the part starts on " + withArticle (firstChord->getName()) + " chord";
            else if (homeShare (win) > homeShare (other))
                r.closeReason = "the part spends most time on " + getNoteName (win.tonic) + (win.minor ? "m" : "");
            else
                r.closeReason = "its notes fit slightly better";
        }

        const auto [scale, fit] = chooseScale (r.profile, r.totalWeight, win.tonic, win.minor);
        r.key.tonic = win.tonic;
        r.key.scale = scale;
        r.key.fit = fit;
        r.key.songKey = getSongKey (win.tonic, scale);
        r.key.name = getKeyName (win.tonic, scale);
        r.outside = findOutside (notes, r);

        // Again with the key known: power chords and close calls follow it.
        r.segments = findChords (in, r.key.songKey);
    }

    r.melody = std::none_of (r.segments.begin(), r.segments.end(), [] (const Segment& s) { return s.chord.has_value(); });
    return r;
}
} // namespace daw::analysis::midiharmony
