#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "GeneratePanel.h"
#include "PitchRows.h"
#include "ScoreView.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The note grid of the piano roll.

    Click empty space to draw a note (drag to set its length), drag a note to move it,
    drag its right edge to resize it. Right-click or double-click deletes. Shift-click adds
    to the selection, Cmd/Ctrl-drag on empty space selects with a rectangle.
    In select mode, empty space never draws: dragging there always selects with a rectangle.
    Keys: Delete, Cmd+A, arrows (Shift+Up/Down = octave). Clicking the keyboard beside the grid
    selects the notes of a pitch (see selectPitch()).

    With the scale filter on, only the pitches of the song key get a row, so notes outside it are
    hidden and moving a note up or down steps through the scale. Each bar also tints the rows of
    its chord from the chord track.
*/
class PianoRollGrid final : public juce::Component, public juce::TooltipClient
{
public:
    PianoRollGrid (Engine&, const PitchRows&);

    void setClip (Track*, MidiClip*);

    /** Call after the rows changed: hidden notes leave the selection. */
    void rowsChanged();

    /** Selects every note of the pitch in the clip, or of its note in every octave. Unless adding, this
        replaces the selection; when adding and those notes are all selected already, they leave it instead.
        Returns false, keeping the selection, when the clip has no such notes. */
    bool selectPitch (int pitch, bool everyOctave, bool add);
    MidiClip* getClip() const noexcept { return clip; }
    Track* getTrack() const noexcept { return track; }
    const juce::SortedSet<int>& getSelectedNotes() const noexcept { return selected; }

    /** Selects just this note (by index in the clip), as a double-click in the score does. */
    void selectOnly (int index);

    /** The note under the mouse: its pitch, value (quarter note, dotted eighth...), length in beats,
        and where it starts as bar.beat.sixteenth. */
    juce::String getTooltip() override;

    static constexpr int rowHeight = 12;
    double pixelsPerBeat = 64.0;
    double snapBeats = 0.25;
    int minimumWidth = 0;
    bool selectMode = false;
    bool showChords = false;

    std::function<void (double factor, float anchorX)> onZoom;

    void updateSize();
    void updatePlayhead();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    int noteAt (juce::Point<float>) const;
    juce::Rectangle<float> getNoteBounds (const MidiNote&) const;
    int yToRow (float y) const noexcept { return juce::jlimit (0, rows.getNumRows() - 1, (int) std::floor (y / (float) rowHeight)); }
    int yToPitch (float y) const noexcept { return rows.getPitch (yToRow (y)); }
    void paintChordRows (juce::Graphics&, juce::Rectangle<int> area);
    double xToBeat (float x) const noexcept { return x / pixelsPerBeat; }
    double snapDown (double beat) const { return std::floor (beat / snapBeats) * snapBeats; }
    double snapNearest (double beat) const { return std::round (beat / snapBeats) * snapBeats; }
    bool isOnRightEdge (int index, juce::Point<float>) const;

    void deleteNotes (juce::SortedSet<int> indices);
    void transposeSelected (int semitones);
    void stepSelected (int rowsUp);
    void shiftSelected (double beats);
    void extendClipToFitNotes();
    void preview (int pitch, float velocity);
    void commit();

    Engine& engine;
    const PitchRows& rows;
    Track* track = nullptr;
    MidiClip* clip = nullptr;

    juce::SortedSet<int> selected;
    std::vector<MidiNote> dragOriginal;

    enum class Mode { none, move, resize, lasso };
    Mode mode = Mode::none;
    int anchor = -1;
    double dragStartBeat = 0.0;
    int dragStartRow = 0, lastPreviewPitch = -1;
    double lastLength = 1.0;
    juce::Rectangle<float> lasso;
    juce::SortedSet<int> lassoBase;   // selection kept when Shift-dragging a rectangle

    int justCreated = -1;
    juce::uint32 justCreatedTime = 0;
    float playheadX = -100.0f;
};

/** Vertical keyboard to the left of the grid, one key per row. Click a key to hear it and select its
    notes in the clip (Shift adds or removes them, Option/Alt takes the note in every octave).
    It can light up the notes of a chord. */
class PianoKeyboardStrip final : public juce::Component, public juce::SettableTooltipClient
{
public:
    PianoKeyboardStrip (Engine& e, const PitchRows& r) : engine (e), rows (r) {}

    void setScrollOffset (int y)
    {
        offset = y;
        repaint();
    }

    void setChord (const std::optional<Chord>&);

    std::function<void (int pitch, const juce::ModifierKeys&)> onKeyClicked;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    Engine& engine;
    const PitchRows& rows;
    int offset = 0;
    std::optional<Chord> chord;
};

/** Bottom panel: shows the selected MIDI clip in a piano roll, or, with the *Piano roll | Score* switch on Score,
    the selected track written out as sheet music (see ScoreView). The choice is remembered (`view.clipEditorMode`). */
class PianoRoll final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    PianoRoll (Engine&, Selection&);
    ~PianoRoll() override;

    /** Called when the ? next to Snap is clicked: open the note lengths guide. */
    std::function<void()> onShowRhythmGuide;

    /** Called when "What is MIDI?" is clicked: open the MIDI guide. */
    std::function<void()> onShowMidiGuide;

    /** Called when "How to read music" is clicked in score mode: open the reading guide. */
    std::function<void()> onShowReadingGuide;

    /** Open the reading guide at its generated-music topic. */
    std::function<void()> onShowGeneratedMusicGuide;

    /** Called when "Sheet music (PDF)..." is clicked in score mode: open the export dialog. */
    std::function<void()> onExportSheetMusic;

    /** Called when the switch changes, so the View menu can tick the right item. */
    std::function<void()> onModeChanged;

    /** Switches between the piano roll (false) and the score (true), and remembers it. */
    void setScoreMode (bool);
    bool isScoreMode() const noexcept { return scoreMode; }

    /** Shows the piano roll with this clip selected, and the note (or -1) selected and in view at `beat`. */
    void openFromScore (Clip*, int note, double beat);

    /** The clip being edited and its track, or nulls. */
    const MidiClip* getClip() const noexcept { return grid.getClip(); }
    const Track* getTrack() const noexcept { return grid.getTrack(); }

    /** Called when the close button is clicked: hide the editor. */
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    friend class ScreenshotRunner;
    struct GridViewport final : public juce::Viewport
    {
        std::function<void (const juce::Rectangle<int>&)> onVisibleAreaChanged;

        void visibleAreaChanged (const juce::Rectangle<int>& r) override
        {
            if (onVisibleAreaChanged)
                onVisibleAreaChanged (r);
        }
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void showSelectedClip();
    void zoom (double factor, float anchorX);
    void updateOctaveButtons();
    void updateTitle();
    void updateModeVisibility();
    void showScaleFilter (bool);
    void updateRows();
    void updateKeyboardChord();
    juce::String describeSelection() const;

    Engine& engine;
    Selection& selection;

    juce::Label title, snapLabel { {}, "Snap" };
    juce::ComboBox snapBox;
    juce::TextButton zoomInButton { "+" }, zoomOutButton { "-" }, generateButton { "Generate..." },
                     octaveDownButton { "Octave -" }, octaveUpButton { "Octave +" }, selectButton { "Select" },
                     scaleButton { "Scale" },
                     closeButton { juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) };
    GuideButton midiGuideButton { "What is MIDI?" };
    GuideButton rhythmButton { "?" };
    juce::TextButton pianoRollModeButton { "Piano roll" }, scoreModeButton { "Score" }, clefButton { "Clef" },
                     pdfButton { "Sheet music (PDF)..." };
    GuideButton readMusicButton { "How to read music" };
    ScoreView scoreView { engine, selection };
    bool scoreMode = false;
    juce::String shownSelection;   // describeSelection() as the title shows it
    PitchRows rows;
    MusicalKey shownKey;
    PianoKeyboardStrip keyboard { engine, rows };
    PianoRollGrid grid { engine, rows };
    GridViewport viewport;
    GeneratePanel generatePanel { engine };

    static constexpr int headerHeight = 30, keyboardWidth = 56;
};
} // namespace daw
