#pragma once

#include "Bounce.h"
#include "LoudnessMeter.h"

namespace daw::loudness
{
/** What measuring a range of the song gave. */
struct SongMeasurement
{
    juce::Result result = juce::Result::ok();
    bool cancelled = false;
    Readings readings;       // integrated, range, maxima, peaks and the length measured (seconds)
};

/**
    Renders the range the Renderer was made for into a loudness meter and returns what it read, the same numbers
    DawIt Meter shows. `cancel` is checked after every block (set it from any thread to stop); `progress`, if given,
    gets 0 to 1. Run it on a background thread: the Renderer is made and destroyed on the message thread.
*/
SongMeasurement measureSong (bounce::Renderer&, double sampleRate, const std::atomic<bool>* cancel = nullptr,
                             const std::function<void (double)>& progress = {});
} // namespace daw::loudness
