#pragma once

#include "BuildBeat.h"
#include "ExerciseFix.h"
#include "FirstSong.h"
#include "Karaoke.h"
#include "MixExport.h"
#include "RangeKey.h"
#include "SongChords.h"
#include "SoundBetter.h"
#include "WriteLyrics.h"

#include <juce_core/juce_core.h>

#include <vector>

namespace daw::gen
{
/** The guided exercises as one model: each has a title, intro, steps (with one or two guide links), a completion text and
    a settings stem, and answers, from the song and a small context of app facts, whether step n is done, what to do,
    what its action button says and what is missing. UI-free. First song's content stays in `gen::firstsong`, and the
    chord exercise's in `gen::songchords` the lyrics exercise's in `gen::writelyrics` and the mixing exercise's in `gen::soundbetter` and the mix and export exercise's in `gen::mixexport` and the beat exercise's in `gen::buildbeat` and the karaoke exercise's in `gen::karaoke` and the range and key exercise's in `gen::rangekey`; this namespace puts them behind one set of questions for the shared window. */
namespace exercises
{
    enum class Exercise { firstSong, songChords, writeLyrics, soundBetter, mixExport, buildBeat, mapStructure, karaoke, rangeKey };

    constexpr Exercise allExercises[] { Exercise::firstSong, Exercise::songChords, Exercise::writeLyrics, Exercise::soundBetter, Exercise::mixExport,
                                        Exercise::buildBeat, Exercise::mapStructure, Exercise::karaoke, Exercise::rangeKey };
    constexpr int numExercises = 9;

    using Link = firstsong::Link;   // `Fix` is declared in ExerciseFix.h

    /** One step, as the window shows it. `why` is the generic text: ask `getWhy` for the one that fits the song. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<Link> links;   // none, one or two guide pages
    };

    /** An exercise's fixed content. */
    struct Info
    {
        Exercise exercise = Exercise::firstSong;
        juce::String settingsStem;        // "learn.firstSong": the key is <stem>.done (and the old <stem>.step, <stem>.stepsVersion)
        juce::String title;
        juce::String intro;
        juce::String completion;
        juce::String carryOnLabel;        // the completion's first button
        juce::String carryOnTooltip;
        bool carryOnOpensStartHere = false;   // else it opens `carryOnLink`
        Link carryOnLink;
        juce::String helpDescription;     // the Help menu item's one-line description
        int windowHeight = 500;           // the window's height in pixels (it is 360 wide)
        std::vector<Step> steps;
    };

    /** Facts about the app that the song itself doesn't hold. The owner (`MainComponent`) fills it in on each ask. */
    struct Context
    {
        firstsong::SavedState saved;       // First song step 4
        ChordMap baseline;                 // First song step 2 and the chord exercise's step 4: the chords at the start
        int playedAlongBars = 0;           // the chord exercise's step 5 (play along)
        bool hasOutputDevice = true;       // the chord exercise's step 5
        songchords::TempoRecord tempo;     // the chord exercise's step 2 (tempo)
        int writeLyricsSectionBar = -1;    // the lyrics exercise: the section chosen earlier this session, by its start bar; -1 none
        writelyrics::SungRecord sung;      // the lyrics exercise's step 4: the song has played through the line
        bool singAlongOpen = false;        // the lyrics exercise's step 4
        const Track* soundBetterSelected = nullptr;     // the mixing exercise: the selected track (maybe a bus or a return), compared with the engine's tracks before use
        const Track* soundBetterRemembered = nullptr;   // the track chosen earlier this session
        bool playing = false;                           // the mixing exercise: the song plays, and the beat being heard
        double heardBeat = 0.0;
        soundbetter::ListenRecord listened;             // the mixing exercise's records (see `soundbetter::Progress`)
        soundbetter::ReductionRecord reduction;
        soundbetter::CompareRecord compared;
        mixexport::Baseline mixBaseline;                // the mix and export exercise: the faders and pans when it opened, the selected track,
        const Track* mixSelected = nullptr;             // the last loudness measurement and the last finished bounce
        mixexport::MeasureRecord mixMeasured;
        mixexport::ExportRecord mixExported;
        bool mixerShowing = false;                      // the mixer panel is open
        const Track* beatSelectedTrack = nullptr;       // the beat exercise: the selected track and clip, compared with the engine's before use
        const Clip* beatSelectedClip = nullptr;
        double beatQuantiseGrid = 0.0;                  // the grid the Quantise dialog is on (open) or was last used with; 0 unknown
        karaoke::KaraokeRecord karaokeRecord;           // the karaoke exercise: what Remove vocals made and the last recording's clips (in memory),
        karaoke::TakeRecord karaokeTake;                // the bars sung along and listened to, and what the app can do right now
        int karaokeSungBars = 0;
        int karaokeListenedBars = 0;
        bool hasInputDevice = true;
        bool recording = false;
        bool demucsInstalled = true;
        bool whisperInstalled = true;
        karaoke::TakeRecord rangeKeyTake;               // the range and key exercise: the last recording's clips, the range, the guide's melody choice,
        singing::Range rangeKeyRange;                         // the selected track (identities only), the last measurement and move, and the bars heard
        const Track* rangeKeyChosenMelody = nullptr;
        const Track* rangeKeySelected = nullptr;
        rangekey::MeasureRecord rangeKeyMeasured;
        rangekey::MoveRecord rangeKeyMove;
        int rangeKeyHeardBars = 0;
    };

    /** An action button: its text, tooltip and what it does (`Fix::none`: the step's own action). */
    struct Action
    {
        juce::String label;
        juce::String tooltip;
        Fix fix = Fix::none;
        bool enabled = true;   // false: the button shows but can't be pressed (the range and key exercise)
    };

    /** What is missing for a step to be done: a line (empty: nothing to say) and the way forward. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Info getInfo (Exercise);

    /** The app-settings key that remembers "finished once" (First song's is the old `learn.firstSong.done`). The step is
        not kept in settings: every exercise starts at step 1 in a new session or project (see `StepMemory`). */
    juce::String getDoneSettingKey (Exercise);

    /** The keys older versions saved the step under (`<stem>.step`, `<stem>.stepsVersion`); the app removes them. */
    juce::StringArray getStaleSettingKeys (Exercise);

    /** Every guide page the exercise's steps and completion link to (for a test). */
    std::vector<Link> getAllLinks (Exercise);

    /** A step number: anything outside the exercise's steps is 0. */
    int clampStep (Exercise, int stored) noexcept;

    /** Where each exercise's window is, in memory only: the open step and whether it was finished. Closing and reopening
        a window in the same session and project keeps the place; a new, opened or restored project and a restart start
        every exercise at step 1 (`reset`). */
    struct StepMemory
    {
        struct Entry
        {
            int step = 0;
            bool finished = false;
        };

        Entry get (Exercise exercise) const noexcept { return entries[(size_t) exercise]; }

        void set (Exercise exercise, int step, bool finished) noexcept
        {
            entries[(size_t) exercise] = { clampStep (exercise, step), finished };
        }

        void reset() noexcept { for (auto& e : entries) e = {}; }

    private:
        Entry entries[numExercises];
    };

    bool isStepDone (Exercise, int step, const Engine&, const Context&);
    juce::String getDoText (Exercise, int step, const Engine&, const Context&);
    juce::String getWhy (Exercise, int step, const Engine&, const Context&);
    Action getAction (Exercise, int step, const Engine&, const Context&);
    Status getStatus (Exercise, int step, const Engine&, const Context&);

    /** The note under a ticked step ("32 bars long"), or empty. */
    juce::String getDoneNote (Exercise, int step, const Engine&, const Context&);
} // namespace exercises
} // namespace daw::gen
