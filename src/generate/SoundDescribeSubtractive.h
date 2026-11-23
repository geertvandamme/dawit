#pragma once

#include "SoundDescribe.h"

namespace daw::gen::sounddescribe
{
/** DawIt Subtractive's describe table: its 28 knobs (sub::Knob order is the window's), 17 starting points,
    16 worked examples and its word and limit notes. */
const Table& getSubtractiveTable();

/** The synth's settings as a sound (one value per knob, in the knob's own units), and back. */
Values getSubtractiveValues (const sub::Settings&);
sub::Settings getSubtractiveSettings (const Values&);
} // namespace daw::gen::sounddescribe
