#include "Rhyme.h"
#include "Pronunciation.h"
#include "engine/Lyrics.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace daw::gen::rhyme
{
namespace
{
    namespace pron = pronunciation;

    /** The last word of the text, without hyphens or punctuation, lower case. */
    juce::String lastWord (const juce::String& text)
    {
        const auto words = juce::StringArray::fromTokens (text.trim(), " \t\n", "");
        return words.isEmpty() ? juce::String() : pron::cleanWord (words[words.size() - 1].removeCharacters ("-"));
    }

    int countVowels (const std::vector<pron::StressedSound>& sounds)
    {
        return (int) std::count_if (sounds.begin(), sounds.end(), [] (auto& s) { return s.isVowel(); });
    }

    Part makePart (const std::vector<pron::StressedSound>& sounds)
    {
        Part part;
        int at = -1;

        for (int i = 0; i < (int) sounds.size(); ++i)
            if (sounds[(size_t) i].isVowel() && sounds[(size_t) i].stress >= 1)
                at = i;

        if (at < 0)
            for (int i = 0; i < (int) sounds.size(); ++i)
                if (sounds[(size_t) i].isVowel())
                    at = i;

        if (at < 0)
            return part;

        part.found = true;
        part.vowel = sounds[(size_t) at].symbol;

        int start = at;
        while (start > 0 && ! sounds[(size_t) start - 1].isVowel())
            --start;

        for (int i = start; i < at; ++i)
            part.onset.push_back (sounds[(size_t) i].symbol);

        for (int i = at + 1; i < (int) sounds.size(); ++i)
        {
            part.tail.push_back (sounds[(size_t) i].symbol);
            part.syllables += sounds[(size_t) i].isVowel() ? 1 : 0;
        }

        part.syllables += 1;
        return part;
    }

    /** Sounds from the spelling, with the last vowel taken as the stressed one. */
    std::vector<pron::StressedSound> guessSounds (const juce::String& word)
    {
        std::vector<pron::StressedSound> out;
        int lastVowel = -1;

        for (auto& p : pron::guessFromSpelling (word))
        {
            pron::StressedSound s;
            s.symbol = p;
            s.stress = pron::isCmuVowel (p) ? 0 : -1;

            if (s.isVowel())
                lastVowel = (int) out.size();

            out.push_back (s);
        }

        if (lastVowel >= 0)
            out[(size_t) lastVowel].stress = 1;

        return out;
    }

    bool isSubsequence (const std::vector<juce::String>& shorter, const std::vector<juce::String>& longer)
    {
        size_t at = 0;

        for (auto& s : longer)
            if (at < shorter.size() && shorter[at] == s)
                ++at;

        return at == shorter.size();
    }

    bool hasLetterOrDigit (const juce::String& s)
    {
        for (auto p = s.getCharPointer(); ! p.isEmpty();)
        {
            const auto c = p.getAndAdvance();

            if (juce::CharacterFunctions::isLetterOrDigit (c))
                return true;
        }

        return false;
    }

    bool isFunctionWord (const juce::String& w)
    {
        static const std::unordered_set<std::string> words
        {
            "a", "an", "the", "and", "or", "but", "so", "i", "me", "my", "you", "your", "he", "him", "his", "she", "her", "it", "its",
            "we", "us", "our", "they", "them", "their", "to", "of", "in", "on", "at", "for", "with", "from", "as", "is", "am", "are",
            "was", "be", "if", "than", "i'm", "i'll", "i've", "it's", "you're", "we're", "they're"
        };

        return words.count (w.toStdString()) > 0;
    }

    struct WordInfo
    {
        int count = 0;
        std::vector<bool> strong;
        bool known = false;
    };

    /** The syllables of one token of a line ("ri-ver", "lonely," "-"). */
    WordInfo analyseToken (const juce::String& token)
    {
        WordInfo info;

        juce::StringArray parts;
        for (auto& p : juce::StringArray::fromTokens (token, "-", ""))
            if (hasLetterOrDigit (p))
                parts.add (p);

        if (parts.isEmpty())
            return info;

        const auto joined = pron::cleanWord (parts.joinIntoString ({}));
        const auto sounds = pron::lookupCmuStressed (joined);
        const int vowels = countVowels (sounds);

        int n;

        if (parts.size() > 1)
            n = parts.size();
        else if (vowels > 0)
            n = vowels;
        else
            n = juce::jmax (1, lyrics::splitEnglishWord (joined).size());

        info.count = n;
        info.known = vowels > 0;
        info.strong.assign ((size_t) n, false);

        if (vowels == n && vowels > 1)
        {
            int i = 0;

            for (auto& s : sounds)
                if (s.isVowel())
                    info.strong[(size_t) i++] = s.stress == 1;
        }
        else if (n > 1)
            info.strong[0] = true;   // no better guess than the first one
        else
            info.strong[0] = ! isFunctionWord (joined);

        return info;
    }

    std::vector<WordInfo> analyseLine (const juce::String& line)
    {
        std::vector<WordInfo> words;

        for (auto& token : juce::StringArray::fromTokens (line, " \t\n", ""))
            if (auto info = analyseToken (token); info.count > 0)
                words.push_back (std::move (info));

        return words;
    }

    juce::String join (const std::vector<juce::String>& sounds, size_t from = 0, size_t to = std::string::npos)
    {
        juce::String s;

        for (size_t i = from; i < sounds.size() && i < to; ++i)
            s << (s.isEmpty() ? "" : " ") << sounds[i];

        return s;
    }

    char familyLetter (Family f)
    {
        switch (f)
        {
            case Family::plosive:   return 'p';
            case Family::fricative: return 'f';
            case Family::nasal:     return 'n';
            case Family::none:      break;
        }

        return '-';
    }

    //==============================================================================
    /** Every dictionary word by its rhyme part (perfect) and by its rhyme part with the last consonant as a family. */
    struct Index
    {
        struct Entry
        {
            std::string word, onset;
            std::string tail;
            int syllables = 0;
        };

        std::vector<Entry> entries;
        std::unordered_map<std::string, std::vector<uint32_t>> perfect, family;
    };

    const Index& getIndex()
    {
        static const Index index = []
        {
            Index ix;
            ix.entries.reserve (130000);

            pron::forEachCmuWord ([&] (const juce::String& word, const std::vector<pron::StressedSound>& sounds)
            {
                if (word.containsAnyOf ("'.-_0123456789"))
                    return;

                const auto part = makePart (sounds);

                if (! part.found)
                    return;

                Index::Entry e;
                e.word = word.toStdString();
                e.onset = join (part.onset).toStdString();
                e.tail = join (part.tail).toStdString();
                e.syllables = countVowels (sounds);

                const auto id = (uint32_t) ix.entries.size();
                const auto key = part.vowel.toStdString() + "|" + e.tail;
                ix.entries.push_back (std::move (e));
                ix.perfect[key].push_back (id);

                if (! part.tail.empty())
                    if (const auto f = getFamily (part.tail.back()); f != Family::none)
                        ix.family[part.vowel.toStdString() + "|" + join (part.tail, 0, part.tail.size() - 1).toStdString() + "|" + familyLetter (f)].push_back (id);
            });

            return ix;
        }();

        return index;
    }
}

//==============================================================================
juce::String getKindName (Kind k)
{
    switch (k)
    {
        case Kind::perfect:    return "perfect";
        case Kind::identical:  return "identical";
        case Kind::family:     return "family";
        case Kind::additive:   return "additive";
        case Kind::assonance:  return "assonance";
        case Kind::consonance: return "consonance";
        case Kind::none:       return "none";
        case Kind::unknown:    return "unknown";
    }

    return {};
}

Family getFamily (const juce::String& c)
{
    for (auto* p : { "p", "b", "t", "d", "k", "g" })
        if (c == p)
            return Family::plosive;

    for (auto* f : { "f", "v", "th", "dh", "s", "z", "sh", "zh", "ch", "jh" })
        if (c == f)
            return Family::fricative;

    for (auto* n : { "m", "n", "ng" })
        if (c == n)
            return Family::nasal;

    return Family::none;
}

juce::String Part::toString() const
{
    return vowel + (tail.empty() ? juce::String() : " " + join (tail));
}

Part getRhymePart (const juce::String& word, bool guess)
{
    const auto w = lastWord (word);

    if (w.isEmpty())
        return {};

    if (auto sounds = pron::lookupCmuStressed (w); ! sounds.empty())
        return makePart (sounds);

    if (! guess)
        return {};

    auto part = makePart (guessSounds (w));
    part.guessed = part.found;
    return part;
}

bool isKnown (const juce::String& word)
{
    const auto w = lastWord (word);
    return w.isNotEmpty() && ! pron::lookupCmuStressed (w).empty();
}

Kind classify (const juce::String& a, const juce::String& b)
{
    const auto pa = getRhymePart (a), pb = getRhymePart (b);

    if (! pa.found || ! pb.found)
        return Kind::unknown;

    if (pa.vowel == pb.vowel)
    {
        if (pa.tail == pb.tail)
            return pa.onset == pb.onset ? Kind::identical : Kind::perfect;

        if (pa.tail.size() == pb.tail.size() && ! pa.tail.empty()
            && std::equal (pa.tail.begin(), pa.tail.end() - 1, pb.tail.begin())
            && getFamily (pa.tail.back()) != Family::none && getFamily (pa.tail.back()) == getFamily (pb.tail.back()))
            return Kind::family;

        if (isSubsequence (pa.tail, pb.tail) || isSubsequence (pb.tail, pa.tail))
            return Kind::additive;

        return Kind::assonance;
    }

    if (pa.tail == pb.tail && ! pa.tail.empty())
        return Kind::consonance;

    return Kind::none;
}

int countSyllables (const juce::String& line)
{
    int n = 0;

    for (auto& w : analyseLine (line))
        n += w.count;

    return n;
}

std::vector<bool> getStressPattern (const juce::String& line)
{
    std::vector<bool> pattern;

    for (auto& w : analyseLine (line))
        pattern.insert (pattern.end(), w.strong.begin(), w.strong.end());

    return pattern;
}

juce::String hyphenate (const juce::String& line)
{
    juce::String out, token;

    auto flush = [&]
    {
        if (token.isEmpty())
            return;

        if (! token.contains ("-"))
        {
            const auto split = lyrics::splitEnglishWord (token);

            if (split.size() > 1 && countVowels (pron::lookupCmuStressed (token)) == split.size())
            {
                out << split.joinIntoString ("-");
                token = {};
                return;
            }
        }

        out << token;
        token = {};
    };

    for (auto p = line.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();

        if (juce::CharacterFunctions::isWhitespace (c))
        {
            flush();
            out << juce::String::charToString (c);
        }
        else
            token << juce::String::charToString (c);
    }

    flush();
    return out;
}

RhymeList findRhymes (const juce::String& word, int maxPerKind)
{
    RhymeList list;
    const auto w = lastWord (word);
    auto part = getRhymePart (w);

    if (! part.found)
    {
        part = getRhymePart (w, true);
        list.guessed = true;
    }

    if (! part.found)
        return list;

    const auto& ix = getIndex();
    const auto self = w.toStdString();
    const auto onset = join (part.onset).toStdString();
    const auto tail = join (part.tail).toStdString();
    const auto vowel = part.vowel.toStdString();

    auto order = [&] (const Index::Entry* a, const Index::Entry* b)
    {
        if (a->syllables != b->syllables) return a->syllables < b->syllables;
        if (a->word.size() != b->word.size()) return a->word.size() < b->word.size();
        return a->word < b->word;
    };

    auto collect = [&] (const std::vector<uint32_t>* ids, Kind kind, bool identical)
    {
        std::vector<const Index::Entry*> picked;

        if (ids != nullptr)
            for (auto id : *ids)
            {
                auto& e = ix.entries[id];

                if (e.word == self)
                    continue;

                if (kind == Kind::family && e.tail == tail)
                    continue;   // a perfect rhyme, listed there

                if (kind != Kind::family && (e.onset == onset) != identical)
                    continue;

                picked.push_back (&e);
            }

        std::sort (picked.begin(), picked.end(), order);

        std::map<int, int> perSyllables;   // a cap for each number of syllables, so longer words aren't crowded out

        for (auto* e : picked)
            if (perSyllables[e->syllables]++ < maxPerKind)
                list.items.push_back ({ juce::String (e->word), kind, e->syllables });
    };

    const auto perfect = ix.perfect.find (vowel + "|" + tail);
    const auto* perfectIds = perfect == ix.perfect.end() ? nullptr : &perfect->second;
    collect (perfectIds, Kind::perfect, false);

    if (! part.tail.empty())
        if (const auto f = getFamily (part.tail.back()); f != Family::none)
            if (const auto fam = ix.family.find (vowel + "|" + join (part.tail, 0, part.tail.size() - 1).toStdString() + "|" + familyLetter (f)); fam != ix.family.end())
                collect (&fam->second, Kind::family, false);

    collect (perfectIds, Kind::identical, true);
    return list;
}
} // namespace daw::gen::rhyme
