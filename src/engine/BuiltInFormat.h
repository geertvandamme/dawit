#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace daw
{
/** The format of DawIt's own effects (DawIt EQ, Compressor, Gate, Stereo, Tune, Reverb, Delay, Saturation and Distortion) and instruments (DawIt
    Subtractive, Additive, FM, Drums and Physical), so the plugin chain, saving and loading treat them like any plugin. They need no scanning: getEffects() and
    getInstruments() list them for the add menus. */
class BuiltInFormat final : public juce::AudioPluginFormat
{
public:
    static constexpr const char* formatName = "DawIt";

    static juce::Array<juce::PluginDescription> getEffects();
    static juce::Array<juce::PluginDescription> getInstruments();

    juce::String getName() const override { return formatName; }
    void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>&, const juce::String& fileOrIdentifier) override;
    bool fileMightContainThisPluginType (const juce::String& fileOrIdentifier) override;
    juce::String getNameOfPluginFromIdentifier (const juce::String& fileOrIdentifier) override { return fileOrIdentifier; }
    bool pluginNeedsRescanning (const juce::PluginDescription&) override { return false; }
    bool doesPluginStillExist (const juce::PluginDescription& d) override { return fileMightContainThisPluginType (d.fileOrIdentifier); }
    bool canScanForPlugins() const override { return false; }
    bool isTrivialToScan() const override { return true; }
    juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&, bool, bool) override { return {}; }
    juce::FileSearchPath getDefaultLocationsToSearch() override { return {}; }
    bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const override { return false; }

private:
    void createPluginInstance (const juce::PluginDescription&, double, int, PluginCreationCallback) override;
};
} // namespace daw
