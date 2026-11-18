#pragma once

#include "engine/Vocoder.h"

#include <juce_core/juce_core.h>

/**
    The words of DawIt Vocoder with no UI in them, so the tests can read them: the explanation of each control (its
    tooltip and the topic the knob group opens), how each knob's value is written, and the line the window shows about
    where the carrier's notes come from, and the pages of the guide What is a vocoder?.
*/
namespace daw::gen::vocoderguide
{
/** The pages of What is a vocoder?: their names, one-line summaries and texts (for the last, the part before the advice on the track). */
constexpr int numPages = 10;
juce::String getPageName (int page);
juce::String getPageSummary (int page);
juce::String getPageText (int page);

/** The controls with a tooltip and an explainer topic, in the order of `vocoder::Knob`. */
enum class Control { bands, smoothing, consonants, voice, low, high, source, wave, root, shape, range, carrier, formant, mix };

constexpr int numControls = 14;

/** "Bands", "Carrier level". */
juce::String getControlTitle (Control);

/** The explanation of a control, used as its tooltip and as the topic its knob opens. */
juce::String getControlText (Control);

/** What a knob reads: "16", "15 ms", "50%", "off" or "-18 dB", "8,000 Hz", "Song chord", "2 octaves", "+3 semitones". */
juce::String formatKnob (Control, const vocoder::Settings&);

/** "8,000 Hz", with a thousands comma. */
juce::String formatHz (double hz);

/** Where the effect sits, for the lead line: a track, a MIDI (instrument) track, a bus or a return. */
enum class HostKind { track, instrument, bus, returnTrack };

/** The line under the window's title about the carrier's notes (empty when all is as it seems): what the host means for the
    voice, then, for Keys, what to do or where the keys go; for Song chord, that the bar has no chord. */
juce::String getLeadLine (vocoder::Source, HostKind, bool armedElsewhere, bool usingFallback);

/** The name of a chord from its pitch classes ("Am", "C", "Bdim"), or its notes when it is none of the triads ("C E G♯ B"). */
juce::String getChordName (int pitchClassMask);

/** The label over the keyboard strip: "Song chord · Am", "Notes (no chord in this bar)", "Keys · C E G". */
juce::String getCarrierLabel (vocoder::Source, bool usingFallback, int carrierMask, int chordMask);

/** What a band says when the mouse rests on it: "Band 9 · 1,087 to 1,430 Hz · voice -21 dB · carrier -30 dB"; `db` values below -90 read "silent". */
juce::String describeBand (int band, double lowEdgeHz, double highEdgeHz, float voiceDb, float carrierDb);
}
