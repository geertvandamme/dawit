#pragma once

#include "engine/Engine.h"
#include "ScorePainter.h"
#include "Selection.h"

namespace daw
{
/** The body of the clip editor in Score mode: the selected MIDI track written out as sheet music, as one long
    line that scrolls sideways, with the clef and key signature pinned at the left edge.

    Above the staff it shows section names, the tempo mark, chord symbols and bar numbers (bars match the ruler).
    The bars of the selected clip have a faint wash, the playhead is a red line, and notes that are sounding light
    up. It is read-only: nothing here changes the song. Click a note to hear it with the track's sound, click
    elsewhere to move the playhead, double-click to open the piano roll there, and right-click for the Clef menu.
    Cmd+scroll zooms, scroll and Shift+scroll move. The notation is built by `gen::notation::build` and drawn
    by `scorepaint`. */
class ScoreView final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    ScoreView (Engine&, Selection&);
    ~ScoreView() override;

    /** Called on double-click: the clip and note index to show in the piano roll (note -1: just open it there), and the beat. */
    std::function<void (Clip*, int note, double beat)> onOpenPianoRoll;

    /** Called by the menu's last item and the header's guide button: open the guide How to read music. */
    std::function<void()> onShowGuide;

    /** Called by the menu's export item and the header's PDF button. */
    std::function<void()> onExportPdf;

    /** True when a MIDI track with notes is shown. */
    bool hasNotes() const noexcept { return ! score.staves.empty() && hasAnyNotes; }

    /** Staff space in pixels, 6 to 16 (saved as `view.scoreZoom`). */
    float getSpace() const noexcept { return space; }
    void setSpace (float);

    /** Zooms by a factor, keeping the music at `anchorX` (in this view) where it is. */
    void zoomBy (double factor, float anchorX);

    /** Shows the Clef menu of the selected track under `target`; the same choices as the right-click menu. */
    void showClefMenu (juce::Component& target);

    /** Rebuilds the notation from the selected track. Called on every change of the song or the selection. */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    class Canvas;

    struct Source { MidiClip* clip = nullptr; int note = 0; };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void rebuild();
    void updateCanvasSize();
    void scrollToSelectedClip();
    void ensureHits();
    void paintScore (juce::Graphics&);
    const scorepaint::Hit* findHit (juce::Point<float>, bool onlyNotesAndRests) const;
    const scorepaint::Hit* findHeaderHit (juce::Point<float>) const;
    juce::String getTooltipAt (juce::Point<float>);
    void handleClick (const juce::MouseEvent&);
    void handleDoubleClick (const juce::MouseEvent&);
    void showMenu();
    void setClef (ScoreClef);
    void addClefItems (juce::PopupMenu&);

    float beatToX (double beat) const;
    double xToBeat (float x) const;
    float getBarX (int bar) const;
    float getTopY() const noexcept;
    float getMargin() const noexcept { return 2.2f * space; }      // room left of the staff for a grand staff's brace
    float getHeaderEnd() const noexcept { return getMargin() + (float) line.headerWidth * space; }
    juce::String describeEvent (const gen::notation::Event&, int staff, int bar) const;
    juce::String describeNote (const gen::notation::WrittenPitch&) const;
    std::vector<std::pair<double, double>> getBarPoints (int bar) const;

    Engine& engine;
    Selection& selection;

    std::unique_ptr<Canvas> canvas;
    juce::Viewport viewport;

    // What is shown: copied from the song when something changed.
    Track* shownTrack = nullptr;
    gen::notation::Input input;
    gen::notation::Score score;
    gen::notation::Line line;
    std::vector<Source> sources;            // one per input note
    std::vector<std::vector<std::pair<double, double>>> barPoints;   // per bar: (beat in the bar, x in spaces) for the playhead
    bool hasAnyNotes = false;
    bool showChords = true;
    float space = 9.0f;
    MidiClip* scrolledTo = nullptr;
    std::vector<scorepaint::Hit> hits, headerHits;
    bool hitsDirty = true;
    juce::Point<float> mousePosition;
    double lastPlayheadBeat = -1.0;
    bool wasPlaying = false;
};
} // namespace daw
