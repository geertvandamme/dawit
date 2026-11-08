#pragma once

#include "BuiltInSynth.h"
#include "EffectTools.h"
#include "SoundFonts.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>
#include <mutex>

namespace daw
{
namespace sampler
{
    /** The four knobs, in the order of the window. */
    enum class Knob { level, transpose, fineTune, bendRange };
    constexpr int numKnobs = 4;

    struct Settings
    {
        float levelDb = 0.0f;         // -24 to +6: presets differ in loudness, so this evens them out
        float transpose = 0.0f;       // -24 to +24 semitones, applied to the note before a zone is chosen
        float fineTuneCents = 0.0f;   // -100 to +100 cents
        float bendRange = 2.0f;       // 1 to 24 semitones each way; General MIDI starts at 2
    };

    constexpr float minLevel = -24.0f, maxLevel = 6.0f, maxTranspose = 24.0f, maxFineTune = 100.0f, minBendRange = 1.0f, maxBendRange = 24.0f;

    /** At most this many voices sound at once. One note can start several (a zone for each layer and side of a stereo pair). */
    constexpr int maxVoices = 64;

    /** The notes the window's test note plays: middle C, or the kick for a drum kit (bank 128). */
    constexpr int testNote = 60, testDrumNote = 36;

    const char* getParameterId (Knob);
    juce::String getKnobName (Knob);

    /** "+3.0 dB", "+5 st", "-20 ct", "2 st": as the knobs show them. */
    juce::String formatKnob (Knob, float value);

    //==============================================================================
    /** General MIDI's 16 families of 8 programs: 0 is Piano (programs 1-8), 15 Sound Effects (121-128). */
    constexpr int numFamilies = 16;
    constexpr int getFamily (int program) noexcept { return program < 0 ? 0 : (program > 127 ? 15 : program / 8); }

    /** The DawIt Synth sound that plays while the SoundFont isn't there or is still loading: the one nearest the
        preset's General MIDI family (Piano: piano; Chromatic Percussion, Guitar, Ethnic: pluck; Organ: organ;
        Bass: bass; Strings, Ensemble, Synth Pad, Synth Effects, Sound Effects: pad; Brass, Reed, Pipe, Synth Lead:
        lead; Percussive: percussion), and drums for a drum kit (bank 128). */
    BuiltInSound getFallbackSound (int bank, int program);

    /** A sampled preset: bank, and program 0 to 127. */
    struct PresetNumber
    {
        int bank = 0, program = 0;
    };

    /** The preset "Use a sampled sound" picks for a DawIt Synth sound: piano 1, organ 17, bass 34, lead 81, pluck 25,
        pad 49 (shown 1 to 128), and the standard kit (bank 128, program 1) for drums and percussion. */
    PresetNumber getSampledPreset (BuiltInSound);
} // namespace sampler

//==============================================================================
/**
    DawIt Sampler: an instrument that plays recordings from a SoundFont (.sf2 or .sf3), with TinySoundFont. It
    answers to the track's MIDI notes, pitch bend, volume, pan, expression and sustain pedal. Four knobs: Level,
    Transpose, Fine tune and Bend range.

    What it plays is a reference to a file (see soundfonts::Reference), a bank and a program. The font is loaded
    once, on a background thread, by soundfonts::Library and shared with every other sampler on the file; each
    sampler plays its own copy (own voices, shared recordings). Until the font is ready, or when its file is missing,
    a DawIt Synth plays instead (the "fallback", picked from the preset's General MIDI family), so a track is
    never silent for lack of a file.

    Threading: all TinySoundFont calls for a sampler's copy happen on the audio thread, except while the copy is
    built. The message thread builds a copy and hands it over through one atomic slot; the audio thread swaps it in at the
    start of a block and puts the old one in a retire slot, and the message thread closes it (`collectRetired`,
    from a timer). The audio thread never allocates or frees. Bank, program and the knobs are read at the start of each
    block, and MIDI is applied at its sample position.
*/
class BuiltInSampler final : public juce::AudioPluginInstance,
                             public juce::ChangeBroadcaster,
                             private juce::Timer
{
public:
    BuiltInSampler();
    ~BuiltInSampler() override;

    static constexpr const char* identifier = "DawIt Sampler";
    static juce::PluginDescription getDescription();

    /** Set by the app to its editor; without one, hosts show a generic editor. */
    static inline std::function<juce::AudioProcessorEditor* (BuiltInSampler&)> createEditorFunction;

    static const char* getParameterId (sampler::Knob);
    juce::RangedAudioParameter* getKnobParameter (sampler::Knob) const;
    sampler::Settings getSettings() const;

    /** Changes the knobs, as a user would. Message thread. */
    void setSettings (const sampler::Settings&);
    void setKnob (sampler::Knob, float value);

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    //==============================================================================
    /** What the sampler is doing about its sound. */
    enum class Status
    {
        noFont,    // no SoundFont chosen yet: the fallback plays
        missing,   // the file isn't on this computer: the fallback plays
        loading,   // it's being loaded: the fallback plays
        failed,    // the file couldn't be read: the fallback plays
        noPreset,  // the font has no sound with this bank and program: the fallback plays
        ready      // the sampled sound plays
    };

    /** The SoundFont this sampler plays, as the project remembers it. */
    soundfonts::Reference getFontReference() const { return reference; }
    int getBank() const noexcept { return bank.load(); }
    int getProgram() const noexcept { return program.load(); }

    /** Chooses a SoundFont and starts loading it (a share of the loaded font when another sampler has it already).
        An empty reference clears it. Message thread. */
    void setFont (const soundfonts::Reference&);

    /** Chooses the sound: a bank (128 for drum kits) and a program, 0 to 127 (shown 1 to 128). Message thread. */
    void setPreset (int bank, int program);

    Status getStatus() const;

    /** The loaded font once it's ready, or null: for the window's lists and pictures. Message thread. */
    std::shared_ptr<soundfonts::Font> getFont() const { return font; }

    /** Why the file couldn't be read, when the status is `failed`. */
    juce::String getFailure() const;

    /** The DawIt Synth sound that plays while the font isn't available. */
    BuiltInSound getFallbackSound() const { return fallbackSound.load(); }

    /** True while the fallback is what plays (no font, loading, missing or no such preset). Updated by the audio thread. */
    bool isPlayingFallback() const noexcept { return playingFallback.load(); }

    /** Takes a ready font's copy and hands it to the audio thread, and closes copies the audio thread has let go. The
        timer calls it; tests call it by hand because they have no message loop. A bounce calls it from its render
        thread once the font has loaded, so it holds a lock: the timer and the bounce never run it at once. */
    void service();

    /** Closes the copies the audio thread has retired. Message thread. */
    void collectRetired();

    /** "Test note" in the window plays the test note for 0.8 seconds. */
    fx::Clap clap;

    /** Plays one key (0 to 127) at velocity 100 for 0.8 seconds, as the window's keys do. Any thread. */
    void playKey (int key) noexcept { requestedKey.store (juce::jlimit (0, 127, key)); }

    /** "DawIt Sampler - Acoustic Grand Piano" for the device bar, inspector and track header; just "DawIt Sampler" with no
        font chosen, and "DawIt Sampler (missing)", "(loading)" or "(unreadable)" while the sound isn't there. Message thread. */
    juce::String getDisplayName() const;

    /** The name of the chosen preset ("Acoustic Grand Piano") once the font is ready, otherwise empty. Message thread. */
    juce::String getPresetName() const;

    /** The zones of the chosen preset, for pictures. Cached until the font or the preset changes, so painting every frame is
        cheap; empty while the font isn't ready. Message thread. */
    const std::vector<soundfonts::Zone>& getZones() const;

    /** The level of what came before the Level knob (as the input) and after it (as the effect). */
    fx::LevelHistory history;

    /** The last note played (after Transpose) and its velocity, for the window's pictures: -1 until a note plays. */
    std::atomic<int> lastNote { -1 }, lastVelocity { 0 };

    /** How many voices sound now, as of the last block. */
    std::atomic<int> activeVoices { 0 };

    //==============================================================================
    const juce::String getName() const override { return identifier; }
    void fillInPluginDescription (juce::PluginDescription&) const override;

    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void reset() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    double getTailLengthSeconds() const override { return 2.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

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

    void timerCallback() override;
    void writeState();
    void readState();
    void dropCopies();
    void publishCopy();

    // Audio thread.
    void adoptCopies();
    void applySettings (bool force);
    void renderSampler (juce::AudioBuffer<float>&, const juce::MidiBuffer&, const sampler::Settings&);
    void renderCopy (juce::AudioBuffer<float>&, int start, int count);
    void handleMessage (const juce::MidiMessage&);
    void releaseAllSampled();
    bool tryRetire (::tsf*) noexcept;
    bool fallbackHasVoices() const;

    std::mutex serviceMutex;
    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*, sampler::numKnobs> knobValues {};

    // The choice: written by the message thread, read at the start of each block.
    std::atomic<int> bank { 0 }, program { 0 };
    soundfonts::Reference reference;
    std::shared_ptr<soundfonts::Font> font;
    bool copyBuilt = false;
    juce::String copyProblem;
    int lastStatus = -1;
    juce::File resolved;

    // The handover (see the class comment).
    std::atomic<::tsf*> pending { nullptr };
    std::array<std::atomic<::tsf*>, 8> retired;
    std::atomic<bool> dropRequested { false };
    std::atomic<int> requestedKey { -1 };
    mutable std::vector<soundfonts::Zone> zoneCache;
    mutable const soundfonts::Font* zoneCacheFont = nullptr;
    mutable int zoneCacheBank = -1, zoneCacheProgram = -1;
    std::atomic<double> rate { 44100.0 };

    // The audio thread's.
    ::tsf* active = nullptr;
    bool presetFound = false, usingSampler = false;
    int appliedBank = -1, appliedProgram = -1, appliedCents = -1000000, appliedBend = -1000000, appliedTranspose = 0;
    std::array<int, 128> soundingKey;   // the key each held note started with, after Transpose; -1 for none
    std::vector<float> scratch;         // interleaved stereo from the copy
    int scratchFrames = 0;
    std::atomic<BuiltInSound> fallbackSound { BuiltInSound::piano };
    BuiltInSynth fallback { fallbackSound };
    juce::MidiBuffer noMidi;
    std::atomic<bool> playingFallback { true };
    juce::SmoothedValue<float> levelGain;
    int testNoteLeft = -1;
    float chunkBefore = 0.0f, chunkAfter = 0.0f;
    int chunkFill = 0;
    int testKey = sampler::testNote;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BuiltInSampler)
};
} // namespace daw
