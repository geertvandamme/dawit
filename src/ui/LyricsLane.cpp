#include "GuideButton.h"
#include "LyricsLane.h"

#include <array>

namespace daw
{
namespace
{
    juce::String getDisplayText (const LyricSyllable& s)
    {
        return s.endsWord ? s.text : s.text + "-";
    }

}

juce::String LyricsLane::getLineText (const LyricLine& line)
{
    juce::String text;

    for (auto& s : line.syllables)
        text << s.text << (s.endsWord ? " " : "");

    return text.trimEnd();
}

LyricsLane::LyricsLane (Engine& e, TimelineView& t) : engine (e), timeline (t)
{
    setWantsKeyboardFocus (true);
    setTooltip ("Lyrics: double-click to type the whole text, then drag the words into place or tap along while the song plays");
    engine.addChangeListener (this);
    expanded = engine.properties.getUserSettings()->getBoolValue ("view.lyricsOpen", false);
}

LyricsLane::~LyricsLane()
{
    engine.removeChangeListener (this);
}

void LyricsLane::setExpanded (bool shouldBeExpanded)
{
    if (expanded == shouldBeExpanded)
        return;

    expanded = shouldBeExpanded;
    engine.properties.getUserSettings()->setValue ("view.lyricsOpen", expanded);
    repaint();

    if (onLayoutChanged != nullptr)
        onLayoutChanged();
}

int LyricsLane::getPreferredHeight() const noexcept
{
    return expanded ? height + juce::roundToInt ((float) textRows * textRowHeight) + 6 : height;
}

std::vector<LyricsLane::TextPlacement> LyricsLane::getTextPlacements (const Lyrics& l) const
{
    // Each line on the first row that is free where it starts; when none is, on the one free soonest,
    // cutting off the words before it there.
    const juce::Font font (juce::FontOptions { textFontHeight });
    std::vector<TextPlacement> placements;
    std::array<float, textRows> rowEnd;
    std::array<int, textRows> lastOnRow;
    rowEnd.fill (std::numeric_limits<float>::lowest());
    lastOnRow.fill (-1);

    for (size_t i = 0; i < l.lines.size(); ++i)
    {
        const float x = timeline.beatToX (l.lines[i].startBeat);
        const float textEnd = x + 7.0f + juce::GlyphArrangement::getStringWidth (font, getLineText (l.lines[i]));
        int row = 0;

        while (row < textRows && rowEnd[(size_t) row] > x)
            ++row;

        if (row == textRows)
        {
            row = (int) (std::min_element (rowEnd.begin(), rowEnd.end()) - rowEnd.begin());
            auto& before = placements[(size_t) lastOnRow[(size_t) row]];
            before.clipEnd = juce::jmin (before.clipEnd, x - 6.0f);
        }

        placements.push_back ({ i, row, x, textEnd, textEnd + 2.0f });
        rowEnd[(size_t) row] = textEnd + 14.0f;
        lastOnRow[(size_t) row] = (int) placements.size() - 1;
    }

    return placements;
}

void LyricsLane::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Another project, or the text edited: the syllable to tap may be gone.
    if (tapTarget.has_value() && ! lyrics::isValid (engine.getLyrics(), *tapTarget))
        stopTapping();

    repaint();
}

void LyricsLane::editText()
{
    if (onEditText != nullptr)
        onEditText();
}

//==============================================================================
void LyricsLane::startTapping (std::optional<double> fromBeat)
{
    const auto& l = engine.getLyrics();

    if (l.isEmpty())
    {
        editText();
        return;
    }

    tapTarget = lyrics::findFrom (l, fromBeat.value_or (engine.isPlaying() ? getHeardBeat() : engine.getPositionBeats()));

    if (! tapTarget.has_value())
        tapTarget = lyrics::Position {};

    // A bar to get ready before the first syllable.
    playingForTapping = ! engine.isPlaying();

    if (playingForTapping)
    {
        engine.setPositionBeats (juce::jmax (0.0, std::floor (l.lines[(size_t) tapTarget->line].startBeat / 4.0) * 4.0 - 4.0));
        engine.play();
    }

    // Open, for the tap guide.
    openedForTapping = ! expanded;
    setExpanded (true);

    grabKeyboardFocus();
    repaint();

    if (onTappingChanged)
        onTappingChanged();
}

void LyricsLane::stopTapping (bool stopPlayback)
{
    if (! tapTarget.has_value())
        return;

    tapTarget.reset();

    if (stopPlayback && playingForTapping)
        engine.stop();

    if (openedForTapping)
        setExpanded (false);

    openedForTapping = playingForTapping = false;
    repaint();

    if (onTappingChanged)
        onTappingChanged();
}

double LyricsLane::getHeardBeat() const
{
    return engine.getHeardPositionBeats();
}

bool LyricsLane::keyPressed (const juce::KeyPress& key)
{
    if (! tapTarget.has_value())
        return false;

    if (key == juce::KeyPress::escapeKey)
    {
        stopTapping (true);
        return true;
    }

    // Space still plays and stops; any letter, digit or punctuation key taps.
    const auto c = key.getTextCharacter();

    if (c <= ' ' || key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
        return false;

    auto l = engine.getLyrics();
    const auto target = *tapTarget;
    lyrics::tap (l, target, snap (getHeardBeat(), key.getModifiers()));
    tapTarget = lyrics::getNext (l, target);
    engine.setLyrics (std::move (l));

    if (! tapTarget.has_value())
        stopTapping();

    return true;
}

void LyricsLane::focusLost (FocusChangeType)
{
    if (! tapTarget.has_value())
        return;

    // A click on a button such as Play passes the keys up to the window: take them back and keep tapping.
    // Anything that really wants them, such as a text box, ends tapping.
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent(); focused == nullptr || focused->isParentOf (this))
    {
        juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<LyricsLane> (this)]
        {
            if (safeThis != nullptr && safeThis->isTapping())
                safeThis->grabKeyboardFocus();
        });
        return;
    }

    stopTapping();
}

//==============================================================================
double LyricsLane::snap (double beat, const juce::ModifierKeys& mods) const
{
    if (mods.isAltDown())
        return beat;

    const double step = timeline.pixelsPerBeat >= 60.0 ? 0.25 : timeline.pixelsPerBeat >= 24.0 ? 0.5 : 1.0;
    return std::round (beat / step) * step;
}

bool LyricsLane::isOverview (const LyricLine& line) const
{
    // Too narrow for a block per syllable: the line as one block.
    const double pixels = (line.endBeat - line.startBeat) * timeline.pixelsPerBeat;
    return pixels / (double) line.syllables.size() < 14.0;
}

LyricsLane::Hit LyricsLane::findHit (juce::Point<float> p) const
{
    const auto& l = getShown();

    if (p.y >= (float) height)
    {
        // The written-out lines, when open: they drag their line.
        if (expanded)
            for (auto& placement : getTextPlacements (l))
                if (p.y >= getTextRowY (placement.row) && p.y < getTextRowY (placement.row) + textRowHeight
                    && p.x >= placement.x - 2.0f && p.x < juce::jmin (placement.textEnd, placement.clipEnd))
                    return { Target::line, { (int) placement.line, 0 } };

        return {};
    }

    for (size_t i = 0; i < l.lines.size(); ++i)
    {
        const auto& line = l.lines[i];
        const float x1 = timeline.beatToX (line.startBeat), x2 = timeline.beatToX (line.endBeat);

        if (p.x < x1 - 3.0f || p.x > x2 + 4.0f)
            continue;

        if (p.y < syllableTop || isOverview (line))
        {
            if (x2 - x1 > 14.0f && p.x >= x2 - 5.0f)
                return { Target::lineEnd, { (int) i, 0 } };

            return { Target::line, { (int) i, 0 } };
        }

        for (size_t j = line.syllables.size(); j-- > 0;)
            if (p.x >= timeline.beatToX (line.syllables[j].beat) - (j == 0 ? 3.0f : 0.0f))
                return { Target::syllable, { (int) i, (int) j } };
    }

    return {};
}

void LyricsLane::updatePlayhead()
{
    const float x = timeline.beatToX (engine.getPositionBeats());
    const auto nowSung = engine.isPlaying() ? lyrics::findAt (engine.getLyrics(), getHeardBeat()) : std::nullopt;

    if (nowSung != sung)
    {
        sung = nowSung;
        repaint();
    }

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 5, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 5, getHeight());
}

//==============================================================================
void LyricsLane::paint (juce::Graphics& g)
{
    g.fillAll (theme::lane);

    if (expanded)
    {
        g.setColour (theme::laneAlt);
        g.fillRect (0, height, getWidth(), getHeight() - height);
    }

    // Bar lines, thinned out when zoomed far out (like the ruler).
    const float barWidth = (float) (timeline.pixelsPerBeat * 4.0);
    const int firstBar = juce::jmax (0, (int) std::floor (timeline.xToBeat (0.0) / 4.0));
    const int lastBar = (int) std::floor (timeline.xToBeat ((double) getWidth()) / 4.0);
    int barStep = 1;

    while (barStep * barWidth < 44.0f)
        barStep *= 2;

    g.setColour (theme::grid);
    for (int bar = firstBar - firstBar % barStep; bar <= lastBar; bar += barStep)
        g.fillRect (timeline.beatToX (bar * 4.0), 0.0f, 1.0f, (float) getHeight());

    const auto& l = getShown();

    if (l.isEmpty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.5f));
        g.drawText ("Double-click to type the song's lyrics, then drag the words into place or tap along",
                    getLocalBounds().withTrimmedLeft (10), juce::Justification::centredLeft);
    }

    for (size_t i = 0; i < l.lines.size(); ++i)
    {
        const auto& line = l.lines[i];
        const float x1 = timeline.beatToX (line.startBeat), x2 = timeline.beatToX (line.endBeat);

        if (x2 < 0.0f || x1 > (float) getWidth())
            continue;

        const bool lineHovered = hover.target != Target::none && hover.position.line == (int) i;
        const bool lineDragged = drag.target != Target::none && drag.position.line == (int) i;

        // The line's bar, with its end as a handle.
        const auto bar = juce::Rectangle<float> (x1, lineTop, juce::jmax (2.0f, x2 - x1), lineHeight);
        g.setColour (colour.withAlpha (lineHovered || lineDragged ? 0.55f : 0.32f));
        g.fillRoundedRectangle (bar, 2.5f);

        if (bar.getWidth() > 14.0f)
        {
            g.setColour (colour.withAlpha ((hover.target == Target::lineEnd || drag.target == Target::lineEnd) && (lineHovered || lineDragged) ? 1.0f : 0.6f));
            g.fillRoundedRectangle (bar.withLeft (bar.getRight() - 3.0f), 1.5f);
        }

        if (line.startsVerse && line.heading.isNotEmpty() && bar.getWidth() > 30.0f)
        {
            g.setColour (theme::text.withAlpha (0.85f));
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText (line.heading.toUpperCase(), bar.withTrimmedLeft (4.0f).withTrimmedRight (5.0f), juce::Justification::centredLeft, true);
        }

        if (isOverview (line))
        {
            const auto block = juce::Rectangle<float> (x1, syllableTop, juce::jmax (2.0f, x2 - x1), syllableHeight);
            g.setColour (colour.withAlpha (sung.has_value() && sung->line == (int) i ? 0.75f : 0.25f));
            g.fillRoundedRectangle (block, 3.0f);

            // The words run on into the gap before the next line.
            const float textEnd = i + 1 < l.lines.size() ? timeline.beatToX (l.lines[i + 1].startBeat) - 4.0f : (float) getWidth();

            if (textEnd - x1 > 16.0f)
            {
                juce::Graphics::ScopedSaveState state (g);
                g.reduceClipRegion (juce::Rectangle<float> (x1, syllableTop, textEnd - x1, syllableHeight).toNearestInt());
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.0f));
                g.drawSingleLineText (getLineText (line), juce::roundToInt (x1 + 4.0f), juce::roundToInt (syllableTop + syllableHeight * 0.5f + 4.0f));
            }

            continue;
        }

        for (size_t j = 0; j < line.syllables.size(); ++j)
        {
            const auto& s = line.syllables[j];
            const float sx1 = timeline.beatToX (s.beat), sx2 = timeline.beatToX (line.getSyllableEnd (j));

            if (sx2 < 0.0f || sx1 > (float) getWidth())
                continue;

            const lyrics::Position here { (int) i, (int) j };
            const auto block = juce::Rectangle<float> (sx1, syllableTop, juce::jmax (1.0f, sx2 - sx1 - 1.0f), syllableHeight);
            const bool isSung = sung.has_value() && *sung == here;
            const bool isHovered = (hover.target == Target::syllable && hover.position == here) || (drag.target == Target::syllable && drag.position == here);

            g.setColour (colour.withAlpha (isSung ? 0.85f : isHovered ? 0.5f : s.pinned || j == 0 ? 0.36f : 0.2f));
            g.fillRoundedRectangle (block, 2.5f);

            // Placed by the user (or the line's start): a solid edge where it starts.
            if (s.pinned || j == 0)
            {
                g.setColour (colour);
                g.fillRect (block.withWidth (juce::jmin (2.0f, block.getWidth())));
            }

            if (tapTarget.has_value() && *tapTarget == here)
            {
                g.setColour (theme::accent);
                g.drawRoundedRectangle (block.expanded (0.5f), 2.5f, 1.5f);
            }

            if (block.getWidth() > 8.0f)
            {
                // Smaller when it doesn't fit, down to a size still easy to read; then cut off.
                juce::Font font (juce::FontOptions (12.5f, isSung ? juce::Font::bold : juce::Font::plain));
                const auto shown = getDisplayText (s);
                const float room = block.getWidth() - 6.0f, width = juce::GlyphArrangement::getStringWidth (font, shown);

                if (width > room)
                    font = font.withHeight (juce::jmax (9.5f, 12.5f * room / width));

                juce::Graphics::ScopedSaveState state (g);
                g.reduceClipRegion (block.toNearestInt());
                g.setColour (isSung ? juce::Colours::black : theme::text);
                g.setFont (font);
                g.drawSingleLineText (shown, juce::roundToInt (block.getX() + 3.5f), juce::roundToInt (syllableTop + syllableHeight * 0.5f + 4.5f));
            }
        }
    }

    if (expanded && tapTarget.has_value() && lyrics::isValid (l, *tapTarget))
    {
        paintTapGuide (g, l);
    }
    else if (expanded)
    {
        // Every line written out in full, starting where it's sung; the syllable being sung lights up.
        const juce::Font font (juce::FontOptions { textFontHeight });

        for (auto& placement : getTextPlacements (l))
        {
            if (placement.clipEnd < 0.0f || placement.x > (float) getWidth())
                continue;

            const auto& line = l.lines[placement.line];
            const bool lineHovered = (hover.target != Target::none && hover.position.line == (int) placement.line)
                                     || (drag.target != Target::none && drag.position.line == (int) placement.line);
            const float y = getTextRowY (placement.row);

            g.setColour (colour.withAlpha (lineHovered ? 1.0f : 0.7f));
            g.fillRect (placement.x, y + 2.0f, 2.0f, textRowHeight - 4.0f);

            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (juce::Rectangle<float> (placement.x, y, juce::jmax (0.0f, placement.clipEnd - placement.x), textRowHeight).toNearestInt());
            g.setFont (font);
            float x = placement.x + 7.0f;

            for (size_t j = 0; j < line.syllables.size(); ++j)
            {
                const auto& syllable = line.syllables[j];
                const auto piece = syllable.text + (syllable.endsWord && j + 1 < line.syllables.size() ? " " : "");
                const bool isSung = sung.has_value() && *sung == lyrics::Position { (int) placement.line, (int) j };

                g.setColour (isSung ? colour.brighter (0.3f) : lineHovered ? theme::text : theme::text.withAlpha (0.88f));
                g.drawSingleLineText (piece, juce::roundToInt (x), juce::roundToInt (y + textRowHeight * 0.5f + 4.5f));
                x += juce::GlyphArrangement::getStringWidth (font, piece);
            }
        }
    }

    g.setColour (theme::playhead); // where updatePlayhead() last put it, as in TrackLanes
    g.fillRect (playheadX, 0.0f, 1.0f, (float) getHeight());

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void LyricsLane::paintTapGuide (juce::Graphics& g, const Lyrics& l)
{
    const auto target = *tapTarget;
    const auto& line = l.lines[(size_t) target.line];
    float x = 12.0f;
    const float y = getTextRowY (0);

    // The line being tapped, each syllable on its own: done ones in the lyrics colour, the next one lit.
    const juce::Font big (juce::FontOptions (16.0f, juce::Font::bold));
    g.setFont (big);

    for (size_t j = 0; j < line.syllables.size(); ++j)
    {
        const auto& syllable = line.syllables[j];
        const auto text = getDisplayText (syllable);
        const float width = juce::GlyphArrangement::getStringWidth (big, text);
        const auto box = juce::Rectangle<float> (x - 3.0f, y, width + 6.0f, textRowHeight);

        if ((int) j == target.syllable)
        {
            g.setColour (theme::accent);
            g.fillRoundedRectangle (box, 3.0f);
            g.setColour (juce::Colours::black);
        }
        else
        {
            g.setColour ((int) j < target.syllable ? colour : theme::text.withAlpha (0.6f));
        }

        g.drawSingleLineText (text, juce::roundToInt (x), juce::roundToInt (y + textRowHeight * 0.5f + 5.5f));
        x += width + (syllable.endsWord ? 12.0f : 6.0f);
    }

    // The line after it, so the next tap after this line's end isn't a surprise.
    if ((size_t) target.line + 1 < l.lines.size())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.5f));
        g.drawText ("then:  " + getLineText (l.lines[(size_t) target.line + 1]),
                    juce::Rectangle<float> (12.0f, getTextRowY (1), (float) getWidth() - 24.0f, textRowHeight), juce::Justification::centredLeft, true);
    }

    g.setColour (theme::accent);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (juce::String::fromUTF8 ("Press any letter key the moment the lit syllable is sung  \xc2\xb7  Space plays or pauses  \xc2\xb7  "
                                        "Esc or Stop ends tapping  \xc2\xb7  drag a syllable afterwards to fix it"),
                juce::Rectangle<float> (12.0f, getTextRowY (2), (float) getWidth() - 24.0f, textRowHeight), juce::Justification::centredLeft, true);
}

//==============================================================================
void LyricsLane::updateHover (const Hit& hit)
{
    if (hit.target == hover.target && hit.position == hover.position)
        return;

    hover = hit;

    // The line's words first, which a narrow line can't show.
    const auto& l = getShown();
    const auto words = lyrics::isValid (l, hit.position) ? juce::String::fromUTF8 ("\xe2\x80\x9c") + getLineText (l.lines[(size_t) hit.position.line])
                                                               + juce::String::fromUTF8 ("\xe2\x80\x9d\n")
                                                         : juce::String();

    switch (hit.target)
    {
        case Target::syllable:
            setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
            setTooltip (words + "Drag to place this syllable (Alt: without snapping); the unplaced ones around it follow. "
                        "Click to move the playhead here, double-click to edit the text, right-click for more");
            break;
        case Target::line:
            setMouseCursor (juce::MouseCursor::DraggingHandCursor);
            setTooltip (words + "Drag to move the whole line, keeping its timing");
            break;
        case Target::lineEnd:
            setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
            setTooltip (words + "Drag to move where the line stops: the unplaced syllables before it stretch or squeeze");
            break;
        case Target::none:
            setMouseCursor (juce::MouseCursor::NormalCursor);
            setTooltip ("Lyrics: double-click to type the whole text, then drag the words into place or tap along while the song plays");
            break;
    }

    repaint();
}

void LyricsLane::mouseMove (const juce::MouseEvent& e)
{
    updateHover (findHit (e.position));
}

void LyricsLane::mouseExit (const juce::MouseEvent&)
{
    updateHover ({});
}

void LyricsLane::mouseDown (const juce::MouseEvent& e)
{
    const auto hit = findHit (e.position);
    const double beat = timeline.xToBeat (e.position.x);

    if (e.mods.isPopupMenu())
    {
        showMenu (hit, beat);
        return;
    }

    drag = hit;
    dragged = false;
    dragOriginal = engine.getLyrics();

    if (! lyrics::isValid (dragOriginal, hit.position))
        return;

    const auto& line = dragOriginal.lines[(size_t) hit.position.line];

    switch (hit.target)
    {
        case Target::syllable: dragOffset = line.syllables[(size_t) hit.position.syllable].beat - beat; break;
        case Target::line:     dragOffset = line.startBeat - beat; break;
        case Target::lineEnd:  dragOffset = line.endBeat - beat; break;
        case Target::none:     break;
    }
}

void LyricsLane::mouseDrag (const juce::MouseEvent& e)
{
    if (drag.target == Target::none || e.mods.isPopupMenu() || (! dragged && e.getDistanceFromDragStart() < 3))
        return;

    dragged = true;
    const double beat = snap (timeline.xToBeat (e.position.x) + dragOffset, e.mods);
    auto moved = dragOriginal;

    switch (drag.target)
    {
        case Target::syllable: lyrics::moveSyllable (moved, drag.position, beat); break;
        case Target::line:     lyrics::moveLine (moved, drag.position.line, beat); break;
        case Target::lineEnd:  lyrics::setLineEnd (moved, drag.position.line, beat); break;
        case Target::none:     break;
    }

    preview = std::move (moved);
    repaint();
}

void LyricsLane::mouseUp (const juce::MouseEvent& e)
{
    if (preview.has_value())
    {
        auto done = std::move (*preview);
        preview.reset();
        engine.setLyrics (std::move (done));
    }
    else if (! dragged && ! e.mods.isPopupMenu() && drag.target == Target::syllable && e.getNumberOfClicks() == 1)
    {
        const auto& l = engine.getLyrics();

        if (lyrics::isValid (l, drag.position))
            engine.setPositionBeats (l.lines[(size_t) drag.position.line].syllables[(size_t) drag.position.syllable].beat);
    }

    drag = {};
    dragged = false;
    updateHover (findHit (e.position));
    repaint();
}

void LyricsLane::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        editText();
}

void LyricsLane::showMenu (const Hit& hit, double beat)
{
    const auto& l = engine.getLyrics();
    juce::PopupMenu menu;
    enum { edit = 1, tapHere, tapStart, unpinOne, unpinLine, respread, clear, guide, singAlong, singLyrics };

    menu.addItem (edit, l.isEmpty() ? "Type the lyrics..." : "Edit the lyrics...");

    if (! l.isEmpty())
    {
        menu.addItem (tapHere, "Tap along from here");
        menu.addItem (tapStart, "Tap along from the start");
        menu.addSeparator();

        if (lyrics::isValid (l, hit.position))
        {
            const auto& line = l.lines[(size_t) hit.position.line];
            const auto& s = line.syllables[(size_t) hit.position.syllable];

            if (hit.target == Target::syllable && s.pinned)
                menu.addItem (unpinOne, "Let \"" + s.text + "\" follow the words around it");

            menu.addItem (unpinLine, "Spread this line's words evenly", std::any_of (line.syllables.begin(), line.syllables.end(),
                                                                                      [] (const LyricSyllable& x) { return x.pinned; }));
        }

        menu.addItem (respread, "Spread all the lines over the sections again");
        menu.addItem (clear, "Delete the lyrics");
    }

    menu.addSeparator();

    if (onSingAlong != nullptr)
        menu.addItem (singAlong, "Sing along with the lyrics...", ! l.isEmpty());

    if (onSingLyrics != nullptr)
        menu.addItem (singLyrics, "Sing the lyrics with a computer voice...", ! l.isEmpty());

    guideicon::addItem (menu, guide, "How lyrics are written...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safeThis = juce::Component::SafePointer<LyricsLane> (this), hit, beat] (int result)
                        {
                            auto* self = safeThis.getComponent();

                            if (self == nullptr || result == 0)
                                return;

                            auto& songEngine = self->engine;
                            auto changed = songEngine.getLyrics();

                            switch (result)
                            {
                                case edit:      self->editText(); return;
                                case guide:     if (self->onShowGuide) self->onShowGuide(); return;
                                case singAlong: if (self->onSingAlong) self->onSingAlong(); return;
                                case singLyrics: if (self->onSingLyrics) self->onSingLyrics(); return;
                                case tapHere:   self->startTapping (hit.target == Target::none ? beat : changed.lines[(size_t) hit.position.line]
                                                                                                          .syllables[(size_t) hit.position.syllable].beat);
                                                return;
                                case tapStart:  self->startTapping (0.0); return;
                                case unpinOne:  lyrics::unpin (changed, hit.position.line, hit.position.syllable); break;
                                case unpinLine: lyrics::unpin (changed, hit.position.line); break;
                                case respread:
                                case clear:
                                {
                                    const bool respreading = result == respread;
                                    auto options = juce::MessageBoxOptions::makeOptionsOkCancel (
                                        juce::MessageBoxIconType::QuestionIcon, respreading ? "Spread the lyrics again" : "Delete the lyrics",
                                        respreading ? "Spread every line over the sections again? Where words were dragged or tapped is forgotten."
                                                    : "Delete all the lyrics of this song?",
                                        respreading ? "Spread" : "Delete", "Cancel", self);
                                    juce::AlertWindow::showAsync (options, [&songEngine, respreading] (int answer)
                                    {
                                        if (answer == 1)
                                            songEngine.setLyrics (respreading ? lyrics::place (lyrics::parse (lyrics::toText (songEngine.getLyrics())), songEngine.getSections())
                                                                          : Lyrics {});
                                    });
                                    return;
                                }
                                default: return;
                            }

                            songEngine.setLyrics (std::move (changed));
                        });
}
} // namespace daw
