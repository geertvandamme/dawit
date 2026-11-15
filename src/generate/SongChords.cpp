#include "SongChords.h"

#include "engine/Tempo.h"

#include <cmath>

namespace daw::gen::songchords
{
using guides::Guide;

static juce::String swapWhy (const Swap&);

std::vector<Step> getSteps()
{
    return {
        { "Bring in a song", "Import a recording you like",
          "A recording is audio: the sound itself, not the notes or the beat. DawIt can't read its tempo or chords the way it reads a MIDI clip, so in the next steps it listens for them.",
          { { Guide::midi, "What MIDI is" } } },
        { "Find its tempo", "Line the song's bars up with the recording",
          "The bars and beats on the ruler are only a grid until the song has the recording's tempo. Then every bar of the grid is a bar of the song, so the chords in step 3 fall one per bar "
          "and the sections in step 4 start on the right bars. DawIt marks the moments where the sound suddenly gets louder, mostly the drums, finds how often that pattern repeats and places "
          "the beats on them. It needs no download, but it needs a clear beat, such as drums, and at least about 6 seconds of it. Tempo detectors often find half or double the tempo "
          "you would tap along to, so tap along to check. A band playing without a click often drifts: when it does, Detect tempo also offers Follow the recording, which gives "
          "every bar its own tempo.",
          { { Guide::rhythm, "Tempo and time" } } },
        { "Find its chords", "Let DawIt listen for one chord per bar",
          "In each bar DawIt measures how strongly each of the 12 note names sounds, in any octave, leaving out short sounds such as drums. Then it picks the "
          "major, minor or seventh chord that fits best, and the lowest note decides between close ones. That's how you find chords by ear too: which notes "
          "ring, and which is lowest. It needs no download. Check the result by ear: a busy bass, passing notes or a sung melody can fool it, and it can't name "
          "sus, diminished or slash chords, so it gives the nearest chord it knows.",
          { { Guide::keyFinding, "Naming a chord from its notes" }, { Guide::rhythm, "Tempo and time" } } },
        { "Find its sections", "Mark its intro, verses and choruses",
          "Most pop and rock songs repeat their parts: once you can play the verse and the chorus, you know most of the song, and the sections show where each comes back. DawIt compares the "
          "four bars before and after every bar line: their chords, their sound (bright or dull), how loud they are and how much drums there are. A new part starts where these change, "
          "after 4, 8 or 16 bars where it can, because songs are mostly built from phrases of 4 bars. Parts with the same chords and sound are the same music heard again, and the loudest "
          "music that comes back is taken as the chorus. Check it by ear: a verse and a chorus with the same chords and sound can't be told apart.",
          { { Guide::structure, "How songs are built" } } },
        { "Play along", "Play the chords as the song plays",
          "Playing along is how most musicians learn songs: the recording keeps going and you fit in. Watch the next chord, not the one you're playing, and keep "
          "the rhythm going even when a change comes late. DawIt doesn't listen to what you play: the tick only says the song played with the chords in front of you.",
          { { Guide::chords, "Playing along" } } },
        { "Swap a chord", "Change one and hear the difference", swapWhy ({}),
          { { Guide::chords, "Your chord track" }, { Guide::chords, "How chords work" } } }
    };
}

juce::String getTitle() { return "Learn the chords of a song"; }

juce::String getHelpDescription() { return "Find the tempo, chords and sections of a recording you like, play along with its chords and swap one, in six steps"; }

juce::String getIntro()
{
    return "Six steps with a song you like: bring it in, let DawIt find its tempo, chords and sections, play along with the chords, then swap one and hear what changes. "
           "Do each step in the main window: the tick appears by itself.";
}

juce::String getCompletionText()
{
    return "Done. You found the tempo, chords and sections of a song, played along with its chords and heard what one swap does. Next, try another song, or practise one part "
           "at a time: click its section on the Sections row and choose Loop this section.";
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
TempoRecord makeTempoRecord (const Engine& engine, const AudioClip& clip)
{
    TempoRecord record;

    if (findSongClip (engine) == &clip)
    {
        record.clip = &clip;
        record.startBeat = clip.startBeat;
        record.tempoMap = engine.getTempoMap();
    }

    return record;
}

TempoRecord makeNoBeatRecord (const Engine& engine, const AudioClip& clip)
{
    auto record = makeTempoRecord (engine, clip);
    record.noClearBeat = ! record.isEmpty();
    return record;
}

int countSectionBars (const SectionList& sections, Span span)
{
    int n = 0;

    for (auto& s : sections)
        n += juce::jmax (0, juce::jmin (s.getEndBar(), span.endBar) - juce::jmax (s.startBar, span.firstBar));

    return n;
}

bool isStep1Done (const Engine& engine)
{
    auto* clip = findSongClip (engine);
    return clip != nullptr && getClipBars (engine, *clip) >= barsNeeded - 1.0e-6;
}

/** True when the record describes the song clip as it is now. */
static bool recordMatches (const Engine& engine, const TempoRecord& record, const AudioClip* clip)
{
    return clip != nullptr && record.clip == clip && juce::exactlyEqual (record.startBeat, clip->startBeat) && record.tempoMap == engine.getTempoMap();
}

bool isStepTempoDone (const Engine& engine, const TempoRecord& record)
{
    return ! record.noClearBeat && recordMatches (engine, record, findSongClip (engine));
}

bool isStepChordsDone (const Engine& engine)
{
    auto* clip = findSongClip (engine);
    return clip != nullptr && countChords (engine.getChords(), getSpan (engine, *clip)) >= barsNeeded;
}

bool isStepSectionsDone (const Engine& engine)
{
    auto* clip = findSongClip (engine);
    return clip != nullptr && countSectionBars (engine.getSections(), getSpan (engine, *clip)) >= barsNeeded;
}

bool isStepPlayAlongDone (int playedAlongBars) noexcept { return playedAlongBars >= barsNeeded; }

bool isStepSwapDone (const Engine& engine, const ChordMap& baseline)
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

bool isStepDone (int step, const Engine& engine, const Progress& progress)
{
    switch (step)
    {
        case stepImport:    return isStep1Done (engine);
        case stepTempo:     return isStepTempoDone (engine, progress.tempo);
        case stepChords:    return isStepChordsDone (engine);
        case stepSections:  return isStepSectionsDone (engine);
        case stepPlayAlong: return isStepPlayAlongDone (progress.playedAlongBars);
        case stepSwap:      return isStepSwapDone (engine, progress.baseline);
        default: break;
    }

    return false;
}

int clampStep (int stored) noexcept
{
    return stored >= 0 && stored < numSteps ? stored : 0;
}

int migrateStep (int stored, int version) noexcept
{
    if (version >= stepsVersion)
        return clampStep (stored);

    constexpr int oldToNew[] { stepImport, stepChords, stepPlayAlong, stepSwap };
    return stored >= 0 && stored < 4 ? oldToNew[stored] : 0;
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
        case stepImport:
            return "Press Import audio... (Cmd+I) and choose a song file on your computer, such as an MP3 or WAV file, or drag the file onto the tracks. DawIt puts it on "
                   "an audio track at the playhead. A song with a clear guitar or piano part and three or four chords is the easiest to start with.";

        case stepTempo:
            return "Press Detect tempo. DawIt listens to " + clipName + " and shows its tempo in BPM (beats per minute) and whether it stays steady. Press Set tempo: the song takes "
                   "that tempo, and " + clipName + " moves so its first downbeat, the first beat of a bar, starts a bar. If the tempo feels twice as fast or as slow as you would "
                   "tap your foot, choose half or double before you press Set tempo.";

        case stepChords:
            return "Press Detect chords. DawIt listens to " + clipName + " and shows the chords it hears, one per bar of the song, and the key they sound like. "
                   "If it says the bars don't line up, choose Detect tempo first and set the tempo as in step 2, then press Detect chords again. "
                   "When the chords look right, press Put on chord track: they appear on the Chords row above the tracks.";

        case stepSections:
            return "Press Detect song structure. DawIt listens to " + clipName + " and shows the parts it hears, such as intro, verse and chorus, with why each got its name and which "
                   "parts are the same music (A, B, C). When they look right, press Put on Sections row: they appear on the Sections row above the chords. Click a section there "
                   "and choose Loop this section to practise it, or Type to change it if DawIt guessed wrong.";

        case stepPlayAlong:
            return "Press Open Play along (Cmd+4). It shows the chord playing now, large, as piano keys and a guitar chord box, with the next one beside it. "
                   "Press Space to play the song and play the chords on a piano, a keyboard or a guitar. If whole chords are too much at first, play only the "
                   "root, the note the chord is named after, then add the rest. The tick comes once you have played along through 4 bars with chords.";

        case stepSwap:
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
    if (step == stepSwap)
        return swapWhy (swap);

    const auto steps = getSteps();
    return step >= 0 && step < numSteps ? steps[(size_t) step].why : juce::String();
}

juce::String getActionLabel (int step)
{
    switch (step)
    {
        case stepImport:    return "Import audio...";
        case stepTempo:     return "Detect tempo";
        case stepChords:    return "Detect chords";
        case stepSections:  return "Detect song structure";
        case stepPlayAlong: return "Open Play along";
        case stepSwap:      return "Open the Chords row";
        default: break;
    }

    return {};
}

juce::String getActionTooltip (int step, const juce::String& clipName, const Swap& swap)
{
    switch (step)
    {
        case stepImport:    return "Opens Import Audio (Cmd+I): choose a song file to put on an audio track. Nothing else in the song changes.";
        case stepTempo:     return "Listens to " + clipName + " and shows its tempo. Nothing changes until you press Set tempo.";
        case stepChords:    return "Listens to " + clipName + " and shows the chords it finds. Nothing changes until you press Put on chord track.";
        case stepSections:  return "Listens to " + clipName + " and shows the parts it finds. Nothing changes until you press Put on Sections row.";
        case stepPlayAlong: return "Opens Play along with the chords (Cmd+4). Press Space to play; the window follows the song.";
        case stepSwap:      return "Unfolds the Chords row and points at bar " + (swap.valid ? juce::String (swap.bar + 1) : juce::String ("1")) + ". Click a bar to choose its chord.";
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

Status getStatus (int step, const Engine& engine, const Progress& progress)
{
    const auto* clip = findSongClip (engine);
    const bool longEnough = clip != nullptr && getClipBars (engine, *clip) >= barsNeeded - 1.0e-6;
    const auto noClip = [] (const char* what) { return juce::String (what) + " needs a recording of at least 4 bars: step 1 brings one in."; };
    const juce::String noChords = "Play along shows the chords on the Chords row, and there are none on your recording yet. Find them first (step 3).";
    const bool tempoDone = isStepTempoDone (engine, progress.tempo);

    switch (step)
    {
        case stepImport:
            if (clip != nullptr && ! longEnough)
                return { "Your longest recording, " + getClipName (*clip) + ", lasts " + formatBars (getClipBars (engine, *clip))
                         + " bars at the song's tempo. Chords are found one per bar, so this exercise needs at least 4: import a longer part of the song.", Fix::none };
            break;

        case stepTempo:
            if (! longEnough)
                return { noClip ("Detect tempo"), Fix::importAudio };

            if (progress.tempo.clip == clip)
            {
                if (progress.tempo.noClearBeat)
                    return { "No clear beat was found in " + getClipName (*clip) + ": it may have no drums, or play freely. If you know the song's tempo, double-click the tempo "
                             "next to the transport and type it, then press Skip: Detect chords checks whether the bars line up.", Fix::none };

                if (! tempoDone)
                    return { "The song's tempo or the place of " + getClipName (*clip) + " has changed since its tempo was found. Detect it again so the bars line up.", Fix::none };
            }

            break;

        case stepChords:
        case stepSections:
        {
            const bool chords = step == stepChords;

            if (! longEnough)
                return { noClip (chords ? "Detect chords" : "Detect song structure"), Fix::importAudio };

            juce::String text;

            if (! tempoDone)
                text = chords ? "Tip: find the tempo first (step 2), so each chord falls on its own bar. Detect chords checks this too."
                              : "Tip: find the tempo first (step 2), so the sections start on the right bars. Detect song structure checks this too.";

            const auto span = getSpan (engine, *clip);

            if (! chords && span.size() < 16 && countSectionBars (engine.getSections(), span) == 0)
                text = (text.isEmpty() ? juce::String() : text + " ") + getClipName (*clip) + " is " + formatBars (getClipBars (engine, *clip))
                       + " bars long. DawIt finds where parts start by comparing the 4 bars on each side of a bar line, so a short recording may give only one or two sections: a whole song "
                         "shows its structure best.";

            return { text, Fix::none };
        }

        case stepPlayAlong:
        case stepSwap:
            if (! longEnough)
                return { noClip ("Detect chords"), Fix::importAudio };

            if (countChords (engine.getChords(), getSpan (engine, *clip)) == 0)
                return { noChords, Fix::detectChords };

            if (step == stepPlayAlong && ! progress.hasOutputDevice)
                return { "No audio output is chosen, so you won't hear the song. Choose one in Audio/MIDI Settings.", Fix::audioSettings };

            if (step == stepPlayAlong && progress.playedAlongBars > 0)
                return { "Played along: " + juce::String (juce::jmin (progress.playedAlongBars, barsNeeded)) + " of " + juce::String (barsNeeded) + " bars.", Fix::none };

            break;

        default: break;
    }

    return {};
}

juce::String getDoneNote (int step, const Engine& engine, const Progress& progress)
{
    const auto* clip = findSongClip (engine);

    if (clip == nullptr)
        return {};

    switch (step)
    {
        case stepImport: return getClipName (*clip) + " is " + formatBars (getClipBars (engine, *clip)) + " bars long at the song's tempo.";

        case stepTempo:
        {
            const auto range = engine.getTempoMap().getRange (clip->startBeat, clip->getEndBeat (engine.getTempoMap()));

            if (range.getLength() < 0.005)
                return "The song is at " + daw::formatBpm (engine.getTempoMap().getBpmAt (clip->startBeat)) + " BPM, the tempo of " + getClipName (*clip) + ".";

            return "The song's tempo follows " + getClipName (*clip) + " bar by bar, from " + daw::formatBpm (range.getStart()) + " to " + daw::formatBpm (range.getEnd()) + " BPM.";
        }

        case stepChords: return juce::String (countChords (engine.getChords(), getSpan (engine, *clip))) + " bars of " + getClipName (*clip) + " have a chord.";

        case stepSections:
        {
            const auto span = getSpan (engine, *clip);
            juce::StringArray names;
            int total = 0;

            for (auto& section : engine.getSections())
                if (section.startBar < span.endBar && section.getEndBar() > span.firstBar)
                {
                    if (++total <= 4)
                        names.add (section.getDisplayName());
                }

            return getClipName (*clip) + " has " + juce::String (total) + (total == 1 ? " section: " : " sections: ") + names.joinIntoString (", ")
                   + (total > 4 ? " and " + juce::String (total - 4) + " more." : ".");
        }

        case stepSwap:
        {
            const auto span = getSpan (engine, *clip);

            for (auto& [bar, was] : progress.baseline)
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
/** The chords as sound, a bar each, scaled per bar by `gains`. */
static juce::AudioBuffer<float> renderChordBars (const juce::StringArray& names, const std::vector<double>& gains, double sampleRate, double bpm)
{
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
            audio.setSample (0, b * barSamples + i, (float) (gains[(size_t) b] * v * juce::jmin (1.0, t * 40.0, (barSeconds - t) * 40.0)));
        }
    }

    return audio;
}

juce::AudioBuffer<float> makeExampleRecording (double sampleRate, double bpm)
{
    const juce::StringArray names { "C", "G", "Am", "F", "C", "G", "Am", "F" };
    return renderChordBars (names, std::vector<double> (8, 1.0), sampleRate, bpm);
}

juce::AudioBuffer<float> makeExampleSongWithBeat (double sampleRate, double bpm)
{
    const juce::StringArray names { "C", "G", "Am", "F", "F", "G", "C", "G", "C", "G", "Am", "F", "F", "G", "C", "G" };
    std::vector<double> gains;

    for (int b = 0; b < names.size(); ++b)
        gains.push_back ((b / 4) % 2 == 0 ? std::pow (10.0, -10.0 / 20.0) : 1.0);   // verse 10 dB softer than chorus

    auto audio = renderChordBars (names, gains, sampleRate, bpm);
    const double beatSeconds = 60.0 / juce::jmax (1.0, bpm);
    const int numBeats = names.size() * 4;
    juce::Random random (1234);   // fixed, so the example never changes

    for (int beat = 0; beat < numBeats; ++beat)
    {
        const int start = (int) std::lround (beat * beatSeconds * sampleRate);

        // A tick on every beat: 30 ms of high noise (each sample minus the one before), fading out fast.
        const int tickSamples = (int) std::lround (0.03 * sampleRate);
        float last = 0.0f;

        for (int i = 0; i < tickSamples && start + i < audio.getNumSamples(); ++i)
        {
            const float noise = random.nextFloat() * 2.0f - 1.0f;
            const float envelope = std::exp (-(float) i / (0.006f * (float) sampleRate));
            audio.getWritePointer (0)[start + i] += 0.2f * envelope * (noise - last) * 0.5f;
            last = noise;
        }

        // A kick on beats 1 and 3: a low sine falling from 70 to 45 Hz over 100 ms.
        if (beat % 4 == 0 || beat % 4 == 2)
        {
            const int kickSamples = (int) std::lround (0.1 * sampleRate);
            double phase = 0.0;

            for (int i = 0; i < kickSamples && start + i < audio.getNumSamples(); ++i)
            {
                const double t = i / sampleRate;
                phase += juce::MathConstants<double>::twoPi * (45.0 + 25.0 * std::exp (-t / 0.03)) / sampleRate;
                audio.getWritePointer (0)[start + i] += (float) (0.35 * std::exp (-t / 0.04) * std::sin (phase));
            }
        }
    }

    return audio;
}
} // namespace daw::gen::songchords
