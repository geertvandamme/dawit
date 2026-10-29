#include "Pronunciation.h"

#include <DawItDictionary.h>

#include <unordered_map>

namespace daw::gen::pronunciation
{
namespace
{
    using Table = std::unordered_map<std::string, std::vector<juce::String>>;

    int entryCount = 0;   // every line of the dictionary, the other ways to say a word included

    /** The embedded cmudict.dict: "word P1 P2 ..." per line, "word(2)" for another way to say it, # comments. */
    const Table& getTable()
    {
        static const Table table = []
        {
            Table t;
            t.reserve (140000);
            const char* p = dawdict::cmudict_dict;
            const char* end = p + dawdict::cmudict_dictSize;

            while (p < end)
            {
                const char* lineEnd = (const char*) std::memchr (p, '\n', (size_t) (end - p));
                if (lineEnd == nullptr) lineEnd = end;

                std::string line (p, lineEnd);
                p = lineEnd + 1;

                if (const auto hash = line.find (" #"); hash != std::string::npos)
                    line.erase (hash);

                const auto space = line.find (' ');

                if (space == std::string::npos || space == 0)
                    continue;

                const auto word = line.substr (0, space);
                ++entryCount;

                if (word.find ('(') != std::string::npos)
                    continue;   // the first way to say a word is the usual one

                std::vector<juce::String> phonemes;
                size_t pos = space;

                while (pos < line.size())
                {
                    while (pos < line.size() && line[pos] == ' ') ++pos;
                    const auto next = line.find (' ', pos);
                    auto token = line.substr (pos, next == std::string::npos ? std::string::npos : next - pos);
                    pos = next == std::string::npos ? line.size() : next;

                    // The stress digit stays ("ay1"); lookupCmu strips it.
                    if (! token.empty())
                        phonemes.push_back (juce::String (token).toLowerCase());
                }

                t.emplace (word, std::move (phonemes));
            }

            return t;
        }();

        return table;
    }

    bool startsWithAt (const juce::String& s, int i, const char* pattern)
    {
        return s.substring (i).startsWith (pattern);
    }

    bool isVowelLetter (juce::juce_wchar c) { return juce::String ("aeiouy").containsChar (c); }
}

juce::String cleanWord (const juce::String& word)
{
    auto w = word.toLowerCase().replaceCharacter (juce::String::fromUTF8 ("\xe2\x80\x99")[0], '\'');
    int start = 0, end = w.length();

    auto isLetter = [&] (int i) { return juce::CharacterFunctions::isLetter (w[i]) || juce::CharacterFunctions::isDigit (w[i]); };

    while (start < end && ! isLetter (start)) ++start;
    while (end > start && ! isLetter (end - 1)) --end;

    return w.substring (start, end);
}

std::vector<juce::String> getAllCmuPhonemes()
{
    auto all = sing::getCmuVowels();
    all.insert (all.end(), sing::getCmuConsonants().begin(), sing::getCmuConsonants().end());
    return all;
}

bool isCmuVowel (const juce::String& p)
{
    for (auto& v : sing::getCmuVowels())
        if (v == p)
            return true;

    return false;
}

int getCmuWordCount() { return (int) getTable().size(); }

int getCmuEntryCount()
{
    getTable();
    return entryCount;
}

std::vector<juce::String> lookupCmu (const juce::String& word)
{
    const auto it = getTable().find (cleanWord (word).toStdString());

    if (it == getTable().end())
        return {};

    std::vector<juce::String> out;
    out.reserve (it->second.size());

    for (auto& p : it->second)
        out.push_back (p.trimCharactersAtEnd ("012"));

    return out;
}

namespace
{
    StressedSound makeStressed (const juce::String& token)
    {
        StressedSound s;
        s.symbol = token.trimCharactersAtEnd ("012");

        if (token.length() > s.symbol.length())
            s.stress = token.getLastCharacter() - '0';

        return s;
    }
}

std::vector<StressedSound> lookupCmuStressed (const juce::String& word)
{
    const auto it = getTable().find (cleanWord (word).toStdString());
    std::vector<StressedSound> out;

    if (it != getTable().end())
        for (auto& p : it->second)
            out.push_back (makeStressed (p));

    return out;
}

void forEachCmuWord (const std::function<void (const juce::String&, const std::vector<StressedSound>&)>& callback)
{
    std::vector<StressedSound> sounds;

    for (auto& [word, phones] : getTable())
    {
        sounds.clear();

        for (auto& p : phones)
            sounds.push_back (makeStressed (p));

        callback (juce::String (word), sounds);
    }
}

std::vector<juce::String> guessFromSpelling (const juce::String& raw)
{
    const auto w = cleanWord (raw).retainCharacters ("abcdefghijklmnopqrstuvwxyz'");
    std::vector<juce::String> out;
    const int n = w.length();

    // A silent e at the end ("make", "ride") makes the vowel before the consonant before it say its name.
    const bool magicE = n >= 3 && w.getLastCharacter() == 'e' && ! isVowelLetter (w[n - 2]) && isVowelLetter (w[n - 3]);
    int numVowelGroups = 0;
    for (int i = 0; i < n; ++i)
        if (isVowelLetter (w[i]) && (i == 0 || ! isVowelLetter (w[i - 1])))
            ++numVowelGroups;

    for (int i = 0; i < n;)
    {
        auto take = [&] (int length, std::initializer_list<const char*> sounds)
        {
            for (auto s : sounds)
                out.emplace_back (s);

            i += length;
        };

        const auto c = w[i];
        const bool last = i == n - 1;

        // Two and three letter patterns first.
        if (startsWithAt (w, i, "tch")) { take (3, { "ch" }); continue; }
        if (startsWithAt (w, i, "sh"))  { take (2, { "sh" }); continue; }
        if (startsWithAt (w, i, "ch"))  { take (2, { "ch" }); continue; }
        if (startsWithAt (w, i, "th"))  { take (2, { "th" }); continue; }
        if (startsWithAt (w, i, "ph"))  { take (2, { "f" }); continue; }
        if (startsWithAt (w, i, "wh"))  { take (2, { "w" }); continue; }
        if (startsWithAt (w, i, "ck"))  { take (2, { "k" }); continue; }
        if (startsWithAt (w, i, "ng"))  { take (2, { "ng" }); continue; }
        if (startsWithAt (w, i, "qu"))  { take (2, { "k", "w" }); continue; }
        if (startsWithAt (w, i, "gh"))  { take (2, {}); continue; }
        if (startsWithAt (w, i, "kn") && i == 0) { take (2, { "n" }); continue; }
        if (startsWithAt (w, i, "ee") || startsWithAt (w, i, "ea") || startsWithAt (w, i, "ie")) { take (2, { "iy" }); continue; }
        if (startsWithAt (w, i, "oo"))  { take (2, { "uw" }); continue; }
        if (startsWithAt (w, i, "ai") || startsWithAt (w, i, "ay")) { take (2, { "ey" }); continue; }
        if (startsWithAt (w, i, "oa") || startsWithAt (w, i, "ow")) { take (2, { "ow" }); continue; }
        if (startsWithAt (w, i, "ou"))  { take (2, { "aw" }); continue; }
        if (startsWithAt (w, i, "oi") || startsWithAt (w, i, "oy")) { take (2, { "oy" }); continue; }
        if (startsWithAt (w, i, "au") || startsWithAt (w, i, "aw")) { take (2, { "ao" }); continue; }
        if (startsWithAt (w, i, "er") || startsWithAt (w, i, "ir") || startsWithAt (w, i, "ur")) { take (2, { "er" }); continue; }

        const bool beforeFront = i + 1 < n && juce::String ("eiy").containsChar (w[i + 1]);

        switch (c)
        {
            case 'a': out.emplace_back (magicE && i == n - 3 ? "ey" : "ae"); break;
            case 'e':
                if (last && i > 0 && numVowelGroups > 1) break;   // the silent e
                out.emplace_back (magicE && i == n - 3 ? "iy" : "eh");
                break;
            case 'i': out.emplace_back (magicE && i == n - 3 ? "ay" : "ih"); break;
            case 'o': out.emplace_back (magicE && i == n - 3 ? "ow" : "aa"); break;
            case 'u': out.emplace_back (magicE && i == n - 3 ? "uw" : "ah"); break;
            case 'y': out.emplace_back (i == 0 ? "y" : last ? "iy" : "ih"); break;
            case 'b': out.emplace_back ("b"); break;
            case 'c': out.emplace_back (beforeFront ? "s" : "k"); break;
            case 'd': out.emplace_back ("d"); break;
            case 'f': out.emplace_back ("f"); break;
            case 'g': out.emplace_back (beforeFront && i + 1 < n && w[i + 1] != 'y' ? "jh" : "g"); break;
            case 'h': out.emplace_back ("hh"); break;
            case 'j': out.emplace_back ("jh"); break;
            case 'k': out.emplace_back ("k"); break;
            case 'l': out.emplace_back ("l"); break;
            case 'm': out.emplace_back ("m"); break;
            case 'n': out.emplace_back ("n"); break;
            case 'p': out.emplace_back ("p"); break;
            case 'q': out.emplace_back ("k"); break;
            case 'r': out.emplace_back ("r"); break;
            case 's': out.emplace_back ("s"); break;
            case 't': out.emplace_back ("t"); break;
            case 'v': out.emplace_back ("v"); break;
            case 'w': out.emplace_back ("w"); break;
            case 'x': out.emplace_back ("k"); out.emplace_back ("s"); break;
            case 'z': out.emplace_back ("z"); break;
            default: break;   // an apostrophe
        }

        ++i;
    }

    return out;
}

Pronounced pronounce (const juce::String& word, const sing::Voice* voice)
{
    Pronounced result;
    const auto clean = cleanWord (word);

    if (clean.isEmpty())
        return result;

    // 1. The voice's own dictionary: its sounds are already in the voice's spelling.
    if (voice != nullptr)
    {
        if (const auto it = voice->dictionary.find (clean); it != voice->dictionary.end())
        {
            result.source = Source::voiceDictionary;

            for (auto& s : it->second)
                result.sounds.push_back ({ s, voice->isVowel (s) });

            return result;
        }
    }

    // 2. CMUdict; 3. a guess.
    auto cmu = lookupCmu (clean);
    result.source = cmu.empty() ? Source::guessed : Source::cmudict;

    if (cmu.empty())
        cmu = guessFromSpelling (clean);

    for (auto& p : cmu)
    {
        const bool vowel = isCmuVowel (p);

        if (voice == nullptr)
            result.sounds.push_back ({ p, vowel });
        else if (const auto mapped = voice->mapPhoneme (p); mapped.isNotEmpty())
            result.sounds.push_back ({ mapped, vowel });   // a sound the voice lacks is left out
    }

    return result;
}
} // namespace daw::gen::pronunciation
