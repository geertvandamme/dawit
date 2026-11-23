#pragma once

#include "SoundDescribe.h"
#include "engine/BuiltInAdditive.h"

namespace daw::gen::sounddescribe
{
/** DawIt Additive's describe table: its 17 knobs (indices 0 to 16) and 32 harmonic bars (17 to 48, one group
    that counts as a single change), 12 starting points with schema-safe names, 12 worked examples and its word
    and limit notes. */
const Table& getAdditiveTable();

/** The synth's settings as a sound (17 knobs, then the 32 bars in dB), and back. */
Values getAdditiveValues (const additive::Settings&);
additive::Settings getAdditiveSettings (const Values&);

/** The bars as a sentence for the prompt: "Saw shape" when they equal one of the shapes, else the bars that are
    on ("h1 0 dB, h2 -8 dB, the rest Off"). */
juce::String describeAdditiveBars (const additive::Settings&);
} // namespace daw::gen::sounddescribe
