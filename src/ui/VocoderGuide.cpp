#include "VocoderGuide.h"

#include "Theme.h"
#include "VocoderEditor.h"
#include "generate/VocoderGuideText.h"

namespace daw::voctext
{
namespace
{
    namespace vg = gen::vocoderguide;

    constexpr double analysisRate = 44100.0;
    constexpr int framesPerSecond = 100;
    constexpr float floorDb = -60.0f;
    constexpr double idleSeconds = 0.9;     // where the pictures rest: inside the "ee"
    constexpr double vowelStarts[] { 0.0, 0.75, 1.5 }, vowelLengths[] { 0.6, 0.6, 0.5 }, hissStart = 2.15, hissLength = 0.25;

    const juce::Colour voiceColour = theme::accent, carrierColour = theme::meterGreen, resultColour = vocoderui::colour;

    double getSampleRateOf (const BuiltInVocoder& p) { return p.getSampleRate() > 0.0 ? p.getSampleRate() : 48000.0; }

    double getPosition (double sinceClap)
    {
        return sinceClap < vocoder::getTestVoiceSeconds() ? sinceClap : idleSeconds;
    }

    void caption (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText (text, r.removeFromTop (22.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.7f);
    }

    //==============================================================================
    /** A band-pass biquad (the standard constant-peak-gain formula), for the pictures only. */
    struct Biquad
    {
        void setBandPass (double rate, double centre, double q)
        {
            const double w = juce::MathConstants<double>::twoPi * centre / rate, alpha = std::sin (w) / (2.0 * q), a0 = 1.0 + alpha;
            b0 = alpha / a0;
            b2 = -alpha / a0;
            a1 = -2.0 * std::cos (w) / a0;
            a2 = (1.0 - alpha) / a0;
        }

        double process (double x)
        {
            const double y = b0 * x + z1;
            z1 = z2 - a1 * y;
            z2 = b2 * x - a2 * y;
            return y;
        }

        double b0 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    };

    /** How loud the made-up voice is in each band, 100 frames a second, in dB below the loudest value (-60 at the least). */
    struct Analysis
    {
        int bands = 0, frames = 0;
        std::vector<float> db;

        float at (int frame, int band) const
        {
            return db[(size_t) juce::jlimit (0, frames - 1, frame) * (size_t) bands + (size_t) band];
        }
    };

    const std::vector<float>& getVoice()
    {
        static const auto voice = vocoder::makeTestVoice (analysisRate);
        return voice;
    }

    Analysis analyse (int bands, double low, double high)
    {
        const auto& voice = getVoice();
        Analysis a;
        a.bands = bands;
        a.frames = (int) (voice.size() * framesPerSecond / analysisRate) + 1;
        a.db.assign ((size_t) (a.frames * bands), floorDb);
        const auto centres = vocoder::getBandCentres (bands, low, high, analysisRate);
        const double q = vocoder::getBandQ (bands, low, high, analysisRate);
        const int hop = (int) (analysisRate / framesPerSecond);
        std::vector<double> level ((size_t) (a.frames * bands), 0.0);
        double peak = 1.0e-9;

        for (int b = 0; b < bands; ++b)
        {
            Biquad one, two;
            one.setBandPass (analysisRate, centres[(size_t) b], q);
            two.setBandPass (analysisRate, centres[(size_t) b], q);
            const double smooth = std::exp (-1.0 / (0.015 * analysisRate));
            double env = 0.0;

            for (size_t i = 0; i < voice.size(); ++i)
            {
                env = smooth * env + (1.0 - smooth) * std::abs (two.process (one.process (voice[i])));

                if (i % (size_t) hop == 0)
                {
                    const auto frame = (size_t) (i / (size_t) hop);
                    level[frame * (size_t) bands + (size_t) b] = env;
                    peak = juce::jmax (peak, env);
                }
            }
        }

        for (size_t i = 0; i < level.size(); ++i)
            a.db[i] = juce::jlimit (floorDb, 0.0f, (float) juce::Decibels::gainToDecibels (level[i] / peak, (double) floorDb) - 3.0f);

        return a;
    }

    const Analysis& getAnalysis (int bands, double low, double high)
    {
        static std::vector<std::pair<std::array<int, 3>, Analysis>> cache;
        const std::array<int, 3> key { bands, juce::roundToInt (low), juce::roundToInt (high) };

        for (auto& entry : cache)
            if (entry.first == key)
                return entry.second;

        if (cache.size() > 12)
            cache.erase (cache.begin());

        cache.push_back ({ key, analyse (bands, low, high) });
        return cache.back().second;
    }

    std::vector<float> getVoiceDb (const Analysis& a, double t)
    {
        std::vector<float> out ((size_t) a.bands);

        for (int b = 0; b < a.bands; ++b)
            out[(size_t) b] = a.at ((int) (t * framesPerSecond), b);

        return out;
    }

    /** How much of the example carrier (a saw on a C major chord, 2 octaves) each band holds, in dB under its loudest band. */
    std::vector<float> getCarrierDb (int bands, double low, double high, double rate)
    {
        const auto edges = vocoder::getBandEdges (bands, low, high, rate);
        const auto notes = vocoder::getChordNotes (0x091, 2);
        std::vector<double> power ((size_t) bands, 0.0);

        for (const int note : notes)
        {
            const double f0 = vocoder::getNoteHz (note);

            for (int n = 1; n * f0 < edges.back(); ++n)
                for (int b = 0; b < bands; ++b)
                    if (n * f0 >= edges[(size_t) b] && n * f0 < edges[(size_t) b + 1])
                        power[(size_t) b] += 1.0 / ((double) n * n);
        }

        const double top = *std::max_element (power.begin(), power.end());
        std::vector<float> out ((size_t) bands, floorDb);

        for (int b = 0; b < bands; ++b)
            if (power[(size_t) b] > 0.0 && top > 0.0)
                out[(size_t) b] = juce::jmax (floorDb, (float) (10.0 * std::log10 (power[(size_t) b] / top)));

        return out;
    }

    /** What comes out: the voice's level in each band while the carrier has something there (at most 12 dB down), else nothing. */
    std::vector<float> getResultDb (const std::vector<float>& voice, const std::vector<float>& carrier)
    {
        std::vector<float> out (voice.size(), floorDb);

        for (size_t b = 0; b < voice.size(); ++b)
            if (carrier[b] > floorDb)
                out[b] = juce::jmax (floorDb, voice[b] + juce::jmax (-12.0f, carrier[b]));

        return out;
    }

    //==============================================================================
    void paintBars (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<float>& db, juce::Colour colour)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (area, 3.0f);
        const float w = area.getWidth() / (float) juce::jmax (1, (int) db.size());

        for (size_t b = 0; b < db.size(); ++b)
        {
            const float h = juce::jlimit (0.0f, 1.0f, (db[b] - floorDb) / -floorDb) * (area.getHeight() - 4.0f);

            if (h < 0.5f)
                continue;

            g.setColour (colour);
            g.fillRect (area.getX() + (float) b * w + 1.0f, area.getBottom() - 2.0f - h, juce::jmax (1.0f, w - 2.0f), h);
        }
    }

    /** The three strips (voice, carrier, result) with their names, one bar for each band. */
    void paintStrips (juce::Graphics& g, juce::Rectangle<float> area, const std::vector<float>& voice, const std::vector<float>& carrier,
                      const std::vector<float>& result, bool withNames = true)
    {
        const float gap = 6.0f, rowHeight = (area.getHeight() - 2.0f * gap) / 3.0f;
        const juce::String names[] { "Voice", "Carrier", "Result" };
        const std::vector<float>* data[] { &voice, &carrier, &result };
        const juce::Colour colours[] { voiceColour, carrierColour, resultColour };
        const float labelWidth = withNames ? 58.0f : 0.0f;

        for (int row = 0; row < 3; ++row)
        {
            auto strip = juce::Rectangle<float> (area.getX(), area.getY() + (float) row * (rowHeight + gap), area.getWidth(), rowHeight);

            if (withNames)
            {
                g.setColour (colours[row]);
                g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
                g.drawText (names[row], strip.removeFromLeft (labelWidth), juce::Justification::centredLeft);
            }

            paintBars (g, strip, *data[row], colours[row]);
        }
    }

    /** The phrase as a bar of its four sounds with the playhead on it. */
    void paintPhrase (juce::Graphics& g, juce::Rectangle<float> area, double t)
    {
        const double total = vocoder::getTestVoiceSeconds();
        auto xFor = [&] (double s) { return area.getX() + (float) (s / total) * area.getWidth(); };
        const juce::String names[] { "ah", "ee", "oo", "s" };

        g.setFont (juce::FontOptions (11.5f));

        for (int i = 0; i < 4; ++i)
        {
            const double start = i < 3 ? vowelStarts[i] : hissStart, length = i < 3 ? vowelLengths[i] : hissLength;
            auto box = juce::Rectangle<float> (xFor (start), area.getY(), xFor (start + length) - xFor (start), area.getHeight());
            g.setColour (theme::panelLight);
            g.fillRoundedRectangle (box, 3.0f);
            g.setColour (theme::text);
            g.drawText (names[i], box, juce::Justification::centred);
        }

        g.setColour (theme::playhead);
        g.fillRect (xFor (t) - 1.0f, area.getY() - 3.0f, 2.0f, area.getHeight() + 6.0f);
    }

    //==============================================================================
    void paintWhat (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p, double since)
    {
        caption (g, r, "Illustration with the made-up voice \"ah ee s\" and a saw on C major, at your Bands, Low and High (not Formant or Smoothing)");
        const auto s = p.getSettings();
        const double rate = getSampleRateOf (p), t = getPosition (since);
        const auto& a = getAnalysis (s.bands, s.lowHz, vocoder::getTopEdge (s.highHz, getSampleRateOf (p)));
        const auto voice = getVoiceDb (a, t);
        const auto carrier = getCarrierDb (s.bands, s.lowHz, s.highHz, rate);
        paintPhrase (g, r.removeFromBottom (26.0f), t);
        r.removeFromBottom (10.0f);
        paintStrips (g, r, voice, carrier, getResultDb (voice, carrier));
    }

    void paintModulatorCarrier (juce::Graphics& g, juce::Rectangle<float> r, double since)
    {
        caption (g, r, "Two sounds in, one out: the result has the carrier's notes and the voice's movement");
        const auto& voice = getVoice();
        const double total = vocoder::getTestVoiceSeconds(), t = getPosition (since);
        const float gap = 8.0f, rowHeight = (r.getHeight() - 2.0f * gap) / 3.0f;
        const int columns = juce::jmax (1, (int) r.getWidth() - 70);
        const juce::String names[] { "Voice", "Carrier", "Result" };
        const juce::Colour colours[] { voiceColour, carrierColour, resultColour };
        std::vector<float> envelope ((size_t) columns);

        for (int row = 0; row < 3; ++row)
        {
            auto strip = juce::Rectangle<float> (r.getX(), r.getY() + (float) row * (rowHeight + gap), r.getWidth(), rowHeight);
            g.setColour (colours[row]);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (names[row], strip.removeFromLeft (64.0f), juce::Justification::centredLeft);
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (strip, 3.0f);
            const float mid = strip.getCentreY(), half = strip.getHeight() * 0.45f;

            for (int x = 0; x < columns; ++x)
            {
                const size_t first = (size_t) ((double) x / columns * (double) voice.size()), last = juce::jmin (voice.size(), (size_t) ((double) (x + 1) / columns * (double) voice.size()));
                float peak = 0.0f;

                for (size_t i = first; i < last; ++i)
                    peak = juce::jmax (peak, std::abs (voice[i]));

                envelope[(size_t) x] = peak;
            }

            const float top = juce::jmax (0.01f, *std::max_element (envelope.begin(), envelope.end()));
            g.setColour (colours[row].withAlpha (0.85f));

            for (int x = 0; x < columns; ++x)
            {
                const float level = row == 1 ? 0.8f : envelope[(size_t) x] / top * (row == 0 ? 1.0f : 0.8f);
                g.fillRect (strip.getX() + (float) x + 1.0f, mid - half * level, 1.0f, juce::jmax (1.0f, 2.0f * half * level));
            }

            g.setColour (theme::playhead);
            g.fillRect (strip.getX() + (float) (t / total) * (float) columns, strip.getY(), 1.5f, strip.getHeight());
        }
    }

    void paintBandsPage (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p)
    {
        const auto s = p.getSettings();
        const double rate = getSampleRateOf (p);
        const auto edges = vocoder::getBandEdges (s.bands, s.lowHz, s.highHz, rate);
        caption (g, r, juce::String (s.bands) + " bands from " + vg::formatHz (edges.front()) + " to " + vg::formatHz (edges.back())
                           + ", each " + juce::String (vocoder::getBandWidthSemitones (s.bands, s.lowHz, s.highHz, rate), 1) + " semitones wide");
        auto area = r.reduced (4.0f, 0.0f);
        auto axis = area.removeFromBottom (24.0f);
        const double lo = 40.0, hi = 14000.0;
        auto xFor = [&] (double hz) { return area.getX() + (float) (std::log (hz / lo) / std::log (hi / lo)) * area.getWidth(); };

        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (area, 3.0f);
        g.setColour (theme::meterYellow.withAlpha (0.12f));
        g.fillRect (juce::Rectangle<float> (xFor (300.0), area.getY(), xFor (3400.0) - xFor (300.0), area.getHeight()));
        g.setColour (theme::meterYellow.withAlpha (0.8f));
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("telephone, 300 to 3,400 Hz", juce::Rectangle<float> (xFor (300.0) + 4.0f, area.getY() + 2.0f, 220.0f, 16.0f), juce::Justification::centredLeft);

        for (size_t k = 0; k < edges.size(); ++k)
        {
            g.setColour (resultColour.withAlpha (0.8f));
            g.fillRect (xFor (edges[k]) - 0.5f, area.getY() + 20.0f, 1.0f, area.getHeight() - 20.0f);
        }

        // The vowels' formants: two peaks each, as in the Peterson and Barney averages.
        struct Vowel { const char* name; double first, second; };
        const Vowel vowels[] { { "ah", 730.0, 1090.0 }, { "ee", 270.0, 2290.0 }, { "oo", 300.0, 870.0 } };

        for (int i = 0; i < 3; ++i)
        {
            const float y = area.getY() + 40.0f + (float) i * 26.0f;
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (vowels[i].name, juce::Rectangle<float> (area.getX() + 6.0f, y - 8.0f, 30.0f, 16.0f), juce::Justification::centredLeft);

            for (const double hz : { vowels[i].first, vowels[i].second })
            {
                g.setColour (voiceColour);
                g.fillEllipse (xFor (hz) - 5.0f, y - 5.0f, 10.0f, 10.0f);
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (10.5f));
                g.drawText (vg::formatHz (hz), juce::Rectangle<float> (xFor (hz) + 7.0f, y - 8.0f, 70.0f, 16.0f), juce::Justification::centredLeft);
            }
        }

        const double bandWidth = vocoder::getBandWidthSemitones (s.bands, s.lowHz, s.highHz, rate);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Every band is " + juce::String (bandWidth, 1) + " semitones wide. Blue dots: the formants of a man's \"ah\", \"ee\" and \"oo\".",
                    juce::Rectangle<float> (area.getX() + 8.0f, area.getBottom() - 26.0f, area.getWidth() - 16.0f, 20.0f), juce::Justification::centredLeft);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f));

        for (const double hz : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
            g.drawText (vg::formatHz (hz), juce::Rectangle<float> (xFor (hz) - 30.0f, axis.getY() + 4.0f, 60.0f, 16.0f), juce::Justification::centred);
    }

    void paintHarmonics (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p)
    {
        caption (g, r, "Illustration: three ideal carriers at 110 Hz under your bands; only a band that holds something can speak");
        const auto s = p.getSettings();
        const auto edges = vocoder::getBandEdges (s.bands, s.lowHz, s.highHz, getSampleRateOf (p));
        const double lo = 50.0, hi = 12000.0;
        const float gap = 8.0f, rowHeight = (r.getHeight() - 2.0f * gap) / 3.0f;
        const juce::String names[] { "Sine: one harmonic", "Saw: harmonic n is 1/n as loud", "Noise: everywhere, no pitch" };

        for (int row = 0; row < 3; ++row)
        {
            auto strip = juce::Rectangle<float> (r.getX() + 4.0f, r.getY() + (float) row * (rowHeight + gap), r.getWidth() - 8.0f, rowHeight);
            auto xFor = [&] (double hz) { return strip.getX() + (float) (std::log (hz / lo) / std::log (hi / lo)) * strip.getWidth(); };
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (strip, 3.0f);

            for (const double e : edges)
            {
                g.setColour (resultColour.withAlpha (0.3f));
                g.fillRect (xFor (e) - 0.5f, strip.getY(), 1.0f, strip.getHeight());
            }

            g.setColour (carrierColour);

            if (row == 0)
                g.fillRect (xFor (110.0) - 1.0f, strip.getY() + 14.0f, 2.0f, strip.getHeight() - 14.0f);
            else if (row == 1)
                for (int n = 1; n * 110.0 < hi; ++n)
                {
                    const float h = juce::jlimit (0.0f, 1.0f, (float) (1.0 + 20.0 * std::log10 (1.0 / n) / 40.0)) * (strip.getHeight() - 14.0f);
                    g.fillRect (xFor (n * 110.0) - 0.5f, strip.getBottom() - h, 1.0f, h);
                }
            else
                g.fillRect (strip.withTrimmedTop (strip.getHeight() * 0.45f).withTrimmedBottom (2.0f).reduced (1.0f, 0.0f));

            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (names[row], strip.reduced (6.0f, 1.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
        }
    }

    void paintHowMany (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p, double since)
    {
        caption (g, r, "Illustration: the same moment of the made-up voice, measured with 4, 8, 16 and 24 bands (your Low and High)");
        const auto s = p.getSettings();
        const double t = getPosition (since);
        const int counts[] { 4, 8, 16, 24 };
        const float gap = 10.0f, w = (r.getWidth() - gap) / 2.0f, h = (r.getHeight() - gap) / 2.0f;

        for (int i = 0; i < 4; ++i)
        {
            auto cell = juce::Rectangle<float> (r.getX() + (float) (i % 2) * (w + gap), r.getY() + (float) (i / 2) * (h + gap), w, h);
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (juce::String (counts[i]) + " bands", cell.removeFromTop (18.0f), juce::Justification::centredLeft);
            paintBars (g, cell, getVoiceDb (getAnalysis (counts[i], s.lowHz, vocoder::getTopEdge (s.highHz, getSampleRateOf (p))), t), voiceColour);
        }
    }

    void paintConsonants (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p, double since)
    {
        const auto s = p.getSettings();
        const double rate = getSampleRateOf (p), total = vocoder::getTestVoiceSeconds(), t = getPosition (since);
        const auto& a = getAnalysis (s.bands, s.lowHz, vocoder::getTopEdge (s.highHz, getSampleRateOf (p)));
        const auto carrier = getCarrierDb (s.bands, s.lowHz, s.highHz, rate);
        const int top = s.bands - 1;
        const float presence = juce::jmax (-12.0f, carrier[(size_t) top]);

        // The synth alone in the top band, and with noise as loud as the voice's hiss (Consonants 100%).
        auto plainDb = [&] (int frame) { return carrier[(size_t) top] > floorDb ? juce::jmax (floorDb, a.at (frame, top) + presence) : floorDb; };
        auto noisyDb = [&] (int frame) { return a.at (frame, top); };
        caption (g, r, "Illustration of the top band over the phrase: the synth alone, and with hiss added as loud as the voice's (Consonants 100%). Not measured from the effect");
        const float gap = 10.0f, rowHeight = (r.getHeight() - gap) / 2.0f;
        const juce::String names[] { "Consonants 0%", "Consonants 100%" };

        for (int row = 0; row < 2; ++row)
        {
            auto strip = juce::Rectangle<float> (r.getX() + 4.0f, r.getY() + (float) row * (rowHeight + gap), r.getWidth() - 8.0f, rowHeight);
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (strip, 3.0f);
            g.setColour (resultColour);

            for (int x = 0; x < (int) strip.getWidth(); ++x)
            {
                const int frame = (int) ((double) x / strip.getWidth() * total * framesPerSecond);
                const float db = row == 0 ? plainDb (frame) : noisyDb (frame);
                const float h = juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb) * (strip.getHeight() - 4.0f);
                g.fillRect (strip.getX() + (float) x, strip.getBottom() - 2.0f - h, 1.0f, h);
            }

            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (names[row], strip.reduced (6.0f, 1.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
            g.setColour (theme::playhead);
            g.fillRect (strip.getX() + (float) (t / total) * strip.getWidth(), strip.getY(), 1.5f, strip.getHeight());
        }
    }

    void paintKeys (juce::Graphics& g, juce::Rectangle<float> area, int lit)
    {
        const bool white[] { true, false, true, false, true, true, false, true, false, true, false, true };
        const int whiteIndex[] { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
        const float w = area.getWidth() / 7.0f;

        for (int pc = 0; pc < 12; ++pc)
            if (white[pc])
            {
                g.setColour ((lit & (1 << pc)) != 0 ? resultColour : juce::Colours::white.withAlpha (0.85f));
                g.fillRect (area.getX() + (float) whiteIndex[pc] * w + 1.0f, area.getY(), w - 2.0f, area.getHeight());
            }

        for (int pc = 0; pc < 12; ++pc)
            if (! white[pc])
            {
                g.setColour ((lit & (1 << pc)) != 0 ? resultColour.darker (0.2f) : juce::Colours::black);
                g.fillRect (area.getX() + ((float) whiteIndex[pc] + 0.68f) * w, area.getY(), w * 0.64f, area.getHeight() * 0.6f);
            }
    }

    void paintBox (juce::Graphics& g, juce::Rectangle<float> box, const juce::String& title, const juce::String& line, juce::Colour colour)
    {
        g.setColour (theme::panel);
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (colour);
        g.drawRoundedRectangle (box, 5.0f, 1.5f);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (title, box.reduced (8.0f, 4.0f).removeFromTop (20.0f), juce::Justification::centredLeft);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawFittedText (line, box.reduced (8.0f, 4.0f).withTrimmedTop (22.0f).toNearestInt(), juce::Justification::topLeft, 3);
    }

    void paintArrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to)
    {
        g.setColour (theme::textDim);
        g.drawArrow (juce::Line<float> (from, to), 1.5f, 8.0f, 8.0f);
    }

    void paintCarrierSources (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p)
    {
        const auto s = p.getSettings();
        const auto label = vg::getCarrierLabel (s.source, p.isUsingFallback(), p.getCarrierPitchMask(), p.getSongChordMask());
        caption (g, r, "Where the carrier's notes come from now: " + label);
        auto keys = r.removeFromBottom (70.0f);
        r.removeFromBottom (12.0f);
        const float boxHeight = 62.0f, boxWidth = r.getWidth() * 0.34f;
        const juce::Colour pink = resultColour;
        auto chord = juce::Rectangle<float> (r.getX() + 4.0f, r.getY() + 4.0f, boxWidth, boxHeight);
        auto played = juce::Rectangle<float> (r.getX() + 4.0f, r.getY() + 84.0f, boxWidth, boxHeight);
        auto notes = juce::Rectangle<float> (r.getRight() - boxWidth - 4.0f, r.getY() + 44.0f, boxWidth, boxHeight);
        paintBox (g, chord, "Song chord", "The chord of the bar, from C3 up. A bar with no chord has none.", pink);
        paintBox (g, played, "Keys", "What you play, on the selected track. Nothing held, another track armed, or a bounce: none.", pink);
        paintBox (g, notes, "Notes", "The root and shape you choose. Always there: it stands in when another source has nothing.", carrierColour);
        paintArrow (g, { chord.getRight(), chord.getCentreY() }, { notes.getX(), notes.getCentreY() - 10.0f });
        paintArrow (g, { played.getRight(), played.getCentreY() }, { notes.getX(), notes.getCentreY() + 10.0f });
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText ("when missing", juce::Rectangle<float> (chord.getRight() + 6.0f, notes.getBottom() + 4.0f, 120.0f, 16.0f), juce::Justification::centredLeft);
        paintKeys (g, keys, p.getCarrierPitchMask());
    }

    void paintPitch (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "Illustration: a spoken monotone (grey, 110 Hz falling to 98 Hz). Full pitch correction would snap it to the nearest note; the vocoder takes the carrier's notes");
        auto area = r.reduced (40.0f, 6.0f);
        const double low = 41.0, high = 58.0;
        auto yFor = [&] (double midi) { return area.getBottom() - (float) ((midi - low) / (high - low)) * area.getHeight(); };
        auto midiOf = [] (double hz) { return 69.0 + 12.0 * std::log2 (hz / 440.0); };
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (area, 3.0f);
        g.setFont (juce::FontOptions (10.5f));

        for (int note = 42; note <= 57; ++note)
        {
            const bool natural = juce::String (vocoder::getPitchName (note % 12)).length() == 1;

            if (natural)
            {
                g.setColour (theme::grid);
                g.fillRect (area.getX(), yFor (note), area.getWidth(), 1.0f);
                g.setColour (theme::textDim);
                g.drawText (vocoder::getPitchName (note % 12) + juce::String (note / 12 - 1), juce::Rectangle<float> (r.getX(), yFor (note) - 7.0f, 38.0f, 14.0f), juce::Justification::centredRight);
            }
        }

        juce::Path voice, tune;
        const int columns = (int) area.getWidth();

        for (int x = 0; x <= columns; ++x)
        {
            const double hz = 110.0 + (98.0 - 110.0) * (double) x / columns, m = midiOf (hz);
            const float px = area.getX() + (float) x;
            x == 0 ? voice.startNewSubPath (px, yFor (m)) : voice.lineTo (px, yFor (m));
            x == 0 ? tune.startNewSubPath (px, yFor (std::round (m))) : tune.lineTo (px, yFor (std::round (m)));
        }

        g.setColour (theme::textDim);
        g.strokePath (voice, juce::PathStrokeType (3.0f));
        g.setColour (voiceColour);
        g.strokePath (tune, juce::PathStrokeType (2.0f));
        g.setColour (resultColour);

        for (const int note : vocoder::getChordNotes (0x091, 1))
            g.fillRect (area.getX(), yFor (note) - 1.0f, area.getWidth(), 2.5f);

        auto legend = juce::Rectangle<float> (area.getX() + 8.0f, area.getY() + 4.0f, 420.0f, 16.0f);
        g.setFont (juce::FontOptions (11.5f));
        g.setColour (theme::textDim);
        g.drawText ("grey: your voice", legend.removeFromLeft (110.0f), juce::Justification::centredLeft);
        g.setColour (voiceColour);
        g.drawText ("blue: snapped to the nearest note", legend.removeFromLeft (210.0f), juce::Justification::centredLeft);
        g.setColour (resultColour);
        g.drawText ("pink: the vocoder, C E G", legend, juce::Justification::centredLeft);
    }

    void paintChain (juce::Graphics& g, juce::Rectangle<float> r)
    {
        caption (g, r, "A common order: compress first, vocode, shape with EQ, and put reverb on a send");
        const float w = (r.getWidth() - 3.0f * 28.0f) / 4.0f, h = 92.0f, y = r.getY() + 20.0f;
        const juce::String titles[] { "Compressor", "Vocoder", "EQ", "Reverb (send)" };
        const juce::String lines[] { "Evens out the voice so quiet syllables still move the bands.",
                                     "Close, dry voice in; a rich carrier; Carrier level sets the loudness.",
                                     "Takes a little harshness off the top, or adds air.",
                                     "After the vocoder, so the bands never measure the echoes." };

        for (int i = 0; i < 4; ++i)
        {
            auto box = juce::Rectangle<float> (r.getX() + (float) i * (w + 28.0f), y, w, h);
            paintBox (g, box, titles[i], lines[i], i == 1 ? resultColour : theme::accent);

            if (i < 3)
                paintArrow (g, { box.getRight() + 2.0f, box.getCentreY() }, { box.getRight() + 26.0f, box.getCentreY() });
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Others vocode first and compress after: take this as a start, not a rule.", juce::Rectangle<float> (r.getX(), y + h + 16.0f, r.getWidth(), 20.0f), juce::Justification::centredLeft);
    }

    void paintYourTrack (juce::Graphics& g, juce::Rectangle<float> r, BuiltInVocoder& p, double since)
    {
        const auto summary = p.getSummary();
        const bool heard = summary.inputPeakDb > floorDb;
        const int n = p.getBandsInForce();
        std::vector<float> voice, carrier, result;

        if (heard)
        {
            auto toDb = [] (float level) { return juce::jlimit (floorDb, 0.0f, juce::Decibels::gainToDecibels (level, floorDb)); };

            for (int b = 0; b < n; ++b)
            {
                voice.push_back (toDb (p.getVoiceLevel (b)));
                carrier.push_back (toDb (p.getCarrierLevel (b)));
                result.push_back (toDb (p.getResultLevel (b)));
            }

            caption (g, r, "Your track, band by band, now (levels as the effect measures them)");
        }
        else
        {
            const auto s = p.getSettings();
            voice = getVoiceDb (getAnalysis (s.bands, s.lowHz, vocoder::getTopEdge (s.highHz, getSampleRateOf (p))), getPosition (since));
            carrier = getCarrierDb (s.bands, s.lowHz, s.highHz, getSampleRateOf (p));
            result = getResultDb (voice, carrier);
            caption (g, r, "Nothing heard from this track, so this illustrates the made-up phrase \"ah ee s\" and a saw on C major");
        }

        paintStrips (g, r, voice, carrier, result);
    }

    juce::String getYourTrackText_ (BuiltInVocoder& p)
    {
        vocoder::SongKey key;
        key.mask = p.getSongKeyMask();
        const auto lines = vocoder::getAdvice (p.getSummary(), p.getSettings(), key);
        return vg::getPageText (9) + "\n\n" + lines.joinIntoString ("\n\n");
    }
} // namespace

juce::String getYourTrackText (BuiltInVocoder& p) { return getYourTrackText_ (p); }

std::vector<fxui::Guide::Page> makeGuide (BuiltInVocoder& processor)
{
    auto* p = &processor;
    std::vector<fxui::Guide::Page> pages;

    for (int i = 0; i < vg::numPages; ++i)
        pages.push_back ({ vg::getPageName (i), vg::getPageSummary (i), vg::getPageText (i), {} });

    pages[0].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double since) { paintWhat (g, r, *p, since); };
    pages[1].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double since) { paintModulatorCarrier (g, r, since); };
    pages[2].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintBandsPage (g, r, *p); };
    pages[3].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintHarmonics (g, r, *p); };
    pages[4].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double since) { paintHowMany (g, r, *p, since); };
    pages[5].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double since) { paintConsonants (g, r, *p, since); };
    pages[6].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintCarrierSources (g, r, *p); };
    pages[7].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintPitch (g, r); };
    pages[8].paint = [] (juce::Graphics& g, juce::Rectangle<float> r, double) { paintChain (g, r); };
    pages[9].paint = [p] (juce::Graphics& g, juce::Rectangle<float> r, double since) { paintYourTrack (g, r, *p, since); };
    pages[9].liveText = [p] { return getYourTrackText_ (*p); };

    // The pages about the phrase play it through the effect; the others only draw an idea.
    for (const int i : { 0, 5, 9 })
    {
        pages[(size_t) i].ownClap = [p] { p->playTestVoice(); };
        pages[(size_t) i].clapText = "Say \"ah ee s\"";
        pages[(size_t) i].clapTooltip = "Plays a made-up voice saying \"ah ee s\" through DawIt Vocoder, on top of what comes in.";
    }

    // The pages that compare sounds have one button for each, all playing the same phrase through the real effect.
    using Part = BuiltInVocoder::DemoPart;
    using Carrier = BuiltInVocoder::DemoCarrier;
    auto example = [p] (const juce::String& text, const juce::String& tip, Part part, int bands, Carrier carrier)
    {
        return fxui::Guide::Page::Listen { text, tip, [p, part, bands, carrier] { p->playDemo (part, bands, carrier); } };
    };

    pages[1].listen = { example ("Voice alone", "Plays the made-up voice \"ah ee s\" as it is, with no effect.", Part::voiceAlone, 0, Carrier::asSet),
                        example ("Carrier alone", "Plays the carrier on its own, with the Source, Wave and notes of the window's settings.", Part::carrierAlone, 0, Carrier::asSet),
                        example ("Result", "Plays the carrier shaped by the voice: the vocoded phrase.", Part::result, 0, Carrier::asSet) };
    pages[3].listen = { example ("Sine carrier", "The phrase through a pure sine carrier: one harmonic per note, so little of the voice's shape comes through.", Part::result, 0, Carrier::sine),
                        example ("Saw carrier", "The phrase through a saw carrier: every harmonic is there for the bands to shape.", Part::result, 0, Carrier::saw),
                        example ("Noise carrier", "The phrase through noise: every frequency is there, so you hear a whisper.", Part::result, 0, Carrier::noise) };

    for (const int bands : { 4, 8, 16, 24 })
        pages[4].listen.push_back (example (juce::String (bands) + " bands", "The phrase through " + juce::String (bands) + " bands, with the window's other settings.", Part::result, bands, Carrier::asSet));

    for (const int i : { 1, 2, 3, 4, 6, 7, 8 })
        pages[(size_t) i].noClap = true;

    return pages;
}
} // namespace daw::voctext
