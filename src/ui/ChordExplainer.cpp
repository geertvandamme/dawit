#include "ChordExplainer.h"
#include "ChordSheetExport.h"
#include "generate/ChordDiagrams.h"
#include "generate/PlayAlong.h"

namespace daw
{
namespace guide = gen::guide;
namespace harmony = gen::harmony;

namespace
{
    juce::Colour familyColour (guide::Family family)
    {
        switch (family)
        {
            case guide::Family::home:    return juce::Colour (0xff5fd38d);
            case guide::Family::away:    return juce::Colour (0xff4fa3ff);
            case guide::Family::tension: return juce::Colour (0xffffa94d);
            case guide::Family::outside: return juce::Colour (0xffb98cff);
        }

        return theme::textDim;
    }

    /** Bass note in octave 3 and the chord in close position just above middle C. */
    std::vector<int> voice (const Chord& chord)
    {
        std::vector<int> notes { 48 + chord.root };

        for (int pitchClass : chord.getPitchClasses())
            notes.push_back (60 + pitchClass);

        return notes;
    }
}

/** A chord, coloured by its family: numeral, name and (when large) the family's name. */
class ChordExplainer::ChordTile final : public juce::Button
{
public:
    ChordTile (const MusicalKey& tileKey, const Chord& c, bool isCompact)
        : juce::Button (c.getName()), chord (c), numeral (guide::getNumeral (tileKey, c)),
          family (guide::getFamily (tileKey, c)), compact (isCompact)
    {
        setTooltip (chord.getName() + " (" + numeral + "), " + guide::getFamilyName (family).toLowerCase()
                    + ". Click to hear it and read about it.");
    }

    const Chord chord;

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.5f);
        const auto colour = familyColour (family);
        const bool lit = getToggleState() || down;

        g.setColour (colour.withAlpha (lit ? 0.85f : highlighted ? 0.32f : 0.18f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (colour.withAlpha (lit ? 1.0f : 0.7f));
        g.drawRoundedRectangle (r, 6.0f, lit ? 2.0f : 1.0f);

        const auto textColour = lit ? juce::Colour (0xff15161a) : theme::text;
        auto area = r.reduced (4.0f, compact ? 3.0f : 6.0f);

        if (compact)
        {
            g.setColour (textColour);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawFittedText (numeral, area.removeFromTop (area.getHeight() * 0.5f).toNearestInt(), juce::Justification::centred, 1, 0.6f);
            g.setFont (juce::FontOptions (12.0f));
            g.drawFittedText (chord.getName(), area.toNearestInt(), juce::Justification::centred, 1, 0.6f);
            return;
        }

        g.setColour (textColour);
        g.setFont (juce::FontOptions (21.0f, juce::Font::bold));
        g.drawFittedText (numeral, area.removeFromTop (26.0f).toNearestInt(), juce::Justification::centred, 1, 0.6f);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText (chord.getName(), area.removeFromTop (20.0f).toNearestInt(), juce::Justification::centred, 1, 0.6f);
        g.setColour (lit ? textColour : colour);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText (guide::getFamilyName (family), area.toNearestInt(), juce::Justification::centred, 1, 0.6f);
    }

private:
    const juce::String numeral;
    const guide::Family family;
    const bool compact;
};

//==============================================================================
/** The circle of fifths: the major keys round the outside, their relative minors inside and the key signatures
    around it. The chords of the key are coloured by family and the home chord is outlined. Click a key to hear
    its chord, double-click it to explore that key. */
class ChordExplainer::CircleView final : public juce::Component, public juce::SettableTooltipClient
{
public:
    CircleView()
    {
        setTooltip ("Click a key to hear its chord and read about it. Double-click it to explore that key.");
    }

    std::function<void (const Chord&)> onChordClicked;
    std::function<void (const MusicalKey&)> onKeyPicked;

    void setKey (const MusicalKey& newKey)
    {
        key = newKey;
        repaint();
    }

    /** Lights up a chord while it plays; nullptr turns it off. */
    void setLit (const Chord* chord)
    {
        lit = chord != nullptr ? std::optional<Chord> (*chord) : std::nullopt;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto centre = getLocalBounds().toFloat().getCentre();
        const float outer = getOuterRadius(), middle = outer * middleProportion, inner = outer * innerProportion;
        const float step = juce::MathConstants<float>::twoPi / 12.0f;
        const auto home = harmony::triad (key, 1);

        for (int ring = 0; ring < 2; ++ring)
        {
            const bool minor = ring == 1;
            const float r1 = minor ? middle : outer, r0 = minor ? inner : middle;

            for (int position = 0; position < 12; ++position)
            {
                const auto chord = getChord (position, minor);
                const int degree = harmony::findDegree (key, chord);
                const bool inKey = degree > 0 && harmony::triad (key, degree) == chord;
                const bool isLit = lit.has_value() && *lit == chord;
                const bool hovered = hoveredSegment == position + ring * 12;
                const auto colour = familyColour (guide::getFamily (key, chord));

                juce::Path segment;
                segment.addPieSegment (centre.x - r1, centre.y - r1, r1 * 2.0f, r1 * 2.0f,
                                       ((float) position - 0.5f) * step, ((float) position + 0.5f) * step, r0 / r1);

                g.setColour (inKey ? colour.withAlpha (isLit ? 0.9f : hovered ? 0.5f : 0.32f)
                                   : (isLit ? theme::text.withAlpha (0.5f) : hovered ? theme::panelLight.brighter (0.25f)
                                                                                       : theme::panelLight));
                g.fillPath (segment);
                g.setColour (theme::panel);
                g.strokePath (segment, juce::PathStrokeType (1.5f));

                // The label, with the numeral below it for the key's chords.
                const auto labelCentre = pointOnCircle (centre, (r0 + r1) * 0.5f, (float) position * step);
                const auto textColour = isLit ? juce::Colour (0xff15161a) : inKey ? theme::text : theme::textDim;
                const float size = minor ? 13.0f : 15.5f;
                auto box = juce::Rectangle<float> (48.0f, inKey ? 32.0f : 18.0f).withCentre (labelCentre);

                g.setColour (textColour);
                g.setFont (juce::FontOptions (size, juce::Font::bold));
                g.drawFittedText (guide::getCircleLabel (position, minor), box.removeFromTop (18.0f).toNearestInt(),
                                  juce::Justification::centred, 1, 0.7f);

                if (inKey)
                {
                    g.setColour (isLit ? textColour : colour);
                    g.setFont (juce::FontOptions (11.5f));
                    g.drawFittedText (guide::getNumeral (key, chord), box.toNearestInt(), juce::Justification::centred, 1, 0.7f);
                }

                if (chord == home)
                {
                    g.setColour (theme::text);
                    g.strokePath (segment, juce::PathStrokeType (2.5f));
                }
            }
        }

        // Key signatures round the outside.
        g.setFont (juce::FontOptions (11.5f));

        for (int position = 0; position < 12; ++position)
        {
            const int count = guide::getKeySignature (position).size();
            const auto text = count == 0 ? juce::String ("none")
                                         : position == 6 ? juce::String (count) + sharp() + "/" + juce::String (count) + flat()
                                                         : juce::String (count) + (position < 6 ? sharp() : flat());
            const auto at = pointOnCircle (centre, outer + 13.0f, (float) position * step);

            g.setColour (position == guide::getCirclePosition (key) ? theme::text : theme::textDim);
            g.drawText (text, juce::Rectangle<float> (50.0f, 16.0f).withCentre (at), juce::Justification::centred);
        }

        // The key's name and signature in the middle.
        auto middleBox = juce::Rectangle<float> (inner * 1.7f, inner * 1.1f).withCentre (centre);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawFittedText (key.getName(), middleBox.removeFromTop (middleBox.getHeight() * 0.5f).toNearestInt(),
                          juce::Justification::centredBottom, 2, 0.7f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawFittedText (guide::describeKeySignature (guide::getCirclePosition (key)), middleBox.toNearestInt(),
                          juce::Justification::centredTop, 2, 0.7f);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int segment = findSegment (e.position);
        setMouseCursor (segment >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);

        if (segment != hoveredSegment)
        {
            hoveredSegment = segment;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hoveredSegment = -1;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (const int segment = findSegment (e.position); segment >= 0 && ! e.mouseWasDraggedSinceMouseDown() && onChordClicked)
            onChordClicked (getChord (segment % 12, segment >= 12));
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (const int segment = findSegment (e.position); segment >= 0 && onKeyPicked)
        {
            const auto chord = getChord (segment % 12, segment >= 12);
            onKeyPicked ({ chord.root, segment >= 12 ? MusicalKey::Scale::minor : MusicalKey::Scale::major });
        }
    }

private:
    static constexpr float middleProportion = 0.68f, innerProportion = 0.38f;

    static juce::String sharp() { return juce::String::fromUTF8 ("\xe2\x99\xaf"); }
    static juce::String flat()  { return juce::String::fromUTF8 ("\xe2\x99\xad"); }

    static juce::Point<float> pointOnCircle (juce::Point<float> centre, float radius, float angle)
    {
        // Angles run clockwise from 12 o'clock.
        return { centre.x + radius * std::sin (angle), centre.y - radius * std::cos (angle) };
    }

    /** The major chord on the outer ring, or the minor chord on the inner ring, at a place. */
    static Chord getChord (int position, bool minor)
    {
        const int tonic = guide::getCircleTonic (position);
        return minor ? Chord { (tonic + 9) % 12, { 0, 3, 7 } } : Chord { tonic, { 0, 4, 7 } };
    }

    float getOuterRadius() const
    {
        return (float) juce::jmin (getWidth(), getHeight()) * 0.5f - 26.0f;
    }

    /** 0-11: a major key at that place, 12-23: the minor key at place - 12, -1: none. */
    int findSegment (juce::Point<float> p) const
    {
        const auto offset = p - getLocalBounds().toFloat().getCentre();
        const float distance = offset.getDistanceFromOrigin(), outer = getOuterRadius();

        if (distance > outer || distance < outer * innerProportion)
            return -1;

        float angle = std::atan2 (offset.x, -offset.y);
        if (angle < 0.0f)
            angle += juce::MathConstants<float>::twoPi;

        const int position = juce::roundToInt (angle / (juce::MathConstants<float>::twoPi / 12.0f)) % 12;
        return distance < outer * middleProportion ? position + 12 : position;
    }

    MusicalKey key;
    std::optional<Chord> lit;
    int hoveredSegment = -1;
};

//==============================================================================
ChordExplainer::ChordExplainer (Engine& e, int bar) : engine (e), startBar (juce::jmax (0, bar)), key (e.getKey())
{
    title.setText ("Chord progressions", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    keyLabel.setColour (juce::Label::textColourId, theme::textDim);
    keyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (keyLabel);

    for (int pc = 0; pc < 12; ++pc)
        tonicBox.addItem (MusicalKey::getTonicName (pc), pc + 1);

    for (int s = 0; s < MusicalKey::numScales; ++s)
        scaleBox.addItem (MusicalKey::getScaleName ((MusicalKey::Scale) s), s + 1);

    auto keyPicked = [this]
    {
        const MusicalKey picked { tonicBox.getSelectedId() - 1, (MusicalKey::Scale) (scaleBox.getSelectedId() - 1) };
        followSongKey = picked == engine.getKey();
        setKey (picked);
    };

    tonicBox.onChange = keyPicked;
    scaleBox.onChange = keyPicked;
    tonicBox.setTooltip ("Explore another key. The song itself doesn't change.");
    scaleBox.setTooltip (tonicBox.getTooltip());
    addAndMakeVisible (tonicBox);
    addAndMakeVisible (scaleBox);

    songKeyButton.setTooltip ("Go back to the song's key");
    songKeyButton.onClick = [this]
    {
        followSongKey = true;
        setKey (engine.getKey());
    };
    addAndMakeVisible (songKeyButton);

    circle = std::make_unique<CircleView>();
    circle->onChordClicked = [this] (const Chord& chord) { showChord (chord); playChord (chord); };
    circle->onKeyPicked = [this] (const MusicalKey& picked)
    {
        followSongKey = picked == engine.getKey();
        setKey (picked);
    };
    addChildComponent (*circle);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    playButton.setTooltip ("Hear the chords one after the other, on the selected MIDI track");
    playButton.onClick = [this] { playing.empty() ? play (getShownChords()) : stop(); };
    addAndMakeVisible (playButton);

    slowToggle.setTooltip ("Practise: Play gives each chord a whole bar, so there is time to change, and lights its diagram");
    addAndMakeVisible (slowToggle);

    diagramPanel.onPlay = [this] (const std::vector<int>& notes, bool strum)
    {
        stop();
        playNotes (notes, strum);
    };
    diagramPanel.onLayoutChanged = [this] { resized(); };
    addAndMakeVisible (diagramPanel);

    putButton.setTooltip ("Write this progression on the chord track, one chord per bar, starting at the bar you clicked");
    putButton.onClick = [this] { putOnChordTrack(); };
    addAndMakeVisible (putButton);

    playAlongButton.setTooltip ("Open the play-along window: the chord playing now, large on piano and guitar, with the next one beside it (Cmd+4)");
    playAlongButton.onClick = [this] { if (onPlayAlong != nullptr) onPlayAlong(); };
    addChildComponent (playAlongButton);

    sheetButton.setTooltip ("Save this key's chords, sevenths and progressions as a PDF to print, for piano or guitar");
    sheetButton.onClick = [this] { chordsheet::showDialog (engine, key); };
    addAndMakeVisible (sheetButton);

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

    setKey (key);
    list.selectRow (getChordTrackChords().empty() ? introRow : chordTrackRow);
    updateHint();
    setSize (width, height);
}

ChordExplainer::~ChordExplainer()
{
    engine.removeChangeListener (this);
}

void ChordExplainer::setPrintChordShapes (bool on)
{
    printChordShapes = on;
}

std::vector<Chord> ChordExplainer::getPrintChords() const
{
    std::vector<Chord> chords;

    for (auto& [bar, chord] : engine.getChords())
        if (std::find (chords.begin(), chords.end(), chord) == chords.end())
            chords.push_back (chord);

    if (chords.empty())
        for (int degree = 1; degree <= 7; ++degree)
            chords.push_back (harmony::triad (key, degree));

    return chords;
}

juce::String ChordExplainer::getPrintSource() const
{
    const int n = (int) getChordTrackChords().size();

    if (printChordShapes)
        return engine.getChords().empty() ? "the chords of " + key.getName() + " (the song has no chord track)" : juce::String ("the chords of the song's chord track");

    return key.getName() + (n > 0 ? ", with the song's chord track (" + juce::String (n) + (n == 1 ? " chord)" : " chords)") : juce::String (", with no chord track in the song"));
}

int ChordExplainer::getNumPrintTopics() const
{
    return printChordShapes ? (int) getPrintChords().size() : firstProgressionRow + (int) progressions.size();
}

std::vector<juce::Rectangle<int>> ChordExplainer::getPrintParts() const
{
    std::vector<juce::Rectangle<int>> parts;

    auto unionOf = [] (const juce::OwnedArray<ChordTile>& tiles)
    {
        juce::Rectangle<int> area;

        for (auto* tile : tiles)
            area = area.isEmpty() ? tile->getBounds() : area.getUnion (tile->getBounds());

        return area;
    };

    if (! printChordShapes)
    {
        if (! keyTiles.isEmpty())
            parts.push_back (unionOf (keyTiles).getUnion (legendArea));

        if (! rowTiles.isEmpty())
            parts.push_back (unionOf (rowTiles));
    }

    if (diagramPanel.isShowingSomething() && ! diagramPanel.getBounds().isEmpty())
        parts.push_back (diagramPanel.getBounds());

    if (! printChordShapes && circle->isVisible())
        parts.push_back (circle->getBounds());

    return parts;
}

booklet::Topic ChordExplainer::showPrintTopic (int index)
{
    if (rowBeforePrinting < 0)
        rowBeforePrinting = juce::jmax (0, list.getSelectedRow());

    booklet::Topic topic;

    if (printChordShapes)
    {
        const auto chords = getPrintChords();
        const auto chord = chords[(size_t) juce::jlimit (0, (int) chords.size() - 1, index)];
        showChord (chord);
        topic.name = "How to play " + chord.getName();
        topic.summary = "On piano and guitar";
    }
    else
    {
        index = juce::jlimit (0, getNumRows() - 1, index);
        list.selectRow (index);
        showRow (index);   // also when it was already selected
        std::tie (topic.name, topic.summary) = getRowText (index);
    }

    topic.text = explanation.getText();
    juce::Rectangle<int> size;
    int gaps = 0;

    for (auto& part : getPrintParts())
    {
        size.setWidth (juce::jmax (size.getWidth(), part.getWidth()));
        size.setHeight (size.getHeight() + part.getHeight() + (gaps++ > 0 ? 10 : 0));
    }

    topic.pictureSize = { size.getWidth(), size.getHeight() };
    return topic;
}

juce::Image ChordExplainer::paintPrintPicture (float scale)
{
    const auto parts = getPrintParts();
    int totalWidth = 0, totalHeight = 0;

    for (auto& part : parts)
    {
        totalWidth = juce::jmax (totalWidth, part.getWidth());
        totalHeight += part.getHeight() + (totalHeight > 0 ? 10 : 0);
    }

    juce::Image image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) totalWidth * scale)), juce::jmax (1, juce::roundToInt ((float) totalHeight * scale)),
                       true, juce::SoftwareImageType());
    juce::Graphics g (image);
    g.fillAll (theme::panel);
    float y = 0.0f;

    for (auto& part : parts)
    {
        const auto snapshot = createComponentSnapshot (part, true, scale);
        g.drawImageAt (snapshot, 0, juce::roundToInt (y));
        y += (float) snapshot.getHeight() + 10.0f * scale;
    }

    return image;
}

void ChordExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
    {
        list.selectRow (rowBeforePrinting);
        showRow (rowBeforePrinting);
    }

    rowBeforePrinting = -1;
}

void ChordExplainer::setStartBar (int bar)
{
    startBar = juce::jmax (0, bar);
    putButton.setButtonText ("Put on chord track at bar " + juce::String (startBar + 1));
}

//==============================================================================
void ChordExplainer::setKey (const MusicalKey& newKey)
{
    stop();
    key = newKey;
    progressions = guide::getCommonProgressions (key);
    circle->setKey (key);
    updateKeyControls();
    rebuildTiles();
    list.updateContent();
    list.repaint();
    showRow (juce::jmax (0, list.getSelectedRow()));
}

void ChordExplainer::updateKeyControls()
{
    tonicBox.setSelectedId (key.tonic + 1, juce::dontSendNotification);
    scaleBox.setSelectedId ((int) key.scale + 1, juce::dontSendNotification);
    songKeyButton.setButtonText (key == engine.getKey() ? "This is the song key" : "Song key: " + engine.getKey().getName());
    songKeyButton.setEnabled (key != engine.getKey());
}

void ChordExplainer::rebuildTiles()
{
    keyTiles.clear();

    std::vector<Chord> chords;
    for (int degree = 1; degree <= 7; ++degree)
        chords.push_back (harmony::triad (key, degree));

    // Minor songs usually make the five chord major, which pulls home harder.
    if (const auto dominant = harmony::getBorrowedDominant (key))
        chords.push_back (*dominant);

    for (auto& chord : chords)
    {
        auto* tile = keyTiles.add (new ChordTile (key, chord, false));
        tile->onClick = [this, chord] { showChord (chord); playChord (chord); };
        addAndMakeVisible (tile);
    }

    resized();
}

int ChordExplainer::getNumRows()
{
    return firstProgressionRow + (int) progressions.size();
}

std::pair<juce::String, juce::String> ChordExplainer::getRowText (int row) const
{
    juce::String name, summary;

    if (row == introRow)
    {
        name = "How chords work";
        summary = "Numerals and the three families";
    }
    else if (row == circleRow)
    {
        name = "The circle of fifths";
        summary = "How keys and their chords are related";
    }
    else if (row == playAlongRow)
    {
        name = "Playing along";
        summary = "Tips for playing the song's chords";
    }
    else if (row == chordTrackRow)
    {
        const auto chords = getChordTrackChords();
        name = "Your chord track";
        summary = chords.empty() ? "Empty so far" : guide::findProgressionName (key, chords);

        if (summary.isEmpty())
        {
            juce::StringArray names;
            for (auto& c : chords)
                if (names.isEmpty() || names[names.size() - 1] != c.getName())
                    names.add (c.getName());

            summary = names.joinIntoString (" ");
        }
    }
    else if (row - firstProgressionRow < (int) progressions.size())
    {
        const auto& p = progressions[(size_t) (row - firstProgressionRow)];
        name = p.name;
        summary = p.summary;
    }

    return { name, summary };
}

void ChordExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
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

void ChordExplainer::selectedRowsChanged (int row)
{
    if (row >= 0)
        showRow (row);
}

void ChordExplainer::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    // Clicking the selected row again brings its explanation back after looking at a chord.
    if (row == list.getSelectedRow())
        showRow (row);
}

void ChordExplainer::showRow (int row)
{
    stop();
    rowTiles.clear();

    std::vector<Chord> chords;

    if (row == introRow)
        explanation.setText (guide::getIntroduction (key), false);
    else if (row == circleRow)
    {
        explanation.setText (guide::explainCircleOfFifths (key), false);
        chords = guide::getCircleProgression (key);
    }
    else if (row == chordTrackRow)
    {
        explanation.setText (guide::explainChordTrack (key, getChordTrackChords()), false);
        chords = getShownChords();
    }
    else if (row == playAlongRow)
    {
        explanation.setText (gen::playalong::explainPlayingAlong (gen::playalong::getChanges (engine.getChords()), engine.getTempo()), false);
        chords = getShownChords();
    }
    else if (row - firstProgressionRow < (int) progressions.size())
    {
        const auto& progression = progressions[(size_t) (row - firstProgressionRow)];
        explanation.setText (guide::explainProgression (key, progression), false);
        chords = progression.getChords (key);
    }

    explanation.moveCaretToTop (false);

    for (size_t i = 0; i < chords.size(); ++i)
    {
        const auto chord = chords[i];
        const auto previous = i > 0 ? std::optional<Chord> (chords[i - 1]) : std::nullopt;
        auto* tile = rowTiles.add (new ChordTile (key, chord, true));
        tile->onClick = [this, chord, previous] { showChord (chord, previous.has_value() ? &*previous : nullptr); playChord (chord); };
        addAndMakeVisible (tile);
    }

    // How to play it, chord by chord (the circle of fifths needs the room for itself).
    if (! chords.empty() && row != circleRow)
        diagramPanel.showProgression (chords);
    else
        diagramPanel.clear();

    circle->setVisible (row == circleRow);
    playButton.setEnabled (! chords.empty());
    putButton.setVisible (row >= firstProgressionRow);
    playAlongButton.setVisible (row == playAlongRow);
    setStartBar (startBar);
    resized();
}

void ChordExplainer::showChord (const Chord& chord, const Chord* previous)
{
    stop();
    explanation.setText (guide::explainChord (key, chord) + "\n\n" + gen::diagrams::explainPlaying (chord), false);
    explanation.moveCaretToTop (false);
    diagramPanel.showChord (chord, previous);
    resized();
}

void ChordExplainer::showHowToPlay (const Chord& chord)
{
    showChord (chord);
}

void ChordExplainer::showPlayingAlong()
{
    list.selectRow (playAlongRow);
}

void ChordExplainer::showTopic (int row)
{
    list.selectRow (juce::jlimit (0, getNumRows() - 1, row));
}

//==============================================================================
std::vector<Chord> ChordExplainer::getChordTrackChords() const
{
    // The first 16 chord changes, in bar order.
    std::vector<Chord> chords;

    for (auto& [bar, chord] : engine.getChords())
    {
        if (! chords.empty() && chords.back() == chord)
            continue;

        if (chords.size() == 16)
            break;

        chords.push_back (chord);
    }

    return chords;
}

std::vector<Chord> ChordExplainer::getShownChords() const
{
    const int row = list.getSelectedRow();

    if (row == chordTrackRow || row == playAlongRow)
        return getChordTrackChords();

    if (row == circleRow)
        return guide::getCircleProgression (key);

    if (row >= firstProgressionRow && row - firstProgressionRow < (int) progressions.size())
        return progressions[(size_t) (row - firstProgressionRow)].getChords (key);

    return {};
}

void ChordExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (followSongKey && key != engine.getKey())
    {
        setKey (engine.getKey());
        return;
    }

    updateKeyControls();
    updateHint();

    // Keep "Your chord track" and "Playing along" up to date while shown and nothing is playing.
    if (const int row = list.getSelectedRow(); (row == chordTrackRow || row == playAlongRow) && playing.empty())
    {
        const auto chords = getChordTrackChords();

        if (chords.size() != (size_t) rowTiles.size()
            || ! std::equal (chords.begin(), chords.end(), rowTiles.begin(), [] (const Chord& c, const ChordTile* t) { return c == t->chord; }))
            showRow (row);
    }

    list.repaint();
}

//==============================================================================
void ChordExplainer::playChord (const Chord& chord)
{
    updateHint();

    const int ms = getChordMs();
    engine.setPreviewSound (BuiltInSound::piano);   // when no MIDI track can play it

    for (int note : voice (chord))
        engine.previewNote (note, note < 60 ? 0.65f : 0.75f, ms - 60);
}

int ChordExplainer::getChordMs() const
{
    // Half a bar at the song tempo, or a whole bar when practising.
    const double beats = slowToggle.getToggleState() ? 4.0 : 2.0;
    return juce::jlimit (500, 4000, juce::roundToInt (beats * 60000.0 / engine.getTempo()));
}

void ChordExplainer::playNotes (const std::vector<int>& notes, bool strum)
{
    updateHint();
    const int ms = getChordMs();

    // Without a MIDI track to play them: DawIt's piano, and its pluck sound for guitar shapes.
    engine.setPreviewSound (strum ? BuiltInSound::pluck : BuiltInSound::piano);

    for (size_t i = 0; i < notes.size(); ++i)
    {
        const int note = notes[i];
        const float velocity = note < 55 ? 0.65f : 0.75f;

        // A strum: the strings one after the other, low to high.
        if (strum && i > 0)
            juce::Timer::callAfterDelay ((int) i * 28, [safeThis = juce::Component::SafePointer<ChordExplainer> (this), note, velocity, ms, i]
            {
                if (auto* self = safeThis.getComponent())
                    self->engine.previewNote (note, velocity, ms - 60 - (int) i * 28);
            });
        else
            engine.previewNote (note, velocity, ms - 60);
    }
}

void ChordExplainer::playAt (size_t index)
{
    highlight (&playing[index]);

    // As the diagrams show it, when they show this progression.
    bool strum = false;
    const auto notes = diagramPanel.isShowingProgression() ? diagramPanel.getProgressionNotes ((int) index, strum) : std::vector<int>();

    if (notes.empty())
        playChord (playing[index]);
    else
        playNotes (notes, strum);
}

void ChordExplainer::play (const std::vector<Chord>& chords)
{
    stop();

    if (chords.empty())
        return;

    playing = chords;
    playIndex = 0;
    playButton.setButtonText ("Stop");
    playAt (0);
    startTimer (getChordMs());
}

void ChordExplainer::timerCallback()
{
    if (++playIndex >= playing.size())
    {
        stop();
        return;
    }

    playAt (playIndex);
}

void ChordExplainer::stop()
{
    stopTimer();
    playing.clear();
    playIndex = 0;
    playButton.setButtonText ("Play");
    highlight (nullptr);
}

void ChordExplainer::highlight (const Chord* chord)
{
    for (int i = 0; i < rowTiles.size(); ++i)
        rowTiles[i]->setToggleState (chord != nullptr && (size_t) i == playIndex, juce::dontSendNotification);

    for (auto* tile : keyTiles)
        tile->setToggleState (chord != nullptr && tile->chord == *chord, juce::dontSendNotification);

    circle->setLit (chord);
    diagramPanel.setPlayingIndex (chord != nullptr ? (int) playIndex : -1);
}

void ChordExplainer::updateHint()
{
    const auto* track = engine.getLiveMidiTrack();

    if (engine.previewsUseOwnSound())
        hintLabel.setText ((track == nullptr || ! track->isMidi() ? juce::String ("No MIDI track selected")
                                                                   : "The selected track plays " + builtInSounds::getName (track->builtInSound).toLowerCase())
                               + ", so chords play on DawIt's piano (guitar shapes on its pluck sound). "
                                 "Select a MIDI track to hear them in its sound.",
                           juce::dontSendNotification);
    else
        hintLabel.setText ("Chords play on the selected track, " + track->name + ".", juce::dontSendNotification);
}

void ChordExplainer::putOnChordTrack()
{
    const int row = list.getSelectedRow();

    if (row < firstProgressionRow || row - firstProgressionRow >= (int) progressions.size())
        return;

    const auto& progression = progressions[(size_t) (row - firstProgressionRow)];
    const auto chords = progression.getChords (key);
    const int first = startBar, last = startBar + (int) chords.size() - 1;
    int replaced = 0;

    for (int i = 0; i < (int) chords.size(); ++i)
        if (const auto* existing = engine.getChord (first + i); existing != nullptr && ! (*existing == chords[(size_t) i]))
            ++replaced;

    auto apply = [safeThis = juce::Component::SafePointer<ChordExplainer> (this), chords, first, last, name = progression.name]
    {
        auto* self = safeThis.getComponent();

        if (self == nullptr)
            return;

        auto map = self->engine.getChords();
        for (int i = 0; i < (int) chords.size(); ++i)
            map[first + i] = chords[(size_t) i];

        self->engine.setChords (std::move (map));
        self->hintLabel.setText ("Put the " + name + " on bars " + juce::String (first + 1) + " to " + juce::String (last + 1) + ".",
                                 juce::dontSendNotification);
    };

    // Picked in another key than the song's: ask whether the song should change to that key as well.
    auto askKeyThenApply = [safeThis = juce::Component::SafePointer<ChordExplainer> (this), apply, newKey = key]
    {
        auto* self = safeThis.getComponent();

        if (self == nullptr)
            return;

        const auto songKey = self->engine.getKey();

        if (newKey == songKey)
        {
            apply();
            return;
        }

        juce::AlertWindow::showYesNoCancelBox (juce::MessageBoxIconType::QuestionIcon, "Change the song key?",
                                               "These chords are in " + newKey.getName() + ", but the song is in "
                                                   + songKey.getName() + ". Change the song key to " + newKey.getName() + " as well?",
                                               "Change to " + newKey.getName(), "Keep " + songKey.getName(), "Cancel", self,
                                               juce::ModalCallbackFunction::create ([safeThis, apply, newKey] (int result)
        {
            if (result == 0) // Cancel: put nothing
                return;

            if (auto* s = safeThis.getComponent(); s != nullptr && result == 1)
            {
                s->followSongKey = true;
                s->engine.setKey (newKey);
            }

            apply();
        }));
    };

    if (replaced == 0)
    {
        askKeyThenApply();
        return;
    }

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Replace chords?",
                                        "Bars " + juce::String (first + 1) + " to " + juce::String (last + 1) + " already have "
                                            + juce::String (replaced) + (replaced == 1 ? " other chord" : " other chords")
                                            + ". Replace them with the " + progression.name + "?",
                                        "Replace", "Cancel", this,
                                        juce::ModalCallbackFunction::create ([askKeyThenApply] (int result) { if (result == 1) askKeyThenApply(); }));
}

//==============================================================================
void ChordExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    // Legend: what the three families mean.
    const std::pair<guide::Family, const char*> entries[] = {
        { guide::Family::home, "Home: at rest" },
        { guide::Family::away, "Away: moving off" },
        { guide::Family::tension, "Tension: wants to go home" },
        { guide::Family::outside, "Outside the key" }
    };

    auto r = legendArea;
    g.setFont (juce::FontOptions (12.5f));

    for (auto& [family, text] : entries)
    {
        auto dot = r.removeFromLeft (14).withSizeKeepingCentre (9, 9).toFloat();
        g.setColour (familyColour (family));
        g.fillEllipse (dot);
        r.removeFromLeft (5);
        g.setColour (theme::textDim);
        const int textWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text) + 2;
        g.drawText (text, r.removeFromLeft (textWidth), juce::Justification::centredLeft);
        r.removeFromLeft (20);
    }
}

void ChordExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    title.setBounds (top.removeFromLeft (260));
    songKeyButton.setBounds (top.removeFromRight (200));

    top.removeFromRight (8);
    scaleBox.setBounds (top.removeFromRight (150));
    top.removeFromRight (6);
    tonicBox.setBounds (top.removeFromRight (70));
    top.removeFromRight (6);
    keyLabel.setBounds (top.removeFromRight (40));
    top.removeFromRight (8);
    exportButton.setBounds (top.removeFromRight (120));

    r.removeFromTop (14);

    auto layoutTiles = [] (juce::OwnedArray<ChordTile>& tiles, juce::Rectangle<int> area, int maxWidth, int gap)
    {
        if (tiles.isEmpty())
            return;

        const int w = juce::jmin (maxWidth, (area.getWidth() - gap * (tiles.size() - 1)) / tiles.size());

        for (auto* tile : tiles)
        {
            tile->setBounds (area.removeFromLeft (w));
            area.removeFromLeft (gap);
        }
    };

    layoutTiles (keyTiles, r.removeFromTop (80), 120, 8);
    r.removeFromTop (6);
    legendArea = r.removeFromTop (20);
    r.removeFromTop (12);

    hintLabel.setBounds (r.removeFromBottom (20));
    r.removeFromBottom (6);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    auto chips = r.removeFromTop (rowTiles.isEmpty() ? 0 : 50);
    layoutTiles (rowTiles, chips, 76, 5);

    if (! rowTiles.isEmpty())
        r.removeFromTop (10);

    auto buttons = r.removeFromTop (28);
    playButton.setBounds (buttons.removeFromLeft (90));
    buttons.removeFromLeft (8);
    slowToggle.setBounds (buttons.removeFromLeft (80));
    buttons.removeFromLeft (8);
    putButton.setBounds (buttons.withWidth (240));
    playAlongButton.setBounds (buttons.removeFromLeft (120));
    sheetButton.setBounds (buttons.removeFromRight (150));
    r.removeFromTop (10);

    if (diagramPanel.isShowingSomething())
    {
        diagramPanel.setBounds (r.removeFromTop (diagramPanel.getIdealHeight (r.getWidth())));
        r.removeFromTop (10);
    }
    else
    {
        diagramPanel.setBounds ({});
    }

    if (circle->isVisible())
    {
        circle->setBounds (r.removeFromLeft (juce::jmin (r.getHeight(), r.getWidth() * 3 / 5)));
        r.removeFromLeft (10);
    }

    explanation.setBounds (r);
    repaint();
}
} // namespace daw
