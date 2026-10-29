#pragma once

#include <array>
#include <atomic>
#include <optional>
#include <vector>

namespace daw::stereo
{
/** Mid and side for a pair of samples. */
struct MidSide { double mid = 0.0, side = 0.0; };
/** Split left and right into the shared and differing parts. */
MidSide midSide (double left, double right) noexcept;
/** Correlation from the sums of left squared, right squared and left times right; silence has no reading. */
std::optional<double> correlation (double leftPower, double rightPower, double cross) noexcept;
/** Side level relative to mid, in dB, limited to -100 to +100. */
double sideToMidDb (double midPower, double sidePower) noexcept;
/** Linkwitz-Riley high-band magnitude at a frequency ratio, for 12, 24 or 48 dB/octave. */
double highMagnitude (double ratio, int slope) noexcept;
/** Change in total level when side power is multiplied by width squared. */
double widthLevelDb (double midPower, double sidePower, double width) noexcept;
/** Correlation of equal-level channels after changing width. */
double widthCorrelation (double midPower, double sidePower, double width) noexcept;
/** Mono level change relative to equally loud channels. */
double monoDropDb (double correlationValue) noexcept;

/** One 50 ms block of input measurements, for a ten-second track analysis. */
struct Block
{
    double left = 0.0, right = 0.0, cross = 0.0, mid = 0.0, side = 0.0;
    double lowLeft = 0.0, lowRight = 0.0, lowCross = 0.0, lowMid = 0.0, lowSide = 0.0;
    float peak = 0.0f;
};
/** A point in the low and high bands, for a goniometer. */
struct Point { float lowLeft = 0.0f, lowRight = 0.0f, highLeft = 0.0f, highRight = 0.0f, left = 0.0f, right = 0.0f; };
/** Stereo readings, written by the audio thread and read independently by any number of views. */
class Measurement final
{
public:
    /** Prepare for the sample rate, clearing all readings. */
    void prepare (double sampleRate) noexcept;
    /** Add one pair and its low-band pair. No allocation or locking. */
    void push (float left, float right, float lowLeft, float lowRight) noexcept;
    /** Add a pair and its filtered low and high bands, retaining the original pair for spectra. */
    void push (float left, float right, float lowLeft, float lowRight, float highLeft, float highRight) noexcept;
    /** Most recent smoothed correlation, or no reading in silence. */
    std::optional<double> getCorrelation (bool low) const noexcept;
    /** Recent 50 ms blocks, oldest first. */
    std::vector<Block> getBlocks() const;
    /** Recent goniometer points, oldest first. */
    std::vector<Point> getPoints() const;
private:
    struct AtomicBlock
    {
        std::array<std::atomic<double>, 10> values {};
        std::atomic<float> peak { 0.0f };
    };
    struct AtomicPoint { std::array<std::atomic<float>, 6> values {}; };
    static constexpr int blockCount = 200, pointCount = 8192;
    std::array<AtomicBlock, blockCount> blocks;
    std::array<AtomicPoint, pointCount> points;
    std::atomic<unsigned long long> blocksWritten { 0 }, pointsWritten { 0 };
    std::array<std::atomic<double>, 6> smoothed {};
    Block current;
    int samplesPerBlock = 2205, samplesInBlock = 0, pointInterval = 1, pointCounter = 0;
    double coefficient = 0.0;
};
} // namespace daw::stereo
