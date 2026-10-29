#include "LyricsActions.h"
#include "analysis/OnsetFinder.h"
#include "engine/Stems.h"

namespace daw
{
namespace lf = analysis::lyricsfinder;

namespace
{
    juce::String barOf (double beat)
    {
        return juce::String ((int) std::floor (beat / 4.0 + 1.0e-6) + 1);
    }
}

class LyricsDetector::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (LyricsDetector& o, Track& t, AudioClip& c, Choices ch)
        : juce::ThreadWithProgressWindow ("Detect lyrics", true, true, 30000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), choices (ch), audio (c.audio), sourceSampleRate (c.sourceSampleRate)
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

        juce::AudioBuffer<float> vocals;
        double vocalsRate = sourceSampleRate;

        if (choices.onlyVocals)
        {
            vocals = std::move (audio);
        }
        else
        {
            if (! stems::isModelInstalled())
            {
                setStatusMessage ("Downloading Demucs, the model that finds the vocals...");
                error = stems::downloadModel (keepGoing);

                if (error.isNotEmpty() || threadShouldExit())
                    return;
            }

            setStatusMessage ("Loading Demucs...");
            setProgress (-1.0);

            std::optional<stems::Stems> separated;

            {
                stems::Separator separator;   // gone before Whisper loads, to keep memory free

                if (! separator.load (stems::getModelFile(), error))
                    return;

                setStatusMessage ("Finding the vocals in \"" + clip->name + "\" with Demucs.\n"
                                  "This takes about as long as the clip plays.");
                setProgress (0.0);
                separated = separator.separate (stems::prepareInput (audio, sourceSampleRate), keepGoing);
            }

            if (! separated.has_value())
            {
                if (! threadShouldExit())
                    error = "Finding the vocals failed. There may not be enough free memory; try a shorter clip or close other apps.";

                return;
            }

            if (stems::isSilent (*separated, stems::vocalsStem))
                return;   // no words: the message says there was no singing

            vocalStemAudio = std::move ((*separated)[stems::vocalsStem]);
            vocals = *vocalStemAudio;
            vocalsRate = stems::sampleRate;
        }

        const auto& model = lf::getModel (choices.model);
        const auto modelName = lf::getModelName (choices.model);

        if (! models::isInstalled (model))
        {
            setStatusMessage ("Downloading " + modelName + ", " + models::formatSize (model.size) + "...");
            error = models::download (model, keepGoing);

            if (error.isNotEmpty() || threadShouldExit())
                return;
        }

        auto mono = lf::prepareInput (vocals, vocalsRate);
        const auto singing = lf::findSinging (mono, lf::sampleRate);

        if (singing.empty())
            return;

        lf::muteOutside (mono, lf::sampleRate, singing);

        setStatusMessage ("Loading " + modelName + "...");
        setProgress (-1.0);

        lf::Transcriber transcriber;

        if (! transcriber.load (models::getFile (model), choices.model, error))
            return;

        setStatusMessage ("Listening for the words in \"" + clip->name + "\" (English only).");
        setProgress (0.0);

        if (auto heard = transcriber.transcribe (mono, keepGoing, error))
        {
            words = lf::cleanUp (std::move (*heard), singing);

            if (choices.snapToSinging)
            {
                setStatusMessage ("Fitting the words to the singing.");
                setProgress (-1.0);
                lf::snapToOnsets (words, analysis::onsets::findOnsets (mono, lf::sampleRate));
            }
        }
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    LyricsDetector& owner;
    Track* track;
    AudioClip* clip;
    Choices choices;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sourceSampleRate;
    juce::String error;
    std::vector<lf::Word> words;
    std::optional<juce::AudioBuffer<float>> vocalStemAudio;   // the vocals Demucs found in a full mix
};

//==============================================================================
LyricsDetector::LyricsDetector (Engine& e, juce::Component& p) : engine (e), parent (p) {}

LyricsDetector::~LyricsDetector() = default;

void LyricsDetector::detect (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    const double from = clip.startBeat, to = clip.getEndBeat (engine.getTempoMap());
    const auto names = (track.name + " " + clip.name).toLowerCase();
    const bool vocalsOnly = names.contains ("vocal") || names.contains ("voice") || names.contains ("vox") || names.contains ("sing");

    const bool replaces = std::any_of (engine.getLyrics().lines.begin(), engine.getLyrics().lines.end(),
                                       [from, to] (const LyricLine& l) { return l.endBeat > from && l.startBeat < to; });

    juce::String message = "Listens to the vocals in \"" + clip.name + "\" and writes the words on the lyrics track, "
                           "timed to the singing, over bars " + barOf (from) + " to " + barOf (to - 1.0e-3) + ".\n\n"
                           "English vocals only for now. It uses Whisper's English models and splits the words into "
                           "syllables by English spelling, so vocals in another language come out as English words "
                           "that sound alike.";

    message += "\n\nWord timing: Whisper's times can be slightly off on singing, so by default each word is moved to where the "
               "voice begins (by at most 0.15 s), and long words are split where the voice starts new syllables.";

    message += "\n\nFor the best result, set the song's tempo first with Detect tempo... in the clip's menu, so the words land "
               "on the right bars and sections. From a full mix, the vocals it finds are kept on a new muted Vocals track below the clip.";

    if (replaces)
        message += " The lyrics already on those bars are replaced.";

    message += "\n\nEverything runs on this Mac; the music isn't sent anywhere. The model you choose is downloaded "
               "from Hugging Face the first time"
             + juce::String (stems::isModelInstalled() ? "." : ", and Demucs (" + models::formatSize (stems::getModelDownloadSize())
                                                                  + ") to find the vocals in a full mix.")
             + "\n\nLyrics of other people's songs belong to them: use what it finds to learn and practise, not to publish.";

    auto* window = new juce::AlertWindow ("Detect lyrics (English only)", message, juce::MessageBoxIconType::NoIcon, &parent);

    window->addComboBox ("source", { "A full mix: find the vocals first", "Only vocals (a vocal track or stem)" }, "The clip is");
    window->getComboBoxComponent ("source")->setSelectedItemIndex (vocalsOnly ? 1 : 0);

    auto modelItem = [] (lf::ModelSize size, const juce::String& what)
    {
        const auto& model = lf::getModel (size);
        return what + (models::isInstalled (model) ? juce::String (" (downloaded)") : " (" + models::formatSize (model.size) + " download)");
    };

    window->addComboBox ("model", { modelItem (lf::ModelSize::small, "Small: quicker"),
                                    modelItem (lf::ModelSize::medium, "Medium: hears more words, about 3 times slower") },
                         "Whisper model");
    window->getComboBoxComponent ("model")->setSelectedItemIndex (
        juce::jlimit (0, 1, engine.properties.getUserSettings()->getIntValue ("analysis.lyricsModel", 0)));

    window->addComboBox ("snap", { "Snap words to the singing", "As Whisper heard them" }, "Word timing");
    window->getComboBoxComponent ("snap")->setTooltip ("Whisper's word times can be a little off on singing. Snapping moves each word's start "
                                                       "to where the voice begins, up to 0.15 s away, and places syllables on the voice's "
                                                       "own starts where it finds them. Words with no clear start nearby stay where Whisper put them.");
    window->getComboBoxComponent ("snap")->setSelectedItemIndex (
        engine.properties.getUserSettings()->getBoolValue ("analysis.lyricsSnap", true) ? 0 : 1);

    window->addButton ("Detect", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window, &track, &clip] (int result)
    {
        // The clip may have been deleted while the question was up.
        if (result != 1 || engine.findTrackFor (&clip) != &track)
            return;

        const int model = juce::jlimit (0, 1, window->getComboBoxComponent ("model")->getSelectedItemIndex());
        engine.properties.getUserSettings()->setValue ("analysis.lyricsModel", model);

        const bool snap = window->getComboBoxComponent ("snap")->getSelectedItemIndex() == 0;
        engine.properties.getUserSettings()->setValue ("analysis.lyricsSnap", snap);

        Choices choices;
        choices.snapToSinging = snap;
        choices.onlyVocals = window->getComboBoxComponent ("source")->getSelectedItemIndex() == 1;
        choices.model = model == 1 ? lf::ModelSize::medium : lf::ModelSize::small;
        start (track, clip, choices);
    }), true);
}

void LyricsDetector::start (Track& track, AudioClip& clip, Choices choices)
{
    if (job != nullptr)
        return;

    job = std::make_unique<Job> (*this, track, clip, choices);
    job->launchThread();
}

void LyricsDetector::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    // The vocals found in a full mix go on a muted track below the clip, to listen to or use later,
    // even when no words were found or the listening was cancelled after the separation.
    if (done->vocalStemAudio.has_value() && engine.findTrackFor (done->clip) == done->track)
    {
        auto* vocals = stems::addStemTrack (engine, engine.indexOf (done->track) + 1, *done->clip, stems::vocalsStem,
                                            std::move (*done->vocalStemAudio));
        vocals->muted = true;
    }

    if (cancelled)
        return;

    if (done->error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't detect the lyrics", done->error);
        return;
    }

    // The progress window blocks editing, but check that the clip is still there anyway.
    if (engine.findTrackFor (done->clip) != done->track)
        return;

    const auto& tempo = engine.getTempoMap();
    const double from = done->clip->startBeat, to = done->clip->getEndBeat (tempo);
    const auto found = lf::toLyrics (done->words, from, tempo, engine.getSections());

    if (found.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Detect lyrics",
                                                "No sung words were found in \"" + done->clip->name + "\". "
                                                + (done->choices.onlyVocals ? "If it's a full mix, choose \"A full mix\" so the vocals are found first. "
                                                                            : juce::String())
                                                + "Only English vocals are recognised for now.");
        return;
    }

    engine.setLyrics (lf::replaceBetween (engine.getLyrics(), found, from, to));

    if (onLyricsFound)
        onLyricsFound();
}
} // namespace daw
