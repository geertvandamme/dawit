#include "ChordGuide.h"

namespace daw::gen::guide
{
namespace
{
    juce::String arrow() { return juce::String::fromUTF8 (" \xe2\x86\x92 "); }
    juce::String dash()  { return juce::String::fromUTF8 (" \xe2\x80\x93 "); }
    juce::String bullet() { return juce::String::fromUTF8 ("\xe2\x80\xa2 "); }

    bool has (const Chord& c, int semitones) { return c.intervals.contains (semitones); }
    bool isMinor (const Chord& c)            { return has (c, 3); }
    bool isDiminished (const Chord& c)       { return has (c, 3) && has (c, 6) && ! has (c, 7); }

    /** Same root and the same basic sound (major, minor or diminished), ignoring sevenths and other colours. */
    bool sameCore (const Chord& a, const Chord& b)
    {
        return a.root == b.root && isMinor (a) == isMinor (b) && isDiminished (a) == isDiminished (b);
    }

    juce::String describeSound (const Chord& c)
    {
        juce::String s;

        if (! has (c, 3) && ! has (c, 4))
            s = "It's a suspended chord: neither major nor minor, open and waiting to move.";
        else if (isDiminished (c))
            s = "It's a diminished chord: tense and unstable, it wants to move on.";
        else if (has (c, 4) && has (c, 8) && ! has (c, 7))
            s = "It's an augmented chord: dreamy and unresolved.";
        else if (isMinor (c))
            s = "It's a minor chord: it sounds darker and sadder than a major chord.";
        else
            s = "It's a major chord: it sounds bright and stable.";

        if (has (c, 10) || has (c, 11))
            s << " The extra note on top (a seventh) adds colour: smoother and jazzier.";
        else if (has (c, 14))
            s << " The added ninth makes it shimmer.";

        return s;
    }

    /** What a degree does in the key, without naming the chord's quality (modes change it). */
    juce::String describeRole (const MusicalKey& key, int degree, const Chord& chord)
    {
        if (! harmony::isMinorKey (key))
        {
            switch (degree)
            {
                case 1: return "This is home: the chord the key is named after. Songs often start and end here, and it feels at rest.";
                case 2: return "A gentle step away from home. It loves to move to the five chord, which then leads home: the famous "
                               "ii-V-I of jazz and pop.";
                case 3: return "A soft cousin of home: it shares two notes with the home chord. It often moves on to the six or four chord.";
                case 4: return "The classic \"away\" chord: it lifts the music off home. Going from four back to one sounds like the "
                               "\"amen\" at the end of a hymn.";
                case 5: return "The tension chord: it wants to go home, and five to one is the strongest move in music. Ending a "
                               "phrase here leaves the listener waiting for more.";
                case 6: return "Home's sad twin, called the relative minor. It shares two notes with the home chord, so it can stand "
                               "in for home with a darker feel.";
                case 7: return "The most unstable chord of the key. It usually moves straight home. Use it as a quick step.";
                default: break;
            }

            return {};
        }

        switch (degree)
        {
            case 1: return "This is home: the chord the key is named after. Songs in this key often start and end here.";
            case 2: return "An uneasy chord. It leads well to the five chord, which then leads home.";
            case 3: return "The relative major: it shares two notes with home but sounds brighter. Moving here feels like the sun "
                           "coming out.";
            case 4: return "The \"away\" chord of a minor key: moody and soulful. Going from four back to one is a gentle, sad way home.";
            case 5: return isMinor (chord)
                               ? "The tension chord. As a minor chord its pull home is soft; many songs make it major (borrowing one "
                                 "note from outside the key) to pull home harder."
                               : "The tension chord. Made major, it pulls home strongly: this is how most minor songs lead back home.";
            case 6: return "A big, warm chord. It starts many epic and emotional progressions.";
            case 7: return "A chord one step below home. It sounds bold, and leads back home or to the three chord.";
            default: break;
        }

        return {};
    }

    /** Names of the chord's notes that are outside the key. */
    juce::StringArray notesOutsideKey (const MusicalKey& key, const Chord& chord)
    {
        const auto names = juce::StringArray::fromTokens (chord.getNoteNames(), " ", "");
        juce::StringArray outside;

        for (int i = 0; i < chord.intervals.size() && i < names.size(); ++i)
            if (! key.contains (chord.root + chord.intervals[i]))
                outside.addIfNotAlreadyThere (names[i]);

        return outside;
    }

    juce::String nameWithNumeral (const MusicalKey& key, const Chord& chord)
    {
        return chord.getName() + " (" + getNumeral (key, chord) + ")";
    }

    juce::String joinNames (const std::vector<Chord>& chords)
    {
        juce::StringArray names;
        for (auto& c : chords)
            names.add (c.getName());

        return names.joinIntoString (dash());
    }

    juce::String joinNumerals (const MusicalKey& key, const std::vector<Chord>& chords)
    {
        juce::StringArray numerals;
        for (auto& c : chords)
            numerals.add (getNumeral (key, c));

        return numerals.joinIntoString (dash());
    }

    /** The chords with each run of the same chord merged into one. */
    std::vector<Chord> mergeRepeats (const std::vector<Chord>& chords)
    {
        std::vector<Chord> merged;

        for (auto& c : chords)
            if (merged.empty() || ! (merged.back() == c))
                merged.push_back (c);

        return merged;
    }

    juce::String stepByStep (const MusicalKey& key, const std::vector<Chord>& chords, bool loops)
    {
        juce::String s;

        for (size_t i = 0; i + 1 < chords.size(); ++i)
            s << bullet() << explainMove (key, chords[i], chords[i + 1]) << "\n";

        if (loops && chords.size() > 1 && ! (chords.back() == chords.front()))
            s << bullet() << "When it repeats: " << explainMove (key, chords.back(), chords.front()) << "\n";

        return s;
    }
}

juce::String getFamilyName (Family family)
{
    switch (family)
    {
        case Family::home:    return "Home";
        case Family::away:    return "Away";
        case Family::tension: return "Tension";
        case Family::outside: return "Outside the key";
    }

    return {};
}

Family getFamily (int degree)
{
    switch (degree)
    {
        case 1: case 3: case 6: return Family::home;
        case 2: case 4:         return Family::away;
        case 5: case 7:         return Family::tension;
        default:                return Family::outside;
    }
}

Family getFamily (const MusicalKey& key, const Chord& chord)
{
    return getFamily (harmony::findDegree (key, chord));
}

juce::String getNumeral (const MusicalKey& key, const Chord& chord)
{
    juce::String numeral;

    if (const int degree = harmony::findDegree (key, chord); degree > 0)
    {
        numeral = harmony::getRomanNumeral (degree, chord);
    }
    else
    {
        // A root between two scale notes is named after the one above (flat) or below (sharp), e.g. bVII.
        const auto intervals = key.getIntervals();
        const int offset = ((chord.root - key.tonic) % 12 + 12) % 12;

        if (const int above = intervals.indexOf ((offset + 1) % 12); above >= 0)
            numeral = "b" + harmony::getRomanNumeral (above + 1, chord);
        else
            numeral = "#" + harmony::getRomanNumeral (juce::jmax (1, intervals.indexOf ((offset + 11) % 12) + 1), chord);
    }

    if (has (chord, 11) && ! isMinor (chord))
        numeral << "maj7";
    else if (has (chord, 10) || (has (chord, 9) && isDiminished (chord)))
        numeral << "7";

    return numeral;
}

Chord buildChord (const MusicalKey& key, const Step& step)
{
    return { harmony::triad (key, step.degree).root, getChordIntervals (key, step.degree, step.colour) };
}

std::vector<Chord> Progression::getChords (const MusicalKey& key) const
{
    std::vector<Chord> chords;
    for (auto& s : steps)
        chords.push_back (buildChord (key, s));

    return chords;
}

std::vector<Progression> getCommonProgressions (const MusicalKey& key)
{
    if (! harmony::isMinorKey (key))
    {
        return {
            { "Pop progression", "The most used progression in pop music.",
              "It starts at home, climbs to the tension chord, takes a sad turn to the relative minor, then moves away. "
              "The away chord leads so naturally back home that the loop keeps rolling. Countless hits use exactly these four chords.",
              { { 1 }, { 5 }, { 6 }, { 4 } } },
            { "Sad pop", "The pop progression, starting on the minor chord.",
              "The same four chords as the pop progression, but starting on the relative minor makes the loop feel more emotional. "
              "It ends on the tension chord, so it pulls back to the start.",
              { { 6 }, { 4 }, { 1 }, { 5 } } },
            { "Fifties progression", "The sound of 1950s doo-wop and many ballads.",
              "Home, its sad twin, away, then tension. The tension at the end leads straight back home when it repeats, "
              "which gives it a warm, rocking feel.",
              { { 1 }, { 6 }, { 4 }, { 5 } } },
            { "Three-chord song", "The backbone of folk, country, punk and rock.",
              "With just home, away and tension you can play thousands of songs. The five-to-one move at the end closes the phrase firmly.",
              { { 1 }, { 4 }, { 5 }, { 1 } } },
            { "Jazz turnaround (ii-V-I)", "The most important move in jazz.",
              "The two chord steps away, the five chord builds tension, and the home chord resolves it. "
              "Jazz players use seventh chords for a smooth, rich sound.",
              { { 2, "seventh" }, { 5, "seventh" }, { 1, "seventh" }, { 1, "seventh" } } },
            { "Canon", "The pattern of Pachelbel's Canon, borrowed by many pop songs.",
              "Eight chords whose bass walks down step by step, which makes it sound graceful and grand. "
              "It visits almost every chord of the key before turning back home.",
              { { 1 }, { 5 }, { 6 }, { 3 }, { 4 }, { 1 }, { 4 }, { 5 } } },
            { "Twelve-bar blues", "Twelve bars and three chords: the form of the blues and early rock and roll.",
              "Four bars at home, two away, two at home, then tension, away, home, and the tension chord again to turn around. "
              "Blues players often make every chord a seventh chord for a gritty sound.",
              { { 1 }, { 1 }, { 1 }, { 1 }, { 4 }, { 4 }, { 1 }, { 1 }, { 5 }, { 4 }, { 1 }, { 5 } } }
        };
    }

    return {
        { "Epic minor", "Big, emotional and cinematic: film scores, anthems and EDM.",
          "From home it falls to the warm six chord, rises to the bright relative major, and the bold seven chord leads back home. "
          "Three of the four chords are major, so it sounds hopeful even though the key is minor.",
          { { 1 }, { 6 }, { 3 }, { 7 } } },
        { "Rising to home", "Two major chords climb to the minor home chord.",
          "The six and seven chords climb step by step, and landing on the minor home chord sounds powerful and heroic. "
          "You hear it in rock and film music.",
          { { 6 }, { 7 }, { 1 }, { 1 } } },
        { "Andalusian cadence", "Steps down from home: Spanish and dramatic.",
          "The bass walks down from home, one step at a time, to a major five chord. That last chord borrows one note from "
          "outside the key, which gives it a strong pull back home. Heard in flamenco and in rock.",
          { { 1 }, { 7 }, { 6 }, { 5, "borrowed_major" } } },
        { "Minor three-chord song", "Home, away, tension, home: the minor version of the three-chord song.",
          "The four chord gives a moody step away, and a major five chord (borrowing one note from outside the key) "
          "pulls firmly back home.",
          { { 1 }, { 4 }, { 5, "borrowed_major" }, { 1 } } },
        { "Minor ii-V-i", "The jazz turnaround in a minor key.",
          "The uneasy two chord leads to a major five chord, which resolves home. Darker and more dramatic than the major version.",
          { { 2 }, { 5, "borrowed_major" }, { 1 }, { 1 } } },
        { "Minor loop", "A moody loop for pop, hip-hop and lo-fi.",
          "Home, then away to the four chord, a warm lift to the six chord, and the seven chord leads back home. "
          "It never uses a strong tension chord, so it floats calmly.",
          { { 1 }, { 4 }, { 6 }, { 7 } } }
    };
}

juce::String getIntroduction (const MusicalKey& key)
{
    std::map<Family, juce::StringArray> families;
    juce::StringArray all;

    for (int degree = 1; degree <= 7; ++degree)
    {
        const auto chord = harmony::triad (key, degree);
        all.add (nameWithNumeral (key, chord));
        families[getFamily (degree)].add (nameWithNumeral (key, chord));
    }

    juce::String s;
    s << "Every key has seven chords, one built on each note of its scale. They are numbered with Roman numerals: "
      << "I is built on the first note, V on the fifth. Upper case means a major chord (bright), lower case a minor chord "
      << "(darker), and a small circle a diminished chord (tense).\n\n"
      << "The chords of " << key.getName() << " are: " << all.joinIntoString (", ") << ".\n\n"
      << "Each chord belongs to a family:\n"
      << bullet() << "Home: " << families[Family::home].joinIntoString (", ") << ". Restful: songs usually start and end on "
      << harmony::triad (key, 1).getName() << ".\n"
      << bullet() << "Away: " << families[Family::away].joinIntoString (", ") << ". They move off home and start a journey.\n"
      << bullet() << "Tension: " << families[Family::tension].joinIntoString (", ") << ". They want to go home.\n\n"
      << "Most progressions travel home" << arrow() << "away" << arrow() << "tension" << arrow() << "home. "
      << "Click a chord above to hear it and read about it, or pick a progression on the left.";
    return s;
}

juce::String explainChord (const MusicalKey& key, const Chord& chord)
{
    const int degree = harmony::findDegree (key, chord);
    const auto outside = notesOutsideKey (key, chord);
    juce::String s;

    if (degree > 0)
        s << chord.getName() << " is the " << getNumeral (key, chord) << " chord in " << key.getName() << ".\n\n";
    else
        s << chord.getName() << " (" << getNumeral (key, chord) << ") is not one of the chords of " << key.getName() << ".\n\n";

    s << "Notes: " << chord.getNoteNames() << ". " << describeSound (chord) << "\n\n";

    if (degree == 0)
        s << "Its root note isn't in the key, so it brings in colour from outside. Chords like this are called borrowed chords: "
          << "used now and then they add surprise, and they usually move back to a chord of the key.\n\n";
    else if (! outside.isEmpty())
        s << (outside.size() == 1 ? "The note " + outside[0] + " is" : "The notes " + outside.joinIntoString (" and ") + " are")
          << " not in the key: it's borrowed from outside, which adds colour.\n\n";

    s << "Family: " << getFamilyName (getFamily (degree)) << ".";

    if (degree > 0)
    {
        s << " " << describeRole (key, degree, chord);

        juce::StringArray next;
        for (int d : harmony::getUsualNextDegrees (key, degree))
            next.add (nameWithNumeral (key, harmony::triad (key, d)));

        if (! next.isEmpty())
            s << "\n\nOften goes to: " << next.joinIntoString (", ") << ".";
    }

    return s;
}

juce::String explainMove (const MusicalKey& key, const Chord& from, const Chord& to)
{
    const auto prefix = nameWithNumeral (key, from) + arrow() + nameWithNumeral (key, to) + ": ";

    if (from == to)
        return prefix + "stays on the same chord, which builds expectation.";

    const int a = harmony::findDegree (key, from), b = harmony::findDegree (key, to);
    const auto fa = getFamily (a), fb = getFamily (b);
    const int rootStep = ((to.root - from.root) % 12 + 12) % 12;

    if (fb == Family::outside)
        return prefix + "steps outside the key for a splash of colour.";

    if (fa == Family::outside)
        return prefix + "comes back into the key after the outside chord.";

    if (b == 1)
    {
        if (a == 5) return prefix + "tension resolves home. This is the strongest move in music, and it sounds like an ending.";
        if (a == 7) return prefix + (isDiminished (from) ? "the unstable chord slides home."
                                                         : "the chord one step below home climbs up to it: a bold, proud arrival.");
        if (a == 4) return prefix + "a soft way home, like the \"amen\" at the end of a hymn.";
        return prefix + "back home: a moment of rest.";
    }

    if (a == 5 && b == 6)
        return prefix + "a surprise: the ear expects home but gets home's twin instead. This is called a deceptive move.";

    if (fb == Family::tension)
    {
        if (fa == Family::away)    return prefix + "from away to tension: the classic build-up before going home.";
        if (fa == Family::tension) return prefix + "keeps the tension going, so the return home will feel even better.";
        return prefix + "goes straight to the tension side, which sets up the return home.";
    }

    if (fa == Family::home && fb == Family::away)
        return prefix + "the music leaves home and sets off on a journey.";

    if (fa == Family::home && fb == Family::home)
        return prefix + (isMinor (from) == isMinor (to) ? juce::String ("stays in the home family: the music moves but keeps its restful feel.")
                                                        : "stays in the home family but changes colour, turning " + juce::String (isMinor (to) ? "darker." : "brighter."));

    if (fa == Family::tension && fb == Family::away)
        return prefix + "the tension eases without going home yet, which keeps the music moving.";

    if (fb == Family::home)
        return prefix + "returns to the home family: a moment of rest.";

    if (rootStep == 5)
        return prefix + "the root falls by a fifth, the most natural way for chords to move.";

    if (rootStep == 1 || rootStep == 2)
        return prefix + "the bass steps up, which sounds like climbing.";

    if (rootStep == 10 || rootStep == 11)
        return prefix + "the bass steps down, which sounds smooth and calm.";

    return prefix + "moves from " + getFamilyName (fa).toLowerCase() + " to " + getFamilyName (fb).toLowerCase() + ".";
}

juce::String explainProgression (const MusicalKey& key, const Progression& progression)
{
    const auto chords = progression.getChords (key);
    juce::String s;
    s << progression.summary << "\n\n" << progression.story << "\n\n"
      << "In " << key.getName() << ": " << joinNames (chords) << "\n"
      << "Numerals: " << joinNumerals (key, chords) << "\n\n"
      << "Step by step:\n" << stepByStep (key, mergeRepeats (chords), true);
    return s.trimEnd();
}

juce::String findProgressionName (const MusicalKey& key, const std::vector<Chord>& chords)
{
    // Loops are compared as cycles, so a chord at both ends (I IV V I) counts once.
    auto asLoop = [] (const std::vector<Chord>& list)
    {
        auto merged = mergeRepeats (list);

        if (merged.size() > 1 && sameCore (merged.front(), merged.back()))
            merged.pop_back();

        return merged;
    };

    const auto played = asLoop (chords);
    const auto progressions = getCommonProgressions (key);

    // Exact matches first: Am F C G in C major is "Sad pop", not the pop progression started elsewhere.
    for (const bool rotated : { false, true })
    {
        for (auto& progression : progressions)
        {
            const auto known = asLoop (progression.getChords (key));

            if (known.size() != played.size() || known.empty())
                continue;

            for (size_t start = rotated ? 1 : 0; start < (rotated ? known.size() : 1); ++start)
            {
                bool same = true;

                for (size_t i = 0; same && i < known.size(); ++i)
                    same = sameCore (played[i], known[(start + i) % known.size()]);

                if (same)
                    return rotated ? progression.name + " (starting on another of its chords)" : progression.name;
            }
        }
    }

    return {};
}

juce::String explainChordTrack (const MusicalKey& key, const std::vector<Chord>& chordTrack)
{
    const auto chords = mergeRepeats (chordTrack);

    if (chords.empty())
        return "Your chord track is empty. Click a bar in the Chords row to set a chord, or put one of the progressions on the "
               "left on it with \"Put on chord track\".";

    juce::String s;
    s << "Your chord track: " << joinNames (chords) << "\n"
      << "In " << key.getName() << " that reads as: " << joinNumerals (key, chords) << "\n\n";

    if (const auto name = findProgressionName (key, chords); name.isNotEmpty())
        s << "That's the " << name << ".\n\n";

    juce::StringArray outside;
    bool visitsHome = false;

    for (auto& c : chords)
    {
        if (getFamily (key, c) == Family::outside || ! notesOutsideKey (key, c).isEmpty())
            outside.addIfNotAlreadyThere (c.getName());

        visitsHome = visitsHome || harmony::findDegree (key, c) == 1;
    }

    if (outside.isEmpty())
        s << "All of these chords are in the key.";
    else
        s << outside.joinIntoString (", ") << (outside.size() == 1 ? " uses" : " use")
          << " notes from outside the key. That adds colour; if it sounds wrong, check the song key.";

    if (harmony::findDegree (key, chords.front()) == 1)
        s << " It starts at home, which tells the ear what the key is.";

    if (! visitsHome)
        s << " It never lands on the home chord (" << harmony::triad (key, 1).getName() << "), so it floats without fully "
          << "resting. That can be lovely, but if the song feels unfinished, try ending on it.";

    if (chords.size() > 1 && getFamily (key, chords.back()) == Family::tension)
        s << " It ends on tension, so when it repeats, the pull back to the start keeps it moving.";

    if (chords.size() > 1)
        s << "\n\nStep by step:\n" << stepByStep (key, chords, true);

    return s.trimEnd();
}

//==============================================================================
int getCirclePosition (const MusicalKey& key)
{
    // How far the scale's tonic lies above the tonic of the major key with the same notes.
    int offset = 0;

    switch (key.scale)
    {
        case MusicalKey::Scale::major:         offset = 0; break;
        case MusicalKey::Scale::dorian:        offset = 2; break;
        case MusicalKey::Scale::phrygian:      offset = 4; break;
        case MusicalKey::Scale::lydian:        offset = 5; break;
        case MusicalKey::Scale::mixolydian:    offset = 7; break;
        case MusicalKey::Scale::minor:
        case MusicalKey::Scale::harmonicMinor:
        case MusicalKey::Scale::melodicMinor:  offset = 9; break;
        case MusicalKey::Scale::locrian:       offset = 11; break;
    }

    const int majorTonic = ((key.tonic - offset) % 12 + 12) % 12;
    return majorTonic * 7 % 12;
}

int getCircleTonic (int position)
{
    return ((position % 12 + 12) % 12) * 7 % 12;
}

juce::String getCircleLabel (int position, bool minor)
{
    static const char* majors[] = { "C", "G", "D", "A", "E", "B", "F#", "Db", "Ab", "Eb", "Bb", "F" };
    static const char* minors[] = { "Am", "Em", "Bm", "F#m", "C#m", "G#m", "D#m", "Bbm", "Fm", "Cm", "Gm", "Dm" };
    const int p = (position % 12 + 12) % 12;
    return minor ? minors[p] : majors[p];
}

juce::StringArray getKeySignature (int position)
{
    static const char* sharps[] = { "F#", "C#", "G#", "D#", "A#", "E#" };
    static const char* flats[] = { "Bb", "Eb", "Ab", "Db", "Gb" };
    const int p = (position % 12 + 12) % 12;
    juce::StringArray signature;

    for (int i = 0; i < (p <= 6 ? p : 12 - p); ++i)
        signature.add (p <= 6 ? sharps[i] : flats[i]);

    return signature;
}

juce::String describeKeySignature (int position)
{
    const int p = (position % 12 + 12) % 12;
    const int count = p <= 6 ? p : 12 - p;

    if (count == 0)
        return "no sharps or flats";

    const auto describe = [count] (const char* sign) { return juce::String (count) + " " + sign + (count == 1 ? "" : "s"); };

    if (p == 6)
        return describe ("sharp") + " or " + describe ("flat");

    return describe (p < 6 ? "sharp" : "flat");
}

std::vector<Chord> getCircleProgression (const MusicalKey& key)
{
    std::vector<Chord> chords;

    for (int degree : { 3, 6, 2, 5, 1 })
        chords.push_back (degree == 5 ? harmony::getBorrowedDominant (key).value_or (harmony::triad (key, 5))
                                      : harmony::triad (key, degree));

    return chords;
}

juce::String explainCircleOfFifths (const MusicalKey& key)
{
    const int position = getCirclePosition (key);
    const auto majorName = [] (int p) { return getCircleLabel (p, false) + " major"; };
    const auto minorName = [] (int p) { return getCircleLabel (p, true).dropLastCharacters (1) + " minor"; };
    const auto signature = getKeySignature (position);

    juce::String s;
    s << "The circle of fifths puts the twelve major keys in a ring, like a clock. Each step clockwise goes up a fifth "
      << "(C to G) and adds a sharp to the key signature. Each step anticlockwise goes down a fifth (C to F) and adds a flat. "
      << "At the bottom, F# major and Gb major are the same key, so the ring closes.\n\n"
      << "The inner ring has the relative minor of each major key: the same notes and key signature, but with a minor chord "
      << "as home, such as C major and A minor.\n\n";

    // Where this key sits.
    s << key.getName();

    if (key.scale == MusicalKey::Scale::major)
        s << " has " << describeKeySignature (position);
    else if (key.scale == MusicalKey::Scale::minor)
        s << " is the relative minor of " << majorName (position) << ": " << describeKeySignature (position);
    else if (key.scale == MusicalKey::Scale::harmonicMinor || key.scale == MusicalKey::Scale::melodicMinor)
        s << " sits at the place of " << minorName (position) << " (" << describeKeySignature (position) << "), with its "
          << (key.scale == MusicalKey::Scale::harmonicMinor ? "seventh note" : "sixth and seventh notes")
          << " raised, which gives a stronger pull home";
    else
        s << " uses the notes of " << majorName (position) << ", so it sits at its place: " << describeKeySignature (position);

    if (! signature.isEmpty())
        s << " (" << signature.joinIntoString (", ") << ")";

    s << ".";

    if (key.scale == MusicalKey::Scale::major)
        s << " Its relative minor is " << minorName (position) << ".";

    s << "\n\n" << "Neighbours on the circle are closely related keys: " << majorName (position + 1) << " and "
      << majorName (position - 1) << " differ from " << majorName (position) << " in one note only, so a song can move "
      << "there smoothly. Keys across the circle, such as " << majorName (position + 6) << ", share few notes and sound far away.\n\n";

    // The key's chords on the circle.
    juce::StringArray majors, minors, others;

    for (int degree = 1; degree <= 7; ++degree)
    {
        const auto chord = harmony::triad (key, degree);
        const auto name = nameWithNumeral (key, chord);

        if (chord.intervals == juce::Array<int> { 0, 4, 7 })
            majors.add (name);
        else if (chord.intervals == juce::Array<int> { 0, 3, 7 })
            minors.add (name);
        else
            others.add (name);
    }

    s << "The chords of " << key.getName() << " are coloured on the circle: " << majors.joinIntoString (", ")
      << " on the outer ring, and " << minors.joinIntoString (", ") << " on the inner ring.";

    if (! others.isEmpty())
        s << " " << (others.size() == 1 ? others[0] : others.joinIntoString (", ", 0, others.size() - 1) + " and " + others[others.size() - 1])
          << (others.size() == 1 ? " is" : " are")
          << " neither major nor minor, so " << (others.size() == 1 ? "it has" : "they have") << " no place on the circle.";

    if (key.scale != MusicalKey::Scale::harmonicMinor && key.scale != MusicalKey::Scale::melodicMinor)
        s << " Together they make a wedge: the key's own place and its two neighbours.";

    // Moving round the circle.
    const auto chain = getCircleProgression (key);
    juce::StringArray chainNames;
    for (auto& c : chain)
        chainNames.add (c.getName());

    s << "\n\nOne step anticlockwise is a fifth down, the strongest move between two chords. "
      << nameWithNumeral (key, chain[3]) << " to " << nameWithNumeral (key, chain[4]) << " ends most songs, and "
      << chain[2].getName() << dash().trim() << chain[3].getName() << dash().trim() << chain[4].getName()
      << " is two steps of it. Play walks the whole chain down to home: " << chainNames.joinIntoString (arrow()) << ".\n\n"
      << "Click a key on the circle to hear its chord and read about it. Double-click it to explore that key here.";
    return s;
}
} // namespace daw::gen::guide
