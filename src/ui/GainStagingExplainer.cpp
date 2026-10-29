#include "GainStagingExplainer.h"

namespace daw
{
using namespace gainstaging;

/** The selected track's chain as a line of levels from left to right: for every step what goes in and
    what comes out, over the quiet, healthy, hot and over zones, with the notes below it. */
class GainStagingExplainer::ChainPicture final : public juce::Component
{
public:
    std::function<void (Topic)> onHintClicked;

    void setSnapshot (Snapshot s)
    {
        snapshot = std::move (s);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        hintAreas.clear();

        auto r = getLocalBounds().reduced (14, 10);
        auto header = r.removeFromTop (22);
        auto hintsArea = r.removeFromBottom (76);
        auto namesArea = r.removeFromBottom (38);
        r.removeFromTop (6);
        auto plot = r.withTrimmedLeft (36).toFloat();

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (snapshot.trackName.isEmpty() ? juce::String ("Select a track to see its levels")
                                                 : "The levels of " + snapshot.trackName + ", right now",
                    header, juce::Justification::centredLeft, true);

        // The legend
        auto legend = header.removeFromRight (330);
        g.setFont (juce::FontOptions (12.0f));

        for (auto zone : { Zone::quiet, Zone::healthy, Zone::hot, Zone::over })
        {
            auto item = legend.removeFromLeft (zone == Zone::over ? 90 : 80);
            g.setColour (getZoneColour (zone));
            g.fillRoundedRectangle (item.removeFromLeft (12).withSizeKeepingCentre (10, 10).toFloat(), 2.0f);
            item.removeFromLeft (5);
            g.setColour (theme::textDim);
            g.drawText (getZoneName (zone), item, juce::Justification::centredLeft, false);
        }

        // The zones, from -60 dB at the bottom to +12 dB at the top.
        auto toY = [plot] (float db) { return plot.getBottom() - (juce::jlimit (bottomDb, topDb, db) - bottomDb) / (topDb - bottomDb) * plot.getHeight(); };

        const std::pair<float, float> ranges[] { { silentBelowDb, quietBelowDb }, { quietBelowDb, hotAboveDb }, { hotAboveDb, overAboveDb }, { overAboveDb, topDb } };
        const Zone zones[] { Zone::quiet, Zone::healthy, Zone::hot, Zone::over };

        for (int i = 0; i < 4; ++i)
        {
            g.setColour (getZoneColour (zones[i]).withAlpha (zones[i] == Zone::quiet ? 0.07f : 0.12f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (plot.getX(), toY (ranges[i].second), plot.getRight(), toY (ranges[i].first)));
        }

        g.setFont (juce::FontOptions (11.0f));

        for (float db : { 12.0f, 0.0f, -6.0f, -24.0f, -60.0f })
        {
            const float y = toY (db);
            g.setColour (theme::gridBar.withAlpha (db == 0.0f ? 1.0f : 0.5f));
            g.fillRect (plot.getX(), y, plot.getWidth(), db == 0.0f ? 1.5f : 1.0f);
            g.setColour (theme::textDim);
            g.drawText ((db > 0.0f ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (plot.getX() - 36.0f, y - 7.0f, 30.0f, 14.0f),
                        juce::Justification::centredRight, false);
        }

        // The sends are not steps of the line: they hang below it, each under the step it is taken after.
        std::vector<Stop> stops, sends;

        for (auto& stop : snapshot.stops)
            (stop.kind == StopKind::send ? sends : stops).push_back (stop);

        const bool playing = std::any_of (stops.begin(), stops.end(), [] (const Stop& s) { return juce::jmax (s.inDb, s.outDb) >= silentBelowDb; });

        if (stops.empty())
            return;

        const float columnWidth = plot.getWidth() / (float) stops.size();
        juce::Point<float> previous;
        bool hasPrevious = false;

        for (size_t i = 0; i < stops.size(); ++i)
        {
            const auto& stop = stops[i];
            const float left = plot.getX() + columnWidth * (float) i;
            const juce::Point<float> in { left + columnWidth * 0.28f, toY (stop.inDb) }, out { left + columnWidth * 0.72f, toY (stop.outDb) };

            if (i > 0)
            {
                g.setColour (theme::gridBar.withAlpha (0.4f));
                g.fillRect (left, plot.getY(), 1.0f, plot.getHeight());
            }

            // The name, and what the step does to the level.
            auto names = namesArea.toFloat().withX (left).withWidth (columnWidth).reduced (3.0f, 0.0f);
            g.setColour (stop.bypassed ? theme::textDim : theme::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawText (stop.name, names.removeFromTop (18.0f), juce::Justification::centred, true);
            g.setFont (juce::FontOptions (11.5f));
            g.setColour (theme::textDim);
            juce::String detail;

            if (stop.kind == StopKind::instrument)
                detail = "the sound starts here";
            else if (stop.kind == StopKind::busInput && stop.isReturn)
                detail = juce::String (stop.numTracks) + (stop.numTracks == 1 ? " send adds" : " sends add") + " up here";
            else if (stop.kind == StopKind::busInput)
                detail = juce::String (stop.numTracks) + (stop.numTracks == 1 ? " track adds" : " tracks add") + " up here";
            else if (stop.kind == StopKind::bus)
                detail = juce::String (juce::jmax (0, stop.numTracks - 1)) + " others, volume " + formatDb (stop.settingDb);
            else if (stop.kind == StopKind::effect)
                detail = stop.bypassed ? "off" : stop.matching ? "match level on" : stop.changeDb.has_value() ? "changes it " + formatDb (*stop.changeDb) : juce::String();
            else
                detail = (stop.kind == StopKind::master ? "all tracks, volume " : "volume ") + formatDb (stop.settingDb);

            g.drawText (detail, names, juce::Justification::centredTop, true);

            if (! playing)
                continue;

            auto lineFor = [&g] (juce::Point<float> a, juce::Point<float> b, float db, bool dashed)
            {
                juce::Path path;
                path.startNewSubPath (a);
                path.lineTo (b);
                g.setColour (getZoneColour (getZone (db)));

                if (dashed)
                {
                    const float dashes[] { 4.0f, 4.0f };
                    juce::Path dashedPath;
                    juce::PathStrokeType (2.0f).createDashedStroke (dashedPath, path, dashes, 2);
                    g.fillPath (dashedPath);
                }
                else
                {
                    g.strokePath (path, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
            };

            auto dot = [&g] (juce::Point<float> p, float db)
            {
                g.setColour (getZoneColour (getZone (db)));
                g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (p));
            };

            // Into a bus or the master come all the other tracks too: a dashed jump.
            if (stop.kind != StopKind::instrument && stop.kind != StopKind::busInput)
            {
                if (hasPrevious)
                    lineFor (previous, in, stops[i - 1].outDb, stop.kind == StopKind::master || stop.kind == StopKind::bus);

                lineFor (in, out, stop.outDb, false);
                dot (in, stop.inDb);
            }

            dot (out, stop.outDb);
            previous = out;
            hasPrevious = true;

            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (stop.outDb < silentBelowDb ? juce::String ("-inf") : (stop.outDb > 0.05f ? "+" : "") + juce::String (stop.outDb, 1),
                        juce::Rectangle<float> (64.0f, 14.0f).withCentre (out.translated (0.0f, out.y > plot.getY() + 20.0f ? -13.0f : 13.0f)),
                        juce::Justification::centred, false);
        }

        // The sends: a small box low in the picture under the step they are taken after, joined to the line by a branch.
        for (auto& send : sends)
        {
            const int column = juce::jlimit (0, (int) stops.size() - 1, juce::jmax (0, send.tapAfter));
            const float x = plot.getX() + columnWidth * ((float) column + 0.72f);
            const juce::Rectangle<float> box (x + 6.0f, plot.getBottom() - 24.0f, juce::jmin (columnWidth - 12.0f, 150.0f), 18.0f);
            g.setColour (theme::textDim.withAlpha (0.6f));
            g.drawLine (x, toY (stops[(size_t) column].outDb), x, box.getCentreY(), 1.2f);
            g.drawLine (x, box.getCentreY(), box.getX(), box.getCentreY(), 1.2f);
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (box, 4.0f);
            g.setColour (theme::gridBar);
            g.drawRoundedRectangle (box, 4.0f, 1.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.drawText (juce::String::fromUTF8 ("\xe2\x86\x92 ") + send.name + "  " + formatDb (send.settingDb) + (send.preFader ? "  PRE" : ""),
                        box.reduced (5.0f, 0.0f), juce::Justification::centredLeft, true);
        }

        // The markers, and what they say.
        for (auto& hint : snapshot.hints)
        {
            const float centre = plot.getX() + columnWidth * ((float) hint.stop + 0.5f);
            paintMarker (g, juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ centre, plot.getY() + 10.0f }), hint.serious);
        }

        auto notes = hintsArea.withTrimmedTop (6);
        g.setFont (juce::FontOptions (12.5f));

        if (! playing)
        {
            g.setColour (theme::textDim);
            g.drawText ("Play the song to see the levels.", notes, juce::Justification::topLeft, true);
            return;
        }

        if (snapshot.hints.empty())
        {
            g.setColour (theme::textDim);
            g.drawText ("Nothing stands out: every step has a sensible level.", notes, juce::Justification::topLeft, true);
            return;
        }

        for (size_t i = 0; i < snapshot.hints.size() && i < 2; ++i)
        {
            const auto& hint = snapshot.hints[i];
            auto line = notes.removeFromTop (35);
            paintMarker (g, line.removeFromLeft (22).removeFromTop (16).toFloat().withSizeKeepingCentre (14.0f, 14.0f), hint.serious);
            g.setColour (theme::text);
            g.drawFittedText (hint.text + " (click to read more)", line, juce::Justification::topLeft, 2, 1.0f);
            hintAreas.push_back ({ line, hint.topic });
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const bool onHint = std::any_of (hintAreas.begin(), hintAreas.end(), [&e] (auto& h) { return h.first.contains (e.getPosition()); });
        setMouseCursor (onHint ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        for (auto& [area, topic] : hintAreas)
            if (area.contains (e.getPosition()) && onHintClicked != nullptr)
                onHintClicked (topic);
    }

    /** A round ! in yellow, or red for a serious one. As on the device bar. */
    static void paintMarker (juce::Graphics& g, juce::Rectangle<float> area, bool serious)
    {
        g.setColour (serious ? theme::meterRed : theme::meterYellow);
        g.fillEllipse (area);
        g.setColour (theme::background);
        g.setFont (juce::FontOptions (area.getHeight() * 0.8f, juce::Font::bold));
        g.drawText ("!", area, juce::Justification::centred, false);
    }

private:
    static constexpr float topDb = 12.0f, bottomDb = silentBelowDb;

    Snapshot snapshot;
    std::vector<std::pair<juce::Rectangle<int>, Topic>> hintAreas;
};

//==============================================================================
GainStagingExplainer::GainStagingExplainer (std::function<Snapshot()> snapshotSource) : getSnapshot (std::move (snapshotSource))
{
    title.setText ("Gain staging: the level at every step", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<ChainPicture>();
    picture->onHintClicked = [this] (Topic topic) { showTopic (topic); };
    addAndMakeVisible (*picture);

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

    list.updateContent();
    showTopic (Topic::what);
    timerCallback();
    startTimerHz (15);
    setSize (width, height);
}

GainStagingExplainer::~GainStagingExplainer()
{
    list.setModel (nullptr);
}

juce::String GainStagingExplainer::getPrintSource() const
{
    const auto name = getSnapshot != nullptr ? getSnapshot().trackName : juce::String();
    return name.isNotEmpty() ? "the chain of the track " + name : juce::String ("no track selected");
}

int GainStagingExplainer::getNumPrintTopics() const
{
    return (int) std::size (allTopics);
}

booklet::Topic GainStagingExplainer::showPrintTopic (int index)
{
    if (! topicBeforePrinting)
        topicBeforePrinting = shown;

    showTopic (allTopics[juce::jlimit (0, getNumPrintTopics() - 1, index)]);

    if (getSnapshot != nullptr)
        picture->setSnapshot (getSnapshot());

    return { getTopicName (shown), getTopicSummary (shown), explanation.getText(), { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image GainStagingExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void GainStagingExplainer::endPrinting()
{
    if (topicBeforePrinting)
        showTopic (*topicBeforePrinting);

    topicBeforePrinting.reset();
}

void GainStagingExplainer::showTopic (Topic topic)
{
    shown = topic;
    const auto index = (int) (std::find (std::begin (allTopics), std::end (allTopics), topic) - std::begin (allTopics));

    if (list.getSelectedRow() != index)
        list.selectRow (index);   // comes back through selectedRowsChanged

    explanation.setText (explain (topic), false);
    explanation.moveCaretToTop (false);
}

int GainStagingExplainer::getNumRows()
{
    return (int) std::size (allTopics);
}

void GainStagingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void GainStagingExplainer::selectedRowsChanged (int row)
{
    if (row >= 0 && row < getNumRows())
        showTopic (allTopics[row]);
}

void GainStagingExplainer::timerCallback()
{
    if (getSnapshot != nullptr)
        picture->setSnapshot (getSnapshot());
}

void GainStagingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void GainStagingExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    picture->setBounds (r.removeFromTop (320));
    r.removeFromTop (12);

    explanation.setBounds (r);
}
} // namespace daw
