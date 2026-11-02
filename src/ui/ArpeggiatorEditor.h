#pragma once

#include "ArpeggiatorGuide.h"

#include <optional>

namespace daw
{
/**
    The steps of DawIt Arpeggiator's pattern: up to 16 steps over the song's beats, the playing one lit. Click a step to
    give it the brush's kind (a step already in it becomes Note), drag to paint several, Shift-click to accent a step,
    double-click or press Delete over a step to make it Note, keys 1 to 5 pick the brush, Cmd+C and Cmd+V copy and paste
    the pattern as a line of text, and the right-click menu has the kinds, the accent, copy and paste and the guide.
*/
class ArpeggiatorStepView final : public juce::Component, public juce::TooltipClient
{
public:
    ArpeggiatorStepView (BuiltInArpeggiator&, arp::StepKind& brush);

    /** Called after a key picked another brush, so the buttons can follow. */
    std::function<void()> onBrushChanged;

    /** Opens the guide, from the right-click menu. */
    std::function<void()> onOpenGuide;
    std::function<void()> onRandomise;   // the Randomise item of the right-click menu

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::String getTooltip() override;

private:
    arptext::StepGrid getGrid() const;
    int getStepAt (juce::Point<int>) const;
    void paintStep (int step, arp::StepKind);
    void showMenu (int step);
    void copyPattern() const;
    void pastePattern();

    BuiltInArpeggiator& processor;
    arp::StepKind& brush;
    int hovered = -1, lastPainted = -1;
    arp::StepKind paintKind = arp::StepKind::note;
    bool painting = false;
};

//==============================================================================
/** The keys it follows, named by their place in the chord, with the note playing now lit and the chord's name above. */
class ArpeggiatorKeyboardView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ArpeggiatorKeyboardView (BuiltInArpeggiator&);

    void paint (juce::Graphics&) override;

private:
    BuiltInArpeggiator& processor;
};

//==============================================================================
/** The last 2 bars of notes it played, as a small piano roll against the beats. */
class ArpeggiatorNotesView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ArpeggiatorNotesView (BuiltInArpeggiator&);

    void paint (juce::Graphics&) override;

private:
    BuiltInArpeggiator& processor;
};

//==============================================================================
/** The window of DawIt Arpeggiator: the keys, the pattern with its brush, the last 2 bars, the controls and the explainer. */
class BuiltInArpeggiatorEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInArpeggiatorEditor (BuiltInArpeggiator&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    void setBrush (arp::StepKind);
    void showOrderMenu();
    void randomise();
    void undoRandomise();

    BuiltInArpeggiator& arpeggiator;
    arp::StepKind brush = arp::StepKind::chord;
    ArpeggiatorKeyboardView keyboardView { arpeggiator };
    ArpeggiatorStepView stepView { arpeggiator, brush };
    ArpeggiatorNotesView notesView { arpeggiator };
    juce::TextButton brushButtons[arp::numStepKinds];
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch orderSwitch { "Order", (int) arp::Knob::order };
    Switch latchSwitch { "Latch", (int) arp::Knob::latch };
    juce::TextButton randomiseButton { "Randomise" }, undoButton { "Undo" };
    std::optional<arp::Settings> beforeRandomise;   // what Undo goes back to
    int randomiseCount = 0;
    int presetIndex = -1;   // the starting point picked last
};
} // namespace daw
