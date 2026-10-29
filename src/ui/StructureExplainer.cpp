#include "StructureExplainer.h"
#include "engine/Tempo.h"
#include "SectionLane.h"

namespace daw
{
namespace structure = gen::structure;

/** A structure as coloured blocks in proportion to their bars, with the energy curve above them and
    the time each section starts below. */
class StructureExplainer::Strip final : public juce::Component
{
public:
    void show (const SectionList& newSections, double newBpm, const juce::String& newCaption, bool withTimes = true)
    {
        sections = newSections;
        bpm = newBpm;
        caption = newCaption;
        showTimes = withTimes;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (r, 4.0f);
        r.reduce (12.0f, 8.0f);

        auto top = r.removeFromTop (16.0f);
        g.setFont (juce::FontOptions (12.0f));
        g.setColour (theme::textDim);
        g.drawText ("Energy", top, juce::Justification::centredLeft);
        g.drawText (caption, top, juce::Justification::centredRight);

        if (sections.empty())
        {
            g.drawText ("No sections", r, juce::Justification::centred);
            return;
        }

        const int first = sections.front().startBar;
        const int total = juce::jmax (1, sections.back().getEndBar() - first);
        const auto curveArea = r.removeFromTop (44.0f).reduced (0.0f, 4.0f);
        auto blocks = r.removeFromTop (32.0f);
        const auto times = r.removeFromTop (16.0f);

        auto xOf = [&] (int bar) { return blocks.getX() + blocks.getWidth() * (float) (bar - first) / (float) total; };

        // The energy curve through the middle of each section, from 1 (bottom) to 5 (top).
        juce::Path curve;
        std::vector<juce::Point<float>> points;

        for (auto& s : sections)
        {
            const float x = (xOf (s.startBar) + xOf (s.getEndBar())) * 0.5f;
            const float y = curveArea.getBottom() - curveArea.getHeight() * (float) (sectionTypes::getEnergy (s.type) - 1) / 4.0f;
            points.push_back ({ x, y });
        }

        curve.startNewSubPath (points.front());
        for (size_t i = 1; i < points.size(); ++i)
            curve.lineTo (points[i]);

        g.setColour (theme::text.withAlpha (0.25f));
        for (int level = 0; level < 5; ++level)
            g.fillRect (curveArea.getX(), curveArea.getBottom() - curveArea.getHeight() * (float) level / 4.0f, curveArea.getWidth(), 1.0f);

        g.setColour (theme::accent);
        g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        for (size_t i = 0; i < points.size(); ++i)
        {
            g.setColour (SectionLane::getColour (sections[i].type).brighter (0.4f));
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (points[i]));
        }

        blocks.removeFromTop (4.0f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));

        for (auto& s : sections)
        {
            const auto block = juce::Rectangle<float> (xOf (s.startBar), blocks.getY(), xOf (s.getEndBar()) - xOf (s.startBar), blocks.getHeight())
                                   .reduced (1.0f, 0.0f);
            g.setColour (SectionLane::getColour (s.type));
            g.fillRoundedRectangle (block, 3.0f);
            g.setColour (theme::text);
            g.drawFittedText (s.getDisplayName(), block.reduced (4.0f, 0.0f).toNearestInt(), juce::Justification::centred, 2, 0.6f);

            if (showTimes && block.getWidth() > 30.0f)
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (11.0f));
                g.drawText (structure::formatDuration (s.startBar - first, bpm), juce::Rectangle<float> (block.getX(), times.getY(), block.getWidth(), times.getHeight()),
                            juce::Justification::bottomLeft);
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            }
        }
    }

private:
    SectionList sections;
    double bpm = 120.0;
    juce::String caption;
    bool showTimes = true;
};

//==============================================================================
StructureExplainer::StructureExplainer (Engine& e, int bar) : engine (e), startBar (juce::jmax (0, bar))
{
    title.setText ("Song structure", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    strip = std::make_unique<Strip>();
    addAndMakeVisible (*strip);

    putButton.setTooltip ("Lay out this structure on the Sections row, starting at the bar you clicked");
    putButton.onClick = [this] { putOnSectionTrack(); };
    addAndMakeVisible (putButton);

    explanation.setMultiLine (true, true);
    explanation.setReadOnly (true);
    explanation.setCaretVisible (false);
    explanation.setScrollbarsShown (true);
    explanation.setFont (juce::FontOptions (15.0f));
    explanation.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    explanation.setColour (juce::TextEditor::textColourId, theme::text);
    explanation.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    explanation.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    explanation.setBorder (juce::BorderSize<int> (10, 12, 10, 12));
    addAndMakeVisible (explanation);
    addAndMakeVisible (exportButton);

    hintLabel.setFont (juce::FontOptions (12.5f));
    hintLabel.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (hintLabel);

    engine.addChangeListener (this);

    list.updateContent();
    list.selectRow (engine.getSections().empty() ? introRow : songRow);
    setSize (width, height);
}

StructureExplainer::~StructureExplainer()
{
    engine.removeChangeListener (this);
}

void StructureExplainer::setStartBar (int bar)
{
    startBar = juce::jmax (0, bar);
    updatePutButton();
}

juce::String StructureExplainer::getPrintSource() const
{
    const int n = (int) engine.getSections().size();
    return n > 0 ? "the song's " + juce::String (n) + (n == 1 ? " section" : " sections") : juce::String ("a song with no sections yet");
}

int StructureExplainer::getNumPrintTopics() const
{
    return firstStructureRow + (int) structure::getStructures().size();
}

booklet::Topic StructureExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = juce::jmax (0, list.getSelectedRow());

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    showTopic (index);
    const auto [name, summary] = getRowText (index);
    return { name, summary, explanation.getText(), { strip->getWidth(), strip->getHeight() }, {} };
}

juce::Image StructureExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*strip, scale);
}

void StructureExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        showTopic (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void StructureExplainer::showTopic (int row)
{
    list.selectRow (juce::jlimit (0, getNumRows() - 1, row));
}

//==============================================================================
int StructureExplainer::getNumRows()
{
    return firstStructureRow + (int) structure::getStructures().size();
}

std::pair<juce::String, juce::String> StructureExplainer::getRowText (int row) const
{
    juce::String name, summary;

    if (row == introRow)
    {
        name = "How songs are built";
        summary = "Sections, phrases, repetition and energy";
    }
    else if (row == sectionsRow)
    {
        name = "The sections";
        summary = "What an intro, verse, chorus or drop is for";
    }
    else if (row == songRow)
    {
        const auto& sections = engine.getSections();
        name = "Your song's structure";
        summary = sections.empty() ? juce::String ("No sections yet") : structure::findStructureName (sections);

        if (summary.isEmpty())
            summary = juce::String ((int) sections.size()) + " sections, " + juce::String (sections.back().getEndBar() - sections.front().startBar) + " bars";
    }
    else if (const auto* s = row - firstStructureRow < (int) structure::getStructures().size()
                                 ? &structure::getStructures()[(size_t) (row - firstStructureRow)] : nullptr)
    {
        name = s->name;
        summary = s->styles;
    }

    return { name, summary };
}

void StructureExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto [name, summary] = getRowText (row);

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (name, 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (summary, 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void StructureExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        showRow (row);
}

void StructureExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // The song's sections or tempo may have changed.
    list.repaintRow (songRow);

    if (const int row = list.getSelectedRow(); row == songRow || row >= firstStructureRow)
        showRow (row);
}

const gen::structure::Structure* StructureExplainer::getSelectedStructure() const
{
    const int index = list.getSelectedRow() - firstStructureRow;
    const auto& structures = structure::getStructures();
    return index >= 0 && index < (int) structures.size() ? &structures[(size_t) index] : nullptr;
}

void StructureExplainer::showRow (int row)
{
    const double bpm = engine.getTempo();

    if (const auto* s = getSelectedStructure(); s != nullptr && row >= firstStructureRow)
    {
        strip->show (s->toSections (0), s->typicalBpm, juce::String (s->getTotalBars()) + " bars, "
                                                           + structure::formatDuration (s->getTotalBars(), s->typicalBpm) + " at "
                                                           + juce::String (juce::roundToInt (s->typicalBpm)) + " BPM");
        explanation.setText (structure::explainStructure (*s, bpm), false);
    }
    else if (row == songRow)
    {
        const auto& sections = engine.getSections();
        const int bars = sections.empty() ? 0 : sections.back().getEndBar() - sections.front().startBar;
        strip->show (sections, bpm, sections.empty() ? juce::String()
                                                     : juce::String (bars) + " bars, " + structure::formatDuration (bars, bpm) + " at "
                                                           + formatBpm (bpm) + " BPM");
        explanation.setText (structure::explainSongStructure (sections, bpm), false);
    }
    else if (row == sectionsRow)
    {
        // Every type once, 4 bars each, to show their colours and energy.
        SectionList all;
        for (auto t : sectionTypes::all)
            all.push_back ({ (int) all.size() * 4, 4, t, {} });

        strip->show (all, bpm, "Every section type and its usual energy", false);
        explanation.setText (structure::explainSections(), false);
    }
    else
    {
        const auto& pop = structure::getStructures().front();
        strip->show (pop.toSections (0), pop.typicalBpm, "Example: " + pop.name);
        explanation.setText (structure::getIntroduction(), false);
    }

    explanation.moveCaretToTop (false);
    updatePutButton();
}

void StructureExplainer::updatePutButton()
{
    const auto* s = getSelectedStructure();

    if (putButton.isVisible() != (s != nullptr))
    {
        putButton.setVisible (s != nullptr);
        resized();
    }

    if (s != nullptr)
        putButton.setButtonText ("Put on section track at bar " + juce::String (startBar + 1));
}

void StructureExplainer::putOnSectionTrack()
{
    const auto* s = getSelectedStructure();

    if (s == nullptr)
        return;

    const auto added = s->toSections (startBar);
    const int first = startBar, end = startBar + s->getTotalBars();
    int replaced = 0;

    for (auto& existing : engine.getSections())
        if (existing.startBar < end && existing.getEndBar() > first)
            ++replaced;

    auto apply = [safeThis = juce::Component::SafePointer<StructureExplainer> (this), added, first, end, name = s->name]
    {
        auto* self = safeThis.getComponent();

        if (self == nullptr)
            return;

        // Sections in the way go; the ones before and after stay.
        SectionList sections;
        for (auto& existing : self->engine.getSections())
            if (existing.getEndBar() <= first || existing.startBar >= end)
                sections.push_back (existing);

        sections.insert (sections.end(), added.begin(), added.end());
        self->engine.setSections (std::move (sections));
        self->hintLabel.setText ("Put the " + name + " structure on bars " + juce::String (first + 1) + " to " + juce::String (end) + ".",
                                 juce::dontSendNotification);
    };

    if (replaced == 0)
    {
        apply();
        return;
    }

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Replace sections?",
                                        "Bars " + juce::String (first + 1) + " to " + juce::String (end) + " already have "
                                            + juce::String (replaced) + (replaced == 1 ? " section" : " sections")
                                            + ". Replace them with the " + s->name + " structure?",
                                        "Replace", "Cancel", this,
                                        juce::ModalCallbackFunction::create ([apply] (int result) { if (result == 1) apply(); }));
}

//==============================================================================
void StructureExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void StructureExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    hintLabel.setBounds (r.removeFromBottom (20));
    r.removeFromBottom (6);

    list.setBounds (r.removeFromLeft (260));
    r.removeFromLeft (14);

    strip->setBounds (r.removeFromTop (128));
    r.removeFromTop (10);

    if (putButton.isVisible())
    {
        putButton.setBounds (r.removeFromTop (28).removeFromLeft (260));
        r.removeFromTop (10);
    }

    explanation.setBounds (r);
}
} // namespace daw
