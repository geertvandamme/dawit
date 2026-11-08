#pragma once

#include "engine/BuiltInSampler.h"
#include "engine/SamplerGuide.h"
#include "EffectWindow.h"

/**
    The pages of "What is a sampler?": the text from `daw::samplerguide` with live pictures that follow the sampler's
    SoundFont, its sound and the last note played. With no SoundFont ready (a stand-in sampler, a missing file) the
    pictures draw a made-up example and say so. "Your track" looks at the notes of the track that plays the sampler.
*/
namespace daw::samplertext
{
/** The notes of the track that holds a processor, as the app finds them (the key and velocity as written in the clips,
    before the sampler's Transpose). `found` is false for a stand-in sampler that is on no track. */
struct TrackNotes
{
    bool found = false;
    juce::String trackName;
    std::vector<samplerguide::Note> notes;
};

/** Installs the app's lookup; it reads the edit under its lock on the message thread. */
void setTrackLookup (std::function<TrackNotes (const juce::AudioProcessor&)>);

/** What "Your track" looks at for this sampler: its font and sound, and the track's notes after Transpose. */
samplerguide::TrackView getTrackView (const BuiltInSampler&, const std::vector<samplerguide::Note>& notes);

/** The pages of "What is a sampler?", with pictures that follow the sampler's SoundFont and the last note played. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInSampler&);
} // namespace daw::samplertext
