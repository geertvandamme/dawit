#include "RhythmGuide.h"
#include "engine/Tempo.h"

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

juce::String formatPosition (double beats)
{
    const int bar = (int) std::floor (beats / 4.0 + 1.0e-9) + 1;
    const int beat = (int) std::floor (std::fmod (beats, 4.0) + 1.0e-9) + 1;
    const int sixteenth = (int) std::floor ((beats - std::floor (beats + 1.0e-9)) * 4.0 + 1.0e-6) + 1;
    return juce::String (bar) + "." + juce::String (beat) + "." + juce::String (juce::jlimit (1, 4, sixteenth));
}

juce::String countAt (double beat)
{
    const double whole = std::floor (beat + tolerance);
    const double part = beat - whole;

    if (near (part, 0.0))   return juce::String ((int) whole % 4 + 1);
    if (near (part, 0.25))  return "e";
    if (near (part, 0.5))   return "&";
    if (near (part, 0.75))  return "a";
    if (near (part, 1.0 / 3.0)) return "trip";
    if (near (part, 2.0 / 3.0)) return "let";
    return {};
}

//==============================================================================
juce::String getTopicName (Topic t)
{
    switch (t)
    {
        case Topic::barsAndBeats:      return "Bars and beats";
        case Topic::noteValues:        return "Note values";
        case Topic::rests:             return "Rests";
        case Topic::dottedAndTriplets: return "Dotted notes and triplets";
        case Topic::counting:          return "Counting";
        case Topic::tempo:             return "Tempo and time";
        case Topic::readingDawIt:      return "Reading DawIt";
    }

    return {};
}

juce::String getTopicSummary (Topic t)
{
    switch (t)
    {
        case Topic::barsAndBeats:      return "4/4, the beat, and the time signature";
        case Topic::noteValues:        return "Whole, half, quarter, eighth, sixteenth";
        case Topic::rests:             return "Silence has a length too";
        case Topic::dottedAndTriplets: return "Half as long again, three in the time of two";
        case Topic::counting:          return "1 e & a: saying the rhythm out loud";
        case Topic::tempo:             return "BPM, and how long a note lasts";
        case Topic::readingDawIt:      return "The ruler, the grid, Snap and 5.3.2";
    }

    return {};
}

juce::String explain (Topic t, double bpm)
{
    switch (t)
    {
        case Topic::barsAndBeats:
            return "Bars and beats\n\n"
                   "The beat is the steady pulse you tap your foot to. Beats are grouped into bars (also called measures), and "
                   "almost all pop, rock, hip-hop and dance music has four beats in a bar: that is 4/4, or \"common time\". The "
                   "first beat of every bar is the strongest; it is where a new chord or section usually starts.\n\n"
                   "The time signature says how a bar is built. The top number is how many beats a bar has; the bottom number "
                   "says which note value counts as one beat (4 means a quarter note). So 4/4 is four quarter-note beats per bar. "
                   "Other common ones:\n"
                   "  3/4: three beats per bar, the waltz (ONE two three, ONE two three).\n"
                   "  6/8: two big beats per bar, each split in three (ONE two three FOUR five six), the rocking feel of many "
                   "ballads and Irish jigs.\n"
                   "  2/4: two beats per bar, marches and polka.\n\n"
                   "DawIt works in 4/4: one bar is four beats. The ruler numbers the bars; the lines between them are the beats.";

        case Topic::noteValues:
            return "Note values\n\n"
                   "A note's value says how long it lasts compared to the beat. In 4/4 the quarter note is one beat, and every "
                   "other value is that halved or doubled:\n"
                   "  Whole note: 4 beats, the whole bar. Snap: 1 bar.\n"
                   "  Half note: 2 beats, half a bar. Snap: 1/2.\n"
                   "  Quarter note: 1 beat. Snap: 1/4.\n"
                   "  Eighth note: half a beat, two per beat. Snap: 1/8.\n"
                   "  Sixteenth note: a quarter of a beat, four per beat. Snap: 1/16.\n"
                   "  Thirty-second note: an eighth of a beat. Snap: 1/32.\n\n"
                   "The fraction in the name is the part of a whole (4/4) bar, which is why DawIt's Snap menu says 1/4 for a "
                   "quarter note and 1/16 for a sixteenth.\n\n"
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
                   "Notes can also be tied: two notes joined into one longer sound, such as a quarter tied to a sixteenth (1.25 "
                   "beats). In a piano roll a tie is simply one longer note.";

        case Topic::counting:
            return "Counting\n\n"
                   "Musicians count rhythms out loud so they can feel where each note falls:\n"
                   "  Quarter notes: 1 2 3 4.\n"
                   "  Eighth notes: 1 & 2 & 3 & 4 & (say \"and\"). The \"&\" is the off-beat, halfway between two beats.\n"
                   "  Sixteenth notes: 1 e & a 2 e & a 3 e & a 4 e & a (say \"one ee and uh\").\n"
                   "  Triplets: 1 trip let 2 trip let.\n\n"
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
                   "  a bar (whole note) lasts " + seconds (beat * 4.0) + "\n"
                   "  a beat (quarter note) " + seconds (beat) + "\n"
                   "  an eighth note " + seconds (beat / 2.0) + "\n"
                   "  a sixteenth note " + seconds (beat / 4.0) + "\n"
                   "  8 bars " + seconds (beat * 32.0) + ", 16 bars " + seconds (beat * 64.0) + "\n\n"
                   "Typical tempos: ballads 60 to 80 BPM, hip-hop 80 to 100, pop and rock 100 to 130, house 120 to 128, techno "
                   "125 to 135, trance 136 to 140, dubstep 140 (felt as 70), drum & bass 170 to 175. Trap is often written at "
                   "140 with half-time drums, so it feels like 70.\n\n"
                   "Half time and double time: the same tempo can feel slower when the snare plays only once per bar (half time) "
                   "or faster when the drums play twice as many notes (double time).";
        }

        case Topic::readingDawIt:
            return "Reading DawIt\n\n"
                   "The ruler at the top numbers the bars. In the piano roll, the strong vertical lines are bars, the lighter ones "
                   "beats, and the faint ones the Snap grid.\n\n"
                   "Snap sets the smallest step for drawing, moving and resizing notes: 1 bar (whole notes), 1/2, 1/4, 1/8, "
                   "1/16 (the default: sixteenth notes) or 1/32. Choose 1/4 to draw on the beats, 1/8 for off-beats, 1/16 for "
                   "busy drum and bass parts. Hold Option/Alt while dragging to place a note off the grid.\n\n"
                   "The position display next to the transport shows where the playhead is as bar.beat.sixteenth, all counted "
                   "from 1, and the time in minutes and seconds. So 1.1.1 is the very start, 1.3.1 is beat 3 of bar 1, and 5.3.2 "
                   "is bar 5, beat 3, the second sixteenth: in counting, the \"e\" of 3 in bar 5.\n\n"
                   "Hover over a note in the piano roll to see its value, its length in beats and where it starts in the same "
                   "bar.beat.sixteenth form.";
    }

    return {};
}
} // namespace daw::gen::rhythm
