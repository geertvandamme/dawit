#include "Exercises.h"

namespace daw::gen::exercises
{
using guides::Guide;

static std::vector<Step> fromFirstSong()
{
    std::vector<Step> steps;

    for (const auto& s : firstsong::getSteps())
    {
        Step step { s.title, s.subtitle, s.why, {} };

        if (s.hasLink)
            step.links.push_back (s.link);

        steps.push_back (std::move (step));
    }

    return steps;
}

static std::vector<Step> fromSongChords()
{
    std::vector<Step> steps;

    for (const auto& s : songchords::getSteps())
        steps.push_back ({ s.title, s.subtitle, s.why, s.links });

    return steps;
}

Info getInfo (Exercise exercise)
{
    Info info;
    info.exercise = exercise;

    switch (exercise)
    {
        case Exercise::firstSong:
            info.settingsStem = "learn.firstSong";
            info.title = firstsong::getTitle();
            info.intro = firstsong::getIntro();
            info.completion = firstsong::getCompletionText();
            info.carryOnLabel = "Carry on with Start here";
            info.carryOnTooltip = "Opens Theory Guides at Start here: the guides in an order that builds up";
            info.carryOnOpensStartHere = true;
            info.steps = fromFirstSong();
            break;

        case Exercise::songChords:
            info.settingsStem = "learn.songChords";
            info.title = songchords::getTitle();
            info.intro = songchords::getIntro();
            info.completion = songchords::getCompletionText();
            info.carryOnLabel = "Open the chord guide";
            info.carryOnTooltip = "Opens Chord progressions at Playing along";
            info.carryOnLink = { Guide::chords, "Playing along" };
            info.steps = fromSongChords();
            break;
    }

    return info;
}

juce::String getStepSettingKey (Exercise exercise) { return getInfo (exercise).settingsStem + ".step"; }
juce::String getDoneSettingKey (Exercise exercise) { return getInfo (exercise).settingsStem + ".done"; }

std::vector<Link> getAllLinks (Exercise exercise)
{
    if (exercise == Exercise::firstSong)
        return firstsong::getAllLinks();

    std::vector<Link> links;

    for (const auto& s : songchords::getSteps())
        links.insert (links.end(), s.links.begin(), s.links.end());

    links.push_back (getInfo (exercise).carryOnLink);
    return links;
}

int clampStep (Exercise exercise, int stored) noexcept
{
    return exercise == Exercise::firstSong ? firstsong::clampStep (stored) : songchords::clampStep (stored);
}

//==============================================================================
static juce::String midiTrackNameOf (const Engine& engine)
{
    const auto* track = firstsong::findMidiTrack (engine);
    return track != nullptr ? track->name : juce::String();
}

static bool hasDrumTrackIn (const Engine& engine)
{
    for (auto& t : engine.getTracks())
        if (firstsong::isDrumTrack (*t))
            return true;

    return false;
}

static juce::String clipNameOf (const Engine& engine)
{
    const auto* clip = songchords::findSongClip (engine);
    return clip != nullptr ? songchords::getClipName (*clip) : juce::String ("your recording");
}

bool isStepDone (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::firstSong)
        return firstsong::isStepDone (step, engine, context.baseline, context.saved);

    return songchords::isStepDone (step, engine, context.baseline, context.playedAlongBars);
}

juce::String getDoText (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::firstSong)
        return firstsong::getDoText (step, engine.getKey(), midiTrackNameOf (engine));

    return songchords::getDoText (step, clipNameOf (engine), songchords::getSwap (engine, context.baseline));
}

juce::String getWhy (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::songChords)
        return songchords::getWhy (step, songchords::getSwap (engine, context.baseline));

    const auto steps = fromFirstSong();
    return step >= 0 && step < (int) steps.size() ? steps[(size_t) step].why : juce::String();
}

Action getAction (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::firstSong)
    {
        const bool chordsEmpty = firstsong::canWriteChords (engine);
        const bool drums = hasDrumTrackIn (engine);
        return { firstsong::getActionLabel (step, midiTrackNameOf (engine), chordsEmpty, drums), firstsong::getActionTooltip (step, chordsEmpty, drums), Fix::none };
    }

    const auto status = songchords::getStatus (step, engine, context.baseline, context.playedAlongBars, context.hasOutputDevice);

    switch (status.fix)
    {
        case Fix::importAudio:
            return { songchords::getActionLabel (0), songchords::getActionTooltip (0, {}, {}), Fix::importAudio };

        case Fix::detectChords:
            return { songchords::getActionLabel (1), songchords::getActionTooltip (1, clipNameOf (engine), {}), Fix::detectChords };

        case Fix::audioSettings:
            return { "Audio/MIDI Settings...", "Opens the audio settings, where you choose where the sound comes out.", Fix::audioSettings };

        case Fix::none: break;
    }

    return { songchords::getActionLabel (step),
             songchords::getActionTooltip (step, clipNameOf (engine), songchords::getSwap (engine, context.baseline)), Fix::none };
}

Status getStatus (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::firstSong)
        return {};

    const auto s = songchords::getStatus (step, engine, context.baseline, context.playedAlongBars, context.hasOutputDevice);
    return { s.text, s.fix };
}

juce::String getDoneNote (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    return exercise == Exercise::songChords ? songchords::getDoneNote (step, engine, context.baseline) : juce::String();
}
} // namespace daw::gen::exercises
