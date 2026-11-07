#include "LoudnessMeter.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace daw::loudness
{
namespace
{
constexpr double pi = 3.14159265358979323846;

double toDb (double linear) { return 20.0 * std::log10 (linear); }

/** The 3.0 dB number with one decimal, and "-" for nothing. */
juce::String oneDecimal (double v) { return juce::String (std::round (v * 10.0) / 10.0, 1); }
/** A target: "-14" when whole, "-12.5" otherwise. */
juce::String shortNumber (double v) { return juce::String (v, std::abs (v - std::round (v)) < 0.05 ? 0 : 1); }
}

//==============================================================================
KFilter designKFilter (double sampleRate)
{
    sampleRate = std::max (sampleRate, 1000.0);
    // Stage parameters from libebur128; at 48 kHz they reproduce BS.1770's Tables 1 and 2.
    const double k = std::tan (pi * 1681.974450955533 / sampleRate);
    const double q = 0.7071752369554196;
    const double vh = std::pow (10.0, 3.999843853973347 / 20.0);
    const double vb = std::pow (vh, 0.4996667741545416);
    const double a0 = 1.0 + k / q + k * k;
    KFilter result;
    result.shelf.b0 = (vh + vb * k / q + k * k) / a0;
    result.shelf.b1 = 2.0 * (k * k - vh) / a0;
    result.shelf.b2 = (vh - vb * k / q + k * k) / a0;
    result.shelf.a1 = 2.0 * (k * k - 1.0) / a0;
    result.shelf.a2 = (1.0 - k / q + k * k) / a0;
    const double h = std::tan (pi * 38.13547087602444 / sampleRate);
    const double h0 = 1.0 + h / 0.5003270373238773 + h * h;
    result.highPass.b0 = 1.0; result.highPass.b1 = -2.0; result.highPass.b2 = 1.0;
    result.highPass.a1 = 2.0 * (h * h - 1.0) / h0;
    result.highPass.a2 = (1.0 - h / 0.5003270373238773 + h * h) / h0;
    return result;
}

double kResponseDb (double frequencyHz, double sampleRate)
{
    const auto magnitude = [&] (const Biquad& c)
    {
        const std::complex<double> z1 = std::polar (1.0, -2.0 * pi * frequencyHz / sampleRate), z2 = z1 * z1;
        return std::abs (c.b0 + c.b1 * z1 + c.b2 * z2) / std::abs (1.0 + c.a1 * z1 + c.a2 * z2);
    };
    const auto k = designKFilter (sampleRate);
    return toDb (magnitude (k.shelf) * magnitude (k.highPass));
}

double gainToTargetDb (double integratedLufs, double targetLufs) noexcept { return targetLufs - integratedLufs; }

double streamingGainDb (double integratedLufs, double truePeakDbtp, double targetLufs, double peakLimitDbtp) noexcept
{
    if (integratedLufs >= targetLufs)
        return targetLufs - integratedLufs;
    return std::max (0.0, std::min (targetLufs - integratedLufs, peakLimitDbtp - truePeakDbtp));
}

//==============================================================================
double Meter::Filter::process (double x) noexcept
{
    const double y = c.b0 * x + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return y;
}

void Meter::prepare (double sampleRate)
{
    rate = std::max (sampleRate, 1000.0);
    const auto k = designKFilter (rate);
    Filter shelf, high;
    shelf.c = k.shelf; high.c = k.highPass;
    for (auto& channel : filters) channel = { shelf, high };
    windowM.assign ((std::size_t) std::max (1, (int) std::lround (rate * 0.4)), 0.0);
    windowS.assign ((std::size_t) std::max (1, (int) std::lround (rate * 3.0)), 0.0);
    windowR.assign (windowS.size(), 0.0);
    interval = std::max (1, (int) std::lround (rate * 0.1));

    // The 4x oversampler: a Kaiser-windowed sinc, 12 taps in each of the three phases between samples (48 in all, like
    // BS.1770 Annex 2), each phase scaled to unity gain at DC. The phase p reads the point p/4 after the sample 6 back.
    const auto bessel = [] (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k2 = 1; k2 < 30; ++k2) { term *= (x / (2.0 * k2)) * (x / (2.0 * k2)); sum += term; }
        return sum;
    };
    constexpr double beta = 7.0, halfWidth = 6.25;
    for (int p = 0; p < 3; ++p)
    {
        double total = 0;
        for (int i = 0; i < tapsPerPhase; ++i)
        {
            const double d = (double) i - 6.0 + 0.25 * (double) (p + 1);
            const double sinc = std::abs (d) < 1.0e-12 ? 1.0 : std::sin (pi * d) / (pi * d);
            const double ratio = d / halfWidth;
            phases[(std::size_t) p][(std::size_t) i] = sinc * bessel (beta * std::sqrt (std::max (0.0, 1.0 - ratio * ratio))) / bessel (beta);
            total += phases[(std::size_t) p][(std::size_t) i];
        }
        for (auto& c : phases[(std::size_t) p]) c /= total;
    }
    reset();
}

void Meter::reset() noexcept
{
    for (auto& channel : filters)
        for (auto& filter : channel) filter.x1 = filter.x2 = filter.y1 = filter.y2 = 0;
    std::fill (windowM.begin(), windowM.end(), 0.0);
    std::fill (windowS.begin(), windowS.end(), 0.0);
    std::fill (windowR.begin(), windowR.end(), 0.0);
    integratedBins = {}; rangeBins = {};
    sumM = sumS = sumR = 0; posM = posS = posR = filledM = filledS = filledR = 0; counter = 0;
    countedSamples = 0; maxM = maxS = 0; hasMaxM = hasMaxS = false;
    samplePeakMax = truePeakMax = blockPeak = 0; peakRing.fill (0.0); peakPos = 0;
    for (auto& channel : history) channel.fill (0.0f);
    historyPos = 0;
    channelsSeen = 1;
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

double Meter::sumOf (const std::vector<double>& window) const noexcept
{
    double sum = 0;
    for (const double v : window) sum += v;
    return sum;
}

void Meter::push (float left, float right, int channels, bool counting) noexcept
{
    if (windowM.empty()) return;
    const int count = std::clamp (channels, 1, 2);
    channelsSeen = std::max (channelsSeen, count);
    double energy = 0, raw = 0, peak = 0, interpolated = 0;
    historyPos = (historyPos + tapsPerPhase - 1) % tapsPerPhase;   // newest sample at historyPos, older ones after it
    for (int ch = 0; ch < count; ++ch)
    {
        const float x = ch == 0 ? left : right;
        auto& f = filters[(std::size_t) ch];
        const double value = f[1].process (f[0].process (x));
        energy += value * value;
        raw += (double) x * x;
        peak = std::max (peak, (double) std::abs (x));
        auto& h = history[(std::size_t) ch];
        h[(std::size_t) historyPos] = h[(std::size_t) historyPos + tapsPerPhase] = x;
        for (const auto& phase : phases)
        {
            double y = 0;
            for (int i = 0; i < tapsPerPhase; ++i) y += phase[(std::size_t) i] * (double) h[(std::size_t) (historyPos + i)];
            interpolated = std::max (interpolated, std::abs (y));
        }
    }
    sumM += energy - windowM[posM]; windowM[posM] = energy; if (++posM == windowM.size()) { posM = 0; sumM = sumOf (windowM); }
    sumS += energy - windowS[posS]; windowS[posS] = energy; if (++posS == windowS.size()) { posS = 0; sumS = sumOf (windowS); }
    sumR += raw - windowR[posR]; windowR[posR] = raw; if (++posR == windowR.size()) { posR = 0; sumR = sumOf (windowR); }
    filledM = std::min (filledM + 1, windowM.size()); filledS = std::min (filledS + 1, windowS.size()); filledR = std::min (filledR + 1, windowR.size());
    blockPeak = std::max (blockPeak, peak);
    if (counting)
    {
        ++countedSamples;
        samplePeakMax = std::max (samplePeakMax, peak);
        truePeakMax = std::max ({ truePeakMax, peak, interpolated });
    }
    if (++counter >= interval)
    {
        counter = 0;
        peakRing[(std::size_t) peakPos] = blockPeak; blockPeak = 0; peakPos = (peakPos + 1) % peakBlocks;
        if (counting)
        {
            if (filledM == windowM.size())
            {
                const double e = sumM / (double) windowM.size();
                add (integratedBins, e);
                if (! hasMaxM || e > maxM) { maxM = e; hasMaxM = true; }
            }
            if (filledS == windowS.size())
            {
                const double e = sumS / (double) windowS.size();
                add (rangeBins, e);
                if (! hasMaxS || e > maxS) { maxS = e; hasMaxS = true; }
            }
        }
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
        if (-70.0 + (double) i * 0.1 >= gate - 1.0e-9) { energy += integratedBins[i].energy; count += integratedBins[i].count; }
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
        if (-70.0 + (double) i * 0.1 >= gate - 1.0e-9) count += rangeBins[i].count;
    if (count == 0) return {};
    std::uint64_t seen = 0; double low = 0, high = 0; bool found = false;
    for (std::size_t i = 0; i < rangeBins.size(); ++i)
    {
        const double value = -70.0 + (double) i * 0.1;
        if (value < gate - 1.0e-9) continue;
        seen += rangeBins[i].count;
        if (! found && (double) seen >= 0.1 * (double) count) { low = value; found = true; }
        if ((double) seen >= 0.95 * (double) count) { high = value; break; }
    }
    return high - low;
}

std::optional<double> Meter::truePeakDb() const noexcept { return truePeakMax > 0.0 ? std::optional<double> (toDb (truePeakMax)) : std::nullopt; }
std::optional<double> Meter::samplePeakDb() const noexcept { return samplePeakMax > 0.0 ? std::optional<double> (toDb (samplePeakMax)) : std::nullopt; }

std::optional<double> Meter::rmsDb() const noexcept
{
    if (filledR == 0 || sumR <= 1.0e-20) return {};
    return 10.0 * std::log10 (sumR / ((double) filledR * channelsSeen));
}

std::optional<double> Meter::crestDb() const noexcept
{
    const auto rms = rmsDb();
    double peak = blockPeak;
    for (const double p : peakRing) peak = std::max (peak, p);
    if (! rms || peak <= 0.0) return {};
    return toDb (peak) - *rms;
}

std::optional<double> Meter::maxMomentary() const noexcept { return hasMaxM ? level (maxM) : std::nullopt; }
std::optional<double> Meter::maxShortTerm() const noexcept { return hasMaxS ? level (maxS) : std::nullopt; }
double Meter::measuredSeconds() const noexcept { return (double) countedSamples / rate; }

bool Meter::hasReading() const noexcept
{
    for (const auto& bin : integratedBins) if (bin.count != 0) return true;
    return false;
}

Readings Meter::getReadings() const noexcept
{
    Readings r;
    r.momentary = momentary(); r.shortTerm = shortTerm(); r.integrated = integrated(); r.range = range();
    r.maxMomentary = maxMomentary(); r.maxShortTerm = maxShortTerm();
    r.truePeak = truePeakDb(); r.samplePeak = samplePeakDb(); r.rms = rmsDb(); r.crest = crestDb();
    r.seconds = measuredSeconds();
    return r;
}

//==============================================================================
juce::String formatClock (double seconds)
{
    const int total = (int) std::floor (std::max (0.0, seconds) + 0.5);
    return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
}

juce::StringArray advise (const AdviceInput& in)
{
    juce::StringArray lines;
    const auto& r = in.readings;
    if (in.scope.isNotEmpty()) lines.add (in.scope);

    // 1. Nothing above -70 LUFS.
    if (! r.integrated)
    {
        lines.add ("Nothing measured yet. Play the song: DawIt Meter measures what passes through it.");
        return lines;
    }

    const double I = *r.integrated, T = in.target, C = in.ceiling;
    const double tp = r.truePeak.value_or (-200.0), sp = r.samplePeak.value_or (-200.0);
    const bool showTarget = in.showTarget && in.kind != TargetKind::mixing;
    const bool short30 = r.seconds < 30.0;

    // 2. Too short to be the song.
    if (short30)
        lines.add ("Only " + formatClock (r.seconds) + " measured. For the song's integrated loudness, measure the whole song, from the start to the end.");

    // 3. On one track, or on a bus that may not hold the whole mix.
    if (in.host == HostKind::track || in.host == HostKind::returnTrack)
    {
        lines.add ("DawIt Meter is on the " + juce::String (in.host == HostKind::track ? "track " : "return ") + in.hostName
                   + ", not on the whole mix, so a target doesn't apply here. Short-term now " + (r.shortTerm ? oneDecimal (*r.shortTerm) : juce::String ("-"))
                   + " LUFS, loudest 3 seconds " + (r.maxShortTerm ? oneDecimal (*r.maxShortTerm) : juce::String ("-"))
                   + " LUFS. Use it to compare the track with an effect on and off at the same loudness.");
        return lines;
    }
    if (in.host == HostKind::bus)
        lines.add ("If the bus " + in.hostName + " holds the whole mix:");

    const bool peakFlat = ! short30 && r.samplePeak && sp - I < 8.0;
    const auto peakText = [&] { return "The peaks are only " + oneDecimal (sp - I) + " dB above the integrated loudness. Masters this dense often sound flat next to dynamic ones once they're normalised. For more punch, let the limiter do less."; };

    // 4. No target while mixing.
    if (! showTarget)
    {
        const double headroom = -tp;
        lines.add ("No target while mixing. Integrated " + oneDecimal (I) + " LUFS, true peak " + (tp >= 0.0 ? "+" : juce::String()) + oneDecimal (tp) + " dBTP: "
                   + (headroom >= 0.0 ? oneDecimal (headroom) + " dB of headroom." : "over 0 dBTP by " + oneDecimal (-headroom) + " dB, so it can clip once exported or played. Turn the mix down."));
        if (peakFlat) lines.add (peakText());
        return lines;
    }

    const auto targetName = [&]() -> juce::String
    {
        switch (in.kind)
        {
            case TargetKind::streaming: return "Streaming";
            case TargetKind::appleMusic: return "Apple Music";
            case TargetKind::podcast: return "Podcast";
            case TargetKind::ebu: return "European broadcast";
            case TargetKind::atsc: return "US broadcast";
            case TargetKind::club: return "Club";
            case TargetKind::mixing: case TargetKind::own: break;
        }
        return "your target of " + shortNumber (T) + " LUFS";
    };
    const auto targetNumber = shortNumber (T);
    const int before = lines.size();
    bool issue = false;

    // 5. True peak over the ceiling.
    if (r.truePeak && tp > C)
    {
        issue = true;
        lines.add ("The true peak reaches " + (tp >= 0.0 ? "+" : juce::String()) + oneDecimal (tp) + " dBTP, above the " + oneDecimal (C) + " dBTP ceiling for "
                   + (in.kind == TargetKind::own ? juce::String ("your target") : targetName())
                   + ". Encoding to AAC or Ogg can make it clip. Set your limiter's ceiling to " + oneDecimal (C)
                   + " dB with true-peak detection, or lower the level going into it.");
    }

    // 6. Integrated loudness against the target.
    const double gap = I - T;
    if (std::abs (gap) <= 1.0)
        lines.add ("Integrated " + oneDecimal (I) + " LUFS: within 1 LU of the " + targetNumber + " LUFS target, so it plays almost unchanged.");
    else if (gap > 0.0)
    {
        issue = true;
        const bool normalised = in.kind == TargetKind::streaming || in.kind == TargetKind::appleMusic || in.kind == TargetKind::own;
        if (normalised)
        {
            const juce::String service = in.kind == TargetKind::streaming ? "Spotify" : in.kind == TargetKind::appleMusic ? "Apple Music" : "a service at " + targetNumber + " LUFS";
            lines.add ("Integrated " + oneDecimal (I) + " LUFS: " + service + " plays it " + oneDecimal (gap) + " dB quieter, at " + targetNumber
                       + " LUFS. If you like how dense it sounds, keep it. If you made it loud to sound louder on streaming, it won't: try backing off the limiter by 3 to 4 dB and compare both at the same loudness.");
        }
        else if (in.kind == TargetKind::club)
            lines.add ("Integrated " + oneDecimal (I) + " LUFS: " + oneDecimal (gap) + " LU above the " + targetNumber
                       + " LUFS club starting point, which is already loud. Check that the limiter isn't taking the punch out of it.");
        else
        {
            const juce::String who = in.kind == TargetKind::podcast ? "podcasts ask for" : in.kind == TargetKind::ebu ? "European broadcast asks for" : "US broadcast asks for";
            lines.add ("Integrated " + oneDecimal (I) + " LUFS: " + oneDecimal (gap) + " LU louder than the " + targetNumber + " LUFS that " + who
                       + ". Turn the whole mix down " + oneDecimal (gap) + " dB.");
        }
    }
    else
    {
        issue = true;
        const double under = T - I;
        if (in.kind == TargetKind::streaming)
        {
            const double up = std::min (under, -1.0 - tp);
            juce::String what;
            if (up <= 0.0)
                what = "with " + oneDecimal (tp) + " dBTP it can't turn it up at all, so it plays " + oneDecimal (under) + " dB quieter than most songs.";
            else if (juce::approximatelyEqual (up, under))
                what = "with " + oneDecimal (tp) + " dBTP it can turn it up the full " + oneDecimal (under) + " dB.";
            else
                what = "with " + oneDecimal (tp) + " dBTP it adds " + oneDecimal (up) + " dB, to " + oneDecimal (I + up) + " LUFS, so it plays "
                       + oneDecimal (under - up) + " dB quieter than most songs.";
            lines.add ("Integrated " + oneDecimal (I) + " LUFS, " + oneDecimal (under) + " LU under " + targetNumber
                       + ". Spotify turns quieter songs up only while the true peak stays at -1 dBTP or lower: " + what
                       + " Other services may not turn it up at all. To sit with them, raise the level into a limiter.");
        }
        else if (in.kind == TargetKind::club)
            lines.add ("Integrated " + oneDecimal (I) + " LUFS, quieter than a typical club master (-9 to -6 LUFS). A DJ can turn it up, but next to other tracks it may sound smaller. A limiter brings it up.");
        else
            lines.add ("Integrated " + oneDecimal (I) + " LUFS, " + oneDecimal (under) + " LU under the " + targetNumber + " LUFS target. Raise the whole mix "
                       + oneDecimal (under) + " dB, and keep the true peak under " + oneDecimal (C) + " dBTP.");
    }

    // 7. Loudness range.
    if (r.seconds >= 60.0 && r.range)
    {
        if (*r.range < 3.0)
        {
            issue = true;
            lines.add ("Loudness range " + oneDecimal (*r.range) + " LU: the quiet and loud parts are very close. If the chorus should lift, try less compression or limiting on the whole mix, or more contrast in the arrangement.");
        }
        else if (*r.range > 15.0)
        {
            issue = true;
            lines.add ("Loudness range " + oneDecimal (*r.range) + " LU: as wide as a film. On a phone or in a car the quiet parts may get lost. Gentle compression on the whole mix narrows it.");
        }
    }

    // 8. Peaks close to the loudness.
    if (peakFlat)
    {
        issue = true;
        lines.add (peakText());
    }

    // 9. Nothing to say: the loudness is close to the target and the rest is fine.
    if (! issue && lines.size() == before + 1)
        lines.add ("Nothing stands out: the loudness, the peaks and the range all suit " + (in.kind == TargetKind::own ? "your target of " + targetNumber + " LUFS" : targetName()) + ".");
    return lines;
}
} // namespace daw::loudness
