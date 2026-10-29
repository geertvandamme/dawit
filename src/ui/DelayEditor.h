#pragma once

#include "DelayGuide.h"

namespace daw
{
/**
    A clap and its echoes with DawIt Delay's settings, in the left and right speaker, against the
    beats of the song. Drag the first echo sideways for the time (to the nearest note value when
    it follows the tempo) and the second up and down for the feedback; double-click to reset.
*/
class DelayEchoView final : public juce::Component
{
public:
    explicit DelayEchoView (BuiltInDelay&);

    /** What the mouse is on: a dot (its control) or the picture. */
    std::function<void (int topic)> onHover;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Handle { none, time, feedback };

    std::optional<juce::Point<float>> getHandlePosition (Handle) const;
    Handle findHandle (juce::Point<float>) const;
    juce::RangedAudioParameter* getParameter (Handle) const;

    BuiltInDelay& processor;
    dlytext::EchoPlot plot;
    Handle hovered = Handle::none, dragging = Handle::none;
    double dragSeconds = 0.0;   // the time shown stays put while dragging
};

//==============================================================================
/** The window of DawIt Delay: the echoes, the wall, the last 4 seconds, the controls and the explainer. */
class BuiltInDelayEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInDelayEditor (BuiltInDelay&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;
    void clapped() override;

    class Wall;

    BuiltInDelay& delay;
    DelayEchoView echoes { delay };
    std::unique_ptr<Wall> wall;
    fxui::LevelHistoryView history;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch syncSwitch { "Sync", dlytext::sync }, modeSwitch { "Mode", (int) dly::Knob::mode };
    int presetIndex = -1;   // the starting point picked last
    double clapTime = -100.0;
};
} // namespace daw
