#include "ScorePainter.h"
#include <DawItFonts.h>

namespace daw::scorepaint
{
using namespace gen::notation;

namespace
{
    using Glyph = juce::juce_wchar;

    // SMuFL code points (Standard Music Font Layout glyphnames.json).
    constexpr Glyph gClef = 0xE050, gClef8vb = 0xE052, fClef = 0xE062, fClef8vb = 0xE064;
    constexpr Glyph noteheadWhole = 0xE0A2, noteheadHalf = 0xE0A3, noteheadBlack = 0xE0A4;
    constexpr Glyph accFlat = 0xE260, accNatural = 0xE261, accSharp = 0xE262;
    constexpr Glyph augmentationDot = 0xE1E7, timeSig0 = 0xE080, tuplet0 = 0xE880, metNoteQuarterUp = 0xE1D5;
    constexpr Glyph restWhole = 0xE4E3, restHalf = 0xE4E4, restQuarter = 0xE4E5, rest8th = 0xE4E6, rest16th = 0xE4E7,
                         rest32nd = 0xE4E8;
    constexpr Glyph flag8thUp = 0xE240, flag8thDown = 0xE241, flag16thUp = 0xE242, flag16thDown = 0xE243,
                         flag32ndUp = 0xE244, flag32ndDown = 0xE245;

    // Sizes in staff spaces, from Bravura's metadata.
    constexpr float headWidth = 1.18f, wholeHeadWidth = 1.688f, stemThickness = 0.12f, staffLineThickness = 0.13f,
                    ledgerThickness = 0.16f, barLineThickness = 0.16f, beamThickness = 0.5f, beamGap = 0.25f,
                    staffGap = 8.0f;

    juce::Typeface::Ptr getBravura()
    {
        static const juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor (dawfonts::Bravura_otf, (size_t) dawfonts::Bravura_otfSize);
        return typeface;
    }

    /** Draws a glyph with its origin at (x, baseline). One em is four staff spaces. */
    void drawGlyph (juce::Graphics& g, Glyph code, float x, float baseline, float space, juce::Colour colour, float scale = 1.0f)
    {
        juce::Font font (juce::FontOptions (getBravura()).withPointHeight (4.0f * space * scale));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (font, juce::String::charToString (code), x, baseline);
        g.setColour (colour);
        glyphs.draw (g);
    }

    float getHeadWidth (Value v) { return v == Value::whole ? wholeHeadWidth : headWidth; }

    float getY (float staffTop, int step, float space) { return staffTop + (4.0f - (float) step * 0.5f) * space; }

    int getBeamLevels (Value v)
    {
        switch (v)
        {
            case Value::thirtySecond: return 3;
            case Value::sixteenth:    return 2;
            case Value::eighth:       return 1;
            case Value::whole:
            case Value::half:
            case Value::quarter:      break;
        }

        return 0;
    }

    Glyph getRestGlyph (Value v)
    {
        switch (v)
        {
            case Value::whole:        return restWhole;
            case Value::half:         return restHalf;
            case Value::quarter:      return restQuarter;
            case Value::eighth:       return rest8th;
            case Value::sixteenth:    return rest16th;
            case Value::thirtySecond: return rest32nd;
        }

        return restQuarter;
    }

    Glyph getFlagGlyph (Value v, bool up)
    {
        switch (v)
        {
            case Value::thirtySecond: return up ? flag32ndUp : flag32ndDown;
            case Value::sixteenth:    return up ? flag16thUp : flag16thDown;
            case Value::eighth:       return up ? flag8thUp : flag8thDown;
            case Value::whole:
            case Value::half:
            case Value::quarter:      break;
        }

        return 0;
    }

    Glyph getAccidentalGlyph (int alter) { return alter < 0 ? accFlat : alter > 0 ? accSharp : accNatural; }

    /** Where one event's parts go, in pixels. */
    struct Geometry
    {
        std::vector<float> headLeft, headY;     // one per pitch
        float left = 0.0f, right = 0.0f;        // the heads' extent
        float stemX = 0.0f, stemStart = 0.0f, tip = 0.0f;
        bool hasStem = false;
    };

    Geometry getGeometry (const Event& e, float x, float staffTop, float space)
    {
        Geometry geo;
        const size_t n = e.pitches.size();
        geo.headLeft.assign (n, x);
        geo.headY.assign (n, 0.0f);

        for (size_t i = 0; i < n; ++i)
            geo.headY[i] = getY (staffTop, e.pitches[i].step, space);

        const float w = getHeadWidth (e.value) * space;

        // Notes a second apart can't share a column: the one on the stem's side moves across the stem.
        if (e.value != Value::whole && n > 1)
        {
            const int count = (int) n;

            if (e.stemUp)
            {
                bool previousDisplaced = false;

                for (int i = 1; i < count; ++i)
                {
                    const bool displace = e.pitches[(size_t) i].step - e.pitches[(size_t) i - 1].step == 1 && ! previousDisplaced;

                    if (displace)
                        geo.headLeft[(size_t) i] = x + w;

                    previousDisplaced = displace;
                }
            }
            else
            {
                bool previousDisplaced = false;

                for (int i = count - 2; i >= 0; --i)
                {
                    const bool displace = e.pitches[(size_t) i + 1].step - e.pitches[(size_t) i].step == 1 && ! previousDisplaced;

                    if (displace)
                        geo.headLeft[(size_t) i] = x - w;

                    previousDisplaced = displace;
                }
            }
        }

        geo.left = *std::min_element (geo.headLeft.begin(), geo.headLeft.end());
        geo.right = *std::max_element (geo.headLeft.begin(), geo.headLeft.end()) + w;

        geo.hasStem = e.value != Value::whole;

        if (geo.hasStem)
        {
            const int lowest = e.pitches.front().step, highest = e.pitches.back().step;

            if (e.stemUp)
            {
                geo.stemX = x + w - stemThickness * space * 0.5f;
                geo.stemStart = getY (staffTop, lowest, space);
                // A stem that would end below the middle line is lengthened to it.
                geo.tip = getY (staffTop, juce::jmax (highest + 7, 4), space);
            }
            else
            {
                geo.stemX = x + stemThickness * space * 0.5f;
                geo.stemStart = getY (staffTop, highest, space);
                geo.tip = getY (staffTop, juce::jmin (lowest - 7, 4), space);
            }
        }

        return geo;
    }

    void fillRect (juce::Graphics& g, float x, float y, float w, float h, juce::Colour c)
    {
        g.setColour (c);
        g.fillRect (x, y, w, h);
    }

    void drawStaffLines (juce::Graphics& g, float x0, float x1, float staffTop, float space, juce::Colour c)
    {
        for (int i = 0; i < 5; ++i)
            fillRect (g, x0, staffTop + (float) i * space - staffLineThickness * space * 0.5f, x1 - x0, staffLineThickness * space, c);
    }

    /** The curve of a tie from x1 to x2 at height y, bulging up (dir < 0) or down. */
    void drawTie (juce::Graphics& g, float x1, float x2, float y, float dir, float space, juce::Colour c)
    {
        if (x2 - x1 < space * 0.5f)
            return;

        const float height = juce::jlimit (0.7f, 1.3f, 0.45f + (x2 - x1) / space * 0.04f) * space * 0.6f;
        const float thickness = 0.2f * space;
        const float mid = (x1 + x2) * 0.5f;

        juce::Path p;
        p.startNewSubPath (x1, y);
        p.quadraticTo (mid, y + dir * (height * 2.0f), x2, y);
        p.quadraticTo (mid, y + dir * (height * 2.0f - thickness * 2.0f), x1, y);
        p.closeSubPath();
        g.setColour (c);
        g.fillPath (p);
    }

    float getBarWidth (const Score& score, const Line& line, int barInLine)
    {
        return (float) (score.barWidths[(size_t) (line.firstBar + barInLine)] * line.scale);
    }

    /** Draws the events of one bar of one staff. */
    void drawBar (juce::Graphics& g, const Score& score, int staffIndex, int bar, const Line& line, int barInLine, float barX,
                  float staffTop, float space, const Ink& ink, std::vector<Hit>* hits, const LitFunction& lit)
    {
        const auto& staff = score.staves[(size_t) staffIndex];
        const auto& b = staff.bars[(size_t) bar];
        const float scale = (float) line.scale;
        const float barWidth = getBarWidth (score, line, barInLine);
        const float barEnd = barX + barWidth * space;

        std::vector<Geometry> geos;
        std::vector<float> xs;

        for (auto& e : b.events)
        {
            const float x = barX + (float) e.x * scale * space;
            xs.push_back (x);
            geos.push_back (e.isRest ? Geometry() : getGeometry (e, x, staffTop, space));
        }

        // Beamed groups share one beam height: the stems all reach the extreme tip.
        for (auto& beam : b.beams)
        {
            if (beam.events.empty())
                continue;

            const bool up = b.events[(size_t) beam.events.front()].stemUp;
            float tip = geos[(size_t) beam.events.front()].tip;

            for (int i : beam.events)
                tip = up ? juce::jmin (tip, geos[(size_t) i].tip) : juce::jmax (tip, geos[(size_t) i].tip);

            for (int i : beam.events)
                geos[(size_t) i].tip = tip;
        }

        for (size_t i = 0; i < b.events.size(); ++i)
        {
            const auto& e = b.events[i];
            const auto& geo = geos[i];
            const bool lit1 = lit && lit (staffIndex, bar, (int) i);
            const auto colour = lit1 ? ink.accent : ink.ink;

            if (e.isRest)
            {
                const bool wholeBar = e.length == unitsPerBar;
                const float w = space * 1.3f;
                const float x = wholeBar ? barX + (barWidth * space - w) * 0.5f : xs[i];
                const float baseline = e.value == Value::whole ? staffTop + space : staffTop + 2.0f * space;
                drawGlyph (g, getRestGlyph (e.value), x, baseline, space, colour);

                if (hits != nullptr)
                    hits->push_back ({ HitKind::rest, staffIndex, bar, (int) i, juce::Rectangle<float> (x, staffTop, w, 4.0f * space), {} });

                continue;
            }

            for (size_t p = 0; p < e.pitches.size(); ++p)
            {
                const auto& pitch = e.pitches[p];
                const float hx = geo.headLeft[p], hy = geo.headY[p];
                const float w = getHeadWidth (e.value) * space;

                for (int l = 10; l <= pitch.step; l += 2)
                    fillRect (g, geo.left - 0.4f * space, getY (staffTop, l, space) - ledgerThickness * space * 0.5f,
                              geo.right - geo.left + 0.8f * space, ledgerThickness * space, ink.ink);

                for (int l = -2; l >= pitch.step; l -= 2)
                    fillRect (g, geo.left - 0.4f * space, getY (staffTop, l, space) - ledgerThickness * space * 0.5f,
                              geo.right - geo.left + 0.8f * space, ledgerThickness * space, ink.ink);

                drawGlyph (g, e.value == Value::whole ? noteheadWhole : e.value == Value::half ? noteheadHalf : noteheadBlack,
                           hx, hy, space, colour);

                if (pitch.showAccidental)
                    drawGlyph (g, getAccidentalGlyph (pitch.alter), geo.left - 1.15f * space, hy, space, colour);

                if (e.dotted)
                    drawGlyph (g, augmentationDot, geo.right + 0.35f * space, (pitch.step % 2 == 0) ? hy - 0.5f * space : hy, space, colour);

                // Ties curve away from the stem, to the next head or to the bar's end; a continuation starts at the bar's start.
                if (e.tiedToNext)
                {
                    float x2 = barEnd - 0.4f * space;

                    if (i + 1 < b.events.size())
                        x2 = xs[i + 1] - 0.2f * space;

                    const float dir = e.stemUp ? 1.0f : -1.0f;
                    drawTie (g, hx + w + 0.15f * space, x2, hy + dir * 0.5f * space, dir, space, colour);
                }

                if (e.tiedFromPrevious && i == 0)
                {
                    const float dir = e.stemUp ? 1.0f : -1.0f;
                    drawTie (g, barX + 0.2f * space, hx - 0.2f * space, hy + dir * 0.5f * space, dir, space, colour);
                }
            }

            if (geo.hasStem)
            {
                const float y0 = e.stemUp ? geo.tip : geo.stemStart, y1 = e.stemUp ? geo.stemStart : geo.tip;
                fillRect (g, geo.stemX - stemThickness * space * 0.5f, y0, stemThickness * space, y1 - y0, colour);

                if (e.beamGroup < 0 && getBeamLevels (e.value) > 0)
                    drawGlyph (g, getFlagGlyph (e.value, e.stemUp), geo.stemX - stemThickness * space * 0.5f, geo.tip, space, colour);
            }

            if (hits != nullptr)
            {
                auto box = juce::Rectangle<float> (geo.left, geo.headY.back(), geo.right - geo.left, geo.headY.front() - geo.headY.back())
                               .expanded (0.2f * space, 0.6f * space);
                hits->push_back ({ HitKind::note, staffIndex, bar, (int) i, box, {} });
            }
        }

        // Beams: one, two or three levels, with a stub for a lone short note in a group.
        for (auto& beam : b.beams)
        {
            if (beam.events.size() < 2)
                continue;

            const bool up = b.events[(size_t) beam.events.front()].stemUp;
            const float tip = geos[(size_t) beam.events.front()].tip;
            const int count = (int) beam.events.size();

            for (int level = 1; level <= 3; ++level)
            {
                const float y = up ? tip + (float) (level - 1) * (beamThickness + beamGap) * space
                                   : tip - (float) (level - 1) * (beamThickness + beamGap) * space - beamThickness * space;

                int i = 0;

                while (i < count)
                {
                    if (getBeamLevels (b.events[(size_t) beam.events[(size_t) i]].value) < level)
                    {
                        ++i;
                        continue;
                    }

                    int j = i;

                    while (j + 1 < count && getBeamLevels (b.events[(size_t) beam.events[(size_t) j + 1]].value) >= level)
                        ++j;

                    const float xi = geos[(size_t) beam.events[(size_t) i]].stemX, xj = geos[(size_t) beam.events[(size_t) j]].stemX;
                    const float half = stemThickness * space * 0.5f;

                    if (j > i)
                        fillRect (g, xi - half, y, xj - xi + stemThickness * space, beamThickness * space, ink.ink);
                    else if (level > 1)
                    {
                        const float stub = 1.2f * space;
                        // The stub points to the neighbour inside the group.
                        if (i > 0)
                            fillRect (g, xi - stub, y, stub + half, beamThickness * space, ink.ink);
                        else
                            fillRect (g, xi - half, y, stub + half, beamThickness * space, ink.ink);
                    }

                    i = j + 1;
                }
            }
        }

        // Triplet brackets and their 3.
        for (auto& tr : b.triplets)
        {
            if (tr.first < 0 || tr.last >= (int) b.events.size() || tr.first > tr.last)
                continue;

            const auto& first = b.events[(size_t) tr.first];
            const bool up = first.stemUp;
            const bool beamed = first.beamGroup >= 0 && first.beamGroup == b.events[(size_t) tr.last].beamGroup;
            const float x0 = geos[(size_t) tr.first].left, x1 = geos[(size_t) tr.last].right;
            float y = geos[(size_t) tr.first].tip;

            for (int i = tr.first; i <= tr.last; ++i)
            {
                const auto& ev = b.events[(size_t) i];

                if (! ev.isRest)
                    y = up ? juce::jmin (y, geos[(size_t) i].tip) : juce::jmax (y, geos[(size_t) i].tip);
            }

            const float dir = up ? -1.0f : 1.0f;
            y += dir * (beamed ? 0.9f : 0.7f) * space;
            const float mid = (x0 + x1) * 0.5f;
            const auto box = juce::Rectangle<float> (x0, y - 0.8f * space, x1 - x0, 1.6f * space);

            if (! beamed)
            {
                const float gap = 0.9f * space;
                fillRect (g, x0, y, mid - gap - x0, 0.1f * space, ink.ink);
                fillRect (g, mid + gap, y, x1 - mid - gap, 0.1f * space, ink.ink);
                fillRect (g, x0, y, 0.1f * space, -dir * 0.5f * space, ink.ink);
                fillRect (g, x1 - 0.1f * space, y, 0.1f * space, -dir * 0.5f * space, ink.ink);
            }

            drawGlyph (g, tuplet0 + 3, mid - 0.35f * space, y + 0.5f * space, space, ink.ink);

            if (hits != nullptr)
                hits->push_back ({ HitKind::tripletBracket, staffIndex, bar, tr.first, box, {} });
        }
    }

    void drawBrace (juce::Graphics& g, float x, float top, float bottom, float space, juce::Colour c)
    {
        // A curly brace opening to the right: its outer edge on the left, an inner edge shifted right so it is thick
        // in the belly and thin at the ends and the point.
        const float w = 1.3f * space, mid = (top + bottom) * 0.5f, h = bottom - top, s = 0.34f * w;
        juce::Path p;
        p.startNewSubPath (x, top);
        p.cubicTo (x - 0.55f * w, top + 0.02f * h, x - 0.5f * w, top + 0.12f * h, x - 0.5f * w, top + 0.25f * h);
        p.cubicTo (x - 0.5f * w, mid - 0.12f * h, x - 0.55f * w, mid - 0.03f * h, x - w, mid);
        p.cubicTo (x - 0.55f * w, mid + 0.03f * h, x - 0.5f * w, mid + 0.12f * h, x - 0.5f * w, bottom - 0.25f * h);
        p.cubicTo (x - 0.5f * w, bottom - 0.12f * h, x - 0.55f * w, bottom - 0.02f * h, x, bottom);
        p.cubicTo (x - 0.25f * w, bottom - 0.02f * h, x - 0.5f * w + s, bottom - 0.12f * h, x - 0.5f * w + s, bottom - 0.25f * h);
        p.cubicTo (x - 0.5f * w + s, mid + 0.12f * h, x - 0.55f * w + s * 0.8f, mid + 0.03f * h, x - w + s * 0.5f, mid);
        p.cubicTo (x - 0.55f * w + s * 0.8f, mid - 0.03f * h, x - 0.5f * w + s, mid - 0.12f * h, x - 0.5f * w + s, mid - 0.25f * h);
        p.cubicTo (x - 0.5f * w + s, top + 0.12f * h, x - 0.25f * w, top + 0.02f * h, x, top);
        p.closeSubPath();
        g.setColour (c);
        g.fillPath (p);
    }

    /** "Bb" as "B♭", "F#m7" as "F♯m7": the root letter stays, a # or b after it becomes a sign. */
    juce::String prettifyChord (const juce::String& name)
    {
        return name.substring (0, 1) + name.substring (1).replace ("#", juce::String::fromUTF8 ("\xe2\x99\xaf"))
                                                          .replace ("b", juce::String::fromUTF8 ("\xe2\x99\xad"));
    }

    float getTextSize (float space) { return juce::jlimit (11.0f, 22.0f, 1.7f * space); }
} // namespace

float getTopMargin (float space) { return 9.0f * space; }
float getBottomMargin (float space) { return 4.0f * space; }

float getLineTopMargin (const Score& score, const Line& line, float space)
{
    const auto inLine = [&line] (int bar) { return bar >= line.firstBar && bar < line.firstBar + line.numBars; };

    for (auto& m : score.sectionMarks)
        if (inLine (m.bar))
            return 9.4f * space;

    for (auto& t : score.tempoMarks)
        if (inLine (t.bar))
            return 9.4f * space;

    for (auto& c : score.chordSymbols)
        if (inLine (c.bar))
            return 6.6f * space;

    return 4.6f * space;
}

float getStaffOffset (int index, float space) { return (float) index * (4.0f + staffGap) * space; }
float getStepY (float staffTop, int step, float space) { return getY (staffTop, step, space); }

float getStaffBlockHeight (const Score& score, float space)
{
    return getStaffOffset ((int) score.staves.size() - 1, space) + 4.0f * space;
}

void drawClef (juce::Graphics& g, Clef clef, int writtenShift, float x, float staffTop, float space, juce::Colour c)
{
    if (clef == Clef::treble)
        drawGlyph (g, writtenShift != 0 ? gClef8vb : gClef, x, staffTop + 3.0f * space, space, c);
    else
        drawGlyph (g, writtenShift != 0 ? fClef8vb : fClef, x, staffTop + 1.0f * space, space, c);
}

void drawHeader (juce::Graphics& g, const Score& score, bool timeSignature, float x, float yTop, float space,
                 const Ink& ink, std::vector<Hit>* hits)
{
    for (size_t s = 0; s < score.staves.size(); ++s)
    {
        const auto& staff = score.staves[s];
        const float top = yTop + getStaffOffset ((int) s, space);

        drawClef (g, staff.clef, staff.writtenShift, x + 0.4f * space, top, space, ink.ink);

        if (hits != nullptr)
            hits->push_back ({ HitKind::clef, (int) s, 0, -1, juce::Rectangle<float> (x + 0.3f * space, top - 1.0f * space, 2.8f * space, 6.0f * space), {} });

        const auto marks = getKeySignature (score.keyPosition, staff.clef);
        float kx = x + 3.5f * space;

        for (auto& m : marks)
        {
            drawGlyph (g, m.alter > 0 ? accSharp : accFlat, kx, getY (top, m.step, space), space, ink.ink);
            kx += space;
        }

        if (! marks.empty() && hits != nullptr)
            hits->push_back ({ HitKind::keySignature, (int) s, 0, -1,
                               juce::Rectangle<float> (x + 3.4f * space, top - 1.0f * space, (float) marks.size() * space + 0.4f * space, 6.0f * space), {} });

        if (timeSignature)
        {
            const float tx = x + (float) (getHeaderWidth (score, true) - 3.0 + 0.5) * space;
            drawGlyph (g, timeSig0 + 4, tx, top + 2.0f * space, space, ink.ink);
            drawGlyph (g, timeSig0 + 4, tx, top + 4.0f * space, space, ink.ink);

            if (hits != nullptr)
                hits->push_back ({ HitKind::timeSignature, (int) s, 0, -1, juce::Rectangle<float> (tx, top, 2.0f * space, 4.0f * space), {} });
        }
    }
}

void drawSystem (juce::Graphics& g, const Score& score, const Line& line, float x, float yTop, float space,
                 const Ink& ink, const Options& options, std::vector<Hit>* hits, const LitFunction& lit)
{
    const float xEnd = x + (float) line.width * space;
    const int first = options.firstBar < 0 ? 0 : juce::jmax (0, options.firstBar - line.firstBar);
    const int last = options.lastBar < 0 ? line.numBars - 1 : juce::jmin (line.numBars - 1, options.lastBar - line.firstBar);
    const float blockBottom = yTop + getStaffBlockHeight (score, space);

    for (size_t s = 0; s < score.staves.size(); ++s)
        drawStaffLines (g, x, xEnd, yTop + getStaffOffset ((int) s, space), space, ink.ink);

    if (score.isGrand())
    {
        fillRect (g, x, yTop, barLineThickness * space, blockBottom - yTop, ink.ink);
        drawBrace (g, x - 0.35f * space, yTop, blockBottom, space, ink.ink);
    }

    if (options.header)
        drawHeader (g, score, line.hasTimeSignature, x, yTop, space, ink, hits);

    const float textSize = getTextSize (space);

    for (int bi = first; bi <= last; ++bi)
    {
        const int bar = line.firstBar + bi;
        const float barX = x + (float) line.barOrigins[(size_t) bi] * space;
        const float barRight = barX + getBarWidth (score, line, bi) * space;

        for (size_t s = 0; s < score.staves.size(); ++s)
            drawBar (g, score, (int) s, bar, line, bi, barX, yTop + getStaffOffset ((int) s, space), space, ink, hits, lit);

        // The bar line, through both staves of a grand staff.
        const bool isFinal = bar == score.numBars - 1 && options.finalBarLine;
        fillRect (g, barRight - barLineThickness * space * (isFinal ? 2.4f : 1.0f), yTop, barLineThickness * space, blockBottom - yTop, ink.ink);

        if (isFinal)
            fillRect (g, barRight - 0.5f * space, yTop, 0.5f * space, blockBottom - yTop, ink.ink);

        if (options.barNumbers)
        {
            g.setColour (ink.dim);
            g.setFont (juce::FontOptions (textSize * 0.8f));
            g.drawText (juce::String (bar + 1), juce::Rectangle<float> (barX + 0.1f * space, yTop - 3.9f * space, 4.0f * space, 1.5f * space),
                        juce::Justification::centredLeft, false);
        }

        if (options.chordSymbols)
            for (auto& c : score.chordSymbols)
                if (c.bar == bar)
                {
                    const auto name = prettifyChord (c.name);
                    const auto box = juce::Rectangle<float> (barX + 0.2f * space, yTop - 6.0f * space, 8.0f * space, 2.0f * space);
                    g.setColour (ink.ink);
                    g.setFont (juce::FontOptions (textSize, juce::Font::bold));
                    g.drawText (name, box, juce::Justification::centredLeft, false);

                    if (hits != nullptr)
                    {
                        juce::GlyphArrangement ga;
                        ga.addLineOfText (juce::FontOptions (textSize, juce::Font::bold), name, 0.0f, 0.0f);
                        hits->push_back ({ HitKind::chordSymbol, 0, bar, -1,
                                           juce::Rectangle<float> (box.getX(), box.getY(), ga.getBoundingBox (0, -1, true).getWidth() + 2.0f, box.getHeight()),
                                           name });
                    }
                }

        if (options.sectionsAndTempo)
        {
            float cursor = barX + 0.2f * space;
            const float rowY = yTop - 8.8f * space, rowH = 2.2f * space;

            for (auto& m : score.sectionMarks)
                if (m.bar == bar)
                {
                    juce::GlyphArrangement ga;
                    ga.addLineOfText (juce::FontOptions (textSize * 0.9f, juce::Font::bold), m.name, 0.0f, 0.0f);
                    const float w = ga.getBoundingBox (0, -1, true).getWidth() + 0.9f * space;
                    g.setColour (ink.ink);
                    g.drawRect (cursor, rowY, w, rowH, 1.0f);
                    g.setFont (juce::FontOptions (textSize * 0.9f, juce::Font::bold));
                    g.drawText (m.name, juce::Rectangle<float> (cursor, rowY, w, rowH), juce::Justification::centred, false);
                    cursor += w + 0.6f * space;
                }

            for (auto& t : score.tempoMarks)
                if (t.bar == bar)
                {
                    drawGlyph (g, metNoteQuarterUp, cursor, rowY + rowH * 0.8f, space, ink.ink, 0.62f);
                    g.setColour (ink.ink);
                    g.setFont (juce::FontOptions (textSize * 0.9f));
                    g.drawText (juce::String (" = ") + juce::String (t.bpm),
                                juce::Rectangle<float> (cursor + 0.9f * space, rowY, 7.0f * space, rowH), juce::Justification::centredLeft, false);
                    cursor += 6.5f * space;
                }
        }
    }
}

juce::String describeEvent (const Event& e)
{
    static const char* names[] = { "whole", "half", "quarter", "eighth", "sixteenth", "thirty-second" };
    const auto name = juce::String (names[(int) e.value]);

    if (e.triplet)
        return name + "-note triplet";

    return (e.dotted ? "dotted " : "") + name + (e.isRest ? " rest" : " note");
}
} // namespace daw::scorepaint
