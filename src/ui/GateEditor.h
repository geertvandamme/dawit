#pragma once

#include "GateGuide.h"

namespace daw
{
/**
    DawIt Gate's curve: level in across, out up, with the open and closed lines, the level it opens
    at and the level it closes at, and the track as a dot with a short trail. Drag the threshold dot
    and the closing-level dot sideways, the range dot up and down (Alt: 0.1 dB steps); scroll changes
    the ratio; double-click a dot resets it; right-click for more.
*/
class GateCurveView final : public juce::Component, public juce::TooltipClient
{
public:
    GateCurveView (BuiltInGate&, const std::vector<gate::Moment>& recent, std::function<void (int)> openGuide);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::String getTooltip() override;

private:
    enum class Dot { none, threshold, closing, range };

    juce::Rectangle<float> getCurveBounds() const;
    Dot getDotAt (juce::Point<float>) const;
    juce::RangedAudioParameter* getParameter (Dot) const;
    void showMenu();

    BuiltInGate& processor;
    const std::vector<gate::Moment>& moments;
    std::function<void (int)> onOpenGuide;
    Dot dragging = Dot::none;
    gate::Settings dragStart;
};

//==============================================================================
/** The last 4 seconds of the track as the gate heard them (see gatetext::drawMoments). Hover for the numbers. */
class GateHistoryView final : public juce::Component, public juce::TooltipClient
{
public:
    GateHistoryView (BuiltInGate&, const std::vector<gate::Moment>& recent, std::function<double()> getBpm);

    void paint (juce::Graphics&) override;
    juce::String getTooltip() override;

private:
    juce::Rectangle<float> getPlotBounds() const;

    BuiltInGate& processor;
    const std::vector<gate::Moment>& moments;
    std::function<double()> bpm;
};

//==============================================================================
/** What the gate listens to: the key's response, with a handle for the low cut and one for the high cut.
    Drag a handle; double-click turns it off; right-click for key listen. */
class KeyFilterView final : public juce::Component, public juce::TooltipClient
{
public:
    KeyFilterView (BuiltInGate&, std::function<void (int)> openGuide);

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    enum class Handle { none, low, high };

    juce::Rectangle<float> getPlot() const;
    Handle getHandleAt (juce::Point<float>) const;
    void showMenu();

    BuiltInGate& processor;
    std::function<void (int)> onOpenGuide;
    Handle dragging = Handle::none;
};

//==============================================================================
/** The window of DawIt Gate: the curve, the last 4 seconds, the key filters, the knobs and the explainer. */
class BuiltInGateEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInGateEditor (BuiltInGate&);
    ~BuiltInGateEditor() override;

private:
    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    BuiltInGate& gateProcessor;
    std::vector<gate::Moment> recent;
    GateCurveView curve;
    GateHistoryView history;
    KeyFilterView keyView;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch keyListenSwitch { "Key listen", gatetext::keyListen }, matchSwitch { "Match level", gatetext::match };
    int presetIndex = -1;                // the starting point picked last
    std::optional<float> presetThreshold;   // the threshold that starting point took from the track, if it did
};
} // namespace daw
