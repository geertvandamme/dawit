#pragma once

#include "engine/Engine.h"
#include "generate/ReadingGuide.h"
#include "GuidePrint.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    A guide to reading sheet music for beginners, opened from the score's header, its right-click menu and the
    sheet-music export dialog: the staff, the treble and bass clefs, note names, ledger lines, the grand staff,
    sharps and flats, key signatures, time and tempo, note values, ties, chord symbols and how DawIt writes
    your MIDI. Each topic has a picture or several, drawn by `ScorePainter` from an example score (with a piano
    keyboard under the notes of the first topics, and the MIDI blocks as played above the score for ties and
    rounding). Click a note in a picture to hear it; Play sounds the topics that have a rhythm and lights each
    note as it plays. The last topic writes out the selected MIDI track (or the first with notes) and explains it.
*/
class ReadingExplainer final : public juce::Component,
                               public guideprint::Source,
                               private juce::ListBoxModel,
                               private juce::ChangeListener,
                               private juce::Timer
{
public:
    ReadingExplainer (Engine&, Selection&);
    ~ReadingExplainer() override;

    /** Selects a topic in the list (from the Theory Guides window or a guide button). */
    void showTopic (gen::readmusic::Topic);

    juce::String getPrintTitle() const override { return "How to read music"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    gen::readmusic::Part makePart() const;
    void show (int index);
    void play();
    void stop();

    Engine& engine;
    Selection& selection;
    int shownRow = 0;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    juce::TextButton playButton { "Play" };
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::readMusic };
    int rowBeforePrinting = -1;
    int printHeight = 0;

    // Playback of the shown example, once through.
    std::vector<MidiNote> playNotes;
    double playStartMs = 0.0, lastBeats = -1.0, playTotal = 0.0;
    bool playing = false;

    static constexpr int width = 1000, height = 740;
};
} // namespace daw
