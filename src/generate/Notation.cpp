#include "Notation.h"

#include <map>
#include <set>

namespace daw::gen::notation
{
namespace
{
    const char* letters = "CDEFGAB";
    constexpr int naturals[] = { 0, 2, 4, 5, 7, 9, 11 };

    int letterOf (juce::juce_wchar c)
    {
        return juce::String (letters).indexOfChar (c);
    }

    /** A pitch class written with this letter. */
    Pitch makePitch (int midi, int letter)
    {
        const int pc = ((midi % 12) + 12) % 12;
        int alter = ((pc - naturals[letter]) % 12 + 12) % 12;

        if (alter > 6)
            alter -= 12;

        return { letter, alter, (midi - alter - naturals[letter]) / 12 - 1 };
    }

    int getTonicLetter (const MusicalKey& key)
    {
        return letterOf (sheet::getKeyName (key)[0]);
    }

    bool signatureHasSharps (const MusicalKey& key)
    {
        const auto signature = guide::getKeySignature (guide::getCirclePosition (key));
        return ! signature.isEmpty() && signature[0].containsChar ('#');
    }

    /** The alteration each letter gets from the key signature. */
    std::array<int, 7> getKeyAlterations (int position)
    {
        std::array<int, 7> alters {};
        const auto signature = guide::getKeySignature (position);

        for (auto& name : signature)
            alters[(size_t) letterOf (name[0])] = name.containsChar ('#') ? 1 : -1;

        return alters;
    }

    // ---- Rhythm -------------------------------------------------------------------------------------------

    struct QNote
    {
        int pitch = 60;
        int start = 0, end = 0;     // units
        int source = 0;
    };

    /** Rounds the notes to the display grid: each beat is read as sixteenths or as eighth-note triplets, whichever
        fits its notes better (R7). */
    std::vector<QNote> quantise (const std::vector<std::pair<MidiNote, int>>& notes)
    {
        constexpr double eps = 1e-9;
        std::map<int, std::vector<double>> points;

        auto endBeat = [] (const MidiNote& n, int startBeat)
        {
            return std::max (startBeat, (int) std::ceil (n.getEnd() - eps) - 1);
        };

        for (auto& [n, source] : notes)
        {
            const int sb = (int) std::floor (n.start + eps);
            const int eb = endBeat (n, sb);
            points[sb].push_back (std::clamp (n.start - sb, 0.0, 1.0));
            points[eb].push_back (std::clamp (n.getEnd() - eb, 0.0, 1.0));
        }

        std::map<int, bool> triplets;

        for (auto& [beat, list] : points)
            triplets[beat] = fitBeat (list).triplets;

        auto toUnits = [&triplets] (double position, int beat)
        {
            const int step = triplets[beat] ? 4 : 3;
            return beat * unitsPerBeat + (int) std::lround ((position - beat) * unitsPerBeat / step) * step;
        };

        std::vector<QNote> result;

        for (auto& [n, source] : notes)
        {
            const int sb = (int) std::floor (n.start + eps);
            QNote q;
            q.pitch = n.pitch;
            q.source = source;
            q.start = toUnits (n.start, sb);
            q.end = std::max (toUnits (n.getEnd(), endBeat (n, sb)), q.start + (triplets[sb] ? 4 : 3));
            result.push_back (q);
        }

        return result;
    }

    /** What one written piece of a note is: the largest value (with at most one dot) that fits, never crossing a
        bar line, never crossing a beat when it starts off the beat, and crossing beat 3 only from beat 1 (R9). */
    std::vector<int> splitDurations (int p, int e)
    {
        std::vector<int> durations;
        static constexpr int candidates[] = { 48, 36, 24, 18, 12, 9, 8, 6, 4, 3 };

        while (p < e)
        {
            int boundary = unitsPerBar;

            if (p % unitsPerBeat != 0)
                boundary = (p / unitsPerBeat + 1) * unitsPerBeat;
            else if (p != 0 && p < 24)
                boundary = 24;

            const int room = std::min (e, boundary) - p;
            int d = 0;

            for (int c : candidates)
            {
                if (c > room)
                    continue;

                const bool triplet = c == 8 || c == 4;
                const int r = p + c;

                // A written piece may not end on a sixteenth point in a beat whose end is a triplet point.
                const bool strands = ! triplet && r % unitsPerBeat != 0 && e % 3 != 0 && r / unitsPerBeat == e / unitsPerBeat;

                if (! strands && (triplet ? (p % 3 != 0 || r % 3 != 0) : p % 3 == 0))
                {
                    d = c;
                    break;
                }
            }

            if (d == 0)
                d = room;

            durations.push_back (d);
            p += d;
        }

        return durations;
    }

    void describeDuration (int d, Event& event)
    {
        event.length = d;
        event.dotted = false;
        event.triplet = false;

        switch (d)
        {
            case 48: event.value = Value::whole; break;
            case 36: event.value = Value::half; event.dotted = true; break;
            case 24: event.value = Value::half; break;
            case 18: event.value = Value::quarter; event.dotted = true; break;
            case 12: event.value = Value::quarter; break;
            case 9:  event.value = Value::eighth; event.dotted = true; break;
            case 8:  event.value = Value::quarter; event.triplet = true; break;
            case 6:  event.value = Value::eighth; break;
            case 4:  event.value = Value::eighth; event.triplet = true; break;
            default: event.value = Value::sixteenth; break;
        }
    }

    Event makeRest (int start, int length)
    {
        Event rest;
        rest.isRest = true;
        rest.start = start;
        describeDuration (length, rest);
        return rest;
    }

    /** Fills the gaps in a bar with rests (R10). */
    void addRests (Bar& bar)
    {
        if (bar.events.empty())
        {
            bar.events.push_back (makeRest (0, unitsPerBar));
            return;
        }

        std::vector<std::pair<int, int>> gaps;
        int cursor = 0;

        for (auto& e : bar.events)
        {
            if (e.start > cursor)
                gaps.push_back ({ cursor, e.start });

            cursor = std::max (cursor, e.start + e.length);
        }

        if (cursor < unitsPerBar)
            gaps.push_back ({ cursor, unitsPerBar });

        auto small = [&bar] (int p, int q)
        {
            while (p < q)
            {
                int d;

                if (p % 3 != 0 || q % 3 != 0)
                    d = (q - p >= 8 && p % 4 == 0) ? 8 : (p % 4 == 0 ? 4 : q - p);
                else
                    d = (q - p >= 6 && p % 6 == 0) ? 6 : 3;

                bar.events.push_back (makeRest (p, d));
                p += d;
            }
        };

        for (auto [a, b] : gaps)
        {
            int p = a;

            while (p < b)
            {
                if (p % unitsPerBeat != 0)
                {
                    const int q = std::min (b, (p / unitsPerBeat + 1) * unitsPerBeat);
                    small (p, q);
                    p = q;
                }
                else if ((p == 0 || p == 24) && b >= p + 24)
                {
                    bar.events.push_back (makeRest (p, 24));
                    p += 24;
                }
                else if (b >= p + unitsPerBeat)
                {
                    bar.events.push_back (makeRest (p, unitsPerBeat));
                    p += unitsPerBeat;
                }
                else
                {
                    small (p, b);
                    p = b;
                }
            }
        }

        std::sort (bar.events.begin(), bar.events.end(), [] (const Event& x, const Event& y) { return x.start < y.start; });
    }

    /** Which way a stem goes for notes at these staff steps: the note furthest from the middle line decides, a tie goes down (R12). */
    bool stemGoesUp (const std::vector<int>& steps)
    {
        int furthest = -1;
        bool up = false;

        for (int s : steps)
        {
            const int distance = std::abs (s - 4);

            if (distance > furthest)
            {
                furthest = distance;
                up = s < 4;
            }
            else if (distance == furthest)
            {
                up = up && s < 4;
            }
        }

        return up;
    }

    struct Span
    {
        int start = 0, end = 0;
        std::vector<std::pair<int, int>> notes;     // pitch, source
    };

    /** One voice: notes starting together form a chord, which lasts until its longest note ends or the next chord starts (R8). */
    std::vector<Span> makeSpans (std::vector<QNote> notes)
    {
        std::sort (notes.begin(), notes.end(), [] (const QNote& a, const QNote& b)
                   { return a.start != b.start ? a.start < b.start : a.pitch < b.pitch; });

        std::vector<Span> spans;

        for (auto& n : notes)
        {
            if (spans.empty() || spans.back().start != n.start)
            {
                spans.push_back ({ n.start, n.end, {} });
            }
            else
            {
                spans.back().end = std::max (spans.back().end, n.end);
            }

            spans.back().notes.push_back ({ n.pitch, n.source });
        }

        for (size_t i = 0; i + 1 < spans.size(); ++i)
            spans[i].end = std::min (spans[i].end, spans[i + 1].start);

        return spans;
    }

    /** Where in a bar everything sits, in staff spaces. */
    constexpr double accidentalWidth = 1.4;

    double getGap (double beats)
    {
        return 2.2 + 3.2 * std::sqrt (beats);
    }

    void engrave (Score& score)
    {
        score.barWidths.assign ((size_t) score.numBars, 0.0);

        for (int b = 0; b < score.numBars; ++b)
        {
            std::map<int, bool> accidentalAt, dottedAt;

            for (auto& staff : score.staves)
                for (auto& e : staff.bars[(size_t) b].events)
                {
                    accidentalAt[e.start] = accidentalAt[e.start] || std::any_of (e.pitches.begin(), e.pitches.end(),
                                                                                  [] (const WrittenPitch& p) { return p.showAccidental; });
                    dottedAt[e.start] = dottedAt[e.start] || e.dotted;
                }

            std::map<int, double> x;
            double cursor = 1.0;
            int previous = -1;

            for (auto& [start, hasAccidental] : accidentalAt)
            {
                if (previous < 0)
                    cursor = 1.0;
                else
                    cursor += getGap ((start - previous) / (double) unitsPerBeat) + (dottedAt[previous] ? 0.8 : 0.0);

                cursor += hasAccidental ? accidentalWidth : 0.0;
                x[start] = cursor;
                previous = start;
            }

            const double width = cursor + getGap ((unitsPerBar - previous) / (double) unitsPerBeat) + (dottedAt[previous] ? 0.8 : 0.0);
            score.barWidths[(size_t) b] = width;

            for (auto& staff : score.staves)
                for (auto& e : staff.bars[(size_t) b].events)
                    e.x = x[e.start];
        }
    }
} // namespace

BeatFit fitBeat (const std::vector<double>& points)
{
    BeatFit fit;
    bool offEighth = false;

    for (double p : points)
    {
        fit.sixteenthError += std::abs (p - std::round (p * 4.0) / 4.0);
        fit.tripletError += std::abs (p - std::round (p * 3.0) / 3.0);
        offEighth = offEighth || std::abs (p - std::round (p * 2.0) / 2.0) > 1e-6;
    }

    fit.triplets = offEighth && fit.tripletError < 0.5 * fit.sixteenthError;
    return fit;
}

Pitch spell (int midi, const MusicalKey& key, const Chord* chordOfBar)
{
    const int pc = ((midi % 12) + 12) % 12;
    const int tonicLetter = getTonicLetter (key);
    const auto intervals = key.getIntervals();

    for (int i = 0; i < intervals.size(); ++i)
        if ((key.tonic + intervals[i]) % 12 == pc)
            return makePitch (midi, (tonicLetter + i) % 7);

    if (chordOfBar != nullptr)
    {
        for (auto& name : juce::StringArray::fromTokens (sheet::describe (key, *chordOfBar).notes, " ", ""))
        {
            const int letter = letterOf (name[0]);
            int alter = 0;

            for (int i = 1; i < name.length(); ++i)
                alter += name[i] == '#' ? 1 : -1;

            if (letter >= 0 && ((naturals[letter] + alter) % 12 + 12) % 12 == pc)
                return makePitch (midi, letter);
        }
    }

    if (harmony::isMinorKey (key))
    {
        if (pc == (key.tonic + 9) % 12)
            return makePitch (midi, (tonicLetter + 5) % 7);

        if (pc == (key.tonic + 11) % 12)
            return makePitch (midi, (tonicLetter + 6) % 7);
    }

    static const char* flats[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    static const char* sharps[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const char* name = (signatureHasSharps (key) ? sharps : flats)[pc];
    return makePitch (midi, letterOf (name[0]));
}

int getStaffStep (Clef clef, const Pitch& pitch)
{
    const int bottom = clef == Clef::treble ? 4 * 7 + 2 : 2 * 7 + 4;   // E4 or G2
    return pitch.octave * 7 + pitch.letter - bottom;
}

int getLedgerLines (int step)
{
    if (step >= 10)
        return (step - 8) / 2;

    if (step <= -2)
        return -step / 2;

    return 0;
}

ClefChoice chooseAutoClef (const std::vector<int>& midiNotes)
{
    if (midiNotes.empty())
        return ClefChoice::treble;

    auto sorted = midiNotes;
    std::sort (sorted.begin(), sorted.end());
    const bool trebleOk = sorted.front() >= 53;     // F3
    const bool bassOk = sorted.back() <= 67;        // G4

    if (trebleOk && bassOk)
        return sorted[sorted.size() / 2] >= 60 ? ClefChoice::treble : ClefChoice::bass;

    if (trebleOk)
        return ClefChoice::treble;

    if (bassOk)
        return ClefChoice::bass;

    return ClefChoice::grand;
}

std::vector<KeyMark> getKeySignature (int circlePosition, Clef clef)
{
    static constexpr int sharpLetters[] = { 3, 0, 4, 1, 5, 2, 6 };      // F C G D A E B
    static constexpr int sharpSteps[] = { 8, 5, 9, 6, 3, 7, 4 };
    static constexpr int flatLetters[] = { 6, 2, 5, 1, 4, 0, 3 };       // B E A D G C F
    static constexpr int flatSteps[] = { 4, 7, 3, 6, 2, 5, 1 };

    std::vector<KeyMark> marks;
    const auto signature = guide::getKeySignature (circlePosition);
    const int shift = clef == Clef::treble ? 0 : 2;

    for (int i = 0; i < signature.size(); ++i)
    {
        const bool sharp = signature[i].containsChar ('#');
        marks.push_back ({ sharp ? sharpLetters[i] : flatLetters[i], sharp ? 1 : -1, (sharp ? sharpSteps[i] : flatSteps[i]) - shift });
    }

    return marks;
}

Score build (const Input& input)
{
    Score score;
    score.key = input.key;
    score.keyPosition = guide::getCirclePosition (input.key);

    // The clef.
    std::vector<int> pitches;

    for (auto& n : input.notes)
        pitches.push_back (n.pitch);

    ClefChoice used = input.clef == ClefChoice::automatic ? chooseAutoClef (pitches) : input.clef;
    score.clefUsed = used;

    switch (used)
    {
        case ClefChoice::grand:      score.staves = { { Clef::treble, 0, {} }, { Clef::bass, 0, {} } }; break;
        case ClefChoice::bass:       score.staves = { { Clef::bass, 0, {} } }; break;
        case ClefChoice::guitar:     score.staves = { { Clef::treble, 12, {} } }; break;
        case ClefChoice::bassGuitar: score.staves = { { Clef::bass, 12, {} } }; break;
        case ClefChoice::automatic:
        case ClefChoice::treble:     score.staves = { { Clef::treble, 0, {} } }; break;
    }

    // Notes to staves (a grand staff splits at middle C), then rounded and made into one voice each.
    std::vector<std::vector<std::pair<MidiNote, int>>> perStaff (score.staves.size());

    for (size_t i = 0; i < input.notes.size(); ++i)
    {
        MidiNote n = input.notes[i];

        if (n.start < 0.0)
        {
            n.length += n.start;
            n.start = 0.0;
        }

        if (n.length <= 0.0 || n.pitch < 0 || n.pitch > 127)
            continue;

        const size_t staff = score.isGrand() && n.pitch < 60 ? 1 : 0;
        perStaff[staff].push_back ({ n, (int) i });
    }

    std::vector<std::vector<Span>> spans;
    int lastUnit = 1;

    for (auto& list : perStaff)
    {
        spans.push_back (makeSpans (quantise (list)));

        for (auto& s : spans.back())
            lastUnit = std::max (lastUnit, s.end);
    }

    score.numBars = std::max (1, (lastUnit + unitsPerBar - 1) / unitsPerBar);
    const auto keyAlters = getKeyAlterations (score.keyPosition);

    for (size_t si = 0; si < score.staves.size(); ++si)
    {
        auto& staff = score.staves[si];
        staff.bars.assign ((size_t) score.numBars, {});

        for (auto& span : spans[si])
        {
            // Spell each note by the chord of the bar it starts in.
            const int startBar = span.start / unitsPerBar;
            const auto chord = input.chords.find (startBar);
            std::vector<WrittenPitch> written;
            std::vector<int> sources;
            std::set<int> seen;

            for (auto& [midi, source] : span.notes)
            {
                sources.push_back (source);

                if (! seen.insert (midi).second)
                    continue;

                WrittenPitch w;
                static_cast<Pitch&> (w) = spell (midi, input.key, chord != input.chords.end() ? &chord->second : nullptr);
                w.octave += staff.writtenShift / 12;
                w.step = getStaffStep (staff.clef, w);
                w.midi = midi;
                written.push_back (w);
            }

            // Split it into written pieces.
            std::vector<std::pair<int, Event>> pieces;
            int p = span.start;

            while (p < span.end)
            {
                const int bar = p / unitsPerBar;
                const int barStart = bar * unitsPerBar;
                const int limit = std::min (span.end, barStart + unitsPerBar);

                for (int d : splitDurations (p - barStart, limit - barStart))
                {
                    Event e;
                    e.pitches = written;
                    e.sourceNotes = sources;
                    e.start = p - barStart;
                    describeDuration (d, e);
                    pieces.push_back ({ bar, e });
                    p += d;
                }
            }

            for (size_t i = 0; i < pieces.size(); ++i)
            {
                pieces[i].second.tiedToNext = i + 1 < pieces.size();
                pieces[i].second.tiedFromPrevious = i > 0;
                staff.bars[(size_t) pieces[i].first].events.push_back (pieces[i].second);
            }
        }

        for (auto& bar : staff.bars)
        {
            std::sort (bar.events.begin(), bar.events.end(), [] (const Event& a, const Event& b) { return a.start < b.start; });
            addRests (bar);

            // Accidentals: shown when the alteration differs from what the key signature and earlier notes in the bar say (R6).
            std::map<int, int> state;

            for (auto& e : bar.events)
                for (auto& w : e.pitches)
                {
                    const auto found = state.find (w.step);
                    const int current = found != state.end() ? found->second : keyAlters[(size_t) w.letter];

                    if (e.tiedFromPrevious)
                        continue;

                    w.showAccidental = w.alter != current;
                    state[w.step] = w.alter;
                }

            // Beams within one beat (R11).
            std::vector<int> group;
            auto flush = [&]
            {
                if (group.size() >= 2)
                {
                    for (int idx : group)
                        bar.events[(size_t) idx].beamGroup = (int) bar.beams.size();

                    bar.beams.push_back ({ group });
                }

                group.clear();
            };

            for (int i = 0; i < (int) bar.events.size(); ++i)
            {
                auto& e = bar.events[(size_t) i];
                const bool short_ = ! e.isRest && (e.value == Value::eighth || e.value == Value::sixteenth || e.value == Value::thirtySecond);

                if (! short_ || (! group.empty() && bar.events[(size_t) group.front()].start / unitsPerBeat != e.start / unitsPerBeat))
                    flush();

                if (short_)
                    group.push_back (i);
            }

            flush();

            // Stems (R12): a beamed group goes by its furthest note.
            for (auto& e : bar.events)
            {
                if (e.isRest || e.beamGroup >= 0)
                    continue;

                std::vector<int> steps;

                for (auto& w : e.pitches)
                    steps.push_back (w.step);

                e.stemUp = stemGoesUp (steps);
            }

            for (auto& beam : bar.beams)
            {
                std::vector<int> steps;

                for (int idx : beam.events)
                    for (auto& w : bar.events[(size_t) idx].pitches)
                        steps.push_back (w.step);

                const bool up = stemGoesUp (steps);

                for (int idx : beam.events)
                    bar.events[(size_t) idx].stemUp = up;
            }

            // Triplet brackets: one per beat that has triplet events.
            for (int i = 0; i < (int) bar.events.size(); ++i)
            {
                if (! bar.events[(size_t) i].triplet)
                    continue;

                const int beat = bar.events[(size_t) i].start / unitsPerBeat;
                int last = i;

                while (last + 1 < (int) bar.events.size() && bar.events[(size_t) last + 1].triplet
                       && bar.events[(size_t) last + 1].start / unitsPerBeat == beat)
                    ++last;

                bar.triplets.push_back ({ i, last });
                i = last;
            }
        }
    }

    // Tempo marks (R13).
    int lastMark = 0;

    for (auto& change : input.tempo.getChanges())
    {
        const int bpm = (int) std::lround (change.bpm);

        if (score.tempoMarks.empty() || std::abs (bpm - lastMark) >= 2)
        {
            const int bar = (int) std::floor (change.beat / 4.0);

            if (bar < score.numBars)
            {
                score.tempoMarks.push_back ({ bar, change.beat - bar * 4.0, bpm });
                lastMark = bpm;
            }
        }
    }

    // Chord symbols and section names (R14).
    std::set<int> sectionStarts;

    for (auto& s : input.sections)
        if (s.startBar >= 0 && s.startBar < score.numBars)
        {
            score.sectionMarks.push_back ({ s.startBar, s.getDisplayName() });
            sectionStarts.insert (s.startBar);
        }

    for (auto& [bar, chord] : input.chords)
    {
        if (bar < 0 || bar >= score.numBars)
            continue;

        const auto previous = input.chords.find (bar - 1);
        const bool changed = previous == input.chords.end() || ! (previous->second == chord);

        if (changed || sectionStarts.count (bar) > 0)
            score.chordSymbols.push_back ({ bar, sheet::describe (input.key, chord).name });
    }

    engrave (score);
    return score;
}

double getHeaderWidth (const Score& score, bool firstLine)
{
    const int count = guide::getKeySignature (score.keyPosition).size();
    return 3.6 + (count > 0 ? count * 1.0 + 0.8 : 0.0) + (firstLine ? 3.0 : 0.0);
}

std::vector<Line> layoutLines (const Score& score, double width, bool stretchLast)
{
    std::vector<Line> lines;
    int i = 0;

    while (i < score.numBars)
    {
        Line line;
        line.firstBar = i;
        line.hasTimeSignature = lines.empty();
        line.headerWidth = getHeaderWidth (score, lines.empty());
        double sum = 0.0;

        while (i + line.numBars < score.numBars)
        {
            const double w = score.barWidths[(size_t) (i + line.numBars)];

            if (line.numBars > 0 && line.headerWidth + sum + w > width)
                break;

            sum += w;
            ++line.numBars;
        }

        const bool last = i + line.numBars >= score.numBars;

        if ((! last || stretchLast) && line.headerWidth + sum < width)
            line.scale = (width - line.headerWidth) / sum;

        double x = line.headerWidth;

        for (int b = 0; b < line.numBars; ++b)
        {
            line.barOrigins.push_back (x);
            x += score.barWidths[(size_t) (i + b)] * line.scale;
        }

        line.width = x;
        lines.push_back (std::move (line));
        i += lines.back().numBars;
    }

    return lines;
}
} // namespace daw::gen::notation
