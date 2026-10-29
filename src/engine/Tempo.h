#pragma once

#include <juce_core/juce_core.h>

#include <optional>

namespace daw
{
/** A tempo as people write it: to two decimals at most, without trailing zeros, such as "120",
    "97.5" or "96.84". Detected tempos are seldom whole numbers, and a hundredth of a BPM still
    adds up: 0.1 BPM off moves a 4-minute song by half a beat at its end. */
inline juce::String formatBpm (double bpm)
{
    auto text = juce::String (std::round (bpm * 100.0) / 100.0, 2);

    if (text.containsChar ('.'))
        text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

    return text;
}

/** A typed tempo, with a decimal point or a comma ("97.5", "97,5", "120 BPM"); nullopt when it
    isn't a number. */
inline std::optional<double> parseBpm (const juce::String& typed)
{
    const auto text = typed.trim().replaceCharacter (',', '.').upToFirstOccurrenceOf ("bpm", false, true).trim();

    if (text.isEmpty() || ! text.containsOnly ("0123456789.") || text.indexOfChar ('.') != text.lastIndexOfChar ('.') || text == ".")
        return std::nullopt;

    return text.getDoubleValue();
}
} // namespace daw
