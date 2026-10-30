#pragma once

#include "SubtractiveGuide.h"

namespace daw
{
/**
    The harmonics of DawIt Subtractive's test note with the filter over them, at the cutoff the newest
    note has right now. Drag left and right for the cutoff, up and down for the resonance;
    double-click to open the filter fully.
*/
class SubtractiveSpectrumView final : public juce::Component
{
public:
    explicit SubtractiveSpectrumView (BuiltInSubtractive&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    BuiltInSubtractive& processor;
    float dragStartCutoff = 0.0f, dragStartResonance = 0.0f;
};

//==============================================================================
/** The window of DawIt Subtractive: the harmonics and the filter, the wave, the envelopes, the last
    4 seconds, the knobs in two rows and the explainer. */
class BuiltInSubtractiveEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInSubtractiveEditor (BuiltInSubtractive&);
    /** Selects a starting sound and shows why its controls are set that way. */
    void applyPreset (int index) override;

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    /** A switch that steps through a choice's items on each click. */
    Switch& addSwitch (sub::Knob, const juce::String& name);

    class Wave;
    class Envelopes;

    BuiltInSubtractive& synth;
    SubtractiveSpectrumView spectrum { synth };
    std::unique_ptr<Wave> wave;
    std::unique_ptr<Envelopes> envelopes;
    fxui::LevelHistoryView history;
    std::map<sub::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;
    std::map<sub::Knob, std::unique_ptr<Switch>> switches;
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
