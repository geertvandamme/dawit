#pragma once

#include <juce_core/juce_core.h>

/** The notebook: free text the user writes at three levels (App, Project, Track). The project and track notes live
    on Engine and Track and are saved in the project; this file holds the App notebook, kept in a plain UTF-8 file
    next to the settings file, and the small pure helpers all three share. */
namespace daw::notebook
{
/** True when the text holds any character that isn't whitespace. Blank notes are not saved. */
bool hasNotes (const juce::String&);

/** The first non-blank line, trimmed and cut to 80 characters with "..." — for the note icon's tooltip. */
juce::String getFirstLine (const juce::String&);

/** Line endings as the notebook keeps them: CRLF and lone CR become LF. */
juce::String normaliseLineEndings (const juce::String&);

/** `Notebook.txt` next to the settings file (or inside the override folder). */
juce::File getNotebookFile();

/** Points the App notebook at another folder, for tests and the screenshot runner. An empty File restores the default. */
void overrideNotebookFolder (const juce::File&);

/** The App notebook's text; a missing or unreadable file is an empty notebook. */
juce::String loadAppNotebook();

/** Writes the App notebook through a temporary file, then renames it over the old one. Returns false on failure. */
bool saveAppNotebook (const juce::String& text);
}
