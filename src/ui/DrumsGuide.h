#pragma once

#include "engine/BuiltInDrums.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Drums: what each knob and part of its window does, with live
    numbers, the "What is drum synthesis?" guide, and the pictures both use: the pads, a hit over time
    with its pitch, the tone's and the noise's spectrum, the 808's metal, and where each drum sits.
*/
namespace daw::drumtext
{
/** The pictures and the drum switch; the knobs are 0 up (drums::Knob), the shared topics fxui::CommonTopic. */
enum Topic { pads = 50, hit, spectrum, selectDrum };

inline const juce::Colour colour { 0xffff7a59 };        // the synth
inline const juce::Colour toneColour { 0xff5fd38d };    // the tone (the body of the drum)
inline const juce::Colour noiseColour { 0xffffb454 };   // the noise and the click
inline const juce::Colour pitchColour { 0xff5cc8ff };   // the pitch curve

/** Each drum's own colour, the same on its pad and in every picture. */
juce::Colour getDrumColour (drums::Drum);

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which drums. */
juce::String getTypical (int topic);

/** What these settings do, in numbers; for a drum's knob, drum `drum`'s. */
juce::String describeSettings (int topic, const drums::Settings&, drums::Drum);

/** What a drum is made of, in a line: "Kick: a sine falling from 200 to 50 Hz, -60 dB in 600 ms". */
juce::String describeDrum (const drums::Settings&, drums::Drum);

/** "C1", "A#2": a note's name, with middle C (60) as C4. */
juce::String getNoteName (int note);

//==============================================================================
/** The pads, three by three, each lit as loud as `levels` says (none: unlit); `selected` outlined. */
void drawPads (juce::Graphics&, juce::Rectangle<float>, drums::Drum selected, const std::array<float, drums::numDrums>* levels, bool detailed);

/** The pad under `point`, if any. */
std::optional<drums::Drum> getPadAt (juce::Rectangle<float>, juce::Point<float>);

/** One hit over its length: the tone (green) and the noise (amber) as how loud they are, the pitch
    curve over them when it bends, and a line where a hit `age` seconds old is now (none when negative). */
void drawHit (juce::Graphics&, juce::Rectangle<float>, const drums::Settings&, drums::Drum, double age, double seconds = -1.0);

/** The partials of a hit's first 170 ms, tone and noise apart, on a line of frequencies from 30 Hz to 16 kHz. */
void drawSpectrum (juce::Graphics&, juce::Rectangle<float>, const drums::Settings&, drums::Drum);

/** The pages of "What is drum synthesis?", with pictures that follow the synth's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInDrums&);
} // namespace daw::drumtext
