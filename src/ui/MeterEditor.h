#pragma once

#include "EffectWindow.h"
#include "StereoScope.h"
#include "engine/BuiltInMeter.h"

namespace daw
{
/** What DawIt Meter's picture colours mean, shared by its window, the chip and (later) its guide. */
namespace meterui
{
/** The effect's own colour (teal). */
inline const juce::Colour colour { 0xff3dd6c6 };

/** Colour of the integrated number and its "to target" line: green within 1 LU of the target, yellow outside it
    (louder than the target is not a fault, so never red), and grey with no reading or with the target hidden. */
juce::Colour getLoudnessColour (std::optional<double> integratedLufs, double targetLufs, bool showTarget);

/** "-12.4", or an en dash for a reading with no data. */
juce::String formatValue (std::optional<double> value, int decimals = 1);
}

/** DawIt Meter's pictures: the readings, the last two minutes of loudness, the level bars and the correlation bar. */
class MeterView final : public juce::Component, public juce::TooltipClient
{
public:
    MeterView (BuiltInMeter&, std::function<void (int)> openGuide, std::function<void (int)> hoverTopic);

    /** Takes the newest block peaks, holds them and repaints. Called by the editor's timer. */
    void update();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    struct Areas
    {
        juce::Rectangle<float> readings, momentary, shortTerm, integrated, toTarget, range, maxShort, truePeak, peaks,
                               history, levels, correlation;
    };

    Areas getAreas() const;
    int getTopicAt (juce::Point<float>) const;
    void drawBar (juce::Graphics&, juce::Rectangle<float>, const juce::String& label, std::optional<double> lufs, const loudness::Readings&, const meterfx::Settings&);
    void drawHistory (juce::Graphics&, juce::Rectangle<float>, const meterfx::Settings&, const loudness::Readings&);
    void drawLevels (juce::Graphics&, juce::Rectangle<float>, const meterfx::Settings&, const loudness::Readings&);

    BuiltInMeter& processor;
    std::function<void (int)> guide, onHover;
    std::array<float, 2> shownDb { { -90.0f, -90.0f } }, holdDb { { -90.0f, -90.0f } }, holdAge { { 0.0f, 0.0f } };
    int lastResetCount = 0;
};

/** DawIt Meter's window: loudness readings against a target, a two-minute history, levels and correlation. */
class BuiltInMeterEditor final : public fxui::EffectEditor
{
public:
    explicit BuiltInMeterEditor (BuiltInMeter&);

private:
    juce::AttributedString buildText (int) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;

    BuiltInMeter& meterProcessor;
    MeterView view;
    juce::OwnedArray<fxui::EffectKnob> knobs;
    Switch showTargetSwitch { "Show target", 2 }, resetNow { "Measurement", 3 }, resetOnPlay { "Reset on play", 4 };
    int presetIndex = -1;
};
}
