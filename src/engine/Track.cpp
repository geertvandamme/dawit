#include "Track.h"

namespace daw
{
Track::Track (Type t, juce::String n, juce::Colour c)
    : type (t), name (std::move (n)), colour (c)
{
}

Track::~Track()
{
    releaseResources();
}

bool Track::hasInstrument() const
{
    for (auto& slot : plugins)
        if (slot->isInstrument)
            return true;

    return false;
}

int Track::getRequiredChannels() const
{
    int channels = 2;

    for (auto& slot : plugins)
        channels = juce::jmax (channels, slot->instance->getTotalNumInputChannels(), slot->instance->getTotalNumOutputChannels());

    return channels;
}

void Track::fitPluginChannels()
{
    if (getRequiredChannels() > scratch.getNumChannels())
        scratch.setSize (getRequiredChannels(), scratch.getNumSamples(), false, true, false);
}

void Track::prepare (double sampleRate, int maxBlockSize)
{
    scratch.setSize (getRequiredChannels(), maxBlockSize, false, true, false);

    if (hasInput())
        busInput.setSize (2, maxBlockSize, false, true, false);

    midi.ensureSize (4096);
    builtInSynth.setCurrentPlaybackSampleRate (sampleRate);
    gainLeft.reset (sampleRate, 0.02);
    gainRight.reset (sampleRate, 0.02);
    sendFaderGain.reset (sampleRate, 0.02);

    for (auto& send : sends)
        send->gain.reset (sampleRate, 0.02);

    activeNotes.reset();

    for (auto& slot : plugins)
    {
        slot->instance->setRateAndBufferSizeDetails (sampleRate, maxBlockSize);
        slot->instance->prepareToPlay (sampleRate, maxBlockSize);
    }
}

void Track::releaseResources()
{
    for (auto& slot : plugins)
        slot->instance->releaseResources();
}

void Track::clearBusInput (int numSamples)
{
    if (busInput.getNumChannels() >= 2 && numSamples > 0 && numSamples <= busInput.getNumSamples())
    {
        busInput.clear (0, 0, numSamples);
        busInput.clear (1, 0, numSamples);
    }
}

void Track::process (const RenderContext& ctx, juce::AudioBuffer<float>& destination, int destinationOffset)
{
    const int n = ctx.numSamples;

    if (n <= 0 || n > scratch.getNumSamples() || (hasInput() && n > busInput.getNumSamples()))
        return;

    scratch.clear (0, 0, n);
    scratch.clear (1, 0, n);
    midi.clear();

    if (hasInput())
    {
        // A bus or a return hears what its tracks sent into it.
        scratch.copyFrom (0, 0, busInput, 0, 0, n);
        scratch.copyFrom (1, 0, busInput, 1, 0, n);
        const float sum = juce::jmax (scratch.getMagnitude (0, 0, n), scratch.getMagnitude (1, 0, n));
        auto old = busInputPeak.load();
        while (sum > old && ! busInputPeak.compare_exchange_weak (old, sum)) {}
        updateMeterLevel (busInputLevel, sum, n, ctx.sampleRate);
    }
    else
    {
        if (ctx.flushNotes)
            releaseActiveNotes();

        if (ctx.playing)
        {
            if (isMidi())
            {
                collectMidiClipEvents (ctx);
            }
            else
            {
                // Plugins that hold the sound back: read the clips that much earlier. After a start, seek or loop
                // wrap the plugins first get the samples that were skipped, so what they hold back is what comes next.
                const int compensation = ctx.instrumentOnly ? 0 : getCompensation();

                if (compensation > 0 && (ctx.restarted || compensation != lastCompensation))
                    preroll (ctx, compensation);

                lastCompensation = compensation;
                renderAudioClips (ctx, compensation, n);
            }
        }
    }

    // Monitor: the input of the device, through this track's effects and fader like its clips.
    if (isAudio() && ctx.inputs != nullptr && armed.load() && monitor.load() && ! ctx.muteMonitoring)
    {
        const auto [left, right] = resolveInputChannels (inputChannel.load(), inputStereo.load(), ctx.numInputs);

        if (left >= 0 && ctx.inputs[left] != nullptr && ctx.inputs[right] != nullptr)
        {
            juce::FloatVectorOperations::add (scratch.getWritePointer (0), ctx.inputs[left] + ctx.liveMidiOffset, n);
            juce::FloatVectorOperations::add (scratch.getWritePointer (1), ctx.inputs[right] + ctx.liveMidiOffset, n);
        }
    }

    if (ctx.liveMidi != nullptr && ! hasInput())
    {
        if (scaleFilter.load())
        {
            // Keys outside the song key don't sound. Note-offs always pass, so a note held while
            // the key changes still stops.
            for (const auto metadata : *ctx.liveMidi)
            {
                const int position = metadata.samplePosition - ctx.liveMidiOffset;

                if (position < 0 || position >= n)
                    continue;

                const auto m = metadata.getMessage();

                if (m.isNoteOn() && (ctx.keyPitchClasses & (1 << (m.getNoteNumber() % 12))) == 0)
                    continue;

                midi.addEvent (m, position);
            }
        }
        else
        {
            midi.addEvents (*ctx.liveMidi, ctx.liveMidiOffset, n, -ctx.liveMidiOffset);
        }
    }

    if (ctx.previewMidi != nullptr && ! hasInput())
        midi.addEvents (*ctx.previewMidi, ctx.liveMidiOffset, n, -ctx.liveMidiOffset);

    auto stereoPeak = [this, n] { return juce::jmax (scratch.getMagnitude (0, 0, n), scratch.getMagnitude (1, 0, n)); };
    auto raise = [] (std::atomic<float>& peak, float level) { if (level > peak.load()) peak = level; };

    if (usesBuiltInSynth())
    {
        juce::AudioBuffer<float> stereo (scratch.getArrayOfWritePointers(), 2, n);
        builtInSynth.renderNextBlock (stereo, midi, 0, n);
        raise (builtInSynthPeak, stereoPeak());
    }

    updateMeterLevel (chainInputLevel, stereoPeak(), n, ctx.sampleRate);

    runPlugins (n, ctx.sampleRate, true, &ctx);

    raise (faderInputPeak, stereoPeak());

    // Sends: copies of the sound after the effects, added into the returns' inputs. Post-fader ones follow the
    // fader (but not the pan, so DawIt Reverb, which listens to left plus right, gets a hard-panned track whole).
    if (! sends.empty() && ! ctx.unityOutput)
    {
        const bool silent = muted.load() || mutedAbove;
        const float faderGain = juce::Decibels::decibelsToGain (volumeDb.load(), -60.0f);
        if (snapGains)
            sendFaderGain.setCurrentAndTargetValue (faderGain);
        else
            sendFaderGain.setTargetValue (faderGain);

        const float* srcL = scratch.getReadPointer (0);
        const float* srcR = scratch.getReadPointer (1);
        float peaks[16] = {};
        const size_t numSends = juce::jmin (sends.size(), (size_t) 16);

        for (size_t s = 0; s < numSends; ++s)
        {
            auto& send = *sends[s];
            const bool heard = ! silent && (! ctx.anySolo || ctx.onSoloPath || send.target->countsAsSoloed);
            const float sendTarget = heard && send.isOn() ? juce::Decibels::decibelsToGain (send.levelDb.load(), sendOffDb) : 0.0f;

            if (snapGains)
                send.gain.setCurrentAndTargetValue (sendTarget);
            else
                send.gain.setTargetValue (sendTarget);
        }

        for (int i = 0; i < n; ++i)
        {
            const float fader = sendFaderGain.getNextValue();

            for (size_t s = 0; s < numSends; ++s)
            {
                auto& send = *sends[s];
                auto& input = send.target->busInput;
                const float g = send.gain.getNextValue() * (send.preFader.load() ? 1.0f : fader);

                if (input.getNumChannels() < 2 || input.getNumSamples() < n)
                    continue;

                const float l = srcL[i] * g, r = srcR[i] * g;
                input.getWritePointer (0)[i] += l;
                input.getWritePointer (1)[i] += r;
                peaks[s] = juce::jmax (peaks[s], std::abs (l), std::abs (r));
            }
        }

        for (size_t s = 0; s < numSends; ++s)
        {
            raise (sends[s]->peak, peaks[s]);
            updateMeterLevel (sends[s]->level, peaks[s], n, ctx.sampleRate);
        }
    }

    // Volume, constant-power balance, mute/solo.
    const bool audible = ctx.unityOutput || (! muted.load() && (! ctx.anySolo || ctx.onSoloPath));
    const float gain = ctx.unityOutput ? 1.0f : (audible ? juce::Decibels::decibelsToGain (volumeDb.load(), -60.0f) : 0.0f);
    const float p = ctx.unityOutput ? 0.0f : pan.load();
    const float halfPi = juce::MathConstants<float>::halfPi;
    const float targetLeft = gain * (p > 0.0f ? std::cos (p * halfPi) : 1.0f);
    const float targetRight = gain * (p < 0.0f ? std::cos (-p * halfPi) : 1.0f);

    if (snapGains || ctx.unityOutput)
    {
        gainLeft.setCurrentAndTargetValue (targetLeft);
        gainRight.setCurrentAndTargetValue (targetRight);
    }
    else
    {
        gainLeft.setTargetValue (targetLeft);
        gainRight.setTargetValue (targetRight);
    }

    snapGains = false;

    const float* inL = scratch.getReadPointer (0);
    const float* inR = scratch.getReadPointer (1);
    float* outL = destination.getWritePointer (0, destinationOffset);
    float* outR = destination.getWritePointer (1, destinationOffset);
    float peakL = 0.0f, peakR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float l = inL[i] * gainLeft.getNextValue();
        const float r = inR[i] * gainRight.getNextValue();
        outL[i] += l;
        outR[i] += r;
        peakL = juce::jmax (peakL, std::abs (l));
        peakR = juce::jmax (peakR, std::abs (r));
    }

    raise (faderOutputPeak, juce::jmax (peakL, peakR));
    updateMeterLevel (peakLeft, peakL, n, ctx.sampleRate);
    updateMeterLevel (peakRight, peakR, n, ctx.sampleRate);
}

int Track::getCompensation() const
{
    return isAudio() ? getEffectsLatency() : 0;
}

int Track::getEffectsLatency() const
{
    int total = 0;

    for (auto& slot : plugins)
        if (! slot->isInstrument && slot->instance != nullptr && (! slot->bypassed.load() || slot->keepsLatencyWhenBypassed))
            total += juce::jmax (0, slot->instance->getLatencySamples());

    return juce::jmin (total, 1 << 18);
}

void Track::preroll (const RenderContext& ctx, int numSamples)
{
    // The chain's output is thrown away: only what the plugins hold afterwards matters.
    for (int done = 0; done < numSamples;)
    {
        const int chunk = juce::jmin (numSamples - done, scratch.getNumSamples());

        for (int ch = 0; ch < scratch.getNumChannels(); ++ch)
            scratch.clear (ch, 0, chunk);

        midi.clear();
        renderAudioClips (ctx, done, chunk);
        runPlugins (chunk, ctx.sampleRate, false, &ctx);
        done += chunk;
    }

    scratch.clear (0, 0, ctx.numSamples);
    scratch.clear (1, 0, ctx.numSamples);
    midi.clear();
}

void Track::runPlugins (int n, double sampleRate, bool metering, const RenderContext* ctx)
{
    auto stereoPeak = [this, n] { return juce::jmax (scratch.getMagnitude (0, 0, n), scratch.getMagnitude (1, 0, n)); };
    auto raise = [] (std::atomic<float>& peak, float level) { if (level > peak.load()) peak = level; };

    auto stereoEnergy = [this, n]
    {
        float sum = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (auto* d = scratch.getReadPointer (ch), * end = d + n; d != end; ++d)
                sum += *d * *d;

        return sum;
    };

    // The UI swaps these for 0 when it reads them, so adding must not lose that.
    auto add = [] (std::atomic<float>& sum, float value)
    {
        auto old = sum.load();
        while (! sum.compare_exchange_weak (old, old + value)) {}
    };

    for (auto& slot : plugins)
    {
        if (ctx != nullptr && ctx->instrumentOnly && ! slot->isInstrument)
            continue;

        const float inputLevel = metering ? stereoPeak() : 0.0f;
        const float inputEnergy = metering && ! slot->isInstrument ? stereoEnergy() : 0.0f;

        if (metering)
        {
            raise (slot->inputPeak, inputLevel);
            add (slot->inputEnergy, inputEnergy);
        }

        if (ctx != nullptr && slot->songContext != nullptr)
            slot->songContext->setSongContext (ctx->keyPitchClasses, ctx->chordPitchClasses);

        auto& plugin = *slot->instance;
        const int numIns = plugin.getTotalNumInputChannels();
        const int numOuts = plugin.getTotalNumOutputChannels();
        const int numChannels = juce::jmax (2, numIns, numOuts);

        // A plugin must never get fewer channels than it writes to; skip it rather than crash.
        // A bypassed plugin that holds the sound back still does, so bypassing it doesn't move the track in time.
        const bool bypassed = slot->bypassed.load();

        if (numChannels > scratch.getNumChannels() || (bypassed && ! slot->keepsLatencyWhenBypassed))
        {
            if (metering)
            {
                raise (slot->outputPeak, inputLevel);
                add (slot->outputEnergy, inputEnergy);
                updateMeterLevel (slot->outputLevel, inputLevel, n, sampleRate);
            }

            continue;
        }

        // Anything beyond our stereo signal (side-chains, extra outputs) starts silent.
        for (int ch = 2; ch < numChannels; ++ch)
            scratch.clear (ch, 0, n);

        juce::AudioBuffer<float> view (scratch.getArrayOfWritePointers(), numChannels, n);

        if (bypassed)
            plugin.processBlockBypassed (view, midi);
        else
            plugin.processBlock (view, midi);

        if (numOuts == 1)
            scratch.copyFrom (1, 0, scratch, 0, 0, n);

        if (! metering)
            continue;

        const float outputLevel = stereoPeak();
        raise (slot->outputPeak, outputLevel);
        updateMeterLevel (slot->outputLevel, outputLevel, n, sampleRate);

        if (! slot->isInstrument)
            add (slot->outputEnergy, stereoEnergy());
    }
}

void Track::renderAudioClips (const RenderContext& ctx, int readAhead, int n)
{
    const double samplesPerBeat = ctx.sampleRate * 60.0 / ctx.bpm;

    for (auto& c : clips)
    {
        if (ctx.onlyClip != nullptr && c.get() != ctx.onlyClip)
            continue;

        auto& clip = static_cast<AudioClip&> (*c);
        const int srcLength = clip.audio.getNumSamples();

        if (srcLength == 0 || clip.muted)
            continue;

        // Read with linear interpolation so files at any sample rate play at the right speed.
        const double ratio = clip.sourceSampleRate / ctx.sampleRate;
        // In output samples: the seconds since the clip started, through any tempo changes.
        // A track with plugins that hold the sound back reads `readAhead` samples later, so the sound comes out on time.
        const double clipPos = (ctx.tempoMap != nullptr ? ctx.tempoMap->getSecondsBetween (clip.startBeat, ctx.startBeat) * ctx.sampleRate
                                                        : (ctx.startBeat - clip.startBeat) * samplesPerBeat) + readAhead;
        const double clipLength = srcLength / ratio;

        if (clipPos >= clipLength || clipPos + n <= 0.0)
            continue;

        const int first = juce::jlimit (0, n, (int) std::ceil (-clipPos));
        const int last = juce::jlimit (0, n, (int) std::ceil (clipLength - clipPos));
        const int numSrcChannels = clip.audio.getNumChannels();
        const float gain = clip.getGain();

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* src = clip.audio.getReadPointer (juce::jmin (ch, numSrcChannels - 1));
            float* dst = scratch.getWritePointer (ch);

            for (int i = first; i < last; ++i)
            {
                const double srcPos = (clipPos + i) * ratio;
                const int index = (int) srcPos;

                if (index < 0 || index >= srcLength)
                    continue;

                const float frac = (float) (srcPos - index);
                const float s0 = src[index];
                const float s1 = index + 1 < srcLength ? src[index + 1] : 0.0f;
                dst[i] += (s0 + frac * (s1 - s0)) * gain;
            }
        }
    }
}

void Track::collectMidiClipEvents (const RenderContext& ctx)
{
    const double samplesPerBeat = ctx.sampleRate * 60.0 / ctx.bpm;
    const double b0 = ctx.startBeat;
    const double b1 = b0 + ctx.numSamples / samplesPerBeat;
    const int lastSample = ctx.numSamples - 1;

    auto toSample = [&] (double beat) { return juce::jlimit (0, lastSample, (int) ((beat - b0) * samplesPerBeat)); };

    // Note-offs go in first, so a pitch that is re-triggered at the same spot is released before it restarts.
    for (int pass = 0; pass < 2; ++pass)
    {
        for (auto& c : clips)
        {
            if (ctx.onlyClip != nullptr && c.get() != ctx.onlyClip)
                continue;

            auto& clip = static_cast<MidiClip&> (*c);
            const double clipEnd = clip.startBeat + clip.lengthBeats;

            if (clip.muted || clip.startBeat >= b1 || clipEnd < b0)
                continue;

            for (auto& note : clip.notes)
            {
                if (note.start >= clip.lengthBeats || note.pitch < 0 || note.pitch > 127)
                    continue;

                const double on = clip.startBeat + note.start;
                const double off = juce::jmin (on + note.length, clipEnd);

                if (pass == 0)
                {
                    if (off >= b0 && off < b1 && activeNotes[(size_t) note.pitch])
                    {
                        midi.addEvent (juce::MidiMessage::noteOff (1, note.pitch), toSample (off));
                        activeNotes.reset ((size_t) note.pitch);
                    }
                }
                else if (on >= b0 && on < b1)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, note.pitch, note.velocity), toSample (on));
                    activeNotes.set ((size_t) note.pitch);

                    if (off < b1) // very short note: starts and ends inside this slice
                    {
                        midi.addEvent (juce::MidiMessage::noteOff (1, note.pitch), toSample (off));
                        activeNotes.reset ((size_t) note.pitch);
                    }
                }
            }
        }
    }
}

void Track::releaseActiveNotes()
{
    for (int pitch = 0; pitch < 128; ++pitch)
        if (activeNotes[(size_t) pitch])
            midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);

    activeNotes.reset();
    midi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
}
} // namespace daw
