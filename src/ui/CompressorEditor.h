#pragma once

#include "CompressorGuide.h"
#include "GuideButton.h"

namespace daw
{
/**
    The last 4 seconds of DawIt Compressor, newest on the right: the level coming in (grey), what
    it turned down (red), what comes out (blue) and the threshold. It also keeps those points for
    the curve's dot, the meters and the explainer.
*/
class CompressorHistory final : public juce::Component
{
public:
    explicit CompressorHistory (BuiltInCompressor&);

    /** Takes the newest points from the compressor. Called by the editor's timer. */
    void update();

    /** The loudest level and the most reduction over the 4 seconds shown. */
    comptext::Recent getRecent() const;

    /** Up to `count` of the newest points, oldest first. */
    std::vector<BuiltInCompressor::HistoryPoint> getNewest (int count) const;

    /** How many points make a second. */
    double getPointsPerSecond() const;

    void paint (juce::Graphics&) override;

private:
    BuiltInCompressor& processor;
    std::vector<BuiltInCompressor::HistoryPoint> ring, incoming;
    int ringPosition = 0, filled = 0;

    static constexpr double seconds = 4.0;
};

//==============================================================================
/**
    The compressor's curve: what comes out (up) for each level that goes in (left to right), from
    -60 to 0 dB, before makeup. Drag the threshold dot sideways and the ratio dot at the right end
    up and down; scroll for the knee, double-click to reset. A dot shows the track as it plays.
*/
class CompressorCurve final : public juce::Component
{
public:
    CompressorCurve (BuiltInCompressor&, const CompressorHistory&);

    /** What the mouse is on: the threshold or ratio dot, or the curve. */
    std::function<void (comptext::Topic)> onHover;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Handle { none, threshold, ratio };

    juce::Rectangle<float> getPlotArea() const;
    float toX (double db) const;
    float toY (double db) const;
    double fromX (float x) const;
    double fromY (float y) const;
    juce::Point<float> getHandlePosition (Handle) const;
    Handle findHandle (juce::Point<float>) const;

    BuiltInCompressor& processor;
    const CompressorHistory& history;
    Handle hovered = Handle::none, dragging = Handle::none;
};

//==============================================================================
/** IN, GR (gain reduction) and OUT, falling back smoothly. */
class CompressorMeter final : public juce::Component
{
public:
    explicit CompressorMeter (const CompressorHistory&);

    /** Follows the history. Called by the editor's timer. */
    void update();

    void paint (juce::Graphics&) override;

private:
    const CompressorHistory& history;
    float in = -100.0f, out = -100.0f, reduction = 0.0f, reductionHeld = 0.0f;
    int holdTicks = 0;
};

//==============================================================================
/** The window of DawIt Compressor: the curve, the history, the meters, the knobs and the explainer. */
class BuiltInCompressorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BuiltInCompressorEditor (BuiltInCompressor&);
    ~BuiltInCompressorEditor() override;

    /** Opens "What is compression?", or brings it to the front, at a page (-1 keeps the one it shows). */
    void openGuide (int page = -1);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;

private:
    class KnobControl;

    void timerCallback() override;
    void hover (std::optional<comptext::Topic>);
    void applyPreset (int index);

    /** The threshold that turns the loudest of the last 4 seconds down by `reductionDb` with
        `settings`, from what the compressor heard with `heardWith`; nothing without a signal. */
    std::optional<float> findThreshold (const comp::Settings& settings, const comp::Settings& heardWith, float reductionDb) const;

    fxui::PluginMode pluginMode { *this };
    BuiltInCompressor& compressor;
    CompressorHistory history { compressor };
    CompressorCurve curve { compressor, history };
    CompressorMeter meter { history };
    CompressorExplainer explainer { compressor };
    juce::OwnedArray<KnobControl> knobs;
    juce::TextButton detection;
    juce::Label detectionLabel { {}, "Detection" };

    juce::Label title { {}, "DawIt Compressor" };
    juce::ComboBox presets;
    juce::TextButton lessonButton { "Set it up step by step" }, listenButton { "Hear what it removes" },
                     helpButton { "?" };
    GuideButton guideButton { "What is compression?" };
    fxui::GuideHost guideHost { *this };
    juce::ToggleButton matchButton { "Match level" };
    juce::Label matchValue;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> matchAttachment;

    // The explainer follows the mouse once it rests on something, so crossing the window doesn't change it.
    std::optional<comptext::Topic> pendingTopic;
    int pendingTicks = 0, ticks = 0;

    std::array<juce::Rectangle<int>, 4> groupAreas;

    static constexpr int headerHeight = 44, knobsHeight = 150, explainerWidth = 350;
};
} // namespace daw
