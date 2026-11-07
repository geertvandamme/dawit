#include "Notebook.h"

#include <juce_data_structures/juce_data_structures.h>

namespace daw::notebook
{
namespace
{
juce::File& folderOverride()
{
    static juce::File f;
    return f;
}
}

bool hasNotes (const juce::String& text) { return text.trim().isNotEmpty(); }

juce::String getFirstLine (const juce::String& text)
{
    for (auto line : juce::StringArray::fromLines (normaliseLineEndings (text)))
    {
        line = line.trim();

        if (line.isEmpty())
            continue;

        return line.length() > 80 ? line.substring (0, 80) + "..." : line;
    }

    return {};
}

juce::String normaliseLineEndings (const juce::String& text)
{
    return text.replace ("\r\n", "\n").replace ("\r", "\n");
}

juce::File getNotebookFile()
{
    if (folderOverride() != juce::File())
        return folderOverride().getChildFile ("Notebook.txt");

    juce::PropertiesFile::Options options;
    options.applicationName = "DawIt";
    options.folderName = "DawIt";
    options.osxLibrarySubFolder = "Application Support";
    options.filenameSuffix = "settings";

    return options.getDefaultFile().getSiblingFile ("Notebook.txt");
}

void overrideNotebookFolder (const juce::File& f) { folderOverride() = f; }

juce::String loadAppNotebook()
{
    const auto file = getNotebookFile();
    return file.existsAsFile() ? normaliseLineEndings (file.loadFileAsString()) : juce::String();
}

bool saveAppNotebook (const juce::String& text)
{
    const auto file = getNotebookFile();
    file.getParentDirectory().createDirectory();

    juce::TemporaryFile temp (file);
    return temp.getFile().replaceWithText (text, false, false, "\n") && temp.overwriteTargetFileWithTemporary();
}
}
