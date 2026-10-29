#pragma once

#include "engine/Engine.h"
#include "engine/RecordingGuide.h"
#include "GuidePrint.h"
#include "RecordingActions.h"
#include "Theme.h"

namespace daw
{
/**
    The "How recording works" guide: from the air to a file, the audio interface, choosing an input, setting the
    level, buffers and latency, hearing yourself, why takes are moved earlier, takes on a loop, and a look at
    your own device. Each topic has a picture (five boxes, three signal strengths on a volt scale, sockets, a
    phrase at three levels, a latency table, two listening paths, a click and its late copy, stacked takes) and the
    last one, "Your session", reads the device, the armed tracks and the last take, with advice when something
    is worth changing. The live input level follows the armed track. Opened from the R button's menu, a take's
    menu, the Audio / MIDI Settings ? and the Monitor warning.
*/
class RecordingExplainer final : public juce::Component,
                                 public guideprint::Source,
                                 private juce::ListBoxModel,
                                 private juce::Timer
{
public:
    explicit RecordingExplainer (Engine&);
    ~RecordingExplainer() override;

    void showTopic (recordingguide::Topic);

    juce::String getPrintTitle() const override { return "How recording works"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void timerCallback() override;

    void show (int index);
    void refreshText();

    Engine& engine;
    int shownRow = 0;
    int rowBeforePrinting = -1;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::recording };

    static constexpr int width = 1000, height = 740, pictureHeight = 330;
};
} // namespace daw
