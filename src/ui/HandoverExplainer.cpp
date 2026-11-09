#include "HandoverExplainer.h"
#include "LoudnessActions.h"
#include "engine/RecordingGuide.h"

namespace daw
{
namespace hg = handoverguide;

namespace
{
const juce::Colour stemColours[] { juce::Colour (0xffe5744f), juce::Colour (0xff5fd38d), juce::Colour (0xfff2c94c), juce::Colour (0xff4fa3ff), juce::Colour (0xffc58bff) };
const juce::Colour dawColour = juce::Colour (0xff4fa3ff), yourColour = juce::Colour (0xfff2c94c);

void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour colour,
           juce::Justification just = juce::Justification::centredLeft, bool bold = false)
{
    g.setColour (colour);
    g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText (s, r.toNearestInt(), just, 2, 0.75f);
}

void tile (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label, juce::Colour edge, float size = 12.0f, bool dim = false)
{
    g.setColour (theme::panel.withAlpha (dim ? 0.5f : 1.0f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (edge.withAlpha (dim ? 0.4f : 1.0f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.2f);
    text (g, label, r.reduced (6.0f, 1.0f), size, dim ? theme::textDim : theme::text, juce::Justification::centredLeft);
}

void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, juce::Colour colour)
{
    g.setColour (colour);
    g.drawLine (juce::Line<float> (from, to), 1.5f);
    const auto angle = (float) std::atan2 (to.y - from.y, to.x - from.x);
    juce::Path head;
    head.addTriangle (to, to + juce::Point<float> (-6.0f, -4.0f).rotatedAboutOrigin (angle), to + juce::Point<float> (-6.0f, 4.0f).rotatedAboutOrigin (angle));
    g.fillPath (head);
}

/** One lane of a "files along the song" picture: positions are fractions of the file's length. */
struct LaneDraw
{
    juce::String name, subtitle, file, tooltip;
    juce::Colour colour;
    std::vector<juce::Range<double>> blocks;
};
} // namespace

//==============================================================================
/** The topic's picture: the example song for topics 1 to 8, the user's song for the last. */
class HandoverExplainer::Picture final : public juce::Component,
                                         public juce::TooltipClient
{
public:
    void setState (hg::Topic t, const hg::SongView& v, const hg::Inputs& in)
    {
        topic = t;
        view = v;
        inputs = in;
        repaint();
    }

    juce::String getTooltip() override
    {
        if (topic != hg::Topic::yourSong)
            return "The picture of the topic, drawn for an example pop song of 5 stems and 13 tracks.";

        const auto p = getMouseXYRelative().toFloat();

        if (inputs.hasPeak && levelBar.contains (p))
            return "Your mix peaks at " + juce::String (inputs.peakDb, 1) + " dBFS (last Measure Loudness, " + inputs.peakScope + ").";

        for (int i = 0; i < (int) laneRects.size() && i < laneTips.size(); ++i)
            if (laneRects[(size_t) i].contains (p))
                return laneTips[i];

        return "Your song as the files Bounce to WAV would make: one lane per stem.";
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        laneRects.clear();
        laneTips.clear();
        levelBar = {};
        const auto area = getLocalBounds().toFloat().reduced (16.0f, 12.0f);

        switch (topic)
        {
            case hg::Topic::stems:       paintStems (g, area); break;
            case hg::Topic::mixOrMaster: paintFolders (g, area); break;
            case hg::Topic::preparing:   paintPreparing (g, area); break;
            case hg::Topic::headroom:    paintHeadroom (g, area); break;
            case hg::Topic::effects:     paintEffects (g, area); break;
            case hg::Topic::checking:    paintChecking (g, area); break;
            case hg::Topic::sendWith:    paintSendWith (g, area); break;
            case hg::Topic::working:     paintWorking (g, area); break;
            case hg::Topic::yourSong:    paintSong (g, area); break;
        }
    }

private:
    static std::vector<LaneDraw> exampleLanes()
    {
        std::vector<LaneDraw> out;
        const double total = hg::getExampleSecondsPerFile() * 2.0 / 4.0;   // the file's length in bars, tail included
        int i = 0;

        for (auto& s : hg::getExampleStems())
        {
            LaneDraw l;
            l.name = s.name;
            l.subtitle = s.tracks.joinIntoString (", ");
            l.file = s.fileName;
            l.colour = stemColours[i++ % 5];

            for (auto& b : s.bars)
                l.blocks.push_back ({ (b.getStart() - 1) / total, b.getEnd() / total });

            out.push_back (l);
        }

        return out;
    }

    void paintStems (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const auto stems = hg::getExampleStems();
        const float colW = (a.getWidth() - 60.0f) / 3.0f;
        const float top = a.getY() + 24.0f;
        const float rowH = (a.getBottom() - top - 4.0f) / (float) hg::exampleNumTracks;
        const juce::String headings[] { "The mix: 1 file", "Stems: 5 files", "Multitracks: 13 files" };

        for (int c = 0; c < 3; ++c)
            text (g, headings[c], { a.getX() + (float) c * (colW + 30.0f), a.getY(), colW, 20.0f }, 14.0f, theme::text, juce::Justification::centredLeft, true);

        tile (g, { a.getX(), top, colW, rowH * 3.0f }, "Mix.wav", dawColour, 14.0f);
        text (g, "the whole song, as you hear it", { a.getX(), top + rowH * 3.0f + 2.0f, colW, 20.0f }, 12.0f, theme::textDim);

        float y = top;
        int stemIndex = 0, fileNumber = 0;

        for (auto& s : stems)
        {
            const int n = juce::jmax (1, s.tracks.size());
            const auto stemR = juce::Rectangle<float> (a.getX() + colW + 30.0f, y, colW, rowH * (float) n - 3.0f);
            tile (g, stemR, s.fileName, stemColours[stemIndex], 12.5f);

            for (int k = 0; k < n; ++k)
            {
                const auto trackR = juce::Rectangle<float> (a.getX() + 2.0f * (colW + 30.0f), y + rowH * (float) k, colW, rowH - 3.0f);
                tile (g, trackR, juce::String (++fileNumber).paddedLeft ('0', 2) + " " + (s.tracks.isEmpty() ? s.name : s.tracks[k]) + ".wav",
                      stemColours[stemIndex], 11.5f);
            }

            g.setColour (stemColours[stemIndex].withAlpha (0.5f));
            g.drawLine (stemR.getRight() + 3.0f, stemR.getY() + 2.0f, a.getX() + 2.0f * (colW + 30.0f) - 3.0f, stemR.getY() + 2.0f);
            y += rowH * (float) n;
            ++stemIndex;
        }
    }

    void paintFolders (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const float colW = (a.getWidth() - 30.0f) / 2.0f;
        const auto left = juce::Rectangle<float> (a.getX(), a.getY(), colW, a.getHeight());
        const auto right = left.withX (a.getX() + colW + 30.0f);

        for (auto [r, name] : { std::pair { left, juce::String ("To a mixing engineer") }, std::pair { right, juce::String ("To a mastering engineer") } })
        {
            g.setColour (theme::panel);
            g.fillRoundedRectangle (r, 6.0f);
            text (g, name, r.reduced (12.0f, 6.0f).removeFromTop (24.0f), 14.0f, theme::text, juce::Justification::centredLeft, true);
        }

        const float h = 30.0f;
        auto l = left.reduced (14.0f, 0.0f).withTrimmedTop (38.0f);
        tile (g, l.removeFromTop (h), "13 multitracks (as you hear them, dry, or both)", dawColour);
        l.removeFromTop (6.0f);
        tile (g, l.removeFromTop (h), "Rough mix - do not mix from.wav", dawColour);
        l.removeFromTop (6.0f);
        tile (g, l.removeFromTop (h), "About these files.txt  and  Tempo map.mid", dawColour);
        l.removeFromTop (6.0f);
        tile (g, l.removeFromTop (h), "Notes.txt and References.txt (yours)", yourColour);

        auto r = right.reduced (14.0f, 0.0f).withTrimmedTop (38.0f);
        tile (g, r.removeFromTop (h), "The premaster: one stereo WAV", dawColour);
        r.removeFromTop (6.0f);
        tile (g, r.removeFromTop (h), "only if asked: 4 stems", dawColour, 12.0f, true);
        r.removeFromTop (6.0f);
        tile (g, r.removeFromTop (h), "Reference (a loud copy, if you made one)", yourColour);
        r.removeFromTop (6.0f);
        tile (g, r.removeFromTop (h), "Notes.txt (yours)", yourColour);
    }

    /** Lanes over the song with bars below, a tail shaded at the end and a "starts at bar 1" mark. */
    void drawLanes (juce::Graphics& g, juce::Rectangle<float> a, const std::vector<LaneDraw>& lanes, double totalBars, double tailFraction, bool recordTips,
                    double barsPerFraction)
    {
        const float labelW = 150.0f, fileW = 118.0f;
        const float x0 = a.getX() + labelW, x1 = a.getRight() - fileW;
        const float laneH = juce::jmin (46.0f, (a.getHeight() - 26.0f) / (float) juce::jmax (1, (int) lanes.size()));
        float y = a.getY();

        for (auto& l : lanes)
        {
            const auto row = juce::Rectangle<float> (a.getX(), y, a.getWidth(), laneH - 4.0f);
            g.setColour (theme::lane);
            g.fillRect (juce::Rectangle<float> (x0, row.getY(), x1 - x0, row.getHeight()));

            for (auto& b : l.blocks)
            {
                g.setColour (l.colour.withAlpha (0.85f));
                g.fillRect (juce::Rectangle<float> (x0 + (float) b.getStart() * (x1 - x0), row.getY() + 3.0f, juce::jmax (2.0f, (float) b.getLength() * (x1 - x0)), row.getHeight() - 6.0f));
            }

            text (g, l.name, row.withWidth (labelW - 6.0f).withTrimmedBottom (row.getHeight() * 0.45f), 13.0f, theme::text, juce::Justification::centredLeft, true);
            text (g, l.subtitle, row.withWidth (labelW - 6.0f).withTrimmedTop (row.getHeight() * 0.5f), 10.5f, theme::textDim);
            text (g, l.file, row.withTrimmedLeft (row.getWidth() - fileW + 6.0f), 12.0f, theme::textDim, juce::Justification::centredLeft);

            if (recordTips)
            {
                laneRects.push_back (row);
                laneTips.add (l.tooltip);
            }

            y += laneH;
        }

        const float bottom = y;
        g.setColour (theme::accent.withAlpha (0.12f));
        const float tailW = (float) tailFraction * (x1 - x0);
        g.fillRect (juce::Rectangle<float> (x1 - tailW, a.getY(), tailW, bottom - a.getY() - 4.0f));
        text (g, "4 s tail", { x1 - 60.0f, bottom - 2.0f, 60.0f, 14.0f }, 10.5f, theme::textDim, juce::Justification::centredRight);

        for (int bar = 0; bar <= (int) totalBars; bar += juce::jmax (1, (int) std::ceil (totalBars / 8.0)))
        {
            const float x = x0 + (float) ((double) bar / barsPerFraction) * (x1 - x0);
            g.setColour (theme::gridBar);
            g.drawVerticalLine (juce::roundToInt (x), bottom - 4.0f, bottom);
            if (x > x1 - 66.0f)
                continue;   // keeps the last numbers clear of the "4 s tail" label

            text (g, juce::String (bar + 1), { x - 14.0f, bottom, 28.0f, 14.0f }, 10.5f, theme::textDim, juce::Justification::centred);
        }

        laneBottom = bottom;
        laneX0 = x0;
        laneX1 = x1;
    }

    void paintPreparing (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const double fileBars = hg::getExampleSecondsPerFile() * 2.0 / 4.0;
        drawLanes (g, a.withTrimmedBottom (62.0f), exampleLanes(), hg::exampleNumBars, bounce::filesTailSeconds / hg::getExampleSecondsPerFile(), false, fileBars);
        text (g, "every file starts at bar 1", { laneX0, laneBottom + 16.0f, 220.0f, 16.0f }, 12.0f, dawColour, juce::Justification::centredLeft, true);
        text (g, "every file is " + hg::describeSeconds (hg::getExampleSecondsPerFile()) + " long", { laneX1 - 240.0f, laneBottom + 16.0f, 240.0f, 16.0f }, 12.0f, dawColour,
              juce::Justification::centredRight, true);
        text (g, "WAV  -  24-bit  -  " + hg::describeRate (48000.0) + " (the project's rate)  -  no dither  -  not normalised",
              { a.getX(), a.getBottom() - 24.0f, a.getWidth(), 22.0f }, 13.0f, theme::text, juce::Justification::centred, true);
    }

    void paintHeadroom (juce::Graphics& g, juce::Rectangle<float> a)
    {
        struct Mix { float peak; juce::String label, note; juce::Colour colour; };
        const Mix mixes[] { { -0.3f, "Peaks at -0.3 dBFS", "too hot", theme::meterRed }, { -6.0f, "Peaks at -6.0 dBFS", "the usual target", theme::meterGreen },
                            { -14.0f, "Peaks at -14.0 dBFS", "fine", theme::meterGreen } };
        const float labelW = 150.0f, noteW = 130.0f, x0 = a.getX() + labelW, x1 = a.getRight() - noteW;
        const float top = a.getY() + 22.0f, rowH = (a.getHeight() - 22.0f) / 3.0f;
        const auto toX = [x0, x1] (float db) { return x0 + (db + 30.0f) / 30.0f * (x1 - x0); };

        for (float line : { 0.0f, -3.0f, -6.0f })
        {
            g.setColour (theme::gridBar);
            g.drawVerticalLine (juce::roundToInt (toX (line)), top - 4.0f, a.getBottom());
            text (g, juce::String (line, 0) + " dB", { toX (line) - 30.0f, a.getY(), 60.0f, 16.0f }, 11.0f, theme::textDim, juce::Justification::centred);
        }

        for (int i = 0; i < 3; ++i)
        {
            const auto row = juce::Rectangle<float> (a.getX(), top + (float) i * rowH, a.getWidth(), rowH - 10.0f);
            g.setColour (theme::lane);
            g.fillRect (juce::Rectangle<float> (x0, row.getY(), x1 - x0, row.getHeight()));
            const auto bar = juce::Rectangle<float> (x0, row.getY() + 6.0f, toX (mixes[i].peak) - x0, row.getHeight() - 12.0f);
            g.setColour (mixes[i].colour.withAlpha (0.85f));
            g.fillRect (bar);

            if (i == 0)   // a flattened top: the loud peaks have been cut off
            {
                g.setColour (theme::text);
                g.fillRect (juce::Rectangle<float> (bar.getRight() - 6.0f, bar.getY(), 6.0f, bar.getHeight()));
            }

            text (g, mixes[i].label, row.withWidth (labelW - 6.0f), 12.5f, theme::text);
            text (g, mixes[i].note, row.withTrimmedLeft (row.getWidth() - noteW + 8.0f), 13.0f, mixes[i].colour, juce::Justification::centredLeft, true);
        }
    }

    void paintEffects (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const juce::String parts[] { "Clip", "DawIt EQ", "DawIt Compressor", "Fader", "Send to Reverb" };
        const float half = (a.getHeight() - 16.0f) / 2.0f;
        const float gap = 22.0f, w = (a.getWidth() - 4.0f * gap) / 5.0f;

        for (int row = 0; row < 2; ++row)
        {
            const float y = a.getY() + (float) row * (half + 16.0f);
            text (g, row == 0 ? "As you hear it: Lead vocal.wav holds all of it" : "Dry: Lead vocal.wav in the dry folder holds only the clip", { a.getX(), y, a.getWidth(), 18.0f }, 13.5f, theme::text,
                  juce::Justification::centredLeft, true);

            for (int i = 0; i < 5; ++i)
            {
                const auto r = juce::Rectangle<float> (a.getX() + (float) i * (w + gap), y + 26.0f, w, half - 40.0f);
                const bool lit = row == 0 || i == 0;
                tile (g, r, parts[i], lit ? dawColour : theme::textDim, 12.5f, ! lit);

                if (i < 4)
                    arrow (g, { r.getRight() + 2.0f, r.getCentreY() }, { r.getRight() + gap - 3.0f, r.getCentreY() }, lit ? dawColour : theme::textDim.withAlpha (0.4f));
            }
        }
    }

    void drawWave (juce::Graphics& g, juce::Rectangle<float> r, float amplitude, juce::Colour colour)
    {
        juce::Path p;

        for (int i = 0; i <= 200; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / 200.0f;
            const float v = amplitude * (0.6f * std::sin ((float) i * 0.35f) + 0.4f * std::sin ((float) i * 0.91f + 1.0f));
            const float y = r.getCentreY() - v * r.getHeight() * 0.5f;

            if (i == 0)
                p.startNewSubPath (x, y);
            else
                p.lineTo (x, y);
        }

        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.4f));
    }

    void paintChecking (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const float half = (a.getWidth() - 30.0f) / 2.0f;
        const auto left = juce::Rectangle<float> (a.getX(), a.getY(), half, a.getHeight());
        const auto right = left.withX (a.getX() + half + 30.0f);
        const float rowH = (a.getHeight() - 24.0f) / 3.0f;

        text (g, "Stems summed, the mix flipped", left.withHeight (20.0f), 13.5f, theme::text, juce::Justification::centredLeft, true);
        drawWave (g, { left.getX(), left.getY() + 40.0f, half, rowH - 22.0f }, 1.0f, dawColour);
        text (g, "stems added up", { left.getX(), left.getY() + 26.0f, half, 14.0f }, 10.5f, theme::textDim);
        drawWave (g, { left.getX(), left.getY() + 40.0f + rowH, half, rowH - 22.0f }, -1.0f, yourColour);
        text (g, "the mix, polarity flipped", { left.getX(), left.getY() + 26.0f + rowH, half, 14.0f }, 10.5f, theme::textDim);
        drawWave (g, { left.getX(), left.getY() + 40.0f + 2.0f * rowH, half, rowH - 22.0f }, 0.0f, theme::meterGreen);
        text (g, "their sum: silence", { left.getX(), left.getY() + 26.0f + 2.0f * rowH, half, 14.0f }, 10.5f, theme::meterGreen);

        text (g, "A bus with a compressor", right.withHeight (20.0f), 13.5f, theme::text, juce::Justification::centredLeft, true);
        drawWave (g, { right.getX(), right.getY() + 40.0f, half, rowH - 22.0f }, 1.0f, dawColour);
        drawWave (g, { right.getX(), right.getY() + 40.0f + rowH, half, rowH - 22.0f }, -0.93f, yourColour);
        drawWave (g, { right.getX(), right.getY() + 40.0f + 2.0f * rowH, half, rowH - 22.0f }, 0.07f, theme::meterYellow);
        text (g, "a bus compressor: a quiet remainder (an illustration)", { right.getX(), right.getY() + 26.0f + 2.0f * rowH, half, 14.0f }, 10.5f, theme::meterYellow);
    }

    void paintSendWith (juce::Graphics& g, juce::Rectangle<float> a)
    {
        text (g, "Summer Song 120bpm Am", a.withHeight (22.0f), 14.0f, theme::text, juce::Justification::centredLeft, true);
        const juce::String dawFiles[] { "01 Drums.wav", "02 Bass.wav", "03 Guitars.wav", "04 Keys.wav", "05 Vocals.wav", "About these files.txt", "Tempo map.mid" };
        const juce::String yours[] { "Rough mix - do not mix from.wav", "Notes.txt", "References.txt" };
        const float rowH = 28.0f, w = (a.getWidth() - 24.0f) / 2.0f;
        float y = a.getY() + 30.0f;

        for (auto& f : dawFiles)
        {
            tile (g, { a.getX(), y, w, rowH - 4.0f }, f, dawColour, 12.5f);
            y += rowH;
        }

        y = a.getY() + 30.0f;

        for (auto& f : yours)
        {
            tile (g, { a.getX() + w + 24.0f, y, w, rowH - 4.0f }, f, yourColour, 12.5f);
            y += rowH;
        }

        text (g, "DawIt writes the blue files", { a.getX() + w + 24.0f, y + 12.0f, w, 18.0f }, 12.5f, dawColour, juce::Justification::centredLeft, true);
        text (g, "you add the yellow ones", { a.getX() + w + 24.0f, y + 32.0f, w, 18.0f }, 12.5f, yourColour, juce::Justification::centredLeft, true);
    }

    void paintWorking (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const juce::String steps[] { "Files sent", "Mix 1", "Your notes", "Mix 2", "Your notes", "Mix 3", "Final" };
        const float gap = 18.0f, w = (a.getWidth() - 6.0f * gap) / 7.0f, y = a.getCentreY() - 30.0f;

        for (int i = 0; i < 7; ++i)
        {
            const bool notes = steps[i] == "Your notes";
            const auto r = juce::Rectangle<float> (a.getX() + (float) i * (w + gap), y, w, 56.0f);
            tile (g, r, steps[i], notes ? yourColour : dawColour, 12.5f);

            if (i < 6)
                arrow (g, { r.getRight() + 1.0f, r.getCentreY() }, { r.getRight() + gap - 2.0f, r.getCentreY() }, theme::textDim);
        }

        const float x0 = a.getX() + 2.0f * (w + gap), x1 = a.getX() + 5.0f * (w + gap) + w;
        g.setColour (theme::textDim);
        g.drawLine (x0, y + 72.0f, x1, y + 72.0f);
        g.drawLine (x0, y + 66.0f, x0, y + 72.0f);
        g.drawLine (x1, y + 66.0f, x1, y + 72.0f);
        text (g, "usually included: 2 or 3 rounds", { x0, y + 76.0f, x1 - x0, 18.0f }, 12.5f, theme::textDim, juce::Justification::centred);
    }

    void paintSong (juce::Graphics& g, juce::Rectangle<float> a)
    {
        if (view.lanes.empty())
        {
            text (g, view.advice.isEmpty() ? juce::String ("Your song has no clips to bounce yet.") : view.advice[0], a, 14.0f, theme::textDim, juce::Justification::centred);
            return;
        }

        const float barW = 112.0f;
        const auto laneArea = a.withTrimmedRight (barW + 12.0f);
        const double songSeconds = view.secondsPerFile - bounce::filesTailSeconds;
        const double beatsPerBar = 4.0;
        const double bars = view.songBeats / beatsPerBar;
        const double toFraction = view.songBeats > 0.0 ? songSeconds / view.secondsPerFile / view.songBeats : 0.0;
        std::vector<LaneDraw> lanes;
        int i = 0;

        for (auto& l : view.lanes)
        {
            LaneDraw d;
            d.name = l.name;
            d.subtitle = l.tracks.joinIntoString (", ");
            d.file = l.fileName;
            d.colour = l.colour.isTransparent() ? stemColours[i % 5] : l.colour;
            ++i;

            for (auto& r : l.ranges)
                d.blocks.push_back ({ r.getStart() * toFraction, r.getEnd() * toFraction });

            const double barsPerBeat = 1.0 / beatsPerBar;
            juce::StringArray parts;

            for (auto& r : l.ranges)
                parts.add (juce::String (juce::roundToInt (r.getStart() * barsPerBeat) + 1) + " to " + juce::String (juce::roundToInt (r.getEnd() * barsPerBeat)));

            d.tooltip = l.name + " stem" + (l.tracks.isEmpty() ? juce::String() : " - " + l.tracks.joinIntoString (", ")) + " - " + l.fileName + " - clips in bars "
                        + (parts.isEmpty() ? juce::String ("none (the file is silent)") : parts.joinIntoString (", "));
            lanes.push_back (d);
        }

        const double fileBars = view.secondsPerFile / songSeconds * bars;
        drawLanes (g, laneArea.withTrimmedBottom (view.skipped.empty() ? 0.0f : 20.0f), lanes, bars, bounce::filesTailSeconds / view.secondsPerFile, true, fileBars);

        if (! view.skipped.empty())
        {
            juce::StringArray names;

            for (auto& s : view.skipped)
                names.add (s.name + " (" + s.reason + ")");

            text (g, "Left out: " + names.joinIntoString (", "), { laneArea.getX(), laneArea.getBottom() - 18.0f, laneArea.getWidth(), 18.0f }, 11.5f, theme::textDim);
        }

        // The mix's last measured peak, as a vertical bar from -30 to +3 dBFS.
        const auto bar = juce::Rectangle<float> (a.getRight() - barW + 40.0f, a.getY() + 40.0f, 26.0f, a.getHeight() - 62.0f);
        levelBar = bar.expanded (30.0f, 8.0f);
        const auto toY = [&bar] (float db) { return bar.getBottom() - (juce::jlimit (-30.0f, 3.0f, db) + 30.0f) / 33.0f * bar.getHeight(); };
        text (g, "Mix peak", { a.getRight() - barW, a.getY(), barW, 18.0f }, 12.5f, theme::text, juce::Justification::centred, true);
        g.setColour (theme::lane);
        g.fillRect (bar);

        if (inputs.hasPeak)
        {
            g.setColour (inputs.peakDb > 0.0f ? theme::meterRed : inputs.peakDb > -3.0f ? theme::meterYellow : theme::meterGreen);
            g.fillRect (juce::Rectangle<float> (bar.getX(), toY (inputs.peakDb), bar.getWidth(), bar.getBottom() - toY (inputs.peakDb)));
            text (g, juce::String (inputs.peakDb, 1) + " dBFS", { a.getRight() - barW, a.getY() + 17.0f, barW, 16.0f }, 12.0f, theme::text, juce::Justification::centred, true);
        }
        else
        {
            text (g, "Measure Loudness to see the peak", bar.reduced (-34.0f, 0.0f).withTrimmedTop (60.0f).withHeight (90.0f), 11.5f, theme::textDim, juce::Justification::centred);
        }

        for (float line : { 0.0f, -3.0f, -6.0f })
        {
            g.setColour (theme::text.withAlpha (0.55f));
            g.drawHorizontalLine (juce::roundToInt (toY (line)), bar.getX() - 6.0f, bar.getRight() + 6.0f);
            text (g, juce::String (line, 0), { bar.getX() - 40.0f, toY (line) - 8.0f, 32.0f, 16.0f }, 10.5f, theme::textDim, juce::Justification::centredRight);
        }
    }

    hg::Topic topic = hg::Topic::stems;
    hg::SongView view;
    hg::Inputs inputs;
    std::vector<juce::Rectangle<float>> laneRects;
    juce::StringArray laneTips;
    juce::Rectangle<float> levelBar;
    float laneBottom = 0.0f, laneX0 = 0.0f, laneX1 = 0.0f;
};

//==============================================================================
HandoverExplainer::HandoverExplainer (Engine& e) : engine (e)
{
    title.setText ("Sending stems to an engineer", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture>();
    addAndMakeVisible (*picture);

    explanation.setMultiLine (true, true);
    explanation.setReadOnly (true);
    explanation.setCaretVisible (false);
    explanation.setScrollbarsShown (true);
    explanation.setFont (juce::FontOptions (15.0f));
    explanation.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    explanation.setColour (juce::TextEditor::textColourId, theme::text);
    explanation.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    explanation.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    explanation.setBorder (juce::BorderSize<int> (10, 12, 10, 12));
    addAndMakeVisible (explanation);

    bounceButton.setTooltip ("Open Bounce to WAV (Cmd+B) to make the files.");
    bounceButton.onClick = [this] { if (onShowBounce) onShowBounce(); };
    addChildComponent (bounceButton);
    addAndMakeVisible (exportButton);

    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
    engine.addChangeListener (this);
    startTimerHz (10);
}

HandoverExplainer::~HandoverExplainer()
{
    engine.removeChangeListener (this);
    list.setModel (nullptr);
}

juce::String HandoverExplainer::getPrintSource() const
{
    return "the pop example song";
}

int HandoverExplainer::getNumPrintTopics() const
{
    return (int) std::size (hg::allTopics) + 1;
}

booklet::Topic HandoverExplainer::showPrintTopic (int index)
{
    if (GuideSourcesTopic::isRow (index, (int) std::size (hg::allTopics)))
        return sources.getPrintTopic();

    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 2, index);
    show (index);
    return { hg::getTopicName (hg::allTopics[index]), hg::getTopicSummary (hg::allTopics[index]), explanation.getText(),
             { picture->getWidth(), picture->getHeight() }, {} };
}

juce::Image HandoverExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (*picture, scale);
}

void HandoverExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void HandoverExplainer::showTopic (hg::Topic topic)
{
    for (int i = 0; i < (int) std::size (hg::allTopics); ++i)
        if (hg::allTopics[i] == topic)
            list.selectRow (i);
}

int HandoverExplainer::getNumRows()
{
    return (int) std::size (hg::allTopics) + 1;
}

void HandoverExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (hg::allTopics)))
    {
        sources.paintRow (g, w, h, selected);
        return;
    }

    if (row < 0 || row >= getNumRows())
        return;

    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    const auto topic = hg::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (hg::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (hg::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void HandoverExplainer::selectedRowsChanged (int row)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (hg::allTopics)))
    {
        showSources();
        return;
    }

    if (row >= 0)
        show (row);
}

void HandoverExplainer::showSources()
{
    bounceButton.setVisible (false);
    sources.select (*this, list, (int) std::size (hg::allTopics), { &list, &title, &exportButton });
}

hg::Inputs HandoverExplainer::makeInputs() const
{
    hg::Inputs in;

    if (auto* settings = engine.properties.getUserSettings())
    {
        in.bounceRate = settings->getDoubleValue ("export.bounceRate", 44100.0);
        in.bounceBits = settings->getIntValue ("export.bounceBits", 24);
    }

    if (const auto last = loudnessui::getLast(); last && last->readings.samplePeak)
    {
        in.hasPeak = true;
        in.peakDb = (float) *last->readings.samplePeak;
        // "The whole song (bar 1 to bar 105, 3:34):" gives "bar 1 to bar 105".
        in.peakScope = last->scope.fromFirstOccurrenceOf ("(", false, false).upToFirstOccurrenceOf (",", false, false);
    }

    return in;
}

void HandoverExplainer::timerCallback()
{
    if (hg::allTopics[juce::jlimit (0, (int) std::size (hg::allTopics) - 1, shownRow)] != hg::Topic::yourSong || sources.isShown())
        return;

    const auto in = makeInputs();
    const bool inputsChanged = ! haveInputs || ! juce::approximatelyEqual (in.bounceRate, lastInputs.bounceRate) || in.bounceBits != lastInputs.bounceBits
                               || in.hasPeak != lastInputs.hasPeak || ! juce::approximatelyEqual (in.peakDb, lastInputs.peakDb) || in.peakScope != lastInputs.peakScope;

    if (dirty || inputsChanged)
        refresh (false);
}

void HandoverExplainer::refresh (bool force)
{
    const auto topic = hg::allTopics[juce::jlimit (0, (int) std::size (hg::allTopics) - 1, shownRow)];
    hg::SongView view;
    const auto in = makeInputs();
    auto body = hg::explain (topic);

    if (topic == hg::Topic::yourSong)
    {
        view = hg::analyse (engine, in);

        for (auto& line : view.advice)
            body << "\n\n" << line;
    }

    lastInputs = in;
    haveInputs = true;
    dirty = false;
    picture->setState (topic, view, in);

    if (force || body != explanation.getText())
    {
        explanation.setText (body, false);

        if (force || topic != hg::Topic::yourSong)
            explanation.moveCaretToTop (false);
    }
}

void HandoverExplainer::show (int index)
{
    sources.close();
    shownRow = juce::jlimit (0, (int) std::size (hg::allTopics) - 1, index);
    bounceButton.setVisible (hg::allTopics[shownRow] == hg::Topic::yourSong);
    resized();
    refresh (true);
}

void HandoverExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void HandoverExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);
    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);
    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    if (sources.isShown())
    {
        sources.show (true, r);
        return;
    }

    picture->setBounds (r.removeFromTop (pictureHeight));
    r.removeFromTop (10);

    if (bounceButton.isVisible())
    {
        bounceButton.setBounds (r.removeFromTop (28).removeFromLeft (150));
        r.removeFromTop (6);
    }

    explanation.setBounds (r);
}
} // namespace daw
