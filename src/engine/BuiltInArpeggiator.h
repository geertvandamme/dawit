#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SongContext.h"
#include "StepGrid.h"

namespace daw
{
namespace arp
{
    constexpr int maxSteps = stepgrid::maxSteps;
    constexpr int maxHeld = 16;                    // keys it follows at once
    constexpr int maxOctaves = 4;
    constexpr int maxSequence = maxHeld * maxOctaves;

    /** The path through the held notes. */
    enum class Order { up, down, upDown, downUp, outsideIn, asPlayed, alberti, random };
    constexpr int numOrders = 8;

    /** What a step plays: the next note of the order, the whole chord, the lowest note an octave down, nothing, or the step before goes on. */
    enum class StepKind { note, chord, bass, rest, tie };
    constexpr int numStepKinds = 5;

    /** "Up", "Down", "Up and down", "Down and up", "Outside in", "As played", "Alberti", "Random". */
    const char* getOrderName (Order);

    /** The one-line description of an order, as the Order menu shows it. */
    const char* getOrderHint (Order);

    /** "Note", "Chord", "Bass", "Rest", "Tie". */
    const char* getKindName (StepKind);

    /** The tooltip of a brush ("The next note of the order"). */
    const char* getKindHint (StepKind);

    /** One letter per kind for the pattern text: N, C, B, R, T (lower case when the step is accented). */
    char getKindLetter (StepKind);

    /** The controls with a value of their own, in the order of the window. The 16 step kinds and accents are parameters too, but not knobs. */
    enum class Knob { order, octaves, rate, gate, swing, steps, accent, latch };
    constexpr int numKnobs = 8;

    constexpr float minGate = 10.0f, maxGate = 200.0f, minSwing = 50.0f, maxSwing = 75.0f;
    constexpr int maxAccent = 40;

    struct Settings
    {
        Order order = Order::up;
        int octaves = 1;
        int rate = stepgrid::defaultStepLength;   // index in stepgrid::stepLengths
        float gatePercent = 80.0f;                // a note lasts this share of its step
        float swingPercent = 50.0f;               // where the second step of each pair falls; 50 is straight
        int steps = maxSteps;                     // how many steps before the pattern starts again
        int accent = 20;                          // MIDI velocity added on an accented step
        bool latch = false;
        std::array<StepKind, maxSteps> kinds {};  // all Note
        std::array<bool, maxSteps> accents {};
    };

    /** A key that is held: its pitch, velocity, and when it was pressed (notes pressed together share a number). `down` is false for a key that only latch or the pedal holds. */
    struct HeldNote
    {
        int pitch = 60, velocity = 100;
        int order = 0;
        bool down = true;
    };

    /** A note of the sequence: its pitch and the velocity of the key it comes from. */
    struct SeqNote
    {
        int pitch = 60, velocity = 100;
    };

    /** The notes the order walks through, from the held notes, order and octaves: the first octave, then the same notes 12 semitones up, and so on. Notes above 127 are dropped. `out` is cleared first. */
    void buildSequence (const std::vector<HeldNote>& held, Order, int octaves, std::vector<SeqNote>& out);

    /** The same as a new vector. */
    std::vector<SeqNote> getSequence (const std::vector<HeldNote>& held, Order, int octaves);

    /** How many steps of Note it takes for the order to come round again with this many held notes: 3 notes Up and down is 4, over 2 octaves 10. */
    int getCycleLength (int numNotes, Order, int octaves);

    /** Which note of `count` Random picks for a step: the same each time for the same step of the song. */
    int getRandomIndex (long long songStep, int count);

    /** Where step `k` (counted from the start of the song) starts, in quarter notes: every second step is moved later by swing (50 % straight, 66.7 % a triplet shuffle). */
    double getStepStartBeats (long long k, int rate, float swingPercent);

    /** How many steps a note started on pattern step `index` lasts: itself and the Tie steps right after it (at most the pattern's length). */
    int getTiedSteps (const Settings&, int index);

    /** Where a note started on step `k` ends: the start of its last tied step plus the gate share of that step, in quarter notes. */
    double getNoteEndBeats (long long k, int tiedSteps, int rate, float swingPercent, float gatePercent);

    /** The velocity of an accented step: the key's velocity plus the accent, at most 127. */
    int getAccentedVelocity (int velocity, int accent);

    /** The pitch of a Bass step: the lowest held note an octave down (the note itself when that would be below 0). */
    int getBassPitch (int lowestPitch);

    /** How many Note and Chord steps a second the pattern plays at this tempo: steps a second times the share of Note and Chord steps. */
    double getNotesPerSecond (const Settings&, double bpm);

    /** After how many patterns the order comes round to the same note on the same step again: cycle / gcd (cycle, noteSteps). 1 when the cycle divides the Note steps. */
    int getCrossingPatterns (int cycle, int noteSteps);

    /** After how many patterns of `steps` steps a pattern lines up with the bar again (4/4): 8 for 6 steps of 1/16, which is 12 beats. */
    int getBarRealignPatterns (int steps, int rate);

    /** The pitch classes (bit 0 = C) of the held notes of a record or chord, named as a chord: "Am7", or the note names when nothing fits. */
    juce::String nameChord (int pitchClasses, int lowestPitchClass);

    /** The pattern as a line of text, to copy and paste: "DawIt Arpeggiator pattern: 1/16 16 NNcR...", with the rate, Steps and one letter per step (lower case when accented). */
    juce::String patternToText (const Settings&);

    /** Reads such a line into `s`, changing only its rate, Steps, kinds and accents. False (and `s` untouched) when the text isn't a pattern. */
    bool patternFromText (const juce::String&, Settings& s);

    /** A starting point with why its settings are what they are. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
        juce::String group;   // the heading it sits under in the menu
    };

    /** The fifty-one starting points, the plain "Up" first, in five groups: Basics, Single notes, Dance and synth, Piano, guitar and harp, Chords and rhythms. */
    const std::vector<Preset>& getPresets();

    /** A new random step row for `in` (the same seed gives the same one): only the step kinds and accents change. Step 1 always sounds, a Tie never follows a Rest or starts the pattern, at most 3 Rests in a row,
        at least 3 sounding steps and 2 Notes from 4 steps up, and at most a third of the sounding steps accented (step 1 always is). */
    Settings randomisePattern (const Settings& in, juce::int64 seed);

    /** A fixed example to play into the instrument without changing the settings: these settings on these notes, for `beats` quarter notes. */
    struct Example
    {
        Settings settings;
        std::array<int, maxHeld> pitches {};
        int numPitches = 0;
        int velocity = 100;
        double beats = 8.0;
    };

    //==============================================================================
    /** What happened in one step, as the arpeggiator saw it: for the pictures and for `analyse`. */
    struct StepRecord
    {
        int step = 0;                                   // 0 is the first step of the pattern
        StepKind kind = StepKind::note;
        int numPlayed = 0;
        std::array<std::uint8_t, maxHeld> played {};     // the pitches of the notes this step started
        std::array<std::uint64_t, 2> heldMask {};        // the held keys: bit p of word p / 64
        int chordPitchClasses = 0;                       // the chord track's chord of the bar, 0 when there is none
        double beat = 0.0;                               // where the step starts, in quarter notes

        bool isHeld (int pitch) const noexcept { return pitch >= 0 && pitch < 128 && ((heldMask[(size_t) (pitch / 64)] >> (pitch % 64)) & 1ULL) != 0; }
        int getNumHeld() const noexcept;
        int getLowestHeld() const noexcept;             // -1 when nothing is held
    };

    /** The newest step records: written by the audio thread, read (without taking them) by the window and the guide. */
    class StepHistory
    {
    public:
        static constexpr int size = 256;

        void push (const StepRecord&) noexcept;

        /** Copies up to `max` of the newest records, oldest first, and returns how many. */
        int getRecent (StepRecord* dest, int max) const;

        /** Forgets everything. Message thread. */
        void clear() noexcept { cleared = written.load(); }

    private:
        struct Slot
        {
            std::array<std::atomic<unsigned long long>, 6> words {};
        };

        std::array<Slot, (size_t) size> slots;
        std::atomic<unsigned long long> written { 0 };
        std::atomic<unsigned long long> cleared { 0 };
    };

    /** What `analyse` looks at. `records` are the steps of the last 2 bars, oldest first. */
    struct AnalysisInput
    {
        Settings settings;
        std::vector<StepRecord> records;
        double bpm = 120.0;              // the tempo the arpeggiator used
    };

    /** The advice for *Your track*, rules 1 to 5 of the plan, as lines of text. */
    std::vector<juce::String> analyse (const AnalysisInput&);
} // namespace arp

//==============================================================================
/**
    DawIt Arpeggiator: a MIDI effect that turns held chords into patterns, in time with the song. It sits
    before the instrument on a MIDI track. Each step of its pattern plays the next note of the order (Up, Down,
    Up and down, Down and up, Outside in, As played, Alberti or Random) over 1 to 4 octaves, the whole chord, the
    bass note, a rest or a tie. It follows the song's clock while the song plays, and a clock of its own at the
    song's tempo otherwise, started again at a new chord. It ends every note it starts: when you let go, stop,
    jump, or switch it off. Latch keeps the chord after you let go; the sustain pedal holds the keys and isn't
    passed on. Switched off, it passes the notes through.

    For learning it also plays a test chord (the chord track's chord of the bar) and fixed examples, and keeps a
    record of its steps for the window's pictures.
*/
class BuiltInArpeggiator final : public juce::AudioPluginInstance, public SongContextReceiver
{
public:
    BuiltInArpeggiator();

    static constexpr const char* identifier = "DawIt Arpeggiator";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInArpeggiator&)> createEditorFunction;

    /** The parameters: the knobs, "kind1" to "kind16" and "accent1" to "accent16" for the steps. */
    static const char* getParameterId (arp::Knob);
    static juce::String getKindParameterId (int step);
    static juce::String getAccentParameterId (int step);
    juce::RangedAudioParameter* getKnobParameter (arp::Knob) const;
    juce::RangedAudioParameter* getKindParameter (int step) const;
    juce::RangedAudioParameter* getAccentParameter (int step) const;

    arp::Settings getSettings() const;
    arp::StepKind getKind (int step) const;
    bool isAccented (int step) const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const arp::Settings&);
    void setKnob (arp::Knob, float value);
    void setKind (int step, arp::StepKind);
    void setAccented (int step, bool accented);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Plays the chord track's chord of the bar (C4 E4 G4 without one) through the arpeggiator for 2 bars, from step 1. Any thread. */
    void playTestChord() noexcept { testChordRequested = true; }

    /** Plays a fixed example into the instrument, with its own settings and notes, without changing the parameters. Message thread. */
    void playExample (const arp::Example&);

    /** Stops a test chord or example now. */
    void stopExample() noexcept { stopRequested = true; }

    /** Whether a test chord or example is playing. */
    bool isExamplePlaying() const noexcept { return examplePlaying.load(); }

    /** The tempo the steps follow now, in BPM, and the host's, 0 while unknown. */
    double getUsedBpm() const { return usedBpm.load(); }
    std::atomic<double> tempoBpm { 0.0 };

    /** The step (0 is the first) played last, -1 before any; the position in quarter notes at the start of the last block; and whether it follows the song's play head. */
    std::atomic<int> playingStep { -1 };
    std::atomic<double> clockBeat { 0.0 };
    std::atomic<bool> usingSongClock { false };

    /** The keys it follows now, the notes it sounds now (bit p of word p / 64), and the pitch classes of the chord track's chord of the bar. */
    std::array<std::atomic<unsigned long long>, 2> heldKeys {}, soundingNotes {};
    std::atomic<int> chordPitchClasses { 0 };

    arp::StepHistory history;

    //==============================================================================
    void setSongContext (int keyPitchClasses, int chordPitchClasses) noexcept override;

    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return createEditorFunction != nullptr; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void resetState();
    void endOwnNotes (juce::MidiBuffer& out, int pos);
    void sendNoteOn (juce::MidiBuffer& out, int pos, int pitch, int velocity, long long offAt);
    void sendNoteOff (juce::MidiBuffer& out, int pos, int pitch);
    void sendDueOffs (juce::MidiBuffer& out, int pos);
    void handleNoteOn (juce::MidiBuffer& out, int pos, int pitch, int velocity, const arp::Settings&);
    void handleNoteOff (juce::MidiBuffer& out, int pos, int pitch, const arp::Settings&);
    void chordStart (juce::MidiBuffer& out, int pos, const arp::Settings&);
    void fireStep (juce::MidiBuffer& out, int pos, long long k, const arp::Settings&);
    void scheduleNextStep (const arp::Settings&, long long minK);
    void beginExample (juce::MidiBuffer& out, const arp::Example&);
    void endExample (juce::MidiBuffer& out, int pos);
    void publishState();
    bool hasHeld() const noexcept { return ! held.empty(); }

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, arp::numKnobs> knobValues {};
    std::array<std::atomic<float>*, arp::maxSteps> kindValues {}, accentValues {};

    double rate = 44100.0;
    stepgrid::Clock stepClock;
    std::atomic<double> usedBpm { 120.0 };
    std::atomic<int> songChord { 0 };

    // What it follows.
    std::vector<arp::HeldNote> held;
    bool pedalDown = false, wasLatch = false;
    int orderStamp = 0;
    long long lastStampSample = -1;
    long long orderCounter = 0;

    // What it has started: the note-offs are due at an absolute sample.
    std::array<bool, 128> sounding {};
    std::array<long long, 128> offAt {};
    int numSounding = 0;

    // This block's clock.
    long long samplesDone = 0;
    double beat0 = 0.0, beatsPerSample = 0.0;
    long long nextK = 0, lastFiredK = std::numeric_limits<long long>::min();
    int nextStepSample = 0;
    bool songBased = false;

    // Test chord and examples.
    std::atomic<bool> testChordRequested { false }, stopRequested { false }, examplePlaying { false };
    juce::SpinLock exampleLock;
    arp::Example pendingExample;
    std::atomic<bool> exampleWaiting { false };
    bool exampleActive = false;
    arp::Example activeExample;
    long long exampleEndsAt = 0;

    std::vector<arp::SeqNote> sequence;
    juce::MidiBuffer scratchOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInArpeggiator)
};
} // namespace daw
