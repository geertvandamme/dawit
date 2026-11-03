#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

namespace daw::presets
{
/** Which plugin a preset is for. A built-in is matched by its identifier alone ("DawIt Reverb"), so the app's
    copy and DawIt's VST3/AU plugins share presets. A hosted plugin is matched by format name plus unique id,
    or, when either id is 0, by name and manufacturer. */
struct Identity
{
    juce::String format;         // "DawIt" for a built-in, otherwise "VST3", "AudioUnit" ...
    juce::String name;           // the built-in's identifier, or the plugin's name
    juce::String manufacturer;
    int uniqueId = 0;
    juce::String version;        // DawIt's version for a built-in, the plugin's own otherwise; not used to match

    bool isBuiltIn() const;
    /** "DawIt Reverb", or "Name (VST3)" for a hosted plugin: used in messages. */
    juce::String getDisplayName() const;
    /** The folder name under Presets/: the identifier, or "<manufacturer> - <name> (<format>)", tidied like a preset name. */
    juce::String getFolderName() const;
};

/** The identity of a built-in, from its identifier. */
Identity builtInIdentity (const juce::String& identifier);
/** The identity of a hosted plugin, from its description. */
Identity hostedIdentity (const juce::PluginDescription&);
/** The identity of a plugin instance: built-in (by its name) when its description says DawIt or is empty, hosted otherwise. */
Identity identityOf (juce::AudioPluginInstance&);
/** True when a preset made for `saved` loads into `target`. */
bool matches (const Identity& saved, const Identity& target);

/** DawIt's version, as saved into preset files. */
juce::String getDawItVersion();

/** One preset: a name, when and by which DawIt it was saved, the plugin it is for and that plugin's state. */
struct Preset
{
    juce::String name;
    juce::String saved;          // ISO 8601
    juce::String dawItVersion;
    Identity plugin;
    juce::MemoryBlock state;
};

constexpr int formatVersion = 1;
constexpr int maxNameLength = 60;

/** Makes a preset of `state` for `plugin`, stamped with now and this DawIt's version. */
Preset makePreset (const juce::String& name, const Identity& plugin, const juce::MemoryBlock& state);

/** The text of a preset file: indented JSON. */
juce::String toJson (const Preset&);

struct ParseResult
{
    bool ok = false;
    Preset preset;
    juce::String error;          // plain words, when not ok
};

/** Reads and checks the text of a preset file. Nothing is trusted: a refusal says why in plain words. */
ParseResult fromJson (const juce::String& text);
ParseResult readFile (const juce::File&);

/** Writes the preset file through a temporary file that replaces the target, so nobody sees half a file. */
bool writeFile (const juce::File&, const Preset&);

/** A name made fit for a file: trimmed, and / \ : * ? " < > | become -. */
juce::String tidyName (const juce::String&);
/** Empty when the name is fine; otherwise why it is not (empty after tidying, or over 60 characters). */
juce::String checkName (const juce::String&);

/** What a library call did. `message` is a full sentence for the user when it failed. */
struct Result
{
    bool ok = false;
    bool nameTaken = false;      // save without replace: a preset with that name is there already
    juce::String message;
    juce::String name;           // the name the preset ended up with
};

/** A preset in the list for one plugin. A file that can't be read is listed with its error. */
struct Entry
{
    juce::String name;
    juce::File file;
    bool readable = false;
    juce::String error;
    juce::String saved;
    juce::String dawItVersion;
    juce::String pluginVersion;
};

/** DawIt's data folder for presets: a Presets sibling of the settings file, or the override. */
juce::File getPresetsFolder();
/** Points every default library at another folder (tests and screenshots); a default File() undoes it. */
void overridePresetsFolder (const juce::File&);

/** The folder of user presets: one subfolder per plugin, one `.dawitpreset` file per preset. It holds no state
    of its own, so any number of Library objects on one folder (the app and the plugins in other hosts) agree. */
class Library
{
public:
    Library();
    explicit Library (const juce::File& rootFolder);

    juce::File getRoot() const { return root; }
    juce::File getFolderFor (const Identity&) const;

    /** Your presets for this plugin, alphabetical (case-insensitive). Files of another plugin are left out. */
    std::vector<Entry> list (const Identity&) const;
    juce::File getFileFor (const Identity&, const juce::String& name) const;
    /** The entry with this name, compared case-insensitively; null when there is none. */
    bool find (const Identity&, const juce::String& name, Entry& found) const;

    /** Saves `state` under `name`. When the name is taken, nothing is written and `nameTaken` is set, unless `replace`. */
    Result save (const Identity&, const juce::String& name, const juce::MemoryBlock& state, bool replace) const;
    /** Saves a ready preset (an imported one, or one with another version, for tests). */
    Result save (const Preset&, bool replace) const;
    Result rename (const Identity&, const juce::String& oldName, const juce::String& newName) const;
    Result remove (const Identity&, const juce::String& name) const;
    /** Reads a library preset by name. */
    ParseResult load (const Identity&, const juce::String& name) const;
    /** Copies a preset file into this plugin's presets; a taken name becomes "Name (2)", "Name (3)". */
    Result importFile (const Identity& target, const juce::File& file) const;
    /** Writes one preset to a file the user picked. A failed write leaves nothing behind. */
    Result exportTo (const Identity&, const juce::String& name, const juce::File& destination) const;
    /** Writes the given settings to a file as a preset, without keeping it in the library. */
    static Result exportState (const Identity&, const juce::String& name, const juce::MemoryBlock& state, const juce::File& destination);

private:
    juce::File root;
};

/** What applying a preset did. */
struct ApplyResult
{
    bool ok = false;
    juce::String message;            // why it was refused, in plain words
    bool versionDiffers = false;     // saved with another version of the plugin
    juce::String savedVersion;
};

/** Loads a preset into a plugin. Everything is decoded and checked first and the plugin is touched only after
    that, so a refusal changes nothing. For a built-in, parameters the preset lacks are set to their defaults
    first (otherwise they would keep whatever they were set to). Message thread; for a hosted plugin the caller
    holds Engine::getLock(). */
ApplyResult apply (const Preset&, juce::AudioPluginInstance& target);
} // namespace daw::presets
