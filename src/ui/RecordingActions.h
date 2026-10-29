#pragma once

#include "engine/Engine.h"
#include "engine/RecordingGuide.h"

namespace daw
{
/**
    The audio recording parts that several views share: the input menu of an audio track (the device's channels,
    each mono and each stereo pair), the Monitor switch with its one-off "use headphones" warning, the input meter,
    the snapshot of the user's device for the guide and the settings panel, and the messages after a recording.
*/
namespace recordingui
{
    /** What the input chip next to R says: "In 1" or "In 1+2" (1-based). */
    juce::String getInputChipText (const Track&);

    /** The device's input channels as a menu for an audio track: each channel mono, then each stereo pair, with the
        device's channel names, the current choice ticked. Ids run from `firstId`; apply the choice with applyInputChoice(). */
    juce::PopupMenu makeInputMenu (Engine&, const Track&, int firstId);

    /** Does what the chosen item of makeInputMenu() says (ignores ids outside it). */
    void applyInputChoice (Engine&, Track&, int firstId, int chosenId);

    /** Opens the input menu under a component and applies the choice. */
    void showInputMenu (Engine&, Track&, juce::Component* target);

    /** Turns Monitor on or off. The first time it is turned on it explains headphones with the real round trip, and
        offers the guide's *Hearing yourself* topic through `showGuide` (may be empty). */
    void setMonitor (Engine&, Track&, bool on, std::function<void (recordingguide::Topic)> showGuide);

    /** The user's device, latency, armed tracks and last take, for the guide's *Your session* and the settings panel. */
    recordingguide::Snapshot makeSnapshot (Engine&);

    /** The input level of an armed audio track and the red "it clipped" mark, which holds for two seconds. */
    struct InputMeter
    {
        /** Call with each new reading (linear gain); returns true when what is drawn changed. */
        bool update (float level);

        float level = 0.0f;
        double clippedAtMs = -1.0e9;
        bool isClipHeld() const;
    };

    /** A mono input meter with marks at -6 and 0 dB and the zones of gainstaging::getZone: quiet below -24 dB, healthy up to -6, hot to 0.
        Vertical when `vertical`, else horizontal. */
    void drawInputMeter (juce::Graphics&, juce::Rectangle<float> area, const InputMeter&, bool vertical);

    /** The meter's tooltip. */
    juce::String getInputMeterTooltip();

    /** Shows the messages of a finished recording (nothing came in, no sound at all, lost samples), once for each
        recording. Call it from a timer; `seenVersion` is the caller's memory of the last one shown. */
    void showReportMessages (Engine&, int& seenVersion);
}
} // namespace daw
