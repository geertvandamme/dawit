#include "PianoRoll.h"
#include "ChordLane.h"
#include "generate/RhythmGuide.h"

namespace daw
{
PianoRollGrid::PianoRollGrid (Engine& e, const PitchRows& r) : engine (e), rows (r)
{
    setWantsKeyboardFocus (true);
}

namespace
{
    /** "C4" for middle C. */
    juce::String pitchName (int pitch) { return MusicalKey::getTonicName (pitch % 12) + juce::String (pitch / 12 - 1); }
}

void PianoRollGrid::selectOnly (int index)
{
    selected.clear();

    if (clip != nullptr && juce::isPositiveAndBelow (index, (int) clip->notes.size()))
        selected.add (index);

    repaint();
}

juce::String PianoRollGrid::getTooltip()
{
    if (clip == nullptr || mode != Mode::none)
        return {};

    const int index = noteAt (getMouseXYRelative().toFloat());

    if (index < 0)
        return {};

    const auto& note = clip->notes[(size_t) index];
    return pitchName (note.pitch) + ": " + gen::rhythm::describeLength (note.length) + ", from "
           + gen::rhythm::formatPosition (clip->startBeat + note.start) + ", velocity " + juce::String (juce::roundToInt (note.velocity * 127.0f));
}

void PianoRollGrid::setClip (Track* t, MidiClip* c)
{
    track = t;
    clip = c;
    selected.clear();
    mode = Mode::none;
    updateSize();
    repaint();
}

void PianoRollGrid::rowsChanged()
{
    if (clip != nullptr)
    {
        for (int i = selected.size(); --i >= 0;)
            if (! juce::isPositiveAndBelow (selected[i], (int) clip->notes.size())
                || ! rows.isShown (clip->notes[(size_t) selected[i]].pitch))
                selected.remove (i);
    }

    mode = Mode::none;
    updateSize();
    repaint();
}

bool PianoRollGrid::selectPitch (int pitch, bool everyOctave, bool add)
{
    if (clip == nullptr)
        return false;

    juce::SortedSet<int> matching;

    for (int i = 0; i < (int) clip->notes.size(); ++i)
    {
        const int p = clip->notes[(size_t) i].pitch;

        if (rows.isShown (p) && (everyOctave ? p % 12 == pitch % 12 : p == pitch))
            matching.add (i);
    }

    if (matching.isEmpty())
        return false;

    if (! add)
        selected = matching;
    else if (std::all_of (matching.begin(), matching.end(), [this] (int i) { return selected.contains (i); }))
        selected.removeValuesIn (matching);
    else
        selected.addSet (matching);

    mode = Mode::none;
    repaint();
    return true;
}

void PianoRollGrid::updateSize()
{
    const double beats = clip != nullptr ? clip->lengthBeats + 8.0 : 16.0;
    setSize (juce::jmax (minimumWidth, (int) (beats * pixelsPerBeat)), rows.getNumRows() * rowHeight);
}

juce::Rectangle<float> PianoRollGrid::getNoteBounds (const MidiNote& n) const
{
    // A hidden note gets empty bounds, so it is neither drawn nor hit.
    const int row = rows.getRow (n.pitch);

    if (row < 0)
        return {};

    return { (float) (n.start * pixelsPerBeat), (float) (row * rowHeight),
             juce::jmax (4.0f, (float) (n.length * pixelsPerBeat)), (float) rowHeight - 1.0f };
}

int PianoRollGrid::noteAt (juce::Point<float> p) const
{
    if (clip == nullptr)
        return -1;

    for (int i = (int) clip->notes.size(); --i >= 0;)
        if (getNoteBounds (clip->notes[(size_t) i]).contains (p))
            return i;

    return -1;
}

bool PianoRollGrid::isOnRightEdge (int index, juce::Point<float> p) const
{
    const auto r = getNoteBounds (clip->notes[(size_t) index]);
    return p.x > r.getRight() - juce::jmin (6.0f, r.getWidth() / 3.0f);
}

//==============================================================================
void PianoRollGrid::paint (juce::Graphics& g)
{
    const auto area = g.getClipBounds();
    g.fillAll (theme::lane);

    if (clip == nullptr)
        return;

    // Pitch rows, with a stronger line where one octave (C to B) ends and the next begins.
    for (int row = yToRow ((float) area.getY()); row <= yToRow ((float) area.getBottom()); ++row)
    {
        const int p = rows.getPitch (row);
        const int y = row * rowHeight;

        if (juce::MidiMessage::isMidiNoteBlack (p))
        {
            g.setColour (theme::laneAlt.darker (0.3f));
            g.fillRect (area.getX(), y, area.getWidth(), rowHeight);
        }

        const bool octaveEnds = row == rows.getNumRows() - 1 || rows.getPitch (row + 1) / 12 != p / 12;
        g.setColour (octaveEnds ? theme::gridBar.withAlpha (0.8f) : theme::grid.withAlpha (0.4f));
        g.fillRect (area.getX(), y + rowHeight - 1, area.getWidth(), 1);
    }

    if (showChords)
        paintChordRows (g, area);

    // Time grid
    double step = snapBeats;
    while (step * pixelsPerBeat < 8.0)
        step *= 2.0;

    for (double b = std::floor (area.getX() / pixelsPerBeat / step) * step; b * pixelsPerBeat <= area.getRight(); b += step)
    {
        const bool isBar = std::fmod (b, 4.0) == 0.0;
        const bool isBeat = std::fmod (b, 1.0) == 0.0;
        g.setColour (isBar ? theme::gridBar : isBeat ? theme::grid.brighter (0.15f) : theme::grid.withAlpha (0.5f));
        g.fillRect ((float) (b * pixelsPerBeat), (float) area.getY(), 1.0f, (float) area.getHeight());
    }

    // Area past the end of the clip
    const float endX = (float) (clip->lengthBeats * pixelsPerBeat);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (endX, 0.0f, (float) getWidth() - endX, (float) getHeight());
    g.setColour (theme::accent.withAlpha (0.7f));
    g.fillRect (endX, 0.0f, 1.5f, (float) getHeight());

    // Notes
    const auto base = track != nullptr ? track->colour : theme::accent;

    for (int i = 0; i < (int) clip->notes.size(); ++i)
    {
        const auto& n = clip->notes[(size_t) i];
        const auto r = getNoteBounds (n);

        if (! r.intersects (area.toFloat()))
            continue;

        const bool inside = n.start < clip->lengthBeats;
        auto colour = base.darker (0.9f * (1.0f - n.velocity)).withAlpha (inside ? 1.0f : 0.35f);
        g.setColour (colour);
        g.fillRoundedRectangle (r, 2.0f);

        const bool isSelected = selected.contains (i);
        g.setColour (isSelected ? juce::Colours::white : juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, isSelected ? 1.5f : 1.0f);
    }

    if (mode == Mode::lasso)
    {
        g.setColour (theme::accent.withAlpha (0.15f));
        g.fillRect (lasso);
        g.setColour (theme::accent);
        g.drawRect (lasso, 1.0f);
    }

    // Playhead, where updatePlayhead() last put it (see TrackLanes).
    g.setColour (theme::playhead);
    g.fillRect (playheadX, 0.0f, 1.5f, (float) getHeight());
}

void PianoRollGrid::paintChordRows (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Each song bar the clip covers tints the rows of its chord, the root a little more.
    const double start = clip->startBeat, end = start + clip->lengthBeats;
    const int firstRow = yToRow ((float) area.getY()), lastRow = yToRow ((float) area.getBottom());

    for (int bar = (int) std::floor (start / 4.0 + 1e-6); bar * 4.0 < end; ++bar)
    {
        const auto* chord = engine.getChord (bar);

        if (chord == nullptr)
            continue;

        const float x1 = (float) ((juce::jmax (start, bar * 4.0) - start) * pixelsPerBeat);
        const float x2 = (float) ((juce::jmin (end, bar * 4.0 + 4.0) - start) * pixelsPerBeat);
        const auto pitchClasses = chord->getPitchClasses();
        const auto colour = ChordLane::getColour (*chord).brighter (0.6f);

        for (int row = firstRow; row <= lastRow; ++row)
        {
            const int pitchClass = rows.getPitch (row) % 12;

            if (pitchClasses.contains (pitchClass))
            {
                g.setColour (colour.withAlpha (pitchClass == chord->root ? 0.3f : 0.18f));
                g.fillRect (x1, (float) (row * rowHeight), x2 - x1, (float) rowHeight - 1.0f);
            }
        }
    }
}

void PianoRollGrid::updatePlayhead()
{
    if (clip == nullptr)
        return;

    const double rel = engine.getPositionBeats() - clip->startBeat;
    const float x = engine.isPlaying() && rel >= 0.0 && rel <= clip->lengthBeats ? (float) (rel * pixelsPerBeat) : -100.0f;

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 6, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 6, getHeight());
}

//==============================================================================
void PianoRollGrid::preview (int pitch, float velocity)
{
    lastPreviewPitch = pitch;
    engine.previewNote (pitch, velocity);
}

void PianoRollGrid::commit()
{
    engine.notesChanged();
    repaint();
}

void PianoRollGrid::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    mode = Mode::none;

    if (clip == nullptr)
        return;

    const int index = noteAt (e.position);

    if (e.mods.isPopupMenu())
    {
        if (index >= 0)
        {
            juce::SortedSet<int> one;
            one.add (index);
            deleteNotes (one);
        }

        return;
    }

    if (index >= 0)
    {
        if (e.mods.isShiftDown())
        {
            if (selected.contains (index))
                selected.removeValue (index);
            else
                selected.add (index);
        }
        else if (! selected.contains (index))
        {
            selected.clear();
            selected.add (index);
        }

        if (! selected.contains (index))
        {
            repaint();
            return;
        }

        anchor = index;
        mode = isOnRightEdge (index, e.position) ? Mode::resize : Mode::move;
        preview (clip->notes[(size_t) index].pitch, clip->notes[(size_t) index].velocity);
    }
    else if (selectMode || e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        if (! e.mods.isShiftDown())
            selected.clear();

        lassoBase = selected;
        mode = Mode::lasso;
        lasso = { e.position, e.position };
        repaint();
        return;
    }
    else
    {
        MidiNote note;
        note.pitch = yToPitch (e.position.y);
        note.start = juce::jmax (0.0, snapDown (xToBeat (e.position.x)));
        note.length = lastLength;
        note.velocity = 0.8f;
        {
            const juce::ScopedLock sl (engine.getLock());
            clip->notes.push_back (note);
        }

        anchor = (int) clip->notes.size() - 1;
        justCreated = anchor;
        justCreatedTime = juce::Time::getMillisecondCounter();
        selected.clear();
        selected.add (anchor);
        mode = Mode::resize;
        preview (note.pitch, note.velocity);
        commit();
    }

    dragOriginal = clip->notes;
    dragStartBeat = xToBeat (e.position.x);
    dragStartRow = yToRow (e.position.y);
    repaint();
}

void PianoRollGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (clip == nullptr || mode == Mode::none)
        return;

    if (mode == Mode::lasso)
    {
        // Notes light up as the rectangle reaches them.
        lasso = juce::Rectangle<float> (e.mouseDownPosition, e.position);
        selected = lassoBase;

        for (int i = 0; i < (int) clip->notes.size(); ++i)
            if (getNoteBounds (clip->notes[(size_t) i]).intersects (lasso))
                selected.add (i);

        repaint();
        return;
    }

    if (anchor < 0 || anchor >= (int) dragOriginal.size() || dragOriginal.size() != clip->notes.size())
        return;

    const auto& a = dragOriginal[(size_t) anchor];
    const double mouseDelta = xToBeat (e.position.x) - dragStartBeat;
    const bool free = e.mods.isAltDown();

    if (mode == Mode::move)
    {
        // Snap the anchor note, then move every selected note by the same amount, and by the same
        // number of rows (with the scale filter on, that keeps the notes in the key).
        double delta = (free ? a.start + mouseDelta : snapNearest (a.start + mouseDelta)) - a.start;
        int rowDelta = yToRow (e.position.y) - dragStartRow;

        for (int i : selected)
        {
            const auto& o = dragOriginal[(size_t) i];
            delta = juce::jmax (delta, -o.start);

            if (const int row = rows.getRow (o.pitch); row >= 0)
                rowDelta = juce::jlimit (-row, rows.getNumRows() - 1 - row, rowDelta);
        }

        const auto movedPitch = [this, rowDelta] (int pitch)
        {
            const int row = rows.getRow (pitch);
            return row >= 0 ? rows.getPitch (row + rowDelta) : pitch;
        };

        {
            const juce::ScopedLock sl (engine.getLock());

            for (int i : selected)
            {
                auto& n = clip->notes[(size_t) i];
                const auto& o = dragOriginal[(size_t) i];
                n.start = o.start + delta;
                n.pitch = movedPitch (o.pitch);
            }
        }

        if (movedPitch (a.pitch) != lastPreviewPitch)
            preview (movedPitch (a.pitch), a.velocity);
    }
    else if (mode == Mode::resize)
    {
        const double end = a.getEnd() + mouseDelta;
        const double newEnd = free ? end : snapNearest (end);
        const double minLength = free ? 1.0 / 64.0 : snapBeats;
        const double deltaLength = newEnd - a.getEnd();

        const juce::ScopedLock sl (engine.getLock());

        for (int i : selected)
            clip->notes[(size_t) i].length = juce::jmax (minLength, dragOriginal[(size_t) i].length + deltaLength);
    }

    commit();
}

void PianoRollGrid::mouseUp (const juce::MouseEvent&)
{
    if (clip == nullptr)
        return;

    if (mode == Mode::lasso)
        lasso = {};
    else if (mode != Mode::none && anchor >= 0 && anchor < (int) clip->notes.size())
    {
        lastLength = clip->notes[(size_t) anchor].length;
        extendClipToFitNotes();
    }

    mode = Mode::none;
    repaint();
}

void PianoRollGrid::mouseMove (const juce::MouseEvent& e)
{
    const int index = noteAt (e.position);
    setMouseCursor (index < 0 ? (selectMode ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor)
                              : isOnRightEdge (index, e.position) ? juce::MouseCursor::LeftRightResizeCursor
                                                                  : juce::MouseCursor::DraggingHandCursor);
}

void PianoRollGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int index = noteAt (e.position);

    // Ignore the second click of a double-click that just drew this note, and quick clicks while selecting.
    if (index < 0 || selectMode || (index == justCreated && juce::Time::getMillisecondCounter() - justCreatedTime < 600))
        return;

    juce::SortedSet<int> one;
    one.add (index);
    deleteNotes (one);
}

void PianoRollGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if ((e.mods.isCommandDown() || e.mods.isCtrlDown()) && wheel.deltaY != 0.0f)
    {
        if (onZoom)
            onZoom (wheel.deltaY > 0 ? 1.12 : 1.0 / 1.12, e.position.x);

        return;
    }

    juce::Component::mouseWheelMove (e, wheel);
}

bool PianoRollGrid::keyPressed (const juce::KeyPress& key)
{
    if (clip == nullptr)
        return false;

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteNotes (selected);
        return true;
    }

    if (key == juce::KeyPress ('a', juce::ModifierKeys::commandModifier, 0))
    {
        selected.clear();
        for (int i = 0; i < (int) clip->notes.size(); ++i)
            if (rows.isShown (clip->notes[(size_t) i].pitch))
                selected.add (i);

        repaint();
        return true;
    }

    if (key == juce::KeyPress::escapeKey)
    {
        selected.clear();
        repaint();
        return true;
    }

    const bool shift = key.getModifiers().isShiftDown();

    if (key.isKeyCode (juce::KeyPress::upKey))    { shift ? transposeSelected (12) : stepSelected (1);   return true; }
    if (key.isKeyCode (juce::KeyPress::downKey))  { shift ? transposeSelected (-12) : stepSelected (-1); return true; }
    if (key.isKeyCode (juce::KeyPress::leftKey))  { shiftSelected (-snapBeats); return true; }
    if (key.isKeyCode (juce::KeyPress::rightKey)) { shiftSelected (snapBeats);  return true; }

    return false;
}

//==============================================================================
void PianoRollGrid::deleteNotes (juce::SortedSet<int> indices)
{
    if (indices.isEmpty())
        return;

    {
        const juce::ScopedLock sl (engine.getLock());

        for (int i = indices.size(); --i >= 0;)
            if (juce::isPositiveAndBelow (indices[i], (int) clip->notes.size()))
                clip->notes.erase (clip->notes.begin() + indices[i]);
    }

    selected.clear();
    mode = Mode::none;
    justCreated = -1;
    commit();
}

void PianoRollGrid::transposeSelected (int semitones)
{
    for (int i : selected)
    {
        const int p = clip->notes[(size_t) i].pitch + semitones;
        if (p < 0 || p > 127)
            return;
    }

    {
        const juce::ScopedLock sl (engine.getLock());
        for (int i : selected)
            clip->notes[(size_t) i].pitch += semitones;
    }

    if (! selected.isEmpty())
        preview (clip->notes[(size_t) selected.getFirst()].pitch, 0.8f);

    commit();
}

void PianoRollGrid::stepSelected (int rowsUp)
{
    // One row: a semitone, or the next note of the key with the scale filter on.
    for (int i : selected)
    {
        const int row = rows.getRow (clip->notes[(size_t) i].pitch) - rowsUp;
        if (row < 0 || row >= rows.getNumRows())
            return;
    }

    {
        const juce::ScopedLock sl (engine.getLock());
        for (int i : selected)
        {
            auto& n = clip->notes[(size_t) i];
            n.pitch = rows.getPitch (rows.getRow (n.pitch) - rowsUp);
        }
    }

    if (! selected.isEmpty())
        preview (clip->notes[(size_t) selected.getFirst()].pitch, 0.8f);

    commit();
}

void PianoRollGrid::shiftSelected (double beats)
{
    for (int i : selected)
        if (clip->notes[(size_t) i].start + beats < 0.0)
            return;

    {
        const juce::ScopedLock sl (engine.getLock());
        for (int i : selected)
            clip->notes[(size_t) i].start += beats;
    }

    extendClipToFitNotes();
    commit();
}

void PianoRollGrid::extendClipToFitNotes()
{
    double end = 0.0;
    for (auto& n : clip->notes)
        end = juce::jmax (end, n.getEnd());

    if (end > clip->lengthBeats)
    {
        {
            const juce::ScopedLock sl (engine.getLock());
            clip->lengthBeats = std::ceil (end / 4.0) * 4.0;
        }

        updateSize();
        commit();
    }
}

//==============================================================================
void PianoKeyboardStrip::setChord (const std::optional<Chord>& newChord)
{
    if (newChord != chord)
    {
        chord = newChord;
        repaint();
    }
}

void PianoKeyboardStrip::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    const int rowHeight = PianoRollGrid::rowHeight;
    const int first = juce::jlimit (0, rows.getNumRows() - 1, offset / rowHeight);
    const int last = juce::jlimit (0, rows.getNumRows() - 1, (offset + getHeight()) / rowHeight);

    // With only the key's notes shown, the keys no longer look like a piano, so each one gets its name.
    const bool nameEveryKey = rows.getNumRows() < 128;
    const auto chordPitchClasses = chord.has_value() ? chord->getPitchClasses() : juce::Array<int>();
    const auto chordColour = chord.has_value() ? ChordLane::getColour (*chord) : juce::Colour();

    for (int row = first; row <= last; ++row)
    {
        const int p = rows.getPitch (row);
        const int y = row * rowHeight - offset;
        const bool black = juce::MidiMessage::isMidiNoteBlack (p);
        const bool inChord = chordPitchClasses.contains (p % 12);
        const bool isRoot = inChord && p % 12 == chord->root;
        const auto key = juce::Rectangle<int> (0, y, black ? getWidth() * 3 / 5 : getWidth(), rowHeight);

        auto colour = black ? juce::Colour (0xff15161a) : juce::Colour (0xffd8d9dd);

        if (inChord)
            colour = isRoot ? chordColour.brighter (0.2f) : chordColour.brighter (0.9f);

        g.setColour (colour);
        g.fillRect (key.withTrimmedBottom (1));

        if (nameEveryKey || p % 12 == 0)
        {
            g.setFont (juce::FontOptions (10.0f, isRoot ? juce::Font::bold : juce::Font::plain));
            g.setColour (isRoot ? juce::Colours::white : black && ! inChord ? theme::textDim : juce::Colour (0xff40434a));
            g.drawText (MusicalKey::getTonicName (p) + juce::String (p / 12 - 1), key.withTrimmedRight (4),
                        juce::Justification::centredRight);
        }
    }

    g.setColour (juce::Colours::black);
    g.fillRect (getWidth() - 1, 0, 1, getHeight());
}

void PianoKeyboardStrip::mouseDown (const juce::MouseEvent& e)
{
    const int row = (int) std::floor ((e.position.y + (float) offset) / (float) PianoRollGrid::rowHeight);
    const int pitch = rows.getPitch (row);
    engine.previewNote (pitch, 0.8f, 400);

    if (onKeyClicked)
        onKeyClicked (pitch, e.mods);
}

//==============================================================================
PianoRoll::PianoRoll (Engine& e, Selection& s) : engine (e), selection (s)
{
    title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (title);

    snapLabel.setColour (juce::Label::textColourId, theme::textDim);
    snapLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (snapLabel);

    snapBox.addItemList ({ "1 bar (whole note)", "1/2 (half note)", "1/4 (quarter note)", "1/8 (eighth note)",
                           "1/16 (sixteenth note)", "1/32 (thirty-second note)" }, 1);
    snapBox.setTooltip ("The smallest step for drawing, moving and resizing notes, as a note value. "
                        "Hold Option/Alt while dragging to go off the grid.");
    snapBox.setSelectedId (5, juce::dontSendNotification);
    snapBox.setWantsKeyboardFocus (false);
    snapBox.onChange = [this]
    {
        static const double values[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 };
        grid.snapBeats = values[juce::jlimit (0, 5, snapBox.getSelectedId() - 1)];
        grid.repaint();
    };
    addAndMakeVisible (snapBox);

    rhythmButton.setWantsKeyboardFocus (false);
    rhythmButton.setTooltip ("Note lengths, beats and bars: a guide to whole, half, quarter, eighth and sixteenth notes, "
                             "rests, triplets, counting and tempo, and how they show in DawIt");
    rhythmButton.onClick = [this] { if (onShowRhythmGuide) onShowRhythmGuide(); };
    addAndMakeVisible (rhythmButton);

    midiGuideButton.setWantsKeyboardFocus (false);
    midiGuideButton.setTooltip ("A guide to what MIDI is: instructions rather than sound, the cable, the messages byte by byte, note "
                                "numbers, velocity, MIDI files, with this clip's notes as the example");
    midiGuideButton.onClick = [this] { if (onShowMidiGuide) onShowMidiGuide(); };
    addAndMakeVisible (midiGuideButton);

    // The switch between the piano roll and the score: two linked toggle buttons at the left of the header.
    pianoRollModeButton.setTooltip ("Show the notes as a piano roll, where you draw and edit them");
    scoreModeButton.setTooltip ("Show the part as sheet music: notes, rests and ties on a staff. Read-only: the MIDI isn't changed");
    pianoRollModeButton.setConnectedEdges (juce::Button::ConnectedOnRight);
    scoreModeButton.setConnectedEdges (juce::Button::ConnectedOnLeft);

    for (auto* b : { &pianoRollModeButton, &scoreModeButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (4711);
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    pianoRollModeButton.onClick = [this] { if (pianoRollModeButton.getToggleState()) setScoreMode (false); };
    scoreModeButton.onClick = [this] { if (scoreModeButton.getToggleState()) setScoreMode (true); };

    clefButton.setTooltip ("Which staff the part is written on. Auto picks the one that needs the fewest ledger lines");
    clefButton.onClick = [this] { scoreView.showClefMenu (clefButton); };
    pdfButton.setTooltip ("Save this part as sheet music to print or read on a tablet");
    pdfButton.onClick = [this] { if (onExportSheetMusic) onExportSheetMusic(); };
    readMusicButton.setTooltip ("How to read music: the staff, clefs, note names, key signatures and how DawIt writes your MIDI as a score");
    readMusicButton.onClick = [this] { if (onShowReadingGuide) onShowReadingGuide(); };

    for (auto* b : { (juce::Button*) &clefButton, (juce::Button*) &pdfButton, (juce::Button*) &readMusicButton })
    {
        b->setWantsKeyboardFocus (false);
        addChildComponent (b);
    }

    scoreView.onOpenPianoRoll = [this] (Clip* clip, int note, double beat) { openFromScore (clip, note, beat); };
    scoreView.onShowGuide = [this] { if (onShowReadingGuide) onShowReadingGuide(); };
    scoreView.onExportPdf = [this] { if (onExportSheetMusic) onExportSheetMusic(); };
    addChildComponent (scoreView);

    closeButton.setWantsKeyboardFocus (false);
    closeButton.setTooltip ("Close the clip editor, to give the tracks more room. Double-click a MIDI clip to open it again.");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    zoomInButton.setTooltip ("Zoom in (Cmd+scroll)");
    zoomOutButton.setTooltip ("Zoom out (Cmd+scroll)");
    zoomInButton.onClick = [this]
    {
        if (scoreMode)
            scoreView.zoomBy (1.2, (float) scoreView.getWidth() / 2.0f);
        else
            zoom (1.4, (float) (viewport.getViewPositionX() + viewport.getViewWidth() / 2));
    };
    zoomOutButton.onClick = [this]
    {
        if (scoreMode)
            scoreView.zoomBy (1.0 / 1.2, (float) scoreView.getWidth() / 2.0f);
        else
            zoom (1.0 / 1.4, (float) (viewport.getViewPositionX() + viewport.getViewWidth() / 2));
    };

    for (auto* b : { &zoomInButton, &zoomOutButton })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }

    // Transpose the whole clip by an octave (arrow keys move just the selected notes).
    for (auto [button, step] : { std::pair (&octaveDownButton, -12), std::pair (&octaveUpButton, 12) })
    {
        button->setWantsKeyboardFocus (false);
        button->setTooltip (juce::String ("Transpose the whole clip one octave ") + (step > 0 ? "up" : "down")
                            + " (Shift+Up/Down moves only the selected notes)");
        button->onClick = [this, semitones = step]
        {
            if (auto* clip = grid.getClip())
                engine.transposeClip (*clip, semitones);
        };
        addAndMakeVisible (button);
    }

    // Select mode: empty space selects instead of drawing, until the button is turned off again.
    selectButton.setClickingTogglesState (true);
    selectButton.setWantsKeyboardFocus (false);
    selectButton.setTooltip ("Select notes: drag on empty space to select with a rectangle instead of drawing. "
                             "Turn it off to draw notes again.");
    selectButton.onClick = [this]
    {
        grid.selectMode = selectButton.getToggleState();
        grid.repaint();
    };
    addAndMakeVisible (selectButton);

    // Scale filter: only the song key's pitches get a row, and the keyboard lights up the chord.
    // It is a setting of the track, so it applies to every clip on it.
    scaleButton.setClickingTogglesState (true);
    scaleButton.setWantsKeyboardFocus (false);
    scaleButton.onClick = [this]
    {
        if (auto* track = selection.getTrack(); track != nullptr && grid.getClip() != nullptr)
        {
            showScaleFilter (scaleButton.getToggleState());
            engine.setScaleFilter (*track, scaleButton.getToggleState());
        }
    };
    addAndMakeVisible (scaleButton);
    updateRows();

    generateButton.setClickingTogglesState (true);
    generateButton.setWantsKeyboardFocus (false);
    generateButton.setTooltip ("Generate chords, melodies, basslines, arpeggios, drums or percussion with a local AI model");
    generateButton.onClick = [this]
    {
        generatePanel.setVisible (generateButton.getToggleState());
        resized();
        repaint();
    };
    addAndMakeVisible (generateButton);

    generatePanel.onClose = [this]
    {
        generateButton.setToggleState (false, juce::dontSendNotification);
        generatePanel.setVisible (false);
        resized();
        repaint();
    };
    addChildComponent (generatePanel);

    keyboard.setTooltip ("Click a key to hear it and select its notes in the clip. Shift-click adds or removes them, "
                         "Option-click (Alt) takes that note in every octave, for example all the Cs.");
    keyboard.onKeyClicked = [this] (int pitch, const juce::ModifierKeys& mods)
    {
        // Focus the grid, so Delete and the arrow keys act on the notes right away.
        if (grid.selectPitch (pitch, mods.isAltDown(), mods.isShiftDown()))
            grid.grabKeyboardFocus();
    };
    addAndMakeVisible (keyboard);

    grid.onZoom = [this] (double factor, float anchorX) { zoom (factor, anchorX); };
    viewport.setViewedComponent (&grid, false);
    viewport.setScrollBarsShown (true, true);
    viewport.onVisibleAreaChanged = [this] (const juce::Rectangle<int>& r)
    {
        keyboard.setScrollOffset (r.getY());
        updateKeyboardChord();
    };
    addAndMakeVisible (viewport);

    selection.addChangeListener (this);
    engine.addChangeListener (this);
    scoreMode = engine.properties.getUserSettings()->getValue ("view.clipEditorMode", "pianoRoll") == "score";
    scoreModeButton.setToggleState (scoreMode, juce::dontSendNotification);
    pianoRollModeButton.setToggleState (! scoreMode, juce::dontSendNotification);
    showSelectedClip();
    updateModeVisibility();
    startTimerHz (30);
}

void PianoRoll::setScoreMode (bool on)
{
    if (on == scoreMode)
        return;

    scoreMode = on;
    engine.properties.getUserSettings()->setValue ("view.clipEditorMode", on ? "score" : "pianoRoll");
    scoreModeButton.setToggleState (on, juce::dontSendNotification);
    pianoRollModeButton.setToggleState (! on, juce::dontSendNotification);
    updateModeVisibility();
    updateTitle();

    if (on)
        scoreView.refresh();

    resized();
    repaint();

    if (onModeChanged)
        onModeChanged();
}

void PianoRoll::updateModeVisibility()
{
    const bool hasClip = grid.getClip() != nullptr;

    // The piano roll's own controls give way to the score's.
    for (juce::Component* c : { (juce::Component*) &snapLabel, (juce::Component*) &snapBox, (juce::Component*) &octaveDownButton,
                                (juce::Component*) &octaveUpButton, (juce::Component*) &selectButton, (juce::Component*) &scaleButton,
                                (juce::Component*) &generateButton, (juce::Component*) &rhythmButton, (juce::Component*) &midiGuideButton })
        c->setVisible (! scoreMode);

    for (juce::Component* c : { (juce::Component*) &clefButton, (juce::Component*) &pdfButton, (juce::Component*) &readMusicButton })
        c->setVisible (scoreMode);

    scoreView.setVisible (scoreMode);
    viewport.setVisible (! scoreMode && hasClip);
    keyboard.setVisible (! scoreMode && hasClip);
    generatePanel.setVisible (! scoreMode && generateButton.getToggleState());
    resized();
}

void PianoRoll::openFromScore (Clip* clip, int note, double beat)
{
    setScoreMode (false);
    auto* track = selection.getTrack();

    if (clip != nullptr && track != nullptr)
        selection.select (track, clip);

    if (auto* midi = grid.getClip())
    {
        int y = viewport.getViewPositionY();

        if (note >= 0 && juce::isPositiveAndBelow (note, (int) midi->notes.size()))
        {
            grid.selectOnly (note);
            y = juce::jmax (0, rows.getNearestRow (midi->notes[(size_t) note].pitch) * PianoRollGrid::rowHeight - viewport.getViewHeight() / 2);
        }

        viewport.setViewPosition (juce::jmax (0, (int) ((beat - midi->startBeat) * grid.pixelsPerBeat) - 48), y);
    }
}

PianoRoll::~PianoRoll()
{
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

void PianoRoll::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, headerHeight - 1, getWidth(), 1);

    if (grid.getClip() == nullptr && ! scoreMode)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (14.0f));
        auto area = getLocalBounds().withTrimmedTop (headerHeight);

        if (generatePanel.isVisible())
            area.removeFromRight (GeneratePanel::preferredWidth);

        g.drawText ("Select a MIDI clip to edit its notes (double-click a MIDI lane to create one).",
                    area, juce::Justification::centred);
    }
}

void PianoRoll::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (8, 4);

    closeButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (12);
    zoomInButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (4);
    zoomOutButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (12);

    // The switch, at the left in both modes.
    pianoRollModeButton.setBounds (header.removeFromLeft (84));
    scoreModeButton.setBounds (header.removeFromLeft (64));
    header.removeFromLeft (12);

    if (scoreMode)
    {
        readMusicButton.setBounds (header.removeFromRight (readMusicButton.getBestWidthForHeight (header.getHeight())));
        header.removeFromRight (8);
        pdfButton.setBounds (header.removeFromRight (150));
        header.removeFromRight (8);
        clefButton.setBounds (header.removeFromRight (64));
        header.removeFromRight (8);
        title.setBounds (header);
        scoreView.setBounds (r);
        return;
    }

    rhythmButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (4);
    snapBox.setBounds (header.removeFromRight (180));
    snapLabel.setBounds (header.removeFromRight (44));
    header.removeFromRight (16);
    generateButton.setBounds (header.removeFromRight (100));
    header.removeFromRight (16);
    octaveUpButton.setBounds (header.removeFromRight (72));
    header.removeFromRight (4);
    octaveDownButton.setBounds (header.removeFromRight (72));
    header.removeFromRight (16);
    selectButton.setBounds (header.removeFromRight (64));
    header.removeFromRight (4);
    scaleButton.setBounds (header.removeFromRight (64));
    header.removeFromRight (16);
    midiGuideButton.setBounds (header.removeFromRight (136));
    header.removeFromRight (8);
    title.setBounds (header);

    if (generatePanel.isVisible())
        generatePanel.setBounds (r.removeFromRight (GeneratePanel::preferredWidth));

    keyboard.setBounds (r.removeFromLeft (keyboardWidth));
    viewport.setBounds (r);

    grid.minimumWidth = viewport.getMaximumVisibleWidth();
    grid.updateSize();
    keyboard.setScrollOffset (viewport.getViewPositionY());
}

void PianoRoll::zoom (double factor, float anchorX)
{
    const double anchorBeat = anchorX / grid.pixelsPerBeat;
    const int anchorOnScreen = (int) anchorX - viewport.getViewPositionX();

    grid.pixelsPerBeat = juce::jlimit (8.0, 600.0, grid.pixelsPerBeat * factor);
    grid.updateSize();
    viewport.setViewPosition ((int) (anchorBeat * grid.pixelsPerBeat) - anchorOnScreen, viewport.getViewPositionY());
    grid.repaint();
}

void PianoRoll::updateOctaveButtons()
{
    auto* clip = grid.getClip();
    octaveDownButton.setEnabled (clip != nullptr && Engine::canTranspose (*clip, -12));
    octaveUpButton.setEnabled (clip != nullptr && Engine::canTranspose (*clip, 12));
}

void PianoRoll::updateTitle()
{
    auto* clip = grid.getClip();
    auto* track = selection.getTrack();

    if (scoreMode)
    {
        title.setText (track != nullptr && track->isMidi() ? track->name + "  -  sheet music" : juce::String(), juce::dontSendNotification);
        return;
    }

    if (clip == nullptr || track == nullptr)
    {
        title.setText ({}, juce::dontSendNotification);
        return;
    }

    auto text = track->name + "  -  " + clip->name;

    if (scaleButton.getToggleState())
    {
        const auto hidden = std::count_if (clip->notes.begin(), clip->notes.end(),
                                           [this] (const MidiNote& n) { return ! rows.isShown (n.pitch); });

        if (hidden > 0)
            text << "   (" << (int) hidden << (hidden == 1 ? " note" : " notes") << " outside " << shownKey.getName() << " hidden)";
    }

    shownSelection = describeSelection();

    if (shownSelection.isNotEmpty())
        text << "   |   " << shownSelection;

    title.setText (text, juce::dontSendNotification);
}

juce::String PianoRoll::describeSelection() const
{
    const auto* clip = grid.getClip();
    const auto& selected = grid.getSelectedNotes();

    if (clip == nullptr || selected.isEmpty())
        return {};

    if (selected.size() == 1 && juce::isPositiveAndBelow (selected[0], (int) clip->notes.size()))
    {
        const auto& note = clip->notes[(size_t) selected[0]];
        return pitchName (note.pitch) + ": " + gen::rhythm::describeLength (note.length) + ", from "
               + gen::rhythm::formatPosition (clip->startBeat + note.start);
    }

    // Several: their shared value, if they have one.
    double length = -1.0;
    bool same = true;

    for (int index : selected)
    {
        if (! juce::isPositiveAndBelow (index, (int) clip->notes.size()))
            continue;

        const double l = clip->notes[(size_t) index].length;
        same = same && (length < 0.0 || std::abs (l - length) < 1.0e-6);
        length = l;
    }

    const auto value = same ? gen::rhythm::nameLength (length) : juce::String();
    return juce::String (selected.size()) + " notes selected" + (value.isNotEmpty() ? ", each a " + value : juce::String());
}

void PianoRoll::showScaleFilter (bool on)
{
    // Keep the pitch in the middle of the view where it was.
    const int centrePitch = rows.getPitch ((viewport.getViewPositionY() + viewport.getViewHeight() / 2) / PianoRollGrid::rowHeight);

    scaleButton.setToggleState (on, juce::dontSendNotification);
    grid.showChords = on;
    updateRows();

    const int y = rows.getNearestRow (centrePitch) * PianoRollGrid::rowHeight - viewport.getViewHeight() / 2;
    viewport.setViewPosition (viewport.getViewPositionX(), juce::jmax (0, y));
    keyboard.setScrollOffset (viewport.getViewPositionY());
    updateKeyboardChord();
    updateTitle();
}

void PianoRoll::updateRows()
{
    shownKey = engine.getKey();

    if (scaleButton.getToggleState())
        rows.showKey (shownKey);
    else
        rows.showAll();

    scaleButton.setTooltip ("Show only the notes of the song key (" + shownKey.getName() + ") in every clip on this track, "
                            "and light up the notes of the chord track's chord on the keyboard. "
                            "Keys outside the key on your MIDI keyboard don't sound or record on this track. "
                            "Turn it off to see every note again. Notes already in clips play as they are.");
    grid.rowsChanged();
    keyboard.repaint();
}

void PianoRoll::updateKeyboardChord()
{
    // The chord under the playhead while it plays through the clip, otherwise the one at the left of the view.
    std::optional<Chord> chord;
    auto* clip = grid.getClip();

    if (clip != nullptr && scaleButton.getToggleState())
    {
        double beat = engine.getPositionBeats();

        if (! engine.isPlaying() || beat < clip->startBeat || beat >= clip->startBeat + clip->lengthBeats)
            beat = clip->startBeat + juce::jlimit (0.0, clip->lengthBeats, viewport.getViewPositionX() / grid.pixelsPerBeat);

        if (auto* c = engine.getChord ((int) std::floor (beat / 4.0 + 1e-6)))
            chord = *c;
    }

    keyboard.setChord (chord);
}

void PianoRoll::showSelectedClip()
{
    auto* clip = selection.getMidiClip();
    auto* track = selection.getTrack();
    const juce::ScopeGuard updateHeader { [this]
    {
        updateOctaveButtons();
        updateTitle();
        updateKeyboardChord();
    } };

    if (engine.getKey() != shownKey)
        updateRows();

    // Follow the scale filter of the clip's track.
    if (const bool filter = clip != nullptr && track->scaleFilter; filter != scaleButton.getToggleState())
        showScaleFilter (filter);

    scaleButton.setEnabled (clip != nullptr);

    if (clip == grid.getClip())
    {
        grid.updateSize();
        grid.repaint();
        return;
    }

    grid.setClip (clip != nullptr ? track : nullptr, clip);
    generatePanel.setTarget (clip != nullptr ? track : nullptr, clip);
    viewport.setVisible (clip != nullptr && ! scoreMode);
    keyboard.setVisible (clip != nullptr && ! scoreMode);

    if (clip != nullptr)
    {
        // Centre the view on the notes it shows, or around C4 when there are none.
        int top = -1, bottom = 128;
        for (auto& n : clip->notes)
        {
            if (rows.isShown (n.pitch))
            {
                top = juce::jmax (top, n.pitch);
                bottom = juce::jmin (bottom, n.pitch);
            }
        }

        const int centrePitch = top < 0 ? 66 : (top + bottom) / 2;
        const int y = rows.getNearestRow (centrePitch) * PianoRollGrid::rowHeight - viewport.getViewHeight() / 2;
        viewport.setViewPosition (0, juce::jmax (0, y));
    }

    repaint();
}

void PianoRoll::changeListenerCallback (juce::ChangeBroadcaster*)
{
    showSelectedClip();
}

void PianoRoll::timerCallback()
{
    if (scoreMode)
    {
        pdfButton.setEnabled (scoreView.hasNotes() && onExportSheetMusic != nullptr);
        readMusicButton.setEnabled (onShowReadingGuide != nullptr);
        clefButton.setEnabled (selection.getTrack() != nullptr && selection.getTrack()->isMidi());
        return;
    }

    grid.updatePlayhead();
    updateKeyboardChord();

    // The selection changes while editing; the title says what is selected.
    if (describeSelection() != shownSelection)
        updateTitle();
}
} // namespace daw
