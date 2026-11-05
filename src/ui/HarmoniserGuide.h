#pragma once

#include "EffectWindow.h"
#include "engine/BuiltInHarmoniser.h"

namespace daw::harmtext
{
/** The pages of What is a harmoniser?, following the effect's settings and what it has heard. The song's key, sections
    and the playhead come from `tunetext::setSongAccess`, installed by MainComponent. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInHarmoniser&);

/** The advice lines of *Your track* for what the effect heard so far, with the made-up phrase's log when nothing was heard. */
juce::String getYourTrackText (BuiltInHarmoniser&);
}
