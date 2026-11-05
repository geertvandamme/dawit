#include "Harmoniser.h"

#include <map>

namespace daw::harm
{
namespace
{
    constexpr int cMajorMask = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11);

    int countBits (int mask) noexcept
    {
        int n = 0;

        for (int i = 0; i < 12; ++i)
            n += (mask >> i) & 1;

        return n;
    }

    juce::String getOrdinal (int n)
    {
        if (n % 100 >= 11 && n % 100 <= 13)
            return juce::String (n) + "th";

        switch (n % 10)
        {
            case 1:  return juce::String (n) + "st";
            case 2:  return juce::String (n) + "nd";
            case 3:  return juce::String (n) + "rd";
            default: return juce::String (n) + "th";
        }
    }

    juce::String withArticle (const juce::String& name)
    {
        // No "u": the only size name starting with it is "unison", which takes "a".
        return (juce::String ("aeio").containsChar (name[0]) ? "an " : "a ") + name;
    }

    /** The size of a shift in Key harmony. 6 semitones is named by the steps of the voice: 3 steps (a 4th, F to B) is an
        augmented 4th, 4 steps (a 5th, B to F) a diminished 5th. */
    juce::String getKeySizeName (int semitones, int steps)
    {
        if (semitones != 6)
            return getSizeName (semitones);

        const int s = std::abs (steps) % 7;
        return s == 3 ? juce::String ("augmented 4th") : s == 4 ? juce::String ("diminished 5th") : getSizeName (semitones);
    }

    int percent (double share) { return juce::roundToInt (share * 100.0); }
}

//==============================================================================
const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets = []
    {
        auto make = [] (const char* name, const char* why, Mode mode, float mix, const Voice& a, const Voice& b)
        {
            Preset p;
            p.name = name;
            p.why = why;
            p.settings.mode = mode;
            p.settings.mixPercent = mix;
            p.settings.voices = { a, b };
            return p;
        };
        //                       on     st  steps level  pan  cents ms
        auto voice = [] (bool on, int semitones, int steps, float level, float pan, float cents = 0.0f, float ms = 0.0f)
        {
            return Voice { on, semitones, steps, level, pan, cents, ms };
        };

        return std::vector<Preset> {
            make ("Third above", "The most used harmony: two steps up the scale, under the lead.", Mode::key, 50.0f,
                  voice (true, 4, 2, -6.0f, 0.0f), voice (false, 7, -2, -6.0f, 0.0f)),
            make ("Thirds above and below", "A harmony each side of the lead, for a fuller chorus.", Mode::key, 50.0f,
                  voice (true, 4, 2, -7.0f, -40.0f), voice (true, -4, -2, -7.0f, 40.0f)),
            make ("Third and fifth above", "A three-note chord on every note. Rich on a hook; watch the 7th of the scale (Where the key isn't enough).", Mode::key, 50.0f,
                  voice (true, 4, 2, -6.0f, -30.0f), voice (true, 7, 4, -8.0f, 30.0f)),
            make ("Octave below", "Weight under a lead without changing its notes.", Mode::fixed, 50.0f,
                  voice (true, -12, 2, -8.0f, 0.0f), voice (false, 7, -2, -6.0f, 0.0f)),
            make ("Fifth and octave", "The power-chord sound, for a single-note guitar or synth riff.", Mode::fixed, 50.0f,
                  voice (true, 7, 2, -6.0f, 0.0f), voice (true, 12, -2, -10.0f, 0.0f)),
            make ("Wide double", "Two slightly different copies either side make the lead wider, like a second take.", Mode::fixed, 50.0f,
                  voice (true, 0, 2, -8.0f, -70.0f, 7.0f, 12.0f), voice (true, 0, -2, -8.0f, 70.0f, -7.0f, 20.0f))
        };
    }();

    return presets;
}

//==============================================================================
int getKeyMask (int keyChoice, Scale scale, int songMask) noexcept
{
    if (keyChoice >= 1 && keyChoice <= 12)
    {
        const int tonic = keyChoice - 1;
        int mask = 0;

        for (const int interval : scale == Scale::major ? std::array<int, 7> { 0, 2, 4, 5, 7, 9, 11 } : std::array<int, 7> { 0, 2, 3, 5, 7, 8, 10 })
            mask |= 1 << ((tonic + interval) % 12);

        return mask;
    }

    songMask &= tune::allNotes;
    return countBits (songMask) == 7 ? songMask : cMajorMask;
}

std::vector<int> getMaskNotes (int mask)
{
    std::vector<int> notes;

    for (int pc = 0; pc < 12; ++pc)
        if ((mask & (1 << pc)) != 0)
            notes.push_back (pc);

    return notes;
}

int stepInKey (int note, int steps, int mask)
{
    const auto notes = getMaskNotes (mask);

    if (notes.empty())
        return note + steps;

    const int count = (int) notes.size();
    const int pc = ((note % 12) + 12) % 12;
    const int octaveBase = note - pc;
    int index = 0;

    for (int i = 0; i < count; ++i)
        if (notes[(size_t) i] <= pc)
            index = i;

    const int total = index + steps;
    const int octaves = (int) std::floor ((double) total / count);
    const int within = total - octaves * count;
    return octaveBase + 12 * octaves + notes[(size_t) within];
}

int getReference (double sungMidi, int mask, int previous)
{
    return tune::chooseTarget (sungMidi, mask, previous);
}

double getShift (const Voice& voice, Mode mode, int reference, int mask)
{
    const int shift = mode == Mode::fixed ? voice.semitones : stepInKey (reference, voice.steps, mask) - reference;
    return (double) juce::jlimit (-12, 12, shift);
}

double getShift (double sungMidi, const Voice& voice, Mode mode, int mask, int& previousReference)
{
    previousReference = getReference (sungMidi, mask, previousReference);
    return getShift (voice, mode, previousReference, mask);
}

double getRatio (double shiftSemitones, float detuneCents) noexcept
{
    return std::pow (2.0, shiftSemitones / 12.0 + (double) detuneCents / 1200.0);
}

//==============================================================================
juce::String getSizeName (int semitones)
{
    static const char* const names[] { "unison", "minor 2nd", "major 2nd", "minor 3rd", "major 3rd", "perfect 4th", "tritone",
                                       "perfect 5th", "minor 6th", "major 6th", "minor 7th", "major 7th", "octave" };
    return names[juce::jlimit (0, 12, std::abs (semitones))];
}

juce::String getIntervalName (Mode mode, int value)
{
    if (value == 0)
        return "unison";

    const juce::String direction = value > 0 ? " up" : " down";

    if (mode == Mode::fixed)
        return getSizeName (value) + direction;

    const int degree = std::abs (value) + 1;
    return (degree == 8 ? juce::String ("octave") : getOrdinal (degree)) + direction;
}

juce::String getNoteName (int pitchClass)
{
    static const char* const names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((pitchClass % 12) + 12) % 12];
}

float getLevelGain (float levelDb) noexcept
{
    return levelDb <= minLevelDb ? 0.0f : std::pow (10.0f, levelDb / 20.0f);
}

std::pair<float, float> getMixGains (float mixPercent) noexcept
{
    const float m = juce::jlimit (0.0f, 1.0f, mixPercent / 100.0f);
    return { juce::jmin (1.0f, 2.0f * (1.0f - m)), juce::jmin (1.0f, 2.0f * m) };
}

std::pair<float, float> getPanGains (float panPercent) noexcept
{
    const float angle = (juce::jlimit (-100.0f, 100.0f, panPercent) * 0.01f + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    return { std::cos (angle), std::sin (angle) };
}

//==============================================================================
juce::String getLatencyLine (const juce::String& trackKind, double lateMs)
{
    const auto late = lateMs > 0.0 ? juce::String (juce::roundToInt (lateMs)) + " ms" : juce::String ("about 27 ms");
    return "On " + juce::String (trackKind.startsWith ("instrument") ? "an " : "a ") + trackKind
           + ", DawIt Harmoniser makes everything in it " + late + " late. Put it on the audio track itself.";
}

std::vector<juce::String> getAdvice (const std::vector<Frame>& frames, const Settings& s, const KeyInfo& key, double fps, const juce::String& latencyLine)
{
    constexpr double minSeconds = 2.0;
    fps = juce::jmax (1.0, fps);
    int loud = 0, clear = 0;
    double centsSum = 0.0;

    for (const auto& f : frames)
    {
        if (f.rmsDb <= -50.0f)
            continue;

        ++loud;

        if (f.lead >= 0.0f)
        {
            ++clear;
            centsSum += std::abs ((double) f.lead - std::round ((double) f.lead)) * 100.0;
        }
    }

    if (loud == 0)
        return { "Play the song once with DawIt Harmoniser on: this page fills with what it heard." };

    std::vector<juce::String> lines;
    bool advised = false;
    const double clearShare = (double) clear / loud;

    // Rule 2: few clear notes.
    if ((double) loud / fps >= minSeconds && clearShare < 0.5)
    {
        lines.push_back ("Only " + juce::String (percent (clearShare)) + "% of the sound had one clear note, so the voices were quiet most of the time. "
                         "DawIt Harmoniser follows one voice or one single-note line; on chords, a full mix or a noisy recording it can't find the note.");
        advised = true;
    }

    bool allInKey = key.known;
    int voicedVoiceFrames = 0;

    for (int v = 0; v < numVoices; ++v)
    {
        const auto& voice = s.voices[(size_t) v];

        if (! voice.on)
            continue;

        std::map<int, int> sizes;             // the size of the shift in semitones, and how many frames
        std::map<int, int> leadNotesAtSix;    // lead pitch classes where the shift is a tritone
        std::map<int, int> outside;           // voice pitch classes outside the key
        int voiced = 0, six = 0, out = 0;

        for (const auto& f : frames)
        {
            const float note = f.voice[(size_t) v];

            if (note < 0.0f || f.lead < 0.0f)
                continue;

            ++voiced;
            const int shift = std::abs (juce::roundToInt (note - f.lead));
            ++sizes[shift];

            if (shift == 6)
            {
                ++six;
                ++leadNotesAtSix[((juce::roundToInt (f.lead) % 12) + 12) % 12];
            }

            const int pc = ((juce::roundToInt (note) % 12) + 12) % 12;

            if (key.known && (key.mask & (1 << pc)) == 0)
            {
                ++out;
                ++outside[pc];
            }
        }

        voicedVoiceFrames += voiced;

        if (out > 0)
            allInKey = false;

        if ((double) voiced / fps < minSeconds)
            continue;

        const juce::String label = "Voice " + juce::String (v + 1);

        // Rule 3: which sizes the key made.
        if (s.mode == Mode::key)
        {
            std::vector<std::pair<int, int>> ordered (sizes.begin(), sizes.end());
            std::stable_sort (ordered.begin(), ordered.end(), [] (const auto& a, const auto& b) { return a.second > b.second; });
            juce::String text = label + " (" + getIntervalName (Mode::key, voice.steps) + ") made ";

            if (ordered.size() == 1)
                text += withArticle (getKeySizeName (ordered[0].first, voice.steps)) + " on every note.";
            else
            {
                const size_t shown = juce::jmin ((size_t) 3, ordered.size());

                for (size_t i = 0; i < shown; ++i)
                {
                    if (i > 0)
                        text += i + 1 == shown ? " and " : ", ";

                    text += withArticle (getKeySizeName (ordered[i].first, voice.steps)) + " on " + juce::String (percent ((double) ordered[i].second / voiced)) + "%";

                    if (i == 0)
                        text += " of the notes";
                }

                text += ": the key decides which.";
            }

            lines.push_back (text);

            // Rule 4: the tritone (an augmented 4th or a diminished 5th, by the voice's steps).
            if ((double) six / voiced >= 0.05)
            {
                std::vector<std::pair<int, int>> notes (leadNotesAtSix.begin(), leadNotesAtSix.end());
                std::stable_sort (notes.begin(), notes.end(), [] (const auto& a, const auto& b) { return a.second > b.second; });
                juce::String where;

                for (size_t i = 0; i < notes.size() && i < 3; ++i)
                    where += (i == 0 ? "" : (i + 1 == notes.size() || i == 2 ? " and " : ", ")) + getNoteName (notes[i].first);

                lines.push_back (label + " (" + getIntervalName (Mode::key, voice.steps) + ") made " + withArticle (getKeySizeName (6, voice.steps)) + " (6 semitones) on " + juce::String (percent ((double) six / voiced))
                                 + "% of the notes, on " + where + (key.known ? " in " + key.name : juce::String()) + ". If it sounds harsh there, try a 3rd or a 6th, or sing that line yourself.");
                advised = true;
            }
        }

        // Rule 5: Fixed leaves the key.
        if (s.mode == Mode::fixed && key.known && voice.semitones % 12 != 0 && (double) out / voiced >= 0.2)
        {
            std::vector<std::pair<int, int>> notes (outside.begin(), outside.end());
            std::stable_sort (notes.begin(), notes.end(), [] (const auto& a, const auto& b) { return a.second > b.second; });
            juce::String names;

            for (size_t i = 0; i < notes.size() && i < 5; ++i)
                names += (i == 0 ? "" : ", ") + getNoteName (notes[i].first);

            lines.push_back ("In Fixed, " + label.toLowerCase() + " (" + (voice.semitones > 0 ? "+" : "") + juce::String (voice.semitones) + ") landed outside " + key.name + " on "
                             + juce::String (percent ((double) out / voiced)) + "% of the notes (" + names + "). Try Harmony: Key to keep the voices in the key, unless you want that sound.");
            advised = true;
        }
    }

    // Rule 6: a lead that is out of tune.
    if ((double) clear / fps >= minSeconds && centsSum / clear > 25.0)
    {
        lines.push_back ("The lead is on average " + juce::String (juce::roundToInt (centsSum / clear)) + " cents from the nearest note, and the voices copy that. "
                         "Put DawIt Tune before DawIt Harmoniser to tune the lead first.");
        advised = true;
    }

    // Rule 7: late on a bus, return or instrument track.
    if (latencyLine.isNotEmpty())
    {
        lines.push_back (latencyLine);
        advised = true;
    }

    // Rule 8: nothing to say.
    if (! advised)
        lines.push_back ("The voices followed the lead on " + juce::String (percent (clearShare)) + "% of the loud sound"
                         + (allInKey && voicedVoiceFrames > 0 ? ", all inside the key." : "."));

    return lines;
}

//==============================================================================
std::vector<tune::TakeNote> getPhraseNotes()
{
    static const int pitch[] { 64, 64, 67, 64, 62, 60 };
    static const double start[] { 0.0, 1.0, 2.0, 3.0, 4.0, 4.5 }, length[] { 1.0, 1.0, 1.0, 1.0, 0.5, 3.5 };
    constexpr double beat = 60.0 / 90.0;
    std::vector<tune::TakeNote> notes;

    for (size_t i = 0; i < 6; ++i)
    {
        tune::TakeNote n;
        n.midi = pitch[i];
        n.start = start[i] * beat;
        n.end = (start[i] + length[i]) * beat;
        notes.push_back (n);
    }

    return notes;
}
}
