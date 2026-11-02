#include "ChordRoll.h"

namespace daw::gen::chordroll
{
std::vector<int> pickDegrees (int numBars, unsigned seed)
{
    std::mt19937 rng (seed);

    // Picks a degree from (degree, weight) pairs, skipping `avoid`.
    auto pick = [&rng] (std::initializer_list<std::pair<int, int>> options, int avoid)
    {
        int total = 0;
        for (auto& o : options)
            if (o.first != avoid)
                total += o.second;

        int roll = (int) (rng() % (unsigned) total);
        for (auto& o : options)
        {
            if (o.first == avoid)
                continue;
            if (roll < o.second)
                return o.first;
            roll -= o.second;
        }
        return 1;
    };

    std::vector<int> degrees;
    for (int i = 0; i < numBars; ++i)
    {
        const int previous = degrees.empty() ? 0 : degrees.back();

        if (i == 0)
            degrees.push_back (pick ({ { 1, 70 }, { 4, 15 }, { 6, 15 } }, 0));
        else if (i == numBars - 1 && numBars >= 4)
            degrees.push_back (pick ({ { 5, 60 }, { 1, 40 } }, previous));
        else
            degrees.push_back (pick ({ { 1, 10 }, { 2, 15 }, { 3, 10 }, { 4, 25 }, { 5, 25 }, { 6, 15 } }, previous));
    }

    return degrees;
}

std::vector<Chord> generate (const MusicalKey& key, int numBars, unsigned seed)
{
    std::vector<Chord> chords;
    for (int degree : pickDegrees (numBars, seed))
        chords.push_back (harmony::triad (key, degree));
    return chords;
}
}
