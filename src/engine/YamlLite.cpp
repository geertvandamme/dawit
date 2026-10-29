#include "YamlLite.h"

namespace daw::yaml
{
const Node* Node::find (const juce::String& key) const
{
    if (kind != Kind::map)
        return nullptr;

    for (auto& e : entries)
        if (e.first == key)
            return &e.second;

    return nullptr;
}

juce::String Node::getString (const juce::String& key, const juce::String& fallback) const
{
    const auto* n = find (key);
    return n != nullptr && n->isScalar() ? n->scalar : fallback;
}

int Node::getInt (const juce::String& key, int fallback) const
{
    const auto* n = find (key);
    return n != nullptr && n->isScalar() && n->scalar.trim().containsOnly ("-0123456789") && n->scalar.trim().isNotEmpty() ? n->scalar.getIntValue() : fallback;
}

double Node::getDouble (const juce::String& key, double fallback) const
{
    const auto* n = find (key);
    return n != nullptr && n->isScalar() && n->scalar.trim().containsOnly ("-+.0123456789eE") && n->scalar.trim().isNotEmpty() ? n->scalar.getDoubleValue() : fallback;
}

bool Node::getBool (const juce::String& key, bool fallback) const
{
    const auto* n = find (key);

    if (n == nullptr || ! n->isScalar())
        return fallback;

    const auto s = n->scalar.trim().toLowerCase();
    return s == "true" ? true : s == "false" ? false : fallback;
}

//==============================================================================
namespace
{
    struct Line
    {
        int indent = 0;
        juce::String text;   // without indentation, comment or trailing space
        int number = 0;
    };

    struct Failure
    {
        juce::String message;
    };

    [[noreturn]] void fail (int line, const juce::String& what)
    {
        throw Failure { "line " + juce::String (line) + ": " + what };
    }

    /** The text without its # comment (a # counts when it starts the line or follows a space, outside quotes). */
    juce::String stripComment (const juce::String& s)
    {
        char quote = 0;

        for (int i = 0; i < s.length(); ++i)
        {
            const auto c = s[i];

            if (quote != 0)
            {
                if (c == '\\' && quote == '"')
                    ++i;
                else if (c == (juce::juce_wchar) quote)
                    quote = 0;
            }
            else if (c == '"' || c == '\'')
            {
                // A quote starts a quoted value only at the start of a value, not inside a word (don't).
                if (i == 0 || juce::String (" [{,:-").containsChar (s[i - 1]))
                    quote = (char) c;
            }
            else if (c == '#' && (i == 0 || s[i - 1] == ' ' || s[i - 1] == '\t'))
                return s.substring (0, i);
        }

        return s;
    }

    /** Index of the colon that ends a map key (followed by a space or the end of the line), or -1. */
    int findKeyColon (const juce::String& s)
    {
        char quote = 0;
        int depth = 0;

        for (int i = 0; i < s.length(); ++i)
        {
            const auto c = s[i];

            if (quote != 0)
            {
                if (c == '\\' && quote == '"')
                    ++i;
                else if (c == (juce::juce_wchar) quote)
                    quote = 0;
            }
            else if ((c == '"' || c == '\'') && (i == 0 || juce::String (" [{,").containsChar (s[i - 1])))
                quote = (char) c;
            else if (c == '[' || c == '{')
                ++depth;
            else if (c == ']' || c == '}')
                --depth;
            else if (c == ':' && depth == 0 && (i + 1 == s.length() || s[i + 1] == ' '))
                return i;
        }

        return -1;
    }

    juce::String unquote (const juce::String& s, int line)
    {
        const auto t = s.trim();

        if (t.length() >= 2 && t[0] == '\'' && t.getLastCharacter() == '\'')
            return t.substring (1, t.length() - 1).replace ("''", "'");

        if (t.length() >= 2 && t[0] == '"' && t.getLastCharacter() == '"')
        {
            juce::String out;

            for (int i = 1; i < t.length() - 1; ++i)
            {
                if (t[i] == '\\' && i + 1 < t.length() - 1)
                {
                    const auto e = t[++i];
                    out << (e == 'n' ? juce::String ("\n") : e == 't' ? juce::String ("\t") : juce::String::charToString (e));
                }
                else
                    out << juce::String::charToString (t[i]);
            }

            return out;
        }

        if (t.isNotEmpty() && (t[0] == '"' || t[0] == '\''))
            fail (line, "a quote that is not closed");

        return t;
    }

    void checkPlain (const juce::String& value, int line)
    {
        if (value.isEmpty())
            return;

        const auto c = value[0];

        if (c == '&' || c == '*')
            fail (line, "anchors and aliases are not supported");

        if (c == '!')
            fail (line, "tags are not supported");

        if (c == '|' || c == '>')
            fail (line, "block scalars (| and >) are not supported");
    }

    Node makeScalar (const juce::String& text, int line)
    {
        const auto t = text.trim();
        Node n;
        n.kind = Node::Kind::scalar;

        if (t.isEmpty() || t == "~" || t == "null")
            return n;   // nothing

        checkPlain (t, line);
        n.scalar = unquote (t, line);
        return n;
    }

    /** Reads a flow list or map starting at `pos` in `s` (one line). */
    Node parseFlow (const juce::String& s, int& pos, int line)
    {
        auto skip = [&] { while (pos < s.length() && (s[pos] == ' ' || s[pos] == '\t')) ++pos; };

        auto parseItem = [&] (bool inMap) -> Node
        {
            skip();

            if (pos < s.length() && (s[pos] == '[' || s[pos] == '{'))
                return parseFlow (s, pos, line);

            juce::String text;

            if (pos < s.length() && (s[pos] == '"' || s[pos] == '\''))
            {
                const auto q = s[pos];
                const int start = pos++;

                while (pos < s.length() && s[pos] != q)
                    pos += (q == '"' && s[pos] == '\\') ? 2 : 1;

                if (pos >= s.length())
                    fail (line, "a quote that is not closed");

                text = s.substring (start, ++pos);
            }
            else
            {
                const int start = pos;

                while (pos < s.length() && s[pos] != ',' && s[pos] != ']' && s[pos] != '}'
                       && ! (inMap && s[pos] == ':' && (pos + 1 >= s.length() || s[pos + 1] == ' ')))
                    ++pos;

                text = s.substring (start, pos);
            }

            return makeScalar (text, line);
        };

        const auto open = s[pos++];
        const auto closer = open == '[' ? ']' : '}';
        Node n;
        n.kind = open == '[' ? Node::Kind::list : Node::Kind::map;
        skip();

        while (pos < s.length() && s[pos] != closer)
        {
            if (open == '[')
                n.items.push_back (parseItem (false));
            else
            {
                const auto key = parseItem (true);
                skip();

                if (pos >= s.length() || s[pos] != ':')
                    fail (line, "a map in [] or {} needs a key: value");

                ++pos;
                n.entries.emplace_back (key.scalar, parseItem (false));
            }

            skip();

            if (pos < s.length() && s[pos] == ',')
                ++pos;
            else if (pos < s.length() && s[pos] != closer)
                fail (line, "expected , or " + juce::String::charToString (closer));

            skip();
        }

        if (pos >= s.length())
            fail (line, "a list or map that goes over more than one line is not supported");

        ++pos;
        return n;
    }

    Node parseInline (const juce::String& value, int line)
    {
        const auto t = value.trim();

        if (t.startsWithChar ('[') || t.startsWithChar ('{'))
        {
            int pos = 0;
            auto n = parseFlow (t, pos, line);

            if (t.substring (pos).trim().isNotEmpty())
                fail (line, "unexpected text after the list or map");

            return n;
        }

        return makeScalar (t, line);
    }

    class Parser
    {
    public:
        explicit Parser (std::vector<Line> l) : lines (std::move (l)) {}

        Node parseDocument()
        {
            if (lines.empty())
                return {};

            auto n = parseBlock (lines[0].indent);

            if (index < lines.size())
                fail (lines[index].number, "unexpected indentation");

            return n;
        }

    private:
        std::vector<Line> lines;
        size_t index = 0;

        static bool isListLine (const Line& l) { return l.text == "-" || l.text.startsWith ("- "); }

        Node parseBlock (int indent)
        {
            if (isListLine (lines[index]))
                return parseList (indent);

            if (findKeyColon (lines[index].text) >= 0)
                return parseMap (indent);

            const auto& l = lines[index++];

            if (index < lines.size() && lines[index].indent >= indent)
                fail (lines[index].number, "multi-line scalars are not supported");

            return parseInline (l.text, l.number);
        }

        Node parseList (int indent)
        {
            Node list;
            list.kind = Node::Kind::list;

            while (index < lines.size() && lines[index].indent == indent && isListLine (lines[index]))
            {
                const auto number = lines[index].number;
                const auto rest = lines[index].text.substring (1).trimStart();

                if (rest.isEmpty())
                {
                    ++index;

                    if (index < lines.size() && lines[index].indent > indent)
                        list.items.push_back (parseBlock (lines[index].indent));
                    else
                        list.items.emplace_back();
                }
                else if (! rest.startsWithChar ('[') && ! rest.startsWithChar ('{') && ! rest.startsWithChar ('"')
                         && ! rest.startsWithChar ('\'') && findKeyColon (rest) >= 0)
                {
                    // "- key: value": a map that continues on the lines indented to where `key` starts.
                    const int childIndent = indent + (lines[index].text.length() - rest.length());
                    lines[index].indent = childIndent;
                    lines[index].text = rest;
                    list.items.push_back (parseMap (childIndent));
                }
                else
                {
                    ++index;
                    list.items.push_back (parseInline (rest, number));
                }
            }

            return list;
        }

        Node parseMap (int indent)
        {
            Node map;
            map.kind = Node::Kind::map;

            while (index < lines.size() && lines[index].indent == indent && ! isListLine (lines[index]))
            {
                const auto line = lines[index];
                const int colon = findKeyColon (line.text);

                if (colon < 0)
                    fail (line.number, "expected key: value");

                const auto key = unquote (line.text.substring (0, colon), line.number);
                const auto value = line.text.substring (colon + 1).trim();
                ++index;

                for (auto& e : map.entries)
                    if (e.first == key)
                        fail (line.number, "the key \"" + key + "\" is there twice");

                if (value.isNotEmpty())
                    map.entries.emplace_back (key, parseInline (value, line.number));
                else if (index < lines.size() && lines[index].indent > indent)
                    map.entries.emplace_back (key, parseBlock (lines[index].indent));
                else if (index < lines.size() && lines[index].indent == indent && isListLine (lines[index]))
                    map.entries.emplace_back (key, parseList (indent));   // a list at the key's own indent
                else
                    map.entries.emplace_back (key, Node());
            }

            return map;
        }
    };
}

Result parse (const juce::String& text)
{
    Result result;

    try
    {
        std::vector<Line> lines;
        int number = 0;

        for (auto raw : juce::StringArray::fromLines (text))
        {
            ++number;

            if (number == 1 && raw.startsWithChar ((juce::juce_wchar) 0xfeff))
                raw = raw.substring (1);

            int indent = 0;

            while (indent < raw.length() && raw[indent] == ' ')
                ++indent;

            if (indent < raw.length() && raw[indent] == '\t')
                fail (number, "tabs in the indentation are not supported; use spaces");

            const auto body = stripComment (raw.substring (indent)).trimEnd();

            if (body.isEmpty())
                continue;

            if (body == "---" || body == "...")
            {
                if (body == "---" && lines.empty())
                    continue;

                fail (number, "several documents in one file are not supported");
            }

            if (body.startsWithChar ('%'))
                fail (number, "directives are not supported");

            lines.push_back ({ indent, body, number });
        }

        result.root = Parser (std::move (lines)).parseDocument();
    }
    catch (const Failure& f)
    {
        result.root = {};
        result.error = f.message;
    }

    return result;
}
} // namespace daw::yaml
