#include "BuiltInArpeggiator.h"
#include "BuiltInFormat.h"
#include "Chord.h"

#include <bit>
#include <cstring>
#include <numeric>

namespace daw
{
namespace arp
{
    const char* getOrderName (Order order)
    {
        switch (order)
        {
            case Order::up:        return "Up";
            case Order::down:      return "Down";
            case Order::upDown:    return "Up and down";
            case Order::downUp:    return "Down and up";
            case Order::outsideIn: return "Outside in";
            case Order::asPlayed:  return "As played";
            case Order::alberti:   return "Alberti";
            case Order::random:    return "Random";
        }

        return "";
    }

    const char* getOrderHint (Order order)
    {
        switch (order)
        {
            case Order::up:        return "Lowest to highest";
            case Order::down:      return "Highest to lowest";
            case Order::upDown:    return "Up, then back, without repeating the ends";
            case Order::downUp:    return "Down, then back up";
            case Order::outsideIn: return "Lowest, highest, then inwards";
            case Order::asPlayed:  return "The order you pressed the keys";
            case Order::alberti:   return "Low, high, middle, high";
            case Order::random:    return "Any held note, the same each time the bar plays";
        }

        return "";
    }

    const char* getKindName (StepKind kind)
    {
        switch (kind)
        {
            case StepKind::note:  return "Note";
            case StepKind::chord: return "Chord";
            case StepKind::bass:  return "Bass";
            case StepKind::rest:  return "Rest";
            case StepKind::tie:   return "Tie";
        }

        return "";
    }

    const char* getKindHint (StepKind kind)
    {
        switch (kind)
        {
            case StepKind::note:  return "The next note of the order";
            case StepKind::chord: return "The whole chord at once";
            case StepKind::bass:  return "The lowest held note, an octave lower";
            case StepKind::rest:  return "Nothing: a gap";
            case StepKind::tie:   return "The step before goes on sounding";
        }

        return "";
    }

    char getKindLetter (StepKind kind)
    {
        return "NCBRT"[juce::jlimit (0, numStepKinds - 1, (int) kind)];
    }

    //==============================================================================
    void buildSequence (const std::vector<HeldNote>& heldNotes, Order order, int octaves, std::vector<SeqNote>& out)
    {
        out.clear();
        octaves = juce::jlimit (1, maxOctaves, octaves);

        std::array<HeldNote, (size_t) maxHeld> base {};
        const int m = juce::jmin ((int) heldNotes.size(), maxHeld);

        if (m == 0)
            return;

        for (int i = 0; i < m; ++i)
            base[(size_t) i] = heldNotes[(size_t) i];

        if (order == Order::asPlayed)
            std::sort (base.begin(), base.begin() + m, [] (const HeldNote& a, const HeldNote& b) { return a.order != b.order ? a.order < b.order : a.pitch < b.pitch; });
        else
            std::sort (base.begin(), base.begin() + m, [] (const HeldNote& a, const HeldNote& b) { return a.pitch < b.pitch; });

        auto add = [&out] (const HeldNote& h, int octave)
        {
            if (h.pitch + 12 * octave <= 127)
                out.push_back ({ h.pitch + 12 * octave, h.velocity });
        };

        if (order == Order::alberti)
        {
            for (int o = 0; o < octaves; ++o)
            {
                if (m == 1)
                {
                    add (base[0], o);
                    continue;
                }

                add (base[0], o);
                add (base[(size_t) m - 1], o);
                add (base[(size_t) ((m - 1) / 2)], o);
                add (base[(size_t) m - 1], o);
            }

            return;
        }

        for (int o = 0; o < octaves; ++o)
            for (int i = 0; i < m; ++i)
                add (base[(size_t) i], o);

        const int n = (int) out.size();

        switch (order)
        {
            case Order::up:
            case Order::asPlayed:
            case Order::random:
            case Order::alberti:
                break;

            case Order::down:
                std::reverse (out.begin(), out.end());
                break;

            case Order::upDown:
            case Order::downUp:
                if (order == Order::downUp)
                    std::reverse (out.begin(), out.end());

                // Back without repeating the ends: n notes up, n - 2 notes back.
                for (int i = n - 2; i >= 1; --i)
                    out.push_back (out[(size_t) i]);

                break;

            case Order::outsideIn:
            {
                std::array<SeqNote, (size_t) maxSequence> copy {};
                const int count = juce::jmin (n, maxSequence);
                std::copy (out.begin(), out.begin() + count, copy.begin());
                out.clear();
                int lo = 0, hi = count - 1;
                bool low = true;

                while (lo <= hi)
                {
                    out.push_back (copy[(size_t) (low ? lo++ : hi--)]);
                    low = ! low;
                }

                break;
            }
        }
    }

    std::vector<SeqNote> getSequence (const std::vector<HeldNote>& held, Order order, int octaves)
    {
        std::vector<SeqNote> out;
        buildSequence (held, order, octaves, out);
        return out;
    }

    int getCycleLength (int numNotes, Order order, int octaves)
    {
        std::vector<HeldNote> notes;

        for (int i = 0; i < juce::jlimit (1, maxHeld, numNotes); ++i)
            notes.push_back ({ 24 + 2 * i, 100, i, true });

        return (int) getSequence (notes, order, octaves).size();
    }

    int getRandomIndex (long long songStep, int count)
    {
        if (count <= 1)
            return 0;

        auto x = (unsigned long long) songStep * 0x9E3779B97F4A7C15ULL;
        x ^= x >> 29;
        x *= 0xBF58476D1CE4E5B9ULL;
        x ^= x >> 32;
        return (int) (x % (unsigned long long) count);
    }

    double getStepStartBeats (long long k, int rate, float swingPercent)
    {
        const double len = stepgrid::getStepBeats (rate);
        const double swing = juce::jlimit ((double) minSwing, (double) maxSwing, (double) swingPercent);
        return (double) k * len + ((k & 1) != 0 ? (swing - 50.0) / 50.0 * len : 0.0);
    }

    int getTiedSteps (const Settings& s, int index)
    {
        const int n = juce::jlimit (1, maxSteps, s.steps);
        int count = 1;

        for (int j = 1; j < n; ++j)
        {
            if (s.kinds[(size_t) ((index + j) % n)] != StepKind::tie)
                break;

            ++count;
        }

        return count;
    }

    double getNoteEndBeats (long long k, int tiedSteps, int rate, float swingPercent, float gatePercent)
    {
        const long long last = k + juce::jmax (1, tiedSteps) - 1;
        const double start = getStepStartBeats (last, rate, swingPercent);
        const double next = getStepStartBeats (last + 1, rate, swingPercent);
        return start + (double) juce::jlimit (minGate, maxGate, gatePercent) / 100.0 * (next - start);
    }

    int getAccentedVelocity (int velocity, int accent)
    {
        return juce::jlimit (1, 127, velocity + accent);
    }

    int getBassPitch (int lowestPitch)
    {
        return lowestPitch >= 12 ? lowestPitch - 12 : lowestPitch;
    }

    double getNotesPerSecond (const Settings& s, double bpm)
    {
        const int n = juce::jlimit (1, maxSteps, s.steps);
        int sounding = 0;

        for (int i = 0; i < n; ++i)
            sounding += (s.kinds[(size_t) i] == StepKind::note || s.kinds[(size_t) i] == StepKind::chord) ? 1 : 0;

        return bpm / 60.0 / stepgrid::getStepBeats (s.rate) * (double) sounding / (double) n;
    }

    int getCrossingPatterns (int cycle, int noteSteps)
    {
        if (cycle < 1 || noteSteps < 1)
            return 1;

        return cycle / std::gcd (cycle, noteSteps);
    }

    int getBarRealignPatterns (int steps, int rate)
    {
        // Counted in 24ths of a beat, which every step length is a whole number of.
        const long long unit = std::llround (stepgrid::getStepBeats (rate) * 24.0);
        const long long pattern = juce::jmax (1, steps) * juce::jmax ((long long) 1, unit);
        return (int) (std::lcm (pattern, 96LL) / pattern);
    }

    juce::String nameChord (int pitchClasses, int lowestPitchClass)
    {
        pitchClasses &= 0xfff;

        if (pitchClasses == 0)
            return {};

        std::vector<int> roots;

        if (lowestPitchClass >= 0 && ((pitchClasses >> (lowestPitchClass % 12)) & 1) != 0)
            roots.push_back (lowestPitchClass % 12);

        for (int pc = 0; pc < 12; ++pc)
            if (((pitchClasses >> pc) & 1) != 0 && std::find (roots.begin(), roots.end(), pc) == roots.end())
                roots.push_back (pc);

        for (int root : roots)
        {
            int relative = 0;

            for (int pc = 0; pc < 12; ++pc)
                if (((pitchClasses >> pc) & 1) != 0)
                    relative |= 1 << ((pc - root + 12) % 12);

            for (auto& quality : Chord::getQualities())
            {
                int bits = 0;

                for (int i : quality.intervals)
                    bits |= 1 << (i % 12);

                if (bits == relative)
                    return Chord::getName (root, quality.intervals);
            }
        }

        juce::StringArray names;

        for (int pc = 0; pc < 12; ++pc)
            if (((pitchClasses >> pc) & 1) != 0)
                names.add (MusicalKey::getTonicName (pc));

        return names.joinIntoString (" ");
    }

    //==============================================================================
    juce::String patternToText (const Settings& s)
    {
        juce::String letters;

        for (int i = 0; i < maxSteps; ++i)
        {
            const char letter = getKindLetter (s.kinds[(size_t) i]);
            letters << (char) (s.accents[(size_t) i] ? std::tolower (letter) : letter);
        }

        return "DawIt Arpeggiator pattern: " + juce::String (stepgrid::stepLengths[juce::jlimit (0, stepgrid::numStepLengths - 1, s.rate)].name) + " "
               + juce::String (s.steps) + " " + letters;
    }

    bool patternFromText (const juce::String& text, Settings& s)
    {
        const auto tokens = juce::StringArray::fromTokens (text.trim(), " \t\r\n", "");

        if (tokens.size() != 6 || ! tokens[0].equalsIgnoreCase ("DawIt") || ! tokens[1].equalsIgnoreCase ("Arpeggiator") || ! tokens[2].equalsIgnoreCase ("pattern:"))
            return false;

        int rateIndex = -1;

        for (int i = 0; i < stepgrid::numStepLengths; ++i)
            if (tokens[3].equalsIgnoreCase (stepgrid::stepLengths[i].name))
                rateIndex = i;

        const int steps = tokens[4].getIntValue();
        const auto& letters = tokens[5];

        if (rateIndex < 0 || ! tokens[4].containsOnly ("0123456789") || steps < 1 || steps > maxSteps || letters.length() < 1 || letters.length() > maxSteps)
            return false;

        std::array<StepKind, maxSteps> kinds {};
        std::array<bool, maxSteps> accents {};

        for (int i = 0; i < letters.length(); ++i)
        {
            const auto letter = letters[i];
            const int kind = juce::String ("NCBRT").indexOfChar (juce::CharacterFunctions::toUpperCase (letter));

            if (kind < 0)
                return false;

            kinds[(size_t) i] = (StepKind) kind;
            accents[(size_t) i] = juce::CharacterFunctions::isLowerCase (letter);
        }

        s.rate = rateIndex;
        s.steps = steps;
        s.kinds = kinds;
        s.accents = accents;
        return true;
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            // `letters` holds one letter per step (N, C, B, R, T), `accented` the steps (from 1) that are accented.
            auto add = [&list] (juce::String name, juce::String why, Order order, int octaves, int rate, float gate, float swing, const char* letters,
                                std::initializer_list<int> accented = {})
            {
                Settings s;
                s.order = order;
                s.octaves = octaves;
                s.rate = rate;
                s.gatePercent = gate;
                s.swingPercent = swing;
                s.steps = (int) std::strlen (letters);

                for (int i = 0; i < s.steps; ++i)
                    s.kinds[(size_t) i] = (StepKind) juce::String ("NCBRT").indexOfChar (letters[i]);

                for (int step : accented)
                    s.accents[(size_t) step - 1] = true;

                list.push_back ({ std::move (name), std::move (why), s });
            };

            constexpr int r32 = 0, r16t = 1, r16 = 2, r8t = 3, r8 = 4, r4 = 5;
            (void) r32;
            (void) r16t;

            add ("Up", "The plain arpeggio: each note of the chord in turn, from the bottom up, four a beat. Start here and change one thing at a time.",
                 Order::up, 1, r16, 80.0f, 50.0f, "NNNNNNNNNNNNNNNN");
            add ("Trance sixteenths", "Short sixteenths climbing two octaves, louder on each beat: the driving trance arpeggio. The 50% gate keeps the fast notes apart.",
                 Order::up, 2, r16, 50.0f, 50.0f, "NNNNNNNNNNNNNNNN", { 1, 5, 9, 13 });
            add ("Up and down", "Up two octaves and back without repeating the ends: a rolling line that circles round the chord.",
                 Order::upDown, 2, r16, 80.0f, 50.0f, "NNNNNNNNNNNNNNNN");
            add ("Falling eighths", "Down the chord in eighths, slightly loosened with 54% swing: relaxed, for synthwave and lo-fi.",
                 Order::down, 1, r8, 90.0f, 54.0f, "NNNNNNNN");
            add ("Alberti bass", "Low, high, middle, high, as in the left hand of Mozart's Sonata K. 545. Hold a chord in the lower middle of the keyboard.",
                 Order::alberti, 1, r8, 100.0f, 50.0f, "NNNNNNNN");
            add ("Ballad: bass and broken chord", "The bass note, then the chord rising in eighths, each note ringing into the next as with the sustain pedal.",
                 Order::up, 2, r8, 150.0f, 50.0f, "BNNNNNNN");
            add ("Chord on every beat", "The chord trigger: the whole chord on each beat, the first one strongest. A pulse for verses and sketches.",
                 Order::up, 1, r4, 90.0f, 50.0f, "CCCC", { 1 });
            add ("Off-beat stabs", "Short chords between the beats, as in ska, reggae and house: the off-beats carry the weight.",
                 Order::up, 1, r8, 40.0f, 50.0f, "RCRCRCRC");
            add ("3-3-2 stabs", "Three hits a bar on 1, the and of 2, and 4: the syncopated 3-3-2 of pop and EDM drops.",
                 Order::up, 1, r8, 60.0f, 50.0f, "CRRCRRCR", { 1 });
            add ("Stride: bass and chord", "Bass on 1 and 3, the chord on 2 and 4: the oom-pah of stride piano and marches.",
                 Order::up, 1, r4, 70.0f, 50.0f, "BCBC");
            add ("Hit and ripple", "The whole chord, held for a moment, then its notes ripple upwards: chords and single notes in one pattern.",
                 Order::up, 2, r16, 80.0f, 50.0f, "CTNNNNNN", { 1 });
            add ("Triplet waves", "Three notes a beat, up and down two octaves: rolls against straight drums, for film and ballads.",
                 Order::upDown, 2, r8t, 80.0f, 50.0f, "NNNNNNNNNNNN");
            add ("Octave bounce", "Hold a single note: it jumps between it and its octave, the disco and synth-pop bass trick.",
                 Order::up, 2, r8, 60.0f, 50.0f, "NNNNNNNN", { 1, 3, 5, 7 });
            add ("Random sparkle", "Short notes picked by chance from the chord over two octaves: always chord tones, and the same each time the bar plays.",
                 Order::random, 2, r16, 40.0f, 50.0f, "NNNNNNNNNNNNNNNN");
            add ("Outside in", "Lowest, highest, then inwards: a figure that circles the chord instead of climbing it.",
                 Order::outsideIn, 1, r16, 70.0f, 50.0f, "NNNNNNNNNNNNNNNN");

            return list;
        }();

        return presets;
    }

    //==============================================================================
    int StepRecord::getNumHeld() const noexcept
    {
        return std::popcount (heldMask[0]) + std::popcount (heldMask[1]);
    }

    int StepRecord::getLowestHeld() const noexcept
    {
        for (int p = 0; p < 128; ++p)
            if (isHeld (p))
                return p;

        return -1;
    }

    void StepHistory::push (const StepRecord& r) noexcept
    {
        const auto n = written.load (std::memory_order_relaxed);
        auto& w = slots[(size_t) (n % (unsigned long long) size)].words;
        w[0] = (unsigned long long) (r.step & 0xff) | ((unsigned long long) (int) r.kind << 8) | ((unsigned long long) (r.numPlayed & 0xff) << 16)
               | ((unsigned long long) (r.chordPitchClasses & 0xfff) << 24);
        unsigned long long beatBits = 0;
        std::memcpy (&beatBits, &r.beat, sizeof (beatBits));
        w[1] = beatBits;
        w[2] = r.heldMask[0];
        w[3] = r.heldMask[1];
        unsigned long long lo = 0, hi = 0;

        for (int i = 0; i < 8; ++i)
        {
            lo |= (unsigned long long) r.played[(size_t) i] << (8 * i);
            hi |= (unsigned long long) r.played[(size_t) (i + 8)] << (8 * i);
        }

        w[4] = lo;
        w[5] = hi;
        written.store (n + 1, std::memory_order_release);
    }

    int StepHistory::getRecent (StepRecord* dest, int max) const
    {
        const auto end = written.load (std::memory_order_acquire);
        const auto floor = cleared.load();
        const auto available = end > floor ? end - floor : 0ULL;
        const auto want = (unsigned long long) juce::jmax (0, juce::jmin (max, size - 8));
        const auto count = juce::jmin (available, want);
        const auto first = end - count;

        for (unsigned long long i = 0; i < count; ++i)
        {
            const auto& w = slots[(size_t) ((first + i) % (unsigned long long) size)].words;
            auto& r = dest[i];
            const auto w0 = w[0].load();
            r.step = (int) (w0 & 0xff);
            r.kind = (StepKind) juce::jlimit (0, numStepKinds - 1, (int) ((w0 >> 8) & 0xff));
            r.numPlayed = juce::jmin ((int) ((w0 >> 16) & 0xff), maxHeld);
            r.chordPitchClasses = (int) ((w0 >> 24) & 0xfff);
            const auto beatBits = w[1].load();
            std::memcpy (&r.beat, &beatBits, sizeof (r.beat));
            r.heldMask = { w[2].load(), w[3].load() };
            const auto lo = w[4].load(), hi = w[5].load();

            for (int j = 0; j < 8; ++j)
            {
                r.played[(size_t) j] = (std::uint8_t) ((lo >> (8 * j)) & 0xff);
                r.played[(size_t) (j + 8)] = (std::uint8_t) ((hi >> (8 * j)) & 0xff);
            }
        }

        // If the writer lapped the reader while it copied, the oldest copies may be mixed: drop them.
        const auto after = written.load (std::memory_order_acquire);
        const auto lost = after - first > (unsigned long long) size ? after - first - (unsigned long long) size : 0ULL;
        const auto kept = lost >= count ? 0ULL : count - lost;

        if (kept < count)
            std::copy (dest + lost, dest + count, dest);

        return (int) kept;
    }

    //==============================================================================
    namespace
    {
        juce::String formatBpm (double bpm)
        {
            const bool whole = std::abs (bpm - std::round (bpm)) < 0.05;
            return whole ? juce::String (juce::roundToInt (bpm)) : juce::String (bpm, 1);
        }

        juce::String formatBars (double bars)
        {
            const bool whole = std::abs (bars - std::round (bars)) < 0.005;
            auto number = whole ? juce::String (juce::roundToInt (bars)) : juce::String (bars, 2);

            if (! whole && number.endsWithChar ('0'))
                number = number.dropLastCharacters (1);

            return number + (number == "1" ? " bar" : " bars");
        }

        juce::String getNoteName (int pitch)
        {
            return MusicalKey::getTonicName (pitch % 12) + juce::String (pitch / 12 - 1);
        }
    }

    std::vector<juce::String> analyse (const AnalysisInput& in)
    {
        const auto& s = in.settings;
        std::vector<juce::String> lines;
        int playedCount = 0;

        for (auto& r : in.records)
            playedCount += r.numPlayed;

        // 1: nothing reached it.
        if (playedCount == 0)
            return { "Nothing reached the arpeggiator in the last 2 bars. Hold a chord on your keyboard, or put a MIDI clip of chords on this track, and play the song." };

        // 2: the notes against the chord track's chords.
        {
            int total = 0, outside = 0;
            const StepRecord* firstMiss = nullptr;

            for (auto& r : in.records)
            {
                if (r.chordPitchClasses == 0)
                    continue;

                for (int i = 0; i < r.numPlayed; ++i)
                {
                    ++total;

                    if (((r.chordPitchClasses >> (r.played[(size_t) i] % 12)) & 1) == 0)
                    {
                        ++outside;

                        if (firstMiss == nullptr)
                            firstMiss = &r;
                    }
                }
            }

            if (total >= 4)
            {
                if (outside == 0)
                {
                    lines.push_back ("Every note it played is in the chord track's chord of its bar, so the arpeggio follows the harmony.");
                }
                else if ((double) outside / (double) total > 0.25)
                {
                    int held = 0;

                    for (int p = 0; p < 128; ++p)
                        if (firstMiss->isHeld (p))
                            held |= 1 << (p % 12);

                    const int low = juce::jmax (0, firstMiss->getLowestHeld());
                    lines.push_back (juce::String (outside) + " of the " + juce::String (total) + " notes it played aren't in the chord track's chord (bar "
                                     + juce::String ((int) std::floor (firstMiss->beat / 4.0) + 1) + ": you hold " + nameChord (held, low % 12) + ", the chord track has "
                                     + nameChord (firstMiss->chordPitchClasses, -1) + "). Notes outside the chord can add colour or clash. If it sounds muddy, hold the chord track's chord, or change the chord track.");
                }
                else
                {
                    lines.push_back ("Most notes it played are in the chord track's chords; the " + juce::String (outside) + " that aren't are passing colour.");
                }
            }
        }

        // 3: the order's cycle against the pattern.
        if (s.order != Order::random)
        {
            const int steps = juce::jlimit (1, maxSteps, s.steps);
            int noteSteps = 0, held = 0;

            for (int i = 0; i < steps; ++i)
                noteSteps += s.kinds[(size_t) i] == StepKind::note ? 1 : 0;

            for (auto it = in.records.rbegin(); it != in.records.rend(); ++it)
                if (it->numPlayed > 0)
                {
                    held = it->getNumHeld();
                    break;
                }

            const int cycle = getCycleLength (held, s.order, s.octaves);

            if (noteSteps > 0 && cycle > 1 && noteSteps % cycle != 0)
            {
                const int patterns = getCrossingPatterns (cycle, noteSteps);
                const double bars = (double) patterns * steps * stepgrid::getStepBeats (s.rate) / 4.0;
                // Only suggest what lines up: a note count or the Up and down order whose cycle divides the Note steps.
                auto fits = [&] (int notes, Order order) { const int c = getCycleLength (notes, order, s.octaves); return notes >= 1 && notes <= maxHeld && c >= 1 && noteSteps % c == 0; };
                juce::StringArray counts;

                for (int notes : { held - 1, held + 1 })
                    if (fits (notes, s.order))
                        counts.add (juce::String (notes));

                const bool orderFits = s.order != Order::upDown && fits (held, Order::upDown);
                juce::String advice;

                if (counts.size() > 0)
                    advice = "Hold " + counts.joinIntoString (" or ") + " notes" + (orderFits ? " or try Up and down" : "") + " to line it up, or keep it for the restless feel.";
                else if (orderFits)
                    advice = "Try Up and down to line it up, or keep it for the restless feel.";
                else
                    advice = "Keep it for the restless feel.";

                lines.push_back ("Your " + juce::String (cycle) + "-note cycle on a pattern with " + juce::String (noteSteps)
                                 + " Note steps comes round at a different step each time; it lines up again every " + juce::String (patterns) + " patterns ("
                                 + formatBars (bars) + "). " + advice);
            }
        }

        // 4: too fast to follow.
        {
            const double perSecond = getNotesPerSecond (s, in.bpm);

            if (perSecond > 12.0 && s.rate < stepgrid::numStepLengths - 1)
            {
                int slower = s.rate + 1;

                for (int i = 0; i < stepgrid::numStepLengths; ++i)
                    if (std::abs (stepgrid::stepLengths[i].beats - 2.0 * stepgrid::getStepBeats (s.rate)) < 1.0e-6)
                        slower = i;

                auto slowerSettings = s;
                slowerSettings.rate = slower;
                lines.push_back ("At " + formatBpm (in.bpm) + " BPM, " + stepgrid::stepLengths[s.rate].name + " steps are " + juce::String (perSecond, 1)
                                 + " notes a second, too fast to follow one by one; they blur into a shimmer. If you want notes you can follow, try "
                                 + stepgrid::stepLengths[slower].name + " (" + juce::String (getNotesPerSecond (slowerSettings, in.bpm), 1) + " a second).");
            }
        }

        // 5: low and fast.
        {
            int lowest = 128;

            for (auto& r : in.records)
                for (int i = 0; i < r.numPlayed; ++i)
                    lowest = juce::jmin (lowest, (int) r.played[(size_t) i]);

            if (lowest < 48 && s.rate <= 2)
                lines.push_back ("The arpeggio goes down to " + getNoteName (lowest)
                                 + ". Fast low notes tend to sound muddy and get in the way of the bass; try holding the chord an octave higher.");
        }

        return lines;
    }
} // namespace arp

//==============================================================================
const char* BuiltInArpeggiator::getParameterId (arp::Knob k)
{
    switch (k)
    {
        case arp::Knob::order:   return "order";
        case arp::Knob::octaves: return "octaves";
        case arp::Knob::rate:    return "rate";
        case arp::Knob::gate:    return "gate";
        case arp::Knob::swing:   return "swing";
        case arp::Knob::steps:   return "steps";
        case arp::Knob::accent:  return "accent";
        case arp::Knob::latch:   return "latch";
    }

    return "";
}

juce::String BuiltInArpeggiator::getKindParameterId (int step)
{
    return "kind" + juce::String (step + 1);
}

juce::String BuiltInArpeggiator::getAccentParameterId (int step)
{
    return "stepaccent" + juce::String (step + 1);
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInArpeggiator::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const arp::Settings d;
    juce::StringArray orders, rates, kinds;

    for (int i = 0; i < arp::numOrders; ++i)
        orders.add (arp::getOrderName ((arp::Order) i));

    for (auto& l : stepgrid::stepLengths)
        rates.add (l.name);

    for (int i = 0; i < arp::numStepKinds; ++i)
        kinds.add (arp::getKindName ((arp::StepKind) i));

    using ID = juce::ParameterID;
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { getParameterId (arp::Knob::order), 1 }, "Order", orders, (int) d.order));
    layout.add (std::make_unique<juce::AudioParameterInt> (ID { getParameterId (arp::Knob::octaves), 1 }, "Octaves", 1, arp::maxOctaves, d.octaves));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { getParameterId (arp::Knob::rate), 1 }, "Rate", rates, d.rate));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ID { getParameterId (arp::Knob::gate), 1 }, "Gate", juce::NormalisableRange<float> { arp::minGate, arp::maxGate, 1.0f }, d.gatePercent));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ID { getParameterId (arp::Knob::swing), 1 }, "Swing", juce::NormalisableRange<float> { arp::minSwing, arp::maxSwing, 0.01f }, d.swingPercent));
    layout.add (std::make_unique<juce::AudioParameterInt> (ID { getParameterId (arp::Knob::steps), 1 }, "Steps", 1, arp::maxSteps, d.steps));
    layout.add (std::make_unique<juce::AudioParameterInt> (ID { getParameterId (arp::Knob::accent), 1 }, "Accent", 0, arp::maxAccent, d.accent));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { getParameterId (arp::Knob::latch), 1 }, "Latch", d.latch));

    for (int i = 0; i < arp::maxSteps; ++i)
        layout.add (std::make_unique<juce::AudioParameterChoice> (ID { getKindParameterId (i), 1 }, "Step " + juce::String (i + 1), kinds, 0));

    for (int i = 0; i < arp::maxSteps; ++i)
        layout.add (std::make_unique<juce::AudioParameterBool> (ID { getAccentParameterId (i), 1 }, "Step " + juce::String (i + 1) + " accent", false));

    return layout;
}

BuiltInArpeggiator::BuiltInArpeggiator()
    : juce::AudioPluginInstance (BusesProperties()),
      parameters (*this, nullptr, "DawItArpeggiator", createLayout())
{
    for (int k = 0; k < arp::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((arp::Knob) k));

    for (int i = 0; i < arp::maxSteps; ++i)
    {
        kindValues[(size_t) i] = parameters.getRawParameterValue (getKindParameterId (i));
        accentValues[(size_t) i] = parameters.getRawParameterValue (getAccentParameterId (i));
    }

    held.reserve ((size_t) arp::maxHeld + 2);
    sequence.reserve ((size_t) arp::maxSequence + 2);
    scratchOut.ensureSize (4096);
}

juce::PluginDescription BuiltInArpeggiator::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "MIDI effect";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44417231;   // "DAr1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 0;
    return d;
}

void BuiltInArpeggiator::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInArpeggiator::getKnobParameter (arp::Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

juce::RangedAudioParameter* BuiltInArpeggiator::getKindParameter (int step) const
{
    return parameters.getParameter (getKindParameterId (juce::jlimit (0, arp::maxSteps - 1, step)));
}

juce::RangedAudioParameter* BuiltInArpeggiator::getAccentParameter (int step) const
{
    return parameters.getParameter (getAccentParameterId (juce::jlimit (0, arp::maxSteps - 1, step)));
}

arp::StepKind BuiltInArpeggiator::getKind (int step) const
{
    return (arp::StepKind) juce::jlimit (0, arp::numStepKinds - 1, juce::roundToInt (kindValues[(size_t) juce::jlimit (0, arp::maxSteps - 1, step)]->load()));
}

bool BuiltInArpeggiator::isAccented (int step) const
{
    return accentValues[(size_t) juce::jlimit (0, arp::maxSteps - 1, step)]->load() >= 0.5f;
}

arp::Settings BuiltInArpeggiator::getSettings() const
{
    auto value = [this] (arp::Knob k) { return knobValues[(size_t) k]->load(); };
    arp::Settings s;
    s.order = (arp::Order) juce::jlimit (0, arp::numOrders - 1, juce::roundToInt (value (arp::Knob::order)));
    s.octaves = juce::jlimit (1, arp::maxOctaves, juce::roundToInt (value (arp::Knob::octaves)));
    s.rate = juce::jlimit (0, stepgrid::numStepLengths - 1, juce::roundToInt (value (arp::Knob::rate)));
    s.gatePercent = juce::jlimit (arp::minGate, arp::maxGate, value (arp::Knob::gate));
    s.swingPercent = juce::jlimit (arp::minSwing, arp::maxSwing, value (arp::Knob::swing));
    s.steps = juce::jlimit (1, arp::maxSteps, juce::roundToInt (value (arp::Knob::steps)));
    s.accent = juce::jlimit (0, arp::maxAccent, juce::roundToInt (value (arp::Knob::accent)));
    s.latch = value (arp::Knob::latch) >= 0.5f;

    for (int i = 0; i < arp::maxSteps; ++i)
    {
        s.kinds[(size_t) i] = getKind (i);
        s.accents[(size_t) i] = isAccented (i);
    }

    return s;
}

void BuiltInArpeggiator::setKnob (arp::Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInArpeggiator::setKind (int step, arp::StepKind kind)
{
    if (auto* p = getKindParameter (step))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) (int) kind));
}

void BuiltInArpeggiator::setAccented (int step, bool accented)
{
    if (auto* p = getAccentParameter (step))
        p->setValueNotifyingHost (accented ? 1.0f : 0.0f);
}

void BuiltInArpeggiator::setSettings (const arp::Settings& s)
{
    setKnob (arp::Knob::order, (float) (int) s.order);
    setKnob (arp::Knob::octaves, (float) s.octaves);
    setKnob (arp::Knob::rate, (float) s.rate);
    setKnob (arp::Knob::gate, s.gatePercent);
    setKnob (arp::Knob::swing, s.swingPercent);
    setKnob (arp::Knob::steps, (float) s.steps);
    setKnob (arp::Knob::accent, (float) s.accent);
    setKnob (arp::Knob::latch, s.latch ? 1.0f : 0.0f);

    for (int i = 0; i < arp::maxSteps; ++i)
    {
        setKind (i, s.kinds[(size_t) i]);
        setAccented (i, s.accents[(size_t) i]);
    }
}

void BuiltInArpeggiator::playExample (const arp::Example& example)
{
    {
        const juce::SpinLock::ScopedLockType lock (exampleLock);
        pendingExample = example;
        exampleWaiting = true;
    }
}

void BuiltInArpeggiator::setSongContext (int, int chord) noexcept
{
    songChord = chord;
    chordPitchClasses = chord;
}

//==============================================================================
void BuiltInArpeggiator::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    stepClock.reset();
    scratchOut.ensureSize (4096);
    resetState();
    history.clear();
}

void BuiltInArpeggiator::resetState()
{
    held.clear();
    pedalDown = false;
    orderCounter = 0;
    orderStamp = 0;
    lastStampSample = -1;
    sounding.fill (false);
    numSounding = 0;
    exampleActive = false;
    examplePlaying = false;
    lastFiredK = std::numeric_limits<long long>::min();
    playingStep = -1;
    publishState();
}

void BuiltInArpeggiator::publishState()
{
    unsigned long long h[2] = {}, n[2] = {};

    for (auto& note : held)
        h[note.pitch / 64] |= 1ULL << (note.pitch % 64);

    for (int p = 0; p < 128; ++p)
        if (sounding[(size_t) p])
            n[p / 64] |= 1ULL << (p % 64);

    heldKeys[0] = h[0];
    heldKeys[1] = h[1];
    soundingNotes[0] = n[0];
    soundingNotes[1] = n[1];
}

void BuiltInArpeggiator::sendNoteOff (juce::MidiBuffer& out, int pos, int pitch)
{
    if (! sounding[(size_t) pitch])
        return;

    out.addEvent (juce::MidiMessage::noteOff (1, pitch), pos);
    sounding[(size_t) pitch] = false;
    --numSounding;
}

void BuiltInArpeggiator::sendNoteOn (juce::MidiBuffer& out, int pos, int pitch, int velocity, long long due)
{
    sendNoteOff (out, pos, pitch);   // a pitch it already sounds is ended first
    out.addEvent (juce::MidiMessage::noteOn (1, pitch, (juce::uint8) juce::jlimit (1, 127, velocity)), pos);
    sounding[(size_t) pitch] = true;
    offAt[(size_t) pitch] = due;
    ++numSounding;
}

void BuiltInArpeggiator::sendDueOffs (juce::MidiBuffer& out, int pos)
{
    if (numSounding == 0)
        return;

    const long long now = samplesDone + pos;

    for (int p = 0; p < 128; ++p)
        if (sounding[(size_t) p] && offAt[(size_t) p] <= now)
            sendNoteOff (out, pos, p);
}

void BuiltInArpeggiator::endOwnNotes (juce::MidiBuffer& out, int pos)
{
    if (numSounding == 0)
        return;

    for (int p = 0; p < 128; ++p)
        sendNoteOff (out, pos, p);
}

void BuiltInArpeggiator::scheduleNextStep (const arp::Settings& s, long long minK)
{
    const double len = stepgrid::getStepBeats (s.rate);
    long long k = juce::jmax (minK, (long long) std::floor (beat0 / len) - 2);
    double x = 0.0;

    for (;; ++k)
    {
        x = (arp::getStepStartBeats (k, s.rate, s.swingPercent) - beat0) / beatsPerSample;

        if (std::ceil (x - 1.0e-6) >= 0.0)
            break;
    }

    nextK = k;
    nextStepSample = (int) juce::jmax (0.0, std::ceil (x - 1.0e-6));
}

void BuiltInArpeggiator::chordStart (juce::MidiBuffer& out, int pos, const arp::Settings& s)
{
    orderCounter = 0;

    if (! songBased)
    {
        // A clock of its own starts at step 1 with the chord.
        beat0 = -pos * beatsPerSample;
        lastFiredK = std::numeric_limits<long long>::min();
        scheduleNextStep (s, 0);
        return;
    }

    // A chord that arrives up to a quarter of a step late plays that step at once.
    const long long prev = nextK - 1;

    if (prev == lastFiredK)
        return;

    const double startX = (arp::getStepStartBeats (prev, s.rate, s.swingPercent) - beat0) / beatsPerSample;
    const double endX = (arp::getStepStartBeats (prev + 1, s.rate, s.swingPercent) - beat0) / beatsPerSample;
    const double late = (double) pos - startX;

    if (late > 0.0 && late <= 0.25 * (endX - startX))
        fireStep (out, pos, prev, s);
}

void BuiltInArpeggiator::handleNoteOn (juce::MidiBuffer& out, int pos, int pitch, int velocity, const arp::Settings& s)
{
    auto exists = std::find_if (held.begin(), held.end(), [pitch] (auto& h) { return h.pitch == pitch; });

    if (exists == held.end() && (int) held.size() >= arp::maxHeld)
        return;

    const bool anyDown = std::any_of (held.begin(), held.end(), [] (auto& h) { return h.down; });
    bool fromNothing = held.empty();

    // Latch: the next chord after letting go of everything replaces the one held.
    if (s.latch && ! anyDown && ! held.empty())
    {
        held.clear();
        endOwnNotes (out, pos);
        fromNothing = true;
        exists = held.end();
    }

    if (exists != held.end())
    {
        exists->velocity = velocity;
        exists->down = true;
    }
    else
    {
        // Notes pressed on the same sample count as pressed together.
        const long long now = samplesDone + pos;

        if (now != lastStampSample)
        {
            ++orderStamp;
            lastStampSample = now;
        }

        held.push_back ({ pitch, velocity, orderStamp, true });
    }

    if (fromNothing)
        chordStart (out, pos, s);
}

void BuiltInArpeggiator::handleNoteOff (juce::MidiBuffer& out, int pos, int pitch, const arp::Settings& s)
{
    auto it = std::find_if (held.begin(), held.end(), [pitch] (auto& h) { return h.pitch == pitch; });

    if (it == held.end())
        return;

    if (s.latch || pedalDown)
        it->down = false;
    else
        held.erase (it);

    if (held.empty())
        endOwnNotes (out, pos);
}

void BuiltInArpeggiator::fireStep (juce::MidiBuffer& out, int pos, long long k, const arp::Settings& s)
{
    const int steps = juce::jlimit (1, arp::maxSteps, s.steps);
    const int index = (int) (((k % steps) + steps) % steps);
    const auto kind = s.kinds[(size_t) index];
    const bool accented = s.accents[(size_t) index];
    const int tied = arp::getTiedSteps (s, index);
    lastFiredK = k;

    arp::StepRecord record;
    record.step = index;
    record.kind = kind;
    record.beat = arp::getStepStartBeats (k, s.rate, s.swingPercent);
    record.chordPitchClasses = songChord.load();

    for (auto& h : held)
        record.heldMask[(size_t) (h.pitch / 64)] |= 1ULL << (h.pitch % 64);

    const double endBeat = arp::getNoteEndBeats (k, tied, s.rate, s.swingPercent, s.gatePercent);
    const long long due = juce::jmax (samplesDone + pos + 1, samplesDone + (long long) std::llround ((endBeat - beat0) / beatsPerSample));

    auto play = [&] (int pitch, int velocity)
    {
        if (record.numPlayed < arp::maxHeld)
            record.played[(size_t) record.numPlayed++] = (std::uint8_t) pitch;

        sendNoteOn (out, pos, pitch, accented ? arp::getAccentedVelocity (velocity, s.accent) : velocity, due);
    };

    switch (kind)
    {
        case arp::StepKind::note:
        {
            arp::buildSequence (held, s.order, s.octaves, sequence);

            if (! sequence.empty())
            {
                const int n = (int) sequence.size();
                const int i = s.order == arp::Order::random ? arp::getRandomIndex (k, n) : (int) (orderCounter % n);
                ++orderCounter;
                play (sequence[(size_t) i].pitch, sequence[(size_t) i].velocity);
            }

            break;
        }

        case arp::StepKind::chord:
        {
            arp::buildSequence (held, arp::Order::up, 1, sequence);

            for (auto& note : sequence)
                play (note.pitch, note.velocity);

            break;
        }

        case arp::StepKind::bass:
        {
            arp::buildSequence (held, arp::Order::up, 1, sequence);

            if (! sequence.empty())
                play (arp::getBassPitch (sequence.front().pitch), sequence.front().velocity);

            break;
        }

        case arp::StepKind::rest:
        case arp::StepKind::tie:
            break;
    }

    playingStep = index;
    history.push (record);
}

void BuiltInArpeggiator::beginExample (juce::MidiBuffer& out, const arp::Example& example)
{
    held.clear();
    endOwnNotes (out, 0);
    pedalDown = false;
    exampleActive = true;
    activeExample = example;
    examplePlaying = true;
    exampleEndsAt = samplesDone + (long long) std::llround (example.beats / beatsPerSample);
}

void BuiltInArpeggiator::endExample (juce::MidiBuffer& out, int pos)
{
    exampleActive = false;
    examplePlaying = false;
    held.clear();
    endOwnNotes (out, pos);
}

void BuiltInArpeggiator::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = audio.getNumSamples();
    auto& out = scratchOut;
    out.clear();

    auto block = stepClock.begin (getPlayHead(), false, 120.0, rate);
    tempoBpm = block.hostBpm;
    usedBpm = block.bpm;
    beatsPerSample = block.beatsPerSample;
    beat0 = block.beat;
    songBased = block.songBased;
    stepClock.commit (block);
    usingSongClock = songBased;
    clockBeat = block.beat;

    const auto parameterSettings = getSettings();

    // Latch switched off: what only latch held is let go.
    if (wasLatch && ! parameterSettings.latch && ! pedalDown)
    {
        held.erase (std::remove_if (held.begin(), held.end(), [] (auto& h) { return ! h.down; }), held.end());

        if (held.empty())
            endOwnNotes (out, 0);
    }

    wasLatch = parameterSettings.latch;

    if (block.jump)
    {
        endOwnNotes (out, 0);
        lastFiredK = std::numeric_limits<long long>::min();
    }

    // A test chord or an example starts at the start of this block, with a chord of its own.
    bool startNotes = false;

    if (stopRequested.exchange (false) && exampleActive)
        endExample (out, 0);

    if (testChordRequested.exchange (false))
    {
        arp::Example example;
        example.settings = parameterSettings;
        const int chord = songChord.load();

        if (chord != 0)
        {
            for (int pc = 0; pc < 12; ++pc)
                if (((chord >> pc) & 1) != 0)
                    example.pitches[(size_t) example.numPitches++] = 60 + pc;
        }
        else
        {
            example.pitches = { 60, 64, 67 };
            example.numPitches = 3;
        }

        beginExample (out, example);
        startNotes = true;
    }
    else if (exampleWaiting.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (exampleLock);

        if (lock.isLocked())
        {
            exampleWaiting = false;
            beginExample (out, pendingExample);
            startNotes = true;
        }
    }

    const arp::Settings s = exampleActive ? activeExample.settings : parameterSettings;
    scheduleNextStep (s, songBased ? std::numeric_limits<long long>::min() : 0);

    if (startNotes)
        for (int i = 0; i < activeExample.numPitches; ++i)
            handleNoteOn (out, 0, activeExample.pitches[(size_t) i], activeExample.velocity, s);

    auto it = midi.begin();
    const auto end = midi.end();
    int pos = 0;

    auto handleEvent = [&] (const juce::MidiMessage& m, int at, bool ons)
    {
        if (m.isNoteOn())
        {
            if (ons && ! exampleActive)
                handleNoteOn (out, at, m.getNoteNumber(), m.getVelocity(), s);

            return;
        }

        if (ons)
            return;

        if (m.isNoteOff())
        {
            if (! exampleActive)
                handleNoteOff (out, at, m.getNoteNumber(), s);
        }
        else if (m.isSustainPedalOn())
        {
            if (! exampleActive)
                pedalDown = true;
        }
        else if (m.isSustainPedalOff())
        {
            if (! exampleActive)
            {
                pedalDown = false;

                if (! s.latch)
                {
                    held.erase (std::remove_if (held.begin(), held.end(), [] (auto& h) { return ! h.down; }), held.end());

                    if (held.empty())
                        endOwnNotes (out, at);
                }
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            if (exampleActive)
                endExample (out, at);

            held.clear();
            pedalDown = false;
            endOwnNotes (out, at);
            out.addEvent (m, at);
        }
        else
        {
            out.addEvent (m, at);
        }
    };

    for (;;)
    {
        int t = total;

        if (numSounding > 0)
        {
            long long soonest = std::numeric_limits<long long>::max();

            for (int p = 0; p < 128; ++p)
                if (sounding[(size_t) p])
                    soonest = juce::jmin (soonest, offAt[(size_t) p]);

            t = (int) juce::jmin ((long long) t, juce::jmax ((long long) pos, soonest - samplesDone));
        }

        if (it != end)
            t = juce::jmin (t, juce::jmax (pos, (*it).samplePosition));

        t = juce::jmin (t, juce::jmax (pos, nextStepSample));

        if (exampleActive)
            t = (int) juce::jmin ((long long) t, juce::jmax ((long long) pos, exampleEndsAt - samplesDone));

        if (t >= total)
            break;

        pos = t;
        sendDueOffs (out, pos);

        if (exampleActive && exampleEndsAt - samplesDone <= pos)
            endExample (out, pos);

        // Note-offs and the rest first, then the note-ons, so a chord that changes on one sample starts from nothing.
        const auto first = it;

        for (int pass = 0; pass < 2; ++pass)
            for (auto e = first; e != end && (*e).samplePosition <= pos; ++e)
                handleEvent ((*e).getMessage(), pos, pass == 1);

        while (it != end && (*it).samplePosition <= pos)
            ++it;

        while (nextStepSample <= pos)
        {
            if (hasHeld() && nextK != lastFiredK)
                fireStep (out, pos, nextK, s);

            scheduleNextStep (s, nextK + 1);
        }
    }

    // Anything left lies at or after the end of the block (it shouldn't): treat it as at the last sample.
    for (auto e = it; e != end; ++e)
    {
        handleEvent ((*e).getMessage(), juce::jmax (0, total - 1), false);
        handleEvent ((*e).getMessage(), juce::jmax (0, total - 1), true);
    }

    block.beat = beat0;
    stepClock.finish (block, total);
    samplesDone += total;
    publishState();
    midi.swapWith (out);
}

void BuiltInArpeggiator::processBlockBypassed (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    // Switched off, it lets the notes through, after ending the notes it started.
    auto& out = scratchOut;
    out.clear();
    endOwnNotes (out, 0);
    held.clear();
    pedalDown = false;
    exampleActive = false;
    examplePlaying = false;
    testChordRequested = false;
    stopRequested = false;
    playingStep = -1;

    for (const auto metadata : midi)
        out.addEvent (metadata.getMessage(), metadata.samplePosition);

    samplesDone += audio.getNumSamples();
    publishState();
    midi.swapWith (out);
}

juce::AudioProcessorEditor* BuiltInArpeggiator::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInArpeggiator::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInArpeggiator::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
