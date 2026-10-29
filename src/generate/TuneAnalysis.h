#pragma once

#include <juce_core/juce_core.h>

#include "engine/MusicalKey.h"
#include "engine/Section.h"
#include "engine/TempoMap.h"
#include "engine/Tune.h"

/**
    "Your track" in the pitch correction guide: what DawIt Tune heard of a sung take, turned into
    notes, how far each was from its note, and advice in plain words. Shared by the guide and
    (later) practising singing. No UI.
*/
namespace daw::gen::tunecheck
{
/** A sung note: a run of voiced frames that stays on one semitone, at least 150 ms long. */
struct Note
{
    int midi = 0;                 // the nearest semitone
    double startBeat = 0.0, endBeat = 0.0;
    double startSeconds = 0.0, endSeconds = 0.0;
    double errorCents = 0.0;      // the median cents from that semitone after the first 80 ms; below 0 is flat
    bool inKey = true;
    int bar = 0;                  // bar 0 is the first
    int section = -1;             // index in the song's sections, or -1
};

/** The notes of one section and how far they sit from their notes on average. */
struct SectionSummary
{
    int section = 0, numNotes = 0;
    double meanCents = 0.0;
};

/** What the log makes of a take. */
struct Analysis
{
    std::vector<Note> notes;
    std::vector<std::pair<int, int>> phrases;   // the first note and the number of notes of each phrase (split by half a second)
    std::vector<SectionSummary> sections;
    double within25 = 0.0, within10 = 0.0;      // the share of notes (0 to 1)
    double meanCorrectionCents = 0.0;           // how far the correction moved voiced frames, on average (always positive)
    double unclearShare = 0.0, unclearSeconds = 0.0;   // of the frames louder than -40 dBFS: those with no clear pitch
};

/** Notes shorter than this aren't counted, and the first part of a note is left out of its error (the slide in). */
constexpr double minNoteSeconds = 0.15, skipSeconds = 0.08, bridgeSeconds = 0.05, phraseGapSeconds = 0.5;

/** Finds the notes in what was heard. Frames with a song position of -1 are ignored. */
Analysis analyse (const std::vector<tune::Frame>&, const TempoMap&, const MusicalKey&, const SectionList&);

/** The advice for a take, one paragraph each. With fewer than 8 notes it says to play the song through. */
std::vector<juce::String> getAdvice (const std::vector<tune::Frame>&, const TempoMap&, const MusicalKey&, const SectionList&, bool correcting);

/** The same from an analysis already made. */
std::vector<juce::String> getAdvice (const Analysis&, const MusicalKey&, const SectionList&, bool correcting);

/** "bar 17", "bars 17 and 18", "bars 17, 18 and 21" (bars given from 0, shown from 1). */
juce::String formatBars (const std::vector<int>& bars);
}
