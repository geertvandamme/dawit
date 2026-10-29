#pragma once

/** Measures the audible differences between kinds and protects saved settings and isolated listening. */
static void runReverbNetworkTests()
{
    using namespace daw;
    constexpr double rate = 48000.0;
    const std::array<verb::Type, 6> types { verb::Type::digital, verb::Type::room, verb::Type::hall,
                                          verb::Type::chamber, verb::Type::plate, verb::Type::spring };
    auto settings = verb::Settings {};
    settings.dampingHz = verb::maxDamping;
    settings.lowCutHz = verb::minLowCut;
    settings.earlyPercent = 0.0f;
    settings.predelayMs = 0.0f;
    auto buildup = [] (const juce::AudioBuffer<float>& response)
    {
        const auto envelope = verb::getEnvelopeDb (response, rate, 2.0);
        const auto peak = *std::max_element (envelope.begin(), envelope.end());
        for (size_t i = 0; i < envelope.size(); ++i)
            if (envelope[i] >= peak - 3.0f) return i * 2.0;
        return 0.0;
    };
    for (auto type : types)
    {
        settings.type = type;
        for (float decay : { 1.0f, 2.0f, 4.0f })
        {
            settings.decaySeconds = decay;
            const auto response = verb::renderImpulse (settings, rate, decay * 1.6 + 0.3);
            const double measured = verb::measureDecay (response, rate);
            check (std::abs (measured - decay) < decay * (type == verb::Type::spring ? 0.20 : 0.15),
                   verb::kinds::getName (type) + " decay " + juce::String (decay, 1) + " s measures " + juce::String (measured, 3) + " s");
        }
        settings.predelayMs = 40.0f;
        auto delayed = verb::renderImpulse (settings, rate, 0.3);
        check (delayed.getMagnitude (0, 1920) <= 0.0f, verb::kinds::getName (type) + " is silent before its pre-delay");
        settings.predelayMs = 0.0f;
    }
    for (auto type : { verb::Type::plate, verb::Type::spring })
    {
        settings.type = type;
        settings.earlyPercent = 0.0f;
        const auto a = verb::renderImpulse (settings, rate, 0.5);
        settings.earlyPercent = 100.0f;
        const auto b = verb::renderImpulse (settings, rate, 0.5);
        bool identical = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                identical = identical && juce::exactlyEqual (a.getSample (ch, i), b.getSample (ch, i));
        check (identical, verb::kinds::getName (type) + " ignores the Early knob");
    }
    settings.earlyPercent = 0.0f;
    settings.decaySeconds = 2.0f;
    std::array<double, 6> buildTimes {};
    double digitalLevel = 0.0;
    for (auto type : types)
    {
        settings.type = type;
        buildTimes[(size_t) type] = buildup (verb::renderImpulse (settings, rate, 0.5));
        verb::Network network;
        network.setSettings (settings);
        network.prepare (rate);
        juce::Random random (785);
        std::array<float, 256> input {}, left {}, right {};
        double energy = 0.0;
        for (int block = 0; block < 1600; ++block)
        {
            for (auto& sample : input) sample = random.nextFloat() * 0.5f - 0.25f;
            network.process (input.data(), nullptr, left.data(), right.data(), 256);
            if (block > 800)
                for (int i = 0; i < 256; ++i) energy += (double) left[(size_t) i] * left[(size_t) i] + (double) right[(size_t) i] * right[(size_t) i];
        }
        if (type == verb::Type::digital) digitalLevel = energy;
        const double difference = 10.0 * std::log10 (energy / digitalLevel);
        check (std::abs (difference) < 2.0, verb::kinds::getName (type) + " steady noise is " + juce::String (difference, 2) + " dB from Digital");
    }
    check (buildTimes[(size_t) verb::Type::hall] > buildTimes[(size_t) verb::Type::room],
           "Hall builds up more slowly than Room at the same size (" + juce::String (buildTimes[2]) + " vs " + juce::String (buildTimes[1]) + " ms)");
    settings.type = verb::Type::plate;
    const double plateBuild = buildup (verb::renderImpulse (settings, rate, 0.5));
    settings.type = verb::Type::room;
    settings.sizePercent = 20.0f;
    const double roomBuild = buildup (verb::renderImpulse (settings, rate, 0.5));
    settings.type = verb::Type::hall;
    settings.sizePercent = 80.0f;
    const double hallBuild = buildup (verb::renderImpulse (settings, rate, 0.5));
    check (plateBuild <= 10.0 && roomBuild <= 25.0 && hallBuild >= 40.0,
           "Build-up Plate/Room/Hall: " + juce::String (plateBuild) + "/" + juce::String (roomBuild) + "/" + juce::String (hallBuild) + " ms");

    {
        // Typical damped settings should be as fair to compare as the undamped test above.
        double reference = 0.0;
        for (auto type : types)
        {
            verb::Network network;
            verb::Settings typical;
            typical.type = type;
            network.setSettings (typical);
            network.prepare (rate);
            juce::Random random (910);
            std::array<float, 256> input {}, left {}, right {};
            double energy = 0.0;
            for (int block = 0; block < 1000; ++block)
            {
                for (auto& sample : input) sample = random.nextFloat() * 0.5f - 0.25f;
                network.process (input.data(), nullptr, left.data(), right.data(), 256);
                if (block >= 500)
                    for (int i = 0; i < 256; ++i) energy += (double) left[(size_t) i] * left[(size_t) i] + (double) right[(size_t) i] * right[(size_t) i];
            }
            if (type == verb::Type::digital) reference = energy;
            const double difference = 10.0 * std::log10 (energy / reference);
            check (std::abs (difference) < 2.0, verb::kinds::getName (type) + " at default damping and Early is " + juce::String (difference, 2) + " dB from Digital");
        }
    }

    auto crest = [] (const juce::AudioBuffer<float>& response)
    {
        const int count = 1440;
        double energy = 0.0, peak = 0.0;
        for (int i = 0; i < count; ++i)
        {
            const double value = response.getSample (0, i);
            energy += value * value;
            peak = juce::jmax (peak, std::abs (value));
        }
        return 20.0 * std::log10 (peak / std::sqrt (energy / count));
    };
    settings = {};
    settings.predelayMs = 0.0f;
    settings.dampingHz = verb::maxDamping;
    settings.lowCutHz = verb::minLowCut;
    settings.earlyPercent = 100.0f;
    settings.type = verb::Type::plate;
    const double plateCrest = crest (verb::renderImpulse (settings, rate, 0.2));
    settings.type = verb::Type::room;
    const double roomCrest = crest (verb::renderImpulse (settings, rate, 0.2));
    settings.type = verb::Type::hall;
    const double hallCrest = crest (verb::renderImpulse (settings, rate, 0.2));
    check (plateCrest < roomCrest - 3.0 && plateCrest < hallCrest - 3.0,
           "First 30 ms crest Plate/Room/Hall: " + juce::String (plateCrest, 1) + "/" + juce::String (roomCrest, 1) + "/" + juce::String (hallCrest, 1) + " dB");
    settings.predelayMs = 40.0f;
    const auto hall = verb::renderImpulse (settings, rate, 0.3);
    int firstHall = 0;
    while (firstHall < hall.getNumSamples() && std::abs (hall.getSample (0, firstHall)) <= 0.0f) ++firstHall;
    check (std::abs (firstHall - 1920 - (int) std::round (verb::getFirstReflectionMs (settings.sizePercent) * rate / 1000.0)) <= 1,
           "Hall's first reflection follows pre-delay by the size's travel time");
    // Leaving reflections out of the picture must not change what drives the Hall's tail.
    const auto tailOnly = verb::renderImpulse (settings, rate, 0.3, false, true);
    const auto earlyOnly = verb::renderImpulse (settings, rate, 0.3, true, false);
    float decompositionError = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < hall.getNumSamples(); ++i)
            decompositionError = juce::jmax (decompositionError, std::abs (hall.getSample (ch, i) - tailOnly.getSample (ch, i) - earlyOnly.getSample (ch, i)));
    check (decompositionError < 1.0e-6f, "Hall's Early and Tail pictures sum to the full sound without changing its feeding");
    settings.predelayMs = 0.0f;
    settings.type = verb::Type::spring;
    settings.widthPercent = 100.0f;
    settings.decaySeconds = 2.5f;
    const auto spring = verb::renderImpulse (settings, rate, 1.0);
    // Envelope correlation finds the repeated spring pulse without depending on its chirp's phase.
    std::vector<double> envelope (1000, 0.0);
    for (int ms = 0; ms < 1000; ++ms)
        for (int sample = ms * 48; sample < (ms + 1) * 48; ++sample)
            envelope[(size_t) ms] += (double) spring.getSample (0, sample) * spring.getSample (0, sample);
    double best = 0.0;
    int repeat = 0;
    for (int lag = 20; lag <= 120; ++lag)
    {
        double correlation = 0.0;
        for (int ms = 0; ms < 400; ++ms) correlation += envelope[(size_t) ms] * envelope[(size_t) (ms + lag)];
        if (correlation > best) { best = correlation; repeat = lag; }
    }
    check (std::abs (repeat - 55) <= 5, "Spring's envelope repeats at " + juce::String (repeat) + " ms for Size 50%");
    auto spectralEnergy = [&spring] (double low, double high)
    {
        double energy = 0.0;
        for (double hz = low; hz <= high; hz += 100.0)
        {
            std::complex<double> sum {};
            const auto rotation = std::polar (1.0, -juce::MathConstants<double>::twoPi * hz / rate);
            std::complex<double> phase { 1.0, 0.0 };
            for (int sample = 0; sample < spring.getNumSamples(); ++sample)
            {
                sum += (double) spring.getSample (0, sample) * phase;
                phase *= rotation;
            }
            energy += std::norm (sum);
        }
        return energy / (1.0 + (high - low) / 100.0);
    };
    const double springBand = 10.0 * std::log10 (spectralEnergy (8000.0, 10000.0) / spectralEnergy (800.0, 1200.0));
    check (springBand < -20.0, "Spring above 8 kHz is " + juce::String (springBand, 1) + " dB below its 1 kHz band");

    {
        verb::Network network;
        settings.type = verb::Type::digital;
        network.setSettings (settings);
        network.prepare (rate);
        std::array<float, 256> input {}, left {}, right {};
        juce::Random random (55);
        float before = 0.0f, after = 0.0f;
        for (int block = 0; block < 400; ++block)
        {
            for (auto& sample : input) sample = 0.25f * (random.nextFloat() * 2.0f - 1.0f);
            if (block == 390) { settings.type = verb::Type::plate; network.setSettings (settings); }
            network.process (input.data(), nullptr, left.data(), right.data(), 256);
            for (float sample : left)
            {
                if (block >= 380 && block < 390) before = juce::jmax (before, std::abs (sample));
                if (block >= 390) after = juce::jmax (after, std::abs (sample));
            }
        }
        check (after < before * 2.0f, "Switching Type during noise keeps the next 50 ms below twice the steady peak");
    }

    BuiltInReverb plugin;
    check (plugin.getSettings().type == verb::Type::digital && verb::getReturnPreset().settings.type == verb::Type::plate,
           "New reverbs start Digital and return reverbs start Plate");
    auto old = plugin.getParameters().copyState();
    for (int i = old.getNumChildren(); --i >= 0;)
        if (old.getChild (i).getProperty ("id").toString() == "type") old.removeChild (i, nullptr);
    plugin.setType (verb::Type::spring);
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary (*old.createXml(), state);
    plugin.setStateInformation (state.getData(), (int) state.getSize());
    check (plugin.getSettings().type == verb::Type::digital, "Loading a state with no Type resets even a Spring instance to Digital");

    const auto beforeSwitch = plugin.getSettings();
    plugin.setType (verb::Type::plate);
    auto expectedSwitch = beforeSwitch;
    expectedSwitch.type = verb::Type::plate;
    check (plugin.getSettings() == expectedSwitch, "Switching Type keeps every knob");
    plugin.setType (verb::Type::digital);
    const auto unchanged = plugin.getSettings();
    plugin.requestAudition (verb::Type::plate);
    auto out = runEffect (plugin, (int) (rate * 4.0), [] (int) { return 0.0f; });
    std::vector<fx::LevelPoint> points (8192);
    const int count = plugin.history.read (points.data(), (int) points.size());
    bool mainSilent = true;
    for (int i = 0; i < count; ++i) mainSilent = mainSilent && points[(size_t) i].inputDb <= -100.0f && points[(size_t) i].effectDb <= -100.0f;
    check (out.getMagnitude (0, out.getNumSamples()) > 0.01f && mainSilent && unchanged == plugin.getSettings() && ! plugin.isAuditionActive(),
           "Showcase audition sounds, leaves the main network and settings untouched, and stops after its tail");
}
