#pragma once

#include "EffectWindow.h"
#include "engine/BuiltInTune.h"
#include "engine/MusicalKey.h"
#include "engine/Section.h"
#include "engine/TempoMap.h"

namespace daw::tunetext
{
/** What DawIt Tune's window and guide need from the song. */
struct SongInfo
{
    MusicalKey key;
    SectionList sections;
    TempoMap tempo;
    int numBars = 16;
};

/** The app's side of it: the song, a way to move the playhead, and the saved settings. Installed by MainComponent. */
struct SongAccess
{
    std::function<SongInfo()> getSong;
    std::function<void (double)> seekToBeat;
    std::function<juce::PropertiesFile*()> getSettings;
};

void setSongAccess (SongAccess);
SongInfo getSong();
void seekToBeat (double beat);
juce::PropertiesFile* getSettings();

/** The pages of What is pitch correction?, following the effect's settings and what it has heard. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInTune&);
}
