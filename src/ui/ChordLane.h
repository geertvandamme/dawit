#pragma once

#include "ChordDiagramViews.h"
#include "engine/ChordEditing.h"
#include "engine/Engine.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The chord track, a row under the ruler that is always there. It shows the chord set on each
    bar; click a bar to pick or type its chord. It has no audio or MIDI: it only tells the
    generator (and the user) what the harmony is.

    Bars can be selected (a click selects one, Shift-click or Shift-drag selects several) and then
    copied, cut, pasted, duplicated after themselves, repeated until the end of the song or cleared,
    from the menu or with Cmd+C, X, V, D and Delete. As with clips, dragging moves the selection,
    or the chord block under the mouse, and with Cmd (Ctrl on Windows) held copies it. Changes
    that would replace other chords ask first.

    The row can be opened (setExpanded()) to show under each chord block how to play it on piano or
    guitar, drawn as in the chord guide: each chord voiced near the one before, with the keys or
    fingers that stay ringed. Click a diagram to hear it. Collapsed is the default.
*/
class ChordLane final : public juce::Component, public juce::SettableTooltipClient, private juce::ChangeListener
{
public:
    ChordLane (Engine&, TimelineView&);
    ~ChordLane() override;

    static constexpr int height = 26;   // collapsed, and the chord blocks' strip when open

    enum class Instrument { piano, guitar };

    /** Opening shows the diagrams under the chords. Both choices are kept in the app's settings. */
    void setExpanded (bool shouldBeExpanded);
    bool isExpanded() const noexcept { return expanded; }
    void setInstrument (Instrument);
    Instrument getInstrument() const noexcept { return instrument; }

    /** The row's height: `height`, or taller when open. */
    int getPreferredHeight() const noexcept;

    /** Called when the row's height or instrument changes, so the layout and header can follow. */
    std::function<void()> onLayoutChanged;

    /** Called with the clicked bar when "Explain chord progressions..." is picked. */
    std::function<void (int bar)> onExplainProgressions;

    /** Called with the clicked bar when "How to play ... on piano and guitar..." is picked. */
    std::function<void (int bar)> onShowHowToPlay;

    /** Called with the clicked bar when "Play chords as..." is picked. */
    std::function<void (int bar)> onPlayChords;

    /** Called when "Play along with the chords..." is picked. */
    std::function<void()> onPlayAlong;

    /** The colour of a chord's block, which other views use for the same chord. */
    static juce::Colour getColour (const Chord&);

    void updatePlayhead();

    /** The selected bars: the first one and how many (0 when nothing is selected). */
    int getSelectionStart() const noexcept { return selectionStart; }
    int getSelectionBars() const noexcept { return selectionBars; }
    void selectBars (int firstBar, int numBars);

    // What the menu items and keys do, on the selected bars.
    void copySelection();
    void cutSelection();
    void paste();
    void duplicateSelection();
    void repeatSelectionToEnd();
    void clearSelection();

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void modifierKeysChanged (const juce::ModifierKeys&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    int barAt (float x) const;
    bool isSelected (int bar) const noexcept { return bar >= selectionStart && bar < selectionStart + selectionBars; }
    void showMenu (int bar);
    void typeChord (int bar);

    /** What Paste puts: chords copied as text elsewhere ("Am F C G") or else DawIt's own copy. */
    ChordBars getClipboard() const;

    /** Sets the chord track to `newChords`, first asking when that replaces `replaced` chords;
        then selects `selectBarsAfter` bars from `selectFrom`. */
    void applyAsking (ChordMap newChords, int replaced, int selectFrom, int selectBarsAfter, const juce::String& action);

    void updateDragPreview (bool copying);

    /** A run of bars with the same chord, and how to play it after the run before. */
    struct Block
    {
        int startBar = 0, endBar = 0;
        Chord chord;
        std::vector<int> voicing, previousVoicing;
        gen::diagrams::GuitarShape shape, previousShape;
    };

    const std::vector<Block>& getBlocks (const ChordMap&);
    juce::Rectangle<float> getDiagramArea (const Block&) const;
    const Block* blockAtDiagram (juce::Point<float>);
    void playBlock (const Block&);

    Engine& engine;
    TimelineView& timeline;
    int hoverBar = -1;
    float playheadX = -100.0f;

    int selectionStart = 0, selectionBars = 0;

    enum class Drag { none, select, move };
    Drag drag = Drag::none;
    int dragAnchorBar = 0, dragBar = 0, dropBar = 0;
    bool dragCopies = false;
    bool blockSelectedOnPress = false;
    bool pressedDiagram = false;            // the press played a diagram: no menu or drag      // a press on a chord selected its block, ready to drag it
    std::optional<ChordMap> dragPreview;   // the chord track as it will be when the drag ends

    bool expanded = false;
    Instrument instrument = Instrument::piano;
    std::vector<Block> blocks;
    ChordMap blocksFrom;                   // the chords `blocks` were made for
    int pianoLow = 55, pianoHigh = 76;     // the range all the keyboards share
    KeyboardDiagram keyboardPainter;       // not shown: they draw the diagrams into this row
    GuitarDiagram guitarPainter;

    static constexpr int pianoDiagramHeight = 56, guitarDiagramHeight = 92;

    static inline ChordBars clipboard;     // shared, so a copy can be pasted after opening another project
    static inline juce::String clipboardText;
};
} // namespace daw
