#pragma once

#include "GuidePrint.h"
#include "GuideFooter.h"
#include "GuideSourcesPanel.h"
#include "StylePictures.h"
#include "generate/StylesGuide.h"

namespace daw
{
/** The Styles of music guide: fourteen audible style sketches, their parts and routing, and advice for this song. */
class StylesExplainer final : public juce::Component,
                              public guideprint::Source,
                              private juce::ListBoxModel,
                              private juce::ChangeListener,
                              private juce::Timer
{
public:
    explicit StylesExplainer (Engine&);
    ~StylesExplainer() override;

    /** Opens the Sources topic, the last in the list. */
    void showSources();

    /** Opens one of the guide's topics. */
    void showTopic (gen::stylesguide::Topic);

    /** Opens New Project from Style at the selected style. */
    std::function<void (const juce::String& styleId)> onNewSong;

    juce::String getPrintTitle() const override { return "Styles of music"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override { return (int) std::size (gen::stylesguide::allTopics) + 1; }   // and Sources
    booklet::Topic showPrintTopic (int) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    int getNumRows() override { return getNumPrintTopics(); }
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override;
    void selectedRowsChanged (int) override;
    void updateFooter (int row);
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void showRow (int);
    void playGroove();

    Engine& engine;
    juce::Label title;
    juce::ListBox list;
    GuideSourcesTopic sources { *this, "styles" };
    GuideFooter footer;
    juce::Component pictureHolder;
    StylePicture tempoPicture, arrangementPicture, groovePicture;
    juce::TextButton playButton { "Play the groove" }, newSongButton { "New song in this style..." };
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::styles };
    int rowBeforePrinting = -1;
    int grooveStep = 0;

    static constexpr int width = 1000, height = 760;
};
} // namespace daw
