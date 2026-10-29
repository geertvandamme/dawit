#pragma once

#include "engine/BuiltInFm.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt FM: what each knob and part of its window does, with live numbers,
    the "What is FM synthesis?" guide, and the pictures both use: the operators wired by their
    algorithm, the spectrum, the wave, the Bessel functions, and the operators' envelopes.
*/
namespace daw::fmtext
{
/** The pictures and the operator switch; the knobs are 0 up (fm::Knob), the shared topics fxui::CommonTopic. */
enum Topic { operators = 50, spectrum, wave, selectOperator };

inline const juce::Colour colour { 0xffa78bfa };              // the synth
inline const juce::Colour inharmonicColour { 0xffffb454 };    // partials between the harmonics
inline const juce::Colour lfoColour { 0xff5cc8ff };           // the LFO

/** Each operator's own colour, the same in every picture: 1 blue, 2 amber, 3 green, 4 pink. */
juce::Colour getOperatorColour (int op);

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which sounds. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, for the test note A3 (220 Hz); for an operator's knob, operator `op`'s. */
juce::String describeSettings (int topic, const fm::Settings&, int op);

/** What operator `op` does in these settings, in a line: "Operator 2: modulates 1 at 2 times the note, index 2.0". */
juce::String describeOperator (const fm::Settings&, int op);

/** How loud the test note peaks with these settings at full velocity, after the output knob, in dB. */
double getPeakDb (const fm::Settings&);

//==============================================================================
/** Where operator `op` sits in the picture of an algorithm drawn in `area`. */
juce::Rectangle<float> getOperatorBox (juce::Rectangle<float> area, int algorithm, int op);

/** The operator (0 to 3) whose box is under `point`, or -1. */
int getOperatorAt (juce::Rectangle<float> area, int algorithm, juce::Point<float> point);

/** The operators as boxes wired by the algorithm: modulators above what they modulate, the carriers
    into the output at the bottom, operator 4's feedback as a loop. Each box shows its number, ratio and
    level, glowing as loud as `envelopes` say (none: as set); `selected` (or -1) is outlined. `detailed`
    adds each one's index and envelope, for the window. */
void drawAlgorithm (juce::Graphics&, juce::Rectangle<float>, const fm::Settings&, int selected, const std::array<float, fm::numOperators>* envelopes,
                    bool detailed);

/** Partials as bars on a line from 0 Hz to `maxHarmonic` times the note (so the even spacing of the
    sidebands shows), in dB from 0 to -60: harmonics in the synth's colour, partials between them in
    amber. With settings, a tick under each operator's frequency. */
void drawSpectrum (juce::Graphics&, juce::Rectangle<float>, const std::vector<fm::Peak>&, double noteHz, double maxHarmonic,
                   const fm::Settings* operatorsOf = nullptr);

/** A few cycles of a note at `noteHz` with the operators' envelopes at `envelopes`: the sound coloured,
    and the carriers without modulation grey. */
void drawWave (juce::Graphics&, juce::Rectangle<float>, const fm::Settings&, double noteHz, const std::array<float, fm::numOperators>& envelopes,
               double cycles);

/** The Bessel functions J0 to J3 against the index from 0 to 10, with a line at `index`. */
void drawBessel (juce::Graphics&, juce::Rectangle<float>, double index);

/** Each operator's envelope that's on, over a note held and let go, in its colour, with a line where a
    note `age` seconds old is now (none when negative); held for `hold` seconds. */
void drawEnvelopes (juce::Graphics&, juce::Rectangle<float>, const fm::Settings&, double age, double hold);

/** How long the pictures hold a note before letting it go, from the envelopes. */
double getHoldSeconds (const fm::Settings&);

/** The pages of "What is FM synthesis?", with pictures that follow the synth's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInFm&);
} // namespace daw::fmtext
