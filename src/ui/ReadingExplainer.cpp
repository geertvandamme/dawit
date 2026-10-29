#include "ReadingExplainer.h"
#include "ChordDiagramViews.h"
#include "ScorePainter.h"
#include "SheetMusicExport.h"

#include <map>

namespace daw
{
namespace guide = gen::readmusic;
namespace notation = gen::notation;

namespace
{
    juce::String getPitchName (const notation::WrittenPitch& p)
    {
        return juce::String::charToString ("CDEFGAB"[p.letter])
               + (p.alter > 0 ? juce::String::fromUTF8 ("\xe2\x99\xaf") : p.alter < 0 ? juce::String::fromUTF8 ("\xe2\x99\xad") : juce::String())
               + juce::String (p.octave);
    }
} // namespace

//==============================================================================
/** The topic's pictures: one or more staves drawn by `ScorePainter` from the example's scores, laid out in columns
    when there are several, shrinking to fit the height it is given. */
class ReadingExplainer::Picture final : public juce::Component,
                                        public juce::SettableTooltipClient
{
public:
    explicit Picture (Engine& e) : engine (e)
    {
        setTooltip ("The example. Click a note to hear it.");
    }

    void setExample (guide::Example e, const juce::String& emptyMessage, bool showTempo)
    {
        example = std::move (e);

        // A tempo mark over every example would only crowd the picture: the topics on tempo and on the part keep it.
        if (! showTempo)
            for (auto& panel : example.panels)
                panel.score.tempoMarks.clear();

        message = emptyMessage;
        keyboards.clear();

        for (auto& panel : example.panels)
        {
            if (panel.keyboardNotes.empty())
            {
                keyboards.push_back (nullptr);
                continue;
            }

            auto keys = std::make_unique<KeyboardDiagram>();
            const auto notes = panel.keyboardNotes;
            keys->setRange (*std::min_element (notes.begin(), notes.end()), *std::max_element (notes.begin(), notes.end()));
            keys->setChord (notes, -1, -1);
            keys->setTooltip ("The keys of these notes. Click to hear them one after another.");
            keys->onClick = [this, notes]
            {
                for (size_t i = 0; i < notes.size(); ++i)
                    juce::Timer::callAfterDelay ((int) i * 260, [safe = juce::Component::SafePointer<Picture> (this), n = notes[i]]
                    {
                        if (safe != nullptr)
                            safe->engine.previewNote (n, 0.8f, 300);
                    });
            };
            addAndMakeVisible (*keys);
            keyboards.push_back (std::move (keys));
        }

        playBeat = -1.0;
        layout (getWidth(), limit);
        repaint();
    }

    /** The beat being played, or -1: notes sounding then are lit. */
    void setPlayBeat (double beat)
    {
        if (! juce::approximatelyEqual (beat, playBeat))
        {
            playBeat = beat;
            repaint();
        }
    }

    void setLimit (int maxHeight) { limit = maxHeight; }

    /** The height the pictures take at this width, shrunk to fit `maxHeight` when they can. */
    int getPreferredHeight (int w, int maxHeight)
    {
        limit = maxHeight;
        return layout (w, maxHeight);
    }

    void resized() override { layout (getWidth(), limit); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        if (example.panels.empty())
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (14.0f));
            g.drawFittedText (message, getLocalBounds().reduced (16), juce::Justification::centred, 2);
            return;
        }

        hits.assign (example.panels.size(), {});
        scorepaint::Ink ink;
        ink.background = theme::panelLight;

        for (size_t i = 0; i < example.panels.size(); ++i)
        {
            auto& p = example.panels[i];
            auto& l = layouts[i];
            const auto origin = l.cell.getPosition().toFloat();

            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawFittedText (p.label, l.cell.withHeight (labelHeight).toNearestInt(), juce::Justification::centredLeft, 2, 0.85f);

            if (! p.played.empty())
                paintPlayed (g, p, l);

            const auto lit = [this, &p] (int staff, int bar, int index)
            {
                const auto& e = p.score.staves[(size_t) staff].bars[(size_t) bar].events[(size_t) index];

                if (e.isRest)
                    return false;

                const double start = bar * 4.0 + e.getStartBeat();

                if (playBeat >= start && playBeat < start + e.getBeats())
                    return true;

                for (auto& w : e.pitches)
                    if (std::find (p.accentNotes.begin(), p.accentNotes.end(), w.midi) != p.accentNotes.end())
                        return true;

                return false;
            };

            for (size_t k = 0; k < l.lines.size(); ++k)
            {
                const float top = origin.y + l.tops[k];
                const float x = origin.x + l.inset;
                scorepaint::drawSystem (g, p.score, l.lines[k], x, top, l.space, ink, {}, &hits[i], lit);

                if (p.showNoteNames)
                {
                    g.setColour (theme::text);
                    g.setFont (juce::FontOptions (juce::jlimit (11.0f, 13.5f, l.space * 1.4f), juce::Font::bold));
                    const float y = top + scorepaint::getStaffBlockHeight (p.score, l.space) + 3.2f * l.space;

                    for (auto& h : hits[i])
                        if (h.kind == scorepaint::HitKind::note && h.bounds.getY() >= top - 20.0f * l.space && h.bounds.getY() < top + 60.0f * l.space
                            && h.bar >= l.lines[k].firstBar && h.bar < l.lines[k].firstBar + l.lines[k].numBars)
                        {
                            const auto& e = p.score.staves[(size_t) h.staff].bars[(size_t) h.bar].events[(size_t) h.event];

                            if (! e.pitches.empty())
                                g.drawText (getPitchName (e.pitches.front()), juce::Rectangle<float> (h.bounds.getCentreX() - 20.0f, y, 40.0f, 16.0f),
                                            juce::Justification::centred, false);
                        }
                }
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (size_t i = 0; i < hits.size(); ++i)
            for (auto& h : hits[i])
                if (h.kind == scorepaint::HitKind::note && h.bounds.contains (e.position))
                {
                    const auto& event = example.panels[i].score.staves[(size_t) h.staff].bars[(size_t) h.bar].events[(size_t) h.event];

                    for (auto& w : event.pitches)
                        engine.previewNote (w.midi, 0.8f, 450);

                    return;
                }
    }

private:
    static constexpr int labelHeight = 22, playedHeight = 76, keyboardHeight = 84, namesHeight = 18, gap = 12;

    /** Where one panel's parts sit, relative to its cell. */
    struct PanelLayout
    {
        juce::Rectangle<int> cell;
        float space = 9.0f;
        float inset = 0.0f;
        std::vector<notation::Line> lines;
        std::vector<float> tops;                // the first staff's top line of each line, from the cell's top
        juce::Rectangle<float> played;          // relative to the cell
    };

    /** Lays the panels out in columns at one size of staff, and returns the height. */
    int layoutAt (int w, float scale)
    {
        const int n = (int) example.panels.size();
        layouts.assign ((size_t) n, {});
        const int cols = n >= 3 ? 3 : 1;
        const int cellWidth = (w - 24 - (cols - 1) * gap) / cols;
        std::vector<int> heights ((size_t) n, 0);

        for (int i = 0; i < n; ++i)
        {
            auto& p = example.panels[(size_t) i];
            auto& l = layouts[(size_t) i];
            l.space = 9.0f * scale;
            l.inset = p.score.isGrand() ? 1.8f * l.space : 0.3f * l.space;

            if (p.numBars >= 0)
            {
                const auto full = notation::layoutLines (p.score, 1.0e9, false).front();
                const int first = juce::jlimit (0, p.score.numBars - 1, p.firstBar);
                const int count = juce::jmax (1, juce::jmin (p.numBars, p.score.numBars - first));
                notation::Line line;
                line.firstBar = first;
                line.numBars = count;
                line.hasTimeSignature = first == 0;
                line.scale = full.scale;
                line.headerWidth = notation::getHeaderWidth (p.score, line.hasTimeSignature);

                for (int k = 0; k < count; ++k)
                    line.barOrigins.push_back (line.headerWidth + (full.barOrigins[(size_t) (first + k)] - full.barOrigins[(size_t) first]));

                line.width = line.barOrigins.back() + p.score.barWidths[(size_t) (first + count - 1)] * line.scale;
                l.space = juce::jlimit (6.0f, l.space, (float) (cellWidth - l.inset) / (float) line.width);
                l.lines = { line };
            }
            else
            {
                l.lines = notation::layoutLines (p.score, ((double) cellWidth - l.inset) / l.space, false);
            }

            float y = (float) labelHeight;

            if (! p.played.empty())
            {
                l.played = { 0.0f, y, (float) cellWidth, (float) playedHeight - 6.0f };
                y += (float) playedHeight;
            }

            const float block = scorepaint::getStaffBlockHeight (p.score, l.space);

            for (auto& line : l.lines)
            {
                y += scorepaint::getLineTopMargin (p.score, line, l.space);
                l.tops.push_back (y);
                y += block + 3.2f * l.space + (p.showNoteNames ? (float) namesHeight : 0.0f);
            }

            if (keyboards[(size_t) i] != nullptr)
                y += 6.0f + (float) keyboardHeight;

            heights[(size_t) i] = (int) std::ceil (y) + 6;
        }

        int total = 12, rowTop = 6;

        for (int row = 0; row * cols < n; ++row)
        {
            int rowHeight = 0;

            for (int c = 0; c < cols && row * cols + c < n; ++c)
                rowHeight = juce::jmax (rowHeight, heights[(size_t) (row * cols + c)]);

            for (int c = 0; c < cols && row * cols + c < n; ++c)
            {
                const int i = row * cols + c;
                auto& l = layouts[(size_t) i];
                l.cell = { 12 + c * (cellWidth + gap), rowTop, cellWidth, heights[(size_t) i] };

                if (auto* keys = keyboards[(size_t) i].get())
                {
                    const int keyTop = l.cell.getY() + heights[(size_t) i] - 6 - keyboardHeight;
                    keys->setBounds (l.cell.getX() + (int) l.inset, keyTop, cellWidth - (int) l.inset, keyboardHeight);
                }
            }

            rowTop += rowHeight;
            total += rowHeight;
        }

        return total;
    }

    int layout (int w, int maxHeight)
    {
        if (example.panels.empty())
            return 70;

        int h = 0;

        for (float scale = 1.0f; scale >= 0.6f; scale -= 0.1f)
        {
            h = layoutAt (juce::jmax (200, w), scale);

            if (h <= maxHeight)
                break;
        }

        return h;
    }

    /** The MIDI blocks as played, over the score, with a line down from each start to where the notes are written. */
    void paintPlayed (juce::Graphics& g, const guide::Panel& p, const PanelLayout& l)
    {
        const auto& line = l.lines.front();
        const auto origin = l.cell.getPosition().toFloat();

        // Beat to x: through where the written events sit, so the blocks line up with the notes below.
        std::map<double, double> points;

        for (int b = line.firstBar; b < line.firstBar + line.numBars; ++b)
        {
            const double barX = line.barOrigins[(size_t) (b - line.firstBar)];

            for (auto& staff : p.score.staves)
                for (auto& e : staff.bars[(size_t) b].events)
                    points[b * 4.0 + e.getStartBeat()] = barX + e.x * line.scale + 0.59;

            points[(b + 1) * 4.0] = barX + p.score.barWidths[(size_t) b] * line.scale - 0.1;
        }

        const auto toX = [&] (double beat)
        {
            auto after = points.lower_bound (beat);

            if (after == points.begin())
                return after->second;

            if (after == points.end())
                return std::prev (after)->second;

            const auto before = std::prev (after);
            const double span = after->first - before->first;
            return before->second + (span > 0.0 ? (beat - before->first) / span : 0.0) * (after->second - before->second);
        };

        int low = 127, high = 0;

        for (auto& b : p.played)
        {
            low = juce::jmin (low, b.pitch);
            high = juce::jmax (high, b.pitch);
        }

        const auto area = l.played.translated (origin.x + l.inset, origin.y);
        const float step = juce::jlimit (4.0f, 12.0f, (area.getHeight() - 6.0f) / (float) (high - low + 1));
        const float staffTop = origin.y + l.tops.front();

        for (auto& b : p.played)
        {
            const float x0 = origin.x + l.inset + (float) toX (b.start) * l.space;
            const float x1 = origin.x + l.inset + (float) toX (b.start + b.length) * l.space;
            const float y = area.getY() + 3.0f + (float) (high - b.pitch) * step;

            g.setColour (theme::text.withAlpha (0.18f));
            g.fillRect (x0, y + step, 1.0f, staffTop - y - step);

            g.setColour (theme::accent.withAlpha (0.85f));
            g.fillRoundedRectangle (x0, y, juce::jmax (4.0f, x1 - x0 - 3.0f), step, 2.0f);
        }
    }

    Engine& engine;
    guide::Example example;
    juce::String message;
    std::vector<PanelLayout> layouts;
    std::vector<std::unique_ptr<KeyboardDiagram>> keyboards;
    std::vector<std::vector<scorepaint::Hit>> hits;
    double playBeat = -1.0;
    int limit = 100000;
};

//==============================================================================
ReadingExplainer::ReadingExplainer (Engine& e, Selection& s) : engine (e), selection (s)
{
    title.setText ("How to read music", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture> (engine);
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

    playButton.setTooltip ("Play the example once, on the selected MIDI track, lighting each note as it sounds");
    playButton.onClick = [this] { playing ? stop() : play(); };
    addAndMakeVisible (playButton);
    addAndMakeVisible (exportButton);

    engine.addChangeListener (this);
    selection.addChangeListener (this);

    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
}

ReadingExplainer::~ReadingExplainer()
{
    stop();
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

gen::readmusic::Part ReadingExplainer::makePart() const
{
    Track* track = selection.getTrack();

    if (track == nullptr || ! track->isMidi())
    {
        track = nullptr;

        for (auto& t : engine.getTracks())
            if (t->isMidi())
            {
                const juce::ScopedLock lock (engine.getLock());
                bool hasNotes = false;

                for (auto& clip : t->clips)
                    if (clip->kind == Clip::Kind::midi && ! static_cast<MidiClip*> (clip.get())->notes.empty())
                        hasNotes = true;

                if (hasNotes)
                {
                    track = t.get();
                    break;
                }
            }
    }

    if (track != nullptr)
        return sheetmusic::getPart (engine, *track, engine.getPositionBeats());

    guide::Part part;
    const juce::ScopedLock lock (engine.getLock());
    part.input.key = engine.getKey();
    part.input.tempo = engine.getTempoMap();
    return part;
}

juce::String ReadingExplainer::getPrintSource() const
{
    const auto part = makePart();
    return part.hasTrack ? "example scores, and the part " + part.trackName : juce::String ("example scores");
}

int ReadingExplainer::getNumPrintTopics() const
{
    return (int) std::size (guide::allTopics);
}

booklet::Topic ReadingExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    printHeight = picture->getPreferredHeight (picture->getWidth(), 100000);   // all of it, not what fits the window
    return { guide::getTopicName (guide::allTopics[index]), guide::getTopicSummary (guide::allTopics[index]), explanation.getText(),
             { picture->getWidth(), printHeight }, {} };
}

juce::Image ReadingExplainer::paintPrintPicture (float scale)
{
    picture->setLimit (100000);
    picture->setSize (picture->getWidth(), printHeight);
    auto image = guideprint::snapshot (*picture, scale);
    resized();
    return image;
}

void ReadingExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void ReadingExplainer::showTopic (guide::Topic topic)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (guide::allTopics[i] == topic)
            list.selectRow (i);
}

int ReadingExplainer::getNumRows()
{
    return (int) std::size (guide::allTopics);
}

void ReadingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = guide::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (guide::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (guide::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void ReadingExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        show (row);
}

void ReadingExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // "Your part" follows the selected track and the song; the others use the song's key and tempo.
    if (! playing && juce::isPositiveAndBelow (shownRow, getNumRows()))
    {
        const auto topic = guide::allTopics[shownRow];

        if (topic == guide::Topic::yours || topic == guide::Topic::keySignatures || topic == guide::Topic::timeAndTempo)
            show (shownRow);
    }
}

void ReadingExplainer::show (int index)
{
    stop();
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    const auto topic = guide::allTopics[shownRow];
    const auto part = makePart();
    auto example = guide::getExample (topic, part);
    playNotes = example.playNotes;
    picture->setExample (std::move (example),
                         topic == guide::Topic::yours ? juce::String (part.hasTrack ? "This track has no notes yet." : "Select a MIDI track to see its part written out here.")
                                                      : juce::String(),
                         topic == guide::Topic::yours || topic == guide::Topic::timeAndTempo);
    explanation.setText (guide::explain (topic, part), false);
    explanation.moveCaretToTop (false);
    playButton.setEnabled (! playNotes.empty());
    resized();
}

//==============================================================================
void ReadingExplainer::play()
{
    if (playNotes.empty())
        return;

    engine.setPreviewSound (BuiltInSound::piano);   // heard when no MIDI track is selected
    playing = true;
    playTotal = 0.0;

    for (auto& n : playNotes)
        playTotal = juce::jmax (playTotal, n.getEnd());

    playStartMs = juce::Time::getMillisecondCounterHiRes() + 30.0;
    lastBeats = -1.0e-6;
    playButton.setButtonText ("Stop");
    startTimer (5);
}

void ReadingExplainer::stop()
{
    stopTimer();
    playing = false;
    playButton.setButtonText ("Play");

    if (picture != nullptr)
        picture->setPlayBeat (-1.0);
}

void ReadingExplainer::timerCallback()
{
    const double msPerBeat = 60000.0 / juce::jmax (1.0, engine.getTempo());
    const double now = (juce::Time::getMillisecondCounterHiRes() - playStartMs) / msPerBeat;

    if (now >= playTotal + 0.25)
    {
        stop();
        return;
    }

    if (now < 0.0)
        return;

    // Everything that starts between the last tick and this one sounds now.
    for (auto& n : playNotes)
        if (n.start > lastBeats && n.start <= now)
            engine.previewNote (n.pitch, n.velocity, juce::jmax (40, juce::roundToInt (n.length * msPerBeat * 0.9)));

    lastBeats = now;
    picture->setPlayBeat (now);
}

//==============================================================================
void ReadingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void ReadingExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    top.removeFromRight (8);
    playButton.setBounds (top.removeFromRight (90));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    const int pictureWidth = r.getWidth();
    const int maxHeight = juce::jmax (120, r.getHeight() * 11 / 20);
    const int pictureHeight = juce::jmin (picture->getPreferredHeight (pictureWidth, maxHeight), r.getHeight() * 3 / 4);
    picture->setBounds (r.removeFromTop (pictureHeight));
    r.removeFromTop (10);
    explanation.setBounds (r);
}
} // namespace daw
