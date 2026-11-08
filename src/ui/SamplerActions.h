#pragma once

#include "engine/BuiltInSampler.h"
#include "engine/Engine.h"
#include "PluginWindows.h"

namespace daw::sampleractions
{
/** The track's DawIt Sampler, or null when it plays something else. */
BuiltInSampler* findSampler (Track&);

/** What a menu item chose: a SoundFont, and one of its sounds (programs 0 to 127; the window shows 1 to 128). */
using PickFn = std::function<void (const soundfonts::Reference&, int bank, int program)>;

/** The sound to start with in a SoundFont: bank 0 program 0 when it has it, otherwise its first sound. Reads the
    file's header only. False when the file can't be read. */
bool getFirstPreset (const soundfonts::Reference&, int& bank, int& program);

/** Runs `then` once MuseScore General is on this computer. When it isn't, first asks "Download MuseScore General?"
    ([Download] [Not now]; Not now does nothing), then downloads it with a progress window and a Cancel button. After a
    download every sampler that was waiting for the set starts loading it. */
void whenMuseScoreGeneralIsHere (Engine&, std::function<void()> then);

/** True when the reference is MuseScore General and it hasn't been downloaded. */
bool needsDownload (const soundfonts::Reference&);

/** The "Add SoundFont..." flow: a file chooser (.sf2 and .sf3; an .sfz gets one sentence of explanation), the file is
    checked, then you choose between copying it into DawIt's SoundFonts folder (the default) and using it where it
    is. A problem is shown in plain words. `whenAdded` gets the reference and its first sound. */
void addSoundFont (Engine&, PickFn whenAdded);

/** The "Sampled sounds" submenu: MuseScore General first (its sounds by General MIDI family, or, while it isn't
    downloaded, the starting points, which ask to download), then each SoundFont you added by file name, then
    "Add SoundFont...". `current` (may be null) has its font and sound ticked. With `openGuide`, the last item opens the
    guide. Items run `pick`, or the add flow. */
juce::PopupMenu createSampledSoundsMenu (Engine&, const BuiltInSampler* current, PickFn pick, std::function<void()> openGuide = {});

/** Puts a track on a sampled sound: DawIt Sampler is added when the track plays something else (replacing its
    instrument), and when it already has one only the SoundFont and sound change. Asks to download MuseScore General
    first when the pick is from it and it isn't there. */
void useSampledSound (Engine&, PluginWindowManager&, Track&, const soundfonts::Reference&, int bank, int program);

/** Installs what "What is a sampler?..." does in the track menus (the app opens the guide); with none, the item is left out. */
void setGuideOpener (std::function<void()>);

/** "Sampled sounds" for a track's instrument menu (track header, add menu, device bar); it ends with the guide. */
juce::PopupMenu createTrackMenu (Engine&, PluginWindowManager&, Track&);
} // namespace daw::sampleractions
