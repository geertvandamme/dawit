#include "BuiltInHarmoniser.h"
#include "BuiltInFormat.h"
#include <cmath>

namespace daw
{
using harm::Knob;
using harm::VoiceKnob;

const char* BuiltInHarmoniser::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::harmony: return "harmony";
        case Knob::key:     return "key";
        case Knob::scale:   return "scale";
        case Knob::mix:     return "mix";
    }

    return "";
}

juce::String BuiltInHarmoniser::getParameterId (int voice, VoiceKnob k)
{
    const juce::String prefix = "v" + juce::String (voice + 1);

    switch (k)
    {
        case VoiceKnob::on:       return prefix + "On";
        case VoiceKnob::interval: return prefix + "Interval";
        case VoiceKnob::steps:    return prefix + "Steps";
        case VoiceKnob::level:    return prefix + "Level";
        case VoiceKnob::pan:      return prefix + "Pan";
        case VoiceKnob::detune:   return prefix + "Detune";
        case VoiceKnob::delay:    return prefix + "Delay";
    }

    return {};
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInHarmoniser::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const harm::Settings defaults;
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::harmony), 1 }, "Harmony", juce::StringArray { "Fixed", "Key" }, 1));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::key), 1 }, "Key",
                                                              juce::StringArray { "Song", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::scale), 1 }, "Scale", juce::StringArray { "Major", "Minor" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (Knob::mix), 1 }, "Mix", juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, defaults.mixPercent));

    for (int v = 0; v < harm::numVoices; ++v)
    {
        const auto& d = defaults.voices[(size_t) v];
        const juce::String name = "Voice " + juce::String (v + 1) + " ";
        auto add = [&] (VoiceKnob k, const char* what, juce::NormalisableRange<float> range, float value)
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (v, k), 1 }, name + what, range, value));
        };

        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { getParameterId (v, VoiceKnob::on), 1 }, name + "on", d.on));
        add (VoiceKnob::interval, "interval (semitones)", { -12.0f, 12.0f, 1.0f }, (float) d.semitones);
        add (VoiceKnob::steps, "interval (steps)", { -7.0f, 7.0f, 1.0f }, (float) d.steps);
        add (VoiceKnob::level, "level", { harm::minLevelDb, harm::maxLevelDb, 0.1f }, d.levelDb);
        add (VoiceKnob::pan, "pan", { -100.0f, 100.0f, 1.0f }, d.panPercent);
        add (VoiceKnob::detune, "detune", { -50.0f, 50.0f, 1.0f }, d.detuneCents);
        add (VoiceKnob::delay, "delay", { 0.0f, 60.0f, 1.0f }, d.delayMs);
    }

    return layout;
}

BuiltInHarmoniser::BuiltInHarmoniser()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo()).withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItHarmoniser", createLayout())
{
    for (int i = 0; i < harm::numKnobs; ++i)
        values[(size_t) i] = parameters.getRawParameterValue (getParameterId ((Knob) i));

    for (int v = 0; v < harm::numVoices; ++v)
    {
        for (int k = 0; k < harm::numVoiceKnobs; ++k)
            voiceValues[(size_t) v][(size_t) k] = parameters.getRawParameterValue (getParameterId (v, (VoiceKnob) k));

        views[(size_t) v].controls = &controls;
        views[(size_t) v].voice = v;
    }

    recent.reserve (4096);
    prepareToPlay (44100.0, 512);
    startDraining();
}

BuiltInHarmoniser::~BuiltInHarmoniser()
{
    stopTimer();
}

juce::PluginDescription BuiltInHarmoniser::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Effect";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44486d31;   // "DHm1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInHarmoniser::fillInPluginDescription (juce::PluginDescription& d) const { d = getDescription(); }

juce::RangedAudioParameter* BuiltInHarmoniser::getKnobParameter (Knob k) const { return parameters.getParameter (getParameterId (k)); }

juce::RangedAudioParameter* BuiltInHarmoniser::getKnobParameter (int voice, VoiceKnob k) const { return parameters.getParameter (getParameterId (voice, k)); }

harm::Settings BuiltInHarmoniser::getSettings() const
{
    harm::Settings s;
    s.mode = values[(size_t) Knob::harmony]->load() >= 0.5f ? harm::Mode::key : harm::Mode::fixed;
    s.key = juce::jlimit (0, 12, juce::roundToInt (values[(size_t) Knob::key]->load()));
    s.scale = values[(size_t) Knob::scale]->load() >= 0.5f ? harm::Scale::minor : harm::Scale::major;
    s.mixPercent = values[(size_t) Knob::mix]->load();

    for (int v = 0; v < harm::numVoices; ++v)
    {
        auto load = [this, v] (VoiceKnob k) { return voiceValues[(size_t) v][(size_t) k]->load(); };
        auto& voice = s.voices[(size_t) v];
        voice.on = load (VoiceKnob::on) >= 0.5f;
        voice.semitones = juce::roundToInt (load (VoiceKnob::interval));
        voice.steps = juce::roundToInt (load (VoiceKnob::steps));
        voice.levelDb = load (VoiceKnob::level);
        voice.panPercent = load (VoiceKnob::pan);
        voice.detuneCents = load (VoiceKnob::detune);
        voice.delayMs = load (VoiceKnob::delay);
    }

    return s;
}

void BuiltInHarmoniser::setSettings (const harm::Settings& s)
{
    auto set = [] (juce::RangedAudioParameter* p, float v) { if (p != nullptr) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set (getKnobParameter (Knob::harmony), s.mode == harm::Mode::key ? 1.0f : 0.0f);
    set (getKnobParameter (Knob::key), (float) s.key);
    set (getKnobParameter (Knob::scale), s.scale == harm::Scale::minor ? 1.0f : 0.0f);
    set (getKnobParameter (Knob::mix), s.mixPercent);

    for (int v = 0; v < harm::numVoices; ++v)
    {
        const auto& voice = s.voices[(size_t) v];
        set (getKnobParameter (v, VoiceKnob::on), voice.on ? 1.0f : 0.0f);
        set (getKnobParameter (v, VoiceKnob::interval), (float) voice.semitones);
        set (getKnobParameter (v, VoiceKnob::steps), (float) voice.steps);
        set (getKnobParameter (v, VoiceKnob::level), voice.levelDb);
        set (getKnobParameter (v, VoiceKnob::pan), voice.panPercent);
        set (getKnobParameter (v, VoiceKnob::detune), voice.detuneCents);
        set (getKnobParameter (v, VoiceKnob::delay), voice.delayMs);
    }
}

void BuiltInHarmoniser::applyPreset (const harm::Preset& p) { setSettings (p.settings); }

int BuiltInHarmoniser::getEffectiveKeyMask() const
{
    const auto s = getSettings();
    return harm::getKeyMask (s.key, s.scale, keyMask.load());
}

bool BuiltInHarmoniser::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
}

//==============================================================================
void BuiltInHarmoniser::Controls::add (const Entry& e) noexcept
{
    if (size == capacity)
    {
        std::copy (entries.begin() + 1, entries.end(), entries.begin());
        --size;
    }

    entries[(size_t) size++] = e;
}

int BuiltInHarmoniser::Controls::find (juce::int64 sample) const noexcept
{
    for (int i = size - 1; i >= 0; --i)
        if (entries[(size_t) i].centre <= sample)
            return i;

    return -1;
}

float BuiltInHarmoniser::Controls::getPeriod (juce::int64 sample) const
{
    if (size == 0)
        return 0.0f;

    const int k = find (sample);

    if (k < 0)
        return entries[0].period;

    const auto& a = entries[(size_t) k];

    if (k + 1 >= size)
        return a.period;

    const auto& b = entries[(size_t) k + 1];

    if (a.period <= 0.0f || b.period <= 0.0f)
        return 0.0f;

    const float t = (float) (sample - a.centre) / (float) juce::jmax ((juce::int64) 1, b.centre - a.centre);
    return a.period + t * (b.period - a.period);
}

float BuiltInHarmoniser::Controls::getRatio (juce::int64 sample, int voice) const
{
    if (size == 0)
        return 1.0f;

    const int k = find (sample);

    if (k < 0)
        return entries[0].ratio[(size_t) voice];

    const auto& a = entries[(size_t) k];

    if (k + 1 >= size)
        return a.ratio[(size_t) voice];

    const auto& b = entries[(size_t) k + 1];

    if (a.period <= 0.0f || b.period <= 0.0f)
        return 1.0f;

    const float t = (float) (sample - a.centre) / (float) juce::jmax ((juce::int64) 1, b.centre - a.centre);
    return a.ratio[(size_t) voice] + t * (b.ratio[(size_t) voice] - a.ratio[(size_t) voice]);
}

//==============================================================================
void BuiltInHarmoniser::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    window = pitch::getWindowSize (rate);
    hop = window / 8;
    latency = getLatencyFor (rate);
    setLatencySamples (latency);
    tracker.prepare (rate, window);
    windowData.assign ((size_t) window, 0.0f);
    mid.assign ((size_t) chunkSize, 0.0f);
    scratch.assign ((size_t) chunkSize, 0.0f);

    int leadSize = 1024;

    while (leadSize <= latency + chunkSize)
        leadSize *= 2;

    int delaySize = 1024;

    while (delaySize <= (int) (0.06 * rate) + 2)
        delaySize *= 2;

    leadMask = leadSize - 1;
    delayMask = delaySize - 1;

    for (auto& ring : leadRing)
        ring.assign ((size_t) leadSize, 0.0f);

    for (int v = 0; v < harm::numVoices; ++v)
    {
        shifters[(size_t) v].prepare (rate, latency);
        voiceOut[(size_t) v].assign ((size_t) chunkSize, 0.0f);
        delayRing[(size_t) v].assign ((size_t) delaySize, 0.0f);
        gainLeft[(size_t) v].reset (rate, 0.010);
        gainRight[(size_t) v].reset (rate, 0.010);
    }

    clap.prepare (rate);
    resetState();
    startDraining();
}

void BuiltInHarmoniser::resetState()
{
    leadCount = 0;
    controls.clear();
    gate.fill (0.0f);
    gainsStarted = false;
    reference = -1;
    liveLead = -1.0f;
    liveVoice[0] = liveVoice[1] = -1.0f;
}

void BuiltInHarmoniser::startDraining()
{
    // The timer needs the message thread; a processor made anywhere else starts it when it is prepared.
    if (auto* manager = juce::MessageManager::getInstanceWithoutCreating(); manager != nullptr && manager->isThisTheMessageThread() && ! isTimerRunning())
        startTimerHz (30);
}

void BuiltInHarmoniser::writeLead (const juce::AudioBuffer<float>& buffer, int offset, int n, int channels)
{
    const float* in[2] { buffer.getReadPointer (0) + offset, buffer.getReadPointer (juce::jmin (1, channels - 1)) + offset };

    for (int i = 0; i < n; ++i)
    {
        const auto at = (size_t) ((leadCount + i) & leadMask);
        leadRing[0][at] = in[0][i];
        leadRing[1][at] = in[1][i];
        mid[(size_t) i] = (in[0][i] + in[1][i]) * 0.5f;
    }
}

void BuiltInHarmoniser::analyse (int n, int offset, juce::int64 t0, const harm::Settings& s, const std::array<bool, harm::numVoices>& active, double ppq, double bpm, bool playing)
{
    const int mask = harm::getKeyMask (s.key, s.scale, keyMask.load());
    const double samplesPerBeat = rate * 60.0 / juce::jmax (1.0, bpm);
    const juce::int64 end = t0 + n;
    const float maxPeriod = (float) shifters[0].getMaxPeriod();

    for (juce::int64 b = (t0 / hop + 1) * hop; b <= end; b += hop)
    {
        if (b < window)
            continue;

        shifters[0].copyMid (b, windowData.data(), window);
        const auto reading = tracker.analyse (windowData.data(), window, pitch::Tracker::minClarity, true);

        Controls::Entry entry;
        entry.centre = b - window / 2;

        harm::Frame frame;
        frame.rmsDb = reading.rmsDb;
        frame.beat = playing ? (float) (ppq + ((double) (offset + (b - t0)) + (double) (latency - window / 2)) / samplesPerBeat) : -1.0f;

        if (reading.voiced)
        {
            entry.period = (float) (rate / tune::getHz (reading.midi));
            frame.lead = reading.midi;
            reference = harm::getReference (reading.midi, mask, reference);

            for (int v = 0; v < harm::numVoices; ++v)
            {
                if (! active[(size_t) v])
                    continue;

                const auto& voice = s.voices[(size_t) v];
                const double shift = harm::getShift (voice, s.mode, reference, mask);
                entry.ratio[(size_t) v] = (float) harm::getRatio (shift, voice.detuneCents);

                if (entry.period <= maxPeriod)
                    frame.voice[(size_t) v] = reading.midi + (float) shift;
            }
        }
        else
            reference = -1;

        controls.add (entry);
        liveLead = frame.lead;
        liveVoice[0] = frame.voice[0];
        liveVoice[1] = frame.voice[1];

        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            fifoFrames[(size_t) start1] = frame;
            fifo.finishedWrite (1);
        }
    }
}

void BuiltInHarmoniser::processChunk (juce::AudioBuffer<float>& buffer, int offset, int n, const harm::Settings& s, double ppq, double bpm, bool playing)
{
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const auto [dry, wet] = harm::getMixGains (s.mixPercent);
    std::array<bool, harm::numVoices> active {};

    for (int v = 0; v < harm::numVoices; ++v)
    {
        const auto& voice = s.voices[(size_t) v];
        active[(size_t) v] = voice.on && harm::getLevelGain (voice.levelDb) > 0.0f && wet > 0.0f;
    }

    writeLead (buffer, offset, n, channels);

    // A voice that isn't heard still takes the input, so it is in time when it comes on, but makes no output.
    const float* in[1] { mid.data() };
    float* discard[1] { scratch.data() };

    for (int v = 0; v < harm::numVoices; ++v)
    {
        if (active[(size_t) v])
            shifters[(size_t) v].push (in, 1, n);
        else
            shifters[(size_t) v].bypass (in, discard, 1, n);
    }

    analyse (n, offset, shifters[0].getInputCount() - n, s, active, ppq, bpm, playing);

    const float gateStep = (float) (1.0 / (0.010 * rate));
    const int maxPeriod = shifters[0].getMaxPeriod();
    int delaySamples[harm::numVoices] {};

    for (int v = 0; v < harm::numVoices; ++v)
    {
        const auto& voice = s.voices[(size_t) v];
        auto& left = gainLeft[(size_t) v];
        auto& right = gainRight[(size_t) v];
        auto& ring = delayRing[(size_t) v];
        const int d = juce::jlimit (0, delayMask, juce::roundToInt (voice.delayMs * 0.001 * rate));
        delaySamples[v] = d;

        if (! active[(size_t) v])
        {
            std::fill (voiceOut[(size_t) v].begin(), voiceOut[(size_t) v].begin() + n, 0.0f);
            gate[(size_t) v] = 0.0f;
            left.setCurrentAndTargetValue (0.0f);
            right.setCurrentAndTargetValue (0.0f);
        }
        else
        {
            float* out[1] { voiceOut[(size_t) v].data() };
            shifters[(size_t) v].pull (out, 1, n, views[(size_t) v]);

            // Where there is no clear pitch (or the note is below the shifter's reach) the voice fades out in 10 ms.
            const juce::int64 base = shifters[(size_t) v].getInputCount() - n - latency;
            float target = gate[(size_t) v], g = gate[(size_t) v];

            for (int i = 0; i < n; ++i)
            {
                if ((i & 7) == 0)
                {
                    const float p = controls.getPeriod (base + i);
                    target = p > 0.0f && p <= (float) maxPeriod ? 1.0f : 0.0f;
                }

                g = target > g ? juce::jmin (target, g + gateStep) : juce::jmax (target, g - gateStep);
                out[0][i] *= g;
            }

            gate[(size_t) v] = g;
            const auto [panLeft, panRight] = harm::getPanGains (voice.panPercent);
            const float level = harm::getLevelGain (voice.levelDb) * wet;

            if (! gainsStarted)
            {
                left.setCurrentAndTargetValue (level * panLeft);
                right.setCurrentAndTargetValue (level * panRight);
            }
            else
            {
                left.setTargetValue (level * panLeft);
                right.setTargetValue (level * panRight);
            }
        }

        // The delay line always takes the voice (silence when it's off), so old sound never comes back.
        for (int i = 0; i < n; ++i)
            ring[(size_t) ((leadCount + i) & delayMask)] = voiceOut[(size_t) v][(size_t) i];
    }

    gainsStarted = true;

    // The lead, held back by the latency, and the voices on top.
    float* write[2] { buffer.getWritePointer (0) + offset, buffer.getWritePointer (juce::jmin (1, channels - 1)) + offset };

    for (int i = 0; i < n; ++i)
    {
        const juce::int64 now = leadCount + i;
        float sum[2] { 0.0f, 0.0f };

        for (int v = 0; v < harm::numVoices; ++v)
        {
            if (! active[(size_t) v])
                continue;

            const float x = delayRing[(size_t) v][(size_t) ((now - delaySamples[v]) & delayMask)];
            const float gl = gainLeft[(size_t) v].getNextValue(), gr = gainRight[(size_t) v].getNextValue();

            if (channels == 1)
                sum[0] += x * std::sqrt (gl * gl + gr * gr);
            else
            {
                sum[0] += x * gl;
                sum[1] += x * gr;
            }
        }

        for (int ch = 0; ch < channels; ++ch)
            write[ch][i] = leadRing[(size_t) ch][(size_t) ((now - latency) & leadMask)] * dry + sum[ch];
    }

    leadCount += n;
}

void BuiltInHarmoniser::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples(), channels = juce::jmin (2, buffer.getNumChannels());

    if (channels < 1 || n <= 0)
        return;

    clap.addTo (buffer, channels);
    const auto s = getSettings();
    double ppq = 0.0, bpm = 120.0;
    bool playing = false;

    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
        {
            playing = position->getIsPlaying() && position->getPpqPosition().hasValue();
            ppq = position->getPpqPosition().orFallback (0.0);
            bpm = position->getBpm().orFallback (120.0);
        }

    for (int offset = 0; offset < n; offset += chunkSize)
        processChunk (buffer, offset, juce::jmin (chunkSize, n - offset), s, ppq, bpm, playing);
}

void BuiltInHarmoniser::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int n = buffer.getNumSamples(), channels = juce::jmin (2, buffer.getNumChannels());

    if (channels < 1 || n <= 0)
        return;

    const float* in[1] { mid.data() };
    float* discard[1] { scratch.data() };

    for (int offset = 0; offset < n; offset += chunkSize)
    {
        const int count = juce::jmin (chunkSize, n - offset);
        writeLead (buffer, offset, count, channels);

        for (auto& shifter : shifters)
            shifter.bypass (in, discard, 1, count);

        for (int i = 0; i < count; ++i)
            for (int ch = 0; ch < channels; ++ch)
                buffer.setSample (ch, offset + i, leadRing[(size_t) ch][(size_t) ((leadCount + i - latency) & leadMask)]);

        leadCount += count;
    }

    for (auto& ring : delayRing)
        std::fill (ring.begin(), ring.end(), 0.0f);

    controls.clear();
    tracker.reset();
    gate.fill (0.0f);
    gainsStarted = false;
    reference = -1;
    liveLead = -1.0f;
    liveVoice[0] = liveVoice[1] = -1.0f;
}

//==============================================================================
void BuiltInHarmoniser::drainFrames()
{
    const int ready = fifo.getNumReady();

    if (ready <= 0)
        return;

    int start1, size1, start2, size2;
    fifo.prepareToRead (ready, start1, size1, start2, size2);
    const size_t maxRecent = (size_t) (recentSeconds * rate / hop);

    auto take = [&] (const harm::Frame& f)
    {
        const bool idle = f.beat < 0.0f && f.lead < 0.0f && f.rmsDb < -60.0f;

        if (! idle)
        {
            if (recent.size() >= maxRecent)
                recent.erase (recent.begin(), recent.begin() + (std::ptrdiff_t) (recent.size() / 4));

            recent.push_back (f);
        }

        if (f.beat < 0.0f)
        {
            lastLogKey = -1000000;
            return;
        }

        const int key = (int) std::floor (f.beat * 100.0f);

        // What this pass hears replaces what an earlier one heard: the bins it skips are cleared.
        if (lastLogKey > -1000000 && key > lastLogKey && key - lastLogKey <= 6)
            log.erase (log.upper_bound (lastLogKey), log.lower_bound (key));

        log[key] = f;
        lastLogKey = key;
    };

    for (int i = 0; i < size1; ++i)
        take (fifoFrames[(size_t) (start1 + i)]);

    for (int i = 0; i < size2; ++i)
        take (fifoFrames[(size_t) (start2 + i)]);

    fifo.finishedRead (size1 + size2);
}

std::vector<harm::Frame> BuiltInHarmoniser::getTakeLog() const
{
    std::vector<harm::Frame> frames;
    frames.reserve (log.size());

    for (const auto& entry : log)
        frames.push_back (entry.second);

    return frames;
}

void BuiltInHarmoniser::clearTakeLog()
{
    log.clear();
    lastLogKey = -1000000;
}

juce::AudioProcessorEditor* BuiltInHarmoniser::createEditor() { return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr; }

void BuiltInHarmoniser::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInHarmoniser::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
}
