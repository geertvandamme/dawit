#pragma once

#include "engine/BuiltInGate.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Gate: what each knob and part of its window does, with live
    numbers, when and how a gate is used, the "What is a gate?" guide, and the pictures both use: the
    curve with its hysteresis loop, the last 4 seconds of the track as the gate heard them, and the
    key filters. The pictures run gate::simulate, the code the processor runs.
*/
namespace daw::gatetext
{
/** The pictures and the switches; the knobs are 0 up (gate::Knob), the shared topics fxui::CommonTopic. */
enum Topic { curve = 50, key, match, keyListen };

inline const juce::Colour colour { 0xff2ec4b6 };          // the gate: teal
inline const juce::Colour closingColour { 0xffff8a5c };   // the closing level
inline const juce::Colour removedColour { 0xffff5c5c };   // what it turns down

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which sounds. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, with the track's last 4 seconds and the song's tempo (0 when unknown). */
juce::String describeSettings (int topic, const gate::Settings&, const std::vector<gate::Moment>& recent, double bpm);

/** A level in whole dB, as the advice and the leads write it: "-40 dB". */
juce::String formatLevel (double db);

//==============================================================================
/** The square the curve is drawn in inside these bounds (with room for the numbers when `labels`). */
juce::Rectangle<float> getCurveSquare (juce::Rectangle<float> bounds, bool labels);

/** The curve: level in across, out up, -80 to 0 dB. The open line (the diagonal), the closed line (the
    range, or the ratio), the dashed opening level and closing level with arrows for the loop, and, when
    `dot` is given, the track's level and what comes out of it, with a trail of earlier ones. Returns the square. */
juce::Rectangle<float> drawCurve (juce::Graphics&, juce::Rectangle<float> bounds, const gate::Settings&, bool labels,
                                  std::optional<std::pair<float, float>> dot = std::nullopt, const std::vector<std::pair<float, float>>& trail = {});

/** Where the dots of the curve are, in the square `drawCurve` returned: the threshold, the closing level
    and the range. */
struct CurveDots
{
    juce::Point<float> threshold, closing, range;
};

CurveDots getCurveDots (juce::Rectangle<float> square, const gate::Settings&);

/** The level in dB that an x or y position of the curve's square stands for. */
float curvePositionToDb (juce::Rectangle<float> square, float position, bool vertical);

/** The track's last moments: what came in (grey), what went out (colour), how much it was turned down
    (red, from the top), the threshold and closing level, and a bar along the bottom while open. With a
    tempo, the beats; with an analysis, the noise floor and the loud parts. */
void drawMoments (juce::Graphics&, juce::Rectangle<float> bounds, const std::vector<gate::Moment>&, const gate::Settings&, bool labels,
                  double bpm = 0.0, const gate::Analysis* analysis = nullptr);

/** What the gate hears of each frequency with these key filters, with the ranges of a kick, a snare
    and cymbals marked. The handles are drawn when `handles` is set. */
void drawKeyFilters (juce::Graphics&, juce::Rectangle<float> bounds, const gate::Settings&, bool labels, bool handles);

/** The frequency at an x position of the key picture's plot area, and back. */
double keyPositionToHz (juce::Rectangle<float> plot, float x);
float keyHzToPosition (juce::Rectangle<float> plot, double hz);

/** The plot area `drawKeyFilters` uses inside its bounds. */
juce::Rectangle<float> getKeyPlot (juce::Rectangle<float> bounds, bool labels);

/** The pages of "What is a gate?", with pictures that follow the gate's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInGate&);
} // namespace daw::gatetext
