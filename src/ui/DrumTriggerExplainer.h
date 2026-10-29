#pragma once

#include <functional>
#include <optional>

#include "engine/DrumTriggerGuide.h"
#include "engine/Engine.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    The "How drum triggers work" guide: what a drum trigger is, transients, finding the hits, velocity, telling
    kick, snare and hi-hat apart, false and missed hits, the grid, replacing or layering with the phase lined up,
    and a look at your own hits. Each topic has a picture drawn from the example drum loop (its waveform, level,
    spectra, and the Sensitivity, Velocity and gap you set in the Hits to MIDI window); clicking the loop, the snare
    hit or the two kicks plays them. "Your clip" reads the open Hits to MIDI window, or the selected drum clip.
    Opened from the ? in the Hits to MIDI window and from an audio clip's right-click menu.
*/
class DrumTriggerExplainer final : public juce::Component,
                                   public guideprint::Source,
                                   private juce::ListBoxModel,
                                   private juce::Timer
{
public:
    /** What "Your clip" looks at: the hits of the open Hits to MIDI window, or a selected drum clip; none when
        there is nothing to look at. */
    using PartProvider = std::function<std::optional<triggerguide::Part>()>;

    DrumTriggerExplainer (Engine&, PartProvider);
    ~DrumTriggerExplainer() override;

    void showTopic (triggerguide::Topic);

    /** The part of a selected MIDI clip on a track that plays drums (DawIt Drums, or the Drums or Percussion
        sound), from its notes; none for any other clip. */
    static std::optional<triggerguide::Part> getPartForClip (const Engine&, const Track&, const MidiClip&);

    juce::String getPrintTitle() const override { return "How drum triggers work"; }
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
    void timerCallback() override;

    void show (int index);
    void refreshText();
    void refreshPicture();
    void play (int what);

    Engine& engine;
    PartProvider getPart;
    int shownRow = 0;
    int rowBeforePrinting = -1;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::triggers };

    static constexpr int width = 1000, height = 740, pictureHeight = 330;
};
} // namespace daw
