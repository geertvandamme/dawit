#pragma once

#include "SubtractiveGuide.h"

namespace daw
{
/**
    The harmonics of DawIt Subtractive's test note with the filter over them, at the cutoff the newest
    note has right now. Drag left and right for the cutoff, up and down for the resonance;
    double-click to open the filter fully.
*/
class SubtractiveSpectrumView final : public juce::Component
{
public:
    explicit SubtractiveSpectrumView (BuiltInSubtractive&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    BuiltInSubtractive& processor;
    float dragStartCutoff = 0.0f, dragStartResonance = 0.0f;
};

//==============================================================================
/** The bar of "Describe a sound", across the top of the pictures. Only the app has one (`SubtractiveDescribeBar`,
    made through `BuiltInSubtractiveEditor::describeBarFactory`), so the plugin builds don't compile the AI code. */
class DescribeBarBase : public juce::Component
{
public:
    /** The height the bar takes when open. */
    static constexpr int barHeight = 76;

    /** 30 times a second while the bar is open (the editor's tick): follows the knobs in a walkthrough. */
    virtual void tick() {}
    /** The bar was just opened: put the cursor in the text box and look for the models. */
    virtual void opened() {}
    /** The bar was just closed: end a walkthrough, cancel a request. */
    virtual void closed() {}
};

//==============================================================================
/** The window of DawIt Subtractive: the harmonics and the filter, the wave, the envelopes, the last
    4 seconds, the knobs in two rows and the explainer. */
class BuiltInSubtractiveEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInSubtractiveEditor (BuiltInSubtractive&);
    /** Selects a starting sound and shows why its controls are set that way. */
    void applyPreset (int index) override;

    /** Set by the app to make the bar of "Describe a sound". Null in a plugin, so there is no button there. */
    static inline std::function<std::unique_ptr<DescribeBarBase> (BuiltInSubtractiveEditor&)> describeBarFactory;

    BuiltInSubtractive& getSynth() noexcept { return synth; }

    /** True when this window has the Describe a sound button (the app, not a plugin). */
    bool hasDescribeBar() const noexcept { return describeBar != nullptr; }
    bool isDescribeBarOpen() const noexcept { return describeOpen; }
    DescribeBarBase* getDescribeBar() noexcept { return describeBar.get(); }

    /** Opens or closes the bar; the pictures shrink or grow to fit. */
    void setDescribeBarOpen (bool);

    /** Puts a ring round a knob or switch (a walkthrough's current step); none removes it. */
    void setHighlight (std::optional<sub::Knob>);

    /** Shows the last description's changes and reasons in the explainer; with `setupSteps` it shows those lines
        (how to set up Ollama) instead. */
    void showDescription (const juce::StringArray& setupSteps = {});

    /** Opens the guide at the page about describing sounds. */
    void openDescribeGuide();

    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    /** A switch that steps through a choice's items on each click. */
    Switch& addSwitch (sub::Knob, const juce::String& name);

    class Wave;
    class Envelopes;

    BuiltInSubtractive& synth;
    SubtractiveSpectrumView spectrum { synth };
    std::unique_ptr<Wave> wave;
    std::unique_ptr<Envelopes> envelopes;
    fxui::LevelHistoryView history;
    std::map<sub::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;
    std::map<sub::Knob, std::unique_ptr<Switch>> switches;
    int presetIndex = -1;   // the starting point picked last
    juce::TextButton describeButton { "Describe a sound" };
    juce::TextButton describeGuideLink { "Read about describing sounds" };   // under the answer in the explainer
    std::unique_ptr<DescribeBarBase> describeBar;
    bool describeOpen = false;
    std::optional<sub::Knob> highlight;
    juce::StringArray describeSetupSteps;   // not empty: the explainer shows how to set up Ollama
};
} // namespace daw
