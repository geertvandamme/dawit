#pragma once

#include "engine/Engine.h"
#include "generate/ChordDiagrams.h"
#include "Theme.h"

namespace daw
{
/** A piano keyboard with a chord's keys lit: the right hand's notes, the left hand's bass note in
    another colour, the root marked, and keys held over from the chord before ringed. Click it to
    hear the chord. */
class KeyboardDiagram final : public juce::Component, public juce::SettableTooltipClient
{
public:
    KeyboardDiagram();

    /** The keys shown, widened to start and end on white keys. */
    void setRange (int lowestNote, int highestNote);
    void setChord (const std::vector<int>& rightHand, int bass, int rootPitchClass, const std::vector<int>& held = {});
    void setShowNames (bool shouldShow) { showNames = shouldShow; repaint(); }

    /** How to spell each pitch class under the keys ("Eb" rather than "D#"); sharps otherwise. */
    void setNames (const std::array<juce::String, 12>& spelled) { names = spelled; repaint(); }

    /** The right hand's notes from low to high, e.g. "E G C". */
    juce::String getNoteNamesText() const;
    void setLit (bool shouldBeLit) { lit = shouldBeLit; repaint(); }

    /** Dark text and lines for white paper, as on a printed chord sheet. */
    void setPrinted (bool isPrinted) { printed = isPrinted; repaint(); }

    /** Dots and names that grow with the keys, for a keyboard filling a window (play along). */
    void setLarge (bool isLarge) { large = isLarge; repaint(); }

    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    int low = 48, high = 83, bass = -1, root = 0;
    std::vector<int> notes, heldNotes;
    std::array<juce::String, 12> names;
    bool showNames = true, lit = false, printed = false, large = false;

    juce::String getName (int note) const;
};

/** A guitar chord box: the six strings (low E on the left), frets down from the nut or from the
    fret it starts at, finger numbers in the dots, the barre, and o / x for open and unplayed
    strings. Dots held over from the chord before are ringed. Click it to hear the shape strummed. */
class GuitarDiagram final : public juce::Component, public juce::SettableTooltipClient
{
public:
    GuitarDiagram();

    void setShape (const gen::diagrams::GuitarShape&, const gen::diagrams::GuitarShape* previous = nullptr);
    void setLit (bool shouldBeLit) { lit = shouldBeLit; repaint(); }
    void setCompact (bool isCompact) { compact = isCompact; repaint(); }

    /** Dark text and lines for white paper, as on a printed chord sheet. */
    void setPrinted (bool isPrinted) { printed = isPrinted; repaint(); }

    /** Lines, marks and numbers that grow with the box, for a chord box filling a window (play along). */
    void setLarge (bool isLarge) { large = isLarge; repaint(); }

    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    gen::diagrams::GuitarShape shape;
    std::vector<bool> held;
    bool lit = false, compact = false, printed = false, large = false;
};

/**
    The chord guide's "how to play it" panel. For one chord: a keyboard with its inversions to pick
    from (the one that moves least from the chord before is starred) and up to three guitar shapes.
    For a progression: a row of keyboards or chord boxes, one per chord, voiced to move as little as
    possible, with the keys and fingers that stay put ringed. Clicking a diagram plays it.
*/
class ChordDiagramPanel final : public juce::Component
{
public:
    ChordDiagramPanel();

    /** Shows how to play one chord; `previous` (the chord before it, if any) picks the smoothest inversion. */
    void showChord (const Chord&, const Chord* previous = nullptr);

    /** Shows a progression, one diagram per chord. */
    void showProgression (const std::vector<Chord>&);

    void clear();
    bool isShowingSomething() const noexcept { return mode != Mode::none; }
    bool isShowingProgression() const noexcept { return mode == Mode::progression; }

    /** The height it needs at a width. */
    int getIdealHeight (int width) const;

    /** Lights the progression's chord at `index` while it plays (-1: none). */
    void setPlayingIndex (int index);

    /** The notes to play for the progression's chord at `index`, as shown: the piano voicing with
        its bass, or the guitar shape (then `strum` is set). */
    std::vector<int> getProgressionNotes (int index, bool& strum) const;

    /** Called with the notes of a clicked diagram, low to high; `strum` for a guitar shape. */
    std::function<void (const std::vector<int>&, bool strum)> onPlay;

    /** Called when it needs another height (switching between piano and guitar). */
    std::function<void()> onLayoutChanged;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    enum class Mode { none, chord, progression };

    void showInversion (int inversion);
    void updateInstrument();

    Mode mode = Mode::none;
    Chord chord;
    std::vector<Chord> chords;
    int smoothInversion = -1;

    // One chord.
    KeyboardDiagram keyboard;
    juce::OwnedArray<juce::TextButton> inversionButtons;
    juce::Label keyboardCaption;
    juce::OwnedArray<GuitarDiagram> guitarBoxes;
    juce::OwnedArray<juce::Label> guitarCaptions;
    std::vector<gen::diagrams::GuitarShape> shapes;
    int inversion = 0;

    // A progression.
    juce::TextButton pianoButton { "Piano" }, guitarButton { "Guitar" };
    juce::OwnedArray<KeyboardDiagram> keyboards;
    juce::OwnedArray<GuitarDiagram> boxes;
    juce::OwnedArray<juce::Label> names;
    std::vector<std::vector<int>> voicings;
    std::vector<gen::diagrams::GuitarShape> progressionShapes;
    bool showGuitar = false;

    static constexpr int perRow = 8;
};
} // namespace daw
