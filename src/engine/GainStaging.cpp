#include "GainStaging.h"

namespace daw::gainstaging
{
Zone getZone (float peakDb)
{
    if (peakDb < silentBelowDb) return Zone::silent;
    if (peakDb < quietBelowDb)  return Zone::quiet;
    if (peakDb <= hotAboveDb)   return Zone::healthy;
    if (peakDb <= overAboveDb)  return Zone::hot;
    return Zone::over;
}

juce::String getZoneName (Zone zone)
{
    switch (zone)
    {
        case Zone::silent:  return "silent";
        case Zone::quiet:   return "quiet";
        case Zone::healthy: return "healthy";
        case Zone::hot:     return "hot";
        case Zone::over:    return "over 0 dB";
    }

    return {};
}

juce::String formatDb (float db)
{
    if (db < silentBelowDb)
        return "-inf dB";

    return (db > 0.05f ? "+" : "") + juce::String (db, 1) + " dB";
}

juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::what:       return "What is gain staging?";
        case Topic::zones:      return "Quiet, healthy, hot, over";
        case Topic::louder:     return "Louder sounds better";
        case Topic::compressor: return "Levels into a compressor";
        case Topic::fader:      return "Effects, fader and master";
        case Topic::bus:        return "Through a bus";
        case Topic::over:       return "Over 0 dB";
        case Topic::steps:      return "Step by step";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::what:       return "How loud the sound is at each step";
        case Topic::zones:      return "What the colours mean";
        case Topic::louder:     return "Compare effects at the same loudness";
        case Topic::compressor: return "Why the level going in matters";
        case Topic::fader:      return "Where to turn up and down";
        case Topic::bus:        return "Why a bus gets louder than its tracks";
        case Topic::over:       return "When it distorts, and when it doesn't";
        case Topic::steps:      return "Setting the levels of a track";
    }

    return {};
}

juce::String explain (Topic topic)
{
    switch (topic)
    {
        case Topic::what:
            return "Every sound goes through a chain of steps before you hear it: the instrument (or the recording), "
                   "each effect in order, the track's fader, any bus it plays into, and then the master, where all tracks come together and go to your speakers.\n\n"
                   "Gain staging means keeping a sensible level at every one of those steps: not so quiet that the sound gets lost, "
                   "and not so loud that the next step is overloaded. Gain just means how much a step turns the level up or down.\n\n"
                   "In DawIt you see it in the device bar under the arrangement. The arrows between the steps take the colour of the level "
                   "passing through them, with its peak in dB above them. Each effect shows a badge with how much louder or quieter it makes "
                   "the sound, and a ! appears where something needs a look: hover over it to read what, click it to read more here.\n\n"
                   "The picture above shows the selected track's chain the same way, live while it plays.";

        case Topic::zones:
            return "The levels DawIt shows are peaks: the loudest moments, in dB below full scale. 0 dB is the loudest a sound file "
                   "or a sound card can take.\n\n"
                   "Grey, quiet (below -24 dB): fine for a soft part. If a whole track is this quiet, its effects work on a faint "
                   "signal and you end up turning things up later.\n\n"
                   "Green, healthy (-24 to -6 dB): where most tracks belong, with room above for the loud moments.\n\n"
                   "Yellow, hot (-6 to 0 dB): loud. Fine now and then, but with several hot tracks the master goes over.\n\n"
                   "Red, over 0 dB: too hot for the next step. Between effects DawIt itself doesn't distort (see Over 0 dB), "
                   "but at the master it does.\n\n"
                   "Peaks are not loudness: a drum hit can peak high and still sound softer than a pad that peaks lower. "
                   "Use the colours as a guide and your ears for the balance.";

        case Topic::louder:
            return "Make a sound a little louder and it almost always seems better: fuller, clearer, more exciting. "
                   "That is how our ears work, and it fools everyone.\n\n"
                   "So an EQ boost, or a compressor with makeup gain, can seem like an improvement when it is mostly just louder. "
                   "To judge an effect honestly, compare it on and off at the same loudness.\n\n"
                   "The badge on each effect in the device bar shows how much louder (+) or quieter (-) the effect makes the sound "
                   "while it plays. Even 1 dB is enough to fool you.\n\n"
                   "DawIt EQ and DawIt Compressor have match level: it keeps what comes out as loud as what goes in, so switching "
                   "the effect on and off changes only the sound, not the loudness. Click the badge on their chip to switch it on "
                   "or off. For other plugins, turn their output down by about the number on the badge.";

        case Topic::compressor:
            return "A compressor turns down what goes over its threshold. The threshold is a fixed level, so what the compressor "
                   "does depends on how loud the sound coming in is.\n\n"
                   "Turn the instrument up by 6 dB and the compressor after it squeezes up to 6 dB more, with the same settings. "
                   "Presets and typical settings assume a healthy level coming in, with peaks around -18 to -6 dB.\n\n"
                   "So set the level going into the compressor first, then its threshold. When you change the level before it "
                   "later, check the threshold again.\n\n"
                   "Many plugins modelled on analog gear (tape, tube and console emulations) are the same: they were made for a "
                   "certain level, and sound different, often distorted, when driven harder.";

        case Topic::fader:
            return "Set levels in the order the sound travels: the source first (the instrument's own volume, or how loud it was "
                   "recorded), then each effect's output, then the track's fader, and the master last.\n\n"
                   "A common tangle: effects keep adding gain, the track gets too loud, and the fader is pulled far down to make up "
                   "for it. It works, but every effect along the way gets a hotter level than it needs, and the fader ends up where "
                   "it is hard to fine-tune. Take the gain back where it was added: in an effect's output, or the instrument.\n\n"
                   "The fader is for the mix, the balance between tracks. Faders within about 10 dB of 0 dB leave the most room to move.\n\n"
                   "Leave the master at 0 dB most of the time. If the whole mix is too loud, turn the tracks down instead.";

        case Topic::bus:
            return "A bus adds its tracks together, so it is louder than any one of them. Two different sounds at -6 dB make about "
                   "-3 dB (+3 dB, because their power adds up), and four equal tracks are 6 dB louder than one. If they play the same "
                   "sound in step, such as two copies of one drum hit, the amplitudes add instead: two tracks are +6 dB, four are +12 dB.\n\n"
                   "Peaks add up too, at most like the second kind. Four drum tracks at -10 dB each are about 6 dB louder together "
                   "(about -4 dB), but when their hits line up exactly, the peaks can reach +2.0 dB in the bus. Hits that never "
                   "happen at the same moment don't add up at all.\n\n"
                   "A bus doesn't make the mix louder by itself: the same tracks would add up in the master anyway.\n\n"
                   "What to do: keep the tracks that go into a bus lower, with peaks around -18 to -12 dB; leave the bus's fader near "
                   "0 dB and balance with the tracks' faders; turn the bus down to move the whole group in the mix. Over 0 dB inside "
                   "DawIt doesn't distort, but the bus's effects, a compressor above all, get the hotter level.\n\n"
                   "The Buses, sends and routing guide shows your song's buses.";

        case Topic::over:
            return "DawIt calculates with floating-point numbers, which have enormous room above 0 dB. So a level over 0 dB between "
                   "effects, or on a track, doesn't distort by itself: turn it down later and the sound comes back unharmed.\n\n"
                   "It does distort in two places: at the master output, where the sound goes to your sound card, and in an exported "
                   "16- or 24-bit file. Both stop at 0 dB and cut off everything above it, which you hear as crackle or harshness on the loud "
                   "moments (a 32-bit float file keeps it, so it can still be turned down later). That is why a ! on the master "
                   "is red.\n\n"
                   "Over 0 dB still matters before the master: plugins modelled on analog gear may distort, and compressors and "
                   "other effects that react to the level work much harder than their settings suggest.\n\n"
                   "Leave room at the master while mixing, with peaks around -6 to -3 dB. Making the finished mix loud is a job "
                   "for mastering, at the end.";

        case Topic::steps:
            return "1. Start with the track's fader and the master at 0 dB.\n\n"
                   "2. Play the loudest part of the song, with only this track (solo it).\n\n"
                   "3. Set the source so the first arrow is green, with peaks around -18 to -10 dB: the instrument's own volume, "
                   "or for a recording the first effect's output.\n\n"
                   "4. Go along the chain. Each effect's badge should stay near 0 dB, unless you want that effect to make the track "
                   "louder. Turn an effect's output down by what it adds, or use match level on DawIt EQ and DawIt Compressor.\n\n"
                   "5. Do the same for every track, then balance the tracks with their faders.\n\n"
                   "6. Watch the master: its peaks should stay below about -6 dB. If they don't, pull all faders down a little, together.\n\n"
                   "Then trust your ears: these numbers are a starting point, not a rule.";
    }

    return {};
}

std::vector<Hint> findHints (const std::vector<Stop>& stops)
{
    std::vector<Hint> hints;

    if (stops.empty())
        return hints;

    // The source: what the instrument plays, or for a recording what comes into the first step.
    const auto& first = stops.front();
    const float source = first.kind == StopKind::instrument || first.kind == StopKind::busInput ? first.outDb : first.inDb;

    if (source >= silentBelowDb && source < veryQuietDb)
        hints.push_back ({ 0, false, Topic::zones,
                           "Very quiet from the start (" + formatDb (source) + " peaks). Nothing breaks, but every effect then works on a faint "
                           "signal and you end up turning things up later. Turn it up at the source: the instrument's own volume, "
                           "or the output of the first effect (for a bus, the tracks that play into it)." });

    float added = 0.0f;
    int busHintAt = -1;   // the stop whose hot sum is already pointed out: the effect after it needn't say so again

    for (int i = 0; i < (int) stops.size(); ++i)
    {
        const auto& stop = stops[(size_t) i];

        if (stop.kind == StopKind::send)
            continue;   // a branch off the line: its level is not a step of the chain

        if (stop.kind == StopKind::busInput || stop.kind == StopKind::bus)
        {
            const float sum = stop.kind == StopKind::busInput ? stop.outDb : stop.inDb;

            // A bus with no effects has nothing to turn down for: the sum goes on to its fader.
            if (sum > overAboveDb && stop.busHasEffects)
            {
                busHintAt = i;

                if (stop.isReturn)
                {
                    hints.push_back ({ i, false, Topic::bus,
                                       "The sends into " + stop.name + " add up to " + formatDb (sum) + " peaks. Nothing distorts inside DawIt, but the "
                                       "return's effects get a hot level. Turn the sends down a little, together, or the first effect's input." });
                    continue;
                }

                hints.push_back ({ i, false, Topic::bus,
                                   "The " + (stop.numTracks > 0 ? juce::String (stop.numTracks) + " tracks" : juce::String ("tracks")) + " in " + stop.name
                                   + " add up to " + formatDb (sum) + " peaks. Nothing distorts inside DawIt, but any effect on the bus gets a hot level. "
                                   "Turn the tracks down a little, together." });
            }
        }
        else if (stop.kind == StopKind::effect && ! stop.bypassed)
        {
            if (stop.inDb > overAboveDb && busHintAt != i - 1)
                hints.push_back ({ i, false, Topic::compressor,
                                   stop.isCompressor
                                       ? "Very hot going into " + stop.name + " (" + formatDb (stop.inDb) + " peaks). Its threshold reacts to the "
                                         "level coming in, so it squeezes much harder than its settings suggest. Turn down before it, then "
                                         "set the threshold."
                                       : "Over 0 dB going into " + stop.name + " (" + formatDb (stop.inDb) + " peaks). DawIt itself doesn't distort "
                                         "here, but many plugins, especially ones modelled on analog gear, expect lower levels and can distort "
                                         "or react differently. Turn down before it." });

            if (stop.changeDb.has_value())
            {
                added += *stop.changeDb;

                if (*stop.changeDb >= louderHintDb)
                    hints.push_back ({ i, false, Topic::louder,
                                       stop.matching
                                           ? stop.name + " still makes the sound " + juce::String (*stop.changeDb, 1) + " dB louder with match level "
                                             "on: its output gain is turned up. Turn the output down to compare it on and off at the same loudness."
                                           : stop.name + " makes the sound " + juce::String (*stop.changeDb, 1) + " dB louder. Louder almost always "
                                             "sounds better at first, so compare it on and off at the same loudness: turn its output down by about "
                                             "that much" + (stop.canMatch ? juce::String (", or click its badge to switch on match level.") : juce::String (".")) });
            }
        }
        else if (stop.kind == StopKind::fader)
        {
            if (stop.inDb >= silentBelowDb && added >= fightHintDb && stop.settingDb <= -fightHintDb)
                hints.push_back ({ i, false, Topic::fader,
                                   "The effects make the sound " + juce::String (added, 1) + " dB louder, then the fader takes "
                                   + juce::String (-stop.settingDb, 1) + " dB off again. It works, but every effect gets a hotter level than "
                                   "it needs. Take the gain back inside the chain (an effect's output, or the instrument) and keep the "
                                   "fader for the mix." });
        }
        else if (stop.kind == StopKind::master)
        {
            if (stop.outDb > overAboveDb)
                hints.push_back ({ i, true, Topic::over,
                                   "The master is over 0 dB (" + formatDb (stop.outDb) + " peaks): this is where it really distorts, in your "
                                   "speakers and in an exported file. Turn the loudest tracks down, or the master." });
        }
    }

    return hints;
}
} // namespace daw::gainstaging
