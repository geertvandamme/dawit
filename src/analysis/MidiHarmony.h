#pragma once

#include "engine/Chord.h"
#include "engine/Clip.h"
#include "engine/TimeSignature.h"

#include <array>
#include <optional>
#include <vector>

namespace daw::analysis
{
/**
    Finding the key, the scale and the chords of a MIDI clip from its notes. Pure functions, no
    audio and no model:

    1. Every note counts its length in beats (inside the stretch looked at) times its velocity,
       added up per note name (C, C#, D...), whatever the octave: the clip's profile.
    2. The key: the profile is compared with the 24 Krumhansl and Kessler key profiles. When the
       best two are close, where the part ends and starts and where it spends its time decide.
    3. The scale on that home note: the one that holds most of the notes played (the simpler scale
       wins a near tie). Notes outside it are explained (a raised seventh, a passing note...).
    4. Chords: per bar, or per half bar when the halves clearly differ, the chord whose notes hold
       the most of the weight; the lowest note decides between chords sharing notes and gives the
       slash chord. A part that only plays one note at a time names a chord only for an arpeggio.

    Positions are in song beats, four beats to a bar, the first bar being bar 0.
*/
namespace midiharmony
{
    /** One note of the clip, in song beats; velocity 0 to 1. */
    struct Note
    {
        int pitch = 60;
        double start = 0.0, length = 1.0;
        float velocity = 0.8f;

        double getEnd() const noexcept { return start + length; }
    };

    /** The notes to look at and the stretch of the song they sit in (the clip's start and end). */
    struct Input
    {
        std::vector<Note> notes;
        double startBeat = 0.0, endBeat = 0.0;
        daw::TimeSignature timeSignature = timesig::fourFour;   // the song's: its bar is the unit the chords are found per
    };

    /** The notes of a MIDI clip in song beats, cut at the clip's end. */
    Input makeInput (const MidiClip&, daw::TimeSignature = timesig::fourFour);

    /** The scales it can name: the nine of `MusicalKey` and three five- and six-note ones. */
    enum class Scale
    {
        major, minor, dorian, phrygian, lydian, mixolydian, locrian, harmonicMinor, melodicMinor,
        majorPentatonic, minorPentatonic, blues
    };

    juce::Array<int> getScaleIntervals (Scale);   // semitones from the home note
    juce::String getScaleName (Scale);            // "minor", "major pentatonic", "blues"
    juce::String getNoteName (int pitchClass);    // "Eb"

    /** A note name spelled for a key: G# (not Ab) as the raised seventh of A minor, Eb as its blue note. */
    juce::String getSpelledName (int pitchClass, int tonic);

    /** "A minor", "D dorian", "A blues". */
    juce::String getKeyName (int tonic, Scale);

    /** The song key to set for a scale: the same scale where the song key has it, major for major
        pentatonic, minor for minor pentatonic and blues. */
    MusicalKey getSongKey (int tonic, Scale);

    /** A chord with its bass as a slash: "C/E"; "Am7" when the bass is its root or unknown (-1). */
    juce::String getChordName (const Chord&, int bass);

    //==============================================================================
    struct FoundKey
    {
        int tonic = 0;
        Scale scale = Scale::major;
        double fit = 0.0;               // share of the weight in the scale, 0 to 1
        MusicalKey songKey;             // what *Set the song's key* sets (getSongKey)
        juce::String name;              // "A minor"
    };

    /** A key as scored against the profile: major or minor, with the correlation, -1 to 1. */
    struct KeyScore
    {
        int tonic = 0;
        bool minor = false;
        double score = 0.0;

        juce::String getName() const { return MusicalKey::getTonicName (tonic) + (minor ? " minor" : " major"); }
    };

    /** A note name outside the scale found, with where it first appears and why it's there. */
    struct OutsideNote
    {
        int pitchClass = 0;
        int firstBar = 0;               // bar 0 is the first
        double share = 0.0;             // of the weight
        juce::String reason;            // "from harmonic minor: it makes the five chord major"
    };

    /** One bar, or one half of a bar, with the chord it plays. */
    struct Segment
    {
        int bar = 0;
        double startBeat = 0.0, endBeat = 0.0;
        bool split = false;             // the bar was split in halves
        bool secondHalf = false;
        std::array<double, 12> weights {};
        double total = 0.0;
        int bass = -1;                  // note name that's lowest for longest, -1 when silent
        bool singleLine = true;         // no two notes sound together

        std::optional<Chord> chord;
        int chordBass = -1;             // the chord's bass when it isn't its root, else -1
        double score = 0.0;             // how well the notes fit the chord
        bool noThird = false;           // only a root and a fifth sound: the key's chord on that root
        std::optional<Chord> otherChord;    // the close second reading, e.g. C/A for Am7
        int otherBass = -1;
        double otherGap = 0.0;          // how much worse it scores, 0 when the notes are the same
        std::vector<int> between;       // note names sounding that aren't in the chord

        juce::String getName() const { return chord ? getChordName (*chord, chordBass) : juce::String(); }
    };

    struct Result
    {
        int numNotes = 0, numNames = 0;
        bool tooFew = false;            // under 8 notes or 3 note names: no key
        std::array<double, 12> profile {};
        double totalWeight = 0.0;

        FoundKey key;
        std::vector<KeyScore> ranking;  // the best three, best first
        bool close = false;             // the top two are within 0.1
        bool relative = false;          // the runner-up is the relative major or minor
        juce::String closeReason;       // "the part ends on A"
        KeyScore runnerUp;

        std::vector<OutsideNote> outside;
        std::vector<Segment> segments;
        bool singleLine = true;
        bool melody = false;            // no chord in any bar
    };

    /** The key, scale and chords of the notes. */
    Result analyse (const Input&);

    /** The chords per bar and half bar. The key (when known) decides power chords and close calls. */
    std::vector<Segment> findChords (const Input&, const std::optional<MusicalKey>& key);

    /** The weight of a note between two beats: the beats it sounds in that stretch times its velocity. */
    double getWeight (const Note&, double from, double to);
} // namespace midiharmony
} // namespace daw::analysis
