#include "StereoScope.h"
#include "Theme.h"
namespace daw::stereoScope
{
void drawGoniometer (juce::Graphics& g, juce::Rectangle<float> bounds, const std::vector<stereo::Point>& points, bool labels, double sampleRate)
{
    auto square = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()), juce::jmin (bounds.getWidth(), bounds.getHeight())).reduced (labels ? 12.0f : 2.0f);
    g.setColour (theme::textDim.withAlpha (0.25f));
    g.drawEllipse (square, 1.0f);
    g.drawLine (square.getCentreX(), square.getY(), square.getCentreX(), square.getBottom(), 1.0f);
    g.drawLine (square.getX(), square.getCentreY(), square.getRight(), square.getCentreY(), 1.0f);
    auto dot = [&] (float left, float right, juce::Colour c)
    {
        const float x = juce::jlimit (-1.0f, 1.0f, (right - left) * 0.7071f);
        const float y = juce::jlimit (-1.0f, 1.0f, (right + left) * 0.7071f);
        g.setColour (c.withAlpha (0.5f));
        g.fillEllipse (square.getCentreX() + x * square.getWidth() * 0.5f - 0.8f, square.getCentreY() - y * square.getHeight() * 0.5f - 0.8f, 1.6f, 1.6f);
    };
    const auto count = (size_t) juce::jmax (1, juce::roundToInt (sampleRate * 0.05));
    const auto first = points.size() > count ? points.size() - count : 0;
    for (size_t i = first; i < points.size(); ++i)
    {
        const auto& p = points[i];
        dot (p.highLeft, p.highRight, colour);
        dot (p.lowLeft, p.lowRight, lowColour);
    }
    if (labels)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("L", bounds.removeFromLeft (12.0f), juce::Justification::centred);
        g.drawText ("R", bounds.removeFromRight (12.0f), juce::Justification::centred);
    }
}
void drawCorrelation (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& name, std::optional<double> value)
{
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (name, bounds.removeFromLeft (27.0f), juce::Justification::centredLeft);
    auto reading = bounds.removeFromRight (value ? 34.0f : 54.0f);
    g.setColour (value && *value < 0.0 ? theme::meterRed : theme::text);
    g.drawText (value ? juce::String (*value, 2) : juce::String ("no reading"), reading, juce::Justification::centredRight);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("-1", bounds.removeFromLeft (12.0f), juce::Justification::centredLeft);
    g.drawText ("+1", bounds.removeFromRight (12.0f), juce::Justification::centredRight);
    auto bar = bounds.reduced (1.0f, 5.0f);
    g.setColour (theme::textDim.withAlpha (0.2f));
    g.fillRoundedRectangle (bar, 2.0f);
    if (! value) return;
    const float x = bar.getX() + (float) ((juce::jlimit (-1.0, 1.0, *value) + 1.0) * 0.5) * bar.getWidth();
    g.setColour (*value < 0.0 ? theme::meterRed : colour);
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (x, bar.getCentreX()), bar.getY(), juce::jmax (x, bar.getCentreX()), bar.getBottom()));
}
}
