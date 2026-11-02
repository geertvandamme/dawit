#pragma once

#include "StepFilterGuide.h"

namespace daw
{
/**
    The pattern of DawIt Step Filter: up to 16 bars, each as high as its step's cutoff, over the song's beats, with the
    cutoff's path through them and the playing step lit. Click or drag to set steps to the height of the mouse (in steps of
    5%, Alt for exact values), double-click or press Delete over a step to open it fully, Cmd+C and Cmd+V copy and paste the
    pattern as a line of text, and the right-click menu has quick heights, the copy and paste and the guide.
*/
class StepFilterPatternView final : public juce::Component, public juce::TooltipClient
{
public:
    explicit StepFilterPatternView (BuiltInStepFilter&);

    /** Opens the guide, from the right-click menu. */
    std::function<void()> onOpenGuide;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::String getTooltip() override;

private:
    stftext::PatternGrid getGrid() const;
    int getStepAt (juce::Point<int>) const;
    float getPaintedHeight (const juce::MouseEvent&) const;
    void setStep (int step, float percent);
    void showMenu (int step);
    void copyPattern() const;
    void pastePattern();

    BuiltInStepFilter& processor;
    int hovered = -1, lastPainted = -1;
    float lastHeight = 100.0f;
    bool painting = false;
};

//==============================================================================
/** The filter at the cutoff playing now: its response from 20 Hz to 20 kHz, the Low to High range shaded, a dot at the cutoff. */
class StepFilterResponseView final : public juce::Component
{
public:
    explicit StepFilterResponseView (BuiltInStepFilter&);

    void paint (juce::Graphics&) override;

private:
    BuiltInStepFilter& processor;
};

//==============================================================================
/** The window of DawIt Step Filter: the pattern, the filter now, the level history, the controls and the explainer. */
class BuiltInStepFilterEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInStepFilterEditor (BuiltInStepFilter&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    void cycle (stf::Knob);

    BuiltInStepFilter& filter;
    StepFilterPatternView patternView { filter };
    StepFilterResponseView responseView { filter };
    fxui::LevelHistoryView history;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch tempoSwitch { "Tempo", (int) stf::Knob::tempo };
    Switch typeSwitch { "Type", (int) stf::Knob::type };
    Switch slopeSwitch { "Slope", (int) stf::Knob::slope };
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
