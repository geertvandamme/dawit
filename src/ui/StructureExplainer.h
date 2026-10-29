#pragma once

#include "engine/Engine.h"
#include "generate/StructureGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    A guide to song structure for beginning musicians, opened from the Sections row's menu. It explains
    how songs are built and what each section is for, shows the typical structures of different styles
    of music as a coloured strip with their energy curve, explains the song's own sections, and puts a
    structure on the section track from the bar that was clicked.
*/
class StructureExplainer final : public juce::Component,
                                 public guideprint::Source,
                                 private juce::ListBoxModel,
                                 private juce::ChangeListener
{
public:
    StructureExplainer (Engine&, int startBar);
    ~StructureExplainer() override;

    /** The first bar "Put on section track" writes to. */
    void setStartBar (int bar);

    // The list: an introduction, the sections, the song's own structure, then the typical structures.
    enum { introRow = 0, sectionsRow = 1, songRow = 2, firstStructureRow = 3 };

    /** Selects a row of the list (from the Theory Guides window). */
    void showTopic (int row);

    juce::String getPrintTitle() const override { return "Song structure"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Strip;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void showRow (int row);

    /** A row of the list: its name and the line under it. */
    std::pair<juce::String, juce::String> getRowText (int row) const;
    const gen::structure::Structure* getSelectedStructure() const;
    void updatePutButton();
    void putOnSectionTrack();

    Engine& engine;
    int startBar = 0;

    juce::Label title, hintLabel;
    juce::ListBox list;
    std::unique_ptr<Strip> strip;
    juce::TextButton putButton;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::structure };
    int rowBeforePrinting = -1;

    static constexpr int width = 940, height = 700;
};
} // namespace daw
