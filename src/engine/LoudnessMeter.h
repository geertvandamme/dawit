#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace daw::loudness
{
/** Measures mono or stereo K-weighted loudness without allocating while processing. */
class Meter final
{
public:
    /** Allocate the sliding windows for a sample rate and clear the measurement. */
    void prepare (double sampleRate);
    /** Clear the measurement without changing the sample rate or allocating. */
    void reset() noexcept;
    /** Add a mono sample, or a stereo pair when channels is two. */
    void push (float left, float right, int channels) noexcept;
    /** Loudness of the last 400 ms, once the window is full. */
    std::optional<double> momentary() const noexcept;
    /** Loudness of the last three seconds, once the window is full. */
    std::optional<double> shortTerm() const noexcept;
    /** Integrated loudness after the absolute and relative gates. */
    std::optional<double> integrated() const noexcept;
    /** Loudness range from gated short-term readings. */
    std::optional<double> range() const noexcept;
private:
    struct Filter
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        double process (double x) noexcept;
    };
    struct Bin { std::uint64_t count = 0; double energy = 0; };
    static constexpr int binCount = 801;
    static std::optional<double> level (double energy) noexcept;
    static void add (std::array<Bin, binCount>& bins, double energy) noexcept;
    std::array<std::array<Filter, 2>, 2> filters {};
    std::vector<double> windowM, windowS;
    std::array<Bin, binCount> integratedBins {}, rangeBins {};
    double sumM = 0, sumS = 0;
    std::size_t posM = 0, posS = 0, filledM = 0, filledS = 0;
    int interval = 1, counter = 0;
};
} // namespace daw::loudness
