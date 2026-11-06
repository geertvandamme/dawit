#include "TimeSignature.h"

#include <cmath>

namespace daw::timesig
{
namespace
{
    const char* const wordsForNumbers[] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
                                            "eleven", "twelve", "thirteen", "fourteen", "fifteen" };

    juce::String number (int n) { return n >= 0 && n <= 15 ? juce::String (wordsForNumbers[n]) : juce::String (n); }

    juce::String noteName (int denominator)
    {
        switch (denominator)
        {
            case 2:  return "half";
            case 4:  return "quarter";
            case 8:  return "eighth";
            case 16: return "sixteenth";
            default: break;
        }

        return "quarter";
    }

    juce::String groupText (const std::vector<int>& groups)
    {
        juce::StringArray parts;

        for (int g : groups)
            parts.add (juce::String (g));

        return parts.joinIntoString ("+");
    }
} // namespace

const std::vector<TimeSignature>& getCommon()
{
    static const std::vector<TimeSignature> list { { 2, 4 }, { 3, 4 }, { 4, 4 }, { 5, 4 }, { 6, 8 }, { 7, 8 }, { 9, 8 }, { 12, 8 }, { 2, 2 } };
    return list;
}

bool isValidDenominator (int denominator) noexcept
{
    return denominator == 2 || denominator == 4 || denominator == 8 || denominator == 16;
}

TimeSignature tidy (TimeSignature s) noexcept
{
    s.numerator = juce::jlimit (minNumerator, maxNumerator, s.numerator);

    if (! isValidDenominator (s.denominator))
        s.denominator = 4;

    return s;
}

double getBarLength (TimeSignature s) noexcept
{
    return s.numerator * 4.0 / s.denominator;
}

double getPulseLength (TimeSignature s) noexcept
{
    return 4.0 / s.denominator;
}

Kind getKind (TimeSignature s) noexcept
{
    if (s.numerator == 6 || s.numerator == 9 || s.numerator == 12 || s.numerator == 15)
        return Kind::compound;

    if (s.numerator <= 4)
        return Kind::simple;

    return Kind::odd;
}

std::vector<int> getGroups (TimeSignature s)
{
    const int n = s.numerator;
    std::vector<int> groups;

    if (n == 1)
    {
        groups.push_back (1);
    }
    else if (n == 6 || n == 9 || n == 12 || n == 15)
    {
        groups.assign ((size_t) (n / 3), 3);
    }
    else if (n <= 4 || s.denominator <= 4)
    {
        groups.assign ((size_t) n, 1);
    }
    else
    {
        groups.assign ((size_t) (n / 2), 2);

        if (n % 2 == 1)
            groups.back() = 3;   // twos and a final three: 5 = 2+3, 7 = 2+2+3
    }

    return groups;
}

std::vector<double> getGroupStarts (TimeSignature s)
{
    std::vector<double> starts;
    double at = 0.0;
    const double pulse = getPulseLength (s);

    for (int g : getGroups (s))
    {
        starts.push_back (at);
        at += g * pulse;
    }

    return starts;
}

bool hasLongGroups (TimeSignature s)
{
    for (int g : getGroups (s))
        if (g > 1)
            return true;

    return false;
}

Click getClick (TimeSignature s, int pulseInBar)
{
    if (pulseInBar == 0)
        return Click::bar;

    if (! hasLongGroups (s))
        return Click::beat;

    int at = 0;

    for (int g : getGroups (s))
    {
        if (at == pulseInBar)
            return Click::group;

        at += g;
    }

    return Click::beat;
}

int getBarAt (TimeSignature s, double beat) noexcept
{
    return (int) std::floor (beat / getBarLength (s) + 1.0e-9);
}

double getBarStart (TimeSignature s, int bar) noexcept
{
    return bar * getBarLength (s);
}

double getBarStartAtOrBefore (TimeSignature s, double beat) noexcept
{
    return getBarStart (s, getBarAt (s, beat));
}

juce::String formatPosition (TimeSignature s, double beat)
{
    const double length = getBarLength (s), pulse = getPulseLength (s);
    const int bar = (int) std::floor (beat / length + 1.0e-9);
    const double inBar = juce::jmax (0.0, beat - bar * length);
    const int beatInBar = juce::jmin (s.numerator - 1, (int) std::floor (inBar / pulse + 1.0e-9));
    const int perPulse = juce::jmax (1, (int) std::lround (pulse * 4.0));
    const int sixteenth = juce::jlimit (0, perPulse - 1, (int) std::floor ((inBar - beatInBar * pulse) * 4.0 + 1.0e-9));

    return juce::String (bar + 1) + "." + juce::String (beatInBar + 1) + "." + juce::String (sixteenth + 1);
}

juce::String toString (TimeSignature s)
{
    return juce::String (s.numerator) + "/" + juce::String (s.denominator);
}

juce::String describe (TimeSignature s)
{
    const auto note = noteName (s.denominator);
    const auto groups = getGroups (s);

    switch (getKind (s))
    {
        case Kind::simple:
            return number (s.numerator) + " " + note + "-note " + (s.numerator == 1 ? "beat" : "beats") + " in each bar";

        case Kind::compound:
        {
            const int beats = s.numerator / 3;
            return number (beats) + " beats in each bar, each three " + note + " notes long (" + number (s.numerator) + " "
                   + note + "s in all)";
        }

        case Kind::odd:
            break;
    }

    if (s.denominator <= 4)
        return number (s.numerator) + " " + note + "-note beats in each bar";

    return number (s.numerator) + " " + note + " notes in each bar, grouped " + groupText (groups);
}

double getFeltBpm (TimeSignature s, double bpm) noexcept
{
    // The felt beat is a dotted note of three bottom notes: 6/8 gives a dotted quarter (1.5 quarters), 6/4 a dotted half.
    return getKind (s) == Kind::compound ? bpm / (3.0 * getPulseLength (s)) : bpm;
}

double getBarSeconds (TimeSignature s, double bpm) noexcept
{
    return getBarLength (s) * 60.0 / bpm;
}

int getMidiClocksPerClick (TimeSignature s) noexcept
{
    const double pulseClocks = 24.0 * getPulseLength (s);

    // A click per beat of three where the beats are all dotted notes; otherwise a click per bottom note.
    if (getKind (s) == Kind::compound)
        return (int) std::lround (pulseClocks * 3.0);

    return (int) std::lround (pulseClocks);
}

double mapBeatKeepingBars (double beat, TimeSignature from, TimeSignature to) noexcept
{
    return beat * getBarLength (to) / getBarLength (from);
}
} // namespace daw::timesig
