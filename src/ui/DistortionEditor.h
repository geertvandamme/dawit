#pragma once

#include "DistortionGuide.h"

namespace daw
{
/**
    DawIt Distortion's curve with the stretch the test note uses, and under it that note before and
    after the curve. Drag up and down for the drive; double-click to reset it.
*/
class DistortionCurveView final : public juce::Component
{
public:
    explicit DistortionCurveView (BuiltInDistortion&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    BuiltInDistortion& processor;
    float dragStartDrive = 0.0f;
};

//==============================================================================
/** The window of DawIt Distortion: the curve, the harmonics, the filters, the last 4 seconds, the knobs and the explainer. */
class BuiltInDistortionEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInDistortionEditor (BuiltInDistortion&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    class Harmonics;
    class Filters;

    BuiltInDistortion& distortion;
    DistortionCurveView curve { distortion };
    std::unique_ptr<Harmonics> harmonics;
    std::unique_ptr<Filters> filters;
    fxui::LevelHistoryView history;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch typeSwitch { "Type", (int) dist::Knob::type }, speakerSwitch { "Speaker", (int) dist::Knob::speaker },
           matchSwitch { "Match level", disttext::match };
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
