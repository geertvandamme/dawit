#pragma once

#include "ChordExplainer.h"
#include "GuideButton.h"
#include "ChordPlayAlong.h"
#include "LyricsSingAlong.h"
#include "ChordActions.h"
#include "ChordStyleActions.h"
#include "AudioToMidiActions.h"
#include "HitsToMidiActions.h"
#include "ChordLane.h"
#include "LyricsActions.h"
#include "SingActions.h"
#include "TempoActions.h"
#include "TempoLane.h"
#include "LyricsExplainer.h"
#include "LyricsEditWindow.h"
#include "SingingExplainer.h"
#include "LyricsLane.h"
#include "SectionLane.h"
#include "StructureActions.h"
#include "StructureExplainer.h"
#include "StemActions.h"
#include "TrackHeader.h"
#include "TrackLanes.h"
#include "TrackActions.h"
#include "engine/Routing.h"
#include "generate/TheoryGuides.h"

namespace daw
{
/** Bar numbers, playhead and loop range. Click to move the playhead, drag to set the loop,
    right-click for the note lengths guide. */
class Ruler final : public juce::Component
{
public:
    Ruler (Engine&, TimelineView&);

    std::function<void()> onLoopChanged;
    std::function<void()> onShowRhythmGuide;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    Engine& engine;
    TimelineView& timeline;
    double dragStartBeat = 0.0;
    bool draggingLoop = false;
};

/** The name at the left of the Sections and Chords rows; clicking it opens that row's guide. */
class RowLabel final : public juce::Component, public juce::SettableTooltipClient
{
public:
    RowLabel (const juce::String& labelText, const juce::String& tooltip);

    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::String text;
};

/** The Chords row's header: the arrow opens or closes the row's diagrams, the name opens the chord
    guide, Play along opens the chords large in a window of their own, and while the row is open, Piano or Guitar picks which diagrams it shows and PDF... saves
    the song's chords as a sheet with those diagrams. */
class ChordTrackHeader final : public juce::Component
{
public:
    explicit ChordTrackHeader (ChordLane&);

    std::function<void()> onOpenGuide;
    std::function<void()> onExportPdf;
    std::function<void()> onPlayAlong;

    /** Follows the row: open or closed, and its instrument. */
    void update();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ChordLane& lane;
    juce::TextButton arrowButton;
    RowLabel label { "Chords", "Click to open the chord guide: chord progressions, the circle of fifths, and how to play chords on piano and guitar" };
    juce::TextButton pianoButton { "Piano" }, guitarButton { "Guitar" }, pdfButton { "PDF..." }, playAlongButton { "Play along" };
};

/** The Lyrics row's header: the arrow opens or closes the row's written-out lines, ? opens the guide
    to writing lyrics, Sing along opens the words to read while recording vocals, Edit... opens the text, Tap starts (or
    stops) tapping the syllables in time while the song plays. */
class LyricsTrackHeader final : public juce::Component
{
public:
    explicit LyricsTrackHeader (LyricsLane&);

    std::function<void()> onOpenGuide;
    std::function<void()> onSingAlong;

    /** Follows the row: open or closed, tapping or not. */
    void update();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    LyricsLane& lane;
    juce::TextButton arrowButton, singButton { "Sing along" }, editButton { "Edit..." }, tapButton { "Tap" };
    GuideButton guideButton { "?" };
};

/** The Sections, Chords and Lyrics rows folded into one thin row, to give the tracks the room when
    they aren't being worked on: a coloured line each, with the sections in their type's colour,
    the chords in their root's colour and the lyrics' lines. Hover it to see what's at that bar,
    click to move the playhead, and double-click to show the rows in full again. */
class FoldedSongRows final : public juce::Component, public juce::TooltipClient, private juce::ChangeListener
{
public:
    FoldedSongRows (Engine&, TimelineView&);
    ~FoldedSongRows() override;

    static constexpr int height = 20;

    /** Taller by a line while the song's tempo changes: the tempo's line comes first. */
    int getPreferredHeight() const { return TempoLane::isNeeded (engine) ? height + lineStep : height; }

    /** Where the line of row `index` is drawn, here and in the header: 0 sections, 1 chords,
        2 lyrics, or with the tempo's line first 0 tempo, 1 sections and so on. */
    static juce::Rectangle<float> getLine (int index, float width);

    static constexpr int lineStep = 5;

    std::function<void()> onUnfold;

    void updatePlayhead();

    juce::String getTooltip() override;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    Engine& engine;
    TimelineView& timeline;
    float playheadX = -100.0f;
};

/** The header of the folded rows: the arrow shows them in full again, and a small legend names the lines. */
class FoldedSongRowsHeader final : public juce::Component, public juce::SettableTooltipClient
{
public:
    FoldedSongRowsHeader();

    std::function<void()> onUnfold;

    /** Adds the tempo's line to the legend, while the song's tempo changes. */
    void setShowsTempo (bool);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::TextButton arrowButton;
    bool showsTempo = false;
};

/** Shows where the track headers being dragged will land: an accent line between two rows (indented to the level it
    will land at), or an outline round a bus's row to put the tracks at the end of that bus. Draws over the rows and
    lets the mouse through. */
class TrackDropMarker final : public juce::Component
{
public:
    TrackDropMarker() { setInterceptsMouseClicks (false, false); }

    void showLine (juce::Rectangle<int> area);
    void showOutline (juce::Rectangle<int> area);
    void hideMarker();

    void paint (juce::Graphics&) override;

private:
    enum class Mode { none, line, outline };
    Mode mode = Mode::none;
};

class ArrangementView final : public juce::Component,
                              private juce::Timer,
                              private juce::ScrollBar::Listener,
                              private juce::ChangeListener
{
public:
    ArrangementView (Engine&, Selection&, PluginWindowManager&);
    ~ArrangementView() override;

    /** Called when the ruler's menu asks for the note lengths guide. */
    std::function<void()> onShowRhythmGuide;

    /** The project's name, for the files it saves (the chord track's PDF). */
    std::function<juce::String()> getProjectName;

    /** Called when a clip is double-clicked (or a MIDI clip created by double-clicking a lane). */
    std::function<void()> onOpenClipEditor;

    /** Opens a file chooser and imports onto the selected audio track (or a new one). */
    void importAudio();
    void importAudio (Track*, double beat);

    void rebuildTracks();

    /** Folds the Sections, Chords and Lyrics rows into thin coloured lines, or shows them in full;
        remembered between sessions. */
    void setSongRowsFolded (bool);
    bool areSongRowsFolded() const noexcept { return songRowsFolded; }

    /** Called when the rows are folded or shown in full, for the View menu's tick. */
    std::function<void()> onSongRowsFoldedChanged;

    /** Makes a karaoke version of the clip (Remove vocals), as its right-click menu does. */
    void removeVocals (const AudioClip&);

    /** Opens the play-along window: the chord playing now, large, with the next one beside it. */
    void showPlayAlong();

    /** Opens the sing-along window: the line to sing now, large, coloured as it's sung, to read while recording vocals. */
    void showSingAlong();

    /** The track menu and the bus commands, shared with the mixer, the device bar and the Track menu. */
    TrackActions& getTrackActions() noexcept { return trackActions; }

    /** Create bus from the selected tracks (Cmd+Shift+G): a bus with a suggested name, selected, its name open. */
    void createBusFromSelection() { trackActions.createBus(); }

    /** Called when something asks for the Buses, sends and routing guide, at a topic. */
    std::function<void (routingguide::Topic)> onShowRoutingGuide;

    /** Opens the How recording works guide at a topic (the track and clip menus, the Monitor warning). Not set: those items stay off. */
    std::function<void (recordingguide::Topic)> onShowRecordingGuide;

    /** Set by the owner: opens the *How drum triggers work* guide at a topic (from the Hits to MIDI window). */
    std::function<void (triggerguide::Topic)> onShowTriggerGuide;

    /** Hits to MIDI: the windows open on audio clips (for the guide's *Your clip* and the screenshots). */
    HitsToMidiActions& getHitsToMidi() noexcept { return hitsToMidi; }

    /** Opens the chord, piano-and-guitar, structure or lyrics guide from the Theory Guides window, at a topic of its
        entry in gen::guides (-1 for where it is). The chord and structure guides start at the playhead's bar, and
        "How to play" shows the chord there (C when there is none). */
    void showGuide (gen::guides::Guide, int topic);

    /** The chord, piano-and-guitar, structure or lyrics guide for the PDF export: the open window's content, or one
        made for the export (the piano-and-guitar one is always made, with a page for each of the song's chords). */
    guideprint::Item getPrintGuide (gen::guides::Guide);

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int headerWidth = 270;

private:
    friend class ScreenshotRunner;   // the --screenshots developer flag sets up the rows and the zoom

    void timerCallback() override;
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void layoutContent();

    /** Shows the Tempo row when the song's tempo changes, and hides it when it's steady. */
    void updateTempoRow();
    int getSongRowsHeight() const;
    void showChordGuide (int bar);
    void showStructureGuide (int bar);
    void showLyricsGuide();
    void showLyricsEditor();
    void showSingingGuide (gen::singing::Topic = gen::singing::Topic::voice);
    int getPlayheadBar() const;
    void updateScrollBar();
    void setViewStart (double beat);
    void zoomAround (double factor, double anchorBeat);
    void deleteTracks (std::vector<Track*>, bool withItsTracks);
    double getTotalBeats() const;

    // Dragging track headers up, down and into buses.
    struct DropTarget
    {
        bool valid = false;
        routing::Place place;
    };

    std::vector<Track*> getDraggedTracks (Track& clicked) const;
    void dragTracks (TrackHeader&, const juce::MouseEvent&);
    void dropTracks (TrackHeader&);
    void cancelTrackDrag();

    Engine& engine;
    Selection& selection;
    PluginWindowManager& pluginWindows;

    TimelineView timeline;
    Ruler ruler { engine, timeline };
    TempoLane tempoLane { engine, timeline };
    RowLabel tempoLabel { "Tempo", "The song's tempo bar by bar, following a recording played without a click. "
                                   "Click to open the guide to note lengths, beats and tempo" };
    bool tempoShown = false;      // the Tempo row, while the tempo changes and the rows aren't folded
    SectionLane sectionLane { engine, timeline };
    ChordLane chordLane { engine, timeline };
    RowLabel sectionsLabel { "Sections", "Click to open the song structure guide: how songs are built, and the usual structures per style" };
    ChordTrackHeader chordsHeader { chordLane };
    LyricsLane lyricsLane { engine, timeline };
    LyricsTrackHeader lyricsHeader { lyricsLane };
    juce::TextButton foldButton;   // left of the Sections label
    FoldedSongRows foldedRows { engine, timeline };
    FoldedSongRowsHeader foldedHeader;
    bool songRowsFolded = false;
    int seenRecordingReport = 0;   // the last recording whose messages were shown
    juce::Viewport viewport;
    juce::Component content;
    TrackActions trackActions { engine, selection, pluginWindows, *this };
    TrackDropMarker dropMarker;
    DropTarget dropTarget;
    juce::OwnedArray<TrackHeader> headers;
    int shownEditGeneration = -1;   // headers are rebuilt when the engine's edit is replaced
    TrackLanes lanes { engine, selection, timeline };
    StemSplitter stemSplitter { engine, *this };
    SingActions singActions { engine, selection, pluginWindows, *this };
    AudioToMidiConverter audioToMidi { engine, *this };
    HitsToMidiActions hitsToMidi { engine, selection, pluginWindows, *this };
    LyricsDetector lyricsDetector { engine, *this };
    TempoDetector tempoDetector { engine, *this };
    ChordDetector chordDetector { engine, *this };
    StructureDetector structureDetector { engine, *this };
    ChordStylePlayer chordStyles { engine, *this };
    juce::ScrollBar hScroll { false };
    juce::TextButton zoomInButton { "+" }, zoomOutButton { "-" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<ChordExplainer> chordExplainer;   // its window, while open
    juce::Component::SafePointer<StructureExplainer> structureExplainer;
    juce::Component::SafePointer<LyricsExplainer> lyricsExplainer;
    juce::Component::SafePointer<LyricsEditWindow> lyricsEditor;
    juce::Component::SafePointer<SingingExplainer> singingExplainer;
    juce::Component::SafePointer<ChordPlayAlong> playAlong;
    juce::Component::SafePointer<LyricsSingAlong> singAlong;

    static constexpr int rulerHeight = 26, scrollBarHeight = 12;
};
} // namespace daw
