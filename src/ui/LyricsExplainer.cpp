#include "LyricsExplainer.h"
#include "LyricsLane.h"
#include "SectionLane.h"

namespace daw
{
namespace guide = gen::lyricsguide;

juce::Colour LyricsExplainer::getRhymeColour (int rhyme)
{
    static const juce::Colour colours[] { juce::Colour (0xffffb454), juce::Colour (0xff5cc8ff), juce::Colour (0xff7ee787),
                                          juce::Colour (0xffff7eb6), LyricsLane::colour, juce::Colour (0xff4fd1c5) };
    return rhyme < 0 ? theme::textDim : colours[rhyme % (int) std::size (colours)];
}

//==============================================================================
/** The topic's example: lines of words, syllables on beats, or syllables under melody notes. */
class LyricsExplainer::Picture final : public juce::Component,
                                    public juce::SettableTooltipClient
{
public:
    void setExample (guide::Example e)
    {
        example = std::move (e);
        repaint();
    }

    int getPreferredHeight() const
    {
        const int lines = juce::jmax (1, (int) example.lines.size());

        switch (example.kind)
        {
            case guide::Kind::words:  return lines * wordsRow + 12;
            case guide::Kind::beats:  return headerHeight + lines * beatsRow + 8;
            case guide::Kind::melody: return headerHeight + lines * melodyRow + 8;
            case guide::Kind::sounds: return headerHeight + lines * soundsRow + 8;
        }

        return 100;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

        switch (example.kind)
        {
            case guide::Kind::words:  paintWords (g); break;
            case guide::Kind::beats:
            case guide::Kind::melody: paintGrid (g); break;
            case guide::Kind::sounds: paintSounds (g); break;
        }
    }

    /** Called with a MIDI pitch and a length in milliseconds to play a note of the sounds example; set by the guide. */
    std::function<void (int, int)> onPlayNote;

    void mouseDown (const juce::MouseEvent&) override
    {
        // A click plays the example at 120 BPM: a beat is half a second.
        if (example.kind != guide::Kind::sounds || ! onPlayNote)
            return;

        for (auto& line : example.lines)
        {
            for (auto& token : line.tokens)
            {
                if (token.pitch < 0)
                    continue;

                const int pitch = token.pitch, length = juce::jmax (80, (int) (token.length * 500.0) - 40);
                juce::Timer::callAfterDelay ((int) (token.beat * 500.0), [safe = juce::Component::SafePointer<Picture> (this), pitch, length]
                {
                    if (safe != nullptr && safe->onPlayNote)
                        safe->onPlayNote (pitch, length);
                });
            }
        }
    }

private:
    static constexpr int wordsRow = 24, beatsRow = 50, melodyRow = 78, soundsRow = 150, headerHeight = 22, labelWidth = 128;

    /** A line's label: a section chip, a rhyme letter in its colour, or a name. */
    void paintLabel (juce::Graphics& g, const guide::ExampleLine& line, juce::Rectangle<float> area)
    {
        if (line.section.has_value())
        {
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            const float w = juce::jmin (area.getWidth(), juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), line.label) + 14.0f);
            const auto chip = area.withWidth (w).withSizeKeepingCentre (w, 19.0f);
            g.setColour (SectionLane::getColour (*line.section));
            g.fillRoundedRectangle (chip, 3.0f);
            g.setColour (theme::text);
            g.drawText (line.label, chip, juce::Justification::centred, true);
            return;
        }

        g.setColour (line.labelRhyme >= 0 ? getRhymeColour (line.labelRhyme) : theme::text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (line.label, area, juce::Justification::centredLeft, true);
    }

    /** How wide a line's words (or chips) are drawn. */
    static float getTokensWidth (const guide::ExampleLine& line)
    {
        float total = 0.0f;

        for (auto& token : line.tokens)
        {
            const juce::Font font (juce::FontOptions (token.section.has_value() ? 12.0f : 14.5f,
                                                      token.section.has_value() || token.stressed || token.rhyme >= 0 ? juce::Font::bold : juce::Font::plain));
            total += token.section.has_value() ? juce::GlyphArrangement::getStringWidth (font, token.text) + 16.0f
                                               : juce::GlyphArrangement::getStringWidth (font, token.text + " ");
        }

        return total;
    }

    void paintWords (juce::Graphics& g)
    {
        // The notes start after the longest line, as far as there's room.
        float widest = 0.0f;

        for (auto& line : example.lines)
            widest = juce::jmax (widest, getTokensWidth (line));

        const float tokensX = 12.0f + (float) labelWidth;
        const float notesX = juce::jlimit (tokensX + 160.0f, juce::jmax (tokensX + 160.0f, (float) getWidth() - 190.0f), tokensX + widest + 20.0f);
        float y = 6.0f;

        for (auto& line : example.lines)
        {
            const auto row = juce::Rectangle<float> (12.0f, y, (float) getWidth() - 24.0f, (float) wordsRow);
            paintLabel (g, line, row.withWidth ((float) labelWidth - 12.0f));

            // The words, or section chips.
            float x = row.getX() + (float) labelWidth;
            {
                juce::Graphics::ScopedSaveState state (g);
                g.reduceClipRegion (juce::Rectangle<float> (x, y, notesX - x - 12.0f, (float) wordsRow).toNearestInt());

                for (auto& token : line.tokens)
                {
                    if (token.section.has_value())
                    {
                        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                        const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), token.text) + 12.0f;
                        const auto chip = juce::Rectangle<float> (x, y + 3.0f, w, (float) wordsRow - 6.0f);
                        g.setColour (SectionLane::getColour (*token.section));
                        g.fillRoundedRectangle (chip, 3.0f);
                        g.setColour (theme::text);
                        g.drawText (token.text, chip, juce::Justification::centred, false);
                        x += w + 4.0f;
                        continue;
                    }

                    const juce::Font font (juce::FontOptions (14.5f, token.stressed || token.rhyme >= 0 ? juce::Font::bold : juce::Font::plain));
                    g.setFont (font);
                    g.setColour (token.wrong ? theme::meterRed.withAlpha (0.9f) : token.rhyme >= 0 ? getRhymeColour (token.rhyme)
                                                                                 : token.stressed ? theme::accent : theme::text);
                    g.drawSingleLineText (token.text, juce::roundToInt (x), juce::roundToInt (y + (float) wordsRow * 0.5f + 5.0f));

                    if (token.wrong && token.text.isNotEmpty())
                    {
                        // The original in Writing with AI: struck out.
                        const float strikeY = y + (float) wordsRow * 0.5f;
                        g.drawLine (x, strikeY, x + juce::GlyphArrangement::getStringWidth (font, token.text), strikeY, 1.2f);
                    }

                    x += juce::GlyphArrangement::getStringWidth (font, token.text + " ");
                }
            }

            if (line.note.isNotEmpty())
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (12.5f));
                g.drawFittedText (line.note, juce::Rectangle<float> (notesX, y, row.getRight() - notesX, (float) wordsRow).toNearestInt(),
                                  juce::Justification::centredLeft, 1, 0.8f);
            }

            y += (float) wordsRow;
        }
    }

    void paintGrid (juce::Graphics& g)
    {
        const bool melody = example.kind == guide::Kind::melody;
        const float rowHeight = (float) (melody ? melodyRow : beatsRow);
        const float gridX = 12.0f + (float) labelWidth, gridRight = (float) getWidth() - 12.0f;
        const double beats = example.endBeat - example.startBeat;
        const float beatWidth = (float) ((gridRight - gridX) / beats);
        auto xOf = [&] (double beat) { return gridX + (float) (beat - example.startBeat) * beatWidth; };

        // The count over the grid: 1 2 3 4, and the pickup before the bar (or a whole bar before it, dimmed).
        g.setFont (juce::FontOptions (11.5f));
        const bool pickupOnly = example.startBeat > -1.5;

        for (int b = (int) std::floor (example.startBeat); b < (int) std::ceil (example.endBeat); ++b)
        {
            const int inBar = ((b % 4) + 4) % 4;
            const bool pickup = b < 0 && pickupOnly;
            g.setColour (b < 0 ? theme::textDim.withAlpha (0.7f) : inBar == 0 ? theme::text : theme::textDim);
            g.drawText (pickup ? juce::String ("pickup") : juce::String (inBar + 1), juce::Rectangle<float> (xOf (b) + 3.0f, 2.0f, beatWidth * (pickup ? 2.0f : 1.0f), 18.0f),
                        juce::Justification::centredLeft, false);
        }

        // Melody notes share one pitch range.
        int low = 127, high = 0;

        for (auto& line : example.lines)
            for (auto& t : line.tokens)
                if (t.pitch >= 0)
                {
                    low = juce::jmin (low, t.pitch);
                    high = juce::jmax (high, t.pitch);
                }

        float y = (float) headerHeight;

        for (auto& line : example.lines)
        {
            const auto row = juce::Rectangle<float> (gridX, y + 2.0f, gridRight - gridX, rowHeight - 6.0f);

            // Strong beats (1 and 3) shaded, bar lines brighter.
            for (int b = (int) std::floor (example.startBeat); b < (int) std::ceil (example.endBeat); ++b)
            {
                const int inBar = ((b % 4) + 4) % 4;

                if (b >= 0 && inBar % 2 == 0)
                {
                    g.setColour (theme::text.withAlpha (inBar == 0 ? 0.07f : 0.045f));
                    g.fillRect (juce::Rectangle<float> (xOf (b), row.getY(), beatWidth, row.getHeight()));
                }

                g.setColour (inBar == 0 ? theme::gridBar : theme::grid);
                g.fillRect (xOf (b), row.getY(), 1.0f, row.getHeight());
            }

            // The label, with what it shows under it.
            auto labelArea = juce::Rectangle<float> (12.0f, row.getY(), (float) labelWidth - 14.0f, row.getHeight());
            paintLabel (g, line, labelArea.removeFromTop (20.0f));
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText (line.note, labelArea.toNearestInt(), juce::Justification::topLeft, 3, 0.85f);

            const float textBaseline = melody ? row.getBottom() - 5.0f : row.getCentreY() + 5.5f;

            for (auto& token : line.tokens)
            {
                const float x = xOf (token.beat);

                if (melody && token.pitch >= 0)
                {
                    const float noteArea = row.getHeight() - 26.0f;
                    const float step = juce::jlimit (3.0f, 9.0f, noteArea / (float) juce::jmax (1, high - low + 1));
                    const float noteY = row.getY() + 4.0f + (float) (high - token.pitch) * step;
                    const auto note = juce::Rectangle<float> (x + 1.0f, noteY, juce::jmax (3.0f, (float) token.length * beatWidth - 2.0f), step + 2.0f);
                    g.setColour (token.stressed ? theme::accent : theme::accent.withAlpha (0.6f));
                    g.fillRoundedRectangle (note, 2.0f);
                }

                if (token.text.isEmpty())
                {
                    // The syllable before is held over this note.
                    g.setColour (theme::textDim);
                    g.fillRect (x + 2.0f, textBaseline - 1.0f, juce::jmax (4.0f, (float) token.length * beatWidth - 6.0f), 1.5f);
                    continue;
                }

                const juce::Font font (juce::FontOptions (melody ? 13.5f : 14.5f, token.stressed ? juce::Font::bold : juce::Font::plain));
                g.setFont (font);
                g.setColour (token.wrong ? theme::meterRed : token.stressed ? theme::text : theme::textDim);
                g.drawSingleLineText (token.text, juce::roundToInt (x + 3.0f), juce::roundToInt (textBaseline));

                if (! melody)
                {
                    // A tick where the syllable starts.
                    g.setColour (token.wrong ? theme::meterRed : token.stressed ? theme::accent : theme::textDim.withAlpha (0.6f));
                    g.fillRect (x, row.getY() + 3.0f, 2.0f, 7.0f);
                }
            }

            y += rowHeight;
        }
    }

    /** The melody of a sung example with each word's sounds as blocks under its note: vowels bright, consonants dim. */
    void paintSounds (juce::Graphics& g)
    {
        const float gridX = 12.0f + (float) labelWidth, gridRight = (float) getWidth() - 12.0f;
        const double beats = example.endBeat - example.startBeat;
        const float beatWidth = (float) ((gridRight - gridX) / beats);
        auto xOf = [&] (double beat) { return gridX + (float) (beat - example.startBeat) * beatWidth; };

        g.setFont (juce::FontOptions (11.5f));

        for (int b = (int) std::floor (example.startBeat); b < (int) std::ceil (example.endBeat); ++b)
        {
            const int inBar = ((b % 4) + 4) % 4;
            g.setColour (b < 0 ? theme::textDim.withAlpha (0.7f) : inBar == 0 ? theme::text : theme::textDim);
            g.drawText (b < 0 ? juce::String ("before") : juce::String (inBar + 1), juce::Rectangle<float> (xOf (b) + 3.0f, 2.0f, beatWidth, 18.0f),
                        juce::Justification::centredLeft, false);
        }

        int low = 127, high = 0;

        for (auto& line : example.lines)
            for (auto& t : line.tokens)
                if (t.pitch >= 0)
                {
                    low = juce::jmin (low, t.pitch);
                    high = juce::jmax (high, t.pitch);
                }

        float y = (float) headerHeight;

        for (auto& line : example.lines)
        {
            const auto row = juce::Rectangle<float> (gridX, y + 2.0f, gridRight - gridX, (float) soundsRow - 6.0f);

            for (int b = (int) std::floor (example.startBeat); b < (int) std::ceil (example.endBeat); ++b)
            {
                const int inBar = ((b % 4) + 4) % 4;

                if (b >= 0 && inBar % 2 == 0)
                {
                    g.setColour (theme::text.withAlpha (inBar == 0 ? 0.07f : 0.045f));
                    g.fillRect (juce::Rectangle<float> (xOf (b), row.getY(), beatWidth, row.getHeight()));
                }

                g.setColour (inBar == 0 ? theme::gridBar : theme::grid);
                g.fillRect (xOf (b), row.getY(), 1.0f, row.getHeight());
            }

            auto labelArea = juce::Rectangle<float> (12.0f, row.getY(), (float) labelWidth - 14.0f, row.getHeight());
            paintLabel (g, line, labelArea.removeFromTop (20.0f));
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.5f));
            g.drawFittedText (line.note, labelArea.toNearestInt(), juce::Justification::topLeft, 4, 0.85f);

            const float step = juce::jlimit (4.0f, 12.0f, 56.0f / (float) juce::jmax (1, high - low + 1));
            const float textY = row.getY() + 70.0f;
            const auto vowelColour = SectionLane::getColour (SectionType::chorus);

            for (auto& token : line.tokens)
            {
                const float x = xOf (token.beat);
                const auto note = juce::Rectangle<float> (x + 1.0f, row.getY() + 6.0f + (float) (high - token.pitch) * step,
                                                          juce::jmax (3.0f, (float) token.length * beatWidth - 2.0f), step + 2.0f);
                g.setColour (theme::accent);
                g.fillRoundedRectangle (note, 2.0f);

                g.setColour (theme::text);
                g.setFont (juce::FontOptions (14.5f, juce::Font::bold));
                g.drawSingleLineText (token.text, juce::roundToInt (x + 3.0f), juce::roundToInt (textY + 12.0f));

                for (auto& s : token.sounds)
                {
                    const auto block = juce::Rectangle<float> (xOf (s.start) + 0.5f, textY + 20.0f, juce::jmax (4.0f, (float) s.length * beatWidth - 1.0f), 26.0f);
                    g.setColour (s.vowel ? vowelColour : theme::textDim.withAlpha (0.4f));
                    g.fillRoundedRectangle (block, 3.0f);
                    g.setColour (theme::text);
                    g.setFont (juce::FontOptions (13.0f, s.vowel ? juce::Font::bold : juce::Font::plain));
                    g.drawFittedText (s.symbol, block.toNearestInt(), juce::Justification::centred, 1, 0.7f);
                }
            }

            y += (float) soundsRow;
        }
    }

    guide::Example example;
};

//==============================================================================
LyricsExplainer::LyricsExplainer (Engine& e) : engine (e)
{
    title.setText ("How lyrics are written", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    picture->onPlayNote = [this] (int pitch, int ms) { engine.previewNote (pitch, 0.8f, ms); };
    picture->setTooltip ("The example. In How a computer sings, click it to hear the notes.");
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

    singingButton.setTooltip ("Open the guide to singing techniques: posture and breathing, warming up, singing in tune and looking after your voice");
    singingButton.onClick = [this] { if (onOpenSingingGuide != nullptr) onOpenSingingGuide(); };
    addChildComponent (singingButton);

    engine.addChangeListener (this);

    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
}

LyricsExplainer::~LyricsExplainer()
{
    engine.removeChangeListener (this);
}

juce::String LyricsExplainer::getPrintSource() const
{
    const int n = (int) engine.getLyrics().lines.size();
    return n > 0 ? "the song's lyrics, " + juce::String (n) + (n == 1 ? " line" : " lines") : juce::String ("a song with no lyrics yet");
}

int LyricsExplainer::getNumPrintTopics() const
{
    return (int) std::size (guide::allTopics);
}

booklet::Topic LyricsExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    printHeight = picture->getPreferredHeight();   // all of it, not what fits the window
    return { guide::getTopicName (guide::allTopics[index]), guide::getTopicSummary (guide::allTopics[index]), explanation.getText(),
             { picture->getWidth(), printHeight }, {} };
}

juce::Image LyricsExplainer::paintPrintPicture (float scale)
{
    picture->setSize (picture->getWidth(), printHeight);
    auto image = guideprint::snapshot (*picture, scale);
    resized();
    return image;
}

void LyricsExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void LyricsExplainer::showTopic (guide::Topic topic)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (guide::allTopics[i] == topic)
            list.selectRow (i);
}

int LyricsExplainer::getNumRows()
{
    return (int) std::size (guide::allTopics);
}

void LyricsExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
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

void LyricsExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        show (row);
}

void LyricsExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // "Your lyrics" follows the song.
    if (guide::allTopics[shownRow] == guide::Topic::yours)
        show (shownRow);
}

void LyricsExplainer::show (int index)
{
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    const auto topic = guide::allTopics[shownRow];
    picture->setExample (guide::getExample (topic, engine.getLyrics()));
    explanation.setText (guide::explain (topic, engine.getLyrics(), engine.getSections(), engine.getTempo()), false);
    explanation.moveCaretToTop (false);
    singingButton.setVisible (topic == guide::Topic::singing && onOpenSingingGuide != nullptr);
    resized();
}

void LyricsExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void LyricsExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    picture->setBounds (r.removeFromTop (juce::jmin (picture->getPreferredHeight(), r.getHeight() / 2 + 60)));
    r.removeFromTop (10);

    if (singingButton.isVisible())
    {
        singingButton.setBounds (r.removeFromTop (28).removeFromLeft (190));
        r.removeFromTop (8);
    }

    explanation.setBounds (r);
}
} // namespace daw
