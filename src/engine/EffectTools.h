#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "SungVoice.h"
#include "Tune.h"

/**
    What DawIt Reverb and DawIt Delay share: a history of how loud the sound and the effect were,
    for their windows, and a hand clap to hear the effect with.
*/
namespace daw::fx
{
/** A number with this many decimals; with none, rounded to a whole number (juce::String (x, 0) would print every digit). */
inline juce::String formatNumber (double value, int decimals)
{
    return decimals > 0 ? juce::String (value, decimals) : juce::String (juce::roundToInt (value));
}

/** How loud the dry sound and the effect are at a mix (0-100%): equal power, so the total stays
    about as loud anywhere between all dry and all effect. */
inline std::pair<float, float> getMixGains (float mixPercent)
{
    const float angle = juce::jlimit (0.0f, 1.0f, mixPercent / 100.0f) * juce::MathConstants<float>::halfPi;
    return { std::cos (angle), std::sin (angle) };
}

/** How loud the effect comes out next to the dry sound at a mix, in dB. */
inline double getWetToDryDb (float mixPercent)
{
    const auto [dry, wet] = getMixGains (mixPercent);

    if (wet < 1.0e-6f) return -100.0;
    if (dry < 1.0e-6f) return 100.0;

    return 20.0 * std::log10 ((double) wet / dry);
}

/** The peaks of one short stretch (LevelHistory::chunk samples), in dB: what came in, and what the
    effect added at the level it is mixed in. */
struct LevelPoint
{
    float inputDb = -100.0f, effectDb = -100.0f;
};

/** The newest level points: written by the audio thread, read by one reader (the editor). */
class LevelHistory
{
public:
    static constexpr int chunk = 64;

    /** Drops the point when nobody reads. */
    void push (LevelPoint point) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            points[(size_t) start1] = point;
            fifo.finishedWrite (1);
        }
    }

    /** Takes up to `max` of the newest points, oldest first. */
    int read (LevelPoint* dest, int max)
    {
        int ready = fifo.getNumReady();

        if (ready > max)
        {
            fifo.finishedRead (ready - max);
            ready = max;
        }

        int start1, size1, start2, size2;
        fifo.prepareToRead (ready, start1, size1, start2, size2);
        std::copy_n (points.begin() + start1, size1, dest);
        std::copy_n (points.begin() + start2, size2, dest + size1);
        fifo.finishedRead (size1 + size2);
        return size1 + size2;
    }

private:
    static constexpr int size = 16384;
    juce::AbstractFifo fifo { size };
    std::vector<LevelPoint> points = std::vector<LevelPoint> ((size_t) size);
};

/** A test sound to hear an effect with, peaking near -6 dB. A hand clap: three quick bursts of noise
    and a short ring, about 40 ms. Or a note: a plain A3 (220 Hz sine) that dies away over a second,
    so whatever an effect adds to it stands out. Or hits in hiss (for a gate): the clap at 0, 250, 500
    and 750 ms over white noise peaking at 0.003 (-50.5 dB), for one second. The editor asks for one; the audio thread adds it to
    what goes into the effect, so it is heard dry and with the effect, like clapping in a room. */
class Clap
{
public:
    enum class Sound { clap, note, hitsInHiss, wideBass, offNote };

    void request (Sound which = Sound::clap) noexcept
    {
        requestedSound = which;
        requested = true;
    }

    /** Takes a waiting request, for an instrument that plays its own test note instead of adding this sound. */
    bool takeRequest() noexcept { return requested.exchange (false); }

    void prepare (double sampleRate)
    {
        rate = sampleRate > 0.0 ? sampleRate : 44100.0;
        position = -1;
    }

    /** Adds the clap, while one plays, to the first `numChannels` channels. */
    void addTo (juce::AudioBuffer<float>& buffer, int numChannels)
    {
        if (requested.exchange (false))
        {
            position = 0;
            lowState = 0.0f;
            wideLowState[0] = wideLowState[1] = 0.0f;
            sound = requestedSound.load();
            voice.prepare (rate);
        }

        if (position < 0)
            return;

        const int length = (int) ((sound == Sound::offNote ? 2.0 : sound == Sound::note || sound == Sound::wideBass ? 1.5 : sound == Sound::hitsInHiss ? 1.0 : 0.045) * rate);

        for (int i = 0; i < buffer.getNumSamples() && position < length; ++i, ++position)
        {
            const double t = position / rate;
            if (sound == Sound::wideBass)
            {
                const float left = getWideBassSample (t, 0);
                const float right = getWideBassSample (t, 1);
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.addSample (ch, i, ch % 2 == 0 ? left : right);
            }
            else if (sound == Sound::offNote)
            {
                const float v = getOffNoteSample (t);
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.addSample (ch, i, v);
            }
            else
            {
                const float v = sound == Sound::note ? getNoteSample (t) : sound == Sound::hitsInHiss ? getHitsInHissSample (t) : getSample (t);
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.addSample (ch, i, v);
            }
        }

        if (position >= length)
            position = -1;
    }

private:
    /** A sung "ah" on A3 that starts 150 cents low, slides up to 30 cents flat and holds there with a gentle vibrato. */
    float getOffNoteSample (double t)
    {
        static const tune::TakeNote note = tune::getOffNote();
        const double attack = juce::jmin (1.0, t / 0.015), release = juce::jmin (1.0, (note.end - t) / 0.03);
        return (float) (voice.next (note.midi + note.cents (t) * 0.01) * attack * release * offNoteLevel);
    }

    float getWideBassSample (double t, int channel)
    {
        const double envelope = juce::jmin (1.0, t / 0.005) * std::exp (-t / 0.6);
        const double phase = channel == 0 ? 0.0 : juce::MathConstants<double>::halfPi;
        const float bass = 0.5f * (float) (envelope * std::sin (juce::MathConstants<double>::twoPi * 55.0 * t + phase));
        const float noise = random.nextFloat() * 2.0f - 1.0f;
        const float coefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 2000.0f / (float) rate);
        wideLowState[channel] += coefficient * (noise - wideLowState[channel]);
        const double sinceHit = t - 0.25 * std::floor (t / 0.25);
        return bass + 0.05f * (float) std::exp (-sinceHit / 0.03) * (noise - wideLowState[channel]);
    }

    static float getNoteSample (double t)
    {
        const double envelope = juce::jmin (1.0, t / 0.005) * std::exp (-t / 0.45);
        return 0.5f * (float) (envelope * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t));
    }

    /** Four claps a quarter of a second apart, over steady white noise (peak 0.003). */
    float getHitsInHissSample (double t)
    {
        float v = (random.nextFloat() * 2.0f - 1.0f) * 0.003f;

        for (int hit = 0; hit < 4; ++hit)
        {
            const double since = t - 0.25 * hit;

            if (since >= 0.0 && since < 0.045)
                v += getSample (since);
        }

        return v;
    }

    float getSample (double t)
    {
        // The hands meet a few times within 15 ms, then the cupped palms ring briefly.
        double envelope = 0.0;

        for (const double start : { 0.0, 0.006, 0.012 })
            if (t >= start)
                envelope = juce::jmax (envelope, std::exp (-(t - start) / 0.0018));

        envelope = juce::jmax (envelope, t >= 0.012 ? 0.6 * std::exp (-(t - 0.012) / 0.008) : 0.0);

        // Noise without its lows: a clap is mostly 1-3 kHz.
        const float noise = random.nextFloat() * 2.0f - 1.0f;
        lowState += 0.15f * (noise - lowState);
        return 0.3f * (float) envelope * (noise - lowState);
    }

    std::atomic<bool> requested { false };
    std::atomic<Sound> requestedSound { Sound::clap };
    Sound sound = Sound::clap;
    double rate = 44100.0;
    int position = -1;
    float lowState = 0.0f;
    float wideLowState[2] {};
    juce::Random random { 7 };
    tune::SungVoice voice;
    static constexpr double offNoteLevel = 1.78;   // the voice peaks near 0.28, so the sound peaks near 0.5 (-6 dB) like the others
};
} // namespace daw::fx
