#pragma once

#include "PhysicalGuide.h"

namespace daw
{
/**
    The model of DawIt Physical: the string or tube with the exciter on it, moving as the newest note plays.
    Drag along it to move where it's played; right-click to choose the exciter or the resonator.
*/
class PhysicalModelView final : public juce::Component
{
public:
    explicit PhysicalModelView (BuiltInPhysical&);

    /** Opens "What is physical modelling?", the last item of the right-click menu. */
    std::function<void()> onOpenGuide;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> getPlot() const;
    void showMenu();

    BuiltInPhysical& processor;
    bool dragging = false;
    double phase = 0.0, lastPaint = 0.0;
};

//==============================================================================
/** The window of DawIt Physical: the model, the harmonics, how long each rings, the last 4 seconds, the
    exciter's and the resonator's knobs, the vibrato and the playing knobs, and the explainer. */
class BuiltInPhysicalEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInPhysicalEditor (BuiltInPhysical&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    /** Greys the knobs the exciter doesn't use. */
    void updateUsed();
    Switch& addSwitch (phys::Knob, const juce::String& name);

    class Harmonics;
    class Rings;

    BuiltInPhysical& synth;
    PhysicalModelView modelView { synth };
    std::unique_ptr<Harmonics> harmonicsView;
    std::unique_ptr<Rings> ringsView;
    fxui::LevelHistoryView history;
    std::map<phys::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;
    std::map<phys::Knob, std::unique_ptr<Switch>> switches;
    int presetIndex = -1;   // the starting point picked last
    phys::Exciter shownExciter = phys::Exciter::pluck;
};
} // namespace daw
