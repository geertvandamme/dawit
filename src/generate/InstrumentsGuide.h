#pragma once

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace daw::gen
{
/**
    The "Instruments in MIDI" guide: how each family of instruments makes its sound (strings, tubes, drums and bars), then one
    topic per family (keys, guitar, bass, strings, brass, woodwinds, voice, synths, drums), orchestration, what makes MIDI
    sound real, and a check of the selected MIDI track's notes against the instrument it's for. The content only: the
    window is InstrumentsExplainer. The instrument table, the guess, the physics numbers and the analysis rules are checked
    in the tests.
*/
namespace instruments
{
    enum class Topic { makeSound, strings, tubes, drumsBars, keys, guitar, bass, bowed, brass, woodwinds, voice, synths,
                       drumKit, orchestration, realMidi, yours };

    inline constexpr Topic allTopics[] { Topic::makeSound, Topic::strings, Topic::tubes, Topic::drumsBars, Topic::keys,
                                         Topic::guitar, Topic::bass, Topic::bowed, Topic::brass, Topic::woodwinds, Topic::voice,
                                         Topic::synths, Topic::drumKit, Topic::orchestration, Topic::realMidi, Topic::yours };

    juce::String getTopicName (Topic);      // "Brass"
    juce::String getTopicSummary (Topic);   // one line for the list

    /** The text of a topic. For "Your track" it is a short introduction followed by `yourTrackText` (the analysis); with no
        text given, the analysis part says to select a MIDI track. */
    juce::String explain (Topic, const juce::String& yourTrackText = {});

    //==============================================================================
    // The instrument table

    enum class Family { keys, guitar, bass, strings, brass, woodwinds, voice, synths, drums };

    constexpr Family allFamilies[] { Family::keys, Family::guitar, Family::bass, Family::strings, Family::brass,
                                     Family::woodwinds, Family::voice, Family::synths, Family::drums };

    juce::String getFamilyName (Family);    // "Woodwinds"

    /** The topic that covers a family: Brass for the brass instruments. */
    Topic getFamilyTopic (Family);

    enum class Instrument { piano, organ, guitar, bassGuitar, violin, viola, cello, doubleBass, stringSection, harp,
                            trumpet, frenchHorn, trombone, tuba, brassSection, flute, oboe, clarinet, bassoon, altoSax, tenorSax,
                            voice, choir, synth, drumKit };

    /** One instrument as the guide knows it: the notes it sounds (MIDI numbers, middle C is 60) and how it's played. */
    struct Info
    {
        Instrument instrument = Instrument::piano;
        Family family = Family::keys;
        juce::String name;                  // "Cello"
        int lowest = -1, highest = -1;      // the full range a skilled player reaches, as it sounds; -1: no range (organ, synth)
        int atOnce = 0;                     // how many notes it plays at once; 0: no limit
        bool breathes = false;              // wind, brass and voices need to breathe
        bool swells = false;                // a held note can grow and fade (so a flat long note is worth a look)
        int writtenHigher = 0;              // semitones its music is written above how it sounds
        int fiveStringLowest = -1;          // a bass guitar with five strings reaches B0 (23); -1 for the rest
        juce::String generalMidi;           // its General MIDI programs, shown 1 to 128: "57, 60"
        juce::StringArray nameWords;        // words in a track name that point to it, lower case
    };

    /** Every instrument, in the order of the enum. */
    const std::vector<Info>& getInstruments();
    const Info& getInfo (Instrument);

    /** True when the instrument's notes are numbers on the General MIDI drum map (35 to 81) and not pitches. */
    bool isDrumKit (Instrument);

    /** The sounding frequency of a MIDI note, in Hz: 69 is 440. */
    double getNoteHz (int note);

    /** The instrument's name in the middle of a sentence: "cello", "French horn", "drum kit". */
    juce::String getSentenceName (const Info&);

    //==============================================================================
    // Which instrument a track is for

    /** What DawIt knows about a track, for the guess. */
    struct Hints
    {
        bool hasSampler = false;            // the track plays DawIt Sampler
        int samplerBank = 0;                // 128 for a drum kit
        int samplerProgram = 0;             // as shown, 1 to 128 (the sampler's own number plus one)
        juce::String samplerPreset;         // the preset's name, for the text: "Cello"
        juce::String trackName;
        juce::String builtInSound;          // "piano", "organ", "bass", "lead", "pluck", "pad", "drums" or "percussion" when the
                                            // track plays the built-in synth; empty otherwise
        juce::String dawItInstrument;       // the name of the plugin when the track has one: "DawIt Drums", "DawIt Subtractive",
                                            // "Some Plugin"; empty when none
    };

    enum class GuessSource { none, sampler, name, builtInSound, plugin, chosen };

    struct Guess
    {
        std::optional<Instrument> instrument;
        GuessSource source = GuessSource::none;
        juce::String detail;                // the preset's name, or the sound's or plugin's name
    };

    /** The instrument a track is for, from its DawIt Sampler preset (General MIDI program, or bank 128 for drums), then its name
        (the longest word first, so "double bass" and "bassoon" beat "bass"), then its DawIt sound (built-in synth sound, DawIt
        Drums, DawIt Subtractive, Additive or FM). DawIt Physical and other plugins give no guess. */
    Guess guessInstrument (const Hints&);

    //==============================================================================
    // The physics

    /** The harmonics of a string (or any whole-number series): `count` multiples of `fundamental`. A2 at 110 Hz: 110, 220, 330, 440. */
    std::vector<double> getHarmonics (double fundamental, int count);

    /** The lowest note of a tube open at both ends: c / 2L. 0.66 m at 343 m/s: 259.8 Hz. `harmonic` 2 is twice that. */
    double getOpenTubeHz (double lengthMetres, int harmonic = 1, double speedOfSound = 343.0);

    /** A tube closed at one end holds only odd harmonics: n c / 4L for n = 1, 3, 5. 0.66 m: 129.9, 389.8, 649.6 Hz. */
    double getClosedTubeHz (double lengthMetres, int oddHarmonic = 1, double speedOfSound = 343.0);

    /** The first six modes of an ideal round drum skin as multiples of the lowest, from the zeros of the Bessel functions:
        1, 1.593, 2.136, 2.295, 2.653, 2.917. */
    std::vector<double> getMembraneRatios();

    /** The first three modes of a bar free at both ends as multiples of the lowest, from beam theory: 1, 2.756, 5.404. */
    std::vector<double> getFreeBarRatios();

    /** How far the n-th harmonic is from the nearest note of equal temperament, in cents: the 5th is -13.7, the 7th -31.2. */
    double getHarmonicCentsFromEqual (int harmonic);

    /** The B-flat trumpet's open harmonics: n times B-flat 2, 116.54 Hz. */
    double getTrumpetHarmonicHz (int harmonic);

    /** How long a delay of `ms` milliseconds is in beats at `bpm`: 15 ms at 120 BPM is 0.03 beat. */
    double getMillisecondsAsBeats (double ms, double bpm);

    //==============================================================================
    // Example data for the pictures

    /** A note of an example line: start and length in beats, velocity 1 to 127. */
    struct ExampleNote
    {
        int pitch = 60;
        double start = 0.0, length = 1.0;
        int velocity = 90;
    };

    /** A value of a controller or the pitch bend at a beat: the pitch bend runs 0 to 16383 (8192 is the middle), the others 0 to 127. */
    struct ExamplePoint
    {
        double beat = 0.0;
        int value = 0;
    };

    /** A line of notes with what is drawn under it. `chordLabels` put a name over a bar ("C" at beat 0). */
    struct ExampleLine
    {
        juce::String label;                 // "Typed", "Played"
        std::vector<ExampleNote> notes;
        std::vector<ExamplePoint> expression;
        std::vector<ExamplePoint> bend;
        std::vector<std::pair<double, juce::String>> chordLabels;
    };

    /** An instrument's range as a bar on the keyboard, with its open strings where it has them. */
    struct RangeBar
    {
        juce::String name;
        int lowest = 0, highest = 0;
        Family family = Family::keys;
        std::vector<int> openStrings;
    };

    /** What a topic's picture is drawn from. Which members are used depends on the topic (see getExample). */
    struct Example
    {
        double bpm = 120.0;
        std::vector<ExampleLine> lines;             // notes to draw and to play with Play; empty for topics without a line to hear
        std::vector<RangeBar> ranges;               // keyboards with range bars
        std::vector<std::vector<double>> numbers;   // frequencies or ratios, one row for each part of the picture
        juce::StringArray labels;                   // what each row of numbers or each tile is
        juce::StringArray details;                  // under each tile: its instruments
        int glideMs = 0;                            // synth topic: the glide time of the overlapping notes
        double strumMs = 0.0;                       // guitar topic: the time between strings
    };

    /** The example for a topic; "Your track" has none (its picture follows the selected track). */
    Example getExample (Topic);

    //==============================================================================
    // Your track

    /** One note of the selected track, as the analysis needs it. */
    struct TrackNote
    {
        int pitch = 60;
        double startBeat = 0.0;
        double startSeconds = 0.0;
        double lengthSeconds = 0.5;
        int velocity = 90;
        int bar = 1;                        // as the ruler shows it: 1 is the first bar
    };

    /** The selected MIDI track: the notes of all its clips, and whether any clip has mod wheel (1), volume (7) or expression (11) events. */
    struct TrackPart
    {
        bool hasTrack = false;
        juce::String trackName;
        std::vector<TrackNote> notes;
        bool hasLoudnessController = false;
    };

    /** What the analysis found, and the text. */
    struct Report
    {
        bool hasNotes = false;
        std::optional<Instrument> instrument;       // the one the notes were checked against; none when unknown
        GuessSource source = GuessSource::none;
        int numNotes = 0;
        int below = 0, above = 0;                   // notes under the lowest note or over the top (drums: outside the drum map)
        int firstBelowBar = 0, firstAboveBar = 0;
        int togetherMoments = 0;                    // places where more notes start together than the instrument plays
        double longestPhraseSeconds = 0.0;
        int longestPhraseBar = 0;
        bool breathMissing = false;
        bool flatVelocity = false;
        int longFlatNotes = 0;                      // notes of 2 seconds or more with no mod wheel, volume or expression
        bool nothingToPoint = false;
        juce::StringArray paragraphs;
        juce::String text;                          // the paragraphs, a blank line between
    };

    /** Checks the notes against the instrument: `chosen` is the dropdown's choice (none: guess from the hints). The rules are
        in the plan: no track, unknown instrument, identity, below, above, too many at once, breath, velocity, swell, all well. */
    Report analyse (const TrackPart&, std::optional<Instrument> chosen, const Hints&);
} // namespace instruments
} // namespace daw::gen
