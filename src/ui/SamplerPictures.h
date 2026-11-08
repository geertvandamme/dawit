#pragma once

#include "engine/BuiltInSampler.h"
#include "Theme.h"

namespace daw::samplerpics
{
/** DawIt Sampler's colour, used for its window, its knobs and its chip. */
inline const juce::Colour colour { 0xffe0a458 };

/** One colour for each recording of a preset, by its place in the list of zones: neighbours differ. */
juce::Colour getZoneColour (int index);

/** The zones of a preset with the second of every stereo pair left out (a left and a right recording share their keys and
    velocities), so each key and velocity range is drawn once. */
std::vector<soundfonts::Zone> getDistinctZones (const std::vector<soundfonts::Zone>&);

/** Where a key sits across `area`: the 128 keys are 128 equal cells, so the zone picture and the keyboard under it line up. */
float getKeyX (juce::Rectangle<float> area, int key);

/** The key under an x position of `area`, 0 to 127. */
int getKeyAt (juce::Rectangle<float> area, float x);

/** Draws the zones as bands: keys across, velocity up (1 at the bottom, 127 at the top), each in its colour with a dot on
    its root. `litKey` (or -1) is the last note played, shown as a line. */
void paintZoneMap (juce::Graphics&, juce::Rectangle<float> plot, const std::vector<soundfonts::Zone>& distinct, int hoveredKey, int litKey);

/** The 128 keys in a strip, black keys darker, an octave label on every C; the hovered key and the last note lit. */
void paintKeyboard (juce::Graphics&, juce::Rectangle<float> area, int hoveredKey, int litKey);

/** The zones that play a key at a velocity (or, with -1, at any velocity), from a list of distinct zones. */
std::vector<int> findZonesAt (const std::vector<soundfonts::Zone>& distinct, int key, int velocity);

/** "E2 - zone C2-F#2 - root D2 - velocity 1-127", or "E2 - no zone: silent" for a key no zone plays. */
juce::String describeKey (const std::vector<soundfonts::Zone>& distinct, int key, int velocity);

/** The tiny strip of a chain box: the zones of the sampler's preset as bands over the keys, with the root dots. Draws
    nothing while the font isn't ready. A bypassed plugin's strip is grey. */
void paintMiniStrip (juce::Graphics&, juce::Rectangle<float> plot, const BuiltInSampler&, bool bypassed);
} // namespace daw::samplerpics
