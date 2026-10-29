#pragma once

#include <juce_core/juce_core.h>

/** Shared word matching for track-name advice, without matching short names inside other words. */
namespace daw::trackname
{
    /** What a plugin calls the track it is on, since a host doesn't tell it the track's name. */
    inline constexpr const char* pluginTrackName = "This track";

    /** The words of a name in lower case, split at spaces, punctuation and between letters and digits. */
    inline juce::StringArray getWords (const juce::String& name)
    {
        juce::StringArray words;
        juce::String current;
        int lastKind = 0;   // 1 letter, 2 digit

        for (auto c : name.toLowerCase())
        {
            const int kind = juce::CharacterFunctions::isLetter (c) ? 1 : juce::CharacterFunctions::isDigit (c) ? 2 : 0;

            if (kind == 0 || (lastKind != 0 && kind != lastKind))
            {
                if (current.isNotEmpty())
                    words.add (current);

                current.clear();
            }

            if (kind != 0)
                current += c;

            lastKind = kind;
        }

        if (current.isNotEmpty())
            words.add (current);

        return words;
    }

    /** Matches a word, including longer source names such as vocals and guitars. */
    inline bool nameHasWord (const juce::StringArray& words, std::initializer_list<const char*> list)
    {
        for (auto& w : words)
            for (auto* candidate : list)
            {
                const juce::String word (candidate);

                // Whole words, and for longer words their beginnings too ("kicks", "vocals2").
                if (w == word || (word.length() >= 4 && w.startsWith (word)))
                    return true;
            }

        return false;
    }

}
