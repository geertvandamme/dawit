#include "TheoryGuides.h"

namespace daw::gen::guides
{
const std::vector<Link>& getLinks()
{
    // From the plan: a link joins two pages and shows on both. An empty topic is the guide's first page.
    static const std::vector<Link> links
    {
        { { Guide::sound, "Frequency is pitch" }, { Guide::eq, "The frequency ranges" } },
        { { Guide::sound, "Amplitude is loudness" }, { Guide::loudness, "Peaks aren't loudness" } },
        { { Guide::sound, "The shape is the timbre" }, { Guide::additive, "The harmonic series" } },
        { { Guide::sound, "The shape is the timbre" }, { Guide::subtractive, "Envelopes: attack, decay, sustain, release" } },
        { { Guide::sound, "Phase cancellation" }, { Guide::stereo, "Mono: phones, clubs and the sum" } },
        { { Guide::sound, "Checking in mono" }, { Guide::stereo, "Mid and side" } },
        { { Guide::sound, "From air to numbers" }, { Guide::recording, "From the air to a file" } },
        { { Guide::sound, "From air to numbers" }, { Guide::saturation, "Aliasing" } },
        { { Guide::sound, "Sound is moving air" }, { Guide::instruments, "How instruments make sound" } },
        { { Guide::spectralFreeze, "A sound as a spectrum" }, { Guide::sound, "The shape is the timbre" } },
        { { Guide::eq, "Where instruments sit" }, { Guide::drums, "The kick: a sine that falls" } },
        { { Guide::eq, "Where instruments sit" }, { Guide::stereo, "Why the low end stays in the middle" } },
        { { Guide::eq, "Louder sounds better: match level" }, { Guide::gainStaging, "Louder sounds better" } },
        { { Guide::compressor, "Threshold and ratio" }, { Guide::gainStaging, "Levels into a compressor" } },
        { { Guide::compressor, "How much, for what" }, { Guide::drums, "Building a kit, and recipes" } },
        { { Guide::compressor, "How much, for what" }, { Guide::routing, "Glue: one compressor for the group" } },
        { { Guide::compressor, "Turning down the loud parts" }, { Guide::gate, "Closing the gaps" } },
        { { Guide::compressor, "Attack: how fast it grabs" }, { Guide::triggers, "Transients" } },
        { { Guide::gate, "Gated reverb" }, { Guide::reverb, "Kinds of reverb" } },
        { { Guide::slicer, "Mute: a trance gate" }, { Guide::gate, "Closing the gaps" } },
        { { Guide::physical, "A wave running round a loop" }, { Guide::delay, "Short delays" } },
        { { Guide::physical, "Karplus-Strong: noise into a note" }, { Guide::instruments, "Strings and their harmonics" } },
        { { Guide::physical, "Tubes and reeds" }, { Guide::instruments, "Tubes: open and closed" } },
        { { Guide::physical, "Recipes for common sounds" }, { Guide::subtractive, "Recipes for common sounds" } },
        { { Guide::subtractive, "Recipes for common sounds" }, { Guide::additive, "Recipes for common sounds" } },
        { { Guide::additive, "Recipes for common sounds" }, { Guide::fm, "Recipes for common sounds" } },
        { { Guide::fm, "Recipes for common sounds" }, { Guide::subtractive, "Recipes for common sounds" } },
        { { Guide::subtractive, "The LFO" }, { Guide::stepFilter, "Step filter, slicer and LFO" } },
        { { Guide::stepFilter, "Cutoff and the three filters" }, { Guide::subtractive, "The filter" } },
        { { Guide::additive, "Drawbars: the Hammond organ" }, { Guide::instruments, "Keys: piano and organ" } },
        { { Guide::saturation, "Harmonics" }, { Guide::additive, "The harmonic series" } },
        { { Guide::distortion, "More than saturation" }, { Guide::saturation, "Pushed too far" } },
        { { Guide::drums, "Drums from oscillators and noise" }, { Guide::instruments, "Drums, bars and bells" } },
        { { Guide::drums, "Velocity, accents and groove" }, { Guide::rhythm, "Feel and the grid" } },
        { { Guide::drums, "Tuning drums to the song" }, { Guide::keyFinding, "What a key is" } },
        { { Guide::instruments, "Synths and electronic instruments" }, { Guide::subtractive, "" } },
        { { Guide::instruments, "What makes MIDI sound real" }, { Guide::midi, "Velocity" } },
        { { Guide::instruments, "What makes MIDI sound real" }, { Guide::sampler, "" } },
        { { Guide::instruments, "Drums and percussion" }, { Guide::triggers, "What a drum trigger is" } },
        { { Guide::midi, "The sustain pedal" }, { Guide::instruments, "Keys: piano and organ" } },
        { { Guide::midi, "Note numbers and pitch" }, { Guide::readMusic, "The grand staff and the piano" } },
        { { Guide::midi, "Note numbers and pitch" }, { Guide::tuning, "Reference pitch: A4 = 440 Hz" } },
        { { Guide::midi, "Backwards: retrograde" }, { Guide::arpeggiator, "Note order" } },
        { { Guide::rhythm, "Quantising" }, { Guide::triggers, "On the grid or as played" } },
        { { Guide::rhythm, "Time signatures" }, { Guide::readMusic, "Time signature, bars and tempo" } },
        { { Guide::rhythm, "Note values" }, { Guide::readMusic, "Note values on the staff" } },
        { { Guide::rhythm, "Note values" }, { Guide::slicer, "Note lengths and steps" } },
        { { Guide::rhythm, "Tempo and time" }, { Guide::delay, "Time and tempo" } },
        { { Guide::chords, "How chords work" }, { Guide::keyFinding, "Naming a chord from its notes" } },
        { { Guide::chords, "How chords work" }, { Guide::lyrics, "Words and music agree" } },
        { { Guide::chords, "How chords work" }, { Guide::arpeggiator, "Chord tones" } },
        { { Guide::chords, "The circle of fifths" }, { Guide::readMusic, "Key signatures" } },
        { { Guide::chords, "Playing along" }, { Guide::chordShapes, "" } },
        { { Guide::chords, "Well-known progressions" }, { Guide::structure, "The sections" } },
        { { Guide::chords, "Well-known progressions" }, { Guide::rhythm, "Bars and beats" } },
        { { Guide::keyFinding, "Inversions and slash chords" }, { Guide::chordShapes, "" } },
        { { Guide::structure, "How songs are built" }, { Guide::lyrics, "Song forms" } },
        { { Guide::structure, "Typical structures" }, { Guide::styles, "Styles of music" } },
        { { Guide::styles, "Drum bus and reverb return" }, { Guide::routing, "Typical buses in a song" } },
        { { Guide::lyrics, "Stress and the beat" }, { Guide::rhythm, "Counting" } },
        { { Guide::lyrics, "Singing along" }, { Guide::singing, "Vowels and words" } },
        { { Guide::singing, "Singing in tune" }, { Guide::tune, "" } },
        { { Guide::singing, "Singing in tune" }, { Guide::tuning, "Cents, hertz and beats" } },
        { { Guide::singing, "Harmonies and doubles" }, { Guide::harmoniser, "" } },
        { { Guide::singing, "The microphone" }, { Guide::recording, "Choosing an input" } },
        { { Guide::recording, "Setting the level" }, { Guide::gainStaging, "Quiet, healthy, hot, over" } },
        { { Guide::gainStaging, "Over 0 dB" }, { Guide::loudness, "True peak" } },
        { { Guide::gainStaging, "Through a bus" }, { Guide::routing, "Levels add up" } },
        { { Guide::gainStaging, "Step by step" }, { Guide::loudness, "Loudness while mixing" } },
        { { Guide::loudness, "Targets and normalisation" }, { Guide::handover, "Headroom and the master" } },
        { { Guide::routing, "Stems and multitracks" }, { Guide::handover, "Stems, multitracks and the mix" } },
        { { Guide::routing, "Insert or send?" }, { Guide::reverb, "Reverb in a mix" } },
        { { Guide::routing, "Sends and returns" }, { Guide::delay, "Delay in a mix" } },
    };

    return links;
}

std::vector<PageRef> getLinksFor (Guide guide, const juce::String& topic, bool isFirstPage)
{
    std::vector<PageRef> found;
    const auto isThis = [&] (const PageRef& end)
    {
        return end.guide == guide && (end.topic == topic || (end.topic.isEmpty() && isFirstPage));
    };

    for (auto& link : getLinks())
    {
        if (isThis (link.a))
            found.push_back (link.b);
        else if (isThis (link.b))
            found.push_back (link.a);
    }

    return found;
}

juce::StringArray validateLinks (const std::vector<Entry>& entries)
{
    juce::StringArray problems;
    juce::StringArray seen;
    std::vector<Guide> used;

    const auto name = [] (const PageRef& r) { return getKey (r.guide) + " / " + (r.topic.isEmpty() ? juce::String ("(first page)") : r.topic); };
    const auto exists = [&] (const PageRef& r)
    {
        const auto found = std::find_if (entries.begin(), entries.end(), [&] (auto& e) { return e.guide == r.guide; });
        return found == entries.end() || found->topics.isEmpty() || r.topic.isEmpty() || found->topics.contains (r.topic);
    };

    for (auto& link : getLinks())
    {
        if (link.a.guide == link.b.guide)
            problems.add ("A link joins a guide to itself: " + name (link.a));

        const auto one = name (link.a) + " <> " + name (link.b);
        const auto other = name (link.b) + " <> " + name (link.a);

        if (seen.contains (one) || seen.contains (other))
            problems.add ("The link is written twice: " + one);

        seen.add (one);

        for (auto* end : { &link.a, &link.b })
        {
            used.push_back (end->guide);

            if (! exists (*end))
                problems.add ("No such page: " + name (*end));
        }
    }

    for (auto g : allGuides)
        if (std::find (used.begin(), used.end(), g) == used.end())
            problems.add ("No link for the guide " + getKey (g));

    return problems;
}

//==============================================================================
juce::String getPageId (Guide guide, const juce::String& topic)
{
    return getKey (guide) + "/" + topic;
}

bool isCountedPage (const juce::String& topic)
{
    return topic != sourcesTopicName && ! topic.startsWith ("Your");
}

Progress getProgress (const Entry& entry, const juce::StringArray& readIds)
{
    Progress progress;

    for (auto& topic : entry.topics)
    {
        if (! isCountedPage (topic))
            continue;

        ++progress.total;

        if (readIds.contains (getPageId (entry.guide, topic)))
            ++progress.read;
    }

    return progress;
}

Progress getPathProgress (const std::vector<Entry>& entries, const juce::StringArray& readIds)
{
    Progress progress;

    for (auto g : getStartPath())
        for (auto& e : entries)
            if (e.guide == g)
            {
                const auto p = getProgress (e, readIds);
                progress.read += p.read;
                progress.total += p.total;
            }

    return progress;
}

std::optional<PagePosition> getFirstUnreadOnPath (const std::vector<Entry>& entries, const juce::StringArray& readIds)
{
    for (auto g : getStartPath())
        for (auto& e : entries)
            if (e.guide == g)
                for (int t = 0; t < e.topics.size(); ++t)
                    if (isCountedPage (e.topics[t]) && ! readIds.contains (getPageId (g, e.topics[t])))
                        return PagePosition { g, t };

    return std::nullopt;
}

std::optional<PagePosition> getNextPage (const std::vector<Entry>& entries, Guide guide, int topic)
{
    const auto found = std::find_if (entries.begin(), entries.end(), [guide] (auto& e) { return e.guide == guide; });

    if (found == entries.end() || found->topics.isEmpty())
        return std::nullopt;

    const auto& topics = found->topics;
    const auto path = getStartPath();
    const auto onPath = std::find (path.begin(), path.end(), guide);
    const bool nextIsSources = topic + 1 < topics.size() && topics[topic + 1] == sourcesTopicName;
    const bool atSources = topic >= 0 && topic < topics.size() && topics[topic] == sourcesTopicName;

    if (onPath != path.end() && (nextIsSources || atSources || topic + 1 >= topics.size()))
    {
        // On Start here, the end of a guide leads to the first page of the next one, skipping Sources.
        for (auto it = onPath + 1; it != path.end(); ++it)
            if (std::any_of (entries.begin(), entries.end(), [it] (auto& e) { return e.guide == *it && ! e.topics.isEmpty(); }))
                return PagePosition { *it, 0 };

        return std::nullopt;
    }

    if (topic + 1 < topics.size())
        return PagePosition { guide, topic + 1 };

    return std::nullopt;
}
} // namespace daw::gen::guides
