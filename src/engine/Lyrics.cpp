#include "Lyrics.h"

#include <cstring>
#include <map>

namespace daw::lyrics
{
namespace
{
    using Verse = std::vector<LyricLine>;

    /** Only the letters (and digits), lower case: "Night," matches "night". */
    juce::String lettersOf (const juce::String& text, bool withDigits)
    {
        juce::String letters;
        const auto lower = text.toLowerCase();

        for (auto p = lower.getCharPointer(); ! p.isEmpty();)
        {
            const auto c = p.getAndAdvance();

            if (juce::CharacterFunctions::isLetter (c) || (withDigits && juce::CharacterFunctions::isDigit (c)))
                letters += juce::String::charToString (c);
        }

        return letters;
    }

    /** The sections that usually have words; intros, solos and drops usually don't. */
    bool isSung (SectionType type)
    {
        switch (type)
        {
            case SectionType::verse:
            case SectionType::preChorus:
            case SectionType::chorus:
            case SectionType::postChorus:
            case SectionType::bridge:
            case SectionType::phrase:     return true;
            case SectionType::intro:
            case SectionType::solo:
            case SectionType::interlude:
            case SectionType::breakdown:
            case SectionType::buildUp:
            case SectionType::drop:
            case SectionType::outro:      return false;
        }

        return false;
    }

    /** The sections that come back with the same words. */
    bool repeatsWords (SectionType type)
    {
        return type == SectionType::chorus || type == SectionType::preChorus || type == SectionType::postChorus;
    }

    /** "[Chorus 2]", "[Pre-chorus]", "[Hook]": the section type a verse's heading names. */
    std::optional<SectionType> getHeadingType (const juce::String& heading)
    {
        const auto name = lettersOf (heading, false);

        if (name.isEmpty())
            return std::nullopt;

        if (name == "hook" || name == "refrain")
            return SectionType::chorus;

        for (auto type : sectionTypes::all)
            if (name == lettersOf (sectionTypes::getName (type), false) || name == lettersOf (sectionTypes::getId (type), false))
                return type;

        return std::nullopt;
    }

    std::vector<Verse> getVerses (const std::vector<LyricLine>& lines)
    {
        std::vector<Verse> verses;

        for (auto& line : lines)
        {
            if (verses.empty() || line.startsVerse)
                verses.emplace_back();

            verses.back().push_back (line);
        }

        return verses;
    }

    double roundUpToBar (double beat)
    {
        return std::ceil (beat / 4.0 - 1.0e-9) * 4.0;
    }

    /** A line placed in a slot of `slot` beats: it fills the slot but for a breath before the next line,
        and is sung no slower than a syllable a beat. */
    void setTiming (LyricLine& line, double start, double slot)
    {
        const double breath = slot >= 8.0 ? 2.0 : slot * 0.25;
        const double shortest = juce::jmin (2.0, slot);
        const double wanted = std::ceil ((double) line.syllables.size() * beatsPerSyllable);

        line.startBeat = juce::jmax (0.0, start);
        line.endBeat = line.startBeat + juce::jlimit (shortest, juce::jmax (shortest, slot - breath), wanted);

        for (auto& s : line.syllables)
            s.pinned = false;
    }

    /** Spreads lines over `length` beats from `start`, each starting on a beat (on a bar when they divide the bars evenly). */
    void fill (std::vector<LyricLine*>& lines, double start, double length)
    {
        const double slot = length / (double) lines.size();

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const double from = start + std::round ((double) i * slot);
            const double to = i + 1 < lines.size() ? start + std::round ((double) (i + 1) * slot) : start + length;
            setTiming (*lines[i], from, to - from);
        }
    }

    void fill (Verse& verse, double start, double length)
    {
        std::vector<LyricLine*> lines;

        for (auto& line : verse)
            lines.push_back (&line);

        fill (lines, start, length);
    }

    /** Lines one after another from `start`, two bars each, with a bar between verses. */
    void follow (std::vector<Verse>& verses, double start, std::vector<LyricLine>& out)
    {
        double beat = start;

        for (auto& verse : verses)
        {
            for (auto& line : verse)
            {
                setTiming (line, beat, beatsPerLine);
                out.push_back (line);
                beat += beatsPerLine;
            }

            beat += 4.0;
        }
    }

    /** Puts each verse on a section, adding the placed lines to `out`; returns the verses no section took. */
    std::vector<Verse> placeOnSections (const std::vector<Verse>& verses, const SectionList& sections, std::vector<LyricLine>& out)
    {
        std::vector<int> verseOf (sections.size(), -1);
        std::vector<bool> used (verses.size(), false);
        std::vector<std::optional<SectionType>> types;

        for (auto& verse : verses)
            types.push_back (getHeadingType (verse.front().heading));

        // Verses with a heading go on the sections of their type, in order; a chorus typed once goes on every chorus.
        std::map<SectionType, int> lastOfType;

        for (size_t s = 0; s < sections.size(); ++s)
        {
            const auto type = sections[s].type;
            int found = -1;

            for (size_t v = 0; v < verses.size() && found < 0; ++v)
                if (! used[v] && types[v] == type)
                    found = (int) v;

            if (found >= 0)
            {
                used[(size_t) found] = true;
                verseOf[s] = found;
                lastOfType[type] = found;
            }
            else if (repeatsWords (type) && lastOfType.count (type) > 0)
            {
                verseOf[s] = lastOfType[type];
            }
        }

        // The others, in order, on the sections still without words that are usually sung (or on any, when none is).
        const bool anySung = std::any_of (sections.begin(), sections.end(), [] (const Section& s) { return isSung (s.type); });
        size_t next = 0;

        for (size_t s = 0; s < sections.size(); ++s)
        {
            if (verseOf[s] >= 0 || (anySung && ! isSung (sections[s].type)))
                continue;

            while (next < verses.size() && used[next])
                ++next;

            if (next >= verses.size())
                break;

            used[next] = true;
            verseOf[s] = (int) next;
        }

        for (size_t s = 0; s < sections.size(); ++s)
        {
            if (verseOf[s] < 0)
                continue;

            auto verse = verses[(size_t) verseOf[s]];
            fill (verse, sections[s].startBar * 4.0, sections[s].bars * 4.0);
            out.insert (out.end(), verse.begin(), verse.end());
        }

        std::vector<Verse> left;

        for (size_t v = 0; v < verses.size(); ++v)
            if (! used[v])
                left.push_back (verses[v]);

        return left;
    }

    void sortLines (Lyrics& l)
    {
        std::stable_sort (l.lines.begin(), l.lines.end(), [] (const LyricLine& a, const LyricLine& b) { return a.startBeat < b.startBeat; });
    }

    /** One line in order: see tidy(). */
    void tidyLine (LyricLine& line)
    {
        auto& s = line.syllables;
        line.startBeat = juce::jmax (0.0, line.startBeat);
        s.front().pinned = false;   // the first is where the line starts
        s.front().beat = line.startBeat;

        double previous = line.startBeat;
        size_t previousIndex = 0;

        for (size_t i = 1; i < s.size(); ++i)
        {
            if (s[i].pinned)
            {
                s[i].beat = juce::jmax (s[i].beat, previous + minGap * (double) (i - previousIndex));
                previous = s[i].beat;
                previousIndex = i;
            }
        }

        line.endBeat = juce::jmax (line.endBeat, previous + minGap * (double) (s.size() - previousIndex));
        spread (line);
    }

    void shiftLine (LyricLine& line, double delta)
    {
        line.startBeat += delta;
        line.endBeat += delta;

        for (auto& s : line.syllables)
            s.beat += delta;
    }
}

//==============================================================================
Lyrics parse (const juce::String& text)
{
    Lyrics lyrics;
    bool newVerse = true;
    juce::String heading;

    for (auto& raw : juce::StringArray::fromLines (text))
    {
        const auto trimmed = raw.trim();

        if (trimmed.isEmpty())
        {
            newVerse = true;
            continue;
        }

        if (trimmed.startsWithChar ('[') && trimmed.endsWithChar (']'))
        {
            heading = trimmed.substring (1, trimmed.length() - 1).trim();
            newVerse = true;
            continue;
        }

        LyricLine line;
        line.startsVerse = newVerse || lyrics.lines.empty();
        line.heading = line.startsVerse ? heading : juce::String();

        for (auto& word : juce::StringArray::fromTokens (trimmed, " \t", ""))
        {
            auto parts = juce::StringArray::fromTokens (word, "-", "");
            parts.removeEmptyStrings();

            for (int i = 0; i < parts.size(); ++i)
                line.syllables.push_back ({ parts[i], i == parts.size() - 1 });
        }

        if (line.syllables.empty())
            continue;

        lyrics.lines.push_back (std::move (line));
        newVerse = false;
        heading = {};
    }

    return lyrics;
}

juce::String toText (const Lyrics& lyrics)
{
    juce::String text;

    for (size_t i = 0; i < lyrics.lines.size(); ++i)
    {
        const auto& line = lyrics.lines[i];

        if (i > 0 && line.startsVerse)
            text << "\n";

        if (line.startsVerse && line.heading.isNotEmpty())
            text << "[" << line.heading << "]\n";

        for (size_t j = 0; j < line.syllables.size(); ++j)
        {
            text << line.syllables[j].text;

            if (j + 1 < line.syllables.size())
                text << (line.syllables[j].endsWord ? " " : "-");
        }

        text << "\n";
    }

    return text.trimEnd();
}

Lyrics place (const Lyrics& parsed, const SectionList& sections, int firstBar)
{
    Lyrics placed;
    auto left = placeOnSections (getVerses (parsed.lines), sections, placed.lines);

    double start = juce::jmax (0, firstBar) * 4.0;

    for (auto& line : placed.lines)
        start = juce::jmax (start, roundUpToBar (line.endBeat));

    if (! placed.lines.empty())
        for (auto& s : sections)
            start = juce::jmax (start, s.getEndBar() * 4.0);

    follow (left, start, placed.lines);
    sortLines (placed);
    tidy (placed);
    return placed;
}

Lyrics setText (const Lyrics& current, const juce::String& text, const SectionList& sections)
{
    auto edited = parse (text);

    // The syllables in order, to find the ones that stayed the same.
    struct Ref
    {
        size_t line, syllable;
        juce::String letters;
    };

    auto flatten = [] (const Lyrics& l)
    {
        std::vector<Ref> refs;

        for (size_t i = 0; i < l.lines.size(); ++i)
            for (size_t j = 0; j < l.lines[i].syllables.size(); ++j)
                refs.push_back ({ i, j, lettersOf (l.lines[i].syllables[j].text, true) });

        return refs;
    };

    const auto before = flatten (current), after = flatten (edited);

    if (before.empty() || after.empty() || (double) before.size() * (double) after.size() > 2.0e7)
        return place (edited, sections);

    // The longest run of syllables both have in the same order: those are the ones kept.
    const size_t n = before.size(), m = after.size();
    std::vector<int> common ((n + 1) * (m + 1), 0);
    auto at = [&] (size_t i, size_t j) -> int& { return common[i * (m + 1) + j]; };

    for (size_t i = n; i-- > 0;)
        for (size_t j = m; j-- > 0;)
            at (i, j) = before[i].letters == after[j].letters ? at (i + 1, j + 1) + 1 : juce::jmax (at (i + 1, j), at (i, j + 1));

    std::vector<std::optional<size_t>> match (m);   // for each new syllable, the old one it was

    for (size_t i = 0, j = 0; i < n && j < m;)
    {
        if (before[i].letters == after[j].letters)
            match[j++] = i++;
        else if (at (i + 1, j) >= at (i, j + 1))
            ++i;
        else
            ++j;
    }

    if (std::none_of (match.begin(), match.end(), [] (auto& o) { return o.has_value(); }))
        return place (edited, sections);

    auto oldSyllable = [&] (size_t index) -> const LyricSyllable& { return current.lines[before[index].line].syllables[before[index].syllable]; };

    // Lines with syllables kept: from where their first kept one was to where their last kept one stopped.
    std::vector<bool> timed (edited.lines.size(), false);

    for (size_t k = 0, flat = 0; k < edited.lines.size(); ++k)
    {
        auto& line = edited.lines[k];
        std::optional<size_t> last;

        for (size_t j = 0; j < line.syllables.size(); ++j, ++flat)
        {
            if (! match[flat].has_value())
                continue;

            const auto& old = oldSyllable (*match[flat]);
            line.syllables[j].pinned = old.pinned;
            line.syllables[j].beat = old.beat;

            if (! timed[k])
                line.startBeat = old.beat - (double) j * beatsPerSyllable;

            const auto& oldRef = before[*match[flat]];
            line.endBeat = current.lines[oldRef.line].getSyllableEnd (oldRef.syllable);
            timed[k] = true;
            last = j;
        }

        if (last.has_value())
            line.endBeat += (double) (line.syllables.size() - 1 - *last) * beatsPerSyllable;
    }

    // New lines go in the gaps: on a free section there, else spread over the gap or after the last line.
    std::vector<LyricLine> added;

    for (size_t k = 0; k < edited.lines.size();)
    {
        if (timed[k])
        {
            ++k;
            continue;
        }

        size_t end = k;
        while (end < edited.lines.size() && ! timed[end])
            ++end;

        const std::optional<double> from = k > 0 ? std::optional<double> (edited.lines[k - 1].endBeat) : std::nullopt;
        const std::optional<double> to = end < edited.lines.size() ? std::optional<double> (edited.lines[end].startBeat) : std::nullopt;

        SectionList free;

        for (auto& s : sections)
        {
            const double sectionStart = s.startBar * 4.0, sectionEnd = s.getEndBar() * 4.0;
            const bool inGap = sectionStart >= from.value_or (0.0) - 1.0e-6 && sectionEnd <= to.value_or (1.0e12) + 1.0e-6;
            bool hasWords = false;

            for (size_t i = 0; i < edited.lines.size(); ++i)
                hasWords = hasWords || (timed[i] && edited.lines[i].startBeat < sectionEnd && edited.lines[i].endBeat > sectionStart);

            if (inGap && ! hasWords)
                free.push_back (s);
        }

        const std::vector<LyricLine> run (edited.lines.begin() + (long) k, edited.lines.begin() + (long) end);
        auto left = placeOnSections (getVerses (run), free, added);

        if (! left.empty())
        {
            if (to.has_value())
            {
                std::vector<LyricLine*> lines;
                std::vector<LyricLine> leftLines;

                for (auto& verse : left)
                    leftLines.insert (leftLines.end(), verse.begin(), verse.end());

                for (auto& line : leftLines)
                    lines.push_back (&line);

                const double gapStart = from.value_or (juce::jmax (0.0, *to - (double) lines.size() * beatsPerLine));
                const double length = juce::jmax (*to - gapStart, (double) lines.size() * 2.0);
                fill (lines, gapStart, length);
                added.insert (added.end(), leftLines.begin(), leftLines.end());
            }
            else
            {
                follow (left, roundUpToBar (*from), added);
            }
        }

        k = end;
    }

    Lyrics result;

    for (size_t k = 0; k < edited.lines.size(); ++k)
        if (timed[k])
            result.lines.push_back (edited.lines[k]);

    result.lines.insert (result.lines.end(), added.begin(), added.end());
    sortLines (result);
    tidy (result);
    return result;
}

void tidy (Lyrics& lyrics)
{
    lyrics.lines.erase (std::remove_if (lyrics.lines.begin(), lyrics.lines.end(), [] (const LyricLine& l) { return l.syllables.empty(); }),
                        lyrics.lines.end());

    for (auto& line : lyrics.lines)
        tidyLine (line);
}

void spread (LyricLine& line)
{
    auto& s = line.syllables;

    if (s.empty())
        return;

    s.front().beat = line.startBeat;
    size_t anchor = 0;
    double anchorBeat = line.startBeat;

    for (size_t i = 1; i <= s.size(); ++i)
    {
        if (i < s.size() && ! s[i].pinned)
            continue;

        const double to = i < s.size() ? s[i].beat : line.endBeat;

        for (size_t j = anchor + 1; j < i; ++j)
            s[j].beat = anchorBeat + (to - anchorBeat) * (double) (j - anchor) / (double) (i - anchor);

        anchor = i;
        anchorBeat = to;
    }
}

//==============================================================================
bool isValid (const Lyrics& lyrics, Position p)
{
    return p.line >= 0 && p.line < (int) lyrics.lines.size() && p.syllable >= 0
           && p.syllable < (int) lyrics.lines[(size_t) p.line].syllables.size();
}

std::optional<Position> getNext (const Lyrics& lyrics, Position p)
{
    if (! isValid (lyrics, p))
        return std::nullopt;

    if (isValid (lyrics, { p.line, p.syllable + 1 }))
        return Position { p.line, p.syllable + 1 };

    if (isValid (lyrics, { p.line + 1, 0 }))
        return Position { p.line + 1, 0 };

    return std::nullopt;
}

std::optional<Position> findAt (const Lyrics& lyrics, double beat)
{
    for (size_t i = 0; i < lyrics.lines.size(); ++i)
    {
        const auto& line = lyrics.lines[i];

        if (beat < line.startBeat || beat >= line.endBeat)
            continue;

        for (size_t j = line.syllables.size(); j-- > 0;)
            if (beat >= line.syllables[j].beat)
                return Position { (int) i, (int) j };
    }

    return std::nullopt;
}

std::optional<Position> findFrom (const Lyrics& lyrics, double beat)
{
    for (size_t i = 0; i < lyrics.lines.size(); ++i)
        for (size_t j = 0; j < lyrics.lines[i].syllables.size(); ++j)
            if (lyrics.lines[i].syllables[j].beat >= beat - 1.0e-6)
                return Position { (int) i, (int) j };

    return std::nullopt;
}

void moveSyllable (Lyrics& lyrics, Position p, double beat)
{
    if (! isValid (lyrics, p))
        return;

    auto& line = lyrics.lines[(size_t) p.line];
    auto& s = line.syllables;
    const auto index = (size_t) p.syllable;

    // The placed syllables around it: it stays between them.
    size_t nextPinned = s.size();

    for (size_t i = index + 1; i < s.size() && nextPinned == s.size(); ++i)
        if (s[i].pinned)
            nextPinned = i;

    const double hi = nextPinned < s.size() ? s[nextPinned].beat - minGap * (double) (nextPinned - index) : 1.0e12;

    if (index == 0)
    {
        const double lineHi = nextPinned < s.size() ? hi : line.endBeat - minGap * (double) s.size();
        line.startBeat = juce::jlimit (0.0, juce::jmax (0.0, lineHi), beat);
    }
    else
    {
        size_t previous = index - 1;

        while (previous > 0 && ! s[previous].pinned)
            --previous;

        const double lo = s[previous].beat + minGap * (double) (index - previous);
        s[index].beat = juce::jlimit (lo, juce::jmax (lo, hi), beat);
        s[index].pinned = true;
        line.endBeat = juce::jmax (line.endBeat, s[index].beat + minGap * (double) (s.size() - index));
    }

    tidyLine (line);
}

void moveLine (Lyrics& lyrics, int index, double startBeat)
{
    if (index < 0 || index >= (int) lyrics.lines.size())
        return;

    auto& line = lyrics.lines[(size_t) index];
    const double length = line.endBeat - line.startBeat;
    double lo = 0.0, hi = 1.0e12;

    if (index > 0)
        lo = juce::jmin (line.startBeat, lyrics.lines[(size_t) index - 1].endBeat);

    if (index + 1 < (int) lyrics.lines.size())
        hi = juce::jmax (line.endBeat, lyrics.lines[(size_t) index + 1].startBeat);

    const double start = juce::jlimit (lo, juce::jmax (lo, hi - length), startBeat);
    shiftLine (line, start - line.startBeat);
    tidyLine (line);
}

void setLineEnd (Lyrics& lyrics, int index, double endBeat)
{
    if (index < 0 || index >= (int) lyrics.lines.size())
        return;

    auto& line = lyrics.lines[(size_t) index];
    auto& s = line.syllables;
    size_t lastPinned = 0;

    for (size_t i = 1; i < s.size(); ++i)
        if (s[i].pinned)
            lastPinned = i;

    const double lo = s[lastPinned].beat + minGap * (double) (s.size() - lastPinned);
    double hi = 1.0e12;

    if (index + 1 < (int) lyrics.lines.size())
        hi = juce::jmax (line.endBeat, lyrics.lines[(size_t) index + 1].startBeat);

    line.endBeat = juce::jlimit (lo, juce::jmax (lo, hi), endBeat);
    tidyLine (line);
}

void unpin (Lyrics& lyrics, int index, int syllable)
{
    if (index < 0 || index >= (int) lyrics.lines.size())
        return;

    auto& line = lyrics.lines[(size_t) index];

    for (size_t i = 0; i < line.syllables.size(); ++i)
        if (syllable < 0 || (int) i == syllable)
            line.syllables[i].pinned = false;

    tidyLine (line);
}

void tap (Lyrics& lyrics, Position p, double beat)
{
    if (! isValid (lyrics, p))
        return;

    auto& line = lyrics.lines[(size_t) p.line];
    auto& s = line.syllables;
    const auto index = (size_t) p.syllable;
    beat = juce::jmax (0.0, beat);

    if (index == 0)
    {
        shiftLine (line, beat - line.startBeat);

        if (p.line > 0)
        {
            auto& previous = lyrics.lines[(size_t) p.line - 1];
            previous.endBeat = juce::jmin (previous.endBeat, beat);
            tidyLine (previous);
        }
    }
    else
    {
        // Syllables after it placed before this tap are from an earlier pass: they follow again.
        for (size_t i = index + 1; i < s.size(); ++i)
            if (s[i].pinned && s[i].beat <= beat + minGap * (double) (i - index))
                s[i].pinned = false;

        const double previousBeat = s[index - 1].beat;
        s[index].beat = juce::jmax (beat, previousBeat + minGap);
        s[index].pinned = true;

        if (index + 1 == s.size())
            line.endBeat = s[index].beat + juce::jlimit (0.5, 2.0, s[index].beat - previousBeat);
        else if (line.endBeat < s[index].beat + minGap * (double) (s.size() - index))
            line.endBeat = s[index].beat + (double) (s.size() - index) * beatsPerSyllable;
    }

    tidyLine (line);
}
//==============================================================================
namespace
{
    bool isPlainVowel (juce::juce_wchar c) noexcept
    {
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
    }

    bool isOneOf (const juce::String& text, std::initializer_list<const char*> list)
    {
        return std::any_of (list.begin(), list.end(), [&text] (const char* item) { return text == item; });
    }

    std::vector<int> findSyllableStarts (const juce::String& word);

    /** The starts of `first`'s syllables, `at`, then `second`'s moved along by `at`. */
    std::vector<int> joinStarts (const std::vector<int>& first, int at, const std::vector<int>& second)
    {
        auto starts = first;
        starts.push_back (at);

        for (int s : second)
            starts.push_back (at + s);

        return starts;
    }

    /** Two words joined ("to-night", "every-thing", "may-be"): each half splits on its own. */
    std::optional<int> findCompound (const juce::String& w)
    {
        for (auto* first : { "some", "any", "every", "may", "with", "for", "in", "out", "be", "to", "what", "when", "where",
                             "your", "my", "him", "her" })
        {
            const auto rest = w.substring ((int) std::strlen (first));

            if (w.startsWith (first) && isOneOf (rest, { "thing", "things", "one", "body", "where", "time", "times", "day", "days",
                                                         "how", "way", "ways", "be", "out", "in", "side", "self", "ever", "more",
                                                         "night", "fore", "morrow", "gether" }))
                return (int) std::strlen (first);
        }

        return std::nullopt;
    }

    /** A silent e before an ending ("lone-ly", "move-ment", "care-less") leaves the ending a syllable of its own. */
    std::optional<int> findEnding (const juce::String& w)
    {
        for (auto* ending : { "ly", "ness", "ment", "ful", "less" })
        {
            const int stem = w.length() - (int) std::strlen (ending);

            if (stem >= 3 && w.endsWith (ending) && w[stem - 1] == 'e' && ! isPlainVowel (w[stem - 2]) && w[stem - 2] != 'y')
                return stem;
        }

        return std::nullopt;
    }

    /** Where each syllable after the first starts, as letter indices of a lower-case word of a to z. */
    std::vector<int> findSyllableStarts (const juce::String& w)
    {
        if (auto at = findCompound (w))
            return joinStarts (findSyllableStarts (w.substring (0, *at)), *at, findSyllableStarts (w.substring (*at)));

        if (auto at = findEnding (w))
            return joinStarts (findSyllableStarts (w.substring (0, *at)), *at, {});

        const int n = w.length();

        // Which letters sound as vowels. y does after a consonant or at the end ("baby", "my"), not at
        // the start ("you") or before a vowel ("beyond"); u after q doesn't ("queen").
        std::vector<bool> vowel ((size_t) n, false);

        for (int i = 0; i < n; ++i)
        {
            if (w[i] == 'y')
                vowel[(size_t) i] = i > 0 && ! (i + 1 < n && isPlainVowel (w[i + 1]));
            else
                vowel[(size_t) i] = isPlainVowel (w[i]) && ! (w[i] == 'u' && i > 0 && w[i - 1] == 'q');
        }

        // Where "-ing" starts, as in "go-ing" or "danc-ing".
        const int ingAt = w.endsWith ("ings") ? n - 4 : (w.endsWith ("ing") ? n - 3 : -1);

        // The groups of vowels, each one syllable, but "i-a" and "i-o" ("pi-a-no", "li-on") are two,
        // except in endings like "-tion", "-cial", "-gion" and "-llion", and so is a vowel before "-ing".
        std::vector<std::pair<int, int>> groups;

        for (int i = 0; i < n;)
        {
            if (! vowel[(size_t) i])
            {
                ++i;
                continue;
            }

            int end = i;

            while (end < n && vowel[(size_t) end])
                ++end;

            const bool beforeIng = end - i >= 2 && end - 1 == ingAt;
            const bool twoVowels = end - i == 2 && w[i] == 'i' && (w[i + 1] == 'a' || w[i + 1] == 'o') && i > 0
                                   && ! juce::String ("cstxgn").containsChar (w[i - 1]) && ! (i >= 2 && w[i - 1] == 'l' && w[i - 2] == 'l');

            if (beforeIng || twoVowels)
            {
                groups.push_back ({ i, end - 1 });
                groups.push_back ({ end - 1, end });
            }
            else
            {
                groups.push_back ({ i, end });
            }

            i = end;
        }

        // A silent e at the end ("time", "loves", "smiled"), but not "-le" after a consonant ("lit-tle",
        // "tum-bled"), "-es" after a hissing sound ("kiss-es", "chan-ges") or "-ed" after t or d ("want-ed").
        if (groups.size() > 1)
        {
            const auto [s, e] = groups.back();
            const bool atEnd = s == n - 1;
            const bool beforeSOrD = s == n - 2 && (w[n - 1] == 's' || w[n - 1] == 'd');

            if (e - s == 1 && w[s] == 'e' && (atEnd || beforeSOrD) && s >= 1 && ! vowel[(size_t) s - 1])
            {
                bool sounded = s >= 2 && w[s - 1] == 'l' && ! vowel[(size_t) s - 2];

                if (beforeSOrD && w[n - 1] == 'd')
                    sounded = sounded || w[s - 1] == 't' || w[s - 1] == 'd';

                if (beforeSOrD && w[n - 1] == 's')
                    sounded = sounded || juce::String ("sxzcg").containsChar (w[s - 1])
                              || (s >= 2 && isOneOf (w.substring (s - 2, s), { "ch", "sh" }));

                if (! sounded)
                    groups.pop_back();
            }
        }

        // The consonants between two vowel groups: one goes with the next syllable ("ba-by"), two are
        // shared ("hap-py"), but pairs that make one sound stay together ("fa-ther", "sing-er") and so
        // do pairs ending in l or r that start a syllable ("ta-ble", "chil-dren").
        std::vector<int> starts;

        for (size_t k = 0; k + 1 < groups.size(); ++k)
        {
            const int a = groups[k].second, b = groups[k + 1].first, count = b - a;
            auto pairAt = [&w] (int i) { return w.substring (i, i + 2); };
            int start = a + 1;

            if (count == 0)
                start = b;
            else if (b == ingAt)
                start = count >= 2 && w[ingAt - 1] == w[ingAt - 2] ? ingAt - 1 : ingAt;   // "run-ning", "danc-ing"
            else if (count == 1)
                start = w[a] == 'x' ? b : a;
            else if (count == 2 && isOneOf (pairAt (a), { "ch", "sh", "th", "ph", "wh" }))
                start = a;
            else if (count == 2 && isOneOf (pairAt (a), { "ck", "ng", "gh" }))
                start = b;
            else if (isOneOf (pairAt (b - 2), { "bl", "br", "cl", "cr", "dr", "fl", "fr", "gl", "gr", "pl", "pr", "tr", "wr" }))
                start = b - 2;
            else if (pairAt (a) == "gh" && w[a + 2] == 't')
                start = a + 3;   // "light-ning"
            else if (isOneOf (pairAt (a), { "ch", "sh", "th", "ph", "wh", "ck", "ng" }))
                start = a + 2;

            starts.push_back (start);
        }

        return starts;
    }
}

juce::StringArray splitEnglishWord (const juce::String& word)
{
    // Hyphens already split it ("ooh-ooh", "well-known"); the parts may split further.
    if (word.containsChar ('-'))
    {
        juce::StringArray parts;

        for (const auto& piece : juce::StringArray::fromTokens (word, "-", ""))
            if (piece.isNotEmpty())
                parts.addArray (splitEnglishWord (piece));

        return parts.isEmpty() ? juce::StringArray { word } : parts;
    }

    juce::String letters;
    std::vector<int> where;   // each letter's index in the word
    bool sure = true;

    for (int i = 0; i < word.length(); ++i)
    {
        const auto c = word[i];

        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            if (c <= 'Z' && letters.isNotEmpty())
                sure = false;   // capitals inside: a name or an abbreviation

            letters += juce::String::charToString (juce::CharacterFunctions::toLowerCase (c));
            where.push_back (i);
        }
        else if (juce::CharacterFunctions::isLetterOrDigit (c))
        {
            sure = false;   // a number, or a letter English spelling rules don't cover
        }
    }

    if (! sure || letters.length() < 4)
        return { word };

    juce::StringArray parts;
    int from = 0, previous = 0;

    for (int start : findSyllableStarts (letters))
    {
        if (start <= previous || start >= letters.length())
            continue;

        const int at = where[(size_t) start];
        parts.add (word.substring (from, at));
        from = at;
        previous = start;
    }

    parts.add (word.substring (from));
    return parts;
}

namespace brief
{
    const std::vector<Option>& getMoods()
    {
        static const std::vector<Option> moods { { {}, "Any" }, { "hopeful", "Hopeful" }, { "happy", "Happy" }, { "sad", "Sad" }, { "longing", "Longing" },
                                                 { "angry", "Angry" }, { "calm", "Calm" }, { "defiant", "Defiant" }, { "playful", "Playful" }, { "dark", "Dark" } };
        return moods;
    }

    const std::vector<Option>& getPointsOfView()
    {
        static const std::vector<Option> views { { {}, "Any" }, { "i", "I" }, { "you", "You" }, { "we", "We" }, { "story", "A story: he, she, they" } };
        return views;
    }

    LyricsBrief tidy (LyricsBrief b)
    {
        auto known = [] (const std::vector<Option>& list, const juce::String& id)
        {
            return std::any_of (list.begin(), list.end(), [&] (auto& o) { return o.first == id; });
        };

        b.about = b.about.trim();

        if (! known (getMoods(), b.mood))
            b.mood = {};

        if (! known (getPointsOfView(), b.pointOfView))
            b.pointOfView = {};

        return b;
    }
} // namespace brief
} // namespace daw::lyrics
