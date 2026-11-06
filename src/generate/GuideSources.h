#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace daw::gen::guidesources
{
    /** A web page that was read for a guide, as a link with a note on what it backs (may be empty). */
    struct Page
    {
        juce::String url;    // always starts with https://
        juce::String note;
    };

    /** A search that shaped a fact, with the result links that were used (may be none). */
    struct Search
    {
        juce::String text;
        juce::StringArray resultUrls;
    };

    /** What `SOURCES.md` records for one guide. */
    struct Sources
    {
        bool found = false;               // the file has a heading with this guide's key
        juce::StringArray notes;          // paragraphs under the heading, written for readers
        std::vector<Page> pages;
        std::vector<Search> searches;

        /** True when no page and no search is recorded. */
        bool isEmpty() const noexcept     { return pages.empty() && searches.empty(); }
    };

    /** The text of `SOURCES.md` as it was embedded when DawIt was built. */
    juce::String getEmbeddedText();

    /** Reads the sources of the guide with this key from a Markdown text in the format of `SOURCES.md`: a `## ` heading
        per guide, a `Guide key: <key>` line under it, notes, then `**Pages read**` (`- <https://...> (note)`) and
        `**Searched for**` or `**Searches**` (`- text (results: <https://...>)`). The file's introduction, and headings
        without a key line, belong to no guide. A note that only says "No sources recorded yet." is left out. */
    Sources parse (const juce::String& markdown, const juce::String& key);

    /** Every guide key that has a heading in the text, in order. */
    juce::StringArray getKeys (const juce::String& markdown);

    /** The sources of a guide from the embedded `SOURCES.md`; read once, on the message thread. */
    Sources getSources (const juce::String& key);

    /** The guide key of a built-in effect or synth from its window title ("DawIt Step Filter" gives "stepFilter"), for the
        "What is ...?" guides, which are built with the plugin and know nothing of `TheoryGuides.h`. Empty for any other name. */
    juce::String getKeyForPlugin (const juce::String& pluginName);

    /** The Sources topic's row summary: "12 pages read, 5 searches", "1 page read", "No sources recorded yet". */
    juce::String getSummary (const Sources&);

    /** The text shown first in the Sources topic, and on the first page of its PDF: where the numbers come from, worked
        out, measured or from the sources. A guide with no sources gets the "none recorded yet" paragraph in place of
        the last two paragraphs. */
    juce::String getIntro (bool hasSources);

    /** The whole topic as plain text for the PDF export: the intro, the notes, then every page with its URL written out
        and every search with its result links. */
    juce::String getPrintText (const Sources&);
} // namespace daw::gen::guidesources
