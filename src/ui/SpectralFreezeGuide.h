#pragma once

#include "engine/BuiltInSpectralFreeze.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Spectral Freeze: what each control and part of its window does,
    the "What is a spectral freeze?" guide, and the pictures both use: the spectrum of the live sound
    and of the hold on a log frequency axis, the hold bin by bin, the chord's waveform and its hold.
*/
namespace daw::sfztext
{
/** The parts of the window with a topic of their own; the knobs are 0 up (sfz::Knob), the shared topics fxui::CommonTopic. */
enum Topic { freezeSwitch = 5, captureAgain = 6, spectrum = 50 };

inline const juce::Colour colour { 0xffb18cff };

juce::String getTitle (int topic);

/** What a control or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** The short text shown as a control's tooltip. */
juce::String getTooltip (int topic);

/** What these settings mean in numbers. */
juce::String describeSettings (int topic, const sfz::Settings&);

//==============================================================================
/** "1,250 Hz", "220 Hz", "10.8 Hz": a frequency with a thousands comma and as many decimals as it needs. */
juce::String formatHz (double hz);

/** How loud each bin of a sound is, as dBFS of a sine's amplitude (a full-scale sine peaks at 0 dB), with the bins' spacing. */
struct Spectrum
{
    double rate = 44100.0;
    int size = 4096;
    std::vector<float> db;

    bool isEmpty() const noexcept { return db.empty(); }
    double getSpacingHz() const noexcept { return size > 0 ? rate / size : 0.0; }

    /** The level at a frequency, interpolated between the two nearest bins. */
    float getDbAt (double hz) const;

    /** The loudest level from `lowHz` up to `highHz`. */
    float getMaxDb (double lowHz, double highHz) const;
};

/** The spectrum of `size` samples (a power of two) with a Hann window. */
Spectrum computeSpectrum (const float* samples, int size, double rate);

/** The newest 4,096 samples going into the plugin (empty, silent, when nothing has played). */
Spectrum getLiveSpectrum (const BuiltInSpectralFreeze&);

/** The hold: the louder of its channels in each bin. */
Spectrum getHoldSpectrum (const sfz::Snapshot&);

/** The test chord's hold, to draw when nothing is held. */
const Spectrum& getExampleSpectrum();

/** What the hold is: whether it is there, its tallest peak, how noisy it is and how loud. */
struct HoldInfo
{
    bool held = false;
    double peakHz = 0.0, flatness = 0.0, rmsDb = -100.0;
};

HoldInfo describeHold (const sfz::Snapshot&);

/** "Not holding", "Holding · A3", "Holding · noise". */
juce::String getStatus (bool freezeOn, const HoldInfo&);

/** The frequency axis and level axis of the spectrum picture. */
struct SpectrumPlot
{
    juce::Rectangle<float> area;
    double minHz = 20.0, maxHz = 20000.0, minDb = -100.0, maxDb = 0.0;

    float x (double hz) const;
    float y (double db) const;
    double getHzAt (float x) const;
};

SpectrumPlot layoutSpectrum (juce::Rectangle<float> bounds);

/** Draws the grid and note marks, the live sound (a thin dim line) and the hold (filled, in colour), and labels the hold's tallest peak. A hover line when `hoverHz` is above zero. */
void drawSpectrum (juce::Graphics&, const SpectrumPlot&, const Spectrum* live, const Spectrum* hold, double peakHz, double hoverHz = 0.0);

/** "1,250 Hz (near D#6) · hold -32 dB · live -40 dB". */
juce::String getHoverText (double hz, const Spectrum* live, const Spectrum* hold);

/** The small picture of the device bar's chip: the hold (colour) when there is one, else the live sound (dim). */
void paintMiniSpectrum (juce::Graphics&, juce::Rectangle<float> plot, const BuiltInSpectralFreeze&, bool bypassed);

/** The pages of "What is a spectral freeze?", with pictures that follow the freeze's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInSpectralFreeze&);
} // namespace daw::sfztext
