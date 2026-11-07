#pragma once

#include "EffectWindow.h"
#include "engine/BuiltInMeter.h"

namespace daw::metertext
{
/** What a measurement of the whole song (File > Measure Loudness...) found: the readings, and a heading that says
    which part of the song it covers ("The whole song (bar 1 to bar 37, 3:12):"). */
struct SongResult
{
    loudness::Readings readings;
    juce::String scope;
};

/** The app's side of "Your track": the last song measurement, and a way to start a new one. Installed by MainComponent;
    empty in a plugin, where the page reads the instance instead. */
struct SongAccess
{
    std::function<std::optional<SongResult>()> getLast;
    std::function<void()> measure;
};

void setSongAccess (SongAccess);

/** The pages of What is loudness? LUFS explained, drawn from the meter's own code and following the effect's readings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInMeter&);
}
