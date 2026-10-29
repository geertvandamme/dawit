#pragma once

#include <vector>
#include <juce_core/juce_core.h>

#include "analysis/HitFinder.h"

namespace daw
{
/**
    The "How drum triggers work" guide: what a trigger is, transients, finding the hits with a sensitivity and a
    minimum gap, velocity, telling kick, snare and hi-hat apart, false and missed hits, the grid, replacing or
    layering with the phase lined up, and a look at the user's own hits. The content only: the window is
    DrumTriggerExplainer. The numbers behind the pictures (velocity curves, bleed, grid offsets, phase) and the
    advice rules are checked in the tests. The same rules give the Hits to MIDI window's status line.
*/
namespace triggerguide
{
    enum class Topic { whatItIs, transients, findingHits, velocity, whichDrum, falseHits, grid, replaceOrLayer, yourClip };
    inline constexpr Topic allTopics[] { Topic::whatItIs, Topic::transients, Topic::findingHits, Topic::velocity, Topic::whichDrum,
                                         Topic::falseHits, Topic::grid, Topic::replaceOrLayer, Topic::yourClip };

    juce::String getTopicName (Topic);
    juce::String getTopicSummary (Topic);

    // ---- Your clip ---------------------------------------------------------------------------------------------

    /** One hit of the part being looked at. */
    struct PartHit
    {
        double beat = 0.0;                                      // from the clip's start, which is taken to sit on the song's grid
        analysis::hits::Drum drum = analysis::hits::Drum::snare;
        int velocity = 100;                                     // 1 to 127
    };

    /** The hits of the open Hits to MIDI window, or the notes of a selected drum clip. */
    struct Part
    {
        std::vector<PartHit> hits;
        double lengthBars = 0.0;
        double bpm = 120.0;
        bool showDrums = true;     // the counts name the drums (sorted by drum, or drum notes)
        bool sameForAll = false;   // every velocity was set to 100: the "same velocity" advice stays quiet
    };

    /** The general text of a topic. For "your clip" it is the lines of `analyse`, one paragraph each, or, with
        no part (null, or no hits), how to get something to look at. */
    juce::String explain (Topic, const Part* = nullptr);

    // ---- The formulas and example data -------------------------------------------------------------------------

    /** Seconds between two hits of the same drum below which a pair is reported (30 ms). */
    inline constexpr double closePairMs = 30.0;

    /** Beats in `ms` milliseconds at `bpm`: 30 ms at 120 BPM is 0.06. */
    double getBeatsFromMs (double ms, double bpm);

    /** Milliseconds in `beats` at `bpm`. */
    double getMsFromBeats (double beats, double bpm);

    /** How long one cycle of `hz` lasts, in ms. 60 Hz: 16.67. */
    double getPeriodMs (double hz);

    /** The level of two equal waves of `hz`, the second `delayMs` late, added: 20 log10 |2 cos (pi f t)| in dB
        against one wave alone, so +6.02 in line; never below -120 (a full cancel). 60 Hz, 8.33 ms: below -40. */
    double getSumGainDb (double hz, double delayMs);

    /** The n-th (from 0) frequency a delay of `delayMs` cancels: (2n + 1) / (2 t). 1 ms: 500, 1500, 2500 Hz. */
    double getNotchHz (double delayMs, int n);

    /** The velocity curves' points at `dbBelowLoudest` for the picture: the same as analysis::hits::getVelocity. */
    int getCurveVelocity (analysis::hits::VelocityCurve, double dbBelowLoudest);

    /** One stroke of the bleed picture's snare recording. */
    struct SnareStroke
    {
        double beat = 0.0;
        double levelDb = 0.0;          // against the loudest backbeat
        enum class Kind { backbeat, ghost, bleed } kind = Kind::backbeat;
    };

    /** Snare hits at 0 dB on beats 1 and 3, ghost notes at -22 dB on 1.75 and 3.5, and the kick bleeding through at
        -26 dB on 0, 2 and 2.5. */
    const std::vector<SnareStroke>& getSnareTrack();

    /** How many strokes of the snare track count at a Sensitivity (a level in dB, below 0): 7 at -30, 4 at -24, 2 at -18. */
    int countKept (double sensitivityDb);

    /** How far off the 1/16 grid the picture's eight hits were played at 120 BPM, in ms: +12, -8, +30, +4, -20, +16, -4, +24. */
    const std::vector<double>& getGridOffsetsMs();

    /** What an offset is after snapping with `strength` (0 to 1): `offsetMs` x (1 - strength). 30 ms at 50%: 15. */
    double getSnappedOffsetMs (double offsetMs, double strength);

    // ---- Advice ------------------------------------------------------------------------------------------------

    /** "18 hits in 2 bars (9 a bar): 5 kick, 5 snare, 8 hi-hat." (the drums only when `showDrums`; empty without hits). */
    juce::String describeCounts (const Part&);

    /** The lines of the "your clip" topic, in the order of the rules: the counts, a steady pulse, the backbeat, close
        pairs, even velocities, quiet snares and the distance from the 1/16 grid. Empty without hits. */
    std::vector<juce::String> analyse (const Part&);

    /** A part from the notes of a drum clip (DawIt Drums or a drum sound): kick, snare and hi-hat notes (by
        `drums::getDrumForNote`, any hi-hat); other drums are left out. */
    struct DrumNote
    {
        int pitch = 36;
        double startBeat = 0.0;
        int velocity = 100;
    };
    Part getPartFromNotes (const std::vector<DrumNote>&, double lengthBeats, double bpm);

    // ---- Tooltips ----------------------------------------------------------------------------------------------

    enum class Tip { sensitivity, minimumGap, writeAs, velocity, keepFeel, snapTo, strength, keepPlaying, muteClip, listen, hear, makeTrack, help };

    /** The tooltip of a control of the Hits to MIDI window. */
    juce::String getTooltip (Tip);
} // namespace triggerguide
} // namespace daw
