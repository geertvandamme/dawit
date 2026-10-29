#include "SingPairing.h"

#include <algorithm>
#include <numeric>
#include <set>

namespace daw::gen::singthis
{
int getBar (double beat) { return (int) std::floor (beat / 4.0 + 1.0e-9) + 1; }

//==============================================================================
Melody buildMelody (std::vector<SourceNote> source, const TempoMap& tempo)
{
    Melody melody;
    constexpr double eps = 1.0e-9;

    source.erase (std::remove_if (source.begin(), source.end(), [] (const SourceNote& n) { return n.lengthBeats <= eps; }), source.end());

    std::vector<double> bounds;
    for (auto& n : source)
    {
        bounds.push_back (n.startBeat);
        bounds.push_back (n.startBeat + n.lengthBeats);
    }

    std::sort (bounds.begin(), bounds.end());
    bounds.erase (std::unique (bounds.begin(), bounds.end(), [] (double a, double b) { return b - a < eps; }), bounds.end());

    struct Piece { int note; double from, to; };
    std::vector<Piece> pieces;
    bool inOverlap = false;

    for (size_t b = 0; b + 1 < bounds.size(); ++b)
    {
        const double t0 = bounds[b], t1 = bounds[b + 1];
        int top = -1, active = 0;

        for (size_t i = 0; i < source.size(); ++i)
        {
            if (source[i].startBeat <= t0 + eps && source[i].startBeat + source[i].lengthBeats >= t1 - eps)
            {
                ++active;

                if (top < 0 || source[i].pitch > source[(size_t) top].pitch)
                    top = (int) i;
            }
        }

        if (active > 1 && ! inOverlap)
            ++melody.overlaps;

        inOverlap = active > 1;

        if (top < 0)
            continue;

        if (! pieces.empty() && pieces.back().note == top && std::abs (pieces.back().to - t0) < eps)
            pieces.back().to = t1;
        else
            pieces.push_back ({ top, t0, t1 });
    }

    for (auto& p : pieces)
    {
        MelodyNote n;
        n.pitch = source[(size_t) p.note].pitch;
        n.startBeat = p.from;
        n.endBeat = p.to;
        n.startSeconds = tempo.beatToSeconds (p.from);
        n.endSeconds = tempo.beatToSeconds (p.to);
        n.velocity = source[(size_t) p.note].velocity;
        n.continuation = source[(size_t) p.note].startBeat < p.from - eps;
        melody.notes.push_back (n);
    }

    return melody;
}

//==============================================================================
std::vector<Syllable> findSyllables (const Lyrics& lyrics, const Melody& melody, Mode mode)
{
    std::vector<Syllable> out;

    if (melody.notes.empty())
        return out;

    const double spanStart = melody.getStartBeat(), spanEnd = melody.getEndBeat();
    bool started = false;

    // The lines in the order they are heard. They are normally in time order already, but tapping the first word of a
    // line can leave it before or over its neighbour, and the words must not be dealt out in the order of the list then.
    std::vector<size_t> order (lyrics.lines.size());
    std::iota (order.begin(), order.end(), size_t (0));
    std::stable_sort (order.begin(), order.end(), [&] (size_t a, size_t b) { return lyrics.lines[a].startBeat < lyrics.lines[b].startBeat; });

    for (size_t o = 0; o < order.size(); ++o)
    {
        const size_t l = order[o];
        const auto& line = lyrics.lines[l];

        // A line that runs on over the next one stops where the next one starts, so its last word doesn't hold notes
        // that belong to the next line's words.
        double lineEnd = line.endBeat;

        if (o + 1 < order.size())
        {
            const double nextStart = lyrics.lines[order[o + 1]].startBeat;

            if (nextStart > line.startBeat + 1.0e-9)
                lineEnd = juce::jmin (lineEnd, nextStart);
        }

        if (mode == Mode::onePerNote && line.startBeat >= spanEnd - 1.0e-9)
            break;

        for (size_t s = 0; s < line.syllables.size(); ++s)
        {
            const auto& syl = line.syllables[s];
            const bool inside = syl.beat >= spanStart - pairingTolerance && syl.beat < spanEnd - 1.0e-9;

            if (mode == Mode::asTimed && ! inside)
                continue;

            if (mode == Mode::onePerNote)
            {
                started = started || syl.beat >= spanStart - pairingTolerance;

                if (! started)
                    continue;
            }

            Syllable out1;
            out1.text = syl.text;
            out1.beat = syl.beat;
            out1.endBeat = juce::jmin (line.getSyllableEnd (s), juce::jmax (lineEnd, syl.beat));
            out1.endsWord = syl.endsWord;
            out1.line = (int) l;
            out.push_back (out1);
        }
    }

    return out;
}

Mode chooseMode (const Lyrics& lyrics, const Melody& melody)
{
    const auto syllables = findSyllables (lyrics, melody, Mode::asTimed);

    if (syllables.empty())
        return Mode::asTimed;

    int onNote = 0;

    for (auto& s : syllables)
    {
        for (auto& n : melody.notes)
        {
            if (! n.continuation && std::abs (n.startBeat - s.beat) <= pairingTolerance + 1.0e-9)
            {
                ++onNote;
                break;
            }
        }
    }

    return (double) onNote >= asTimedShare * (double) syllables.size() - 1.0e-9 ? Mode::asTimed : Mode::onePerNote;
}

//==============================================================================
std::vector<SyllableSounds> splitSounds (const std::vector<pronunciation::Sound>& sounds, int numSyllables)
{
    std::vector<SyllableSounds> out ((size_t) juce::jmax (0, numSyllables));

    if (out.empty() || sounds.empty())
        return out;

    std::vector<int> vowels;
    for (size_t i = 0; i < sounds.size(); ++i)
        if (sounds[i].vowel)
            vowels.push_back ((int) i);

    if (vowels.empty())
    {
        out[0].nucleus.push_back (sounds.back());   // "hmm": the last sound carries the note
        out[0].onset.assign (sounds.begin(), sounds.end() - 1);
        return out;
    }

    const int V = (int) vowels.size(), S = numSyllables;
    std::vector<std::pair<int, int>> vowelRange;   // per syllable, which vowels (indices into `vowels`) it holds

    for (int k = 0; k < S; ++k)
    {
        int a = k * V / S, b = (k + 1) * V / S;
        a = juce::jmin (a, V - 1);
        vowelRange.emplace_back (a, juce::jmax (b, a + 1));
    }

    for (int k = 0; k < S; ++k)
    {
        auto& syl = out[(size_t) k];
        const bool reused = k > 0 && vowelRange[(size_t) k] == vowelRange[(size_t) k - 1];

        for (int v = vowelRange[(size_t) k].first; v < vowelRange[(size_t) k].second; ++v)
            syl.nucleus.push_back (sounds[(size_t) vowels[(size_t) v]]);

        if (reused)
            continue;   // more syllables than vowels: it holds the same vowel

        const int firstVowel = vowels[(size_t) vowelRange[(size_t) k].first];
        const int lastVowel = vowels[(size_t) vowelRange[(size_t) k].second - 1];

        // Consonants before: from the end of the syllable before (or the word's start) to the vowel.
        int from = 0;
        if (k > 0)
        {
            const int prevLast = vowels[(size_t) vowelRange[(size_t) k - 1].second - 1];
            const int between = firstVowel - prevLast - 1;
            from = prevLast + 1 + (between >= 2 ? 1 : 0);   // with two or more, the first closes the syllable before
        }

        for (int i = from; i < firstVowel; ++i)
            syl.onset.push_back (sounds[(size_t) i]);

        // Consonants after: to the next syllable's share, or the word's end.
        int to = (int) sounds.size();
        if (k + 1 < S && ! (vowelRange[(size_t) k + 1] == vowelRange[(size_t) k]))
        {
            const int nextFirst = vowels[(size_t) vowelRange[(size_t) k + 1].first];
            const int between = nextFirst - lastVowel - 1;
            to = lastVowel + 1 + (between >= 2 ? 1 : 0);
        }
        else if (k + 1 < S)
            to = (int) sounds.size();

        for (int i = lastVowel + 1; i < to; ++i)
            syl.coda.push_back (sounds[(size_t) i]);

        // Vowels inside a nucleus that are not at its ends need their consonants too; they stay in the nucleus run.
    }

    return out;
}

//==============================================================================
namespace
{
    juce::String joinSyllables (const std::vector<Syllable>& syllables, const std::vector<int>& indices)
    {
        juce::String out;

        for (size_t i = 0; i < indices.size(); ++i)
        {
            out << pronunciation::cleanWord (syllables[(size_t) indices[i]].text);

            if (i + 1 < indices.size())
                out << "-";
        }

        return out;
    }
}

Pairing pair (const Melody& melody, const Lyrics& lyrics, Mode mode, const sing::Voice* voice)
{
    Pairing p;
    p.mode = mode;
    p.syllables = findSyllables (lyrics, melody, mode);
    p.anyWords = ! findSyllables (lyrics, melody, Mode::onePerNote).empty();
    p.notes.assign (melody.notes.size(), {});

    // Words: runs of syllables joined by endsWord = false.
    p.wordOfSyllable.assign (p.syllables.size(), -1);

    for (size_t s = 0; s < p.syllables.size();)
    {
        Word w;
        size_t e = s;

        while (true)
        {
            w.syllables.push_back ((int) e);
            p.wordOfSyllable[e] = (int) p.words.size();

            if (p.syllables[e].endsWord || e + 1 >= p.syllables.size() || p.syllables[e + 1].line != p.syllables[e].line)
                break;

            ++e;
        }

        w.text = joinSyllables (p.syllables, w.syllables);
        juce::String joined;
        for (int i : w.syllables)
            joined << p.syllables[(size_t) i].text;

        w.sounds = pronunciation::pronounce (joined, voice);
        p.words.push_back (std::move (w));
        s = e + 1;
    }

    p.sounds.assign (p.syllables.size(), {});

    for (auto& w : p.words)
    {
        auto split = splitSounds (w.sounds.sounds, (int) w.syllables.size());

        for (size_t k = 0; k < w.syllables.size() && k < split.size(); ++k)
            p.sounds[(size_t) w.syllables[k]] = std::move (split[k]);
    }

    if (mode == Mode::asTimed)
    {
        std::vector<int> pairedNote (p.syllables.size(), -1);

        for (size_t s = 0; s < p.syllables.size(); ++s)
        {
            int best = -1;
            double bestDistance = pairingTolerance + 1.0e-9;

            for (size_t n = 0; n < melody.notes.size(); ++n)
            {
                if (melody.notes[n].continuation || p.notes[n].syllable >= 0)
                    continue;

                const double d = std::abs (melody.notes[n].startBeat - p.syllables[s].beat);

                if (d < bestDistance)
                {
                    bestDistance = d;
                    best = (int) n;
                }
            }

            if (best < 0)
            {
                p.unpairedSyllables.push_back ((int) s);
                continue;
            }

            p.notes[(size_t) best] = { (int) s, true };
            pairedNote[s] = best;
        }

        // The notes inside a syllable hold it.
        for (size_t s = 0; s < p.syllables.size(); ++s)
        {
            if (pairedNote[s] < 0)
                continue;

            for (size_t n = (size_t) pairedNote[s] + 1; n < melody.notes.size(); ++n)
            {
                if (p.notes[n].syllable >= 0 || melody.notes[n].startBeat >= p.syllables[s].endBeat - pairingTolerance)
                    break;

                p.notes[n] = { (int) s, false };
            }
        }
    }
    else
    {
        size_t next = 0;
        int numNotes = 0;

        for (size_t n = 0; n < melody.notes.size(); ++n)
        {
            if (melody.notes[n].continuation)
            {
                if (n > 0 && p.notes[n - 1].syllable >= 0)
                    p.notes[n] = { p.notes[n - 1].syllable, false };

                continue;
            }

            ++numNotes;

            if (next < p.syllables.size())
                p.notes[n] = { (int) next++, true };
        }

        p.leftOverSyllables = (int) (p.syllables.size() - next);
        p.firstLeftOverSyllable = (int) next;
        p.leftOverNotes = numNotes - (int) next;

        if (p.leftOverNotes < 0)
            p.leftOverNotes = 0;
    }

    for (size_t n = 0; n < p.notes.size(); ++n)
        if (p.notes[n].syllable < 0)
            p.notesWithoutWords.push_back ((int) n);

    return p;
}

bool canSing (const Pairing& p)
{
    for (auto& a : p.notes)
        if (a.starts)
            return true;

    return false;
}

//==============================================================================
juce::String getWordText (const Pairing&, const Word& w) { return w.text; }

namespace
{
    juce::String plural (int n, const juce::String& one, const juce::String& many)
    {
        return juce::String (n) + " " + (n == 1 ? one : many);
    }

    juce::String barsText (int first, int last)
    {
        return first == last ? "bar " + juce::String (first) : "bars " + juce::String (first) + juce::String::fromUTF8 ("\xe2\x80\x93") + juce::String (last);
    }

    juce::String quoted (const juce::String& s)
    {
        return "\"" + s + "\"";
    }

    /** The syllables' text: joined with hyphens inside a word and spaces between words. */
    juce::String joinText (const Pairing& p, int from, int to)
    {
        juce::String out;

        for (int i = from; i < to; ++i)
        {
            out << pronunciation::cleanWord (p.syllables[(size_t) i].text);

            if (i + 1 < to)
                out << (p.syllables[(size_t) i].endsWord ? " " : "-");
        }

        return out;
    }
}

std::vector<Advice> getAdvice (const Melody& melody, const Pairing& p)
{
    std::vector<Advice> advice;

    if (melody.notes.empty())
    {
        advice.push_back ({ Rule::noWords, "There are no notes to sing. Pick a MIDI clip with a melody.", true });
        return advice;
    }

    const int firstBar = getBar (melody.getStartBeat()), lastBar = getBar (melody.getEndBeat() - 1.0e-6);

    if (! p.anyWords)
    {
        advice.push_back ({ Rule::noWords, "There are no words on the Lyrics row over " + barsText (firstBar, lastBar)
                                              + ". Type them with Edit... in the Lyrics header first.", true });
        return advice;
    }

    std::vector<Advice> problems;
    const juce::String dash = juce::String::fromUTF8 ("\xe2\x80\x93");

    // 2. Notes without words (As timed).
    if (p.mode == Mode::asTimed && ! p.notesWithoutWords.empty())
    {
        const int n = (int) p.notesWithoutWords.size();
        const int a = getBar (melody.notes[(size_t) p.notesWithoutWords.front()].startBeat);
        const int b = getBar (melody.notes[(size_t) p.notesWithoutWords.back()].startBeat);
        problems.push_back ({ Rule::notesWithoutWords,
                              plural (n, "note", "notes") + " in " + barsText (a, b) + (n == 1 ? " has no words, so it's left out" : " have no words, so they're left out")
                                  + ". Add words there, or try One syllable per note.", false });
    }

    // 3. Syllables without a note (As timed).
    if (p.mode == Mode::asTimed && ! p.unpairedSyllables.empty())
    {
        const int n = (int) p.unpairedSyllables.size();
        const auto& first = p.syllables[(size_t) p.unpairedSyllables.front()];
        const auto name = quoted (pronunciation::cleanWord (first.text));

        if (n == 1)
            problems.push_back ({ Rule::syllablesWithoutNote, name + " in bar " + juce::String (getBar (first.beat))
                                      + " starts between notes, so it isn't sung. Drag it onto a note on the Lyrics row, or try One syllable per note.", false });
        else
            problems.push_back ({ Rule::syllablesWithoutNote, name + " in bar " + juce::String (getBar (first.beat)) + " and " + juce::String (n - 1)
                                      + (n == 2 ? " other syllable start" : " other syllables start") + " between notes, so they aren't sung. "
                                      "Drag them onto notes on the Lyrics row, or try One syllable per note.", false });
    }

    // 4. Counts differ (One syllable per note).
    if (p.mode == Mode::onePerNote && (p.leftOverSyllables > 0 || p.leftOverNotes > 0))
    {
        int numNotes = 0;
        for (auto& n : melody.notes)
            if (! n.continuation)
                ++numNotes;

        const juce::String counts = plural (numNotes, "note", "notes") + " and " + plural ((int) p.syllables.size(), "syllable", "syllables") + ": ";

        if (p.leftOverSyllables > 0)
        {
            const int n = p.leftOverSyllables;
            const auto text = joinText (p, p.firstLeftOverSyllable, (int) p.syllables.size());
            problems.push_back ({ Rule::countsDiffer, counts + (n == 1 ? "the last syllable (" : "the last " + juce::String (n) + " syllables (")
                                      + quoted (text) + (n == 1 ? ") isn't sung." : ") aren't sung."), false });
        }
        else
        {
            const int n = p.leftOverNotes;
            problems.push_back ({ Rule::countsDiffer, counts + (n == 1 ? "the last note isn't sung. " : "the last " + juce::String (n) + " notes aren't sung. ")
                                      + "To hold a syllable over several notes, time the words and use As timed.", false });
        }
    }

    // 5. Guessed words.
    {
        juce::StringArray guessed;
        std::set<juce::String> seen;

        for (auto& w : p.words)
            if (w.sounds.source == pronunciation::Source::guessed && ! w.sounds.sounds.empty() && seen.insert (w.text).second)
                guessed.add (quoted (w.text));

        if (! guessed.isEmpty())
        {
            const bool one = guessed.size() == 1;
            problems.push_back ({ Rule::guessedWords, guessed.joinIntoString (", ") + (one ? " isn't" : " aren't") + " in the dictionary, so "
                                      + (one ? "its sounds were" : "their sounds were") + " guessed from the spelling. Listen to "
                                      + (one ? "it" : "them") + ", or spell " + (one ? "it" : "them") + " the way " + (one ? "it sounds." : "they sound."), false });
        }
    }

    // 6. Overlapping notes.
    if (melody.overlaps > 0)
        problems.push_back ({ Rule::overlap, "Notes overlap in " + plural (melody.overlaps, "place", "places")
                                  + ". A voice sings one note at a time, so the top one is sung.", false });

    // 7. Out of range.
    {
        int high = 0, low = 0;

        for (auto& n : melody.notes)
        {
            high += n.pitch > highestMidi ? 1 : 0;
            low += n.pitch < lowestMidi ? 1 : 0;
        }

        juce::String text;

        if (high > 0)
            text << plural (high, "note goes", "notes go") << " above C6, higher than nearly every singer. Try " << (high == 1 ? "it" : "them") << " an octave lower.";

        if (low > 0)
            text << (text.isEmpty() ? "" : " ") << plural (low, "note goes", "notes go") << " below E2, lower than nearly every singer. Try "
                 << (low == 1 ? "it" : "them") << " an octave higher.";

        if (text.isNotEmpty())
            problems.push_back ({ Rule::outOfRange, text, false });
    }

    // 8. Crowded notes.
    {
        std::vector<std::pair<juce::String, size_t>> crowded;

        for (size_t n = 0; n < melody.notes.size(); ++n)
        {
            const auto& a = p.notes[n];

            if (! a.starts)
                continue;

            const auto& sounds = p.sounds[(size_t) a.syllable];
            const int consonants = (int) (sounds.onset.size() + sounds.coda.size());

            if (melody.notes[n].endSeconds - melody.notes[n].startSeconds < crowdedSeconds && consonants >= crowdedConsonants)
                crowded.emplace_back (pronunciation::cleanWord (p.syllables[(size_t) a.syllable].text), n);
        }

        if (! crowded.empty())
        {
            const auto& [word, n] = crowded.front();
            const double seconds = melody.notes[n].endSeconds - melody.notes[n].startSeconds;
            juce::String text = quoted (word) + " in bar " + juce::String (getBar (melody.notes[n].startBeat)) + " has many consonants on a "
                                + juce::String (seconds, 2) + " s note and may sound rushed.";

            if (crowded.size() > 1)
                text << " " << juce::String ((int) crowded.size() - 1) << (crowded.size() == 2 ? " more note is" : " more notes are") << " like it.";

            text << " Give it a longer note or a shorter word.";
            problems.push_back ({ Rule::crowded, text, false });
        }
    }

    // 9. Melisma (for information).
    {
        std::vector<int> held (p.syllables.size(), 0);

        for (auto& a : p.notes)
            if (a.syllable >= 0 && ! a.starts)
                ++held[(size_t) a.syllable];

        juce::StringArray lines;
        int more = 0;

        for (size_t s = 0; s < held.size(); ++s)
        {
            if (held[s] == 0)
                continue;

            if (lines.size() < 2)
                lines.add (quoted (pronunciation::cleanWord (p.syllables[s].text)) + " is held over " + juce::String (held[s] + 1)
                           + " notes: the vowel slides from note to note.");
            else
                ++more;
        }

        if (! lines.isEmpty())
        {
            auto text = lines.joinIntoString (" ");

            if (more > 0)
                text << " " << juce::String (more) << (more == 1 ? " more syllable is held too." : " more syllables are held too.");

            advice.push_back ({ Rule::melisma, text, false });
        }
    }

    if (problems.empty())
        advice.insert (advice.begin(), { Rule::allGood, "Every note has a syllable and every syllable a note: ready to sing.", false });

    for (auto& pr : problems)
        advice.push_back (pr);

    return advice;
}

juce::String describeNote (const Melody& melody, const Pairing& p, size_t i)
{
    const auto& note = melody.notes[i];
    const auto name = juce::MidiMessage::getMidiNoteName (note.pitch, true, true, 4);
    const auto where = "Bar " + juce::String (getBar (note.startBeat)) + juce::String::fromUTF8 (" \xc2\xb7 ") + name + juce::String::fromUTF8 (" \xc2\xb7 ");
    const auto& a = p.notes[i];

    if (a.syllable < 0)
        return where + "no words: left out";

    const auto text = quoted (pronunciation::cleanWord (p.syllables[(size_t) a.syllable].text));

    if (! a.starts)
        return where + text + " held";

    juce::String sounds;
    const auto& s = p.sounds[(size_t) a.syllable];

    for (auto* group : { &s.onset, &s.nucleus, &s.coda })
        for (auto& sound : *group)
            sounds << (sounds.isEmpty() ? "" : " ") << sound.symbol.fromLastOccurrenceOf ("/", false, false);

    return where + text + " (" + sounds + ")";
}
} // namespace daw::gen::singthis
