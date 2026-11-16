#pragma once

#include "ExerciseFix.h"
#include "FirstSong.h"
#include "SongChords.h"
#include "engine/Engine.h"

#include <juce_core/juce_core.h>

#include <set>
#include <vector>

namespace daw::gen
{
/** The "Sing karaoke to a song" exercise, in six steps: bring in a song, take the vocals out, find the words, sing along
    with them, record a take and listen back. UI-free: the texts, the live "is it done" rules, the in-memory records of
    what Remove vocals and the last recording made, the two bar counters and the statuses. Every tick comes from the song
    or from a real result (a track, timed lyric lines, a clip the last recording made, bars that were played); no button
    press ticks a step.

    Step numbers (0 first): 0 bring in, 1 vocals out, 2 words, 3 sing along, 4 record, 5 listen back. */
namespace karaoke
{
    constexpr int numSteps = 6;
    constexpr int stepImport = 0, stepVocals = 1, stepWords = 2, stepSingAlong = 3, stepRecord = 4, stepListen = 5;

    constexpr const char* settingsStem = "learn.karaoke";
    constexpr int windowHeight = 740;   // to be confirmed by a screenshot at the tallest step (the plan's group C)

    /** The song needs this many bars; at least this many timed lines must start inside it; this many different bars
        must be sung along; a take needs this many bars; and at most this many bars of the take must be listened to. */
    constexpr int barsNeeded = 4, linesNeeded = 2, singBarsNeeded = 4, takeBarsNeeded = 2, listenBarsNeeded = 4;

    /** The Vocals track counts as taken out when it is muted or at most this loud (dB), to within 0.05 dB. */
    constexpr double loudestGuideDb = -6.0;

    using Fix = exercises::Fix;

    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();             // "Sing karaoke to a song"
    juce::String getIntro();
    juce::String getHelpDescription();   // the Help menu item's one-line description
    juce::String getCompletionText();

    /** A step number read from a setting: anything outside the six steps is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** What Remove vocals made, in memory (kept by the owner, cleared when another song opens): the tracks it added, which
        of them is Vocals, and the span of the clip it worked on. Tracks are identities only: compare with the engine's own
        (`Engine::indexOf`) and never dereference one that isn't there. */
    struct KaraokeRecord
    {
        std::vector<const Track*> tracks;
        const Track* vocals = nullptr;
        double startBeat = 0.0, endBeat = 0.0;

        bool isEmpty() const noexcept { return tracks.empty() && vocals == nullptr; }
    };

    /** The record for tracks just added by Remove vocals or Split into stems: `added` and `vocals` (the Vocals track; may
        also be in `added` or null), with the span of the audio clips on them. Empty when there are none. */
    KaraokeRecord makeKaraokeRecord (const Engine&, const std::vector<Track*>& added, const Track* vocals);

    /** What the last recording made, in memory: the clips (identities only, by address, from the engine's report) and
        whether a recording came in silent and made none. */
    struct TakeRecord
    {
        std::vector<const AudioClip*> clips;
        bool silent = false;

        bool isEmpty() const noexcept { return clips.empty() && ! silent; }
    };

    /** The record of the engine's last recording report (empty when it made no clip and wasn't silent). */
    TakeRecord makeTakeRecord (const Engine&);

    /** What the exercise knows beyond the song itself. The two counts come from the owner's counters. */
    struct Progress
    {
        KaraokeRecord karaoke;
        TakeRecord take;
        int sungBars = 0;                // distinct bars sung along (`SungAlongCounter`)
        int listenedBars = 0;            // distinct bars of the take listened to (`ListenCounter`)
        bool hasOutputDevice = true;
        bool hasInput = true;            // the audio device has input channels
        bool recording = false;
        bool demucsInstalled = true;
        bool whisperInstalled = true;    // the small English model, the one Detect lyrics offers first
    };

    //==============================================================================
    /** True when `clip` (an address only) is still in the edit. */
    bool isInEdit (const Engine&, const Clip* clip);

    /** The song clip: the longest unmuted audio clip that isn't one the take record names (ties go to the earliest start),
        or null. Find it fresh each time. */
    const AudioClip* findSongClip (const Engine&, const TakeRecord&);

    /** The song's bars: those of the karaoke source clip while the karaoke record has them, else of the song clip. Empty
        (size 0) when there is no song. */
    songchords::Span getSongSpan (const Engine&, const Progress&);

    /** The Vocals track: the karaoke record's, if still in the edit, else the first audio track named "Vocals" (any
        case) with a clip overlapping the song's bars. Null when there is none. */
    const Track* findVocalsTrack (const Engine&, const Progress&);

    /** The first audio clip on the Vocals track, or null. */
    const AudioClip* findVocalsClip (const Engine&, const Progress&);

    /** The clip Detect lyrics works on: the Vocals track's clip when there is one, else the song clip. */
    const AudioClip* findDetectClip (const Engine&, const Progress&);

    /** The clip Remove vocals works on: the song clip. */
    const AudioClip* findRemoveClip (const Engine&, const Progress&);

    /** Whether the Vocals track is muted (the track, all its clips, or by routing) or at most -6 dB. */
    bool areVocalsOut (const Engine&, const Track& vocals);

    /** True when some syllable of the lyrics is sung in `bar` (0 first). */
    bool barHoldsSyllable (const Engine&, int bar);

    /** The bar's index (0 first) for a beat. */
    int getBarOf (const Engine&, double beat);

    /** Whether `bar` is one of the song's bars and holds a syllable: what Sing along counts. */
    bool isSungBar (const Engine&, const Progress&, int bar);

    /** The timed lines that start inside the song's bars. */
    int countTimedLines (const Engine&, const Progress&);

    /** The take: the first clip of the take record that is still in the edit, is at least 2 bars long and overlaps a bar
        with words (one that is heard first when `preferHeard`). Null when there is none. */
    const AudioClip* findTake (const Engine&, const Progress&, bool preferHeard = false);

    /** The track a clip is on, or null. */
    const Track* findTrackOf (const Engine&, const AudioClip&);

    /** Whether the clip is heard: unmuted, on a track that isn't silenced. */
    bool isClipHeard (const Engine&, const AudioClip&);

    /** The bars the take must be listened to: its length in whole bars, at most 4 (at least 1). */
    int getListenBarsNeeded (const Engine&, const AudioClip& take);

    /** The bar (0 first) Record starts at: one bar before the first words (the first timed line in the song's bars, else
        the first line, else the song's first bar), never before bar 0. */
    int getRecordStartBar (const Engine&, const Progress&);

    /** The track Record writes to. `armed`: an audio track is armed. `unarmed`: none is, but an empty audio track exists
        (press R on it). `none`: neither (Add a Recording track fixes it). */
    struct RecordTarget
    {
        enum class Kind { armed, unarmed, none };
        Kind kind = Kind::none;
        const Track* track = nullptr;
    };

    RecordTarget findRecordTarget (const Engine&);

    //==============================================================================
    /** Counts the different bars of the song sung along: it counts a bar that holds a syllable while the song plays with
        Sing along showing; the same bar twice counts once. */
    class SungAlongCounter
    {
    public:
        void feed (int bar, bool holdsSyllable, bool playing, bool singAlongShowing)
        {
            if (holdsSyllable && playing && singAlongShowing)
                bars.insert (bar);
        }

        int getCount() const noexcept { return (int) bars.size(); }
        void reset() { bars.clear(); }

    private:
        std::set<int> bars;
    };

    /** Counts the different bars of the take listened to: it counts a bar while the song plays (not while recording) with
        the playhead inside the take and the take heard. */
    class ListenCounter
    {
    public:
        void feed (int bar, bool insideTake, bool playing, bool recording, bool heard)
        {
            if (insideTake && playing && ! recording && heard)
                bars.insert (bar);
        }

        int getCount() const noexcept { return (int) bars.size(); }
        void reset() { bars.clear(); }

    private:
        std::set<int> bars;
    };

    //==============================================================================
    bool isStepImportDone (const Engine&, const Progress&);
    bool isStepVocalsDone (const Engine&, const Progress&);
    bool isStepWordsDone (const Engine&, const Progress&);
    bool isStepSingAlongDone (const Progress&) noexcept;
    bool isStepRecordDone (const Engine&, const Progress&);
    bool isStepListenDone (const Engine&, const Progress&);

    bool isStepDone (int step, const Engine&, const Progress&);

    //==============================================================================
    /** What to do in a step, with the bar Record starts at filled in. */
    juce::String getDoText (int step, const Engine&, const Progress&);

    /** The step's own action button text and tooltip (not a fix). */
    struct Action
    {
        juce::String label;
        juce::String tooltip;
        Fix fix = Fix::none;
    };

    Action getAction (int step, const Engine&, const Progress&);

    /** What stops a step being done now: a line for the status area and the fix the button becomes (`Fix::none`: the
        button stays the step's own action). Empty text: nothing to say. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step, or empty. */
    juce::String getDoneNote (int step, const Engine&, const Progress&);

    juce::String getFixLabel (Fix);
    juce::String getFixTooltip (Fix);
} // namespace karaoke
} // namespace daw::gen
