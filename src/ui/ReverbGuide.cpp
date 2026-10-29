#include "ReverbGuide.h"
#include "engine/Tempo.h"
#include "engine/ReverbKinds.h"

#include <juce_dsp/juce_dsp.h>

namespace daw::verbtext
{
using verb::Knob;

namespace
{
    juce::String formatMs (double ms)
    {
        return fx::formatNumber (ms, ms < 10.0 ? 1 : 0) + " ms";
    }

    juce::String formatMetres (double m)
    {
        return fx::formatNumber (m, m < 10.0 ? 1 : 0) + " m";
    }

    /** A real place whose reverb lasts about this long. */
    juce::String getPlaceFor (double decay)
    {
        if (decay < 0.4) return "a car, a small furnished room or a vocal booth";
        if (decay < 0.8) return "a living room or a bedroom";
        if (decay < 1.3) return "a classroom, a small club or a studio live room";
        if (decay < 2.2) return "a concert hall";
        if (decay < 3.5) return "a large hall or a stone church";
        if (decay < 6.0) return "a cathedral";
        return "no ordinary room: only huge stone buildings, caves and empty water tanks ring this long";
    }

    constexpr double renderRate = 24000.0;

    /** The song's tempo, or 120 BPM while it is unknown. */
    double songTempo (double bpm) { return bpm > 0.0 ? bpm : 120.0; }
}

juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::predelay:  return "Pre-delay";
        case (int) Knob::size:      return "Size";
        case (int) Knob::decay:     return "Decay";
        case (int) Knob::damping:   return "Damping";
        case (int) Knob::lowCut:    return "Low cut";
        case (int) Knob::early:     return "Early reflections";
        case (int) Knob::width:     return "Width";
        case (int) Knob::mix:       return "Mix";
        case response:              return "What one clap turns into";
        case room:                  return "The room";
        case type:                  return "Type";
        case fxui::overview:        return "DawIt Reverb";
        case fxui::presets:         return "Starting points";
        case fxui::clap:            return "Clap";
        case fxui::listen:          return "Hear only the reverb";
        case fxui::guide:           return "What is reverb?";
        case fxui::history:         return "The last 4 seconds";
        default:                    return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::predelay:
            return "The time between the sound and the start of its reverb. In a real room, the sound from the instrument reaches "
                   "you first, straight away. The reflections have further to go, over the walls, so they arrive later; in a big "
                   "room, a lot later. That gap is a strong clue for the ear about how big the space is and how close the sound is.\n\n"
                   "In a mix, pre-delay is also a trick for clarity: the start of every word or note is heard dry, so it stays "
                   "sharp and up front, and the reverb follows behind it. Without pre-delay the reverb smears over the start of "
                   "the sound, which pushes it back and blurs it.";
        case (int) Knob::size:
            return "How big the room is. A bigger room has its walls further away, so:\n"
                   "- the early reflections come later and further apart;\n"
                   "- the echoes in the tail come back less often at first, so the tail builds up more slowly and sounds "
                   "smoother and grander.\n\n"
                   "Size and decay are separate: a big room with lots of soft surfaces can still die away quickly, and a small "
                   "tiled bathroom can ring for a long time. Size is about the shape of the reverb in time, decay about how long "
                   "it lasts.";
        case (int) Knob::decay:
            return "How long the tail lasts: the time it takes to die away by 60 dB, a millionth of its energy, which the ear "
                   "hears as gone. Acousticians call it RT60, the reverberation time.\n\n"
                   "In a real room it depends on how big the room is and how much its surfaces soak up. Wallace Sabine found the "
                   "rule around 1900: RT60 = 0.161 x volume / absorption. Twice the volume, twice the time; twice the absorption "
                   "(curtains, carpet, people), half the time.\n\n"
                   "Short decays sound like a room around the sound; long ones like a hall, a church or an effect. In a song, the "
                   "tail should usually have died away, or nearly, before the next chord or phrase, or the reverb blurs them "
                   "together.";
        case (int) Knob::damping:
            return "Makes the high frequencies die away sooner than the lows. That's what real rooms do: air, wood, curtains, "
                   "carpet and people soak up highs much more than lows, so in every reflection the sound loses a little more of "
                   "its brightness. A tail that stays bright to the end sounds hard and artificial; a darker tail sounds warm, "
                   "natural and further away.\n\n"
                   "Above the damping frequency the tail dies away in half the decay time or less. Turn it down for a warm, "
                   "soft room or an old recording; up (or off) for a bright, hard space or a plate.";
        case (int) Knob::lowCut:
            return "Takes the lows out of the reverb, and only out of the reverb: the dry sound keeps all its lows.\n\n"
                   "Low frequencies take long to die away in rooms, and in a mix a long low tail turns into a rumble that makes "
                   "the bass and kick unclear: mud. Cutting the reverb's lows keeps the low end tight while the space stays. It "
                   "is one of the first things engineers do to a reverb.";
        case (int) Knob::early:
            return "How loud the early reflections are: the first few echoes off the nearest walls, floor and ceiling, in the "
                   "first 10 to 100 ms. They are too close together to hear as echoes; the ear hears them as the character of "
                   "the room: its size, its shape, how close the walls are. Small rooms are mostly early reflections; in big "
                   "halls the tail takes over.\n\n"
                   "More early reflections make a sound thicker and more 'in a room'; fewer make the reverb a smooth tail "
                   "behind the sound, like a plate reverb or a big hall from far away.";
        case (int) Knob::width:
            return "How stereo the reverb is. In a real room the reflections come from all sides, so what reaches the left ear "
                   "differs from what reaches the right: that difference is what makes a space sound wide and enveloping.\n\n"
                   "At 100% the reverb's left and right are as different as they get. At 0% they are the same: mono, in the "
                   "middle. Narrower reverb keeps a sound focused and centred, and leaves room at the sides for other "
                   "instruments; mono is often best on bass and kick.";
        case (int) Knob::mix:
            return "How much of what comes out is reverb. 0% is only the dry sound, 100% only the reverb; in between the two "
                   "are mixed so the whole stays about as loud.\n\n"
                   "Reverb is a little like salt: too little and the sound is dry and disconnected, too much and everything is "
                   "far away and blurry. A good way to set it: turn it up until you clearly hear it, then back down until you "
                   "just stop noticing it. Then switch it off: if you miss it, it's right. Most reverb in a mix is felt more "
                   "than heard.\n\n"
                   "In most mixes one reverb sits on a return track and every track sends a part of itself to it, so all "
                   "instruments share one space. On a return, set the mix to 100% (the Return starting points do) and set each "
                   "track's amount with its send. See Buses, sends and routing.";
        case response:
            return "What a single short sound turns into, left to right in time, with the level up. The white bar is the "
                   "direct sound, the clap itself. After the pre-delay come the early reflections (orange): single echoes "
                   "off the nearest walls. Then the tail (green): thousands of reflections too dense to tell apart, dying "
                   "away. The dashed line shows the decay: from the tail's start down 60 dB, after the decay time. With damping, the "
                   "highs die away sooner than the line, so the outline of the whole tail falls a little faster at first.\n\n"
                   "Acousticians measure rooms just like this: pop a balloon or fire a starting pistol, record what comes "
                   "back, and read it. They call it the impulse response.\n\n"
                   "Drag the dot at the start of the reverb for the pre-delay and the dot at the end of the decay line for the "
                   "decay. Click Clap to hear it.";
        case room:
            return "The room the size stands for, seen from above: the sound (orange) and you (white). The white line is the "
                   "direct sound, the shortest way. The orange lines are the first reflections: one off each wall. Every "
                   "reflection is a longer way, so it comes later, and quieter.\n\n"
                   "The walls show the damping: hard walls (light grey) reflect the highs, soft walls (brown) soak them up.\n\n"
                   "Click Clap to watch the sound spread and bounce, slowed down 20 times. Each wall reflection travels as if it "
                   "came from a mirror image of the sound behind that wall, and the images of images make the tail.";
        case fxui::overview:
            return {};
        case fxui::presets:
            return "Settings for common sounds, each explaining why every knob is set the way it is. They are a start, not an "
                   "answer: play the track and adjust by ear, especially the mix.";
        case fxui::clap:
            return "Claps your hands into the reverb, the way you would to hear what a room sounds like. The clap is heard dry "
                   "and with the reverb at the mix, so change a knob and clap again to compare. It works while the song is "
                   "stopped.";
        case fxui::listen:
            return "Plays only the reverb, at full level, without the dry sound. Use it to hear what the reverb itself does: "
                   "whether its tail is too long, too bright, too boomy or too metallic. Turn it off again to set the mix.";
        case fxui::guide:
            return "A guide to what reverb is, for beginners: how sound bounces around a room, what the ear hears in it, why "
                   "rooms sound different, the kinds of reverb, how to use reverb in a mix, and how DawIt Reverb makes its sound. "
                   "The pictures follow the knobs here.";
        case fxui::history:
            return "The last 4 seconds while the track plays: grey the track, green the reverb as loud as it is mixed. Look at "
                   "what happens after a note or a hit: the green tail hanging on. If it hangs on into the next note or beat "
                   "(the lines), the reverb may blur the playing; a shorter decay or a lower mix keeps it clear.";
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::predelay:
            return "0-10 ms: small rooms, drums, things that should sound as if they were in the room.\n"
                   "20-40 ms: most instruments in a medium or large space.\n"
                   "60-120 ms: vocals, to keep the words clear in front of the reverb.\n"
                   "To the tempo: a 1/32 or 1/16 note, so the reverb starts in the groove.";
        case (int) Knob::size:
            return "Small (10-30%): drums, guitars, a room around a dry sound.\n"
                   "Medium (40-60%): vocals, most instruments.\n"
                   "Large (70-100%): strings, choirs, pads, ballads, effects.";
        case (int) Knob::decay:
            return "0.3-0.8 s: rooms, drums, fast songs.\n"
                   "1.2-2.0 s: vocals and instruments in most pop and rock.\n"
                   "2.0-3.5 s: ballads, strings, piano, choir.\n"
                   "4 s and more: pads, ambient, effects.";
        case (int) Knob::damping:
            return "3-5 kHz: warm and dark: halls, strings, vintage.\n"
                   "6-8 kHz: natural for most things.\n"
                   "10 kHz to off: bright: plates, snare, airy vocals.";
        case (int) Knob::lowCut:
            return "60-100 Hz: strings, piano, anything whose low end belongs in the space.\n"
                   "150-300 Hz: vocals, guitars, drums: most things in a mix.\n"
                   "300-500 Hz: bass and kick, if they get any reverb at all.";
        case (int) Knob::early:
            return "70-100%: small rooms, drum rooms, making a dry sound sound recorded in a room.\n"
                   "30-50%: natural halls.\n"
                   "0-20%: plates, ambient tails, vocals that should stay close.";
        case (int) Knob::width:
            return "100%: most reverbs.\n"
                   "50-80%: to keep a sound focused, or to leave room for others.\n"
                   "0%: bass, kick, and checking how the reverb sounds in mono.";
        case (int) Knob::mix:
            return "5-15%: just a touch, felt rather than heard.\n"
                   "15-30%: clearly in a space; vocals, guitars, keys.\n"
                   "30-50%: far away, ambient, strings in a hall.\n"
                   "100%: with Hear only the reverb, for a pure wash.";
        default:
            return {};
    }
}

juce::String describeSettings (int t, const verb::Settings& s, double bpm)
{
    const double first = verb::getFirstReflectionMs (s.sizePercent);

    switch (t)
    {
        case (int) Knob::predelay:
        {
            auto text = verb::formatKnob (Knob::predelay, s) + ": the reverb starts " + formatMs (s.predelayMs) + " after the sound. In a real "
                        "room that's walls about " + formatMetres (s.predelayMs * 0.343) + " further away than the sound, as sound "
                        "travels 34 cm every millisecond.";

            if (bpm > 0.0)
                text << " At " << formatBpm (bpm) << " BPM a 1/16 note lasts " << formatMs (15000.0 / bpm) << " and a 1/32 note "
                     << formatMs (7500.0 / bpm) << ".";

            return text;
        }

        case (int) Knob::size:
            if (s.type == verb::Type::plate)
                return "Size " + verb::formatKnob (Knob::size, s) + ": on a plate, how big the sheet is. A bigger plate has more resonances, closer together, so its tail sounds smoother and a little deeper.";
            if (s.type == verb::Type::spring)
                return "Size " + verb::formatKnob (Knob::size, s) + ": the spring's length: a repeat every " + formatMs (verb::kinds::springRoundTripMs (s.sizePercent)) + ".";
            return verb::formatKnob (Knob::size, s) + ": a room about " + formatMetres (verb::getRoomMetres (s.sizePercent)) + " across. The "
                   "first reflection comes " + formatMs (first) + " after the direct sound (after the pre-delay), and the last "
                   "early reflection after " + formatMs (first * 3.8) + ".";

        case (int) Knob::decay:
        {
            auto text = verb::formatKnob (Knob::decay, s) + " to die away by 60 dB: about as long as " + getPlaceFor (s.decaySeconds) + ".";

            if (bpm > 0.0)
            {
                const double beats = s.decaySeconds * bpm / 60.0;
                text << " At " << formatBpm (bpm) << " BPM the tail lasts " << juce::String (beats, 1) << " beats"
                     << (beats >= 4.0 ? " (" + juce::String (beats / 4.0, 1) + " bars of 4/4)" : juce::String()) << ".";
            }

            if (s.type == verb::Type::plate)
                text << " On a real plate the damper pad sets this: from under a second to about 5 s.";
            if (s.type == verb::Type::spring)
                text << " A real tank's decay is fixed, usually 1.75 to 4 s.";
            return text;
        }

        case (int) Knob::damping:
            if (! verb::isDampingOn (s.dampingHz))
                return "Off: the highs ring as long as the lows: bright and hard, like tiles, glass or a metal plate.";

            return verb::formatKnob (Knob::damping, s) + ": above it the tail dies away in " + fxui::formatSeconds (s.decaySeconds / 2.0)
                   + " or less, instead of " + fxui::formatSeconds (s.decaySeconds) + ". Higher still, faster still.";

        case (int) Knob::lowCut:
            if (! verb::isLowCutOn (s.lowCutHz))
                return "Off: the reverb keeps all its lows.";

            return verb::formatKnob (Knob::lowCut, s) + ": at " + verb::formatKnob (Knob::lowCut, s) + " the reverb is 3 dB quieter, an octave "
                   "lower (" + juce::String (juce::roundToInt (s.lowCutHz / 2.0f)) + " Hz) about 12 dB, two octaves lower 24 dB. The dry "
                   "sound keeps all its lows.";

        case (int) Knob::early:
            if (s.type == verb::Type::plate || s.type == verb::Type::spring)
                return "Early reflections: a " + verb::kinds::getName (s.type).toLowerCase() + " has no walls, so it has no early reflections. This knob works on the other types.";
            return verb::formatKnob (Knob::early, s) + ". With this size, the twelve early reflections come between " + formatMs (first)
                   + " and " + formatMs (first * 3.8) + " after the pre-delay.";

        case (int) Knob::width:
            if (s.widthPercent < 0.5f)
                return "0%: the reverb is mono, the same left and right, in the middle.";

            return verb::formatKnob (Knob::width, s) + (s.widthPercent > 99.5f ? ": as wide as it goes." : ": between mono and fully wide.");

        case (int) Knob::mix:
        {
            const double db = fx::getWetToDryDb (s.mixPercent);

            if (db <= -99.0) return "0%: only the dry sound.";
            if (db >= 99.0) return "100%: only the reverb.";

            return verb::formatKnob (Knob::mix, s) + ": the reverb comes out " + juce::String (std::abs (db), 1) + " dB "
                   + (db < 0.0 ? "under" : "over") + " the dry sound.";
        }

        case response:
            return "This clap: pre-delay " + verb::formatKnob (Knob::predelay, s) + ", then " + formatMs (first * 3.8)
                   + " of early reflections, then a tail of " + verb::formatKnob (Knob::decay, s) + ".";

        case room:
            if (s.type == verb::Type::plate || s.type == verb::Type::spring)
                return verb::kinds::describeSettings (s);
            return "About " + formatMetres (verb::getRoomMetres (s.sizePercent)) + " across, with "
                   + (verb::isDampingOn (s.dampingHz) && s.dampingHz < 6000.0f ? "soft" : verb::isDampingOn (s.dampingHz) ? "fairly hard" : "hard")
                   + " walls.";

        default:
            return {};
    }
}

//==============================================================================
Response computeResponse (const verb::Settings& s)
{
    Response r;
    r.settings = s;
    r.seconds = getShownSeconds (s);
    const auto early = verb::renderImpulse (s, renderRate, r.seconds, true, false);
    const auto tail = verb::renderImpulse (s, renderRate, r.seconds, false, true);
    // Keep the actual response peaks rather than inventing an echo pattern for the zoom strip.
    const int zoomSamples = juce::jmin (early.getNumSamples(), (int) (renderRate * 0.1));
    for (int sample = 1; sample + 1 < zoomSamples; ++sample)
    {
        auto magnitude = [&] (int at)
        {
            return std::abs (early.getSample (0, at)) + std::abs (tail.getSample (0, at));
        };
        const float peak = magnitude (sample);
        if (peak > 0.00001f && peak >= magnitude (sample - 1) && peak > magnitude (sample + 1))
            r.arrivals.emplace_back ((float) ((double) sample / renderRate * 1000.0), peak);
    }
    r.earlyDb = verb::getEnvelopeDb (early, renderRate, r.windowMs);
    r.tailDb = verb::getEnvelopeDb (tail, renderRate, r.windowMs);

    // Next to the direct sound: a click of 1 in the same window, as loud as the mix leaves it.
    const auto [dry, wet] = fx::getMixGains (s.mixPercent);
    const double windowSamples = r.windowMs * renderRate / 1000.0;
    const float offset = (float) (10.0 * std::log10 (windowSamples) + juce::Decibels::gainToDecibels ((double) wet, -100.0)
                                  - juce::Decibels::gainToDecibels (juce::jmax (0.05, (double) dry)));

    for (auto* levels : { &r.earlyDb, &r.tailDb })
        for (auto& db : *levels)
            db += offset;

    const int start = (int) (s.predelayMs / r.windowMs), end = start + (int) (80.0 / r.windowMs);

    for (int i = start; i < juce::jmin (end, (int) r.tailDb.size()); ++i)
        r.tailStartDb = juce::jmax (r.tailStartDb, r.tailDb[(size_t) i]);

    return r;
}

const Response& ResponseCache::get (const verb::Settings& s)
{
    if (! valid || s != response.settings)
    {
        response = computeResponse (s);
        valid = true;
    }

    return response;
}

double getShownSeconds (const verb::Settings& s)
{
    return juce::jlimit (0.4, 16.0, (s.predelayMs / 1000.0 + s.decaySeconds) * 1.25 + 0.05);
}

ResponsePlot drawResponse (juce::Graphics& g, juce::Rectangle<float> bounds, const Response& r, double bpm, double seconds, bool labels)
{
    ResponsePlot plot;
    plot.area = bounds.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f).withTrimmedTop (labels ? 20.0f : 4.0f).withTrimmedRight (8.0f);
    plot.seconds = seconds;
    const auto& area = plot.area;
    const auto& s = r.settings;

    g.setFont (juce::FontOptions (10.0f));

    for (float db = 0.0f; db >= ResponsePlot::bottomDb; db -= 20.0f)
    {
        g.setColour (theme::grid);
        g.fillRect (area.getX(), plot.toY (db), area.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (26.0f, 12.0f).withCentre ({ area.getX() - 15.0f, plot.toY (db) }),
                    juce::Justification::centredRight);
    }

    // Seconds along the bottom, in steps that fit.
    double step = 5.0;
    const double maxTicks = juce::jlimit (1.0, 8.0, std::floor ((double) area.getWidth() / 55.0));

    for (const double nice : { 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 5.0 })
        if (seconds / nice <= maxTicks) { step = nice; break; }

    for (double t = 0.0; t <= seconds + 1.0e-9; t += step)
    {
        g.setColour (theme::grid);
        g.fillRect (plot.toX (t), area.getY(), 1.0f, area.getHeight());
        g.setColour (theme::textDim);
        g.drawText (fxui::formatSeconds (t).replace (" ms", "ms").replace (".00 s", " s"),
                    juce::Rectangle<float> (50.0f, 12.0f).withCentre ({ plot.toX (t), area.getBottom() + 8.0f }), juce::Justification::centred);
    }

    if (bpm > 0.0)
    {
        g.setColour (theme::gridBar.withAlpha (0.55f));
        const double beat = 60.0 / bpm;
        const float dashes[] = { 2.0f, 3.0f };

        for (double t = beat; t < seconds; t += beat)
            g.drawDashedLine ({ plot.toX (t), area.getY(), plot.toX (t), area.getBottom() }, dashes, 2, 1.0f);
    }

    // The early reflections and the tail, as filled outlines.
    auto drawLevels = [&] (const std::vector<float>& levels, juce::Colour c)
    {
        juce::Path fill, line;
        fill.startNewSubPath (area.getX(), area.getBottom());
        bool started = false;

        for (size_t i = 0; i < levels.size(); ++i)
        {
            const double t = (i + 0.5) * r.windowMs / 1000.0;

            if (t > seconds)
                break;

            const float x = plot.toX (t), y = plot.toY (levels[i]);
            fill.lineTo (x, y);
            started ? line.lineTo (x, y) : line.startNewSubPath (x, y);
            started = true;
        }

        fill.lineTo (plot.toX (juce::jmin (seconds, r.seconds)), area.getBottom());
        fill.closeSubPath();
        g.setColour (c.withAlpha (0.25f));
        g.fillPath (fill);
        g.setColour (c);
        g.strokePath (line, juce::PathStrokeType (1.4f));
    };

    drawLevels (r.tailDb, colour);

    if (s.earlyPercent > 0.5f)
        drawLevels (r.earlyDb, earlyColour);

    // The direct sound.
    const auto [dry, wet] = fx::getMixGains (s.mixPercent);
    const float directDb = juce::Decibels::gainToDecibels (dry, -100.0f) - juce::Decibels::gainToDecibels (juce::jmax (0.05f, dry));
    g.setColour (juce::Colours::white);
    g.fillRect (area.getX(), plot.toY (directDb), 3.0f, area.getBottom() - plot.toY (directDb));

    // The decay: from the tail's start down 60 dB.
    const double tailStart = s.predelayMs / 1000.0, end = tailStart + s.decaySeconds;
    const float dashes[] = { 5.0f, 4.0f };

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (area.toNearestInt());
        g.setColour (theme::text.withAlpha (0.8f));
        g.drawDashedLine ({ plot.at (tailStart, r.tailStartDb), plot.at (end, r.tailStartDb - 60.0) }, dashes, 2, 1.2f);
    }

    if (labels)
    {
        g.setFont (juce::FontOptions (11.0f));
        auto label = [&] (const juce::String& text, float x, float y, juce::Colour c, juce::Justification j = juce::Justification::centredLeft)
        {
            g.setColour (c);
            g.drawText (text, juce::Rectangle<float> (170.0f, 14.0f).withPosition (j == juce::Justification::centredRight ? x - 170.0f : x, y - 7.0f), j);
        };

        label ("direct sound", area.getX() + 6.0f, plot.toY (directDb) + 8.0f, juce::Colours::white);

        if (s.predelayMs > 2.0f)
        {
            const float y = area.getY() - 10.0f;
            g.setColour (theme::textDim);
            g.drawArrow ({ area.getX(), y, plot.toX (tailStart), y }, 1.0f, 5.0f, 5.0f);
            label ("pre-delay " + verb::formatKnob (Knob::predelay, s), plot.toX (tailStart) + 4.0f, y, theme::textDim);
        }

        const float tailLabelX = plot.toX (tailStart + juce::jmin (s.decaySeconds * 0.35, seconds * 0.35));
        label ("tail", tailLabelX, plot.toY (r.tailStartDb - 0.35 * 60.0) - 14.0f, colour);

        if (s.earlyPercent > 0.5f)
            label ("early reflections", plot.toX (tailStart + verb::getFirstReflectionMs (s.sizePercent) * 4.0 / 1000.0) + 4.0f,
                   plot.toY (r.tailStartDb) - 12.0f, earlyColour);

        label ("-60 dB after " + verb::formatKnob (Knob::decay, s), plot.toX (end) - 4.0f, plot.toY (r.tailStartDb - 60.0) - 10.0f,
               theme::text, juce::Justification::centredRight);
    }

    return plot;
}

//==============================================================================
void drawRoom (juce::Graphics& g, juce::Rectangle<float> bounds, const verb::Settings& s, double since, bool labels)
{
    const double metres = verb::getRoomMetres (s.sizePercent);

    // The room grows with the size: 40 m fills the space, 5 m is small, but still big enough to see.
    auto space = bounds.withTrimmedBottom (labels ? 30.0f : 16.0f);
    const float fill = (float) juce::jmap (std::log (metres / 5.0) / std::log (8.0), 0.0, 1.0, 0.42, 1.0);
    const float w = juce::jmin (space.getWidth(), space.getHeight() * 1.5f) * fill, h = w / 1.5f;
    const auto roomArea = juce::Rectangle<float> (w, h).withCentre (space.getCentre());
    const double pixelsPerMetre = w / metres;

    const float hard = verb::isDampingOn (s.dampingHz) ? (float) (std::log (s.dampingHz / verb::minDamping) / std::log (verb::maxDamping / verb::minDamping)) : 1.0f;
    const auto wall = juce::Colour (0xff8a6a4a).interpolatedWith (juce::Colour (0xffc9ced8), hard);

    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRect (roomArea);

    const juce::Point<float> source { roomArea.getX() + w * 0.3f, roomArea.getY() + h * 0.42f };
    const juce::Point<float> listener { roomArea.getX() + w * 0.68f, roomArea.getY() + h * 0.6f };

    // After a clap: the sound and its mirror images behind the walls spread at 343 m/s, slowed down 20 times.
    constexpr double slowDown = 20.0;
    const double t = since / slowDown;
    const double fade = std::pow (10.0, -3.0 * t / juce::jmax (0.05, (double) s.decaySeconds));

    if (since >= 0.0 && fade > 0.01)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (roomArea.toNearestInt());
        const float radius = (float) (t * 343.0 * pixelsPerMetre);
        const float sx = source.x - roomArea.getX(), sy = source.y - roomArea.getY();

        for (int kx = -8; kx <= 8; ++kx)
        {
            for (int ky = -8; ky <= 8; ++ky)
            {
                const int order = std::abs (kx) + std::abs (ky);

                if (order > 12)
                    continue;

                const float x = roomArea.getX() + (float) kx * w + ((kx & 1) != 0 ? w - sx : sx);
                const float y = roomArea.getY() + (float) ky * h + ((ky & 1) != 0 ? h - sy : sy);
                const float alpha = (float) (0.7 * fade * std::pow (0.6 + 0.35 * hard, order));
                g.setColour ((order == 0 ? juce::Colours::white : earlyColour).withAlpha (alpha));
                g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ x, y }), order == 0 ? 2.0f : 1.4f);
            }
        }
    }

    // The direct way and the first reflections: the way to each wall's mirror image of the sound.
    const juce::Point<float> images[] = { { 2.0f * roomArea.getX() - source.x, source.y }, { 2.0f * roomArea.getRight() - source.x, source.y },
                                          { source.x, 2.0f * roomArea.getY() - source.y }, { source.x, 2.0f * roomArea.getBottom() - source.y } };

    for (auto image : images)
    {
        // Where the line from the image to the listener crosses the wall.
        juce::Point<float> hit;
        const juce::Line<float> way { image, listener };
        const juce::Line<float> walls[] = { { roomArea.getTopLeft(), roomArea.getBottomLeft() }, { roomArea.getTopRight(), roomArea.getBottomRight() },
                                            { roomArea.getTopLeft(), roomArea.getTopRight() }, { roomArea.getBottomLeft(), roomArea.getBottomRight() } };

        for (auto& wallLine : walls)
            if (way.intersects (wallLine, hit))
                break;

        g.setColour (earlyColour.withAlpha (0.55f));
        g.drawLine ({ source, hit }, 1.2f);
        g.drawLine ({ hit, listener }, 1.2f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawLine ({ source, listener }, 1.6f);

    g.setColour (wall);
    g.drawRect (roomArea, juce::jmap (hard, 5.0f, 2.0f));

    g.setColour (earlyColour);
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (source));
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (listener));

    g.setFont (juce::FontOptions (11.0f));
    g.setColour (theme::textDim);
    auto under = juce::Rectangle<float> (roomArea.getX(), roomArea.getBottom() + 3.0f, roomArea.getWidth(), 14.0f);
    g.drawText ("about " + formatMetres (metres) + " across", under, juce::Justification::centred);

    if (labels)
    {
        g.drawText (juce::String (hard > 0.6f ? "hard walls" : hard > 0.35f ? "fairly hard walls" : "soft walls") + " (damping "
                        + verb::formatKnob (Knob::damping, s) + ")",
                    under.translated (0.0f, 14.0f), juce::Justification::centred);
        g.setColour (earlyColour);
        g.drawText ("sound", juce::Rectangle<float> (60.0f, 14.0f).withCentre (source.translated (0.0f, -14.0f)), juce::Justification::centred);
        g.setColour (juce::Colours::white);
        g.drawText ("you", juce::Rectangle<float> (60.0f, 14.0f).withCentre (listener.translated (0.0f, 14.0f)), juce::Justification::centred);

        if (since >= 0.0 && fade > 0.01)
        {
            g.setColour (theme::textDim);
            g.drawText ("slowed down 20 times", bounds.withHeight (14.0f), juce::Justification::centredRight);
        }
    }
}

//==============================================================================
namespace
{
    void drawCaption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& left, const juce::String& right = {})
    {
        auto line = r.removeFromTop (18.0f);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::text);
        g.drawText (left, line, juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.drawText (right, line, juce::Justification::centredRight, true);
        r.removeFromTop (6.0f);
    }

    /** A box with a name, for the diagram of how the reverb works. */
    juce::Rectangle<float> drawBox (juce::Graphics& g, juce::Rectangle<float> box, const juce::String& name, const juce::String& detail, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.18f));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (c);
        g.drawRoundedRectangle (box, 5.0f, 1.2f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (theme::text);
        g.drawText (name, box.withTrimmedBottom (detail.isNotEmpty() ? box.getHeight() / 2.0f - 2.0f : 0.0f), juce::Justification::centred);

        if (detail.isNotEmpty())
        {
            g.setFont (juce::FontOptions (10.5f));
            g.setColour (theme::textDim);
            g.drawFittedText (detail, box.withTrimmedTop (box.getHeight() / 2.0f - 2.0f).reduced (4.0f, 2.0f).toNearestInt(), juce::Justification::centredTop, 2);
        }

        return box;
    }

    void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow ({ from, to }, 1.2f, 7.0f, 7.0f);
    }

    /** Where the response of the reverb's settings is drawn, kept so it is computed once per change. */
    struct Pictures
    {
        ResponseCache current;
        std::array<ResponseCache, 6> kinds;
        juce::Image spectrogram;
        verb::Settings spectrogramFor;
        bool hasSpectrogram = false;
    };

    /** The response's level per time and frequency: the highs dying away sooner with damping. */
    juce::Image makeSpectrogram (const verb::Settings& settings, double seconds)
    {
        constexpr int order = 9, size = 1 << order, hop = size / 2, rows = 120;
        auto s = settings;
        s.predelayMs = 0.0f;
        const auto response = verb::renderImpulse (s, renderRate, seconds, false, true);
        const int columns = juce::jmax (1, (response.getNumSamples() - size) / hop);
        juce::Image image (juce::Image::RGB, columns, rows, true);
        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann, false);
        std::vector<float> data (2 * size);
        float loudest = -200.0f;
        std::vector<float> levels ((size_t) (columns * rows));

        for (int c = 0; c < columns; ++c)
        {
            std::fill (data.begin(), data.end(), 0.0f);

            for (int i = 0; i < size; ++i)
                data[(size_t) i] = 0.5f * (response.getSample (0, c * hop + i) + response.getSample (1, c * hop + i));

            window.multiplyWithWindowingTable (data.data(), size);
            fft.performFrequencyOnlyForwardTransform (data.data(), true);

            for (int row = 0; row < rows; ++row)
            {
                // Rows from 12 kHz at the top to 60 Hz at the bottom, evenly in octaves.
                const double hz = 60.0 * std::pow (200.0, (rows - 1 - row) / (double) (rows - 1));
                const int bin = juce::jlimit (1, size / 2 - 1, (int) std::round (hz * size / renderRate));
                const float db = juce::Decibels::gainToDecibels (data[(size_t) bin] + 1.0e-9f);
                levels[(size_t) (row * columns + c)] = db;
                loudest = juce::jmax (loudest, db);
            }
        }

        for (int c = 0; c < columns; ++c)
        {
            for (int row = 0; row < rows; ++row)
            {
                const float v = juce::jlimit (0.0f, 1.0f, 1.0f + (levels[(size_t) (row * columns + c)] - loudest) / 70.0f);
                image.setPixelAt (c, row, juce::Colour (0xff15171b).interpolatedWith (colour, v).interpolatedWith (juce::Colours::white, v * v * 0.5f));
            }
        }

        return image;
    }
}

void drawKind (juce::Graphics& g, juce::Rectangle<float> r, const verb::Settings& s, double since, bool labels)
{
    if (s.type == verb::Type::digital || s.type == verb::Type::room)
    {
        drawRoom (g, r, s, since, labels);
        return;
    }

    const double animation = since >= 0.0 && since < 12.0 ? since : std::fmod (juce::Time::getMillisecondCounterHiRes() / 1000.0, 5.0);
    auto area = r.reduced (22.0f, 26.0f);
    g.setFont (juce::FontOptions (11.0f));
    auto label = [&] (const juce::String& text, juce::Point<float> position)
    {
        if (! labels) return;
        g.setColour (theme::text);
        g.drawFittedText (text, juce::Rectangle<int> (160, 16).withCentre (position.toInt()), juce::Justification::centred, 1);
    };
    auto dot = [&] (juce::Point<float> position, juce::Colour tint)
    {
        g.setColour (tint);
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (position));
    };

    if (s.type == verb::Type::spring)
    {
        g.setColour (theme::textDim);
        g.drawRoundedRectangle (area, 6.0f, 2.0f);
        const float left = area.getX() + 30.0f, right = area.getRight() - 30.0f;
        for (int coil = 0; coil < 2; ++coil)
        {
            const float y = area.getY() + area.getHeight() * (coil == 0 ? 0.35f : 0.68f);
            juce::Path spring;
            spring.startNewSubPath (left, y);
            for (int n = 1; n <= 240; ++n)
            {
                const float x = left + (right - left) * (float) n / 240.0f;
                spring.lineTo (x, y + 9.0f * std::sin ((float) n * juce::MathConstants<float>::twoPi / 12.0f));
            }
            g.setColour (colour);
            g.strokePath (spring, juce::PathStrokeType (1.5f));
            const double roundTrip = verb::kinds::springRoundTripMs (s.sizePercent) * 0.020 * (coil == 0 ? 1.0 : 1.12);
            const double trips = animation / roundTrip;
            const double phase = std::fmod (trips, 1.0);
            const float along = (float) (phase < 0.5 ? phase * 2.0 : 2.0 - phase * 2.0);
            const float radius = (float) (5.0 * std::pow (1.3, std::min (trips, 5.0)));
            g.setColour (earlyColour.withAlpha ((float) std::pow (0.6, std::floor (trips))));
            g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, 22.0f).withCentre ({ left + along * (right - left), y }));
            dot ({ left, y }, earlyColour);
            dot ({ right, y }, theme::text);
        }
        label ("driver", { left, area.getY() - 12.0f });
        label ("pickup", { right, area.getY() - 12.0f });
        if (labels)
        {
            g.setColour (theme::textDim);
            g.drawFittedText ("Pulse: 20x slower; each trip wider and fainter", r.withTop (r.getBottom() - 17.0f).toNearestInt(), juce::Justification::centred, 1);
        }
        return;
    }

    if (s.type == verb::Type::plate)
    {
        auto sheet = area.reduced (24.0f, 6.0f);
        g.setColour (theme::textDim.withAlpha (0.25f));
        g.fillRect (sheet);
        g.setColour (theme::textDim);
        g.drawRect (sheet, 1.5f);
        for (const float x : { sheet.getX(), sheet.getRight() })
            for (const float y : { sheet.getY(), sheet.getBottom() })
            {
                juce::Path suspension;
                suspension.startNewSubPath (x, y);
                for (int n = 1; n <= 6; ++n)
                    suspension.lineTo (x + (n % 2 == 0 ? -4.0f : 4.0f), y + (y < sheet.getCentreY() ? -1.0f : 1.0f) * (float) n * 3.0f);
                g.strokePath (suspension, juce::PathStrokeType (1.0f));
            }
        const auto driver = sheet.getRelativePoint (0.32f, 0.48f);
        const juce::Rectangle<float> damper (sheet.getX() + sheet.getWidth() * 0.65f, sheet.getY() + 5.0f, 18.0f, sheet.getHeight() - 10.0f);
        g.setColour (theme::textDim.withAlpha (0.5f));
        g.fillRect (damper);
        {
            const juce::Graphics::ScopedSaveState saved (g);
            g.reduceClipRegion (sheet.toNearestInt());
            for (const double hz : { 100.0, 1000.0, 10000.0 })
            {
                const float radius = (float) (verb::kinds::plateGroupVelocity (hz) / 100.0 * animation / 2.0) * sheet.getWidth();
                g.setColour ((hz > 9000.0 ? earlyColour : hz > 900.0 ? colour : theme::accent).withAlpha (0.8f));
                g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (driver), 1.6f);
            }
        }
        dot (driver, earlyColour);
        dot (sheet.getRelativePoint (0.80f, 0.25f), theme::text);
        dot (sheet.getRelativePoint (0.80f, 0.78f), theme::text);
        label ("driver", { driver.x, sheet.getY() - 14.0f });
        label ("two pickups", { sheet.getRight(), sheet.getY() - 14.0f });
        label ("damper", { damper.getCentreX(), sheet.getBottom() + 14.0f });
        if (labels)
        {
            g.setColour (theme::textDim);
            g.drawFittedText ("10 kHz (orange), 1 kHz (green), 100 Hz (blue): 100x slower", r.withTop (r.getBottom() - 17.0f).toNearestInt(), juce::Justification::centred, 1);
        }
        return;
    }

    const bool hall = s.type == verb::Type::hall;
    auto floor = area.withSizeKeepingCentre (juce::jmin (area.getWidth(), area.getHeight() * (hall ? 2.0f : 1.6f)), area.getHeight());
    juce::Path walls;
    walls.startNewSubPath (floor.getX(), floor.getY());
    walls.lineTo (floor.getRight(), floor.getY() + (hall ? 0.0f : floor.getHeight() * 0.15f));
    walls.lineTo (floor.getRight() - (hall ? 0.0f : floor.getWidth() * 0.12f), floor.getBottom());
    walls.lineTo (floor.getX(), floor.getBottom() - (hall ? 0.0f : floor.getHeight() * 0.12f));
    walls.closeSubPath();
    g.setColour (theme::textDim);
    g.strokePath (walls, juce::PathStrokeType (2.0f));
    const auto source = floor.getRelativePoint (hall ? 0.12f : 0.2f, 0.4f);
    const auto listener = floor.getRelativePoint (hall ? 0.58f : 0.8f, 0.65f);
    if (hall)
    {
        g.setColour (theme::textDim.withAlpha (0.3f));
        g.fillRect (floor.withWidth (floor.getWidth() * 0.2f));
        label ("stage", { source.x, floor.getBottom() + 13.0f });
    }
    dot (source, earlyColour);
    dot (listener, theme::text);
    if (! hall)
    {
        g.setColour (earlyColour);
        g.drawArrow ({ source, source.translated (-22.0f, 0.0f) }, 1.3f, 6.0f, 6.0f);
    }
    label (hall ? "sound" : "speaker", { source.x, source.y - 16.0f });
    label (hall ? "listener" : "microphone", { listener.x, listener.y + 16.0f });
    const float radius = (float) (animation * 343.0 / 20.0 / verb::getRoomMetres (s.sizePercent)) * floor.getWidth();
    {
        const juce::Graphics::ScopedSaveState saved (g);
        g.reduceClipRegion (walls);
        // Images of images show later bounces too, so the small chamber keeps moving through its tail.
        for (int horizontal = -8; horizontal <= 8; ++horizontal)
            for (int vertical = -8; vertical <= 8; ++vertical)
            {
                const float localX = source.x - floor.getX(), localY = source.y - floor.getY();
                const float imageX = floor.getX() + (float) horizontal * floor.getWidth()
                                   + (horizontal % 2 == 0 ? localX : floor.getWidth() - localX);
                const float imageY = floor.getY() + (float) vertical * floor.getHeight()
                                   + (vertical % 2 == 0 ? localY : floor.getHeight() - localY);
                const int reflections = std::abs (horizontal) + std::abs (vertical);
                g.setColour (colour.withAlpha (0.65f * std::pow (0.87f, (float) reflections)));
                g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ imageX, imageY }), 1.2f);
            }
    }
    for (int ray = 0; ray < 3; ++ray)
    {
        const auto bounce = floor.getRelativePoint (0.35f + (float) ray * 0.18f, ray % 2 == 0 ? 0.0f : 1.0f);
        const double rayMs = verb::getFirstReflectionMs (s.sizePercent) * (ray == 0 ? 1.0 : ray == 1 ? 1.6 : 2.3);
        const double arrival = rayMs * 20.0 / 1000.0;
        const bool arriving = animation >= arrival && animation < arrival + 0.4;
        g.setColour (earlyColour.withAlpha (arriving ? 1.0f : 0.22f));
        g.drawLine ({ source, bounce });
        g.drawLine ({ bounce, listener });
        label (formatMs (rayMs), { bounce.x, bounce.y + (ray % 2 == 0 ? -11.0f : 11.0f) });
    }
}

namespace
{
    /** The first 100 ms, enlarged so individual reflections can be compared. */
    void drawDensityStrip (juce::Graphics& g, juce::Rectangle<float> r, const Response& response)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.0f));
        juce::String densityLabel = "first 100 ms";
        if (response.settings.type == verb::Type::room || response.settings.type == verb::Type::chamber)
            densityLabel << " (1,000/s at 11 ms)";
        else if (response.settings.type == verb::Type::hall)
            densityLabel << " (1,000/s at 172 ms)";
        g.drawText (densityLabel, r.removeFromTop (13.0f), juce::Justification::centredLeft);
        g.setColour (earlyColour);
        for (const auto& [milliseconds, magnitude] : response.arrivals)
        {
            const float x = r.getX() + milliseconds / 100.0f * r.getWidth();
            const float height = juce::jlimit (0.08f, 1.0f, (juce::Decibels::gainToDecibels (magnitude) + 85.0f) / 85.0f) * r.getHeight();
            g.drawVerticalLine (juce::roundToInt (x), r.getBottom() - height, r.getBottom());
        }
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInReverb& reverb)
{
    auto pictures = std::make_shared<Pictures>();
    BuiltInReverb* processor = &reverb;
    std::vector<fxui::Guide::Page> pages;

    // Moving pictures play again every few seconds while nobody claps.
    auto loop = [] (double since, double period)
    {
        return since < 30.0 ? since : std::fmod (juce::Time::getMillisecondCounterHiRes() / 1000.0, period);
    };

    pages.push_back ({ "Sound in a room", "It bounces off every wall",
        "Clap your hands in a room. The sound spreads from your hands in all directions at 343 metres a second: the speed of "
        "sound. A little of it reaches your ears straight away: the direct sound. The rest hits the walls, the floor, the ceiling "
        "and the furniture, and bounces off them, the way a ball bounces off a wall. Some of that comes to your ears too, a little "
        "later, because it took a longer way.\n\n"
        "Then it bounces again, and again. Every bounce makes more reflections, coming from more directions, ever closer together, "
        "and every bounce takes a little of the sound's energy away: the wall soaks some of it up. After a few hundred bounces, "
        "the sound has died away.\n\n"
        "All those reflections together are reverberation, or reverb. You almost never hear a sound without it: every room, "
        "hall, stairwell and church has its own. Even outdoors there is a little, from the ground and buildings. Only a special "
        "room with walls that soak up everything (an anechoic chamber) has none, and people find it eerie: sound without a "
        "space feels wrong.\n\n"
        "The picture shows a room from above, slowed down 20 times. The white ring is the sound itself. The orange rings look as "
        "if they come from behind the walls: a reflection travels exactly as if it came from a mirror image of the sound on the "
        "other side of the wall. Mirror images of mirror images make the later reflections. Acousticians call this the image "
        "source method, and simple reverbs and room simulators use it.\n\n"
        "Click Clap to hear the room the reverb's knobs describe.",
        [processor, loop] (juce::Graphics& g, juce::Rectangle<float> r, double since)
        {
            drawCaption (g, r, "A clap in the room, seen from above", "the white line is the direct sound, orange the first reflections");
            drawRoom (g, r, processor->getSettings(), loop (since, 4.0), true);
        } });

    pages.push_back ({ "Direct, early, tail", "The three parts of every reverb",
        "What reaches your ears after a clap comes in three parts.\n\n"
        "1. The direct sound: the shortest way, so it arrives first and loudest. Your brain uses it to hear where the sound is: "
        "even in a very reverberant church you can point at the person speaking. This is the precedence effect: the first sound "
        "to arrive decides the direction.\n\n"
        "2. The early reflections: the first bounces off the nearest surfaces, in the first 10 to 100 milliseconds. They come too "
        "quickly to hear as separate echoes; instead, they tell your brain how big the room is and how close the walls are. A small "
        "room's early reflections come almost at once; a hall's come later and further apart. The gap between the direct sound and "
        "the first reflection is called the pre-delay.\n\n"
        "3. The tail (late reverberation): after a few dozen bounces, the reflections come from everywhere, thousands per second, far "
        "too dense to tell apart. They blend into a smooth wash that dies away evenly. Its length is the decay time.\n\n"
        "The picture is the reverb's impulse response with its knobs as they are now: what one very short click turns into. Change "
        "the pre-delay, size, early reflections and decay in the reverb's window and watch the parts move.",
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "What one clap turns into", "the dashed lines are the beats at " + formatBpm (songTempo (processor->tempoBpm.load())) + " BPM");
            drawResponse (g, r, pictures->current.get (s), songTempo (processor->tempoBpm.load()), getShownSeconds (s), true);
        } });

    pages.push_back ({ "Decay time", "How long a room rings",
        "The decay time is how long the tail takes to die away by 60 decibels: down to a millionth of its energy, which the ear hears "
        "as silence. Acousticians call it RT60.\n\n"
        "Two things decide it in a real room: its size and how much its surfaces soak up. In a big room sound travels further between "
        "bounces, so it bounces less often per second and loses its energy more slowly. Soft, porous things (curtains, carpet, sofas, "
        "people, clothes) soak sound up; hard, smooth things (stone, tiles, glass, concrete) reflect almost all of it.\n\n"
        "Wallace Sabine, a physics professor at Harvard, worked out the rule around 1900, while fixing a lecture hall nobody could "
        "understand a word in. He moved seat cushions in and out of the hall at night and timed how long an organ pipe rang:\n\n"
        "    RT60 = 0.161 x volume / absorption\n\n"
        "The volume in cubic metres; the absorption counts each surface's area times how much it soaks up, with an open window as 1 "
        "(nothing comes back). An empty living room of 60 m3 with 15 m2 of absorption: 0.161 x 60 / 15 = 0.64 s. A concert hall "
        "of 15,000 m3 with 1,200 m2: 2.0 s. An empty room rings longer than a furnished one, and an audience shortens a hall's decay.\n\n"
        "Typical decay times: car or vocal booth 0.2-0.4 s, living room 0.4-0.6 s, classroom 0.7-1 s, concert hall 1.8-2.2 s, "
        "stone church 2-4 s, cathedral 4-8 s.\n\n"
        "Music and speech need different rooms. Speech needs a short decay, or the syllables blur into each other. Orchestras and "
        "choirs want a long one, which blends the notes and makes them sound full; music by Bach and Gregorian chant was written for "
        "stone churches, and sounds thin in a dry room.",
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double seconds = juce::jmax (6.5, getShownSeconds (s));
            drawCaption (g, r, "Your decay (dashed) and real places", "how long until 60 dB quieter");
            const auto plot = drawResponse (g, r, pictures->current.get (s), 0.0, seconds, false);
            const std::pair<double, const char*> places[] = { { 0.5, "living room" }, { 2.0, "concert hall" }, { 6.0, "cathedral" } };
            const auto start = pictures->current.get (s).tailStartDb;

            g.setFont (juce::FontOptions (11.0f));

            for (auto& [decay, name] : places)
            {
                const auto end = plot.at (s.predelayMs / 1000.0 + decay, start - 60.0);
                g.setColour (earlyColour.withAlpha (0.8f));
                g.drawLine ({ plot.at (s.predelayMs / 1000.0, start), end }, 1.2f);
                g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (end));
                g.drawText (juce::String (name) + " " + juce::String (decay, 1) + " s", juce::Rectangle<float> (140.0f, 14.0f).withPosition (end.x + 6.0f, end.y - 7.0f),
                            juce::Justification::centredLeft);
            }
        } });

    pages.push_back ({ "Absorption and damping", "The highs die away first",
        "Rooms don't soak up all frequencies the same. Soft materials catch high frequencies far better than low ones: the short "
        "waves of a 5 kHz tone (7 cm) get lost in the fibres of a curtain, while the 3-metre waves of a 110 Hz tone pass right "
        "through it. The air itself also absorbs the highest frequencies over long distances. So with every bounce the sound gets "
        "a little darker, and in the tail the highs fade out first.\n\n"
        "That is why a reverb sounds natural when its highs die away sooner, and hard or metallic when they don't. A bathroom full of "
        "tiles rings bright; a wooden hall with velvet seats sounds warm; a church of stone sounds both long and fairly bright.\n\n"
        "Low frequencies are the opposite problem: they are hard to stop, ring long, and in a small room they build up at certain "
        "notes (room modes) where the room's size fits a whole number of half-waves. Studios put thick 'bass traps' in the corners "
        "against that. In a mix, reverb in the lows mostly sounds like mud, which is why reverbs have a low cut.\n\n"
        "The picture shows the reverb's tail by frequency: time left to right, low notes at the bottom, high at the top; the brighter, "
        "the louder. With damping on, the top fades out before the bottom. Turn the damping in the reverb's window and watch it.",
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double seconds = juce::jlimit (1.0, 10.0, s.decaySeconds * 1.1 + 0.1);

            if (! pictures->hasSpectrogram || s != pictures->spectrogramFor)
            {
                pictures->spectrogram = makeSpectrogram (s, seconds);
                pictures->spectrogramFor = s;
                pictures->hasSpectrogram = true;
            }

            drawCaption (g, r, "The tail by frequency, from the clap on", "damping " + verb::formatKnob (Knob::damping, s) + ", low cut " + verb::formatKnob (Knob::lowCut, s));
            auto area = r.withTrimmedLeft (52.0f).withTrimmedBottom (16.0f);
            g.drawImage (pictures->spectrogram, area, juce::RectanglePlacement::stretchToFit);

            g.setFont (juce::FontOptions (10.5f));
            auto toY = [&] (double hz) { return area.getBottom() - area.getHeight() * (float) (std::log (hz / 60.0) / std::log (200.0)); };

            for (const double hz : { 100.0, 300.0, 1000.0, 3000.0, 10000.0 })
            {
                g.setColour (theme::textDim);
                g.drawText (hz >= 1000.0 ? juce::String (hz / 1000.0, 0) + " kHz" : juce::String ((int) hz) + " Hz",
                            juce::Rectangle<float> (48.0f, 12.0f).withPosition (r.getX(), toY (hz) - 6.0f), juce::Justification::centredRight);
            }

            if (verb::isDampingOn (s.dampingHz) && s.dampingHz < 12000.0f)
            {
                const float dashes[] = { 4.0f, 4.0f };
                g.setColour (earlyColour);
                g.drawDashedLine ({ area.getX(), toY (s.dampingHz), area.getRight(), toY (s.dampingHz) }, dashes, 2, 1.0f);
                g.drawText ("damping", area.withHeight (12.0f).withY (toY (s.dampingHz) - 14.0f).reduced (6.0f, 0.0f), juce::Justification::centredRight);
            }

            g.setColour (theme::textDim);
            g.drawText ("0", area.withHeight (14.0f).withY (area.getBottom() + 2.0f), juce::Justification::centredLeft);
            g.drawText (fxui::formatSeconds (seconds), area.withHeight (14.0f).withY (area.getBottom() + 2.0f), juce::Justification::centredRight);
        } });

    pages.push_back ({ "Size, distance, pre-delay", "How the ear hears space",
        "From reverb alone, your ears can tell a lot: how big a room is, how far away a sound is, and even what the walls are made of.\n\n"
        "How big: the time until the first reflections arrive, and how far apart they are. In a small room the walls are 2-3 metres "
        "away: the first reflection is only a few milliseconds behind the direct sound. In a concert hall it is about 15-35 ms at a seat in the middle of the floor. That's the "
        "pre-delay: a longer pre-delay sounds like a bigger space, even with a short tail.\n\n"
        "How far away: the further a sound is from you, the quieter its direct sound gets, but the reverb stays about as loud, "
        "because it fills the whole room. So the balance between direct sound and reverb tells your brain the distance: a lot of "
        "reverb sounds far away, little reverb close. Far sounds also lose their highs and their early reflections come closer to the "
        "direct sound. This is how a mix gets depth: dry sounds are in front, wetter sounds further back.\n\n"
        "What the walls are like: how bright the tail stays (damping), and how even it is.\n\n"
        "The pictures show a small and a big room side by side, with the time to the first reflection under each.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "A small room and a big one", "the gap before the first reflection grows with the room");
            verb::Settings small, big;
            small.sizePercent = 15.0f;
            big.sizePercent = 95.0f;
            auto left = r.removeFromLeft (r.getWidth() / 2.0f).reduced (8.0f, 0.0f);
            auto right = r.reduced (8.0f, 0.0f);

            for (auto [area, s] : { std::pair { left, small }, std::pair { right, big } })
            {
                auto under = area.removeFromBottom (18.0f);
                drawRoom (g, area, s, -1.0, false);
                g.setColour (earlyColour);
                g.setFont (juce::FontOptions (12.0f));
                g.drawText ("first reflection after " + formatMs (verb::getFirstReflectionMs (s.sizePercent)), under, juce::Justification::centred);
            }
        } });

    struct AuditionSequence final : juce::Timer
    {
        BuiltInReverb& effect;
        int playing = -1;
        const std::array<verb::Type, 6> order { verb::Type::room, verb::Type::hall, verb::Type::chamber, verb::Type::plate, verb::Type::spring, verb::Type::digital };
        explicit AuditionSequence (BuiltInReverb& processorToPlay) : effect (processorToPlay) {}
        void start()
        {
            playing = 0;
            effect.requestAudition (order[0]);
            startTimer (3000);
        }
        void timerCallback() override
        {
            ++playing;
            if (playing >= 6)
            {
                playing = -1;
                stopTimer();
                return;
            }
            effect.requestAudition (order[(size_t) playing]);
        }
    };
    auto sequence = std::make_shared<AuditionSequence> (reverb);
    pages.push_back ({ "Kinds of reverb", "Rooms, halls, plates, springs", verb::kinds::explain (verb::kinds::Topic::kinds),
        [pictures, sequence] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Six kinds, the same clap", "white: direct sound; orange: early reflections; green: tail");
            const float w = r.getWidth() / 3.0f, h = r.getHeight() / 2.0f;
            for (int index = 0; index < 6; ++index)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (index % 3), r.getY() + h * (float) (index / 3), w, h).reduced (5.0f, 3.0f);
                if (sequence->playing == index)
                {
                    g.setColour (theme::accent);
                    g.drawRoundedRectangle (cell, 4.0f, 2.0f);
                }
                cell.reduce (5.0f, 3.0f);
                const auto kind = sequence->order[(size_t) index];
                const auto& preset = verb::kinds::getShowcase (kind);
                auto settings = preset.settings;
                settings.mixPercent = 50.0f;
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                g.drawText (verb::kinds::getName (kind), cell.removeFromTop (16.0f), juce::Justification::centredLeft);
                g.setFont (juce::FontOptions (10.5f));
                g.setColour (theme::textDim);
                g.drawText (preset.name, cell.removeFromTop (14.0f), juce::Justification::centredLeft);
                const auto& responseData = pictures->kinds[(size_t) index].get (settings);
                auto density = cell.removeFromBottom (31.0f);
                drawResponse (g, cell, responseData, 0.0, 4.0, false);
                drawDensityStrip (g, density, responseData);
            }
        }, [sequence] { sequence->start(); }, "Hear each in turn", "Plays one clap through each kind, 3 seconds apart, in the order shown." });

    for (const auto kind : { verb::Type::room, verb::Type::hall, verb::Type::chamber, verb::Type::plate, verb::Type::spring })
    {
        const auto& preset = verb::kinds::getShowcase (kind);
        auto settings = preset.settings;
        settings.mixPercent = 50.0f;
        pages.push_back ({ verb::kinds::getName (kind), verb::kinds::getSummary (kind), verb::kinds::explain (kind),
            [settings, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double since)
            {
                auto responseArea = r.removeFromBottom (112.0f);
                auto strip = responseArea.removeFromBottom (32.0f);
                drawKind (g, r, settings, since, true);
                const auto& data = pictures->kinds[(size_t) settings.type].get (settings);
                drawResponse (g, responseArea, data, 0.0, getShownSeconds (settings), false);
                drawDensityStrip (g, strip, data);
            }, [processor, kind] { processor->requestAudition (kind); }, "Clap: " + verb::kinds::getName (kind).toLowerCase(),
            "Plays a clap through a " + verb::kinds::getName (kind).toLowerCase() + " (the " + preset.name + " starting point), without changing your reverb." });
    }
    pages.push_back ({ "Digital and convolution", "From circuits to captured spaces", verb::kinds::explain (verb::Type::digital),
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const std::array<std::pair<const char*, const char*>, 10> milestones {{
                { "1930s", "Abbey Road chamber" }, { "1941", "Hammond's spring patent" }, { "1947", "Peg o' My Heart" },
                { "1956", "Capitol's chambers" }, { "1957", "EMT 140" }, { "1961", "Fender Reverb; EMT 140S" },
                { "1962", "Schroeder" }, { "1976", "EMT 250" }, { "1978", "Lexicon 224" }, { "1991", "Jot's FDN" } }};
            const float row = r.getHeight() / (float) milestones.size();
            g.setFont (juce::FontOptions (12.0f));
            for (const auto& [year, item] : milestones)
            {
                auto line = r.removeFromTop (row);
                g.setColour (earlyColour);
                g.drawText (year, line.removeFromLeft (75.0f), juce::Justification::centredLeft);
                g.setColour (theme::text);
                g.drawText (item, line, juce::Justification::centredLeft);
            }
        }, [processor] { processor->requestAudition (verb::Type::digital); }, "Clap: digital", "Plays a clap through Digital (the Neutral start starting point), without changing your reverb." });
    pages.push_back ({ "Reverb effects", "Gated, reverse, shimmer, moving tails", verb::kinds::explain (verb::kinds::Topic::effects),
        [] (juce::Graphics& g, juce::Rectangle<float> r, double since)
        {
            const char* names[] = { "Gated", "Reverse", "Shimmer", "Modulated" };
            for (int effect = 0; effect < 4; ++effect)
            {
                auto cell = juce::Rectangle<float> (r.getX() + r.getWidth() * 0.5f * (float) (effect % 2), r.getY() + r.getHeight() * 0.5f * (float) (effect / 2), r.getWidth() * 0.5f, r.getHeight() * 0.5f).reduced (12.0f, 8.0f);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.0f));
                g.drawText (names[effect], cell.removeFromTop (18.0f), juce::Justification::centredLeft);
                ResponsePlot plot { cell.reduced (2.0f, 14.0f), 2.0 };
                juce::Path curve;
                for (int sample = 0; sample <= 200; ++sample)
                {
                    const double time = (double) sample / 100.0;
                    double db = -30.0 * time;
                    if (effect == 0) db = time < 0.30 ? -8.0 : time < 0.32 ? -8.0 - (time - 0.30) * 2600.0 : -90.0;
                    if (effect == 1) db = time <= 1.0 ? -60.0 + 54.0 * time : -90.0;
                    const auto point = plot.at (time, db);
                    if (sample == 0) curve.startNewSubPath (point); else curve.lineTo (point);
                }
                g.setColour (colour);
                g.strokePath (curve, juce::PathStrokeType (2.0f));
                if (effect >= 2)
                {
                    juce::Path extra;
                    for (int n = 0; n <= 160; ++n)
                    {
                        const double time = 0.2 + (double) n / 100.0;
                        const double db = effect == 2 ? (time < 0.8 ? -30.0 + (time - 0.2) * 25.0 : -15.0 - (time - 0.8) * 20.0) : -35.0 + 8.0 * std::sin (juce::MathConstants<double>::pi * time);
                        const auto point = plot.at (time, db);
                        if (n == 0) extra.startNewSubPath (point); else extra.lineTo (point);
                    }
                    g.setColour (earlyColour);
                    g.strokePath (extra, juce::PathStrokeType (1.5f));
                    g.drawText (effect == 2 ? "octave-up band" : "pitch: +/-0.3%, 0.5 Hz", cell.removeFromBottom (14.0f), juce::Justification::centred);
                }
                if (since >= 0.0 && since < 2.0)
                {
                    g.setColour (theme::accent.withAlpha (0.5f));
                    g.drawVerticalLine (juce::roundToInt (plot.toX (since)), plot.area.getY(), plot.area.getBottom());
                }
                g.setColour (theme::textDim);
                g.drawText ("0                                       2 s", cell.withHeight (14.0f).withY (cell.getBottom()), juce::Justification::centred);
            }
        } });

    pages.push_back ({ "Reverb in a mix", "Space, depth and clarity",
        "In a mix, reverb does three jobs: it puts dry recordings in a space, so they sound real; it gives the mix depth, with some "
        "sounds in front and others behind; and it glues the parts together, as if the band played in one room.\n\n"
        "When it's used:\n\n"
        "- Vocals: a plate or hall of 1-2 s, with pre-delay, to put the voice in a space without washing out the words.\n"
        "- Drums: a short room for realism; a big plate or hall on the snare for the size of 1980s ballads; little on the kick.\n"
        "- Guitars and keys: a room or a medium hall, more on clean parts, less on distorted ones, which are dense already.\n"
        "- Strings, pads, choirs and piano ballads: long halls, the spaces they were written for.\n"
        "- Bass: none, or very little.\n"
        "- As an effect: a huge tail on one word or the last chord, or a reverb swelling into a drop.\n\n"
        "How it's used:\n\n"
        "- Less than you think. Turn the mix up until you hear the reverb clearly, then back until you just stop noticing it. Switch "
        "it off: if the sound gets thinner and drier, the reverb is doing its job.\n\n"
        "- Keep the lows dry. Cut the reverb's lows (150-300 Hz on most things), and give bass and kick little or no reverb.\n\n"
        "- Pre-delay for clarity. 30-100 ms on vocals keeps the words in front of the reverb.\n\n"
        "- Fit the tempo. A tail that dies away before the next beat or phrase keeps the music clear; at 120 BPM a beat is 0.5 s, a "
        "bar 2 s.\n\n"
        "- Depth by distance. What should be up front (the lead vocal, the main instrument) gets less reverb, or a shorter one; what "
        "should be behind gets more. Pads and strings can be far back.\n\n"
        "- One or two spaces. Many very different reverbs make the mix sound like it was recorded in several places at once. Using "
        "similar settings on the tracks that should belong together helps.\n\n"
        "- Check in mono. Wide reverbs can sound thinner or louder when left and right are added together, as on many phone and "
        "bluetooth speakers.\n\n"
        "In most mixes one reverb sits on a return track and every track sends some of itself to it, so all share one room and "
        "the computer runs only one reverb. In DawIt: Track > New Return Track > Reverb, then turn up the Reverb knob on each "
        "mixer strip. A send of -10 dB gives about as much reverb as a 20% mix on the track. Put a reverb on the track itself "
        "when only that track needs it, or for a special effect.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Depth: more reverb sounds further away", "seen from the listener, as on a stage");
            auto stage = r.reduced (20.0f, 6.0f);
            juce::Path floor;
            floor.startNewSubPath (stage.getX(), stage.getBottom());
            floor.lineTo (stage.getRight(), stage.getBottom());
            floor.lineTo (stage.getRight() - stage.getWidth() * 0.2f, stage.getY());
            floor.lineTo (stage.getX() + stage.getWidth() * 0.2f, stage.getY());
            floor.closeSubPath();
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.fillPath (floor);

            struct Place { const char* name; float depth, across; int mix; };
            const Place places[] = { { "lead vocal", 0.12f, 0.5f, 15 }, { "drums", 0.45f, 0.5f, 20 }, { "guitar", 0.38f, 0.22f, 22 },
                                     { "piano", 0.42f, 0.78f, 25 }, { "strings", 0.8f, 0.35f, 35 }, { "pad", 0.88f, 0.68f, 45 } };

            for (auto& p : places)
            {
                const float y = stage.getBottom() - p.depth * stage.getHeight();
                const float inset = stage.getWidth() * 0.2f * p.depth;
                const float x = stage.getX() + inset + (stage.getWidth() - 2.0f * inset) * p.across;
                const float size = 26.0f * (1.0f - 0.55f * p.depth);
                g.setColour (colour.withAlpha (0.15f + 0.5f * p.depth));
                g.fillEllipse (juce::Rectangle<float> (size * 2.4f, size * 2.4f).withCentre ({ x, y }));
                g.setColour (juce::Colours::white.withAlpha (1.0f - 0.5f * p.depth));
                g.fillEllipse (juce::Rectangle<float> (size, size).withCentre ({ x, y }));
                g.setFont (juce::FontOptions (11.5f));
                g.drawText (juce::String (p.name) + ", " + juce::String (p.mix) + "% reverb",
                            juce::Rectangle<float> (150.0f, 14.0f).withCentre ({ x, y + size * 1.2f + 6.0f }), juce::Justification::centred);
            }

            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (11.0f));
            g.drawText ("front: dry and close", stage.withTop (stage.getBottom() - 14.0f), juce::Justification::centredLeft);
            g.drawText ("back: more reverb, far away", stage.withHeight (14.0f), juce::Justification::centred);
        } });

    pages.push_back ({ "How DawIt Reverb works", "Delay lines feeding each other",
        "DawIt Reverb is an algorithmic reverb: it doesn't model a real room, but builds what a room does out of simple parts.\n\n"
        "1. Pre-delay: the input is kept in a memory (a delay line) and read out later.\n\n"
        "2. Early reflections: the same memory is read at twelve moments after that, each a little quieter, some on the left and "
        "some on the right. The size spreads them out: in a bigger room they come later.\n\n"
        "3. Diffusion: four all-pass filters smear each sound out over a few milliseconds, like a rough wall scatters it, so the tail "
        "starts dense instead of as single echoes.\n\n"
        "4. The tail: eight delay lines between 6 and 80 ms long (longer in a bigger room), with lengths that share no common "
        "factor, so their echoes rarely land together. What comes out of each line is mixed into all eight again through a "
        "Hadamard matrix (adding and subtracting them, without gaining or losing energy) and fed back in. Every trip around makes "
        "more echoes from more 'directions': a feedback delay network, the design of Jean-Marc Jot from 1991 that most digital "
        "reverbs build on.\n\n"
        "5. Decay: on each trip, a line keeps just enough of the sound to die away by 60 dB in the decay time: a 50 ms line with a "
        "2 s decay keeps 10^(-3 x 0.05 / 2), 84% of its level, per trip.\n\n"
        "6. Damping: a gentle low-pass filter in each line takes a little more off the highs on every trip, so at the damping "
        "frequency the tail dies away twice as fast.\n\n"
        "7. Low cut, width and mix: the lows are cut from the reverb, its left and right are made more alike for less width, and it "
        "is mixed with the dry sound.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "From the dry sound to the reverb", {});
            const float h = 46.0f, gap = 16.0f;
            const float col = (r.getWidth() - 4.0f * gap) / 5.0f;
            const float midY = r.getCentreY();
            auto box = [&] (int column, float y, float height = 46.0f) { return juce::Rectangle<float> (r.getX() + (col + gap) * (float) column, y - height / 2.0f, col, height); };

            const auto in = drawBox (g, box (0, midY), "Sound in", "dry", theme::textDim);
            const auto pre = drawBox (g, box (1, midY), "Pre-delay", "a memory, read later", theme::accent);
            const auto early = drawBox (g, box (2, midY - 70.0f), "Early reflections", "12 single echoes", earlyColour);
            const auto diffuse = drawBox (g, box (2, midY + 30.0f), "Diffusion", "4 all-passes", colour);
            const auto tank = drawBox (g, box (3, midY + 30.0f, 92.0f), "8 delay lines", "mixed and fed back: decay and damping on every trip", colour);
            const auto out = drawBox (g, box (4, midY - 70.0f + h / 2.0f + 18.0f, 82.0f), "Low cut, width, mix", "with the dry sound", theme::accent);

            arrow (g, { in.getRight(), midY }, { pre.getX(), midY });
            arrow (g, { pre.getRight(), midY }, { early.getX(), early.getCentreY() });
            arrow (g, { pre.getRight(), midY }, { diffuse.getX(), diffuse.getCentreY() });
            arrow (g, { diffuse.getRight(), diffuse.getCentreY() }, { tank.getX(), diffuse.getCentreY() });
            arrow (g, { early.getRight(), early.getCentreY() }, { out.getX(), out.getY() + 16.0f });
            arrow (g, { tank.getRight(), tank.getCentreY() }, { out.getX(), out.getBottom() - 16.0f });

            // The feedback loop under the lines.
            juce::Path loopPath;
            loopPath.startNewSubPath (tank.getRight() - 12.0f, tank.getBottom());
            loopPath.quadraticTo (tank.getCentreX(), tank.getBottom() + 34.0f, tank.getX() + 12.0f, tank.getBottom());
            g.setColour (colour);
            g.strokePath (loopPath, juce::PathStrokeType (1.4f));
            g.drawArrow ({ tank.getX() + 16.0f, tank.getBottom() + 6.0f, tank.getX() + 12.0f, tank.getBottom() }, 1.4f, 7.0f, 7.0f);
            g.setFont (juce::FontOptions (10.5f));
            g.drawText ("round and round", juce::Rectangle<float> (tank.getWidth(), 14.0f).withPosition (tank.getX(), tank.getBottom() + 22.0f), juce::Justification::centred);

            // The dry sound, around everything to the mix.
            juce::Path dry;
            const float under = r.getBottom() - 16.0f;
            dry.startNewSubPath (in.getCentreX(), in.getBottom());
            dry.lineTo (in.getCentreX(), under);
            dry.lineTo (out.getCentreX(), under);
            g.setColour (theme::textDim);
            g.strokePath (dry, juce::PathStrokeType (1.2f));
            g.drawArrow ({ out.getCentreX(), under, out.getCentreX(), out.getBottom() }, 1.2f, 7.0f, 7.0f);
            g.drawText ("the dry sound", juce::Rectangle<float> (out.getCentreX() - in.getCentreX(), 14.0f).withPosition (in.getCentreX(), under - 16.0f),
                        juce::Justification::centred);
        } });

    pages.back().text += "\n\n" + juce::String::fromUTF8 ("8. Type: the same parts, set up differently. Room: the early reflections close together, the tail right after them. Hall: a few far-apart early reflections, and the tail fed from them, so it swells over the first tenth of a second. Chamber: the early reflections smeared by more diffusion, so they melt into the tail. Plate: no early reflections, and more diffusion before the lines, so the tail is dense from the first millisecond. Spring: a different circuit. For each side there is one loop as long as a trip along the spring and back, with a chain of all-pass filters in it that delays some frequencies more than others, which turns each repeat into a chirp (the method of Välimäki, Parker and Abel, 2010). The right spring is 12% longer than the left. Digital: the parts exactly as described above.");
    auto advice = [processor]
    {
        const auto host = fxui::getHostTrack (*processor);
        return verb::kinds::analyse (host.name, host.isReturn, processor->getSettings(), processor->tempoBpm.load()).joinIntoString ("\n\n");
    };
    pages.push_back ({ "Your track", "Which kind suits your track", verb::kinds::explain (verb::kinds::Topic::yourTrack),
        [processor, pictures] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto host = fxui::getHostTrack (*processor);
            const auto settings = processor->getSettings();
            drawCaption (g, r, (host.name.isEmpty() ? "Stand-in reverb" : host.name) + ": " + verb::kinds::getName (settings.type), "The response with your current settings");
            drawResponse (g, r, pictures->current.get (settings), processor->tempoBpm.load(), getShownSeconds (settings), true);
        }, {}, {}, {}, [advice] { return verb::kinds::explain (verb::kinds::Topic::yourTrack) + "\n\n" + advice(); } });
    return pages;
}
} // namespace daw::verbtext
