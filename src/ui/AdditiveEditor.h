#pragma once

#include "AdditiveGuide.h"
#include "DescribeController.h"

namespace daw
{
/**
    The 32 harmonics of DawIt Additive as bars to draw on: drag over them to set their levels (the
    bottom is off), double-click one to turn it off, right-click to fill them with a shape. The
    colour follows the newest note's swell and fade.
*/
class AdditivePartialsView final : public juce::Component
{
public:
    explicit AdditivePartialsView (BuiltInAdditive&);

    /** Opens "What is additive synthesis?", the last item of the right-click menu. */
    std::function<void()> onOpenGuide;

    /** Lights these bars (harmonic numbers 1 to 32), as a walkthrough step of Describe a sound asks; none clears them. */
    void setLitBars (const std::vector<int>&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> getPlot() const;
    void draw (juce::Point<float>);
    void showMenu();
    void setAll (const std::array<float, additive::numPartials>&);
    void beginGestures();
    void endGestures();

    BuiltInAdditive& processor;
    int hovered = 0, lastPartial = 0;   // 0: none
    float lastDb = 0.0f;
    bool drawing = false;
    std::array<bool, additive::numPartials> litBars {};
};

//==============================================================================
/** The window of DawIt Additive: the harmonics to draw, the wave, a note over time, the last 4
    seconds, the knobs in two rows and the explainer. */
class BuiltInAdditiveEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInAdditiveEditor (BuiltInAdditive&);

    BuiltInAdditive& getSynth() noexcept { return synth; }

    /** The part of the window that belongs to Describe a sound (the button, the bar, the ring, the answer). */
    DescribeController& getDescribe() noexcept { return *describe; }

    /** True when this window has the Describe a sound button (the app, not a plugin). */
    bool hasDescribeBar() const noexcept { return describe->hasBar(); }
    bool isDescribeBarOpen() const noexcept { return describe->isOpen(); }
    DescribeBarBase* getDescribeBar() noexcept { return describe->getBar(); }

    /** Opens or closes the bar; the pictures shrink or grow to fit. */
    void setDescribeBarOpen (bool open) { describe->setDescribeBarOpen (open); }

    /** Opens the guide at the page about describing sounds. */
    void openDescribeGuide();

    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    /** A switch that steps through a choice's items on each click. */
    Switch& addSwitch (additive::Knob, const juce::String& name);

    class Wave;
    class OverTime;

    BuiltInAdditive& synth;
    AdditivePartialsView partialsView { synth };
    std::unique_ptr<Wave> wave;
    std::unique_ptr<OverTime> overTime;
    fxui::LevelHistoryView history;
    std::map<additive::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;
    std::map<additive::Knob, std::unique_ptr<Switch>> switches;
    int presetIndex = -1;   // the starting point picked last
    std::unique_ptr<DescribeController> describe;
};
} // namespace daw
