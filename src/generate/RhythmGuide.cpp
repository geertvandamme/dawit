#include "RhythmGuide.h"
#include "engine/Tempo.h"
#include "engine/TimeSignature.h"

namespace daw::gen::rhythm
{
namespace
{
    constexpr double tolerance = 1.0e-3;

    bool near (double a, double b) { return std::abs (a - b) < tolerance; }

    /** "quarter" from "quarter note". */
    juce::String shortName (const juce::String& name) { return name.upToLastOccurrenceOf (" note", false, false); }

    juce::String withArticle (const juce::String& name)
    {
        return (juce::String ("aeiou").containsChar (name[0]) ? "an " : "a ") + name;
    }

    /** Plain and dotted values, longest first, with their short names: for splitting a length into tied notes. */
    std::vector<std::pair<double, juce::String>> getTieValues()
    {
        std::vector<std::pair<double, juce::String>> values;

        for (auto& v : getNoteValues())
        {
            values.push_back ({ v.beats * 1.5, "dotted " + shortName (v.name) });
            values.push_back ({ v.beats, shortName (v.name) });
        }

        return values;
    }
}

const std::vector<NoteValue>& getNoteValues()
{
    static const std::vector<NoteValue> values {
        { "whole note", "semibreve", "1 bar", 4.0 },
        { "half note", "minim", "1/2", 2.0 },
        { "quarter note", "crotchet", "1/4", 1.0 },
        { "eighth note", "quaver", "1/8", 0.5 },
        { "sixteenth note", "semiquaver", "1/16", 0.25 },
        { "thirty-second note", "demisemiquaver", "1/32", 0.125 }
    };

    return values;
}

juce::String nameLength (double beats)
{
    for (auto& v : getNoteValues())
    {
        if (near (beats, v.beats))
            return v.name;

        if (near (beats, v.beats * 1.5))
            return "dotted " + v.name;

        // Three in the time of two: a triplet note lasts two thirds of the plain one.
        if (v.beats <= 2.0 && v.beats >= 0.25 && near (beats, v.beats * 2.0 / 3.0))
            return shortName (v.name) + "-note triplet";
    }

    return {};
}

juce::String formatBeats (double beats)
{
    for (int thirds : { 1, 2, 4, 5 })
        if (near (beats, thirds / 3.0))
            return juce::String (thirds) + "/3 beat" + (thirds > 3 ? "s" : "");

    for (int sixths : { 1, 5 })
        if (near (beats, sixths / 6.0))
            return juce::String (sixths) + "/6 beat";

    if (near (beats, 1.0))
        return "1 beat";

    auto text = juce::String (beats, 3);
    while (text.containsChar ('.') && (text.endsWithChar ('0') || text.endsWithChar ('.')))
        text = text.dropLastCharacters (1);

    return text + " beats";
}

juce::String describeLength (double beats)
{
    if (const auto name = nameLength (beats); name.isNotEmpty())
        return name + ", " + formatBeats (beats);

    // Not one note value: split it into tied notes, longest first, if two or three do it.
    juce::StringArray parts;
    double left = beats;

    for (auto& [value, name] : getTieValues())
    {
        while (left >= value - tolerance && parts.size() < 3)
        {
            parts.add (withArticle (name));
            left -= value;
        }
    }

    if (std::abs (left) > tolerance || parts.size() < 2)
        return formatBeats (beats);

    const auto last = parts[parts.size() - 1];
    parts.remove (parts.size() - 1);
    return formatBeats (beats) + " (" + parts.joinIntoString (", ") + " tied to " + last + ")";
}

juce::String formatPosition (daw::TimeSignature sig, double beats)
{
    return timesig::formatPosition (sig, beats);
}

juce::String countAt (daw::TimeSignature sig, double beat)
{
    const double length = timesig::getBarLength (sig), pulse = timesig::getPulseLength (sig);
    double inBar = std::fmod (beat, length);

    if (inBar < 0.0)
        inBar += length;

    const double pulses = inBar / pulse;
    const double whole = std::floor (pulses + tolerance);
    const double part = pulses - whole;

    if (near (part, 0.0) || near (part, 1.0))  return juce::String ((int) whole % sig.numerator + 1);
    if (near (part, 0.25))  return "e";
    if (near (part, 0.5))   return "&";
    if (near (part, 0.75))  return "a";
    if (near (part, 1.0 / 3.0)) return "trip";
    if (near (part, 2.0 / 3.0)) return "let";
    return {};
}

juce::String describeClicks (daw::TimeSignature sig)
{
    auto list = [] (const std::vector<int>& numbers)
    {
        juce::StringArray parts;

        for (int n : numbers)
            parts.add (juce::String (n));

        if (parts.size() == 1)
            return parts[0];

        const auto last = parts[parts.size() - 1];
        parts.remove (parts.size() - 1);
        return parts.joinIntoString (", ") + " and " + last;
    };

    std::vector<int> groups, softs;

    for (int i = 1; i < sig.numerator; ++i)
        (timesig::getClick (sig, i) == timesig::Click::group ? groups : softs).push_back (i + 1);

    if (groups.empty())
        return softs.empty() ? juce::String ("loud on 1") : "loud on 1, softer on " + list (softs);

    return "loud on 1, medium on " + list (groups) + (softs.empty() ? juce::String() : juce::String (", soft on the others"));
}

juce::StringArray getAdvice (const SongRhythm& song)
{
    juce::StringArray advice;
    const auto sig = timesig::tidy (song.signature);
    const auto sigText = timesig::toString (sig);
    const double barLength = timesig::getBarLength (sig);

    auto number = [] (double v)
    {
        auto text = juce::String (v, 2);

        while (text.contains (".") && (text.endsWithChar ('0') || text.endsWithChar ('.')))
            text = text.dropLastCharacters (1);

        return text;
    };

    // R1: a clip longer than a bar that doesn't fill whole bars drifts against the bar lines when it repeats.
    std::vector<const ClipLength*> uneven;

    for (auto& clip : song.clips)
    {
        const double bars = clip.beats / barLength;

        if (clip.beats > barLength + 0.01 && std::abs (bars - std::round (bars)) * barLength > 0.01)
            uneven.push_back (&clip);
    }

    if (! uneven.empty())
    {
        const auto& clip = *uneven.front();
        const double bars = clip.beats / barLength;
        const int low = (int) std::floor (bars), high = low + 1;
        auto text = "'" + clip.name + "' is " + number (clip.beats) + " beats long: " + juce::String (bars, 1) + " bars of " + sigText
                    + ". A part that doesn't fill whole bars drifts against the bar lines when it repeats; try "
                    + number (low * barLength) + " or " + number (high * barLength) + " beats (" + juce::String (low) + " or "
                    + juce::String (high) + " bars).";

        if (uneven.size() > 1)
            text << " And " << (int) uneven.size() - 1 << " more.";

        advice.add (text);
    }

    // R2: many notes on triplets in 4/4 are a shuffle, which sheet music often writes in 12/8.
    if (sig == timesig::fourFour)
    {
        int offBeat = 0, triplets = 0, halves = 0;

        for (double start : song.noteStarts)
        {
            const double part = start - std::floor (start);
            const double nearestBeat = std::min (part, 1.0 - part);

            if (nearestBeat <= 0.03)
                continue;

            ++offBeat;

            if (std::abs (part - 1.0 / 3.0) <= 0.03 || std::abs (part - 2.0 / 3.0) <= 0.03)
                ++triplets;

            if (std::abs (part - 0.5) <= 0.03)
                ++halves;
        }

        if (offBeat >= 16 && triplets >= 0.6 * offBeat && halves <= 0.2 * offBeat)
            advice.add ("Many of your notes fall on triplets (" + juce::String (triplets) + " of the " + juce::String (offBeat)
                        + " notes between the beats): a shuffle or swing feel. Sheet music often writes this feel in 12/8, where every "
                          "beat splits into three eighths. To try it, start a song in 12/8 at " + formatBpm (song.bpm * 1.5)
                        + " BPM, so its dotted-quarter beat stays at " + formatBpm (song.bpm) + ".");
    }

    // R3: the grouping of an odd metre is DawIt's choice.
    if (timesig::getKind (sig) == timesig::Kind::odd && sig.denominator >= 8)
    {
        juce::StringArray groups;

        for (int g : timesig::getGroups (sig))
            groups.add (juce::String (g));

        advice.add ("DawIt groups " + sigText + " as " + groups.joinIntoString ("+") + "; music in " + sigText
                    + " may also use another grouping.");
    }

    return advice;
}

//==============================================================================
juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::barsAndBeats:      return "Bars and beats";
        case Topic::timeSignatures:    return "Time signatures";
        case Topic::compoundAndOdd:    return "Compound and odd metres";
        case Topic::noteValues:        return "Note values";
        case Topic::rests:             return "Rests";
        case Topic::dottedAndTriplets: return "Dotted notes and triplets";
        case Topic::counting:          return "Counting";
        case Topic::tempo:             return "Tempo and time";
        case Topic::readingDawIt:      return "Reading DawIt";
        case Topic::yourSong:          return "Your song's time signature";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::barsAndBeats:      return "4/4, the beat, and the time signature";
        case Topic::timeSignatures:    return "2/4, 3/4, 4/4 and 2/2: the simple metres";
        case Topic::compoundAndOdd:    return "6/8, 12/8, 5/4, 7/8: beats of three and uneven bars";
        case Topic::noteValues:        return "Whole, half, quarter, eighth, sixteenth";
        case Topic::rests:             return "Silence has a length too";
        case Topic::dottedAndTriplets: return "Half as long again, three in the time of two";
        case Topic::counting:          return "1 e & a: saying the rhythm out loud";
        case Topic::tempo:             return "BPM, and how long a note lasts";
        case Topic::readingDawIt:      return "The ruler, the grid, Snap and 5.3.2";
        case Topic::yourSong:          return "What your song's bars and clicks are";
    }

    return {};
}

juce::String explain (Topic t, double bpm, daw::TimeSignature sig, const juce::StringArray& advice)
{
    sig = timesig::tidy (sig);
    const auto sigText = timesig::toString (sig);

    switch (t)
    {
        case Topic::barsAndBeats:
            return "Bars and beats\n\n"
                   "The beat is the steady pulse you tap your foot to. Beats are grouped into bars (also called measures), and "
                   "almost all pop, rock, hip-hop and dance music has four beats in a bar: that is 4/4, or \"common time\". The "
                   "first beat of every bar is the strongest; it is where a new chord or section usually starts.\n\n"
                   "The time signature says how a bar is built. The top number is how many notes fill a bar; the bottom number "
                   "says which note value it counts (4 means a quarter note). So 4/4 is four quarter-note beats per bar. "
                   "Other common ones:\n"
                   "  3/4: three beats per bar, the waltz (ONE two three, ONE two three).\n"
                   "  6/8: two big beats per bar, each split in three (ONE two three FOUR five six), the rocking feel of many "
                   "ballads and Irish jigs.\n"
                   "  2/4: two beats per bar, marches and polkas.\n\n"
                   "Your song is in " + sigText + ": " + timesig::describe (sig) + ". To choose another, click the time signature next to "
                   "the tempo; the topics Time signatures and Compound and odd metres explain the common ones. The ruler numbers "
                   "the bars; the lines between them are the beats.";

        case Topic::timeSignatures:
            return "Time signatures\n\n"
                   "A time signature is two numbers, one above the other. The top number counts; the bottom number says what it "
                   "counts: 2 is a half note, 4 a quarter note, 8 an eighth note and 16 a sixteenth. So 3/4 is three quarter "
                   "notes in a bar, and 6/8 is six eighth notes.\n\n"
                   "When the top number is 2, 3 or 4, each beat is one of the bottom notes, and it splits into two halves. These "
                   "are the simple metres:\n"
                   "  2/4: two beats, ONE two. Marches and polkas.\n"
                   "  3/4: three beats, ONE two three. Waltzes, minuets and mazurkas.\n"
                   "  4/4: four beats, ONE two THREE four, with beat 3 a little stronger than 2 and 4. Most pop, rock, hip-hop and "
                   "dance music. It is also written as a C, for common time.\n"
                   "  2/2: two half-note beats, each split into two quarters. It holds as much as a bar of 4/4, but you feel two "
                   "big beats instead of four, which suits fast music. It is also written as a C with a line through it: cut time, "
                   "or alla breve.\n\n"
                   "The first beat of the bar is the strongest. It is where a chord usually changes and a section starts, which is "
                   "why chords and sections in DawIt belong to bars.\n\n"
                   "The time signature doesn't change how long a note is. At 120 BPM a quarter note lasts 0.5 seconds in 4/4 and "
                   "in 3/4; a bar of 3/4 just ends a beat sooner, after 1.5 seconds instead of 2.\n\n"
                   "Each row in the picture is one bar. Play a row to hear it: the loudest note is beat 1.";

        case Topic::compoundAndOdd:
            return juce::String ("Compound and odd metres\n\n"
                   "When the top number is 6, 9 or 12, you count the bar in big beats of three. 6/8 has six eighth notes, but you "
                   "feel two beats: ONE two three FOUR five six. Each big beat is a dotted quarter note (three eighths), and it "
                   "splits into three, not two. These are the compound metres:\n"
                   "  6/8: two beats of three. Irish jigs, tarantellas and many ballads.\n"
                   "  9/8: three beats of three.\n"
                   "  12/8: four beats of three. Slow blues: it sounds like 4/4 with every beat split into three.\n"
                   "  6/4: usually two beats of three quarter notes.\n\n"
                   "3/4 and 6/8 both hold six eighth notes, grouped differently. 3/4 is three beats of two eighths (ONE and TWO "
                   "and THREE and); 6/8 is two beats of three (ONE two three FOUR five six). Written music shows which by beaming "
                   "the eighths in twos or in threes.\n\n"
                   "Odd metres have a top number such as 5 or 7, which doesn't split into equal beats. Their beats are a mix of "
                   "twos and threes, so some beats are longer than others:\n"
                   "  5/4: five beats, usually felt as 3 + 2 or 2 + 3. Dave Brubeck's Take Five is in 5/4.\n"
                   "  7/8: seven eighths, grouped 2+2+3, 2+3+2 or 3+2+2, depending on the music. Radiohead's Paranoid Android has "
                   "passages in 7/8.\n"
                   "  9/8 can be odd too: Brubeck's Blue Rondo ") + juce::String::fromUTF8 ("\xc3\xa0") + " la Turk groups it 2+2+2+3.\n\n"
                   "DawIt groups 5/8 as 2+3 and 7/8 as 2+2+3. The metronome clicks every eighth, loud on beat 1 and medium where "
                   "each group starts, and the stronger grid lines show the same groups.\n\n"
                   "Play a row and count along: the loudest note is beat 1, the medium ones start each group.";

        case Topic::noteValues:
            return "Note values\n\n"
                   "A note's value says how long it lasts compared to the beat. In 4/4 the quarter note is one beat, and every "
                   "other value is that halved or doubled:\n"
                   "  Whole note: 4 beats, a whole bar of 4/4. Snap: 1 bar in 4/4.\n"
                   "  Half note: 2 beats, half a bar. Snap: 1/2.\n"
                   "  Quarter note: 1 beat. Snap: 1/4.\n"
                   "  Eighth note: half a beat, two per beat. Snap: 1/8.\n"
                   "  Sixteenth note: a quarter of a beat, four per beat. Snap: 1/16.\n"
                   "  Thirty-second note: an eighth of a beat. Snap: 1/32.\n\n"
                   "The fraction in the name is the part of a whole (4/4) bar, which is why DawIt's Snap menu says 1/4 for a "
                   "quarter note and 1/16 for a sixteenth. Snap's 1 bar is always a whole bar of your song: 3 beats in 3/4, 3.5 in "
                   "7/8.\n\n"
                   "In written music you can tell them apart by their look: the whole note is an open oval, the half note an "
                   "open oval with a stem, the quarter note a filled one with a stem, and every flag on the stem halves it "
                   "again (one flag: eighth, two: sixteenth, three: thirty-second). Notes next to each other are often joined "
                   "by beams instead of flags. The guide How to read music shows them on the staff.\n\n"
                   "British and other musicians use other names: semibreve (whole), minim (half), crotchet (quarter), quaver "
                   "(eighth), semiquaver (sixteenth) and demisemiquaver (thirty-second).\n\n"
                   "Click a row and Play to hear it against the beat. In the piano roll, hover over a note to see its value.";

        case Topic::rests:
            return "Rests\n\n"
                   "Silence is part of the rhythm, and it has the same lengths as notes: a whole rest, half rest, quarter rest, "
                   "eighth rest and so on. A rhythm is as much about where nothing plays as where something does: a bassline "
                   "with a rest before the next bar, a hi-hat that skips a beat, a vocal that breathes.\n\n"
                   "In a piano roll there are no rest symbols: an empty stretch of a row is a rest. So the length of a note "
                   "and the space after it together make the rhythm. Short notes with space between them sound punchy "
                   "(staccato); notes that run right up to the next one sound smooth (legato).\n\n"
                   "Try it: draw four quarter notes in a bar, then shorten each to an eighth. The notes start at the same "
                   "moments, but every one is now followed by an eighth rest, and the groove gets tighter.";

        case Topic::dottedAndTriplets:
            return "Dotted notes and triplets\n\n"
                   "A dot after a note makes it half as long again. A dotted quarter is 1 + 0.5 = 1.5 beats, a dotted half is 3 "
                   "beats, a dotted eighth is 0.75 beats. A dotted quarter followed by an eighth fills two beats with a long-short "
                   "feel you hear everywhere in pop (and the dotted eighth plus sixteenth gives the bouncy \"da-dum\" of "
                   "shuffles and marches).\n\n"
                   "A triplet fits three notes in the time of two. Three eighth-note triplets fill one beat (instead of two "
                   "eighths); three quarter-note triplets fill two beats. Each triplet note lasts two thirds of the plain one: an "
                   "eighth-note triplet is 1/3 of a beat. You count them \"1 trip let, 2 trip let\".\n\n"
                   "Triplets are why some music swings: in jazz, blues and shuffles the eighth notes are played long-short, like "
                   "the first two-thirds and the last third of a triplet. Those notes fall between the lines of the 1/16 grid: "
                   "draw them with Snap off (hold Option/Alt while dragging) or on 1/32 to get close.\n\n"
                   "A dotted quarter is also the beat of 6/8 and 12/8. A shuffle in 4/4, with its eighths played as triplets, is "
                   "often written in 12/8 instead, slow blues especially: each of its beats becomes a dotted quarter of three "
                   "eighths. It sounds the same; only the counting on paper changes.\n\n"
                   "Notes can also be tied: two notes joined into one longer sound, such as a quarter tied to a sixteenth (1.25 "
                   "beats). In a piano roll a tie is simply one longer note.";

        case Topic::counting:
            return "Counting\n\n"
                   "Musicians count rhythms out loud so they can feel where each note falls:\n"
                   "  Quarter notes: 1 2 3 4.\n"
                   "  Eighth notes: 1 & 2 & 3 & 4 & (say \"and\"). The \"&\" is the off-beat, halfway between two beats.\n"
                   "  Sixteenth notes: 1 e & a 2 e & a 3 e & a 4 e & a (say \"one ee and uh\").\n"
                   "  Triplets: 1 trip let 2 trip let.\n"
                   "  In 3/4: 1 & 2 & 3 &.\n"
                   "  In 6/8: 1 2 3 4 5 6, with the weight on 1 and 4.\n\n"
                   "Any note can be named by its count: a hi-hat \"on the &s\" plays on the off-beats, a snare \"on 2 and 4\" plays "
                   "the backbeat, and a chord \"on the a of 4\" is pushed a sixteenth before the next bar.\n\n"
                   "Counting helps most when a rhythm feels hard: say the full count slowly and clap only on the notes. Play a row "
                   "here and count along; the count of each note lights up as it plays.";

        case Topic::tempo:
        {
            const double beat = 60.0 / juce::jmax (1.0, bpm);
            auto seconds = [] (double s) { return juce::String (s, s < 0.1 ? 3 : 2) + " s"; };

            return "Tempo and time\n\n"
                   "The tempo is the speed of the beat, in beats per minute (BPM). At 60 BPM a beat lasts exactly one second; at "
                   "120 BPM half a second. Note values stay the same at any tempo (a quarter note is always one beat), but how "
                   "long they last in seconds changes: double the tempo and every note takes half as long.\n\n"
                   "At your song's " + formatBpm (bpm) + " BPM:\n"
                   "  a bar of " + sigText + " lasts " + seconds (timesig::getBarSeconds (sig, bpm)) + "\n"
                   "  a beat (quarter note) " + seconds (beat) + "\n"
                   "  an eighth note " + seconds (beat / 2.0) + "\n"
                   "  a sixteenth note " + seconds (beat / 4.0) + "\n"
                   "  8 bars " + seconds (timesig::getBarSeconds (sig, bpm) * 8.0) + ", 16 bars " + seconds (timesig::getBarSeconds (sig, bpm) * 16.0) + "\n\n"
                   "In DawIt the BPM always counts quarter notes, whatever the time signature, as MIDI files and DAWs such as Logic "
                   "Pro do; lengths in beats (a clip of 12 beats, a note of 0.5 beats) are quarter notes too. In 6/8 and 12/8 the "
                   "beat you feel is a dotted quarter, one and a half quarter notes, so at 120 BPM it goes at 80 a minute. Sheet "
                   "music often writes such a tempo as a dotted quarter = 80.\n\n"
                   "Typical tempos: ballads 60 to 80 BPM, hip-hop 80 to 100, pop and rock 100 to 130, house 120 to 128, techno "
                   "125 to 135, trance 136 to 140, dubstep 140 (felt as 70), drum & bass 170 to 175. Trap is often written at "
                   "140 with half-time drums, so it feels like 70.\n\n"
                   "Half time and double time: the same tempo can feel slower when the snare plays only once per bar (half time) "
                   "or faster when the drums play twice as many notes (double time).";
        }

        case Topic::readingDawIt:
            return "Reading DawIt\n\n"
                   "The ruler at the top numbers the bars and shows the time signature at its left. In the arrangement and the piano "
                   "roll, the strong lines are bars, the lighter ones beats, and the faint ones the Snap grid. In 6/8 the lighter "
                   "lines are the two big beats, and the eighths between them are fainter.\n\n"
                   "Snap sets the smallest step for drawing, moving and resizing notes: 1 bar (whole notes), 1/2, 1/4, 1/8, "
                   "1/16 (the default: sixteenth notes) or 1/32. Choose 1/4 to draw on the beats, 1/8 for off-beats, 1/16 for "
                   "busy drum and bass parts. Hold Option/Alt while dragging to place a note off the grid.\n\n"
                   "The position display next to the transport shows where the playhead is as bar.beat.sixteenth, all counted "
                   "from 1, and the time in minutes and seconds. The middle number counts the bottom note of the time signature. In "
                   "4/4, 1.1.1 is the very start, 1.3.1 is beat 3 of bar 1, and 5.3.2 is bar 5, beat 3, the second sixteenth: in "
                   "counting, the \"e\" of 3 in bar 5. In 6/8 the middle number counts eighths, 1 to 6, so 2.4.1 is bar 2's fourth "
                   "eighth: its second big beat.\n\n"
                   "Hover over a note in the piano roll to see its value, its length in beats and where it starts in the same "
                   "bar.beat.sixteenth form.";

        case Topic::yourSong:
        {
            auto trimmed = [] (double v)
            {
                auto text = juce::String (v, 2);

                while (text.endsWithChar ('0') && ! text.endsWith (".0"))
                    text = text.dropLastCharacters (1);

                return text;
            };

            const double bpmNow = juce::jmax (1.0, bpm);
            juce::String text = "Your song's time signature\n\nYour song is in " + sigText + ": " + timesig::describe (sig) + ". At "
                                + formatBpm (bpmNow) + " BPM a bar lasts " + trimmed (timesig::getBarSeconds (sig, bpmNow)) + " s.\n";

            if (timesig::getKind (sig) == timesig::Kind::compound)
            {
                const double groupLength = 3.0 * timesig::getPulseLength (sig);
                const char* const dotted[] = { "dotted eighth", "dotted quarter", "dotted half" };
                const int which = sig.denominator == 16 ? 0 : (sig.denominator == 8 ? 1 : 2);
                text << "DawIt's BPM counts quarter notes. In " << sigText << " the beat you feel is a " << dotted[which] << ", which goes at "
                     << formatBpm (timesig::getFeltBpm (sig, bpmNow)) << " a minute (" << formatBpm (bpmNow)
                     << juce::String::fromUTF8 (" \xc3\xb7 ") << trimmed (groupLength) << ").\n";
            }

            text << "The metronome clicks " << sig.numerator << (sig.numerator == 1 ? " time" : " times") << " a bar: "
                 << describeClicks (sig) << ".\n";

            for (auto& line : advice)
                text << line << "\n";

            text << "To change the time signature, click " << sigText << " next to the tempo. If the song already has clips, chords "
                    "or lyrics, DawIt asks whether each part keeps its bar or its timing.";
            return text;
        }
    }

    return {};
}
} // namespace daw::gen::rhythm
