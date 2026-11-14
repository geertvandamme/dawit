#pragma once

#include "TheoryGuides.h"
#include "engine/Chord.h"
#include "engine/Engine.h"
#include "engine/MusicalKey.h"

#include <juce_core/juce_core.h>

#include <utility>
#include <vector>

namespace daw::gen
{
/** The first-song exercise: four short steps in the user's own song (hear a note, change a chord, add a rhythm, save).
    Everything here is UI-free: the texts, the live "is it done" checks, the example data and the two writers. */
namespace firstsong
{
    constexpr int numSteps = 4;

    /** The two app-settings keys the exercise remembers (nothing else is stored). */
    inline const char* const stepSettingKey = "learn.firstSong.step";
    inline const char* const doneSettingKey = "learn.firstSong.done";

    /** A link to a page of an existing guide. */
    struct Link
    {
        guides::Guide guide = guides::Guide::midi;
        juce::String topic;
    };

    /** One step: its heading, its one-line summary, what to do and why. `link` is the guide page it points at
        (`hasLink` false for the step with none). */
    struct Step
    {
        juce::String title;
        juce::String subtitle;
        juce::String why;
        Link link;
        bool hasLink = true;
    };

    /** The four steps, in order. */
    std::vector<Step> getSteps();

    /** Every guide page the exercise, the start tiles and the pointers link to (for a test and for the UI). */
    std::vector<Link> getAllLinks();

    juce::String getTitle();         // "First song exercise"
    juce::String getIntro();
    juce::String getCompletionText();

    /** What to do in step `step` (0 to 3). Step 1 names the existing MIDI track (empty: none yet); step 2 names the
        song's key and example chords. Out-of-range steps give an empty string. */
    juce::String getDoText (int step, const MusicalKey& key, const juce::String& midiTrackName);

    /** The step's action button text, from the song's state (step 1 *Add a piano and play it* or *Open <name> and play it*;
        step 2 *Put in four chords* or *Open the Chords row*; step 3 *Add drums* or *Write a basic beat*; step 4 *Save...*). */
    juce::String getActionLabel (int step, const juce::String& midiTrackName, bool chordTrackEmpty, bool hasDrumTrack);

    /** The tooltip of that button. */
    juce::String getActionTooltip (int step, bool chordTrackEmpty, bool hasDrumTrack);

    /** A step number read from a setting: anything outside 0 to 3 is 0. */
    int clampStep (int stored) noexcept;

    /** True when the stored "finished once" flag holds exactly `true`. */
    bool readDone (const juce::var& stored);

    //==============================================================================
    /** True for a track that plays drums: the built-in Drums or Percussion sound, or DawIt Drums as the instrument. */
    bool isDrumTrack (const Track&);

    /** What the saved state of the project is, as the exercise needs it. */
    struct SavedState
    {
        bool hasFile = false;
        bool unsaved = false;
    };

    /** Step 1: a non-drum MIDI track has a note. */
    bool isStep1Done (const Engine&);

    /** Step 2: the chord track has a chord and differs from `baseline` (the chords when the step was opened). */
    bool isStep2Done (const Engine&, const ChordMap& baseline);

    /** Step 3: a drum track has at least 4 notes. */
    bool isStep3Done (const Engine&);

    /** Step 4: the project has a file and no unsaved changes. */
    bool isStep4Done (const SavedState&);

    /** Whether step `step` (0 to 3) is done. */
    bool isStepDone (int step, const Engine&, const ChordMap& baseline, const SavedState&);

    //==============================================================================
    /** The first non-drum MIDI track that has a note, else the first non-drum MIDI track, else null. */
    const Track* findMidiTrack (const Engine&);

    /** Bars 1 to 4 (indexes 0 to 3) of the key's first well-known progression: C G Am F in C major, Am F C G in A minor. */
    std::vector<std::pair<int, Chord>> getExampleChords (const MusicalKey&);

    /** The suggested one-bar swap: the 4th chord of the example replaced by the key's 2nd-degree triad in a major
        key (F to Dm in C) or its 4th-degree triad in a minor key (G to Dm in A minor). `bar` is the bar it replaces. */
    std::pair<int, Chord> getSwapSuggestion (const MusicalKey&);

    /** A basic rock beat for `bars` bars of 4/4: per bar kick (36) on beats 1 and 3, snare (38) on 2 and 4, closed
        hi-hat (42) on every eighth; 12 notes a bar, each 0.25 beats long, from beat 0. */
    std::vector<MidiNote> getBasicBeat (int bars);

    /** Whether the edit's chord track is empty (so the example may be written). */
    bool canWriteChords (const Engine&);

    /** Whether `clip` may get the basic beat: it is empty, on a drum track, and the song is in 4/4. */
    bool canWriteBeat (const Engine&, const MidiClip&);

    /** Writes the example chords into the empty chord track as one undo step "Put in four chords". Returns false and
        changes nothing when the chord track has any chord. Message thread. */
    bool writeExampleChords (Engine&);

    /** Writes the basic beat over the whole length of an empty clip on a drum track as one undo step "Write a basic
        beat". Returns false and changes nothing otherwise. Never changes the track's instrument or sound. Message thread. */
    bool writeBasicBeat (Engine&, MidiClip&);
} // namespace firstsong
} // namespace daw::gen
