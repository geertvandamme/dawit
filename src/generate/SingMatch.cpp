#include "SingMatch.h"
#include "SingScore.h"

#include <limits>

namespace daw::gen::singthis
{
int countSyllablesToMatch (const Lyrics& lyrics, const Melody& melody)
{
    return (int) findSyllables (lyrics, melody, Mode::onePerNote).size();
}

MatchResult matchNotesToSyllables (const Melody& melody, int syllables)
{
    MatchResult result;
    result.method = Method::mergeSplit;
    result.notesBefore = (int) melody.notes.size();
    result.syllables = syllables;

    auto notes = melody.notes;
    const auto length = [] (const MelodyNote& n) { return n.endBeat - n.startBeat; };

    // Too many notes: fold the shortest inner note into the one before it.
    while ((int) notes.size() > syllables)
    {
        int best = -1;

        for (int i = 1; i + 1 < (int) notes.size(); ++i)
        {
            const auto idx = (size_t) i;

            if (notes[idx].startSeconds - notes[idx - 1].endSeconds >= phraseRestSeconds
                || notes[idx + 1].startSeconds - notes[idx].endSeconds >= phraseRestSeconds)
                continue;   // it starts or ends a phrase

            if (best < 0 || length (notes[idx]) < length (notes[(size_t) best]) - 1.0e-9)
                best = i;
        }

        if (best < 0)
            break;

        auto& prev = notes[(size_t) best - 1];
        const auto cur = notes[(size_t) best];

        if (length (cur) > length (prev) + 1.0e-9)
        {
            prev.pitch = cur.pitch;
            prev.velocity = cur.velocity;
        }

        prev.endBeat = cur.endBeat;
        prev.endSeconds = cur.endSeconds;
        notes.erase (notes.begin() + best);
        ++result.merged;
    }

    // Too few notes: cut the longest in half.
    while ((int) notes.size() < syllables)
    {
        int best = -1;

        for (int i = 0; i < (int) notes.size(); ++i)
            if (length (notes[(size_t) i]) >= 2.0 * minSplitBeats - 1.0e-9
                && (best < 0 || length (notes[(size_t) i]) > length (notes[(size_t) best]) + 1.0e-9))
                best = i;

        if (best < 0)
            break;

        auto first = notes[(size_t) best];
        auto second = first;
        first.endBeat = second.startBeat = (first.startBeat + first.endBeat) * 0.5;
        first.endSeconds = second.startSeconds = (first.startSeconds + first.endSeconds) * 0.5;
        notes[(size_t) best] = first;
        notes.insert (notes.begin() + best + 1, second);
        ++result.split;
    }

    for (auto& n : notes)
        result.notes.push_back ({ n.pitch, n.startBeat, n.endBeat - n.startBeat, n.velocity });

    result.exact = (int) notes.size() == syllables;
    result.remaining = result.exact ? 0 : (int) notes.size() - syllables;
    return result;
}

MatchResult matchNotesToSyllableTimes (const Melody& melody, const std::vector<Syllable>& syllables, Method method)
{
    MatchResult result;
    result.method = method == Method::syllableTimesCapped ? Method::syllableTimesCapped : Method::syllableTimes;
    result.notesBefore = (int) melody.notes.size();
    result.syllables = (int) syllables.size();
    constexpr double eps = 1.0e-9;

    if ((int) melody.notes.size() == (int) syllables.size() || melody.notes.empty() || syllables.empty())
    {
        for (auto& n : melody.notes)
            result.notes.push_back ({ n.pitch, n.startBeat, n.endBeat - n.startBeat, n.velocity });

        result.exact = result.notesBefore == result.syllables;
        result.remaining = result.exact ? 0 : result.notesBefore - result.syllables;
        return result;
    }

    // Syllables in time order, as they are heard.
    std::vector<Syllable> sorted = syllables;
    std::stable_sort (sorted.begin(), sorted.end(), [] (const Syllable& a, const Syllable& b) { return a.beat < b.beat; });

    // The phrases of the melody: a rest of phraseRestSeconds or more ends one.
    struct Phrase { double from, to; };
    std::vector<Phrase> phrases;

    for (size_t k = 0; k < melody.notes.size(); ++k)
    {
        if (k == 0 || melody.notes[k].startSeconds - melody.notes[k - 1].endSeconds >= phraseRestSeconds)
            phrases.push_back ({ melody.notes[k].startBeat, melody.notes[k].endBeat });
        else
            phrases.back().to = melody.notes[k].endBeat;
    }

    for (size_t i = 0; i < sorted.size(); ++i)
    {
        const double beat = sorted[i].beat;
        int source = -1;      // the note sounding at the beat
        int earlier = -1;     // else the last note that ended before it
        int later = -1;       // else the first one that starts after it

        for (int k = 0; k < (int) melody.notes.size(); ++k)
        {
            const auto& m = melody.notes[(size_t) k];

            if (m.startBeat <= beat + eps && beat < m.endBeat - eps)
                source = k;
            else if (m.endBeat <= beat + eps)
                earlier = k;
            else if (later < 0)
                later = k;
        }

        double cap = 0.0;
        int pitchFrom = source;

        if (source >= 0)
            cap = melody.notes[(size_t) source].endBeat;
        else
        {
            pitchFrom = earlier >= 0 ? earlier : later;
            // In a rest: it lasts to where the rest ends, or to the syllable's own end after the last note.
            cap = later >= 0 ? melody.notes[(size_t) later].startBeat : juce::jmax (sorted[i].endBeat, beat + minNoteBeats);
        }

        double len = 0.0;

        if (method == Method::syllableTimesCapped)
        {
            double end = cap;

            if (i + 1 < sorted.size())
                end = juce::jmin (end, sorted[i + 1].beat);

            len = end - beat;
        }
        else
        {
            const bool hasNext = i + 1 < sorted.size();
            const double nextBeat = hasNext ? sorted[i + 1].beat : 0.0;
            double phraseEnd = 0.0, nextPhraseStart = std::numeric_limits<double>::max();
            bool inPhrase = false;

            for (auto& ph : phrases)
            {
                if (ph.from <= beat + eps && beat < ph.to - eps)
                {
                    inPhrase = true;
                    phraseEnd = ph.to;
                }
                else if (ph.from > beat + eps)
                {
                    nextPhraseStart = juce::jmin (nextPhraseStart, ph.from);
                }
            }

            const double limit = inPhrase ? phraseEnd : nextPhraseStart;

            if (hasNext && nextBeat < limit - eps)
            {
                len = juce::jmin (nextBeat - syllableGapBeats, limit) - beat;
            }
            else
            {
                // The last syllable of its phrase.
                const double remaining = source >= 0 ? melody.notes[(size_t) source].endBeat - beat : 0.0;
                double end = beat + juce::jmin (lastSyllableMaxBeats, juce::jmax (remaining, sorted[i].endBeat - beat));
                end = juce::jmin (end, nextPhraseStart);

                if (hasNext)
                    end = juce::jmin (end, nextBeat - syllableGapBeats);

                len = end - beat;
            }
        }

        if (len < minNoteBeats - eps)
        {
            len = minNoteBeats;
            ++result.tooShort;
        }

        const auto& from = melody.notes[(size_t) pitchFrom];
        result.notes.push_back ({ from.pitch, beat, len, from.velocity });
    }

    for (auto& n : result.notes)
    {
        bool onOldStart = false;

        for (auto& m : melody.notes)
            if (std::abs (m.startBeat - n.startBeat) <= movedBeats + eps)
                onOldStart = true;

        if (! onOldStart)
            ++result.moved;
    }

    result.exact = true;
    return result;
}

juce::String describeMatch (const MatchResult& r, bool done)
{
    const auto note = [] (int n) { return juce::String (n) + (n == 1 ? " note" : " notes"); };

    if (r.method != Method::mergeSplit)
    {
        juce::String text;
        const auto moves = r.moved == 0 ? juce::String ("move no note starts by more than a 16th")
                                        : "move " + juce::String (r.moved) + (r.moved == 1 ? " note start" : " note starts") + " by more than a 16th";

        if (done)
            text << "Put a note on each syllable: " << note (r.getNotesAfter()) << " now, " << r.syllables << " syllables. "
                 << (r.moved == 0 ? juce::String ("No") : juce::String (r.moved)) << (r.moved == 1 ? " note start" : " note starts")
                 << " moved by more than a 16th.";
        else
            text << note (r.notesBefore) << ", " << r.syllables << (r.syllables == 1 ? " syllable" : " syllables") << ". Matching would " << moves << ".";

        if (r.tooShort > 0)
            text << " " << r.tooShort << (r.tooShort == 1 ? " note is" : " notes are") << " held at a 32nd note, the shortest allowed, though "
                 << (r.method == Method::syllableTimesCapped ? "the next syllable or the end of its note" : "the next syllable") << " comes sooner.";

        return text;
    }

    juce::StringArray parts;

    if (r.merged > 0)
        parts.add ((done ? "merged " : "merge ") + note (r.merged) + " into the one before");

    if (r.split > 0)
        parts.add ((done ? "split " : "split ") + note (r.split) + " in two");

    juce::String text;

    if (done)
    {
        text = parts.isEmpty() ? juce::String ("Nothing changed") : parts.joinIntoString (" and ");
        text = text.substring (0, 1).toUpperCase() + text.substring (1);
        text << ": " << note (r.getNotesAfter()) << " now, " << r.syllables << " syllables.";
    }
    else
    {
        text << note (r.notesBefore) << ", " << r.syllables << (r.syllables == 1 ? " syllable" : " syllables") << ". Matching would "
             << (parts.isEmpty() ? juce::String ("change nothing") : parts.joinIntoString (" and ")) << ".";
    }

    if (! r.exact)
    {
        if (r.remaining > 0)
            text << " " << r.remaining << " too many notes " << (done ? "" : "would ") << "remain: the rest start or end a phrase, which are kept.";
        else
            text << " " << -r.remaining << (r.remaining == -1 ? " syllable " : " syllables ") << (done ? "" : "would ") << "still have no note: the notes left are too short to split.";
    }

    return text;
}
} // namespace daw::gen::singthis
