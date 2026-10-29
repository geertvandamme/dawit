#pragma once

#include "FmGuide.h"

namespace daw
{
/**
    The four operators of DawIt FM, wired by their algorithm: click one to show its knobs, drag up or
    down on it to change its level, right-click to choose the algorithm. The boxes glow with the newest
    note's envelopes.
*/
class FmOperatorsView final : public juce::Component
{
public:
    explicit FmOperatorsView (BuiltInFm&);

    /** The operator clicked (0 to 3), to show its knobs. */
    std::function<void (int)> onSelect;

    /** Opens "What is FM synthesis?", the last item of the right-click menu. */
    std::function<void()> onOpenGuide;

    /** The operator whose knobs are shown, outlined. */
    void setSelected (int op);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> getPlot() const;
    void showMenu();

    BuiltInFm& processor;
    int selected = 0, hovered = -1, dragging = -1;
    float dragFrom = 0.0f;
};

//==============================================================================
/** The window of DawIt FM: the operators, the spectrum, the wave, the last 4 seconds, the selected
    operator's knobs, the algorithm and the playing knobs, and the explainer. */
class BuiltInFmEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInFmEditor (BuiltInFm&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    /** Shows operator `op`'s knobs. */
    void select (int op);
    void updateGroups();
    Switch& addSwitch (fm::Knob, const juce::String& name);

    class Spectrum;
    class Wave;

    BuiltInFm& synth;
    FmOperatorsView operatorsView { synth };
    std::unique_ptr<Spectrum> spectrumView;
    std::unique_ptr<Wave> wave;
    fxui::LevelHistoryView history;
    std::map<fm::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;
    std::map<fm::Knob, std::unique_ptr<Switch>> switches;
    std::unique_ptr<Switch> operatorSwitch;
    int selected = 1;      // operator 2, the modulator, which is where FM starts
    int presetIndex = -1;  // the starting point picked last
    bool shownAsCarrier = false;
};
} // namespace daw
