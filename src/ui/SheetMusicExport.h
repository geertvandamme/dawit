#pragma once

#include "engine/Engine.h"
#include "generate/ReadingGuide.h"

namespace daw::sheetmusic
{
/** A track's part written out as sheet music in a PDF: portrait pages with the song's title, the part, the tempo and
    the key at the top of the first, lines of music justified to the page width (never splitting a bar, each starting
    with the clef, the key signature and the bar number), a final bar line, and a page number with "Made with DawIt"
    at the foot of every page. Black on white, drawn through the same `scorepaint` as the score view, at 300 dpi. */
enum class Paper { a4, letter };

/** What the first page says above the music. */
struct Header
{
    juce::String title;     // the song ("Untitled" when it has no name)
    juce::String part;      // the track's name
    juce::String keyName;   // "A minor"
    int bpm = 120;
};

/** The part of a MIDI track as `gen::notation` and the guide read it: the notes of all its clips (beats from bar 0),
    the song's key, chords, sections and tempo, and the track's clef. Takes the engine's lock while it copies. */
gen::readmusic::Part getPart (Engine&, Track&, double playheadBeat);

/** The pages of the sheet, `pixelsPerPoint` pixels per point (300 dpi by default). Empty when the part has no notes. */
std::vector<juce::Image> renderPages (const gen::readmusic::Part&, const Header&, Paper, float pixelsPerPoint = 300.0f / 72.0f);

/** Writes the sheet for a part into a PDF. Fails with a message when there are no notes. */
juce::Result exportPdf (const gen::readmusic::Part&, const Header&, Paper, const juce::File&);

/** "Song - Track": the file's name without its extension. */
juce::String getFileName (const juce::String& title, const juce::String& part);

/** The paper chosen last time (`export.sheetPaper`). */
Paper getSavedPaper (Engine&);

/** File > Export Sheet Music (PDF)... and the score's PDF button: asks for the paper, then where to save the selected
    MIDI track's part; the dialog also has a button that calls `onShowGuide`. Says so when there is no track or it has no notes. */
void showDialog (Engine&, Track*, const juce::String& projectName, std::function<void()> onShowGuide = {});
} // namespace daw::sheetmusic
