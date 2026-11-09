#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/AboutInfo.h"

namespace daw
{
/**
    The About DawIt window: the version and build commit (with a Copy button for bug reports), that DawIt is free
    software under AGPL-3.0-only with no warranty, what that lets you do, where the source of this version is, and a
    button to the Licences window. It only lays out what `about::` returns; the text shrinks to fit the fixed size.
*/
class AboutComponent final : public juce::Component
{
public:
    /** The window's fixed size is 640 x 480. */
    AboutComponent();

    /** Called by the Licences... button. */
    std::function<void()> onShowLicences;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Block
    {
        juce::String text;
        juce::Colour colour;
        juce::Rectangle<int> area;
    };

    /** Lays the text blocks out at a font size; returns the bottom of the last block. */
    int layoutBlocks (float fontSize);

    juce::String versionLine, copyLine, sourceUrl;
    juce::Array<Block> blocks;
    float bodySize = 14.0f;
    juce::Rectangle<int> versionArea, titleArea, subtitleArea;
    juce::TextButton copyButton { "Copy" }, sourceButton { "Source code" }, licencesButton { "Licences..." };
};

/**
    The Licences window: a list on the left (grouped, with headings) of DawIt's licence and every third-party notice,
    and a read-only, selectable, scrolling text on the right with the full text. Everything is compiled in, so it works
    offline. Up and Down move through the list.
*/
class LicencesComponent final : public juce::Component, private juce::ListBoxModel
{
public:
    /** Opens on the licence of DawIt itself. Its opening size is 960 x 660. */
    LicencesComponent();
    ~LicencesComponent() override;

    /** Shows the notice with this file name (as `about::Notice::fileName`); false when there is none. */
    bool selectNotice (const juce::String& fileName);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row
    {
        bool isHeading = false;
        juce::String text;
        int notice = -1;      // index into notices for an entry
    };

    int getNumRows() override { return rows.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;

    void showNotice (int rowIndex);

    std::vector<about::Notice> notices;
    juce::Array<Row> rows;
    int currentRow = -1;
    juce::ListBox list { {}, this };
    juce::Label header;
    juce::TextEditor text;
};
}
