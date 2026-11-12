#include "AiSetup.h"

namespace daw::gen::aisetup
{
juce::String getModeId (Mode mode)
{
    switch (mode)
    {
        case Mode::pattern: return "pattern";
        case Mode::ai:      return "ai";
    }

    return "pattern";
}

Mode getModeFromId (const juce::String& id)
{
    // "chatMusician" is the name of a mode DawIt no longer has; people who used it get the AI part.
    return id == "ai" || id == "chatMusician" ? Mode::ai : Mode::pattern;
}

Mode migrateMode (bool oldSwitchOn, const juce::String& oldModel)
{
    return oldSwitchOn || oldModel.trim().isNotEmpty() ? Mode::ai : Mode::pattern;
}

void tidySettings (juce::PropertySet& settings)
{
    if (settings.containsKey ("generator.mode"))
        settings.setValue ("generator.mode", getModeId (getModeFromId (settings.getValue ("generator.mode"))));

    for (auto* key : { "generator.chatMusicianModel", "generator.chatTask", "generator.modelChecks" })
        settings.removeValue (key);
}

//==============================================================================
SetupInfo describe (Mode mode, const OllamaClient::ModelList& list, const juce::String& url, const juce::String& chosenModel)
{
    SetupInfo info;

    if (mode == Mode::pattern)
        return info;

    const auto chosen = chosenModel.trim();
    info.offersRetry = true;

    if (! list.reachable)
    {
        info.state = State::unreachable;
        info.status = "Ollama isn't running at " + url + ".";
        return info;
    }

    if (list.models.empty())
    {
        info.state = State::noModels;
        info.status = "Ollama is running but has no models installed.";
        return info;
    }

    if (chosen.isNotEmpty() && list.find (chosen) == nullptr)
    {
        info.state = State::notInstalled;
        info.status = "'" + chosen + "' isn't installed in Ollama.";
        return info;
    }

    info.offersRetry = false;
    const auto count = (int) list.models.size();
    info.status = "Connected to Ollama " + juce::String::fromUTF8 ("\xc2\xb7") + " " + juce::String (count) + (count == 1 ? " model" : " models");
    info.state = State::ready;
    info.ready = true;
    return info;
}

juce::StringArray getSetupSteps()
{
    return { "1. Install Ollama from ollama.com.",
             "2. Start it: open the Ollama app, or run: ollama serve",
             "3. Get a model: ollama pull qwen2.5:7b (a 4.7 GB download; any chat model works).",
             "4. Press Retry." };
}
} // namespace daw::gen::aisetup
