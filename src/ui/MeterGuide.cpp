#include "MeterGuide.h"
#include "MeterEditor.h"
#include "Theme.h"
#include <cmath>

namespace daw::metertext
{
namespace
{
    SongAccess songAccess;

    struct PageText { const char* name; const char* summary; const char* text; };

    // The facts and wording come from the plan's Theory section (ITU-R BS.1770-5, EBU Tech 3341 and 3342, R 128, and the services' own pages).
    const PageText pageTexts[] = {
    { "Peaks aren't loudness", "Same peak, 11 dB apart", R"loud(A peak meter shows the biggest swing of the wave. Your ears don't follow that. They follow how much sound arrives over a moment, not the single highest point.

The picture shows two sounds with the same peak, -6 dBFS. The first is a steady tone. The second is the same tone in short bursts, 40 ms on and then silence until the next one half a second later, like a hit on every beat at 120 BPM. A peak meter shows them as equal. DawIt Meter measures the steady tone at -6.0 LUFS and the bursts at -17.0 LUFS. The bursts sound for only 8% of the time, and 10 × log10 (0.08) is -11 dB.

The same happens in a mix. A drum loop and a pad can peak at the same level while the pad sounds much louder. A master squashed by a limiter peaks at -1 dB, just like a dynamic one, and sounds far louder.

That's why streaming services, broadcasters and mastering engineers measure loudness in LUFS, not in peaks. You still need the peaks, because they tell you whether something clips. DawIt Meter shows both.)loud" },
    { "What LUFS means", "Loudness units, relative to full scale", R"loud(LUFS stands for Loudness Units relative to Full Scale. It comes from the international standard ITU-R BS.1770, which broadcasters and streaming services use. You'll also see LKFS (Loudness, K-weighted, relative to Full Scale). It's the same unit under another name.

The reference is a full-scale sine wave. A 1 kHz sine peaking at 0 dBFS in both channels measures about 0 LUFS, and in one channel only, -3.0 LUFS. Music is far quieter than that steady, full-scale tone, so its loudness is almost always a negative number. Closer to 0 is louder: -8 LUFS is louder than -14 LUFS.

One LU (loudness unit) is one decibel. Turn a song up by 3 dB and it measures 3 LU higher, from -17 to -14 LUFS. A difference is written in LU ("the chorus is 4 LU louder than the verse"), and a level in LUFS.

Some numbers to place it. European television aims at -23 LUFS. Spotify plays songs at about -14 LUFS. A loud club master is often between -9 and -6 LUFS.

What LUFS doesn't tell you: whether the song is good, or how loud it is in your room (that depends on your volume knob and speakers). It measures the level in the file, weighted the way the ear hears, so that two songs at the same LUFS sound about equally loud. It works well for music and speech. The standard itself says it isn't meant for judging pure tones.)loud" },
    { "K-weighting", "The filter that makes the meter hear like an ear", R"loud(Our ears don't hear all frequencies equally. Deep bass needs much more energy to sound as loud as the middle, and the range from about 2 to 5 kHz stands out. So before measuring, the meter passes the sound through a filter called K-weighting.

It has two parts. The first raises everything above about 2 kHz, by up to 4 dB. It models how the head itself boosts those frequencies on their way into the ear. The second turns the low end down: about 1 dB at 100 Hz, 4 dB at 50 Hz, 8 dB at 30 Hz and 13 dB at 20 Hz. Around 1 kHz it changes almost nothing (+0.7 dB, which the formula takes off again).

What this means in a mix:
- A bass-heavy mix measures quieter than a peak or RMS meter suggests, because its energy sits where K-weighting counts less.
- A bright mix measures louder. A boost at 3 kHz raises the LUFS more than the same boost at 60 Hz.
- Sub-bass below 40 Hz counts much less (a quarter of its energy or less), yet it still takes up headroom. It can push the peaks up without making the song measure louder.

K-weighting is a simple model. It doesn't change with the playback volume the way hearing does (we hear more bass when it's loud). It was chosen because it matched listening tests on real music and speech well, not because it copies the ear exactly.)loud" },
    { "Momentary, short-term, integrated", "0.4 s, 3 s and the whole song", R"loud(A loudness meter gives three readings over three lengths of time.

Momentary: the last 0.4 seconds. It jumps with every beat and word. Use it to see single hits and words.

Short-term: the last 3 seconds. It follows a phrase, or a bar or two, and moves smoothly. This is the one to watch while you mix, to compare a verse with a chorus or an effect on and off.

Integrated: the whole song, or everything since you pressed Reset. It's the one number streaming services and broadcasters use, so it's the one a target means.

Integrated leaves out the quiet parts. The song is cut into 400 ms blocks, a new one every 100 ms. Blocks below -70 LUFS (near silence) are dropped. The rest are averaged, then blocks more than 10 LU below that average are dropped too, and the average is taken again. This is called gating. It means a quiet intro or a pause doesn't pull the number down.

An example: a song with a 10-second intro at -40 LUFS, a 40-second verse at -16 and a 40-second chorus at -11. Its integrated loudness is -12.8 LUFS. The intro is more than 10 LU under the average, so it's left out. Counted in, the result would be -13.3. Notice that -12.8 is closer to the chorus than to the verse. The average is taken over the sound's energy, not over the numbers, so loud parts count for more.

In DawIt Meter, integrated starts again when you press Reset, or when playback starts if Reset on play is on. For the real number, play the whole song from start to end. A loop of the chorus measures the chorus.)loud" },
    { "Loudness range", "How far the loudness moves, in LU", R"loud(Loudness range (LRA) says how much the loudness moves between the quieter and louder parts of a song, in LU. The EBU defines it in Tech 3342.

It's worked out from the short-term readings (3-second windows, ten a second). The quietest 10% and the loudest 5% are ignored, so a single bang or a fade-out doesn't count. LRA is the distance between what's left at the bottom and at the top. Like integrated loudness, it leaves out near-silence and very quiet parts (here, more than 20 LU below the average).

Typical values vary a lot by style:
- about 3 to 6 LU: a dense pop, EDM or rock master, where verse and chorus are close;
- more: songs that move between quiet and loud sections;
- 15 LU or more: film soundtracks and much classical music.

The example song (verse at -16, chorus at -11) has an LRA of 5 LU.

LRA isn't a score: a ballad and a club track should differ. It isn't the crest factor either. The crest factor is about peaks against the average from moment to moment; LRA is about the song's sections. Measure at least a minute of the song: the EBU says the reading hasn't settled during the first 60 seconds.)loud" },
    { "True peak", "The peaks between the samples", R"loud(A digital recording is a row of samples. The real sound wave, the one your converter or headphones make from them, is a smooth curve through those samples. Between two samples it can swing higher than either of them. These are inter-sample peaks.

A sample-peak meter looks only at the samples, so it can miss them. The picture shows a high tone, a quarter of the sample rate (12 kHz at 48 kHz), whose samples all sit at exactly 0 dBFS. The sample-peak meter says 0.0 dB. The wave between them reaches +3.0 dBTP, 3 dB over full scale. That's an extreme case, but on short, bright hits a sample meter often reads several dB low.

True peak, in dBTP (decibels true peak), is measured by working out the curve between the samples. DawIt Meter computes four points for every sample (4× oversampling), as ITU-R BS.1770 describes. That finds the peak to within about half a decibel.

Why it matters:
- A converter, or a phone playing the file, can clip on a peak that the sample meter never showed.
- Encoding to MP3, AAC or Ogg changes the wave slightly, and its peaks can come out higher than the original's.

That's why masters for streaming usually stop at -1 dBTP rather than 0 dBFS. Spotify asks for -1 dBTP or lower, and -2 dBTP for masters louder than -14 LUFS. European broadcast (EBU R 128) allows at most -1 dBTP, and US television (ATSC A/85) -2 dBTP. A limiter with true-peak detection, set to -1 dB, does the job.

DawIt Meter holds the highest true peak since Reset, and turns it red above the ceiling of your target.)loud" },
    { "Targets and normalisation", "What each service turns up or down", R"loud(Streaming services don't play every song as loud as it was mastered. They measure each song's integrated loudness and turn it up or down to one level, so you don't have to reach for the volume between songs. This is loudness normalisation.

Typical playback levels (each service sets its own, and they change now and then):
- Spotify, YouTube, Tidal, Amazon Music: about -14 LUFS.
- Apple Music, with Sound Check (on by default on new devices): about -16 LUFS.
- Podcasts: often -16 LUFS.
- European television and radio (EBU R 128): -23 LUFS, true peak at most -1 dBTP.
- US television (ATSC A/85): -24 LUFS, true peak at most -2 dBTP.

Turning down is simple: a master at -8 LUFS plays 6 dB quieter on Spotify. Turning up has a limit. Spotify turns a quiet song up only as far as its peaks allow, keeping the true peak at -1 dBTP or lower. Its own example: a song at -20 LUFS with a true peak of -5 dBTP is turned up 4 dB, to -16 LUFS, not to -14. Not every service turns quiet songs up at all.

Normalisation isn't everywhere. Listeners can switch it off, Spotify's web player and some TVs and speakers don't use it, Spotify Premium also has a Loud (-11 LUFS) and a Quiet (-19 LUFS) setting, and a downloaded file, a CD or a DJ set plays the song as it is.

So a streaming target isn't a rule you must hit. It tells you how loud your song will be played. Master near it and what you hear is what listeners get. Broadcast is different: there, the number is a delivery requirement.

The picture shows the gain that would bring your song to each target: from the meter's integrated loudness when it has one, otherwise from a master at -9 LUFS. A streaming service applies it itself, turning up only as far as the peaks allow. For broadcast, podcasts and clubs it's the change you would make yourself.)loud" },
    { "Loud masters and the loudness war", "Squashed masters lose, once normalised", R"loud(From the 1990s on, masters got louder and louder, as each record tried to jump out on the radio. A limiter makes a song louder at the same peak by flattening the peaks, so the average rises. This race is called the loudness war.

What you lose: the punch of the drums (their attacks are cut off), the difference between a quiet verse and a big chorus, and in the end clean sound. What you gain, where nothing is normalised, is a louder song, and louder sounds better at first.

Normalisation changes the sum. Take two masters:
- a dynamic one at -14 LUFS, peaking at -1.0 dBTP: 13 dB between its peaks and its loudness;
- a squashed one at -7 LUFS, peaking at -0.3 dBTP: only 6.7 dB between them.

On Spotify the squashed one is turned down 7 dB, so both are equally loud. Now the dynamic one's drums still rise 13 dB above the loudness, while the squashed one's peaks reach only -7.3 dBTP. Next to the other it sounds flatter and smaller, not louder.

To judge a master, look at the gap between the peaks and the loudness. The meter shows the crest factor (the peak minus the RMS over the last 3 seconds; a sine wave's is 3 dB), and Your track compares the peak with the integrated loudness. In one well-known comparison, a heavily limited remaster had a 7 dB gap and sounded feeble once normalised, a modern pop song 9 dB, and a dynamic jazz recording 20 dB.

Loud still has its place. Club and EDM masters are often -9 to -6 LUFS, because clubs and DJ software play files as they are, and the density is part of the sound. It's a choice, not a mistake. Make it on purpose, and compare at the same loudness.)loud" },
    { "Loudness while mixing", "No target, but fair comparisons", R"loud(While you mix, you don't need a loudness target. A mix keeps room for mastering, with peaks around -6 dBFS on the master as in the Gain staging guide, whatever its LUFS. The final loudness is set at the end, by the limiter.

Where a loudness meter helps while mixing:
- Fair comparisons. Louder sounds better, even by 1 dB. Put DawIt Meter after an effect and note the short-term loudness with the effect off. Switch it on, and turn the effect's output until the short-term reads the same. In the picture the effect adds 1.8 dB (-17.0 against -15.2 LUFS), so turn its output down 1.8 dB. Now you hear only what the effect does.
- A reference song. Import a finished song you like onto a track, put DawIt Meter on it, and turn the track down until its short-term matches your mix's. At the same loudness you hear the real differences in tone and punch.
- Sections. Watch the short-term in the verse and the chorus. If they read the same, the lift has to come from the arrangement.
- Tracks and buses. A meter on the drum bus or the vocals shows how loud they sit. Peaks can't tell you that, because a snare's peak is far above its loudness.

DawIt Meter passes the sound through untouched and adds no delay, so it can sit anywhere: after the last effect of a track, on a bus, or on a bus that holds the whole mix. File > Measure Loudness... measures the whole song at once.)loud" },
    { "How DawIt Meter works", "From samples to numbers", R"loud(1. K-weighting. Each channel goes through the two filters of ITU-R BS.1770, the head shelf and the low cut, set for the sample rate.
2. Mean square. The filtered samples are squared and averaged over 0.4 seconds (momentary) and 3 seconds (short-term), and left and right are added with the same weight. -0.691 + 10 × log10 of that gives LUFS.
3. Every 100 ms a new 400 ms block is counted for integrated loudness, and a new short-term value for the loudness range. They're counted in steps of 0.1 LU, so the meter can run for hours without needing more memory. Then the gates are applied: -70 LUFS and 10 LU below for integrated, and -70 LUFS and 20 LU below for the range, taken between the 10th and 95th percentiles.
4. True peak. Four points are worked out for every sample (a 48-tap filter like the one in BS.1770), and the highest is kept.
5. Levels. Sample peak, RMS and crest factor are measured on the sound as it is (not K-weighted) over the last 3 seconds. Correlation is measured as in DawIt Stereo.

The sound itself passes through unchanged, bit for bit, with no delay. The readings follow the EBU's Tech 3341 ("EBU Mode") and Tech 3342. DawIt's tests check them with the EBU test signals: a 1 kHz tone at -23 dBFS in both channels reads -23.0 LUFS, a tone at -20 dBFS followed by one at -30 dBFS gives a range of 10 LU, and a tone whose samples sit at 0 dBFS reads +3.0 dBTP.

A mono channel is measured as one channel, as BS.1770 says. So a mono track in another DAW reads 3 LU lower than the same sound in both channels of a stereo track.)loud" },
    };

    constexpr int numTextPages = (int) (sizeof (pageTexts) / sizeof (pageTexts[0]));
    constexpr int yourTrackPage = numTextPages;   // the last page
    const float notANumber = std::numeric_limits<float>::quiet_NaN();

    //==============================================================================
    // Text helpers

    double roundTo (double v, int decimals)
    {
        const double scale = std::pow (10.0, decimals);
        const double rounded = std::round (v * scale) / scale;
        return juce::exactlyEqual (rounded, 0.0) ? 0.0 : rounded;   // never "-0.0"
    }

    juce::String num (double v, int decimals = 1) { return juce::String (roundTo (v, decimals), decimals); }
    juce::String signedNum (double v, int decimals = 1) { const auto r = roundTo (v, decimals); return (r > 0.0 ? "+" : "") + juce::String (r, decimals); }

    //==============================================================================
    // Made-up signals, measured with the meter itself, so the pictures' numbers are computed.

    loudness::Readings measure (double rate, double seconds, const std::function<void (double, float&, float&)>& source)
    {
        loudness::Meter meter;
        meter.prepare (rate);
        const auto count = (long) (rate * seconds);

        for (long i = 0; i < count; ++i)
        {
            float left = 0.0f, right = 0.0f;
            source ((double) i / rate, left, right);
            meter.push (left, right, 2);
        }

        return meter.getReadings();
    }

    float sine (double t, double db, double hz = 997.0, double phase = 0.0)
    {
        return (float) (std::pow (10.0, db / 20.0) * std::sin (juce::MathConstants<double>::twoPi * hz * t + phase));
    }

    constexpr double burstOn = 0.04, burstEvery = 0.5;

    /** 40 ms on, then silent until the next one half a second later: on for 8% of the time, so 11 dB quieter in loudness. */
    double burstEnvelope (double t) { return std::fmod (t, burstEvery) < burstOn ? 1.0 : 0.0; }

    /** A steady tone and the same tone in bursts, both peaking at -6 dBFS, and what the meter says about each. */
    struct PeakExample { double steady = 0.0, bursts = 0.0; };

    const PeakExample& getPeakExample()
    {
        static const PeakExample example = []
        {
            PeakExample e;
            const auto steady = measure (48000.0, 6.0, [] (double t, float& l, float& r) { l = r = sine (t, -6.0); });
            const auto bursts = measure (48000.0, 6.0, [] (double t, float& l, float& r) { l = r = (float) burstEnvelope (t) * sine (t, -6.0); });
            e.steady = steady.shortTerm.value_or (-70.0);
            e.bursts = bursts.shortTerm.value_or (-70.0);
            return e;
        }();
        return example;
    }

    /** The example song: 10 s at -40, 40 s at -16 and 40 s at -11 dBFS, with momentary and short-term every 0.1 s. */
    struct SongExample
    {
        std::vector<float> momentary, shortTerm;
        loudness::Readings readings;
    };

    const SongExample& getSongExample()
    {
        static const SongExample example = []
        {
            // 16 kHz keeps the one-off calculation quick; the K-weighting is designed for the rate, so the numbers match 48 kHz.
            constexpr double rate = 16000.0;
            constexpr int block = 1600;
            SongExample e;
            loudness::Meter meter;
            meter.prepare (rate);
            double phase = 0.0;

            for (int b = 0; b < 900; ++b)
            {
                const double t = b * 0.1;
                const double db = t < 10.0 ? -40.0 : (t < 50.0 ? -16.0 : -11.0), gain = std::pow (10.0, db / 20.0);

                for (int i = 0; i < block; ++i)
                {
                    const auto v = (float) (gain * std::sin (phase));
                    phase += juce::MathConstants<double>::twoPi * 997.0 / rate;
                    meter.push (v, v, 2);
                }

                const auto m = meter.momentary(), s = meter.shortTerm();
                e.momentary.push_back (m ? (float) *m : notANumber);
                e.shortTerm.push_back (s ? (float) *s : notANumber);
            }

            e.readings = meter.getReadings();
            return e;
        }();
        return example;
    }

    /** The picture of a wave between its samples: fs/4 sine at amplitude 1.41, phase 45 degrees (EBU Tech 3341 test 19). */
    const loudness::Readings& getTruePeakExample()
    {
        static const loudness::Readings readings = measure (48000.0, 1.0, [] (double t, float& l, float& r)
        {
            // The samples sit at +-1.0 (0 dBFS); the wave through them reaches 1.414.
            const double n = std::round (t * 48000.0);
            l = r = (float) (std::sqrt (2.0) * std::sin (juce::MathConstants<double>::halfPi * n + juce::MathConstants<double>::pi * 0.25));
        });
        return readings;
    }

    /** An effect off and on (the two tones peak at different levels, -17.0 and -15.2 dBFS): the loudness it adds. */
    struct MixExample { double off = 0.0, on = 0.0; };

    const MixExample& getMixExample()
    {
        static const MixExample example = []
        {
            MixExample e;
            // The effect's own gain: 1.8 dB, the kind of lift a small EQ boost makes on a mix.
            const auto off = measure (16000.0, 6.0, [] (double t, float& l, float& r) { l = r = sine (t, -17.0); });
            const auto on = measure (16000.0, 6.0, [] (double t, float& l, float& r) { l = r = sine (t, -15.2); });
            e.off = off.shortTerm.value_or (-70.0);
            e.on = on.shortTerm.value_or (-70.0);
            return e;
        }();
        return example;
    }

    //==============================================================================
    // Drawing helpers

    const auto teal = meterui::colour;

    void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Justification j, juce::Colour c = theme::text, bool bold = false, int lines = 1)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (s, r.toNearestInt(), j, lines, 0.7f);
    }

    float mapTo (double v, double lo, double hi, float from, float to)
    {
        return from + (float) juce::jlimit (0.0, 1.0, (v - lo) / (hi - lo)) * (to - from);
    }

    void dashed (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour c, float thickness = 1.2f)
    {
        const float dashes[] { 5.0f, 4.0f };
        g.setColour (c);
        g.drawDashedLine (juce::Line<float> (a, b), dashes, 2, thickness);
    }

    /** A line through values with gaps where there is no reading (NaN). */
    void drawSeries (juce::Graphics& g, const std::vector<float>& values, juce::Rectangle<float> plot, double lo, double hi, double spanPoints, juce::Colour c, float width)
    {
        juce::Path path;
        bool open = false;

        for (size_t i = 0; i < values.size(); ++i)
        {
            if (std::isnan (values[i])) { open = false; continue; }

            const float x = plot.getX() + (float) ((double) i / spanPoints) * plot.getWidth();
            const float y = mapTo (values[i], lo, hi, plot.getBottom(), plot.getY());

            if (open) path.lineTo (x, y); else path.startNewSubPath (x, y);
            open = true;
        }

        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    /** The grid of a LUFS graph: a line and label every 12 LU. */
    void drawLufsGrid (juce::Graphics& g, juce::Rectangle<float> plot, double lo, double hi, double step)
    {
        for (double v = std::ceil (lo / step) * step; v <= hi + 0.01; v += step)
        {
            const float y = mapTo (v, lo, hi, plot.getBottom(), plot.getY());
            g.setColour (theme::grid);
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            text (g, num (v, 0), { plot.getX() - 34.0f, y - 7.0f, 30.0f, 14.0f }, 10.5f, juce::Justification::centredRight, theme::textDim);
        }
    }

    //==============================================================================
    // 1. Peaks aren't loudness

    void paintPeaks (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto& example = getPeakExample();
        const struct { const char* name; bool bursts; double lufs; } rows[] { { "A steady tone", false, example.steady }, { "The same tone in short bursts", true, example.bursts } };
        auto footer = r.removeFromBottom (18.0f);
        const float rowHeight = r.getHeight() / 2.0f;

        for (int i = 0; i < 2; ++i)
        {
            auto row = r.withTop (r.getY() + rowHeight * (float) i).withHeight (rowHeight).reduced (0.0f, 6.0f);
            auto side = row.removeFromRight (row.getWidth() * 0.3f);
            auto wave = row.reduced (0.0f, 14.0f);
            g.setColour (juce::Colours::black.withAlpha (0.28f));
            g.fillRoundedRectangle (wave, 4.0f);
            text (g, rows[i].name, row.withHeight (14.0f), 11.5f, juce::Justification::topLeft, theme::textDim);
            const float centre = wave.getCentreY(), half = wave.getHeight() * 0.5f - 2.0f, peak = half * 0.5f;   // -6 dBFS is half the full-scale height
            g.setColour (teal.withAlpha (0.75f));

            if (! rows[i].bursts)
                g.fillRect (wave.getX() + 2.0f, centre - peak, wave.getWidth() - 4.0f, peak * 2.0f);
            else
                for (int b = 0; b < 3; ++b)
                    g.fillRect (wave.getX() + 2.0f + (float) (b * burstEvery / 1.25) * (wave.getWidth() - 4.0f), centre - peak, juce::jmax (2.0f, (float) (burstOn / 1.25) * (wave.getWidth() - 4.0f)), peak * 2.0f);

            dashed (g, { wave.getX(), centre - peak }, { wave.getRight(), centre - peak }, theme::textDim);
            dashed (g, { wave.getX(), centre + peak }, { wave.getRight(), centre + peak }, theme::textDim);
            text (g, "peak -6.0 dBFS", { wave.getRight() - 110.0f, centre - peak - 15.0f, 108.0f, 13.0f }, 10.5f, juce::Justification::centredRight, theme::textDim);
            side.reduce (14.0f, 0.0f);
            text (g, num (rows[i].lufs) + " LUFS", side.withTrimmedTop (side.getHeight() * 0.25f).removeFromTop (30.0f), 22.0f, juce::Justification::centredLeft, teal, true);
            text (g, "short-term loudness", side.withTrimmedTop (side.getHeight() * 0.25f + 32.0f).removeFromTop (16.0f), 11.5f, juce::Justification::centredLeft, theme::textDim);
        }

        text (g, num (example.steady - example.bursts) + " dB apart, at the same peak.", footer, 12.0f, juce::Justification::centred, theme::text);
    }

    //==============================================================================
    // 2. What LUFS means

    void paintScale (juce::Graphics& g, juce::Rectangle<float> r, BuiltInMeter& processor)
    {
        constexpr double lo = -60.0, hi = 0.0;
        auto bar = r.reduced (24.0f, 0.0f);
        const float barY = bar.getCentreY() + 8.0f, barH = 18.0f;
        const auto track = juce::Rectangle<float> (bar.getX(), barY - barH * 0.5f, bar.getWidth(), barH);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (track, 4.0f);
        auto xFor = [&] (double v) { return mapTo (v, lo, hi, track.getX(), track.getRight()); };

        for (double v = lo; v <= hi + 0.1; v += 10.0)
        {
            g.setColour (theme::gridBar);
            g.drawVerticalLine ((int) xFor (v), track.getY(), track.getBottom());
            text (g, num (v, 0), { xFor (v) - 20.0f, track.getBottom() + 4.0f, 40.0f, 14.0f }, 11.0f, juce::Justification::centred, theme::textDim);
        }

        text (g, "LUFS", { track.getRight() - 40.0f, track.getBottom() + 18.0f, 40.0f, 14.0f }, 11.0f, juce::Justification::centredRight, theme::textDim);

        // Club masters, then the markers; labels sit on three levels so -16 and -14 don't collide.
        g.setColour (theme::meterYellow.withAlpha (0.45f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (-9.0), track.getY(), xFor (-6.0), track.getBottom()));

        const struct { double v; const char* label; int level; juce::Colour colour; } marks[] {
            { -23.0, "-23  European TV", 0, theme::accent }, { -16.0, "-16  Apple Music", 1, theme::meterGreen },
            { -14.0, "-14  Streaming", 2, theme::meterGreen }, { -7.5, "-9 to -6  Club", 0, theme::meterYellow } };

        for (const auto& m : marks)
        {
            const float labelY = track.getY() - 18.0f - 20.0f * (float) (2 - m.level) - 6.0f;
            g.setColour (m.colour);
            g.drawVerticalLine ((int) xFor (m.v), labelY + 14.0f, track.getY());
            text (g, m.label, { xFor (m.v) - 4.0f, labelY, 140.0f, 14.0f }, 11.5f, juce::Justification::centredLeft, m.colour);
        }

        const auto shortTerm = processor.getReadings().shortTerm;

        if (shortTerm)
        {
            g.setColour (teal);
            g.fillEllipse (xFor (*shortTerm) - 7.0f, barY - 7.0f, 14.0f, 14.0f);
            text (g, "now " + num (*shortTerm) + " LUFS", { xFor (*shortTerm) - 60.0f, track.getBottom() + 34.0f, 120.0f, 14.0f }, 11.5f, juce::Justification::centred, teal);
        }
        else
        {
            text (g, "Play the song through DawIt Meter and its short-term loudness appears here as a dot.", r.removeFromBottom (18.0f), 11.5f, juce::Justification::centred, theme::textDim);
        }
    }

    //==============================================================================
    // 3. K-weighting

    void paintK (juce::Graphics& g, juce::Rectangle<float> r)
    {
        constexpr double lo = -16.0, hi = 6.0, fLo = 20.0, fHi = 20000.0;
        auto plot = r.reduced (0.0f, 4.0f).withTrimmedLeft (40.0f).withTrimmedRight (10.0f).withTrimmedBottom (22.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot, 4.0f);
        auto xFor = [&] (double hz) { return mapTo (std::log (hz / fLo), 0.0, std::log (fHi / fLo), plot.getX(), plot.getRight()); };
        auto yFor = [&] (double db) { return mapTo (db, lo, hi, plot.getBottom(), plot.getY()); };

        for (double db = -15.0; db <= 5.0; db += 5.0)
        {
            g.setColour (juce::exactlyEqual (db, 0.0) ? theme::gridBar : theme::grid);
            g.drawHorizontalLine ((int) yFor (db), plot.getX(), plot.getRight());
            text (g, signedNum (db, 0), { plot.getX() - 40.0f, yFor (db) - 7.0f, 36.0f, 14.0f }, 10.5f, juce::Justification::centredRight, theme::textDim);
        }

        for (double hz : { 20.0, 100.0, 1000.0, 10000.0 })
        {
            g.setColour (theme::grid);
            g.drawVerticalLine ((int) xFor (hz), plot.getY(), plot.getBottom());
            text (g, hz >= 1000.0 ? num (hz / 1000.0, 0) + " kHz" : num (hz, 0) + " Hz", { xFor (hz) - 30.0f, plot.getBottom() + 4.0f, 60.0f, 14.0f }, 10.5f, juce::Justification::centred, theme::textDim);
        }

        juce::Path curve;

        for (int i = 0; i <= 240; ++i)
        {
            const double hz = fLo * std::pow (fHi / fLo, i / 240.0);
            const float x = xFor (hz), y = yFor (loudness::kResponseDb (hz, 48000.0));
            if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        }

        g.setColour (teal);
        g.strokePath (curve, juce::PathStrokeType (2.5f));

        for (double hz : { 20.0, 50.0, 100.0, 1000.0, 4000.0 })
        {
            const double db = loudness::kResponseDb (hz, 48000.0);
            const float x = xFor (hz), y = yFor (db);
            g.setColour (theme::text);
            g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
            const bool below = db < 0.0 && hz < 500.0;
            text (g, num (hz, 0) + " Hz  " + signedNum (db) + " dB", { x + 6.0f, y + (below ? 2.0f : -18.0f), 120.0f, 14.0f }, 11.0f, juce::Justification::centredLeft, theme::text);
        }
    }

    //==============================================================================
    // 4. Momentary, short-term, integrated

    void paintOverTime (juce::Graphics& g, juce::Rectangle<float> r, BuiltInMeter& processor)
    {
        constexpr double lo = -60.0, hi = 0.0;
        const auto readings = processor.getReadings();
        const bool live = readings.seconds >= 30.0;
        std::vector<float> momentary, shortTerm;
        double span = 900.0;
        std::optional<double> integrated;

        if (live)
        {
            for (const auto& p : processor.getHistory()) { momentary.push_back (p.momentary); shortTerm.push_back (p.shortTerm); }
            span = (double) meterfx::historyLength;
            integrated = readings.integrated;
        }
        else
        {
            const auto& example = getSongExample();
            momentary = example.momentary; shortTerm = example.shortTerm;
            integrated = example.readings.integrated;
        }

        text (g, live ? "Your meter's last 2 minutes" : "A made-up song: 10 s at -40, 40 s at -16, 40 s at -11 LUFS", r.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        auto plot = r.withTrimmedLeft (40.0f).withTrimmedRight (10.0f).withTrimmedTop (4.0f).withTrimmedBottom (22.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot, 4.0f);

        if (! live)
        {
            // The intro is more than 10 LU under the average of the rest, so integrated loudness leaves it out.
            const auto gated = juce::Rectangle<float>::leftTopRightBottom (plot.getX(), plot.getY(), plot.getX() + (float) (100.0 / span) * plot.getWidth(), plot.getBottom());
            g.setColour (theme::textDim.withAlpha (0.18f));
            g.fillRect (gated);
            text (g, "gated out", gated.withHeight (16.0f), 11.0f, juce::Justification::centred, theme::textDim);
        }

        drawLufsGrid (g, plot, lo, hi, 12.0);
        drawSeries (g, momentary, plot, lo, hi, span, teal.withAlpha (0.55f), 1.0f);
        drawSeries (g, shortTerm, plot, lo, hi, span, teal, 2.8f);

        if (integrated)
        {
            const float y = mapTo (*integrated, lo, hi, plot.getBottom(), plot.getY());
            const float dots[] { 2.0f, 4.0f };
            g.setColour (theme::text);
            g.drawDashedLine (juce::Line<float> (plot.getX(), y, plot.getRight(), y), dots, 2, 1.4f);
            text (g, "integrated " + num (*integrated) + " LUFS", { plot.getRight() - 190.0f, y + 2.0f, 186.0f, 14.0f }, 11.5f, juce::Justification::centredRight, theme::text);
        }

        text (g, "thin: momentary    thick: short-term    dotted: integrated", { plot.getX(), plot.getBottom() + 4.0f, plot.getWidth(), 14.0f }, 11.0f, juce::Justification::centred, theme::textDim);
    }

    //==============================================================================
    // 5. Loudness range

    void paintRange (juce::Graphics& g, juce::Rectangle<float> r, BuiltInMeter& processor)
    {
        const auto readings = processor.getReadings();
        const bool live = readings.seconds >= 60.0 && readings.range.has_value();
        std::vector<float> values;
        std::optional<double> lra;

        if (live)
        {
            for (const auto& p : processor.getHistory()) values.push_back (p.shortTerm);
            lra = readings.range;
        }
        else
        {
            values = getSongExample().shortTerm;
            lra = getSongExample().readings.range;
        }

        // The EBU's gates: above -70 LUFS, and within 20 LU of the average of what is left.
        std::vector<double> kept;
        double energy = 0.0;
        int absolute = 0;

        for (const auto v : values)
            if (! std::isnan (v) && v > -70.0f) { energy += std::pow (10.0, v / 10.0); ++absolute; }

        const double gate = absolute > 0 ? 10.0 * std::log10 (energy / absolute) - 20.0 : -70.0;

        for (const auto v : values)
            if (! std::isnan (v) && v > -70.0f && v > gate) kept.push_back (v);

        std::sort (kept.begin(), kept.end());
        const double low = kept.empty() ? 0.0 : kept[(size_t) std::floor (0.10 * (double) (kept.size() - 1))],
                     high = kept.empty() ? 0.0 : kept[(size_t) std::floor (0.95 * (double) (kept.size() - 1))];

        text (g, live ? "Your meter's last 2 minutes of short-term loudness" : "The short-term values of a made-up song (verse -16, chorus -11 LUFS)", r.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        constexpr double lo = -45.0, hi = -5.0;
        auto plot = r.withTrimmedLeft (10.0f).withTrimmedRight (10.0f).withTrimmedTop (22.0f).withTrimmedBottom (22.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot, 4.0f);
        std::array<int, 40> bins {};
        int tallest = 1;

        for (const auto v : values)
            if (! std::isnan (v) && v >= lo && v < hi && v > -70.0f)
                tallest = juce::jmax (tallest, ++bins[(size_t) std::floor (v - lo)]);

        const float binWidth = plot.getWidth() / 40.0f;

        for (size_t b = 0; b < bins.size(); ++b)
        {
            if (bins[b] == 0) continue;
            const double centre = lo + (double) b + 0.5;
            const float h = (float) bins[b] / (float) tallest * (plot.getHeight() - 46.0f);
            g.setColour (centre > gate ? teal.withAlpha (0.85f) : theme::textDim.withAlpha (0.5f));
            g.fillRect (plot.getX() + (float) b * binWidth + 1.0f, plot.getBottom() - h, binWidth - 2.0f, h);
        }

        for (double v = -40.0; v <= -10.0; v += 10.0)
            text (g, num (v, 0), { mapTo (v, lo, hi, plot.getX(), plot.getRight()) - 20.0f, plot.getBottom() + 4.0f, 40.0f, 14.0f }, 10.5f, juce::Justification::centred, theme::textDim);

        text (g, "LUFS", { plot.getRight() - 40.0f, plot.getBottom() + 4.0f, 40.0f, 14.0f }, 10.5f, juce::Justification::centredRight, theme::textDim);

        if (! kept.empty())
        {
            const float x0 = mapTo (low, lo, hi, plot.getX(), plot.getRight()), x1 = mapTo (high, lo, hi, plot.getX(), plot.getRight());
            g.setColour (theme::text);
            g.drawVerticalLine ((int) x0, plot.getY() + 20.0f, plot.getBottom());
            g.drawVerticalLine ((int) x1, plot.getY() + 20.0f, plot.getBottom());
            g.drawLine (x0, plot.getY() + 14.0f, x1, plot.getY() + 14.0f, 1.5f);
            text (g, "10th percentile", { x0 - 100.0f, plot.getY() + 22.0f, 96.0f, 14.0f }, 10.5f, juce::Justification::centredRight, theme::text);
            text (g, "95th percentile", { x1 + 4.0f, plot.getY() + 22.0f, 96.0f, 14.0f }, 10.5f, juce::Justification::centredLeft, theme::text);
            text (g, "LRA " + (lra ? num (*lra) : num (high - low)) + " LU", { x0, plot.getY(), x1 - x0, 14.0f }, 12.0f, juce::Justification::centred, theme::text, true);
        }

        if (! live)
            text (g, "grey: below the gate, left out", { plot.getX() + 8.0f, plot.getY() + 4.0f, 200.0f, 14.0f }, 10.5f, juce::Justification::centredLeft, theme::textDim);
    }

    //==============================================================================
    // 6. True peak

    void paintTruePeak (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto& readings = getTruePeakExample();
        auto side = r.removeFromRight (r.getWidth() * 0.27f);
        auto plot = r.reduced (4.0f, 8.0f);
        constexpr double top = 1.7;
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot, 4.0f);
        auto yFor = [&] (double v) { return mapTo (v, -top, top, plot.getBottom(), plot.getY()); };
        const int samples = 12;
        auto xFor = [&] (double n) { return plot.getX() + 70.0f + (float) (n / (samples - 1)) * (plot.getWidth() - 84.0f); };

        g.setColour (theme::gridBar);
        g.drawHorizontalLine ((int) yFor (0.0), plot.getX(), plot.getRight());
        dashed (g, { plot.getX(), yFor (1.0) }, { plot.getRight(), yFor (1.0) }, theme::meterGreen);
        dashed (g, { plot.getX(), yFor (-1.0) }, { plot.getRight(), yFor (-1.0) }, theme::meterGreen);
        dashed (g, { plot.getX(), yFor (std::sqrt (2.0)) }, { plot.getRight(), yFor (std::sqrt (2.0)) }, theme::meterRed);
        dashed (g, { plot.getX(), yFor (-std::sqrt (2.0)) }, { plot.getRight(), yFor (-std::sqrt (2.0)) }, theme::meterRed);
        text (g, "0 dBFS", { plot.getX() + 6.0f, yFor (1.0) - 15.0f, 80.0f, 13.0f }, 10.5f, juce::Justification::centredLeft, theme::meterGreen);
        text (g, "+3 dBTP", { plot.getX() + 6.0f, yFor (std::sqrt (2.0)) - 15.0f, 80.0f, 13.0f }, 10.5f, juce::Justification::centredLeft, theme::meterRed);

        // The wave through the samples: a sine at a quarter of the sample rate (one cycle every four samples).
        juce::Path wave;
        const int steps = 240;

        for (int i = 0; i <= steps; ++i)
        {
            const double n = (double) i / steps * (samples - 1);
            const float x = xFor (n), y = yFor (std::sqrt (2.0) * std::sin (juce::MathConstants<double>::halfPi * n + juce::MathConstants<double>::pi * 0.25));
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }

        g.setColour (teal);
        g.strokePath (wave, juce::PathStrokeType (2.2f));
        g.setColour (theme::text);

        for (int n = 0; n < samples; ++n)
        {
            const float x = xFor (n), y = yFor (std::sqrt (2.0) * std::sin (juce::MathConstants<double>::halfPi * n + juce::MathConstants<double>::pi * 0.25));
            g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        }

        side.reduce (14.0f, 20.0f);
        text (g, "Sample peak", side.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        text (g, num (readings.samplePeak.value_or (0.0)) + " dBFS", side.removeFromTop (28.0f), 20.0f, juce::Justification::centredLeft, theme::meterGreen, true);
        side.removeFromTop (14.0f);
        text (g, "True peak", side.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        text (g, signedNum (readings.truePeak.value_or (0.0)) + " dBTP", side.removeFromTop (28.0f), 20.0f, juce::Justification::centredLeft, theme::meterRed, true);
        side.removeFromTop (10.0f);
        text (g, "EBU Tech 3341 test 19. The wave peaks at +3.0 dBTP; a meter passes the test reading +2.6 to +3.2.", side.removeFromTop (64.0f), 11.0f, juce::Justification::topLeft, theme::textDim, false, 4);
    }

    //==============================================================================
    // 7. Targets and normalisation

    void paintTargets (juce::Graphics& g, juce::Rectangle<float> r, BuiltInMeter& processor)
    {
        const auto integrated = processor.getReadings().integrated;
        const double loudnessNow = integrated.value_or (-9.0);
        text (g, integrated ? "The gain from your integrated " + num (loudnessNow) + " LUFS to each target" : "The gain from a master at -9 LUFS to each target", r.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        const char* names[] { "Streaming", "Apple Music", "Podcast", "European TV", "US TV", "Club and DJ" };
        const auto& presets = meterfx::getPresets();
        const int rows = 6;
        auto area = r.withTrimmedTop (8.0f).withTrimmedBottom (22.0f);
        const float rowHeight = area.getHeight() / (float) rows;
        auto labelArea = area.removeFromLeft (150.0f);
        constexpr double range = 18.0;
        auto xFor = [&] (double db) { return mapTo (db, -range, range, area.getX() + 8.0f, area.getRight() - 8.0f); };

        g.setColour (theme::gridBar);
        g.drawVerticalLine ((int) xFor (0.0), area.getY(), area.getBottom());

        for (int i = 0; i < rows; ++i)
        {
            const double target = (double) presets[(size_t) i].settings.targetLufs, gain = loudness::gainToTargetDb (loudnessNow, target);
            const float y = area.getY() + rowHeight * (float) i;
            text (g, juce::String (names[i]) + " (" + num (target, 0) + ")", labelArea.withY (y).withHeight (rowHeight), 12.0f, juce::Justification::centredRight, theme::text);
            const float x0 = xFor (0.0), x1 = xFor (gain);
            g.setColour (gain < -0.05 ? teal : (gain > 0.05 ? theme::meterYellow : theme::textDim));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (x0, x1), y + rowHeight * 0.22f, juce::jmax (x0, x1) + (std::abs (gain) < 0.05 ? 2.0f : 0.0f), y + rowHeight * 0.78f));
            text (g, signedNum (gain) + " dB", { juce::jmax (x0, x1) + 6.0f, y, 70.0f, rowHeight }, 12.0f, juce::Justification::centredLeft, theme::text, true);
        }

        text (g, "Left of the line: down to the target. Right of it: up to it.", r.removeFromBottom (16.0f), 11.0f, juce::Justification::centred, theme::textDim);
    }

    //==============================================================================
    // 8. Loud masters

    void paintMasters (juce::Graphics& g, juce::Rectangle<float> r)
    {
        struct Master { const char* name; double lufs, peak; };
        const Master masters[] { { "Dynamic", -14.0, -1.0 }, { "Squashed", -7.0, -0.3 } };
        constexpr double lo = -24.0, hi = 0.0, target = -14.0;
        text (g, "Loudness and peak of two masters, before and after a service turns them to -14 LUFS", r.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        auto plot = r.withTrimmedLeft (40.0f).withTrimmedTop (22.0f).withTrimmedBottom (42.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot, 4.0f);

        for (double v = lo; v <= hi + 0.1; v += 6.0)
        {
            const float y = mapTo (v, lo, hi, plot.getBottom(), plot.getY());
            g.setColour (theme::grid);
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            text (g, num (v, 0), { plot.getX() - 38.0f, y - 7.0f, 34.0f, 14.0f }, 10.5f, juce::Justification::centredRight, theme::textDim);
        }

        const float column = plot.getWidth() / 4.0f;

        for (int c = 0; c < 4; ++c)
        {
            const auto& m = masters[c % 2];
            const bool after = c >= 2;
            const double gain = after ? loudness::gainToTargetDb (m.lufs, target) : 0.0, level = m.lufs + gain, peak = m.peak + gain;
            const float x = plot.getX() + column * (float) c + column * 0.3f, w = column * 0.4f;
            const float yLevel = mapTo (level, lo, hi, plot.getBottom(), plot.getY()), yPeak = mapTo (peak, lo, hi, plot.getBottom(), plot.getY());
            g.setColour (c % 2 == 0 ? teal.withAlpha (0.85f) : theme::meterYellow.withAlpha (0.85f));
            g.fillRect (x, yPeak, w, yLevel - yPeak);
            g.setColour (theme::text);
            g.fillRect (x, yPeak - 1.0f, w, 2.0f);
            g.fillRect (x, yLevel - 1.0f, w, 2.0f);
            text (g, signedNum (peak) + " dBTP", { x - 24.0f, yPeak - 16.0f, w + 48.0f, 13.0f }, 10.5f, juce::Justification::centred, theme::text);
            text (g, num (level) + " LUFS", { x - 24.0f, yLevel + 2.0f, w + 48.0f, 13.0f }, 10.5f, juce::Justification::centred, theme::text);

            if (yLevel - yPeak > 20.0f)
                text (g, num (peak - level) + " dB", { x, yPeak + (yLevel - yPeak) * 0.5f - 7.0f, w, 14.0f }, 11.5f, juce::Justification::centred, juce::Colours::black, true);

            text (g, m.name, { x - 20.0f, plot.getBottom() + 3.0f, w + 40.0f, 14.0f }, 11.5f, juce::Justification::centred, theme::text);
        }

        text (g, "Before", { plot.getX(), plot.getBottom() + 22.0f, column * 2.0f, 14.0f }, 12.0f, juce::Justification::centred, theme::textDim, true);
        text (g, "After normalising to -14 LUFS", { plot.getX() + column * 2.0f, plot.getBottom() + 22.0f, column * 2.0f, 14.0f }, 12.0f, juce::Justification::centred, theme::textDim, true);
        g.setColour (theme::gridBar);
        g.drawVerticalLine ((int) (plot.getX() + column * 2.0f), plot.getY(), plot.getBottom() + 36.0f);
    }

    //==============================================================================
    // 9. Loudness while mixing

    void paintMixing (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const auto& example = getMixExample();
        constexpr double lo = -30.0, hi = -6.0;
        text (g, "Short-term loudness with an effect off and on", r.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        auto area = r.reduced (16.0f, 14.0f).withTrimmedBottom (46.0f);
        const struct { const char* name; double lufs; juce::Colour colour; } rows[] { { "Effect off", example.off, theme::textDim }, { "Effect on", example.on, teal } };
        auto labels = area.removeFromLeft (90.0f);
        const float rowHeight = area.getHeight() / 2.0f;

        for (int i = 0; i < 2; ++i)
        {
            const float y = area.getY() + rowHeight * (float) i;
            text (g, rows[i].name, labels.withY (y).withHeight (rowHeight), 12.5f, juce::Justification::centredRight, theme::text);
            const float x1 = mapTo (rows[i].lufs, lo, hi, area.getX(), area.getRight());
            g.setColour (rows[i].colour);
            g.fillRect (area.getX() + 8.0f, y + rowHeight * 0.2f, x1 - area.getX() - 8.0f, rowHeight * 0.6f);
            text (g, num (rows[i].lufs) + " LUFS", { x1 + 8.0f, y, 100.0f, rowHeight }, 13.0f, juce::Justification::centredLeft, theme::text, true);
        }

        const double added = example.on - example.off;
        text (g, "The effect adds " + num (added) + " dB. Turn its output down " + num (added) + " dB to hear only what it does.", r.removeFromBottom (36.0f), 13.0f, juce::Justification::centred, theme::text);
    }

    //==============================================================================
    // 10. How DawIt Meter works

    void paintBlocks (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const std::vector<std::vector<juce::String>> rows {
            { "In (left, right)", "K-weighting: head shelf, low cut", "Mean square 0.4 s and 3 s", "Every 100 ms: blocks into 0.1 LU bins, gates", "M, S, I and LRA in LUFS and LU" },
            { "In (left, right)", "4 times oversampling (48 taps)", "Highest point kept", "True peak in dBTP" },
            { "In (left, right)", "Not weighted", "Peak, RMS and crest over 3 s", "Sample peak in dBFS, RMS, crest in dB" } };
        const char* captions[] { "Loudness", "True peak", "Levels" };
        const float rowHeight = r.getHeight() / 3.0f;

        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto row = r.withTop (r.getY() + rowHeight * (float) i).withHeight (rowHeight).reduced (0.0f, 8.0f);
            text (g, captions[i], row.removeFromTop (14.0f), 11.5f, juce::Justification::centredLeft, theme::textDim);
            const int n = (int) rows[i].size();
            const float gap = 18.0f, boxWidth = (row.getWidth() - gap * (float) (n - 1)) / (float) n;

            for (int b = 0; b < n; ++b)
            {
                const auto box = juce::Rectangle<float> (row.getX() + (float) b * (boxWidth + gap), row.getY() + 4.0f, boxWidth, row.getHeight() - 8.0f);
                g.setColour (b == 0 ? theme::panelLight : (b == n - 1 ? teal.withAlpha (0.28f) : theme::panel));
                g.fillRoundedRectangle (box, 5.0f);
                g.setColour (b == n - 1 ? teal : theme::gridBar);
                g.drawRoundedRectangle (box, 5.0f, 1.2f);
                text (g, rows[i][(size_t) b], box.reduced (6.0f, 3.0f), 11.5f, juce::Justification::centred, theme::text, false, 3);

                if (b < n - 1)
                {
                    const float y = box.getCentreY(), x = box.getRight();
                    g.setColour (theme::textDim);
                    g.drawLine (x + 2.0f, y, x + gap - 3.0f, y, 1.5f);
                    juce::Path head;
                    head.addTriangle (x + gap - 2.0f, y, x + gap - 8.0f, y - 4.0f, x + gap - 8.0f, y + 4.0f);
                    g.fillPath (head);
                }
            }
        }
    }

    //==============================================================================
    // 11. Your track

    /** Where "Your track" gets its numbers: this meter when it is in the song or a plugin, else the last song measurement. */
    struct TrackData
    {
        bool has = false, live = false;
        loudness::AdviceInput advice;
    };

    TrackData getTrackData (BuiltInMeter& processor)
    {
        TrackData d;
        const auto settings = processor.getSettings();
        d.advice.target = (double) settings.targetLufs;
        d.advice.ceiling = (double) settings.ceilingDbtp;
        d.advice.showTarget = settings.showTarget;
        d.advice.kind = processor.getTargetKind();
        const auto host = fxui::getHostTrack (processor);

        if (host.name.isNotEmpty())
        {
            d.has = d.live = true;
            d.advice.readings = processor.getReadings();
            d.advice.hostName = host.name;
            d.advice.host = fxui::isPlugin (processor) ? loudness::HostKind::unknown
                                                       : (host.isReturn ? loudness::HostKind::returnTrack : (host.isBus ? loudness::HostKind::bus : loudness::HostKind::track));
        }
        else if (songAccess.getLast != nullptr)
        {
            if (const auto last = songAccess.getLast())
            {
                d.has = true;
                d.advice.readings = last->readings;
                d.advice.scope = last->scope;
            }
        }

        return d;
    }

    void paintYourTrack (juce::Graphics& g, juce::Rectangle<float> r, BuiltInMeter& processor)
    {
        const auto d = getTrackData (processor);

        if (! d.has)
        {
            text (g, "No measurement yet.\n\nPlay the song through a DawIt Meter, or measure the whole song.", r, 14.0f, juce::Justification::centred, theme::textDim);
            return;
        }

        const auto& readings = d.advice.readings;
        const auto& a = d.advice;
        text (g, d.live ? (a.hostName.isNotEmpty() ? a.hostName + ": what DawIt Meter measured" : juce::String ("What DawIt Meter measured"))
                        : (a.scope.isNotEmpty() ? a.scope.trimCharactersAtEnd (":") : juce::String ("The last song measurement")),
              r.removeFromTop (16.0f), 12.0f, juce::Justification::centredLeft, theme::textDim);
        auto bars = r.removeFromTop (juce::jmin (r.getHeight() * 0.5f, 120.0f)).reduced (0.0f, 6.0f);
        const float rowHeight = bars.getHeight() / 2.0f;
        auto labels = bars.removeFromLeft (96.0f);
        bars.removeFromRight (86.0f);

        // Integrated loudness against the target.
        {
            constexpr double lo = -40.0, hi = 0.0;
            auto row = bars.withHeight (rowHeight).reduced (0.0f, 4.0f);
            text (g, "Integrated", labels.withHeight (rowHeight), 12.0f, juce::Justification::centredRight, theme::text);
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.fillRoundedRectangle (row, 3.0f);

            if (readings.integrated)
            {
                g.setColour (meterui::getLoudnessColour (readings.integrated, a.target, a.showTarget));
                g.fillRect (row.getX(), row.getY(), mapTo (*readings.integrated, lo, hi, 0.0f, row.getWidth()), row.getHeight());
                text (g, num (*readings.integrated) + " LUFS", { row.getRight() + 6.0f, row.getY(), 84.0f, row.getHeight() }, 12.0f, juce::Justification::centredLeft, theme::text, true);
            }

            if (a.showTarget)
            {
                const float x = mapTo (a.target, lo, hi, row.getX(), row.getRight());
                dashed (g, { x, row.getY() - 3.0f }, { x, row.getBottom() + 3.0f }, theme::text);
            }
        }

        // True peak against the ceiling.
        {
            constexpr double lo = -20.0, hi = 3.0;
            auto row = bars.withTop (bars.getY() + rowHeight).withHeight (rowHeight).reduced (0.0f, 4.0f);
            text (g, "True peak", labels.withY (labels.getY() + rowHeight).withHeight (rowHeight), 12.0f, juce::Justification::centredRight, theme::text);
            g.setColour (juce::Colours::black.withAlpha (0.3f));
            g.fillRoundedRectangle (row, 3.0f);

            if (readings.truePeak)
            {
                g.setColour (*readings.truePeak > a.ceiling ? theme::meterRed : theme::meterGreen);
                g.fillRect (row.getX(), row.getY(), mapTo (*readings.truePeak, lo, hi, 0.0f, row.getWidth()), row.getHeight());
                text (g, signedNum (*readings.truePeak) + " dBTP", { row.getRight() + 6.0f, row.getY(), 84.0f, row.getHeight() }, 12.0f, juce::Justification::centredLeft, theme::text, true);
            }

            const float x = mapTo (a.ceiling, lo, hi, row.getX(), row.getRight());
            dashed (g, { x, row.getY() - 3.0f }, { x, row.getBottom() + 3.0f }, theme::meterRed);
        }

        // The last two minutes, for a meter that has been playing.
        r.removeFromTop (6.0f);
        const auto history = d.live ? processor.getHistory() : std::vector<meterfx::HistoryPoint>();

        if (history.empty())
        {
            text (g, "LRA " + meterui::formatValue (readings.range) + " LU     loudest 3 seconds " + meterui::formatValue (readings.maxShortTerm) + " LUFS     measured " + loudness::formatClock (readings.seconds),
                  r.removeFromTop (30.0f), 12.5f, juce::Justification::centred, theme::textDim);
            return;
        }

        constexpr double lo = -48.0, hi = 0.0;
        auto plot = r.withTrimmedLeft (96.0f).withTrimmedRight (86.0f).withTrimmedBottom (18.0f);
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plot, 4.0f);
        std::vector<float> momentary, shortTerm;

        for (const auto& p : history) { momentary.push_back (p.momentary); shortTerm.push_back (p.shortTerm); }

        if (a.showTarget)
        {
            const float y = mapTo (a.target, lo, hi, plot.getBottom(), plot.getY());
            dashed (g, { plot.getX(), y }, { plot.getRight(), y }, theme::textDim);
        }

        drawSeries (g, momentary, plot, lo, hi, (double) meterfx::historyLength, teal.withAlpha (0.55f), 1.0f);
        drawSeries (g, shortTerm, plot, lo, hi, (double) meterfx::historyLength, teal, 2.5f);
        text (g, "the last 2 minutes: thin momentary, thick short-term, dashed target", { plot.getX(), plot.getBottom() + 2.0f, plot.getWidth(), 14.0f }, 10.5f, juce::Justification::centred, theme::textDim);
    }

    //==============================================================================
    void drawPicture (juce::Graphics& g, juce::Rectangle<float> area, int page, BuiltInMeter& processor)
    {
        g.setColour (theme::textDim);
        g.setFont (14.0f);
        const juce::String titles[] { "Same peak, different loudness", "The loudness scale", "The K-weighting curve", "Loudness over time", "How far the loudness moves",
                                      "A wave between its samples", "What each target would do", "Two masters, before and after", "An effect off and on", "From samples to numbers", "Your track" };
        g.drawText (titles[page], area.removeFromTop (28.0f), juce::Justification::centredLeft);
        area.reduce (12.0f, 8.0f);

        switch (page)
        {
            case 0:  paintPeaks (g, area); break;
            case 1:  paintScale (g, area, processor); break;
            case 2:  paintK (g, area); break;
            case 3:  paintOverTime (g, area, processor); break;
            case 4:  paintRange (g, area, processor); break;
            case 5:  paintTruePeak (g, area); break;
            case 6:  paintTargets (g, area, processor); break;
            case 7:  paintMasters (g, area); break;
            case 8:  paintMixing (g, area); break;
            case 9:  paintBlocks (g, area); break;
            default: paintYourTrack (g, area, processor); break;
        }
    }
}

void setSongAccess (SongAccess access) { songAccess = std::move (access); }

std::vector<fxui::Guide::Page> makeGuide (BuiltInMeter& processor)
{
    std::vector<fxui::Guide::Page> pages;

    for (int i = 0; i < numTextPages; ++i)
        pages.push_back ({ pageTexts[i].name, pageTexts[i].summary, juce::String::fromUTF8 (pageTexts[i].text),
                           [&processor, i] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, i, processor); } });

    const bool standIn = fxui::getHostTrack (processor).name.isEmpty();
    juce::String introduction = standIn
        ? "This page reads the last measurement of the whole song. Press Measure the song (or choose File > Measure Loudness...) and DawIt Meter renders the song, or the loop if it is on, without playing it, and reads it the way it reads a live signal.\n\n"
          "Then it says what it sees: how the integrated loudness and the true peak compare with the target and the ceiling in this window, and whether the range and the peaks are worth a look. Open this guide from a DawIt Meter on a bus that holds the whole mix, or last on a track, and this page reads what that meter hears instead."
        : "Play the song through the place where this DawIt Meter sits. It measures what passes through it and says what it sees: how the integrated loudness and the true peak compare with your target and ceiling, and whether the range and the peaks are worth a look.\n\n"
          "The picture shows the integrated loudness against the target, the true peak against the ceiling, and the last 2 minutes. For the song's real figure, play it from the start to the end.";

    fxui::Guide::Page yourTrack { "Your track", "Your numbers against the target", introduction,
                                  [&processor] (juce::Graphics& g, juce::Rectangle<float> r, double) { drawPicture (g, r, yourTrackPage, processor); } };

    if (standIn && songAccess.measure != nullptr)
    {
        yourTrack.ownClap = [measure = songAccess.measure] { measure(); };
        yourTrack.clapText = "Measure the song";
        yourTrack.clapTooltip = "Renders the song (or the loop, if it is on) without playing it and measures its loudness.";
    }
    else
    {
        yourTrack.noClap = true;
    }

    yourTrack.liveText = [&processor, introduction]
    {
        const auto d = getTrackData (processor);
        return d.has ? introduction + "\n\n" + loudness::advise (d.advice).joinIntoString ("\n\n") : introduction;
    };
    pages.push_back (std::move (yourTrack));
    return pages;
}
}
