#include "StereoGuide.h"
#include "StereoScope.h"
#include "Theme.h"
#include <cmath>

namespace daw::stereotext
{
namespace
{
    void drawPicture (juce::Graphics& g, juce::Rectangle<float> area, int page, BuiltInStereo& processor)
    {
        const auto settings = processor.getSettings();
        const auto pink = stereoScope::colour;
        const auto amber = stereoScope::lowColour;
        g.setColour (theme::textDim);
        g.setFont (14.0f);
        auto title = area.removeFromTop (28.0f);
        const juce::String titles[] { "Three stereo sounds", "Left and right, mid and side", "Four stereo shapes", "Level change in mono", "A wide pad: before and after", "Crossover bands", "Level and correlation against width", "Starting points", "A wide pad at 400 Hz", "The signal path", "Your track: input and output" };
        g.drawText (titles[page], title, juce::Justification::centredLeft);
        area.reduce (12.0f, 12.0f);
        if (page == 10)
        {
            auto left = area.removeFromLeft (area.getWidth() * 0.5f).reduced (8.0f);
            auto right = area.reduced (8.0f);
            g.setColour (theme::text); g.setFont (14.0f);
            g.drawText ("In", left.removeFromTop (20.0f), juce::Justification::centred);
            g.drawText ("Out", right.removeFromTop (20.0f), juce::Justification::centred);
            stereoScope::drawGoniometer (g, left.removeFromTop (left.getHeight() * 0.8f), processor.inputMeasurement.getPoints());
            stereoScope::drawGoniometer (g, right.removeFromTop (right.getHeight() * 0.8f), processor.outputMeasurement.getPoints());
            const auto inputLow = processor.inputMeasurement.getCorrelation (true), inputAll = processor.inputMeasurement.getCorrelation (false);
            const auto outputLow = processor.outputMeasurement.getCorrelation (true), outputAll = processor.outputMeasurement.getCorrelation (false);
            stereoScope::drawCorrelation (g, left.removeFromTop (left.getHeight() * 0.5f), "Low", inputLow);
            stereoScope::drawCorrelation (g, left, "All", inputAll);
            stereoScope::drawCorrelation (g, right.removeFromTop (right.getHeight() * 0.5f), "Low", outputLow);
            stereoScope::drawCorrelation (g, right, "All", outputAll);
            return;
        }
        if (page == 0 || page == 1)
        {
            const juce::StringArray labels = page == 0 ? juce::StringArray { "Vocal: centred", "Guitar: left", "Pad: wide" }
                                                         : juce::StringArray { "Left", "Right", "Mid", "Side" };
            if (page == 1)
            {
                auto pairs = area.removeFromBottom (48.0f);
                g.setColour (theme::textDim); g.setFont (12.0f);
                g.drawFittedText ("L / R to M / S:   0.8 / 0.4 to 0.6 / 0.2     0.5 / 0.5 to 0.5 / 0\n0.6 / 0 to 0.3 / 0.3     0.5 / -0.5 to 0 / 0.5", pairs.toNearestInt(), juce::Justification::centred, 2);
            }
            const float rowHeight = area.getHeight() / (float) labels.size();
            for (int row = 0; row < labels.size(); ++row)
            {
                auto rowArea = area.removeFromTop (rowHeight).reduced (3.0f);
                auto name = rowArea.removeFromLeft (110.0f);
                g.setColour (theme::text); g.drawText (labels[row], name, juce::Justification::centredLeft);
                auto plot = rowArea.reduced (4.0f);
                if (page == 0) plot.removeFromRight (100.0f);
                for (int line = 0; line < (page == 0 ? 2 : 1); ++line)
                {
                    juce::Path wave;
                    for (int i = 0; i <= 160; ++i)
                    {
                        const float x = (float) i / 160.0f;
                        const float a = x * 12.56637f;
                        float value = 0.0f;
                        if (page == 0)
                            value = row == 1 && line == 1 ? 0.0f : std::sin (a + (row == 2 && line == 1 ? 1.5708f : 0.0f));
                        else
                        {
                            const float l = std::sin (a), r = std::sin (a - 1.0472f);
                            value = row == 0 ? l : row == 1 ? r : row == 2 ? (l + r) * 0.5f : (l - r) * 0.5f;
                        }
                        const float centre = page == 0 ? plot.getY() + plot.getHeight() * (line == 0 ? 0.25f : 0.75f) : plot.getCentreY();
                        const float y = centre - value * plot.getHeight() * (page == 0 ? 0.20f : 0.38f);
                        if (i == 0) wave.startNewSubPath (plot.getX(), y); else wave.lineTo (plot.getX() + x * plot.getWidth(), y);
                    }
                    g.setColour (line == 0 ? pink : amber); g.strokePath (wave, juce::PathStrokeType (1.5f));
                }
                if (page == 0)
                {
                    auto speakers = rowArea.removeFromRight (92.0f).reduced (4.0f);
                    const float y = speakers.getCentreY(); g.setColour (theme::textDim); g.drawLine (speakers.getX(), y, speakers.getRight(), y);
                    const float x = row == 1 ? speakers.getX() + 8.0f : speakers.getCentreX();
                    g.setColour (pink); g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
                    g.setColour (theme::textDim); g.drawText ("L                                 R", speakers.withY (y + 4.0f), juce::Justification::centred);
                }
            }
            return;
        }
        if (page == 3 || page == 7 || page == 9)
        {
            const juce::StringArray labels = page == 3 ? juce::StringArray { "Lead vocal  0.0 dB", "Wide pad  -2.2 dB", "Stereo reverb  -3.0 dB", "Widened synth  -6.0 dB", "Upside-down bass  -16.0 dB" }
                : page == 7 ? juce::StringArray { "Mix bus  120 Hz / 100%", "Synth bass  200 Hz / 100%", "Fully mono  120 Hz / 0%", "Vinyl-safe  150 Hz / 100%", "Wide pads  200 Hz / 150%", "Mono check  Off / Mono" }
                : juce::StringArray { "Crossover", "Low band to mono", "High band width", "Add bands", "Listen", "Match level", "Output" };
            const float row = area.getHeight() / (float) labels.size();
            for (int i = 0; i < labels.size(); ++i)
            {
                auto r = area.removeFromTop (row).reduced (3.0f);
                g.setColour (theme::textDim.withAlpha (0.12f)); g.fillRoundedRectangle (r, 4.0f);
                if (page == 3)
                {
                    const float drops[] { 0.0f, 2.2f, 3.0f, 6.0f, 16.0f };
                    auto bar = r.removeFromRight (r.getWidth() * 0.42f).reduced (4.0f, row * 0.28f);
                    bar.setWidth (bar.getWidth() * drops[i] / 16.0f);
                    g.setColour (pink.withAlpha (0.65f)); g.fillRoundedRectangle (bar, 2.0f);
                }
                else if (page == 7)
                {
                    const float cutoffs[] { 120.0f, 200.0f, 120.0f, 150.0f, 200.0f, 0.0f };
                    const float widths[] { 100.0f, 100.0f, 0.0f, 100.0f, 150.0f, 100.0f };
                    auto bar = r.removeFromRight (r.getWidth() * 0.42f).reduced (4.0f, row * 0.2f);
                    auto low = bar.removeFromLeft (bar.getWidth() * cutoffs[i] / 500.0f);
                    g.setColour (amber.withAlpha (0.65f)); g.fillRoundedRectangle (low, 2.0f);
                    bar.setWidth (bar.getWidth() * widths[i] / 200.0f);
                    g.setColour (pink.withAlpha (0.65f)); g.fillRoundedRectangle (bar, 2.0f);
                }
                g.setColour (theme::text); g.drawText (labels[i], r.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
            }
            return;
        }
        if (page == 2)
        {
            const juce::StringArray labels { "Mono  +1.00", "Left only  0", "Wide  0", "Upside down  -1.00" };
            const float w = area.getWidth() / 4.0f;
            for (int i = 0; i < 4; ++i)
            {
                auto r = area.removeFromLeft (w).reduced (8.0f);
                auto caption = r.removeFromBottom (24.0f); auto c = r.getCentre();
                g.setColour (theme::textDim.withAlpha (0.4f)); g.drawRect (r, 1.0f);
                g.setColour (pink);
                for (int n = 0; n < 240; ++n)
                {
                    const float a = (float) n * 0.19f; const float y = std::sin (a) * juce::jmin (r.getWidth(), r.getHeight()) * 0.42f;
                    const float x = i == 0 ? 0.0f : i == 1 ? y : i == 2 ? std::sin (a * 1.31f) * r.getWidth() * 0.38f : std::sin (a) * r.getWidth() * 0.42f;
                    g.fillEllipse (c.x + x - 1.0f, c.y + (i == 3 ? 0.0f : y) - 1.0f, 2.0f, 2.0f);
                }
                g.setColour (theme::text); g.drawText (labels[i], caption, juce::Justification::centred);
            }
            return;
        }
        auto legend = area.removeFromBottom (25.0f);
        const juce::String legendText = page == 5 ? "20 Hz                         Frequency                         20 kHz  |  grey: 12 / 24 / 48; current: " + juce::String (settings.slope) + " dB/oct"
                                     : page == 6 ? "0%           Width           200%  |  pink: pad, amber: noise; bright: correlation, faint: dB"
                                     : page == 4 || page == 8 ? "20 Hz                         Frequency                         20 kHz  |  grey: mid, pink: side before and after"
                                     : "Pink: left / mid    Amber: right / side";
        g.setColour (theme::textDim); g.drawText (legendText, legend, juce::Justification::centred);
        g.setColour (theme::textDim.withAlpha (0.35f)); g.drawRect (area, 1.0f);
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());
        if (page == 5)
        {
            for (int slope : { 12, 24, 48 })
            {
                juce::Path curve;
                for (int i = 0; i <= 200; ++i)
                {
                    const float x = (float) i / 200.0f;
                    const double hz = 20.0 * std::pow (1000.0, (double) x);
                    const float high = (float) stereo::highMagnitude (hz / juce::jmax (20.0, (double) settings.monoBelowHz), slope);
                    const float y = area.getY() + (0.9f - high * 0.8f) * area.getHeight();
                    if (i == 0) curve.startNewSubPath (area.getX(), y); else curve.lineTo (area.getX() + x * area.getWidth(), y);
                }
                g.setColour (theme::textDim.withAlpha (0.4f)); g.strokePath (curve, juce::PathStrokeType (1.0f));
            }
        }
        if (page == 6)
        {
            g.setColour (theme::textDim); g.setFont (11.0f);
            g.drawText ("Correlation +1 / level +6 dB", area.removeFromTop (16.0f), juce::Justification::centredLeft);
            g.drawText ("Correlation -1 / level -6 dB", area.removeFromBottom (16.0f), juce::Justification::centredLeft);
            const float marker = area.getX() + juce::jlimit (0.0f, 1.0f, settings.width / 200.0f) * area.getWidth();
            g.setColour (theme::textDim); g.drawVerticalLine ((int) marker, area.getY(), area.getBottom());
            for (int example = 0; example < 2; ++example)
            {
                juce::Path level;
                for (int i = 0; i <= 200; ++i)
                {
                    const float x = (float) i / 200.0f;
                    const double db = stereo::widthLevelDb (example == 0 ? 4.0 : 1.0, 1.0, (double) x * 2.0);
                    const float y = area.getY() + (0.5f - (float) db * 0.08f) * area.getHeight();
                    if (i == 0) level.startNewSubPath (area.getX(), y); else level.lineTo (area.getX() + x * area.getWidth(), y);
                }
                g.setColour ((example == 0 ? pink : amber).withAlpha (0.5f)); g.strokePath (level, juce::PathStrokeType (1.0f));
            }
        }
        for (int line = 0; line < ((page == 4 || page == 8) ? 3 : 2); ++line)
        {
            juce::Path path;
            for (int i = 0; i <= 200; ++i)
            {
                const float x = (float) i / 200.0f;
                float y = 0.0f;
                if (page == 5)
                {
                    const double ratio = std::pow (1000.0, (double) x) * 20.0 / juce::jmax (20.0, (double) settings.monoBelowHz);
                    const float high = (float) stereo::highMagnitude (ratio, settings.slope);
                    y = line == 0 ? high : 1.0f - high;
                    y = 0.9f - y * 0.8f;
                }
                else if (page == 4 || page == 8)
                {
                    const double hz = 20.0 * std::pow (1000.0, (double) x);
                    const double high = stereo::highMagnitude (hz / (page == 8 ? 400.0 : 150.0), 24);
                    const double midDb = -20.0 - 3.0 * std::log2 (hz / 1000.0);
                    const double sideDb = midDb - 6.0 + (line == 2 ? 20.0 * std::log10 (juce::jmax (1.0e-6, high)) : 0.0);
                    y = (float) juce::jlimit (0.0, 1.0, -(line == 0 ? midDb : sideDb) / 90.0);
                }
                else if (page == 6)
                {
                    const double w = (double) x * 2.0;
                    y = line == 0 ? (float) (0.55 - stereo::widthCorrelation (4.0, 1.0, w) * 0.4) : (float) (0.55 - stereo::widthCorrelation (1.0, 1.0, w) * 0.4);
                }
                else y = 0.5f + 0.32f * std::sin (x * 12.56637f + (line == 0 ? 0.0f : 1.0472f));
                const auto point = juce::Point<float> (area.getX() + x * area.getWidth(), area.getY() + y * area.getHeight());
                if (i == 0) path.startNewSubPath (point); else path.lineTo (point);
            }
            if (page == 4 || page == 8)
                g.setColour (line == 0 ? theme::textDim : pink.withAlpha (line == 1 ? 0.4f : 1.0f));
            else
                g.setColour (line == 0 ? pink : amber);
            g.strokePath (path, juce::PathStrokeType (line == 1 && (page == 4 || page == 8) ? 1.0f : 2.0f));
        }
    }
}

std::vector<fxui::Guide::Page> makeGuide (BuiltInStereo& processor)
{
    std::vector<fxui::Guide::Page> pages;
    pages.push_back ({ "Left, right and in between", "What stereo is", juce::String::fromUTF8 (R"stereo(Stereo is two signals at once: one for the left speaker, one for the right. Your ears compare them and place each sound somewhere between the speakers. That picture is called the stereo image.

- The same sound, equally loud in both speakers, seems to come from the middle, where there is no speaker. Engineers call it the phantom centre. A lead vocal, a kick and a bass usually sit there.
- The same sound louder in one speaker moves towards it. That's what a pan knob does. In DawIt, turning the pan to the left leaves the left side as it is and turns the right side down; all the way left, the right side is silent.
- Different sounds in each speaker, like two microphones on a piano, a stereo reverb or a synth pad with chorus, seem to fill the space between the speakers. That's what we call wide.

A stereo track can hold all three at once. DawIt Stereo works on the third kind: how much of a sound differs between left and right, and at which frequencies. It doesn't move sounds left or right; the pan knob does that.

The picture shows three sounds: a vocal in the middle, a guitar panned left and a wide pad, each as its left and right wave and where you'd hear it.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 0, processor); } });
    pages.push_back ({ "Mid and side", "What both speakers share, and what differs", juce::String::fromUTF8 (R"stereo(Any stereo sound can be split another way than into left and right: into what both speakers share, the mid, and what differs between them, the side.

- Mid = (left + right) / 2
- Side = (left - right) / 2

And back again: left = mid + side, right = mid - side. Nothing is lost. It's the same sound, described differently.

Some numbers, for one moment of a sound:
- left 0.8, right 0.4: mid 0.6, side 0.2;
- the same in both, 0.5 and 0.5: mid 0.5, side 0. A mono sound has no side;
- on the left only, 0.6 and 0: mid 0.3, side 0.3, equally big;
- the right upside down, 0.5 and -0.5: mid 0, side 0.5. All side.

So the side holds the width. Turn the side down and the sound gets narrower; at zero, left and right are the same: mono. Turn it up and it gets wider. That's all a width control does, and it's how DawIt Stereo works inside.

Some tools define mid and side without the halves, or divide by 1.41 instead. The proportions are the same; only the numbers' size changes.

The picture shows a short stretch of a wide sound: its left and right on top, its mid and side below.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 1, processor); } });
    pages.push_back ({ "Seeing stereo", "The goniometer and the correlation meter", juce::String::fromUTF8 (R"stereo(Two pictures show stereo at a glance. DawIt Stereo's window has both, for the sound going in and coming out.

The goniometer (also called a vectorscope or phase scope) draws every moment of the sound as a dot. Up and down is the mid, left and right is the side. So:
- a mono sound draws a vertical line;
- a sound on the left only draws a line leaning 45 degrees to the left; on the right only, 45 degrees to the right;
- a wide sound draws a cloud: the wider, the rounder;
- a sound whose left and right are upside down to each other draws a flat line. That's the one to watch: in mono it vanishes.

In DawIt Stereo, the dots below the Mono below frequency are amber and the rest pink, so you see the low end close into a vertical line while the highs keep their cloud.

The correlation meter says much the same in one number, from -1 to +1:
- +1: left and right move together. A mono sound, also when it's panned to one side but still in both speakers.
- 0: left and right have nothing to do with each other, like two microphones far apart, or a very wide reverb.
- -1: left and right are opposite. In mono, everything cancels.

Readings between 0 and +1 are generally safe in mono. The further below 0, the more disappears when left and right are added together. Short dips below 0 in a wide reverb are usually fine; staying there is a sign of a problem.

Meters differ in how quickly they follow the sound; there is no standard. DawIt's follows over about a third of a second, and shows two bars: Low, below the Mono below frequency, and All, the whole sound.

Two caveats. A sound on one side only reads 0, because there's nothing on the other side to compare with; it doesn't cancel in mono. And silence has no reading at all.

The picture shows four sounds as a goniometer, with their correlation: mono, left only, wide, and upside down.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 2, processor); } });
    pages.push_back ({ "Mono: phones, clubs and the sum", "What's left when the two become one", juce::String::fromUTF8 (R"stereo(A lot of music is heard in mono: from a phone with one speaker (phones with two have them only a phone's length apart), many Bluetooth and smart speakers, ceiling speakers in shops, and the subwoofers of many club systems, which get left and right added together. Even with two speakers, someone standing near one of them hears mostly that one.

Mono adds left and right together. With mid and side: (mid + side) + (mid - side) = 2 x mid. The side drops out completely. So in mono:
- what's in the middle stays;
- a sound panned to one side stays, but gets quieter against the middle;
- what's wide loses its side, and gets quieter and duller: reverbs, wide pads and doubled guitars shrink back;
- what's upside down between the speakers cancels and disappears.

For a sound equally loud in both speakers, the correlation tells you how much it drops in mono against a sound in the middle: about 2 dB at 0.2, 3 dB at 0, 6 dB at -0.5 and 16 dB at -0.95.

That's why engineers check their mix in mono now and then. Not to make it sound good in mono, but to make sure nothing important vanishes: the vocal, the kick and the bass should be as clear in mono as in stereo.

Try it: put DawIt Stereo on a bus that every track plays into, choose the Mono check starting point and play the song. The picture shows how much five kinds of part drop in mono.

To measure one clip, open What is sound? > Checking in mono: how many dB it loses in mono, and in which band.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 3, processor); } });
    pages.push_back ({ "Why the low end stays in the middle", "Bass and kick: solid on every system", juce::String::fromUTF8 (R"stereo(In almost any pop, rock, hip-hop or dance record, the kick and the bass come from the middle, and everything below about 100 to 150 Hz is close to mono. There are good reasons.

- You can't hear where deep bass comes from. In a room, sounds below about 100 Hz are hard to place; that's why a home cinema gets away with one subwoofer anywhere in the room. Width down there adds little you hear as width.
- Mono systems lose it. In mono the side drops out, and many club systems play their subwoofers in mono. A bass that's partly side sounds full in your studio and thin in the club. With the low end mono, both are the same.
- Vinyl. A record's groove moves side to side for what both channels share and up and down for what differs. A wide or out-of-phase bass makes the groove deep and shallow in turns, and the needle can jump out. Engineers who cut records make the bass mono below a frequency; the figures they give range from about 100 to 300 Hz.
- Both speakers share the work. The low end holds most of a mix's energy. In the middle it plays equally from both speakers, so neither runs out of room first.
- A solid centre. A kick and a bass in one point sound tighter than the same notes smeared across the stereo image, and they leave the sides free for guitars, keys and reverbs.

Where does a wide low end come from? Stereo synth sounds with detuned voices or chorus, stereo pianos and pads, stereo reverbs without a low cut, a bass recorded with two microphones, and stereo wideners.

This is a habit, not a law. In a classical or acoustic recording made with two microphones in a hall, the room's low end is naturally a little wide, and many engineers leave it. For a song built from tracks, a mono low end is the safe choice.

Mono below doesn't make the mono version any louder: in mono the low side was already gone. It makes the stereo version match the mono one, so what you hear while mixing is what every system plays.

The picture shows a wide pad's mid and side by frequency, before and after Mono below 150 Hz: below the line, the side drops away.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 4, processor); } });
    pages.push_back ({ "Mono below: the crossover", "Splitting the sound at a frequency", juce::String::fromUTF8 (R"stereo(Mono below makes everything under a frequency mono and leaves the rest alone. To do that, DawIt Stereo first splits the sound in two with a crossover: a low band below the frequency and a high band above it. The low band's left and right are made the same (its side is removed). The high band keeps its width, set by Width. Then the two are added back together.

No filter is a brick wall. The Slope says how quickly the change happens:
- 12 dB per octave: gentle. An octave below the frequency, the side is only 14 dB down; it fades out over a few octaves.
- 24 dB per octave: the usual choice. An octave below, the side is 25 dB down; two octaves below, 48 dB.
- 48 dB per octave: sharp. An octave below, 48 dB down. For when the mono part should stop close to the frequency.

At the frequency itself, each band is at half its level (-6 dB), so the side there is half as big as it was.

The crossover is a Linkwitz-Riley design, named after the two engineers who described it in 1976 for loudspeakers. Its two bands add back to a flat response: a sound that's already mono comes out at the same level at every frequency. Like every crossover of its kind, it shifts the timing (the phase) of the low end a little, the same in both speakers. You won't hear that on its own. It matters only when the same sound also plays on another track without the effect, like two microphones on one bass: then put DawIt Stereo on the bus they share. At Off, the crossover is out of the way and the sound passes untouched.

Typical frequencies: 100 to 150 Hz on a mix or a bus, 150 to 250 Hz on a bass whose body should be mono. Above about 300 Hz you start narrowing things whose width you can hear: guitars, the left hand of a stereo piano, the body of a pad.

The picture shows the two bands for the current slope and frequency, and their sum: the straight line on top.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 5, processor); } });
    pages.push_back ({ "Width: narrower and wider", "Turning the side down or up", juce::String::fromUTF8 (R"stereo(Width turns the side up or down above the Mono below frequency (everywhere, when Mono below is Off). The mid is left as it is.
- 0%: no side, so left and right are the same: mono.
- 100%: as it was.
- 200%: the side twice as big (+6 dB): much wider.

Three things change with it.
- Loudness. More side adds level. For a sound whose left and right are unrelated, 150% makes it about 2 dB louder and 200% about 4 dB; 0% makes it 3 dB quieter. Louder sounds better at first hearing, so Match level, on from the start, keeps the level the same and you judge only the width.
- Correlation. A pad that reads 0.6 drops to about 0.3 at 150% and to 0 at 200%. Unrelated left and right (0) go to -0.4 at 150%.
- Stereo against mono. Left plus right stays exactly the same whatever the Width, because the side cancels whatever its size (with Match level off; with it on, the whole sound is turned down a little to keep the loudness). But the stereo version now has more side than the mono one, so a part you've widened is louder in stereo than in mono. The wider you go, the more it shrinks back on a phone.

Width only works with what already differs between left and right. A mono sound has no side, so Width does nothing to it. To make a mono sound wide, give each side something different: a stereo reverb, a short delay on one side (What is delay?), or a second take of the part panned the other way. Those change the mono sound too (a short delay added to the sound gives comb filtering), which this Width never does.

Typical settings: 80 to 100% to calm a busy stereo sound, 110 to 140% to open up pads, backing vocals or a reverb return. Engineers rarely go past 150%.

The picture shows how the level and the correlation of two sounds change as Width goes from 0 to 200%: a pad (correlation 0.6) and unrelated noise (0).)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 6, processor); } });
    pages.push_back ({ "Where to use it", "Mix bus, bass, pads, vinyl", juce::String::fromUTF8 (R"stereo(- The whole mix, on a bus that every track plays into: Mono below 120 Hz at 24 dB per octave, Width 100%. It tidies whatever wide low end slipped in from pads, reverbs and stereo samples.
- A synth bass: Mono below 150 to 250 Hz, so the notes stay centred while the bright, chorused top keeps its width.
- A kick, a bass or a single voice that came as a stereo file: Width 0% makes it fully mono, one solid point that you then place with the pan knob.
- Pads, keys and stereo reverbs: Mono below 150 to 200 Hz, and Width 110 to 150% if they should spread out behind the vocal.
- A mix for vinyl: Mono below about 150 Hz. Ask the cutting engineer, who decides in the end.
- Checking: the Mono check starting point plays everything in mono, as a phone would. Switch Listen back to Stereo before you export.

Where it isn't needed: on a track that's already mono (nothing changes; Your track says so), on something you want wide down low on purpose, like an ambient pad or a recording of a room, and as a cure for a sound that cancels in mono because two microphones are out of step. Mono below hides that in the low end; fixing the polarity or the timing (What is sound? > Phase cancellation) fixes it everywhere.

The picture shows the starting points side by side: where their Mono below sits and how wide they leave the rest.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 7, processor); } });
    pages.push_back ({ "Too much", "When narrowing or widening goes wrong", juce::String::fromUTF8 (R"stereo(- Mono below set too high. Above about 300 Hz you hear width clearly, so a stereo piano, guitars or a pad lose their spread and the mix gets narrow. Turn it down until you hear the width come back, then a little lower.
- Too much width. Past about 150%, sounds get hollow and phasey and seem to come from beyond the speakers or from inside your head, and the correlation sits below 0. In mono they drop back much more than the rest.
- Widening the low end. Width over 100% with Mono below Off widens the bass too, usually the opposite of what you want. Keep Mono below on when you widen.
- Narrowing what's already mono: nothing happens.
- Mixing for mono. Make the stereo mix good, and use mono to check that nothing disappears.
- Judging by loudness. A wider sound is louder; with Match level off, compare at the same level.

The picture shows a wide pad with Mono below at 400 Hz: much more than the bass has lost its width.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 8, processor); } });
    pages.push_back ({ "How DawIt Stereo works", "From crossover to output", juce::String::fromUTF8 (R"stereo(The sound goes through these steps:
1. Crossover: a Linkwitz-Riley filter splits left and right at the Mono below frequency into a low and a high band (skipped when Mono below is Off).
2. Low band: left and right both become their average, the mid. The low end is now mono.
3. High band: split into mid and side; the side is multiplied by Width; then back to left and right.
4. The two bands are added together.
5. Listen: Stereo plays the result. Mono plays its mid in both speakers, as a mono system would. Side plays only what differs, in both speakers, so you hear what the width is made of.
6. Match level compares the loudness going in and coming out over about half a second and evens it out (not in Mono and Side, which are for checking).
7. Output: a last level, -12 to +12 dB.

DawIt Stereo sits in the chain before the track's fader and pan, so on a track it hears the sound before the pan knob places it: a mono track panned to one side is still mono here. On a bus it hears the tracks after their pans. It adds no delay, so the track stays in time with the others. On a track with only one channel it does nothing.

The meters measure before and after: the goniometers, the correlation of the whole sound and of the part below the Mono below frequency (below 120 Hz when it's Off), and the mid and side by frequency.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 9, processor); } });
    pages.push_back ({ "Your track", "How wide your track is, down low and overall", juce::String::fromUTF8 (R"stereo(Play the song where this track plays. DawIt Stereo measures what comes in over the last 10 seconds of sound (quiet moments don't count) and says what it sees: how wide the low end is, whether anything cancels in mono, and what your settings do to it.

It measures how far the side is under the mid, below the Mono below frequency (below 120 Hz when it's Off) and for the whole sound, and the correlation of both, going in and coming out. The picture shows them, with the goniometers.

Opened from the Theory Guides window without a track, this page uses a stand-in: put DawIt Stereo on a track and open the guide from its window.)stereo"),
        [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, 10, processor); } });
    const auto introduction = pages.back().text;
    pages.back().liveText = [&processor, introduction]
    {
        return introduction + "\n\n" + stereo::analyse (processor.inputMeasurement.getBlocks(), processor.outputMeasurement.getBlocks(),
                                                        processor.getSettings()).joinIntoString ("\n\n");
    };
    return pages;
}
}
