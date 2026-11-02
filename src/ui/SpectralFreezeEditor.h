#pragma once

#include "SpectralFreezeGuide.h"

namespace daw
{
/**
    The spectrum of DawIt Spectral Freeze: the live sound as a thin line and the hold as a filled shape, 20 Hz to 20 kHz
    on a log axis, with the hold's tallest peak labelled. Move the mouse over it to read the frequency, the nearest note
    and the levels; the right-click menu takes a new snapshot, lets go, or opens the guide.
*/
class SpectralFreezeSpectrumView final : public juce::Component, public juce::TooltipClient
{
public:
    explicit SpectralFreezeSpectrumView (BuiltInSpectralFreeze&);

    /** Opens the guide, from the right-click menu. */
    std::function<void()> onOpenGuide;

    /** Takes the newest live sound, and the hold a few times a second. Called by the editor's timer. */
    void update();

    /** What the hold is now, for the header's status. */
    const sfztext::HoldInfo& getHoldInfo() const noexcept { return holdInfo; }

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    sfztext::SpectrumPlot getPlot() const;

    BuiltInSpectralFreeze& processor;
    sfztext::Spectrum live, hold;
    sfztext::HoldInfo holdInfo;
    double hoverHz = 0.0;
    int ticks = 0;
};

//==============================================================================
/** The window's frame wants a "hear only" switch and a clap of its own; a freeze has neither, so these stand in for them. */
struct SpectralFreezeStand
{
    std::atomic<bool> unusedListen { false };
    fx::Clap unusedClap;
};

/** The window of DawIt Spectral Freeze: the spectrum, the level history, the Hold, Colour and Output controls and the explainer. */
class BuiltInSpectralFreezeEditor final : private SpectralFreezeStand, public fxui::EffectEditor
{
public:
    explicit BuiltInSpectralFreezeEditor (BuiltInSpectralFreeze&);

    void resized() override;

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    BuiltInSpectralFreeze& freeze;
    SpectralFreezeSpectrumView spectrumView { freeze };
    fxui::LevelHistoryView history;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch freezeSwitch { "Freeze", sfztext::freezeSwitch };
    Switch captureSwitch { "Snapshot", sfztext::captureAgain };
    juce::Label status;
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
