#pragma once

#include <juce_core/juce_core.h>

/**
    The pages of "What is pitch correction?", DawIt Tune's guide: their names, one-line summaries and
    texts, with no UI in them so the tests can read them. The window draws the pictures
    (`tunetext::makeGuide` in src/ui/TuneGuide.cpp) and adds the live text of *Your track*.
*/
namespace daw::gen::tuneguide
{
constexpr int numPages = 10;

/** "Cents: how far out is out". */
juce::String getPageName (int page);

/** One line for the list: "100 cents to a semitone, and what ears notice". */
juce::String getPageSummary (int page);

/** What the page says (for the last page, the part before the advice on the track). */
juce::String getPageText (int page);
}
