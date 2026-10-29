#pragma once

#include "engine/BuiltInDelay.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Delay: what each control and part of its window does, with
    live numbers, the "What is delay?" guide, and the pictures both use: the echoes in time, and
    the wall that would give them.
*/
namespace daw::dlytext
{
/** The pictures in the window; the controls are 0 up (dly::Knob), the shared topics fxui::CommonTopic. */
enum Topic { echoes = 50, wall, sync };

inline const juce::Colour colour { 0xffb79cff };

juce::String getTitle (int topic);

/** What a control or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a control. */
juce::String getTypical (int topic);

/** What these settings mean in numbers, at the song's tempo (0: unknown). */
juce::String describeSettings (int topic, const dly::Settings&, double bpm);

//==============================================================================
/** Where echoes are drawn: time left to right from 0 to `seconds`, and two lanes, left above
    right, each with the level up from -60 to 0 dB (the dry sound). */
struct EchoPlot
{
    juce::Rectangle<float> left, right;
    double seconds = 2.0;

    float toX (double t) const { return left.getX() + left.getWidth() * (float) juce::jlimit (0.0, 1.0, t / seconds); }
    float toY (const juce::Rectangle<float>& lane, double db) const { return juce::jmap ((float) juce::jlimit (-60.0, 0.0, db), -60.0f, 0.0f, lane.getBottom(), lane.getY() + 2.0f); }
    double fromX (float x) const { return (x - left.getX()) / juce::jmax (1.0f, left.getWidth()) * seconds; }
};

/** One echo: when, on which side, how loud next to the dry sound, and how much the filters took off its highs. */
struct Echo
{
    double seconds = 0.0;
    bool onLeft = true, onRight = true;
    double db = 0.0, highsDb = 0.0;
};

/** The echoes of one click, until they are 60 dB down (at most `most`). */
std::vector<Echo> getEchoes (const dly::Settings&, double bpm, int most = 48);

/** How much time to show: a few echoes, between 1 and 8 seconds. */
double getShownSeconds (const dly::Settings&, double bpm);

/** Draws a click and its echoes, with the beats of the song. Returns where it drew. */
EchoPlot drawEchoes (juce::Graphics&, juce::Rectangle<float>, const dly::Settings&, double bpm, double seconds, bool labels);

/** You and the wall that would give this echo, with the distance on a scale of real places. For a
    while after a clap, the echoes come back to you as rings, in real time. */
void drawWall (juce::Graphics&, juce::Rectangle<float>, const dly::Settings&, double bpm, double secondsSinceClap, bool labels);

/** The pages of "What is delay?", with pictures that follow the delay's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInDelay&);
} // namespace daw::dlytext
