#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace daw::plugins
{
/** True if the description is one of DawIt's own effects or synths built as a VST3 or AU plugin (maker "DawIt").
    The add-plugin menus hide these: the built-in with the same name is used instead of listing it twice.
    The built-ins themselves (format "DawIt") and other makers' plugins are not copies. */
inline bool isDawItPluginCopy (const juce::PluginDescription& d)
{
    const bool isPluginFormat = d.pluginFormatName == "VST3" || d.pluginFormatName == "AudioUnit";
    return isPluginFormat && d.manufacturerName.trim().equalsIgnoreCase ("DawIt");
}
} // namespace daw::plugins
