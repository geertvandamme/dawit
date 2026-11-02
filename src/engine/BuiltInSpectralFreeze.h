#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "EffectTools.h"
#include "SongContext.h"

namespace daw
{
namespace sfz
{
    /** The controls with a value of their own, in the order of the window. Freeze is a parameter too, but a switch. */
    enum class Knob { fade, brightness, movement, mix, output };
    constexpr int numKnobs = 5;

    constexpr float minFadeMs = 10.0f, maxFadeMs = 10000.0f;
    constexpr float minBrightnessDb = -6.0f, maxBrightnessDb = 6.0f;
    constexpr float minOutputDb = -24.0f, maxOutputDb = 12.0f;

    /** The sound is held as this many overlapping pieces: each piece starts a quarter of its length after the one before. */
    constexpr int overlap = 4;

    /** Hann squared, overlapped at a quarter of its length, sums to this at every sample. */
    constexpr double overlapSum = 1.5;

    /** How many pieces the snapshot looks at, and the newest piece against the one before gives each bin's frequency. */
    constexpr int captureFrames = 4;

    /** Plucked A minor test chord: peak level (linear, -12 dBFS), length and when it is captured, in seconds. */
    constexpr float testChordPeak = 0.25f;
    constexpr double testChordSeconds = 2.0, testChordCaptureSeconds = 0.25;

    struct Settings
    {
        bool freeze = false;
        float fadeMs = 300.0f;
        float brightnessDb = 0.0f;       // dB per octave about 1 kHz
        float movementPercent = 20.0f;
        float mixPercent = 100.0f;
        float outputDb = 0.0f;
    };

    //==============================================================================
    /** The size of each piece of sound the freeze looks at: 4,096 samples up to 48 kHz, 8,192 up to 96 kHz, 16,384 above, so a piece lasts about 93 ms. */
    int getFftSize (double sampleRate);

    /** The step between pieces: a quarter of the piece. */
    inline int getHopSize (int fftSize) { return fftSize / overlap; }

    /** A real FFT of `fftSize` samples has size / 2 + 1 bins, from 0 Hz to half the rate. */
    inline int getNumBins (int fftSize) { return fftSize / 2 + 1; }

    /** The gap between neighbouring bins: sample rate / size (10.77 Hz at 44.1 kHz and 4,096). */
    double getBinSpacingHz (double sampleRate, int fftSize);

    /** How long a piece is, in ms: size / rate (92.9 ms at 44.1 kHz and 4,096). */
    double getWindowMs (double sampleRate, int fftSize);

    /** How many bins have a centre frequency from `lowHz` up to, not including, `highHz`. */
    int countBins (double lowHz, double highHz, double sampleRate, int fftSize);

    /** How many bins apart two notes a semitone above and below `hz` are. */
    double getSemitoneGapBins (double hz, double sampleRate, int fftSize);

    /** The periodic Hann window of `size` samples. */
    std::vector<float> makeHann (int size);

    /** The sum of the Hann window squared over every piece that overlaps at sample `at`, hop size / 4: 1.5 everywhere. */
    double getOverlapSum (int size, int at);

    /** The size of a capture in samples: one piece plus three hops (7,168 samples, 162.5 ms, at 44.1 kHz). */
    int getCaptureSpan (int fftSize);

    /** Brightness: the gain in dB at `hz` of a tilt of `dbPerOctave` about 1 kHz. */
    double getTiltDb (double hz, double dbPerOctave);

    /** The gains of the dry sound and of the hold while the hold is `progress` (0 to 1) of the way in, at Mix `mixPercent`: equal power, dry * cos(theta), hold * sin(theta), theta = progress * mix * pi/2. */
    std::pair<double, double> getFadeGains (double progress, double mixPercent);

    /** The exact frequency in Hz of the sound in bin `bin`, from the phase of that bin in two pieces one hop apart. Within 2 bins of the bin's centre. */
    double getPhaseVocoderHz (int bin, double earlierPhase, double laterPhase, double sampleRate, int fftSize);

    /** The spectral flatness of `count` powers: geometric mean over arithmetic mean, 0 for pure tones and 1 for white noise. Powers below 1e-12 (-120 dB) count as 1e-12. */
    double getSpectralFlatness (const double* powers, int count);

    /** "A3", "A#3": the nearest note of a frequency, middle C being C4. */
    juce::String getNoteName (double hz);

    /** The nearest note's MIDI number, and how many cents (hundredths of a semitone) `hz` is above it (-50 to 50). */
    struct Note
    {
        int midi = 0;
        double cents = 0.0;
    };

    Note getNearestNote (double hz);

    /** The pitch class (0 is C) of a MIDI note. */
    int getPitchClass (int midi);

    //==============================================================================
    /** The held moment: for each channel and bin, how loud the bin was (an FFT magnitude, RMS over the four pieces), its exact frequency and its phase in the newest piece. */
    struct Snapshot
    {
        double rate = 0.0;
        int size = 0;
        int channels = 0;           // 0: nothing held
        float priorPeakRms = 0.0f;  // the loudest 23 ms (linear RMS) of the second before the capture, for the advice
        std::array<std::vector<float>, 2> level, freq, phase;

        bool isHeld() const noexcept { return channels > 0 && size > 0; }
        int getNumBins() const noexcept { return sfz::getNumBins (size); }

        /** Sets the size and channels and zeroes everything; allocates. */
        void prepare (double sampleRate, int fftSize, int numChannels);
    };

    /** The same hold for another sample rate: each bin's energy moves to the bin of its frequency and is spread over its neighbours like a Hann window's peak. The total power stays. */
    Snapshot remapSnapshot (const Snapshot&, double newRate, int newSize);

    /** The mean square (linear power) of the sound the hold stands for, in one channel: Parseval over the bins. A full-scale sine is 0.5. */
    double getSnapshotPower (const Snapshot&, int channel);

    /** The RMS of the hold over its channels, linear. */
    double getSnapshotRms (const Snapshot&);

    /** The spectral flatness of a channel of the hold over 50 Hz to 16 kHz. */
    double getSnapshotFlatness (const Snapshot&, int channel);

    /** The frequency of the loudest bin's tone, in Hz (0 for a silent hold): its estimated frequency, summed over the bins that share it. */
    double getSnapshotPeakHz (const Snapshot&);

    /** The hold stored as text for the plugin's state (base64), and back. `fromText` returns an empty snapshot for text that isn't one. */
    juce::String snapshotToText (const Snapshot&);
    Snapshot snapshotFromText (const juce::String&, double rate, int size, int channels, float priorPeakRms);

    //==============================================================================
    /** "300 ms", "1.5 s", "10 s", "+3.0 dB/oct", "20%", "100%", "-9.0 dB": as the window shows them. */
    juce::String formatKnobValue (Knob, float value);

    /** The same from the settings. */
    juce::String formatKnob (Knob, const Settings&);

    /** A starting point with why its settings are what they are. It leaves Freeze as it is. */
    struct Preset
    {
        juce::String name, why;
        Settings settings;
    };

    /** The six starting points, Held chord first. */
    const std::vector<Preset>& getPresets();

    //==============================================================================
    /** What `analyse` looks at. Levels are dBFS of the hold's RMS. */
    struct AnalysisInput
    {
        bool holding = false;
        float movementPercent = 20.0f;
        double holdDb = -100.0;            // the hold's RMS
        double priorPeakDb = -100.0;       // the loudest 23 ms of the second before the capture
        double flatness = 0.0;
        double peakHz = 0.0;               // the tallest peak
        bool keyKnown = false;
        int keyPitchClasses = 0;           // bit 0 is C
        juce::String keyName;              // "A minor"
    };

    /** The advice for *Your track*, rules Z1 to Z8 of the plan, as lines of text. */
    std::vector<juce::String> analyse (const AnalysisInput&);
} // namespace sfz

//==============================================================================
/**
    DawIt Spectral Freeze: takes about the last 0.15 s of a track's sound and holds it for as long as you like.
    Freeze keeps a snapshot (each frequency's level and exact pitch, taken from four overlapping 93 ms
    pieces of the sound) and builds new sound from it every 23 ms, each bin carrying on from where its
    wave was, so the hold has no loop and no seam. Movement adds random phase and level wander,
    Brightness tilts the colour about 1 kHz, Fade is how long the hold takes to come in and go, Mix
    sets it against the track (equal power) and Output sets the level. With nothing held the track
    passes untouched. The hold carries on through stop and play, and is saved with the plugin's state.
    No latency: the hold is made from the past.

    For learning it also plays a test chord that it holds 0.25 s in, and lets the window read the live
    input and the published snapshot.
*/
class BuiltInSpectralFreeze final : public juce::AudioPluginInstance, public SongContextReceiver, private juce::Timer
{
public:
    BuiltInSpectralFreeze();
    ~BuiltInSpectralFreeze() override;

    static constexpr const char* identifier = "DawIt Spectral Freeze";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInSpectralFreeze&)> createEditorFunction;

    /** The parameters: the knobs, and "freeze". */
    static const char* getParameterId (sfz::Knob);
    static constexpr const char* freezeParameterId = "freeze";
    juce::RangedAudioParameter* getKnobParameter (sfz::Knob) const;
    juce::RangedAudioParameter* getFreezeParameter() const;

    sfz::Settings getSettings() const;

    /** Changes the parameters, as a user would. Message thread. */
    void setSettings (const sfz::Settings&);
    void setKnob (sfz::Knob, float value);
    void setFreeze (bool);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Takes a new snapshot of the sound now and crossfades to it (Capture again). Does nothing while not holding. Message thread. */
    void requestCapture() noexcept { captureRequested = true; }

    /** Plays the test chord and holds it 0.25 s in, replacing any hold; Freeze is switched on a moment later. Message thread. */
    void playTestChord();

    /** Plays A3 and A#3 together for a second without holding them, for the guide's page on time and frequency. Message thread. */
    void playTwoNotes() noexcept { pairRequested = true; }

    /** How many times the test chord has been captured, counted on the audio thread. */
    int getChordCaptures() const noexcept { return chordCaptures.load(); }

    /** True while the hold is on or fading out; how far in it is (0 to 1). */
    bool isHolding() const noexcept { return holdingFlag.load(); }
    float getHoldAmount() const noexcept { return holdAmount.load(); }

    /** The published snapshot, copied. False when nothing has been captured. Message thread. */
    bool getSnapshot (sfz::Snapshot& dest) const;
    bool hasSnapshot() const noexcept { return latestSlot.load() >= 0; }

    /** Copies the newest `count` input samples of a channel without consuming them, oldest first; returns how many samples have been written in all. */
    juce::int64 readInput (int channel, float* dest, int count) const;

    /** The sample rate the plugin was prepared with, and its piece size. */
    double getRate() const noexcept { return rate; }
    int getFftSize() const noexcept { return fftSize; }

    /** The notes of the song's key (bit 0 is C), 0 while unknown. */
    int getKeyPitchClasses() const noexcept { return keyBits.load(); }

    fx::LevelHistory levels;   // per 64 samples: the input's peak, and the peak of the hold as it is mixed in

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override { return (double) getSettings().fadeMs / 1000.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return createEditorFunction != nullptr; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void setSongContext (int keyPitchClasses, int) noexcept override { keyBits = keyPitchClasses; }

private:
    struct HoldVoice;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;

    void allocate();
    void captureInto (sfz::Snapshot&);
    sfz::Snapshot& beginWrite();
    void publish (sfz::Snapshot&);
    void startVoice (const sfz::Snapshot&);
    void captureAndStart();
    void adoptIncoming();
    void processSegment (juce::AudioBuffer<float>&, int from, int count, int numChannels);
    float getChordSample (double t) const;
    void handOverState (sfz::Snapshot&&);

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, sfz::numKnobs> knobValues {};
    std::atomic<float>* freezeValue = nullptr;

    double rate = 0.0;
    int fftSize = 4096, hop = 1024, numChannelsUsed = 2;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> hann;
    float inverseScale = 1.0f;
    std::vector<float> fftData, spare;

    // The last second of input, per channel: the audio thread writes, the window reads without consuming.
    int ringSize = 0;
    std::array<std::unique_ptr<std::atomic<float>[]>, 2> ring;
    std::atomic<juce::int64> ringWritten { 0 };
    juce::int64 ringPosition = 0;   // the audio thread's own count

    // The snapshot: three slots, the audio thread writes the one after the published one (each slot has a counter, odd while written).
    std::array<sfz::Snapshot, 3> slots;
    std::array<std::atomic<unsigned>, 3> slotGeneration {};
    std::atomic<int> latestSlot { -1 };

    // A snapshot from the plugin's state for the audio thread to adopt.
    sfz::Snapshot incoming;
    std::atomic_flag incomingLock;
    std::atomic<bool> incomingReady { false };
    std::unique_ptr<sfz::Snapshot> restoredBeforePrepare;

    // The hold: two voices (the old one fades out while the new one comes in), and the gains.
    std::array<std::unique_ptr<HoldVoice>, 2> voices;
    int currentVoice = 0;
    bool adoptedNow = false, crossfading = false, holdTarget = false, lastFreeze = false, chordHold = false, useStoredOnRise = false;
    double holdAngle = 0.0, crossfade = 0.0;   // holdAngle: 0 to 1 of the way to the hold; crossfade: 0 (old voice) to 1 (new voice)
    unsigned voiceCounter = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed, outputSmoothed;

    std::atomic<bool> captureRequested { false }, holdingFlag { false };
    std::atomic<float> holdAmount { 0.0f };
    std::atomic<int> keyBits { 0 };

    // The test chord.
    std::atomic<bool> chordRequested { false }, pairRequested { false };
    bool pairMode = false;
    std::atomic<int> chordCaptures { 0 };
    int chordPosition = -1, chordCaptureAt = 0, seenChordCaptures = 0, timerTicks = 0, writingSlot = 0;
    bool chordPending = false;

    // The level history's chunk.
    float chunkIn = 0.0f, chunkHold = 0.0f;
    int chunkCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInSpectralFreeze)
};
} // namespace daw
