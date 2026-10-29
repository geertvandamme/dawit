#include "BuiltInCompressor.h"
#include "BuiltInFormat.h"

namespace daw
{
namespace comp
{
    double getReductionDb (const Settings& s, double levelDb)
    {
        const double over = levelDb - s.thresholdDb, slope = 1.0 - 1.0 / juce::jmax (1.0, (double) s.ratio), knee = s.kneeDb;

        if (knee > 0.0 && 2.0 * std::abs (over) <= knee)
            return slope * (over + knee / 2.0) * (over + knee / 2.0) / (2.0 * knee);

        return over > 0.0 ? slope * over : 0.0;
    }

    float findThreshold (const Settings& s, const std::vector<float>& levelsDb, double rate, double reductionDb)
    {
        auto mostAt = [&] (double threshold)
        {
            auto t = s;
            t.thresholdDb = (float) threshold;
            Ballistics b;
            b.setTimes (t.attackMs, t.releaseMs, rate);
            double most = 0.0;

            for (auto db : levelsDb)
                most = juce::jmax (most, b.process (getReductionDb (t, db)));

            return most;
        };

        // The reduction only grows as the threshold goes down, so halve the range until it's found.
        double low = minThreshold, high = maxThreshold;

        if (mostAt (low) < reductionDb)
            return minThreshold;

        if (mostAt (high) >= reductionDb)
            return maxThreshold;

        for (int i = 0; i < 30; ++i)
        {
            const double middle = 0.5 * (low + high);
            (mostAt (middle) >= reductionDb ? low : high) = middle;
        }

        return (float) (0.5 * (low + high));
    }

    double getCoefficient (double ms, double sampleRate)
    {
        return std::exp (-1000.0 / (juce::jmax (0.001, ms) * sampleRate));
    }

    void Ballistics::setTimes (double attackMs, double releaseMs, double sampleRate)
    {
        attack = getCoefficient (attackMs, sampleRate);
        release = getCoefficient (releaseMs, sampleRate);
    }

    std::vector<float> simulateReduction (const Settings& s, const std::vector<float>& levelDb, double sampleRate)
    {
        Ballistics b;
        b.setTimes (s.attackMs, s.releaseMs, sampleRate);
        const double rmsCoefficient = getCoefficient (rmsWindowMs, sampleRate);
        double power = 0.0;
        std::vector<float> reduction;
        reduction.reserve (levelDb.size());

        for (auto db : levelDb)
        {
            double detected = db;

            if (s.detection == Detection::rms)
            {
                // A tone's RMS is 3 dB under its peak; averaged, it also rises and falls more slowly.
                const double tone = juce::Decibels::decibelsToGain ((double) db) / std::sqrt (2.0);
                power = tone * tone + rmsCoefficient * (power - tone * tone);
                detected = juce::Decibels::gainToDecibels (std::sqrt (power), -120.0);
            }

            reduction.push_back ((float) b.process (getReductionDb (s, detected)));
        }

        return reduction;
    }

    juce::String formatKnob (Knob k, const Settings& s)
    {
        auto db = [] (float v, bool sign) { return (sign && v >= 0.05f ? "+" : "") + juce::String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB"; };
        auto ms = [] (float v)
        {
            if (v >= 1000.0f)
                return juce::String (v / 1000.0f, 2) + " s";

            return juce::String (v, v < 1.0f ? 2 : v < 10.0f ? 1 : 0) + " ms";
        };

        switch (k)
        {
            case Knob::threshold: return db (s.thresholdDb, false);
            case Knob::ratio:     return juce::String (s.ratio, s.ratio < 10.0f ? 1 : 0) + ":1";
            case Knob::knee:      return db (s.kneeDb, false);
            case Knob::attack:    return ms (s.attackMs);
            case Knob::release:   return ms (s.releaseMs);
            case Knob::makeup:    return db (s.makeupDb, true);
            case Knob::mix:       return juce::String (juce::roundToInt (s.mixPercent)) + "%";
            case Knob::sidechain: return isSidechainFilterOn (s.sidechainHz) ? juce::String (juce::roundToInt (s.sidechainHz)) + " Hz" : juce::String ("Off");
            case Knob::detection: return s.detection == Detection::rms ? "RMS" : "Peak";
        }

        return {};
    }

    const std::vector<Preset>& getPresets()
    {
        static const std::vector<Preset> presets = []
        {
            using D = Detection;
            std::vector<Preset> list;
            auto add = [&list] (juce::String name, juce::String why, float target, Settings s, std::array<juce::String, numKnobs> notes)
            {
                list.push_back ({ std::move (name), std::move (why), target, s, std::move (notes) });
            };

            add ("Neutral start", "Middle-of-the-road settings that do a little to almost anything: a place to start turning knobs and "
                                  "hearing what each one does.", 4.0f,
                 { -18.0f, 3.0f, 6.0f, 10.0f, 150.0f, 0.0f, 100.0f, 20.0f, D::peak },
                 { "Only what's louder than -18 dB is turned down. Lower it until the loudest parts get 3-6 dB of reduction.",
                   "3:1 is moderate: clearly in control, still natural.",
                   "A 6 dB soft knee: the compression comes in gradually around the threshold instead of all at once.",
                   "10 ms lets the very start of notes through and catches the rest: a good middle.",
                   "150 ms lets go fairly soon, without pumping on most material.",
                   "None yet: raise it by about as much as the meter shows on the loud parts, or turn on Match level.",
                   "100%: only the compressed signal.",
                   "Off: the compressor listens to the whole track.",
                   "Peak: reacts to every peak, the most direct." });

            add ("Lead vocal", "A voice jumps between whispers and shouts within a phrase. Compression evens that out, so every word "
                               "stays clear above the band without riding the fader.", 5.0f,
                 { -20.0f, 3.0f, 6.0f, 5.0f, 80.0f, 3.0f, 100.0f, 100.0f, D::peak },
                 { "Set so the loudest phrases get 4-6 dB of reduction and the quiet words little or none.",
                   "3:1 evens out the phrases while it still sounds like a person singing. 4:1 to 6:1 for rock and rap that should sit "
                   "right in your face, 2:1 for a gentle ballad.",
                   "A soft knee makes the compression come in gradually, which hides it on a voice.",
                   "5 ms catches loud syllables quickly but lets the first moment of the consonants (t, k, p) through, so the words stay "
                   "crisp. Under 1 ms the voice starts to sound squashed and lisping.",
                   "80 ms lets go between words, so each word is treated on its own. Much shorter makes the breaths and the room swell up "
                   "between words; much longer keeps quiet words turned down after a loud one.",
                   "Brings back the level the compression took off, so the vocal is as loud as before but steadier.",
                   "100%. Pop vocals sometimes use 70-80% to keep a little of the natural life.",
                   "At 100 Hz: plosives (p and b) and handling noise are mostly lows. Without the filter they'd make the compressor duck "
                   "the next word.",
                   "Peak, to catch a sudden loud syllable at once." });

            add ("Backing vocals", "Backing vocals should sit steadily behind the lead, like one instrument. Heavier compression than the "
                                   "lead takes away their ups and downs, so no single voice pokes out.", 7.0f,
                 { -24.0f, 5.0f, 3.0f, 2.0f, 100.0f, 5.0f, 100.0f, 120.0f, D::peak },
                 { "Lower than for a lead: 6-8 dB of reduction on the loudest parts.",
                   "5:1: firm, so the level hardly moves. That's what makes them blend.",
                   "A small knee: firm control, still smooth.",
                   "2 ms: fast, so the starts of words don't stick out. The lead keeps the attitude, the backing vocals stay smooth.",
                   "100 ms: back between words, without pumping.",
                   "Brings them back to the level they had; then set the blend with the fader.",
                   "100%: they should be even, not natural.",
                   "At 120 Hz: backing vocals have little useful low end, and the plosives shouldn't trigger the compressor.",
                   "Peak: no syllable gets through louder than the rest." });

            add ("Spoken voice", "Speech, a podcast or a voice-over should be understandable at any listening level, in a car or on "
                                 "earbuds. Compression brings the quiet words up and holds back the loud laughs.", 6.0f,
                 { -24.0f, 3.0f, 6.0f, 10.0f, 150.0f, 5.0f, 100.0f, 100.0f, D::rms },
                 { "So that normal speech gets about 4-6 dB of reduction and a laugh or shout more.",
                   "3:1 makes speech even without sounding processed. Radio uses more (6:1 and up) with a limiter after it.",
                   "Soft, so the compression isn't heard coming and going.",
                   "10 ms keeps the consonants clear.",
                   "150 ms: slow enough that the pauses between words don't rush up.",
                   "Brings everything up: with the loud parts held back, the voice as a whole can be louder.",
                   "100%.",
                   "At 100 Hz: a deep voice close to the microphone has a lot of low end (the proximity effect) that shouldn't drive "
                   "the compressor.",
                   "RMS: follows how loud the speech sounds over time rather than each peak: smoother, more like the ear." });

            add ("Kick drum", "A kick is a short click followed by a low boom. With a slow attack, compression lets the click through "
                              "and turns the boom down: the kick sounds punchier and tighter.", 4.0f,
                 { -18.0f, 4.0f, 0.0f, 25.0f, 60.0f, 3.0f, 100.0f, 20.0f, D::peak },
                 { "So each kick gets 3-6 dB of reduction.",
                   "4:1: a clear grip on the body of the kick.",
                   "Hard (0 dB): the compressor grabs decisively, which adds punch on drums.",
                   "25 ms: the click is over in about 5-10 ms, so it passes while the compressor is still getting going; only the boom "
                   "after it is turned down. That contrast is the punch. At 1 ms you'd flatten the click and the kick would sound dull.",
                   "60 ms: back to 0 before the next kick. At 120 BPM a kick on every beat comes every 500 ms, on eighth notes every 250 ms. "
                   "A release longer than the gap makes the next kick start already turned down.",
                   "Brings the level back up, now with more click compared to the boom.",
                   "100%.",
                   "Off: the kick's low end is what should drive the compressor.",
                   "Peak: a kick is all peak, and the compressor has to see it." });

            add ("Snare drum", "A snare has a crack (the stick hitting the head), then a body and the rattle of the wires. Compression evens "
                               "out soft and hard hits and, through the attack, brings out the crack or the body.", 5.0f,
                 { -20.0f, 4.0f, 2.0f, 10.0f, 100.0f, 3.0f, 100.0f, 20.0f, D::peak },
                 { "So the hits get 4-6 dB of reduction.",
                   "4:1: softer and harder hits come closer together.",
                   "Nearly hard: decisive, like on the kick.",
                   "10 ms lets the crack through. Longer (20-30 ms) gives more crack and snap; shorter (1-3 ms) takes the crack off and "
                   "brings up the body and the wires: fatter, but less punchy.",
                   "100 ms: back before the next hit. A shorter release brings the ring and the room up after each hit, for more "
                   "sustain; some like that sound.",
                   "Brings the level back.",
                   "100%.",
                   "Off.",
                   "Peak: the crack is a sharp peak." });

            add ("Drum bus", "Light compression on the whole kit ties the drums together: they move together as one instrument. This "
                             "is the 'glue'.", 2.5f,
                 { -16.0f, 2.0f, 6.0f, 30.0f, 150.0f, 2.0f, 100.0f, 120.0f, D::peak },
                 { "Only 2-3 dB of reduction on the loudest hits.",
                   "2:1: gentle, glue rather than squashing.",
                   "Soft: the compressor works smoothly across all the drums.",
                   "30 ms: slow enough for every hit to keep its attack.",
                   "150 ms. Ideally the reduction moves with the groove: back to almost 0 just before the next beat. Watch the red in "
                   "the history graph.",
                   "Brings back the 2-3 dB.",
                   "100%.",
                   "At 120 Hz. Without it the kick, which has the most energy, drives most of the compression, and every kick ducks the "
                   "snare and cymbals: pumping. The filter makes the compressor listen to the whole kit more evenly.",
                   "Peak." });

            add ("Parallel drums", "Squash the drums hard and blend that in under the untouched drums. The untouched part keeps the punch "
                                   "of every hit, the squashed part adds weight, room and energy between the hits. Also called New York "
                                   "compression.", 12.0f,
                 { -30.0f, 8.0f, 0.0f, 1.0f, 100.0f, 8.0f, 35.0f, 20.0f, D::peak },
                 { "Low: 10-15 dB of reduction. On its own it would sound terrible; blended in, it sounds big.",
                   "8:1: heavy, almost limiting.",
                   "Hard.",
                   "1 ms: flattens every hit, so what's left is the body and the room.",
                   "100 ms: comes back up quickly between the hits, which is where the energy comes from.",
                   "Brings the squashed part up so it's strong enough to be heard under the untouched drums.",
                   "This is the trick: 35% squashed drums under 65% untouched drums. More for energy, less for natural.",
                   "Off.",
                   "Peak." });

            add ("Bass", "On a bass, different notes come out at very different levels: some strings and frets are louder, and some "
                         "notes boom in the room. Compression evens them out, so the bass holds the low end steadily under the song.", 5.0f,
                 { -20.0f, 4.0f, 6.0f, 20.0f, 150.0f, 3.0f, 100.0f, 20.0f, D::rms },
                 { "So the loudest notes get 4-6 dB of reduction.",
                   "4:1: notes and fingers even out. Slap bass often uses more.",
                   "Soft.",
                   "20 ms: lets the pluck or pick through so you hear each note start, then holds the sustain steady.",
                   "150 ms. A low E is 41 Hz: one wave of it lasts 24 ms. A release only a few waves long makes the compressor follow "
                   "the waveform itself, which distorts the bass. 100-300 ms is smooth.",
                   "Brings it back to the level it had, now steadier.",
                   "100%.",
                   "Off: the lows are the bass.",
                   "RMS: reacts to how loud the notes are over time rather than to each peak: smoother for a sustained instrument." });

            add ("Acoustic guitar", "Strummed acoustic guitar is very dynamic: strumming a little harder makes peaks that jump out. "
                                    "Compression keeps the strumming even, so it sits in the mix as a steady bed.", 4.0f,
                 { -20.0f, 3.0f, 6.0f, 15.0f, 120.0f, 3.0f, 100.0f, 80.0f, D::rms },
                 { "So the loudest strums get 3-5 dB of reduction.",
                   "3:1: steady while it still sounds acoustic.",
                   "Soft, to stay natural.",
                   "15 ms keeps the pick attack of each strum.",
                   "120 ms: back between the strums.",
                   "Brings back the level.",
                   "100%.",
                   "At 80 Hz: the boom of the body shouldn't decide how much is compressed.",
                   "RMS: follows the strumming as a whole rather than each string." });

            add ("Electric guitar", "A clean electric guitar has sharp picking peaks; compression evens them out and adds sustain. A "
                                    "distorted guitar is already compressed by the distortion and rarely needs more.", 4.0f,
                 { -18.0f, 4.0f, 3.0f, 5.0f, 100.0f, 3.0f, 100.0f, 80.0f, D::peak },
                 { "So the loudest notes get 3-5 dB of reduction.",
                   "4:1: even notes, longer sustain: the classic funk and country sound goes up to 8:1.",
                   "Small.",
                   "5 ms: quick, for an even, smooth sound. Slower (20 ms) keeps more of the pick.",
                   "100 ms: notes ring out longer as the compressor lets go.",
                   "Brings back the level.",
                   "100%.",
                   "At 80 Hz: the low strings shouldn't push the higher notes down.",
                   "Peak: catches the picking." });

            add ("Piano", "Piano dynamics are part of the music: a pianist plays softer and louder on purpose. Gentle compression only "
                          "keeps the loudest chords from jumping out in a mix.", 2.5f,
                 { -18.0f, 2.0f, 10.0f, 25.0f, 250.0f, 2.0f, 100.0f, 20.0f, D::rms },
                 { "Only 2-3 dB of reduction on the loudest chords.",
                   "2:1: gentle, so the playing keeps its expression.",
                   "Very soft (10 dB): the compression comes in so gradually that you hardly hear it.",
                   "25 ms keeps the hammer attack.",
                   "250 ms: slow, so the decay of the notes isn't pulled up.",
                   "Brings back the level.",
                   "100%.",
                   "Off.",
                   "RMS: follows the loudness of the playing like the ear does." });

            add ("Pad or strings", "Pads and strings are already smooth. Light, slow compression only holds their level steady as the "
                                   "chords change.", 2.5f,
                 { -20.0f, 2.0f, 10.0f, 50.0f, 400.0f, 2.0f, 100.0f, 20.0f, D::rms },
                 { "Only 2-3 dB of reduction.",
                   "2:1: gentle.",
                   "Very soft.",
                   "50 ms: slow. A pad has no attack to protect, but a fast attack would make it breathe.",
                   "400 ms: slow, so the level doesn't move audibly.",
                   "Brings back the level.",
                   "100%.",
                   "Off.",
                   "RMS: smooth, for a smooth sound." });

            add ("Mix bus", "On the whole mix, a little compression makes everything move together and sound finished. Very little: "
                            "1-2 dB. More quickly makes the mix sound small and flat.", 1.5f,
                 { -12.0f, 1.5f, 8.0f, 30.0f, 200.0f, 1.0f, 100.0f, 80.0f, D::rms },
                 { "Only 1-2 dB of reduction on the loudest parts.",
                   "1.5:1: barely compressing, just holding things together.",
                   "Soft.",
                   "30 ms keeps the punch of the drums.",
                   "200 ms; try fitting it to the tempo, so the mix breathes with the song.",
                   "Brings back the 1-2 dB.",
                   "100%.",
                   "At 80 Hz: without it the kick and bass drive the compressor, and the whole mix ducks on every kick.",
                   "RMS: follows the loudness of the music." });

            add ("Limiter", "A limiter stops the level from going over the threshold: a ratio of 20:1 and the fastest attack. Use it to "
                            "catch the odd peak, for example at the end of a mix, not to make things loud.", 2.0f,
                 { -6.0f, 20.0f, 0.0f, 0.1f, 50.0f, 0.0f, 100.0f, 20.0f, D::peak },
                 { "The highest level you want to let through.",
                   "20:1: what goes over is all but stopped.",
                   "Hard: nothing over the threshold gets a pass.",
                   "0.1 ms, as fast as it goes. Real limiters also look a few milliseconds ahead so not even the first moment of a peak "
                   "passes; DawIt Compressor doesn't, so the very fastest peaks still get a little through.",
                   "50 ms: lets go quickly, so you only hear it on the peaks.",
                   "0 dB: a limiter is for safety, not for loudness.",
                   "100%.",
                   "Off: every peak counts.",
                   "Peak: it has to see every peak." });

            return list;
        }();

        return presets;
    }
} // namespace comp

//==============================================================================
namespace
{
    using comp::Knob;

    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v)
                 {
                     return juce::jlimit (s, e, std::round (v * 100.0f) / 100.0f);
                 } };
    }

    /** How a parameter reads in the editor and to hosts: the same as formatKnob. */
    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([k] (float v, int)
            {
                comp::Settings s;
                s.thresholdDb = s.kneeDb = s.makeupDb = s.ratio = s.attackMs = s.releaseMs = s.mixPercent = s.sidechainHz = v;
                return comp::formatKnob (k, s);
            })
            .withValueFromStringFunction ([k] (const juce::String& text)
            {
                if (k == Knob::sidechain && text.containsIgnoreCase ("off"))
                    return comp::minSidechain;

                const float v = text.retainCharacters ("-0123456789.").getFloatValue();
                return text.trim().endsWithIgnoreCase (" s") ? v * 1000.0f : v;
            });
    }

    constexpr int chunkSize = BuiltInCompressor::historyChunk;
}

const char* BuiltInCompressor::getParameterId (Knob k)
{
    switch (k)
    {
        case Knob::threshold: return "threshold";
        case Knob::ratio:     return "ratio";
        case Knob::knee:      return "knee";
        case Knob::attack:    return "attack";
        case Knob::release:   return "release";
        case Knob::makeup:    return "makeup";
        case Knob::mix:       return "mix";
        case Knob::sidechain: return "sidechain";
        case Knob::detection: return "detection";
    }

    return "";
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInCompressor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const comp::Settings d;
    auto add = [&layout] (Knob k, const char* name, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { getParameterId (k), 1 }, name, range, value, knobText (k)));
    };

    add (Knob::threshold, "Threshold", { comp::minThreshold, comp::maxThreshold, 0.1f }, d.thresholdDb);
    add (Knob::ratio, "Ratio", logRange (comp::minRatio, comp::maxRatio), d.ratio);
    add (Knob::knee, "Knee", { 0.0f, comp::maxKnee, 0.1f }, d.kneeDb);
    add (Knob::attack, "Attack", logRange (comp::minAttack, comp::maxAttack), d.attackMs);
    add (Knob::release, "Release", logRange (comp::minRelease, comp::maxRelease), d.releaseMs);
    add (Knob::makeup, "Makeup", { comp::minMakeup, comp::maxMakeup, 0.1f }, d.makeupDb);
    add (Knob::mix, "Mix", { 0.0f, 100.0f, 1.0f }, d.mixPercent);
    add (Knob::sidechain, "Sidechain filter", logRange (comp::minSidechain, comp::maxSidechain), d.sidechainHz);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { getParameterId (Knob::detection), 1 }, "Detection",
                                                              juce::StringArray { "Peak", "RMS" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "match", 1 }, "Match level", false));
    return layout;
}

BuiltInCompressor::BuiltInCompressor()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItCompressor", createLayout())
{
    for (int k = 0; k < comp::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (getParameterId ((Knob) k));

    matchLevel = parameters.getRawParameterValue ("match");
}

juce::PluginDescription BuiltInCompressor::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Dynamics";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44436d31;   // "DCm1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInCompressor::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInCompressor::getKnobParameter (Knob k) const
{
    return parameters.getParameter (getParameterId (k));
}

comp::Settings BuiltInCompressor::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    comp::Settings s;
    s.thresholdDb = value (Knob::threshold);
    s.ratio = value (Knob::ratio);
    s.kneeDb = value (Knob::knee);
    s.attackMs = value (Knob::attack);
    s.releaseMs = value (Knob::release);
    s.makeupDb = value (Knob::makeup);
    s.mixPercent = value (Knob::mix);
    s.sidechainHz = value (Knob::sidechain);
    s.detection = value (Knob::detection) >= 0.5f ? comp::Detection::rms : comp::Detection::peak;
    return s;
}

void BuiltInCompressor::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void BuiltInCompressor::setSettings (const comp::Settings& s)
{
    setKnob (Knob::threshold, s.thresholdDb);
    setKnob (Knob::ratio, s.ratio);
    setKnob (Knob::knee, s.kneeDb);
    setKnob (Knob::attack, s.attackMs);
    setKnob (Knob::release, s.releaseMs);
    setKnob (Knob::makeup, s.makeupDb);
    setKnob (Knob::mix, s.mixPercent);
    setKnob (Knob::sidechain, s.sidechainHz);
    setKnob (Knob::detection, s.detection == comp::Detection::rms ? 1.0f : 0.0f);
}

void BuiltInCompressor::setMatchingLevel (bool on)
{
    if (auto* p = parameters.getParameter ("match"))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

void BuiltInCompressor::applyPreset (const comp::Preset& preset)
{
    setSettings (preset.settings);
    setMatchingLevel (false);
}

//==============================================================================
bool BuiltInCompressor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInCompressor::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto s = getSettings();
    ballistics.setTimes (s.attackMs, s.releaseMs, rate);
    ballistics.state = 0.0;
    rmsCoefficient = comp::getCoefficient (comp::rmsWindowMs, rate);
    rmsState = 0.0;
    sidechainShown = -1.0f;
    std::fill_n (&sidechainMemory[0][0][0], 8, 0.0);
    powerIn = powerOut = 0.0;

    makeupGain.reset (rate, 0.05);
    makeupGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (s.makeupDb));
    mix.reset (rate, 0.05);
    mix.setCurrentAndTargetValue (s.mixPercent / 100.0f);
    matchGain.reset (rate, 0.1);
    matchGain.setCurrentAndTargetValue (1.0f);
}

namespace
{
    /** Runs one sample through the sidechain filter (transposed direct form II). */
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

void BuiltInCompressor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

    const auto s = getSettings();
    ballistics.setTimes (s.attackMs, s.releaseMs, rate);
    const bool filterOn = comp::isSidechainFilterOn (s.sidechainHz);

    if (filterOn && std::abs (s.sidechainHz - sidechainShown) > 0.01f)
    {
        if (sidechainShown < 0.0f)
            std::fill_n (&sidechainMemory[0][0][0], 8, 0.0);

        sidechainShown = s.sidechainHz;
        sidechainStages = eq::makeStages ({ true, eq::BandType::lowCut, s.sidechainHz, 0.0f, eq::flatQ, 12 }, rate);
    }
    else if (! filterOn)
    {
        sidechainShown = -1.0f;
    }

    const bool removed = listenToRemoved.load(), matching = isMatchingLevel() && ! removed;
    makeupGain.setTargetValue (matching ? 1.0f : juce::Decibels::decibelsToGain (s.makeupDb));
    mix.setTargetValue (s.mixPercent / 100.0f);
    const double powerCoefficient = 1.0 - std::exp (-chunkSize / (0.5 * rate));   // about half a second

    for (int start = 0; start < total; start += chunkSize)
    {
        const int n = juce::jmin (chunkSize, total - start);
        HistoryPoint point;
        float inPeak = 0.0f, outPeak = 0.0f;
        double detectorPeak = 0.0, mostReduction = 0.0, sumIn = 0.0, sumOut = 0.0;

        for (int i = 0; i < n; ++i)
        {
            // What the compressor listens to: the channels together, through the sidechain filter.
            double level = 0.0, power = 0.0;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const double x = buffer.getSample (ch, start + i);
                const double heard = filterOn ? runStages (sidechainStages, sidechainMemory[ch], x) : x;
                level = juce::jmax (level, std::abs (heard));
                power += heard * heard;
                sumIn += x * x;
                inPeak = juce::jmax (inPeak, (float) std::abs (x));
            }

            if (s.detection == comp::Detection::rms)
            {
                rmsState = power / numChannels + rmsCoefficient * (rmsState - power / numChannels);
                level = std::sqrt (rmsState);
            }

            const double levelDb = juce::Decibels::gainToDecibels (level, -120.0);
            const double reduction = ballistics.process (comp::getReductionDb (s, levelDb));
            const float gain = (float) juce::Decibels::decibelsToGain (-reduction, -200.0);
            const float wetAmount = mix.getNextValue(), makeup = makeupGain.getNextValue();
            detectorPeak = juce::jmax (detectorPeak, level);
            mostReduction = juce::jmax (mostReduction, reduction);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch, start + i);
                const float dry = *data, wet = dry * gain;
                const float blended = dry + wetAmount * (wet - dry);   // without makeup, for match level
                sumOut += (double) blended * blended;
                *data = removed ? dry - wet : dry + wetAmount * (wet * makeup - dry);
            }
        }

        // Match level: the makeup that makes the last half second as loud after as before. Silence keeps the last value.
        powerIn += powerCoefficient * (sumIn / (n * numChannels) - powerIn);
        powerOut += powerCoefficient * (sumOut / (n * numChannels) - powerOut);

        if (powerIn > 1.0e-8 && powerOut > 1.0e-12)
            matchGainDb = juce::jlimit (-24.0f, 24.0f, (float) (10.0 * std::log10 (powerIn / powerOut)));

        matchGain.setTargetValue (matching ? juce::Decibels::decibelsToGain (matchGainDb.load()) : 1.0f);

        for (int i = 0; i < n; ++i)
        {
            const float gain = matchGain.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch, start + i);
                *data *= gain;
                outPeak = juce::jmax (outPeak, std::abs (*data));
            }
        }

        if (numChannels == 1 && buffer.getNumChannels() > 1)
            buffer.copyFrom (1, start, buffer, 0, start, n);

        point.inputDb = juce::Decibels::gainToDecibels (inPeak, -100.0f);
        point.outputDb = juce::Decibels::gainToDecibels (outPeak, -100.0f);
        point.detectorDb = (float) juce::Decibels::gainToDecibels (detectorPeak, -100.0);
        point.reductionDb = (float) mostReduction;

        // For a small picture elsewhere: keep the loudest point until it's taken.
        if (point.detectorDb > previewDetectorDb.load (std::memory_order_relaxed))
        {
            previewReductionDb.store (point.reductionDb, std::memory_order_relaxed);
            previewDetectorDb.store (point.detectorDb, std::memory_order_release);
        }

        // For the graphs: drop what doesn't fit while nobody reads.
        int start1, size1, start2, size2;
        historyFifo.prepareToWrite (1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            history[(size_t) start1] = point;
            historyFifo.finishedWrite (1);
        }
    }
}

BuiltInCompressor::Preview BuiltInCompressor::takePreview()
{
    Preview p;
    p.detectorDb = previewDetectorDb.exchange (-100.0f, std::memory_order_acquire);
    p.reductionDb = previewReductionDb.load (std::memory_order_relaxed);
    return p;
}

int BuiltInCompressor::readHistory (HistoryPoint* dest, int max)
{
    int ready = historyFifo.getNumReady();

    if (ready > max)
    {
        historyFifo.finishedRead (ready - max);
        ready = max;
    }

    int start1, size1, start2, size2;
    historyFifo.prepareToRead (ready, start1, size1, start2, size2);
    std::copy_n (history.begin() + start1, size1, dest);
    std::copy_n (history.begin() + start2, size2, dest + size1);
    historyFifo.finishedRead (size1 + size2);
    return size1 + size2;
}

juce::AudioProcessorEditor* BuiltInCompressor::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInCompressor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInCompressor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
