#pragma once

#include "Notation.h"

#include <vector>

namespace daw::gen
{
/** How to read sheet music, for beginners: the staff, the treble and bass clefs, note names on the lines and
    spaces, ledger lines, the grand staff, sharps and flats, key signatures, time and tempo, note values, ties,
    chord symbols and how DawIt writes MIDI as a score, each with a picture drawn from an example score; and a look
    at the selected track's own part. Used by the reading guide. The staff drawing itself is `notation` + the UI's
    ScorePainter. */
namespace readmusic
{
    enum class Topic { staff, clefs, trebleNotes, bassNotes, ledgerLines, grandStaff, accidentals, keySignatures,
                       timeAndTempo, noteValues, ties, chordSymbols, howDawItWrites, yours };

    constexpr Topic allTopics[] { Topic::staff, Topic::clefs, Topic::trebleNotes, Topic::bassNotes, Topic::ledgerLines,
                                  Topic::grandStaff, Topic::accidentals, Topic::keySignatures, Topic::timeAndTempo,
                                  Topic::noteValues, Topic::ties, Topic::chordSymbols, Topic::howDawItWrites, Topic::yours };

    juce::String getTopicName (Topic);      // "Ties and the beat"
    juce::String getTopicSummary (Topic);   // one line for the list

    /** A MIDI note as the piano roll draws it: a block at a beat. */
    struct MidiBlock
    {
        double start = 0.0, length = 1.0;
        int pitch = 60;
    };

    /** One staff picture: a score written by `notation::build`, with what to add to it. */
    struct Panel
    {
        juce::String label;                     // what the picture shows ("Treble clef")
        notation::Score score;
        std::vector<int> accentNotes;           // MIDI notes drawn in the accent colour
        std::vector<int> keyboardNotes;         // lit on a keyboard under the staff; empty: no keyboard
        bool showNoteNames = false;             // the letter of each note under the staff
        std::vector<MidiBlock> played;          // MIDI blocks drawn above the staff, as played
        int firstBar = 0;                       // the bars to show: from here, `numBars` of them (-1: all)
        int numBars = -1;
    };

    /** The pictures of a topic, and the notes that play when Play is pressed (empty for topics without a rhythm). */
    struct Example
    {
        std::vector<Panel> panels;
        std::vector<MidiNote> playNotes;
    };

    /** The selected MIDI track as the guide sees it: its notes (beats from bar 0), the song's key, chords, sections and
        tempo, the clef chosen for it, and where the playhead is. `hasTrack` is false when no MIDI track is selected. */
    struct Part
    {
        bool hasTrack = false;
        juce::String trackName;
        notation::Input input;
        double playheadBeat = 0.0;
        bool autoClef = true;                   // the clef is Auto (input.clef is automatic)
    };

    /** The pictures of a topic; "yours" shows four bars round the playhead of the part. The key and tempo of the
        song come from `part.input`. */
    Example getExample (Topic, const Part&);

    /** The explanation of a topic; "yours" is `analysePart (part).text`. */
    juce::String explain (Topic, const Part&);

    /** What "Your part" says about the track: the numbers behind the text, and the text. */
    struct PartReport
    {
        bool hasTrack = false;
        int numNotes = 0;
        int firstBar = 1, lastBar = 1;          // shown as the ruler does (bar 1 is the first)
        notation::ClefChoice clef = notation::ClefChoice::treble;
        int outsideKey = 0;                     // notes that carry a sharp, flat or natural
        int raisedSeventh = 0;                  // of those, the raised 7th of a minor key
        int moved = 0;                          // notes whose start is more than 0.0625 beats from where they're written
        double maxMoved = 0.0;
        int maxMovedBar = 1;
        int tripletBeats = 0;
        int held = 0;                           // notes still held when the next one starts
        juce::StringArray advice;
        juce::String text;
    };

    PartReport analysePart (const Part&);
} // namespace readmusic
} // namespace daw::gen
