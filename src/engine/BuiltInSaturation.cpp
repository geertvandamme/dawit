#include "BuiltInSaturation.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace sat
{
    namespace
    {
        // The tube curve is shifted this far, so its negative half rounds off later than the positive.
        constexpr float tubeBias = 0.35f;

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

    juce::String getTypeName (Type t)
    {
        switch (t)
        {
            case Type::tape: return "Tape";
            case Type::tube: return "Tube";
            case Type::clip: return "Clip";
        }

        return {};
    }

    float shape (Type t, float x)
    {
        switch (t)
        {
            case Type::tape:
                return std::tanh (x);

            case Type::tube:
            {
                static const float offset = std::tanh (tubeBias), slope = 1.0f - offset * offset;
                return (std::tanh (x + tubeBias) - offset) / slope;
            }

            case Type::clip:
                return juce::jlimit (-1.0f, 1.0f, x);
        }

        return x;
    }

    std::vector<float> getHarmonicsDb (const Settings& s, float inputPeak, int count)
    {
        // One cycle of a sine through the curve; the harmonics are its Fourier series.
        constexpr int n = 2048;
        const float drive = juce::Decibels::decibelsToGain (s.driveDb);
        std::vector<float> wave (n);

        for (int i = 0; i < n; ++i)
            wave[(size_t) i] = shape (s.type, drive * inputPeak * (float) std::sin (juce::MathConstants<double>::twoPi * i / n));

        std::vector<float> levels;

        for (int k = 1; k <= count; ++k)
        {
            double re = 0.0, im = 0.0;

            for (int i = 0; i < n; ++i)
            {
                const double angle = juce::MathConstants<double>::twoPi * k * i / n;
                re += wave[(size_t) i] * std::cos (angle);
                im -= wave[(size_t) i] * std::sin (angle);
            }

            levels.push_back ((float) juce::Decibels::gainToDecibels (2.0 * std::sqrt (re * re + im * im) / n, -150.0));
        }

        return levels;
    }

    float getThdPercent (const std::vector<float>& harmonicsDb)
    {
        if (harmonicsDb.empty())
            return 0.0f;

        double others = 0.0;

        for (size_t k = 1; k < harmonicsDb.size(); ++k)
            others += std::pow (10.0, harmonicsDb[k] / 10.0);

        return (float) (100.0 * std::sqrt (others) / std::pow (10.0, harmonicsDb[0] / 20.0));
    }

    juce::String getHarmonicInterval (int n)
    {
        static const char* names[] = { "", "the note", "octave", "octave + 5th", "2 octaves", "2 oct + 3rd", "2 oct + 5th", "2 oct + flat 7th",
                                       "3 octaves", "3 oct + 2nd", "3 oct + 3rd", "3 oct + sharp 4th", "3 oct + 5th", "3 oct + flat 6th",
                                       "3 oct + flat 7th", "3 oct + 7th", "4 octaves" };
        return juce::isPositiveAndBelow (n, (int) std::size (names)) ? juce::String (names[n]) : juce::String ();
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        auto db = [] (float v, bool sign) { return (sign && v >= 0.05f ? "+" : "") + juce::String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB"; };

        switch (k)
        {
            case Knob::drive:  return db (s.driveDb, true);
            case Knob::type:   return getTypeName (s.type);
            case Knob::tone:   return isToneOn (s.toneHz) ? juce::String (s.toneHz / 1000.0f, 1) + " kHz" : juce::String ("Off");
            case Knob::output: return db (s.outputDb, true);
            case Knob::mix:    return juce::String (juce::roundToInt (s.mixPercent)) + "%";
        }

        return {};
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

            add ("Neutral start", "Moderate tape saturation: clearly there on loud parts, gentle on quiet ones. Turn the drive and "
                                  "switch the type to hear what each does, with match level on so louder doesn't fool you.",
                 { 12.0f, Type::tape, 20000.0f, 0.0f, 100.0f },
                 { "+12 dB: the loudest parts are rounded off noticeably; quiet parts pass almost clean.",
                   "Tape: soft and symmetric, the gentlest.",
                   "Off: everything the curve adds is heard.",
                   "0 dB, with match level keeping it as loud as it was.",
                   "100%: all saturated." });

            add ("Warm vocal", "A little tube saturation makes a vocal thicker and more present, so it stands out in a mix without "
                               "being turned up. You shouldn't hear it as distortion; you should miss it when it's off.",
                 { 8.0f, Type::tube, 12000.0f, 0.0f, 100.0f },
                 { "+8 dB: only the loudest words get noticeably rounded.",
                   "Tube: the even harmonics it adds (an octave, an octave and a fifth above each note) blend in as warmth.",
                   "12 kHz: takes the edge off any fizz on s and t sounds.",
                   "0 dB, matched.",
                   "100%." });

            add ("Bass on small speakers", "Phones and laptop speakers can't play the lowest notes of a bass. Saturation adds "
                                           "harmonics an octave and more above them, which small speakers can play, and the ear "
                                           "fills in the missing low note from them. Blended in parallel, the clean low end stays.",
                 { 18.0f, Type::tube, 5000.0f, 0.0f, 50.0f },
                 { "+18 dB: strong, so the harmonics are clearly there.",
                   "Tube: the octave (the 2nd harmonic) is what makes the low note heard on small speakers.",
                   "5 kHz: keeps the added harmonics from turning into buzz.",
                   "0 dB, matched.",
                   "50%: the clean bass keeps its weight and the saturated half adds the harmonics." });

            add ("Drum crunch", "Hard-clipped drums blended under the clean ones: the hits keep their shape, and the clipped copy "
                                "adds bite and loudness between them.",
                 { 20.0f, Type::clip, 10000.0f, 0.0f, 35.0f },
                 { "+20 dB: the hits are clipped hard.",
                   "Clip: cuts the peaks flat, for bite and crunch.",
                   "10 kHz: keeps the cymbals from turning to hash.",
                   "0 dB, matched.",
                   "35%: parallel, the clean drums on top." });

            add ("Mix glue", "A touch of tape on the whole mix: the peaks are rounded a little, the parts feel more connected, and "
                             "the mix gets a little denser. Barely audible on its own.",
                 { 4.0f, Type::tape, 20000.0f, 0.0f, 100.0f },
                 { "+4 dB: only the highest peaks are touched.",
                   "Tape: the softest curve.",
                   "Off.",
                   "0 dB, matched.",
                   "100%." });

            add ("Synth warmth", "Digital synths can sound thin and clean. Tube saturation adds the harmonics and slight compression "
                                 "of analog gear.",
                 { 14.0f, Type::tube, 9000.0f, 0.0f, 100.0f },
                 { "+14 dB.",
                   "Tube: even harmonics, warm.",
                   "9 kHz: smooths the top.",
                   "0 dB, matched.",
                   "100%." });

            add ("Fuzz guitar", "As much as it goes: the sound is squared off into a buzzing fuzz, like a 1960s fuzz pedal. Best on "
                                "single notes and power chords.",
                 { 34.0f, Type::clip, 4000.0f, 0.0f, 100.0f },
                 { "+34 dB: almost every part of the wave is clipped; the sound becomes nearly a square wave.",
                   "Clip: hard, like a transistor fuzz.",
                   "4 kHz: a fuzz without a high cut is harsh and fizzy; guitar speakers cut above about 5 kHz anyway.",
                   "0 dB, matched: the fuzz is far louder than the clean guitar without it.",
                   "100%." });

            add ("Lo-fi radio", "Distorted and band-limited, like an old radio or a cheap speaker. For an intro, a break or a "
                                "voice in the background.",
                 { 22.0f, Type::clip, 2500.0f, 0.0f, 100.0f },
                 { "+22 dB.",
                   "Clip.",
                   "2.5 kHz: the top is gone, like a small speaker.",
                   "0 dB, matched.",
                   "100%." });

            return list;
        }();

        return presets;
    }
} // namespace sat

//==============================================================================
namespace
{
    using sat::Knob;

    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float v, int)
            {
                sat::Settings s;
                s.driveDb = s.toneHz = s.outputDb = s.mixPercent = v;
                return sat::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (k == Knob::tone && text.containsIgnoreCase ("off"))
                    return sat::maxTone;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();
                return text.containsIgnoreCase ("khz") ? v * 1000.0f : v;
            });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;
}

const char* BuiltInSaturation::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::drive:  return "drive";
        case Knob::type:   return "type";
        case Knob::tone:   return "tone";
        case Knob::output: return "output";
        case Knob::mix:    return "mix";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInSaturation::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const sat::Settings d;
    auto add = [&layout] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    add (Knob::drive, "Drive", { 0.0f, sat::maxDrive, 0.1f }, d.driveDb);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::type), 1 }, "Type",
                                                              juce::StringArray { "Tape", "Tube", "Clip" }, 0));
    add (Knob::tone, "Tone", logRange (sat::minTone, sat::maxTone), d.toneHz);
    add (Knob::output, "Output", { sat::minOutput, sat::maxOutput, 0.1f }, d.outputDb);
    add (Knob::mix, "Mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "match", 1 }, "Match level", true));
    return layout;
}

BuiltInSaturation::BuiltInSaturation()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItSaturation", createLayout())
{
    for (int k = 0; k < sat::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    matchLevel = parameters.getRawParameterValue ("match");
}

juce::PluginDescription BuiltInSaturation::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Distortion";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44537431;   // "DSt1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInSaturation::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInSaturation::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

sat::Settings BuiltInSaturation::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    sat::Settings s;
    s.driveDb = value (Knob::drive);
    s.type = (sat::Type) juce::jlimit (0, sat::numTypes - 1, juce::roundToInt (value (Knob::type)));
    s.toneHz = value (Knob::tone);
    s.outputDb = value (Knob::output);
    s.mixPercent = value (Knob::mix);
    return s;
}

void BuiltInSaturation::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInSaturation::setSettings (const sat::Settings& s)
{
    setKnob (Knob::drive, s.driveDb);
    setKnob (Knob::type, (float) (int) s.type);
    setKnob (Knob::tone, s.toneHz);
    setKnob (Knob::output, s.outputDb);
    setKnob (Knob::mix, s.mixPercent);
}

void BuiltInSaturation::setMatchingLevel (bool on)
{
    if (auto* p = parameters.getParameter ("match"))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

//==============================================================================
bool BuiltInSaturation::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInSaturation::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    blockSize = juce::jmax (64, maximumBlockSize);

    // Linear-phase filters, so the saturated sound stays in time with the dry sound it's mixed with.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampler->initProcessing ((size_t) blockSize);
    latency = juce::roundToInt (oversampler->getLatencyInSamples());
    setLatencySamples (latency);

    dry.setSize (2, blockSize);
    delayLine.setSize (2, latency + 1);
    delayLine.clear();
    delayWrite = 0;
    clap.prepare (rate);

    const auto s = getSettings();
    drive.reset (rate * oversampling, 0.05);
    drive.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (s.driveDb));
    mix.reset (rate, 0.05);
    mix.setCurrentAndTargetValue (s.mixPercent / 100.0f);
    outputGain.reset (rate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (s.outputDb));
    matchGain.reset (rate, 0.1);
    matchGain.setCurrentAndTargetValue (1.0f);
    matchGainDb = 0.0f;

    shownTone = -1.0f;
    std::fill_n (&toneMemory[0][0][0], 8, 0.0);
    std::fill_n (&cleanToneMemory[0][0][0], 8, 0.0);

    for (auto* state : { dcIn, dcOut, cleanDcIn, cleanDcOut })
        std::fill_n (state, 2, 0.0f);
    sumWetDry = sumDry = powerIn = powerOut = 0.0;
    peakHold = 0.0f;
}

void BuiltInSaturation::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples(), size = delayLine.getNumSamples();

    if (numChannels == 0 || size < 2)
        return;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        float* line = delayLine.getWritePointer (ch);
        int w = delayWrite;

        for (int i = 0; i < total; ++i)
        {
            line[w] = data[i];
            w = (w + 1) % size;
            data[i] = line[w];
        }
    }

    delayWrite = (delayWrite + total) % size;
}

void BuiltInSaturation::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (numChannels == 0 || oversampler == nullptr)
        return;

    clap.addTo (buffer, numChannels);

    const auto s = getSettings();
    const bool toneOn = sat::isToneOn (s.toneHz);

    if (toneOn && std::abs (s.toneHz - shownTone) > 0.01f)
    {
        if (shownTone < 0.0f)
        {
            std::fill_n (&toneMemory[0][0][0], 8, 0.0);
            std::fill_n (&cleanToneMemory[0][0][0], 8, 0.0);
        }

        shownTone = s.toneHz;
        toneStages = eq::makeStages ({ true, eq::BandType::highCut, s.toneHz, 0.0f, eq::flatQ, 12 }, rate);
    }
    else if (! toneOn)
    {
        shownTone = -1.0f;
    }

    const bool only = listenToEffect.load(), matching = isMatchingLevel();
    drive.setTargetValue (juce::Decibels::decibelsToGain (s.driveDb));
    mix.setTargetValue (s.mixPercent / 100.0f);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));
    const float dcPole = 1.0f - juce::MathConstants<float>::twoPi * 10.0f / (float) rate;
    const double fitCoefficient = 1.0 - std::exp (-1.0 / (0.015 * rate));                // about 15 ms, quick enough to follow a new note
    const double powerCoefficient = 1.0 - std::exp (-chunkSize / (0.5 * rate));           // about half a second

    for (int start = 0; start < total; start += blockSize)
    {
        const int n = juce::jmin (blockSize, total - start);

        // The dry sound, held back as long as the oversampling filters hold back the saturated sound.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* in = buffer.getReadPointer (ch, start);
            float* held = dry.getWritePointer (ch);
            float* line = delayLine.getWritePointer (ch);
            int w = delayWrite;

            for (int i = 0; i < n; ++i)
            {
                line[w] = in[i];
                w = (w + 1) % delayLine.getNumSamples();
                held[i] = line[w];
            }
        }

        delayWrite = (delayWrite + n) % delayLine.getNumSamples();

        // The curve, at four times the rate.
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numChannels, (size_t) start, (size_t) n);
        auto up = oversampler->processSamplesUp (block);

        for (size_t i = 0; i < up.getNumSamples(); ++i)
        {
            const float d = drive.getNextValue();

            for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
                up.setSample ((int) ch, (int) i, sat::shape (s.type, d * up.getSample ((int) ch, (int) i)));
        }

        oversampler->processSamplesDown (block);

        for (int c = 0; c < n; c += chunkSize)
        {
            const int m = juce::jmin (chunkSize, n - c);
            float inPeak = 0.0f, addedPeak = 0.0f;
            double sumIn = 0.0, sumOut = 0.0;

            for (int i = c; i < c + m; ++i)
            {
                // After the curve: no DC from the tube's lopsided curve, then the tone. The clean copy that what the
                // curve added is measured against goes through the same filters, so it stays in step with it.
                float wet[2] = {}, clean[2] = {}, reference[2] = {};

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto filter = [&] (float v, float& in, float& out, double (&memory)[2][2])
                    {
                        const float dc = v - in + dcPole * out;
                        in = v;
                        out = dc;
                        return toneOn ? (float) sat::runStages (toneStages, memory, dc) : dc;
                    };

                    clean[ch] = dry.getSample (ch, i);
                    wet[ch] = filter (buffer.getSample (ch, start + i), dcIn[ch], dcOut[ch], toneMemory[ch]);
                    reference[ch] = filter (clean[ch], cleanDcIn[ch], cleanDcOut[ch], cleanToneMemory[ch]);
                    inPeak = juce::jmax (inPeak, std::abs (clean[ch]));
                }

                // A clean copy of the input at the level of the output: what's left over is what the curve added.
                const float monoDry = 0.5f * (reference[0] + reference[numChannels - 1]), monoWet = 0.5f * (wet[0] + wet[numChannels - 1]);
                sumWetDry += fitCoefficient * ((double) monoWet * monoDry - sumWetDry);
                sumDry += fitCoefficient * ((double) monoDry * monoDry - sumDry);
                const float fit = sumDry > 1.0e-9 ? (float) (sumWetDry / sumDry) : drive.getCurrentValue();   // quiet: the curve is straight

                const float wetAmount = mix.getNextValue(), out = outputGain.getNextValue(), match = matchGain.getNextValue();

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float added = wet[ch] - fit * reference[ch];
                    const float blended = clean[ch] + wetAmount * (wet[ch] - clean[ch]);
                    sumIn += (double) clean[ch] * clean[ch];
                    sumOut += (double) blended * blended;
                    addedPeak = juce::jmax (addedPeak, std::abs (added * wetAmount * out * match));
                    buffer.setSample (ch, start + i, (only ? added : blended) * out * match);
                }
            }

            // Match level: the gain that makes the last half second as loud after as before.
            powerIn += powerCoefficient * (sumIn / (m * numChannels) - powerIn);
            powerOut += powerCoefficient * (sumOut / (m * numChannels) - powerOut);

            if (powerIn > 1.0e-8 && powerOut > 1.0e-12)
                matchGainDb = juce::jlimit (-36.0f, 24.0f, (float) (10.0 * std::log10 (powerIn / powerOut)));

            matchGain.setTargetValue (matching ? juce::Decibels::decibelsToGain (matchGainDb.load()) : 1.0f);

            // For the pictures: the loudest recent input, falling back slowly.
            peakHold = juce::jmax (inPeak, peakHold * 0.995f);
            recentPeak = peakHold;

            history.push ({ juce::Decibels::gainToDecibels (inPeak, -100.0f), juce::Decibels::gainToDecibels (addedPeak, -100.0f) });
        }
    }

    if (numChannels == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, total);
}

juce::AudioProcessorEditor* BuiltInSaturation::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInSaturation::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInSaturation::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
