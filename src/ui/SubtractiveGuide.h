#pragma once

#include "engine/BuiltInSubtractive.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Subtractive: what each knob and part of its window does, with
    live numbers, the "What is subtractive synthesis?" guide, and the pictures both use: the
    harmonics of the oscillators with the filter over them, the wave before and after the filter,
    the envelopes, the LFO and the path the sound takes.
*/
namespace daw::subtext
{
/** The pictures; the knobs are 0 up (sub::Knob), the shared topics fxui::CommonTopic. */
enum Topic { spectrum = 50, wave, envelopes };

inline const juce::Colour colour { 0xff9b8cff };         // the synth, and the sound after the filter
inline const juce::Colour filterColour { 0xffffb454 };   // the filter and its envelope
inline const juce::Colour ampColour { 0xff5fd38d };      // the amp envelope
inline const juce::Colour lfoColour { 0xff5cc8ff };      // the LFO
inline const juce::Colour secondColour { 0xffff8fb1 };   // oscillator 2

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which sounds. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, for the test note A3 (220 Hz). */
juce::String describeSettings (int topic, const sub::Settings&);

//==============================================================================
/** The harmonics of the oscillators at `noteHz` as bars on a frequency scale (20 Hz to 20 kHz):
    grey before the filter, coloured after it, with the filter's curve at `cutoffHz` over them. */
void drawSpectrum (juce::Graphics&, juce::Rectangle<float>, const sub::Settings&, double noteHz, double cutoffHz, bool labels);

/** A few cycles of the note before the filter (grey) and after it (coloured). */
void drawWave (juce::Graphics&, juce::Rectangle<float>, const sub::Settings&, double noteHz, double cutoffHz, double cycles);

/** An ADSR envelope's shape, with the four stages named, and a dot where a note `age` seconds old
    (let go `releasedAt` seconds after its start, or still held when negative) is now; no dot when age is negative. */
void drawEnvelope (juce::Graphics&, juce::Rectangle<float>, const sub::Envelope&, juce::Colour, const juce::String& name,
                   double age, double releasedAt, bool labels);

/** The path of the sound: oscillators and noise, mixer, filter, amp, out, with the envelopes and the
    LFO that move them; the part `highlight` (0 oscillators, 1 mixer, 2 filter, 3 amp, 4 envelopes,
    5 LFO, -1 none) lit. */
void drawFlow (juce::Graphics&, juce::Rectangle<float>, const sub::Settings&, int highlight);

/** One cycle of a wave and its first harmonics as bars. */
void drawOscillator (juce::Graphics&, juce::Rectangle<float>, sub::Wave, float widthPercent, juce::Colour);

/** The LFO's wave over a few seconds, moving `seconds` along, and what it does to its target. */
void drawLfo (juce::Graphics&, juce::Rectangle<float>, const sub::Settings&, double seconds);

/** The pages of "What is subtractive synthesis?", with pictures that follow the synth's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInSubtractive&);
} // namespace daw::subtext
