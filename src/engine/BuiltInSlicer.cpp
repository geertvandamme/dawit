#include "BuiltInSlicer.h"
#include "BuiltInFormat.h"

#include <map>

namespace daw
{
namespace slc
{
    const char* getModeName (Mode mode)
    {
        switch (mode)
        {
            case Mode::through: return "Through";
            case Mode::repeat:  return "Repeat";
            case Mode::reverse: return "Reverse";
            case Mode::mute:    return "Mute";
        }

        return "";
    }

    int getSliceCapacity (double sampleRate)
    {
        // One beat at the slowest tempo, and a few samples to spare for rounding.
        return (int) std::ceil (sampleRate * 60.0 / (double) minOwnBpm) + 64;
    }

    namespace
    {
        juce::String formatBpm (double bpm)
        {
            const bool whole = std::abs (bpm - std::round (bpm)) < 0.05;
            return whole ? juce::String (juce::roundToInt (bpm)) : juce::String (bpm, 1);
        }

        juce::String formatMs (double ms)
        {
            return ms >= 100.0 ? juce::String (juce::roundToInt (ms)) : juce::String (ms, 1);
        }
    }

    juce::String patternToText (const Settings& s)
    {
        juce::String letters;

        for (int i = 0; i < maxSteps; ++i)
            letters << "TRVM"[(int) s.modes[(size_t) i]];

        return "DawIt Slicer pattern: " + juce::String (stepLengths[juce::jlimit (0, numStepLengths - 1, s.stepLength)].name) + " "
               + juce::String (s.steps) + " " + letters;
    }

    bool patternFromText (const juce::String& text, Settings& s)
    {
        const auto tokens = juce::StringArray::fromTokens (text.trim(), " \t\r\n", "");

        if (tokens.size() != 6 || ! tokens[0].equalsIgnoreCase ("DawIt") || ! tokens[1].equalsIgnoreCase ("Slicer") || ! tokens[2].equalsIgnoreCase ("pattern:"))
            return false;

        int length = -1;

        for (int i = 0; i < numStepLengths; ++i)
            if (tokens[3].equalsIgnoreCase (stepLengths[i].name))
                length = i;

        const int steps = tokens[4].getIntValue();
        const auto& letters = tokens[5];

        if (length < 0 || ! tokens[4].containsOnly ("0123456789") || steps < 1 || steps > maxSteps || letters.length() < 1 || letters.length() > maxSteps)
            return false;

        std::array<Mode, maxSteps> modes {};

        for (int i = 0; i < letters.length(); ++i)
        {
            const int mode = juce::String ("TRVM").indexOfChar (juce::CharacterFunctions::toUpperCase (letters[i]));

            if (mode < 0)
                return false;

            modes[(size_t) i] = (Mode) mode;
        }

        s.stepLength = length;
        s.steps = steps;
        s.modes = modes;
        return true;
    }

    juce::String formatKnob (Knob knob, const Settings& s)
    {
        switch (knob)
        {
            case Knob::stepLength: return stepLengths[juce::jlimit (0, numStepLengths - 1, s.stepLength)].name;
            case Knob::steps:      return juce::String (s.steps);
            case Knob::roll:       return juce::String (s.roll);
            case Knob::smooth:     return fx::formatNumber (s.smoothMs, s.smoothMs < 10.0f ? 1 : 0) + " ms";
            case Knob::mix:        return juce::String (juce::roundToInt (s.mixPercent)) + "%";
            case Knob::tempo:      return s.ownTempo ? "Own" : "Song";
            case Knob::ownBpm:     return formatBpm (s.ownBpm) + " BPM";
        }

        return {};
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;
            auto every = [] (int first, int last, int stride, Mode mode)
            {
                std::vector<std::pair<int, Mode>> v;

                for (int i = first; i <= last; i += stride)
                    v.push_back ({ i, mode });

                return v;
            };

            auto addList = [&list] (juce::String name, juce::String why, Settings s, const std::vector<std::pair<int, Mode>>& steps)
            {
                for (auto& [step, mode] : steps)
                    s.modes[(size_t) step - 1] = mode;   // steps are numbered from 1, as in the window

                list.push_back ({ std::move (name), std::move (why), s });
            };

            addList ("Neutral: every step plays",
                 "Nothing changes until you click a step. Start here and paint your own pattern.", Settings {}, {});

            addList ("Stutter into the next bar",
                     "The first sixteenth of beat 4 plays four times before the next bar: the stutter before a new section. 1/16 keeps it tight, Smooth 3 ms keeps the cuts clean, Mix 100% so the repeats replace the track.",
                     Settings {}, every (14, 16, 1, Mode::repeat));

            Settings gate;
            addList ("Trance gate",
                     "Every other sixteenth is silent, so a held chord pulses in sixteenths. Best on pads and long notes; 3 ms keeps the edges crisp without clicks.",
                     gate, every (2, 16, 2, Mode::mute));

            Settings soft = gate;
            soft.smoothMs = 15.0f;
            soft.mixPercent = 50.0f;
            addList ("Soft pulse",
                     "The gaps become 6 dB dips with soft edges: a gentle, breathing movement under a mix rather than a chop.",
                     soft, every (2, 16, 2, Mode::mute));

            Settings swell;
            swell.stepLength = 4;   // 1/8
            swell.steps = 8;
            addList ("Reverse swell",
                 "The last eighth of every bar plays the eighth before it backwards: a hit there turns into a swell into the downbeat. 8 steps of 1/8 make one bar.",
                 swell, { { 8, Mode::reverse } });

            Settings rolls;
            rolls.stepLength = 4;
            rolls.steps = 8;
            rolls.roll = 3;
            addList ("Triplet rolls",
                 "For a hi-hat track: steps 2 and 6, the second halves of beats 1 and 3, play the start of the eighth before them three times, as sixteenth triplets, rolling into the snares on beats 2 and 4. Roll 3 makes the triplets.",
                 rolls, { { 2, Mode::repeat }, { 6, Mode::repeat } });

            return list;
        }();

        return presets;
    }

    //==============================================================================
    void StepHistory::push (const StepRecord& r) noexcept
    {
        const auto n = written.load (std::memory_order_relaxed);
        auto& slot = slots[(size_t) (n % (unsigned long long) size)];
        slot.values[0] = (float) r.step;
        slot.values[1] = (float) (int) r.mode;
        slot.values[2] = r.sliced ? 1.0f : 0.0f;
        slot.values[3] = r.inPeak;
        slot.values[4] = r.inRms;
        slot.values[5] = r.outPeak;
        slot.values[6] = r.outRms;
        slot.values[7] = r.slicePeak;
        slot.beat = r.beat;
        written.store (n + 1, std::memory_order_release);
    }

    int StepHistory::getRecent (StepRecord* dest, int max) const
    {
        const auto end = written.load (std::memory_order_acquire);
        const auto floor = cleared.load();
        const auto available = end > floor ? end - floor : 0ULL;
        const auto want = (unsigned long long) juce::jmin (max, size - 8);
        const auto count = juce::jmin (available, want);
        const auto first = end - count;

        for (unsigned long long i = 0; i < count; ++i)
        {
            const auto& slot = slots[(size_t) ((first + i) % (unsigned long long) size)];
            auto& r = dest[i];
            r.step = (int) slot.values[0].load();
            r.mode = (Mode) juce::jlimit (0, numModes - 1, (int) slot.values[1].load());
            r.sliced = slot.values[2].load() > 0.5f;
            r.inPeak = slot.values[3].load();
            r.inRms = slot.values[4].load();
            r.outPeak = slot.values[5].load();
            r.outRms = slot.values[6].load();
            r.slicePeak = slot.values[7].load();
            r.beat = slot.beat.load();
        }

        // If the writer lapped the reader while it copied, the oldest copies may be mixed: drop them.
        const auto after = written.load (std::memory_order_acquire);
        const auto lost = after - first > (unsigned long long) size ? after - first - (unsigned long long) size : 0ULL;
        const auto kept = lost >= count ? 0ULL : count - lost;

        if (kept < count)
            std::copy (dest + lost, dest + count, dest);

        return (int) kept;
    }

    //==============================================================================
    std::vector<juce::String> analyse (const AnalysisInput& in)
    {
        const auto toDb = [] (float linear) { return (double) juce::Decibels::gainToDecibels (linear, -100.0f); };
        const auto& s = in.settings;
        const int steps = juce::jlimit (1, maxSteps, s.steps);

        // S0: nothing heard.
        const bool heard = std::any_of (in.records.begin(), in.records.end(), [&] (auto& r) { return toDb (r.inPeak) >= -60.0; });

        if (! heard)
            return { "Play the track, or click Play a bar, to see it here." };

        std::vector<juce::String> lines;
        int repeats = 0, reverses = 0, mutes = 0;

        for (int i = 0; i < steps; ++i)
        {
            const auto mode = s.modes[(size_t) i];
            repeats += mode == Mode::repeat ? 1 : 0;
            reverses += mode == Mode::reverse ? 1 : 0;
            mutes += mode == Mode::mute ? 1 : 0;
        }

        const int changing = repeats + reverses + mutes;

        // S1: no song tempo.
        if (! s.ownTempo && ! in.hostHasTempo)
            lines.push_back ("No song tempo reaches DawIt Slicer here, so it slices at its own " + formatBpm (s.ownBpm)
                             + " BPM. If the track has a tempo, choose Own and set it, or the steps will drift against the beat.");

        // S2: all Through.
        if (changing == 0)
            lines.push_back ("Every step plays through, so DawIt Slicer leaves the track as it is. Click a step to make it repeat, reverse or mute.");

        // S3: a repeat or reverse of a slice that was almost silent, one line per step, at most three.
        float loudest = 0.0f;

        for (auto& r : in.records)
            loudest = juce::jmax (loudest, r.inPeak);

        std::map<int, std::pair<double, Mode>> quiet;   // by step: the biggest gap and the mode

        for (auto& r : in.records)
        {
            if ((r.mode != Mode::repeat && r.mode != Mode::reverse) || ! r.sliced)
                continue;

            const double gap = toDb (loudest) - toDb (r.slicePeak);

            if (gap > 30.0 && (quiet.count (r.step) == 0 || gap > quiet[r.step].first))
                quiet[r.step] = { gap, r.mode };
        }

        int shown = 0;

        for (auto& [step, info] : quiet)
        {
            if (shown++ == 3)
                break;

            const bool back = info.second == Mode::reverse;
            lines.push_back ("Step " + juce::String (step + 1) + (back ? " plays backwards" : " repeats") + " a slice that was "
                             + juce::String (juce::roundToInt (info.first)) + " dB quieter than the loudest step, so you hear a gap. Move the "
                             + (back ? "reverse" : "repeat") + " to just after a note, or make it a Mute step if a gap is what you want.");
        }

        // S4: the pattern makes the track quieter.
        double inEnergy = 0.0, outEnergy = 0.0;

        for (auto& r : in.records)
        {
            inEnergy += (double) r.inRms * r.inRms;
            outEnergy += (double) r.outRms * r.outRms;
        }

        if (inEnergy > 0.0)
        {
            const double drop = 10.0 * std::log10 (inEnergy / juce::jmax (outEnergy, 1.0e-12));

            if (drop >= 6.0)
                lines.push_back ("The pattern makes the track about " + juce::String (juce::roundToInt (drop)) + " dB quieter on average"
                                 + (mutes > 0 ? ", mostly from its " + juce::String (mutes) + (mutes == 1 ? " Mute step" : " Mute steps") : juce::String())
                                 + ". Fine for a fill; for longer, raise the track's fader" + (mutes > 0 ? " or use fewer Mute steps." : "."));
        }

        // S5: busy.
        if (changing >= 12)
            lines.push_back (juce::String (changing) + " of " + juce::String (steps)
                             + " steps change the sound. Slicing tends to work best as a contrast: try keeping it for the bar before a new section.");

        // S6: summary.
        juce::StringArray parts;

        if (repeats > 0) parts.add (juce::String (repeats) + (repeats == 1 ? " repeat" : " repeats"));
        if (reverses > 0) parts.add (juce::String (reverses) + (reverses == 1 ? " reverse" : " reverses"));
        if (mutes > 0) parts.add (juce::String (mutes) + (mutes == 1 ? " mute" : " mutes"));

        lines.push_back ("The last 2 bars: " + juce::String (steps) + " steps of " + formatKnob (Knob::stepLength, s) + " ("
                         + formatMs (getStepMs (s.stepLength, in.bpm)) + " ms at " + formatBpm (in.bpm) + " BPM): "
                         + (parts.isEmpty() ? juce::String ("every step plays through") : parts.joinIntoString (", ")) + ".");
        return lines;
    }
} // namespace slc

//==============================================================================
namespace
{
    using slc::Knob;

    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    /** How a float parameter reads in the editor and to hosts: the same as formatKnob. */
    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float v, int)
            {
                slc::Settings s;
                s.smoothMs = s.mixPercent = s.ownBpm = v;
                return slc::formatKnob (k, s);
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                return text.retainCharacters ("-0123456789.").getFloatValue();
            });
    }

    constexpr double pi = juce::MathConstants<double>::pi;
}

const char* BuiltInSlicer::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::stepLength: return "step";
        case Knob::steps:      return "steps";
        case Knob::roll:       return "roll";
        case Knob::smooth:     return "smooth";
        case Knob::mix:        return "mix";
        case Knob::tempo:      return "tempo";
        case Knob::ownBpm:     return "ownbpm";
    }

    return "";
}

juce::String BuiltInSlicer::getModeParameterId (int step)
{
    return "mode" + juce::String (step + 1);
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInSlicer::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const slc::Settings d;
    juce::StringArray lengths, modes;

    for (auto& l : slc::stepLengths)
        lengths.add (l.name);

    for (int m = 0; m < slc::numModes; ++m)
        modes.add (slc::getModeName ((slc::Mode) m));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::stepLength), 1 }, "Step length", lengths, d.stepLength));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { getParameterId (Knob::steps), 1 }, "Steps", 1, slc::maxSteps, d.steps));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { getParameterId (Knob::roll), 1 }, "Roll", 1, slc::maxRoll, d.roll));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (Knob::smooth), 1 }, "Smooth",
                                                             logRange (slc::minSmooth, slc::maxSmooth), d.smoothMs, knobText (Knob::smooth)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (Knob::mix), 1 }, "Mix",
                                                             juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, d.mixPercent, knobText (Knob::mix)));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::tempo), 1 }, "Tempo source",
                                                              juce::StringArray { "Song", "Own" }, 0));   // Song
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (Knob::ownBpm), 1 }, "Own BPM",
                                                             juce::NormalisableRange<float> { slc::minOwnBpm, slc::maxOwnBpm, 0.1f }, d.ownBpm,
                                                             knobText (Knob::ownBpm)));

    for (int i = 0; i < slc::maxSteps; ++i)
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getModeParameterId (i), 1 }, "Step " + juce::String (i + 1), modes, 0));

    return layout;
}

BuiltInSlicer::BuiltInSlicer()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItSlicer", createLayout())
{
    for (int k = 0; k < slc::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    for (int i = 0; i < slc::maxSteps; ++i)
        modeValues[(size_t) i] = parameters.getRawParameterValue (getModeParameterId (i));
}

juce::PluginDescription BuiltInSlicer::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Rhythm";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44536c31;   // "DSl1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInSlicer::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInSlicer::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

juce::RangedAudioParameter* BuiltInSlicer::getModeParameter (int step) const
{
    return parameters.getParameter (getModeParameterId (juce::jlimit (0, slc::maxSteps - 1, step)));
}

slc::Mode BuiltInSlicer::getMode (int step) const
{
    return (slc::Mode) juce::jlimit (0, slc::numModes - 1, juce::roundToInt (modeValues[(size_t) juce::jlimit (0, slc::maxSteps - 1, step)]->load()));
}

slc::Settings BuiltInSlicer::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    slc::Settings s;
    s.stepLength = juce::jlimit (0, slc::numStepLengths - 1, juce::roundToInt (value (Knob::stepLength)));
    s.steps = juce::jlimit (1, slc::maxSteps, juce::roundToInt (value (Knob::steps)));
    s.roll = juce::jlimit (1, slc::maxRoll, juce::roundToInt (value (Knob::roll)));
    s.smoothMs = value (Knob::smooth);
    s.mixPercent = value (Knob::mix);
    s.ownTempo = value (Knob::tempo) >= 0.5f;
    s.ownBpm = value (Knob::ownBpm);

    for (int i = 0; i < slc::maxSteps; ++i)
        s.modes[(size_t) i] = getMode (i);

    return s;
}

void BuiltInSlicer::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInSlicer::setMode (int step, slc::Mode mode)
{
    if (auto* p = getModeParameter (step))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) (int) mode));
}

void BuiltInSlicer::setSettings (const slc::Settings& s)
{
    setKnob (Knob::stepLength, (float) s.stepLength);
    setKnob (Knob::steps, (float) s.steps);
    setKnob (Knob::roll, (float) s.roll);
    setKnob (Knob::smooth, s.smoothMs);
    setKnob (Knob::mix, s.mixPercent);
    setKnob (Knob::tempo, s.ownTempo ? 1.0f : 0.0f);
    setKnob (Knob::ownBpm, s.ownBpm);

    for (int i = 0; i < slc::maxSteps; ++i)
        setMode (i, s.modes[(size_t) i]);
}

//==============================================================================
bool BuiltInSlicer::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInSlicer::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    capacity = slc::getSliceCapacity (rate);

    // The only allocation: made here, never while the sound runs.
    for (auto& buffer : store)
        for (auto& channel : buffer)
            channel.assign ((size_t) capacity, 0.0f);

    recIdx = 0;
    heldIdx = 1;
    stepClock.reset();
    barActive = false;
    setLatencySamples (0);
    clap.prepare (rate);
    resetState();
    history.clear();
}

void BuiltInSlicer::resetState()
{
    cur = Voice {};
    old = Voice {};
    fading = false;
    fadePos = fadeLength = 0;
    stepActive = stepComplete = runHasSlice = lastComplete = usesSlice = false;
    recCount = lastLength = heldLength = tInStep = pieceCount = 0;
    lastInPeak = heldPeak = 0.0f;
    meterSamples = 0;
    playingSliced = false;
}

float BuiltInSlicer::readVoice (const Voice& v, int channel, const float* in) const
{
    switch (v.kind)
    {
        case Voice::Kind::silent:  return 0.0f;
        case Voice::Kind::input:   return in[channel];
        case Voice::Kind::forward: return v.length > 0 ? v.data[(size_t) channel][juce::jmin (v.pos, v.length - 1)] : 0.0f;
        case Voice::Kind::reverse: return v.length > 0 ? v.data[(size_t) channel][juce::jmax (0, v.length - 1 - v.pos)] : 0.0f;
    }

    return 0.0f;
}

void BuiltInSlicer::startFade (const Voice& next, int pieceLength, float smoothMs)
{
    const int length = juce::jmin (juce::roundToInt (smoothMs * 0.001 * rate), pieceLength / 4);

    if (length >= 1)
    {
        // The sound before carries on while it fades out; at the end of a slice it holds its last sample, so the join never jumps.
        old = cur;
        fading = true;
        fadePos = 0;
        fadeLength = length;
    }
    else
    {
        fading = false;
    }

    cur = next;
}

void BuiltInSlicer::finishStep()
{
    stepActive = false;
    lastComplete = stepComplete && recCount > 0;
    lastLength = recCount;
    lastInPeak = meterInPeak;

    if (stepComplete && meterSamples > 0)
    {
        slc::StepRecord r;
        r.step = stepIndex;
        r.mode = stepMode;
        r.sliced = usesSlice;
        r.inPeak = meterInPeak;
        r.outPeak = meterOutPeak;
        r.inRms = (float) std::sqrt (meterInSq / meterSamples);
        r.outRms = (float) std::sqrt (meterOutSq / meterSamples);
        r.slicePeak = usesSlice ? heldPeak : 0.0f;
        r.beat = stepBeatStart;
        history.push (r);
    }
}

void BuiltInSlicer::beginStep (long long step, double phaseSamples, const slc::Settings& s, double stepSamples)
{
    if (stepActive)
        finishStep();

    const int n = juce::jlimit (1, slc::maxSteps, s.steps);
    stepAbs = step;
    stepActive = true;
    stepComplete = phaseSamples < 0.5;
    stepIndex = (int) (((step % n) + n) % n);
    stepMode = s.modes[(size_t) stepIndex];
    stepBeatStart = (double) step * slc::getStepBeats (s.stepLength);
    tInStep = juce::roundToInt (phaseSamples);
    pieceCount = tInStep;
    usesSlice = false;

    // A run of Repeat and Reverse steps plays one slice: the step just before the run (or, when that wasn't whole, the first step after it that has one).
    const bool slicing = stepMode == slc::Mode::repeat || stepMode == slc::Mode::reverse;

    if (slicing && ! runHasSlice && lastComplete && lastLength > 0)
    {
        heldIdx = recIdx;
        recIdx = 1 - recIdx;
        heldLength = lastLength;
        heldPeak = lastInPeak;
        runHasSlice = true;
    }
    else if (! slicing)
    {
        runHasSlice = false;
    }

    usesSlice = slicing && runHasSlice;
    lastComplete = false;
    recCount = 0;
    meterSamples = 0;
    meterInSq = meterOutSq = 0.0;
    meterInPeak = meterOutPeak = 0.0f;

    Voice next;
    int pieceLength = juce::jmax (1, (int) std::round (stepSamples));

    if (stepMode == slc::Mode::mute)
    {
        next.kind = Voice::Kind::silent;
    }
    else if (usesSlice)
    {
        next.kind = stepMode == slc::Mode::repeat ? Voice::Kind::forward : Voice::Kind::reverse;
        next.data = { store[heldIdx][0].data(), store[heldIdx][1].data() };
        next.length = heldLength;
        next.changes = true;
        pieceLength = heldLength;

        if (stepMode == slc::Mode::repeat)
        {
            next.piece = juce::jmax (1, heldLength / s.roll);
            pieceLength = next.piece;
        }
    }
    else
    {
        next.kind = Voice::Kind::input;
    }

    const bool sameSource = (next.kind == Voice::Kind::input && cur.kind == Voice::Kind::input)
                            || (next.kind == Voice::Kind::silent && cur.kind == Voice::Kind::silent);

    if (sameSource)
    {
        cur = next;
        fading = false;
    }
    else
    {
        startFade (next, pieceLength, s.smoothMs);
    }

    playingStep = stepIndex;
    playingSliced = usesSlice;
}

void BuiltInSlicer::renderRange (juce::AudioBuffer<float>& buffer, int from, int to, int numChannels, const slc::Settings& s, bool listen)
{
    const float m = mixNow;

    for (int i = from; i < to; ++i)
    {
        // A repeated piece starts again.
        if (cur.kind == Voice::Kind::forward && cur.piece > 0 && pieceCount >= cur.piece)
        {
            Voice again = cur;
            again.pos = 0;
            startFade (again, cur.piece, s.smoothMs);
            pieceCount = 0;
        }

        std::array<float, 2> in {};

        for (int ch = 0; ch < numChannels; ++ch)
            in[(size_t) ch] = buffer.getSample (ch, i);

        if (recCount < capacity)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                store[recIdx][(size_t) ch][(size_t) recCount] = in[(size_t) ch];

            ++recCount;
        }

        float w = 1.0f;

        if (fading)
            w = 0.5f - 0.5f * (float) std::cos (pi * (double) fadePos / (double) fadeLength);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float wet = readVoice (cur, ch, in.data()), changed = cur.changes ? wet : 0.0f;

            if (fading)
            {
                const float before = readVoice (old, ch, in.data());
                wet = (1.0f - w) * before + w * wet;
                changed = (1.0f - w) * (old.changes ? before : 0.0f) + w * changed;
            }

            const float mixed = (1.0f - m) * in[(size_t) ch] + m * wet;
            buffer.setSample (ch, i, listen ? changed : mixed);

            meterInPeak = juce::jmax (meterInPeak, std::abs (in[(size_t) ch]));
            meterOutPeak = juce::jmax (meterOutPeak, std::abs (mixed));
            meterInSq += (double) in[(size_t) ch] * in[(size_t) ch];
            meterOutSq += (double) mixed * mixed;
        }

        if (cur.kind == Voice::Kind::forward || cur.kind == Voice::Kind::reverse)
            ++cur.pos;

        if (fading)
        {
            if (old.kind == Voice::Kind::forward || old.kind == Voice::Kind::reverse)
                ++old.pos;

            if (++fadePos >= fadeLength)
                fading = false;
        }

        ++tInStep;
        ++pieceCount;
        meterSamples += numChannels;
    }
}

void BuiltInSlicer::startTestBar()
{
    barActive = true;
    barBeat = 0.0;
    barNoise.setSeed (11);
    barHatState = 0.0f;
}

void BuiltInSlicer::addTestBar (juce::AudioBuffer<float>& buffer, int numChannels, double beatsPerSample, double bpm)
{
    const double secondsPerBeat = 60.0 / bpm;
    const double twoPi = 2.0 * pi;

    for (int i = 0; i < buffer.getNumSamples() && barActive; ++i, barBeat += beatsPerSample)
    {
        if (barBeat >= 4.0)
        {
            barActive = false;
            break;
        }

        double v = 0.0;

        // Kick on beats 1 and 3: a sine that falls from 150 to 50 Hz.
        for (const double beat : { 0.0, 2.0 })
        {
            const double t = (barBeat - beat) * secondsPerBeat;

            if (t >= 0.0 && t < 0.6)
                v += 0.65 * std::exp (-t / 0.12) * std::min (1.0, t / 0.001) * std::sin (twoPi * (50.0 * t + 100.0 * 0.03 * (1.0 - std::exp (-t / 0.03))));
        }

        // Snare on beats 2 and 4: noise and a 180 Hz body.
        const float noise = barNoise.nextFloat() * 2.0f - 1.0f;
        barHatState += 0.3f * (noise - barHatState);
        const float bright = noise - barHatState;

        for (const double beat : { 1.0, 3.0 })
        {
            const double t = (barBeat - beat) * secondsPerBeat;

            if (t >= 0.0 && t < 0.5)
                v += 0.3 * std::exp (-t / 0.08) * noise + 0.25 * std::exp (-t / 0.06) * std::sin (twoPi * 180.0 * t);
        }

        // Closed hat on every eighth.
        {
            const double sinceEighth = (barBeat - 0.5 * std::floor (barBeat / 0.5)) * secondsPerBeat;
            v += 0.15 * std::exp (-sinceEighth / 0.02) * bright;
        }

        // A quiet A minor chord (A3, C4, E4) held for the bar, about 18 dB under the drums.
        const double fade = std::min ({ 1.0, barBeat * secondsPerBeat / 0.005, (4.0 - barBeat) * secondsPerBeat / 0.005 });
        const double t = barBeat * secondsPerBeat;

        for (const double hz : { 220.0, 261.63, 329.63 })
            v += 0.035 * fade * std::sin (twoPi * hz * t);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addSample (ch, i, (float) v);
    }
}

void BuiltInSlicer::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (numChannels == 0 || capacity == 0 || total == 0)
        return;

    const auto s = getSettings();

    // The clock: the song's, when the song plays and gives its position; otherwise the slicer's own.
    auto block = stepClock.begin (getPlayHead(), s.ownTempo, (double) s.ownBpm, rate);
    tempoBpm = block.hostBpm;
    const double bpm = block.bpm;
    const double beatsPerSample = block.beatsPerSample;
    usedBpm = bpm;
    const bool songBased = block.songBased;

    // A request for the test bar starts the slicer's own clock at step 1.
    const bool testBar = clap.takeRequest();

    if (testBar)
    {
        startTestBar();
        stepClock.restartOwn (block);
    }

    const double beat = block.beat;
    const bool jump = block.jump;

    if (jump)
        resetState();

    stepClock.commit (block);
    usingSongClock = songBased;
    clockBeat = beat;

    if (barActive)
        addTestBar (buffer, numChannels, beatsPerSample, bpm);

    // The steps in this block.
    const double stepBeats = slc::getStepBeats (s.stepLength);
    const double stepSamples = stepBeats / beatsPerSample;
    const double x0 = beat / stepBeats;
    long long k0 = (long long) std::floor (x0);
    double phase = (x0 - (double) k0) * stepSamples;

    if (stepSamples - phase < 0.5)
    {
        ++k0;
        phase = 0.0;
    }
    else if (phase < 0.5)
    {
        phase = 0.0;
    }

    if (! stepActive || k0 != stepAbs)
        beginStep (k0, phase, s, stepSamples);

    const bool listen = listenToEffect.load();
    mixNow = juce::jlimit (0.0f, 1.0f, s.mixPercent / 100.0f);
    double nextBoundary = stepSamples - phase;
    int pos = 0;

    while (pos < total)
    {
        const int end = juce::jmin (total, (int) std::ceil (nextBoundary - 1.0e-6));

        if (end > pos)
        {
            renderRange (buffer, pos, end, numChannels, s, listen);
            pos = end;
        }

        if (pos < total)
        {
            beginStep (stepAbs + 1, 0.0, s, stepSamples);
            nextBoundary += stepSamples;
        }
    }

    stepClock.finish (block, total);

    if (numChannels == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, total);
}

juce::AudioProcessorEditor* BuiltInSlicer::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInSlicer::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInSlicer::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
