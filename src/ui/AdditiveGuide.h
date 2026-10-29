#pragma once

#include "engine/BuiltInAdditive.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Additive: what each knob and part of its window does, with live
    numbers, the "What is additive synthesis?" guide, and the pictures both use: the harmonics as bars,
    the wave they add up to, a note's harmonics over time, the harmonic series as notes, stretched
    partials and the Hammond's drawbars.
*/
namespace daw::additext
{
/** The pictures; the knobs are 0 up (additive::Knob), the shared topics fxui::CommonTopic. */
enum Topic { partials = 50, wave, overTime };

inline const juce::Colour colour { 0xff4fd1c5 };       // the synth, and the odd harmonics
inline const juce::Colour evenColour { 0xffff8fb1 };   // the even harmonics
inline const juce::Colour moveColour { 0xffffb454 };   // the swell and fade
inline const juce::Colour ampColour { 0xff5fd38d };    // the amp envelope
inline const juce::Colour lfoColour { 0xff5cc8ff };    // the LFO

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which sounds. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, for the test note A3 (220 Hz). */
juce::String describeSettings (int topic, const additive::Settings&);

/** How loud the test note peaks with these settings at full velocity, after the output knob, in dB. */
double getPeakDb (const additive::Settings&);

/** "A3", "C#6": a MIDI note's name, with middle C (60) as C4. */
juce::String getNoteName (int note);

//==============================================================================
/** Where the bars go inside the area drawPartials is given, for finding the bar under the mouse. */
juce::Rectangle<float> getPartialsPlot (juce::Rectangle<float>);

/** The harmonic (1 to 32) under `x` in the plot, and the level in dB at `y` (offDb at the bottom, 0 at most). */
int getPartialAt (juce::Rectangle<float> plot, float x);
float getDbAt (juce::Rectangle<float> plot, float y);

/** The harmonics as bars: the outline the level drawn, the colour what's heard after the brightness,
    the even harmonics, the velocity, the LFO (-1 to 1) and `envelope` (each harmonic's swell and fade
    now, or none), odd ones teal and even ones pink. Bars `lit` returns true for stand out. */
void drawPartials (juce::Graphics&, juce::Rectangle<float>, const additive::Settings&, const std::array<float, additive::numPartials>* envelope,
                   float velocity, float lfo, std::function<bool (int)> lit = nullptr);

/** A few cycles of a note at `noteHz`, `seconds` after its start (no movement when negative): all the
    harmonics added up, coloured, and the 1st alone, grey. */
void drawWave (juce::Graphics&, juce::Rectangle<float>, const additive::Settings&, double noteHz, double seconds, double cycles);

/** A note over time: a row per harmonic (the 1st at the bottom), brighter where louder, with the amp
    envelope over it; the key let go at `releasedAt` (or when the picture chooses, when negative) and a
    line where a note `age` seconds old is now (none when negative). */
void drawOverTime (juce::Graphics&, juce::Rectangle<float>, const additive::Settings&, double age, double releasedAt);

/** The partials of A3 as ticks on a line: where perfect harmonics would be, and where the stretch puts them. */
void drawStretch (juce::Graphics&, juce::Rectangle<float>, float stretchPercent);

/** The nine drawbars of a Hammond organ, pulled out as far as these settings' harmonics are loud. */
void drawDrawbars (juce::Graphics&, juce::Rectangle<float>, const additive::Settings&);

/** The pages of "What is additive synthesis?", with pictures that follow the synth's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInAdditive&);
} // namespace daw::additext
