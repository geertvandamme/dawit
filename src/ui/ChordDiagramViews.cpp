#include "ChordDiagramViews.h"

namespace daw
{
namespace diagrams = gen::diagrams;

namespace
{
    constexpr std::array<int, 6> openStrings { 40, 45, 50, 55, 59, 64 };   // E2 A2 D3 G3 B3 E4

    const juce::Colour rightHandColour = theme::accent;
    const juce::Colour bassColour { 0xffffa94d };
    const juce::Colour heldColour = theme::meterGreen;

    // On white paper: dark text and lines instead of light ones.
    const juce::Colour printedInk { 0xff1d1e22 }, printedInkDim { 0xff6b6f78 };

    bool isBlack (int note)
    {
        const int pc = (note % 12 + 12) % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    std::vector<int> shapePitches (const diagrams::GuitarShape& shape)
    {
        std::vector<int> pitches;
        for (size_t s = 0; s < shape.frets.size() && s < openStrings.size(); ++s)
            if (shape.frets[s] >= 0)
                pitches.push_back (openStrings[s] + shape.frets[s]);

        return pitches;
    }

    /** The chord's spelled note name for each of its pitch classes ("Eb", not "D#", in Cm). */
    std::array<juce::String, 12> spell (const Chord& chord)
    {
        std::array<juce::String, 12> names;
        const auto spelled = juce::StringArray::fromTokens (chord.getNoteNames(), " ", {});
        const auto pitchClasses = chord.getPitchClasses();

        for (int i = 0; i < pitchClasses.size() && i < spelled.size(); ++i)
            names[(size_t) pitchClasses[i]] = spelled[i];

        return names;
    }
}

//==============================================================================
KeyboardDiagram::KeyboardDiagram()
{
    setTooltip ("Blue keys: the right hand. Orange: the left hand's bass note. Click to hear it.");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void KeyboardDiagram::setRange (int lowestNote, int highestNote)
{
    low = lowestNote;
    high = juce::jmax (lowestNote, highestNote);

    while (isBlack (low))
        --low;

    while (isBlack (high))
        ++high;

    repaint();
}

void KeyboardDiagram::setChord (const std::vector<int>& rightHand, int bassNote, int rootPitchClass, const std::vector<int>& held)
{
    notes = rightHand;
    bass = bassNote;
    root = rootPitchClass;
    heldNotes = held;
    repaint();
}

void KeyboardDiagram::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    if (lit)
    {
        g.setColour (theme::accent.withAlpha (0.25f));
        g.fillRoundedRectangle (area, 4.0f);
    }

    area = area.reduced (2.0f);
    const float nameHeight = large ? juce::jmax (16.0f, area.getHeight() * 0.1f) : 16.0f;
    const auto keysArea = showNames ? area.withTrimmedBottom (nameHeight) : area;

    std::vector<int> whites;
    for (int n = low; n <= high; ++n)
        if (! isBlack (n))
            whites.push_back (n);

    if (whites.empty())
        return;

    const float keyWidth = keysArea.getWidth() / (float) whites.size();
    const float blackWidth = keyWidth * 0.62f, blackHeight = keysArea.getHeight() * 0.6f;
    const auto contains = [] (const std::vector<int>& v, int n) { return std::find (v.begin(), v.end(), n) != v.end(); };

    auto keyColour = [&] (int n, juce::Colour normal)
    {
        if (n == bass)
            return bassColour;

        return contains (notes, n) ? rightHandColour : normal;
    };

    // A dot on a played key: filled for the root, ringed green when it stays from the chord before.
    auto drawDot = [&] (int n, float cx, float bottom, float width)
    {
        if (n != bass && ! contains (notes, n))
            return;

        // Large: the same size on white and black keys, growing with them.
        const float r = large ? keyWidth * 0.16f : juce::jlimit (2.0f, 5.0f, width * 0.28f);
        const float line = large ? juce::jmax (1.2f, r * 0.22f) : 1.2f;
        const auto dot = juce::Rectangle<float> (cx - r, bottom - r * 2.0f - (large ? r * 0.7f : 3.0f), r * 2.0f, r * 2.0f);

        if (contains (heldNotes, n))
        {
            g.setColour (heldColour);
            g.drawEllipse (dot.expanded (large ? r * 0.35f : 1.5f), large ? line * 1.4f : 1.6f);
        }

        g.setColour (juce::Colour (0xff15161a));

        if ((n % 12 + 12) % 12 == root)
            g.fillEllipse (dot);
        else
            g.drawEllipse (dot.reduced (line * 0.5f), line);
    };

    for (size_t i = 0; i < whites.size(); ++i)
    {
        const auto key = juce::Rectangle<float> (keysArea.getX() + keyWidth * (float) i, keysArea.getY(), keyWidth, keysArea.getHeight());
        g.setColour (keyColour (whites[i], printed ? juce::Colours::white : juce::Colour (0xffe8e9ec)));
        g.fillRect (key.reduced (0.5f, 0.0f));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (key, 0.6f);
        drawDot (whites[i], key.getCentreX(), key.getBottom(), keyWidth);
    }

    for (int n = low; n <= high; ++n)
    {
        if (! isBlack (n))
            continue;

        const auto whiteIndex = (float) (std::lower_bound (whites.begin(), whites.end(), n) - whites.begin());
        const auto key = juce::Rectangle<float> (keysArea.getX() + keyWidth * whiteIndex - blackWidth * 0.5f, keysArea.getY(), blackWidth, blackHeight);
        g.setColour (keyColour (n, juce::Colour (0xff1d1e22)));
        g.fillRect (key);
        g.setColour (juce::Colours::black);
        g.drawRect (key, 0.6f);
        drawDot (n, key.getCentreX(), key.getBottom(), blackWidth);
    }

    if (! showNames)
        return;

    // The names of the played notes, under their keys (bass first, then the right hand).
    g.setFont (juce::FontOptions (large ? nameHeight * 0.72f : 11.5f));
    const float nameWidth = large ? keyWidth * 1.6f : 32.0f;
    std::vector<int> played { bass };
    played.insert (played.end(), notes.begin(), notes.end());

    for (int n : played)
    {
        if (n < low || n > high)
            continue;

        float cx = 0.0f;
        if (isBlack (n))
            cx = keysArea.getX() + keyWidth * (float) (std::lower_bound (whites.begin(), whites.end(), n) - whites.begin());
        else
            cx = keysArea.getX() + keyWidth * ((float) (std::lower_bound (whites.begin(), whites.end(), n) - whites.begin()) + 0.5f);

        g.setColour (n == bass ? (printed ? bassColour.darker (0.5f) : bassColour) : printed ? printedInk : theme::text);
        g.drawText (getName (n), juce::Rectangle<float> (cx - nameWidth * 0.5f, keysArea.getBottom() + 1.0f, nameWidth, nameHeight - 1.0f), juce::Justification::centred);
    }
}

juce::String KeyboardDiagram::getName (int note) const
{
    const auto& spelled = names[(size_t) ((note % 12 + 12) % 12)];
    return spelled.isNotEmpty() ? spelled : juce::MidiMessage::getMidiNoteName (note, true, false, 4);
}

juce::String KeyboardDiagram::getNoteNamesText() const
{
    juce::StringArray text;
    for (int n : notes)
        text.add (getName (n));

    return text.joinIntoString (" ");
}

void KeyboardDiagram::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onClick != nullptr)
        onClick();
}

//==============================================================================
GuitarDiagram::GuitarDiagram()
{
    setTooltip ("A chord box: strings from the low E (left) to the high E (right), frets downwards. "
                "Numbers are fingers (1 index, 2 middle, 3 ring, 4 little), o an open string, x a string you don't play. "
                "Green rings: fingers that stay where they were. Click to hear it strummed.");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void GuitarDiagram::setShape (const diagrams::GuitarShape& newShape, const diagrams::GuitarShape* previous)
{
    shape = newShape;
    held.assign (shape.frets.size(), false);

    if (previous != nullptr && previous->frets.size() == shape.frets.size())
        for (size_t s = 0; s < shape.frets.size(); ++s)
            held[s] = shape.frets[s] > 0 && previous->frets[s] == shape.frets[s];

    repaint();
}

void GuitarDiagram::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const auto ink = printed ? printedInk : theme::text, inkDim = printed ? printedInkDim : theme::textDim;

    if (lit)
    {
        g.setColour (theme::accent.withAlpha (0.25f));
        g.fillRoundedRectangle (area, 4.0f);
    }

    if (shape.isEmpty())
    {
        g.setColour (inkDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText ("No easy shape", area.toNearestInt(), juce::Justification::centred, 2);
        return;
    }

    const int baseFret = shape.getBaseFret();
    int highestFret = 0;
    for (int f : shape.frets)
        highestFret = juce::jmax (highestFret, f);

    const int numFrets = juce::jmax (4, highestFret - baseFret + 1);
    // Large boxes scale their lines and text from the size the normal box is drawn at.
    const float s = large ? juce::jmax (1.0f, juce::jmin (area.getWidth() / 92.0f, area.getHeight() / 110.0f)) : 1.0f;
    // Room on the left for the fret number, when the box doesn't start at the nut.
    const float labelWidth = s * (baseFret == 1 ? 6.0f : compact ? 14.0f : 18.0f), markHeight = s * (compact ? 11.0f : 14.0f);
    auto grid = area.reduced (4.0f * s, 3.0f * s).withTrimmedLeft (labelWidth).withTrimmedTop (markHeight).withTrimmedRight (labelWidth * 0.4f);
    const float stringGap = grid.getWidth() / 5.0f, fretGap = grid.getHeight() / (float) numFrets;
    auto stringX = [&] (int string) { return grid.getX() + stringGap * (float) string; };

    // Frets, the nut (or the fret the box starts at) and the strings.
    g.setColour (inkDim);
    for (int f = 0; f <= numFrets; ++f)
        g.fillRect (grid.getX(), grid.getY() + fretGap * (float) f - 0.5f * s, grid.getWidth(), s);

    if (baseFret == 1)
    {
        g.setColour (ink);
        g.fillRect (grid.getX() - 0.5f * s, grid.getY() - 3.0f * s, grid.getWidth() + s, 3.5f * s);
    }
    else
    {
        g.setColour (ink);
        g.setFont (juce::FontOptions (s * (compact ? 10.0f : 12.0f)));
        g.drawText (juce::String (baseFret), juce::Rectangle<float> (area.getX(), grid.getY(), labelWidth + 2.0f * s, fretGap), juce::Justification::centred);
    }

    g.setColour (ink.withAlpha (0.8f));
    for (int string = 0; string < 6; ++string)
        g.fillRect (stringX (string) - 0.5f * s, grid.getY(), s * (1.0f + (float) (5 - string) * 0.15f), grid.getHeight());

    const float r = juce::jmin (stringGap * 0.46f, fretGap * 0.36f);
    auto fretY = [&] (int fret) { return grid.getY() + fretGap * ((float) (fret - baseFret) + 0.5f); };

    // Open and unplayed strings above the nut.
    g.setFont (juce::FontOptions (s * (compact ? 10.0f : 12.5f)));
    for (int string = 0; string < 6; ++string)
    {
        const int f = shape.frets[(size_t) string];
        if (f > 0)
            continue;

        g.setColour (f == 0 ? ink : inkDim);
        g.drawText (f == 0 ? "o" : "x", juce::Rectangle<float> (stringX (string) - 7.0f * s, area.getY() + s, 14.0f * s, markHeight), juce::Justification::centred);
    }

    // The barre, then the fingers.
    if (shape.hasBarre())
    {
        const auto bar = juce::Rectangle<float> (stringX (shape.barreFrom) - r, fretY (shape.barreFret) - r,
                                                 stringX (shape.barreTo) - stringX (shape.barreFrom) + r * 2.0f, r * 2.0f);
        g.setColour (rightHandColour);
        g.fillRoundedRectangle (bar, r);
    }

    for (int string = 0; string < 6; ++string)
    {
        const int f = shape.frets[(size_t) string];
        if (f <= 0)
            continue;

        const auto dot = juce::Rectangle<float> (stringX (string) - r, fretY (f) - r, r * 2.0f, r * 2.0f);
        const bool inBarre = shape.hasBarre() && f == shape.barreFret && string >= shape.barreFrom && string <= shape.barreTo;

        if (! inBarre)
        {
            g.setColour (rightHandColour);
            g.fillEllipse (dot);
        }

        if (held[(size_t) string])
        {
            g.setColour (heldColour);
            g.drawEllipse (dot.expanded (2.0f * s), 2.0f * s);
        }

        // In a barre, the finger number goes on its lowest string only.
        if (r >= 4.5f && shape.fingers[(size_t) string] > 0 && ! (inBarre && string != shape.barreFrom))
        {
            g.setColour (juce::Colour (0xff15161a));
            g.setFont (juce::FontOptions (r * 1.45f, juce::Font::bold));
            g.drawText (juce::String (shape.fingers[(size_t) string]), dot, juce::Justification::centred);
        }
    }
}

void GuitarDiagram::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onClick != nullptr)
        onClick();
}

//==============================================================================
ChordDiagramPanel::ChordDiagramPanel()
{
    addChildComponent (keyboard);
    keyboard.onClick = [this]
    {
        if (onPlay != nullptr)
        {
            auto notes = diagrams::pianoInversion (chord, inversion);
            notes.insert (notes.begin(), diagrams::pianoBass (chord));
            onPlay (notes, false);
        }
    };

    keyboardCaption.setFont (juce::FontOptions (12.5f));
    keyboardCaption.setColour (juce::Label::textColourId, theme::textDim);
    addChildComponent (keyboardCaption);

    for (auto* button : { &pianoButton, &guitarButton })
    {
        button->setClickingTogglesState (true);
        button->setRadioGroupId (0x5ca1e);
        addChildComponent (button);
    }

    pianoButton.setToggleState (true, juce::dontSendNotification);
    pianoButton.setTooltip ("Each chord on the piano, in the inversion nearest the chord before");
    guitarButton.setTooltip ("Each chord on the guitar, in the shape nearest the one before");
    pianoButton.onClick = guitarButton.onClick = [this]
    {
        showGuitar = guitarButton.getToggleState();
        updateInstrument();
    };
}

void ChordDiagramPanel::clear()
{
    mode = Mode::none;
    keyboard.setVisible (false);
    keyboardCaption.setVisible (false);
    inversionButtons.clear();
    guitarBoxes.clear();
    guitarCaptions.clear();
    keyboards.clear();
    boxes.clear();
    names.clear();
    pianoButton.setVisible (false);
    guitarButton.setVisible (false);
    repaint();
}

void ChordDiagramPanel::showChord (const Chord& newChord, const Chord* previous)
{
    clear();
    mode = Mode::chord;
    chord = newChord;

    // The keyboard covers every inversion and the bass, so it doesn't move when another is picked.
    int lowest = diagrams::pianoBass (chord), highest = lowest;
    for (int i = 0; i < diagrams::getNumInversions (chord); ++i)
        for (int n : diagrams::pianoInversion (chord, i))
            highest = juce::jmax (highest, n);

    keyboard.setRange (lowest, highest);
    keyboard.setShowNames (true);
    keyboard.setVisible (true);
    keyboardCaption.setVisible (true);

    smoothInversion = previous != nullptr ? diagrams::findSmoothInversion (chord, diagrams::pianoInversion (*previous, 0)) : -1;

    for (int i = 0; i < diagrams::getNumInversions (chord) && i < 4; ++i)
    {
        static const char* shortNames[] = { "Root", "1st", "2nd", "3rd" };
        auto* button = inversionButtons.add (new juce::TextButton (juce::String (shortNames[i]) + (i == smoothInversion ? juce::String (juce::CharPointer_UTF8 (" \xe2\x98\x85")) : juce::String())));
        button->setClickingTogglesState (true);
        button->setRadioGroupId (0x1e7);
        button->setTooltip (diagrams::getInversionName (i) + (i == smoothInversion ? ": the one that moves least from " + previous->getName() : juce::String()));
        button->onClick = [this, i] { showInversion (i); if (keyboard.onClick) keyboard.onClick(); };
        addAndMakeVisible (button);
    }

    showInversion (smoothInversion >= 0 ? smoothInversion : 0);

    shapes = diagrams::getGuitarShapes (chord, 3);

    for (auto& shape : shapes)
    {
        auto* box = guitarBoxes.add (new GuitarDiagram());
        box->setShape (shape);
        box->onClick = [this, shape] { if (onPlay != nullptr) onPlay (shapePitches (shape), true); };
        addAndMakeVisible (box);

        auto* caption = guitarCaptions.add (new juce::Label ({}, shape.getTab() + "\n" + diagrams::getDifficultyName (shape.difficulty)));
        caption->setFont (juce::FontOptions (12.0f));
        caption->setJustificationType (juce::Justification::centredTop);
        caption->setColour (juce::Label::textColourId, shape.difficulty == diagrams::Difficulty::easy ? theme::meterGreen
                                                       : shape.difficulty == diagrams::Difficulty::medium ? bassColour : theme::meterRed);
        caption->setTooltip (diagrams::describeGuitarShape (shape));
        addAndMakeVisible (caption);
    }

    resized();
}

void ChordDiagramPanel::showInversion (int newInversion)
{
    inversion = newInversion;

    for (int i = 0; i < inversionButtons.size(); ++i)
        inversionButtons[i]->setToggleState (i == inversion, juce::dontSendNotification);

    const auto notes = diagrams::pianoInversion (chord, inversion);
    keyboard.setChord (notes, diagrams::pianoBass (chord), chord.root);
    keyboard.setNames (spell (chord));

    keyboardCaption.setText (diagrams::getInversionName (inversion) + ": " + keyboard.getNoteNamesText()
                                 + (inversion == smoothInversion ? juce::String (juce::CharPointer_UTF8 ("  \xe2\x98\x85 smoothest from the chord before")) : juce::String()),
                             juce::dontSendNotification);
}

void ChordDiagramPanel::showProgression (const std::vector<Chord>& newChords)
{
    clear();

    if (newChords.empty())
        return;

    mode = Mode::progression;
    chords = newChords;
    voicings = diagrams::voiceProgression (chords);
    progressionShapes = diagrams::shapeProgression (chords);

    pianoButton.setVisible (true);
    guitarButton.setVisible (true);

    int lowest = 127, highest = 0;
    for (auto& v : voicings)
        for (int n : v)
        {
            lowest = juce::jmin (lowest, n);
            highest = juce::jmax (highest, n);
        }

    for (size_t i = 0; i < chords.size(); ++i)
    {
        auto* name = names.add (new juce::Label ({}, chords[i].getName()));
        name->setFont (juce::FontOptions (13.0f, juce::Font::bold));
        name->setJustificationType (juce::Justification::centred);
        name->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (name);

        auto* keys = keyboards.add (new KeyboardDiagram());
        keys->setRange (lowest, highest);
        keys->setShowNames (false);
        keys->setChord (voicings[i], -1, chords[i].root, i > 0 ? voicings[i - 1] : std::vector<int>());
        keys->onClick = [this, i] { bool strum = false; if (onPlay != nullptr) onPlay (getProgressionNotes ((int) i, strum), strum); };
        addChildComponent (keys);

        auto* box = boxes.add (new GuitarDiagram());
        box->setCompact (true);
        box->setShape (progressionShapes[i], i > 0 ? &progressionShapes[i - 1] : nullptr);
        box->onClick = keys->onClick;
        addChildComponent (box);
    }

    updateInstrument();
}

void ChordDiagramPanel::updateInstrument()
{
    for (auto* k : keyboards)
        k->setVisible (! showGuitar);

    for (auto* b : boxes)
        b->setVisible (showGuitar);

    resized();

    if (onLayoutChanged != nullptr)
        onLayoutChanged();
}

void ChordDiagramPanel::setPlayingIndex (int index)
{
    for (int i = 0; i < keyboards.size(); ++i)
        keyboards[i]->setLit (i == index);

    for (int i = 0; i < boxes.size(); ++i)
        boxes[i]->setLit (i == index);
}

std::vector<int> ChordDiagramPanel::getProgressionNotes (int index, bool& strum) const
{
    strum = false;

    if (mode != Mode::progression || index < 0 || index >= (int) chords.size())
        return {};

    if (showGuitar && ! progressionShapes[(size_t) index].isEmpty())
    {
        strum = true;
        return shapePitches (progressionShapes[(size_t) index]);
    }

    auto notes = voicings[(size_t) index];
    notes.insert (notes.begin(), diagrams::pianoBass (chords[(size_t) index]));
    return notes;
}

int ChordDiagramPanel::getIdealHeight (int) const
{
    if (mode == Mode::chord)
        return 168;

    if (mode == Mode::progression)
    {
        const int rows = ((int) chords.size() + perRow - 1) / perRow;
        return 30 + rows * (18 + (showGuitar ? 100 : 62) + 6);
    }

    return 0;
}

void ChordDiagramPanel::resized()
{
    auto r = getLocalBounds();

    if (mode == Mode::chord)
    {
        auto left = r.removeFromLeft (r.getWidth() * 52 / 100);
        r.removeFromLeft (14);

        auto buttons = left.removeFromBottom (24);
        left.removeFromBottom (4);
        keyboardCaption.setBounds (left.removeFromBottom (18));
        keyboard.setBounds (left);

        const int buttonWidth = juce::jmin (64, buttons.getWidth() / juce::jmax (1, inversionButtons.size()));
        for (auto* b : inversionButtons)
        {
            b->setBounds (buttons.removeFromLeft (buttonWidth - 4));
            buttons.removeFromLeft (4);
        }

        const int boxWidth = juce::jmin (110, r.getWidth() / juce::jmax (1, guitarBoxes.size()));
        for (int i = 0; i < guitarBoxes.size(); ++i)
        {
            auto column = r.removeFromLeft (boxWidth);
            guitarCaptions[i]->setBounds (column.removeFromBottom (32));
            guitarBoxes[i]->setBounds (column.reduced (4, 0));
        }

        return;
    }

    if (mode != Mode::progression)
        return;

    auto top = r.removeFromTop (24);
    pianoButton.setBounds (top.removeFromLeft (70));
    top.removeFromLeft (4);
    guitarButton.setBounds (top.removeFromLeft (70));
    r.removeFromTop (6);

    const int columns = juce::jmin (perRow, (int) chords.size());
    const int tileWidth = juce::jmin (150, r.getWidth() / juce::jmax (1, columns));
    const int diagramHeight = showGuitar ? 100 : 62;

    for (int i = 0; i < (int) chords.size(); ++i)
    {
        const int x = r.getX() + (i % perRow) * tileWidth, y = r.getY() + (i / perRow) * (18 + diagramHeight + 6);
        names[i]->setBounds (x, y, tileWidth, 18);
        const auto diagram = juce::Rectangle<int> (x, y + 18, tileWidth, diagramHeight).reduced (3, 0);
        keyboards[i]->setBounds (diagram);
        boxes[i]->setBounds (diagram.withSizeKeepingCentre (juce::jmin (diagram.getWidth(), 92), diagram.getHeight()));
    }
}

void ChordDiagramPanel::paint (juce::Graphics& g)
{
    if (mode != Mode::progression)
        return;

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (showGuitar ? "Each shape near the one before. Green rings: fingers that stay. Click to hear it."
                           : "Inversions nearest the chord before. Green rings: keys that stay down. Click to hear it.",
                getLocalBounds().removeFromTop (24).withTrimmedLeft (156), juce::Justification::centredLeft, true);
}
} // namespace daw
