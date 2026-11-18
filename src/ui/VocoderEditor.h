#pragma once

#include "EffectWindow.h"
#include "engine/BuiltInVocoder.h"

namespace daw
{
namespace vocoderui
{
/** DawIt Vocoder's colour in its window, its guide and the device bar chip. */
inline const juce::Colour colour { 0xfff472b6 };
}

/**
    The bands of DawIt Vocoder, three strips across the same log-spaced bands: what each band hears of the voice, what the
    carrier holds there, and the result, as bars from -60 to 0 dB. Click a band to hear only that band, click it again to
    hear all of them (until the window closes); hover for its range and levels; right-click for more.
*/
class VocoderBandsView final : public juce::Component, public juce::TooltipClient
{
public:
    VocoderBandsView (BuiltInVocoder&, std::function<void (int)> openGuide, std::function<void (int)> hoverTopic);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    juce::Rectangle<float> getBarArea() const;
    int getBandAt (juce::Point<float>) const;
    std::vector<double> getEdges() const;

    BuiltInVocoder& processor;
    std::function<void (int)> guide, hoverTopic;
    int hovered = -1;
};

/** One octave of keys, lit for the notes the carrier plays now, with the source in force and the line about where its
    notes come from. */
class VocoderKeyboardView final : public juce::Component, public juce::TooltipClient
{
public:
    VocoderKeyboardView (BuiltInVocoder&, std::function<void (int)> hoverTopic);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    BuiltInVocoder& processor;
    std::function<void (int)> hoverTopic;
};

/** The line the window and its guide show about the carrier's notes: the host's meaning for the voice, then what Keys or
    Song chord do now. Empty when nothing needs saying. */
juce::String getVocoderLeadLine (const BuiltInVocoder&);

/** The window of DawIt Vocoder: the bands and the keyboard strip, the level history, the knob groups and the explainer. */
class BuiltInVocoderEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInVocoderEditor (BuiltInVocoder&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;
    static std::atomic<bool>& unusedListen();
    static fx::Clap& unusedClap();

    BuiltInVocoder& vocoderProcessor;
    VocoderBandsView bands;
    VocoderKeyboardView keyboard;
    fxui::LevelHistoryView history;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    int presetIndex = -1;
};
} // namespace daw
