#pragma once

#include "engine/Engine.h"
#include "generate/KeyFindingGuide.h"
#include "GuidePrint.h"
#include "GuideFooter.h"
#include "GuideSourcesPanel.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The guide *Finding the key and chords*, opened from a MIDI clip's right-click menu (Detect key and chords...), the
    piano roll's Key and chords button and the Theory Guides window: what a key is, major and minor scales, the modes,
    harmonic and melodic minor, pentatonic and blues scales, how DawIt finds the key, how a chord is named from its
    notes, inversions and slash chords, sus and diminished chords, and the notes between the chords. Each topic has a
    picture: a keyboard and the note names of each scale or chord (click one to hear it), or a profile or the notes of
    a short example part. The last topic, *Your clip*, analyses the selected MIDI clip: the clip's bars with a chord
    above each (and its Roman numeral), the notes coloured by whether they are in the chord, outside the scale or
    neither, and the note-name profile. Under it, three buttons: put the chords on the chord track (asking before
    replacing chords already there), set the song's key to the one found, and show only the notes of that key on the
    clip's track. It follows the selection and the edits.
*/
class KeyFindingExplainer final : public juce::Component,
                                  public guideprint::Source,
                                  private juce::ListBoxModel,
                                  private juce::ChangeListener
{
public:
    KeyFindingExplainer (Engine&, Selection&);
    ~KeyFindingExplainer() override;

    /** Selects a topic in the list (from the Theory Guides window, the clip menu or the piano roll). */
    /** Opens the Sources topic, the last in the list. */
    void showSources();

    void showTopic (gen::keyfinding::Topic);

    /** What *Your clip* shows: the selected MIDI clip's analysis, or why there is none. */
    struct YourClip
    {
        bool found = false;                     // a MIDI clip is selected (or found on the selected track)
        bool drums = false;                     // its track plays drum sounds: no pitches
        juce::String trackName;
        Track* track = nullptr;                 // valid only straight after `analyseSelection()`
        analysis::midiharmony::Input input;
        analysis::midiharmony::Result result;
        std::optional<MusicalKey> numeralKey;   // the key the numerals are written in
    };

    juce::String getPrintTitle() const override { return "Finding the key and chords"; }
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

    /** The selected MIDI clip (or, with only a MIDI track selected, its clip under the playhead, else its first) analysed. */
    YourClip analyseSelection() const;

    void show (int index);
    void updateActions();
    void putOnChordTrack();
    void setSongKey();
    void toggleScaleFilter();
    void setStatus (const juce::String&);

    Engine& engine;
    Selection& selection;
    int shownRow = 0;
    YourClip yours;

    juce::Label title, status;
    juce::ListBox list;
    GuideSourcesTopic sources { *this, "keyFinding" };
    GuideFooter footer;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    juce::TextButton putButton { "Put chords on chord track" }, keyButton { "Set the song's key" },
                     filterButton { "Show only notes in the key" };
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::keyFinding };
    int rowBeforePrinting = -1;
    int printHeight = 0;

    static constexpr int width = 1000, height = 780;
};
} // namespace daw
