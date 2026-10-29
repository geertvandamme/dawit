#pragma once

#include <juce_core/juce_core.h>

#include <functional>

namespace daw::models
{
/**
    The large models DawIt downloads on first use (Demucs for stems, Whisper for lyrics), each
    at a fixed revision and checked against its size and SHA-256 before it's used. They live in
    the Models folder next to DawIt's settings, so they're downloaded only once.
*/
struct Model
{
    const char* fileName;
    const char* url;
    const char* sha256;
    juce::int64 size;
};

/** Reports progress from 0 to 1; returning false cancels. */
using ProgressFn = std::function<bool (double)>;

/** ~/Library/Application Support/DawIt/Models on macOS. */
juce::File getFolder();

juce::File getFile (const Model&);

/** True when the file is there at its full size. */
bool isInstalled (const Model&);

/** The size as people read it: "84 MB", "1.5 GB". */
juce::String formatSize (juce::int64 bytes);

/** Downloads the model to getFile() and checks it. Returns an error message, or an empty string
    on success or when cancelled. Blocks; call it from a background thread. */
juce::String download (const Model&, const ProgressFn&);
} // namespace daw::models
