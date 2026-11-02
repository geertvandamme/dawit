#include "BuiltInSpectralFreeze.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace sfz
{
    namespace
    {
        constexpr double pi = juce::MathConstants<double>::pi;
        constexpr double twoPi = juce::MathConstants<double>::twoPi;

        juce::String minus() { return juce::String::fromUTF8 ("\xe2\x88\x92"); }

        /** "-66" with a real minus sign, "+4", "0". */
        juce::String signedText (int n, bool showPlus)
        {
            return n < 0 ? minus() + juce::String (-n) : (n > 0 && showPlus ? "+" : "") + juce::String (n);
        }

        /** "220 Hz", "1,250 Hz". */
        juce::String formatHz (double hz)
        {
            const int n = juce::roundToInt (hz);
            return n >= 1000 ? juce::String (n / 1000) + "," + juce::String (n % 1000).paddedLeft ('0', 3) + " Hz" : juce::String (n) + " Hz";
        }

        double wrapPhase (double p)
        {
            return p - twoPi * std::round (p / twoPi);
        }

        /** The size of a Hann window's peak at `offset` bins from its centre, relative to the centre (1 at 0, 0.5 at 1, 0 at 2). */
        double hannResponse (double offset)
        {
            const double a = std::abs (offset);

            if (a < 1.0e-9)
                return 1.0;

            if (std::abs (1.0 - a * a) < 1.0e-6)
                return 0.5;

            return std::abs (std::sin (pi * a) / (pi * a * (1.0 - a * a)));
        }
    }

    int getFftSize (double sampleRate)
    {
        return sampleRate < 60000.0 ? 4096 : sampleRate <= 100000.0 ? 8192 : 16384;
    }

    double getBinSpacingHz (double sampleRate, int fftSize) { return sampleRate / fftSize; }
    double getWindowMs (double sampleRate, int fftSize) { return 1000.0 * fftSize / sampleRate; }

    int countBins (double lowHz, double highHz, double sampleRate, int fftSize)
    {
        const double spacing = getBinSpacingHz (sampleRate, fftSize);
        int count = 0;

        for (int k = 0; k < getNumBins (fftSize); ++k)
            if (k * spacing >= lowHz && k * spacing < highHz)
                ++count;

        return count;
    }

    double getSemitoneGapBins (double hz, double sampleRate, int fftSize)
    {
        return hz * (std::pow (2.0, 1.0 / 12.0) - 1.0) / getBinSpacingHz (sampleRate, fftSize);
    }

    std::vector<float> makeHann (int size)
    {
        std::vector<float> w ((size_t) size);

        for (int i = 0; i < size; ++i)
            w[(size_t) i] = (float) (0.5 * (1.0 - std::cos (twoPi * i / size)));

        return w;
    }

    double getOverlapSum (int size, int at)
    {
        const int hopSize = getHopSize (size);
        double sum = 0.0;

        for (int j = 0; j < overlap; ++j)
        {
            const double w = 0.5 * (1.0 - std::cos (twoPi * (at % hopSize + j * hopSize) / size));
            sum += w * w;
        }

        return sum;
    }

    int getCaptureSpan (int fftSize) { return fftSize + (captureFrames - 1) * getHopSize (fftSize); }

    double getTiltDb (double hz, double dbPerOctave)
    {
        return dbPerOctave * std::log2 (juce::jmax (1.0, hz) / 1000.0);
    }

    std::pair<double, double> getFadeGains (double progress, double mixPercent)
    {
        const double angle = juce::jlimit (0.0, 1.0, progress) * juce::jlimit (0.0, 1.0, mixPercent / 100.0) * pi / 2.0;
        return { std::cos (angle), std::sin (angle) };
    }

    double getPhaseVocoderHz (int bin, double earlierPhase, double laterPhase, double sampleRate, int fftSize)
    {
        const int hopSize = getHopSize (fftSize);
        const double expected = twoPi * bin * hopSize / fftSize;
        const double deviation = wrapPhase (laterPhase - earlierPhase - expected);
        return (bin + deviation * fftSize / (twoPi * hopSize)) * sampleRate / fftSize;
    }

    double getSpectralFlatness (const double* powers, int count)
    {
        if (count <= 0)
            return 0.0;

        double sumLog = 0.0, sum = 0.0;

        for (int i = 0; i < count; ++i)
        {
            const double p = juce::jmax (1.0e-12, powers[i]);
            sumLog += std::log (p);
            sum += p;
        }

        return std::exp (sumLog / count) / (sum / count);
    }

    Note getNearestNote (double hz)
    {
        Note n;

        if (hz <= 0.0)
            return n;

        const double exact = 69.0 + 12.0 * std::log2 (hz / 440.0);
        n.midi = juce::jlimit (0, 127, juce::roundToInt (exact));
        n.cents = 100.0 * (exact - n.midi);
        return n;
    }

    int getPitchClass (int midi) { return ((midi % 12) + 12) % 12; }

    juce::String getNoteName (double hz)
    {
        if (hz <= 0.0)
            return {};

        static const char* const names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int midi = getNearestNote (hz).midi;
        return juce::String (names[getPitchClass (midi)]) + juce::String (midi / 12 - 1);
    }

    //==============================================================================
    void Snapshot::prepare (double sampleRate, int fftSize, int numChannels)
    {
        rate = sampleRate;
        size = fftSize;
        channels = numChannels;
        priorPeakRms = 0.0f;

        for (size_t c = 0; c < 2; ++c)
        {
            level[c].assign ((size_t) sfz::getNumBins (fftSize), 0.0f);
            freq[c].assign ((size_t) sfz::getNumBins (fftSize), 0.0f);
            phase[c].assign ((size_t) sfz::getNumBins (fftSize), 0.0f);
        }
    }

    Snapshot remapSnapshot (const Snapshot& source, double newRate, int newSize)
    {
        Snapshot out;
        out.prepare (newRate, newSize, source.channels);
        out.priorPeakRms = source.priorPeakRms;

        if (! source.isHeld())
            return out;

        const int sourceBins = source.getNumBins(), newBins = out.getNumBins();
        const double powerScale = std::pow ((double) newSize / source.size, 2.0);

        for (int c = 0; c < source.channels; ++c)
        {
            const auto ch = (size_t) c;
            std::vector<double> energy ((size_t) newBins, 0.0), freqSum ((size_t) newBins, 0.0);

            for (int k = 0; k < sourceBins; ++k)
            {
                const double e = (double) source.level[ch][(size_t) k] * source.level[ch][(size_t) k] * powerScale;
                const double f = source.freq[ch][(size_t) k];
                const int d = juce::roundToInt (f * newSize / newRate);

                if (e <= 0.0 || f < 0.0 || d < 0 || d >= newBins)
                    continue;

                energy[(size_t) d] += e;
                freqSum[(size_t) d] += e * f;
            }

            std::vector<double> power ((size_t) newBins, 0.0), powerFreq ((size_t) newBins, 0.0);

            for (int d = 0; d < newBins; ++d)
            {
                const double e = energy[(size_t) d];

                if (e <= 0.0)
                    continue;

                const double f = freqSum[(size_t) d] / e;
                const double fractional = f * newSize / newRate;
                double shapes[5];
                double total = 0.0;

                for (int i = -2; i <= 2; ++i)
                {
                    const double r = hannResponse (d + i - fractional);
                    shapes[i + 2] = r * r;
                    total += shapes[i + 2];
                }

                for (int i = -2; i <= 2; ++i)
                {
                    const int b = d + i;

                    if (b < 0 || b >= newBins)
                        continue;

                    const double share = e * shapes[i + 2] / total;
                    power[(size_t) b] += share;
                    powerFreq[(size_t) b] += share * f;
                }
            }

            for (int b = 0; b < newBins; ++b)
            {
                const auto i = (size_t) b;
                out.level[ch][i] = (float) std::sqrt (power[i]);
                out.freq[ch][i] = power[i] > 0.0 ? (float) (powerFreq[i] / power[i]) : (float) (b * newRate / newSize);
                out.phase[ch][i] = (float) wrapPhase (-pi * b);   // a sine peaking at the middle of the piece
            }
        }

        return out;
    }

    double getSnapshotPower (const Snapshot& s, int channel)
    {
        if (! s.isHeld())
            return 0.0;

        const auto& lv = s.level[(size_t) juce::jlimit (0, 1, channel)];
        const int bins = s.getNumBins();
        double sum = (double) lv[0] * lv[0] + (double) lv[(size_t) (bins - 1)] * lv[(size_t) (bins - 1)];

        for (int k = 1; k < bins - 1; ++k)
            sum += 2.0 * lv[(size_t) k] * lv[(size_t) k];

        return 8.0 * sum / (3.0 * (double) s.size * s.size);
    }

    double getSnapshotRms (const Snapshot& s)
    {
        if (! s.isHeld())
            return 0.0;

        double sum = 0.0;

        for (int c = 0; c < s.channels; ++c)
            sum += getSnapshotPower (s, c);

        return std::sqrt (sum / s.channels);
    }

    double getSnapshotFlatness (const Snapshot& s, int channel)
    {
        if (! s.isHeld())
            return 0.0;

        const auto& lv = s.level[(size_t) juce::jlimit (0, 1, channel)];
        const double spacing = getBinSpacingHz (s.rate, s.size);
        std::vector<double> powers;

        for (int k = 0; k < s.getNumBins(); ++k)
        {
            const double hz = k * spacing;

            if (hz >= 50.0 && hz <= 16000.0)
            {
                const double amplitude = 4.0 * lv[(size_t) k] / s.size;
                powers.push_back (amplitude * amplitude);
            }
        }

        // Only the band that holds the sound counts: a cymbal's wash between 2 and 8 kHz is noise however empty the rest is.
        // The band is every bin whose neighbourhood (about 40 bins) is within 20 dB of the strongest neighbourhood.
        const int count = (int) powers.size(), half = 20;

        if (count <= 2 * half)
            return getSpectralFlatness (powers.data(), count);

        std::vector<double> smooth ((size_t) count, 0.0);
        double running = 0.0;

        for (int i = 0; i < count; ++i)
        {
            running += powers[(size_t) i];

            if (i > 2 * half)
                running -= powers[(size_t) (i - 2 * half - 1)];

            if (i >= 2 * half)
                smooth[(size_t) (i - half)] = running / (2 * half + 1);
        }

        double strongest = 0.0;

        for (auto v : smooth)
            strongest = juce::jmax (strongest, v);

        std::vector<double> inBand;

        for (int i = half; i < count - half; ++i)
            if (smooth[(size_t) i] >= strongest * 0.01)
                inBand.push_back (powers[(size_t) i]);

        return getSpectralFlatness (inBand.data(), (int) inBand.size());
    }

    double getSnapshotPeakHz (const Snapshot& s)
    {
        if (! s.isHeld())
            return 0.0;

        int best = -1;
        float loudest = 0.0f;

        for (int c = 0; c < s.channels; ++c)
            for (int k = 0; k < s.getNumBins(); ++k)
                if (s.level[(size_t) c][(size_t) k] > loudest)
                    loudest = s.level[(size_t) c][(size_t) k], best = k + c * s.getNumBins();

        if (best < 0)
            return 0.0;

        return s.freq[(size_t) (best / s.getNumBins())][(size_t) (best % s.getNumBins())];
    }

    juce::String snapshotToText (const Snapshot& s)
    {
        if (! s.isHeld())
            return {};

        juce::MemoryBlock block;

        for (int c = 0; c < s.channels; ++c)
            for (auto* values : { &s.level[(size_t) c], &s.freq[(size_t) c], &s.phase[(size_t) c] })
                block.append (values->data(), values->size() * sizeof (float));

        return juce::Base64::toBase64 (block.getData(), block.getSize());
    }

    Snapshot snapshotFromText (const juce::String& text, double rate, int size, int channels, float priorPeakRms)
    {
        Snapshot s;
        const bool sizeOk = size == 4096 || size == 8192 || size == 16384;

        if (! sizeOk || channels < 1 || channels > 2 || rate < 8000.0 || rate > 384000.0)
            return s;

        juce::MemoryOutputStream out;

        if (! juce::Base64::convertFromBase64 (out, text))
            return s;

        const auto bins = (size_t) getNumBins (size);

        if (out.getDataSize() != (size_t) channels * 3 * bins * sizeof (float))
            return s;

        s.prepare (rate, size, channels);
        s.priorPeakRms = priorPeakRms;
        const auto* data = static_cast<const char*> (out.getData());

        for (size_t c = 0; c < (size_t) channels; ++c)
            for (auto* values : { &s.level[c], &s.freq[c], &s.phase[c] })
            {
                std::memcpy (values->data(), data, bins * sizeof (float));
                data += bins * sizeof (float);
            }

        return s;
    }

    //==============================================================================
    juce::String formatKnobValue (Knob k, float v)
    {
        switch (k)
        {
            case Knob::fade:
            {
                if (v < 1000.0f)
                    return juce::String (juce::roundToInt (v)) + " ms";

                const int tenths = juce::roundToInt (v / 100.0f);
                return tenths >= 100 ? "10 s" : juce::String (tenths / 10) + "." + juce::String (tenths % 10) + " s";
            }
            case Knob::brightness: return (v > 0.04f ? "+" : "") + juce::String (v, 1) + " dB/oct";
            case Knob::movement:
            case Knob::mix:        return juce::String (juce::roundToInt (v)) + "%";
            case Knob::output:     return juce::String (v, 1) + " dB";
        }

        return {};
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        switch (k)
        {
            case Knob::fade:       return formatKnobValue (k, s.fadeMs);
            case Knob::brightness: return formatKnobValue (k, s.brightnessDb);
            case Knob::movement:   return formatKnobValue (k, s.movementPercent);
            case Knob::mix:        return formatKnobValue (k, s.mixPercent);
            case Knob::output:     return formatKnobValue (k, s.outputDb);
        }

        return {};
    }

    const std::vector<Preset>& getPresets()
    {
        static const auto presets = []
        {
            auto make = [] (const char* name, float fade, float brightness, float movement, float mix, const char* why)
            {
                Preset p;
                p.name = name;
                p.why = why;
                p.settings.fadeMs = fade;
                p.settings.brightnessDb = brightness;
                p.settings.movementPercent = movement;
                p.settings.mixPercent = mix;
                return p;
            };

            return std::vector<Preset> {
                make ("Held chord", 300.0f, 0.0f, 10.0f, 100.0f, "Only the hold, still and clean, like a piano chord with the pedal down for ever."),
                make ("Pad under the playing", 1500.0f, -1.0f, 30.0f, 50.0f, "The hold swells in slowly under what you play, a little darker so it sits behind."),
                make ("Breath texture", 2000.0f, 1.0f, 60.0f, 100.0f, "A breath or cymbal becomes a moving wash; Movement keeps noise from sounding mechanical."),
                make ("Dark drone", 3000.0f, -3.0f, 20.0f, 70.0f, "The highs rolled away: a low bed under a verse that doesn't fight the vocal."),
                make ("Shimmer", 1000.0f, 2.0f, 100.0f, 60.0f, "Every frequency wanders: a glittering, restless hold above the track."),
                make ("Sudden stop", 10.0f, 0.0f, 0.0f, 100.0f, "The sound stops dead on one held moment, an effect for a break; 10 ms is fast but doesn't click.")
            };
        }();

        return presets;
    }

    //==============================================================================
    std::vector<juce::String> analyse (const AnalysisInput& in)
    {
        std::vector<juce::String> lines;

        if (! in.holding)
        {
            lines.push_back ("Nothing is held. Play your track and click Freeze while the sound you want rings.");
            return lines;
        }

        const int holdDb = juce::roundToInt (in.holdDb);

        if (in.holdDb < -60.0)
        {
            lines.push_back ("The hold is almost silent (" + signedText (holdDb, false) + " dBFS): the 0.15 s or so before you clicked were quiet. Click Capture again while the sound plays.");
            return lines;
        }

        if (in.priorPeakDb - in.holdDb >= 10.0)
            lines.push_back ("You caught the sound as it faded: the hold is " + juce::String (juce::roundToInt (in.priorPeakDb - in.holdDb))
                             + " dB quieter than the loudest moment just before. For a fuller hold, click Capture again just after the note starts.");

        if (in.flatness >= 0.3 && in.movementPercent < 20.0f)
            lines.push_back ("Your hold is mostly noise (like a breath, a cymbal or a drum). At Movement " + juce::String (juce::roundToInt (in.movementPercent))
                             + "% it can sound static or mechanical: try 30 to 60%.");
        else if (in.flatness >= 0.3)
            lines.push_back ("Your hold is mostly noise (like a breath, a cymbal or a drum). At Movement " + juce::String (juce::roundToInt (in.movementPercent))
                             + "% its levels and phases keep wandering, so it moves like a soft wash instead of sounding still and mechanical.");
        else if (in.flatness < 0.1 && in.peakHz > 0.0)
        {
            const auto note = getNearestNote (in.peakHz);
            lines.push_back ("Your hold is mostly notes. Its tallest peak is " + formatHz (in.peakHz) + ", nearest " + getNoteName (in.peakHz) + " ("
                             + signedText (juce::roundToInt (note.cents), true) + " cents); it may be a harmonic of the note played.");

            if (in.keyKnown)
            {
                static const char* const names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
                const int pc = getPitchClass (note.midi);
                const juce::String name (names[pc]);
                const bool inKey = (in.keyPitchClasses & (1 << pc)) != 0;

                lines.push_back (inKey ? name + " is in the song's key (" + in.keyName + "), so the hold can stay under most chords as a drone."
                                       : name + " is not in the song's key (" + in.keyName + "): it will clash with some chords. Capture a note from the key, or hold it only where it fits.");
            }
        }
        else if (in.flatness >= 0.1 && in.flatness < 0.3)
            lines.push_back ("Your hold is a mix of notes and noise (" + signedText (holdDb, false) + " dBFS).");

        return lines;
    }
} // namespace sfz

//==============================================================================
namespace
{
    using sfz::Knob;

    constexpr double pi = juce::MathConstants<double>::pi;
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float v, int) { return sfz::formatKnobValue (k, v); })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                auto value = text.retainCharacters ("-0123456789.").getFloatValue();

                if (k == Knob::fade && ! text.containsIgnoreCase ("ms") && text.containsIgnoreCase ("s"))
                    value *= 1000.0f;

                return value;
            });
    }

    /** A small random generator, so two bounces of the same sound are the same. */
    struct Rng
    {
        uint32_t state = 1;

        void seed (unsigned n) { state = 0x9E3779B9u * (n + 1u); if (state == 0) state = 1; }

        float next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (float) (state >> 8) * (1.0f / 16777216.0f);
        }
    };

    /** The notes of the test chord: A3, C4 and E4, each with harmonics 2 to 6 at 1/n. */
    constexpr std::array<double, 3> chordHz { 220.0, 261.63, 329.63 };
    constexpr double chordScale = 0.064;   // makes the plucked chord peak near -12 dBFS
}

//==============================================================================
/** One held sound being played: the snapshot's bins carried on in phase, turned back into sound a piece at a time. */
struct BuiltInSpectralFreeze::HoldVoice
{
    struct Context
    {
        juce::dsp::FFT* fft = nullptr;
        const std::vector<float>* hann = nullptr;
        float inverseScale = 1.0f;
        float* data = nullptr;
    };

    void allocate (int fftSizeIn, double sampleRate, int numChannels)
    {
        size = fftSizeIn;
        hop = sfz::getHopSize (fftSizeIn);
        bins = sfz::getNumBins (fftSizeIn);
        rate = sampleRate;
        channels = juce::jlimit (1, 2, numChannels);
        wobble.assign ((size_t) bins, 0.0f);
        wobbleTarget.assign ((size_t) bins, 0.0f);
        jitter.assign ((size_t) bins, 0.0f);

        for (size_t c = 0; c < 2; ++c)
        {
            level[c].assign ((size_t) bins, 0.0f);
            log2Hz[c].assign ((size_t) bins, 0.0f);
            phase[c].assign ((size_t) bins, 0.0);
            increment[c].assign ((size_t) bins, 0.0);
            ola[c].assign ((size_t) size, 0.0f);
            ready[c].assign ((size_t) hop, 0.0f);
        }

        active = false;
    }

    void start (const sfz::Snapshot& s, unsigned seed)
    {
        for (int c = 0; c < channels; ++c)
        {
            const auto ch = (size_t) c;
            const auto from = (size_t) juce::jmin (c, s.channels - 1);

            for (int k = 0; k < bins; ++k)
            {
                const auto i = (size_t) k;
                const double f = s.freq[from][i];
                level[ch][i] = s.level[from][i];
                phase[ch][i] = s.phase[from][i];
                increment[ch][i] = twoPi * f * hop / rate;
                log2Hz[ch][i] = (float) std::log2 (juce::jlimit (20.0, rate / 2.0, f) / 1000.0);
            }

            target[ch] = sfz::getSnapshotPower (s, (int) from);
            pBar[ch] = target[ch];
            gain0[ch] = gain1[ch] = 1.0;
            std::fill (ola[ch].begin(), ola[ch].end(), 0.0f);
        }

        std::fill (wobble.begin(), wobble.end(), 0.0f);
        std::fill (wobbleTarget.begin(), wobbleTarget.end(), 0.0f);
        olaHead = pos = blocks = 0;
        primed = false;
        rng.seed (seed);
        active = true;
    }

    /** Builds the piece that starts `skip` samples before the next output sample and lays it over the pieces already waiting. */
    void addPiece (const Context& cx, float movement, float brightnessDb, int skip)
    {
        const float spread = movement * (float) pi;
        const float wobbleDb = movement * 6.0f;
        const float wobbleChance = (float) hop / (float) rate;   // a new level target about once a second
        const float wobbleStep = 1.0f - std::exp (-(float) hop / (0.25f * (float) rate));
        const float oneStepIn = 1.0f / (float) sfz::overlapSum;

        for (int k = 0; k < bins; ++k)
        {
            const auto i = (size_t) k;
            const float chance = rng.next(), value = rng.next(), wander = rng.next();

            if (chance < wobbleChance)
                wobbleTarget[i] = value * 2.0f - 1.0f;

            wobble[i] += (wobbleTarget[i] - wobble[i]) * wobbleStep;
            jitter[i] = (wander * 2.0f - 1.0f) * spread;
        }

        const int half = size / 2;

        for (int c = 0; c < channels; ++c)
        {
            const auto ch = (size_t) c;
            float* data = cx.data;

            for (int k = 0; k <= half; ++k)
            {
                const auto i = (size_t) k;
                double p = phase[ch][i] + increment[ch][i] + (double) jitter[i];
                p -= twoPi * std::round (p / twoPi);
                phase[ch][i] = p;
                const float db = wobble[i] * wobbleDb + brightnessDb * log2Hz[ch][i];
                const float magnitude = level[ch][i] * std::exp2 (db * 0.16609640474f);
                data[2 * k] = magnitude * std::cos ((float) p);
                data[2 * k + 1] = magnitude * std::sin ((float) p);
            }

            data[1] = 0.0f;
            data[2 * half + 1] = 0.0f;
            cx.fft->performRealOnlyInverseTransform (data);
            const float scale = cx.inverseScale * oneStepIn;
            const auto& w = *cx.hann;

            for (int i = skip; i < size; ++i)
            {
                int index = olaHead + i - skip;

                if (index >= size)
                    index -= size;

                ola[ch][(size_t) index] += data[i] * w[(size_t) i] * scale;
            }
        }
    }

    /** Starts the next quarter piece: the pieces still waiting, plus the new one, make the next `hop` samples. */
    void startHop (const Context& cx, float movement, float brightnessDb)
    {
        if (! primed)
        {
            // The three pieces that began before this moment, so the first samples are as loud as the rest.
            for (int back = 3; back >= 1; --back)
                addPiece (cx, movement, brightnessDb, back * hop);

            primed = true;
        }

        addPiece (cx, movement, brightnessDb, 0);
        const double alpha = 1.0 - std::exp (-(double) hop / (0.25 * rate));

        for (int c = 0; c < channels; ++c)
        {
            const auto ch = (size_t) c;
            double sum = 0.0;

            for (int i = 0; i < hop; ++i)
            {
                const auto index = (size_t) ((olaHead + i) % size);
                const float v = ola[ch][index];
                ready[ch][(size_t) i] = v;
                ola[ch][index] = 0.0f;
                sum += (double) v * v;
            }

            const double power = sum / hop;
            const double a = juce::jmax (alpha, 1.0 / (blocks + 1));
            pBar[ch] += (power - pBar[ch]) * (blocks == 0 ? 1.0 : a);
            gain0[ch] = gain1[ch];
            gain1[ch] = target[ch] <= 1.0e-20 ? 0.0 : pBar[ch] > 1.0e-20 ? juce::jlimit (0.25, 8.0, std::sqrt (target[ch] / pBar[ch])) : 1.0;

            if (blocks == 0)
                gain0[ch] = gain1[ch];
        }

        olaHead = (olaHead + hop) % size;
        ++blocks;
    }

    /** The next output sample of each channel. */
    void next (const Context& cx, float movement, float brightnessDb, float* out)
    {
        if (pos == 0)
            startHop (cx, movement, brightnessDb);

        const double fraction = (double) pos / hop;

        for (int c = 0; c < channels; ++c)
        {
            const auto ch = (size_t) c;
            out[c] = ready[ch][(size_t) pos] * (float) (gain0[ch] + (gain1[ch] - gain0[ch]) * fraction);
        }

        if (++pos == hop)
            pos = 0;
    }

    bool active = false, primed = false;
    int size = 0, hop = 0, bins = 0, channels = 2, olaHead = 0, pos = 0, blocks = 0;
    double rate = 44100.0;
    std::array<std::vector<float>, 2> level, log2Hz, ola, ready;
    std::array<std::vector<double>, 2> phase, increment;
    std::vector<float> wobble, wobbleTarget, jitter;
    std::array<double, 2> target {}, pBar {}, gain0 {}, gain1 {};
    Rng rng;
};

//==============================================================================
const char* BuiltInSpectralFreeze::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::fade:       return "fade";
        case Knob::brightness: return "brightness";
        case Knob::movement:   return "movement";
        case Knob::mix:        return "mix";
        case Knob::output:     return "output";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInSpectralFreeze::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const sfz::Settings d;
    auto id = [] (Knob k) { return juce::ParameterID { getParameterId (k), 1 }; };
    auto percent = [] { return juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }; };

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { freezeParameterId, 1 }, "Freeze", d.freeze));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::fade), "Fade", logRange (sfz::minFadeMs, sfz::maxFadeMs), d.fadeMs, knobText (Knob::fade)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::brightness), "Brightness", juce::NormalisableRange<float> { sfz::minBrightnessDb, sfz::maxBrightnessDb, 0.1f },
                                                             d.brightnessDb, knobText (Knob::brightness)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::movement), "Movement", percent(), d.movementPercent, knobText (Knob::movement)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::mix), "Mix", percent(), d.mixPercent, knobText (Knob::mix)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::output), "Output", juce::NormalisableRange<float> { sfz::minOutputDb, sfz::maxOutputDb, 0.1f },
                                                             d.outputDb, knobText (Knob::output)));
    return layout;
}

BuiltInSpectralFreeze::BuiltInSpectralFreeze()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItSpectralFreeze", createLayout())
{
    for (int k = 0; k < sfz::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    freezeValue = parameters.getRawParameterValue (freezeParameterId);
    incomingLock.clear();

    for (auto& g : slotGeneration)
        g = 0;
}

BuiltInSpectralFreeze::~BuiltInSpectralFreeze()
{
    stopTimer();
}

juce::PluginDescription BuiltInSpectralFreeze::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Spectral";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x4446727A;   // "DFrz"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInSpectralFreeze::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInSpectralFreeze::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

juce::RangedAudioParameter* BuiltInSpectralFreeze::getFreezeParameter() const
{
    return parameters.getParameter (freezeParameterId);
}

sfz::Settings BuiltInSpectralFreeze::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    sfz::Settings s;
    s.freeze = freezeValue->load() >= 0.5f;
    s.fadeMs = value (Knob::fade);
    s.brightnessDb = value (Knob::brightness);
    s.movementPercent = value (Knob::movement);
    s.mixPercent = value (Knob::mix);
    s.outputDb = value (Knob::output);
    return s;
}

void BuiltInSpectralFreeze::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInSpectralFreeze::setFreeze (bool on)
{
    if (auto* p = getFreezeParameter())
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

void BuiltInSpectralFreeze::setSettings (const sfz::Settings& s)
{
    setKnob (Knob::fade, s.fadeMs);
    setKnob (Knob::brightness, s.brightnessDb);
    setKnob (Knob::movement, s.movementPercent);
    setKnob (Knob::mix, s.mixPercent);
    setKnob (Knob::output, s.outputDb);
    setFreeze (s.freeze);
}

bool BuiltInSpectralFreeze::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

//==============================================================================
void BuiltInSpectralFreeze::allocate()
{
    fftSize = sfz::getFftSize (rate);
    hop = sfz::getHopSize (fftSize);
    fft = std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 ((double) fftSize)));
    hann = sfz::makeHann (fftSize);
    fftData.assign ((size_t) fftSize * 2, 0.0f);
    spare.assign ((size_t) sfz::getNumBins (fftSize), 0.0f);

    // How the inverse transform scales: a unit impulse should come back as 1.
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    fftData[0] = 1.0f;
    fft->performRealOnlyForwardTransform (fftData.data(), true);
    fft->performRealOnlyInverseTransform (fftData.data());
    inverseScale = std::abs (fftData[0]) > 1.0e-9f ? 1.0f / fftData[0] : 1.0f;

    ringSize = juce::nextPowerOfTwo (juce::roundToInt (rate));

    for (auto& r : ring)
        r = std::make_unique<std::atomic<float>[]> ((size_t) ringSize);

    ringPosition = 0;
    ringWritten = 0;

    for (auto& v : voices)
    {
        v = std::make_unique<HoldVoice>();
        v->allocate (fftSize, rate, numChannelsUsed);
    }
}

void BuiltInSpectralFreeze::prepareToPlay (double sampleRate, int)
{
    stopTimer();
    const double oldRate = rate;
    const int oldSize = fftSize;
    sfz::Snapshot kept;
    bool hadSnapshot = latestSlot.load() >= 0 && oldRate > 0.0;

    if (hadSnapshot)
        getSnapshot (kept);

    // A saved hold that the audio thread has not taken yet is the newest one.
    const bool pendingState = incomingReady.load() && incoming.isHeld();

    if (pendingState)
    {
        kept = incoming;
        hadSnapshot = true;
    }

    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    numChannelsUsed = juce::jlimit (1, 2, getTotalNumInputChannels());
    allocate();
    setLatencySamples (0);

    // The snapshot is kept, for the new rate if it changed. A state loaded before the plugin was prepared replaces it.
    if (restoredBeforePrepare != nullptr)
    {
        kept = *restoredBeforePrepare;
        restoredBeforePrepare.reset();
        useStoredOnRise = true;
    }
    else if (pendingState)
        useStoredOnRise = true;
    else if (hadSnapshot)
        useStoredOnRise = freezeValue->load() >= 0.5f;
    else
        useStoredOnRise = false;

    for (auto& g : slotGeneration)
        g = 0;

    latestSlot = -1;

    for (auto& s : slots)
        s.prepare (rate, fftSize, numChannelsUsed);

    if (kept.isHeld())
    {
        if (kept.size != fftSize || std::abs (kept.rate - rate) > 0.5)
            kept = sfz::remapSnapshot (kept, rate, fftSize);

        auto& w = beginWrite();
        w = kept;
        publish (w);
    }

    juce::ignoreUnused (oldSize);
    incoming.prepare (rate, fftSize, numChannelsUsed);
    incomingReady = false;
    chordRequested = false;
    pairRequested = false;
    pairMode = false;
    chordPosition = -1;
    chordPending = false;
    chordHold = false;
    captureRequested = false;
    holdTarget = false;
    lastFreeze = false;
    crossfading = false;
    holdAngle = 0.0;
    crossfade = 1.0;
    currentVoice = 0;
    voiceCounter = 0;
    holdingFlag = false;
    holdAmount = 0.0f;
    chunkIn = chunkHold = 0.0f;
    chunkCount = 0;

    const auto s = getSettings();
    mixSmoothed.reset (rate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (s.mixPercent);
    outputSmoothed.reset (rate, 0.02);
    outputSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (s.outputDb));
}

sfz::Snapshot& BuiltInSpectralFreeze::beginWrite()
{
    writingSlot = (latestSlot.load() + 1) % 3;
    slotGeneration[(size_t) writingSlot].fetch_add (1, std::memory_order_acq_rel);   // odd: being written
    return slots[(size_t) writingSlot];
}

void BuiltInSpectralFreeze::publish (sfz::Snapshot&)
{
    slotGeneration[(size_t) writingSlot].fetch_add (1, std::memory_order_acq_rel);   // even: complete
    latestSlot.store (writingSlot, std::memory_order_release);
}

bool BuiltInSpectralFreeze::getSnapshot (sfz::Snapshot& dest) const
{
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        const int i = latestSlot.load (std::memory_order_acquire);

        if (i < 0)
            return false;

        const auto index = (size_t) i;
        const unsigned before = slotGeneration[index].load (std::memory_order_acquire);

        if ((before & 1u) != 0)
            continue;

        dest = slots[index];

        if (slotGeneration[index].load (std::memory_order_acquire) == before)
            return dest.isHeld();
    }

    return false;
}

juce::int64 BuiltInSpectralFreeze::readInput (int channel, float* dest, int count) const
{
    channel = juce::jlimit (0, 1, channel);
    count = juce::jlimit (0, ringSize, count);
    const auto written = ringWritten.load (std::memory_order_acquire);

    if (ring[(size_t) channel] == nullptr)
    {
        std::fill (dest, dest + count, 0.0f);
        return 0;
    }

    for (int i = 0; i < count; ++i)
    {
        const auto index = written - count + i;
        dest[i] = index >= 0 ? ring[(size_t) channel][(size_t) (index & (ringSize - 1))].load (std::memory_order_relaxed) : 0.0f;
    }

    return written;
}

//==============================================================================
void BuiltInSpectralFreeze::captureInto (sfz::Snapshot& s)
{
    const int bins = sfz::getNumBins (fftSize);
    const int mask = ringSize - 1;
    s.rate = rate;
    s.size = fftSize;
    s.channels = numChannelsUsed;
    const auto newest = ringPosition;

    for (int c = 0; c < numChannelsUsed; ++c)
    {
        const auto ch = (size_t) c;
        auto& level = s.level[ch];
        std::fill (level.begin(), level.end(), 0.0f);

        for (int frame = 0; frame < sfz::captureFrames; ++frame)
        {
            const auto start = newest - fftSize - (sfz::captureFrames - 1 - frame) * hop;

            for (int i = 0; i < fftSize; ++i)
            {
                const auto index = start + i;
                const float v = index >= 0 ? ring[ch][(size_t) (index & mask)].load (std::memory_order_relaxed) : 0.0f;
                fftData[(size_t) i] = v * hann[(size_t) i];
            }

            std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
            fft->performRealOnlyForwardTransform (fftData.data(), true);

            for (int k = 0; k < bins; ++k)
            {
                const float re = fftData[(size_t) (2 * k)], im = fftData[(size_t) (2 * k + 1)];
                level[(size_t) k] += re * re + im * im;

                if (frame == sfz::captureFrames - 2)
                    spare[(size_t) k] = std::atan2 (im, re);
                else if (frame == sfz::captureFrames - 1)
                    s.phase[ch][(size_t) k] = std::atan2 (im, re);
            }
        }

        for (int k = 0; k < bins; ++k)
        {
            const auto i = (size_t) k;
            level[i] = std::sqrt (level[i] / (float) sfz::captureFrames);
            s.freq[ch][i] = (float) sfz::getPhaseVocoderHz (k, spare[i], s.phase[ch][i], rate, fftSize);
        }
    }

    // The loudest 23 ms of the second before, for the advice.
    float loudest = 0.0f;
    const auto blocks = juce::jmin ((juce::int64) ringSize, newest) / hop;

    for (int c = 0; c < numChannelsUsed; ++c)
        for (juce::int64 b = 0; b < blocks; ++b)
        {
            double sum = 0.0;

            for (int i = 0; i < hop; ++i)
            {
                const float v = ring[(size_t) c][(size_t) ((newest - (b + 1) * hop + i) & mask)].load (std::memory_order_relaxed);
                sum += (double) v * v;
            }

            loudest = juce::jmax (loudest, (float) std::sqrt (sum / hop));
        }

    s.priorPeakRms = loudest;
}

void BuiltInSpectralFreeze::startVoice (const sfz::Snapshot& s)
{
    if (crossfading)
    {
        voices[(size_t) (1 - currentVoice)]->active = false;   // a second capture inside a crossfade: the old voice goes at once
        crossfading = false;
    }

    const bool running = voices[0]->active || voices[1]->active;
    const int index = running ? 1 - currentVoice : currentVoice;
    voices[(size_t) index]->start (s, 0x5F000000u + ++voiceCounter);

    if (running)
    {
        crossfading = true;
        crossfade = 0.0;
    }
    else
        crossfade = 1.0;

    currentVoice = index;
}

void BuiltInSpectralFreeze::captureAndStart()
{
    auto& w = beginWrite();
    captureInto (w);
    publish (w);
    startVoice (w);
}

void BuiltInSpectralFreeze::adoptIncoming()
{
    if (! incomingReady.load (std::memory_order_acquire) || incomingLock.test_and_set (std::memory_order_acquire))
        return;

    if (incoming.size == fftSize && std::abs (incoming.rate - rate) < 0.5)
    {
        auto& w = beginWrite();
        w.rate = incoming.rate;
        w.size = incoming.size;
        w.channels = incoming.channels;
        w.priorPeakRms = incoming.priorPeakRms;

        for (size_t c = 0; c < 2; ++c)
        {
            std::copy (incoming.level[c].begin(), incoming.level[c].end(), w.level[c].begin());
            std::copy (incoming.freq[c].begin(), incoming.freq[c].end(), w.freq[c].begin());
            std::copy (incoming.phase[c].begin(), incoming.phase[c].end(), w.phase[c].begin());
        }

        publish (w);
        useStoredOnRise = true;
        adoptedNow = true;
    }

    incomingReady.store (false, std::memory_order_release);
    incomingLock.clear (std::memory_order_release);
}

void BuiltInSpectralFreeze::handOverState (sfz::Snapshot&& s)
{
    if (rate <= 0.0)
    {
        restoredBeforePrepare = std::make_unique<sfz::Snapshot> (std::move (s));
        return;
    }

    if (s.size != fftSize || std::abs (s.rate - rate) > 0.5)
        s = sfz::remapSnapshot (s, rate, fftSize);

    while (incomingLock.test_and_set (std::memory_order_acquire))
        juce::Thread::yield();

    incoming = std::move (s);
    incomingReady.store (true, std::memory_order_release);
    incomingLock.clear (std::memory_order_release);
}

//==============================================================================
void BuiltInSpectralFreeze::playTestChord()
{
    seenChordCaptures = chordCaptures.load();
    timerTicks = 0;
    chordRequested = true;
    startTimerHz (30);
}

void BuiltInSpectralFreeze::timerCallback()
{
    if (chordCaptures.load() != seenChordCaptures)
    {
        seenChordCaptures = chordCaptures.load();
        setFreeze (true);
        stopTimer();
    }
    else if (++timerTicks > 150)
        stopTimer();
}

float BuiltInSpectralFreeze::getChordSample (double t) const
{
    if (pairMode)
    {
        // A3 and A#3, equal level, -12 dBFS together, with 5 ms at each end so they don't click.
        const double envelope = juce::jmin (1.0, juce::jmin (t, 1.0 - t) / 0.005);
        return (float) (0.125 * envelope * (std::sin (twoPi * 220.0 * t) + std::sin (twoPi * 233.08 * t)));
    }

    double sum = 0.0;

    for (const double f : chordHz)
        for (int n = 1; n <= 6; ++n)
            if (f * n < rate * 0.5)
                sum += std::sin (twoPi * f * n * t) / n;

    const double envelope = juce::jmin (1.0, t / 0.005) * std::exp (-t * 2.302585092994046);   // 20 dB a second
    return (float) (sum * envelope * chordScale);
}

void BuiltInSpectralFreeze::processSegment (juce::AudioBuffer<float>& buffer, int from, int count, int numCh)
{
    const double fadeSamples = juce::jmax (1.0, (double) knobValues[(size_t) Knob::fade]->load() * 0.001 * rate);
    const double step = 1.0 / fadeSamples;
    const float movement = knobValues[(size_t) Knob::movement]->load() * 0.01f;
    const float brightness = knobValues[(size_t) Knob::brightness]->load();
    const HoldVoice::Context cx { fft.get(), &hann, inverseScale, fftData.data() };
    const int chordLength = juce::roundToInt ((pairMode ? 1.0 : sfz::testChordSeconds) * rate);
    const int mask = ringSize - 1;

    for (int i = from; i < from + count; ++i)
    {
        float in[2] {};

        for (int c = 0; c < numCh; ++c)
            in[c] = buffer.getSample (c, i);

        if (chordPosition >= 0)
        {
            const float v = getChordSample (chordPosition / rate);

            for (int c = 0; c < numCh; ++c)
                in[c] += v;

            if (++chordPosition >= chordLength)
                chordPosition = -1;
        }

        for (int c = 0; c < numCh; ++c)
            ring[(size_t) c][(size_t) (ringPosition & mask)].store (in[c], std::memory_order_relaxed);

        ++ringPosition;
        const float mixPercent = mixSmoothed.getNextValue(), outputGain = outputSmoothed.getNextValue();
        float out[2] {};
        float holdPeak = 0.0f;

        if (voices[0]->active || voices[1]->active)
        {
            holdAngle = juce::jlimit (0.0, 1.0, holdAngle + (holdTarget ? step : -step));
            float hold[2] {}, current[2] {}, old[2] {};

            if (crossfading)
            {
                const auto oldIndex = (size_t) (1 - currentVoice);
                voices[oldIndex]->next (cx, movement, brightness, old);
                voices[(size_t) currentVoice]->next (cx, movement, brightness, current);

                for (int c = 0; c < numCh; ++c)
                    hold[c] = old[c] * (float) (1.0 - crossfade) + current[c] * (float) crossfade;

                crossfade += step;

                if (crossfade >= 1.0)
                {
                    crossfade = 1.0;
                    crossfading = false;
                    voices[oldIndex]->active = false;
                }
            }
            else
            {
                voices[(size_t) currentVoice]->next (cx, movement, brightness, current);

                for (int c = 0; c < numCh; ++c)
                    hold[c] = current[c];
            }

            const auto [dry, wet] = sfz::getFadeGains (holdAngle, mixPercent);

            for (int c = 0; c < numCh; ++c)
            {
                const float h = hold[c] * (float) wet * outputGain;
                out[c] = in[c] * (float) dry * outputGain + h;
                holdPeak = juce::jmax (holdPeak, std::abs (h));
            }

            if (! holdTarget && holdAngle <= 0.0)
            {
                voices[0]->active = voices[1]->active = false;
                crossfading = false;
            }
        }
        else
        {
            for (int c = 0; c < numCh; ++c)
                out[c] = in[c] * outputGain;
        }

        float inPeak = 0.0f;

        for (int c = 0; c < numCh; ++c)
        {
            buffer.setSample (c, i, out[c]);
            inPeak = juce::jmax (inPeak, std::abs (in[c]));
        }

        chunkIn = juce::jmax (chunkIn, inPeak);
        chunkHold = juce::jmax (chunkHold, holdPeak);

        if (++chunkCount >= fx::LevelHistory::chunk)
        {
            levels.push ({ juce::Decibels::gainToDecibels (chunkIn, -100.0f), juce::Decibels::gainToDecibels (chunkHold, -100.0f) });
            chunkIn = chunkHold = 0.0f;
            chunkCount = 0;
        }
    }

    ringWritten.store (ringPosition, std::memory_order_release);
}

void BuiltInSpectralFreeze::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (numChannelsUsed, buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    if (ring[0] == nullptr || numCh < 1 || total < 1)
        return;

    adoptedNow = false;
    adoptIncoming();
    const bool freeze = freezeValue->load() >= 0.5f;

    if (freeze && ! lastFreeze)
    {
        if (chordHold)
            chordHold = false;   // the test chord's own capture is the hold
        else if (useStoredOnRise && hasSnapshot())
        {
            sfz::Snapshot& stored = slots[(size_t) latestSlot.load()];
            startVoice (stored);
        }
        else
            captureAndStart();

        useStoredOnRise = false;
    }
    else if (! freeze && lastFreeze)
        useStoredOnRise = false;
    else if (freeze && adoptedNow && hasSnapshot())
        startVoice (slots[(size_t) latestSlot.load()]);   // the saved hold arrived while a hold was already on

    lastFreeze = freeze;
    holdTarget = freeze || chordHold;

    if (captureRequested.exchange (false) && holdTarget)
        captureAndStart();

    if (pairRequested.exchange (false))
    {
        pairMode = true;
        chordPosition = 0;
        chordPending = false;
    }

    if (chordRequested.exchange (false))
    {
        pairMode = false;
        chordPosition = 0;
        chordCaptureAt = juce::roundToInt (sfz::testChordCaptureSeconds * rate);
        chordPending = true;
    }

    mixSmoothed.setTargetValue (knobValues[(size_t) Knob::mix]->load());
    outputSmoothed.setTargetValue (juce::Decibels::decibelsToGain (knobValues[(size_t) Knob::output]->load()));

    int position = 0;

    while (position < total)
    {
        int count = total - position;

        if (chordPending && chordPosition >= 0)
            count = juce::jmin (count, juce::jmax (1, chordCaptureAt - chordPosition));

        processSegment (buffer, position, count, numCh);
        position += count;

        if (chordPending && chordPosition >= chordCaptureAt)
        {
            chordPending = false;
            captureAndStart();
            chordHold = true;
            holdTarget = true;
            ++chordCaptures;
        }
    }

    // Mono layouts have one channel; a wider buffer keeps whatever it had.
    holdingFlag = holdTarget || holdAngle > 0.0;
    holdAmount = (float) holdAngle;
}

juce::AudioProcessorEditor* BuiltInSpectralFreeze::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInSpectralFreeze::getStateInformation (juce::MemoryBlock& dest)
{
    auto xml = parameters.copyState().createXml();

    if (xml == nullptr)
        return;

    sfz::Snapshot s;

    if (freezeValue->load() >= 0.5f && getSnapshot (s))
    {
        auto* hold = xml->createNewChildElement ("Hold");
        hold->setAttribute ("rate", s.rate);
        hold->setAttribute ("size", s.size);
        hold->setAttribute ("channels", s.channels);
        hold->setAttribute ("peak", (double) s.priorPeakRms);
        hold->addTextElement (sfz::snapshotToText (s));
    }

    copyXmlToBinary (*xml, dest);
}

void BuiltInSpectralFreeze::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    sfz::Snapshot restored;

    if (auto* hold = xml->getChildByName ("Hold"))
    {
        restored = sfz::snapshotFromText (hold->getAllSubText().trim(), hold->getDoubleAttribute ("rate"), hold->getIntAttribute ("size"),
                                          hold->getIntAttribute ("channels"), (float) hold->getDoubleAttribute ("peak"));
        xml->removeChildElement (hold, true);
    }

    // The snapshot goes first, so the audio thread has it when it sees Freeze switched on.
    if (restored.isHeld())
        handOverState (std::move (restored));

    parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
