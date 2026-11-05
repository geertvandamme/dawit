#pragma once

#include "EffectWindow.h"
#include "HarmoniserGuide.h"
#include "TunePictures.h"
#include "engine/BuiltInHarmoniser.h"

namespace daw
{
/** DawIt Harmoniser's pictures: the *now* column (a keyboard with the lead note, each voice's note and the interval between
    them), the trace of the last seconds with a line for the lead and one per voice, and the level under it. */
class HarmoniserView final : public juce::Component, public juce::TooltipClient
{
public:
    HarmoniserView (BuiltInHarmoniser&, std::function<void (int)> openGuide, std::function<void (int)> hoverTopic);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    static constexpr float nowWidth = 300.0f, levelHeight = 54.0f;
    juce::Rectangle<float> getNowArea() const;
    juce::Rectangle<float> getTraceArea() const;
    juce::Rectangle<float> getLevelArea() const;
    tunepics::Trace makeTrace() const;
    void paintNow (juce::Graphics&, juce::Rectangle<float>);
    double getSeconds() const;
    juce::String describeFrame (int frame) const;

    BuiltInHarmoniser& processor;
    std::function<void (int)> guide, hoverTopic;
    mutable std::vector<tune::Frame> traceFrames;
    mutable std::array<std::vector<float>, harm::numVoices> traceVoices;
    int centreNote = 60;
};

/** DawIt Harmoniser's window: the harmony (Fixed or Key, the key, the scale, the mix), the two voices, and the pictures. */
class BuiltInHarmoniserEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInHarmoniserEditor (BuiltInHarmoniser&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;
    void showKeyMenu();
    void changeSettings (const std::function<void (harm::Settings&)>&);
    void updateVoiceKnobs (harm::Mode);
    static std::atomic<bool>& unusedListen();

    BuiltInHarmoniser& harmoniser;
    HarmoniserView view;
    Switch harmony { "Harmony", 0 }, keyChoice { "Key", 1 }, scale { "Scale", 2 };
    std::unique_ptr<fxui::EffectKnob> mix;
    juce::OwnedArray<Switch> voiceOn;
    std::array<std::array<std::unique_ptr<fxui::EffectKnob>, 5>, harm::numVoices> voiceKnobs;   // interval, level, pan, detune, delay
    int presetIndex = -1;
    int shownMode = -1;
};
}
