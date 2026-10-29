#pragma once

#include "engine/BuiltInDistortion.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Distortion: what each knob and part of its window does, with
    live numbers, when and how distortion is used, the "What is distortion?" guide, and the
    pictures both use: the curve, a note before and after it, the harmonics it ends up with, and
    the filters before and after the curve.
*/
namespace daw::disttext
{
/** The pictures and the match switch; the knobs are 0 up (dist::Knob), the shared topics fxui::CommonTopic. */
enum Topic { curve = 50, harmonics, filters, match };

inline const juce::Colour colour { 0xffff4d6d };       // the distorted sound, odd harmonics
inline const juce::Colour evenColour { 0xffffd166 };   // even harmonics
inline const juce::Colour beforeColour { 0xff5cc8ff }; // the filter before the curve

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which sounds. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, to the A3 test note with this peak (as a gain). */
juce::String describeSettings (int topic, const dist::Settings&, float peak);

/** The level to show the pictures at: the track's recent peak, or -6 dB while it is silent. */
inline float getShownPeak (float recentPeak) { return recentPeak > 0.003f ? recentPeak : 0.5f; }

//==============================================================================
/** The drive and curve together: in (left to right, -1 to 1) against out, with the part the A3 note
    at this peak uses after the tight low cut drawn thicker, and the straight line of no change dashed. */
juce::Rectangle<float> drawCurve (juce::Graphics&, juce::Rectangle<float>, const dist::Settings&, float peak, bool labels);

/** The A3 note with this peak before the curve (grey) and after it (coloured) at the same loudness,
    without the filters, so only the change in shape shows. */
void drawBeforeAfter (juce::Graphics&, juce::Rectangle<float>, const dist::Settings&, float peak, double cycles);

/** The harmonics the A3 note with this peak ends up with, filters included, as bars against the
    loudest (0 dB to -80), even ones in yellow, odd ones in red, named as intervals. */
void drawHarmonics (juce::Graphics&, juce::Rectangle<float>, const dist::Settings&, float peak, int count, bool labels);

/** What the filters do to each frequency: the tight low cut before the curve (blue) and the tone and
    speaker after it (red), from 20 Hz to 20 kHz. */
void drawFilters (juce::Graphics&, juce::Rectangle<float>, const dist::Settings&, bool labels);

/** The pages of "What is distortion?", with pictures that follow the distortion's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInDistortion&);
} // namespace daw::disttext
