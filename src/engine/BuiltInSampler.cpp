#include "BuiltInSampler.h"

#include "BuiltInFormat.h"
#include "SoundFontCore.h"

#include <cmath>

namespace daw
{
namespace sampler
{
    const char* getParameterId (Knob k)
    {
        switch (k)
        {
            case Knob::level:     return "level";
            case Knob::transpose: return "transpose";
            case Knob::fineTune:  return "fineTune";
            case Knob::bendRange: return "bendRange";
        }

        return "";
    }

    juce::String getKnobName (Knob k)
    {
        switch (k)
        {
            case Knob::level:     return "Level";
            case Knob::transpose: return "Transpose";
            case Knob::fineTune:  return "Fine tune";
            case Knob::bendRange: return "Bend range";
        }

        return "";
    }

    juce::String formatKnob (Knob k, float value)
    {
        auto signedInt = [] (int n) { return (n > 0 ? "+" : "") + juce::String (n); };

        switch (k)
        {
            case Knob::level:
            {
                const int tenths = juce::roundToInt (value * 10.0f);
                return (tenths > 0 ? "+" : "") + juce::String ((double) tenths / 10.0, 1) + " dB";
            }
            case Knob::transpose: return signedInt (juce::roundToInt (value)) + " st";
            case Knob::fineTune:  return signedInt (juce::roundToInt (value)) + " ct";
            case Knob::bendRange: return juce::String (juce::roundToInt (value)) + " st";
        }

        return {};
    }

    BuiltInSound getFallbackSound (int bank, int program)
    {
        if (bank == soundfonts::drumBank)
            return BuiltInSound::drums;

        switch (getFamily (program))
        {
            case 0:  return BuiltInSound::piano;       // Piano
            case 1:  return BuiltInSound::pluck;       // Chromatic Percussion
            case 2:  return BuiltInSound::organ;       // Organ
            case 3:  return BuiltInSound::pluck;       // Guitar
            case 4:  return BuiltInSound::bass;        // Bass
            case 5:  return BuiltInSound::pad;         // Strings
            case 6:  return BuiltInSound::pad;         // Ensemble
            case 7:  return BuiltInSound::lead;        // Brass
            case 8:  return BuiltInSound::lead;        // Reed
            case 9:  return BuiltInSound::lead;        // Pipe
            case 10: return BuiltInSound::lead;        // Synth Lead
            case 11: return BuiltInSound::pad;         // Synth Pad
            case 12: return BuiltInSound::pad;         // Synth Effects
            case 13: return BuiltInSound::pluck;       // Ethnic
            case 14: return BuiltInSound::percussion;  // Percussive
            default: return BuiltInSound::pad;         // Sound Effects
        }
    }

    PresetNumber getSampledPreset (BuiltInSound sound)
    {
        switch (sound)
        {
            case BuiltInSound::piano:      return { 0, 0 };
            case BuiltInSound::organ:      return { 0, 16 };
            case BuiltInSound::bass:       return { 0, 33 };
            case BuiltInSound::lead:       return { 0, 80 };
            case BuiltInSound::pluck:      return { 0, 24 };
            case BuiltInSound::pad:        return { 0, 48 };
            case BuiltInSound::drums:
            case BuiltInSound::percussion: return { soundfonts::drumBank, 0 };
        }

        return {};
    }
} // namespace sampler

using sampler::Knob;

namespace
{
    juce::AudioParameterFloatAttributes knobText (Knob k)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction ([k] (float value, int) { return sampler::formatKnob (k, value); });
    }

    constexpr int chunkSize = fx::LevelHistory::chunk;
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInSampler::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const sampler::Settings d;

    auto add = [&layout] (Knob k, juce::NormalisableRange<float> range, float value)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { sampler::getParameterId (k), 1 }, sampler::getKnobName (k), range, value, knobText (k)));
    };

    add (Knob::level, { sampler::minLevel, sampler::maxLevel, 0.1f }, d.levelDb);
    add (Knob::transpose, { -sampler::maxTranspose, sampler::maxTranspose, 1.0f }, d.transpose);
    add (Knob::fineTune, { -sampler::maxFineTune, sampler::maxFineTune, 1.0f }, d.fineTuneCents);
    add (Knob::bendRange, { sampler::minBendRange, sampler::maxBendRange, 1.0f }, d.bendRange);
    return layout;
}

BuiltInSampler::BuiltInSampler()
    : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItSampler", createLayout())
{
    for (int k = 0; k < sampler::numKnobs; ++k)
        knobValues[(size_t) k] = parameters.getRawParameterValue (sampler::getParameterId ((Knob) k));

    for (auto& slot : retired)
        slot.store (nullptr);

    soundingKey.fill (-1);
    levelGain.reset (44100.0, 0.02);
    levelGain.setCurrentAndTargetValue (1.0f);
}

BuiltInSampler::~BuiltInSampler()
{
    stopTimer();

    if (auto* p = pending.exchange (nullptr))
        soundfonts::Font::closeCopy (p);

    if (active != nullptr)
        soundfonts::Font::closeCopy (active);

    collectRetired();
}

juce::PluginDescription BuiltInSampler::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "Sampler";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44736d70;   // "Dsmp"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

void BuiltInSampler::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

const char* BuiltInSampler::getParameterId (sampler::Knob k)
{
    return sampler::getParameterId (k);
}

juce::RangedAudioParameter* BuiltInSampler::getKnobParameter (Knob k) const
{
    return parameters.getParameter (sampler::getParameterId (k));
}

sampler::Settings BuiltInSampler::getSettings() const
{
    auto value = [this] (Knob k) { return knobValues[(size_t) k]->load(); };
    sampler::Settings s;
    s.levelDb = value (Knob::level);
    s.transpose = value (Knob::transpose);
    s.fineTuneCents = value (Knob::fineTune);
    s.bendRange = value (Knob::bendRange);
    return s;
}

void BuiltInSampler::setSettings (const sampler::Settings& s)
{
    setKnob (Knob::level, s.levelDb);
    setKnob (Knob::transpose, s.transpose);
    setKnob (Knob::fineTune, s.fineTuneCents);
    setKnob (Knob::bendRange, s.bendRange);
}

void BuiltInSampler::setKnob (Knob k, float value)
{
    if (auto* p = getKnobParameter (k))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

//==============================================================================
BuiltInSampler::Status BuiltInSampler::getStatus() const
{
    if (reference.isEmpty())
        return Status::noFont;

    if (font == nullptr)
        return Status::missing;

    switch (font->getState())
    {
        case soundfonts::Font::State::loading: return Status::loading;
        case soundfonts::Font::State::failed:  return Status::failed;
        case soundfonts::Font::State::ready:   break;
    }

    if (copyProblem.isNotEmpty())
        return Status::failed;

    return font->getPresetIndex (bank.load(), program.load()) < 0 ? Status::noPreset : Status::ready;
}

juce::String BuiltInSampler::getPresetName() const
{
    if (font == nullptr || font->getState() != soundfonts::Font::State::ready)
        return {};

    for (auto& preset : font->getPresets())
        if (preset.bank == bank.load() && preset.program == program.load())
            return preset.name;

    return {};
}

juce::String BuiltInSampler::getDisplayName() const
{
    juce::String suffix;

    switch (getStatus())
    {
        case Status::noFont:   return identifier;
        case Status::missing:  suffix = " (missing)"; break;
        case Status::loading:  suffix = " (loading)"; break;
        case Status::failed:   suffix = " (unreadable)"; break;
        case Status::noPreset: return juce::String (identifier) + " - no such sound";
        case Status::ready:    break;
    }

    if (suffix.isNotEmpty())
        return identifier + suffix;

    const auto name = getPresetName();
    return name.isEmpty() ? juce::String (identifier) : juce::String (identifier) + " - " + name;
}

const std::vector<soundfonts::Zone>& BuiltInSampler::getZones() const
{
    if (font == nullptr || font->getState() != soundfonts::Font::State::ready)
    {
        zoneCache.clear();
        zoneCacheFont = nullptr;
        return zoneCache;
    }

    if (zoneCacheFont != font.get() || zoneCacheBank != bank.load() || zoneCacheProgram != program.load())
    {
        zoneCache = font->getZones (bank.load(), program.load());
        zoneCacheFont = font.get();
        zoneCacheBank = bank.load();
        zoneCacheProgram = program.load();
    }

    return zoneCache;
}

juce::String BuiltInSampler::getFailure() const
{
    if (font != nullptr && font->getState() == soundfonts::Font::State::failed)
        return font->getProblem();

    return copyProblem;
}

void BuiltInSampler::dropCopies()
{
    if (auto* p = pending.exchange (nullptr))
        soundfonts::Font::closeCopy (p);

    dropRequested.store (true);
}

void BuiltInSampler::setFont (const soundfonts::Reference& ref)
{
    reference = ref;
    zoneCacheFont = nullptr;
    dropCopies();
    font.reset();
    copyBuilt = false;
    copyProblem = {};
    resolved = ref.find();

    if (resolved != juce::File())
        font = soundfonts::Library::get().acquire (resolved);

    writeState();

    auto* manager = juce::MessageManager::getInstanceWithoutCreating();

    if (! isTimerRunning() && manager != nullptr && manager->isThisTheMessageThread())
        startTimer (200);

    service();
    sendChangeMessage();
}

void BuiltInSampler::setPreset (int newBank, int newProgram)
{
    newBank = juce::jlimit (0, soundfonts::drumBank, newBank);
    newProgram = juce::jlimit (0, 127, newProgram);

    if (newBank == bank.load() && newProgram == program.load())
        return;

    bank.store (newBank);
    program.store (newProgram);
    fallbackSound.store (sampler::getFallbackSound (newBank, newProgram));
    writeState();
    service();
    sendChangeMessage();
}

void BuiltInSampler::publishCopy()
{
    copyBuilt = true;
    auto* copy = font->makeCopy (rate.load(), sampler::maxVoices);

    if (copy == nullptr)
    {
        copyProblem = "There isn't enough memory to play this SoundFont.";
        return;
    }

    if (auto* old = pending.exchange (copy))
        soundfonts::Font::closeCopy (old);
}

void BuiltInSampler::service()
{
    const std::lock_guard<std::mutex> lock (serviceMutex);
    collectRetired();

    if (font != nullptr && ! copyBuilt && font->getState() == soundfonts::Font::State::ready)
        publishCopy();

    if (const int status = (int) getStatus(); status != lastStatus)
    {
        lastStatus = status;
        sendChangeMessage();
    }
}

void BuiltInSampler::timerCallback()
{
    service();
}

void BuiltInSampler::collectRetired()
{
    for (auto& slot : retired)
        if (auto* p = slot.exchange (nullptr))
            soundfonts::Font::closeCopy (p);
}

void BuiltInSampler::writeState()
{
    auto& s = parameters.state;
    auto put = [&s] (const char* name, const juce::var& value, bool isDefault)
    {
        if (isDefault)
            s.removeProperty (name, nullptr);
        else
            s.setProperty (name, value, nullptr);
    };

    put ("font", reference.fileName, reference.fileName.isEmpty());
    put ("size", juce::String (reference.size), reference.size == 0);
    put ("link", reference.linkedPath, reference.linkedPath.isEmpty());
    put ("bank", bank.load(), bank.load() == 0);
    put ("program", program.load(), program.load() == 0);
}

void BuiltInSampler::readState()
{
    const auto& s = parameters.state;
    soundfonts::Reference ref;
    ref.fileName = s.getProperty ("font").toString();
    ref.size = s.getProperty ("size").toString().getLargeIntValue();
    ref.linkedPath = s.getProperty ("link").toString();
    bank.store (juce::jlimit (0, soundfonts::drumBank, (int) s.getProperty ("bank", 0)));
    program.store (juce::jlimit (0, 127, (int) s.getProperty ("program", 0)));
    fallbackSound.store (sampler::getFallbackSound (bank.load(), program.load()));
    setFont (ref);
}

void BuiltInSampler::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInSampler::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
        readState();
    }
}

juce::AudioProcessorEditor* BuiltInSampler::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

//==============================================================================
bool BuiltInSampler::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet().isDisabled();
}

void BuiltInSampler::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    const double r = sampleRate > 0.0 ? sampleRate : 44100.0;
    rate.store (r);
    clap.prepare (r);
    fallback.setCurrentPlaybackSampleRate (r);
    levelGain.reset (r, 0.02);
    levelGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getSettings().levelDb));

    scratchFrames = juce::jmax (512, maximumBlockSize);
    scratch.assign ((size_t) scratchFrames * 2, 0.0f);
    noMidi.ensureSize (64);

    if (active != nullptr)
    {
        tsf_set_output (active, TSF_STEREO_INTERLEAVED, juce::roundToInt (r), 0.0f);
        appliedBank = -1;   // everything is applied again at the next block
    }

    if (font != nullptr && ! copyBuilt)
        service();
}

void BuiltInSampler::reset()
{
    releaseAllSampled();
    fallback.allNotesOff (0, false);
    testNoteLeft = -1;
}

//==============================================================================
bool BuiltInSampler::tryRetire (::tsf* copy) noexcept
{
    for (auto& slot : retired)
    {
        ::tsf* expected = nullptr;

        if (slot.compare_exchange_strong (expected, copy))
            return true;
    }

    return false;
}

void BuiltInSampler::adoptCopies()
{
    const bool drop = dropRequested.exchange (false);

    if (active != nullptr && (drop || pending.load (std::memory_order_acquire) != nullptr))
    {
        if (tryRetire (active))
        {
            active = nullptr;
            soundingKey.fill (-1);
            presetFound = false;
        }
        else if (drop)
            dropRequested.store (true);   // no free retire slot: keep playing it, and try again at the next block
    }

    if (active == nullptr)
    {
        if (auto* next = pending.exchange (nullptr))
        {
            active = next;
            tsf_set_output (active, TSF_STEREO_INTERLEAVED, juce::roundToInt (rate.load()), 0.0f);
            appliedBank = -1;   // choose the preset, tuning and bend range on this copy
            appliedCents = appliedBend = -1000000;
        }
    }
}

void BuiltInSampler::applySettings (bool force)
{
    if (active == nullptr)
    {
        presetFound = false;
        return;
    }

    const int b = bank.load(), p = program.load();

    if (force || b != appliedBank || p != appliedProgram)
    {
        presetFound = tsf_channel_set_bank_preset (active, 0, b, p) != 0;
        appliedBank = b;
        appliedProgram = p;
    }

    const auto s = getSettings();
    const int cents = juce::roundToInt (s.fineTuneCents), bend100 = juce::roundToInt (s.bendRange * 100.0f);

    if (force || cents != appliedCents)
    {
        tsf_channel_set_tuning (active, 0, (float) cents / 100.0f);
        appliedCents = cents;
    }

    if (force || bend100 != appliedBend)
    {
        tsf_channel_set_pitchrange (active, 0, (float) bend100 / 100.0f);
        appliedBend = bend100;
    }

    appliedTranspose = juce::roundToInt (s.transpose);
}

void BuiltInSampler::releaseAllSampled()
{
    if (active != nullptr)
        tsf_channel_note_off_all (active, 0);

    soundingKey.fill (-1);
}

bool BuiltInSampler::fallbackHasVoices() const
{
    for (int i = 0; i < fallback.getNumVoices(); ++i)
        if (fallback.getVoice (i)->isVoiceActive())
            return true;

    return false;
}

void BuiltInSampler::renderCopy (juce::AudioBuffer<float>& buffer, int start, int count)
{
    const int channels = buffer.getNumChannels();

    while (count > 0)
    {
        const int n = juce::jmin (count, scratchFrames);
        tsf_render_float (active, scratch.data(), n, 0);

        auto* left = buffer.getWritePointer (0, start);
        auto* right = channels > 1 ? buffer.getWritePointer (1, start) : nullptr;

        for (int i = 0; i < n; ++i)
        {
            const float l = scratch[(size_t) i * 2], r = scratch[(size_t) i * 2 + 1];

            if (right != nullptr)
            {
                left[i] += l;
                right[i] += r;
            }
            else
                left[i] += 0.5f * (l + r);
        }

        start += n;
        count -= n;
    }
}

void BuiltInSampler::handleMessage (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber(), key = note + appliedTranspose;

        if (soundingKey[(size_t) note] >= 0)   // the same key again: the first one is let go
            tsf_channel_note_off (active, 0, soundingKey[(size_t) note]);

        soundingKey[(size_t) note] = -1;

        if (key < 0 || key > 127)
            return;

        // Plus a half, so that a velocity of 100 reaches TinySoundFont as 100 and not as 99.99999.
        const float velocity = juce::jmin (1.0f, ((float) m.getVelocity() + 0.5f) / 127.0f);
        tsf_channel_note_on (active, 0, key, velocity);
        soundingKey[(size_t) note] = key;
        lastNote.store (key, std::memory_order_relaxed);
        lastVelocity.store ((int) m.getVelocity(), std::memory_order_relaxed);
    }
    else if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();

        if (soundingKey[(size_t) note] >= 0)
        {
            tsf_channel_note_off (active, 0, soundingKey[(size_t) note]);
            soundingKey[(size_t) note] = -1;
        }
    }
    else if (m.isAllNotesOff())
    {
        tsf_channel_note_off_all (active, 0);
        soundingKey.fill (-1);
    }
    else if (m.isAllSoundOff())
    {
        tsf_channel_sounds_off_all (active, 0);
        soundingKey.fill (-1);
    }
    else if (m.isPitchWheel())
    {
        tsf_channel_set_pitchwheel (active, 0, m.getPitchWheelValue());
    }
    else if (m.isController())
    {
        const int number = m.getControllerNumber();

        // Volume, pan, expression, the pedal, Reset All Controllers and the RPN messages that set the bend range.
        if (number == 7 || number == 10 || number == 11 || number == 64 || number == 121 || number == 6 || number == 38
            || number == 98 || number == 99 || number == 100 || number == 101)
        {
            tsf_channel_midi_control (active, 0, number, m.getControllerValue());

            if (number == 121)   // that puts the bend range and tuning back to General MIDI's: the knobs say otherwise
                applySettings (true);
        }
    }
}

void BuiltInSampler::renderSampler (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const sampler::Settings&)
{
    const int numSamples = buffer.getNumSamples();
    int position = 0;

    for (const auto metadata : midi)
    {
        const int at = juce::jlimit (position, numSamples, metadata.samplePosition);

        if (at > position)
            renderCopy (buffer, position, at - position);

        position = at;
        handleMessage (metadata.getMessage());
    }

    if (position < numSamples)
        renderCopy (buffer, position, numSamples - position);
}

void BuiltInSampler::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    adoptCopies();
    applySettings (false);
    const auto settings = getSettings();
    levelGain.setTargetValue (juce::Decibels::decibelsToGain (settings.levelDb));

    // The window's test note: on now, off after 0.8 s.
    int requested = requestedKey.exchange (-1);

    if (clap.takeRequest())
        requested = bank.load() == soundfonts::drumBank ? sampler::testDrumNote : sampler::testNote;

    if (requested >= 0)
    {
        if (testNoteLeft >= 0)   // a key still held by the last request ends now
            midi.addEvent (juce::MidiMessage::noteOff (1, testKey), 0);

        testKey = requested;
        midi.addEvent (juce::MidiMessage::noteOn (1, testKey, (juce::uint8) 100), 0);
        testNoteLeft = juce::jmax (1, (int) (0.8 * rate.load()));
    }

    if (testNoteLeft >= 0)
    {
        if (testNoteLeft < numSamples)
            midi.addEvent (juce::MidiMessage::noteOff (1, testKey), testNoteLeft);

        testNoteLeft = testNoteLeft < numSamples ? -1 : testNoteLeft - numSamples;
    }

    // Which engine plays: the font's copy, or the fallback until there is one.
    const bool wantSampler = active != nullptr && presetFound;

    if (wantSampler != usingSampler)
    {
        if (usingSampler)
            releaseAllSampled();
        else
            fallback.allNotesOff (0, true);

        usingSampler = wantSampler;
    }

    playingFallback.store (! usingSampler, std::memory_order_relaxed);

    if (usingSampler)
    {
        renderSampler (buffer, midi, settings);

        if (fallbackHasVoices())   // a note that began on the fallback rings out
            fallback.renderNextBlock (buffer, noMidi, 0, numSamples);
    }
    else
    {
        fallback.renderNextBlock (buffer, midi, 0, numSamples);
    }

    // Level, and the level history for the window.
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = levelGain.getNextValue();
        const float l = left[i], r = right != nullptr ? right[i] : l;
        chunkBefore = juce::jmax (chunkBefore, std::abs (l), std::abs (r));
        left[i] = l * gain;

        if (right != nullptr)
            right[i] = r * gain;

        chunkAfter = juce::jmax (chunkAfter, std::abs (left[i]), right != nullptr ? std::abs (right[i]) : 0.0f);

        if (++chunkFill == chunkSize)
        {
            history.push ({ juce::Decibels::gainToDecibels (chunkBefore, -100.0f), juce::Decibels::gainToDecibels (chunkAfter, -100.0f) });
            chunkBefore = chunkAfter = 0.0f;
            chunkFill = 0;
        }
    }

    activeVoices.store (active != nullptr ? tsf_active_voice_count (active) : 0, std::memory_order_relaxed);
}
} // namespace daw
