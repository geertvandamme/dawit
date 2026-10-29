#pragma once

#include "engine/Engine.h"
#include "Selection.h"
#include "Theme.h"

namespace daw
{
/**
    The Tempo row, under the ruler while the song's tempo changes (when it follows a recording bar
    by bar, see TempoMap). It draws the tempo as a line between the song's slowest and fastest
    tempo, with the BPM where there's room, and says the bar and its tempo under the mouse. A click
    moves the playhead, as on the other rows. The right-click menu makes the tempo steady again
    (the average) and opens the guide to note lengths, beats and tempo. Read-only for now.
*/
class TempoLane final : public juce::Component, public juce::TooltipClient, private juce::ChangeListener
{
public:
    TempoLane (Engine&, TimelineView&);
    ~TempoLane() override;

    static constexpr int height = 34;

    /** The tempo's colour, here and in the folded song rows. */
    static inline const juce::Colour colour { 0xffe8a54b };

    /** Whether the row is needed: the tempo changes somewhere in the song. */
    static bool isNeeded (const Engine& engine) { return ! engine.getTempoMap().isSteady(); }

    /** Called when "Note lengths, beats and tempo..." is picked. */
    std::function<void()> onShowRhythmGuide;

    /** Asks before making the tempo steady at the song's average, which stops it following the recording. */
    void makeSteady();

    void updatePlayhead();

    juce::String getTooltip() override;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    /** The beats the song's tempo covers: to the song's end, or the last change and a bar. */
    double getEndBeat() const;

    Engine& engine;
    TimelineView& timeline;
    float playheadX = -100.0f;
};
} // namespace daw
