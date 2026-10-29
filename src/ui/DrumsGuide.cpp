#include "DrumsGuide.h"

#include <juce_dsp/juce_dsp.h>

namespace daw::drumtext
{
using drums::Drum;
using drums::Knob;
using drums::numDrums;

namespace
{
    constexpr double renderRate = 48000.0;
    constexpr double lowHz = 30.0, highHz = 16000.0;

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    juce::String formatDb (double db)
    {
        return (db >= 0.05 ? "+" : "") + juce::String (std::abs (db) < 0.05 ? 0.0 : db, 1) + " dB";
    }

    void drawCaption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& left, const juce::String& right = {})
    {
        auto line = r.removeFromTop (18.0f);
        g.setFont (juce::FontOptions (13.0f));
        g.setColour (theme::text);
        g.drawText (left, line, juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.drawText (right, line, juce::Justification::centredRight, true);
        r.removeFromTop (4.0f);
    }

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Colour c = theme::textDim,
                    juce::Justification justification = juce::Justification::centredLeft, float size = 11.0f)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size));
        g.drawFittedText (text, r.toNearestInt(), justification, 1, 0.8f);
    }

    double now()
    {
        return juce::Time::getMillisecondCounterHiRes() / 1000.0;
    }

    /** A key for a drum's sound, so a picture is worked out again only when the sound changes. */
    juce::String getKey (const drums::Sound& s, Drum d, double seconds, int width)
    {
        return juce::String ((int) d) + "|" + juce::String (s.tuneHz) + "|" + juce::String (s.bendSemitones) + "|" + juce::String (s.bendMs) + "|"
               + juce::String (s.decaySeconds) + "|" + juce::String (s.noisePercent) + "|" + juce::String (s.noiseDecaySeconds) + "|"
               + juce::String (s.toneHz) + "|" + juce::String (s.clickPercent) + "|" + juce::String (s.levelDb) + "|" + juce::String (seconds) + "|"
               + juce::String (width);
    }

    float toLogX (juce::Rectangle<float> plot, double hz)
    {
        return plot.getX() + plot.getWidth() * (float) (std::log (hz / lowHz) / std::log (highHz / lowHz));
    }

    void drawFrequencyGrid (juce::Graphics& g, juce::Rectangle<float> plot)
    {
        for (const double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        {
            const float x = toLogX (plot, hz);
            g.setColour (theme::grid);
            g.fillRect (x, plot.getY(), 1.0f, plot.getHeight());
            drawLabel (g, hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + "k" : juce::String ((int) hz),
                       juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ x, plot.getBottom() + 8.0f }), theme::textDim, juce::Justification::centred);
        }
    }

    /** How loud a part is, column by column across a hit: the peak in each column's stretch. */
    std::vector<float> getColumns (const std::vector<float>& samples, int columns)
    {
        std::vector<float> peaks ((size_t) juce::jmax (1, columns), 0.0f);
        const double perColumn = (double) samples.size() / (double) peaks.size();

        for (size_t c = 0; c < peaks.size(); ++c)
            for (size_t i = (size_t) (c * perColumn); i < (size_t) ((c + 1) * perColumn) && i < samples.size(); ++i)
                peaks[c] = juce::jmax (peaks[c], std::abs (samples[i]));

        return peaks;
    }

    /** A spectrum on the log line, one value (in dB) per pixel column, from the first 8192 samples of a part. */
    std::vector<float> getSpectrumColumns (const std::vector<float>& samples, juce::Rectangle<float> plot)
    {
        constexpr int order = 13, size = 1 << order;
        std::vector<float> data ((size_t) size * 2, 0.0f);

        for (int i = 0; i < size && i < (int) samples.size(); ++i)
            data[(size_t) i] = samples[(size_t) i] * (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (size - 1)));

        juce::dsp::FFT (order).performFrequencyOnlyForwardTransform (data.data());
        const int columns = juce::jmax (1, (int) plot.getWidth());
        std::vector<float> db ((size_t) columns, -200.0f);

        for (int c = 0; c < columns; ++c)
        {
            const double from = lowHz * std::pow (highHz / lowHz, (double) c / columns), to = lowHz * std::pow (highHz / lowHz, (double) (c + 1) / columns);
            const int first = (int) (from / renderRate * size), last = juce::jmax (first, (int) (to / renderRate * size));
            double power = 0.0;

            for (int bin = first; bin <= last && bin < size / 2; ++bin)
                power += (double) data[(size_t) bin] * data[(size_t) bin];

            const double amplitude = 4.0 * std::sqrt (power / (last - first + 1)) / size;
            db[(size_t) c] = (float) juce::Decibels::gainToDecibels (amplitude, -200.0);
        }

        return db;
    }

    double getPeakDb (const drums::Settings& s, Drum d)
    {
        const auto hit = drums::render (s, d, renderRate, juce::jmin (0.5, drums::getLength (s.drums[(size_t) d], d)));
        float peak = 0.0f;

        for (size_t i = 0; i < hit.tone.size(); ++i)
            peak = juce::jmax (peak, std::abs (hit.tone[i] + hit.noise[i]));

        return juce::Decibels::gainToDecibels (peak, -100.0f) + s.outputDb;
    }

    /** The nearest note to a frequency and how far off it is, in cents. */
    std::pair<int, double> getNearestNote (double hz)
    {
        const double exact = 69.0 + 12.0 * std::log2 (juce::jmax (1.0, hz) / 440.0);
        const int note = juce::roundToInt (exact);
        return { note, (exact - note) * 100.0 };
    }
}

juce::Colour getDrumColour (Drum d)
{
    switch (d)
    {
        case Drum::kick:      return juce::Colour (0xffff6b6b);
        case Drum::snare:     return juce::Colour (0xffffa94d);
        case Drum::clap:      return juce::Colour (0xffffd43b);
        case Drum::lowTom:    return juce::Colour (0xff69db7c);
        case Drum::highTom:   return juce::Colour (0xff38d9a9);
        case Drum::closedHat: return juce::Colour (0xff4dabf7);
        case Drum::openHat:   return juce::Colour (0xff74c0fc);
        case Drum::cymbal:    return juce::Colour (0xffa5d8ff);
        case Drum::cowbell:   return juce::Colour (0xffda77f2);
    }

    return colour;
}

juce::String getNoteName (int note)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1);
}

//==============================================================================
juce::String getTitle (int t)
{
    switch (t)
    {
        case (int) Knob::tune:       return "Tune";
        case (int) Knob::bend:       return "Bend";
        case (int) Knob::bendTime:   return "Bend time";
        case (int) Knob::decay:      return "Decay";
        case (int) Knob::noise:      return "Noise";
        case (int) Knob::noiseDecay: return "Noise decay";
        case (int) Knob::tone:       return "Tone: the noise's filter";
        case (int) Knob::click:      return "Click";
        case (int) Knob::level:      return "Level";
        case (int) Knob::choke:      return "Choke";
        case (int) Knob::velocity:   return "Velocity";
        case (int) Knob::output:     return "Output";
        case pads:                   return "The pads";
        case hit:                    return "One hit over time";
        case spectrum:               return "The partials of a hit";
        case selectDrum:             return "Which drum";
        case fxui::overview:         return "DawIt Drums";
        case fxui::presets:          return "Kits";
        case fxui::clap:             return "Play a hit";
        case fxui::listen:           return "Hear the drum alone";
        case fxui::guide:            return "What is drum synthesis?";
        case fxui::history:          return "The last 4 seconds";
        default:                     return {};
    }
}

juce::String describe (int t)
{
    switch (t)
    {
        case (int) Knob::tune:
            return "The pitch of the drum's tone. For a kick or a tom it's where the pitch settles once the bend is over: a kick usually "
                   "sits between 40 and 70 Hz, below most bass notes. For the hi-hats, the cymbal and the cowbell it moves all their "
                   "square waves together, keeping their clashing ratios, so the metal sounds smaller or bigger.";

        case (int) Knob::bend:
            return "How far above its tune a hit starts. A real drumhead is tighter for a moment when it's struck hard, so its pitch "
                   "starts high and falls; a synthesised kick does the same on purpose. +24 semitones is two octaves: an 808 kick starts "
                   "at four times its tune. The fall is heard as punch, not as a pitch: until the bend gets slow (see Bend time).";

        case (int) Knob::bendTime:
            return "How fast the pitch falls back: about two thirds of the way in this time, nearly all of it in three times as long. "
                   "Short (10-40 ms): a punchy thump. Longer (80-200 ms): the fall is slow enough to hear as a slide, the \"pew\" of a "
                   "Syndrum or the zap of an electro kick.";

        case (int) Knob::decay:
            return "How long the tone rings: the time it takes to fade to -60 dB, about silent. It falls by the same number of dB every "
                   "moment, as a real drum does: a third of the decay in, it's already 20 dB down. A tight kick: 0.2-0.4 s; an 808 boom: "
                   "1-3 s; a closed hi-hat: 30-80 ms.";

        case (int) Knob::noise:
            return "How much of the hit is noise rather than tone. Noise has every frequency at once and no pitch: the rattle of a "
                   "snare's wires, the hiss of a cymbal, the breath of a brush. A snare is about half tone, half noise; a kick has none; "
                   "a clap is nothing else.";

        case (int) Knob::noiseDecay:
            return "How long the noise rings, to -60 dB, apart from the tone. A snare's wires keep rattling a little after its head has "
                   "stopped; an open hi-hat's hiss hangs on. For the clap, it's the tail after its three bursts.";

        case (int) Knob::tone:
            return "Where the noise's filter sits, and for the metal drums the metal's too. The snare and the cymbals keep what's above "
                   "it (a high-pass): higher is thinner and brighter. The clap and the cowbell keep a band around it (a band-pass): it "
                   "sets where they ring. The kick and the toms keep what's below it (a low-pass), for a darker thud.";

        case (int) Knob::click:
            return "A burst of noise under 2 ms long at the very start: the beater hitting the skin, the stick on the head. It's what "
                   "makes a kick audible on a phone speaker, which can't play its low notes, and what cuts through a loud mix.";

        case (int) Knob::level:
            return "How loud this drum is in the kit. Balancing the drums is most of programming a beat: the kick and the snare up "
                   "front, the hi-hats a few dB under them.";

        case (int) Knob::choke:
            return "A hi-hat is two cymbals on a stand: an open hi-hat rings until the drummer's foot closes them. With choke on, a "
                   "closed hi-hat (or a new hit on the hi-hat) cuts off the one still ringing, in 5 ms, the way drum machines have done "
                   "since the 808. Off, they ring over each other.";

        case (int) Knob::velocity:
            return "How much playing softer makes a hit quieter. Velocity is how hard a note was played (0 to 127 in MIDI): the "
                   "accents and ghost notes that make a beat groove. At 100% a hit played half as hard is 6 dB quieter; at 0% every hit "
                   "is the same, like an early drum machine.";

        case (int) Knob::output:
            return "The kit's level going into the track's effects and fader. Drums are the peakiest part of a mix, so leave headroom: "
                   "a kick and a snare peaking around -8 dB leave room for the hits that land together.";

        case pads:
            return "The nine drums, each with the note that plays it on the General MIDI drum map, the one MIDI files and most drum "
                   "software use: 36 (C2) the kick, 38 the snare, 42 the closed hi-hat... Click a pad to play it and show its knobs "
                   "below; it lights as it sounds. Right-click for the guide.";

        case hit:
            return "One hit of the selected drum over its whole length, left to right: how loud the tone is (green) and the noise "
                   "(amber), mirrored up and down like a waveform, and the pitch (blue) when the drum bends. Watch the bend: the pitch "
                   "falls fast and settles, while the level fades evenly. The white line is the newest hit.";

        case spectrum:
            return "The frequencies in the first 170 ms of a hit, from 30 Hz on the left to 16 kHz on the right: the tone (green) as "
                   "peaks at its pitches, and the noise (amber) as a broad band shaped by the Tone knob's filter. A kick lives under 100 "
                   "Hz, a snare's tone around 200 Hz and its noise above, the hi-hats above 7 kHz.";

        case selectDrum:
            return "Which drum the knobs below belong to. Click to step through the nine, or click a pad.";

        case fxui::clap:
            return "Plays the selected drum, even while the song is stopped. Or play the track's MIDI notes, a MIDI file's drums, or a "
                   "MIDI keyboard: C2 is the kick, D2 the snare, F#2 the closed hi-hat (some DAWs call them C1, D1 and F#1, counting middle C as C3).";

        case fxui::listen:
            return "Plays only the selected drum, whatever the track plays. Use it to tune a drum while the beat runs.";

        case fxui::presets:
            return "Whole kits to start from, each telling why it sounds the way it does and what matters in each drum.";

        case fxui::guide:
            return "A guide for beginners: drum machines and how they make drums from oscillators and noise, the kick, the snare, "
                   "hi-hats and cymbals, the clap, toms and cowbell, decay and choke, velocity and groove, tuning drums to the song, "
                   "and where each drum sits.";

        case fxui::history:
            return "The last 4 seconds: grey how loud the selected drum was, coloured the whole kit.";

        default:
            return {};
    }
}

juce::String getTypical (int t)
{
    switch (t)
    {
        case (int) Knob::tune:       return "Kick 40-70 Hz; snare 150-250 Hz; toms 80-250 Hz; the metal 150-400 Hz for its lowest square.";
        case (int) Knob::bend:       return "Kick +12 to +36 st; snare and toms +3 to +10; off for metal.";
        case (int) Knob::bendTime:   return "10-40 ms for a punchy kick; 60-150 ms for toms that slide.";
        case (int) Knob::decay:      return "Kick 0.3-0.8 s, an 808 boom 1-3 s; snare tone 0.1-0.2 s; closed hi-hat 30-80 ms, open 0.3-0.6 s, cymbal 1-3 s.";
        case (int) Knob::noise:      return "Kick 0; toms 0-15%; snare 50-70%; hi-hats 20-70%; clap all.";
        case (int) Knob::noiseDecay: return "Snare 0.15-0.3 s; clap tail 0.2-0.6 s; hi-hats like their decay.";
        case (int) Knob::tone:       return "Snare 1-3 kHz; clap 1-1.5 kHz; hi-hats 6-10 kHz; cymbal 4-8 kHz.";
        case (int) Knob::click:      return "Kick 20-70%; snare 10-30%; the rest 0.";
        case (int) Knob::level:      return "Kick and snare at 0 dB; hi-hats -4 to -8 dB; percussion lower still.";
        case (int) Knob::choke:      return "On, as a real hi-hat behaves.";
        case (int) Knob::velocity:   return "40% for machine-like beats; 70-100% for played, human ones.";
        case (int) Knob::output:     return "0 dB, with the kick peaking around -8 dB.";
        default:                     return {};
    }
}

juce::String describeDrum (const drums::Settings& s, Drum d)
{
    const auto& o = s.drums[(size_t) d];
    const juce::String name = drums::getDrumName (d) + ": ";
    const juce::String fade = ", -60 dB in " + drums::formatKnob (Knob::decay, s, d);

    switch (drums::getBody (d))
    {
        case drums::Body::sine:
        case drums::Body::head:
            return name + (o.bendSemitones > 0.05f ? "a tone falling from " + formatHz (drums::getPitchAt (o, 0.0)) + " to " + formatHz (o.tuneHz)
                                                   : "a tone at " + formatHz (o.tuneHz))
                   + fade + (o.noisePercent > 0.5f ? ", with " + drums::formatKnob (Knob::noise, s, d) + " noise" : juce::String());

        case drums::Body::metal:
            return name + "six square waves from " + formatHz (o.tuneHz) + " to " + formatHz (o.tuneHz * drums::getMetalRatios()[5]) + fade;

        case drums::Body::bell:
            return name + "two square waves at " + formatHz (o.tuneHz) + " and " + formatHz (o.tuneHz * drums::bellRatio) + fade;

        case drums::Body::none:
            return name + "three bursts of noise around " + formatHz (o.toneHz) + ", then a tail of " + drums::formatKnob (Knob::noiseDecay, s, d);
    }

    return name;
}

juce::String describeSettings (int t, const drums::Settings& s, Drum d)
{
    const auto& o = s.drums[(size_t) d];
    const auto name = drums::getDrumName (d);
    const auto body = drums::getBody (d);

    if (drums::isDrumKnob ((Knob) juce::jlimit (0, drums::numKnobs - 1, t)) && ! drums::isUsed (d, (Knob) t))
    {
        if (body == drums::Body::none)
            return "The clap has no tone, only noise, so this knob does nothing for it.";

        return "The " + name.toLowerCase() + "'s metal doesn't bend, so this knob does nothing for it.";
    }

    switch (t)
    {
        case (int) Knob::tune:
        {
            if (body == drums::Body::metal)
            {
                juce::StringArray hz;

                for (auto ratio : drums::getMetalRatios())
                    hz.add (juce::String (juce::roundToInt (o.tuneHz * ratio)));

                return "The six squares run at " + hz.joinIntoString (", ") + " Hz: none a whole-number multiple of another, so they clash like metal.";
            }

            if (body == drums::Body::bell)
                return "The two squares run at " + formatHz (o.tuneHz) + " and " + formatHz (o.tuneHz * drums::bellRatio) + ".";

            const auto [note, cents] = getNearestNote (o.tuneHz);
            return "The " + name.toLowerCase() + " settles at " + formatHz (o.tuneHz) + ", " + (std::abs (cents) < 10.0 ? juce::String ("close to ") : juce::String ("between notes, near "))
                   + getNoteName (note) + " (" + juce::String (440.0 * std::pow (2.0, (note - 69) / 12.0), 1) + " Hz).";
        }

        case (int) Knob::bend:
        case (int) Knob::bendTime:
            if (o.bendSemitones < 0.05f)
                return "No bend: the pitch stays at " + formatHz (o.tuneHz) + " from the start.";

            return "A hit starts at " + formatHz (drums::getPitchAt (o, 0.0)) + " (" + drums::formatKnob (Knob::bend, s, d) + "), is at "
                   + formatHz (drums::getPitchAt (o, o.bendMs / 1000.0)) + " after " + drums::formatKnob (Knob::bendTime, s, d) + " and at "
                   + formatHz (drums::getPitchAt (o, 3.0 * o.bendMs / 1000.0)) + " after three times as long.";

        case (int) Knob::decay:
            return "The tone fades to -60 dB in " + drums::formatKnob (Knob::decay, s, d) + ": -20 dB after " + fxui::formatSeconds (o.decaySeconds / 3.0) + ".";

        case (int) Knob::noise:
            return body == drums::Body::none ? juce::String ("The clap is all noise.")
                                             : drums::formatKnob (Knob::noise, s, d) + " noise and " + juce::String (juce::roundToInt (100.0f - o.noisePercent)) + "% tone.";

        case (int) Knob::noiseDecay:
            return (body == drums::Body::none ? "After its three bursts, the tail fades to -60 dB in " : "The noise fades to -60 dB in ")
                   + drums::formatKnob (Knob::noiseDecay, s, d) + (o.noisePercent < 0.5f && body != drums::Body::none ? ", but the noise is at 0%." : ".");

        case (int) Knob::tone:
            switch (drums::getFilter (d))
            {
                case drums::Filter::lowPass:  return "A low-pass at " + formatHz (o.toneHz) + ": the noise keeps what's below it.";
                case drums::Filter::bandPass: return "A band-pass around " + formatHz (o.toneHz) + ": the " + name.toLowerCase() + " rings in a band there.";
                case drums::Filter::highPass: return "A high-pass at " + formatHz (o.toneHz) + ": the " + name.toLowerCase() + " keeps what's above it.";
            }

            return {};

        case (int) Knob::click:
            return o.clickPercent < 0.5f ? juce::String ("No click.") : "A click of " + drums::formatKnob (Knob::click, s, d) + " at the start, gone in about 2 ms.";

        case (int) Knob::level:
            return drums::formatKnob (Knob::level, s, d) + ": the " + name.toLowerCase() + " alone peaks at about " + formatDb (getPeakDb (s, d)) + " at full velocity.";

        case (int) Knob::choke:
            return s.choke ? "On: a closed hi-hat cuts off an open one." : "Off: the hi-hats ring over each other.";

        case (int) Knob::velocity:
            return "A hit played half as hard is " + formatDb (juce::Decibels::gainToDecibels (drums::getVelocityScale (s, 0.5f))) + ".";

        case (int) Knob::output:
            return "The kick at full velocity peaks at about " + formatDb (getPeakDb (s, Drum::kick)) + ".";

        default:
            return {};
    }
}

//==============================================================================
std::optional<Drum> getPadAt (juce::Rectangle<float> r, juce::Point<float> p)
{
    if (! r.contains (p))
        return std::nullopt;

    const int column = juce::jlimit (0, 2, (int) ((p.x - r.getX()) / (r.getWidth() / 3.0f)));
    const int row = juce::jlimit (0, 2, (int) ((p.y - r.getY()) / (r.getHeight() / 3.0f)));
    return (Drum) (row * 3 + column);
}

void drawPads (juce::Graphics& g, juce::Rectangle<float> r, Drum selected, const std::array<float, numDrums>* levels, bool detailed)
{
    const float w = r.getWidth() / 3.0f, h = r.getHeight() / 3.0f;

    for (int i = 0; i < numDrums; ++i)
    {
        const auto d = (Drum) i;
        const auto c = getDrumColour (d);
        const auto pad = juce::Rectangle<float> (r.getX() + w * (float) (i % 3), r.getY() + h * (float) (i / 3), w, h).reduced (5.0f);
        const float lit = levels != nullptr ? juce::jlimit (0.0f, 1.0f, std::pow ((*levels)[(size_t) i], 0.5f)) : 0.0f;

        g.setColour (theme::panel.brighter (0.05f));
        g.fillRoundedRectangle (pad, 8.0f);
        g.setColour (c.withAlpha (0.12f + 0.55f * lit));
        g.fillRoundedRectangle (pad, 8.0f);
        g.setColour (d == selected ? theme::text : c.withAlpha (0.6f));
        g.drawRoundedRectangle (pad, 8.0f, d == selected ? 2.2f : 1.0f);

        auto inside = pad.reduced (8.0f, 6.0f);
        g.setColour (c);
        g.setFont (juce::FontOptions (juce::jlimit (10.0f, 15.0f, pad.getHeight() * 0.2f), juce::Font::bold));
        g.drawFittedText (drums::getDrumName (d), inside.removeFromTop (juce::jlimit (12.0f, 20.0f, pad.getHeight() * 0.28f)).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.8f);

        if (detailed)
            drawLabel (g, juce::String (drums::getNote (d)) + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")) + getNoteName (drums::getNote (d)),
                       inside.removeFromTop (14.0f));
    }
}

void drawHit (juce::Graphics& g, juce::Rectangle<float> r, const drums::Settings& s, Drum d, double age, double seconds)
{
    const auto& o = s.drums[(size_t) d];
    const double length = seconds > 0.0 ? seconds : juce::jlimit (0.05, 3.0, drums::getLength (o, d));
    auto plot = r.withTrimmedBottom (14.0f).withTrimmedRight (drums::isUsed (d, Knob::bend) ? 46.0f : 0.0f);
    const int columns = juce::jmax (1, (int) plot.getWidth());

    // Working a hit out takes a moment, so the last one is kept for as long as the sound stays the same.
    static juce::String lastKey;
    static std::vector<float> toneColumns, noiseColumns;
    const auto key = getKey (o, d, length, columns) + "|" + juce::String (s.velocityPercent);

    if (key != lastKey)
    {
        const auto rendered = drums::render (s, d, renderRate, length);
        toneColumns = getColumns (rendered.tone, columns);
        noiseColumns = getColumns (rendered.noise, columns);
        lastKey = key;
    }

    float loudest = 0.05f;

    for (size_t c = 0; c < toneColumns.size(); ++c)
        loudest = juce::jmax (loudest, toneColumns[c], noiseColumns[c]);

    g.setColour (theme::text.withAlpha (0.12f));
    g.fillRect (plot.getX(), plot.getCentreY(), plot.getWidth(), 1.0f);

    auto drawPart = [&] (const std::vector<float>& part, juce::Colour c)
    {
        juce::Path shape;
        shape.startNewSubPath (plot.getX(), plot.getCentreY());

        for (size_t i = 0; i < part.size(); ++i)
            shape.lineTo (plot.getX() + (float) i, plot.getCentreY() - plot.getHeight() * 0.48f * part[i] / loudest);

        for (size_t i = part.size(); i-- > 0;)
            shape.lineTo (plot.getX() + (float) i, plot.getCentreY() + plot.getHeight() * 0.48f * part[i] / loudest);

        shape.closeSubPath();
        g.setColour (c.withAlpha (0.45f));
        g.fillPath (shape);
        g.setColour (c);
        g.strokePath (shape, juce::PathStrokeType (1.0f));
    };

    drawPart (noiseColumns, noiseColour);
    drawPart (toneColumns, toneColour);

    // The pitch, on its own scale on the right.
    if (drums::isUsed (d, Knob::bend) && o.bendSemitones > 0.05f)
    {
        const double top = drums::getPitchAt (o, 0.0), bottom = o.tuneHz * 0.8;
        auto toY = [&plot, top, bottom] (double hz) { return plot.getBottom() - plot.getHeight() * (float) (std::log (hz / bottom) / std::log (top / bottom)); };
        juce::Path pitch;

        for (int x = 0; x <= columns; x += 2)
        {
            const float y = toY (drums::getPitchAt (o, length * x / columns));
            x == 0 ? pitch.startNewSubPath (plot.getX(), y) : pitch.lineTo (plot.getX() + (float) x, y);
        }

        g.setColour (pitchColour);
        g.strokePath (pitch, juce::PathStrokeType (2.0f));
        drawLabel (g, formatHz (top), { plot.getRight() + 4.0f, toY (top) - 2.0f, 44.0f, 13.0f }, pitchColour);
        drawLabel (g, formatHz (o.tuneHz), { plot.getRight() + 4.0f, toY (o.tuneHz) - 13.0f, 44.0f, 13.0f }, pitchColour);
    }

    if (age >= 0.0 && age <= length)
    {
        g.setColour (juce::Colours::white);
        g.fillRect (plot.getX() + plot.getWidth() * (float) (age / length), plot.getY(), 1.5f, plot.getHeight());
    }

    drawLabel (g, "0 s", { plot.getX(), plot.getBottom() + 1.0f, 40.0f, 13.0f });
    drawLabel (g, fxui::formatSeconds (length), { plot.getRight() - 60.0f, plot.getBottom() + 1.0f, 60.0f, 13.0f }, theme::textDim,
               juce::Justification::centredRight);
}

void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r, const drums::Settings& s, Drum d)
{
    auto plot = r.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f);
    const auto& o = s.drums[(size_t) d];

    static juce::String lastKey;
    static std::vector<float> tone, noise;
    const auto key = getKey (o, d, 0.0, (int) plot.getWidth());

    if (key != lastKey)
    {
        const auto rendered = drums::render (s, d, renderRate, 8192.0 / renderRate);
        tone = getSpectrumColumns (rendered.tone, plot);
        noise = getSpectrumColumns (rendered.noise, plot);
        lastKey = key;
    }

    float top = -200.0f;

    for (size_t i = 0; i < tone.size(); ++i)
        top = juce::jmax (top, tone[i], noise[i]);

    top = std::ceil (top / 10.0f) * 10.0f;
    auto toY = [&plot, top] (float db) { return juce::jmap (juce::jlimit (top - 60.0f, top, db), top - 60.0f, top, plot.getBottom(), plot.getY()); };

    drawFrequencyGrid (g, plot);

    for (int i = 0; i <= 2; ++i)
    {
        const float db = top - 20.0f * (float) i;
        g.setColour (theme::grid);
        g.fillRect (plot.getX(), toY (db), plot.getWidth(), 1.0f);
        drawLabel (g, juce::String (-20 * i), { r.getX(), toY (db) - 7.0f, 26.0f, 14.0f }, theme::textDim, juce::Justification::centredRight);
    }

    auto drawPart = [&] (const std::vector<float>& part, juce::Colour c)
    {
        juce::Path fill;
        fill.startNewSubPath (plot.getX(), plot.getBottom());

        for (size_t i = 0; i < part.size(); ++i)
            fill.lineTo (plot.getX() + (float) i, toY (part[i]));

        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.closeSubPath();
        g.setColour (c.withAlpha (0.3f));
        g.fillPath (fill);
        g.setColour (c);
        g.strokePath (fill, juce::PathStrokeType (1.2f));
    };

    if (drums::getBody (d) == drums::Body::none || o.noisePercent > 0.5f || o.clickPercent > 0.5f)
        drawPart (noise, noiseColour);

    if (drums::getBody (d) != drums::Body::none && o.noisePercent < 99.5f)
        drawPart (tone, toneColour);
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInDrums& synth)
{
    BuiltInDrums* processor = &synth;
    std::vector<fxui::Guide::Page> pages;

    pages.push_back ({ "Drums from oscillators and noise", "What drum synthesis is",
        "A drum machine plays drums without a drummer. The first ones, such as Wurlitzer's Sideman (1959), made their sounds with "
        "simple circuits; Roland's CR-78 (1978) and above all the TR-808 (1980) made every drum from scratch with the same parts as "
        "a synthesiser: oscillators for the tone, noise for the hiss and rattle, envelopes for how each hit dies away. The 808 sold "
        "poorly at first, because it didn't sound like real drums. Then hip-hop, electro and pop made its booming kick and snappy "
        "clap their own, and it never left.\n\n"
        "The TR-909 (1983) kept synthesised kicks, snares and toms but played recorded cymbals, and became the sound of house and "
        "techno. Machines such as the LinnDrum (1982) went all the way to recordings (samples). Synthesised drums stayed because "
        "every part can be tuned and shaped: a kick can be a long bass note, a snare can be all crack or all body.\n\n"
        "Every drum here is made of three things: a tone (a sine, or square waves that clash like metal), noise, and how both fade. "
        "Learn the kick and the snare and the rest follow.\n\n"
        "The picture shows the nine drums of the window's kit, each one hit: the tone green, the noise amber.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The window's kit, one hit each", "green: the tone; amber: the noise");
            const float w = r.getWidth() / 3.0f, h = r.getHeight() / 3.0f;

            for (int i = 0; i < numDrums; ++i)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) (i % 3), r.getY() + h * (float) (i / 3), w, h).reduced (6.0f, 4.0f);
                drawLabel (g, drums::getDrumName ((Drum) i), cell.removeFromTop (15.0f), getDrumColour ((Drum) i), juce::Justification::centredLeft, 12.0f);

                // Each cell keeps its picture until its drum changes, so it isn't worked out again for every frame.
                static std::array<juce::String, numDrums> keys;
                static std::array<juce::Image, numDrums> cache;
                const auto key = getKey (s.drums[(size_t) i], (Drum) i, 0.0, (int) cell.getWidth()) + juce::String ((int) cell.getHeight());

                if (keys[(size_t) i] != key || ! cache[(size_t) i].isValid())
                {
                    cache[(size_t) i] = juce::Image (juce::Image::ARGB, juce::jmax (1, (int) cell.getWidth()), juce::jmax (1, (int) cell.getHeight()), true);
                    juce::Graphics cg (cache[(size_t) i]);
                    drawHit (cg, cache[(size_t) i].getBounds().toFloat(), s, (Drum) i, -1.0);
                    keys[(size_t) i] = key;
                }

                g.drawImageAt (cache[(size_t) i], (int) cell.getX(), (int) cell.getY());
            }
        } });

    pages.push_back ({ "The kick: a sine that falls", "Punch and boom",
        "Strike a big drum and its skin is stretched tighter for an instant, so it starts higher and falls to its own pitch as it "
        "settles. A synthesised kick does exactly that, and nothing else: one sine that starts high and drops fast.\n\n"
        "Bend is how far above its tune it starts: an 808 kick at +24 semitones begins two octaves up, at four times its pitch. "
        "Bend time is how fast it falls. Over 10-40 ms the fall is too quick to hear as a pitch; you hear it as punch. Decay is how "
        "long the low sine rings afterwards: the boom. A tight rock or pop kick lasts a few tenths of a second; a trap 808 rings for "
        "seconds and plays the bass line.\n\n"
        "Click adds a burst of noise under 2 ms long at the start: the beater on the skin. Small speakers can't play 50 Hz, so on a "
        "phone the click and the start of the bend are what you hear of the kick.\n\n"
        "The picture shows the window's kick: its level (green) fading evenly and its pitch (blue) falling fast and settling.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The window's kick", describeSettings ((int) Knob::bend, s, Drum::kick));
            drawHit (g, r, s, Drum::kick, std::fmod (now(), 1.5), -1.0);
        } });

    pages.push_back ({ "The snare: a tone and noise", "The head and the wires",
        "A snare drum has two sounds at once. Its head rings with a short tone, around 150 to 250 Hz. Under the bottom head, a set "
        "of wires (the snares) buzz against it and add a burst of noise. Synthesising one means adding those two.\n\n"
        "The tone here is the head's first two modes. A drumhead doesn't vibrate like a string, whose overtones are whole-number "
        "multiples: an ideal round membrane's second mode is at 1.59 times its first. That's why drums sound like drums and not "
        "like notes.\n\n"
        "Noise is how much of the hit is the wires. Noise decay lets them ring on after the head has stopped. Tone sets the high-pass "
        "on the noise: lower is a fat, papery snare, higher a thin crack.\n\n"
        "The picture shows the window's snare: the tone and the noise of one hit over time, and below, where each sits in frequency "
        "(in dB under the loudest).",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The window's snare", "green: the head's tone; amber: the wires");
            auto top = r.removeFromTop (r.getHeight() * 0.5f);
            drawHit (g, top, s, Drum::snare, std::fmod (now(), 1.0), -1.0);
            r.removeFromTop (10.0f);
            drawSpectrum (g, r, s, Drum::snare);
        } });

    pages.push_back ({ "Hi-hats and cymbals: metal", "Six square waves that clash",
        "A cymbal is a metal plate, and a plate's modes are nowhere near a harmonic series: hundreds of partials that don't line up, "
        "which the ear hears as a shimmer without a pitch.\n\n"
        "The TR-808 faked it cheaply and brilliantly: six square waves at 205.3, 304.4, 369.6, 522.7, 540 and 800 Hz. None is a "
        "whole-number multiple of another, and each square has its own odd harmonics, so together they make a dense, clashing set "
        "of partials. A high-pass filter keeps only the top of it, where the metal shimmers, and an envelope makes it a closed "
        "hi-hat (60 ms), an open one (half a second) or a cymbal (two seconds). Noise adds hiss; more noise sounds more like a "
        "recorded cymbal.\n\n"
        "Tune moves all six together, keeping their clash: lower is a bigger, darker plate.\n\n"
        "The picture shows the six squares and their first harmonics on a line of frequencies, and the spectrum of the window's "
        "closed hi-hat, after its filter.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const auto& hat = s.drums[(size_t) Drum::closedHat];
            drawCaption (g, r, "Six squares from " + formatHz (hat.tuneHz), "ticks: each square and its odd harmonics");
            auto top = r.removeFromTop (r.getHeight() * 0.32f).withTrimmedLeft (30.0f).withTrimmedBottom (16.0f);
            drawFrequencyGrid (g, top);
            const auto& ratios = drums::getMetalRatios();

            for (size_t i = 0; i < ratios.size(); ++i)
            {
                const auto c = getDrumColour (Drum::closedHat).withRotatedHue ((float) i * 0.09f);

                for (int n = 1; n <= 15; n += 2)
                {
                    const double hz = hat.tuneHz * ratios[i] * n;

                    if (hz > highHz)
                        break;

                    const float height = top.getHeight() * (n == 1 ? 1.0f : 0.8f / (float) n + 0.15f);
                    g.setColour (c.withAlpha (n == 1 ? 1.0f : 0.6f));
                    g.fillRect (toLogX (top, hz) - 1.0f, top.getBottom() - height, 2.0f, height);
                }
            }

            r.removeFromTop (12.0f);
            drawSpectrum (g, r, s, Drum::closedHat);
        } });

    pages.push_back ({ "The clap: hands not quite together", "Bursts of noise and a tail",
        "A handclap is noise: no pitch at all, a slap of air in a band around 1 kHz. A crowd clapping is many slaps a few "
        "milliseconds apart, and that blur is what makes a clap sound like a clap and not like a snare.\n\n"
        "The 808 made it with noise through a band-pass filter and an envelope that fires several times in quick succession, then "
        "a longer tail like a room's reverb. Here: three bursts 11 ms apart, then the tail. Tone sets the band (lower is thicker, "
        "higher is thinner), and noise decay how long the tail rings.\n\n"
        "Claps often play with the snare on beats 2 and 4, or replace it, as in much of trap and house.\n\n"
        "The picture shows the window's clap: the three bursts and the tail.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The window's clap", "the first 80 ms, then the whole tail below");
            auto top = r.removeFromTop (r.getHeight() * 0.5f);
            drawHit (g, top, s, Drum::clap, -1.0, 0.08);
            r.removeFromTop (10.0f);
            drawHit (g, r, s, Drum::clap, std::fmod (now(), 1.0), -1.0);
        } });

    pages.push_back ({ "Toms and the cowbell", "Pitched drums",
        "Toms are kicks with a higher tune: a sine that bends down a little and rings. A low and a high tom a few semitones apart "
        "make the fills that lead into a chorus. With a long bend time, the falling pitch is heard as a slide: the \"pew\" of the "
        "Syndrum and Simmons drums of the late 1970s and 80s (try the Electro kit).\n\n"
        "The cowbell is the 808's oddest and most famous sound: just two of its square waves, at 540 and 800 Hz, through a "
        "band-pass, with a quick decay. The two notes are about a sixth apart and slightly out of tune with each other, which is "
        "what makes it clank.\n\n"
        "The picture shows the window's two toms and the cowbell's spectrum, with its two squares and their harmonics.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The window's toms and cowbell", "blue: the toms' pitch");
            auto top = r.removeFromTop (r.getHeight() * 0.5f);
            auto left = top.removeFromLeft (top.getWidth() * 0.5f).reduced (4.0f, 0.0f);
            drawLabel (g, "Low tom", left.removeFromTop (14.0f), getDrumColour (Drum::lowTom));
            drawHit (g, left, s, Drum::lowTom, -1.0);
            top = top.reduced (4.0f, 0.0f);
            drawLabel (g, "High tom", top.removeFromTop (14.0f), getDrumColour (Drum::highTom));
            drawHit (g, top, s, Drum::highTom, -1.0);
            r.removeFromTop (10.0f);
            drawLabel (g, "Cowbell", r.removeFromTop (14.0f), getDrumColour (Drum::cowbell));
            r.removeFromTop (8.0f);
            drawSpectrum (g, r, s, Drum::cowbell);
        } });

    pages.push_back ({ "Decay and choke", "How long each drum rings",
        "Decay is the most-used knob on a drum machine. Each part fades by the same number of dB every moment, as real drums do, and "
        "the decay is the time to reach -60 dB, about silent. A third of the way in it's already 20 dB down, so a long decay mostly "
        "adds a quiet tail.\n\n"
        "Short decays make a tight, dry beat with room for the bass and the vocal; long ones fill the space. Within one beat, "
        "short and long drums answer each other: closed hi-hats ticking, an open one breathing on the off-beat.\n\n"
        "A real hi-hat can't ring open and closed at once: the drummer's foot closes the two cymbals. Choke does the same: a closed "
        "hi-hat cuts off an open one in 5 ms.\n\n"
        "The picture plays the window's open hi-hat, cut off by a closed one after 150 ms (above), and with the choke off (below).",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            const double length = juce::jlimit (0.2, 2.0, drums::getLength (s.drums[(size_t) Drum::openHat], Drum::openHat));
            drawCaption (g, r, "An open hi-hat, then a closed one at 150 ms", "the open one's length: " + fxui::formatSeconds (length));
            const float h = (r.getHeight() - 30.0f) / 2.0f;

            for (int i = 0; i < 2; ++i)
            {
                auto box = r.removeFromTop (h);
                r.removeFromTop (14.0f);
                drawLabel (g, i == 0 ? "choke on: cut off" : "choke off: both ring", box.removeFromTop (14.0f), i == 0 ? colour : theme::textDim);

                static std::array<std::vector<float>, 2> columns;
                static std::array<juce::String, 2> keys;
                const auto key = getKey (s.drums[(size_t) Drum::openHat], Drum::openHat, length, (int) box.getWidth())
                                 + getKey (s.drums[(size_t) Drum::closedHat], Drum::closedHat, 0.0, 0);

                if (keys[(size_t) i] != key)
                {
                    BuiltInDrums machine;
                    auto settings = s;
                    settings.choke = i == 0;
                    machine.setSettings (settings);
                    machine.prepareToPlay (renderRate, 512);
                    std::vector<float> samples;
                    juce::AudioBuffer<float> block (2, 512);

                    for (int pos = 0; pos < (int) (length * renderRate); pos += 512)
                    {
                        juce::MidiBuffer midi;

                        if (pos == 0)
                            midi.addEvent (juce::MidiMessage::noteOn (10, drums::getNote (Drum::openHat), 1.0f), 0);

                        if (pos <= (int) (0.15 * renderRate) && (int) (0.15 * renderRate) < pos + 512)
                            midi.addEvent (juce::MidiMessage::noteOn (10, drums::getNote (Drum::closedHat), 1.0f), (int) (0.15 * renderRate) - pos);

                        machine.processBlock (block, midi);
                        samples.insert (samples.end(), block.getReadPointer (0), block.getReadPointer (0) + 512);
                    }

                    columns[(size_t) i] = getColumns (samples, (int) box.getWidth());
                    keys[(size_t) i] = key;
                }

                float loudest = 0.01f;

                for (auto v : columns[(size_t) i])
                    loudest = juce::jmax (loudest, v);

                juce::Path shape;
                shape.startNewSubPath (box.getX(), box.getCentreY());

                for (size_t x = 0; x < columns[(size_t) i].size(); ++x)
                    shape.lineTo (box.getX() + (float) x, box.getCentreY() - box.getHeight() * 0.48f * columns[(size_t) i][x] / loudest);

                for (size_t x = columns[(size_t) i].size(); x-- > 0;)
                    shape.lineTo (box.getX() + (float) x, box.getCentreY() + box.getHeight() * 0.48f * columns[(size_t) i][x] / loudest);

                shape.closeSubPath();
                g.setColour (getDrumColour (Drum::openHat).withAlpha (0.5f));
                g.fillPath (shape);
                const float at = box.getX() + box.getWidth() * (float) (0.15 / length);
                g.setColour (theme::text);
                g.fillRect (at, box.getY(), 1.0f, box.getHeight());
            }
        } });

    pages.push_back ({ "Velocity, accents and groove", "How hard each hit is",
        "A drummer never hits twice the same way. Accents (louder hits) mark the beats that matter, ghost notes (very soft ones) "
        "fill the gaps, and the pattern of loud and soft is a large part of what makes a beat groove. In MIDI it's velocity, 0 to "
        "127, how hard each note was played.\n\n"
        "Velocity sets how much softer hits are quieter: at 100%, a hit at half velocity is 6 dB quieter; at 0%, every hit is the "
        "same, like the earliest drum machines (the 808 had a single accent for the whole kit).\n\n"
        "A classic pattern to start from, 16 steps to a bar of 4/4: the kick on beats 1 and 3 (and the \"and\" of 3), the snare on 2 "
        "and 4, closed hi-hats on every eighth note, a little softer on the off-beats. Play it, then move one kick or make a hi-hat "
        "softer and hear the feel change. The piano roll's velocity lane edits how hard each hit is.\n\n"
        "The picture shows that pattern, each hit as tall as its velocity.",
        [] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            drawCaption (g, r, "One bar, 16 steps", "height: velocity");
            struct Row { Drum drum; std::array<int, 16> velocity; };
            const Row rows[] = { { Drum::closedHat, { 100, 0, 60, 0, 100, 0, 60, 0, 100, 0, 60, 0, 100, 0, 60, 0 } },
                                 { Drum::snare, { 0, 0, 0, 0, 110, 0, 0, 30, 0, 0, 0, 0, 110, 0, 0, 0 } },
                                 { Drum::kick, { 120, 0, 0, 0, 0, 0, 0, 0, 110, 0, 90, 0, 0, 0, 0, 0 } } };
            const int step = (int) std::fmod (now() * 8.0, 16.0);
            auto grid = r.withTrimmedLeft (80.0f);
            const float rowH = grid.getHeight() / 3.0f, w = grid.getWidth() / 16.0f;

            for (int i = 0; i < 3; ++i)
            {
                const auto& row = rows[i];
                const float y = grid.getY() + rowH * (float) i;
                drawLabel (g, drums::getDrumName (row.drum), { r.getX(), y, 76.0f, rowH }, getDrumColour (row.drum));

                for (int s = 0; s < 16; ++s)
                {
                    auto cell = juce::Rectangle<float> (grid.getX() + w * (float) s, y, w, rowH).reduced (2.0f, 4.0f);
                    g.setColour ((s / 4) % 2 == 0 ? theme::panel.brighter (0.08f) : theme::panel.brighter (0.03f));
                    g.fillRect (cell);

                    if (row.velocity[(size_t) s] > 0)
                    {
                        g.setColour (getDrumColour (row.drum).withAlpha (s == step ? 1.0f : 0.7f));
                        g.fillRect (cell.withTop (cell.getBottom() - cell.getHeight() * (float) row.velocity[(size_t) s] / 127.0f));
                    }
                }
            }

            g.setColour (theme::text.withAlpha (0.6f));
            g.fillRect (grid.getX() + w * (float) step, grid.getY(), 1.5f, grid.getHeight());
        } });

    pages.push_back ({ "Tuning drums to the song", "A kick in the key",
        "A kick has a pitch, and a long one has a very clear pitch. When it rings at a note that isn't in the song's key, it rubs "
        "against the bass and the chords, and the low end sounds muddy. Producers tune the kick to the song: usually its root (A "
        "for a song in A minor), sometimes the fifth.\n\n"
        "The low notes are close together in Hz: A1 is 55 Hz, A#1 58.3, B1 61.7, C2 65.4. A kick at 50 Hz sits between G1 (49 Hz) "
        "and G#1 (51.9 Hz). Tune shows which note the drum is nearest when you hover it.\n\n"
        "A trap 808 is played as the bass line itself: the same kick at different pitches, a note per chord. Toms are tuned too, "
        "often a fourth or a fifth apart, and a snare's ring can clash or agree with the song.\n\n"
        "The picture shows the notes from C1 to E3 by their frequency, with where the window's kick and toms settle.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "The low notes, and where the drums settle", "white keys light, black keys dark");
            auto keys = r.withTrimmedBottom (r.getHeight() * 0.35f);
            constexpr int first = 24, last = 52;   // C1 to E3
            const float w = keys.getWidth() / (float) (last - first + 1);

            for (int note = first; note <= last; ++note)
            {
                const bool black = juce::String ("010100101010").substring (note % 12, note % 12 + 1) == "1";
                auto key = juce::Rectangle<float> (keys.getX() + w * (float) (note - first), keys.getY(), w, keys.getHeight()).reduced (1.0f, 0.0f);
                g.setColour (black ? theme::panel.brighter (0.02f) : theme::text.withAlpha (0.18f));
                g.fillRect (key);
                drawLabel (g, getNoteName (note), key.removeFromBottom (14.0f), theme::textDim, juce::Justification::centred, 9.5f);
                const double hz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
                drawLabel (g, hz < 100.0 ? juce::String (hz, 1) : juce::String (juce::roundToInt (hz)), key.removeFromBottom (13.0f), theme::textDim,
                           juce::Justification::centred, 9.0f);
            }

            auto marks = r.withTop (keys.getBottom() + 6.0f);
            int row = 0;

            for (const auto d : { Drum::kick, Drum::lowTom, Drum::highTom })
            {
                const double hz = s.drums[(size_t) d].tuneHz;
                const double note = 69.0 + 12.0 * std::log2 (hz / 440.0);
                const float x = keys.getX() + w * (float) (note - first + 0.5);

                if (note < first - 0.5 || note > last + 0.5)
                    continue;

                const auto [nearest, cents] = getNearestNote (hz);
                g.setColour (getDrumColour (d));
                g.fillRect (x - 1.0f, keys.getY(), 2.0f, keys.getHeight() + 6.0f + 16.0f * (float) row);
                drawLabel (g, drums::getDrumName (d) + " " + formatHz (hz) + ": " + getNoteName (nearest) + " " + (cents >= 0.0 ? "+" : "")
                                  + juce::String (juce::roundToInt (cents)) + " ct",
                           x > marks.getCentreX() ? juce::Rectangle<float> (x - 224.0f, marks.getY() + 16.0f * (float) row, 220.0f, 14.0f)
                                                  : juce::Rectangle<float> (x + 4.0f, marks.getY() + 16.0f * (float) row, 220.0f, 14.0f),
                           getDrumColour (d), x > marks.getCentreX() ? juce::Justification::centredRight : juce::Justification::centredLeft);
                ++row;
            }
        } });

    pages.push_back ({ "Building a kit, and recipes", "Where each drum sits, and common sounds",
        "A kit works when each drum has its own place: the kick under 100 Hz, the snare's body around 200 Hz and its crack above "
        "2 kHz, the toms between them, the hi-hats and cymbals above 6 kHz. When two drums fight for the same place, change one's "
        "tune or tone rather than its level.\n\n"
        "Build it in order: the kick first (tune it to the song, then bend and decay for punch and boom), then the snare against it "
        "(its tone above the kick, its noise for crack), then the hi-hats (short and quiet: they set the pace), then the rest.\n\n"
        "Recipes, all among the kits:\n"
        "808 kick: tune 45-55 Hz, bend +24 st over 30 ms, decay 0.6 s; for trap, a decay of 2 s or more, tuned to the key.\n"
        "909 kick: tune 50-60 Hz, bend +30 st over 15-20 ms, decay 0.4 s, click 50-70%.\n"
        "Snare: tune 180-220 Hz, bend +5 to +10 st, noise 60-70%, tone 1.5-3 kHz.\n"
        "Clap: tone 1-1.5 kHz, a tail of 0.2-0.6 s.\n"
        "Hi-hats: closed 40-80 ms, open 0.3-0.5 s with choke on, tone 7-9 kHz.\n"
        "Electro zap: bend +48 st over 10 ms. Syndrum tom: bend +24 st over 120 ms.\n\n"
        "The picture shows where each drum of the window's kit sits: the band of frequencies within 20 dB of its loudest.",
        [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
        {
            const auto s = processor->getSettings();
            drawCaption (g, r, "Where each drum sits", "the band within 20 dB of its loudest, in its first 170 ms");
            auto plot = r.withTrimmedLeft (90.0f).withTrimmedBottom (16.0f);
            drawFrequencyGrid (g, plot);
            const float rowH = plot.getHeight() / (float) numDrums;

            static std::array<juce::String, numDrums> keys;
            static std::array<std::vector<float>, numDrums> spectra;

            for (int i = 0; i < numDrums; ++i)
            {
                const auto d = (Drum) i;
                const auto key = getKey (s.drums[(size_t) i], d, 0.0, (int) plot.getWidth());

                if (keys[(size_t) i] != key)
                {
                    const auto rendered = drums::render (s, d, renderRate, 8192.0 / renderRate);
                    std::vector<float> all (rendered.tone.size());

                    for (size_t n = 0; n < all.size(); ++n)
                        all[n] = rendered.tone[n] + rendered.noise[n];

                    spectra[(size_t) i] = getSpectrumColumns (all, plot);
                    keys[(size_t) i] = key;
                }

                const auto& spectrumColumns = spectra[(size_t) i];
                const float top = *std::max_element (spectrumColumns.begin(), spectrumColumns.end());
                const float y = plot.getY() + rowH * (float) i;
                drawLabel (g, drums::getDrumName (d), { r.getX(), y, 86.0f, rowH }, getDrumColour (d));

                for (size_t x = 0; x < spectrumColumns.size(); ++x)
                {
                    const float above = spectrumColumns[x] - (top - 20.0f);

                    if (above > 0.0f)
                    {
                        g.setColour (getDrumColour (d).withAlpha (0.25f + 0.75f * juce::jmin (1.0f, above / 20.0f)));
                        g.fillRect (plot.getX() + (float) x, y + rowH * 0.2f, 1.0f, rowH * 0.6f);
                    }
                }
            }
        } });

    return pages;
}
} // namespace daw::drumtext
