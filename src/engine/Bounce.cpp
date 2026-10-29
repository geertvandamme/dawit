#include "Bounce.h"
#include "Routing.h"

namespace daw::bounce
{
juce::Range<double> getLoopRange (const Engine& engine)
{
    return { engine.loopStartBeat.load(), engine.loopEndBeat.load() };
}

juce::Range<double> getSongRange (const Engine& engine)
{
    return { 0.0, engine.getSongEndBar() * 4.0 };
}

juce::Range<double> getDefaultRange (const Engine& engine)
{
    const auto loop = getLoopRange (engine);
    return engine.loopEnabled.load() && loop.getLength() > 0.01 ? loop : getSongRange (engine);
}

//==============================================================================
Renderer::Renderer (Engine& e, const Settings& s)
    : engine (e), settings (s),
      wasPlaying (e.isPlaying()), wasLooping (e.loopEnabled.load()), wasClicking (e.metronomeEnabled.load()),
      wasCountingIn (e.countInEnabled.load()), oldPosition (e.getPositionBeats())
{
    engine.stop();
    engine.beginOffline (settings.sampleRate, blockSize);
    engine.setBounceFocus (settings.focus);

    // A MIDI track's effects hold the sound back; render that much extra and drop it from the start.
    // (An audio track's clips are read earlier by the same amount, so it is on time already.)
    if (settings.focus.mode == BounceFocus::Mode::oneChain && settings.focus.track != nullptr
        && settings.focus.track->isMidi() && ! settings.focus.instrumentOnly)
        latencySamples = settings.focus.track->getEffectsLatency();

    engine.loopEnabled = false;   // the range plays straight through, and the tail after it
    engine.metronomeEnabled = false;
    engine.countInEnabled = false;
    engine.setPositionBeats (settings.startBeat);
    engine.play();
}

Renderer::~Renderer()
{
    engine.clearBounceFocus();
    engine.stop();
    engine.loopEnabled = wasLooping;
    engine.metronomeEnabled = wasClicking;
    engine.countInEnabled = wasCountingIn;
    engine.endOffline();
    engine.setPositionBeats (oldPosition);

    if (wasPlaying)
        engine.play();
}

Outcome Renderer::run (const Sink& sink, const std::function<bool (double)>& keepGoing)
{
    Outcome outcome;
    const double rate = settings.sampleRate;
    const auto bodySamples = (juce::int64) std::llround (engine.getTempoMap().getSecondsBetween (settings.startBeat, settings.endBeat) * rate);
    const auto tailSamples = (juce::int64) std::llround (juce::jmax (0.0, settings.tailSeconds) * rate);

    if (bodySamples <= 0)
    {
        outcome.result = juce::Result::fail ("There is nothing to bounce: the range is empty.");
        return outcome;
    }

    juce::AudioBuffer<float> block (2, blockSize);
    const auto total = latencySamples + bodySamples + tailSamples;
    juce::int64 rendered = 0, done = 0, skipLeft = latencySamples;

    // Hands on what is kept: everything after the first `latencySamples` rendered.
    auto emit = [&] (const juce::AudioBuffer<float>& buffer, int n)
    {
        const int from = (int) juce::jmin ((juce::int64) n, skipLeft);
        skipLeft -= from;

        if (n > from)
        {
            outcome.peak = juce::jmax (outcome.peak, buffer.getMagnitude (from, n - from));
            sink (buffer, from, n - from);
            done += n - from;
        }
    };

    // The range itself (and the time the effects hold the sound back).
    while (rendered < latencySamples + bodySamples)
    {
        const int n = (int) juce::jmin ((juce::int64) blockSize, latencySamples + bodySamples - rendered);
        engine.renderOffline (block.getArrayOfWritePointers(), 2, n);
        emit (block, n);
        rendered += n;

        if (keepGoing != nullptr && ! keepGoing ((double) rendered / (double) total))
        {
            outcome.cancelled = true;
            break;
        }
    }

    // The tail, kept up to where it has died away (below -80 dB).
    if (! outcome.cancelled && tailSamples > 0)
    {
        juce::AudioBuffer<float> tail (2, (int) tailSamples);

        for (int at = 0; at < (int) tailSamples; at += blockSize)
        {
            const int n = juce::jmin (blockSize, (int) tailSamples - at);
            engine.renderOffline (block.getArrayOfWritePointers(), 2, n);

            for (int ch = 0; ch < 2; ++ch)
                tail.copyFrom (ch, at, block, ch, 0, n);

            if (keepGoing != nullptr && ! keepGoing ((double) (rendered + at + n) / (double) total))
            {
                outcome.cancelled = true;
                break;
            }
        }

        if (! outcome.cancelled)
        {
            int last = (int) tailSamples;

            if (! settings.keepWholeTail)
            {
                while (last > 0 && std::abs (tail.getSample (0, last - 1)) < 1.0e-4f && std::abs (tail.getSample (1, last - 1)) < 1.0e-4f)
                    --last;

                // A little more, so the end of the sound isn't cut mid-wave.
                last = juce::jmin ((int) tailSamples, last + (last > 0 ? (int) (0.05 * rate) : 0));
            }

            emit (tail, last);
        }
    }

    if (outcome.cancelled)
    {
        outcome.result = juce::Result::fail ("Cancelled");
        return outcome;
    }

    outcome.seconds = (double) done / rate;
    return outcome;
}

Outcome Renderer::render (const juce::File& target, const std::function<bool (double)>& keepGoing)
{
    const auto temp = target.getSiblingFile ("." + target.getFileNameWithoutExtension() + ".bouncing.wav");
    temp.deleteFile();

    std::unique_ptr<juce::AudioFormatWriter> writer;

    if (std::unique_ptr<juce::OutputStream> stream (temp.createOutputStream()); stream != nullptr)
        writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                                     .withSampleRate (settings.sampleRate)
                                                                     .withNumChannels (2)
                                                                     .withBitsPerSample (settings.bitDepth));

    if (writer == nullptr)
    {
        Outcome failed;
        failed.result = juce::Result::fail ("Couldn't write " + target.getFullPathName() + ".");
        return failed;
    }

    auto outcome = run ([&] (const juce::AudioBuffer<float>& buffer, int start, int count)
                        { writer->writeFromAudioSampleBuffer (buffer, start, count); }, keepGoing);
    writer.reset();

    if (! outcome.result.wasOk())
    {
        temp.deleteFile();
        return outcome;
    }

    if (! temp.moveFileTo (target))
    {
        temp.deleteFile();
        outcome.result = juce::Result::fail ("Couldn't write " + target.getFullPathName() + ".");
    }

    return outcome;
}

Outcome Renderer::renderToBuffer (juce::AudioBuffer<float>& audio, const std::function<bool (double)>& keepGoing)
{
    audio.setSize (2, 0);

    return run ([&] (const juce::AudioBuffer<float>& buffer, int start, int count)
                {
                    const int old = audio.getNumSamples();
                    audio.setSize (2, old + count, true, true, false);

                    for (int ch = 0; ch < 2; ++ch)
                        audio.copyFrom (ch, old, buffer, ch, start, count);
                }, keepGoing);
}

Outcome bounceToFile (Engine& engine, const Settings& settings, const juce::File& target)
{
    Renderer renderer (engine, settings);
    return renderer.render (target);
}

//==============================================================================
InPlacePlan planInPlace (const Engine& engine, Track& track, const Clip* clip)
{
    InPlacePlan plan;
    plan.track = &track;
    plan.clip = clip;

    if (! track.isAudio() && ! track.isMidi())
    {
        plan.problem = "Only audio and MIDI tracks can be bounced in place.";
        return plan;
    }

    bool any = false;

    for (auto& c : track.clips)
    {
        if (c->muted || (clip != nullptr && c.get() != clip))
            continue;

        const double end = c->getEndBeat (engine.getTempoMap());
        plan.startBeat = any ? juce::jmin (plan.startBeat, c->startBeat) : c->startBeat;
        plan.endBeat = any ? juce::jmax (plan.endBeat, end) : end;
        any = true;
    }

    if (! any)
        plan.problem = clip != nullptr ? "That clip is muted." : "The track has no unmuted clip.";
    else if (plan.endBeat - plan.startBeat < 1.0e-6)
        plan.problem = "There is nothing to bounce: the clip is empty.";

    return plan;
}

Settings getInPlaceSettings (const InPlacePlan& plan)
{
    Settings settings;
    settings.startBeat = plan.startBeat;
    settings.endBeat = plan.endBeat;
    settings.sampleRate = 48000.0;
    settings.tailSeconds = inPlaceTailSeconds;
    settings.focus.mode = BounceFocus::Mode::oneChain;
    settings.focus.track = plan.track;
    settings.focus.clip = plan.clip;
    return settings;
}

InPlaceResult addBouncedTrack (Engine& engine, const InPlacePlan& plan, juce::AudioBuffer<float> audio)
{
    InPlaceResult out;
    const double rate = getInPlaceSettings (plan).sampleRate;

    // The track may have gone while a modal job ran.
    if (! plan.isOk() || engine.indexOf (plan.track) < 0 || (plan.clip != nullptr && engine.findTrackFor (plan.clip) != plan.track))
    {
        out.result = juce::Result::fail ("The track or clip is no longer in the song.");
        return out;
    }

    if (audio.getNumSamples() == 0)
    {
        out.result = juce::Result::fail ("There is nothing to bounce: the track makes no sound.");
        return out;
    }

    auto& original = *plan.track;

    // A result that would clip in a 24-bit file is turned down to -1 dBFS; the clip gain makes up for it.
    out.peak = audio.getMagnitude (0, audio.getNumSamples());

    if (out.peak > 1.0f)
    {
        const float neededDb = juce::Decibels::gainToDecibels (out.peak / inPlaceTargetPeak);
        out.gainDb = juce::jmin (neededDb, inPlaceMaxGainDb);
        audio.applyGain (juce::Decibels::decibelsToGain (-out.gainDb));

        out.note = "The bounce peaked at " + juce::String (juce::Decibels::gainToDecibels (out.peak), 1)
                   + " dB, so it was turned down and its clip gain raised by " + juce::String (out.gainDb, 1) + " dB.";

        if (neededDb > inPlaceMaxGainDb)
            out.note << " That is the most a clip allows, so the loudest peaks may still clip when saved.";
    }

    const double lengthBeats = engine.getTempoMap().getBeatAfter (plan.startBeat, audio.getNumSamples() / rate) - plan.startBeat;
    const int index = engine.indexOf (&original);
    const auto bouncedName = original.name + " (bounced)";
    Track* target = nullptr;

    // One clip: it goes on the track right below when that is this track's "(bounced)" track and the place is free.
    if (plan.clip != nullptr && index + 1 < (int) engine.getTracks().size())
    {
        auto* below = engine.getTracks()[(size_t) index + 1].get();

        if (below->isAudio() && below->name == bouncedName && below->bus == original.bus)
        {
            bool free = true;

            for (auto& c : below->clips)
                free = free && (c->startBeat >= plan.startBeat + lengthBeats || c->getEndBeat (engine.getTempoMap()) <= plan.startBeat);

            if (free)
                target = below;
        }
    }

    if (target == nullptr)
    {
        target = engine.addTrack (Track::Type::audio, index + 1);
        {
            const juce::ScopedLock sl (engine.getLock());
            target->name = bouncedName;
            target->colour = original.colour;
            target->volumeDb = original.volumeDb.load();
            target->pan = original.pan.load();
        }

        for (auto& send : original.sends)
        {
            engine.setSend (*target, *send->target, send->levelDb.load());

            if (auto* copy = routing::findSend (*target, *send->target))
                copy->preFader = send->preFader.load();
        }
    }

    out.track = target;
    out.clip = engine.addAudioClip (*target, std::move (audio), rate, plan.startBeat,
                                    plan.clip != nullptr && plan.clip->name.isNotEmpty() ? plan.clip->name : original.name);
    engine.setClipGain (*out.clip, out.gainDb);

    if (plan.clip != nullptr)
        engine.setClipMuted (*const_cast<Clip*> (plan.clip), true);
    else
        original.muted = true;

    engine.sendChangeMessage();
    return out;
}

InPlaceResult bounceInPlace (Engine& engine, Track& track, const Clip* clip)
{
    const auto plan = planInPlace (engine, track, clip);

    if (! plan.isOk())
    {
        InPlaceResult failed;
        failed.result = juce::Result::fail (plan.problem);
        return failed;
    }

    juce::AudioBuffer<float> audio;
    Outcome outcome;
    {
        Renderer renderer (engine, getInPlaceSettings (plan));
        outcome = renderer.renderToBuffer (audio);
    }

    if (! outcome.result.wasOk())
    {
        InPlaceResult failed;
        failed.result = outcome.result;
        return failed;
    }

    return addBouncedTrack (engine, plan, std::move (audio));
}

juce::Result writeClipToWav (const AudioClip& clip, const juce::File& target, int* bitsPerSample)
{
    const int numSamples = clip.audio.getNumSamples(), numChannels = clip.audio.getNumChannels();

    if (numSamples == 0 || numChannels == 0 || clip.sourceSampleRate <= 0.0)
        return juce::Result::fail ("The clip has no audio.");

    juce::AudioBuffer<float> audio (numChannels, numSamples);
    const float gain = clip.getGain();

    for (int ch = 0; ch < numChannels; ++ch)
        audio.copyFrom (ch, 0, clip.audio.getReadPointer (ch), numSamples, gain);

    const int bits = audio.getMagnitude (0, numSamples) > 1.0f ? 32 : 24;

    if (bitsPerSample != nullptr)
        *bitsPerSample = bits;

    target.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (target.createOutputStream());
    std::unique_ptr<juce::AudioFormatWriter> writer;

    if (stream != nullptr)
        writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                                     .withSampleRate (clip.sourceSampleRate)
                                                                     .withNumChannels (numChannels)
                                                                     .withBitsPerSample (bits));

    if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (audio, 0, numSamples))
        return juce::Result::fail ("Couldn't write " + target.getFullPathName() + ".");

    return juce::Result::ok();
}
} // namespace daw::bounce
