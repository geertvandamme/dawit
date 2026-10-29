#pragma once

#include "GuidePrint.h"
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

    /** Opens one of the guide's topics. */
    void showTopic (gen::stylesguide::Topic);

    /** Opens New Project from Style at the selected style. */
    std::function<void (const juce::String& styleId)> onNewSong;

    juce::String getPrintTitle() const override { return "Styles of music"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override { return (int) std::size (gen::stylesguide::allTopics); }
    booklet::Topic showPrintTopic (int) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int getNumRows() override { return getNumPrintTopics(); }
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override;
    void selectedRowsChanged (int) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void showRow (int);
    void playGroove();

    Engine& engine;
    juce::Label title;
    juce::ListBox list;
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
