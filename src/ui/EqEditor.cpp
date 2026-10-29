#include "EqEditor.h"

namespace daw
{
using eqtext::Topic;

namespace
{
    constexpr float graphRangeDb = 18.0f, spectrumRangeDb = 90.0f, handleRadius = 9.0f;
    constexpr int labelHeight = 16, regionHeight = 18;

    // The instrument overview: the names left of the plot, a legend above the rows.
    constexpr int nameWidth = 112, legendHeight = 20;

    /** Marks a control with what the explainer should tell about it when the mouse is on it. */
    void setTopic (juce::Component& c, Topic topic, int band = -1)
    {
        c.getProperties().set ("eqTopic", (int) topic);

        if (band >= 0)
            c.getProperties().set ("eqBand", band);
    }

    juce::String parameterId (int band, const char* what) { return "b" + juce::String (band + 1) + what; }
}

//==============================================================================
EqGraph::EqGraph (BuiltInEq& p) : processor (p)
{
    ringBefore.assign ((size_t) fftSize, 0.0f);
    ringAfter.assign ((size_t) fftSize, 0.0f);
    readBefore.assign ((size_t) fftSize, 0.0f);
    readAfter.assign ((size_t) fftSize, 0.0f);
    fftData.assign ((size_t) fftSize * 2, 0.0f);
    levelBefore.assign ((size_t) fftSize / 2, -120.0f);
    levelAfter.assign ((size_t) fftSize / 2, -120.0f);
}

double EqGraph::getSampleRate() const
{
    return processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
}

juce::Rectangle<float> EqGraph::getPlotArea() const
{
    return getLocalBounds().toFloat().withTrimmedBottom ((float) (labelHeight + regionHeight + 2))
                                     .withTrimmedLeft (showingInstruments ? (float) nameWidth : 0.0f);
}

void EqGraph::setShowingInstruments (bool show)
{
    showingInstruments = show;
    hoveredInstrument = hoveredZone = selectedInstrument = -1;
    repaint();
}

juce::Rectangle<float> EqGraph::getInstrumentRow (int instrument) const
{
    const auto area = getPlotArea().withTrimmedTop ((float) legendHeight);
    const float height = area.getHeight() / (float) eq::getInstruments().size();

    // From the left edge, so the name belongs to the row.
    return { 0.0f, area.getY() + height * (float) instrument, area.getRight(), height };
}

std::pair<int, int> EqGraph::findInstrument (juce::Point<float> position) const
{
    const auto& list = eq::getInstruments();

    for (int i = 0; i < (int) list.size(); ++i)
    {
        if (! getInstrumentRow (i).contains (position))
            continue;

        const auto& zones = list[(size_t) i].zones;

        for (int z = 0; z < (int) zones.size(); ++z)
            if (position.x >= frequencyToX (zones[(size_t) z].from) && position.x < frequencyToX (zones[(size_t) z].to))
                return { i, z };

        return { i, -1 };
    }

    return { -1, -1 };
}

float EqGraph::frequencyToX (double hz) const
{
    const auto area = getPlotArea();
    return area.getX() + area.getWidth() * (float) (std::log (hz / eq::minFreq) / std::log (eq::maxFreq / eq::minFreq));
}

double EqGraph::xToFrequency (float x) const
{
    const auto area = getPlotArea();
    return eq::minFreq * std::pow (eq::maxFreq / eq::minFreq, (double) ((x - area.getX()) / area.getWidth()));
}

float EqGraph::gainToY (double db) const
{
    const auto area = getPlotArea().reduced (0.0f, 12.0f);
    return area.getCentreY() - (float) (db / graphRangeDb) * area.getHeight() * 0.5f;
}

double EqGraph::yToGain (float y) const
{
    const auto area = getPlotArea().reduced (0.0f, 12.0f);
    return (area.getCentreY() - y) / (area.getHeight() * 0.5f) * graphRangeDb;
}

juce::Point<float> EqGraph::getHandlePosition (int band) const
{
    const auto b = processor.getBand (band);

    // Cuts and notches have no gain: their dot sits on the 0 dB line.
    return { frequencyToX (b.freq), gainToY (eq::hasGain (b.type) ? b.gainDb : 0.0f) };
}

int EqGraph::findHandle (juce::Point<float> position) const
{
    int best = -1;
    float bestDistance = handleRadius + 5.0f;

    for (int i = 0; i < eq::numBands; ++i)
    {
        const float d = getHandlePosition (i).getDistanceFrom (position);

        if (d <= bestDistance)
        {
            best = i;
            bestDistance = d;
        }
    }

    return best;
}

void EqGraph::setParameter (int band, const char* what, float value)
{
    if (auto* p = processor.getBandParameter (band, what))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

//==============================================================================
void EqGraph::updateSpectrum()
{
    const int n = processor.readAnalyzer (readBefore.data(), readAfter.data(), fftSize);

    if (n == 0)
    {
        // Nothing comes through (bypassed, or not playing): let the spectrum sink away.
        for (auto* levels : { &levelBefore, &levelAfter })
            for (auto& level : *levels)
                level = juce::jmax (-120.0f, level - 1.5f);

        return;
    }

    for (int i = 0; i < n; ++i)
    {
        ringBefore[(size_t) ringPosition] = readBefore[(size_t) i];
        ringAfter[(size_t) ringPosition] = readAfter[(size_t) i];
        ringPosition = (ringPosition + 1) % fftSize;
    }

    auto analyse = [this] (const std::vector<float>& ring, std::vector<float>& levels)
    {
        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = ring[(size_t) ((ringPosition + i) % fftSize)];

        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        // A full-scale sine comes out of a Hann-windowed FFT at a quarter of the size.
        for (int b = 0; b < fftSize / 2; ++b)
        {
            const float db = juce::Decibels::gainToDecibels (fftData[(size_t) b] * 4.0f / (float) fftSize, -120.0f);
            auto& level = levels[(size_t) b];
            level = db > level ? db : juce::jmax (db, level - 1.5f);   // jump up, fall back about 45 dB a second
        }
    };

    analyse (ringBefore, levelBefore);
    analyse (ringAfter, levelAfter);
}

//==============================================================================
void EqGraph::paint (juce::Graphics& g)
{
    const auto area = getPlotArea();
    paintGrid (g, area);

    if (showingInstruments)
        paintInstruments (g, area);
    else
        paintSpectrum (g, area);

    paintCurves (g, area);
}

void EqGraph::paintGrid (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (theme::lane);
    g.fillRoundedRectangle (area, 4.0f);

    // A line at every 10, 20, 30 ... Hz; brighter at 100 Hz, 1 kHz and 10 kHz.
    for (double decade = 10.0; decade < eq::maxFreq; decade *= 10.0)
    {
        for (int m = 1; m <= 9; ++m)
        {
            const double hz = decade * m;

            if (hz < eq::minFreq || hz > eq::maxFreq)
                continue;

            g.setColour (m == 1 ? theme::gridBar : theme::grid);
            g.fillRect (frequencyToX (hz), area.getY(), 1.0f, area.getHeight());
        }
    }

    g.setFont (juce::FontOptions (11.0f));

    for (double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        g.setColour (theme::textDim);
        g.drawText (hz < 1000.0 ? juce::String ((int) hz) : juce::String ((int) (hz / 1000.0)) + "k",
                    juce::Rectangle<float> (frequencyToX (hz) - 20.0f, area.getBottom() + 1.0f, 40.0f, (float) labelHeight), juce::Justification::centred);
    }

    for (int db = -18; db <= 18; db += 6)
    {
        const float y = gainToY (db);
        g.setColour (db == 0 ? theme::gridBar.brighter (0.3f) : theme::grid);
        g.fillRect (area.getX(), y, area.getWidth(), db == 0 ? 1.5f : 1.0f);

        if (db != -18 && db != 18 && ! showingInstruments)
        {
            g.setColour (theme::textDim.withAlpha (0.8f));
            g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (area.getX() + 4.0f, y - 13.0f, 30.0f, 12.0f),
                        juce::Justification::centredLeft);
        }
    }

    // The ranges people talk about, under the frequencies.
    struct Region { double from, to; const char* name; };
    static const Region regions[] = { { 20, 60, "sub" }, { 60, 250, "bass" }, { 250, 500, "low mids" }, { 500, 2000, "mids" },
                                      { 2000, 6000, "presence" }, { 6000, 10000, "brilliance" }, { 10000, 20000, "air" } };
    const float regionY = area.getBottom() + (float) labelHeight + 2.0f;
    g.setFont (juce::FontOptions (10.5f));

    for (size_t i = 0; i < std::size (regions); ++i)
    {
        const auto& r = regions[i];
        const auto box = juce::Rectangle<float>::leftTopRightBottom (frequencyToX (r.from), regionY, frequencyToX (r.to), regionY + (float) regionHeight);
        g.setColour (i % 2 == 0 ? theme::panelLight : theme::panel.brighter (0.05f));
        g.fillRect (box.reduced (0.5f, 0.0f));
        g.setColour (theme::textDim);
        g.drawText (r.name, box, juce::Justification::centred, true);
    }
}

void EqGraph::paintSpectrum (juce::Graphics& g, juce::Rectangle<float> area)
{
    const double rate = getSampleRate();
    const float maxBefore = *std::max_element (levelBefore.begin(), levelBefore.end());
    const float maxAfter = *std::max_element (levelAfter.begin(), levelAfter.end());

    if (juce::jmax (maxBefore, maxAfter) < -100.0f)
        return;

    // The level at a column: between bins where they're wider than a pixel, the loudest bin where narrower.
    auto levelAt = [&] (const std::vector<float>& levels, double f0, double f1)
    {
        const double b0 = f0 * fftSize / rate, b1 = f1 * fftSize / rate;
        const int last = fftSize / 2 - 2;

        if (b1 - b0 < 1.0)
        {
            const double b = juce::jlimit (0.0, (double) last, (b0 + b1) * 0.5);
            const int i = (int) b;
            return juce::jmap ((float) (b - i), levels[(size_t) i], levels[(size_t) i + 1]);
        }

        float level = -120.0f;

        for (int i = juce::jlimit (0, last, (int) b0); i <= juce::jlimit (0, last, (int) b1); ++i)
            level = juce::jmax (level, levels[(size_t) i]);

        return level;
    };

    auto toY = [&] (float db) { return area.getY() + juce::jlimit (0.0f, 1.0f, -db / spectrumRangeDb) * area.getHeight(); };

    juce::Path before, after;
    after.startNewSubPath (area.getX(), area.getBottom());
    before.startNewSubPath (area.getX(), toY (levelAt (levelBefore, xToFrequency (area.getX()), xToFrequency (area.getX() + 1.0f))));

    for (float x = area.getX(); x <= area.getRight(); x += 2.0f)
    {
        const double f0 = xToFrequency (x - 1.0f), f1 = xToFrequency (x + 1.0f);
        before.lineTo (x, toY (levelAt (levelBefore, f0, f1)));
        after.lineTo (x, toY (levelAt (levelAfter, f0, f1)));
    }

    after.lineTo (area.getRight(), area.getBottom());
    after.closeSubPath();

    g.setColour (theme::accent.withAlpha (0.13f));
    g.fillPath (after);
    g.setColour (theme::accent.withAlpha (0.4f));
    g.strokePath (after, juce::PathStrokeType (1.0f));
    g.setColour (theme::textDim.withAlpha (0.45f));
    g.strokePath (before, juce::PathStrokeType (1.0f));
}

void EqGraph::paintInstruments (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto& list = eq::getInstruments();
    const auto zoneColour = [] (const eq::InstrumentZone& z) { return z.problem ? theme::meterRed : theme::meterGreen; };

    // The instrument being looked at stands out; the others turn grey, so its ranges are easy to follow.
    const int focus = selectedInstrument >= 0 ? selectedInstrument : hoveredInstrument;
    const auto grey = theme::textDim;

    // The legend, above the rows.
    {
        auto legend = area.withHeight ((float) legendHeight).reduced (6.0f, 3.0f);
        g.setFont (juce::FontOptions (11.0f));

        auto key = [&] (juce::Colour colour, float height, const juce::String& text, bool outlined)
        {
            const float w = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text);
            auto item = legend.removeFromRight (w + 30.0f);
            legend.removeFromRight (10.0f);
            const auto swatch = item.removeFromLeft (22.0f).withSizeKeepingCentre (20.0f, height);
            g.setColour (outlined ? colour.withAlpha (0.25f) : colour);
            g.fillRoundedRectangle (swatch, 2.0f);

            if (outlined)
            {
                g.setColour (colour.withAlpha (0.7f));
                g.drawRoundedRectangle (swatch, 2.0f, 1.0f);
            }

            g.setColour (theme::textDim);
            g.drawText (text, item.withTrimmedLeft (4.0f), juce::Justification::centredLeft);
        };

        key (theme::meterGreen, 12.0f, "boost to bring out", true);
        key (theme::meterRed, 12.0f, "problems hide here", true);
        key (theme::accent.withAlpha (0.3f), 4.0f, "overtones", false);
        key (theme::accent.withAlpha (0.8f), 7.0f, "notes", false);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("INSTRUMENTS", juce::Rectangle<float> (0.0f, area.getY(), (float) nameWidth - 10.0f, (float) legendHeight),
                    juce::Justification::centredRight);
    }

    for (int i = 0; i < (int) list.size(); ++i)
    {
        const auto& instrument = list[(size_t) i];
        const auto row = getInstrumentRow (i);
        const bool isFocus = i == focus, greyed = focus >= 0 && ! isFocus;

        if (isFocus)
        {
            g.setColour (theme::panelLight.withAlpha (0.7f));
            g.fillRect (row);

            if (i == selectedInstrument)
            {
                g.setColour (theme::accent);
                g.fillRect (row.withWidth (3.0f));
            }
        }
        else if (i % 2 == 1)
        {
            g.setColour (theme::laneAlt.withAlpha (0.6f));
            g.fillRect (row.withLeft (area.getX()));
        }

        g.setColour (isFocus ? theme::text : greyed ? grey.withAlpha (0.6f) : theme::textDim.brighter (0.25f));
        g.setFont (juce::FontOptions (12.0f, isFocus ? juce::Font::bold : juce::Font::plain));
        g.drawText (instrument.name, row.withWidth ((float) nameWidth - 10.0f), juce::Justification::centredRight, true);

        // The bars at the bottom of the row: the notes thick, the overtones thin and faint.
        const auto inner = row.reduced (0.0f, 2.0f);
        const float barY = inner.getBottom() - 6.0f;
        g.setColour (greyed ? grey.withAlpha (0.15f) : theme::accent.withAlpha (isFocus ? 0.45f : 0.3f));
        g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (frequencyToX (instrument.notesFrom), barY - 2.0f,
                                                                            frequencyToX (instrument.overtonesTo), barY + 2.0f), 2.0f);
        g.setColour (greyed ? grey.withAlpha (0.3f) : theme::accent.withAlpha (isFocus ? 1.0f : 0.8f));
        g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (frequencyToX (instrument.notesFrom), barY - 3.5f,
                                                                            frequencyToX (instrument.notesTo), barY + 3.5f), 2.0f);

        // The ranges worth knowing, named where the name fits.
        g.setFont (juce::FontOptions (10.5f));

        for (int z = 0; z < (int) instrument.zones.size(); ++z)
        {
            const auto& zone = instrument.zones[(size_t) z];
            const auto colour = greyed ? grey : zoneColour (zone);
            const bool zoneHovered = i == hoveredInstrument && z == hoveredZone;
            const auto box = juce::Rectangle<float>::leftTopRightBottom (frequencyToX (zone.from) + 1.0f, inner.getY(), frequencyToX (zone.to) - 1.0f, inner.getBottom());

            g.setColour (colour.withAlpha (zoneHovered ? 0.35f : greyed ? 0.05f : 0.14f));
            g.fillRoundedRectangle (box, 3.0f);
            g.setColour (colour.withAlpha (zoneHovered ? 1.0f : greyed ? 0.2f : 0.55f));
            g.drawRoundedRectangle (box, 3.0f, zoneHovered ? 1.5f : 1.0f);

            if ((float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), zone.name) + 6.0f <= box.getWidth())
            {
                g.setColour (zoneHovered ? juce::Colours::white : greyed ? grey.withAlpha (0.45f) : colour.brighter (0.5f));
                g.drawText (zone.name, box.withBottom (barY - 3.5f), juce::Justification::centred);
            }
        }
    }

    // The range the mouse is on, over the whole height, to see which bands touch it.
    if (juce::isPositiveAndBelow (hoveredInstrument, (int) list.size()))
    {
        const auto& zones = list[(size_t) hoveredInstrument].zones;

        if (juce::isPositiveAndBelow (hoveredZone, (int) zones.size()))
        {
            const auto& z = zones[(size_t) hoveredZone];
            g.setColour (zoneColour (z).withAlpha (0.07f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (frequencyToX (z.from), area.getY(), frequencyToX (z.to), area.getBottom()));
        }
    }
}

void EqGraph::paintCurves (juce::Graphics& g, juce::Rectangle<float> area)
{
    const double rate = getSampleRate();
    std::array<eq::Band, eq::numBands> bands;
    std::array<eq::Stages, eq::numBands> stages;

    for (int i = 0; i < eq::numBands; ++i)
    {
        bands[(size_t) i] = processor.getBand (i);
        stages[(size_t) i] = eq::makeStages (bands[(size_t) i], rate);
    }

    const int focus = dragging >= 0 ? dragging : hovered;
    const double extraDb = processor.getOutputDb() + (processor.isMatchingLevel() ? processor.matchGainDb.load() : 0.0f);
    juce::Path total;

    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (area.toNearestInt());

        for (int i = 0; i < eq::numBands; ++i)
        {
            if (! bands[(size_t) i].on)
                continue;

            juce::Path curve, fill;
            const float zeroY = gainToY (0.0);
            fill.startNewSubPath (area.getX(), zeroY);

            for (float x = area.getX(); x <= area.getRight(); x += 2.0f)
            {
                const double db = juce::Decibels::gainToDecibels (stages[(size_t) i].getMagnitude (xToFrequency (x), rate), -100.0);
                const float y = gainToY (juce::jmax (-40.0, db));
                fill.lineTo (x, y);

                if (x > area.getX())
                    curve.lineTo (x, y);
                else
                    curve.startNewSubPath (x, y);
            }

            fill.lineTo (area.getRight(), zeroY);
            fill.closeSubPath();
            const auto colour = eqtext::getBandColour (i);

            if (i == focus)
            {
                g.setColour (colour.withAlpha (0.2f));
                g.fillPath (fill);
            }

            g.setColour (colour.withAlpha (i == focus ? 0.9f : 0.5f));
            g.strokePath (curve, juce::PathStrokeType (i == focus ? 1.6f : 1.1f));
        }

        // What the whole EQ does: the bands added up (in dB), with the output and match level gain.
        for (float x = area.getX(); x <= area.getRight(); x += 2.0f)
        {
            const double hz = xToFrequency (x);
            double db = extraDb;

            for (int i = 0; i < eq::numBands; ++i)
                if (bands[(size_t) i].on)
                    db += juce::Decibels::gainToDecibels (stages[(size_t) i].getMagnitude (hz, rate), -100.0);

            const float y = gainToY (juce::jmax (-40.0, db));

            if (x > area.getX())
                total.lineTo (x, y);
            else
                total.startNewSubPath (x, y);
        }

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.strokePath (total, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // The dots, numbered like the band columns.
    for (int i = 0; i < eq::numBands; ++i)
    {
        const auto pos = getHandlePosition (i);
        const auto colour = eqtext::getBandColour (i);
        const auto circle = juce::Rectangle<float> (handleRadius * 2.0f, handleRadius * 2.0f).withCentre (pos);
        const bool on = bands[(size_t) i].on;

        if (on)
        {
            g.setColour (colour);
            g.fillEllipse (circle);
        }
        else
        {
            g.setColour (theme::background.withAlpha (0.8f));
            g.fillEllipse (circle);
            g.setColour (colour.withAlpha (0.6f));
            g.drawEllipse (circle, 1.5f);
        }

        if (i == focus)
        {
            g.setColour (juce::Colours::white);
            g.drawEllipse (circle.expanded (2.5f), 1.5f);
        }

        g.setColour (on ? juce::Colours::black.withAlpha (0.85f) : colour.withAlpha (0.8f));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (juce::String (i + 1), circle, juce::Justification::centred);
    }

    // The values of the band being dragged or pointed at, next to its dot.
    if (juce::isPositiveAndBelow (focus, eq::numBands))
    {
        const auto& b = bands[(size_t) focus];
        juce::String text = eq::getTypeName (b.type) + "  " + eqtext::formatFrequency (b.freq);

        if (eq::hasGain (b.type))
            text << "  " << eqtext::formatGain (b.gainDb);

        text << "  Q " << juce::String (b.q, 2);

        if (eq::isCut (b.type))
            text << "  " << b.slope << " dB/oct";

        const auto font = juce::Font (juce::FontOptions (12.0f));
        const float w = (float) juce::GlyphArrangement::getStringWidthInt (font, text) + 14.0f;
        const auto pos = getHandlePosition (focus);
        auto box = juce::Rectangle<float> (w, 20.0f).withCentre ({ pos.x, pos.y - 24.0f });

        if (box.getY() < area.getY() + 2.0f)
            box.setY (pos.y + 16.0f);

        box = box.constrainedWithin (area.reduced (2.0f));
        g.setColour (theme::background.withAlpha (0.92f));
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (eqtext::getBandColour (focus));
        g.drawRoundedRectangle (box, 4.0f, 1.0f);
        g.setColour (theme::text);
        g.setFont (font);
        g.drawText (text, box, juce::Justification::centred);
    }

    const int solo = processor.soloBand.load();

    if (juce::isPositiveAndBelow (solo, eq::numBands))
    {
        g.setColour (eqtext::getBandColour (solo));
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText ("Listening to band " + juce::String (solo + 1) + " only", area.reduced (40.0f, 6.0f), juce::Justification::topLeft);
    }
}

//==============================================================================
void EqGraph::mouseMove (const juce::MouseEvent& e)
{
    const int h = findHandle (e.position);

    if (h != hovered)
    {
        hovered = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();

        if (onHover)
            onHover (h);
    }

    if (showingInstruments)
        updateInstrumentHover (e.position, h >= 0);
}

void EqGraph::updateInstrumentHover (juce::Point<float> position, bool onDot)
{
    // The dots come first: on one, the explainer tells about the band.
    auto [instrument, zone] = onDot ? std::pair<int, int> { -1, -1 } : findInstrument (position);

    // With an instrument clicked, only its own row responds; the others stay grey until the next click.
    if (selectedInstrument >= 0 && instrument != selectedInstrument)
        instrument = zone = -1;

    if (instrument == hoveredInstrument && zone == hoveredZone)
        return;

    hoveredInstrument = instrument;
    hoveredZone = zone;
    repaint();

    // Off its row (and off the dots), the explainer goes back to the clicked instrument.
    const int shown = instrument >= 0 ? instrument : ! onDot ? selectedInstrument : -1;

    if (shown >= 0 && onInstrumentHover)
        onInstrumentHover (shown, instrument >= 0 ? zone : -1);
}

void EqGraph::mouseExit (const juce::MouseEvent&)
{
    if (hovered >= 0 && dragging < 0)
    {
        hovered = -1;
        repaint();
    }

    if (hoveredInstrument >= 0)
    {
        hoveredInstrument = hoveredZone = -1;
        repaint();

        if (selectedInstrument >= 0 && onInstrumentHover)
            onInstrumentHover (selectedInstrument, -1);
    }
}

void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    const int h = findHandle (e.position);

    if (h < 0)
    {
        // Clicking an instrument keeps it in focus; clicking it again, or off the rows, lets go,
        // and clicking another one moves the focus there.
        if (showingInstruments && ! e.mods.isPopupMenu() && e.getNumberOfClicks() == 1)
        {
            const int instrument = findInstrument (e.position).first;
            selectedInstrument = instrument == selectedInstrument ? -1 : instrument;
            hoveredInstrument = hoveredZone = -2;   // so the row under the mouse responds again
            updateInstrumentHover (e.position, false);
        }

        return;
    }

    if (e.mods.isPopupMenu())
    {
        showBandMenu (h);
        return;
    }

    dragging = h;
    dragOffset = getHandlePosition (h) - e.position;

    // Grabbing a band that's off switches it on: you want to hear what you're moving.
    if (! processor.getBand (h).on)
        setParameter (h, "on", 1.0f);

    for (auto* what : { "freq", "gain" })
        if (auto* p = processor.getBandParameter (h, what))
            p->beginChangeGesture();

    if (e.mods.isAltDown())
    {
        soloBeforeDrag = processor.soloBand.load();
        processor.soloBand = h;
    }

    if (onHover)
        onHover (h);
}

void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0)
        return;

    const auto p = e.position + dragOffset;
    setParameter (dragging, "freq", (float) juce::jlimit ((double) eq::minFreq, (double) eq::maxFreq, xToFrequency (p.x)));

    if (eq::hasGain (processor.getBand (dragging).type))
        setParameter (dragging, "gain", (float) juce::jlimit (-(double) eq::maxGainDb, (double) eq::maxGainDb, std::round (yToGain (p.y) * 10.0) / 10.0));

    // Alt solos the band for as long as it's held.
    const bool soloed = processor.soloBand.load() == dragging;

    if (e.mods.isAltDown() && ! soloed)
    {
        soloBeforeDrag = processor.soloBand.load();
        processor.soloBand = dragging;
    }
    else if (! e.mods.isAltDown() && soloed && soloBeforeDrag != dragging)
    {
        processor.soloBand = soloBeforeDrag;
    }

    repaint();
}

void EqGraph::mouseUp (const juce::MouseEvent&)
{
    if (dragging < 0)
        return;

    for (auto* what : { "freq", "gain" })
        if (auto* p = processor.getBandParameter (dragging, what))
            p->endChangeGesture();

    if (processor.soloBand.load() == dragging && soloBeforeDrag != dragging)
        processor.soloBand = soloBeforeDrag;

    soloBeforeDrag = -1;
    dragging = -1;
    repaint();
}

void EqGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (const int h = findHandle (e.position); h >= 0)
    {
        processor.setBand (h, eq::getDefaultBand (h));
        return;
    }

    // On an empty spot: a band that's off becomes a bell there, the bells first.
    for (int i : { 2, 3, 1, 4, 0, 5 })
    {
        if (processor.getBand (i).on)
            continue;

        const auto gain = (float) juce::jlimit (-(double) eq::maxGainDb, (double) eq::maxGainDb, std::round (yToGain (e.position.y) * 10.0) / 10.0);
        processor.setBand (i, { true, eq::BandType::bell, (float) xToFrequency (e.position.x), gain, 1.0f, 12 });

        if (onHover)
            onHover (i);

        return;
    }
}

void EqGraph::showBandMenu (int band)
{
    const auto b = processor.getBand (band);
    juce::PopupMenu menu;
    menu.addSectionHeader ("Band " + juce::String (band + 1));

    for (int t = 0; t < eq::numTypes; ++t)
        menu.addItem (1 + t, eq::getTypeName ((eq::BandType) t) + "  (" + eq::getTypeAlias ((eq::BandType) t) + ")", true, (int) b.type == t);

    menu.addSeparator();

    if (eq::isCut (b.type))
    {
        menu.addItem (20, "12 dB/oct", true, b.slope == 12);
        menu.addItem (21, "24 dB/oct", true, b.slope == 24);
        menu.addSeparator();
    }

    menu.addItem (30, b.on ? "Switch off" : "Switch on");
    menu.addItem (31, "Solo: hear only this band", true, processor.soloBand.load() == band);
    menu.addItem (32, "Reset");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safeThis = juce::Component::SafePointer<EqGraph> (this), band] (int result)
                        {
                            if (safeThis == nullptr || result <= 0)
                                return;

                            auto& eqProcessor = safeThis->processor;

                            if (result <= eq::numTypes)
                                safeThis->setParameter (band, "type", (float) (result - 1));
                            else if (result == 20 || result == 21)
                                safeThis->setParameter (band, "slope", result == 21 ? 1.0f : 0.0f);
                            else if (result == 30)
                                safeThis->setParameter (band, "on", eqProcessor.getBand (band).on ? 0.0f : 1.0f);
                            else if (result == 31)
                                eqProcessor.soloBand = eqProcessor.soloBand.load() == band ? -1 : band;
                            else if (result == 32)
                                eqProcessor.setBand (band, eq::getDefaultBand (band));

                            safeThis->repaint();
                        });
}

//==============================================================================
/** One band's controls under the graph: on, solo, type (and slope for cuts), frequency, gain and Q. */
class BuiltInEqEditor::BandControls final : public juce::Component
{
public:
    BandControls (BuiltInEq& p, int i) : processor (p), index (i)
    {
        auto& state = processor.getParameters();
        setTopic (*this, Topic::band, index);

        power.setButtonText ("On");
        powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, parameterId (index, "on"), power);
        setTopic (power, Topic::power, index);

        solo.setClickingTogglesState (false);
        solo.setColour (juce::TextButton::buttonOnColourId, eqtext::getBandColour (index).darker (0.2f));
        solo.onClick = [this] { processor.soloBand = processor.soloBand.load() == index ? -1 : index; };
        setTopic (solo, Topic::solo, index);

        for (int t = 0; t < eq::numTypes; ++t)
            type.addItem (eq::getTypeName ((eq::BandType) t), t + 1);

        typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, parameterId (index, "type"), type);
        setTopic (type, Topic::type, index);

        // A cut's slope, 12 or 24 dB per octave: one click switches.
        slope.onClick = [this]
        {
            if (auto* parameter = processor.getBandParameter (index, "slope"))
                parameter->setValueNotifyingHost (processor.getBand (index).slope >= 24 ? 0.0f : 1.0f);
        };
        setTopic (slope, Topic::slope, index);

        const std::pair<juce::Slider*, const char*> sliders[] = { { &freq, "freq" }, { &gain, "gain" }, { &q, "q" } };
        const Topic topics[] = { Topic::frequency, Topic::gain, Topic::q };
        const char* names[] = { "Freq", "Gain", "Q" };

        for (int s = 0; s < 3; ++s)
        {
            auto* slider = sliders[s].first;
            slider->setScrollWheelEnabled (false);   // scrolling never changes a value
            slider->setColour (juce::Slider::trackColourId, eqtext::getBandColour (index).withAlpha (0.45f));
            slider->setColour (juce::Slider::textBoxTextColourId, theme::text);
            attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId (index, sliders[s].second), *slider));
            setTopic (*slider, topics[s], index);

            auto* label = labels.add (new juce::Label ({}, names[s]));
            label->setFont (juce::FontOptions (11.5f));
            label->setColour (juce::Label::textColourId, theme::textDim);
            setTopic (*label, topics[s], index);
            addAndMakeVisible (label);
        }

        for (juce::Component* c : std::initializer_list<juce::Component*> { &power, &solo, &type, &slope, &freq, &gain, &q })
        {
            c->setWantsKeyboardFocus (false);
            addAndMakeVisible (c);
        }

        update();
    }

    /** Follows the band: solo, the slope (cuts only), gain (not for cuts and notches). */
    void update()
    {
        const auto b = processor.getBand (index);
        solo.setToggleState (processor.soloBand.load() == index, juce::dontSendNotification);

        if (slope.isVisible() != eq::isCut (b.type))
        {
            slope.setVisible (eq::isCut (b.type));
            resized();
        }

        slope.setButtonText (juce::String (b.slope) + " dB");
        gain.setEnabled (eq::hasGain (b.type));
        labels[1]->setEnabled (eq::hasGain (b.type));

        if (b.on != shownOn)
        {
            shownOn = b.on;
            for (auto* c : { (juce::Component*) &type, (juce::Component*) &slope, (juce::Component*) &freq, (juce::Component*) &gain, (juce::Component*) &q })
                c->setAlpha (b.on ? 1.0f : 0.5f);

            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto colour = eqtext::getBandColour (index);
        auto bounds = getLocalBounds().toFloat();
        g.setColour (theme::panelLight.withAlpha (shownOn ? 1.0f : 0.6f));
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (colour.withAlpha (shownOn ? 1.0f : 0.4f));
        g.fillRoundedRectangle (bounds.removeFromTop (4.0f), 2.0f);

        const auto number = juce::Rectangle<float> (18.0f, 18.0f).withCentre ({ 15.0f, 17.0f });
        g.fillEllipse (number);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (juce::String (index + 1), number, juce::Justification::centred);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 6);
        r.removeFromTop (2);
        auto top = r.removeFromTop (22);
        top.removeFromLeft (22);
        solo.setBounds (top.removeFromRight (40));
        top.removeFromRight (4);
        power.setBounds (top);

        r.removeFromTop (5);
        auto typeRow = r.removeFromTop (22);

        if (slope.isVisible())
        {
            slope.setBounds (typeRow.removeFromRight (42));
            typeRow.removeFromRight (4);
        }

        type.setBounds (typeRow);

        for (int s = 0; s < 3; ++s)
        {
            r.removeFromTop (5);
            auto row = r.removeFromTop (22);
            labels[s]->setBounds (row.removeFromLeft (32));
            (s == 0 ? freq : s == 1 ? gain : q).setBounds (row);
        }
    }

private:
    BuiltInEq& processor;
    const int index;
    bool shownOn = true;

    juce::ToggleButton power;
    juce::TextButton solo { "Solo" };
    juce::ComboBox type;
    juce::TextButton slope;
    juce::Slider freq { juce::Slider::LinearBar, juce::Slider::TextBoxLeft }, gain { juce::Slider::LinearBar, juce::Slider::TextBoxLeft },
                 q { juce::Slider::LinearBar, juce::Slider::TextBoxLeft };
    juce::OwnedArray<juce::Label> labels;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
};

//==============================================================================
BuiltInEqEditor::BuiltInEqEditor (BuiltInEq& p) : juce::AudioProcessorEditor (&p), eqProcessor (p)
{
    title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (title);

    presets.setTextWhenNothingSelected ("Starting point...");
    const auto& list = eq::getPresets();

    for (size_t i = 0; i < list.size(); ++i)
        presets.addItem (list[i].name, (int) i + 1);

    presets.onChange = [this]
    {
        const int index = presets.getSelectedId() - 1;

        if (juce::isPositiveAndBelow (index, (int) eq::getPresets().size()))
        {
            eqProcessor.applyPreset (eq::getPresets()[(size_t) index]);
            explainer.showPreset (index);
        }
    };
    setTopic (presets, Topic::presets);

    lessonButton.onClick = [this]
    {
        // The band being looked at, if it's a bell or notch; otherwise band 3, the first bell.
        const int focused = explainer.getFocusedBand();
        const auto type = focused >= 0 ? eqProcessor.getBand (focused).type : eq::BandType::lowCut;
        explainer.startLesson (type == eq::BandType::bell || type == eq::BandType::notch ? focused : 2);
    };
    explainer.onLessonChanged = [this] { lessonButton.setEnabled (! explainer.isInLesson()); };

    instrumentsButton.setClickingTogglesState (true);
    instrumentsButton.setTooltip ("Show where common instruments live in the spectrum");
    instrumentsButton.onClick = [this]
    {
        graph.setShowingInstruments (instrumentsButton.getToggleState());

        if (instrumentsButton.getToggleState())
            explainer.showTopic (Topic::instruments);
        else
            explainer.showOverview();
    };
    setTopic (instrumentsButton, Topic::instruments);

    guideButton.setTooltip ("What is EQ?: a guide for beginners to equalising, with pictures that follow this window: the frequency "
                            "ranges, band types, Q, slopes, where instruments sit, masking and common problems");
    if (fxui::isPlugin (*getAudioProcessor()))
        guideButton.setTooltip ("Opens the guide to " + getAudioProcessor()->getName() + " inside this window, with pictures that follow your settings.");
    guideButton.onClick = [this] { openGuide(); };

    helpButton.setTooltip ("How the EQ works and how to read the graph");
    helpButton.onClick = [this] { explainer.showOverview(); };

    matchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (eqProcessor.getParameters(), "match", matchButton);
    setTopic (matchButton, Topic::match);
    matchValue.setColour (juce::Label::textColourId, theme::textDim);
    matchValue.setFont (juce::FontOptions (12.0f));
    setTopic (matchValue, Topic::match);

    outputLabel.setColour (juce::Label::textColourId, theme::textDim);
    outputLabel.setJustificationType (juce::Justification::centredRight);
    output.setScrollWheelEnabled (false);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (eqProcessor.getParameters(), "output", output);
    setTopic (output, Topic::output);
    setTopic (outputLabel, Topic::output);

    for (juce::Component* c : std::initializer_list<juce::Component*> { &presets, &lessonButton, &instrumentsButton, &guideButton, &helpButton, &matchButton, &matchValue, &outputLabel, &output })
    {
        c->setWantsKeyboardFocus (false);
        addAndMakeVisible (c);
    }

    graph.onHover = [this] (int band)
    {
        if (band >= 0)
            explainer.showBand (band, Topic::band);
    };

    graph.onInstrumentHover = [this] (int instrument, int zone) { explainer.showInstrument (instrument, zone); };

    addAndMakeVisible (graph);
    addAndMakeVisible (explainer);

    for (int i = 0; i < eq::numBands; ++i)
        addAndMakeVisible (bands.add (new BandControls (eqProcessor, i)));

    addMouseListener (this, true);
    setSize (1240, 640);
    startTimerHz (30);
}

void BuiltInEqEditor::openGuide (int page)
{
    guideHost.show ("What is EQ?", [this]
    {
        return new fxui::Guide ("What is EQ?", eqtext::makeGuide (eqProcessor), nullptr,
                                "The pictures follow the settings in DawIt EQ's window.");
    }, "the settings in " + getAudioProcessor()->getName() + "'s window", page);
}

BuiltInEqEditor::~BuiltInEqEditor()
{
    removeMouseListener (this);

    // Solo is for listening while the window is open; closing it plays the whole EQ again.
    eqProcessor.soloBand = -1;
}

void BuiltInEqEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    g.setColour (theme::panel);
    g.fillRect (getLocalBounds().removeFromTop (headerHeight));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, headerHeight - 1, getWidth(), 1);
}

void BuiltInEqEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (12, 9);

    title.setBounds (header.removeFromLeft (92));
    presets.setBounds (header.removeFromLeft (190));
    header.removeFromLeft (10);
    lessonButton.setBounds (header.removeFromLeft (170));
    header.removeFromLeft (10);
    instrumentsButton.setBounds (header.removeFromLeft (110));
    header.removeFromLeft (10);
    guideButton.setBounds (header.removeFromLeft (134));

    helpButton.setBounds (header.removeFromRight (28));
    header.removeFromRight (14);
    output.setBounds (header.removeFromRight (110));
    outputLabel.setBounds (header.removeFromRight (54));
    header.removeFromRight (12);
    matchValue.setBounds (header.removeFromRight (64));
    matchButton.setBounds (header.removeFromRight (110));

    explainer.setBounds (r.removeFromRight (explainerWidth));

    auto bandArea = r.removeFromBottom (bandsHeight).reduced (9, 8);
    const int columnWidth = bandArea.getWidth() / eq::numBands;

    for (auto* b : bands)
        b->setBounds (bandArea.removeFromLeft (columnWidth).reduced (3, 0));

    graph.setBounds (r.withTrimmedLeft (12).withTrimmedRight (12).withTrimmedTop (10));
}

void BuiltInEqEditor::mouseEnter (const juce::MouseEvent& e)
{
    // Explain whatever control the mouse is on (the graph tells about its dots itself).
    for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
    {
        const auto& properties = c->getProperties();

        if (! properties.contains ("eqTopic"))
            continue;

        const auto topic = (Topic) (int) properties["eqTopic"];

        if (properties.contains ("eqBand"))
            explainer.showBand ((int) properties["eqBand"], topic);
        else
            explainer.showTopic (topic);

        return;
    }
}

void BuiltInEqEditor::timerCallback()
{
    graph.updateSpectrum();
    graph.repaint();

    for (auto* b : bands)
        b->update();

    matchValue.setText (eqProcessor.isMatchingLevel() ? eqtext::formatGain (eqProcessor.matchGainDb.load()) : juce::String(), juce::dontSendNotification);

    if (++ticks % 3 == 0)
        explainer.refresh();
}
} // namespace daw
