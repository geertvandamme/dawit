#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

namespace daw::analysis::onsets
{
/** Where notes and syllables begin in mono audio, in seconds: the moments the sound's spectrum
    rises sharply (spectral flux of 32 ms windows every 10 ms, between 100 Hz and 5 kHz).
    Rises in the quiet parts (more than 40 dB under the loudest) are ignored, and two onsets are
    at least 60 ms apart. Meant for one voice or instrument at a time, such as a vocal stem. */
std::vector<double> findOnsets (const juce::AudioBuffer<float>& mono, double sampleRate);
} // namespace daw::analysis::onsets
