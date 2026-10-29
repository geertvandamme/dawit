#include "SingActions.h"
#include "engine/DiffSinger.h"

namespace daw
{
namespace st = gen::singthis;

namespace
{
    /** A voice's name for a track: "Hoshino Hanami ~AI-dol~ for DiffSinger" becomes "Hoshino Hanami". */
    juce::String getShortName (juce::String name)
    {
        for (auto* cut : { " ~", " for DiffSinger", " (" })
            if (name.contains (cut))
                name = name.upToFirstOccurrenceOf (cut, false, false);

        return name.trim().isEmpty() ? juce::String ("Voice") : name.trim();
    }
}

class SingActions::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (SingActions& o, SingRequest r, std::string trackName)
        : juce::ThreadWithProgressWindow ("Singing", true, true, 30000, "Cancel", &o.parent),
          owner (o), request (std::move (r)), name (std::move (trackName))
    {
        setStatusMessage ("Loading the voice...");
    }

    void run() override
    {
        auto cancel = [this] { return threadShouldExit(); };

        setProgress (-1.0);
        auto singer = sing::DiffSinger::create (request.voice, error);

        if (singer == nullptr)
            return;

        setStatusMessage ("Working out how each sound lasts...");
        auto score = st::buildScore (request.melody, request.pairing, &request.voice, request.settings, singer.get(), cancel);

        if (threadShouldExit())
            return;

        if (score.phrases.empty())
        {
            error = "There is nothing to sing: no note has a syllable.";
            return;
        }

        for (auto& w : singer->getWarnings())
            warnings.add (w);

        setProgress (0.0);
        result = sing::renderPhrases (*singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, request.steps,
                                      [this] (double done, const juce::String& message)
                                      {
                                          if (done >= 0.0)
                                          {
                                              setProgress (done);
                                              setStatusMessage (message);
                                          }

                                          return ! threadShouldExit();
                                      }, error);

        for (auto& w : singer->getWarnings())
            warnings.addIfNotAlreadyThere (w);
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    SingActions& owner;
    SingRequest request;
    std::string name;
    juce::String error;
    juce::StringArray warnings;
    std::optional<sing::Rendering> result;
};

//==============================================================================
SingActions::SingActions (Engine& e, Selection& s, PluginWindowManager& w, juce::Component& p)
    : engine (e), selection (s), pluginWindows (w), parent (p)
{
}

SingActions::~SingActions() = default;

void SingActions::open (Track& track, MidiClip* clip)
{
    if (window != nullptr)
        window->setSource (&track, clip);

    pluginWindows.show (&window, "Sing with a computer voice", [this, &track, clip]
    {
        auto* w = new SingWindow (engine, &track, clip);
        w->onOpenGuide = [this] (gen::lyricsguide::Topic topic) { if (onOpenGuide) onOpenGuide (topic); };
        w->onSing = [this] (SingRequest request) { start (std::move (request)); };
        window = w;
        return w;
    });
}

void SingActions::openForLyrics()
{
    Track* chosen = selection.getTrack() != nullptr && selection.getTrack()->type == Track::Type::midi ? selection.getTrack() : nullptr;

    if (chosen == nullptr)
    {
        // The first MIDI track with notes.
        for (auto& t : engine.getTracks())
        {
            if (t->type != Track::Type::midi)
                continue;

            for (auto& c : t->clips)
                if (c->kind == Clip::Kind::midi && ! static_cast<MidiClip&> (*c).notes.empty())
                    chosen = chosen != nullptr ? chosen : t.get();
        }
    }

    if (chosen == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Sing the lyrics",
                                                "There's no MIDI melody to sing yet. Add a MIDI track with the melody's notes, then try again.");
        return;
    }

    open (*chosen, nullptr);
}

void SingActions::start (SingRequest request)
{
    if (job != nullptr || request.track == nullptr || engine.indexOf (request.track) < 0)
        return;

    const auto name = request.track->name.toStdString();
    job = std::make_unique<Job> (*this, std::move (request), name);
    job->launchThread();
}

void SingActions::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    if (cancelled)
        return;

    if (done->error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't sing this",
                                                done->error + "\n\nIf the voice is the problem, Step by step... in the Sing window says what to check.");
        return;
    }

    // The progress window blocks editing, but check that the melody's track is still there.
    if (! done->result.has_value() || engine.indexOf (done->request.track) < 0)
        return;

    auto* vocal = sing::addVocalTrack (engine, *done->request.track, std::move (*done->result), getShortName (done->request.voice.name), done->request.muteMelody);

    if (vocal != nullptr)
        selection.select (vocal);

    if (! done->warnings.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Sung, with a note",
                                                "The vocal is on a new track. " + done->warnings.joinIntoString ("\n"));
}
} // namespace daw
