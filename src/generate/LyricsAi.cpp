#include "LyricsAi.h"
#include "LyricsGuide.h"
#include "Pronunciation.h"

#include <map>
#include <set>

namespace daw::gen::lyricsai
{
namespace
{
    //==============================================================================
    // The text of the lyrics box as lines and verses.

    struct TextLine
    {
        int start = 0, end = 0;     // characters; `end` is before the newline
        juce::String text;
        bool heading = false, blank = false;
        int verse = -1;
        int number = 0;             // 1-based among the content lines of its verse
    };

    struct VerseInfo
    {
        juce::String heading;
        std::vector<int> lines;     // indices of its content lines
        int spanStart = 0, spanEnd = 0;
    };

    struct Layout
    {
        std::vector<TextLine> lines;
        std::vector<VerseInfo> verses;
    };

    bool isHeadingText (const juce::String& t)
    {
        const auto s = t.trim();
        return s.startsWithChar ('[') && s.endsWithChar (']');
    }

    Layout layoutOf (const juce::String& text)
    {
        Layout layout;
        int at = 0, unnamed = 0, current = -1;
        const int length = text.length();

        while (at <= length)
        {
            int end = text.substring (at).indexOfChar ('\n');
            end = end < 0 ? length : at + end;

            TextLine line;
            line.start = at;
            line.end = end;
            line.text = text.substring (at, end);
            line.blank = line.text.trim().isEmpty();
            line.heading = ! line.blank && isHeadingText (line.text);

            if (line.blank)
                current = -1;
            else if (line.heading)
            {
                VerseInfo v;
                v.heading = line.text.trim().substring (1, line.text.trim().length() - 1).trim();
                v.spanStart = line.start;
                v.spanEnd = line.end;
                layout.verses.push_back (v);
                current = (int) layout.verses.size() - 1;
                line.verse = current;
            }
            else
            {
                if (current < 0)
                {
                    VerseInfo v;
                    v.heading = "Verse " + juce::String (++unnamed);
                    v.spanStart = line.start;
                    layout.verses.push_back (v);
                    current = (int) layout.verses.size() - 1;
                }

                auto& v = layout.verses[(size_t) current];
                v.lines.push_back ((int) layout.lines.size());
                v.spanEnd = line.end;
                line.verse = current;
                line.number = (int) v.lines.size();
            }

            layout.lines.push_back (line);

            if (end >= length)
                break;

            at = end + 1;
        }

        return layout;
    }

    int findLineAt (const Layout& layout, int position)
    {
        for (size_t i = 0; i < layout.lines.size(); ++i)
            if (position >= layout.lines[i].start && position <= layout.lines[i].end)
                return (int) i;

        return layout.lines.empty() ? -1 : (int) layout.lines.size() - 1;
    }

    juce::String lastWordOf (const juce::String& line)
    {
        const auto words = juce::StringArray::fromTokens (line.trim(), " \t", "");
        return words.isEmpty() ? juce::String() : pronunciation::cleanWord (words[words.size() - 1].removeCharacters ("-"));
    }

    /** The letters only, lower case: two lines with the same letters are the same line. */
    juce::String normalise (const juce::String& s)
    {
        juce::String out;
        const auto lower = s.toLowerCase();

        for (auto p = lower.getCharPointer(); ! p.isEmpty();)
            if (const auto c = p.getAndAdvance(); juce::CharacterFunctions::isLetterOrDigit (c))
                out += juce::String::charToString (c);

        return out;
    }

    juce::String ordinalWord (int n, const juce::String& one, const juce::String& many)
    {
        return juce::String (n) + " " + (n == 1 ? one : many);
    }

    bool isRhymeKind (rhyme::Kind k)
    {
        return k != rhyme::Kind::none && k != rhyme::Kind::unknown;
    }

    /** How two words rhyme: by sound, else by the spelling. */
    rhyme::Kind rhymeKindOf (const juce::String& a, const juce::String& b)
    {
        const auto k = rhyme::classify (a, b);

        if (k != rhyme::Kind::unknown)
            return k;

        return lyricsguide::rhymes (a, b) ? rhyme::Kind::perfect : rhyme::Kind::none;
    }

    /** "ends on light: rhymes with night (line 2)", "ends on ride: a family rhyme with night (line 2)". */
    juce::String rhymeSentence (const juce::String& word, rhyme::Kind kind, const juce::String& partner, int line, bool withEnds)
    {
        juce::String s = withEnds ? "ends on " + word + ": " : juce::String();
        const auto where = partner + (line > 0 ? " (line " + juce::String (line) + ")" : juce::String());

        switch (kind)
        {
            case rhyme::Kind::perfect:    return s + "rhymes with " + where;
            case rhyme::Kind::identical:
            case rhyme::Kind::additive:
            case rhyme::Kind::assonance:  return s + "an " + rhyme::getKindName (kind) + " rhyme with " + where;
            case rhyme::Kind::family:
            case rhyme::Kind::consonance: return s + "a " + rhyme::getKindName (kind) + " rhyme with " + where;
            case rhyme::Kind::none:
            case rhyme::Kind::unknown:    break;
        }

        return s + "doesn't rhyme with " + where;
    }

    void fillSyllables (Context& c, const std::vector<juce::String>& lines)
    {
        for (auto& l : lines)
        {
            c.lineSyllables.push_back (rhyme::countSyllables (l));
            c.lineStress.push_back (rhyme::getStressPattern (l));
        }
    }

    /** The rhyme partner of each replaced line (by the verse's scheme), leaving out the lines being replaced. */
    void fillPartners (Context& c, const std::vector<int>& replacedIndices)
    {
        c.partners.assign (replacedIndices.size(), {});

        for (size_t t = 0; t < replacedIndices.size(); ++t)
        {
            const int j = replacedIndices[t];

            if (j < 0 || j >= c.scheme.length() || c.scheme[j] == 'X')
                continue;

            int best = -1;

            for (int k = 0; k < c.scheme.length(); ++k)
                if (k != j && c.scheme[k] == c.scheme[j] && std::find (replacedIndices.begin(), replacedIndices.end(), k) == replacedIndices.end())
                    if (best < 0 || std::abs (k - j) < std::abs (best - j))
                        best = k;

            if (best >= 0)
                c.partners[t] = { best + 1, c.lastWords[best] };
        }
    }

    void fillVerse (Context& c, const Layout& layout, int verse)
    {
        c.lastWords.clear();

        if (verse < 0)
            return;

        for (int idx : layout.verses[(size_t) verse].lines)
            c.lastWords.add (lastWordOf (layout.lines[(size_t) idx].text));

        c.scheme = lyricsguide::findRhymeScheme (c.lastWords);
    }

    int getVerseAround (const Layout& layout, int position)
    {
        // The verse the cursor is in, else the one before it, else none.
        int best = -1;

        for (size_t v = 0; v < layout.verses.size(); ++v)
        {
            if (layout.verses[v].spanStart <= position)
                best = (int) v;

            if (position >= layout.verses[v].spanStart && position <= layout.verses[v].spanEnd)
                return (int) v;
        }

        return best;
    }

    bool isSungType (SectionType type)
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

    bool repeatsWords (SectionType type)
    {
        return type == SectionType::chorus || type == SectionType::preChorus || type == SectionType::postChorus;
    }

    //==============================================================================
    juce::var makeObject (std::initializer_list<std::pair<const char*, juce::var>> properties)
    {
        auto* o = new juce::DynamicObject();

        for (auto& [name, value] : properties)
            o->setProperty (name, value);

        return juce::var (o);
    }

    juce::var makeArray (std::initializer_list<juce::var> items)
    {
        juce::Array<juce::var> a;

        for (auto& i : items)
            a.add (i);

        return a;
    }

    juce::var listSchema (const juce::var& items, int minItems, int maxItems)
    {
        return makeObject ({ { "type", "array" }, { "items", items }, { "minItems", minItems }, { "maxItems", maxItems } });
    }

    juce::var stringsSchema (int count)
    {
        return listSchema (makeObject ({ { "type", "string" } }), count, count);
    }

    juce::String getMoodText (const juce::String& id)
    {
        for (auto& [i, name] : lyrics::brief::getMoods())
            if (i == id && i.isNotEmpty())
                return name.toLowerCase();

        return {};
    }

    juce::String getViewText (const juce::String& id)
    {
        if (id == "i")     return "first person: \"I\" (a confession)";
        if (id == "you")   return "singing to someone: \"you\"";
        if (id == "we")    return "\"we\"";
        if (id == "story") return "a story about others: he, she, they";
        return {};
    }

    /** Whether any word of the text is missing from the pronouncing dictionary (headings are skipped). */
    bool anyGuessed (const juce::String& text)
    {
        for (auto& line : juce::StringArray::fromLines (text))
        {
            if (isHeadingText (line))
                continue;

            for (auto& w : juce::StringArray::fromTokens (line, " \t", ""))
                if (const auto plain = w.removeCharacters ("-"); normalise (plain).isNotEmpty() && ! rhyme::isKnown (plain))
                    return true;
        }

        return false;
    }

    //==============================================================================
    /** One line as it should be in the text: no numbering or bullets, no quotes, single spaces. */
    juce::String cleanLine (juce::String s)
    {
        s = s.trim();

        // 1. / 1) / 1: and bullets.
        int i = 0;
        while (i < s.length() && juce::CharacterFunctions::isDigit (s[i]))
            ++i;

        if (i > 0 && i < s.length() && (s[i] == '.' || s[i] == ')' || s[i] == ':') && i + 1 < s.length() && s[i + 1] == ' ')
            s = s.substring (i + 1).trim();
        else if (s.length() > 2 && juce::String ("-*").containsChar (s[0]) && s[1] == ' ')
            s = s.substring (2).trim();

        const juce::String quotes = juce::String::fromUTF8 ("\"\xe2\x80\x9c\xe2\x80\x9d'");

        while (s.length() >= 2 && quotes.containsChar (s[0]) && quotes.containsChar (s[s.length() - 1]))
            s = s.substring (1, s.length() - 1).trim();

        while (s.contains ("  "))
            s = s.replace ("  ", " ");

        return s;
    }

    bool hasLetters (const juce::String& s) { return normalise (s).isNotEmpty(); }

    juce::var parseJson (juce::String reply)
    {
        reply = reply.trim();

        if (reply.startsWith ("```"))
        {
            const int firstBrace = reply.indexOfChar ('{');
            const int lastBrace = reply.lastIndexOfChar ('}');

            if (firstBrace >= 0 && lastBrace > firstBrace)
                reply = reply.substring (firstBrace, lastBrace + 1);
        }

        return juce::JSON::parse (reply);
    }

    juce::StringArray linesOf (const juce::var& v)
    {
        juce::StringArray out;

        if (auto* a = v.getArray())
        {
            for (auto& e : *a)
                for (auto& piece : juce::StringArray::fromLines (e.toString()))
                    if (const auto line = cleanLine (piece); hasLetters (line))
                        out.add (rhyme::hyphenate (line));
        }
        else if (v.isString())
        {
            for (auto& piece : juce::StringArray::fromLines (v.toString()))
                if (const auto line = cleanLine (piece); hasLetters (line))
                    out.add (rhyme::hyphenate (line));
        }

        return out;
    }

    juce::String rangeText (int low, int high)
    {
        return low == high ? ordinalWord (low, "syllable", "syllables") : juce::String (low) + " to " + juce::String (high) + " syllables";
    }

    /** "4 lines, 7 to 9 syllables" for lines of a section. */
    juce::String summarise (const std::vector<int>& counts, const juce::StringArray& lines)
    {
        const auto [low, high] = std::minmax_element (counts.begin(), counts.end());
        auto s = ordinalWord (lines.size(), "line", "lines") + ", " + (counts.empty() ? juce::String ("0 syllables") : rangeText (*low, *high));
        juce::StringArray last;

        for (auto& l : lines)
            last.add (lastWordOf (l));

        if (const auto scheme = lyricsguide::findRhymeScheme (last); scheme.containsAnyOf ("ABCDEFGH"))
            s << ", rhymes " << scheme;

        return s;
    }

    juce::String songText (const SongPlan& plan, const std::vector<juce::StringArray>& written)
    {
        juce::String text;

        auto add = [&] (size_t item)
        {
            if (text.isNotEmpty())
                text << "\n\n";

            text << "[" << plan.items[item].heading << "]\n" << written[item].joinIntoString ("\n");
        };

        if (plan.isDefaultForm)
            for (int item : plan.order)
                add ((size_t) item);
        else
            for (size_t i = 0; i < plan.items.size(); ++i)
                add (i);

        return text;
    }

    void labelSyllables (Alternative& a, const Context& c, Kind kind)
    {
        if (kind == Kind::selection && c.wholeLines && c.linesWanted > 1)
        {
            juce::StringArray now, before;
            bool far = false;

            for (size_t i = 0; i < a.lineSyllables.size() && i < c.lineSyllables.size(); ++i)
            {
                now.add (juce::String (a.lineSyllables[i]));
                before.add (juce::String (c.lineSyllables[i]));
                far = far || std::abs (a.lineSyllables[i] - c.lineSyllables[i]) >= 2;
            }

            a.syllableLabel = now.joinIntoString (", ") + " syllables, the original has " + before.joinIntoString (", ");
            a.syllableWarn = far;
            return;
        }

        int original = 0;
        juce::String ref = "the original";

        if (kind == Kind::selection && ! c.lineSyllables.empty())
            original = c.lineSyllables[0];
        else if (kind == Kind::nextLine)
        {
            original = c.previousSyllables;
            ref = "the line before";
        }

        if (original <= 0)
            return;

        const int diff = a.syllables - original;
        auto s = ordinalWord (a.syllables, "syllable", "syllables");

        if (diff == 0)
            s << ", like " << ref;
        else
            s << ", " << std::abs (diff) << (diff > 0 ? " more" : " fewer") << " than " << ref;

        if (std::abs (diff) >= 2)
        {
            s << ": harder to sing to the same melody";
            a.syllableWarn = true;
        }

        a.syllableLabel = s;
    }

    void labelRhyme (Alternative& a, const Context& c, Kind kind)
    {
        if (a.lines.isEmpty())
            return;

        const auto last = lastWordOf (a.lines[a.lines.size() - 1]);

        if (kind == Kind::selection && ! (c.wholeLines && c.linesWanted > 1) && c.endsLine && ! c.partners.empty() && c.partners[0].line > 0)
        {
            const auto k = rhymeKindOf (last, c.partners[0].word);
            a.rhymeWarn = ! isRhymeKind (k);
            a.rhymeLabel = a.rhymeWarn ? "doesn't rhyme with " + c.partners[0].word + " (line " + juce::String (c.partners[0].line) + ") any more"
                                       : rhymeSentence (last, k, c.partners[0].word, c.partners[0].line, true);
        }
        else if (kind == Kind::nextLine)
        {
            for (int k = juce::jmin (c.lastLine, c.lastWords.size()); k >= 1; --k)
                if (const auto kindOf = rhymeKindOf (last, c.lastWords[k - 1]); isRhymeKind (kindOf))
                {
                    a.rhymeLabel = rhymeSentence (last, kindOf, c.lastWords[k - 1], k, false);
                    break;
                }
        }
    }

    juce::String getLinesText (const juce::StringArray& lines) { return lines.joinIntoString ("\n"); }
}

//==============================================================================
int getLinesForBars (int bars)
{
    return juce::jlimit (2, 8, bars / 2);
}

int getAlternativeCount (Kind kind)
{
    switch (kind)
    {
        case Kind::song:           return 1;
        case Kind::sectionRewrite:
        case Kind::sectionAdd:     return 2;
        case Kind::nextLine:
        case Kind::selection:      return 3;
        case Kind::rhymes:         return 1;
    }

    return 1;
}

SongPlan buildSongPlan (const SectionList& sections)
{
    SongPlan plan;
    std::set<SectionType> written;

    for (auto& s : sections)
    {
        if (! isSungType (s.type) || (repeatsWords (s.type) && written.count (s.type) > 0))
            continue;

        written.insert (s.type);
        plan.items.push_back ({ sectionTypes::getName (s.type), s.type, getLinesForBars (s.bars) });
    }

    if (plan.items.empty())
    {
        plan.isDefaultForm = true;
        plan.items = { { "Verse", SectionType::verse, 4 }, { "Chorus", SectionType::chorus, 4 }, { "Verse", SectionType::verse, 4 },
                       { "Bridge", SectionType::bridge, 4 } };
        plan.order = { 0, 1, 2, 1, 3, 1 };
        plan.note = "No sections yet: written as verse, chorus, verse, chorus, bridge, chorus";
    }

    return plan;
}

//==============================================================================
Context buildContext (const Request& r)
{
    Context c;
    c.kind = r.kind;

    const auto layout = layoutOf (r.text);
    const int length = r.text.length();
    const int start = juce::jlimit (0, length, r.rangeStart);
    const int end = juce::jlimit (start, length, r.rangeEnd);

    auto contentLinesOf = [&] (int verse)
    {
        std::vector<juce::String> lines;

        if (verse >= 0)
            for (int idx : layout.verses[(size_t) verse].lines)
                lines.push_back (layout.lines[(size_t) idx].text);

        return lines;
    };

    switch (r.kind)
    {
        case Kind::song:
        {
            c.replaceStart = 0;
            c.replaceEnd = length;
            c.target = r.text;
            c.wholeLines = true;
            break;
        }

        case Kind::rhymes:
        {
            int a = start, b = end;
            auto isWordChar = [&] (juce::juce_wchar ch) { return juce::CharacterFunctions::isLetter (ch) || ch == '\'' || ch == '-'; };

            if (a == b)
            {
                while (a > 0 && isWordChar (r.text[a - 1]))
                    --a;

                while (b < length && isWordChar (r.text[b]))
                    ++b;
            }
            else
            {
                while (a < b && ! isWordChar (r.text[a])) ++a;
                while (b > a && ! isWordChar (r.text[b - 1])) --b;
            }

            if (a >= b)
            {
                c.error = "Select a word, or put the cursor in one.";
                break;
            }

            c.replaceStart = a;
            c.replaceEnd = b;
            c.target = r.text.substring (a, b);
            c.word = pronunciation::cleanWord (c.target.removeCharacters ("-"));

            if (const int li = findLineAt (layout, a); li >= 0)
            {
                const auto& line = layout.lines[(size_t) li];
                c.lineText = line.text.trim();
                c.endsLine = b >= line.end - (line.text.length() - line.text.trimEnd().length());
                c.heading = line.verse >= 0 ? layout.verses[(size_t) line.verse].heading : juce::String();
                c.firstLine = c.lastLine = line.number;
                fillVerse (c, layout, line.verse);
            }

            break;
        }

        case Kind::nextLine:
        {
            int li = findLineAt (layout, start);

            if (li >= 0 && (layout.lines[(size_t) li].blank || layout.lines[(size_t) li].heading))
            {
                // On a blank or heading line: continue the verse before it, when there is one.
                int back = li - 1;

                while (back >= 0 && layout.lines[(size_t) back].blank)
                    --back;

                li = back >= 0 && ! layout.lines[(size_t) back].heading ? back : li;
            }

            if (li < 0 || layout.lines[(size_t) li].blank || layout.lines[(size_t) li].heading)
            {
                // Nothing to follow: the first line of a new verse, at the end of the text (or at the heading).
                const int verse = li >= 0 ? layout.lines[(size_t) li].verse : -1;
                c.heading = verse >= 0 ? layout.verses[(size_t) verse].heading : juce::String ("Verse");
                c.replaceStart = c.replaceEnd = li >= 0 ? layout.lines[(size_t) li].end : length;
                c.prefix = c.replaceStart > 0 ? "\n" : "";
                c.lastLine = 0;
                fillVerse (c, layout, verse);
            }
            else
            {
                const auto& line = layout.lines[(size_t) li];
                c.heading = layout.verses[(size_t) line.verse].heading;
                c.lastLine = line.number;
                c.replaceStart = c.replaceEnd = line.end;
                c.prefix = "\n";
                c.previousSyllables = rhyme::countSyllables (line.text);
                fillVerse (c, layout, line.verse);
            }

            c.firstLine = c.lastLine + 1;
            c.linesWanted = 1;
            c.anchor = r.text.substring (juce::jmax (0, c.replaceStart - 24), c.replaceStart);
            break;
        }

        case Kind::selection:
        {
            std::vector<int> covered;

            for (size_t i = 0; i < layout.lines.size(); ++i)
            {
                auto& l = layout.lines[i];

                if (! l.blank && ! l.heading && l.end >= start && l.start <= end && (end > start ? l.end > start && l.start < end : true))
                    covered.push_back ((int) i);
            }

            if (covered.empty() || start == end)
            {
                c.error = "Select some words or lines first.";
                break;
            }

            const int verse = layout.lines[(size_t) covered.front()].verse;

            if (std::any_of (covered.begin(), covered.end(), [&] (int i) { return layout.lines[(size_t) i].verse != verse; }))
            {
                c.error = "Select lines from one verse.";
                break;
            }

            c.heading = layout.verses[(size_t) verse].heading;
            fillVerse (c, layout, verse);
            const auto& first = layout.lines[(size_t) covered.front()];
            const auto& last = layout.lines[(size_t) covered.back()];
            c.firstLine = first.number;
            c.lastLine = last.number;

            const int trimmedStart = first.start + (first.text.length() - first.text.trimStart().length());
            const int trimmedEnd = last.end - (last.text.length() - last.text.trimEnd().length());
            const bool coversAll = start <= trimmedStart && end >= trimmedEnd;

            if (covered.size() == 1 && ! coversAll)
            {
                // Words inside one line.
                int a = start, b = end;

                while (a < b && juce::CharacterFunctions::isWhitespace (r.text[a])) ++a;
                while (b > a && juce::CharacterFunctions::isWhitespace (r.text[b - 1])) --b;

                c.wholeLines = false;
                c.replaceStart = a;
                c.replaceEnd = b;
                c.target = r.text.substring (a, b);
                c.linesWanted = 1;
                c.endsLine = b >= trimmedEnd;
                fillSyllables (c, { c.target });
                fillPartners (c, { first.number - 1 });
            }
            else
            {
                c.wholeLines = true;
                c.replaceStart = first.start;
                c.replaceEnd = last.end;
                c.target = r.text.substring (first.start, last.end);
                c.linesWanted = (int) covered.size();
                c.endsLine = true;
                std::vector<juce::String> lines;
                std::vector<int> indices;

                for (int i : covered)
                {
                    lines.push_back (layout.lines[(size_t) i].text);
                    indices.push_back (layout.lines[(size_t) i].number - 1);
                }

                fillSyllables (c, lines);
                fillPartners (c, indices);
            }

            break;
        }

        case Kind::sectionRewrite:
        {
            const int verse = getVerseAround (layout, start);

            if (verse < 0 || layout.verses[(size_t) verse].lines.empty())
            {
                c.error = "Put the cursor in a section with words.";
                break;
            }

            auto& v = layout.verses[(size_t) verse];
            const auto& first = layout.lines[(size_t) v.lines.front()];
            const auto& last = layout.lines[(size_t) v.lines.back()];
            c.heading = v.heading;
            c.firstLine = 1;
            c.lastLine = (int) v.lines.size();
            c.wholeLines = true;
            c.replaceStart = first.start;
            c.replaceEnd = last.end;
            c.target = r.text.substring (first.start, last.end);
            c.linesWanted = (int) v.lines.size();
            c.endsLine = true;
            fillVerse (c, layout, verse);
            fillSyllables (c, contentLinesOf (verse));
            break;
        }

        case Kind::sectionAdd:
        {
            const int verse = getVerseAround (layout, start);
            c.heading = r.addHeading;
            c.wholeLines = true;
            c.linesWanted = getLinesForBars (r.addBars);
            c.replaceStart = c.replaceEnd = verse >= 0 ? layout.verses[(size_t) verse].spanEnd : length;
            c.prefix = (c.replaceStart > 0 ? "\n\n[" : "[") + r.addHeading + "]\n";
            c.anchor = r.text.substring (juce::jmax (0, c.replaceStart - 24), c.replaceStart);
            fillVerse (c, layout, verse);
            break;
        }
    }

    return c;
}

//==============================================================================
juce::String buildSystemPrompt()
{
    return "You are a songwriting partner for a beginner. You write song lyrics in English that are meant to be sung, not read.\n"
           "Craft rules:\n"
           "- Show, don't tell: prefer a concrete image (a coat by the door, two cups on the table) to a named feeling.\n"
           "- Use everyday words, said the way people speak: lyrics are heard once, at speed.\n"
           "- Keep one idea, one point of view and one tense through the song.\n"
           "- Lines in a verse have about the same number of syllables, and their stresses fall in the same places.\n"
           "- Follow the rhyme scheme of the lyrics you are given. A rhyme matches the last stressed vowel and everything after it; "
           "near rhymes are fine.\n"
           "- Never reuse lines from existing songs, and don't copy the original text you are asked to replace.\n"
           "- Write only the words to sing: no numbering, no quotation marks, no chords, no explanations inside the lines.\n"
           "Answer only with JSON that follows the schema you are given.";
}

juce::String buildUserPrompt (const Request& r, const Context& c)
{
    juce::String p;
    const auto& b = r.brief;

    // The brief.
    if (b.about.isNotEmpty() || b.mood.isNotEmpty() || b.pointOfView.isNotEmpty())
    {
        p << "Song brief:\n";

        if (b.about.isNotEmpty())       p << "- About: " << b.about << "\n";
        if (b.mood.isNotEmpty())        p << "- Mood: " << getMoodText (b.mood) << "\n";
        if (b.pointOfView.isNotEmpty()) p << "- Point of view: " << getViewText (b.pointOfView) << "\n";

        p << "\n";
    }

    p << "Key: " << r.key.getName() << ". Tempo: " << juce::String (juce::roundToInt (r.bpm)) << " BPM.\n";

    // The sections of the song, with the lines each wants (the chorus is written once).
    juce::StringArray structure;

    for (auto& s : r.sections)
        if (isSungType (s.type))
            structure.add (s.getDisplayName() + " (" + juce::String (s.bars) + " bars, " + ordinalWord (getLinesForBars (s.bars), "line", "lines") + ")");

    if (! structure.isEmpty())
        p << "Sections of the song, in order: " << structure.joinIntoString (", ") << ".\n";

    p << "\n";

    const auto count = getAlternativeCount (r.kind);

    auto marked = [&]
    {
        if (r.text.trim().isEmpty())
            return juce::String ("(no lyrics yet)");

        const auto start = juce::jlimit (0, r.text.length(), c.replaceStart), end = juce::jlimit (start, r.text.length(), c.replaceEnd);
        return r.text.substring (0, start) + (start == end ? "<<<INSERT HERE>>>" : "<<<" + r.text.substring (start, end) + ">>>") + r.text.substring (end);
    };

    switch (r.kind)
    {
        case Kind::song:
        {
            const auto plan = buildSongPlan (r.sections);
            p << "Task: write the lyrics of the whole song. Write these sections, each once, in this order";

            if (plan.isDefaultForm)
                p << " (the chorus is sung three times: verse, chorus, verse, chorus, bridge, chorus)";
            else
                p << " (a chorus is sung every time its section comes, so write it once)";

            p << ":\n";

            for (auto& item : plan.items)
                p << "- " << item.heading << ": exactly " << ordinalWord (item.lines, "line", "lines") << "\n";

            p << "New words for every verse. Give exactly 1 version.\n";

            if (r.text.trim().isNotEmpty())
                p << "\nThe lyrics so far (they will be replaced):\n" << r.text << "\n";

            break;
        }

        case Kind::sectionRewrite:
            p << "Task: rewrite the lines of the section [" << c.heading << "] that are marked with <<< and >>>. Each alternative has exactly "
              << ordinalWord (c.linesWanted, "line", "lines") << ". Give exactly " << count << " alternatives.\n\nThe lyrics:\n" << marked() << "\n";
            break;

        case Kind::sectionAdd:
            p << "Task: write a new section [" << c.heading << "] at <<<INSERT HERE>>>, to follow what is there. Each alternative has exactly "
              << ordinalWord (c.linesWanted, "line", "lines") << ". Give exactly " << count << " alternatives.\n\nThe lyrics:\n" << marked() << "\n";
            break;

        case Kind::nextLine:
            p << "Task: write the next line of " << (c.lastLine > 0 ? "the section [" + c.heading + "], line " + juce::String (c.firstLine) : "a new section")
              << ", at <<<INSERT HERE>>>. Each alternative is exactly 1 line. Give exactly " << count << " alternatives.\n\nThe lyrics:\n" << marked() << "\n";
            break;

        case Kind::selection:
            p << "Task: rewrite " << (c.wholeLines ? "the " + juce::String (c.linesWanted > 1 ? "lines" : "line") + " marked with <<< and >>>" : "the words marked with <<< and >>>")
              << " in the section [" << c.heading << "]. "
              << (c.wholeLines ? "Each alternative has exactly " + ordinalWord (c.linesWanted, "line", "lines") + "." : "Each alternative is only the replacement words, on 1 line.")
              << " Give exactly " << count << " alternatives.\n\nThe lyrics:\n" << marked() << "\n";
            break;

        case Kind::rhymes:
        {
            const auto list = rhyme::findRhymes (c.word, 8);
            juce::StringArray candidates;

            for (auto& item : list.items)
                if (item.kind != rhyme::Kind::identical)
                    candidates.add (item.word);

            p << "Task: pick rhymes for the word \"" << c.word << "\" at the end of the line \"" << c.lineText << "\", the ones that suit the line best. "
              << "Choose from this list from a pronouncing dictionary, or add words of your own that rhyme: " << candidates.joinIntoString (", ")
              << ".\nGive 6 to 12 words, best first.\n\nThe lyrics:\n" << (r.text.trim().isEmpty() ? juce::String (c.lineText) : r.text) << "\n";
            break;
        }
    }

    // What the target looks like.
    if (r.kind == Kind::selection || r.kind == Kind::sectionRewrite)
    {
        for (size_t i = 0; i < c.lineSyllables.size(); ++i)
        {
            juce::String stress;

            for (bool strong : c.lineStress[i])
                stress << (strong ? "S" : "w");

            p << "Original " << (c.wholeLines ? "line " + juce::String (c.firstLine + (int) i) : juce::String ("words")) << ": " << c.lineSyllables[i]
              << " syllables, stress " << stress << " (S strong, w weak).";

            if (i < c.partners.size() && c.partners[i].line > 0)
                p << " It rhymes with \"" << c.partners[i].word << "\" (line " << c.partners[i].line << ").";

            p << "\n";
        }
    }
    else if (r.kind == Kind::nextLine && c.previousSyllables > 0)
        p << "The line before has " << c.previousSyllables << " syllables.\n";

    if (! c.scheme.isEmpty() && r.kind != Kind::song && r.kind != Kind::rhymes)
        p << "The rhyme scheme of this section so far: " << c.scheme << " (X: no rhyme).\n";

    // The asks.
    juce::StringArray asks;
    const auto& a = r.asks;

    if (a.moreConcrete)  asks.add ("Use things you can see, hear or touch instead of named feelings.");
    if (a.simplerWords)  asks.add ("Use simpler, everyday words.");
    if (a.newWords)      asks.add ("Say the same idea without the original's words.");
    if (a.anotherAngle)  asks.add ("Take a different picture or point of view on the same idea.");
    if (a.shorter)       asks.add ("Use fewer syllables than the original.");
    if (a.longer)        asks.add ("Use more syllables than the original.");

    if (a.keepSyllables && (r.kind == Kind::selection || r.kind == Kind::sectionRewrite) && ! a.shorter && ! a.longer && ! c.lineSyllables.empty())
        asks.add ("Keep the number of syllables of each original line, so it fits the same melody.");

    if (a.keepRhyme && (r.kind == Kind::selection || r.kind == Kind::sectionRewrite))
        for (auto& partner : c.partners)
            if (partner.line > 0)
                asks.add ("End on a word that rhymes with \"" + partner.word + "\".");

    if (a.keepRhyme && r.kind == Kind::nextLine && ! c.lastWords.isEmpty() && c.scheme.isNotEmpty())
        asks.add ("Rhyme with an earlier line of the section where the rhyme scheme calls for it.");

    if (a.yourWords.trim().isNotEmpty())
        asks.add (a.yourWords.trim());

    if (! asks.isEmpty() && r.kind != Kind::rhymes)
        p << "\nAsks:\n- " << asks.joinIntoString ("\n- ") << "\n";

    p << "\nReply with JSON only, following this schema:\n" << juce::JSON::toString (buildSchema (r, c), true) << "\n";
    return p;
}

juce::var buildSchema (const Request& r, const Context& c)
{
    const auto text = makeObject ({ { "type", "string" } });

    if (r.kind == Kind::rhymes)
        return makeObject ({ { "type", "object" },
                             { "properties", makeObject ({ { "rhymes", listSchema (text, 6, 12) } }) },
                             { "required", makeArray ({ "rhymes" }) } });

    if (r.kind == Kind::song)
    {
        const auto plan = buildSongPlan (r.sections);
        const auto item = makeObject ({ { "type", "object" },
                                        { "properties", makeObject ({ { "heading", text }, { "lines", listSchema (text, 2, 8) } }) },
                                        { "required", makeArray ({ "heading", "lines" }) } });
        return makeObject ({ { "type", "object" },
                             { "properties", makeObject ({ { "sections", listSchema (item, (int) plan.items.size(), (int) plan.items.size()) } }) },
                             { "required", makeArray ({ "sections" }) } });
    }

    const int count = getAlternativeCount (r.kind);
    const auto item = makeObject ({ { "type", "object" },
                                    { "properties", makeObject ({ { "lines", stringsSchema (c.linesWanted) }, { "note", text } }) },
                                    { "required", makeArray ({ "lines", "note" }) } });
    return makeObject ({ { "type", "object" },
                         { "properties", makeObject ({ { "alternatives", listSchema (item, count, count) } }) },
                         { "required", makeArray ({ "alternatives" }) } });
}

//==============================================================================
Result parseReply (const Request& r, const Context& c, const juce::String& reply)
{
    Result result;
    const auto json = parseJson (reply);

    if (! json.isObject())
    {
        result.error = "The answer wasn't in the form DawIt asked for. Try again.";
        return result;
    }

    //------------------------------------------------------------------ rhymes
    if (r.kind == Kind::rhymes)
    {
        std::set<juce::String> seen { normalise (c.word) };

        if (auto* list = json["rhymes"].getArray())
        {
            for (auto& e : *list)
            {
                const auto raw = e.isObject() ? e["word"].toString() : e.toString();
                const auto w = pronunciation::cleanWord (cleanLine (raw).removeCharacters ("-"));

                if (! hasLetters (w) || w.containsChar (' ') || ! seen.insert (normalise (w)).second)
                    continue;

                RhymePick pick;
                pick.word = w;
                pick.guessed = ! rhyme::isKnown (w) || ! rhyme::isKnown (c.word);
                pick.kind = rhymeKindOf (w, c.word);
                result.rhymes.push_back (pick);
            }
        }

        if (result.rhymes.empty())
            result.error = "No rhymes came back. Try again.";

        return result;
    }

    //------------------------------------------------------------------ the whole song
    if (r.kind == Kind::song)
    {
        const auto plan = buildSongPlan (r.sections);
        std::vector<juce::StringArray> written;
        auto* list = json["sections"].getArray();

        if (list == nullptr || list->size() < (int) plan.items.size())
        {
            result.error = "The answer was missing sections. Try again.";
            return result;
        }

        Alternative alt;
        std::map<juce::String, int> total;

        for (auto& item : plan.items)
            ++total[item.heading];

        std::map<juce::String, int> seen;

        for (size_t i = 0; i < plan.items.size(); ++i)
        {
            auto lines = linesOf ((*list)[(int) i]["lines"]);

            if (lines.size() < plan.items[i].lines)
            {
                result.error = "The " + plan.items[i].heading + " came back with " + juce::String (lines.size()) + " lines instead of "
                               + juce::String (plan.items[i].lines) + ". Try again.";
                return result;
            }

            while (lines.size() > plan.items[i].lines)
                lines.remove (lines.size() - 1);

            std::vector<int> counts;
            for (auto& l : lines)
                counts.push_back (rhyme::countSyllables (l));

            auto name = plan.items[i].heading;

            if (total[name] > 1)
                name << " " << ++seen[name];

            alt.sectionSummaries.push_back (name + ": " + summarise (counts, lines));
            written.push_back (lines);
        }

        alt.text = songText (plan, written);
        alt.lines = juce::StringArray::fromLines (alt.text);

        if (anyGuessed (alt.text))
            alt.guessedLabel = "guessed from the spelling";

        alt.syllables = rhyme::countSyllables (alt.text.replace ("\n[", "\n").replace ("[", "").replace ("]", ""));
        alt.summary = plan.note;
        result.note = plan.note;
        result.alternatives.push_back (alt);
        return result;
    }

    //------------------------------------------------------------------ alternatives of lines
    auto* list = json["alternatives"].getArray();

    if (list == nullptr)
    {
        result.error = "The answer didn't hold any alternatives. Try again.";
        return result;
    }

    const auto original = normalise (c.target);
    std::set<juce::String> seen;
    bool anyWithin = false;
    const bool lineTarget = (r.kind == Kind::selection && c.linesWanted == 1 && ! c.lineSyllables.empty() && c.lineSyllables[0] > 0)
                            || (r.kind == Kind::nextLine && c.previousSyllables > 0);
    const int targetSyllables = r.kind == Kind::nextLine ? c.previousSyllables : (c.lineSyllables.empty() ? 0 : c.lineSyllables[0]);

    for (auto& entry : *list)
    {
        auto lines = linesOf (entry["lines"].isVoid() ? entry["line"] : entry["lines"]);

        if (lines.isEmpty())
            continue;

        // The number of lines: words inside a line are one line; a longer answer is cut, a shorter one rejected.
        if (r.kind == Kind::selection && ! c.wholeLines)
            lines = juce::StringArray (lines.joinIntoString (" "));
        else
        {
            if (lines.size() < c.linesWanted)
                continue;

            while (lines.size() > c.linesWanted)
                lines.remove (lines.size() - 1);
        }

        const auto key = normalise (lines.joinIntoString (" "));

        if ((original.isNotEmpty() && key == original) || ! seen.insert (key).second)
            continue;

        Alternative alt;
        alt.lines = lines;
        alt.text = getLinesText (lines);
        alt.note = entry["note"].toString().trim();

        for (auto& l : lines)
        {
            alt.lineSyllables.push_back (rhyme::countSyllables (l));
            alt.syllables += alt.lineSyllables.back();
        }

        if (anyGuessed (alt.text))
            alt.guessedLabel = "guessed from the spelling";

        if (r.kind == Kind::sectionRewrite || r.kind == Kind::sectionAdd)
            alt.summary = summarise (alt.lineSyllables, lines);
        else
        {
            labelSyllables (alt, c, r.kind);
            labelRhyme (alt, c, r.kind);
        }

        if (lineTarget && std::abs (alt.syllables - targetSyllables) <= 2)
            anyWithin = true;

        result.alternatives.push_back (std::move (alt));
    }

    if (result.alternatives.empty())
        result.error = "Nothing usable came back. Try again.";

    result.shouldRetry = lineTarget && r.asks.keepSyllables && ! r.asks.shorter && ! r.asks.longer && ! anyWithin;
    return result;
}

//==============================================================================
std::optional<Applied> applyAlternative (const juce::String& current, const Context& c, const Alternative& alt)
{
    if (c.replaceStart < 0 || c.replaceEnd < c.replaceStart || c.replaceEnd > current.length())
        return std::nullopt;

    if (current.substring (c.replaceStart, c.replaceEnd) != c.target)
        return std::nullopt;

    if (c.replaceStart == c.replaceEnd && c.anchor.isNotEmpty()
        && current.substring (juce::jmax (0, c.replaceStart - c.anchor.length()), c.replaceStart) != c.anchor)
        return std::nullopt;

    Applied a;
    a.text = current.substring (0, c.replaceStart) + c.prefix + alt.text + current.substring (c.replaceEnd);
    a.start = c.replaceStart + c.prefix.length();
    a.end = a.start + alt.text.length();
    return a;
}

std::optional<Applied> applyWord (const juce::String& current, const Context& c, const juce::String& word)
{
    Alternative alt;
    alt.text = word;
    return applyAlternative (current, c, alt);
}

//==============================================================================
juce::String getCannedReply (const Request& r, const Context& c, bool withLongOne)
{
    static const char* stock[] { "your coat still hangs beside the door", "two cups, and one is never full", "the empty chair where you would sit",
                                 "I count the lamps along the street", "the radio plays our old song", "a taxi hums into the rain",
                                 "I'm learning how to walk alone", "the morning finds me on the stairs" };
    const char* notes[] { "a picture instead of a feeling", "a detail only this song has", "a place that says they've gone" };

    auto lineList = [] (int count, int from)
    {
        juce::Array<juce::var> a;

        for (int i = 0; i < count; ++i)
            a.add (juce::String (stock[(from + i) % 8]));

        return a;
    };

    if (r.kind == Kind::rhymes)
        return juce::JSON::toString (makeObject ({ { "rhymes", makeArray ({ "light", "bright", "tonight", "sight", "ride", "time", "polite" }) } }), true);

    if (r.kind == Kind::song)
    {
        const auto plan = buildSongPlan (r.sections);
        juce::Array<juce::var> sections;

        for (size_t i = 0; i < plan.items.size(); ++i)
            sections.add (makeObject ({ { "heading", plan.items[i].heading }, { "lines", lineList (plan.items[i].lines, (int) i * 3) } }));

        return juce::JSON::toString (makeObject ({ { "sections", sections } }), true);
    }

    juce::Array<juce::var> alternatives;
    const int count = getAlternativeCount (r.kind);

    for (int i = 0; i < count; ++i)
    {
        auto lines = lineList (c.linesWanted, i * 3);

        if (withLongOne && i == count - 1 && ! lines.isEmpty())
            lines.set (0, "the empty chair where you would sit beside me every single night");

        alternatives.add (makeObject ({ { "lines", lines }, { "note", juce::String (notes[i % 3]) } }));
    }

    return juce::JSON::toString (makeObject ({ { "alternatives", alternatives } }), true);
}
} // namespace daw::gen::lyricsai
