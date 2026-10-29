#include "ChordLane.h"
#include "generate/Harmony.h"

namespace daw
{
namespace harmony = gen::harmony;

juce::Colour ChordLane::getColour (const Chord& chord)
{
    // Chords on the same root share a colour, so repeats in a progression are easy to spot.
    return juce::Colour::fromHSV ((float) chord.root / 12.0f, 0.45f, 0.55f, 1.0f);
}

ChordLane::ChordLane (Engine& e, TimelineView& t) : engine (e), timeline (t)
{
    setTooltip ("Chord track: click a bar to set its chord. The generator follows these chords, "
                "and fills empty bars with the chords it writes. Shift-click or Shift-drag across bars to select them, "
                "then copy (Cmd+C), paste (Cmd+V) or duplicate them (Cmd+D). Drag chords to move them, "
                "with Cmd held to copy them, like clips.");
    setWantsKeyboardFocus (true);
    engine.addChangeListener (this);

    auto& settings = *engine.properties.getUserSettings();
    expanded = settings.getBoolValue ("view.chordTrackOpen", false);
    instrument = settings.getValue ("view.chordTrackInstrument") == "guitar" ? Instrument::guitar : Instrument::piano;
    guitarPainter.setCompact (true);
    keyboardPainter.setShowNames (false);
}

void ChordLane::setExpanded (bool shouldBeExpanded)
{
    if (expanded == shouldBeExpanded)
        return;

    expanded = shouldBeExpanded;
    engine.properties.getUserSettings()->setValue ("view.chordTrackOpen", expanded);
    repaint();

    if (onLayoutChanged != nullptr)
        onLayoutChanged();
}

void ChordLane::setInstrument (Instrument newInstrument)
{
    if (instrument == newInstrument)
        return;

    instrument = newInstrument;
    engine.properties.getUserSettings()->setValue ("view.chordTrackInstrument", instrument == Instrument::guitar ? "guitar" : "piano");
    repaint();

    if (onLayoutChanged != nullptr)
        onLayoutChanged();
}

int ChordLane::getPreferredHeight() const noexcept
{
    if (! expanded)
        return height;

    return height + (instrument == Instrument::guitar ? guitarDiagramHeight : pianoDiagramHeight) + 8;
}

const std::vector<ChordLane::Block>& ChordLane::getBlocks (const ChordMap& chords)
{
    if (! blocks.empty() && chords == blocksFrom)
        return blocks;

    blocksFrom = chords;
    blocks.clear();

    // As the chord blocks are drawn: a chord repeated on the following bars is one block.
    for (auto it = chords.begin(); it != chords.end();)
    {
        Block block;
        block.startBar = it->first;
        block.chord = it->second;
        block.endBar = block.startBar + 1;

        for (++it; it != chords.end() && it->first == block.endBar && it->second == block.chord; ++it)
            ++block.endBar;

        blocks.push_back (block);
    }

    // Voiced through the whole song, as the chord guide does with a progression.
    std::vector<Chord> progression;
    for (auto& b : blocks)
        progression.push_back (b.chord);

    const auto voicings = gen::diagrams::voiceProgression (progression);
    const auto shapes = gen::diagrams::shapeProgression (progression);
    pianoLow = 127;
    pianoHigh = 0;

    for (size_t i = 0; i < blocks.size(); ++i)
    {
        blocks[i].voicing = voicings[i];
        blocks[i].shape = shapes[i];

        if (i > 0)
        {
            blocks[i].previousVoicing = voicings[i - 1];
            blocks[i].previousShape = shapes[i - 1];
        }

        for (int n : voicings[i])
        {
            pianoLow = juce::jmin (pianoLow, n);
            pianoHigh = juce::jmax (pianoHigh, n);
        }
    }

    return blocks;
}

juce::Rectangle<float> ChordLane::getDiagramArea (const Block& block) const
{
    // Under the block, from its start (kept in view when the block starts left of it), as wide
    // as a diagram reads well and no wider than the block.
    const float x1 = juce::jmax (timeline.beatToX (block.startBar * 4.0), 0.0f), x2 = timeline.beatToX (block.endBar * 4.0);
    const bool guitar = instrument == Instrument::guitar;
    const float width = juce::jmin (x2 - x1 - 6.0f, guitar ? 84.0f : 150.0f);
    const float h = (float) (guitar ? guitarDiagramHeight : pianoDiagramHeight);

    return { x1 + 4.0f, (float) height + 3.0f, width, h };
}

const ChordLane::Block* ChordLane::blockAtDiagram (juce::Point<float> position)
{
    if (! expanded || position.y < (float) height)
        return nullptr;

    for (auto& block : getBlocks (engine.getChords()))
        if (const auto area = getDiagramArea (block); area.getWidth() >= 30.0f && area.contains (position))
            return &block;

    return nullptr;
}

void ChordLane::playBlock (const Block& block)
{
    // As the chord guide plays it: the piano with its bass note, the guitar shape strummed.
    const bool guitar = instrument == Instrument::guitar && ! block.shape.isEmpty();
    std::vector<int> notes;

    if (guitar)
    {
        static constexpr int openStrings[] { 40, 45, 50, 55, 59, 64 };
        for (size_t s = 0; s < block.shape.frets.size(); ++s)
            if (block.shape.frets[s] >= 0)
                notes.push_back (openStrings[s] + block.shape.frets[s]);
    }
    else
    {
        notes.push_back (gen::diagrams::pianoBass (block.chord));
        notes.insert (notes.end(), block.voicing.begin(), block.voicing.end());
    }

    engine.setPreviewSound (guitar ? BuiltInSound::pluck : BuiltInSound::piano);
    const int ms = juce::jlimit (500, 2000, juce::roundToInt (2.0 * 60000.0 / engine.getTempo()));

    for (size_t i = 0; i < notes.size(); ++i)
    {
        const int note = notes[i];

        if (guitar && i > 0)
            juce::Timer::callAfterDelay ((int) i * 28, [safeThis = juce::Component::SafePointer<ChordLane> (this), note, ms, i]
            {
                if (auto* self = safeThis.getComponent())
                    self->engine.previewNote (note, 0.75f, ms - (int) i * 28);
            });
        else
            engine.previewNote (note, note < 55 ? 0.65f : 0.75f, ms);
    }
}

ChordLane::~ChordLane()
{
    engine.removeChangeListener (this);
}

void ChordLane::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

int ChordLane::barAt (float x) const
{
    return (int) std::floor (timeline.xToBeat (x) / 4.0);
}

void ChordLane::updatePlayhead()
{
    const float x = timeline.beatToX (engine.getPositionBeats());

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 5, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 5, getHeight());
}

void ChordLane::paint (juce::Graphics& g)
{
    g.fillAll (theme::laneAlt);

    const float barWidth = (float) (timeline.pixelsPerBeat * 4.0);
    const int firstBar = juce::jmax (0, barAt (0.0f));
    const int lastBar = barAt ((float) getWidth());
    const auto& chords = dragPreview.has_value() ? *dragPreview : engine.getChords();

    // Bar lines, thinned out when zoomed far out (like the ruler).
    int barStep = 1;
    while (barStep * barWidth < 44.0f)
        barStep *= 2;

    g.setColour (theme::grid);
    for (int bar = firstBar - firstBar % barStep; bar <= lastBar; bar += barStep)
        g.fillRect (timeline.beatToX (bar * 4.0), 0.0f, 1.0f, (float) getHeight());

    if (hoverBar >= 0 && chords.count (hoverBar) == 0 && drag == Drag::none)
    {
        const auto r = juce::Rectangle<float> (timeline.beatToX (hoverBar * 4.0), 0.0f, barWidth, (float) height).reduced (1.0f, 3.0f);
        g.setColour (theme::text.withAlpha (0.06f));
        g.fillRoundedRectangle (r, 3.0f);

        if (r.getWidth() > 14.0f)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.0f));
            g.drawText ("+", r.withTrimmedLeft (5.0f), juce::Justification::centredLeft);
        }
    }

    // A chord repeated on the following bars is drawn as one block.
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));

    for (auto it = chords.lower_bound (firstBar - 64); it != chords.end() && it->first <= lastBar;)
    {
        const int startBar = it->first;
        const Chord& chord = it->second;
        int endBar = startBar + 1;

        for (++it; it != chords.end() && it->first == endBar && it->second == chord; ++it)
            ++endBar;

        if (endBar < firstBar)
            continue;

        const float x1 = timeline.beatToX (startBar * 4.0), x2 = timeline.beatToX (endBar * 4.0);
        const auto r = juce::Rectangle<float> (x1, 0.0f, x2 - x1, (float) height).reduced (1.0f, 3.0f);
        const bool hovered = hoverBar >= startBar && hoverBar < endBar && drag == Drag::none;

        g.setColour (getColour (chord).withAlpha (hovered ? 1.0f : 0.85f));
        g.fillRoundedRectangle (r, 3.0f);

        if (r.getWidth() > 12.0f)
        {
            // Keep the name visible when the block starts left of the view.
            const auto text = r.withLeft (juce::jmax (r.getX(), 0.0f)).reduced (5.0f, 0.0f);
            g.setColour (theme::text);
            g.drawFittedText (chord.getName(), text.toNearestInt(), juce::Justification::centredLeft, 1, 0.7f);
        }
    }

    // How to play each block, under it, when the row is open.
    if (expanded)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (0, height, getWidth(), 1);

        const bool guitar = instrument == Instrument::guitar;

        for (auto& block : getBlocks (chords))
        {
            const auto area = getDiagramArea (block);

            if (area.getWidth() < 30.0f || ! g.clipRegionIntersects (area.toNearestInt()))
                continue;

            juce::Component& painter = guitar ? static_cast<juce::Component&> (guitarPainter) : keyboardPainter;

            if (guitar)
                guitarPainter.setShape (block.shape, block.previousShape.isEmpty() ? nullptr : &block.previousShape);
            else
            {
                keyboardPainter.setRange (pianoLow, pianoHigh);
                keyboardPainter.setChord (block.voicing, -1, block.chord.root, block.previousVoicing);
            }

            painter.setSize (juce::roundToInt (area.getWidth()), juce::roundToInt (area.getHeight()));
            juce::Graphics::ScopedSaveState state (g);
            g.setOrigin (area.getPosition().toInt());
            painter.paint (g);
        }
    }

    // The selected bars, or where a dragged selection will land.
    if (selectionBars > 0)
    {
        const int first = drag == Drag::move ? dropBar : selectionStart;
        const auto r = juce::Rectangle<float> (timeline.beatToX (first * 4.0), 0.0f, barWidth * (float) selectionBars, (float) height);
        g.setColour (theme::text.withAlpha (0.12f));
        g.fillRect (r);
        g.setColour (theme::text.withAlpha (hasKeyboardFocus (false) || drag != Drag::none ? 0.9f : 0.45f));
        g.drawRect (r.reduced (0.5f, 1.0f), 1.5f);
    }

    g.setColour (theme::playhead); // where updatePlayhead() last put it, as in TrackLanes
    g.fillRect (playheadX, 0.0f, 1.0f, (float) getHeight());

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void ChordLane::mouseMove (const juce::MouseEvent& e)
{
    const int bar = juce::jmax (-1, barAt (e.position.x));

    if (bar != hoverBar)
    {
        hoverBar = bar;
        repaint();
    }
}

void ChordLane::mouseExit (const juce::MouseEvent&)
{
    hoverBar = -1;
    repaint();
}

void ChordLane::selectBars (int firstBar, int numBars)
{
    selectionStart = juce::jmax (0, firstBar);
    selectionBars = juce::jmax (0, numBars);
    repaint();
}

void ChordLane::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    const int bar = juce::jmax (0, barAt (e.position.x));
    drag = Drag::none;
    dragPreview.reset();
    pressedDiagram = false;

    // A click on a diagram plays it.
    if (! e.mods.isPopupMenu())
    {
        if (const auto* block = blockAtDiagram (e.position))
        {
            pressedDiagram = true;
            playBlock (*block);
            return;
        }
    }

    dragBar = bar;
    blockSelectedOnPress = false;

    if (e.mods.isPopupMenu())
    {
        if (! isSelected (bar))
            selectBars (bar, 1);
    }
    else if (e.mods.isShiftDown())
    {
        // Shift selects: a click extends the selection to the clicked bar (from its far end).
        dragAnchorBar = selectionBars == 0 ? bar : bar < selectionStart ? selectionStart + selectionBars - 1 : selectionStart;
        drag = Drag::select;
        selectBars (juce::jmin (dragAnchorBar, bar), std::abs (bar - dragAnchorBar) + 1);
    }
    else if (isSelected (bar))
    {
        drag = Drag::move;
        dropBar = selectionStart;
    }
    else if (engine.getChord (bar) != nullptr)
    {
        // Like a clip: dragging a chord block moves it (with Cmd, copies it). A plain click still
        // selects only its bar, in mouseUp.
        const Chord chord = *engine.getChord (bar);
        int first = bar, end = bar + 1;

        while (first > 0 && engine.getChord (first - 1) != nullptr && *engine.getChord (first - 1) == chord)
            --first;

        while (engine.getChord (end) != nullptr && *engine.getChord (end) == chord)
            ++end;

        selectBars (first, end - first);
        blockSelectedOnPress = true;
        drag = Drag::move;
        dropBar = selectionStart;
    }
}

void ChordLane::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::none || ! e.mouseWasDraggedSinceMouseDown())
        return;

    const int bar = juce::jmax (0, barAt (e.position.x));

    if (drag == Drag::select)
    {
        // Shift-click extends the selection; Shift-drag selects from where it started.
        dragAnchorBar = dragBar;
        selectBars (juce::jmin (dragAnchorBar, bar), std::abs (bar - dragAnchorBar) + 1);
        return;
    }

    dropBar = juce::jmax (0, selectionStart + bar - dragBar);
    updateDragPreview (e.mods.isCommandDown());
}

void ChordLane::modifierKeysChanged (const juce::ModifierKeys& mods)
{
    if (drag == Drag::move && dragPreview.has_value())
        updateDragPreview (mods.isCommandDown());
}

void ChordLane::updateDragPreview (bool copying)
{
    dragCopies = copying;
    auto preview = engine.getChords();

    if (copying)
        chordEdits::pasteBars (preview, chordEdits::copyBars (preview, selectionStart, selectionBars), dropBar);
    else
        chordEdits::moveBars (preview, selectionStart, selectionBars, dropBar);

    dragPreview = std::move (preview);
    setMouseCursor (copying ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
    repaint();
}

void ChordLane::mouseUp (const juce::MouseEvent& e)
{
    if (std::exchange (pressedDiagram, false))
        return;

    const auto finished = drag;
    const bool dragged = e.mouseWasDraggedSinceMouseDown();
    drag = Drag::none;
    setMouseCursor (juce::MouseCursor::NormalCursor);

    if (finished == Drag::move && dragged && dragPreview.has_value())
    {
        auto newChords = std::move (*dragPreview);
        dragPreview.reset();

        if (dropBar == selectionStart)
        {
            repaint();
            return;
        }

        // Moving over bars it is leaving changes nothing to ask about; only the chords it lands on count.
        const auto& chords = engine.getChords();
        int replaced = 0;

        for (int bar = dropBar; bar < dropBar + selectionBars; ++bar)
        {
            const auto before = chords.find (bar);
            const auto after = std::as_const (newChords).find (bar);
            const bool leaving = ! dragCopies && isSelected (bar);
            replaced += before != chords.end() && ! leaving && (after == newChords.end() || ! (after->second == before->second)) ? 1 : 0;
        }

        applyAsking (std::move (newChords), replaced, dropBar, selectionBars, dragCopies ? "Copy" : "Move");
        return;
    }

    dragPreview.reset();

    if (dragged || e.mods.isShiftDown())
    {
        repaint();
        return;
    }

    // A click: pick this bar's chord. A click (or right-click) inside a selection of several bars
    // keeps it, so its menu can copy or duplicate them all.
    const int bar = juce::jmax (0, barAt (e.position.x));

    if (! isSelected (bar) || blockSelectedOnPress)
        selectBars (bar, 1);

    showMenu (bar);
}

bool ChordLane::keyPressed (const juce::KeyPress& key)
{
    const auto cmd = juce::ModifierKeys::commandModifier;

    if (key == juce::KeyPress ('c', cmd, 0))      copySelection();
    else if (key == juce::KeyPress ('x', cmd, 0)) cutSelection();
    else if (key == juce::KeyPress ('v', cmd, 0)) paste();
    else if (key == juce::KeyPress ('d', cmd, 0)) duplicateSelection();
    else if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) clearSelection();
    else if (key == juce::KeyPress::escapeKey)    selectBars (0, 0);
    else if (key == juce::KeyPress::leftKey && selectionBars > 0)  selectBars (juce::jmax (0, selectionStart - 1), 1);
    else if (key == juce::KeyPress::rightKey && selectionBars > 0) selectBars (selectionStart + selectionBars, 1);
    else return false;

    return true;
}

//==============================================================================
ChordBars ChordLane::getClipboard() const
{
    // Chord symbols copied from somewhere else, such as a chord sheet or a web page.
    const auto text = juce::SystemClipboard::getTextFromClipboard();

    if (text != clipboardText && text.length() < 2000)
        if (auto fromText = chordEdits::fromText (text); ! fromText.chords.empty())
            return fromText;

    return clipboard;
}

void ChordLane::copySelection()
{
    if (selectionBars <= 0)
        return;

    clipboard = chordEdits::copyBars (engine.getChords(), selectionStart, selectionBars);
    clipboardText = chordEdits::toText (clipboard);
    juce::SystemClipboard::copyTextToClipboard (clipboardText);
}

void ChordLane::cutSelection()
{
    copySelection();

    auto chords = engine.getChords();
    chordEdits::clearBars (chords, selectionStart, selectionBars);
    engine.setChords (std::move (chords));
}

void ChordLane::paste()
{
    const auto copy = getClipboard();

    if (copy.isEmpty())
        return;

    // At the selection, or else at the bar the playhead is in.
    const int at = selectionBars > 0 ? selectionStart : (int) std::floor (engine.getPositionBeats() / 4.0);
    auto chords = engine.getChords();
    const int replaced = chordEdits::countReplaced (chords, copy, at);
    chordEdits::pasteBars (chords, copy, at);
    applyAsking (std::move (chords), replaced, at, copy.bars, "Paste");
}

void ChordLane::duplicateSelection()
{
    if (selectionBars <= 0)
        return;

    const auto copy = chordEdits::copyBars (engine.getChords(), selectionStart, selectionBars);
    const int at = selectionStart + selectionBars;
    auto chords = engine.getChords();
    const int replaced = chordEdits::countReplaced (chords, copy, at);
    chordEdits::pasteBars (chords, copy, at);
    applyAsking (std::move (chords), replaced, at, selectionBars, "Duplicate");
}

void ChordLane::repeatSelectionToEnd()
{
    const int end = engine.getSongEndBar();

    if (selectionBars <= 0 || end <= selectionStart + selectionBars)
        return;

    auto chords = engine.getChords();
    const auto before = chords;
    const int from = selectionStart + selectionBars;
    chordEdits::repeatUntil (chords, selectionStart, selectionBars, end);
    const int replaced = chordEdits::countReplaced (before, chordEdits::copyBars (chords, from, end - from), from);
    applyAsking (std::move (chords), replaced, selectionStart, end - selectionStart, "Repeat");
}

void ChordLane::clearSelection()
{
    if (selectionBars <= 0)
        return;

    auto chords = engine.getChords();
    chordEdits::clearBars (chords, selectionStart, selectionBars);
    engine.setChords (std::move (chords));
}

void ChordLane::applyAsking (ChordMap newChords, int replaced, int selectFrom, int selectBarsAfter, const juce::String& action)
{
    auto apply = [safeThis = juce::Component::SafePointer<ChordLane> (this), newChords, selectFrom, selectBarsAfter]
    {
        if (auto* self = safeThis.getComponent())
        {
            self->engine.setChords (newChords);
            self->selectBars (selectFrom, selectBarsAfter);
        }
    };

    if (replaced == 0)
    {
        apply();
        return;
    }

    const int last = selectFrom + selectBarsAfter;
    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Replace chords?",
                                        "Bars " + juce::String (selectFrom + 1) + " to " + juce::String (last) + " already have "
                                            + juce::String (replaced) + (replaced == 1 ? " other chord" : " other chords")
                                            + (replaced == 1 ? ". Replace it?" : ". Replace them?"),
                                        action + " and replace", "Cancel", this,
                                        juce::ModalCallbackFunction::create ([apply, safeThis = juce::Component::SafePointer<ChordLane> (this)] (int result)
                                        {
                                            if (result == 1)
                                                apply();
                                            else if (auto* self = safeThis.getComponent())
                                                self->repaint();
                                        }));
}

void ChordLane::showMenu (int bar)
{
    const auto& key = engine.getKey();
    const auto* current = engine.getChord (bar);
    const auto* previous = engine.getChord (bar - 1);

    std::vector<Chord> choices;
    auto addChord = [&] (juce::PopupMenu& m, const Chord& chord, const juce::String& hint = {})
    {
        choices.push_back (chord);
        juce::PopupMenu::Item item (chord.getName());
        item.itemID = (int) choices.size();
        item.isTicked = current != nullptr && *current == chord;
        item.shortcutKeyDescription = hint;   // shown right-aligned, in the shortcut column
        m.addItem (item);
    };

    // Each in-key chord with its roman numeral and harmonic function, e.g. "Dm   iv  subdominant".
    auto describe = [] (int degree, const Chord& chord)
    {
        return harmony::getRomanNumeral (degree, chord) + "   " + harmony::getFunction (degree);
    };

    auto seventh = [&key] (int degree)
    {
        return Chord { harmony::triad (key, degree).root, gen::getChordIntervals (key, degree, "seventh") };
    };

    juce::PopupMenu menu, sevenths;
    menu.addSectionHeader ("Bar " + juce::String (bar + 1) + (current != nullptr ? ": " + current->getName() : juce::String()));

    // What usually comes next, given the chord before this bar.
    if (previous != nullptr)
    {
        if (const int from = harmony::findDegree (key, *previous); from > 0)
        {
            menu.addSectionHeader ("Usual after " + previous->getName());

            for (int degree : harmony::getUsualNextDegrees (key, from))
                addChord (menu, harmony::triad (key, degree), describe (degree, harmony::triad (key, degree)));
        }
    }

    menu.addSectionHeader ("In " + key.getName());

    for (int degree = 1; degree <= 7; ++degree)
    {
        const auto chord = harmony::triad (key, degree);
        addChord (menu, chord, describe (degree, chord));

        const auto withSeventh = seventh (degree);
        auto numeral = harmony::getRomanNumeral (degree, withSeventh);
        if (withSeventh.intervals.contains (6) && withSeventh.intervals.contains (10))
            numeral = numeral.replace (juce::CharPointer_UTF8 ("\xc2\xb0"), juce::CharPointer_UTF8 ("\xc3\xb8")); // half-diminished
        addChord (sevenths, withSeventh, numeral + "7   " + harmony::getFunction (degree));
    }

    if (const auto dominant = harmony::getBorrowedDominant (key))
    {
        addChord (menu, *dominant, "V   dominant, from harmonic minor");
        addChord (sevenths, { dominant->root, { 0, 4, 7, 10 } }, "V7   dominant, from harmonic minor");
    }

    menu.addSubMenu ("Sevenths in " + key.getName(), sevenths);

    juce::PopupMenu other;
    for (int root = 0; root < 12; ++root)
    {
        juce::PopupMenu qualities;
        for (auto& q : Chord::getQualities())
            addChord (qualities, { root, q.intervals });

        other.addSubMenu (MusicalKey::getTonicName (root), qualities);
    }
    menu.addSubMenu ("Other chord", other);

    enum { typeId = 10000, repeatId, clearId, clearAllId, explainId, playId, copyId, cutId, pasteId, duplicateId, repeatToEndId, howToPlayId, playAlongId };
    menu.addSeparator();
    menu.addItem (typeId, "Type a chord...");

    if (previous != nullptr)
        menu.addItem (repeatId, "Same as bar " + juce::String (bar) + " (" + previous->getName() + ")");

    // Editing the selected bars (this bar when only it is selected).
    const auto& chords = engine.getChords();
    const bool anyChord = chords.lower_bound (selectionStart) != chords.lower_bound (selectionStart + selectionBars);
    const int songEnd = engine.getSongEndBar();
    const auto bars = selectionBars > 1 ? "bars " + juce::String (selectionStart + 1) + "-" + juce::String (selectionStart + selectionBars)
                                        : juce::String ("this bar");
    const auto pasted = getClipboard();

    auto addEdit = [&menu] (int id, const juce::String& text, const juce::String& shortcut, bool enabled)
    {
        juce::PopupMenu::Item item (text);
        item.itemID = id;
        item.isEnabled = enabled;
        item.shortcutKeyDescription = shortcut;
        menu.addItem (item);
    };

    menu.addSeparator();
    addEdit (copyId, "Copy " + bars, juce::CharPointer_UTF8 ("\xe2\x8c\x98""C"), anyChord);
    addEdit (cutId, "Cut " + bars, juce::CharPointer_UTF8 ("\xe2\x8c\x98""X"), anyChord);
    addEdit (pasteId, pasted.isEmpty() ? juce::String ("Paste") : "Paste " + chordEdits::toText (pasted).substring (0, 40),
             juce::CharPointer_UTF8 ("\xe2\x8c\x98""V"), ! pasted.isEmpty());
    addEdit (duplicateId, "Duplicate " + bars + " after itself", juce::CharPointer_UTF8 ("\xe2\x8c\x98""D"), anyChord);
    addEdit (repeatToEndId, "Repeat " + bars + " until the end of the song" + (songEnd > 0 ? " (bar " + juce::String (songEnd) + ")" : juce::String()),
             {}, anyChord && songEnd > selectionStart + selectionBars);
    addEdit (clearId, "Clear " + bars, juce::CharPointer_UTF8 ("\xe2\x8c\xab"), anyChord);
    menu.addItem (clearAllId, "Clear all chords", ! chords.empty());

    menu.addSeparator();

    if (onPlayChords != nullptr)
        menu.addItem (playId, "Play chords as...", ! engine.getChords().empty());

    if (onPlayAlong != nullptr)
        menu.addItem (playAlongId, "Play along with the chords...", ! engine.getChords().empty());

    if (onExplainProgressions != nullptr)
        menu.addItem (explainId, "Explain chord progressions...");

    if (onShowHowToPlay != nullptr && current != nullptr)
        menu.addItem (howToPlayId, "How to play " + current->getName() + " on piano and guitar...");

    const auto area = juce::Rectangle<float> (timeline.beatToX (bar * 4.0), 0.0f, (float) (timeline.pixelsPerBeat * 4.0), (float) getHeight());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (area.toNearestInt())),
                        [safeThis = juce::Component::SafePointer<ChordLane> (this), bar, choices] (int result)
                        {
                            auto* self = safeThis.getComponent();

                            if (self == nullptr || result == 0)
                                return;

                            auto& track = self->engine;

                            if (result >= 1 && result <= (int) choices.size())
                                track.setChord (bar, choices[(size_t) result - 1]);
                            else if (result == typeId)
                                self->typeChord (bar);
                            else if (result == repeatId && track.getChord (bar - 1) != nullptr)
                                track.setChord (bar, *track.getChord (bar - 1));
                            else if (result == clearId)
                                self->clearSelection();
                            else if (result == copyId)
                                self->copySelection();
                            else if (result == cutId)
                                self->cutSelection();
                            else if (result == pasteId)
                                self->paste();
                            else if (result == duplicateId)
                                self->duplicateSelection();
                            else if (result == repeatToEndId)
                                self->repeatSelectionToEnd();
                            else if (result == clearAllId)
                                track.setChords ({});
                            else if (result == howToPlayId && self->onShowHowToPlay != nullptr)
                                self->onShowHowToPlay (bar);
                            else if (result == explainId && self->onExplainProgressions != nullptr)
                                self->onExplainProgressions (bar);
                            else if (result == playId && self->onPlayChords != nullptr)
                                self->onPlayChords (bar);
                            else if (result == playAlongId && self->onPlayAlong != nullptr)
                                self->onPlayAlong();
                        });
}

void ChordLane::typeChord (int bar)
{
    const auto* current = engine.getChord (bar);
    auto* window = new juce::AlertWindow ("Chord for bar " + juce::String (bar + 1),
                                          "Type a chord symbol, for example Am, F#m7, Bbmaj7 or Csus4.",
                                          juce::MessageBoxIconType::NoIcon, this);
    window->addTextEditor ("chord", current != nullptr ? current->getName() : juce::String());
    window->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safeThis = juce::Component::SafePointer<ChordLane> (this), window, bar] (int result)
        {
            auto* self = safeThis.getComponent();

            if (self == nullptr || result != 1)
                return;

            const auto text = window->getTextEditorContents ("chord");

            if (text.trim().isEmpty())
                self->engine.setChord (bar, std::nullopt);
            else if (const auto chord = Chord::fromName (text))
                self->engine.setChord (bar, *chord);
            else
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Unknown chord",
                                                        "\"" + text + "\" isn't a chord symbol I know.");
        }), true);
}
} // namespace daw
