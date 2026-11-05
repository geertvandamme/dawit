#pragma once

#include <juce_core/juce_core.h>

/**
    The pages of "What is a harmoniser?", DawIt Harmoniser's guide: their names, one-line summaries and
    texts, and the explanations of its controls, with no UI in them so the tests can read them. The window
    draws the pictures and adds the live text of *Your track*.
*/
namespace daw::gen::harmoniserguide
{
constexpr int numPages = 10;

/** "Intervals". */
juce::String getPageName (int page);

/** One line for the list: "How far apart two notes are, in semitones and in names". */
juce::String getPageSummary (int page);

/** What the page says (for the last page, the part before the advice on the track). */
juce::String getPageText (int page);

/** The controls with a tooltip and an explainer topic. */
enum class Control { harmony, key, scale, mix, on, intervalFixed, intervalKey, level, pan, detune, delay };

/** The explanation of a control, used as its tooltip and as the topic the knob group opens. */
juce::String getControlText (Control);

/** The paragraph added to the singing guide's Harmonies and doubles: it points here. */
juce::String getSingingParagraph();
}
