#pragma once

#include "ExerciseFix.h"
#include "FirstSong.h"
#include "engine/Engine.h"
#include "engine/Lyrics.h"
#include "engine/Section.h"

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

namespace daw::gen
{
/** The "Write lyrics to your song" exercise, in four steps: pick a section (loop it), write a line for it, time the line by
    tapping and sing it with Sing along open. UI-free: the texts, the live "is it done" rules, the chosen-section and
    counts-for rules, the statuses and tips, and the tracker that notices the song playing through the line.

    Step numbers (0 first): 0 pick a section, 1 write a line, 2 time it by tapping, 3 sing it. The exercise changes nothing
    in the song itself; the user's own actions do. */
namespace writelyrics
{
    constexpr int numSteps = 4;

    /** The step numbers. */
    constexpr int stepSection = 0, stepWrite = 1, stepTap = 2, stepSing = 3;

    using Fix = exercises::Fix;

    /** The app-settings keys' stem: the key is `learn.writeLyrics.done`. */
    inline const char* const settingsStem = "learn.writeLyrics";

    /** The window's height in pixels: four rows with long texts need more than the other exercises. */
    constexpr int windowHeight = 780;

    /** A line under this many seconds is "sung" by one sample inside it; a longer one needs a sample in each half. */
    constexpr double shortLineSeconds = 0.5;

    /** More syllables a beat than this is fast to sing (the lyrics guide's *Your lyrics* rule). */
    constexpr double fastSyllablesPerBeat = 2.5;

    /** One step: heading, one-line summary, why and one or two guide pages. */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        std::vector<firstsong::Link> links;
    };

    std::vector<Step> getSteps();

    juce::String getTitle();             // "Write lyrics to your song"
    juce::String getIntro();
    juce::String getHelpDescription();   // the Help menu item's one-line description
    juce::String getCompletionText();

    /** A step number read from a setting: anything outside the four steps is 0. */
    int clampStep (int stored) noexcept;

    //==============================================================================
    /** The loop as the engine holds it. */
    struct Loop
    {
        bool on = false;
        double startBeat = 0.0, endBeat = 0.0;
    };

    /** The chosen section: the sung section the loop lies in (loop on, its start and end inside one section of a sung
        type, `lyrics::isSung`), else the section the user chose earlier this session if a sung section still starts at
        `rememberedBar` (-1: none). `barLength` is the song's bar in beats. nullopt when there is none. */
    std::optional<Section> findChosenSection (const SectionList&, const Loop&, double barLength, int rememberedBar);

    /** Whether a line counts for a section: the middle of its span (start to end beat) lies inside the section's bars, so
        a pickup before the bar line still counts. */
    bool lineCountsFor (const LyricLine&, const Section&, double barLength);

    /** A line is timed when it has at least 2 syllables and every syllable after its first is pinned. */
    bool isLineTimed (const LyricLine&);

    /** How many of a line's syllables after the first are not pinned. */
    int countUnplaced (const LyricLine&);

    /** The line the exercise is about: the first counting line that is timed, else the first counting line. Its index in
        `lyrics.lines`, or -1. Find it fresh each time; never keep the index. */
    int findLine (const Lyrics&, const Section&, double barLength);

    /** The line as typed without hyphens, trimmed: "hold on, hold on tonight". */
    juce::String getLineText (const LyricLine&);

    /** The words of the line that sound like 2 or more syllables (`rhyme::countSyllables`) but are typed as one block,
        lower case without punctuation: "along", "river". */
    juce::StringArray findUnsplitWords (const LyricLine&);

    /** Whether more than 2.5 syllables a beat fall in the line. */
    bool isLineFast (const LyricLine&);

    /** The line's syllables as they sound, counted the way the lyrics guide's *Your lyrics* topic does (`rhyme::countSyllables` on the hyphenated text): "I walked along the river late" is 8 though it is typed as 6 blocks. */
    int countSungSyllables (const LyricLine&);

    /** Whether any unmuted clip on an unmuted track overlaps the section's bars. */
    bool hasSoundIn (const Engine&, const Section&);

    //==============================================================================
    /** The sung record, in memory: the song played through a line, and the line's text then. */
    struct SungRecord
    {
        bool done = false;
        juce::String text;
    };

    /** The line the tracker follows: its span in beats and its text. */
    struct LineSpan
    {
        double startBeat = 0.0, endBeat = 0.0;
        juce::String text;
    };

    /** Notices the song playing through a line with Sing along open. `MainComponent`'s 5 Hz timer feeds it the heard
        beat. Done when, in one pass, a sample falls in the line's first half and a later one in its second half (a line
        under `shortLineSeconds` needs one sample inside). A stop, Sing along closing, a change of the line's text or the
        beat going backwards (a loop wrap) starts a new pass. Once done it stays done, for that text. */
    class SungTracker
    {
    public:
        /** One sample. `line` is null when there is no line to follow. */
        void feed (double heardBeat, bool playing, bool singAlongShowing, const LineSpan* line, double secondsPerBeat);

        /** The record, to hand to the exercise. */
        SungRecord getRecord() const { return { done, doneText }; }

        /** Forgets everything (a new song, or Start over). */
        void reset();

    private:
        void startNewPass();

        bool done = false;
        juce::String doneText;
        bool seenFirstHalf = false;
        double lastBeat = -1.0e9;
        juce::String passText;
    };

    //==============================================================================
    /** What the exercise needs from the app besides the song. */
    struct Progress
    {
        int rememberedBar = -1;        // the section chosen earlier this session, by its start bar; -1 none
        SungRecord sung;
        bool hasOutputDevice = true;
        bool singAlongOpen = false;
    };

    /** The loop as the engine holds it now. */
    Loop getLoop (const Engine&);

    /** The chosen section for the song now. */
    std::optional<Section> findChosenSection (const Engine&, const Progress&);

    bool isStepDone (int step, const Engine&, const Progress&);

    /** What to do, with the chosen section's name filled in ("your section" while there is none). */
    juce::String getDoText (int step, const Engine&, const Progress&);

    /** The action button: the step's own label and tooltip. */
    juce::String getActionLabel (int step);
    juce::String getActionTooltip (int step, const juce::String& sectionName);

    /** The label and tooltip of a fix's button. */
    juce::String getFixLabel (Fix);
    juce::String getFixTooltip (Fix, const Engine&, const Progress&);

    /** What is missing for a step to be done: a line (empty: nothing to say) and the way forward. Tips that don't stop the
        step (nothing sounds, a word to split) are part of the text. */
    struct Status
    {
        juce::String text;
        Fix fix = Fix::none;
    };

    Status getStatus (int step, const Engine&, const Progress&);

    /** The line under a ticked step ("Chorus, bars 13 to 20, is your section."), or empty. */
    juce::String getDoneNote (int step, const Engine&, const Progress&);

    /** The *Show me* pointer's sentence. `sectionName` is empty when there is no sung section to point at. */
    juce::String getShowMeText (int step, const juce::String& sectionName);

    /** "bars 13 to 20" for a section (bar 0 is shown as 1). */
    juce::String getBarsText (const Section&);

    /** The first sung section (for the Sections row pointer when no section is chosen), or nullopt. */
    std::optional<Section> findFirstSungSection (const SectionList&);
} // namespace writelyrics
} // namespace daw::gen
