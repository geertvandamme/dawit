#pragma once
#include "engine/BuiltInStereo.h"
#include "EffectWindow.h"

namespace daw::stereotext
{
/** The pages of What is stereo width?, following the effect's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInStereo&);
}
