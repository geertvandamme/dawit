#include "SpectralFreezeGuide.h"

namespace daw::sfztext
{
using sfz::Knob;

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    juce::String dot() { return juce::String::fromUTF8 (" \xc2\xb7 "); }

    juce::String formatDb (double db, int decimals = 1)
    {
        if (std::abs (db) < 0.05)
            return "0.0 dB";

        return (db > 0.0 ? "+" : "") + fx::formatNumber (db, decimals) + " dB";
    }

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

    void drawNote (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text)
    {
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (theme::textDim);
        g.drawFittedText (text, r.toNearestInt(), juce::Justification::centredLeft, 2);
    }

    void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow ({ from, to }, 1.2f, 7.0f, 7.0f);
    }

    juce::Rectangle<float> drawBox (juce::Graphics& g, juce::Rectangle<float> box, const juce::String& name, const juce::String& detail, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.18f));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (c);
        g.drawRoundedRectangle (box, 5.0f, 1.2f);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (theme::text);
        g.drawText (name, box.withTrimmedBottom (detail.isNotEmpty() ? box.getHeight() / 2.0f - 2.0f : 0.0f), juce::Justification::centred);

        if (detail.isNotEmpty())
        {
            g.setFont (juce::FontOptions (10.5f));
            g.setColour (theme::textDim);
            g.drawFittedText (detail, box.withTrimmedTop (box.getHeight() / 2.0f - 2.0f).reduced (4.0f, 2.0f).toNearestInt(), juce::Justification::centredTop, 2);
        }

        return box;
    }

    //==============================================================================
    // Example sounds for the pictures, made once.
    constexpr double exampleRate = 44100.0;

    /** The test chord at time t: A3, C4 and E4 with harmonics 1 to 6 at 1/n, plucked (5 ms attack, 20 dB a second decay). */
    double getChordSample (double t)
    {
        double sum = 0.0;

        for (const double f : { 220.0, 261.63, 329.63 })
            for (int n = 1; n <= 6; ++n)
                sum += std::sin (twoPi * f * n * t) / n;

        return sum * juce::jmin (1.0, t / 0.005) * std::exp (-t * 2.302585092994046) * 0.064;
    }

    const std::vector<float>& getChordWave()
    {
        static const std::vector<float> wave = []
        {
            std::vector<float> w ((size_t) (2.0 * exampleRate));

            for (size_t i = 0; i < w.size(); ++i)
                w[i] = (float) getChordSample ((double) i / exampleRate);

            return w;
        }();
        return wave;
    }

    /** A made-up snare hit over half a second: a noise burst plus a 190 Hz body, 1 ms attack, the noise 60 dB down in 150 ms and the body in 80 ms. */
    const std::vector<float>& getSnareWave()
    {
        static const std::vector<float> wave = []
        {
            std::vector<float> w ((size_t) (0.5 * exampleRate));
            juce::Random random (190);

            for (size_t i = 0; i < w.size(); ++i)
            {
                const double t = (double) i / exampleRate;
                const double attack = juce::jmin (1.0, t / 0.001);
                const double noise = (random.nextDouble() * 2.0 - 1.0) * std::pow (10.0, -3.0 * t / 0.15);
                const double body = std::sin (twoPi * 190.0 * t) * std::pow (10.0, -3.0 * t / 0.08);
                w[i] = (float) (attack * (0.5 * noise + 0.5 * body));
            }

            return w;
        }();
        return wave;
    }

    constexpr double snareCaptureEnd = 0.17, loopSeconds = 0.15;

    /** The size of a hold made from the snare: its RMS over the 0.15 s before the capture, as a steady, noise-like wave. */
    const std::vector<float>& getSnareHold()
    {
        static const std::vector<float> wave = []
        {
            const auto& snare = getSnareWave();
            const auto from = (size_t) ((snareCaptureEnd - loopSeconds) * exampleRate), to = (size_t) (snareCaptureEnd * exampleRate);
            double sum = 0.0;

            for (auto i = from; i < to; ++i)
                sum += (double) snare[i] * snare[i];

            const double rms = std::sqrt (sum / (double) (to - from));
            std::vector<float> w (snare.size());
            juce::Random random (23);

            for (auto& v : w)
                v = (float) (rms * 1.73 * (random.nextDouble() * 2.0 - 1.0));   // uniform noise has an RMS of 1/sqrt 3

            return w;
        }();
        return wave;
    }

    /** Draws samples from `from` to `to` as a minimum and maximum per pixel column, `scale` being the amplitude that fills half the height. */
    void drawWave (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<float>& s, size_t from, size_t to, float scale, juce::Colour c)
    {
        const int columns = juce::jmax (1, (int) r.getWidth());
        g.setColour (c);

        for (int x = 0; x < columns; ++x)
        {
            const auto a = from + (to - from) * (size_t) x / (size_t) columns;
            const auto b = juce::jmax (a + 1, from + (to - from) * (size_t) (x + 1) / (size_t) columns);
            float low = 0.0f, high = 0.0f;

            for (auto i = a; i < b && i < s.size(); ++i)
            {
                low = juce::jmin (low, s[i]);
                high = juce::jmax (high, s[i]);
            }

            const float top = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, high / scale) * r.getHeight() * 0.5f;
            const float bottom = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, low / scale) * r.getHeight() * 0.5f;
            g.fillRect (r.getX() + (float) x, top, 1.0f, juce::jmax (1.0f, bottom - top));
        }
    }

    /** One fast Fourier transform per size, kept for the next time (message thread only). */
    juce::dsp::FFT& getFft (int size)
    {
        static std::map<int, std::unique_ptr<juce::dsp::FFT>> ffts;
        auto& fft = ffts[size];

        if (fft == nullptr)
            fft = std::make_unique<juce::dsp::FFT> ((int) std::round (std::log2 ((double) size)));

        return *fft;
    }

    /** Two notes, A3 and A#3, equal level: analysed with a Hann window of `length` samples, padded with zeros to 65,536 so the curve is smooth. Returns the levels from 190 to 265 Hz, scaled to a peak of 1. */
    std::vector<float> getTwoNotesCurve (int length, int points)
    {
        constexpr int padded = 65536;
        std::vector<float> data ((size_t) padded * 2, 0.0f);
        const auto hann = sfz::makeHann (length);

        for (int i = 0; i < length; ++i)
        {
            const double t = i / exampleRate;
            data[(size_t) i] = (float) (0.5 * (std::sin (twoPi * 220.0 * t) + std::sin (twoPi * 233.08 * t))) * hann[(size_t) i];
        }

        getFft (padded).performRealOnlyForwardTransform (data.data(), true);
        const double spacing = exampleRate / padded;
        std::vector<float> curve ((size_t) points);
        float peak = 1.0e-9f;

        for (int p = 0; p < points; ++p)
        {
            const double hz = 190.0 + 75.0 * p / (points - 1);
            const int k = juce::roundToInt (hz / spacing);
            curve[(size_t) p] = std::hypot (data[(size_t) (2 * k)], data[(size_t) (2 * k + 1)]);
            peak = juce::jmax (peak, curve[(size_t) p]);
        }

        for (auto& v : curve)
            v /= peak;

        return curve;
    }

    double getExamplePeakHz()
    {
        const auto& s = getExampleSpectrum();
        const int bins = (int) s.db.size();
        int best = 1;

        for (int k = 1; k < bins - 1; ++k)
            if (s.db[(size_t) k] > s.db[(size_t) best])
                best = k;

        const double a = s.db[(size_t) best - 1], b = s.db[(size_t) best], c = s.db[(size_t) best + 1];
        const double denominator = a - 2.0 * b + c;
        const double delta = std::abs (denominator) > 1.0e-9 ? 0.5 * (a - c) / denominator : 0.0;
        return ((double) best + delta) * s.getSpacingHz();
    }

    /** What the pictures show as the hold: yours when something is held, else the test chord's. */
    struct GuideHold
    {
        Spectrum spectrum;
        double peakHz = 0.0;
        bool real = false;
    };

    GuideHold getGuideHold (const BuiltInSpectralFreeze& p)
    {
        sfz::Snapshot snapshot;

        if (p.getSnapshot (snapshot) && snapshot.isHeld())
            return { getHoldSpectrum (snapshot), describeHold (snapshot).peakHz, true };

        return { getExampleSpectrum(), getExamplePeakHz(), false };
    }
} // namespace

//==============================================================================
juce::String formatHz (double hz)
{
    if (hz < 100.0)
        return juce::String (hz, 1) + " Hz";

    const int n = juce::roundToInt (hz);
    return (n >= 1000 ? juce::String (n / 1000) + "," + juce::String::formatted ("%03d", n % 1000) : juce::String (n)) + " Hz";
}

float Spectrum::getDbAt (double hz) const
{
    if (db.empty() || size <= 0)
        return -120.0f;

    const double position = juce::jlimit (0.0, (double) db.size() - 1.001, hz / getSpacingHz());
    const auto i = (size_t) position;
    return juce::jmap ((float) (position - (double) i), db[i], db[i + 1]);
}

float Spectrum::getMaxDb (double lowHz, double highHz) const
{
    if (db.empty() || size <= 0)
        return -120.0f;

    const double spacing = getSpacingHz();
    const int from = juce::jlimit (0, (int) db.size() - 1, (int) std::floor (lowHz / spacing));
    const int to = juce::jlimit (0, (int) db.size() - 1, (int) std::ceil (highHz / spacing));

    if (to - from < 2)
        return getDbAt (0.5 * (lowHz + highHz));

    float best = -120.0f;

    for (int k = from; k <= to; ++k)
        best = juce::jmax (best, db[(size_t) k]);

    return best;
}

Spectrum computeSpectrum (const float* samples, int size, double rate)
{
    Spectrum s;
    s.rate = rate;
    s.size = size;
    std::vector<float> data ((size_t) size * 2, 0.0f);
    const auto hann = sfz::makeHann (size);

    for (int i = 0; i < size; ++i)
        data[(size_t) i] = samples[i] * hann[(size_t) i];

    getFft (size).performRealOnlyForwardTransform (data.data(), true);
    const int bins = sfz::getNumBins (size);
    s.db.resize ((size_t) bins);

    for (int k = 0; k < bins; ++k)
    {
        const float amplitude = 4.0f * std::hypot (data[(size_t) (2 * k)], data[(size_t) (2 * k + 1)]) / (float) size;
        s.db[(size_t) k] = 20.0f * std::log10 (juce::jmax (amplitude, 1.0e-6f));
    }

    return s;
}

Spectrum getLiveSpectrum (const BuiltInSpectralFreeze& p)
{
    constexpr int size = 4096;
    std::vector<float> samples (size);
    const double rate = p.getRate() > 0.0 ? p.getRate() : 44100.0;
    Spectrum best;

    for (int c = 0; c < 2; ++c)
    {
        p.readInput (c, samples.data(), size);
        auto s = computeSpectrum (samples.data(), size, rate);

        if (best.isEmpty())
            best = std::move (s);
        else
            for (size_t k = 0; k < best.db.size(); ++k)
                best.db[k] = juce::jmax (best.db[k], s.db[k]);
    }

    return best;
}

Spectrum getHoldSpectrum (const sfz::Snapshot& snapshot)
{
    Spectrum s;

    if (! snapshot.isHeld())
        return s;

    s.rate = snapshot.rate;
    s.size = snapshot.size;
    s.db.resize ((size_t) snapshot.getNumBins());

    for (size_t k = 0; k < s.db.size(); ++k)
    {
        float level = 0.0f;

        for (int c = 0; c < snapshot.channels; ++c)
            level = juce::jmax (level, snapshot.level[(size_t) c][k]);

        s.db[k] = 20.0f * std::log10 (juce::jmax (4.0f * level / (float) snapshot.size, 1.0e-6f));
    }

    return s;
}

const Spectrum& getExampleSpectrum()
{
    static const Spectrum s = []
    {
        const auto& chord = getChordWave();
        const int end = juce::roundToInt (0.25 * exampleRate);
        return computeSpectrum (chord.data() + (end - 4096), 4096, exampleRate);
    }();
    return s;
}

HoldInfo describeHold (const sfz::Snapshot& snapshot)
{
    HoldInfo info;

    if (! snapshot.isHeld())
        return info;

    info.held = true;
    info.peakHz = sfz::getSnapshotPeakHz (snapshot);
    double flatness = 0.0;

    for (int c = 0; c < snapshot.channels; ++c)
        flatness += sfz::getSnapshotFlatness (snapshot, c);

    info.flatness = flatness / snapshot.channels;
    info.rmsDb = 20.0 * std::log10 (juce::jmax (1.0e-6, sfz::getSnapshotRms (snapshot)));
    return info;
}

juce::String getStatus (bool freezeOn, const HoldInfo& info)
{
    if (! freezeOn)
        return "Not holding";

    if (! info.held || info.rmsDb < -100.0)
        return "Holding" + dot() + "silence";

    return "Holding" + dot() + (info.flatness >= 0.3 ? juce::String ("noise") : sfz::getNoteName (info.peakHz));
}

//==============================================================================
float SpectrumPlot::x (double hz) const
{
    return area.getX() + area.getWidth() * (float) (std::log (juce::jlimit (minHz, maxHz, hz) / minHz) / std::log (maxHz / minHz));
}

float SpectrumPlot::y (double db) const
{
    return area.getBottom() - area.getHeight() * (float) ((juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb));
}

double SpectrumPlot::getHzAt (float px) const
{
    const double t = juce::jlimit (0.0, 1.0, (double) (px - area.getX()) / juce::jmax (1.0f, area.getWidth()));
    return minHz * std::pow (maxHz / minHz, t);
}

SpectrumPlot layoutSpectrum (juce::Rectangle<float> bounds)
{
    SpectrumPlot plot;
    plot.area = bounds.withTrimmedLeft (30.0f).withTrimmedBottom (16.0f);
    return plot;
}

void drawSpectrum (juce::Graphics& g, const SpectrumPlot& plot, const Spectrum* live, const Spectrum* hold, double peakHz, double hoverHz)
{
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.fillRect (plot.area);
    g.setFont (juce::FontOptions (10.0f));

    for (double db = plot.minDb + 20.0; db <= plot.maxDb + 1.0e-6; db += 20.0)
    {
        g.setColour (juce::approximatelyEqual (db, 0.0) ? theme::gridBar : theme::grid);
        g.fillRect (plot.area.getX(), plot.y (db) - 0.5f, plot.area.getWidth(), 1.0f);
        g.setColour (theme::textDim);
        g.drawText (juce::String (juce::roundToInt (db)), juce::Rectangle<float> (plot.area.getX() - 30.0f, plot.y (db) - 6.0f, 26.0f, 12.0f), juce::Justification::centredRight);
    }

    // An A every octave: the lines are an octave apart, as you hear them.
    int octave = 1;

    for (double hz = 55.0; hz <= plot.maxHz; hz *= 2.0, ++octave)
    {
        g.setColour (theme::grid);
        g.fillRect (plot.x (hz) - 0.5f, plot.area.getY(), 1.0f, plot.area.getHeight());
        g.setColour (theme::textDim);
        g.drawText ("A" + juce::String (octave) + " " + juce::String (juce::roundToInt (hz)), juce::Rectangle<float> (plot.x (hz) - 36.0f, plot.area.getBottom() + 2.0f, 72.0f, 12.0f), juce::Justification::centred);
    }

    const int columns = juce::jmax (2, (int) plot.area.getWidth());
    auto makePath = [&] (const Spectrum& s, bool fill)
    {
        juce::Path p;

        for (int i = 0; i < columns; ++i)
        {
            const float px = plot.area.getX() + (float) i;
            const juce::Point<float> pt { px, plot.y (s.getMaxDb (plot.getHzAt (px), plot.getHzAt (px + 1.0f))) };

            if (i > 0)
                p.lineTo (pt);
            else if (fill)
            {
                p.startNewSubPath (plot.area.getX(), plot.area.getBottom());
                p.lineTo (pt);
            }
            else
                p.startNewSubPath (pt);
        }

        if (fill)
        {
            p.lineTo (plot.area.getRight(), plot.area.getBottom());
            p.closeSubPath();
        }

        return p;
    };

    if (hold != nullptr && ! hold->isEmpty())
    {
        g.setColour (colour.withAlpha (0.4f));
        g.fillPath (makePath (*hold, true));
        g.setColour (colour);
        g.strokePath (makePath (*hold, false), juce::PathStrokeType (1.3f));
    }

    if (live != nullptr && ! live->isEmpty())
    {
        g.setColour (theme::text.withAlpha (0.6f));
        g.strokePath (makePath (*live, false), juce::PathStrokeType (1.0f));
    }

    if (hold != nullptr && ! hold->isEmpty() && peakHz > 0.0)
    {
        const auto label = formatHz (peakHz) + dot() + sfz::getNoteName (peakHz);
        const float px = plot.x (peakHz), py = plot.y (hold->getDbAt (peakHz));
        auto box = juce::Rectangle<float> (px - 52.0f, juce::jmax (plot.area.getY() + 2.0f, py - 22.0f), 104.0f, 16.0f)
                       .constrainedWithin (plot.area.reduced (2.0f));
        g.setColour (theme::background.withAlpha (0.85f));
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (label, box, juce::Justification::centred);
        g.setColour (colour);
        g.fillEllipse (px - 3.0f, py - 3.0f, 6.0f, 6.0f);
    }

    if (hoverHz > 0.0)
    {
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillRect (plot.x (hoverHz) - 0.5f, plot.area.getY(), 1.0f, plot.area.getHeight());
    }
}

juce::String getHoverText (double hz, const Spectrum* live, const Spectrum* hold)
{
    auto text = formatHz (hz) + " (near " + sfz::getNoteName (hz) + ")";

    if (hold != nullptr && ! hold->isEmpty())
        text += dot() + "hold " + juce::String (juce::roundToInt (hold->getMaxDb (hz * 0.97, hz * 1.03))) + " dB";

    if (live != nullptr && ! live->isEmpty())
        text += dot() + "live " + juce::String (juce::roundToInt (live->getMaxDb (hz * 0.97, hz * 1.03))) + " dB";

    return text;
}

void paintMiniSpectrum (juce::Graphics& g, juce::Rectangle<float> plot, const BuiltInSpectralFreeze& p, bool bypassed)
{
    // The hold is copied a few times a second, not on every repaint.
    struct Cache
    {
        juce::uint32 stamp = 0;
        Spectrum hold;
    };

    static std::map<const BuiltInSpectralFreeze*, Cache> caches;
    const auto now = juce::Time::getMillisecondCounter();

    for (auto it = caches.begin(); it != caches.end();)
        it = now - it->second.stamp > 10000 ? caches.erase (it) : std::next (it);

    auto& cache = caches[&p];

    if (cache.stamp == 0 || now - cache.stamp > 200)
    {
        sfz::Snapshot snapshot;
        cache.hold = p.getSnapshot (snapshot) ? getHoldSpectrum (snapshot) : Spectrum {};
        cache.stamp = now;
    }

    const bool holding = p.isHolding() && ! cache.hold.isEmpty();
    Spectrum live;

    if (! holding)
    {
        constexpr int size = 1024;
        float samples[size];
        p.readInput (0, samples, size);
        live = computeSpectrum (samples, size, p.getRate() > 0.0 ? p.getRate() : 44100.0);
    }

    const Spectrum& shown = holding ? cache.hold : live;
    const int columns = juce::jmax (4, (int) (plot.getWidth() / 2.0f));
    const float width = plot.getWidth() / (float) columns;
    g.setColour (bypassed ? theme::textDim.withAlpha (0.4f) : holding ? colour : theme::textDim);

    for (int i = 0; i < columns; ++i)
    {
        const double low = 20.0 * std::pow (1000.0, (double) i / columns), high = 20.0 * std::pow (1000.0, (double) (i + 1) / columns);
        const float db = shown.isEmpty() ? -120.0f : shown.getMaxDb (low, high);
        const float height = plot.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f);
        g.fillRect (plot.getX() + width * (float) i, plot.getBottom() - height, juce::jmax (1.0f, width - 0.5f), juce::jmax (holding ? 1.0f : 0.0f, height));
    }
}

//==============================================================================
juce::String getTitle (int topic)
{
    switch (topic)
    {
        case (int) Knob::fade:       return "Fade";
        case (int) Knob::brightness: return "Brightness";
        case (int) Knob::movement:   return "Movement";
        case (int) Knob::mix:        return "Mix";
        case (int) Knob::output:     return "Output";
        case freezeSwitch:           return "Freeze";
        case captureAgain:           return "Capture again";
        case spectrum:               return "The spectrum";
        case fxui::CommonTopic::overview: return "DawIt Spectral Freeze";
        case fxui::CommonTopic::presets:  return "Starting points";
        case fxui::CommonTopic::clap:     return "Test sound";
        case fxui::CommonTopic::listen:   return "DawIt Spectral Freeze";
        case fxui::CommonTopic::guide:    return "What is a spectral freeze?";
        case fxui::CommonTopic::history:  return "The last 4 seconds";
        default: break;
    }

    return {};
}

juce::String getTooltip (int topic)
{
    switch (topic)
    {
        case freezeSwitch:           return "Freeze: hold about the last 0.15 s of sound for as long as you like. Click again to let go; the hold fades in and out over Fade.";
        case captureAgain:           return "Take a new snapshot of the sound now. The new hold fades in as the old one fades out.";
        case (int) Knob::fade:       return "How long the hold takes to come in and to go: 10 ms stops a sound at once, 2 s or more swells in like a pad.";
        case (int) Knob::brightness: return "Tilts the hold around 1 kHz: up makes it brighter and airier, down darker. The level stays the same.";
        case (int) Knob::movement:   return "0%: the hold is perfectly still. Higher: each frequency's level and phase wander at random, so the hold breathes and shimmers.";
        case (int) Knob::mix:        return "How much of the hold you hear against your track while it holds. 100%: only the hold. With nothing held, Mix leaves your track untouched.";
        case (int) Knob::output:     return "The level of everything that comes out.";
        case spectrum:               return "Live sound (thin line) and the hold (filled), 20 Hz to 20 kHz. Move over it to read the frequency and levels.";
        case fxui::CommonTopic::clap: return "Plays a plucked A minor chord and holds it 0.25 s in, replacing any hold.";
        case fxui::CommonTopic::history: return "The last 4 seconds: how loud your track was (grey) and the hold as it is mixed in (in colour).";
        default: break;
    }

    return {};
}

juce::String describe (int topic)
{
    switch (topic)
    {
        case freezeSwitch:
            return "Freeze takes one moment of a sound and keeps it sounding for as long as you like. Click it while a sound rings: DawIt takes the snapshot of about the last 0.15 seconds and fades the hold in over Fade. "
                   "Click it again and the hold fades out over Fade, and your track plays on as before. The hold carries on when the song stops or starts, like a held pedal, and is saved with the project.";

        case captureAgain:
            return "Takes a new snapshot of the sound now. The new snapshot fades in while the old one fades out. Use it when you froze silence, or caught a note as it faded: "
                   "click it just after a note starts for a fuller hold.";

        case (int) Knob::fade:
            return "Fade sets how long the hold takes to come in when you click Freeze, and to go when you click it off: from 10 ms, a sudden stop, to 10 seconds, a slow swell. "
                   "0.3 s comes in smoothly without sounding slow. Even 10 ms is a short fade, not a jump, so it doesn't click.";

        case (int) Knob::brightness:
            return "Brightness tilts the hold's spectrum around 1 kHz, by up to 6 dB per octave. At +3 dB per octave, 8 kHz (three octaves above 1 kHz) comes up 9 dB and 125 Hz goes down 9 dB. "
                   "DawIt then sets the hold back to the level it had, so Brightness changes the colour, not the loudness. A single pure tone has nothing to tilt and stays as it was. "
                   "A darker hold tends to sit behind other parts, such as under a vocal; a brighter one comes forward and adds air. That's a mixing habit, not a rule.";

        case (int) Knob::movement:
            return "A hold made of perfectly steady frequencies can sound static and mechanical, most of all on noise such as a breath. Movement brings it to life with a little randomness.\n"
                   "- Movement 0%: every bin keeps its level and its frequency. The cleanest hold of a chord or a vowel.\n"
                   "- Movement 20 to 50%: each bin's level drifts slowly up and down, and its phase is nudged at random. The hold breathes.\n"
                   "- Movement 100%: levels wander by up to 6 dB, about once a second, and each bin's phase is random every 23 ms. The hold shimmers and smears into an airy wash. The pitches stay put, but a pure tone grows rough.\n"
                   "The level stays the same whatever Movement is set to.";

        case (int) Knob::mix:
            return "Mix sets what you hear while it holds.\n"
                   "- 100%: only the hold. Your track goes quiet under it, which is how a freeze stops a sound in its tracks.\n"
                   "- 50%: both, each 3 dB down, so together they're about as loud as either alone.\n"
                   "- 0%: only your track.\n"
                   "When nothing is held, your track passes through untouched, whatever Mix says; only Output still sets its level.";

        case (int) Knob::output:
            return "Output sets the level of everything that comes out, from -24 to +12 dB. At 0 dB with nothing held, your track passes through exactly as it came in.";

        case spectrum:
            return "The spectrum of what comes into DawIt Spectral Freeze right now: low frequencies on the left (20 Hz), high on the right (20 kHz), louder higher up. Every octave gets the same width, as you hear them. "
                   "The thin line is the live sound. The coloured shape is the hold, the snapshot DawIt Spectral Freeze plays. Its tallest peak is labelled with its frequency and nearest note. "
                   "The tallest peak isn't always the note you played: it can be a harmonic, an octave or a fifth above it. A note shows a row of peaks, one per harmonic; a breath or a cymbal shows a broad hill with no clear peaks.";

        case fxui::CommonTopic::clap:
            return "Plays a plucked A minor chord through the freeze and holds it 0.25 seconds after it starts, replacing any hold. Freeze switches on a moment later. Use it to hear and see a hold even while the song is stopped.";

        case fxui::CommonTopic::history:
            return "The last 4 seconds, newest on the right: how loud the track coming in was (grey), and the hold as it is mixed in (in colour). While it holds, the colour stays level, however the track changes.";

        default: break;
    }

    return {};
}

juce::String describeSettings (int topic, const sfz::Settings& s)
{
    switch (topic)
    {
        case (int) Knob::fade:
            return "The hold comes in and goes out over " + sfz::formatKnob (Knob::fade, s) + ".";

        case (int) Knob::brightness:
            return sfz::formatKnob (Knob::brightness, s) + ": 8 kHz " + formatDb (sfz::getTiltDb (8000.0, s.brightnessDb)) + ", 125 Hz " + formatDb (sfz::getTiltDb (125.0, s.brightnessDb)) + ", 1 kHz stays as it is.";

        case (int) Knob::movement:
            return sfz::formatKnob (Knob::movement, s) + ": each frequency's level wanders by up to " + fx::formatNumber (0.06 * s.movementPercent, 1) + " dB.";

        case (int) Knob::mix:
        {
            const auto gains = sfz::getFadeGains (1.0, s.mixPercent);
            return sfz::formatKnob (Knob::mix, s) + " while it holds: your track " + formatDb (juce::Decibels::gainToDecibels (gains.first, -100.0))
                   + ", the hold " + formatDb (juce::Decibels::gainToDecibels (gains.second, -100.0)) + ".";
        }

        case (int) Knob::output:
            return "Everything comes out " + sfz::formatKnob (Knob::output, s) + ".";

        default: break;
    }

    return {};
}

//==============================================================================
std::vector<fxui::Guide::Page> makeGuide (BuiltInSpectralFreeze& freeze)
{
    BuiltInSpectralFreeze* processor = &freeze;
    std::vector<fxui::Guide::Page> pages;

    struct PageText { const char* name; const char* summary; const char* text; };
    static const PageText texts[] = {
        { "Holding a moment", "A sound that never ends", R"sfz(A spectral freeze takes one moment of a sound and keeps it sounding for as long as you like. Strike a piano chord, click Freeze while it rings, and the chord stays: no fading, no new attack, just the chord, held still. A breath becomes a soft wind; a cymbal becomes a steady wash.

It's for pads, drones and textures made from sounds you already have. Hold the last chord of a song under a spoken line. Hold a sung "ah" as a bed under a verse. Or stop a sound dead for a break.

It's called spectral because it doesn't keep the wave itself. It keeps the sound's spectrum: how loud each frequency was in about the last 0.15 seconds before you clicked. From that list it builds new sound, over and over, so the hold has no beginning, no end and no loop that repeats.

What happens:
- Freeze on: DawIt takes the snapshot and fades the hold in over Fade (0.3 s to start with).
- While it holds: the hold plays with your track by Mix. At Mix 100% you hear only the hold.
- Freeze off: the hold fades out over Fade, and your track plays on as before.
- Capture again: a new snapshot fades in while the old one fades out.

This is not the Freeze in other DAWs' track menus, which renders a track to audio to save computer power. Spectral effects such as Ableton Live's Spectral Time have a freeze like this one.

Click Hear a freeze: a plucked A minor chord plays and is held 0.25 s after it starts. The picture shows the chord dying away in grey and the hold carrying on in colour.)sfz" },
        { "A sound as a spectrum", "How loud each frequency is", R"sfz(Any sound can be described as many sine waves at once, each at its own frequency and level. That list is its spectrum. What is sound? shows it for steady shapes: a note at 220 Hz has its fundamental at 220 Hz and harmonics at 440, 660 and 880 Hz and up.

The picture is the spectrum of what comes into DawIt Spectral Freeze right now: low frequencies on the left (20 Hz), high on the right (20 kHz), louder higher up. Every octave gets the same width, as you hear them.

- A held note shows a row of peaks, one per harmonic.
- A chord shows several rows mixed together.
- A breath or a cymbal shows a broad hill with no clear peaks. That's noise: every frequency at once.

The coloured shape is the hold, the snapshot DawIt Spectral Freeze plays. Its tallest peak is labelled with its frequency and nearest note. The tallest peak isn't always the note you played: it can be a harmonic, an octave or a fifth above it. Move the mouse over the picture to read the frequency, the hold's level and the live sound's level there.)sfz" },
        { "Frequency bins", "The FFT's slices of the spectrum", R"sfz(To find a spectrum, the computer takes a short piece of the sound, here 4,096 samples, and runs a fast Fourier transform (FFT) on it. The FFT answers one question for many frequencies at once: how much of this frequency is in the piece?

The frequencies it answers for are evenly spaced, like the rungs of a ladder. Each rung is a frequency bin. The spacing is the sample rate divided by the length of the piece: 44,100 ÷ 4,096 = 10.8 Hz. So the bins sit at 0, 10.8, 21.5, 32.3 Hz and so on, up to 22,050 Hz, half the sample rate: 2,049 bins in all.

Before the FFT, the piece is faded in and out with a smooth curve called a Hann window, so its hard edges don't add frequencies of their own. The price: one pure tone lights up a few neighbouring bins, not one. Its peak is 4 bins wide at the base (43 Hz here) and about 2 bins wide at half its height.

Even spacing means the bins are sparse at the bottom and crowded at the top. Between 20 and 40 Hz, the lowest octave you hear, there are only 2 bins. Between 10 and 20 kHz there are 929. That's why a freeze holds the detail of high sounds better than of low ones.

A note rarely sits exactly on a bin: 220 Hz falls between 215.3 and 226.1 Hz. The freeze still holds 220 Hz, because it also measures where in its cycle each bin's wave is (its phase) and how far that moves in 23 ms, which gives the exact frequency. See What a snapshot keeps.

The picture zooms in on the hold around its tallest peak, with one bar per bin.)sfz" },
        { "Time or frequency", "You can't have both sharp", R"sfz(A longer piece gives bins that are closer together, but it blurs time. A shorter piece follows quick changes, but its bins are further apart. You can't have both: the bin spacing times the length of the piece is always 1.

At 44.1 kHz:
- 1,024 samples: 23 ms long, bins 43.1 Hz apart.
- 4,096 samples, as DawIt Spectral Freeze uses: 93 ms, bins 10.8 Hz apart.
- 16,384 samples: 372 ms, bins 2.7 Hz apart.

Why it matters for a freeze. Two notes a semitone apart are about 6% apart in frequency. At A5 (880 and 932 Hz) that's 52 Hz, about 5 bins: the freeze holds them apart. At A3 (220 and 233 Hz) it's 13 Hz, about 1 bin. At A1 (55 and 58 Hz) it's 3.3 Hz, a third of a bin: they land in the same bins and come out blurred, as one rough tone. So low chords turn muddy in a freeze sooner than high ones. For a clearer hold, freeze a chord played higher up, or with its low notes spread apart.

A longer piece would separate low notes, but it would take in more of what changes: freeze a quick melody and you hold several notes at once. DawIt uses about 85 to 93 ms (4,096 samples at 44.1 and 48 kHz, 8,192 at 88.2 and 96 kHz), a common middle choice.

Click Play two notes to hear A3 and A#3 together. The picture shows them analysed at the three lengths: at 1,024 samples one hump, at 16,384 two clear peaks.)sfz" },
        { "What a snapshot keeps", "Level and pitch, not time", R"sfz(For each bin the snapshot keeps how loud it was, averaged over about the last 0.15 seconds, and its exact frequency. That's why a held A stays an A. It also keeps where in its cycle each bin's wave was at the end (its phase), so the hold starts with a note's harmonics lined up as they were; from there each one simply carries on at its own frequency.

It keeps no order in time. Those 0.15 seconds become one still picture. So:
- Attacks are lost. The knock of a piano hammer, the crack of a snare and the start of a consonant are spread over the snapshot and lose their edge. A frozen snare becomes a steady "shhh" with the drum's colour; a frozen "k" becomes a hiss.
- Rhythm is lost. Freeze a drum loop and you hold a blend of whatever was in those 0.15 seconds.
- Vibrato stops. A singer's wobble becomes one steady pitch, or a slightly blurred one if the snapshot caught both ends of it.
- The moment matters. Click Freeze just after a piano note starts and you hold the bright beginning; click late and you hold the darker, quieter end.

The best sounds to freeze already hold still for a moment: a sung vowel, a held chord, a pad, a breath, the ring after a hit.

The picture shows a snare hit over half a second, and below it the hold made from it: the same colours, with no hit.)sfz" },
        { "Freeze or loop?", "Why a hold isn't a repeat", R"sfz(You could also hold a sound by looping a short piece of it, as a sampler does. A loop sounds different, for three reasons.
- A loop repeats. Loop 0.15 seconds and it plays about 6 or 7 times a second: you hear a flutter, a rhythm. A freeze builds each new piece from the snapshot with every wave carried on where it was, so there's no point where it starts over.
- A loop has a seam. Where the end jumps back to the start, the wave jumps too, and that clicks unless the loop is cut at matching points or crossfaded. Even then, two different moments of a sound don't always join smoothly.
- A loop keeps time. Everything in the piece comes back each time: the attack, the vibrato, a change of note. A freeze has no time in it.

A loop is better when you want the real sound, changes and all: a drum loop, a riff, a sampled note with its attack. A freeze is better when you want a still colour: a pad, a drone, a wash.

Granular synthesis sits between the two: it plays many short, overlapping grains of a recording, and can wander through it.

The picture shows a loop of about 0.15 s of the snare repeating, with its seams marked, above the freeze's even hold.)sfz" },
        { "Movement and brightness", "Bring a still hold to life", R"sfz(A hold made of perfectly steady frequencies can sound static and mechanical, most of all on noise such as a breath. Movement brings it to life with a little randomness, as many freezes do.
- Movement 0%: every bin keeps its level and its frequency. The cleanest hold of a chord or a vowel.
- Movement 20 to 50%: each bin's level drifts slowly up and down, and its phase is nudged at random. The hold breathes.
- Movement 100%: levels wander by up to 6 dB, about once a second, and each bin's phase is random every 23 ms. The hold shimmers and smears into an airy wash, like the extreme stretching of PaulStretch, which randomises phases on purpose. The pitches stay put, but a pure tone grows rough.

The level stays the same whatever Movement is set to.

Brightness tilts the hold's spectrum around 1 kHz, by up to 6 dB per octave. At +3 dB per octave, 8 kHz (three octaves above 1 kHz) comes up 9 dB and 125 Hz goes down 9 dB. DawIt then sets the hold back to the level it had, so Brightness changes the colour, not the loudness. A single pure tone has nothing to tilt and stays as it was.

A darker hold tends to sit behind other parts, such as under a vocal; a brighter one comes forward and adds air. That's a mixing habit, not a rule.

The picture shows the hold with Brightness's tilt line through 1 kHz, and a band for how far Movement lets each level wander.)sfz" },
        { "Fade, mix and letting go", "How the hold starts and stops", R"sfz(Fade sets how long the hold takes to come in when you click Freeze, and to go when you click it off: from 10 ms, a sudden stop, to 10 seconds, a slow swell. 0.3 s comes in smoothly without sounding slow. Even 10 ms is a short fade, not a jump, so it doesn't click.

Mix sets what you hear while it holds:
- 100%: only the hold. Your track goes quiet under it, which is how a freeze stops a sound in its tracks.
- 50%: both, each 3 dB down, so together they're about as loud as either alone.
- 0%: only your track.
When nothing is held, your track passes through untouched, whatever Mix says; only Output still sets its level.

What happens when:
- The sound stops: the hold goes on. That's the point: a breath keeps sounding after it ends.
- You click Freeze on silence: the hold is silent too. Click Capture again while the sound plays.
- The song stops or starts: the hold carries on, like a held pedal. Click Freeze off to let go.
- You save the project: the hold is saved with it and sounds the same when you open it again.
- You bypass it: your track plays untouched at once, and the hold stops.
- Timing: no delay. The hold is made from sound already heard, so your track stays in time.

DawIt can't yet switch Freeze on at a point in the song. To keep a hold, play to the moment, click Freeze, set Mix to 100% and bounce the track in place: the result is an audio clip of the hold.)sfz" },
        { "Make a pad from your sound", "Step by step, and what it can't do", R"sfz(Here is how to turn a sound of your own into a pad under a song, with the Am F C G of the example song (96 BPM, so a bar lasts 2.5 s).

1. Choose a sound that lasts a moment: a keys chord, a sung "ah", a breath or a cymbal. Put it on its own track, or add DawIt Spectral Freeze to the track that has it.
2. Add DawIt Spectral Freeze (the + Effect button in the inspector or the device bar). Press play.
3. Click Freeze while the sound rings, not on its first moment. The snapshot is the 0.15 s before your click, and the start of a note is noisy and still changing. A plucked chord also fades (the test chord loses 20 dB a second), so click about 0.25 s after it starts, as Test sound does. If the status says Holding but the hold is quiet, Your track says so: click Capture again.
4. Pick a starting point to begin from. Pad under the playing (Fade 1.5 s, Brightness -1 dB/oct, Movement 30%, Mix 50%) sits under the part you play. Held chord (Fade 300 ms, Movement 10%, Mix 100%) gives the clean hold. Breath texture (Fade 2 s, Movement 60%) suits a breath or a cymbal, which are noise.
5. Follow the chords. A hold stays on the chord it was taken from, so on each chord change click Capture again about 0.25 s after the new chord starts: then the 0.15 s it takes holds the new chord, not the end of the old one. The new hold crossfades with the old one over Fade: at 1.5 s that is the first 1.5 s of a 2.5 s bar.
6. Make it sit. Lower Mix so your track is heard through the hold. A darker hold tends to sit behind a vocal: try Brightness at -2 dB/oct or lower. On a noisy hold such as a breath, Movement of 20% or more keeps it from sounding static and mechanical; 30 to 60% is a good start.
7. Keep it. Set Mix to 100% and use Track > Bounce in Place: you get an audio clip of the hold. A bounce plays only the hold you have now, because DawIt can't yet capture at a point in the song by itself. For a pad that follows the chords, capture and bounce one chord at a time (Bounce in Place mutes the original track; unmute it to capture the next), then trim each clip to its bar. Copy a clip with Cmd-drag to use it under other sections.

What it is, honestly: an effect, not an instrument. It takes no notes. Playing a MIDI key doesn't change what it holds, and a hold stays at the pitch it was taken at: an A3 stays an A3. DawIt Sampler plays SoundFonts, but it can't yet load a recording of your own, so there is no way yet to play a bounced hold across the keys; a held clip can only be placed on the timeline like any audio. The picture shows the four chords, a capture on each, and each hold crossfading into the next.)sfz" },
        { "How DawIt Spectral Freeze works", "Pieces, spectra, new pieces", R"sfz(1. It keeps the last second of your track.
2. When you click Freeze, it takes the newest 0.15 s as 4 overlapping pieces of 4,096 samples, 1,024 samples (23 ms) apart, each faded with a Hann window.
3. An FFT turns each piece into 2,049 bins. The snapshot keeps each bin's level over the four pieces, its exact frequency, from how far the bin's phase moved from the third piece to the fourth, and its phase in the fourth piece, where the hold starts from.
4. Every 23 ms it builds a new piece: each bin at its level, its phase carried on by its frequency, with Movement's randomness, tilted by Brightness and set back to the snapshot's level.
5. An inverse FFT turns the piece back into sound. It's faded with the same window and laid over the pieces before it, four overlapping at a time, so the joins can't be heard.
6. Fade brings the hold in and out; Mix sets it against your track with equal power; Output sets the level of everything.

Each side of a stereo track has its own snapshot. Movement uses the same randomness on both, so a mono sound stays mono. At 88.2 and 96 kHz the pieces are 8,192 samples, so they last as long and the bins stay about as far apart.)sfz" },
        { "Your track", "What the hold is made of", R"sfz(This topic looks at the hold on the track DawIt Spectral Freeze is on: how loud it is, whether it's mostly notes or mostly noise, its tallest peak and nearest note, and, in DawIt, whether that note is in the song's key. Play the track, click Freeze while a sound rings, and the advice follows.)sfz" },
    };

    for (auto& t : texts)
        pages.push_back ({ juce::String::fromUTF8 (t.name), juce::String::fromUTF8 (t.summary), juce::String::fromUTF8 (t.text), nullptr });

    for (size_t i = 0; i < pages.size(); ++i)
        pages[i].noClap = i != 0 && i != 3;

    // 1. Holding a moment: the chord dying away, and its hold.
    pages[0].ownClap = [processor] { processor->playTestChord(); };
    pages[0].clapText = "Hear a freeze";
    pages[0].clapTooltip = "Plays a plucked A minor chord and holds it 0.25 s after it starts, replacing any hold.";
    pages[0].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        sfz::Snapshot snapshot;
        const bool held = processor->getSnapshot (snapshot) && snapshot.isHeld();
        drawCaption (g, r, "The test chord dying away (grey), and its hold carrying on (colour)",
                     held ? "your hold: " + juce::String (juce::roundToInt (describeHold (snapshot).rmsDb)) + " dBFS" : "nothing held yet");
        const auto plot = r.withTrimmedBottom (46.0f);
        const auto& wave = getChordWave();
        const float seconds = 2.0f, captureAt = 0.25f;
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot);
        drawWave (g, plot, wave, 0, wave.size(), 0.3f, theme::textDim);

        // The hold's size: the chord's level in the 0.15 s it was taken from (its RMS, and a wave's peaks run about twice that), as a band that comes in over Fade.
        double sumSquares = 0.0;
        const auto from = (size_t) ((captureAt - 0.15f) * exampleRate), to = (size_t) (captureAt * exampleRate);

        for (size_t i = from; i < to; ++i)
            sumSquares += (double) wave[i] * wave[i];

        const float peak = 2.0f * (float) std::sqrt (sumSquares / (double) (to - from));

        const float fade = s.fadeMs * 0.001f;
        juce::Path band;
        const int columns = (int) plot.getWidth();
        bool started = false;

        for (int x = 0; x <= columns; ++x)
        {
            const float t = seconds * (float) x / (float) columns;

            if (t < captureAt)
                continue;

            const float a = juce::jlimit (0.0f, 1.0f, (t - captureAt) / juce::jmax (0.001f, fade)) * peak / 0.3f * plot.getHeight() * 0.5f;
            const juce::Point<float> pt { plot.getX() + (float) x, plot.getCentreY() - a };

            if (! started)
                band.startNewSubPath (pt);
            else
                band.lineTo (pt);

            started = true;
        }

        for (int x = columns; x >= 0; --x)
        {
            const float t = seconds * (float) x / (float) columns;

            if (t < captureAt)
                continue;

            const float a = juce::jlimit (0.0f, 1.0f, (t - captureAt) / juce::jmax (0.001f, fade)) * peak / 0.3f * plot.getHeight() * 0.5f;
            band.lineTo (plot.getX() + (float) x, plot.getCentreY() + a);
        }

        band.closeSubPath();
        g.setColour (colour.withAlpha (0.4f));
        g.fillPath (band);
        g.setColour (colour);
        g.strokePath (band, juce::PathStrokeType (1.2f));

        const float captureX = plot.getX() + plot.getWidth() * captureAt / seconds;
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.fillRect (captureX - 0.5f, plot.getY(), 1.0f, plot.getHeight());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("Freeze, 0.25 s in", juce::Rectangle<float> (captureX + 4.0f, plot.getY() + 2.0f, 140.0f, 14.0f), juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.0f));
        g.setColour (theme::textDim);

        for (int i = 0; i <= 4; ++i)
            g.drawText (juce::String (0.5 * i, 1) + " s", juce::Rectangle<float> (plot.getX() + plot.getWidth() * (float) i / 4.0f - 20.0f, plot.getBottom() + 1.0f, 40.0f, 12.0f), juce::Justification::centred);

        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "The band is the size of the hold: it comes in over Fade (" + sfz::formatKnob (Knob::fade, s) + "), then stays level. The chord itself fades 20 dB a second.");
    };

    // 2. A sound as a spectrum: the live sound and the hold.
    pages[1].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto hold = getGuideHold (*processor);
        const auto live = getLiveSpectrum (*processor);
        drawCaption (g, r, "The live sound (thin line) and the hold (colour)", hold.real ? "your hold" : "example: the test chord's hold");
        auto plot = layoutSpectrum (r.withTrimmedBottom (32.0f));
        drawSpectrum (g, plot, &live, &hold.spectrum, hold.peakHz);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "Low on the left, high on the right, every octave the same width; louder is higher. The lines are the As: A3 is 220 Hz, A4 440 Hz.");
    };

    // 3. Frequency bins: the hold bin by bin around its tallest peak.
    pages[2].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto hold = getGuideHold (*processor);
        const auto& s = hold.spectrum;
        const double spacing = s.getSpacingHz();
        drawCaption (g, r, "The hold around its tallest peak, one bar per bin", "bins " + juce::String (spacing, 1) + " Hz apart (" + formatHz (s.rate).dropLastCharacters (3)
                                                                            + juce::String::fromUTF8 (" \xc3\xb7 ") + formatHz (s.size).dropLastCharacters (3) + ")");
        const int centre = juce::jlimit (12, juce::jmax (12, (int) s.db.size() - 13), juce::roundToInt (hold.peakHz / spacing));
        const auto plot = r.withTrimmedBottom (46.0f);
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot);
        float top = -120.0f;

        for (int k = centre - 12; k <= centre + 12; ++k)
            top = juce::jmax (top, s.db[(size_t) k]);

        const float cell = plot.getWidth() / 25.0f;
        g.setFont (juce::FontOptions (9.5f));

        for (int i = 0; i < 25; ++i)
        {
            const int k = centre - 12 + i;
            const float height = plot.getHeight() * juce::jlimit (0.02f, 1.0f, 1.0f + (s.db[(size_t) k] - top) / 70.0f) * 0.92f;
            g.setColour (k == juce::roundToInt (hold.peakHz / spacing) ? juce::Colours::white : colour.withAlpha (0.85f));
            g.fillRect (plot.getX() + cell * (float) i + 2.0f, plot.getBottom() - height, cell - 4.0f, height);

            if (i % 2 == 0)
            {
                g.setColour (theme::textDim);
                g.drawText (juce::String ((double) k * spacing, 1), juce::Rectangle<float> (plot.getX() + cell * (float) i - 6.0f, plot.getBottom() + 2.0f, cell + 12.0f, 12.0f), juce::Justification::centred);
            }
        }

        g.setColour (theme::textDim);
        g.drawText ("bin frequency, Hz", juce::Rectangle<float> (plot.getX(), plot.getBottom() + 16.0f, plot.getWidth(), 12.0f), juce::Justification::centred);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 14.0f), hold.real ? "The white bar is the loudest bin of your hold." : "Example: the test chord's hold. The white bar is the loudest bin.");
    };

    // 4. Time or frequency: two notes at three window lengths.
    pages[3].ownClap = [processor] { processor->playTwoNotes(); };
    pages[3].clapText = "Play two notes";
    pages[3].clapTooltip = "Plays A3 and A#3 together for a second. They are not held.";
    pages[3].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "A3 and A#3 (220 and 233.08 Hz) analysed with three lengths of piece", "44.1 kHz");
        static const auto curves = [] { return std::array<std::vector<float>, 3> { getTwoNotesCurve (1024, 300), getTwoNotesCurve (4096, 300), getTwoNotesCurve (16384, 300) }; }();
        const int lengths[] = { 1024, 4096, 16384 };
        const float gap = 14.0f, width = (r.getWidth() - 2.0f * gap) / 3.0f;

        for (int p = 0; p < 3; ++p)
        {
            auto cell = juce::Rectangle<float> (r.getX() + (width + gap) * (float) p, r.getY(), width, r.getHeight() - 30.0f);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.setColour (theme::text);
            g.drawText (juce::String (lengths[p]) + " samples" + (p == 1 ? " (DawIt)" : ""), cell.removeFromTop (16.0f), juce::Justification::centredLeft);
            auto plot = cell.withTrimmedBottom (42.0f);
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.fillRect (plot);
            juce::Path curve;
            curve.startNewSubPath (plot.getX(), plot.getBottom());

            for (int i = 0; i < 300; ++i)
                curve.lineTo (plot.getX() + plot.getWidth() * (float) i / 299.0f, plot.getBottom() - plot.getHeight() * 0.95f * curves[(size_t) p][(size_t) i]);

            curve.lineTo (plot.getRight(), plot.getBottom());
            curve.closeSubPath();
            g.setColour (colour.withAlpha (0.45f));
            g.fillPath (curve);
            g.setColour (colour);
            g.strokePath (curve, juce::PathStrokeType (1.3f));

            for (const double hz : { 220.0, 233.08 })
            {
                const float x = plot.getX() + plot.getWidth() * (float) ((hz - 190.0) / 75.0);
                g.setColour (juce::Colours::white.withAlpha (0.5f));
                g.fillRect (x - 0.5f, plot.getY(), 1.0f, plot.getHeight());
                g.setFont (juce::FontOptions (10.0f));
                g.drawText (hz < 225.0 ? "A3" : "A#3", juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 1.0f, 40.0f, 12.0f), juce::Justification::centred);
            }

            // The length of the piece, as a bar: 16,384 samples fill the width.
            const double ms = 1000.0 * lengths[p] / exampleRate;
            const auto bar = juce::Rectangle<float> (plot.getX(), plot.getBottom() + 18.0f, plot.getWidth() * (float) lengths[p] / 16384.0f, 6.0f);
            g.setColour (theme::accent);
            g.fillRect (bar);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (10.5f));
            g.drawText (juce::String (juce::roundToInt (ms)) + " ms long, bins " + juce::String (exampleRate / lengths[p], 1) + " Hz apart",
                        juce::Rectangle<float> (plot.getX(), plot.getBottom() + 25.0f, plot.getWidth(), 13.0f), juce::Justification::centredLeft);
        }

        drawNote (g, r.withTrimmedTop (r.getHeight() - 14.0f), "190 to 265 Hz, each curve scaled to its own peak. The shorter the piece, the wider each peak.");
    };

    // 5 and 6. A snare hit and what a freeze or a loop makes of it.
    auto drawHoldRow = [] (juce::Graphics& g, juce::Rectangle<float> plot, const juce::String& label)
    {
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot);
        const auto& hold = getSnareHold();
        drawWave (g, plot, hold, 0, hold.size(), 1.0f, colour);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (label, plot.reduced (6.0f, 3.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
    };

    pages[4].paint = [drawHoldRow] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "A snare hit over half a second, and the hold made from it", "0.5 s");
        const float rowHeight = (r.getHeight() - 46.0f) / 2.0f;
        auto top = r.removeFromTop (rowHeight);
        r.removeFromTop (8.0f);
        auto bottom = r.removeFromTop (rowHeight);
        const auto& snare = getSnareWave();
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (top);
        const float captureFrom = top.getX() + top.getWidth() * (float) ((snareCaptureEnd - loopSeconds) / 0.5), captureTo = top.getX() + top.getWidth() * (float) (snareCaptureEnd / 0.5);
        g.setColour (colour.withAlpha (0.22f));
        g.fillRect (captureFrom, top.getY(), captureTo - captureFrom, top.getHeight());
        drawWave (g, top, snare, 0, snare.size(), 1.0f, theme::textDim);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("the hit: a sharp attack, then it dies away", top.reduced (6.0f, 3.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.5f));
        g.setColour (colour);
        g.drawText ("the snapshot of about 0.15 s", juce::Rectangle<float> (captureTo + 4.0f, top.getBottom() - 16.0f, 130.0f, 13.0f), juce::Justification::centredLeft);
        drawHoldRow (g, bottom, "the hold: the same colour, with no hit");
        drawNote (g, r.withTrimmedTop (4.0f), "The snapshot keeps how loud each frequency was, not when. Every moment of the hold is the same.");
    };

    pages[5].paint = [drawHoldRow] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "A loop of about 0.15 s of the snare, above the freeze's even hold", "about " + juce::String (1.0 / loopSeconds, 1) + " repeats a second");
        const float rowHeight = (r.getHeight() - 46.0f) / 2.0f;
        auto top = r.removeFromTop (rowHeight);
        r.removeFromTop (8.0f);
        auto bottom = r.removeFromTop (rowHeight);
        const auto& snare = getSnareWave();
        const auto loopLength = (size_t) (loopSeconds * exampleRate);
        std::vector<float> looped (snare.size());

        for (size_t i = 0; i < looped.size(); ++i)
            looped[i] = snare[i % loopLength];

        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (top);
        drawWave (g, top, looped, 0, looped.size(), 1.0f, theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

        for (int seam = 1; seam * loopSeconds < 0.5; ++seam)
        {
            const float x = top.getX() + top.getWidth() * (float) (seam * loopSeconds / 0.5);
            g.setColour (juce::Colour (0xffff7a7a));
            g.fillRect (x - 0.5f, top.getY(), 1.0f, top.getHeight());
            g.drawText ("seam", juce::Rectangle<float> (x + 3.0f, top.getBottom() - 16.0f, 40.0f, 13.0f), juce::Justification::centredLeft);
        }

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("a loop: the hit comes back every 0.15 s", top.reduced (6.0f, 3.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
        drawHoldRow (g, bottom, "a freeze: no start, no end, no seam");
        drawNote (g, r.withTrimmedTop (4.0f), "Each seam is where the end jumps back to the start. A freeze builds every new piece from the snapshot, carrying each wave on.");
    };

    // 7. Movement and brightness: the tilt line and the band.
    pages[6].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        const auto hold = getGuideHold (*processor);
        drawCaption (g, r, "The hold, Brightness's tilt through 1 kHz and Movement's band", hold.real ? "your hold" : "example: the test chord's hold");
        auto plot = layoutSpectrum (r.withTrimmedBottom (32.0f));
        drawSpectrum (g, plot, nullptr, &hold.spectrum, 0.0);

        const float band = juce::jmax (1.0f, (float) (0.06 * s.movementPercent) / (float) (plot.maxDb - plot.minDb) * plot.area.getHeight());
        juce::Path wander;
        const int columns = (int) plot.area.getWidth();

        for (int i = 0; i <= columns; ++i)
        {
            const float px = plot.area.getX() + (float) i;
            const float y = plot.y (hold.spectrum.getMaxDb (plot.getHzAt (px), plot.getHzAt (px + 1.0f))) - band;
            i == 0 ? wander.startNewSubPath (px, y) : wander.lineTo (px, y);
        }

        for (int i = columns; i >= 0; --i)
        {
            const float px = plot.area.getX() + (float) i;
            wander.lineTo (px, plot.y (hold.spectrum.getMaxDb (plot.getHzAt (px), plot.getHzAt (px + 1.0f))) + band);
        }

        wander.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillPath (wander);

        const double reference = hold.spectrum.getDbAt (hold.peakHz) - 12.0;
        g.setColour (juce::Colour (0xfff2994a));
        g.drawLine (plot.area.getX(), plot.y (reference + sfz::getTiltDb (plot.minHz, s.brightnessDb)), plot.area.getRight(), plot.y (reference + sfz::getTiltDb (plot.maxHz, s.brightnessDb)), 2.0f);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("Brightness " + sfz::formatKnob (Knob::brightness, s) + ": 8 kHz " + formatDb (sfz::getTiltDb (8000.0, s.brightnessDb)) + ", 125 Hz " + formatDb (sfz::getTiltDb (125.0, s.brightnessDb)),
                    juce::Rectangle<float> (plot.area.getX() + 8.0f, plot.area.getY() + 4.0f, 380.0f, 14.0f), juce::Justification::centredLeft);
        g.setColour (theme::text);
        g.drawText ("Movement " + sfz::formatKnob (Knob::movement, s) + ": levels wander by up to " + fx::formatNumber (0.06 * s.movementPercent, 1) + " dB (the pale band)",
                    juce::Rectangle<float> (plot.area.getX() + 8.0f, plot.area.getY() + 20.0f, 420.0f, 14.0f), juce::Justification::centredLeft);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "The orange line is the tilt: it passes through 1 kHz at the same level, rises to the right for a positive Brightness and falls for a negative one. DawIt sets the level back afterwards.");
    };

    // 8. Fade, mix and letting go: the gains over time.
    pages[7].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "Your track and the hold, from Freeze on to Freeze off", "Fade " + sfz::formatKnob (Knob::fade, s) + ", Mix " + sfz::formatKnob (Knob::mix, s));
        const auto plot = r.withTrimmedBottom (46.0f);
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot);
        const float stops[] = { 0.0f, 0.14f, 0.36f, 0.64f, 0.86f, 1.0f };
        auto xAt = [&] (float t) { return plot.getX() + plot.getWidth() * t; };
        auto yAt = [&] (double gain) { return plot.getBottom() - 6.0f - (plot.getHeight() - 14.0f) * (float) gain; };
        const auto full = sfz::getFadeGains (1.0, s.mixPercent);

        auto curve = [&] (bool hold)
        {
            juce::Path p;
            p.startNewSubPath (xAt (stops[0]), yAt (hold ? 0.0 : 1.0));
            p.lineTo (xAt (stops[1]), yAt (hold ? 0.0 : 1.0));

            for (int i = 1; i <= 40; ++i)
            {
                const auto gains = sfz::getFadeGains ((double) i / 40.0, s.mixPercent);
                p.lineTo (xAt (juce::jmap ((float) i / 40.0f, stops[1], stops[2])), yAt (hold ? gains.second : gains.first));
            }

            p.lineTo (xAt (stops[3]), yAt (hold ? full.second : full.first));

            for (int i = 1; i <= 40; ++i)
            {
                const auto gains = sfz::getFadeGains (1.0 - (double) i / 40.0, s.mixPercent);
                p.lineTo (xAt (juce::jmap ((float) i / 40.0f, stops[3], stops[4])), yAt (hold ? gains.second : gains.first));
            }

            p.lineTo (xAt (stops[5]), yAt (hold ? 0.0 : 1.0));
            return p;
        };

        g.setColour (theme::textDim);
        g.strokePath (curve (false), juce::PathStrokeType (2.2f));
        g.setColour (colour);
        g.strokePath (curve (true), juce::PathStrokeType (2.2f));

        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.setColour (theme::text);
        const float markers[] = { stops[1], stops[2], stops[3], stops[4] };
        const char* const names[] = { "Freeze on", "Fade done", "Freeze off", "Fade done" };

        for (int i = 0; i < 4; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.fillRect (xAt (markers[i]) - 0.5f, plot.getY(), 1.0f, plot.getHeight());
            g.setColour (theme::text);
            g.drawText (names[i], juce::Rectangle<float> (xAt (markers[i]) - 50.0f, plot.getBottom() + 2.0f, 100.0f, 14.0f), juce::Justification::centred);
        }

        g.setColour (theme::textDim);
        g.drawText ("your track", juce::Rectangle<float> (xAt (stops[0]) + 6.0f, yAt (1.0) + 4.0f, 120.0f, 14.0f), juce::Justification::centredLeft);
        g.setColour (colour);
        g.drawText ("the hold", juce::Rectangle<float> (xAt (stops[2]) + 6.0f, yAt (full.second) + 4.0f, 120.0f, 14.0f), juce::Justification::centredLeft);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText ("Fade " + sfz::formatKnob (Knob::fade, s), juce::Rectangle<float> (xAt (stops[1]), plot.getBottom() + 18.0f, xAt (stops[2]) - xAt (stops[1]), 12.0f), juce::Justification::centred);
        g.drawText ("Fade " + sfz::formatKnob (Knob::fade, s), juce::Rectangle<float> (xAt (stops[3]), plot.getBottom() + 18.0f, xAt (stops[4]) - xAt (stops[3]), 12.0f), juce::Justification::centred);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 14.0f), "Levels as gains, equal power: at Mix 100% the track goes to nothing; at 50% each is 3 dB down. Times are not to scale.");
    };

    // 9. Make a pad: chords with a capture on each, the holds crossfading.
    pages[8].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto s = processor->getSettings();
        drawCaption (g, r, "Am, F, C, G: capture again on each chord, the holds crossfade", "Fade " + sfz::formatKnob (Knob::fade, s));
        const auto plot = r.withTrimmedBottom (46.0f);
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.fillRect (plot);
        const char* const chords[] = { "Am", "F", "C", "G" };
        const float barWidth = plot.getWidth() / 4.0f;
        const auto chordRow = plot.withHeight (28.0f).reduced (0.0f, 2.0f);
        const float fadeWidth = barWidth * 0.6f;   // 1.5 s of a 2.5 s bar, as a shape; it doesn't follow the knob

        for (int i = 0; i < 4; ++i)
        {
            auto cell = juce::Rectangle<float> (plot.getX() + barWidth * (float) i + 2.0f, chordRow.getY(), barWidth - 4.0f, chordRow.getHeight());
            g.setColour (theme::textDim.withAlpha (0.25f));
            g.fillRoundedRectangle (cell, 3.0f);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (chords[i], cell, juce::Justification::centred);

            const float x = plot.getX() + barWidth * (float) i;
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.fillRect (x + 6.0f, plot.getY() + 34.0f, 1.0f, plot.getHeight() - 56.0f);
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.drawText ("Capture again", juce::Rectangle<float> (x + 10.0f, plot.getY() + 34.0f, barWidth - 12.0f, 13.0f), juce::Justification::centredLeft);
        }

        const float top = plot.getY() + 56.0f, bottom = plot.getBottom() - 10.0f, height = bottom - top;

        for (int i = 0; i < 4; ++i)
        {
            const float x0 = plot.getX() + barWidth * (float) i + 6.0f;
            const float x1 = i < 3 ? plot.getX() + barWidth * (float) (i + 1) + 6.0f : plot.getRight() - 4.0f;
            juce::Path hold;
            const float inEnd = juce::jmin (x1, x0 + fadeWidth);
            hold.startNewSubPath (i == 0 ? x0 : x0, bottom);

            if (i > 0)
                hold.lineTo (x0, bottom);
            hold.lineTo (inEnd, top);
            hold.lineTo (x1, top);

            if (i < 3)
                hold.lineTo (juce::jmin (x1 + fadeWidth, plot.getRight() - 2.0f), bottom);
            else
                hold.lineTo (x1, bottom);

            hold.closeSubPath();
            g.setColour (colour.withAlpha (0.28f));
            g.fillPath (hold);
            g.setColour (colour);
            g.strokePath (hold, juce::PathStrokeType (1.4f));
        }

        juce::ignoreUnused (height);
        drawNote (g, r.withTrimmedTop (r.getHeight() - 30.0f), "One bar is 2.5 s at 96 BPM. Each shape is one hold's level: it fades in over Fade as the one before fades out. Not to scale in time.");
    };

    // 10. How it works: a block diagram.
    pages[9].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        drawCaption (g, r, "From your track to the hold", "pieces, spectra, new pieces");
        const float gapX = 30.0f, h = 62.0f, col = (r.getWidth() - 3.0f * gapX) / 4.0f;
        const float rowTop = r.getY() + 14.0f, rowBottom = r.getY() + 14.0f + h + 66.0f;
        auto box = [&] (int column, float y) { return juce::Rectangle<float> (r.getX() + (col + gapX) * (float) column, y, col, h); };
        const auto b0 = drawBox (g, box (0, rowTop), "Last second", "the input, kept all the time", theme::textDim);
        const auto b1 = drawBox (g, box (1, rowTop), "4 windows", "4,096 samples, 1,024 apart (23 ms), Hann", colour);
        const auto b2 = drawBox (g, box (2, rowTop), "FFT", "2,049 bins per window", colour);
        const auto b3 = drawBox (g, box (3, rowTop), "Snapshot", "level, frequency, phase per bin", colour);
        const auto c0 = drawBox (g, box (0, rowBottom), "New piece, every 23 ms", "phase carried on; Movement, Brightness", colour);
        const auto c1 = drawBox (g, box (1, rowBottom), "Inverse FFT", "back into sound", colour);
        const auto c2 = drawBox (g, box (2, rowBottom), "Overlap", "4 at a time, same window", colour);
        const auto c3 = drawBox (g, box (3, rowBottom), "Fade, Mix, Output", "equal-power mix with your track", theme::accent);

        arrow (g, { b0.getRight(), b0.getCentreY() }, { b1.getX(), b1.getCentreY() });
        arrow (g, { b1.getRight(), b1.getCentreY() }, { b2.getX(), b2.getCentreY() });
        arrow (g, { b2.getRight(), b2.getCentreY() }, { b3.getX(), b3.getCentreY() });
        arrow (g, { c0.getRight(), c0.getCentreY() }, { c1.getX(), c1.getCentreY() });
        arrow (g, { c1.getRight(), c1.getCentreY() }, { c2.getX(), c2.getCentreY() });
        arrow (g, { c2.getRight(), c2.getCentreY() }, { c3.getX(), c3.getCentreY() });

        // The snapshot feeds the new pieces: down, then back to the left.
        const float middle = (b3.getBottom() + c0.getY()) * 0.5f;
        juce::Path feed;
        feed.startNewSubPath (b3.getCentreX(), b3.getBottom());
        feed.lineTo (b3.getCentreX(), middle);
        feed.lineTo (c0.getCentreX(), middle);
        g.setColour (theme::textDim);
        g.strokePath (feed, juce::PathStrokeType (1.2f));
        arrow (g, { c0.getCentreX(), middle }, { c0.getCentreX(), c0.getY() });
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("taken once, when you click Freeze", juce::Rectangle<float> (c0.getCentreX() + 8.0f, middle - 16.0f, 220.0f, 14.0f), juce::Justification::centredLeft);
        g.drawText ("then again and again while it holds", juce::Rectangle<float> (c0.getCentreX() + 8.0f, middle + 2.0f, 240.0f, 14.0f), juce::Justification::centredLeft);
    };

    // 11. Your track: what the hold is made of.
    pages[10].paint = [processor] (juce::Graphics& g, juce::Rectangle<float> r, double)
    {
        const auto host = fxui::getHostTrack (*processor);
        sfz::Snapshot snapshot;
        const bool held = processor->getSnapshot (snapshot) && snapshot.isHeld();
        const auto info = describeHold (snapshot);
        drawCaption (g, r, (host.name.isEmpty() ? "Stand-in spectral freeze" : host.name) + ": the hold",
                     held ? "flatness " + juce::String (info.flatness, 2) + dot() + formatHz (info.peakHz) + " (" + sfz::getNoteName (info.peakHz) + ")" + dot() + juce::String (juce::roundToInt (info.rmsDb)) + " dBFS"
                          : "nothing is held");
        const auto plot = layoutSpectrum (r.withTrimmedBottom (4.0f));

        if (! held)
        {
            drawSpectrum (g, plot, nullptr, nullptr, 0.0);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (14.0f));
            g.drawText ("Play your track and click Freeze while the sound you want rings.", plot.area, juce::Justification::centred);
            return;
        }

        const auto hold = getHoldSpectrum (snapshot);
        const auto live = getLiveSpectrum (*processor);
        drawSpectrum (g, plot, &live, &hold, info.peakHz);
    };

    const auto introduction = pages[10].text;
    pages[10].liveText = [processor, introduction]
    {
        sfz::Snapshot snapshot;
        const bool held = processor->getSnapshot (snapshot) && snapshot.isHeld();
        const auto s = processor->getSettings();
        const auto host = fxui::getHostTrack (*processor);
        const auto info = describeHold (snapshot);

        sfz::AnalysisInput input;
        input.holding = s.freeze && held;
        input.movementPercent = s.movementPercent;
        input.holdDb = info.rmsDb;
        input.priorPeakDb = 20.0 * std::log10 (juce::jmax (1.0e-5, (double) snapshot.priorPeakRms));
        input.flatness = info.flatness;
        input.peakHz = info.peakHz;
        input.keyPitchClasses = processor->getKeyPitchClasses();
        input.keyName = host.keyName;
        input.keyKnown = input.keyPitchClasses != 0 && host.keyName.isNotEmpty();
        juce::StringArray lines;

        for (auto& line : sfz::analyse (input))
            lines.add (line);

        return introduction + "\n\n" + lines.joinIntoString ("\n\n");
    };

    return pages;
}
} // namespace daw::sfztext
