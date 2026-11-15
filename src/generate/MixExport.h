#pragma once

#include "ExerciseFix.h"
#include "FirstSong.h"
#include "engine/Engine.h"
#include "engine/LoudnessMeter.h"

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace daw::gen
{
/** The "Mix and export your song" exercise, in four steps: balance the faders, pan the parts, measure the loudness of the
    whole song and export it as a WAV. UI-free: the texts, the baseline of faders and pans, the name rule that suggests a
    track to pan, the live "is it done" rules, the statuses, tips and done notes, and the two records (a loudness
    measurement and a finished bounce) that the app fills in.

    Step numbers (0 first): 0 balance, 1 pan, 2 loudness, 3 export. The exercise changes nothing in the song: the user's
    own actions do. */
namespace mixexport
{
    constexpr int numSteps = 4;

    /** The step numbers. */
    constexpr int stepBalance = 0, stepPan = 1, stepLoudness = 2, stepExport = 3;

    using Fix = exercises::Fix;

    /** The app-settings keys' stem: the key is `learn.mixExport.done`. */
    inline const char* const settingsStem = "learn.mixExport";

    /** The window's height in pixels (four rows with long texts; to be checked by a screenshot). */
    constexpr int windowHeight = 700;

    // DawIt's own thresholds (not standards).
    constexpr double minFaderMoveDb = 1.0;       // a fader must move this much from where it was
    constexpr double minPanMove = 0.10;          // a pan knob this much (10%)
    constexpr double centreTipPan = 0.25;        // a centre part panned this far out gets a tip
    constexpr double farDownDb = -20.0;          // a fader below this gets a tip

    /** One step: heading, one-line summary, why and one or two guide pages. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();             // "Mix and export your song"
    juce::String getIntro();
    juce::String getHelpDescription();   // the Help menu item's one-line description
    juce::String getCompletionText();

    /** A step number read from a setting: anything outside the four steps is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** Each track's fader and pan when the exercise opened (every track except the master; buses and returns count). */
    struct Baseline
    {
        struct Entry
        {
            const Track* track = nullptr;   // only compared with the engine's tracks, never dereferenced before that
            float volumeDb = 0.0f;
            float pan = 0.0f;
        };

        bool valid = false;
        std::vector<Entry> tracks;
        float masterDb = 0.0f;
    };

    /** The faders and pans now. */
    Baseline takeBaseline (const Engine&);

    /** "C", "35 L", "40 R": a pan as the mixer's headers show it. */
    juce::String formatPan (float pan);

    //==============================================================================
    /** Whether the track is an audio or MIDI track (the ones the pan step is about). */
    bool isSoundTrack (const Track&);

    /** Whether the track's name says a backing vocal (backing, bv, bvs, harmony). */
    bool isBackingVocal (const Track&);

    /** A part that usually sits in the middle: kick, snare, drums, bass or a lead (not backing) vocal. */
    bool isCentrePart (const Track&);

    /** The first audio or MIDI track that its name calls a guitar, keys, pad or ensemble, or a backing vocal; null for none. */
    const Track* findPanCandidate (const Engine&);

    /** The track the pointers and texts name: `selected` when it is still in the song and is an audio or MIDI track, else the
        first audio or MIDI track with a clip; null for none. `selected` is only compared with the engine's tracks. */
    const Track* findShownTrack (const Engine&, const Track* selected);

    //==============================================================================
    /** DawIt's pan on one-channel sound: the gain of each side at `pan` (-1 left, +1 right). The near side keeps its level
        and the far side is turned down by cos (pan × 90°). The same rule as `Track`'s. */
    struct PanGains
    {
        double left = 1.0, right = 1.0;
    };

    PanGains getPanGains (double pan);

    /** The total power of a one-channel part panned to `pan`, against the part in the middle, in dB (0 at the middle,
        -3.01 all the way out, -1.25 halfway). */
    double getPanPowerDb (double pan);

    /** What `pan` does to the part added to mono, against the middle, in dB (-6.02 all the way out). */
    double getMonoSumDb (double pan);

    //==============================================================================
    /** The loudness measurement the app last made, in memory: its readings and what it covered. */
    struct MeasureRecord
    {
        bool valid = false;
        bool wholeSong = true;
        double startBeat = 0.0, endBeat = 0.0;   // the range measured (the loop's bars when `wholeSong` is false)
        loudness::Readings readings;
    };

    /** What a finished bounce made. */
    enum class What { mix, selectedTracks, multitracks, stems };

    /** The last bounce that finished through the Bounce to WAV window, in memory. */
    struct ExportRecord
    {
        bool valid = false;
        What what = What::mix;
        bool wholeSong = true;
        double startBeat = 0.0, endBeat = 0.0;
        int bitDepth = 24;
        double sampleRate = 44100.0;
        float peak = 0.0f;                  // linear, the loudest sample
        double seconds = 0.0;
        juce::String fileName;              // "My Song.wav"
    };

    //==============================================================================
    /** What the exercise needs from the app besides the song. */
    struct Progress
    {
        Baseline baseline;
        const Track* selected = nullptr;   // the selected track, compared with the engine's tracks before use
        MeasureRecord measured;
        ExportRecord exported;
        bool hasOutputDevice = true;
    };

    /** The track whose fader moved furthest (at least `minFaderMoveDb`) from the baseline, with where it was and is. */
    struct FaderMove
    {
        const Track* track = nullptr;
        float fromDb = 0.0f, toDb = 0.0f;
    };

    std::optional<FaderMove> findFaderMove (const Engine&, const Baseline&);

    /** The audio or MIDI track whose pan moved furthest (at least `minPanMove`) from the baseline. */
    struct PanMove
    {
        const Track* track = nullptr;
        float from = 0.0f, to = 0.0f;
    };

    std::optional<PanMove> findPanMove (const Engine&, const Baseline&);

    /** Whether the measurement covers the whole song and has an integrated loudness and a true peak. */
    bool isMeasured (const MeasureRecord&);

    /** Whether the export is the mix of the whole song and a 16- or 24-bit file doesn't clip. */
    bool isExported (const ExportRecord&);

    /** Whether a 16- or 24-bit file with this peak (linear) clips. */
    bool clips (int bitDepth, float peak);

    bool isStepDone (int step, const Engine&, const Progress&);

    /** What to do, with the pan candidate's name filled in for step 2. */
    juce::String getDoText (int step, const Engine&, const Progress&);

    /** The step's own button: label and tooltip. */
    juce::String getActionLabel (int step);
    juce::String getActionTooltip (int step);

    /** The label and tooltip of a fix's button. */
    juce::String getFixLabel (Fix);
    juce::String getFixTooltip (Fix);

    /** What is missing for a step to be done, or a tip: a line (empty: nothing to say) and the way forward. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step, or empty. */
    juce::String getDoneNote (int step, const Engine&, const Progress&);

    /** The *Show me* pointer's sentence. `trackName` is the track pointed at (empty: a track's). */
    juce::String getShowMeText (int step, const juce::String& trackName);

    /** "3:12": minutes and seconds. */
    juce::String formatLength (double seconds);

    /** The dB to turn down by to get a peak of `peakDb` back under 0 with a dB to spare: ceil (peak + 1). */
    int getTurnDownDb (double peakDb);
} // namespace mixexport
} // namespace daw::gen
