#include "NoteRealizer.h"
#include "ChordStyles.h"
#include "PromptBuilder.h"

namespace daw::gen
{
namespace
{
    double quantise (double beats, double grid) { return std::round (beats / grid) * grid; }

    /** Models sometimes wrap JSON in prose or code fences: keep only the outermost object. */
    juce::var parseReply (const juce::String& reply)
    {
        const int first = reply.indexOfChar ('{');
        const int last = reply.lastIndexOfChar ('}');

        if (first < 0 || last <= first)
            return {};

        return juce::JSON::parse (reply.substring (first, last + 1));
    }

    int degreeToPitch (const MusicalKey& key, int basePitch, int degree, int octave)
    {
        const auto intervals = key.getIntervals();
        const int index = juce::jlimit (0, intervals.size() - 1, degree - 1);
        return juce::jlimit (0, 127, basePitch + intervals[index] + 12 * octave);
    }

    /** Lowest pitch with the key's tonic pitch class at or above `atLeast`. */
    int tonicAtOrAbove (const MusicalKey& key, int atLeast)
    {
        int pitch = atLeast;
        while (((pitch - key.tonic) % 12 + 12) % 12 != 0)
            ++pitch;

        return pitch;
    }

    float toVelocity (const juce::var& v, int fallback)
    {
        const int value = v.isVoid() ? fallback : (int) v;
        return (float) juce::jlimit (1, 127, value) / 127.0f;
    }

    /** Repeats a pattern of `patternBeats` until `totalBeats` is filled. */
    std::vector<MidiNote> tile (const std::vector<MidiNote>& pattern, double patternBeats, double totalBeats)
    {
        std::vector<MidiNote> out;

        for (double offset = 0.0; offset < totalBeats - 1.0e-6; offset += patternBeats)
        {
            for (auto n : pattern)
            {
                n.start += offset;

                if (n.start >= totalBeats - 1.0e-6)
                    continue;

                n.length = juce::jmin (n.length, totalBeats - n.start);
                out.push_back (n);
            }
        }

        return out;
    }

    void sortByStart (std::vector<MidiNote>& notes)
    {
        std::stable_sort (notes.begin(), notes.end(), [] (auto& a, auto& b)
        {
            return ! juce::exactlyEqual (a.start, b.start) ? a.start < b.start : a.pitch < b.pitch;
        });
    }

    /** One note at a time: drop duplicates on the same start and trim notes that overlap the next. */
    void makeMonophonic (std::vector<MidiNote>& notes)
    {
        sortByStart (notes);
        std::vector<MidiNote> out;

        for (auto& n : notes)
        {
            if (! out.empty() && std::abs (out.back().start - n.start) < 1.0e-6)
                continue;

            if (! out.empty() && out.back().getEnd() > n.start)
                out.back().length = n.start - out.back().start;

            out.push_back (n);
        }

        notes = std::move (out);
    }

    Chord chordOnDegree (const MusicalKey& key, int degree, const juce::String& colour)
    {
        degree = juce::jlimit (1, 7, degree);
        return { (key.tonic + key.getIntervals()[degree - 1]) % 12, getChordIntervals (key, degree, colour) };
    }

    /** The reply's "chords" list (one chord per bar of the pattern), if the request asked for one. */
    std::vector<Chord> parseBarChords (const Request& r, const juce::var& reply)
    {
        std::vector<Chord> chords;

        if (const auto* list = reply["chords"].getArray(); list != nullptr && r.asksForBarChords())
            for (auto& c : *list)
                if ((int) chords.size() < r.getPatternBars())
                    chords.push_back (chordOnDegree (r.key, (int) c["degree"], c["colour"].toString()));

        return chords;
    }

    /** Repeats a pattern of per-bar chords over the clip, on the bars the chord track leaves empty. */
    void fillEmptyBars (const Request& r, const std::vector<Chord>& pattern, Result& result)
    {
        result.newChords.assign ((size_t) r.bars, std::nullopt);

        if (pattern.empty())
            return;

        for (int bar = 0; bar < r.bars; ++bar)
            if (! r.getTrackChord (bar).has_value())
                result.newChords[(size_t) bar] = pattern[(size_t) (bar % r.getPatternBars()) % pattern.size()];
    }

    /** The chord of a bar: the chord track's, or else the one the model wrote for it. */
    std::optional<Chord> chordOfBar (const Request& r, const Result& result, int bar)
    {
        if (auto chord = r.getTrackChord (bar))
            return chord;

        return bar >= 0 && bar < (int) result.newChords.size() ? result.newChords[(size_t) bar] : std::nullopt;
    }

    /** e.g. "Am - F - C - G", with a chord held over several bars named once. */
    juce::String describeBarChords (const Request& r, const Result& result, int bars)
    {
        juce::StringArray names;

        for (int bar = 0; bar < bars; ++bar)
        {
            const auto chord = chordOfBar (r, result, bar);
            const auto name = chord.has_value() ? chord->getName() : juce::String ("-");

            if (names.isEmpty() || names[names.size() - 1] != name)
                names.add (name);
        }

        return names.joinIntoString (" - ");
    }
} // namespace

//==============================================================================
std::vector<int> voiceChord (const juce::Array<int>& pitchClasses, const std::vector<int>& previous, int low, int high)
{
    std::vector<std::vector<int>> candidates;

    for (int rotation = 0; rotation < pitchClasses.size(); ++rotation)
    {
        for (int bottom = low; bottom < low + 12; ++bottom)
        {
            if (bottom % 12 != pitchClasses[rotation])
                continue;

            std::vector<int> voicing { bottom };

            for (int i = 1; i < pitchClasses.size(); ++i)
            {
                int p = voicing.back() + 1;
                while (p % 12 != pitchClasses[(rotation + i) % pitchClasses.size()])
                    ++p;

                voicing.push_back (p);
            }

            if (voicing.back() <= high)
                candidates.push_back (voicing);
        }
    }

    if (candidates.empty())
        return {};

    auto cost = [&previous, low] (const std::vector<int>& v)
    {
        if (previous.empty()) // first chord: centre it an octave above the bottom (middle C by default)
        {
            double mean = 0.0;
            for (int p : v)
                mean += p;

            return std::abs (mean / (double) v.size() - (low + 12.0));
        }

        double total = 0.0;
        for (int p : v)
        {
            int nearest = 127;
            for (int q : previous)
                nearest = juce::jmin (nearest, std::abs (p - q));

            total += nearest;
        }

        return total;
    };

    return *std::min_element (candidates.begin(), candidates.end(),
                              [&] (auto& a, auto& b) { return cost (a) < cost (b); });
}

namespace
{
    bool realizeChords (const Request& r, const juce::var& reply, Result& result, juce::String& error)
    {
        const auto* chords = reply["chords"].getArray();

        if (chords == nullptr || chords->isEmpty())
        {
            error = "The model returned no chords.";
            return false;
        }

        const double patternBeats = r.getPatternBars() * 4.0;
        std::vector<MidiNote> pattern;
        std::vector<int> previous;
        juce::StringArray symbols;
        std::vector<std::pair<Chord, juce::Range<double>>> spans;
        double position = 0.0;

        for (int i = 0; i < chords->size() && position < patternBeats - 1.0e-6; ++i)
        {
            const auto& entry = chords->getReference (i);
            const int degree = juce::jlimit (1, 7, (int) entry["degree"]);
            const auto colour = entry["colour"].toString();
            double beats = juce::jmax (0.5, quantise ((double) entry["beats"], 0.5));

            // The last chord fills whatever is left of the pattern.
            if (i == chords->size() - 1 || position + beats > patternBeats)
                beats = patternBeats - position;

            const auto chord = chordOnDegree (r.key, degree, colour);
            const auto voicing = voiceChord (chord.getPitchClasses(), previous);

            for (int pitch : voicing)
                pattern.push_back ({ pitch, position, beats, 0.7f });

            previous = voicing;
            symbols.add (chord.getName());
            spans.push_back ({ chord, { position, position + beats } });
            position += beats;
        }

        // For the chord track: the chord that sounds longest in each bar.
        std::vector<Chord> perBar;

        for (int bar = 0; bar < r.getPatternBars(); ++bar)
        {
            const juce::Range<double> barRange (bar * 4.0, bar * 4.0 + 4.0);
            const auto longest = std::max_element (spans.begin(), spans.end(), [&barRange] (auto& a, auto& b)
            {
                return a.second.getIntersectionWith (barRange).getLength() < b.second.getIntersectionWith (barRange).getLength();
            });

            if (longest != spans.end())
                perBar.push_back (longest->first);
        }

        fillEmptyBars (r, perBar, result);
        result.notes = tile (pattern, patternBeats, r.bars * 4.0);
        result.summary = symbols.joinIntoString (" - ");
        return true;
    }

    /** A chord part over the chord track: the model's rhythm, voiced with each bar's chord. */
    bool realizeChordRhythm (const Request& r, const juce::var& reply, Result& result, juce::String& error)
    {
        const auto* hits = reply["hits"].getArray();

        if (hits == nullptr || hits->isEmpty())
        {
            error = "The model returned no chord rhythm.";
            return false;
        }

        fillEmptyBars (r, parseBarChords (r, reply), result);

        const double rhythmBeats = r.getRhythmBars() * 4.0;
        std::vector<MidiNote> rhythm;

        for (auto& h : *hits)
        {
            const double start = quantise ((double) h["start"], 0.25);

            if (start < 0.0 || start >= rhythmBeats)
                continue;

            const double length = juce::jlimit (0.25, rhythmBeats - start, quantise ((double) h["length"], 0.25));
            rhythm.push_back ({ 0, start, length, toVelocity (h["velocity"], 90) });
        }

        if (rhythm.empty())
        {
            error = "None of the model's chord hits fell inside the pattern.";
            return false;
        }

        makeMonophonic (rhythm); // one chord at a time

        std::vector<int> voicing;
        std::optional<Chord> voiced;

        for (auto& hit : tile (rhythm, rhythmBeats, r.bars * 4.0))
        {
            // A hit that rings into a bar with another chord is split there, so each note fits its bar.
            for (double start = hit.start; start < hit.getEnd() - 1.0e-6;)
            {
                const int bar = (int) std::floor (start / 4.0 + 1.0e-9);
                const auto chord = chordOfBar (r, result, bar);
                int nextBar = bar + 1;

                while (nextBar * 4.0 < hit.getEnd() && chordOfBar (r, result, nextBar) == chord)
                    ++nextBar;

                const double end = juce::jmin (hit.getEnd(), nextBar * 4.0);

                if (chord.has_value())
                {
                    if (chord != voiced)
                    {
                        voicing = voiceChord (chord->getPitchClasses(), voicing);
                        voiced = chord;
                    }

                    for (int pitch : voicing)
                        result.notes.push_back ({ pitch, start, end - start, hit.velocity });
                }

                start = end;
            }
        }

        if (result.notes.empty())
        {
            error = "The chord rhythm only plays where there are no chords.";
            return false;
        }

        sortByStart (result.notes);
        result.summary = describeBarChords (r, result, r.getPatternBars()) + " (" + juce::String ((int) rhythm.size())
                       + " hits per " + juce::String (r.getRhythmBars()) + "-bar rhythm)";
        return true;
    }

    /** A part in a playing style: the style's pattern (or the model's variation of it), voiced on
        each bar's chord. */
    bool realizeStyle (const Request& r, const juce::var& reply, Result& result, juce::String& error)
    {
        const auto* base = styles::findStyle (r.playingStyle);

        if (base == nullptr)
        {
            error = "Unknown playing style \"" + r.playingStyle + "\".";
            return false;
        }

        auto style = *base;

        if (r.varyStyle)
        {
            // Events the model got wrong are left out; the take only fails if none is usable.
            style.bars = r.getStyleBars();
            style.events.clear();

            if (const auto* events = reply["events"].getArray())
            {
                for (auto& item : *events)
                {
                    juce::String ignored;

                    if (auto e = styles::parseEvent (item, style.getLengthBeats(), 127.0, ignored); e && styles::playsRole (style.instrument, e->role))
                        style.events.push_back (*e);
                }
            }

            if (style.events.empty())
            {
                error = "The model wrote no usable playing pattern.";
                return false;
            }
        }

        if (r.asksForBarChords())
            fillEmptyBars (r, parseBarChords (r, reply), result);

        std::vector<styles::ChordSpan> spans;

        for (int bar = 0; bar < r.bars; ++bar)
            if (const auto chord = chordOfBar (r, result, bar))
                spans.push_back ({ bar * 4.0, bar * 4.0 + 4.0, *chord });

        if (spans.empty())
        {
            error = "There are no chords to play: set some on the chord track.";
            return false;
        }

        result.notes = styles::play (style, spans, r.bars * 4.0, r.bpm);

        if (result.notes.empty())
        {
            error = "The playing pattern made no notes.";
            return false;
        }

        result.summary = style.name + (r.varyStyle ? " (varied, " + juce::String ((int) style.events.size()) + " events)" : juce::String())
                         + " over " + describeBarChords (r, result, r.bars);
        return true;
    }

    bool realizeLine (const Request& r, const juce::var& reply, Result& result, juce::String& error)
    {
        const auto* notes = reply["notes"].getArray();

        if (notes == nullptr || notes->isEmpty())
        {
            error = "The model returned no notes.";
            return false;
        }

        const double patternBeats = r.getPatternBars() * 4.0;
        const int base = r.part == Part::bassline ? tonicAtOrAbove (r.key, 31)   // G1..F#2
                       : r.part == Part::arpeggio ? tonicAtOrAbove (r.key, 55)   // G3..F#4
                                                  : tonicAtOrAbove (r.key, 60);  // C4..B4
        std::vector<MidiNote> pattern;

        for (auto& n : *notes)
        {
            const double start = quantise ((double) n["start"], 0.25);

            if (start < 0.0 || start >= patternBeats)
                continue;

            MidiNote note;
            note.start = start;
            note.length = juce::jlimit (0.25, patternBeats - start, quantise ((double) n["length"], 0.25));
            note.pitch = degreeToPitch (r.key, base, (int) n["degree"], juce::jlimit (-2, 2, (int) n["octave"]));
            note.velocity = toVelocity (n["velocity"], 100);
            pattern.push_back (note);
        }

        if (pattern.empty())
        {
            error = "None of the model's notes fell inside the pattern.";
            return false;
        }

        if (r.part != Part::arpeggio)
            makeMonophonic (pattern);
        else
            sortByStart (pattern);

        fillEmptyBars (r, parseBarChords (r, reply), result);
        result.notes = tile (pattern, patternBeats, r.bars * 4.0);
        result.summary = juce::String ((int) pattern.size()) + " notes"
                       + (r.getPatternBars() < r.bars ? " (" + juce::String (r.getPatternBars()) + "-bar phrase, repeated)" : juce::String());
        return true;
    }

    /** Drums and percussion: named hits on the General MIDI drum map. */
    bool realizeHits (const Request& r, const juce::var& reply, Result& result, juce::String& error)
    {
        const auto* hits = reply["hits"].getArray();
        const bool drums = r.part == Part::drums;
        const juce::String what = drums ? "drum" : "percussion";

        if (hits == nullptr || hits->isEmpty())
        {
            error = "The model returned no " + what + " hits.";
            return false;
        }

        const double patternBeats = r.getPatternBars() * 4.0;
        std::vector<MidiNote> pattern;

        for (auto& h : *hits)
        {
            const int pitch = drums ? getDrumNote (h["drum"].toString()) : getPercussionNote (h["instrument"].toString());
            const double start = quantise ((double) h["start"], 0.125);

            if (pitch < 0 || start < 0.0 || start >= patternBeats)
                continue;

            const bool duplicate = std::any_of (pattern.begin(), pattern.end(), [&] (auto& n)
            {
                return n.pitch == pitch && std::abs (n.start - start) < 1.0e-6;
            });

            if (! duplicate)
                pattern.push_back ({ pitch, start, 0.25, toVelocity (h["velocity"], 100) });
        }

        if (pattern.empty())
        {
            error = "None of the model's " + what + " hits could be used.";
            return false;
        }

        sortByStart (pattern);
        result.notes = tile (pattern, patternBeats, r.bars * 4.0);
        result.summary = juce::String ((int) pattern.size()) + " hits per " + juce::String (r.getPatternBars()) + "-bar loop";
        return true;
    }
}

juce::Array<int> getChordIntervals (const MusicalKey& key, int degree, const juce::String& colour)
{
    const auto scale = key.getIntervals();
    const int d = juce::jlimit (1, 7, degree) - 1;

    // Semitones from the root to the scale note `steps` degrees above it.
    auto above = [&] (int steps)
    {
        const int index = d + steps;
        return scale[index % 7] + 12 * (index / 7) - scale[d];
    };

    if (colour == "borrowed_major") return { 0, 4, 7 };
    if (colour == "borrowed_minor") return { 0, 3, 7 };
    if (colour == "sus2")           return { 0, 2, above (4) };
    if (colour == "sus4")           return { 0, 5, above (4) };
    if (colour == "seventh")        return { 0, above (2), above (4), above (6) };
    if (colour == "ninth")          return { 0, above (2), above (4), above (6), above (8) };
    if (colour == "add9")           return { 0, above (2), above (4), above (8) };
    return { 0, above (2), above (4) }; // triad
}

juce::String getChordName (int root, const juce::Array<int>& intervals)
{
    return Chord::getName (root, intervals);
}

bool realize (const Request& request, const juce::String& modelReply, Result& result, juce::String& error)
{
    result = {};
    result.lengthBeats = request.bars * 4.0;

    const auto reply = parseReply (modelReply);

    if (! reply.isObject())
    {
        error = "The model's reply wasn't valid JSON.";
        return false;
    }

    if (request.usesPlayingStyle())
        return realizeStyle (request, reply, result, error);

    if (request.isChordRhythm())
        return realizeChordRhythm (request, reply, result, error);

    switch (request.part)
    {
        case Part::chords: return realizeChords (request, reply, result, error);
        case Part::drums:
        case Part::percussion: return realizeHits (request, reply, result, error);
        case Part::melody:
        case Part::bassline:
        case Part::arpeggio:
            break;
    }

    return realizeLine (request, reply, result, error);
}
} // namespace daw::gen
