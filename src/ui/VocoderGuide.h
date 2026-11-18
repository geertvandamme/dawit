#pragma once

#include "EffectWindow.h"
#include "engine/BuiltInVocoder.h"

namespace daw::voctext
{
/** The pages of What is a vocoder?, following the effect's settings (bands, range, carrier) and, on the last page, what
    the effect has heard of the track. The test sound plays the made-up voice "ah ee s" through the effect itself. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInVocoder&);

/** The text of *Your track*: the page's introduction and the advice for what the effect heard in the last 8 seconds. */
juce::String getYourTrackText (BuiltInVocoder&);
}
