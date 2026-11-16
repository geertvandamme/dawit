#include "Karaoke.h"

#include "TheoryGuides.h"
#include "analysis/LyricsFinder.h"
#include "WriteLyrics.h"
#include "engine/ModelDownload.h"
#include "engine/Routing.h"
#include "engine/Stems.h"

#include <cmath>

namespace daw::gen::karaoke
{
using guides::Guide;

namespace
{
    constexpr double eps = 1.0e-6;

    const AudioClip* asAudio (const Clip* clip) { return dynamic_cast<const AudioClip*> (clip); }

    /** The whole bars between two beats, as `songchords::getSpan` makes them. */
    songchords::Span spanOfBeats (const Engine& engine, double startBeat, double endBeat)
    {
        const double barLength = engine.getBarLength();
        songchords::Span span;
        span.firstBar = juce::jmax (0, (int) std::ceil (startBeat / barLength - eps));
        span.endBar = juce::jmax (span.firstBar, (int) std::floor (endBeat / barLength + eps));
        return span;
    }

    bool isTakeClip (const TakeRecord& take, const Clip* clip)
    {
        for (auto* c : take.clips)
            if (c == clip)
                return true;

        return false;
    }

    juce::String barsText (double bars) { return juce::String ((int) std::floor (bars + eps)); }

    juce::String dbText (double db) { return juce::String (juce::roundToInt (db)); }
}

std::vector<Step> getSteps()
{
    return {
        { "Bring in a song", "An audio recording of a song with singing",
          "A recording is audio: the sound itself, not notes you can edit. The next steps split that sound into parts and listen to the words in it.",
          { { Guide::midi, "What MIDI is" } } },
        { "Take the vocals out", "Make a karaoke version",
          "Demucs, a model that has learned what voices, drums and bass sound like, splits the song into four parts that add up to the mix. Without the vocals you hear the whole band, low end included. "
          "A little reverb from the voice can stay, and backing vocals go with the lead.",
          { { Guide::sound, "Taking the vocals out" } } },
        { "Find the words", "Let DawIt write down what is sung",
          "Whisper, a speech model, hears the words; DawIt then moves each word to where the voice begins and splits it into syllables. It works for English only, and it can mishear a sung word: "
          "double-click the Lyrics row to correct the words. The words of someone else's song belong to them: use them to practise, not to publish.",
          { { Guide::lyrics, "Your lyrics" } } },
        { "Sing along", "Read the words as they light up",
          "Reading the words in time frees you to listen to the band and your pitch. Stand or sit up straight and breathe in before the count-in ends. Wear headphones so you can hear the song over yourself.",
          { { Guide::lyrics, "Singing along" }, { Guide::singing, "Posture and breathing" } } },
        { "Record a take", "Record yourself singing over the karaoke version",
          juce::String::fromUTF8 ("Wear closed headphones, not speakers: speakers leak the song into your take, and a microphone that hears itself can howl. Set the input so your loudest notes peak around \xe2\x88\x92" "18 to \xe2\x88\x92" "10 dB, never near 0 dB: "
          "a clipped take can't be repaired. Monitor stays off; most audio interfaces let you hear yourself directly without delay."),
          { { Guide::recording, "Hearing yourself" }, { Guide::recording, "Setting the level" } } },
        { "Listen back", "Hear how you really sound",
          "Your voice on a recording sounds higher and thinner than in your head, and almost everyone finds that strange at first. While you sing you hear yourself through the air and through the bones of your head, "
          "which adds low end; a recording has only the air part, closer to what others hear. Listen for where you came in late or drifted off the note, then record another take and keep the best.",
          { { Guide::recording, "Takes and loop recording" }, { Guide::singing, "Singing in tune" } } }
    };
}

juce::String getTitle() { return "Sing karaoke to a song"; }

juce::String getIntro()
{
    return "Pick a song you like and sing it yourself. DawIt takes the singer out, finds the words and shows them as they're sung. Then you record a take and hear how you sound. "
           "Everything runs on this computer. Use headphones from step 4 on.";
}

juce::String getHelpDescription() { return "Take the vocals out of a song you like, see its words light up, sing along, then record yourself and listen back."; }

juce::String getCompletionText()
{
    return juce::String::fromUTF8 ("You made a karaoke version of a song, sang it from the words and heard yourself back. Record a few more takes and keep the best one (How recording works \xc2\xb7 Takes and loop recording). "
           "The singing guide shows how to breathe, warm up and sing in tune.");
}

int clampStep (int stored) noexcept { return stored >= 0 && stored < numSteps ? stored : 0; }

//==============================================================================
KaraokeRecord makeKaraokeRecord (const Engine& engine, const std::vector<Track*>& added, const Track* vocals)
{
    KaraokeRecord record;
    bool any = false;

    const auto take = [&] (const Track* track)
    {
        if (track == nullptr || engine.indexOf (track) < 0)
            return;

        for (auto* t : record.tracks)
            if (t == track)
                return;

        record.tracks.push_back (track);

        for (auto& c : track->clips)
            if (auto* clip = asAudio (c.get()))
            {
                const double end = clip->getEndBeat (engine.getTempoMap());
                record.startBeat = any ? juce::jmin (record.startBeat, clip->startBeat) : clip->startBeat;
                record.endBeat = any ? juce::jmax (record.endBeat, end) : end;
                any = true;
            }
    };

    for (auto* t : added)
        take (t);

    take (vocals);
    record.vocals = vocals != nullptr && engine.indexOf (vocals) >= 0 ? vocals : nullptr;
    return record;
}

TakeRecord makeTakeRecord (const Engine& engine)
{
    TakeRecord record;
    bool silent = false;

    for (auto& report : engine.getLastRecordingReport())
    {
        record.clips.insert (record.clips.end(), report.clips.begin(), report.clips.end());
        silent = silent || report.removedAsSilent;
    }

    record.silent = silent && record.clips.empty();
    return record;
}

//==============================================================================
bool isInEdit (const Engine& engine, const Clip* clip)
{
    for (auto& t : engine.getTracks())
        for (auto& c : t->clips)
            if (c.get() == clip)
                return true;

    return false;
}

const AudioClip* findSongClip (const Engine& engine, const TakeRecord& take)
{
    const AudioClip* best = nullptr;
    double bestLength = 0.0;
    const auto& tempo = engine.getTempoMap();

    for (auto& t : engine.getTracks())
        for (auto& c : t->clips)
        {
            auto* clip = asAudio (c.get());

            if (clip == nullptr || clip->muted || isTakeClip (take, clip))
                continue;

            const double length = clip->getLengthBeats (tempo);

            if (best == nullptr || length > bestLength + 1.0e-9 || (std::abs (length - bestLength) <= 1.0e-9 && clip->startBeat < best->startBeat))
            {
                best = clip;
                bestLength = length;
            }
        }

    return best;
}

songchords::Span getSongSpan (const Engine& engine, const Progress& progress)
{
    const auto& record = progress.karaoke;

    if (! record.tracks.empty() && record.endBeat > record.startBeat + eps)
        return spanOfBeats (engine, record.startBeat, record.endBeat);

    if (auto* clip = findSongClip (engine, progress.take))
        return songchords::getSpan (engine, *clip);

    return {};
}

const Track* findVocalsTrack (const Engine& engine, const Progress& progress)
{
    if (progress.karaoke.vocals != nullptr && engine.indexOf (progress.karaoke.vocals) >= 0)
        return progress.karaoke.vocals;

    const auto span = getSongSpan (engine, progress);

    if (span.size() <= 0)
        return nullptr;

    const double barLength = engine.getBarLength();
    const double start = span.firstBar * barLength, end = span.endBar * barLength;

    for (auto& t : engine.getTracks())
    {
        if (! t->isAudio() || ! t->name.trim().equalsIgnoreCase ("Vocals"))
            continue;

        for (auto& c : t->clips)
            if (auto* clip = asAudio (c.get()))
                if (clip->startBeat < end - eps && start < clip->getEndBeat (engine.getTempoMap()) - eps)
                    return t.get();
    }

    return nullptr;
}

const AudioClip* findVocalsClip (const Engine& engine, const Progress& progress)
{
    if (auto* vocals = findVocalsTrack (engine, progress))
        for (auto& c : vocals->clips)
            if (auto* clip = asAudio (c.get()))
                return clip;

    return nullptr;
}

const AudioClip* findDetectClip (const Engine& engine, const Progress& progress)
{
    if (auto* clip = findVocalsClip (engine, progress))
        return clip;

    return findSongClip (engine, progress.take);
}

const AudioClip* findRemoveClip (const Engine& engine, const Progress& progress) { return findSongClip (engine, progress.take); }

/** Muted: the track, a bus or solo routing silences it, or it has clips and all are muted. */
static bool isVocalsMuted (const Engine& engine, const Track& vocals)
{
    if (routing::getSilence (vocals, engine.getTracks()) != routing::Silence::none)
        return true;

    bool anyClip = false, allMuted = true;

    for (auto& c : vocals.clips)
        if (asAudio (c.get()) != nullptr)
        {
            anyClip = true;
            allMuted = allMuted && c->muted;
        }

    return anyClip && allMuted;
}

bool areVocalsOut (const Engine& engine, const Track& vocals)
{
    return isVocalsMuted (engine, vocals) || vocals.volumeDb.load() <= loudestGuideDb + 0.05;
}

int getBarOf (const Engine& engine, double beat) { return (int) std::floor (beat / engine.getBarLength() + eps); }

bool barHoldsSyllable (const Engine& engine, int bar)
{
    const double barLength = engine.getBarLength();

    for (auto& line : engine.getLyrics().lines)
        for (auto& syllable : line.syllables)
            if (syllable.beat >= bar * barLength - eps && syllable.beat < (bar + 1) * barLength - eps)
                return true;

    return false;
}

bool isSungBar (const Engine& engine, const Progress& progress, int bar)
{
    return getSongSpan (engine, progress).contains (bar) && barHoldsSyllable (engine, bar);
}

int countTimedLines (const Engine& engine, const Progress& progress)
{
    const auto span = getSongSpan (engine, progress);

    if (span.size() <= 0)
        return 0;

    const double barLength = engine.getBarLength();
    int n = 0;

    for (auto& line : engine.getLyrics().lines)
        if (writelyrics::isLineTimed (line) && line.startBeat >= span.firstBar * barLength - eps && line.startBeat < span.endBar * barLength - eps)
            ++n;

    return n;
}

const Track* findTrackOf (const Engine& engine, const AudioClip& clip)
{
    for (auto& t : engine.getTracks())
        for (auto& c : t->clips)
            if (c.get() == &clip)
                return t.get();

    return nullptr;
}

bool isClipHeard (const Engine& engine, const AudioClip& clip)
{
    auto* track = findTrackOf (engine, clip);
    return track != nullptr && ! clip.muted && routing::getSilence (*track, engine.getTracks()) == routing::Silence::none;
}

/** Whether a bar with words overlaps the clip. */
static bool clipOverlapsWords (const Engine& engine, const AudioClip& clip)
{
    const double barLength = engine.getBarLength();
    const double end = clip.getEndBeat (engine.getTempoMap());

    for (auto& line : engine.getLyrics().lines)
        for (auto& syllable : line.syllables)
        {
            const double barStart = std::floor (syllable.beat / barLength + eps) * barLength;

            if (barStart < end - eps && clip.startBeat < barStart + barLength - eps)
                return true;
        }

    return false;
}

const AudioClip* findTake (const Engine& engine, const Progress& progress, bool preferHeard)
{
    const AudioClip* fallback = nullptr;

    for (auto* candidate : progress.take.clips)
    {
        if (! isInEdit (engine, candidate))
            continue;

        // The address is in the edit now, so it is safe to read.
        if (candidate->getLengthBeats (engine.getTempoMap()) / engine.getBarLength() < takeBarsNeeded - eps || ! clipOverlapsWords (engine, *candidate))
            continue;

        if (! preferHeard || isClipHeard (engine, *candidate))
            return candidate;

        if (fallback == nullptr)
            fallback = candidate;
    }

    return fallback;
}

int getListenBarsNeeded (const Engine& engine, const AudioClip& take)
{
    const int bars = (int) std::floor (take.getLengthBeats (engine.getTempoMap()) / engine.getBarLength() + eps);
    return juce::jlimit (1, listenBarsNeeded, bars);
}

int getRecordStartBar (const Engine& engine, const Progress& progress)
{
    const auto span = getSongSpan (engine, progress);
    const double barLength = engine.getBarLength();
    const auto& lines = engine.getLyrics().lines;
    int words = span.firstBar;
    bool found = false;

    for (int pass = 0; pass < 2 && ! found; ++pass)
        for (auto& line : lines)
            if ((pass == 1 || writelyrics::isLineTimed (line)) && line.startBeat >= span.firstBar * barLength - eps
                && (span.size() <= 0 || line.startBeat < span.endBar * barLength - eps))
            {
                words = getBarOf (engine, line.startBeat);
                found = true;
                break;
            }

    return juce::jmax (0, words - 1);
}

RecordTarget findRecordTarget (const Engine& engine)
{
    RecordTarget target;

    for (auto& t : engine.getTracks())
        if (t->isAudio() && t->armed.load())
        {
            target.kind = RecordTarget::Kind::armed;
            target.track = t.get();
            return target;
        }

    for (auto& t : engine.getTracks())
        if (t->isAudio() && t->clips.empty())
        {
            target.kind = RecordTarget::Kind::unarmed;
            target.track = t.get();
            return target;
        }

    return target;
}

//==============================================================================
bool isStepImportDone (const Engine& engine, const Progress& progress)
{
    auto* clip = findSongClip (engine, progress.take);
    return clip != nullptr && songchords::getClipBars (engine, *clip) >= barsNeeded - eps;
}

bool isStepVocalsDone (const Engine& engine, const Progress& progress)
{
    if (! isStepImportDone (engine, progress))
        return false;

    auto* vocals = findVocalsTrack (engine, progress);

    if (vocals == nullptr || ! areVocalsOut (engine, *vocals))
        return false;

    const auto span = getSongSpan (engine, progress);
    const double barLength = engine.getBarLength();
    const double start = span.firstBar * barLength, end = span.endBar * barLength;

    // Another clip that is heard covers at least 4 of the song's bars.
    for (auto& t : engine.getTracks())
    {
        if (t.get() == vocals || ! t->isAudio() || routing::getSilence (*t, engine.getTracks()) != routing::Silence::none)
            continue;

        for (auto& c : t->clips)
            if (auto* clip = asAudio (c.get()); clip != nullptr && ! clip->muted && ! isTakeClip (progress.take, clip))
            {
                const double overlap = juce::jmin (end, clip->getEndBeat (engine.getTempoMap())) - juce::jmax (start, clip->startBeat);

                if (overlap / barLength >= barsNeeded - eps)
                    return true;
            }
    }

    return false;
}

bool isStepWordsDone (const Engine& engine, const Progress& progress) { return countTimedLines (engine, progress) >= linesNeeded; }

bool isStepSingAlongDone (const Progress& progress) noexcept { return progress.sungBars >= singBarsNeeded; }

bool isStepRecordDone (const Engine& engine, const Progress& progress) { return findTake (engine, progress) != nullptr; }

bool isStepListenDone (const Engine& engine, const Progress& progress)
{
    auto* take = findTake (engine, progress, true);
    return take != nullptr && progress.listenedBars >= getListenBarsNeeded (engine, *take);
}

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepImport:    return isStepImportDone (engine, progress);
        case stepVocals:    return isStepVocalsDone (engine, progress);
        case stepWords:     return isStepWordsDone (engine, progress);
        case stepSingAlong: return isStepSingAlongDone (progress);
        case stepRecord:    return isStepRecordDone (engine, progress);
        case stepListen:    return isStepListenDone (engine, progress);
        default: break;
    }

    return false;
}

//==============================================================================
juce::String getDoText (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepImport:
            return "Import a recording of a song with a singer, at least 4 bars long (8 seconds at 120 BPM).";
        case stepVocals:
            return "Click Remove vocals. In the window, choose whether the singer stays faintly as a guide, then let it run. It adds a Karaoke track without the singer and a Vocals track under it, and mutes your original clip.";
        case stepWords:
            return "Click Detect lyrics. It listens to the Vocals track, so the singer is already on their own, and puts the words on the Lyrics row, each syllable where it's sung.";
        case stepSingAlong:
            return "Open Sing along (Cmd+5), press Play and sing. The colour runs over each syllable while it's sung; after a rest, four dots count you in. Sing through 4 bars with words.";
        case stepRecord:
            return "Arm a track with R, then click Record from bar " + juce::String (getRecordStartBar (engine, progress) + 1)
                   + ". DawIt starts one bar before the first words, so you have time to come in. Sing at least 2 bars, then press Stop.";
        case stepListen:
            return "Click Play your take and listen through it, with the karaoke version. Listen once just for the timing, once for the pitch.";
        default: break;
    }

    return {};
}

static juce::String clipNameOrDefault (const AudioClip* clip) { return clip != nullptr ? songchords::getClipName (*clip) : juce::String ("your recording"); }

Action getAction (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepImport:
            return { "Import audio...", "Choose an audio file and put it on a new track (Cmd+I)", Fix::none };
        case stepVocals:
            return { "Remove vocals...", "Open Remove vocals (karaoke version) for \"" + clipNameOrDefault (findRemoveClip (engine, progress)) + "\"", Fix::none };
        case stepWords:
            return { "Detect lyrics...", "Open Detect lyrics for \"" + clipNameOrDefault (findDetectClip (engine, progress)) + "\"", Fix::none };
        case stepSingAlong:
            return { "Open Sing along", "Show the words in large letters, coloured as they're sung (Cmd+5)", Fix::none };
        case stepRecord:
            return { "Record from bar " + juce::String (getRecordStartBar (engine, progress) + 1),
                     "Start recording on the armed track one bar before the first line of words. Press Stop when you're done.", Fix::none };
        case stepListen:
            return { "Play your take", "Move the playhead to the start of your take and play", Fix::none };
        default: break;
    }

    return {};
}

juce::String getFixLabel (Fix fix)
{
    switch (fix)
    {
        case Fix::importAudio:        return "Import audio...";
        case Fix::detectLyrics:       return "Detect lyrics...";
        case Fix::audioSettings:      return "Audio/MIDI Settings...";
        case Fix::addRecordingTrack:  return "Add a Recording track";
        case Fix::none:
        case Fix::detectTempo:
        case Fix::detectChords:
        case Fix::detectSections:
        case Fix::openSections:
        case Fix::openLyrics:         break;
    }

    return {};
}

juce::String getFixTooltip (Fix fix)
{
    switch (fix)
    {
        case Fix::importAudio:        return "Choose an audio file and put it on a new track (Cmd+I)";
        case Fix::detectLyrics:       return "Open Detect lyrics, to find the words of the song";
        case Fix::audioSettings:      return "Opens the audio settings, where you choose where the sound comes out and what you record from.";
        case Fix::addRecordingTrack:  return "Add an audio track called Recording and show its R button";
        case Fix::none:
        case Fix::detectTempo:
        case Fix::detectChords:
        case Fix::detectSections:
        case Fix::openSections:
        case Fix::openLyrics:         break;
    }

    return {};
}

Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    if (step <= stepImport || step >= numSteps || isStepDone (step, engine, progress))
        return {};

    if (! isStepImportDone (engine, progress))
        return { "Bring in a song first (step 1).", Fix::importAudio };

    const auto noOutput = juce::String ("No audio output is chosen. Choose one in Audio/MIDI Settings to hear the song.");

    switch (step)
    {
        case stepVocals:
        {
            if (auto* vocals = findVocalsTrack (engine, progress); vocals != nullptr && ! areVocalsOut (engine, *vocals))
                return { juce::String::fromUTF8 ("The Vocals track is louder than \xe2\x88\x92" "6 dB. Turn its fader down, or press M on it to mute it."), Fix::none };

            if (! progress.demucsInstalled)
                return { "The first time, Demucs (" + models::formatSize (stems::getModelDownloadSize()) + ") is downloaded from Hugging Face. Remove vocals asks before it downloads and shows its progress. "
                         "Splitting takes about as long as the song on a fast computer, and longer on an older one.", Fix::none };

            return {};
        }

        case stepWords:
        {
            const bool hasVocals = findVocalsTrack (engine, progress) != nullptr;
            const auto whisperSize = models::formatSize (analysis::lyricsfinder::getModel (analysis::lyricsfinder::ModelSize::small).size);
            juce::String text = progress.whisperInstalled ? "Whisper small (" + whisperSize + ") is downloaded."
                                                         : "Whisper small is a " + whisperSize + " download the first time.";

            if (hasVocals)
                text << " Demucs isn't needed again: the Vocals track has the singer on their own.";
            else
                text << " Without a Vocals track, Detect lyrics finds the vocals first with Demucs"
                     << (progress.demucsInstalled ? juce::String (".") : " (" + models::formatSize (stems::getModelDownloadSize()) + " download the first time).");

            return { text, Fix::none };
        }

        case stepSingAlong:
        {
            if (! isStepWordsDone (engine, progress))
                return { "Find the words first (step 3).", Fix::detectLyrics };

            if (! progress.hasOutputDevice)
                return { noOutput, Fix::audioSettings };

            if (progress.sungBars > 0)
                return { "Sung along: " + juce::String (progress.sungBars) + " of " + juce::String (singBarsNeeded) + " bars", Fix::none };

            return {};
        }

        case stepRecord:
        {
            if (! isStepWordsDone (engine, progress))
                return { "Find the words first (step 3).", Fix::detectLyrics };

            if (! progress.hasInput)
                return { "No audio input was found. Connect a microphone or an audio interface, then choose it in Audio/MIDI Settings.", Fix::audioSettings };

            const auto target = findRecordTarget (engine);

            if (target.kind == RecordTarget::Kind::none)
                return { "Add an audio track to record on.", Fix::addRecordingTrack };

            if (target.kind == RecordTarget::Kind::unarmed)
                return { "Press R on " + target.track->name + " to arm it.", Fix::none };

            if (progress.take.silent)
                return { "The last take came in silent, so it was removed. Check the input chip next to R and the level (Setting the level).", Fix::none };

            return {};
        }

        case stepListen:
        {
            if (findTake (engine, progress) == nullptr)
                return { "Record a take first (step 5).", Fix::none };

            if (progress.recording)
                return { "Stop recording first.", Fix::none };

            if (! progress.hasOutputDevice)
                return { noOutput, Fix::audioSettings };

            if (auto* take = findTake (engine, progress, true); take != nullptr && progress.listenedBars > 0)
                return { "Heard " + juce::String (progress.listenedBars) + " of " + juce::String (getListenBarsNeeded (engine, *take)) + " bars of your take.", Fix::none };

            return {};
        }

        default: break;
    }

    return {};
}

juce::String getDoneNote (int step, const Engine& engine, const Progress& progress)
{
    if (! isStepDone (step, engine, progress))
        return {};

    switch (step)
    {
        case stepImport:
        {
            auto* clip = findSongClip (engine, progress.take);
            return clip != nullptr ? "\"" + songchords::getClipName (*clip) + "\", " + barsText (songchords::getClipBars (engine, *clip)) + " bars long." : juce::String();
        }

        case stepVocals:
        {
            auto* vocals = findVocalsTrack (engine, progress);

            if (vocals == nullptr)
                return {};

            if (isVocalsMuted (engine, *vocals))
                return "The vocals are muted.";

            return "The vocals play at " + dbText (vocals->volumeDb.load()) + " dB as a guide. Press M on Vocals when you know the tune.";
        }

        case stepWords:
        {
            const auto span = getSongSpan (engine, progress);
            const double barLength = engine.getBarLength();
            double first = 1.0e12, last = -1.0e12;
            int n = 0;

            for (auto& line : engine.getLyrics().lines)
                if (writelyrics::isLineTimed (line) && line.startBeat >= span.firstBar * barLength - eps && line.startBeat < span.endBar * barLength - eps)
                {
                    first = juce::jmin (first, line.startBeat);
                    last = juce::jmax (last, line.endBeat);
                    ++n;
                }

            return juce::String (n) + " lines of words, from bar " + juce::String (getBarOf (engine, first) + 1) + " to bar "
                   + juce::String (juce::jmax (getBarOf (engine, first), getBarOf (engine, last - 1.0e-3)) + 1) + ".";
        }

        case stepSingAlong:
            return "You sang along through " + juce::String (singBarsNeeded) + " bars.";

        case stepRecord:
        {
            auto* take = findTake (engine, progress);

            if (take == nullptr)
                return {};

            auto* track = findTrackOf (engine, *take);
            return "Your take is " + barsText (songchords::getClipBars (engine, *take)) + " bars long, on " + (track != nullptr ? track->name : juce::String ("a track")) + ".";
        }

        case stepListen:
            return "You listened back to your take.";

        default: break;
    }

    return {};
}
} // namespace daw::gen::karaoke
