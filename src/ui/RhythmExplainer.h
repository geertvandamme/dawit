#pragma once

#include "engine/Engine.h"
#include "generate/RhythmGuide.h"
#include "GuidePrint.h"
#include "GuideSourcesPanel.h"
#include "Theme.h"

namespace daw
{
/**
    A guide to rhythm for beginning musicians, opened from the piano roll (the ? next to Snap) and the
    ruler's menu: bars and beats, note values, rests, dotted notes and triplets, counting, tempo, and
    how they show in DawIt. Each topic shows example rhythms as one bar of blocks, like the piano
    roll, with the written note symbol and the count of every note; Play sounds the selected one
    against the beat on the selected MIDI track, lighting each note as it plays.
*/
class RhythmExplainer final : public juce::Component,
                              public guideprint::Source,
                              private juce::ListBoxModel,
                              private juce::ChangeListener,
                              private juce::Timer
{
public:
    explicit RhythmExplainer (Engine&);
    ~RhythmExplainer() override;

    /** Selects a topic in the list (from the Theory Guides window). */
    /** Opens the Sources topic, the last in the list. */
    void showSources();

    void showTopic (gen::rhythm::Topic);

    juce::String getPrintTitle() const override { return "Note lengths, beats and bars"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** One example rhythm: notes and rests over one bar. */
    struct Hit
    {
        double start = 0.0, length = 1.0;
        bool rest = false;
        float velocity = 0.8f;
        juce::String label;   // shown under it instead of its count
    };

    /** The written symbol at the start of a row. */
    struct Symbol
    {
        Symbol (double b = 0.0, bool isRest = false, bool isDotted = false, bool isTriplet = false)
            : beats (b), rest (isRest), dotted (isDotted), triplet (isTriplet) {}

        double beats;   // 0: none; else the plain value (4 whole ... 0.125 thirty-second)
        bool rest, dotted, triplet;
    };

    enum class Labels { count, seconds, position };

    struct Row
    {
        Row (juce::String n, juce::String d, Symbol s, std::vector<Hit> h, double beats = 4.0, Labels l = Labels::count, int barNumber = 1)
            : name (std::move (n)), detail (std::move (d)), symbol (s), hits (std::move (h)), beatsPerBar (beats), labels (l), bar (barNumber) {}

        juce::String name, detail;
        Symbol symbol;
        std::vector<Hit> hits;
        double beatsPerBar;   // the bar's length in quarter notes (4 for 4/4, 3 for 6/8, 3.5 for 7/8)
        Labels labels;
        int bar;   // the bar number positions are shown in

        // Rows of the metre topics: the grid follows the signature (strong lines where its groups start, bracketed above).
        bool bySignature = false;
        daw::TimeSignature signature = daw::timesig::fourFour;

        /** Where the beats the "Hear the beat" clicks fall: the signature's group starts, else every quarter note. */
        std::vector<double> getBeatStarts() const;
    };

    /** The example rhythms of a topic; the song's tempo and signature shape the topics about time and *Your song*. */
    static std::vector<Row> getRows (gen::rhythm::Topic, double bpm, daw::TimeSignature = daw::timesig::fourFour);

    /** What the advice on *Your song's time signature* looks at, read from the song (every clip's length and the start of every MIDI note). */
    static gen::rhythm::SongRhythm getSongRhythm (const Engine&);

private:
    class RowsView;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void showRow (int index);
    void play();
    void stop();

    Engine& engine;

    juce::Label title, hintLabel;
    juce::ListBox list;
    GuideSourcesTopic sources { *this, "rhythm" };
    std::unique_ptr<RowsView> rowsView;
    juce::TextButton playButton { "Play" };
    juce::ToggleButton beatToggle { "Hear the beat" };
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::rhythm };
    int rowBeforePrinting = -1;

    // Playback of the selected example: twice through its bar.
    std::vector<Row> rows;
    double playStartMs = 0.0, lastBeats = -1.0;
    int playingRow = -1;

    static constexpr int width = 960, height = 720, timesThrough = 2;
};
} // namespace daw
