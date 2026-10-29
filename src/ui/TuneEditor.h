#pragma once

#include "EffectWindow.h"
#include "TuneGuide.h"
#include "TunePictures.h"
#include "engine/BuiltInTune.h"

namespace daw
{
/** DawIt Tune's pictures: the tuner for the note being sung now and the trace of the last seconds, with the level under it. */
class TuneView final : public juce::Component, public juce::TooltipClient
{
public:
    TuneView (BuiltInTune&, std::function<void (int)> openGuide, std::function<void (int)> hoverTopic);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    static constexpr float tunerWidth = 300.0f, levelHeight = 54.0f;
    juce::Rectangle<float> getTunerArea() const;
    juce::Rectangle<float> getTraceArea() const;
    juce::Rectangle<float> getLevelArea() const;
    tunepics::Trace makeTrace() const;
    tunepics::KeyInfo getKeyInfo() const;
    double getSeconds() const;

    BuiltInTune& processor;
    std::function<void (int)> guide, hoverTopic;
};

/** DawIt Tune's window: the mode, the target, how fast and how far to move notes, and the tuner and trace. */
class BuiltInTuneEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInTuneEditor (BuiltInTune&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    BuiltInTune& tuner;
    TuneView view;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch mode { "Mode", 0 }, target { "Target", 1 }, keepVibrato { "Keep vibrato", 5 };
    int presetIndex = -1;
};
}
