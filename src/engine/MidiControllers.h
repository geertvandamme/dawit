#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace daw
{
/** One controller message in a MIDI clip: a controller number (0 to 119), pitch bend or channel pressure, at a
    beat from the clip start, with a value in the message's own range. The value holds until the next event of
    the same kind, as on a cable. See `daw::controllers`. */
struct ControllerEvent
{
    int kind = 0;
    double beat = 0.0;
    int value = 0;

    bool operator== (const ControllerEvent& other) const noexcept
    {
        return kind == other.kind && juce::exactlyEqual (beat, other.beat) && value == other.value;
    }
};

/** Controller events of MIDI clips: the kinds, ranges and rest values, and pure functions on a clip's list
    (kept sorted by beat, then kind, with one value per kind per beat). */
namespace controllers
{
    constexpr int sustainPedal = 64, modWheel = 1, volume = 7, pan = 10, expression = 11;
    constexpr int maxController = 119;                  // 120 to 127 are channel mode messages, not knobs
    constexpr int pitchBend = 128, channelPressure = 129;
    constexpr int numKinds = 130;
    constexpr int bendCentre = 8192, bendMax = 16383;
    constexpr double lineStep = 1.0 / 32.0;             // the finest spacing of drawn points, in beats

    using List = std::vector<ControllerEvent>;

    /** True for a controller number, pitch bend or channel pressure. */
    inline bool isValidKind (int kind) noexcept { return kind >= 0 && kind <= channelPressure && ! (kind > maxController && kind < pitchBend); }

    /** The largest value of a kind: 16,383 for pitch bend, 127 for the others. */
    inline int getMaxValue (int kind) noexcept { return kind == pitchBend ? bendMax : 127; }

    /** The value a part sits at before anything is sent: 8,192 for the bend, 127 for volume and expression,
        64 (middle) for pan and 0 for the rest. */
    inline int getRestValue (int kind) noexcept
    {
        if (kind == pitchBend)
            return bendCentre;

        if (kind == volume || kind == expression)
            return 127;

        return kind == pan ? 64 : 0;
    }

    /** True for the kinds a Reset All Controllers puts back, and so DawIt resets on stop, jump and at a clip's end:
        the pedal, bend, mod wheel, expression and pressure. Volume, pan and the rest keep their last value. */
    inline bool isResetKind (int kind) noexcept
    {
        return kind == sustainPedal || kind == pitchBend || kind == modWheel || kind == expression || kind == channelPressure;
    }

    /** The value a reset puts back: the rest value. */
    inline int getResetValue (int kind) noexcept { return getRestValue (kind); }

    /** A lane's name in plain words, lower case after the first letter: "Pitch bend", "Sustain pedal", "Controller 74". */
    inline juce::String getName (int kind)
    {
        switch (kind)
        {
            case pitchBend:       return "Pitch bend";
            case channelPressure: return "Channel pressure";
            case sustainPedal:    return "Sustain pedal";
            case modWheel:        return "Mod wheel";
            case volume:          return "Volume";
            case pan:             return "Pan";
            case expression:      return "Expression";
            default:              return "Controller " + juce::String (kind);
        }
    }

    /** Puts a list in order: events of unknown kinds are dropped, beats made 0 or more, values clamped, sorted by
        beat then kind, and where two events of a kind share a beat the later one in the list wins. */
    inline void tidy (List& events)
    {
        events.erase (std::remove_if (events.begin(), events.end(), [] (const ControllerEvent& e) { return ! isValidKind (e.kind) || ! std::isfinite (e.beat); }),
                      events.end());

        for (auto& e : events)
        {
            e.beat = juce::jmax (0.0, e.beat);
            e.value = juce::jlimit (0, getMaxValue (e.kind), e.value);
        }

        std::stable_sort (events.begin(), events.end(), [] (const ControllerEvent& a, const ControllerEvent& b)
        {
            return a.beat < b.beat || (juce::exactlyEqual (a.beat, b.beat) && a.kind < b.kind);
        });

        // A later write replaces an earlier one at the same beat and kind (stable order keeps the later one last).
        List out;

        for (auto& e : events)
        {
            if (! out.empty() && out.back().kind == e.kind && juce::exactlyEqual (out.back().beat, e.beat))
                out.back() = e;
            else
                out.push_back (e);
        }

        events = std::move (out);
    }

    /** The value of `kind` at `beat` (an event at the beat counts), or its rest value before the first event. */
    inline int getValueAt (const List& events, int kind, double beat)
    {
        int value = getRestValue (kind);

        for (auto& e : events)
        {
            if (e.beat > beat + 1.0e-9)
                break;

            if (e.kind == kind)
                value = e.value;
        }

        return value;
    }

    /** True when some event of `kind` is at or before `beat`. */
    inline bool hasValueBy (const List& events, int kind, double beat)
    {
        for (auto& e : events)
            if (e.kind == kind && e.beat <= beat + 1.0e-9)
                return true;

        return false;
    }

    /** The kinds that have events, in ascending order. */
    inline std::vector<int> getKindsUsed (const List& events)
    {
        std::vector<int> kinds;

        for (auto& e : events)
            if (std::find (kinds.begin(), kinds.end(), e.kind) == kinds.end())
                kinds.push_back (e.kind);

        std::sort (kinds.begin(), kinds.end());
        return kinds;
    }

    /** The events of one kind, in order. */
    inline List getLane (const List& events, int kind)
    {
        List lane;

        for (auto& e : events)
            if (e.kind == kind)
                lane.push_back (e);

        return lane;
    }

    /** The list with the events of `kind` replaced by `lane` (whose events are given that kind), tidied. */
    inline List withLane (const List& events, int kind, List lane)
    {
        List out;

        for (auto& e : events)
            if (e.kind != kind)
                out.push_back (e);

        for (auto e : lane)
        {
            e.kind = kind;
            out.push_back (e);
        }

        tidy (out);
        return out;
    }

    /** How far thinning may move the sound: 1 step, and 32 steps for the 14-bit pitch bend (the same 0.8 %). */
    inline int getThinTolerance (int kind) noexcept { return kind == pitchBend ? 32 : 1; }

    /** The list with the points of `kind` that lie on a straight line between their neighbours (within
        `tolerance` steps) removed, so a recorded wheel takes less room. The first and last point stay. */
    inline List thinOut (const List& events, int kind, int tolerance)
    {
        const auto lane = getLane (events, kind);

        if (lane.size() < 3)
            return events;

        List kept { lane.front() };
        size_t anchor = 0;

        while (anchor + 1 < lane.size())
        {
            // The farthest point that every point between the anchor and it stays within the tolerance of the line to.
            size_t best = anchor + 1;

            for (size_t end = anchor + 2; end < lane.size(); ++end)
            {
                bool fits = true;

                for (size_t i = anchor + 1; i < end && fits; ++i)
                {
                    const double t = (lane[i].beat - lane[anchor].beat) / (lane[end].beat - lane[anchor].beat);
                    const double on = lane[anchor].value + t * (lane[end].value - lane[anchor].value);
                    fits = std::abs (on - lane[i].value) <= tolerance + 1.0e-9;
                }

                if (! fits)
                    break;

                best = end;
            }

            kept.push_back (lane[best]);
            anchor = best;
        }

        return withLane (events, kind, kept);
    }

    /** Points for a straight line of `kind` from one beat and value to another, one every `step` beats at most,
        with repeated values left out. */
    inline List getLinePoints (int kind, double beat0, int value0, double beat1, int value1, double step = lineStep)
    {
        List points;

        if (beat1 < beat0)
        {
            std::swap (beat0, beat1);
            std::swap (value0, value1);
        }

        const int steps = juce::jmax (1, (int) std::ceil ((beat1 - beat0) / step - 1.0e-9));

        for (int i = 0; i <= steps; ++i)
        {
            const double t = (double) i / steps;
            const int value = juce::jlimit (0, getMaxValue (kind), juce::roundToInt (value0 + t * (value1 - value0)));

            if (! points.empty() && points.back().value == value)
                continue;   // a repeated value changes nothing, and the last point is only needed when it differs

            points.push_back ({ kind, beat0 + t * (beat1 - beat0), value });
        }

        return points;
    }

    /** What is left of a clip's events when it is cut at `cut` beats: the left part keeps the events before the cut,
        the right one those from it on, moved to start at 0, and begins with each lane's value at the cut (when it
        isn't the rest value) so it sounds the same alone. */
    struct Split
    {
        List left, right;
    };

    inline Split splitAt (const List& events, double cut)
    {
        Split s;

        for (auto e : events)
        {
            if (e.beat < cut - 1.0e-9)
            {
                s.left.push_back (e);
            }
            else
            {
                e.beat = juce::jmax (0.0, e.beat - cut);
                s.right.push_back (e);
            }
        }

        for (int kind : getKindsUsed (s.left))
        {
            const bool startsAtCut = std::any_of (s.right.begin(), s.right.end(), [kind] (const ControllerEvent& e)
                                                  { return e.kind == kind && e.beat < 1.0e-9; });
            const int value = getValueAt (s.left, kind, cut);

            if (! startsAtCut && value != getRestValue (kind))
                s.right.push_back ({ kind, 0.0, value });
        }

        tidy (s.right);
        return s;
    }

    /** The events a clip of `clipLength` beats plays, as they are when another clip of `shift` beats later joins it:
        events past the clip's end are dropped (they are never played) and the rest moved by `shift`. */
    inline List getAudible (const List& events, double clipLength, double shift)
    {
        List out;

        for (auto e : events)
            if (e.beat < clipLength)
            {
                e.beat += shift;
                out.push_back (e);
            }

        return out;
    }

    /** A message to send at a beat of the song (not of the clip). */
    struct TimedMessage
    {
        double beat = 0.0;
        int kind = 0;
        int value = 0;
    };

    /** What a clip sends during a span of the song, from `spanStart` up to but not including `spanEnd`. */
    struct SpanMessages
    {
        std::vector<TimedMessage> resets;       // at the clip's end: sent after the Note Offs
        std::vector<TimedMessage> controllers;  // the chase at the span start, then the events: sent before the Note Ons
    };

    /** The controller messages of a clip that starts at `clipStart` and is `clipLength` beats long, for a span of the
        song. With `chase` (the span starts after a stop, jump, loop wrap or edit) every lane that already has a value
        sends its last value at the span start, so a part sounds the same wherever playback starts. Events before the
        clip's end play; at the clip's end the reset kinds the clip used and that are away from rest go back to rest. */
    inline SpanMessages getSpanMessages (const List& events, double clipStart, double clipLength, double spanStart, double spanEnd, bool chase)
    {
        SpanMessages out;
        const double clipEnd = clipStart + clipLength;

        if (events.empty() || clipStart >= spanEnd || clipEnd < spanStart)
            return out;

        if (chase && spanStart >= clipStart && spanStart < clipEnd)
        {
            const double rel = spanStart - clipStart;

            for (int kind : getKindsUsed (events))
            {
                int value = -1;

                for (auto& e : events)
                {
                    if (e.beat >= rel - 1.0e-9)
                        break;

                    if (e.kind == kind)
                        value = e.value;
                }

                // Only what differs from the state a reset leaves needs sending; the others are already there.
                if (value >= 0 && (value != getRestValue (kind) || ! isResetKind (kind)))
                    out.controllers.push_back ({ spanStart, kind, value });
            }
        }

        for (auto& e : events)
        {
            if (e.beat >= clipLength)
                break;

            const double beat = clipStart + e.beat;

            if (beat >= spanStart && beat < spanEnd)
                out.controllers.push_back ({ beat, e.kind, e.value });
        }

        if (clipEnd >= spanStart && clipEnd < spanEnd)
        {
            for (int kind : getKindsUsed (events))
            {
                if (! isResetKind (kind))
                    continue;

                int last = getRestValue (kind);

                for (auto& e : events)
                {
                    if (e.beat >= clipLength)
                        break;

                    if (e.kind == kind)
                        last = e.value;
                }

                if (last != getRestValue (kind))
                    out.resets.push_back ({ clipEnd, kind, getResetValue (kind) });
            }
        }

        return out;
    }

    /** The MIDI message for an event, on channel 1. */
    inline juce::MidiMessage toMessage (int kind, int value)
    {
        if (kind == pitchBend)
            return juce::MidiMessage::pitchWheel (1, juce::jlimit (0, bendMax, value));

        if (kind == channelPressure)
            return juce::MidiMessage::channelPressureChange (1, juce::jlimit (0, 127, value));

        return juce::MidiMessage::controllerEvent (1, kind, juce::jlimit (0, 127, value));
    }
}
} // namespace daw
