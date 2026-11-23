#pragma once

#include "engine/BuiltInSubtractive.h"

#include <array>
#include <vector>

namespace daw::gen
{
/**
    The words behind "Describe a sound" in DawIt Subtractive, without any UI and without needing a model.

    The user types what they want to hear ("a warm pad that swells in slowly"). `buildSystemPrompt`,
    `buildUserPrompt` and `buildSchema` make what goes to Ollama: the rules, the 28 knobs with their ranges and
    usual settings, the starting points with their values, worked examples and the current sound. `readReply`
    turns the answer into a described sound (`sub::DescribedSound`): DawIt reads every value itself (a number with
    its unit, or a choice by name), keeps it inside the knob's range, drops knobs the synth doesn't have and
    changes that equal the base, and checks the words against the result (`getWordNotes`, `getLimitNotes`). A
    language model writes likely text and does not hear sound, so none of this trusts it. `Walkthrough` goes
    through the changes one knob at a time; `SettingsUndo` keeps the last 20 settings for the bar's Undo.
    Nothing here touches the edit or the engine lock.
*/
namespace sounddescribe
{
    /** The temperature the request uses: low, because the answer must follow the table of knobs. */
    constexpr double temperature = 0.3;

    /** The most changes an answer may have; more are cut. */
    constexpr int maxChanges = 10;

    /** The `startFrom` value that means "change the sound in the window now". */
    inline const char* currentSoundName = "Current sound";

    /** The knob's id in the prompt and the reply (the parameter's id: "cutoff", "ampAttack"). */
    juce::String getKnobId (sub::Knob);

    /** A knob from its id; case, spaces, hyphens and underscores don't matter ("Amp attack" is "ampAttack"). */
    bool getKnobFromId (const juce::String& id, sub::Knob& result);

    /** The knob's name with its part of the window: "Oscillator 1 wave", "Filter envelope attack". */
    juce::String getKnobTitle (sub::Knob);

    //==============================================================================
    /** A value the model wrote, read as the knob's own number (see sub::getKnobValue). */
    struct ValueRead
    {
        bool ok = false;
        float value = 0.0f;
        juce::String note;   // DawIt's: "The model asked for 40 kHz; the knob stops at 20 kHz." when it was clamped
    };

    /** Reads "1.2 kHz", "800 ms", "60%", "+7 st", "12 cents", "2.5 oct", "-30 dB", "Off", or for a choice "lowpass",
        "pulse", "legato", "24 dB/oct". Out-of-range values are clamped with a note; a step is rounded. */
    ValueRead readValue (sub::Knob, const juce::String& text);

    /** As above for a JSON value: a string, or a bare number in the knob's usual unit (Hz, seconds, percent, ...). */
    ValueRead readValue (sub::Knob, const juce::var&);

    /** A value as the knob shows it ("1.80 kHz"), for any number in the knob's own units. */
    juce::String formatKnobValue (sub::Knob, float value);

    //==============================================================================
    /** What goes into a request, as plain copies taken on the message thread so a worker can use it. */
    struct Request
    {
        juce::String words;                // "a warm pad that swells in slowly"
        bool followUp = false;             // change the current sound; otherwise pick the closest starting point
        sub::Settings current;             // the sound in the window now
        int seed = 0;                      // 0 lets Ollama pick one
        juce::String model;                // for the record
    };

    /** The rules (use only the listed knobs, give units, change few, one sentence each, say what the synth lacks)
        and the table of knobs: id, name, range and unit, and the usual settings. */
    juce::String buildSystemPrompt();

    /** The starting points with their values, worked examples (three, picked by words in common with the
        description), the two follow-up examples, the current sound and the description. */
    juce::String buildUserPrompt (const Request&);

    /** The JSON schema the reply follows: `startFrom` (17 starting points or "Current sound"), `summary`, and
        up to 10 `changes` of `knob` (the 28 ids), `value` and `reason`. */
    juce::var buildSchema();

    /** The names of the worked examples the prompt would use for these words, best match first. */
    juce::StringArray pickExamples (const juce::String& words);

    /** The result of reading an answer. */
    struct Result
    {
        bool ok = false;
        juce::String error;            // "The model's answer couldn't be read. Ask again, or pick another model."
        sub::DescribedSound sound;
        int unknownKnobs = 0;          // named knobs this synth doesn't have
        int droppedEqual = 0;          // changes that equal the base value
        int droppedExtra = 0;          // changes beyond the 10th
    };

    /** Reads the model's reply. The base is the current sound for a follow-up, else the named starting point
        (an unknown name falls back to the current sound, with a note). Changes come out in the window's order. */
    Result readReply (const Request&, const juce::String& reply);

    /** The sound the changes make: the base with every change applied. */
    sub::Settings getResultSettings (const sub::DescribedSound&);

    //==============================================================================
    /** Advice on the words against the result (a pluck that doesn't fade, a "brighter" that got darker). Never blocks an answer. */
    juce::StringArray getWordNotes (const juce::String& words, bool followUp, const sub::Settings& base, const sub::Settings& result);

    /** What this synth can't do that the words ask for (an 808's falling pitch, bells, a real piano, effects), with where to find it. */
    juce::StringArray getLimitNotes (const juce::String& words);

    //==============================================================================
    /** The walkthrough: the changes one knob at a time in the window's order, with a first step that loads the
        starting point when the base is not the sound in the window. It works on the settings the caller holds
        (`live`), which the caller then sets on the synth. */
    class Walkthrough
    {
    public:
        struct Step
        {
            bool loadsBase = false;      // "Load the starting point Warm pad"
            sub::Knob knob = sub::Knob::cutoff;
            float before = 0.0f, target = 0.0f;   // the knob's own units
            juce::String reason, note;
            sub::Settings snapshot;      // the settings when the step became the current one, for Back
            bool taken = false;          // the snapshot is valid
        };

        Walkthrough (const sub::DescribedSound&, const sub::Settings& live);

        int getNumSteps() const noexcept { return (int) steps.size(); }
        int getIndex() const noexcept { return index; }
        bool isFinished() const noexcept { return index >= getNumSteps(); }
        const Step& getStep (int i) const { return steps[(size_t) i]; }

        /** Within 2% of the knob's travel (the log scale for the cutoff and the times); a choice must be exact. */
        static bool isReached (sub::Knob, float value, float target);

        /** Whether the current step's knob is at its target in `live` (a load-base step: never, it is a click). */
        bool isCurrentReached (const sub::Settings& live) const;

        /** The user got there: keep their value and go on. */
        void markDone (const sub::Settings& live);

        /** "Set it for me": set the target (or load the base) and go on. */
        void setForUser (sub::Settings& live);

        /** Leave this knob as it is and go on. */
        void skip (const sub::Settings& live);

        /** Go back a step and put its knob back as it was when that step began. */
        void back (sub::Settings& live);

        /** Set every knob that is left and end. */
        void finishAll (sub::Settings& live);

    private:
        void begin (const sub::Settings& live);
        void advance (const sub::Settings& live);

        std::vector<Step> steps;
        sub::Settings baseSettings;
        int index = 0;
    };

    /** The last 20 settings from before a change, for the bar's Undo (the app's undo doesn't cover synth knobs). */
    class SettingsUndo
    {
    public:
        static constexpr int maxSteps = 20;

        void push (const sub::Settings&);
        bool canUndo() const noexcept { return ! list.empty(); }
        bool pop (sub::Settings& result);
        void clear() { list.clear(); }
        int size() const noexcept { return (int) list.size(); }

    private:
        std::vector<sub::Settings> list;
    };

    /** Whether two sets of settings are the same knob for knob. */
    bool areSame (const sub::Settings&, const sub::Settings&);
} // namespace sounddescribe
} // namespace daw::gen
