#pragma once

#include "ReverbGuide.h"

namespace daw
{
/**
    What one clap turns into with DawIt Reverb's settings: the direct sound, the pre-delay, the
    early reflections and the tail, with the beats of the song. Drag the dot at the start of the
    reverb for the pre-delay and the one at the end of the decay line for the decay; double-click
    to reset them.
*/
class ReverbResponseView final : public juce::Component
{
public:
    explicit ReverbResponseView (BuiltInReverb&);

    /** What the mouse is on: a dot (its knob) or the picture. */
    std::function<void (int topic)> onHover;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Handle { none, predelay, decay };

    juce::Point<float> getHandlePosition (Handle) const;
    Handle findHandle (juce::Point<float>) const;
    verb::Knob getKnob (Handle h) const { return h == Handle::predelay ? verb::Knob::predelay : verb::Knob::decay; }

    BuiltInReverb& processor;
    mutable verbtext::ResponseCache cache;
    verbtext::ResponsePlot plot;
    Handle hovered = Handle::none, dragging = Handle::none;
    double dragSeconds = 0.0;   // the time shown stays put while dragging
};

//==============================================================================
/** The window of DawIt Reverb: the response, the room, the last 4 seconds, the knobs and the explainer. */
class BuiltInReverbEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInReverbEditor (BuiltInReverb&);

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;
    void clapped() override;

    class Room;

    BuiltInReverb& reverb;
    ReverbResponseView response { reverb };
    std::unique_ptr<Room> room;
    fxui::LevelHistoryView history;
    Switch typeSwitch { "Type", verbtext::type };
    juce::OwnedArray<fxui::EffectKnob> knobs;
    int presetIndex = -1;   // the starting point picked last
    double clapTime = -100.0;
};
} // namespace daw
