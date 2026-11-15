#include "GainStagingExplainer.h"
#include "engine/Routing.h"
#include "generate/MixExport.h"

namespace daw
{
using namespace gainstaging;

/** The selected track's chain as a line of levels from left to right: for every step what goes in and
    what comes out, over the quiet, healthy, hot and over zones, with the notes below it. */
class GainStagingExplainer::ChainPicture final : public juce::Component
{
public:
    std::function<void (Topic)> onHintClicked;

    void setSnapshot (Snapshot s)
    {
        snapshot = std::move (s);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        hintAreas.clear();

        auto r = getLocalBounds().reduced (14, 10);
        auto header = r.removeFromTop (22);
        auto hintsArea = r.removeFromBottom (76);
        auto namesArea = r.removeFromBottom (38);
        r.removeFromTop (6);
        auto plot = r.withTrimmedLeft (36).toFloat();

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (snapshot.trackName.isEmpty() ? juce::String ("Select a track to see its levels")
                                                 : "The levels of " + snapshot.trackName + ", right now",
                    header, juce::Justification::centredLeft, true);

        // The legend
        auto legend = header.removeFromRight (330);
        g.setFont (juce::FontOptions (12.0f));

        for (auto zone : { Zone::quiet, Zone::healthy, Zone::hot, Zone::over })
        {
            auto item = legend.removeFromLeft (zone == Zone::over ? 90 : 80);
            g.setColour (getZoneColour (zone));
            g.fillRoundedRectangle (item.removeFromLeft (12).withSizeKeepingCentre (10, 10).toFloat(), 2.0f);
            item.removeFromLeft (5);
            g.setColour (theme::textDim);
            g.drawText (getZoneName (zone), item, juce::Justification::centredLeft, false);
        }

        // The zones, from -60 dB at the bottom to +12 dB at the top.
        auto toY = [plot] (float db) { return plot.getBottom() - (juce::jlimit (bottomDb, topDb, db) - bottomDb) / (topDb - bottomDb) * plot.getHeight(); };

        const std::pair<float, float> ranges[] { { silentBelowDb, quietBelowDb }, { quietBelowDb, hotAboveDb }, { hotAboveDb, overAboveDb }, { overAboveDb, topDb } };
        const Zone zones[] { Zone::quiet, Zone::healthy, Zone::hot, Zone::over };

        for (int i = 0; i < 4; ++i)
        {
            g.setColour (getZoneColour (zones[i]).withAlpha (zones[i] == Zone::quiet ? 0.07f : 0.12f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (plot.getX(), toY (ranges[i].second), plot.getRight(), toY (ranges[i].first)));
        }

        g.setFont (juce::FontOptions (11.0f));

        for (float db : { 12.0f, 0.0f, -6.0f, -24.0f, -60.0f })
        {
            const float y = toY (db);
            g.setColour (theme::gridBar.withAlpha (juce::exactlyEqual (db, 0.0f) ? 1.0f : 0.5f));
            g.fillRect (plot.getX(), y, plot.getWidth(), juce::exactlyEqual (db, 0.0f) ? 1.5f : 1.0f);
            g.setColour (theme::textDim);
            g.drawText ((db > 0.0f ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (plot.getX() - 36.0f, y - 7.0f, 30.0f, 14.0f),
                        juce::Justification::centredRight, false);
        }

        // The sends are not steps of the line: they hang below it, each under the step it is taken after.
        std::vector<Stop> stops, sends;

        for (auto& stop : snapshot.stops)
            (stop.kind == StopKind::send ? sends : stops).push_back (stop);

        const bool playing = std::any_of (stops.begin(), stops.end(), [] (const Stop& s) { return juce::jmax (s.inDb, s.outDb) >= silentBelowDb; });

        if (stops.empty())
            return;

        const float columnWidth = plot.getWidth() / (float) stops.size();
        juce::Point<float> previous;
        bool hasPrevious = false;

        for (size_t i = 0; i < stops.size(); ++i)
        {
            const auto& stop = stops[i];
            const float left = plot.getX() + columnWidth * (float) i;
            const juce::Point<float> in { left + columnWidth * 0.28f, toY (stop.inDb) }, out { left + columnWidth * 0.72f, toY (stop.outDb) };

            if (i > 0)
            {
                g.setColour (theme::gridBar.withAlpha (0.4f));
                g.fillRect (left, plot.getY(), 1.0f, plot.getHeight());
            }

            // The name, and what the step does to the level.
            auto names = namesArea.toFloat().withX (left).withWidth (columnWidth).reduced (3.0f, 0.0f);
            g.setColour (stop.bypassed ? theme::textDim : theme::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawText (stop.name, names.removeFromTop (18.0f), juce::Justification::centred, true);
            g.setFont (juce::FontOptions (11.5f));
            g.setColour (theme::textDim);
            juce::String detail;

            if (stop.kind == StopKind::instrument)
                detail = "the sound starts here";
            else if (stop.kind == StopKind::busInput && stop.isReturn)
                detail = juce::String (stop.numTracks) + (stop.numTracks == 1 ? " send adds" : " sends add") + " up here";
            else if (stop.kind == StopKind::busInput)
                detail = juce::String (stop.numTracks) + (stop.numTracks == 1 ? " track adds" : " tracks add") + " up here";
            else if (stop.kind == StopKind::bus)
                detail = juce::String (juce::jmax (0, stop.numTracks - 1)) + " others, volume " + formatDb (stop.settingDb);
            else if (stop.kind == StopKind::effect)
                detail = stop.bypassed ? "off" : stop.matching ? "match level on" : stop.changeDb.has_value() ? "changes it " + formatDb (*stop.changeDb) : juce::String();
            else
                detail = (stop.kind == StopKind::master ? "all tracks, volume " : "volume ") + formatDb (stop.settingDb);

            g.drawText (detail, names, juce::Justification::centredTop, true);

            if (! playing)
                continue;

            auto lineFor = [&g] (juce::Point<float> a, juce::Point<float> b, float db, bool dashed)
            {
                juce::Path path;
                path.startNewSubPath (a);
                path.lineTo (b);
                g.setColour (getZoneColour (getZone (db)));

                if (dashed)
                {
                    const float dashes[] { 4.0f, 4.0f };
                    juce::Path dashedPath;
                    juce::PathStrokeType (2.0f).createDashedStroke (dashedPath, path, dashes, 2);
                    g.fillPath (dashedPath);
                }
                else
                {
                    g.strokePath (path, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
            };

            auto dot = [&g] (juce::Point<float> p, float db)
            {
                g.setColour (getZoneColour (getZone (db)));
                g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (p));
            };

            // Into a bus or the master come all the other tracks too: a dashed jump.
            if (stop.kind != StopKind::instrument && stop.kind != StopKind::busInput)
            {
                if (hasPrevious)
                    lineFor (previous, in, stops[i - 1].outDb, stop.kind == StopKind::master || stop.kind == StopKind::bus);

                lineFor (in, out, stop.outDb, false);
                dot (in, stop.inDb);
            }

            dot (out, stop.outDb);
            previous = out;
            hasPrevious = true;

            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (stop.outDb < silentBelowDb ? juce::String ("-inf") : (stop.outDb > 0.05f ? "+" : "") + juce::String (stop.outDb, 1),
                        juce::Rectangle<float> (64.0f, 14.0f).withCentre (out.translated (0.0f, out.y > plot.getY() + 20.0f ? -13.0f : 13.0f)),
                        juce::Justification::centred, false);
        }

        // The sends: a small box low in the picture under the step they are taken after, joined to the line by a branch.
        for (auto& send : sends)
        {
            const int column = juce::jlimit (0, (int) stops.size() - 1, juce::jmax (0, send.tapAfter));
            const float x = plot.getX() + columnWidth * ((float) column + 0.72f);
            const juce::Rectangle<float> box (x + 6.0f, plot.getBottom() - 24.0f, juce::jmin (columnWidth - 12.0f, 150.0f), 18.0f);
            g.setColour (theme::textDim.withAlpha (0.6f));
            g.drawLine (x, toY (stops[(size_t) column].outDb), x, box.getCentreY(), 1.2f);
            g.drawLine (x, box.getCentreY(), box.getX(), box.getCentreY(), 1.2f);
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (box, 4.0f);
            g.setColour (theme::gridBar);
            g.drawRoundedRectangle (box, 4.0f, 1.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.drawText (juce::String::fromUTF8 ("\xe2\x86\x92 ") + send.name + "  " + formatDb (send.settingDb) + (send.preFader ? "  PRE" : ""),
                        box.reduced (5.0f, 0.0f), juce::Justification::centredLeft, true);
        }

        // The markers, and what they say.
        for (auto& hint : snapshot.hints)
        {
            const float centre = plot.getX() + columnWidth * ((float) hint.stop + 0.5f);
            paintMarker (g, juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ centre, plot.getY() + 10.0f }), hint.serious);
        }

        auto notes = hintsArea.withTrimmedTop (6);
        g.setFont (juce::FontOptions (12.5f));

        if (! playing)
        {
            g.setColour (theme::textDim);
            g.drawText ("Play the song to see the levels.", notes, juce::Justification::topLeft, true);
            return;
        }

        if (snapshot.hints.empty())
        {
            g.setColour (theme::textDim);
            g.drawText ("Nothing stands out: every step has a sensible level.", notes, juce::Justification::topLeft, true);
            return;
        }

        for (size_t i = 0; i < snapshot.hints.size() && i < 2; ++i)
        {
            const auto& hint = snapshot.hints[i];
            auto line = notes.removeFromTop (35);
            paintMarker (g, line.removeFromLeft (22).removeFromTop (16).toFloat().withSizeKeepingCentre (14.0f, 14.0f), hint.serious);
            g.setColour (theme::text);
            g.drawFittedText (hint.text + " (click to read more)", line, juce::Justification::topLeft, 2, 1.0f);
            hintAreas.push_back ({ line, hint.topic });
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const bool onHint = std::any_of (hintAreas.begin(), hintAreas.end(), [&e] (auto& h) { return h.first.contains (e.getPosition()); });
        setMouseCursor (onHint ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        for (auto& [area, topic] : hintAreas)
            if (area.contains (e.getPosition()) && onHintClicked != nullptr)
                onHintClicked (topic);
    }

    /** A round ! in yellow, or red for a serious one. As on the device bar. */
    static void paintMarker (juce::Graphics& g, juce::Rectangle<float> area, bool serious)
    {
        g.setColour (serious ? theme::meterRed : theme::meterYellow);
        g.fillEllipse (area);
        g.setColour (theme::background);
        g.setFont (juce::FontOptions (area.getHeight() * 0.8f, juce::Font::bold));
        g.drawText ("!", area, juce::Justification::centred, false);
    }

private:
    static constexpr float topDb = 12.0f, bottomDb = silentBelowDb;

    Snapshot snapshot;
    std::vector<std::pair<juce::Rectangle<int>, Topic>> hintAreas;
};

namespace gainstaging
{
SongSnapshot getSongSnapshot (const Engine* engine)
{
    SongSnapshot snapshot;

    if (engine != nullptr)
    {
        for (auto* t : routing::getVisibleTracks (engine->getTracks()))
        {
            const bool sound = t->isAudio() || t->isMidi();
            const float peak = juce::jmax (t->peakLeft.load(), t->peakRight.load());
            snapshot.rows.push_back ({ t->name, t->colour, t->volumeDb.load(), juce::Decibels::gainToDecibels (peak, -100.0f), t->pan.load(),
                                       false, sound, sound && gen::mixexport::isCentrePart (*t) });
        }

        if (! snapshot.rows.empty())
        {
            const float peak = juce::jmax (engine->masterPeakLeft.load(), engine->masterPeakRight.load());
            snapshot.rows.push_back ({ "Master", theme::textDim, engine->masterVolumeDb.load(), juce::Decibels::gainToDecibels (peak, -100.0f), 0.0f, true, false, false });
            return snapshot;
        }
    }

    // The example song's faders and pans: an example, not a rule.
    snapshot.example = true;
    const auto add = [&snapshot] (const char* name, juce::uint32 colour, float db, float pan, bool sound, bool centre)
    {
        snapshot.rows.push_back ({ name, juce::Colour (colour), db, db - 7.0f, pan, false, sound, centre });
    };
    add ("Kick", 0xffe0735a, -2.0f, 0.0f, true, true);
    add ("Snare", 0xffe0a24a, -4.0f, 0.0f, true, true);
    add ("Hats", 0xffd9c94a, -9.0f, 0.25f, true, false);
    add ("Bass", 0xff5aa0e0, -4.0f, 0.0f, true, true);
    add ("Keys", 0xff8a7ae0, -7.0f, -0.20f, true, false);
    add ("Lead vocal", 0xffe06ab0, 0.0f, 0.0f, true, true);
    add ("BV 1", 0xff5ac9a0, -9.0f, -0.40f, true, false);
    add ("BV 2", 0xff5ac9a0, -9.0f, 0.40f, true, false);
    add ("Drums", 0xffe0735a, 0.0f, 0.0f, false, false);
    add ("Backing vocals", 0xff5ac9a0, 0.0f, 0.0f, false, false);
    add ("Vocals", 0xffe06ab0, 0.0f, 0.0f, false, false);
    snapshot.rows.push_back ({ "Master", theme::textDim, 0.0f, -8.0f, 0.0f, true, false, false });
    return snapshot;
}
} // namespace gainstaging

//==============================================================================
/** The song as a picture for the two topics after *Step by step*. In `balance` mode a row per track: its live level as a bar and its
    fader as a marker with the dB, on a -30 to +6 dB scale (the master has a -6 dB line). In `pan` mode a row per audio or MIDI track
    with a dot at its pan, over a centre band and two side zones. */
class GainStagingExplainer::SongPicture final : public juce::Component
{
public:
    enum class Mode { balance, pan };

    void setMode (Mode m)
    {
        mode = m;
        repaint();
    }

    void setSnapshot (SongSnapshot s)
    {
        snapshot = std::move (s);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        auto r = getLocalBounds().reduced (14, 10);
        auto header = r.removeFromTop (22);
        auto caption = r.removeFromBottom (34);

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        const bool balance = mode == Mode::balance;
        g.drawText (balance ? (snapshot.example ? "Example faders and levels" : "Your faders and levels, right now")
                            : (snapshot.example ? "Example pans" : "Where your parts sit, left to right"),
                    header, juce::Justification::centredLeft, true);

        std::vector<const SongRow*> rows;

        for (auto& row : snapshot.rows)
            if (balance || row.isSound)
                rows.push_back (&row);

        auto names = r.removeFromLeft (110);
        r.removeFromLeft (8);
        auto readout = r.removeFromRight (balance ? 62 : 0);
        const auto plot = r.toFloat();
        const int axisHeight = 16;
        const float rowHeight = rows.empty() ? 20.0f : juce::jmin (26.0f, (plot.getHeight() - axisHeight) / (float) rows.size());
        const auto body = plot.withTrimmedTop ((float) axisHeight);

        if (balance)
            paintBalance (g, rows, names, plot, body, readout, rowHeight);
        else
            paintPan (g, rows, names, plot, body, rowHeight);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.5f));
        g.drawFittedText (getCaption (rows), caption, juce::Justification::topLeft, 2, 1.0f);
    }

private:
    static constexpr float lowDb = -30.0f, highDb = 6.0f;

    float dbToX (juce::Rectangle<float> plot, float db) const
    {
        return plot.getX() + (juce::jlimit (lowDb, highDb, db) - lowDb) / (highDb - lowDb) * plot.getWidth();
    }

    void paintBalance (juce::Graphics& g, const std::vector<const SongRow*>& rows, juce::Rectangle<int> names, juce::Rectangle<float> plot,
                       juce::Rectangle<float> body, juce::Rectangle<int> readout, float rowHeight) const
    {
        g.setFont (juce::FontOptions (11.0f));

        for (float db = lowDb; db <= highDb + 0.1f; db += 6.0f)
        {
            const float x = dbToX (plot, db);
            g.setColour (theme::textDim.withAlpha (juce::exactlyEqual (db, 0.0f) ? 0.5f : 0.2f));
            g.drawVerticalLine ((int) x, body.getY(), body.getBottom());
            g.setColour (theme::textDim);
            g.drawText (juce::String ((int) db), juce::Rectangle<float> (x - 18.0f, plot.getY(), 36.0f, 14.0f), juce::Justification::centred, false);
        }

        float y = body.getY();

        for (auto* row : rows)
        {
            const auto line = juce::Rectangle<float> (plot.getX(), y, plot.getWidth(), rowHeight);
            const auto bar = line.reduced (0.0f, rowHeight * 0.3f);
            const bool low = row->faderDb < (float) gen::mixexport::farDownDb;

            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, row->isMaster ? juce::Font::bold : juce::Font::plain));
            g.drawText (row->name, juce::Rectangle<int> (names.getX(), (int) y, names.getWidth(), (int) rowHeight), juce::Justification::centredRight, true);

            // The live level: the master's peak is amber above -6 dB and red above 0.
            auto colour = row->colour;

            if (row->isMaster)
                colour = row->levelDb > 0.0f ? theme::meterRed : row->levelDb > -6.0f ? theme::meterYellow : theme::meterGreen;

            const float levelX = dbToX (plot, row->levelDb);
            g.setColour (colour.withAlpha (0.85f));
            g.fillRect (juce::Rectangle<float> (plot.getX(), bar.getY(), juce::jmax (0.0f, levelX - plot.getX()), bar.getHeight()));

            if (row->isMaster)
            {
                g.setColour (theme::meterYellow);
                g.drawVerticalLine ((int) dbToX (plot, -6.0f), line.getY(), line.getBottom());
            }

            // The fader: a marker, amber when it is far down.
            const float faderX = dbToX (plot, row->faderDb);
            g.setColour (low ? theme::meterYellow : theme::text);
            g.fillRect (juce::Rectangle<float> (faderX - 2.0f, line.getY() + 1.0f, 4.0f, line.getHeight() - 2.0f));
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (juce::String (row->faderDb, 1) + " dB", juce::Rectangle<int> (readout.getX() + 4, (int) y, readout.getWidth() - 4, (int) rowHeight),
                        juce::Justification::centredLeft, false);
            y += rowHeight;
        }
    }

    void paintPan (juce::Graphics& g, const std::vector<const SongRow*>& rows, juce::Rectangle<int> names, juce::Rectangle<float> plot,
                   juce::Rectangle<float> body, float rowHeight) const
    {
        const float centreX = plot.getCentreX(), half = plot.getWidth() / 2.0f;
        const float bandHalf = (float) gen::mixexport::centreTipPan * half;

        g.setColour (theme::accent.withAlpha (0.12f));
        g.fillRect (juce::Rectangle<float> (centreX - bandHalf, body.getY(), bandHalf * 2.0f, body.getHeight()));
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("L", juce::Rectangle<float> (plot.getX(), plot.getY(), 20.0f, 14.0f), juce::Justification::centredLeft, false);
        g.drawText ("R", juce::Rectangle<float> (plot.getRight() - 20.0f, plot.getY(), 20.0f, 14.0f), juce::Justification::centredRight, false);
        g.drawText ("kick, snare, bass, lead vocal", juce::Rectangle<float> (centreX - 90.0f, plot.getY(), 180.0f, 14.0f), juce::Justification::centred, false);
        g.drawText ("guitars, keys, pads, backing vocals", juce::Rectangle<float> (plot.getX() + 20.0f, plot.getY(), centreX - bandHalf - plot.getX() - 24.0f, 14.0f),
                    juce::Justification::centred, true);
        g.drawText ("guitars, keys, pads, backing vocals", juce::Rectangle<float> (centreX + bandHalf + 4.0f, plot.getY(), plot.getRight() - centreX - bandHalf - 24.0f, 14.0f),
                    juce::Justification::centred, true);

        float y = body.getY();

        for (auto* row : rows)
        {
            const float cy = y + rowHeight / 2.0f;
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f));
            g.drawText (row->name, juce::Rectangle<int> (names.getX(), (int) y, names.getWidth(), (int) rowHeight), juce::Justification::centredRight, true);
            g.setColour (theme::textDim.withAlpha (0.35f));
            g.drawHorizontalLine ((int) cy, plot.getX(), plot.getRight());
            g.setColour (theme::textDim.withAlpha (0.6f));
            g.drawVerticalLine ((int) centreX, cy - 4.0f, cy + 4.0f);

            const float x = centreX + juce::jlimit (-1.0f, 1.0f, row->pan) * half;
            const float d = juce::jmin (14.0f, rowHeight - 4.0f);
            g.setColour (row->colour);
            g.fillEllipse (x - d / 2.0f, cy - d / 2.0f, d, d);

            if (row->isCentre && std::abs (row->pan) >= (float) gen::mixexport::centreTipPan - 1.0e-4f)
            {
                g.setColour (theme::meterYellow);
                g.drawEllipse (x - d / 2.0f - 2.0f, cy - d / 2.0f - 2.0f, d + 4.0f, d + 4.0f, 1.8f);
                const bool right = row->pan > 0.0f;
                g.setFont (juce::FontOptions (11.5f));
                g.drawText ("usually in the middle", juce::Rectangle<float> (right ? x - 150.0f : x + d, y, 140.0f, rowHeight),
                            right ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
            }

            y += rowHeight;
        }
    }

    juce::String getCaption (const std::vector<const SongRow*>& rows) const
    {
        if (snapshot.example)
            return "An example, not a rule.";

        if (mode == Mode::balance)
        {
            for (auto* row : rows)
                if (row->faderDb < (float) gen::mixexport::farDownDb && ! row->isMaster)
                    return row->name + "'s fader is below -20 dB. Turn down its source instead (" + (row->isSound ? "its instrument or clip's gain" : "the tracks in it")
                           + "), then bring the fader back up.";

            return "The bar is the level coming through each track, the mark is its fader. The master's line marks -6 dB: keep its peaks below it.";
        }

        int centred = 0;

        for (auto* row : rows)
            centred += std::abs (row->pan) < 0.05f ? 1 : 0;

        if (rows.size() >= 3 && centred == (int) rows.size())
            return "Everything is in the middle. Try panning one part, such as the keys or a guitar, a quarter of the way to one side.";

        return "Each dot is a track's pan. The shaded band is the middle, where the kick, snare, bass and lead vocal usually sit.";
    }

    Mode mode = Mode::balance;
    SongSnapshot snapshot;
};

//==============================================================================
juce::Component& GainStagingExplainer::getPicture()
{
    if (shown == Topic::balance || shown == Topic::pan)
        return *songPicture;

    return *picture;
}

GainStagingExplainer::GainStagingExplainer (std::function<Snapshot()> snapshotSource, const Engine* engine)
    : getSnapshot (std::move (snapshotSource)), songEngine (engine)
{
    title.setText ("Gain staging: the level at every step", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<ChainPicture>();
    picture->onHintClicked = [this] (Topic topic) { showTopic (topic); };
    addAndMakeVisible (*picture);
    songPicture = std::make_unique<SongPicture>();
    addChildComponent (*songPicture);

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
    footer.onReadChanged = [this] { list.repaint(); };
    addChildComponent (footer);
    addAndMakeVisible (exportButton);

    list.updateContent();
    showTopic (Topic::what);
    timerCallback();
    startTimerHz (15);
    setSize (width, height);
}

GainStagingExplainer::~GainStagingExplainer()
{
    list.setModel (nullptr);
}

juce::String GainStagingExplainer::getPrintSource() const
{
    const auto name = getSnapshot != nullptr ? getSnapshot().trackName : juce::String();
    return name.isNotEmpty() ? "the chain of the track " + name : juce::String ("no track selected");
}

int GainStagingExplainer::getNumPrintTopics() const
{
    return (int) std::size (allTopics) + 1;
}

booklet::Topic GainStagingExplainer::showPrintTopic (int index)
{
    if (GuideSourcesTopic::isRow (index, (int) std::size (allTopics)))
        return sources.getPrintTopic();

    if (! topicBeforePrinting)
        topicBeforePrinting = shown;

    showTopic (allTopics[juce::jlimit (0, getNumPrintTopics() - 1, index)]);

    if (getSnapshot != nullptr)
        picture->setSnapshot (getSnapshot());

    songPicture->setSnapshot (getSongSnapshot (songEngine));
    auto& shownPicture = getPicture();
    return { getTopicName (shown), getTopicSummary (shown), explanation.getText(), { shownPicture.getWidth(), shownPicture.getHeight() }, {} };
}

juce::Image GainStagingExplainer::paintPrintPicture (float scale)
{
    return guideprint::snapshot (getPicture(), scale);
}

void GainStagingExplainer::endPrinting()
{
    if (topicBeforePrinting)
        showTopic (*topicBeforePrinting);

    topicBeforePrinting.reset();
}

void GainStagingExplainer::showTopic (Topic topic)
{
    sources.close();
    shown = topic;
    const auto index = (int) (std::find (std::begin (allTopics), std::end (allTopics), topic) - std::begin (allTopics));

    if (list.getSelectedRow() != index)
        list.selectRow (index);   // comes back through selectedRowsChanged

    const bool song = topic == Topic::balance || topic == Topic::pan;
    songPicture->setMode (topic == Topic::pan ? SongPicture::Mode::pan : SongPicture::Mode::balance);
    songPicture->setSnapshot (getSongSnapshot (songEngine));
    songPicture->setVisible (song);
    picture->setVisible (! song);
    explanation.setText (explain (topic), false);
    explanation.moveCaretToTop (false);
    updateFooter();
}

int GainStagingExplainer::getNumRows()
{
    return (int) std::size (allTopics) + 1;
}

void GainStagingExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (allTopics)))
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

    const auto topic = allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (getTopicName (topic), 10, 5, w - 36, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (getTopicSummary (topic), 10, 23, w - 36, 16, juce::Justification::centredLeft, true);

    GuideFooter::paintTick (g, w, h, gen::guides::Guide::gainStaging, getTopicName (topic));

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void GainStagingExplainer::selectedRowsChanged (int row)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (allTopics)))
    {
        showSources();
        return;
    }

    if (row >= 0 && row < getNumRows())
        showTopic (allTopics[row]);
}

void GainStagingExplainer::showSources()
{
    sources.select (*this, list, (int) std::size (allTopics), { &list, &title, &exportButton });
    updateFooter();
}

void GainStagingExplainer::timerCallback()
{
    if (getSnapshot != nullptr)
        picture->setSnapshot (getSnapshot());

    if (songPicture->isVisible())
        songPicture->setSnapshot (getSongSnapshot (songEngine));
}

void GainStagingExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void GainStagingExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    footer.takeSpace (r);

    if (sources.isShown())
    {
        sources.show (true, r);
        return;
    }

    const auto pictureArea = r.removeFromTop (320);
    picture->setBounds (pictureArea);
    songPicture->setBounds (pictureArea);
    r.removeFromTop (12);

    explanation.setBounds (r);
}

void GainStagingExplainer::updateFooter()
{
    if (footer.update (gen::guides::Guide::gainStaging, sources.isShown() ? gen::guides::sourcesTopicName : getTopicName (shown)))
        resized();
}

bool GainStagingExplainer::keyPressed (const juce::KeyPress& key)
{
    return footer.keyPressed (key);
}
} // namespace daw
