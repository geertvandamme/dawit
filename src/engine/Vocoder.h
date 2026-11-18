#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <vector>

/**
    The rules of DawIt Vocoder with no audio in them, so the processor, its window, the guide's pictures and the
    tests all use the same numbers: the log-spaced bands (edges, centres, widths and Q) for any Bands, Low and High,
    which bands get consonant noise, how a chord or chosen notes are placed as a carrier, the starting points, a
    made-up test voice, and what the last seconds say about the track ("Your track").
*/
namespace daw::vocoder
{
/** The most bands: everything the processor holds is sized for this many. */
constexpr int maxBands = 24;

/** The choices of Bands. */
constexpr std::array<int, 5> bandCounts { 4, 8, 12, 16, 24 };

/** The ranges of the band range in Hz, and its defaults. */
constexpr float minLowHz = 50.0f, maxLowHz = 400.0f, defaultLowHz = 100.0f;
constexpr float minHighHz = 4000.0f, maxHighHz = 12000.0f, defaultHighHz = 8000.0f;

/** Bands whose upper edge reaches this get noise for consonants (and High never goes below it). */
constexpr double consonantHz = 4000.0;

/** The top band can't pass this share of the sample rate. */
constexpr double topEdgeOfRate = 0.45;

/** The other ranges. */
constexpr float minSmoothingMs = 5.0f, maxSmoothingMs = 100.0f;
constexpr float minVoiceDb = -60.0f;    // the bottom of Voice is "off"
constexpr float minCarrierDb = -24.0f, maxCarrierDb = 12.0f;
constexpr float maxFormant = 12.0f;
constexpr int maxCarrierVoices = 16;

/** Where the carrier's notes come from. */
enum class Source { songChord, notes, keys };

/** The carrier's sound. */
enum class Wave { saw, pulse, choir, noise };

/** The notes played by Notes: the root alone, with its fifth, or a major or minor chord. */
enum class Shape { note, fifth, major, minor };

/** The controls of DawIt Vocoder. The defaults are the starting point Talking synth. */
struct Settings
{
    int bands = 16;
    float smoothingMs = 15.0f;
    float consonantsPercent = 50.0f;
    float voiceDb = minVoiceDb;         // minVoiceDb: no voice under the result
    float lowHz = defaultLowHz, highHz = defaultHighHz;
    Source source = Source::songChord;
    Wave wave = Wave::saw;
    int root = 0;                       // 0 = C
    Shape shape = Shape::major;
    int range = 2;                      // octaves, 1 to 3
    float carrierDb = 0.0f;
    float formantSemitones = 0.0f;
    float mixPercent = 100.0f;
};

/** The edges of the bands, lowest first: bands + 1 numbers, Low × (High / Low)^(k / bands), where High can't pass 0.45 × the sample rate. */
std::vector<double> getBandEdges (int bands, double lowHz, double highHz, double sampleRate);

/** The top of the range in force: High, or 0.45 × the sample rate when that is lower. */
double getTopEdge (double highHz, double sampleRate);

/** The middle of each band (the geometric mean of its edges). */
std::vector<double> getBandCentres (int bands, double lowHz, double highHz, double sampleRate);

/** How wide every band is, in octaves and in semitones. */
double getBandWidthOctaves (int bands, double lowHz, double highHz, double sampleRate);
double getBandWidthSemitones (int bands, double lowHz, double highHz, double sampleRate);

/** The band-pass Q of each band's biquad: 1 / (2 sinh (ln 2 × width / 2)), the width in octaves, so a single biquad is -3 dB at the
    band's edges. Each band runs two such biquads in a row, so neighbouring bands cross at about -6 dB (tested). */
double getBandQ (int bands, double lowHz, double highHz, double sampleRate);

/** The bands (bit k = band k) that get noise for consonants: each whose upper edge reaches 4 kHz, and always the top one. */
int getConsonantBands (int bands, double lowHz, double highHz, double sampleRate);

/** What Formant multiplies the bands' centres by: 2^(semitones / 12). */
double getFormantRatio (double semitones);

/** The frequency of a MIDI note, A4 = 440 Hz. */
double getNoteHz (int midiNote);

/** The name of a pitch class with a sharp: "C", "C♯" ... "B". */
juce::String getPitchName (int pitchClass);

/** The same two lists written into a fixed array, for the audio thread (no allocation); they return how many notes there are. */
using NoteList = std::array<int, maxCarrierVoices>;
int writeChordNotes (int pitchClassMask, int octaves, NoteList& out);
int writeChosenNotes (int root, Shape shape, int octaves, NoteList& out);

/** The notes of a chord as a carrier: each pitch class in the mask at MIDI 48 + class (C3 up), then the same
    notes an octave up for each octave of Range after the first. At most maxCarrierVoices, lowest first. */
std::vector<int> getChordNotes (int pitchClassMask, int octaves);

/** The notes Notes plays: the root from C3 up, in the shape, with the Range octaves. */
std::vector<int> getChosenNotes (int root, Shape shape, int octaves);

/** The pitch classes (as a mask) of a list of notes. */
int getPitchMask (const std::vector<int>& notes);

/** A starting point and why. */
struct Preset
{
    juce::String name, why;
    Settings settings;
};

/** The five starting points; the first, Talking synth, is the default. */
const std::vector<Preset>& getPresets();

/** Seconds of the made-up voice of the test sound (ah, ee, oo, s), and the sound itself at a sample rate: a formant
    synth at 110 Hz gliding to 98 Hz, mid-level. */
double getTestVoiceSeconds();
std::vector<float> makeTestVoice (double sampleRate);

/** What the audio thread heard in the last 8 seconds. */
struct Summary
{
    float inputPeakDb = -100.0f, resultPeakDb = -100.0f;
    double hissFraction = 0.0;              // the share of the time the voice was hissy (s, f, t)
    double chordFallbackFraction = 0.0;     // the share of the time Song chord found no chord and played the chosen notes
    int limiterHits = 0;
};

/** What the song says: the notes of its key (0: no key known) and, when known, its tonic and whether it is minor. */
struct SongKey
{
    int mask = 0;
    int tonic = -1;
    bool minor = false;
};

/** The sentences of "Your track", the first that applies leading: nothing heard, a quiet voice, no chords, a note out of
    the key, dull words, a low top, the limiter, four bands, or all fine. Never empty. */
juce::StringArray getAdvice (const Summary&, const Settings&, const SongKey&);
}
