#include "Tune.h"
#include "SungVoice.h"

namespace daw::tune
{
Zone getZone (double cents) noexcept
{
    const double d = std::abs (cents);
    return d <= inTuneCents ? Zone::inTune : (d <= closeCents ? Zone::close : Zone::out);
}

juce::String getZoneName (Zone zone)
{
    switch (zone)
    {
        case Zone::inTune: return "In tune";
        case Zone::close:  return "Close";
        case Zone::out:    return "Out";
    }

    return {};
}

int chooseTarget (double midi, int mask, int previous)
{
    mask &= allNotes;

    if (mask == 0)
        mask = allNotes;

    auto allowed = [mask] (int note) { return (mask & (1 << (((note % 12) + 12) % 12))) != 0; };
    const int centre = (int) std::floor (midi);
    int best = -1;
    double bestDistance = 1.0e9;

    // The nearest allowed note; scanning upwards with a strict test keeps the lower one on a tie.
    for (int note = centre - 12; note <= centre + 13; ++note)
    {
        if (! allowed (note))
            continue;

        const double d = std::abs (midi - note);

        if (d < bestDistance - 1.0e-9)
        {
            bestDistance = d;
            best = note;
        }
    }

    // Stay on the note before until the pitch is 15 cents past the halfway point: that is 30 cents more
    // distance from it than from the nearest.
    if (previous >= 0 && previous != best && allowed (previous) && std::abs (midi - previous) - bestDistance <= 0.30 + 1.0e-9)
        return previous;

    return best;
}

double applyTolerance (double errorCents, double toleranceCents) noexcept
{
    return std::abs (errorCents) <= toleranceCents ? 0.0 : errorCents;
}

double applyAmount (double errorCents, double amountPercent) noexcept
{
    return errorCents * amountPercent * 0.01;
}

double Glide::advance (double target, double seconds, double speedMs) noexcept
{
    const double tau = speedMs * 0.001 / 3.0;

    if (tau <= 1.0e-9)
        current = target;
    else
        current += (target - current) * (1.0 - std::exp (-seconds / tau));

    return current;
}

void CentreFilter::prepare (double framesPerSecond)
{
    const double k = std::tan (juce::MathConstants<double>::pi * cutoffHz / framesPerSecond);
    const double q = juce::MathConstants<double>::sqrt2 * 0.5;
    const double norm = 1.0 / (1.0 + k / q + k * k);
    b0 = k * k * norm;
    b1 = 2.0 * b0;
    b2 = b0;
    a1 = 2.0 * (k * k - 1.0) * norm;
    a2 = (1.0 - k / q + k * k) * norm;
    z1 = z2 = 0.0;
}

void CentreFilter::restart (double value) noexcept
{
    z1 = value * (1.0 - b0);
    z2 = value * (b2 - a2);
}

double CentreFilter::process (double x) noexcept
{
    const double y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

//==============================================================================
const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets {
        { "Check only", "Changes nothing. Shows each note and how many cents it's out, so you learn how the take sits before fixing anything.",
          { Mode::check, Target::key, 50.0f, 100.0f, 10.0f, true } },
        { "Natural fix", "Pulls notes into tune within about 50 ms, so slides and scoops still sound sung, leaves notes within 10 cents alone and keeps the vibrato. The usual light fix for a lead vocal.",
          { Mode::correct, Target::key, 50.0f, 100.0f, 10.0f, true } },
        { "Gentle polish", "Removes a little over half of what's out, slowly, so the singer's character stays. For a good take with a few loose notes.",
          { Mode::correct, Target::key, 120.0f, 60.0f, 15.0f, true } },
        { "Hard tune", "Jumps to the nearest note of the key at once, so slides become steps: the robotic sound of Cher's Believe and much pop and hip-hop since. A sound, not a fix.",
          { Mode::correct, Target::key, 0.0f, 100.0f, 0.0f, false } }
    };

    return presets;
}

int getTargetMask (Target target, int keyMask, int chordMask) noexcept
{
    switch (target)
    {
        case Target::key:       return keyMask & allNotes;
        case Target::chords:    return chordMask != 0 ? chordMask & allNotes : keyMask & allNotes;
        case Target::chromatic: return allNotes;
    }

    return allNotes;
}

//==============================================================================
void Corrector::prepare (double framesPerSecond)
{
    fps = framesPerSecond > 1.0 ? framesPerSecond : 200.0;
    maxGapFrames = juce::jmax (1, (int) std::ceil (0.025 * fps));
    filter.prepare (fps);
    reset();
}

void Corrector::reset() noexcept
{
    active = false;
    gapFrames = 0;
    previousTarget = -1;
    glide.reset (0.0);
}

Step Corrector::step (double midi, const Settings& s, int mask) noexcept
{
    Step out;

    if (midi < 0.0)
    {
        if (active && ++gapFrames > maxGapFrames)
            active = false;

        return out;
    }

    // A new note: after a gap, or a jump of more than a semitone from the frame before.
    if (! active || std::abs (midi - lastMidi) > 1.0)
    {
        active = true;
        filter.restart (midi);
        previousTarget = -1;
        glide.reset (0.0);
    }

    gapFrames = 0;
    lastMidi = midi;
    const double centre = s.keepVibrato ? filter.process (midi) : midi;
    const int target = chooseTarget (centre, mask, previousTarget);
    previousTarget = target;
    double goal = 0.0;

    if (s.mode == Mode::correct)
        goal = applyAmount (applyTolerance ((target - centre) * 100.0, s.toleranceCents), s.amountPercent);

    out.voiced = true;
    out.centreMidi = centre;
    out.target = target;
    out.correctionCents = glide.advance (goal, 1.0 / fps, s.speedMs);
    return out;
}

std::vector<float> simulate (const std::vector<float>& sung, const Settings& settings, int mask, double framesPerSecond)
{
    Corrector corrector;
    corrector.prepare (framesPerSecond);
    std::vector<float> out (sung.size(), -1.0f);

    for (size_t i = 0; i < sung.size(); ++i)
    {
        const auto step = corrector.step (sung[i], settings, mask);

        if (step.voiced)
            out[i] = sung[i] + (float) (step.correctionCents * 0.01);
    }

    return out;
}

//==============================================================================
juce::AudioBuffer<float> makeTake (const std::vector<TakeNote>& notes, double rate, double seconds)
{
    juce::AudioBuffer<float> audio (1, juce::jmax (1, (int) std::ceil (seconds * rate)));
    audio.clear();
    auto* out = audio.getWritePointer (0);
    SungVoice voice;
    voice.prepare (rate);

    for (const auto& note : notes)
    {
        const int first = juce::jmax (0, (int) (note.start * rate)), last = juce::jmin (audio.getNumSamples(), (int) (note.end * rate));

        for (int i = first; i < last; ++i)
        {
            const double t = i / rate - note.start;
            const double cents = note.cents ? note.cents (t) : 0.0;
            const double attack = juce::jmin (1.0, t / 0.015), release = juce::jmin (1.0, (note.end - i / rate) / 0.03);
            out[i] += (float) (voice.next (note.midi + cents * 0.01) * note.level * attack * release);
        }
    }

    return audio;
}

std::vector<float> makeFrames (const std::vector<TakeNote>& notes, double seconds, double frameSeconds)
{
    std::vector<float> frames ((size_t) std::ceil (seconds / frameSeconds), -1.0f);

    for (size_t i = 0; i < frames.size(); ++i)
    {
        const double t = (double) i * frameSeconds;

        for (const auto& note : notes)
            if (t >= note.start && t < note.end)
                frames[i] = (float) (note.midi + (note.cents ? note.cents (t - note.start) : 0.0) * 0.01);
    }

    return frames;
}

TakeNote getOffNote()
{
    TakeNote n;
    n.midi = 57.0;   // A3
    n.start = 0.0;
    n.end = 2.0;
    n.cents = [] (double t)
    {
        const double base = t < 0.25 ? -150.0 + 120.0 * (t / 0.25) : -30.0;
        const double vibrato = t > 0.5 ? 15.0 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * (t - 0.5)) * juce::jmin (1.0, (t - 0.5) / 0.3) : 0.0;
        return base + vibrato;
    };
    return n;
}

std::vector<TakeNote> getExamplePhraseNotes()
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;
    TakeNote e, g, a;
    e.midi = 64; e.start = 0.0; e.end = 0.9;
    e.cents = [] (double t) { return t < 0.2 ? -150.0 + 140.0 * (t / 0.2) : -10.0; };
    g.midi = 67; g.start = 1.0; g.end = 1.9;
    g.cents = [] (double t) { return -35.0 + (t >= 0.3 ? 20.0 * std::sin (twoPi * 5.5 * (t - 0.3)) : 0.0); };
    a.midi = 69; a.start = 2.0; a.end = 3.0;
    a.cents = [] (double t) { return t < 0.4 ? 0.0 : -30.0 * (t - 0.4) / 0.6; };
    return { e, g, a };
}

std::vector<TakeNote> getScoopNotes()
{
    TakeNote e;
    e.midi = 64; e.start = 0.0; e.end = 1.0;
    e.cents = [] (double t) { return t < 0.2 ? -150.0 + 140.0 * (t / 0.2) : -10.0; };
    return { e };
}

std::vector<TakeNote> getVibratoNotes()
{
    TakeNote n;
    n.midi = 69; n.start = 0.0; n.end = 2.0;
    n.cents = [] (double t) { return -30.0 + 40.0 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * t); };
    return { n };
}

std::vector<TakeNote> getHardTuneNotes()
{
    TakeNote n;
    n.midi = 67; n.start = 0.0; n.end = 1.35;
    n.cents = [] (double t)
    {
        if (t < 0.6)
            return 30.0 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * t);

        if (t < 0.75)
            return 200.0 * (t - 0.6) / 0.15;

        return 200.0;
    };
    return { n };
}
}
