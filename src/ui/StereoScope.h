#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "engine/StereoMeasure.h"

/** Shared stereo pictures, also available to other meters. */
namespace daw::stereoScope
{
inline const juce::Colour colour { 0xffff7fc8 };
inline const juce::Colour lowColour { 0xffffc857 };
/** Draw recent low and high band samples as mid and side. */
void drawGoniometer (juce::Graphics&, juce::Rectangle<float>, const std::vector<stereo::Point>&, bool labels = true, double sampleRate = 48000.0);
/** Draw correlation from -1 to +1, or no reading in silence. */
void drawCorrelation (juce::Graphics&, juce::Rectangle<float>, const juce::String&, std::optional<double>);
}
