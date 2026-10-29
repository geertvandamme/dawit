#pragma once

#include "ChordDiagramViews.h"
#include "engine/Engine.h"
#include "generate/ChordGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    An interactive guide to chord progressions for beginning musicians, opened from the chord track's menu.
    It shows the chords of a key (the song's key unless another is picked) by family: home, away and tension.
    Click a chord to hear it and read what it does, with how to play it on piano (inversions) and guitar
    (chord boxes); pick a well-known progression or the song's own chord track to hear it and read why it
    works, see it on piano or guitar, and put it on the chord track. The circle of fifths shows how keys and
    their chords are related, and "Playing along" gives tips for playing along with the song's chords in the
    play-along window. Chords play on the selected MIDI track.
*/
class ChordExplainer final : public juce::Component,
                             public guideprint::Source,
                             private juce::ListBoxModel,
                             private juce::ChangeListener,
                             private juce::Timer
{
public:
    ChordExplainer (Engine&, int startBar);
    ~ChordExplainer() override;

    /** The first bar "Put on chord track" writes to. */
    void setStartBar (int bar);

    /** Shows how to play a chord on piano and guitar (from the chord track's menu). */
    void showHowToPlay (const Chord&);

    /** Shows the topic on playing along with the song's chords (from the play-along window). */
    void showPlayingAlong();

    // The list: an introduction, the circle of fifths, the song's chord track, playing along with it,
    // then the well-known progressions.
    enum { introRow = 0, circleRow = 1, chordTrackRow = 2, playAlongRow = 3, firstProgressionRow = 4 };

    /** Selects a row of the list (from the Theory Guides window). */
    void showTopic (int row);

    /** Makes this the "Piano and guitar chords" guide of a booklet: a page for each chord of the song's chord track
        (each once, in order of first use; the key's seven chords when the track is empty), with how to play it. */
    void setPrintChordShapes (bool);

    juce::String getPrintTitle() const override { return printChordShapes ? "Piano and guitar chords" : "Chord progressions and the circle of fifths"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    /** Called when "Play along..." is clicked: opens the play-along window. */
    std::function<void()> onPlayAlong;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class ChordTile;
    class CircleView;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void setKey (const MusicalKey&);
    void updateKeyControls();
    void rebuildTiles();
    void showRow (int row);
    void showChord (const Chord&, const Chord* previous = nullptr);
    void updateHint();

    /** A row of the list: its name and the line under it. */
    std::pair<juce::String, juce::String> getRowText (int row) const;

    /** The chords a "Piano and guitar chords" booklet has a page for. */
    std::vector<Chord> getPrintChords() const;

    /** The parts of the picture, in this component's coordinates, top to bottom. */
    std::vector<juce::Rectangle<int>> getPrintParts() const;

    std::vector<Chord> getChordTrackChords() const;
    std::vector<Chord> getShownChords() const;   // of the selected row: one per bar, repeats kept

    void playChord (const Chord&);
    void playNotes (const std::vector<int>& notes, bool strum);
    void playAt (size_t index);
    int getChordMs() const;
    void play (const std::vector<Chord>&);
    void stop();
    void highlight (const Chord*);
    void putOnChordTrack();

    Engine& engine;
    int startBar = 0;
    MusicalKey key;
    bool followSongKey = true;   // until another key is picked here

    std::vector<gen::guide::Progression> progressions;

    juce::Label title, keyLabel { {}, "Key" }, hintLabel;
    juce::ComboBox tonicBox, scaleBox;
    juce::TextButton songKeyButton;
    juce::OwnedArray<ChordTile> keyTiles, rowTiles;
    std::unique_ptr<CircleView> circle;   // shown with the circle of fifths row
    juce::ListBox list;
    juce::TextButton playButton { "Play" }, putButton, sheetButton { "Chord sheet (PDF)..." }, playAlongButton { "Play along..." };
    juce::ToggleButton slowToggle { "Slowly" };
    ChordDiagramPanel diagramPanel;
    juce::TextEditor explanation;
    juce::Rectangle<int> legendArea;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::chords,
                                            "Save this guide as a PDF to print or read on a tablet: a cover with the contents, then a page per topic with its picture and text "
                                            "(the Chord sheet button saves a key's chords to play from instead) (Cmd+P)" };
    bool printChordShapes = false;
    int rowBeforePrinting = -1;

    std::vector<Chord> playing;
    size_t playIndex = 0;

    static constexpr int width = 940, height = 800;
};
} // namespace daw
