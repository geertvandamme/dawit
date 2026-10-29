#include "GuideButton.h"
#include "LyricsSingAlong.h"
#include "engine/Tempo.h"
#include "LyricsLane.h"
#include "SectionLane.h"

namespace daw
{
namespace singalong = gen::singalong;
namespace playalong = gen::playalong;

namespace
{
    constexpr auto settingTextSize = "singAlong.textSize", settingShowPrevious = "singAlong.showPrevious";

    constexpr float rowSpacing = 1.22f;   // a row's height for each point of the font's size
}

//==============================================================================
LyricsSingAlong::LyricsSingAlong (Engine& e) : engine (e)
{
    auto* settings = engine.properties.getUserSettings();
    textSize = juce::jlimit (0, 2, settings->getIntValue (settingTextSize, 1));
    showPrevious = settings->getBoolValue (settingShowPrevious, true);

    playButton.setTooltip ("Play or stop the song (Space), from the playhead");
    playButton.onClick = [this] { engine.togglePlay(); };

    sizeBox.addItemList ({ "Large text", "Larger text", "Largest text" }, 1);
    sizeBox.setSelectedItemIndex (textSize, juce::dontSendNotification);
    sizeBox.setTooltip ("How big the words are: larger to read them from across the room. They also grow with the window "
                        "(drag its corner, or make it full screen)");
    sizeBox.onChange = [this]
    {
        textSize = juce::jlimit (0, 2, sizeBox.getSelectedItemIndex());
        engine.properties.getUserSettings()->setValue (settingTextSize, textSize);
        repaint();
    };

    previousToggle.setTooltip ("Show the line you've just sung, faded, above the one to sing now");
    previousToggle.setToggleState (showPrevious, juce::dontSendNotification);
    previousToggle.onClick = [this]
    {
        showPrevious = previousToggle.getToggleState();
        engine.properties.getUserSettings()->setValue (settingShowPrevious, showPrevious);
        resized();
        repaint();
    };

    tipsButton.setTooltip ("Tips: reading the words while you record (reading ahead, breathing in the rests, coming in on time), and a guide to singing techniques");
    tipsButton.onClick = [this]
    {
        juce::PopupMenu menu;
        guideicon::addItem (menu, 1, "Reading the words while you record", onOpenGuide != nullptr);
        guideicon::addItem (menu, 2, "Singing techniques", onOpenSingingGuide != nullptr);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&tipsButton), [safe = juce::Component::SafePointer<LyricsSingAlong> (this)] (int result)
        {
            if (safe == nullptr)
                return;

            if (result == 1 && safe->onOpenGuide != nullptr)
                safe->onOpenGuide();
            else if (result == 2 && safe->onOpenSingingGuide != nullptr)
                safe->onOpenSingingGuide();
        });
    };

    // Clicking them mustn't take the keys away: Space plays and stops the song.
    for (juce::Component* c : std::initializer_list<juce::Component*> { &playButton, &sizeBox, &previousToggle, &tipsButton })
    {
        c->setWantsKeyboardFocus (false);
        c->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (c);
    }

    setSize (width, height);
    update (true);

    engine.addChangeListener (this);
    startTimerHz (30);
}

LyricsSingAlong::~LyricsSingAlong()
{
    engine.removeChangeListener (this);
}

void LyricsSingAlong::changeListenerCallback (juce::ChangeBroadcaster*)
{
    update (true);
}

void LyricsSingAlong::timerCallback()
{
    update (false);
}

void LyricsSingAlong::update (bool force)
{
    const playalong::Loop loop { engine.loopEnabled.load(), engine.loopStartBeat.load(), engine.loopEndBeat.load() };
    heardBeat = engine.getHeardPositionBeats();

    const auto& lyrics = engine.getLyrics();
    const auto m = singalong::findMoment (lyrics, heardBeat, loop);
    const int count = singalong::getCountIn (lyrics, m);
    const auto p = playalong::findMoment ({}, engine.getSections(), heardBeat, loop, 0);
    const bool playing = engine.isPlaying();

    const bool lineChanged = force || m.line != moment.line || m.singing != moment.singing || m.previous != moment.previous || m.next != moment.next;
    const bool beatChanged = p.bar != position.bar || p.beatInBar != position.beatInBar || p.section != position.section;
    const bool countChanged = count != countIn;
    const bool wasSinging = moment.singing;

    moment = m;
    position = p;
    countIn = count;

    if (force || playing != wasPlaying)
    {
        wasPlaying = playing;
        playButton.setButtonText (playing ? "Stop" : "Play");
    }

    if (lineChanged)
    {
        repaint();
        return;
    }

    // The colour running over the words; the rest only when it changes.
    if (moment.singing || wasSinging)
        repaint (currentArea);

    if (beatChanged)
        repaint (positionArea);

    if (beatChanged || countChanged)
        repaint (waitArea);
}

float LyricsSingAlong::getTextScale() const
{
    return textSize == 0 ? 0.75f : textSize == 1 ? 1.0f : 1.25f;
}

//==============================================================================
LyricsSingAlong::Layout LyricsSingAlong::layOut (const LyricLine& line, float maxSize, float areaWidth, float maxHeight)
{
    Layout layout;
    const size_t n = line.syllables.size();

    if (n == 0 || areaWidth <= 0.0f || maxSize <= 0.0f)
        return layout;

    // Widths at a size of 100, scaled to the size chosen.
    const juce::Font reference (juce::FontOptions (100.0f, juce::Font::bold));
    const float space = juce::GlyphArrangement::getStringWidth (reference, " ");
    std::vector<float> widths;

    for (size_t i = 0; i < n; ++i)
    {
        Piece piece;
        piece.text = line.syllables[i].text + (line.syllables[i].endsWord && i + 1 < n ? " " : "");
        piece.syllable = i;
        layout.pieces.push_back (piece);
        widths.push_back (juce::GlyphArrangement::getStringWidth (reference, piece.text));
    }

    // The width of the syllables from `from` up to `to`, without the space after the last.
    const auto rowWidth = [&] (size_t from, size_t to)
    {
        float w = 0.0f;
        for (size_t i = from; i < to; ++i)
            w += widths[i];

        return to > from && layout.pieces[to - 1].text.endsWithChar (' ') ? w - space : w;
    };

    const float total = rowWidth (0, n);
    const float oneRow = juce::jmin (maxSize, areaWidth / juce::jmax (1.0f, total) * 100.0f, maxHeight / rowSpacing);
    size_t split = n;

    // Two rows, split between words where the rows come out most even, when one row would be small.
    if (oneRow < maxSize * 0.7f)
    {
        float widest = total;
        size_t best = n;

        for (size_t k = 1; k < n; ++k)
        {
            if (! line.syllables[k - 1].endsWord)
                continue;

            const float w = juce::jmax (rowWidth (0, k), rowWidth (k, n));

            if (w < widest)
            {
                widest = w;
                best = k;
            }
        }

        const float twoRows = juce::jmin (maxSize, areaWidth / juce::jmax (1.0f, widest) * 100.0f, maxHeight / (2.0f * rowSpacing));

        if (best < n && twoRows > oneRow * 1.15f)
        {
            split = best;
            layout.rows = 2;
            layout.fontSize = twoRows;
        }
    }

    if (layout.rows == 1)
        layout.fontSize = oneRow;

    const float scale = layout.fontSize / 100.0f;
    layout.rowWidths = { rowWidth (0, split) * scale, rowWidth (split, n) * scale };

    for (int row = 0; row < layout.rows; ++row)
    {
        float x = (areaWidth - layout.rowWidths[(size_t) row]) * 0.5f;

        for (size_t i = row == 0 ? 0 : split; i < (row == 0 ? split : n); ++i)
        {
            auto& piece = layout.pieces[i];
            piece.row = row;
            piece.x = x;
            piece.width = widths[i] * scale;
            x += piece.width;
        }
    }

    return layout;
}

//==============================================================================
void LyricsSingAlong::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto bar = r.removeFromTop (28);
    playButton.setBounds (bar.removeFromLeft (72));
    bar.removeFromLeft (16);
    sizeBox.setBounds (bar.removeFromLeft (140));
    bar.removeFromLeft (12);
    previousToggle.setBounds (bar.removeFromLeft (120));
    tipsButton.setBounds (bar.removeFromRight (80));

    r.removeFromTop (10);
    positionArea = r.removeFromTop (30);
    r.removeFromTop (8);

    // The line before, the count-in, the line to sing, and the next line, top to bottom.
    const int h = r.getHeight();
    previousArea = showPrevious ? r.removeFromTop (h * 16 / 100) : juce::Rectangle<int>();
    waitArea = r.removeFromTop (juce::jlimit (36, 70, h / 10));
    currentArea = r.removeFromTop (r.getHeight() * 60 / 100);
    r.removeFromTop (8);
    nextArea = r;
}

void LyricsSingAlong::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    const auto& lyrics = engine.getLyrics();

    if (lyrics.isEmpty())
    {
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        auto r = getLocalBounds().reduced (60).withTrimmedTop (40);
        g.drawFittedText ("The song has no lyrics yet", r.removeFromTop (40), juce::Justification::centred, 1);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("Type them with Edit... in the Lyrics header, or find the words of a recording (right-click an audio clip, "
                          "Detect lyrics...). Time them by dragging or tapping, and they show here, large, as the song plays.",
                          r.removeFromTop (80), juce::Justification::centredTop, 4);
        return;
    }

    // Where the song is.
    const auto& sections = engine.getSections();
    g.setColour (position.section >= 0 ? SectionLane::getColour (sections[(size_t) position.section].type).brighter (0.4f) : theme::text);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawFittedText (playalong::describePosition (position, sections), positionArea.withTrimmedRight (260), juce::Justification::centredLeft, 1);

    juce::String info = formatBpm (engine.getTempo()) + " BPM";
    if (engine.loopEnabled.load())
        info << juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 looping bars ")) << juce::String ((int) std::floor (engine.loopStartBeat.load() / 4.0) + 1)
             << "-" << juce::String ((int) std::ceil (engine.loopEndBeat.load() / 4.0 - 1.0e-6));

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (info, positionArea, juce::Justification::centredRight);

    const float scale = getTextScale();
    const auto lineAt = [&lyrics] (int index) -> const LyricLine* { return index >= 0 && index < (int) lyrics.lines.size() ? &lyrics.lines[(size_t) index] : nullptr; };

    if (showPrevious)
        if (const auto* previous = lineAt (moment.previous); previous != nullptr && moment.previous != moment.line)
            paintLine (g, *previous, previousArea, (float) previousArea.getHeight() * 0.5f * scale, false, theme::textDim.withAlpha (0.55f));

    paintWait (g);

    if (const auto* current = lineAt (moment.line))
    {
        paintLine (g, *current, currentArea, (float) currentArea.getHeight() * 0.42f * scale, true,
                   moment.singing ? theme::text : theme::text.withAlpha (0.8f));
    }
    else
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmin (48.0f, (float) currentArea.getHeight() * 0.3f), juce::Font::bold));
        g.drawFittedText ("The lyrics have ended", currentArea, juce::Justification::centred, 1);
    }

    if (const auto* next = lineAt (moment.next); next != nullptr && moment.next != moment.line)
        paintLine (g, *next, nextArea, (float) nextArea.getHeight() * 0.36f * scale, false, theme::textDim);
}

void LyricsSingAlong::paintLine (juce::Graphics& g, const LyricLine& line, juce::Rectangle<int> area, float maxSize, bool current, juce::Colour colour)
{
    if (area.isEmpty())
        return;

    const auto box = area.toFloat().reduced (12.0f, 0.0f);
    const auto layout = layOut (line, maxSize, box.getWidth(), box.getHeight());

    if (layout.pieces.empty())
        return;

    const float rowHeight = layout.fontSize * rowSpacing;
    const float top = box.getCentreY() - rowHeight * (float) layout.rows * 0.5f;
    g.setFont (juce::FontOptions (layout.fontSize, juce::Font::bold));

    for (const auto& piece : layout.pieces)
    {
        const juce::Rectangle<float> r (box.getX() + piece.x, top + rowHeight * (float) piece.row, piece.width + 2.0f, rowHeight);
        g.setColour (colour);
        g.drawText (piece.text, r, juce::Justification::centredLeft, false);

        if (! current)
            continue;

        // The part already sung, in the lyrics' colour, over the letters still to sing.
        const float sung = singalong::getSung (line, piece.syllable, heardBeat);

        if (sung <= 0.0f)
            continue;

        const float letters = piece.text.endsWithChar (' ') ? piece.width - juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), " ") : piece.width;
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (r.withWidth (sung >= 1.0f ? r.getWidth() : letters * sung).getSmallestIntegerContainer());
        g.setColour (LyricsLane::colour);
        g.drawText (piece.text, r, juce::Justification::centredLeft, false);
    }
}

void LyricsSingAlong::paintWait (juce::Graphics& g)
{
    // A count-in after a rest: 4, 3, 2, 1, the beat now lit.
    if (countIn > 0)
    {
        const float dot = juce::jmin (34.0f, (float) waitArea.getHeight() * 0.8f);
        const float gap = dot * 0.5f;
        auto row = waitArea.toFloat().withSizeKeepingCentre (dot * 4.0f + gap * 3.0f, dot);

        for (int n = 4; n >= 1; --n)
        {
            const auto circle = row.removeFromLeft (dot);
            row.removeFromLeft (gap);
            const bool now = n == countIn, past = n > countIn;
            g.setColour (now ? theme::accent : past ? theme::panelLight.withAlpha (0.5f) : theme::panelLight);
            g.fillEllipse (circle);
            g.setColour (now ? juce::Colour (0xff15161a) : theme::textDim.withAlpha (past ? 0.4f : 1.0f));
            g.setFont (juce::FontOptions (dot * 0.5f, juce::Font::bold));
            g.drawText (juce::String (n), circle, juce::Justification::centred);
        }

        return;
    }

    // A long rest: when the next line comes.
    if (! moment.singing && moment.line >= 0 && moment.beatsToLine > 4.0)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmin (24.0f, (float) waitArea.getHeight() * 0.6f)));
        g.drawText ("Next line " + playalong::describeWait (moment.beatsToLine), waitArea, juce::Justification::centred);
    }
}
} // namespace daw
