#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace daw::tune
{
/** What the shifter is told about the sound: for any input sample, the period of the pitch there and how far to move it. */
class PsolaControls
{
public:
    virtual ~PsolaControls() = default;

    /** The length of one cycle in samples; 0 where there is no clear pitch. */
    virtual float getPeriod (juce::int64 sample) const = 0;

    /** What the frequency is multiplied by: 1 leaves the pitch alone. */
    virtual float getRatio (juce::int64 sample) const = 0;
};

/**
    Moves the pitch of one voice without moving its formants: TD-PSOLA (Moulines and Charpentier,
    1990). The voice is cut into grains two cycles long, one around each cycle's strongest point,
    and the grains are laid down again closer together (higher pitch) or further apart (lower).
    Each grain keeps the shape of its cycle, and with it the colour of the vowel.

    It holds the sound back by `latency` samples, so it can look ahead at the next cycles. Where the
    pitch is not to be moved (no clear pitch, or less than a cent) the output is the held-back input
    bit for bit; a 5 ms crossfade joins the two. The marks come from the middle (left plus right) and
    the same grains are applied to every channel.
*/
class PsolaShifter
{
public:
    static constexpr int maxChannels = 2;

    /** A change of pitch under this factor (a cent) is left alone. */
    static constexpr float minRatioChange = 0.000578f;

    /** `latency`: how long the output is held back (the tracker's window over two, plus its hop). */
    void prepare (double sampleRate, int latency);
    void reset();

    /** The longest cycle it can move: lower voices are passed through (there isn't the look-ahead for them). */
    int getMaxPeriod() const noexcept { return maxPeriod; }
    juce::int64 getInputCount() const noexcept { return count; }

    /** Takes `n` samples of every channel and gives back the `n` samples from `latency` ago, moved as the controls say. */
    void process (const float* const* in, float* const* out, int numChannels, int n, const PsolaControls&);

    /** process() in two halves, for a caller that needs to look at the new input (to find the pitch) before the
        output is made: push the block, then pull the same number of samples. */
    void push (const float* const* in, int numChannels, int n);
    void pull (float* const* out, int numChannels, int n, const PsolaControls&);

    /** The middle (left plus right over two) of the `n` samples before input sample `end`, oldest first. */
    void copyMid (juce::int64 end, float* dest, int n) const noexcept;

    /** The same delay and nothing else, for a bypassed effect. */
    void bypass (const float* const* in, float* const* out, int numChannels, int n);

private:
    bool isWet (juce::int64 sample, const PsolaControls&) const;
    void generateGrains (juce::int64 finalFrom, juce::int64 uEnd, const PsolaControls&);
    void addMarksUntil (juce::int64 limit, const PsolaControls&);
    void emitGrain (juce::int64 sigma, int half, juce::int64 finalFrom, const PsolaControls&);
    float getMid (juce::int64 sample) const noexcept { return mid[(size_t) (sample & mask)]; }

    static constexpr int ringSize = 16384;
    static constexpr juce::int64 mask = ringSize - 1;
    double rate = 44100.0;
    int latency = 1280, maxPeriod = 441, numMarks = 0;
    juce::int64 count = 0, scanFrom = 0;
    bool chainActive = false;
    double nextSynth = 0.0;
    float polarity = 1.0f, lastPeriod = 100.0f, wetWeight = 0.0f, fadeStep = 0.01f;
    juce::int64 marks[128] {};
    std::vector<float> ring[maxChannels], acc[maxChannels], mid, norm;
};
}
