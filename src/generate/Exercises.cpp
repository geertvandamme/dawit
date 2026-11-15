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

static std::vector<Step> fromWriteLyrics()
{
    std::vector<Step> steps;

    for (const auto& s : writelyrics::getSteps())
        steps.push_back ({ s.title, s.subtitle, s.why, s.links });

    return steps;
}

static std::vector<Step> fromSoundBetter()
{
    std::vector<Step> steps;

    for (const auto& s : soundbetter::getSteps())
        steps.push_back ({ s.title, s.subtitle, s.why, s.links });

    return steps;
}

static std::vector<Step> fromMixExport()
{
    std::vector<Step> steps;

    for (const auto& s : mixexport::getSteps())
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
            info.helpDescription = "Four short steps in your own song: hear a note, change a chord, add a rhythm and save";
            info.intro = firstsong::getIntro();
            info.completion = firstsong::getCompletionText();
            info.carryOnLabel = "Carry on with Start here";
            info.carryOnTooltip = "Opens Theory Guides at Start here: the guides in an order that builds up";
            info.carryOnOpensStartHere = true;
            info.steps = fromFirstSong();
            info.windowHeight = 500;
            break;

        case Exercise::songChords:
            info.settingsStem = "learn.songChords";
            info.title = songchords::getTitle();
            info.helpDescription = songchords::getHelpDescription();
            info.intro = songchords::getIntro();
            info.completion = songchords::getCompletionText();
            info.carryOnLabel = "Open the chord guide";
            info.carryOnTooltip = "Opens Chord progressions at Playing along";
            info.carryOnLink = { Guide::chords, "Playing along" };
            info.steps = fromSongChords();
            info.windowHeight = 740;   // longer texts and a status line: taller, so the buttons stay in view
            break;

        case Exercise::writeLyrics:
            info.settingsStem = writelyrics::settingsStem;
            info.title = writelyrics::getTitle();
            info.helpDescription = writelyrics::getHelpDescription();
            info.intro = writelyrics::getIntro();
            info.completion = writelyrics::getCompletionText();
            info.carryOnLabel = "Open the lyrics guide";
            info.carryOnTooltip = "Opens How lyrics are written at Your lyrics: rhymes, syllables and gaps in your words";
            info.carryOnLink = { Guide::lyrics, "Your lyrics" };
            info.steps = fromWriteLyrics();
            info.windowHeight = writelyrics::windowHeight;
            break;

        case Exercise::soundBetter:
            info.settingsStem = soundbetter::settingsStem;
            info.title = soundbetter::getTitle();
            info.helpDescription = soundbetter::getHelpDescription();
            info.intro = soundbetter::getIntro();
            info.completion = soundbetter::getCompletionText();
            info.carryOnLabel = "Next: the track's levels";
            info.carryOnTooltip = "Opens Gain staging at Step by step: how loud a track should be going into each effect and out of its fader.";
            info.carryOnLink = { Guide::gainStaging, "Step by step" };
            info.steps = fromSoundBetter();
            info.windowHeight = soundbetter::windowHeight;
            break;

        case Exercise::mixExport:
            info.settingsStem = mixexport::settingsStem;
            info.title = mixexport::getTitle();
            info.helpDescription = mixexport::getHelpDescription();
            info.intro = mixexport::getIntro();
            info.completion = mixexport::getCompletionText();
            info.carryOnLabel = "Next: mastering or an engineer";
            info.carryOnTooltip = "Opens Sending stems to an engineer at Mixing or mastering?: what mastering does, and which files an engineer wants.";
            info.carryOnLink = { Guide::handover, "Mixing or mastering?" };
            info.steps = fromMixExport();
            info.windowHeight = mixexport::windowHeight;
            break;
    }

    return info;
}

juce::String getDoneSettingKey (Exercise exercise) { return getInfo (exercise).settingsStem + ".done"; }

juce::StringArray getStaleSettingKeys (Exercise exercise)
{
    const auto stem = getInfo (exercise).settingsStem;
    return { stem + ".step", stem + ".stepsVersion" };
}

std::vector<Link> getAllLinks (Exercise exercise)
{
    if (exercise == Exercise::firstSong)
        return firstsong::getAllLinks();

    std::vector<Link> links;

    for (const auto& s : getInfo (exercise).steps)
        links.insert (links.end(), s.links.begin(), s.links.end());

    links.push_back (getInfo (exercise).carryOnLink);
    return links;
}

int clampStep (Exercise exercise, int stored) noexcept
{
    switch (exercise)
    {
        case Exercise::firstSong:   return firstsong::clampStep (stored);
        case Exercise::songChords:  return songchords::clampStep (stored);
        case Exercise::writeLyrics: return writelyrics::clampStep (stored);
        case Exercise::soundBetter: return soundbetter::clampStep (stored);
        case Exercise::mixExport:   return mixexport::clampStep (stored);
    }

    return 0;
}

//==============================================================================
/** What the chord exercise needs from the context. */
static songchords::Progress progressOf (const Context& context)
{
    return { context.baseline, context.playedAlongBars, context.hasOutputDevice, context.tempo };
}

/** What the lyrics exercise needs from the context. */
static writelyrics::Progress lyricsProgressOf (const Context& context)
{
    return { context.writeLyricsSectionBar, context.sung, context.hasOutputDevice, context.singAlongOpen };
}

/** What the mixing exercise needs from the context. */
static soundbetter::Progress soundBetterProgressOf (const Context& context)
{
    return { context.soundBetterSelected, context.soundBetterRemembered, context.playing, context.heardBeat, context.hasOutputDevice,
             context.listened, context.reduction, context.compared };
}

/** What the mix and export exercise needs from the context. */
static mixexport::Progress mixExportProgressOf (const Context& context)
{
    return { context.mixBaseline, context.mixSelected, context.mixMeasured, context.mixExported, context.hasOutputDevice };
}

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

    if (exercise == Exercise::writeLyrics)
        return writelyrics::isStepDone (step, engine, lyricsProgressOf (context));

    if (exercise == Exercise::soundBetter)
        return soundbetter::isStepDone (step, engine, soundBetterProgressOf (context));

    if (exercise == Exercise::mixExport)
        return mixexport::isStepDone (step, engine, mixExportProgressOf (context));

    return songchords::isStepDone (step, engine, progressOf (context));
}

juce::String getDoText (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::firstSong)
        return firstsong::getDoText (step, engine.getKey(), midiTrackNameOf (engine));

    if (exercise == Exercise::writeLyrics)
        return writelyrics::getDoText (step, engine, lyricsProgressOf (context));

    if (exercise == Exercise::soundBetter)
        return soundbetter::getDoText (step, engine, soundBetterProgressOf (context));

    if (exercise == Exercise::mixExport)
        return mixexport::getDoText (step, engine, mixExportProgressOf (context));

    return songchords::getDoText (step, clipNameOf (engine), songchords::getSwap (engine, context.baseline));
}

juce::String getWhy (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::songChords)
        return songchords::getWhy (step, songchords::getSwap (engine, context.baseline));

    const auto steps = exercise == Exercise::writeLyrics ? fromWriteLyrics() : exercise == Exercise::soundBetter ? fromSoundBetter()
                                                                                                             : exercise == Exercise::mixExport ? fromMixExport() : fromFirstSong();
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

    if (exercise == Exercise::writeLyrics)
    {
        const auto progress = lyricsProgressOf (context);
        const auto fix = writelyrics::getStatus (step, engine, progress).fix;

        if (fix != Fix::none)
            return { writelyrics::getFixLabel (fix), writelyrics::getFixTooltip (fix, engine, progress), fix };

        auto section = writelyrics::findChosenSection (engine, progress);

        if (! section.has_value())
            section = writelyrics::findFirstSungSection (engine.getSections());

        return { writelyrics::getActionLabel (step),
                 writelyrics::getActionTooltip (step, step == writelyrics::stepSection && section.has_value() ? section->getDisplayName() : juce::String()), Fix::none };
    }

    if (exercise == Exercise::soundBetter)
    {
        const auto a = soundbetter::getAction (step, engine, soundBetterProgressOf (context));
        return { a.label, a.tooltip, a.fix };
    }

    if (exercise == Exercise::mixExport)
    {
        const auto fix = mixexport::getStatus (step, engine, mixExportProgressOf (context)).fix;

        if (fix != Fix::none)
            return { mixexport::getFixLabel (fix), mixexport::getFixTooltip (fix), fix };

        return { mixexport::getActionLabel (step), mixexport::getActionTooltip (step), Fix::none };
    }

    const auto status = songchords::getStatus (step, engine, progressOf (context));

    switch (status.fix)
    {
        case Fix::importAudio:
            return { songchords::getActionLabel (songchords::stepImport), songchords::getActionTooltip (songchords::stepImport, {}, {}), Fix::importAudio };

        case Fix::detectChords:
            return { songchords::getActionLabel (songchords::stepChords), songchords::getActionTooltip (songchords::stepChords, clipNameOf (engine), {}), Fix::detectChords };

        case Fix::detectTempo:
            return { songchords::getActionLabel (songchords::stepTempo), songchords::getActionTooltip (songchords::stepTempo, clipNameOf (engine), {}), Fix::detectTempo };

        case Fix::detectSections:
            return { songchords::getActionLabel (songchords::stepSections), songchords::getActionTooltip (songchords::stepSections, clipNameOf (engine), {}), Fix::detectSections };

        case Fix::audioSettings:
            return { "Audio/MIDI Settings...", "Opens the audio settings, where you choose where the sound comes out.", Fix::audioSettings };

        case Fix::openSections:
        case Fix::openLyrics:
        case Fix::none: break;
    }

    // The tempo and sections steps name their own action; the others are the step's own (Fix::none).
    const auto own = step == songchords::stepTempo ? Fix::detectTempo : step == songchords::stepSections ? Fix::detectSections : Fix::none;
    return { songchords::getActionLabel (step),
             songchords::getActionTooltip (step, clipNameOf (engine), songchords::getSwap (engine, context.baseline)), own };
}

Status getStatus (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::firstSong)
        return {};

    if (exercise == Exercise::writeLyrics)
    {
        const auto w = writelyrics::getStatus (step, engine, lyricsProgressOf (context));
        return { w.text, w.fix };
    }

    if (exercise == Exercise::soundBetter)
    {
        const auto b = soundbetter::getStatus (step, engine, soundBetterProgressOf (context));
        return { b.text, b.fix };
    }

    if (exercise == Exercise::mixExport)
    {
        const auto m = mixexport::getStatus (step, engine, mixExportProgressOf (context));
        return { m.text, m.fix };
    }

    const auto s = songchords::getStatus (step, engine, progressOf (context));
    return { s.text, s.fix };
}

juce::String getDoneNote (Exercise exercise, int step, const Engine& engine, const Context& context)
{
    if (exercise == Exercise::writeLyrics)
        return writelyrics::getDoneNote (step, engine, lyricsProgressOf (context));

    if (exercise == Exercise::soundBetter)
        return soundbetter::getDoneNote (step, engine, soundBetterProgressOf (context));

    if (exercise == Exercise::mixExport)
        return mixexport::getDoneNote (step, engine, mixExportProgressOf (context));

    return exercise == Exercise::songChords ? songchords::getDoneNote (step, engine, progressOf (context)) : juce::String();
}
} // namespace daw::gen::exercises
