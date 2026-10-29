#include "BuiltInDistortion.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace dist
{
    namespace
    {
        // The sample rate the pictures' filters are worked out at.
        constexpr double pictureRate = 48000.0;

        /** x / (1 + |x|^p)^(1/p): a slope of 1 through 0, flattening towards 1; the higher p, the harder the knee. */
        inline float knee (float x, float p)
        {
            return x / std::pow (1.0f + std::pow (std::abs (x), p), 1.0f / p);
        }

        void addStages (std::vector<eq::Biquad>& list, const eq::Band& band, double sampleRate)
        {
            const auto stages = eq::makeStages (band, sampleRate);

            for (int i = 0; i < stages.num; ++i)
                list.push_back (stages.filters[(size_t) i]);
        }

        double getGain (const std::vector<eq::Biquad>& filters, double hz)
        {
            double gain = 1.0;

            for (auto& f : filters)
                gain *= f.getMagnitude (hz, pictureRate);

            return gain;
        }

        // A guitar speaker in its cabinet: the cone can't move enough air below about 90 Hz (with a bump where the
        // cabinet resonates), it breaks up into a bright bump around 2.5 kHz, and above 5 kHz it hardly plays at all.
        std::vector<eq::Band> getSpeakerBands()
        {
            using eq::BandType;
            return { { true, BandType::lowCut, 85.0f, 0.0f, 1.1f, 12 },
                     { true, BandType::bell, 450.0f, -3.0f, 0.9f, 12 },
                     { true, BandType::bell, 2500.0f, 4.0f, 1.4f, 12 },
                     { true, BandType::highCut, 5000.0f, 0.0f, eq::flatQ, 24 } };
        }
    }

    juce::String getTypeName (Type t)
    {
        switch (t)
        {
            case Type::overdrive:  return "Overdrive";
            case Type::distortion: return "Distortion";
            case Type::fuzz:       return "Fuzz";
            case Type::rectify:    return "Rectify";
            case Type::fold:       return "Fold";
        }

        return {};
    }

    float shape (Type t, float x)
    {
        switch (t)
        {
            case Type::overdrive:
                // Soft, and the bottom half gives in a little sooner than the top, like unmatched clipping diodes.
                return x >= 0.0f ? knee (x, 2.0f) : 0.8f * knee (x / 0.8f, 2.0f);

            case Type::distortion:
                return knee (x, 6.0f);

            case Type::fuzz:
                return x >= 0.0f ? knee (x, 4.0f) : 0.5f * knee (x / 0.5f, 4.0f);

            case Type::rectify:
                return knee (std::abs (x), 2.0f);

            case Type::fold:
                return std::sin (x);
        }

        return x;
    }

    float distort (Type t, float drive, float x)
    {
        if (t == Type::fuzz)
            return shape (t, drive * (x + fuzzBias)) - shape (t, drive * fuzzBias);

        return shape (t, drive * x);
    }

    std::vector<eq::Biquad> getFiltersBefore (const Settings& s, double sampleRate)
    {
        std::vector<eq::Biquad> list;

        if (isTightOn (s.tightHz))
            addStages (list, { true, eq::BandType::lowCut, s.tightHz, 0.0f, eq::flatQ, 12 }, sampleRate);

        return list;
    }

    std::vector<eq::Biquad> getFiltersAfter (const Settings& s, double sampleRate)
    {
        std::vector<eq::Biquad> list;

        if (isToneOn (s.toneHz))
            addStages (list, { true, eq::BandType::highCut, s.toneHz, 0.0f, eq::flatQ, 12 }, sampleRate);

        if (s.speaker)
            for (auto& band : getSpeakerBands())
                addStages (list, band, sampleRate);

        return list;
    }

    double getGainBefore (const Settings& s, double hz) { return getGain (getFiltersBefore (s, pictureRate), hz); }
    double getGainAfter (const Settings& s, double hz)  { return getGain (getFiltersAfter (s, pictureRate), hz); }

    double getSpeakerDb (double hz)
    {
        double db = 0.0;

        for (auto& band : getSpeakerBands())
            db += eq::getResponseDb (band, hz, pictureRate);

        return db;
    }

    float getPeakIntoCurve (const Settings& s, float inputPeak)
    {
        return inputPeak * (float) getGainBefore (s, noteHz);
    }

    std::vector<float> getHarmonicsDb (const Settings& s, float inputPeak, int count)
    {
        // One cycle of the note through the curve; the harmonics are its Fourier series, each then filtered.
        constexpr int n = 2048;
        const float drive = juce::Decibels::decibelsToGain (s.driveDb), peak = getPeakIntoCurve (s, inputPeak);
        std::vector<float> wave (n);

        for (int i = 0; i < n; ++i)
            wave[(size_t) i] = distort (s.type, drive, peak * (float) std::sin (juce::MathConstants<double>::twoPi * i / n));

        const auto filters = getFiltersAfter (s, pictureRate);
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

            const double level = 2.0 * std::sqrt (re * re + im * im) / n * getGain (filters, k * noteHz);
            levels.push_back ((float) juce::Decibels::gainToDecibels (level, -150.0));
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

    juce::String formatKnob (Knob k, const Settings& s)
    {
        auto db = [] (float v) { return (v >= 0.05f ? "+" : "") + juce::String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB"; };

        switch (k)
        {
            case Knob::tight:   return isTightOn (s.tightHz) ? juce::String (juce::roundToInt (s.tightHz)) + " Hz" : juce::String ("Off");
            case Knob::drive:   return db (s.driveDb);
            case Knob::type:    return getTypeName (s.type);
            case Knob::tone:    return isToneOn (s.toneHz) ? juce::String (s.toneHz / 1000.0f, 1) + " kHz" : juce::String ("Off");
            case Knob::speaker: return s.speaker ? "On" : "Off";
            case Knob::output:  return db (s.outputDb);
            case Knob::mix:     return juce::String (juce::roundToInt (s.mixPercent)) + "%";
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

            add ("Neutral start", "A medium overdrive with every filter off, so you hear the curve on its own. Switch the type and "
                                  "turn the drive, then add tight, tone and the speaker one at a time to hear what each one does.",
                 { 20.0f, 24.0f, Type::overdrive, 20000.0f, false, 0.0f, 100.0f },
                 { "Off: the whole sound goes into the curve, lows and all.",
                   "+24 dB: clearly distorted, but not yet a wall.",
                   "Overdrive: the softest curve.",
                   "Off: every harmonic the curve makes is heard, fizz and all.",
                   "Off: the raw distortion, as a pedal sounds straight into a mixing desk.",
                   "0 dB, with match level keeping it as loud as it was.",
                   "100%: all distorted." });

            add ("Edge of breakup", "A clean amp just starting to break up: play softly and it stays almost clean, dig in and it "
                                    "growls. For blues, indie, country and funk rhythm guitar, and for electric piano.",
                 { 120.0f, 12.0f, Type::overdrive, 8000.0f, true, 0.0f, 100.0f },
                 { "120 Hz: keeps the lowest strings from blurring the grit.",
                   "+12 dB: only the loudest notes reach the bend, so how hard you play sets how dirty it sounds.",
                   "Overdrive: soft, so it follows the playing.",
                   "8 kHz: smooths the very top.",
                   "On: through a guitar speaker, the way an amp is heard.",
                   "0 dB, matched.",
                   "100%." });

            add ("Rock rhythm", "Crunchy rhythm guitar for rock: power chords and riffs that bite but stay clear enough that "
                                "every note of the chord is heard. Less gain than most people think; record it twice and pan "
                                "the takes left and right for a wide wall.",
                 { 150.0f, 26.0f, Type::distortion, 7000.0f, true, 0.0f, 100.0f },
                 { "150 Hz: the low strings stay tight instead of flubby.",
                   "+26 dB: crunchy, with room left between the notes.",
                   "Distortion: hard and even, for a chunky, compressed sound.",
                   "7 kHz: takes the fizz off.",
                   "On: the speaker removes the harsh top a real amp never plays.",
                   "0 dB, matched.",
                   "100%." });

            add ("High-gain metal", "Tight, heavy rhythm guitar for metal: fast palm-muted riffs on low strings that stay sharp "
                                    "instead of turning into rumble. Metal players cut the lows before the distortion (often with "
                                    "an overdrive pedal in front of the amp) for exactly this.",
                 { 300.0f, 42.0f, Type::distortion, 6500.0f, true, 0.0f, 100.0f },
                 { "300 Hz: the big trick. The low notes still sound, but they don't drive the curve into mud; the speaker and the "
                   "harmonics give the weight back.",
                   "+42 dB: near the most it goes, a solid wall.",
                   "Distortion: hard and even.",
                   "6.5 kHz: high gain makes a lot of fizz; this tames it.",
                   "On.",
                   "0 dB, matched.",
                   "100%." });

            add ("Singing lead", "A lead guitar sound with long, singing notes. The drive squashes the dynamics, so a held note "
                                 "stays loud as the string dies away: that's sustain.",
                 { 200.0f, 38.0f, Type::overdrive, 6000.0f, true, 0.0f, 100.0f },
                 { "200 Hz: keeps the lead out of the bass's way.",
                   "+38 dB: lots, for sustain.",
                   "Overdrive: soft and lopsided, smoother and more vocal than the hard type.",
                   "6 kHz: a round, singing top.",
                   "On.",
                   "0 dB, matched.",
                   "100%." });

            add ("Vintage fuzz", "The buzzing, sputtering fuzz of the 1960s: a square wave that chokes off as the note dies. Best "
                                 "on single-note riffs and simple chords.",
                 { 20.0f, 40.0f, Type::fuzz, 4500.0f, true, 0.0f, 100.0f },
                 { "Off: old fuzz pedals take the whole sound, lows and all, which makes them woolly.",
                   "+40 dB: fuzz pedals are run almost all the way up.",
                   "Fuzz: lopsided and hard, and as the note dies it can't get past the bias and chokes off: the sputter.",
                   "4.5 kHz: a fuzz without a high cut is harsh.",
                   "On.",
                   "0 dB, matched: fuzz is far louder than a clean guitar.",
                   "100%." });

            add ("Octave fuzz", "A fuzz an octave up, like Jimi Hendrix's Octavia. Best on single notes, high on the neck.",
                 { 150.0f, 24.0f, Type::rectify, 5000.0f, true, 0.0f, 100.0f },
                 { "150 Hz.",
                   "+24 dB: enough to square the octave off into a fuzz.",
                   "Rectify: flipping the bottom of the wave up doubles its frequency: an octave up.",
                   "5 kHz.",
                   "On.",
                   "0 dB, matched.",
                   "100%: blend it lower to hear the note and its octave together." });

            add ("Bass grit", "Distorted bass that keeps its low end: only a copy is distorted, and with the lows cut before the "
                              "curve, so the clean bass underneath keeps its weight and the distortion adds growl in the mids. "
                              "Many bass players run a clean and a dirty amp side by side for the same reason.",
                 { 250.0f, 30.0f, Type::distortion, 4000.0f, false, 0.0f, 40.0f },
                 { "250 Hz: the lowest notes don't get distorted, so they stay solid.",
                   "+30 dB: a strong growl.",
                   "Distortion.",
                   "4 kHz: keeps the growl from fizzing.",
                   "Off: a guitar speaker would take the bass away.",
                   "0 dB, matched.",
                   "40%: the clean bass on top, the dirt underneath." });

            add ("Drum smash", "A smashed copy of the drums blended under the clean ones: the kit gets loud, aggressive and "
                               "roomy while the hits keep their shape. Common on rock and electronic drums, and on drum room mics.",
                 { 20.0f, 30.0f, Type::distortion, 9000.0f, false, 0.0f, 30.0f },
                 { "Off.",
                   "+30 dB: smashed flat.",
                   "Distortion.",
                   "9 kHz: keeps the cymbals from turning to hash.",
                   "Off.",
                   "0 dB, matched.",
                   "30%: parallel, the clean drums on top." });

            add ("Megaphone vocal", "A voice through a megaphone, a telephone or a cheap radio: thin, band-limited and broken up. "
                                    "For an intro, one line, a verse that should feel distant, or a backing vocal.",
                 { 700.0f, 20.0f, Type::distortion, 3000.0f, false, 0.0f, 100.0f },
                 { "700 Hz: all the body of the voice is gone, as through a tiny speaker.",
                   "+20 dB: broken up, but the words are clear.",
                   "Distortion.",
                   "3 kHz: the top is gone too, which leaves only the middle.",
                   "Off.",
                   "0 dB, matched.",
                   "100%." });

            add ("Synth folds", "A plain synth sound turned rich, hollow and metallic. Wavefolding is a classic of West Coast "
                                "synthesis; turn the drive while a note holds and the sound sweeps through its folds.",
                 { 20.0f, 14.0f, Type::fold, 10000.0f, false, 0.0f, 100.0f },
                 { "Off.",
                   "+14 dB: the wave folds a few times; more drive, more folds, brighter sound.",
                   "Fold: what goes over the top folds back down.",
                   "10 kHz: softens the highest folds.",
                   "Off.",
                   "0 dB, matched.",
                   "100%." });

            return list;
        }();

        return presets;
    }
} // namespace dist

//==============================================================================
namespace
{
    using dist::Knob;

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
                dist::Settings s;
                s.tightHz = s.driveDb = s.toneHz = s.outputDb = s.mixPercent = v;
                return dist::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return k == Knob::tight ? dist::minTight : dist::maxTone;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();
                return text.containsIgnoreCase ("khz") ? v * 1000.0f : v;
            });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;
}

void BuiltInDistortion::Chain::set (const std::vector<eq::Biquad>& list)
{
    const int n = juce::jmin (maxFilters, (int) list.size());

    if (n != num)
        std::fill_n (&memory[0][0][0], 2 * maxFilters * 2, 0.0);

    std::copy_n (list.begin(), n, filters.begin());
    num = n;
}

float BuiltInDistortion::Chain::run (int channel, float input)
{
    double x = input;

    for (int i = 0; i < num; ++i)
    {
        const auto& f = filters[(size_t) i];
        auto& z = memory[channel][i];
        const double y = f.b0 * x + z[0];
        z[0] = f.b1 * x - f.a1 * y + z[1];
        z[1] = f.b2 * x - f.a2 * y;
        x = y;
    }

    return (float) x;
}

const char* BuiltInDistortion::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::tight:   return "tight";
        case Knob::drive:   return "drive";
        case Knob::type:    return "type";
        case Knob::tone:    return "tone";
        case Knob::speaker: return "speaker";
        case Knob::output:  return "output";
        case Knob::mix:     return "mix";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInDistortion::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const dist::Settings d;
    auto add = [&layout] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    add (Knob::tight, "Tight", logRange (dist::minTight, dist::maxTight), d.tightHz);
    add (Knob::drive, "Drive", { 0.0f, dist::maxDrive, 0.1f }, d.driveDb);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::type), 1 }, "Type",
                                                              juce::StringArray { "Overdrive", "Distortion", "Fuzz", "Rectify", "Fold" }, 0));
    add (Knob::tone, "Tone", logRange (dist::minTone, dist::maxTone), d.toneHz);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { getParameterId (Knob::speaker), 1 }, "Speaker", d.speaker));
    add (Knob::output, "Output", { dist::minOutput, dist::maxOutput, 0.1f }, d.outputDb);
    add (Knob::mix, "Mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "match", 1 }, "Match level", true));
    return layout;
}

BuiltInDistortion::BuiltInDistortion()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItDistortion", createLayout())
{
    for (int k = 0; k < dist::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    matchLevel = parameters.getRawParameterValue ("match");
}

juce::PluginDescription BuiltInDistortion::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Distortion";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44447331;   // "DDs1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInDistortion::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInDistortion::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

dist::Settings BuiltInDistortion::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    dist::Settings s;
    s.tightHz = value (Knob::tight);
    s.driveDb = value (Knob::drive);
    s.type = (dist::Type) juce::jlimit (0, dist::numTypes - 1, juce::roundToInt (value (Knob::type)));
    s.toneHz = value (Knob::tone);
    s.speaker = value (Knob::speaker) >= 0.5f;
    s.outputDb = value (Knob::output);
    s.mixPercent = value (Knob::mix);
    return s;
}

void BuiltInDistortion::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInDistortion::setSettings (const dist::Settings& s)
{
    setKnob (Knob::tight, s.tightHz);
    setKnob (Knob::drive, s.driveDb);
    setKnob (Knob::type, (float) (int) s.type);
    setKnob (Knob::tone, s.toneHz);
    setKnob (Knob::speaker, s.speaker ? 1.0f : 0.0f);
    setKnob (Knob::output, s.outputDb);
    setKnob (Knob::mix, s.mixPercent);
}

void BuiltInDistortion::setMatchingLevel (bool on)
{
    if (auto* p = parameters.getParameter ("match"))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

//==============================================================================
bool BuiltInDistortion::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInDistortion::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    blockSize = juce::jmax (64, maximumBlockSize);

    // Linear-phase filters, so the distorted sound stays in time with the dry sound it's mixed with.
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

    hasFilters = false;

    for (auto* chain : { &before, &after, &cleanBefore, &cleanAfter })
    {
        chain->num = 0;
        std::fill_n (&chain->memory[0][0][0], 2 * Chain::maxFilters * 2, 0.0);
    }

    for (auto* state : { dcIn, dcOut, cleanDcIn, cleanDcOut })
        std::fill_n (state, 2, 0.0f);

    sumWetDry = sumDry = powerIn = powerOut = 0.0;
    peakHold = 0.0f;
}

void BuiltInDistortion::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

void BuiltInDistortion::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

    if (! hasFilters || std::abs (s.tightHz - shownFilters.tightHz) > 0.01f || std::abs (s.toneHz - shownFilters.toneHz) > 0.01f
        || s.speaker != shownFilters.speaker)
    {
        const auto filtersBefore = dist::getFiltersBefore (s, rate), filtersAfter = dist::getFiltersAfter (s, rate);
        before.set (filtersBefore);
        cleanBefore.set (filtersBefore);
        after.set (filtersAfter);
        cleanAfter.set (filtersAfter);
        shownFilters = s;
        hasFilters = true;
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

        // The dry sound, held back as long as the oversampling filters hold back the distorted sound; the
        // distorted sound's lows are cut before it goes into the curve.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* in = buffer.getWritePointer (ch, start);
            float* held = dry.getWritePointer (ch);
            float* line = delayLine.getWritePointer (ch);
            int w = delayWrite;

            for (int i = 0; i < n; ++i)
            {
                line[w] = in[i];
                w = (w + 1) % delayLine.getNumSamples();
                held[i] = line[w];
                in[i] = before.run (ch, in[i]);
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
                up.setSample ((int) ch, (int) i, dist::distort (s.type, d, up.getSample ((int) ch, (int) i)));
        }

        oversampler->processSamplesDown (block);

        for (int c = 0; c < n; c += chunkSize)
        {
            const int m = juce::jmin (chunkSize, n - c);
            float inPeak = 0.0f, addedPeak = 0.0f;
            double sumIn = 0.0, sumOut = 0.0;

            for (int i = c; i < c + m; ++i)
            {
                // After the curve: no DC from the lopsided curves, then the tone and the speaker. The clean copy that what
                // the curve added is measured against goes through the same filters, tight included, so it stays in step.
                float wet[2] = {}, clean[2] = {}, reference[2] = {};

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto blockDc = [dcPole] (float v, float& in, float& out)
                    {
                        const float dc = v - in + dcPole * out;
                        in = v;
                        out = dc;
                        return dc;
                    };

                    clean[ch] = dry.getSample (ch, i);
                    wet[ch] = after.run (ch, blockDc (buffer.getSample (ch, start + i), dcIn[ch], dcOut[ch]));
                    reference[ch] = cleanAfter.run (ch, blockDc (cleanBefore.run (ch, clean[ch]), cleanDcIn[ch], cleanDcOut[ch]));
                    inPeak = juce::jmax (inPeak, std::abs (clean[ch]));
                }

                // A clean copy at the level of the output: what's left over is what the curve added.
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
                matchGainDb = juce::jlimit (-48.0f, 24.0f, (float) (10.0 * std::log10 (powerIn / powerOut)));

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

juce::AudioProcessorEditor* BuiltInDistortion::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInDistortion::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInDistortion::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
