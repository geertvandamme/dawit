#pragma once

#include "engine/Engine.h"

namespace daw::bounceui
{
/** File > Bounce to WAV...: asks what to bounce (the mix, or the selected tracks as you hear them soloed), the
    range (the loop or the whole song), the sample rate, the bit depth and whether to let the end ring out, then
    where to save it, and bounces with a progress window and Cancel. `projectName` names the file. `selected` are
    the tracks selected in the headers; `pickSelected` starts the window on *Selected tracks* (the track menu). */
void showDialog (Engine&, const juce::String& projectName, const std::vector<Track*>& selected = {}, bool pickSelected = false);
} // namespace daw::bounceui
