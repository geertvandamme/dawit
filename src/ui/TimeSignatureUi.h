#pragma once

#include "engine/Engine.h"

#include <functional>

namespace daw
{
/** The pieces of the interface that show and change the song's time signature: the menu of the common
    signatures (from the transport bar and the ruler), the *Other...* dialog, the question asked when the
    song already has parts in it, and the tooltip texts. */
namespace timesigui
{
    /** What a grid line is: the start of a bar, of a beat group (the beats you count: all the quarters in 4/4 and
        3/4, the dotted quarters in 6/8), of a pulse (the bottom note, drawn only where it differs from the beat:
        eighths in 6/8 and 7/8, quarters in 2/2), or none of these. */
    enum class Line { bar, beat, pulse, other };

    /** What kind of grid line falls on `beat`. */
    Line getLineKind (daw::TimeSignature, double beat);

    /** The distance between the lines the grid draws when zoomed in, in quarter notes: the pulse where beats are
        split into pulses (6/8: 0.5, 2/2: 1), else one quarter note. */
    double getLineStep (daw::TimeSignature);

    /** A length in beats without trailing zeros: "3", "3.5". */
    juce::String formatBeats (double beats);

    /** A tooltip for the button next to the tempo: what a bar is, and how the tempo counts. */
    juce::String getButtonTooltip (const Engine&);

    /** A tooltip for the signature drawn on the ruler. */
    juce::String getMarkTooltip (daw::TimeSignature);

    /** The sentence the metronome button's tooltip ends with: how many clicks a bar has and how loud each is. */
    juce::String getMetronomeSentence (daw::TimeSignature);

    /** True when the song has clips, chords, sections, lyrics or tempo changes, so changing the signature
        needs the question *Keep each part in its bar* or *Keep the timing*. */
    bool hasPartsToMove (const Engine&);

    /** Adds the nine common signatures (the song's ticked), *Other...* and a separator to `menu`. They are
        disabled while recording, because a bar line that moves under a take would break it. */
    void addItems (juce::PopupMenu& menu, Engine&, juce::Component* owner);

    /** Opens the menu of signatures under `target` (or at the mouse), with the guide as its last item. */
    void showMenu (Engine&, juce::Component& target, std::function<void()> onShowGuide, bool atMouse = false);

    /** Changes the signature: straight away in an empty song, otherwise after asking how the parts move.
        `owner` anchors the dialog. */
    void choose (Engine&, daw::TimeSignature, juce::Component* owner);

    /** Opens the dialog with a top number and a bottom number. */
    void showOtherDialog (Engine&, juce::Component* owner);
} // namespace timesigui
} // namespace daw
