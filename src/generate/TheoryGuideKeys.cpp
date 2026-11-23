#include "TheoryGuides.h"

// The guides' keys, the Sources topic name and the path for beginners, apart from the guides' pages: the built-in plugins link these (for the
// guide footer) without every guide's text.
namespace daw::gen::guides
{
const juce::String sourcesTopicName { "Sources" };

juce::String getKey (Guide guide)
{
    using G = Guide;

    switch (guide)
    {
        case G::readMusic:    return "readMusic";
        case G::keyFinding:   return "keyFinding";
        case G::sound:        return "sound";
        case G::midi:         return "midi";
        case G::rhythm:       return "rhythm";
        case G::chords:       return "chords";
        case G::chordShapes:  return "chordShapes";
        case G::structure:    return "structure";
        case G::styles:       return "styles";
        case G::lyrics:       return "lyrics";
        case G::singing:      return "singing";
        case G::tune:         return "tune";
        case G::harmoniser:   return "harmoniser";
        case G::gainStaging:  return "gainStaging";
        case G::eq:           return "eq";
        case G::compressor:   return "compressor";
        case G::gate:         return "gate";
        case G::stereo:       return "stereo";
        case G::routing:      return "routing";
        case G::loudness:     return "loudness";
        case G::reverb:       return "reverb";
        case G::delay:        return "delay";
        case G::slicer:       return "slicer";
        case G::arpeggiator:  return "arpeggiator";
        case G::sampler:      return "sampler";
        case G::handover:     return "handover";
        case G::instruments:  return "instruments";
        case G::vocoder:      return "vocoder";
        case G::stepFilter:   return "stepFilter";
        case G::spectralFreeze: return "spectralFreeze";
        case G::saturation:   return "saturation";
        case G::distortion:   return "distortion";
        case G::subtractive:  return "subtractive";
        case G::additive:     return "additive";
        case G::fm:           return "fm";
        case G::drums:        return "drums";
        case G::physical:     return "physical";
        case G::recording:    return "recording";
        case G::tuning:       return "tuning";
        case G::triggers:     return "triggers";
    }

    return {};
}

std::optional<Guide> findGuide (const juce::String& key)
{
    for (auto g : allGuides)
        if (getKey (g).equalsIgnoreCase (key))
            return g;

    return std::nullopt;
}

std::vector<Guide> getStartPath()
{
    return { Guide::sound, Guide::rhythm, Guide::chords, Guide::structure, Guide::styles, Guide::gainStaging, Guide::eq, Guide::compressor };
}
} // namespace daw::gen::guides
