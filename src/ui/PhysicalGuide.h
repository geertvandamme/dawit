#pragma once

#include "engine/BuiltInPhysical.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Physical: what each knob and part of its window does, with live
    numbers, the "What is physical modelling?" guide, and the pictures both use: the string or tube
    with the exciter on it, the harmonics of a note, and how long each one rings.
*/
namespace daw::phystext
{
/** The pictures; the knobs are 0 up (phys::Knob), the shared topics fxui::CommonTopic. */
enum Topic { model = 50, harmonics, rings };

inline const juce::Colour colour { 0xffe0b26b };          // the synth: the string or tube
inline const juce::Colour exciterColour { 0xffff8fb1 };   // the exciter: the pick, hammer, bow or breath
inline const juce::Colour vibratoColour { 0xff5cc8ff };   // the vibrato

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob: what it is usually set to, for which sounds. */
juce::String getTypical (int topic);

/** What these settings do, in numbers, for the test note A3 (220 Hz). */
juce::String describeSettings (int topic, const phys::Settings&);

/** How loud the test note peaks with these settings at full velocity, after the output knob, in dB. */
double getPeakDb (const phys::Settings&);

/** The harmonics (up to the 24th) that playing at the position leaves out: "5, 10, 15 and 20", or empty. */
juce::String listMissing (const phys::Settings&);

//==============================================================================
/** A note of the test note A3, rendered once for the pictures and kept until the settings change, so they can
    measure its harmonics at any moment. */
class NoteCache
{
public:
    /** The note, held for `holdSeconds` and then let go, rendered at phys::getAnalysisRate. */
    const std::vector<float>& get (const phys::Settings&, double holdSeconds);

private:
    phys::Settings settings;
    double hold = -100.0;
    std::vector<float> sound;
};

//==============================================================================
/** Where the string or tube lies in a picture of the model drawn in `area`. */
juce::Rectangle<float> getResonatorArea (juce::Rectangle<float> area);

/** The position (in percent, from the left end) at `x` in a picture of the model. */
float getPositionAt (juce::Rectangle<float> area, float x);

/** The model: a string between its bridge and nut, or a tube from its closed end to its open end, with the exciter
    where it plays (a pick, a hammer, a bow, or a reed at the mouthpiece). The string moves with the harmonics the
    position excites, slowed down so the eye can follow, `phase` cycles of its fundamental in, as loud as `level`
    (0 to 1); in a tube the air's pressure does. `detailed` adds the ends' names, the position and the missing harmonics. */
void drawModel (juce::Graphics&, juce::Rectangle<float>, const phys::Settings&, double phase, float level, bool detailed);

/** Harmonics as bars at 1, 2, 3... times the note, in dB from -10 down to -80, those the resonator can't hold
    left out; `missing` marks the ones the position leaves out with a small ring. */
void drawHarmonics (juce::Graphics&, juce::Rectangle<float>, const std::vector<double>& levels, const phys::Settings&, bool missing);

/** How long each of the first `count` harmonics of A3 rings, held, as bars on seconds from 0.01 to 20 (spaced
    evenly by ratio), with a line at the release when `released`. */
void drawRingTimes (juce::Graphics&, juce::Rectangle<float>, const phys::Settings&, int count, bool released);

/** The pages of "What is physical modelling?", with pictures that follow the synth's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInPhysical&);
} // namespace daw::phystext
