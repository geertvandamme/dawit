#pragma once

#include "OllamaClient.h"


namespace daw::gen::aisetup
{
/** How the Generate panel makes a part: from DawIt's own patterns, or with a local AI model. */
enum class Mode { pattern, ai };

/** The saved name of a mode (`generator.mode`): "pattern" or "ai". */
juce::String getModeId (Mode);

/** The mode a saved name stands for. An unknown name gives `Mode::pattern`; the name of a removed mode ("chatMusician")
    gives `Mode::ai`. */
Mode getModeFromId (const juce::String&);

/** The first mode to show when `generator.mode` was never saved: the AI part when the old switch was on or a model had
    been chosen (`generator.model`), otherwise the built-in pattern. */
Mode migrateMode (bool oldSwitchOn, const juce::String& oldModel);

/** Tidies the app settings once on load: maps a saved mode of a removed kind to the AI part and removes the settings that
    nothing reads any more. Other keys are untouched. */
void tidySettings (juce::PropertySet&);

//==============================================================================
/** Where the AI of a mode stands, for the Generate panel's status line. */
enum class State
{
    notNeeded,       // Built-in pattern: no AI
    unreachable,     // Ollama didn't answer
    noModels,        // Ollama answers but has no chat models
    notInstalled,    // the chosen model isn't in the list
    ready
};

struct SetupInfo
{
    State state = State::notNeeded;
    juce::String status;      // the status line ("Connected to Ollama · 3 models")
    bool ready = false;       // Generate can start now
    bool offersRetry = false; // Retry, Set up... and the link to Built-in pattern: a problem with Ollama or the model

    /** True when Generate may start. */
    bool canStart() const noexcept { return state == State::ready || state == State::notNeeded; }
};

/** Maps what Ollama reported and the chosen model to a state and its texts. Pure. */
SetupInfo describe (Mode mode, const OllamaClient::ModelList& list, const juce::String& url, const juce::String& chosenModel);

/** The steps of the Set up... callout, one per line ("1. Install Ollama from ollama.com."). */
juce::StringArray getSetupSteps();
} // namespace daw::gen::aisetup
