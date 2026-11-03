#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>
#include <optional>
#include <vector>

namespace daw::analysis::rangefinder
{
/** How often the pitch is looked at, and how long a held note must last, how steady it must be, and how
    long a gap of no clear pitch can be bridged. DawIt's own rules (not a standard). */
constexpr double frameSeconds = 0.01;
constexpr double minHeldSeconds = 0.5;
constexpr double heldToleranceCents = 35.0;
constexpr double maxGapSeconds = 0.05;

/** The notes a range can have, as in the singing guide's voice row: C2 (36) to C7 (96), and the fewest semitones
    between the lowest and the highest note for a usable range. */
constexpr int lowestNote = 36, highestNote = 96, minSpan = 5;

/** One note the singer held steady. */
struct HeldNote
{
    int note = 60;              // the nearest MIDI note (60 = C4)
    double start = 0.0;         // seconds from the start of the clip
    double length = 0.0;        // seconds
    double centsOff = 0.0;      // how far the middle of the stretch is from the note, in cents (+ sharp)
};

/** What was found. */
struct Result
{
    enum class Verdict { nothingHeld, tooNarrow, measured };

    Verdict verdict = Verdict::nothingHeld;
    std::vector<float> frames;          // the pitch every frameSeconds as a fractional MIDI note, or -1 for no clear pitch
    std::vector<HeldNote> held;         // in time order
    int lowest = -1, highest = -1;      // the lowest and highest held note, kept within C2 to C7
    int rawLowest = -1, rawHighest = -1;// the same before that
    bool lowestClamped = false, highestClamped = false;
    double firstPitchSeconds = 0.0, lastPitchSeconds = 0.0;   // from the first to the last clear frame, for the picture

    /** The semitones from the lowest to the highest held note (0 with nothing held). */
    int getSpan() const noexcept { return lowest < 0 ? 0 : highest - lowest; }
};

/** Progress from 0 to 1; returning false cancels. */
using Progress = std::function<bool (double)>;

/** Stage 1: the pitch of the audio (mono at its own rate, channels averaged) every 10 ms with DawIt's pitch tracker
    over a 40 ms window and its octave guard. The frame i is the pitch around i * 10 ms. An empty list when cancelled. */
std::vector<float> findFrames (const juce::AudioBuffer<float>&, double sampleRate, const Progress& = {});

/** Stage 2: the held notes in a list of frames (fractional MIDI, below 0 for no clear pitch). Per run of clear frames
    (gaps up to 50 ms bridged) the pitch is smoothed so vibrato averages out, and a held note is a stretch of at least
    0.5 s whose smoothed pitch stays within 35 cents of the stretch's median, as that median rounded to a note. */
Result findHeldNotes (const std::vector<float>& frames, double secondsPerFrame = frameSeconds);

/** Both stages. Nothing (std::nullopt) when `progress` cancelled. */
std::optional<Result> measure (const juce::AudioBuffer<float>&, double sampleRate, const Progress& = {});
} // namespace daw::analysis::rangefinder
