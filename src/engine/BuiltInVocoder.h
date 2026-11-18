#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "EffectTools.h"
#include "SongContext.h"
#include "Vocoder.h"

namespace daw
{
namespace vocoder
{
/** The controls of DawIt Vocoder, in the order of Settings. */
enum class Knob { bands, smoothing, consonants, voice, low, high, source, wave, root, shape, range, carrier, formant, mix };
constexpr int numKnobs = 14;
}

/**
    A channel vocoder: the track it sits on is the voice. The voice (left and right added, halved) is cut into 4 to 24
    log-spaced bands between Low and High; each band's loudness is followed by an envelope. A carrier made inside the
    effect (a saw, pulse, choir or noise, playing the song's chord, notes you choose or keys you play) is cut into the
    same bands, and each band is turned up and down by the voice's envelope. Consonants add noise to the top bands while
    the voice is hissy. The result goes through Carrier level and Mix and a soft limiter that keeps it within +-1.
    Moving Bands, Low or High crossfades from the old bank of filters to a new one over 20 ms. It reads the live MIDI the
    track passes it (keys), and the song's key and chord (SongContextReceiver). Stereo, or mono heard as the same sound left and
    right. No latency, no side input.
*/
class BuiltInVocoder final : public juce::AudioPluginInstance, public SongContextReceiver
{
public:
    BuiltInVocoder();
    ~BuiltInVocoder() override = default;

    static constexpr const char* identifier = "DawIt Vocoder";
    static juce::PluginDescription getDescription();
    static inline std::function<juce::AudioProcessorEditor* (BuiltInVocoder&)> createEditorFunction;

    static const char* getParameterId (vocoder::Knob);
    juce::RangedAudioParameter* getKnobParameter (vocoder::Knob) const;
    vocoder::Settings getSettings() const;
    void setSettings (const vocoder::Settings&);
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Sets every control to a starting point. */
    void applyPreset (const vocoder::Preset&);

    //==============================================================================
    // What the audio thread shows the window, the guide and the device bar chip (read from any thread, no lock).

    /** The bands in force (after any crossfade has started): the count, Low, and the top edge after the 0.45 × rate clamp. */
    int getBandsInForce() const noexcept { return bandsShown.load(); }
    double getLowInForce() const noexcept { return lowShown.load(); }
    double getHighInForce() const noexcept { return highShown.load(); }

    /** The latest level of each band, linear (1 is a full-scale sine at the band's centre): the voice, the carrier, the result. */
    float getVoiceLevel (int band) const noexcept { return voiceLevels[(size_t) band].load(); }
    float getCarrierLevel (int band) const noexcept { return carrierLevels[(size_t) band].load(); }
    float getResultLevel (int band) const noexcept { return resultLevels[(size_t) band].load(); }

    /** The pitch classes (bit 0 = C) the carrier plays now, and whether the chosen Notes stand in for a source that has nothing. */
    int getCarrierPitchMask() const noexcept { return carrierMask.load(); }
    bool isUsingFallback() const noexcept { return fallbackShown.load(); }

    /** The song's key and chord as the track last told it (pitch-class masks, 0: none). */
    int getSongKeyMask() const noexcept { return keyMask.load(); }
    int getSongChordMask() const noexcept { return chordMask.load(); }

    /** What the last 8 seconds were like, for "Your track". */
    vocoder::Summary getSummary() const;

    /** Plays the made-up voice "ah ee s" into the effect once, on top of what comes in. */
    void playTestVoice() noexcept { demoRequested = false; testRequested = true; }

    /** What a guide example plays: the voice alone, the carrier alone, or the vocoded result. */
    enum class DemoPart { voiceAlone, carrierAlone, result };

    /** The carrier an example plays: the one set in the window, or a pure sine, a saw or noise. */
    enum class DemoCarrier { asSet, sine, saw, noise };

    /** Plays the made-up voice once, on its own (what comes in is muted), as a guide example: the part to hear, with
        `bands` bands when above 0 and the given carrier. Mix is ignored; the other settings are the window's. */
    void playDemo (DemoPart part, int bands = 0, DemoCarrier carrier = DemoCarrier::asSet) noexcept
    {
        requestedPart = (int) part;
        requestedBands = bands;
        requestedCarrier = (int) carrier;
        demoRequested = true;
        testRequested = true;
    }

    /** Hear only one band (0-based), or all of them (-1). Session only. */
    void setHearOnlyBand (int band) noexcept { hearOnly = band; }
    int getHearOnlyBand() const noexcept { return hearOnly.load(); }

    /** How loud the voice and the effect were, for the window's level history. */
    fx::LevelHistory levelHistory;

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return createEditorFunction != nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void setSongContext (int keyPitchClasses, int chordPitchClasses) noexcept override
    {
        keyMask = keyPitchClasses;
        chordMask = chordPitchClasses;
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Coeffs { double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct State { double z1 = 0, z2 = 0; };
    static void makeBandPass (Coeffs&, double hz, double q, double rate);
    static double run (const Coeffs&, State&, double x) noexcept;

    /** One set of filters for one Bands, Low and High: the voice's analysis bands, the carrier's synthesis bands
        (one set per channel) and the meters of each band. */
    struct Bank
    {
        int bands = 0;
        double low = 0.0, top = 0.0, rate = 44100.0;
        int consonantMask = 0;
        std::array<double, vocoder::maxBands> centre {};
        std::array<Coeffs, vocoder::maxBands> analysis, synthesis;
        std::array<std::array<State, 2>, vocoder::maxBands> analysisState;
        std::array<std::array<std::array<State, 2>, 2>, vocoder::maxBands> synthesisState;
        std::array<double, vocoder::maxBands> quickCoef {};   // evens out the ripple of a band's own tone before the attack and release
        std::array<double, vocoder::maxBands> quickEnergy {}, voiceEnergy {}, carrierEnergy {}, resultEnergy {};

        void configure (int newBands, double newLow, double newTop, double newRate, double ratio) noexcept;
        void setFormant (double ratio) noexcept;
        void reset() noexcept;
        bool matches (int newBands, double newLow, double newTop) const noexcept;

        /** One sample: the voice and the carrier (plus noise for consonants in the masked bands) in, the band sum of each channel out. */
        void process (double voice, const double* carrier, const double* noise, double attack, double release, double meter, juce::uint32 hearMask, double* out) noexcept;
    };

    /** One note of the carrier, fading in and out. */
    struct Slot
    {
        int note = -1;
        double hz = 0.0;
        double gain = 0.0, target = 0.0, step = 0.0;
        double phase[2][3] {};
    };

    struct SummarySlot
    {
        float inputPeak = 0.0f, resultPeak = 0.0f;
        int samples = 0, hissy = 0, fallbacks = 0, hits = 0;
    };

    void handleMidi (const juce::MidiMessage&) noexcept;
    void processChunk (juce::AudioBuffer<float>&, int offset, int n) noexcept;
    void setCarrierNotes (const vocoder::NoteList&, int count, double releaseSeconds) noexcept;
    void resetState() noexcept;
    void publish() noexcept;

    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, (size_t) vocoder::numKnobs> values {};

    std::atomic<int> keyMask { 0 }, chordMask { 0 }, hearOnly { -1 }, carrierMask { 0 }, bandsShown { 16 };
    std::atomic<double> lowShown { 100.0 }, highShown { 8000.0 };
    std::atomic<bool> fallbackShown { false }, testRequested { false }, demoRequested { false };
    std::atomic<int> requestedPart { 0 }, requestedBands { 0 }, requestedCarrier { 0 };
    std::array<std::atomic<float>, vocoder::maxBands> voiceLevels, carrierLevels, resultLevels;
    std::atomic<float> summaryInput { -100.0f }, summaryResult { -100.0f }, summaryHiss { 0.0f }, summaryFallback { 0.0f };
    std::atomic<int> summaryHits { 0 };

    // Audio thread state.
    double rate = 44100.0;
    std::array<Bank, 2> banks;
    int current = 0;
    bool configured = false, fading = false;
    double fadeX = 0.0, fadeStep = 0.0, formantRatio = 1.0;
    juce::SmoothedValue<float> formantSmooth, carrierGain, voiceGain, dryGain, wetGain;

    std::array<Slot, vocoder::maxCarrierVoices> slots;
    vocoder::NoteList wanted {};
    int wantedCount = 0;
    bool wasKeys = false;
    double norm = 1.0;
    std::array<bool, 128> held {};
    int heldCount = 0;
    juce::Random random { 1234 };

    // The hiss detector: the voice above 4 kHz against below 1.5 kHz.
    Coeffs hissHigh, hissLow;
    State hissHighState[2], hissLowState[2];
    double hissHighEnergy = 0.0, hissLowEnergy = 0.0, hissAmount = 0.0;

    std::vector<float> testVoice;
    int testPosition = -1;
    bool demoActive = false;                         // the test voice plays as a guide example
    DemoPart demoPart = DemoPart::result;
    int demoBands = 0;
    DemoCarrier demoCarrier = DemoCarrier::asSet;

    std::array<SummarySlot, 8> summary;
    int summaryIndex = 0;
    int levelCount = 0;
    float levelIn = 0.0f, levelEffect = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInVocoder)
};
}
