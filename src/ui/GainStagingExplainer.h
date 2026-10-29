#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "engine/GainStaging.h"
#include "Theme.h"
#include "GuidePrint.h"

namespace daw
{
namespace gainstaging
{
    /** Grey, green, yellow, red: as on the device bar's arrows. */
    inline juce::Colour getZoneColour (Zone zone)
    {
        switch (zone)
        {
            case Zone::silent:  return theme::textDim.withAlpha (0.6f);
            case Zone::quiet:   return theme::textDim;
            case Zone::healthy: return theme::meterGreen;
            case Zone::hot:     return theme::meterYellow;
            case Zone::over:    return theme::meterRed;
        }

        return theme::textDim;
    }

    /** The selected track's chain right now: what the device bar shows. */
    struct Snapshot
    {
        juce::String trackName;     // empty: no track selected
        std::vector<Stop> stops;
        std::vector<Hint> hints;
    };
} // namespace gainstaging

//==============================================================================
/**
    A guide to gain staging, opened from the device bar: the level at every step of a track's chain,
    what the colours mean, comparing effects at the same loudness, levels into a compressor, where to
    turn up and down, when over 0 dB distorts, and a step by step. At the top it draws the selected
    track's chain live, as a line of levels through the instrument, effects, fader and master, with
    what the device bar noticed; click a note to read about it.
*/
class GainStagingExplainer final : public juce::Component,
                                   public guideprint::Source,
                                   private juce::ListBoxModel,
                                   private juce::Timer
{
public:
    explicit GainStagingExplainer (std::function<gainstaging::Snapshot()> getSnapshot);
    ~GainStagingExplainer() override;

    void showTopic (gainstaging::Topic);

    juce::String getPrintTitle() const override { return "Gain staging"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class ChainPicture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void timerCallback() override;

    std::function<gainstaging::Snapshot()> getSnapshot;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<ChainPicture> picture;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::gainStaging };
    std::optional<gainstaging::Topic> topicBeforePrinting;
    gainstaging::Topic shown = gainstaging::Topic::what;

    static constexpr int width = 980, height = 720;
};
} // namespace daw
