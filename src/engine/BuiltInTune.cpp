#include "BuiltInTune.h"
#include "BuiltInFormat.h"
#include <cmath>

namespace daw
{
using tune::Knob;

const char* BuiltInTune::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::mode:        return "mode";
        case Knob::target:      return "target";
        case Knob::speed:       return "speed";
        case Knob::amount:      return "amount";
        case Knob::tolerance:   return "tolerance";
        case Knob::keepVibrato: return "keepVibrato";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInTune::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value));
    };

    add (Knob::mode, "Mode", { 0.0f, 1.0f, 1.0f }, 0.0f);
    add (Knob::target, "Target", { 0.0f, 2.0f, 1.0f }, 0.0f);
    add (Knob::speed, "Speed", { 0.0f, 400.0f, 1.0f }, 50.0f);
    add (Knob::amount, "Amount", { 0.0f, 100.0f, 1.0f }, 100.0f);
    add (Knob::tolerance, "Tolerance", { 0.0f, 50.0f, 1.0f }, 10.0f);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { getParameterId (Knob::keepVibrato), 1 }, "Keep vibrato", true));
    return layout;
}

BuiltInTune::BuiltInTune()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo()).withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItTune", createLayout())
{
    for (int i = 0; i < tune::numKnobs; ++i)
        values[(size_t) i] = parameters.getRawParameterValue (getParameterId ((Knob) i));

    recent.reserve (4096);
    prepareToPlay (44100.0, 512);
    startDraining();
}

BuiltInTune::~BuiltInTune()
{
    stopTimer();
}

juce::PluginDescription BuiltInTune::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Effect";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44547531;
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInTune::fillInPluginDescription (juce::PluginDescription& d) const { d = getDescription(); }

juce::RangedAudioParameter* BuiltInTune::getKnobParameter (Knob k) const { return parameters.getParameter (getParameterId (k)); }

tune::Settings BuiltInTune::getSettings() const
{
    tune::Settings s;
    s.mode = values[(size_t) Knob::mode]->load() >= 0.5f ? tune::Mode::correct : tune::Mode::check;
    const int target = juce::roundToInt (values[(size_t) Knob::target]->load());
    s.target = target == 1 ? tune::Target::chords : (target >= 2 ? tune::Target::chromatic : tune::Target::key);
    s.speedMs = values[(size_t) Knob::speed]->load();
    s.amountPercent = values[(size_t) Knob::amount]->load();
    s.toleranceCents = values[(size_t) Knob::tolerance]->load();
    s.keepVibrato = values[(size_t) Knob::keepVibrato]->load() >= 0.5f;
    return s;
}

void BuiltInTune::setSettings (const tune::Settings& s)
{
    auto set = [this] (Knob k, float v) { if (auto* p = getKnobParameter (k)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set (Knob::mode, s.mode == tune::Mode::correct ? 1.0f : 0.0f);
    set (Knob::target, s.target == tune::Target::key ? 0.0f : (s.target == tune::Target::chords ? 1.0f : 2.0f));
    set (Knob::speed, s.speedMs);
    set (Knob::amount, s.amountPercent);
    set (Knob::tolerance, s.toleranceCents);
    set (Knob::keepVibrato, s.keepVibrato ? 1.0f : 0.0f);
}

void BuiltInTune::applyPreset (const tune::Preset& p) { setSettings (p.settings); }

bool BuiltInTune::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
}

int BuiltInTune::getLatencyFor (double sampleRate)
{
    const int w = pitch::getWindowSize (sampleRate);
    return w / 2 + w / 8;
}

//==============================================================================
void BuiltInTune::Controls::add (const Entry& e) noexcept
{
    if (size == capacity)
    {
        std::copy (entries.begin() + 1, entries.end(), entries.begin());
        --size;
    }

    entries[(size_t) size++] = e;
}

int BuiltInTune::Controls::find (juce::int64 sample) const noexcept
{
    for (int i = size - 1; i >= 0; --i)
        if (entries[(size_t) i].centre <= sample)
            return i;

    return -1;
}

float BuiltInTune::Controls::getPeriod (juce::int64 sample) const
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

float BuiltInTune::Controls::getRatio (juce::int64 sample) const
{
    if (size == 0)
        return 1.0f;

    const int k = find (sample);

    if (k < 0)
        return entries[0].ratio;

    const auto& a = entries[(size_t) k];

    if (k + 1 >= size)
        return a.ratio;

    const auto& b = entries[(size_t) k + 1];

    if (a.period <= 0.0f || b.period <= 0.0f)
        return 1.0f;

    const float t = (float) (sample - a.centre) / (float) juce::jmax ((juce::int64) 1, b.centre - a.centre);
    return a.ratio + t * (b.ratio - a.ratio);
}

//==============================================================================
void BuiltInTune::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    window = pitch::getWindowSize (rate);
    hop = window / 8;
    latency = window / 2 + hop;
    setLatencySamples (latency);
    tracker.prepare (rate, window);
    windowData.assign ((size_t) window, 0.0f);
    corrector.prepare (rate / hop);
    shifter.prepare (rate, latency);
    controls.clear();
    clap.prepare (rate);
    liveSung = liveCorrected = -1.0f;
    startDraining();
}

void BuiltInTune::startDraining()
{
    // The timer needs the message thread; a processor made anywhere else starts it when it is prepared.
    if (auto* manager = juce::MessageManager::getInstanceWithoutCreating(); manager != nullptr && manager->isThisTheMessageThread() && ! isTimerRunning())
        startTimerHz (30);
}

void BuiltInTune::analyse (int n, juce::int64 t0)
{
    auto s = getSettings();

    if (hearOriginal.load())
        s.mode = tune::Mode::check;

    const int mask = tune::getTargetMask (s.target, keyMask.load(), chordMask.load());
    double ppq = 0.0, bpm = 120.0;
    bool playing = false;

    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
        {
            playing = position->getIsPlaying() && position->getPpqPosition().hasValue();
            ppq = position->getPpqPosition().orFallback (0.0);
            bpm = position->getBpm().orFallback (120.0);
        }

    const double samplesPerBeat = rate * 60.0 / juce::jmax (1.0, bpm);
    const juce::int64 end = t0 + n;

    for (juce::int64 b = (t0 / hop + 1) * hop; b <= end; b += hop)
    {
        if (b < window)
            continue;

        shifter.copyMid (b, windowData.data(), window);
        const auto reading = tracker.analyse (windowData.data(), window, pitch::Tracker::minClarity, true);
        const auto step = corrector.step (reading.voiced ? reading.midi : -1.0, s, mask);

        Controls::Entry entry;
        entry.centre = b - window / 2;

        if (reading.voiced)
        {
            entry.period = (float) (rate / tune::getHz (reading.midi));
            entry.ratio = (float) tune::getRatio (step.correctionCents);
        }

        controls.add (entry);

        tune::Frame frame;
        frame.sung = reading.voiced ? reading.midi : -1.0f;
        frame.corrected = reading.voiced ? reading.midi + (float) (step.correctionCents * 0.01) : -1.0f;
        frame.rmsDb = reading.rmsDb;
        frame.beat = playing ? (float) (ppq + ((double) (b - t0) + (double) (latency - window / 2)) / samplesPerBeat) : -1.0f;
        liveSung = frame.sung;
        liveCorrected = frame.corrected;

        int start1, size1, start2, size2;
        fifo.prepareToWrite (1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            fifoFrames[(size_t) start1] = frame;
            fifo.finishedWrite (1);
        }
    }
}

void BuiltInTune::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples(), channels = juce::jmin (2, buffer.getNumChannels());

    if (channels < 1 || n <= 0)
        return;

    clap.addTo (buffer, channels);
    const auto t0 = shifter.getInputCount();
    shifter.push (buffer.getArrayOfReadPointers(), channels, n);
    analyse (n, t0);
    shifter.pull (buffer.getArrayOfWritePointers(), channels, n, controls);
}

void BuiltInTune::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int n = buffer.getNumSamples(), channels = juce::jmin (2, buffer.getNumChannels());

    if (channels < 1 || n <= 0)
        return;

    shifter.bypass (buffer.getArrayOfReadPointers(), buffer.getArrayOfWritePointers(), channels, n);
    controls.clear();
    corrector.reset();
    tracker.reset();
    liveSung = liveCorrected = -1.0f;
}

//==============================================================================
void BuiltInTune::drainFrames()
{
    const int ready = fifo.getNumReady();

    if (ready <= 0)
        return;

    int start1, size1, start2, size2;
    fifo.prepareToRead (ready, start1, size1, start2, size2);
    const size_t maxRecent = (size_t) (recentSeconds * rate / hop);

    auto take = [&] (const tune::Frame& f)
    {
        const bool idle = f.beat < 0.0f && f.sung < 0.0f && f.rmsDb < -60.0f;

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

std::vector<tune::Frame> BuiltInTune::getTakeLog() const
{
    std::vector<tune::Frame> frames;
    frames.reserve (log.size());

    for (const auto& entry : log)
        frames.push_back (entry.second);

    return frames;
}

void BuiltInTune::clearTakeLog()
{
    log.clear();
    lastLogKey = -1000000;
}

juce::AudioProcessorEditor* BuiltInTune::createEditor() { return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr; }

void BuiltInTune::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInTune::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
}
