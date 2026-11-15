#include "MixExport.h"

#include "engine/TrackNameWords.h"

#include "SoundBetter.h"

#include <algorithm>
#include <cmath>

namespace daw::gen::mixexport
{
using guides::Guide;

namespace
{
    constexpr double epsilon = 1.0e-4;
    constexpr double beatsPerBar = 4.0;   // 4/4 for now

    /** "4", "44.1": a number with no trailing zero. */
    juce::String trimmed (double value, int decimals = 1)
    {
        auto text = juce::String (value, decimals);

        if (text.containsChar ('.'))
            text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

        return text;
    }

    /** "-2.1", "+1.4": one decimal, with a plus sign above zero. */
    juce::String signedDb (double db)
    {
        const auto text = juce::String (db, 1);
        return db > 0.05 ? "+" + text : text;
    }

    double toDb (float peak) { return juce::Decibels::gainToDecibels ((double) peak, -100.0); }

    bool hasClip (const Track& track) { return ! track.clips.empty(); }

    bool hasTrackWithSound (const Engine& engine)
    {
        for (auto& t : engine.getTracks())
            if (isSoundTrack (*t) && hasClip (*t))
                return true;

        return false;
    }

    /** "bars 13 to 20" for a range in beats (bar 0 is shown as 1). */
    juce::String barsText (double startBeat, double endBeat)
    {
        return "bars " + juce::String (juce::roundToInt (startBeat / beatsPerBar) + 1) + " to " + juce::String (juce::roundToInt (endBeat / beatsPerBar));
    }

    /** What Spotify would do with a song, as a sentence that follows "Integrated ...". */
    juce::String spotifyText (double integrated, double truePeak)
    {
        const double gain = loudness::streamingGainDb (integrated, truePeak);

        if (integrated > -13.0)
            return " Spotify would play it " + juce::String (integrated + 14.0, 1) + " dB quieter.";

        if (integrated >= -15.0)
            return " That's about where Spotify plays songs.";

        if (gain <= 0.05)
            return " Its peaks leave Spotify no room to turn it up.";

        const double reaches = integrated + gain;

        if (reaches >= -14.0 - 0.05)
            return " Spotify would turn it up " + juce::String (gain, 1) + " dB, to about -14 LUFS.";

        return " Spotify would turn it up " + juce::String (gain, 1) + " dB, as far as its peaks allow, to about " + juce::String (reaches, 1) + " LUFS.";
    }
}

//==============================================================================
std::vector<Step> getSteps()
{
    return {
        { "Balance the levels", "How loud each part is against the others",
          "Balance is most of a mix. Engineers set it with the faders alone, before any EQ or compression, because it shows what really needs fixing. "
          "A balance that works where the most parts play usually works in the quieter parts too. "
          "Listen at a moderate level, one you could talk over, and keep it there: at different volumes the ear hears bass and treble differently, and louder almost always seems better. "
          "Judge by ear, not by the meters: a snare can peak high and still sound quieter than a pad.",
          { { Guide::gainStaging, "Balancing the tracks" }, { Guide::gainStaging, "Effects, fader and master" } } },
        { "Pan the parts", "A place for each part, left to right",
          "Parts that all sit in the middle compete to be heard. Panning gives each one a place of its own, so each is clearer, and the mix gets wider. "
          "The lowest and most important parts usually stay in the middle: both speakers share them, so they stay solid on every system, even in mono. "
          "In DawIt, panning a part all the way out makes it up to 3 dB quieter, so check its level again after.",
          { { Guide::gainStaging, "Panning the parts" }, { Guide::stereo, "Why the low end stays in the middle" } } },
        { "Check the loudness", "How loud is the whole song?",
          "Peaks show whether something clips, not how loud a song sounds: the ear follows how much sound arrives over time. "
          "A loudness meter measures that in LUFS, the unit streaming services use. Integrated loudness is the average of the whole song, so it's measured from the start to the end. "
          "While you mix there's no target to hit: just leave room, with the true peak below about -1 dBTP. "
          "Streaming services play every song at about the same loudness. Spotify, for one, plays songs at about -14 LUFS: it turns louder ones down, and quieter ones up only as far as their peaks allow. "
          "Making a song loud is a job for mastering, at the end.",
          { { Guide::loudness, "Loudness while mixing" }, { Guide::loudness, "Targets and normalisation" } } },
        { "Export", "A WAV file of your song",
          "Exporting, or bouncing, records what DawIt plays into a file: every track through its effects, buses and faders, then the master. Your song doesn't change, and muted tracks stay silent. "
          "A 16- or 24-bit file can't hold a peak over 0 dB: the loudest moments are cut off, which sounds harsh. That's why the loudness step checks the peaks first. "
          "A 32-bit float file keeps them, but most players and services need a fixed-point file.",
          { { Guide::routing, "Bouncing to audio" }, { Guide::gainStaging, "Over 0 dB" } } },
    };
}

juce::String getTitle() { return "Mix and export your song"; }

juce::String getIntro()
{
    return "Four steps to a finished file of your song: balance the levels, place the parts left and right, check how loud it is, then export it as a WAV you can play anywhere. "
           "Do each step in the main window: the tick appears by itself.";
}

juce::String getHelpDescription()
{
    return "Balance the faders, pan the parts, check the loudness and export a WAV of your song, in four steps";
}

juce::String getCompletionText()
{
    return "Done. Your song is balanced, placed, measured and exported: a file you can play to anyone. "
           "Listen to it somewhere else, on headphones, a phone or in a car, and note what you'd change: then change it and export again. "
           "To make it as loud as released songs, or to hand it to an engineer, read on.";
}

int clampStep (int stored) noexcept
{
    return stored >= 0 && stored < numSteps ? stored : 0;
}

//==============================================================================
Baseline takeBaseline (const Engine& engine)
{
    Baseline b;
    b.valid = true;
    b.masterDb = engine.masterVolumeDb.load();

    for (auto& t : engine.getTracks())
        b.tracks.push_back ({ t.get(), t->volumeDb.load(), t->pan.load() });

    return b;
}

juce::String formatPan (float pan)
{
    const int percent = juce::roundToInt (std::abs (pan) * 100.0f);

    if (percent == 0)
        return "C";

    return juce::String (percent) + (pan < 0.0f ? " L" : " R");
}

//==============================================================================
bool isSoundTrack (const Track& track) { return track.isAudio() || track.isMidi(); }

bool isBackingVocal (const Track& track)
{
    const auto words = trackname::getWords (track.name);
    return trackname::nameHasWord (words, { "backing", "bv", "bvs", "harmony" });
}

bool isCentrePart (const Track& track)
{
    using S = soundbetter::Source;

    switch (soundbetter::getSource (track))
    {
        case S::kick:
        case S::snare:
        case S::bass:   return true;

        // A drum track counts only when its name says drums or kit: the name rule also calls hats, cymbals and percussion drums, and those sit to the sides.
        case S::drums:  return trackname::nameHasWord (trackname::getWords (track.name), { "drum", "drums", "kit" });
        case S::vocal:  return ! isBackingVocal (track);
        case S::other:
        case S::guitar:
        case S::keys:
        case S::ensemble: break;
    }

    return false;
}

const Track* findPanCandidate (const Engine& engine)
{
    using S = soundbetter::Source;

    for (auto& t : engine.getTracks())
    {
        if (! isSoundTrack (*t))
            continue;

        switch (soundbetter::getSource (*t))
        {
            case S::guitar:
            case S::keys:
            case S::ensemble:  return t.get();
            case S::vocal:     if (isBackingVocal (*t)) return t.get(); break;
            case S::other:
            case S::kick:
            case S::snare:
            case S::drums:
            case S::bass:      break;
        }
    }

    return nullptr;
}

const Track* findShownTrack (const Engine& engine, const Track* selected)
{
    if (selected != nullptr && engine.indexOf (selected) >= 0 && isSoundTrack (*selected))
        return selected;

    for (auto& t : engine.getTracks())
        if (isSoundTrack (*t) && hasClip (*t))
            return t.get();

    return nullptr;
}

//==============================================================================
PanGains getPanGains (double pan)
{
    const double halfPi = juce::MathConstants<double>::halfPi;
    return { pan > 0.0 ? std::cos (pan * halfPi) : 1.0, pan < 0.0 ? std::cos (-pan * halfPi) : 1.0 };
}

double getPanPowerDb (double pan)
{
    const auto g = getPanGains (pan);
    return 10.0 * std::log10 ((g.left * g.left + g.right * g.right) / 2.0);
}

double getMonoSumDb (double pan)
{
    const auto g = getPanGains (pan);
    return 20.0 * std::log10 ((g.left + g.right) / 2.0);
}

//==============================================================================
std::optional<FaderMove> findFaderMove (const Engine& engine, const Baseline& baseline)
{
    std::optional<FaderMove> best;

    for (auto& e : baseline.tracks)
    {
        if (e.track == nullptr || engine.indexOf (e.track) < 0)
            continue;

        const float now = e.track->volumeDb.load();
        const double moved = std::abs ((double) now - (double) e.volumeDb);

        if (moved >= minFaderMoveDb - epsilon && (! best.has_value() || moved > std::abs ((double) best->toDb - (double) best->fromDb)))
            best = FaderMove { e.track, e.volumeDb, now };
    }

    return best;
}

std::optional<PanMove> findPanMove (const Engine& engine, const Baseline& baseline)
{
    std::optional<PanMove> best;

    for (auto& e : baseline.tracks)
    {
        if (e.track == nullptr || engine.indexOf (e.track) < 0 || ! isSoundTrack (*e.track))
            continue;

        const float now = e.track->pan.load();
        const double moved = std::abs ((double) now - (double) e.pan);

        if (moved >= minPanMove - epsilon && (! best.has_value() || moved > std::abs ((double) best->to - (double) best->from)))
            best = PanMove { e.track, e.pan, now };
    }

    return best;
}

bool isMeasured (const MeasureRecord& m)
{
    return m.valid && m.wholeSong && m.readings.integrated.has_value() && m.readings.truePeak.has_value();
}

bool clips (int bitDepth, float peak)
{
    return bitDepth != 32 && peak > 1.0f;
}

bool isExported (const ExportRecord& e)
{
    return e.valid && e.what == What::mix && e.wholeSong && ! clips (e.bitDepth, e.peak);
}

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepBalance:   return findFaderMove (engine, progress.baseline).has_value();
        case stepPan:       return findPanMove (engine, progress.baseline).has_value();
        case stepLoudness:  return isMeasured (progress.measured);
        case stepExport:    return isExported (progress.exported);
        default: break;
    }

    return false;
}

//==============================================================================
juce::String getDoText (int step, const Engine& engine, const Progress&)
{
    switch (step)
    {
        case stepBalance:
            return "Press Open the mixer (Cmd+2) and play the busiest part of your song, usually a chorus. Start with the part the song is built on, the drums and bass or the lead vocal. "
                   "Then set each other part's fader until you hear it clearly, and back a little. The tick comes when a fader has moved 1 dB or more.";

        case stepPan:
        {
            const auto* candidate = findPanCandidate (engine);
            const auto start = candidate != nullptr ? "Turn the pan knob of " + candidate->name + ", above its fader, to the left or right while the song plays."
                                                    : juce::String ("Turn a track's pan knob in the mixer to the left or right while the song plays.");
            return start + " Keep the kick, snare, bass and lead vocal in the middle, and send parts like guitars, keys and backing vocals to the sides. "
                           "A part played twice sounds wide with one take on each side.";
        }

        case stepLoudness:
            return "Press Measure the whole song. DawIt plays the song into a loudness meter, faster than real time and without a sound, and shows the numbers in What is loudness?. "
                   "File > Measure Loudness... does the same, but with the loop on it measures only the loop.";

        case stepExport:
            return "Press Export the song.... The Bounce to WAV window opens on The mix and Whole song, with Let the end ring out on, so the last reverb isn't cut off. "
                   "44.1 kHz and 24-bit suit most uses; if a distributor asks for 16-bit at 44.1 kHz, choose that. "
                   "Press Bounce..., choose where to save, and DawIt writes the file, faster than the song plays.";

        default: break;
    }

    return {};
}

juce::String getActionLabel (int step)
{
    switch (step)
    {
        case stepBalance:
        case stepPan:       return "Open the mixer";
        case stepLoudness:  return "Measure the whole song";
        case stepExport:    return "Export the song...";
        default: break;
    }

    return {};
}

juce::String getActionTooltip (int step)
{
    switch (step)
    {
        case stepBalance:
        case stepPan:       return "Shows the mixer under the tracks (Cmd+2): a fader, a pan knob and a meter for every track.";
        case stepLoudness:  return "Renders the whole song, even with the loop on, without playing it, and measures its loudness the way DawIt Meter does. Nothing in the song changes.";
        case stepExport:    return "Opens Bounce to WAV on The mix and Whole song. Nothing is written until you press Bounce... and choose where to save.";
        default: break;
    }

    return {};
}

juce::String getFixLabel (Fix fix)
{
    switch (fix)
    {
        case Fix::importAudio:    return "Import audio...";
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
        case Fix::importAudio:    return "Opens Import Audio (Cmd+I): choose a song file to put on an audio track. Nothing else in the song changes.";
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

//==============================================================================
Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    const bool done = isStepDone (step, engine, progress);

    if ((step == stepBalance || step == stepPan) && ! done)
    {
        if (! hasTrackWithSound (engine))
            return { "Your song has no tracks with sound yet. Make some with Help > First Song Exercise..., or import a recording with File > Import Audio....", Fix::importAudio };

        if (! progress.hasOutputDevice)
            return { "DawIt has no audio output, so you won't hear the " + juce::String ("balance") + ". Choose an output in Audio/MIDI Settings....", Fix::audioSettings };
    }

    switch (step)
    {
        case stepBalance:
        {
            if (! done && progress.baseline.valid
                && std::abs ((double) engine.masterVolumeDb.load() - (double) progress.baseline.masterDb) >= minFaderMoveDb - epsilon)
                return { "You moved the master, which turns the whole song up or down. The balance is between the tracks: move a track's fader instead.", Fix::none };

            const Track* lowest = nullptr;

            for (auto& t : engine.getTracks())
                if (t->volumeDb.load() < farDownDb && (lowest == nullptr || t->volumeDb.load() < lowest->volumeDb.load()))
                    lowest = t.get();

            if (lowest != nullptr)
                return { "Tip: " + lowest->name + "'s fader is at " + juce::String (lowest->volumeDb.load(), 1) + " dB. Faders within about 10 dB of 0 dB are easiest to fine-tune. "
                         + (lowest->hasInput() ? "If it is far too loud, turn down the tracks in it, then bring the fader back up."
                                               : "If a part is far too loud, turn down its instrument or its clip's gain, then bring the fader back up."), Fix::none };

            break;
        }

        case stepPan:
        {
            for (auto& t : engine.getTracks())
                if (isSoundTrack (*t) && isCentrePart (*t) && std::abs ((double) t->pan.load()) >= centreTipPan - epsilon)
                    return { "Tip: " + t->name + " is panned " + formatPan (t->pan.load()) + ". The kick, snare, bass and lead vocal usually sit in the middle, "
                             "so the base of the song is solid in both speakers. Double-click its pan knob to centre it, unless you want it there.", Fix::none };

            break;
        }

        case stepLoudness:
        {
            const auto& m = progress.measured;

            if (! hasTrackWithSound (engine))
                return { "Your song has nothing to measure yet. Add a clip to a track first.", Fix::none };

            if (m.valid && ! m.wholeSong)
                return { "That measured only the loop, " + barsText (m.startBeat, m.endBeat) + " (" + formatLength (m.readings.seconds)
                         + "). Press Measure the whole song for the song's loudness.", Fix::none };

            if (isMeasured (m))
            {
                const double tp = *m.readings.truePeak;

                if (tp > 0.0)
                    return { "Tip: the true peak reaches " + signedDb (tp) + " dBTP, over 0, so the song can clip once exported or played. Turn the tracks down by about "
                             + juce::String (getTurnDownDb (tp)) + " dB (the master works too) and measure again.", Fix::none };

                if (tp > -1.0)
                    return { "Tip: the true peak is " + juce::String (tp, 1) + " dBTP, less than 1 dB below 0. Streaming services turn songs into smaller files (AAC or Ogg), "
                             "which can push peaks this close over 0. Leave at least 1 dB.", Fix::none };
            }

            break;
        }

        case stepExport:
        {
            const auto& e = progress.exported;

            if (! e.valid)
                break;

            if (e.what == What::selectedTracks)
                return { "That file holds only the selected tracks. For the song, choose The mix under What.", Fix::none };

            if (e.what == What::multitracks || e.what == What::stems)
                return { "That made one file per " + juce::String (e.what == What::multitracks ? "track (multitracks)" : "bus or track outside a bus (stems)")
                         + ". For the song as one file, choose The mix under What.", Fix::none };

            if (! e.wholeSong)
                return { "That file is only the loop, " + barsText (e.startBeat, e.endBeat) + ". Choose Whole song under Range.", Fix::none };

            const double peakDb = toDb (e.peak);

            if (clips (e.bitDepth, e.peak))
                return { "The mix peaks at " + signedDb (peakDb) + " dB, so the " + juce::String (e.bitDepth) + "-bit file clips on its loudest moments. Turn the tracks down by about "
                         + juce::String (getTurnDownDb (peakDb)) + " dB (the master works too) and export again.", Fix::none };

            if (e.bitDepth == 32 && e.peak > 1.0f)
                return { "Tip: the file peaks at " + signedDb (peakDb) + " dB. 32-bit float keeps that, but played as it is, or turned into another format, it clips. "
                         "Turn the tracks down by about " + juce::String (getTurnDownDb (peakDb)) + " dB for a file to share.", Fix::none };

            break;
        }

        default: break;
    }

    return {};
}

juce::String getDoneNote (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepBalance:
        {
            const auto move = findFaderMove (engine, progress.baseline);

            if (move.has_value())
                return move->track->name + " moved from " + juce::String (move->fromDb, 1) + " to " + juce::String (move->toDb, 1) + " dB.";

            break;
        }

        case stepPan:
        {
            const auto move = findPanMove (engine, progress.baseline);

            if (move.has_value())
                return move->track->name + " is now " + formatPan (move->to) + " (it was " + formatPan (move->from) + ").";

            break;
        }

        case stepLoudness:
        {
            const auto& m = progress.measured;

            if (! isMeasured (m))
                break;

            const double integrated = *m.readings.integrated, tp = *m.readings.truePeak;
            return "Integrated " + juce::String (integrated, 1) + " LUFS, true peak " + signedDb (tp) + " dBTP"
                   + (tp > 0.0 ? "." : ": " + juce::String (-tp, 1) + " dB of headroom.")
                   + spotifyText (integrated, tp);
        }

        case stepExport:
        {
            const auto& e = progress.exported;

            if (! isExported (e))
                break;

            return e.fileName + ": " + formatLength (e.seconds) + ", " + juce::String (e.bitDepth) + "-bit, " + trimmed (e.sampleRate / 1000.0, 1) + " kHz, peak "
                   + signedDb (toDb (e.peak)) + " dB.";
        }

        default: break;
    }

    return {};
}

juce::String getShowMeText (int step, const juce::String& trackName)
{
    const auto name = trackName.isNotEmpty() ? trackName + "'s" : juce::String ("A track's");

    switch (step)
    {
        case stepBalance:   return name + " fader: drag it up or down while the song plays.";
        case stepPan:       return name + " pan knob: drag it to move the part left or right. Double-click centres it.";
        case stepLoudness:  return "The master's meter shows peaks as the song plays. For loudness, measure the whole song: press Measure the whole song.";
        case stepExport:    return "The master: everything you hear goes through it into the file. Press Export the song..., or choose File > Bounce to WAV... (Cmd+B).";
        default: break;
    }

    return {};
}

juce::String formatLength (double seconds)
{
    const int total = juce::roundToInt (seconds);
    return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
}

int getTurnDownDb (double peakDb)
{
    return (int) std::ceil (peakDb + 1.0);
}
} // namespace daw::gen::mixexport
