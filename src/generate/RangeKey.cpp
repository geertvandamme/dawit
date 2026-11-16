#include "RangeKey.h"

#include "TheoryGuides.h"
#include "engine/Routing.h"

#include <cmath>

namespace daw::gen::rangekey
{
using guides::Guide;
namespace sg = singing;

namespace
{
    constexpr double eps = 1.0e-6;

    juce::String semitonesText (int n) { return juce::String (n) + (n == 1 ? " semitone" : " semitones"); }

    juce::String directionText (int shift) { return shift < 0 ? "lower" : "higher"; }

    juce::String rangeNames (sg::Range r) { return sg::getNoteName (r.lowest) + " to " + sg::getNoteName (r.highest); }

    bool isMelodyOk (const Engine& engine, const Progress& progress)
    {
        auto* track = findMelodyTrack (engine, progress);
        return track != nullptr && countNotes (*track) >= melodyNotesNeeded;
    }

    /** The recordings' status: a key control number to use elsewhere, "−4", and "4 semitones lower". */
    juce::String blockedText (const sg::Analysis& a, int shift)
    {
        const auto minus = juce::String::fromUTF8 ("\xe2\x88\x92");
        return "Your backing is a recording, which DawIt can't move yet. To sing it in " + (a.newKeyName.isNotEmpty() ? a.newKeyName : juce::String ("the new key"))
               + ", set a karaoke player's key control to " + (shift < 0 ? minus : juce::String ("+")) + juce::String (std::abs (shift))
               + ", or ask your band for it " + semitonesText (std::abs (shift)) + " " + directionText (shift) + ".";
    }
}

std::vector<Step> getSteps()
{
    return {
        { "Record your range", "Low and high, held for 2 seconds",
          "DawIt can only fit the song to your voice once it knows your notes. Warm up first: a warmed-up voice reaches further, more safely.",
          { { Guide::singing, "Measuring your range" }, { Guide::singing, "Warming up" } } },
        { "Keep it as your range", "Measure the take and use it",
          "DawIt finds the notes you held steady and names the closest voice type. Check by ear that it heard the right octave before you keep it.",
          { { Guide::singing, "Measuring your range" }, { Guide::singing, "Voice types and range" } } },
        { "Find the song's melody", "The tune you'll sing, as notes",
          "DawIt compares notes, so the tune has to be on a MIDI track. A recording of you singing it can be turned into notes with Convert to MIDI...",
          { { Guide::singing, "Your voice and your song" } } },
        { "Choose a key that suits you", "Move the song, not your voice",
          "Moving every note by the same number of semitones keeps the song the same, only higher or lower.",
          { { Guide::singing, "Choosing a key for your voice" }, { Guide::singing, "Your voice and your song" } } },
        { "Hear it in the new key", "Play it and sing along",
          "A key that looks right on paper can still feel wrong. Sing a chorus: if the top notes strain, try a semitone lower.",
          { { Guide::singing, "Choosing a key for your voice" }, { Guide::singing, "Singing in tune" } } }
    };
}

juce::String getTitle() { return "Find your range and the right key"; }

juce::String getIntro()
{
    return "Find out which notes you can sing, check them against your song's melody, and move the song so it fits your voice. "
           "You need a microphone, and a melody on a MIDI track (or a recording of it to turn into notes).";
}

juce::String getHelpDescription() { return "Five steps: measure your voice, compare it with your melody and move the song to a key that suits you"; }

juce::String getCompletionText()
{
    return "Keep trying the song by ear to check that it feels comfortable. Your range is saved for every song, so Your voice and your song checks each new melody against it. "
           "Measure again in a few weeks: a beginner's range often grows.";
}

int clampStep (int stored) noexcept { return stored >= 0 && stored < numSteps ? stored : 0; }

//==============================================================================
const AudioClip* findTake (const Engine& engine, const Progress& progress)
{
    const AudioClip* found = nullptr;

    for (auto* candidate : progress.take.clips)
        if (karaoke::isInEdit (engine, candidate) && candidate->getLengthSeconds() >= takeSecondsNeeded - eps)
            found = candidate;   // the last one is the newest

    return found;
}

int countNotes (const Track& track)
{
    int n = 0;

    for (auto& c : track.clips)
        if (c->kind == Clip::Kind::midi)
            n += (int) static_cast<const MidiClip&> (*c).notes.size();

    return n;
}

const Track* findMelodyTrack (const Engine& engine, const Progress& progress)
{
    std::vector<const Track*> tracks;
    std::vector<sg::TrackInfo> infos;

    for (auto& t : engine.getTracks())
        if (t->isMidi())
        {
            tracks.push_back (t.get());
            infos.push_back ({ t->name, countNotes (*t) > 0, t->usesBuiltInSynth() && t->builtInSound.load() == BuiltInSound::drums });
        }

    // The guide's own choice stands while that track is still there.
    if (progress.chosenMelody != nullptr && std::find (tracks.begin(), tracks.end(), progress.chosenMelody) != tracks.end())
        return progress.chosenMelody;

    const auto it = std::find (tracks.begin(), tracks.end(), progress.selectedTrack);
    const int index = sg::chooseMelodyTrack (infos, it == tracks.end() ? -1 : (int) (it - tracks.begin()));
    return index >= 0 ? tracks[(size_t) index] : nullptr;
}

singthis::Melody buildMelodyOf (const Engine& engine, const Track* track)
{
    std::vector<singthis::SourceNote> source;

    if (track != nullptr)
        for (auto& c : track->clips)
            if (c->kind == Clip::Kind::midi)
                for (auto& n : static_cast<const MidiClip&> (*c).notes)
                    source.push_back ({ n.pitch, c->startBeat + n.start, n.length, n.velocity });

    return singthis::buildMelody (std::move (source), engine.getTempoMap());
}

sg::Analysis analyseMelody (const Engine& engine, const Progress& progress)
{
    const auto* track = findMelodyTrack (engine, progress);
    return sg::analyse (buildMelodyOf (engine, track), track != nullptr ? track->name : juce::String(), engine.getSections(), engine.getKey(),
                        engine.getLyrics(), engine.getTempo(), progress.range);
}

std::set<int> getMelodyBars (const Engine& engine, const Track* track)
{
    std::set<int> bars;

    if (track == nullptr)
        return bars;

    const double barLength = engine.getBarLength();

    for (auto& c : track->clips)
        if (c->kind == Clip::Kind::midi)
            for (auto& n : static_cast<const MidiClip&> (*c).notes)
            {
                const double start = c->startBeat + n.start;
                bars.insert ((int) std::floor (start / barLength + eps));
            }

    return bars;
}

const AudioClip* findConvertClip (const Engine& engine, const Progress& progress)
{
    for (auto& t : engine.getTracks())
    {
        if (! t->isAudio() || ! (sg::hasVocalWord (t->name) || t->name.trim().equalsIgnoreCase ("Vocals")))
            continue;

        for (auto& c : t->clips)
            if (c->kind == Clip::Kind::audio)
            {
                const auto* clip = static_cast<const AudioClip*> (c.get());

                if (std::find (progress.take.clips.begin(), progress.take.clips.end(), clip) == progress.take.clips.end())
                    return clip;
            }
    }

    return nullptr;
}

int getOfferedShift (const Engine& engine, const Progress& progress)
{
    if (! progress.range.isSet() || ! isMelodyOk (engine, progress))
        return 0;

    const auto a = analyseMelody (engine, progress);
    using V = sg::Analysis::Verdict;

    switch (a.verdict)
    {
        case V::tooHigh:
        case V::tooLow:
            return a.octave ? 0 : a.shift;

        case V::nearTop:
        case V::nearBottom:
        {
            // Never offered opposite to this exercise's own last move.
            const auto& last = progress.lastMove;

            if (last.present && last.keyAfter == engine.getKey() && ((last.shift < 0) != (a.shift < 0)))
                return 0;

            return a.shift;
        }

        case V::noMelody:
        case V::noRange:
        case V::noKey:
        case V::fits:
            break;
    }

    return 0;
}

bool isNudge (const Engine& engine, const Progress& progress)
{
    return getOfferedShift (engine, progress) != 0 && isStepKeyDone (engine, progress);
}

songmove::BackingCheck checkBacking (const Engine& engine, const Progress& progress)
{
    std::vector<const Clip*> ignored (progress.take.clips.begin(), progress.take.clips.end());
    return songmove::checkBacking (engine, findMelodyTrack (engine, progress), ignored);
}

int getHeardBarsNeeded (const Engine& engine, const Progress& progress)
{
    const int bars = (int) getMelodyBars (engine, findMelodyTrack (engine, progress)).size();
    return juce::jlimit (1, hearBarsNeeded, bars);
}

//==============================================================================
bool isStepRecordDone (const Engine& engine, const Progress& progress) { return findTake (engine, progress) != nullptr || progress.range.isSet(); }

bool isStepKeepDone (const Progress& progress) noexcept { return progress.range.isSet(); }

bool isStepMelodyDone (const Engine& engine, const Progress& progress) { return isMelodyOk (engine, progress); }

bool isStepKeyDone (const Engine& engine, const Progress& progress)
{
    if (! progress.range.isSet() || ! isMelodyOk (engine, progress))
        return false;

    const auto a = analyseMelody (engine, progress);
    using V = sg::Analysis::Verdict;

    switch (a.verdict)
    {
        case V::fits:
        case V::nearTop:
        case V::nearBottom:
            return true;

        case V::tooHigh:
        case V::tooLow:
            return a.octave;

        case V::noMelody:
        case V::noRange:
        case V::noKey:
            break;
    }

    return false;
}

bool isStepHearDone (const Engine& engine, const Progress& progress)
{
    return isStepKeyDone (engine, progress) && ! progress.recording && progress.heardBars >= getHeardBarsNeeded (engine, progress);
}

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepRecord: return isStepRecordDone (engine, progress);
        case stepKeep:   return isStepKeepDone (progress);
        case stepMelody: return isStepMelodyDone (engine, progress);
        case stepKey:    return isStepKeyDone (engine, progress);
        case stepHear:   return isStepHearDone (engine, progress);
        default: break;
    }

    return false;
}

//==============================================================================
juce::String getDoText (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepRecord:
            return "Click Record, slide down to your lowest comfortable note and hold it for 2 seconds, then slide up to your highest comfortable note and hold it for 2 seconds. Press Stop.";
        case stepKeep:
            return "Click Measure this take. DawIt finds the notes you held, and the singing guide opens at Measuring your range: click Use as my range to keep them. "
                   "You can also set your lowest and highest notes by hand.";
        case stepMelody:
            return "Put the tune on a MIDI track with at least 4 notes: write it, or record yourself singing it and choose Convert to MIDI... in the clip's menu.";
        case stepKey:
        {
            if (progress.range.isSet() && isMelodyOk (engine, progress))
            {
                const auto a = analyseMelody (engine, progress);

                if (a.paragraphs.size() > 0)
                    return a.paragraphs[0];
            }

            return "Set your range and find the melody first: DawIt then compares the two and tells you which way to move the song.";
        }
        case stepHear:
        {
            const auto bar = *getMelodyBars (engine, findMelodyTrack (engine, progress)).begin() + 1;
            return "Click Play from bar " + juce::String (bar) + " and sing the melody. Hear " + juce::String (getHeardBarsNeeded (engine, progress))
                   + " bars of it in the new key.";
        }
        default: break;
    }

    return {};
}

Action getAction (int step, const Engine& engine, const Progress& progress)
{
    Action a;

    switch (step)
    {
        case stepRecord:
        {
            const auto target = karaoke::findRecordTarget (engine);
            a.label = "Record";
            a.tooltip = "Record yourself on the armed track: slide down to your lowest comfortable note and hold it, then up to your highest and hold it.";
            a.op = Op::record;
            a.enabled = target.kind == karaoke::RecordTarget::Kind::armed && ! progress.recording;

            if (target.kind == karaoke::RecordTarget::Kind::none)
            {
                a.fix = Fix::addRecordingTrack;
                a.enabled = true;
                a.op = Op::addRecordingTrack;
            }

            if (! progress.hasInput)
            {
                a.fix = Fix::audioSettings;
                a.op = Op::audioSettings;
                a.enabled = true;
            }

            return a;
        }

        case stepKeep:
            if (findTake (engine, progress) != nullptr)
            {
                a.label = "Measure this take";
                a.tooltip = "Find the notes you held in the take. Nothing in the song changes.";
                a.op = Op::measureTake;
            }
            else
            {
                a.label = "Set it by hand...";
                a.tooltip = "Open Voice types and range, where you choose your lowest and highest notes yourself.";
                a.op = Op::setByHand;
            }

            return a;

        case stepMelody:
            a.label = "Convert to MIDI...";
            a.op = Op::convertToMidi;

            if (auto* clip = findConvertClip (engine, progress); clip != nullptr)
                a.tooltip = "Turn the recording \"" + (clip->name.isNotEmpty() ? clip->name : juce::String ("your recording")) + "\" into MIDI notes you can compare with your range.";
            else
            {
                a.tooltip = "Turns a recording of the melody into notes. It needs an audio clip on a vocal track, such as Vocals.";
                a.enabled = false;
            }

            return a;

        case stepKey:
        {
            const int shift = getOfferedShift (engine, progress);
            a.op = Op::moveSong;
            a.tooltip = "Move the key, the chords and every MIDI note except drums by " + juce::String (shift != 0 ? std::abs (shift) : 0)
                        + " semitones, as one step you can undo. Recordings stay as they are.";
            a.shift = shift;

            if (shift == 0)
            {
                a.label = "Move the song";
                a.tooltip = "Move the key, the chords and every MIDI note except drums by N semitones, as one step you can undo. Recordings stay as they are.";
                a.enabled = false;
                return a;
            }

            a.label = "Move the song " + semitonesText (std::abs (shift)) + " " + directionText (shift);
            a.enabled = ! checkBacking (engine, progress).blocked && songmove::check (engine, shift).refusal == songmove::Refusal::none;
            return a;
        }

        case stepHear:
        {
            const auto bars = getMelodyBars (engine, findMelodyTrack (engine, progress));
            a.bar = bars.empty() ? 0 : *bars.begin();
            a.label = "Play from bar " + juce::String (a.bar + 1);
            a.tooltip = "Play the song from the first bar of the melody, to hear and sing it in the new key.";
            a.op = Op::playFromBar;
            a.enabled = progress.hasOutputDevice && ! progress.recording;
            return a;
        }

        default: break;
    }

    return a;
}

Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    if (step < 0 || step >= numSteps || isStepDone (step, engine, progress))
        return {};

    switch (step)
    {
        case stepRecord:
        {
            if (! progress.hasInput)
                return { "No microphone found: choose an input in Audio settings.", Fix::audioSettings };

            if (progress.recording)
                return { "Recording... hold your highest note, then press Stop.", Fix::none };

            const auto target = karaoke::findRecordTarget (engine);

            if (target.kind == karaoke::RecordTarget::Kind::none)
                return { "There is no recording track yet.", Fix::addRecordingTrack };

            if (target.kind == karaoke::RecordTarget::Kind::unarmed)
                return { "Press R on '" + target.track->name + "' to arm it", Fix::none };

            return {};
        }

        case stepKeep:
        {
            const auto& m = progress.measured;

            if (m.present && m.range.isSet() && m.range != progress.range)
                return { "DawIt measured " + rangeNames (m.range) + " in '" + m.clipName + "'. Click Use as my range in the guide to keep it.", Fix::none };

            if (findTake (engine, progress) == nullptr)
                return { "Record a take first (step 1), or set your range by hand.", Fix::none };

            return {};
        }

        case stepMelody:
        {
            if (auto* clip = findConvertClip (engine, progress); clip != nullptr)
                return { "Your melody is a recording (\"" + (clip->name.isNotEmpty() ? clip->name : juce::String ("your recording"))
                         + "\"). Convert to MIDI... turns it into notes.", Fix::none };

            return { "Write the tune on a MIDI track, or record yourself singing it and choose Convert to MIDI... in the clip's menu.", Fix::none };
        }

        case stepKey:
        {
            if (! progress.range.isSet())
                return { "Set your range first (step 2).", Fix::none };

            if (! isMelodyOk (engine, progress))
                return { "Find the melody first (step 3).", Fix::none };

            const auto a = analyseMelody (engine, progress);

            if (a.verdict == sg::Analysis::Verdict::noKey)
                return { "No key fits all of it, so there is nothing to move. Skip this step, or change a few of the highest or lowest notes.", Fix::none };

            const int shift = getOfferedShift (engine, progress);

            if (shift == 0)
                return {};

            if (const auto backing = checkBacking (engine, progress); backing.blocked)
                return { blockedText (a, shift), Fix::none };

            if (const auto check = songmove::check (engine, shift); check.refusal != songmove::Refusal::none)
                return { check.message, Fix::none };

            if (const auto backing = checkBacking (engine, progress); ! backing.recordingsLeft.empty())
            {
                juce::StringArray names;

                for (auto& n : backing.recordingsLeft)
                    names.add ("'" + n + "'");

                return { names.joinIntoString (", ") + " stays in the old key: DawIt can't move recordings.", Fix::none };
            }

            return {};
        }

        case stepHear:
        {
            if (! isStepKeyDone (engine, progress))
                return { "Choose a key first (step 4).", Fix::none };

            if (! progress.hasOutputDevice)
                return { "No audio output is chosen. Choose one in Audio/MIDI Settings to hear the song.", Fix::audioSettings };

            if (progress.recording)
                return { "Stop recording first.", Fix::none };

            if (progress.heardBars > 0)
                return { "Heard: " + juce::String (progress.heardBars) + " of " + juce::String (getHeardBarsNeeded (engine, progress)) + " bars", Fix::none };

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
        case stepRecord:
            if (auto* take = findTake (engine, progress))
                return (take->name.isNotEmpty() ? take->name : juce::String ("Take")) + ": " + juce::String (take->getLengthSeconds(), 1) + " s.";

            return "Your range is already set. Record a new take to measure again.";

        case stepKeep:
        {
            const int type = sg::getClosestType (progress.range);
            return "Your range: " + rangeNames (progress.range) + ", " + semitonesText (progress.range.highest - progress.range.lowest)
                   + (type >= 0 ? " (closest: " + sg::getVoiceTypes()[(size_t) type].name.toLowerCase() + ")" : juce::String()) + ".";
        }

        case stepMelody:
        {
            const auto a = analyseMelody (engine, progress);
            return "The melody on '" + a.trackName + "': " + sg::getNoteName (a.lowest) + " to " + sg::getNoteName (a.highest) + ", "
                   + semitonesText (a.highest - a.lowest) + ".";
        }

        case stepKey:
        {
            const auto a = analyseMelody (engine, progress);

            if (a.octave)
                return "Sing it an octave " + directionText (a.shift) + ": nothing in the song has to change.";

            juce::String note = a.sparesTop > 0 ? "Your melody fits: " + semitonesText (a.sparesTop) + " to spare at the top."
                              : a.sparesBottom > 0 ? "Your melody fits: " + semitonesText (a.sparesBottom) + " to spare at the bottom."
                              : juce::String ("Your melody fits, from your lowest note to your highest.");

            if (const int shift = getOfferedShift (engine, progress); shift != 0)
                note << " Optional: " << semitonesText (std::abs (shift)) << " " << directionText (shift) << " may feel easier on your "
                     << (a.verdict == sg::Analysis::Verdict::nearTop ? "top" : "bottom") << " notes.";

            return note;
        }

        case stepHear:
            return "You heard " + juce::String (progress.heardBars) + " bars in " + engine.getKey().getName() + ".";

        default: break;
    }

    return {};
}
} // namespace daw::gen::rangekey
