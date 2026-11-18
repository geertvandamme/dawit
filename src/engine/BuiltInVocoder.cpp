#include "BuiltInVocoder.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace
{
juce::NormalisableRange<float> logRange (float start, float end)
{
    return { start, end,
             [] (float s, float e, float p) { return s * std::pow (e / s, p); },
             [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
             [] (float, float, float v) { return v; } };
}

constexpr int chunkSize = 32;
constexpr double carrierAmplitude = 0.5, noiseAmplitude = 0.35, consonantAmplitude = 0.5;
constexpr double detuneUp = 1.00405, detuneDown = 1.0 / 1.00405;   // 7 cents

double polyBlep (double t, double dt) noexcept
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

/** A band-limited saw from -1 to 1; moves the phase on. */
double sawStep (double& phase, double inc) noexcept
{
    const double value = 2.0 * phase - 1.0 - polyBlep (phase, inc);
    phase += inc;
    phase -= std::floor (phase);
    return value;
}

double softLimit (double x) noexcept
{
    const double a = std::abs (x);

    if (a <= 0.9)
        return x;

    return std::copysign (0.9 + 0.1 * std::tanh ((a - 0.9) / 0.1), x);
}
}

//==============================================================================
const char* BuiltInVocoder::getParameterId (vocoder::Knob k)
{
    using vocoder::Knob;

    switch (k)
    {
        case Knob::bands:      return "bands";
        case Knob::smoothing:  return "smoothing";
        case Knob::consonants: return "consonants";
        case Knob::voice:      return "voice";
        case Knob::low:        return "low";
        case Knob::high:       return "high";
        case Knob::source:     return "source";
        case Knob::wave:       return "wave";
        case Knob::root:       return "root";
        case Knob::shape:      return "shape";
        case Knob::range:      return "range";
        case Knob::carrier:    return "carrier";
        case Knob::formant:    return "formant";
        case Knob::mix:        return "mix";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInVocoder::createLayout()
{
    using vocoder::Knob;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const vocoder::Settings d;
    auto id = [] (Knob k) { return juce::ParameterID { getParameterId (k), 1 }; };
    auto addFloat = [&] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (id (k), name, range, value));
    };
    auto addChoice = [&] (Knob k, const char* name, juce::StringArray choices, int value)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (id (k), name, choices, value));
    };

    addChoice (Knob::bands, "Bands", { "4", "8", "12", "16", "24" }, 3);
    addFloat (Knob::smoothing, "Smoothing", { vocoder::minSmoothingMs, vocoder::maxSmoothingMs, 0.1f, 0.5f }, d.smoothingMs);
    addFloat (Knob::consonants, "Consonants", { 0.0f, 100.0f, 1.0f }, d.consonantsPercent);
    addFloat (Knob::voice, "Voice", { vocoder::minVoiceDb, 0.0f, 0.1f }, d.voiceDb);
    addFloat (Knob::low, "Low", logRange (vocoder::minLowHz, vocoder::maxLowHz), d.lowHz);
    addFloat (Knob::high, "High", logRange (vocoder::minHighHz, vocoder::maxHighHz), d.highHz);
    addChoice (Knob::source, "Source", { "Song chord", "Notes", "Keys" }, (int) d.source);
    addChoice (Knob::wave, "Wave", { "Saw", "Pulse", "Choir", "Noise" }, (int) d.wave);

    juce::StringArray roots;

    for (int pc = 0; pc < 12; ++pc)
        roots.add (vocoder::getPitchName (pc));

    addChoice (Knob::root, "Root", roots, d.root);
    addChoice (Knob::shape, "Shape", { "Note", "Fifth", "Major", "Minor" }, (int) d.shape);
    layout.add (std::make_unique<juce::AudioParameterInt> (id (Knob::range), "Range", 1, 3, d.range));
    addFloat (Knob::carrier, "Carrier", { vocoder::minCarrierDb, vocoder::maxCarrierDb, 0.1f }, d.carrierDb);
    addFloat (Knob::formant, "Formant", { -vocoder::maxFormant, vocoder::maxFormant, 0.1f }, d.formantSemitones);
    addFloat (Knob::mix, "Mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    return layout;
}

BuiltInVocoder::BuiltInVocoder()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItVocoder", createLayout())
{
    for (int k = 0; k < vocoder::numKnobs; ++k)
        values[(size_t) k] = parameters.getRawParameterValue (getParameterId ((vocoder::Knob) k));

    for (int k = 0; k < vocoder::maxBands; ++k)
    {
        voiceLevels[(size_t) k] = 0.0f;
        carrierLevels[(size_t) k] = 0.0f;
        resultLevels[(size_t) k] = 0.0f;
    }
}

juce::PluginDescription BuiltInVocoder::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Voice";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44766f63;   // "Dvoc"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInVocoder::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInVocoder::getKnobParameter (vocoder::Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

vocoder::Settings BuiltInVocoder::getSettings() const
{
    using vocoder::Knob;
    auto value = [this] (Knob k) { return values[(size_t) k]->load(); };
    vocoder::Settings s;
    s.bands = vocoder::bandCounts[(size_t) juce::jlimit (0, 4, juce::roundToInt (value (Knob::bands)))];
    s.smoothingMs = value (Knob::smoothing);
    s.consonantsPercent = value (Knob::consonants);
    s.voiceDb = value (Knob::voice);
    s.lowHz = value (Knob::low);
    s.highHz = value (Knob::high);
    s.source = (vocoder::Source) juce::jlimit (0, 2, juce::roundToInt (value (Knob::source)));
    s.wave = (vocoder::Wave) juce::jlimit (0, 3, juce::roundToInt (value (Knob::wave)));
    s.root = juce::jlimit (0, 11, juce::roundToInt (value (Knob::root)));
    s.shape = (vocoder::Shape) juce::jlimit (0, 3, juce::roundToInt (value (Knob::shape)));
    s.range = juce::jlimit (1, 3, juce::roundToInt (value (Knob::range)));
    s.carrierDb = value (Knob::carrier);
    s.formantSemitones = value (Knob::formant);
    s.mixPercent = value (Knob::mix);
    return s;
}

void BuiltInVocoder::setSettings (const vocoder::Settings& s)
{
    using vocoder::Knob;
    auto set = [this] (Knob k, float value)
    {
        if (auto* p = getKnobParameter (k))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    int bandsIndex = 3;

    for (size_t i = 0; i < vocoder::bandCounts.size(); ++i)
        if (vocoder::bandCounts[i] == s.bands)
            bandsIndex = (int) i;

    set (Knob::bands, (float) bandsIndex);
    set (Knob::smoothing, s.smoothingMs);
    set (Knob::consonants, s.consonantsPercent);
    set (Knob::voice, s.voiceDb);
    set (Knob::low, s.lowHz);
    set (Knob::high, s.highHz);
    set (Knob::source, (float) s.source);
    set (Knob::wave, (float) s.wave);
    set (Knob::root, (float) s.root);
    set (Knob::shape, (float) s.shape);
    set (Knob::range, (float) s.range);
    set (Knob::carrier, s.carrierDb);
    set (Knob::formant, s.formantSemitones);
    set (Knob::mix, s.mixPercent);
}

void BuiltInVocoder::applyPreset (const vocoder::Preset& preset)
{
    setSettings (preset.settings);
}

vocoder::Summary BuiltInVocoder::getSummary() const
{
    vocoder::Summary s;
    s.inputPeakDb = summaryInput.load();
    s.resultPeakDb = summaryResult.load();
    s.hissFraction = summaryHiss.load();
    s.chordFallbackFraction = summaryFallback.load();
    s.limiterHits = summaryHits.load();
    return s;
}

bool BuiltInVocoder::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

//==============================================================================
void BuiltInVocoder::makeBandPass (Coeffs& c, double hz, double q, double rate)
{
    const double w = juce::MathConstants<double>::twoPi * hz / rate, alpha = std::sin (w) / (2.0 * q), a0 = 1.0 + alpha;
    c.b0 = alpha / a0;
    c.b1 = 0.0;
    c.b2 = -alpha / a0;
    c.a1 = -2.0 * std::cos (w) / a0;
    c.a2 = (1.0 - alpha) / a0;
}

double BuiltInVocoder::run (const Coeffs& c, State& s, double x) noexcept
{
    const double y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
}

void BuiltInVocoder::Bank::configure (int newBands, double newLow, double newTop, double newRate, double ratio) noexcept
{
    bands = newBands;
    low = newLow;
    top = newTop;
    rate = newRate;
    consonantMask = 1 << (bands - 1);
    const double q = 1.0 / (2.0 * std::sinh (std::log (2.0) * std::log2 (top / low) / bands / 2.0));

    for (int k = 0; k < bands; ++k)
    {
        const double lower = low * std::pow (top / low, (double) k / bands), upper = low * std::pow (top / low, (double) (k + 1) / bands);
        centre[(size_t) k] = std::sqrt (lower * upper);
        makeBandPass (analysis[(size_t) k], centre[(size_t) k], q, rate);
        quickCoef[(size_t) k] = 1.0 - std::exp (-centre[(size_t) k] / rate);

        if (upper > vocoder::consonantHz - 0.5)
            consonantMask |= 1 << k;
    }

    setFormant (ratio);
    reset();
}

void BuiltInVocoder::Bank::setFormant (double ratio) noexcept
{
    if (bands <= 0)
        return;

    const double q = 1.0 / (2.0 * std::sinh (std::log (2.0) * std::log2 (top / low) / bands / 2.0));

    for (int k = 0; k < bands; ++k)
        makeBandPass (synthesis[(size_t) k], juce::jmin (centre[(size_t) k] * ratio, vocoder::topEdgeOfRate * rate), q, rate);
}

void BuiltInVocoder::Bank::reset() noexcept
{
    analysisState = {};
    synthesisState = {};
    quickEnergy = {};
    voiceEnergy = {};
    carrierEnergy = {};
    resultEnergy = {};
}

bool BuiltInVocoder::Bank::matches (int newBands, double newLow, double newTop) const noexcept
{
    return bands == newBands && std::abs (low - newLow) < 1.0e-6 && std::abs (top - newTop) < 1.0e-6;
}

void BuiltInVocoder::Bank::process (double voice, const double* carrier, const double* noise, double attack, double release, double meter, juce::uint32 hearMask, double* out) noexcept
{
    out[0] = out[1] = 0.0;

    for (int k = 0; k < bands; ++k)
    {
        const auto i = (size_t) k;
        const double a = run (analysis[i], analysisState[i][1], run (analysis[i], analysisState[i][0], voice));
        quickEnergy[i] += (a * a - quickEnergy[i]) * quickCoef[i];
        const double e = quickEnergy[i];
        voiceEnergy[i] += (e - voiceEnergy[i]) * (e > voiceEnergy[i] ? attack : release);
        const double envelope = std::sqrt (2.0 * voiceEnergy[i]);
        const bool hasNoise = ((consonantMask >> k) & 1) != 0;
        double band[2];

        for (int ch = 0; ch < 2; ++ch)
        {
            const double in = carrier[ch] + (hasNoise ? noise[ch] : 0.0);
            const double y = run (synthesis[i], synthesisState[i][(size_t) ch][1], run (synthesis[i], synthesisState[i][(size_t) ch][0], in));
            band[ch] = y * envelope;

            if (ch == 0)
                carrierEnergy[i] += (y * y - carrierEnergy[i]) * meter;
        }

        resultEnergy[i] += (0.5 * (band[0] * band[0] + band[1] * band[1]) - resultEnergy[i]) * meter;

        if (hearMask == 0 || ((hearMask >> k) & 1) != 0)
        {
            out[0] += band[0];
            out[1] += band[1];
        }
    }
}

//==============================================================================
void BuiltInVocoder::resetState() noexcept
{
    banks[0].reset();
    banks[1].reset();
    hissHighState[0] = hissLowState[0] = {};
    hissHighEnergy = hissLowEnergy = hissAmount = 0.0;

    for (auto& slot : slots)
        slot.gain = slot.target = 0.0;
}

void BuiltInVocoder::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto s = getSettings();
    configured = fading = false;
    current = 0;
    fadeX = 0.0;
    formantRatio = vocoder::getFormantRatio (s.formantSemitones);
    formantSmooth.reset (rate, 0.05);
    formantSmooth.setCurrentAndTargetValue (s.formantSemitones);

    const auto mix = fx::getMixGains (s.mixPercent);
    auto prepare = [this] (juce::SmoothedValue<float>& v, float value)
    {
        v.reset (rate, 0.02);
        v.setCurrentAndTargetValue (value);
    };
    prepare (carrierGain, juce::Decibels::decibelsToGain (s.carrierDb));
    prepare (voiceGain, s.voiceDb <= vocoder::minVoiceDb + 0.05f ? 0.0f : juce::Decibels::decibelsToGain (s.voiceDb));
    prepare (dryGain, mix.first);
    prepare (wetGain, mix.second);

    // The hiss detector: a high-pass at 4 kHz and a low-pass at 1.5 kHz (RBJ, Q 0.707).
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        double w = twoPi * vocoder::consonantHz / rate, alpha = std::sin (w) / (2.0 * 0.7071), c = std::cos (w), a0 = 1.0 + alpha;
        hissHigh = { (1.0 + c) / 2.0 / a0, -(1.0 + c) / a0, (1.0 + c) / 2.0 / a0, -2.0 * c / a0, (1.0 - alpha) / a0 };
        w = twoPi * 1500.0 / rate;
        alpha = std::sin (w) / (2.0 * 0.7071);
        c = std::cos (w);
        a0 = 1.0 + alpha;
        hissLow = { (1.0 - c) / 2.0 / a0, (1.0 - c) / a0, (1.0 - c) / 2.0 / a0, -2.0 * c / a0, (1.0 - alpha) / a0 };
    }

    resetState();
    held.fill (false);
    heldCount = 0;
    wantedCount = 0;
    wanted = {};
    wasKeys = false;
    norm = 1.0;

    for (auto& slot : slots)
    {
        slot = {};
        slot.note = -1;
    }

    testVoice = vocoder::makeTestVoice (rate);
    testPosition = -1;
    summary = {};
    summaryIndex = 0;
    levelCount = 0;
    levelIn = levelEffect = 0.0f;
}

void BuiltInVocoder::handleMidi (const juce::MidiMessage& m) noexcept
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();

        if (! held[(size_t) n])
            ++heldCount;

        held[(size_t) n] = true;
    }
    else if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();

        if (held[(size_t) n])
            --heldCount;

        held[(size_t) n] = false;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.fill (false);
        heldCount = 0;
    }
}

void BuiltInVocoder::setCarrierNotes (const vocoder::NoteList& notes, int count, double releaseSeconds) noexcept
{
    const double fadeIn = 1.0 / (0.02 * rate), fadeOut = 1.0 / (juce::jmax (0.001, releaseSeconds) * rate);
    auto isWanted = [&] (int note) { return std::find (notes.begin(), notes.begin() + count, note) != notes.begin() + count; };

    for (auto& slot : slots)
        if (slot.target > 0.0 && ! isWanted (slot.note))
        {
            slot.target = 0.0;
            slot.step = fadeOut;
        }

    for (int n = 0; n < count; ++n)
    {
        const int note = notes[(size_t) n];
        Slot* found = nullptr;
        bool fresh = false;

        for (auto& slot : slots)
            if (slot.note == note && (slot.target > 0.0 || slot.gain > 0.0))
                found = &slot;

        if (found != nullptr && found->target > 0.0)
            continue;

        if (found == nullptr)
        {
            for (auto& slot : slots)
                if (slot.target <= 0.0 && slot.gain <= 0.0 && found == nullptr)
                    found = &slot;

            if (found == nullptr)
                for (auto& slot : slots)
                    if (slot.target <= 0.0 && (found == nullptr || slot.gain < found->gain))
                        found = &slot;

            fresh = true;
        }

        if (found == nullptr)
            continue;

        if (fresh)
        {
            found->note = note;
            found->hz = vocoder::getNoteHz (note);

            for (auto& channel : found->phase)
                for (auto& phase : channel)
                    phase = random.nextDouble();
        }

        found->target = 1.0;
        found->step = fadeIn;
    }

    wanted = notes;
    wantedCount = count;
}

void BuiltInVocoder::processChunk (juce::AudioBuffer<float>& buffer, int offset, int n) noexcept
{
    auto s = getSettings();
    const bool demoSine = demoActive && testPosition >= 0 && demoCarrier == DemoCarrier::sine;

    if (demoActive && testPosition >= 0)
    {
        // A guide example: its band count and carrier take the settings' place while the phrase plays.
        if (demoBands > 0)
            s.bands = demoBands;

        if (demoCarrier == DemoCarrier::noise)
            s.wave = vocoder::Wave::noise;
        else if (demoCarrier == DemoCarrier::saw || demoSine)
            s.wave = vocoder::Wave::saw;
    }

    const double top = vocoder::getTopEdge (s.highHz, rate);

    if (! configured)
    {
        banks[0].configure (s.bands, s.lowHz, top, rate, formantRatio);
        current = 0;
        fading = false;
        configured = true;
    }
    else if (! fading && ! banks[(size_t) current].matches (s.bands, s.lowHz, top))
    {
        banks[(size_t) (1 - current)].configure (s.bands, s.lowHz, top, rate, formantRatio);
        fading = true;
        fadeX = 0.0;
        fadeStep = 1.0 / (0.02 * rate);
    }

    {
        const auto& shown = banks[(size_t) (fading ? 1 - current : current)];
        bandsShown = shown.bands;
        lowShown = shown.low;
        highShown = shown.top;
    }

    formantSmooth.setTargetValue (s.formantSemitones);
    const double ratio = vocoder::getFormantRatio (formantSmooth.skip (n));

    if (std::abs (ratio - formantRatio) > 1.0e-6)
    {
        formantRatio = ratio;
        banks[0].setFormant (ratio);
        banks[1].setFormant (ratio);
    }

    const double attack = 1.0 - std::exp (-1.0 / (0.001 * rate));
    const double release = 1.0 - std::exp (-1.0 / (0.0005 * s.smoothingMs * rate));
    const double meter = 1.0 - std::exp (-1.0 / (0.02 * rate)), hissCoef = 1.0 - std::exp (-1.0 / (0.005 * rate));
    const double hissRamp = 1.0 / (0.01 * rate), normCoef = 1.0 - std::exp (-1.0 / (0.02 * rate));

    // Which notes the carrier plays: the source's, or Notes when it has none (a bar with no chord, no key held).
    vocoder::NoteList list {};
    int count = 0;
    bool usingKeys = false, chordFallback = false, fallback = false;

    if (s.source == vocoder::Source::songChord)
    {
        if (const int mask = chordMask.load(); mask != 0)
            count = vocoder::writeChordNotes (mask, s.range, list);
        else
            fallback = chordFallback = true;
    }
    else if (s.source == vocoder::Source::keys)
    {
        for (int note = 0; note < 128 && count < vocoder::maxCarrierVoices; ++note)
            if (held[(size_t) note])
                list[(size_t) count++] = note;

        usingKeys = count > 0;
        fallback = count == 0;
    }

    if (count == 0)
        count = vocoder::writeChosenNotes (s.root, s.shape, s.range, list);

    if (count != wantedCount || ! std::equal (list.begin(), list.begin() + count, wanted.begin()))
    {
        setCarrierNotes (list, count, wasKeys ? 0.06 : 0.02);
        carrierMask = vocoder::getPitchMask ({ list.begin(), list.begin() + count });
    }

    wasKeys = usingKeys;
    fallbackShown = fallback;

    const double normTarget = 1.0 / std::sqrt ((double) juce::jmax (1, count));
    carrierGain.setTargetValue (juce::Decibels::decibelsToGain (s.carrierDb));
    voiceGain.setTargetValue (s.voiceDb <= vocoder::minVoiceDb + 0.05f ? 0.0f : juce::Decibels::decibelsToGain (s.voiceDb));
    const auto mix = fx::getMixGains (s.mixPercent);
    dryGain.setTargetValue (mix.first);
    wetGain.setTargetValue (mix.second);

    const double consonantGain = s.consonantsPercent / 100.0 * consonantAmplitude;
    const juce::uint32 hearMask = hearOnly.load() >= 0 ? 1u << (juce::uint32) hearOnly.load() : 0u;
    float* left = buffer.getWritePointer (0) + offset;
    const bool stereo = buffer.getNumChannels() > 1;
    float* right = stereo ? buffer.getWritePointer (1) + offset : left;   // a mono track is heard as the same sound left and right
    float peakIn = 0.0f, peakOut = 0.0f;
    int hits = 0;
    bool bad = false;

    for (int i = 0; i < n; ++i)
    {
        const bool demoNow = demoActive && testPosition >= 0;

        if (testPosition >= 0)
        {
            const float t = testVoice[(size_t) testPosition];

            // A guide example hears only the made-up voice; the plain test sound goes on top of what comes in.
            left[i] = demoActive ? t : left[i] + t;

            if (stereo)
                right[i] = demoActive ? t : right[i] + t;

            if (++testPosition >= (int) testVoice.size())
                testPosition = -1;
        }
        else if (demoActive)
        {
            demoActive = false;
        }

        const double dry[2] = { (double) left[i], (double) right[i] };
        const double voice = 0.5 * (dry[0] + dry[1]);

        // How hissy the voice is: above 4 kHz against below 1.5 kHz.
        const double high = run (hissHigh, hissHighState[0], voice), low = run (hissLow, hissLowState[0], voice);
        hissHighEnergy += (high * high - hissHighEnergy) * hissCoef;
        hissLowEnergy += (low * low - hissLowEnergy) * hissCoef;
        const bool hissy = hissHighEnergy > 2.0 * hissLowEnergy && hissHighEnergy > 1.0e-6;
        hissAmount = juce::jlimit (0.0, 1.0, hissAmount + (hissy ? hissRamp : -hissRamp));

        // The carrier.
        norm += (normTarget - norm) * normCoef;
        double carrier[2] = { 0.0, 0.0 };

        if (s.wave == vocoder::Wave::noise)
        {
            carrier[0] = (random.nextDouble() * 2.0 - 1.0) * noiseAmplitude;
            carrier[1] = (random.nextDouble() * 2.0 - 1.0) * noiseAmplitude;
        }
        else
        {
            for (auto& slot : slots)
            {
                if (slot.target <= 0.0 && slot.gain <= 0.0)
                    continue;

                if (slot.gain < slot.target)
                    slot.gain = juce::jmin (slot.target, slot.gain + slot.step);
                else if (slot.gain > slot.target)
                    slot.gain = juce::jmax (slot.target, slot.gain - slot.step);

                const double inc = slot.hz / rate, amplitude = slot.gain * carrierAmplitude * norm;

                if (s.wave == vocoder::Wave::choir)
                {
                    static constexpr double detune[3] = { detuneDown, 1.0, detuneUp };

                    for (int ch = 0; ch < 2; ++ch)
                        for (int j = 0; j < 3; ++j)
                            carrier[ch] += sawStep (slot.phase[ch][j], inc * detune[ch == 0 ? j : 2 - j]) * amplitude * 0.5774;
                }
                else if (s.wave == vocoder::Wave::pulse)
                {
                    constexpr double duty = 0.25;
                    double second = slot.phase[0][0] + duty;
                    second -= std::floor (second);
                    const double a = 2.0 * slot.phase[0][0] - 1.0 - polyBlep (slot.phase[0][0], inc);
                    const double b = 2.0 * second - 1.0 - polyBlep (second, inc);
                    const double value = (a - b) * 0.6 * amplitude;
                    slot.phase[0][0] += inc;
                    slot.phase[0][0] -= std::floor (slot.phase[0][0]);
                    carrier[0] += value;
                    carrier[1] += value;
                }
                else
                {
                    double value;

                    if (demoSine)
                    {
                        value = std::sin (juce::MathConstants<double>::twoPi * slot.phase[0][0]) * amplitude;
                        slot.phase[0][0] += inc;
                        slot.phase[0][0] -= std::floor (slot.phase[0][0]);
                    }
                    else
                    {
                        value = sawStep (slot.phase[0][0], inc) * amplitude;
                    }

                    carrier[0] += value;
                    carrier[1] += value;
                }
            }
        }

        // Noise for the consonants, while the voice is hissy.
        const double noiseLevel = hissAmount * consonantGain;
        const double noise[2] = { (random.nextDouble() * 2.0 - 1.0) * noiseLevel, (random.nextDouble() * 2.0 - 1.0) * noiseLevel };

        double out[2];
        banks[(size_t) current].process (voice, carrier, noise, attack, release, meter, hearMask, out);

        if (fading)
        {
            double other[2];
            banks[(size_t) (1 - current)].process (voice, carrier, noise, attack, release, meter, hearMask, other);
            out[0] += (other[0] - out[0]) * fadeX;
            out[1] += (other[1] - out[1]) * fadeX;
            fadeX += fadeStep;

            if (fadeX >= 1.0)
            {
                current = 1 - current;
                fading = false;
                fadeX = 0.0;
            }
        }

        const double carrierG = carrierGain.getNextValue(), voiceG = voiceGain.getNextValue(), dryG = dryGain.getNextValue(), wetG = wetGain.getNextValue();
        double result[2];

        for (int ch = 0; ch < 2; ++ch)
        {
            const double wet = out[ch] * carrierG + voiceG * dry[ch];
            double mixed = dry[ch] * dryG + wet * wetG;

            // A guide example plays one part at a fixed level, whatever Mix says.
            if (demoNow)
                mixed = demoPart == DemoPart::voiceAlone ? dry[ch] : demoPart == DemoPart::carrierAlone ? carrier[ch] * carrierG * 0.5 : out[ch] * carrierG;

            if (wetG > 0.0 || demoNow)
            {
                if (std::abs (mixed) > 1.0)
                    ++hits;

                mixed = softLimit (mixed);
            }

            if (! std::isfinite (mixed))
            {
                mixed = 0.0;
                bad = true;
            }

            result[ch] = mixed;
            levelEffect = juce::jmax (levelEffect, (float) std::abs (wet * wetG));
        }

        left[i] = (float) result[0];
        right[i] = (float) result[1];
        peakIn = juce::jmax (peakIn, (float) std::abs (dry[0]), (float) std::abs (dry[1]));
        peakOut = juce::jmax (peakOut, (float) std::abs (result[0]), (float) std::abs (result[1]));
        levelIn = juce::jmax (levelIn, (float) std::abs (dry[0]), (float) std::abs (dry[1]));

        if (++levelCount >= fx::LevelHistory::chunk)
        {
            levelHistory.push ({ juce::Decibels::gainToDecibels (levelIn, -100.0f), juce::Decibels::gainToDecibels (levelEffect, -100.0f) });
            levelCount = 0;
            levelIn = levelEffect = 0.0f;
        }
    }

    if (bad)
        resetState();

    auto& slot = summary[(size_t) summaryIndex];
    slot.inputPeak = juce::jmax (slot.inputPeak, peakIn);
    slot.resultPeak = juce::jmax (slot.resultPeak, peakOut);
    slot.samples += n;
    slot.hissy += hissAmount > 0.5 ? n : 0;
    slot.fallbacks += chordFallback ? n : 0;
    slot.hits += hits;

    if ((double) slot.samples >= rate)
    {
        summaryIndex = (summaryIndex + 1) % (int) summary.size();
        summary[(size_t) summaryIndex] = {};
    }
}

void BuiltInVocoder::publish() noexcept
{
    const bool incoming = fading && fadeX > 0.5;
    const auto& bank = banks[(size_t) (incoming ? 1 - current : current)];
    const float gain = carrierGain.getCurrentValue();

    for (int k = 0; k < vocoder::maxBands; ++k)
    {
        const auto i = (size_t) k;
        const bool on = k < bank.bands;
        voiceLevels[i] = on ? (float) std::sqrt (2.0 * bank.voiceEnergy[i]) : 0.0f;
        carrierLevels[i] = on ? (float) std::sqrt (2.0 * bank.carrierEnergy[i]) : 0.0f;
        resultLevels[i] = on ? (float) std::sqrt (2.0 * bank.resultEnergy[i]) * gain : 0.0f;
    }

    float input = 0.0f, result = 0.0f;
    long long samples = 0, hissy = 0, fallbacks = 0, hits = 0;

    for (const auto& slot : summary)
    {
        input = juce::jmax (input, slot.inputPeak);
        result = juce::jmax (result, slot.resultPeak);
        samples += slot.samples;
        hissy += slot.hissy;
        fallbacks += slot.fallbacks;
        hits += slot.hits;
    }

    summaryInput = juce::Decibels::gainToDecibels (input, -100.0f);
    summaryResult = juce::Decibels::gainToDecibels (result, -100.0f);
    summaryHiss = samples > 0 ? (float) ((double) hissy / (double) samples) : 0.0f;
    summaryFallback = samples > 0 ? (float) ((double) fallbacks / (double) samples) : 0.0f;
    summaryHits = (int) juce::jmin (hits, (long long) 1000000);
}

void BuiltInVocoder::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (buffer.getNumChannels() < 1)
        return;

    // Something that isn't a number never reaches the filters: it becomes silence and the filters start again.
    bool broken = false;

    for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int i = 0; i < total; ++i)
            if (! std::isfinite (data[i]))
            {
                data[i] = 0.0f;
                broken = true;
            }
    }

    if (broken)
        resetState();

    if (testRequested.exchange (false))
    {
        testPosition = 0;
        demoActive = demoRequested.load();
        demoPart = (DemoPart) requestedPart.load();
        demoBands = requestedBands.load();
        demoCarrier = (DemoCarrier) requestedCarrier.load();
    }

    auto it = midi.begin();
    int position = 0;

    while (position < total)
    {
        while (it != midi.end() && (*it).samplePosition <= position)
        {
            handleMidi ((*it).getMessage());
            ++it;
        }

        int end = juce::jmin (total, position + chunkSize);

        if (it != midi.end())
            end = juce::jmin (end, juce::jmax (position + 1, (*it).samplePosition));

        processChunk (buffer, position, end - position);
        position = end;
    }

    publish();
}

juce::AudioProcessorEditor* BuiltInVocoder::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInVocoder::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInVocoder::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
