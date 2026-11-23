#pragma once

#include "engine/BuiltInSubtractive.h"
#include "engine/DescribedSound.h"

#include <functional>
#include <vector>

namespace daw::gen
{
/**
    The words behind "Describe a sound" for any built-in synth that has a describe table, without any UI and
    without needing a model.

    The user types what they want to hear ("a warm pad that swells in slowly"). `buildSystemPrompt`,
    `buildUserPrompt` and `buildSchema` make what goes to Ollama: the rules, the synth's knobs with their ranges
    and usual settings, the starting points with their values, worked examples and the current sound.
    `readReply` turns the answer into a described sound (`daw::DescribedSound`): DawIt reads every value itself (a
    number with its unit, or a choice by name), keeps it inside the knob's range, drops knobs the synth doesn't
    have and changes that equal the base, and checks the words against the result (the table's word and limit
    notes). A language model writes likely text and does not hear sound, so none of this trusts it. `Walkthrough`
    goes through the changes one step at a time; `ValuesUndo` keeps the last 20 sounds for the bar's Undo.
    Nothing here touches the edit or the engine lock.

    A synth is described by a `Table` (DawIt Subtractive's is `getSubtractiveTable`, DawIt Additive's
    `getAdditiveTable`). A sound is a list of values, one per knob, in the knob's own units (`Values`).
*/
namespace sounddescribe
{
    /** The temperature the request uses: low, because the answer must follow the table of knobs. */
    constexpr double temperature = 0.3;

    /** The most changes an answer may have; more are cut. A whole group of knobs (the harmonic bars) counts as one. */
    constexpr int maxChanges = 10;

    /** The most members one group of knobs may have in an answer; more are cut. */
    constexpr int maxGroupMembers = 12;

    /** The `startFrom` value that means "change the sound in the window now". */
    inline const char* currentSoundName = "Current sound";

    /** A sound: one value per knob of the synth, in the knob's own units (see the synth's getKnobValue). */
    using Values = std::vector<float>;

    //==============================================================================
    /** One knob of a synth as the prompt, the reader and the walkthrough need it. */
    struct KnobDesc
    {
        juce::String id;             // in the prompt and the reply: "cutoff", "ampAttack", "h3"
        juce::String title;          // with its part of the window: "Filter envelope attack"
        juce::String typical;        // how it is usually set
        sub::KnobInfo info;          // range, step, choice, log scale and unit
        juce::StringArray choices;   // the names of a choice's values
        int group = -1;              // knobs with the same group number (0 up) are one change and one walkthrough step
        int order = 0;               // the window's order, for sorting the changes and the walkthrough
        bool hasOff = false;         // the text "Off" is a value ...
        float offValue = 0.0f;       // ... and this is it
        bool offExact = false;       // a target of Off counts as reached only at Off
        bool inPrompt = true;        // a row of its own in the prompt's table of knobs
        juce::String noun = "knob";  // "knob" or "bar", for the notes
    };

    /** A starting point: the synth's preset by name, with its `why` (the prompt uses the first sentence), and its
        values. `name` is what the prompt and the schema say, so it has no quotation marks; `displayName` is the
        preset's own name (empty when it is the same). */
    struct StartingPoint
    {
        juce::String name, why;
        Values values;
        juce::String displayName;
    };

    /** One change in a worked example. */
    struct ExampleChange
    {
        int knob = 0;
        const char* value = "";
        const char* reason = "";
    };

    /** A worked example: from a starting point, a description and two changes. */
    struct Example
    {
        const char* preset;
        const char* keywords;        // for picking the examples that match the words
        const char* description;
        const char* summary;
        ExampleChange first, second;
    };

    /** Everything that makes one synth different. Built once per synth, in its own file. */
    struct Table
    {
        juce::String synthName;                   // "DawIt Subtractive"
        std::vector<KnobDesc> knobs;              // by knob index
        std::vector<StartingPoint> starts;        // Init first
        std::vector<Example> examples;
        juce::StringArray fallbackExamples;       // when the words match fewer than three examples
        juce::String promptHead;                  // the system prompt before the table of knobs
        juce::StringArray extraKnobRows;          // rows added to the table (the harmonic bars, as one row)
        juce::String followUpExamples;            // the follow-up examples in the user prompt
        juce::StringArray groupTitles;            // by group number: "Harmonics"
        std::function<juce::String (int knob)> memberLabel;   // a group member's short name: "3rd" (the sentences say "the 3rd")

        /** A value as the knob shows it ("1.80 kHz"). */
        std::function<juce::String (int knob, float value)> format;

        /** A choice from lower-case words, or -1. */
        std::function<int (int knob, const juce::String& lowerText)> readChoice;

        /** A sound in words for the prompt: the knobs that differ from `skipEqualTo` (all when null) and the rest. */
        std::function<juce::String (const Values&, const Values* skipEqualTo)> describeValues;

        /** Advice on the words against the result. */
        std::function<juce::StringArray (const juce::String& words, bool followUp, const Values& base, const Values& result)> wordNotes;

        /** What this synth can't do that the words ask for. */
        std::function<juce::StringArray (const juce::String& words)> limitNotes;

        int getNumKnobs() const noexcept { return (int) knobs.size(); }
        const Values& getInit() const { return starts.front().values; }
    };

    /** The knob's id in the prompt and the reply. */
    juce::String getKnobId (const Table&, int knob);

    /** A knob from its id; case, spaces, hyphens and underscores don't matter ("Amp attack" is "ampAttack"). */
    bool getKnobFromId (const Table&, const juce::String& id, int& result);

    //==============================================================================
    /** A value the model wrote, read as the knob's own number (see the synth's getKnobValue). */
    struct ValueRead
    {
        bool ok = false;
        float value = 0.0f;
        juce::String note;   // DawIt's: "The model asked for 40 kHz; the knob stops at 20 kHz." when it was clamped
    };

    /** Reads "1.2 kHz", "800 ms", "60%", "+7 st", "12 cents", "2.5 oct", "-30 dB", "Off", or for a choice "lowpass",
        "pulse", "legato", "24 dB/oct". Out-of-range values are clamped with a note; a step is rounded. */
    ValueRead readValue (const Table&, int knob, const juce::String& text);

    /** As above for a JSON value: a string, or a bare number in the knob's usual unit (Hz, seconds, percent, ...). */
    ValueRead readValue (const Table&, int knob, const juce::var&);

    /** A value as the knob shows it ("1.80 kHz"), for any number in the knob's own units. */
    juce::String formatKnobValue (const Table&, int knob, float value);

    //==============================================================================
    /** What goes into a request, as plain copies taken on the message thread so a worker can use it. */
    struct Request
    {
        juce::String words;                // "a warm pad that swells in slowly"
        bool followUp = false;             // change the current sound; otherwise pick the closest starting point
        Values current;                    // the sound in the window now
        int seed = 0;                      // 0 lets Ollama pick one
        juce::String model;                // for the record
    };

    /** The rules (use only the listed knobs, give units, change few, one sentence each, say what the synth lacks)
        and the table of knobs: id, name, range and unit, and the usual settings. */
    juce::String buildSystemPrompt (const Table&);

    /** The starting points with their values, worked examples (three, picked by words in common with the
        description, the closest last), the two follow-up examples, the current sound and the description. */
    juce::String buildUserPrompt (const Table&, const Request&);

    /** The JSON schema the reply follows: `startFrom` (the starting points or "Current sound"), `summary`, and
        up to 10 `changes` of `knob` (the ids), `value` and `reason`. */
    juce::var buildSchema (const Table&);

    /** The names of the worked examples the prompt would use for these words, best match first. */
    juce::StringArray pickExamples (const Table&, const juce::String& words);

    /** The indices into the table's examples that go with these words, best match first (three of them). */
    std::vector<int> pickExampleIndices (const Table&, const juce::String& words);

    /** The result of reading an answer. */
    struct Result
    {
        bool ok = false;
        juce::String error;            // "The model's answer couldn't be read. Ask again, or pick another model."
        DescribedSound sound;
        int unknownKnobs = 0;          // named knobs this synth doesn't have
        int droppedEqual = 0;          // changes that equal the base value
        int droppedExtra = 0;          // changes beyond the 10th
        int droppedGroup = 0;          // members of a group beyond the 12th
    };

    /** Reads the model's reply. The base is the current sound for a follow-up, else the named starting point
        (an unknown name falls back to the current sound, with a note). Changes come out in the window's order. */
    Result readReply (const Table&, const Request&, const juce::String& reply);

    /** How many changes the answer has for the user: a whole group (the harmonic bars) counts as one. */
    int countChanges (const Table&, const DescribedSound&);

    /** The sound the changes make: the base with every change applied. */
    Values getResultValues (const DescribedSound&);

    //==============================================================================
    /** Shared helpers for the tables' word and limit notes. */
    juce::String padWords (const juce::String& words);                       // " a warm pad " for whole-word matching
    bool hasWord (const juce::String& paddedWords, const juce::String& phrase);
    bool hasAny (const juce::String& paddedWords, std::initializer_list<const char*> phrases);
    juce::String formatCentroid (double hz);                                  // "1.73 kHz" or "220 Hz"
    juce::String tidy (juce::String);                                         // "20.0 kHz" becomes "20 kHz"
    juce::String firstSentence (const juce::String&);
    bool nearlyEqual (float a, float b);

    //==============================================================================
    /** The walkthrough: the changes one step at a time in the window's order (a group of knobs, such as the
        harmonic bars, is one step), with a first step that loads the starting point when the base is not the
        sound in the window. It works on the values the caller holds (`live`), which the caller then sets on the synth. */
    class Walkthrough
    {
    public:
        struct Step
        {
            bool loadsBase = false;      // "Load the starting point Warm pad"
            int knob = 0;                // the step's knob, or its first knob in a group
            float before = 0.0f, target = 0.0f;   // of that knob, in its own units
            int group = -1;
            std::vector<DescribedChange> members;   // every change of the step: one for a knob, the bars for a group
            juce::String reason, note;
            Values snapshot;             // the values when the step became the current one, for Back
            bool taken = false;          // the snapshot is valid
        };

        Walkthrough (const Table&, const DescribedSound&, const Values& live);

        int getNumSteps() const noexcept { return (int) steps.size(); }
        int getIndex() const noexcept { return index; }
        bool isFinished() const noexcept { return index >= getNumSteps(); }
        const Step& getStep (int i) const { return steps[(size_t) i]; }

        /** Within 2% of the knob's travel (the log scale for the cutoff and the times); a choice must be exact;
            a target of Off, for a knob with offExact, only at Off. */
        static bool isReached (const Table&, int knob, float value, float target);

        /** Whether every knob of the current step is at its target in `live` (a load-base step: never, it is a click). */
        bool isCurrentReached (const Values& live) const;

        /** The user got there: keep their values and go on. */
        void markDone (const Values& live);

        /** "Set it for me": set the targets (or load the base) and go on. */
        void setForUser (Values& live);

        /** Leave these knobs as they are and go on. */
        void skip (const Values& live);

        /** Go back a step and put its knobs back as they were when that step began. */
        void back (Values& live);

        /** Set every knob that is left and end. */
        void finishAll (Values& live);

    private:
        void begin (const Values& live);
        void advance (const Values& live);

        const Table& table;
        std::vector<Step> steps;
        Values baseValues;
        int index = 0;
    };

    /** The last 20 sounds from before a change, for the bar's Undo (the app's undo doesn't cover synth knobs). */
    class ValuesUndo
    {
    public:
        static constexpr int maxSteps = 20;

        void push (const Values&);
        bool canUndo() const noexcept { return ! list.empty(); }
        bool pop (Values& result);
        void clear() { list.clear(); }
        int size() const noexcept { return (int) list.size(); }

    private:
        std::vector<Values> list;
    };

    /** Whether two sounds are the same knob for knob (within half a knob's step, as the synth rounds a value to its step). */
    bool areSame (const Table&, const Values&, const Values&);
} // namespace sounddescribe
} // namespace daw::gen
