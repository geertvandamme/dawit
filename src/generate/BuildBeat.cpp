#include "BuildBeat.h"

#include "RhythmGuide.h"
#include "engine/TimeSignature.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace daw::gen::buildbeat
{
using guides::Guide;

namespace
{
    constexpr double tolerance = 0.02;

    bool isOnGrid (double offset, double step)
    {
        const double lines = offset / step;
        return std::abs (lines - std::round (lines)) * step < tolerance;
    }

    /** The pitches of the kick, snare and hi-hat roles, and how a role ranks when two drums say the same thing (hat first). */
    int rank (Role role) noexcept
    {
        return role == Role::hat ? 0 : role == Role::snare ? 1 : 2;
    }

    /** The bars to look at, the groove bar first and then the others in order. */
    std::vector<const Bar*> getOrder (const std::vector<Bar>& bars)
    {
        std::vector<const Bar*> order;
        const auto* groove = findGrooveBar (bars);

        if (groove != nullptr)
            order.push_back (groove);

        for (auto& bar : bars)
            if (&bar != groove)
                order.push_back (&bar);

        return order;
    }

    /** The starts of one drum's hits in a bar. */
    std::map<int, std::vector<double>> groupByPitch (const Bar& bar)
    {
        std::map<int, std::vector<double>> groups;

        for (auto& hit : bar.hits)
            if (getRole (hit.pitch) != Role::none)
                groups[hit.pitch].push_back (hit.offset);

        return groups;
    }

    /** "1", "2 &", "the & of 3": how a hit at `offset` beats into the bar is counted. */
    juce::String countText (const Engine& engine, double offset)
    {
        const auto sig = engine.getTimeSignature();
        const double pulse = timesig::getPulseLength (sig);
        const double pulses = offset / pulse;
        const int number = (int) std::floor (pulses + 1.0e-3) + 1;
        const auto part = rhythm::countAt (sig, offset);

        if (part == juce::String (number))
            return part;

        return part.isNotEmpty() ? "the " + part + " of " + juce::String (number) : "beat " + juce::String (offset / pulse + 1.0, 2);
    }

    juce::String joinCounts (const juce::StringArray& counts)
    {
        if (counts.size() < 2)
            return counts.joinIntoString ("");

        auto head = counts;
        head.remove (counts.size() - 1);
        return head.joinIntoString (", ") + " and " + counts[counts.size() - 1];
    }
}

//==============================================================================
std::vector<Step> getSteps()
{
    using L = firstsong::Link;
    return {
        { "Kick and snare", "The heartbeat and the backbeat",
          "The kick and the snare carry the beat. The snare on 2 and 4 is the backbeat, the accent almost every rock and pop song has.",
          { L { Guide::instruments, "Drums and percussion" }, L { Guide::rhythm, "Counting" } } },
        { "Hi-hats", "Keeping time",
          "The hi-hat fills the space between the kick and snare and tells everyone how fast the beat moves. Eighths sound steady; sixteenths sound busier.",
          { L { Guide::rhythm, "Counting" }, L { Guide::rhythm, "Reading DawIt" } } },
        { "Make it groove", "Loud and soft",
          "A drummer never hits twice the same way. Louder hits on the beat and softer ones between give the beat a pulse. A very soft snare between "
          "the backbeats, a ghost note at 20 to 40, adds more.",
          { L { Guide::midi, "Velocity" }, L { Guide::drums, "Velocity, accents and groove" } } },
        { "Add swing", "Long-short pairs",
          "Swing makes the first note of each pair longer and the second shorter. 50% is straight; 66% is the triplet feel of a shuffle. A little swing "
          "makes a stiff beat bounce.",
          { L { Guide::rhythm, "Swing" }, L { Guide::rhythm, "Quantising" } } },
    };
}

juce::String getTitle() { return "Build a beat"; }

juce::String getIntro()
{
    return "A beat in four steps, on a drum track of your own song. Each step ticks when the notes are really there. Press Space at any point to hear where you are.";
}

juce::String getHelpDescription() { return "Kick and snare, hi-hats, accents and swing: build a drum beat in four steps."; }

juce::String getCompletionText()
{
    return "You built a beat: kick and snare, hi-hats, accents and swing. Copy the clip to fill the song (Cmd+D), change it for the chorus, and add a fill on the toms at the end of a section.";
}

juce::String getTrackMenuTooltip() { return "Build a beat on this track, step by step."; }

int clampStep (int stored) noexcept
{
    return stored >= 0 && stored < numSteps ? stored : 0;
}

//==============================================================================
Role getRole (int pitch) noexcept
{
    switch (pitch)
    {
        case 35: case 36:                       return Role::kick;
        case 37: case 38: case 39: case 40:     return Role::snare;
        case 42: case 44: case 46: case 51:     return Role::hat;
        default:                                break;
    }

    return Role::none;
}

juce::String getDrumName (int pitch)
{
    switch (pitch)
    {
        case 35: case 36:           return "Kick";
        case 37:                    return "Side stick";
        case 38: case 40:           return "Snare";
        case 39:                    return "Clap";
        case 42: case 44: case 46:  return "Hi-hat";
        case 51:                    return "Ride";
        default:                    break;
    }

    return {};
}

//==============================================================================
bool hasDrumTrack (const Engine& engine)
{
    for (auto& t : engine.getTracks())
        if (firstsong::isDrumTrack (*t))
            return true;

    return false;
}

Beat findBeat (const Engine& engine, const Progress& progress)
{
    // The selected track and clip are only compared with the engine's own, never used before they are found there.
    const Track* selectedTrack = nullptr;
    const MidiClip* selectedClip = nullptr;
    const Track* selectedClipTrack = nullptr;
    std::vector<const Track*> drums;

    for (auto& t : engine.getTracks())
    {
        if (! firstsong::isDrumTrack (*t))
            continue;

        drums.push_back (t.get());

        if (t.get() == progress.selectedTrack)
            selectedTrack = t.get();

        for (auto& c : t->clips)
            if (c.get() == progress.selectedClip && c->kind == Clip::Kind::midi)
            {
                selectedClip = static_cast<const MidiClip*> (c.get());
                selectedClipTrack = t.get();
            }
    }

    if (selectedClip != nullptr)
        return { selectedClipTrack, selectedClip };

    if (drums.empty())
        return {};

    std::vector<const Track*> order;

    if (selectedTrack != nullptr)
        order.push_back (selectedTrack);

    for (auto* t : drums)
        if (t != selectedTrack)
            order.push_back (t);

    for (auto* t : order)
        for (auto& c : t->clips)
            if (c->kind == Clip::Kind::midi && ! static_cast<const MidiClip&> (*c).notes.empty())
                return { t, static_cast<const MidiClip*> (c.get()) };

    for (auto* t : order)
        for (auto& c : t->clips)
            if (c->kind == Clip::Kind::midi)
                return { t, static_cast<const MidiClip*> (c.get()) };

    return { order.front(), nullptr };
}

//==============================================================================
std::vector<Bar> getBars (const Engine& engine, const MidiClip& clip)
{
    const double barLength = engine.getBarLength();
    std::map<int, Bar> bars;

    if (barLength <= 0.0)
        return {};

    for (auto& note : clip.notes)
    {
        if (note.start >= clip.lengthBeats - 1.0e-9)
            continue;

        const double songStart = clip.startBeat + note.start;
        const int index = (int) std::floor (songStart / barLength + 1.0e-9);
        auto& bar = bars[index];
        bar.index = index;
        bar.hits.push_back ({ note.pitch, juce::jmax (0.0, songStart - index * barLength), juce::jlimit (0, 127, juce::roundToInt (note.velocity * 127.0f)) });
    }

    std::vector<Bar> result;

    for (auto& [index, bar] : bars)
        result.push_back (std::move (bar));

    return result;
}

bool hasRole (const Bar& bar, Role role)
{
    return std::any_of (bar.hits.begin(), bar.hits.end(), [role] (const Hit& h) { return getRole (h.pitch) == role; });
}

int countHats (const Bar& bar)
{
    return (int) std::count_if (bar.hits.begin(), bar.hits.end(), [] (const Hit& h) { return getRole (h.pitch) == Role::hat; });
}

const Bar* findGrooveBar (const std::vector<Bar>& bars)
{
    for (auto& bar : bars)
        if (hasRole (bar, Role::kick) && hasRole (bar, Role::snare))
            return &bar;

    return nullptr;
}

bool isKickAndSnare (const std::vector<Bar>& bars)
{
    return findGrooveBar (bars) != nullptr;
}

bool hasHats (const std::vector<Bar>& bars)
{
    for (auto& bar : bars)
        if (hasRole (bar, Role::kick) && hasRole (bar, Role::snare) && countHats (bar) >= minHats)
            return true;

    return false;
}

std::optional<Spread> findVelocitySpread (const std::vector<Bar>& bars)
{
    for (auto* bar : getOrder (bars))
    {
        std::optional<Spread> best;
        std::map<int, Spread> spreads;

        for (auto& hit : bar->hits)
        {
            if (getRole (hit.pitch) == Role::none)
                continue;

            auto found = spreads.find (hit.pitch);

            if (found == spreads.end())
                spreads[hit.pitch] = { hit.pitch, hit.velocity, hit.velocity };
            else
            {
                found->second.lowest = juce::jmin (found->second.lowest, hit.velocity);
                found->second.highest = juce::jmax (found->second.highest, hit.velocity);
            }
        }

        for (auto& [pitch, spread] : spreads)
        {
            const int width = spread.highest - spread.lowest;

            if (width < minVelocitySteps)
                continue;

            if (! best.has_value() || width > best->highest - best->lowest
                || (width == best->highest - best->lowest && rank (getRole (pitch)) < rank (getRole (best->pitch))))
                best = spread;
        }

        if (best.has_value())
            return best;
    }

    return std::nullopt;
}

std::optional<DrumSwing> findDrumSwing (const std::vector<Bar>& bars)
{
    for (auto* bar : getOrder (bars))
    {
        std::optional<DrumSwing> best;

        for (auto& [pitch, offsets] : groupByPitch (*bar))
        {
            const auto found = quantise::findSwing (offsets);

            if (found.found && (! best.has_value() || rank (getRole (pitch)) < rank (getRole (best->pitch))))
                best = DrumSwing { pitch, found };
        }

        if (best.has_value())
            return best;
    }

    return std::nullopt;
}

std::optional<double> findDotted (const std::vector<Bar>& bars)
{
    for (auto* bar : getOrder (bars))
        for (auto& [pitch, offsets] : groupByPitch (*bar))
            for (double pairBeats : { 1.0, 0.5 })
            {
                int dotted = 0, others = 0;
                double sum = 0.0;

                for (double offset : offsets)
                {
                    double place = 0.0;
                    const auto kind = quantise::classifySwing (offset, pairBeats, &place);

                    if (kind == quantise::SwingPlace::other && place > dottedLimit && place <= 0.80)
                    {
                        ++dotted;
                        sum += place;
                    }
                    else if (kind != quantise::SwingPlace::onPair)
                        ++others;
                }

                if (dotted >= 2 && dotted > others)
                    return 100.0 * sum / dotted;
            }

    return std::nullopt;
}

bool hatsMoveInEighths (const std::vector<Bar>& bars)
{
    const Bar* bar = findGrooveBar (bars);

    if (bar == nullptr || countHats (*bar) == 0)
    {
        bar = nullptr;

        for (auto& b : bars)
            if (countHats (b) > 0)
            {
                bar = &b;
                break;
            }
    }

    if (bar == nullptr)
        return false;

    for (auto& hit : bar->hits)
        if (getRole (hit.pitch) == Role::hat && ! isOnGrid (hit.offset, 0.5))
            return false;

    return true;
}

//==============================================================================
static std::vector<Bar> getBeatBars (const Engine& engine, const Progress& progress)
{
    const auto beat = findBeat (engine, progress);
    return beat.clip != nullptr ? getBars (engine, *beat.clip) : std::vector<Bar>();
}

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    const auto bars = getBeatBars (engine, progress);

    switch (step)
    {
        case stepKickSnare: return isKickAndSnare (bars);
        case stepHats:      return hasHats (bars);
        case stepGroove:    return findVelocitySpread (bars).has_value();
        case stepSwing:     return findDrumSwing (bars).has_value();
        default:            break;
    }

    return false;
}

juce::String getDoText (int step)
{
    switch (step)
    {
        case stepKickSnare:
            return "In the drum clip, put the kick (C2, note 36) on beats 1 and 3, and the snare (D2, note 38) on 2 and 4. Click in the piano roll's grid to add a note.";

        case stepHats:
            return "Add the closed hi-hat (F#2, note 42) on every eighth note: 1 & 2 & 3 & 4 &. Set Snap to 1/8 to place them easily.";

        case stepGroove:
            return "Turn on Velocity (V) and drag the hi-hats on the &s down, to about 70, and leave the ones on the beat near 100.";

        case stepSwing:
            return "Click Quantise with swing..., choose 1/8 if your hi-hats play eighths (1/16 for sixteenths), set Swing to about 58% and press Quantise. "
                   "Play it, then try 66%.";

        default: break;
    }

    return {};
}

Action getAction (int step, const Engine& engine, const Progress& progress)
{
    if (! hasDrumTrack (engine))
        return { "Add drums", "Add a track with the Drums sound and an empty clip, open in the piano roll", Fix::none };

    if (step == stepSwing)
    {
        const auto beat = findBeat (engine, progress);

        if (beat.clip != nullptr && ! beat.clip->notes.empty())
            return { "Quantise with swing...",
                     "Open Quantise on the whole drum clip with Swing set. Nothing moves until you press Quantise; Cmd+Z undoes it.", Fix::none };
    }

    return { "Open the beat", "Select the drum clip and open it in the piano roll", Fix::none };
}

Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    if (! hasDrumTrack (engine))
        return { "A drum track plays the Drums sound or DawIt Drums.", Fix::none };

    const auto bars = getBeatBars (engine, progress);
    bool anyNote = false, kickOrSnare = false;

    for (auto& bar : bars)
        for (auto& hit : bar.hits)
        {
            anyNote = true;
            kickOrSnare = kickOrSnare || getRole (hit.pitch) == Role::kick || getRole (hit.pitch) == Role::snare;
        }

    if (anyNote && ! kickOrSnare)
        return { "This clip has no kick or snare yet. On the General MIDI drum map the kick is C2 (note 36) and the snare D2 (note 38); "
                 "a kit with its own map may use other notes, so use the ones that sound like a kick and a snare.", Fix::none };

    if (! progress.hasOutputDevice)
        return { "No audio output is chosen, so you won't hear the beat. Choose one in Audio/MIDI Settings.", Fix::audioSettings };

    if (step == stepSwing && ! isStepDone (step, engine, progress))
    {
        if (hatsMoveInEighths (bars) && std::abs (progress.quantiseGridBeats - 0.25) < 1.0e-6)
            return { "Swing at 1/16 moves only sixteenths, so your eighths stay put. Choose 1/8.", Fix::none };

        if (const auto dotted = findDotted (bars))
            return { juce::String (juce::roundToInt (*dotted)) + "% is a dotted rhythm, sharper than swing: try 55 to 70%.", Fix::none };
    }

    return {};
}

juce::String getDoneNote (int step, const Engine& engine, const Progress& progress)
{
    const auto bars = getBeatBars (engine, progress);

    switch (step)
    {
        case stepKickSnare:
        {
            const auto* bar = findGrooveBar (bars);

            if (bar == nullptr)
                return {};

            int loudest = 0;

            for (auto& hit : bar->hits)
                if (getRole (hit.pitch) == Role::snare)
                    loudest = juce::jmax (loudest, hit.velocity);

            // Ghost notes between the backbeats don't change where the snare falls: count the hits close to the loudest.
            std::vector<double> offsets;

            for (auto& hit : bar->hits)
                if (getRole (hit.pitch) == Role::snare && hit.velocity > loudest - minVelocitySteps
                    && std::none_of (offsets.begin(), offsets.end(), [&hit] (double o) { return std::abs (o - hit.offset) < tolerance; }))
                    offsets.push_back (hit.offset);

            std::sort (offsets.begin(), offsets.end());
            const auto sig = engine.getTimeSignature();
            const auto is = [&offsets] (std::initializer_list<double> expected)
            {
                return offsets.size() == expected.size() && std::equal (offsets.begin(), offsets.end(), expected.begin(), [] (double a, double b) { return std::abs (a - b) < tolerance; });
            };

            if (sig.numerator == 4 && sig.denominator == 4)
            {
                if (is ({ 1.0, 3.0 }))
                    return "Snare on 2 and 4: the backbeat";

                if (is ({ 2.0 }))
                    return "Snare on 3: a half-time feel";
            }

            juce::StringArray counts;

            for (double offset : offsets)
                counts.add (countText (engine, offset));

            return "Snare on " + joinCounts (counts);
        }

        case stepHats:
        {
            const Bar* bar = nullptr;

            for (auto* candidate : getOrder (bars))
                if (hasRole (*candidate, Role::kick) && hasRole (*candidate, Role::snare) && countHats (*candidate) >= minHats)
                {
                    bar = candidate;
                    break;
                }

            if (bar == nullptr)
                return {};

            std::vector<double> starts;
            bool allRides = true;

            for (auto& hit : bar->hits)
                if (getRole (hit.pitch) == Role::hat)
                {
                    starts.push_back (hit.offset);
                    allRides = allRides && hit.pitch == 51;
                }

            const int count = (int) starts.size();
            const auto noun = juce::String (allRides ? "ride" : "hi-hat") + (count == 1 ? "" : "s");
            const bool all1 = std::all_of (starts.begin(), starts.end(), [] (double s) { return isOnGrid (s, 1.0); });
            const bool all2 = std::all_of (starts.begin(), starts.end(), [] (double s) { return isOnGrid (s, 0.5); });
            const bool all4 = std::all_of (starts.begin(), starts.end(), [] (double s) { return isOnGrid (s, 0.25); });
            const auto sig = engine.getTimeSignature();
            const bool fourFour = sig.numerator == 4 && sig.denominator == 4;
            juce::String spacing;

            if (all1)
                spacing = fourFour && count == 4 ? "quarter notes, 1 2 3 4" : "quarter notes";
            else if (all2)
                spacing = fourFour && count == 8 ? "eighth notes, 1 & 2 & 3 & 4 &" : "eighth notes";
            else if (all4)
                spacing = "sixteenth notes";

            return juce::String (count) + " " + noun + (spacing.isNotEmpty() ? ": " + spacing : juce::String());
        }

        case stepGroove:
        {
            const auto spread = findVelocitySpread (bars);
            return spread.has_value() ? getDrumName (spread->pitch) + " from " + juce::String (spread->lowest) + " to " + juce::String (spread->highest) : juce::String();
        }

        case stepSwing:
        {
            const auto swing = findDrumSwing (bars);

            if (! swing.has_value())
                return {};

            const auto name = getDrumName (swing->pitch);
            const bool hats = getRole (swing->pitch) == Role::hat && swing->pitch != 51;
            const auto who = hats ? juce::String ("Hi-hats swing their") : "The " + name.toLowerCase() + " swings its";
            return who + (swing->found.pairBeats > 0.75 ? " eighths" : " sixteenths") + " at about " + juce::String (juce::roundToInt (swing->found.percent)) + "%";
        }

        default: break;
    }

    return {};
}

quantise::Settings getQuantiseStart (const Engine& engine, const Progress& progress)
{
    const double grid = hatsMoveInEighths (getBeatBars (engine, progress)) ? 0.5 : 0.25;
    return { grid, 1.0, false, startSwing };
}

juce::String getShowMeText (int step, bool quantiseDialogOpen)
{
    switch (step)
    {
        case stepKickSnare: return "Click in this grid to add a note.";
        case stepHats:      return "Choose 1/8 here, then click to add the hi-hats.";
        case stepGroove:    return "Turn Velocity on here, then drag a note's bar up or down.";
        case stepSwing:     return quantiseDialogOpen ? "Set Swing here, then press Quantise." : "Click Quantise... here, then set Swing.";
        default: break;
    }

    return {};
}
} // namespace daw::gen::buildbeat
