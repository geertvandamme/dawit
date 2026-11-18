#include "Vocoder.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace daw::vocoder
{
double getTopEdge (double highHz, double sampleRate)
{
    return juce::jmin (highHz, topEdgeOfRate * sampleRate);
}

std::vector<double> getBandEdges (int bands, double lowHz, double highHz, double sampleRate)
{
    bands = juce::jlimit (1, maxBands, bands);
    const double top = juce::jmax (lowHz * 1.01, getTopEdge (highHz, sampleRate));
    std::vector<double> edges ((size_t) bands + 1);

    for (int k = 0; k <= bands; ++k)
        edges[(size_t) k] = lowHz * std::pow (top / lowHz, (double) k / bands);

    return edges;
}

std::vector<double> getBandCentres (int bands, double lowHz, double highHz, double sampleRate)
{
    const auto edges = getBandEdges (bands, lowHz, highHz, sampleRate);
    std::vector<double> centres (edges.size() - 1);

    for (size_t k = 0; k < centres.size(); ++k)
        centres[k] = std::sqrt (edges[k] * edges[k + 1]);

    return centres;
}

double getBandWidthOctaves (int bands, double lowHz, double highHz, double sampleRate)
{
    const double top = juce::jmax (lowHz * 1.01, getTopEdge (highHz, sampleRate));
    return std::log2 (top / lowHz) / juce::jlimit (1, maxBands, bands);
}

double getBandWidthSemitones (int bands, double lowHz, double highHz, double sampleRate)
{
    return 12.0 * getBandWidthOctaves (bands, lowHz, highHz, sampleRate);
}

double getBandQ (int bands, double lowHz, double highHz, double sampleRate)
{
    const double width = getBandWidthOctaves (bands, lowHz, highHz, sampleRate);
    return 1.0 / (2.0 * std::sinh (std::log (2.0) * width / 2.0));
}

int getConsonantBands (int bands, double lowHz, double highHz, double sampleRate)
{
    const auto edges = getBandEdges (bands, lowHz, highHz, sampleRate);
    const int count = (int) edges.size() - 1;
    int mask = 1 << (count - 1);

    for (int k = 0; k < count; ++k)
        if (edges[(size_t) k + 1] > consonantHz - 0.5)
            mask |= 1 << k;

    return mask;
}

double getFormantRatio (double semitones)
{
    return std::pow (2.0, semitones / 12.0);
}

double getNoteHz (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

juce::String getPitchName (int pitchClass)
{
    static const char* names[] = { "C", "C\xe2\x99\xaf", "D", "D\xe2\x99\xaf", "E", "F", "F\xe2\x99\xaf", "G", "G\xe2\x99\xaf", "A", "A\xe2\x99\xaf", "B" };
    return juce::String::fromUTF8 (names[((pitchClass % 12) + 12) % 12]);
}

int writeChordNotes (int pitchClassMask, int octaves, NoteList& out)
{
    int count = 0;

    for (int octave = 0; octave < juce::jlimit (1, 3, octaves); ++octave)
        for (int pc = 0; pc < 12; ++pc)
            if ((pitchClassMask & (1 << pc)) != 0 && count < maxCarrierVoices)
                out[(size_t) count++] = 48 + pc + 12 * octave;

    return count;
}

int writeChosenNotes (int root, Shape shape, int octaves, NoteList& out)
{
    int offsets[3] = { 0, 0, 0 }, numOffsets = 1;

    switch (shape)
    {
        case Shape::note:  break;
        case Shape::fifth: offsets[1] = 7; numOffsets = 2; break;
        case Shape::major: offsets[1] = 4; offsets[2] = 7; numOffsets = 3; break;
        case Shape::minor: offsets[1] = 3; offsets[2] = 7; numOffsets = 3; break;
    }

    int count = 0;

    for (int octave = 0; octave < juce::jlimit (1, 3, octaves); ++octave)
        for (int i = 0; i < numOffsets; ++i)
            if (count < maxCarrierVoices)
                out[(size_t) count++] = 48 + juce::jlimit (0, 11, root) + offsets[i] + 12 * octave;

    return count;
}

std::vector<int> getChordNotes (int pitchClassMask, int octaves)
{
    NoteList list {};
    const int count = writeChordNotes (pitchClassMask, octaves, list);
    return { list.begin(), list.begin() + count };
}

std::vector<int> getChosenNotes (int root, Shape shape, int octaves)
{
    NoteList list {};
    const int count = writeChosenNotes (root, shape, octaves, list);
    return { list.begin(), list.begin() + count };
}

int getPitchMask (const std::vector<int>& notes)
{
    int mask = 0;

    for (const int note : notes)
        mask |= 1 << (((note % 12) + 12) % 12);

    return mask;
}

//==============================================================================
const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> list;
        auto add = [&list] (const char* name, const char* why, const std::function<void (Settings&)>& change)
        {
            Settings s;
            change (s);
            list.push_back ({ name, why, s });
        };

        add ("Talking synth", "A saw chord supplies many harmonics for 16 bands to shape, while the chord follows the song's harmony.", [] (Settings&) {});
        add ("Soft choir", "Slower smoothing blurs fast consonants so vowels glide like singers, and detuned saws sound like a group.",
             [] (Settings& s) { s.bands = 12; s.wave = Wave::choir; s.smoothingMs = 40.0f; s.consonantsPercent = 25.0f; s.voiceDb = -18.0f; });
        add ("Robot", "One steady pitch is what a speaking machine sounds like; the short smoothing keeps it crisp.",
             [] (Settings& s) { s.source = Source::notes; s.root = 0; s.shape = Shape::note; s.wave = Wave::pulse; s.range = 1; s.smoothingMs = 8.0f; s.consonantsPercent = 60.0f; });
        add ("Whisper", "Noise has no pitch, so you hear the words and the breath only; a higher top band keeps the breath airy.",
             [] (Settings& s) { s.wave = Wave::noise; s.consonantsPercent = 0.0f; s.highHz = 10000.0f; });
        add ("Four bands", "Only four slices for a coarse, robotic sound. Word clarity depends on the voice and carrier; try more bands for finer detail.",
             [] (Settings& s) { s.bands = 4; });
        return list;
    }();

    return presets;
}

//==============================================================================
namespace
{
struct Resonator
{
    double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void bandPass (double hz, double q, double rate)
    {
        const double w = juce::MathConstants<double>::twoPi * hz / rate, alpha = std::sin (w) / (2.0 * q), a0 = 1.0 + alpha;
        b0 = alpha / a0; b1 = 0.0; b2 = -alpha / a0; a1 = -2.0 * std::cos (w) / a0; a2 = (1.0 - alpha) / a0;
    }

    void highPass (double hz, double rate)
    {
        const double w = juce::MathConstants<double>::twoPi * hz / rate, alpha = std::sin (w) / (2.0 * 0.7071), c = std::cos (w), a0 = 1.0 + alpha;
        b0 = (1.0 + c) / 2.0 / a0; b1 = -(1.0 + c) / a0; b2 = b0; a1 = -2.0 * c / a0; a2 = (1.0 - alpha) / a0;
    }

    double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

struct Vowel { double first, second, seconds; };
constexpr Vowel vowels[] = { { 730.0, 1090.0, 0.6 }, { 270.0, 2290.0, 0.6 }, { 300.0, 870.0, 0.5 } };
constexpr double gapSeconds = 0.15, hissSeconds = 0.25;

double sumSeconds()
{
    double total = hissSeconds;

    for (const auto& v : vowels)
        total += v.seconds + gapSeconds;

    return total;
}
}

double getTestVoiceSeconds()
{
    return sumSeconds();
}

std::vector<float> makeTestVoice (double rate)
{
    std::vector<float> out ((size_t) juce::roundToInt (sumSeconds() * rate), 0.0f);
    const double total = sumSeconds();
    double phase = 0.0, start = 0.0;
    juce::Random random (121);

    auto fill = [&] (double seconds, const std::function<double (double)>& sample, double peak)
    {
        const size_t first = (size_t) juce::roundToInt (start * rate), count = (size_t) juce::roundToInt (seconds * rate);
        std::vector<double> segment (count);
        double most = 1.0e-9;

        for (size_t i = 0; i < count; ++i)
        {
            const double t = start + (double) i / rate;
            const double fade = juce::jmin (1.0, juce::jmin ((double) i, (double) (count - i)) / (0.02 * rate));
            segment[i] = sample (t) * fade;
            most = juce::jmax (most, std::abs (segment[i]));
        }

        for (size_t i = 0; i < count && first + i < out.size(); ++i)
            out[first + i] = (float) (segment[i] * peak / most);
    };

    for (const auto& vowel : vowels)
    {
        Resonator one, two;
        one.bandPass (vowel.first, 8.0, rate);
        two.bandPass (vowel.second, 12.0, rate);

        fill (vowel.seconds, [&] (double t)
        {
            phase += (110.0 + (98.0 - 110.0) * t / total) / rate;
            phase -= std::floor (phase);
            const double source = 2.0 * phase - 1.0;
            return one.process (source) + 0.5 * two.process (source);
        }, 0.3);

        start += vowel.seconds + gapSeconds;
    }

    Resonator high;
    high.highPass (consonantHz, rate);
    fill (hissSeconds, [&] (double) { return high.process (random.nextDouble() * 2.0 - 1.0); }, 0.25);
    return out;
}

//==============================================================================
namespace
{
juce::String minus (int value)
{
    return value < 0 ? juce::String::fromUTF8 ("\xe2\x88\x92") + juce::String (-value) : juce::String (value);
}

juce::String withCommas (int value)
{
    const auto digits = juce::String (value);
    juce::String text;

    for (int i = 0; i < digits.length(); ++i)
    {
        if (i > 0 && (digits.length() - i) % 3 == 0)
            text << ",";

        text << digits.substring (i, i + 1);
    }

    return text;
}

bool isKnown (const SongKey& key)
{
    return key.mask != 0 && key.mask != 0xfff;
}
}

juce::StringArray getAdvice (const Summary& summary, const Settings& s, const SongKey& key)
{
    juce::StringArray advice;
    const int inputPeak = juce::roundToInt (summary.inputPeakDb);

    if (summary.inputPeakDb < -60.0f)
    {
        advice.add ("Nothing was heard from this track in the last 8 seconds. Play a part with a voice, or press Say \"ah ee s\".");
        return advice;
    }

    if (summary.inputPeakDb < -40.0f)
        advice.add ("Your voice peaks at " + minus (inputPeak) + " dB. The bands can only give back what they hear: raise the track's level or compress it before the vocoder.");

    const auto chosen = getChosenNotes (s.root, s.shape, s.range);

    const bool pitched = s.wave != Wave::noise; // noise has no pitch, so notes and keys do not matter

    if (pitched && s.source == Source::songChord && summary.chordFallbackFraction > 0.5)
    {
        juce::StringArray names;

        for (const int pc : { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 })
            if ((getPitchMask (chosen) & (1 << pc)) != 0)
                names.add (getPitchName (pc));

        advice.add ("Song chord found no chord for over half the recent audio, so your chosen notes (" + names.joinIntoString (", ") + ") played. Add chords on the chord track or choose Notes.");
    }

    if (pitched && isKnown (key) && (s.source == Source::notes || (s.source == Source::songChord && summary.chordFallbackFraction > 0.5)))
    {
        for (const int note : chosen)
        {
            const int pc = note % 12;

            if ((key.mask & (1 << pc)) != 0)
                continue;

            const auto name = getPitchName (pc);

            if (key.tonic >= 0)
                advice.add (name + " is not in " + getPitchName (key.tonic) + (key.minor ? " minor" : " major") + ". Try the " + (key.minor ? "Minor" : "Major") + " shape on " + getPitchName (key.tonic) + ".");
            else
                advice.add (name + " is not in the song's key. Try another root or shape.");

            break;
        }
    }

    const int hissPercent = juce::roundToInt (summary.hissFraction * 100.0);

    if (summary.hissFraction >= 0.05 && s.consonantsPercent < 20.0f)
        advice.add ("The input sounds hissy about " + juce::String (hissPercent) + "% of the time, but Consonants is " + juce::String (juce::roundToInt (s.consonantsPercent)) + "%. Try 40% if consonants sound dull.");

    if (summary.hissFraction >= 0.05 && s.highHz < 6000.0f)
        advice.add ("High is " + withCommas (juce::roundToInt (s.highHz)) + " Hz, leaving a narrow region for high-frequency hiss. Try 8,000 Hz if s sounds lose definition.");

    if (summary.limiterHits > 0)
        advice.add ("The soft limiter caught peaks. Lower Carrier by 3 dB.");

    if (s.bands == 4)
        advice.add ("With 4 bands the sound is coarse and robotic. Try 12 or 16 for finer detail if words are hard to follow.");

    if (advice.isEmpty())
        advice.add ("Voice peaks at " + minus (inputPeak) + " dB, result at " + minus (juce::roundToInt (summary.resultPeakDb)) + " dB. Compare their loudness by ear. Try Soft choir for a pad.");

    return advice;
}
}
