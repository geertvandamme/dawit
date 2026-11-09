#pragma once

#include "engine/Engine.h"
#include "generate/InstrumentsGuide.h"
#include "GuidePrint.h"
#include "GuideFooter.h"
#include "GuideSourcesPanel.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/** What DawIt knows about a MIDI track for guessing the instrument it is for: its DawIt Sampler preset, name, built-in sound
    or DawIt instrument. Message thread. */
namespace instrumentsui
{
    gen::instruments::Hints getHints (Engine&, Track&);

    /** The guide's topic for a track: its guessed instrument's family (Brass for a trumpet), or *Your track* when unknown. */
    gen::instruments::Topic getTopicFor (Engine&, Track&);

    /** The colour of a family of instruments in the guide's pictures (never red: red marks notes out of range). */
    juce::Colour getFamilyColour (gen::instruments::Family);
} // namespace instrumentsui

/**
    The *Instruments in MIDI* guide: how each family makes its sound, its range and how it is played, and how to write
    it in MIDI. Each topic has a picture: tiles, standing waves, frequency rows, instrument ranges on one keyboard, or
    lines of notes with their velocities and controllers. Click a note or the end of a range bar to hear it; Play
    sounds the example line (the guitar strum, the bass line, the drum beat, the typed and played string line, the
    synth lead). The last topic checks the selected MIDI track (or the first with notes) against the instrument it's
    for: a dropdown picks the instrument, and starts as a guess from the track's DawIt Sampler preset, name or sound.
    Nothing is saved.
*/
class InstrumentsExplainer final : public juce::Component,
                                   public guideprint::Source,
                                   private juce::ListBoxModel,
                                   private juce::ChangeListener,
                                   private juce::Timer
{
public:
    InstrumentsExplainer (Engine&, Selection&);
    ~InstrumentsExplainer() override;

    /** Selects a topic in the list (from the Theory Guides window, a menu item or a guide button). */
    void showTopic (gen::instruments::Topic);

    /** Opens the Sources topic, the last in the list. */
    void showSources();

    juce::String getPrintTitle() const override { return "Instruments in MIDI"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void updateFooter();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    struct Captured;
    Captured capture() const;
    void fillChoice (const Captured&);
    void show (int index);
    void refreshYours();
    void play();
    void stop();

    Engine& engine;
    Selection& selection;
    int shownRow = 0;

    juce::Label title;
    juce::ListBox list;
    GuideSourcesTopic sources { *this, "instruments" };
    GuideFooter footer;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    juce::Label choiceLabel;
    juce::ComboBox choice;
    juce::TextButton playButton { "Play" };
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::instruments };
    int rowBeforePrinting = -1;
    int printHeight = 0;
    Track* choiceTrack = nullptr;       // the track the dropdown was last filled for; a new one resets it to Guess
    bool updatingChoice = false;

    // Playback of the shown example, once through, in beats of the example's own tempo.
    struct PlayEvent
    {
        double beat = 0.0;
        int pitch = -1;                 // -1: a controller or bend message
        float velocity = 0.8f;
        double lengthBeats = 0.0;
        juce::MidiMessage message;
    };

    std::vector<PlayEvent> playEvents;
    double playStartMs = 0.0, lastBeats = -1.0, playTotal = 0.0, playBpm = 120.0;
    bool playing = false;

    static constexpr int width = 1000, height = 760;
};
} // namespace daw
