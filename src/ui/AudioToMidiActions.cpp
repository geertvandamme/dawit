#include "AudioToMidiActions.h"
#include "engine/AudioToMidi.h"

namespace daw
{
class AudioToMidiConverter::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (AudioToMidiConverter& o, Track& t, AudioClip& c, Choices ch)
        : juce::ThreadWithProgressWindow ("Convert to MIDI", true, true, 10000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), choices (ch), audio (c.audio), sourceSampleRate (c.sourceSampleRate)
    {
        setStatusMessage ("Listening for the notes in \"" + c.name + "\"...");
    }

    void run() override
    {
        const auto input = audiotomidi::prepareInput (audio, sourceSampleRate);
        const auto activations = audiotomidi::runModel (input, [this] (double done)
        {
            setProgress (done);
            return ! threadShouldExit();
        }, error);

        if (! activations.has_value())
            return;

        // Fewer notes asks for stronger starts and stronger notes; more notes for weaker ones.
        audiotomidi::Settings settings;
        const float thresholds[3][2] = { { 0.6f, 0.4f }, { 0.5f, 0.3f }, { 0.35f, 0.2f } };
        settings.onsetThreshold = thresholds[choices.sensitivity][0];
        settings.frameThreshold = thresholds[choices.sensitivity][1];

        notes = audiotomidi::findNotes (*activations, settings);

        if (choices.oneAtATime)
            notes = audiotomidi::keepOneAtATime (std::move (notes), settings.minNoteSeconds);
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    AudioToMidiConverter& owner;
    Track* track;
    AudioClip* clip;
    Choices choices;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sourceSampleRate;
    juce::String error;
    std::vector<audiotomidi::Note> notes;
};

//==============================================================================
AudioToMidiConverter::AudioToMidiConverter (Engine& e, juce::Component& p) : engine (e), parent (p) {}

AudioToMidiConverter::~AudioToMidiConverter() = default;

void AudioToMidiConverter::convert (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    // A bass, a voice or a melody is one note at a time; that also drops the overtones the model hears.
    const auto names = (track.name + " " + clip.name).toLowerCase();
    const bool single = names.contains ("bass") || names.contains ("vocal") || names.contains ("voice")
                        || names.contains ("lead") || names.contains ("melody");

    auto* window = new juce::AlertWindow ("Convert to MIDI",
                                          "Finds the notes in \"" + clip.name + "\" and puts them in a new MIDI track below it. "
                                          "Works best on one instrument or voice, such as a stem. For drums, use Hits to MIDI... instead.",
                                          juce::MessageBoxIconType::NoIcon, &parent);

    window->addComboBox ("notes", { "Several (chords and melody)", "One at a time (melody, bass, voice)" }, "Notes at once");
    window->getComboBoxComponent ("notes")->setSelectedItemIndex (single ? 1 : 0);
    window->addComboBox ("sensitivity", { "Fewer notes", "Normal", "More notes" }, "Sensitivity");
    window->getComboBoxComponent ("sensitivity")->setSelectedItemIndex (1);
    window->addComboBox ("grid", { "Off", "1/32", "1/16", "1/8" }, "Quantize");
    window->getComboBoxComponent ("grid")->setSelectedItemIndex (0);
    window->addButton ("Convert", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window, &track, &clip] (int result)
    {
        // The clip may have been deleted while the question was up.
        if (result != 1 || engine.findTrackFor (&clip) != &track)
            return;

        const double grids[] = { 0.0, 0.125, 0.25, 0.5 };
        Choices choices;
        choices.oneAtATime = window->getComboBoxComponent ("notes")->getSelectedItemIndex() == 1;
        choices.sensitivity = juce::jlimit (0, 2, window->getComboBoxComponent ("sensitivity")->getSelectedItemIndex());
        choices.gridBeats = grids[juce::jlimit (0, 3, window->getComboBoxComponent ("grid")->getSelectedItemIndex())];
        start (track, clip, choices);
    }), true);
}

void AudioToMidiConverter::start (Track& track, AudioClip& clip, Choices choices)
{
    if (job != nullptr)
        return;

    job = std::make_unique<Job> (*this, track, clip, choices);
    job->launchThread();
}

void AudioToMidiConverter::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    if (cancelled)
        return;

    if (done->error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't convert to MIDI", done->error);
        return;
    }

    // The progress window blocks editing, but check that the clip is still there anyway.
    if (engine.findTrackFor (done->clip) != done->track)
        return;

    const double lengthBeats = done->clip->getLengthBeats (engine.getTempoMap());
    auto notes = audiotomidi::toClipNotes (done->notes, engine.getTempoMap(), done->clip->startBeat, lengthBeats, done->choices.gridBeats);

    if (notes.empty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Convert to MIDI",
                                                "No notes were found in \"" + done->clip->name + "\". "
                                                "Try More notes, or a clip with a clearer pitched sound.");
        return;
    }

    auto* midiClip = audiotomidi::addMidiTrack (engine, *done->track, *done->clip, std::move (notes));

    if (auto* midiTrack = engine.findTrackFor (midiClip); midiTrack != nullptr && onConverted)
        onConverted (*midiTrack, *midiClip);
}
} // namespace daw
