#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace daw::about
{
    /** The groups of the Licences window, in the order they are listed. */
    enum class Group
    {
        dawit,          // DawIt's own licence and where the texts come from
        builtIn,        // everything compiled into the app
        downloaded,     // fetched after asking the user; only the notice is kept
        installer,      // used to make the Windows installer
        separate,       // programs DawIt talks to but doesn't include
        other           // a notice file that has no entry in the name table
    };

    /** The heading of a group in the Licences list. */
    juce::String getGroupName (Group group);

    /** One licence or notice the Licences window can show. */
    struct Notice
    {
        juce::String name;          // readable name shown in the list
        Group group = Group::other;
        juce::String fileName;      // base name of the embedded file; empty for a fixed text
        juce::String text;          // the full text, as its makers wrote it
    };

    /** The readable name and group of an embedded notice file, or false when the name table has no entry for it. */
    bool findNoticeEntry (const juce::String& fileName, juce::String& name, Group& group);

    /** Every notice compiled into the app (LICENSE and every file under licenses/) in listing order: DawIt first,
        then each group with its entries alphabetical by name. A file missing from the name table still appears,
        under Group::other with its file name. The Ollama entry has no file. */
    std::vector<Notice> getNotices();

    /** The line above the text of a notice: its name and group, and for a download the note that it isn't in the app. */
    juce::String getNoticeHeader (const Notice& notice);

    /** "Version 0.1.4 · commit 1a2b3c4" (the commit is cut to 7 characters), or "... · commit unknown" when empty. */
    juce::String getVersionLine (const juce::String& version, const juce::String& commit);

    /** "DawIt 0.1.4 (commit 1a2b3c4)" for a bug report; "commit unknown" when the commit is empty. */
    juce::String getCopyLine (const juce::String& version, const juce::String& commit);

    /** Where the "Source code" button goes and what the About window says about it. */
    struct SourceInfo
    {
        juce::String url;       // the page to open
        juce::String line;      // "Source code of this version: github.com/..., tag v0.1.4"
        juce::String note;      // under the line when the build has local changes or no commit; otherwise empty
    };

    /** The source link: a clean build at the tag v<version> links to the tag, a clean build at another commit to that
        commit, a build with local changes or without a commit to the tag with a note. A non-empty override replaces
        the link in every case (and the note, since it may point to something other than the tag). */
    SourceInfo getSourceInfo (const juce::String& version, const juce::String& commit, bool hasLocalChanges,
                              bool isExactTag, const juce::String& urlOverride);

    /** The licence paragraph of the About window. */
    juce::String getLicenceText();

    /** The paragraph about what the licence lets the user do. */
    juce::String getRightsText();

    /** The line about third-party parts and Ollama. */
    juce::String getThirdPartyText();
}
