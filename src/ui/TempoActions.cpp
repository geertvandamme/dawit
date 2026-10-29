#include "TempoActions.h"
#include "engine/Tempo.h"

namespace daw
{
namespace tf = analysis::tempofinder;

class TempoDetector::Job final : public juce::ThreadWithProgressWindow
{
public:
    Job (TempoDetector& o, Track& t, AudioClip& c)
        : juce::ThreadWithProgressWindow ("Detect tempo", true, true, 10000, "Cancel", &o.parent),
          owner (o), track (&t), clip (&c), audio (c.audio), sourceSampleRate (c.sourceSampleRate)
    {
        setStatusMessage ("Listening for the beat in \"" + c.name + "\"...");
    }

    void run() override
    {
        setProgress (-1.0);
        const auto mono = tf::prepareInput (audio, sourceSampleRate);

        if (! threadShouldExit())
            tempo = tf::analyse (mono);
    }

    void threadComplete (bool cancelled) override
    {
        owner.finished (cancelled);   // deletes this
    }

    TempoDetector& owner;
    Track* track;
    AudioClip* clip;
    juce::AudioBuffer<float> audio;   // copied up front, so the clip isn't read from another thread
    double sourceSampleRate;
    std::optional<tf::Tempo> tempo;
};

//==============================================================================
TempoDetector::TempoDetector (Engine& e, juce::Component& p) : engine (e), parent (p) {}

TempoDetector::~TempoDetector() = default;

void TempoDetector::detect (Track& track, AudioClip& clip)
{
    if (job != nullptr || clip.audio.getNumSamples() == 0)
        return;

    job = std::make_unique<Job> (*this, track, clip);
    job->launchThread();
}

void TempoDetector::finished (bool cancelled)
{
    const auto done = std::move (job);   // deleted when this returns, which the progress window allows

    if (cancelled || engine.findTrackFor (done->clip) != done->track)
        return;

    if (! done->tempo.has_value())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Detect tempo",
                                                "No clear beat was found in \"" + done->clip->name + "\". It may have no drums or "
                                                "other clear rhythm, or be shorter than about 6 seconds.");
        return;
    }

    offer (*done->track, *done->clip, *done->tempo);
}

void TempoDetector::offer (Track& track, AudioClip& clip, const tf::Tempo& tempo)
{
    const double bpm = tf::roundBpm (tempo.bpm);

    // A band that drifts can be followed bar by bar, when there are at least two bars to measure.
    std::optional<tf::FollowedTempo> follow;

    if (! tempo.steady)
        if (auto followed = tf::followRecording (tempo, clip.startBeat); followed.bars > 1)
            follow = std::move (followed);

    juce::String message = "\"" + clip.name + "\" is at about " + formatBpm (bpm) + " BPM";

    if (tempo.steady)
        message += ", steady all the way through.";
    else
        message += ", but its tempo moves between " + formatBpm (std::round (tempo.slowest)) + " and "
                 + formatBpm (std::round (tempo.fastest)) + " BPM, like a band playing without a click.";

    if (follow.has_value())
        message += "\n\nFollow the recording gives every bar its own tempo, measured from one of the band's downbeats to the next ("
                 + juce::String (follow->bars) + " bars, from " + formatBpm (follow->range.getStart()) + " to " + formatBpm (follow->range.getEnd())
                 + " BPM). The bars, chords, sections, lyrics and the metronome then stay with the band all the way through, and the "
                   "Tempo row shows how it speeds up and slows down. The clip moves so its first downbeat starts a bar, and the "
                   "recording plays as it is. One tempo can only follow the band on average: its bars drift away from the song's.";

    message += "\n\nHow it was found: DawIt marks the moments where the sound suddenly gets louder, mostly the drums, "
               "and looks for how often that pattern repeats. Then it places the beats on those moments, and takes "
               "the beat where the chords change and the kick hits as the first of a bar. A detector can find half or "
               "double the tempo you'd tap your foot to: pick another one below if it feels too slow or too fast.";

    const auto& songTempo = engine.getTempoMap();
    const auto songRange = songTempo.getRange (0.0, juce::jmax (4.0, engine.getSongEndBar() * 4.0));
    message += "\n\n" + (songTempo.isSteady() ? "The song is at " + formatBpm (songTempo.getBpmAt (0.0)) + " BPM now."
                                              : "The song's tempo follows a recording now, from " + formatBpm (songRange.getStart()) + " to "
                                                    + formatBpm (songRange.getEnd()) + " BPM; the new tempo replaces it.")
             + " With a new tempo, MIDI clips, chords, sections and lyrics stay on their bars; audio keeps its speed, so audio "
               "clips get longer or shorter on the timeline.";

    auto* window = new juce::AlertWindow ("Detect tempo", message, juce::MessageBoxIconType::NoIcon, &parent);

    // Following the recording first when it drifts, then the tempo found, half and double, within
    // the tempos the song can have. 0 stands for following.
    std::vector<double> choices;
    juce::StringArray items;

    if (follow.has_value())
    {
        choices.push_back (0.0);
        items.add ("Follow the recording, bar by bar (" + formatBpm (follow->range.getStart()) + " to " + formatBpm (follow->range.getEnd()) + " BPM)");
    }

    choices.push_back (bpm);
    items.add (formatBpm (bpm) + " BPM, as found" + (follow.has_value() ? juce::String (", on average") : juce::String()));

    if (tf::roundBpm (bpm / 2.0) >= 20.0)
    {
        choices.push_back (tf::roundBpm (bpm / 2.0));
        items.add (formatBpm (choices.back()) + " BPM, half (it felt too fast)");
    }

    if (tf::roundBpm (bpm * 2.0) <= 300.0)
    {
        choices.push_back (tf::roundBpm (bpm * 2.0));
        items.add (formatBpm (choices.back()) + " BPM, double (it felt too slow)");
    }

    window->addComboBox ("tempo", items, "Song tempo");
    window->getComboBoxComponent ("tempo")->setSelectedItemIndex (0);
    window->addComboBox ("clip", { "Move it so its first downbeat starts a bar", "Leave it where it is (not when following)" }, "The clip");
    window->getComboBoxComponent ("clip")->setSelectedItemIndex (0);
    window->addButton ("Set tempo", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    const double downbeatSeconds = tempo.getFirstDownbeatSeconds();

    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window, &track, &clip, choices, downbeatSeconds, follow] (int result)
    {
        // The clip may have been deleted while the question was up.
        if (result != 1 || engine.findTrackFor (&clip) != &track)
            return;

        const auto index = (size_t) juce::jlimit (0, (int) choices.size() - 1, window->getComboBoxComponent ("tempo")->getSelectedItemIndex());

        if (follow.has_value() && juce::exactlyEqual (choices[index], 0.0))
        {
            // Following only works with the downbeats on the bar lines, so the clip always moves.
            engine.setTempoMap (follow->map);
            const juce::ScopedLock sl (engine.getLock());
            clip.startBeat = follow->clipStartBeat;
        }
        else
        {
            const double newBpm = choices[index];
            engine.setTempo (newBpm);

            if (window->getComboBoxComponent ("clip")->getSelectedItemIndex() == 0)
            {
                // The nearest bar line to where the downbeat is now; a bar later if the clip would start before the song.
                const double offset = downbeatSeconds * newBpm / 60.0;
                double start = std::round ((clip.startBeat + offset) / 4.0) * 4.0 - offset;

                while (start < 0.0)
                    start += 4.0;

                const juce::ScopedLock sl (engine.getLock());
                clip.startBeat = start;
            }
        }

        engine.sendChangeMessage();   // the views redraw the clips at the new tempo

        if (onTempoSet)
            onTempoSet();
    }), true);
}

//==============================================================================
juce::String adviseLiningUp (const AudioClip& clip, const tf::Tempo& tempo, const TempoMap& song, const juce::String& what)
{
    if (! song.isSteady())
    {
        // The song's tempo changes (it follows a recording): check the recording's downbeats land on bar lines.
        int onBars = 0, downbeats = 0;

        for (size_t i = (size_t) juce::jmax (0, tempo.firstDownbeat); i < tempo.beats.size(); i += 4, ++downbeats)
        {
            const double beat = song.getBeatAfter (clip.startBeat, tempo.beats[i]);
            onBars += std::abs (beat / 4.0 - std::round (beat / 4.0)) * 4.0 <= 0.35 ? 1 : 0;
        }

        if (downbeats > 0 && onBars < downbeats * 8 / 10)
            return "For the best result, set the song's tempo from this recording first: only " + juce::String (onBars) + " of its "
                 + juce::String (downbeats) + " bars start on one of the song's bar lines, so its " + what + " fall across them. "
                   "Choose Detect tempo first, set the tempo, then detect the " + what + " again.";

        return {};
    }

    // Half or double the tempo counts: the bars still line up.
    const double songBpm = song.getBpmAt (clip.startBeat);
    const double ratio = songBpm / tempo.bpm;
    const bool sameTempo = std::abs (ratio - 1.0) < 0.015 || std::abs (ratio - 2.0) < 0.03 || std::abs (ratio - 0.5) < 0.008;
    const double downbeat = clip.startBeat + tempo.getFirstDownbeatSeconds() * songBpm / 60.0;
    const double offBar = std::abs (downbeat / 4.0 - std::round (downbeat / 4.0)) * 4.0;

    if (! sameTempo)
        return "For the best result, set the song's tempo first: the recording is at about " + formatBpm (tf::roundBpm (tempo.bpm))
             + " BPM and the song at " + formatBpm (songBpm) + " BPM, so its " + what + " don't fall on the song's bars. "
               "Choose Detect tempo first, set the tempo, then detect the " + what + " again.";

    if (! tempo.steady)
        return "For the best result, let the song follow this recording first: its tempo moves between " + formatBpm (std::round (tempo.slowest))
             + " and " + formatBpm (std::round (tempo.fastest)) + " BPM, so with one tempo its bars drift away from the song's and its "
             + what + " fall across them. Choose Detect tempo first and Follow the recording, then detect the " + what + " again.";

    if (offBar > 0.35)
        return "For the best result, line the clip up with the bars first: its bars start " + juce::String (offBar, 1)
             + " beats away from the song's bar lines, so its " + what + " fall across the bar lines. Detect tempo can move it.";

    return {};
}
} // namespace daw
