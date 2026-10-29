#include "SingAlong.h"

namespace daw::gen::singalong
{
namespace
{
    constexpr double eps = 1.0e-6;

    juce::String formatBeats (double beats)
    {
        const auto rounded = std::round (beats * 2.0) / 2.0;
        const auto text = std::abs (rounded - std::round (rounded)) < eps ? juce::String ((int) std::round (rounded)) : juce::String (rounded, 1);
        return text + (std::abs (rounded - 1.0) < eps ? " beat" : " beats");
    }

    juce::String formatSeconds (double seconds)
    {
        return juce::String (seconds, 1) + " seconds";
    }
}

Moment findMoment (const Lyrics& lyrics, double beat, const playalong::Loop& loop)
{
    Moment m;
    const auto& lines = lyrics.lines;
    const bool looping = loop.enabled && loop.endBeat > loop.startBeat && beat >= loop.startBeat && beat < loop.endBeat;

    // The lines that play, in order: in a loop, those that start in it.
    std::vector<int> order;
    for (int i = 0; i < (int) lines.size(); ++i)
        if (! looping || (lines[(size_t) i].startBeat >= loop.startBeat - eps && lines[(size_t) i].startBeat < loop.endBeat))
            order.push_back (i);

    if (order.empty())
        return m;

    const int count = (int) order.size();
    int at = -1;

    for (int k = 0; k < count && at < 0; ++k)
    {
        const auto& line = lines[(size_t) order[(size_t) k]];

        if (line.startBeat <= beat + eps && beat < line.endBeat)
        {
            at = k;
            m.singing = true;
        }
    }

    if (at < 0)
    {
        for (int k = 0; k < count && at < 0; ++k)
        {
            if (const auto start = lines[(size_t) order[(size_t) k]].startBeat; start > beat)
            {
                at = k;
                m.beatsToLine = start - beat;
            }
        }
    }

    if (at < 0)
    {
        // After the last line: the song's lyrics have ended, or the loop comes round to its first line.
        if (! looping)
        {
            m.previous = order.back();
            return m;
        }

        at = 0;
        m.beatsToLine = (loop.endBeat - beat) + (lines[(size_t) order.front()].startBeat - loop.startBeat);
    }

    m.line = order[(size_t) at];

    if (at > 0)
        m.previous = order[(size_t) at - 1];
    else if (looping && count > 1)
        m.previous = order.back();

    if (at + 1 < count)
        m.next = order[(size_t) at + 1];
    else if (looping && count > 1)
        m.next = order.front();

    if (m.singing)
    {
        const auto& line = lines[(size_t) m.line];

        for (size_t s = 0; s < line.syllables.size(); ++s)
            if (line.syllables[s].beat <= beat + eps && beat < line.getSyllableEnd (s))
                m.syllable = (int) s;
    }

    return m;
}

float getSung (const LyricLine& line, size_t syllable, double beat)
{
    if (syllable >= line.syllables.size())
        return 0.0f;

    const double start = line.syllables[syllable].beat;
    const double end = juce::jmin (line.getSyllableEnd (syllable), start + maxFillBeats);

    if (beat <= start)
        return 0.0f;

    if (beat >= end || end - start < eps)
        return 1.0f;

    return (float) ((beat - start) / (end - start));
}

int getCountIn (const Lyrics& lyrics, const Moment& m)
{
    if (m.singing || m.line < 0 || m.line >= (int) lyrics.lines.size() || m.beatsToLine > 4.0 + eps)
        return 0;

    // Only after a rest: a line that follows the one before closely needs no count.
    if (m.previous >= 0 && m.previous < m.line)
    {
        const double rest = lyrics.lines[(size_t) m.line].startBeat - lyrics.lines[(size_t) m.previous].endBeat;

        if (rest < restBeats - eps)
            return 0;
    }

    return juce::jlimit (1, 4, (int) std::ceil (m.beatsToLine - eps));
}

juce::String getText (const LyricLine& line)
{
    juce::String text;

    for (size_t i = 0; i < line.syllables.size(); ++i)
    {
        text << line.syllables[i].text;

        if (line.syllables[i].endsWord && i + 1 < line.syllables.size())
            text << " ";
    }

    return text;
}

//==============================================================================
Breath findShortestBreath (const Lyrics& lyrics)
{
    Breath shortest;

    for (size_t i = 1; i < lyrics.lines.size(); ++i)
    {
        const double rest = juce::jmax (0.0, lyrics.lines[i].startBeat - lyrics.lines[i - 1].endBeat);

        if (shortest.line < 0 || rest < shortest.beats - eps)
        {
            shortest.line = (int) i;
            shortest.beats = rest;
        }
    }

    return shortest;
}

std::pair<int, double> findLongestLine (const Lyrics& lyrics)
{
    std::pair<int, double> longest { -1, 0.0 };

    for (size_t i = 0; i < lyrics.lines.size(); ++i)
    {
        const double beats = lyrics.lines[i].endBeat - lyrics.lines[i].startBeat;

        if (longest.first < 0 || beats > longest.second + eps)
            longest = { (int) i, beats };
    }

    return longest;
}

juce::String explainSingingAlong (const Lyrics& lyrics, double bpm)
{
    juce::String text =
        "Sing along (the button in the Lyrics header, or Cmd+5) is for recording vocals. It shows the words large enough to read "
        "from the microphone while the song plays and records: the line to sing now, the next one under it, and the one before "
        "fading out above. No lyric sheet in your hands, and no paper rustling into the take. It's just as useful for learning "
        "a part before you record it.\n\n"
        "The colour runs over each syllable while it's sung, from where it starts to where the next one begins (in at most "
        + juce::String ((int) maxFillBeats) + " beats), so a held note fills slowly and a quick word fast. What's coloured is behind "
        "you; keep your eyes on what's still white.\n\n"
        "Tips from singing teachers:\n"
        "- Read ahead. Look at the next line while you finish this one, as you do when you read aloud.\n"
        "- Breathe in the rests before a line, not in the middle of a word. Before a line that comes after a rest the window counts "
        "down the last beats, 4, 3, 2, 1: breathe in on the count and come in when it runs out.\n"
        "- Many lines start just before the bar line, on an \"and\" (a pickup). Come in when the colour starts, not on the beat you "
        "expect.\n"
        "- Wear closed headphones when you record, so the band doesn't leak into the microphone.\n"
        "- Learn a song with the singer, then without. Remove vocals (karaoke version), in an audio clip's right-click menu, can keep the "
        "voice quietly at -12 dB as a guide; when you know the song, mute the Vocals track.\n"
        "- Sing in a key that suits you. A song that's too high or too low strains the voice; many men sing songs by women an "
        "octave lower, and the other way round.\n"
        "- Stand up, keep your shoulders loose and breathe low, into the belly, so the breath lasts the whole line.\n"
        "- Record several takes on new audio tracks and listen back: you hear the timing and the pitch far better afterwards than "
        "while singing, and you can keep the best one.\n\n";

    if (lyrics.isEmpty())
        return text + "Your song has no lyrics yet. Click Edit... in the Lyrics header and type them, or right-click a recording and "
                      "choose Detect lyrics (English only)..., then time them by dragging or tapping. Sing along then shows them as "
                      "the song plays.";

    const double secondsPerBeat = 60.0 / juce::jmax (1.0, bpm);
    const int count = (int) lyrics.lines.size();
    text << "Your song has " << count << (count == 1 ? " line" : " lines") << " to sing.";

    if (const auto [index, beats] = findLongestLine (lyrics); index >= 0)
        text << " The longest is \"" << getText (lyrics.lines[(size_t) index]) << "\": " << formatBeats (beats) << ", about "
             << formatSeconds (beats * secondsPerBeat) << " in one breath" << (beats * secondsPerBeat > 6.0 ? ", so breathe in deeply before it." : ".");

    if (const auto breath = findShortestBreath (lyrics); breath.line >= 0)
    {
        const auto& line = lyrics.lines[(size_t) breath.line];

        if (breath.beats < 0.5)
            text << " There's almost no rest before \"" << getText (line) << "\": take a quick, small breath at the end of the line before it.";
        else if (breath.beats < restBeats)
            text << " The shortest rest is " << formatBeats (breath.beats) << " (" << formatSeconds (breath.beats * secondsPerBeat)
                 << ") before \"" << getText (line) << "\": breathe in quickly there.";
        else
            text << " Every line has a rest of at least " << formatBeats (breath.beats) << " (" << formatSeconds (breath.beats * secondsPerBeat)
                 << ") before it, the shortest before \"" << getText (line) << "\": time for a full breath each time.";
    }

    return text;
}
} // namespace daw::gen::singalong
