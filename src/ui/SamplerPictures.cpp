#include "SamplerPictures.h"
#include "engine/SamplerGuide.h"

namespace daw::samplerpics
{
namespace
{
    bool isBlackKey (int key)
    {
        const int pc = key % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    juce::String describeRange (int low, int high)
    {
        return samplerguide::getNoteName (low) + "-" + samplerguide::getNoteName (high);
    }
}

juce::Colour getZoneColour (int index)
{
    return juce::Colour::fromHSV (std::fmod ((float) index * 0.618034f, 1.0f), 0.55f, 0.85f, 1.0f);
}

std::vector<soundfonts::Zone> getDistinctZones (const std::vector<soundfonts::Zone>& zones)
{
    std::vector<soundfonts::Zone> out;

    for (auto& z : zones)
    {
        const bool seen = std::any_of (out.begin(), out.end(), [&z] (auto& o)
                                       { return o.lowKey == z.lowKey && o.highKey == z.highKey && o.lowVelocity == z.lowVelocity && o.highVelocity == z.highVelocity; });

        if (! seen)
            out.push_back (z);
    }

    return out;
}

float getKeyX (juce::Rectangle<float> area, int key)
{
    return area.getX() + area.getWidth() * (float) key / 128.0f;
}

int getKeyAt (juce::Rectangle<float> area, float x)
{
    return juce::jlimit (0, 127, (int) std::floor ((x - area.getX()) / area.getWidth() * 128.0f));
}

void paintZoneMap (juce::Graphics& g, juce::Rectangle<float> plot, const std::vector<soundfonts::Zone>& distinct, int hoveredKey, int litKey)
{
    g.setColour (theme::background.withAlpha (0.7f));
    g.fillRect (plot);

    // Octave lines at every C.
    g.setColour (theme::grid);

    for (int key = 0; key < 128; key += 12)
        g.fillRect (getKeyX (plot, key), plot.getY(), 1.0f, plot.getHeight());

    auto velocityY = [&plot] (int velocity) { return plot.getBottom() - plot.getHeight() * (float) velocity / 127.0f; };

    for (size_t i = 0; i < distinct.size(); ++i)
    {
        auto& z = distinct[i];
        const auto band = juce::Rectangle<float> (getKeyX (plot, z.lowKey), velocityY (z.highVelocity),
                                                  getKeyX (plot, z.highKey + 1) - getKeyX (plot, z.lowKey),
                                                  velocityY (z.lowVelocity - 1) - velocityY (z.highVelocity));
        const auto c = getZoneColour ((int) i);
        g.setColour (c.withAlpha (0.55f));
        g.fillRect (band.reduced (0.5f));
        g.setColour (c);
        g.drawRect (band.reduced (0.5f), 1.0f);

        // The dot on the root: the key this recording was made at.
        const float rootX = getKeyX (plot, juce::jlimit (0, 127, z.rootKey)) + plot.getWidth() / 256.0f;
        const float rootY = juce::jlimit (plot.getY() + 4.0f, plot.getBottom() - 4.0f, band.getCentreY());
        g.setColour (theme::text);
        g.fillEllipse (rootX - 3.0f, rootY - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (rootX - 3.0f, rootY - 3.0f, 6.0f, 6.0f, 1.0f);
    }

    if (litKey >= 0)
    {
        g.setColour (theme::text.withAlpha (0.8f));
        g.fillRect (getKeyX (plot, litKey), plot.getY(), juce::jmax (1.0f, plot.getWidth() / 128.0f), plot.getHeight());
    }

    if (hoveredKey >= 0)
    {
        g.setColour (theme::text.withAlpha (0.35f));
        g.fillRect (getKeyX (plot, hoveredKey), plot.getY(), juce::jmax (1.0f, plot.getWidth() / 128.0f), plot.getHeight());
    }

    g.setColour (theme::gridBar);
    g.drawRect (plot, 1.0f);
}

void paintKeyboard (juce::Graphics& g, juce::Rectangle<float> area, int hoveredKey, int litKey)
{
    const float cell = area.getWidth() / 128.0f;
    g.setFont (juce::FontOptions (9.5f));

    for (int key = 0; key < 128; ++key)
    {
        const auto r = juce::Rectangle<float> (getKeyX (area, key), area.getY(), cell, area.getHeight());
        auto c = isBlackKey (key) ? juce::Colour (0xff3a3d45) : juce::Colour (0xffd9dce3);

        if (key == litKey)
            c = samplerpics::colour;
        else if (key == hoveredKey)
            c = c.interpolatedWith (samplerpics::colour, 0.5f);

        g.setColour (c);
        g.fillRect (r.reduced (0.0f, 0.0f).withTrimmedRight (0.5f));

        if (key % 12 == 0)
        {
            g.setColour (juce::Colour (0xff1b1c20));
            g.drawText (samplerguide::getNoteName (key), juce::Rectangle<float> (r.getX() - 4.0f, r.getBottom() - 14.0f, 28.0f, 13.0f),
                        juce::Justification::centredLeft, false);
        }
    }
}

std::vector<int> findZonesAt (const std::vector<soundfonts::Zone>& distinct, int key, int velocity)
{
    std::vector<int> found;

    for (size_t i = 0; i < distinct.size(); ++i)
        if (distinct[i].containsKey (key) && (velocity < 0 || distinct[i].containsVelocity (velocity)))
            found.push_back ((int) i);

    return found;
}

juce::String describeKey (const std::vector<soundfonts::Zone>& distinct, int key, int velocity)
{
    const auto name = samplerguide::getNoteName (key);
    const auto found = findZonesAt (distinct, key, velocity);

    if (found.empty())
    {
        const auto any = findZonesAt (distinct, key, -1);
        return any.empty() ? name + " - no zone: silent" : name + " - no zone at this velocity";
    }

    const auto& z = distinct[(size_t) found.front()];
    return name + " - zone " + describeRange (z.lowKey, z.highKey) + " - root " + samplerguide::getNoteName (z.rootKey)
           + " - velocity " + juce::String (z.lowVelocity) + "-" + juce::String (z.highVelocity);
}

void paintMiniStrip (juce::Graphics& g, juce::Rectangle<float> plot, const BuiltInSampler& s, bool bypassed)
{
    const auto distinct = getDistinctZones (s.getZones());

    for (size_t i = 0; i < distinct.size(); ++i)
    {
        auto& z = distinct[i];
        const float top = plot.getBottom() - plot.getHeight() * (float) z.highVelocity / 127.0f;
        const float bottom = plot.getBottom() - plot.getHeight() * (float) (z.lowVelocity - 1) / 127.0f;
        const auto band = juce::Rectangle<float> (getKeyX (plot, z.lowKey), top, getKeyX (plot, z.highKey + 1) - getKeyX (plot, z.lowKey), bottom - top);
        g.setColour (bypassed ? theme::textDim.withAlpha (0.35f) : getZoneColour ((int) i).withAlpha (0.8f));
        g.fillRect (band.reduced (0.0f, 0.3f).withTrimmedRight (0.5f));
    }
}
} // namespace daw::samplerpics
