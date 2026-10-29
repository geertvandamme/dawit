#include "DelayGuide.h"
#include "engine/Tempo.h"

namespace daw::dlytext
{
using dly::Knob;

namespace
{
    juce::String formatMs (double ms)
    {
        if (ms >= 1000.0)
            return juce::String (ms / 1000.0, 2) + " s";

        return fx::formatNumber (ms, ms < 10.0 ? 1 : 0) + " ms";
    }

    juce::String formatMetres (double m)
    {
        return fx::formatNumber (m, m < 10.0 ? 1 : 0) + " m";
    }

    /** A place with a wall about this far away. */
    juce::String getPlaceFor (double metres)
    {
        if (metres < 5.0) return "the other side of a small room";
        if (metres < 15.0) return "the back wall of a hall";
        if (metres < 40.0) return "a building across the street";
        if (metres < 120.0) return "the far stand of a stadium";
        if (metres < 400.0) return "a cliff across a valley";
        return "the far side of a canyon";
    }

    constexpr double viewRate = 48000.0;
}

juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::time:     return "Time";
        case (int) Knob::feedback: return "Feedback";
        case (int) Knob::highCut:  return "High cut";
        case (int) Knob::lowCut:   return "Low cut";
        case (int) Knob::mode:     return "Stereo or ping-pong";
        case (int) Knob::mix:      return "Mix";
        case echoes:               return "The echoes of one clap";
        case wall:                 return "The wall";
        case sync:                 return "Sync to the tempo";
        case fxui::overview:       return "DawIt Delay";
        case fxui::presets:        return "Starting points";
        case fxui::clap:           return "Clap";
        case fxui::listen:         return "Hear only the echoes";
        case fxui::guide:          return "What is delay?";
        case fxui::history:        return "The last 4 seconds";
        default:                   return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::time:
            return "How long after the sound its echo comes. The delay keeps everything it hears in a memory and plays it back "
                   "this much later.\n\n"
                   "Followed to the tempo (Sync on), the time is a note value: the echoes land on the song's grid, so they become "
                   "part of the rhythm. Free (Sync off), it's a time in milliseconds, for effects that shouldn't follow the beat: "
                   "a quick slapback, a doubling, a widening.\n\n"
                   "Very roughly: under 30 ms you don't hear an echo but a thicker or wider sound; 50-150 ms is a quick 'slap'; "
                   "longer times are clear, separate echoes. Changing the time while it plays glides to the new time, which bends "
                   "the pitch of the echoes like a tape machine.";
        case (int) Knob::feedback:
            return "How much of each echo is fed back in to make the next one. Without feedback there's one echo. At 50%, each "
                   "echo is half as loud as the one before (6 dB quieter), so about ten are heard; at 80% they keep going for "
                   "dozens. At 100% they would never stop, and above it they would grow louder and louder: the runaway, "
                   "screaming echoes of dub and psychedelic music. DawIt Delay stops at 95%, where they still die away.\n\n"
                   "More feedback makes the echoes fill the gaps and blur together; less keeps them clean and rhythmic.";
        case (int) Knob::highCut:
            return "Takes the highs off the echoes, and because every echo goes round again, each repeat gets a little darker "
                   "than the one before. That is how tape and analog delays sounded, and why they are loved: the echoes fall back "
                   "behind the sound instead of competing with it, and a long trail of repeats melts into a soft wash.\n\n"
                   "Off, every repeat is as bright as the sound itself: clear and digital, but it can get harsh and busy.";
        case (int) Knob::lowCut:
            return "Takes the lows off the echoes, again on every repeat. Echoes of bass notes and kick drums pile up into "
                   "rumble and mud; thin echoes keep the low end clear while they still carry the rhythm and the melody.";
        case (int) Knob::mode:
            return "Stereo: what comes from the left echoes on the left, what comes from the right on the right. The echoes stay "
                   "where the sound is.\n\n"
                   "Ping-pong: the echoes bounce from side to side. The first comes from the left, the next from the right, then "
                   "the left again, while the sound itself stays where it was. It makes a sound wide and moving, and the echoes "
                   "easier to hear apart from it.";
        case (int) Knob::mix:
            return "How much of what comes out is echoes. 0% is only the dry sound, 100% only the echoes; in between the two "
                   "are mixed so the whole stays about as loud.\n\n"
                   "On an instrument, echoes usually sit a good deal under the sound; listen to them in the gaps of the playing, "
                   "where they should fill the space without crowding it.";
        case echoes:
            return "A clap and its echoes, left to right in time, as loud as they come out (the white bar is the clap). The top "
                   "lane is the left speaker, the bottom the right. Each echo is quieter than the one before by the feedback, and "
                   "darker by the filters: the duller its colour, the fewer highs it has left. The dashed lines are the beats of "
                   "the song, so you can see where the echoes fall in the groove.\n\n"
                   "Drag the first echo sideways for the time and the second up and down for the feedback; double-click to reset. "
                   "Click Clap to hear it.";
        case wall:
            return "How far away a wall would have to be to give this echo: the sound goes there and comes back, at 343 m/s. "
                   "The scale goes from a room to a canyon. Click Clap and watch the echoes come back to you as rings.";
        case sync:
            return "On: the time is a note value of the song's tempo, from 1/32 to a whole bar, with dotted notes (1.5 times as "
                   "long) and triplets (two thirds as long). The echoes stay in time, even when the tempo changes. Off: the time "
                   "is in milliseconds, whatever the tempo.";
        case fxui::presets:
            return "Settings for common uses, each explaining why every control is set the way it is. They are a start, not an "
                   "answer: play the track and adjust by ear, especially the mix and the feedback.";
        case fxui::clap:
            return "Claps your hands into the delay, the way you would to hear an echo. The clap is heard dry and with its echoes "
                   "at the mix, so change a control and clap again to compare. It works while the song is stopped.";
        case fxui::listen:
            return "Plays only the echoes, at full level, without the dry sound. Use it to hear what the echoes themselves sound "
                   "like: how many there are, how dark, how they bounce. Turn it off again to set the mix.";
        case fxui::guide:
            return "A guide to delay and echo, for beginners: echoes in the world, how a delay works, feedback, time and tempo, "
                   "short delays, the sound of tape echo, stereo, and using delay in a mix. The pictures follow the controls here.";
        case fxui::history:
            return "The last 4 seconds while the track plays: grey the track, purple the echoes as loud as they are mixed. The "
                   "lines are the beats. Echoes that fall in the gaps of the playing are heard; echoes on top of busy playing "
                   "mostly blur it.";
        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::time:
            return "15-35 ms, free: doubling and widening.\n"
                   "80-150 ms, free: slapback on vocals and guitar.\n"
                   "1/8 or 1/4: echoes in time on vocals and leads.\n"
                   "Dotted 1/8: rhythmic guitar patterns.\n"
                   "1/2 or a bar: long, spacious echoes.";
        case (int) Knob::feedback:
            return "0-10%: one echo (slapback, doubling).\n"
                   "20-40%: a few echoes, clean.\n"
                   "50-70%: a long trail, ambient.\n"
                   "80-95%: dub and effects; ride it.";
        case (int) Knob::highCut:
            return "2-4 kHz: dark, tape-like, far behind.\n"
                   "5-8 kHz: natural, a little softer than the sound.\n"
                   "Off: bright, digital, up front.";
        case (int) Knob::lowCut:
            return "100-200 Hz: most things.\n"
                   "300-500 Hz: thin, telephone-like echoes that stay out of the way.\n"
                   "Off: when the echoes should keep their weight.";
        case (int) Knob::mode:
            return "Stereo: echoes in place, most uses.\n"
                   "Ping-pong: wide, bouncing echoes on synths and keys; with no feedback and a short time, a widener.";
        case (int) Knob::mix:
            return "10-20%: echoes behind a vocal.\n"
                   "25-40%: echoes as part of the part (guitar patterns, synths).\n"
                   "40-50%: wide doubling.\n"
                   "100%: with Hear only the echoes, for a pure echo track.";
        default:
            return {};
    }
}

juce::String describeSettings (int t, const dly::Settings& s, double bpm)
{
    const double ms = dly::getDelayMs (s, bpm), tempo = dly::getTempo (bpm);
    const auto tempoText = formatBpm (tempo) + " BPM" + (bpm > 0.0 ? "" : " (until the song tells its tempo)");

    switch (t)
    {
        case (int) Knob::time:
        case sync:
        {
            if (s.sync)
                return dly::formatKnob (Knob::time, s) + " at " + tempoText + ": " + formatMs (ms) + ".";

            const int nearest = dly::findNearestNote (ms, tempo);
            return formatMs (ms) + ", not following the tempo. At " + tempoText + " the nearest note value is "
                   + dly::noteValues[nearest].name + " (" + formatMs (dly::getNoteMs (nearest, tempo)) + ").";
        }

        case (int) Knob::feedback:
        {
            const int repeats = dly::getAudibleRepeats (s.feedbackPercent);

            if (repeats <= 1)
                return dly::formatKnob (Knob::feedback, s) + ": one echo only.";

            return dly::formatKnob (Knob::feedback, s) + ": each echo is " + juce::String (-dly::getRepeatDb (s.feedbackPercent, 2), 1)
                   + " dB quieter than the one before; " + juce::String (repeats) + " echoes are heard before they're 60 dB down, over "
                   + formatMs (ms * repeats) + ".";
        }

        case (int) Knob::highCut:
            if (! dly::isHighCutOn (s.highCutHz))
                return "Off: every repeat is as bright as the sound.";

            return dly::formatKnob (Knob::highCut, s) + ": at 5 kHz the first echo has lost " + juce::String (-dly::getFilteredDb (s, 5000.0, 1, viewRate), 1)
                   + " dB, the third " + juce::String (-dly::getFilteredDb (s, 5000.0, 3, viewRate), 1) + " dB.";

        case (int) Knob::lowCut:
            if (! dly::isLowCutOn (s.lowCutHz))
                return "Off: the echoes keep all their lows.";

            return dly::formatKnob (Knob::lowCut, s) + ": at 60 Hz the first echo has lost " + juce::String (-dly::getFilteredDb (s, 60.0, 1, viewRate), 1)
                   + " dB, the third " + juce::String (-dly::getFilteredDb (s, 60.0, 3, viewRate), 1) + " dB.";

        case (int) Knob::mode:
            return s.mode == dly::Mode::pingPong ? "Ping-pong: the echoes go left, right, left..." : "Stereo: the echoes stay where the sound is.";

        case (int) Knob::mix:
        {
            const double db = fx::getWetToDryDb (s.mixPercent);

            if (db <= -99.0) return "0%: only the dry sound.";
            if (db >= 99.0) return "100%: only the echoes.";

            return dly::formatKnob (Knob::mix, s) + ": the first echo comes out " + juce::String (std::abs (db), 1) + " dB "
                   + (db < 0.0 ? "under" : "over") + " the dry sound.";
        }

        case echoes:
            return "An echo every " + formatMs (ms) + (s.sync ? " (" + dly::formatKnob (Knob::time, s) + ")" : juce::String()) + ", "
                   + juce::String (dly::getAudibleRepeats (s.feedbackPercent)) + " of them.";

        case wall:
            return "A wall " + formatMetres (dly::getWallMetres (ms)) + " away: " + getPlaceFor (dly::getWallMetres (ms)) + ".";

        default:
            return {};
    }
}

//==============================================================================
std::vector<Echo> getEchoes (const dly::Settings& s, double bpm, int most)
{
    const double seconds = dly::getDelayMs (s, bpm) / 1000.0;
    const auto [dry, wet] = fx::getMixGains (s.mixPercent);
    const double offset = juce::Decibels::gainToDecibels ((double) wet, -100.0) - juce::Decibels::gainToDecibels (juce::jmax (0.05, (double) dry));
    std::vector<Echo> list;

    for (int n = 1; n <= most; ++n)
    {
        const double db = dly::getRepeatDb (s.feedbackPercent, n);

        if (db < -60.0)
            break;

        Echo e;
        e.seconds = seconds * n;
        e.db = db + offset;
        e.highsDb = dly::getFilteredDb (s, 4000.0, n, viewRate);

        if (s.mode == dly::Mode::pingPong)
        {
            e.onLeft = n % 2 == 1;
            e.onRight = ! e.onLeft;
        }

        list.push_back (e);
    }

    return list;
}

double getShownSeconds (const dly::Settings& s, double bpm)
{
    const double d = dly::getDelayMs (s, bpm) / 1000.0;
    return juce::jlimit (1.0, 8.0, d * (juce::jmin (dly::getAudibleRepeats (s.feedbackPercent), 8) + 1.5));
}

EchoPlot drawEchoes (juce::Graphics& g, juce::Rectangle<float> bounds, const dly::Settings& s, double bpm, double seconds, bool labels)
{
    EchoPlot plot;
    plot.seconds = seconds;
    auto area = bounds.withTrimmedLeft (22.0f).withTrimmedBottom (16.0f).withTrimmedTop (labels ? 16.0f : 2.0f).withTrimmedRight (8.0f);
    plot.left = area.removeFromTop (area.getHeight() / 2.0f).withTrimmedBottom (3.0f);
    plot.right = area.withTrimmedTop (3.0f);
    const double tempo = dly::getTempo (bpm);

    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

    for (auto [lane, name] : { std::pair { plot.left, "L" }, std::pair { plot.right, "R" } })
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (lane);
        g.setColour (theme::grid);
        g.fillRect (lane.getX(), plot.toY (lane, -30.0), lane.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.drawText (name, juce::Rectangle<float> (bounds.getX(), lane.getY(), 18.0f, lane.getHeight()), juce::Justification::centred);
    }

    // The beats, and the bars a little brighter.
    const double beat = 60.0 / tempo;
    const float dashes[] = { 2.0f, 3.0f };
    int n = 1;

    for (double t = beat; t < seconds; t += beat, ++n)
    {
        g.setColour (n % 4 == 0 ? theme::gridBar : theme::gridBar.withAlpha (0.45f));
        g.drawDashedLine ({ plot.toX (t), plot.left.getY(), plot.toX (t), plot.right.getBottom() }, dashes, 2, 1.0f);
    }

    g.setFont (juce::FontOptions (10.0f));
    g.setColour (theme::textDim);
    double step = 0.25;

    for (const double nice : { 0.1, 0.25, 0.5, 1.0, 2.0 })
        if (seconds / nice <= 8.0) { step = nice; break; }

    for (double t = 0.0; t <= seconds + 1.0e-9; t += step)
        g.drawText (fxui::formatSeconds (t).replace (" ms", "ms").replace (".00 s", " s"),
                    juce::Rectangle<float> (50.0f, 12.0f).withCentre ({ plot.toX (t), plot.right.getBottom() + 8.0f }), juce::Justification::centred);

    // The clap, in both speakers.
    const auto [dry, wet] = fx::getMixGains (s.mixPercent);
    const double dryDb = juce::Decibels::gainToDecibels ((double) dry, -100.0) - juce::Decibels::gainToDecibels (juce::jmax (0.05, (double) dry));
    const float barWidth = juce::jlimit (2.0f, 8.0f, plot.left.getWidth() * (float) (0.012 / seconds) + 2.0f);

    for (auto& lane : { plot.left, plot.right })
    {
        g.setColour (juce::Colours::white);
        g.fillRect (plot.toX (0.0), plot.toY (lane, dryDb), barWidth, lane.getBottom() - plot.toY (lane, dryDb));
    }

    // The echoes: shorter for quieter, duller for darker.
    const auto list = getEchoes (s, bpm);

    for (auto& e : list)
    {
        if (e.seconds > seconds)
            break;

        const float darkness = (float) juce::jlimit (0.0, 1.0, -e.highsDb / 30.0);
        const auto c = colour.interpolatedWith (juce::Colour (0xff6a5a48), darkness);

        for (auto [lane, on] : { std::pair { plot.left, e.onLeft }, std::pair { plot.right, e.onRight } })
        {
            if (! on)
                continue;

            const float y = plot.toY (lane, e.db);
            g.setColour (c);
            g.fillRect (plot.toX (e.seconds), y, barWidth, lane.getBottom() - y);
        }
    }

    if (labels && ! list.empty())
    {
        g.setFont (juce::FontOptions (11.0f));
        const auto& first = list.front();
        const auto& lane = first.onLeft ? plot.left : plot.right;
        g.setColour (juce::Colours::white);
        g.drawText ("clap", juce::Rectangle<float> (60.0f, 14.0f).withPosition (plot.toX (0.0) + barWidth + 3.0f, plot.left.getY() + 2.0f),
                    juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("1st echo after " + formatMs (first.seconds * 1000.0) + (s.sync ? " (" + dly::formatKnob (Knob::time, s) + ")" : juce::String()),
                    juce::Rectangle<float> (220.0f, 14.0f).withPosition (plot.toX (first.seconds) + barWidth + 3.0f, plot.toY (lane, first.db) - 1.0f),
                    juce::Justification::centredLeft);

        if (list.size() > 1)
        {
            const auto& second = list[1];
            const auto& secondLane = second.onLeft ? plot.left : plot.right;
            g.drawText (juce::String (dly::getRepeatDb (s.feedbackPercent, 2), 1) + " dB",
                        juce::Rectangle<float> (70.0f, 14.0f).withPosition (plot.toX (second.seconds) + barWidth + 3.0f, plot.toY (secondLane, second.db) - 1.0f),
                        juce::Justification::centredLeft);
        }

        g.setColour (theme::textDim);
        g.drawText (juce::String (list.size()) + (list.size() == 1 ? " echo" : " echoes") + " until 60 dB down; beats at "
                        + formatBpm (tempo) + " BPM",
                    juce::Rectangle<float> (bounds.getX(), bounds.getY(), bounds.getWidth(), 14.0f), juce::Justification::centredRight);
    }

    return plot;
}

//==============================================================================
void drawWall (juce::Graphics& g, juce::Rectangle<float> bounds, const dly::Settings& s, double bpm, double since, bool labels)
{
    const double ms = dly::getDelayMs (s, bpm), metres = dly::getWallMetres (ms);
    auto area = bounds.withTrimmedBottom (labels ? 34.0f : 18.0f);
    const float ground = area.getBottom() - 6.0f;
    const float left = area.getX() + 24.0f, right = area.getRight() - 14.0f;

    // Distances on a scale of tenfolds, from 1 to 1000 m.
    auto toX = [&] (double m) { return juce::jmap ((float) (std::log10 (juce::jlimit (1.0, 1000.0, m))), 0.0f, 3.0f, left, right); };

    g.setColour (theme::gridBar);
    g.fillRect (area.getX(), ground, area.getWidth(), 1.5f);
    g.setFont (juce::FontOptions (10.0f));

    for (const double m : { 1.0, 10.0, 100.0, 1000.0 })
    {
        g.setColour (theme::gridBar);
        g.fillRect (toX (m), ground, 1.0f, 5.0f);
        g.setColour (theme::textDim);
        g.drawText (m >= 1000.0 ? "1 km" : juce::String ((int) m) + " m", juce::Rectangle<float> (40.0f, 12.0f).withCentre ({ toX (m), ground + 12.0f }),
                    juce::Justification::centred);
    }

    // You, at the start, and the wall.
    const juce::Point<float> head { left, ground - 34.0f };
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (head));
    g.fillRect (head.x - 1.5f, head.y + 6.0f, 3.0f, ground - head.y - 6.0f);

    const float wallX = toX (metres);
    const float wallTop = area.getY() + 18.0f;
    g.setColour (colour.withAlpha (0.35f));
    g.fillRect (wallX, wallTop, 10.0f, ground - wallTop);
    g.setColour (colour);
    g.drawRect (juce::Rectangle<float> (wallX, wallTop, 10.0f, ground - wallTop), 1.2f);

    // The way there and back.
    g.setColour (colour.withAlpha (0.6f));
    g.drawArrow ({ head.x + 10.0f, head.y - 6.0f, wallX - 4.0f, head.y - 6.0f }, 1.2f, 6.0f, 6.0f);
    g.drawArrow ({ wallX - 4.0f, head.y + 6.0f, head.x + 10.0f, head.y + 6.0f }, 1.2f, 6.0f, 6.0f);

    // After a clap: a ring around you for every echo that comes back, in real time.
    if (since >= 0.0)
    {
        for (auto& e : getEchoes (s, bpm, 24))
        {
            const double age = since - e.seconds;

            if (age < 0.0 || age > 0.6)
                continue;

            const float level = (float) juce::jlimit (0.0, 1.0, 1.0 + e.db / 50.0);
            const float radius = 8.0f + (float) age * 90.0f;
            g.setColour (colour.withAlpha (level * (float) (1.0 - age / 0.6)));
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (head), 2.0f);
        }
    }

    g.setFont (juce::FontOptions (11.5f));
    g.setColour (theme::text);
    const auto label = "a wall " + formatMetres (metres) + " away";
    const float labelX = juce::jlimit (area.getX(), area.getRight() - 160.0f, wallX - 80.0f);
    g.drawText (label, juce::Rectangle<float> (160.0f, 14.0f).withPosition (labelX, area.getY()), juce::Justification::centred);

    if (labels)
    {
        auto under = bounds.withTop (bounds.getBottom() - 16.0f);
        g.setColour (theme::textDim);
        g.drawText (getPlaceFor (metres) + ": " + formatMs (ms) + " there and back", under, juce::Justification::centred);
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

    /** A frequency response from 20 Hz to 20 kHz, `db` per frequency, from +8 to -40 dB. */
    void drawResponse (juce::Graphics& g, juce::Rectangle<float> r, const std::function<double (double)>& db, juce::Colour c, bool grid)
    {
        auto toX = [&] (double hz) { return r.getX() + r.getWidth() * (float) (std::log (hz / 20.0) / std::log (1000.0)); };
        auto toY = [&] (double v) { return juce::jmap ((float) juce::jlimit (-40.0, 8.0, v), -40.0f, 8.0f, r.getBottom(), r.getY()); };

        if (grid)
        {
            g.setFont (juce::FontOptions (10.0f));

            for (const double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (toX (hz), r.getY(), 1.0f, r.getHeight());
                g.setColour (theme::textDim);
                g.drawText (hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + "k" : juce::String ((int) hz),
                            juce::Rectangle<float> (40.0f, 12.0f).withCentre ({ toX (hz), r.getBottom() + 8.0f }), juce::Justification::centred);
            }

            for (const double v : { 0.0, -20.0 })
            {
                g.setColour (theme::grid);
                g.fillRect (r.getX(), toY (v), r.getWidth(), 1.0f);
                g.setColour (theme::textDim);
                g.drawText (juce::String ((int) v) + " dB", juce::Rectangle<float> (40.0f, 12.0f).withPosition (r.getX() + 2.0f, toY (v) - 13.0f),
                            juce::Justification::centredLeft);
            }
        }

        juce::Path p;

        // A quarter of a pixel at a time, so the narrow notches at the top still show.
        const int steps = (int) r.getWidth() * 4;

        for (int i = 0; i <= steps; ++i)
        {
            const float x = (float) i / 4.0f;
            const double hz = 20.0 * std::pow (1000.0, x / r.getWidth());
            const float y = toY (db (hz));
            i == 0 ? p.startNewSubPath (r.getX(), y) : p.lineTo (r.getX() + x, y);
        }

        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInDelay& delay)
{
    BuiltInDelay* processor = &delay;
    std::vector<fxui::Guide::Page> pages;

    auto loop = [] (double since, double period)
    {
        return since < 30.0 ? since : std::fmod (juce::Time::getMillisecondCounterHiRes() / 1000.0, period);
    };

    pages.push_back ({ "Echoes", "Sound that comes back",
        "Shout towards a cliff, a big building or the far side of a valley, and a moment later you hear your shout again: an echo. "
        "The sound travelled to the wall at 343 metres a second, bounced off, and came back. The further the wall, the longer it "
        "takes: a wall 17 metres away gives an echo after a tenth of a second, a cliff 170 metres away after a full second.\n\n"
        "Why don't you hear echoes in a room? You do, but they come back so quickly that the ear doesn't hear them as separate. "
        "Under about 30 to 50 milliseconds, the brain fuses a reflection with the sound, and hears it as part of the sound: a little "
        "louder, fuller, coloured by the room. That is reverb. Only when a reflection comes later than that, and is strong enough, "
        "do we hear it as a repeat. This limit is called the echo threshold.\n\n"
        "With several walls, you can hear several echoes: in a canyon, or between two buildings, the sound can bounce back and forth "
        "between them, giving echo after echo, each quieter than the last. That is what a delay with feedback does.\n\n"
        "The picture shows you and the wall that would give the delay's echo, on a scale that goes from a room to a canyon. Click "
        "Clap and watch the echoes come back.",
        [processor, loop] (juce::Graphics& g, juce::Rectangle<float> r, double since)
        {
            drawCaption (g, r, "The wall that gives this echo", "distances in tenfolds");
            drawWall (g, r, processor->getSettings(), processor->tempoBpm.load(), loop (since, 4.0), true);
        } });

    pages.push_back ({ "How a delay works", "Record now, play back later",
        "A delay is a memory for sound: it records what comes in and plays it back a little later. Mixed with the original, you hear "
        "the sound and then its echo.\n\n"
        "The first delays were tape machines. A loop of tape ran past a recording head and, a few centimetres on, a playback head: "
        "the time the tape took to get from one to the other was the delay. Moving the heads apart or slowing the tape made the "
        "echo come later. Famous ones are the Echoplex (1961), the Binson Echorec and the Roland RE-201 Space Echo (1974). Because "
        "tape wears and wobbles, their echoes were darker and a little unsteady, which is still a much-loved sound.\n\n"
        "In the late 1970s 'bucket brigade' chips passed the sound along a chain of hundreds of tiny capacitors, like a line of "
        "people passing buckets of water: analog delay, in a guitar pedal. Then came digital delays, which store the sound as "
        "numbers.\n\n"
        "A digital delay keeps the samples in a ring of memory. A write position goes round the ring storing each new sample; a "
        "read position follows a fixed distance behind it. That distance, in samples, divided by the sample rate, is the delay "
        "time. At 48,000 samples a second, a delay of 250 ms keeps 12,000 samples between the two. DawIt Delay keeps up to 4 "
        "seconds.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const double ms = processor->getDelayMs();
            drawCaption (g, r, "A ring of memory", "the gap between writing and reading is the delay");
            const auto centre = r.getCentre().translated (-r.getWidth() * 0.18f, 0.0f);
            const float radius = juce::jmin (r.getHeight(), r.getWidth()) * 0.38f;
            const double fraction = ms / dly::longestMs;
            const double now = std::fmod (juce::Time::getMillisecondCounterHiRes() / 6000.0, 1.0);
            const auto at = [&] (double turn, float rr) { const double a = juce::MathConstants<double>::twoPi * turn - juce::MathConstants<double>::halfPi;
                                                         return centre + juce::Point<float> ((float) std::cos (a), (float) std::sin (a)) * rr; };

            // The stored sound around the ring, and the part between reading and writing.
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 18.0f);
            juce::Path between;
            between.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, (float) (juce::MathConstants<double>::twoPi * (now - fraction)),
                                   (float) (juce::MathConstants<double>::twoPi * now), true);
            g.setColour (colour.withAlpha (0.55f));
            g.strokePath (between, juce::PathStrokeType (18.0f));

            g.setColour (juce::Colours::white);
            g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (at (now, radius)));
            g.setColour (colour);
            g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (at (now - fraction, radius)));

            g.setFont (juce::FontOptions (12.0f));
            g.setColour (juce::Colours::white);
            g.drawText ("write: now", juce::Rectangle<float> (110.0f, 14.0f).withCentre (at (now, radius + 34.0f)), juce::Justification::centred);
            g.setColour (colour);
            g.drawText ("read: " + formatMs (ms) + " ago", juce::Rectangle<float> (140.0f, 14.0f).withCentre (at (now - fraction, radius + 34.0f)), juce::Justification::centred);

            auto side = r.withTrimmedLeft (r.getWidth() * 0.62f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f));
            g.drawFittedText ("The whole ring: " + formatMs (dly::longestMs) + "\nThe delay: " + formatMs (ms) + " = "
                                  + juce::String (juce::roundToInt (ms * 48.0)) + " samples at 48 kHz\n\nThe ring turns slowly here; "
                                  "really, the write position moves 48,000 samples a second.",
                              side.toNearestInt(), juce::Justification::centredLeft, 8);
        } });

    pages.push_back ({ "Feedback", "Echoes of echoes",
        "With feedback, a part of what comes out of the delay goes back in, so the echo gets delayed again and gives a second echo, "
        "which gives a third, and so on: like the sound bouncing between two walls.\n\n"
        "The feedback is the share that goes back in. Each echo is that much of the one before: at 50%, every echo is half as loud "
        "as the last, 6 dB quieter. The level of echo n is feedback to the power n-1. In decibels it drops in equal steps, so on "
        "the picture's dB scale the echoes go down in a straight line.\n\n"
        "How many you hear: until they are about 60 dB down. At 30% feedback that's 6 echoes, at 50% 10, at 70% 20, at 90% 66.\n\n"
        "At 100%, every echo is as loud as the last and they never stop. Above 100%, each is louder than the last and they grow "
        "until they distort: the 'runaway' that dub engineers and guitarists play with by turning the feedback up for a moment. "
        "On tape machines, the darkening of every repeat kept it musical. DawIt Delay stops at 95%.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double bpm = processor->tempoBpm.load();
            drawCaption (g, r, "The echoes, each quieter by the feedback", dly::formatKnob (Knob::feedback, s) + " feedback");
            drawEchoes (g, r, s, bpm, getShownSeconds (s, bpm), true);
        } });

    pages.push_back ({ "Time and tempo", "Echoes that groove",
        "Echoes that land on the song's grid become part of the rhythm; echoes that don't can sound messy. So delays usually follow "
        "the tempo: their time is a note value.\n\n"
        "A quarter note (one beat) lasts 60,000 / BPM milliseconds: at 120 BPM, 500 ms. An eighth is half of that, a sixteenth a "
        "quarter. A dotted note is one and a half times as long; a triplet two thirds.\n\n"
        "Which to use:\n"
        "- 1/4 and 1/8: the echoes land on the beats or between them; steady and clear.\n"
        "- Dotted 1/8 (three sixteenths): the echoes fall across the beat and fill in the sixteenths you didn't play, which makes a "
        "running pattern out of simple playing. The sound of U2's The Edge.\n"
        "- Triplets: a swinging, rolling feel against straight playing.\n"
        "- 1/2 or a bar: long echoes that repeat whole phrases.\n\n"
        "Without a tempo (Sync off) the time is just milliseconds, for effects that aren't about rhythm: slapback, doubling and "
        "widening, which are all much shorter than a beat.\n\n"
        "The picture shows where three common note values land on the song's beats, and the table how long each note value lasts.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const double tempo = dly::getTempo (processor->tempoBpm.load());
            drawCaption (g, r, "Echoes against the beats at " + formatBpm (tempo) + " BPM", "the clap, then its echoes");
            auto table = r.removeFromRight (190.0f);
            const int notes[] = { dly::quarterNote, 7, 3 };
            const float rowHeight = r.getHeight() / 3.0f;
            const double seconds = 4.0 * 60.0 / tempo;

            for (int i = 0; i < 3; ++i)
            {
                auto row = r.withHeight (rowHeight).withY (r.getY() + rowHeight * (float) i).reduced (0.0f, 4.0f);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                g.drawText (dly::noteValues[notes[i]].name, row.removeFromLeft (84.0f), juce::Justification::centredLeft);
                g.setColour (juce::Colours::black.withAlpha (0.2f));
                g.fillRect (row);
                auto toX = [&] (double t) { return row.getX() + row.getWidth() * (float) (t / seconds); };

                for (int b = 0; b <= 4; ++b)
                {
                    g.setColour (b % 4 == 0 ? theme::gridBar : theme::gridBar.withAlpha (0.5f));
                    g.fillRect (toX (b * 60.0 / tempo), row.getY(), 1.0f, row.getHeight());
                }

                g.setColour (juce::Colours::white);
                g.fillRect (toX (0.0), row.getY() + 4.0f, 4.0f, row.getHeight() - 8.0f);
                const double d = dly::getNoteMs (notes[i], tempo) / 1000.0;
                float level = 0.8f;

                for (double t = d; t < seconds; t += d, level *= 0.7f)
                {
                    const float h = (row.getHeight() - 8.0f) * level;
                    g.setColour (colour);
                    g.fillRect (toX (t), row.getBottom() - 4.0f - h, 4.0f, h);
                }
            }

            g.setFont (juce::FontOptions (11.5f));

            for (int i = 0; i < dly::numNoteValues; ++i)
            {
                auto line = table.withHeight (table.getHeight() / dly::numNoteValues).withY (table.getY() + table.getHeight() / dly::numNoteValues * (float) i);
                g.setColour (theme::textDim);
                g.drawText (dly::noteValues[i].name, line.withTrimmedLeft (14.0f), juce::Justification::centredLeft);
                g.setColour (theme::text);
                g.drawText (formatMs (dly::getNoteMs (i, tempo)), line, juce::Justification::centredRight);
            }
        } });

    pages.push_back ({ "Short delays", "Slapback, doubling, width",
        "Below the echo threshold, a delay doesn't sound like an echo at all, and turns into a whole family of effects:\n\n"
        "- Slapback, 80-150 ms, one repeat: a quick 'slap' right behind the sound. The 1950s rock 'n' roll sound of Elvis Presley "
        "and Sun Records, made with a tape machine playing back what it had just recorded.\n\n"
        "- Doubling, 20-50 ms: sounds like a second performance in unison. Before double tracking, engineers used it to make a "
        "voice sound like two.\n\n"
        "- Widening (the Haas effect), 1-30 ms on one side only: the ear hears the sound where it arrives first, and the late copy "
        "on the other side as width. Helmut Haas studied it in 1949.\n\n"
        "- Comb filtering, under about 10 ms: the copy adds to the sound at some frequencies and cancels it at others, because the "
        "copy is in step at some and out of step at others. The response looks like the teeth of a comb, and sounds hollow and "
        "metallic. Slowly changing the time makes the teeth sweep: that is a flanger (1-10 ms) or a chorus (10-30 ms).\n\n"
        "The picture shows the comb filter of a sound plus one copy at the delay's time, when it's under 20 ms, otherwise at 3 ms. "
        "Its notches are where the copy is exactly half a cycle late: the first at 1 / (2 x the delay).",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const double ms = processor->getDelayMs();
            const double shown = ms < 20.0 ? ms : 3.0;
            drawCaption (g, r, "A sound plus a copy " + formatMs (shown) + " later", "notches at " + juce::String (juce::roundToInt (500.0 / shown)) + " Hz, "
                                   + juce::String (juce::roundToInt (1500.0 / shown)) + " Hz, ...");
            drawResponse (g, r.withTrimmedBottom (18.0f), [shown] (double hz)
            {
                const double phase = juce::MathConstants<double>::twoPi * hz * shown / 1000.0;
                const double re = 1.0 + std::cos (phase), im = -std::sin (phase);
                return 20.0 * std::log10 (juce::jmax (1.0e-4, std::sqrt (re * re + im * im) / 2.0)) + 6.02;
            }, colour, true);
        } });

    pages.push_back ({ "The sound of the repeats", "Darker every time",
        "On a tape echo, every trip through the machine cost the sound a little: the tape lost some of the highs, added a little "
        "hiss, a little wobble and a little saturation. With feedback, the sound went through again and again, so each repeat was "
        "darker and softer than the one before, until the last ones were only a murmur.\n\n"
        "That turned out to be exactly what makes echoes sit well in a mix. Dark echoes fall behind the sound instead of competing "
        "with it; the words or notes stay clear, and the echoes fill the space around them. Bright, perfect digital echoes are clearer "
        "but busier, and can sound harsh when they pile up.\n\n"
        "DawIt Delay does the darkening with a high cut, and the thinning with a low cut, inside the feedback loop: every repeat goes "
        "through them again. The picture shows what is left of each frequency in the first five echoes: each one lower at the top "
        "(the high cut) and at the bottom (the low cut).",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "What's left of each frequency in echoes 1 to 5", "high cut " + dly::formatKnob (Knob::highCut, s) + ", low cut " + dly::formatKnob (Knob::lowCut, s));
            auto area = r.withTrimmedBottom (18.0f);

            for (int n = 5; n >= 1; --n)
                drawResponse (g, area, [s, n] (double hz) { return dly::getFilteredDb (s, hz, n, viewRate); },
                              colour.withAlpha (1.0f - 0.15f * (float) (n - 1)), n == 5);

            g.setFont (juce::FontOptions (11.0f));
            g.setColour (colour);
            g.drawText ("1st echo on top, 5th at the bottom", area.reduced (8.0f, 4.0f), juce::Justification::bottomRight);
        } });

    pages.push_back ({ "Stereo and ping-pong", "Where the echoes come from",
        "A delay can put its echoes anywhere between the speakers.\n\n"
        "Stereo: each side has its own memory. A sound on the left echoes on the left; a sound in the middle echoes in the middle. "
        "The echoes stay with the sound.\n\n"
        "Ping-pong: the sound goes into the left memory only, which feeds the right memory, which feeds the left again. So the "
        "echoes bounce from side to side, while the sound itself stays where it was. That makes the sound wider, and the echoes "
        "easier to hear apart from the sound, because they come from somewhere else.\n\n"
        "With ping-pong and no feedback there's only one echo, on the left: with a short time, that is the Haas effect from Short "
        "delays, which makes a mono sound wide. Check it in mono, though: added together, the sound and its short copy can sound "
        "hollow.\n\n"
        "The picture shows the left speaker above the right, with the delay's mode.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double bpm = processor->tempoBpm.load();
            drawCaption (g, r, "Left above, right below", dly::formatKnob (Knob::mode, s));
            drawEchoes (g, r, s, bpm, getShownSeconds (s, bpm), true);
        } });

    pages.push_back ({ "Delay in a mix", "Echoes in the gaps",
        "Delay adds space, depth and movement to a mix, often more clearly than reverb, because an echo doesn't wash over the sound "
        "like a reverb tail does.\n\n"
        "When it's used:\n\n"
        "- Vocals: echoes in the gaps (1/4 or 1/8), slapback for rock'n'roll and country (80-150 ms), throws on single words.\n"
        "- Guitars: slapback for rockabilly, dotted-eighth patterns that become part of the riff (U2's The Edge built his sound on "
        "them), long echoes on leads and solos.\n"
        "- Synths and keys: ping-pong to make them wide and moving.\n"
        "- Mono parts: 15-35 ms on one side makes them wide.\n"
        "- Dub, reggae and ambient: long feedback trails, ridden by hand, as part of the music.\n"
        "- Less on drums and bass, where echoes blur the groove; a single echo on one snare hit is an effect.\n\n"
        "How it's used:\n\n"
        "- Echoes belong in the gaps. On a vocal, the echoes should be heard in the pauses between phrases, not on top of the next "
        "words. A note value that fits the phrasing, a low mix and a little feedback usually do it.\n\n"
        "- Throws. Engineers often turn the delay up for one word or note only (the last word of a line, say), so that just it "
        "echoes into the gap. In DawIt you can do this by changing the mix knob at that moment.\n\n"
        "- Ducking. Many delays turn their echoes down while the sound plays and up in the gaps. DawIt Delay doesn't, but a lower "
        "mix and a dark high cut keep the echoes out of the way in the same spirit.\n\n"
        "- Filter the echoes. Dark, thin echoes (high cut 3-6 kHz, low cut 150-300 Hz) stay behind the sound and keep the low end "
        "clear.\n\n"
        "- Delay and reverb together: a delay before a reverb gives echoes in a space. In DawIt, put DawIt Delay before DawIt "
        "Reverb in the track's chain.\n\n"
        "- Keep it in time. Synced echoes support the groove; free ones are for slapback and widening.\n\n"
        "A delay that several tracks share sits on a return track: Track > New Return Track > Delay, then turn up the Delay knob "
        "on each mixer strip. On a return the mix is 100%, and each send sets how much a track gets. A delay on the track itself "
        "is fine when only that track needs it, such as a slapback on one guitar.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double tempo = dly::getTempo (processor->tempoBpm.load());
            const double d = dly::getDelayMs (s, tempo) / 1000.0, beat = 60.0 / tempo, seconds = 8.0 * beat;
            drawCaption (g, r, "Two sung phrases and their echoes", "the echoes are heard in the gap");
            auto lane = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 160.0f));
            auto toX = [&] (double t) { return lane.getX() + lane.getWidth() * (float) (t / seconds); };
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (lane);

            for (int b = 0; b <= 8; ++b)
            {
                g.setColour (b % 4 == 0 ? theme::gridBar : theme::gridBar.withAlpha (0.45f));
                g.fillRect (toX (b * beat), lane.getY(), 1.0f, lane.getHeight());
            }

            // Two phrases of four short words, each followed by two beats of rest.
            const float wordH = lane.getHeight() * 0.42f;
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (lane.toNearestInt());

            for (int phrase = 0; phrase < 2; ++phrase)
            {
                for (int w = 0; w < 4; ++w)
                {
                    const double start = (phrase * 4.0 + w * 0.5) * beat, length = 0.45 * beat;
                    g.setColour (juce::Colours::white.withAlpha (0.85f));
                    g.fillRoundedRectangle (juce::Rectangle<float> (toX (start) + 1.0f, lane.getY() + 8.0f, toX (length) - toX (0.0), wordH), 3.0f);

                    // The last word echoes into the rest.
                    if (w < 3)
                        continue;

                    for (auto& e : getEchoes (s, tempo, 12))
                    {
                        const float level = (float) juce::jlimit (0.05, 1.0, 1.0 + e.db / 40.0);
                        g.setColour (colour.withAlpha (0.3f + 0.6f * level));
                        g.fillRoundedRectangle (juce::Rectangle<float> (toX (start + e.seconds) + 1.0f, lane.getBottom() - 8.0f - wordH * level,
                                                                        toX (length) - toX (0.0), wordH * level), 3.0f);
                    }
                }
            }

            g.setFont (juce::FontOptions (11.0f));
            g.setColour (theme::textDim);
            g.drawText ("white: the singing; purple: the echoes of the last word of each phrase, every " + formatMs (d * 1000.0),
                        lane.translated (0.0f, lane.getHeight() + 4.0f).withHeight (14.0f), juce::Justification::centredLeft);
        } });

    pages.push_back ({ "How DawIt Delay works", "A memory with a loop",
        "1. The sound goes into a memory for each side, up to 4 seconds long, and is read back the delay time later. When the time "
        "follows the tempo, it is worked out from the song's BPM. A change of time glides over a fifth of a second, like a tape "
        "machine speeding up or slowing down, instead of jumping, which would click.\n\n"
        "2. What is read goes through a high cut and a low cut (12 dB per octave each).\n\n"
        "3. That is the echo. A part of it, the feedback, is added back into the memory, so it comes round again: through the "
        "filters again, darker again, quieter again. In ping-pong mode, the left memory's echo goes into the right memory and the "
        "right's into the left; the sound itself goes only into the left.\n\n"
        "4. The echoes are mixed with the dry sound.\n\n"
        "In a memory of 4 seconds at 48 kHz that's 192,000 samples per side. A time between two samples (say 250.3 samples) reads "
        "between them, which is what lets the time glide smoothly.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "From the dry sound to the echoes", {});
            const float col = (r.getWidth() - 4.0f * 18.0f) / 5.0f, midY = r.getCentreY() - 10.0f;
            auto box = [&] (int column, float y, float height = 50.0f) { return juce::Rectangle<float> (r.getX() + (col + 18.0f) * (float) column, y - height / 2.0f, col, height); };

            const auto in = drawBox (g, box (0, midY), "Sound in", "dry", theme::textDim);
            const auto add = drawBox (g, box (1, midY, 36.0f), "+", {}, colour);
            const auto memory = drawBox (g, box (2, midY), "Memory", "read the time later", colour);
            const auto filters = drawBox (g, box (3, midY), "High and low cut", "on every repeat", colour);
            const auto out = drawBox (g, box (4, midY), "Mix", "with the dry sound", theme::accent);

            arrow (g, { in.getRight(), midY }, { add.getX(), midY });
            arrow (g, { add.getRight(), midY }, { memory.getX(), midY });
            arrow (g, { memory.getRight(), midY }, { filters.getX(), midY });
            arrow (g, { filters.getRight(), midY }, { out.getX(), midY });

            // The feedback: back from after the filters to the adder.
            juce::Path back;
            const float y = midY + 70.0f;
            back.startNewSubPath (filters.getCentreX(), filters.getBottom());
            back.lineTo (filters.getCentreX(), y);
            back.lineTo (add.getCentreX(), y);
            g.setColour (colour);
            g.strokePath (back, juce::PathStrokeType (1.4f));
            g.drawArrow ({ add.getCentreX(), y, add.getCentreX(), add.getBottom() }, 1.4f, 7.0f, 7.0f);
            g.setFont (juce::FontOptions (11.5f));
            g.drawText ("feedback: a part of every echo goes round again", juce::Rectangle<float> (filters.getCentreX() - add.getCentreX(), 16.0f)
                                                                              .withPosition (add.getCentreX(), y + 4.0f), juce::Justification::centred);

            // The dry sound around everything, to the mix.
            juce::Path dry;
            const float top = midY - 60.0f;
            dry.startNewSubPath (in.getCentreX(), in.getY());
            dry.lineTo (in.getCentreX(), top);
            dry.lineTo (out.getCentreX(), top);
            g.setColour (theme::textDim);
            g.strokePath (dry, juce::PathStrokeType (1.2f));
            g.drawArrow ({ out.getCentreX(), top, out.getCentreX(), out.getY() }, 1.2f, 7.0f, 7.0f);
            g.drawText ("the dry sound", juce::Rectangle<float> (out.getCentreX() - in.getCentreX(), 16.0f).withPosition (in.getCentreX(), top - 18.0f),
                        juce::Justification::centred);
        } });

    return pages;
}
} // namespace daw::dlytext
