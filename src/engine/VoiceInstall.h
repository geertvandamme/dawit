#pragma once

#include "Voice.h"

#include <functional>

namespace daw::sing
{
/** Reports progress from 0 to 1; returning false cancels. */
using InstallProgressFn = std::function<bool (double)>;

struct InstallResult
{
    juce::File installed;      // the voice's folder, or the vocoder's, once it is in place
    juce::String error;       // what went wrong, naming the file; empty on success
    juce::String notice;      // something to tell the user although it worked ("the vocoder is still missing")
    bool wasVocoder = false;  // a vocoder was added rather than a voice
    bool cancelled = false;
};

/** Adds a voice to the voices folder from a .zip, a folder, or a .oudep vocoder pack (a zip with a vocoder.yaml in it).
    A voice is unpacked, found inside whatever folders it was packed in, and checked with loadVoice: a voice that
    isn't a DiffSinger voice, lacks a file or has no English is removed again and the error says why. A voice that
    only lacks its vocoder is kept, with a notice. Blocks; call it from a background thread. */
InstallResult installVoice (const juce::File& source, const juce::File& voicesFolder, const InstallProgressFn& = {});

/** The folder vocoders are kept in: Voices/Vocoders. */
juce::File getVocodersFolder (const juce::File& voicesFolder);

/** Moves a voice's folder to the bin. */
bool removeVoice (const juce::File& voiceFolder);
} // namespace daw::sing
