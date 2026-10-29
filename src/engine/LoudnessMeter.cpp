#include "LoudnessMeter.h"
#include <algorithm>
#include <cmath>

namespace daw::loudness
{
namespace
{
constexpr double pi = 3.14159265358979323846;
}

double Meter::Filter::process (double x) noexcept
{
    const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return y;
}

void Meter::prepare (double sampleRate)
{
    sampleRate = std::max (sampleRate, 1000.0);
    const double k = std::tan (pi * 1681.974450955533 / sampleRate);
    const double q = 0.7071752369554196;
    const double vh = std::pow (10.0, 3.999843853973347 / 20.0);
    const double vb = std::pow (vh, 0.4996667741545416);
    const double a0 = 1.0 + k / q + k * k;
    Filter shelf;
    shelf.b0 = (vh + vb * k / q + k * k) / a0;
    shelf.b1 = 2.0 * (k * k - vh) / a0;
    shelf.b2 = (vh - vb * k / q + k * k) / a0;
    shelf.a1 = 2.0 * (k * k - 1.0) / a0;
    shelf.a2 = (1.0 - k / q + k * k) / a0;
    const double h = std::tan (pi * 38.13547087602444 / sampleRate);
    const double h0 = 1.0 + h / 0.5003270373238773 + h * h;
    Filter high;
    high.b0 = 1.0; high.b1 = -2.0; high.b2 = 1.0;
    high.a1 = 2.0 * (h * h - 1.0) / h0;
    high.a2 = (1.0 - h / 0.5003270373238773 + h * h) / h0;
    for (auto& channel : filters) channel = { shelf, high };
    windowM.resize ((std::size_t) std::max (1, (int) std::lround (sampleRate * 0.4)));
    windowS.resize ((std::size_t) std::max (1, (int) std::lround (sampleRate * 3.0)));
    interval = std::max (1, (int) std::lround (sampleRate * 0.1));
    reset();
}

void Meter::reset() noexcept
{
    for (auto& channel : filters)
        for (auto& filter : channel) filter.x1 = filter.x2 = filter.y1 = filter.y2 = 0;
    std::fill (windowM.begin(), windowM.end(), 0.0);
    std::fill (windowS.begin(), windowS.end(), 0.0);
    integratedBins = {}; rangeBins = {};
    sumM = sumS = 0; posM = posS = filledM = filledS = 0; counter = 0;
}

std::optional<double> Meter::level (double energy) noexcept
{
    if (energy <= 0.0) return {};
    return -0.691 + 10.0 * std::log10 (energy);
}
void Meter::add (std::array<Bin, binCount>& bins, double energy) noexcept
{
    const auto value = level (energy);
    if (! value || *value < -70.0) return;
    const int index = std::clamp ((int) ((*value + 70.0) * 10.0), 0, binCount - 1);
    ++bins[(std::size_t) index].count;
    bins[(std::size_t) index].energy += energy;
}
void Meter::push (float left, float right, int channels) noexcept
{
    if (windowM.empty()) return;
    double energy = 0;
    for (int ch = 0; ch < std::clamp (channels, 1, 2); ++ch)
    {
        auto& f = filters[(std::size_t) ch];
        const double value = f[1].process (f[0].process (ch == 0 ? left : right));
        energy += value * value;
    }
    sumM += energy - windowM[posM]; windowM[posM] = energy; posM = (posM + 1) % windowM.size();
    sumS += energy - windowS[posS]; windowS[posS] = energy; posS = (posS + 1) % windowS.size();
    filledM = std::min (filledM + 1, windowM.size()); filledS = std::min (filledS + 1, windowS.size());
    if (++counter >= interval)
    {
        counter = 0;
        if (filledM == windowM.size()) add (integratedBins, sumM / (double) windowM.size());
        if (filledS == windowS.size()) add (rangeBins, sumS / (double) windowS.size());
    }
}
std::optional<double> Meter::momentary() const noexcept { return filledM == windowM.size() && filledM != 0 ? level (sumM / (double) filledM) : std::nullopt; }
std::optional<double> Meter::shortTerm() const noexcept { return filledS == windowS.size() && filledS != 0 ? level (sumS / (double) filledS) : std::nullopt; }
std::optional<double> Meter::integrated() const noexcept
{
    double energy = 0; std::uint64_t count = 0;
    for (const auto& bin : integratedBins) { energy += bin.energy; count += bin.count; }
    if (count == 0) return {};
    const double gate = level (energy / (double) count).value_or (-70.0) - 10.0;
    energy = 0; count = 0;
    for (std::size_t i = 0; i < integratedBins.size(); ++i)
        if (-70.0 + (double) i * 0.1 >= gate) { energy += integratedBins[i].energy; count += integratedBins[i].count; }
    return count != 0 ? level (energy / (double) count) : std::nullopt;
}
std::optional<double> Meter::range() const noexcept
{
    double energy = 0; std::uint64_t count = 0;
    for (const auto& bin : rangeBins) { energy += bin.energy; count += bin.count; }
    if (count == 0) return {};
    const double gate = level (energy / (double) count).value_or (-70.0) - 20.0;
    count = 0;
    for (std::size_t i = 0; i < rangeBins.size(); ++i)
        if (-70.0 + (double) i * 0.1 >= gate) count += rangeBins[i].count;
    if (count == 0) return {};
    std::uint64_t seen = 0; double low = 0, high = 0; bool found = false;
    for (std::size_t i = 0; i < rangeBins.size(); ++i)
    {
        const double value = -70.0 + (double) i * 0.1;
        if (value < gate) continue;
        seen += rangeBins[i].count;
        if (! found && (double) seen >= 0.1 * (double) count) { low = value; found = true; }
        if ((double) seen >= 0.95 * (double) count) { high = value; break; }
    }
    return high - low;
}
} // namespace daw::loudness
