#pragma once

#include "SaturationGuide.h"

namespace daw
{
/**
    DawIt Saturation's curve with the stretch the track uses, and under it a sine at the track's
    level before and after the curve. Drag up and down for the drive; double-click to reset it.
*/
class SaturationCurveView final : public juce::Component
{
public:
    explicit SaturationCurveView (BuiltInSaturation&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    BuiltInSaturation& processor;
    float dragStartDrive = 0.0f;
};

//==============================================================================
/** The window of DawIt Saturation: the curve, the harmonics, the last 4 seconds, the knobs and the explainer. */
class BuiltInSaturationEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInSaturationEditor (BuiltInSaturation&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    class Harmonics;

    BuiltInSaturation& saturation;
    SaturationCurveView curve { saturation };
    std::unique_ptr<Harmonics> harmonics;
    fxui::LevelHistoryView history;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch typeSwitch { "Type", (int) sat::Knob::type }, matchSwitch { "Match level", sattext::match };
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
