#include "ChordStyles.h"
#include "NoteRealizer.h"

#include "DawItStyles.h"

namespace daw::gen::styles
{
namespace
{
    constexpr double epsilon = 1.0e-6;
    constexpr double shortestNote = 0.02;   // beats

    constexpr std::array<int, 6> openStrings { 40, 45, 50, 55, 59, 64 };   // E2 A2 D3 G3 B3 E4

    juce::String getInstrumentId (Instrument i)
    {
        switch (i)
        {
            case Instrument::piano:   return "piano";
            case Instrument::guitar:  return "guitar";
            case Instrument::synth:   return "synth";
            case Instrument::strings: return "strings";
            case Instrument::bass:    return "bass";
        }

        return {};
    }

    BuiltInSound getDefaultSound (Instrument i)
    {
        switch (i)
        {
            case Instrument::piano:   return BuiltInSound::piano;
            case Instrument::guitar:
            case Instrument::synth:   return BuiltInSound::pluck;
            case Instrument::strings: return BuiltInSound::pad;
            case Instrument::bass:    return BuiltInSound::bass;
        }

        return BuiltInSound::piano;
    }

    const std::pair<const char*, Role> roleIds[] {
        { "bass", Role::bass },   { "third", Role::third }, { "fifth", Role::fifth }, { "octave", Role::octave },
        { "tone", Role::tone },   { "top", Role::top },     { "upper", Role::upper }, { "all", Role::all },
        { "approach", Role::approach }, { "mute", Role::mute }
    };

    /** The first of `choices` (semitones above the root) that the chord has, else `fallback`. */
    int intervalOf (const Chord& chord, std::initializer_list<int> choices, int fallback)
    {
        for (int choice : choices)
            for (int i : chord.intervals)
                if (i % 12 == choice)
                    return choice;

        return fallback;
    }

    int thirdOf (const Chord& chord) { return intervalOf (chord, { 4, 3, 5, 2 }, 4); }   // the sus note stands in for the third
    int fifthOf (const Chord& chord) { return intervalOf (chord, { 7, 6, 8 }, 7); }

    /** The pitch of this pitch class between `low` and `high` (at least an octave apart) closest to `target`. */
    int nearestPitch (int pitchClass, int low, int high, double target)
    {
        int best = -1;

        for (int p = low; p <= high; ++p)
            if (p % 12 == pitchClass && (best < 0 || std::abs (p - target) < std::abs (best - target)))
                best = p;

        return best;
    }

    /** The lowest note of `pitches` with this pitch class, or `fallback`. */
    int findPitchClass (const std::vector<int>& pitches, int pitchClass, int fallback)
    {
        for (int p : pitches)
            if (p % 12 == pitchClass)
                return p;

        return fallback;
    }

    /** Joins neighbouring spans of the same chord, so a note held over a bar line rings on. */
    std::vector<ChordSpan> mergeSpans (const std::vector<ChordSpan>& spans)
    {
        std::vector<ChordSpan> merged;

        for (auto& span : spans)
        {
            if (! merged.empty() && std::abs (merged.back().end - span.start) < epsilon && merged.back().chord == span.chord)
                merged.back().end = span.end;
            else
                merged.push_back (span);
        }

        return merged;
    }

    std::vector<int> pitchesFor (const Event& e, const Voicing& v, const Voicing* next)
    {
        std::vector<int> pitches;

        switch (e.role)
        {
            case Role::bass:   pitches = { v.bass }; break;
            case Role::third:  pitches = { v.third }; break;
            case Role::fifth:  pitches = { v.fifth }; break;
            case Role::octave: pitches = { v.bass + 12 }; break;
            case Role::top:    pitches = { v.upper.empty() ? v.bass : v.upper.back() }; break;
            case Role::upper:  pitches = v.upper; break;

            case Role::all:
            case Role::mute:
                pitches = v.upper;
                pitches.insert (pitches.begin(), v.bass);
                break;

            case Role::tone:
            {
                if (v.upper.empty())
                {
                    pitches = { v.bass };
                    break;
                }

                const int size = (int) v.upper.size();
                pitches = { v.wrapsOctaves ? v.upper[(size_t) (e.index % size)] + 12 * (e.index / size)
                                           : v.upper[(size_t) juce::jmin (e.index, size - 1)] };
                break;
            }

            case Role::approach:
                // A half step up into the next chord's bass, or the fifth below when the bass stays.
                pitches = { next != nullptr && next->bass != v.bass ? next->bass - 1 : v.fifth - 12 };
                break;
        }

        for (auto& p : pitches)
            p += 12 * e.octave;

        pitches.erase (std::remove_if (pitches.begin(), pitches.end(), [] (int p) { return p < 0 || p > 127; }), pitches.end());
        return pitches;
    }

    /** One pitch sounds once at a time: a note ends where the same pitch starts again, and two
        that start together become one. */
    void removeOverlaps (std::vector<MidiNote>& notes)
    {
        std::stable_sort (notes.begin(), notes.end(), [] (auto& a, auto& b)
        {
            return ! juce::exactlyEqual (a.start, b.start) ? a.start < b.start : a.pitch < b.pitch;
        });

        std::vector<MidiNote> out;
        std::array<int, 128> last;
        last.fill (-1);

        for (auto& n : notes)
        {
            if (const int i = last[(size_t) n.pitch]; i >= 0)
            {
                auto& before = out[(size_t) i];

                if (std::abs (before.start - n.start) < epsilon)
                {
                    before.length = juce::jmax (before.length, n.length);
                    before.velocity = juce::jmax (before.velocity, n.velocity);
                    continue;
                }

                if (before.getEnd() > n.start)
                    before.length = n.start - before.start;
            }

            last[(size_t) n.pitch] = (int) out.size();
            out.push_back (n);
        }

        out.erase (std::remove_if (out.begin(), out.end(), [] (auto& n) { return n.length < shortestNote; }), out.end());
        notes = std::move (out);
    }
} // namespace

//==============================================================================
juce::String getInstrumentName (Instrument i)
{
    switch (i)
    {
        case Instrument::piano:   return "Piano and keys";
        case Instrument::guitar:  return "Guitar";
        case Instrument::synth:   return "Synth";
        case Instrument::strings: return "Strings";
        case Instrument::bass:    return "Bass";
    }

    return {};
}

std::optional<Instrument> instrumentFromId (const juce::String& id)
{
    for (auto i : allInstruments)
        if (getInstrumentId (i) == id)
            return i;

    return std::nullopt;
}

bool Style::suitsGenre (const juce::String& genre) const
{
    for (auto& g : genres)
        if (g.equalsIgnoreCase (genre.trim()))
            return true;

    return false;
}

juce::String getRoleId (Role role)
{
    for (auto& [name, r] : roleIds)
        if (r == role)
            return name;

    return {};
}

bool playsRole (Instrument instrument, Role role)
{
    return instrument != Instrument::bass || ! (role == Role::top || role == Role::upper || role == Role::all || role == Role::mute);
}

std::optional<Role> roleFromId (const juce::String& id)
{
    for (auto& [name, role] : roleIds)
        if (id == name)
            return role;

    return std::nullopt;
}

juce::StringArray getRoleIds()
{
    juce::StringArray ids;
    for (auto& [name, role] : roleIds)
        ids.add (name);

    return ids;
}

//==============================================================================
std::optional<Event> parseEvent (const juce::var& item, double patternBeats, double velocityScale, juce::String& error)
{
    Event e;
    e.start = (double) item["start"];
    e.length = (double) item.getProperty ("length", 1.0);

    if (e.start < 0.0 || e.start >= patternBeats - epsilon)
    {
        error = "an event at beat " + juce::String (e.start) + " lies outside the pattern.";
        return std::nullopt;
    }

    if (e.length < shortestNote)
    {
        error = "an event at beat " + juce::String (e.start) + " has no length.";
        return std::nullopt;
    }

    const auto role = roleFromId (item["role"].toString());

    if (! role.has_value())
    {
        error = "unknown role \"" + item["role"].toString() + "\".";
        return std::nullopt;
    }

    e.role = *role;
    e.index = juce::jlimit (0, 15, (int) item.getProperty ("index", 0));
    e.octave = juce::jlimit (-2, 2, (int) item.getProperty ("octave", 0));
    e.velocity = juce::jlimit (0.05f, 1.0f, (float) ((double) item.getProperty ("velocity", 0.75 * velocityScale) / velocityScale));

    const auto strum = item["strum"].toString();

    if (strum.isNotEmpty() && strum != "none" && strum != "down" && strum != "up")
    {
        error = "unknown strum \"" + strum + "\".";
        return std::nullopt;
    }

    e.strum = strum == "down" ? Strum::down : strum == "up" ? Strum::up : Strum::none;
    return e;
}

juce::String describeEvents (const std::vector<Event>& events)
{
    juce::StringArray lines;

    for (auto& e : events)
    {
        juce::String line;
        line << "{\"start\": " << e.start << ", \"length\": " << e.length << ", \"role\": \"" << getRoleId (e.role) << "\"";

        if (e.role == Role::tone)
            line << ", \"index\": " << e.index;

        if (e.octave != 0)
            line << ", \"octave\": " << e.octave;

        line << ", \"velocity\": " << juce::roundToInt (e.velocity * 127.0f);

        if (e.strum != Strum::none)
            line << ", \"strum\": \"" << (e.strum == Strum::down ? "down" : "up") << "\"";

        lines.add (line + "}");
    }

    return lines.joinIntoString ("\n");
}

//==============================================================================
std::optional<Style> parseStyle (const juce::var& v, juce::String& error)
{
    if (! v.isObject())
    {
        error = "A style must be a JSON object.";
        return std::nullopt;
    }

    Style style;
    style.id = v["id"].toString();
    style.name = v["name"].toString();
    style.description = v["description"].toString();

    if (style.name.isEmpty())
    {
        error = "A style needs a name.";
        return std::nullopt;
    }

    const auto fail = [&] (const juce::String& what) -> std::optional<Style>
    {
        error = "Style \"" + style.name + "\": " + what;
        return std::nullopt;
    };

    const auto instrument = instrumentFromId (v["instrument"].toString());

    if (! instrument.has_value())
        return fail ("unknown instrument \"" + v["instrument"].toString() + "\".");

    style.instrument = *instrument;
    style.sound = getDefaultSound (*instrument);

    if (v.hasProperty ("sound"))
    {
        const auto sound = builtInSounds::fromId (v["sound"].toString());

        if (! sound.has_value())
            return fail ("unknown sound \"" + v["sound"].toString() + "\".");

        style.sound = *sound;
    }

    const auto voicing = v["voicing"].toString();

    if (voicing.isNotEmpty() && voicing != "power")
        return fail ("unknown voicing \"" + voicing + "\".");

    style.powerChords = voicing == "power";
    style.strumMs = juce::jlimit (0.0, 60.0, (double) v.getProperty ("strumMs", 0.0));
    style.bars = juce::jlimit (1, 4, (int) v.getProperty ("bars", 1));

    const auto* events = v["events"].getArray();

    if (events == nullptr || events->isEmpty())
        return fail ("it has no events.");

    for (auto& item : *events)
    {
        juce::String why;
        const auto e = parseEvent (item, style.getLengthBeats(), 1.0, why);

        if (! e.has_value())
            return fail (why);

        style.events.push_back (*e);
    }

    static const juce::StringArray knownParts { "chords", "arpeggio", "bassline" };

    if (const auto* parts = v["parts"].getArray())
    {
        for (auto& part : *parts)
        {
            if (! knownParts.contains (part.toString()))
                return fail ("unknown part \"" + part.toString() + "\".");

            style.parts.add (part.toString());
        }
    }

    if (style.parts.isEmpty())
        style.parts.add (style.instrument == Instrument::bass ? "bassline" : "chords");

    if (const auto* genres = v["genres"].getArray())
        for (auto& genre : *genres)
            style.genres.add (genre.toString());

    return style;
}

std::vector<Style> parseStyles (const juce::String& json, juce::String& error)
{
    std::vector<Style> result;
    const auto parsed = juce::JSON::parse (json);
    const auto* list = parsed["styles"].getArray();

    if (list == nullptr)
    {
        error = "No \"styles\" list.";
        return {};
    }

    for (auto& item : *list)
    {
        auto style = parseStyle (item, error);

        if (! style.has_value())
            return {};

        result.push_back (std::move (*style));
    }

    return result;
}

const std::vector<Style>& getLibrary()
{
    static const auto library = []
    {
        juce::String error;
        auto styles = parseStyles (juce::String::fromUTF8 (dawstyles::ChordStyles_json, dawstyles::ChordStyles_jsonSize), error);
        jassert (error.isEmpty());   // the tests check the built-in library
        return styles;
    }();

    return library;
}

std::vector<const Style*> getStylesFor (Instrument instrument)
{
    std::vector<const Style*> found;

    for (auto& style : getLibrary())
        if (style.instrument == instrument)
            found.push_back (&style);

    return found;
}

const Style* findStyle (const juce::String& id)
{
    for (auto& style : getLibrary())
        if (style.id == id)
            return &style;

    return nullptr;
}

//==============================================================================
std::vector<ChordSpan> spansFromChordTrack (const ChordMap& chords, int firstBar, int endBar)
{
    std::vector<ChordSpan> spans;

    for (auto& [bar, chord] : chords)
        if (bar >= firstBar && bar < endBar)
            spans.push_back ({ (bar - firstBar) * 4.0, (bar - firstBar + 1) * 4.0, chord });

    return spans;
}

/** Calls `onShape` with every playable shape of the chord and its cost (lower is easier and nearer `previous`). */
static void forEachGuitarShape (const Chord& chord, const std::vector<int>* previous,
                                const std::function<void (const std::vector<int>&, double)>& onShape)
{
    const auto pitchClasses = chord.getPitchClasses();
    const int fifthClass = (chord.root + fifthOf (chord)) % 12;
    const bool fifthOptional = pitchClasses.size() > 3;   // with four or more notes, a shape may leave out the fifth

    // Fretted notes lie within four frets (window + 1 to window + 4); open strings can join in.
    for (int window = 0; window <= 9; ++window)
    {
        std::array<std::vector<int>, 6> options;

        for (int s = 0; s < 6; ++s)
        {
            if (pitchClasses.contains (openStrings[(size_t) s] % 12))
                options[(size_t) s].push_back (0);

            for (int fret = window + 1; fret <= window + 4; ++fret)
                if (pitchClasses.contains ((openStrings[(size_t) s] + fret) % 12))
                    options[(size_t) s].push_back (fret);
        }

        // The played strings are next to each other, at least four of them, from the lowest
        // (which plays the root) to the B or high E string.
        for (int lowest = 0; lowest <= 2; ++lowest)
        {
            for (int highest = juce::jmax (4, lowest + 3); highest <= 5; ++highest)
            {
                if (std::any_of (options.begin() + lowest, options.begin() + highest + 1, [] (auto& o) { return o.empty(); }))
                    continue;

                std::array<size_t, 6> pick {};

                for (;;)
                {
                    std::vector<int> shape (6, -1);
                    for (int s = lowest; s <= highest; ++s)
                        shape[(size_t) s] = options[(size_t) s][pick[(size_t) s]];

                    const bool complete = [&]
                    {
                        if ((openStrings[(size_t) lowest] + shape[(size_t) lowest]) % 12 != chord.root)
                            return false;

                        for (int pc : pitchClasses)
                        {
                            bool found = false;
                            for (int s = lowest; s <= highest; ++s)
                                found = found || (openStrings[(size_t) s] + shape[(size_t) s]) % 12 == pc;

                            if (! found && ! (fifthOptional && pc == fifthClass))
                                return false;
                        }

                        return true;
                    }();

                    if (complete)
                    {
                        int fretted = 0, lowestFret = 99, open = 0, fifths = 0;
                        for (int s = lowest; s <= highest; ++s)
                        {
                            const int f = shape[(size_t) s];
                            open += f == 0 ? 1 : 0;
                            fretted += f > 0 ? 1 : 0;
                            fifths += (openStrings[(size_t) s] + f) % 12 == fifthClass ? 1 : 0;

                            if (f > 0)
                                lowestFret = juce::jmin (lowestFret, f);
                        }

                        int atLowest = 0;
                        for (int f : shape)
                            atLowest += f == lowestFret ? 1 : 0;

                        // A barre holds all the strings at the lowest fret with one finger; it can't leave strings open.
                        const bool barre = atLowest >= 2 && open == 0 && fretted > 4;
                        const int fingers = barre ? 1 + fretted - atLowest : fretted;

                        if (fingers <= 4)
                        {
                            int highestFret = 0;
                            for (int f : shape)
                                highestFret = juce::jmax (highestFret, f);

                            double cost = (fretted > 0 ? lowestFret : 0) * 0.6
                                          + (fretted > 0 ? highestFret - lowestFret : 0) * 0.3   // a stretch
                                          - (highest - lowest + 1) * 0.8
                                          + (lowest == 2 ? 0.5 : 0.0)
                                          + (highest == 4 ? 0.6 : 0.0)                            // high E left out
                                          + (fifths == 0 ? 1.0 : 0.0)
                                          + (barre ? 1.0 : 0.0)
                                          + open * (window == 0 ? -0.4 : 1.0);

                            if (previous != nullptr && previous->size() == 6)
                                for (size_t s = 0; s < 6; ++s)
                                    if (shape[s] >= 0 && (*previous)[s] >= 0)
                                        cost += std::abs (shape[s] - (*previous)[s]) * 0.15;

                            onShape (shape, cost);
                        }
                    }

                    // Next combination of the strings' options.
                    int s = lowest;
                    for (; s <= highest; ++s)
                    {
                        if (++pick[(size_t) s] < options[(size_t) s].size())
                            break;

                        pick[(size_t) s] = 0;
                    }

                    if (s > highest)
                        break;
                }
            }
        }
    }
}

std::vector<int> findGuitarShape (const Chord& chord, const std::vector<int>* previous)
{
    std::vector<int> best;
    double bestCost = 0.0;

    forEachGuitarShape (chord, previous, [&] (const std::vector<int>& shape, double cost)
    {
        if (best.empty() || cost < bestCost)
        {
            best = shape;
            bestCost = cost;
        }
    });

    return best;
}

std::vector<std::vector<int>> findGuitarShapes (const Chord& chord, int maxShapes,
                                                const std::function<bool (const std::vector<int>&)>& accept)
{
    std::vector<std::pair<double, std::vector<int>>> all;
    forEachGuitarShape (chord, nullptr, [&all, &accept] (const std::vector<int>& shape, double cost)
    {
        if (accept == nullptr || accept (shape))
            all.emplace_back (cost, shape);
    });
    std::stable_sort (all.begin(), all.end(), [] (auto& a, auto& b) { return a.first < b.first; });

    // Where a shape sits: its lowest fretted note, and the string its root is on.
    auto position = [] (const std::vector<int>& shape)
    {
        int lowestFret = 99, rootString = -1;
        for (size_t s = 0; s < shape.size(); ++s)
        {
            if (shape[s] > 0)
                lowestFret = juce::jmin (lowestFret, shape[s]);
            if (rootString < 0 && shape[s] >= 0)
                rootString = (int) s;
        }

        const bool open = std::find (shape.begin(), shape.end(), 0) != shape.end();
        return std::tuple { open, lowestFret, rootString };
    };

    // The easiest shape first, then the easiest one in each other place on the neck.
    std::vector<std::vector<int>> shapes;

    for (auto& [cost, shape] : all)
    {
        if ((int) shapes.size() >= maxShapes)
            break;

        const auto [open, fret, rootString] = position (shape);
        const bool elsewhere = std::all_of (shapes.begin(), shapes.end(), [&, o = open, f = fret, r = rootString] (auto& other)
        {
            const auto [otherOpen, otherFret, otherRoot] = position (other);
            return (o != otherOpen || r != otherRoot || std::abs (f - otherFret) >= 3) && ! (o && otherOpen);
        });

        if (elsewhere)
            shapes.push_back (shape);
    }

    return shapes;
}

std::vector<int> getShapePitches (const std::vector<int>& shape)
{
    std::vector<int> pitches;

    for (size_t s = 0; s < shape.size() && s < openStrings.size(); ++s)
        if (shape[s] >= 0)
            pitches.push_back (openStrings[s] + shape[s]);

    return pitches;
}

Voicing voice (const Style& style, const Chord& chord, const Voicing* previous)
{
    Voicing v;
    const int third = thirdOf (chord), fifth = fifthOf (chord);
    const auto pitchClasses = chord.getPitchClasses();
    const auto previousUpper = previous != nullptr ? previous->upper : std::vector<int>();

    auto setBass = [&] (int low, int high, double start)
    {
        v.bass = nearestPitch (chord.root, low, high, previous != nullptr ? previous->bass : start);
        v.third = v.bass + third;
        v.fifth = v.bass + fifth;
    };

    // The chord tones stacked right above the bass, when a range has no room for a voicing.
    auto stacked = [&]
    {
        std::vector<int> upper;
        for (int i : chord.intervals)
            if (i > 0)
                upper.push_back (v.bass + i + (i < 12 ? 12 : 0));

        std::sort (upper.begin(), upper.end());
        return upper;
    };

    if (style.powerChords)
    {
        setBass (40, 52, 45);
        v.upper = { v.bass + fifth, v.bass + 12 };
        v.wrapsOctaves = false;
        return v;
    }

    switch (style.instrument)
    {
        case Instrument::guitar:
        {
            v.shape = findGuitarShape (chord, previous != nullptr && ! previous->shape.empty() ? &previous->shape : nullptr);

            if (v.shape.empty())   // no playable shape: a close voicing in the guitar's range instead
            {
                setBass (40, 52, 45);
                v.upper = voiceChord (pitchClasses, previousUpper, 52, 76);
                break;
            }

            const auto pitches = getShapePitches (v.shape);
            v.bass = pitches.front();
            v.upper.assign (pitches.begin() + 1, pitches.end());
            v.third = findPitchClass (v.upper, (chord.root + third) % 12, v.bass + third);
            v.fifth = findPitchClass (v.upper, (chord.root + fifth) % 12, v.upper.front());   // the alternate bass string
            v.wrapsOctaves = false;
            return v;
        }

        case Instrument::piano:
            setBass (36, 50, 43);
            v.upper = voiceChord (pitchClasses, previousUpper, 52, 79);
            break;

        case Instrument::synth:
            setBass (36, 48, 42);
            v.upper = voiceChord (pitchClasses, previousUpper, 55, 81);
            break;

        case Instrument::strings:
        {
            setBass (36, 48, 42);
            v.upper = voiceChord (pitchClasses, previousUpper, 55, 79);

            // Open it up like a string section: the second note from the top drops an octave.
            if (v.upper.size() >= 3 && v.upper[v.upper.size() - 2] - 12 > v.bass)
            {
                v.upper[v.upper.size() - 2] -= 12;
                std::sort (v.upper.begin(), v.upper.end());
            }
            break;
        }

        case Instrument::bass:
            setBass (28, 40, 34);
            v.upper = stacked();
            break;
    }

    if (v.upper.empty())
        v.upper = stacked();

    return v;
}

std::vector<MidiNote> play (const Style& style, const std::vector<ChordSpan>& chordSpans, double totalBeats, double bpm)
{
    const auto spans = mergeSpans (chordSpans);

    // Every chord is voiced close to the one before it.
    std::vector<Voicing> voicings;
    for (auto& span : spans)
        voicings.push_back (voice (style, span.chord, voicings.empty() ? nullptr : &voicings.back()));

    auto spanAt = [&spans] (double beat)
    {
        for (size_t i = 0; i < spans.size(); ++i)
            if (spans[i].start <= beat + epsilon && beat < spans[i].end - epsilon)
                return (int) i;

        return -1;
    };

    auto nextSpanAfter = [&spans] (double beat)
    {
        for (size_t i = 0; i < spans.size(); ++i)
            if (spans[i].start > beat + epsilon)
                return (int) i;

        return -1;
    };

    const double beatsPerMs = bpm / 60000.0;
    std::vector<MidiNote> notes;

    for (double offset = 0.0; offset < totalBeats - epsilon; offset += style.getLengthBeats())
    {
        for (auto& e : style.events)
        {
            const double start = offset + e.start;

            if (start >= totalBeats - epsilon)
                continue;

            int used = spanAt (start);

            // A pushed chord: shortly before the next chord and ringing on into it for at least as long.
            if (const int next = nextSpanAfter (start); next >= 0)
            {
                const double early = spans[(size_t) next].start - start;

                if (early <= 0.5 + epsilon && start + e.length - spans[(size_t) next].start >= early - epsilon)
                    used = next;
            }

            if (used < 0)
                continue;

            const auto& span = spans[(size_t) used];
            const double end = juce::jmin (start + (e.role == Role::mute ? juce::jmin (e.length, 0.08) : e.length), span.end, totalBeats);
            const float velocity = e.role == Role::mute ? e.velocity * 0.45f : e.velocity;
            auto pitches = pitchesFor (e, voicings[(size_t) used], used + 1 < (int) voicings.size() ? &voicings[(size_t) used + 1] : nullptr);

            if (e.strum != Strum::none && pitches.size() > 1 && style.strumMs > 0.0)
            {
                std::sort (pitches.begin(), pitches.end());

                // An up stroke catches the higher strings only, top first.
                if (e.strum == Strum::up)
                {
                    pitches.erase (pitches.begin(), pitches.end() - (std::ptrdiff_t) juce::jmin ((size_t) 4, pitches.size()));
                    std::reverse (pitches.begin(), pitches.end());
                }
            }

            for (size_t k = 0; k < pitches.size(); ++k)
            {
                const double delay = e.strum != Strum::none ? (double) k * style.strumMs * beatsPerMs : 0.0;
                const float strumVelocity = velocity * (e.strum != Strum::none ? 1.0f - 0.04f * (float) k : 1.0f);

                if (end - (start + delay) >= shortestNote)
                    notes.push_back ({ pitches[k], start + delay, end - (start + delay), juce::jlimit (0.05f, 1.0f, strumVelocity) });
            }
        }
    }

    removeOverlaps (notes);
    return notes;
}

MidiClip* addTrack (Engine& engine, const Style& style, std::vector<MidiNote> notes, double startBeat, double lengthBeats, int index)
{
    auto* track = engine.addTrack (Track::Type::midi, index);
    track->name = (style.instrument == Instrument::piano ? juce::String ("Keys") : getInstrumentName (style.instrument)) + " - " + style.name;
    engine.setBuiltInSound (*track, style.sound);

    auto* clip = engine.addMidiClip (*track, startBeat, lengthBeats);
    {
        const juce::ScopedLock sl (engine.getLock());
        clip->name = style.name;
        clip->lengthBeats = lengthBeats;
        clip->notes = std::move (notes);
    }

    engine.notesChanged();
    return clip;
}
} // namespace daw::gen::styles
