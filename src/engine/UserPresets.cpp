#include "UserPresets.h"

#include <algorithm>

namespace daw::presets
{
namespace
{
    constexpr const char* builtInFormatName = "DawIt";   // BuiltInFormat::formatName, without pulling that file in
    constexpr const char* fileTag = "dawitpreset";
    const juce::String fileExtension = ".dawitpreset";

    juce::File presetsFolderOverride;

    juce::String base64Of (const juce::MemoryBlock& block)
    {
        juce::MemoryOutputStream out;
        juce::Base64::convertToBase64 (out, block.getData(), block.getSize());
        return out.toString();
    }

    bool fromBase64 (const juce::String& text, juce::MemoryBlock& block)
    {
        juce::MemoryOutputStream out (block, false);
        return juce::Base64::convertFromBase64 (out, text);
    }

    Result failure (const juce::String& message)
    {
        Result r;
        r.message = message;
        return r;
    }

    juce::String uniqueName (const Library& library, const Identity& id, const juce::String& wanted)
    {
        Entry found;
        if (! library.find (id, wanted, found))
            return wanted;

        for (int n = 2; n < 10000; ++n)
        {
            const auto candidate = wanted + " (" + juce::String (n) + ")";
            if (! library.find (id, candidate, found))
                return candidate;
        }

        return wanted;
    }

    /** The settings of a built-in are a JUCE value tree as XML: <Tag><PARAM id= value=/>...</Tag>. */
    std::unique_ptr<juce::XmlElement> stateXml (const juce::MemoryBlock& state)
    {
        return juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
    }
} // namespace

//==============================================================================
bool Identity::isBuiltIn() const { return format == builtInFormatName; }

juce::String Identity::getDisplayName() const
{
    return isBuiltIn() ? name : name + " (" + format + ")";
}

juce::String Identity::getFolderName() const
{
    auto folder = isBuiltIn() ? tidyName (name)
                              : tidyName ((manufacturer.isNotEmpty() ? manufacturer + " - " : juce::String()) + name + " (" + format + ")");
    return folder.isEmpty() ? juce::String ("Other") : folder.replaceCharacter ('.', '_').substring (0, 100);
}

Identity builtInIdentity (const juce::String& identifier)
{
    Identity id;
    id.format = builtInFormatName;
    id.name = identifier;
    id.manufacturer = "DawIt";
    id.version = getDawItVersion();
    return id;
}

Identity hostedIdentity (const juce::PluginDescription& d)
{
    if (d.pluginFormatName == builtInFormatName)
        return builtInIdentity (d.name);

    Identity id;
    id.format = d.pluginFormatName;
    id.name = d.name;
    id.manufacturer = d.manufacturerName;
    id.uniqueId = d.uniqueId;
    id.version = d.version;
    return id;
}

Identity identityOf (juce::AudioPluginInstance& instance)
{
    juce::PluginDescription d;
    instance.fillInPluginDescription (d);

    if (d.pluginFormatName.isEmpty() || d.pluginFormatName == builtInFormatName)
        return builtInIdentity (instance.getName());

    return hostedIdentity (d);
}

bool matches (const Identity& saved, const Identity& target)
{
    if (saved.isBuiltIn() || target.isBuiltIn())
        return saved.isBuiltIn() && target.isBuiltIn() && saved.name == target.name;

    if (saved.format != target.format)
        return false;

    if (saved.uniqueId != 0 && target.uniqueId != 0)
        return saved.uniqueId == target.uniqueId;

    return saved.name == target.name && saved.manufacturer == target.manufacturer;
}

juce::String getDawItVersion()
{
#if defined (JUCE_APPLICATION_VERSION_STRING)
    return JUCE_APPLICATION_VERSION_STRING;
#elif defined (JucePlugin_VersionString)
    return JucePlugin_VersionString;
#else
    return "0.1.0";
#endif
}

//==============================================================================
Preset makePreset (const juce::String& name, const Identity& plugin, const juce::MemoryBlock& state)
{
    Preset p;
    p.name = name;
    p.saved = juce::Time::getCurrentTime().toISO8601 (true);
    p.dawItVersion = getDawItVersion();
    p.plugin = plugin;
    p.state = state;
    return p;
}

juce::String toJson (const Preset& p)
{
    auto* plugin = new juce::DynamicObject();
    plugin->setProperty ("format", p.plugin.format);
    plugin->setProperty ("name", p.plugin.name);
    plugin->setProperty ("manufacturer", p.plugin.manufacturer);
    plugin->setProperty ("uniqueId", p.plugin.uniqueId);
    plugin->setProperty ("version", p.plugin.version);

    auto* o = new juce::DynamicObject();
    o->setProperty ("format", fileTag);
    o->setProperty ("version", formatVersion);
    o->setProperty ("name", p.name);
    o->setProperty ("saved", p.saved);
    o->setProperty ("dawitVersion", p.dawItVersion);
    o->setProperty ("plugin", juce::var (plugin));
    o->setProperty ("state", base64Of (p.state));
    return juce::JSON::toString (juce::var (o), false);
}

ParseResult fromJson (const juce::String& text)
{
    ParseResult r;
    const juce::String notAPreset = "This isn't a DawIt preset file.";

    if (text.trim().isEmpty())
    {
        r.error = "The file is empty.";
        return r;
    }

    juce::var v;
    if (juce::JSON::parse (text, v).failed() || ! v.isObject())
    {
        r.error = notAPreset;
        return r;
    }

    if (v["format"].toString() != fileTag)
    {
        r.error = notAPreset;
        return r;
    }

    if (! v["version"].isInt() && ! v["version"].isInt64() && ! v["version"].isDouble())
    {
        r.error = notAPreset;
        return r;
    }

    if ((int) v["version"] > formatVersion)
    {
        r.error = "This preset was made by a newer DawIt. Update DawIt to use it.";
        return r;
    }

    const auto plugin = v["plugin"];
    if (! plugin.isObject() || plugin["name"].toString().isEmpty() || plugin["format"].toString().isEmpty())
    {
        r.error = "The preset doesn't say which plugin it is for.";
        return r;
    }

    const auto stateText = v["state"].toString();
    if (stateText.isEmpty())
    {
        r.error = "The preset has no saved settings.";
        return r;
    }

    juce::MemoryBlock state;
    if (! fromBase64 (stateText, state) || state.getSize() == 0)
    {
        r.error = "The preset's settings are damaged.";
        return r;
    }

    r.preset.name = v["name"].toString();
    r.preset.saved = v["saved"].toString();
    r.preset.dawItVersion = v["dawitVersion"].toString();
    r.preset.plugin.format = plugin["format"].toString();
    r.preset.plugin.name = plugin["name"].toString();
    r.preset.plugin.manufacturer = plugin["manufacturer"].toString();
    r.preset.plugin.uniqueId = (int) plugin["uniqueId"];
    r.preset.plugin.version = plugin["version"].toString();
    r.preset.state = state;
    r.ok = true;
    return r;
}

ParseResult readFile (const juce::File& file)
{
    if (! file.existsAsFile())
    {
        ParseResult r;
        r.error = "The file isn't there.";
        return r;
    }

    return fromJson (file.loadFileAsString());
}

bool writeFile (const juce::File& file, const Preset& p)
{
    if (! file.getParentDirectory().createDirectory().wasOk())
        return false;

    juce::TemporaryFile temp (file);
    return temp.getFile().replaceWithText (toJson (p), false, false, "\n") && temp.overwriteTargetFileWithTemporary();
}

juce::String tidyName (const juce::String& name)
{
    auto s = name.trim();

    for (auto c : juce::String ("/\\:*?\"<>|"))
        s = s.replaceCharacter (c, '-');

    return s.trim();
}

juce::String checkName (const juce::String& name)
{
    const auto tidy = tidyName (name);

    if (tidy.isEmpty())
        return "Give the preset a name.";

    if (tidy.length() > maxNameLength)
        return "Use a name of up to " + juce::String (maxNameLength) + " characters.";

    return {};
}

//==============================================================================
juce::File getPresetsFolder()
{
    if (presetsFolderOverride != juce::File())
        return presetsFolderOverride;

    juce::PropertiesFile::Options options;
    options.applicationName = "DawIt";
    options.folderName = "DawIt";
    options.osxLibrarySubFolder = "Application Support";
    options.filenameSuffix = "settings";

    return options.getDefaultFile().getSiblingFile ("Presets");
}

void overridePresetsFolder (const juce::File& f) { presetsFolderOverride = f; }

//==============================================================================
Library::Library() : root (getPresetsFolder()) {}
Library::Library (const juce::File& rootFolder) : root (rootFolder) {}

juce::File Library::getFolderFor (const Identity& id) const { return root.getChildFile (id.getFolderName()); }

juce::File Library::getFileFor (const Identity& id, const juce::String& name) const
{
    return getFolderFor (id).getChildFile (tidyName (name) + fileExtension);
}

std::vector<Entry> Library::list (const Identity& id) const
{
    std::vector<Entry> entries;

    for (const auto& file : getFolderFor (id).findChildFiles (juce::File::findFiles, false, "*" + fileExtension))
    {
        Entry e;
        e.file = file;
        e.name = file.getFileNameWithoutExtension();
        const auto parsed = readFile (file);
        e.readable = parsed.ok;

        if (! parsed.ok)
            e.error = parsed.error;
        else if (! matches (parsed.preset.plugin, id))
            continue;   // the folder is only for tidiness; a file for another plugin is not ours
        else
        {
            // The name inside the file keeps the case it was saved with, which a case-insensitive disk may not keep in the file's name.
            if (tidyName (parsed.preset.name).equalsIgnoreCase (e.name))
                e.name = tidyName (parsed.preset.name);

            e.saved = parsed.preset.saved;
            e.dawItVersion = parsed.preset.dawItVersion;
            e.pluginVersion = parsed.preset.plugin.version;
        }

        entries.push_back (std::move (e));
    }

    std::sort (entries.begin(), entries.end(), [] (const Entry& a, const Entry& b)
    {
        const int c = a.name.compareIgnoreCase (b.name);
        return c != 0 ? c < 0 : a.name < b.name;
    });
    return entries;
}

bool Library::find (const Identity& id, const juce::String& name, Entry& found) const
{
    const auto wanted = tidyName (name);

    for (auto& e : list (id))
        if (e.name.equalsIgnoreCase (wanted))
        {
            found = e;
            return true;
        }

    return false;
}

Result Library::save (const Identity& id, const juce::String& name, const juce::MemoryBlock& state, bool replace) const
{
    return save (makePreset (tidyName (name), id, state), replace);
}

Result Library::save (const Preset& preset, bool replace) const
{
    const auto bad = checkName (preset.name);
    if (bad.isNotEmpty())
        return failure (bad);

    const auto name = tidyName (preset.name);
    Entry existing;

    if (find (preset.plugin, name, existing) && ! replace)
    {
        Result r;
        r.nameTaken = true;
        r.name = existing.name;
        r.message = "There is already a preset called \"" + existing.name + "\".";
        return r;
    }

    auto toWrite = preset;
    toWrite.name = name;
    const auto file = existing.file != juce::File() && existing.name.equalsIgnoreCase (name) ? existing.file : getFileFor (preset.plugin, name);

    if (! writeFile (file, toWrite))
        return failure ("Couldn't save \"" + name + "\": DawIt can't write to its presets folder.");

    Result r;
    r.ok = true;
    r.name = name;
    return r;
}

Result Library::rename (const Identity& id, const juce::String& oldName, const juce::String& newName) const
{
    const auto bad = checkName (newName);
    if (bad.isNotEmpty())
        return failure (bad);

    Entry from;
    if (! find (id, oldName, from))
        return failure ("There is no preset called \"" + oldName + "\".");

    const auto name = tidyName (newName);
    Entry taken;

    if (find (id, name, taken) && taken.file != from.file)
        return failure ("There is already a preset called \"" + taken.name + "\".");

    auto parsed = readFile (from.file);
    if (! parsed.ok)
        return failure ("Couldn't rename \"" + from.name + "\": " + parsed.error);

    parsed.preset.name = name;
    const auto target = getFileFor (id, name);

    if (! writeFile (target, parsed.preset))
        return failure ("Couldn't rename \"" + from.name + "\": DawIt can't write to its presets folder.");

    if (target != from.file)
        from.file.deleteFile();

    Result r;
    r.ok = true;
    r.name = name;
    return r;
}

Result Library::remove (const Identity& id, const juce::String& name) const
{
    Entry e;
    if (! find (id, name, e))
        return failure ("There is no preset called \"" + name + "\".");

    if (! e.file.deleteFile())
        return failure ("Couldn't delete \"" + e.name + "\": DawIt can't change its presets folder.");

    Result r;
    r.ok = true;
    r.name = e.name;
    return r;
}

ParseResult Library::load (const Identity& id, const juce::String& name) const
{
    Entry e;
    if (! find (id, name, e))
    {
        ParseResult r;
        r.error = "There is no preset called \"" + name + "\".";
        return r;
    }

    auto r = readFile (e.file);
    if (r.ok)
        r.preset.name = e.name;
    return r;
}

Result Library::importFile (const Identity& target, const juce::File& file) const
{
    const auto parsed = readFile (file);
    const auto shown = file.getFileNameWithoutExtension();

    if (! parsed.ok)
        return failure ("Couldn't import \"" + shown + "\": " + parsed.error);

    if (! matches (parsed.preset.plugin, target))
        return failure ("\"" + parsed.preset.name + "\" is a preset for " + parsed.preset.plugin.getDisplayName() + ", not " + target.getDisplayName() + ".");

    auto preset = parsed.preset;
    auto wanted = tidyName (preset.name.isNotEmpty() ? preset.name : shown);

    if (checkName (wanted).isNotEmpty())
        wanted = tidyName (shown);

    if (checkName (wanted).isNotEmpty())
        wanted = "Imported preset";

    preset.name = uniqueName (*this, target, wanted);
    return save (preset, false);
}

Result Library::exportTo (const Identity& id, const juce::String& name, const juce::File& destination) const
{
    const auto loaded = load (id, name);
    if (! loaded.ok)
        return failure ("Couldn't export \"" + name + "\": " + loaded.error);

    if (! writeFile (destination, loaded.preset))
        return failure ("Couldn't export \"" + name + "\": DawIt can't write to that place.");

    Result r;
    r.ok = true;
    r.name = loaded.preset.name;
    return r;
}

Result Library::exportState (const Identity& id, const juce::String& name, const juce::MemoryBlock& state, const juce::File& destination)
{
    if (! writeFile (destination, makePreset (tidyName (name), id, state)))
        return failure ("Couldn't export \"" + name + "\": DawIt can't write to that place.");

    Result r;
    r.ok = true;
    r.name = name;
    return r;
}

//==============================================================================
ApplyResult apply (const Preset& preset, juce::AudioPluginInstance& target)
{
    ApplyResult r;
    const auto targetIdentity = identityOf (target);

    if (! matches (preset.plugin, targetIdentity))
    {
        r.message = "\"" + preset.name + "\" is a preset for " + preset.plugin.getDisplayName() + ", not " + targetIdentity.getDisplayName() + ".";
        return r;
    }

    if (preset.state.getSize() == 0)
    {
        r.message = "The preset has no saved settings.";
        return r;
    }

    juce::MemoryBlock toApply = preset.state;

    if (targetIdentity.isBuiltIn())
    {
        // Check the settings are this built-in's own, and fill in what the preset lacks with defaults.
        auto xml = stateXml (preset.state);
        juce::MemoryBlock current;
        target.getStateInformation (current);
        auto currentXml = stateXml (current);

        if (xml == nullptr || currentXml == nullptr || ! xml->hasTagName (currentXml->getTagName()))
        {
            r.message = "The settings in \"" + preset.name + "\" are damaged.";
            return r;
        }

        if (currentXml->getChildByName ("PARAM") != nullptr)
        {
            for (auto* parameter : target.getParameters())
            {
                auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter);
                if (ranged == nullptr)
                    continue;

                bool present = false;
                for (auto* child : xml->getChildWithTagNameIterator ("PARAM"))
                    present = present || child->getStringAttribute ("id") == ranged->paramID;

                if (! present)
                {
                    auto* added = xml->createNewChildElement ("PARAM");
                    added->setAttribute ("id", ranged->paramID);
                    added->setAttribute ("value", (double) ranged->convertFrom0to1 (ranged->getDefaultValue()));
                }
            }

            toApply.reset();
            juce::AudioProcessor::copyXmlToBinary (*xml, toApply);
        }
    }

    target.setStateInformation (toApply.getData(), (int) toApply.getSize());

    r.ok = true;
    r.savedVersion = preset.plugin.version;
    r.versionDiffers = preset.plugin.version.isNotEmpty() && preset.plugin.version != targetIdentity.version;
    return r;
}
} // namespace daw::presets
