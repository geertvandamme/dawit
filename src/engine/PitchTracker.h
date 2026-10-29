#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

/**
    Finding the pitch of a voice, for DawIt Tune and for the pitch shown in the wave editor.
    McLeod's method (the normalised square difference), worked out through the FFT so it is quick
    enough to run every few milliseconds on the audio thread.
*/
namespace daw::pitch
{
/** One look at a stretch of audio. */
struct Reading
{
    bool voiced = false;       // a clear pitch: the sound repeats, and is loud enough
    float midi = 0.0f;         // the pitch as a fractional MIDI note (69 = A4 = 440 Hz); only when voiced
    float clarity = 0.0f;      // 0 to 1: how well the sound repeats itself at that pitch
    float rmsDb = -100.0f;     // how loud the stretch is, in dB
};

inline float midiFromHz (double hz) noexcept { return (float) (69.0 + 12.0 * std::log2 (hz / 440.0)); }
inline double hzFromMidi (double midi) noexcept { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }

/** The window length for a sample rate: the first power of two of at least 40 ms (2048 at 44.1 and
    48 kHz, 4096 at 88.2 and 96 kHz). */
int getWindowSize (double sampleRate);

/** The window length for the tuner, which listens for bass strings: the first power of two of at least 85 ms
    (4096 at 44.1 and 48 kHz, 8192 at 88.2 and 96 kHz). With the lowest note at 30 Hz the longest delay is
    under half of it, which the method needs. */
int getTunerWindowSize (double sampleRate);

/** Works out the pitch of a window of audio. Everything is allocated in prepare(), so analyse() is
    safe on the audio thread. */
class Tracker
{
public:
    /** Voiced from this clarity and this loudness (DawIt's choice). Sung vowels score 0.95 and up; two notes a tritone
        apart, the closest thing to a pitch that two notes make, scored 0.915. */
    static constexpr float minClarity = 0.93f;
    static constexpr float minRmsDb = -50.0f;

    /** Sets up for windows of up to `maxWindow` samples at `sampleRate`. `lowestHz` is the lowest pitch looked for
        (50 Hz for a voice; the tuner uses 30 Hz and a longer window to hear a bass). */
    void prepare (double sampleRate, int maxWindow, double lowestHz = 50.0);

    /** The pitch of `numSamples` samples (no more than the prepared window). `minHighest` is how
        clear the best match must be; `guard` turns on the octave guard, which drops a lone reading
        an octave away from the last two (it needs the readings to follow each other). */
    Reading analyse (const float* samples, int numSamples, float minHighest = minClarity, bool guard = false);

    /** Forgets the readings the octave guard compares with. */
    void reset() noexcept { history[0] = history[1] = -1000.0f; }

    /** The match at each delay for the last window analysed, from 0 samples to the longest delay looked at
        (for the picture of how the pitch is found). */
    const std::vector<float>& getMatch() const noexcept { return nsdf; }

private:
    double rate = 44100.0, lowest = 50.0;
    int maxSize = 0, order = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> data, nsdf, squares;
    std::vector<int> peaks;
    float history[2] { -1000.0f, -1000.0f };
};
}
