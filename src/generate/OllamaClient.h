#pragma once

#include <juce_core/juce_core.h>

namespace daw::gen
{
/**
    Minimal client for a local Ollama server (https://ollama.com).
    All calls block, so run them on a background thread.
*/
class OllamaClient
{
public:
    explicit OllamaClient (juce::String baseUrl = "http://localhost:11434") : url (std::move (baseUrl)) {}

    const juce::String& getUrl() const noexcept { return url; }

    /** Names of installed chat models (embedding-only models are skipped). */
    juce::StringArray listModels (juce::String& error) const;

    struct ChatOptions
    {
        juce::String model;
        juce::String systemPrompt, userPrompt;
        juce::var schema;                       // JSON schema the reply must follow
        double temperature = 0.8;
        int seed = 0;                           // sampling seed; 0 lets Ollama pick one
        int maxTokens = 4096;                   // reply limit, thinking included
        bool think = false;                     // let reasoning models (qwen3, gemma4, ...) think before answering
        std::function<bool()> shouldStop;       // polled while streaming
        std::function<void (int)> onProgress;   // characters of the reply received so far
        std::function<void (int)> onThinking;   // characters of thinking received so far
        std::function<void()> onThinkingUnsupported; // the model can't think: it answers straight away instead
    };

    /** Streams a chat completion and returns the full reply text, or an empty string with `error` set.
        If thinking is asked for but the model can't think, it asks again without. */
    juce::String chat (const ChatOptions&, juce::String& error) const;

    /** Sends a raw, fully formatted prompt with a JSON schema. A truncated reply is an error. */
    juce::String generateRaw (const ChatOptions&, juce::String& error) const;

private:
    juce::String chat (const ChatOptions&, bool think, juce::String& error) const;

    juce::String url;
};
} // namespace daw::gen
