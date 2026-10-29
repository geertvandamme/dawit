#pragma once

#include <juce_events/juce_events.h>

#include "engine/Track.h"

namespace daw
{
/** The currently selected tracks and clip, shared between views. Message thread only.

    One track is "the" selected track (`getTrack()`: the one clicked last, which the device bar, the piano roll
    and the keyboard follow); Shift-click and Cmd-click on a header or strip select several (`getTracks()`), so
    a menu can act on all of them, such as Create bus. */
class Selection final : public juce::ChangeBroadcaster
{
public:
    Track* getTrack() const noexcept { return track; }
    Clip* getClip() const noexcept { return clip; }

    /** The selected tracks in the order they were chosen; the one clicked last is `getTrack()`. */
    const std::vector<Track*>& getTracks() const noexcept { return tracks; }

    bool isSelected (const Track* t) const noexcept
    {
        return t != nullptr && std::find (tracks.begin(), tracks.end(), t) != tracks.end();
    }

    MidiClip* getMidiClip() const noexcept
    {
        return clip != nullptr && clip->kind == Clip::Kind::midi ? static_cast<MidiClip*> (clip) : nullptr;
    }

    /** Selects only this track (and a clip on it). */
    void select (Track* newTrack, Clip* newClip = nullptr)
    {
        const bool alone = newTrack == nullptr ? tracks.empty() : (tracks.size() == 1 && tracks.front() == newTrack);

        if (newTrack == track && newClip == clip && alone)
            return;

        track = anchor = newTrack;
        clip = newClip;
        tracks.clear();

        if (newTrack != nullptr)
            tracks.push_back (newTrack);

        sendSynchronousChangeMessage();
    }

    /** Cmd-click: adds the track to the selection, or takes it out. The device bar follows the one added. */
    void toggle (Track* t)
    {
        if (t == nullptr)
            return;

        if (const auto it = std::find (tracks.begin(), tracks.end(), t); it != tracks.end())
        {
            tracks.erase (it);

            if (track == t)
            {
                track = tracks.empty() ? nullptr : tracks.back();
                clip = nullptr;
            }
        }
        else
        {
            tracks.push_back (t);
            track = t;
            clip = nullptr;
        }

        anchor = track;
        sendSynchronousChangeMessage();
    }

    /** Shift-click: selects every track from the one clicked last to this one, in the order of `rows`. */
    void extendTo (Track* t, const std::vector<Track*>& rows)
    {
        const auto from = std::find (rows.begin(), rows.end(), anchor);
        const auto to = std::find (rows.begin(), rows.end(), t);

        if (t == nullptr || anchor == nullptr || from == rows.end() || to == rows.end())
        {
            select (t);
            return;
        }

        tracks.assign (std::min (from, to), std::max (from, to) + 1);
        track = t;
        clip = nullptr;
        sendSynchronousChangeMessage();
    }

    /** Must be called before a track or clip is deleted. */
    void forget (const Track* t, const Clip* c = nullptr)
    {
        if (t != nullptr && isSelected (t))
        {
            if (t == track || tracks.size() == 1)
            {
                select (nullptr, nullptr);
                return;
            }

            tracks.erase (std::find (tracks.begin(), tracks.end(), t));
            anchor = track;
            sendSynchronousChangeMessage();
        }
        else if (c != nullptr && c == clip)
        {
            select (track, nullptr);
        }
    }

private:
    Track* track = nullptr;
    Track* anchor = nullptr;   // where Shift-click starts from
    std::vector<Track*> tracks;
    Clip* clip = nullptr;
};

/** Horizontal mapping between beats and pixels, shared by the ruler and the track lanes. */
struct TimelineView
{
    double pixelsPerBeat = 28.0;
    double viewStartBeat = 0.0;

    float beatToX (double beat) const noexcept { return (float) ((beat - viewStartBeat) * pixelsPerBeat); }
    double xToBeat (double x) const noexcept { return viewStartBeat + x / pixelsPerBeat; }
};
} // namespace daw
