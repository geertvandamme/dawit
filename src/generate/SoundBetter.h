#pragma once

#include "ExerciseFix.h"
#include "FirstSong.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInEq.h"
#include "engine/Engine.h"

#include <juce_core/juce_core.h>

#include <array>
#include <vector>

namespace daw::gen
{
/** The "Make a track sound better" exercise, in five steps: pick a track and listen, make room for it with DawIt EQ,
    even it out with DawIt Compressor, give it space with DawIt Reverb on a return, and compare each effect off and on.
    UI-free: the texts, the chosen-track rule, the starting-point table, the EQ cut/boost analysis, the send check, the
    live "is it done" rules, the statuses and tips, and three small trackers (listening, compressor reduction, comparing)
    that the app's 5 Hz timer feeds.

    Step numbers (0 first): 0 pick a track and listen, 1 EQ, 2 compressor, 3 reverb on a send, 4 compare. The exercise
    changes nothing in the song; the user's own clicks (and the *Add* buttons) do. */
namespace soundbetter
{
    constexpr int numSteps = 5;

    /** The step numbers. */
    constexpr int stepListen = 0, stepEq = 1, stepCompressor = 2, stepReverb = 3, stepCompare = 4;

    using Fix = exercises::Fix;

    /** The app-settings keys' stem: the key is `learn.soundBetter.done`. */
    inline const char* const settingsStem = "learn.soundBetter";

    /** The window's height in pixels (five rows with long texts; to be checked by a screenshot). */
    constexpr int windowHeight = 780;

    // DawIt's own thresholds (not standards).
    constexpr double listenSeconds = 4.0;          // how long the chosen track must be heard in the song
    constexpr double minCutDb = 1.5;               // the EQ must lower something by at least this much...
    constexpr double minCutHz = 40.0, maxCutHz = 16000.0;   // ...between these frequencies
    constexpr double bigBoostDb = 6.0;             // a boost this big gets a tip
    constexpr double minReductionDb = 2.0;         // the compressor must be heard turning the track down by this much
    constexpr double heavyReductionDb = 10.0;      // and this much gets a tip
    constexpr double compareSeconds = 2.0;         // each way, switched off and switched on

    /** One step: heading, one-line summary, why and one or two guide pages. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();             // "Make a track sound better"
    juce::String getIntro();
    juce::String getHelpDescription();   // the Help menu item's one-line description
    juce::String getCompletionText();

    /** A step number read from a setting: anything outside the five steps is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** The three effects the exercise is about. */
    enum class Effect { eq, compressor, reverb };

    /** The track's first DawIt EQ or DawIt Compressor in its chain (a switched-on one first when `preferOn`), or null.
        For `Effect::reverb` it looks at the track's own chain too: use `getSends` for the returns. */
    PluginSlot* findEffect (const Track&, Effect, bool preferOn = true);

    /** The chosen track: the selected one when it is an audio or MIDI track, else `remembered` (the one chosen earlier
        this session) when it is still in the song and is an audio or MIDI track, else null. Both pointers are only
        compared with the engine's tracks, never dereferenced before that. */
    const Track* findChosenTrack (const Engine&, const Track* selected, const Track* remembered);

    /** Whether any audio or MIDI track has a clip: there is something to listen to. */
    bool hasTrackWithSound (const Engine&);

    /** Whether an unmuted clip of the track lies under the beat and the track isn't silenced (muted, in a muted bus,
        or another track soloed): the track can be heard there. */
    bool isHeardAt (const Engine&, const Track&, double beat);

    /** The start beat of the track's first unmuted clip, or -1 when it has none. */
    double getFirstClipBeat (const Track&);

    //==============================================================================
    /** What the track's name (or, for a MIDI track on the built-in synth with an unknown name, its sound) says it is. */
    enum class Source { other, vocal, kick, snare, drums, bass, guitar, keys, ensemble };

    Source getSource (const Track&);

    /** The names of the EQ and compressor starting points to suggest for the track, empty for none (an EQ for a bass is
        "Bass", for a pad "Make room for a vocal" when another track is a vocal, else none). */
    struct StartingPoints
    {
        juce::String eq, compressor;
    };

    StartingPoints getStartingPoints (const Engine&, const Track&);

    //==============================================================================
    /** What an EQ's bands do to the level between 40 Hz and 16 kHz, summed over the bands that are on, in 1/24 octave
        steps at 48 kHz: the deepest cut and the biggest boost (in dB, 0 or more), and where. */
    struct EqAnalysis
    {
        double cutDb = 0.0, cutHz = 0.0;
        double boostDb = 0.0, boostHz = 0.0;
    };

    EqAnalysis analyseBands (const std::array<eq::Band, eq::numBands>&);
    EqAnalysis analyseEq (const BuiltInEq&);

    /** "400 Hz", "3.2 kHz": two significant figures. */
    juce::String formatFrequency (double hz);

    //==============================================================================
    /** One send of the chosen track that is on, to a return, and what the return has. */
    struct SendInfo
    {
        const Track* returnTrack = nullptr;
        float levelDb = 0.0f;
        bool returnMuted = false;
        PluginSlot* reverb = nullptr;       // the return's DawIt Reverb (a switched-on one first), or null
        bool reverbOn = false;
        float mixPercent = 100.0f;          // of that reverb
        bool counts() const noexcept { return ! returnMuted && reverb != nullptr && reverbOn; }
    };

    /** The track's sends that are on (above Off), in the order the track has them. */
    std::vector<SendInfo> getSends (const Track&);

    /** The effects the compare step watches, found fresh each time: the track's DawIt EQs and Compressors, and the
        DawIt Reverbs on the returns it sends to (send on). */
    std::vector<PluginSlot*> findExerciseEffects (const Track&);

    //==============================================================================
    /** The listen record, in memory: seconds of the chosen track heard in the song. */
    struct ListenRecord
    {
        double seconds = 0.0;
        bool isDone() const noexcept { return seconds >= listenSeconds - 1.0e-6; }
    };

    /** The reduction record: the most the chosen track's DawIt Compressor turned down while playing. */
    struct ReductionRecord
    {
        double maxDb = 0.0;
    };

    /** The compare record: for each exercise effect, seconds played with it switched on and switched off. */
    struct CompareRecord
    {
        struct Entry
        {
            const PluginSlot* slot = nullptr;
            double onSeconds = 0.0, offSeconds = 0.0;
        };

        std::vector<Entry> entries;

        /** The effect heard at least `compareSeconds` each way among `current`, or null. */
        const PluginSlot* findCompared (const std::vector<PluginSlot*>& current) const;
    };

    /** The longest gap between two feeds that counts as time heard (a stalled timer doesn't add up). */
    constexpr double longestFeedSeconds = 0.5;

    /** Seconds heard: fed on each timer tick with whether the song plays and the chosen track is heard under the heard
        playhead (`isHeardAt`), and the seconds since the last feed. */
    class ListenTracker
    {
    public:
        void feed (bool playing, bool heard, double secondsSinceLastFeed);
        ListenRecord getRecord() const { return record; }
        void reset() { record = {}; }

    private:
        ListenRecord record;
    };

    /** The most reduction heard while playing. `compressor` is the slot being read (null: none, which clears the
        record); another slot than last time also starts afresh. `recentDb` is `BuiltInCompressor::getRecentReductionDb`. */
    class ReductionTracker
    {
    public:
        void feed (bool playing, const PluginSlot* compressor, float recentDb);
        ReductionRecord getRecord() const { return record; }
        void reset() { record = {}; slot = nullptr; }

    private:
        ReductionRecord record;
        const PluginSlot* slot = nullptr;
    };

    /** Seconds each exercise effect played on and off. Effects that are no longer in the song are forgotten. */
    class CompareTracker
    {
    public:
        struct Seen
        {
            const PluginSlot* slot = nullptr;
            bool on = false;
        };

        void feed (bool playing, const std::vector<Seen>& effects, double secondsSinceLastFeed);
        CompareRecord getRecord() const { return record; }
        void reset() { record = {}; }

    private:
        CompareRecord record;
    };

    //==============================================================================
    /** What the exercise needs from the app besides the song. */
    struct Progress
    {
        const Track* selected = nullptr;     // what is selected now (maybe a bus, a return or null)
        const Track* remembered = nullptr;   // the track chosen earlier this session
        bool playing = false;
        double heardBeat = 0.0;
        bool hasOutputDevice = true;
        ListenRecord listened;
        ReductionRecord reduction;
        CompareRecord compared;
    };

    /** The chosen track for the song now. */
    const Track* findChosenTrack (const Engine&, const Progress&);

    bool isStepDone (int step, const Engine&, const Progress&);

    /** What to do, with the track's name and starting point filled in. */
    juce::String getDoText (int step, const Engine&, const Progress&);

    /** The action button: the step's own label and tooltip. `hasEffect`: the track already has the step's effect (steps 2 and 3). */
    juce::String getActionLabel (int step, bool hasEffect);
    juce::String getActionTooltip (int step, const juce::String& trackName, bool hasEffect);

    /** The label and tooltip of a fix's button. */
    juce::String getFixLabel (Fix);
    juce::String getFixTooltip (Fix);

    /** The label and tooltip of a step's action for the song now (its fix when the status names one). */
    struct ActionText
    {
        juce::String label, tooltip;
        Fix fix = Fix::none;
    };

    ActionText getAction (int step, const Engine&, const Progress&);

    /** What is missing for a step to be done, and the tips that go with it: lines joined with new lines (empty: nothing
        to say), and the way forward. Tips stay while the step is done. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step ("DawIt EQ takes up to 4.0 dB off, most around 400 Hz."), or empty. */
    juce::String getDoneNote (int step, const Engine&, const Progress&);

    /** The *Show me* pointer's sentence for the step. */
    juce::String getShowMeText (int step, const Engine&, const Progress&);
} // namespace soundbetter
} // namespace daw::gen
