#pragma once

#include "engine/Engine.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/** The timeline area to the right of the track headers: draws and edits clips. A bus has no clips: its lane shows
    its tracks' clips as thin bars, one line per track, so a folded bus still shows where its tracks play; the lanes
    of the tracks in a bus have a faint tint of the bus's colour. Double-click a bus's lane to fold it. */
class TrackLanes final : public juce::Component, public juce::FileDragAndDropTarget, public juce::TooltipClient
{
public:
    TrackLanes (Engine&, Selection&, TimelineView&);

    static constexpr int rowHeight = 84;

    std::function<void (double deltaBeats)> onScroll;
    std::function<void (double factor, double anchorBeat)> onZoom;
    std::function<void (Track&, double beat)> onImportRequest;
    std::function<void (Track&, MidiClip&)> onSingThis;
    /** The clip menu's *Bounce clip in place* (audio and MIDI clips). */
    std::function<void (Track&, Clip&)> onBounceClip;
    std::function<void (Track&, AudioClip&)> onSplitIntoStems;
    std::function<void (Track&, AudioClip&)> onRemoveVocals;
    std::function<void (Track&, AudioClip&)> onConvertToMidi;
    /** The clip menu's *Hits to MIDI...* (audio clips). */
    std::function<void (Track&, AudioClip&)> onHitsToMidi;
    std::function<void (Track&, AudioClip&)> onDetectLyrics;
    std::function<void (Track&, AudioClip&)> onDetectTempo;
    std::function<void (Track&, AudioClip&)> onDetectChords;
    std::function<void (Track&, AudioClip&)> onDetectStructure;

    /** The clip menu's *About recording audio...*: open the How recording works guide at its takes topic. */
    std::function<void()> onShowRecordingGuide;
    std::function<void()> onShowTriggerGuide;

    /** Called when a clip is double-clicked, or a MIDI clip created by double-clicking: open the clip editor. */
    std::function<void()> onOpenClipEditor;

    /** Places the files one after another on the track, starting at the given beat. */
    void importAudioFiles (const juce::Array<juce::File>&, Track&, double startBeat);

    void deleteSelectedClip();
    void duplicateSelectedClip();
    void toggleSelectedClipMute();
    void updatePlayhead();

    /** While audio is being recorded, redraws the growing take of each armed track (call from the 30 Hz timer). */
    void updateRecording();

    /** The rows shown: every track but those inside folded buses, in order. Rebuilt from the engine on each call. */
    const std::vector<Track*>& getRows() const;

    juce::String getTooltip() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void modifierKeysChanged (const juce::ModifierKeys&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragMove (const juce::StringArray&, int x, int y) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int x, int y) override;

private:
    struct Hit
    {
        Track* track = nullptr;
        Clip* clip = nullptr;
        bool onRightEdge = false;
    };

    Hit findClipAt (juce::Point<float>) const;
    Track* trackAtY (float y) const;
    int getRowOf (const Track*) const;
    void drawBusSummary (juce::Graphics&, const Track& bus, int row);

    /** The tracks a bus's or a return's lane shows: the tracks that play into the bus, or that send to the return. */
    std::vector<const Track*> getLaneTracks (const Track& bus) const;
    juce::Rectangle<float> getClipBounds (int row, const Clip&) const;
    double snap (double beat, const juce::ModifierKeys&) const;
    double snapStep() const;
    void drawGrid (juce::Graphics&, juce::Rectangle<int> area);
    void drawClip (juce::Graphics&, const Track&, const Clip&, juce::Rectangle<float>, bool selected);
    void drawLiveTakes (juce::Graphics&);
    bool isHiddenTake (const Track&, const Clip&) const;
    void showClipMenu (Track&, Clip&);
    void setCopying (bool);

    Engine& engine;
    Selection& selection;
    TimelineView& timeline;

    enum class Drag { none, move, resize };
    Drag drag = Drag::none;
    Track* dragTrack = nullptr;
    Track* dragOriginalTrack = nullptr;
    Clip* dragClip = nullptr;
    double dragOriginalStart = 0.0, dragOriginalLength = 0.0, dragMouseStartBeat = 0.0;
    bool copying = false;   // Cmd (Ctrl on Windows) held while moving: dropping leaves the original and adds a copy

    mutable std::vector<Track*> rows;   // see getRows()

    int dropRow = -1;
    float playheadX = -100.0f;
    bool drewLiveTakes = false;
};
} // namespace daw
