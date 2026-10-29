#pragma once

#include "engine/BuiltInSaturation.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Saturation: what each knob and part of its window does, with
    live numbers, the "What is saturation?" guide, and the pictures both use: the curve, a sine
    before and after it, and the harmonics it adds.
*/
namespace daw::sattext
{
/** The pictures and the match switch; the knobs are 0 up (sat::Knob), the shared topics fxui::CommonTopic. */
enum Topic { curve = 50, harmonics, match };

inline const juce::Colour colour { 0xffff8a5c };       // the saturated sound, odd harmonics
inline const juce::Colour evenColour { 0xffffd166 };   // even harmonics

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, to a sine with this peak (as a gain). */
juce::String describeSettings (int topic, const sat::Settings&, float peak);

/** The level to show the pictures at: the track's recent peak, or -6 dB while it is silent. */
inline float getShownPeak (float recentPeak) { return recentPeak > 0.003f ? recentPeak : 0.5f; }

//==============================================================================
/** The curve: in (left to right, -1 to 1) against out, with the part the track uses (up to its
    peak) drawn thicker and the straight line of no change dashed. Returns where it drew. */
juce::Rectangle<float> drawCurve (juce::Graphics&, juce::Rectangle<float>, const sat::Settings&, float peak, bool labels);

/** A sine with this peak before the curve (grey) and after it at the same loudness (coloured), so
    only the change in shape shows. */
void drawBeforeAfter (juce::Graphics&, juce::Rectangle<float>, const sat::Settings&, float peak, double cycles);

/** The harmonics the curve makes of a sine with this peak, as bars against the note itself (0 dB
    to -80), even ones in yellow, odd ones in orange, named as intervals. */
void drawHarmonics (juce::Graphics&, juce::Rectangle<float>, const sat::Settings&, float peak, int count, bool labels);

/** The pages of "What is saturation?", with pictures that follow the saturation's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInSaturation&);
} // namespace daw::sattext
