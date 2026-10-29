#include "ChordActions.h"
#include "TempoActions.h"

namespace daw
{
namespace cf = analysis::chordfinder;
namespace tf = analysis::tempofinder;

namespace
{
    juce::String keyName (const MusicalKey& key)
    {
        return MusicalKey::getTonicName (key.tonic) + " " + MusicalKey::getScaleName (key.scale);
    }
}

class ChordDetector::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (ChordDetector& o, Track& t, AudioClip& c, const TempoMap& songTempoMap)
        : juce::ThreadWithProgressWindow ("Detect chords", true, true, 10000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), songTempo (songTempoMap), startBeat (c.startBeat), audio (c.audio), sourceSampleRate (c.sourceSampleRate)
    {
        setStatusMessage ("Listening for the chords in \"" + c.name + "\"...");
    }

    void run() override
    {
        setProgress (-1.0);
        result = cf::analyse (cf::prepareInput (audio, sourceSampleRate), startBeat, songTempo);

        if (! threadShouldExit())
            tempo = tf::analyse (tf::prepareInput (audio, sourceSampleRate));   // to check the bars line up
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    ChordDetector& owner;
    Track* track;
    AudioClip* clip;
    TempoMap songTempo;   // copied, so the song isn't read from another thread
    double startBeat;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sourceSampleRate;
    cf::Result result;
    std::optional<tf::Tempo> tempo;
};

//==============================================================================
ChordDetector::ChordDetector (Engine& e, juce::Component& p) : engine (e), parent (p) {}

ChordDetector::~ChordDetector() = default;

void ChordDetector::detect (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    job = std::make_unique<Job> (*this, track, clip, engine.getTempoMap());
    job->launchThread();
}

void ChordDetector::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    // The clip may have gone, or moved or the tempo changed, while it listened.
    if (cancelled || engine.findTrackFor (done->clip) != done->track)
        return;

    const bool hasChords = std::any_of (done->result.bars.begin(), done->result.bars.end(), [] (const cf::BarChord& b) { return b.chord.has_value(); });

    if (! hasChords)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Detect chords",
                                                "No chords were found in \"" + done->clip->name + "\": it may be drums only, "
                                                "or too short to fill half a bar.");
        return;
    }

    offer (*done->track, *done->clip, done->result, done->tempo, done->songTempo);
}

void ChordDetector::offer (Track& track, AudioClip& clip, const cf::Result& result, const std::optional<tf::Tempo>& tempo, const TempoMap& songTempo)
{
    // The chords as a chord sheet, four bars a line, each line with its first bar's number.
    juce::StringArray lines;

    for (size_t i = 0; i < result.bars.size() && lines.size() < 8; i += 4)
    {
        juce::StringArray names;

        for (size_t j = i; j < juce::jmin (i + 4, result.bars.size()); ++j)
            names.add (result.bars[j].chord.has_value() ? result.bars[j].chord->getName() : juce::String ("-"));

        lines.add ("Bar " + juce::String (result.bars[i].bar + 1) + ":   " + names.joinIntoString ("  |  "));
    }

    if (result.bars.size() > 32)
        lines.add ("and " + juce::String ((int) result.bars.size() - 32) + " bars more");

    juce::String message = "The chords in \"" + clip.name + "\", one per bar of the song:\n\n" + lines.joinIntoString ("\n")
                         + "\n\nThey sound like " + keyName (result.key) + "; the song is in " + keyName (engine.getKey()) + " now.";

    // Do the recording's beats and bars line up with the song's?
    bool linesUp = true;

    if (tempo.has_value())
    {
        const auto advice = adviseLiningUp (clip, *tempo, songTempo, "chords");
        linesUp = advice.isEmpty();
        message += "\n\n" + (linesUp ? juce::String ("The recording's tempo and bars line up with the song's, so each chord falls on its own bar.") : advice);
    }

    message += "\n\nHow it was found: DawIt listens to which notes sound in each bar (the 12 note names, in any octave, without the "
               "drums) and compares them with every major, minor and seventh chord. The bass note decides between close ones, and "
               "chords of the key win a tie. Check them by ear: passing notes, a busy bass or a sung melody can fool it.";

    auto* window = new juce::AlertWindow ("Detect chords", message, juce::MessageBoxIconType::NoIcon, &parent);
    const bool newKey = result.key != engine.getKey();

    if (newKey)
    {
        window->addComboBox ("key", { "Set the song's key to " + keyName (result.key), "Keep " + keyName (engine.getKey()) }, "Key");
        window->getComboBoxComponent ("key")->setSelectedItemIndex (0);
    }

    window->addButton ("Put on chord track", 1, linesUp ? juce::KeyPress (juce::KeyPress::returnKey) : juce::KeyPress());

    if (! linesUp && onDetectTempo != nullptr)
        window->addButton ("Detect tempo first", 2, juce::KeyPress (juce::KeyPress::returnKey));

    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window, &track, &clip, result, newKey] (int choice)
    {
        // The clip may have been deleted while the question was up.
        if (choice == 0 || engine.findTrackFor (&clip) != &track)
            return;

        if (choice == 2)
        {
            onDetectTempo (track, clip);
            return;
        }

        auto chords = engine.getChords();

        for (const auto& bar : result.bars)
        {
            if (bar.chord.has_value())
                chords[bar.bar] = *bar.chord;
            else
                chords.erase (bar.bar);
        }

        engine.setChords (std::move (chords));

        if (newKey && window->getComboBoxComponent ("key")->getSelectedItemIndex() == 0)
            engine.setKey (result.key);

        if (onChordsSet)
            onChordsSet();
    }), true);
}
} // namespace daw
