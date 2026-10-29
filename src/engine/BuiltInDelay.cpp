#include "BuiltInDelay.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace dly
{
    namespace
    {
        eq::Band highCutBand (float hz) { return { true, eq::BandType::highCut, hz, 0.0f, eq::flatQ, 12 }; }
        eq::Band lowCutBand (float hz) { return { true, eq::BandType::lowCut, hz, 0.0f, eq::flatQ, 12 }; }

        inline double runStages (const eq::Stages& stages, double (&memory)[2][2], double x)
        {
            for (int st = 0; st < stages.num; ++st)
            {
                const auto& f = stages.filters[(size_t) st];
                auto& z = memory[st];
                const double y = f.b0 * x + z[0];
                z[0] = f.b1 * x - f.a1 * y + z[1];
                z[1] = f.b2 * x - f.a2 * y;
                x = y;
            }

            return x;
        }
    }

    double getNoteMs (int note, double bpm)
    {
        return 60000.0 / getTempo (bpm) * noteValues[juce::jlimit (0, numNoteValues - 1, note)].beats;
    }

    double getDelayMs (const Settings& s, double bpm)
    {
        return juce::jlimit (1.0, longestMs, s.sync ? getNoteMs (s.note, bpm) : (double) s.timeMs);
    }

    int getAudibleRepeats (float feedbackPercent)
    {
        const double fb = juce::jlimit (0.0, maxFeedback / 100.0, feedbackPercent / 100.0);

        if (fb < 0.001)
            return 1;

        return 1 + (int) std::floor (-60.0 / (20.0 * std::log10 (fb)));
    }

    double getRepeatDb (float feedbackPercent, int n)
    {
        const double fb = juce::jlimit (0.0, maxFeedback / 100.0, feedbackPercent / 100.0);

        if (n <= 1)
            return 0.0;

        return fb < 1.0e-6 ? -100.0 : 20.0 * (n - 1) * std::log10 (fb);
    }

    double getFilteredDb (const Settings& s, double hz, int repeats, double sampleRate)
    {
        double gain = 1.0;

        if (isHighCutOn (s.highCutHz))
            gain *= eq::makeStages (highCutBand (s.highCutHz), sampleRate).getMagnitude (hz, sampleRate);

        if (isLowCutOn (s.lowCutHz))
            gain *= eq::makeStages (lowCutBand (s.lowCutHz), sampleRate).getMagnitude (hz, sampleRate);

        return repeats * juce::Decibels::gainToDecibels (gain, -200.0);
    }

    int findNearestNote (double ms, double bpm)
    {
        int best = 0;

        for (int i = 1; i < numNoteValues; ++i)
            if (std::abs (std::log (getNoteMs (i, bpm) / ms)) < std::abs (std::log (getNoteMs (best, bpm) / ms)))
                best = i;

        return best;
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        auto hz = [] (float v)
        {
            if (v >= 1000.0f)
                return juce::String (v / 1000.0f, 1) + " kHz";

            return juce::String (juce::roundToInt (v)) + " Hz";
        };

        auto ms = [] (double v)
        {
            if (v >= 1000.0)
                return juce::String (v / 1000.0, 2) + " s";

            return fx::formatNumber (v, v < 10.0 ? 1 : 0) + " ms";
        };

        switch (k)
        {
            case Knob::time:     return s.sync ? juce::String (noteValues[juce::jlimit (0, numNoteValues - 1, s.note)].name) : ms (s.timeMs);
            case Knob::feedback: return juce::String (juce::roundToInt (s.feedbackPercent)) + "%";
            case Knob::highCut:  return isHighCutOn (s.highCutHz) ? hz (s.highCutHz) : juce::String ("Off");
            case Knob::lowCut:   return isLowCutOn (s.lowCutHz) ? hz (s.lowCutHz) : juce::String ("Off");
            case Knob::mode:     return s.mode == Mode::pingPong ? "Ping-pong" : "Stereo";
            case Knob::mix:      return juce::String (juce::roundToInt (s.mixPercent)) + "%";
        }

        return {};
    }

    const Preset& getReturnPreset()
    {
        for (auto& preset : getPresets())
            if (preset.name == "Return: quarter echo")
                return preset;

        return getPresets().front();
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;
            auto add = [&list] (juce::String name, juce::String why, Settings s, std::array<juce::String, numKnobs> notes)
            {
                list.push_back ({ std::move (name), std::move (why), s, std::move (notes) });
            };

            auto synced = [] (int note, float feedback, float highCut, float lowCut, Mode mode, float mix)
            {
                return Settings { 350.0f, true, note, feedback, highCut, lowCut, mode, mix };
            };

            auto free = [] (float ms, float feedback, float highCut, float lowCut, Mode mode, float mix)
            {
                return Settings { ms, false, 5, feedback, highCut, lowCut, mode, mix };
            };

            add ("Neutral start", "Eighth-note echoes that fade out after a few repeats: something to hear every control on.",
                 synced (5, 35.0f, 6000.0f, 150.0f, Mode::stereo, 25.0f),
                 { "1/8, following the tempo: the echoes fall between the beats, in time with the song.",
                   "35%: each echo comes back at about a third of the level of the one before: three or four repeats are heard.",
                   "6 kHz: every repeat is a little darker, so the echoes fall back behind the sound.",
                   "150 Hz: the echoes don't add to the low end.",
                   "Stereo: the echoes come from where the sound comes from.",
                   "25%: the echoes are clearly heard, but quieter than the sound." });

            add ("Slapback", "One quick echo, the sound of 1950s rock 'n' roll vocals and rockabilly guitar (Elvis, Sun Records): a "
                             "tape machine playing back what it had just recorded.",
                 free (110.0f, 5.0f, 5000.0f, 200.0f, Mode::stereo, 30.0f),
                 { "110 ms, not following the tempo: slapback is about one fixed, quick echo, 80-150 ms. Shorter fattens, longer "
                   "sounds more like an echo.",
                   "5%: practically one single echo.",
                   "5 kHz: tape echo was darker than the original.",
                   "200 Hz: a thin echo, so it doesn't thicken the low end.",
                   "Stereo.",
                   "30%: loud enough to hear the 'slap'." });

            add ("Vocal echo", "Quarter-note echoes behind a vocal: the ends of the phrases repeat in the gaps, without blurring the words.",
                 synced (8, 30.0f, 4500.0f, 300.0f, Mode::stereo, 18.0f),
                 { "1/4: the echo of a word lands on the next beat.",
                   "30%: two or three repeats.",
                   "4.5 kHz: dark echoes stay behind the voice; bright ones compete with it.",
                   "300 Hz: thin echoes don't muddy the vocal.",
                   "Stereo.",
                   "18%: you notice the echoes most in the pauses, which is where they should be." });

            add ("Dotted eighth guitar", "The rhythmic guitar echo of U2's The Edge and many others: dotted eighth notes, so the echoes "
                                         "fall between the notes played and make a running pattern out of simple picking.",
                 synced (7, 40.0f, 7000.0f, 250.0f, Mode::stereo, 35.0f),
                 { "Dotted 1/8: three sixteenths. Echoes of eighth notes land on the sixteenths in between, so two notes become a "
                   "galloping pattern.",
                   "40%: a few repeats keep the pattern going.",
                   "7 kHz: the echoes stay clear enough to be part of the rhythm.",
                   "250 Hz.",
                   "Stereo.",
                   "35%: the echoes are almost as important as the notes." });

            add ("Ping-pong", "Echoes that bounce from left to right: wide and moving. Good on synths, keys and short phrases.",
                 synced (5, 45.0f, 8000.0f, 200.0f, Mode::pingPong, 30.0f),
                 { "1/8.",
                   "45%: enough repeats to hear them bounce.",
                   "8 kHz.",
                   "200 Hz.",
                   "Ping-pong: the first echo comes from the left, the next from the right, and so on. The sound itself stays in the middle.",
                   "30%." });

            add ("Wide double", "One echo, so short it isn't heard as an echo: on one side only, it makes a mono sound wide. This is the "
                                "Haas effect: the ear hears the first arrival as where the sound is, and the copy as width.",
                 free (18.0f, 0.0f, 12000.0f, 150.0f, Mode::pingPong, 40.0f),
                 { "18 ms: under about 30 ms the ear fuses a copy with the sound instead of hearing an echo.",
                   "0%: one copy only.",
                   "12 kHz.",
                   "150 Hz: the low end stays in the middle.",
                   "Ping-pong without feedback: the single copy comes only from the left, the sound from the middle, so it widens.",
                   "40%. Check in mono: very short delays can make a thin, hollow sound when left and right are added together." });

            add ("Dub echo", "Long, dark, washing repeats that keep going, as in reggae and dub, where the engineer played the echo "
                             "like an instrument.",
                 synced (9, 70.0f, 2500.0f, 300.0f, Mode::pingPong, 35.0f),
                 { "Dotted 1/4: long, lazy echoes across the beat.",
                   "70%: many repeats; turn it up to near 90% for the runaway echo dub is known for, and back down before it gets too loud.",
                   "2.5 kHz: each repeat darker, like a worn tape echo, so they melt into a wash.",
                   "300 Hz: thin echoes, so the bass stays clean.",
                   "Ping-pong.",
                   "35%." });

            add ("Ambient repeats", "Slow, soft echoes that keep going and become a background: for pads, piano and guitar swells.",
                 synced (10, 60.0f, 4000.0f, 200.0f, Mode::pingPong, 30.0f),
                 { "1/2: long, slow echoes.",
                   "60%: many repeats that overlap with what's played next.",
                   "4 kHz: soft and far away.",
                   "200 Hz.",
                   "Ping-pong: they move around the listener.",
                   "30%." });

            add ("Return: quarter echo", "For a return track: quarter-note echoes that several tracks can share, heard mostly in the gaps "
                                         "between phrases.",
                 synced (8, 30.0f, 4500.0f, 300.0f, Mode::stereo, 100.0f),
                 { "1/4: the echo of a word lands on the next beat.",
                   "30%: two or three repeats.",
                   "4.5 kHz: dark echoes stay behind the sounds they repeat.",
                   "300 Hz: thin echoes don't muddy the low end.",
                   "Stereo: the echoes keep the left and right of what is sent.",
                   "100%: on a return, only the echoes; each send sets how much a track gets." });

            add ("Return: dotted ping-pong", "For a return track: dotted-eighth echoes that bounce from left to right, shared by the parts "
                                             "that should move: synths, keys, guitar lines.",
                 synced (7, 35.0f, 5000.0f, 250.0f, Mode::pingPong, 100.0f),
                 { "Dotted 1/8: three sixteenths, so the echoes fall between the notes played.",
                   "35%: three or four repeats.",
                   "5 kHz: the echoes stay behind the sound.",
                   "250 Hz.",
                   "Ping-pong: the first echo comes from the left, the next from the right, and so on.",
                   "100%: on a return, only the echoes." });

            return list;
        }();

        return presets;
    }

    //==============================================================================
    void Echoes::prepare (double sampleRate)
    {
        rate = sampleRate > 0.0 ? sampleRate : 44100.0;

        for (auto& m : memory)
            m.assign ((size_t) (longestMs * rate / 1000.0) + 4, 0.0f);

        delaySamples.reset (rate, 0.2);
        delaySamples.setCurrentAndTargetValue (0.0);   // the next settings start at their time, without a glide
        reset();
    }

    void Echoes::reset()
    {
        for (auto& m : memory)
            std::fill (m.begin(), m.end(), 0.0f);

        write = 0;
        std::fill_n (&filterMemory[0][0][0][0], 16, 0.0);
    }

    void Echoes::setSettings (const Settings& s, double delayMs)
    {
        const bool first = memory[0].empty() || delaySamples.getTargetValue() <= 0.0;
        settings = s;
        const double target = juce::jlimit (1.0, (double) memory[0].size() - 2.0, delayMs * rate / 1000.0);

        if (first)
            delaySamples.setCurrentAndTargetValue (target);
        else
            delaySamples.setTargetValue (target);

        feedback = juce::jlimit (0.0f, maxFeedback / 100.0f, s.feedbackPercent / 100.0f);

        const bool high = isHighCutOn (s.highCutHz), low = isLowCutOn (s.lowCutHz);

        if (high)
            highCut = eq::makeStages (highCutBand (s.highCutHz), rate);

        if (low)
            lowCut = eq::makeStages (lowCutBand (s.lowCutHz), rate);

        for (int side = 0; side < 2; ++side)
        {
            if (high && ! highCutOn) std::fill_n (&filterMemory[side][0][0][0], 4, 0.0);
            if (low && ! lowCutOn) std::fill_n (&filterMemory[side][1][0][0], 4, 0.0);
        }

        highCutOn = high;
        lowCutOn = low;
    }

    void Echoes::process (const float* inL, const float* inR, float* outL, float* outR, int numSamples)
    {
        const int size = (int) memory[0].size();

        if (size < 4)
            return;

        const bool pingPong = settings.mode == Mode::pingPong;

        for (int n = 0; n < numSamples; ++n)
        {
            // Read `delay` behind where this sample goes, between two samples when it's a fraction.
            const double delay = delaySamples.getNextValue();
            const double position = write - delay + size;
            const int i0 = (int) position % size, i1 = (i0 + 1) % size;
            const float frac = (float) (position - std::floor (position));
            float echo[2];

            for (int side = 0; side < 2; ++side)
            {
                const auto& m = memory[(size_t) side];
                double v = m[(size_t) i0] + frac * (m[(size_t) i1] - m[(size_t) i0]);

                if (highCutOn) v = runStages (highCut, filterMemory[side][0], v);
                if (lowCutOn) v = runStages (lowCut, filterMemory[side][1], v);

                echo[side] = (float) v;
            }

            const float left = inL[n], right = inR != nullptr ? inR[n] : inL[n];

            if (pingPong)
            {
                memory[0][(size_t) write] = 0.5f * (left + right) + feedback * echo[1];
                memory[1][(size_t) write] = feedback * echo[0];
            }
            else
            {
                memory[0][(size_t) write] = left + feedback * echo[0];
                memory[1][(size_t) write] = right + feedback * echo[1];
            }

            write = (write + 1) % size;
            outL[n] = echo[0];
            outR[n] = echo[1];
        }
    }
} // namespace dly

//==============================================================================
namespace
{
    using dly::Knob;

    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    /** How a parameter reads in the editor and to hosts: the same as formatKnob. */
    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float v, int)
            {
                dly::Settings s;
                s.sync = false;
                s.timeMs = s.feedbackPercent = s.highCutHz = s.lowCutHz = s.mixPercent = v;
                return dly::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return k == Knob::highCut ? dly::maxHighCut : dly::minLowCut;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();

                if (text.containsIgnoreCase ("khz"))
                    return v * 1000.0f;

                return k == Knob::time && text.trim().endsWithIgnoreCase (" s") ? v * 1000.0f : v;
            });
    }
}

const char* BuiltInDelay::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::time:     return "time";
        case Knob::feedback: return "feedback";
        case Knob::highCut:  return "highcut";
        case Knob::lowCut:   return "lowcut";
        case Knob::mode:     return "mode";
        case Knob::mix:      return "mix";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInDelay::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const dly::Settings d;
    auto add = [&layout] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    juce::StringArray notes;

    for (auto& n : dly::noteValues)
        notes.add (n.name);

    add (Knob::time, "Time", logRange (dly::minTime, dly::maxTime), d.timeMs);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { syncId, 1 }, "Sync to tempo", d.sync));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { noteId, 1 }, "Note value", notes, d.note));
    add (Knob::feedback, "Feedback", { 0.0f, dly::maxFeedback, 1.0f }, d.feedbackPercent);
    add (Knob::highCut, "High cut", logRange (dly::minHighCut, dly::maxHighCut), d.highCutHz);
    add (Knob::lowCut, "Low cut", logRange (dly::minLowCut, dly::maxLowCut), d.lowCutHz);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::mode), 1 }, "Mode",
                                                              juce::StringArray { "Stereo", "Ping-pong" }, 0));
    add (Knob::mix, "Mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    return layout;
}

BuiltInDelay::BuiltInDelay()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItDelay", createLayout())
{
    for (int k = 0; k < dly::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    syncValue = parameters.getRawParameterValue (syncId);
    noteValue = parameters.getRawParameterValue (noteId);
}

juce::PluginDescription BuiltInDelay::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Delay";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44446c31;   // "DDl1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInDelay::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInDelay::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

dly::Settings BuiltInDelay::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    dly::Settings s;
    s.timeMs = value (Knob::time);
    s.sync = syncValue->load() >= 0.5f;
    s.note = juce::jlimit (0, dly::numNoteValues - 1, juce::roundToInt (noteValue->load()));
    s.feedbackPercent = value (Knob::feedback);
    s.highCutHz = value (Knob::highCut);
    s.lowCutHz = value (Knob::lowCut);
    s.mode = value (Knob::mode) >= 0.5f ? dly::Mode::pingPong : dly::Mode::stereo;
    s.mixPercent = value (Knob::mix);
    return s;
}

void BuiltInDelay::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInDelay::setSync (bool on)
{
    if (auto* p = parameters.getParameter (syncId))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

void BuiltInDelay::setNote (int note)
{
    if (auto* p = parameters.getParameter (noteId))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, dly::numNoteValues - 1, note)));
}

void BuiltInDelay::setSettings (const dly::Settings& s)
{
    setKnob (Knob::time, s.timeMs);
    setSync (s.sync);
    setNote (s.note);
    setKnob (Knob::feedback, s.feedbackPercent);
    setKnob (Knob::highCut, s.highCutHz);
    setKnob (Knob::lowCut, s.lowCutHz);
    setKnob (Knob::mode, s.mode == dly::Mode::pingPong ? 1.0f : 0.0f);
    setKnob (Knob::mix, s.mixPercent);
}

double BuiltInDelay::getTailLengthSeconds() const
{
    const auto s = getSettings();
    return getDelayMs() / 1000.0 * dly::getAudibleRepeats (s.feedbackPercent) + 0.1;
}

//==============================================================================
bool BuiltInDelay::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInDelay::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    echoes.prepare (rate);
    echoes.setSettings (getSettings(), getDelayMs());
    wet.setSize (2, juce::jmax (64, maximumBlockSize));
    clap.prepare (rate);

    const bool only = listenToEffect.load();
    const auto [dry, wetLevel] = fx::getMixGains (getSettings().mixPercent);
    dryGain.reset (rate, 0.05);
    dryGain.setCurrentAndTargetValue (only ? 0.0f : dry);
    wetGain.reset (rate, 0.05);
    wetGain.setCurrentAndTargetValue (only ? 1.0f : wetLevel);
}

void BuiltInDelay::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (numChannels == 0)
        return;

    if (auto* playHead = getPlayHead())
        if (const auto position = playHead->getPosition())
            if (const auto bpm = position->getBpm())
                tempoBpm = *bpm;

    clap.addTo (buffer, numChannels);

    const auto s = getSettings();
    echoes.setSettings (s, dly::getDelayMs (s, tempoBpm.load()));

    const bool only = listenToEffect.load();
    const auto [dry, wetLevel] = fx::getMixGains (s.mixPercent);
    dryGain.setTargetValue (only ? 0.0f : dry);
    wetGain.setTargetValue (only ? 1.0f : wetLevel);
    constexpr int chunk = fx::LevelHistory::chunk;

    for (int start = 0; start < total;)
    {
        const int n = juce::jmin (total - start, wet.getNumSamples());
        float* l = buffer.getWritePointer (0, start);
        float* r = numChannels > 1 ? buffer.getWritePointer (1, start) : nullptr;
        float* wetL = wet.getWritePointer (0);
        float* wetR = wet.getWritePointer (1);
        echoes.process (l, r, wetL, wetR, n);

        for (int c = 0; c < n; c += chunk)
        {
            const int m = juce::jmin (chunk, n - c);
            float inPeak = 0.0f, wetPeak = 0.0f;

            for (int i = c; i < c + m; ++i)
            {
                const float dg = dryGain.getNextValue(), wg = wetGain.getNextValue();
                const float wl = wetL[i] * wg, wr = wetR[i] * wg;
                inPeak = juce::jmax (inPeak, std::abs (l[i]), r != nullptr ? std::abs (r[i]) : 0.0f);
                wetPeak = juce::jmax (wetPeak, std::abs (wl), std::abs (wr));

                if (r != nullptr)
                {
                    l[i] = l[i] * dg + wl;
                    r[i] = r[i] * dg + wr;
                }
                else
                {
                    l[i] = l[i] * dg + 0.5f * (wl + wr);
                }
            }

            history.push ({ juce::Decibels::gainToDecibels (inPeak, -100.0f), juce::Decibels::gainToDecibels (wetPeak, -100.0f) });
        }

        start += n;
    }

    if (numChannels == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, total);
}

juce::AudioProcessorEditor* BuiltInDelay::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInDelay::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInDelay::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
