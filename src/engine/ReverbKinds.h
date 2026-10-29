#pragma once

#include "BuiltInReverb.h"

/** The kinds of reverb: their beginner texts, listening examples and advice for a named track. */
namespace daw::verb::kinds
{
    enum class Topic { kinds, room, hall, chamber, plate, spring, digital, effects, yourTrack };
    enum class Source { unknown, vocal, snare, kick, drums, bass, guitar, keys, ensemble };

    /** The short name shown by Type and above each listening example. */
    juce::String getName (Type);
    /** What distinguishes this kind, in one line. */
    juce::String getSummary (Type);
    /** The planned guide text for one kind. Digital includes convolution. */
    juce::String explain (Type);
    /** The planned text for a guide topic. */
    juce::String explain (Topic);
    /** The existing starting point used to draw and audition this kind. */
    const Preset& getShowcase (Type);
    /** The Type explainer's general text. */
    juce::String getTypeDescription();
    /** Common choices of type, described as habits rather than rules. */
    juce::String getTypeTypical();
    /** What the selected kind does with these settings. */
    juce::String describeSettings (const Settings&);
    /** Recognises source words in the track name, including numbered names such as BV 1. */
    Source getSource (const juce::String&);
    /** Advice for the named host track; an empty name means a stand-in reverb. */
    juce::StringArray analyse (const juce::String& name, bool isReturn, const Settings&, double tempoBpm);
    /** Reflection arrivals per second in a room of this volume, after this elapsed time. */
    double reflectionRate (double volumeM3, double timeSeconds);
    /** Milliseconds until the room reaches the given reflection rate. */
    double densityTimeMs (double volumeM3, double echoesPerSecond = 1000.0);
    /** Bending-wave group velocity in a 0.5 mm steel sheet, in metres per second. */
    double plateGroupVelocity (double frequencyHz);
    /** Milliseconds for one trip along the spring and back, from Size in percent. */
    double springRoundTripMs (float sizePercent);
}
