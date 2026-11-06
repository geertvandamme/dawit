#pragma once

#include "engine/TimeSignature.h"

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

    /** A position in beats as DawIt's position display shows it: bar.beat.sixteenth, all from 1, where the
        beat counts the bottom note of the signature; 18.25 beats is "5.3.2" in 4/4 and "7.1.2" in 3/4. */
    juce::String formatPosition (daw::TimeSignature, double beats);

    /** How a musician counts the moment `beat` (in beats from the song start; the bar is found from the
        signature): "1".."4" on the bottom notes (1 to 6 in 6/8), "e", "&" and "a" on the parts between them,
        "trip" and "let" on triplets; empty off those. */
    juce::String countAt (daw::TimeSignature, double beat);

    /** How the metronome clicks a bar, for the guide and tooltips: "loud on 1, softer on 2, 3 and 4" (4/4),
        "loud on 1, medium on 4, soft on the others" (6/8), "loud on 1, medium on 3 and 5, soft on the others" (7/8). */
    juce::String describeClicks (daw::TimeSignature);

    enum class Topic { barsAndBeats, timeSignatures, compoundAndOdd, noteValues, rests, dottedAndTriplets, counting, tempo,
                       readingDawIt, yourSong };

    constexpr Topic allTopics[] { Topic::barsAndBeats, Topic::timeSignatures, Topic::compoundAndOdd, Topic::noteValues,
                                  Topic::rests, Topic::dottedAndTriplets, Topic::counting, Topic::tempo,
                                  Topic::readingDawIt, Topic::yourSong };

    juce::String getTopicName (Topic);      // "Note values"
    juce::String getTopicSummary (Topic);   // one line for the list

    /** A MIDI or audio clip of the song, for the advice. */
    struct ClipLength
    {
        juce::String name;
        double beats = 0.0;
    };

    /** What the advice on *Your song's time signature* looks at. */
    struct SongRhythm
    {
        daw::TimeSignature signature = daw::timesig::fourFour;
        double bpm = 120.0;
        std::vector<ClipLength> clips;      // every MIDI and audio clip
        std::vector<double> noteStarts;     // where every note of the MIDI clips starts, in beats from the song start
    };

    /** The advice paragraphs for a song (R1 to R3): a clip that doesn't fill whole bars, a triplet feel in 4/4 that
        12/8 would write, and the grouping of an odd metre. Empty when there is nothing to say. */
    juce::StringArray getAdvice (const SongRhythm&);

    /** The explanation of a topic. The tempo topic uses the song's tempo, and the topics about bars the song's
        time signature, for their examples; *Your song's time signature* adds the advice paragraphs. */
    juce::String explain (Topic, double bpm, daw::TimeSignature, const juce::StringArray& advice = {});
} // namespace rhythm
} // namespace daw::gen
