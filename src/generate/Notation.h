#pragma once

#include "ChordSheet.h"
#include "engine/Section.h"
#include "engine/TempoMap.h"

namespace daw::gen
{
/** Writing MIDI notes out as sheet music: which letter each note gets, where it sits on the staff, which
    clef the part needs, how its rhythm is rounded and split into written values, ties and rests, and where
    everything goes along a line. Pure and tested; the drawing is in ui/ScorePainter. The rules (R1 to R14)
    are in plans/score-view-midi-as-sheet-music.md. DawIt writes 4/4 only, one voice per staff.

    Rhythm is worked in twelfths of a beat ("units"), so a sixteenth is 3 units, an eighth-note triplet 4
    and a bar 48, and written lengths add up exactly. */
namespace notation
{
    constexpr int unitsPerBeat = 12;
    constexpr int unitsPerBar = 48;

    /** What the user chose in the Clef menu. */
    enum class ClefChoice
    {
        automatic,
        treble,
        bass,
        grand,          // treble and bass staves, split at middle C
        guitar,         // treble, written an octave above the sound
        bassGuitar      // bass, written an octave above the sound
    };

    enum class Clef { treble, bass };

    /** Written note values, whole to 32nd (the rounding grid only makes sixteenths and longer). */
    enum class Value { whole, half, quarter, eighth, sixteenth, thirtySecond };

    /** A note spelled: a letter (0 = C ... 6 = B), an alteration (-1 flat, 1 sharp) and an octave (C4 = middle C). */
    struct Pitch
    {
        int letter = 0;
        int alter = 0;
        int octave = 4;

        /** The MIDI note it sounds as (E#4 is 65). */
        int getMidi() const noexcept
        {
            static constexpr int naturals[] = { 0, 2, 4, 5, 7, 9, 11 };
            return (octave + 1) * 12 + naturals[letter] + alter;
        }

        bool operator== (const Pitch&) const = default;
    };

    /** A pitch on a staff. `step` counts lines and spaces from the bottom line (0); `midi` is the sound. */
    struct WrittenPitch : Pitch
    {
        int step = 0;
        int midi = 60;
        bool showAccidental = false;    // draw a sharp, flat or natural before the head
    };

    /** One thing in a bar: a chord of written pitches, or a rest. */
    struct Event
    {
        bool isRest = false;
        std::vector<WrittenPitch> pitches;      // low to high; empty for a rest
        Value value = Value::quarter;
        bool dotted = false;
        bool triplet = false;                   // three in the time of two
        int start = 0;                          // units from the bar's start
        int length = 12;                        // units, as written (a triplet eighth is 4)
        bool tiedToNext = false;                // a tie to the next event (also over the bar line)
        bool tiedFromPrevious = false;
        int beamGroup = -1;                     // index in the bar's beams, or -1 (a flag, or none)
        bool stemUp = false;
        double x = 0.0;                         // staff spaces from the bar's left edge
        std::vector<int> sourceNotes;           // indices into the input notes, for tooltips

        double getStartBeat() const noexcept { return start / (double) unitsPerBeat; }
        double getBeats() const noexcept { return length / (double) unitsPerBeat; }
    };

    /** Events joined by one beam: always in one beat. */
    struct Beam { std::vector<int> events; };

    /** A bracket over the events of one beat that are triplets. */
    struct Bracket { int first = 0, last = 0; };

    struct Bar
    {
        std::vector<Event> events;              // in time order, adding up to 4 beats
        std::vector<Beam> beams;
        std::vector<Bracket> triplets;
    };

    struct Staff
    {
        Clef clef = Clef::treble;
        int writtenShift = 0;                   // 12 when written an octave above the sound (guitars)
        std::vector<Bar> bars;
    };

    /** A sharp or flat of a key signature: where it sits on a staff. */
    struct KeyMark
    {
        int letter = 0;
        int alter = 1;
        int step = 0;
    };

    struct TempoMark { int bar = 0; double beat = 0.0; int bpm = 120; };
    struct ChordSymbol { int bar = 0; juce::String name; };
    struct SectionMark { int bar = 0; juce::String name; };

    /** Everything a part is written from. Notes are in beats from bar 0. */
    struct Input
    {
        std::vector<MidiNote> notes;
        MusicalKey key;
        ChordMap chords;
        std::vector<Section> sections;
        TempoMap tempo;
        ClefChoice clef = ClefChoice::automatic;
    };

    struct Score
    {
        MusicalKey key;
        ClefChoice clefUsed = ClefChoice::treble;   // the choice with Auto resolved
        std::vector<Staff> staves;                  // one, or two (treble then bass) for the grand staff
        int numBars = 1;
        int keyPosition = 0;                        // place on the circle of fifths
        std::vector<double> barWidths;              // natural widths in staff spaces, the same for every staff
        std::vector<TempoMark> tempoMarks;
        std::vector<ChordSymbol> chordSymbols;
        std::vector<SectionMark> sectionMarks;

        bool isGrand() const noexcept { return staves.size() == 2; }
    };

    /** How well one beat fits each rounding grid (R7): the summed rounding error of its start and end points
        (fractions of a beat, 0 to 1) on the sixteenth grid and on the eighth-note triplet grid, and which wins.
        Triplets win when a point is off the eighth grid and their error is under half the sixteenths'. */
    struct BeatFit
    {
        double sixteenthError = 0.0;
        double tripletError = 0.0;
        bool triplets = false;
    };

    BeatFit fitBeat (const std::vector<double>& points);

    /** A letter and alteration for a MIDI note in a key: in the scale, else a tone of the bar's chord (if any),
        else the raised 6th or 7th of a minor key, else a flat (a sharp when the key signature has sharps). */
    Pitch spell (int midi, const MusicalKey&, const Chord* chordOfBar);

    /** Lines and spaces above the staff's bottom line: E4 is 0 on the treble staff, G2 on the bass staff. */
    int getStaffStep (Clef, const Pitch&);

    /** How many ledger lines a note at this staff step needs (0 within the staff or just beyond it). */
    int getLedgerLines (int step);

    /** The clef Auto picks for these MIDI notes: treble, bass or grand. */
    ClefChoice chooseAutoClef (const std::vector<int>& midiNotes);

    /** The sharps or flats of a key signature (the place on the circle, 0 to 11) on a staff, in the order they're written. */
    std::vector<KeyMark> getKeySignature (int circlePosition, Clef);

    /** Writes the notes out. Never changes the notes. */
    Score build (const Input&);

    /** One line of music: a run of bars, with the header (clef and key signature, and the time signature on the first). */
    struct Line
    {
        int firstBar = 0;
        int numBars = 0;
        bool hasTimeSignature = false;
        double headerWidth = 0.0;
        double width = 0.0;                     // header and bars, as stretched
        double scale = 1.0;                     // how much the bars were stretched
        std::vector<double> barOrigins;         // staff spaces from the line's left edge, one per bar
    };

    /** The width of a line's header in staff spaces. */
    double getHeaderWidth (const Score&, bool firstLine);

    /** Breaks the bars into lines no wider than `width` staff spaces (a single bar wider than that gets a line of its own),
        never splitting a bar. Lines are stretched to the width, the last one only when `stretchLast` is set. */
    std::vector<Line> layoutLines (const Score&, double width, bool stretchLast = false);

    /** Where an event sits, in staff spaces from the line's left edge. */
    inline double getEventX (const Line& line, int barInLine, const Event& event)
    {
        return line.barOrigins[(size_t) barInLine] + event.x * line.scale;
    }
} // namespace notation
} // namespace daw::gen
