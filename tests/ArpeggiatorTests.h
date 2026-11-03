// DawIt Arpeggiator: the engine tests (included by EngineTests.cpp, after the MIDI effect host tests: they use its test plugins and SlicerTests.h's play head).
#pragma once

namespace arptests
{
using namespace daw;
using slicertests::Head;

/** A note-on or note-off the arpeggiator sent, at a sample counted from where the drive started. */
struct Event
{
    long long sample = 0;
    int pitch = 0;
    bool on = false;
    int velocity = 0;
};

struct Input
{
    long long sample;
    juce::MidiMessage message;
};

inline Input noteOn (long long sample, int pitch, int velocity = 100) { return { sample, juce::MidiMessage::noteOn (1, pitch, (juce::uint8) velocity) }; }
inline Input noteOff (long long sample, int pitch) { return { sample, juce::MidiMessage::noteOff (1, pitch) }; }
inline Input controller (long long sample, int number, int value) { return { sample, juce::MidiMessage::controllerEvent (1, number, value) }; }
inline Input allNotesOff (long long sample) { return { sample, juce::MidiMessage::allNotesOff (1) }; }

/** Runs the arpeggiator for `numSamples` in blocks of 256 with the given input; the play head (may be null) is set by `clock` before each block. Other messages that come out are kept in `others`. */
inline std::vector<Event> drive (BuiltInArpeggiator& plugin, Head* head, long long numSamples, const std::vector<Input>& input, const slicertests::Clock& clock = {},
                                 bool prepare = true, std::vector<juce::MidiMessage>* others = nullptr, long long startSample = 0)
{
    if (prepare)
        plugin.prepareToPlay (testRate, testBlock);

    plugin.setPlayHead (head);
    std::vector<Event> events;
    juce::AudioBuffer<float> audio (2, testBlock);

    for (long long pos = 0; pos < numSamples; pos += testBlock)
    {
        const int n = (int) juce::jmin ((long long) testBlock, numSamples - pos);
        audio.setSize (2, n, false, false, true);

        if (head != nullptr && clock)
            clock (*head, (int) (startSample + pos));

        juce::MidiBuffer midi;

        for (auto& in : input)
            if (in.sample >= pos && in.sample < pos + n)
                midi.addEvent (in.message, (int) (in.sample - pos));

        plugin.processBlock (audio, midi);

        for (const auto metadata : midi)
        {
            const auto m = metadata.getMessage();

            if (m.isNoteOnOrOff())
                events.push_back ({ pos + metadata.samplePosition, m.getNoteNumber(), m.isNoteOn(), m.getVelocity() });
            else if (others != nullptr)
                others->push_back (m);
        }
    }

    return events;
}

inline std::vector<Event> onsOf (const std::vector<Event>& events)
{
    std::vector<Event> result;

    for (auto& e : events)
        if (e.on)
            result.push_back (e);

    return result;
}

inline const Event* findOff (const std::vector<Event>& events, int pitch, long long after = 0)
{
    for (auto& e : events)
        if (! e.on && e.pitch == pitch && e.sample >= after)
            return &e;

    return nullptr;
}

inline arp::Settings withKinds (arp::Settings s, const char* letters)
{
    s.steps = (int) std::strlen (letters);

    for (int i = 0; i < s.steps; ++i)
        s.kinds[(size_t) i] = (arp::StepKind) juce::String ("NCBRT").indexOfChar (letters[i]);

    return s;
}

inline std::vector<arp::HeldNote> held (std::initializer_list<int> pitches)
{
    std::vector<arp::HeldNote> notes;
    int order = 0;

    for (int p : pitches)
        notes.push_back ({ p, 100, order++, true });

    return notes;
}

inline std::vector<int> pitchesOf (const std::vector<arp::SeqNote>& sequence)
{
    std::vector<int> pitches;

    for (auto& n : sequence)
        pitches.push_back (n.pitch);

    return pitches;
}

inline std::vector<int> pitchesOf (const std::vector<Event>& events)
{
    std::vector<int> pitches;

    for (auto& e : events)
        pitches.push_back (e.pitch);

    return pitches;
}

inline bool closeTo (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; }

constexpr int chordCEG = (1 << 0) | (1 << 4) | (1 << 7);
}

//==============================================================================
static void runArpeggiatorTests()
{
    using namespace daw;
    using namespace arptests;
    using arp::Order;
    std::cout << "DawIt Arpeggiator" << std::endl;

    // The notes of the order.
    {
        const auto c = held ({ 64, 67, 60 });
        check (pitchesOf (arp::getSequence (c, Order::up, 2)) == std::vector<int> { 60, 64, 67, 72, 76, 79 }, "arpeggiator: Up over 2 octaves is 60 64 67 72 76 79");
        check (arp::getSequence (c, Order::up, 3).back().pitch == 91, "arpeggiator: Up over 3 octaves ends on G6 (91)");
        check (pitchesOf (arp::getSequence (c, Order::down, 1)) == std::vector<int> { 67, 64, 60 }, "arpeggiator: Down is 67 64 60");
        check (pitchesOf (arp::getSequence (c, Order::upDown, 1)) == std::vector<int> { 60, 64, 67, 64 }, "arpeggiator: Up and down is 60 64 67 64");
        check (pitchesOf (arp::getSequence (c, Order::upDown, 2)) == std::vector<int> { 60, 64, 67, 72, 76, 79, 76, 72, 67, 64 }, "arpeggiator: Up and down over 2 octaves has 10 notes");
        check (pitchesOf (arp::getSequence (c, Order::downUp, 1)) == std::vector<int> { 67, 64, 60, 64 }, "arpeggiator: Down and up is 67 64 60 64");
        check (arp::getCycleLength (3, Order::upDown, 1) == 4 && arp::getCycleLength (3, Order::upDown, 2) == 10 && arp::getCycleLength (6, Order::upDown, 1) == 10
                   && arp::getCycleLength (3, Order::up, 2) == 6 && arp::getCycleLength (3, Order::up, 3) == 9 && arp::getCycleLength (4, Order::up, 1) == 4,
               "arpeggiator: the Up and down cycle is 2m - 2 (3 notes: 4 and 10) and Up's is notes times octaves");
        check (pitchesOf (arp::getSequence (held ({ 60, 64, 67, 71 }), Order::outsideIn, 1)) == std::vector<int> { 60, 71, 64, 67 }
                   && pitchesOf (arp::getSequence (c, Order::outsideIn, 1)) == std::vector<int> { 60, 67, 64 },
               "arpeggiator: Outside in is 60 67 64, and 60 71 64 67 for four notes");
        check (pitchesOf (arp::getSequence (c, Order::alberti, 1)) == std::vector<int> { 60, 67, 64, 67 }, "arpeggiator: Alberti is low, high, middle, high: 60 67 64 67");
        check (pitchesOf (arp::getSequence (held ({ 64, 60, 67 }), Order::asPlayed, 1)) == std::vector<int> { 64, 60, 67 }, "arpeggiator: As played follows the order the keys were pressed: 64 60 67");

        auto together = held ({ 67, 60, 64 });

        for (auto& n : together)
            n.order = 1;

        check (pitchesOf (arp::getSequence (together, Order::asPlayed, 1)) == std::vector<int> { 60, 64, 67 }, "arpeggiator: notes pressed together count from low to high");
        check (arp::getBassPitch (60) == 48 && arp::getBassPitch (5) == 5, "arpeggiator: the Bass step is the lowest note an octave down (48 for C4)");

        bool high = true;

        for (auto order : { Order::up, Order::down, Order::upDown, Order::downUp, Order::outsideIn, Order::asPlayed, Order::alberti, Order::random })
            for (auto& n : arp::getSequence (held ({ 127, 120 }), order, 2))
                high = high && n.pitch <= 127;

        check (high && arp::getSequence (held ({ 127 }), Order::up, 2).size() == 1, "arpeggiator: notes above 127 are dropped");
        check (arp::getRandomIndex (12, 6) == arp::getRandomIndex (12, 6) && arp::getRandomIndex (5, 1) == 0, "arpeggiator: the random pick is the same for the same step of the song");

        bool spread = false;

        for (long long k = 0; k < 40; ++k)
            spread = spread || arp::getRandomIndex (k, 6) != arp::getRandomIndex (0, 6);

        check (spread, "arpeggiator: the random pick varies over the steps");
    }

    // Times, lengths and the numbers behind the advice.
    {
        constexpr double perBeat = testRate * 60.0 / 120.0;   // samples in a beat at 120 BPM
        check (closeTo (arp::getStepStartBeats (1, 2, 50.0f) * perBeat, 6000.0, 1.0) && closeTo (arp::getStepStartBeats (5, 2, 50.0f) * perBeat, 30000.0, 1.0),
               "arpeggiator: 1/16 steps start every 6000 samples at 120 BPM and 48 kHz");
        check (closeTo (arp::getStepStartBeats (1, 2, 66.67f) * perBeat, 8000.0, 1.0) && closeTo (arp::getStepStartBeats (1, 2, 75.0f) * perBeat, 9000.0, 1.0)
                   && closeTo (arp::getStepStartBeats (1, 2, 54.0f) * perBeat, 6480.0, 1.0) && closeTo (arp::getStepStartBeats (2, 2, 66.67f) * perBeat, 12000.0, 1.0),
               "arpeggiator: swing moves the second step of a pair to 8000 samples at 66.7 %, 9000 at 75 % and 6480 at 54 %, and the first stays");
        check (closeTo (arp::getNoteEndBeats (0, 1, 2, 50.0f, 50.0f) * perBeat, 3000.0, 1.0) && closeTo (arp::getNoteEndBeats (0, 1, 2, 50.0f, 100.0f) * perBeat, 6000.0, 1.0)
                   && closeTo (arp::getNoteEndBeats (0, 1, 2, 50.0f, 150.0f) * perBeat, 9000.0, 1.0),
               "arpeggiator: gate 50 % lasts 3000 samples, 100 % 6000 and 150 % 9000");
        check (closeTo (arp::getNoteEndBeats (0, 2, 2, 50.0f, 80.0f) * perBeat, 10800.0, 1.0), "arpeggiator: a note tied to the next step at gate 80 % lasts 1.8 steps (10 800 samples)");

        auto tied = withKinds ({}, "NTTN");
        check (arp::getTiedSteps (tied, 0) == 3 && arp::getTiedSteps (tied, 3) == 1 && arp::getTiedSteps (tied, 1) == 2, "arpeggiator: a note lasts through the Tie steps after it");
        check (arp::getAccentedVelocity (100, 20) == 120 && arp::getAccentedVelocity (120, 20) == 127, "arpeggiator: accent 20 makes velocity 100 into 120 and 120 into 127");
        check (stepgrid::getStepIndex (1.0, 2, 16) == 4, "arpeggiator: at quarter note 1.0, 1/16 steps and 16 steps, it is step 5 (index 4)");

        check (arp::getCrossingPatterns (3, 16) == 3 && arp::getCrossingPatterns (4, 16) == 1 && arp::getCrossingPatterns (10, 16) == 5,
               "arpeggiator: 3 notes over 16 steps line up after 3 patterns (3 bars of 1/16), 4 notes at once");
        check (arp::getBarRealignPatterns (16, 2) == 1 && arp::getBarRealignPatterns (6, 2) == 8 && arp::getBarRealignPatterns (12, 3) == 1,
               "arpeggiator: 6 steps of 1/16 line up with the bar after 8 patterns (12 beats), 16 and triplet 12 at once");

        arp::Settings fast;
        fast.rate = 0;
        arp::Settings sixteenth;
        check (closeTo (arp::getNotesPerSecond (fast, 140.0), 18.67, 0.01) && closeTo (arp::getNotesPerSecond (sixteenth, 140.0), 9.33, 0.01) && closeTo (arp::getNotesPerSecond (sixteenth, 120.0), 8.0, 1.0e-9),
               "arpeggiator: at 140 BPM, 1/32 is 18.67 notes a second and 1/16 is 9.33; at 120 BPM 1/16 is 8");
        check (closeTo (arp::getNotesPerSecond (withKinds (sixteenth, "RCRC"), 120.0), 4.0, 1.0e-9), "arpeggiator: rests don't count as notes");

        check (arp::nameChord ((1 << 9) | (1 << 0) | (1 << 4) | (1 << 7), 9) == "Am7" && arp::nameChord (chordCEG, 0) == "C" && arp::nameChord ((1 << 9) | (1 << 0) | (1 << 4), 9) == "Am",
               "arpeggiator: held pitch classes are named as chords: Am7, C, Am");
    }

    // Pattern text and starting points.
    {
        arp::Settings s;
        s.rate = 4;
        s.steps = 8;
        s = withKinds (s, "NNCRBTNC");
        s.accents[2] = s.accents[6] = true;
        arp::Settings copy;
        const auto text = arp::patternToText (s);
        check (text.startsWith ("DawIt Arpeggiator pattern: 1/8 8 ") && arp::patternFromText (text, copy) && copy.rate == s.rate && copy.steps == s.steps
                   && copy.kinds == s.kinds && copy.accents == s.accents,
               "arpeggiator: the pattern text round-trips, with the accents in lower case " + text);
        arp::Settings before = copy;
        check (! arp::patternFromText ("DawIt Slicer pattern: 1/16 16 TTTTTTTTTTTTTTTT", copy) && ! arp::patternFromText ("DawIt Arpeggiator pattern: 1/16 16 NNXN", copy)
                   && ! arp::patternFromText ("DawIt Arpeggiator pattern: 1/5 16 NNNN", copy) && copy.kinds == before.kinds && copy.rate == before.rate,
               "arpeggiator: text that isn't a pattern is refused and changes nothing");

        const auto& presets = arp::getPresets();
        bool complete = presets.size() == 51, distinct = true;
        juce::StringArray names;

        for (auto& p : presets)
        {
            complete = complete && p.name.isNotEmpty() && p.why.length() > 40;
            names.add (p.name);
            arp::Settings round;
            distinct = distinct && arp::patternFromText (arp::patternToText (p.settings), round) && round.kinds == p.settings.kinds;
        }

        names.removeDuplicates (false);
        check (complete && names.size() == 51 && distinct, "arpeggiator: fifty-one starting points, each with a name, a why and a pattern that survives copying");
        check (presets[0].name == "Up" && presets[0].settings.order == Order::up && presets[0].settings.steps == 16 && closeTo (presets[0].settings.gatePercent, 80.0, 0.001),
               "arpeggiator: the first starting point is the plain Up, which is also what a new one starts as");

        auto find = [&presets] (const char* name) -> const arp::Preset& { return *std::find_if (presets.begin(), presets.end(), [name] (auto& p) { return p.name == name; }); };
        const auto& threeThreeTwo = find ("3-3-2 stabs");
        check (arp::patternToText (threeThreeTwo.settings) == "DawIt Arpeggiator pattern: 1/8 8 CRRCRRCRNNNNNNNN" || arp::patternToText (threeThreeTwo.settings).contains ("1/8 8 cRRCRRCR"),
               "arpeggiator: 3-3-2 stabs hits steps 1, 4 and 7 of 8 eighths, the first accented " + arp::patternToText (threeThreeTwo.settings));
        check (find ("Trance sixteenths").settings.accents[0] && find ("Trance sixteenths").settings.accents[12] && ! find ("Trance sixteenths").settings.accents[1]
                   && closeTo (find ("Trance sixteenths").settings.gatePercent, 50.0, 0.001) && find ("Trance sixteenths").settings.octaves == 2,
               "arpeggiator: Trance sixteenths: 50 % gate, 2 octaves, accents on 1 5 9 13");
        check (find ("Ballad: bass and broken chord").settings.kinds[0] == arp::StepKind::bass && closeTo (find ("Ballad: bass and broken chord").settings.gatePercent, 150.0, 0.001)
                   && find ("Hit and ripple").settings.kinds[1] == arp::StepKind::tie && find ("Falling eighths").settings.order == Order::down
                   && closeTo (find ("Falling eighths").settings.swingPercent, 54.0, 0.001) && find ("Triplet waves").settings.rate == 3 && find ("Triplet waves").settings.steps == 12
                   && find ("Random sparkle").settings.order == Order::random && find ("Outside in").settings.order == Order::outsideIn,
               "arpeggiator: starting points have the plan's orders, gates, swing, rates and steps");
        bool allAccent20 = true;

        for (auto& p : presets)
            allAccent20 = allAccent20 && p.settings.accent == 20 && ! p.settings.latch;

        check (allAccent20, "arpeggiator: every starting point has Accent 20 and Latch off");

        // The 36 added starting points: groups, ranges, spot checks.
        bool inRange = true, groupsOk = presets[14].group == "Basics" && presets[15].group == "Single notes" && presets[50].group == "Chords and rhythms";
        bool fastOk = true;

        for (auto& p : presets)
        {
            const auto& st = p.settings;
            inRange = inRange && st.steps >= 1 && st.steps <= arp::maxSteps && st.octaves >= 1 && st.octaves <= 4 && st.rate >= 0 && st.rate < stepgrid::numStepLengths
                      && st.gatePercent >= arp::minGate && st.gatePercent <= arp::maxGate && st.swingPercent >= arp::minSwing && st.swingPercent <= arp::maxSwing && p.group.isNotEmpty();
            fastOk = fastOk && (p.name == "Rolled chord every beat" || arp::getNotesPerSecond (st, 120.0) <= 12.0 + 1.0e-9);
        }

        check (inRange && groupsOk, "arpeggiator: every starting point is inside the knobs' ranges and has its menu group");
        check (fastOk && closeTo (arp::getNotesPerSecond (find ("Rolled chord every beat").settings, 120.0), 12.0, 1.0e-9),
               "arpeggiator: no starting point is over 12 notes a second at 120 BPM, and Rolled chord every beat is exactly 12");
        const std::vector<arp::HeldNote> ceg = { { 60, 100, 0, true }, { 64, 100, 0, true }, { 67, 100, 0, true } };
        const auto down = arp::getSequence (ceg, find ("Down").settings.order, find ("Down").settings.octaves);
        check (down.size() == 3 && down[0].pitch == 67 && down[1].pitch == 64 && down[2].pitch == 60, "arpeggiator: Down on C E G plays 67 64 60");
        check (arp::getBarRealignPatterns (3, find ("Waltz: bass and two chords").settings.rate) == 4
                   && arp::getBarRealignPatterns (5, find ("Five against four").settings.rate) == 16,
               "arpeggiator: the waltz lines up with the bar after 4 patterns (3 bars), five against four after 16 (5 bars)");
        check (closeTo (arp::getStepStartBeats (1, find ("Shuffle").settings.rate, find ("Shuffle").settings.swingPercent), 2.0 / 3.0, 1.0e-3),
               "arpeggiator: the shuffle's second eighth starts at 2/3 of a beat");
        check (find ("Off-beat bass").settings.kinds[0] == arp::StepKind::rest && find ("Off-beat bass").settings.kinds[1] == arp::StepKind::bass
                   && find ("Charleston").settings.kinds[3] == arp::StepKind::chord && find ("Pad chord every bar").settings.kinds[1] == arp::StepKind::tie
                   && find ("Waltz: bass and two chords").settings.steps == 3 && find ("Waltz: bass and two chords").settings.accents[0] && find ("12/8 ballad").settings.steps == 12,
               "arpeggiator: the new starting points' step kinds");

        // Randomise.
        bool rulesOk = true, keepsOk = true, repeatable = true;
        std::set<juce::String> distinctPatterns;

        for (auto& p : presets)
            for (int seed = 1; seed <= 200; ++seed)
            {
                const auto r = arp::randomisePattern (p.settings, seed);
                const int n = r.steps;
                auto sounds = [] (arp::StepKind k) { return k == arp::StepKind::note || k == arp::StepKind::chord || k == arp::StepKind::bass; };
                int sounding = 0, notes = 0, accented = 0, restRun = 0;
                rulesOk = rulesOk && sounds (r.kinds[0]) && r.accents[0];

                for (int i = 0; i < n; ++i)
                {
                    const auto k = r.kinds[(size_t) i];
                    sounding += sounds (k) ? 1 : 0;
                    notes += k == arp::StepKind::note ? 1 : 0;
                    accented += r.accents[(size_t) i] ? 1 : 0;
                    restRun = k == arp::StepKind::rest ? restRun + 1 : 0;
                    rulesOk = rulesOk && restRun <= 3 && (k != arp::StepKind::tie || (i > 0 && r.kinds[(size_t) i - 1] != arp::StepKind::rest));
                }

                rulesOk = rulesOk && accented <= juce::jmax (1, sounding / 3) && (n < 4 || (sounding >= 3 && notes >= 2));
                keepsOk = keepsOk && r.order == p.settings.order && r.octaves == p.settings.octaves && r.rate == p.settings.rate && r.steps == p.settings.steps
                          && closeTo (r.gatePercent, p.settings.gatePercent, 1.0e-6) && closeTo (r.swingPercent, p.settings.swingPercent, 1.0e-6) && r.accent == p.settings.accent && r.latch == p.settings.latch
                          && arp::getNotesPerSecond (r, 120.0) <= 2.0 / stepgrid::getStepBeats (r.rate) + 1.0e-9;
                repeatable = repeatable && arp::patternToText (r) == arp::patternToText (arp::randomisePattern (p.settings, seed));

                if (p.name == "Up")
                    distinctPatterns.insert (arp::patternToText (r));
            }

        check (rulesOk, "arpeggiator: Randomise keeps its rules (step 1 sounds, no Tie after a Rest, at most 3 Rests in a row, 3 sounding steps, 2 Notes, accents at most a third) for 200 seeds on every starting point");
        check (keepsOk, "arpeggiator: Randomise keeps Order, Octaves, Rate, Steps, Gate, Swing, Accent and Latch");
        check (repeatable && distinctPatterns.size() >= 20, "arpeggiator: the same seed gives the same pattern, and 200 seeds give at least 20 different ones");
    }

    // The processor: timing, on its own clock (stopped).
    auto ownClock = [] (arp::Settings s, std::vector<Input> input, long long length, std::vector<juce::MidiMessage>* others = nullptr)
    {
        BuiltInArpeggiator plugin;
        plugin.setSettings (s);
        return drive (plugin, nullptr, length, input, {}, true, others);
    };
    const std::vector<Input> chord = { noteOn (0, 60), noteOn (0, 64), noteOn (0, 67) };

    {
        arp::Settings s;
        auto events = ownClock (s, chord, 48000);
        const auto ons = onsOf (events);
        check (ons.size() >= 8 && ons[0].sample == 0 && ons[0].pitch == 60 && closeTo ((double) ons[1].sample, 6000.0, 1.0) && ons[1].pitch == 64
                   && closeTo ((double) ons[2].sample, 12000.0, 1.0) && ons[2].pitch == 67 && closeTo ((double) ons[3].sample, 18000.0, 1.0) && ons[3].pitch == 60,
               "arpeggiator: Up on C E G steps 60 64 67 60 every 6000 samples");
        const auto* off = findOff (events, 60);
        check (off != nullptr && closeTo ((double) off->sample, 4800.0, 1.0), "arpeggiator: at gate 80 % a note ends 4800 samples after it starts");

        s.gatePercent = 50.0f;
        events = ownClock (s, chord, 12000);
        off = findOff (events, 60);
        check (off != nullptr && closeTo ((double) off->sample, 3000.0, 1.0), "arpeggiator: gate 50 % ends the note 3000 samples after it starts");

        // Overlap, and the same pitch ended first.
        s.gatePercent = 150.0f;
        events = ownClock (s, { noteOn (0, 60), noteOn (0, 64) }, 20000);
        const auto* secondOn = [&events]() -> const Event* { for (auto& e : events) if (e.on && e.pitch == 64) return &e; return nullptr; }();
        const auto* firstOff = findOff (events, 60);
        check (secondOn != nullptr && firstOff != nullptr && secondOn->sample < firstOff->sample && closeTo ((double) firstOff->sample, 9000.0, 1.0),
               "arpeggiator: at gate 150 % the next note starts before the one before ends");
        events = ownClock (s, { noteOn (0, 60) }, 20000);
        int lastEvent = -1, sixThousandOff = -1, sixThousandOn = -1;

        for (int i = 0; i < (int) events.size(); ++i)
        {
            lastEvent = i;

            if (events[(size_t) i].sample == 6000 && events[(size_t) i].pitch == 60)
                (events[(size_t) i].on ? sixThousandOn : sixThousandOff) = i;
        }

        check (lastEvent > 0 && sixThousandOff >= 0 && sixThousandOn > sixThousandOff, "arpeggiator: a pitch that still sounds is ended before it is started again");

        // Swing.
        s = {};
        s.swingPercent = 66.67f;
        auto ons2 = onsOf (ownClock (s, chord, 20000));
        check (ons2.size() >= 3 && closeTo ((double) ons2[1].sample, 8000.0, 1.0) && closeTo ((double) ons2[2].sample, 12000.0, 1.0), "arpeggiator: swing 66.7 % puts the second step at 8000 samples");
        s.swingPercent = 75.0f;
        ons2 = onsOf (ownClock (s, chord, 20000));
        check (ons2.size() >= 3 && closeTo ((double) ons2[1].sample, 9000.0, 1.0), "arpeggiator: swing 75 % puts the second step at 9000 samples");

        // Accents and velocity.
        s = withKinds ({}, "NN");
        s.accents[0] = true;
        auto accentEvents = ownClock (s, { noteOn (0, 60, 100), noteOn (0, 64, 90) }, 14000);
        const auto accentOns = onsOf (accentEvents);
        check (accentOns.size() >= 3 && accentOns[0].velocity == 120 && accentOns[1].velocity == 90 && accentOns[2].velocity == 120,
               "arpeggiator: an accented step plays 20 louder (100 becomes 120) and a step that isn't accented plays the key's own velocity");
        s = withKinds ({}, "NN");
        s.accents[0] = true;
        accentEvents = ownClock (s, { noteOn (0, 60, 120), noteOn (0, 64, 100) }, 14000);
        check (onsOf (accentEvents).size() >= 3 && onsOf (accentEvents)[0].velocity == 127 && onsOf (accentEvents)[1].velocity == 100,
               "arpeggiator: accent stops at 127 (120 becomes 127), and the step that isn't accented keeps the key's velocity");

        // Chord, bass and the order's counter.
        s = withKinds ({}, "C");
        auto chordOns = onsOf (ownClock (s, chord, 8000));
        check (chordOns.size() >= 3 && chordOns[0].sample == 0 && chordOns[1].sample == 0 && chordOns[2].sample == 0 && pitchesOf (chordOns).size() >= 3
                   && std::set<int> { chordOns[0].pitch, chordOns[1].pitch, chordOns[2].pitch } == std::set<int> { 60, 64, 67 },
               "arpeggiator: a Chord step starts all the held notes on one sample");
        s = withKinds ({}, "BN");
        auto bassOns = onsOf (ownClock (s, chord, 14000));
        check (bassOns.size() >= 3 && bassOns[0].pitch == 48 && bassOns[1].pitch == 60 && closeTo ((double) bassOns[1].sample, 6000.0, 1.0) && bassOns[2].pitch == 48,
               "arpeggiator: a Bass step plays the lowest note an octave down (48) and doesn't move the order on");
        s = withKinds ({}, "NCN");
        const auto indexOns = pitchesOf (onsOf (ownClock (s, chord, 17000)));
        check (indexOns.size() >= 5 && std::vector<int> (indexOns.begin(), indexOns.begin() + 5) == std::vector<int> { 60, 60, 64, 67, 64 },
               "arpeggiator: Note, Chord, Note plays the first note, the chord, then the second note");
        s = withKinds ({}, "NR");
        const auto restOns = onsOf (ownClock (s, chord, 25000));
        check (restOns.size() == 3 && restOns[1].sample >= 11999 && restOns[1].sample <= 12001, "arpeggiator: a Rest step plays nothing");

        // A tie.
        s = withKinds ({}, "NT");
        events = ownClock (s, { noteOn (0, 60) }, 20000);
        off = findOff (events, 60);
        check (off != nullptr && closeTo ((double) off->sample, 10800.0, 1.0) && onsOf (events).size() == 2 && closeTo ((double) onsOf (events)[1].sample, 12000.0, 1.0),
               "arpeggiator: a Tie after a Note at gate 80 % makes one note of 10 800 samples");
        s = withKinds ({}, "TN");
        check (onsOf (ownClock (s, { noteOn (0, 60) }, 8000)).size() == 1, "arpeggiator: a Tie with nothing before it stays silent");
    }

    // Starting from nothing, and the chord that comes a little late.
    {
        arp::Settings s;
        // Released, then a new chord: the order starts again at its first note, on its own clock at step 1.
        auto events = ownClock (s, { noteOn (0, 60), noteOn (0, 64), noteOn (0, 67), noteOff (7000, 60), noteOff (7000, 64), noteOff (7000, 67),
                                     noteOn (20000, 67), noteOn (20000, 62) }, 40000);
        const auto ons = onsOf (events);
        bool restarted = false;

        for (auto& e : ons)
            if (e.sample >= 20000 && e.sample < 20010)
                restarted = e.pitch == 62;

        check (ons.size() >= 4 && restarted, "arpeggiator: a new chord after letting go starts again from its first note, on step 1 of its own clock");
        // Letting go ends what it started.
        bool silent = true;
        long long lastOn = 0;

        for (auto& e : ons)
            if (e.sample < 20000)
                lastOn = e.sample;

        silent = lastOn < 7000;
        check (silent, "arpeggiator: no new note starts after every key is let go");

        // On the song's clock a chord up to a quarter of a step late plays that step at once; later, it waits.
        Head head;
        auto run = [&] (long long sample)
        {
            BuiltInArpeggiator plugin;
            plugin.setSettings (s);
            return onsOf (drive (plugin, &head, 20000, { noteOn (sample, 60), noteOn (sample, 64), noteOn (sample, 67) }, slicertests::steady (120.0)));
        };

        auto late = run (960);
        check (! late.empty() && closeTo ((double) late[0].sample, 960.0, 1.0) && late[0].pitch == 60 && late.size() >= 2 && closeTo ((double) late[1].sample, 6000.0, 1.0),
               "arpeggiator: a chord 20 ms into a 125 ms step plays that step as it arrives");
        late = run (3600);
        check (! late.empty() && closeTo ((double) late[0].sample, 6000.0, 1.0), "arpeggiator: a chord 60 % into a step waits for the next step");
        late = run (0);
        check (! late.empty() && late[0].sample == 0 && late[0].pitch == 60, "arpeggiator: a chord on a step plays at once");

        // The pattern is counted from the start of the song: at quarter note 1.0 it is step 5 (index 4).
        BuiltInArpeggiator plugin;
        plugin.setSettings (s);
        drive (plugin, &head, 2000, { noteOn (0, 60) }, slicertests::steady (120.0, 1.0));
        arp::StepRecord records[8];
        const int n = plugin.history.getRecent (records, 8);
        check (n >= 1 && records[0].step == 4 && closeTo (records[0].beat, 1.0, 1.0e-6) && records[0].kind == arp::StepKind::note && records[0].numPlayed == 1 && records[0].played[0] == 60
                   && records[0].isHeld (60) && records[0].getNumHeld() == 1,
               "arpeggiator: from quarter note 1.0 the first step is step 5 and the step records say what was held and played");
    }

    // Random is the same each time a bar plays, and always from the held notes.
    {
        arp::Settings s;
        s.order = Order::random;
        s.octaves = 1;
        auto play = [&]
        {
            BuiltInArpeggiator plugin;
            plugin.setSettings (s);
            Head head;
            return onsOf (drive (plugin, &head, 96000, chord, slicertests::steady (120.0)));
        };

        const auto first = play(), second = play();
        bool same = first.size() == second.size() && ! first.empty(), inSet = true;

        for (size_t i = 0; same && i < first.size(); ++i)
            same = first[i].pitch == second[i].pitch && first[i].sample == second[i].sample;

        for (auto& e : first)
            inSet = inSet && (e.pitch == 60 || e.pitch == 64 || e.pitch == 67);

        std::set<int> used;

        for (auto& e : first)
            used.insert (e.pitch);

        check (same && inSet && used.size() >= 2, "arpeggiator: Random plays the same notes each time a bar plays, all from the held set");
    }

    // Flush, seek, bypass, latch and the pedal.
    {
        arp::Settings s;
        s.gatePercent = 150.0f;
        BuiltInArpeggiator plugin;
        plugin.setSettings (s);
        auto events = drive (plugin, nullptr, 3000, { noteOn (0, 60), noteOn (0, 64) });
        check (plugin.soundingNotes[0].load() != 0, "arpeggiator: the notes it sounds are published for the window");
        auto more = drive (plugin, nullptr, 3000, { allNotesOff (0) }, {}, false);
        check (plugin.soundingNotes[0].load() == 0 && plugin.heldKeys[0].load() == 0 && findOff (more, 60) != nullptr && onsOf (more).empty(),
               "arpeggiator: all notes off (a flush) ends its notes and forgets the held keys, so nothing plays on");

        // A seek: it ends its notes at once.
        BuiltInArpeggiator seeker;
        seeker.setSettings (s);
        Head head;
        const slicertests::Clock jumping = [] (Head& h, int sample)
        {
            h.bpm = 120.0;
            h.ppq = sample < 4096 ? sample / 24000.0 : 8.0 + (sample - 4096) / 24000.0;
            h.seconds = h.ppq / 2.0;
        };
        events = drive (seeker, &head, 8192, { noteOn (0, 60), noteOn (0, 64) }, jumping);
        const auto* ended = findOff (events, 60, 4096);
        check (ended != nullptr && ended->sample == 4096, "arpeggiator: a seek ends the notes it started");

        // Bypassed: ends its notes and lets everything through.
        BuiltInArpeggiator bypassed;
        bypassed.setSettings (s);
        drive (bypassed, nullptr, 3000, { noteOn (0, 60), noteOn (0, 64) });
        juce::AudioBuffer<float> audio (2, 256);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 61, (juce::uint8) 90), 5);
        bypassed.processBlockBypassed (audio, midi);
        bool offs = false, through = false;

        for (const auto metadata : midi)
        {
            const auto m = metadata.getMessage();
            offs = offs || (m.isNoteOff() && m.getNoteNumber() == 60 && metadata.samplePosition == 0);
            through = through || (m.isNoteOn() && m.getNoteNumber() == 61 && metadata.samplePosition == 5);
        }

        check (offs && through && bypassed.soundingNotes[0].load() == 0 && bypassed.heldKeys[0].load() == 0, "arpeggiator: bypassed, it ends its notes and passes the MIDI through");

        // Latch: the chord goes on after letting go; the next chord replaces it.
        s = {};
        s.latch = true;
        BuiltInArpeggiator latched;
        latched.setSettings (s);
        events = drive (latched, nullptr, 48000, { noteOn (0, 60), noteOn (0, 64), noteOff (1000, 60), noteOff (1000, 64), noteOn (24000, 62), noteOn (24000, 65) });
        long long before = 0, after = 0;
        bool oldNotesAfter = false;

        for (auto& e : onsOf (events))
        {
            before += e.sample > 1000 && e.sample < 24000 ? 1 : 0;
            after += e.sample >= 24000 ? 1 : 0;
            oldNotesAfter = oldNotesAfter || (e.sample >= 24000 && (e.pitch == 60 || e.pitch == 64));
        }

        const auto firstNew = [&]() -> int { for (auto& e : onsOf (events)) if (e.sample >= 24000) return e.pitch; return -1; }();
        check (before >= 3 && after >= 3 && ! oldNotesAfter && firstNew == 62, "arpeggiator: Latch keeps the pattern going after letting go, until the next chord replaces it");

        // The pedal holds the keys and isn't passed on.
        s = {};
        std::vector<juce::MidiMessage> others;
        BuiltInArpeggiator pedal;
        pedal.setSettings (s);
        events = drive (pedal, nullptr, 40000, { noteOn (0, 60), noteOn (0, 64), controller (500, 64, 127), noteOff (1000, 60), noteOff (1000, 64), controller (20000, 64, 0) }, {}, true, &others);
        long long lastOn = 0, onsAfterRelease = 0;

        for (auto& e : onsOf (events))
        {
            lastOn = e.sample;
            onsAfterRelease += e.sample > 1000 && e.sample < 20000 ? 1 : 0;
        }

        bool pedalPassed = false;

        for (auto& m : others)
            pedalPassed = pedalPassed || m.isSustainPedalOn() || m.isSustainPedalOff();

        check (onsAfterRelease >= 2 && lastOn <= 20000 && ! pedalPassed && pedal.soundingNotes[0].load() == 0 && pedal.soundingNotes[1].load() == 0,
               "arpeggiator: with the pedal down the keys stay held; the pedal is not passed on, and lifting it ends the pattern");

        // Other messages pass.
        others.clear();
        BuiltInArpeggiator passing;
        drive (passing, nullptr, 512, { { 10, juce::MidiMessage::pitchWheel (1, 9000) } }, {}, true, &others);
        check (others.size() == 1 && others[0].isPitchWheel(), "arpeggiator: pitch bend and other messages pass through");
    }

    // The test chord and the examples.
    {
        const int aMinor = (1 << 9) | (1 << 0) | (1 << 4);
        arp::Settings s = withKinds ({}, "C");
        s.order = Order::down;
        BuiltInArpeggiator plugin;
        plugin.setSettings (s);
        plugin.setSongContext (0, aMinor);
        plugin.prepareToPlay (testRate, testBlock);
        plugin.playTestChord();
        auto events = drive (plugin, nullptr, 200000, {}, {}, false);
        const auto ons = onsOf (events);
        const std::set<int> first { ons.size() > 2 ? ons[0].pitch : -1, ons.size() > 2 ? ons[1].pitch : -1, ons.size() > 2 ? ons[2].pitch : -1 };
        check (ons.size() >= 3 && ons[0].sample == 0 && ons[2].sample == 0 && first == std::set<int> { 60, 64, 69 }, "arpeggiator: the test chord holds the chord's notes from C4 up: Am is 60 64 69");
        bool ended = ! plugin.isExamplePlaying() && plugin.soundingNotes[0].load() == 0 && plugin.heldKeys[0].load() == 0;
        long long lastOn = 0;

        for (auto& e : ons)
            lastOn = e.sample;

        check (ended && lastOn <= 192000 && lastOn >= 180000 && plugin.getSettings().kinds[0] == arp::StepKind::chord && plugin.getSettings().order == Order::down,
               "arpeggiator: the test chord plays for 2 bars and leaves the settings alone");

        BuiltInArpeggiator none;
        none.setSettings (s);
        none.prepareToPlay (testRate, testBlock);
        none.playTestChord();
        const auto cMajor = onsOf (drive (none, nullptr, 2000, {}, {}, false));
        check (cMajor.size() == 3 && std::set<int> { cMajor[0].pitch, cMajor[1].pitch, cMajor[2].pitch } == std::set<int> { 60, 64, 67 }, "arpeggiator: the test chord is C4 E4 G4 where there is no chord");

        // An example uses its own settings and notes.
        BuiltInArpeggiator example;
        example.setSettings (s);   // Down, Chord
        arp::Example e;
        e.settings = withKinds ({}, "N");
        e.pitches = { 60, 64, 67 };
        e.numPitches = 3;
        e.beats = 2.0;
        example.prepareToPlay (testRate, testBlock);
        example.playExample (e);
        const auto exampleEvents = drive (example, nullptr, 60000, {}, {}, false);
        const auto exampleOns = onsOf (exampleEvents);
        long long lastExampleOn = 0;

        for (auto& on : exampleOns)
            lastExampleOn = on.sample;

        check (exampleOns.size() >= 2 && exampleOns[0].pitch == 60 && exampleOns[1].pitch == 64 && example.getSettings().order == Order::down && ! example.isExamplePlaying()
                   && example.soundingNotes[0].load() == 0 && example.soundingNotes[1].load() == 0 && lastExampleOn < 48000,
               "arpeggiator: an example plays its own pattern and notes, ends after its length, and the settings are unchanged");
    }

    // History: the newest steps, oldest first.
    {
        arp::StepHistory history;

        for (int i = 0; i < 300; ++i)
        {
            arp::StepRecord r;
            r.step = i % 16;
            r.kind = arp::StepKind::chord;
            r.numPlayed = 2;
            r.played[0] = 60;
            r.played[1] = 99;
            r.chordPitchClasses = chordCEG;
            r.heldMask = { 1ULL << 60, 1ULL << 3 };
            r.beat = i * 0.25;
            history.push (r);
        }

        arp::StepRecord out[300];
        const int n = history.getRecent (out, 300);
        check (n > 200 && out[n - 1].step == 299 % 16 && closeTo (out[n - 1].beat, 299 * 0.25, 1.0e-9) && out[n - 1].kind == arp::StepKind::chord && out[n - 1].played[1] == 99
                   && out[n - 1].chordPitchClasses == chordCEG && out[n - 1].isHeld (60) && out[n - 1].isHeld (67) && ! out[n - 1].isHeld (61) && out[0].beat < out[n - 1].beat,
               "arpeggiator: the step history keeps the newest steps, oldest first, with what was held and played");
        history.clear();
        check (history.getRecent (out, 300) == 0, "arpeggiator: clearing the history empties it");
    }

    // On a MIDI track, before an instrument: the clip's chord becomes the pattern, bypassed it comes through whole.
    {
        using Kind = TestMidiPlugin::Kind;
        auto engine = std::make_unique<Engine>();
        FakeDevice device;
        engine->deviceManager.removeAudioCallback (engine.get());
        engine->pluginFormats.addFormat (new TestMidiFormat());
        engine->audioDeviceAboutToStart (&device);
        auto* track = engine->addTrack (Track::Type::midi);
        auto* clip = engine->addMidiClip (*track, 0.0, 4.0);
        {
            const juce::ScopedLock sl (engine->getLock());
            clip->notes.push_back ({ 60, 0.0, 4.0, 0.8f });
            clip->notes.push_back ({ 64, 0.0, 4.0, 0.8f });
            clip->notes.push_back ({ 67, 0.0, 4.0, 0.8f });
        }
        engine->notesChanged();
        juce::String error;
        auto* instrument = engine->addPlugin (*track, TestMidiPlugin::getDescription (Kind::recorder), error, 0);
        auto* slot = engine->addPlugin (*track, BuiltInArpeggiator::getDescription(), error, 5);
        check (slot != nullptr && instrument != nullptr && slot->isMidiEffect && track->plugins.size() == 2 && track->plugins[0].get() == slot,
               "arpeggiator: DawIt Arpeggiator goes on a MIDI track before the instrument " + error);

        if (slot == nullptr || instrument == nullptr)
            return;

        TestMidiPlugin::heard.clear();
        engine->setPositionBeats (0.0);
        engine->play();
        render (*engine, 0.6);
        check (TestMidiPlugin::heard.size() >= 4 && TestMidiPlugin::heard[0] == 60 && TestMidiPlugin::heard[1] == 64 && TestMidiPlugin::heard[2] == 67 && TestMidiPlugin::heard[3] == 60,
               "arpeggiator: a clip's chord reaches the instrument as 60 64 67 60 ... in time with the song");

        engine->setPluginBypassed (*slot, true);
        engine->setPositionBeats (0.0);
        TestMidiPlugin::heard.clear();
        render (*engine, 0.6);
        check (TestMidiPlugin::heard == std::vector<int> { 60, 64, 67 }, "arpeggiator: bypassed, the clip's chord reaches the instrument as it is");
        engine->setPluginBypassed (*slot, false);
        engine->stop();
    }

    // Saved and loaded: its settings, and the chain order (arpeggiator before the audio effect).
    {
        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::midi);
        juce::String error, text;
        auto* tune = engine.addPlugin (*track, BuiltInTune::getDescription(), error);
        auto* slot = engine.addPlugin (*track, BuiltInArpeggiator::getDescription(), error);
        auto* arpeggiator = slot != nullptr ? dynamic_cast<BuiltInArpeggiator*> (slot->instance.get()) : nullptr;
        const auto& presets = arp::getPresets();

        if (arpeggiator == nullptr || tune == nullptr)
        {
            check (false, "arpeggiator: the engine adds DawIt Arpeggiator and DawIt Tune to a MIDI track " + error);
            return;
        }

        arpeggiator->setSettings (presets[8].settings);
        const auto folder = juce::File::createTempFile ("arpeggiator");
        folder.createDirectory();
        const juce::ScopeGuard guard { [folder] { folder.deleteRecursively(); } };
        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        juce::StringArray warnings;
        const bool ok = project::save (engine, folder.getChildFile ("Song.dawit"), text, error) && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
        BuiltInArpeggiator* reloaded = nullptr;
        bool order = ok && ! loaded.getTracks().empty() && loaded.getTracks()[0]->plugins.size() == 2;

        if (order)
        {
            auto& plugins = loaded.getTracks()[0]->plugins;
            reloaded = dynamic_cast<BuiltInArpeggiator*> (plugins[0]->instance.get());
            order = reloaded != nullptr && plugins[0]->isMidiEffect && plugins[1]->isAudioEffect();
        }

        check (order && warnings.isEmpty() && arp::patternToText (reloaded->getSettings()) == arp::patternToText (presets[8].settings)
                   && reloaded->getSettings().order == presets[8].settings.order && closeTo (reloaded->getSettings().gatePercent, presets[8].settings.gatePercent, 0.01)
                   && reloaded->getSettings().rate == presets[8].settings.rate,
               "arpeggiator: a project opens with the arpeggiator's settings and the chain order arpeggiator, effect " + error + warnings.joinIntoString (" "));
    }

    // The registry and the add list.
    {
        const auto effects = BuiltInFormat::getMidiEffects();
        bool listed = effects.size() == 1 && effects[0].fileOrIdentifier == BuiltInArpeggiator::identifier && ! effects[0].isInstrument;
        bool notInEffects = true;

        for (auto& d : BuiltInFormat::getEffects())
            notInEffects = notInEffects && d.fileOrIdentifier != BuiltInArpeggiator::identifier;

        check (listed && notInEffects, "arpeggiator: the format lists it as a MIDI effect, apart from the audio effects");
        BuiltInArpeggiator plugin;
        check (plugin.isMidiEffect() && plugin.acceptsMidi() && plugin.producesMidi() && plugin.getTotalNumInputChannels() == 0 && plugin.getTotalNumOutputChannels() == 0
                   && plugin.getLatencySamples() == 0,
               "arpeggiator: it is a MIDI effect with no audio and no latency");
    }
}

//==============================================================================
static void runArpeggiatorAnalysisTests()
{
    using namespace daw;
    using namespace arptests;
    std::cout << "DawIt Arpeggiator advice" << std::endl;

    auto record = [] (std::initializer_list<int> played, std::initializer_list<int> heldNotes, int chord, double beat, int step = 0)
    {
        arp::StepRecord r;
        r.step = step;
        r.beat = beat;
        r.chordPitchClasses = chord;

        for (int p : played)
            r.played[(size_t) r.numPlayed++] = (std::uint8_t) p;

        for (int p : heldNotes)
            r.heldMask[(size_t) (p / 64)] |= 1ULL << (p % 64);

        return r;
    };

    auto lines = [] (const arp::AnalysisInput& in) { return arp::analyse (in); };
    auto has = [] (const std::vector<juce::String>& l, const juce::String& text) { return std::any_of (l.begin(), l.end(), [&text] (auto& s) { return s.contains (text); }); };

    const std::initializer_list<int> four { 60, 64, 67, 71 };   // 4 held notes: a cycle of 4 divides 16
    arp::AnalysisInput base;
    base.bpm = 120.0;

    // 1: nothing held.
    {
        auto in = base;
        auto l = lines (in);
        check (l.size() == 1 && l[0].startsWith ("Nothing reached the arpeggiator in the last 2 bars. Hold a chord on your keyboard, or put a MIDI clip of chords on this track, and play the song."),
               "arpeggiator advice 1: with no records it says nothing reached it, and stops");
        in.records.push_back (record ({}, four, 0, 0.0));
        in.settings = withKinds ({}, "R");
        check (lines (in).size() == 1 && has (lines (in), "Nothing reached"), "arpeggiator advice 1: steps that played no note count as nothing");
    }

    // 2: the notes against the chord track.
    {
        auto in = base;
        in.records = { record ({ 60 }, four, chordCEG, 0.0), record ({ 64 }, four, chordCEG, 0.25), record ({ 67 }, four, chordCEG, 0.5), record ({ 60 }, four, chordCEG, 0.75) };
        check (has (lines (in), "Every note it played is in the chord track's chord of its bar, so the arpeggio follows the harmony."), "arpeggiator advice 2: every note in the chord is said so");

        in.records = { record ({ 60 }, { 57, 60, 64, 67 }, chordCEG, 8.0), record ({ 61 }, { 57, 60, 64, 67 }, chordCEG, 8.25), record ({ 66 }, { 57, 60, 64, 67 }, chordCEG, 8.5), record ({ 60 }, { 57, 60, 64, 67 }, chordCEG, 8.75) };
        auto l = lines (in);
        check (has (l, "2 of the 4 notes it played aren't in the chord track's chord (bar 3: you hold Am7, the chord track has C). Notes outside the chord can add colour or clash. If it sounds muddy, hold the chord track's chord, or change the chord track."),
               "arpeggiator advice 2: more than a quarter outside names the bar, what you hold and the chord track's chord");

        in.records.clear();
        for (int i = 0; i < 8; ++i)
            in.records.push_back (record ({ i == 3 ? 61 : 60 }, four, chordCEG, i * 0.25));

        check (has (lines (in), "Most notes it played are in the chord track's chords; the 1 that aren't are passing colour."), "arpeggiator advice 2: a few outside are passing colour (1 of 8)");

        in.records.clear();
        for (int i = 0; i < 8; ++i)
            in.records.push_back (record ({ i < 2 ? 61 : 60 }, four, chordCEG, i * 0.25));

        check (has (lines (in), "Most notes it played are in the chord track's chords; the 2 that aren't are passing colour."),
               "arpeggiator advice 2: exactly a quarter outside is still passing colour");

        in.records = { record ({ 60 }, four, chordCEG, 0.0), record ({ 61 }, four, chordCEG, 0.25), record ({ 62 }, four, chordCEG, 0.5) };
        check (! has (lines (in), "chord track"), "arpeggiator advice 2: quiet with fewer than 4 notes in bars that have a chord");
        in.records = { record ({ 60 }, four, 0, 0.0), record ({ 61 }, four, 0, 0.25), record ({ 62 }, four, 0, 0.5), record ({ 63 }, four, 0, 0.75), record ({ 64 }, four, 0, 1.0) };
        check (! has (lines (in), "chord track"), "arpeggiator advice 2: quiet without a chord track");
    }

    // 3: the cycle against the pattern.
    {
        auto in = base;
        in.records = { record ({ 60 }, { 60, 64, 67 }, 0, 0.0) };
        auto l = lines (in);
        check (has (l, "Your 3-note cycle on a pattern with 16 Note steps comes round at a different step each time; it lines up again every 3 patterns (3 bars). Hold 2 or 4 notes or try Up and down to line it up, or keep it for the restless feel."),
               "arpeggiator advice 3: 3 notes over 16 Note steps line up every 3 patterns (3 bars)");

        in.records = { record ({ 60 }, four, 0, 0.0) };
        check (! has (lines (in), "-note cycle"), "arpeggiator advice 3: quiet when the cycle divides the Note steps (4 notes)");
        in.records = { record ({ 60 }, { 60, 64, 67 }, 0, 0.0) };
        in.settings.order = arp::Order::upDown;
        check (! has (lines (in), "-note cycle"), "arpeggiator advice 3: quiet for Up and down on 3 notes (cycle 4)");
        in.settings.order = arp::Order::random;
        check (! has (lines (in), "-note cycle"), "arpeggiator advice 3: quiet for Random");
        in.settings = withKinds ({}, "CCCC");
        check (! has (lines (in), "-note cycle"), "arpeggiator advice 3: quiet with no Note steps");
        in.settings = withKinds ({}, "NNNNNN");
        in.settings.rate = 2;
        in.records = { record ({ 60 }, { 60, 64, 67, 71 }, 0, 0.0) };
        l = lines (in);
        check (has (l, "Your 4-note cycle on a pattern with 6 Note steps comes round at a different step each time; it lines up again every 2 patterns (0.75 bars). Hold 3 notes or try Up and down to line it up, or keep it for the restless feel."),
               "arpeggiator advice 3: 4 notes over 6 Note steps cross too, every 2 patterns (0.75 bars)");
    }

    // 3, only suggestions that line up.
    {
        auto in = base;
        in.records = { record ({ 60 }, { 60, 64, 67, 71, 74 }, 0, 0.0) };
        auto l = lines (in);
        check (has (l, "Your 5-note cycle on a pattern with 16 Note steps") && has (l, "Hold 4 notes or try Up and down") && ! has (l, "4 or 6"),
               "arpeggiator advice 3: 5 notes on 16 Note steps suggest 4 notes, not 6 (a 6-note cycle doesn't divide 16); Up and down (cycle 8) fits");
        in.settings = withKinds ({}, "NNNNNN");
        in.records = { record ({ 60 }, four, 0, 0.0) };
        l = lines (in);
        check (has (l, "Hold 3 notes or try Up and down") && ! has (l, "3 or 5"), "arpeggiator advice 3: 4 notes on 6 Note steps suggest 3 (not 5), and Up and down (a cycle of 6)");
        in.settings = withKinds ({}, "NNNNNNNNNNNNNNNN");
        in.settings.order = arp::Order::upDown;
        in.records = { record ({ 60 }, four, 0, 0.0) };
        l = lines (in);
        check (has (l, "Your 6-note cycle on a pattern with 16 Note steps") && ! has (l, "try Up and down") && ! has (l, "Try Up and down") && has (l, "Hold 3 or 5 notes"),
               "arpeggiator advice 3: Up and down with 4 notes (a cycle of 6) isn't offered Up and down again; 3 and 5 notes (cycles 4 and 8) fit 16");
        in.settings = {};
        in.records = { record ({ 60 }, { 60, 64, 67 }, 0, 0.0) };
        check (has (lines (in), "Hold 2 or 4 notes or try Up and down to line it up"), "arpeggiator advice 3: 3 notes on 16 Note steps still suggest 2 or 4 notes and Up and down");
    }

    // 4: too fast.
    {
        auto in = base;
        in.records = { record ({ 60 }, four, 0, 0.0) };
        in.bpm = 140.0;
        in.settings.rate = 0;
        check (has (lines (in), "At 140 BPM, 1/32 steps are 18.7 notes a second, too fast to follow one by one; they blur into a shimmer. If you want notes you can follow, try 1/16 (9.3 a second)."),
               "arpeggiator advice 4: 1/32 at 140 BPM is 18.7 notes a second, and 1/16 is suggested");
        in.settings.rate = 2;
        check (! has (lines (in), "too fast"), "arpeggiator advice 4: quiet at 9.3 notes a second");
        in.bpm = 180.0;
        check (! has (lines (in), "too fast"), "arpeggiator advice 4: quiet at exactly 12 notes a second");
        in.bpm = 181.0;
        check (has (lines (in), "too fast"), "arpeggiator advice 4: speaks above 12 notes a second");
        in.settings = withKinds (in.settings, "NRNRNRNR");
        check (! has (lines (in), "too fast"), "arpeggiator advice 4: rests lower the notes a second");
    }

    // 5: low and fast.
    {
        auto in = base;
        in.records = { record ({ 47 }, four, 0, 0.0) };
        check (has (lines (in), "The arpeggio goes down to B2. Fast low notes tend to sound muddy and get in the way of the bass; try holding the chord an octave higher."),
               "arpeggiator advice 5: a note below C3 at 1/16 is called low and fast (B2)");
        in.records = { record ({ 48 }, four, 0, 0.0) };
        check (! has (lines (in), "goes down to"), "arpeggiator advice 5: quiet at C3");
        in.records = { record ({ 40 }, four, 0, 0.0) };
        in.settings.rate = 4;
        check (! has (lines (in), "goes down to"), "arpeggiator advice 5: quiet at 1/8");
        in.settings.rate = 1;
        check (has (lines (in), "goes down to E2"), "arpeggiator advice 5: speaks at 1/16 triplets");
    }
}

//==============================================================================
// Create MIDI clip from arpeggiator: the notes-only capture, the pairing and the write-out.
namespace arptests
{
/** A song with a MIDI track "Keys": the chords clip (C4 E4 G4 for 2 beats, A3 C4 E4 for 2) at `start`, the arpeggiator,
    and the test recorder as its instrument. */
struct WriteOutSong
{
    std::unique_ptr<Engine> engine;
    FakeDevice device;
    Track* track = nullptr;
    MidiClip* clip = nullptr;
    BuiltInArpeggiator* arpeggiator = nullptr;
    PluginSlot* arpSlot = nullptr;

    explicit WriteOutSong (double start = 0.0, bool withRecorder = true)
    {
        engine = std::make_unique<Engine>();
        engine->deviceManager.removeAudioCallback (engine.get());
        engine->pluginFormats.addFormat (new TestMidiFormat());
        engine->audioDeviceAboutToStart (&device);
        track = engine->addTrack (Track::Type::midi);
        track->name = "Keys";
        clip = addChords (start);
        juce::String error;

        if (withRecorder)
            engine->addPlugin (*track, TestMidiPlugin::getDescription (TestMidiPlugin::Kind::recorder), error);

        arpSlot = engine->addPlugin (*track, BuiltInArpeggiator::getDescription(), error);
        arpeggiator = arpSlot != nullptr ? dynamic_cast<BuiltInArpeggiator*> (arpSlot->instance.get()) : nullptr;
    }

    MidiClip* addChords (double start)
    {
        auto* c = engine->addMidiClip (*track, start, 4.0);
        {
            const juce::ScopedLock sl (engine->getLock());
            for (int pitch : { 60, 64, 67 })
                c->notes.push_back ({ pitch, 0.0, 2.0, 0.8f });
            for (int pitch : { 57, 60, 64 })
                c->notes.push_back ({ pitch, 2.0, 2.0, 0.8f });
        }
        engine->notesChanged();
        return c;
    }

    arp::Settings getSettings() const { return arpeggiator->getSettings(); }
};

inline std::vector<int> pitchesOf (const std::vector<MidiNote>& notes)
{
    std::vector<int> pitches;

    for (auto& n : notes)
        pitches.push_back (n.pitch);

    return pitches;
}
}

static void runArpeggioWriteOutTests()
{
    using namespace daw;
    using namespace arptests;
    std::cout << "Arpeggio written out as MIDI notes" << std::endl;

    constexpr double oneSample = 1.0 / 48000.0 * 2.0;   // beats at 120 BPM: one sample is 0.0000417 beat; 2 samples of slack for the rounding of a start and an end
    const std::vector<int> expectedPitches { 60, 64, 67, 60, 64, 67, 60, 64, 57, 60, 64, 57, 60, 64, 57, 60 };

    // The pairing.
    {
        auto on = [] (double beat, int pitch, int velocity) { return MidiCapture::Event { beat, juce::MidiMessage::noteOn (1, pitch, (juce::uint8) velocity) }; };
        auto off = [] (double beat, int pitch) { return MidiCapture::Event { beat, juce::MidiMessage::noteOff (1, pitch) }; };
        auto zero = [] (double beat, int pitch) { return MidiCapture::Event { beat, juce::MidiMessage::noteOn (1, pitch, (juce::uint8) 0) }; };

        const auto basic = bounce::pairNotes ({ on (4.0, 60, 127), off (5.0, 60), on (4.5, 64, 64), off (6.0, 64) }, 4.0, 4.0);
        check (basic.size() == 2 && basic[0].pitch == 60 && closeTo (basic[0].start, 0.0, 1e-9) && closeTo (basic[0].length, 1.0, 1e-9) && closeTo (basic[0].velocity, 1.0, 1e-6)
                   && basic[1].pitch == 64 && closeTo (basic[1].start, 0.5, 1e-9) && closeTo (basic[1].length, 1.5, 1e-9) && closeTo (basic[1].velocity, 64.0 / 127.0, 1e-6),
               "write-out: notes are relative to the clip's start, with velocity as the 7-bit value over 127");

        const auto overlap = bounce::pairNotes ({ on (0.0, 60, 100), on (1.0, 60, 90), off (2.0, 60), off (3.0, 60) }, 0.0, 8.0);
        check (overlap.size() == 2 && closeTo (overlap[0].length, 2.0, 1e-9) && closeTo (overlap[1].start, 1.0, 1e-9) && closeTo (overlap[1].length, 2.0, 1e-9),
               "write-out: two note-ons of one pitch pair with the note-offs in order");

        const auto velocityZero = bounce::pairNotes ({ on (0.0, 60, 100), zero (1.0, 60) }, 0.0, 4.0);
        check (velocityZero.size() == 1 && closeTo (velocityZero[0].length, 1.0, 1e-9), "write-out: a note-on with velocity 0 ends the note");

        const auto cut = bounce::pairNotes ({ on (3.0, 60, 100), off (6.0, 60), on (4.0, 62, 100), off (5.0, 62), on (3.5, 64, 100) }, 0.0, 4.0);
        check (cut.size() == 2 && closeTo (cut[0].length, 1.0, 1e-9) && cut[1].pitch == 64 && closeTo (cut[1].length, 0.5, 1e-9),
               "write-out: notes still sounding are cut at the clip's end, notes starting at the end are dropped");

        const auto empty = bounce::pairNotes ({ on (1.0, 60, 100), off (1.0, 60), off (2.0, 61), on (3.0, 62, 100), { 3.5, juce::MidiMessage::allNotesOff (1) } }, 0.0, 4.0);
        check (empty.size() == 1 && empty[0].pitch == 62 && closeTo (empty[0].length, 0.5, 1e-9),
               "write-out: zero-length notes and stray note-offs are dropped, and all-notes-off ends the notes");
    }

    // The example: 16 notes, Up, 1/16, 80 %.
    {
        WriteOutSong song;
        check (song.arpeggiator != nullptr, "write-out: the example song has the arpeggiator");

        if (song.arpeggiator == nullptr)
            return;

        const auto plan = bounce::planWriteOut (*song.engine, *song.track, song.clip);
        check (plan.isOk() && closeTo (plan.startBeat, 0.0, 1e-9) && closeTo (plan.endBeat, 4.0, 1e-9), "write-out: the plan covers the clip");

        MidiCapture capture;
        {
            bounce::Renderer renderer (*song.engine, bounce::getWriteOutSettings (plan, capture));
            check (renderer.renderDiscarding().result.wasOk(), "write-out: the notes-only render runs");
        }
        const auto notes = bounce::pairNotes (capture.events, plan.startBeat, 4.0);
        bool timing = notes.size() == 16;

        for (size_t k = 0; timing && k < notes.size(); ++k)
            timing = closeTo (notes[k].start, 0.25 * (double) k, oneSample) && closeTo (notes[k].length, 0.2, oneSample);

        check (notes.size() == 16 && timing, "write-out: the example clip gives 16 notes at 0.25 k beats, each 0.2 beat long");
        check (pitchesOf (notes) == expectedPitches, "write-out: the pitches are C E G C E G C E, then A C E A C E A C");
        check (! notes.empty() && closeTo (notes[0].velocity, 102.0 / 127.0, 1e-4), "write-out: the velocity is 102/127");

        const auto before = song.clip->notes;
        const auto result = bounce::addWrittenOutTrack (*song.engine, plan, capture);
        check (result.result.wasOk() && result.track != nullptr && result.clip != nullptr && result.numNotes == 16, "write-out: the notes go into a new clip " + result.result.getErrorMessage());

        if (result.track != nullptr && result.clip != nullptr)
        {
            check (song.engine->indexOf (result.track) == song.engine->indexOf (song.track) + 1 && result.track->name == "Keys (arpeggio)" && result.track->isMidi(),
                   "write-out: a MIDI track \"Keys (arpeggio)\" right below the original");
            check (closeTo (result.clip->startBeat, 0.0, 1e-9) && closeTo (result.clip->lengthBeats, 4.0, 1e-9) && result.clip->notes.size() == 16,
                   "write-out: the new clip has the source's start and length");
            check (result.track->getNumMidiEffects() == 0 && result.track->plugins.size() == 1 && result.track->plugins[0]->isInstrument,
                   "write-out: the new track has the instrument and no MIDI effect");
            check (song.clip->muted && ! song.track->muted.load(), "write-out: the source clip is muted, not its track");
            check (miditransform::sameNotes (song.clip->notes, before) && song.track->plugins.size() == 2 && song.track->plugins[0].get() == song.arpSlot
                       && song.track->getNumMidiEffects() == 1 && ! song.arpSlot->bypassed.load(),
                   "write-out: the source clip's notes and the source track's plugins are unchanged");
            check (song.engine->canUndoArpeggioWriteOut (*song.clip) && song.engine->canUndoArpeggioWriteOut (*result.clip) && ! song.engine->canUndoArpeggioWriteOut (*song.track),
                   "write-out: undo is offered for the source and the result clip");

            // The same notes as the playback, through the fake device into the recording instrument.
            song.engine->setPluginBypassed (*song.arpSlot, false);
            song.clip->muted = false;
            result.clip->muted = true;   // a muted track still hands its notes to a plugin instrument
            TestMidiPlugin::heard.clear();
            song.engine->setPositionBeats (0.0);
            song.engine->play();
            render (*song.engine, 2.0 - 0.01);
            song.engine->stop();
            check (TestMidiPlugin::heard == expectedPitches, "write-out: the written notes equal what plays through the arpeggiator");
            song.clip->muted = true;
            result.clip->muted = false;
        }
    }

    // Undo and a second write-out.
    {
        WriteOutSong song;
        auto* second = song.addChords (16.0);
        const int oldTracks = (int) song.engine->getTracks().size();
        const auto first = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
        check (first.result.wasOk() && (int) song.engine->getTracks().size() == oldTracks + 1, "write-out: a clip writes out in one call " + first.result.getErrorMessage());

        const auto again = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
        check (! again.result.wasOk() && again.result.getErrorMessage() == "That clip is muted.", "write-out: a muted clip gives its problem");

        const auto next = bounce::writeOutArpeggio (*song.engine, *song.track, second);
        check (next.result.wasOk() && next.track == first.track && (int) song.engine->getTracks().size() == oldTracks + 1 && first.track->clips.size() == 2
                   && closeTo (next.clip->startBeat, 16.0, 1e-9) && closeTo (next.clip->notes[0].start, 0.0, 1e-9),
               "write-out: the next clip goes on the same \"(arpeggio)\" track, with notes relative to the clip");
        check (song.engine->canUndoArpeggioWriteOut (*second) && ! song.engine->canUndoArpeggioWriteOut (*song.clip),
               "write-out: undo remembers the latest write-out only");

        check (song.engine->undoArpeggioWriteOut() && ! second->muted && first.track->clips.size() == 1 && song.engine->indexOf (first.track) >= 0,
               "write-out: undo removes the clip and unmutes the source, keeping a track it didn't make");
        check (! song.engine->canUndoArpeggioWriteOut (*second) && ! song.engine->undoArpeggioWriteOut(), "write-out: undo works once");

        // The first write-out made the track: undo of that one removes it.
        WriteOutSong other;
        const auto made = bounce::writeOutArpeggio (*other.engine, *other.track, other.clip);
        const int tracks = (int) other.engine->getTracks().size();
        check (made.result.wasOk() && other.engine->undoArpeggioWriteOut() && (int) other.engine->getTracks().size() == tracks - 1 && ! other.clip->muted,
               "write-out: undo removes the track the write-out made, and unmutes the source");

        // After a note edit the undo is refused.
        const auto again2 = bounce::writeOutArpeggio (*other.engine, *other.track, other.clip);
        {
            const juce::ScopedLock sl (other.engine->getLock());
            again2.clip->notes[0].start += 0.125;
        }
        check (again2.result.wasOk() && ! other.engine->canUndoArpeggioWriteOut (*other.clip) && ! other.engine->undoArpeggioWriteOut(), "write-out: undo is refused after a note of the new clip was moved");
    }

    // Problems.
    {
        WriteOutSong song;
        auto* audio = song.engine->addTrack (Track::Type::audio);
        check (! bounce::planWriteOut (*song.engine, *audio).isOk(), "write-out: an audio track has nothing to write out");

        auto* plain = song.engine->addTrack (Track::Type::midi);
        auto* plainClip = song.engine->addMidiClip (*plain, 0.0, 4.0);
        plainClip->notes.push_back ({ 60, 0.0, 1.0, 0.8f });
        check (! bounce::planWriteOut (*song.engine, *plain, plainClip).isOk(), "write-out: a track without a MIDI effect has nothing to write out");

        song.engine->setPluginBypassed (*song.arpSlot, true);
        check (! bounce::planWriteOut (*song.engine, *song.track, song.clip).isOk(), "write-out: a bypassed MIDI effect has nothing to write out");
        song.engine->setPluginBypassed (*song.arpSlot, false);

        auto* empty = song.engine->addMidiClip (*song.track, 8.0, 4.0);
        const auto emptyPlan = bounce::planWriteOut (*song.engine, *song.track, empty);
        check (! emptyPlan.isOk() && emptyPlan.problem.isNotEmpty(), "write-out: an empty clip has nothing to write out");
        check (bounce::planWriteOut (*song.engine, *song.track).isOk() && closeTo (bounce::planWriteOut (*song.engine, *song.track).endBeat, 4.0, 1e-9),
               "write-out: the track's range is that of its unmuted clips with notes");
        song.engine->setClipMuted (*song.clip, true);
        check (! bounce::planWriteOut (*song.engine, *song.track, song.clip).isOk(), "write-out: a muted clip has nothing to write out");

        // Rests only: nothing comes out.
        song.engine->setClipMuted (*song.clip, false);
        const auto numTracks = song.engine->getTracks().size();
        auto s = song.getSettings();
        s = withKinds (s, "RRRR");
        song.arpeggiator->setSettings (s);
        const auto none = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
        check (! none.result.wasOk() && none.result.getErrorMessage().startsWith ("Nothing came out") && ! song.clip->muted && song.engine->getTracks().size() == numTracks,
               "write-out: when nothing comes out, nothing changes and the message says so");
    }

    // Settings: accents, swing, ties, a later bar, a tempo change, Random, Latch.
    {
        auto written = [&] (const std::function<void (arp::Settings&)>& change, double start = 0.0, const std::function<void (Engine&)>& before = {})
        {
            WriteOutSong song (start);
            auto s = song.getSettings();
            change (s);
            song.arpeggiator->setSettings (s);

            if (before)
                before (*song.engine);

            const auto result = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
            return result.clip != nullptr ? result.clip->notes : std::vector<MidiNote>();
        };

        const auto accented = written ([] (arp::Settings& s) { for (int i : { 0, 4, 8, 12 }) s.accents[(size_t) i] = true; });
        bool accentsOk = accented.size() == 16;

        for (size_t k = 0; accentsOk && k < 16; ++k)
            accentsOk = closeTo (accented[k].velocity, (k % 4 == 0 ? 122.0 : 102.0) / 127.0, 1e-4);

        check (accentsOk, "write-out: accent 20 on steps 1, 5, 9, 13 gives 122/127 there and 102/127 elsewhere");

        const auto bar5 = written ([] (arp::Settings&) {}, 16.0);
        bool same = bar5.size() == 16 && pitchesOf (bar5) == expectedPitches;

        for (size_t k = 0; same && k < bar5.size(); ++k)
            same = closeTo (bar5[k].start, 0.25 * (double) k, oneSample) && closeTo (bar5[k].length, 0.2, oneSample);

        check (same, "write-out: the same clip at bar 5 gives the same notes relative to the clip");

        const auto swung = written ([] (arp::Settings& s) { s.swingPercent = 54.0f; });
        check (swung.size() == 16 && closeTo (swung[1].start - swung[0].start, 0.27, oneSample) && closeTo (swung[2].start, 0.5, oneSample),
               "write-out: swing 54 % puts the odd steps 0.27 beat after the even one");

        const auto tied = written ([] (arp::Settings& s) { s.kinds[1] = arp::StepKind::tie; });
        check (tied.size() == 15 && closeTo (tied[0].length, arp::getNoteEndBeats (0, 2, arp::Settings{}.rate, 50.0f, 80.0f), oneSample) && closeTo (tied[0].length, 0.45, oneSample),
               "write-out: a tie on step 2 makes step 1's note 0.45 beat long and leaves 15 notes");

        const auto slowed = written ([] (arp::Settings&) {}, 0.0, [] (Engine& e) { e.setTempoMap (TempoMap (std::vector<TempoChange> { { 0.0, 120.0 }, { 2.0, 60.0 } })); });
        bool grid = slowed.size() == 16;

        for (size_t k = 0; grid && k < slowed.size(); ++k)
            grid = closeTo (slowed[k].start, 0.25 * (double) k, oneSample);

        check (grid, "write-out: through a tempo change from 120 to 60 BPM the starts are still 0.25 k beats");

        const auto random1 = written ([] (arp::Settings& s) { s.order = arp::Order::random; });
        const auto random2 = written ([] (arp::Settings& s) { s.order = arp::Order::random; });
        check (random1.size() == 16 && pitchesOf (random1) == pitchesOf (random2), "write-out: Random gives the same notes every time");

        const auto latched = written ([] (arp::Settings& s) { s.latch = true; });
        double lastEnd = 0.0, lastStart = 0.0;

        for (auto& n : latched)
        {
            lastEnd = juce::jmax (lastEnd, n.getEnd());
            lastStart = juce::jmax (lastStart, n.start);
        }

        check (! latched.empty() && lastStart < 4.0 && lastEnd <= 4.0 + 1e-9, "write-out: with Latch on, no note starts at or after the clip's end and the last ends by it");
    }

    // Random: the write-out equals the playback.
    {
        WriteOutSong song;
        auto s = song.getSettings();
        s.order = arp::Order::random;
        song.arpeggiator->setSettings (s);
        TestMidiPlugin::heard.clear();
        song.engine->setPositionBeats (0.0);
        song.engine->play();
        render (*song.engine, 2.0 - 0.01);
        song.engine->stop();
        const auto played = TestMidiPlugin::heard;
        const auto result = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
        check (result.clip != nullptr && pitchesOf (result.clip->notes) == played && played.size() == 16, "write-out: Random written out equals what plays");
    }

    // The new track copies the sound, the mixer settings, the bus and the sends.
    {
        WriteOutSong song;
        auto* bus = song.engine->createBus ({ song.track }, "Synths");
        auto* reverb = song.engine->addReturn ("Reverb");
        song.engine->setSend (*song.track, *reverb, -12.0f);
        routing::findSend (*song.track, *reverb)->preFader = true;
        song.track->volumeDb = -6.0f;
        song.track->pan = -0.3f;
        song.track->colour = juce::Colours::orange;
        song.engine->setScoreClef (*song.track, ScoreClef::bass);
        song.engine->setBuiltInSound (*song.track, BuiltInSound::pluck);
        juce::String error;
        auto* eq = song.engine->addPlugin (*song.track, BuiltInEq::getDescription(), error);
        song.engine->setPluginBypassed (*eq, true);
        const auto sourcePlugins = song.track->plugins.size();

        const auto result = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
        auto* t = result.track;
        check (result.result.wasOk() && t != nullptr && t->bus == bus && t->bus == song.track->bus && song.track->plugins.size() == sourcePlugins,
               "write-out: the new track plays into the same bus");

        if (t != nullptr)
        {
            auto* send = routing::findSend (*t, *reverb);
            check (send != nullptr && closeTo (send->levelDb.load(), -12.0, 1e-6) && send->preFader.load(), "write-out: the sends are copied");
            check (closeTo (t->volumeDb.load(), -6.0, 1e-6) && closeTo (t->pan.load(), -0.3, 1e-6) && t->colour == juce::Colours::orange && t->scoreClef == ScoreClef::bass
                       && t->builtInSound.load() == BuiltInSound::pluck,
                   "write-out: volume, pan, colour, clef and sound are copied");
            check (t->plugins.size() == 2 && t->plugins[0]->isInstrument && t->plugins[1]->isAudioEffect() && t->plugins[1]->bypassed.load()
                       && t->plugins[1]->instance->getName() == BuiltInEq::getDescription().name && t->getNumMidiEffects() == 0,
                   "write-out: the instrument and the audio effect are copied, with their bypass, and no MIDI effect");
        }
    }

    // Saved and loaded: the new track has its notes and its sound, and no MIDI effect.
    {
        WriteOutSong song (0.0, false);
        juce::String error, text;
        song.engine->addPlugin (*song.track, BuiltInEq::getDescription(), error);
        const auto result = bounce::writeOutArpeggio (*song.engine, *song.track, song.clip);
        const auto folder = juce::File::createTempFile ("arpeggiowriteout");
        folder.createDirectory();
        const juce::ScopeGuard guard { [folder] { folder.deleteRecursively(); } };
        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        juce::StringArray warnings;
        const bool ok = result.clip != nullptr && project::save (*song.engine, folder.getChildFile ("Song.dawit"), text, error)
                        && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
        MidiClip* clip = nullptr;
        Track* t = nullptr;

        if (ok && loaded.getTracks().size() == 2)
        {
            t = loaded.getTracks()[1].get();
            clip = t->clips.size() == 1 ? static_cast<MidiClip*> (t->clips[0].get()) : nullptr;
        }

        bool sameSaved = clip != nullptr && result.clip != nullptr && clip->notes.size() == result.clip->notes.size();

        for (size_t i = 0; sameSaved && i < clip->notes.size(); ++i)   // the text keeps a double to 15 digits
            sameSaved = clip->notes[i].pitch == result.clip->notes[i].pitch && closeTo (clip->notes[i].start, result.clip->notes[i].start, 1e-9)
                        && closeTo (clip->notes[i].length, result.clip->notes[i].length, 1e-9) && closeTo (clip->notes[i].velocity, result.clip->notes[i].velocity, 1e-6);

        check (clip != nullptr && t->name == "Keys (arpeggio)" && t->getNumMidiEffects() == 0 && t->plugins.size() == 1 && t->plugins[0]->isAudioEffect()
                   && sameSaved && closeTo (clip->lengthBeats, 4.0, 1e-9),
               "write-out: a project with the result saves and loads with the same notes " + error + warnings.joinIntoString (" "));
        check (loaded.getTracks().size() == 2 && loaded.getTracks()[0]->clips.size() == 1 && loaded.getTracks()[0]->clips[0]->muted && loaded.getTracks()[0]->getNumMidiEffects() == 1,
               "write-out: the muted source and its arpeggiator are saved too");
    }

    // The track version: all the unmuted clips in one pass, and the source track is muted.
    {
        WriteOutSong song;
        song.addChords (8.0);
        const auto result = bounce::writeOutArpeggio (*song.engine, *song.track);
        check (result.result.wasOk() && result.numNotes == 32 && closeTo (result.clip->startBeat, 0.0, 1e-9) && closeTo (result.clip->lengthBeats, 12.0, 1e-9)
                   && song.track->muted.load() && song.engine->canUndoArpeggioWriteOut (*song.track) && song.engine->canUndoArpeggioWriteOut (*result.track),
               "write-out: the track version writes all its clips into one clip and mutes the source track");
        check (song.engine->undoArpeggioWriteOut() && ! song.track->muted.load(), "write-out: undoing the track version unmutes the track");
    }
}
