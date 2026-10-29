#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace daw
{
/**
    The small pictures of a setting that sit in the boxes of a chain: the response curve of DawIt EQ, the curve of
    DawIt Compressor (level in across, level out up, with the diagonal for no compression) and the gate's. The
    device bar draws them in its chips and the Signal Flow window in its boxes, so they look the same in both.
    They only read the plugin's settings.
*/
namespace chainpictures
{
    /** True for DawIt EQ, DawIt Compressor, DawIt Gate and DawIt Tune, the effects that have a picture. */
    bool hasPicture (const juce::AudioProcessor*);

    /** Draws the zero line (or the diagonal) and the curve of the plugin's settings inside `plot`, one point per pixel
        of its width. A bypassed plugin's curve is grey. Draws nothing for a plugin without a picture. */
    void paintCurve (juce::Graphics&, juce::Rectangle<float> plot, const juce::AudioProcessor*, bool bypassed);

    /** The area of the picture drawn behind the EQ's curve by the device bar: where 0 dB sits on `plot`. */
    float getEqY (juce::Rectangle<float> plot, double db);
} // namespace chainpictures
} // namespace daw
