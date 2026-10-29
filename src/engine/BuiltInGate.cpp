#include "BuiltInGate.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace gate
{
    double getReductionDb (const Settings& s, double levelDb, bool open)
    {
        if (open)
            return 0.0;

        if (isGate (s.ratio))
            return (double) s.rangeDb;

        return juce::jmax (0.0, juce::jmin ((double) s.rangeDb, ((double) s.thresholdDb - levelDb) * ((double) s.ratio - 1.0)));
    }

    double getKeyResponseDb (const Settings& s, double hz, double sampleRate)
    {
        double db = 0.0;

        if (isKeyLowOn (s.keyLowHz))
            db += eq::getResponseDb ({ true, eq::BandType::lowCut, s.keyLowHz, 0.0f, eq::flatQ, 12 }, hz, sampleRate);

        if (isKeyHighOn (s.keyHighHz))
            db += eq::getResponseDb ({ true, eq::BandType::highCut, s.keyHighHz, 0.0f, eq::flatQ, 12 }, hz, sampleRate);

        return db;
    }

    Run simulate (const Settings& s, const std::vector<float>& levelsDb, double rate)
    {
        Detector detector;
        detector.setRate (rate);
        OpenState state;
        Smoother smoother;
        smoother.setTimes (s.attackMs, s.releaseMs, rate);
        smoother.state = juce::Decibels::decibelsToGain (-(double) s.rangeDb, -200.0);

        const int holdSamples = juce::roundToInt (s.holdMs * rate / 1000.0), flickerSamples = juce::roundToInt (flickerMs * rate / 1000.0);
        Run run;
        run.gainDb.reserve (levelsDb.size());
        run.open.reserve (levelsDb.size());

        for (const float db : levelsDb)
        {
            const double level = detector.process (juce::Decibels::decibelsToGain ((double) db, -200.0));
            const double levelDb = juce::Decibels::gainToDecibels (level, -200.0);
            const bool open = state.process (levelDb, s, holdSamples, flickerSamples);
            const double gain = smoother.process (juce::Decibels::decibelsToGain (-getReductionDb (s, levelDb, open), -200.0));
            run.gainDb.push_back ((float) juce::Decibels::gainToDecibels (gain, -200.0));
            run.open.push_back (open ? 1 : 0);
        }

        run.closes = state.closes;
        run.flickers = state.flickers;
        return run;
    }

    namespace
    {
        float sumDb (double a, double b)
        {
            return (float) (10.0 * std::log10 (std::pow (10.0, a / 10.0) + std::pow (10.0, b / 10.0)));
        }
    }

    std::vector<float> makeDrumHits (double seconds)
    {
        std::vector<float> levels ((size_t) juce::roundToInt (seconds * exampleRate));

        for (size_t i = 0; i < levels.size(); ++i)
        {
            const double t = (double) i / exampleRate;
            double hit = -200.0;

            if (t >= 0.1)
                hit = -6.0 - 200.0 * (std::fmod (t - 0.1, 0.5));

            levels[i] = sumDb (hit, -55.0);
        }

        return levels;
    }

    std::vector<float> makeFadingNote()
    {
        std::vector<float> levels ((size_t) (2.0 * exampleRate));

        for (size_t i = 0; i < levels.size(); ++i)
        {
            const double t = (double) i / exampleRate;
            levels[i] = (float) (-20.0 - 20.0 * t + 1.5 * std::sin (juce::MathConstants<double>::twoPi * 6.0 * t));
        }

        return levels;
    }

    std::vector<float> makeHitStart()
    {
        std::vector<float> levels ((size_t) (0.03 * exampleRate), -60.0f);
        std::fill (levels.begin() + (long) (0.01 * exampleRate), levels.end(), -6.0f);
        return levels;
    }

    std::vector<float> makeTomHit()
    {
        std::vector<float> levels ((size_t) (1.5 * exampleRate));

        for (size_t i = 0; i < levels.size(); ++i)
        {
            const double t = (double) i / exampleRate;
            levels[i] = sumDb (t >= 0.1 ? -6.0 - 40.0 * (t - 0.1) : -200.0, -50.0);
        }

        return levels;
    }

    std::vector<float> makeReverbSnare()
    {
        std::vector<float> levels ((size_t) (1.5 * exampleRate));

        for (size_t i = 0; i < levels.size(); ++i)
        {
            const double t = (double) i / exampleRate;
            double level = -200.0;

            if (t >= 0.1)
                level = 10.0 * std::log10 (std::pow (10.0, (-6.0 - 200.0 * (t - 0.1)) / 10.0) + std::pow (10.0, (-12.0 - 30.0 * (t - 0.1)) / 10.0));

            levels[i] = sumDb (level, -60.0);
        }

        return levels;
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        auto db = [] (float v) { return juce::String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB"; };
        auto ms = [] (float v)
        {
            if (v >= 1000.0f)
                return juce::String (v / 1000.0f, 2) + " s";

            return juce::String (v, v < 1.0f ? 2 : v < 10.0f ? 1 : 0) + " ms";
        };

        switch (k)
        {
            case Knob::threshold:  return db (s.thresholdDb);
            case Knob::hysteresis: return db (s.hysteresisDb);
            case Knob::range:      return s.rangeDb < 0.5f ? juce::String ("0 dB") : "-" + juce::String (juce::roundToInt (s.rangeDb)) + " dB";
            case Knob::ratio:      return isGate (s.ratio) ? juce::String ("Gate") : juce::String (s.ratio, 1) + ":1";
            case Knob::attack:     return ms (s.attackMs);
            case Knob::hold:       return s.holdMs >= 1000.0f ? ms (s.holdMs) : juce::String (juce::roundToInt (s.holdMs)) + " ms";
            case Knob::release:    return ms (s.releaseMs);
            case Knob::keyLow:     return isKeyLowOn (s.keyLowHz) ? juce::String (juce::roundToInt (s.keyLowHz)) + " Hz" : juce::String ("Off");
            case Knob::keyHigh:
                if (! isKeyHighOn (s.keyHighHz))
                    return "Off";

                return s.keyHighHz >= 1000.0f ? juce::String (s.keyHighHz / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (s.keyHighHz)) + " Hz";
        }

        return {};
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            std::vector<Preset> list;
            auto add = [&list] (juce::String name, juce::String why, bool follow, Settings s, std::array<juce::String, numKnobs> notes)
            {
                list.push_back ({ std::move (name), std::move (why), follow, s, std::move (notes) });
            };

            add ("Neutral start", "A plain gate, middle of the road: a place to start turning knobs and hearing what each does.", false,
                 { -40.0f, 4.0f, 80.0f, 10.0f, 0.5f, 20.0f, 100.0f, 20.0f, 20000.0f },
                 { "-40 dB: above typical hiss, below most notes. Raise it until the gaps are quiet.",
                   "It opens at -40 dB and closes at -44 dB, so fading notes don't make it flicker.",
                   "-80 dB: closed is as good as silent.",
                   "Gate: open or closed, nothing in between.",
                   "0.5 ms: fast enough for most sounds, without clicks.",
                   "20 ms, like the fixed hold many gates have: enough to ride over the waves of low notes.",
                   "100 ms: the tail fades over a few hundred milliseconds instead of stopping.",
                   "Off: it listens to the whole sound.",
                   "Off." });

            add ("Kick drum", "A close kick microphone also hears the snare, the toms and the cymbals. The gate opens only for the kick, so "
                              "the rest of the kit leaves the kick's track, and the kick sounds shorter and tighter.", true,
                 { -30.0f, 4.0f, 40.0f, 10.0f, 0.02f, 30.0f, 80.0f, 20.0f, 150.0f },
                 { "Set from the track once it has played: just above the bleed between the kicks.",
                   "4 dB: a clean close as the kick dies away.",
                   "-40 dB: the bleed is gone in the mix, without the hard silence of a full close.",
                   "Gate: a kick should be there or not.",
                   "0.02 ms: open before the beater's click passes; the click is what makes a kick heard on small speakers.",
                   "30 ms: rides over the slow waves of the kick's body, around 60 Hz.",
                   "80 ms: tight. Longer for a boomy kick that should ring.",
                   "Off: the kick's lows are what should open it.",
                   "150 Hz: the snare's body (around 200 Hz) counts 6 dB less, its crack and the cymbals far less, so they don't open it." });

            add ("Snare drum", "A snare microphone sits right next to the hi-hat and hears the kick too. The gate keeps the snare and drops "
                               "most of the rest, so the snare's EQ and compression work on the snare alone.", true,
                 { -30.0f, 4.0f, 30.0f, 10.0f, 0.02f, 40.0f, 150.0f, 150.0f, 5000.0f },
                 { "Just above the loudest bleed (usually the hi-hat), below the softest hits you want. Ghost notes are quiet: check they still come through.",
                   "4 dB.",
                   "-30 dB: turned down, not silent. The overheads fill the gaps with the same kit, so a full close isn't needed.",
                   "Gate.",
                   "0.02 ms: the crack is in the first milliseconds.",
                   "40 ms.",
                   "150 ms: lets the snare's ring and wires fade. Shorter for a dry, tight snare.",
                   "150 Hz: the kick (around 60 Hz) counts 16 dB less, so it doesn't open the snare's gate.",
                   "5 kHz: the hi-hat and cymbals, mostly above 6 kHz, count less." });

            add ("Toms", "Toms play only now and then, and their microphones hear the whole kit the rest of the time. Gating them is one of "
                         "the most common uses of a gate on drums: it takes a lot of bleed and ringing out of the mix.", true,
                 { -30.0f, 6.0f, 30.0f, 10.0f, 0.05f, 100.0f, 350.0f, 20.0f, 2000.0f },
                 { "Above the kit's bleed, below the softest tom hit.",
                   "6 dB: toms ring long and unevenly; more hysteresis keeps the gate from flickering at the end.",
                   "-30 dB.",
                   "Gate.",
                   "0.05 ms.",
                   "100 ms: a low tom swings slowly.",
                   "350 ms: lets the tom ring out. Too short and it sounds choked.",
                   "Off: a floor tom's lows are what should open it.",
                   "2 kHz: cymbals count much less; at 8 kHz, about 24 dB less." });

            add ("Distorted guitar", "High gain turns up the hiss, hum and string noise between riffs as much as the notes. A gate shuts it "
                                     "off the moment you stop playing, which tight riffs need. Put it before DawIt Distortion to follow the "
                                     "playing, or after it to catch the hiss it adds.", true,
                 { -50.0f, 6.0f, 80.0f, 10.0f, 0.5f, 10.0f, 60.0f, 20.0f, 20000.0f },
                 { "Above the hiss, set while nothing plays; check the quietest palm mutes still open it.",
                   "6 dB: distorted notes hold and then drop fast, with noise riding on them; a wide gap closes cleanly.",
                   "-80 dB: silent between riffs.",
                   "Gate: stops should be stops.",
                   "0.5 ms: fast, without a click.",
                   "10 ms: short, so the gate keeps up with fast riffs.",
                   "60 ms: closes quickly after a stop, the way a player mutes the strings.",
                   "Off.",
                   "Off." });

            add ("Lead vocal", "Between phrases, a vocal microphone hears the room, the headphones and the singer moving. An expander turns "
                               "that down by up to 12 dB, gently enough that breaths and soft endings stay natural.", true,
                 { -45.0f, 3.0f, 12.0f, 2.0f, 2.0f, 50.0f, 200.0f, 100.0f, 20000.0f },
                 { "Just under the quietest words, above the room and headphone spill.",
                   "3 dB.",
                   "-12 dB: the gaps quieter, not silent; a vocal with no air around it sounds edited.",
                   "2:1: sounds near the threshold are turned down only a little, so soft endings fade instead of stopping.",
                   "2 ms: a voice starts softly; slower than for drums avoids clicks.",
                   "50 ms: rides over short pauses inside words.",
                   "200 ms: words end naturally.",
                   "100 Hz: rumble and the thump of p and b don't open it.",
                   "Off." });

            add ("Spoken voice", "A podcast or voice-over is all voice and pauses, and in the pauses the room, a computer fan or the street "
                                 "come forward. A firmer expander turns them down by up to 15 dB.", true,
                 { -50.0f, 3.0f, 15.0f, 3.0f, 1.0f, 100.0f, 250.0f, 100.0f, 20000.0f },
                 { "Above the room noise in the pauses, below the quietest words.",
                   "3 dB.",
                   "-15 dB: the pauses clearly quieter, still not dead.",
                   "3:1: firmer than for singing, still not a hard cut.",
                   "1 ms.",
                   "100 ms: stays open over the short gaps between words, so it doesn't pump within a sentence.",
                   "250 ms.",
                   "100 Hz: desk bumps and rumble don't open it.",
                   "Off." });

            add ("Gated reverb", "The 1980s drum sound: a big reverb that stops dead. Put DawIt Reverb before this gate on the drum track, "
                                 "with a large room or hall and plenty of reverb; the gate cuts the tail off after each hit.", false,
                 { -20.0f, 2.0f, 80.0f, 10.0f, 0.02f, 350.0f, 20.0f, 20.0f, 20000.0f },
                 { "High, just under the hits: only the hit and the loudest part of the reverb keep it open. Raise it for a shorter sound.",
                   "2 dB: a sharp, exact close.",
                   "-80 dB: the tail stops dead.",
                   "Gate.",
                   "0.02 ms: the hit comes through whole.",
                   "350 ms: how long the reverb goes on once it falls under the closing level. Shorter for a tighter sound; at 120 BPM a beat is 500 ms.",
                   "20 ms: the cut-off itself. Fast, but not so fast that it clicks.",
                   "Off.",
                   "Off." });

            return list;
        }();

        return presets;
    }

    Analysis analyse (const std::vector<Moment>& moments, const Settings& s, float matchGainDb)
    {
        Analysis a;
        std::vector<float> levels;
        levels.reserve (moments.size());

        for (const auto& m : moments)
        {
            a.flickers += m.flickers;

            // Digital silence (a gap between clips, or a test sound that has ended) says nothing about the noise.
            if (m.detectorDb > -90.0f)
            {
                ++a.heard;
                levels.push_back (m.detectorDb);
            }
        }

        auto whole = [] (double v) { return juce::String (juce::roundToInt (v)) + " dB"; };

        if (a.heard < 500)
        {
            a.advice.add ("Play the track for a few seconds to see what the gate makes of it.");
            return a;
        }

        std::sort (levels.begin(), levels.end());
        auto percentile = [&] (double p) { return levels[(size_t) (p * (double) (levels.size() - 1))]; };
        a.floorDb = percentile (0.10);
        a.loudDb = percentile (0.95);
        a.silentGaps = a.floorDb < -80.0f;

        if (a.silentGaps)
        {
            a.advice.add ("Between the notes this track falls almost silent (its quietest parts are under -80 dB), as software instruments "
                          "usually do. There's no noise for a gate to remove: here it would only shorten the notes. Leave it off, or use it to "
                          "shape the sound on purpose. (Moments of digital silence, such as gaps between clips, are not counted.)");
            return a;
        }

        const double floorDb = a.floorDb, loudDb = a.loudDb, spread = loudDb - floorDb, threshold = s.thresholdDb;

        if (spread >= 12.0)
            a.suggestedDb = juce::jlimit (minThreshold, maxThreshold, (float) juce::roundToInt (floorDb + 6.0));

        const juce::String suggestion = a.suggestedDb ? whole (*a.suggestedDb) : juce::String();

        if (spread < 12.0)
            a.advice.add ("The quiet parts (" + whole (floorDb) + ") are only " + whole (spread) + " under the loud parts (" + whole (loudDb)
                          + "), so there's little room for a threshold between them. Try a ratio of 2:1 to 3:1 with a range of 6-10 dB, so the "
                            "quiet parts are turned down rather than cut.");
        else if (threshold < floorDb + 3.0)
            a.advice.add ("The gate opens on the noise: the quiet parts are at " + whole (floorDb) + " and the threshold at " + whole (threshold)
                          + ". Try about " + suggestion + ".");
        else if (threshold > loudDb - 6.0)
            a.advice.add ("The threshold (" + whole (threshold) + ") is within 6 dB of the loud parts (" + whole (loudDb)
                          + "), so quieter notes may not open it and others get cut short. Try about " + suggestion + ".");
        else
            a.advice.add ("The threshold (" + whole (threshold) + ") sits " + whole (threshold - floorDb) + " above the noise (" + whole (floorDb)
                          + ") and " + whole (loudDb - threshold) + " under the loud parts (" + whole (loudDb)
                          + "), so it should open on the notes and close in the gaps.");

        if (a.flickers >= 3)
            a.advice.add ("It flickered open and shut " + juce::String (a.flickers) + " times in the last 4 seconds (chatter). Try more "
                          "hysteresis (6 dB) or a longer hold (30-50 ms).");

        if (matchGainDb > 1.0f)
            a.advice.add ("Match level would add " + juce::String (matchGainDb, 1) + " dB, so the gate is taking away parts of the notes, not "
                          "only noise. Try a lower threshold or a longer release.");

        return a;
    }
} // namespace gate

//==============================================================================
namespace
{
    using gate::Knob;

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
                gate::Settings s;
                s.thresholdDb = s.hysteresisDb = s.rangeDb = s.ratio = s.attackMs = s.holdMs = s.releaseMs = s.keyLowHz = s.keyHighHz = v;
                return gate::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("gate"))
                    return gate::maxRatio;

                if (text.containsIgnoreCase ("off"))
                    return k == Knob::keyLow ? gate::minKeyLow : k == Knob::keyHigh ? gate::maxKeyHigh : 0.0f;

                float v = text.retainCharacters ("-0123456789.").getFloatValue();

                if (k == Knob::range)
                    v = std::abs (v);

                const auto trimmed = text.trim();
                return trimmed.endsWithIgnoreCase (" khz") || trimmed.endsWithIgnoreCase (" s") ? v * 1000.0f : v;
            });
    }

    /** Runs one sample through a key filter (transposed direct form II). */
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

    constexpr int chunkSize = 32;
}

const char* BuiltInGate::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::threshold:  return "threshold";
        case Knob::hysteresis: return "hysteresis";
        case Knob::range:      return "range";
        case Knob::ratio:      return "ratio";
        case Knob::attack:     return "attack";
        case Knob::hold:       return "hold";
        case Knob::release:    return "release";
        case Knob::keyLow:     return "keyLow";
        case Knob::keyHigh:    return "keyHigh";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInGate::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const gate::Settings d;
    auto add = [&layout] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    add (Knob::threshold, "Threshold", { gate::minThreshold, gate::maxThreshold, 0.1f }, d.thresholdDb);
    add (Knob::hysteresis, "Hysteresis", { 0.0f, gate::maxHysteresis, 0.1f }, d.hysteresisDb);
    add (Knob::range, "Range", { 0.0f, gate::maxRange, 0.1f }, d.rangeDb);
    add (Knob::ratio, "Ratio", logRange (gate::minRatio, gate::maxRatio), d.ratio);
    add (Knob::attack, "Attack", logRange (gate::minAttack, gate::maxAttack), d.attackMs);
    add (Knob::hold, "Hold", { 0.0f, gate::maxHold, 1.0f, 0.4f }, d.holdMs);
    add (Knob::release, "Release", logRange (gate::minRelease, gate::maxRelease), d.releaseMs);
    add (Knob::keyLow, "Key low cut", logRange (gate::minKeyLow, gate::maxKeyLow), d.keyLowHz);
    add (Knob::keyHigh, "Key high cut", logRange (gate::minKeyHigh, gate::maxKeyHigh), d.keyHighHz);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "match", 1 }, "Match level", false));
    return layout;
}

BuiltInGate::BuiltInGate()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItGate", createLayout())
{
    for (int k = 0; k < gate::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    matchLevel = parameters.getRawParameterValue ("match");
}

juce::PluginDescription BuiltInGate::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Dynamics";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44477431;   // "DGt1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInGate::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInGate::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

gate::Settings BuiltInGate::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    gate::Settings s;
    s.thresholdDb = value (Knob::threshold);
    s.hysteresisDb = value (Knob::hysteresis);
    s.rangeDb = value (Knob::range);
    s.ratio = value (Knob::ratio);
    s.attackMs = value (Knob::attack);
    s.holdMs = value (Knob::hold);
    s.releaseMs = value (Knob::release);
    s.keyLowHz = value (Knob::keyLow);
    s.keyHighHz = value (Knob::keyHigh);
    return s;
}

void BuiltInGate::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInGate::setSettings (const gate::Settings& s)
{
    setKnob (Knob::threshold, s.thresholdDb);
    setKnob (Knob::hysteresis, s.hysteresisDb);
    setKnob (Knob::range, s.rangeDb);
    setKnob (Knob::ratio, s.ratio);
    setKnob (Knob::attack, s.attackMs);
    setKnob (Knob::hold, s.holdMs);
    setKnob (Knob::release, s.releaseMs);
    setKnob (Knob::keyLow, s.keyLowHz);
    setKnob (Knob::keyHigh, s.keyHighHz);
}

void BuiltInGate::setMatchingLevel (bool on)
{
    if (auto* p = parameters.getParameter ("match"))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

void BuiltInGate::applyPreset (const gate::Preset& preset, std::optional<float> trackThreshold)
{
    auto s = preset.settings;

    if (preset.followTrack && trackThreshold)
        s.thresholdDb = juce::jlimit (gate::minThreshold, gate::maxThreshold, *trackThreshold);

    setSettings (s);
    setMatchingLevel (false);
}

//==============================================================================
bool BuiltInGate::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInGate::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto s = getSettings();
    clap.prepare (rate);
    detector.setRate (rate);
    detector.level = 0.0;
    openState = {};
    smoother.setTimes (s.attackMs, s.releaseMs, rate);
    smoother.state = juce::Decibels::decibelsToGain (-(double) s.rangeDb, -200.0);   // starts closed
    lowShown = highShown = -1.0f;
    std::fill_n (&lowMemory[0][0][0], 8, 0.0);
    std::fill_n (&highMemory[0][0][0], 8, 0.0);
    powerIn = powerOut = 0.0;
    matchGain.reset (rate, 0.1);
    matchGain.setCurrentAndTargetValue (1.0f);

    momentSamples = juce::jmax (1, juce::roundToInt (gate::momentMs * rate / 1000.0));
    momentCount = lastFlickers = 0;
    momentIn = momentOut = 0.0f;
    momentDetector = momentReduction = 0.0;
    written = 0;
    flickerTotal = 0;
}

void BuiltInGate::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    smoother.setTimes (s.attackMs, s.releaseMs, rate);
    const int holdSamples = juce::roundToInt (s.holdMs * rate / 1000.0), flickerSamples = juce::roundToInt (gate::flickerMs * rate / 1000.0);
    const bool lowOn = gate::isKeyLowOn (s.keyLowHz), highOn = gate::isKeyHighOn (s.keyHighHz);

    if (lowOn && std::abs (s.keyLowHz - lowShown) > 0.01f)
    {
        if (lowShown < 0.0f)
            std::fill_n (&lowMemory[0][0][0], 8, 0.0);

        lowShown = s.keyLowHz;
        lowStages = eq::makeStages ({ true, eq::BandType::lowCut, s.keyLowHz, 0.0f, eq::flatQ, 12 }, rate);
    }
    else if (! lowOn)
    {
        lowShown = -1.0f;
    }

    if (highOn && std::abs (s.keyHighHz - highShown) > 0.01f)
    {
        if (highShown < 0.0f)
            std::fill_n (&highMemory[0][0][0], 8, 0.0);

        highShown = s.keyHighHz;
        highStages = eq::makeStages ({ true, eq::BandType::highCut, s.keyHighHz, 0.0f, eq::flatQ, 12 }, rate);
    }
    else if (! highOn)
    {
        highShown = -1.0f;
    }

    const bool removed = listenToRemoved.load(), keyListen = listenToKey.load(), matching = isMatchingLevel() && ! removed && ! keyListen;
    const double powerCoefficient = 1.0 - std::exp (-chunkSize / (0.5 * rate));   // about half a second

    float chunkInput[chunkSize];
    double chunkDetector[chunkSize], chunkReduction[chunkSize];
    int chunkFlickers[chunkSize];
    bool chunkOpen[chunkSize];

    for (int start = 0; start < total; start += chunkSize)
    {
        const int n = juce::jmin (chunkSize, total - start);
        double sumIn = 0.0, sumOut = 0.0;

        for (int i = 0; i < n; ++i)
        {
            // What the gate listens to: the channels together (the loudest of them), through the key filters.
            double level = 0.0, inPeak = 0.0;
            double heardSamples[2] {};

            for (int ch = 0; ch < numChannels; ++ch)
            {
                double x = buffer.getSample (ch, start + i);
                inPeak = juce::jmax (inPeak, std::abs (x));
                sumIn += x * x;

                if (lowOn)
                    x = runStages (lowStages, lowMemory[ch], x);

                if (highOn)
                    x = runStages (highStages, highMemory[ch], x);

                heardSamples[ch] = x;
                level = juce::jmax (level, std::abs (x));
            }

            const double detected = detector.process (level);
            const double detectedDb = juce::Decibels::gainToDecibels (detected, -120.0);
            const bool open = openState.process (detectedDb, s, holdSamples, flickerSamples);
            const double reduction = gate::getReductionDb (s, detectedDb, open);
            const double gain = smoother.process (juce::Decibels::decibelsToGain (-reduction, -200.0));

            chunkInput[i] = (float) inPeak;
            chunkDetector[i] = detected;
            chunkReduction[i] = juce::jmax (0.0, -juce::Decibels::gainToDecibels (gain, -200.0));
            chunkFlickers[i] = openState.flickers;
            chunkOpen[i] = open;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch, start + i);
                const float dry = *data, wet = dry * (float) gain;
                sumOut += (double) wet * wet;
                *data = keyListen ? (float) heardSamples[ch] : removed ? dry - wet : wet;
            }
        }

        // Match level: the gain that makes the last half second as loud after as before. Silence keeps the last value.
        powerIn += powerCoefficient * (sumIn / (n * numChannels) - powerIn);
        powerOut += powerCoefficient * (sumOut / (n * numChannels) - powerOut);

        if (powerIn > 1.0e-8 && powerOut > 1.0e-12)
            matchGainDb = juce::jlimit (-24.0f, 24.0f, (float) (10.0 * std::log10 (powerIn / powerOut)));

        matchGain.setTargetValue (matching ? juce::Decibels::decibelsToGain (matchGainDb.load()) : 1.0f);

        for (int i = 0; i < n; ++i)
        {
            const float matchValue = matchGain.getNextValue();
            float outPeak = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch, start + i);
                *data *= matchValue;
                outPeak = juce::jmax (outPeak, std::abs (*data));
            }

            if (numChannels == 1 && buffer.getNumChannels() > 1)
                buffer.setSample (1, start + i, buffer.getSample (0, start + i));

            // One moment of the history every 2 ms.
            momentIn = juce::jmax (momentIn, chunkInput[i]);
            momentOut = juce::jmax (momentOut, outPeak);
            momentDetector = juce::jmax (momentDetector, chunkDetector[i]);
            momentReduction = juce::jmax (momentReduction, chunkReduction[i]);

            if (++momentCount >= momentSamples)
            {
                const auto index = (size_t) (written.load (std::memory_order_relaxed) % gate::numMoments);
                auto& slot = slots[index];
                const float detectorDb = (float) juce::Decibels::gainToDecibels (momentDetector, -100.0);
                const int flickers = chunkFlickers[i] - lastFlickers;
                slot.input.store (juce::Decibels::gainToDecibels (momentIn, -100.0f), std::memory_order_relaxed);
                slot.output.store (juce::Decibels::gainToDecibels (momentOut, -100.0f), std::memory_order_relaxed);
                slot.detector.store (detectorDb, std::memory_order_relaxed);
                slot.reduction.store ((float) momentReduction, std::memory_order_relaxed);
                slot.openAndFlickers.store (flickers * 2 + (chunkOpen[i] ? 1 : 0), std::memory_order_relaxed);
                written.fetch_add (1, std::memory_order_release);
                flickerTotal.fetch_add (flickers, std::memory_order_relaxed);

                // For a small picture elsewhere: keep the loudest moment until it's taken.
                if (detectorDb > previewDetectorDb.load (std::memory_order_relaxed))
                {
                    previewReductionDb.store ((float) momentReduction, std::memory_order_relaxed);
                    previewOpen.store (chunkOpen[i], std::memory_order_relaxed);
                    previewDetectorDb.store (detectorDb, std::memory_order_release);
                }

                lastFlickers = chunkFlickers[i];
                momentCount = 0;
                momentIn = momentOut = 0.0f;
                momentDetector = momentReduction = 0.0;
            }
        }
    }
}

std::vector<gate::Moment> BuiltInGate::getRecent() const
{
    const long long end = written.load (std::memory_order_acquire);
    const long long count = juce::jmin ((long long) gate::numMoments, end);
    std::vector<gate::Moment> moments;
    moments.reserve ((size_t) count);

    for (long long i = end - count; i < end; ++i)
    {
        const auto& slot = slots[(size_t) (i % gate::numMoments)];
        gate::Moment m;
        m.inputDb = slot.input.load (std::memory_order_relaxed);
        m.outputDb = slot.output.load (std::memory_order_relaxed);
        m.detectorDb = slot.detector.load (std::memory_order_relaxed);
        m.reductionDb = slot.reduction.load (std::memory_order_relaxed);
        const int packed = slot.openAndFlickers.load (std::memory_order_relaxed);
        m.open = (packed & 1) != 0;
        m.flickers = packed >> 1;
        moments.push_back (m);
    }

    return moments;
}

BuiltInGate::Preview BuiltInGate::takePreview()
{
    Preview p;
    p.detectorDb = previewDetectorDb.exchange (-100.0f, std::memory_order_acquire);
    p.reductionDb = previewReductionDb.load (std::memory_order_relaxed);
    p.open = previewOpen.load (std::memory_order_relaxed);
    return p;
}

juce::AudioProcessorEditor* BuiltInGate::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInGate::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInGate::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
