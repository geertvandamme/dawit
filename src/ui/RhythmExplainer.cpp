#include "RhythmExplainer.h"
#include "engine/Tempo.h"

namespace daw
{
namespace rhythm = gen::rhythm;

namespace
{
    /** `count` notes of `length` beats one after the other from `start`. */
    std::vector<RhythmExplainer::Hit> notes (double length, int count, double start = 0.0)
    {
        std::vector<RhythmExplainer::Hit> hits;
        for (int i = 0; i < count; ++i)
            hits.push_back ({ start + i * length, length, false, 0.8f, {} });

        return hits;
    }

    /** Notes and rests taking turns: each pair is (length, rest). */
    std::vector<RhythmExplainer::Hit> pattern (std::initializer_list<std::pair<double, bool>> parts)
    {
        std::vector<RhythmExplainer::Hit> hits;
        double at = 0.0;

        for (auto& [length, rest] : parts)
        {
            hits.push_back ({ at, length, rest, 0.8f, {} });
            at += length;
        }

        return hits;
    }

    /** A written note or rest, drawn small: open or filled head, stem, flags, a dot, a triplet's 3. */
    void drawSymbol (juce::Graphics& g, juce::Rectangle<float> area, const RhythmExplainer::Symbol& s)
    {
        if (s.beats <= 0.0)
            return;

        g.setColour (theme::text);
        const int flags = s.beats <= 0.125 ? 3 : s.beats <= 0.25 ? 2 : s.beats <= 0.5 ? 1 : 0;
        const float cx = area.getX() + 9.0f, cy = area.getBottom() - 9.0f;

        if (s.rest)
        {
            if (s.beats >= 2.0)
            {
                // Whole rest hangs below the line, half rest sits on it.
                const float lineY = area.getCentreY();
                g.fillRect (area.getX() + 2.0f, lineY, 16.0f, 1.0f);
                g.fillRect (area.getX() + 5.0f, s.beats >= 4.0 ? lineY : lineY - 5.0f, 10.0f, 5.0f);
            }
            else if (s.beats >= 1.0)
            {
                juce::Path zigzag;
                const float x = area.getX() + 6.0f, y = area.getY() + 8.0f;
                zigzag.startNewSubPath (x, y);
                zigzag.lineTo (x + 7.0f, y + 8.0f);
                zigzag.lineTo (x + 1.0f, y + 15.0f);
                zigzag.lineTo (x + 8.0f, y + 22.0f);
                zigzag.quadraticTo (x - 1.0f, y + 20.0f, x + 4.0f, y + 30.0f);
                g.strokePath (zigzag, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            else
            {
                const float x = area.getX() + 12.0f, top = area.getY() + 10.0f;
                g.drawLine (x, top, x - 6.0f, top + 22.0f, 1.6f);

                for (int i = 0; i < flags; ++i)
                {
                    g.fillEllipse (x - 10.0f - i * 1.5f, top + i * 7.0f - 1.0f, 5.0f, 5.0f);
                    g.drawLine (x - 8.0f - i * 1.5f, top + i * 7.0f + 3.0f, x - i * 1.5f, top + i * 7.0f, 1.4f);
                }
            }

            return;
        }

        juce::Path head;
        head.addEllipse (-5.0f, -3.6f, 10.0f, 7.2f);
        head.applyTransform (juce::AffineTransform::rotation (-0.35f).translated (cx, cy));

        if (s.beats >= 2.0)
            g.strokePath (head, juce::PathStrokeType (1.6f));
        else
            g.fillPath (head);

        if (s.beats < 4.0)
        {
            const float stemX = cx + 4.4f, top = area.getY() + 4.0f;
            g.fillRect (stemX, top, 1.4f, cy - top - 1.0f);

            for (int i = 0; i < flags; ++i)
            {
                juce::Path flag;
                flag.startNewSubPath (stemX + 1.0f, top + i * 6.0f);
                flag.quadraticTo (stemX + 9.0f, top + i * 6.0f + 6.0f, stemX + 6.0f, top + i * 6.0f + 13.0f);
                g.strokePath (flag, juce::PathStrokeType (1.5f));
            }
        }

        if (s.dotted)
            g.fillEllipse (cx + 8.0f, cy - 1.5f, 3.2f, 3.2f);

        if (s.triplet)
        {
            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText ("3", juce::Rectangle<float> (cx + 7.0f, area.getY() - 2.0f, 12.0f, 12.0f), juce::Justification::centred);
        }
    }
}

//==============================================================================
std::vector<RhythmExplainer::Row> RhythmExplainer::getRows (rhythm::Topic topic, double bpm)
{
    using T = rhythm::Topic;
    std::vector<Row> rows;

    switch (topic)
    {
        case T::barsAndBeats:
        {
            rows.push_back ({ "4/4", "Four beats per bar", { 1.0 }, notes (1.0, 4) });
            rows.back().hits[0].velocity = 1.0f;

            rows.push_back ({ "3/4", "Three beats: the waltz", { 1.0 }, notes (1.0, 3), 3 });
            rows.back().hits[0].velocity = 1.0f;

            // 6/8: six eighths felt as two beats of three.
            Row sixEight { "6/8", "Two beats of three eighths", { 0.5 }, notes (0.5, 6), 3 };
            sixEight.hits[0].velocity = sixEight.hits[3].velocity = 1.0f;

            for (int i = 0; i < 6; ++i)
                sixEight.hits[(size_t) i].label = juce::String (i + 1);

            rows.push_back (sixEight);
            break;
        }

        case T::noteValues:
            for (auto& v : rhythm::getNoteValues())
                rows.push_back ({ juce::String (v.name).substring (0, 1).toUpperCase() + juce::String (v.name).substring (1),
                                  rhythm::formatBeats (v.beats) + ", Snap " + v.fraction, { v.beats },
                                  notes (v.beats, (int) std::round (4.0 / v.beats)) });
            break;

        case T::rests:
            rows.push_back ({ "Quarter notes", "No rests: four in a row", { 1.0 }, notes (1.0, 4) });
            rows.push_back ({ "Quarter rests", "Note, rest, note, rest", { 1.0, true },
                              pattern ({ { 1.0, false }, { 1.0, true }, { 1.0, false }, { 1.0, true } }) });
            rows.push_back ({ "Eighth rests", "Short notes with space", { 0.5, true },
                              pattern ({ { 0.5, false }, { 0.5, true }, { 0.5, false }, { 0.5, true },
                                         { 0.5, false }, { 0.5, true }, { 0.5, false }, { 0.5, true } }) });
            rows.push_back ({ "A line that breathes", "A half rest at the end", { 2.0, true },
                              pattern ({ { 1.0, false }, { 0.5, false }, { 0.5, false }, { 2.0, true } }) });
            break;

        case T::dottedAndTriplets:
            rows.push_back ({ "Dotted quarter + eighth", "1.5 + 0.5 beats, long-short", { 1.0, false, true },
                              pattern ({ { 1.5, false }, { 0.5, false }, { 1.5, false }, { 0.5, false } }) });
            rows.push_back ({ "Dotted eighth + sixteenth", "0.75 + 0.25 beats, bouncy", { 0.5, false, true },
                              pattern ({ { 0.75, false }, { 0.25, false }, { 0.75, false }, { 0.25, false },
                                         { 0.75, false }, { 0.25, false }, { 0.75, false }, { 0.25, false } }) });
            rows.push_back ({ "Eighth-note triplets", "Three per beat", { 0.5, false, false, true }, notes (1.0 / 3.0, 12) });
            rows.push_back ({ "Quarter-note triplets", "Three in two beats", { 1.0, false, false, true }, notes (2.0 / 3.0, 6) });
            rows.push_back ({ "Swing eighths", "Long-short, like a triplet", { 0.5, false, false, true },
                              pattern ({ { 2.0 / 3.0, false }, { 1.0 / 3.0, false }, { 2.0 / 3.0, false }, { 1.0 / 3.0, false },
                                         { 2.0 / 3.0, false }, { 1.0 / 3.0, false }, { 2.0 / 3.0, false }, { 1.0 / 3.0, false } }) });
            break;

        case T::counting:
            rows.push_back ({ "1 2 3 4", "Quarter notes", { 1.0 }, notes (1.0, 4) });
            rows.push_back ({ "1 & 2 &", "Eighth notes", { 0.5 }, notes (0.5, 8) });
            rows.push_back ({ "1 e & a", "Sixteenth notes", { 0.25 }, notes (0.25, 16) });
            rows.push_back ({ "1 trip let", "Eighth-note triplets", { 0.5, false, false, true }, notes (1.0 / 3.0, 12) });
            rows.push_back ({ "On 2 and 4", "The backbeat (snare)", { 1.0 },
                              pattern ({ { 1.0, true }, { 1.0, false }, { 1.0, true }, { 1.0, false } }) });
            rows.push_back ({ "On the &s", "Off-beats (hi-hat)", { 0.5 },
                              pattern ({ { 0.5, true }, { 0.5, false }, { 0.5, true }, { 0.5, false },
                                         { 0.5, true }, { 0.5, false }, { 0.5, true }, { 0.5, false } }) });
            break;

        case T::tempo:
            rows.push_back ({ "One bar at " + formatBpm (bpm) + " BPM", "Each beat's start in seconds",
                              { 1.0 }, notes (1.0, 4), 4, Labels::seconds });
            rows.push_back ({ "Eighth notes", "Twice as many, half as long", { 0.5 }, notes (0.5, 8), 4, Labels::seconds });
            break;

        case T::readingDawIt:
        {
            rows.push_back ({ "Beats of bar 1", "bar.beat.sixteenth", { 1.0 }, notes (1.0, 4), 4, Labels::position, 1 });
            rows.push_back ({ "Sixteenths of a beat", "The last number counts them", { 0.25 }, notes (0.25, 4, 2.0), 4, Labels::position, 1 });

            Row single { "A note at 5.3.2", "Bar 5, beat 3, second sixteenth", { 0.25 }, { { 2.25, 0.25, false, 0.8f, {} } }, 4, Labels::position, 5 };
            rows.push_back (single);
            break;
        }
    }

    return rows;
}

//==============================================================================
/** The example rhythms of a topic, one per row: its symbol and name, then one bar of blocks with
    the beat lines, and the count (or time, or position) under every note. */
class RhythmExplainer::RowsView final : public juce::Component
{
public:
    std::function<void (int)> onRowClicked;

    void setRows (std::vector<Row> newRows, double newBpm)
    {
        rows = std::move (newRows);
        bpm = newBpm;
        selected = rows.empty() ? -1 : 0;
        playingRow = playingHit = -1;
        repaint();
    }

    void setSelected (int row) { selected = row; repaint(); }
    int getSelected() const { return selected; }

    void setPlaying (int row, int hit)
    {
        if (row != playingRow || hit != playingHit)
        {
            playingRow = row;
            playingHit = hit;
            repaint();
        }
    }

    int getRowHeight() const { return rows.empty() ? 60 : juce::jmin (64, getHeight() / (int) rows.size()); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int row = (int) (e.position.y / (float) getRowHeight());

        if (row >= 0 && row < (int) rows.size())
        {
            setSelected (row);

            if (onRowClicked)
                onRowClicked (row);
        }
    }

    void paint (juce::Graphics& g) override
    {
        const int rowHeight = getRowHeight();

        for (int i = 0; i < (int) rows.size(); ++i)
        {
            const auto& row = rows[(size_t) i];
            auto r = juce::Rectangle<float> (0.0f, (float) (i * rowHeight), (float) getWidth(), (float) rowHeight).reduced (0.0f, 3.0f);

            g.setColour (i == selected ? theme::accent.withAlpha (0.22f) : theme::panelLight);
            g.fillRoundedRectangle (r, 4.0f);

            auto left = r.removeFromLeft (230.0f).reduced (8.0f, 4.0f);
            drawSymbol (g, left.removeFromLeft (30.0f), row.symbol);
            left.removeFromLeft (6.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
            g.drawFittedText (row.name, left.removeFromTop (left.getHeight() * 0.5f).toNearestInt(), juce::Justification::bottomLeft, 1, 0.8f);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.0f));
            g.drawFittedText (row.detail, left.toNearestInt(), juce::Justification::topLeft, 1, 0.8f);

            // One bar, four beats wide (a 3/4 bar is three of them).
            auto bar = r.reduced (10.0f, 6.0f);
            const auto labels = bar.removeFromBottom (14.0f);
            const float beatWidth = bar.getWidth() / 4.0f;
            auto xOf = [&] (double beat) { return bar.getX() + (float) beat * beatWidth; };

            g.setColour (theme::background.withAlpha (0.6f));
            g.fillRect (bar.withWidth (beatWidth * (float) row.beatsPerBar));

            for (int step = 0; step <= row.beatsPerBar * 4; ++step)
            {
                const bool beat = step % 4 == 0;
                g.setColour (beat ? theme::gridBar : theme::grid.withAlpha (0.7f));
                g.fillRect (xOf (step / 4.0), bar.getY(), beat ? 1.5f : 1.0f, bar.getHeight());
            }

            for (int h = 0; h < (int) row.hits.size(); ++h)
            {
                const auto& hit = row.hits[(size_t) h];
                const auto block = juce::Rectangle<float> (xOf (hit.start), bar.getCentreY() - 8.0f,
                                                           (float) hit.length * beatWidth, 16.0f).reduced (1.5f, 0.0f);
                const bool lit = i == playingRow && h == playingHit;

                if (hit.rest)
                {
                    g.setColour (theme::textDim.withAlpha (lit ? 0.9f : 0.45f));
                    g.drawRoundedRectangle (block, 3.0f, 1.0f);

                    if (block.getWidth() > 30.0f)
                    {
                        g.setFont (juce::FontOptions (10.5f));
                        g.drawText ("rest", block, juce::Justification::centred);
                    }
                }
                else
                {
                    const auto colour = theme::accent.withMultipliedBrightness (0.6f + 0.4f * hit.velocity);
                    g.setColour (lit ? juce::Colours::white : colour);
                    g.fillRoundedRectangle (block, 3.0f);
                }

                auto text = hit.label;

                if (text.isEmpty())
                {
                    switch (row.labels)
                    {
                        case Labels::count:    text = rhythm::countAt (hit.start); break;
                        case Labels::seconds:  text = juce::String (hit.start * 60.0 / juce::jmax (1.0, bpm), 2) + " s"; break;
                        case Labels::position: text = rhythm::formatPosition ((row.bar - 1) * 4.0 + hit.start); break;
                    }
                }

                const float labelWidth = juce::jmax (block.getWidth() + 3.0f, row.labels == Labels::count ? 0.0f : 44.0f);

                if (text.isNotEmpty() && labelWidth >= 10.0f)
                {
                    g.setColour (lit ? theme::text : theme::textDim);
                    g.setFont (juce::FontOptions (11.0f, lit ? juce::Font::bold : juce::Font::plain));
                    g.drawText (text, juce::Rectangle<float> (block.getX(), labels.getY(), labelWidth, labels.getHeight()),
                                juce::Justification::centredLeft, false);
                }
            }
        }
    }

private:
    std::vector<Row> rows;
    double bpm = 120.0;
    int selected = -1, playingRow = -1, playingHit = -1;
};

//==============================================================================
RhythmExplainer::RhythmExplainer (Engine& e) : engine (e)
{
    title.setText ("Note lengths, beats and bars", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    rowsView = std::make_unique<RowsView>();
    rowsView->onRowClicked = [this] (int) { if (playingRow >= 0) play(); };
    addAndMakeVisible (*rowsView);

    playButton.setTooltip ("Play the selected rhythm twice, on the selected MIDI track");
    playButton.onClick = [this] { playingRow >= 0 ? stop() : play(); };
    addAndMakeVisible (playButton);

    beatToggle.setToggleState (true, juce::dontSendNotification);
    beatToggle.setTooltip ("Play a low note on every beat under the rhythm, so you hear where the beats are");
    addAndMakeVisible (beatToggle);

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
    list.selectRow (1);   // note values: what most people come for
    setSize (width, height);
}

RhythmExplainer::~RhythmExplainer()
{
    stop();
    engine.removeChangeListener (this);
}

int RhythmExplainer::getNumRows()
{
    return (int) std::size (rhythm::allTopics);
}

void RhythmExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = rhythm::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (rhythm::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (rhythm::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void RhythmExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        showRow (row);
}

void RhythmExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // The tempo, or the track the examples play on, may have changed.
    if (const int row = list.getSelectedRow(); row >= 0 && rhythm::allTopics[row] == rhythm::Topic::tempo && playingRow < 0)
        showRow (row);
}

juce::String RhythmExplainer::getPrintSource() const
{
    return "the song's tempo, " + juce::String (juce::roundToInt (engine.getTempo())) + " BPM";
}

int RhythmExplainer::getNumPrintTopics() const
{
    return (int) std::size (rhythm::allTopics);
}

booklet::Topic RhythmExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = juce::jmax (0, list.getSelectedRow());

    index = juce::jlimit (0, getNumRows() - 1, index);
    showRow (index);   // not through the list, so nothing else changes
    const auto topic = rhythm::allTopics[index];
    return { rhythm::getTopicName (topic), rhythm::getTopicSummary (topic), explanation.getText(), { rowsView->getWidth(), rowsView->getHeight() }, {} };
}

juce::Image RhythmExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*rowsView, scale);
}

void RhythmExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        showRow (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void RhythmExplainer::showTopic (rhythm::Topic topic)
{
    const auto index = (int) (std::find (std::begin (rhythm::allTopics), std::end (rhythm::allTopics), topic) - std::begin (rhythm::allTopics));

    if (list.getSelectedRow() != index)
        list.selectRow (index);   // comes back through selectedRowsChanged
}

void RhythmExplainer::showRow (int index)
{
    stop();
    const auto topic = rhythm::allTopics[juce::jlimit (0, getNumRows() - 1, index)];
    rows = getRows (topic, engine.getTempo());
    rowsView->setRows (rows, engine.getTempo());
    explanation.setText (rhythm::explain (topic, engine.getTempo()), false);
    explanation.moveCaretToTop (false);

    const auto* track = engine.getLiveMidiTrack();
    hintLabel.setText (! engine.previewsUseOwnSound() ? "Click a rhythm, then Play: it sounds on " + track->name + "."
                                                      : juce::String ("Click a rhythm, then Play: it sounds on DawIt's piano, or on a MIDI track when you select one."),
                       juce::dontSendNotification);
    resized();
}

//==============================================================================
void RhythmExplainer::play()
{
    engine.setPreviewSound (BuiltInSound::piano);   // heard when no MIDI track is selected

    stop();
    playingRow = rowsView->getSelected();

    if (playingRow < 0 || playingRow >= (int) rows.size())
    {
        playingRow = -1;
        return;
    }

    playStartMs = juce::Time::getMillisecondCounterHiRes() + 30.0;
    lastBeats = -1.0e-6;
    playButton.setButtonText ("Stop");
    startTimer (5);
}

void RhythmExplainer::stop()
{
    stopTimer();
    playingRow = -1;
    playButton.setButtonText ("Play");

    if (rowsView != nullptr)
        rowsView->setPlaying (-1, -1);
}

void RhythmExplainer::timerCallback()
{
    const auto& row = rows[(size_t) playingRow];
    const double msPerBeat = 60000.0 / juce::jmax (1.0, engine.getTempo());
    const double now = (juce::Time::getMillisecondCounterHiRes() - playStartMs) / msPerBeat;
    const double total = row.beatsPerBar * (double) timesThrough;

    if (now >= total)
    {
        stop();
        return;
    }

    if (now < 0.0)
        return;

    // Everything that starts between the last tick and this one sounds now.
    for (int pass = 0; pass < timesThrough; ++pass)
    {
        const double offset = pass * row.beatsPerBar;

        for (auto& hit : row.hits)
            if (! hit.rest && offset + hit.start > lastBeats && offset + hit.start <= now)
                engine.previewNote (72, hit.velocity, juce::jmax (40, juce::roundToInt (hit.length * msPerBeat * 0.85)));

        for (int beat = 0; beat < row.beatsPerBar && beatToggle.getToggleState(); ++beat)
            if (offset + beat > lastBeats && offset + beat <= now)
                engine.previewNote (48, beat == 0 ? 0.55f : 0.35f, juce::roundToInt (msPerBeat * 0.4));
    }

    lastBeats = now;

    const double inBar = std::fmod (now, (double) row.beatsPerBar);
    int lit = -1;

    for (int h = 0; h < (int) row.hits.size(); ++h)
        if (inBar >= row.hits[(size_t) h].start && inBar < row.hits[(size_t) h].start + row.hits[(size_t) h].length)
            lit = h;

    rowsView->setPlaying (playingRow, lit);
}

//==============================================================================
void RhythmExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void RhythmExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    hintLabel.setBounds (r.removeFromBottom (20));
    r.removeFromBottom (6);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    rowsView->setBounds (r.removeFromTop (juce::jmax (1, (int) rows.size()) * 64));
    r.removeFromTop (8);

    auto buttons = r.removeFromTop (28);
    playButton.setBounds (buttons.removeFromLeft (90));
    buttons.removeFromLeft (12);
    beatToggle.setBounds (buttons.removeFromLeft (160));
    r.removeFromTop (10);

    explanation.setBounds (r);
}
} // namespace daw
