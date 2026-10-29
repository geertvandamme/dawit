#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <functional>
#include <optional>
#include <vector>

namespace daw::analysis::hits
{
/**
    Finding the hits (drum strokes, clicks, claps) in a recording, for Hits to MIDI. It works in
    two stages so the window's controls answer at once:

    1. Candidates, once per clip (a second or two for a song): every rise in level, found as in the
       tempo finder (40 bands from 30 Hz to 11 kHz, every 5.8 ms), with its time refined on the
       audio to the sample, its peak level and how its energy divides between low, middle and high.
    2. Picking, on every change of a setting: the candidates that are loud enough and not too soon
       after the last one, each sorted into kick, snare or hi-hat, with a velocity from how loud it
       was. Hand edits (moved, added, removed or re-sorted hits) are put on top.
*/

/** The drums a hit is sorted into, by where its energy is. */
enum class Drum
{
    kick,    // below 150 Hz: General MIDI note 36
    snare,   // 150 Hz to 6 kHz: note 38
    hihat    // above 6 kHz: note 42 (closed hi-hat)
};

/** The General MIDI note a drum is written as. */
int getNoteForDrum (Drum);

/** How loudness becomes velocity. */
enum class VelocityCurve
{
    natural,   // a hit 12 dB quieter gets 64
    gentle,    // soft hits keep more velocity
    strong,    // soft hits get much less
    same       // every hit gets 100
};

/** One place where the sound rises, before any setting has chosen. */
struct Candidate
{
    double time = 0.0;      // seconds from the clip's start: the start of the rise
    float rise = 0.0f;      // how much the spectrum rose there (dB summed over the bands)
    float levelDb = -100.0f;   // the peak level in the 30 ms after the time
    float low = 0.0f, mid = 0.0f, high = 0.0f;   // energy below 150 Hz, 150 Hz to 6 kHz, above 6 kHz in the first 50 ms
};

/** A hit that has been picked. */
struct Hit
{
    double time = 0.0;
    Drum drum = Drum::snare;
    int velocity = 100;      // 1 to 127
    float levelDb = -100.0f;
    float rise = 0.0f;
    bool drumFixed = false;  // the drum was chosen by hand
};

/** The choices that pick hits from candidates. */
struct PickSettings
{
    double sensitivityDb = -30.0;   // the quietest hit that counts, in dB below the loudest candidate (-6 to -60)
    double minGapMs = 30.0;         // nothing new counts this long after a hit (5 to 250)
    std::optional<Drum> singleDrum; // every hit is this drum; empty: sort by drum
    VelocityCurve curve = VelocityCurve::natural;
};

/** A change made by hand, kept as a list and put on top of the automatic hits after each picking. */
struct HandEdit
{
    enum class Kind { add, move, remove, setDrum };

    Kind kind = Kind::add;
    double time = 0.0;        // add: where; move, remove, setDrum: the hit to match (the nearest within matchSeconds)
    Candidate target;         // add and move: the hit's new place, level and energies
    Drum drum = Drum::snare;  // setDrum
};

/** How far from a hit a hand edit still finds it. */
constexpr double matchSeconds = 0.010;

/** Every rise in the recording, in time order. `shouldExit` is checked often (return true to
    stop; the result is then empty) and `progress` gets 0 to 1. */
std::vector<Candidate> findCandidates (const juce::AudioBuffer<float>& audio, double sampleRate,
                                       const std::function<bool()>& shouldExit = {},
                                       const std::function<void (double)>& progress = {});

/** A candidate for a hit at `time` (seconds), measured on the audio: the strongest rise within
    `searchSeconds` of it is used unless `exact`, where the time is kept as it is. Used by hand
    edits. The `rise` is the largest of the candidates' if none is near. */
Candidate measureCandidate (const juce::AudioBuffer<float>& audio, double sampleRate, double time,
                            double searchSeconds, const std::vector<Candidate>& candidates, bool exact);

/** The drum whose part of the spectrum holds most of the candidate's energy. */
Drum classify (const Candidate&);

/** The velocity (1 to 127) of a hit `dbBelowLoudest` (0 or more) under the loudest of its drum. */
int getVelocity (VelocityCurve, double dbBelowLoudest);

/** The hits: the candidates within Sensitivity of the loudest, at least the gap apart (the first
    wins), sorted into drums, with the hand edits applied, and velocities from the level against
    the loudest hit of the same drum. */
std::vector<Hit> pick (const std::vector<Candidate>& candidates, const PickSettings&, const std::vector<HandEdit>& edits = {});
} // namespace daw::analysis::hits
