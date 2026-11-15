#pragma once

#include "engine/Engine.h"
#include "generate/MixExport.h"

namespace daw::bounceui
{
/** File > Bounce to WAV...: asks what to bounce (the mix, the selected tracks as you hear them soloed, or one file per
    track or per bus into a new folder, as you hear them or dry), the range (the loop or the whole song), the sample rate, the bit depth and whether to let the end ring out, then
    where to save it, and bounces with a progress window and Cancel. `projectName` names the file. `selected` are
    the tracks selected in the headers; `pickSelected` starts the window on *Selected tracks* (the track menu);
    `pickMixSong` starts it on *The mix* and *Whole song* (the mix and export exercise). */
void showDialog (Engine&, const juce::String& projectName, const std::vector<Track*>& selected = {}, bool pickSelected = false, bool pickMixSong = false);

/** Sets what happens when a bounce finishes (not after a cancel or a failure): the listener gets what was bounced, the
    peak, the length and the file or folder name, on the message thread. Set once by MainComponent (the exercise's export record). */
void setFinishedListener (std::function<void (const gen::mixexport::ExportRecord&)>);

/** Sets what the "?" in the Bounce to WAV window does: it opens the Sending stems to an engineer guide, at *Preparing the
    files* when `files` is true (the choice is multitracks or stems) and at *Mixing or mastering?* otherwise. Set once by MainComponent. */
void setGuideOpener (std::function<void (bool files)>);

/** Runs `go` at once, or, when a DawIt Sampler would play the DawIt Synth in a bounce (its SoundFont is missing), first
    lists those tracks and asks "Bounce anyway" or "Cancel". Every bounce and render of the song calls it. */
void goUnlessFallbacks (const Engine&, std::function<void()> go);
} // namespace daw::bounceui
