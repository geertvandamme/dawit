#pragma once

#include "engine/BuiltInReverb.h"
#include "EffectWindow.h"

/**
    The words and pictures of DawIt Reverb: what each knob and part of its window does, with live
    numbers, the "What is reverb?" guide, and the two pictures both use: the response (what one
    clap turns into) and the room seen from above.
*/
namespace daw::verbtext
{
/** The pictures in the window; the knobs are 0 up (verb::Knob), the shared topics fxui::CommonTopic. */
enum Topic { response = 50, room, type };

inline const juce::Colour colour { 0xff6fd3c0 };        // the tail
inline const juce::Colour earlyColour { 0xffffb35c };   // the early reflections

juce::String getTitle (int topic);

/** What a knob or part of the window does, and why: the theory. */
juce::String describe (int topic);

/** Typical settings, for a knob. */
juce::String getTypical (int topic);

/** What these settings mean in numbers, at the song's tempo (0: unknown). */
juce::String describeSettings (int topic, const verb::Settings&, double bpm);

//==============================================================================
/** What one clap turns into: the levels of the early reflections and of the tail over time, in dB
    next to the direct sound at the mix (one value per `windowMs`). */
struct Response
{
    verb::Settings settings;
    double seconds = 0.0, windowMs = 5.0;
    std::vector<float> earlyDb, tailDb;
    std::vector<std::pair<float, float>> arrivals; // time in ms and magnitude, first 100 ms
    float tailStartDb = -100.0f;   // the tail at its loudest, where the decay is measured from
};

Response computeResponse (const verb::Settings&);

/** Computes a response again only when the settings changed. */
struct ResponseCache
{
    const Response& get (const verb::Settings&);

private:
    Response response;
    bool valid = false;
};

/** Where a response is drawn: time left to right, from 0 to `seconds`; level up, from -90 to +6 dB. */
struct ResponsePlot
{
    juce::Rectangle<float> area;
    double seconds = 2.0;
    static constexpr float topDb = 6.0f, bottomDb = -90.0f;

    float toX (double t) const { return area.getX() + area.getWidth() * (float) juce::jlimit (0.0, 1.0, t / seconds); }
    float toY (double db) const { return juce::jmap ((float) juce::jlimit ((double) bottomDb, (double) topDb, db), bottomDb, topDb, area.getBottom(), area.getY()); }

    /** Without keeping inside the area, for lines that run off it (draw them clipped). */
    juce::Point<float> at (double t, double db) const
    {
        return { area.getX() + area.getWidth() * (float) (t / seconds), juce::jmap ((float) db, bottomDb, topDb, area.getBottom(), area.getY()) };
    }

    double fromX (float x) const { return (x - area.getX()) / juce::jmax (1.0f, area.getWidth()) * seconds; }
};

/** How much time to show for these settings: the whole tail, and a little more. */
double getShownSeconds (const verb::Settings&);

/** Draws the response with its labels: direct sound, pre-delay, early reflections, tail, and
    the decay line to -60 dB. With a tempo, the beats too. Returns where it drew. */
ResponsePlot drawResponse (juce::Graphics&, juce::Rectangle<float>, const Response&, double bpm, double seconds, bool labels);

/** The room the size stands for, seen from above: the walls (harder or softer with the damping),
    the sound and the listener, the direct way and the first reflections. For a while after a
    clap, the sound spreads and bounces, slowed down 20 times. */
void drawRoom (juce::Graphics&, juce::Rectangle<float>, const verb::Settings&, double secondsSinceClap, bool labels);

/** Draws the physical thing that makes this kind of reverb, with a moving clap. */
void drawKind (juce::Graphics&, juce::Rectangle<float>, const verb::Settings&, double secondsSinceClap, bool labels);

/** The pages of "What is reverb?", with pictures that follow the reverb's settings. */
std::vector<fxui::Guide::Page> makeGuide (BuiltInReverb&);
} // namespace daw::verbtext
