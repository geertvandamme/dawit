#pragma once

#include "engine/Engine.h"
#include "generate/TuningGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    The "How to tune a guitar" guide: what a tuner shows, cents and beats, the tunings, the reference pitch, how to
    tune a string, why strings drift, tuning by ear, harmonics, bass and ukulele, and a last topic that compares
    the tuning chosen in the Tuner with the song. Each topic has a picture (the tuner's needle, a beating wave, the
    strings of the tuning, a reference scale, a pitch path, a drift, a neck, a vibrating string, two listening
    windows and the strings against the key). Clicking a string in the tunings picture plays its note. Opened from
    the Tuner's ? button and the last item of its right-click menu.
*/
class TuningExplainer final : public juce::Component,
                              public guideprint::Source,
                              private juce::ListBoxModel,
                              private juce::Timer
{
public:
    explicit TuningExplainer (Engine&);
    ~TuningExplainer() override;

    void showTopic (gen::tuning::Topic);

    juce::String getPrintTitle() const override { return "How to tune a guitar"; }
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
    void refreshPicture();
    void playString (gen::tuning::Tuning, int stringIndex);
    void stopTone();

    /** The tuning and reference pitch the Tuner window last saved. */
    gen::tuning::Tuning getSavedTuning() const;
    double getSavedReference() const;

    Engine& engine;
    int shownRow = 0;
    int rowBeforePrinting = -1;
    juce::uint32 toneEndsAt = 0;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::tuning };

    static constexpr int width = 1000, height = 740, pictureHeight = 330;
};
} // namespace daw
