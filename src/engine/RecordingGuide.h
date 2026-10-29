#pragma once

#include <vector>
#include <juce_core/juce_core.h>

namespace daw
{
/**
    The "How recording works" guide: what happens between a voice and a take, the audio interface and its three
    kinds of signal, choosing an input, setting the level, buffers and latency, hearing yourself, why takes are
    moved earlier, takes on a loop, and a look at the user's own device and last take. The content only: the
    window is RecordingExplainer. The formulas (buffer length, round trip, distance, ranges, voltages, file size)
    and the advice rules are checked in the tests.
*/
namespace recordingguide
{
    enum class Topic { air, audioInterface, chooseInput, level, buffers, monitoring, moved, takes, yourSession };
    inline constexpr Topic allTopics[] { Topic::air, Topic::audioInterface, Topic::chooseInput, Topic::level, Topic::buffers,
                                         Topic::monitoring, Topic::moved, Topic::takes, Topic::yourSession };

    juce::String getTopicName (Topic);
    juce::String getTopicSummary (Topic);

    /** The general text of a topic. For "your session" this is the part that doesn't depend on the device. */
    juce::String explain (Topic);

    // ---- The formulas -----------------------------------------------------------------------------------------

    /** How long one buffer lasts: samples / rate, in ms. 128 at 48 kHz: 2.667 ms. */
    double getBufferMs (int bufferSamples, double sampleRate);

    /** The least a sound can be delayed going in and out: two buffers, in ms. 128 at 48 kHz: 5.333 ms. */
    double getMinimumRoundTripMs (int bufferSamples, double sampleRate);

    /** How far sound travels in `ms` milliseconds in air (343 m/s), in metres. 5.333 ms: 1.83 m. */
    double getDistanceMetres (double ms);

    /** The range between a converter's smallest step and its top: 20 log10 (2^bits), in dB. 24 bits: 144.5, 16 bits: 96.3. */
    double getBitRangeDb (int bits);

    /** Volts of a professional line level in dBu: 0.775 x 10^(dBu / 20). +4 dBu: 1.228 V. */
    double getVoltsFromDbu (double dbu);

    /** Volts of a consumer line level in dBV: 10^(dBV / 20). -10 dBV: 0.316 V. */
    double getVoltsFromDbv (double dbv);

    /** The gain between two voltages, in dB: 20 log10 (high / low). 2.5 mV to 1.228 V: 53.8 dB. */
    double getGainDb (double lowVolts, double highVolts);

    /** Bytes a minute of a WAV file takes: rate x bits / 8 x channels x 60. 48 kHz, 24 bits, mono: 8,640,000. */
    double getBytesPerMinute (double sampleRate, int bits, int numChannels);

    /** "8.64 MB" (a million bytes). */
    juce::String describeBytes (double bytes);

    // ---- Example data for the pictures ------------------------------------------------------------------------

    /** One buffer size in the latency table. */
    struct BufferRow
    {
        int samples = 0;
        double bufferMs = 0.0;       // one buffer
        double roundTripMs = 0.0;    // two buffers plus the extra
    };

    /** The sizes 64, 128, 256, 512 and 1024 at `sampleRate`; the round trip is two buffers plus `extraMs`, what the
        device reports beyond its buffers (converters, driver) and what was measured. */
    std::vector<BufferRow> getBufferTable (double sampleRate, double extraMs);

    /** One signal strength of the interface picture. */
    struct SignalLevel
    {
        juce::String name;
        double volts = 0.0;
        bool hasValue = true;        // the instrument level has none: it differs for every guitar
    };

    /** Microphone about 2.5 mV, instrument (no figure), home line -10 dBV and studio line +4 dBu, weakest first. */
    std::vector<SignalLevel> getSignalLevels();

    /** One sung phrase of the level picture. */
    struct LevelExample
    {
        juce::String label;
        double peakDb = 0.0;         // of the take as it ends up in the file
        double amplitude = 0.0;      // the wave's peak before the converter, linear
        double drive = 1.0;          // the wave is multiplied by this and then cut at +-1: above 1 it clips
        bool clipped = false;
    };

    /** The phrase at -36 dB (too quiet), -12 dB (good) and clipped flat at 0 dB (amplitudes 0.0158, 0.251, 1.0 x 2). */
    std::vector<LevelExample> getLevelExamples();

    /** The phrase's wave, `numPoints` samples from -1 to 1: a swell of 3 pitched peaks, scaled and, when it clips, cut at +-1. */
    std::vector<float> getPhrase (const LevelExample&, int numPoints);

    /** One take of the takes picture, in bars as the ruler shows them. */
    struct TakeBar
    {
        int pass = 1;
        double startBar = 9.0, endBar = 13.0;   // the take covers [startBar, endBar)
        bool cutShort = false;                  // stopped before the loop end
        bool playing = false;                   // the newest complete pass
    };

    /** The loop is bars 9 to 12 (it ends where bar 13 starts): pass 1 from bar 10, passes 2 and 3 whole, pass 4 cut at bar 11; pass 3 plays. */
    std::vector<TakeBar> getTakesExample();
    inline constexpr int exampleLoopStartBar = 9, exampleLoopEndBar = 13;

    // ---- Your session ----------------------------------------------------------------------------------------

    /** An armed audio track. */
    struct ArmedTrack
    {
        juce::String name;
        int inputChannel = 0;        // 0-based first channel
        bool stereo = false;
        bool monitor = false;
        float inputPeakDb = -100.0f; // the live input level
    };

    /** What the "your session" topic reads: the device, its latency, what is armed and the last take. */
    struct Snapshot
    {
        juce::String inputDevice, outputDevice;   // empty: none
        double sampleRate = 0.0;
        int bufferSamples = 0;
        int numInputs = 0;                         // input channels the device has
        double reportedInputMs = 0.0, reportedOutputMs = 0.0;
        bool measured = false;                     // Measure latency has been run for this pair of devices
        double measuredExtraMs = 0.0;              // what it found beyond the reported figures, either sign
        std::vector<ArmedTrack> armed;
        bool inputAllZero = false;                 // exactly zero for a second while armed
        bool hasLastTake = false;
        juce::String lastTakeName;
        double lastTakePeakDb = -100.0;
    };

    /** "Input 1 (mono)" or "Inputs 1+2 (stereo)" for a 0-based first channel. */
    juce::String describeInput (int firstChannel, bool stereo);

    /** The round trip the takes are corrected by: reported in and out plus the measured extra, in ms. */
    double getRoundTripMs (const Snapshot&);

    /** One line for the settings panel: "Round trip 6.4 ms: 3.1 in, 3.3 out, reported" or, once measured,
        "Round trip 7.9 ms: 3.1 in, 3.3 out, measured, 1.5 ms more than reported" ("less than" for a negative extra). */
    juce::String describeLatency (const Snapshot&);

    /** The snapshot as plain lines: devices, rate and buffer, round trip, armed tracks, last take. */
    juce::String describeSession (const Snapshot&);

    /** The message for exactly-zero input while armed (the macOS microphone permission). */
    juce::String getZeroInputMessage();

    /** What to try, in the order of the rules; empty when all is well. */
    struct Advice
    {
        juce::String text;
        Topic topic = Topic::yourSession;    // the topic that explains it
    };

    /** Where each rule starts to speak, as in the plan. */
    inline constexpr double slowRoundTripMs = 12.0;      // with Monitor on
    inline constexpr int largeBufferSamples = 512;       // with a track armed
    inline constexpr double clippedPeakDb = -0.1;
    inline constexpr double closePeakDb = -6.0;
    inline constexpr double quietPeakDb = -24.0;
    inline constexpr double measuredNoteMs = 1.0;        // the measured extra is mentioned from this size

    std::vector<Advice> analyse (const Snapshot&);
} // namespace recordingguide
} // namespace daw
