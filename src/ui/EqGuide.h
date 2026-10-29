#pragma once

#include "EffectWindow.h"
#include "EqExplainer.h"

/**
    "What is EQ?", the guide to equalising for beginners, opened from DawIt EQ's window and the Theory Guides window:
    what an EQ does, the frequency ranges, the band types, Q, slopes, cutting and boosting, where instruments sit,
    masking, common problems in words, match level, and how DawIt EQ works. The pictures are drawn from the EQ's own
    filters and follow the settings in its window.
*/
namespace daw::eqtext
{
/** The pages of "What is EQ?". */
std::vector<fxui::Guide::Page> makeGuide (BuiltInEq&);
} // namespace daw::eqtext
