#pragma once

#include "OllamaClient.h"

#include <map>
#include <optional>

namespace daw::gen::aisetup
{
/** How the Generate panel makes a part: from DawIt's own patterns, with a local AI model, or with a ChatMusician model. */
enum class Mode { pattern, ai, chatMusician };

/** The saved name of a mode (`generator.mode`): "pattern", "ai" or "chatMusician". */
juce::String getModeId (Mode);

/** The mode a saved name stands for; an unknown name gives `Mode::pattern`. */
Mode getModeFromId (const juce::String&);

/** The first mode to show when `generator.mode` was never saved: ChatMusician when the old `generator.chatMusician` switch
    was on, otherwise the AI part when a model had been chosen (`generator.model`), otherwise the built-in pattern. */
Mode migrateMode (bool oldChatMusicianOn, const juce::String& oldModel);

//==============================================================================
/** The models of the list, split for the ChatMusician model box: names containing "chatmusician" (any case) first, the
    rest after ("Other models (not ChatMusician by name)"). Each part keeps the list's order. */
struct OrderedModels
{
    std::vector<OllamaClient::ModelInfo> likely, others;
};

OrderedModels orderForChatMusician (const std::vector<OllamaClient::ModelInfo>&);

//==============================================================================
/** The result of the one-bar format check of a model, remembered by the model's digest so a re-pulled or changed model is
    checked again. A pass shows the model follows the format; it is no proof that the model is ChatMusician. */
struct CheckRecord
{
    bool passed = false;
    juce::String date;       // "2026-11-11"
};

class ModelChecks
{
public:
    /** The record for a digest, or none when the model was never checked. */
    std::optional<CheckRecord> get (const juce::String& digest) const;

    /** Remembers a result for a digest (today's date when `date` is empty). Empty digests are ignored. */
    void set (const juce::String& digest, bool passed, const juce::String& date = {});

    /** The saved form (`generator.modelChecks`): a small JSON object of digest to "passed|failed yyyy-mm-dd". */
    juce::String toString() const;
    static ModelChecks fromString (const juce::String&);

private:
    std::map<juce::String, CheckRecord> records;
};

/** "2026-11-11" as "11 Nov 2026"; the text unchanged when it isn't a date. */
juce::String formatCheckDate (const juce::String& isoDate);

//==============================================================================
/** Where the AI of a mode stands, for the Generate panel's status line. */
enum class State
{
    notNeeded,       // Built-in pattern: no AI
    unreachable,     // Ollama didn't answer
    noModels,        // Ollama answers but has no chat models
    noChoice,        // ChatMusician mode with no model chosen
    notInstalled,    // the chosen model isn't in the list
    needsCheck,      // ChatMusician model not tried yet: Generate runs the one-bar check first
    checkFailed,     // ChatMusician model failed the one-bar check
    ready
};

struct SetupInfo
{
    State state = State::notNeeded;
    juce::String status;      // the status line ("Connected to Ollama · 3 models")
    juce::String checkLine;   // ChatMusician only: the line about the one-bar check; empty otherwise
    bool ready = false;       // Generate can start now (an unchecked model is not ready: it is checked first)
    bool offersRetry = false; // Retry, Set up... and the link to Built-in pattern: a problem with Ollama or the model
    bool offersTestAgain = false;   // the check failed

    /** True when Generate may start, running the check first if the model is unchecked. */
    bool canStart() const noexcept { return state == State::ready || state == State::needsCheck || state == State::notNeeded; }
};

/** Maps what Ollama reported, the chosen model and the saved check results to a state and its texts. `chosenModel` is the
    AI part's model or the ChatMusician model, depending on `mode`. Pure. */
SetupInfo describe (Mode mode, const OllamaClient::ModelList& list, const juce::String& url,
                    const juce::String& chosenModel, const ModelChecks& checks);

/** The steps of the Set up... callout, one per line ("1. Install Ollama from ollama.com."), for the mode. In ChatMusician
    mode step 3 names no download. */
juce::StringArray getSetupSteps (Mode mode);

//==============================================================================
/** Asks a model for one bar over C (the ChatMusician prompt and reply format) and says whether the reply follows the format.
    `request` gets the prompt and the schema, and returns the raw reply or sets its error. On failure `error` says why. */
bool runOneBarCheck (const std::function<juce::String (const juce::String&, const juce::var&, juce::String&)>& request,
                     juce::String& error);
} // namespace daw::gen::aisetup
