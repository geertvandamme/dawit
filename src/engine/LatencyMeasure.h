#pragma once

#include <vector>
#include <juce_core/juce_core.h>

namespace daw
{
/**
    Measuring the real round-trip delay of an audio interface: DawIt plays eight clicks and listens for them
    coming back (through a cable from an output to an input, or from a speaker to a microphone). This holds the
    parts that need no device: the click, where each one is played, and the finder that reads the delay from
    what came back. Engine::startLatencyMeasurement runs it on the audio thread.
*/
namespace latency
{
    inline constexpr int numClicks = 8;
    inline constexpr double leadSeconds = 0.2;       // quiet before the first click
    inline constexpr double spacingSeconds = 0.25;   // between clicks; the longest delay that can be measured
    inline constexpr double tailSeconds = 0.1;       // listening after the last window
    inline constexpr float clickLevelDb = -12.0f;    // the clicks' peak, as in the plan: clear but never clipping
    inline constexpr int minClicksFound = 5;         // clicks that must agree for a result (4 of 8 is a failure)
    inline constexpr double agreeSeconds = 0.002;    // clicks whose delays are within this of the middle one count

    /** One click: a short, decaying burst whose first sample is the loudest, so its start is easy to find. */
    std::vector<float> makeClick (double sampleRate);

    /** The sample, counted from the start of the measuring, where click `index` (0 to 7) begins. */
    int getClickStart (int index, double sampleRate);

    /** How many samples the measuring lasts, clicks and listening. */
    int getTotalSamples (double sampleRate);

    /** What the finder found. */
    struct Result
    {
        bool ok = false;                 // enough clicks agreed
        int clicksFound = 0;             // clicks heard at about the same delay
        double roundTripSamples = 0.0;   // from a click played to the same click heard (the middle of those that agree)
    };

    /** Reads the round trip from what was heard (one channel, from the first sample of the measuring). For each
        click it looks in the quarter second after it was played for the first sample above a quarter of that
        window's peak; windows quieter than -54 dB, or than a fifth of the loudest window, don't count. */
    Result findRoundTrip (const std::vector<float>& heard, double sampleRate);
} // namespace latency
} // namespace daw
