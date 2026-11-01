#pragma once

#include "engine/BuiltInSlicer.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Slicer: what each control and part of its window does, with
    live numbers, the "What is a slicer?" guide, and the pictures both use: the pattern of steps
    over the song's beats, and the last 2 bars as the slicer heard them.
*/
namespace daw::slctext
{
/** The parts of the window with a topic of their own; the knobs are 0 up (slc::Knob), the shared topics fxui::CommonTopic.
    The four mode buttons are `modeButton` plus the mode. */
enum Topic { pattern = 50, lastBars, modeButton };

inline const juce::Colour colour { 0xfff2994a };

/** What a mode is drawn in: the track's own grey, the effect's orange, blue for backwards, and a dark gap. */
juce::Colour getModeColour (slc::Mode);

juce::String getTitle (int topic);

/** What a control or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a control. */
juce::String getTypical (int topic);

/** What these settings mean in numbers, at the tempo the slicer uses. */
juce::String describeSettings (int topic, const slc::Settings&, double bpm);

//==============================================================================
/** The step (0 is the first) that a Repeat or Reverse step plays from: the step just before its run of Repeat and Reverse
    steps, counted round the pattern; -1 for any other step, or when every step is a Repeat or Reverse. */
int getSliceSource (const slc::Settings&, int step);

/** Where the 16 cells of the pattern picture sit: a row, with room above for the arcs and below for the labels. */
struct PatternGrid
{
    juce::Rectangle<float> cells;
    float cellWidth = 1.0f;

    juce::Rectangle<float> getCell (int step) const { return { cells.getX() + cellWidth * (float) step, cells.getY(), cellWidth, cells.getHeight() }; }

    /** The step under a point, or -1 outside the row. */
    int getStepAt (juce::Point<float>) const;
};

PatternGrid layoutPattern (juce::Rectangle<float> bounds);

/** Draws the pattern over the beat and bar lines, the playing step lit (-1 for none) and the hovered one (-1) outlined. */
void drawPattern (juce::Graphics&, const PatternGrid&, const slc::Settings&, int playingStep, int hoveredStep, bool labels);

/** "Step 6 of 16 · beat 2, 2nd sixteenth · Repeat: plays step 5 again (125 ms)": what a cell does. `sliceMissing` says it is
    a Repeat or Reverse step that has nothing to play yet. */
juce::String getCellDescription (const slc::Settings&, int step, double bpm, bool sliceMissing);

/** The last 2 bars against the beats: per step, how loud the track came in (grey) and what came out (in the step's colour), with its mode below. */
void drawLastBars (juce::Graphics&, juce::Rectangle<float>, const std::vector<slc::StepRecord>&, const slc::Settings&);

/** The steps of the last 2 bars, oldest first (at most 64). */
std::vector<slc::StepRecord> getLastBars (BuiltInSlicer&);

/** The pages of "What is a slicer?", with pictures that follow the slicer's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInSlicer&);
} // namespace daw::slctext
