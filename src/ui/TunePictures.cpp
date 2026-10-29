#include "TunePictures.h"

#include "Theme.h"
#include "generate/SingingGuide.h"

namespace daw::tunepics
{
juce::Colour getZoneColour (tune::Zone zone)
{
    switch (zone)
    {
        case tune::Zone::inTune: return theme::meterGreen;
        case tune::Zone::close:  return theme::meterYellow;
        case tune::Zone::out:    return theme::meterRed;
    }

    return theme::meterGreen;
}

Nearest getNearest (double midi)
{
    const int note = juce::roundToInt (midi);
    return { note, (midi - note) * 100.0 };
}

juce::String getNoteName (int midi) { return gen::singing::getNoteName (midi); }

juce::String describeCents (double cents)
{
    const int n = juce::roundToInt (std::abs (cents));

    if (std::abs (cents) < 0.5)
        return "in tune";

    return juce::String (n) + (n == 1 ? " cent " : " cents ") + (cents < 0.0 ? "flat" : "sharp");
}

namespace
{
    bool isBlack (int midi) { const int pc = ((midi % 12) + 12) % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
    bool inKey (int midi, int mask) { return (mask & (1 << (((midi % 12) + 12) % 12))) != 0; }
}

void paintTuner (juce::Graphics& g, juce::Rectangle<float> area, double sungMidi, const KeyInfo& key)
{
    TunerPicture picture;
    picture.has = sungMidi >= 0.0;
    const auto n = getNearest (sungMidi);

    if (picture.has)
    {
        picture.note = getNoteName (n.note);
        picture.cents = n.cents;
        picture.line = describeCents (n.cents);

        if (key.name.isNotEmpty())
        {
            const bool in = inKey (n.note, key.mask);
            auto name = getNoteName (n.note);
            picture.extraLine = in ? "in " + key.name : "not in " + key.name + ": " + name.dropLastCharacters (name.length() - (name.containsChar ('#') ? 2 : 1));
        }
    }

    paintTuner (g, area, picture);
}

void paintTuner (juce::Graphics& g, juce::Rectangle<float> area, const TunerPicture& p)
{
    const bool has = p.has;
    const float fade = juce::jlimit (0.0f, 1.0f, p.fade);
    const double a = std::abs (p.cents);
    const auto zoneColour = has ? getZoneColour (a <= p.greenCents ? tune::Zone::inTune : (a <= p.yellowCents ? tune::Zone::close : tune::Zone::out)).withMultipliedAlpha (0.4f + 0.6f * fade)
                                : theme::textDim;

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jlimit (22.0f, p.maxNoteSize, area.getHeight() * 0.3f), juce::Font::bold));
    auto top = area.removeFromTop (area.getHeight() * 0.38f);
    g.setColour ((has ? theme::text : theme::textDim).withMultipliedAlpha (has ? 0.4f + 0.6f * fade : 1.0f));
    g.drawText (has ? p.note : juce::String::fromUTF8 ("\xe2\x80\x93"), top, juce::Justification::centred);

    // The needle: -50 to +50 cents, with the zones behind it.
    auto needle = area.removeFromTop (area.getHeight() * 0.35f).reduced (14.0f, 6.0f);
    auto scale = needle.withTrimmedBottom (needle.getHeight() * 0.35f);
    auto xFor = [&] (double cents) { return scale.getX() + (float) ((juce::jlimit (-50.0, 50.0, cents) + 50.0) / 100.0) * scale.getWidth(); };
    g.setColour (getZoneColour (tune::Zone::out).withAlpha (0.25f));
    g.fillRoundedRectangle (scale, 3.0f);
    g.setColour (getZoneColour (tune::Zone::close).withAlpha (0.35f));
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (-p.yellowCents), scale.getY(), xFor (p.yellowCents), scale.getBottom()));
    g.setColour (getZoneColour (tune::Zone::inTune).withAlpha (0.45f));
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (-p.greenCents), scale.getY(), xFor (p.greenCents), scale.getBottom()));
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.0f));

    for (const int c : { -50, -25, 0, 25, 50 })
    {
        g.fillRect (xFor (c) - 0.5f, scale.getBottom(), 1.0f, 4.0f);
        g.drawText (c > 0 ? "+" + juce::String (c) : juce::String (c), juce::Rectangle<float> (xFor (c) - 16.0f, scale.getBottom() + 4.0f, 32.0f, 12.0f), juce::Justification::centred);
    }

    if (has)
    {
        g.setColour (zoneColour);
        g.fillRoundedRectangle (juce::Rectangle<float> (4.0f, scale.getHeight() + 8.0f).withCentre ({ xFor (p.cents), scale.getCentreY() }), 2.0f);
    }

    g.setColour (zoneColour);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (has ? p.line : (p.line.isNotEmpty() ? p.line : juce::String ("no clear pitch")), area.removeFromTop (22.0f), juce::Justification::centred);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.5f));

    if (has && p.extraLine.isNotEmpty())
        g.drawText (p.extraLine, area.removeFromTop (20.0f), juce::Justification::centred);

    if (has && p.noteInfo.isNotEmpty())
        g.drawText (p.noteInfo, area.removeFromTop (20.0f), juce::Justification::centred);
}

void paintMiniTuner (juce::Graphics& g, juce::Rectangle<float> area, double sungMidi, bool dim)
{
    const bool has = sungMidi >= 0.0;
    const auto n = getNearest (sungMidi);
    const auto c = has && ! dim ? getZoneColour (tune::getZone (n.cents)) : theme::textDim;
    g.setColour (c);
    g.setFont (juce::FontOptions (juce::jmin (13.0f, area.getHeight() * 0.5f), juce::Font::bold));
    g.drawText (has ? getNoteName (n.note) : juce::String::fromUTF8 ("\xe2\x80\x93"), area.removeFromTop (area.getHeight() * 0.55f), juce::Justification::centred);
    auto bar = area.reduced (3.0f, 1.0f);
    g.setColour (theme::gridBar);
    g.fillRect (bar.withSizeKeepingCentre (bar.getWidth(), 1.0f));
    g.fillRect (bar.withSizeKeepingCentre (1.0f, bar.getHeight()));

    if (has)
    {
        const float x = bar.getCentreX() + (float) (juce::jlimit (-50.0, 50.0, n.cents) / 50.0) * bar.getWidth() * 0.5f;
        g.setColour (c);
        g.fillRect (juce::Rectangle<float> (3.0f, bar.getHeight()).withCentre ({ x, bar.getCentreY() }));
    }
}

namespace
{
    struct Rows { int low = 48, count = 15; };

    Rows getRows (const Trace& t, int first)
    {
        Rows rows;

        if (t.fixedLow >= 0)
        {
            rows.low = t.fixedLow;
            return rows;
        }

        std::vector<float> pitches;
        const auto& frames = *t.frames;
        const int recent = juce::jmax (1, (int) (3.0 * t.framesPerSecond));

        for (int i = (int) frames.size() - 1; i >= first && (int) pitches.size() < recent; --i)
            if (frames[(size_t) i].sung >= 0.0f)
                pitches.push_back (frames[(size_t) i].sung);

        if (pitches.empty())
            for (int i = first; i < (int) frames.size(); ++i)
                if (frames[(size_t) i].sung >= 0.0f)
                    pitches.push_back (frames[(size_t) i].sung);

        float centre = 60.0f;

        if (! pitches.empty())
        {
            std::sort (pitches.begin(), pitches.end());
            centre = pitches[pitches.size() / 2];
        }

        rows.low = juce::roundToInt (centre) - 7;
        return rows;
    }
}

void paintTrace (juce::Graphics& g, juce::Rectangle<float> area, const Trace& t)
{
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillRoundedRectangle (area, 4.0f);

    if (t.frames == nullptr)
        return;

    const auto& frames = *t.frames;
    const int shown = juce::jmax (2, juce::roundToInt (t.seconds * t.framesPerSecond));
    const int first = juce::jmax (0, (int) frames.size() - shown);
    const auto rows = getRows (t, first);
    auto plot = area.reduced (4.0f);
    auto margin = plot.removeFromLeft (traceMargin);
    plot.removeFromBottom (t.showBars ? 14.0f : 0.0f);
    const float rowHeight = plot.getHeight() / (float) rows.count;
    auto yFor = [&] (double pitch) { return plot.getBottom() - (float) (pitch - rows.low + 0.5) * rowHeight; };
    const float pixelsPerFrame = plot.getWidth() / (float) shown;
    auto xFor = [&] (int i) { return plot.getRight() - (float) ((int) frames.size() - 1 - i) * pixelsPerFrame; };

    g.saveState();
    g.reduceClipRegion (plot.getSmallestIntegerContainer());

    for (int r = 0; r < rows.count; ++r)
    {
        const int note = rows.low + r;
        const auto row = juce::Rectangle<float> (plot.getX(), yFor (note) - rowHeight * 0.5f, plot.getWidth(), rowHeight);
        g.setColour (inKey (note, t.keyMask) ? theme::panelLight.withAlpha (0.65f) : (isBlack (note) ? theme::background.withAlpha (0.5f) : theme::lane.withAlpha (0.4f)));
        g.fillRect (row.reduced (0.0f, 0.5f));
    }

    // Bar numbers.
    if (t.showBars)
    {
        int lastBar = -1;
        g.setFont (juce::FontOptions (9.5f));

        for (int i = first; i < (int) frames.size(); ++i)
        {
            if (frames[(size_t) i].beat < 0.0f)
                continue;

            const int bar = (int) std::floor (frames[(size_t) i].beat / 4.0f);

            if (bar != lastBar)
            {
                lastBar = bar;
                g.setColour (theme::gridBar.withAlpha (0.6f));
                g.fillRect (xFor (i), plot.getY(), 1.0f, plot.getHeight());
            }
        }
    }

    auto pathFor = [&] (bool corrected, std::function<void (juce::Point<float>, juce::Point<float>, const tune::Frame&)> segment)
    {
        for (int i = juce::jmax (first + 1, 1); i < (int) frames.size(); ++i)
        {
            const auto& a = frames[(size_t) i - 1];
            const auto& b = frames[(size_t) i];
            const float pa = corrected ? a.corrected : a.sung, pb = corrected ? b.corrected : b.sung;

            if (pa < 0.0f || pb < 0.0f || a.sung < 0.0f || b.sung < 0.0f)
                continue;

            segment ({ xFor (i - 1), yFor (pa) }, { xFor (i), yFor (pb) }, b);
        }
    };

    if (t.showCorrected)
    {
        juce::Path corrected;
        bool open = false;
        int previous = -2;
        int index = 0;

        for (int i = first; i < (int) frames.size(); ++i, ++index)
        {
            const auto& f = frames[(size_t) i];

            if (f.corrected < 0.0f || f.sung < 0.0f)
            {
                open = false;
                continue;
            }

            const juce::Point<float> p { xFor (i), yFor (f.corrected) };

            if (! open || previous != i - 1)
                corrected.startNewSubPath (p);
            else
                corrected.lineTo (p);

            open = true;
            previous = i;
        }

        g.setColour (theme::accent);
        g.strokePath (corrected, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    pathFor (false, [&] (juce::Point<float> a, juce::Point<float> b, const tune::Frame& f)
    {
        const auto n = getNearest (f.sung);
        g.setColour (getZoneColour (tune::getZone (n.cents)));
        g.drawLine (a.x, a.y, b.x, b.y, 1.8f);
    });

    g.restoreState();

    // The note names, the key's in the text colour.
    g.setFont (juce::FontOptions (10.0f));

    for (int r = 0; r < rows.count; ++r)
    {
        const int note = rows.low + r;
        g.setColour (inKey (note, t.keyMask) ? theme::text : theme::textDim.withAlpha (0.6f));
        g.drawText (getNoteName (note), juce::Rectangle<float> (margin.getX(), yFor (note) - rowHeight * 0.5f, margin.getWidth() - 2.0f, rowHeight), juce::Justification::centredRight);
    }

    if (t.showBars)
    {
        int lastBar = -1;
        float lastLabel = -1000.0f;
        g.setFont (juce::FontOptions (9.5f));
        g.setColour (theme::textDim);

        for (int i = first; i < (int) frames.size(); ++i)
        {
            if (frames[(size_t) i].beat < 0.0f)
                continue;

            const int bar = (int) std::floor (frames[(size_t) i].beat / 4.0f);

            if (bar != lastBar)
            {
                lastBar = bar;
                const float x = xFor (i);

                if (x + 28.0f < plot.getRight() && x >= plot.getX() && x - lastLabel >= 26.0f)
                {
                    lastLabel = x;
                    g.drawText (juce::String (bar + 1), juce::Rectangle<float> (x + 2.0f, plot.getBottom() + 1.0f, 28.0f, 12.0f), juce::Justification::centredLeft);
                }
            }
        }
    }
}

TraceHit hitTrace (juce::Rectangle<float> area, const Trace& t, juce::Point<float> p)
{
    TraceHit hit;

    if (t.frames == nullptr || t.frames->empty())
        return hit;

    const auto& frames = *t.frames;
    auto plot = area.reduced (4.0f);
    plot.removeFromLeft (traceMargin);
    plot.removeFromBottom (t.showBars ? 14.0f : 0.0f);

    if (! plot.contains (p))
        return hit;

    const int shown = juce::jmax (2, juce::roundToInt (t.seconds * t.framesPerSecond));
    const int first = juce::jmax (0, (int) frames.size() - shown);
    const float pixelsPerFrame = plot.getWidth() / (float) shown;
    const int index = juce::jlimit (first, (int) frames.size() - 1, (int) frames.size() - 1 - juce::roundToInt ((plot.getRight() - p.x) / pixelsPerFrame));
    const auto& f = frames[(size_t) index];
    hit.valid = true;
    hit.frame = index;
    hit.beat = f.beat;

    if (f.sung >= 0.0f)
    {
        hit.sung = f.sung;
        hit.nearest = getNearest (f.sung);
    }
    else
    {
        // No clear pitch there: say what row the mouse is on instead.
        const auto rows = getRows (t, first);
        const float rowHeight = plot.getHeight() / (float) rows.count;
        hit.nearest.note = rows.low + juce::roundToInt ((plot.getBottom() - p.y) / rowHeight - 0.5f);
        hit.sung = -1.0;
    }

    return hit;
}

juce::String describeHit (const TraceHit& hit, const KeyInfo& key)
{
    if (! hit.valid)
        return {};

    const juce::String dot = juce::String::fromUTF8 (" \xc2\xb7 ");
    juce::String text;

    if (hit.beat >= 0.0)
        text << "Bar " << (int) std::floor (hit.beat / 4.0) + 1 << dot;

    if (hit.sung < 0.0)
        return text + "no clear pitch here: a consonant, a breath, silence or several notes";

    text << getNoteName (hit.nearest.note) << dot << describeCents (hit.nearest.cents);

    if (key.name.isNotEmpty())
        text << dot << (inKey (hit.nearest.note, key.mask) ? "in " : "not in ") << key.name;

    return text;
}

std::vector<tune::Frame> makeFrames (const std::vector<float>& sung, const std::vector<float>& corrected)
{
    std::vector<tune::Frame> frames (sung.size());

    for (size_t i = 0; i < sung.size(); ++i)
    {
        frames[i].sung = sung[i];
        frames[i].corrected = i < corrected.size() ? corrected[i] : sung[i];
        frames[i].rmsDb = sung[i] >= 0.0f ? -20.0f : -100.0f;
    }

    return frames;
}
} // namespace daw::tunepics
