#pragma once
#include "StereoGuide.h"
#include "StereoScope.h"

namespace daw
{
/** The live stereo display, its crossover handle and correlation meters. */
class StereoPictures final : public juce::Component, public juce::TooltipClient
{
public:
    StereoPictures (BuiltInStereo&, std::function<void (int)>, std::function<void (int)>);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::String getTooltip() override;
private:
    juce::Rectangle<float> spectrumBounds() const;
    void setFrequency (float x);
    BuiltInStereo& processor;
    std::function<void (int)> guide, onHover;
    bool dragging = false;
    float dragFrequency = 120.0f;
};
/** DawIt Stereo's editor: width, crossover, listening and live stereo pictures. */
class BuiltInStereoEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInStereoEditor (BuiltInStereo&);
private:
    juce::AttributedString buildText (int) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;
    BuiltInStereo& stereoProcessor;
    StereoPictures pictures;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch slope { "Slope", 1 }, listen { "Listen", 3 }, match { "Match level", 51 };
    int presetIndex = -1;
};
}
