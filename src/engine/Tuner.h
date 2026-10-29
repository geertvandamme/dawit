#pragma once

#include <array>
#include <atomic>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

namespace daw::pitch { class Tracker; }   // kept out of this header: Engine.h includes it, and juce_dsp is heavy

namespace daw::tuner
{
/**
    The tuner's way in for sound. While the tuner listens, the audio thread copies one input channel of the device (or
    the mono sum of a stereo pair) into a lock-free FIFO; the tuner window drains it on the message thread. It holds
    no more than about three seconds, and drops what doesn't fit rather than wait.
*/
class InputTap
{
public:
    InputTap();

    /** Starts or stops copying. Starting empties the FIFO. Message thread. */
    void setListening (bool on);
    bool isListening() const noexcept { return listening.load(); }

    /** Which device input to copy: the first channel (0-based), and whether it and the next one are a stereo pair (summed to mono). */
    void setInput (int firstChannel, bool stereo) noexcept;
    int getChannel() const noexcept { return channel.load(); }
    bool isStereo() const noexcept { return stereo.load(); }

    /** Copies the chosen input of one block into the FIFO. Audio thread; does nothing when not listening. */
    void push (const float* const* inputs, int numInputs, int numSamples) noexcept;

    /** Takes up to `maxSamples` samples out of the FIFO and returns how many. Message thread. */
    int pull (float* destination, int maxSamples) noexcept;

private:
    static constexpr int capacity = 1 << 17;
    juce::AbstractFifo fifo { capacity };
    std::vector<float> storage;
    std::vector<float> mix;                  // audio thread: one block, mixed to mono
    std::atomic<bool> listening { false }, stereo { false };
    std::atomic<int> channel { 0 };
    std::atomic<int> clearRequest { 0 };
};

/** What the listener makes of the sound: the note, and why there is none. */
struct Reading
{
    bool hasNote = false;       // a pitch heard now, or held for up to 2 s after the sound ended
    double midi = 0.0;          // the pitch as a fractional MIDI note (69 = A4 = 440 Hz at 440 Hz); the median of the last 5 readings
    double hz = 0.0;            // the same in hertz, at A4 = 440 Hz
    double ageSeconds = 0.0;    // 0 while the note sounds; how long ago it ended when held
    float fade = 1.0f;          // 1 while sounding and for the first 0.5 s of the hold, then falling to 0 at 2 s
    float levelDb = -100.0f;    // loudness of the last window, in dB
    bool tooQuiet = true;       // below -50 dB
    bool exactlyZero = false;   // a second of samples that are exactly zero: nothing plugged in, or the microphone refused
};

/**
    Finds the note in what the tap delivers. A plain class with no UI: feed() takes samples (from the tap, or straight
    from a test), looks at the newest 85 ms about every 21 ms, and keeps a smoothed reading: the median of the last 5
    voiced readings, so a single slip never moves the needle. A note stays 2 s after the sound ends. Everything is
    allocated in prepare().
*/
class Listener
{
public:
    static constexpr double holdSeconds = 2.0;
    static constexpr double lowestHz = 30.0;

    Listener();
    ~Listener();

    /** Sets up for a sample rate and forgets everything heard. Message thread. */
    void prepare (double sampleRate);

    /** Adds samples (any amount) and updates the reading. */
    void feed (const float* samples, int numSamples);

    /** Takes what the tap has and feeds it. Returns how many samples there were. */
    int service (InputTap&);

    /** Forgets the note and the level. */
    void reset();

    const Reading& getReading() const noexcept { return reading; }
    double getSampleRate() const noexcept { return rate; }

private:
    void analyseWindow();
    void noteBlockLevel();
    bool isBeating() const;

    double rate = 0.0;
    int windowSize = 0, hop = 1024, sinceAnalysis = 0, filled = 0, writePos = 0;
    long long samplesSinceVoiced = 0, zeroRun = 0;
    std::vector<float> ring, ordered;
    std::vector<float> pullBuffer;
    std::unique_ptr<pitch::Tracker> tracker;
    std::array<float, 5> recent {};
    int numRecent = 0, nextRecent = 0;
    std::array<float, 16> blockDb {};   // loudness of the last 16 hops, oldest first once full
    int numBlocks = 0, nextBlock = 0;
    Reading reading;
};
}
