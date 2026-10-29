#pragma once

#include "engine/Engine.h"
#include "engine/MidiGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
class ComputerKeyboardTyping;

/**
    A guide to what MIDI is, for beginners, opened with "What is MIDI?" in the piano roll: instructions
    rather than sound, where it came from, the cable and its speed, messages byte by byte, note
    numbers and pitch, velocity, pedals and wheels, ticks and clock, MIDI files, and a look at the
    clip in the piano roll. Each topic has a picture that uses that clip as its example (a made-up
    one when no MIDI clip is shown): its notes as the bytes they're sent as, lit at the playhead
    while the song plays, its velocities, and its MIDI file byte by byte. Click a key of the 128-note
    keyboard to hear it on the clip's track and see the message it sends. The computer keyboard topic draws
    the strip's sixteen keys as they are now, and clicking one plays it.
*/
class MidiExplainer final : public juce::Component,
                            public guideprint::Source,
                            private juce::ListBoxModel,
                            private juce::Timer
{
public:
    /** The MIDI clip in the piano roll and its track, or nulls. */
    struct ClipView
    {
        const Track* track = nullptr;
        const MidiClip* clip = nullptr;
    };

    /** `typing` (may be null) is the computer keyboard handler whose live state the keyboard topic shows. */
    MidiExplainer (Engine&, std::function<ClipView()> getClipView, ComputerKeyboardTyping* typing = nullptr);
    ~MidiExplainer() override;

    void showTopic (midiguide::Topic);

    juce::String getPrintTitle() const override { return "What is MIDI?"; }
    juce::String getPrintSource() const override { return printSource; }
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** What the picture shows of the clip, copied from it (or the made-up example). */
    struct ClipData
    {
        bool isExample = true;
        juce::String name;
        std::vector<MidiNote> notes;
        double lengthBeats = 4.0;
        std::vector<midiguide::Event> events;
        juce::MemoryBlock file;
        std::vector<midiguide::FilePart> fileParts;
        double bpm = 120.0;
    };

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void timerCallback() override;

    /** Copies the clip again when it, its notes, the tempo or the key changed. */
    void updateClip (bool force);

    Engine& engine;
    std::function<ClipView()> getClipView;
    midiguide::Topic topic = midiguide::Topic::whatItIs;

    juce::Label title, hintLabel;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::midi };
    std::optional<midiguide::Topic> topicBeforePrinting;
    juce::String printSource = "a made-up example";

    juce::int64 shownSignature = -1;   // the clip, its notes, tempo and key that the picture and text show

    static constexpr int width = 1000, height = 790, pictureHeight = 340;
};
} // namespace daw
