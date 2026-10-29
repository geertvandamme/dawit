#include "EqGuide.h"

namespace daw::eqtext
{
namespace
{
    constexpr double drawRate = 48000.0;
    const juce::Colour colour { theme::accent };
    const juce::Colour cutColour { 0xffff7a6b }, boostColour { 0xff5fd38d };

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

    juce::String formatDb (double db, int decimals = 1)
    {
        return (db > 0.05 ? "+" : "") + juce::String (db, decimals) + " dB";
    }

    /** Frequencies from 20 Hz to 20 kHz left to right, evenly by octaves, and dB up. */
    struct Plot
    {
        juce::Rectangle<float> area;
        double lowDb = -18.0, highDb = 18.0;

        float x (double hz) const
        {
            const double t = std::log (juce::jlimit (20.0, 20000.0, hz) / 20.0) / std::log (1000.0);
            return area.getX() + area.getWidth() * (float) t;
        }

        double hzAt (float px) const { return 20.0 * std::pow (1000.0, (px - area.getX()) / juce::jmax (1.0f, area.getWidth())); }
        float y (double db) const { return juce::jmap ((float) juce::jlimit (lowDb, highDb, db), (float) lowDb, (float) highDb, area.getBottom(), area.getY()); }
    };

    /** The grid: frequencies along the bottom, dB on the left (none with a `dbStep` of 0); leaves room for the labels. */
    Plot drawGrid (juce::Graphics& g, juce::Rectangle<float> r, double lowDb, double highDb, int dbStep, bool labels = true)
    {
        Plot p { r.withTrimmedLeft (labels ? 34.0f : 0.0f).withTrimmedBottom (labels ? 16.0f : 0.0f), lowDb, highDb };
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (p.area);
        g.setFont (juce::FontOptions (10.5f));

        for (double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        {
            g.setColour (theme::grid);
            g.fillRect (p.x (hz), p.area.getY(), 1.0f, p.area.getHeight());

            if (labels)
            {
                g.setColour (theme::textDim);
                g.drawText (hz >= 1000.0 ? juce::String (hz / 1000.0) + "k" : juce::String (hz),
                            juce::Rectangle<float> (p.x (hz) - 20.0f, p.area.getBottom() + 2.0f, 40.0f, 13.0f), juce::Justification::centred);
            }
        }

        for (int db = (int) lowDb; dbStep > 0 && db <= (int) highDb; db += dbStep)
        {
            g.setColour (db == 0 ? theme::gridBar : theme::grid);
            g.fillRect (p.area.getX(), p.y (db), p.area.getWidth(), 1.0f);

            if (labels)
            {
                g.setColour (theme::textDim);
                g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (r.getX(), p.y (db) - 7.0f, 30.0f, 14.0f),
                            juce::Justification::centredRight);
            }
        }

        return p;
    }

    /** The curve, filled towards 0 dB (or the bottom when 0 dB is off the plot). */
    void drawCurve (juce::Graphics& g, const Plot& p, const std::function<double (double)>& dbAt, juce::Colour c, float thickness = 2.0f,
                    bool fill = false)
    {
        juce::Path path;

        for (float px = p.area.getX(); px <= p.area.getRight(); px += 1.0f)
        {
            const float py = p.y (dbAt (p.hzAt (px)));

            if (px <= p.area.getX())
                path.startNewSubPath (px, py);
            else
                path.lineTo (px, py);
        }

        if (fill)
        {
            const float base = p.highDb > 0.0 && p.lowDb < 0.0 ? p.y (0.0) : p.area.getBottom();
            juce::Path area (path);
            area.lineTo (p.area.getRight(), base);
            area.lineTo (p.area.getX(), base);
            area.closeSubPath();
            g.setColour (c.withAlpha (0.18f));
            g.fillPath (area);
        }

        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (thickness));
    }

    double getTotalDb (const BuiltInEq& eqProcessor, double hz)
    {
        double db = 0.0;

        for (int i = 0; i < eq::numBands; ++i)
            db += eq::getResponseDb (eqProcessor.getBand (i), hz, drawRate);

        return db;
    }

    /** The ranges people talk about, as a strip under a plot. */
    void drawRegions (juce::Graphics& g, const Plot& p, juce::Rectangle<float> strip, bool detailed)
    {
        const auto& regions = eq::getRegions();

        for (size_t i = 0; i < regions.size(); ++i)
        {
            const auto& region = regions[i];
            auto box = juce::Rectangle<float>::leftTopRightBottom (p.x (region.from), strip.getY(), p.x (region.to), strip.getBottom()).reduced (0.5f, 0.0f);
            g.setColour (i % 2 == 0 ? theme::panelLight : theme::panel.brighter (0.06f));
            g.fillRect (box);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            auto inner = box.reduced (4.0f, 3.0f);
            g.drawFittedText (region.name, inner.removeFromTop (16.0f).toNearestInt(), juce::Justification::centred, 1, 0.7f);

            if (detailed)
            {
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (11.0f));
                g.drawFittedText (formatFrequency (region.from) + " - " + formatFrequency (region.to), inner.removeFromTop (14.0f).toNearestInt(),
                                  juce::Justification::centred, 1, 0.7f);
                g.drawFittedText (region.sound, inner.toNearestInt(), juce::Justification::centredTop, 4, 0.8f);
            }
        }
    }

    eq::Band makeBand (eq::BandType type, float hz, float gainDb, float q, int slope = 12)
    {
        return { true, type, hz, gainDb, q, slope };
    }

    /** A soft hump of an instrument's energy on the frequency axis, for the masking picture: highest at `hz`, falling
        `width` dB per octave squared away from it. */
    double getHumpDb (double hz, double centre, double peakDb, double width)
    {
        const double octaves = std::log2 (hz / centre);
        return peakDb - width * octaves * octaves;
    }
} // namespace

std::vector<fxui::Guide::Page> makeGuide (BuiltInEq& eqProcessor)
{
    BuiltInEq* processor = &eqProcessor;
    std::vector<fxui::Guide::Page> pages;

    // Numbers for the text, from the EQ's own filters.
    const auto cut12 = makeBand (eq::BandType::lowCut, 100.0f, 0.0f, eq::flatQ, 12), cut24 = makeBand (eq::BandType::lowCut, 100.0f, 0.0f, eq::flatQ, 24);
    const auto at = [] (const eq::Band& b, double hz) { return eq::getResponseDb (b, hz, drawRate); };
    const double bump = at (makeBand (eq::BandType::lowCut, 100.0f, 0.0f, 2.0f), 100.0);

    pages.push_back ({ "What an EQ does", "Turning frequencies up and down",
        "An EQ (equaliser) turns the level up or down in one range of frequencies and leaves the rest alone. The bass and treble "
        "knobs on a stereo are a simple EQ with two bands; DawIt EQ has six, and each can be placed anywhere from 20 Hz to 20 kHz.\n\n"
        "Every sound is a mix of frequencies (see What is sound?): a low hum of the body and the note, and many higher overtones "
        "that give it its character, its attack and its air. An EQ changes that balance. Turning up 3 kHz on a voice makes the "
        "words clearer; turning down 300 Hz takes away a boxy, muffled sound; cutting everything under 100 Hz on a guitar leaves "
        "the low end to the bass.\n\n"
        "The name comes from telephone lines in the early 20th century. A long cable lost more of the high frequencies than the "
        "low ones, so engineers added circuits that made the response equal again: they equalised it. Studios took the idea over "
        "for shaping sound. In 1972 George Massenburg described the parametric EQ, where each band's frequency, gain and width "
        "can all be set: the kind DawIt EQ is.\n\n"
        "Gains are in decibels: +6 dB is about twice the amplitude, -6 dB half, and 1 dB is about the smallest change most "
        "people notice on music. Most good EQ moves are small: 2 to 4 dB.\n\n"
        "The picture is the curve of DawIt EQ's window right now: above the 0 dB line the EQ boosts, below it it cuts. "
        "Under it are the ranges engineers name, from sub-bass to air.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The curve in DawIt EQ's window", "boost above the line, cut below");
            auto strip = r.removeFromBottom (26.0f);
            r.removeFromBottom (4.0f);
            const auto p = drawGrid (g, r, -18.0, 18.0, 6);
            drawCurve (g, p, [processor] (double hz) { return getTotalDb (*processor, hz); }, colour, 2.4f, true);
            drawRegions (g, p, strip.withX (p.area.getX()).withWidth (p.area.getWidth()), false);
        } });

    pages.push_back ({ "The frequency ranges", "Sub-bass, bass, mids, presence, air",
        "We hear from about 20 Hz to 20 kHz, ten octaves (many adults only up to 15 or 17 kHz). Engineers split that into ranges "
        "with names, so they can talk about them:\n\n"
        "- Sub-bass, 20-60 Hz: felt more than heard, the rumble under a kick or a synth bass. Phones and laptops don't play it.\n"
        "- Bass, 60-250 Hz: the weight of kick and bass, and the low notes of piano and guitar. Too much is boomy.\n"
        "- Low mids, 250-500 Hz: body and warmth. Most instruments have energy here, so it piles up: too much is muddy or boxy.\n"
        "- Mids, 500 Hz-2 kHz: the core of most instruments and voices. Too much is honky or nasal, too little hollow.\n"
        "- Presence, 2-6 kHz: clarity and attack, the consonants of words. The ear is most sensitive here, so small changes "
        "are easy to hear, and too much is harsh.\n"
        "- Brilliance, 6-10 kHz: the s and t sounds, cymbals, string and breath noise.\n"
        "- Air, 10-20 kHz: openness and sparkle.\n\n"
        "A note's pitch is its lowest frequency, the fundamental: the A above middle C is 440 Hz. But it also has overtones at "
        "880, 1320, 1760 Hz and on. So a voice sings notes between about 100 Hz and 1 kHz, yet its clarity is at 2-5 kHz and "
        "its breath above 10 kHz. That's why an EQ can change a sound's character without changing its notes. The piano's "
        "lowest note, A0, is 27.5 Hz, and its highest, C8, 4186 Hz: everything above that is overtones.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The ranges engineers name", "20 Hz to 20 kHz, evenly by octaves");
            const auto plot = drawGrid (g, r, -1.0, 1.0, 0);
            auto rows = plot.area.reduced (0.0f, 6.0f);
            const auto& regions = eq::getRegions();
            auto strip = rows.removeFromTop (44.0f);

            for (size_t i = 0; i < regions.size(); ++i)
            {
                const auto& region = regions[i];
                auto box = juce::Rectangle<float>::leftTopRightBottom (plot.x (region.from), strip.getY(), plot.x (region.to), strip.getBottom()).reduced (1.0f, 0.0f);
                g.setColour (i % 2 == 0 ? colour.withAlpha (0.28f) : colour.withAlpha (0.16f));
                g.fillRect (box);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
                g.drawFittedText (region.name, box.removeFromTop (24.0f).toNearestInt(), juce::Justification::centredBottom, 1, 0.6f);
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (10.5f));
                g.drawFittedText (formatFrequency (region.from) + "-" + formatFrequency (region.to), box.toNearestInt(), juce::Justification::centredTop, 1, 0.55f);
            }

            const auto row = [&rows, &plot, &g] (double from, double to, juce::Colour c, const juce::String& label)
            {
                rows.removeFromTop (16.0f);
                auto line = rows.removeFromTop (30.0f);
                g.setColour (c);
                g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (plot.x (from), line.getY(), plot.x (to), line.getY() + 8.0f), 3.0f);
                g.setFont (juce::FontOptions (11.5f));
                g.drawText (label, juce::Rectangle<float> (plot.x (from), line.getY() + 10.0f, 420.0f, 16.0f), juce::Justification::centredLeft);
            };

            row (20.0, 20000.0, theme::text.withAlpha (0.55f), "what we hear: 20 Hz to 20 kHz (many adults up to 15-17 kHz)");
            row (27.5, 4186.0, theme::text.withAlpha (0.8f), "the piano's notes: A0 27.5 Hz to C8 4186 Hz");
            row (80.0, 1000.0, juce::Colour (0xffc792ea), "a singer's notes: about 80 Hz to 1 kHz");

            // A4 and its overtones: one note, many frequencies.
            rows.removeFromTop (16.0f);
            auto overtones = rows.removeFromTop (34.0f);

            for (int n = 1; n <= 12; ++n)
            {
                const float x = plot.x (440.0 * n);
                g.setColour (n == 1 ? colour : colour.withAlpha (0.55f));
                g.fillRect (x - 1.0f, overtones.getY(), 2.0f, 16.0f - (float) n * 0.8f);
            }

            g.setColour (colour);
            g.setFont (juce::FontOptions (11.5f));
            g.drawText ("one note, A4: 440 Hz, and its overtones at 880, 1320, 1760 Hz...",
                        juce::Rectangle<float> (plot.x (440.0) - 2.0f, overtones.getY() + 18.0f, 420.0f, 16.0f), juce::Justification::centredLeft);
        } });

    pages.push_back ({ "Bells, shelves and cuts", "The shapes a band can have",
        "Each band of an EQ has a shape, its type:\n\n"
        "- Bell (also called peak): raises or lowers a range around its frequency, like a hill or a valley. The most used type, "
        "for anything in the middle: more presence, less mud.\n"
        "- Low shelf and high shelf: raise or lower everything below (or above) the frequency by the same amount, like a step. "
        "The bass and treble knobs on a stereo are shelves.\n"
        "- Low cut (high-pass) and high cut (low-pass): remove everything below (or above) the frequency, more the further away "
        "it is. A low cut on most tracks except kick and bass is one of the most common moves in mixing: it clears rumble and "
        "makes room in the low end.\n"
        "- Notch: a very narrow cut that takes out one frequency, such as a 50 or 60 Hz mains hum or one ringing note of a drum.\n\n"
        "The names can confuse: a low cut lets the highs pass, so it's also called a high-pass filter.\n\n"
        "The picture shows each type, as DawIt EQ's filters draw them: the bell and shelves at +6 dB.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "The six band types", "from DawIt EQ's own filters");
            const std::pair<eq::Band, juce::String> shapes[] {
                { makeBand (eq::BandType::lowCut, 120.0f, 0.0f, eq::flatQ), "Low cut (high-pass), 120 Hz" },
                { makeBand (eq::BandType::lowShelf, 200.0f, 6.0f, eq::flatQ), "Low shelf, 200 Hz, +6 dB" },
                { makeBand (eq::BandType::bell, 1000.0f, 6.0f, 1.0f), "Bell, 1 kHz, +6 dB" },
                { makeBand (eq::BandType::notch, 1000.0f, 0.0f, 4.0f), "Notch, 1 kHz" },
                { makeBand (eq::BandType::highShelf, 5000.0f, 6.0f, eq::flatQ), "High shelf, 5 kHz, +6 dB" },
                { makeBand (eq::BandType::highCut, 8000.0f, 0.0f, eq::flatQ), "High cut (low-pass), 8 kHz" } };
            const float w = r.getWidth() / 3.0f, h = r.getHeight() / 2.0f;

            for (int i = 0; i < 6; ++i)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 3), r.getY() + h * (float) (i / 3), w, h).reduced (6.0f, 4.0f);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                g.drawText (shapes[i].second, cell.removeFromTop (16.0f), juce::Justification::centredLeft, true);
                const auto p = drawGrid (g, cell, -24.0, 12.0, 12, false);
                const auto band = shapes[i].first;
                drawCurve (g, p, [band] (double hz) { return eq::getResponseDb (band, hz, drawRate); }, getBandColour (i), 2.0f, true);
            }
        } });

    pages.push_back ({ "Q: how wide a band is", "Narrow or wide",
        "Q sets how wide a bell or notch is: a low Q is wide and gentle, a high Q narrow and sharp. The width is often given in "
        "octaves, measured where the boost has reached half its decibels:\n\n"
        "- Q " + juce::String (0.67, 2) + ": about " + juce::String (eq::getBandwidthOctaves (0.67), 1) + " octaves. Broad and musical, "
        "for changing the tone of a whole instrument.\n"
        "- Q 1.41: " + juce::String (eq::getBandwidthOctaves (1.41), 1) + " octave. A middle width, a good place to start.\n"
        "- Q 4.32: a third of an octave (" + juce::String (eq::getBandwidthOctaves (4.32), 2) + "). Narrow: for one problem "
        "frequency, such as a boxy ring.\n"
        "- Q 10 and up: a notch-like sliver, for a hum or one resonance.\n\n"
        "A rule of thumb: boost wide, cut narrow. A wide boost sounds natural, like a different instrument or microphone; a narrow "
        "boost sounds like a whistle. Problems such as a ringing drum or a room's boom are narrow, so a narrow cut removes them "
        "without taking the good around them.\n\n"
        "On a cut or shelf, Q changes the corner: 0.71 is the smoothest, without a bump. A higher Q gives a bump before the "
        "slope: at Q 2 a low cut adds " + formatDb (bump) + " right at its frequency. That bump is the resonance of a synth filter.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Bells at 1 kHz, +9 dB, three widths", "wide to narrow");
            const auto p = drawGrid (g, r, -3.0, 12.0, 3);
            const float qs[] { 0.67f, 1.41f, 4.32f };
            const juce::Colour colours[] { boostColour, colour, cutColour };

            for (int i = 0; i < 3; ++i)
            {
                const auto band = makeBand (eq::BandType::bell, 1000.0f, 9.0f, qs[i]);
                drawCurve (g, p, [band] (double hz) { return eq::getResponseDb (band, hz, drawRate); }, colours[i], 2.0f);
                g.setColour (colours[i]);
                g.setFont (juce::FontOptions (12.0f));
                g.drawText ("Q " + juce::String (qs[i], 2) + ": " + juce::String (eq::getBandwidthOctaves (qs[i]), 2) + " octaves",
                            juce::Rectangle<float> (p.area.getRight() - 190.0f, p.area.getY() + 6.0f + 18.0f * (float) i, 180.0f, 16.0f),
                            juce::Justification::centredRight);
            }
        } });

    pages.push_back ({ "Cuts and their slopes", "How steep a cut is",
        "A low or high cut doesn't stop at its frequency like a wall: it goes down a slope, measured in dB per octave. DawIt EQ's "
        "cuts have two slopes, 12 and 24 dB per octave.\n\n"
        "A low cut at 100 Hz with 12 dB per octave is " + formatDb (at (cut12, 100.0)) + " at 100 Hz (the corner, where it "
        "starts to work), " + formatDb (at (cut12, 50.0)) + " an octave lower at 50 Hz, and " + formatDb (at (cut12, 25.0))
        + " at 25 Hz. With 24 dB per octave it is " + formatDb (at (cut24, 50.0)) + " at 50 Hz and " + formatDb (at (cut24, 25.0))
        + " at 25 Hz: twice as steep.\n\n"
        "Gentle slopes sound natural and are good for shaping (taking some lows off a vocal); steep ones for removing what "
        "shouldn't be there at all, such as rumble under a guitar, without touching what's just above it.\n\n"
        "Where to put a low cut: just under the lowest note the instrument plays. A guitar's lowest note is 82 Hz (E2), so a "
        "low cut at 70-80 Hz takes nothing it needs; a voice can usually start at 80-120 Hz. Sweep it up while listening until "
        "the sound gets thin, then back off a little. On kick and bass the lows are the point, so they usually get none, or "
        "one at 20-30 Hz for the rumble no speaker plays.",
        [cut12, cut24] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "A low cut at 100 Hz", "12 and 24 dB per octave");
            const auto p = drawGrid (g, r, -48.0, 6.0, 6);
            drawCurve (g, p, [cut12] (double hz) { return eq::getResponseDb (cut12, hz, drawRate); }, colour, 2.0f);
            drawCurve (g, p, [cut24] (double hz) { return eq::getResponseDb (cut24, hz, drawRate); }, cutColour, 2.0f);
            g.setFont (juce::FontOptions (11.5f));

            for (double hz : { 25.0, 50.0, 100.0 })
            {
                for (const auto* b : { &cut12, &cut24 })
                {
                    const double db = eq::getResponseDb (*b, hz, drawRate);
                    g.setColour (b == &cut12 ? colour : cutColour);
                    g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ p.x (hz), p.y (db) }));
                    g.drawText (formatDb (db), juce::Rectangle<float> (p.x (hz) + 6.0f, p.y (db) - 7.0f, 70.0f, 14.0f), juce::Justification::centredLeft);
                }
            }

            g.setColour (colour);
            g.drawText ("12 dB per octave", juce::Rectangle<float> (p.area.getRight() - 150.0f, p.area.getBottom() - 40.0f, 140.0f, 14.0f), juce::Justification::centredRight);
            g.setColour (cutColour);
            g.drawText ("24 dB per octave", juce::Rectangle<float> (p.area.getRight() - 150.0f, p.area.getBottom() - 22.0f, 140.0f, 14.0f), juce::Justification::centredRight);
        } });

    pages.push_back ({ "Cut narrow, boost wide", "And cut before you boost",
        "Two habits most engineers share:\n\n"
        "Cut before you boost. Often a sound isn't missing something; something else is in the way. Taking out 3 dB of mud at "
        "300 Hz makes a voice sound clearer, just like adding 3 dB at 3 kHz would, but without making it louder or harsher. "
        "Cuts also leave headroom, while every boost makes the track louder and can push the next effect harder (see Gain "
        "staging).\n\n"
        "Cut narrow, boost wide. What sounds bad is usually narrow: the ring of a drum, a boxy resonance of a room, a whistle of a "
        "microphone. A narrow cut (Q 4-8) removes it and leaves the rest. What sounds good is usually wide: warmth, presence, air. "
        "A wide boost (Q 0.5-1) lifts a whole region and sounds natural.\n\n"
        "To find a bad frequency: boost a narrow bell by 10 dB, sweep it slowly while the track plays until the sound gets "
        "really ugly, and cut there instead, by 3-6 dB. DawIt EQ's Find a bad frequency button walks you through it.\n\n"
        "And listen in the mix, not only solo: a guitar that sounds thin on its own may be exactly right with the bass.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "A narrow cut where it sounds boxy, a wide boost for presence", "Q 6 and Q 0.7");
            const auto p = drawGrid (g, r, -12.0, 6.0, 3);
            const auto cut = makeBand (eq::BandType::bell, 350.0f, -8.0f, 6.0f), boost = makeBand (eq::BandType::bell, 3000.0f, 3.0f, 0.7f);
            drawCurve (g, p, [cut, boost] (double hz) { return eq::getResponseDb (cut, hz, drawRate) + eq::getResponseDb (boost, hz, drawRate); },
                       colour, 2.4f, true);
            g.setFont (juce::FontOptions (12.0f));
            g.setColour (cutColour);
            g.drawText ("-8 dB at 350 Hz, Q 6: the box gone", juce::Rectangle<float> (p.x (350.0) - 110.0f, p.y (-8.0) + 6.0f, 220.0f, 14.0f), juce::Justification::centred);
            g.setColour (boostColour);
            g.drawText ("+3 dB at 3 kHz, Q 0.7: clearer", juce::Rectangle<float> (p.x (3000.0) - 110.0f, p.y (3.0) - 22.0f, 220.0f, 14.0f), juce::Justification::centred);
        } });

    pages.push_back ({ "Where instruments sit", "Their notes, and their overtones",
        "Every instrument covers a part of the spectrum: its notes (the fundamentals) and, far above them, its overtones, attack "
        "and noises. Knowing where they sit tells you where to look.\n\n"
        "A kick drum has most of its energy below 100 Hz, but the click of the beater at 2-5 kHz is what makes it heard on a "
        "phone. A bass guitar plays from 41 Hz (its low E) and its growl is at 700 Hz-1.5 kHz. A voice sings between about 80 Hz "
        "and 1 kHz, with its clarity at 2-5 kHz and the s sounds at 5-8 kHz.\n\n"
        "The picture shows common instruments: the solid bar is their notes, the faint one how high their overtones reach. "
        "Where many bars overlap, at 200-500 Hz, mixes get crowded: that's where most mud comes from.\n\n"
        "DawIt EQ's Instruments button shows the same instruments over its graph, with the ranges worth knowing for each: where "
        "a problem usually hides (red) and what a boost brings out (green). Its starting points set the EQ up for each one, "
        "with why.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Common instruments: their notes, and how high their overtones reach", "typical values");
            const auto& instruments = eq::getInstruments();
            auto plotArea = r.withTrimmedLeft (110.0f);
            const auto p = drawGrid (g, plotArea, -1.0, 1.0, 0);
            const float rowHeight = p.area.getHeight() / (float) juce::jmax (1, (int) instruments.size());

            for (size_t i = 0; i < instruments.size(); ++i)
            {
                const auto& instrument = instruments[i];
                const auto row = juce::Rectangle<float> (r.getX(), p.area.getY() + rowHeight * (float) i, r.getWidth(), rowHeight);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (11.5f));
                g.drawText (instrument.name, row.withWidth (140.0f), juce::Justification::centredLeft, true);
                const float h = juce::jmin (12.0f, rowHeight - 4.0f), y = row.getCentreY() - h / 2.0f;
                g.setColour (colour.withAlpha (0.3f));
                g.fillRect (juce::Rectangle<float>::leftTopRightBottom (p.x (instrument.notesFrom), y, p.x (instrument.overtonesTo), y + h));
                g.setColour (colour);
                g.fillRect (juce::Rectangle<float>::leftTopRightBottom (p.x (instrument.notesFrom), y, juce::jmax (p.x (instrument.notesTo), p.x (instrument.notesFrom) + 3.0f), y + h));
            }
        } });

    pages.push_back ({ "Making room: masking", "When one sound hides another",
        "A loud sound hides a quieter one that's close to it in frequency: the ear can't separate them. This is called masking. "
        "It's why a mix of good-sounding tracks can sound muddy together, and why a guitar disappears when the keys come in.\n\n"
        "EQ is the main tool against it: give each sound its own place. If two instruments fight over the same range, cut a "
        "little of it in the one that needs it less, and they both come through. The kick and the bass are the classic case: "
        "both want 50-100 Hz. If the kick owns 50-60 Hz, give the bass 80-100 Hz, with a small cut where the other one is "
        "strongest. A vocal and the guitars fight around 2-4 kHz: a gentle cut in the guitars there lets the words through "
        "without turning the vocal up.\n\n"
        "Low cuts help too: every track that doesn't need the low end and has a low cut leaves more room for kick and bass.\n\n"
        "The picture shows a kick (orange) and a bass (blue) on top of each other, and the same bass with a cut of 5 dB at "
        "60 Hz: the kick comes through where the bass made room.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "A kick and a bass in the same range", "and a cut in the bass where the kick is strongest");
            const auto p = drawGrid (g, r, -36.0, 0.0, 6);
            const juce::Colour kickColour { 0xffffa94d }, bassColour { 0xff5cc8ff };
            const auto cut = makeBand (eq::BandType::bell, 60.0f, -5.0f, 1.4f);
            const auto kick = [] (double hz) { return getHumpDb (hz, 60.0, -4.0, 6.0); };
            const auto bass = [] (double hz) { return getHumpDb (hz, 90.0, -6.0, 3.0); };

            drawCurve (g, p, kick, kickColour, 2.0f, true);
            drawCurve (g, p, bass, bassColour.withAlpha (0.45f), 1.4f);
            drawCurve (g, p, [bass, cut] (double hz) { return bass (hz) + eq::getResponseDb (cut, hz, drawRate); }, bassColour, 2.2f);

            g.setFont (juce::FontOptions (12.0f));
            g.setColour (kickColour);
            g.drawText ("kick", juce::Rectangle<float> (p.x (60.0) - 40.0f, p.y (-4.0) - 18.0f, 80.0f, 14.0f), juce::Justification::centred);
            g.setColour (bassColour);
            g.drawText ("bass, with a cut at 60 Hz (faint: before)", juce::Rectangle<float> (p.x (900.0), p.y (-8.0), 260.0f, 14.0f), juce::Justification::centredLeft);
        } });

    juce::String problemList;

    for (auto& problem : eq::getProblems())
        problemList << "- " << problem.name.substring (0, 1).toUpperCase() << problem.name.substring (1) << " (" << formatFrequency (problem.from)
                 << " - " << formatFrequency (problem.to) << (problem.tooMuch ? ", too much" : ", too little") << "): " << problem.fix << ".\n";

    pages.push_back ({ "Common problems, in words", "Muddy, boxy, harsh, thin",
        "People describe sound in words, and most of those words point at a range. These are where to look first; every "
        "recording is different, so sweep around them and listen:\n\n" + problemList +
        "\nA problem in one track is often a problem of the mix: a muddy mix is usually several tracks each with a little too much "
        "at 200-500 Hz, so cut a little in each rather than a lot in one.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Where each problem usually is", "red: too much there; blue: too little");
            const auto p = drawGrid (g, r, -1.0, 1.0, 0);
            const auto& problems = eq::getProblems();
            const float rowHeight = p.area.getHeight() / (float) problems.size();

            for (size_t i = 0; i < problems.size(); ++i)
            {
                const auto& problem = problems[i];
                const auto c = problem.tooMuch ? cutColour : juce::Colour (0xff5cc8ff);
                auto bar = juce::Rectangle<float>::leftTopRightBottom (p.x (problem.from), p.area.getY() + rowHeight * (float) i + 4.0f,
                                                                        p.x (problem.to), p.area.getY() + rowHeight * (float) (i + 1) - 4.0f);
                g.setColour (c.withAlpha (0.35f));
                g.fillRoundedRectangle (bar, 4.0f);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                g.drawText (problem.name, bar.withX (bar.getRight() + 6.0f).withWidth (120.0f), juce::Justification::centredLeft);
            }
        } });

    pages.push_back ({ "Louder sounds better: match level", "Comparing honestly",
        "Play the same sound twice, the second time 1 dB louder, and most people say the louder one sounds better: fuller, "
        "clearer, closer. The ear hears more bass and treble when it's loud (the equal-loudness curves). So an EQ that boosts "
        "seems to improve things even when it doesn't, because it also makes the sound louder.\n\n"
        "To judge an EQ honestly, compare before and after at the same loudness. That's what Match level in DawIt EQ's window "
        "does: it compares the level going in with the level coming out over the last half second, and turns the output up or "
        "down so they match. Then switching a band, or the whole EQ, off and on compares only the tone.\n\n"
        "A good habit: make a change, switch it off and on a few times with match level on, and keep it only if it's really "
        "better. Often a cut you liked is just quieter, and a boost just louder.\n\n"
        "The picture shows the curve in the window, and, dashed, what reaches the next effect when match level is on.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const bool on = processor->isMatchingLevel();
            const double matchDb = processor->matchGainDb.load();
            drawCaption (g, r, "The curve, and with match level", on ? "Match level on: " + formatDb (matchDb) + " right now"
                                                                     : "Match level off: it would change the level by " + formatDb (matchDb));
            const auto p = drawGrid (g, r, -18.0, 18.0, 6);
            drawCurve (g, p, [processor] (double hz) { return getTotalDb (*processor, hz); }, colour, 2.4f, true);

            juce::Path dashed;
            juce::Path line;

            for (float px = p.area.getX(); px <= p.area.getRight(); px += 1.0f)
            {
                const float py = p.y (getTotalDb (*processor, p.hzAt (px)) + matchDb);

                if (px <= p.area.getX())
                    line.startNewSubPath (px, py);
                else
                    line.lineTo (px, py);
            }

            const float dashes[] { 6.0f, 4.0f };
            juce::PathStrokeType (1.6f).createDashedStroke (dashed, line, dashes, 2);
            g.setColour (on ? boostColour : theme::textDim);
            g.fillPath (dashed);
        } });

    pages.push_back ({ "How DawIt EQ works", "Six filters in a row",
        "Each of DawIt EQ's six bands is a small digital filter, a biquad: it makes every output sample from the last two "
        "input samples and the last two output samples, each times a number. Those five numbers set the shape; they come from "
        "the formulas of the Audio EQ Cookbook by Robert Bristow-Johnson, which most digital EQs use.\n\n"
        "A 24 dB per octave cut is two 12 dB filters in a row. The bands run one after the other, so their effects add up in "
        "decibels: the curve in the window is the sum of the six coloured curves in the picture.\n\n"
        "When you turn a knob, the band glides to the new setting over a few milliseconds instead of jumping, so there's no "
        "click. Solo plays only the range one band works on, so you can hear what it changes. And the spectrum in the window is "
        "measured before and after the EQ with a fast Fourier transform.\n\n"
        "Like most digital EQs, its filters change the timing of the frequencies slightly near each band (the phase), the same "
        "way an analog EQ does. That's part of how an EQ sounds, and inaudible at the small settings most mixes use.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "Each band of the window, and their sum", "the bands add up in dB");
            const auto p = drawGrid (g, r, -18.0, 18.0, 6);

            for (int i = 0; i < eq::numBands; ++i)
            {
                const auto band = processor->getBand (i);

                if (band.on)
                    drawCurve (g, p, [band] (double hz) { return eq::getResponseDb (band, hz, drawRate); }, getBandColour (i), 2.2f);
            }

            drawCurve (g, p, [processor] (double hz) { return getTotalDb (*processor, hz); }, juce::Colours::white.withAlpha (0.7f), 1.4f);
        } });

    return pages;
}
} // namespace daw::eqtext
