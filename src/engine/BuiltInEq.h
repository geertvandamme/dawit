#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace daw
{
namespace eq
{
    /** What a band does; the order of the type menu. */
    enum class BandType { lowCut, lowShelf, bell, notch, highShelf, highCut };
    constexpr int numTypes = 6;

    /** "Low cut", "Bell", ... */
    juce::String getTypeName (BandType);

    /** The other name people use for it: "high-pass" for a low cut, "peak" for a bell. */
    juce::String getTypeAlias (BandType);

    /** Shelves and bells raise or lower their range; cuts and notches always remove it. */
    inline bool hasGain (BandType t) { return t == BandType::lowShelf || t == BandType::bell || t == BandType::highShelf; }
    inline bool isCut (BandType t) { return t == BandType::lowCut || t == BandType::highCut; }

    struct Band
    {
        bool on = true;
        BandType type = BandType::bell;
        float freq = 1000.0f;   // Hz: the corner of a cut or shelf, the centre of a bell or notch
        float gainDb = 0.0f;
        float q = 0.71f;
        int slope = 12;         // dB per octave, for cuts: 12 or 24
    };

    constexpr int numBands = 6;
    constexpr float minFreq = 20.0f, maxFreq = 20000.0f, maxGainDb = 18.0f, minQ = 0.1f, maxQ = 18.0f;

    /** Q of the smoothest cut and shelf, without a bump: 1/sqrt(2) (Butterworth). */
    constexpr float flatQ = 0.7071f;

    /** The six bands of a new EQ, in the classic order: low cut, low shelf, two bells, high shelf, high cut. */
    Band getDefaultBand (int index);

    /** One second-order filter (biquad), normalised so a0 = 1. */
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

        /** How much the filter changes the level at `freq`, as a gain (1 = unchanged). */
        double getMagnitude (double freq, double sampleRate) const;
    };

    /** A band as one or two biquads in a row: a 24 dB/oct cut is two 12 dB/oct ones. */
    struct Stages
    {
        std::array<Biquad, 2> filters;
        int num = 1;

        double getMagnitude (double freq, double sampleRate) const;
    };

    /** The filters of a band, from the formulas of the Audio EQ Cookbook (Robert Bristow-Johnson). */
    Stages makeStages (const Band&, double sampleRate);

    /** The filter used to solo a band: only the frequencies it works on. A low cut and low shelf
        let through what's below the frequency, a high cut and high shelf what's above, a bell
        and notch the range around it. */
    Stages makeSoloStages (const Band&, double sampleRate);

    /** The band's effect on the level at `freq`, in dB (0 when it's off). */
    double getResponseDb (const Band&, double freq, double sampleRate);

    /** The width of a bell or notch with this Q, in octaves: 2/ln 2 * asinh (1 / 2Q). */
    double getBandwidthOctaves (double q);

    /** A range of the spectrum as engineers name it, with what it sounds like. */
    struct Region
    {
        float from, to;
        juce::String name, sound;
    };

    /** Sub-bass, bass, low mids, mids, presence, brilliance and air: 20 Hz to 20 kHz without gaps. */
    const std::vector<Region>& getRegions();

    /** A sound people describe in words ("muddy", "harsh"), where it usually comes from, and what to try. */
    struct Problem
    {
        juce::String name;
        float from, to;
        bool tooMuch;   // too much of this range; false: too little
        juce::String fix;
    };

    /** Boomy, thin, muddy, boxy, nasal, harsh, sibilant and dull: typical ranges, low to high (by where they start). */
    const std::vector<Problem>& getProblems();

    /** A starting point for an instrument, with why each band is set the way it is. */
    struct Preset
    {
        juce::String name, why;
        std::array<Band, numBands> bands;
        std::array<juce::String, numBands> notes;   // empty for bands it leaves off
    };

    /** Flat first, then the instruments. */
    const std::vector<Preset>& getPresets();

    /** A range of an instrument worth knowing: where a problem usually hides (cut it if it
        bothers you), or what a boost brings out. */
    struct InstrumentZone
    {
        float from, to;
        juce::String name, text;
        bool problem;
    };

    /** Where a common instrument lives in the spectrum, for the EQ's instrument overview. */
    struct Instrument
    {
        juce::String name, summary;
        bool pitched;                 // false for drums and cymbals: their "notes" are the body of the drum
        float notesFrom, notesTo;     // its fundamentals: the lowest and highest notes it plays
        float overtonesTo;            // how high its overtones, attack and noises reach
        float lowCutHz;               // a low cut up to here takes nothing it needs; 0 for instruments whose lows are the point
        std::vector<InstrumentZone> zones;   // low to high
    };

    /** Drums, bass, guitars, keys, strings, brass and voices, typical values. */
    const std::vector<Instrument>& getInstruments();
} // namespace eq

//==============================================================================
/**
    DawIt EQ, the built-in equaliser: six bands, each a low cut, low shelf, bell, notch, high shelf
    or high cut, with frequency, gain and Q. It runs in the track's chain like any effect plugin
    (see BuiltInFormat.h), so adding, bypassing, saving and bouncing work as for plugins.

    For learning it also offers, while the track plays:
      - band solo: hear only the frequencies one band works on (soloBand);
      - match level: keep the output as loud as the input, to compare without "louder sounds better";
      - the signal before and after the EQ, for the editor's spectrum (readAnalyzer);
      - the newest signal after the EQ, for a small spectrum elsewhere, such as the device bar (readPreview).
*/
class BuiltInEq final : public juce::AudioPluginInstance
{
public:
    BuiltInEq();

    static constexpr const char* identifier = "DawIt EQ";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInEq&)> createEditorFunction;

    /** The band's current settings, from its parameters. */
    eq::Band getBand (int index) const;

    /** Changes the band's parameters, as a user would. Message thread. */
    void setBand (int index, const eq::Band&);

    /** The band's parameter for "on", "type", "freq", "gain", "q" or "slope". */
    juce::RangedAudioParameter* getBandParameter (int index, const juce::String& what) const;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    void applyPreset (const eq::Preset&);

    /** Output gain in dB, and whether match level is on. */
    float getOutputDb() const { return outputDb->load(); }
    bool isMatchingLevel() const { return matchLevel->load() >= 0.5f; }
    void setMatchingLevel (bool);

    /** The band being listened to on its own, or -1. Not saved. */
    std::atomic<int> soloBand { -1 };

    /** The gain match level applies right now, in dB. */
    std::atomic<float> matchGainDb { 0.0f };

    /** Takes up to `max` of the newest mono samples from before and after the EQ, oldest first.
        One reader at a time (the editor). Returns how many were read. */
    int readAnalyzer (float* before, float* after, int max);

    /** Copies the newest `count` mono samples after the EQ (at most previewSize), oldest first.
        Unlike readAnalyzer it takes nothing away, so it works next to the editor; it never blocks.
        Returns how many samples were written since the start: unchanged means nothing played. */
    juce::int64 readPreview (float* dest, int count) const;
    static constexpr int previewSize = 2048;

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override { return 0.0; }
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

private:
    struct BandParameters
    {
        std::atomic<float>* on = nullptr, * type = nullptr, * freq = nullptr, * gain = nullptr, * q = nullptr, * slope = nullptr;
    };

    /** A band as it plays: settings gliding towards the parameters, its filters and their memory. */
    struct BandState
    {
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freq, q;
        juce::SmoothedValue<float> gainDb, amount;   // amount: 0 off .. 1 on, so switching doesn't click
        eq::BandType type = eq::BandType::bell;
        int slope = 12;
        eq::Stages stages;
        double memory[2][2][2] {};   // [channel][stage][z1, z2]
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateBand (int index, int numSamples, bool snap);
    eq::Band readTarget (int index) const;

    juce::AudioProcessorValueTreeState parameters;
    std::array<BandParameters, eq::numBands> bandParameters;
    std::atomic<float>* outputDb = nullptr, * matchLevel = nullptr;

    std::array<BandState, eq::numBands> bands;
    eq::Stages soloStages;
    int soloShown = -1;
    double soloMemory[2][2][2] {};

    double rate = 44100.0;
    double powerIn = 0.0, powerOut = 0.0;
    juce::SmoothedValue<float> outputGain, matchGain;

    // Before / after samples for the editor's spectrum.
    static constexpr int analyzerSize = 8192;
    juce::AbstractFifo analyzerFifo { analyzerSize };
    juce::AudioBuffer<float> analyzerBuffer { 2, analyzerSize };

    // The preview: a ring the audio thread keeps overwriting, read by the message thread.
    std::array<std::atomic<float>, previewSize> previewRing {};
    std::atomic<juce::int64> previewWritten { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInEq)
};
} // namespace daw
