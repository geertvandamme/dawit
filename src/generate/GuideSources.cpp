#include "GuideSources.h"
#include "DawItSources.h"

namespace daw::gen::guidesources
{
    namespace
    {
        const char* const noneRecorded = "No sources recorded yet.";

        enum class Part { notes, pages, searches, other };

        /** The https:// links written as <...> in a line. */
        juce::StringArray getLinks (const juce::String& line)
        {
            juce::StringArray links;
            int from = 0;

            for (;;)
            {
                const auto open = line.indexOfChar (from, '<');

                if (open < 0)
                    break;

                const auto close = line.indexOfChar (open, '>');

                if (close < 0)
                    break;

                const auto link = line.substring (open + 1, close);

                if (link.startsWith ("http"))
                    links.add (link);

                from = close + 1;
            }

            return links;
        }

        /** Splits the text into the sections that start at a "## " heading: the heading's line, then its lines. */
        std::vector<juce::StringArray> getSections (const juce::String& markdown)
        {
            std::vector<juce::StringArray> sections;

            for (auto line : juce::StringArray::fromLines (markdown))
            {
                line = line.trimEnd();

                if (line.startsWith ("## "))
                    sections.emplace_back();

                if (! sections.empty())
                    sections.back().add (line);
            }

            return sections;
        }

        /** The key of a section, or empty. */
        juce::String getKey (const juce::StringArray& section)
        {
            for (int i = 1; i < section.size() && i < 4; ++i)
                if (section[i].startsWith ("Guide key:"))
                    return section[i].fromFirstOccurrenceOf (":", false, false).trim().removeCharacters ("`");

            return {};
        }
    } // namespace

    juce::String getEmbeddedText()
    {
        return juce::String::fromUTF8 (dawsources::SOURCES_md, dawsources::SOURCES_mdSize);
    }

    juce::StringArray getKeys (const juce::String& markdown)
    {
        juce::StringArray keys;

        for (auto& section : getSections (markdown))
            if (const auto key = getKey (section); key.isNotEmpty())
                keys.add (key);

        return keys;
    }

    Sources parse (const juce::String& markdown, const juce::String& key)
    {
        Sources sources;

        for (auto& section : getSections (markdown))
        {
            if (getKey (section) != key)
                continue;

            sources.found = true;
            auto part = Part::notes;
            juce::String paragraph;

            const auto endParagraph = [&]
            {
                if (paragraph.isNotEmpty() && paragraph != noneRecorded)
                    sources.notes.add (paragraph);

                paragraph.clear();
            };

            for (int i = 1; i < section.size(); ++i)
            {
                const auto& line = section[i];
                const auto trimmed = line.trim();

                if (trimmed.startsWith ("Guide key:"))
                    continue;

                if (trimmed.startsWith ("**"))
                {
                    endParagraph();
                    part = trimmed.startsWith ("**Pages read") ? Part::pages
                         : trimmed.startsWith ("**Search")     ? Part::searches
                                                               : Part::other;
                    continue;
                }

                if (part == Part::notes)
                {
                    if (trimmed.isEmpty())
                        endParagraph();
                    else
                        paragraph += (paragraph.isEmpty() ? "" : " ") + trimmed;
                }
                else if (trimmed.startsWith ("- "))
                {
                    const auto item = trimmed.substring (2).trim();

                    if (part == Part::pages)
                    {
                        const auto links = getLinks (item);

                        if (item.startsWithChar ('<') && links.size() > 0)
                        {
                            auto note = item.fromFirstOccurrenceOf (">", false, false).trim();

                            if (note.startsWithChar ('(') && note.endsWithChar (')'))
                                note = note.substring (1, note.length() - 1).trim();

                            sources.pages.push_back ({ links[0], note });
                        }
                    }
                    else if (part == Part::searches)
                    {
                        const auto cut = item.indexOf (" (results:");
                        Search search;
                        search.text = (cut >= 0 ? item.substring (0, cut) : item).trim();

                        if (cut >= 0)
                            search.resultUrls = getLinks (item.substring (cut));

                        sources.searches.push_back (search);
                    }
                }
            }

            endParagraph();
            break;
        }

        return sources;
    }

    Sources getSources (const juce::String& key)
    {
        return parse (getEmbeddedText(), key);
    }

    juce::String getKeyForPlugin (const juce::String& pluginName)
    {
        static const std::pair<const char*, const char*> table[] = {
            { "DawIt Tune", "tune" }, { "DawIt Harmoniser", "harmoniser" }, { "DawIt EQ", "eq" }, { "DawIt Compressor", "compressor" },
            { "DawIt Gate", "gate" }, { "DawIt Stereo", "stereo" }, { "DawIt Reverb", "reverb" }, { "DawIt Delay", "delay" },
            { "DawIt Arpeggiator", "arpeggiator" }, { "DawIt Slicer", "slicer" }, { "DawIt Step Filter", "stepFilter" },
            { "DawIt Spectral Freeze", "spectralFreeze" }, { "DawIt Saturation", "saturation" }, { "DawIt Distortion", "distortion" },
            { "DawIt Subtractive", "subtractive" }, { "DawIt Additive", "additive" }, { "DawIt FM", "fm" },
            { "DawIt Drums", "drums" }, { "DawIt Physical", "physical" } };

        for (const auto& [name, key] : table)
            if (pluginName == name)
                return key;

        return {};
    }

    juce::String getSummary (const Sources& sources)
    {
        juce::StringArray parts;
        const auto pages = (int) sources.pages.size(), searches = (int) sources.searches.size();

        if (pages > 0)
            parts.add (juce::String (pages) + (pages == 1 ? " page read" : " pages read"));

        if (searches > 0)
            parts.add (juce::String (searches) + (searches == 1 ? " search" : " searches"));

        return parts.isEmpty() ? juce::String ("No sources recorded yet") : parts.joinIntoString (", ");
    }

    juce::String getIntro (bool hasSources)
    {
        juce::String text = juce::String::fromUTF8 (
            "Where the numbers in this guide come from.\n\n"
            "Worked out: some numbers follow from a formula. Doubling the power of a sound adds 3 dB, because 10 \xC3\x97 log10 2 = 3.01. "
            "DawIt's own tests check many of these.\n\n"
            "Measured: what the pictures show about your song or an effect's sound is measured by DawIt as you watch. "
            "Its tests check those measurements on known test sounds.\n\n");

        if (hasSources)
            text += "From the sources below: definitions, standards, history and typical values. A typical value is a usual range, not a rule: "
                    "engineers and musicians often work outside it.\n\n"
                    "The pages were read when the guide was written or checked. The searches are what was looked up when no single page was used. "
                    "A page can move or change after it was read. Click a link to open it in your browser, or right-click it to copy it.";
        else
            text += "No sources have been recorded for this guide yet. It was written before DawIt began keeping a list. "
                    "They will be added here when the guide is next checked.";

        return text;
    }

    juce::String getPrintText (const Sources& sources)
    {
        juce::String text = getIntro (! sources.isEmpty());

        for (auto& note : sources.notes)
            text += "\n\n" + note;

        if (! sources.pages.empty())
        {
            text += "\n\nPages read\n";

            for (auto& page : sources.pages)
                text += "\n" + page.url + (page.note.isEmpty() ? juce::String() : " (" + page.note + ")");
        }

        if (! sources.searches.empty())
        {
            text += "\n\nSearched for\n";

            for (auto& search : sources.searches)
            {
                text += "\n" + search.text;

                for (auto& url : search.resultUrls)
                    text += "\n    " + url;
            }
        }

        return text;
    }
} // namespace daw::gen::guidesources
