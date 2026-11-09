#include "AboutWindow.h"
#include "Theme.h"
#include "BuildInfo.h"

namespace daw
{
namespace
{
    juce::String getVersion() { return juce::String (buildinfo::version); }
    juce::String getCommit() { return juce::String (buildinfo::commit); }

    /** The height a block of text takes at this font size and width. */
    int measureText (const juce::String& s, float size, int width)
    {
        juce::AttributedString a;
        a.setJustification (juce::Justification::topLeft);
        a.append (s, juce::FontOptions (size), juce::Colours::white);
        juce::TextLayout layout;
        layout.createLayout (a, (float) width);
        return (int) std::ceil (layout.getHeight());
    }
}

AboutComponent::AboutComponent()
{
    const auto version = getVersion();
    const auto commit = getCommit();
    versionLine = about::getVersionLine (version, commit);
    copyLine = about::getCopyLine (version, commit);

    const auto source = about::getSourceInfo (version, commit, buildinfo::hasLocalChanges, buildinfo::isExactTag,
                                              juce::String (buildinfo::sourceUrlOverride));
    sourceUrl = source.url;

    blocks.add ({ "Copyright (C) 2026 DawIt contributors.", theme::textDim, {} });
    blocks.add ({ about::getLicenceText(), theme::text, {} });
    blocks.add ({ about::getRightsText(), theme::text, {} });
    blocks.add ({ source.line, theme::text, {} });

    if (source.note.isNotEmpty())
        blocks.add ({ source.note, theme::textDim, {} });

    blocks.add ({ about::getThirdPartyText(), theme::textDim, {} });

    copyButton.setTooltip ("Copy the version and commit, for a bug report");
    copyButton.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (copyLine); };
    sourceButton.setTooltip ("Open the source code of this version on GitHub");
    sourceButton.onClick = [this] { juce::URL (sourceUrl).launchInDefaultBrowser(); };
    licencesButton.setTooltip ("Read the licence of DawIt and of every part it's built from. Works offline");
    licencesButton.onClick = [this] { if (onShowLicences) onShowLicences(); };

    for (auto* b : { &copyButton, &sourceButton, &licencesButton })
        addAndMakeVisible (*b);

    setSize (640, 480);
}

int AboutComponent::layoutBlocks (float fontSize)
{
    const int margin = 28;
    const int width = getWidth() - 2 * margin;
    int y = 118;
    const int buttonsBeforeIndex = 4;   // the buttons follow the source line (and its note)
    int index = 0;

    for (auto& b : blocks)
    {
        if (index == buttonsBeforeIndex + (blocks.size() == 6 ? 1 : 0))
            y += 40;   // room for the buttons row

        const int h = measureText (b.text, fontSize, width);
        b.area = { margin, y, width, h };
        y += h + 12;
        ++index;
    }

    return y;
}

void AboutComponent::resized()
{
    const int margin = 28;
    titleArea = { margin, 20, getWidth() - 2 * margin, 44 };
    subtitleArea = { margin, 64, getWidth() - 2 * margin, 20 };
    versionArea = { margin, 88, getWidth() - 2 * margin - 80, 22 };
    copyButton.setBounds (getWidth() - margin - 70, 88, 70, 24);

    // Shrink the text until everything fits above the bottom margin.
    bodySize = 14.0f;

    while (bodySize > 9.0f && layoutBlocks (bodySize) > getHeight() - 16)
        bodySize -= 0.5f;

    layoutBlocks (bodySize);

    const int buttonsIndex = blocks.size() == 6 ? 5 : 4;
    const int buttonsY = blocks[buttonsIndex - 1].area.getBottom() + 8;
    sourceButton.setBounds (margin, buttonsY, 120, 26);
    licencesButton.setBounds (margin + 132, buttonsY, 120, 26);
}

void AboutComponent::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    g.setColour (theme::accent);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText ("DawIt", titleArea, juce::Justification::centredLeft);

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("DAW In Theory", subtitleArea, juce::Justification::centredLeft, 1, 0.7f);
    g.drawFittedText (versionLine, versionArea, juce::Justification::centredLeft, 1, 0.7f);

    for (const auto& b : blocks)
    {
        juce::AttributedString a;
        a.setJustification (juce::Justification::topLeft);
        a.append (b.text, juce::FontOptions (bodySize), b.colour);
        a.draw (g, b.area.toFloat());
    }
}

LicencesComponent::LicencesComponent()
{
    notices = about::getNotices();

    about::Group last = about::Group::other;
    bool first = true;

    for (int i = 0; i < (int) notices.size(); ++i)
    {
        if (first || notices[(size_t) i].group != last)
        {
            rows.add ({ true, about::getGroupName (notices[(size_t) i].group), -1 });
            last = notices[(size_t) i].group;
            first = false;
        }

        rows.add ({ false, notices[(size_t) i].name, i });
    }

    list.setRowHeight (24);
    list.setColour (juce::ListBox::backgroundColourId, theme::lane);
    list.setTooltip ("Choose a licence or notice to read");
    addAndMakeVisible (list);

    header.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    header.setColour (juce::Label::textColourId, theme::text);
    header.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (header);

    text.setMultiLine (true, false);   // no wrapping: the texts are laid out by their makers
    text.setReadOnly (true);
    text.setCaretVisible (false);
    text.setScrollbarsShown (true);
    text.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    text.setColour (juce::TextEditor::backgroundColourId, theme::background);
    text.setColour (juce::TextEditor::textColourId, theme::text);
    text.setColour (juce::TextEditor::outlineColourId, theme::grid);
    text.setColour (juce::TextEditor::focusedOutlineColourId, theme::accent);
    text.setTooltip ("The full text, as its makers wrote it. Select and copy it with Cmd+C");
    addAndMakeVisible (text);

    showNotice (1);
    list.selectRow (1);

    setSize (960, 660);
}

LicencesComponent::~LicencesComponent() = default;

bool LicencesComponent::selectNotice (const juce::String& fileName)
{
    for (int i = 0; i < rows.size(); ++i)
        if (rows[i].notice >= 0 && notices[(size_t) rows[i].notice].fileName == fileName)
        {
            list.selectRow (i);
            list.scrollToEnsureRowIsOnscreen (i);
            return true;
        }

    return false;
}

void LicencesComponent::showNotice (int rowIndex)
{
    if (! juce::isPositiveAndBelow (rowIndex, rows.size()) || rows[rowIndex].isHeading)
        return;

    currentRow = rowIndex;
    const auto& notice = notices[(size_t) rows[rowIndex].notice];
    header.setText (about::getNoticeHeader (notice), juce::dontSendNotification);
    text.setText (notice.text, false);
    text.setCaretPosition (0);
    text.scrollEditorToPositionCaret (0, 0); // always show the top of a newly chosen text
    text.scrollEditorToPositionCaret (0, 0);
}

void LicencesComponent::selectedRowsChanged (int lastRowSelected)
{
    if (! juce::isPositiveAndBelow (lastRowSelected, rows.size()))
        return;

    if (! rows[lastRowSelected].isHeading)
    {
        showNotice (lastRowSelected);
        return;
    }

    // A heading can't be shown: step on in the direction of travel (or onto the first entry below it).
    const int step = currentRow > lastRowSelected ? -1 : 1;
    int target = lastRowSelected + step;

    while (juce::isPositiveAndBelow (target, rows.size()) && rows[target].isHeading)
        target += step;

    if (juce::isPositiveAndBelow (target, rows.size()))
        list.selectRow (target);
    else
        list.selectRow (currentRow);
}

void LicencesComponent::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (! juce::isPositiveAndBelow (row, rows.size()))
        return;

    const auto& r = rows.getReference (row);

    if (r.isHeading)
    {
        g.setColour (theme::panelLight);
        g.fillRect (0, 0, width, height);
        g.setColour (theme::accent);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawFittedText (r.text, { 10, 0, width - 14, height }, juce::Justification::centredLeft, 1, 0.7f);
        return;
    }

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, width, height);
    }

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (r.text, { 18, 0, width - 22, height }, juce::Justification::centredLeft, 1, 0.7f);
}

void LicencesComponent::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void LicencesComponent::resized()
{
    auto area = getLocalBounds().reduced (10);
    list.setBounds (area.removeFromLeft (270));
    area.removeFromLeft (10);
    header.setBounds (area.removeFromTop (44));
    text.setBounds (area);
}
}
