#pragma once

#include "GenerationRequest.h"

namespace daw::gen
{
/** Drum names the model may use, in General MIDI order of importance. */
juce::StringArray getDrumNames();

/** General MIDI note for a drum name from getDrumNames(), or -1. */
int getDrumNote (const juce::String& name);

/** Hand percussion instruments the model may use for a percussion part. */
juce::StringArray getPercussionNames();

/** General MIDI note for a name from getPercussionNames(), or -1. */
int getPercussionNote (const juce::String& name);

/** Chord colours the model may use: how the in-key chord on a degree is extended or altered. */
juce::StringArray getChordColours();

/** `count` ideas for the part picked at random, each from a different kind (density, contour, rhythm...), such as
    "sparse: few notes and plenty of rests". They go in Request::directions to steer a take away from the obvious. */
juce::StringArray pickDirections (const Request&, int count, juce::Random&);

juce::String buildSystemPrompt();
juce::String buildUserPrompt (const Request&);

/** JSON schema the model's reply must follow (used for constrained decoding). */
juce::var buildSchema (const Request&);
} // namespace daw::gen
