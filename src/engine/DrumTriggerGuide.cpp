#include "DrumTriggerGuide.h"

#include <algorithm>
#include <cmath>

#include "BuiltInDrums.h"

namespace daw::triggerguide
{
namespace
{
    using analysis::hits::Drum;

    juce::String describeNumber (double value)
    {
        return std::abs (value - std::round (value)) < 0.05 ? juce::String ((int) std::lround (value)) : juce::String (value, 1);
    }

    juce::String drumWord (Drum drum)
    {
        switch (drum)
        {
            case Drum::kick:  return "kick";
            case Drum::snare: return "snare";
            case Drum::hihat: return "hi-hat";
        }

        return {};
    }

    int count (const Part& part, Drum drum)
    {
        return (int) std::count_if (part.hits.begin(), part.hits.end(), [drum] (const PartHit& h) { return h.drum == drum; });
    }

    /** The pulse line for `perBar` hits a bar, or empty when it is not near 4, 8 or 16. */
    juce::String getPulseName (double perBar, int& target)
    {
        for (const int t : { 4, 8, 16 })
        {
            if (std::abs (perBar - (double) t) <= 0.1 * t)
            {
                target = t;
                return t == 4 ? "quarter notes" : (t == 8 ? "eighth notes" : "sixteenth notes");
            }
        }

        return {};
    }
}

//==============================================================================
juce::String getTopicName (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItIs:       return "What a drum trigger is";
        case Topic::transients:     return "Transients";
        case Topic::findingHits:    return "Finding the hits";
        case Topic::velocity:       return "How hard: velocity";
        case Topic::whichDrum:      return "Kick, snare or hi-hat";
        case Topic::falseHits:      return "False and missed hits";
        case Topic::grid:           return "On the grid or as played";
        case Topic::replaceOrLayer: return "Replace or layer";
        case Topic::yourClip:       return "Your clip";
    }

    return {};
}

juce::String getTopicSummary (Topic topic)
{
    switch (topic)
    {
        case Topic::whatItIs:       return "Hits in a recording become notes";
        case Topic::transients:     return "The fast rise at the start of a hit";
        case Topic::findingHits:    return "A sensitivity and a minimum gap";
        case Topic::velocity:       return "Louder hits become higher velocities";
        case Topic::whichDrum:      return "Telling drums apart by their frequencies";
        case Topic::falseHits:      return "Bleed, cymbals, rolls and flams";
        case Topic::grid:           return "Snap with a strength, or keep the feel";
        case Topic::replaceOrLayer: return "And why the two must line up";
        case Topic::yourClip:       return "What was found, and what to try";
    }

    return {};
}

juce::String explain (Topic topic, const Part* part)
{
    switch (topic)
    {
        case Topic::whatItIs:
            return "A drum trigger listens to a recording of drums and writes a note every time something is hit. Producers use it for three things:\n\n"
                   "- Replace a sound: a snare that was recorded thin or dull plays a better snare instead, hit for hit.\n"
                   "- Layer a sound: a second drum plays under the original, such as a deep kick under a kick recorded in a small room. "
                   "The room stays; the new kick adds weight.\n"
                   "- Turn a loop into a part: a drum loop or a drum stem becomes MIDI notes you can see, edit and play with any kit.\n\n"
                   "In DawIt, right-click an audio clip and choose Hits to MIDI.... The hits become notes in a new MIDI track below, at the "
                   "same times, with the velocity from how loud each hit was, played by DawIt Drums. The original audio stays where it is, "
                   "so you can keep it, mute it, or play both.\n\n"
                   "A drum trigger only asks when something was hit and how hard. It doesn't hear pitch. For a bass, a voice or a melody, "
                   "use Convert to MIDI... instead: it finds pitched notes, and drums have no clear pitch, so it isn't made for them.\n\n"
                   "The picture shows a two-bar drum loop at 120 BPM: the waveform with a marker on every hit, and below it the notes the hits become.";

        case Topic::transients:
            return "A transient is the fast rise at the start of a sound. When a stick hits a snare, the level jumps from almost nothing to its "
                   "loudest within a few milliseconds, then dies away over a tenth of a second or more. A kick's beater, a hand clap and a "
                   "plucked string do the same.\n\n"
                   "That rise is what a trigger looks for. It's the clearest sign that something new has started: a sound that's dying away "
                   "only gets quieter, and a held note stays about level.\n\n"
                   "A hit's time is the start of its rise, not its peak. DawIt first finds the hit in steps of 5.8 ms, then looks back for the "
                   "first sample where the sound starts to change fast: where the step from one sample to the next reaches a tenth of the hit's "
                   "biggest step (20 dB under it). It watches the change rather than the level so that a hi-hat on top of a kick that's still "
                   "ringing gets its own start. At 44.1 kHz one sample lasts 0.023 ms, far finer than anyone can hear.\n\n"
                   "Not every sound has a sharp transient. A bowed cello, a pad or a softly sung note fades in, and where it starts is a matter "
                   "of opinion. Triggers work best on drums and other things that are struck.\n\n"
                   "The picture shows the first 60 ms of a snare hit: the rise, the peak, and the marker at the start of the rise.";

        case Topic::findingHits:
            return "DawIt listens to the clip in 40 frequency bands and asks, every 5.8 ms, how much each band got louder. Added up, that's a "
                   "rise curve: flat while a sound holds or fades, with a spike at every hit. Finding note starts this way is called spectral "
                   "flux, and it's the usual first step in onset detection. A second hit only a few milliseconds after another can hide in "
                   "that curve, so DawIt also looks, millisecond by millisecond, for sudden jumps in the sound itself.\n\n"
                   "Two settings decide which spikes count as hits:\n\n"
                   "- Sensitivity is how quiet a hit may be, measured from the loudest hit in the clip. At -30 dB a hit counts if its peak is no "
                   "more than 30 dB under the loudest. Raise it (towards -12 dB) when markers appear on bleed or noise; lower it (towards "
                   "-45 dB) when soft hits are missed.\n"
                   "- Minimum gap is how long after a hit nothing new counts. At 30 ms, a second spike 12 ms after a hit is taken as part of it: "
                   "a kick's beater bouncing back, or a rattle of the snare. Drum modules have the same setting, often called mask time or "
                   "retrigger: Roland's runs from 0 to 64 ms. Keep it shorter than the fastest real notes: sixteenth notes at 120 BPM are "
                   "125 ms apart, and 32nd notes at 240 BPM are 31 ms apart.\n\n"
                   "Any marker can be fixed by hand: drag it to move it, click an empty spot to add a hit, or select one and press Delete.\n\n"
                   "The picture shows the example loop's level with the sensitivity line, the hits that count, and a beater bounce 12 ms "
                   "after a kick, which the gap keeps out.";

        case Topic::velocity:
            return "Every MIDI note has a velocity from 1 to 127: how hard it's played. A trigger takes it from how loud each hit was, compared "
                   "with the loudest hit of the same drum. The loudest snare gets 127; quieter snares get less.\n\n"
                   "How much less is the velocity curve:\n\n"
                   "- Natural: a hit 12 dB quieter gets velocity 64, and one 20 dB quieter gets 40. This is the reverse of the curve the DLS "
                   "standard for sample players uses to turn velocity into level (40 x log10 (127 / velocity) dB quieter), so a player that "
                   "follows it plays each hit back about as loud as it was.\n"
                   "- Gentle: softer hits keep more velocity. 20 dB quieter gets 59.\n"
                   "- Strong: the differences grow. 20 dB quieter gets 13, so ghost notes stand clearly apart.\n"
                   "- Same for all: every hit gets 100, the way a drum machine plays.\n\n"
                   "Velocity isn't loudness itself: what it does is up to the instrument. The MIDI standard leaves the curve to each instrument, "
                   "and few instruments follow the DLS curve exactly. DawIt Drums turns velocity into level with its own Velocity knob; a sampled kit may "
                   "switch to a recording of a softer hit. So the same notes can sound more or less even on different kits.\n\n"
                   "The picture shows the four curves from 0 to -40 dB, with the example's hits as dots.";

        case Topic::whichDrum:
            return "With Sort by drum, each hit becomes the drum whose part of the spectrum gets most of the energy the hit adds in its "
                   "first 50 ms:\n\n"
                   "- Kick: below 150 Hz. Its tone usually sits between 40 and 70 Hz, below most bass notes. Written as note 36.\n"
                   "- Snare: 150 Hz to 6 kHz. A body around 150 to 250 Hz and the noise of the snare wires above it. Written as note 38.\n"
                   "- Hi-hat: above 6 kHz, where a hi-hat's metal is. Written as note 42, the closed hi-hat.\n\n"
                   "These are General MIDI's drum notes, so the part plays a kick, a snare and a hi-hat on DawIt Drums and on almost any drum kit.\n\n"
                   "Added energy means the spectrum of the 50 ms after the hit minus that of the 50 ms before it. A kick still ringing from "
                   "the beat before is in both, so it drops out, and a hi-hat or snare played over it isn't taken for a kick.\n\n"
                   "It's a rule of thumb, and it has limits. One hit gets one note: when the kick and the hi-hat hit together on beat 1, the one "
                   "with more energy (usually the kick) wins, and that hi-hat is missing from the part. Toms, claps, rides and crashes have no note "
                   "of their own here: a tom usually sorts as a kick or a snare, a crash as a hi-hat. Fix them with a right-click on the marker "
                   "before you make the track, or later in the piano roll.\n\n"
                   "Without Sort by drum every hit becomes the one drum you choose, which is right for a single drum such as a snare recording.\n\n"
                   "The picture shows the spectrum of a kick, a snare and a hi-hat from the example loop, with the borders at 150 Hz and 6 kHz "
                   "and how much of the energy each hit adds falls in each part.";

        case Topic::falseHits:
            return "A trigger hears level, not drums. Anything loud enough that rises counts as a hit; anything too quiet doesn't.\n\n"
                   "- Bleed: every microphone on a kit also hears the other drums (this is also called spill). In a snare microphone the kick can "
                   "be as loud as the snare's own ghost notes, the very soft strokes between the backbeats. Raise Sensitivity until the bleed's "
                   "markers go, and add ghost notes back by hand if they matter. A stem from Split into stems... usually has less bleed than a "
                   "microphone, but rarely none.\n"
                   "- Cymbals and hi-hats ring for a long time and wash over everything. Their ring doesn't rise, so it seldom makes a hit by "
                   "itself, but it can hide quiet hits under it. Many engineers don't trigger hi-hats at all: they keep the recording, or program "
                   "the part.\n"
                   "- Rolls and flams: a flam is two strokes so close together they sound like one fat hit; a roll is a fast run of soft strokes. "
                   "A long Minimum gap merges them into one; a short one finds each stroke, but may also find a kick's bounce. Snare rolls are "
                   "among the hardest things to trigger well.\n"
                   "- All the same: when every hit plays the same sample, fast runs can sound like a machine gun. Velocity helps; kits that switch "
                   "between several recordings of each hit help more.\n\n"
                   "Where the detector can't tell, look at the waveform and move, add or delete markers yourself.\n\n"
                   "The picture shows a snare recording with snare hits at 0 dB, two ghost notes at -22 dB and the kick bleeding through at -26 dB. "
                   "At -30 dB everything counts, the bleed too; at -24 dB the bleed goes and the ghost notes stay; at -18 dB only the backbeats "
                   "are left.";

        case Topic::grid:
            return "Hits are found to the sample, in seconds. DawIt's notes sit on the timeline in beats, so each time goes through the song's "
                   "tempo: at 120 BPM a beat lasts 0.5 s, so a hit 1.5 s into a clip that starts on bar 3 (beat 8) lands on beat 11. Tempo "
                   "changes are followed.\n\n"
                   "Then you choose:\n\n"
                   "- Keep feel leaves every hit where it was played. A drummer is never exactly on the grid, and the small pushes and drags "
                   "are part of the groove. This is where DawIt starts.\n"
                   "- Snap to the grid moves each hit towards the nearest grid line (1/16 is a sixteenth note). Strength says how far: 100% puts "
                   "it on the line, 50% halfway. At 120 BPM a hit 30 ms late (0.06 beats) ends up 15 ms late at 50%, and on the line at 100%. "
                   "Many producers use 50 to 90% to tidy a loose part and keep some of its life; how much is a matter of taste.\n\n"
                   "Snapping only helps when the song's tempo matches the recording. If the hits drift further from the grid bar after bar, set "
                   "the tempo first (Detect tempo... in the clip's menu).\n\n"
                   "The picture shows eight played hits and where they go at 0%, 50% and 100%.";

        case Topic::replaceOrLayer:
            return "The new track can be used in two ways:\n\n"
                   "- Layer: the original keeps playing and the new drum plays with it. The room, the player's touch and the new drum's weight "
                   "all stay. A common use: a deep kick under a kick recorded in a small room.\n"
                   "- Replace: the original is muted and only the new drum plays. Choose Mute the clip in Hits to MIDI, or mute it later from the "
                   "clip's right-click menu.\n\n"
                   "When you layer, the two must line up. A kick's 60 Hz wave rises and falls every 16.7 ms. Play two copies exactly together "
                   "and they add up: twice the swing, 6 dB louder. Move one by half a cycle, 8.3 ms, and one pushes while the other pulls: the "
                   "low end cancels and the kick goes thin. Smaller shifts cancel higher frequencies: two copies 1 ms apart cancel at 500 Hz, "
                   "1.5 kHz, 2.5 kHz and so on, a hollow sound called comb filtering.\n\n"
                   "DawIt puts every note at the start of the hit's rise, so a drum that sounds at once lines up with the original. Some "
                   "samples begin with a little silence, and some instruments answer a few milliseconds late. To check, zoom in on both: they "
                   "should start together and swing the same way first. If the layer sounds thinner than either drum alone, move the notes by a "
                   "millisecond or two, or flip the polarity of one with a plugin that can.\n\n"
                   "The picture shows two 60 Hz kick waves: in line, and 8.3 ms apart, with what you hear when they add up.";

        case Topic::yourClip:
        {
            if (part == nullptr || part->hits.empty())
                return "Right-click an audio clip with drums and choose Hits to MIDI..., or select a drum part, and this page looks at it.";

            juce::StringArray lines;

            for (const auto& line : analyse (*part))
                lines.add (line);

            return lines.joinIntoString ("\n\n");
        }
    }

    return {};
}

//==============================================================================
double getBeatsFromMs (double ms, double bpm)
{
    return ms * bpm / 60000.0;
}

double getMsFromBeats (double beats, double bpm)
{
    return bpm > 0.0 ? beats * 60000.0 / bpm : 0.0;
}

double getPeriodMs (double hz)
{
    return hz > 0.0 ? 1000.0 / hz : 0.0;
}

double getSumGainDb (double hz, double delayMs)
{
    const double amplitude = std::abs (2.0 * std::cos (juce::MathConstants<double>::pi * hz * delayMs / 1000.0));
    return amplitude < 1.0e-6 ? -120.0 : juce::jmax (-120.0, 20.0 * std::log10 (amplitude));
}

double getNotchHz (double delayMs, int n)
{
    return delayMs > 0.0 ? (2.0 * n + 1.0) / (2.0 * delayMs / 1000.0) : 0.0;
}

int getCurveVelocity (analysis::hits::VelocityCurve curve, double dbBelowLoudest)
{
    return analysis::hits::getVelocity (curve, dbBelowLoudest);
}

const std::vector<SnareStroke>& getSnareTrack()
{
    using Kind = SnareStroke::Kind;
    static const std::vector<SnareStroke> track {
        { 0.0, -26.0, Kind::bleed },    { 1.0, 0.0, Kind::backbeat }, { 1.75, -22.0, Kind::ghost },
        { 2.0, -26.0, Kind::bleed },    { 2.5, -26.0, Kind::bleed },  { 3.0, 0.0, Kind::backbeat },
        { 3.5, -22.0, Kind::ghost }
    };
    return track;
}

int countKept (double sensitivityDb)
{
    const auto& track = getSnareTrack();
    return (int) std::count_if (track.begin(), track.end(), [sensitivityDb] (const SnareStroke& s) { return s.levelDb >= sensitivityDb; });
}

const std::vector<double>& getGridOffsetsMs()
{
    static const std::vector<double> offsets { 12.0, -8.0, 30.0, 4.0, -20.0, 16.0, -4.0, 24.0 };
    return offsets;
}

double getSnappedOffsetMs (double offsetMs, double strength)
{
    return offsetMs * (1.0 - juce::jlimit (0.0, 1.0, strength));
}

//==============================================================================
juce::String describeCounts (const Part& part)
{
    const int n = (int) part.hits.size();

    if (n == 0 || part.lengthBars <= 0.0)
        return {};

    const double perBar = n / part.lengthBars;
    auto text = juce::String (n) + (n == 1 ? " hit in " : " hits in ") + describeNumber (part.lengthBars)
                + (std::abs (part.lengthBars - 1.0) < 1.0e-9 ? " bar (" : " bars (") + describeNumber (perBar) + " a bar)";

    if (part.showDrums)
    {
        juce::StringArray drums;

        for (const auto drum : { Drum::kick, Drum::snare, Drum::hihat })
            if (const int c = count (part, drum); c > 0)
                drums.add (juce::String (c) + " " + drumWord (drum));

        if (! drums.isEmpty())
            text << ": " << drums.joinIntoString (", ");
    }

    return text + ".";
}

std::vector<juce::String> analyse (const Part& part)
{
    std::vector<juce::String> lines;
    const int n = (int) part.hits.size();

    if (n == 0 || part.lengthBars <= 0.0)
        return lines;

    // 1. The counts.
    lines.push_back (describeCounts (part));

    // 2. A steady pulse, for all hits and for the hi-hats alone.
    {
        int target = 0;
        const auto name = getPulseName (n / part.lengthBars, target);

        if (name.isNotEmpty())
            lines.push_back ("That's " + juce::String (target) + " hits a bar on average, as many as there are " + name + ": a steady pulse.");

        const int hats = count (part, Drum::hihat);

        if (part.showDrums && hats >= 4)
        {
            int hatTarget = 0;
            const auto hatName = getPulseName (hats / part.lengthBars, hatTarget);

            if (hatName.isNotEmpty())
                lines.push_back ("The hi-hat plays " + juce::String (hatTarget) + " a bar, as many as there are " + hatName + ": a steady pulse.");
        }
    }

    // 3. The backbeat.
    {
        int snares = 0, backbeat = 0;

        for (const auto& h : part.hits)
        {
            if (h.drum != Drum::snare)
                continue;

            ++snares;
            const double inBar = std::fmod (h.beat, 4.0);

            if (std::abs (inBar - 1.0) <= 0.125 || std::abs (inBar - 3.0) <= 0.125)
                ++backbeat;
        }

        if (snares >= 4 && backbeat * 4 >= snares * 3)
            lines.push_back ("Most snare hits land on beats 2 and 4: the backbeat of rock and pop.");
    }

    // 4. Pairs of the same drum closer than 30 ms.
    {
        int pairs = 0;

        for (const auto drum : { Drum::kick, Drum::snare, Drum::hihat })
        {
            std::vector<double> beats;

            for (const auto& h : part.hits)
                if (h.drum == drum)
                    beats.push_back (h.beat);

            std::sort (beats.begin(), beats.end());

            for (size_t i = 1; i < beats.size(); ++i)
                if (getMsFromBeats (beats[i] - beats[i - 1], part.bpm) < closePairMs)
                    ++pairs;
        }

        if (pairs > 0)
            lines.push_back (juce::String (pairs) + (pairs == 1 ? " pair of hits is" : " pairs of hits are")
                             + " less than 30 ms apart. That's a flam if the drummer played one, or one hit found twice: try a Minimum gap of 50 ms.");
    }

    // 5. Even velocities (needs a few hits to mean anything).
    if (! part.sameForAll && n >= 4)
    {
        int lowest = 127, highest = 1;

        for (const auto& h : part.hits)
        {
            lowest = juce::jmin (lowest, h.velocity);
            highest = juce::jmax (highest, h.velocity);
        }

        if (highest - lowest < 8)
            lines.push_back ("Every hit has about the same velocity (" + (lowest == highest ? "all " + juce::String (lowest) : juce::String (lowest) + " to " + juce::String (highest))
                             + "). If the drummer played accents and ghost notes, try the Strong curve; for a machine-like part this is right.");
    }

    // 6. Quiet snares.
    {
        const int quiet = (int) std::count_if (part.hits.begin(), part.hits.end(), [] (const PartHit& h) { return h.drum == Drum::snare && h.velocity <= 40; });

        if (quiet >= 1)
            lines.push_back (juce::String (quiet) + (quiet == 1 ? " quiet snare hit" : " quiet snare hits")
                             + " (velocity 40 or less): ghost notes, the soft strokes between the backbeats. Keep them if you can hear them in the original.");
    }

    // 7. How far from the 1/16 grid.
    if (n >= 8)
    {
        double total = 0.0;

        for (const auto& h : part.hits)
            total += std::abs (h.beat - std::round (h.beat / 0.25) * 0.25);

        const double ms = getMsFromBeats (total / n, part.bpm);
        const auto number = juce::String ((int) std::lround (ms));

        if (ms < 3.0)
            lines.push_back ("The hits sit on the 1/16 grid (on average " + number + " ms off): played to a click, programmed or snapped.");
        else if (ms <= 15.0)
            lines.push_back ("On average " + number + " ms off the 1/16 grid: tight, with a human feel. Keep feel keeps it.");
        else
            lines.push_back ("On average " + number + " ms off the 1/16 grid: a loose part, a shuffle or triplet feel, or a song tempo that doesn't match the recording. "
                             "Check the tempo (Detect tempo...) before snapping.");
    }

    return lines;
}

Part getPartFromNotes (const std::vector<DrumNote>& notes, double lengthBeats, double bpm)
{
    Part part;
    part.bpm = bpm;
    part.lengthBars = lengthBeats / 4.0;

    for (const auto& note : notes)
    {
        const auto drum = drums::getDrumForNote (note.pitch);

        if (! drum.has_value())
            continue;

        PartHit hit;
        hit.beat = note.startBeat;
        hit.velocity = juce::jlimit (1, 127, note.velocity);

        if (*drum == drums::Drum::kick)
            hit.drum = Drum::kick;
        else if (*drum == drums::Drum::snare)
            hit.drum = Drum::snare;
        else if (*drum == drums::Drum::closedHat || *drum == drums::Drum::openHat)
            hit.drum = Drum::hihat;
        else
            continue;

        part.hits.push_back (hit);
    }

    std::sort (part.hits.begin(), part.hits.end(), [] (const PartHit& a, const PartHit& b) { return a.beat < b.beat; });
    return part;
}

//==============================================================================
juce::String getTooltip (Tip tip)
{
    switch (tip)
    {
        case Tip::sensitivity:  return "How quiet a hit may be and still count, in dB below the loudest hit. Raise it (towards -12 dB) if markers appear on bleed or noise; lower it (towards -45 dB) if soft hits are missed.";
        case Tip::minimumGap:   return "After a hit, nothing new counts for this long, so one hit isn't found twice (a beater bouncing back, a rattle). 30 ms suits most parts; shorten it for fast rolls.";
        case Tip::writeAs:      return "Sort by drum: kick (36), snare (38) or hi-hat (42) by where each hit's energy is. Or every hit as one drum, for a recording of a single drum.";
        case Tip::velocity:     return "How loudness becomes velocity. Natural: 12 dB quieter is velocity 64. Gentle keeps soft hits louder, Strong makes them softer, Same for all gives every hit 100.";
        case Tip::keepFeel:     return "Leaves every hit where it was played, so the groove stays.";
        case Tip::snapTo:       return "Moves the hits towards the nearest line of this grid.";
        case Tip::strength:     return "How far each hit moves towards the grid: 100% onto the line, 50% halfway, 0% not at all.";
        case Tip::keepPlaying:  return "Layer: the original audio keeps playing with the new drums.";
        case Tip::muteClip:     return "Replace: mutes the original clip, so only the new drums play. Unmute it any time from its right-click menu.";
        case Tip::listen:       return "Plays the clip with the hits on top, from the start or from where you clicked on the ruler. Click again to stop.";
        case Tip::hear:         return "What Listen plays: the original, the hits as DawIt Drums will play them, or both.";
        case Tip::makeTrack:    return "Puts the hits in a new MIDI track below, with DawIt Drums. The original stays.";
        case Tip::help:         return "How drum triggers work: transients, sensitivity, velocity, bleed and lining up a layer.";
    }

    return {};
}
} // namespace daw::triggerguide
