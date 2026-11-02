#include "BuiltInStepFilter.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace stf
{
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

        juce::String formatHeight (float height)
        {
            auto text = juce::String (height, 1);
            return text.endsWith (".0") ? text.dropLastCharacters (2) : text;
        }

        double toDb (double linear)
        {
            return 20.0 * std::log10 (juce::jmax (1.0e-5, linear));
        }
    }

    juce::String getTypeName (FilterType t)
    {
        switch (t)
        {
            case FilterType::lowPass:  return "Low-pass";
            case FilterType::bandPass: return "Band-pass";
            case FilterType::highPass: return "High-pass";
        }

        return {};
    }

    double getHeightCutoffHz (double lowHz, double highHz, double heightPercent)
    {
        const double lo = juce::jmax (1.0, juce::jmin (lowHz, highHz)), hi = juce::jmax (1.0, juce::jmax (lowHz, highHz));
        return lo * std::pow (hi / lo, juce::jlimit (0.0, 100.0, heightPercent) / 100.0);
    }

    double getStepCutoffHz (const Settings& s, int step)
    {
        return getHeightCutoffHz (s.lowHz, s.highHz, s.heights[(size_t) juce::jlimit (0, maxSteps - 1, step)]);
    }

    double clampCutoff (double hz, double sampleRate)
    {
        return juce::jlimit ((double) minCutoff, juce::jmax ((double) minCutoff, sampleRate * maxCutoffShare), hz);
    }

    double getGlideMs (const Settings& s, double bpm)
    {
        return juce::jmax (minGlideMs, juce::jlimit (0.0f, 100.0f, s.glidePercent) / 100.0 * getStepMs (s.stepLength, bpm));
    }

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    juce::String formatKnob (Knob knob, const Settings& s)
    {
        switch (knob)
        {
            case Knob::stepLength: return stepLengths[juce::jlimit (0, numStepLengths - 1, s.stepLength)].name;
            case Knob::steps:      return juce::String (s.steps);
            case Knob::tempo:      return s.ownTempo ? "Own" : "Song";
            case Knob::ownBpm:     return formatBpm (s.ownBpm) + " BPM";
            case Knob::type:       return getTypeName (s.type);
            case Knob::slope:      return s.steep ? "24 dB" : "12 dB";
            case Knob::lowCutoff:  return formatHz (s.lowHz);
            case Knob::highCutoff: return formatHz (s.highHz);
            case Knob::resonance:  return juce::String (juce::roundToInt (s.resonancePercent)) + "%";
            case Knob::glide:      return juce::String (juce::roundToInt (s.glidePercent)) + "%";
            case Knob::mix:        return juce::String (juce::roundToInt (s.mixPercent)) + "%";
            case Knob::output:     return fx::formatNumber (s.outputDb, 1) + " dB";
        }

        return {};
    }

    juce::String formatKnobValue (Knob knob, float value)
    {
        Settings s;

        switch (knob)
        {
            case Knob::stepLength: s.stepLength = juce::roundToInt (value); break;
            case Knob::steps:      s.steps = juce::roundToInt (value); break;
            case Knob::tempo:      s.ownTempo = value >= 0.5f; break;
            case Knob::ownBpm:     s.ownBpm = value; break;
            case Knob::type:       s.type = (FilterType) juce::jlimit (0, filt::numFilterTypes - 1, juce::roundToInt (value)); break;
            case Knob::slope:      s.steep = value >= 0.5f; break;
            case Knob::lowCutoff:  s.lowHz = value; break;
            case Knob::highCutoff: s.highHz = value; break;
            case Knob::resonance:  s.resonancePercent = value; break;
            case Knob::glide:      s.glidePercent = value; break;
            case Knob::mix:        s.mixPercent = value; break;
            case Knob::output:     s.outputDb = value; break;
        }

        return formatKnob (knob, s);
    }

    juce::String patternToText (const Settings& s)
    {
        juce::StringArray heights;

        for (auto h : s.heights)
            heights.add (formatHeight (h));

        return "DawIt Step Filter pattern: " + juce::String (stepLengths[juce::jlimit (0, numStepLengths - 1, s.stepLength)].name) + " "
               + juce::String (s.steps) + " " + heights.joinIntoString (" ");
    }

    bool patternFromText (const juce::String& text, Settings& s)
    {
        const auto tokens = juce::StringArray::fromTokens (text.trim(), " \t\r\n", "");

        if (tokens.size() != 6 + maxSteps || ! tokens[0].equalsIgnoreCase ("DawIt") || ! tokens[1].equalsIgnoreCase ("Step")
            || ! tokens[2].equalsIgnoreCase ("Filter") || ! tokens[3].equalsIgnoreCase ("pattern:"))
            return false;

        int length = -1;

        for (int i = 0; i < numStepLengths; ++i)
            if (tokens[4].equalsIgnoreCase (stepLengths[i].name))
                length = i;

        const int steps = tokens[5].getIntValue();

        if (length < 0 || ! tokens[5].containsOnly ("0123456789") || steps < 1 || steps > maxSteps)
            return false;

        std::array<float, maxSteps> heights {};

        for (int i = 0; i < maxSteps; ++i)
        {
            const auto& t = tokens[6 + i];
            const float h = t.getFloatValue();

            if (t.isEmpty() || ! t.containsOnly ("0123456789.") || h < 0.0f || h > 100.0f)
                return false;

            heights[(size_t) i] = h;
        }

        s.stepLength = length;
        s.steps = steps;
        s.heights = heights;
        return true;
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;

            auto add = [&list] (juce::String name, juce::String why, Settings s, const std::vector<float>& heights)
            {
                for (size_t i = 0; i < heights.size() && i < (size_t) maxSteps; ++i)
                    s.heights[i] = heights[i];

                list.push_back ({ std::move (name), std::move (why), s });
            };

            add ("Neutral: every step open",
                 "Every step is open, so you hear the track almost unchanged. Drag a step down to hear it darken.", Settings {}, {});

            Settings pulse;
            pulse.stepLength = 4;   // 1/8
            pulse.steps = 8;
            pulse.lowHz = 400.0f;
            pulse.highHz = 4000.0f;
            pulse.resonancePercent = 20.0f;
            pulse.glidePercent = 15.0f;
            add ("Eighth-note pulse",
                 "Low and high cutoffs take turns on eighth notes, so a held chord pulses between dark and bright while it keeps sounding. A little Resonance makes the bright steps shine; Glide 15% takes the edge off each change.",
                 pulse, { 0, 100, 0, 100, 0, 100, 0, 100 });

            Settings acid;
            acid.lowHz = 200.0f;
            acid.highHz = 3200.0f;
            acid.resonancePercent = 65.0f;
            acid.glidePercent = 5.0f;
            acid.outputDb = -9.0f;
            add ("Acid steps",
                 "High resonance makes each sixteenth's cutoff sing like a note: the squelch of an acid bassline. Best on a saw bass or lead. Output -9 dB makes up for the resonance peak.",
                 acid, { 0, 50, 25, 75, 0, 100, 25, 50, 0, 75, 25, 100, 0, 50, 75, 25 });

            Settings talk;
            talk.stepLength = 4;
            talk.steps = 8;
            talk.type = FilterType::bandPass;
            talk.steep = false;
            talk.lowHz = 400.0f;
            talk.highHz = 2500.0f;
            talk.resonancePercent = 40.0f;
            talk.glidePercent = 50.0f;
            add ("Talking band-pass",
                 "A band moving between 400 Hz and 2.5 kHz sounds like a voice changing vowels. Glide 50% makes it talk rather than click. Try it on a copy of a vocal or a guitar.",
                 talk, { 0, 60, 30, 100, 0, 60, 30, 100 });

            Settings rise;
            rise.stepLength = 5;   // 1/4
            rise.type = FilterType::highPass;
            rise.steep = false;
            rise.lowHz = 30.0f;
            rise.highHz = 2000.0f;
            rise.resonancePercent = 25.0f;
            rise.glidePercent = 100.0f;
            std::vector<float> rising;

            for (int i = 0; i < maxSteps; ++i)
                rising.push_back ((float) i * 100.0f / (float) (maxSteps - 1));

            add ("Build-up rise",
                 "Over four bars the high-pass climbs from 30 Hz to 2 kHz and the bass drains out, so the next section lands harder. The pattern restarts every four bars from bar 1, so start the build-up on bar 1, 5, 9 and so on.",
                 rise, rising);

            Settings pad;
            pad.stepLength = 4;
            pad.steps = 8;
            pad.steep = false;
            pad.lowHz = 600.0f;
            pad.highHz = 3000.0f;
            pad.resonancePercent = 15.0f;
            pad.glidePercent = 60.0f;
            add ("Soft pad movement",
                 "A gentle, breathing change of tone under a pad. 12 dB keeps it open; long Glide makes the steps flow into each other.",
                 pad, { 100, 40, 70, 20, 100, 50, 80, 30 });

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
        slot.values[1] = r.cutoffHz;
        slot.values[2] = r.inPeak;
        slot.values[3] = r.inRms;
        slot.values[4] = r.outPeak;
        slot.values[5] = r.outRms;
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
            r.cutoffHz = slot.values[1].load();
            r.inPeak = slot.values[2].load();
            r.inRms = slot.values[3].load();
            r.outPeak = slot.values[4].load();
            r.outRms = slot.values[5].load();
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
        const auto& s = in.settings;
        const int steps = juce::jlimit (1, maxSteps, s.steps);

        // F0: nothing heard.
        const bool heard = std::any_of (in.records.begin(), in.records.end(), [&] (auto& r) { return toDb (r.inPeak) >= -60.0; });

        if (! heard)
            return { "Play the track, or click Play a bar, to see it here." };

        std::vector<juce::String> lines;

        // The cutoffs of the steps in use.
        double lowestHz = 1.0e9, highestHz = 0.0, logSum = 0.0;

        for (int i = 0; i < steps; ++i)
        {
            const double hz = getStepCutoffHz (s, i);
            lowestHz = juce::jmin (lowestHz, hz);
            highestHz = juce::jmax (highestHz, hz);
            logSum += std::log2 (hz);
        }

        const double meanHz = std::exp2 (logSum / steps);
        const bool nothingMoves = std::log2 (highestHz / lowestHz) <= 1.0 / 12.0 + 1.0e-9;

        // What each step did to the level: its input and output energy over the records, as dB, for steps that had sound.
        struct StepLevel { double inEnergy = 0.0, outEnergy = 0.0; int count = 0; };
        std::array<StepLevel, maxSteps> perStep {};
        double loudestGainDb = -1000.0;

        for (auto& r : in.records)
        {
            if (r.step < 0 || r.step >= maxSteps || toDb (r.inRms) < -60.0)
                continue;

            auto& l = perStep[(size_t) r.step];
            l.inEnergy += (double) r.inRms * r.inRms;
            l.outEnergy += (double) r.outRms * r.outRms;
            ++l.count;

            if (toDb (r.inPeak) >= -60.0)
                loudestGainDb = juce::jmax (loudestGainDb, toDb (r.outPeak) - toDb (r.inPeak));
        }

        std::vector<std::pair<int, double>> changes;   // step, output minus input in dB

        for (int i = 0; i < steps; ++i)
            if (perStep[(size_t) i].count > 0)
                changes.push_back ({ i, 10.0 * std::log10 (juce::jmax (1.0e-12, perStep[(size_t) i].outEnergy) / perStep[(size_t) i].inEnergy) });

        // F1: no song tempo.
        if (! s.ownTempo && ! in.hostHasTempo)
            lines.push_back ("No song tempo reaches DawIt Step Filter here, so it steps at its own " + formatBpm (s.ownBpm)
                             + " BPM. If the track has a tempo, choose Own and set it, or the steps will drift against the beat.");

        // F2: nothing moves.
        if (nothingMoves)
            lines.push_back ("Every step has the same cutoff (" + formatHz (meanHz) + "), so the tone doesn't move. Drag some steps up or down, or turn Low cutoff and High cutoff apart.");

        // F3: hard to hear.
        if (! nothingMoves && changes.size() >= 2)
        {
            double lo = 1000.0, hi = -1000.0;

            for (auto& c : changes)
            {
                lo = juce::jmin (lo, c.second);
                hi = juce::jmax (hi, c.second);
            }

            if (hi - lo < 1.5)
                lines.push_back ("The steps differ in what they do to this track's level by at most " + juce::String (hi - lo, 1) + " dB, so the pattern may be hard to hear. The track may have little sound between "
                                 + formatHz (lowestHz) + " and " + formatHz (highestHz)
                                 + ": try a lower Low cutoff (low-pass) or a higher High cutoff (high-pass), or add Resonance.");
        }

        // F4: louder.
        if (loudestGainDb > 6.0)
        {
            const auto x = juce::String (juce::roundToInt (loudestGainDb));
            lines.push_back ("At its loudest the filter makes the track " + x + " dB louder than it came in, from the resonance peak. Turn Output down by about " + x
                             + " dB, or lower Resonance, so nothing after it clips.");
        }

        // F5: a step that leaves almost nothing, one line each, at most three.
        int shown = 0;

        for (auto& c : changes)
            if (c.second <= -30.0 && shown++ < 3)
                lines.push_back ("Step " + juce::String (c.first + 1) + " takes the track down by " + juce::String (juce::roundToInt (-c.second))
                                 + " dB, almost to silence. That works as a gate-like pulse; for a softer one, raise that step or Low cutoff on a low-pass, or lower that step or High cutoff on a high-pass.");

        // F6: summary.
        juce::String where = nothingMoves ? " at " + formatHz (meanHz) : " moving between " + formatHz (lowestHz) + " and " + formatHz (highestHz);
        juce::String level;

        if (! changes.empty())
        {
            double smallest = 1000.0, largest = -1000.0;

            for (auto& c : changes)
            {
                smallest = juce::jmin (smallest, c.second);
                largest = juce::jmax (largest, c.second);
            }

            // Signed, so a boost and a cut are told apart; one number when the rounded ends agree.
            auto signedDb = [] (double v)
            {
                const int n = juce::roundToInt (v);
                return (n > 0 ? juce::String ("+") : n < 0 ? juce::String::fromUTF8 ("\xe2\x88\x92") : juce::String()) + juce::String (std::abs (n));
            };

            level = juce::roundToInt (smallest) == juce::roundToInt (largest)
                        ? "; the steps change the level by " + signedDb (smallest) + " dB"
                        : "; the steps change the level from " + signedDb (smallest) + " to " + signedDb (largest) + " dB";
        }

        lines.push_back ("The last 2 bars: " + juce::String (steps) + " steps of " + formatKnob (Knob::stepLength, s) + " ("
                         + formatMs (getStepMs (s.stepLength, in.bpm)) + " ms at " + formatBpm (in.bpm) + " BPM), a "
                         + getTypeName (s.type).toLowerCase() + where + level + ".");
        return lines;
    }
} // namespace stf

//==============================================================================
namespace
{
    using stf::Knob;

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
            .withStringFromValueFunction ([k] (float v, int) { return stf::formatKnobValue (k, v); })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                auto value = text.retainCharacters ("-0123456789.").getFloatValue();

                if ((k == Knob::lowCutoff || k == Knob::highCutoff) && text.containsIgnoreCase ("k"))
                    value *= 1000.0f;

                return value;
            });
    }

    juce::AudioParameterFloatAttributes heightText()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; })
            .withValueFromStringFunction ([] (const juce::String& text) { return text.retainCharacters ("-0123456789.").getFloatValue(); });
    }

    constexpr double pi = juce::MathConstants<double>::pi;

    /** The part of a saw wave's step that a band-limited saw corrects (polyBLEP), so the test chord doesn't alias. */
    double polyBlep (double t, double dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }

        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }

        return 0.0;
    }

    /** The test chord: A2, E3, A3, C4 and E4, as saw waves. */
    constexpr std::array<double, 5> chordHz { 110.0, 164.81, 220.0, 261.63, 329.63 };
}

const char* BuiltInStepFilter::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::stepLength: return "step";
        case Knob::steps:      return "steps";
        case Knob::tempo:      return "tempo";
        case Knob::ownBpm:     return "ownbpm";
        case Knob::type:       return "type";
        case Knob::slope:      return "slope";
        case Knob::lowCutoff:  return "low";
        case Knob::highCutoff: return "high";
        case Knob::resonance:  return "resonance";
        case Knob::glide:      return "glide";
        case Knob::mix:        return "mix";
        case Knob::output:     return "output";
    }

    return "";
}

juce::String BuiltInStepFilter::getHeightParameterId (int step)
{
    return "height" + juce::String (step + 1);
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInStepFilter::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const stf::Settings d;
    juce::StringArray lengths, types;

    for (auto& l : stf::stepLengths)
        lengths.add (l.name);

    for (int t = 0; t < filt::numFilterTypes; ++t)
        types.add (stf::getTypeName ((filt::FilterType) t));

    auto id = [] (Knob k) { return juce::ParameterID { getParameterId (k), 1 }; };
    auto percent = [] (float interval) { return juce::NormalisableRange<float> { 0.0f, 100.0f, interval }; };

    layout.add (std::make_unique<juce::AudioParameterChoice> (id (Knob::stepLength), "Step length", lengths, d.stepLength));
    layout.add (std::make_unique<juce::AudioParameterInt> (id (Knob::steps), "Steps", 1, stf::maxSteps, d.steps));
    layout.add (std::make_unique<juce::AudioParameterChoice> (id (Knob::tempo), "Tempo source", juce::StringArray { "Song", "Own" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::ownBpm), "Own BPM", juce::NormalisableRange<float> { stf::minOwnBpm, stf::maxOwnBpm, 0.1f },
                                                             d.ownBpm, knobText (Knob::ownBpm)));
    layout.add (std::make_unique<juce::AudioParameterChoice> (id (Knob::type), "Type", types, (int) d.type));
    layout.add (std::make_unique<juce::AudioParameterChoice> (id (Knob::slope), "Slope", juce::StringArray { "12 dB", "24 dB" }, (int) d.steep));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::lowCutoff), "Low cutoff", logRange (stf::minCutoff, stf::maxCutoff), d.lowHz, knobText (Knob::lowCutoff)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::highCutoff), "High cutoff", logRange (stf::minCutoff, stf::maxCutoff), d.highHz, knobText (Knob::highCutoff)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::resonance), "Resonance", percent (1.0f), d.resonancePercent, knobText (Knob::resonance)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::glide), "Glide", percent (1.0f), d.glidePercent, knobText (Knob::glide)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::mix), "Mix", percent (1.0f), d.mixPercent, knobText (Knob::mix)));
    layout.add (std::make_unique<juce::AudioParameterFloat> (id (Knob::output), "Output", juce::NormalisableRange<float> { stf::minOutputDb, stf::maxOutputDb, 0.1f },
                                                             d.outputDb, knobText (Knob::output)));

    for (int i = 0; i < stf::maxSteps; ++i)
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getHeightParameterId (i), 1 }, "Step " + juce::String (i + 1),
                                                                 juce::NormalisableRange<float> { 0.0f, 100.0f }, d.heights[(size_t) i], heightText()));

    return layout;
}

BuiltInStepFilter::BuiltInStepFilter()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItStepFilter", createLayout())
{
    for (int k = 0; k < stf::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    for (int i = 0; i < stf::maxSteps; ++i)
        heightValues[(size_t) i] = parameters.getRawParameterValue (getHeightParameterId (i));
}

juce::PluginDescription BuiltInStepFilter::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Filter";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44537466;   // "DStf"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInStepFilter::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInStepFilter::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

juce::RangedAudioParameter* BuiltInStepFilter::getHeightParameter (int step) const
{
    return parameters.getParameter (getHeightParameterId (juce::jlimit (0, stf::maxSteps - 1, step)));
}

float BuiltInStepFilter::getHeight (int step) const
{
    return juce::jlimit (0.0f, 100.0f, heightValues[(size_t) juce::jlimit (0, stf::maxSteps - 1, step)]->load());
}

stf::Settings BuiltInStepFilter::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    stf::Settings s;
    s.stepLength = juce::jlimit (0, stf::numStepLengths - 1, juce::roundToInt (value (Knob::stepLength)));
    s.steps = juce::jlimit (1, stf::maxSteps, juce::roundToInt (value (Knob::steps)));
    s.ownTempo = value (Knob::tempo) >= 0.5f;
    s.ownBpm = value (Knob::ownBpm);
    s.type = (filt::FilterType) juce::jlimit (0, filt::numFilterTypes - 1, juce::roundToInt (value (Knob::type)));
    s.steep = value (Knob::slope) >= 0.5f;
    s.lowHz = value (Knob::lowCutoff);
    s.highHz = value (Knob::highCutoff);
    s.resonancePercent = value (Knob::resonance);
    s.glidePercent = value (Knob::glide);
    s.mixPercent = value (Knob::mix);
    s.outputDb = value (Knob::output);

    for (int i = 0; i < stf::maxSteps; ++i)
        s.heights[(size_t) i] = getHeight (i);

    return s;
}

void BuiltInStepFilter::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInStepFilter::setHeight (int step, float percent)
{
    if (auto* p = getHeightParameter (step))
        p->setValueNotifyingHost (p->convertTo0to1 (percent));
}

void BuiltInStepFilter::setSettings (const stf::Settings& s)
{
    setKnob (Knob::stepLength, (float) s.stepLength);
    setKnob (Knob::steps, (float) s.steps);
    setKnob (Knob::tempo, s.ownTempo ? 1.0f : 0.0f);
    setKnob (Knob::ownBpm, s.ownBpm);
    setKnob (Knob::type, (float) (int) s.type);
    setKnob (Knob::slope, s.steep ? 1.0f : 0.0f);
    setKnob (Knob::lowCutoff, s.lowHz);
    setKnob (Knob::highCutoff, s.highHz);
    setKnob (Knob::resonance, s.resonancePercent);
    setKnob (Knob::glide, s.glidePercent);
    setKnob (Knob::mix, s.mixPercent);
    setKnob (Knob::output, s.outputDb);

    for (int i = 0; i < stf::maxSteps; ++i)
        setHeight (i, s.heights[(size_t) i]);
}

//==============================================================================
bool BuiltInStepFilter::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInStepFilter::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    stepClock.reset();
    chordActive = false;
    setLatencySamples (0);
    clap.prepare (rate);
    outputGain.reset (rate, 0.02);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().outputDb));
    cutoffInit = false;
    wasSteep = getSettings().steep;

    for (auto& f : first)
        f.reset();

    for (auto& f : second)
        f.reset();

    resetState();
    history.clear();
}

void BuiltInStepFilter::resetState()
{
    stepKnown = stepComplete = false;
    meterSamples = 0;
    chunkIn = chunkChange = 0.0f;
    chunkCount = 0;
}

void BuiltInStepFilter::finishStep()
{
    if (stepComplete && meterSamples > 0)
    {
        stf::StepRecord r;
        r.step = stepIndex;
        r.cutoffHz = (float) std::exp2 (targetOct);
        r.inPeak = meterInPeak;
        r.outPeak = meterOutPeak;
        r.inRms = (float) std::sqrt (meterInSq / meterSamples);
        r.outRms = (float) std::sqrt (meterOutSq / meterSamples);
        r.beat = stepBeatStart;
        history.push (r);
    }

    stepComplete = false;
}

void BuiltInStepFilter::beginStep (long long step, bool jumped, const stf::Settings& s, double stepSamples)
{
    if (stepKnown)
    {
        if (jumped)
            stepComplete = false;   // the step was cut short: its levels would mislead

        finishStep();
    }

    const int n = juce::jlimit (1, stf::maxSteps, s.steps);
    stepAbs = step;
    stepKnown = true;
    stepComplete = ! jumped && cutoffInit;
    stepIndex = (int) (((step % n) + n) % n);
    stepBeatStart = (double) step * stf::getStepBeats (s.stepLength);
    meterSamples = 0;
    meterInSq = meterOutSq = 0.0;
    meterInPeak = meterOutPeak = 0.0f;

    // The cutoff moves from where it is to the step's value, straight in octaves.
    const double target = std::log2 (stf::clampCutoff (stf::getStepCutoffHz (s, stepIndex), rate));
    const int minSamples = juce::jmax (1, (int) std::ceil (stf::minGlideMs * 0.001 * rate));

    if (! cutoffInit)
    {
        currentOct = startOct = targetOct = target;
        glidePos = glideLength = 1;
        cutoffInit = true;
    }
    else
    {
        startOct = currentOct;
        targetOct = target;
        glidePos = 0;
        glideLength = jumped ? minSamples : juce::jmax (minSamples, (int) std::round (juce::jlimit (0.0f, 100.0f, s.glidePercent) / 100.0 * stepSamples));
    }

    playingStep = stepIndex;
}

void BuiltInStepFilter::startChord()
{
    chordActive = true;
    chordBeat = 0.0;
    chordPhase.fill (0.0);
}

void BuiltInStepFilter::addChord (juce::AudioBuffer<float>& buffer, int numChannels, double beatsPerSample)
{
    const double secondsPerBeat = beatsPerSample > 0.0 ? 1.0 / (beatsPerSample * rate) : 1.0;

    for (int i = 0; i < buffer.getNumSamples() && chordActive; ++i, chordBeat += beatsPerSample)
    {
        if (chordBeat >= 4.0)
        {
            chordActive = false;
            break;
        }

        double v = 0.0;

        for (size_t n = 0; n < chordHz.size(); ++n)
        {
            const double dt = chordHz[n] / rate;
            auto& phase = chordPhase[n];
            v += 2.0 * phase - 1.0 - polyBlep (phase, dt);
            phase += dt;
            phase -= std::floor (phase);
        }

        // 0.05 each: five saws peak at 0.25, about -12 dBFS, with a 5 ms fade at each end.
        const double fade = std::min ({ 1.0, chordBeat * secondsPerBeat / 0.005, (4.0 - chordBeat) * secondsPerBeat / 0.005 });
        v *= 0.05 * fade;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addSample (ch, i, (float) v);
    }
}

void BuiltInStepFilter::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (numChannels == 0 || total == 0)
        return;

    const auto s = getSettings();

    // The clock: the song's, when the song plays and gives its position; otherwise the step filter's own.
    auto block = stepClock.begin (getPlayHead(), s.ownTempo, (double) s.ownBpm, rate);
    tempoBpm = block.hostBpm;
    usedBpm = block.bpm;

    // A request for the test chord starts the own clock at step 1.
    if (clap.takeRequest())
    {
        startChord();
        stepClock.restartOwn (block);
    }

    stepClock.commit (block);
    usingSongClock = block.songBased;
    clockBeat = block.beat;

    if (chordActive)
        addChord (buffer, numChannels, block.beatsPerSample);

    const double stepBeats = stf::getStepBeats (s.stepLength);
    const double invStepBeats = 1.0 / stepBeats;
    const double stepSamples = stepBeats / block.beatsPerSample;
    const int minSamples = juce::jmax (1, (int) std::ceil (stf::minGlideMs * 0.001 * rate));

    // A change of the settings under the playing step (a height, Low or High) moves the cutoff over the minimum time.
    if (stepKnown && ! block.jump)
    {
        const double target = std::log2 (stf::clampCutoff (stf::getStepCutoffHz (s, stepIndex), rate));

        if (std::abs (target - targetOct) > 1.0e-9)
        {
            startOct = currentOct;
            targetOct = target;
            glidePos = 0;
            glideLength = minSamples;
        }
    }

    if (s.steep != wasSteep)
    {
        for (auto& f : second)
            f.reset();

        wasSteep = s.steep;
    }

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));
    const double kMain = 1.0 / (double) filt::getQ (s.resonancePercent);
    const double kFlat = 1.0 / 0.7071;
    const float mix = juce::jlimit (0.0f, 1.0f, s.mixPercent / 100.0f);
    const bool listen = listenToEffect.load();
    float* channels[2] = { buffer.getWritePointer (0), numChannels > 1 ? buffer.getWritePointer (1) : nullptr };
    bool forceStep = block.jump || ! stepKnown;

    for (int i = 0; i < total; ++i)
    {
        const double beatNow = block.beat + (double) i * block.beatsPerSample;
        const long long k = (long long) std::floor (beatNow * invStepBeats + 1.0e-9);

        if (forceStep || k != stepAbs)
        {
            beginStep (k, forceStep && block.jump && i == 0, s, stepSamples);
            forceStep = false;
        }

        if (glidePos < glideLength)
        {
            ++glidePos;
            currentOct = startOct + (targetOct - startOct) * ((double) glidePos / (double) glideLength);
        }
        else
        {
            currentOct = targetOct;
        }

        if (! juce::exactlyEqual (currentOct, cachedOct))
        {
            cachedOct = currentOct;
            cachedG = std::tan (pi * std::exp2 (currentOct) / rate);
        }

        const float gain = outputGain.getNextValue();
        float inPeak = 0.0f, changePeak = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const double x = channels[ch][i];
            double y = first[(size_t) ch].run (x, cachedG, s.steep ? kFlat : kMain, s.type);

            if (s.steep)
                y = second[(size_t) ch].run (y, cachedG, kMain, s.type);

            if (! std::isfinite (y))
            {
                first[(size_t) ch].reset();
                second[(size_t) ch].reset();
                y = 0.0;
            }

            const float wet = (float) y;
            const float dry = (float) x;
            const float out = (listen ? dry - wet : (1.0f - mix) * dry + mix * wet) * gain;
            channels[ch][i] = out;

            inPeak = juce::jmax (inPeak, std::abs (dry));
            changePeak = juce::jmax (changePeak, std::abs (out - dry));
            meterInPeak = juce::jmax (meterInPeak, std::abs (dry));
            meterOutPeak = juce::jmax (meterOutPeak, std::abs (out));
            meterInSq += (double) dry * dry;
            meterOutSq += (double) out * out;
        }

        meterSamples += numChannels;
        chunkIn = juce::jmax (chunkIn, inPeak);
        chunkChange = juce::jmax (chunkChange, changePeak);

        if (++chunkCount >= fx::LevelHistory::chunk)
        {
            levels.push ({ juce::Decibels::gainToDecibels (chunkIn, -100.0f), juce::Decibels::gainToDecibels (chunkChange, -100.0f) });
            chunkIn = chunkChange = 0.0f;
            chunkCount = 0;
        }
    }

    stepClock.finish (block, total);
    currentCutoffHz = (float) std::exp2 (currentOct);

    if (numChannels == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, total);
}

juce::AudioProcessorEditor* BuiltInStepFilter::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInStepFilter::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInStepFilter::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
