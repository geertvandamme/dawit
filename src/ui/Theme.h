#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
namespace theme
{
    inline const juce::Colour background   { 0xff1b1c20 };
    inline const juce::Colour panel        { 0xff25272d };
    inline const juce::Colour panelLight   { 0xff30333a };
    inline const juce::Colour lane         { 0xff222429 };
    inline const juce::Colour laneAlt      { 0xff1f2125 };
    inline const juce::Colour grid         { 0xff2e3138 };
    inline const juce::Colour gridBar      { 0xff464a54 };
    inline const juce::Colour text         { 0xffd9dce3 };
    inline const juce::Colour textDim      { 0xff8a8f9a };
    inline const juce::Colour accent       { 0xff4fa3ff };
    inline const juce::Colour playhead     { 0xffff5c5c };
    inline const juce::Colour loop         { 0xff4fa3ff };
    inline const juce::Colour meterGreen   { 0xff5fd38d };
    inline const juce::Colour meterYellow  { 0xfff2c94c };
    inline const juce::Colour meterRed     { 0xffff5c5c };
}

class DawLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DawLookAndFeel()
        : juce::LookAndFeel_V4 (juce::LookAndFeel_V4::ColourScheme {
              0xff25272d, 0xff1b1c20, 0xff30333a, 0xff464a54, 0xffd9dce3,
              0xff4fa3ff, 0xffffffff, 0xff3a6ea5, 0xffd9dce3 })
    {
        setColour (juce::TextButton::buttonColourId, theme::panelLight);
        setColour (juce::TextButton::buttonOnColourId, theme::accent);
        setColour (juce::Slider::thumbColourId, theme::accent);
        setColour (juce::Slider::trackColourId, theme::accent.withAlpha (0.6f));
        setColour (juce::Slider::rotarySliderFillColourId, theme::accent);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, theme::text);
        setColour (juce::ComboBox::backgroundColourId, theme::panelLight);
        setColour (juce::ResizableWindow::backgroundColourId, theme::background);
    }
};

/** Position of a linear gain level on the meters' scale, which runs from -60 dB to +6 dB. */
inline float meterProportion (float level)
{
    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
}

/** Shared helper: draws a vertical stereo peak meter. Levels are linear gain. */
inline void drawStereoMeter (juce::Graphics& g, juce::Rectangle<float> area, float left, float right)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (area);

    const float w = (area.getWidth() - 1.0f) / 2.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        const float level = ch == 0 ? left : right;
        auto bar = juce::Rectangle<float> (area.getX() + (float) ch * (w + 1.0f), area.getY(), w, area.getHeight());
        const float h = bar.getHeight() * meterProportion (level);
        g.setColour (level > 1.0f ? theme::meterRed : theme::meterGreen);
        g.fillRect (bar.removeFromBottom (h));
    }
}

/** Shared helper: draws a horizontal mono peak meter with a 0 dB tick. Level is linear gain. */
inline void drawLevelBar (juce::Graphics& g, juce::Rectangle<float> area, float level)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (area);

    g.setColour (level > 1.0f ? theme::meterRed : theme::meterGreen);
    g.fillRect (area.withWidth (area.getWidth() * meterProportion (level)));

    g.setColour (theme::textDim.withAlpha (0.6f));
    g.fillRect (area.getX() + area.getWidth() * meterProportion (1.0f), area.getY(), 1.0f, area.getHeight());
}
} // namespace daw
