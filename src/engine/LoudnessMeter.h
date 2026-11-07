#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace daw::loudness
{
/** One second-order filter section: b0, b1, b2 on top, 1, a1, a2 below. */
struct Biquad { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

/** The two stages of BS.1770's K-weighting: a shelf that models the head, then a high-pass. */
struct KFilter { Biquad shelf, highPass; };

/** The K-weighting for a sample rate. At 48 kHz it equals the standard's Tables 1 and 2. */
KFilter designKFilter (double sampleRate);

/** How much K-weighting changes a sine of this frequency, in dB (+0.69 dB at 997 Hz, -13 dB at 20 Hz). */
double kResponseDb (double frequencyHz, double sampleRate);

/** The gain that moves a measured integrated loudness to the target, in dB (negative: turn it down). */
double gainToTargetDb (double integratedLufs, double targetLufs) noexcept;

/** What a Spotify-style service does to a song: turn a louder song down to the target, and turn a quieter one up only
    while its true peak stays at or below `peakLimitDbtp`. Returns the gain in dB. */
double streamingGainDb (double integratedLufs, double truePeakDbtp, double targetLufs = -14.0, double peakLimitDbtp = -1.0) noexcept;

/** Everything a meter reads at one moment. A reading with no data yet has no value. */
struct Readings
{
    std::optional<double> momentary, shortTerm, integrated, range;   // LUFS, LUFS, LUFS, LU
    std::optional<double> maxMomentary, maxShortTerm;                // LUFS
    std::optional<double> truePeak, samplePeak;                      // the loudest so far, dBTP and dBFS
    std::optional<double> rms, crest;                                // the last 3 seconds: dBFS, and peak minus RMS in dB
    double seconds = 0.0;                                            // how long it has measured
};

/**
    Measures mono or stereo loudness by ITU-R BS.1770-5 and EBU R 128, and true peak, for any sample rate.
    Nothing is allocated while processing, and the memory doesn't grow however long it runs: integrated loudness
    and the range come from histograms of 0.1 LU bins.
*/
class Meter final
{
public:
    /** Allocate the sliding windows for a sample rate and clear the measurement. */
    void prepare (double sampleRate);
    /** Clear the measurement without changing the sample rate or allocating. */
    void reset() noexcept;
    /** Add a mono sample, or a stereo pair when channels is two. While `counting` is false the windows follow the
        sound but integrated loudness, the range, the maxima and the time don't count it (a stopped host). */
    void push (float left, float right, int channels, bool counting = true) noexcept;
    /** Loudness of the last 400 ms, once the window is full. */
    std::optional<double> momentary() const noexcept;
    /** Loudness of the last three seconds, once the window is full. */
    std::optional<double> shortTerm() const noexcept;
    /** Integrated loudness after the absolute and relative gates. */
    std::optional<double> integrated() const noexcept;
    /** Loudness range from gated short-term readings. */
    std::optional<double> range() const noexcept;
    /** The highest true peak so far, in dBTP (4x oversampled, BS.1770 Annex 2). */
    std::optional<double> truePeakDb() const noexcept;
    /** The highest sample peak so far, in dBFS. */
    std::optional<double> samplePeakDb() const noexcept;
    /** RMS of the last three seconds in dBFS, averaged over the channels. */
    std::optional<double> rmsDb() const noexcept;
    /** Crest factor: the peak of the last three seconds minus their RMS, in dB. */
    std::optional<double> crestDb() const noexcept;
    /** The highest momentary and short-term loudness so far. */
    std::optional<double> maxMomentary() const noexcept;
    std::optional<double> maxShortTerm() const noexcept;
    /** The seconds that counted. */
    double measuredSeconds() const noexcept;
    /** True once any 400 ms block was above -70 LUFS, so integrated loudness has a reading. */
    bool hasReading() const noexcept;
    /** All readings at once. */
    Readings getReadings() const noexcept;

private:
    struct Filter
    {
        Biquad c;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        double process (double x) noexcept;
    };
    struct Bin { std::uint64_t count = 0; double energy = 0; };
    static constexpr int binCount = 801;
    static constexpr int tapsPerPhase = 12, peakBlocks = 30;
    static std::optional<double> level (double energy) noexcept;
    static void add (std::array<Bin, binCount>& bins, double energy) noexcept;
    double sumOf (const std::vector<double>&) const noexcept;
    std::array<std::array<Filter, 2>, 2> filters {};
    std::vector<double> windowM, windowS, windowR;
    std::array<Bin, binCount> integratedBins {}, rangeBins {};
    double sumM = 0, sumS = 0, sumR = 0;
    std::size_t posM = 0, posS = 0, posR = 0, filledM = 0, filledS = 0, filledR = 0;
    int interval = 1, counter = 0, channelsSeen = 1;
    double rate = 48000.0;
    std::uint64_t countedSamples = 0;
    double maxM = 0, maxS = 0;            // energies
    bool hasMaxM = false, hasMaxS = false;
    double samplePeakMax = 0, truePeakMax = 0;   // linear
    double blockPeak = 0;
    std::array<double, peakBlocks> peakRing {};
    int peakPos = 0;
    // The oversampler: the last 12 samples of each channel (written twice, so they read in one run) and the three fractional phases.
    std::array<std::array<float, 2 * tapsPerPhase>, 2> history {};
    int historyPos = 0;
    std::array<std::array<double, tapsPerPhase>, 3> phases {};
};

//==============================================================================
/** The starting points of DawIt Meter, in the order of BuiltInMeter's list, and `own` for a target set by hand. */
enum class TargetKind { streaming, appleMusic, podcast, ebu, atsc, club, mixing, own };

/** What kind of place DawIt Meter sits in, for the advice. */
enum class HostKind { unknown, track, bus, returnTrack };

/** What the advice looks at: one measurement, and the meter's settings. */
struct AdviceInput
{
    Readings readings;
    double target = -14.0, ceiling = -1.0;
    bool showTarget = true;
    TargetKind kind = TargetKind::streaming;
    HostKind host = HostKind::unknown;
    juce::String hostName;
    juce::String scope;   // for a song measurement: "The whole song (bar 1 to bar 37, 3:12):", shown first
};

/** The paragraphs of "Your track": what the measurement says about the song, in order. */
juce::StringArray advise (const AdviceInput&);

/** Minutes and seconds, "3:12". */
juce::String formatClock (double seconds);
} // namespace daw::loudness
