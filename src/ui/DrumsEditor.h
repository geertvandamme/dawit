#pragma once

#include "DrumsGuide.h"

namespace daw
{
/**
    The nine pads of DawIt Drums: click one to play it and show its knobs, right-click for the guide.
    Each lights as its drum sounds.
*/
class DrumPadsView final : public juce::Component
{
public:
    explicit DrumPadsView (BuiltInDrums&);

    /** The pad clicked, to show its knobs. */
    std::function<void (drums::Drum)> onSelect;

    /** Opens "What is drum synthesis?", the last item of the right-click menu. */
    std::function<void()> onOpenGuide;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> getPlot() const;

    BuiltInDrums& processor;
    std::optional<drums::Drum> hovered;
};

//==============================================================================
/** The window of DawIt Drums: the pads, a hit of the selected drum over time, its spectrum, the last
    4 seconds, the selected drum's knobs, the kit's knobs, and the explainer. */
class BuiltInDrumsEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInDrumsEditor (BuiltInDrums&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    /** Shows drum `d`'s knobs. */
    void select (drums::Drum);
    void updateGroups();

    class Hit;
    class Spectrum;

    BuiltInDrums& synth;
    DrumPadsView padsView { synth };
    std::unique_ptr<Hit> hitView;
    std::unique_ptr<Spectrum> spectrumView;
    fxui::LevelHistoryView history;
    std::map<drums::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;
    std::unique_ptr<Switch> chokeSwitch, drumSwitch;
    drums::Drum selected = drums::Drum::kick;
    int presetIndex = -1;   // the kit picked last
};
} // namespace daw
