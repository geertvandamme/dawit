#include "StemActions.h"
#include "engine/ModelDownload.h"

namespace daw
{
class StemSplitter::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (StemSplitter& o, Track& t, AudioClip& c, std::optional<stems::KaraokeChoices> k)
        : juce::ThreadWithProgressWindow (k.has_value() ? "Remove vocals" : "Split into stems", true, true, 30000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), karaoke (k),
          input (stems::prepareInput (c.audio, c.sourceSampleRate))
    {
        setStatusMessage ("Starting...");
    }

    void run() override
    {
        auto keepGoing = [this] (double done)
        {
            setProgress (done);
            return ! threadShouldExit();
        };

        if (! stems::isModelInstalled())
        {
            setStatusMessage ("Downloading the Demucs model...");
            error = stems::downloadModel (keepGoing);

            if (error.isNotEmpty() || threadShouldExit())
                return;
        }

        setStatusMessage ("Loading the Demucs model...");
        setProgress (-1.0);

        stems::Separator separator;
        if (! separator.load (stems::getModelFile(), error))
            return;

        setStatusMessage ("Separating drums, bass, vocals and other sounds of \"" + clip->name + "\".\n"
                          "This takes about as long as the clip plays, or longer on a slower Mac.");
        setProgress (0.0);
        result = separator.separate (input, keepGoing);

        if (! result.has_value() && ! threadShouldExit())
            error = "The separation failed. There may not be enough free memory; try a shorter clip or close other apps.";
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    StemSplitter& owner;
    Track* track;
    AudioClip* clip;
    std::optional<stems::KaraokeChoices> karaoke;   // set for a karaoke version
    juce::AudioBuffer<float> input;   // copied up front, so the clip isn't read from another thread
    juce::String error;
    std::optional<stems::Stems> result;
};

//==============================================================================
StemSplitter::StemSplitter (Engine& e, juce::Component& p) : engine (e), parent (p) {}

StemSplitter::~StemSplitter() = default;

void StemSplitter::split (Track& track, AudioClip& clip)
{
    downloadThenStart (track, clip, std::nullopt);
}

void StemSplitter::removeVocals (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    auto* settings = engine.properties.getUserSettings();

    juce::String message = "Makes a version of \"" + clip.name + "\" without its vocals, to learn the vocal part with Sing along "
                           "showing the words, or to practise an instrument with Play along.\n\n"
                           "Demucs, a model that listens to what each sound is, splits the clip into drums, bass, vocals and "
                           "other sounds, as Split into stems does. The song without the vocals goes on a new Karaoke track below "
                           "it, the vocals on a Vocals track under that, and the clip is muted, so nothing plays twice.\n\n"
                           "It removes the lead and backing vocals. Some reverb or echo of the voice can stay faintly, and so can "
                           "a melody an instrument plays along with the voice. Why taking out the middle of the stereo (left "
                           "minus right) doesn't do this is in What is sound?, under Taking the vocals out.\n\n"
                           "It runs on this Mac and takes about as long as the clip plays"
                         + juce::String (stems::isModelInstalled() ? "." : "; the model is downloaded from Hugging Face the first time ("
                                                                             + models::formatSize (stems::getModelDownloadSize()) + ").")
                         + " Use karaoke versions of other people's songs to practise, not to publish.";

    auto* window = new juce::AlertWindow ("Remove vocals (karaoke version)", message, juce::MessageBoxIconType::NoIcon, &parent);

    window->addComboBox ("instrumental", { "One clip on a Karaoke track", "As stems: drums, bass and other sounds on tracks of their own" },
                         "Without the vocals");
    window->getComboBoxComponent ("instrumental")->setSelectedItemIndex (settings->getBoolValue ("karaoke.asStems", false) ? 1 : 0);

    juce::StringArray guides;
    for (auto guide : stems::allGuideLevels)
        guides.add (stems::getGuideName (guide));

    window->addComboBox ("guide", guides, "Guide vocals");
    window->getComboBoxComponent ("guide")->setSelectedItemIndex (juce::jlimit (0, guides.size() - 1, settings->getIntValue ("karaoke.guideLevel", 0)));

    window->addButton ("Remove vocals", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window, &track, &clip] (int result)
    {
        // The clip may have been deleted while the question was up.
        if (result != 1 || engine.findTrackFor (&clip) != &track)
            return;

        const bool asStems = window->getComboBoxComponent ("instrumental")->getSelectedItemIndex() == 1;
        const int guide = juce::jlimit (0, (int) std::size (stems::allGuideLevels) - 1, window->getComboBoxComponent ("guide")->getSelectedItemIndex());
        engine.properties.getUserSettings()->setValue ("karaoke.asStems", asStems);
        engine.properties.getUserSettings()->setValue ("karaoke.guideLevel", guide);

        stems::KaraokeChoices choices;
        choices.asStems = asStems;
        choices.guide = stems::allGuideLevels[guide];

        // Once the dialog has gone, so the download question doesn't open under it.
        juce::MessageManager::callAsync ([this, safe = juce::Component::SafePointer<juce::Component> (&parent), &track, &clip, choices]
        {
            if (safe != nullptr && engine.findTrackFor (&clip) == &track)
                downloadThenStart (track, clip, choices);
        });
    }), true);
}

void StemSplitter::downloadThenStart (Track& track, AudioClip& clip, std::optional<stems::KaraokeChoices> karaoke)
{
    if (job != nullptr)
        return;

    if (stems::isModelInstalled())
    {
        start (track, clip, karaoke);
        return;
    }

    const auto megabytes = juce::String (juce::roundToInt ((double) stems::getModelDownloadSize() / 1.0e6));

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Download the stem model?",
                                        juce::String (karaoke.has_value() ? "Removing the vocals" : "Splitting into stems")
                                            + " uses Demucs, a model that separates a mix into drums, bass, "
                                            "vocals and other sounds. It runs on this Mac, but has to be downloaded once from "
                                            "Hugging Face (" + megabytes + " MB).",
                                        "Download", "Cancel", &parent,
                                        juce::ModalCallbackFunction::create ([this, &track, &clip, karaoke] (int result)
                                        {
                                            // The clip may have been deleted while the question was up.
                                            if (result == 1 && engine.findTrackFor (&clip) == &track)
                                                start (track, clip, karaoke);
                                        }));
}

void StemSplitter::start (Track& track, AudioClip& clip, std::optional<stems::KaraokeChoices> karaoke)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    job = std::make_unique<Job> (*this, track, clip, karaoke);
    job->launchThread();
}

void StemSplitter::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    if (cancelled)
        return;

    if (done->error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                done->karaoke.has_value() ? "Couldn't remove the vocals" : "Couldn't split into stems", done->error);
        return;
    }

    // The progress window blocks editing, but check that the clip is still there anyway.
    if (! done->result.has_value() || engine.findTrackFor (done->clip) != done->track)
        return;

    if (done->karaoke.has_value())
    {
        const auto karaoke = stems::addKaraokeTracks (engine, *done->track, *done->clip, std::move (*done->result), *done->karaoke);

        if (karaoke.vocals == nullptr)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Remove vocals",
                                                    "Demucs heard no vocals in \"" + done->clip->name + "\", so it already is "
                                                    "a version without them. Nothing was added.");
        else if (onStemsAdded)
            onStemsAdded (karaoke.instrumental);

        return;
    }

    const auto added = stems::addStemTracks (engine, *done->track, *done->clip, std::move (*done->result));

    if (added.empty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Split into stems",
                                                "\"" + done->clip->name + "\" is silent, so there are no stems to add.");
    else if (onStemsAdded)
        onStemsAdded (added);
}
} // namespace daw
