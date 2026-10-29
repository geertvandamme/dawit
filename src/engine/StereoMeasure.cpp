#include "StereoMeasure.h"
#include <algorithm>
#include <cmath>

namespace daw::stereo
{
MidSide midSide (double left, double right) noexcept { return { (left + right) * 0.5, (left - right) * 0.5 }; }
std::optional<double> correlation (double leftPower, double rightPower, double cross) noexcept
{
    if (std::max (leftPower, rightPower) < 1.0e-20) return std::nullopt;
    if (std::min (leftPower, rightPower) < std::max (leftPower, rightPower) * 1.0e-6) return 0.0;
    return std::clamp (cross / std::sqrt (leftPower * rightPower), -1.0, 1.0);
}
double sideToMidDb (double midPower, double sidePower) noexcept
{
    return std::clamp (10.0 * std::log10 (std::max (sidePower, 1.0e-20) / std::max (midPower, 1.0e-20)), -100.0, 100.0);
}
double highMagnitude (double ratio, int slope) noexcept
{
    const int order = slope == 12 ? 1 : slope == 48 ? 4 : 2;
    const double p = std::pow (std::max (ratio, 0.0), 2 * order);
    return p / (1.0 + p);
}
double widthLevelDb (double midPower, double sidePower, double width) noexcept
{
    return 10.0 * std::log10 (std::max (midPower + width * width * sidePower, 1.0e-20) / std::max (midPower + sidePower, 1.0e-20));
}
double widthCorrelation (double midPower, double sidePower, double width) noexcept
{
    const double side = width * width * sidePower;
    return (midPower - side) / std::max (midPower + side, 1.0e-20);
}
double monoDropDb (double correlationValue) noexcept
{
    return 10.0 * std::log10 (std::max ((1.0 + correlationValue) * 0.5, 1.0e-20));
}
void Measurement::prepare (double sampleRate) noexcept
{
    const double rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    samplesPerBlock = std::max (1, (int) std::lround (rate * 0.05));
    pointInterval = 1;
    coefficient = 1.0 - std::exp (-1.0 / (rate * 0.3));
    samplesInBlock = pointCounter = 0;
    current = {};
    blocksWritten = pointsWritten = 0;
    for (auto& value : smoothed) value = 0.0;
}
void Measurement::push (float left, float right, float lowLeft, float lowRight) noexcept
{
    push (left, right, lowLeft, lowRight, left - lowLeft, right - lowRight);
}
void Measurement::push (float left, float right, float lowLeft, float lowRight, float highLeft, float highRight) noexcept
{
    const auto ms = midSide (left, right), low = midSide (lowLeft, lowRight);
    const std::array<double, 6> powers { (double) left * left, (double) right * right, (double) left * right,
                                         (double) lowLeft * lowLeft, (double) lowRight * lowRight, (double) lowLeft * lowRight };
    for (size_t i = 0; i < powers.size(); ++i)
    {
        const double previous = smoothed[i].load (std::memory_order_relaxed);
        smoothed[i].store (previous + coefficient * (powers[i] - previous), std::memory_order_relaxed);
    }
    current.left += powers[0]; current.right += powers[1]; current.cross += powers[2];
    current.mid += ms.mid * ms.mid; current.side += ms.side * ms.side;
    current.lowLeft += powers[3]; current.lowRight += powers[4]; current.lowCross += powers[5];
    current.lowMid += low.mid * low.mid; current.lowSide += low.side * low.side;
    current.peak = std::max (current.peak, std::max (std::abs (left), std::abs (right)));
    if (++pointCounter >= pointInterval)
    {
        pointCounter = 0;
        const auto index = pointsWritten.load (std::memory_order_relaxed);
        auto& slot = points[(size_t) (index % pointCount)];
        const std::array<float, 6> values { lowLeft, lowRight, highLeft, highRight, left, right };
        for (size_t i = 0; i < values.size(); ++i) slot.values[i].store (values[i], std::memory_order_relaxed);
        pointsWritten.store (index + 1, std::memory_order_release);
    }
    if (++samplesInBlock >= samplesPerBlock)
    {
        const auto index = blocksWritten.load (std::memory_order_relaxed);
        auto& slot = blocks[(size_t) (index % blockCount)];
        const std::array<double, 10> values { current.left, current.right, current.cross, current.mid, current.side,
                                               current.lowLeft, current.lowRight, current.lowCross, current.lowMid, current.lowSide };
        for (size_t i = 0; i < values.size(); ++i) slot.values[i].store (values[i], std::memory_order_relaxed);
        slot.peak.store (current.peak, std::memory_order_relaxed);
        blocksWritten.store (index + 1, std::memory_order_release);
        current = {};
        samplesInBlock = 0;
    }
}
std::optional<double> Measurement::getCorrelation (bool low) const noexcept
{
    const size_t i = low ? 3 : 0;
    return correlation (smoothed[i].load(), smoothed[i + 1].load(), smoothed[i + 2].load());
}
std::vector<Block> Measurement::getBlocks() const
{
    const auto end = blocksWritten.load (std::memory_order_acquire);
    const auto count = std::min (end, (unsigned long long) blockCount);
    std::vector<Block> result;
    result.reserve ((size_t) count);
    for (auto index = end - count; index < end; ++index)
    {
        const auto& slot = blocks[(size_t) (index % blockCount)];
        std::array<double, 10> v {};
        for (size_t i = 0; i < v.size(); ++i) v[i] = slot.values[i].load (std::memory_order_relaxed);
        result.push_back ({ v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], slot.peak.load (std::memory_order_relaxed) });
    }
    return result;
}
std::vector<Point> Measurement::getPoints() const
{
    const auto end = pointsWritten.load (std::memory_order_acquire);
    const auto count = std::min (end, (unsigned long long) pointCount);
    std::vector<Point> result;
    result.reserve ((size_t) count);
    for (auto index = end - count; index < end; ++index)
    {
        const auto& v = points[(size_t) (index % pointCount)].values;
        result.push_back ({ v[0].load(), v[1].load(), v[2].load(), v[3].load(), v[4].load(), v[5].load() });
    }
    return result;
}
} // namespace daw::stereo
