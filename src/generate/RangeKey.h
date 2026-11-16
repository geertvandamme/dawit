#pragma once

#include "ExerciseFix.h"
#include "FirstSong.h"
#include "Karaoke.h"
#include "SingMatch.h"
#include "SingingGuide.h"
#include "SongMove.h"
#include "engine/Engine.h"

#include <juce_core/juce_core.h>

#include <set>
#include <vector>

namespace daw::gen
{
/** The "Find your range and the right key" exercise, in five steps: record a low-to-high take, keep the measured notes as
    your range, find the song's melody, compare it with your range and move the song to a key that suits you, then hear it
    in the new key. UI-free: the texts, the live "is it done" rules, the records of the last measurement and the last move,
    the heard-bars counter and the statuses. Every tick comes from the song or a real result; no button press ticks a step.

    Step numbers (0 first): 0 record, 1 keep, 2 melody, 3 key, 4 hear. */
namespace rangekey
{
    constexpr int numSteps = 5;
    constexpr int stepRecord = 0, stepKeep = 1, stepMelody = 2, stepKey = 3, stepHear = 4;

    constexpr const char* settingsStem = "learn.rangeKey";
    constexpr int windowHeight = 620;   // the tallest step (a long status under the move button) ends near y = 510 at 700; confirmed by screenshot

    /** A range take lasts at least this many seconds; the melody has at least this many notes; and the song is heard
        through this many different bars with melody notes (all of them when there are fewer). */
    constexpr double takeSecondsNeeded = 3.0;
    constexpr int melodyNotesNeeded = 4, hearBarsNeeded = 4;

    using Fix = exercises::Fix;

    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();             // "Find your range and the right key"
    juce::String getIntro();
    juce::String getHelpDescription();   // the Help menu item's one-line description
    juce::String getCompletionText();

    /** A step number read from a setting: anything outside the five steps is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** The last range measured in a take, in memory (the owner keeps it, clears it with a new or opened project). */
    struct MeasureRecord
    {
        bool present = false;
        singing::Range range;
        juce::String clipName;
    };

    /** The last move this exercise made, in memory: the shift, the key it left the song in and the undo step count right
        after it. It is forgotten (ignored) when the key no longer matches. */
    struct MoveRecord
    {
        bool present = false;
        int shift = 0;
        MusicalKey keyAfter;
        int undoSteps = 0;
    };

    /** What the exercise knows beyond the song itself. */
    struct Progress
    {
        karaoke::TakeRecord take;               // the clips the last recording made
        singing::Range range;                   // the singer's range (the two settings)
        const Track* chosenMelody = nullptr;    // the melody track the singing guide's window has chosen (identity only), or null
        const Track* selectedTrack = nullptr;   // the selected track (identity only), or null
        MeasureRecord measured;
        MoveRecord lastMove;
        int heardBars = 0;                      // distinct melody bars heard since the key was chosen (`HeardBarsCounter`)
        bool hasOutputDevice = true;
        bool hasInput = true;                   // the audio device has input channels
        bool recording = false;
    };

    //==============================================================================
    /** The range take: the newest clip of the last recording that is still in the edit and lasts at least 3 seconds. */
    const AudioClip* findTake (const Engine&, const Progress&);

    /** The melody track: the guide's chosen one while it is a MIDI track in the edit, else `singing::chooseMelodyTrack`
        (the selected track first). The singing guide uses the same rule. Null when there is none. */
    const Track* findMelodyTrack (const Engine&, const Progress&);

    /** The notes on a track's MIDI clips. */
    int countNotes (const Track&);

    /** The melody of a track (`singthis::buildMelody`), empty for null. */
    singthis::Melody buildMelodyOf (const Engine&, const Track*);

    /** What the singing guide's check says about the melody track and the range: always `singing::analyse`. */
    singing::Analysis analyseMelody (const Engine&, const Progress&);

    /** The bars (0 first) that hold notes of the melody track. */
    std::set<int> getMelodyBars (const Engine&, const Track*);

    /** The audio clip "Convert to MIDI..." is offered on in step 3: the first clip on an audio track with a vocal word in
        its name (or named Vocals) that is not the range take. Null when there is none. */
    const AudioClip* findConvertClip (const Engine&, const Progress&);

    /** The move the key step offers: the analysis' shift when the melody is outside the range, or the optional nudge when
        it fits but sits near the top or bottom; 0 for an octave, no shift, no key, or a nudge against the last move. */
    int getOfferedShift (const Engine&, const Progress&);

    /** True when the key step's nudge (not a needed move) is what getOfferedShift gives. */
    bool isNudge (const Engine&, const Progress&);

    /** Whether the song can be moved now: the move is not blocked by a recording and no note would leave MIDI 0 to 127. */
    songmove::BackingCheck checkBacking (const Engine&, const Progress&);

    //==============================================================================
    /** Counts the different bars with melody notes heard in the new key: it counts a bar that holds melody notes while the
        song plays (not while recording) with the melody track heard. The owner resets it when the song moves or step 4 stops
        being done. */
    class HeardBarsCounter
    {
    public:
        void feed (int bar, bool holdsMelody, bool playing, bool recording, bool melodyHeard)
        {
            if (holdsMelody && playing && ! recording && melodyHeard)
                bars.insert (bar);
        }

        int getCount() const noexcept { return (int) bars.size(); }
        void reset() { bars.clear(); }

    private:
        std::set<int> bars;
    };

    /** The bars to hear: 4, or all of the melody's bars when there are fewer (at least 1). */
    int getHeardBarsNeeded (const Engine&, const Progress&);

    //==============================================================================
    bool isStepRecordDone (const Engine&, const Progress&);
    bool isStepKeepDone (const Progress&) noexcept;
    bool isStepMelodyDone (const Engine&, const Progress&);
    bool isStepKeyDone (const Engine&, const Progress&);
    bool isStepHearDone (const Engine&, const Progress&);

    bool isStepDone (int step, const Engine&, const Progress&);

    //==============================================================================
    /** What to do in a step, with the numbers filled in. */
    juce::String getDoText (int step, const Engine&, const Progress&);

    /** What a step's own button does. */
    enum class Op { none, record, addRecordingTrack, measureTake, setByHand, convertToMidi, moveSong, playFromBar, audioSettings };

    /** The step's button: its text, tooltip, whether it can be pressed, and what it does. */
    struct Action
    {
        juce::String label;
        juce::String tooltip;
        Fix fix = Fix::none;
        bool enabled = true;
        Op op = Op::none;
        int shift = 0;      // Op::moveSong: the semitones
        int bar = 0;        // Op::playFromBar: the bar (0 first)
    };

    Action getAction (int step, const Engine&, const Progress&);

    /** What stops a step being done now: a line for the status area and the fix the button becomes. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step, or empty. */
    juce::String getDoneNote (int step, const Engine&, const Progress&);
} // namespace rangekey
} // namespace daw::gen
