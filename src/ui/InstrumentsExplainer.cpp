#include "InstrumentsExplainer.h"
#include "engine/BuiltInSampler.h"
#include "engine/MidiGuide.h"

#include <cmath>

namespace daw
{
namespace gi = gen::instruments;

namespace
{
    juce::String utf8 (const char* s) { return juce::String::fromUTF8 (s); }

    /** The equal-tempered note nearest a frequency. */
    int getPitchOf (double hz) { return juce::jlimit (0, 127, juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0))); }

    // juce::String (double, 0) means default precision, so whole hertz are rounded explicitly.
    juce::String formatHz (double hz) { return (hz < 100.0 ? juce::String (hz, 1) : juce::String (juce::roundToInt (hz))) + " Hz"; }

    juce::String formatBeats (double b) { return juce::String (b, 2); }

    juce::String getDrumLabel (int pitch)
    {
        switch (pitch)
        {
            case 36: return "Kick (36)";
            case 38: return "Snare (38)";
            case 42: return "Closed hi-hat (42)";
            default: break;
        }

        return "Note " + juce::String (pitch);
    }

    /** When each line of an example starts, in beats: all together, except the typed and played lines, which sound one after the other. */
    std::vector<double> getLineOffsets (gi::Topic topic, const gi::Example& e)
    {
        std::vector<double> offsets (e.lines.size(), 0.0);

        if (topic == gi::Topic::realMidi)
        {
            double at = 0.0;

            for (size_t i = 0; i < e.lines.size(); ++i)
            {
                offsets[i] = at;
                double end = 0.0;

                for (auto& n : e.lines[i].notes)
                    end = juce::jmax (end, n.start + n.length);

                for (auto& p : e.lines[i].expression)
                    end = juce::jmax (end, p.beat);

                at += end + 1.0;
            }
        }

        return offsets;
    }

    /** A line of dots along a path: a standing wave of `humps` half-waves between two ends, drawn as the envelope both ways. */
    void drawEnvelope (juce::Graphics& g, juce::Rectangle<float> r, const std::function<float (float)>& shape, juce::Colour colour)
    {
        juce::Path up, down;
        const int steps = 64;

        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float) i / (float) steps;
            const float x = r.getX() + t * r.getWidth();
            const float d = shape (t) * r.getHeight() * 0.5f;

            if (i == 0)
            {
                up.startNewSubPath (x, r.getCentreY() - d);
                down.startNewSubPath (x, r.getCentreY() + d);
            }
            else
            {
                up.lineTo (x, r.getCentreY() - d);
                down.lineTo (x, r.getCentreY() + d);
            }
        }

        g.setColour (colour);
        g.strokePath (up, juce::PathStrokeType (1.8f));
        g.setColour (colour.withAlpha (0.45f));
        g.strokePath (down, juce::PathStrokeType (1.2f));
    }
} // namespace

//==============================================================================
namespace instrumentsui
{
gi::Hints getHints (Engine& engine, Track& track)
{
    gi::Hints h;
    h.trackName = track.name;
    const juce::ScopedLock lock (engine.getLock());

    for (auto& slot : track.plugins)
    {
        if (auto* sampler = dynamic_cast<BuiltInSampler*> (slot->instance.get()))
        {
            h.hasSampler = true;
            h.samplerBank = sampler->getBank();
            h.samplerProgram = sampler->getProgram() + 1;
            h.samplerPreset = sampler->getPresetName();
        }

        if (slot->isInstrument && h.dawItInstrument.isEmpty())
            h.dawItInstrument = slot->instance->getName();
    }

    if (track.usesBuiltInSynth())
        h.builtInSound = builtInSounds::getId (track.builtInSound.load());

    return h;
}

gi::Topic getTopicFor (Engine& engine, Track& track)
{
    const auto guess = gi::guessInstrument (getHints (engine, track));
    return guess.instrument.has_value() ? gi::getFamilyTopic (gi::getInfo (*guess.instrument).family) : gi::Topic::yours;
}

juce::Colour getFamilyColour (gi::Family f)
{
    switch (f)
    {
        case gi::Family::keys:      return juce::Colour (0xff4fa3ff);
        case gi::Family::guitar:    return juce::Colour (0xffe08a3c);
        case gi::Family::bass:      return juce::Colour (0xff9b7be0);
        case gi::Family::strings:   return juce::Colour (0xff5fd38d);
        case gi::Family::brass:     return juce::Colour (0xfff2c94c);
        case gi::Family::woodwinds: return juce::Colour (0xff4cc9c0);
        case gi::Family::voice:     return juce::Colour (0xffe77fb3);
        case gi::Family::synths:    return juce::Colour (0xff7fd0f0);
        case gi::Family::drums:     return juce::Colour (0xffb08a62);
    }

    return theme::accent;
}
} // namespace instrumentsui

//==============================================================================
/** The selected track's notes as the analysis and the picture need them. */
struct InstrumentsExplainer::Captured
{
    gi::TrackPart part;
    std::vector<double> lengthBeats;    // for each note of `part`
    gi::Hints hints;
    Track* track = nullptr;
};

//==============================================================================
/** The topic's picture. Everything is drawn from the example's data (`gi::getExample`), or from the selected track for *Your
    track*. Notes and the ends of range bars are hit areas: a click plays them and the tooltip says what's under the mouse. */
class InstrumentsExplainer::Picture final : public juce::Component,
                                            public juce::TooltipClient
{
public:
    explicit Picture (Engine& e) : engine (e) {}

    /** What *Your track* draws: the notes, the instrument they're checked against and a message for when there are none. */
    struct Yours
    {
        std::vector<gi::TrackNote> notes;
        std::vector<double> lengthBeats;
        std::optional<gi::Instrument> instrument;
        juce::String message;
    };

    void setExample (gi::Topic t, gi::Example e)
    {
        topic = t;
        example = std::move (e);
        offsets = getLineOffsets (topic, example);
        yours = {};
        playBeat = -1.0;
        repaint();
    }

    void setYours (Yours y)
    {
        topic = gi::Topic::yours;
        example = {};
        offsets.clear();
        yours = std::move (y);
        repaint();
    }

    /** The beat being played, or -1: the notes sounding then are lit. */
    void setPlayBeat (double beat)
    {
        if (! juce::approximatelyEqual (beat, playBeat))
        {
            playBeat = beat;
            repaint();
        }
    }

    /** The height the picture wants at this width, at most `maxHeight`. */
    int getPreferredHeight (int maxHeight) const
    {
        int h = 280;

        switch (topic)
        {
            case gi::Topic::makeSound:     h = 300; break;
            case gi::Topic::strings:       h = 310; break;
            case gi::Topic::tubes:         h = 280; break;
            case gi::Topic::drumsBars:     h = 250; break;
            case gi::Topic::keys:          h = 270; break;
            case gi::Topic::guitar:        h = 280; break;
            case gi::Topic::bass:          h = 280; break;
            case gi::Topic::bowed:         h = 170; break;
            case gi::Topic::brass:         h = 330; break;
            case gi::Topic::woodwinds:     h = 210; break;
            case gi::Topic::voice:         h = 210; break;
            case gi::Topic::synths:        h = 290; break;
            case gi::Topic::drumKit:       h = 270; break;
            case gi::Topic::orchestration: h = 560; break;
            case gi::Topic::realMidi:      h = 400; break;
            case gi::Topic::yours:         h = 280; break;
        }

        return juce::jmin (h, maxHeight);
    }

    juce::String getTooltip() override
    {
        const auto p = getMouseXYRelative().toFloat();

        for (auto it = hits.rbegin(); it != hits.rend(); ++it)
            if (it->area.contains (p))
                return it->tip;

        return {};
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto it = hits.rbegin(); it != hits.rend(); ++it)
            if (it->area.contains (e.position))
            {
                if (it->pitch >= 0)
                    engine.previewNote (it->pitch, it->velocity, 700);

                return;
            }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        hits.clear();
        const auto area = getLocalBounds().toFloat().reduced (14.0f, 10.0f);

        switch (topic)
        {
            case gi::Topic::makeSound:     paintTiles (g, area); break;
            case gi::Topic::strings:       paintStrings (g, area); break;
            case gi::Topic::tubes:         paintTubes (g, area); break;
            case gi::Topic::drumsBars:     paintPartials (g, area); break;
            case gi::Topic::keys:
            case gi::Topic::guitar:
            case gi::Topic::bass:
            case gi::Topic::synths:
            case gi::Topic::drumKit:
            case gi::Topic::realMidi:
            case gi::Topic::orchestration:
            case gi::Topic::bowed:
            case gi::Topic::brass:
            case gi::Topic::woodwinds:
            case gi::Topic::voice:         paintMixed (g, area); break;
            case gi::Topic::yours:         paintYours (g, area); break;
        }
    }

private:
    struct Hit
    {
        juce::Rectangle<float> area;
        int pitch = -1;                 // -1: tooltip only
        float velocity = 0.8f;
        juce::String tip;
    };

    void addHit (juce::Rectangle<float> r, int pitch, const juce::String& tip, float velocity = 0.8f)
    {
        hits.push_back ({ r, pitch, velocity, tip });
    }

    static void label (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size = 12.0f, bool bold = false,
                       juce::Colour colour = theme::textDim, juce::Justification just = juce::Justification::centredLeft)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (text, r.toNearestInt(), just, 2, 0.8f);
    }

    //==============================================================================
    // How instruments make sound: eight tiles.
    void paintTiles (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const int cols = 4, rows = 2;
        const float gap = 10.0f;
        const float tw = (area.getWidth() - gap * (cols - 1)) / cols, th = (area.getHeight() - gap * (rows - 1)) / rows;

        for (int i = 0; i < (int) example.labels.size(); ++i)
        {
            const juce::Rectangle<float> tile (area.getX() + (float) (i % cols) * (tw + gap), area.getY() + (float) (i / cols) * (th + gap), tw, th);
            g.setColour (theme::lane);
            g.fillRoundedRectangle (tile, 6.0f);

            auto inner = tile.reduced (10.0f, 8.0f);
            const auto glyph = inner.removeFromTop (inner.getHeight() * 0.42f);
            paintGlyph (g, glyph, i);
            inner.removeFromTop (4.0f);
            label (g, example.labels[i], inner.removeFromTop (20.0f), 14.0f, true, theme::text);
            label (g, example.details[i], inner, 12.0f);
            addHit (tile, -1, example.labels[i] + ": " + example.details[i]);
        }
    }

    static void paintGlyph (juce::Graphics& g, juce::Rectangle<float> r, int kind)
    {
        const auto c = theme::accent;
        g.setColour (c);
        const auto centre = r.getCentre();

        switch (kind)
        {
            case 0:   // string: a standing wave between two posts
                drawEnvelope (g, r.reduced (8.0f, 2.0f), [] (float t) { return std::sin (juce::MathConstants<float>::twoPi * t) * 0.9f; }, c);
                g.fillRect (r.getX() + 4.0f, r.getY(), 3.0f, r.getHeight());
                g.fillRect (r.getRight() - 7.0f, r.getY(), 3.0f, r.getHeight());
                break;

            case 1:   // air column: a tube with a standing wave
                g.drawRoundedRectangle (r.reduced (6.0f, 4.0f), 5.0f, 1.5f);
                drawEnvelope (g, r.reduced (12.0f, 8.0f), [] (float t) { return std::sin (juce::MathConstants<float>::pi * t); }, c);
                break;

            case 2:   // reed: a flat blade that flaps against its mouthpiece
                g.fillRoundedRectangle (r.getX() + 10.0f, centre.y + 4.0f, r.getWidth() - 20.0f, 4.0f, 2.0f);
                drawEnvelope (g, { r.getX() + 10.0f, centre.y - 12.0f, r.getWidth() - 20.0f, 14.0f },
                              [] (float t) { return std::sin (juce::MathConstants<float>::pi * t * 6.0f) * t; }, c);
                break;

            case 3:   // lips: two lips buzzing
            {
                juce::Path p;
                p.addCentredArc (centre.x, centre.y - 8.0f, 22.0f, 9.0f, 0.0f, juce::MathConstants<float>::pi * 0.5f, juce::MathConstants<float>::pi * 1.5f, true);
                p.addCentredArc (centre.x, centre.y + 8.0f, 22.0f, 9.0f, 0.0f, -juce::MathConstants<float>::pi * 0.5f, juce::MathConstants<float>::pi * 0.5f, true);
                g.strokePath (p, juce::PathStrokeType (2.0f));
                break;
            }

            case 4:   // membrane: a drum head
                for (float k : { 1.0f, 0.66f, 0.33f })
                    g.drawEllipse (juce::Rectangle<float> (r.getWidth() * 0.55f * k, r.getHeight() * 0.8f * k).withCentre (centre), 1.5f);

                break;

            case 5:   // bar: a bar on two supports, nodes at a fifth of its length
                g.fillRoundedRectangle (juce::Rectangle<float> (r.getWidth() * 0.7f, 8.0f).withCentre (centre.translated (0.0f, -4.0f)), 2.0f);
                g.fillRect (centre.x - r.getWidth() * 0.35f * 0.55f, centre.y + 2.0f, 3.0f, 10.0f);
                g.fillRect (centre.x + r.getWidth() * 0.35f * 0.55f, centre.y + 2.0f, 3.0f, 10.0f);
                break;

            case 6:   // vocal folds: two folds with a gap
            {
                juce::Path p;
                p.startNewSubPath (r.getX() + 14.0f, centre.y - 10.0f);
                p.quadraticTo (centre.x, centre.y + 2.0f, r.getRight() - 14.0f, centre.y - 10.0f);
                p.startNewSubPath (r.getX() + 14.0f, centre.y + 10.0f);
                p.quadraticTo (centre.x, centre.y - 2.0f, r.getRight() - 14.0f, centre.y + 10.0f);
                g.strokePath (p, juce::PathStrokeType (2.0f));
                break;
            }

            default:  // speaker cone
                g.drawEllipse (juce::Rectangle<float> (r.getHeight() * 0.9f, r.getHeight() * 0.9f).withCentre (centre), 1.5f);
                g.fillEllipse (juce::Rectangle<float> (r.getHeight() * 0.3f, r.getHeight() * 0.3f).withCentre (centre));
                break;
        }
    }

    //==============================================================================
    // Strings and their harmonics.
    void paintStrings (juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (example.numbers.size() < 2 || example.labels.size() < 2)
            return;

        const float topH = area.getHeight() * 0.62f;
        auto upper = area.withHeight (topH);
        auto lower = area.withTrimmedTop (topH + 8.0f);

        label (g, example.labels[0], upper.removeFromTop (18.0f), 12.5f, true);
        const float rowH = upper.getHeight() / 4.0f;
        const float labelW = 118.0f;

        for (int i = 0; i < 4; ++i)
        {
            const auto row = upper.removeFromTop (rowH);
            const double hz = example.numbers[0][(size_t) i];
            const int n = i + 1;
            const auto text = juce::String (n == 1 ? "Whole string" : n == 2 ? "Two halves" : n == 3 ? "Three thirds" : "Four quarters");
            label (g, text, row.withWidth (labelW).withTrimmedBottom (rowH * 0.5f), 11.5f);
            label (g, formatHz (hz), row.withWidth (labelW).withTrimmedTop (rowH * 0.45f), 12.5f, true, theme::text);

            const auto wave = row.withTrimmedLeft (labelW).reduced (6.0f, 2.0f);
            drawEnvelope (g, wave, [n] (float t) { return std::sin (juce::MathConstants<float>::pi * (float) n * t); }, instrumentsui::getFamilyColour (gi::Family::strings));
            g.setColour (theme::text);
            g.fillRect (wave.getX() - 2.0f, wave.getY(), 3.0f, wave.getHeight());
            g.fillRect (wave.getRight() - 1.0f, wave.getY(), 3.0f, wave.getHeight());
            addHit (row, getPitchOf (hz), "Harmonic " + juce::String (n) + " · " + formatHz (hz) + " · " + midiguide::getNoteName (getPitchOf (hz)) + " · click to hear it");
        }

        label (g, example.labels[1], lower.removeFromTop (18.0f), 12.5f, true);
        const float lh = lower.getHeight() / 3.0f;
        const double wholeHz = example.numbers[1][0];
        const double fractions[] { 1.0, 0.5, 2.0 / 3.0 };
        const char* names[] { "Whole string", "Half the length", "Two thirds of the length" };

        for (int i = 0; i < 3; ++i)
        {
            const auto row = lower.removeFromTop (lh);
            const double hz = example.numbers[1][(size_t) i];
            const auto line = row.withTrimmedLeft (labelW).withTrimmedRight (170.0f);
            const float len = line.getWidth() * (float) fractions[(size_t) i];
            g.setColour (instrumentsui::getFamilyColour (gi::Family::strings));
            g.fillRoundedRectangle (line.getX(), row.getCentreY() - 2.0f, len, 4.0f, 2.0f);
            label (g, names[i], row.withWidth (labelW), 11.5f);
            label (g, formatHz (hz) + (i > 0 ? "  (x " + juce::String (hz / wholeHz, 2) + ")" : juce::String()),
                   { line.getX() + len + 8.0f, row.getY(), 160.0f, row.getHeight() }, 12.5f, true, theme::text);
            addHit (row, getPitchOf (hz), juce::String (names[i]) + " · " + formatHz (hz) + " · " + midiguide::getNoteName (getPitchOf (hz)) + " · click to hear it");
        }
    }

    //==============================================================================
    // Tubes: open at both ends and closed at one.
    void paintTubes (juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (example.numbers.size() < 2 || example.labels.size() < 2)
            return;

        const float rowH = area.getHeight() / 2.0f;

        for (int row = 0; row < 2; ++row)
        {
            auto r = area.withY (area.getY() + (float) row * rowH).withHeight (rowH - 6.0f);
            label (g, example.labels[row] + " · a 66 cm tube", r.removeFromTop (18.0f), 12.5f, true);

            const auto tubeArea = r.removeFromLeft (r.getWidth() * 0.5f).reduced (4.0f, 8.0f);
            auto tube = tubeArea.withSizeKeepingCentre (tubeArea.getWidth(), juce::jmin (60.0f, tubeArea.getHeight()));
            g.setColour (theme::textDim);
            g.drawRoundedRectangle (tube, 6.0f, 1.5f);

            if (row == 1)
            {
                g.setColour (theme::textDim);
                g.fillRoundedRectangle (tube.getX() - 3.0f, tube.getY() - 2.0f, 7.0f, tube.getHeight() + 4.0f, 3.0f);
            }

            // The first standing wave that fits: half a wave open, a quarter closed.
            const bool closed = row == 1;
            drawEnvelope (g, tube.reduced (6.0f, 6.0f),
                          [closed] (float t) { return closed ? std::cos (juce::MathConstants<float>::halfPi * t) : std::sin (juce::MathConstants<float>::pi * t); },
                          instrumentsui::getFamilyColour (gi::Family::woodwinds));

            const auto& hz = example.numbers[(size_t) row];
            const float barH = r.getHeight() / 3.0f;
            r.removeFromLeft (10.0f);

            for (int i = 0; i < 3; ++i)
            {
                const auto b = r.removeFromTop (barH);
                const int harmonic = closed ? 2 * i + 1 : i + 1;
                const float w = (float) (hz[(size_t) i] / 800.0) * (b.getWidth() - 190.0f);
                g.setColour (instrumentsui::getFamilyColour (gi::Family::woodwinds));
                g.fillRoundedRectangle (b.getX(), b.getCentreY() - 7.0f, w, 14.0f, 3.0f);
                label (g, "harmonic " + juce::String (harmonic) + " · " + formatHz (hz[(size_t) i]), { b.getX() + w + 6.0f, b.getY(), 200.0f, b.getHeight() }, 11.5f, false, theme::text);
                addHit (b, getPitchOf (hz[(size_t) i]), "Harmonic " + juce::String (harmonic) + " of the " + (closed ? "closed" : "open") + " tube · " + formatHz (hz[(size_t) i])
                                                         + " · click to hear the nearest note, " + midiguide::getNoteName (getPitchOf (hz[(size_t) i])));
            }
        }
    }

    //==============================================================================
    // Drums, bars and bells: partials of four things on one frequency axis.
    void paintPartials (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const float labelW = 130.0f, maxRatio = 6.0f;
        const float rowH = area.getHeight() / (float) juce::jmax (1, (int) example.numbers.size());
        const float x0 = area.getX() + labelW, axisW = area.getWidth() - labelW - 14.0f;
        const auto xOf = [=] (double ratio) { return x0 + (float) (ratio / maxRatio) * axisW; };

        for (int i = 0; i < (int) example.numbers.size(); ++i)
        {
            const auto row = area.withY (area.getY() + (float) i * rowH).withHeight (rowH);
            const auto colour = instrumentsui::getFamilyColour (i == 0 ? gi::Family::guitar : i == 1 ? gi::Family::drums : gi::Family::synths);
            label (g, example.labels[i], row.withWidth (labelW - 6.0f), 12.5f, true, theme::text);

            g.setColour (theme::grid);
            g.fillRect (x0, row.getCentreY() - 0.5f, axisW, 1.0f);
            const auto& ratios = example.numbers[(size_t) i];

            for (size_t k = 0; k < ratios.size(); ++k)
            {
                const float x = xOf (ratios[k]);
                g.setColour (colour);
                g.fillRect (x - 1.5f, row.getCentreY() - rowH * 0.2f, 3.0f, rowH * 0.4f);
                const bool above = k % 2 == 0;
                const juce::Rectangle<float> t (x - 20.0f, above ? row.getCentreY() - rowH * 0.2f - 16.0f : row.getCentreY() + rowH * 0.2f, 40.0f, 16.0f);
                label (g, juce::String (ratios[k], k == 0 || ratios[k] >= 10.0 || juce::approximatelyEqual (std::round (ratios[k]), ratios[k]) ? 0 : 3), t, 11.0f, false, theme::textDim, juce::Justification::centred);
                const double hz = 110.0 * ratios[k];
                addHit ({ x - 8.0f, row.getY(), 16.0f, rowH }, getPitchOf (hz),
                        example.labels[i] + " · x" + juce::String (ratios[k], 3) + " the first · " + formatHz (hz) + " if the first is 110 Hz · click to hear it");
            }
        }

        label (g, "Frequency, as a multiple of the lowest", { x0, area.getBottom() - 14.0f, axisW, 14.0f }, 10.5f, false, theme::textDim, juce::Justification::centredRight);
    }

    //==============================================================================
    // Range bars on a keyboard, with numbers or lines of notes under them.
    void paintMixed (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const bool hasRanges = ! example.ranges.empty();
        const bool hasBrass = topic == gi::Topic::brass && example.numbers.size() >= 2;
        const bool hasLines = ! example.lines.empty();
        const int sections = (hasLines ? 1 : 0) + (hasBrass ? 1 : 0);

        if (hasRanges)
        {
            const int n = (int) example.ranges.size();
            const float natural = 22.0f * (float) n + 22.0f;
            const float h = sections == 0 ? area.getHeight() : juce::jmin (natural, area.getHeight() * (topic == gi::Topic::orchestration ? 0.64f : 0.45f));
            paintRanges (g, area.removeFromTop (h));
            area.removeFromTop (10.0f);
        }

        if (hasBrass)
            paintBrassNumbers (g, area);
        else if (hasLines)
        {
            const float each = area.getHeight() / (float) example.lines.size();

            for (int i = 0; i < (int) example.lines.size(); ++i)
                paintLine (g, area.withY (area.getY() + (float) i * each).withHeight (each - (i + 1 < (int) example.lines.size() ? 8.0f : 0.0f)), i);
        }
    }

    void paintRanges (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto& bars = example.ranges;
        int lo = 127, hi = 0;

        for (auto& b : bars)
        {
            lo = juce::jmin (lo, b.lowest);
            hi = juce::jmax (hi, b.highest);
        }

        lo = juce::jmax (0, lo - 1);
        hi = juce::jmin (127, hi + 2);
        const float nameW = 112.0f, axisH = 18.0f;
        const float x0 = area.getX() + nameW, axisW = area.getWidth() - nameW - 6.0f;
        const auto xOf = [=] (double p) { return x0 + (float) ((p - lo) / (double) (hi - lo + 1)) * axisW; };
        const float rowH = juce::jlimit (11.0f, 26.0f, (area.getHeight() - axisH) / (float) bars.size());

        for (int p = lo; p <= hi + 1; ++p)
            if (p % 12 == 0)
            {
                g.setColour (theme::grid);
                g.fillRect (xOf (p), area.getY() + axisH, 1.0f, rowH * (float) bars.size());
                label (g, "C" + juce::String (p / 12 - 1), { xOf (p) - 14.0f, area.getY(), 28.0f, axisH }, 11.0f, false, theme::textDim, juce::Justification::centred);
            }

        for (int i = 0; i < (int) bars.size(); ++i)
        {
            const auto& b = bars[(size_t) i];
            const float y = area.getY() + axisH + (float) i * rowH;
            const juce::Rectangle<float> bar (xOf (b.lowest), y + 2.0f, xOf (b.highest + 1) - xOf (b.lowest), rowH - 4.0f);
            const auto colour = instrumentsui::getFamilyColour (b.family);
            label (g, b.name, { area.getX(), y, nameW - 6.0f, rowH }, rowH < 15.0f ? 10.5f : 12.0f, true, theme::text);
            g.setColour (colour.withAlpha (0.85f));
            g.fillRoundedRectangle (bar, 3.0f);

            const auto text = midiguide::getNoteName (b.lowest) + " - " + midiguide::getNoteName (b.highest);

            // Open-string dots sit along the bottom edge so they never cover the name of the range.
            if (bar.getWidth() > 78.0f)
                label (g, text, b.openStrings.empty() ? bar : bar.withTrimmedBottom (7.0f), 10.5f, true, juce::Colours::black.withAlpha (0.75f), juce::Justification::centred);
            else
                label (g, text, { bar.getRight() + 5.0f, y, 70.0f, rowH }, 10.5f, false, theme::textDim);

            for (int s : b.openStrings)
            {
                g.setColour (juce::Colours::white);
                g.fillEllipse (xOf (s + 0.5) - 2.5f, bar.getBottom() - 7.0f, 5.0f, 5.0f);
            }

            const auto tip = b.name + " · " + midiguide::getNoteName (b.lowest) + " to " + midiguide::getNoteName (b.highest) + " · "
                             + juce::String (juce::roundToInt (gi::getNoteHz (b.lowest))) + " to " + juce::String (juce::roundToInt (gi::getNoteHz (b.highest)))
                             + " Hz · click an end to hear it";
            addHit (bar, -1, tip);
            addHit (bar.withWidth (juce::jmin (14.0f, bar.getWidth() / 2.0f)), b.lowest, tip);
            addHit (bar.withTrimmedLeft (bar.getWidth() - juce::jmin (14.0f, bar.getWidth() / 2.0f)), b.highest, tip);

            for (int s : b.openStrings)
                addHit ({ xOf (s + 0.5) - 5.0f, bar.getBottom() - 10.0f, 10.0f, 10.0f }, s,
                        "Open string · " + midiguide::getNoteName (s) + " · " + formatHz (gi::getNoteHz (s)) + " · click to hear it");
        }
    }

    /** The trumpet's harmonics: their frequencies, and how far each is from the nearest note of equal temperament. */
    void paintBrassNumbers (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto& hz = example.numbers[0];
        const auto& cents = example.numbers[1];
        label (g, example.labels[0] + ", and how far each is from the nearest note (cents)", area.removeFromTop (18.0f), 12.5f, true);
        const int n = (int) hz.size();
        const float colW = area.getWidth() / (float) n;
        const float zeroY = area.getY() + area.getHeight() * 0.5f, scale = (area.getHeight() * 0.5f - 34.0f) / 35.0f;

        g.setColour (theme::gridBar);
        g.fillRect (area.getX(), zeroY, area.getWidth(), 1.0f);

        for (int i = 0; i < n; ++i)
        {
            const float cx = area.getX() + ((float) i + 0.5f) * colW;
            const double c = cents[(size_t) i];
            const bool flatter = c < -0.5, sharper = c > 0.5;
            const float len = (float) std::abs (c) * scale;
            const juce::Rectangle<float> bar (cx - 12.0f, c < 0.0 ? zeroY : zeroY - len, 24.0f, juce::jmax (2.0f, len));
            g.setColour (std::abs (c) > 10.0 ? theme::meterYellow : theme::meterGreen);
            g.fillRoundedRectangle (bar, 2.0f);
            label (g, juce::String (i + 2), { cx - 30.0f, area.getY(), 60.0f, 16.0f }, 12.0f, true, theme::text, juce::Justification::centred);
            label (g, juce::String (hz[(size_t) i], 1) + " Hz", { cx - 40.0f, area.getBottom() - 34.0f, 80.0f, 16.0f }, 11.0f, false, theme::textDim, juce::Justification::centred);
            label (g, (c < -0.05 ? utf8 ("\xe2\x88\x92") : juce::String()) + juce::String (std::abs (c), 1) + utf8 ("\xc2\xa2"),
                   { cx - 40.0f, area.getBottom() - 18.0f, 80.0f, 16.0f }, 11.5f, true, theme::text, juce::Justification::centred);
            addHit ({ cx - colW * 0.5f, area.getY(), colW, area.getHeight() }, getPitchOf (hz[(size_t) i]),
                    "Harmonic " + juce::String (i + 2) + " · " + formatHz (hz[(size_t) i]) + " · " + juce::String (std::abs (c), 1) + " cents "
                        + (flatter ? "flat of" : sharper ? "sharp of" : "from") + " " + midiguide::getNoteName (getPitchOf (hz[(size_t) i])) + " · click to hear that note");
        }
    }

    //==============================================================================
    // A line of notes as a piano roll, with velocity bars and the controllers under it.
    void paintLine (juce::Graphics& g, juce::Rectangle<float> area, int index)
    {
        const auto& line = example.lines[(size_t) index];
        const bool drumMap = topic == gi::Topic::drumKit;
        const double offset = offsets[(size_t) index];
        const auto colour = instrumentsui::getFamilyColour (topic == gi::Topic::drumKit ? gi::Family::drums
                                                            : topic == gi::Topic::guitar ? gi::Family::guitar
                                                            : topic == gi::Topic::bass ? gi::Family::bass
                                                            : topic == gi::Topic::synths ? gi::Family::synths
                                                            : topic == gi::Topic::keys ? gi::Family::keys
                                                            : topic == gi::Topic::orchestration ? (index == 0 ? gi::Family::strings : gi::Family::woodwinds)
                                                                                                : gi::Family::strings);

        double extent = 4.0;

        for (auto& l : example.lines)
        {
            for (auto& n : l.notes)
                extent = juce::jmax (extent, n.start + n.length);

            for (auto& p : l.expression)
                extent = juce::jmax (extent, p.beat);

            for (auto& p : l.bend)
                extent = juce::jmax (extent, p.beat);
        }

        extent = std::ceil (extent);
        auto head = area.removeFromTop (18.0f);
        label (g, line.label, head, 12.5f, true);

        for (auto& c : line.chordLabels)
        {
            const float labelLeft = area.getX() + 76.0f;
            label (g, c.second, { labelLeft + (float) (c.first / extent) * (area.getWidth() - 76.0f) + 4.0f, head.getY(), 40.0f, head.getHeight() }, 12.0f, true, theme::text);
        }

        const bool wantExpression = ! line.expression.empty() || topic == gi::Topic::realMidi;
        const bool wantBend = ! line.bend.empty();
        const float laneH = juce::jmin (34.0f, area.getHeight() * 0.2f);
        const float exprH = wantExpression ? laneH : 0.0f, bendH = wantBend ? laneH : 0.0f;
        const auto lanes = area.removeFromBottom (laneH + exprH + bendH + 6.0f * (1 + (wantExpression ? 1 : 0) + (wantBend ? 1 : 0)));
        const auto roll = area.withTrimmedLeft (76.0f);
        const float x0 = roll.getX(), w = roll.getWidth();
        const auto xOf = [=] (double beat) { return x0 + (float) (beat / extent) * w; };

        // Rows: the drum numbers used, or the pitches from the lowest to the highest.
        std::vector<int> rowPitches;

        if (drumMap)
        {
            for (auto& n : line.notes)
                if (std::find (rowPitches.begin(), rowPitches.end(), n.pitch) == rowPitches.end())
                    rowPitches.push_back (n.pitch);

            std::sort (rowPitches.begin(), rowPitches.end(), std::greater<int>());
        }

        int lo = 127, hi = 0;

        for (auto& l : example.lines)
            for (auto& n : l.notes)
            {
                lo = juce::jmin (lo, n.pitch);
                hi = juce::jmax (hi, n.pitch);
            }

        if (! drumMap && (topic == gi::Topic::orchestration || topic == gi::Topic::realMidi))
        {
            lo = 127;
            hi = 0;

            for (auto& n : line.notes)
            {
                lo = juce::jmin (lo, n.pitch);
                hi = juce::jmax (hi, n.pitch);
            }
        }

        lo -= 1;
        hi += 1;
        const int numRows = drumMap ? (int) rowPitches.size() : hi - lo + 1;
        const float rowH = roll.getHeight() / (float) juce::jmax (1, numRows);

        g.setColour (theme::lane);
        g.fillRect (roll);

        for (int r = 0; r < numRows; ++r)
        {
            const int p = drumMap ? rowPitches[(size_t) r] : hi - r;
            const float y = roll.getY() + (float) r * rowH;

            if (drumMap)
            {
                label (g, getDrumLabel (p), { area.getX(), y, 72.0f, rowH }, 10.0f, false, theme::textDim);
                g.setColour (theme::grid);
                g.fillRect (x0, y + rowH - 1.0f, w, 1.0f);
            }
            else if (rowH >= 9.0f || p % 12 == 0)
            {
                if (p % 12 == 0 || rowH >= 12.0f)
                    label (g, midiguide::getNoteName (p), { area.getX(), y, 66.0f, juce::jmax (rowH, 12.0f) }, 10.5f, false, theme::textDim, juce::Justification::centredRight);

                g.setColour (theme::grid);
                g.fillRect (x0, y + rowH - 1.0f, w, 1.0f);
            }
        }

        for (int b = 0; b <= (int) extent; ++b)
        {
            g.setColour (b % 4 == 0 ? theme::gridBar : theme::grid);
            g.fillRect (xOf (b), roll.getY(), 1.0f, roll.getHeight() + lanes.getHeight());
        }

        // Notes, lit while they sound.
        for (size_t i = 0; i < line.notes.size(); ++i)
        {
            const auto& n = line.notes[i];
            const int row = drumMap ? (int) (std::find (rowPitches.begin(), rowPitches.end(), n.pitch) - rowPitches.begin()) : hi - n.pitch;
            const bool lit = playBeat >= offset + n.start && playBeat < offset + n.start + juce::jmax (n.length, 0.2);
            juce::Rectangle<float> r (xOf (n.start), roll.getY() + (float) row * rowH + 1.0f, juce::jmax (drumMap ? 7.0f : 4.0f, xOf (n.start + n.length) - xOf (n.start) - 1.0f), juce::jmax (3.0f, rowH - 2.0f));
            g.setColour (lit ? juce::Colours::white : colour.withAlpha (0.55f + 0.45f * (float) n.velocity / 127.0f));
            g.fillRoundedRectangle (r, 2.0f);

            // How far this note runs into the next one (legato), when it's close enough to mean something.
            double nextStart = 1.0e9;

            for (auto& m : line.notes)
                if (m.start > n.start + 1.0e-6)
                    nextStart = juce::jmin (nextStart, m.start);

            const double overlap = n.start + n.length - nextStart;
            auto tip = midiguide::getNoteName (n.pitch) + " · velocity " + juce::String (n.velocity);

            if (drumMap)
                tip = getDrumLabel (n.pitch) + " · velocity " + juce::String (n.velocity);

            if (overlap > 0.0004 && overlap < 0.5)
                tip += " · " + formatBeats (overlap) + " beats overlap";

            addHit (r.expanded (1.0f), n.pitch, tip + " · click to hear it", (float) n.velocity / 127.0f);
        }

        // Velocity bars.
        auto velArea = lanes.withTrimmedLeft (76.0f).withHeight (laneH).withY (lanes.getY() + 6.0f);
        label (g, "Velocity", velArea.withX (area.getX()).withWidth (72.0f), 10.5f);
        g.setColour (theme::lane);
        g.fillRect (velArea);

        for (auto& n : line.notes)
        {
            g.setColour (colour);
            g.fillRect (xOf (n.start), velArea.getBottom() - (float) n.velocity / 127.0f * velArea.getHeight(), 4.0f, (float) n.velocity / 127.0f * velArea.getHeight());
        }

        float y = velArea.getBottom() + 6.0f;

        if (wantExpression)
        {
            const juce::Rectangle<float> er (x0, y, w, exprH);
            label (g, "Expression", { area.getX(), y, 72.0f, exprH }, 10.5f);
            g.setColour (theme::lane);
            g.fillRect (er);

            if (line.expression.empty())
                label (g, "none: every note at full level", er.reduced (6.0f, 0.0f), 10.5f, false, theme::textDim);
            else
                drawControllerLine (g, er, line.expression, 127, extent, colour);

            y += exprH + 6.0f;
        }

        if (wantBend)
        {
            const juce::Rectangle<float> br (x0, y, w, bendH);
            label (g, "Pitch bend", { area.getX(), y, 72.0f, bendH }, 10.5f);
            g.setColour (theme::lane);
            g.fillRect (br);
            g.setColour (theme::grid);
            g.fillRect (br.getX(), br.getCentreY(), br.getWidth(), 1.0f);
            drawControllerLine (g, br, line.bend, 16383, extent, colour);
            addHit (br, -1, "Pitch bend: up 2 semitones into the note, then back. The two later notes overlap, so a mono synth with glide on slides between them, over its glide time ("
                              + juce::String (example.glideMs) + " ms in this example).");
        }
    }

    static void drawControllerLine (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<gi::ExamplePoint>& points, int maxValue, double extent, juce::Colour colour)
    {
        juce::Path p;
        const auto xOf = [&] (double beat) { return r.getX() + (float) (beat / extent) * r.getWidth(); };
        const auto yOf = [&] (int v) { return r.getBottom() - 2.0f - (float) v / (float) maxValue * (r.getHeight() - 4.0f); };
        bool first = true;

        for (auto& pt : points)
        {
            if (first)
            {
                p.startNewSubPath (r.getX(), yOf (pt.value));
                first = false;
            }

            p.lineTo (xOf (pt.beat), yOf (pt.value));
        }

        if (! points.empty())
            p.lineTo (r.getRight(), yOf (points.back().value));

        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.8f));
    }

    //==============================================================================
    // Your track: its notes as a pitch strip against the instrument's range.
    void paintYours (juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (yours.notes.empty())
        {
            label (g, yours.message, area.reduced (16.0f), 14.0f, false, theme::textDim, juce::Justification::centred);
            return;
        }

        const gi::Info* info = yours.instrument.has_value() ? &gi::getInfo (*yours.instrument) : nullptr;
        const bool hasBand = info != nullptr && info->lowest >= 0;
        int lo = 127, hi = 0;
        double first = 1.0e9, last = 0.0;

        for (size_t i = 0; i < yours.notes.size(); ++i)
        {
            const auto& n = yours.notes[i];
            lo = juce::jmin (lo, n.pitch);
            hi = juce::jmax (hi, n.pitch);
            first = juce::jmin (first, n.startBeat);
            last = juce::jmax (last, n.startBeat + yours.lengthBeats[i]);
        }

        if (hasBand)
        {
            lo = juce::jmin (lo, info->lowest);
            hi = juce::jmax (hi, info->highest);
        }

        lo = juce::jmax (0, lo - 2);
        hi = juce::jmin (127, hi + 2);
        const double barLen = 4.0;
        first = std::floor (first / barLen) * barLen;
        last = juce::jmax (last, first + barLen);
        last = std::ceil (last / barLen) * barLen;

        const float footH = 30.0f, axisH = 16.0f, leftW = 40.0f;
        const auto strip = area.withTrimmedLeft (leftW).withTrimmedTop (axisH).withTrimmedBottom (footH);
        const auto xOf = [=] (double beat) { return strip.getX() + (float) ((beat - first) / (last - first)) * strip.getWidth(); };
        const float rowH = strip.getHeight() / (float) (hi - lo + 1);
        const auto yOf = [=] (int pitch) { return strip.getY() + (float) (hi - pitch) * rowH; };

        g.setColour (theme::lane);
        g.fillRect (strip);

        if (hasBand)
        {
            const juce::Rectangle<float> band (strip.getX(), yOf (info->highest), strip.getWidth(), (float) (info->highest - info->lowest + 1) * rowH);
            g.setColour (theme::meterGreen.withAlpha (0.14f));
            g.fillRect (band);
            g.setColour (theme::meterGreen.withAlpha (0.6f));
            g.fillRect (band.getX(), band.getY(), band.getWidth(), 1.0f);
            g.fillRect (band.getX(), band.getBottom() - 1.0f, band.getWidth(), 1.0f);
        }

        for (int p = lo; p <= hi; ++p)
            if (p % 12 == 0)
            {
                g.setColour (theme::grid);
                g.fillRect (strip.getX(), yOf (p) + rowH - 1.0f, strip.getWidth(), 1.0f);
                label (g, "C" + juce::String (p / 12 - 1), { area.getX(), yOf (p) - 6.0f + rowH * 0.5f, leftW - 4.0f, 12.0f }, 10.5f, false, theme::textDim, juce::Justification::centredRight);
            }

        const int totalBars = juce::roundToInt ((last - first) / barLen);
        const int labelEvery = juce::jmax (1, (int) std::ceil (totalBars / juce::jmax (1.0f, strip.getWidth() / 34.0f)));

        for (int b = 0; b <= totalBars; ++b)
        {
            g.setColour (theme::gridBar);
            g.fillRect (xOf (first + b * barLen), strip.getY(), 1.0f, strip.getHeight());

            if (b < totalBars && b % labelEvery == 0)
                label (g, juce::String (juce::roundToInt (first / barLen) + b + 1), { xOf (first + b * barLen) + 2.0f, area.getY(), 30.0f, axisH }, 10.5f);
        }

        // Moments with more notes than the instrument plays at once.
        std::vector<size_t> order (yours.notes.size());

        for (size_t i = 0; i < order.size(); ++i)
            order[i] = i;

        std::stable_sort (order.begin(), order.end(), [this] (size_t a, size_t b) { return yours.notes[a].startBeat < yours.notes[b].startBeat; });
        bool anyCrowded = false;

        for (size_t i = 0; i < order.size();)
        {
            size_t j = i + 1;

            while (j < order.size() && yours.notes[order[j]].startBeat - yours.notes[order[j - 1]].startBeat <= 0.05)
                ++j;

            if (info != nullptr && info->atOnce > 0 && (int) (j - i) > info->atOnce)
            {
                anyCrowded = true;
                const float x = xOf (yours.notes[order[i]].startBeat);
                g.setColour (theme::meterYellow);
                juce::Path tri;
                tri.addTriangle (x - 4.0f, strip.getBottom() + 3.0f, x + 4.0f, strip.getBottom() + 3.0f, x, strip.getBottom() - 4.0f);
                g.fillPath (tri);
                addHit ({ x - 6.0f, strip.getBottom() - 6.0f, 12.0f, 14.0f }, -1,
                        "Bar " + juce::String (yours.notes[order[i]].bar) + " · " + juce::String ((int) (j - i)) + " notes start together, more than the "
                            + gi::getSentenceName (*info) + " plays at once");
            }

            i = j;
        }

        for (size_t i = 0; i < yours.notes.size(); ++i)
        {
            const auto& n = yours.notes[i];
            const bool outside = hasBand && (n.pitch < info->lowest || n.pitch > info->highest);
            const juce::Rectangle<float> r (xOf (n.startBeat), yOf (n.pitch) + 0.5f, juce::jmax (3.0f, xOf (n.startBeat + yours.lengthBeats[i]) - xOf (n.startBeat) - 1.0f), juce::jmax (2.0f, rowH - 1.0f));
            g.setColour (outside ? theme::meterRed : theme::accent);
            g.fillRoundedRectangle (r, 1.5f);
            addHit (r.expanded (0.0f, 1.0f), n.pitch, "Bar " + juce::String (n.bar) + " · " + midiguide::getNoteName (n.pitch) + " · velocity " + juce::String (n.velocity)
                                                       + (outside ? " · outside the range" : juce::String()) + " · click to hear it", (float) n.velocity / 127.0f);
        }

        juce::String key = "Notes in blue, bar numbers on top.";

        if (hasBand)
            key = "The green band is the " + gi::getSentenceName (*info) + "'s range, " + midiguide::getNoteName (info->lowest) + " to " + midiguide::getNoteName (info->highest)
                  + ". Red notes are outside it" + (anyCrowded ? "; a yellow triangle marks more notes at once than it can play." : ".");
        else if (info != nullptr)
            key = "This instrument has no fixed range. Notes in blue, bar numbers on top.";
        else
            key = "No instrument chosen yet, so there is no range to draw. Choose one above.";

        label (g, key, area.withTop (area.getBottom() - footH + 6.0f), 11.5f);
    }

    Engine& engine;
    gi::Topic topic = gi::Topic::makeSound;
    gi::Example example;
    std::vector<double> offsets;
    Yours yours;
    std::vector<Hit> hits;
    double playBeat = -1.0;
};

//==============================================================================
InstrumentsExplainer::InstrumentsExplainer (Engine& e, Selection& s) : engine (e), selection (s)
{
    title.setText ("Instruments in MIDI", juce::dontSendNotification);
    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    list.setModel (this);
    list.setRowHeight (46);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    picture = std::make_unique<Picture> (engine);
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

    choiceLabel.setText ("Instrument:", juce::dontSendNotification);
    choiceLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addChildComponent (choiceLabel);

    choice.setTooltip ("Which instrument Your track checks against. Guess uses the track's DawIt Sampler preset, then its name, then its DawIt sound.");
    choice.addItem ("Guess", 1);

    for (auto f : gi::allFamilies)
    {
        choice.addSectionHeading (gi::getFamilyName (f));

        int index = 0;

        for (auto& info : gi::getInstruments())
        {
            if (info.family == f)
                choice.addItem (info.name, 100 + index);

            ++index;
        }
    }

    choice.setSelectedId (1, juce::dontSendNotification);
    choice.onChange = [this]
    {
        if (! updatingChoice)
            refreshYours();
    };
    addChildComponent (choice);

    playButton.setTooltip ("Play the example once, on the selected MIDI track's instrument (or a piano), lighting each note as it sounds");
    playButton.onClick = [this] { playing ? stop() : play(); };
    addAndMakeVisible (playButton);
    addAndMakeVisible (exportButton);

    engine.addChangeListener (this);
    selection.addChangeListener (this);

    list.updateContent();
    list.selectRow (0);
    setSize (width, height);
}

InstrumentsExplainer::~InstrumentsExplainer()
{
    stop();
    selection.removeChangeListener (this);
    engine.removeChangeListener (this);
}

InstrumentsExplainer::Captured InstrumentsExplainer::capture() const
{
    Captured c;
    Track* track = selection.getTrack();

    if (track == nullptr || ! track->isMidi())
    {
        track = nullptr;

        for (auto& t : engine.getTracks())
            if (t->isMidi())
            {
                const juce::ScopedLock lock (engine.getLock());
                bool hasNotes = false;

                for (auto& clip : t->clips)
                    if (clip->kind == Clip::Kind::midi && ! static_cast<MidiClip*> (clip.get())->notes.empty())
                        hasNotes = true;

                if (hasNotes)
                {
                    track = t.get();
                    break;
                }
            }
    }

    if (track == nullptr)
        return c;

    c.track = track;
    c.part.hasTrack = true;
    c.part.trackName = track->name;
    c.hints = instrumentsui::getHints (engine, *track);

    const auto tempo = engine.getTempoMap();
    const double barLength = engine.getBarLength();
    const juce::ScopedLock lock (engine.getLock());

    for (auto& clip : track->clips)
    {
        if (clip->kind != Clip::Kind::midi)
            continue;

        auto* midi = static_cast<MidiClip*> (clip.get());

        for (auto& n : midi->notes)
        {
            gi::TrackNote t;
            t.pitch = n.pitch;
            t.startBeat = n.start + midi->startBeat;
            t.startSeconds = tempo.beatToSeconds (t.startBeat);
            t.lengthSeconds = tempo.beatToSeconds (t.startBeat + n.length) - t.startSeconds;
            t.velocity = juce::jlimit (1, 127, juce::roundToInt (n.velocity * 127.0f));
            t.bar = (int) std::floor (t.startBeat / barLength) + 1;
            c.part.notes.push_back (t);
            c.lengthBeats.push_back (n.length);
        }

        for (auto& event : midi->controllers)
            if (event.kind == controllers::modWheel || event.kind == controllers::volume || event.kind == controllers::expression)
                c.part.hasLoudnessController = true;
    }

    return c;
}

void InstrumentsExplainer::fillChoice (const Captured& c)
{
    updatingChoice = true;

    if (c.track != choiceTrack)
    {
        choiceTrack = c.track;
        choice.setSelectedId (1, juce::dontSendNotification);
    }

    juce::String text = "Guess (unknown)";
    const auto guess = gi::guessInstrument (c.hints);

    if (guess.instrument.has_value())
    {
        const char* from = guess.source == gi::GuessSource::sampler ? "its DawIt Sampler preset"
                         : guess.source == gi::GuessSource::name ? "its name" : "its DawIt sound";
        text = "Guess (" + gi::getInfo (*guess.instrument).name + ", from " + from + ")";
    }
    else if (! c.part.hasTrack)
        text = "Guess";

    // Selecting the same id again would leave the old text showing, so go through "none" first.
    const int shown = choice.getSelectedId();
    choice.changeItemText (1, text);
    choice.setSelectedId (0, juce::dontSendNotification);
    choice.setSelectedId (shown, juce::dontSendNotification);
    updatingChoice = false;
}

juce::String InstrumentsExplainer::getPrintSource() const
{
    const auto c = capture();
    return c.part.hasTrack ? "example pictures, and the track " + c.part.trackName : juce::String ("example pictures");
}

int InstrumentsExplainer::getNumPrintTopics() const
{
    return (int) std::size (gi::allTopics) + 1;
}

booklet::Topic InstrumentsExplainer::showPrintTopic (int index)
{
    if (GuideSourcesTopic::isRow (index, (int) std::size (gi::allTopics)))
        return sources.getPrintTopic();

    if (rowBeforePrinting < 0)
        rowBeforePrinting = shownRow;

    index = juce::jlimit (0, getNumPrintTopics() - 1, index);
    show (index);
    printHeight = picture->getPreferredHeight (100000);
    return { gi::getTopicName (gi::allTopics[index]), gi::getTopicSummary (gi::allTopics[index]), explanation.getText(),
             { picture->getWidth(), printHeight }, {} };
}

juce::Image InstrumentsExplainer::paintPrintPicture (float scale)
{
    picture->setSize (picture->getWidth(), printHeight);
    auto image = guideprint::snapshot (*picture, scale);
    resized();
    return image;
}

void InstrumentsExplainer::endPrinting()
{
    if (rowBeforePrinting >= 0)
        show (rowBeforePrinting);

    rowBeforePrinting = -1;
}

void InstrumentsExplainer::showTopic (gi::Topic topic)
{
    for (int i = 0; i < (int) std::size (gi::allTopics); ++i)
        if (gi::allTopics[i] == topic)
            list.selectRow (i);
}

int InstrumentsExplainer::getNumRows()
{
    return (int) std::size (gi::allTopics) + 1;
}

void InstrumentsExplainer::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (gi::allTopics)))
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

    const auto topic = gi::allTopics[row];
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (gi::getTopicName (topic), 10, 5, w - 16, 18, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (gi::getTopicSummary (topic), 10, 23, w - 16, 16, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRect (0, h - 1, w, 1);
}

void InstrumentsExplainer::selectedRowsChanged (int row)
{
    if (GuideSourcesTopic::isRow (row, (int) std::size (gi::allTopics)))
    {
        showSources();
        return;
    }

    if (row >= 0)
        show (row);
}

void InstrumentsExplainer::showSources()
{
    sources.select (*this, list, (int) std::size (gi::allTopics), { &list, &title, &exportButton });
}

void InstrumentsExplainer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Your track follows the selected track and its notes.
    if (! sources.isShown() && juce::isPositiveAndBelow (shownRow, getNumRows()) && gi::allTopics[shownRow] == gi::Topic::yours)
        refreshYours();
}

void InstrumentsExplainer::refreshYours()
{
    const auto c = capture();
    fillChoice (c);

    std::optional<gi::Instrument> chosen;

    if (choice.getSelectedId() >= 100)
        chosen = gi::getInstruments()[(size_t) (choice.getSelectedId() - 100)].instrument;

    const auto report = gi::analyse (c.part, chosen, c.hints);

    Picture::Yours y;
    y.notes = c.part.notes;
    y.lengthBeats = c.lengthBeats;
    y.instrument = report.instrument;
    y.message = c.part.hasTrack ? "This track has no notes yet." : "Select a MIDI track to see its notes against the instrument they're for.";
    picture->setYours (std::move (y));

    explanation.setText (gi::explain (gi::Topic::yours, report.text), false);
    explanation.moveCaretToTop (false);
}

void InstrumentsExplainer::show (int index)
{
    sources.close();
    stop();
    shownRow = juce::jlimit (0, getNumRows() - 1, index);
    const auto topic = gi::allTopics[shownRow];
    const bool yours = topic == gi::Topic::yours;

    choiceLabel.setVisible (yours);
    choice.setVisible (yours);

    if (yours)
    {
        refreshYours();
        playEvents.clear();
    }
    else
    {
        auto example = gi::getExample (topic);
        playBpm = example.bpm;
        playEvents.clear();
        playTotal = 0.0;
        const auto offsets = getLineOffsets (topic, example);

        for (size_t i = 0; i < example.lines.size(); ++i)
        {
            auto& line = example.lines[i];

            for (auto& n : line.notes)
            {
                playEvents.push_back ({ offsets[i] + n.start, n.pitch, (float) n.velocity / 127.0f, n.length, {} });
                playTotal = juce::jmax (playTotal, offsets[i] + n.start + n.length);
            }

            const auto addPoints = [&] (const std::vector<gi::ExamplePoint>& points, bool bend)
            {
                bool firstPoint = true;

                for (auto& p : points)
                {
                    const auto msg = bend ? juce::MidiMessage::pitchWheel (1, p.value) : juce::MidiMessage::controllerEvent (1, controllers::expression, p.value);
                    playEvents.push_back ({ firstPoint ? offsets[i] : offsets[i] + p.beat, -1, 0.0f, 0.0, msg });
                    firstPoint = false;
                }
            };

            addPoints (line.expression, false);
            addPoints (line.bend, true);
        }

        std::stable_sort (playEvents.begin(), playEvents.end(), [] (const PlayEvent& a, const PlayEvent& b) { return a.beat < b.beat; });
        engine.setPreviewSound (topic == gi::Topic::drumKit ? BuiltInSound::drums : BuiltInSound::piano);   // heard when no MIDI track is selected
        picture->setExample (topic, std::move (example));
        explanation.setText (gi::explain (topic), false);
        explanation.moveCaretToTop (false);
    }

    playButton.setEnabled (! playEvents.empty());
    resized();
}

//==============================================================================
void InstrumentsExplainer::play()
{
    if (playEvents.empty())
        return;

    playing = true;
    playStartMs = juce::Time::getMillisecondCounterHiRes() + 30.0;
    lastBeats = -1.0e-6;
    playButton.setButtonText ("Stop");
    startTimer (5);
}

void InstrumentsExplainer::stop()
{
    stopTimer();

    if (playing)
    {
        // Put the bend and the expression back where a track expects them.
        engine.previewMessage (juce::MidiMessage::pitchWheel (1, controllers::bendCentre));
        engine.previewMessage (juce::MidiMessage::controllerEvent (1, controllers::expression, 127));
    }

    playing = false;
    playButton.setButtonText ("Play");

    if (picture != nullptr)
        picture->setPlayBeat (-1.0);
}

void InstrumentsExplainer::timerCallback()
{
    const double msPerBeat = 60000.0 / juce::jmax (1.0, playBpm);
    const double now = (juce::Time::getMillisecondCounterHiRes() - playStartMs) / msPerBeat;

    if (now >= playTotal + 0.25)
    {
        stop();
        return;
    }

    if (now < 0.0)
        return;

    // Everything that starts between the last tick and this one sounds now.
    for (auto& e : playEvents)
        if (e.beat > lastBeats && e.beat <= now)
        {
            if (e.pitch >= 0)
                engine.previewNote (e.pitch, e.velocity, juce::jmax (40, juce::roundToInt (e.lengthBeats * msPerBeat * 0.9)));
            else
                engine.previewMessage (e.message);
        }

    lastBeats = now;
    picture->setPlayBeat (now);
}

//==============================================================================
void InstrumentsExplainer::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void InstrumentsExplainer::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    exportButton.setBounds (top.removeFromRight (120));
    top.removeFromRight (8);
    playButton.setBounds (top.removeFromRight (90));
    title.setBounds (top);
    r.removeFromTop (12);

    list.setBounds (r.removeFromLeft (250));
    r.removeFromLeft (14);

    if (sources.isShown())
    {
        sources.show (true, r);
        return;
    }

    if (choice.isVisible())
    {
        auto row = r.removeFromTop (28);
        choiceLabel.setBounds (row.removeFromLeft (90));
        choice.setBounds (row);
        r.removeFromTop (8);
    }

    const int maxHeight = juce::jmax (120, r.getHeight() * 11 / 20);
    const int pictureHeight = juce::jmin (picture->getPreferredHeight (maxHeight), r.getHeight() * 3 / 4);
    picture->setBounds (r.removeFromTop (pictureHeight));
    r.removeFromTop (10);
    explanation.setBounds (r);
}
} // namespace daw
