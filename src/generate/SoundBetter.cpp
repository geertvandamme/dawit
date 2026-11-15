#include "SoundBetter.h"

#include "engine/TrackNameWords.h"
#include "engine/BuiltInReverb.h"
#include "engine/ReverbKinds.h"
#include "engine/Routing.h"

#include <algorithm>
#include <cmath>

namespace daw::gen::soundbetter
{
using guides::Guide;

namespace
{
    constexpr double epsilon = 1.0e-6;

    /** "4", "3.5": a number with no trailing zero. */
    juce::String trimmed (double value, int decimals = 1)
    {
        auto text = juce::String (value, decimals);

        if (text.containsChar ('.'))
            text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

        return text;
    }

    juce::String oneDecimal (double value) { return juce::String (value, 1); }

    juce::String nameOrDefault (const Track* track) { return track != nullptr ? track->name : juce::String ("your track"); }
}

std::vector<Step> getSteps()
{
    return {
        { "Pick a track and listen", "Choose one track and hear it in the song",
          "Effects are for problems you can hear, so listen before you reach for one. Listen in the song, not only on its own: a guitar that sounds thin alone may be just right with the bass. "
          "DawIt reads the track's name to suggest starting points, so a track called Vocal, Bass or Kick gets the settings that usually suit it.",
          { { Guide::eq, "Common problems, in words" }, { Guide::eq, "Where instruments sit" } } },
        { "EQ to make room", "Cut what's in the way",
          "A loud sound hides a quieter one close to it in frequency. This is called masking, and it's why good tracks can sound muddy together: the kick and the bass both want 50 to 100 Hz, a voice and the guitars meet around 2 to 4 kHz. "
          "EQ gives each sound its own place. Most engineers cut before they boost: taking 3 dB of mud out at 300 Hz makes a voice clearer, much as adding 3 dB at 3 kHz would, but without making it louder or harsher. "
          "A low cut on a track that doesn't need the lows leaves room for the kick and the bass. Starting points are typical settings, not rules: every recording differs, so trust your ears.",
          { { Guide::eq, "Making room: masking" }, { Guide::eq, "Cut narrow, boost wide" } } },
        { "Compression to even out", "Turn down the loudest parts",
          "A compressor turns a sound down while it's loud and leaves it alone while it's quiet, like a hand on the fader that's never late. A singer is louder on some words than others, and a bass player hits some notes harder; evened out, the track sits in one place in the mix. "
          "Typical amounts for the loudest parts are 3 to 6 dB on a vocal or a bass and 1 to 2 dB on a whole mix; 10 dB or more is an effect. DawIt adds it after the EQ. "
          "Many engineers cut before the compressor, so it doesn't react to rumble or mud, and shape the tone after it, but there's no fixed rule.",
          { { Guide::compressor, "Turning down the loud parts" }, { Guide::compressor, "How much, for what" } } },
        { "Reverb on a send", "Give it a space shared with the song",
          "A reverb on a return is shared: every track that sends to it sounds as if it were played in the same room, and each track's send decides how much it gets. "
          "The track itself plays on as before; the send only adds a copy. That's why the reverb on a return is set to 100% wet. "
          "Use less than you think: turn the send up until you hear the reverb clearly, then back until you just stop noticing it.",
          { { Guide::routing, "Sends and returns" }, { Guide::reverb, "Reverb in a mix" } } },
        { "Compare with the effect off", "Is it really better?",
          "Of two versions, the louder one tends to sound better, even when it's only about 1 dB louder, because the ear hears more bass and treble when it's loud. "
          "So a boost or a compressor's makeup can seem to help just because the track got louder. Match level evens out the loudness, so switching off and on compares only what the effect does. "
          "Switch quickly: the ear's detailed memory of a sound fades within a few seconds. If the track sounds better without an effect, leave it off: that's a good result too.",
          { { Guide::gainStaging, "Louder sounds better" }, { Guide::compressor, "Too much" } } }
    };
}

juce::String getTitle() { return "Make a track sound better"; }

juce::String getIntro()
{
    return "Five steps to a track that sits better in your song: pick one track and listen, make room for it with EQ, even out its level with a compressor, "
           "give it space with a shared reverb, then compare it all with the effects off. Do each step in the main window: the tick appears by itself.";
}

juce::String getHelpDescription() { return "Pick a track, give it an EQ, a compressor and a shared reverb, then compare with them off, in five steps"; }

juce::String getCompletionText()
{
    return "Your track has been through the main tools of mixing: EQ to make room, compression to even it out and a shared reverb for space, each checked with it off. "
           "Try it on another track, starting with the vocal or the bass, and send more tracks to the same reverb so they share one room.";
}

int clampStep (int stored) noexcept { return stored >= 0 && stored < numSteps ? stored : 0; }

//==============================================================================
static bool isOn (const PluginSlot& slot) { return ! slot.bypassed.load(); }

static bool isOfEffect (const PluginSlot& slot, Effect effect)
{
    if (slot.instance == nullptr)
        return false;

    switch (effect)
    {
        case Effect::eq:         return dynamic_cast<BuiltInEq*> (slot.instance.get()) != nullptr;
        case Effect::compressor: return dynamic_cast<BuiltInCompressor*> (slot.instance.get()) != nullptr;
        case Effect::reverb:     return dynamic_cast<BuiltInReverb*> (slot.instance.get()) != nullptr;
    }

    return false;
}

static juce::String getEffectName (const PluginSlot& slot)
{
    if (isOfEffect (slot, Effect::eq))         return "EQ";
    if (isOfEffect (slot, Effect::compressor)) return "Compressor";
    return "Reverb";
}

PluginSlot* findEffect (const Track& track, Effect effect, bool preferOn)
{
    PluginSlot* first = nullptr;

    for (auto& slot : track.plugins)
    {
        if (! isOfEffect (*slot, effect))
            continue;

        if (! preferOn || isOn (*slot))
            return slot.get();

        if (first == nullptr)
            first = slot.get();
    }

    return first;
}

static bool isSoundTrack (const Track& track) { return track.isAudio() || track.isMidi(); }

const Track* findChosenTrack (const Engine& engine, const Track* selected, const Track* remembered)
{
    for (auto* candidate : { selected, remembered })
        if (candidate != nullptr && engine.indexOf (candidate) >= 0 && isSoundTrack (*candidate))
            return candidate;

    return nullptr;
}

const Track* findChosenTrack (const Engine& engine, const Progress& progress)
{
    return findChosenTrack (engine, progress.selected, progress.remembered);
}

bool hasTrackWithSound (const Engine& engine)
{
    for (auto& t : engine.getTracks())
        if (isSoundTrack (*t) && ! t->clips.empty())
            return true;

    return false;
}

bool isHeardAt (const Engine& engine, const Track& track, double beat)
{
    if (routing::getSilence (track, engine.getTracks()) != routing::Silence::none)
        return false;

    const auto& tempo = engine.getTempoMap();

    for (auto& clip : track.clips)
        if (! clip->muted && clip->startBeat <= beat && beat < clip->getEndBeat (tempo))
            return true;

    return false;
}

double getFirstClipBeat (const Track& track)
{
    double first = -1.0;

    for (auto& clip : track.clips)
        if (! clip->muted && (first < 0.0 || clip->startBeat < first))
            first = clip->startBeat;

    return first;
}

//==============================================================================
Source getSource (const Track& track)
{
    switch (verb::kinds::getSource (track.name))
    {
        case verb::kinds::Source::vocal:    return Source::vocal;
        case verb::kinds::Source::snare:    return Source::snare;
        case verb::kinds::Source::kick:     return Source::kick;
        case verb::kinds::Source::drums:    return Source::drums;
        case verb::kinds::Source::bass:     return Source::bass;
        case verb::kinds::Source::guitar:   return Source::guitar;
        case verb::kinds::Source::keys:     return Source::keys;
        case verb::kinds::Source::ensemble: return Source::ensemble;
        case verb::kinds::Source::unknown:  break;
    }

    if (track.usesBuiltInSynth())
    {
        switch (track.builtInSound.load())
        {
            case BuiltInSound::piano:
            case BuiltInSound::organ:      return Source::keys;
            case BuiltInSound::bass:       return Source::bass;
            case BuiltInSound::pad:        return Source::ensemble;
            case BuiltInSound::drums:
            case BuiltInSound::percussion: return Source::drums;
            case BuiltInSound::lead:
            case BuiltInSound::pluck:      break;
        }
    }

    return Source::other;
}

StartingPoints getStartingPoints (const Engine& engine, const Track& track)
{
    bool songHasVocal = false;

    for (auto& t : engine.getTracks())
        if (t.get() != &track && isSoundTrack (*t) && getSource (*t) == Source::vocal)
            songHasVocal = true;

    const juce::String makeRoom = songHasVocal ? "Make room for a vocal" : juce::String();
    const auto words = trackname::getWords (track.name);
    const auto has = [&words] (std::initializer_list<const char*> list) { return trackname::nameHasWord (words, list); };

    switch (getSource (track))
    {
        case Source::vocal:    return { "Vocal", has ({ "backing", "bv", "bvs", "harmony" }) ? "Backing vocals" : "Lead vocal" };
        case Source::kick:     return { "Kick drum", "Kick drum" };
        case Source::snare:    return { "Snare drum", "Snare drum" };
        case Source::drums:    return { "Drum bus", "Drum bus" };
        case Source::bass:     return { "Bass", "Bass" };
        case Source::guitar:
        {
            const bool acoustic = has ({ "acoustic" });
            return { acoustic ? "Acoustic guitar" : makeRoom, acoustic ? "Acoustic guitar" : "Electric guitar" };
        }
        case Source::keys:     return { songHasVocal ? makeRoom : "Piano", "Piano" };
        case Source::ensemble: return { makeRoom, "Pad or strings" };
        case Source::other:    break;
    }

    return { makeRoom, "Neutral start" };
}

//==============================================================================
EqAnalysis analyseBands (const std::array<eq::Band, eq::numBands>& bands)
{
    constexpr double rate = 48000.0;
    const auto sumAt = [&] (double hz)
    {
        double db = 0.0;

        for (auto& b : bands)
            db += eq::getResponseDb (b, hz, rate);

        return db;
    };

    EqAnalysis result;
    double lowest = 0.0, highest = 0.0, lowestHz = minCutHz, highestHz = minCutHz;

    // 1/24 octave steps from 40 Hz to 16 kHz.
    const int steps = (int) std::floor (24.0 * std::log2 (maxCutHz / minCutHz));

    for (int k = 0; k <= steps + 1; ++k)
    {
        const double hz = k > steps ? maxCutHz : minCutHz * std::pow (2.0, k / 24.0);
        const double db = sumAt (hz);

        if (db < lowest)  { lowest = db;  lowestHz = hz; }
        if (db > highest) { highest = db; highestHz = hz; }
    }

    // Refine each extreme in 1/192 octave steps, so a bell at 400 Hz reads as 400 Hz.
    const auto refine = [&] (double centre, bool wantLowest, double& value)
    {
        double bestHz = centre, best = value;

        for (int k = -8; k <= 8; ++k)
        {
            const double hz = juce::jlimit (minCutHz, maxCutHz, centre * std::pow (2.0, k / 192.0));
            const double db = sumAt (hz);

            if (wantLowest ? db < best : db > best)
            {
                best = db;
                bestHz = hz;
            }
        }

        value = best;
        return bestHz;
    };

    if (lowest < -1.0e-9)
    {
        result.cutHz = refine (lowestHz, true, lowest);
        result.cutDb = -lowest;
    }

    if (highest > 1.0e-9)
    {
        result.boostHz = refine (highestHz, false, highest);
        result.boostDb = highest;
    }

    return result;
}

EqAnalysis analyseEq (const BuiltInEq& plugin)
{
    std::array<eq::Band, eq::numBands> bands;

    for (int i = 0; i < eq::numBands; ++i)
        bands[(size_t) i] = plugin.getBand (i);

    return analyseBands (bands);
}

juce::String formatFrequency (double hz)
{
    if (hz >= 1000.0)
    {
        const double k = hz / 1000.0;
        return (k < 10.0 ? trimmed (k, 1) : juce::String (juce::roundToInt (k))) + " kHz";
    }

    const int rounded = hz < 100.0 ? juce::roundToInt (hz) : juce::roundToInt (hz / 10.0) * 10;
    return juce::String (rounded) + " Hz";
}

//==============================================================================
std::vector<SendInfo> getSends (const Track& track)
{
    std::vector<SendInfo> result;

    for (auto& send : track.sends)
    {
        if (! send->isOn() || send->target == nullptr)
            continue;

        SendInfo info;
        info.returnTrack = send->target;
        info.levelDb = send->levelDb.load();
        info.returnMuted = send->target->muted.load();
        info.reverb = findEffect (*send->target, Effect::reverb, true);

        if (info.reverb != nullptr)
        {
            info.reverbOn = isOn (*info.reverb);

            if (auto* reverb = dynamic_cast<BuiltInReverb*> (info.reverb->instance.get()))
                info.mixPercent = reverb->getSettings().mixPercent;
        }

        result.push_back (info);
    }

    return result;
}

std::vector<PluginSlot*> findExerciseEffects (const Track& track)
{
    std::vector<PluginSlot*> effects;

    for (auto& slot : track.plugins)
        if (isOfEffect (*slot, Effect::eq) || isOfEffect (*slot, Effect::compressor))
            effects.push_back (slot.get());

    for (auto& send : track.sends)
        if (send->isOn() && send->target != nullptr)
            for (auto& slot : send->target->plugins)
                if (isOfEffect (*slot, Effect::reverb))
                    effects.push_back (slot.get());

    return effects;
}

//==============================================================================
const PluginSlot* CompareRecord::findCompared (const std::vector<PluginSlot*>& current) const
{
    for (auto* slot : current)
        for (auto& e : entries)
            if (e.slot == slot && e.onSeconds >= compareSeconds - epsilon && e.offSeconds >= compareSeconds - epsilon)
                return slot;

    return nullptr;
}

void ListenTracker::feed (bool playing, bool heard, double secondsSinceLastFeed)
{
    if (playing && heard)
        record.seconds += juce::jlimit (0.0, longestFeedSeconds, secondsSinceLastFeed);
}

void ReductionTracker::feed (bool playing, const PluginSlot* compressor, float recentDb)
{
    if (compressor != slot)
    {
        record = {};
        slot = compressor;
    }

    if (compressor != nullptr && playing)
        record.maxDb = juce::jmax (record.maxDb, (double) recentDb);
}

void CompareTracker::feed (bool playing, const std::vector<Seen>& effects, double secondsSinceLastFeed)
{
    auto& entries = record.entries;

    entries.erase (std::remove_if (entries.begin(), entries.end(), [&effects] (const CompareRecord::Entry& e)
    {
        return std::none_of (effects.begin(), effects.end(), [&e] (const Seen& s) { return s.slot == e.slot; });
    }), entries.end());

    if (! playing)
        return;

    const double dt = juce::jlimit (0.0, longestFeedSeconds, secondsSinceLastFeed);

    for (auto& seen : effects)
    {
        auto found = std::find_if (entries.begin(), entries.end(), [&seen] (const CompareRecord::Entry& e) { return e.slot == seen.slot; });

        if (found == entries.end())
        {
            entries.push_back ({ seen.slot, 0.0, 0.0 });
            found = entries.end() - 1;
        }

        (seen.on ? found->onSeconds : found->offSeconds) += dt;
    }
}

//==============================================================================
namespace
{
    /** What the EQs of the chosen track do: any, any switched on, the deepest cut among those on and the biggest boost. */
    struct EqSituation
    {
        bool any = false, anyOn = false;
        EqAnalysis bestCut, bigBoost;
    };

    EqSituation getEqSituation (const Track& track)
    {
        EqSituation s;

        for (auto& slot : track.plugins)
        {
            auto* plugin = slot->instance != nullptr ? dynamic_cast<BuiltInEq*> (slot->instance.get()) : nullptr;

            if (plugin == nullptr)
                continue;

            s.any = true;

            if (! isOn (*slot))
                continue;

            const auto a = analyseEq (*plugin);
            s.anyOn = true;

            if (a.cutDb > s.bestCut.cutDb)
                s.bestCut = a;

            if (a.boostDb > s.bigBoost.boostDb)
                s.bigBoost = a;
        }

        return s;
    }

    /** The most the chosen track's compressor reduced, and whether a switched-on one exists. */
    bool hasCompressorOn (const Track& track)
    {
        auto* slot = findEffect (track, Effect::compressor, true);
        return slot != nullptr && isOn (*slot);
    }

    const SendInfo* findCountingSend (const std::vector<SendInfo>& sends)
    {
        for (auto& s : sends)
            if (s.counts())
                return &s;

        return nullptr;
    }

    juce::String getTrackBars (const Engine& engine, double beat) { return juce::String ((int) std::floor (beat / engine.getBarLength() + 1.0e-9) + 1); }

    const juce::String noTrackText = "Pick a track first (step 1).";
}

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    const auto* track = findChosenTrack (engine, progress);

    if (track == nullptr)
        return false;

    switch (step)
    {
        case stepListen:     return progress.listened.isDone();
        case stepEq:         return getEqSituation (*track).bestCut.cutDb >= minCutDb - 1.0e-9;
        case stepCompressor: return hasCompressorOn (*track) && progress.reduction.maxDb >= minReductionDb - epsilon;
        case stepReverb:     return findCountingSend (getSends (*track)) != nullptr;
        case stepCompare:    return progress.compared.findCompared (findExerciseEffects (*track)) != nullptr;
        default:             break;
    }

    return false;
}

juce::String getDoText (int step, const Engine& engine, const Progress& progress)
{
    const auto* track = findChosenTrack (engine, progress);
    const auto name = nameOrDefault (track);
    const auto points = track != nullptr ? getStartingPoints (engine, *track) : StartingPoints();

    switch (step)
    {
        case stepListen:
            return "Click the name of a track with sound in it, such as a vocal, a bass or a guitar. Then press Play and listen to it with the rest of the song. "
                   "What bothers you? It may be muddy, harsh or thin, loud in some places and lost in others, or dry, as if played in a cupboard. Each of the next steps works on one of these.";

        case stepEq:
        {
            const bool exists = track != nullptr && findEffect (*track, Effect::eq) != nullptr;
            const auto first = exists ? "Press Open DawIt EQ to open the one on " + name + "."
                                      : "Press Add DawIt EQ: it goes on " + name + " and its window opens.";

            if (points.eq.isNotEmpty())
                return first + " Choose the starting point " + points.eq + " at the top of the window, then play the song and listen. "
                       "Change what doesn't suit your recording: drag a dot down to cut more, or up to cut less.";

            return first + " Turn on the low cut on the left and, while the song plays, drag it up until the track starts to sound thin, then back a little. "
                   "If it sounds muddy, drag a bell down a few dB somewhere between 200 and 500 Hz; if harsh, between 2 and 5 kHz.";
        }

        case stepCompressor:
        {
            const bool exists = track != nullptr && findEffect (*track, Effect::compressor) != nullptr;
            const auto start = points.compressor.isNotEmpty() ? points.compressor : juce::String ("Neutral start");
            double target = 4.0;

            for (auto& p : comp::getPresets())
                if (p.name == start)
                    target = p.targetReductionDb;

            const auto first = exists ? "Press Open DawIt Compressor to open the one on " + name + "."
                                      : "Press Add DawIt Compressor: it goes on " + name + " and its window opens.";
            const auto choose = start == "Neutral start" ? juce::String (" It starts at Neutral start.")
                                                         : " Choose the starting point " + start + " at the top of its window.";
            return first + choose + " Then play a part where " + name + " plays: the red line shows how much it turns down. "
                   "Lower Threshold until the loudest parts come down by about " + trimmed (target) + " dB, or press Set it up step by step and it sets the threshold for you.";
        }

        case stepReverb:
            return "Press Open the Sends menu and choose New return track > Reverb. A return track called Reverb appears, with DawIt Reverb on it, and " + name
                   + " sends a copy of its sound to it at -10 dB. Play the song and set the amount by ear: drag the send in the device bar up or down, or turn the Reverb knob in the mixer (Cmd+2). "
                     "If your song already has a reverb return, choose that one in the menu instead.";

        case stepCompare:
            return "Turn on Match level in the EQ and compressor windows, so the track plays as loud with them as without. Then, while the song plays, click the dot on an effect's chip in the device bar to switch it off, "
                   "listen, and click again to switch it on. For the reverb, select the return and do the same on DawIt Reverb. Go back and forth a few times, a few seconds each way.";

        default: break;
    }

    return {};
}

juce::String getActionLabel (int step, bool hasEffect)
{
    switch (step)
    {
        case stepListen:     return "Play";
        case stepEq:         return hasEffect ? "Open DawIt EQ" : "Add DawIt EQ";
        case stepCompressor: return hasEffect ? "Open DawIt Compressor" : "Add DawIt Compressor";
        case stepReverb:     return "Open the Sends menu";
        case stepCompare:    return "Play";
        default:             break;
    }

    return {};
}

juce::String getActionTooltip (int step, const juce::String& trackName, bool hasEffect)
{
    const auto name = trackName.isNotEmpty() ? trackName : juce::String ("your track");

    switch (step)
    {
        case stepListen:
        case stepCompare:
            return "Plays the song from the playhead. Press Space to stop.";

        case stepEq:
            return hasEffect ? "Opens the DawIt EQ on " + name + "."
                             : "Adds DawIt EQ to " + name + ", after its other effects, and opens it. It starts flat, so nothing changes until you move a band. Remove it with the x on its chip.";

        case stepCompressor:
            return hasEffect ? "Opens the DawIt Compressor on " + name + "."
                             : "Adds DawIt Compressor to " + name + ", after its other effects, and opens it at Neutral start, which turns down what goes over -18 dB, so you may hear it at once. Remove it with the x on its chip.";

        case stepReverb:
            return "Opens the Sends menu of " + name + ", as its Sends button does. Nothing changes until you choose an item.";

        default: break;
    }

    return {};
}

juce::String getFixLabel (Fix fix)
{
    switch (fix)
    {
        case Fix::importAudio:    return "Import Audio...";
        case Fix::audioSettings:  return "Audio/MIDI Settings...";
        case Fix::none:
        case Fix::detectTempo:
        case Fix::detectChords:
        case Fix::detectSections:
        case Fix::openSections:
        case Fix::openLyrics:     break;
    }

    return {};
}

juce::String getFixTooltip (Fix fix)
{
    switch (fix)
    {
        case Fix::importAudio:    return "Opens the file chooser, to bring a recording into your song.";
        case Fix::audioSettings:  return "Opens the audio settings, where you choose where the sound comes out.";
        case Fix::none:
        case Fix::detectTempo:
        case Fix::detectChords:
        case Fix::detectSections:
        case Fix::openSections:
        case Fix::openLyrics:     break;
    }

    return {};
}

ActionText getAction (int step, const Engine& engine, const Progress& progress)
{
    const auto fix = getStatus (step, engine, progress).fix;

    if (fix != Fix::none)
        return { getFixLabel (fix), getFixTooltip (fix), fix };

    const auto* track = findChosenTrack (engine, progress);
    const bool hasEffect = track != nullptr && ((step == stepEq && findEffect (*track, Effect::eq) != nullptr)
                                                || (step == stepCompressor && findEffect (*track, Effect::compressor) != nullptr));
    return { getActionLabel (step, hasEffect), getActionTooltip (step, track != nullptr ? track->name : juce::String(), hasEffect), Fix::none };
}

//==============================================================================
static Status getListenStatus (const Engine& engine, const Progress& progress)
{
    const auto* track = findChosenTrack (engine, progress);

    if (! hasTrackWithSound (engine))
        return { "Your song has no track with sound yet. Import a recording with File > Import Audio..., or make a first song with Help > First Song Exercise....", Fix::importAudio };

    if (track == nullptr)
    {
        if (progress.selected != nullptr && engine.indexOf (progress.selected) >= 0 && ! isSoundTrack (*progress.selected))
            return { progress.selected->name + (progress.selected->isReturn() ? " is a return" : " is a bus")
                         + ": it has no sound of its own. Click the name of an audio or MIDI track.", Fix::none };

        return { "Click the name of a track with sound in it.", Fix::none };
    }

    juce::StringArray lines;
    auto fix = Fix::none;

    switch (routing::getSilence (*track, engine.getTracks()))
    {
        case routing::Silence::muted:    lines.add (track->name + " is muted, so you won't hear it. Click its M to unmute it."); break;
        case routing::Silence::busMuted: lines.add (track->name + " plays into a muted bus, so you won't hear it. Click the bus's M to unmute it."); break;
        case routing::Silence::solo:     lines.add ("Another track is soloed, so " + track->name + " is silent. Click S on the soloed track to turn solo off."); break;
        case routing::Silence::none:     break;
    }

    if (! progress.listened.isDone())
    {
        const double first = getFirstClipBeat (*track);

        if (lines.isEmpty() && progress.playing && ! isHeardAt (engine, *track, progress.heardBeat))
        {
            if (first >= 0.0)
                lines.add (track->name + " has nothing to play here. Move the playhead to bar " + getTrackBars (engine, first) + ", where its first clip starts.");
            else
                lines.add (track->name + " has no clip to play. Click a track that has sound.");
        }

        if (! progress.hasOutputDevice)
        {
            lines.add ("DawIt has no audio output, so you won't hear anything. Choose an output in Audio/MIDI Settings....");
            fix = Fix::audioSettings;
        }
    }

    return { lines.joinIntoString ("\n"), fix };
}

static Status getEqStatus (const Track& track)
{
    const auto s = getEqSituation (track);
    juce::StringArray lines;

    if (s.any && ! s.anyOn)
        lines.add ("DawIt EQ on " + track.name + " is switched off. Click the dot on its chip in the device bar to switch it on.");
    else if (s.any && s.bestCut.cutDb < minCutDb - 1.0e-9)
        lines.add ("Your EQ doesn't cut anything yet. This step is about making room: turn something down, with the low cut or a dot dragged below the line.");

    if (s.bigBoost.boostDb >= bigBoostDb - 1.0e-9)
        lines.add ("Tip: your EQ raises " + formatFrequency (s.bigBoost.boostHz) + " by " + oneDecimal (s.bigBoost.boostDb)
                   + " dB. A big boost makes the track louder and pushes the next effect harder. Turn on Match level to hear whether it's really better.");

    return { lines.joinIntoString ("\n"), Fix::none };
}

static Status getCompressorStatus (const Track& track, const Progress& progress)
{
    juce::StringArray lines;
    auto* slot = findEffect (track, Effect::compressor, true);

    if (slot != nullptr && ! isOn (*slot))
        lines.add ("DawIt Compressor on " + track.name + " is switched off. Click the dot on its chip to switch it on.");
    else if (slot != nullptr && progress.reduction.maxDb < minReductionDb - epsilon)
        lines.add ("DawIt Compressor hasn't turned " + track.name + " down by 2 dB yet (" + oneDecimal (progress.reduction.maxDb)
                   + " dB so far). Play a part where " + track.name + " plays, and lower Threshold until the red line moves.");

    if (slot != nullptr && progress.reduction.maxDb >= heavyReductionDb - epsilon)
        lines.add ("Tip: it turned down up to " + oneDecimal (progress.reduction.maxDb)
                   + " dB. That's a lot for evening out: the track may sound flat or pump. Raise Threshold, or lower Ratio.");

    return { lines.joinIntoString ("\n"), Fix::none };
}

static Status getReverbStatus (const Track& track)
{
    const auto sends = getSends (track);
    juce::StringArray lines;

    if (findCountingSend (sends) == nullptr && ! sends.empty())
    {
        const auto& s = sends.front();
        const auto name = s.returnTrack->name;

        if (s.returnMuted)
            lines.add ("The return " + name + " is muted, so the reverb is silent. Click its M to unmute it.");
        else if (s.reverb == nullptr)
            lines.add (name + " has no DawIt Reverb on it. Send " + track.name + " to a reverb return too: in the Sends menu, choose New return track > Reverb.");
        else
            lines.add ("DawIt Reverb on " + name + " is switched off. Select the return and click the dot on its chip to switch it on.");
    }

    const auto source = getSource (track);

    if (source == Source::bass || source == Source::kick)
        lines.add ("Tip: bass and kick usually get little or no reverb, because a tail in the low end turns to mud. Keep the send low, or try this step on another track.");

    for (auto& s : sends)
        if (s.reverb != nullptr && s.mixPercent < 99.5f)
        {
            lines.add ("Tip: DawIt Reverb on " + s.returnTrack->name + " has its Mix at " + juce::String (juce::roundToInt (s.mixPercent)) + "%, so the return also plays a dry copy of "
                       + track.name + ". On a return, set Mix to 100%: then the send alone decides how much reverb a track gets.");
            break;
        }

    return { lines.joinIntoString ("\n"), Fix::none };
}

static Status getCompareStatus (const Track& track, const Progress& progress, bool done)
{
    const auto effects = findExerciseEffects (track);
    juce::StringArray lines;
    auto fix = Fix::none;

    if (effects.empty())
    {
        lines.add ("Add an EQ, a compressor or a reverb send first (steps 2 to 4), so there's something to switch off.");
    }
    else if (! done)
    {
        lines.add ("Play the song and switch an effect off and on while it plays: each way needs at least 2 seconds.");

        if (! progress.hasOutputDevice)
        {
            lines.add ("DawIt has no audio output, so you won't hear anything. Choose an output in Audio/MIDI Settings....");
            fix = Fix::audioSettings;
        }
    }

    for (auto* slot : effects)
    {
        auto* eqPlugin = dynamic_cast<BuiltInEq*> (slot->instance.get());
        auto* compressor = dynamic_cast<BuiltInCompressor*> (slot->instance.get());
        const bool matching = eqPlugin != nullptr ? eqPlugin->isMatchingLevel() : compressor != nullptr ? compressor->isMatchingLevel() : true;

        if (! matching)
        {
            lines.add ("Tip: Match level is off in DawIt " + getEffectName (*slot) + ", so switching it also changes how loud the track is. Turn it on in its window to compare fairly.");
            break;
        }
    }

    return { lines.joinIntoString ("\n"), fix };
}

Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    if (step == stepListen)
        return getListenStatus (engine, progress);

    if (step < stepListen || step > stepCompare)
        return {};

    const auto* track = findChosenTrack (engine, progress);

    if (track == nullptr)
        return { noTrackText, Fix::none };

    switch (step)
    {
        case stepEq:         return getEqStatus (*track);
        case stepCompressor: return getCompressorStatus (*track, progress);
        case stepReverb:     return getReverbStatus (*track);
        case stepCompare:    return getCompareStatus (*track, progress, isStepDone (step, engine, progress));
        default:             break;
    }

    return {};
}

juce::String getDoneNote (int step, const Engine& engine, const Progress& progress)
{
    if (! isStepDone (step, engine, progress))
        return {};

    const auto* track = findChosenTrack (engine, progress);

    switch (step)
    {
        case stepListen:
            return track->name + " is your track.";

        case stepEq:
        {
            const auto a = getEqSituation (*track).bestCut;
            return "DawIt EQ takes up to " + oneDecimal (a.cutDb) + " dB off, most around " + formatFrequency (a.cutHz) + ".";
        }

        case stepCompressor:
            return "DawIt Compressor turned the loudest parts down by up to " + oneDecimal (progress.reduction.maxDb) + " dB.";

        case stepReverb:
        {
            const auto sends = getSends (*track);
            const auto* s = findCountingSend (sends);
            return track->name + " sends to " + s->returnTrack->name + " at " + oneDecimal (s->levelDb) + " dB.";
        }

        case stepCompare:
        {
            const auto* slot = progress.compared.findCompared (findExerciseEffects (*track));
            return "You compared DawIt " + getEffectName (*slot) + " off and on. It's " + (isOn (*slot) ? "on" : "off") + " now.";
        }

        default: break;
    }

    return {};
}

juce::String getShowMeText (int step, const Engine& engine, const Progress& progress)
{
    const auto* track = findChosenTrack (engine, progress);
    const auto name = nameOrDefault (track);

    switch (step)
    {
        case stepListen:
            return "Click a track's name to choose it.";

        case stepEq:
            return track != nullptr && findEffect (*track, Effect::eq) != nullptr ? "DawIt EQ on " + name + ": click to open it."
                                                                                  : "+ Effect adds an effect to " + name + ".";

        case stepCompressor:
            return track != nullptr && findEffect (*track, Effect::compressor) != nullptr ? "DawIt Compressor on " + name + ": click to open it."
                                                                                          : "+ Effect adds an effect to " + name + ".";

        case stepReverb:
            return "Sends: send " + name + " to a reverb on a return track.";

        case stepCompare:
        {
            const auto effects = track != nullptr ? findExerciseEffects (*track) : std::vector<PluginSlot*>();
            return effects.empty() ? juce::String ("Add an effect first: then click its dot to switch it off and on while the song plays.")
                                   : "Click this dot to switch DawIt " + getEffectName (*effects.front()) + " off and on while the song plays.";
        }

        default: break;
    }

    return {};
}
} // namespace daw::gen::soundbetter
