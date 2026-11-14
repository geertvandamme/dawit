#include "SongChords.h"

#include <cmath>

namespace daw::gen::songchords
{
using guides::Guide;

static juce::String swapWhy (const Swap&);

std::vector<Step> getSteps()
{
    return {
        { "Bring in a song", "Import a recording you like",
          "A recording is audio: the sound itself, not the notes. DawIt can't read its chords the way it reads a MIDI clip, so in the next step it listens for them.",
          { { Guide::midi, "What MIDI is" } } },
        { "Find its chords", "Let DawIt listen for one chord per bar",
          "In each bar DawIt measures how strongly each of the 12 note names sounds, in any octave, leaving out short sounds such as drums. Then it picks the "
          "major, minor or seventh chord that fits best, and the lowest note decides between close ones. That's how you find chords by ear too: which notes "
          "ring, and which is lowest. It needs no download. Check the result by ear: a busy bass, passing notes or a sung melody can fool it, and it can't name "
          "sus, diminished or slash chords, so it gives the nearest chord it knows.",
          { { Guide::keyFinding, "Naming a chord from its notes" }, { Guide::rhythm, "Tempo and time" } } },
        { "Play along", "Play the chords as the song plays",
          "Playing along is how most musicians learn songs: the recording keeps going and you fit in. Watch the next chord, not the one you're playing, and keep "
          "the rhythm going even when a change comes late. DawIt doesn't listen to what you play: the tick only says the song played with the chords in front of you.",
          { { Guide::chords, "Playing along" } } },
        { "Swap a chord", "Change one and hear the difference", swapWhy ({}),
          { { Guide::chords, "Your chord track" }, { Guide::chords, "How chords work" } } }
    };
}

juce::String getTitle() { return "Learn the chords of a song"; }

juce::String getIntro()
{
    return "Four steps with a song you like: bring it in, let DawIt find its chords, play along with them, then swap one and hear what changes. "
           "Do each step in the main window: the tick appears by itself.";
}

juce::String getCompletionText()
{
    return "Done. You found the chords of a song, played along with them and heard what one swap does. Next, try another song, or loop a hard change "
           "and practise it slowly: drag along the ruler to loop those bars.";
}

//==============================================================================
const AudioClip* findSongClip (const Engine& engine)
{
    const AudioClip* best = nullptr;
    double bestLength = 0.0;
    const auto& tempo = engine.getTempoMap();

    for (auto& t : engine.getTracks())
        for (auto& c : t->clips)
        {
            auto* clip = dynamic_cast<const AudioClip*> (c.get());

            if (clip == nullptr || clip->muted)
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

double getClipBars (const Engine& engine, const AudioClip& clip)
{
    return clip.getLengthBeats (engine.getTempoMap()) / engine.getBarLength();
}

Span getSpan (const Engine& engine, const AudioClip& clip)
{
    const double barLength = engine.getBarLength();
    const double start = clip.startBeat / barLength, end = clip.getEndBeat (engine.getTempoMap()) / barLength;
    Span span;
    span.firstBar = juce::jmax (0, (int) std::ceil (start - 1.0e-6));
    span.endBar = juce::jmax (span.firstBar, (int) std::floor (end + 1.0e-6));
    return span;
}

int countChords (const ChordMap& chords, Span span)
{
    int n = 0;

    for (auto& [bar, chord] : chords)
    {
        juce::ignoreUnused (chord);

        if (span.contains (bar))
            ++n;
    }

    return n;
}

juce::String getClipName (const AudioClip& clip)
{
    return clip.name.isNotEmpty() ? clip.name : juce::String ("your recording");
}

//==============================================================================
bool isStep1Done (const Engine& engine)
{
    auto* clip = findSongClip (engine);
    return clip != nullptr && getClipBars (engine, *clip) >= barsNeeded - 1.0e-6;
}

bool isStep2Done (const Engine& engine)
{
    auto* clip = findSongClip (engine);
    return clip != nullptr && countChords (engine.getChords(), getSpan (engine, *clip)) >= barsNeeded;
}

bool isStep3Done (int playedAlongBars) noexcept { return playedAlongBars >= barsNeeded; }

bool isStep4Done (const Engine& engine, const ChordMap& baseline)
{
    auto* clip = findSongClip (engine);

    if (clip == nullptr)
        return false;

    const auto span = getSpan (engine, *clip);
    const auto& now = engine.getChords();

    for (auto& [bar, was] : baseline)
        if (span.contains (bar))
            if (auto it = now.find (bar); it != now.end() && ! (it->second == was))
                return true;

    return false;
}

bool isStepDone (int step, const Engine& engine, const ChordMap& baseline, int playedAlongBars)
{
    switch (step)
    {
        case 0: return isStep1Done (engine);
        case 1: return isStep2Done (engine);
        case 2: return isStep3Done (playedAlongBars);
        case 3: return isStep4Done (engine, baseline);
        default: break;
    }

    return false;
}

int clampStep (int stored) noexcept
{
    return stored >= 0 && stored < numSteps ? stored : 0;
}

//==============================================================================
juce::String Swap::getSharedText() const
{
    juce::StringArray names;

    for (int pc : shared)
        names.add (MusicalKey::getTonicName (pc));

    return names.size() < 2 ? names.joinIntoString (", ") : names.joinIntoString (", ", 0, names.size() - 1) + " and " + names[names.size() - 1];
}

Swap getSwapFor (int bar, const Chord& chord)
{
    Swap swap;
    const bool major = chord.intervals.contains (4);
    const bool minor = ! major && chord.intervals.contains (3) && ! chord.intervals.contains (6) && ! chord.intervals.contains (8);

    if (! major && ! minor)
        return swap;

    swap.valid = true;
    swap.bar = bar;
    swap.from = chord;
    swap.to = Chord { (chord.root + (major ? 9 : 3)) % 12, major ? juce::Array<int> { 0, 3, 7 } : juce::Array<int> { 0, 4, 7 } };

    const juce::Array<int> fromTriad { chord.root, (chord.root + (major ? 4 : 3)) % 12, (chord.root + 7) % 12 };
    const auto toNotes = swap.to.getPitchClasses();

    for (int pc : fromTriad)
        if (toNotes.contains (pc))
            swap.shared.add (pc);

    return swap;
}

Swap getSwap (const Engine& engine, const ChordMap& baseline)
{
    auto* clip = findSongClip (engine);

    if (clip == nullptr)
        return {};

    const auto span = getSpan (engine, *clip);

    for (const ChordMap* map : { &baseline, &engine.getChords() })
        for (auto& [bar, chord] : *map)
            if (span.contains (bar))
                return getSwapFor (bar, chord);

    return {};
}

//==============================================================================
juce::String getDoText (int step, const juce::String& clipName, const Swap& swap)
{
    switch (step)
    {
        case 0:
            return "Press Import audio... (Cmd+I) and choose a song file on your computer, such as an MP3 or WAV file, or drag the file onto the tracks. DawIt puts it on "
                   "an audio track at the playhead. A song with a clear guitar or piano part and three or four chords is the easiest to start with.";

        case 1:
            return "Press Detect chords. DawIt listens to " + clipName + " and shows the chords it hears, one per bar of the song, and the key they sound like. "
                   "If it says the bars don't line up, choose Detect tempo first, set the song's tempo from the recording, then press Detect chords again. "
                   "When the chords look right, press Put on chord track: they appear on the Chords row above the tracks.";

        case 2:
            return "Press Open Play along (Cmd+4). It shows the chord playing now, large, as piano keys and a guitar chord box, with the next one beside it. "
                   "Press Space to play the song and play the chords on a piano, a keyboard or a guitar. If whole chords are too much at first, play only the "
                   "root, the note the chord is named after, then add the rest. The tick comes once you have played along through 4 bars with chords.";

        case 3:
        {
            const auto bar = swap.valid ? juce::String (swap.bar + 1) : juce::String ("1");
            const auto to = swap.valid ? swap.to.getName() : juce::String ("Am");
            const auto from = swap.valid ? swap.from.getName() : juce::String ("C");
            return "Click bar " + bar + " on the Chords row and choose " + to + " in place of " + from + ". The recording doesn't change: it still plays " + from
                   + ". Play your new chord along with it in Play along, or click it in the open Chords row to hear it on its own, and listen: does it fit, "
                     "clash, or change the mood? Then try a swap of your own. Undo (Cmd+Z) brings the found chord back.";
        }

        default: break;
    }

    return {};
}

static juce::String swapWhy (const Swap& swap)
{
    const bool major = ! swap.valid || swap.isMajor();
    const auto from = swap.valid ? swap.from.getName() : juce::String ("C");
    const auto to = swap.valid ? swap.to.getName() : juce::String ("Am");
    const auto shared = swap.valid ? swap.getSharedText() : juce::String ("C and E");

    return to + juce::String (major ? " is the relative minor of " : " is the relative major of ") + from
           + juce::String (major ? ": its root is 3 semitones lower" : ": its root is 3 semitones higher") + ", and they share two of their three notes, " + shared
           + ". Chords that share notes can often stand in for each other, which musicians call a substitution. That's why the swap still fits the song, and why it often sounds "
           + juce::String (major ? "darker." : "brighter.");
}

juce::String getWhy (int step, const Swap& swap)
{
    if (step == 3)
        return swapWhy (swap);

    const auto steps = getSteps();
    return step >= 0 && step < numSteps ? steps[(size_t) step].why : juce::String();
}

juce::String getActionLabel (int step)
{
    switch (step)
    {
        case 0: return "Import audio...";
        case 1: return "Detect chords";
        case 2: return "Open Play along";
        case 3: return "Open the Chords row";
        default: break;
    }

    return {};
}

juce::String getActionTooltip (int step, const juce::String& clipName, const Swap& swap)
{
    switch (step)
    {
        case 0: return "Opens Import Audio (Cmd+I): choose a song file to put on an audio track. Nothing else in the song changes.";
        case 1: return "Listens to " + clipName + " and shows the chords it finds. Nothing changes until you press Put on chord track.";
        case 2: return "Opens Play along with the chords (Cmd+4). Press Space to play; the window follows the song.";
        case 3: return "Unfolds the Chords row and points at bar " + (swap.valid ? juce::String (swap.bar + 1) : juce::String ("1")) + ". Click a bar to choose its chord.";
        default: break;
    }

    return {};
}

/** Bars to one decimal, rounded down so that 3.95 never reads as the 4 the exercise needs. */
static juce::String formatBars (double bars)
{
    const double tenths = std::floor (bars * 10.0 + 1.0e-6);
    return std::abs (tenths - 10.0 * std::floor (tenths / 10.0)) < 0.5 ? juce::String ((int) (tenths / 10.0)) : juce::String (tenths / 10.0, 1);
}

Status getStatus (int step, const Engine& engine, const ChordMap& baseline, int playedAlongBars, bool hasOutputDevice)
{
    juce::ignoreUnused (baseline);
    const auto* clip = findSongClip (engine);
    const bool longEnough = clip != nullptr && getClipBars (engine, *clip) >= barsNeeded - 1.0e-6;
    const juce::String noClip = "Detect chords needs a recording of at least 4 bars: step 1 brings one in.";
    const juce::String noChords = "Play along shows the chords on the Chords row, and there are none on your recording yet. Find them first (step 2).";

    switch (step)
    {
        case 0:
            if (clip != nullptr && ! longEnough)
                return { "Your longest recording, " + getClipName (*clip) + ", lasts " + formatBars (getClipBars (engine, *clip))
                         + " bars at the song's tempo. Chords are found one per bar, so this exercise needs at least 4: import a longer part of the song.", Fix::none };
            break;

        case 1:
            if (! longEnough)
                return { noClip, Fix::importAudio };
            break;

        case 2:
        case 3:
            if (! longEnough)
                return { noClip, Fix::importAudio };

            if (countChords (engine.getChords(), getSpan (engine, *clip)) == 0)
                return { noChords, Fix::detectChords };

            if (step == 2 && ! hasOutputDevice)
                return { "No audio output is chosen, so you won't hear the song. Choose one in Audio/MIDI Settings.", Fix::audioSettings };

            if (step == 2 && playedAlongBars > 0)
                return { "Played along: " + juce::String (juce::jmin (playedAlongBars, barsNeeded)) + " of " + juce::String (barsNeeded) + " bars.", Fix::none };

            break;

        default: break;
    }

    return {};
}

juce::String getDoneNote (int step, const Engine& engine, const ChordMap& baseline)
{
    const auto* clip = findSongClip (engine);

    if (clip == nullptr)
        return {};

    switch (step)
    {
        case 0: return getClipName (*clip) + " is " + formatBars (getClipBars (engine, *clip)) + " bars long at the song's tempo.";
        case 1: return juce::String (countChords (engine.getChords(), getSpan (engine, *clip))) + " bars of " + getClipName (*clip) + " have a chord.";

        case 3:
        {
            const auto span = getSpan (engine, *clip);

            for (auto& [bar, was] : baseline)
                if (span.contains (bar))
                    if (auto it = engine.getChords().find (bar); it != engine.getChords().end() && ! (it->second == was))
                        return "Bar " + juce::String (bar + 1) + ": " + was.getName() + " is now " + it->second.getName() + ".";

            break;
        }

        default: break;
    }

    return {};
}

//==============================================================================
juce::AudioBuffer<float> makeExampleRecording (double sampleRate, double bpm)
{
    const juce::StringArray names { "C", "G", "Am", "F", "C", "G", "Am", "F" };
    const double barSeconds = 4.0 * 60.0 / juce::jmax (1.0, bpm);
    const int barSamples = (int) std::lround (sampleRate * barSeconds);
    juce::AudioBuffer<float> audio (1, barSamples * names.size());
    audio.clear();

    for (int b = 0; b < names.size(); ++b)
    {
        const auto chord = *Chord::fromName (names[b]);

        for (int i = 0; i < barSamples; ++i)
        {
            const double t = i / sampleRate;
            double v = 0.0;

            for (int pc : chord.getPitchClasses())
            {
                const double hz = 440.0 * std::pow (2.0, (60 + pc - 69) / 12.0);   // around middle C

                for (int h = 1; h <= 4; ++h)
                    v += 0.06 / h * std::sin (juce::MathConstants<double>::twoPi * hz * h * t);
            }

            v += 0.15 * std::sin (juce::MathConstants<double>::twoPi * 440.0 * std::pow (2.0, (36 + chord.root - 69) / 12.0) * t);
            audio.setSample (0, b * barSamples + i, (float) (v * juce::jmin (1.0, t * 40.0, (barSeconds - t) * 40.0)));
        }
    }

    return audio;
}
} // namespace daw::gen::songchords
