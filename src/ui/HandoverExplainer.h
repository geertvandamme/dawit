#pragma once

#include "engine/Engine.h"
#include "engine/HandoverGuide.h"
#include "GuidePrint.h"
#include "GuideFooter.h"
#include "GuideSourcesPanel.h"
#include "Theme.h"

namespace daw
{
/**
    The "Sending stems to an engineer" guide: stems, multitracks and the mix, what a mixing or a mastering engineer
    wants, preparing the files, headroom, printed or dry effects, checking that the files add up, what to send with
    them and working through revisions. Each of the first eight topics has a picture drawn from the pop example
    song; the last, "Your song's stems", draws your own song as the lanes Bounce to WAV would write, with the mix's
    last measured peak beside them and advice below. A button there opens Bounce to WAV. It follows the song, the
    last Measure Loudness and the remembered rate and bit depth, recomputing on a short timer after a change.
*/
class HandoverExplainer final : public juce::Component,
                                public guideprint::Source,
                                private juce::ListBoxModel,
                                private juce::ChangeListener,
                                private juce::Timer
{
public:
    explicit HandoverExplainer (Engine&);
    ~HandoverExplainer() override;

    /** The "Bounce to WAV..." button on the last topic. */
    std::function<void()> onShowBounce;

    /** Opens the Sources topic, the last in the list. */
    void showSources();

    void showTopic (handoverguide::Topic);

    juce::String getPrintTitle() const override { return "Sending stems to an engineer"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void updateFooter();
    void changeListenerCallback (juce::ChangeBroadcaster*) override { dirty = true; }
    void timerCallback() override;

    void show (int index);
    handoverguide::Inputs makeInputs() const;
    void refresh (bool force);

    Engine& engine;
    int shownRow = 0;
    int rowBeforePrinting = -1;
    bool dirty = true;
    handoverguide::Inputs lastInputs;
    bool haveInputs = false;

    juce::Label title;
    juce::ListBox list;
    GuideSourcesTopic sources { *this, "handover" };
    GuideFooter footer;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    juce::TextButton bounceButton { "Bounce to WAV..." };
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::handover };

    static constexpr int width = 1000, height = 760, pictureHeight = 330;
};
} // namespace daw
