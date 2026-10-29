#include "PromptBuilder.h"
#include "engine/Tempo.h"
#include "ChordStyles.h"

#include <numeric>

namespace daw::gen
{
namespace
{
    struct DrumInfo { const char* name; int note; };

    const DrumInfo drums[] = {
        { "kick", 36 }, { "snare", 38 }, { "clap", 39 }, { "rimshot", 37 },
        { "closed_hat", 42 }, { "pedal_hat", 44 }, { "open_hat", 46 },
        { "low_tom", 45 }, { "mid_tom", 47 }, { "high_tom", 50 },
        { "crash", 49 }, { "ride", 51 }, { "shaker", 70 }, { "tambourine", 54 }, { "cowbell", 56 }
    };

    // Hand percussion on the General MIDI drum map (the whistles are left out on purpose).
    const DrumInfo percussion[] = {
        { "conga_high", 63 }, { "conga_mute", 62 }, { "conga_low", 64 }, { "bongo_high", 60 }, { "bongo_low", 61 },
        { "shaker", 82 }, { "maracas", 70 }, { "cabasa", 69 }, { "tambourine", 54 }, { "cowbell", 56 },
        { "claves", 75 }, { "woodblock_high", 76 }, { "woodblock_low", 77 }, { "timbale_high", 65 }, { "timbale_low", 66 },
        { "agogo_high", 67 }, { "agogo_low", 68 }, { "triangle_mute", 80 }, { "triangle_open", 81 },
        { "guiro_short", 73 }, { "guiro_long", 74 }, { "cuica_mute", 78 }, { "cuica_open", 79 }, { "vibraslap", 58 }
    };

    template <size_t size>
    juce::StringArray namesOf (const DrumInfo (&list)[size])
    {
        juce::StringArray names;
        for (auto& d : list)
            names.add (d.name);

        return names;
    }

    template <size_t size>
    int noteOf (const DrumInfo (&list)[size], const juce::String& name)
    {
        for (auto& d : list)
            if (name.equalsIgnoreCase (d.name))
                return d.note;

        return -1;
    }

    juce::var object (std::initializer_list<std::pair<const char*, juce::var>> properties)
    {
        auto* o = new juce::DynamicObject();
        for (auto& [name, value] : properties)
            o->setProperty (name, value);

        return juce::var (o);
    }

    juce::var array (std::initializer_list<juce::var> items)
    {
        return juce::Array<juce::var> (items);
    }

    juce::var stringArray (const juce::StringArray& strings)
    {
        juce::Array<juce::var> items;
        for (auto& s : strings)
            items.add (s);

        return items;
    }

    juce::var listOf (juce::var itemProperties, juce::var required, int exactCount = 0)
    {
        auto item = object ({ { "type", "object" }, { "properties", itemProperties }, { "required", required } });
        auto list = object ({ { "type", "array" }, { "items", item } });

        if (exactCount > 0)
        {
            list.getDynamicObject()->setProperty ("minItems", exactCount);
            list.getDynamicObject()->setProperty ("maxItems", exactCount);
        }

        return list;
    }

    /** An object with the given lists, all required, in this order (models write them in order). */
    juce::var schemaForLists (const std::vector<std::pair<juce::String, juce::var>>& lists)
    {
        auto* properties = new juce::DynamicObject();
        juce::Array<juce::var> names;

        for (auto& [name, list] : lists)
        {
            properties->setProperty (name, list);
            names.add (name);
        }

        return object ({ { "type", "object" }, { "properties", juce::var (properties) }, { "required", names } });
    }

    juce::var degreeSchema() { return object ({ { "type", "integer" }, { "enum", array ({ 1, 2, 3, 4, 5, 6, 7 }) } }); }
    juce::var colourSchema() { return object ({ { "type", "string" }, { "enum", stringArray (getChordColours()) } }); }

    /** One chord per bar, for the bars the chord track leaves empty. */
    juce::var barChordsSchema (const Request& r)
    {
        return listOf (object ({ { "degree", degreeSchema() }, { "colour", colourSchema() } }),
                       array ({ "degree", "colour" }), r.getPatternBars());
    }

    juce::String describeColours()
    {
        return "Colours: triad, seventh, ninth, sus2, sus4, add9 keep the chord in key; borrowed_major and "
               "borrowed_minor force a major or minor triad (for example borrowed_major on degree 5 in a minor key).";
    }

    /** In-key triad on each scale degree, e.g. "1=Am, 2=Bdim, 3=C" for A minor. */
    juce::String describeDiatonicChords (const MusicalKey& key)
    {
        const auto intervals = key.getIntervals();
        juce::StringArray chords;

        for (int d = 0; d < intervals.size(); ++d)
        {
            const int third = (intervals[(d + 2) % 7] - intervals[d] + 12) % 12;
            const int fifth = (intervals[(d + 4) % 7] - intervals[d] + 12) % 12;
            const char* suffix = third == 4 ? (fifth == 8 ? "aug" : "") : (fifth == 6 ? "dim" : "m");
            chords.add (juce::String (d + 1) + "=" + MusicalKey::getTonicName (key.tonic + intervals[d]) + suffix);
        }

        return chords.joinIntoString (", ");
    }

    /** Ideas that push a take away from the most obvious answer, grouped by kind: one take gets at most one of
        each kind, so they never contradict each other. */
    using IdeaKinds = std::vector<std::vector<const char*>>;

    const IdeaKinds& getIdeaKinds (const Request& r)
    {
        static const IdeaKinds melodyIdeas {
            { "sparse: few notes and plenty of rests", "busy: runs of 8th and 16th notes",
              "long held notes joined by short quick notes" },
            { "start on an off-beat (for example beat 0.5) instead of on beat 0", "begin with a rest of one or two beats",
              "start right on beat 0 with a strong accent" },
            { "an overall rising shape", "an overall falling shape", "an arch: climb to the highest note in the middle, then come down",
              "a high register (mostly octave 1)", "a low, warm register (octave 0 and below)" },
            { "call and answer: the second half answers the first", "one short motif, repeated with a small change each time",
              "repeated notes for a rhythmic, percussive hook", "a long flowing line with few repeated notes" },
            { "mostly stepwise motion", "a few leaps of a fourth or a fifth", "an octave jump as the hook" },
            { "syncopation: notes that land just before the beat", "dotted rhythms (lengths of 0.75 and 1.5 beats)",
              "a mix of long and short notes" }
        };

        static const IdeaKinds basslineIdeas {
            { "only chord roots", "roots and fifths", "roots with passing notes that lead into the next chord",
              "octave jumps between low and high roots" },
            { "long sustained notes", "steady 8th notes", "syncopated: some notes just before the beat",
              "notes on the off-beats, between the kicks", "a rest at the end of each bar, then a pickup into the next" },
            { "stay in the sub register (octave -1)", "play higher and more melodic (octave 0 and 1)" },
            { "short, staccato notes", "legato: each note runs into the next" }
        };

        static const IdeaKinds arpeggioIdeas {
            { "go up through the chord", "go down through the chord", "go up and back down",
              "a broken pattern that skips around the chord", "alternate a low note with higher notes" },
            { "steady 16th notes", "steady 8th notes", "groups of three 16ths across the beat for a rolling feel" },
            { "keep it within one octave", "spread it over two octaves" },
            { "accent the first note of each group", "let the velocity rise over the pattern", "add the 2 or 4 of the scale as passing colour" }
        };

        static const IdeaKinds progressionIdeas {
            { "one chord per bar", "two chords in some bars", "hold one chord for two bars somewhere" },
            { "plain triads", "seventh chords", "ninth or add9 colours", "one sus2 or sus4 chord", "one borrowed chord as a surprise" },
            { "do not start on degree 1", "end on degree 5 for a turnaround", "end on degree 4 for an open feel",
              "a bass line that walks down by step" }
        };

        static const IdeaKinds chordRhythmIdeas {
            { "long sustained chords", "short stabs", "pushed chords that land an 8th before the bar",
              "a steady 8th-note pulse", "a rest on beat 0 of some bars" },
            { "accents on the off-beats", "soft, even velocity", "a strong first hit, then softer ones" }
        };

        static const IdeaKinds drumsIdeas {
            { "an extra kick just before the snare", "a syncopated kick pattern", "a simple, sparse kick" },
            { "open hat on the off-beats", "16th-note hi-hats with accents", "8th-note hi-hats", "the ride instead of the hi-hats",
              "a shaker next to the hats" },
            { "ghost snares between the backbeats", "rimshot instead of snare", "a clap on top of the snare",
              "a half-time feel: snare only on beat 2 of each bar" },
            { "a short fill on the last beat of the pattern", "a crash on beat 0", "toms in the fill at the end" },
            { "minimal and spacious", "busy and driving" }
        };

        static const IdeaKinds percussionIdeas {
            { "congas lead the pattern", "bongos lead the pattern", "bells lead (agogo or cowbell)",
              "woods lead (claves, woodblocks)", "timbales lead the pattern" },
            { "no shaker this time", "shaker only on the off-beat 8ths", "a shaker with strong accents and soft ghost notes" },
            { "sparse and spacious", "busy and rolling" },
            { "a clave-like figure of five hits over two bars", "an open-and-muted conversation between two drums",
              "a tambourine or triangle accent on the off-beats" }
        };

        static const IdeaKinds styleIdeas {
            { "stay close to the example, with one small change per bar", "change the rhythm of the second bar",
              "make it busier: add notes between the existing ones", "make it sparser: leave out a few hits" },
            { "a pickup or short fill on the last beat of the pattern", "push one chord an 8th before the bar",
              "leave beat 0 of the second bar empty", "an accent on an off-beat" },
            { "play the chord tones in a different order", "add the top note an octave higher on the accents",
              "let some notes ring longer", "shorter, more staccato notes" }
        };

        if (r.usesPlayingStyle())
            return r.varyStyle ? styleIdeas : progressionIdeas;   // played as it is, the model only writes chords

        switch (r.part)
        {
            case Part::chords:     return r.isChordRhythm() ? chordRhythmIdeas : progressionIdeas;
            case Part::melody:     return melodyIdeas;
            case Part::bassline:   return basslineIdeas;
            case Part::arpeggio:   return arpeggioIdeas;
            case Part::drums:      return drumsIdeas;
            case Part::percussion: return percussionIdeas;
        }

        return melodyIdeas;
    }

    /** What each role plays, for the roles the instrument plays. */
    juce::String describeRoles (styles::Instrument instrument, bool guitar)
    {
        using styles::Role;
        const std::pair<Role, juce::String> roles[] {
            { Role::bass, "\"bass\" the root, low" },
            { Role::third, "\"third\" the third above the bass" },
            { Role::fifth, "\"fifth\" the fifth above the bass" },
            { Role::octave, "\"octave\" the bass an octave up" },
            { Role::tone, juce::String ("\"tone\" with \"index\" one chord tone counted up from the lowest note above the bass (0, 1, 2 ...; ")
                              + (guitar ? "higher indexes stay on the top string)" : "past the top of the chord it goes on an octave higher)") },
            { Role::top, "\"top\" the highest note" },
            { Role::upper, "\"upper\" the chord without the bass" },
            { Role::all, "\"all\" the whole chord" },
            { Role::approach, "\"approach\" a half step into the next chord's bass (walking bass)" },
            { Role::mute, "\"mute\" a short, muted strum" }
        };

        juce::StringArray described;
        for (auto& [role, text] : roles)
            if (styles::playsRole (instrument, role))
                described.add (text);

        return described.joinIntoString ("; ");
    }

    juce::String describePart (const Request& r)
    {
        const int patternBars = r.getPatternBars();
        const juce::String beats (patternBars * 4);

        if (r.usesPlayingStyle())
        {
            const auto* style = styles::findStyle (r.playingStyle);

            if (style == nullptr)
                return {};

            if (! r.varyStyle)
                return "The part is played in the style \"" + style->name + "\" (" + style->description + ") over the song's "
                       "chords. You only choose the chords for the bars that have none yet.";

            const int bars = r.getStyleBars();
            const bool guitar = style->instrument == styles::Instrument::guitar && ! style->powerChords;
            juce::String text;
            text << "Play the chords in the style \"" << style->name << "\" (" << styles::getInstrumentName (style->instrument).toLowerCase()
                 << "): " << style->description << "\n"
                 << "Write the playing pattern for " << bars << " bar" << (bars == 1 ? "" : "s") << " (" << bars * 4 << " beats) as \"events\"; "
                 "it is looped over the song's chords. You don't write pitches: each event names a part of the chord, and the notes come "
                 "from the chord sounding at that moment, voiced for the instrument" << (guitar ? " as real guitar chord shapes" : "") << ".\n"
                 "Roles: " << describeRoles (style->instrument, guitar) << ".\n"
                 "Each event: \"start\" in beats from 0, \"length\" in beats, \"role\", \"index\" (0 unless the role is tone), \"octave\" "
                 "(-1, 0 or 1 moves the notes), \"velocity\" 1-127, \"strum\" "
                 << (style->instrument == styles::Instrument::bass
                         ? "(always \"none\": a bass plays one note at a time)"
                         : "(\"down\" or \"up\" spreads the notes like a strummed guitar, up strokes catching only the high strings; "
                           "\"none\" plays them together)")
                 << ".\n"
                 "The style's own pattern (" << style->bars << " bar" << (style->bars == 1 ? "" : "s") << "):\n"
                 << styles::describeEvents (style->events) << "\n"
                 "Write a variation of it: keep its feel, instrument and tempo feel, but change some rhythms, chord tones or accents"
                 << (bars > 1 ? ", and let the second bar answer the first." : ".");
            return text;
        }

        if (r.isChordRhythm())
        {
            const juce::String rhythmBars (r.getRhythmBars());
            return "Write the rhythm of a chord part for " + rhythmBars + " bar" + (r.getRhythmBars() == 1 ? "" : "s")
                   + " (" + juce::String (r.getRhythmBars() * 4) + " beats); it will be looped. The chords themselves come "
                   "from the chord track below: you only decide when they are struck and how long they ring.\n"
                   "Each hit: \"start\" in beats from 0 (multiples of 0.25), \"length\" in beats, \"velocity\" 1-127. "
                   "Use the chord rhythm typical for the style (for example sustained pads, off-beat stabs in house, "
                   "syncopated pushes in pop, offbeat skanks in reggae).";
        }

        switch (r.part)
        {
            case Part::chords:
                return "Write a chord progression of " + juce::String (patternBars) + " bars (" + beats + " beats).\n"
                       "Each entry is one chord: \"degree\" is the chord root as a scale degree (1-7), \"colour\" "
                       "says how to build it, \"beats\" is how long it lasts. The beats must add up to exactly " + beats + ". "
                       "Usually one or two chords per bar.\n"
                       "Chords are built from the key automatically; with colour \"triad\" the degrees give: "
                       + describeDiatonicChords (r.key) + ".\n"
                       + describeColours() + " Pick progressions and colours that are idiomatic for the style.";

            case Part::melody:
                return "Write the main melody (lead / hook) for " + juce::String (patternBars) + " bars (" + beats + " beats).\n"
                       "Each note: \"start\" in beats from 0 (use values like 0, 0.5, 1.75), \"length\" in beats, "
                       "\"degree\" as a scale degree 1-7, \"octave\" (0 = middle register, 1 = an octave up, -1 = down), "
                       "\"velocity\" 1-127. One note at a time (no overlaps), leave some rests, repeat and vary a "
                       "short motif so it is memorable, and end phrases on stable degrees (1, 3, 5).";

            case Part::bassline:
                return "Write a bassline for " + juce::String (patternBars) + " bars (" + beats + " beats).\n"
                       "Each note: \"start\" in beats from 0, \"length\" in beats, \"degree\" as a scale degree 1-7, "
                       "\"octave\" (0 = normal bass register, -1 = sub, 1 = higher), \"velocity\" 1-127. "
                       "One note at a time. Lock to the chord roots and use the bass rhythm typical for the style "
                       "(for example off-beat 8ths in house and trance, rolling 16ths in techno, sustained notes in ambient).";

            case Part::arpeggio:
                return "Write an arpeggio pattern for " + juce::String (patternBars) + " bars (" + beats + " beats).\n"
                       "Each note: \"start\" in beats from 0, \"length\" in beats, \"degree\" as a scale degree 1-7, "
                       "\"octave\" (0 = middle register, 1 = up, -1 = down), \"velocity\" 1-127. "
                       "Mostly steady 16th or 8th notes that outline the chords, with a repeating shape.";

            case Part::drums:
                return "Write a drum groove of " + juce::String (patternBars) + " bars (" + beats + " beats); it will be looped.\n"
                       "Each hit: \"start\" in beats from 0 on a 16th-note grid (multiples of 0.25), \"drum\" (one of the "
                       "allowed names), \"velocity\" 1-127. Follow the conventions of the style (for example "
                       "four-on-the-floor kicks in house and EDM, half-time snares and fast hi-hat rolls in trap, "
                       "breakbeats in drum & bass). Use velocity for groove (accents and ghost notes)"
                       + juce::String (patternBars > 1 ? ", and make the last bar a slight variation." : ".");

            case Part::percussion:
                return "Write a percussion part of " + juce::String (patternBars) + " bars (" + beats + " beats); it will be "
                       "looped and layered over a drum kit, so leave kick, snare and hi-hats to the drums.\n"
                       "Each hit: \"start\" in beats from 0 on a 16th-note grid (multiples of 0.25), \"instrument\" (one of "
                       "the allowed names), \"velocity\" 1-127. Follow the conventions of the style (for example shaker "
                       "16ths and conga patterns in house, tambourine on 2 and 4 in pop and soul, clave and bongo patterns "
                       "in latin and reggaeton, layered congas, shakers and bells in afrobeats). Use two to four instruments, "
                       "answer high hits with low or open hits with muted ones, leave gaps (no instrument on every 16th), "
                       "and use velocity for accents and ghost notes"
                       + juce::String (patternBars > 1 ? ", and make the last bar a slight variation." : ".");
        }

        return {};
    }
}

juce::StringArray getDrumNames()                          { return namesOf (drums); }
int getDrumNote (const juce::String& name)                { return noteOf (drums, name); }
juce::StringArray getPercussionNames()                    { return namesOf (percussion); }
int getPercussionNote (const juce::String& name)          { return noteOf (percussion, name); }

juce::StringArray getChordColours()
{
    return { "triad", "seventh", "ninth", "sus2", "sus4", "add9", "borrowed_major", "borrowed_minor" };
}

juce::StringArray pickDirections (const Request& r, int count, juce::Random& random)
{
    const auto& kinds = getIdeaKinds (r);
    std::vector<size_t> order (kinds.size());
    std::iota (order.begin(), order.end(), size_t (0));

    for (size_t i = order.size(); i > 1; --i)
        std::swap (order[i - 1], order[(size_t) random.nextInt ((int) i)]);

    juce::StringArray ideas;

    for (size_t i = 0; i < juce::jmin (order.size(), (size_t) juce::jmax (0, count)); ++i)
    {
        const auto& ideasOfKind = kinds[order[i]];
        ideas.add (ideasOfKind[(size_t) random.nextInt ((int) ideasOfKind.size())]);
    }

    return ideas;
}

juce::String buildSystemPrompt()
{
    return "You are an expert music producer and composer. You write MIDI parts as JSON that follows the given "
           "schema exactly. Music is in 4/4: one bar is 4 beats, and times are in beats counted from 0 at the start "
           "of the pattern. Pitches are given as scale degrees of the song's key, so everything stays in key. "
           "Write parts that sound idiomatic for the requested style.";
}

juce::String buildUserPrompt (const Request& r)
{
    const auto intervals = r.key.getIntervals();
    juce::StringArray degrees;

    for (int i = 0; i < intervals.size(); ++i)
        degrees.add (juce::String (i + 1) + "=" + MusicalKey::getTonicName (r.key.tonic + intervals[i]));

    juce::String prompt;
    prompt << "Style: " << r.style << "\n"
           << "Tempo: " << formatBpm (r.bpm) << " BPM, 4/4\n"
           << "Key: " << r.key.getName() << " (scale degrees: " << degrees.joinIntoString (", ") << ")\n\n"
           << describePart (r) << "\n";

    if (r.part == Part::drums)
        prompt << "Allowed drum names: " << getDrumNames().joinIntoString (", ") << "\n";
    else if (r.part == Part::percussion)
        prompt << "Allowed instrument names: " << getPercussionNames().joinIntoString (", ") << "\n";

    // Existing harmony from other tracks helps the melody, bass and arpeggio fit what is already there.
    bool anyHarmony = false;
    for (auto& bar : r.harmonyPerBar)
        anyHarmony = anyHarmony || bar.isNotEmpty();

    if (anyHarmony && ! isPercussive (r.part))
    {
        prompt << "\nNotes already playing on other tracks, per bar (fit your part to them):\n";

        for (int i = 0; i < juce::jmin (r.harmonyPerBar.size(), r.getPatternBars()); ++i)
            prompt << "  bar " << (i + 1) << ": " << (r.harmonyPerBar[i].isEmpty() ? "-" : r.harmonyPerBar[i]) << "\n";
    }

    // The chord track is the song's harmony: the part follows it, and fills its empty bars.
    if (r.hasChordTrack() && ! isPercussive (r.part))
    {
        prompt << "\nChord track (the song's chords; your part must follow them):\n";

        for (int bar = 0; bar < r.getPatternBars(); ++bar)
        {
            const auto chord = r.getTrackChord (bar);
            prompt << "  bar " << (bar + 1) << ": "
                   << (chord.has_value() ? chord->getName() + " (" + chord->getNoteNames() + ")" : juce::String ("no chord yet"))
                   << "\n";
        }

        if (r.part != Part::chords && ! r.usesPlayingStyle())
            prompt << "Favour chord tones on strong beats. Pitches are scale degrees, so for a chord note outside the key "
                      "use the nearest degree.\n";
    }

    if (r.asksForBarChords())
    {
        prompt << "\nAlso write \"chords\": the chord of each bar, exactly " << r.getPatternBars() << " entries, each a "
               << "chord root as a scale degree (1-7) and a colour. With colour \"triad\" the degrees give: "
               << describeDiatonicChords (r.key) << ". " << describeColours() << " "
               << (r.hasChordTrack() ? "For bars that have a chord on the chord track, repeat that chord; choose the others so they fit."
                                     : "Write the progression first, then a part that fits it.")
               << " These chords are added to the song's chord track.\n";
    }

    if (! r.directions.isEmpty())
    {
        prompt << "\nIdeas for this take, so it doesn't sound like every other " << getPartShortName (r.part)
               << " part (still keep the style; the producer's direction below wins if they clash):\n";

        for (auto& idea : r.directions)
            prompt << "- " << idea << "\n";
    }

    if (r.extraPrompt.trim().isNotEmpty())
        prompt << "\nExtra direction from the producer: " << r.extraPrompt.trim() << "\n";

    return prompt;
}

juce::var buildSchema (const Request& r)
{
    const auto number = object ({ { "type", "number" } });
    const auto integer = object ({ { "type", "integer" } });
    std::vector<std::pair<juce::String, juce::var>> lists;

    // The chords come first, so the model settles the harmony before writing over it.
    if (r.asksForBarChords())
        lists.push_back ({ "chords", barChordsSchema (r) });

    if (r.usesPlayingStyle())
    {
        // Only the roles the style's instrument plays.
        juce::StringArray roles;
        const auto* style = styles::findStyle (r.playingStyle);

        for (auto& id : styles::getRoleIds())
            if (style == nullptr || styles::playsRole (style->instrument, *styles::roleFromId (id)))
                roles.add (id);

        if (r.varyStyle)
            lists.push_back ({ "events", listOf (object ({ { "start", number },
                                                           { "length", number },
                                                           { "role", object ({ { "type", "string" }, { "enum", stringArray (roles) } }) },
                                                           { "index", object ({ { "type", "integer" }, { "enum", array ({ 0, 1, 2, 3, 4, 5, 6, 7 }) } }) },
                                                           { "octave", object ({ { "type", "integer" }, { "enum", array ({ -1, 0, 1 }) } }) },
                                                           { "velocity", integer },
                                                           { "strum", object ({ { "type", "string" }, { "enum", array ({ "none", "down", "up" }) } }) } }),
                                                 array ({ "start", "length", "role", "index", "octave", "velocity", "strum" })) });

        return schemaForLists (lists);
    }

    if (r.isChordRhythm())
    {
        lists.push_back ({ "hits", listOf (object ({ { "start", number }, { "length", number }, { "velocity", integer } }),
                                           array ({ "start", "length", "velocity" })) });
        return schemaForLists (lists);
    }

    switch (r.part)
    {
        case Part::chords:
            return schemaForLists ({ { "chords", listOf (object ({ { "degree", degreeSchema() },
                                                                   { "colour", colourSchema() },
                                                                   { "beats", number } }),
                                                         array ({ "degree", "colour", "beats" })) } });

        case Part::drums:
            return schemaForLists ({ { "hits", listOf (object ({ { "start", number },
                                                                 { "drum", object ({ { "type", "string" }, { "enum", stringArray (getDrumNames()) } }) },
                                                                 { "velocity", integer } }),
                                                       array ({ "start", "drum", "velocity" })) } });

        case Part::percussion:
            return schemaForLists ({ { "hits", listOf (object ({ { "start", number },
                                                                 { "instrument", object ({ { "type", "string" }, { "enum", stringArray (getPercussionNames()) } }) },
                                                                 { "velocity", integer } }),
                                                       array ({ "start", "instrument", "velocity" })) } });

        case Part::melody:
        case Part::bassline:
        case Part::arpeggio:
            break;
    }

    lists.push_back ({ "notes", listOf (object ({ { "start", number },
                                                  { "length", number },
                                                  { "degree", degreeSchema() },
                                                  { "octave", object ({ { "type", "integer" }, { "enum", array ({ -1, 0, 1, 2 }) } }) },
                                                  { "velocity", integer } }),
                                        array ({ "start", "length", "degree", "octave", "velocity" })) });
    return schemaForLists (lists);
}
} // namespace daw::gen
