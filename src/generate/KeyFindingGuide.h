#pragma once

#include "analysis/MidiHarmony.h"
#include "engine/Chord.h"

#include <map>
#include <vector>

namespace daw::gen
{
/** The guide *Finding the key and chords*: what a key is, major, minor and the modes, harmonic and melodic minor,
    pentatonic and blues scales, how DawIt finds a key from the notes of a MIDI clip, how a chord is named from its
    notes (inversions, slash chords, sus and diminished chords, notes between the chords), and the key, scale and
    chords of the selected clip with what to do with them. The content only (no UI): the window is
    KeyFindingExplainer, the finder is `analysis::midiharmony`. Facts, texts and numbers are checked in the tests. */
namespace keyfinding
{
    enum class Topic { key, majorMinor, modes, harmonicMelodic, pentatonic, howFound, naming, inversions, susDim, between, yours };

    constexpr Topic allTopics[] { Topic::key, Topic::majorMinor, Topic::modes, Topic::harmonicMelodic, Topic::pentatonic,
                                  Topic::howFound, Topic::naming, Topic::inversions, Topic::susDim, Topic::between, Topic::yours };

    juce::String getTopicName (Topic);      // "The modes"
    juce::String getTopicSummary (Topic);   // one line for the list

    /** One row of a picture: a scale or a chord as note names (0 = C), and as MIDI notes to play it. */
    struct Row
    {
        juce::String label;                 // "C major", "Csus2"
        juce::String steps;                 // "W W H W W W H": written between the keys, when it applies
        std::vector<int> pitchClasses;      // the notes drawn
        std::vector<int> pitches;           // MIDI notes to play, lowest first (empty: play the pitch classes up from 60)
        std::vector<int> marked;            // note names to highlight (a characteristic or raised note)
        int home = -1;                      // the home note (scales), the root (chords)
        int bass = -1;                      // the lit bass key (chords)
        juce::String note;                  // what the row shows
    };

    /** The pictures of a topic: rows to draw, and (topics 1, 6, 7, 10) the notes of a short example part in song beats. */
    struct Example
    {
        std::vector<Row> rows;
        std::vector<analysis::midiharmony::Note> notes;
        juce::String caption;
    };

    Example getExample (Topic);

    /** The short tune of topics 1 and 6: C E G E | F A G | E D C D | C, quarter notes at full velocity. */
    std::vector<analysis::midiharmony::Note> getExampleTune();

    /** The text of topics 1 to 10, as written in the plan. Topic 11 gives the prompt to select a clip. */
    juce::String getText (Topic);

    //==============================================================================
    /** What the *Your clip* topic needs to know about the selection. */
    struct ClipView
    {
        bool found = false;                     // a MIDI clip is selected (or found)
        bool drums = false;                     // its track plays a drum sound: no pitches
        juce::String trackName;
        std::optional<MusicalKey> songKey;      // the song's key, when set
        analysis::midiharmony::Result result;
    };

    /** The *Your clip* text: key and fit, how close the runner-up is, outside notes, the song's key, chords with
        numerals and close calls, the progression's name, half-bar changes, and a check by ear. */
    juce::String explainYourClip (const ClipView&);

    //==============================================================================
    /** A chord for a bar of the song (bar 0 is the first). */
    struct BarChord
    {
        int bar = 0;
        Chord chord;
    };

    /** The downbeat chord of each bar that has one: what goes on the chord track (without the bass, one a bar). */
    std::vector<BarChord> getBarChords (const std::vector<analysis::midiharmony::Segment>&);

    /** What putting the clip's chords on the chord track does. */
    struct Merge
    {
        std::vector<BarChord> toWrite;          // bars that get a chord
        std::vector<BarChord> replaced;         // of those, bars that already had another chord (the chord there now)
        int firstBar = 0, lastBar = 0;          // the clip's bars
    };

    /** Bars without a found chord keep what the chord track has. */
    Merge mergeIntoChordTrack (const std::vector<analysis::midiharmony::Segment>&, const ChordMap& existing);

    /** The chord track with the merge written in. */
    ChordMap applyMerge (const Merge&, ChordMap existing);

    /** "Replace the chords in bars 1 to 8? 3 of them already have other chords (bar 2: Dm, bar 4: G, bar 6: C)."
        Empty when nothing would be replaced. */
    juce::String getReplaceQuestion (const Merge&);
} // namespace keyfinding
} // namespace daw::gen
