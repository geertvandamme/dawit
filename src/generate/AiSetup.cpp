#include "AiSetup.h"
#include "ChatMusician.h"

namespace daw::gen::aisetup
{
juce::String getModeId (Mode mode)
{
    switch (mode)
    {
        case Mode::pattern:      return "pattern";
        case Mode::ai:           return "ai";
        case Mode::chatMusician: return "chatMusician";
    }

    return "pattern";
}

Mode getModeFromId (const juce::String& id)
{
    if (id == "ai")
        return Mode::ai;

    return id == "chatMusician" ? Mode::chatMusician : Mode::pattern;
}

Mode migrateMode (bool oldChatMusicianOn, const juce::String& oldModel)
{
    if (oldChatMusicianOn)
        return Mode::chatMusician;

    return oldModel.trim().isNotEmpty() ? Mode::ai : Mode::pattern;
}

OrderedModels orderForChatMusician (const std::vector<OllamaClient::ModelInfo>& models)
{
    OrderedModels ordered;

    for (auto& m : models)
        (m.name.containsIgnoreCase ("chatmusician") ? ordered.likely : ordered.others).push_back (m);

    return ordered;
}

//==============================================================================
std::optional<CheckRecord> ModelChecks::get (const juce::String& digest) const
{
    const auto it = records.find (digest);
    return it != records.end() ? std::optional<CheckRecord> (it->second) : std::nullopt;
}

void ModelChecks::set (const juce::String& digest, bool passed, const juce::String& date)
{
    if (digest.isEmpty())
        return;

    records[digest] = { passed, date.isNotEmpty() ? date : juce::Time::getCurrentTime().formatted ("%Y-%m-%d") };
}

juce::String ModelChecks::toString() const
{
    auto* object = new juce::DynamicObject();

    for (auto& [digest, record] : records)
        object->setProperty (digest, (record.passed ? "passed " : "failed ") + record.date);

    return juce::JSON::toString (juce::var (object), true);
}

ModelChecks ModelChecks::fromString (const juce::String& text)
{
    ModelChecks checks;

    const auto json = juce::JSON::parse (text);   // keeps the object alive

    if (auto* object = json.getDynamicObject())
    {
        for (auto& property : object->getProperties())
        {
            const auto value = property.value.toString();
            checks.records[property.name.toString()] = { value.startsWith ("passed"), value.fromFirstOccurrenceOf (" ", false, false) };
        }
    }

    return checks;
}

juce::String formatCheckDate (const juce::String& isoDate)
{
    static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    const auto parts = juce::StringArray::fromTokens (isoDate, "-", "");

    if (parts.size() != 3 || parts[0].length() != 4 || ! parts[1].containsOnly ("0123456789") || ! parts[2].containsOnly ("0123456789"))
        return isoDate;

    const int month = parts[1].getIntValue(), day = parts[2].getIntValue();
    return juce::isPositiveAndBelow (month - 1, 12) && day > 0 ? juce::String (day) + " " + months[month - 1] + " " + parts[0] : isoDate;
}

//==============================================================================
SetupInfo describe (Mode mode, const OllamaClient::ModelList& list, const juce::String& url,
                    const juce::String& chosenModel, const ModelChecks& checks)
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

    if (chosen.isEmpty() && mode == Mode::chatMusician)
    {
        info.state = State::noChoice;
        info.status = "Choose a ChatMusician model.";
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

    if (mode == Mode::chatMusician)
    {
        const auto record = checks.get (list.find (chosen)->digest);

        if (! record.has_value())
        {
            info.state = State::needsCheck;
            info.ready = false;
            info.checkLine = "Not checked yet: DawIt will ask it for one test bar first.";
        }
        else if (record->passed)
        {
            info.checkLine = "Passed the one-bar test on " + formatCheckDate (record->date)
                           + ". This shows it follows the format, not that its music is good.";
        }
        else
        {
            info.state = State::checkFailed;
            info.ready = false;
            info.offersTestAgain = true;
            info.checkLine = "Failed the one-bar test: its reply didn't follow the format. It may not be a ChatMusician model.";
        }
    }

    return info;
}

juce::StringArray getSetupSteps (Mode mode)
{
    juce::StringArray steps;
    steps.add ("1. Install Ollama from ollama.com.");
    steps.add ("2. Start it: open the Ollama app, or run: ollama serve");

    if (mode == Mode::chatMusician)
        steps.add ("3. Install a ChatMusician model in Ollama yourself. DawIt doesn't name a download yet, because its licence and "
                   "origin haven't been checked. Then choose it here; DawIt tries it with a one-bar test before first use.");
    else
        steps.add ("3. Get a model: ollama pull qwen2.5:7b (a 4.7 GB download; any chat model works).");

    steps.add ("4. Press Retry.");
    return steps;
}

//==============================================================================
bool runOneBarCheck (const std::function<juce::String (const juce::String&, const juce::var&, juce::String&)>& request,
                     juce::String& error)
{
    const MusicalKey key { 0, MusicalKey::Scale::major };
    const std::vector<Chord> chords { Chord { 0, { 0, 4, 7 } } };   // C
    juce::String requestError;
    const auto reply = request (chatmusician::buildPrompt (key, chords), chatmusician::buildSchema (1), requestError);

    if (requestError.isNotEmpty())
    {
        error = requestError;
        return false;
    }

    std::vector<MidiNote> notes;
    return chatmusician::parseReply (reply, 1, 0.0, notes, error);
}
} // namespace daw::gen::aisetup
