#include "StructureActions.h"
#include "TempoActions.h"
#include "generate/StructureGuide.h"

#include <map>

namespace daw
{
namespace sf = analysis::structurefinder;
namespace tf = analysis::tempofinder;

class StructureDetector::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (StructureDetector& o, Track& t, AudioClip& c, const TempoMap& songTempoMap)
        : juce::ThreadWithProgressWindow ("Detect song structure", true, true, 10000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), songTempo (songTempoMap), startBeat (c.startBeat), audio (c.audio), sourceSampleRate (c.sourceSampleRate)
    {
        setStatusMessage ("Listening for the parts of \"" + c.name + "\"...");
    }

    void run() override
    {
        setProgress (-1.0);
        sections = sf::analyse (sf::prepareInput (audio, sourceSampleRate), startBeat, songTempo);

        if (! threadShouldExit())
            tempo = tf::analyse (tf::prepareInput (audio, sourceSampleRate));   // to check the bars line up
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    StructureDetector& owner;
    Track* track;
    AudioClip* clip;
    TempoMap songTempo;   // copied, so the song isn't read from another thread
    double startBeat;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sourceSampleRate;
    std::vector<sf::FoundSection> sections;
    std::optional<tf::Tempo> tempo;
};

//==============================================================================
StructureDetector::StructureDetector (Engine& e, juce::Component& p) : engine (e), parent (p) {}

StructureDetector::~StructureDetector() = default;

void StructureDetector::detect (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    job = std::make_unique<Job> (*this, track, clip, engine.getTempoMap());
    job->launchThread();
}

void StructureDetector::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    // The clip may have gone while it listened.
    if (cancelled || engine.findTrackFor (done->clip) != done->track)
        return;

    if (done->sections.empty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Detect song structure",
                                                "No parts were found in \"" + done->clip->name + "\": it's silent, or shorter than half a bar.");
        return;
    }

    offer (*done->track, *done->clip, done->sections, done->tempo, done->songTempo);
}

void StructureDetector::offer (Track& track, AudioClip& clip, const std::vector<sf::FoundSection>& found,
                               const std::optional<tf::Tempo>& tempo, const TempoMap& songTempo)
{
    SectionList sections;
    juce::StringArray lines;
    std::map<int, juce::String> firstOfGroup;

    for (const auto& f : found)
    {
        const auto& s = f.section;
        sections.push_back (s);

        // Why it got its name the first time; after that, which part it's the music of.
        const auto first = firstOfGroup.find (f.group);
        const auto why = first != firstOfGroup.end() ? "the same music as " + first->second : f.reason;
        firstOfGroup.emplace (f.group, s.getDisplayName());

        if (lines.size() < 16)
            lines.add (s.getDisplayName() + ", bars " + juce::String (s.startBar + 1) + "-" + juce::String (s.getEndBar())
                       + " (" + sf::getGroupName (f.group) + "): " + why);
    }

    if (found.size() > 16)
        lines.add ("and " + juce::String ((int) found.size() - 16) + " parts more");

    juce::String message = "The parts of \"" + clip.name + "\", on the song's bars:\n\n" + lines.joinIntoString ("\n")
                         + "\n\nThe music in order: " + sf::getPattern (found) + ". The same letter is the same music heard again.";

    if (const auto name = gen::structure::findStructureName (sections); name.isNotEmpty())
        message += " This is the " + name + " structure from the song structure guide.";

    // Do the recording's beats and bars line up with the song's?
    bool linesUp = true;

    if (tempo.has_value())
    {
        const auto advice = adviseLiningUp (clip, *tempo, songTempo, "sections");
        linesUp = advice.isEmpty();

        if (! linesUp)
            message += "\n\n" + advice;
    }

    // There's no undo yet: say what goes.
    const int from = sections.front().startBar, to = sections.back().getEndBar();
    const auto replaced = std::count_if (engine.getSections().begin(), engine.getSections().end(),
                                         [from, to] (const Section& s) { return s.startBar < to && s.getEndBar() > from; });

    if (replaced > 0)
        message += "\n\nThis replaces the " + juce::String ((int) replaced) + (replaced == 1 ? " section" : " sections")
                 + " on the Sections row from bar " + juce::String (from + 1) + " to " + juce::String (to) + ".";

    message += "\n\nHow it was found: DawIt compares the bars before and after each bar line: their chords, their sound (bright or "
               "dull), how loud they are and how much drums there are. A new part starts where these change, after 4, 8 or 16 bars "
               "where it can, because songs are built from phrases of 4 bars. Parts with the same chords and sound are the same music "
               "heard again. The loudest music that comes back is taken as the chorus, the other music that comes back as the verse, "
               "and a part heard once after the second chorus as a bridge. Check it by ear: a verse and chorus with the same chords "
               "and sound can't be told apart. Drag a section's edges to move them, or click it to change its type.";

    auto* window = new juce::AlertWindow ("Detect song structure", message, juce::MessageBoxIconType::NoIcon, &parent);
    window->addButton ("Put on Sections row", 1, linesUp ? juce::KeyPress (juce::KeyPress::returnKey) : juce::KeyPress());

    if (! linesUp && onDetectTempo != nullptr)
        window->addButton ("Detect tempo first", 2, juce::KeyPress (juce::KeyPress::returnKey));

    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, &track, &clip, sections] (int choice)
    {
        // The clip may have been deleted while the question was up.
        if (choice == 0 || engine.findTrackFor (&clip) != &track)
            return;

        if (choice == 2)
        {
            onDetectTempo (track, clip);
            return;
        }

        engine.setSections (replaceSections (engine.getSections(), sections));

        if (onSectionsSet)
            onSectionsSet();
    }), true);
}
} // namespace daw
