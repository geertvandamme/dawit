#include "OllamaClient.h"

namespace daw::gen
{
namespace
{
    juce::String unreachableMessage (const juce::String& url)
    {
        return "Can't reach Ollama at " + url + ". Start the Ollama app (or run \"ollama serve\") and try again.";
    }

    juce::var makeObject (std::initializer_list<std::pair<const char*, juce::var>> properties)
    {
        auto* o = new juce::DynamicObject();
        for (auto& [name, value] : properties)
            o->setProperty (name, value);

        return juce::var (o);
    }
}

juce::StringArray OllamaClient::listModels (juce::String& error) const
{
    int status = 0;
    auto stream = juce::URL (url + "/api/tags")
                      .createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                              .withConnectionTimeoutMs (3000)
                                              .withStatusCode (&status));

    if (stream == nullptr || status != 200)
    {
        error = unreachableMessage (url);
        return {};
    }

    const auto json = juce::JSON::parse (stream->readEntireStreamAsString());
    juce::StringArray names;

    if (auto* models = json["models"].getArray())
    {
        for (auto& m : *models)
        {
            // Embedding models (BERT family) can't chat.
            const auto family = m["details"]["family"].toString();
            const auto name = m["name"].toString();

            if (name.isNotEmpty() && ! family.containsIgnoreCase ("bert") && ! name.containsIgnoreCase ("embed"))
                names.add (name);
        }
    }

    names.sortNatural();
    return names;
}

juce::String OllamaClient::chat (const ChatOptions& options, juce::String& error) const
{
    auto reply = chat (options, options.think, error);

    // Ollama refuses "think" for models that can't think (such as qwen2.5): ask them plainly.
    if (options.think && error.contains ("does not support thinking"))
    {
        if (options.onThinkingUnsupported)
            options.onThinkingUnsupported();

        error.clear();
        reply = chat (options, false, error);
    }

    return reply;
}

juce::String OllamaClient::generateRaw (const ChatOptions& options, juce::String& error) const
{
    if (options.shouldStop && options.shouldStop())
    {
        error = "Cancelled.";
        return {};
    }

    auto sampling = makeObject ({ { "temperature", options.temperature }, { "num_predict", options.maxTokens },
                                  { "num_ctx", 2048 }, { "top_k", 40 }, { "top_p", 0.9 }, { "repeat_penalty", 1.1 },
                                  { "stop", juce::var (juce::Array<juce::var> { "</s>", "Human:" }) } });

    if (options.seed != 0)
        sampling.getDynamicObject()->setProperty ("seed", options.seed);

    const auto body = makeObject ({ { "model", options.model }, { "prompt", options.userPrompt },
                                    { "raw", true }, { "stream", false }, { "format", options.schema }, { "keep_alive", "15m" },
                                    { "options", sampling } });
    int status = 0;
    auto stream = juce::URL (url + "/api/generate")
                      .withPOSTData (juce::JSON::toString (body, true))
                      .createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                                              .withExtraHeaders ("Content-Type: application/json")
                                              .withConnectionTimeoutMs (180000)
                                              .withStatusCode (&status));

    if (stream == nullptr)
    {
        error = unreachableMessage (url);
        return {};
    }

    if (status != 200)
    {
        const auto reply = juce::JSON::parse (stream->readEntireStreamAsString());
        error = "Ollama error: " + (reply["error"].toString().isNotEmpty() ? reply["error"].toString()
                                                                           : "HTTP " + juce::String (status));
        return {};
    }

    const auto reply = juce::JSON::parse (stream->readEntireStreamAsString());
    if (options.shouldStop && options.shouldStop())
        error = "Cancelled.";
    else if (! reply.isObject() || ! (bool) reply["done"])
        error = "The connection ended before the model finished.";
    else if (reply["done_reason"].toString() == "length")
        error = "The model's reply was cut short. Try again.";
    else if (reply["response"].toString().isEmpty())
        error = "The model returned an empty reply.";
    return error.isEmpty() ? reply["response"].toString() : juce::String();
}

juce::String OllamaClient::chat (const ChatOptions& options, bool think, juce::String& error) const
{
    const auto messages = juce::Array<juce::var> {
        makeObject ({ { "role", "system" }, { "content", options.systemPrompt } }),
        makeObject ({ { "role", "user" }, { "content", options.userPrompt } })
    };

    auto sampling = makeObject ({ { "temperature", options.temperature },
                                  { "num_predict", options.maxTokens },
                                  { "num_ctx", juce::jmax (8192, options.maxTokens + 4096) } }); // room for the prompt

    if (options.seed != 0)
        sampling.getDynamicObject()->setProperty ("seed", options.seed);

    const auto body = makeObject ({
        { "model", options.model },
        { "messages", messages },
        { "format", options.schema },
        { "stream", true },
        { "keep_alive", "15m" },
        { "think", think }, // without it, models that can think do so, which is slow and can use up the reply limit
        { "options", sampling }
    });

    int status = 0;
    // The timeout also covers loading the model into memory on the first request.
    auto stream = juce::URL (url + "/api/chat")
                      .withPOSTData (juce::JSON::toString (body, true))
                      .createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                                              .withExtraHeaders ("Content-Type: application/json")
                                              .withConnectionTimeoutMs (180000)
                                              .withStatusCode (&status));

    if (stream == nullptr)
    {
        error = unreachableMessage (url);
        return {};
    }

    if (status != 200)
    {
        const auto reply = juce::JSON::parse (stream->readEntireStreamAsString());
        error = "Ollama error: " + (reply["error"].toString().isNotEmpty() ? reply["error"].toString()
                                                                           : "HTTP " + juce::String (status));
        return {};
    }

    // Streaming replies are one JSON object per line.
    juce::String content;
    int thinkingLength = 0;
    bool cutOff = false;

    while (! stream->isExhausted())
    {
        if (options.shouldStop && options.shouldStop())
        {
            error = "Cancelled.";
            return {};
        }

        const auto line = stream->readNextLine().trim();

        if (line.isEmpty())
            continue;

        const auto chunk = juce::JSON::parse (line);

        if (chunk["error"].toString().isNotEmpty())
        {
            error = "Ollama error: " + chunk["error"].toString();
            return {};
        }

        if (const auto thinking = chunk["message"]["thinking"].toString(); thinking.isNotEmpty())
        {
            thinkingLength += thinking.length();

            if (options.onThinking)
                options.onThinking (thinkingLength);
        }

        if (const auto text = chunk["message"]["content"].toString(); text.isNotEmpty())
        {
            content << text;

            if (options.onProgress)
                options.onProgress (content.length());
        }

        if ((bool) chunk["done"])
        {
            cutOff = chunk["done_reason"].toString() == "length";
            break;
        }
    }

    if (content.isEmpty() && cutOff && think)
        error = "The model thought for so long that it ran out of room before answering. Try again, or turn off Think first.";
    else if (content.isEmpty())
        error = "The model returned an empty reply.";

    return content;
}
} // namespace daw::gen
