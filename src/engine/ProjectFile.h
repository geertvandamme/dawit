#pragma once

#include "Engine.h"

/**
    DawIt projects (.dawit): indented JSON holding the song settings, the chord track, the lyrics and every
    track with its clips and plugins. Plugin state is stored as base64 strings. Audio clips refer to
    files in an "Audio Files" folder next to the project, by relative path; saving copies audio
    there. MIDI tracks remember the sound of the built-in synth (projects from before it had
    sounds get the piano). Things that belong to the setup rather than the song (audio device, metronome, record
    arm, view settings) are not part of a project.

    Buses are tracks of type "bus" that hold their tracks in a nested "tracks" list, so the file reads like the
    folders it shows; a bus has plugins but no clips, and "folded" is written only when true. A song with a bus is
    saved as version 2, which an older DawIt refuses rather than lose the tracks in it; a song without buses stays
    at version 1 and opens everywhere.

    Returns (tracks that hear the sends of other tracks) are top-level tracks of type "return", with plugins but no
    clips, listed after the other tracks. A track or bus with sends writes
    "sends": [ { "return": 0, "levelDb": -10.0, "preFader": true } ], where "return" counts the returns in file
    order, and "preFader" is written only when true; sends that are off are not saved. A song with a return is
    version 3 (an older DawIt would read a return as a MIDI track and drop the sends); an index with no return is
    skipped with a warning.
*/
namespace daw::project
{
constexpr const char* fileExtension = ".dawit";
constexpr const char* oldFileExtension = ".minidaw";   // from when the app was called MiniDAW

/** Wildcards for file choosers: new and old project files. */
constexpr const char* fileWildcards = "*.dawit;*.minidaw";

/** True for a project file, new or old. */
inline bool isProjectFile (const juce::File& file)
{
    return file.hasFileExtension (fileExtension) || file.hasFileExtension (oldFileExtension);
}

constexpr const char* audioFolderName = "Audio Files";
constexpr int formatVersion = 3;               // the newest this build reads, and what a song with a return is saved as
constexpr int formatVersionWithoutReturns = 2; // what a song with buses but no returns is saved as
constexpr int formatVersionWithoutBuses = 1;   // what a song without buses is saved as, so older versions open it

/** The project as JSON text. Audio paths are written relative to `projectFile` (absolute when it
    is File()). Without plugin state the text is cheap to make, for spotting most edits quickly. */
juce::String toText (const Engine&, const juce::File& projectFile, bool includePluginState = true);

/** Copies audio the project doesn't have yet into its audio folder (pointing the clips at the
    copies), then writes the project. On success `savedText` is what was written. */
bool save (Engine&, const juce::File& projectFile, juce::String& savedText, juce::String& error);

/** Replaces the edit with the project in the file. Problems that don't stop the rest from loading,
    such as a missing plugin or audio file, end up in `warnings`. The caller must forget any
    pointers to tracks and clips first. */
bool load (Engine&, const juce::File& projectFile, juce::String& error, juce::StringArray& warnings);
} // namespace daw::project
