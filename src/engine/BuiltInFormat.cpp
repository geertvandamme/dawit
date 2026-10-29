#include "BuiltInFormat.h"
#include "BuiltInAdditive.h"
#include "BuiltInCompressor.h"
#include "BuiltInDelay.h"
#include "BuiltInDistortion.h"
#include "BuiltInDrums.h"
#include "BuiltInEq.h"
#include "BuiltInFm.h"
#include "BuiltInGate.h"
#include "BuiltInStereo.h"
#include "BuiltInTune.h"
#include "BuiltInPhysical.h"
#include "BuiltInReverb.h"
#include "BuiltInSaturation.h"
#include "BuiltInSubtractive.h"

namespace daw
{
juce::Array<juce::PluginDescription> BuiltInFormat::getEffects()
{
    return { BuiltInEq::getDescription(), BuiltInCompressor::getDescription(), BuiltInGate::getDescription(), BuiltInStereo::getDescription(), BuiltInTune::getDescription(), BuiltInReverb::getDescription(), BuiltInDelay::getDescription(),
             BuiltInSaturation::getDescription(), BuiltInDistortion::getDescription() };
}

juce::Array<juce::PluginDescription> BuiltInFormat::getInstruments()
{
    return { BuiltInSubtractive::getDescription(), BuiltInAdditive::getDescription(), BuiltInFm::getDescription(), BuiltInDrums::getDescription(),
             BuiltInPhysical::getDescription() };
}

void BuiltInFormat::findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& results, const juce::String& fileOrIdentifier)
{
    auto all = getEffects();
    all.addArray (getInstruments());

    for (auto& d : all)
        if (d.fileOrIdentifier == fileOrIdentifier)
            results.add (new juce::PluginDescription (d));
}

bool BuiltInFormat::fileMightContainThisPluginType (const juce::String& fileOrIdentifier)
{
    auto all = getEffects();
    all.addArray (getInstruments());
    return std::any_of (all.begin(), all.end(), [&] (auto& d) { return d.fileOrIdentifier == fileOrIdentifier; });
}

void BuiltInFormat::createPluginInstance (const juce::PluginDescription& description, double, int, PluginCreationCallback callback)
{
    if (description.fileOrIdentifier == BuiltInEq::identifier)
        callback (std::make_unique<BuiltInEq>(), {});
    else if (description.fileOrIdentifier == BuiltInCompressor::identifier)
        callback (std::make_unique<BuiltInCompressor>(), {});
    else if (description.fileOrIdentifier == BuiltInStereo::identifier)
        callback (std::make_unique<BuiltInStereo>(), {});
    else if (description.fileOrIdentifier == BuiltInTune::identifier)
        callback (std::make_unique<BuiltInTune>(), {});
    else if (description.fileOrIdentifier == BuiltInGate::identifier)
        callback (std::make_unique<BuiltInGate>(), {});
    else if (description.fileOrIdentifier == BuiltInReverb::identifier)
        callback (std::make_unique<BuiltInReverb>(), {});
    else if (description.fileOrIdentifier == BuiltInDelay::identifier)
        callback (std::make_unique<BuiltInDelay>(), {});
    else if (description.fileOrIdentifier == BuiltInSaturation::identifier)
        callback (std::make_unique<BuiltInSaturation>(), {});
    else if (description.fileOrIdentifier == BuiltInDistortion::identifier)
        callback (std::make_unique<BuiltInDistortion>(), {});
    else if (description.fileOrIdentifier == BuiltInSubtractive::identifier)
        callback (std::make_unique<BuiltInSubtractive>(), {});
    else if (description.fileOrIdentifier == BuiltInAdditive::identifier)
        callback (std::make_unique<BuiltInAdditive>(), {});
    else if (description.fileOrIdentifier == BuiltInFm::identifier)
        callback (std::make_unique<BuiltInFm>(), {});
    else if (description.fileOrIdentifier == BuiltInDrums::identifier)
        callback (std::make_unique<BuiltInDrums>(), {});
    else if (description.fileOrIdentifier == BuiltInPhysical::identifier)
        callback (std::make_unique<BuiltInPhysical>(), {});
    else
        callback (nullptr, "DawIt has no built-in effect called " + description.name);
}
} // namespace daw
