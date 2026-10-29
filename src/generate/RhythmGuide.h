#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace daw::gen
{
/** Plain-language explanations of rhythm for beginning musicians: note values and rests, dotted
    notes and triplets, counting, tempo, and how they show in DawIt's ruler, Snap grid and position
    display. Used by the rhythm guide and the piano roll's note tooltips. */
namespace rhythm
{
    /** A plain note value, from the whole note down to the thirty-second note. */
    struct NoteValue
    {
        juce::String name;      // "quarter note"
        juce::String british;   // "crotchet"
        juce::String fraction;  // "1/4", as the Snap menu says it
        double beats;           // in 4/4, where a beat is a quarter note
    };

    /** Whole, half, quarter, eighth, sixteenth and thirty-second notes. */
    const std::vector<NoteValue>& getNoteValues();

    /** The name of a length that is one note value, e.g. "quarter note", "dotted eighth note" or
        "eighth-note triplet"; empty when it isn't one. */
    juce::String nameLength (double beats);

    /** A note's length for people: "quarter note, 1 beat", "dotted eighth note, 0.75 beats", or for a
        length that isn't one value "1.75 beats (a dotted quarter tied to a sixteenth)". */
    juce::String describeLength (double beats);

    /** "1 beat", "0.75 beats", "1/3 beat". */
    juce::String formatBeats (double beats);

    /** A position in beats as DawIt's position display shows it: bar.beat.sixteenth, all from 1,
        e.g. 18.25 beats is "5.3.2". */
    juce::String formatPosition (double beats);

    /** How a musician counts the moment `beat` (from the start of the bar): "1".."4" on the beats,
        "e", "&" and "a" on the sixteenths in between, "trip" and "let" on triplets; empty off those. */
    juce::String countAt (double beat);

    enum class Topic { barsAndBeats, noteValues, rests, dottedAndTriplets, counting, tempo, readingDawIt };

    constexpr Topic allTopics[] { Topic::barsAndBeats, Topic::noteValues, Topic::rests, Topic::dottedAndTriplets,
                                  Topic::counting, Topic::tempo, Topic::readingDawIt };

    juce::String getTopicName (Topic);      // "Note values"
    juce::String getTopicSummary (Topic);   // one line for the list

    /** The explanation of a topic; the tempo topic uses the song's tempo for its examples. */
    juce::String explain (Topic, double bpm);
} // namespace rhythm
} // namespace daw::gen
