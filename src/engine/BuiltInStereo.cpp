#include "BuiltInStereo.h"
#include "BuiltInFormat.h"
#include <cmath>

namespace daw
{
namespace stereo
{
const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets {
        { "Mix bus: tighten the low end", "Whatever wide low end slipped in from pads, reverbs and stereo samples is made mono, and nothing above 120 Hz changes. The usual first step on a bus that holds the whole mix.", { 120, 24, 100, 0, 0 }, true },
        { "Synth bass: mono body, wide top", "The notes and the body stay in the middle, solid on every system; the bright, chorused top keeps its width.", { 200, 24, 100, 0, 0 }, true },
        { "Fully mono", "For a kick, a bass or a single voice that came as a stereo file: one solid point, which the track's pan knob then places.", { 120, 24, 0, 0, 0 }, true },
        { "Vinyl-safe", "A wide or out-of-phase bass makes a record's groove deep and shallow, and the needle can jump. Cutting engineers usually make the bass mono somewhere between 100 and 300 Hz; 150 Hz is a common middle. Ask yours.", { 150, 24, 100, 0, 0 }, true },
        { "Wide pads", "The pad spreads out behind the vocal while its low end stays centred. Match level keeps the wider pad from simply sounding louder.", { 200, 24, 150, 0, 0 }, true },
        { "Mono check", "Hear the whole sound as a phone plays it. Match level is off so you hear how much quieter the wide parts get. Switch Listen back to Stereo before you export.", { 0, 24, 100, 1, 0 }, false }
    };
    return presets;
}
juce::StringArray analyse (const std::vector<Block>& input, const std::vector<Block>& output, const Settings& s)
{
    Block in, out; int count = 0;
    auto add = [] (Block& a, const Block& b) { a.left += b.left; a.right += b.right; a.cross += b.cross; a.mid += b.mid; a.side += b.side; a.lowLeft += b.lowLeft; a.lowRight += b.lowRight; a.lowCross += b.lowCross; a.lowMid += b.lowMid; a.lowSide += b.lowSide; };
    for (size_t i = 0; i < input.size(); ++i)
        if (input[i].peak > 0.00316228f) { ++count; add (in, input[i]); if (i < output.size()) add (out, output[i]); }
    juce::StringArray lines;
    const auto f = juce::String (juce::roundToInt (s.monoBelowHz > 0 ? s.monoBelowHz : 120));
    const auto lowSide = sideToMidDb (in.lowMid, in.lowSide), wholeSide = sideToMidDb (in.mid, in.side);
    const auto lowR = correlation (in.lowLeft, in.lowRight, in.lowCross).value_or (0.0), wholeR = correlation (in.left, in.right, in.cross).value_or (0.0);
    const auto outLowR = correlation (out.lowLeft, out.lowRight, out.lowCross).value_or (0.0), outR = correlation (out.left, out.right, out.cross).value_or (0.0);
    const double share = 10.0 * std::log10 (juce::jmax (1.0e-10, (in.lowLeft + in.lowRight) / juce::jmax (1.0e-20, in.left + in.right)));
    if (count < 40) { lines.add ("Play the song where this track plays. DawIt Stereo needs a couple of seconds of sound to measure it."); return lines; }
    else if (wholeSide <= -40) lines.add ("Your track is mono: left and right are the same (the side is more than 40 dB under the mid). Mono below and Width have nothing to work on here. The pan knob places it; to make it wide, add a stereo reverb or a short delay on one side.");
    else
    {
        if (share <= -30) lines.add ("There's little below " + f + " Hz in this track (" + juce::String (juce::roundToInt (-share)) + " dB under the rest), so Mono below changes little. It matters most on basses, kicks, pads, stereo pianos, reverbs and whole mixes.");
        else if (lowR < 0) lines.add ("Below " + f + " Hz left and right work against each other (correlation " + juce::String (lowR, 2) + "): in mono most of that low end cancels. This often comes from a stereo widener, two microphones on one source, or a polarity switch. " + (s.monoBelowHz > 0 ? "Mono below fixes the low end; also check the source (What is sound? > Phase cancellation)." : "Mono below " + f + " Hz would fix the low end; also check the source (What is sound? > Phase cancellation)."));
        else if (lowSide >= -12)
        {
            if (s.monoBelowHz > 0)
            {
                // The measured low band is the sound under the crossover; a gradual crossover leaves what sits just under or over it nearly as wide as it was.
                const auto outSide = sideToMidDb (out.lowMid, out.lowSide);
                const auto sideWas = juce::String (juce::roundToInt (-lowSide)), sideNow = juce::String (juce::roundToInt (-outSide));
                const auto sides = "the difference between left and right (the side) was " + sideWas + " dB under what they share (the mid) and is now " + sideNow + " dB under";
                if (lowSide - outSide >= 6.0) lines.add ("Mono below " + f + " Hz makes the low end mono: " + sides + " (correlation " + juce::String (lowR, 2) + " to " + juce::String (outLowR, 2) + ").");
                else lines.add ("Mono below " + f + " Hz changes this low end little: " + sides + ". Most of the low sound here lies close to " + f + " Hz, where the crossover works gradually (each side of it is 6 dB down at " + f + " Hz). " + (s.slope < 48 ? "Try a higher Mono below frequency or a steeper Slope." : "Try a higher Mono below frequency."));
            }
            else lines.add ("Below " + f + " Hz your track is wide: its side is only " + juce::String (juce::roundToInt (-lowSide)) + " dB under its mid (correlation " + juce::String (lowR, 2) + "). In mono, or on a club's mono subwoofers, that part drops out, so the low end sounds different on every system. Try Mono below " + f + " Hz.");
        }
        if (s.width > 100 && outR < 0) lines.add ("With Width at " + juce::String (juce::roundToInt (s.width)) + "% the whole track's correlation is " + juce::String (outR, 2) + ": left and right work more against each other than with each other. It can sound hollow, and in mono only its mid is left, so it drops back much further than the rest of the mix. Try 130% or less.");
        else if (s.width <= 100 && wholeR < 0) lines.add ("Your track's left and right work against each other (correlation " + juce::String (wholeR, 2) + "): in mono parts of it cancel. Look for a polarity switch or a stereo widener earlier in the chain; Width under 100% softens it.");
        if (lines.isEmpty()) lines.add ("Below " + f + " Hz your track is close to mono already (side " + juce::String (juce::roundToInt (-lowSide)) + " dB under the mid), and the whole track's correlation is " + juce::String (wholeR, 2) + ". Mono below changes little here, and that's fine.");
    }
    if (s.listen != 0) lines.add ("Listen is set to " + juce::String (s.listen == 1 ? "Mono: the track plays in mono" : "Side: the track plays only its side") + ". Switch back to Stereo before you export.");
    return lines;
}
}
using stereo::Knob;
const char* BuiltInStereo::getParameterId (Knob k)
{
    switch (k) { case Knob::monoBelow: return "monoBelow"; case Knob::slope: return "slope"; case Knob::width: return "width"; case Knob::listen: return "listen"; case Knob::output: return "output"; }
    return "";
}
juce::AudioProcessorValueTreeState::ParameterLayout BuiltInStereo::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&] (Knob k, const char* name, juce::NormalisableRange<float> range, float value) { layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value)); };
    juce::NormalisableRange<float> frequency (0.0f, 500.0f,
        [] (float, float, float v) { return v < 0.02f ? 0.0f : 20.0f * std::pow (25.0f, (v - 0.02f) / 0.98f); },
        [] (float, float, float v) { return v < 20.0f ? 0.0f : 0.02f + 0.98f * std::log (v / 20.0f) / std::log (25.0f); },
        [] (float, float, float v) { return v < 10.0f ? 0.0f : juce::jlimit (20.0f, 500.0f, std::round (v)); });
    juce::NormalisableRange<float> slopes (12.0f, 48.0f,
        [] (float, float, float v) { return v < 0.25f ? 12.0f : (v < 0.75f ? 24.0f : 48.0f); },
        [] (float, float, float v) { return v < 18.0f ? 0.0f : (v < 36.0f ? 0.5f : 1.0f); },
        [] (float, float, float v) { return v < 18.0f ? 12.0f : (v < 36.0f ? 24.0f : 48.0f); });
    add (Knob::monoBelow, "Mono below", frequency, 120);
    add (Knob::slope, "Slope", slopes, 24);
    add (Knob::width, "Width", { 0.0f, 200.0f, 0.1f }, 100);
    add (Knob::listen, "Listen", { 0.0f, 2.0f, 1.0f }, 0);
    add (Knob::output, "Output", { -12.0f, 12.0f, 0.1f }, 0);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "match", 1 }, "Match level", true));
    return layout;
}
BuiltInStereo::BuiltInStereo() : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo()).withOutput ("Output", juce::AudioChannelSet::stereo())), parameters (*this, nullptr, "DawItStereo", createLayout())
{
    for (int i = 0; i < stereo::numKnobs; ++i) values[(size_t) i] = parameters.getRawParameterValue (getParameterId ((Knob) i));
    matchLevel = parameters.getRawParameterValue ("match");
}
juce::PluginDescription BuiltInStereo::getDescription()
{
    juce::PluginDescription d; d.name = d.descriptiveName = identifier; d.pluginFormatName = BuiltInFormat::formatName; d.category = "Effect"; d.manufacturerName = "DawIt"; d.version = "1.0"; d.fileOrIdentifier = identifier; d.uniqueId = d.deprecatedUid = 0x44537431; d.isInstrument = false; d.numInputChannels = d.numOutputChannels = 2; return d;
}
void BuiltInStereo::fillInPluginDescription (juce::PluginDescription& d) const { d = getDescription(); }
juce::RangedAudioParameter* BuiltInStereo::getKnobParameter (Knob k) const { return parameters.getParameter (getParameterId (k)); }
stereo::Settings BuiltInStereo::getSettings() const { return { values[0]->load(), (values[1]->load() < 18.0f ? 12 : (values[1]->load() < 36.0f ? 24 : 48)), values[2]->load(), juce::roundToInt (values[3]->load()), values[4]->load() }; }
void BuiltInStereo::setKnob (Knob k, float v) { if (k == Knob::slope) v = v < 18.0f ? 12.0f : (v < 36.0f ? 24.0f : 48.0f); if (auto* p = getKnobParameter (k)) p->setValueNotifyingHost (p->convertTo0to1 (v)); }
void BuiltInStereo::setSettings (const stereo::Settings& s) { setKnob (Knob::monoBelow, s.monoBelowHz); setKnob (Knob::slope, (float) s.slope); setKnob (Knob::width, s.width); setKnob (Knob::listen, (float) s.listen); setKnob (Knob::output, s.outputDb); }
void BuiltInStereo::setMatchingLevel (bool on) { if (auto* p = parameters.getParameter ("match")) p->setValueNotifyingHost (on ? 1.0f : 0.0f); }
void BuiltInStereo::applyPreset (const stereo::Preset& p) { setSettings (p.settings); setMatchingLevel (p.match); }
bool BuiltInStereo::isBusesLayoutSupported (const BusesLayout& l) const { const auto out = l.getMainOutputChannelSet(); return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out; }
void BuiltInStereo::prepareToPlay (double sr, int)
{
    rate = sr > 0 ? sr : 44100; clap.prepare (rate); inputMeasurement.prepare (rate); outputMeasurement.prepare (rate); paths[0] = {}; paths[1] = {}; measurementOutput = {}; activePath = fadeRemaining = 0;
    auto s = getSettings(); paths[0].slope = s.slope; paths[0].on = s.monoBelowHz >= 20; fadeLength = juce::jmax (1, juce::roundToInt (rate * 0.02));
    widthSmooth.reset (rate, 0.02); widthSmooth.setCurrentAndTargetValue (s.width * 0.01f); cutoffSmooth.reset (rate, 0.02); cutoffSmooth.setCurrentAndTargetValue (juce::jmax (20.0f, s.monoBelowHz)); powerIn = powerOut = 0; gain = 1;
}
void BuiltInStereo::split (Path& p, int ch, double sample, double cutoff, double& low, double& high)
{
    low = high = sample;
    const double omega = 2.0 * juce::MathConstants<double>::pi * juce::jlimit (20.0, rate * 0.45, cutoff) / rate;
    if (p.slope == 12)
    {
        const double k = std::tan (omega * 0.5), a = (k - 1.0) / (k + 1.0), b = k / (k + 1.0), h = 1.0 / (k + 1.0);
        for (int i = 0; i < 2; ++i) { low = p.low[ch][i].run (low, b, b, 0, a, 0); high = p.high[ch][i].run (high, h, -h, 0, a, 0); }
        high = -high;
        return;
    }
    const double c = std::cos (omega), sn = std::sin (omega);
    const int stages = p.slope == 48 ? 4 : 2;
    for (int i = 0; i < stages; ++i)
    {
        const double q = stages == 2 ? 0.7071067811865476 : (i % 2 == 0 ? 0.541196100146197 : 1.306562964876377);
        const double alpha = sn / (2.0 * q), a0 = 1.0 + alpha, a1 = -2.0 * c / a0, a2 = (1.0 - alpha) / a0;
        const double b = (1.0 - c) * 0.5 / a0, h = (1.0 + c) * 0.5 / a0;
        low = p.low[ch][i].run (low, b, 2.0 * b, b, a1, a2);
        high = p.high[ch][i].run (high, h, -2.0 * h, h, a1, a2);
    }
}
void BuiltInStereo::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    if (buffer.getNumChannels() < 2) return;
    clap.addTo (buffer, 2);
    const auto s = getSettings();
    const bool on = s.monoBelowHz >= 20.0f;
    const int slope = s.slope < 18 ? 12 : (s.slope < 36 ? 24 : 48);
    if (measurementOutput.slope != slope)
    {
        measurementOutput = {};
        measurementOutput.slope = slope;
    }
    if (fadeRemaining == 0 && (paths[activePath].on != on || paths[activePath].slope != slope))
    {
        const int next = 1 - activePath; paths[next] = {}; paths[next].on = on; paths[next].slope = slope; fadeRemaining = fadeLength;
    }
    if (measurementOutput.slope != slope) { measurementOutput = {}; measurementOutput.slope = slope; }
    widthSmooth.setTargetValue (s.width * 0.01f); cutoffSmooth.setTargetValue (on ? s.monoBelowHz : 120.0f);
    auto* left = buffer.getWritePointer (0); auto* right = buffer.getWritePointer (1);
    const double coefficient = std::exp (-1.0 / (rate * 0.5));
    const double outGain = juce::Decibels::decibelsToGain ((double) s.outputDb);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const double l = left[i], r = right[i], width = widthSmooth.getNextValue(), cutoff = cutoffSmooth.getNextValue();
        auto run = [&] (Path& p, double& ol, double& oright, double& ll, double& lr, double& hl, double& hr)
        {
            split (p, 0, l, cutoff, ll, hl); split (p, 1, r, cutoff, lr, hr);
            if (! p.on) { ol = (l + r + width * (l - r)) * 0.5; oright = (l + r - width * (l - r)) * 0.5; return; }
            const double midLow = (ll + lr) * 0.5, midHigh = (hl + hr) * 0.5, sideHigh = (hl - hr) * 0.5 * width;
            ol = midLow + midHigh + sideHigh; oright = midLow + midHigh - sideHigh;
        };
        double a, b, lowL, lowR, highLIn, highRIn; run (paths[activePath], a, b, lowL, lowR, highLIn, highRIn);
        if (fadeRemaining > 0)
        {
            double c, d, dummyL, dummyR, dummyHighL, dummyHighR; run (paths[1 - activePath], c, d, dummyL, dummyR, dummyHighL, dummyHighR);
            const double t = 1.0 - (double) fadeRemaining / (double) fadeLength;
            a += (c - a) * t; b += (d - b) * t;
            lowL += (dummyL - lowL) * t; lowR += (dummyR - lowR) * t;
            highLIn += (dummyHighL - highLIn) * t; highRIn += (dummyHighR - highRIn) * t;
            if (--fadeRemaining == 0) activePath = 1 - activePath;
        }
        inputMeasurement.push ((float) l, (float) r, (float) lowL, (float) lowR, (float) highLIn, (float) highRIn);
        powerIn = coefficient * powerIn + (1.0 - coefficient) * (l * l + r * r);
        powerOut = coefficient * powerOut + (1.0 - coefficient) * (a * a + b * b);
        const double target = powerOut > 1.0e-12 ? juce::jlimit (0.25, 4.0, std::sqrt (powerIn / powerOut)) : 1.0;
        gain += (target - gain) * (1.0 - std::exp (-1.0 / (rate * 0.1)));
        matchGainDb.store ((float) juce::Decibels::gainToDecibels (gain));
        if (s.listen == 1) a = b = (a + b) * 0.5;
        else if (s.listen == 2) a = b = (a - b) * 0.5;
        else if (isMatchingLevel()) { a *= gain; b *= gain; }
        a *= outGain; b *= outGain;
        if (listenToRemoved.load()) { a = l - a; b = r - b; }
        left[i] = (float) a; right[i] = (float) b;
        double measuredL, measuredR, highL, highR;
        split (measurementOutput, 0, a, cutoff, measuredL, highL);
        split (measurementOutput, 1, b, cutoff, measuredR, highR);
        outputMeasurement.push ((float) a, (float) b, (float) measuredL, (float) measuredR, (float) highL, (float) highR);
    }
}
juce::AudioProcessorEditor* BuiltInStereo::createEditor() { return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr; }
void BuiltInStereo::getStateInformation (juce::MemoryBlock& dest) { if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, dest); }
void BuiltInStereo::setStateInformation (const void* data, int size) { if (auto xml = getXmlFromBinary (data, size); xml != nullptr && xml->hasTagName (parameters.state.getType())) parameters.replaceState (juce::ValueTree::fromXml (*xml)); }
}
