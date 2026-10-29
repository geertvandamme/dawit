#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

namespace daw::tune
{
/**
    A made-up sung "ah": harmonics of the pitch under the resonances of an "ah" (about 730 and
    1,090 Hz, then 2,440 Hz), so the vowel stays where it is whatever note it is sung on. For the
    tests, the test sound and the example song's sung take.
*/
class SungVoice
{
public:
    void prepare (double sampleRate)
    {
        rate = sampleRate > 0.0 ? sampleRate : 44100.0;
        phase = 0.0;
        counter = 0;
    }

    /** The next sample, for a fractional MIDI pitch (69 = 440 Hz). */
    float next (double midi) noexcept
    {
        const double hz = 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);

        if ((counter++ & 31) == 0)
        {
            const int count = juce::jlimit (1, maxHarmonics, (int) (4200.0 / hz));
            numHarmonics = count;

            for (int n = 1; n <= count; ++n)
                amplitude[(size_t) n - 1] = (float) (getEnvelope (n * hz) / n);
        }

        phase += hz / rate;
        phase -= std::floor (phase);
        double sum = 0.0;

        for (int n = 1; n <= numHarmonics; ++n)
            sum += amplitude[(size_t) n - 1] * std::sin (juce::MathConstants<double>::twoPi * n * phase);

        return (float) (sum * 0.45);
    }

    /** How strong the vowel's resonances make a frequency, around 1 at the first one. */
    static double getEnvelope (double hz) noexcept
    {
        const auto peak = [hz] (double centre, double width, double gain) { const double d = (hz - centre) / width; return gain / (1.0 + d * d); };
        return 0.08 + peak (730.0, 90.0, 1.0) + peak (1090.0, 110.0, 0.8) + peak (2440.0, 200.0, 0.25);
    }

private:
    static constexpr int maxHarmonics = 96;
    double rate = 44100.0, phase = 0.0;
    int counter = 0, numHarmonics = 1;
    std::array<float, maxHarmonics> amplitude {};
};
}
