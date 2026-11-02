#pragma once

#include "engine/BuiltInStepFilter.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Step Filter: what each control and part of its window does, with
    live numbers, the "What is a step filter?" guide, and the pictures both use: the pattern of step
    heights over the song's beats, the filter at the cutoff playing now, and the last 2 bars as the
    step filter heard them.
*/
namespace daw::stftext
{
/** The parts of the window with a topic of their own; the knobs are 0 up (stf::Knob), the shared topics fxui::CommonTopic. */
enum Topic { pattern = 50, filterNow };

inline const juce::Colour colour { 0xff3fc7b4 };

juce::String getTitle (int topic);

/** What a control or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a control. */
juce::String getTypical (int topic);

/** The short text shown as a control's tooltip. */
juce::String getTooltip (int topic);

/** What these settings mean in numbers, at the tempo the step filter uses. */
juce::String describeSettings (int topic, const stf::Settings&, double bpm);

//==============================================================================
/** Where the 16 columns of the pattern picture sit: a row of bars, with room at the left for the Hz labels and below for the step numbers. */
struct PatternGrid
{
    juce::Rectangle<float> cells;
    float cellWidth = 1.0f;

    juce::Rectangle<float> getCell (int step) const { return { cells.getX() + cellWidth * (float) step, cells.getY(), cellWidth, cells.getHeight() }; }

    /** The step under a point, or -1 outside the row. */
    int getStepAt (juce::Point<float>) const;

    /** The height in percent that a y position stands for, kept between 0 and 100. */
    float getHeightAt (float y) const;
};

PatternGrid layoutPattern (juce::Rectangle<float> bounds);

/** Draws the pattern: a bar per step as high as its cutoff, over the beat and bar lines, the cutoff's path through the bars, the
    playing step lit (-1 for none) and the hovered one (-1) outlined. */
void drawPattern (juce::Graphics&, const PatternGrid&, const stf::Settings&, double bpm, int playingStep, int hoveredStep);

/** "Step 5 of 16 · beat 2, 1st sixteenth · 1.0 kHz (50%)": what a cell does. */
juce::String getCellDescription (const stf::Settings&, int step);

/** The filter at one cutoff, from 20 Hz to 20 kHz, with the Low to High range shaded and a dot at the cutoff. */
void drawFilterNow (juce::Graphics&, juce::Rectangle<float>, const stf::Settings&, double cutoffHz);

/** The last 2 bars against the beats: per step, how loud the track came in (grey) and what came out (in colour), with its cutoff below. */
void drawLastBars (juce::Graphics&, juce::Rectangle<float>, const std::vector<stf::StepRecord>&, const stf::Settings&);

/** The steps of the last 2 bars, oldest first (at most 64). */
std::vector<stf::StepRecord> getLastBars (BuiltInStepFilter&);

/** The pages of "What is a step filter?", with pictures that follow the step filter's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInStepFilter&);
} // namespace daw::stftext
