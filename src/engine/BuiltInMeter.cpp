#include "BuiltInMeter.h"
#include "BuiltInFormat.h"
#include <cmath>

namespace daw
{
namespace meterfx
{
const std::vector<Preset>& getPresets()
{
    using K = loudness::TargetKind;
    static const std::vector<Preset> presets {
        { "Streaming (Spotify, YouTube, Tidal, Amazon)", "These services play songs at about -14 LUFS. A master near it plays almost unchanged, and a louder one is turned down. Spotify asks for a true peak of -1 dBTP or lower (-2 dBTP for masters louder than -14 LUFS), because encoding can raise the peaks.", { -14.0f, -1.0f, true, true }, K::streaming },
        { "Apple Music", "Apple Music's Sound Check plays songs at about -16 LUFS. A master made for -14 LUFS plays 2 dB quieter there, which is fine.", { -16.0f, -1.0f, true, true }, K::appleMusic },
        { "Podcast", "Many podcast platforms ask for about -16 LUFS for a stereo show, with peaks at -1 dBTP or lower. Speech varies less than music, so it's easy to keep close to the target.", { -16.0f, -1.0f, true, true }, K::podcast },
        { "Broadcast, Europe (EBU R 128)", "European television and radio deliver every programme at -23.0 LUFS (\xc2\xb1" "1 LU where that's not practical, as in live shows), with a true peak of -1 dBTP or lower.", { -23.0f, -1.0f, true, true }, K::ebu },
        { "Broadcast, US (ATSC A/85)", "US television asks for -24 LKFS (the same as LUFS) and a true peak of -2 dBTP or lower.", { -24.0f, -2.0f, true, true }, K::atsc },
        { "Club and DJ", "Clubs and DJ software usually play files as they are, without normalisation, so club and EDM masters are often loud: about -9 to -6 LUFS. That's a choice, not a rule, and it costs punch.", { -8.0f, -1.0f, true, true }, K::club },
        { "Mixing (no target)", "While mixing there's no loudness target. Keep headroom (peaks around -6 dBFS on the master), and use the short-term loudness to compare effects and sections at the same level.", { -14.0f, -1.0f, false, true }, K::mixing }
    };
    return presets;
}
}

using meterfx::Knob;

const char* BuiltInMeter::getParameterId (Knob k)
{
    switch (k) { case Knob::target: return "target"; case Knob::ceiling: return "ceiling"; }
    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInMeter::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "target", 1 }, "Target", juce::NormalisableRange<float> { -30.0f, -5.0f, 0.1f }, -14.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "ceiling", 1 }, "Ceiling", juce::NormalisableRange<float> { -3.0f, 0.0f, 0.1f }, -1.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "showTarget", 1 }, "Show target", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "resetOnPlay", 1 }, "Reset on play", true));
    // Which starting point was chosen (0: none), so the advice can name it; it counts only while the knobs still match it.
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "startingPoint", 1 }, "Starting point", 0, (int) meterfx::getPresets().size(), 1));
    return layout;
}

BuiltInMeter::BuiltInMeter()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo()).withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItMeter", createLayout())
{
    target = parameters.getRawParameterValue ("target");
    ceiling = parameters.getRawParameterValue ("ceiling");
    showTarget = parameters.getRawParameterValue ("showTarget");
    resetOnPlay = parameters.getRawParameterValue ("resetOnPlay");
    startingPoint = parameters.getRawParameterValue ("startingPoint");
    const auto none = std::nanf ("");
    for (auto& v : published) v = none;
    for (auto& v : historyMomentary) v = none;
    for (auto& v : historyShortTerm) v = none;
    for (auto& v : blockPeak) v = 0.0f;
}

juce::PluginDescription BuiltInMeter::getDescription()
{
    juce::PluginDescription d; d.name = d.descriptiveName = identifier; d.pluginFormatName = BuiltInFormat::formatName; d.category = "Effect"; d.manufacturerName = "DawIt"; d.version = "1.0";
    d.fileOrIdentifier = identifier; d.uniqueId = d.deprecatedUid = 0x444d7431; d.isInstrument = false; d.numInputChannels = d.numOutputChannels = 2;   // "DMt1"
    return d;
}
void BuiltInMeter::fillInPluginDescription (juce::PluginDescription& d) const { d = getDescription(); }
juce::RangedAudioParameter* BuiltInMeter::getKnobParameter (Knob k) const { return parameters.getParameter (getParameterId (k)); }
meterfx::Settings BuiltInMeter::getSettings() const { return { target->load(), ceiling->load(), showTarget->load() >= 0.5f, resetOnPlay->load() >= 0.5f }; }
void BuiltInMeter::setKnob (Knob k, float v) { if (auto* p = getKnobParameter (k)) p->setValueNotifyingHost (p->convertTo0to1 (v)); }
void BuiltInMeter::setShowTarget (bool on) { if (auto* p = parameters.getParameter ("showTarget")) p->setValueNotifyingHost (on ? 1.0f : 0.0f); }
void BuiltInMeter::setResetOnPlay (bool on) { if (auto* p = parameters.getParameter ("resetOnPlay")) p->setValueNotifyingHost (on ? 1.0f : 0.0f); }
void BuiltInMeter::setSettings (const meterfx::Settings& s) { setKnob (Knob::target, s.targetLufs); setKnob (Knob::ceiling, s.ceilingDbtp); setShowTarget (s.showTarget); setResetOnPlay (s.resetOnPlay); }

void BuiltInMeter::applyPreset (const meterfx::Preset& p)
{
    auto s = getSettings();
    s.targetLufs = p.settings.targetLufs; s.ceilingDbtp = p.settings.ceilingDbtp; s.showTarget = p.settings.showTarget;
    setSettings (s);
    const auto& presets = meterfx::getPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].name == p.name)
            if (auto* parameter = parameters.getParameter ("startingPoint"))
                parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) i + 1.0f));
}

loudness::TargetKind BuiltInMeter::getTargetKind() const
{
    const int index = juce::roundToInt (startingPoint->load()) - 1;
    const auto& presets = meterfx::getPresets();
    if (index >= 0 && index < (int) presets.size())
    {
        const auto& p = presets[(size_t) index].settings;
        const auto s = getSettings();
        if (std::abs (p.targetLufs - s.targetLufs) < 0.05f && std::abs (p.ceilingDbtp - s.ceilingDbtp) < 0.05f && p.showTarget == s.showTarget)
            return presets[(size_t) index].kind;
    }
    return loudness::TargetKind::own;
}

bool BuiltInMeter::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
}

void BuiltInMeter::prepareToPlay (double sr, int)
{
    const double newRate = sr > 0 ? sr : 44100.0;
    // Preparing again at the same rate (after an offline pass, or a device restart) keeps what was measured; Reset and
    // Reset on play are the ways to clear it.
    const bool keep = prepared && juce::exactlyEqual (newRate, rate);
    rate = newRate;
    clap.prepare (rate); measurement.prepare (rate);

    if (! keep)
        meter.prepare (rate);

    prepared = true;
    tickInterval = juce::jmax (1, juce::roundToInt (rate * 0.1)); tickCounter = 0; wasPlaying = false;
    measuring = true;
    publish();
}

loudness::Readings BuiltInMeter::getReadings() const
{
    const auto get = [this] (size_t i) -> std::optional<double>
    {
        const float v = published[i].load();
        return std::isnan (v) ? std::nullopt : std::optional<double> ((double) v);
    };
    loudness::Readings r;
    r.momentary = get (0); r.shortTerm = get (1); r.integrated = get (2); r.range = get (3); r.maxMomentary = get (4); r.maxShortTerm = get (5);
    r.truePeak = get (6); r.samplePeak = get (7); r.rms = get (8); r.crest = get (9);
    r.seconds = (double) published[10].load();
    return r;
}

void BuiltInMeter::publish() noexcept
{
    const auto r = meter.getReadings();
    const auto none = std::nanf ("");
    const auto put = [&] (size_t i, const std::optional<double>& v) { published[i].store (v ? (float) *v : none); };
    put (0, r.momentary); put (1, r.shortTerm); put (2, r.integrated); put (3, r.range); put (4, r.maxMomentary); put (5, r.maxShortTerm);
    put (6, r.truePeak); put (7, r.samplePeak); put (8, r.rms); put (9, r.crest);
    published[10].store ((float) r.seconds);
}

std::vector<meterfx::HistoryPoint> BuiltInMeter::getHistory() const
{
    const auto written = historyWritten.load();
    const auto count = (size_t) juce::jmin<unsigned long long> (written, (unsigned long long) meterfx::historyLength);
    std::vector<meterfx::HistoryPoint> points;
    points.reserve (count);
    for (auto i = written - count; i < written; ++i)
    {
        const auto slot = (size_t) (i % (unsigned long long) meterfx::historyLength);
        points.push_back ({ historyMomentary[slot].load(), historyShortTerm[slot].load() });
    }
    return points;
}

void BuiltInMeter::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    if (channels == 0) return;
    clap.addTo (buffer, channels);

    if (resetRequested.exchange (false))
    {
        meter.reset();
        ++resetCount;
    }

    bool counting = true;
    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
        {
            const bool playing = position->getIsPlaying();
            if (playing && ! wasPlaying && resetOnPlay->load() >= 0.5f)
            {
                meter.reset();
                ++resetCount;
            }
            wasPlaying = playing;
            counting = playing;
        }
    measuring = counting;

    const float* left = buffer.getReadPointer (0);
    const float* right = channels > 1 ? buffer.getReadPointer (1) : nullptr;
    float peakLeft = 0.0f, peakRight = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float l = left[i], r = right != nullptr ? right[i] : 0.0f;
        meter.push (l, r, channels, counting);
        peakLeft = juce::jmax (peakLeft, std::abs (l)); peakRight = juce::jmax (peakRight, std::abs (r));
        if (right != nullptr) measurement.push (l, r, l, r);
        if (++tickCounter >= tickInterval)
        {
            tickCounter = 0;
            const auto none = std::nanf ("");
            const auto slot = (size_t) (historyWritten.load() % (unsigned long long) meterfx::historyLength);
            const auto m = meter.momentary(), s = meter.shortTerm();
            historyMomentary[slot].store (m ? (float) *m : none);
            historyShortTerm[slot].store (s ? (float) *s : none);
            ++historyWritten;
        }
    }
    blockPeak[0].store (peakLeft); blockPeak[1].store (peakRight);
    publish();
}

juce::AudioProcessorEditor* BuiltInMeter::createEditor() { return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr; }
void BuiltInMeter::getStateInformation (juce::MemoryBlock& dest) { if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, dest); }
void BuiltInMeter::setStateInformation (const void* data, int size) { if (auto xml = getXmlFromBinary (data, size); xml != nullptr && xml->hasTagName (parameters.state.getType())) parameters.replaceState (juce::ValueTree::fromXml (*xml)); }
}
