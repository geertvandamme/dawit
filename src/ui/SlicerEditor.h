#pragma once

#include "SlicerGuide.h"

namespace daw
{
/**
    The pattern of DawIt Slicer: up to 16 steps over the song's beats, the playing one lit. Click a step to give it the
    brush's mode (a step already in that mode becomes Through), drag to paint several, double-click or press Delete over a
    step to make it Through, keys 1 to 4 pick the brush, Cmd+C and Cmd+V copy and paste the pattern as a line of text, and
    the right-click menu has the modes, the copy and paste and the guide.
*/
class SlicerPatternView final : public juce::Component, public juce::TooltipClient
{
public:
    SlicerPatternView (BuiltInSlicer&, slc::Mode& brush);

    /** Called after a key picked another brush, so the buttons can follow. */
    std::function<void()> onBrushChanged;

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
    slctext::PatternGrid getGrid() const;
    int getStepAt (juce::Point<int>) const;
    void paintStep (int step, slc::Mode);
    void showMenu (int step);
    void copyPattern() const;
    void pastePattern();

    BuiltInSlicer& processor;
    slc::Mode& brush;
    int hovered = -1, lastPainted = -1;
    slc::Mode paintMode = slc::Mode::through;
    bool painting = false;
};

//==============================================================================
/** The last 2 bars as the slicer heard them: the track in grey, what came out in colour, per step. */
class SlicerLastBarsView final : public juce::Component
{
public:
    explicit SlicerLastBarsView (BuiltInSlicer&);

    void paint (juce::Graphics&) override;

private:
    BuiltInSlicer& processor;
};

//==============================================================================
/** The window of DawIt Slicer: the pattern with its brush, the last 2 bars, the controls and the explainer. */
class BuiltInSlicerEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInSlicerEditor (BuiltInSlicer&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    void setBrush (slc::Mode);

    BuiltInSlicer& slicer;
    slc::Mode brush = slc::Mode::repeat;
    SlicerPatternView patternView { slicer, brush };
    SlicerLastBarsView lastBars { slicer };
    juce::TextButton brushButtons[slc::numModes];
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch tempoSwitch { "Tempo", (int) slc::Knob::tempo };
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
