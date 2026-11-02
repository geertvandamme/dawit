#pragma once

#include <complex>

#include <juce_audio_basics/juce_audio_basics.h>

namespace daw
{
/**
    The filter DawIt Subtractive and DawIt Step Filter share: a 2-pole (12 dB per octave) state-variable
    filter that can move every sample, its resonance scale, and the maths of the analog filter it models,
    for the pictures and the tests. A 24 dB slope is two in a row: a flat stage, then the resonant one.
*/
namespace filt
{
    enum class FilterType { lowPass, bandPass, highPass };
    constexpr int numFilterTypes = 3;

    /** The Q with no resonance: the flattest 2-pole response, 3 dB down at the cutoff. */
    constexpr float flatQ = 0.7071f;

    /** The resonance as Q: 0.71 at 0% (no peak) up to 20 at 100%, where it nearly rings on its own. */
    inline float getQ (float resonancePercent)
    {
        return flatQ * std::pow (20.0f / flatQ, juce::jlimit (0.0f, 100.0f, resonancePercent) / 100.0f);
    }

    /** One 2-pole stage of the analog filter, at x = hz / cutoff. */
    inline std::complex<double> getStage (FilterType type, double x, double q)
    {
        const std::complex<double> s (0.0, x);
        const auto denominator = s * s + s / q + 1.0;

        switch (type)
        {
            case FilterType::lowPass:  return 1.0 / denominator;
            case FilterType::bandPass: return (s / q) / denominator;
            case FilterType::highPass: return (s * s) / denominator;
        }

        return 1.0;
    }

    /** The filter's response at `hz`: a 2-pole stage, or for `steep` (24 dB) two in a row, the first without resonance. */
    inline std::complex<double> getFilterResponse (FilterType type, bool steep, double cutoffHz, float resonancePercent, double hz)
    {
        const double x = hz / juce::jmax (1.0, cutoffHz);
        const auto resonant = getStage (type, x, getQ (resonancePercent));
        return steep ? getStage (type, x, flatQ) * resonant : resonant;
    }

    /** How much the filter changes the level at `hz`, in dB. */
    inline double getFilterDb (FilterType type, bool steep, double cutoffHz, float resonancePercent, double hz)
    {
        return 20.0 * std::log10 (juce::jmax (1.0e-10, std::abs (getFilterResponse (type, steep, cutoffHz, resonancePercent, hz))));
    }

    /** The width in Hz of a band-pass's band, between its -3 dB points relative to the peak at the cutoff, for either slope. */
    inline double getBandWidthHz (bool steep, double cutoffHz, float resonancePercent)
    {
        auto db = [&] (double hz) { return 20.0 * std::log10 (juce::jmax (1.0e-10, std::abs (getFilterResponse (FilterType::bandPass, steep, cutoffHz, resonancePercent, hz)))); };
        const double target = db (cutoffHz) - 3.0103;
        double lowEdge = 0.0, highEdge = 0.0;
        double a = cutoffHz * 0.01, b = cutoffHz;   // below the cutoff the level rises with frequency

        for (int i = 0; i < 60; ++i)
        {
            const double m = 0.5 * (a + b);
            (db (m) < target ? a : b) = m;
        }

        lowEdge = 0.5 * (a + b);
        a = cutoffHz;
        b = cutoffHz * 100.0;

        for (int i = 0; i < 60; ++i)
        {
            const double m = 0.5 * (a + b);
            (db (m) < target ? b : a) = m;
        }

        highEdge = 0.5 * (a + b);
        return highEdge - lowEdge;
    }

    /** A 2-pole state-variable filter (topology-preserving transform), cheap enough to move every sample. */
    struct Svf
    {
        double ic1 = 0.0, ic2 = 0.0;

        void reset() noexcept { ic1 = ic2 = 0.0; }

        /** One sample. `g` is tan (pi * cutoff / rate) and `k` is 1 / Q. */
        double run (double x, double g, double k, FilterType type) noexcept
        {
            const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
            const double v3 = x - ic2;
            const double v1 = a1 * ic1 + a2 * v3;
            const double v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0 * v1 - ic1;
            ic2 = 2.0 * v2 - ic2;

            switch (type)
            {
                case FilterType::lowPass:  return v2;
                case FilterType::bandPass: return k * v1;   // a peak of 1 at the cutoff, as the analog band-pass
                case FilterType::highPass: return x - k * v1 - v2;
            }

            return v2;
        }
    };
} // namespace filt
} // namespace daw
