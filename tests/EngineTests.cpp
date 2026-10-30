#include "engine/LoudnessMeter.h"
#include "engine/BuiltInStereo.h"
// Headless checks for the audio engine. Drives the audio callback by hand with a fake device.
// Usage: DawItTests [instrument] [effect] [--ollama <model>] [--demucs <model file>] [--whisper <model file>]
// Plugins are .vst3 or .component paths, or Audio Unit ids such as "AudioUnit:Effects/aufx,rvb2,appl".
//   --ollama runs one real generation per part type against a local Ollama server.
//   --demucs splits a made-up mix into stems with the real Demucs model (ggml-model-htdemucs-4s-f16.bin).
//   --voice <folder> sings "feel the night" with a real DiffSinger voice and prints how long it took.
//   --whisper finds the words in speech made by macOS's voice with a real Whisper model (ggml-small.en.bin).

#include "analysis/ChordFinder.h"
#include "analysis/HitFinder.h"
#include "analysis/LyricsFinder.h"
#include "analysis/OnsetFinder.h"
#include "analysis/StructureFinder.h"
#include "analysis/TempoFinder.h"
#include "engine/AudioToMidi.h"
#include "engine/DrumTriggerGuide.h"
#include "engine/HitsToMidi.h"
#include "engine/Bounce.h"
#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInDelay.h"
#include "engine/BuiltInEq.h"
#include "engine/BuiltInTune.h"
#include "engine/BuiltInReverb.h"
#include "engine/BuiltInSaturation.h"
#include "engine/BuiltInDistortion.h"
#include "engine/BuiltInGate.h"
#include "engine/StereoMeasure.h"
#include "engine/BuiltInSubtractive.h"
#include "engine/BuiltInAdditive.h"
#include "engine/BuiltInFm.h"
#include "engine/BuiltInDrums.h"
#include "engine/BuiltInPhysical.h"
#include "engine/BuiltInFormat.h"
#include "engine/OwnPluginCopies.h"
#include "DawItBuiltInList.h"
#include "engine/ChordEditing.h"
#include "engine/GainStaging.h"
#include "engine/Lyrics.h"
#include "engine/Engine.h"
#include "engine/MidiExport.h"
#include "engine/MidiGuide.h"
#include "engine/ComputerKeyboard.h"
#include "engine/PdfWriter.h"
#include "engine/Tuner.h"
#include "generate/TuningGuide.h"
#include "engine/ExampleSong.h"
#include "engine/ProjectFile.h"
#include "engine/Routing.h"
#include "engine/SignalFlow.h"
#include "engine/LatencyMeasure.h"
#include "engine/RecordingGuide.h"
#include "engine/RoutingGuide.h"
#include "engine/Stems.h"
#include "engine/DiffSinger.h"
#include "engine/Singer.h"
#include "engine/Voice.h"
#include "engine/VoiceInstall.h"
#include "engine/YamlLite.h"
#include "generate/SingMatch.h"
#include "generate/SingPairing.h"
#include "generate/SingScore.h"
#include "generate/SingingGuide.h"
#include "generate/Pronunciation.h"
#include "engine/Tempo.h"
#include "generate/ChordDiagrams.h"
#include "generate/ChatMusician.h"
#include "generate/ChordGuide.h"
#include "generate/ChordSheet.h"
#include "generate/Notation.h"
#include "generate/ReadingGuide.h"
#include "generate/ChordStyles.h"
#include "generate/LyricsAi.h"
#include "generate/LyricsGuide.h"
#include "generate/Rhyme.h"
#include "generate/PlayAlong.h"
#include "generate/SingAlong.h"
#include "generate/Harmony.h"
#include "generate/NoteRealizer.h"
#include "generate/OllamaClient.h"
#include "generate/PromptBuilder.h"
#include "generate/RhythmGuide.h"
#include "generate/StructureGuide.h"
#include "generate/SongTemplates.h"
#include "generate/StylesGuide.h"
#include "generate/Takes.h"
#include "generate/TheoryGuides.h"
#include "ui/PitchRows.h"
#include "ui/Theme.h"
#include "engine/GuideBooklet.h"

#include <complex>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>

namespace
{
constexpr double testRate = 48000.0;
constexpr int testBlock = 256;

struct FakeDevice final : public juce::AudioIODevice
{
    FakeDevice() : juce::AudioIODevice ("Fake", "Fake") {}

    juce::StringArray getOutputChannelNames() override { return { "L", "R" }; }
    int numInputs = 0, inputLatency = 0, outputLatency = 0;   // input channels, and the latencies the device reports

    juce::StringArray getInputChannelNames() override
    {
        juce::StringArray names;

        for (int i = 0; i < numInputs; ++i)
            names.add ("In " + juce::String (i + 1));

        return names;
    }
    juce::Array<double> getAvailableSampleRates() override { return { testRate }; }
    juce::Array<int> getAvailableBufferSizes() override { return { testBlock }; }
    int getDefaultBufferSize() override { return testBlock; }
    juce::String open (const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start (juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return testBlock; }
    double getCurrentSampleRate() override { return testRate; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return 3; }
    juce::BigInteger getActiveInputChannels() const override { juce::BigInteger bits; bits.setRange (0, numInputs, true); return bits; }
    int getOutputLatencyInSamples() override { return outputLatency; }
    int getInputLatencyInSamples() override { return inputLatency; }
};

int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << std::endl;
    failures += ok ? 0 : 1;
}

float rms (const juce::AudioBuffer<float>& b, int ch, int start = 0, int num = -1)
{
    if (num < 0)
        num = b.getNumSamples() - start;

    return b.getRMSLevel (ch, start, num);
}

juce::AudioBuffer<float> render (daw::Engine& engine, double seconds)
{
    const int numBlocks = (int) std::ceil (seconds * testRate / testBlock);
    juce::AudioBuffer<float> out (2, numBlocks * testBlock);

    for (int b = 0; b < numBlocks; ++b)
    {
        float* channels[] = { out.getWritePointer (0, b * testBlock), out.getWritePointer (1, b * testBlock) };
        engine.audioDeviceIOCallbackWithContext (nullptr, 0, channels, 2, testBlock, {});
    }

    return out;
}

juce::PluginDescription findPlugin (daw::Engine& engine, const juce::String& path)
{
    juce::OwnedArray<juce::PluginDescription> found;

    for (auto* format : engine.pluginFormats.getFormats())
        if (format->fileMightContainThisPluginType (path))
            format->findAllTypesForFile (found, path);

    return found.isEmpty() ? juce::PluginDescription() : *found.getFirst();
}
} // namespace

//==============================================================================
static void runMidiExportTests()
{
    using namespace daw::midiexport;
    using Scale = daw::MusicalKey::Scale;
    std::cout << "MIDI export" << std::endl;

    // Key signatures, including modes (which take their parent major's signature).
    auto sig = [] (int tonic, Scale scale) { return getKeySignature ({ tonic, scale }); };
    check (sig (9, Scale::minor) == std::pair (0, true) && sig (3, Scale::major) == std::pair (-3, false)
               && sig (6, Scale::minor) == std::pair (3, true) && sig (2, Scale::dorian) == std::pair (0, false)
               && sig (4, Scale::phrygian) == std::pair (0, false) && sig (7, Scale::mixolydian) == std::pair (0, false)
               && sig (5, Scale::lydian) == std::pair (0, false) && sig (11, Scale::locrian) == std::pair (0, false)
               && sig (1, Scale::major) == std::pair (-5, false),
           "key signatures for keys and modes");

    daw::Track track (daw::Track::Type::midi, "Keys", juce::Colours::blue);

    auto first = std::make_unique<daw::MidiClip>();
    first->name = "Intro";
    first->startBeat = 4.0;
    first->lengthBeats = 4.0;
    first->notes = { { 60, 0.0, 1.0, 1.0f }, { 60, 1.0, 1.0, 0.5f },   // re-triggered at beat 1
                     { 64, 3.0, 2.0, 0.8f },                           // rings past the end: shortened
                     { 67, 5.0, 1.0, 0.8f } };                         // starts after the end: dropped
    auto second = std::make_unique<daw::MidiClip>();
    second->startBeat = 12.0;
    second->lengthBeats = 2.0;
    second->notes = { { 48, 0.5, 1.0, 0.8f } };

    const auto& clip = *first;
    track.clips.push_back (std::move (first));
    track.clips.push_back (std::move (second));

    const SongInfo song { daw::TempoMap (126.0), { 9, Scale::minor } };
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DawIt test.mid");
    juce::String error;

    auto roundTrip = [&] (const juce::MidiFile& midi)
    {
        juce::MidiFile read;
        const bool written = write (midi, file, error);
        juce::FileInputStream in (file);
        const bool ok = written && in.openedOk() && read.readFrom (in);
        check (ok, "file writes and reads back " + error);
        return read;
    };

    auto noteEvents = [] (const juce::MidiMessageSequence& sequence)
    {
        juce::StringArray events;
        for (auto* e : sequence)
            if (e->message.isNoteOnOrOff())
                events.add (juce::String (e->message.isNoteOn() ? "on" : "off") + " " + juce::String (e->message.getNoteNumber())
                            + " @" + juce::String ((int) e->message.getTimeStamp()));
        return events.joinIntoString (", ");
    };

    const auto clipFile = roundTrip (createClipFile (clip, track.name, song));
    check (clipFile.getTimeFormat() == 960 && clipFile.getNumTracks() == 2, "type 1 file at 960 ticks per beat");

    juce::MidiMessageSequence tempos, keys;
    clipFile.findAllTempoEvents (tempos);
    clipFile.findAllKeySigEvents (keys);
    check (tempos.getNumEvents() == 1 && std::abs (60.0 / tempos.getEventPointer (0)->message.getTempoSecondsPerQuarterNote() - 126.0) < 0.01
               && keys.getNumEvents() == 1 && keys.getEventPointer (0)->message.isKeySignatureMajorKey() == false,
           "tempo 126 BPM and A minor key signature");

    const auto* notes = clipFile.getTrack (1);
    check (noteEvents (*notes) == "on 60 @0, off 60 @960, on 60 @960, off 60 @1920, on 64 @2880, off 64 @3840",
           "clip notes start at 0, are trimmed to the clip, and re-triggers are released first: " + noteEvents (*notes));
    check (notes->getEventPointer (0)->message.getTextFromTextMetaEvent() == "Intro"
               && notes->getEndTime() == 3840.0 && notes->getEventPointer (notes->getNumEvents() - 1)->message.isEndOfTrackMetaEvent(),
           "named after the clip and ends where the clip ends");

    const auto trackFile = roundTrip (createTrackFile (track, song));
    check (noteEvents (*trackFile.getTrack (1)).endsWith ("off 64 @7680, on 48 @12000, off 48 @12960")
               && noteEvents (*trackFile.getTrack (1)).startsWith ("on 60 @3840")
               && trackFile.getTrack (1)->getEventPointer (0)->message.getTextFromTextMetaEvent() == "Keys",
           "track export keeps song positions: " + noteEvents (*trackFile.getTrack (1)));

    track.clips[1]->muted = true;
    check (! noteEvents (*roundTrip (createTrackFile (track, song)).getTrack (1)).contains ("on 48 "), "track export leaves out muted clips");

    file.deleteFile();
}

//==============================================================================
static void runSoundGuideTests()
{
    using namespace daw::soundguide;
    std::cout << "What is sound?" << std::endl;

    for (const auto topic : allTopics)
        check (getTopicName (topic).isNotEmpty() && getTopicSummary (topic).isNotEmpty() && explain (topic).length() > 400,
               "the sound guide explains " + getTopicName (topic));

    check (getCopySumDb (500.0, 0.001, false) <= -60.0f && std::abs (getCopySumDb (1000.0, 0.001, false) - 6.02f) < 0.01f
               && getCopySumDb (1000.0, 0.001, true) <= -60.0f && getCopySumDb (20.0, 0.0, true) <= -60.0f
               && std::abs (getCopySumDb (500.0, 0.001, true) - 6.02f) < 0.01f && std::abs (getCopySumDb (250.0, 0.001, false) - 3.01f) < 0.01f,
           "a copy 1 ms late cancels 500 Hz and doubles 1 kHz; flipped, the other way round, and with no delay it cancels everything");

    // Left minus right: what's in the middle cancels, what's on one side keeps its level, and the low end goes.
    {
        constexpr double rate = 44100.0;
        juce::AudioBuffer<float> middle (2, 44100), side (2, 44100), mix (2, 44100), mono (1, 44100);

        for (int i = 0; i < 44100; ++i)
        {
            const float voice = 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate);
            const float bass = 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 60.0 * i / rate);
            const float guitar = 0.2f * (float) std::sin (juce::MathConstants<double>::twoPi * 1000.0 * i / rate);
            middle.setSample (0, i, voice);
            middle.setSample (1, i, voice);
            side.setSample (0, i, guitar);
            side.setSample (1, i, 0.0f);
            mix.setSample (0, i, voice + bass + guitar);
            mix.setSample (1, i, voice + bass);
            mono.setSample (0, i, voice + bass);
        }

        const auto inMiddle = measureLeftMinusRight (middle, rate);
        const auto onOneSide = measureLeftMinusRight (side, rate);
        const auto both = measureLeftMinusRight (mix, rate);
        check (! inMiddle.stereo && inMiddle.differenceDb <= -99.0f && std::abs (inMiddle.mixDb - juce::Decibels::gainToDecibels (0.3f / std::sqrt (2.0f))) < 0.05f,
               "left minus right cancels what's in the middle");
        check (onOneSide.stereo && std::abs (onOneSide.differenceDb - onOneSide.mixDb) < 0.05f,
               "left minus right keeps a sound on one side at its level (" + juce::String (onOneSide.differenceDb - onOneSide.mixDb, 2) + " dB)");
        check (both.stereo && both.mixLowDb - both.differenceLowDb > 30.0f && both.differenceDb < both.mixDb - 5.0f,
               "a bass in the middle is lost below 150 Hz (" + juce::String (both.mixLowDb - both.differenceLowDb, 1) + " dB)");
        check (! measureLeftMinusRight (mono, rate).stereo && measureLeftMinusRight (mono, rate).differenceDb <= -99.0f,
               "a mono clip leaves nothing");

        const auto& typical = getTypicalMix();
        int middleGone = 0, vocalsKept = 0;
        for (const auto& part : typical)
        {
            if (! survivesLeftMinusRight (part))
                ++middleGone;
            if (part.vocals && survivesLeftMinusRight (part))
                ++vocalsKept;
        }
        check (middleGone == 4 && vocalsKept == 1 && typical.size() == 9,
               "in the typical mix left minus right takes the vocal, kick, snare and bass, but keeps the vocal reverb");
    }

    // The mono check: (L+R)/2, overall and per band.
    {
        constexpr double rate = 44100.0;
        const double twoPi = juce::MathConstants<double>::twoPi;
        auto make = [&] (auto fillSample, int seconds = 2, int channels = 2)
        {
            juce::AudioBuffer<float> b (channels, (int) (rate * seconds));
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                float l = 0.0f, r = 0.0f;
                fillSample (i, l, r);
                b.setSample (0, i, l);
                if (channels > 1)
                    b.setSample (1, i, r);
            }
            return b;
        };
        auto tone = [&] (double hz, int i) { return (float) std::sin (twoPi * hz * i / rate); };

        const auto same = measureMono (make ([&] (int i, float& l, float& r) { l = r = 0.3f * tone (500.0, i); }), rate);
        check (! same.stereo, "the same signal in left and right is not stereo for the mono check");
        const auto sameSide = measureMono (make ([&] (int i, float& l, float& r) { l = 0.3f * tone (500.0, i); r = 0.3f * tone (500.0, i) + 0.001f * tone (900.0, i); }), rate);
        check (sameSide.stereo && std::abs (sameSide.lossDb) < 0.01f && std::abs (sameSide.correlation - 1.0f) < 0.01f,
               "almost the same left and right lose 0 dB in mono (" + juce::String (sameSide.lossDb, 3) + ")");

        const auto leftOnly = measureMono (make ([&] (int i, float& l, float& r) { l = 0.3f * tone (500.0, i); r = 0.0f; }), rate);
        check (leftOnly.stereo && std::abs (leftOnly.lossDb + 3.01f) < 0.02f, "a sound on the left only loses 3.01 dB in mono (" + juce::String (leftOnly.lossDb, 3) + ")");

        const auto half = measureMono (make ([&] (int i, float& l, float& r) { l = 0.3f * tone (500.0, i); r = -0.5f * l; }), rate);
        check (half.stereo && std::abs (half.lossDb + 10.0f) < 0.05f && half.correlation < -0.99f,
               "right = left flipped at half level loses 10 dB (" + juce::String (half.lossDb, 3) + ")");

        const auto opposite = measureMono (make ([&] (int i, float& l, float& r) { l = 0.3f * tone (500.0, i); r = -l; }), rate);
        check (opposite.stereo && opposite.lossDb <= -60.0f, "exactly opposite left and right cancel in mono");

        juce::Random random (42);
        const auto unrelated = measureMono (make ([&] (int, float& l, float& r) { l = 0.2f * (random.nextFloat() * 2.0f - 1.0f); r = 0.2f * (random.nextFloat() * 2.0f - 1.0f); }, 10), rate);
        check (unrelated.stereo && std::abs (unrelated.lossDb + 3.0f) < 0.2f && std::abs (unrelated.correlation) < 0.05f,
               "unrelated left and right lose 3 dB in mono (" + juce::String (unrelated.lossDb, 2) + ", correlation " + juce::String (unrelated.correlation, 3) + ")");

        const auto oneChannel = measureMono (make ([&] (int i, float& l, float&) { l = 0.3f * tone (500.0, i); }, 2, 1), rate);
        check (! oneChannel.stereo && describeMono (oneChannel).contains ("mono: left and right are the same"), "a one-channel clip is not stereo");

        const auto bands = measureMono (make ([&] (int i, float& l, float& r)
                                              {
                                                  l = 0.2f * tone (60.0, i) + 0.2f * tone (1000.0, i);
                                                  r = -0.5f * 0.2f * tone (60.0, i) + 0.2f * tone (1000.0, i);
                                              }), rate);
        check (std::abs (bands.bands[0].lossDb + 10.0f) < 0.3f && std::abs (bands.bands[2].lossDb) < 0.3f && bands.mostLostBand == 0,
               "a flipped bass and a centred 1 kHz: low band " + juce::String (bands.bands[0].lossDb, 2) + ", mids " + juce::String (bands.bands[2].lossDb, 2)
                   + ", the low band loses most");

        // The advice, one string per rule.
        check (describeMono (same).startsWith ("Your clip is mono"), "mono advice: a mono clip");
        MonoCheck quiet = leftOnly;
        quiet.measurable = false;
        check (describeMono (quiet) == "Too short or too quiet to measure.", "mono advice: too short or too quiet");
        check (describeMono (sameSide).startsWith ("Mostly in the middle: in mono it's only 0.0 dB quieter (correlation +1.00), so the clip as a whole holds up in mono."), "mono advice: mostly in the middle");
        MonoCheck wide = unrelated;
        wide.measurable = true;
        wide.mostLostBand = -1;
        wide.correlation = 0.02f;
        check (describeMono (wide).startsWith ("Wide, as stereo music often is: 3.0 dB quieter"), "mono advice: wide");
        const auto text = describeMono (bands);
        check (text.startsWith ("Wide, as stereo music often is") && text.contains ("The low end loses most: 10.0 dB below 150 Hz"),
               "mono advice: wide, with the low end named (" + text + ")");
        MonoCheck against = unrelated;
        against.measurable = true;
        against.mostLostBand = -1;
        against.correlation = -0.2f;
        check (describeMono (against).startsWith ("Parts of this clip work against each other: in mono it loses 3.0 dB"), "mono advice: working against itself");
        MonoCheck mids = bands;
        mids.mostLostBand = 3;
        mids.bands[3].lossDb = -7.0f;
        check (describeMono (mids).contains ("The upper mids lose most (7.0 dB)"), "mono advice: a band in the upper mids");
        mids.mostLostBand = 2;
        mids.bands[2].lossDb = -7.0f;
        check (describeMono (mids).contains ("The mids lose most") , "mono advice: a band in the mids");
    }

    // The envelope of a note: attack, decay, sustain and release, measured from 10% to 90%.
    {
        constexpr double rate = 44100.0;
        // A sine at 440 Hz: A 50 ms, D 200 ms down to 0.5, held for 1 s, R 300 ms, after half a second of silence.
        const double aT = 0.05, dT = 0.2, hT = 1.0, rT = 0.3, lead = 0.5;
        juce::AudioBuffer<float> note (1, (int) (rate * 3.0));
        note.clear();
        for (int i = 0; i < note.getNumSamples(); ++i)
        {
            const double t = i / rate - lead;
            double env = 0.0;
            if (t >= 0.0 && t < aT) env = t / aT;
            else if (t >= aT && t < aT + dT) env = 1.0 - 0.5 * (t - aT) / dT;
            else if (t >= aT + dT && t < aT + dT + hT) env = 0.5;
            else if (t >= aT + dT + hT && t < aT + dT + hT + rT) env = 0.5 * (1.0 - (t - aT - dT - hT) / rT);
            note.setSample (0, i, (float) (env * std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate)));
        }

        const auto e = measureEnvelope (note, rate, (int) (rate * 1.2));
        check (e.found && e.sustained && e.hasDecay, "the envelope finds a sustained note with a decay");
        check (std::abs (e.attack - 0.040) < 0.006, "envelope attack of a 50 ms ramp is about 40 ms (" + juce::String (e.attack * 1000.0, 1) + " ms)");
        check (std::abs (e.decay - 0.160) < 0.010, "envelope decay of 200 ms is about 160 ms (" + juce::String (e.decay * 1000.0, 1) + " ms)");
        check (std::abs (e.sustainDb + 6.02f) < 0.5f, "envelope sustain is about -6 dB (" + juce::String (e.sustainDb, 2) + " dB)");
        check (std::abs (e.release - 0.240) < 0.010 && e.releaseFound, "envelope release of 300 ms is about 240 ms (" + juce::String (e.release * 1000.0, 1) + " ms)");
        check (std::abs (e.onsetSeconds - lead) < 0.03 && e.endSeconds > lead + aT + dT + hT, "the envelope finds where the note starts and ends");
        const auto sameNote = measureEnvelope (note, rate, (int) (rate * 0.6));
        check (sameNote.found && std::abs (sameNote.attack - e.attack) < 1.0e-9, "the focus anywhere in the note finds the same note");
        const auto text = describeEnvelope (e);
        check (text.startsWith ("Attack 4") && text.contains (", decay 0.1") && text.contains ("it holds for") && text.contains ("then fades out in 0.2"),
               "the envelope text for a sustained note: " + text);
        check (describeEnvelope (measureEnvelope (note, rate, (int) (rate * 0.1))) == "Silence: nothing to measure." || ! measureEnvelope (note, rate, (int) (rate * 0.1)).found,
               "no note found in the silence before it");

        // An exponential fade (a struck string): tau 0.5 s falls 17.4 dB a second.
        juce::AudioBuffer<float> pluck (1, (int) (rate * 4.0));
        pluck.clear();
        for (int i = (int) (rate * 0.5); i < pluck.getNumSamples(); ++i)
        {
            const double t = i / rate - 0.5;
            pluck.setSample (0, i, (float) (0.8 * std::min (1.0, t / 0.005) * std::exp (-t / 0.5) * std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / rate)));
        }

        const auto fade = measureEnvelope (pluck, rate, (int) (rate * 1.0));
        check (fade.found && ! fade.sustained && std::abs (fade.fadeDbPerSecond - 17.4f) < 1.5f,
               "an exponential fade is a fading note, about 17.4 dB a second (" + juce::String (fade.fadeDbPerSecond, 1) + ")");
        check (describeEnvelope (fade).contains ("keeps fading, about 17 dB a second") && describeEnvelope (fade).contains ("A hard start"),
               "the envelope text for a fading note with a hard start: " + describeEnvelope (fade));

        juce::AudioBuffer<float> silence (2, (int) (rate * 2.0));
        silence.clear();
        const auto none = measureEnvelope (silence, rate, 1000);
        check (! none.found && none.silent && describeEnvelope (none) == "Silence: nothing to measure.", "the envelope of silence: nothing to measure");

        check (describeEnvelope (measureEnvelope (note, rate, (int) (rate * 0.1))).startsWith ("Silence")
                   || describeEnvelope (measureEnvelope (note, rate, (int) (rate * 0.1))).startsWith ("No single note"),
               "the envelope text with no note under the focus");

        // The example clip: the numbers the guide shows when no clip is open.
        const auto example = makeExampleClip (rate);
        check (example.getNumChannels() == 2 && example.getNumSamples() == (int) (8.0 * rate), "the example clip is 8 s of stereo");
        const auto m = measureMono (example, rate);
        check (m.stereo && m.measurable && std::abs (m.lossDb + 0.83f) < 0.2f && std::abs (m.correlation - 0.65f) < 0.03f,
               "the example clip loses about 0.83 dB in mono, with correlation +0.65 (" + juce::String (m.lossDb, 2) + ", " + juce::String (m.correlation, 3) + ")");
        check (std::abs (m.bands[0].lossDb + 10.0f) < 1.0f && std::abs (m.bands[2].lossDb) < 0.5f && m.mostLostBand == 0,
               "the example clip's low band loses about 10 dB and is the one that loses most (" + juce::String (m.bands[0].lossDb, 2) + ")");
        check (m.bands[4].lossDb < -0.2f && m.bands[4].lossDb > -3.5f, "the example clip's highs lose a little (" + juce::String (m.bands[4].lossDb, 2) + " dB)");
        check (describeMono (m).contains ("Mostly in the middle: in mono it's only 0.8 dB quieter (correlation +0.65)") && describeMono (m).contains ("The low end loses most: 10.0 dB"),
               "the example clip's mono text names the low end: " + describeMono (m));

        const auto ee = measureEnvelope (example, rate, (int) (rate * 1.0));
        check (ee.found && ee.sustained && std::abs (ee.attack - 0.016) < 0.006 && std::abs (ee.decay - 0.160) < 0.010
                   && std::abs (ee.sustainDb + 6.0f) < 0.5f && std::abs (ee.release - 0.240) < 0.010,
               "the example clip's envelope at 1.0 s: A 16 ms, D 160 ms, S -6 dB, R 240 ms");
    }

    // The shapes: between -1 and 1, starting at 0 going up (the square at its top), with their harmonics.
    bool inRange = true;
    for (const auto shape : allShapes)
        for (int i = 0; i < 1000; ++i)
            inRange = inRange && std::abs (getSample (shape, i / 1000.0, 0.01)) <= 1.0001f;

    check (inRange, "every shape stays between -1 and 1, smoothed or not");
    check (std::abs (getSample (Shape::sine, 0.25) - 1.0f) < 1.0e-6f && std::abs (getSample (Shape::triangle, 0.25) - 1.0f) < 1.0e-6f
               && std::abs (getSample (Shape::saw, 0.0)) < 1.0e-6f && std::abs (getSample (Shape::saw, 0.25) - 0.5f) < 1.0e-6f
               && std::abs (getSample (Shape::square, 0.1) - 1.0f) < 1.0e-6f && std::abs (getSample (Shape::square, 0.6) + 1.0f) < 1.0e-6f,
           "the shapes are where they should be in their cycle");
    check (getHarmonicLevel (Shape::sine, 2) == 0.0f && getHarmonicLevel (Shape::saw, 2) == 0.5f && getHarmonicLevel (Shape::square, 2) == 0.0f
               && std::abs (getHarmonicLevel (Shape::square, 3) - 1.0f / 3.0f) < 1.0e-6f && std::abs (getHarmonicLevel (Shape::triangle, 3) - 1.0f / 9.0f) < 1.0e-6f,
           "a saw has every harmonic, a square and a triangle only the odd ones");

    check (getNoteName (440.0f) == "A4" && getNoteName (261.6f) == "C4" && getNoteName (452.0f) == "A4", "frequencies are named after the nearest note");
    check (formatHz (440.0f) == "440 Hz" && formatHz (1500.0f) == "1.50 kHz" && formatHz (20000.0f) == "20.0 kHz", "frequencies are written in Hz and kHz");
    check (formatTime (0.01) == "10 ms" && formatTime (0.0025) == "2.5 ms" && formatTime (0.25) == "0.25 s" && formatTime (3.21) == "3.2 s",
           "times are written in ms and s");
    check (formatWavelength (440.0f) == "78 cm" && formatWavelength (100.0f) == "3.4 m", "a 440 Hz wave is 78 cm long in the air");

    // Pitch: a note's frequency, nothing for silence or noise.
    constexpr double rate = 44100.0;
    std::vector<float> audio (2205);
    auto fill = [&] (const std::function<float (int)>& f) { for (int i = 0; i < (int) audio.size(); ++i) audio[(size_t) i] = f (i); };

    for (const float hz : { 55.0f, 220.0f, 440.0f, 1200.0f })
    {
        fill ([&] (int i) { return 0.5f * getSample (Shape::saw, i * hz / rate, hz / rate); });
        const float found = estimatePitch (audio.data(), (int) audio.size(), rate);
        check (std::abs (found - hz) < hz * 0.01f, "the pitch of a " + juce::String (hz) + " Hz saw is found (" + juce::String (found, 1) + " Hz)");
    }

    fill ([&] (int i) { return 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / rate)
                               + 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate); });
    check (std::abs (estimatePitch (audio.data(), (int) audio.size(), rate) - 220.0f) < 2.0f, "a strong second harmonic doesn't make it an octave higher");

    fill ([] (int) { return 0.0f; });
    check (estimatePitch (audio.data(), (int) audio.size(), rate) == 0.0f, "silence has no pitch");

    juce::Random random (3);
    fill ([&] (int) { return random.nextFloat() - 0.5f; });
    check (estimatePitch (audio.data(), (int) audio.size(), rate) == 0.0f, "noise has no pitch");

    check (describeMoment (-90.0f, -90.0f, -80.0f, 0.0f).startsWith ("Silence"), "a silent moment is silence");
    check (describeMoment (-60.0f, -12.0f, -3.0f, 0.0f).startsWith ("An attack"), "a jump in level is an attack");
    check (describeMoment (-12.0f, -20.0f, -14.0f, 220.0f).startsWith ("Fading out") && describeMoment (-12.0f, -20.0f, -14.0f, 220.0f).contains ("A3"),
           "a drop in level is a fade, with its pitch");
    check (describeMoment (-14.0f, -14.0f, -9.0f, 440.0f).contains ("A4") && describeMoment (-14.0f, -14.0f, -9.0f, 0.0f).contains ("no"),
           "a held sound says its pitch, or that it has none");
}

static void runGainStagingTests()
{
    using namespace daw::gainstaging;
    std::cout << "Gain staging" << std::endl;

    check (getZone (-70.0f) == Zone::silent && getZone (-30.0f) == Zone::quiet && getZone (-24.0f) == Zone::healthy
               && getZone (-6.0f) == Zone::healthy && getZone (-3.0f) == Zone::hot && getZone (0.0f) == Zone::hot && getZone (0.5f) == Zone::over,
           "levels fall in the quiet, healthy, hot and over zones");
    check (formatDb (-70.0f) == "-inf dB" && formatDb (2.0f) == "+2.0 dB" && formatDb (-12.5f) == "-12.5 dB", "levels are written with their sign");

    for (auto topic : allTopics)
        check (getTopicName (topic).isNotEmpty() && getTopicSummary (topic).isNotEmpty() && explain (topic).length() > 200,
               "the guide explains \"" + getTopicName (topic) + "\"");

    auto stop = [] (juce::String name, StopKind kind, float in, float out)
    {
        Stop s;
        s.name = std::move (name);
        s.kind = kind;
        s.inDb = in;
        s.outDb = out;
        return s;
    };

    // An instrument at a healthy level, an EQ that makes it 8 dB louder, a compressor hit over 0 dB,
    // a fader pulling 9 dB off again, and a master over 0 dB.
    std::vector<Stop> chain { stop ("Synth", StopKind::instrument, -100.0f, -12.0f), stop ("DawIt EQ", StopKind::effect, -12.0f, 1.0f),
                              stop ("DawIt Compressor", StopKind::effect, 1.0f, -2.0f), stop ("Fader", StopKind::fader, -2.0f, -11.0f),
                              stop ("Master", StopKind::master, -6.0f, 1.5f) };
    chain[1].changeDb = 8.0f;
    chain[1].canMatch = true;
    chain[2].changeDb = 0.0f;
    chain[2].isCompressor = true;
    chain[3].settingDb = -9.0f;

    auto hints = findHints (chain);
    auto has = [&hints] (int at, Topic topic) { return std::any_of (hints.begin(), hints.end(), [=] (const Hint& h) { return h.stop == at && h.topic == topic; }); };

    check (hints.size() == 4 && has (1, Topic::louder) && has (2, Topic::compressor) && has (3, Topic::fader) && has (4, Topic::over),
           "the hints find the louder effect, the hot compressor, the fader taking it back and the master over 0 dB ("
               + juce::String ((int) hints.size()) + ")");
    check (hints.back().serious && ! hints.front().serious && hints.front().text.contains ("badge"),
           "only the master's is serious, and DawIt EQ's points at its badge");

    chain[1].matching = true;
    chain[1].changeDb = 0.3f;
    chain[2].inDb = -1.0f;
    chain[3].settingDb = -3.0f;
    chain[4].outDb = -4.0f;
    check (findHints (chain).empty(), "with match level on and sensible levels, nothing stands out");

    chain[1].changeDb = 7.0f;
    hints = findHints (chain);
    check (hints.size() == 1 && has (1, Topic::louder) && hints[0].text.contains ("output"),
           "an effect still much louder with match level on points at its output gain");
    chain[1].changeDb = 0.3f;

    chain[0].outDb = -45.0f;
    hints = findHints (chain);
    check (hints.size() == 1 && has (0, Topic::zones), "a very quiet start is noticed");

    for (auto& s : chain)
        s.inDb = s.outDb = -100.0f;

    check (findHints (chain).empty(), "nothing is noticed in silence");

    // Buses: the tracks add up.
    {
        std::vector<Stop> busChain { stop ("Drums", StopKind::busInput, -100.0f, 2.0f), stop ("DawIt EQ", StopKind::effect, 2.0f, 2.0f),
                                     stop ("Fader", StopKind::fader, 2.0f, 2.0f), stop ("Master", StopKind::master, -6.0f, -6.0f) };
        busChain[0].numTracks = 4;
        busChain[1].changeDb = 0.0f;
        const auto busHints = findHints (busChain);
        check (busHints.size() == 1 && busHints[0].stop == 0 && busHints[0].topic == Topic::bus && ! busHints[0].serious && busHints[0].text.contains ("+2.0 dB")
                   && busHints[0].text.contains ("4 tracks in Drums"),
               "gain staging: a bus whose tracks add up over 0 dB gets one hint, and its first effect doesn't repeat it");

        std::vector<Stop> trackChain { stop ("Synth", StopKind::instrument, -100.0f, -12.0f), stop ("Fader", StopKind::fader, -12.0f, -12.0f),
                                       stop ("Drums", StopKind::bus, 1.0f, 1.0f), stop ("Master", StopKind::master, -6.0f, -6.0f) };
        const auto trackHints = findHints (trackChain);
        check (trackHints.size() == 1 && trackHints[0].stop == 2 && trackHints[0].topic == Topic::bus, "gain staging: a bus on a track's way gets the hint when its sum is over 0 dB");

        // A bus with no effects has nothing to turn down for: the hot-sum hint stays quiet.
        busChain[1].kind = StopKind::fader;
        busChain[0].busHasEffects = false;
        check (findHints (busChain).empty(), "gain staging: a hot bus without effects gets no hint");
        trackChain[2].busHasEffects = false;
        check (findHints (trackChain).empty(), "gain staging: a hot bus on a track's way gets no hint when it has no effects");
        trackChain[2].busHasEffects = true;

        std::vector<Stop> quietChain { stop ("Drums", StopKind::busInput, -100.0f, -45.0f), stop ("Fader", StopKind::fader, -45.0f, -45.0f),
                                       stop ("Master", StopKind::master, -45.0f, -45.0f) };
        const auto quietHints = findHints (quietChain);
        check (quietHints.size() == 1 && quietHints[0].stop == 0 && quietHints[0].topic == Topic::zones, "gain staging: a bus chain that starts very quiet is noticed");
    }
}

static void runPitchRowsTests()
{
    std::cout << "Piano roll rows" << std::endl;
    daw::PitchRows rows;

    check (rows.getNumRows() == 128 && rows.getPitch (0) == 127 && rows.getPitch (127) == 0 && rows.getRow (60) == 67
               && rows.getPitch (-5) == 127 && rows.getPitch (500) == 0,
           "every pitch gets a row, highest first");

    rows.showKey ({ 9, daw::MusicalKey::Scale::minor });   // A minor: the white keys
    bool onlyWhiteKeys = true;
    for (int row = 0; row < rows.getNumRows(); ++row)
        onlyWhiteKeys = onlyWhiteKeys && ! juce::MidiMessage::isMidiNoteBlack (rows.getPitch (row));

    check (rows.getNumRows() == 75 && onlyWhiteKeys && rows.getPitch (0) == 127 && rows.getPitch (74) == 0,
           "the key's filter keeps only its pitches");
    check (! rows.isShown (61) && rows.getRow (61) == -1 && rows.getRow (60) == rows.getRow (62) + 1
               && rows.getNearestRow (61) == rows.getRow (60),
           "a hidden pitch has no row, and the nearest shown one below it stands in");
    check (rows.getPitch (rows.getRow (64) - 1) == 65 && rows.getPitch (rows.getRow (60) + 1) == 59,
           "one row up or down is the next note of the key");

    rows.showKey ({ 6, daw::MusicalKey::Scale::major });   // F# major has no C: the nearest row then comes from below
    check (! rows.isShown (0) && rows.getNearestRow (0) == rows.getNumRows() - 1 && rows.getPitch (rows.getNearestRow (60)) == 59,
           "the nearest row works without the pitch's own row");

    rows.showAll();
    check (rows.getNumRows() == 128 && rows.isShown (61), "turning the filter off brings every row back");
}

static void runChordTrackTests()
{
    using namespace daw::gen;
    using daw::Chord;
    std::cout << "Chord track" << std::endl;

    // Chord symbols.
    auto name = [] (const char* text) { const auto c = Chord::fromName (text); return c.has_value() ? c->getName() : juce::String ("?"); };
    check (name ("Am") == "Am" && name ("F#m7") == "F#m7" && name ("Bbmaj7") == "Bbmaj7" && name ("csus4") == "Csus4"
               && name ("Dmin") == "Dm" && name ("E7") == "E7" && name ("Gmadd9") == "Gm(add9)",
           "chord symbols parse and print the same");
    check (! Chord::fromName ("H7").has_value() && ! Chord::fromName ("Cxyz").has_value() && ! Chord::fromName ("").has_value(),
           "unknown chord symbols are rejected");
    check (Chord::fromName ("Am")->getNoteNames() == "A C E" && Chord::fromName ("E")->getNoteNames() == "E G# B"
               && Chord::fromName ("Ebm7")->getNoteNames() == "Eb Gb Bb Db" && Chord::fromName ("Bdim7")->getNoteNames() == "B D F Ab"
               && Chord::fromName ("Fsus4")->getNoteNames() == "F Bb C" && Chord::fromName ("Cadd9")->getNoteNames() == "C E G D",
           "chord notes are spelled from the root: " + Chord::fromName ("E")->getNoteNames());

    for (auto& q : Chord::getQualities())
        if (Chord::fromName (Chord { 2, q.intervals }.getName()) != Chord { 2, q.intervals })
            check (false, "D" + juce::String (q.suffix) + " survives a round trip through its name");

    // Theory hints for the chord menu.
    {
        namespace harmony = daw::gen::harmony;
        const daw::MusicalKey cMajor, aMinor { 9, daw::MusicalKey::Scale::minor };
        juce::StringArray numerals;
        for (int d = 1; d <= 7; ++d)
            numerals.add (harmony::getRomanNumeral (d, harmony::triad (cMajor, d)));
        check (numerals.joinIntoString (" ") == juce::String (juce::CharPointer_UTF8 ("I ii iii IV V vi vii\xc2\xb0")),
               "roman numerals in C major: " + numerals.joinIntoString (" "));
        check (harmony::findDegree (aMinor, *Chord::fromName ("F")) == 6 && harmony::findDegree (aMinor, *Chord::fromName ("F#")) == 0,
               "F is degree 6 of A minor, F# isn't in it");
        check (harmony::getUsualNextDegrees (cMajor, 5)[0] == 1 && harmony::getFunction (5) == "dominant", "V resolves to I");
        check (harmony::getBorrowedDominant (aMinor).has_value() && harmony::getBorrowedDominant (aMinor)->getName() == "E"
                   && ! harmony::getBorrowedDominant (cMajor).has_value(),
               "minor keys offer the major dominant from harmonic minor");
    }

    // The engine keeps one chord per bar.
    {
        daw::Engine engine;
        engine.setChord (2, Chord::fromName ("F"));
        engine.setChord (3, Chord::fromName ("G"));
        engine.setChord (3, std::nullopt);
        check (engine.getChord (2) != nullptr && engine.getChord (2)->getName() == "F" && engine.getChord (3) == nullptr
                   && engine.getChords().size() == 1,
               "chords can be set and cleared per bar");
    }

    Request r;
    r.key = { 9, daw::MusicalKey::Scale::minor }; // A minor
    r.bars = 4;
    r.part = Part::melody;
    Result result;
    juce::String error;

    // Without a chord track, a melody comes with its own progression, which fills the chord track.
    check (r.needsChords() && r.asksForBarChords() && juce::JSON::toString (buildSchema (r)).contains ("\"chords\""),
           "melody without a chord track asks for chords");
    check (realize (r, "{\"chords\":[{\"degree\":1,\"colour\":\"triad\"},{\"degree\":6,\"colour\":\"triad\"},"
                       "{\"degree\":3,\"colour\":\"triad\"},{\"degree\":7,\"colour\":\"seventh\"}],"
                       "\"notes\":[{\"start\":0,\"length\":1,\"degree\":1,\"octave\":0,\"velocity\":100}]}",
                   result, error)
               && result.newChords.size() == 4 && result.newChords[0]->getName() == "Am" && result.newChords[3]->getName() == "G7",
           "melody's chords are returned for the chord track " + error);

    // With a full chord track, nothing is asked for or added, and the prompt lists the chords.
    r.chordTrack = { Chord::fromName ("Am"), Chord::fromName ("F"), Chord::fromName ("C"), Chord::fromName ("E") };
    const auto prompt = buildUserPrompt (r);
    check (! r.needsChords() && ! juce::JSON::toString (buildSchema (r)).contains ("\"chords\""),
           "melody over a full chord track doesn't ask for chords");
    check (prompt.contains ("Chord track") && prompt.contains ("bar 4: E (E G# B)"), "prompt lists the chord track");
    realize (r, "{\"notes\":[{\"start\":0,\"length\":1,\"degree\":1,\"octave\":0,\"velocity\":100}]}", result, error);
    check (std::none_of (result.newChords.begin(), result.newChords.end(), [] (auto& c) { return c.has_value(); }),
           "nothing is added over a full chord track");

    // The pattern spans whole cycles of the chord track.
    r.bars = 8;
    r.chordTrack = { Chord::fromName ("Am"), Chord::fromName ("F"), Chord::fromName ("C") };
    r.chordTrack.resize (8);
    check (r.getChordPeriod() == 8 && r.getPatternBars() == 8, "a non-repeating chord track is written out in full");
    r.chordTrack = { Chord::fromName ("Am"), Chord::fromName ("F"), Chord::fromName ("Am"), Chord::fromName ("F"),
                     Chord::fromName ("Am"), Chord::fromName ("F"), Chord::fromName ("Am"), Chord::fromName ("F") };
    r.part = Part::drums;
    check (r.getPatternBars() == 2, "drums ignore the chord track");
    r.part = Part::percussion;
    check (r.getPatternBars() == 2 && ! r.needsChords(), "percussion ignores the chord track too");
    r.part = Part::melody;
    check (r.getChordPeriod() == 2 && r.getPatternBars() == 8, "a 2-bar chord loop keeps the 8-bar melody");
    r.bars = 12;
    r.chordTrack.resize (12);
    for (int bar = 8; bar < 12; ++bar)
        r.chordTrack[(size_t) bar] = r.chordTrack[(size_t) bar - 2];
    r.chordTrack[11] = Chord::fromName ("E");
    check (r.getPatternBars() == 12, "a change late in the clip makes the pattern cover the whole clip");

    // Chord part over the chord track: the model writes a rhythm, the notes come from the track.
    r.part = Part::chords;
    r.bars = 4;
    r.chordTrack = { Chord::fromName ("Am"), Chord::fromName ("F"), std::nullopt, Chord::fromName ("E7") };
    check (r.isChordRhythm() && r.asksForBarChords(), "chord part over a partial chord track writes a rhythm and the missing chords");
    const auto schema = juce::JSON::toString (buildSchema (r));
    check (schema.contains ("\"hits\"") && schema.contains ("\"chords\"") && schema.contains ("\"minItems\": 4"),
           "rhythm schema has hits and one chord per bar");
    check (realize (r, "{\"chords\":[{\"degree\":1,\"colour\":\"triad\"},{\"degree\":6,\"colour\":\"triad\"},"
                       "{\"degree\":4,\"colour\":\"seventh\"},{\"degree\":5,\"colour\":\"triad\"}],"
                       "\"hits\":[{\"start\":0,\"length\":6,\"velocity\":100},{\"start\":6,\"length\":1,\"velocity\":80}]}",
                   result, error),
           "chord rhythm parses " + error);
    check (result.summary.startsWith ("Am - F - Dm7 - E7"), "track chords are kept, the empty bar gets the model's chord: " + result.summary);
    check (! result.newChords[0].has_value() && result.newChords[2]->getName() == "Dm7" && ! result.newChords[3].has_value(),
           "only the empty bar gets a new chord");

    std::map<double, std::set<int>> chordsByStart;
    for (auto& n : result.notes)
        chordsByStart[n.start].insert (n.pitch % 12);
    check (chordsByStart[0.0] == std::set<int> { 9, 0, 4 } && chordsByStart[4.0] == std::set<int> { 5, 9, 0 }
               && chordsByStart[6.0] == std::set<int> { 5, 9, 0 } && chordsByStart[12.0] == std::set<int> { 4, 8, 11, 2 },
           "a hit ringing across a bar line is split, and each part plays that bar's chord");
    check (std::all_of (result.notes.begin(), result.notes.end(), [] (auto& n) { return n.getEnd() <= 16.0 + 1.0e-9; })
               && std::none_of (result.notes.begin(), result.notes.end(), [] (auto& n) { return n.start < 4.0 && n.getEnd() > 4.0 + 1.0e-9; }),
           "no chord note rings into the next chord");

    // A chord progression without a chord track fills it with the longest chord of each bar.
    r.chordTrack.clear();
    check (! r.isChordRhythm() && r.needsChords() && ! r.asksForBarChords(), "chord part without a chord track is a progression");
    realize (r, "{\"chords\":[{\"degree\":1,\"colour\":\"triad\",\"beats\":3},{\"degree\":5,\"colour\":\"triad\",\"beats\":1},"
                "{\"degree\":6,\"colour\":\"triad\",\"beats\":4},{\"degree\":3,\"colour\":\"triad\",\"beats\":4},"
                "{\"degree\":7,\"colour\":\"triad\",\"beats\":4}]}", result, error);
    juce::StringArray filled;
    for (auto& c : result.newChords)
        filled.add (c.has_value() ? c->getName() : "-");
    check (filled.joinIntoString (" ") == "Am F C G", "progression fills the chord track per bar: " + filled.joinIntoString (" "));
}

static void runGeneratorTests()
{
    using namespace daw::gen;
    std::cout << "Generator (realizing model replies)" << std::endl;

    Request r;
    r.key = { 9, daw::MusicalKey::Scale::minor }; // A minor
    r.bars = 4;

    // Chords: i - VI - III - VII in A minor, wrapped in a code fence like chatty models do.
    r.part = Part::chords;
    Result result;
    juce::String error;
    const bool ok = realize (r, "```json\n{\"chords\":[{\"degree\":1,\"colour\":\"triad\",\"beats\":4},"
                                "{\"degree\":6,\"colour\":\"triad\",\"beats\":4},"
                                "{\"degree\":3,\"colour\":\"triad\",\"beats\":4},"
                                "{\"degree\":7,\"colour\":\"seventh\",\"beats\":3}]}\n```", result, error);
    check (ok, "chord reply parses " + error);
    check (result.summary == "Am - F - C - G7", "chord symbols: " + result.summary);

    // In-key chords on every degree, with colours.
    const daw::MusicalKey aMinor { 9, daw::MusicalKey::Scale::minor }, cMajor {};
    juce::StringArray names;
    for (int d = 1; d <= 7; ++d)
        names.add (getChordName ((aMinor.tonic + aMinor.getIntervals()[d - 1]) % 12, getChordIntervals (aMinor, d, "seventh")));
    check (names.joinIntoString (" ") == "Am7 Bm7b5 Cmaj7 Dm7 Em7 Fmaj7 G7", "A minor sevenths: " + names.joinIntoString (" "));
    check (getChordName (7, getChordIntervals (cMajor, 5, "ninth")) == "G9", "G9 in C major");
    check (getChordName (0, getChordIntervals (cMajor, 1, "add9")) == "Cadd9", "Cadd9");
    check (getChordName (2, getChordIntervals (cMajor, 2, "sus4")) == "Dsus4", "Dsus4");
    check (getChordName (4, getChordIntervals (aMinor, 5, "borrowed_major")) == "E", "borrowed major V in A minor is E");
    check (juce::exactlyEqual (result.lengthBeats, 16.0), "result covers 4 bars");

    std::set<int> firstChord;
    double lastEnd = 0.0;
    for (auto& n : result.notes)
    {
        if (juce::exactlyEqual (n.start, 0.0))
            firstChord.insert (n.pitch % 12);

        lastEnd = juce::jmax (lastEnd, n.getEnd());
    }
    check (firstChord == std::set<int> { 9, 0, 4 }, "first chord is A C E");
    check (std::abs (lastEnd - 16.0) < 1.0e-9, "last chord is stretched to fill the bars");

    int maxLeap = 0;
    for (auto& a : result.notes)
        for (auto& b : result.notes)
            if (juce::exactlyEqual (b.start, a.getEnd()))
            {
                int nearest = 127;
                for (auto& c : result.notes)
                    if (juce::exactlyEqual (c.start, b.start))
                        nearest = juce::jmin (nearest, std::abs (c.pitch - a.pitch));
                maxLeap = juce::jmax (maxLeap, nearest);
            }
    check (maxLeap <= 5, "voice leading keeps every voice within a 4th (" + juce::String (maxLeap) + " semitones)");

    // Melody: degrees map into the key, overlaps are trimmed, an 8-bar phrase repeats to fill 16 bars.
    r.part = Part::melody;
    r.bars = 16;
    check (realize (r, "{\"notes\":[{\"start\":0,\"length\":2,\"degree\":1,\"octave\":0,\"velocity\":100},"
                       "{\"start\":1,\"length\":1,\"degree\":5,\"octave\":0,\"velocity\":90},"
                       "{\"start\":2.1,\"length\":0.5,\"degree\":3,\"octave\":1,\"velocity\":80},"
                       "{\"start\":40,\"length\":1,\"degree\":2,\"octave\":0,\"velocity\":80}]}",
                    result, error), "melody reply parses " + error);
    check (result.notes.size() == 6, "out-of-range note dropped, 3-note phrase repeated twice (" + juce::String ((int) result.notes.size()) + ")");
    check (result.notes[0].pitch == 69 && result.notes[1].pitch == 76 && result.notes[2].pitch == 84,
           "degrees 1, 5, 3+octave in A minor -> A4, E5, C6");
    check (juce::exactlyEqual (result.notes[0].length, 1.0), "overlapping note is trimmed to the next one");
    check (juce::exactlyEqual (result.notes[2].start, 2.0), "start is quantised to 16ths");
    check (juce::exactlyEqual (result.notes[3].start, 32.0), "phrase repeats after 8 bars");
    check (std::all_of (result.notes.begin(), result.notes.end(), [&] (auto& n) { return r.key.contains (n.pitch); }),
           "every melody note is in key");

    // Bass sits low.
    r.part = Part::bassline;
    r.bars = 4;
    realize (r, "{\"notes\":[{\"start\":0,\"length\":1,\"degree\":1,\"octave\":0,\"velocity\":100}]}", result, error);
    check (! result.notes.empty() && result.notes[0].pitch == 33, "bass degree 1 in A minor is A1 (" + juce::String (result.notes[0].pitch) + ")");

    // Drums: General MIDI mapping, unknown names ignored, a 2-bar loop fills 4 bars.
    r.part = Part::drums;
    check (realize (r, "{\"hits\":[{\"start\":0,\"drum\":\"kick\",\"velocity\":120},"
                       "{\"start\":1,\"drum\":\"snare\",\"velocity\":100},"
                       "{\"start\":1,\"drum\":\"snare\",\"velocity\":100},"
                       "{\"start\":2,\"drum\":\"cowbell_xl\",\"velocity\":100},"
                       "{\"start\":0.5,\"drum\":\"closed_hat\",\"velocity\":60}]}", result, error),
           "drum reply parses " + error);
    check (result.notes.size() == 6, "duplicate and unknown hits dropped, loop repeated (" + juce::String ((int) result.notes.size()) + ")");
    check (result.notes[0].pitch == 36 && result.notes[1].pitch == 42 && result.notes[2].pitch == 38, "kick 36, hat 42, snare 38");
    check (juce::exactlyEqual (result.notes[3].start, 8.0), "2-bar loop repeats at beat 8");

    // Percussion: its own instruments on the General MIDI drum map, looped the same way.
    r.part = Part::percussion;
    check (realize (r, "{\"hits\":[{\"start\":0,\"instrument\":\"conga_high\",\"velocity\":110},"
                       "{\"start\":0.25,\"instrument\":\"shaker\",\"velocity\":50},"
                       "{\"start\":0.5,\"instrument\":\"conga_mute\",\"velocity\":70},"
                       "{\"start\":1,\"instrument\":\"snare\",\"velocity\":100},"
                       "{\"start\":3,\"instrument\":\"claves\",\"velocity\":90}]}", result, error),
           "percussion reply parses " + error);
    check (result.notes.size() == 8 && result.notes[0].pitch == 63 && result.notes[1].pitch == 82 && result.notes[2].pitch == 62
               && result.notes[3].pitch == 75 && result.newChords.empty(),
           "conga 63, shaker 82, muted conga 62, claves 75; kit drums are not percussion (" + juce::String ((int) result.notes.size()) + ")");
    check (! realize (r, "{\"hits\":[{\"start\":0,\"drum\":\"kick\",\"velocity\":100}]}", result, error)
               && error.contains ("percussion"),
           "a percussion reply without instruments is rejected: " + error);

    check (! realize (r, "sorry, I can't do that", result, error) && error.isNotEmpty(), "non-JSON reply is rejected: " + error);

    // Prompt carries the key, style, bars and existing harmony.
    r.part = Part::melody;
    r.style = "Trance";
    r.harmonyPerBar = { "A C E", "F A C" };
    const auto prompt = buildUserPrompt (r);
    check (prompt.contains ("A minor") && prompt.contains ("1=A") && prompt.contains ("7=G") && prompt.contains ("Trance")
               && prompt.contains ("bar 2: F A C"),
           "prompt mentions key, degrees, style and harmony");
    {
        Request drums;
        drums.part = Part::drums;
        check (juce::JSON::toString (buildSchema (drums)).contains ("closed_hat"), "drum schema lists drum names");

        Request percussion = r;
        percussion.part = Part::percussion;
        const auto percussionPrompt = buildUserPrompt (percussion);
        check (juce::JSON::toString (buildSchema (percussion)).contains ("conga_high")
                   && ! juce::JSON::toString (buildSchema (percussion)).contains ("closed_hat")
                   && percussionPrompt.contains ("Allowed instrument names: conga_high") && ! percussionPrompt.contains ("bar 2: F A C"),
               "percussion has its own instruments in the schema and prompt, and no harmony");
    }

    r.part = Part::chords;
    check (buildUserPrompt (r).contains ("1=Am, 2=Bdim, 3=C, 4=Dm, 5=Em, 6=F, 7=G"), "chord prompt lists the in-key chords of A minor");
    check (juce::JSON::toString (buildSchema (r)).contains ("borrowed_major"), "chord schema lists the colours");

    runChordTrackTests();
    runMidiExportTests();
}

static void runOllamaTests (const juce::String& model)
{
    using namespace daw::gen;
    std::cout << "Ollama end-to-end with " << model << std::endl;

    // Thinking: off unless asked for; asked of a model that can't think, it answers anyway.
    for (const bool think : { false, true })
    {
        Request r;
        r.part = Part::bassline;
        r.key = { 9, daw::MusicalKey::Scale::minor };
        r.bars = 2;

        OllamaClient::ChatOptions options;
        options.model = model;
        options.systemPrompt = buildSystemPrompt();
        options.userPrompt = buildUserPrompt (r);
        options.schema = buildSchema (r);
        options.think = think;
        options.maxTokens = think ? 16384 : 4096;

        int thinking = 0;
        bool unsupported = false;
        options.onThinking = [&thinking] (int chars) { thinking = chars; };
        options.onThinkingUnsupported = [&unsupported] { unsupported = true; };

        juce::String error;
        const auto reply = OllamaClient().chat (options, error);
        Result result;

        if (error.isEmpty())
            realize (r, reply, result, error);

        if (! think)
            check (error.isEmpty() && thinking == 0 && ! unsupported, "without Think first the model doesn't think " + error);
        else
            check (error.isEmpty() && (thinking > 0) != unsupported,
                   juce::String ("with Think first the model ") + (unsupported ? "can't think, and answers straight away " : "thinks first ") + error);
    }

    // Takes: a fresh seed and random ideas each, so three melodies over the same chords differ.
    {
        Request r;
        r.part = Part::melody;
        r.style = "Pop";
        r.key = { 9, daw::MusicalKey::Scale::minor };
        r.bars = 2;
        r.chordTrack = { daw::Chord::fromName ("Am"), daw::Chord::fromName ("F") };
        juce::Random random (3);
        std::set<juce::String> distinct;
        int made = 0;

        for (int take = 0; take < 3; ++take)
        {
            auto takeRequest = r;
            takeRequest.directions = pickDirections (r, getDirectionCount (defaultVariation), random);

            OllamaClient::ChatOptions options;
            options.model = model;
            options.systemPrompt = buildSystemPrompt();
            options.userPrompt = buildUserPrompt (takeRequest);
            options.schema = buildSchema (takeRequest);
            options.temperature = getOllamaTemperature (defaultVariation);
            options.seed = 1 + random.nextInt (1000000);

            juce::String error;
            const auto reply = OllamaClient().chat (options, error);
            Result result;

            if (error.isEmpty() && realize (takeRequest, reply, result, error))
            {
                ++made;
                juce::String notes;
                for (auto& n : result.notes)
                    notes << n.pitch << "@" << n.start << " ";

                distinct.insert (notes);
                std::cout << "      take " << (take + 1) << " (" << takeRequest.directions.joinIntoString ("; ") << "): score "
                          << juce::roundToInt (scoreTake (takeRequest, result) * 100.0) << ", " << result.summary << std::endl;
            }
        }

        check (made == 3 && distinct.size() == 3, "three takes come back, all different");
    }

    for (int p = 0; p < numParts; ++p)
    {
        Request r;
        r.part = (Part) p;
        r.style = isPercussive ((Part) p) ? "House" : "EDM";
        r.key = { 9, daw::MusicalKey::Scale::minor };
        r.bpm = 126.0;
        r.bars = 4;

        OllamaClient::ChatOptions options;
        options.model = model;
        options.systemPrompt = buildSystemPrompt();
        options.userPrompt = buildUserPrompt (r);
        options.schema = buildSchema (r);

        const auto started = juce::Time::getMillisecondCounterHiRes();
        juce::String error;
        const auto reply = OllamaClient().chat (options, error);
        Result result;

        if (error.isEmpty())
            realize (r, reply, result, error);

        const auto seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
        const bool inKey = isPercussive (r.part)
                        || std::all_of (result.notes.begin(), result.notes.end(), [&] (auto& n) { return r.key.contains (n.pitch); })
                        || r.part == Part::chords; // chord qualities may add chromatic colour notes

        check (error.isEmpty() && ! result.notes.empty() && inKey,
               getPartName (r.part) + ": " + (error.isNotEmpty() ? error : result.summary)
                   + juce::String::formatted ("  (%.1f s)", seconds));
    }

    // Over a chord track: full for the chord part (rhythm only), half empty for the melody (fills the rest).
    for (auto part : { Part::chords, Part::melody })
    {
        Request r;
        r.part = part;
        r.style = "EDM";
        r.key = { 9, daw::MusicalKey::Scale::minor };
        r.bpm = 126.0;
        r.bars = 4;
        r.chordTrack = { daw::Chord::fromName ("Am"), daw::Chord::fromName ("F"),
                         part == Part::chords ? daw::Chord::fromName ("C") : std::nullopt,
                         part == Part::chords ? daw::Chord::fromName ("G") : std::nullopt };

        OllamaClient::ChatOptions options;
        options.model = model;
        options.systemPrompt = buildSystemPrompt();
        options.userPrompt = buildUserPrompt (r);
        options.schema = buildSchema (r);

        const auto started = juce::Time::getMillisecondCounterHiRes();
        juce::String error;
        const auto reply = OllamaClient().chat (options, error);
        Result result;

        if (error.isEmpty())
            realize (r, reply, result, error);

        const auto seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
        juce::StringArray added;
        for (auto& c : result.newChords)
            added.add (c.has_value() ? c->getName() : "-");

        const bool filled = part == Part::chords ? added.joinIntoString (" ") == "- - - -"
                                                 : added[0] == "-" && added[1] == "-" && added[2] != "-" && added[3] != "-";
        check (error.isEmpty() && ! result.notes.empty() && filled,
               getPartName (r.part) + " over the chord track: " + (error.isNotEmpty() ? error : result.summary)
                   + ", new chords: " + added.joinIntoString (" ") + juce::String::formatted ("  (%.1f s)", seconds));
    }

    // Playing styles varied by the model: every take plays, over the chord track.
    for (auto id : { "piano-ballad", "guitar-folk", "synth-offbeat", "bass-walking" })
    {
        Request r;
        r.part = juce::String (id).startsWith ("bass") ? Part::bassline : Part::chords;
        r.style = "Pop";
        r.bars = 4;
        r.playingStyle = id;
        r.varyStyle = true;
        r.chordTrack = { daw::Chord::fromName ("C"), daw::Chord::fromName ("Am"), daw::Chord::fromName ("F"), daw::Chord::fromName ("G") };
        juce::Random random (7);
        r.directions = pickDirections (r, getDirectionCount (defaultVariation), random);

        OllamaClient::ChatOptions options;
        options.model = model;
        options.systemPrompt = buildSystemPrompt();
        options.userPrompt = buildUserPrompt (r);
        options.schema = buildSchema (r);
        options.maxTokens = 4096;

        juce::String error;
        const auto reply = OllamaClient().chat (options, error);
        Result result;

        if (error.isEmpty())
            realize (r, reply, result, error);

        check (error.isEmpty() && ! result.notes.empty(),
               juce::String ("a varied ") + id + " plays: " + result.summary + ", score " + juce::String (scoreTake (r, result), 2) + " " + error);
    }

    // Lyrics: one rewrite of a line and one rhymes request return replies DawIt can read.
    {
        namespace lai = daw::gen::lyricsai;
        const juce::String text = "[Verse]\nI walked a-long the ri-ver late\nthe ci-ty lights be-hind me\nI feel so sad and lone-ly now\nthe way it used to find me";

        for (const auto kind : { lai::Kind::selection, lai::Kind::rhymes })
        {
            lai::Request r;
            r.kind = kind;
            r.text = text;
            r.rangeStart = kind == lai::Kind::rhymes ? text.indexOf ("late") + 1 : text.indexOf ("I feel");
            r.rangeEnd = kind == lai::Kind::rhymes ? r.rangeStart : text.indexOf ("now") + 3;
            r.brief = { "a night walk after a break-up", "longing", "i" };
            const auto context = lai::buildContext (r);

            OllamaClient::ChatOptions options;
            options.model = model;
            options.systemPrompt = lai::buildSystemPrompt();
            options.userPrompt = lai::buildUserPrompt (r, context);
            options.schema = lai::buildSchema (r, context);
            options.maxTokens = 4096;

            juce::String error;
            const auto reply = OllamaClient().chat (options, error);
            const auto result = error.isEmpty() ? lai::parseReply (r, context, reply) : lai::Result();
            check (error.isEmpty() && result.error.isEmpty() && (kind == lai::Kind::rhymes ? ! result.rhymes.empty() : ! result.alternatives.empty()),
                   juce::String (kind == lai::Kind::rhymes ? "lyrics AI: rhymes for late come back " : "lyrics AI: a rewrite of line 3 comes back ")
                       + (error.isNotEmpty() ? error : result.error + " (" + juce::String ((int) result.alternatives.size()) + " alternatives, "
                                                          + juce::String ((int) result.rhymes.size()) + " rhymes)"));
        }
    }
}

static void runMetronomeTests()
{
    std::cout << "Metronome and count-in" << std::endl;

    // An engine of its own with no tracks, so all you hear is the click.
    daw::Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    FakeDevice device;
    engine.audioDeviceAboutToStart (&device);

    constexpr int samplesPerBeat = (int) (testRate / 2.0); // 120 BPM
    const auto peakAt = [] (const juce::AudioBuffer<float>& b, int beat)
    {
        return b.getMagnitude (0, beat * samplesPerBeat, samplesPerBeat / 10);
    };
    const auto quietBetween = [] (const juce::AudioBuffer<float>& b, int beat)
    {
        return b.getMagnitude (0, beat * samplesPerBeat + samplesPerBeat / 4, samplesPerBeat / 2) < 1e-4f;
    };

    engine.play();
    auto out = render (engine, 1.0);
    check (! engine.metronomeEnabled.load() && out.getMagnitude (0, 0, out.getNumSamples()) < 1e-4f, "the metronome is off by default");

    engine.metronomeEnabled = true;
    engine.setPositionBeats (0.0);
    out = render (engine, 2.0);
    check (peakAt (out, 0) > 0.2f && peakAt (out, 1) > 0.1f && peakAt (out, 2) > 0.1f && peakAt (out, 3) > 0.1f,
           "it clicks on every beat");
    check (quietBetween (out, 0) && quietBetween (out, 1) && quietBetween (out, 2), "and is silent between beats");
    check (peakAt (out, 0) > peakAt (out, 1) * 1.3f, "the first beat of the bar is accented");

    engine.setPositionBeats (0.5);
    out = render (engine, 0.2);
    check (out.getMagnitude (0, 0, out.getNumSamples()) < 1e-4f, "jumping to the middle of a beat doesn't click");

    engine.metronomeOnlyWhileRecording = true;
    engine.setPositionBeats (0.0);
    out = render (engine, 1.0);
    check (out.getMagnitude (0, 0, out.getNumSamples()) < 1e-4f, "\"only while recording\" is quiet during playback");
    engine.metronomeOnlyWhileRecording = false;
    engine.metronomeEnabled = false;

    // Count-in: one bar of clicks while the playhead waits, then recording starts.
    engine.stop();
    render (engine, 0.05);
    auto* track = engine.addTrack (daw::Track::Type::midi);
    engine.setLiveMidiTrack (track);
    engine.countInEnabled = true;
    engine.setPositionBeats (8.0);
    engine.startRecording();

    out = render (engine, 1.0);
    check (std::abs (engine.getPositionBeats() - 8.0) < 1e-9 && std::abs (engine.getCountInBeatsLeft() - 2.0) < 0.02,
           "during the count-in the playhead waits");
    check (peakAt (out, 0) > 0.2f && peakAt (out, 1) > 0.1f && quietBetween (out, 0),
           "the count-in clicks even with the metronome off");

    render (engine, 1.1);
    check (engine.getCountInBeatsLeft() == 0.0 && std::abs (engine.getPositionBeats() - 8.2) < 0.02 && engine.isRecording(),
           "after one bar the transport starts and recording goes on");

    engine.stopRecording();
    engine.stop();
    check (track->clips.empty(), "a count-in with nothing played leaves no take");
}

static void runBuiltInEqTests()
{
    using namespace daw;
    using eq::BandType;
    std::cout << "Built-in EQ" << std::endl;

    // Plays a steady sine through the EQ and measures the change in level over the last half, in dB.
    auto measure = [] (BuiltInEq& plugin, double freq, double seconds = 0.6)
    {
        plugin.prepareToPlay (testRate, testBlock);
        const int total = (int) (seconds * testRate);
        juce::AudioBuffer<float> buffer (2, testBlock);
        juce::MidiBuffer midi;
        double sumIn = 0.0, sumOut = 0.0;

        for (int pos = 0; pos < total; pos += testBlock)
        {
            for (int i = 0; i < testBlock; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    buffer.setSample (ch, i, 0.25f * (float) std::sin (juce::MathConstants<double>::twoPi * freq * (pos + i) / testRate));

            double in = 0.0;
            for (int i = 0; i < testBlock; ++i)
                in += (double) buffer.getSample (0, i) * buffer.getSample (0, i);

            plugin.processBlock (buffer, midi);

            if (pos >= total / 2)
            {
                sumIn += in;
                for (int i = 0; i < testBlock; ++i)
                    sumOut += (double) buffer.getSample (0, i) * buffer.getSample (0, i);
            }
        }

        return 10.0 * std::log10 (juce::jmax (1.0e-30, sumOut) / sumIn);
    };

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };
    auto withOnly = [] (eq::Band band)
    {
        auto plugin = std::make_unique<BuiltInEq>();
        for (int i = 0; i < eq::numBands; ++i)
            plugin->setBand (i, { false, eq::getDefaultBand (i).type, eq::getDefaultBand (i).freq, 0.0f, 1.0f, 12 });
        plugin->setBand (2, band);
        return plugin;
    };

    {
        BuiltInEq flat;
        check (near (measure (flat, 100.0), 0.0, 0.01) && near (measure (flat, 1000.0), 0.0, 0.01) && near (measure (flat, 10000.0), 0.0, 0.01),
               "a new EQ changes nothing: the bands start at 0 dB and the cuts off");
    }

    {
        auto bell = withOnly ({ true, BandType::bell, 1000.0f, 6.0f, 1.0f, 12 });
        const double atCentre = measure (*bell, 1000.0), far = measure (*bell, 100.0);
        check (near (atCentre, 6.0, 0.1) && far < 0.5, "a +6 dB bell at 1 kHz raises 1 kHz by 6 dB (" + juce::String (atCentre, 2) + ") and leaves 100 Hz (" + juce::String (far, 2) + ")");
    }

    {
        auto cut = withOnly ({ true, BandType::lowCut, 100.0f, 0.0f, eq::flatQ, 12 });
        const double atCorner = measure (*cut, 100.0), octaveBelow = measure (*cut, 50.0, 1.0), above = measure (*cut, 1000.0);
        check (near (atCorner, -3.01, 0.1) && near (octaveBelow, -12.3, 0.2) && above > -0.1,
               "a 12 dB/oct low cut at 100 Hz: -3 dB at 100 Hz (" + juce::String (atCorner, 2) + "), about -12 dB an octave below ("
                   + juce::String (octaveBelow, 2) + "), nothing at 1 kHz");

        auto steep = withOnly ({ true, BandType::lowCut, 100.0f, 0.0f, eq::flatQ, 24 });
        const double steepCorner = measure (*steep, 100.0), steepBelow = measure (*steep, 50.0, 1.0);
        check (near (steepCorner, -3.01, 0.1) && near (steepBelow, -24.1, 0.3),
               "at 24 dB/oct: still -3 dB at 100 Hz (" + juce::String (steepCorner, 2) + "), -24 dB an octave below (" + juce::String (steepBelow, 2) + ")");

        // Q above 0.71 adds a bump just above the corner.
        double peak = -100.0;
        for (double f = 80.0; f < 200.0; f += 1.0)
            peak = juce::jmax (peak, eq::getResponseDb ({ true, BandType::lowCut, 100.0f, 0.0f, 4.0f, 12 }, f, testRate));
        check (near (peak, 12.1, 0.3), "a low cut with Q 4 has a bump of about 12 dB (" + juce::String (peak, 1) + ")");
    }

    {
        auto shelf = withOnly ({ true, BandType::highShelf, 5000.0f, 6.0f, eq::flatQ, 12 });
        const double high = measure (*shelf, 15000.0), mid = measure (*shelf, 5000.0), low = measure (*shelf, 200.0);
        check (near (high, 6.0, 0.4) && near (mid, 3.0, 0.2) && near (low, 0.0, 0.1),
               "a +6 dB high shelf at 5 kHz: +6 dB well above, half (+3) at 5 kHz, nothing low (" + juce::String (high, 2) + ", " + juce::String (mid, 2) + ")");

        auto lowShelf = withOnly ({ true, BandType::lowShelf, 200.0f, -6.0f, eq::flatQ, 12 });
        check (near (measure (*lowShelf, 40.0, 1.0), -6.0, 0.3) && near (measure (*lowShelf, 5000.0), 0.0, 0.1), "a -6 dB low shelf at 200 Hz lowers 40 Hz by 6 dB");
    }

    {
        auto notch = withOnly ({ true, BandType::notch, 60.0f, 0.0f, 10.0f, 12 });
        const double at = measure (*notch, 60.0, 1.5), beside = measure (*notch, 80.0, 1.0);
        check (at < -30.0 && beside > -1.0, "a notch at 60 Hz removes 60 Hz (" + juce::String (at, 1) + " dB) and keeps 80 Hz (" + juce::String (beside, 2) + ")");

        auto highCut = withOnly ({ true, BandType::highCut, 2000.0f, 0.0f, eq::flatQ, 12 });
        // A digital filter falls a little faster towards the top (half the sample rate): -12.3 dB plus a bit.
        const double octaveAbove = measure (*highCut, 4000.0);
        check (near (octaveAbove, -12.6, 0.4), "a 12 dB/oct high cut at 2 kHz is about -12 dB at 4 kHz (" + juce::String (octaveAbove, 2) + ")");
    }

    {
        // What the editor draws is what plays.
        bool same = true;
        juce::String worst;

        for (int t = 0; t < eq::numTypes; ++t)
        {
            const eq::Band band { true, (BandType) t, 700.0f, 5.0f, 2.0f, t % 2 == 0 ? 24 : 12 };
            auto plugin = withOnly (band);

            for (double f : { 150.0, 500.0, 700.0, 1100.0, 4000.0 })
            {
                const double drawn = eq::getResponseDb (band, f, testRate), heard = measure (*plugin, f);
                if (drawn > -40.0 && std::abs (drawn - heard) > 0.15)
                {
                    same = false;
                    worst = eq::getTypeName (band.type) + " at " + juce::String (f) + " Hz: drawn " + juce::String (drawn, 2) + ", heard " + juce::String (heard, 2);
                }
            }
        }

        check (same, "for every type, the curve matches the measured level " + worst);
    }

    check (near (eq::getBandwidthOctaves (1.4142), 1.0, 0.01) && near (eq::getBandwidthOctaves (0.6667), 2.0, 0.02),
           "Q 1.41 is one octave wide, Q 0.67 two octaves");

    {
        auto plugin = withOnly ({ true, BandType::bell, 1000.0f, 0.0f, 2.0f, 12 });
        plugin->soloBand = 2;
        const double centre = measure (*plugin, 1000.0), far = measure (*plugin, 100.0);
        plugin->soloBand = -1;
        check (near (centre, 0.0, 0.1) && far < -20.0, "soloing a bell plays only its range: 1 kHz stays, 100 Hz goes (" + juce::String (far, 1) + " dB)");
    }

    {
        auto plugin = withOnly ({ true, BandType::bell, 1000.0f, 12.0f, 0.5f, 12 });
        const double louder = measure (*plugin, 1000.0);
        plugin->getParameters().getParameter ("match")->setValueNotifyingHost (1.0f);
        const double matched = measure (*plugin, 1000.0, 4.0);
        check (near (louder, 12.0, 0.2) && near (matched, 0.0, 0.5) && near (plugin->matchGainDb.load(), -12.0, 0.5),
               "match level turns a +12 dB boost back to the input's loudness (" + juce::String (matched, 2) + " dB)");

        plugin->getParameters().getParameter ("match")->setValueNotifyingHost (0.0f);
        auto* output = plugin->getParameters().getParameter ("output");
        output->setValueNotifyingHost (output->convertTo0to1 (-6.0f));
        check (near (measure (*plugin, 100.0), -6.0 + eq::getResponseDb (plugin->getBand (2), 100.0, testRate), 0.1), "output gain lowers everything by its dB");
    }

    {
        auto plugin = withOnly ({ true, BandType::bell, 1000.0f, 9.0f, 3.0f, 12 });
        measure (*plugin, 1000.0, 0.1);
        std::vector<float> before (8192), after (8192);
        const int n = plugin->readAnalyzer (before.data(), after.data(), 8192);
        const float inPeak = juce::FloatVectorOperations::findMaximum (before.data(), n), outPeak = juce::FloatVectorOperations::findMaximum (after.data(), n);
        check (n >= 4096 && near (inPeak, 0.25, 0.01) && outPeak > inPeak * 2.5f && plugin->readAnalyzer (before.data(), after.data(), 8192) == 0,
               "the spectrum gets the signal before and after the EQ, once");

        // The device bar's small spectrum reads next to the editor: the newest samples stay.
        std::vector<float> preview ((size_t) BuiltInEq::previewSize);
        const auto written = plugin->readPreview (preview.data(), BuiltInEq::previewSize);
        const float previewPeak = juce::FloatVectorOperations::findMaximum (preview.data(), BuiltInEq::previewSize);
        check (written >= n && near (previewPeak, outPeak, 0.02) && plugin->readPreview (preview.data(), 16) == written,
               "the preview keeps the newest signal after the EQ, also once the spectrum has read it");
    }

    {
        BuiltInEq a, b;
        a.setBand (3, { true, BandType::notch, 321.0f, 0.0f, 7.5f, 12 });
        a.setBand (0, { true, BandType::lowCut, 45.0f, 0.0f, 1.2f, 24 });
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto band3 = b.getBand (3), band0 = b.getBand (0);
        check (band3.type == BandType::notch && near (band3.freq, 321.0, 0.5) && near (band3.q, 7.5, 0.01) && band0.on && band0.slope == 24,
               "the settings survive saving and loading the EQ's state");
    }

    {
        const auto& presets = eq::getPresets();
        bool valid = presets.size() >= 8 && presets.front().name == "Flat";

        for (auto& p : presets)
            for (int i = 0; i < eq::numBands; ++i)
            {
                const auto& band = p.bands[(size_t) i];
                valid = valid && band.freq >= eq::minFreq && band.freq <= eq::maxFreq && std::abs (band.gainDb) <= eq::maxGainDb
                        && band.q >= eq::minQ && band.q <= eq::maxQ && (band.on == p.notes[(size_t) i].isNotEmpty() || p.name == "Flat");
            }

        BuiltInEq plugin;
        plugin.applyPreset (presets[1]);
        check (valid && plugin.getBand (0).on && near (plugin.getBand (0).freq, 90.0, 0.5) && ! plugin.getBand (5).on,
               "every starting point is in range and explains each band it uses; Vocal sets a low cut at 90 Hz");
    }

    {
        // The instrument overview: ranges in order and on the graph, a low cut below the notes, every range explained.
        const auto& instruments = eq::getInstruments();
        bool valid = instruments.size() >= 10;

        for (auto& instrument : instruments)
        {
            valid = valid && instrument.name.isNotEmpty() && instrument.summary.isNotEmpty() && ! instrument.zones.empty()
                    && instrument.notesFrom >= eq::minFreq && instrument.notesFrom < instrument.notesTo
                    && instrument.notesTo <= instrument.overtonesTo && instrument.overtonesTo <= eq::maxFreq
                    && instrument.lowCutHz < instrument.notesFrom;
            float previous = 0.0f;

            for (auto& zone : instrument.zones)
            {
                valid = valid && zone.from >= previous && zone.from < zone.to && zone.from >= eq::minFreq && zone.to <= eq::maxFreq
                        && zone.name.isNotEmpty() && zone.text.isNotEmpty();
                previous = zone.from;
            }
        }

        check (valid, "every instrument of the overview has its notes, overtones and ranges in order and explained");
    }

    {
        // Through the engine and a project file, like any plugin.
        const auto folder = juce::File::createTempFile ("eq");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::audio);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInEq::getDescription(), error);
        auto* eqPlugin = slot != nullptr ? dynamic_cast<BuiltInEq*> (slot->instance.get()) : nullptr;
        check (eqPlugin != nullptr && ! slot->isInstrument && eqPlugin->getPluginDescription().pluginFormatName == "DawIt",
               "the engine adds DawIt EQ to a track's chain as an effect " + error);

        if (eqPlugin != nullptr)
        {
            eqPlugin->setBand (2, { true, BandType::bell, 330.0f, -4.5f, 2.0f, 12 });
            const auto file = folder.getChildFile ("Song.dawit");
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = daw::project::save (engine, file, text, error) && daw::project::load (loaded, file, error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInEq*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && near (reloaded->getBand (2).gainDb, -4.5, 0.01) && near (reloaded->getBand (2).freq, 330.0, 0.5),
                   "a project with DawIt EQ opens with its settings, without scanning " + error + warnings.joinIntoString (" "));
        }
    }
}

static void runBuiltInCompressorTests()
{
    using namespace daw;
    using comp::Knob;
    std::cout << "Built-in compressor" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // Plays a steady sine through the compressor and measures the change in level over the last half, in dB.
    auto measure = [] (BuiltInCompressor& plugin, double freq, float amplitude, double seconds = 1.0)
    {
        plugin.prepareToPlay (testRate, testBlock);
        const int total = (int) (seconds * testRate);
        juce::AudioBuffer<float> buffer (2, testBlock);
        juce::MidiBuffer midi;
        double sumIn = 0.0, sumOut = 0.0;

        for (int pos = 0; pos < total; pos += testBlock)
        {
            double in = 0.0;

            for (int i = 0; i < testBlock; ++i)
            {
                const float x = amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * freq * (pos + i) / testRate);
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
                in += (double) x * x;
            }

            plugin.processBlock (buffer, midi);

            if (pos >= total / 2)
            {
                sumIn += in;
                for (int i = 0; i < testBlock; ++i)
                    sumOut += (double) buffer.getSample (0, i) * buffer.getSample (0, i);
            }
        }

        return 10.0 * std::log10 (juce::jmax (1.0e-30, sumOut) / sumIn);
    };

    auto settings = [] (float threshold, float ratio, float knee = 0.0f)
    {
        comp::Settings s;
        s.thresholdDb = threshold;
        s.ratio = ratio;
        s.kneeDb = knee;
        return s;
    };

    {
        const auto hard = settings (-20.0f, 4.0f), soft = settings (-20.0f, 4.0f, 6.0f);
        check (comp::getReductionDb (hard, -25.0) == 0.0 && near (comp::getReductionDb (hard, -12.0), 6.0, 1.0e-9)
                   && near (comp::getReductionDb (soft, -20.0), 0.5625, 1.0e-9) && comp::getReductionDb (soft, -23.5) == 0.0
                   && near (comp::getReductionDb (soft, -17.0), comp::getReductionDb (hard, -17.0), 1.0e-9)
                   && near (comp::getReductionDb (soft, -10.0), 7.5, 1.0e-9) && comp::getReductionDb (settings (-20.0f, 1.0f), 0.0) == 0.0,
               "the curve: 4:1 turns 8 dB over into 2, a 6 dB knee starts 3 dB under the threshold and joins the line 3 dB over it");

        const std::vector<float> steady (1500, -6.0f);
        const float t = comp::findThreshold (soft, steady, 1500.0, 4.0);
        check (near (comp::getReductionDb (settings (t, 4.0f, 6.0f), -6.0), 4.0, 0.001)
                   && near (comp::findThreshold (settings (0.0f, 1.0f), steady, 1500.0, 4.0), comp::minThreshold, 1.0e-6),
               "the threshold for 4 dB of reduction on a steady level is found, and none with a ratio of 1:1");

        // Short hits: 10 ms at -6 dB every half second. A slow attack only catches them with a lower threshold.
        std::vector<float> hits (3000, -40.0f);
        for (size_t i = 0; i < hits.size(); ++i)
            if (i % 750 < 15)
                hits[i] = -6.0f;

        auto slow = soft;
        slow.attackMs = 25.0f;
        const float forHits = comp::findThreshold (slow, hits, 1500.0, 4.0);
        slow.thresholdDb = forHits;
        comp::Ballistics b;
        b.setTimes (slow.attackMs, slow.releaseMs, 1500.0);
        double most = 0.0;
        for (auto db : hits)
            most = juce::jmax (most, b.process (comp::getReductionDb (slow, db)));

        check (forHits < t - 6.0f && near (most, 4.0, 0.01),
               "for short hits and a 25 ms attack the threshold goes " + juce::String (t - forHits, 1) + " dB lower to turn them down by 4 dB");
    }

    {
        comp::Ballistics b;
        b.setTimes (10.0, 100.0, testRate);

        for (int i = 0; i < 480; ++i)
            b.process (10.0);

        const double afterAttack = b.state;

        for (int i = 0; i < 4800; ++i)
            b.process (0.0);

        check (near (afterAttack, 6.32, 0.01) && near (b.state, afterAttack * 0.368, 0.01),
               "after the attack time the reduction has gone 63% of the way, after the release time it has let go 63%");
    }

    {
        BuiltInCompressor plugin;
        auto s = settings (-20.0f, 4.0f);
        s.detection = comp::Detection::rms;
        plugin.setSettings (s);

        // A sine at 0.5 has an RMS of -9.0 dB: 11 dB over, turned down by 11 * 3/4.
        const double steady = measure (plugin, 1000.0, 0.5f);
        check (near (steady, -8.27, 0.3), "a steady tone 11 dB over the threshold at 4:1 is turned down by about 8.3 dB: " + juce::String (steady, 2));

        s.makeupDb = 6.0f;
        plugin.setSettings (s);
        check (near (measure (plugin, 1000.0, 0.02f), 6.0, 0.05), "under the threshold only the makeup changes the level");

        s.makeupDb = 0.0f;
        s.mixPercent = 0.0f;
        plugin.setSettings (s);
        check (near (measure (plugin, 1000.0, 0.5f), 0.0, 0.05), "with the mix at 0% the track is untouched");

        s.mixPercent = 100.0f;
        plugin.setSettings (s);
        plugin.setMatchingLevel (true);
        const double matched = measure (plugin, 1000.0, 0.5f, 2.0);
        check (near (matched, 0.0, 0.3) && near (plugin.matchGainDb.load(), 8.27, 0.4),
               "match level makes the compressed track as loud as it came in: " + juce::String (matched, 2));

        plugin.setMatchingLevel (false);
        plugin.listenToRemoved = true;
        const double removed = measure (plugin, 1000.0, 0.5f);
        plugin.listenToRemoved = false;
        check (near (removed, 20.0 * std::log10 (1.0 - std::pow (10.0, -8.27 / 20.0)), 0.4),
               "hear what it removes plays the difference between the input and the compressed track: " + juce::String (removed, 2));
    }

    {
        // A low tone drives the compressor; with the sidechain filter it hardly notices it.
        BuiltInCompressor plugin;
        auto s = settings (-30.0f, 8.0f);
        s.detection = comp::Detection::rms;
        s.releaseMs = 300.0f;
        plugin.setSettings (s);
        const double unfiltered = measure (plugin, 40.0, 0.5f);
        s.sidechainHz = 400.0f;
        plugin.setSettings (s);
        const double filtered = measure (plugin, 40.0, 0.5f);
        check (unfiltered < -15.0 && filtered > unfiltered + 10.0,
               "the sidechain filter keeps a low tone from driving the compressor: " + juce::String (unfiltered, 1) + " vs " + juce::String (filtered, 1) + " dB");
    }

    {
        // A slow attack lets the start of a hit through; a fast one catches it after its first wave (it can't look ahead).
        auto startVersusLater = [&] (float attackMs)
        {
            BuiltInCompressor plugin;
            auto s = settings (-30.0f, 10.0f);
            s.attackMs = attackMs;
            plugin.setSettings (s);
            plugin.prepareToPlay (testRate, testBlock);
            juce::AudioBuffer<float> buffer (2, 4800);
            juce::MidiBuffer midi;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                for (int ch = 0; ch < 2; ++ch)
                    buffer.setSample (ch, i, 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 1000.0 * i / testRate));

            plugin.processBlock (buffer, midi);
            return juce::Decibels::gainToDecibels (buffer.getMagnitude (0, 96, 192) / buffer.getMagnitude (0, 3840, 960));
        };

        const float slow = startVersusLater (30.0f), fast = startVersusLater (0.1f);
        check (slow > 12.0f && fast < 1.0f,
               "with a 30 ms attack 2-6 ms into a hit it comes through " + juce::String (slow, 1) + " dB louder, with 0.1 ms only "
                   + juce::String (fast, 1) + " dB");

        // 24 dB over at 10:1 is 21.6 dB at the peaks of the wave; between them the release lets go a little.
        BuiltInCompressor plugin;
        plugin.setSettings (settings (-30.0f, 10.0f));
        measure (plugin, 1000.0, 0.5f, 0.5);
        std::vector<BuiltInCompressor::HistoryPoint> points (20000);
        const int n = plugin.readHistory (points.data(), (int) points.size());
        const auto& last = points[(size_t) juce::jmax (0, n - 1)];
        const int blocks = ((int) (0.5 * testRate) + testBlock - 1) / testBlock;
        check (n == blocks * testBlock / BuiltInCompressor::historyChunk && near (last.inputDb, -6.02, 0.1)
                   && near (last.reductionDb, 21.0, 1.2) && near (last.outputDb, last.inputDb - last.reductionDb, 0.5) && plugin.readHistory (points.data(), 10) == 0,
               "the history holds the levels and the reduction of every 32 samples, once: " + juce::String (n) + " points, "
                   + juce::String (last.inputDb, 1) + " in, " + juce::String (last.reductionDb, 1) + " down, " + juce::String (last.outputDb, 1) + " out");

        // The device bar's dot: the loudest level since it last looked, and how far that was turned down.
        plugin.takePreview();
        juce::AudioBuffer<float> buffer (2, testBlock);
        juce::MidiBuffer midi;

        for (int pos = 0; pos < (int) (0.1 * testRate); pos += testBlock)
        {
            for (int i = 0; i < testBlock; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    buffer.setSample (ch, i, 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 1000.0 * (pos + i) / testRate));

            plugin.processBlock (buffer, midi);
        }

        const auto dot = plugin.takePreview();
        check (near (dot.detectorDb, -6.02, 0.2) && near (dot.reductionDb, 21.0, 1.2) && plugin.takePreview().detectorDb <= -100.0f,
               "the preview gives the loudest level since it was taken (" + juce::String (dot.detectorDb, 1) + " dB) and its reduction ("
                   + juce::String (dot.reductionDb, 1) + " dB), once");
    }

    {
        BuiltInCompressor a, b;
        auto s = settings (-27.5f, 6.5f, 9.0f);
        s.attackMs = 3.3f;
        s.sidechainHz = 150.0f;
        s.detection = comp::Detection::rms;
        a.setSettings (s);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto loaded = b.getSettings();
        check (near (loaded.thresholdDb, -27.5, 0.01) && near (loaded.ratio, 6.5, 0.01) && near (loaded.attackMs, 3.3, 0.01)
                   && near (loaded.sidechainHz, 150.0, 0.1) && loaded.detection == comp::Detection::rms,
               "the settings survive saving and loading the compressor's state");

        check (comp::formatKnob (Knob::ratio, s) == "6.5:1" && comp::formatKnob (Knob::attack, s) == "3.3 ms"
                   && comp::formatKnob (Knob::sidechain, comp::Settings()) == "Off" && a.getKnobParameter (Knob::release)->getCurrentValueAsText() == "150 ms"
                   && near (a.getKnobParameter (Knob::release)->getValueForText ("1.5 s"), a.getKnobParameter (Knob::release)->convertTo0to1 (1500.0f), 0.001),
               "the knobs read as 6.5:1, 3.3 ms, Off and 150 ms");
    }

    {
        const auto& presets = comp::getPresets();
        bool valid = presets.size() >= 12 && presets.front().name == "Neutral start";

        for (auto& p : presets)
        {
            const auto& s = p.settings;
            valid = valid && p.why.isNotEmpty() && p.targetReductionDb > 0.0f
                    && s.thresholdDb >= comp::minThreshold && s.thresholdDb <= comp::maxThreshold && s.ratio >= comp::minRatio && s.ratio <= comp::maxRatio
                    && s.attackMs >= comp::minAttack && s.attackMs <= comp::maxAttack && s.releaseMs >= comp::minRelease && s.releaseMs <= comp::maxRelease
                    && s.kneeDb >= 0.0f && s.kneeDb <= comp::maxKnee && s.makeupDb >= comp::minMakeup && s.makeupDb <= comp::maxMakeup
                    && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& note) { return note.isNotEmpty(); });
        }

        BuiltInCompressor plugin;
        const auto kick = std::find_if (presets.begin(), presets.end(), [] (auto& p) { return p.name == "Kick drum"; });
        plugin.setMatchingLevel (true);

        if (kick != presets.end())
            plugin.applyPreset (*kick);

        check (valid && kick != presets.end() && near (plugin.getSettings().attackMs, 25.0, 0.01) && ! plugin.isMatchingLevel(),
               "every starting point is in range and explains each knob; Kick drum lets the click through with a 25 ms attack");
    }

    {
        // Through the engine and a project file, like any plugin.
        const auto folder = juce::File::createTempFile ("compressor");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::audio);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInCompressor::getDescription(), error);
        auto* compressor = slot != nullptr ? dynamic_cast<BuiltInCompressor*> (slot->instance.get()) : nullptr;
        check (compressor != nullptr && ! slot->isInstrument && compressor->getPluginDescription().pluginFormatName == "DawIt",
               "the engine adds DawIt Compressor to a track's chain as an effect " + error);

        if (compressor != nullptr)
        {
            compressor->setKnob (Knob::threshold, -31.0f);
            const auto file = folder.getChildFile ("Song.dawit");
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = daw::project::save (engine, file, text, error) && daw::project::load (loaded, file, error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInCompressor*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && near (reloaded->getSettings().thresholdDb, -31.0, 0.01),
                   "a project with DawIt Compressor opens with its settings, without scanning " + error + warnings.joinIntoString (" "));
        }
    }
}

// Runs a buffer through an effect plugin in blocks; `input` gives each sample (the same on both sides).
template <typename Plugin>
static juce::AudioBuffer<float> runEffect (Plugin& plugin, int numSamples, const std::function<float (int)>& input)
{
    plugin.prepareToPlay (testRate, testBlock);
    juce::AudioBuffer<float> out (2, numSamples), block (2, testBlock);
    juce::MidiBuffer midi;

    for (int pos = 0; pos < numSamples; pos += testBlock)
    {
        const int n = juce::jmin (testBlock, numSamples - pos);
        block.setSize (2, n, false, false, true);

        for (int i = 0; i < n; ++i)
            block.setSample (0, i, input (pos + i)), block.setSample (1, i, input (pos + i));

        plugin.processBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, n);
    }

    return out;
}

static double rmsDb (const juce::AudioBuffer<float>& b, int start, int num)
{
    double sum = 0.0;

    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = start; i < start + num; ++i)
            sum += (double) b.getSample (ch, i) * b.getSample (ch, i);

    return 10.0 * std::log10 (juce::jmax (1.0e-20, sum / (num * b.getNumChannels())));
}

template <typename Plugin, typename PluginTypeCheck>
static void checkThroughProject (const juce::PluginDescription& description, const juce::String& name, const std::function<void (Plugin&)>& change,
                                 const PluginTypeCheck& changed)
{
    using namespace daw;
    const auto folder = juce::File::createTempFile ("effect");
    folder.createDirectory();
    const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* track = engine.addTrack (Track::Type::audio);
    juce::String error, text;
    auto* slot = engine.addPlugin (*track, description, error);
    auto* plugin = slot != nullptr ? dynamic_cast<Plugin*> (slot->instance.get()) : nullptr;
    check (plugin != nullptr && ! slot->isInstrument && plugin->getPluginDescription().pluginFormatName == "DawIt",
           "the engine adds " + name + " to a track's chain as an effect " + error);

    if (plugin == nullptr)
        return;

    change (*plugin);
    const auto file = folder.getChildFile ("Song.dawit");
    Engine loaded;
    loaded.deviceManager.removeAudioCallback (&loaded);
    juce::StringArray warnings;
    const bool ok = daw::project::save (engine, file, text, error) && daw::project::load (loaded, file, error, warnings);
    auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                         ? dynamic_cast<Plugin*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
    check (reloaded != nullptr && warnings.isEmpty() && changed (*reloaded),
           "a project with " + name + " opens with its settings, without scanning " + error + warnings.joinIntoString (" "));
}

#include "ReverbKindsTests.h"
#include "ReverbNetworkTests.h"

static void runBuiltInReverbTests()
{
    using namespace daw;
    using verb::Knob;
    std::cout << "Built-in reverb" << std::endl;

    {
        const auto baseline = verb::renderImpulse ({}, 48000.0, 1.0);
        double energy = 0.0;
        bool pinned = true;
        for (int channel = 0; channel < 2; ++channel)
        {
            int first = -1;
            for (int sample = 0; sample < baseline.getNumSamples(); ++sample)
            {
                const double value = baseline.getSample (channel, sample);
                energy += value * value;
                if (first < 0 && std::abs (value) > 0.0) first = sample;
            }
            pinned = pinned && first == 1356;
        }
        check (pinned && std::abs (energy / 1.319954598505594 - 1.0) < 1.0e-6,
               "Digital retains its original 48 kHz impulse energy and first sample (1356), so old projects sound the same");
    }

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    check (near (verb::getLoopGain (0.05, 1.0), std::pow (10.0, -0.15), 1.0e-9) && near (verb::getRoomMetres (0.0f), 5.0, 1.0e-9)
               && near (verb::getRoomMetres (100.0f), 40.0, 1.0e-9) && near (verb::getSabineDecay (1000.0, 80.0), 2.0125, 1.0e-6)
               && near (fx::getWetToDryDb (50.0f), 0.0, 1.0e-6) && fx::getWetToDryDb (0.0f) <= -100.0,
           "a trip of 50 ms keeps 10^-0.15 of the level for a 1 s decay; the size spans 5 to 40 m; Sabine gives 2.0 s for "
           "1000 m3 and 80 m2 of absorption; 50% mix is as loud wet as dry");

    {
        // At the damping frequency, one trip keeps the loop gain squared.
        const double g = 0.9, rate = 48000.0, hz = 5000.0;
        const double b = verb::getDampingPole (g, hz, rate);
        const std::complex<double> z = std::polar (1.0, -juce::MathConstants<double>::twoPi * hz / rate);
        const double atHz = std::abs (g * (1.0 - b) / (1.0 - b * z));
        check (b > 0.0 && b < 1.0 && near (atHz, g * g, 1.0e-6),
               "the damping filter keeps the loop gain at the lows and its square at the damping frequency (" + juce::String (atHz, 4) + ")");
    }

    {
        // The decay knob is the time to die away by 60 dB, measured like a room.
        bool allNear = true;
        juce::String measured;

        for (const float decay : { 0.5f, 1.5f, 4.0f })
        {
            verb::Settings s;
            s.decaySeconds = decay;
            s.dampingHz = verb::maxDamping;
            s.lowCutHz = verb::minLowCut;
            s.earlyPercent = 0.0f;
            s.predelayMs = 0.0f;
            const auto response = verb::renderImpulse (s, testRate, decay * 1.6 + 0.3);
            const double t = verb::measureDecay (response, testRate);
            measured << juce::String (t, 2) << " s ";
            allNear = allNear && near (t, decay, decay * 0.15);
        }

        check (allNear, "an impulse dies away by 60 dB in the decay time: 0.5, 1.5 and 4 s give " + measured);
    }

    {
        verb::Settings s;
        s.predelayMs = 40.0f;
        s.earlyPercent = 100.0f;
        const auto response = verb::renderImpulse (s, testRate, 0.3);
        const int predelay = (int) (0.04 * testRate);
        const int first = predelay + (int) std::round (verb::getFirstReflectionMs (s.sizePercent) * testRate / 1000.0);
        bool silentBefore = true;

        for (int i = 0; i < predelay; ++i)
            silentBefore = silentBefore && std::abs (response.getSample (0, i)) < 1.0e-6f && std::abs (response.getSample (1, i)) < 1.0e-6f;

        const float atFirst = juce::jmax (response.getMagnitude (0, first, 4), response.getMagnitude (1, first, 4));
        check (silentBefore && atFirst > 0.05f, "nothing comes out during the 40 ms pre-delay, and the first reflection comes "
                                                + juce::String (verb::getFirstReflectionMs (s.sizePercent), 1) + " ms later");
    }

    {
        // Damping: the highs die away sooner than the lows.
        verb::Settings s;
        s.decaySeconds = 2.0f;
        s.dampingHz = 2000.0f;
        s.lowCutHz = verb::minLowCut;
        s.mixPercent = 100.0f;
        auto toneTail = [&] (double hz)
        {
            BuiltInReverb plugin;
            plugin.setSettings (s);
            const int on = (int) (0.6 * testRate), total = (int) (1.4 * testRate);
            const auto out = runEffect (plugin, total, [&] (int i) { return i < on ? 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / testRate) : 0.0f; });
            return rmsDb (out, (int) (1.2 * testRate), (int) (0.1 * testRate)) - rmsDb (out, (int) (0.4 * testRate), (int) (0.1 * testRate));
        };

        const double low = toneTail (300.0), high = toneTail (8000.0);
        check (high < low - 15.0, "with damping at 2 kHz an 8 kHz tone's tail fades much faster than a 300 Hz tone's ("
                                      + juce::String (high, 1) + " against " + juce::String (low, 1) + " dB after 0.6 s)");
    }

    {
        // The low cut keeps the reverb out of the lows.
        auto lowLevel = [&] (float cut)
        {
            verb::Settings s;
            s.lowCutHz = cut;
            s.mixPercent = 100.0f;
            BuiltInReverb plugin;
            plugin.setSettings (s);
            const auto out = runEffect (plugin, (int) testRate, [] (int i) { return 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 50.0 * i / testRate); });
            return rmsDb (out, (int) (0.5 * testRate), (int) (0.5 * testRate));
        };

        const double off = lowLevel (verb::minLowCut), on = lowLevel (400.0f);
        check (on < off - 20.0, "a 400 Hz low cut takes a 50 Hz tone out of the reverb (" + juce::String (on - off, 1) + " dB)");
    }

    {
        // Mix, width and hearing only the reverb.
        auto noise = [] (int i) { return 0.25f * (float) std::sin (i * 0.37) * (float) std::sin (i * 0.0113); };
        BuiltInReverb dry;
        auto s = dry.getSettings();
        s.mixPercent = 0.0f;
        dry.setSettings (s);
        const auto dryOut = runEffect (dry, 8192, noise);
        float most = 0.0f;

        for (int i = 0; i < 8192; ++i)
            most = juce::jmax (most, std::abs (dryOut.getSample (0, i) - noise (i)));

        BuiltInReverb mono;
        s.mixPercent = 100.0f;
        s.widthPercent = 0.0f;
        mono.setSettings (s);
        const auto monoOut = runEffect (mono, 8192, noise);
        float side = 0.0f;

        for (int i = 0; i < 8192; ++i)
            side = juce::jmax (side, std::abs (monoOut.getSample (0, i) - monoOut.getSample (1, i)));

        BuiltInReverb only;
        only.listenToEffect = true;
        const auto onlyOut = runEffect (only, 4, [] (int i) { return i == 0 ? 1.0f : 0.0f; });

        check (most < 1.0e-5f && side < 1.0e-5f && std::abs (onlyOut.getSample (0, 0)) < 1.0e-6f,
               "at 0% mix the sound passes untouched; at 0% width the reverb is the same left and right; hearing only the "
               "reverb leaves out the dry sound");
    }

    {
        // Steady loudness: a long tail is fed less, so it doesn't build up much louder than a short one.
        auto steady = [&] (float decay, float size)
        {
            verb::Settings s;
            s.decaySeconds = decay;
            s.sizePercent = size;
            s.mixPercent = 100.0f;
            s.earlyPercent = 0.0f;
            s.lowCutHz = verb::minLowCut;
            s.dampingHz = verb::maxDamping;
            BuiltInReverb plugin;
            plugin.setSettings (s);
            juce::Random random (3);
            const int total = (int) ((decay * 2.0 + 0.5) * testRate);
            const auto out = runEffect (plugin, total, [&] (int) { return 0.25f * (random.nextFloat() * 2.0f - 1.0f); });
            return rmsDb (out, total / 2, total / 2) - juce::Decibels::gainToDecibels (0.25 / std::sqrt (3.0));
        };

        const double shortTail = steady (0.4f, 20.0f), longTail = steady (12.0f, 100.0f);
        check (std::abs (shortTail) < 6.0 && std::abs (longTail) < 6.0,
               "steady noise comes out of the undamped reverb about as loud as it goes in, with a short or a 12 s tail ("
                   + juce::String (shortTail, 1) + " and " + juce::String (longTail, 1) + " dB)");
    }

    {
        BuiltInReverb plugin;
        plugin.clap.request();
        auto s = plugin.getSettings();
        s.mixPercent = 100.0f;
        plugin.setSettings (s);
        const auto out = runEffect (plugin, (int) testRate, [] (int) { return 0.0f; });
        std::vector<fx::LevelPoint> points (4096);
        const int n = plugin.history.read (points.data(), (int) points.size());
        float mostIn = -100.0f, mostWet = -100.0f;

        for (int i = 0; i < n; ++i)
        {
            mostIn = juce::jmax (mostIn, points[(size_t) i].inputDb);
            mostWet = juce::jmax (mostWet, points[(size_t) i].effectDb);
        }

        check (out.getMagnitude (0, (int) (0.3 * testRate), (int) (0.1 * testRate)) > 0.001f && mostIn > -12.0f && mostWet > -40.0f
                   && n == (int) testRate / fx::LevelHistory::chunk,
               "a clap goes into the reverb and its tail is still heard 0.3 s later; the history has the clap and its reverb");
    }

    {
        BuiltInReverb a, b;
        verb::Settings s;
        s.predelayMs = 33.0f;
        s.decaySeconds = 3.3f;
        s.dampingHz = 4200.0f;
        s.mixPercent = 41.0f;
        a.setSettings (s);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto loaded = b.getSettings();
        check (near (loaded.predelayMs, 33.0, 0.1) && near (loaded.decaySeconds, 3.3, 0.01) && near (loaded.dampingHz, 4200.0, 1.0)
                   && near (loaded.mixPercent, 41.0, 0.01) && near (b.getTailLengthSeconds(), 0.033 + 1.2 * 3.3 + 0.1, 0.01),
               "the reverb's settings survive saving and loading its state, and its tail is as long as its decay");

        check (verb::formatKnob (Knob::decay, s) == "3.30 s" && verb::formatKnob (Knob::damping, s) == "4.2 kHz"
                   && verb::formatKnob (Knob::lowCut, verb::Settings { 0, 0, 1, 20000, 20, 0, 0, 0 }) == "Off"
                   && a.getKnobParameter (Knob::predelay)->getCurrentValueAsText() == "33 ms"
                   && near (a.getKnobParameter (Knob::decay)->getValueForText ("800 ms"), a.getKnobParameter (Knob::decay)->convertTo0to1 (0.8f), 0.001),
               "the reverb's knobs read as 3.30 s, 4.2 kHz, Off and 33 ms, and 800 ms can be typed for the decay");
    }

    {
        const auto& presets = verb::getPresets();
        bool valid = presets.size() >= 10 && presets.front().name == "Neutral start";

        for (auto& p : presets)
        {
            const auto& s = p.settings;
            valid = valid && p.why.isNotEmpty() && p.typeNote.isNotEmpty() && s.predelayMs >= 0.0f && s.predelayMs <= verb::maxPredelay
                    && s.decaySeconds >= verb::minDecay && s.decaySeconds <= verb::maxDecay && s.dampingHz >= verb::minDamping
                    && s.dampingHz <= verb::maxDamping && s.lowCutHz >= verb::minLowCut && s.lowCutHz <= verb::maxLowCut
                    && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& note) { return note.isNotEmpty(); });
        }

        check (valid, "every reverb starting point is in range and explains each knob");
    }

    checkThroughProject<BuiltInReverb> (BuiltInReverb::getDescription(), "DawIt Reverb",
                                        [] (BuiltInReverb& r) { r.setKnob (Knob::decay, 5.5f); r.setType (verb::Type::spring); },
                                        [&] (BuiltInReverb& r) { return near (r.getSettings().decaySeconds, 5.5, 0.01) && r.getSettings().type == verb::Type::spring; });
}

static void runBuiltInDelayTests()
{
    using namespace daw;
    using dly::Knob;
    std::cout << "Built-in delay" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    dly::Settings eighth;
    check (near (dly::getNoteMs (dly::quarterNote, 120.0), 500.0, 1.0e-9) && near (dly::getDelayMs (eighth, 120.0), 250.0, 1.0e-9)
               && near (dly::getDelayMs (eighth, 0.0), 250.0, 1.0e-9) && dly::getAudibleRepeats (0.0f) == 1
               && dly::getAudibleRepeats (50.0f) == 10 && near (dly::getRepeatDb (50.0f, 3), -12.04, 0.01)
               && near (dly::getWallMetres (100.0), 17.15, 1.0e-9) && dly::findNearestNote (370.0, 120.0) == 7,
           "a quarter note at 120 BPM is 500 ms, an eighth 250 ms (also while the tempo is unknown); 50% feedback gives 10 "
           "repeats above -60 dB, the third 12 dB down; a 100 ms echo is a wall 17 m away; 370 ms is nearest a dotted 1/8");

    // Echoes of a click: where they land and how loud they are.
    auto clickEchoes = [] (const dly::Settings& s)
    {
        BuiltInDelay plugin;
        plugin.setSettings (s);
        plugin.listenToEffect = true;
        return runEffect (plugin, (int) testRate, [] (int i) { return i == 0 ? 1.0f : 0.0f; });
    };

    {
        dly::Settings s;
        s.sync = false;
        s.timeMs = 100.0f;
        s.feedbackPercent = 50.0f;
        s.highCutHz = dly::maxHighCut;
        s.lowCutHz = dly::minLowCut;
        const auto out = clickEchoes (s);
        const int d = (int) (0.1 * testRate);
        const float first = out.getSample (0, d), second = out.getSample (0, 2 * d), before = out.getMagnitude (0, 0, d - 1);
        check (near (first, 1.0, 0.01) && near (second, 0.5, 0.01) && before < 1.0e-6f,
               "with 100 ms and 50% feedback, a click comes back after 100 ms and again at half the level after 200 ms ("
                   + juce::String (first, 3) + ", " + juce::String (second, 3) + ", " + juce::String (before, 3) + ")");
    }

    {
        dly::Settings s;
        s.sync = false;
        s.timeMs = 50.0f;
        s.feedbackPercent = 60.0f;
        s.mode = dly::Mode::pingPong;
        s.highCutHz = dly::maxHighCut;
        s.lowCutHz = dly::minLowCut;
        const auto out = clickEchoes (s);
        const int d = (int) (0.05 * testRate);
        check (out.getSample (0, d) > 0.4f && std::abs (out.getSample (1, d)) < 1.0e-6f
                   && out.getSample (1, 2 * d) > 0.2f && std::abs (out.getSample (0, 2 * d)) < 1.0e-6f,
               "ping-pong: the first echo comes from the left only, the second from the right only");
    }

    {
        // The filters darken every repeat a little more.
        dly::Settings s;
        s.sync = false;
        s.timeMs = 200.0f;
        s.feedbackPercent = 80.0f;
        s.highCutHz = 2000.0f;
        s.lowCutHz = dly::minLowCut;
        auto echoLevel = [&] (double hz, int n)
        {
            BuiltInDelay plugin;
            plugin.setSettings (s);
            plugin.listenToEffect = true;
            const int burst = (int) (0.1 * testRate), d = (int) (0.2 * testRate);
            const auto out = runEffect (plugin, (int) (1.0 * testRate), [&] (int i) { return i < burst ? 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / testRate) : 0.0f; });
            return rmsDb (out, n * d + burst / 4, burst / 2);
        };

        const double lowDrop = echoLevel (200.0, 1) - echoLevel (200.0, 3), highDrop = echoLevel (6000.0, 1) - echoLevel (6000.0, 3);
        const double expected = -dly::getFilteredDb (s, 6000.0, 2, testRate) - 2.0 * dly::getRepeatDb (s.feedbackPercent, 2);
        check (near (lowDrop, 3.9, 0.6) && near (highDrop, expected, 1.5),
               "from the first echo to the third, 200 Hz loses only the feedback (" + juce::String (lowDrop, 1) + " dB) and 6 kHz also "
                   + "two trips through the 2 kHz high cut (" + juce::String (highDrop, 1) + " dB, expected " + juce::String (expected, 1) + ")");
    }

    {
        // Synced to the song's tempo through the play head.
        struct Head final : public juce::AudioPlayHead
        {
            juce::Optional<PositionInfo> getPosition() const override
            {
                PositionInfo info;
                info.setBpm (90.0);
                return info;
            }
        } head;

        BuiltInDelay plugin;
        plugin.setPlayHead (&head);
        auto s = plugin.getSettings();
        s.sync = true;
        s.note = dly::quarterNote;
        s.feedbackPercent = 0.0f;
        plugin.setSettings (s);
        plugin.listenToEffect = true;
        const auto out = runEffect (plugin, (int) testRate, [] (int i) { return i == 0 ? 1.0f : 0.0f; });
        const int expected = (int) std::round (60.0 / 90.0 * testRate);
        int loudest = 0;

        for (int i = 1; i < out.getNumSamples(); ++i)
            if (std::abs (out.getSample (0, i)) > std::abs (out.getSample (0, loudest)))
                loudest = i;

        check (std::abs (loudest - expected) <= 2 && near (plugin.tempoBpm.load(), 90.0, 1.0e-9) && near (plugin.getDelayMs(), 666.67, 0.01),
               "synced to a 1/4 note at the song's 90 BPM, the echo comes after 667 ms (" + juce::String (loudest) + " " + juce::String (plugin.getDelayMs()) + ")");
    }

    {
        // Stable at the most feedback, and the clap reaches the history.
        BuiltInDelay plugin;
        auto s = plugin.getSettings();
        s.feedbackPercent = dly::maxFeedback;
        s.highCutHz = dly::maxHighCut;
        s.lowCutHz = dly::minLowCut;
        s.mixPercent = 100.0f;
        plugin.setSettings (s);
        plugin.clap.request();
        const auto out = runEffect (plugin, (int) (4.0 * testRate), [] (int) { return 0.0f; });
        std::vector<fx::LevelPoint> points (8192);
        const int n = plugin.history.read (points.data(), (int) points.size());
        check (out.getMagnitude (0, out.getNumSamples() - 12000, 12000) < 0.5f && out.getMagnitude (0, out.getNumSamples() - 12000, 12000) > 0.001f
                   && n > 0 && near (plugin.getTailLengthSeconds(), 0.25 * dly::getAudibleRepeats (95.0f) + 0.1, 0.01),
               "at 95% feedback a clap keeps echoing for seconds without growing, and the tail lasts all its repeats ("
                   + juce::String (out.getMagnitude (0, out.getNumSamples() - 12000, 12000), 3) + ", " + juce::String (n) + ", "
                   + juce::String (plugin.getTailLengthSeconds(), 2) + " s)");
    }

    {
        BuiltInDelay a, b;
        dly::Settings s;
        s.sync = false;
        s.timeMs = 432.0f;
        s.note = 9;
        s.feedbackPercent = 61.0f;
        s.mode = dly::Mode::pingPong;
        a.setSettings (s);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto loaded = b.getSettings();
        check (! loaded.sync && near (loaded.timeMs, 432.0, 0.1) && loaded.note == 9 && near (loaded.feedbackPercent, 61.0, 0.01)
                   && loaded.mode == dly::Mode::pingPong,
               "the delay's settings survive saving and loading its state");

        s.sync = true;
        check (dly::formatKnob (Knob::time, s) == "dotted 1/4" && dly::formatKnob (Knob::mode, s) == "Ping-pong"
                   && a.getKnobParameter (Knob::time)->getCurrentValueAsText() == "432 ms"
                   && near (a.getKnobParameter (Knob::time)->getValueForText ("1.5 s"), a.getKnobParameter (Knob::time)->convertTo0to1 (1500.0f), 0.001),
               "the delay's controls read as dotted 1/4, Ping-pong and 432 ms, and 1.5 s can be typed");
    }

    {
        const auto& presets = dly::getPresets();
        bool valid = presets.size() >= 8 && presets.front().name == "Neutral start";

        for (auto& p : presets)
        {
            const auto& s = p.settings;
            valid = valid && p.why.isNotEmpty() && s.timeMs >= dly::minTime && s.timeMs <= dly::maxTime && s.feedbackPercent <= dly::maxFeedback
                    && s.note >= 0 && s.note < dly::numNoteValues && s.highCutHz >= dly::minHighCut && s.lowCutHz <= dly::maxLowCut
                    && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& note) { return note.isNotEmpty(); });
        }

        check (valid, "every delay starting point is in range and explains each control");
    }

    checkThroughProject<BuiltInDelay> (BuiltInDelay::getDescription(), "DawIt Delay",
                                       [] (BuiltInDelay& d) { d.setKnob (Knob::feedback, 77.0f); d.setSync (false); },
                                       [&] (BuiltInDelay& d) { return near (d.getSettings().feedbackPercent, 77.0, 0.01) && ! d.getSettings().sync; });
}

static void runBuiltInSaturationTests()
{
    using namespace daw;
    using sat::Knob;
    using sat::Type;
    std::cout << "Built-in saturation" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    {
        bool unity = true, tapeSymmetric = true;

        for (int t = 0; t < sat::numTypes; ++t)
            unity = unity && near (sat::shape ((Type) t, 1.0e-4f) / 1.0e-4f, 1.0, 0.001);

        for (float x : { 0.3f, 1.0f, 3.0f })
            tapeSymmetric = tapeSymmetric && near (sat::shape (Type::tape, -x), -sat::shape (Type::tape, x), 1.0e-6);

        check (unity && tapeSymmetric && sat::shape (Type::tube, 3.0f) < -sat::shape (Type::tube, -3.0f) && near (sat::shape (Type::clip, 5.0f), 1.0, 1.0e-6),
               "every curve passes small values unchanged; tape is symmetric, tube rounds its top off sooner than its bottom, clip stops at 1");
    }

    {
        sat::Settings s;
        s.driveDb = 12.0f;
        const auto tape = sat::getHarmonicsDb (s, 0.5f, 8);
        s.type = Type::tube;
        const auto tube = sat::getHarmonicsDb (s, 0.5f, 8);
        s.type = Type::clip;
        s.driveDb = 36.0f;
        const auto clip = sat::getHarmonicsDb (s, 0.5f, 8);
        s.driveDb = 0.0f;
        s.type = Type::tape;
        const auto gentle = sat::getHarmonicsDb (s, 0.1f, 8);

        // A square wave's harmonics: the odd ones at 1/n.
        check (tape[1] < -100.0f && tape[2] > -30.0f && tube[1] > -30.0f && near (clip[2] - clip[0], -9.54, 0.3) && near (clip[4] - clip[0], -13.98, 0.4)
                   && sat::getThdPercent (gentle) < 0.5f && sat::getThdPercent (tape) > 3.0f,
               "tape adds only odd harmonics, tube even ones too, hard clipping at full drive nears a square wave (3rd "
                   + juce::String (clip[2] - clip[0], 1) + " dB, 5th " + juce::String (clip[4] - clip[0], 1) + " dB), and a quiet sine "
                   + "barely changes (THD " + juce::String (sat::getThdPercent (gentle), 2) + "% against " + juce::String (sat::getThdPercent (tape), 1) + "%)");
        check (sat::getHarmonicInterval (2) == "octave" && sat::getHarmonicInterval (3) == "octave + 5th", "the harmonics are named as intervals");
    }

    auto sine = [] (double hz, float amplitude) { return [hz, amplitude] (int i) { return amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / testRate); }; };

    // The level of one frequency in the second half of a buffer, as a gain (a Hann-windowed DFT).
    auto levelAt = [] (const juce::AudioBuffer<float>& b, double hz)
    {
        const int start = b.getNumSamples() / 2, n = b.getNumSamples() - start;
        double re = 0.0, im = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / n);
            const double a = juce::MathConstants<double>::twoPi * hz * i / testRate;
            re += w * b.getSample (0, start + i) * std::cos (a);
            im -= w * b.getSample (0, start + i) * std::sin (a);
        }

        return 4.0 * std::sqrt (re * re + im * im) / n;
    };

    {
        // Match level keeps it as loud; without it, the drive makes a quiet sound much louder.
        BuiltInSaturation plugin;
        auto s = plugin.getSettings();
        s.driveDb = 24.0f;
        plugin.setSettings (s);
        const int total = (int) testRate;
        const auto matched = runEffect (plugin, total, sine (440.0, 0.1f));
        plugin.setMatchingLevel (false);
        const auto loud = runEffect (plugin, total, sine (440.0, 0.1f));
        const double inDb = juce::Decibels::gainToDecibels (0.1 / std::sqrt (2.0));
        const double matchedDb = rmsDb (matched, total / 2, total / 2), loudDb = rmsDb (loud, total / 2, total / 2);
        check (near (matchedDb, inDb, 1.0) && loudDb > inDb + 12.0,
               "with match level, 24 dB of drive comes out as loud as it went in (" + juce::String (matchedDb - inDb, 1) + " dB); without, "
                   + juce::String (loudDb - inDb, 1) + " dB louder");
    }

    {
        // The dry and saturated sound line up despite the oversampling filters: half and half doesn't cancel anything.
        BuiltInSaturation plugin;
        auto s = plugin.getSettings();
        s.driveDb = 0.0f;
        s.mixPercent = 50.0f;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        const int total = (int) (0.5 * testRate);
        const auto out = runEffect (plugin, total, sine (12000.0, 0.05f));
        const double kept = levelAt (out, 12000.0);
        check (plugin.getLatencySamples() > 0 && near (kept, 0.05, 0.003),
               "the dry sound is held back as long as the oversampling (" + juce::String (plugin.getLatencySamples())
                   + " samples), so a 50% mix keeps a 12 kHz tone whole (" + juce::String (kept, 4) + ")");
    }

    {
        // Hear only what it adds: nothing of the note itself, only its harmonics.
        BuiltInSaturation plugin;
        auto s = plugin.getSettings();
        s.driveDb = 18.0f;
        s.type = Type::tube;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        plugin.listenToEffect = true;
        const auto out = runEffect (plugin, (int) testRate, sine (220.0, 0.3f));
        const double note = levelAt (out, 220.0), octave = levelAt (out, 440.0), fifth = levelAt (out, 660.0);
        check (note < 0.2 * octave && octave > 0.01 && fifth > 0.01,
               "hearing only what it adds leaves out the note (" + juce::String (note, 4) + ") and keeps its harmonics (octave "
                   + juce::String (octave, 3) + ", octave + fifth " + juce::String (fifth, 3) + ")");
    }

    {
        // Aliasing: at 48 kHz the 5th harmonic of 9.1 kHz (45.5 kHz) can't be stored and folds back down to 2.5 kHz.
        sat::Settings s;
        s.driveDb = 24.0f;
        const double hz = 9100.0;
        juce::AudioBuffer<float> naive (1, (int) testRate);

        for (int i = 0; i < naive.getNumSamples(); ++i)
            naive.setSample (0, i, sat::shape (s.type, juce::Decibels::decibelsToGain (s.driveDb) * sine (hz, 0.5f) (i)));

        BuiltInSaturation plugin;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        const auto out = runEffect (plugin, (int) testRate, sine (hz, 0.5f));
        const double alias = std::abs (5.0 * hz - testRate);
        const double naiveDb = juce::Decibels::gainToDecibels (levelAt (naive, alias) / levelAt (naive, hz));
        const double ovDb = juce::Decibels::gainToDecibels (levelAt (out, alias) / levelAt (out, hz));
        check (naiveDb > -40.0 && ovDb < naiveDb - 20.0,
               "oversampling keeps the 5th harmonic of 9.1 kHz from folding back to " + juce::String (alias / 1000.0, 1) + " kHz ("
                   + juce::String (ovDb, 1) + " dB, against " + juce::String (naiveDb, 1) + " dB without)");
    }

    {
        BuiltInSaturation plugin;
        plugin.clap.request (fx::Clap::Sound::note);
        const auto out = runEffect (plugin, (int) (2.0 * testRate), [] (int) { return 0.0f; });
        std::vector<fx::LevelPoint> points (4096);
        const int n = plugin.history.read (points.data(), (int) points.size());
        float added = -100.0f;

        for (int i = 0; i < n; ++i)
            added = juce::jmax (added, points[(size_t) i].effectDb);

        check (out.getMagnitude (0, (int) (1.0 * testRate), (int) (0.2 * testRate)) > 0.01f && out.getMagnitude (0, (int) (1.6 * testRate), 4800) < 1.0e-4f
                   && added > -40.0f && plugin.recentPeak.load() > 0.0f,
               "the test note plays for about a second and a half, and the history shows what the saturation added to it");
    }

    {
        BuiltInSaturation a, b;
        sat::Settings s;
        s.driveDb = 21.5f;
        s.type = Type::clip;
        s.toneHz = 3300.0f;
        s.mixPercent = 40.0f;
        a.setSettings (s);
        a.setMatchingLevel (false);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto loaded = b.getSettings();
        check (near (loaded.driveDb, 21.5, 0.01) && loaded.type == Type::clip && near (loaded.toneHz, 3300.0, 1.0) && near (loaded.mixPercent, 40.0, 0.01)
                   && ! b.isMatchingLevel(),
               "the saturation's settings survive saving and loading its state");
        check (sat::formatKnob (Knob::drive, s) == "+21.5 dB" && sat::formatKnob (Knob::tone, s) == "3.3 kHz" && sat::formatKnob (Knob::type, s) == "Clip"
                   && sat::formatKnob (Knob::tone, sat::Settings()) == "Off",
               "the saturation's knobs read as +21.5 dB, 3.3 kHz, Clip and Off");
    }

    {
        const auto& presets = sat::getPresets();
        bool valid = presets.size() >= 8 && presets.front().name == "Neutral start";

        for (auto& p : presets)
        {
            const auto& s = p.settings;
            valid = valid && p.why.isNotEmpty() && s.driveDb >= 0.0f && s.driveDb <= sat::maxDrive && s.toneHz >= sat::minTone && s.toneHz <= sat::maxTone
                    && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& note) { return note.isNotEmpty(); });
        }

        check (valid, "every saturation starting point is in range and explains each knob");
    }

    checkThroughProject<BuiltInSaturation> (BuiltInSaturation::getDescription(), "DawIt Saturation",
                                            [] (BuiltInSaturation& p) { p.setKnob (Knob::drive, 27.0f); },
                                            [&] (BuiltInSaturation& p) { return near (p.getSettings().driveDb, 27.0, 0.01); });
}

// Plays notes through DawIt Subtractive, Additive, FM, Drums or Physical: `events` are (seconds, note, velocity), a velocity of 0 lets the note go.
template <typename Synth>
static juce::AudioBuffer<float> playSynth (Synth& synth, double seconds, std::vector<std::tuple<double, int, float>> events)
{
    synth.prepareToPlay (testRate, testBlock);
    const int total = (int) (seconds * testRate);
    juce::AudioBuffer<float> out (2, total), block (2, testBlock);

    for (int pos = 0; pos < total; pos += testBlock)
    {
        const int n = juce::jmin (testBlock, total - pos);
        block.setSize (2, n, false, false, true);
        juce::MidiBuffer midi;

        for (auto& [time, note, velocity] : events)
        {
            const int at = (int) (time * testRate) - pos;

            if (at >= 0 && at < n)
                midi.addEvent (velocity > 0.0f ? juce::MidiMessage::noteOn (1, note, velocity) : juce::MidiMessage::noteOff (1, note), at);
        }

        synth.processBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, n);
    }

    return out;
}

// The level of one frequency in a stretch of the left channel, as the amplitude of a sine (Goertzel).
static double levelAt (const juce::AudioBuffer<float>& b, double hz, int start, int num)
{
    const double w = juce::MathConstants<double>::twoPi * hz / testRate;
    double re = 0.0, im = 0.0;

    for (int i = 0; i < num; ++i)
    {
        // A Hann window, so nearby harmonics don't leak in.
        const double window = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (num - 1));
        re += b.getSample (0, start + i) * window * std::cos (w * i);
        im -= b.getSample (0, start + i) * window * std::sin (w * i);
    }

    return 4.0 * std::sqrt (re * re + im * im) / num;
}

static double levelDb (const juce::AudioBuffer<float>& b, double hz, int start, int num)
{
    return juce::Decibels::gainToDecibels (levelAt (b, hz, start, num), -150.0);
}

static void runBuiltInSubtractiveTests()
{
    using namespace daw;
    using sub::Knob;
    std::cout << "DawIt Subtractive" << std::endl;
    const double pi = juce::MathConstants<double>::pi;

    // The waves' harmonics.
    check (std::abs (std::abs (sub::getHarmonic (sub::Wave::saw, 50.0f, 1)) - 2.0 / pi) < 1.0e-9
               && std::abs (std::abs (sub::getHarmonic (sub::Wave::saw, 50.0f, 4)) - 2.0 / (4.0 * pi)) < 1.0e-9,
           "a saw has every harmonic at 1/n");
    check (std::abs (sub::getHarmonic (sub::Wave::square, 50.0f, 2)) < 1.0e-9 && std::abs (std::abs (sub::getHarmonic (sub::Wave::square, 50.0f, 3)) - 4.0 / (3.0 * pi)) < 1.0e-9,
           "a square has only the odd harmonics, at 4 / (pi n)");
    check (std::abs (sub::getHarmonic (sub::Wave::square, 25.0f, 4)) < 1.0e-9 && std::abs (sub::getHarmonic (sub::Wave::square, 25.0f, 2)) > 0.1,
           "a 25% pulse misses every 4th harmonic but has the 2nd");
    check (std::abs (std::abs (sub::getHarmonic (sub::Wave::triangle, 50.0f, 3)) - 8.0 / (9.0 * pi * pi)) < 1.0e-9
               && std::abs (sub::getHarmonic (sub::Wave::triangle, 50.0f, 2)) < 1.0e-12
               && std::abs (sub::getHarmonic (sub::Wave::sine, 50.0f, 1) - 1.0) < 1.0e-12 && std::abs (sub::getHarmonic (sub::Wave::sine, 50.0f, 2)) < 1.0e-12,
           "a triangle has odd harmonics at 1/n squared, a sine only the first");

    // The partials add up to the wave: a saw rises from -1 to 1.
    {
        sub::Settings saw;
        const auto partials = sub::getPartials (saw, 100.0, 20000.0, 20000.0);
        const auto wave = sub::getWave (partials, 0.01, 101, false);
        check (std::abs (wave[25] - (-0.5f)) < 0.03f && std::abs (wave[75] - 0.5f) < 0.03f && std::abs (wave[50]) < 0.03f,
               "the partials of a saw add up to a rising ramp");
    }

    // The filter.
    using FT = sub::FilterType;
    check (std::abs (sub::getQ (0.0f) - 0.7071f) < 1.0e-4f && std::abs (sub::getQ (100.0f) - 20.0f) < 1.0e-3f, "resonance runs from Q 0.71 to 20");
    check (std::abs (sub::getFilterDb (FT::lowPass, false, 1000.0, 0.0f, 1000.0) + 3.01) < 0.02
               && std::abs (sub::getFilterDb (FT::lowPass, true, 1000.0, 0.0f, 1000.0) + 6.02) < 0.02
               && std::abs (sub::getFilterDb (FT::highPass, false, 1000.0, 0.0f, 1000.0) + 3.01) < 0.02
               && std::abs (sub::getFilterDb (FT::bandPass, false, 1000.0, 0.0f, 1000.0)) < 0.02,
           "at the cutoff: -3 dB for 12 dB/oct, -6 dB for 24 dB/oct, and the band-pass's peak at 0 dB");
    check (std::abs (sub::getFilterDb (FT::lowPass, false, 1000.0, 0.0f, 4000.0) + 24.1) < 0.2
               && std::abs (sub::getFilterDb (FT::lowPass, true, 1000.0, 0.0f, 4000.0) + 48.2) < 0.4
               && sub::getFilterDb (FT::lowPass, true, 1000.0, 0.0f, 100.0) > -0.01,
           "two octaves past the cutoff: about -24 dB at 12 dB/oct and -48 dB at 24 dB/oct, the lows untouched");
    check (std::abs (sub::getFilterDb (FT::lowPass, false, 1000.0, 100.0f, 1000.0) - 26.02) < 0.05, "full resonance peaks at +26 dB at the cutoff");

    // Where the filter goes.
    {
        sub::Settings s;
        s.cutoffHz = 300.0f;
        s.envOctaves = 3.0f;
        s.velocityPercent = 100.0f;
        check (std::abs (sub::getCutoffAt (s, 60, 1.0f, 1.0f, 0.0f) - 2400.0) < 0.01 && std::abs (sub::getCutoffAt (s, 60, 1.0f, 0.5f, 0.0f) - 300.0 * std::pow (2.0, 1.5)) < 0.01
                   && std::abs (sub::getCutoffAt (s, 60, 0.5f, 1.0f, 0.0f) - 300.0 * std::pow (2.0, 1.5)) < 0.01,
               "the filter envelope opens the cutoff by its amount in octaves, half as far for a note played half as hard");
        s.envOctaves = 0.0f;
        s.keyTrackPercent = 100.0f;
        check (std::abs (sub::getCutoffAt (s, 72, 1.0f, 0.0f, 0.0f) - 600.0) < 0.01 && std::abs (sub::getCutoffAt (s, 60, 1.0f, 0.0f, 1.0f) - 600.0) < 0.01
                   && juce::exactlyEqual (sub::getCutoffAt (s, 127, 1.0f, 0.0f, 5.0f), 20000.0),
               "key tracking moves the cutoff an octave per octave, the LFO by its octaves, and it stops at 20 kHz");
    }

    {
        const sub::Envelope e { 0.1f, 0.2f, 0.5f, 0.4f };
        check (std::abs (sub::getEnvelopeLevel (e, 0.05) - 0.5f) < 1.0e-5f && std::abs (sub::getEnvelopeLevel (e, 0.2) - 0.75f) < 1.0e-5f
                   && std::abs (sub::getEnvelopeLevel (e, 1.0) - 0.5f) < 1.0e-5f && std::abs (sub::getEnvelopeLevel (e, 1.2, 1.0) - 0.25f) < 1.0e-5f
                   && juce::exactlyEqual (sub::getEnvelopeLevel (e, 2.0, 1.0), 0.0f),
               "an envelope rises in the attack, falls to the sustain in the decay, holds, and fades out in the release");
    }

    check (std::abs (sub::getBeatHz (220.0, 7.0f) - 0.893) < 0.002, "two oscillators 7 cents apart at A3 beat 0.89 times a second");

    // The sound itself.
    const int at = (int) (0.3 * testRate), length = 8192;
    BuiltInSubtractive synth;

    auto out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    int crossings = 0;
    for (int i = at + 1; i < at + (int) testRate / 10; ++i)
        crossings += out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f ? 1 : 0;

    const float peak = out.getMagnitude (0, at, length);
    check (crossings >= 21 && crossings <= 23 && std::abs (juce::Decibels::gainToDecibels (peak) - sub::sawPeakDb) < 1.0f
               && juce::exactlyEqual (out.getSample (1, at + 100), out.getSample (0, at + 100)),
           "Init plays A3 at 220 Hz, peaking near -12 dB on both sides (" + juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB)");

    const double openFifth = levelDb (out, 1100.0, at, length) - levelDb (out, 220.0, at, length);
    check (std::abs (openFifth + 14.0) < 1.5, "with the filter open, the saw's 5th harmonic is 14 dB under the 1st (" + juce::String (openFifth, 1) + " dB)");

    auto s = synth.getSettings();
    s.cutoffHz = 300.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double closedFifth = levelDb (out, 1100.0, at, length) - levelDb (out, 220.0, at, length);
    const double expectedFifth = -14.0 + sub::getFilterDb (sub::FilterType::lowPass, true, 300.0, 0.0f, 1100.0) - sub::getFilterDb (sub::FilterType::lowPass, true, 300.0, 0.0f, 220.0);
    check (std::abs (closedFifth - expectedFifth) < 2.0, "a 24 dB low-pass at 300 Hz takes the 5th harmonic down to " + juce::String (closedFifth, 1)
                                                              + " dB, as its curve says (" + juce::String (expectedFifth, 1) + ")");

    synth.withoutFilter = true;
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (std::abs (levelDb (out, 1100.0, at, length) - levelDb (out, 220.0, at, length) - openFifth) < 1.0, "without the filter, the harmonics are back");
    synth.withoutFilter = false;

    // The amp envelope.
    s = {};
    s.ampEnv = { 0.1f, 0.1f, 1.0f, 0.05f };
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f }, { 0.3, 57, 0.0f } });
    const float early = out.getMagnitude (0, 0, (int) (0.01 * testRate)), full = out.getMagnitude (0, (int) (0.15 * testRate), 2400);
    const float afterRelease = out.getMagnitude (0, (int) (0.36 * testRate), 2400);
    check (early < full * 0.15f && full > 0.2f && afterRelease < 1.0e-4f, "a note rises in its attack and is gone after its release");

    // Poly and mono.
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.0, 61, 1.0f }, { 0.0, 64, 1.0f } });
    const int chordAt = (int) (0.25 * testRate);
    check (levelAt (out, 220.0, chordAt, length) > 0.05 && levelAt (out, sub::getNoteHz (61), chordAt, length) > 0.05
               && levelAt (out, sub::getNoteHz (64), chordAt, length) > 0.05,
           "poly plays the three notes of a chord");

    s.mode = sub::Mode::mono;
    synth.setSettings (s);
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.2, 64, 1.0f } });
    const int legatoAt = (int) (0.3 * testRate);
    check (levelAt (out, 220.0, legatoAt, length) < 0.01 && levelAt (out, sub::getNoteHz (64), legatoAt, length) > 0.1
               && out.getMagnitude (0, (int) (0.2 * testRate), 480) > 0.2f,
           "mono plays one note at a time: the new note takes over without starting from silence");

    s.glideSeconds = 0.3f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 45, 1.0f }, { 0.2, 57, 1.0f } });
    int midCrossings = 0;
    for (int i = (int) (0.25 * testRate); i < (int) (0.3 * testRate); ++i)
        midCrossings += out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f ? 1 : 0;
    check (midCrossings > 6 && midCrossings < 10, "glide slides between the notes: 50 ms after the change it's between 110 and 220 Hz ("
                                                      + juce::String (midCrossings * 20) + " Hz)");

    // The window's test note.
    synth.setSettings ({});
    synth.clap.request (fx::Clap::Sound::note);
    out = playSynth (synth, 1.2, {});
    check (out.getMagnitude (0, (int) (0.4 * testRate), 2400) > 0.1f && out.getMagnitude (0, (int) (0.75 * testRate), 2400) > 0.1f
               && out.getMagnitude (0, (int) (1.05 * testRate), 2400) < 1.0e-3f,
           "Play a note plays A3 for 0.8 seconds, then its release");

    // The starting points.
    bool allFine = true;
    juce::String worst;
    for (auto& preset : sub::getPresets())
    {
        synth.setSettings (preset.settings);
        const auto again = synth.getSettings();
        const auto played = playSynth (synth, 2.0, { { 0.0, 48, 1.0f }, { 0.0, 55, 1.0f }, { 0.0, 64, 1.0f }, { 1.2, 48, 0.0f }, { 1.2, 55, 0.0f }, { 1.2, 64, 0.0f } });
        const float loudest = played.getMagnitude (0, 0, played.getNumSamples());
        const bool fine = loudest > 0.02f && loudest < 1.0f && std::abs (again.cutoffHz - preset.settings.cutoffHz) < 1.0f
                          && again.mode == preset.settings.mode && preset.why.isNotEmpty();
        if (! fine)
            worst << preset.name << " (" << juce::String (loudest, 2) << ") ";
        allFine = allFine && fine;
    }
    check (allFine && sub::getPresets().size() >= 10, "every starting point keeps its settings and plays a chord without clipping " + worst);

    check (sub::formatKnob (Knob::cutoff, s) == "20.0 kHz" && sub::formatKnob (Knob::pitch2, [] { sub::Settings x; x.pitch2 = 7.0f; return x; }()) == "+7 st"
               && sub::formatKnob (Knob::ampAttack, [] { sub::Settings x; x.ampEnv.attack = 0.35f; return x; }()) == "350 ms"
               && sub::formatKnob (Knob::envAmount, [] { sub::Settings x; x.envOctaves = 3.0f; return x; }()) == "+3.0 oct",
           "the knobs show their values the way they're read");

    // An instrument in a track's chain, saved with the project.
    {
        const auto folder = juce::File::createTempFile ("subtractive");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::midi);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInSubtractive::getDescription(), error, 0);
        auto* plugin = slot != nullptr ? dynamic_cast<BuiltInSubtractive*> (slot->instance.get()) : nullptr;
        check (plugin != nullptr && slot->isInstrument && track->hasInstrument() && ! track->usesBuiltInSynth(),
               "the engine adds DawIt Subtractive to a MIDI track as its instrument " + error);

        if (plugin != nullptr)
        {
            plugin->setSettings (sub::getPresets()[2].settings);
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = project::save (engine, folder.getChildFile ("Song.dawit"), text, error) && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInSubtractive*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && std::abs (reloaded->getSettings().resonancePercent - 75.0f) < 0.5f
                       && reloaded->getSettings().mode == sub::Mode::mono,
                   "a project with DawIt Subtractive opens with its sound, without scanning " + error + warnings.joinIntoString (" "));
        }
    }
}

static void runBuiltInAdditiveTests()
{
    using namespace daw;
    using additive::Knob;
    using additive::Shape;
    std::cout << "DawIt Additive" << std::endl;
    const double pi = juce::MathConstants<double>::pi;
    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // The shapes the bars can be filled with.
    const auto saw = additive::getShapeLevels (Shape::saw), square = additive::getShapeLevels (Shape::square);
    const auto triangle = additive::getShapeLevels (Shape::triangle), organ = additive::getShapeLevels (Shape::organ);
    check (near (saw[0], 0.0, 1.0e-4) && near (saw[1], -6.02, 0.01) && near (saw[2], -9.54, 0.01) && near (saw[31], -30.1, 0.05),
           "a saw's harmonics are at 1/n: -6 dB at the 2nd, -9.5 at the 3rd, -30 at the 32nd");
    check (juce::exactlyEqual (square[1], additive::offDb) && near (square[2], -9.54, 0.01) && near (triangle[2], -19.08, 0.01)
               && juce::exactlyEqual (triangle[3], additive::offDb),
           "a square has the odd harmonics at 1/n, a triangle at 1/n squared, neither the even ones");
    check (near (organ[3], 0.0, 1.0e-4) && juce::exactlyEqual (organ[4], additive::offDb), "the organ fills the 1st to 4th harmonics, as four drawbars");

    // Where the partials are.
    check (near (additive::getRatio (5, 0.0f), 5.0, 1.0e-12) && near (additive::getRatio (1, 100.0f), 1.0, 1.0e-12)
               && near (additive::getStretchCents (8, 100.0f), 194.0, 2.0)
               && additive::getStretchCents (16, 50.0f) > additive::getStretchCents (8, 50.0f) && additive::getStretchCents (8, 50.0f) > additive::getStretchCents (4, 50.0f)
               && additive::getStretchCents (4, 50.0f) > 0.0,
           "without stretch the partials are whole-number multiples; stretch pulls the higher ones sharper (the 8th 194 cents at 100%) and leaves the 1st");

    // How loud each one is.
    {
        additive::Settings flat;
        flat.levelsDb.fill (0.0f);
        flat.brightnessDb = -6.0f;
        check (near (additive::getPartialGain (flat, 4), 0.25, 0.002) && near (additive::getPartialGain (flat, 8), 0.125, 0.002),
               "a brightness of -6 dB an octave on equal bars gives a saw's 1/n");

        flat.brightnessDb = 0.0f;
        flat.evenPercent = 0.0f;
        check (juce::exactlyEqual (additive::getPartialGain (flat, 2), 0.0) && near (additive::getPartialGain (flat, 3), 1.0, 1.0e-9),
               "with the even harmonics at 0%, only the odd ones are left");

        flat.evenPercent = 100.0f;
        flat.velocityPercent = 100.0f;
        check (near (additive::getVelocityScale (flat, 0.5f), 0.5, 1.0e-6) && near (additive::getPartialGain (flat, 2, 0.5f), std::pow (10.0, -3.0 / 20.0), 1.0e-6),
               "at 100% velocity, a note played half as hard is 6 dB quieter with its harmonics 3 dB an octave darker");

        flat.lfoTarget = additive::LfoTarget::even;
        flat.lfoDepthPercent = 100.0f;
        check (near (additive::getPartialGain (flat, 2, 1.0f, -1.0f), 0.0, 1.0e-9) && near (additive::getPartialGain (flat, 2, 1.0f, 1.0f), 1.0, 1.0e-9),
               "the LFO on the even harmonics takes them all the way out and back");

        additive::Settings two;
        two.levelsDb[1] = 0.0f;
        check (near (additive::getNormalisation ({}), 1.0, 1.0e-9) && near (additive::getNormalisation (two), 1.0 / std::sqrt (2.0), 1.0e-9),
               "the sum is turned down to the power of one sine: two harmonics at full by 3 dB, one sine not at all");
    }

    // Harmonics that move.
    {
        additive::Settings s;
        s.swellSeconds = 3.1f;
        check (near (additive::getPartialEnvelope (s, 2, 0.05), 0.5, 1.0e-6) && near (additive::getPartialEnvelope (s, 32, 1.55), 0.5, 1.0e-6)
                   && near (additive::getPartialEnvelope (s, 1, 0.0), 1.0, 1.0e-12) && near (additive::getPartialEnvelope (s, 32, 4.0), 1.0, 1.0e-12),
               "the swell brings the 32nd harmonic in over its whole time and the 2nd in 1/31 of it");

        s.swellSeconds = 0.0f;
        s.fadeSeconds = 1.0f;
        check (near (additive::getPartialEnvelope (s, 2, 1.0), 0.5, 1.0e-9) && near (additive::getPartialEnvelope (s, 3, 1.0), 0.25, 1.0e-9)
                   && near (additive::getPartialEnvelope (s, 1, 5.0), 1.0, 1.0e-12),
               "the fade halves the 2nd harmonic in its time, the 3rd twice as fast, and leaves the 1st");

        s.fadeSeconds = additive::maxFade;
        check (near (additive::getPartialEnvelope (s, 32, 10.0), 1.0, 1.0e-12) && additive::formatKnob (Knob::fade, s) == "Off", "the fade at its top is off");
    }

    // The partials add up to the wave, and their phases change its peaks but not their levels.
    {
        additive::Settings s;
        s.levelsDb = saw;
        const double norm = additive::getNormalisation (s);
        const auto ramp = additive::getWave (additive::getPartials (s, 1.0), 1.0, 101);
        check (near (ramp[25], norm * pi / 4.0, norm * 0.03) && near (ramp[75], -norm * pi / 4.0, norm * 0.03),
               "a saw's harmonics, aligned, add up to its ramp");

        s.levelsDb.fill (0.0f);
        auto peakOf = [] (const additive::Settings& x)
        {
            float peak = 0.0f;
            for (auto v : additive::getWave (additive::getPartials (x, 1.0), 1.0, 4000))
                peak = juce::jmax (peak, std::abs (v));
            return peak;
        };
        const float aligned = peakOf (s);
        s.phases = additive::Phases::spread;
        const float spread = peakOf (s);
        check (spread < aligned * 0.35f, "spread phases keep 32 equal harmonics' peak low (" + juce::String (spread / aligned, 2) + " of aligned)");

        // Clearly lower for the starting points that spread them, whose harmonics are many and about as loud, and never higher for any.
        juce::String higher;
        for (auto& preset : additive::getPresets())
        {
            auto x = preset.settings;
            x.phases = additive::Phases::spread;
            const float spreadPeak = peakOf (x);
            x.phases = additive::Phases::aligned;
            const float alignedPeak = peakOf (x);

            if (spreadPeak > alignedPeak * (preset.settings.phases == additive::Phases::spread ? 0.9f : 1.001f))
                higher << preset.name << " (" << juce::String (spreadPeak / alignedPeak, 2) << ") ";
        }
        check (higher.isEmpty(), "spread phases peak at least 10% lower for the starting points that use them, and never higher " + higher);
    }

    // The sound itself.
    const int at = (int) (0.3 * testRate), length = 8192;
    BuiltInAdditive synth;

    auto out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    int crossings = 0;
    for (int i = at + 1; i < at + (int) testRate / 10; ++i)
        crossings += out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f ? 1 : 0;

    const float peak = out.getMagnitude (0, at, length);
    check (crossings >= 21 && crossings <= 23 && std::abs (juce::Decibels::gainToDecibels (peak) - additive::sinePeakDb) < 0.5f
               && levelDb (out, 440.0, at, length) - levelDb (out, 220.0, at, length) < -60.0
               && juce::exactlyEqual (out.getSample (1, at + 100), out.getSample (0, at + 100)),
           "Init plays A3 as one sine at 220 Hz, peaking near -12 dB on both sides (" + juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB)");

    additive::Settings s;
    s.levelsDb = saw;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double fifth = levelDb (out, 1100.0, at, length) - levelDb (out, 220.0, at, length);
    const double sawFirst = levelDb (out, 220.0, at, length);
    check (near (fifth, -13.98, 0.5), "a saw from sines has its 5th harmonic 14 dB under the 1st (" + juce::String (fifth, 1) + " dB)");

    synth.onlyFundamental = true;
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (levelDb (out, 440.0, at, length) - levelDb (out, 220.0, at, length) < -60.0 && near (levelDb (out, 220.0, at, length), sawFirst, 0.2),
           "Hear only the 1st harmonic leaves the 1st at the level it has in the sound and nothing else");
    synth.onlyFundamental = false;

    s.evenPercent = 0.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (levelDb (out, 440.0, at, length) - levelDb (out, 220.0, at, length) < -60.0 && near (levelDb (out, 660.0, at, length) - levelDb (out, 220.0, at, length), -9.54, 0.5),
           "with the even harmonics at 0%, the 2nd is gone and the 3rd stays at -9.5 dB");

    s = {};
    s.levelsDb.fill (0.0f);
    s.brightnessDb = -6.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double fourth = levelDb (out, 880.0, at, length) - levelDb (out, 220.0, at, length);
    check (near (fourth, -12.04, 0.5), "a brightness of -6 dB an octave puts the 4th harmonic 12 dB under the 1st (" + juce::String (fourth, 1) + " dB)");

    // Stretch: the 8th partial alone moves from 1760 Hz to where the formula says.
    s = {};
    s.levelsDb.fill (additive::offDb);
    s.levelsDb[7] = 0.0f;
    s.stretchPercent = 100.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double stretchedHz = 220.0 * additive::getRatio (8, 100.0f);
    check (levelAt (out, stretchedHz, at, length) > 0.1 && levelAt (out, 1760.0, at, length) < 0.01,
           "with full stretch the 8th partial sounds at " + juce::String (stretchedHz, 0) + " Hz, not 1760");

    // The fade, measured: two halvings of the 2nd harmonic in 0.4 seconds is -12 dB.
    s = {};
    s.levelsDb[1] = 0.0f;
    s.fadeSeconds = 0.2f;
    synth.setSettings (s);
    out = playSynth (synth, 0.8, { { 0.0, 57, 1.0f } });
    const int early = (int) (0.1 * testRate), late = (int) (0.5 * testRate), window = 2048;
    const double drop = levelDb (out, 440.0, late, window) - levelDb (out, 440.0, early, window);
    const double firstDrop = levelDb (out, 220.0, late, window) - levelDb (out, 220.0, early, window);
    check (near (drop, -12.04, 1.0) && near (firstDrop, 0.0, 0.3), "the fade takes the 2nd harmonic down 12 dB in two halvings and leaves the 1st ("
                                                                     + juce::String (drop, 1) + " dB)");

    // A partial above what the sample rate can hold is left out, so nothing folds back.
    s = {};
    s.levelsDb.fill (additive::offDb);
    s.levelsDb[8] = 0.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.3, { { 0.0, 100, 1.0f } });
    check (out.getMagnitude (0, 0, out.getNumSamples()) < 1.0e-6f, "the 9th harmonic of a note at 2637 Hz (23.7 kHz) is left out, so nothing aliases");

    // The amp envelope and velocity.
    s = {};
    s.ampEnv = { 0.1f, 0.1f, 1.0f, 0.05f };
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f }, { 0.3, 57, 0.0f } });
    const float start = out.getMagnitude (0, 0, (int) (0.01 * testRate)), full = out.getMagnitude (0, (int) (0.15 * testRate), 2400);
    const float afterRelease = out.getMagnitude (0, (int) (0.36 * testRate), 2400);
    check (start < full * 0.15f && full > 0.2f && afterRelease < 1.0e-4f, "a note rises in its attack and is gone after its release");

    s = {};
    s.velocityPercent = 100.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 0.5f } });
    check (near (juce::Decibels::gainToDecibels (out.getMagnitude (0, at, length)), additive::sinePeakDb - 6.02, 0.5),
           "at 100% velocity a note played half as hard is 6 dB quieter");

    // Poly and mono.
    s = {};
    synth.setSettings (s);
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.0, 61, 1.0f }, { 0.0, 64, 1.0f } });
    const int chordAt = (int) (0.25 * testRate);
    check (levelAt (out, 220.0, chordAt, length) > 0.05 && levelAt (out, sub::getNoteHz (61), chordAt, length) > 0.05
               && levelAt (out, sub::getNoteHz (64), chordAt, length) > 0.05,
           "poly plays the three notes of a chord");

    s.mode = sub::Mode::mono;
    synth.setSettings (s);
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.2, 64, 1.0f } });
    const int legatoAt = (int) (0.3 * testRate);
    check (levelAt (out, 220.0, legatoAt, length) < 0.01 && levelAt (out, sub::getNoteHz (64), legatoAt, length) > 0.1
               && out.getMagnitude (0, (int) (0.2 * testRate), 480) > 0.2f,
           "mono plays one note at a time: the new note takes over without starting from silence");

    s.glideSeconds = 0.3f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 45, 1.0f }, { 0.2, 57, 1.0f } });
    int midCrossings = 0;
    for (int i = (int) (0.25 * testRate); i < (int) (0.3 * testRate); ++i)
        midCrossings += out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f ? 1 : 0;
    check (midCrossings > 6 && midCrossings < 10, "glide slides between the notes: 50 ms after the change it's between 110 and 220 Hz ("
                                                      + juce::String (midCrossings * 20) + " Hz)");

    // The window's test note.
    synth.setSettings ({});
    synth.clap.request (fx::Clap::Sound::note);
    out = playSynth (synth, 1.2, {});
    check (out.getMagnitude (0, (int) (0.4 * testRate), 2400) > 0.1f && out.getMagnitude (0, (int) (0.75 * testRate), 2400) > 0.1f
               && out.getMagnitude (0, (int) (1.05 * testRate), 2400) < 1.0e-3f,
           "Play a note plays A3 for 0.8 seconds, then its release");

    // The starting points.
    bool allFine = true;
    juce::String worst;
    for (auto& preset : additive::getPresets())
    {
        synth.setSettings (preset.settings);
        const auto again = synth.getSettings();
        const auto played = playSynth (synth, 2.0, { { 0.0, 48, 1.0f }, { 0.0, 55, 1.0f }, { 0.0, 64, 1.0f }, { 1.2, 48, 0.0f }, { 1.2, 55, 0.0f }, { 1.2, 64, 0.0f } });
        const float loudest = played.getMagnitude (0, 0, played.getNumSamples());
        bool sameBars = true;

        for (size_t n = 0; n < again.levelsDb.size(); ++n)
            sameBars = sameBars && near (again.levelsDb[n], preset.settings.levelsDb[n], 0.06);

        const bool fine = loudest > 0.02f && loudest < 1.0f && sameBars && again.mode == preset.settings.mode
                          && near (again.stretchPercent, preset.settings.stretchPercent, 0.5) && preset.why.isNotEmpty() && preset.partials.isNotEmpty();
        if (! fine)
            worst << preset.name << " (" << juce::String (loudest, 2) << ") ";
        allFine = allFine && fine;
    }
    check (allFine && additive::getPresets().size() >= 10, "every starting point keeps its harmonics and settings and plays a chord without clipping " + worst);

    check (additive::formatKnob (Knob::brightness, [] { additive::Settings x; x.brightnessDb = -3.0f; return x; }()) == "-3.0 dB/oct"
               && additive::formatKnob (Knob::swell, {}) == "Off" && additive::formatLevel (additive::offDb) == "Off" && additive::formatLevel (-14.0f) == "-14.0 dB"
               && additive::formatKnob (Knob::phases, {}) == "Aligned" && additive::getOrdinal (1) == "1st" && additive::getOrdinal (12) == "12th"
               && additive::getOrdinal (22) == "22nd" && additive::getOrdinal (23) == "23rd",
           "the knobs and bars show their values the way they're read");

    // An instrument in a track's chain, saved with the project.
    {
        const auto folder = juce::File::createTempFile ("additive");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::midi);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInAdditive::getDescription(), error, 0);
        auto* plugin = slot != nullptr ? dynamic_cast<BuiltInAdditive*> (slot->instance.get()) : nullptr;
        check (plugin != nullptr && slot->isInstrument && track->hasInstrument() && ! track->usesBuiltInSynth(),
               "the engine adds DawIt Additive to a MIDI track as its instrument " + error);

        if (plugin != nullptr)
        {
            plugin->setSettings (additive::getPresets()[6].settings);
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = project::save (engine, folder.getChildFile ("Song.dawit"), text, error) && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInAdditive*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && near (reloaded->getSettings().stretchPercent, 100.0, 0.5)
                       && near (reloaded->getSettings().levelsDb[9], -24.0, 0.06) && juce::exactlyEqual (reloaded->getSettings().levelsDb[8], additive::offDb),
                   "a project with DawIt Additive opens with its harmonics and sound, without scanning " + error + warnings.joinIntoString (" "));
        }
    }
}

static void runBuiltInFmTests()
{
    using namespace daw;
    using fm::Knob;
    std::cout << "DawIt FM" << std::endl;
    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // Ratios and the algorithms' wiring.
    fm::Operator bellModulator;
    bellModulator.coarse = 1;
    bellModulator.finePercent = 40.0f;
    check (near (fm::getCoarseRatio (0), 0.5, 1.0e-12) && near (fm::getCoarseRatio (14), 14.0, 1.0e-12) && near (fm::getRatio (bellModulator), 1.4, 1.0e-9),
           "a ratio is coarse (0.5, then 1 to 16) times 1 plus the fine percentage, as on a DX7");

    bool wiredRight = true;
    for (int a = 0; a < fm::numAlgorithms; ++a)
    {
        fm::Settings x;
        x.algorithm = a;
        bool anyCarrier = false;

        for (int op = 0; op < fm::numOperators; ++op)
        {
            const bool carrier = fm::isCarrier (x, op);
            const int targets = fm::getTargets (x, op);
            anyCarrier = anyCarrier || carrier;
            wiredRight = wiredRight && (carrier ? targets == 0 : targets != 0) && (targets & ((1 << (op + 1)) - 1)) == targets;
        }

        wiredRight = wiredRight && anyCarrier && fm::getAlgorithm (a).name.isNotEmpty();
    }
    {
        fm::Settings pairs;
        pairs.algorithm = 4;
        check (wiredRight && fm::isCarrier (pairs, 0) && fm::isCarrier (pairs, 2) && ! fm::isCarrier (pairs, 1) && fm::getTargets (pairs, 3) == 4,
               "every algorithm has a carrier, every other operator modulates only lower ones, and Two pairs hears 1 and 3");
    }

    // The theory: Bessel functions and Carson's rule.
    double power = 0.0;
    for (int k = -20; k <= 20; ++k)
        power += std::pow (fm::getBessel (k, 3.0), 2.0);
    check (near (fm::getBessel (0, 0.0), 1.0, 1.0e-9) && near (fm::getBessel (1, 1.0), 0.4401, 1.0e-4) && near (fm::getBessel (0, 2.405), 0.0, 1.0e-3)
               && near (fm::getBessel (-1, 2.0), -fm::getBessel (1, 2.0), 1.0e-12) && near (power, 1.0, 1.0e-6)
               && near (fm::getCarsonHz (2.0, 220.0), 1320.0, 1.0e-9),
           "the Bessel functions: J0(0) = 1, J1(1) = 0.440, the carrier gone at an index of 2.405, the sidebands' power adding up to 1; Carson's 2 (I + 1) fm");

    // The spectrum measured from the sound matches the Bessel functions.
    auto at = [] (const std::vector<fm::Peak>& peaks, double hz)
    {
        double amplitude = 0.0;
        for (auto& p : peaks)
            if (std::abs (p.hz - hz) < hz * 0.01)
                amplitude = juce::jmax (amplitude, p.amplitude);
        return amplitude;
    };

    std::array<float, fm::numOperators> full;
    full.fill (1.0f);
    juce::String mismatch;

    for (const int modulator : { 1, 2, 3 })
    {
        fm::Settings x;
        x.ops[1].coarse = modulator;
        x.ops[1].levelPercent = 20.0f;
        const auto measured = fm::getSpectrum (x, 220.0, full);
        const auto expected = fm::getSidebands (1.0, modulator, 2.0);

        for (auto& p : expected)
        {
            if (p.hz * 220.0 > 11000.0 || p.amplitude < 0.003)
                continue;

            const double difference = juce::Decibels::gainToDecibels (at (measured, p.hz * 220.0), -200.0) - juce::Decibels::gainToDecibels (p.amplitude);

            if (std::abs (difference) > 0.3)
                mismatch << "1:" << modulator << " at " << juce::String (p.hz, 1) << " (" << juce::String (difference, 1) << " dB) ";
        }
    }
    check (mismatch.isEmpty(), "the spectrum of 1:1, 1:2 and 1:3 at an index of 2 has the sidebands the Bessel functions give, within 0.3 dB " + mismatch);

    {
        fm::Settings x;
        x.ops[1].coarse = 2;
        x.ops[1].levelPercent = 30.0f;
        const auto peaks = fm::getSpectrum (x, 220.0, full);
        bool onlyOdd = ! peaks.empty();
        for (auto& p : peaks)
        {
            const int n = juce::roundToInt (p.hz / 220.0);
            onlyOdd = onlyOdd && (n % 2 == 1 || p.amplitude < 1.0e-3) && near (p.hz, n * 220.0, 1.0);
        }
        check (onlyOdd, "a modulator at twice the carrier makes only odd harmonics, a hollow sound");

        const auto& bell = fm::getPresets()[4].settings;
        const auto bellPeaks = fm::getSpectrum (bell, 220.0, full);
        bool between = false;
        for (auto& p : bellPeaks)
            between = between || (p.amplitude > 0.01 && std::abs (p.hz / 220.0 - std::round (p.hz / 220.0)) > 0.15);
        check (between && fm::getPresets()[4].name.contains ("bell"), "Chowning's bell at 1 : 1.4 has loud partials between the harmonics");
    }

    fm::Settings four;
    four.algorithm = 7;
    for (auto& o : four.ops)
        o.levelPercent = 100.0f;
    check (near (fm::getNormalisation (four), 0.5, 1.0e-9) && near (fm::getNormalisation ({}), 1.0, 1.0e-9)
               && near (fm::getIndex ([] { fm::Settings x; x.ops[1].levelPercent = 25.0f; return x; }(), 1), 2.5, 1.0e-9),
           "four carriers at full are turned down to one sine's power, and a modulator at 25% has an index of 2.5");

    // The sound itself.
    const int start = (int) (0.3 * testRate), length = 8192;
    BuiltInFm synth;

    auto out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const float peak = out.getMagnitude (0, start, length);
    check (std::abs (juce::Decibels::gainToDecibels (peak) - fm::sinePeakDb) < 0.5f && levelDb (out, 440.0, start, length) - levelDb (out, 220.0, start, length) < -60.0
               && juce::exactlyEqual (out.getSample (1, start + 100), out.getSample (0, start + 100)),
           "Init plays A3 as one sine at 220 Hz, peaking near -12 dB on both sides (" + juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB)");

    fm::Settings s;
    s.ops[1].coarse = 2;
    s.ops[1].levelPercent = 20.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double third = levelDb (out, 660.0, start, length) - levelDb (out, 220.0, start, length);
    const auto sidebands = fm::getSidebands (1.0, 2.0, 2.0);
    const double expectedThird = juce::Decibels::gainToDecibels (at (sidebands, 3.0) / at (sidebands, 1.0));
    check (near (third, expectedThird, 0.5) && levelDb (out, 440.0, start, length) - levelDb (out, 220.0, start, length) < -60.0,
           "played at 1 : 2 and an index of 2, the 3rd harmonic is where the Bessel functions put it (" + juce::String (third, 1) + " dB, expected "
               + juce::String (expectedThird, 1) + ") and there's no 2nd");

    synth.withoutModulators = true;
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (levelDb (out, 660.0, start, length) - levelDb (out, 220.0, start, length) < -60.0, "Hear the carriers alone leaves a plain sine");
    synth.withoutModulators = false;

    // At 1 : 3 no sideband lands on the carrier, so it vanishes at an index of 2.405, and the sound's power stays the same.
    s = {};
    s.ops[1].coarse = 3;
    s.ops[1].levelPercent = 24.05f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double rmsModulated = out.getRMSLevel (0, start, length);
    check (levelDb (out, 220.0, start, length) - levelDb (out, 880.0, start, length) < -30.0,
           "at an index of 2.405 the carrier's own frequency is gone and the sidebands carry the sound");

    s.ops[1].levelPercent = 0.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (near (juce::Decibels::gainToDecibels (rmsModulated / out.getRMSLevel (0, start, length)), 0.0, 0.2),
           "modulation moves the power into sidebands without making the sound louder");

    // Feedback turns operator 4's sine towards a saw.
    s = {};
    s.algorithm = 7;
    s.ops[0].levelPercent = 0.0f;
    s.ops[3].levelPercent = 100.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double plainSecond = levelDb (out, 440.0, start, length) - levelDb (out, 220.0, start, length);
    s.feedbackPercent = 100.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double fedSecond = levelDb (out, 440.0, start, length) - levelDb (out, 220.0, start, length);
    const double fedFourth = levelDb (out, 880.0, start, length) - levelDb (out, 220.0, start, length);
    check (plainSecond < -60.0 && fedSecond > -15.0 && fedSecond < 0.0 && fedFourth < fedSecond,
           "feedback adds harmonics falling away like a saw's (the 2nd at " + juce::String (fedSecond, 1) + " dB, the 4th " + juce::String (fedFourth, 1) + ")");

    // The modulator's envelope: bright at the start, a sine once it has decayed.
    s = {};
    s.ops[1].levelPercent = 20.0f;
    s.ops[1].env = { 0.001f, 0.2f, 0.0f, 0.1f };
    synth.setSettings (s);
    out = playSynth (synth, 0.8, { { 0.0, 57, 1.0f } });
    const int window = 2048;
    const double early = levelDb (out, 660.0, (int) (0.01 * testRate), window) - levelDb (out, 220.0, (int) (0.01 * testRate), window);
    const double late = levelDb (out, 660.0, (int) (0.5 * testRate), window) - levelDb (out, 220.0, (int) (0.5 * testRate), window);
    check (early > -20.0 && late < -60.0, "a modulator's envelope makes the note bright at the start and pure once it has decayed ("
                                              + juce::String (early, 1) + " dB, then " + juce::String (late, 1) + ")");

    // A partial above half the rate is filtered out inside, not folded back.
    s = {};
    s.ops[0].coarse = 16;
    synth.setSettings (s);
    out = playSynth (synth, 0.4, { { 0.0, 93, 1.0f } });
    const double folded = juce::Decibels::gainToDecibels (out.getMagnitude (0, (int) (0.1 * testRate), 4800), -150.0f);
    check (folded < fm::sinePeakDb - 40.0, "a carrier at 28 kHz (16 times A6) is filtered out at twice the rate instead of folding back to 20 kHz ("
                                               + juce::String (folded, 1) + " dB)");

    // The amp side: velocity, poly, mono and glide.
    s = {};
    s.velocityPercent = 100.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 0.5f } });
    check (near (juce::Decibels::gainToDecibels (out.getMagnitude (0, start, length)), fm::sinePeakDb - 6.02, 0.5),
           "at 100% velocity a note played half as hard is 6 dB quieter");

    s = {};
    s.ops[0].env = { 0.1f, 0.1f, 1.0f, 0.05f };
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f }, { 0.3, 57, 0.0f } });
    check (out.getMagnitude (0, 0, (int) (0.01 * testRate)) < 0.05f && out.getMagnitude (0, (int) (0.15 * testRate), 2400) > 0.2f
               && out.getMagnitude (0, (int) (0.36 * testRate), 2400) < 1.0e-4f,
           "the carrier's envelope is the note's: it rises in its attack and is gone after its release");

    s = {};
    synth.setSettings (s);
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.0, 61, 1.0f }, { 0.0, 64, 1.0f } });
    const int chordAt = (int) (0.25 * testRate);
    check (levelAt (out, 220.0, chordAt, length) > 0.05 && levelAt (out, sub::getNoteHz (61), chordAt, length) > 0.05
               && levelAt (out, sub::getNoteHz (64), chordAt, length) > 0.05,
           "poly plays the three notes of a chord");

    s.mode = sub::Mode::mono;
    synth.setSettings (s);
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.2, 64, 1.0f } });
    const int legatoAt = (int) (0.3 * testRate);
    check (levelAt (out, 220.0, legatoAt, length) < 0.01 && levelAt (out, sub::getNoteHz (64), legatoAt, length) > 0.1
               && out.getMagnitude (0, (int) (0.2 * testRate), 480) > 0.2f,
           "mono plays one note at a time: the new note takes over without starting from silence");

    s.glideSeconds = 0.3f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 45, 1.0f }, { 0.2, 57, 1.0f } });
    int crossings = 0;
    for (int i = (int) (0.25 * testRate); i < (int) (0.3 * testRate); ++i)
        crossings += out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f ? 1 : 0;
    check (crossings > 6 && crossings < 10, "glide slides between the notes: 50 ms after the change it's between 110 and 220 Hz ("
                                                + juce::String (crossings * 20) + " Hz)");

    // The window's test note.
    synth.setSettings ({});
    synth.clap.request (fx::Clap::Sound::note);
    out = playSynth (synth, 1.2, {});
    check (out.getMagnitude (0, (int) (0.4 * testRate), 2400) > 0.1f && out.getMagnitude (0, (int) (0.75 * testRate), 2400) > 0.1f
               && out.getMagnitude (0, (int) (1.05 * testRate), 2400) < 1.0e-3f,
           "Play a note plays A3 for 0.8 seconds, then its release");

    // The starting points.
    bool allFine = true;
    juce::String worst;
    for (auto& preset : fm::getPresets())
    {
        synth.setSettings (preset.settings);
        const auto again = synth.getSettings();
        const auto played = playSynth (synth, 2.0, { { 0.0, 48, 1.0f }, { 0.0, 55, 1.0f }, { 0.0, 64, 1.0f }, { 1.2, 48, 0.0f }, { 1.2, 55, 0.0f }, { 1.2, 64, 0.0f } });
        const float loudest = played.getMagnitude (0, 0, played.getNumSamples());
        bool same = again.algorithm == preset.settings.algorithm && again.mode == preset.settings.mode
                    && near (again.feedbackPercent, preset.settings.feedbackPercent, 0.5) && preset.why.isNotEmpty();

        for (int op = 0; op < fm::numOperators; ++op)
        {
            const auto& o = again.ops[(size_t) op];
            const auto& expected = preset.settings.ops[(size_t) op];
            same = same && o.coarse == expected.coarse && near (o.finePercent, expected.finePercent, 0.5) && near (o.levelPercent, expected.levelPercent, 0.3)
                   && near (o.env.decay, expected.env.decay, expected.env.decay * 0.01) && (expected.levelPercent < 0.5f || preset.operators[(size_t) op].isNotEmpty());
        }

        const bool fine = loudest > 0.02f && loudest < 1.0f && same;
        if (! fine)
            worst << preset.name << " (" << juce::String (loudest, 2) << ") ";
        allFine = allFine && fine;
    }
    check (allFine && fm::getPresets().size() >= 10, "every starting point keeps its operators and settings, tells what each one does, and plays a chord without clipping " + worst);

    {
        fm::Settings x;
        x.ops[1].coarse = 14;
        x.ops[1].finePercent = 41.0f;
        x.ops[1].env.attack = 0.35f;
        check (fm::formatKnob (Knob::coarse, x, 1) == juce::String (juce::CharPointer_UTF8 ("\xc3\x97")) + "14" && fm::formatKnob (Knob::fine, x, 1) == "+41%"
                   && fm::formatKnob (Knob::attack, x, 1) == "350 ms" && fm::formatKnob (Knob::feedback, x) == "Off" && fm::formatKnob (Knob::algorithm, x) == "Stack"
                   && fm::formatRatio (1.41) == "1.41" && fm::formatRatio (0.5) == "0.50" && fm::formatRatio (3.0) == "3"
                   && BuiltInFm::getParameterId (Knob::level, 2) == "op3Level",
               "the knobs show their values the way they're read");
    }

    // An instrument in a track's chain, saved with the project.
    {
        const auto folder = juce::File::createTempFile ("fm");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::midi);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInFm::getDescription(), error, 0);
        auto* plugin = slot != nullptr ? dynamic_cast<BuiltInFm*> (slot->instance.get()) : nullptr;
        check (plugin != nullptr && slot->isInstrument && track->hasInstrument() && ! track->usesBuiltInSynth(),
               "the engine adds DawIt FM to a MIDI track as its instrument " + error);

        if (plugin != nullptr)
        {
            plugin->setSettings (fm::getPresets()[2].settings);
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = project::save (engine, folder.getChildFile ("Song.dawit"), text, error) && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInFm*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && reloaded->getSettings().algorithm == 4 && reloaded->getSettings().ops[3].coarse == 14
                       && near (reloaded->getSettings().ops[3].levelPercent, 32.0, 0.3),
                   "a project with DawIt FM opens with its algorithm and operators, without scanning " + error + warnings.joinIntoString (" "));

            // Choosing the DawIt Synth from the instrument menu takes the instrument plugin out; the track plays its own sound again.
            engine.setBuiltInSound (*track, BuiltInSound::bass);
            engine.removePlugin (*track, slot);
            check (track->usesBuiltInSynth() && ! track->hasInstrument() && track->builtInSound.load() == BuiltInSound::bass,
                   "taking the instrument plugin out hands the track back to the DawIt Synth, with its sound kept");
        }
    }
}

// The power per hertz of a stretch of samples between two frequencies, in dB, for comparing bands of noise.
static double bandDensityDb (const std::vector<float>& samples, double rate, double lowHz, double highHz)
{
    constexpr int order = 13, size = 1 << order;
    std::vector<float> data ((size_t) size * 2, 0.0f);

    for (int i = 0; i < size && i < (int) samples.size(); ++i)
        data[(size_t) i] = samples[(size_t) i] * (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (size - 1)));

    juce::dsp::FFT (order).performFrequencyOnlyForwardTransform (data.data());
    double power = 0.0;
    int bins = 0;

    for (int bin = (int) (lowHz / rate * size); bin <= (int) (highHz / rate * size) && bin < size / 2; ++bin, ++bins)
        power += (double) data[(size_t) bin] * data[(size_t) bin];

    return 10.0 * std::log10 (juce::jmax (1.0e-30, power / juce::jmax (1, bins)));
}

static void runBuiltInDrumsTests()
{
    using namespace daw;
    using drums::Drum;
    using drums::Knob;
    std::cout << "DawIt Drums" << std::endl;
    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };
    auto toBuffer = [] (const std::vector<float>& v)
    {
        juce::AudioBuffer<float> b (1, (int) v.size());
        b.copyFrom (0, 0, v.data(), (int) v.size());
        return b;
    };
    auto sum = [] (const drums::Hit& h)
    {
        std::vector<float> all (h.tone.size());
        for (size_t i = 0; i < all.size(); ++i)
            all[i] = h.tone[i] + h.noise[i];
        return all;
    };
    auto peak = [] (const std::vector<float>& v, double rate, double from, double to)
    {
        float p = 0.0f;
        for (int i = (int) (from * rate); i < (int) (to * rate) && i < (int) v.size(); ++i)
            p = juce::jmax (p, std::abs (v[(size_t) i]));
        return p;
    };

    // The General MIDI drum map.
    check (drums::getDrumForNote (36) == Drum::kick && drums::getDrumForNote (35) == Drum::kick && drums::getDrumForNote (38) == Drum::snare
               && drums::getDrumForNote (39) == Drum::clap && drums::getDrumForNote (42) == Drum::closedHat && drums::getDrumForNote (44) == Drum::closedHat
               && drums::getDrumForNote (46) == Drum::openHat && drums::getDrumForNote (49) == Drum::cymbal && drums::getDrumForNote (56) == Drum::cowbell
               && drums::getDrumForNote (45) == Drum::lowTom && drums::getDrumForNote (50) == Drum::highTom && ! drums::getDrumForNote (60).has_value()
               && drums::getNote (Drum::kick) == 36,
           "the notes of the General MIDI drum map play their drums: 36 the kick, 38 the snare, 42 and 46 the hi-hats, 56 the cowbell");

    check (near (drums::getMetalRatios()[0], 1.0, 1.0e-12) && near (drums::getMetalRatios()[5] * 205.3, 800.0, 1.0e-9) && near (drums::bellRatio * 540.0, 800.0, 1.0e-9),
           "the metal is the TR-808's six squares from 205.3 to 800 Hz, the cowbell its 540 and 800 Hz");

    const drums::Settings kit;
    const double rate = 48000.0;

    // The kick: a sine falling from +24 semitones to its tune.
    const auto& kick = kit.drums[(size_t) Drum::kick];
    check (near (drums::getPitchAt (kick, 0.0), 200.0, 1.0e-9) && near (drums::getPitchAt (kick, 0.03), 50.0 * std::pow (2.0, 2.0 * std::exp (-1.0)), 1.0e-6)
               && near (drums::getPitchAt (kick, 1.0), 50.0, 0.01) && near (juce::Decibels::gainToDecibels (drums::getEnvelope (0.6, 0.6)), -60.0, 1.0e-6),
           "the kick starts at 200 Hz (two octaves over its 50 Hz), falls most of the way in its bend time, and a decay is the time to -60 dB");

    {
        auto plain = kit;
        plain.drums[(size_t) Drum::kick].clickPercent = 0.0f;
        const auto hit = drums::render (plain, Drum::kick, rate, 1.0);

        // Count the cycles in a stretch and compare with the pitch curve's own count.
        auto cycles = [&] (double from, double to)
        {
            int count = 0;
            for (int i = (int) (from * rate) + 1; i < (int) (to * rate); ++i)
                count += hit.tone[(size_t) i - 1] < 0.0f && hit.tone[(size_t) i] >= 0.0f ? 1 : 0;
            return count;
        };
        auto expected = [&] (double from, double to)
        {
            double phase = 0.0;
            for (double t = from; t < to; t += 1.0 / rate)
                phase += drums::getPitchAt (kick, t) / rate;
            return phase;
        };
        const int early = cycles (0.0, 0.04), late = cycles (0.3, 0.5);
        check (near (early, expected (0.0, 0.04), 1.0) && near (late, 10.0, 1.0) && early / 0.04 > 1.5 * late / 0.2,
               "played, the kick's pitch falls as the curve says: " + juce::String (early) + " cycles in its first 40 ms, " + juce::String (late)
                   + " in 200 ms once it has settled at 50 Hz");

        const double measured = juce::Decibels::gainToDecibels (peak (hit.tone, rate, 0.3, 0.325) / peak (hit.tone, rate, 0.05, 0.075));
        const double wanted = juce::Decibels::gainToDecibels (drums::getEnvelope (0.6, 0.3) / drums::getEnvelope (0.6, 0.05));
        check (near (measured, wanted, 1.0) && near (juce::Decibels::gainToDecibels (peak (hit.tone, rate, 0.0, 0.05)), -8.0, 1.0),
               "the kick peaks near -8 dB and fades as its decay says (" + juce::String (measured, 1) + " dB from 50 to 300 ms, expected "
                   + juce::String (wanted, 1) + ")");
    }

    // The snare: two modes of the head, and noise above the tone knob.
    {
        auto s = kit;
        s.drums[(size_t) Drum::snare].bendSemitones = 0.0f;
        const auto hit = drums::render (s, Drum::snare, rate, 0.4);
        const auto tone = toBuffer (hit.tone);
        const int from = (int) (0.005 * rate), length = (int) (0.12 * rate);
        const double first = levelDb (tone, 180.0, from, length), second = levelDb (tone, 180.0 * 1.59, from, length), between = levelDb (tone, 230.0, from, length);
        const double highs = bandDensityDb (hit.noise, rate, 3000.0, 8000.0), lows = bandDensityDb (hit.noise, rate, 100.0, 400.0);
        check (first - between > 15.0 && second - between > 10.0 && first > second && highs - lows > 15.0,
               "the snare's tone has the head's modes at 180 and 286 Hz, and its noise is " + juce::String (highs - lows, 0) + " dB stronger above 3 kHz than under 400 Hz");
    }

    // Metal: the hi-hats keep their highs.
    {
        const auto hat = sum (drums::render (kit, Drum::closedHat, rate, 0.2));
        const double highs = bandDensityDb (hat, rate, 8000.0, 14000.0), lows = bandDensityDb (hat, rate, 200.0, 1000.0);
        check (highs - lows > 15.0 && peak (hat, rate, 0.0, 0.01) > 0.02f && peak (hat, rate, 0.12, 0.2) < 1.0e-4f,
               "the closed hi-hat is " + juce::String (highs - lows, 0) + " dB stronger above 8 kHz than under 1 kHz, and gone in its 60 ms");

        const auto bell = toBuffer (drums::render (kit, Drum::cowbell, rate, 0.3).tone);
        const double at540 = levelDb (bell, 540.0, 0, 8192), at800 = levelDb (bell, 800.0, 0, 8192), at670 = levelDb (bell, 670.0, 0, 8192);
        check (at540 - at670 > 15.0 && at800 - at670 > 15.0, "the cowbell rings at 540 and 800 Hz, with nothing in between");
    }

    // The clap: bursts, then a tail.
    {
        const auto clapHit = drums::render (kit, Drum::clap, rate, 0.4).noise;
        const float gap = peak (clapHit, rate, 0.008, 0.0105);
        check (peak (clapHit, rate, 0.0, 0.002) > gap * 3.0f && peak (clapHit, rate, 0.011, 0.013) > gap * 3.0f && peak (clapHit, rate, 0.022, 0.024) > gap * 3.0f
                   && near (drums::getClapEnvelope (kit.drums[(size_t) Drum::clap], 0.011), 1.0, 1.0e-6),
               "the clap is three bursts 11 ms apart, like hands not quite together, then its tail");
    }

    // In the instrument: notes, velocity, the choke, one drum alone, the test hit.
    BuiltInDrums machine;
    auto out = playSynth (machine, 0.5, { { 0.0, 36, 1.0f } });
    const float kickPeak = out.getMagnitude (0, 0, out.getNumSamples());
    auto silent = playSynth (machine, 0.3, { { 0.0, 60, 1.0f } });
    check (kickPeak > 0.3f && silent.getMagnitude (0, 0, silent.getNumSamples()) < 1.0e-6f && juce::exactlyEqual (out.getSample (1, 2000), out.getSample (0, 2000)),
           "note 36 plays the kick on both sides; a note that isn't a drum plays nothing");

    {
        auto s = kit;
        s.velocityPercent = 100.0f;
        machine.setSettings (s);
        out = playSynth (machine, 0.5, { { 0.0, 36, 0.5f } });
        check (near (juce::Decibels::gainToDecibels (out.getMagnitude (0, 0, out.getNumSamples()) / kickPeak), -6.02, 0.3),
               "at 100% velocity a hit played half as hard is 6 dB quieter");
    }

    machine.setSettings (kit);
    out = playSynth (machine, 0.5, { { 0.0, 46, 1.0f }, { 0.1, 42, 1.0f } });
    const float choked = out.getMagnitude (0, (int) (0.25 * testRate), (int) (0.1 * testRate));
    auto loose = kit;
    loose.choke = false;
    machine.setSettings (loose);
    out = playSynth (machine, 0.5, { { 0.0, 46, 1.0f }, { 0.1, 42, 1.0f } });
    const float ringing = out.getMagnitude (0, (int) (0.25 * testRate), (int) (0.1 * testRate));
    check (choked < ringing * 0.01f && ringing > 1.0e-4f, "a closed hi-hat cuts off an open one, unless the choke is off");

    machine.setSettings (kit);
    machine.selectedDrum = (int) Drum::kick;
    machine.onlySelected = true;
    out = playSynth (machine, 0.3, { { 0.0, 36, 1.0f }, { 0.0, 42, 1.0f } });
    std::vector<float> alone ((size_t) out.getNumSamples());
    for (size_t i = 0; i < alone.size(); ++i)
        alone[i] = out.getSample (0, (int) i);
    machine.onlySelected = false;
    check (bandDensityDb (alone, testRate, 8000.0, 14000.0) < bandDensityDb (alone, testRate, 40.0, 200.0) - 40.0,
           "Hear it alone keeps the selected kick and leaves the hi-hat out");

    machine.selectedDrum = (int) Drum::cowbell;
    machine.clap.request (fx::Clap::Sound::note);
    out = playSynth (machine, 0.3, {});
    check (levelAt (out, 540.0, 0, 8192) > 0.005 && out.getMagnitude (0, 0, out.getNumSamples()) > 0.01f, "Play a hit plays the selected drum, the cowbell");

    // The kits.
    bool allFine = true;
    juce::String worst;
    for (auto& preset : drums::getPresets())
    {
        machine.setSettings (preset.settings);
        const auto again = machine.getSettings();
        const auto played = playSynth (machine, 1.5, { { 0.0, 36, 1.0f }, { 0.0, 42, 1.0f }, { 0.25, 42, 0.8f }, { 0.5, 38, 1.0f }, { 0.5, 39, 1.0f },
                                                       { 0.5, 42, 1.0f }, { 0.75, 46, 0.8f }, { 1.0, 36, 1.0f }, { 1.0, 49, 1.0f }, { 1.2, 50, 1.0f },
                                                       { 1.3, 45, 1.0f }, { 1.4, 56, 1.0f } });
        const float loudest = played.getMagnitude (0, 0, played.getNumSamples());
        bool same = again.choke == preset.settings.choke && near (again.velocityPercent, preset.settings.velocityPercent, 0.5) && preset.why.isNotEmpty();

        for (int d = 0; d < drums::numDrums; ++d)
        {
            const auto& o = again.drums[(size_t) d];
            const auto& expected = preset.settings.drums[(size_t) d];
            same = same && near (o.tuneHz, expected.tuneHz, expected.tuneHz * 0.002) && near (o.decaySeconds, expected.decaySeconds, expected.decaySeconds * 0.01)
                   && near (o.noisePercent, expected.noisePercent, 0.5) && near (o.toneHz, expected.toneHz, expected.toneHz * 0.01)
                   && near (o.bendSemitones, expected.bendSemitones, 0.06) && near (o.levelDb, expected.levelDb, 0.06);
        }

        const bool fine = loudest > 0.1f && loudest < 1.0f && same;
        if (! fine)
            worst << preset.name << " (" << juce::String (loudest, 2) << ") ";
        allFine = allFine && fine;
    }
    check (allFine && drums::getPresets().size() >= 6, "every kit keeps its drums and settings and plays a beat with every drum without clipping " + worst);

    check (drums::formatKnob (Knob::tune, kit, Drum::kick) == "50 Hz" && drums::formatKnob (Knob::bend, kit, Drum::kick) == "+24 st"
               && drums::formatKnob (Knob::tone, kit, Drum::snare) == "1.50 kHz" && drums::formatKnob (Knob::decay, kit, Drum::cymbal) == "1.80 s"
               && drums::formatKnob (Knob::bend, kit, Drum::closedHat) == "Off" && drums::formatKnob (Knob::choke, kit) == "On"
               && ! drums::isUsed (Drum::clap, Knob::tune) && ! drums::isUsed (Drum::closedHat, Knob::bend) && drums::isUsed (Drum::kick, Knob::bend)
               && BuiltInDrums::getParameterId (Knob::decay, Drum::openHat) == "openHatDecay",
           "the knobs show their values the way they're read, and say which drums they change");

    // An instrument in a track's chain, saved with the project.
    {
        const auto folder = juce::File::createTempFile ("drums");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::midi);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInDrums::getDescription(), error, 0);
        auto* plugin = slot != nullptr ? dynamic_cast<BuiltInDrums*> (slot->instance.get()) : nullptr;
        check (plugin != nullptr && slot->isInstrument && track->hasInstrument() && ! track->usesBuiltInSynth(),
               "the engine adds DawIt Drums to a MIDI track as its instrument " + error);

        if (plugin != nullptr)
        {
            plugin->setSettings (drums::getPresets()[2].settings);
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = project::save (engine, folder.getChildFile ("Song.dawit"), text, error) && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInDrums*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && near (reloaded->getSettings().drums[(size_t) Drum::kick].decaySeconds, 2.5, 0.01)
                       && near (reloaded->getSettings().drums[(size_t) Drum::kick].tuneHz, 43.65, 0.1),
                   "a project with DawIt Drums opens with its kit, without scanning " + error + warnings.joinIntoString (" "));
        }
    }
}

// The settings of one of DawIt Physical's starting points, by name.
static daw::phys::Settings getPhysicalPreset (const juce::String& name)
{
    for (auto& preset : daw::phys::getPresets())
        if (preset.name == name)
            return preset.settings;

    check (false, "DawIt Physical has a starting point called " + name);
    return {};
}

// How far a note in a stretch of the left channel is from `hz`, in cents: the strongest frequency within 60 cents of it.
static double centsOff (const juce::AudioBuffer<float>& b, double hz, int start, int num)
{
    double best = 0.0, loudest = 0.0;

    for (double cents = -60.0; cents <= 60.0; cents += 0.5)
    {
        const double level = levelAt (b, hz * std::pow (2.0, cents / 1200.0), start, num);

        if (level > loudest)
        {
            loudest = level;
            best = cents;
        }
    }

    return best;
}


static void runBuiltInPhysicalTests()
{
    using namespace daw;
    using phys::Knob;
    using phys::Exciter;
    std::cout << "DawIt Physical" << std::endl;
    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    // The loop: one period around for a string, half for a tube; the damping growing with the square of the frequency.
    {
        phys::Settings s;
        const auto string = phys::getLoop (s, 220.0, testRate);
        s.resonator = phys::Resonator::tube;
        const auto tube = phys::getLoop (s, 220.0, testRate);
        s.exciter = Exciter::blow;
        s.resonator = phys::Resonator::string;
        check (near (string.length, 48000.0 / 220.0, 1.0e-9) && near (tube.length, 24000.0 / 220.0, 1.0e-9) && near (string.tripsPerSecond, 220.0, 1.0e-9)
                   && near (tube.tripsPerSecond, 440.0, 1.0e-9) && phys::getResonator (s) == phys::Resonator::tube
                   && near (string.sideA + string.sideB, string.length - (string.pole / (1.0 - string.pole)), 0.2),
               "a string's loop is a period long (218.2 samples for A3 at 48 kHz), a tube's half, and a reed always plays a tube");
        check (near (phys::getDampingDbPerSecond (0.0f, 1000.0), 0.0, 1.0e-12) && near (phys::getDampingDbPerSecond (100.0f, 1000.0), 600.0, 1.0e-9)
                   && near (phys::getDampingDbPerSecond (40.0f, 2000.0), 4.0 * phys::getDampingDbPerSecond (40.0f, 1000.0), 1.0e-9),
               "the damping is 600 dB a second at 1 kHz at 100%, and four times as much an octave up");
    }

    {
        phys::Settings s;
        s.positionPercent = 20.0f;
        const bool string = near (phys::getPositionLevel (s, 5), 0.0, 1.0e-9) && near (phys::getPositionLevel (s, 10), 0.0, 1.0e-9)
                            && near (phys::getPositionLevel (s, 1), std::sin (juce::MathConstants<double>::pi / 5.0), 1.0e-9);
        s.positionPercent = 50.0f;
        const bool middle = near (phys::getPositionLevel (s, 2), 0.0, 1.0e-9) && near (phys::getPositionLevel (s, 3), 1.0, 1.0e-9);
        s.resonator = phys::Resonator::tube;
        const bool tube = ! phys::hasHarmonic (s, 2) && phys::hasHarmonic (s, 3) && near (phys::getPositionLevel (s, 1), std::cos (juce::MathConstants<double>::pi / 4.0), 1.0e-9);
        check (string && middle && tube, "playing at 1/5 of the string leaves out the 5th and 10th harmonics, the middle the even ones, and a tube has only odd ones");
    }

    {
        phys::Settings s;
        s.dampingPercent = 0.0f;
        const double plain = phys::getRingSeconds (s, 220.0, 1), plainHigh = phys::getRingSeconds (s, 220.0, 9);
        s.dampingPercent = 40.0f;
        const double ninth = phys::getRingSeconds (s, 220.0, 9), expectedNinth = 60.0 / (60.0 / 3.0 + phys::getDampingDbPerSecond (40.0f, 1980.0));
        const double released = phys::getRingSeconds (s, 220.0, 1, true);
        check (near (plain, 3.0, 0.01) && near (plainHigh, 3.0, 0.01) && near (ninth, expectedNinth, expectedNinth * 0.1) && near (released, 0.3, 0.01),
               "without damping every harmonic rings for the decay; with it the 9th rings " + juce::String (ninth, 2) + " s (expected "
                   + juce::String (expectedNinth, 2) + "); after the key is let go, for the release");
    }

    // The sound itself.
    const int start = (int) (0.3 * testRate), length = 8192;
    BuiltInPhysical synth;
    auto out = playSynth (synth, 1.5, { { 0.0, 57, 1.0f } });
    const float peak = out.getMagnitude (0, 0, out.getNumSamples());
    const double later = levelDb (out, 220.0, (int) (1.3 * testRate) - length / 2, length) - levelDb (out, 220.0, start - length / 2, length);
    check (std::abs (centsOff (out, 220.0, start, length)) < 1.0 && near (later, -20.0, 1.5) && juce::exactlyEqual (out.getSample (1, start), out.getSample (0, start)),
           "Init plays A3 in tune (" + juce::String (centsOff (out, 220.0, start, length), 1) + " cents) on both sides, its fundamental falling 20 dB a second for a 3 s decay ("
               + juce::String (later, 1) + " dB)");
    check (std::abs (juce::Decibels::gainToDecibels (peak) - phys::peakDb) < 2.0f, "Init peaks near -12 dB (" + juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB)");

    check (levelDb (out, 1100.0, start, length) < juce::jmin (levelDb (out, 880.0, start, length), levelDb (out, 1320.0, start, length)) - 30.0,
           "plucked at a fifth of the way, the 5th harmonic is missing between a loud 4th and 6th");

    phys::Settings s;
    s.positionPercent = 50.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (levelDb (out, 440.0, start, length) < levelDb (out, 660.0, start, length) - 30.0 && levelDb (out, 880.0, start, length) < levelDb (out, 660.0, start, length) - 30.0,
           "plucked in the middle, the even harmonics are gone");

    // The damping: the highs die sooner, as getRingSeconds says.
    auto dropOf = [&] (const juce::AudioBuffer<float>& b, double hz)
    {
        return levelDb (b, hz, (int) (0.6 * testRate) - length / 2, length) - levelDb (b, hz, (int) (0.2 * testRate) - length / 2, length);
    };

    s = {};
    s.dampingPercent = 0.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.8, { { 0.0, 57, 1.0f } });
    const double flatSixth = dropOf (out, 1320.0) / 0.4;
    s.dampingPercent = 60.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.8, { { 0.0, 57, 1.0f } });
    const double dampedSixth = dropOf (out, 1320.0) / 0.4, expectedSixth = -60.0 / phys::getRingSeconds (s, 220.0, 6);
    check (near (flatSixth, -20.0, 3.0) && near (dampedSixth, expectedSixth, std::abs (expectedSixth) * 0.15),
           "without damping the 6th harmonic fades like the fundamental (" + juce::String (flatSixth, 1) + " dB a second); at 60% it fades at "
               + juce::String (dampedSixth, 1) + " dB a second (expected " + juce::String (expectedSixth, 1) + ")");

    // A tube: only the odd harmonics, at the same pitch.
    s = {};
    s.resonator = phys::Resonator::tube;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    check (std::abs (centsOff (out, 220.0, start, length)) < 1.0 && levelDb (out, 440.0, start, length) < levelDb (out, 660.0, start, length) - 30.0
               && levelDb (out, 880.0, start, length) < levelDb (out, 660.0, start, length) - 30.0,
           "a plucked tube plays A3 with only the odd harmonics");

    // A strike: a hard hammer brighter than a soft one, both in tune.
    s = {};
    s.exciter = Exciter::strike;
    s.hardnessPercent = 100.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double hardTop = levelDb (out, 1540.0, start, length) - levelDb (out, 220.0, start, length);
    s.hardnessPercent = 0.0f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double softTop = levelDb (out, 1540.0, start, length) - levelDb (out, 220.0, start, length);
    check (hardTop > softTop + 15.0 && std::abs (centsOff (out, 220.0, start, length)) < 1.0,
           "a hard hammer leaves the 7th harmonic " + juce::String (hardTop - softTop, 1) + " dB louder than soft felt does");

    // A bow: the note lasts as long as the bow moves, in tune, and stops after the release.
    s = getPhysicalPreset ("Bowed cello");
    s.vibratoDepthPercent = 0.0f;
    synth.setSettings (s);
    out = playSynth (synth, 1.6, { { 0.0, 57, 1.0f }, { 1.2, 57, 0.0f } });
    const double bowEarly = levelDb (out, 220.0, (int) (0.4 * testRate), length), bowLate = levelDb (out, 220.0, (int) (0.9 * testRate), length);
    check (near (bowLate, bowEarly, 1.5) && std::abs (centsOff (out, 220.0, (int) (0.5 * testRate), length)) < 10.0
               && out.getMagnitude (0, (int) (1.55 * testRate), 2400) < 1.0e-3f,
           "a bowed note holds its level while the bow moves (" + juce::String (bowLate - bowEarly, 1) + " dB), in tune ("
               + juce::String (centsOff (out, 220.0, (int) (0.5 * testRate), length), 1) + " cents), and stops after the release");
    check (levelDb (out, 440.0, (int) (0.5 * testRate), length) < levelDb (out, 220.0, (int) (0.5 * testRate), length) - 3.0,
           "the bowed string slips once a period: the fundamental leads, as in Helmholtz's sawtooth");

    {
        auto bowed = getPhysicalPreset ("Bowed cello");
        bowed.vibratoDepthPercent = 0.0f;
        bowed.noisePercent = 0.0f;
        juce::String found;
        bool helmholtz = true;

        for (const double hz : { 110.0, 220.0, 440.0 })
        {
            const auto parts = phys::renderParts (bowed, hz, testRate, 0.62);
            const size_t count = (size_t) (3.0 * testRate / hz), first = parts.point.size() - count;
            int slips = 0, sticking = 0;
            bool slipping = false;

            for (size_t i = first; i < parts.point.size(); ++i)
            {
                const bool slip = parts.point[i] < 0.5f * parts.push[i];
                slips += slip && ! slipping ? 1 : 0;
                sticking += std::abs (parts.point[i] - parts.push[i]) < 0.05f * parts.push[i] ? 1 : 0;
                slipping = slip;
            }

            helmholtz = helmholtz && slips == 3 && sticking > (int) count / 2;
            found << juce::String (slips) << " ";
        }

        check (helmholtz, "under the bow the string sticks to the hair and slips back once a period, at A2, A3 and A4 (" + found.trim()
                              + " slips in 3 periods)");
    }

    // A reed: a tube with odd harmonics, in tune, as long as the breath lasts.
    s = getPhysicalPreset ("Clarinet");
    synth.setSettings (s);
    out = playSynth (synth, 1.2, { { 0.0, 57, 1.0f }, { 0.9, 57, 0.0f } });
    const int blowAt = (int) (0.5 * testRate);
    check (std::abs (centsOff (out, 220.0, blowAt, length)) < 3.0 && levelDb (out, 440.0, blowAt, length) < levelDb (out, 660.0, blowAt, length) - 25.0
               && out.getMagnitude (0, (int) (1.1 * testRate), 2400) < 1.0e-3f,
           "a reed on a tube plays A3 in tune with only odd harmonics, and stops when the breath does");

    // The exciter alone: a pluck is only its burst of noise; a bow or a breath on its own is almost silent.
    synth.exciterOnly = true;
    synth.setSettings ({});
    out = playSynth (synth, 0.3, { { 0.0, 57, 1.0f } });
    const bool burstOnly = out.getMagnitude (0, 0, 200) > 0.01f && out.getMagnitude (0, (int) (0.05 * testRate), 4800) < 1.0e-3f;
    synth.setSettings (getPhysicalPreset ("Clarinet"));
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f } });
    const double breathAlone = juce::Decibels::gainToDecibels (out.getMagnitude (0, (int) (0.3 * testRate), 4800), -150.0f);
    synth.exciterOnly = false;
    check (burstOnly && breathAlone < -30.0, "without the resonator a pluck is a 4.5 ms burst of noise and a breath is nearly silent ("
                                                 + juce::String (breathAlone, 1) + " dB): the string or tube makes the note");

    // Release, velocity, poly, mono and glide.
    s = {};
    s.releaseSeconds = 0.1f;
    synth.setSettings (s);
    out = playSynth (synth, 1.0, { { 0.0, 57, 1.0f }, { 0.4, 57, 0.0f } });
    check (levelDb (out, 220.0, (int) (0.7 * testRate), 4096) < levelDb (out, 220.0, (int) (0.3 * testRate), 4096) - 50.0,
           "letting the key go stops the string in the release time");

    s = {};
    s.velocityPercent = 100.0f;
    synth.setSettings (s);
    const double full = levelDb (playSynth (synth, 0.6, { { 0.0, 57, 1.0f } }), 220.0, start, length);
    const double half = levelDb (playSynth (synth, 0.6, { { 0.0, 57, 0.5f } }), 220.0, start, length);
    check (near (half - full, -6.02, 1.0), "at 100% velocity a note played half as hard is 6 dB quieter (" + juce::String (half - full, 1) + " dB)");

    synth.setSettings ({});
    out = playSynth (synth, 0.5, { { 0.0, 57, 1.0f }, { 0.0, 61, 1.0f }, { 0.0, 64, 1.0f } });
    const double rootLevel = levelAt (out, 220.0, start, length), thirdLevel = levelAt (out, sub::getNoteHz (61), start, length),
                 fifthLevel = levelAt (out, sub::getNoteHz (64), start, length);
    const double spread = juce::Decibels::gainToDecibels (juce::jmax (rootLevel, thirdLevel, fifthLevel) / juce::jmin (rootLevel, thirdLevel, fifthLevel));
    check (rootLevel > 0.005 && thirdLevel > 0.005 && fifthLevel > 0.005 && spread < 3.0,
           "poly plays the three notes of a chord, every pluck as full: their fundamentals within " + juce::String (spread, 1) + " dB");

    s = {};
    s.mode = sub::Mode::mono;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 57, 1.0f }, { 0.2, 64, 1.0f } });
    const int hammerAt = (int) (0.3 * testRate);
    check (levelAt (out, sub::getNoteHz (64), hammerAt, length) > 4.0 * levelAt (out, 220.0, hammerAt, length)
               && out.getMagnitude (0, (int) (0.2 * testRate), 480) < 0.6f * out.getMagnitude (0, 0, 480),
           "mono legato is a hammer-on: the ringing string moves to the new pitch without being plucked again");

    s = getPhysicalPreset ("Clarinet");
    s.glideSeconds = 0.3f;
    synth.setSettings (s);
    out = playSynth (synth, 0.6, { { 0.0, 45, 1.0f }, { 0.2, 57, 1.0f } });
    int crossings = 0;
    for (int i = (int) (0.25 * testRate); i < (int) (0.3 * testRate); ++i)
        crossings += out.getSample (0, i - 1) < 0.0f && out.getSample (0, i) >= 0.0f ? 1 : 0;
    check (crossings > 6 && crossings < 10, "glide slides between the notes: 50 ms after the change it's between 110 and 220 Hz (" + juce::String (crossings * 20) + " Hz)");

    // Higher rates play the same note.
    {
        BuiltInPhysical fast;
        fast.setSettings ({});
        fast.prepareToPlay (96000.0, testBlock);
        juce::AudioBuffer<float> high (2, 96000);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 57, 1.0f), 0);
        fast.processBlock (high, midi);
        auto at96 = [&high] (double hz, int first, int num)
        {
            const double w = juce::MathConstants<double>::twoPi * hz / 96000.0;
            double re = 0.0, im = 0.0;

            for (int i = 0; i < num; ++i)
            {
                const double window = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (num - 1));
                re += high.getSample (0, first + i) * window * std::cos (w * i);
                im -= high.getSample (0, first + i) * window * std::sin (w * i);
            }

            return juce::Decibels::gainToDecibels (4.0 * std::sqrt (re * re + im * im) / num, -150.0);
        };
        double best = 0.0, loudest = -200.0;
        for (double cents = -20.0; cents <= 20.0; cents += 0.5)
        {
            const double level = at96 (220.0 * std::pow (2.0, cents / 1200.0), 28800, 16384);
            if (level > loudest)
            {
                loudest = level;
                best = cents;
            }
        }
        const double fall = at96 (220.0, 76800 - 8192, 16384) - at96 (220.0, 28800 - 8192, 16384);
        check (near (fall, -10.0, 1.5) && std::abs (best) < 1.0,
               "at 96 kHz the note is as in tune (" + juce::String (best, 1) + " cents) and decays the same (" + juce::String (fall, 1) + " dB in half a second)");
    }

    // The window's test note.
    synth.setSettings (getPhysicalPreset ("Bowed cello"));
    synth.clap.request (fx::Clap::Sound::note);
    out = playSynth (synth, 2.2, {});
    check (out.getMagnitude (0, (int) (0.8 * testRate), 2400) > 0.02f && out.getMagnitude (0, (int) (2.1 * testRate), 2400) < 1.0e-3f,
           "Play a note plays A3 for 1.5 seconds, then its release");

    // The starting points.
    bool allFine = true;
    juce::String worst;
    for (auto& preset : phys::getPresets())
    {
        synth.setSettings (preset.settings);
        const auto again = synth.getSettings();
        const auto played = playSynth (synth, 2.0, { { 0.0, 48, 1.0f }, { 0.0, 55, 1.0f }, { 0.0, 64, 1.0f }, { 1.2, 48, 0.0f }, { 1.2, 55, 0.0f }, { 1.2, 64, 0.0f } });
        const float loudest = played.getMagnitude (0, 0, played.getNumSamples());
        const bool same = again.exciter == preset.settings.exciter && again.resonator == preset.settings.resonator && again.mode == preset.settings.mode
                          && near (again.positionPercent, preset.settings.positionPercent, 0.3) && near (again.decaySeconds, preset.settings.decaySeconds, preset.settings.decaySeconds * 0.01)
                          && near (again.dampingPercent, preset.settings.dampingPercent, 0.5) && preset.why.isNotEmpty()
                          && std::any_of (preset.notes.begin(), preset.notes.end(), [] (auto& note) { return note.isNotEmpty(); });
        const bool fine = loudest > 0.02f && loudest < 1.0f && same;
        if (! fine)
            worst << preset.name << " (" << juce::String (loudest, 2) << ") ";
        allFine = allFine && fine;
    }
    check (allFine && phys::getPresets().size() >= 12, "every starting point keeps its settings, says why, and plays a chord without clipping " + worst);

    // The pictures' numbers come from the sound.
    {
        const auto levels = phys::getHarmonics ({}, 220.0, 0.3, 12);
        check (levels.size() == 12 && levels[4] < 0.03 * juce::jmin (levels[3], levels[5]) && levels[0] > 0.001,
               "the harmonics measured for the window show the 5th missing at a fifth of the way along");
    }

    {
        phys::Settings x;
        x.decaySeconds = 1.2f;
        x.releaseSeconds = 0.35f;
        check (phys::formatKnob (Knob::exciter, x) == "Pluck" && phys::formatKnob (Knob::decay, x) == "1.20 s" && phys::formatKnob (Knob::release, x) == "350 ms"
                   && phys::formatKnob (Knob::noise, x) == "Off" && phys::formatKnob (Knob::vibratoRate, x) == "5.5 Hz" && phys::formatKnob (Knob::position, x) == "20%"
                   && juce::String (BuiltInPhysical::getParameterId (Knob::vibratoDepth)) == "vibratoDepth",
               "the knobs show their values the way they're read");
        x.exciter = Exciter::blow;
        check (! phys::isUsed (x, Knob::position) && ! phys::isUsed (x, Knob::hardness) && phys::isUsed (x, Knob::pressure) && phys::formatKnob (Knob::resonator, x) == "Tube"
                   && phys::isUsed ({}, Knob::hardness) && ! phys::isUsed ({}, Knob::pressure),
               "the knobs an exciter doesn't use are marked: a reed has no position or hardness, a pluck no pressure");
    }

    // An instrument in a track's chain, saved with the project.
    {
        const auto folder = juce::File::createTempFile ("physical");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* track = engine.addTrack (Track::Type::midi);
        juce::String error, text;
        auto* slot = engine.addPlugin (*track, BuiltInPhysical::getDescription(), error, 0);
        auto* plugin = slot != nullptr ? dynamic_cast<BuiltInPhysical*> (slot->instance.get()) : nullptr;
        check (plugin != nullptr && slot->isInstrument && track->hasInstrument() && ! track->usesBuiltInSynth(),
               "the engine adds DawIt Physical to a MIDI track as its instrument " + error);

        if (plugin != nullptr)
        {
            plugin->setSettings (getPhysicalPreset ("Bowed cello"));
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::StringArray warnings;
            const bool ok = project::save (engine, folder.getChildFile ("Song.dawit"), text, error) && project::load (loaded, folder.getChildFile ("Song.dawit"), error, warnings);
            auto* reloaded = ok && ! loaded.getTracks().empty() && ! loaded.getTracks()[0]->plugins.empty()
                                 ? dynamic_cast<BuiltInPhysical*> (loaded.getTracks()[0]->plugins[0]->instance.get()) : nullptr;
            check (reloaded != nullptr && warnings.isEmpty() && reloaded->getSettings().exciter == Exciter::bow
                       && near (reloaded->getSettings().vibratoDepthPercent, 18.0, 0.5) && near (reloaded->getSettings().positionPercent, 18.0, 0.3),
                   "a project with DawIt Physical opens with its exciter and settings, without scanning " + error + warnings.joinIntoString (" "));
        }
    }
}

static void runBuiltInDistortionTests()
{
    using namespace daw;
    using dist::Knob;
    using dist::Type;
    std::cout << "Built-in distortion" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };

    {
        bool unity = true;

        for (auto t : { Type::overdrive, Type::distortion, Type::fold })
            unity = unity && near (dist::shape (t, 1.0e-3f) / 1.0e-3f, 1.0, 0.001);

        check (unity && std::abs (dist::distort (Type::fuzz, 100.0f, 0.001f)) < 0.01f * std::abs (dist::distort (Type::fuzz, 100.0f, -0.3f)) && near (dist::shape (Type::rectify, -0.3f), dist::shape (Type::rectify, 0.3f), 1.0e-6)
                   && dist::shape (Type::rectify, -0.3f) > 0.0f && dist::shape (Type::overdrive, 3.0f) > -dist::shape (Type::overdrive, -3.0f)
                   && dist::shape (Type::fuzz, 5.0f) > 0.9f && near (dist::shape (Type::fuzz, -5.0f), -0.5, 0.05)
                   && near (dist::shape (Type::distortion, 5.0f), 1.0, 0.01) && std::abs (dist::shape (Type::fold, juce::MathConstants<float>::pi)) < 1.0e-5f,
               "overdrive, distortion and fold pass small values unchanged, a driven fuzz chokes them off, rectify turns everything positive; "
               "overdrive and fuzz are lopsided, distortion flattens at 1, and fold comes back down past the top");
    }

    {
        dist::Settings s;
        s.type = Type::distortion;
        s.driveDb = dist::maxDrive;
        const auto square = dist::getHarmonicsDb (s, 0.5f, 8);
        s.type = Type::rectify;
        s.driveDb = 0.0f;
        const auto octave = dist::getHarmonicsDb (s, 0.5f, 8);
        s.type = Type::overdrive;
        s.driveDb = 24.0f;
        const auto overdrive = dist::getHarmonicsDb (s, 0.5f, 8);
        s.type = Type::fuzz;
        s.driveDb = 40.0f;
        const auto fuzz = dist::getHarmonicsDb (s, 0.5f, 8);

        check (near (square[2] - square[0], -9.54, 0.5) && square[1] < -100.0f && octave[0] < octave[1] - 60.0f && overdrive[1] - overdrive[0] > -40.0f
                   && fuzz[1] - fuzz[0] > -30.0f,
               "full distortion nears a square wave (3rd " + juce::String (square[2] - square[0], 1) + " dB, no even ones), rectify leaves the "
                   + "octave and drops the note (" + juce::String (octave[0] - octave[1], 1) + " dB), overdrive adds even harmonics ("
                   + juce::String (overdrive[1] - overdrive[0], 1) + " dB) and fuzz loud ones (" + juce::String (fuzz[1] - fuzz[0], 1) + " dB)");
    }

    {
        dist::Settings s;
        s.speaker = true;
        const double at1k = dist::getSpeakerDb (1000.0), at10k = dist::getSpeakerDb (10000.0), at40 = dist::getSpeakerDb (40.0);
        s.tightHz = 1000.0f;
        check (std::abs (at1k) < 4.0 && at10k < -20.0 && at40 < -12.0 && near (dist::getGainAfter (s, 10000.0), juce::Decibels::decibelsToGain (at10k), 0.001)
                   && dist::getPeakIntoCurve (s, 1.0f) < 0.1f,
               "the guitar speaker keeps the mids (" + juce::String (at1k, 1) + " dB at 1 kHz) and drops the highs (" + juce::String (at10k, 1)
                   + " dB at 10 kHz) and the deep lows (" + juce::String (at40, 1) + " dB at 40 Hz); tight at 1 kHz keeps the A3 note mostly out of the curve");
    }

    auto sine = [] (double hz, float amplitude) { return [hz, amplitude] (int i) { return amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / testRate); }; };

    // The level of one frequency in the second half of a buffer, as a gain (a Hann-windowed DFT).
    auto levelAt = [] (const juce::AudioBuffer<float>& b, double hz)
    {
        const int start = b.getNumSamples() / 2, n = b.getNumSamples() - start;
        double re = 0.0, im = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / n);
            const double a = juce::MathConstants<double>::twoPi * hz * i / testRate;
            re += w * b.getSample (0, start + i) * std::cos (a);
            im -= w * b.getSample (0, start + i) * std::sin (a);
        }

        return 4.0 * std::sqrt (re * re + im * im) / n;
    };

    {
        // Tight cuts the lows before the curve; in parallel the clean lows stay.
        BuiltInDistortion plugin;
        dist::Settings s;
        s.driveDb = 0.0f;
        s.type = Type::distortion;
        s.tightHz = 500.0f;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        const int total = (int) testRate;
        const double cut = levelAt (runEffect (plugin, total, sine (80.0, 0.05f)), 80.0);
        plugin.setKnob (Knob::mix, 50.0f);
        const double parallel = levelAt (runEffect (plugin, total, sine (80.0, 0.05f)), 80.0);
        check (cut < 0.05 * 0.05 && near (parallel, 0.025, 0.003),
               "tight at 500 Hz keeps 80 Hz out of the curve (" + juce::String (juce::Decibels::gainToDecibels (cut / 0.05), 1)
                   + " dB), and at a 50% mix the clean low end stays (" + juce::String (parallel, 4) + ")");
    }

    {
        BuiltInDistortion plugin;
        plugin.setKnob (Knob::drive, 36.0f);
        const int total = (int) testRate;
        const auto matched = runEffect (plugin, total, sine (440.0, 0.1f));
        plugin.setMatchingLevel (false);
        const auto loud = runEffect (plugin, total, sine (440.0, 0.1f));
        const double inDb = juce::Decibels::gainToDecibels (0.1 / std::sqrt (2.0));
        const double matchedDb = rmsDb (matched, total / 2, total / 2), loudDb = rmsDb (loud, total / 2, total / 2);
        check (near (matchedDb, inDb, 1.0) && loudDb > inDb + 12.0,
               "with match level, 36 dB of drive comes out as loud as it went in (" + juce::String (matchedDb - inDb, 1) + " dB); without, "
                   + juce::String (loudDb - inDb, 1) + " dB louder");
    }

    {
        BuiltInDistortion plugin;
        dist::Settings s;
        s.driveDb = 0.0f;
        s.type = Type::distortion;
        s.mixPercent = 50.0f;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        const auto out = runEffect (plugin, (int) (0.5 * testRate), sine (12000.0, 0.05f));
        const double kept = levelAt (out, 12000.0);
        check (plugin.getLatencySamples() > 0 && near (kept, 0.05, 0.003),
               "the dry sound is held back as long as the oversampling, so a 50% mix keeps a 12 kHz tone whole (" + juce::String (kept, 4) + ")");
    }

    {
        // Hear only what it adds, through the speaker: the clean copy goes through the same filters, so the note itself is gone.
        BuiltInDistortion plugin;
        dist::Settings s;
        s.driveDb = 24.0f;
        s.type = Type::distortion;
        s.speaker = true;
        s.tightHz = 100.0f;
        plugin.setSettings (s);
        plugin.setMatchingLevel (false);
        plugin.listenToEffect = true;
        const auto out = runEffect (plugin, (int) testRate, sine (220.0, 0.3f));
        const double note = levelAt (out, 220.0), third = levelAt (out, 660.0), fifth = levelAt (out, 1100.0);
        check (note < 0.2 * third && third > 0.01 && fifth > 0.01,
               "hearing only what it adds leaves out the note (" + juce::String (note, 4) + ") and keeps its harmonics (3rd "
                   + juce::String (third, 3) + ", 5th " + juce::String (fifth, 3) + "), through tight and the speaker");
    }

    {
        BuiltInDistortion plugin;
        plugin.clap.request (fx::Clap::Sound::note);
        const auto out = runEffect (plugin, (int) (2.0 * testRate), [] (int) { return 0.0f; });
        std::vector<fx::LevelPoint> points (4096);
        const int n = plugin.history.read (points.data(), (int) points.size());
        float added = -100.0f;

        for (int i = 0; i < n; ++i)
            added = juce::jmax (added, points[(size_t) i].effectDb);

        check (out.getMagnitude (0, (int) (1.0 * testRate), (int) (0.2 * testRate)) > 0.01f && added > -40.0f && plugin.recentPeak.load() > 0.0f,
               "the test note plays through the distortion, and the history shows what it added");
    }

    {
        BuiltInDistortion a, b;
        dist::Settings s;
        s.tightHz = 320.0f;
        s.driveDb = 33.5f;
        s.type = Type::fold;
        s.toneHz = 4200.0f;
        s.speaker = true;
        s.mixPercent = 60.0f;
        a.setSettings (s);
        a.setMatchingLevel (false);
        juce::MemoryBlock state;
        a.getStateInformation (state);
        b.setStateInformation (state.getData(), (int) state.getSize());
        const auto loaded = b.getSettings();
        check (near (loaded.tightHz, 320.0, 1.0) && near (loaded.driveDb, 33.5, 0.01) && loaded.type == Type::fold && near (loaded.toneHz, 4200.0, 1.0)
                   && loaded.speaker && near (loaded.mixPercent, 60.0, 0.01) && ! b.isMatchingLevel(),
               "the distortion's settings survive saving and loading its state");
        check (dist::formatKnob (Knob::tight, s) == "320 Hz" && dist::formatKnob (Knob::tight, dist::Settings()) == "Off" && dist::formatKnob (Knob::drive, s) == "+33.5 dB"
                   && dist::formatKnob (Knob::type, s) == "Fold" && dist::formatKnob (Knob::speaker, s) == "On" && dist::formatKnob (Knob::tone, s) == "4.2 kHz",
               "the distortion's knobs read as 320 Hz, Off, +33.5 dB, Fold, On and 4.2 kHz");
    }

    {
        const auto& presets = dist::getPresets();
        bool valid = presets.size() >= 10 && presets.front().name == "Neutral start";

        for (auto& p : presets)
        {
            const auto& s = p.settings;
            valid = valid && p.why.isNotEmpty() && s.driveDb >= 0.0f && s.driveDb <= dist::maxDrive && s.tightHz >= dist::minTight && s.tightHz <= dist::maxTight
                    && s.toneHz >= dist::minTone && s.toneHz <= dist::maxTone && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& note) { return note.isNotEmpty(); });
        }

        check (valid, "every distortion starting point is in range and explains each knob");
    }

    checkThroughProject<BuiltInDistortion> (BuiltInDistortion::getDescription(), "DawIt Distortion",
                                            [] (BuiltInDistortion& p) { p.setKnob (Knob::drive, 31.0f); p.setKnob (Knob::speaker, 1.0f); },
                                            [&] (BuiltInDistortion& p) { return near (p.getSettings().driveDb, 31.0, 0.01) && p.getSettings().speaker; });
}

static void runBuiltInGateTests()
{
    using namespace daw;
    using gate::Knob;
    std::cout << "Built-in gate" << std::endl;

    auto near = [] (double value, double expected, double tolerance) { return std::abs (value - expected) <= tolerance; };
    auto toDb = [] (double x) { return 20.0 * std::log10 (juce::jmax (1.0e-12, std::abs (x))); };

    // A gate with these settings, run over `seconds` of a left and a right signal (per sample).
    auto play = [] (BuiltInGate& plugin, const gate::Settings& s, double seconds, const std::function<float (int)>& left,
                    const std::function<float (int)>& right)
    {
        plugin.setSettings (s);
        plugin.prepareToPlay (testRate, testBlock);
        const int total = (int) (seconds * testRate);
        juce::AudioBuffer<float> out (2, total), block (2, testBlock);
        juce::MidiBuffer midi;

        for (int pos = 0; pos < total; pos += testBlock)
        {
            const int n = juce::jmin (testBlock, total - pos);
            block.setSize (2, n, false, false, true);

            for (int i = 0; i < n; ++i)
                block.setSample (0, i, left (pos + i)), block.setSample (1, i, right (pos + i));

            plugin.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
        }

        return out;
    };

    auto playMono = [&] (BuiltInGate& plugin, const gate::Settings& s, double seconds, const std::function<float (int)>& input)
    {
        return play (plugin, s, seconds, input, input);
    };

    auto sine = [] (double hz, double db, double fadeSeconds = 0.0)
    {
        return [=] (int i)
        {
            const double t = i / testRate, fade = fadeSeconds > 0.0 ? juce::jmin (1.0, t / fadeSeconds) : 1.0;
            return (float) (juce::Decibels::decibelsToGain (db) * fade * std::sin (juce::MathConstants<double>::twoPi * hz * t));
        };
    };

    auto peakDb = [&] (const juce::AudioBuffer<float>& b, double fromSeconds, double toSeconds)
    {
        float peak = 0.0f;

        for (int i = (int) (fromSeconds * testRate); i < (int) (toSeconds * testRate); ++i)
            peak = juce::jmax (peak, std::abs (b.getSample (0, i)));

        return toDb (peak);
    };

    // The static curve.
    {
        gate::Settings s;
        auto e2 = s, e4 = s, e4r = s;
        e2.ratio = 2.0f;
        e4.ratio = 4.0f;
        e4r.ratio = 4.0f;
        e4r.rangeDb = 20.0f;
        const double under = s.thresholdDb - 10.0;
        check (juce::exactlyEqual (gate::getReductionDb (s, -90.0, false), 80.0) && juce::exactlyEqual (gate::getReductionDb (s, -10.0, true), 0.0)
                   && juce::exactlyEqual (gate::getReductionDb (e2, 0.0, true), 0.0)
                   && near (gate::getReductionDb (e2, under, false), 10.0, 1.0e-6) && near (gate::getReductionDb (e4, under, false), 30.0, 1.0e-6)
                   && near (gate::getReductionDb (e4r, under, false), 20.0, 1.0e-6) && gate::getReductionDb (e2, 0.0, false) >= 0.0,
               "a closed gate turns down by the whole range, an open one not at all; an expander 10 dB under the threshold is turned down by 10 dB "
               "at 2:1 and 30 dB at 4:1, at most the range");
    }

    // The key filters.
    {
        gate::Settings s;
        s.keyLowHz = 150.0f;
        const double kick = gate::getKeyResponseDb (s, 60.0);
        s.keyLowHz = 200.0f;
        const double lowSnare = gate::getKeyResponseDb (s, 50.0);
        s.keyLowHz = 1000.0f;
        const double lows = gate::getKeyResponseDb (s, 100.0);
        s = {};
        s.keyHighHz = 2000.0f;
        const double hat = gate::getKeyResponseDb (s, 8000.0);
        s.keyHighHz = 150.0f;
        const double snare = gate::getKeyResponseDb (s, 200.0);
        check (near (kick, -16.0, 0.3) && near (lowSnare, -24.1, 0.3) && near (lows, -40.0, 0.3) && near (hat, -24.1, 0.3) && near (snare, -6.2, 0.3)
                   && near (gate::getKeyResponseDb ({}, 1000.0), 0.0, 1.0e-6),
               "the key filters take 16 dB off a 60 Hz kick with a 150 Hz low cut, 24 dB off 8 kHz with a 2 kHz high cut and 6 dB off 200 Hz with "
               "a 150 Hz high cut (" + juce::String (kick, 1) + ", " + juce::String (hat, 1) + ", " + juce::String (snare, 1) + " dB)");
    }

    // Attack: after one attack time the gate is 63% open.
    {
        BuiltInGate plugin;
        gate::Settings s;
        s.attackMs = 10.0f;
        const auto out = playMono (plugin, s, 0.1, [] (int) { return 0.1f; });
        const double gain = out.getSample (0, (int) (0.01 * testRate) - 1) / 0.1;
        check (near (gain, 0.632, 0.002), "after one attack time (10 ms) the gate is 63% open (" + juce::String (gain, 3) + ")");
    }

    // Release and hold: it falls under the threshold 23.0 ms after the step (the detector falls 8.7 dB per 10 ms), then closes.
    {
        const int step = (int) (0.3 * testRate);
        auto input = [] (int i) { return i < step ? 0.1f : 0.001f; };
        gate::Settings s;
        s.hysteresisDb = 0.0f;
        s.holdMs = 0.0f;
        BuiltInGate plugin;
        auto out = playMono (plugin, s, 0.6, input);
        auto gainAt = [&] (const juce::AudioBuffer<float>& b, double seconds) { return b.getSample (0, step + (int) (seconds * testRate) - 1) / 0.001; };
        const double before = gainAt (out, 0.02), after = gainAt (out, 0.123);
        check (near (before, 1.0, 0.001) && near (after, 0.368, 0.01),
               "the gate is still open 20 ms after the step and a release time (100 ms) after it has closed it has fallen to 37% (" + juce::String (before, 3)
                   + ", " + juce::String (after, 3) + ")");

        s.holdMs = 50.0f;
        BuiltInGate held;
        out = playMono (held, s, 0.6, input);
        const double holding = gainAt (out, 0.07), released = gainAt (out, 0.173);
        check (near (holding, 1.0, 0.001) && near (released, 0.368, 0.01),
               "a hold of 50 ms keeps the gate open until 73 ms after the step, and it is at 37% a release time later (" + juce::String (holding, 3)
                   + ", " + juce::String (released, 3) + ")");
    }

    // Hysteresis: a level between the closing and opening levels keeps an open gate open.
    {
        const int step = (int) (0.3 * testRate);
        const float quiet = juce::Decibels::decibelsToGain (-42.0f);
        auto input = [quiet] (int i) { return i < step ? 0.1f : quiet; };
        gate::Settings s;
        BuiltInGate withHysteresis, without;
        const auto a = playMono (withHysteresis, s, 0.9, input);
        s.hysteresisDb = 0.0f;
        const auto b = playMono (without, s, 0.9, input);
        const int at = step + (int) (0.5 * testRate) - 1;
        const double open = a.getSample (0, at) / quiet, closed = b.getSample (0, at) / quiet;
        check (near (open, 1.0, 0.001) && closed < 0.02,
               "at -42 dB a gate with a threshold of -40 dB and 4 dB of hysteresis stays open, without hysteresis it closes ("
                   + juce::String (open, 3) + ", " + juce::String (closed, 3) + ")");
    }

    // Range and ratio on a steady sound under the threshold.
    {
        auto steady = [&] (double db, float range, float ratio)
        {
            BuiltInGate plugin;
            gate::Settings s;
            s.rangeDb = range;
            s.ratio = ratio;
            s.hysteresisDb = 0.0f;
            const float x = juce::Decibels::decibelsToGain ((float) db);
            const auto out = playMono (plugin, s, 1.0, [x] (int) { return x; });
            return toDb (out.getSample (0, out.getNumSamples() - 1));
        };

        const double r20 = steady (-60.0, 20.0f, 10.0f), r80 = steady (-60.0, 80.0f, 10.0f);
        check (near (r20, -80.0, 0.1) && near (r80, -140.0, 1.0), "a closed gate turns -60 dB down to -80 dB with a range of 20 dB and to -140 dB with 80 dB ("
                                                                      + juce::String (r20, 1) + ", " + juce::String (r80, 1) + ")");

        const double e2 = steady (-50.0, 80.0f, 2.0f), e4 = steady (-50.0, 80.0f, 4.0f), e4r = steady (-50.0, 20.0f, 4.0f);
        check (near (e2, -60.0, 0.1) && near (e4, -80.0, 0.1) && near (e4r, -70.0, 0.1),
               "an expander 10 dB under the threshold gives -60 dB at 2:1, -80 dB at 4:1 and -70 dB at 4:1 with a range of 20 dB ("
                   + juce::String (e2, 1) + ", " + juce::String (e4, 1) + ", " + juce::String (e4r, 1) + ")");

        // The simulation is the same code.
        gate::Settings s;
        s.ratio = 2.0f;
        s.hysteresisDb = 0.0f;
        const auto run = gate::simulate (s, std::vector<float> ((size_t) (gate::exampleRate), -50.0f), gate::exampleRate);
        check (near (run.gainDb.back(), -10.0, 0.1) && near (run.gainDb.back(), e2 + 50.0, 0.1),
               "the simulation gives the same gain as the processor on a steady -50 dB at 2:1 (" + juce::String (run.gainDb.back(), 2) + " dB)");
    }

    // The key: a low note under a key's low cut doesn't open the gate; key listen plays the key.
    {
        gate::Settings s;
        s.thresholdDb = -30.0f;
        s.keyLowHz = 200.0f;
        BuiltInGate cut, plain;
        const auto a = playMono (cut, s, 1.0, sine (50.0, -20.0, 0.05));
        s.keyLowHz = 20.0f;
        const auto b = playMono (plain, s, 1.0, sine (50.0, -20.0, 0.05));
        const double cutPeak = peakDb (a, 0.8, 1.0), plainPeak = peakDb (b, 0.8, 1.0);
        check (cutPeak < -90.0 && near (plainPeak, -20.0, 0.2),
               "a 50 Hz note at -20 dB doesn't open a gate whose key has a 200 Hz low cut, and passes without it (" + juce::String (cutPeak, 1) + ", "
                   + juce::String (plainPeak, 1) + " dB)");

        BuiltInGate listen;
        listen.listenToKey = true;
        s.keyLowHz = 1000.0f;
        const auto c = playMono (listen, s, 0.5, sine (100.0, -20.0));
        const double heard = peakDb (c, 0.3, 0.5);
        check (near (heard, -60.0, 0.3), "key listen plays the key: 100 Hz at -20 dB through a 1 kHz low cut comes out at -60 dB (" + juce::String (heard, 1) + ")");
    }

    // Stereo channels share one key and one gain.
    {
        BuiltInGate plugin;
        const auto out = play (plugin, {}, 0.5, [] (int) { return 0.1f; }, [] (int) { return 0.001f; });
        const double right = out.getSample (1, out.getNumSamples() - 1);
        check (near (right, 0.001, 0.00001), "a quiet right channel stays open with a loud left one: the channels share a key and a gain");
    }

    // Hear what it removes.
    {
        BuiltInGate closed, open;
        closed.listenToRemoved = true;
        open.listenToRemoved = true;
        const auto a = playMono (closed, {}, 0.5, [] (int) { return 0.001f; });
        const auto b = playMono (open, {}, 0.5, [] (int) { return 0.1f; });
        const double removedClosed = a.getSample (0, a.getNumSamples() - 1), removedOpen = b.getSample (0, b.getNumSamples() - 1);
        check (near (removedClosed, 0.001, 1.0e-6) && std::abs (removedOpen) < 1.0e-5,
               "hear what it removes plays the whole quiet sound while the gate is closed and nothing while it is open");
    }

    // Match level: bursts of a note over noise.
    {
        juce::Random random (11);
        std::vector<float> noise ((size_t) (3.0 * testRate));

        for (auto& x : noise)
            x = (random.nextFloat() * 2.0f - 1.0f) * 0.001f;

        auto bursts = [&] (int i)
        {
            const double t = i / testRate, inCycle = std::fmod (t, 0.5);
            const float burst = inCycle < 0.1 ? (float) (0.316 * std::sin (juce::MathConstants<double>::twoPi * 1000.0 * t)) : 0.0f;
            return burst + noise[(size_t) i];
        };

        BuiltInGate noiseOnly, tooHigh;
        gate::Settings s;
        playMono (noiseOnly, s, 3.0, bursts);
        s.thresholdDb = -5.0f;
        playMono (tooHigh, s, 3.0, bursts);
        const float small = noiseOnly.matchGainDb.load(), large = tooHigh.matchGainDb.load();
        check (small >= -0.01f && small < 0.3f && large > 1.0f,
               "match level adds almost nothing when the gate only removes noise (" + juce::String (small, 2) + " dB) and over 1 dB when it cuts the notes ("
                   + juce::String (large, 1) + " dB)");
    }

    // Flickers, and the history.
    {
        auto alternate = [] (int i) { return ((int) (i / (0.015 * testRate)) % 2 == 0 ? 0.0316f : 0.00316f); };
        gate::Settings s;
        s.hysteresisDb = 0.0f;
        s.holdMs = 0.0f;
        BuiltInGate chatter, held;
        playMono (chatter, s, 1.0, alternate);
        const int chatters = chatter.flickerTotal.load();
        s.holdMs = 20.0f;
        playMono (held, s, 1.0, alternate);
        const int calm = held.flickerTotal.load();
        int sum = 0;

        for (const auto& m : chatter.getRecent())
            sum += m.flickers;

        check (chatters >= 30 && calm == 0 && sum == chatters,
               "a gate with no hysteresis and no hold flickers on a level that alternates every 15 ms (" + juce::String (chatters)
                   + " times), a hold of 20 ms stops it, and the history's flickers add up to the count");

        BuiltInGate longRun;
        playMono (longRun, {}, 4.1, [] (int) { return 0.1f; });
        const auto recent = longRun.getRecent();
        check (recent.size() == (size_t) gate::numMoments && recent.back().open && near (recent.back().inputDb, -20.0, 0.1),
               "the history keeps the last 2000 moments (4 seconds in steps of 2 ms)");
    }

    // The test sound: four claps over hiss.
    {
        BuiltInGate plugin;
        gate::Settings s;
        s.rangeDb = 0.0f;
        plugin.clap.request (fx::Clap::Sound::hitsInHiss);
        const auto out = playMono (plugin, s, 1.0, [] (int) { return 0.0f; });
        const double hiss = peakDb (out, 0.1, 0.24), hit = peakDb (out, 0.5, 0.55), end = peakDb (out, 0.9, 1.0);
        check (near (hiss, -50.5, 0.6) && hit > -20.0 && near (end, -50.5, 0.6), "Hits and hiss plays four claps over hiss at -50.5 dB (" + juce::String (hiss, 1) + ", "
                                                                                   + juce::String (hit, 1) + " dB)");
    }

    // The simulation on the examples.
    {
        gate::Settings s;
        s.hysteresisDb = 0.0f;
        s.holdMs = 0.0f;
        const auto fading = gate::makeFadingNote();
        const auto bare = gate::simulate (s, fading, gate::exampleRate);
        s.hysteresisDb = 4.0f;
        const auto cured = gate::simulate (s, fading, gate::exampleRate);
        check (bare.closes == 2 && cured.closes == 1, "a fading note closes the gate twice without hysteresis or hold and once with 4 dB of hysteresis ("
                                                           + juce::String (bare.closes) + ", " + juce::String (cured.closes) + ")");

        const auto start = gate::makeHitStart();
        gate::Settings slow, fast;
        slow.attackMs = 5.0f;
        fast.attackMs = 0.05f;
        const int firstMs = (int) (0.011 * gate::exampleRate) - 1;   // the end of the first millisecond of the hit
        const double slowGain = gate::simulate (slow, start, gate::exampleRate).gainDb[(size_t) firstMs];
        const double fastGain = gate::simulate (fast, start, gate::exampleRate).gainDb[(size_t) firstMs];
        check (near (slowGain, -14.8, 0.3) && fastGain > -0.1, "a 5 ms attack lets the first millisecond of a hit through 15 dB down, 0.05 ms hardly at all ("
                                                                    + juce::String (slowGain, 1) + ", " + juce::String (fastGain, 2) + " dB)");

        const auto hits = gate::simulate ({}, gate::makeDrumHits (4.0), gate::exampleRate);
        const double between = hits.gainDb[(size_t) (0.45 * gate::exampleRate)], late = hits.gainDb[(size_t) (0.59 * gate::exampleRate)];
        check (hits.open[(size_t) (0.15 * gate::exampleRate)] == 1 && hits.open[(size_t) (0.45 * gate::exampleRate)] == 0 && between < -10.0 && late < between - 8.0,
               "with the starting settings the gate is open for a drum hit and closing between hits (" + juce::String (between, 1) + " dB at 0.45 s, "
                   + juce::String (late, 1) + " dB at 0.59 s)");
    }

    // Texts and starting points.
    {
        gate::Settings s;
        s.ratio = 2.0f;
        s.keyLowHz = 20.0f;
        s.attackMs = 0.05f;
        s.releaseMs = 1500.0f;
        const bool texts = gate::formatKnob (Knob::ratio, {}) == "Gate" && gate::formatKnob (Knob::ratio, s) == "2.0:1"
                           && gate::formatKnob (Knob::range, {}) == "-80 dB" && gate::formatKnob (Knob::keyLow, {}) == "Off"
                           && gate::formatKnob (Knob::keyHigh, {}) == "Off" && gate::formatKnob (Knob::attack, s) == "0.05 ms"
                           && gate::formatKnob (Knob::release, s) == "1.50 s";
        bool every = true;

        for (int k = 0; k < gate::numKnobs; ++k)
            every = every && gate::formatKnob ((Knob) k, {}).isNotEmpty();

        check (texts && every, "the knobs read 'Gate', '2.0:1', '-80 dB', 'Off', '0.05 ms' and '1.50 s'");

        const auto& presets = gate::getPresets();
        bool valid = presets.size() == 8 && presets.front().name == "Neutral start";
        const gate::Preset *reverb = nullptr, *vocal = nullptr, *voice = nullptr;

        for (const auto& p : presets)
        {
            const auto& t = p.settings;
            valid = valid && p.why.isNotEmpty() && t.thresholdDb >= gate::minThreshold && t.thresholdDb <= gate::maxThreshold
                    && t.hysteresisDb <= gate::maxHysteresis && t.rangeDb <= gate::maxRange && t.ratio >= gate::minRatio && t.ratio <= gate::maxRatio
                    && t.attackMs >= gate::minAttack && t.attackMs <= gate::maxAttack && t.holdMs <= gate::maxHold && t.releaseMs >= gate::minRelease
                    && t.releaseMs <= gate::maxRelease && t.keyLowHz >= gate::minKeyLow && t.keyLowHz <= gate::maxKeyLow && t.keyHighHz >= gate::minKeyHigh
                    && t.keyHighHz <= gate::maxKeyHigh && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& note) { return note.isNotEmpty(); });

            if (p.name == "Gated reverb") reverb = &p;
            if (p.name == "Lead vocal") vocal = &p;
            if (p.name == "Spoken voice") voice = &p;
        }

        check (valid && reverb != nullptr && reverb->settings.holdMs >= 250.0f && reverb->settings.releaseMs <= 30.0f && vocal != nullptr
                   && voice != nullptr && ! gate::isGate (vocal->settings.ratio) && ! gate::isGate (voice->settings.ratio) && vocal->settings.rangeDb <= 20.0f
                   && voice->settings.rangeDb <= 20.0f,
               "every gate starting point is in range and explains each knob; gated reverb holds long and closes fast; the voices are expanders");

        BuiltInGate plugin;
        plugin.applyPreset (presets[1], -49.0f);
        const bool followed = near (plugin.getSettings().thresholdDb, -49.0, 0.1);
        plugin.applyPreset (presets[0], -49.0f);
        check (followed && near (plugin.getSettings().thresholdDb, -40.0, 0.1), "a starting point that follows the track takes its threshold, the others keep theirs");
    }

    // Advice on a made-up track.
    {
        auto track = [] (int quiet, float quietDb, int loud, float loudDb)
        {
            std::vector<gate::Moment> moments;

            for (int i = 0; i < quiet; ++i)
                moments.push_back ({ quietDb, quietDb, quietDb, 0.0f, false, 0 });

            for (int i = 0; i < loud; ++i)
                moments.push_back ({ loudDb, loudDb, loudDb, 0.0f, true, 0 });

            return moments;
        };

        auto first = [] (const gate::Analysis& a) { return a.advice.isEmpty() ? juce::String() : a.advice[0]; };
        const auto noisy = track (1400, -55.0f, 600, -10.0f);
        gate::Settings s;
        s.thresholdDb = -60.0f;
        const auto low = gate::analyse (noisy, s, 0.0f);
        s.thresholdDb = -40.0f;
        const auto good = gate::analyse (noisy, s, 0.0f);
        s.thresholdDb = -12.0f;
        const auto high = gate::analyse (noisy, s, 0.0f);
        check (near (low.floorDb, -55.0, 0.01) && near (low.loudDb, -10.0, 0.01) && low.suggestedDb && near (*low.suggestedDb, -49.0, 0.01)
                   && first (low).contains ("opens on the noise") && first (low).contains ("-49")
                   && first (good).contains ("sits 15 dB above the noise") && first (high).contains ("within 6 dB") && first (high).contains ("-49"),
               "the advice finds the noise floor (-55 dB) and the loud parts (-10 dB), suggests -49 dB, and says whether the threshold is too low, right or too high");

        const auto silent = gate::analyse (track (1400, -85.0f, 600, -10.0f), {}, 0.0f);
        const auto narrow = gate::analyse (track (1400, -30.0f, 600, -22.0f), {}, 0.0f);
        const auto little = gate::analyse (track (1900, -100.0f, 100, -10.0f), {}, 0.0f);
        check (silent.silentGaps && ! silent.suggestedDb && silent.advice.size() == 1 && first (silent).contains ("silent")
                   && ! narrow.suggestedDb && first (narrow).contains ("little room") && little.advice.size() == 1 && first (little).contains ("Play the track"),
               "the advice says a silent track needs no gate, that a narrow gap needs an expander, and asks for the track to play first");

        // Digital silence (gaps between clips, a test sound that ended) doesn't count towards the noise floor.
        auto gappy = track (1000, -120.0f, 0, -10.0f);
        const auto noiseAndNotes = track (1000, -55.0f, 600, -10.0f);
        gappy.insert (gappy.end(), noiseAndNotes.begin(), noiseAndNotes.end());
        const auto gaps = gate::analyse (gappy, {}, 0.0f);
        check (near (gaps.floorDb, -55.0, 0.01) && ! gaps.silentGaps && gaps.suggestedDb && near (*gaps.suggestedDb, -49.0, 0.01) && gaps.heard == 1600,
               "the advice ignores digital silence when it looks for the noise floor");

        auto chatty = track (1400, -55.0f, 600, -10.0f);
        chatty[0].flickers = 5;
        const auto flickering = gate::analyse (chatty, {}, 2.0f);
        check (flickering.flickers == 5 && flickering.advice.size() == 3 && flickering.advice[1].contains ("5 times") && flickering.advice[2].contains ("2.0 dB"),
               "the advice counts flickers and says when match level has to add much");
    }

    checkThroughProject<BuiltInGate> (BuiltInGate::getDescription(), "DawIt Gate",
                                      [] (BuiltInGate& p) { p.setKnob (Knob::threshold, -33.0f); p.setKnob (Knob::ratio, 3.0f); p.setKnob (Knob::keyLow, 150.0f); p.setMatchingLevel (true); },
                                      [&] (BuiltInGate& p) { return near (p.getSettings().thresholdDb, -33.0, 0.01) && near (p.getSettings().ratio, 3.0, 0.01)
                                                                     && near (p.getSettings().keyLowHz, 150.0, 0.5) && p.isMatchingLevel(); });
}

static void runProjectFileTests()
{
    namespace project = daw::project;
    std::cout << "Project files" << std::endl;

    const auto folder = juce::File::createTempFile ("project");
    folder.createDirectory();
    const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

    // A short sine as an audio file somewhere outside the project.
    const auto source = folder.getChildFile ("elsewhere").getChildFile ("Kick.wav");
    source.getParentDirectory().createDirectory();
    {
        juce::AudioBuffer<float> sine (1, 4410);
        for (int i = 0; i < sine.getNumSamples(); ++i)
            sine.setSample (0, i, 0.5f * std::sin ((float) i * 0.1f));

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (source.createOutputStream());
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (44100.0).withNumChannels (1).withBitsPerSample (16));
        writer->writeFromAudioSampleBuffer (sine, 0, sine.getNumSamples());
    }

    daw::Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    engine.setTempo (96.84);   // as Detect tempo sets it, to the hundredth
    engine.setKey ({ 9, daw::MusicalKey::Scale::minor });
    engine.masterVolumeDb = -3.0f;
    engine.loopEnabled = true;
    engine.loopStartBeat = 4.0;
    engine.loopEndBeat = 12.0;
    engine.setChords ({ { 0, *daw::Chord::fromName ("Am") }, { 2, daw::Chord { 5, { 0, 4, 7, 11, 14 } } } });

    auto* lead = engine.addTrack (daw::Track::Type::midi);
    lead->name = "Lead \"one\"";
    lead->volumeDb = -6.5f;
    lead->pan = 0.25f;
    lead->muted = true;
    engine.setScaleFilter (*lead, true);
    engine.setBuiltInSound (*lead, daw::BuiltInSound::pad);
    auto* clip = engine.addMidiClip (*lead, 8.0, 8.0);
    clip->name = "Riff";
    clip->muted = true;
    clip->notes = { { 57, 0.0, 0.5, 100 / 127.0f }, { 60, 0.75, 1.25, 64 / 127.0f } };

    auto* drums = engine.addTrack (daw::Track::Type::audio);
    drums->name = "Drums";
    juce::String error;
    auto* audioClip = engine.addAudioClip (*drums, source, 4.0, error);

    if (audioClip != nullptr)
        engine.setClipGain (*audioClip, -4.5f);

    const auto projectFile = folder.getChildFile ("Song").getChildFile ("Song.dawit");
    projectFile.getParentDirectory().createDirectory();
    juce::String text;
    check (audioClip != nullptr && project::save (engine, projectFile, text, error) && projectFile.loadFileAsString() == text,
           "a project saves " + error);

    const auto copied = projectFile.getSiblingFile ("Audio Files").getChildFile ("Kick.wav");
    check (copied.existsAsFile() && audioClip->file == copied && text.contains ("\"file\": \"Audio Files/Kick.wav\""),
           "saving copies audio into the Audio Files folder, referred to by relative path");

    bool noteOnOneLine = false;
    for (auto& line : juce::StringArray::fromLines (text))
        noteOnOneLine = noteOnOneLine || (line.contains ("\"pitch\": 57") && line.contains ("\"velocity\": 100"));

    check (text.startsWith ("{\n  \"format\": \"DawIt project\"") && noteOnOneLine && text.contains ("\"tonic\": \"A\""),
           "the file is indented JSON, with one note per line");

    daw::Engine loaded;
    loaded.deviceManager.removeAudioCallback (&loaded);
    juce::StringArray warnings;
    const bool ok = project::load (loaded, projectFile, error, warnings);
    check (ok && warnings.isEmpty() && project::toText (loaded, projectFile) == text, "loading it gives the same project " + error);

    const auto& tracks = loaded.getTracks();
    check (tracks.size() == 2 && tracks[0]->name == "Lead \"one\"" && tracks[0]->scaleFilter.load() && tracks[0]->muted.load()
               && tracks[0]->builtInSound.load() == daw::BuiltInSound::pad && text.contains ("\"builtInSound\": \"pad\"")
               && tracks[0]->colour == lead->colour && loaded.getChord (2) != nullptr && *loaded.getChord (2) == *engine.getChord (2)
               && loaded.getKey() == engine.getKey() && juce::exactlyEqual (loaded.getTempo(), 96.84) && loaded.loopEndBeat.load() == 12.0,
           "tracks, chords, key, tempo and loop come back");
    check (tracks[0]->clips[0]->muted && ! tracks[1]->clips[0]->muted && text.contains ("\"muted\": true"), "a muted clip stays muted");
    {
        // The Score view's clef: Auto isn't written and a missing key loads as Auto; Bass guitar comes back.
        check (! text.contains ("scoreClef") && tracks[0]->scoreClef == daw::ScoreClef::automatic, "the score clef Auto is not written, and loads as Auto");
        engine.setScoreClef (*engine.getTracks()[0], daw::ScoreClef::bassGuitar);
        juce::String clefText;
        daw::Engine clefLoaded;
        clefLoaded.deviceManager.removeAudioCallback (&clefLoaded);
        juce::StringArray clefWarnings;
        check (project::save (engine, projectFile, clefText, error) && clefText.contains ("\"scoreClef\": \"bassGuitar\"")
                   && project::load (clefLoaded, projectFile, error, clefWarnings) && clefLoaded.getTracks()[0]->scoreClef == daw::ScoreClef::bassGuitar
                   && clefLoaded.getTracks()[1]->scoreClef == daw::ScoreClef::automatic,
               "a track's score clef (Bass guitar) is saved and loaded");
        engine.setScoreClef (*engine.getTracks()[0], daw::ScoreClef::automatic);
        project::save (engine, projectFile, clefText, error);
    }
    check (juce::exactlyEqual (static_cast<daw::AudioClip&> (*tracks[1]->clips[0]).gainDb, -4.5f), "an audio clip keeps its gain");

    // Saving to another folder brings the audio along.
    const auto other = folder.getChildFile ("Copy").getChildFile ("Copy.dawit");
    other.getParentDirectory().createDirectory();
    check (project::save (engine, other, text, error) && other.getSiblingFile ("Audio Files").getChildFile ("Kick.wav").existsAsFile(),
           "Save As elsewhere copies the audio there too");

    copied.deleteFile();
    warnings.clear();
    check (project::load (loaded, projectFile, error, warnings) && warnings.size() == 1 && loaded.getTracks()[1]->clips.empty(),
           "a missing audio file is reported and the rest still loads");

    const auto bad = folder.getChildFile ("bad.dawit");
    bad.replaceWithText ("{ \"format\": \"DawIt project\", \"version\": 99 }");
    check (! project::load (loaded, bad, error, warnings) && error.contains ("newer") && loaded.getTracks().size() == 2,
           "a project from a newer version is refused, leaving the open one alone");
    // Projects saved before the app was renamed still open, and from before the built-in synth had
    // sounds they get the piano.
    const auto oldProject = folder.getChildFile ("Song").getChildFile ("Old.minidaw");
    oldProject.replaceWithText (projectFile.loadFileAsString().replace ("\"DawIt project\"", "\"MiniDAW project\"")
                                                              .replace ("\"builtInSound\": \"pad\",", ""));
    warnings.clear();
    check (project::isProjectFile (oldProject) && project::load (loaded, oldProject, error, warnings) && loaded.getTracks().size() == 2
               && loaded.getTracks()[0]->builtInSound.load() == daw::BuiltInSound::piano,
           "a .minidaw project from before the rename still opens");

    bad.replaceWithText ("not json");
    check (! project::load (loaded, bad, error, warnings), "a broken file is refused");

    // Buses: nested in the file, version 2 only when there is one.
    {
        const auto busFile = folder.getChildFile ("Buses").getChildFile ("Buses.dawit");
        busFile.getParentDirectory().createDirectory();

        daw::Engine busSong;
        busSong.deviceManager.removeAudioCallback (&busSong);
        auto* kick = busSong.addTrack (daw::Track::Type::midi);
        kick->name = "Kick";
        auto* snare = busSong.addTrack (daw::Track::Type::midi);
        snare->name = "Snare";
        auto* bass = busSong.addTrack (daw::Track::Type::midi);
        bass->name = "Bass";
        auto* busDrums = busSong.createBus ({ kick, snare }, "Drums");
        auto* sub = busSong.createBus ({ snare }, "Sub");
        busDrums->volumeDb = -2.0f;
        busDrums->muted = true;
        busSong.setFolded (*busDrums, true);

        juce::String busError, busText;
        auto* slot = busSong.addPlugin (*busDrums, daw::BuiltInCompressor::getDescription(), busError);
        auto* compressor = slot != nullptr ? dynamic_cast<daw::BuiltInCompressor*> (slot->instance.get()) : nullptr;

        if (compressor != nullptr)
        {
            auto settings = compressor->getSettings();
            settings.ratio = 2.0f;
            settings.attackMs = 30.0f;
            compressor->setSettings (settings);
        }

        check (project::save (busSong, busFile, busText, busError), "a project with buses saves " + busError);

        juce::var parsed;
        juce::JSON::parse (busText, parsed);
        auto* top = parsed["tracks"].getArray();
        const auto& drumsJson = top != nullptr && top->size() == 2 ? (*top)[0] : juce::var();
        const auto* members = drumsJson["tracks"].getArray();
        check (busText.contains ("\"version\": 2") && busText.contains ("\"type\": \"bus\"") && busText.contains ("\"folded\": true")
                   && drumsJson["name"].toString() == "Drums" && members != nullptr && members->size() == 2 && (*members)[0]["name"].toString() == "Kick"
                   && (*members)[1]["type"].toString() == "bus" && (*members)[1]["tracks"].getArray() != nullptr
                   && ! drumsJson.hasProperty ("clips") && ! (*members)[1].hasProperty ("clips") && (*top)[1]["name"].toString() == "Bass",
               "buses are saved as version 2, nested in their bus, without clips");
        std::cout << "  (a bus in the file:)\n" << busText.substring (busText.indexOf ("\"type\": \"bus\"") - 10, busText.indexOf ("\"type\": \"bus\"") + 330) << std::endl;

        daw::Engine busLoaded;
        busLoaded.deviceManager.removeAudioCallback (&busLoaded);
        juce::StringArray busWarnings;
        const bool busOk = project::load (busLoaded, busFile, busError, busWarnings);
        const auto& loadedTracks = busLoaded.getTracks();
        auto* reloaded = busOk && ! loadedTracks.empty() && ! loadedTracks[0]->plugins.empty()
                             ? dynamic_cast<daw::BuiltInCompressor*> (loadedTracks[0]->plugins[0]->instance.get()) : nullptr;
        check (busOk && busWarnings.isEmpty() && project::toText (busLoaded, busFile) == busText && loadedTracks.size() == 5
                   && loadedTracks[1]->bus == loadedTracks[0].get() && loadedTracks[3]->bus == loadedTracks[2].get() && loadedTracks[2]->bus == loadedTracks[0].get()
                   && loadedTracks[0]->folded && loadedTracks[0]->muted.load() && juce::exactlyEqual (loadedTracks[0]->volumeDb.load(), -2.0f)
                   && daw::routing::isTidy (loadedTracks) && reloaded != nullptr && juce::exactlyEqual (reloaded->getSettings().ratio, 2.0f)
                   && juce::exactlyEqual (reloaded->getSettings().attackMs, 30.0f) && sub != nullptr,
               "a project with buses loads with the same tree, folding, mute, fader and effect settings, and saves to the same text " + busError);

        // Without a bus nothing changes: version 1, no folded, no nested tracks.
        check (text.contains ("\"version\": 1") && ! text.contains ("\"folded\"") && text.indexOf ("\"tracks\"") == text.lastIndexOf ("\"tracks\""),
               "a song without buses is still saved as version 1, with no bus entries");

        // A project written by hand, as older versions saved them.
        const auto oldFile = folder.getChildFile ("OldStyle.dawit");
        oldFile.replaceWithText ("{ \"format\": \"DawIt project\", \"version\": 1, \"tempo\": 100, \"tracks\": ["
                                 "{ \"type\": \"midi\", \"name\": \"Lead\", \"volumeDb\": -3, \"plugins\": [], \"clips\": [] },"
                                 "{ \"type\": \"audio\", \"name\": \"Vox\", \"plugins\": [], \"clips\": [] } ] }");
        warnings.clear();
        daw::Engine old;
        old.deviceManager.removeAudioCallback (&old);
        const bool oldOk = project::load (old, oldFile, error, warnings);
        const auto firstText = project::toText (old, oldFile);
        check (oldOk && warnings.isEmpty() && old.getTracks().size() == 2 && old.getTracks()[0]->name == "Lead" && old.getTracks()[1]->name == "Vox"
                   && ! old.getTracks()[0]->isBus() && firstText.contains ("\"version\": 1"),
               "a version-1 project written by an older DawIt loads as it was and is saved as version 1 again");

        bad.replaceWithText ("{ \"format\": \"DawIt project\", \"version\": 4 }");
        check (! project::load (loaded, bad, error, warnings) && error.contains ("newer"), "a project of version 4 is refused as newer");
    }
}

static void runBuiltInSynthTests()
{
    std::cout << "Built-in synth sounds" << std::endl;
    using daw::BuiltInSound;
    namespace sounds = daw::builtInSounds;

    auto renderSynth = [] (daw::BuiltInSynth& synth, juce::MidiBuffer& events, double seconds)
    {
        juce::AudioBuffer<float> out (2, (int) (seconds * testRate));
        out.clear();
        synth.renderNextBlock (out, events, 0, out.getNumSamples());
        events.clear();
        return out;
    };

    auto peak = [] (const juce::AudioBuffer<float>& b)
    {
        return juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples()));
    };

    auto isFinite = [] (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i)))
                    return false;

        return true;
    };

    auto activeVoices = [] (daw::BuiltInSynth& synth)
    {
        int active = 0;
        for (int i = 0; i < synth.getNumVoices(); ++i)
            active += synth.getVoice (i)->isVoiceActive() ? 1 : 0;

        return active;
    };

    for (auto sound : sounds::all)
    {
        std::atomic<BuiltInSound> selected { sound };
        daw::BuiltInSynth synth (selected);
        synth.setCurrentPlaybackSampleRate (testRate);
        const auto name = sounds::getName (sound);
        check (sounds::fromId (sounds::getId (sound)) == sound, name + ": its id reads back");

        std::vector<int> notes { 28, 60, 96 };

        if (sound == BuiltInSound::drums)
            notes = { 36, 37, 38, 39, 42, 44, 46, 45, 50, 49, 51, 53, 54, 56, 70, 75 };
        else if (sound == BuiltInSound::percussion)
            for (int note = 54; note <= 82; ++note)
                notes.push_back (note);
        bool plays = true, clean = true, ends = true;

        for (int note : notes)
        {
            juce::MidiBuffer events;
            events.addEvent (juce::MidiMessage::noteOn (1, note, 0.9f), 0);
            const auto held = renderSynth (synth, events, 0.4);
            events.addEvent (juce::MidiMessage::noteOff (1, note), 0);
            const auto tail = renderSynth (synth, events, 8.0);

            plays = plays && peak (held) > 0.02f;
            clean = clean && isFinite (held) && isFinite (tail) && peak (held) < 1.0f && peak (tail) < 1.0f;
            ends = ends && activeVoices (synth) == 0;
        }

        check (plays, name + " plays, low to high");
        check (clean, name + " stays below full scale");
        check (ends, name + ": notes end after their release");
    }

    std::atomic<BuiltInSound> drums { BuiltInSound::drums };
    daw::BuiltInSynth kit (drums);
    kit.setCurrentPlaybackSampleRate (testRate);
    juce::MidiBuffer events;
    events.addEvent (juce::MidiMessage::noteOn (1, 46, 0.9f), 0);
    events.addEvent (juce::MidiMessage::noteOff (1, 46), 100);
    renderSynth (kit, events, 0.05);
    check (activeVoices (kit) == 1, "a drum rings on after its note-off");
    events.addEvent (juce::MidiMessage::noteOn (1, 42, 0.9f), 0);
    renderSynth (kit, events, 0.05);
    check (activeVoices (kit) == 1, "a closed hi-hat cuts off the open one");

    std::atomic<BuiltInSound> percussion { BuiltInSound::percussion };
    daw::BuiltInSynth hands (percussion);
    hands.setCurrentPlaybackSampleRate (testRate);
    events.addEvent (juce::MidiMessage::noteOn (1, 81, 0.9f), 0);
    renderSynth (hands, events, 0.1);
    events.addEvent (juce::MidiMessage::noteOn (1, 80, 0.9f), 0);
    renderSynth (hands, events, 0.04);
    check (activeVoices (hands) == 1, "a muted triangle cuts off the open one");

    // Changing the sound leaves sounding notes alone; the next note gets the new sound.
    std::atomic<BuiltInSound> choice { BuiltInSound::organ };
    daw::BuiltInSynth synth (choice);
    synth.setCurrentPlaybackSampleRate (testRate);
    events.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
    renderSynth (synth, events, 0.1);
    choice = BuiltInSound::drums;
    check (rms (renderSynth (synth, events, 2.0), 0, (int) testRate) > 0.01f, "a held organ note keeps sounding after switching to drums");
    events.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
    renderSynth (synth, events, 0.2);
    events.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
    events.addEvent (juce::MidiMessage::noteOff (1, 60), 10);
    renderSynth (synth, events, 2.0);
    check (activeVoices (synth) == 0, "the next note plays as a drum, which has died away");
}

static void runChordGuideTests()
{
    using namespace daw;
    using namespace daw::gen::guide;
    std::cout << "Chord progression explainer" << std::endl;

    const MusicalKey cMajor { 0, MusicalKey::Scale::major }, aMinor { 9, MusicalKey::Scale::minor };
    auto chord = [] (const char* name) { return *Chord::fromName (name); };
    auto chords = [&chord] (std::initializer_list<const char*> names)
    {
        std::vector<Chord> list;
        for (auto* n : names)
            list.push_back (chord (n));
        return list;
    };

    check (getFamily (cMajor, chord ("C")) == Family::home && getFamily (cMajor, chord ("Am")) == Family::home
           && getFamily (cMajor, chord ("F")) == Family::away && getFamily (cMajor, chord ("G")) == Family::tension
           && getFamily (cMajor, chord ("Bb")) == Family::outside, "chords fall into home, away, tension or outside");
    check (getNumeral (cMajor, chord ("Am")) == "vi" && getNumeral (cMajor, chord ("G7")) == "V7"
           && getNumeral (cMajor, chord ("Cmaj7")) == "Imaj7" && getNumeral (cMajor, chord ("Bb")) == "bVII"
           && getNumeral (aMinor, chord ("E")) == "V" && getNumeral (aMinor, chord ("Em")) == "v",
           "numerals, also for sevenths and chords from outside the key");

    const auto major = getCommonProgressions (cMajor), minor = getCommonProgressions (aMinor);
    check (! major.empty() && major[0].getChords (cMajor) == chords ({ "C", "G", "Am", "F" }), "the pop progression is C G Am F in C major");
    check (! minor.empty() && minor[0].getChords (aMinor) == chords ({ "Am", "F", "C", "G" }), "minor keys get minor progressions, epic minor first");

    check (findProgressionName (aMinor, chords ({ "Am", "F", "C", "G" })) == "Epic minor"
           && findProgressionName (cMajor, chords ({ "Am", "F", "C", "G" })) == "Sad pop",
           "the same chords are a different progression in another key");
    check (findProgressionName (cMajor, chords ({ "G", "Am", "F", "C" })).startsWith ("Pop progression (starting"),
           "a progression started on another of its chords is recognised");
    check (findProgressionName (cMajor, chords ({ "C", "F", "G", "C" })) == "Three-chord song"
           && findProgressionName (cMajor, chords ({ "Dm7", "G7", "Cmaj7" })) == "Jazz turnaround (ii-V-I)",
           "loops ending where they start, and sevenths, still match");
    check (findProgressionName (cMajor, chords ({ "C", "E", "Bb" })).isEmpty(), "unknown progressions have no name");

    check (explainMove (cMajor, chord ("G"), chord ("C")).contains ("strongest move")
           && explainMove (cMajor, chord ("G"), chord ("Am")).contains ("deceptive")
           && explainMove (cMajor, chord ("C"), chord ("Bb")).contains ("outside the key")
           && explainMove (cMajor, chord ("F"), chord ("C")).contains ("amen"),
           "moves are explained: resolution, deceptive move, outside chords, the amen");
    check (explainMove (aMinor, chord ("G"), chord ("Am")).contains ("climbs up")
           && explainMove (cMajor, chord ("Bdim"), chord ("C")).contains ("unstable")
           && explainMove (aMinor, chord ("Am"), chord ("G")).contains ("sets up the return home")
           && explainMove (aMinor, chord ("F"), chord ("C")).contains ("keeps its restful feel"),
           "the seven chord of a minor key isn't called unstable, and leaving home isn't heading home");

    const auto e = explainChord (aMinor, chord ("E"));
    check (e.contains ("G#") && e.contains ("borrowed") && e.contains ("Tension"), "a borrowed chord names its outside note");
    const auto am = explainChord (cMajor, chord ("Am"));
    check (am.contains ("vi chord in C major") && am.contains ("A C E") && am.contains ("minor chord") && am.contains ("Family: Home"),
           "a chord's explanation gives its numeral, notes, sound and family");

    check (explainChordTrack (cMajor, {}).contains ("empty"), "an empty chord track says how to start");
    const auto epic = explainChordTrack (aMinor, chords ({ "Am", "Am", "F", "C", "G" }));
    check (epic.contains ("Epic minor") && epic.contains ("All of these chords are in the key") && epic.contains ("starts at home"),
           "the song's chord track is recognised and described");
    const auto floating = explainChordTrack (cMajor, chords ({ "F", "G" }));
    check (floating.contains ("never lands on the home chord") && floating.contains ("ends on tension"), "advice for a progression without home");

    bool allKeys = true;
    for (int tonic = 0; tonic < 12; ++tonic)
        for (int scale = 0; scale < MusicalKey::numScales; ++scale)
        {
            const MusicalKey key { tonic, (MusicalKey::Scale) scale };
            allKeys = allKeys && getIntroduction (key).isNotEmpty() && explainCircleOfFifths (key).contains ("Neighbours")
                      && getCircleProgression (key).size() == 5 && getCircleProgression (key).back() == gen::harmony::triad (key, 1);

            for (auto& p : getCommonProgressions (key))
                allKeys = allKeys && p.getChords (key).size() == p.steps.size() && explainProgression (key, p).contains ("Step by step");

            for (int degree = 1; degree <= 7; ++degree)
                allKeys = allKeys && explainChord (key, gen::harmony::triad (key, degree)).contains ("Family: ");
        }
    check (allKeys, "every key and scale has an introduction, the circle of fifths, progressions and chord explanations");

    const MusicalKey dDorian { 2, MusicalKey::Scale::dorian }, eMajor { 4, MusicalKey::Scale::major }, fMajor { 5, MusicalKey::Scale::major };
    check (getCirclePosition (cMajor) == 0 && getCirclePosition (aMinor) == 0 && getCirclePosition (dDorian) == 0
           && getCirclePosition (eMajor) == 4 && getCirclePosition (fMajor) == 11 && getCircleTonic (11) == 5,
           "keys sit at their relative major's place on the circle");
    check (getKeySignature (0).isEmpty() && getKeySignature (4).joinIntoString (" ") == "F# C# G# D#"
           && getKeySignature (11).joinIntoString (" ") == "Bb" && describeKeySignature (9) == "3 flats"
           && describeKeySignature (6) == "6 sharps or 6 flats" && getCircleLabel (6, true) == "D#m",
           "key signatures and labels round the circle");
    check (getCircleProgression (cMajor) == chords ({ "Em", "Am", "Dm", "G", "C" })
           && getCircleProgression (aMinor) == chords ({ "C", "F", "Bdim", "E", "Am" }),
           "the chain of fifths down to home, with the major V in minor");

    const auto circle = explainCircleOfFifths (cMajor);
    check (circle.contains ("G major and F major differ from C major") && circle.contains ("relative minor is A minor")
           && circle.contains ("Bdim (vii") && explainCircleOfFifths (dDorian).contains ("uses the notes of C major")
           && explainCircleOfFifths (eMajor).contains ("4 sharps (F#, C#, G#, D#)"),
           "the circle of fifths explanation follows the key");
}

static void runTakeTests()
{
    using namespace daw;
    using namespace daw::gen;
    std::cout << "Variation and takes" << std::endl;

    check (std::abs (getOllamaTemperature (defaultVariation) - 0.8) < 1.0e-9,
           "the default variation keeps the old sampling temperature");
    check (getOllamaTemperature (0.0) < getOllamaTemperature (1.0) && std::abs (getOllamaTemperature (-1.0) - getOllamaTemperature (0.0)) < 1.0e-9,
           "more variation samples hotter, and the setting is clamped");
    check (getDirectionCount (0.0) == 0 && getDirectionCount (defaultVariation) == 2 && getDirectionCount (1.0) == 3,
           "no ideas at the lowest variation, two by default, three at full");

    Request r;
    r.key = { 9, MusicalKey::Scale::minor };
    r.bars = 4;
    bool allParts = true;

    for (int p = 0; p < numParts; ++p)
    {
        r.part = (Part) p;
        juce::Random random (7);
        const auto ideas = pickDirections (r, 10, random);
        juce::StringArray unique (ideas);
        unique.removeDuplicates (false);
        allParts = allParts && ideas.size() >= 2 && unique.size() == ideas.size();
    }

    check (allParts, "every part has ideas of at least two kinds, never the same idea twice");

    r.part = Part::melody;
    juce::Random a (42), b (42);
    const auto picked = pickDirections (r, 2, a);
    check (picked.size() == 2 && picked == pickDirections (r, 2, b), "the same seed picks the same ideas");
    juce::Random none (1);
    check (pickDirections (r, 0, none).isEmpty(), "no ideas when none are asked for");

    std::set<juce::String> seen;
    for (int seed = 1; seed <= 20; ++seed)
    {
        juce::Random random (seed);
        seen.insert (pickDirections (r, 2, random).joinIntoString ("|"));
    }
    check (seen.size() >= 10, "different seeds give different ideas");

    r.directions = picked;
    const auto prompt = buildUserPrompt (r);
    check (prompt.contains ("Ideas for this take") && prompt.contains ("- " + picked[0]) && prompt.contains ("- " + picked[1]),
           "the ideas are listed in the prompt");
    r.directions.clear();
    check (! buildUserPrompt (r).contains ("Ideas for this take"), "no ideas section without ideas");

    // Scoring: a varied melody on chord tones beats one note hammered on every 16th.
    r.chordTrack = { Chord::fromName ("Am"), Chord::fromName ("F"), Chord::fromName ("C"), Chord::fromName ("G") };
    Result good, flat, empty;
    const int line[] = { 69, 72, 76, 74, 72, 69, 65, 69, 72, 67, 64, 67, 71, 74, 67, 62 };
    for (int i = 0; i < 16; ++i)
        good.notes.push_back ({ line[i], i * 1.0 + (i % 4 == 3 ? 0.5 : 0.0), i % 4 == 3 ? 0.5 : 0.75, 0.6f + 0.1f * (float) (i % 3) });
    for (int i = 0; i < 64; ++i)
        flat.notes.push_back ({ 71, i * 0.25, 0.25, 0.8f });

    const auto goodScore = scoreTake (r, good), flatScore = scoreTake (r, flat);
    check (goodScore > 0.7 && goodScore <= 1.0, "a varied melody on chord tones scores high (" + juce::String (goodScore, 2) + ")");
    check (flatScore < goodScore - 0.25, "one note on every 16th scores much lower (" + juce::String (flatScore, 2) + ")");
    check (scoreTake (r, empty) <= 0.0, "an empty take scores zero");

    // Drums: a groove with a kick, backbeat and hats with accents beats a wall of flat shakers.
    r.part = Part::drums;
    Result groove, wall;
    for (int step = 0; step < 32; ++step)
    {
        if (step % 4 == 0) groove.notes.push_back ({ 36, step * 0.25, 0.25, 0.9f });
        if (step % 8 == 4) groove.notes.push_back ({ 38, step * 0.25, 0.25, 0.85f });
        if (step % 2 == 0) groove.notes.push_back ({ 42, step * 0.25, 0.25, step % 4 == 2 ? 0.8f : 0.5f });
        if (step == 30) groove.notes.push_back ({ 38, step * 0.25, 0.25, 0.35f });
        wall.notes.push_back ({ 70, step * 0.25, 0.25, 0.8f });
    }
    std::sort (groove.notes.begin(), groove.notes.end(), [] (auto& x, auto& y) { return x.start < y.start; });
    const auto grooveScore = scoreTake (r, groove), wallScore = scoreTake (r, wall);
    check (grooveScore > 0.8, "a full drum groove scores high (" + juce::String (grooveScore, 2) + ")");
    check (wallScore < 0.4, "a wall of flat shakers scores low as drums (" + juce::String (wallScore, 2) + ")");

    r.part = Part::percussion;
    Result congas;
    for (int step = 0; step < 32; ++step)
        if (step % 3 == 0 || step % 8 == 6)
            congas.notes.push_back ({ step % 3 == 0 ? 63 : 64, step * 0.25, 0.25, step % 6 == 0 ? 0.9f : 0.55f });
    for (int step = 2; step < 32; step += 4)
        congas.notes.push_back ({ 82, step * 0.25, 0.25, 0.6f });
    std::sort (congas.notes.begin(), congas.notes.end(), [] (auto& x, auto& y) { return x.start < y.start; });
    check (scoreTake (r, congas) > scoreTake (r, wall) + 0.3, "percussion with gaps and accents beats a shaker on every 16th");
}

static void runSplitJoinTests()
{
    using namespace daw;
    std::cout << "Split and join clips" << std::endl;

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* track = engine.addTrack (Track::Type::midi);
    auto* clip = engine.addMidiClip (*track, 4.0, 8.0);
    clip->name = "Verse";
    clip->notes = { { 60, 0.0, 1.0, 0.8f }, { 62, 3.0, 2.0, 0.7f }, { 64, 4.0, 1.0, 0.6f }, { 65, 7.5, 2.0, 0.5f } };

    check (! engine.canSplit (*clip, 4.0) && ! engine.canSplit (*clip, 12.0) && engine.canSplit (*clip, 8.0),
           "a clip splits only strictly inside it");
    check (engine.splitClip (*track, *clip, 13.0) == nullptr && track->clips.size() == 1, "splitting outside the clip does nothing");

    engine.setClipMuted (*clip, true);
    auto* right = static_cast<MidiClip*> (engine.splitClip (*track, *clip, 8.0));
    check (right != nullptr && track->clips.size() == 2, "splitting adds a clip");
    check (right->muted, "both parts of a muted clip stay muted");
    engine.setClipMuted (*clip, false);
    engine.setClipMuted (*right, false);
    check (juce::exactlyEqual (clip->lengthBeats, 4.0) && juce::exactlyEqual (right->startBeat, 8.0)
               && juce::exactlyEqual (right->lengthBeats, 4.0) && right->name == "Verse",
           "the parts meet at the cut and keep the name");
    check (clip->notes.size() == 2 && juce::exactlyEqual (clip->notes[1].length, 1.0),
           "notes before the cut stay, and a note across it ends at the cut");
    check (right->notes.size() == 2 && right->notes[0].pitch == 64 && juce::exactlyEqual (right->notes[0].start, 0.0)
               && juce::exactlyEqual (right->notes[1].start, 3.5),
           "notes from the cut on move to the new clip");

    check (Engine::findNeighbourClip (*track, *clip, true) == right && Engine::findNeighbourClip (*track, *right, false) == clip
               && Engine::findNeighbourClip (*track, *clip, false) == nullptr && Engine::findNeighbourClip (*track, *right, true) == nullptr,
           "neighbours follow the clips' order on the track");

    auto* joined = engine.joinClips (*track, *right, *clip);
    check (joined == clip && track->clips.size() == 1 && juce::exactlyEqual (clip->startBeat, 4.0)
               && juce::exactlyEqual (clip->lengthBeats, 8.0),
           "joining keeps the earlier clip and spans both");
    check (clip->notes.size() == 4 && clip->notes[2].pitch == 64 && juce::exactlyEqual (clip->notes[2].start, 4.0)
               && juce::exactlyEqual (clip->notes[3].start, 7.5) && juce::exactlyEqual (clip->notes[3].length, 0.5),
           "joined notes keep their place; the note cut at the clip end stays cut");

    // A gap between the clips stays empty, and notes that couldn't be heard are dropped.
    auto* later = engine.addMidiClip (*track, 16.0, 2.0);
    later->notes = { { 67, 0.5, 4.0, 0.8f }, { 69, 3.0, 1.0, 0.8f } };
    joined = engine.joinClips (*track, *clip, *later);
    check (joined == clip && juce::exactlyEqual (clip->lengthBeats, 14.0) && clip->notes.size() == 5
               && juce::exactlyEqual (clip->notes[4].start, 12.5) && juce::exactlyEqual (clip->notes[4].length, 1.5),
           "joining across a gap keeps the notes where they were and drops the hidden ones");

    // Audio: at 120 BPM a beat is half a second, so at 1000 Hz a beat is 500 samples.
    auto* audioTrack = engine.addTrack (Track::Type::audio);
    check (engine.findNeighbourClip (*track, *clip, true) == nullptr, "MIDI and audio clips don't count as neighbours");

    auto addAudio = [&] (double start, double rate, int channels, int length, std::function<float (int, int)> sample)
    {
        auto c = std::make_unique<AudioClip>();
        c->name = "Loop";
        c->file = juce::File::getCurrentWorkingDirectory().getChildFile ("Loop.wav");
        c->startBeat = start;
        c->sourceSampleRate = rate;
        c->audio.setSize (channels, length);
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < length; ++i)
                c->audio.setSample (ch, i, sample (ch, i));
        c->buildPeaks();
        auto* raw = c.get();
        audioTrack->clips.push_back (std::move (c));
        return raw;
    };

    auto* loop = addAudio (2.0, 1000.0, 1, 4000, [] (int, int i) { return (float) i / 4000.0f; });
    check (! engine.canSplit (*loop, 2.0) && ! engine.canSplit (*loop, 10.0) && engine.canSplit (*loop, 4.0),
           "an audio clip splits only strictly inside it");

    auto* tail = static_cast<AudioClip*> (engine.splitClip (*audioTrack, *loop, 4.0));
    check (tail != nullptr && loop->audio.getNumSamples() == 1000 && tail->audio.getNumSamples() == 3000
               && juce::exactlyEqual (tail->startBeat, 4.0) && juce::exactlyEqual (tail->audio.getSample (0, 0), 0.25f)
               && tail->name == "Loop" && tail->peakMax.size() == (3000 + AudioClip::peakBlockSize - 1) / AudioClip::peakBlockSize,
           "splitting audio cuts the samples at the playhead");
    check (loop->file == juce::File() && tail->file == juce::File(), "split audio no longer points at the source file");

    {
        // Saving writes each part to its own file, and they load back the same.
        const auto folder = juce::File::createTempFile ("split");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
        const auto projectFile = folder.getChildFile ("Song.dawit");
        juce::String text, error;
        juce::StringArray warnings;
        check (project::save (engine, projectFile, text, error) && loop->file.existsAsFile() && tail->file.existsAsFile()
                   && loop->file != tail->file,
               "saving writes each part of split audio to a file " + error);

        Engine reloaded;
        reloaded.deviceManager.removeAudioCallback (&reloaded);
        check (project::load (reloaded, projectFile, error, warnings), "the project with split audio loads " + error);
        auto& clips = reloaded.getTracks()[1]->clips;
        check (clips.size() == 2 && static_cast<AudioClip&> (*clips[0]).audio.getNumSamples() == 1000
                   && static_cast<AudioClip&> (*clips[1]).audio.getNumSamples() == 3000 && juce::exactlyEqual (clips[1]->startBeat, 4.0),
               "split audio loads back as two clips");
    }

    check (engine.joinClips (*audioTrack, *tail, *loop) == loop && audioTrack->clips.size() == 1
               && loop->audio.getNumSamples() == 4000 && juce::exactlyEqual (loop->audio.getSample (0, 1000), 0.25f)
               && juce::exactlyEqual (loop->audio.getSample (0, 3999), 3999.0f / 4000.0f),
           "joining the parts gives back the audio");
    check (engine.joinClips (*audioTrack, *loop, *clip) == nullptr, "a MIDI and an audio clip don't join");

    // A stereo clip at another sample rate, one beat after the end: silence fills the gap and
    // the mono clip is copied to both channels.
    addAudio (11.0, 2000.0, 2, 2000, [] (int ch, int) { return ch == 0 ? 0.5f : 0.25f; });
    check (engine.joinClips (*audioTrack, *loop, *audioTrack->clips[1]) == loop && loop->audio.getNumChannels() == 2
               && loop->audio.getNumSamples() == 5500 && juce::exactlyEqual (loop->audio.getSample (1, 100), loop->audio.getSample (0, 100))
               && juce::exactlyEqual (loop->audio.getSample (0, 4200), 0.0f)
               && std::abs (loop->audio.getSample (0, 5000) - 0.5f) < 1.0e-4f && std::abs (loop->audio.getSample (1, 5000) - 0.25f) < 1.0e-4f,
           "joining across a gap at another sample rate keeps the timing");

    // Overlapping clips are mixed, as they sounded.
    auto* a = addAudio (20.0, 1000.0, 1, 1000, [] (int, int) { return 0.1f; });
    addAudio (21.0, 1000.0, 1, 1000, [] (int, int) { return 0.2f; });
    check (engine.joinClips (*audioTrack, *a, *audioTrack->clips[2]) == a && a->audio.getNumSamples() == 1500
               && std::abs (a->audio.getSample (0, 700) - 0.3f) < 1.0e-6f && std::abs (a->audio.getSample (0, 1200) - 0.2f) < 1.0e-6f,
           "overlapping audio is mixed when joined");

    auto* ramp = addAudio (30.0, 1000.0, 2, 1000, [] (int ch, int i) { return (float) (i + ch * 1000); });
    engine.reverseClip (*ramp);
    check (juce::exactlyEqual (ramp->audio.getSample (0, 0), 999.0f) && juce::exactlyEqual (ramp->audio.getSample (0, 999), 0.0f)
               && juce::exactlyEqual (ramp->audio.getSample (1, 0), 1999.0f) && ramp->audio.getNumSamples() == 1000
               && juce::exactlyEqual (ramp->startBeat, 30.0) && ramp->file == juce::File(),
           "reversing plays every channel backwards in the same place, and the clip leaves its file");
    engine.reverseClip (*ramp);
    check (juce::exactlyEqual (ramp->audio.getSample (0, 0), 0.0f) && juce::exactlyEqual (ramp->audio.getSample (1, 999), 1999.0f),
           "reversing twice gives back the audio");

    // Clip gain: split parts keep it, and a joined clip sounds as the two did at their own gains.
    auto* quiet = addAudio (40.0, 1000.0, 1, 2000, [] (int, int) { return 0.4f; });
    engine.setClipGain (*quiet, 30.0f);
    check (juce::exactlyEqual (quiet->gainDb, AudioClip::maxGainDb), "clip gain is limited to +12 dB");
    engine.setClipGain (*quiet, -6.0f);
    auto* quietTail = static_cast<AudioClip*> (engine.splitClip (*audioTrack, *quiet, 41.0));
    check (quietTail != nullptr && juce::exactlyEqual (quietTail->gainDb, -6.0f), "both parts of a split clip keep its gain");

    engine.setClipGain (*quietTail, 0.0f);
    engine.joinClips (*audioTrack, *quiet, *quietTail);
    const float heardFirst = quiet->audio.getSample (0, 100) * quiet->getGain(), heardSecond = quiet->audio.getSample (0, 1500) * quiet->getGain();
    check (juce::exactlyEqual (quiet->gainDb, -6.0f) && std::abs (heardFirst - 0.4f * juce::Decibels::decibelsToGain (-6.0f)) < 1.0e-6f
               && std::abs (heardSecond - 0.4f) < 1.0e-6f,
           "joining clips at different gains keeps how each part sounded");
}

static void runStemTests()
{
    using namespace daw;
    std::cout << "Stems" << std::endl;

    // Demucs wants 44.1 kHz stereo: mono is copied to both sides, other rates are resampled.
    juce::AudioBuffer<float> mono (1, 22050);
    for (int i = 0; i < mono.getNumSamples(); ++i)
        mono.setSample (0, i, (float) std::sin (juce::MathConstants<double>::twoPi * 100.0 * i / 22050.0));

    const auto prepared = stems::prepareInput (mono, 22050.0);
    check (prepared.getNumChannels() == 2 && prepared.getNumSamples() == 44100
               && juce::exactlyEqual (prepared.getSample (0, 1000), prepared.getSample (1, 1000))
               && std::abs (prepared.getSample (0, 1000) - (float) std::sin (juce::MathConstants<double>::twoPi * 100.0 * 1000 / 44100.0)) < 0.01f,
           "input is made 44.1 kHz stereo");
    check (stems::prepareInput (prepared, 44100.0).getNumSamples() == 44100, "44.1 kHz input keeps its length");

    // A stand-in for Demucs: stem s is the input times (s + 1) / 10, with the left and right
    // sides kept apart. Put back together, the pieces must give exactly that over the whole clip.
    juce::AudioBuffer<float> mix (2, 300000);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < mix.getNumSamples(); ++i)
            mix.setSample (ch, i, (float) std::sin (i * 0.001 + ch));

    std::atomic<int> calls { 0 };
    auto scale = [&calls] (const juce::AudioBuffer<float>& piece, const std::function<bool (double)>& keepGoing) -> std::optional<stems::Stems>
    {
        ++calls;
        stems::Stems out;
        for (int s = 0; s < stems::numStems; ++s)
        {
            out[(size_t) s].makeCopyOf (piece);
            out[(size_t) s].applyGain ((float) (s + 1) / 10.0f);
        }
        keepGoing (1.0);
        return out;
    };

    double lastProgress = -1.0;
    const auto split = stems::separateInPieces (mix, 3, scale, [&] (double p) { lastProgress = p; return true; });
    float worst = 0.0f;
    if (split.has_value())
        for (int s = 0; s < stems::numStems; ++s)
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < mix.getNumSamples(); ++i)
                    worst = juce::jmax (worst, std::abs ((*split)[(size_t) s].getSample (ch, i) - mix.getSample (ch, i) * (float) (s + 1) / 10.0f));

    check (split.has_value() && calls == 3 && (*split)[0].getNumSamples() == 300000 && worst < 1.0e-5f,
           "pieces are separated apart and joined back seamlessly (worst error " + juce::String (worst) + ")");
    check (lastProgress >= 0.0 && lastProgress <= 1.0, "progress stays between 0 and 1");
    check (stems::separateInPieces (mix, 1, scale, [] (double) { return true; }).has_value()
               && stems::separateInPieces (mix, 50, scale, [] (double) { return true; }).has_value(),
           "one piece, or more pieces than make sense, work too");

    // Cancelling stops the pieces that are still running; a failing piece fails the whole split.
    auto slow = [] (const juce::AudioBuffer<float>&, const std::function<bool (double)>& keepGoing) -> std::optional<stems::Stems>
    {
        for (int step = 0; step < 1000; ++step)
        {
            if (! keepGoing (step / 1000.0))
                return std::nullopt;

            juce::Thread::sleep (5);
        }
        return stems::Stems();
    };

    int reports = 0;
    const auto started = juce::Time::getMillisecondCounter();
    check (! stems::separateInPieces (mix, 4, slow, [&reports] (double) { return ++reports < 3; }).has_value()
               && juce::Time::getMillisecondCounter() - started < 2000,
           "cancelling stops the separation early");

    std::atomic<int> piecesRun { 0 };
    auto failSecond = [&] (const juce::AudioBuffer<float>& piece, const std::function<bool (double)>& keepGoing) -> std::optional<stems::Stems>
    {
        if (++piecesRun == 2)
            return std::nullopt;
        return scale (piece, keepGoing);
    };
    check (! stems::separateInPieces (mix, 3, failSecond, [] (double) { return true; }).has_value(), "a failing piece fails the split");
    check (stems::choosePieceCount (44100 * 4) == 1 && stems::choosePieceCount (44100 * 12) == juce::jmin (3, juce::SystemStats::getNumPhysicalCpus())
               && stems::choosePieceCount (44100 * 600) <= 8,
           "a few seconds run as one piece; longer clips are spread over the cores");

    // A stem far quieter than the loudest one is left out.
    stems::Stems levels;
    const float gains[] = { 0.5f, 0.012f, 0.02f, 0.0f };
    for (int s = 0; s < stems::numStems; ++s)
    {
        levels[(size_t) s].setSize (2, 1000);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 1000; ++i)
                levels[(size_t) s].setSample (ch, i, (i % 2 == 0 ? 1.0f : -1.0f) * gains[s]);
    }
    check (! stems::isSilent (levels, 0) && stems::isSilent (levels, 1) && ! stems::isSilent (levels, 2) && stems::isSilent (levels, 3),
           "stems more than 30 dB below the loudest count as silent");

    // The stems go on new tracks right below the clip's track, at the clip's position.
    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* first = engine.addTrack (Track::Type::midi);
    auto* source = engine.addTrack (Track::Type::audio);
    auto* last = engine.addTrack (Track::Type::audio);
    juce::AudioBuffer<float> song (2, 2000);
    song.clear();
    auto* clip = engine.addAudioClip (*source, std::move (song), 48000.0, 6.0, "Song");
    check (clip->file == juce::File() && juce::exactlyEqual (clip->sourceSampleRate, 48000.0) && clip->audio.getNumSamples() == 2000
               && clip->name == "Song" && ! clip->peakMax.empty(),
           "a clip can be added from audio in memory");

    auto added = stems::addStemTracks (engine, *source, *clip, levels);
    const auto& tracks = engine.getTracks();
    check (added.size() == 2 && tracks.size() == 5 && tracks[0].get() == first && tracks[1].get() == source
               && tracks[2].get() == added[0] && tracks[3].get() == added[1] && tracks[4].get() == last,
           "the stems that aren't silent go right below the source track");
    auto& drums = static_cast<AudioClip&> (*added[0]->clips[0]);
    check (added[0]->name == "Drums" && added[1]->name == "Other" && drums.name == "Song drums" && juce::exactlyEqual (drums.startBeat, 6.0)
               && juce::exactlyEqual (drums.sourceSampleRate, 44100.0) && drums.audio.getNumSamples() == 1000 && drums.file == juce::File(),
           "each stem track is named after its stem and starts where the clip does");
    check (clip->muted && ! source->muted.load(), "the source clip is muted, not its track");

    engine.addAudioClip (*last, juce::AudioBuffer<float> (1, 100), 44100.0, 0.0, "Other clip");
    engine.addAudioClip (*last, juce::AudioBuffer<float> (1, 100), 44100.0, 8.0, "Song");
    stems::addStemTracks (engine, *last, static_cast<AudioClip&> (*last->clips[1]), levels);
    check (last->clips[1]->muted && ! last->clips[0]->muted && ! last->muted.load() && engine.getTracks().size() == 7,
           "the other clips on the track keep playing");

    stems::Stems silence;
    for (auto& b : silence)
    {
        b.setSize (2, 100);
        b.clear();
    }
    check (stems::addStemTracks (engine, *last, static_cast<AudioClip&> (*last->clips[0]), silence).empty() && engine.getTracks().size() == 7
               && ! last->clips[0]->muted,
           "a silent clip gives no stems and stays unmuted");

    // One stem on its own, as Detect lyrics keeps the vocals it found in a full mix.
    clip->gainDb = -3.0f;
    auto* vocals = stems::addStemTrack (engine, engine.indexOf (source) + 1, *clip, 3, juce::AudioBuffer<float> (2, 500));
    auto& vocalClip = static_cast<AudioClip&> (*vocals->clips[0]);
    check (engine.getTracks()[2].get() == vocals && vocals->name == "Vocals" && vocalClip.name == "Song vocals"
               && juce::exactlyEqual (vocalClip.startBeat, 6.0) && juce::exactlyEqual (vocalClip.gainDb, -3.0f),
           "a single stem goes on a track right below, named after it, where the clip is and as loud");

    // Karaoke: made-up stems that add up to a mix, so drums + bass + other is the mix without the vocals.
    {
        stems::Stems parts;
        const float freqs[] = { 0.05f, 0.011f, 0.023f, 0.037f };
        juce::AudioBuffer<float> whole (2, 4000);
        whole.clear();

        for (int s = 0; s < stems::numStems; ++s)
        {
            parts[(size_t) s].setSize (2, 4000);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 4000; ++i)
                    parts[(size_t) s].setSample (ch, i, 0.2f * std::sin (freqs[s] * (float) i + (float) ch));

            for (int ch = 0; ch < 2; ++ch)
                whole.addFrom (ch, 0, parts[(size_t) s], ch, 0, 4000);
        }

        std::array<bool, stems::numStems> all;
        all.fill (true);
        std::array<bool, stems::numStems> noVocals = all;
        noVocals[(size_t) stems::vocalsStem] = false;

        const auto together = stems::mixStems (parts, all);
        const auto instrumental = stems::mixStems (parts, noVocals);
        float worstWhole = 0.0f, worstKaraoke = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 4000; ++i)
            {
                worstWhole = juce::jmax (worstWhole, std::abs (together.getSample (ch, i) - whole.getSample (ch, i)));
                worstKaraoke = juce::jmax (worstKaraoke, std::abs (instrumental.getSample (ch, i) + parts[(size_t) stems::vocalsStem].getSample (ch, i)
                                                                   - whole.getSample (ch, i)));
            }
        check (together.getNumChannels() == 2 && together.getNumSamples() == 4000 && worstWhole < 1.0e-6f && worstKaraoke < 1.0e-6f,
               "the stems mixed back give the mix, and without the vocals the mix minus the vocals");

        check (juce::exactlyEqual (stems::getGuideDb (stems::GuideVocals::off), 0.0f) && juce::exactlyEqual (stems::getGuideDb (stems::GuideVocals::quiet), -12.0f)
                   && juce::exactlyEqual (stems::getGuideDb (stems::GuideVocals::softer), -6.0f) && stems::getGuideName (stems::GuideVocals::quiet).contains ("-12 dB"),
               "the guide vocals are off, -12 dB or -6 dB");

        Engine withVocals;
        withVocals.deviceManager.removeAudioCallback (&withVocals);
        auto* band = withVocals.addTrack (Track::Type::audio);
        auto* after = withVocals.addTrack (Track::Type::midi);
        band->name = "Band";
        auto* recording = withVocals.addAudioClip (*band, juce::AudioBuffer<float> (whole), 44100.0, 8.0, "Song");
        recording->gainDb = -2.0f;

        stems::KaraokeChoices choices;
        auto karaoke = stems::addKaraokeTracks (withVocals, *band, *recording, parts, choices);
        const auto& songTracks = withVocals.getTracks();
        auto* karaokeClip = karaoke.instrumental.size() == 1 && ! karaoke.instrumental[0]->clips.empty()
                                ? static_cast<AudioClip*> (karaoke.instrumental[0]->clips[0].get()) : nullptr;
        check (karaokeClip != nullptr && karaoke.vocals != nullptr && songTracks.size() == 4 && songTracks[1].get() == karaoke.instrumental[0]
                   && songTracks[2].get() == karaoke.vocals && songTracks[3].get() == after,
               "the karaoke version goes right below the clip's track, with the vocals under it");
        check (karaokeClip != nullptr && karaoke.instrumental[0]->name == "Karaoke" && karaokeClip->name == "Song karaoke"
                   && juce::exactlyEqual (karaokeClip->startBeat, 8.0) && juce::exactlyEqual (karaokeClip->gainDb, -2.0f)
                   && karaokeClip->audio.getNumSamples() == 4000 && std::abs (karaokeClip->audio.getSample (1, 1234) - instrumental.getSample (1, 1234)) < 1.0e-6f,
               "the Karaoke track holds the drums, bass and other sounds together, where the clip was and as loud");
        check (karaoke.vocals != nullptr && karaoke.vocals->name == "Vocals" && karaoke.vocals->muted.load()
                   && juce::exactlyEqual (karaoke.vocals->volumeDb.load(), 0.0f) && recording->muted && ! band->muted.load(),
               "with the guide off, the vocals track is muted, and so is the original clip");

        // Saved with the project and loaded back.
        const auto folder = juce::File::createTempFile ("karaoke");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        juce::String text, error;
        juce::StringArray warnings;
        const bool ok = project::save (withVocals, folder.getChildFile ("Karaoke.dawit"), text, error)
                        && project::load (loaded, folder.getChildFile ("Karaoke.dawit"), error, warnings);
        const auto& back = loaded.getTracks();
        check (ok && back.size() == 4 && back[1]->name == "Karaoke" && back[2]->name == "Vocals" && back[2]->muted.load() && back[0]->clips[0]->muted
                   && ! back[1]->clips.empty() && static_cast<AudioClip&> (*back[1]->clips[0]).audio.getNumSamples() == 4000,
               "the karaoke version and the vocals are saved with the project " + error);

        // As stems, with quiet guide vocals.
        auto* second = withVocals.addAudioClip (*band, juce::AudioBuffer<float> (whole), 44100.0, 40.0, "Second");
        choices.asStems = true;
        choices.guide = stems::GuideVocals::quiet;
        karaoke = stems::addKaraokeTracks (withVocals, *band, *second, parts, choices);
        check (karaoke.instrumental.size() == 3 && karaoke.instrumental[0]->name == "Drums" && karaoke.instrumental[1]->name == "Bass"
                   && karaoke.instrumental[2]->name == "Other" && withVocals.getTracks()[4].get() == karaoke.vocals,
               "as stems, the drums, bass and other sounds go on tracks of their own, above the vocals");
        check (karaoke.vocals != nullptr && ! karaoke.vocals->muted.load() && juce::exactlyEqual (karaoke.vocals->volumeDb.load(), -12.0f) && second->muted,
               "quiet guide vocals play at -12 dB");

        // No singing: nothing is added and the clip keeps playing.
        stems::Stems instrumentalOnly = parts;
        instrumentalOnly[(size_t) stems::vocalsStem].clear();
        auto* third = withVocals.addAudioClip (*band, juce::AudioBuffer<float> (whole), 44100.0, 80.0, "Third");
        const auto before = withVocals.getTracks().size();
        karaoke = stems::addKaraokeTracks (withVocals, *band, *third, instrumentalOnly, choices);
        check (karaoke.vocals == nullptr && karaoke.instrumental.empty() && withVocals.getTracks().size() == before && ! third->muted,
               "a clip without vocals gets no karaoke version and stays unmuted");
    }
}

/** Splits a made-up mix of a kick drum and a bass line with the real model: the kicks should end
    up in the drums and the bass line in the bass. */
static void runDemucsTest (const juce::File& modelFile)
{
    using namespace daw;
    std::cout << "Demucs with " << modelFile.getFullPathName() << std::endl;

    stems::Separator separator;
    juce::String error;
    check (separator.load (modelFile, error), "the model loads " + error);

    const int length = (int) (stems::sampleRate * 12.0);
    juce::AudioBuffer<float> kicks (2, length), bass (2, length);
    kicks.clear();
    bass.clear();
    const int beat = (int) (stems::sampleRate * 0.5);

    for (int i = 0; i < length; ++i)
    {
        const int sinceKick = i % beat;
        const double t = sinceKick / stems::sampleRate;
        const float kick = (float) (0.8 * std::exp (-t * 30.0) * std::sin (juce::MathConstants<double>::twoPi * (50.0 * t + 400.0 * (1.0 - std::exp (-t * 40.0)) / 40.0)));

        const double note = (i / (beat * 2)) % 2 == 0 ? 55.0 : 73.4;   // A1 and D2, a bar each
        const float b = (float) (0.3 * (2.0 * std::fmod (note * i / stems::sampleRate, 1.0) - 1.0)) * (sinceKick > beat / 8 ? 1.0f : 0.5f);

        for (int ch = 0; ch < 2; ++ch)
        {
            kicks.setSample (ch, i, kick);
            bass.setSample (ch, i, b);
        }
    }

    juce::AudioBuffer<float> mix (2, length);
    for (int ch = 0; ch < 2; ++ch)
    {
        mix.copyFrom (ch, 0, kicks, ch, 0, length);
        mix.addFrom (ch, 0, bass, ch, 0, length);
    }

    const auto started = juce::Time::getMillisecondCounterHiRes();
    double progress = 0.0;
    const auto result = separator.separate (mix, [&progress] (double p) { progress = juce::jmax (progress, p); return true; });
    const auto seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
    check (result.has_value(), "12 seconds separate in " + juce::String (seconds, 1) + " s");

    if (! result.has_value())
        return;

    // How much of each part each stem holds: the correlation of the stem with the part.
    auto share = [&] (const juce::AudioBuffer<float>& stem, const juce::AudioBuffer<float>& part)
    {
        double dot = 0.0, energy = 0.0;
        for (int i = 0; i < length; ++i)
        {
            dot += (double) stem.getSample (0, i) * part.getSample (0, i);
            energy += juce::square ((double) part.getSample (0, i));
        }
        return dot / energy;
    };

    const auto& drums = (*result)[0];
    const auto& bassStem = (*result)[1];
    std::cout << "        kick in drums " << share (drums, kicks) << ", in bass " << share (bassStem, kicks)
              << "; bass in bass " << share (bassStem, bass) << ", in drums " << share (drums, bass) << std::endl;
    check (share (drums, kicks) > 0.7 && share (bassStem, kicks) < 0.3, "the kick drum goes to the drums");
    check (share (bassStem, bass) > 0.7 && share (drums, bass) < 0.3, "the bass line goes to the bass");
    check (progress > 0.9, "progress reaches the end");
}

static void runChordStyleTests()
{
    using namespace daw;
    namespace st = gen::styles;
    std::cout << "Chord styles" << std::endl;

    const auto& library = st::getLibrary();
    std::set<juce::String> ids;
    bool everyInstrument = true;

    for (auto& style : library)
        ids.insert (style.id);

    for (auto i : st::allInstruments)
        everyInstrument = everyInstrument && st::getStylesFor (i).size() >= 3;

    check (library.size() >= 30 && ids.size() == library.size(), "the built-in library reads, with " + juce::String ((int) library.size()) + " styles and unique ids");
    check (everyInstrument, "every instrument has at least three styles");

    {
        juce::String error;
        auto bad = st::parseStyle (juce::JSON::parse (R"({ "name": "X", "instrument": "piano", "events": [ { "start": 0, "role": "melody" } ] })"), error);
        check (! bad.has_value() && error.contains ("melody"), "an unknown role is refused and named");
        bad = st::parseStyle (juce::JSON::parse (R"({ "name": "X", "instrument": "piano", "events": [ { "start": 4, "role": "bass" } ] })"), error);
        check (! bad.has_value() && error.contains ("outside"), "an event past the end of the pattern is refused");
        bad = st::parseStyle (juce::JSON::parse (R"({ "name": "X", "instrument": "banjo", "events": [ { "start": 0, "role": "bass" } ] })"), error);
        check (! bad.has_value() && error.contains ("banjo"), "an unknown instrument is refused");
    }

    // C - Am - F - G, one bar each.
    const std::vector<Chord> progression { { 0, { 0, 4, 7 } }, { 9, { 0, 3, 7 } }, { 5, { 0, 4, 7 } }, { 7, { 0, 4, 7 } } };
    ChordMap chords;
    for (int bar = 0; bar < 4; ++bar)
        chords[bar] = progression[(size_t) bar];

    const auto spans = st::spansFromChordTrack (chords, 0, 4);
    check (spans.size() == 4 && juce::exactlyEqual (spans[1].start, 4.0) && juce::exactlyEqual (spans[1].end, 8.0) && spans[1].chord == progression[1],
           "the chord track becomes one span per bar");

    juce::StringArray offKey, overlapping, silent;

    for (auto& style : library)
    {
        const auto notes = st::play (style, spans, 16.0, 120.0);

        if (notes.empty())
            silent.add (style.name);

        std::map<int, double> soundingUntil;

        for (auto& n : notes)
        {
            // A note plays its bar's chord, or the next bar's when it is pushed in just before it.
            const int bar = juce::jlimit (0, 3, (int) std::floor (n.start / 4.0 + 1.0e-6));
            const int pushedBar = juce::jlimit (0, 3, (int) std::floor ((n.start + 0.5) / 4.0 + 1.0e-6));
            const bool approachNote = style.id == "bass-walking" && std::abs (std::fmod (n.start, 4.0) - 3.0) < 1.0e-6;
            const bool fits = progression[(size_t) bar].getPitchClasses().contains (n.pitch % 12)
                              || progression[(size_t) pushedBar].getPitchClasses().contains (n.pitch % 12);

            if (! fits && ! approachNote)
                offKey.addIfNotAlreadyThere (style.name);

            if (soundingUntil.count (n.pitch) > 0 && soundingUntil[n.pitch] > n.start + 1.0e-6)
                overlapping.addIfNotAlreadyThere (style.name);

            soundingUntil[n.pitch] = n.getEnd();
        }
    }

    check (silent.isEmpty(), "every style plays notes" + (silent.isEmpty() ? juce::String() : ": not " + silent.joinIntoString (", ")));
    check (offKey.isEmpty(), "every style plays only the chords' notes (and walking bass its approach notes)"
                                 + (offKey.isEmpty() ? juce::String() : ": not " + offKey.joinIntoString (", ")));
    check (overlapping.isEmpty(), "no style plays a pitch again while it still sounds"
                                      + (overlapping.isEmpty() ? juce::String() : ": " + overlapping.joinIntoString (", ")));

    {
        // Block chords: the root at the bottom of each bar, the right hand moving smoothly.
        const auto notes = st::play (*st::findStyle ("piano-block"), spans, 16.0, 120.0);
        bool rootsAtBottom = true, smooth = true;
        std::vector<int> previousUpper;

        for (int bar = 0; bar < 4; ++bar)
        {
            std::vector<int> barNotes;
            for (auto& n : notes)
                if (n.start >= bar * 4.0 - 1.0e-6 && n.start < bar * 4.0 + 0.5)
                    barNotes.push_back (n.pitch);

            std::sort (barNotes.begin(), barNotes.end());
            rootsAtBottom = rootsAtBottom && ! barNotes.empty() && barNotes.front() % 12 == progression[(size_t) bar].root && barNotes.front() < 52;

            const std::vector<int> upper (barNotes.begin() + 1, barNotes.end());
            if (! previousUpper.empty() && upper.size() == previousUpper.size())
                for (size_t i = 0; i < upper.size(); ++i)
                    smooth = smooth && std::abs (upper[i] - previousUpper[i]) <= 4;

            previousUpper = upper;
        }

        check (rootsAtBottom, "block chords have the root in the left hand");
        check (smooth, "the right hand moves each voice by four semitones at most between chords");
    }

    {
        // Folk strum: a down stroke runs low to high, 11 ms apart; an up stroke high to low on four strings at most.
        const auto notes = st::play (*st::findStyle ("guitar-folk"), spans, 4.0, 120.0);
        std::vector<MidiNote> down, up;
        for (auto& n : notes)
        {
            if (n.start < 0.2)
                down.push_back (n);
            else if (n.start >= 1.5 && n.start < 1.7)
                up.push_back (n);
        }

        bool downOk = down.size() >= 5, upOk = ! up.empty() && up.size() <= 4;
        for (size_t i = 1; i < down.size(); ++i)
            downOk = downOk && down[i].pitch > down[i - 1].pitch && std::abs (down[i].start - down[i - 1].start - 0.022) < 1.0e-6;
        for (size_t i = 1; i < up.size(); ++i)
            upOk = upOk && up[i].pitch < up[i - 1].pitch && up[i].start > up[i - 1].start;

        check (downOk, "a down strum plays the strings low to high, 11 ms apart");
        check (upOk, "an up strum plays at most four strings, high to low");
    }

    {
        // Open chord shapes as guitarists play them.
        auto shapeOf = [] (const char* name) { return st::findGuitarShape (*Chord::fromName (name)); };
        check (shapeOf ("C") == std::vector<int> { -1, 3, 2, 0, 1, 0 }, "C is the open C shape (x32010)");
        check (shapeOf ("Am") == std::vector<int> { -1, 0, 2, 2, 1, 0 }, "Am is x02210");
        check (shapeOf ("Em") == std::vector<int> { 0, 2, 2, 0, 0, 0 }, "Em is 022000");
        check (shapeOf ("D") == std::vector<int> { -1, -1, 0, 2, 3, 2 }, "D is xx0232");
        check (shapeOf ("E") == std::vector<int> { 0, 2, 2, 1, 0, 0 }, "E is 022100");
        check (shapeOf ("A") == std::vector<int> { -1, 0, 2, 2, 2, 0 }, "A is x02220");

        // Every chord quality on every root gets a playable shape.
        juce::StringArray missing, unplayable;

        for (int root = 0; root < 12; ++root)
        {
            for (auto& q : Chord::getQualities())
            {
                const Chord chord { root, q.intervals };
                const auto shape = st::findGuitarShape (chord);

                if (shape.empty())
                {
                    missing.add (chord.getName());
                    continue;
                }

                const auto pitches = st::getShapePitches (shape);
                int low = 99, high = 0, fretted = 0;
                for (int f : shape)
                    if (f > 0)
                    {
                        low = juce::jmin (low, f);
                        high = juce::jmax (high, f);
                        ++fretted;
                    }

                // With four notes or more, a shape may leave out the fifth.
                const int fifth = q.intervals.contains (7) ? 7 : q.intervals.contains (6) ? 6 : q.intervals.contains (8) ? 8 : 7;
                bool tones = true;
                for (int pc : chord.getPitchClasses())
                    tones = tones && (std::any_of (pitches.begin(), pitches.end(), [pc] (int p) { return p % 12 == pc; })
                                      || (chord.getPitchClasses().size() > 3 && pc == (root + fifth) % 12));

                if (pitches.size() < 4 || pitches.front() % 12 != root || (fretted > 0 && high - low > 3) || ! tones)
                    unplayable.add (chord.getName());
            }
        }

        check (missing.isEmpty(), "every chord quality on every root has a guitar shape" + (missing.isEmpty() ? juce::String() : ": not " + missing.joinIntoString (", ")));
        check (unplayable.isEmpty(), "shapes have four strings or more, the root in the bass, the chord's notes and a span of four frets at most"
                                         + (unplayable.isEmpty() ? juce::String() : ": not " + unplayable.joinIntoString (", ")));
    }

    {
        // A pushed chord, and a bar without a chord.
        juce::String error;
        const auto push = st::parseStyle (juce::JSON::parse (R"({ "name": "Push", "instrument": "synth",
            "events": [ { "start": 0, "length": 1, "role": "top" }, { "start": 3.5, "length": 1, "role": "top" } ] })"), error);
        ChordMap withGap { { 0, progression[0] }, { 1, progression[3] }, { 3, progression[1] } };
        const auto notes = st::play (*push, st::spansFromChordTrack (withGap, 0, 4), 16.0, 120.0);

        auto noteAt = [&notes] (double beat) -> const MidiNote*
        {
            for (auto& n : notes)
                if (std::abs (n.start - beat) < 1.0e-6)
                    return &n;

            return nullptr;
        };

        const auto* pushed = noteAt (3.5);
        check (pushed != nullptr && progression[3].getPitchClasses().contains (pushed->pitch % 12) && ! progression[0].getPitchClasses().contains (pushed->pitch % 12),
               "a note just before the next chord that rings on into it plays the next chord");
        check (std::none_of (notes.begin(), notes.end(), [] (auto& n) { return n.start >= 8.0 - 1.0e-6 && n.start < 11.5 - 1.0e-6; })
                   && std::all_of (notes.begin(), notes.end(), [] (auto& n) { return n.getEnd() <= 8.0 + 1.0e-6 || n.start >= 11.5 - 1.0e-6; }),
               "nothing plays in a bar without a chord");
        check (noteAt (11.5) != nullptr && progression[1].getPitchClasses().contains (noteAt (11.5)->pitch % 12),
               "a push from an empty bar plays the coming chord");
    }

    {
        // Walking bass: the fourth beat steps into the next chord's root a half step below.
        const auto notes = st::play (*st::findStyle ("bass-walking"), spans, 16.0, 120.0);
        bool approaches = true;
        for (auto& n : notes)
            if (std::abs (n.start - 3.0) < 1.0e-6 || std::abs (n.start - 7.0) < 1.0e-6)
            {
                const int nextRoot = progression[(size_t) (n.start < 4.0 ? 1 : 2)].root;
                approaches = approaches && (n.pitch + 1) % 12 == nextRoot;
            }

        check (approaches, "the walking bass's fourth beat leads into the next root");
    }

    {
        Engine engine;
        engine.addTrack (Track::Type::midi);
        engine.addTrack (Track::Type::audio);
        const auto& style = *st::findStyle ("guitar-reggae");
        auto* clip = st::addTrack (engine, style, st::play (style, spans, 16.0, 120.0), 8.0, 16.0, 1);
        auto* track = engine.findTrackFor (clip);

        check (track != nullptr && engine.indexOf (track) == 1 && track->builtInSound == BuiltInSound::pluck
                   && track->name == "Guitar - Reggae skank" && clip->name == "Reggae skank",
               "the part goes on a new track at the given place, with the style's sound and name");
        check (juce::exactlyEqual (clip->startBeat, 8.0) && juce::exactlyEqual (clip->lengthBeats, 16.0) && ! clip->notes.empty(),
               "its clip starts at the first bar and is as long as the bars");
    }
}

static void runPlayingStyleGeneratorTests()
{
    using namespace daw;
    using namespace daw::gen;
    namespace st = gen::styles;
    std::cout << "Generate panel with playing styles" << std::endl;

    // C - Am - F - G on the chord track, under a 4-bar clip.
    Request r;
    r.part = Part::chords;
    r.bars = 4;
    r.bpm = 100.0;
    r.style = "Pop";
    for (auto name : { "C", "Am", "F", "G" })
        r.chordTrack.push_back (Chord::fromName (name));

    r.playingStyle = "piano-ballad";
    r.varyStyle = false;
    check (r.usesPlayingStyle() && ! r.isChordRhythm() && ! r.needsModel(), "a style played as it is over the chord track needs no model");

    Result result;
    juce::String error;
    std::vector<st::ChordSpan> spans;
    for (int bar = 0; bar < 4; ++bar)
        spans.push_back ({ bar * 4.0, bar * 4.0 + 4.0, *r.chordTrack[(size_t) bar] });

    const auto expected = st::play (*st::findStyle ("piano-ballad"), spans, 16.0, 100.0);
    bool same = realize (r, "{}", result, error) && result.notes.size() == expected.size();
    for (size_t i = 0; same && i < expected.size(); ++i)
        same = result.notes[i].pitch == expected[i].pitch && juce::exactlyEqual (result.notes[i].start, expected[i].start);
    check (same && result.summary == "Ballad arpeggio over C - Am - F - G", "it plays the style's own pattern: " + result.summary + error);

    {
        auto melody = r;
        melody.part = Part::melody;
        check (! melody.usesPlayingStyle() && melody.needsModel(), "a melody ignores the playing style");

        auto withGap = r;
        withGap.chordTrack[2] = std::nullopt;
        check (withGap.needsModel() && withGap.asksForBarChords() && juce::JSON::toString (buildSchema (withGap)).contains ("\"chords\"")
                   && ! juce::JSON::toString (buildSchema (withGap)).contains ("\"events\""),
               "with a bar missing its chord, the model writes only the chords");

        Result filled;
        const bool ok = realize (withGap, R"({"chords":[{"degree":1,"colour":"triad"},{"degree":6,"colour":"triad"},{"degree":2,"colour":"triad"},{"degree":5,"colour":"triad"}]})",
                                 filled, error);
        check (ok && filled.newChords.size() == 4 && filled.newChords[2].has_value() && filled.newChords[2]->getName() == "Dm"
                   && std::any_of (filled.notes.begin(), filled.notes.end(), [] (auto& n) { return n.start >= 8.0 && n.start < 12.0 && n.pitch % 12 == 2; }),
               "and the style plays over the chord it wrote");
    }

    // Vary it: the model writes the pattern.
    r.varyStyle = true;
    check (r.needsModel() && r.getStyleBars() == 2, "a varied style asks the model for a two-bar pattern");

    const auto schema = juce::JSON::toString (buildSchema (r));
    const auto prompt = buildUserPrompt (r);
    check (schema.contains ("\"events\"") && schema.contains ("\"approach\"") && schema.contains ("\"strum\"") && ! schema.contains ("\"hits\""),
           "the schema asks for events with roles and strums");
    check (prompt.contains ("Ballad arpeggio") && prompt.contains (st::describeEvents (st::findStyle ("piano-ballad")->events))
               && prompt.contains ("variation") && ! prompt.contains ("Favour chord tones"),
           "the prompt names the style and shows its pattern");

    const juce::String reply = R"({"events":[
        {"start":0,"length":4,"role":"bass","index":0,"octave":0,"velocity":90,"strum":"none"},
        {"start":1,"length":1,"role":"upper","index":0,"octave":0,"velocity":70,"strum":"down"},
        {"start":2.5,"length":1,"role":"tone","index":2,"octave":1,"velocity":64,"strum":"none"},
        {"start":5,"length":1,"role":"melody","index":0,"octave":0,"velocity":64,"strum":"none"},
        {"start":9,"length":1,"role":"top","index":0,"octave":0,"velocity":64,"strum":"none"}]})";
    Result varied;
    check (realize (r, reply, varied, error) && varied.summary.startsWith ("Ballad arpeggio (varied, 3 events)"),
           "a varied pattern plays, leaving out events it can't use: " + varied.summary + error);
    check (std::all_of (varied.notes.begin(), varied.notes.end(), [&r] (auto& n)
           {
               return r.chordTrack[(size_t) juce::jlimit (0, 3, (int) (n.start / 4.0))]->getPitchClasses().contains (n.pitch % 12);
           }), "its notes are the chords' notes");

    Result nothing;
    check (! realize (r, R"({"events":[{"start":0,"length":1,"role":"solo","index":0,"octave":0,"velocity":90,"strum":"none"}]})", nothing, error)
               && error.contains ("no usable"),
           "a take with no usable events fails");

    // Scoring: a copy of the style isn't a variation, and a single note isn't the style any more.
    auto eventsOf = [] (const std::vector<st::Event>& events, int bars)
    {
        juce::String json ("{\"events\":[");
        juce::StringArray items;
        for (int bar = 0; bar < bars; ++bar)
            for (auto& e : events)
                items.add (juce::String ("{\"start\":") + juce::String (e.start + bar * 4.0) + ",\"length\":" + juce::String (e.length)
                           + ",\"role\":\"" + st::getRoleId (e.role) + "\",\"index\":" + juce::String (e.index) + ",\"octave\":" + juce::String (e.octave)
                           + ",\"velocity\":" + juce::String (juce::roundToInt (e.velocity * 127.0f)) + ",\"strum\":\"none\"}");
        return json + items.joinIntoString (",") + "]}";
    };

    auto ballad = st::findStyle ("piano-ballad")->events;
    auto changed = ballad;
    changed[3].start = 1.75;
    changed[5].index = 0;
    changed.push_back ({ 3.75, 0.25, st::Role::tone, 3, 0, 0.5f, st::Strum::none });

    // The variation keeps the style's first bar and answers it with the changed one.
    auto answered = ballad;
    for (auto e : changed)
    {
        e.start += 4.0;
        answered.push_back (e);
    }

    Result copy, variation, sparse;
    realize (r, eventsOf (ballad, 2), copy, error);
    realize (r, eventsOf (answered, 1), variation, error);

    realize (r, R"({"events":[{"start":0,"length":8,"role":"bass","index":0,"octave":0,"velocity":90,"strum":"none"}]})", sparse, error);

    const double copyScore = scoreTake (r, copy), variationScore = scoreTake (r, variation), sparseScore = scoreTake (r, sparse);
    check (variationScore > copyScore && variationScore > sparseScore,
           "a variation scores above a copy of the style and a single held note ("
               + juce::String (variationScore, 2) + ", " + juce::String (copyScore, 2) + ", " + juce::String (sparseScore, 2) + ")");

    // The panel's lists: which parts offer which styles.
    bool partsOk = true;
    for (auto& style : st::getLibrary())
        partsOk = partsOk && ! style.parts.isEmpty() && (style.instrument == st::Instrument::bass) == style.parts.contains ("bassline");
    check (partsOk && st::findStyle ("synth-arp-up")->parts.contains ("arpeggio") && ! st::findStyle ("synth-pad")->parts.contains ("arpeggio"),
           "bass styles are basslines, arpeggio styles also show for the arpeggio part");
    {
        auto bass = r;
        bass.part = Part::bassline;
        bass.playingStyle = "bass-walking";
        const auto bassSchema = juce::JSON::toString (buildSchema (bass));
        Result walked;
        realize (bass, R"({"events":[{"start":0,"length":1,"role":"bass","index":0,"octave":0,"velocity":90,"strum":"none"},
                                     {"start":1,"length":1,"role":"upper","index":0,"octave":0,"velocity":90,"strum":"none"}]})", walked, error);
        check (! bassSchema.contains ("\"upper\"") && bassSchema.contains ("\"approach\"") && ! buildUserPrompt (bass).contains ("\"upper\" the chord")
                   && walked.summary.contains ("1 events"),
               "a bass style only offers (and plays) one-note roles");
    }

    check (st::findStyle ("piano-house")->suitsGenre ("house") && ! st::findStyle ("piano-house")->suitsGenre ("Jazz"),
           "styles know the styles of music they suit");
}

static void runSectionTests()
{
    using namespace daw;
    namespace structure = gen::structure;
    std::cout << "Song sections and structure" << std::endl;

    {
        const auto tidy = tidySections ({ { 8, 8, SectionType::chorus, {} }, { 0, 12, SectionType::verse, {} },
                                          { 16, 0, SectionType::bridge, {} }, { 20, 4, SectionType::outro, {} }, { 20, 8, SectionType::outro, "End" } });
        check (tidy.size() == 3 && tidy[0].startBar == 0 && tidy[0].bars == 8 && tidy[1].type == SectionType::chorus
                   && tidy[2].name == "End" && tidy[2].bars == 8,
               "sections are sorted, overlaps trimmed, empty ones dropped, and a later one at the same bar wins");
    }

    {
        const auto replaced = replaceSections ({ { 0, 4, SectionType::intro, {} }, { 4, 8, SectionType::verse, {} }, { 12, 8, SectionType::chorus, {} },
                                                 { 24, 8, SectionType::outro, {} } },
                                               { { 6, 8, SectionType::verse, "Verse 1" }, { 14, 8, SectionType::chorus, "Chorus 1" } });
        check (replaced.size() == 4 && replaced[0].type == SectionType::intro && replaced[1].name == "Verse 1" && replaced[2].getEndBar() == 22
                   && replaced[3].type == SectionType::outro,
               "found sections replace the ones over the same bars and leave the others");
    }

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    engine.setSections ({ { 4, 8, SectionType::verse, {} }, { 0, 4, SectionType::intro, {} }, { 12, 8, SectionType::chorus, "Hook" } });
    check (engine.getSections().size() == 3 && engine.getSections()[0].type == SectionType::intro && engine.getSectionAt (13) != nullptr
               && engine.getSectionAt (13)->getDisplayName() == "Hook" && engine.getSectionAt (20) == nullptr
               && engine.getSectionAt (5)->getDisplayName() == "Verse",
           "the engine keeps the sections in order and finds the one at a bar");

    {
        const auto folder = juce::File::createTempFile ("sections");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
        const auto file = folder.getChildFile ("Song.dawit");
        juce::String text, error;
        juce::StringArray warnings;
        const bool saved = project::save (engine, file, text, error);
        const auto before = engine.getSections();

        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        const bool ok = project::load (loaded, file, error, warnings);
        check (saved && ok && text.contains ("\"sections\"") && loaded.getSections() == before, "sections are saved with the project and come back " + error);

        loaded.resetEdit();
        check (loaded.getSections().empty(), "a new project has no sections");

        file.replaceWithText (text.replace ("\"sections\"", "\"oldSections\""));
        check (project::load (loaded, file, error, warnings) && loaded.getSections().empty(), "a project from before sections opens without them");
    }

    // The structures.
    const auto& structures = structure::getStructures();
    bool wholePhrases = true, explained = true, recognised = true;
    std::set<juce::String> names;

    for (auto& s : structures)
    {
        names.insert (s.name);

        for (auto& part : s.parts)
            wholePhrases = wholePhrases && part.bars % 4 == 0;

        explained = explained && s.story.isNotEmpty() && s.tips.isNotEmpty() && s.styles.isNotEmpty()
                    && structure::explainStructure (s, 120.0).contains (s.story);
        recognised = recognised && structure::findStructureName (s.toSections (8)) == s.name;
    }

    check (structures.size() >= 10 && names.size() == structures.size(), juce::String ((int) structures.size()) + " structures, each with its own name");
    check (wholePhrases, "every section of every structure is a whole number of 4-bar phrases");
    check (explained, "every structure explains why it works, where it is used and where to start");
    check (recognised, "every structure is recognised from its sections");

    const auto pop = structures.front().toSections (16);
    bool contiguous = pop.front().startBar == 16;
    for (size_t i = 1; i < pop.size(); ++i)
        contiguous = contiguous && pop[i].startBar == pop[i - 1].getEndBar();

    check (contiguous && pop.back().getEndBar() == 16 + structures.front().getTotalBars(), "a structure is laid out bar after bar from where it starts");
    check (pop[1].getDisplayName() == "Verse 1" && pop[4].getDisplayName() == "Verse 2" && pop[0].getDisplayName() == "Intro",
           "repeated sections are numbered, single ones are not");

    const auto* hipHop = std::find_if (structures.begin(), structures.end(), [] (auto& s) { return s.name.startsWith ("Hip-hop"); }).base();
    check (hipHop != nullptr && hipHop->toSections (0)[1].getDisplayName() == "Hook 1", "structures can name their sections (a hip-hop hook)");

    check (structure::formatDuration (64, 120.0) == "2:08" && structure::formatDuration (3, 120.0) == "0:06", "durations read as minutes and seconds");

    bool allTypes = true;
    const auto glossary = structure::explainSections();
    for (auto t : sectionTypes::all)
        allTypes = allTypes && glossary.contains (sectionTypes::getName (t)) && structure::describeSection (t).isNotEmpty();
    check (allTypes && structure::getIntroduction().contains ("phrase"), "every section type is explained, and so are bars and phrases");

    {
        // A song with a late chorus, an odd length, a gap and no outro.
        const SectionList song { { 0, 16, SectionType::intro, {} }, { 16, 16, SectionType::verse, {} }, { 32, 6, SectionType::preChorus, {} },
                                 { 40, 8, SectionType::chorus, {} } };
        const auto text = structure::explainSongStructure (song, 100.0);
        check (text.contains ("Pre-chorus is 6 bars") && text.contains ("Bars 39-40 have no section") && text.contains ("starts at 1:36")
                   && text.contains ("only once") && text.contains ("no outro"),
               "the song's structure gets advice: odd lengths, gaps, a late chorus, a single chorus, no outro");
        check (structure::explainSongStructure (structures.front().toSections (0), 110.0).contains ("This is the Pop song"),
               "a known structure is named");
        check (structure::explainSongStructure ({}, 120.0).contains ("No sections yet"), "an empty section track says how to start");
    }
}

static void runChordEditingTests()
{
    using namespace daw;
    std::cout << "Copy and duplicate chords" << std::endl;

    const auto C = *Chord::fromName ("C"), Am = *Chord::fromName ("Am"), F = *Chord::fromName ("F"), G = *Chord::fromName ("G"),
               Dm = *Chord::fromName ("Dm");
    const ChordMap song { { 0, C }, { 1, Am }, { 3, G }, { 8, Dm } };

    const auto copy = chordEdits::copyBars (song, 0, 4);
    check (copy.bars == 4 && copy.chords.size() == 3 && copy.chords.at (1) == Am && copy.chords.count (2) == 0,
           "copying bars keeps their chords and the gap between them");
    check (chordEdits::toText (copy) == "C Am - G", "copied bars read as text, a dash for a bar without a chord");

    {
        auto chords = song;
        chordEdits::pasteBars (chords, copy, 4);
        check (chords.size() == 7 && chords.at (4) == C && chords.at (7) == G && chords.at (8) == Dm,
               "duplicating 4 bars after themselves makes 8, and leaves the bars after alone");
        check (chordEdits::countReplaced (song, copy, 4) == 0, "duplicating onto empty bars has nothing to ask about");
    }

    {
        auto chords = song;
        check (chordEdits::countReplaced (chords, copy, 7) == 1 && chordEdits::countReplaced (chords, copy, 0) == 0,
               "pasting counts the other chords it replaces, not the same ones");
        check (chordEdits::countReplaced (chords, copy, 6) == 1, "a gap in the copy landing on a chord counts as replacing it");
        chordEdits::pasteBars (chords, copy, 6);
        check (chords.count (8) == 0 && chords.at (9) == G, "a gap in the copy clears the bar it lands on");
    }

    {
        auto chords = song;
        chordEdits::moveBars (chords, 0, 2, 1);
        check (chords.count (0) == 0 && chords.at (1) == C && chords.at (2) == Am && chords.at (3) == G,
               "moving bars onto themselves shifts them and clears the bar they left");
        chordEdits::pasteBars (chords, chordEdits::copyBars (chords, 1, 2), -1);
        check (chords.count (-1) == 0 && chords.at (0) == Am, "nothing is pasted before the first bar");
    }

    {
        ChordMap chords { { 0, C }, { 1, Am }, { 2, F }, { 3, G }, { 13, Dm } };
        const int written = chordEdits::repeatUntil (chords, 0, 4, 14);
        check (written == 10 && chords.at (4) == C && chords.at (11) == G && chords.at (12) == C && chords.at (13) == Am,
               "repeating 4 bars until bar 14 fills 10 bars, the last repeat cut short");
        check (chordEdits::repeatUntil (chords, 0, 4, 4) == 0, "nothing to repeat when the song ends with the selection");
    }

    {
        const auto sheet = chordEdits::fromText ("| Am | F | C | G |");
        check (sheet.bars == 4 && sheet.chords.at (0) == Am && sheet.chords.at (3) == G, "chords copied as text from a chord sheet paste");
        const auto repeats = chordEdits::fromText ("Dm7, G7 % -  Cmaj7");
        check (repeats.bars == 5 && repeats.chords.at (2).getName() == "G7" && repeats.chords.count (3) == 0
                   && repeats.chords.at (4).getName() == "Cmaj7",
               "commas, % for the chord again and - for an empty bar");
        check (chordEdits::fromText ("Hello there").isEmpty() && chordEdits::fromText ("").isEmpty(),
               "other text on the clipboard isn't taken for chords");
        check (chordEdits::fromText (chordEdits::toText (copy)) == copy, "copied text pastes back as the same bars");
    }

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    check (engine.getSongEndBar() == 0, "an empty song ends at bar 0");
    auto* track = engine.addTrack (Track::Type::midi);
    engine.addMidiClip (*track, 4.0, 13.0);
    check (engine.getSongEndBar() == 5, "the song ends after the bar its last clip ends in");
    engine.setSections ({ { 0, 8, SectionType::verse, {} } });
    check (engine.getSongEndBar() == 8, "a section further on makes the song longer");
}

static void runChordDiagramTests()
{
    using namespace daw;
    namespace diagrams = gen::diagrams;
    std::cout << "Piano and guitar chords" << std::endl;

    auto chord = [] (const char* name) { return *Chord::fromName (name); };
    auto firstShape = [&chord] (const char* name)
    {
        const auto shapes = diagrams::getGuitarShapes (chord (name));
        return shapes.empty() ? diagrams::GuitarShape() : shapes.front();
    };

    // Piano.
    check (diagrams::pianoInversion (chord ("C"), 0) == std::vector<int> { 60, 64, 67 }
               && diagrams::pianoInversion (chord ("C"), 1) == std::vector<int> { 64, 67, 72 }
               && diagrams::pianoInversion (chord ("C"), 2) == std::vector<int> { 55, 60, 64 },
           "C in root position, 1st and 2nd inversion, round middle C");
    check (diagrams::getNumInversions (chord ("G7")) == 4 && diagrams::getInversionName (3) == "3rd inversion",
           "a seventh chord has a 3rd inversion");
    check (diagrams::pianoInversion (chord ("Cadd9"), 0) == std::vector<int> { 60, 62, 64, 67 }, "a ninth is played as a second in close position");
    check (diagrams::pianoBass (chord ("C")) == 48 && diagrams::pianoBass (chord ("A")) == 45, "the left hand plays the root below the right hand");
    check (diagrams::findSmoothInversion (chord ("F"), { 60, 64, 67 }) == 2, "after C, F moves least in 2nd inversion (C F A)");
    check (diagrams::findSmoothInversion (chord ("G"), { 60, 64, 67 }) == 1 && diagrams::findSmoothInversion (chord ("G"), {}) == 0,
           "after C, G moves least in 1st inversion (B D G); with no chord before, root position");

    const auto voicings = diagrams::voiceProgression ({ chord ("C"), chord ("Am"), chord ("F"), chord ("G") });
    int largestJump = 0;
    for (size_t i = 1; i < voicings.size(); ++i)
        for (size_t n = 0; n < voicings[i].size() && n < voicings[i - 1].size(); ++n)
            largestJump = juce::jmax (largestJump, std::abs (voicings[i][n] - voicings[i - 1][n]));
    check (voicings.size() == 4 && voicings[1] == std::vector<int> { 60, 64, 69 } && largestJump <= 3,
           "C Am F G is voiced so no note moves more than three semitones");

    // Guitar.
    const auto c = firstShape ("C");
    check (c.getTab() == "x32010" && c.fingers == std::vector<int> { 0, 3, 2, 0, 1, 0 } && c.isOpen() && ! c.hasBarre()
               && c.difficulty == diagrams::Difficulty::easy,
           "C is the open shape x32010 with fingers 3, 2 and 1, easy");

    bool openBeginnerChords = true;
    for (auto* name : { "C", "G", "D", "A", "E", "Am", "Em", "Dm" })
        openBeginnerChords = openBeginnerChords && firstShape (name).isOpen() && firstShape (name).difficulty == diagrams::Difficulty::easy;
    check (openBeginnerChords, "the beginner chords C, G, D, A, E, Am, Em and Dm come first as easy open shapes");

    const auto f = firstShape ("F");
    check (f.getTab() == "133211" && f.hasBarre() && f.barreFret == 1 && f.barreFrom == 0 && f.barreTo == 5
               && f.fingers == std::vector<int> { 1, 3, 4, 2, 1, 1 } && f.difficulty == diagrams::Difficulty::hard,
           "F is the full barre at fret 1, with fingers 1, 3, 4 and 2: hard");

    const auto smallF = diagrams::makeGuitarShape ({ -1, -1, 3, 2, 1, 1 });
    check (smallF.hasBarre() && smallF.barreFrom == 4 && smallF.barreTo == 5 && smallF.fingers[2] == 3 && smallF.difficulty == diagrams::Difficulty::medium,
           "the small F lays the index finger across the top two strings");
    check (! diagrams::makeGuitarShape ({ -1, -1, 0, 2, 3, 2 }).hasBarre(), "D isn't played with a barre");
    check (! diagrams::makeGuitarShape ({ 8, 7, 5, 5, 5, 8 }).isPlayable(), "a shape needing five fingers isn't playable");
    check (diagrams::makeGuitarShape ({ -1, 13, 15, 15, 15, 13 }).getTab() == "x 13 15 15 15 13"
               && diagrams::makeGuitarShape ({ -1, 7, 9, 9, 9, 7 }).getBaseFret() == 7 && c.getBaseFret() == 1,
           "tabs past fret 9 are spaced; boxes start at the nut or at the shape's lowest fret");

    bool allPlayable = true, someAlternatives = true;
    for (int root = 0; root < 12; ++root)
    {
        for (auto& q : Chord::getQualities())
        {
            const auto shapes = diagrams::getGuitarShapes ({ root, q.intervals });
            for (auto& s : shapes)
                allPlayable = allPlayable && s.isPlayable();
            someAlternatives = someAlternatives && ! shapes.empty();
        }
    }
    check (allPlayable && someAlternatives, "every chord DawIt offers has a guitar shape, and every shape shown fits four fingers");

    const auto progression = diagrams::shapeProgression ({ chord ("C"), chord ("Am"), chord ("F"), chord ("G") });
    check (progression.size() == 4 && progression[1].getTab() == "x02210", "a progression gets one shape per chord");

    check (diagrams::describeGuitarShape (f).contains ("barre chord") && diagrams::describeGuitarShape (c).contains ("Don't play string 6"),
           "shapes are described for beginners, barres and unplayed strings too");

    const auto aboutF = diagrams::explainPlaying (chord ("F"));
    check (aboutF.contains ("capo") && aboutF.contains ("fret 1") && aboutF.contains ("E shape") && aboutF.contains ("1st inversion"),
           "F gets a capo tip: on fret 1, the E shape sounds as F");
    check (diagrams::explainPlaying (chord ("Am")).contains ("shapes to learn first"), "easy chords are recommended to learn first");

    // Guides can always be heard: with no MIDI track to play them, previews use the engine's own synth.
    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    check (engine.previewsUseOwnSound(), "with no track selected, previews play on DawIt's own sound");
    auto* keys = engine.addTrack (Track::Type::midi);
    engine.setLiveMidiTrack (keys);
    check (! engine.previewsUseOwnSound(), "a selected MIDI track plays the previews");
    engine.setBuiltInSound (*keys, BuiltInSound::drums);
    check (engine.previewsUseOwnSound(), "a drum track doesn't: chords would play as drums");
    engine.setLiveMidiTrack (engine.addTrack (Track::Type::audio));
    check (engine.previewsUseOwnSound(), "nor does an audio track");
    engine.setLiveMidiTrack (nullptr);

    {
        Engine silent;
        silent.deviceManager.removeAudioCallback (&silent);
        FakeDevice device;
        silent.audioDeviceAboutToStart (&device);
        const float before = rms (render (silent, 0.2), 0);
        silent.setPreviewSound (BuiltInSound::pluck);
        silent.previewNote (60, 0.8f, 400);
        const auto heard = render (silent, 0.3);
        check (before < 1.0e-6f && rms (heard, 0) > 0.001f, "a chord guide preview with no track selected is heard");
        silent.audioDeviceStopped();
    }
}

static void runLyricsTests()
{
    using namespace daw;
    std::cout << "Lyrics" << std::endl;

    auto words = [] (const LyricLine& line)
    {
        juce::String text;

        for (auto& s : line.syllables)
            text << s.text << (s.endsWord ? " " : "-");

        return text.trimEnd();
    };

    auto beats = [] (const LyricLine& line)
    {
        juce::StringArray list;

        for (auto& s : line.syllables)
            list.add (juce::String (s.beat, 3).trimCharactersAtEnd ("0").trimCharactersAtEnd ("."));

        return list.joinIntoString (" ");
    };

    const juce::String text = "[Verse]\nI walked a-long the ri-ver\nand you were there\n\n[Chorus]\nto-ge-ther now\nall night\n\n[Verse]\nthe sun came up\nand we were gone";

    {
        const auto parsed = lyrics::parse ("  [Verse]\r\nI walked a-long the ri-ver  \nand you were there\n\n\n[Chorus]\nto-ge-ther now\nall night\n\n[Verse]\nthe sun came up\nand we were gone\n");
        check (parsed.lines.size() == 6 && parsed.lines[0].syllables.size() == 7 && words (parsed.lines[0]) == "I walked a-long the ri-ver"
                   && parsed.lines[0].startsVerse && parsed.lines[0].heading == "Verse" && ! parsed.lines[1].startsVerse
                   && parsed.lines[2].startsVerse && parsed.lines[2].heading == "Chorus" && ! parsed.lines[0].syllables[2].endsWord,
               "typed lyrics become lines of syllables, split at hyphens, in verses with their headings");
        check (lyrics::toText (parsed) == text, "and they're written back the same way: " + lyrics::toText (parsed).replace ("\n", " / "));
    }

    // An intro, then verse, chorus, verse, chorus of 8 bars each.
    const SectionList song { { 0, 4, SectionType::intro, {} }, { 4, 8, SectionType::verse, {} }, { 12, 8, SectionType::chorus, {} },
                             { 20, 8, SectionType::verse, {} }, { 28, 8, SectionType::chorus, {} } };

    {
        const auto placed = lyrics::place (lyrics::parse (text), song);
        check (placed.lines.size() == 8 && juce::exactlyEqual (placed.lines[0].startBeat, 16.0) && juce::exactlyEqual (placed.lines[1].startBeat, 32.0)
                   && juce::exactlyEqual (placed.lines[2].startBeat, 48.0) && juce::exactlyEqual (placed.lines[4].startBeat, 80.0)
                   && words (placed.lines[6]) == "to-ge-ther now" && juce::exactlyEqual (placed.lines[6].startBeat, 112.0),
               "verses go on their sections, each line on its own bars, and the chorus typed once goes on both choruses ("
                   + juce::String ((int) placed.lines.size()) + " lines)");
        check (beats (placed.lines[2]) == "48 49 50 51" && juce::exactlyEqual (placed.lines[2].endBeat, 52.0) && juce::exactlyEqual (placed.lines[0].endBeat, 23.0),
               "a new line is sung a syllable a beat at most: " + beats (placed.lines[2]));

        const auto plain = lyrics::place (lyrics::parse ("one two\nthree four\n\nfive six"), song);
        check (plain.lines.size() == 3 && juce::exactlyEqual (plain.lines[0].startBeat, 16.0) && juce::exactlyEqual (plain.lines[2].startBeat, 48.0),
               "verses without headings go in order on the sections that are sung, skipping the intro");

        const auto loose = lyrics::place (lyrics::parse ("one two\nthree four\n\nfive six"), {}, 2);
        check (loose.lines.size() == 3 && juce::exactlyEqual (loose.lines[0].startBeat, 8.0) && juce::exactlyEqual (loose.lines[1].startBeat, 16.0)
                   && juce::exactlyEqual (loose.lines[2].startBeat, 28.0),
               "without sections the lines follow two bars apart, with a bar between verses");
    }

    {
        Lyrics l;
        l.lines.push_back ({ 0.0, 4.0, true, {}, { { "one" }, { "two" }, { "three" }, { "four" } } });
        lyrics::tidy (l);
        check (beats (l.lines[0]) == "0 1 2 3", "unplaced syllables are spread evenly over the line: " + beats (l.lines[0]));

        lyrics::moveSyllable (l, { 0, 2 }, 3.0);
        check (beats (l.lines[0]) == "0 1.5 3 3.5" && l.lines[0].syllables[2].pinned,
               "dragging a word places it, and the words around it stretch or squeeze to follow: " + beats (l.lines[0]));

        lyrics::moveSyllable (l, { 0, 1 }, 10.0);
        check (l.lines[0].syllables[1].beat < 3.0 && l.lines[0].syllables[1].pinned, "a word can't be dragged past a placed one");
        lyrics::unpin (l, 0, 1);

        lyrics::moveSyllable (l, { 0, 0 }, 1.0);
        check (beats (l.lines[0]) == "1 2 3 3.5", "dragging the first word moves where the line starts: " + beats (l.lines[0]));

        lyrics::setLineEnd (l, 0, 5.0);
        check (beats (l.lines[0]) == "1 2 3 4", "and the end of the line moves the words after the last placed one: " + beats (l.lines[0]));

        lyrics::moveLine (l, 0, 9.0);
        check (beats (l.lines[0]) == "9 10 11 12" && juce::exactlyEqual (l.lines[0].endBeat, 13.0), "moving a line keeps its timing");

        lyrics::unpin (l, 0);
        check (beats (l.lines[0]) == "9 10 11 12" && ! l.lines[0].syllables[2].pinned, "and unpinning lets its words be spread again");
    }

    {
        // Timing survives editing the text: a word changed, a line added at the end.
        auto l = lyrics::place (lyrics::parse ("one two three\nfour five six"), {});
        lyrics::moveSyllable (l, { 0, 2 }, 2.5);
        lyrics::moveLine (l, 1, 9.0);

        const auto edited = lyrics::setText (l, "one two three\nfour FIVE, seven six\nan-o-ther line", {});
        check (edited.lines.size() == 3 && edited.lines[0].syllables[2].pinned && juce::exactlyEqual (edited.lines[0].syllables[2].beat, 2.5)
                   && juce::exactlyEqual (edited.lines[1].startBeat, 9.0) && juce::exactlyEqual (edited.lines[1].endBeat, l.lines[1].endBeat)
                   && words (edited.lines[1]) == "four FIVE, seven six",
               "editing the text keeps the timing of what didn't change: " + beats (edited.lines[0]) + " / " + beats (edited.lines[1]));
        check (juce::exactlyEqual (edited.lines[2].startBeat, 12.0) && words (edited.lines[2]) == "an-o-ther line",
               "and a new line follows the last one, on the next bar (" + juce::String (edited.lines[2].startBeat) + ")");

        const auto inserted = lyrics::setText (l, "one two three\nsomething new\nfour five six", {});
        check (inserted.lines.size() == 3 && words (inserted.lines[1]) == "something new" && inserted.lines[1].startBeat >= inserted.lines[0].endBeat
                   && inserted.lines[1].startBeat < 9.0 && juce::exactlyEqual (inserted.lines[2].startBeat, 9.0),
               "a line typed between two goes in the gap between them");

        const auto chorus = lyrics::setText (lyrics::place (lyrics::parse ("[Verse]\none two"), song), "[Verse]\none two\n\n[Chorus]\nla la", song);
        check (chorus.lines.size() == 3 && juce::exactlyEqual (chorus.lines[1].startBeat, 48.0) && juce::exactlyEqual (chorus.lines[2].startBeat, 112.0),
               "a chorus added later goes on the free choruses");
    }

    {
        // Tapping along: the first syllable moves the line, the others are placed one by one.
        auto l = lyrics::place (lyrics::parse ("one two three\nfour five"), {});
        lyrics::tap (l, { 0, 0 }, 2.0);
        lyrics::tap (l, { 0, 1 }, 2.75);
        check (juce::exactlyEqual (l.lines[0].startBeat, 2.0) && juce::exactlyEqual (l.lines[0].syllables[1].beat, 2.75) && l.lines[0].syllables[1].pinned
                   && l.lines[0].syllables[2].beat > 2.75,
               "a tap on a line's first syllable moves the line, the next one is placed where it was tapped: " + beats (l.lines[0]));
        lyrics::tap (l, { 0, 2 }, 3.5);
        check (juce::exactlyEqual (l.lines[0].endBeat, 4.25), "tapping the last syllable ends the line just after it");

        const auto next = lyrics::getNext (l, { 0, 2 });
        const auto at = lyrics::findAt (l, 3.0);
        check (next.has_value() && *next == lyrics::Position { 1, 0 } && ! lyrics::getNext (l, { 1, 1 }).has_value()
                   && at.has_value() && *at == lyrics::Position { 0, 1 } && ! lyrics::findAt (l, 6.0).has_value()
                   && lyrics::findFrom (l, 4.0).has_value() && *lyrics::findFrom (l, 4.0) == lyrics::Position { 1, 0 },
               "the next syllable to tap, and the one sung at a beat");
    }

    {
        // Saved in the project: placed syllables with their beat, the others spread again when loading.
        const auto folder = juce::File::createTempFile ("lyrics");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        Engine engine, loaded;
        engine.deviceManager.removeAudioCallback (&engine);
        loaded.deviceManager.removeAudioCallback (&loaded);
        engine.setSections (song);
        auto l = lyrics::place (lyrics::parse (text), song);
        lyrics::moveSyllable (l, { 0, 3 }, 18.5);
        engine.setLyrics (l);

        juce::String saved, error;
        juce::StringArray warnings;
        const auto file = folder.getChildFile ("Song.dawit");
        const bool ok = project::save (engine, file, saved, error) && project::load (loaded, file, error, warnings);
        check (ok && loaded.getLyrics() == engine.getLyrics() && saved.contains ("\"lyrics\"") && saved.contains ("\"joined\": true"),
               "the lyrics are saved in the project and load with the same timing " + error);

        loaded.resetEdit();
        check (loaded.getLyrics().isEmpty(), "a new project has no lyrics");

        // The song brief for the lyrics AI.
        {
            Engine withBrief, reloaded;
            withBrief.deviceManager.removeAudioCallback (&withBrief);
            reloaded.deviceManager.removeAudioCallback (&reloaded);
            withBrief.setLyricsBrief ({ "  a night walk after a break-up ", "longing", "bogus" });
            check (withBrief.getLyricsBrief().about == "a night walk after a break-up" && withBrief.getLyricsBrief().pointOfView.isEmpty(),
                   "lyrics brief: the text is trimmed and an unknown point of view becomes Any");

            withBrief.setLyricsBrief ({ "a night walk after a break-up", "longing", "i" });
            juce::String briefSaved, briefError;
            juce::StringArray briefWarnings;
            const auto briefFile = folder.getChildFile ("Brief.dawit");
            const bool briefOk = project::save (withBrief, briefFile, briefSaved, briefError) && project::load (reloaded, briefFile, briefError, briefWarnings);
            check (briefOk && reloaded.getLyricsBrief() == withBrief.getLyricsBrief() && briefSaved.contains ("\"lyricsBrief\"") && briefSaved.contains ("\"pointOfView\": \"i\""),
                   "lyrics brief: it is saved in the project and loads again " + briefError);

            reloaded.resetEdit();
            check (reloaded.getLyricsBrief().isEmpty(), "lyrics brief: a new project has none");

            juce::String plainSaved, plainError;
            const auto plainFile = folder.getChildFile ("NoBrief.dawit");
            const bool plainOk = project::save (loaded, plainFile, plainSaved, plainError) && project::load (withBrief, plainFile, plainError, briefWarnings);
            check (plainOk && ! plainSaved.contains ("lyricsBrief") && withBrief.getLyricsBrief().isEmpty(),
                   "lyrics brief: a project without one is saved without the key and loads empty " + plainError);
        }
    }
}

static void runRhymeTests()
{
    namespace rh = daw::gen::rhyme;
    using K = rh::Kind;
    std::cout << "Rhymes and syllables" << std::endl;

    auto kind = [] (const char* a, const char* b) { return rh::classify (a, b); };
    check (kind ("night", "light") == K::perfect && kind ("polite", "night") == K::perfect && kind ("river", "shiver") == K::perfect,
           "rhyme: night / light, polite / night and river / shiver are perfect");
    check (kind ("night", "knight") == K::identical && kind ("night", "tonight") == K::identical && kind ("leave", "believe") == K::identical,
           "rhyme: night / knight, night / tonight and leave / believe are identical");
    check (kind ("night", "ride") == K::family && kind ("feel", "heels") == K::additive && kind ("night", "time") == K::assonance
               && kind ("home", "coat") == K::assonance && kind ("night", "boat") == K::consonance && kind ("night", "table") == K::none,
           "rhyme: family (night / ride), additive (feel / heels), assonance (night / time, home / coat), consonance (night / boat), none (night / table)");
    check (kind ("night", "zzyzx") == K::unknown && rh::getRhymePart ("zzyzx", true).guessed && ! rh::getRhymePart ("zzyzx").found,
           "rhyme: a word that is not in the dictionary classifies as unknown and can be guessed from its spelling");

    const auto river = rh::getRhymePart ("river");
    check (river.toString() == "ih v er" && river.syllables == 2 && river.onset.size() == 1 && river.onset[0] == "r"
               && rh::getFamily ("t") == rh::Family::plosive && rh::getFamily ("sh") == rh::Family::fricative
               && rh::getFamily ("ng") == rh::Family::nasal && rh::getFamily ("l") == rh::Family::none,
           "rhyme: the rhyme part of river is ih v er over 2 syllables, and the consonant families");

    const char* examples[] { "I walked a-long the ri-ver late", "the ci-ty lights be-hind me", "I feel so sad and lone-ly now",
                             "your coat still hangs be-side the door", "two cups, and one is ne-ver full", "the emp-ty chair where you would sit" };
    const int expected[] { 8, 7, 8, 8, 8, 8 };
    bool counts = true;
    for (int i = 0; i < 6; ++i)
        counts = counts && rh::countSyllables (examples[i]) == expected[i];
    check (counts, "rhyme: the six example lines have 8, 7, 8, 8, 8 and 8 syllables");
    check (rh::countSyllables ("lonely") == 2 && rh::countSyllables ("lone-ly") == 2 && rh::countSyllables ("night - o") == 2 && rh::countSyllables ("") == 0,
           "rhyme: an unhyphenated word is counted by sound (lonely is 2) and a dash counts none");

    const auto pattern = rh::getStressPattern ("I feel so sad and lonely now");
    check (pattern == std::vector<bool> { false, true, false, true, false, true, false, true },
           "rhyme: \"I feel so sad and lonely now\" is weak-strong four times");

    const auto night = rh::findRhymes ("night", 400);
    auto has = [&] (const rh::RhymeList& l, const char* word, K k)
    {
        return std::any_of (l.items.begin(), l.items.end(), [&] (auto& r) { return r.word == word && r.kind == k; });
    };
    check (has (night, "light", K::perfect) && has (night, "bright", K::perfect) && has (night, "polite", K::perfect)
               && ! has (night, "night", K::perfect) && ! has (night, "knight", K::perfect) && ! has (night, "tonight", K::perfect)
               && has (night, "tonight", K::identical) && has (night, "ride", K::family) && ! night.guessed,
           "rhyme: the dictionary's rhymes for night have light, bright and polite as perfect, tonight as identical, ride as family");
    const auto unknown = rh::findRhymes ("zzyzx");
    check (unknown.guessed, "rhyme: rhymes for a word the dictionary lacks are flagged as guessed");

    const auto together = rh::hyphenate ("together forever, lonely");
    juce::StringArray words;
    for (auto& w : juce::StringArray::fromTokens (together, " ", ""))
        words.add (w);
    bool agree = words.size() == 3 && together.removeCharacters ("-") == "together forever, lonely";
    for (auto& w : words)
        agree = agree && (! w.contains ("-") || rh::countSyllables (w) == rh::countSyllables (w.removeCharacters ("-")));
    check (agree && together.startsWith ("to-ge-ther ") && rh::countSyllables (together) == 8 && rh::hyphenate ("one-way night") == "one-way night",
           "rhyme: hyphenating gives to-ge-ther where the counts agree (" + together + ")");
}

static void runLyricsAiTests()
{
    namespace ai = daw::gen::lyricsai;
    namespace rh = daw::gen::rhyme;
    using daw::lyrics::place;
    std::cout << "Lyrics AI" << std::endl;

    const juce::String pop = "[Verse]\nI walked a-long the ri-ver late\nthe ci-ty lights be-hind me\nyour voice still ring-ing in my head\nthe way it used to find me\n\n"
                             "[Chorus]\nhold on, hold on to-night\nwe could run be-fore the light";
    auto lineRange = [&] (const juce::String& line) { const int at = pop.indexOf (line); return std::pair<int, int> { at, at + line.length() }; };

    const daw::SectionList sections { { 0, 8, daw::SectionType::verse, {} }, { 8, 8, daw::SectionType::chorus, {} },
                                      { 16, 16, daw::SectionType::verse, {} }, { 32, 8, daw::SectionType::chorus, {} },
                                      { 40, 4, daw::SectionType::outro, {} } };

    auto makeRequest = [&] (ai::Kind kind, const juce::String& text, int a, int b)
    {
        ai::Request r;
        r.kind = kind;
        r.text = text;
        r.rangeStart = a;
        r.rangeEnd = b;
        r.sections = sections;
        r.key = { 9, daw::MusicalKey::Scale::minor };
        r.bpm = 90.0;
        r.brief = { "a night walk after a break-up", "longing", "i" };
        r.asks.moreConcrete = true;
        return r;
    };

    // Context.
    const auto line3 = lineRange ("your voice still ring-ing in my head");
    const auto line4 = lineRange ("the way it used to find me");
    auto reqLine3 = makeRequest (ai::Kind::selection, pop, line3.first, line3.second);
    const auto ctx3 = ai::buildContext (reqLine3);
    auto reqLine4 = makeRequest (ai::Kind::selection, pop, line4.first, line4.second);
    const auto ctx4 = ai::buildContext (reqLine4);
    check (ctx3.error.isEmpty() && ctx3.heading == "Verse" && ctx3.firstLine == 3 && ctx3.wholeLines && ctx3.lineSyllables == std::vector<int> { 8 }
               && ctx3.partners.size() == 1 && ctx3.partners[0].line == 0 && ctx3.scheme == "XAXA" && ctx3.target == "your voice still ring-ing in my head",
           "lyrics AI context: line 3 of the verse has 8 syllables and no rhyme partner (" + ctx3.scheme + ")");
    check (ctx4.firstLine == 4 && ctx4.partners[0].line == 2 && ctx4.partners[0].word == "me" && ctx4.endsLine,
           "lyrics AI context: line 4 rhymes with line 2");

    const auto words3 = ai::buildContext (makeRequest (ai::Kind::selection, pop, pop.indexOf ("ring-ing"), pop.indexOf ("ring-ing") + 8));
    check (! words3.wholeLines && words3.target == "ring-ing" && words3.lineSyllables == std::vector<int> { 2 } && ! words3.endsLine,
           "lyrics AI context: words inside a line are not whole lines");
    check (! ai::buildContext (makeRequest (ai::Kind::selection, pop, line3.first, line3.first)).error.isEmpty()
               && ! ai::buildContext (makeRequest (ai::Kind::selection, pop, line3.first, pop.indexOf ("hold on") + 3)).error.isEmpty(),
           "lyrics AI context: nothing selected, or lines of two verses, is an error");

    // Counts and the plan.
    check (ai::getLinesForBars (4) == 2 && ai::getLinesForBars (8) == 4 && ai::getLinesForBars (16) == 8 && ai::getLinesForBars (2) == 2 && ai::getLinesForBars (40) == 8,
           "lyrics AI: lines wanted for 4, 8, 16, 2 and 40 bars are 2, 4, 8, 2 and 8");
    const auto plan = ai::buildSongPlan (sections);
    check (plan.items.size() == 3 && plan.items[0].heading == "Verse" && plan.items[0].lines == 4 && plan.items[1].heading == "Chorus" && plan.items[2].heading == "Verse"
               && plan.items[2].lines == 8 && ! plan.isDefaultForm && ai::buildSongPlan ({}).isDefaultForm
               && ai::buildSongPlan ({}).note == "No sections yet: written as verse, chorus, verse, chorus, bridge, chorus" && ai::buildSongPlan ({}).order.size() == 6,
           "lyrics AI: the song plan has the chorus once and nothing for the outro, and a default form without sections");

    // Prompts and schemas.
    const auto prompt = ai::buildUserPrompt (reqLine3, ctx3);
    const auto songRequest = makeRequest (ai::Kind::song, "", 0, 0);
    const auto songPrompt = ai::buildUserPrompt (songRequest, ai::buildContext (songRequest));
    check (prompt.contains ("a night walk after a break-up") && prompt.contains ("longing") && prompt.contains ("A minor") && prompt.contains ("90 BPM")
               && prompt.contains ("Verse (8 bars, 4 lines)") && prompt.contains ("Chorus (8 bars, 4 lines)") && prompt.contains ("Verse (16 bars, 8 lines)")
               && ! prompt.contains ("Outro") && prompt.contains ("<<<your voice still ring-ing in my head>>>") && prompt.contains ("8 syllables")
               && prompt.contains ("see, hear or touch") && prompt.contains ("exactly 3 alternatives"),
           "lyrics AI prompt: the brief, key, tempo, sung sections with their lines, the marked target and the asks");
    int chorusItems = 0;
    for (int at = songPrompt.indexOf ("- Chorus: exactly"); at >= 0; at = songPrompt.indexOf (at + 1, "- Chorus: exactly"))
        ++chorusItems;
    check (chorusItems == 1 && songPrompt.contains ("- Verse: exactly 4 lines") && songPrompt.contains ("- Verse: exactly 8 lines"),
           "lyrics AI prompt: for the whole song the chorus is written once");
    check (ai::buildSystemPrompt().contains ("Never reuse lines from existing songs"), "lyrics AI prompt: the system prompt forbids reusing lines of existing songs");

    auto schemaCount = [] (const juce::var& schema, const char* list) { return (int) schema["properties"][list]["minItems"]; };
    const auto sectionRequest = makeRequest (ai::Kind::sectionRewrite, pop, 3, 3);
    const auto sectionContext = ai::buildContext (sectionRequest);
    const auto sectionSchema = ai::buildSchema (sectionRequest, sectionContext);
    check (schemaCount (ai::buildSchema (reqLine3, ctx3), "alternatives") == 3 && (int) ai::buildSchema (reqLine3, ctx3)["properties"]["alternatives"]["maxItems"] == 3
               && schemaCount (sectionSchema, "alternatives") == 2
               && (int) sectionSchema["properties"]["alternatives"]["items"]["properties"]["lines"]["minItems"] == 4
               && schemaCount (ai::buildSchema (songRequest, ai::buildContext (songRequest)), "sections") == 3
               && ai::getAlternativeCount (ai::Kind::song) == 1 && ai::getAlternativeCount (ai::Kind::sectionAdd) == 2,
           "lyrics AI schema: 3 alternatives for a line, 2 for a section, exact line counts, a section each for the song");

    // Parsing: numbering, quotes, a duplicate and the original go.
    const juce::String messy = R"({"alternatives":[
        {"lines":["1. your coat still hangs beside the door"],"note":"a picture instead of a feeling"},
        {"lines":["\"two cups, and one is never full\""],"note":"a detail"},
        {"lines":["your coat still hangs beside the door"],"note":"again"},
        {"lines":["your voice still ringing in my head"],"note":"the original"},
        {"lines":["- the empty chair where you would sit", ""],"note":"a place"}]})";
    const auto cleaned = ai::parseReply (reqLine3, ctx3, messy);
    check (cleaned.error.isEmpty() && cleaned.alternatives.size() == 3 && cleaned.alternatives[0].lines.size() == 1
               && cleaned.alternatives[0].lines[0].startsWith ("your coat still hangs") && cleaned.alternatives[1].lines[0] == "two cups, and one is ne-ver full"
               && cleaned.alternatives[2].lines[0].startsWith ("the em-pty chair") && cleaned.alternatives[0].note == "a picture instead of a feeling",
           "lyrics AI parse: numbering, quotes, an empty line, a duplicate and the original are dropped (" + juce::String ((int) cleaned.alternatives.size()) + ")");
    const auto cut = ai::parseReply (reqLine3, ctx3, R"({"alternatives":[{"lines":["your coat hangs by the door","two cups on the shelf"],"note":""}]})");
    check (cut.alternatives.size() == 1 && cut.alternatives[0].lines.size() == 1 && cut.alternatives[0].lines[0] == "your coat hangs by the door",
           "lyrics AI parse: 2 lines for a 1-line target are cut to the first");
    const auto twoLines = lineRange ("I walked a-long the ri-ver late\nthe ci-ty lights be-hind me");
    const auto ctx12 = ai::buildContext (makeRequest (ai::Kind::selection, pop, twoLines.first, twoLines.second));
    const auto tooFew = ai::parseReply (makeRequest (ai::Kind::selection, pop, twoLines.first, twoLines.second), ctx12,
                                        R"({"alternatives":[{"lines":["only one line"],"note":""}]})");
    check (ctx12.linesWanted == 2 && ctx12.wholeLines && tooFew.alternatives.empty() && tooFew.error.isNotEmpty(),
           "lyrics AI parse: 1 line for a 2-line target is rejected");
    check (ai::parseReply (reqLine3, ctx3, "not json at all").error.isNotEmpty() && ai::parseReply (reqLine3, ctx3, R"({"alternatives":5})").error.isNotEmpty()
               && ai::parseReply (reqLine3, ctx3, "```json\n{\"alternatives\":[{\"lines\":[\"your coat hangs by the door\"],\"note\":\"\"}]}\n```").alternatives.size() == 1,
           "lyrics AI parse: a malformed reply gives an error, one in a code fence is read");

    // Labels.
    const auto labelled = ai::parseReply (reqLine3, ctx3, R"({"alternatives":[
        {"lines":["your coat still hangs beside the door"],"note":""},
        {"lines":["your coat still hangs beside the old door"],"note":""},
        {"lines":["your coat still hangs beside the door where we used to stay"],"note":""}]})");
    check (labelled.alternatives.size() == 3 && labelled.alternatives[0].syllableLabel == "8 syllables, like the original" && ! labelled.alternatives[0].syllableWarn
               && labelled.alternatives[1].syllableLabel == "9 syllables, 1 more than the original" && ! labelled.alternatives[1].syllableWarn
               && labelled.alternatives[2].syllableWarn && labelled.alternatives[2].syllableLabel.startsWith ("13 syllables, 5 more than the original: harder to sing")
               && labelled.alternatives[0].rhymeLabel.isEmpty() && ! labelled.shouldRetry,
           "lyrics AI labels: the same count, 1 more, and 5 more in yellow; an X line has no rhyme label");
    check (ai::parseReply (reqLine3, ctx3, R"({"alternatives":[{"lines":["a long line that has far too many syllables in it all"],"note":""}]})").shouldRetry,
           "lyrics AI: no alternative within 2 syllables asks once more");

    const auto rhymed = ai::parseReply (reqLine4, ctx4, R"({"alternatives":[
        {"lines":["and nothing left for me to see"],"note":""},
        {"lines":["the way it used to feel like home"],"note":""}]})");
    check (rhymed.alternatives.size() == 2 && rhymed.alternatives[0].rhymeLabel == "ends on see: rhymes with me (line 2)" && ! rhymed.alternatives[0].rhymeWarn
               && rhymed.alternatives[1].rhymeLabel == "doesn't rhyme with me (line 2) any more" && rhymed.alternatives[1].rhymeWarn,
           "lyrics AI labels: it rhymes with its partner, or it doesn't any more (yellow)");

    const juce::String aabb = "I miss you every night\nand nothing feels right";
    const auto aabbLine = std::pair<int, int> { aabb.indexOf ("and nothing"), aabb.length() };
    const auto aabbRequest = makeRequest (ai::Kind::selection, aabb, aabbLine.first, aabbLine.second);
    const auto aabbContext = ai::buildContext (aabbRequest);
    const auto family = ai::parseReply (aabbRequest, aabbContext, R"({"alternatives":[{"lines":["I take the late bus for a ride"],"note":""}]})");
    check (aabbContext.partners[0].line == 1 && family.alternatives.size() == 1 && family.alternatives[0].rhymeLabel == "ends on ride: a family rhyme with night (line 1)",
           "lyrics AI labels: a family rhyme names its kind");

    const auto next = makeRequest (ai::Kind::nextLine, pop, line4.first - 1, line4.first - 1);   // the end of line 3
    const auto nextCtx = ai::buildContext (next);
    const auto nextReply = ai::parseReply (next, nextCtx, R"({"alternatives":[{"lines":["the rain keeps time with me"],"note":""},{"lines":["a door that wouldn't close"],"note":""}]})");
    check (nextCtx.lastLine == 3 && nextCtx.firstLine == 4 && nextCtx.previousSyllables == 8 && nextReply.alternatives.size() == 2
               && nextReply.alternatives[0].rhymeLabel == "an identical rhyme with me (line 2)" && nextReply.alternatives[1].rhymeLabel.isEmpty()
               && nextReply.alternatives[0].syllableLabel == "6 syllables, 2 fewer than the line before: harder to sing to the same melody" && nextReply.alternatives[0].syllableWarn,
           "lyrics AI labels: the next line says which earlier line it rhymes with, or nothing");

    const auto unknownWord = ai::parseReply (reqLine3, ctx3, R"({"alternatives":[{"lines":["your zzyzx hangs beside the door"],"note":""}]})");
    check (unknownWord.alternatives.size() == 1 && unknownWord.alternatives[0].guessedLabel == "guessed from the spelling",
           "lyrics AI labels: a word the dictionary lacks is marked as guessed");

    // Apply.
    const auto applied = ai::applyAlternative (pop, ctx3, labelled.alternatives[0]);
    check (applied.has_value() && applied->text.substring (0, ctx3.replaceStart) == pop.substring (0, ctx3.replaceStart)
               && applied->text.substring (applied->end) == pop.substring (ctx3.replaceEnd) && applied->text.substring (applied->start, applied->end) == labelled.alternatives[0].text
               && applied->text.contains ("the way it used to find me"),
           "lyrics AI apply: only the range changes, the text before and after is byte-identical");
    check (! ai::applyAlternative (pop.replace ("voice", "VOICE"), ctx3, labelled.alternatives[0]).has_value(),
           "lyrics AI apply: a range whose text changed is refused");
    const auto inserted = ai::applyAlternative (pop, nextCtx, nextReply.alternatives[0]);
    check (inserted.has_value() && inserted->text.contains ("your voice still ring-ing in my head\nthe rain keeps time with me\nthe way it used")
               && ! ai::applyAlternative ("x" + pop, nextCtx, nextReply.alternatives[0]).has_value(),
           "lyrics AI apply: a next line goes after the line, and not when the text before moved");

    const auto addRequest = [&] { auto r = makeRequest (ai::Kind::sectionAdd, pop, 5, 5); r.addHeading = "Bridge"; r.addBars = 8; return r; }();
    const auto addCtx = ai::buildContext (addRequest);
    const auto added = ai::parseReply (addRequest, addCtx, ai::getCannedReply (addRequest, addCtx));
    const auto addApplied = ai::applyAlternative (pop, addCtx, added.alternatives[0]);
    check (addCtx.linesWanted == 4 && added.alternatives.size() == 2 && added.alternatives[0].lines.size() == 4 && added.alternatives[0].summary.startsWith ("4 lines, ")
               && addApplied.has_value() && addApplied->text.contains ("the way it used to find me\n\n[Bridge]\nyour coat"),
           "lyrics AI: a new section goes under its heading at the end of the verse, with its lines and syllables summarised");

    // The whole song.
    const auto songContext = ai::buildContext (songRequest);
    const auto song = ai::parseReply (songRequest, songContext, ai::getCannedReply (songRequest, songContext));
    check (song.error.isEmpty() && song.alternatives.size() == 1 && song.alternatives[0].sectionSummaries.size() == 3
               && song.alternatives[0].sectionSummaries[0].startsWith ("Verse 1: 4 lines, ") && song.alternatives[0].sectionSummaries[1].startsWith ("Chorus: 4 lines, ")
               && song.alternatives[0].sectionSummaries[2].startsWith ("Verse 2: 8 lines, "),
           "lyrics AI: the whole song has a summary line per section");
    const auto placed = place (daw::lyrics::parse (song.alternatives[0].text), sections);
    int inFirstChorus = 0, inSecondChorus = 0;
    for (auto& l : placed.lines)
    {
        inFirstChorus += l.startBeat >= 32.0 && l.startBeat < 64.0 ? 1 : 0;
        inSecondChorus += l.startBeat >= 128.0 && l.startBeat < 160.0 ? 1 : 0;
    }
    check (inFirstChorus == 4 && inSecondChorus == 4 && placed.lines.size() == 4 + 4 + 8 + 4,
           "lyrics AI: the whole song's text puts the chorus on every chorus section (" + juce::String (inFirstChorus) + ", " + juce::String (inSecondChorus) + ")");
    check (ai::parseReply (songRequest, songContext, R"({"sections":[{"heading":"Verse","lines":["a","b"]}]})").error.isNotEmpty(),
           "lyrics AI: a song with too few sections is an error");

    auto noSections = makeRequest (ai::Kind::song, "", 0, 0);
    noSections.sections = {};
    const auto defaultSong = ai::parseReply (noSections, ai::buildContext (noSections), ai::getCannedReply (noSections, ai::buildContext (noSections)));
    check (defaultSong.error.isEmpty() && defaultSong.note.startsWith ("No sections yet") && defaultSong.alternatives[0].text.contains ("[Verse]")
               && defaultSong.alternatives[0].text.indexOf ("[Chorus]") < defaultSong.alternatives[0].text.lastIndexOf ("[Chorus]")
               && daw::lyrics::parse (defaultSong.alternatives[0].text).lines.size() == 6 * 4,
           "lyrics AI: without sections the song is the default form, chorus three times, with the note");

    // Rhymes.
    const juce::String quiet = "the quiet night";
    const auto rhymeRequest = makeRequest (ai::Kind::rhymes, quiet, 13, 13);
    const auto rhymeContext = ai::buildContext (rhymeRequest);
    const auto picks = ai::parseReply (rhymeRequest, rhymeContext, ai::getCannedReply (rhymeRequest, rhymeContext));
    auto kindOf = [&] (const char* word)
    {
        for (auto& p : picks.rhymes)
            if (p.word == word)
                return p.kind;
        return rh::Kind::unknown;
    };
    check (rhymeContext.word == "night" && rhymeContext.target == "night" && picks.rhymes.size() == 7 && kindOf ("light") == rh::Kind::perfect
               && kindOf ("tonight") == rh::Kind::identical && kindOf ("ride") == rh::Kind::family && kindOf ("time") == rh::Kind::assonance && ! picks.rhymes[0].guessed,
           "lyrics AI rhymes: the picks are labelled by their sound");
    check (ai::buildUserPrompt (rhymeRequest, rhymeContext).contains ("from a pronouncing dictionary") && ai::applyWord (quiet, rhymeContext, "light").value_or (ai::Applied()).text == "the quiet light",
           "lyrics AI rhymes: the prompt lists the dictionary's rhymes and a pick replaces the word");
}

static void runReadingGuideTests()
{
    using namespace daw;
    namespace rm = gen::readmusic;
    namespace tg = gen::guides;
    std::cout << "Reading guide" << std::endl;

    auto partOf = [] (const juce::String& name, std::vector<MidiNote> notes, MusicalKey key = {})
    {
        rm::Part part;
        part.hasTrack = true;
        part.trackName = name;
        part.input.notes = std::move (notes);
        part.input.key = key;
        part.input.clef = gen::notation::ClefChoice::automatic;
        return part;
    };

    rm::Part none;
    none.input.key = { 9, MusicalKey::Scale::minor };
    const auto yours = partOf ("Keys", { { 69, 0, 1, 0.8f }, { 72, 1, 1, 0.8f }, { 76, 2, 1, 0.8f }, { 72, 3, 1, 0.8f } }, { 9, MusicalKey::Scale::minor });

    // Every topic has a name, a summary, a text and (with a part) a picture.
    bool complete = true;

    for (auto t : rm::allTopics)
    {
        const auto& part = t == rm::Topic::yours ? yours : none;
        complete = complete && rm::getTopicName (t).isNotEmpty() && rm::getTopicSummary (t).isNotEmpty()
                   && rm::explain (t, part).startsWith (rm::getTopicName (t)) && rm::explain (t, part).length() > 60
                   && ! rm::getExample (t, part).panels.empty();

        for (auto& panel : rm::getExample (t, part).panels)
            complete = complete && ! panel.score.staves.empty() && panel.score.numBars >= 1;
    }

    check (std::size (rm::allTopics) == 15 && complete, "reading guide: 15 topics, each with a summary, a text and a picture");

    // The song's key and tempo come into the texts.
    check (rm::explain (rm::Topic::keySignatures, none).contains ("A minor: no sharps or flats")
               && rm::explain (rm::Topic::timeAndTempo, none).contains ("= 120 at the top means 120 quarter notes"),
           "reading guide: the key signature and tempo texts use the song's key and tempo");

    // The example pictures hold what the plan says.
    {
        const auto staff = rm::getExample (rm::Topic::staff, none).panels[0];
        const auto grand = rm::getExample (rm::Topic::grandStaff, none).panels[0];
        const auto how = rm::getExample (rm::Topic::howDawItWrites, none);
        const auto generated = rm::getExample (rm::Topic::generatedMusic, none);
        juce::String values;

        for (auto& e : how.panels[0].score.staves[0].bars[0].events)
            if (! e.isRest)
                values << e.length << (e.tiedToNext ? "~ " : " ");

        check (staff.score.numBars == 12 && staff.showNoteNames && grand.score.isGrand() && grand.keyboardNotes.size() == 6
                   && rm::getExample (rm::Topic::trebleNotes, none).panels[0].keyboardNotes.size() == 9
                   && how.panels[0].played.size() == 5,
               "reading guide: the staff picture has 12 notes, the grand staff 6 lit keys, the treble notes 9");
        check (values.startsWith ("6 6 12 6 6~ ") && how.panels[0].score.staves[0].bars.size() >= 1,
               "reading guide: the 'How DawIt writes your MIDI' example is written eighth, eighth, quarter, eighth, eighth tied to a quarter (" + values + ")");
        check (! rm::getExample (rm::Topic::noteValues, none).playNotes.empty() && rm::getExample (rm::Topic::staff, none).playNotes.empty(),
               "reading guide: the note values, ties and rounding topics have a rhythm to play, the staff picture hasn't");
        const auto& n = generated.playNotes;
        check (n.size() == 4 && n[0].pitch == 60 && n[1].pitch == 62 && n[2].pitch == 64 && n[3].pitch == 67
               && juce::exactlyEqual (n[0].start, 0.0) && juce::exactlyEqual (n[3].start, 3.0)
               && juce::exactlyEqual (n[3].length, 1.0) && ! generated.panels[0].score.chordSymbols.empty()
               && generated.panels[0].score.chordSymbols[0].name == "C",
               "reading guide: generated music example has C D E G on the beats over a C label");
    }

    // Your part.
    {
        // A bass part from E1 to G2 on the grid.
        std::vector<MidiNote> bassNotes;
        const int pitches[] = { 28, 31, 33, 35, 36, 38, 40, 41, 43, 33 };

        for (int i = 0; i < 10; ++i)
            bassNotes.push_back ({ pitches[i], (double) i, 1.0, 0.8f });

        const auto bass = rm::analysePart (partOf ("Bass", bassNotes));
        check (bass.numNotes == 10 && bass.clef == gen::notation::ClefChoice::bass && bass.moved == 0 && bass.outsideKey == 0
                   && bass.text.contains ("Bass has 10 notes in bars 1\xe2\x80\x93" "3.")
                   && bass.advice.size() == 1 && bass.advice[0].contains ("Clef > Bass guitar")
                   && bass.text.contains ("Every note is in the key, so the score needs no accidentals.")
                   && bass.text.contains ("Every note starts on the grid"),
               "reading guide: a Bass part E1 to G2 gets the bass guitar advice, no accidentals, no rounding");
        check (bass.text.contains ("its highest note, G2, is G4 or lower") && bass.text.contains ("The lowest note, E1, needs 4 ledger lines."),
               "reading guide: Your part gives the clef's reason and the ledger lines (" + bass.text + ")");

        // Played freely: 4 of 10 notes start 0.12 beats late.
        auto loose = bassNotes;

        for (int i : { 1, 3, 5, 7 })
        {
            loose[(size_t) i].start += 0.12;
            loose[(size_t) i].length -= 0.12;
        }

        const auto freely = rm::analysePart (partOf ("Pad", loose));
        check (freely.moved == 4 && freely.advice.size() == 1 && freely.advice[0].startsWith ("This part was played freely")
                   && freely.maxMoved > 0.11 && freely.maxMoved < 0.13,
               "reading guide: 4 of 10 notes moved by more than 0.0625 beats gives the freely-played advice");

        // A minor with the raised seventh.
        const auto minor = rm::analysePart (partOf ("Lead", { { 69, 0, 1, 0.8f }, { 71, 1, 1, 0.8f }, { 72, 2, 1, 0.8f }, { 68, 3, 1, 0.8f } },
                                                    { 9, MusicalKey::Scale::minor }));
        check (minor.outsideKey == 1 && minor.raisedSeventh == 1 && minor.text.contains (juce::String::fromUTF8 ("G\xe2\x99\xaf, the raised seventh")),
               "reading guide: a G sharp in A minor is the raised seventh");

        check (rm::analysePart (none).text.startsWith ("Select a MIDI track")
                   && rm::analysePart (partOf ("Drums", {})).text == "Drums has no notes yet. Draw some in the piano roll and they'll appear here.",
               "reading guide: Your part has texts for no track and for no notes");

        rm::Part guitar = partOf ("Rhythm gtr", { { 45, 0, 1, 0.8f }, { 52, 1, 1, 0.8f }, { 64, 2, 1, 0.8f }, { 79, 3, 1, 0.8f } });
        check (rm::analysePart (guitar).advice.size() == 1 && rm::analysePart (guitar).advice[0].contains ("Clef > Guitar"),
               "reading guide: a wide guitar part on a grand staff gets the guitar advice");

        auto chosen = partOf ("Bass", bassNotes);
        chosen.autoClef = false;
        chosen.input.clef = gen::notation::ClefChoice::bassGuitar;
        check (rm::analysePart (chosen).text.contains ("you chose it in the Clef menu") && rm::analysePart (chosen).advice.isEmpty(),
               "reading guide: a chosen clef is explained as chosen and gets no clef advice");
    }

    // The entry in the Theory Guides window.
    {
        const auto entries = tg::getEntries();
        const auto entry = tg::getEntry (tg::Guide::readMusic);
        const auto path = tg::getStartPath();
        bool found = true;

        for (const auto* query : { "clef", "key signature", "sheet music", "ledger line", "treble clef", "do I need to read music", "abc" })
        {
            const auto results = tg::search (entries, query);
            found = found && std::any_of (results.begin(), results.end(), [&] (const auto& r) { return entries[(size_t) r.entry].guide == tg::Guide::readMusic; });
        }

        check (entry.area == tg::Area::readingMusic && tg::getAreaName (tg::Area::readingMusic) == "Reading music" && entry.name == "How to read music"
                   && entry.topics.size() == 15 && entry.plugin.isEmpty() && found
                   && std::find (path.begin(), path.end(), tg::Guide::readMusic) == path.end(),
               "reading guide: its entry has 15 topics, is searchable, and is not on Start here");
    }
}

static void runLyricsGuideTests()
{
    using namespace daw;
    namespace guide = gen::lyricsguide;
    std::cout << "Lyrics guide" << std::endl;

    check (guide::rhymes ("night", "light") && guide::rhymes ("tonight", "bite") && guide::rhymes ("go", "know") && guide::rhymes ("me", "see")
               && guide::rhymes ("late", "gate") && guide::rhymes ("head,", "ahead") && ! guide::rhymes ("late", "head") && ! guide::rhymes ("ends", "again"),
           "words that rhyme are found by their spelling: night/light, go/know, me/see, late/gate (" + guide::getRhymeKey ("late") + ")");
    check (guide::findRhymeScheme ({ "late", "me", "head", "me" }) == "XAXA" && guide::findRhymeScheme ({ "tonight", "light", "go", "know" }) == "AABB"
               && guide::findRhymeScheme ({ "night", "go", "light", "slow" }) == "ABAB" && guide::findRhymeScheme ({ "one", "two" }) == "XX",
           "and the rhyme scheme of a verse is named the usual way: XAXA, AABB, ABAB");

    const auto song = lyrics::place (lyrics::parse ("[Verse]\nI walked a-long the ri-ver late\nthe ci-ty lights be-hind me\nyour voice still ring-ing in my head\n"
                                                    "the way it used to find me\n\n[Chorus]\nhold on, hold on to-night\nwe could run be-fore the light\n\n"
                                                    "[Chorus]\nhold on, hold on to-night\nwe could fly be-fore the light"), {});
    const auto verses = guide::analyse (song);
    check (verses.size() == 3 && verses[0].scheme == "XAXA" && verses[1].scheme == "AA" && verses[0].lines[0].syllables == 8
               && verses[2].heading == "Chorus 2" && verses[0].lines[1].rhyme == "A",
           "the song's own lyrics: each verse with its rhyme scheme and syllables a line (" + verses[0].scheme + ", "
               + juce::String (verses[0].lines[0].syllables) + ")");

    check (guide::rhymes ("night", "knight") && guide::rhymes ("day", "day") && ! guide::rhymes ("night", "ride") && guide::rhymes ("zzyzx", "zzyzx"),
           "rhymes by sound: identical words count as repetition, a family rhyme doesn't count in the scheme, unknown words fall back to spelling");

    {
        // Plan: 15 topics, the new ones with their text and pictures.
        const auto find = guide::explain (guide::Topic::findRhyme, {}, {}, 120.0);
        const auto ai = guide::explain (guide::Topic::writeWithAi, {}, {}, 120.0);
        const auto rows = guide::getExample (guide::Topic::findRhyme, {});
        const auto original = guide::getExample (guide::Topic::writeWithAi, {});
        const auto entries = gen::guides::getEntries();
        const auto lyricsEntry = gen::guides::getEntry (gen::guides::Guide::lyrics);
        check (std::size (guide::allTopics) == 15 && guide::getTopicName (guide::Topic::findRhyme) == "Finding a rhyme"
                   && guide::getTopicName (guide::Topic::writeWithAi) == "Writing with AI"
                   && lyricsEntry.topics[3] == "Finding a rhyme" && lyricsEntry.topics[9] == "Writing with AI"
                   && lyricsEntry.opensFrom.contains ("Write with AI"),
               "lyrics guide: 15 topics, Finding a rhyme after Rhyme and Writing with AI after Writing tips");
        check (find.contains ("Pat Pattison") && find.contains ("126,000") && find.contains ("Spaghetti") && rows.kind == guide::Kind::words && rows.lines.size() == 7
                   && rows.lines[0].label == "Perfect" && rows.lines[0].tokens.size() == 3 && rows.lines[5].tokens.size() == 2
                   && guide::explain (guide::Topic::rhyme, {}, {}, 120.0).contains ("last stressed vowel"),
               "lyrics guide: Finding a rhyme has its text and a row per kind of rhyme, and Rhyme says the last stressed vowel");
        const auto struck = std::count_if (original.lines[2].tokens.begin(), original.lines[2].tokens.end(), [] (auto& t) { return t.wrong; });
        check (ai.contains ("Ollama") && ai.contains ("PhonologyBench") && ai.contains ("Copyright Office") && ai.contains ("Munich")
                   && ai.contains ("isn't legal advice") && original.lines.size() == 8 && struck == 7 && original.lines[5].note.contains ("8 syllables, like the original"),
               "lyrics guide: Writing with AI mentions Ollama, PhonologyBench, the Copyright Office and Munich, and shows the struck original with three alternatives");

        bool found = true;
        for (auto* q : { "assonance", "ollama", "rhyming dictionary" })
        {
            const auto results = gen::guides::search (entries, q);
            found = found && std::any_of (results.begin(), results.end(), [&] (auto& r) { return entries[(size_t) r.entry].guide == gen::guides::Guide::lyrics; });
        }
        check (found, "theory guides: assonance, ollama and rhyming dictionary find the lyrics guide");
    }

    const SectionList sections { { 0, 8, SectionType::verse, {} }, { 40, 8, SectionType::bridge, {} } };
    const auto yours = guide::explain (guide::Topic::yours, song, sections, 120.0);
    check (yours.contains ("rhyme scheme XAXA") && yours.contains ("Not every chorus has the same words") && yours.contains ("The Bridge at bar 41 has no words yet"),
           "and what to look at: choruses that differ, a sung section without words");

    Lyrics crowded;
    crowded.lines.push_back ({ 0.0, 2.0, true, {}, {} });
    for (auto* w : { "so", "ma", "ny", "words", "in", "so", "lit", "tle", "time" })
        crowded.lines[0].syllables.push_back ({ w, true });
    lyrics::tidy (crowded);
    check (guide::explain (guide::Topic::yours, crowded, {}, 120.0).contains ("fast to sing")
               && guide::explain (guide::Topic::yours, {}, {}, 120.0).contains ("no lyrics yet"),
           "a line with too many syllables for its beats is pointed out, and an empty song says how to start");

    bool all = true;
    for (auto topic : guide::allTopics)
        all = all && guide::getTopicName (topic).isNotEmpty() && guide::getTopicSummary (topic).isNotEmpty()
              && guide::explain (topic, song, sections, 120.0).length() > 200 && (topic == guide::Topic::yours || ! guide::getExample (topic, song).lines.empty());
    check (all, "every topic has a name, a summary, an explanation and a picture");

    const auto stress = guide::getExample (guide::Topic::stress, song);
    check (stress.lines.size() == 3 && ! std::any_of (stress.lines[0].tokens.begin(), stress.lines[0].tokens.end(), [] (auto& t) { return t.wrong; })
               && std::any_of (stress.lines[1].tokens.begin(), stress.lines[1].tokens.end(), [] (auto& t) { return t.wrong; })
               && ! std::any_of (stress.lines[2].tokens.begin(), stress.lines[2].tokens.end(), [] (auto& t) { return t.wrong; }),
           "the stress picture marks only the wrong line's syllables as on the wrong beats");
}

static void runSingAlongTests()
{
    namespace sa = daw::gen::singalong;
    using daw::LyricLine;

    std::cout << "\n-- Sing along with the lyrics --\n";

    auto makeLine = [] (double start, double end, std::initializer_list<std::tuple<const char*, double, bool>> syllables)
    {
        LyricLine line;
        line.startBeat = start;
        line.endBeat = end;

        for (auto& [text, beat, endsWord] : syllables)
        {
            daw::LyricSyllable s;
            s.text = text;
            s.beat = beat;
            s.endsWord = endsWord;
            line.syllables.push_back (s);
        }

        return line;
    };

    // Two lines close together, then a rest of 8 beats before the third.
    daw::Lyrics song;
    song.lines = { makeLine (0.0, 4.0, { { "Walk", 0.0, false }, { "ing", 1.0, true }, { "home", 2.0, true } }),
                   makeLine (4.5, 8.0, { { "in", 4.5, true }, { "the", 5.0, true }, { "rain", 6.0, true } }),
                   makeLine (16.0, 22.0, { { "hold", 16.0, true }, { "on", 17.0, true } }) };

    auto m = sa::findMoment (song, 1.5);
    check (m.line == 0 && m.singing && m.syllable == 1 && m.previous == -1 && m.next == 1, "while a line is sung it's the one shown, with the syllable sung now");

    m = sa::findMoment (song, 4.2);
    check (m.line == 1 && ! m.singing && std::abs (m.beatsToLine - 0.3) < 1.0e-9 && m.previous == 0 && m.next == 2 && sa::getCountIn (song, m) == 0,
           "between lines the next one is shown, and a line right after another gets no count-in");

    m = sa::findMoment (song, 13.0);
    check (m.line == 2 && std::abs (m.beatsToLine - 3.0) < 1.0e-9 && sa::getCountIn (song, m) == 3
               && sa::getCountIn (song, sa::findMoment (song, 12.0)) == 4 && sa::getCountIn (song, sa::findMoment (song, 15.5)) == 1,
           "after a rest the last 4 beats are counted in: 4, 3, 2, 1");
    check (sa::getCountIn (song, sa::findMoment (song, 10.0)) == 0 && sa::getCountIn (song, sa::findMoment (song, 16.5)) == 0,
           "no count-in further away or while singing");
    check (sa::getCountIn (song, sa::findMoment (song, -2.0)) == 2, "the first line is counted in too");

    m = sa::findMoment (song, 25.0);
    check (m.line == -1 && m.previous == 2 && m.next == -1, "after the last line nothing comes, and the last one was sung");

    m = sa::findMoment (song, 9.0, { true, 0.0, 12.0 });
    check (m.line == 0 && std::abs (m.beatsToLine - 3.0) < 1.0e-9 && m.next == 1 && m.previous == 1,
           "in a loop the first line comes round again after the loop's last");

    const auto& held = song.lines[2];
    check (std::abs (sa::getSung (song.lines[0], 0, 0.5) - 0.5f) < 1.0e-6f && sa::getSung (song.lines[0], 1, 0.5) == 0.0f
               && sa::getSung (song.lines[0], 0, 3.0) == 1.0f && std::abs (sa::getSung (held, 1, 18.0) - 0.5f) < 1.0e-6f
               && sa::getSung (held, 1, 19.5) == 1.0f,
           "the colour fills a syllable until the next one, and one before a long end in at most 2 beats");

    check (sa::getText (song.lines[0]) == "Walking home" && sa::getText (song.lines[1]) == "in the rain", "a line's syllables are joined into words");

    const auto breath = sa::findShortestBreath (song);
    const auto longest = sa::findLongestLine (song);
    check (breath.line == 1 && std::abs (breath.beats - 0.5) < 1.0e-9 && longest.first == 2 && std::abs (longest.second - 6.0) < 1.0e-9,
           "the shortest rest and the longest line are found");

    const auto tips = sa::explainSingingAlong (song, 120.0);
    check (tips.contains ("\"hold on\": 6 beats, about 3.0 seconds") && tips.contains ("before \"in the rain\"") && tips.contains ("Breathe")
               && sa::explainSingingAlong ({}, 120.0).contains ("no lyrics yet"),
           "the tips look at the song's longest line and shortest rest, and an empty song says how to start");
}

static void runPlayAlongTests()
{
    namespace pa = daw::gen::playalong;
    using daw::Chord;

    std::cout << "\n-- Play along with the chords --\n";

    const auto chord = [] (const char* name) { return *Chord::fromName (name); };

    // Am Am F C, an empty bar, then Am again.
    daw::ChordMap chords { { 0, chord ("Am") }, { 1, chord ("Am") }, { 2, chord ("F") }, { 3, chord ("C") }, { 5, chord ("Am") } };
    const auto changes = pa::getChanges (chords);
    check (changes.size() == 4, "a chord repeated on the next bar is one change; after an empty bar it's a new one");
    check (changes[0].startBar == 0 && changes[0].endBar == 2 && changes[0].getBars() == 2 && changes[3].startBar == 5,
           "each change knows its bars");
    check (pa::getChanges ({}).empty(), "no chords, no changes");

    daw::Section chorus;
    chorus.startBar = 0;
    chorus.bars = 4;
    chorus.type = daw::SectionType::chorus;
    chorus.name = "Chorus 2";
    const daw::SectionList sections { chorus };

    // Held over two bars: the countdown runs over both.
    auto m = pa::findMoment (changes, sections, 1.0);
    check (m.current == 0 && m.bar == 0 && m.beatInBar == 1, "beat 2 of bar 1 plays the first Am");
    check (std::abs (m.beatsLeft - 7.0) < 1.0e-9 && std::abs (m.runBeats - 8.0) < 1.0e-9, "the Am held over two bars changes in 7 beats, of 8");
    check (m.next == 1 && std::abs (m.beatsToNext - 7.0) < 1.0e-9, "the next chord is F, in 7 beats");
    check (! pa::isAboutToChange (m), "the next chord isn't lit 7 beats ahead");
    check (m.coming.size() == 3 && m.coming[1].change == 2 && std::abs (m.coming[2].inBeats - 19.0) < 1.0e-9,
           "the chords coming: F, C, then Am after the empty bar");
    check (pa::describePosition (m, sections) == juce::String (juce::CharPointer_UTF8 ("Chorus 2 \xc2\xb7 bar 1 of 4")), "the section and the bar in it");

    check (pa::isAboutToChange (pa::findMoment (changes, sections, 7.0)), "the next chord lights up on the last beat before it");
    check (! pa::isAboutToChange (pa::findMoment (changes, sections, 6.5)), "and not before that beat");

    // Before an empty bar, the next chord is the one after it.
    m = pa::findMoment (changes, sections, 12.5);
    check (m.current == 2 && std::abs (m.beatsLeft - 3.5) < 1.0e-9 && m.next == 3 && std::abs (m.beatsToNext - 7.5) < 1.0e-9,
           "on C before an empty bar, the C ends in 3.5 beats and the Am comes in 7.5");

    m = pa::findMoment (changes, sections, 17.0);
    check (m.current == -1 && std::abs (m.beatsLeft - 3.0) < 1.0e-9 && std::abs (m.runBeats - 4.0) < 1.0e-9 && m.next == 3,
           "in the empty bar: no chord, Am in 3 beats, the bar counting down");
    check (pa::describePosition (m, sections) == "Bar 5", "outside the sections, just the bar");

    // The song's end.
    m = pa::findMoment (changes, sections, 22.0);
    check (m.current == 3 && std::abs (m.beatsLeft - 2.0) < 1.0e-9 && m.next == -1 && m.coming.empty(), "on the last chord nothing comes after it");
    m = pa::findMoment (changes, sections, 30.0);
    check (m.current == -1 && m.beatsLeft < 0.0 && m.next == -1, "after the last chord nothing changes");
    m = pa::findMoment ({}, {}, 4.0);
    check (m.current == -1 && m.next == -1 && m.bar == 1, "without chords, only the bar");

    // Round the loop (bars 1 to 3): after F comes C, then the Am at the loop's start again.
    const pa::Loop loop { true, 0.0, 12.0 };
    m = pa::findMoment (changes, sections, 10.0, loop);
    check (m.current == 1 && std::abs (m.beatsLeft - 2.0) < 1.0e-9 && m.next == 0 && std::abs (m.beatsToNext - 2.0) < 1.0e-9,
           "at the loop's end the next chord is the one at its start");
    check (m.coming.size() >= 2 && m.coming[1].change == 1 && std::abs (m.coming[1].inBeats - 10.0) < 1.0e-9, "and the loop's chords come round again");
    m = pa::findMoment (changes, sections, 2.0, { true, 0.0, 8.0 });
    check (m.current == 0 && m.beatsLeft < 0.0 && m.next == -1, "a loop inside one chord never changes");
    m = pa::findMoment (changes, sections, 14.0, loop);
    check (m.current == 2 && m.next == 3, "after the loop's end it plays on as usual");

    // Words.
    check (pa::describeWait (0.0) == "now" && pa::describeWait (0.4) == "in 1 beat" && pa::describeWait (1.0) == "in 1 beat"
           && pa::describeWait (3.0) == "in 3 beats" && pa::describeWait (7.5) == "in 8 beats", "waits under two bars are in beats, rounded up");
    check (pa::describeWait (8.0) == "in 2 bars" && pa::describeWait (13.0) == "in 4 bars", "longer waits are in bars, rounded up");

    // A capo.
    check (pa::getShapeChord (chord ("Gm"), 3) == chord ("Em") && pa::getShapeChord (chord ("C"), 0) == chord ("C"),
           "with a capo on fret 3, Gm is played as an Em shape");
    check (pa::getShapeChord (chord ("C"), 5) == chord ("G"), "the shape wraps round the octave (C with capo 5 is a G shape)");
    check (pa::describeCapo (chord ("Gm"), 3).contains ("Em shape") && pa::describeCapo (chord ("Gm"), 3).contains ("Gm")
           && pa::describeCapo (chord ("Gm"), 0).isEmpty(), "the capo line names the shape and the sound");

    const auto inAFlat = pa::getChanges ({ { 0, chord ("Ab") }, { 1, chord ("Db") }, { 2, chord ("Eb") }, { 3, chord ("Fm") } });
    check (pa::findEasiestCapo (inAFlat) == 1, "Ab Db Eb Fm: capo 1 makes them G C D Em, all open chords");
    check (pa::findEasiestCapo (pa::getChanges ({ { 0, chord ("G") }, { 1, chord ("C") }, { 2, chord ("D") }, { 3, chord ("Em") } })) == 0,
           "G C D Em are open chords already: no capo");

    // The guide's text.
    check (pa::explainPlayingAlong ({}, 120.0).contains ("no chords yet"), "the tips say how to get chords when there are none");
    const auto text = pa::explainPlayingAlong (changes, 120.0);
    check (text.contains ("3 different chords") && text.contains ("Look at the next chord") && text.contains ("anchor fingers"),
           "the tips, and how many chords the song has");
    check (text.contains ("quickest change comes after 1 bar of F in bar 3") && text.contains ("2.0 seconds at 120 BPM"),
           "the quickest change, in seconds at the song's tempo");
    check (pa::explainPlayingAlong (inAFlat, 100.0).contains ("capo on fret 1"), "a capo is suggested when it makes the chords easier");

    // What's heard: the position itself while stopped.
    daw::Engine engine;
    engine.setPositionBeats (6.0);
    check (std::abs (engine.getHeardPositionBeats() - engine.getPositionBeats()) < 1.0e-9, "while stopped, what's heard is the playhead");
}

static void runMidiGuideTests()
{
    namespace mg = daw::midiguide;
    std::cout << "What is MIDI?" << std::endl;

    const auto near = [] (double a, double b, double tolerance) { return std::abs (a - b) < tolerance; };

    // The numbers.
    check (near (mg::getCableSeconds (3), 0.00096, 1.0e-9), "a three-byte message takes 0.96 ms at 31,250 bits a second, 10 bits a byte");
    check (near (mg::getFrequency (69), 440.0, 1.0e-9) && near (mg::getFrequency (57), 220.0, 1.0e-9) && near (mg::getFrequency (60), 261.63, 0.01),
           "note 69 is 440 Hz, an octave down is 220, middle C is 261.63 Hz");
    check (mg::getNoteName (60) == "C4" && mg::getNoteName (69) == "A4" && mg::getNoteName (mg::pianoLowest) == "A0" && mg::getNoteName (mg::pianoHighest) == "C8",
           "note 60 is C4, 69 is A4, and a piano goes from A0 (21) to C8 (108)");
    check (mg::pitchBendCentre == 8192 && mg::clockPulsesPerQuarter == 24, "pitch bend's middle is 8192, and clock is 24 pulses a quarter note");

    const auto& controllers = mg::getCommonControllers();
    const auto controllerName = [&controllers] (int number)
    {
        const auto it = std::find_if (controllers.begin(), controllers.end(), [number] (const mg::Controller& c) { return c.number == number; });
        return it != controllers.end() ? it->name : juce::String();
    };
    check (controllerName (1) == "Modulation wheel" && controllerName (7) == "Volume" && controllerName (10) == "Pan" && controllerName (64) == "Sustain pedal",
           "controller 1 is the mod wheel, 7 volume, 10 pan, 64 the sustain pedal");

    // Messages.
    const auto noteOn = juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100);
    check (mg::toHex (noteOn) == "90 3C 64" && mg::toBinary (0x90) == "1001 0000" && mg::toHex (juce::MidiMessage::noteOff (1, 60)) == "80 3C 00",
           "a Note On for middle C at velocity 100 is 90 3C 64, its status byte 1001 0000");
    check (mg::describe (noteOn) == "Note On, channel 1, C4 (note 60), velocity 100"
               && mg::describe (juce::MidiMessage::noteOn (2, 69, (juce::uint8) 0)).startsWith ("Note Off, channel 2, A4 (note 69), as a Note On")
               && mg::describe (juce::MidiMessage::controllerEvent (1, 64, 127)).contains ("sustain pedal")
               && mg::describe (juce::MidiMessage::pitchWheel (1, 8192)).contains ("no bend"),
           "messages in words: " + mg::describe (noteOn));
    check (mg::toMidiVelocity (0.8f) == 102 && mg::toMidiVelocity (1.0f) == 127 && mg::toMidiVelocity (2.0f) == 127, "velocity 0.8 is sent as 102");

    daw::MidiClip clip;
    clip.name = "Keys";
    clip.lengthBeats = 4.0;
    clip.notes = { { 60, 0.0, 1.0, 0.8f }, { 64, 0.0, 1.0, 0.8f }, { 67, 0.0, 1.0, 0.8f },   // a chord
                   { 60, 1.0, 1.0, 0.8f },                                                   // C again right after it ends
                   { 72, 3.0, 4.0, 0.8f },                                                   // rings past the end
                   { 74, 5.0, 1.0, 0.8f } };                                                 // after the end: not played
    const auto events = mg::getEvents (clip);
    juce::StringArray order;
    for (auto& e : events)
        order.add (mg::toHex (e.message) + " @" + juce::String (e.beat));

    check (events.size() == 10 && order[3] == "80 3C 00 @1" && order[6] == "90 3C 66 @1" && order[9] == "80 48 00 @4",
           "the clip as messages: offs before a re-struck note, a ringing note ends with the clip, a late one left out: " + order.joinIntoString (", "));
    check (mg::countBytes (events, false) == 30 && mg::countBytes (events, true) == 26,
           "10 messages are 30 bytes, 26 with running status (4 status bytes repeat)");

    const auto stats = mg::getVelocityStats (clip);
    check (stats.count == 5 && stats.lowest == 102 && stats.highest == 102 && stats.distinct == 1, "the velocities of the notes that play");

    // The MIDI file.
    const auto file = mg::getClipFile (clip, 120.0, { 9, daw::MusicalKey::Scale::minor }, "Keys");
    const auto parts = mg::describeFile (file);
    const auto valueOf = [&parts] (const juce::String& name)
    {
        for (auto& p : parts)
            if (p.name == name)
                return p.value;

        return juce::String ("(missing)");
    };

    check (! parts.empty() && parts.front().name == "MThd" && valueOf ("Format").startsWith ("1") && valueOf ("Tracks") == "2"
               && valueOf ("Division") == "960 ticks a quarter note",
           "the file's header: type 1, two tracks, 960 ticks a quarter note");
    check (valueOf ("Tempo") == "500000 microseconds a quarter note" && valueOf ("Time signature") == "4/4"
               && valueOf ("Key signature") == "0 sharps, minor" && valueOf ("Note On").startsWith ("channel 1, C4"),
           "its events: tempo " + valueOf ("Tempo") + ", " + valueOf ("Time signature") + ", " + valueOf ("Key signature") + ", " + valueOf ("Note On"));

    // The texts.
    bool allText = true;
    for (auto topic : mg::allTopics)
        allText = allText && mg::getTopicName (topic).isNotEmpty() && mg::getTopicSummary (topic).isNotEmpty()
                  && mg::explain (topic, &clip, 120.0).length() > 200;
    check (allText, "every topic has a name, a summary and a text");
    check (mg::explain (mg::Topic::history, nullptr, 120.0).contains ("1983") && mg::explain (mg::Topic::cable, nullptr, 120.0).contains ("31,250")
               && mg::explain (mg::Topic::time, nullptr, 120.0).contains ("500000 microseconds") && mg::explain (mg::Topic::time, nullptr, 120.0).contains ("0.52 ms"),
           "the facts: 1983, 31,250 bits a second, and at 120 BPM a quarter note is 500000 microseconds and a tick 0.52 ms");

    const auto yours = mg::explain (mg::Topic::yourClip, &clip, 120.0);
    check (yours.contains ("5 notes, from C4 (note 60") && yours.contains ("every note is at 102") && yours.contains ("10 messages")
               && yours.contains ("most notes starting together is 3, at 1.1.1"),
           "the look at the clip: its range, the same velocity everywhere, its messages and its chord");
    check (mg::explain (mg::Topic::yourClip, nullptr, 120.0).contains ("Select a MIDI clip"), "without a clip it says how to get one");
    check (std::size (mg::allTopics) == 11 && mg::allTopics[6] == mg::Topic::computerKeyboard && mg::getTopicName (mg::Topic::computerKeyboard) == "The computer keyboard"
               && mg::getTopicSummary (mg::Topic::computerKeyboard).isNotEmpty() && mg::explain (mg::Topic::computerKeyboard, nullptr, 120.0).contains ("No MIDI keyboard at hand?"),
           "the guide has 11 topics, with The computer keyboard after Velocity");
}

static void runAuditionTests()
{
    std::cout << "Audition player" << std::endl;

    daw::Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    FakeDevice device;
    engine.audioDeviceAboutToStart (&device);
    render (engine, 0.1);   // lets the master gain settle

    juce::AudioBuffer<float> clip (2, 1000);

    for (int i = 0; i < 1000; ++i)
    {
        clip.setSample (0, i, 0.5f * std::sin ((float) i * 0.05f));
        clip.setSample (1, i, 0.25f * std::cos ((float) i * 0.03f));
    }

    check (! engine.isAuditioning() && rms (render (engine, 0.05), 0) == 0.0f, "nothing plays before an audition");

    engine.startAudition (clip);
    check (engine.isAuditioning() && engine.getAuditionPosition() == 0, "an audition starts at the first sample");
    auto out = render (engine, 1000.0 / testRate);   // four blocks: 1024 samples
    bool same = true;

    for (int i = 0; i < 1000; ++i)
        same = same && std::abs (out.getSample (0, i) - clip.getSample (0, i)) < 1.0e-5f
               && std::abs (out.getSample (1, i) - clip.getSample (1, i)) < 1.0e-5f;

    check (same, "it plays the buffer sample for sample, left and right");
    check (out.getMagnitude (0, 1000, 24) == 0.0f && ! engine.isAuditioning() && engine.getAuditionPosition() == 1000,
           "it stops at the end, silent after it, with the position at the buffer's length");

    engine.startAudition (clip);
    render (engine, 256.0 / testRate);
    check (engine.isAuditioning() && engine.getAuditionPosition() == 256, "the position advances by a block");
    engine.stopAudition();
    out = render (engine, 0.05);
    check (! engine.isAuditioning() && rms (out, 0) == 0.0f, "stopping is silent at once");

    engine.startAudition (clip, 500);
    out = render (engine, 256.0 / testRate);
    check (std::abs (out.getSample (0, 0) - clip.getSample (0, 500)) < 1.0e-5f && std::abs (out.getSample (0, 255) - clip.getSample (0, 755)) < 1.0e-5f,
           "it can start in the middle");

    juce::AudioBuffer<float> mono (1, 600);

    for (int i = 0; i < 600; ++i)
        mono.setSample (0, i, 0.3f);

    engine.startAudition (mono);
    out = render (engine, 256.0 / testRate);
    check (std::abs (out.getSample (0, 10) - 0.3f) < 1.0e-5f && std::abs (out.getSample (1, 10) - 0.3f) < 1.0e-5f, "a mono buffer plays on both sides");

    engine.startAudition (clip);
    engine.play();
    out = render (engine, 0.05);
    check (! engine.isAuditioning() && rms (out, 0) == 0.0f, "starting the transport stops it");
    engine.stop();
    render (engine, 0.05);

    engine.startAudition (clip);
    engine.beginOffline (testRate, testBlock);
    juce::AudioBuffer<float> bounce (2, testBlock);
    float* channels[] = { bounce.getWritePointer (0), bounce.getWritePointer (1) };
    engine.renderOffline (channels, 2, testBlock);
    check (bounce.getMagnitude (0, 0, testBlock) == 0.0f, "a bounce never hears it");
    engine.endOffline();
    engine.stopAudition();
}

static void runDrumTriggerGuideTests()
{
    namespace tg = daw::triggerguide;
    namespace hf = daw::analysis::hits;
    std::cout << "How drum triggers work" << std::endl;

    const auto near = [] (double a, double b, double tolerance) { return std::abs (a - b) < tolerance; };

    // Texts.
    bool allText = true;

    for (auto topic : tg::allTopics)
        allText = allText && tg::getTopicName (topic).isNotEmpty() && tg::getTopicSummary (topic).isNotEmpty() && tg::explain (topic).length() > 90;

    check (allText && std::size (tg::allTopics) == 9, "the guide has nine topics, each with a name, a summary and a text");
    check (tg::explain (tg::Topic::findingHits).contains ("5.8 ms") && tg::explain (tg::Topic::velocity).contains ("DLS")
               && tg::explain (tg::Topic::whichDrum).contains ("note 42") && tg::explain (tg::Topic::replaceOrLayer).contains ("comb filtering")
               && tg::explain (tg::Topic::grid).contains ("beat 11"),
           "the topics carry their facts");
    check (tg::explain (tg::Topic::yourClip).startsWith ("Right-click an audio clip with drums"), "without a part, Your clip says how to get one");
    bool tips = true;

    for (auto tip : { tg::Tip::sensitivity, tg::Tip::minimumGap, tg::Tip::writeAs, tg::Tip::velocity, tg::Tip::keepFeel, tg::Tip::snapTo, tg::Tip::strength,
                      tg::Tip::keepPlaying, tg::Tip::muteClip, tg::Tip::listen, tg::Tip::hear, tg::Tip::makeTrack, tg::Tip::help })
        tips = tips && tg::getTooltip (tip).isNotEmpty();

    check (tips, "every control has a tooltip");

    // The numbers.
    check (near (tg::getBeatsFromMs (30.0, 120.0), 0.06, 1.0e-9) && near (tg::getMsFromBeats (0.06, 120.0), 30.0, 1.0e-9),
           "30 ms at 120 BPM is 0.06 beat");
    check (near (tg::getPeriodMs (60.0), 16.667, 0.001), "a 60 Hz wave lasts 16.67 ms");
    check (near (tg::getSumGainDb (60.0, 0.0), 6.02, 0.05) && tg::getSumGainDb (60.0, 1000.0 / 120.0) < -40.0,
           "60 Hz in line adds +6.02 dB, and 8.33 ms apart cancels (below -40 dB)");
    check (near (tg::getNotchHz (1.0, 0), 500.0, 1.0e-9) && near (tg::getNotchHz (1.0, 1), 1500.0, 1.0e-9) && near (tg::getNotchHz (1.0, 2), 2500.0, 1.0e-9),
           "1 ms apart cancels 500, 1500 and 2500 Hz");
    check (near (tg::getSumGainDb (500.0, 1.0), -120.0, 1.0e-9), "and the sum at a notch is silent");

    const int dbs[] { 0, -6, -12, -20, -30, -40 };
    const int natural[] { 127, 90, 64, 40, 23, 13 }, gentle[] { 127, 101, 80, 59, 40, 27 }, strong[] { 127, 64, 32, 13, 4, 1 };
    bool curves = true;

    for (int i = 0; i < 6; ++i)
        curves = curves && std::abs (tg::getCurveVelocity (hf::VelocityCurve::natural, -dbs[i]) - natural[i]) <= 1
                 && std::abs (tg::getCurveVelocity (hf::VelocityCurve::gentle, -dbs[i]) - gentle[i]) <= 1
                 && std::abs (tg::getCurveVelocity (hf::VelocityCurve::strong, -dbs[i]) - strong[i]) <= 1
                 && tg::getCurveVelocity (hf::VelocityCurve::same, -dbs[i]) == 100;

    check (curves, "the velocity curves at 0, -6, -12, -20, -30 and -40 dB match the table");
    check (tg::countKept (-30.0) == 7 && tg::countKept (-24.0) == 4 && tg::countKept (-18.0) == 2, "the snare track keeps 7, 4 and 2 strokes at -30, -24 and -18 dB");
    check (tg::getGridOffsetsMs().size() == 8 && near (tg::getSnappedOffsetMs (30.0, 0.5), 15.0, 1.0e-9) && near (tg::getSnappedOffsetMs (30.0, 1.0), 0.0, 1.0e-9)
               && near (tg::getSnappedOffsetMs (30.0, 0.0), 30.0, 1.0e-9),
           "snapping 30 ms late: 30, 15 and 0 ms at 0, 50 and 100%");

    // Rules, on made-up parts.
    using tg::PartHit;
    const auto hit = [] (double beat, hf::Drum drum, int velocity) { return PartHit { beat, drum, velocity }; };
    const auto said = [] (const std::vector<juce::String>& lines, const juce::String& text)
    {
        return std::any_of (lines.begin(), lines.end(), [&text] (const juce::String& l) { return l.contains (text); });
    };

    tg::Part loop;
    loop.lengthBars = 2.0;
    loop.bpm = 120.0;

    for (double b : { 0.0, 2.0, 4.0, 6.0, 6.75 })
        loop.hits.push_back (hit (b, hf::Drum::kick, 127));

    for (double b : { 1.0, 3.0, 5.0, 7.0 })
        loop.hits.push_back (hit (b, hf::Drum::snare, 127));

    loop.hits.push_back (hit (3.75, hf::Drum::snare, 36));

    for (int i = 0; i < 8; ++i)
        loop.hits.push_back (hit (0.5 + i, hf::Drum::hihat, i % 2 == 0 ? 127 : 113));

    auto lines = tg::analyse (loop);
    check (! lines.empty() && lines[0] == "18 hits in 2 bars (9 a bar): 5 kick, 5 snare, 8 hi-hat.", "rule 1: the counts, " + (lines.empty() ? juce::String() : lines[0]));
    check (said (lines, "The hi-hat plays 4 a bar, as many as there are quarter notes") && ! said (lines, "That's"),
           "rule 2: the hi-hats' steady pulse is said, and 9 hits a bar is no pulse");
    check (said (lines, "Most snare hits land on beats 2 and 4"), "rule 3: the backbeat");
    check (said (lines, "1 quiet snare hit (velocity 40 or less)"), "rule 6: the ghost note");
    check (said (lines, "sit on the 1/16 grid (on average 0 ms off)"), "rule 7: on the grid");
    check (! said (lines, "less than 30 ms") && ! said (lines, "same velocity"), "no close pairs and no even velocities in the loop");

    auto spaced = loop;
    spaced.hits.push_back (hit (0.03, hf::Drum::kick, 127));   // 15 ms after the first kick
    check (said (tg::analyse (spaced), "1 pair of hits is less than 30 ms apart") && said (tg::analyse (spaced), "Minimum gap of 50 ms"),
           "rule 4: a pair 15 ms apart");

    tg::Part flat;
    flat.lengthBars = 1.0;

    for (int i = 0; i < 8; ++i)
        flat.hits.push_back (hit (0.5 * i, hf::Drum::snare, i % 2 == 0 ? 100 : 95));

    lines = tg::analyse (flat);
    check (said (lines, "That's 8 hits a bar on average, as many as there are eighth notes") && said (lines, "about the same velocity (95 to 100)"),
           "rules 2 and 5: eight hits a bar, all about the same");
    flat.sameForAll = true;
    check (! said (tg::analyse (flat), "same velocity"), "rule 5 is quiet when Same for all is chosen");

    tg::Part loose;
    loose.lengthBars = 1.0;

    for (int i = 0; i < 8; ++i)
        loose.hits.push_back (hit (0.5 * i + 0.1 * ((i % 2) ? 1.0 : -1.0) * 0.5, hf::Drum::kick, 60 + i * 9));   // 0.05 beat = 25 ms off

    check (said (tg::analyse (loose), "off the 1/16 grid: a loose part"), "rule 7: a loose part (25 ms off)");

    tg::Part tight;
    tight.lengthBars = 1.0;

    for (int i = 0; i < 8; ++i)
        tight.hits.push_back (hit (0.5 * i + 0.02, hf::Drum::kick, 40 + i * 10));   // 10 ms off at 120 BPM

    check (said (tg::analyse (tight), "On average 10 ms off the 1/16 grid: tight"), "rule 7: tight with a human feel");

    tg::Part few;
    few.lengthBars = 1.0;
    few.hits = { hit (0.0, hf::Drum::kick, 100), hit (1.0, hf::Drum::snare, 90), hit (2.0, hf::Drum::kick, 80) };
    lines = tg::analyse (few);
    check (lines.size() == 1 && lines[0] == "3 hits in 1 bar (3 a bar): 2 kick, 1 snare.", "a short part says only its counts: " + (lines.empty() ? juce::String() : lines[0]));
    few.showDrums = false;
    check (tg::describeCounts (few) == "3 hits in 1 bar (3 a bar).", "the drums are left out when the part isn't sorted");
    check (tg::analyse (tg::Part()).empty() && tg::describeCounts (tg::Part()).isEmpty(), "no hits, no lines");
    check (tg::explain (tg::Topic::yourClip, &loop).contains ("18 hits in 2 bars") && tg::explain (tg::Topic::yourClip, &loop).contains ("\n\n"),
           "Your clip joins the lines");

    // A part from the notes of a drum clip.
    const auto fromNotes = tg::getPartFromNotes ({ { 36, 0.0, 120 }, { 38, 1.0, 100 }, { 46, 0.5, 80 }, { 45, 2.0, 90 }, { 99, 3.0, 90 } }, 8.0, 120.0);
    check (fromNotes.hits.size() == 3 && fromNotes.lengthBars == 2.0 && fromNotes.hits[1].drum == hf::Drum::hihat && fromNotes.hits[2].drum == hf::Drum::snare,
           "a drum clip's notes: kick, snare and hi-hat, in time order, toms and others left out");
}

static void runChordSheetTests()
{
    using namespace daw;
    namespace sheet = gen::sheet;
    std::cout << "Chord sheets" << std::endl;

    auto names = [] (const std::vector<sheet::Entry>& entries, bool numerals)
    {
        juce::StringArray list;

        for (auto& e : entries)
            list.add (numerals ? e.numeral : e.name);

        return list.joinIntoString (" ");
    };

    {
        const auto c = sheet::make ({ 0, MusicalKey::Scale::major });
        check (names (c.chords, false) == "C Dm Em F G Am Bdim" && names (c.chords, true) == juce::CharPointer_UTF8 ("I ii iii IV V vi vii\xc2\xb0")
                   && c.chords[0].family == gen::guide::Family::home && c.chords[4].family == gen::guide::Family::tension,
               "a major key's sheet has its seven chords with their numerals and families: " + names (c.chords, false));
        check (names (c.sevenths, false) == "Cmaj7 Dm7 Em7 Fmaj7 G7 Am7 Bm7b5" && c.sevenths[6].numeral == juce::CharPointer_UTF8 ("vii\xc3\xb8" "7"),
               "its sevenths: " + names (c.sevenths, false) + ", the last written " + c.sevenths[6].numeral);
        check (names (c.borrowed, false) == "Fm Ab Bb" && names (c.borrowed, true) == "iv bVI bVII" && c.keySignature == "no sharps or flats"
                   && ! c.progressions.empty() && c.progressions[0].numerals == "I - V - vi - IV",
               "the chords a major key borrows (" + names (c.borrowed, false) + "), its key signature and its progressions (" + c.progressions[0].numerals + ")");

        const auto am = sheet::make ({ 9, MusicalKey::Scale::minor });
        const auto d = sheet::make ({ 2, MusicalKey::Scale::major });
        check (names (am.borrowed, false) == "E E7" && d.keySignature.startsWith ("2 sharps") && d.keySignature.endsWith ("F# C#"),
               "a minor key borrows its major dominant (" + names (am.borrowed, false) + "), and D major has " + d.keySignature);

        // Spelled for the key, not by pitch alone.
        const auto eb = sheet::make ({ 3, MusicalKey::Scale::major });
        const auto e = sheet::make ({ 4, MusicalKey::Scale::major });
        const auto db = sheet::make ({ 1, MusicalKey::Scale::major });
        const auto bb = sheet::describe ({ 0, MusicalKey::Scale::major }, { 10, { 0, 4, 7 } });
        const auto sharp = sheet::describe ({ 4, MusicalKey::Scale::major }, { 10, { 0, 4, 7 } });
        const auto inKey = sheet::describe ({ 4, MusicalKey::Scale::major }, { 8, { 0, 3, 7 } });
        check (bb.name == "Bb" && bb.numeral == "bVII" && bb.family == gen::guide::Family::outside && sharp.name == "A#" && inKey.name == "G#m"
                   && inKey.notes == "G# B D#" && inKey.numeral == "iii",
               "a song's chords are spelled for its key, also from outside it: " + bb.name + " (" + bb.numeral + ") in C major, " + sharp.name
                   + " and " + inKey.name + " in E major");
        check (names (eb.borrowed, false) == "Abm Cb Db" && names (e.chords, false) == "E F#m G#m A B C#m D#dim" && e.chords[2].notes == "G# B D#"
                   && db.keyName == "Db major" && names (db.chords, false) == "Db Ebm Fm Gb Ab Bbm Cdim" && eb.progressions[0].chords[1].name == "Bb",
               "chords are spelled for the key: Eb major borrows " + names (eb.borrowed, false) + ", E major has " + names (e.chords, false)
                   + ", and " + db.keyName + " has " + names (db.chords, false));
    }

    {
        const auto eb = sheet::findCapo ({ 3, MusicalKey::Scale::major });
        const auto g = sheet::findCapo ({ 7, MusicalKey::Scale::major });
        check (eb.fret > 0 && eb.easyWith > eb.easyWithout && eb.shapesKey.tonic == ((3 - eb.fret) % 12 + 12) % 12 && g.fret == 0,
               "a capo on fret " + juce::String (eb.fret) + " makes Eb major easier on guitar (the shapes of " + eb.shapesKey.getName() + ", "
                   + juce::String (eb.easyWith) + " easy chords instead of " + juce::String (eb.easyWithout) + "); G major needs none");
    }

    {
        // Two pages, red and blue; every object where the cross-reference table says it is.
        std::vector<juce::Image> pages;

        for (auto colour : { juce::Colours::red, juce::Colours::blue })
        {
            juce::Image image (juce::Image::RGB, 60, 85, false);
            image.clear (image.getBounds(), colour);
            pages.push_back (image);
        }

        juce::MemoryOutputStream out;
        const auto result = pdf::write (pages, out, juce::CharPointer_UTF8 ("Chords in F\xe2\x99\xaf major"));
        const std::string text (static_cast<const char*> (out.getData()), out.getDataSize());
        auto contains = [&text] (const std::string& part) { return text.find (part) != std::string::npos; };
        const auto start = std::stoll (text.substr (text.rfind ("startxref\n") + 10));
        bool offsetsRight = text.compare ((size_t) start, 4, "xref") == 0;
        std::istringstream table (text.substr ((size_t) start + 5));
        int first = 0, count = 0;
        table >> first >> count;
        std::string line;
        std::getline (table, line);
        std::getline (table, line);   // object 0, the free one

        for (int i = 1; i < count; ++i)
        {
            std::getline (table, line);
            const auto offset = (size_t) std::stoll (line);
            offsetsRight = offsetsRight && text.compare (offset, std::to_string (i).size() + 6, std::to_string (i) + " 0 obj") == 0;
        }

        // The first picture, unpacked again.
        const auto imageAt = text.find ("/Subtype /Image");
        const auto length = std::stoi (text.substr (text.find ("/Length ", imageAt) + 8));
        const auto dataAt = text.find ("stream\n", imageAt) + 7;
        juce::MemoryInputStream compressed (text.data() + dataAt, (size_t) length, false);
        juce::GZIPDecompressorInputStream unzip (compressed);
        juce::MemoryBlock pixels;
        unzip.readIntoMemoryBlock (pixels);
        const auto* rgb = static_cast<const juce::uint8*> (pixels.getData());

        check (result.wasOk() && text.rfind ("%PDF-1.4", 0) == 0 && text.size() > 6 && text.compare (text.size() - 6, 6, "%%EOF\n") == 0
                   && contains ("/Count 2") && count == 10 && offsetsRight && contains ("/MediaBox [0 0 595.28 841.89]")
                   && contains ("/Title <FEFF00430068006F") && contains ("266F"),
               "a PDF of two pages has its catalog, page list, properties and three objects per page, each where the table says");
        // Each row starts with its PNG filter byte; the first row of a flat colour has none to undo (or Sub, which gives the same pixel).
        check (pixels.getSize() == 60 * 85 * 3 + 85 && rgb[1] == 255 && rgb[2] == 0 && rgb[3] == 0,
               "a page's picture is stored losslessly as rows of red, green and blue, each led by its filter byte (" + juce::String ((int) pixels.getSize()) + " bytes)");
    }
}

/** The first image stream in a PDF's text, inflated and with its PNG filters undone, as rows of RGB. */
static juce::MemoryBlock readPdfImage (const std::string& text, size_t from, int width, int height, std::set<int>* filtersUsed = nullptr)
{
    const auto imageAt = text.find ("/Subtype /Image", from);
    const auto length = std::stoi (text.substr (text.find ("/Length ", imageAt) + 8));
    const auto dataAt = text.find ("stream\n", imageAt) + 7;
    juce::MemoryInputStream compressed (text.data() + dataAt, (size_t) length, false);
    juce::GZIPDecompressorInputStream unzip (compressed);
    juce::MemoryBlock raw;
    unzip.readIntoMemoryBlock (raw);
    const auto* in = static_cast<const juce::uint8*> (raw.getData());
    const size_t n = (size_t) width * 3;
    juce::MemoryBlock result (n * (size_t) height, true);
    auto* out = static_cast<juce::uint8*> (result.getData());

    if (raw.getSize() != (n + 1) * (size_t) height)
        return {};

    for (int y = 0; y < height; ++y)
    {
        const int filter = in[(size_t) y * (n + 1)];

        if (filtersUsed != nullptr)
            filtersUsed->insert (filter);


        for (size_t i = 0; i < n; ++i)
        {
            const int left = i >= 3 ? out[(size_t) y * n + i - 3] : 0;
            const int up = y > 0 ? out[(size_t) (y - 1) * n + i] : 0;
            const int upLeft = (y > 0 && i >= 3) ? out[(size_t) (y - 1) * n + i - 3] : 0;
            const int x = in[(size_t) y * (n + 1) + 1 + i];
            int value = x;

            if (filter == 1) value = x + left;
            else if (filter == 2) value = x + up;
            else if (filter == 4)
            {
                const int p = left + up - upLeft, pa = std::abs (p - left), pb = std::abs (p - up), pc = std::abs (p - upLeft);
                value = x + ((pa <= pb && pa <= pc) ? left : (pb <= pc ? up : upLeft));
            }

            out[(size_t) y * n + i] = (juce::uint8) value;
        }
    }

    return result;
}

static void runPdfTests()
{
    using namespace daw;
    std::cout << "PDF writer" << std::endl;

    auto toText = [] (juce::MemoryOutputStream& out) { return std::string (static_cast<const char*> (out.getData()), out.getDataSize()); };

    {
        juce::Image image (juce::Image::RGB, 37, 5, false);

        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 37; ++x)
                image.setPixelAt (x, y, juce::Colour ((juce::uint8) (x * 6), (juce::uint8) (y * 50), (juce::uint8) ((x * 7 + y * 13) % 256)));

        image.setPixelAt (3, 2, juce::Colours::white);
        image.setPixelAt (20, 4, juce::Colours::black);
        image.setPixelAt (36, 0, juce::Colour (12, 250, 99));

        pdf::Document document ("Test");
        document.addPage (image, pdf::a4Width, pdf::a4Height);
        juce::MemoryOutputStream out;
        const auto result = document.write (out);
        const auto text = toText (out);
        std::set<int> filters;
        const auto pixels = readPdfImage (text, 0, 37, 5, &filters);
        bool same = pixels.getSize() == 37 * 5 * 3;

        for (int y = 0; y < 5 && same; ++y)
            for (int x = 0; x < 37; ++x)
            {
                const auto* p = static_cast<const juce::uint8*> (pixels.getData()) + (size_t) (y * 37 + x) * 3;
                const auto c = image.getPixelAt (x, y);
                same = same && p[0] == c.getRed() && p[1] == c.getGreen() && p[2] == c.getBlue();
            }

        check (result.wasOk() && same && text.find ("/Predictor 15 /Colors 3 /BitsPerComponent 8 /Columns 37") != std::string::npos
                   && ! filters.empty() && *filters.rbegin() <= 4 && ! filters.count (3),
               "a picture written with PNG predictors comes back pixel for pixel (" + juce::String ((int) filters.size()) + " different row filters used)");
    }

    {
        juce::Image gradient (juce::Image::RGB, 256, 64, false);

        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 256; ++x)
                gradient.setPixelAt (x, y, juce::Colour ((juce::uint8) x, (juce::uint8) (y * 4), (juce::uint8) ((x + y * 2) / 2)));

        juce::MemoryOutputStream plain;
        {
            juce::GZIPCompressorOutputStream zip (plain, 6);
            std::vector<juce::uint8> row (256 * 3);

            for (int y = 0; y < 64; ++y)
            {
                for (int x = 0; x < 256; ++x)
                {
                    const auto c = gradient.getPixelAt (x, y);
                    row[(size_t) x * 3] = c.getRed();
                    row[(size_t) x * 3 + 1] = c.getGreen();
                    row[(size_t) x * 3 + 2] = c.getBlue();
                }

                zip.write (row.data(), row.size());
            }
        }

        pdf::Document document;
        document.addPage (gradient, pdf::a4Width, pdf::a4Height);
        juce::MemoryOutputStream out;
        document.write (out);
        const auto text = toText (out);
        const auto imageAt = text.find ("/Subtype /Image");
        const auto length = std::stoi (text.substr (text.find ("/Length ", imageAt) + 8));
        check (length < (int) plain.getDataSize(),
               "a smooth gradient is smaller with the predictors: " + juce::String (length) + " bytes against " + juce::String ((int) plain.getDataSize()));
    }

    // Finds the dictionary of object `number` in the text, using the cross-reference table.
    auto objectText = [] (const std::string& text, int number)
    {
        const auto start = std::stoll (text.substr (text.rfind ("startxref\n") + 10));
        std::istringstream table (text.substr ((size_t) start + 5));
        int first = 0, count = 0;
        table >> first >> count;
        std::string line;
        std::getline (table, line);

        for (int i = 0; i <= number; ++i)
            std::getline (table, line);

        const auto offset = (size_t) std::stoll (line);
        return text.substr (offset, text.find ("endobj", offset) - offset);
    };
    auto numberAfter = [] (const std::string& s, const std::string& key)
    {
        const auto at = s.find (key);
        return at == std::string::npos ? -1 : std::stoi (s.substr (at + key.size()));
    };

    {
        juce::Image page (juce::Image::RGB, 20, 28, false);
        page.clear (page.getBounds(), juce::Colours::white);
        pdf::Document document ("Bookmarks");
        document.addPage (page, pdf::a4Width, pdf::a4Height);
        document.addPage (page, pdf::a4Width, pdf::a4Height);
        document.addPage (page, pdf::a4Width, pdf::a4Height);
        document.setBookmarks ({ { "A", 0, 0 }, { "A1", 1, 1 }, { "B", 2, 0 } });
        juce::MemoryOutputStream out;
        document.write (out);
        const auto text = toText (out);
        const int root = numberAfter (text, "/Outlines ");
        const auto rootText = objectText (text, root);
        const int a = numberAfter (rootText, "/First "), b = numberAfter (rootText, "/Last ");
        const auto aText = objectText (text, a);
        const int a1 = numberAfter (aText, "/First ");
        const auto a1Text = objectText (text, a1);
        const auto bText = objectText (text, b);
        auto destPage = [&] (const std::string& t) { return numberAfter (t, "/Dest ["); };
        const auto pagesText = objectText (text, 2);
        const int firstPage = numberAfter (pagesText, "/Kids [");

        check (rootText.find ("/Type /Outlines") != std::string::npos && numberAfter (rootText, "/Count ") == 3 && a != b
                   && numberAfter (aText, "/Last ") == a1 && a1 != -1 && numberAfter (a1Text, "/Parent ") == a && numberAfter (bText, "/Prev ") == a
                   && numberAfter (aText, "/Next ") == b && destPage (aText) == firstPage && destPage (a1Text) == firstPage + 3 && destPage (bText) == firstPage + 6,
               "bookmarks make an outline: A with the child A1, then B, each jumping to its page");
    }

    {
        juce::Image page (juce::Image::RGB, 20, 28, false);
        page.clear (page.getBounds(), juce::Colours::white);
        pdf::Document document;
        document.addPage (page, pdf::a4Width, pdf::a4Height, { { { 40.0f, 100.0f, 200.0f, 20.0f }, 1 } });
        document.addPage (page, pdf::a4Width, pdf::a4Height);
        juce::MemoryOutputStream out;
        document.write (out);
        const auto text = toText (out);
        const auto page0 = objectText (text, 4), page1 = objectText (text, 7);
        const int linkNumber = numberAfter (page0, "/Annots [");
        const auto link = objectText (text, linkNumber);
        const auto rectAt = link.find ("/Rect [");
        juce::StringArray rect;
        rect.addTokens (juce::String (link.substr (rectAt + 7, link.find (']', rectAt) - rectAt - 7)), " ", "");
        check (link.find ("/Subtype /Link") != std::string::npos && rect.size() == 4 && std::abs (rect[0].getFloatValue() - 40.0f) < 0.01f
                   && std::abs (rect[1].getFloatValue() - 721.89f) < 0.01f && std::abs (rect[2].getFloatValue() - 240.0f) < 0.01f
                   && std::abs (rect[3].getFloatValue() - 741.89f) < 0.01f && numberAfter (link, "/Dest [") == 7 && page1.find ("/Annots") == std::string::npos,
               "a link on page 1 covers the right rectangle (" + rect.joinIntoString (" ") + ") and jumps to page 2's object");
    }

    {
        juce::Image page (juce::Image::RGB, 60, 85, false);
        page.clear (page.getBounds(), juce::Colours::red);
        pdf::Document document;
        juce::MemoryOutputStream nothing;
        check (document.write (nothing).failed() && document.getNumPages() == 0, "a document with no pages can't be written");
        document.addPage (page, pdf::a4Width, pdf::a4Height);
        document.addPage (page, pdf::letterWidth, pdf::letterHeight);
        juce::MemoryOutputStream out;
        document.write (out);
        const auto text = toText (out);
        check (document.getNumPages() == 2 && text.find ("/Outlines") == std::string::npos && text.find ("/Annots") == std::string::npos
                   && text.find ("/MediaBox [0 0 612.00 792.00]") != std::string::npos,
               "without bookmarks or links there is no outline or annotation list, and a page can have Letter's size");
    }
}

static void runGuideBookletTests()
{
    using namespace daw;
    std::cout << "Guide booklets" << std::endl;

    // Paper.
    {
        const auto a4 = booklet::getSize (booklet::Paper::a4), letter = booklet::getSize (booklet::Paper::letter);
        check (std::abs (a4.x - 595.28f) < 0.01f && std::abs (a4.y - 841.89f) < 0.01f && std::abs (letter.x - 612.0f) < 0.01f && std::abs (letter.y - 792.0f) < 0.01f,
               "A4 is 595.28 x 841.89 points and Letter 612 x 792");
        check (booklet::getPixelSize (booklet::Paper::a4, 300.0f) == juce::Point<int> (2480, 3508)
                   && booklet::getPixelSize (booklet::Paper::letter, 300.0f) == juce::Point<int> (2550, 3300)
                   && booklet::getPixelSize (booklet::Paper::a4, 36.0f) == juce::Point<int> (298, 421)
                   && booklet::getPixelSize (booklet::Paper::letter, 36.0f) == juce::Point<int> (306, 396),
               "a page at 300 dpi is 2480 x 3508 pixels on A4 and 2550 x 3300 on Letter; at 36 dpi 298 x 421 and 306 x 396");
        bool regions = true;

        for (auto r : { "US", "CA", "MX", "PH" })
            regions = regions && booklet::getDefaultPaper (r) == booklet::Paper::letter;

        for (auto r : { "GB", "BE", "DE", "" })
            regions = regions && booklet::getDefaultPaper (r) == booklet::Paper::a4;

        check (regions, "Letter is the default in the US, Canada, Mexico and the Philippines, A4 in Britain, Belgium, Germany and without a region");
        check (booklet::getName (booklet::Paper::a4).contains ("210 x 297") && booklet::getName (booklet::Paper::letter).contains ("8.5 x 11"),
               "the paper names give their sizes");
    }

    // Contrast.
    {
        check (std::abs (booklet::getContrastRatio (juce::Colours::black, juce::Colours::white) - 21.0f) < 0.01f
                   && std::abs (booklet::getContrastRatio (juce::Colours::red, juce::Colours::red) - 1.0f) < 0.001f
                   && std::abs (booklet::getContrastRatio (juce::Colour (0xff767676), juce::Colours::white) - 4.54f) < 0.02f,
               "contrast: black on white 21:1, a colour on itself 1:1, #767676 on white 4.54:1");
    }

    // Paper colours.
    {
        const auto bg = booklet::toPaper (theme::background);
        const auto light = booklet::toPaper (theme::panelLight);
        const auto text = booklet::toPaper (theme::text);
        const auto dim = booklet::toPaper (theme::textDim);
        const auto white = juce::Colours::white;
        auto hex = [] (juce::Colour c) { return "#" + c.toDisplayString (false).toLowerCase(); };

        check (bg.getRed() >= 250 && bg.getGreen() >= 250 && bg.getBlue() >= 250,
               "the app's background becomes white on paper (" + hex (bg) + ")");
        const float lightness = booklet::getOkLightness (light);
        check (lightness > 0.85f && lightness < 0.92f, "the lighter panel becomes a light grey (" + hex (light) + ", L " + juce::String (lightness, 2) + ")");
        check (booklet::getContrastRatio (text, light) >= 7.0f && booklet::getContrastRatio (text, white) >= 7.0f,
               "the text is near-black on paper: " + hex (text) + ", " + juce::String (booklet::getContrastRatio (text, light), 1) + ":1 on the panel");
        check (booklet::getContrastRatio (dim, light) >= 4.5f, "the dim text still reads on the panel: " + hex (dim) + ", "
                                                                   + juce::String (booklet::getContrastRatio (dim, light), 1) + ":1");

        auto hueDistance = [] (juce::Colour a, juce::Colour b)
        {
            float d = std::abs (a.getHue() - b.getHue()) * 360.0f;
            return std::min (d, 360.0f - d);
        };

        std::vector<juce::Colour> coloured { theme::accent, theme::meterGreen, theme::meterYellow, theme::meterRed };

        for (auto hexValue : { 0xff5cc8ffu, 0xff7d8bffu, 0xffb38cffu, 0xffff8fb1u, 0xffffa94du, 0xff5fd38du, 0xff4fd1c5u, 0xffe0b26bu })
            coloured.push_back (juce::Colour (hexValue));

        bool keep = true;
        juce::String worst;

        for (auto c : coloured)
        {
            const auto p = booklet::toPaper (c);

            if (hueDistance (c, p) > 8.0f || booklet::getContrastRatio (p, white) < 3.0f)
            {
                keep = false;
                worst = hex (c) + " -> " + hex (p);
            }
        }

        check (keep, "every colour with a meaning keeps its hue within 8 degrees and reads on white at 3:1 (the accent becomes " + hex (booklet::toPaper (theme::accent)) + ") " + worst);

        const auto selection = booklet::toPaper (theme::accent.withAlpha (0.35f).overlaidWith (juce::Colours::transparentBlack).interpolatedWith (theme::panelLight, 0.65f));
        check (booklet::getOkLightness (selection) >= 0.65f && selection.getBlue() >= selection.getRed() && selection.getBlue() >= selection.getGreen(),
               "the accent at 35% over the panel becomes a light blue, not a dark block (" + hex (selection) + ")");

        juce::Image three (juce::Image::ARGB, 3, 1, true);
        three.setPixelAt (0, 0, theme::background);
        three.setPixelAt (1, 0, theme::text);
        three.setPixelAt (2, 0, theme::accent.withAlpha (0.5f));
        auto mapped = three.createCopy();
        booklet::toPaper (mapped);
        const auto m0 = mapped.getPixelAt (0, 0), m1 = mapped.getPixelAt (1, 0), m2 = mapped.getPixelAt (2, 0);
        check (m0 == booklet::toPaper (theme::background) && m1 == booklet::toPaper (theme::text)
                   && std::abs (m2.getFloatAlpha() - 0.5f) < 0.01f && m2.getBlue() > m2.getRed(),
               "an image's pixels are mapped like single colours, and alpha is kept");
    }

    // Layout.
    auto makeGuide = [] (const juce::String& name, int topics, int paragraphs, juce::Point<int> picture)
    {
        booklet::Guide guide;
        guide.name = name;
        guide.summary = "A short summary of " + name;
        guide.area = "Effects";
        guide.colour = juce::Colour (0xff5cc8ff);
        guide.source = "a test";

        for (int t = 0; t < topics; ++t)
        {
            booklet::Topic topic;
            topic.name = name + " topic " + juce::String (t + 1);
            topic.summary = "Summary " + juce::String (t + 1);
            juce::StringArray texts;

            for (int p = 0; p < paragraphs; ++p)
                texts.add ("Paragraph " + juce::String (p + 1) + " of " + topic.name + ". A reverb is a sum of many echoes. It places a sound in a room.");

            topic.text = texts.joinIntoString ("\n\n");
            topic.pictureSize = picture;
            guide.topics.push_back (topic);
        }

        return guide;
    };

    {
        booklet::Booklet b;
        b.songLine = juce::String::fromUTF8 ("My Song \xc2\xb7 A minor \xc2\xb7 96 BPM");
        b.date = "26 October 2026";
        b.guides.push_back (makeGuide ("What is reverb?", 7, 3, { 774, 360 }));
        const booklet::Options options;
        const auto plan = booklet::makePlan (b, options);
        bool links = plan.pages.size() == 8 && plan.pages[0].links.size() == 7;

        for (int i = 0; links && i < 7; ++i)
            links = plan.pages[0].links[(size_t) i].targetPage == i + 1 && plan.topicPages[0][(size_t) i] == i + 1;

        bool bookmarks = plan.bookmarks.size() == 7;

        for (size_t i = 0; bookmarks && i < 7; ++i)
            bookmarks = plan.bookmarks[i].level == 0 && plan.bookmarks[i].page == (int) i + 1;

        bool covered = true;

        for (int i = 1; i < 8; ++i)
            covered = covered && plan.pages[(size_t) i].text == juce::Range<int> (0, b.guides[0].topics[(size_t) i - 1].text.length())
                      && plan.pages[(size_t) i].kind == booklet::PageInfo::Kind::topic;

        check (links && bookmarks && covered, "a guide of 7 short topics is 8 pages: a cover whose 7 links jump to the topics, 7 bookmarks, each text on its page");
        check (std::abs (plan.pages[1].pictureArea.getWidth() - 515.28f) < 0.01f, "a wide picture fills the text width (" + juce::String (plan.pages[1].pictureArea.getWidth(), 2) + " pt)");
    }

    {
        booklet::Booklet b;
        b.guides.push_back (makeGuide ("Long", 2, 60, { 400, 1000 }));
        b.guides.front().topics[0].pictureSize = { 400, 1000 };
        b.guides.front().topics[1].pictureSize = { 0, 0 };
        const booklet::Options options;
        const auto plan = booklet::makePlan (b, options);
        const auto& text = b.guides[0].topics[0].text;
        int first = plan.topicPages[0][0], second = plan.topicPages[0][1];
        bool joined = second - first >= 3 && plan.pages[(size_t) first].text.getStart() == 0 && plan.pages[(size_t) second - 1].text.getEnd() == text.length();

        for (int i = first + 1; i < second; ++i)
            joined = joined && plan.pages[(size_t) i].text.getStart() == plan.pages[(size_t) i - 1].text.getEnd() && plan.pages[(size_t) i].continued
                     && plan.pages[(size_t) i].text.getLength() > 0;

        check (joined && plan.pages[0].links[1].targetPage == second, "60 long paragraphs run over " + juce::String (second - first) + " pages, following each other exactly, and the next topic starts after them");

        const auto& pic = plan.pages[(size_t) first].pictureArea;
        check (pic.getHeight() <= 0.45f * pdf::a4Height + 0.01f && std::abs (pic.getWidth() / pic.getHeight() - 0.4f) < 0.01f
                   && std::abs (pic.getCentreX() - pdf::a4Width * 0.5f) < 0.1f,
               "a tall picture is at most 45% of the page's height, keeps its proportions and is centred (" + juce::String (pic.getHeight(), 2) + " pt tall)");
        check (plan.pages[(size_t) second].pictureArea.isEmpty() && plan.pages[(size_t) second].textTop < plan.pages[(size_t) first].textTop,
               "a topic without a picture starts its text higher");
    }

    {
        booklet::Booklet b;
        b.title = "Two guides";
        b.summary = "A set";
        b.guides.push_back (makeGuide ("Guide A", 3, 2, { 700, 300 }));
        b.guides.push_back (makeGuide ("Guide B", 4, 2, { 700, 300 }));
        const auto plan = booklet::makePlan (b, {});
        bool marks = plan.bookmarks.size() == 9;
        const int pagesOfMarks[] { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        const int levels[] { 0, 1, 1, 1, 0, 1, 1, 1, 1 };

        for (size_t i = 0; marks && i < 9; ++i)
            marks = plan.bookmarks[i].page == pagesOfMarks[i] && plan.bookmarks[i].level == levels[i];

        check (plan.pages.size() == 10 && plan.guidePages == std::vector<int> ({ 1, 5 }) && plan.pages[0].links.size() == 2
                   && plan.pages[0].links[0].targetPage == 1 && plan.pages[0].links[1].targetPage == 5
                   && plan.pages[1].kind == booklet::PageInfo::Kind::guideCover && plan.pages[5].kind == booklet::PageInfo::Kind::guideCover && marks,
               "a set of two guides: the set's cover, each guide's cover and its topics (" + juce::String ((int) plan.pages.size()) + " pages), with bookmarks nested by guide");
    }

    {
        booklet::Booklet b;
        b.guides.push_back (makeGuide ("Many", 40, 1, { 0, 0 }));
        const auto plan = booklet::makePlan (b, {});
        int contentsPages = 0, linkCount = 0;
        bool right = true;

        for (size_t i = 0; i < plan.pages.size(); ++i)
        {
            if (plan.pages[i].kind == booklet::PageInfo::Kind::cover || plan.pages[i].kind == booklet::PageInfo::Kind::contents)
            {
                ++contentsPages;

                for (auto& link : plan.pages[i].links)
                {
                    right = right && link.targetPage == plan.topicPages[0][(size_t) linkCount];
                    ++linkCount;
                }
            }
        }

        check (contentsPages > 1 && linkCount == 40 && right && plan.topicPages[0][0] == contentsPages,
               "40 topics need " + juce::String (contentsPages) + " contents pages; every topic has its link, and the first topic follows them");
    }

    {
        bool all = true;
        int count = 0;

        for (auto& entry : gen::guides::getEntries())
        {
            if (entry.topics.isEmpty())
                continue;

            booklet::Booklet b;
            booklet::Guide guide;
            guide.name = entry.name;
            guide.summary = entry.summary;
            guide.area = gen::guides::getAreaName (entry.area);

            for (auto& t : entry.topics)
            {
                booklet::Topic topic;
                topic.name = t;
                topic.text = "Text.";
                guide.topics.push_back (topic);
            }

            b.guides.push_back (guide);
            const auto plan = booklet::makePlan (b, {});
            all = all && (int) plan.pages.size() == 1 + entry.topics.size() && plan.pages[0].links.size() == (size_t) entry.topics.size();
            ++count;
        }

        check (all && count >= 7, "a booklet for each of the " + juce::String (count) + " guides with listed topics has a cover and a page per topic");
    }

    // Drawing and writing.
    {
        booklet::Booklet b;
        b.songLine = "My Song";
        b.guides.push_back (makeGuide ("What is reverb?", 7, 3, { 774, 360 }));
        booklet::Options options;
        options.dpi = 36.0f;
        options.paper = booklet::Paper::a4;

        for (auto& topic : b.guides[0].topics)
        {
            topic.picture = juce::Image (juce::Image::ARGB, 77, 36, true);
            topic.picture.clear (topic.picture.getBounds(), theme::panelLight);
        }

        const auto plan = booklet::makePlan (b, options);
        const auto page = booklet::renderPage (b, plan, 1, options);
        options.paper = booklet::Paper::letter;
        const auto letter = booklet::renderPage (b, booklet::makePlan (b, options), 1, options);
        const auto& area = plan.pages[1].pictureArea;
        const float scale = 36.0f / 72.0f;
        const auto centre = page.getPixelAt (juce::roundToInt (area.getCentreX() * scale), juce::roundToInt (area.getCentreY() * scale));
        bool footerInk = false;

        for (int x = 20; x < page.getWidth() - 20 && ! footerInk; ++x)
            for (int y = page.getHeight() - 28; y < page.getHeight() - 14; ++y)
                if (page.getPixelAt (x, y).getBrightness() < 0.9f)
                    footerInk = true;

        options.paper = booklet::Paper::a4;
        options.paperColours = false;
        const auto dark = booklet::renderPage (b, plan, 1, options);
        const auto darkCentre = dark.getPixelAt (juce::roundToInt (area.getCentreX() * scale), juce::roundToInt (area.getCentreY() * scale));

        check (page.getWidth() == 298 && page.getHeight() == 421 && letter.getWidth() == 306 && letter.getHeight() == 396
                   && page.getPixelAt (0, 0) == juce::Colours::white.withAlpha (1.0f) && footerInk,
               "a page at 36 dpi is 298 x 421 px on A4 and 306 x 396 on Letter, white in the corner, with the page number at the foot");
        check (centre.getRed() > 200 && centre.getGreen() > 200 && centre.getBlue() > 200 && darkCentre.getRed() < 80 && darkCentre.getGreen() < 80,
               "a dark picture is light on paper (red " + juce::String (centre.getRed()) + ") and stays dark as on screen (red " + juce::String (darkCentre.getRed()) + ")");

        options.paperColours = true;
        options.dpi = 36.0f;
        juce::MemoryOutputStream out;
        const auto result = booklet::write (b, options, out);
        const std::string text (static_cast<const char*> (out.getData()), out.getDataSize());
        size_t links = 0;

        for (size_t at = text.find ("/Subtype /Link"); at != std::string::npos; at = text.find ("/Subtype /Link", at + 1))
            ++links;

        check (result.wasOk() && text.find ("/Count 8") != std::string::npos && text.find ("/Type /Outlines") != std::string::npos && links == 7
                   && text.find ("/Title <FEFF0057") != std::string::npos,
               "writing the booklet gives a PDF of 8 pages with an outline, 7 links and the guide's name as its title");
    }
}

static void runBounceTests()
{
    using namespace daw;
    std::cout << "Bounce to WAV" << std::endl;

    const auto folder = juce::File::createTempFile ("bounce");
    folder.createDirectory();
    const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    engine.setTempo (120.0);

    auto* keys = engine.addTrack (Track::Type::midi);
    auto* clip = engine.addMidiClip (*keys, 0.0, 16.0);
    {
        const juce::ScopedLock sl (engine.getLock());
        for (int beat = 0; beat < 16; beat += 2)
            clip->notes.push_back ({ 60 + beat % 7, (double) beat, 1.5, 0.8f });
    }
    engine.notesChanged();

    check (bounce::getDefaultRange (engine) == juce::Range<double> (0.0, 16.0), "with the loop off, the whole song is bounced: bars 1 to 4");
    engine.loopStartBeat = 4.0;
    engine.loopEndBeat = 12.0;
    engine.loopEnabled = true;
    check (bounce::getDefaultRange (engine) == juce::Range<double> (4.0, 12.0), "with the loop on, the loop range is bounced");

    auto read = [] (const juce::File& file)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        juce::AudioBuffer<float> audio;
        double rate = 0.0;
        int bits = 0;

        if (reader != nullptr)
        {
            audio.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&audio, 0, (int) reader->lengthInSamples, 0, true, true);
            rate = reader->sampleRate;
            bits = (int) reader->bitsPerSample;
        }

        return std::tuple { audio, rate, bits };
    };

    // The loop range, at 44.1 kHz in 16-bit, with the metronome on: it must not be in the file.
    engine.metronomeEnabled = true;
    engine.setPositionBeats (2.0);
    bounce::Settings settings;
    settings.startBeat = 4.0;
    settings.endBeat = 12.0;
    settings.sampleRate = 44100.0;
    settings.bitDepth = 16;
    const auto loopFile = folder.getChildFile ("Loop.wav");
    const auto loopBounce = bounce::bounceToFile (engine, settings, loopFile);
    const auto [loopAudio, loopRate, loopBits] = read (loopFile);

    check (loopBounce.result.wasOk() && loopAudio.getNumChannels() == 2 && juce::approximatelyEqual (loopRate, 44100.0) && loopBits == 16,
           "the loop is bounced to a stereo 16-bit WAV at 44.1 kHz " + loopBounce.result.getErrorMessage());
    check (loopAudio.getNumSamples() == 4 * 44100 && juce::approximatelyEqual (loopBounce.seconds, 4.0),
           "two bars at 120 BPM make exactly 4 seconds");
    check (rms (loopAudio, 0) > 0.01f && loopBounce.peak > 0.01f && loopBounce.peak <= 1.0f, "the notes are in the file");
    check (engine.loopEnabled.load() && engine.metronomeEnabled.load() && ! engine.isPlaying() && ! engine.isOffline(),
           "afterwards the loop, the metronome and the stopped transport are as they were");
    check (! folder.getChildFile (".Loop.bouncing.wav").exists(), "no temporary file is left behind");

    // Only the metronome: a silent bounce.
    {
        Engine quiet;
        quiet.deviceManager.removeAudioCallback (&quiet);
        quiet.metronomeEnabled = true;
        quiet.countInEnabled = true;
        quiet.setSections ({ { 0, 1, SectionType::intro, {} } });
        bounce::Settings oneBar;
        oneBar.endBeat = bounce::getSongRange (quiet).getEnd();
        const auto file = folder.getChildFile ("Click.wav");
        const auto outcome = bounce::bounceToFile (quiet, oneBar, file);
        const auto [audio, rate, bits] = read (file);
        check (outcome.result.wasOk() && audio.getNumSamples() == 96000 && audio.getMagnitude (0, audio.getNumSamples()) < 1.0e-6f && bits == 24,
               "the metronome and count-in are left out (24-bit by default)");
    }

    // The whole song, in 32-bit float, with a tail that is cut once it has died away.
    settings = {};
    settings.startBeat = 0.0;
    settings.endBeat = 16.0;
    settings.bitDepth = 32;
    settings.tailSeconds = 4.0;
    {
        const juce::ScopedLock sl (engine.getLock());
        clip->notes.push_back ({ 48, 15.0, 1.0, 1.0f });   // still sounding at the end
    }
    engine.notesChanged();
    const auto songFile = folder.getChildFile ("Song.wav");
    const auto song = bounce::bounceToFile (engine, settings, songFile);
    const auto [songAudio, songRate, songBits] = read (songFile);
    check (song.result.wasOk() && songBits == 32 && songAudio.getNumSamples() > 8 * 48000 && songAudio.getNumSamples() < 12 * 48000,
           "the song gets a tail, trimmed to where it has died away (" + juce::String (song.seconds, 2) + " s)");
    check (songAudio.getMagnitude (0, 8 * 48000 - 2400, 2400) > 1.0e-3f, "the last note still sounds at the end of the range");

    // Cancelling leaves nothing.
    const auto cancelledFile = folder.getChildFile ("Cancelled.wav");
    {
        bounce::Renderer renderer (engine, settings);
        const auto outcome = renderer.render (cancelledFile, [] (double progress) { return progress < 0.2; });
        check (outcome.cancelled && ! outcome.result.wasOk() && ! cancelledFile.exists(), "a cancelled bounce leaves no file");
    }

    bounce::Settings emptyRange;
    emptyRange.startBeat = emptyRange.endBeat = 8.0;
    check (! bounce::bounceToFile (engine, emptyRange, folder.getChildFile ("Empty.wav")).result.wasOk(),
           "an empty range isn't bounced");
}

namespace
{
/** A song for the bounce focus tests: deterministic sines in audio clips at 48 kHz, so a bounce can be compared sample by sample. */
struct FocusSong
{
    FocusSong()
    {
        engine.deviceManager.removeAudioCallback (&engine);
        engine.setTempo (120.0);
    }

    daw::Track* addAudio (const juce::String& name, float hz, float level = 0.25f, double seconds = 2.0, double startBeat = 0.0, bool stereo = false)
    {
        auto* t = engine.addTrack (daw::Track::Type::audio);
        t->name = name;
        juce::AudioBuffer<float> sine (stereo ? 2 : 1, (int) (seconds * 48000.0));

        for (int ch = 0; ch < sine.getNumChannels(); ++ch)
            for (int i = 0; i < sine.getNumSamples(); ++i)
                sine.setSample (ch, i, level * (ch == 1 ? 0.5f : 1.0f) * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / 48000.0f));

        engine.addAudioClip (*t, std::move (sine), 48000.0, startBeat, name);
        return t;
    }

    juce::AudioBuffer<float> render (daw::bounce::Settings s, double endBeat = 4.0)
    {
        s.sampleRate = 48000.0;
        s.endBeat = endBeat;
        s.startBeat = 0.0;
        daw::bounce::Renderer renderer (engine, s);
        juce::AudioBuffer<float> audio;
        renderer.renderToBuffer (audio);
        return audio;
    }

    daw::bounce::Settings soloed (std::vector<const daw::Track*> tracks)
    {
        daw::bounce::Settings s;
        s.focus.mode = daw::BounceFocus::Mode::heardAsSoloed;
        s.focus.tracks = std::move (tracks);
        return s;
    }

    daw::bounce::Settings chain (const daw::Track* track, const daw::Clip* clip = nullptr, bool instrumentOnly = false)
    {
        daw::bounce::Settings s;
        s.focus.mode = daw::BounceFocus::Mode::oneChain;
        s.focus.track = track;
        s.focus.clip = clip;
        s.focus.instrumentOnly = instrumentOnly;
        return s;
    }

    static float difference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumSamples() != b.getNumSamples())
            return 1000.0f;

        float most = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                most = juce::jmax (most, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));

        return most;
    }

    daw::Engine engine;
};
} // namespace

static void runBounceFocusTests()
{
    using namespace daw;
    std::cout << "Bounce focus: selected tracks and one chain" << std::endl;

    // Heard as soloed: the selected tracks stand in for the solo buttons.
    {
        FocusSong s;
        auto* a = s.addAudio ("A", 440.0f);
        auto* b = s.addAudio ("B", 660.0f);
        auto* reverb = s.engine.addReturn ("Verb");
        s.engine.setSend (*a, *reverb, -6.0f);

        b->muted = true;
        const auto withoutB = s.render ({});
        b->muted = false;
        const auto selected = s.render (s.soloed ({ a }));
        check (FocusSong::difference (withoutB, selected) < 1.0e-6f,
               "selected tracks: the bounce equals the mix with the others muted, with the return's share");
        check (selected.getMagnitude (0, selected.getNumSamples()) > 0.2f, "selected tracks: the track is heard");

        b->soloed = true;
        const auto soloedB = s.render (s.soloed ({ a }));
        check (FocusSong::difference (withoutB, soloedB) < 1.0e-6f && b->soloed.load() && ! a->soloed.load() && ! reverb->soloed.load(),
               "selected tracks: the solo buttons change nothing in it and are not touched");
        b->soloed = false;

        const auto both = s.render ({});
        check (both.getMagnitude (0, both.getNumSamples()) > selected.getMagnitude (0, selected.getNumSamples()) + 0.1f,
               "the plain mix still has every track after a focused bounce");
    }

    // A selected track in a bus keeps the bus fader.
    {
        FocusSong s;
        auto* a = s.addAudio ("A", 440.0f);
        auto* b = s.addAudio ("B", 660.0f);
        auto* bus = s.engine.createBus ({ a, b }, "Group");
        bus->volumeDb = 6.0f;
        const float alone = s.render (s.soloed ({ a })).getMagnitude (0, 96000);
        bus->volumeDb = 0.0f;
        const float plain = s.render (s.soloed ({ a })).getMagnitude (0, 96000);
        check (std::abs (alone / plain - 1.995f) < 0.01f, "selected tracks: a +6 dB bus fader makes the track " + juce::String (alone / plain, 3) + " times louder");
        const auto busSelected = s.render (s.soloed ({ bus }));
        check (busSelected.getMagnitude (0, 96000) > plain * 1.5f, "selecting a bus bounces everything in it");
    }

    // One chain: the effect stays, the fader, pan and master volume don't. Each render is the first of a fresh song,
    // because DawIt EQ's match level learns as it plays.
    {
        auto make = [] (FocusSong& s)
        {
            auto* a = s.addAudio ("A", 1000.0f);
            s.addAudio ("B", 660.0f);
            juce::String error;
            auto* slot = s.engine.addPlugin (*a, BuiltInEq::getDescription(), error);
            auto* eqPlugin = slot != nullptr ? dynamic_cast<BuiltInEq*> (slot->instance.get()) : nullptr;

            if (eqPlugin != nullptr)
                eqPlugin->setBand (0, { true, eq::BandType::bell, 1000.0f, 6.0f, 1.0f, 12 });

            return a;
        };

        FocusSong plainSong, chainSong;
        auto* plainTrack = make (plainSong);
        auto* chainTrack = make (chainSong);
        const auto expected = plainSong.render (plainSong.soloed ({ plainTrack }));
        chainTrack->volumeDb = -10.0f;
        chainTrack->pan = -1.0f;
        chainSong.engine.masterVolumeDb = -6.0f;
        const auto chain = chainSong.render (chainSong.chain (chainTrack));
        const float most = FocusSong::difference (chain, expected);
        check (most < 1.0e-6f && expected.getMagnitude (0, 96000) > 0.3f,
               "one chain: the clip through the EQ alone, with no fader, pan or master volume (" + juce::String (most, 8) + ")");
        check (chain.getMagnitude (1, 0, 96000) > 0.3f, "one chain: the right channel is not silenced by a pan to the left");

        // Instrument only: the clip times its gain, no effects.
        auto* audioClip = static_cast<AudioClip*> (chainTrack->clips[0].get());
        chainSong.engine.setClipGain (*audioClip, -3.0f);
        const auto dry = chainSong.render (chainSong.chain (chainTrack, nullptr, true));
        float worst = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < audioClip->audio.getNumSamples(); ++i)
                worst = juce::jmax (worst, std::abs (dry.getSample (ch, i) - audioClip->audio.getSample (0, i) * audioClip->getGain()));

        check (worst < 1.0e-6f && dry.getNumSamples() == 96000, "instrument only: equals the clip times its gain, both channels (" + juce::String (worst, 8) + ")");
    }

    // The clip filter: the second of two clips alone equals the track with the first deleted.
    {
        FocusSong s;
        auto* a = s.addAudio ("A", 440.0f, 0.25f, 1.0, 0.0);
        juce::AudioBuffer<float> second (1, 48000);

        for (int i = 0; i < 48000; ++i)
            second.setSample (0, i, 0.3f * std::sin (juce::MathConstants<float>::twoPi * 880.0f * (float) i / 48000.0f));

        auto* clip2 = s.engine.addAudioClip (*a, std::move (second), 48000.0, 4.0, "two");
        const auto only = s.render (s.chain (a, clip2), 6.0);
        s.engine.removeClip (*a, a->clips[0].get());
        const auto removed = s.render (s.chain (a), 6.0);
        check (FocusSong::difference (only, removed) < 1.0e-5f, "one clip: bouncing the second clip equals the track with the first deleted");
    }

    // Normal playback is not changed by the focus: the solo buttons still decide when nothing is focused.
    {
        FocusSong s;
        auto* a = s.addAudio ("A", 440.0f);
        auto* b = s.addAudio ("B", 660.0f);
        a->soloed = true;
        const auto aloud = s.render ({});
        b->muted = false;
        a->soloed = false;
        b->muted = true;
        const auto withoutB = s.render ({});
        check (FocusSong::difference (aloud, withoutB) < 1.0e-6f, "a bounce of the mix still follows the solo buttons");
    }
}

static void runBounceInPlaceTests()
{
    using namespace daw;
    std::cout << "Bounce in place" << std::endl;

    const auto folder = juce::File::createTempFile ("bounceinplace");
    folder.createDirectory();
    const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

    // An audio track with an EQ, in a bus, with a send: the mix is the same afterwards.
    {
        FocusSong s;
        auto* a = s.addAudio ("Lead", 1000.0f, 0.25f, 2.0, 2.0);
        auto* other = s.addAudio ("Other", 330.0f);
        auto* bus = s.engine.createBus ({ a }, "Synths");
        auto* reverb = s.engine.addReturn ("Reverb");
        a->volumeDb = -8.0f;
        a->pan = 0.4f;
        a->colour = juce::Colours::orange;
        s.engine.setSend (*a, *reverb, -12.0f);
        routing::findSend (*a, *reverb)->preFader = true;
        juce::String error;
        auto* slot = s.engine.addPlugin (*a, BuiltInEq::getDescription(), error);
        dynamic_cast<BuiltInEq*> (slot->instance.get())->setBand (0, { true, eq::BandType::bell, 1000.0f, 6.0f, 1.0f, 12 });
        const int oldSize = (int) s.engine.getTracks().size();

        const auto before = s.render ({}, 12.0);
        const auto result = bounce::bounceInPlace (s.engine, *a);
        check (result.result.wasOk() && result.track != nullptr && result.clip != nullptr, "bounce in place works " + result.result.getErrorMessage());

        if (result.track != nullptr)
        {
            check ((int) s.engine.getTracks().size() == oldSize + 1 && s.engine.indexOf (result.track) == s.engine.indexOf (a) + 1,
                   "bounce in place: the new track is right below the original");
            check (result.track->isAudio() && result.track->bus == bus && result.track->name == "Lead (bounced)" && result.track->colour == a->colour,
                   "bounce in place: same bus, colour and a name with (bounced)");
            check (juce::approximatelyEqual (result.track->volumeDb.load(), -8.0f) && juce::approximatelyEqual (result.track->pan.load(), 0.4f)
                       && result.track->plugins.empty(),
                   "bounce in place: the fader and pan are copied and there are no effects");
            auto* send = routing::findSend (*result.track, *reverb);
            check (send != nullptr && juce::approximatelyEqual (send->levelDb.load(), -12.0f) && send->preFader.load(), "bounce in place: the send keeps its level and pre-fader choice");
            check (a->muted.load() && a->plugins.size() == 1 && a->clips.size() == 1 && ! a->clips[0]->muted, "bounce in place: the original is muted and otherwise untouched");
            check (juce::approximatelyEqual (result.clip->startBeat, 2.0), "bounce in place: the clip starts where the first clip started");

            const auto after = s.render ({}, 12.0);
            check (FocusSong::difference (before, after) < 1.0e-4f, "bounce in place: the mix afterwards equals the mix before (" + juce::String (FocusSong::difference (before, after), 6) + ")");
            check (other->clips.size() == 1, "bounce in place leaves other tracks alone");

            // Saves and loads with all of it.
            const auto file = folder.getChildFile ("Song.dawit");
            Engine loaded;
            loaded.deviceManager.removeAudioCallback (&loaded);
            juce::String text;
            juce::StringArray warnings;
            const bool ok = project::save (s.engine, file, text, error) && project::load (loaded, file, error, warnings);
            Track* reloaded = nullptr;

            for (auto& t : loaded.getTracks())
                if (t->name == "Lead (bounced)")
                    reloaded = t.get();

            check (ok && reloaded != nullptr && juce::approximatelyEqual (reloaded->volumeDb.load(), -8.0f) && juce::approximatelyEqual (reloaded->pan.load(), 0.4f)
                       && reloaded->sends.size() == 1 && reloaded->clips.size() == 1,
                   "a project with a bounced track saves and loads with its fader, pan and send " + error);
        }
    }

    // Clip mode: only the clip is muted, and a later clip reuses the "(bounced)" track.
    {
        FocusSong s;
        auto* a = s.addAudio ("Pad", 440.0f, 0.25f, 1.0, 0.0);
        juce::AudioBuffer<float> second (1, 48000);

        for (int i = 0; i < 48000; ++i)
            second.setSample (0, i, 0.2f * std::sin (juce::MathConstants<float>::twoPi * 880.0f * (float) i / 48000.0f));

        auto* clip2 = s.engine.addAudioClip (*a, std::move (second), 48000.0, 8.0, "two");
        const auto first = bounce::bounceInPlace (s.engine, *a, a->clips[0].get());
        check (first.result.wasOk() && a->clips[0]->muted && ! clip2->muted && ! a->muted.load(), "bounce clip in place: only that clip is muted");
        const auto secondResult = bounce::bounceInPlace (s.engine, *a, clip2);
        check (secondResult.result.wasOk() && secondResult.track == first.track && first.track->clips.size() == 2 && clip2->muted,
               "bounce clip in place: a free '(bounced)' track below is used again");
        const auto third = bounce::bounceInPlace (s.engine, *a, a->clips[0].get());
        check (! third.result.wasOk(), "a muted clip isn't bounced");

        // A clip in the way gets a new track.
        a->clips[0]->muted = false;
        const auto fourth = bounce::bounceInPlace (s.engine, *a, a->clips[0].get());
        check (fourth.result.wasOk() && fourth.track != first.track, "bounce clip in place: with a clip in the way, a new track is made");
    }

    // A track with no unmuted clip.
    {
        FocusSong s;
        auto* a = s.addAudio ("Muted", 440.0f);
        a->clips[0]->muted = true;
        check (! bounce::planInPlace (s.engine, *a).isOk() && ! bounce::bounceInPlace (s.engine, *a).result.wasOk(), "a track with no unmuted clip can't be bounced in place");
    }

    // Peak 2.0: turned down to -1 dBFS, with +7.02 dB of clip gain.
    {
        FocusSong s;
        auto* a = s.addAudio ("Loud", 440.0f, 2.0f);
        const auto result = bounce::bounceInPlace (s.engine, *a);
        const float stored = result.clip != nullptr ? result.clip->audio.getMagnitude (0, result.clip->audio.getNumSamples()) : 0.0f;
        check (result.result.wasOk() && std::abs (stored - 0.8913f) < 0.001f && std::abs (result.gainDb - 7.02f) < 0.05f
                   && std::abs (result.clip->gainDb - 7.02f) < 0.05f && result.note.isNotEmpty(),
               "a bounce that peaks at 2.0 is stored at 0.8913 with +7.02 dB of clip gain (" + juce::String (stored, 4) + ", " + juce::String (result.gainDb, 2) + ")");
        check (std::abs (stored * result.clip->getGain() - 2.0f) < 0.01f, "the clip gain makes up for the turning down");

        FocusSong huge;
        auto* h = huge.addAudio ("Huge", 440.0f, 8.0f);
        const auto big = bounce::bounceInPlace (huge.engine, *h);
        check (big.result.wasOk() && juce::approximatelyEqual (big.gainDb, 12.0f) && big.note.contains ("most a clip allows"), "a gain over +12 dB is limited, and says so");
    }

    // A MIDI track with a bypassed DawIt Tune lands in time.
    {
        auto bounceMidi = [] (bool withTune)
        {
            FocusSong s;
            auto* t = s.engine.addTrack (Track::Type::midi);
            t->name = "Keys";
            auto* clip = s.engine.addMidiClip (*t, 0.0, 4.0);
            {
                const juce::ScopedLock sl (s.engine.getLock());
                clip->notes.push_back ({ 60, 0.0, 1.0, 0.8f });
                clip->notes.push_back ({ 67, 2.0, 1.0, 0.8f });
            }
            s.engine.notesChanged();
            s.engine.setBuiltInSound (*t, BuiltInSound::pluck);   // the piano and the organ start from a random phase; the pluck doesn't
            juce::String error;

            if (withTune)
                if (auto* slot = s.engine.addPlugin (*t, BuiltInTune::getDescription(), error))
                    slot->bypassed = true;

            const auto result = bounce::bounceInPlace (s.engine, *t);
            return result.clip != nullptr ? result.clip->audio : juce::AudioBuffer<float>();
        };

        const auto plain = bounceMidi (false);
        const auto tuned = bounceMidi (true);
        float most = 0.0f;
        for (int ch = 0; ch < 2 && plain.getNumSamples() > 0; ++ch)
            for (int i = 0; i < juce::jmin (plain.getNumSamples(), tuned.getNumSamples()); ++i)
                most = juce::jmax (most, std::abs (plain.getSample (juce::jmin (ch, plain.getNumChannels() - 1), i) - tuned.getSample (juce::jmin (ch, tuned.getNumChannels() - 1), i)));

        check (plain.getNumSamples() > 48000 && most < 1.0e-5f, "a MIDI track with a bypassed DawIt Tune bounces in time (" + juce::String (most, 8) + ")");
    }

    // The instrument and sound of a MIDI track never change.
    {
        FocusSong s;
        auto* t = s.engine.addTrack (Track::Type::midi);
        auto* clip = s.engine.addMidiClip (*t, 0.0, 4.0);
        {
            const juce::ScopedLock sl (s.engine.getLock());
            clip->notes.push_back ({ 60, 0.0, 1.0, 0.8f });
        }
        s.engine.notesChanged();
        s.engine.setBuiltInSound (*t, BuiltInSound::organ);
        const auto result = bounce::bounceInPlace (s.engine, *t);
        check (result.result.wasOk() && t->builtInSound.load() == BuiltInSound::organ && ! t->hasInstrument() && t->clips.size() == 1 && clip->notes.size() == 1
                   && t->muted.load(),
               "bounce in place never changes the original's sound, notes or instrument");
    }

    // A clip as a WAV file.
    {
        FocusSong s;
        auto* a = s.addAudio ("Voice", 440.0f, 0.5f, 1.0, 0.0, true);
        auto* clip = static_cast<AudioClip*> (a->clips[0].get());
        s.engine.setClipGain (*clip, -6.0f);
        const auto file = folder.getChildFile ("Clip.wav");
        int bits = 0;
        check (bounce::writeClipToWav (*clip, file, &bits).wasOk() && bits == 24, "an audio clip is written as a 24-bit WAV");

        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        float most = 0.0f;
        bool shape = reader != nullptr && reader->numChannels == 2 && juce::approximatelyEqual (reader->sampleRate, 48000.0) && reader->lengthInSamples == 48000;

        if (shape)
        {
            juce::AudioBuffer<float> read (2, 48000);
            reader->read (&read, 0, 48000, 0, true, true);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 48000; ++i)
                    most = juce::jmax (most, std::abs (read.getSample (ch, i) - clip->audio.getSample (ch, i) * clip->getGain()));
        }

        check (shape && most < 1.0e-6f, "the WAV holds the clip's audio times its gain, at its rate and channels (" + juce::String (most, 8) + ")");

        s.engine.setClipGain (*clip, 12.0f);
        check (bounce::writeClipToWav (*clip, file, &bits).wasOk() && bits == 32, "a clip that peaks over 0 dB is written as 32-bit float");
    }
}

static void runRhythmTests()
{
    namespace rhythm = daw::gen::rhythm;
    std::cout << "Note lengths, beats and bars" << std::endl;

    juce::StringArray names;
    for (double beats : { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 })
        names.add (rhythm::nameLength (beats));
    check (names.joinIntoString ("|") == "whole note|half note|quarter note|eighth note|sixteenth note|thirty-second note",
           "plain note values are named: " + names.joinIntoString (", "));

    check (rhythm::nameLength (1.5) == "dotted quarter note" && rhythm::nameLength (0.75) == "dotted eighth note"
               && rhythm::nameLength (3.0) == "dotted half note",
           "dotted notes are half as long again");
    check (rhythm::nameLength (1.0 / 3.0) == "eighth-note triplet" && rhythm::nameLength (2.0 / 3.0) == "quarter-note triplet"
               && rhythm::nameLength (0.3333) == "eighth-note triplet",
           "triplets are two thirds of the plain note");
    check (rhythm::nameLength (1.75).isEmpty() && rhythm::nameLength (0.3).isEmpty(), "other lengths have no single name");

    check (rhythm::describeLength (1.0) == "quarter note, 1 beat" && rhythm::describeLength (0.75) == "dotted eighth note, 0.75 beats"
               && rhythm::describeLength (1.0 / 3.0) == "eighth-note triplet, 1/3 beat",
           "a length is described with its value and beats: " + rhythm::describeLength (0.75));
    check (rhythm::describeLength (1.75) == "1.75 beats (a dotted quarter tied to a sixteenth)"
               && rhythm::describeLength (1.25) == "1.25 beats (a quarter tied to a sixteenth)"
               && rhythm::describeLength (0.3) == "0.3 beats",
           "other lengths are split into tied notes: " + rhythm::describeLength (1.75));

    check (rhythm::formatPosition (0.0) == "1.1.1" && rhythm::formatPosition (18.25) == "5.3.2" && rhythm::formatPosition (3.75) == "1.4.4",
           "positions read bar.beat.sixteenth, like the position display");

    juce::StringArray counts;
    for (double beat : { 0.0, 0.25, 0.5, 0.75, 1.0, 3.0, 1.0 / 3.0, 2.0 / 3.0, 0.125 })
        counts.add (rhythm::countAt (beat));
    check (counts.joinIntoString (" ") == "1 e & a 2 4 trip let ", "counting: " + counts.joinIntoString (" "));

    bool topics = true;
    for (auto t : rhythm::allTopics)
        topics = topics && rhythm::getTopicName (t).isNotEmpty() && rhythm::getTopicSummary (t).isNotEmpty()
                 && rhythm::explain (t, 120.0).startsWith (rhythm::getTopicName (t));
    check (topics, "every topic has a name, a summary and an explanation");
    check (rhythm::explain (rhythm::Topic::tempo, 120.0).contains ("a beat (quarter note) 0.50 s")
               && rhythm::explain (rhythm::Topic::tempo, 120.0).contains ("a bar (whole note) lasts 2.00 s"),
           "the tempo topic works out the song's note lengths in seconds");
    check (rhythm::explain (rhythm::Topic::noteValues, 120.0).contains ("Snap: 1/16") && rhythm::explain (rhythm::Topic::readingDawIt, 120.0).contains ("5.3.2"),
           "the explanations tie note values to Snap and the position display");
}

static void runAudioToMidiTests()
{
    using namespace daw;
    namespace a2m = audiotomidi;
    std::cout << "Audio to MIDI" << std::endl;

    // A melody and a chord in a plucked tone with overtones (as in the Python reference check).
    struct Played { double start, length; int pitch; };
    const Played melody[] = { { 0.25, 0.45, 60 }, { 0.75, 0.45, 64 }, { 1.25, 0.45, 67 }, { 1.75, 0.9, 72 },
                              { 3.0, 0.45, 69 }, { 3.5, 0.45, 65 }, { 4.0, 0.95, 62 } };
    const Played chord[] = { { 5.25, 1.0, 55 }, { 5.25, 1.0, 59 }, { 5.25, 1.0, 62 } };

    auto render = [&] (double rate)
    {
        juce::AudioBuffer<float> b (2, (int) (rate * 7.0));
        b.clear();
        auto add = [&] (const Played& p)
        {
            const double f = 440.0 * std::pow (2.0, (p.pitch - 69) / 12.0);
            for (int i = 0; i < (int) (rate * p.length); ++i)
            {
                const double t = i / rate;
                double tone = 0.0;
                for (int h = 1; h <= 5; ++h)
                    tone += std::sin (juce::MathConstants<double>::twoPi * f * h * t) / h;
                const double env = juce::jmin (1.0, t / 0.005) * std::exp (-t * 2.5) * juce::jmin (1.0, (p.length - t) / 0.02);
                for (int ch = 0; ch < 2; ++ch)
                    b.addSample (ch, (int) (rate * p.start) + i, (float) (0.15 * tone * env));
            }
        };
        for (auto& p : melody) add (p);
        for (auto& p : chord) add (p);
        return b;
    };

    auto found = [] (const std::vector<a2m::Note>& notes, const Played& p)
    {
        return std::any_of (notes.begin(), notes.end(), [&] (const a2m::Note& n)
        {
            return n.pitch == p.pitch && std::abs (n.start - p.start) < 0.03 && std::abs (n.end - (p.start + p.length)) < 0.08;
        });
    };

    juce::String error;
    const auto input = a2m::prepareInput (render (a2m::sampleRate), a2m::sampleRate);
    const auto started = juce::Time::getMillisecondCounterHiRes();
    const auto activations = a2m::runModel (input, [] (double) { return true; }, error);
    const auto ms = juce::Time::getMillisecondCounterHiRes() - started;
    check (activations.has_value() && activations->numFrames == 602, "the model runs on 7 seconds in " + juce::String (juce::roundToInt (ms)) + " ms " + error);

    if (! activations.has_value())
        return;

    const auto notes = a2m::findNotes (*activations, {});
    check (std::all_of (std::begin (melody), std::end (melody), [&] (auto& p) { return found (notes, p); })
               && std::all_of (std::begin (chord), std::end (chord), [&] (auto& p) { return found (notes, p); }),
           "every played note is found, at its time (" + juce::String ((int) notes.size()) + " notes, with overtones)");

    const auto single = a2m::keepOneAtATime (notes);
    std::vector<int> pitches;
    for (auto& n : single)
        if (n.start < 5.0)
            pitches.push_back (n.pitch);
    check (pitches == std::vector<int> { 60, 64, 67, 72, 69, 65, 62 }, "one at a time keeps just the melody");

    juce::String error44;
    const auto stereo44 = a2m::prepareInput (render (44100.0), 44100.0);
    const auto activations44 = a2m::runModel (stereo44, [] (double) { return true; }, error44);
    const auto notes44 = activations44.has_value() ? a2m::findNotes (*activations44, {}) : std::vector<a2m::Note>();
    check (std::abs ((int) stereo44.size() - (int) input.size()) <= 1
               && std::all_of (std::begin (melody), std::end (melody), [&] (auto& p) { return found (notes44, p); }),
           "44.1 kHz stereo is brought down to 22.05 kHz mono and gives the same melody");

    int calls = 0;
    check (! a2m::runModel (std::vector<float> (22050 * 20, 0.0f), [&calls] (double) { return ++calls < 2; }, error).has_value() && calls == 2,
           "converting can be cancelled between windows");

    // Note detection rules, on made-up activations: a note at middle C from frame 10 to 39 with its
    // onset at frame 10, and a too-short note at C5.
    a2m::Activations made;
    made.numFrames = 100;
    made.notes.assign (100 * a2m::numPitches, 0.0f);
    made.onsets.assign (100 * a2m::numPitches, 0.0f);
    auto set = [&] (std::vector<float>& v, int frame, int pitch, float value) { v[(size_t) (frame * a2m::numPitches + pitch - a2m::lowestPitch)] = value; };
    for (int t = 10; t < 40; ++t) set (made.notes, t, 60, 0.8f);
    set (made.onsets, 10, 60, 0.9f);
    for (int t = 50; t < 61; ++t) set (made.notes, t, 72, 0.8f);
    set (made.onsets, 50, 72, 0.9f);

    const auto fromOnsets = a2m::findNotes (made, {});
    check (fromOnsets.size() == 1 && fromOnsets[0].pitch == 60 && juce::exactlyEqual (fromOnsets[0].start, a2m::frameToSeconds (10))
               && juce::exactlyEqual (fromOnsets[0].end, a2m::frameToSeconds (40)) && std::abs (fromOnsets[0].amplitude - 0.8f) < 1.0e-6f,
           "a note runs from its onset until its energy fades, and short ones are dropped");

    a2m::Settings noOnsets;
    noOnsets.onsetThreshold = 2.0f;
    const auto fromEnergy = a2m::findNotes (made, noOnsets);
    noOnsets.melodiaTrick = false;
    check (fromEnergy.size() == 1 && fromEnergy[0].pitch == 60 && juce::exactlyEqual (fromEnergy[0].start, a2m::frameToSeconds (10))
               && juce::exactlyEqual (fromEnergy[0].end, a2m::frameToSeconds (39)) && a2m::findNotes (made, noOnsets).empty(),
           "the melodia trick finds notes whose onset wasn't heard");
    check (std::abs (a2m::frameToSeconds (172) - (172 * 256 / 22050.0 - 0.010326)) < 1.0e-5, "frame times follow basic-pitch's window correction");

    const std::vector<a2m::Note> overlapping { { 0.0, 1.0, 60, 0.8f }, { 0.5, 2.0, 64, 0.5f }, { 0.2, 0.3, 72, 0.3f } };
    const auto kept = a2m::keepOneAtATime (overlapping);
    check (kept.size() == 2 && kept[0].pitch == 60 && kept[1].pitch == 64 && juce::exactlyEqual (kept[1].start, 1.0) && juce::exactlyEqual (kept[1].end, 2.0),
           "one at a time: the stronger note wins, a weaker one keeps the free part or goes");

    // At 120 BPM a second is two beats.
    const std::vector<a2m::Note> timed { { 0.26, 0.74, 60, 0.7f }, { 0.5, 1.0, 62, 0.0f }, { 0.75, 1.25, 62, 0.9f },
                                         { 3.5, 5.0, 65, 0.5f }, { 4.5, 5.0, 67, 0.5f } };
    const auto free = a2m::toClipNotes (timed, daw::TempoMap (120.0), 0.0, 8.0);
    check (free.size() == 4 && std::abs (free[0].start - 0.52) < 1.0e-9 && std::abs (free[0].length - 0.96) < 1.0e-9
               && std::abs (free[1].length - 0.5) < 1.0e-9 && juce::exactlyEqual (free[1].velocity, 0.05f)
               && std::abs (free[3].start - 7.0) < 1.0e-9 && std::abs (free[3].length - 1.0) < 1.0e-9,
           "notes become beats; a later note of the same pitch cuts the earlier one; the clip end cuts too");
    const auto quantised = a2m::toClipNotes (timed, daw::TempoMap (120.0), 0.0, 8.0, 0.25);
    check (juce::exactlyEqual (quantised[0].start, 0.5) && juce::exactlyEqual (quantised[0].length, 1.0), "quantising snaps starts and ends to the grid");

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* source = engine.addTrack (Track::Type::audio);
    source->name = "Bass";
    auto* after = engine.addTrack (Track::Type::audio);
    auto* audioClip = engine.addAudioClip (*source, juce::AudioBuffer<float> (1, 44100 * 4), 44100.0, 8.0, "Song bass");
    auto* midiClip = a2m::addMidiTrack (engine, *source, *audioClip, free);
    auto* midiTrack = engine.findTrackFor (midiClip);
    check (midiTrack != nullptr && engine.indexOf (midiTrack) == 1 && engine.indexOf (after) == 2 && midiTrack->name == "Bass MIDI"
               && midiTrack->builtInSound.load() == BuiltInSound::bass,
           "the notes go on a new MIDI track below, with the bass sound for a bass");
    check (juce::exactlyEqual (midiClip->startBeat, 8.0) && juce::exactlyEqual (midiClip->lengthBeats, 8.0) && midiClip->notes.size() == 4
               && midiClip->name == "Song bass",
           "the MIDI clip sits where the audio clip is and is as long");
}


//==============================================================================
namespace hittest
{
    /** Adds a decaying burst of `shape` (called with the time in seconds) at `at` seconds. */
    template <typename Shape>
    static void addBurst (juce::AudioBuffer<float>& b, double rate, double at, double length, float gain, Shape shape)
    {
        const int start = (int) std::lround (at * rate);

        for (int i = 0; i < (int) (length * rate) && start + i < b.getNumSamples(); ++i)
        {
            const double t = i / rate;
            b.getWritePointer (0)[start + i] += gain * (float) (std::exp (-t / (length / 5.0)) * shape (t));
        }
    }

    static auto clickShape()
    {
        return [state = std::make_shared<juce::Random> (7)] (double) { return (double) state->nextFloat() * 2.0 - 1.0; };
    }

    static juce::AudioBuffer<float> makeClicks (int count, double spacing, double rate, double seconds, float noise = 0.0f)
    {
        juce::AudioBuffer<float> b (1, (int) (seconds * rate));
        b.clear();
        juce::Random r (3);

        if (noise > 0.0f)
            for (int i = 0; i < b.getNumSamples(); ++i)
                b.getWritePointer (0)[i] = noise * (r.nextFloat() * 2.0f - 1.0f);

        for (int k = 0; k < count; ++k)
            addBurst (b, rate, k * spacing, 0.004, 0.8f, clickShape());

        return b;
    }

    static juce::AudioBuffer<float> makeTones (const std::vector<double>& hz, double rate, double length, float gain = 0.2f)
    {
        juce::AudioBuffer<float> b (1, (int) ((length + 0.5) * rate));
        b.clear();
        addBurst (b, rate, 0.2, length, gain, [&] (double t)
        {
            double v = 0.0;
            for (size_t i = 0; i < hz.size(); ++i)
                v += std::sin (juce::MathConstants<double>::twoPi * hz[i] * t + (double) i);
            return v;
        });
        return b;
    }
}

static void runHitFinderTests()
{
    using namespace daw;
    namespace hf = analysis::hits;
    std::cout << "Finding hits" << std::endl;

    const double rate = 44100.0;
    const auto clicks = hittest::makeClicks (8, 0.5, rate, 4.0);
    const auto found = hf::findCandidates (clicks, rate);
    bool timesRight = found.size() == 8;

    for (size_t i = 0; timesRight && i < found.size(); ++i)
        timesRight = std::abs (found[i].time - 0.5 * (double) i) < 0.0005;

    check (timesRight, "8 clicks 0.5 s apart are 8 candidates at their times to 0.5 ms (found " + juce::String ((int) found.size()) + ")");
    juce::AudioBuffer<float> quiet (1, 44100 * 2);
    quiet.clear();
    check (hf::findCandidates (quiet, rate).empty(), "silence has no candidates");

    int polls = 0;
    check (hf::findCandidates (clicks, rate, [&] { return ++polls > 2; }).empty() && polls > 2, "cancelling stops the search");
    double lastProgress = 0.0;
    hf::findCandidates (clicks, rate, {}, [&] (double p) { lastProgress = p; });
    check (juce::exactlyEqual (lastProgress, 1.0), "progress ends at 1");

    const auto noisy = hf::findCandidates (hittest::makeClicks (8, 0.5, rate, 4.0, 0.02f), rate);
    check (hf::pick (noisy, {}).size() == 8, "clicks with a 2% noise floor still give 8 hits (" + juce::String ((int) hf::pick (noisy, {}).size()) + ")");

    // A double hit 12 ms apart: one hit at a 30 ms gap, two at 5 ms.
    {
        juce::AudioBuffer<float> b (1, (int) (2.0 * rate));
        b.clear();
        hittest::addBurst (b, rate, 0.5, 0.004, 0.8f, hittest::clickShape());
        hittest::addBurst (b, rate, 0.512, 0.004, 0.8f, hittest::clickShape());
        const auto c = hf::findCandidates (b, rate);
        hf::PickSettings wide, narrow;
        wide.minGapMs = 30.0;
        narrow.minGapMs = 5.0;
        check (hf::pick (c, wide).size() == 1 && hf::pick (c, narrow).size() == 2,
               "a double hit 12 ms apart is 1 hit at a 30 ms gap and 2 at 5 ms (" + juce::String ((int) c.size()) + " candidates)");
    }

    // Velocity.
    check (hf::getVelocity (hf::VelocityCurve::natural, 0.0) == 127 && std::abs (hf::getVelocity (hf::VelocityCurve::natural, 12.0) - 64) <= 1
               && std::abs (hf::getVelocity (hf::VelocityCurve::natural, 20.0) - 40) <= 1,
           "Natural: 0, -12 and -20 dB give 127, 64 and 40");
    check (std::abs (hf::getVelocity (hf::VelocityCurve::gentle, 20.0) - 59) <= 1 && std::abs (hf::getVelocity (hf::VelocityCurve::strong, 20.0) - 13) <= 1
               && hf::getVelocity (hf::VelocityCurve::strong, 40.0) == 1 && hf::getVelocity (hf::VelocityCurve::same, 33.0) == 100,
           "Gentle and Strong at -20 dB give 59 and 13; Same for all gives 100");

    // Sorting by drum.
    {
        const auto kick = hf::findCandidates (hittest::makeTones ({ 60.0 }, rate, 0.15), rate);
        const auto hat = hf::findCandidates (hittest::makeTones ({ 7000.0, 8000.0, 9000.0, 10000.0 }, rate, 0.05), rate);
        const auto snare = hf::findCandidates (hittest::makeTones ({ 200.0, 1200.0, 1800.0, 2500.0, 3100.0, 3900.0 }, rate, 0.1), rate);
        check (! kick.empty() && hf::classify (kick.front()) == hf::Drum::kick, "a 60 Hz burst is a kick");
        check (! hat.empty() && hf::classify (hat.front()) == hf::Drum::hihat, "a burst of 7 to 10 kHz is a hi-hat");
        check (! snare.empty() && hf::classify (snare.front()) == hf::Drum::snare, "a 200 Hz tone with 1 to 4 kHz is a snare");
        hf::PickSettings single;
        single.singleDrum = hf::Drum::snare;
        check (! kick.empty() && hf::pick (kick, single).front().drum == hf::Drum::snare, "Write as one drum makes every hit that drum");
    }

    // Sensitivity on the snare track of the guide: 7, 4 and 2 hits at -30, -24 and -18 dB.
    {
        std::vector<hf::Candidate> track;
        auto add = [&] (double beat, double db) { hf::Candidate c; c.time = beat * 0.5; c.levelDb = (float) db; c.rise = 20.0f; c.mid = 1.0f; track.push_back (c); };
        add (0.0, -26); add (1.0, 0); add (1.75, -22); add (2.0, -26); add (2.5, -26); add (3.0, 0); add (3.5, -22);
        std::vector<int> counts;

        for (double s : { -30.0, -24.0, -18.0 })
        {
            hf::PickSettings p;
            p.sensitivityDb = s;
            counts.push_back ((int) hf::pick (track, p).size());
        }

        check (counts == std::vector<int> { 7, 4, 2 }, "Sensitivity -30, -24 and -18 dB keep 7, 4 and 2 hits");

        // Hand edits survive a re-pick: remove, move, add and set the drum.
        hf::PickSettings p;
        std::vector<hf::HandEdit> edits;
        hf::HandEdit remove; remove.kind = hf::HandEdit::Kind::remove; remove.time = 0.0 + 0.004;
        hf::HandEdit move; move.kind = hf::HandEdit::Kind::move; move.time = 0.5; move.target = track[1]; move.target.time = 0.52;
        hf::HandEdit add2; add2.kind = hf::HandEdit::Kind::add; add2.target = track[1]; add2.target.time = 1.9; add2.target.levelDb = -10.0f;
        hf::HandEdit drum; drum.kind = hf::HandEdit::Kind::setDrum; drum.time = 1.5; drum.drum = hf::Drum::kick;
        edits = { remove, move, add2, drum };

        for (double s : { -30.0, -18.0 })
        {
            p.sensitivityDb = s;
            const auto hits = hf::pick (track, p, edits);
            bool moved = false, added = false, removed = true, drumSet = false;

            for (const auto& h : hits)
            {
                moved = moved || std::abs (h.time - 0.52) < 1.0e-9;
                added = added || std::abs (h.time - 1.9) < 1.0e-9;
                removed = removed && std::abs (h.time) > 0.001;
                drumSet = drumSet || (std::abs (h.time - 1.5) < 1.0e-9 && h.drum == hf::Drum::kick && h.drumFixed);
            }

            check (moved && added && removed && drumSet, "hand edits (move, add, remove, drum) survive a re-pick at " + juce::String (s) + " dB");
        }
    }
}

static void runHitsToMidiTests()
{
    using namespace daw;
    namespace hf = analysis::hits;
    namespace h2m = hitstomidi;
    std::cout << "Hits to MIDI" << std::endl;

    auto hitAt = [] (double seconds, hf::Drum d = hf::Drum::snare, int v = 100) { hf::Hit h; h.time = seconds; h.drum = d; h.velocity = v; return h; };

    // Clicks at 120 BPM, the clip at beat 8: notes at clip beats 0 to 7.
    {
        std::vector<hf::Hit> hits;
        for (int i = 0; i < 8; ++i)
            hits.push_back (hitAt (0.5 * i));

        const auto notes = h2m::toClipNotes (hits, TempoMap (120.0), 8.0, 8.0);
        bool right = notes.size() == 8;
        for (size_t i = 0; right && i < notes.size(); ++i)
            right = std::abs (notes[i].start - (double) i) < 0.002 && notes[i].pitch == 38 && std::abs (notes[i].length - 0.25) < 1.0e-9;

        check (right, "clicks at 120 BPM become notes at clip beats 0 to 7 (beats 8 to 15 in the song), a sixteenth long");
        check (std::abs (0.030 / 0.5 - 0.06) < 1.0e-9, "30 ms at 120 BPM is 0.06 beat");
        check (h2m::toClipNotes ({ hitAt (4.2) }, TempoMap (120.0), 0.0, 8.0).empty(), "a note past the clip's end is dropped");
        const auto close = h2m::toClipNotes ({ hitAt (0.0), hitAt (0.05), hitAt (0.0, hf::Drum::kick, 90) }, TempoMap (120.0), 0.0, 8.0);
        check (close.size() == 3 && std::abs (close[1].length - 0.1) < 1.0e-9 && close[1].pitch == 38 && close[0].pitch == 36 && std::abs (close[2].length - 0.25) < 1.0e-9,
               "a note ends before the next one of the same pitch");
    }

    // Snap: a hit at beat 4.06 on a 1/16 grid at 0, 50 and 100%; the grid is the song's, not the clip's.
    for (const auto& [strength, expected] : { std::pair { 0.0, 4.06 }, std::pair { 0.5, 4.03 }, std::pair { 1.0, 4.0 } })
    {
        h2m::Timing timing;
        timing.snap = true;
        timing.strength = strength;
        const auto notes = h2m::toClipNotes ({ hitAt (4.06 * 0.5 - 1.1 * 0.5) }, TempoMap (120.0), 1.1, 16.0, timing);
        check (notes.size() == 1 && std::abs (notes[0].start + 1.1 - expected) < 1.0e-9,
               "snap at " + juce::String (strength * 100.0) + "% puts 4.06 at " + juce::String (expected, 2) + " (song grid, clip at 1.1)");
    }

    // The example loop through the finder.
    {
        const double rate = 44100.0;
        const auto loop = h2m::renderExampleLoop (rate);
        const auto candidates = hf::findCandidates (loop, rate);
        const auto hits = hf::pick (candidates, {});
        int counts[3] = { 0, 0, 0 };

        for (const auto& h : hits)
            ++counts[(int) h.drum];

        check (hits.size() == 18 && counts[0] == 5 && counts[1] == 5 && counts[2] == 8,
               "the example loop gives 18 hits: 5 kick, 5 snare, 8 hi-hat (" + juce::String ((int) hits.size()) + ": "
                   + juce::String (counts[0]) + ", " + juce::String (counts[1]) + ", " + juce::String (counts[2]) + ")");

        // Each hit within 2 ms of its beat time (plus its played-late time).
        const auto& example = h2m::getExampleLoop();
        int matched = 0, velocityOk = 0;

        for (const auto& e : example)
        {
            if (e.beat > 4.0 && e.beat < 4.1)
                continue;   // the bounce is merged into the beat-4 kick

            const double t = e.beat * 0.5 + e.lateMs / 1000.0;

            for (const auto& h : hits)
                if (std::abs (h.time - t) < 0.002 && h.drum == e.drum)
                {
                    ++matched;
                    const int expectedVelocity = e.drum == hf::Drum::kick ? 127 : e.drum == hf::Drum::snare ? (e.levelDb < -10.0 ? 36 : 127)
                                                                                                             : (e.levelDb > -15.0 ? 127 : 113);
                    velocityOk += std::abs (h.velocity - expectedVelocity) <= (e.drum == hf::Drum::kick || e.levelDb < -20.0 ? 3 : 10) ? 1 : 0;
                }
        }

        check (matched == 18, "each example hit is within 2 ms of its time and drum (" + juce::String (matched) + " of 18)");
        check (velocityOk == 18, "Natural velocities of the example: kicks 127, snares about 127, ghost 36, hats about 127 and 113 (" + juce::String (velocityOk) + " of 18)");

        // Every sensitivity row of the loop: a lower value finds the ghost note too.
        hf::PickSettings strict;
        strict.sensitivityDb = -18.0;
        check (hf::pick (candidates, strict).size() < 18, "a higher sensitivity finds fewer hits on the example loop");
    }

    // Listen: the clip, the hits, or both.
    {
        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* source = engine.addTrack (Track::Type::audio);
        source->name = "Drum loop";
        auto* after = engine.addTrack (Track::Type::audio);
        auto* clip = engine.addAudioClip (*source, h2m::renderExampleLoop (44100.0), 44100.0, 8.0, "Loop");
        const auto hits = hf::pick (hf::findCandidates (clip->audio, 44100.0), {});
        const auto notes = h2m::toClipNotes (hits, engine.getTempoMap(), 8.0, clip->getLengthBeats (engine.getTempoMap()));
        const auto original = h2m::renderListen (*clip, notes, engine.getTempoMap(), 8.0, 44100.0, h2m::Hear::original);
        const auto onlyHits = h2m::renderListen (*clip, notes, engine.getTempoMap(), 8.0, 44100.0, h2m::Hear::hits);
        const auto both = h2m::renderListen (*clip, notes, engine.getTempoMap(), 8.0, 44100.0, h2m::Hear::both);
        check (original.getNumSamples() == clip->audio.getNumSamples() && original.getNumChannels() == 2
                   && juce::exactlyEqual (original.getSample (1, 20000), clip->audio.getSample (0, 20000))
                   && onlyHits.getMagnitude (0, onlyHits.getNumSamples()) > 0.05f
                   && both.getMagnitude (0, both.getNumSamples()) > original.getMagnitude (0, original.getNumSamples()) * 0.9f,
               "Listen renders the clip, the hits or both, as long as the clip");

        // Making the track.
        source->plugins.clear();
        const auto before = engine.getTracks().size();
        check (h2m::addDrumTrack (engine, *source, *clip, {}, false) == nullptr && engine.getTracks().size() == before,
               "no hits make no track");

        auto* made = h2m::addDrumTrack (engine, *source, *clip, notes, true);
        auto* track = made != nullptr ? engine.findTrackFor (made) : nullptr;
        const auto* machine = track != nullptr && ! track->plugins.empty() ? dynamic_cast<BuiltInDrums*> (track->plugins[0]->instance.get()) : nullptr;
        check (track != nullptr && engine.indexOf (track) == 1 && engine.indexOf (after) == 2 && track->name == "Drum loop hits" && machine != nullptr
                   && juce::exactlyEqual (made->startBeat, 8.0) && made->notes.size() == 18 && clip->muted && source->plugins.empty()
                   && source->builtInSound.load() == BuiltInSound::piano,
               "the notes go in a track below the source with DawIt Drums; the clip is muted on replace; the source is untouched");

        auto* second = h2m::addDrumTrack (engine, *source, *clip, notes, false);
        check (second != nullptr, "layering makes a track too");
        engine.setClipMuted (*clip, false);
        engine.removeTrack (engine.findTrackFor (second));

        // Save and load.
        const auto folder = juce::File::createTempFile ("hits");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
        engine.setClipMuted (*clip, true);
        const auto projectFile = folder.getChildFile ("hits.dawit");
        juce::String text, error;
        juce::StringArray warnings;
        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        const bool saved = project::save (engine, projectFile, text, error);
        const bool ok = saved && project::load (loaded, projectFile, error, warnings);
        const auto& lt = loaded.getTracks();
        bool same = ok && lt.size() == 3 && lt[1]->name == "Drum loop hits" && ! lt[1]->plugins.empty()
                    && dynamic_cast<BuiltInDrums*> (lt[1]->plugins[0]->instance.get()) != nullptr && ! lt[1]->clips.empty();

        if (same)
        {
            const auto& saved1 = static_cast<MidiClip&> (*lt[1]->clips[0]);
            same = saved1.notes.size() == 18 && lt[0]->clips[0]->muted;

            for (size_t i = 0; same && i < saved1.notes.size(); ++i)
                same = saved1.notes[i].pitch == made->notes[i].pitch && std::abs (saved1.notes[i].start - made->notes[i].start) < 1.0e-6
                       && std::abs (saved1.notes[i].velocity - made->notes[i].velocity) < 0.01f;
        }

        check (same && project::toText (loaded, projectFile) == text, "the made track saves and loads unchanged " + error);
    }
}

//==============================================================================
/** A band playing Am F C G, a bar each: kick on 1 and 3, snare on 2 and 4, hi-hat on the eighths,
    a chord and a bass note per bar. The tempo goes from `bpm` to `bpm + drift` over the length. */
static juce::AudioBuffer<float> makeBand (double bpm, double firstBeat, double seconds, double rate,
                                          std::vector<double>& beats, double drift = 0.0)
{
    juce::AudioBuffer<float> audio (1, (int) (seconds * rate));
    audio.clear();
    juce::Random random (42);

    for (double t = firstBeat; t < seconds - 0.5;)
    {
        beats.push_back (t);
        t += 60.0 / (bpm + drift * t / seconds);
    }

    auto* out = audio.getWritePointer (0);
    const int length = audio.getNumSamples();

    auto add = [&] (double at, double duration, const std::function<float (double)>& sound)
    {
        for (int i = (int) (at * rate); i < juce::jmin (length, (int) ((at + duration) * rate)); ++i)
            out[i] += sound (i / rate - at);
    };

    const double chords[4][4] = { { 45, 57, 60, 64 }, { 41, 57, 60, 65 }, { 48, 60, 64, 67 }, { 43, 59, 62, 67 } };

    for (size_t k = 0; k + 1 < beats.size(); ++k)
    {
        const double at = beats[k], length2 = beats[k + 1] - beats[k];
        const int inBar = (int) (k % 4);

        if (inBar == 0 || inBar == 2)
            add (at, 0.2, [] (double t) { return (float) (0.8 * std::exp (-t * 25.0) * std::sin (juce::MathConstants<double>::twoPi * (50.0 * t + 70.0 * (1.0 - std::exp (-t * 30.0)) / 30.0))); });
        else
            add (at, 0.15, [&random] (double t) { return (float) (0.4 * std::exp (-t * 30.0) * (random.nextFloat() * 2.0 - 1.0)); });

        for (double hat : { at, at + length2 / 2.0 })
            add (hat, 0.04, [&random] (double t) { return (float) (0.12 * std::exp (-t * 120.0) * (random.nextFloat() * 2.0 - 1.0)); });

        if (inBar == 0)
        {
            const auto& chord = chords[(k / 4) % 4];
            const double barLength = length2 * 4.0;

            add (at, barLength, [&chord] (double t)
            {
                double v = 0.0;

                for (int n = 0; n < 4; ++n)
                    v += (n == 0 ? 0.12 : 0.05) * std::sin (juce::MathConstants<double>::twoPi * 440.0 * std::pow (2.0, (chord[n] - 69.0) / 12.0) * t);

                return (float) (v * juce::jmin (1.0, t * 50.0));
            });
        }
    }

    return audio;
}

static void runTempoFinderTests()
{
    using namespace daw;
    namespace tf = analysis::tempofinder;
    std::cout << "Detecting the tempo" << std::endl;

    check (juce::exactlyEqual (tf::roundBpm (119.97), 120.0) && juce::exactlyEqual (tf::roundBpm (119.93), 119.93)
               && juce::exactlyEqual (tf::roundBpm (97.4649), 97.46),
           "a tempo is rounded to a whole BPM when it's within 0.05, otherwise to 0.01");
    check (daw::formatBpm (120.0) == "120" && daw::formatBpm (97.5) == "97.5" && daw::formatBpm (96.84) == "96.84"
               && daw::formatBpm (96.8449) == "96.84" && daw::formatBpm (99.999) == "100",
           "a tempo is written with up to two decimals and no trailing zeros");
    check (daw::parseBpm ("97.5") == 97.5 && daw::parseBpm ("97,5") == 97.5 && daw::parseBpm (" 120 BPM") == 120.0
               && ! daw::parseBpm ("fast").has_value() && ! daw::parseBpm ("9.7.5").has_value() && ! daw::parseBpm ("").has_value(),
           "a typed tempo takes a decimal point or comma");

    {
        // A band recorded without a click at an odd tempo: found to the hundredth, so it stays on the bars.
        std::vector<double> beats;
        const auto band = makeBand (97.37, 0.6, 30.0, 44100.0, beats);
        const auto found = tf::analyse (tf::prepareInput (band, 44100.0));
        check (found.has_value() && std::abs (tf::roundBpm (found->bpm) - 97.37) < 0.011,
               "a tempo of 97.37 BPM is found to the hundredth (found " + (found.has_value() ? juce::String (found->bpm, 3) : juce::String ("none")) + ")");
    }

    for (const auto& [bpm, firstBeat] : { std::pair { 120.0, 1.3 }, std::pair { 97.0, 0.6 }, std::pair { 140.0, 2.0 }, std::pair { 84.0, 0.25 } })
    {
        std::vector<double> beats;
        const auto band = makeBand (bpm, firstBeat, 30.0, 44100.0, beats);
        const auto found = tf::analyse (tf::prepareInput (band, 44100.0));
        check (found.has_value(), "a beat is found at " + juce::String (bpm) + " BPM");

        if (! found.has_value())
            continue;

        // Each beat found against the nearest real one.
        double worst = 0.0, offset = 0.0;

        for (double b : found->beats)
        {
            const double nearest = *std::min_element (beats.begin(), beats.end(), [b] (double x, double y) { return std::abs (x - b) < std::abs (y - b); });
            worst = juce::jmax (worst, std::abs (b - nearest));
            offset += (b - nearest) / (double) found->beats.size();
        }

        check (std::abs (found->bpm - bpm) < 0.1 && juce::exactlyEqual (tf::roundBpm (found->bpm), bpm) && found->steady,
               "the tempo is " + juce::String (bpm) + " BPM, steady (found " + juce::String (found->bpm, 2) + ")");
        check (worst < 0.025 && found->beats.size() + 4 >= beats.size(),
               "the beats land on the band's beats (" + juce::String (found->beats.size()) + " of " + juce::String (beats.size())
                   + ", worst " + juce::String (worst * 1000.0, 1) + " ms, on average " + juce::String (offset * 1000.0, 1) + " ms)");
        check (std::abs (found->getFirstDownbeatSeconds() - firstBeat) < 0.025,
               "the first bar starts on the band's first downbeat (" + juce::String (found->getFirstDownbeatSeconds(), 3) + " s)");
    }

    {
        std::vector<double> beats;
        const auto band = makeBand (100.0, 0.5, 40.0, 44100.0, beats, 10.0);
        const auto found = tf::analyse (tf::prepareInput (band, 44100.0));
        check (found.has_value() && ! found->steady && found->slowest < 102.5 && found->fastest > 107.5,
               "a band speeding up from 100 to 110 BPM is found to drift ("
                   + (found.has_value() ? juce::String (found->slowest, 1) + " to " + juce::String (found->fastest, 1) : juce::String ("none")) + ")");
    }

    {
        juce::AudioBuffer<float> silence (1, (int) (tf::sampleRate * 10.0)), pad (1, (int) (tf::sampleRate * 10.0));
        silence.clear();

        for (int i = 0; i < pad.getNumSamples(); ++i)
            pad.setSample (0, i, (float) (0.2 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / tf::sampleRate)
                                          + 0.2 * std::sin (juce::MathConstants<double>::twoPi * 277.2 * i / tf::sampleRate)));

        check (! tf::analyse (silence).has_value() && ! tf::analyse (pad).has_value(), "silence and a held chord have no tempo");
    }
}

//==============================================================================
static void runTempoMapTests()
{
    using daw::TempoMap;
    namespace tf = daw::analysis::tempofinder;
    std::cout << "Tempo map" << std::endl;

    const auto near = [] (double a, double b, double tolerance = 1.0e-9) { return std::abs (a - b) < tolerance; };

    // Tidying.
    {
        const TempoMap tidied ({ { 16.0, 90.0 }, { 4.0, 120.0 }, { 8.0, 120.0 }, { 20.0, 500.0 } });
        const auto& c = tidied.getChanges();
        check (c.size() == 3 && near (c[0].beat, 0.0) && near (c[0].bpm, 120.0) && near (c[1].beat, 16.0) && near (c[1].bpm, 90.0)
                   && near (c[2].bpm, 300.0) && ! tidied.isSteady(),
               "tempo changes are sorted, the first starts the song, a repeat is dropped and 500 BPM is kept to 300");
        check (TempoMap (std::vector<daw::TempoChange> {}).isSteady() && near (TempoMap (std::vector<daw::TempoChange> {}).getBpmAt (10.0), 120.0)
                   && TempoMap ({ { 0.0, 100.0 }, { 8.0, 100.0 } }).isSteady() && TempoMap (97.5) == TempoMap ({ { 0.0, 97.5 } }),
               "no changes is 120 BPM, and the same tempo again is one tempo");
    }

    // 120 BPM for 8 beats (4 s), then 60 BPM (a beat a second).
    const TempoMap map ({ { 0.0, 120.0 }, { 8.0, 60.0 } });
    check (near (map.beatToSeconds (8.0), 4.0) && near (map.beatToSeconds (10.0), 6.0) && near (map.beatToSeconds (-2.0), -1.0)
               && near (map.secondsToBeat (6.0), 10.0) && near (map.secondsToBeat (-1.0), -2.0),
           "beats to seconds and back across a change, and before the song at the first tempo");
    check (near (map.getSecondsBetween (6.0, 10.0), 3.0) && near (map.getBeatAfter (6.0, 3.0), 10.0),
           "from beat 6 to 10 takes 1 s at 120 and 2 s at 60 BPM");
    check (near (map.getBpmAt (7.99), 120.0) && near (map.getBpmAt (8.0), 60.0) && map.getNextChangeAfter (0.0) == 8.0
               && ! map.getNextChangeAfter (8.0).has_value(),
           "the tempo at a beat, and the next change");
    check (near (map.getRange (0.0, 16.0).getStart(), 60.0) && near (map.getRange (0.0, 16.0).getEnd(), 120.0)
               && near (map.getRange (9.0, 16.0).getStart(), 60.0) && near (map.getAverageBpm (0.0, 16.0), 80.0),
           "slowest, fastest and average: 16 beats in 12 s is 80 BPM");

    bool roundTrips = true;
    for (double b = -3.0; b < 20.0; b += 0.37)
        roundTrips = roundTrips && near (map.secondsToBeat (map.beatToSeconds (b)), b, 1.0e-9);
    check (roundTrips, "every beat comes back from its seconds");

    // An audio clip over the change: 1 s at 120 BPM (2 beats), then 5 s at 60 (5 beats).
    {
        daw::AudioClip clip;
        clip.sourceSampleRate = 48000.0;
        clip.audio.setSize (1, 6 * 48000);
        clip.startBeat = 6.0;
        check (near (clip.getLengthBeats (map), 7.0, 1.0e-6) && near (clip.getEndBeat (map), 13.0, 1.0e-6),
               "a 6-second clip from beat 6 lasts 7 beats: 2 before the change and 5 after");
    }

    // Playback through a change: 2 s at 120 BPM (4 beats), then 60 BPM.
    {
        daw::Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        FakeDevice device;
        engine.audioDeviceAboutToStart (&device);
        engine.setTempoMap (TempoMap ({ { 0.0, 120.0 }, { 4.0, 60.0 } }));

        // A recording with a click 3 s in: heard 3 s in, at beat 5.
        auto* audioTrack = engine.addTrack (daw::Track::Type::audio);
        juce::AudioBuffer<float> audio (1, (int) (4.0 * testRate));
        audio.clear();
        audio.setSample (0, (int) (3.0 * testRate), 1.0f);
        engine.addAudioClip (*audioTrack, audio, testRate, 0.0, "Click");

        // A note on beat 6: 2 s at 120, then 2 beats at 60 BPM, so 4 s in.
        auto* midiTrack = engine.addTrack (daw::Track::Type::midi);
        engine.addMidiClip (*midiTrack, 0.0, 8.0)->notes.push_back ({ 72, 6.0, 1.0, 0.9f });

        engine.setPositionBeats (0.0);
        engine.play();
        const auto out = render (engine, 4.5);
        engine.stop();

        int click = -1, note = -1;
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const float v = std::abs (out.getSample (0, i));

            if (click < 0 && v > 0.3f && i < (int) (3.5 * testRate))
                click = i;

            if (note < 0 && v > 0.01f && i > (int) (3.5 * testRate))
                note = i;
        }

        check (std::abs (click - (int) (3.0 * testRate)) <= 1,
               "a recording plays straight through a tempo change: its click 3 s in is heard at " + juce::String (click / testRate, 4) + " s");
        check (note > 0 && std::abs (note - (int) (4.0 * testRate)) < (int) (0.005 * testRate),
               "a note on beat 6 starts 4 s in, after 2 s at 120 BPM and 2 beats at 60 (" + juce::String (note / testRate, 4) + " s)");
        check (near (engine.getPositionBeats(), 4.0 + (out.getNumSamples() / testRate - 2.0), 1.0e-6),
               "the playhead moves a beat a second after the change (beat " + juce::String (engine.getPositionBeats(), 3) + ")");
        engine.setPositionBeats (5.0);
        check (near (engine.getTempo(), 60.0) && near (engine.getHeardPositionBeats(), 5.0), "the tempo at the playhead, and what's heard while stopped");
    }

    // Following a band that speeds up from 100 to 110 BPM over 40 s.
    {
        std::vector<double> beats;
        const auto band = makeBand (100.0, 0.5, 40.0, 44100.0, beats, 10.0);
        const auto found = tf::analyse (tf::prepareInput (band, 44100.0));
        check (found.has_value() && ! found->steady, "the drifting band is found to drift");

        if (found.has_value())
        {
            const auto followed = tf::followRecording (*found, 1.0);

            // How far each of the band's downbeats lands from a bar line, in ms.
            auto worstMs = [&beats] (const TempoMap& tempo, double clipStart)
            {
                double worst = 0.0;

                for (size_t k = 0; k + 4 < beats.size(); k += 4)
                {
                    const double beat = tempo.getBeatAfter (clipStart, beats[k]);
                    const double offBeats = std::abs (beat - std::round (beat / 4.0) * 4.0);
                    worst = juce::jmax (worst, offBeats * 60000.0 / tempo.getBpmAt (beat));
                }

                return worst;
            };

            const double lead = found->getFirstDownbeatSeconds() * found->bpm / 60.0;
            const double steadyStart = std::round ((1.0 + lead) / 4.0) * 4.0 - lead;
            const double followedWorst = worstMs (followed.map, followed.clipStartBeat), steadyWorst = worstMs (TempoMap (found->bpm), steadyStart);

            check (! followed.map.isSteady() && followed.bars >= 15 && followed.range.getStart() < 102.5 && followed.range.getEnd() > 107.5,
                   "following it gives every bar its tempo: " + juce::String (followed.bars) + " bars from "
                       + juce::String (followed.range.getStart(), 1) + " to " + juce::String (followed.range.getEnd(), 1) + " BPM");
            check (followedWorst < 25.0 && steadyWorst > 150.0,
                   "the band's downbeats land on the bar lines all the way through (worst " + juce::String (followedWorst, 1)
                       + " ms), where one tempo drifts " + juce::String (juce::roundToInt (steadyWorst)) + " ms off");
            const double firstDownbeat = followed.map.getBeatAfter (followed.clipStartBeat, found->getFirstDownbeatSeconds());
            check (followed.clipStartBeat >= 0.0 && near (firstDownbeat, followed.firstBar * 4.0, 1.0e-6),
                   "the clip moves so its first downbeat starts bar " + juce::String (followed.firstBar + 1));
        }

        tf::Tempo tooShort;
        tooShort.bpm = 97.0;
        tooShort.beats = { 0.5, 1.1, 1.7 };
        check (tf::followRecording (tooShort, 0.0).map == TempoMap (97.0), "with less than a bar to measure, it's the tempo found");
    }

    // A MIDI file: a tempo event at each change; a clip's file starts at the tempo where the clip is.
    {
        daw::Track track (daw::Track::Type::midi, "Keys", juce::Colours::blue);
        auto clip = std::make_unique<daw::MidiClip>();
        clip->startBeat = 4.0;
        clip->lengthBeats = 8.0;
        clip->notes.push_back ({ 60, 0.0, 1.0, 0.8f });
        const auto& midiClip = *clip;
        track.clips.push_back (std::move (clip));

        const daw::midiexport::SongInfo song { map, {} };
        juce::MidiMessageSequence songTempos, clipTempos;
        daw::midiexport::createTrackFile (track, song).findAllTempoEvents (songTempos);
        daw::midiexport::createClipFile (midiClip, "Keys", song).findAllTempoEvents (clipTempos);

        auto describe = [] (const juce::MidiMessageSequence& events)
        {
            juce::StringArray text;
            for (auto* e : events)
                text.add (juce::String (juce::roundToInt (60.0 / e->message.getTempoSecondsPerQuarterNote())) + " @" + juce::String ((int) e->message.getTimeStamp()));
            return text.joinIntoString (", ");
        };

        check (describe (songTempos) == "120 @0, 60 @7680" && describe (clipTempos) == "120 @0, 60 @3840",
               "MIDI files get a tempo event per change: " + describe (songTempos) + "; from the clip at beat 5: " + describe (clipTempos));
    }

    // Saved with the project and loaded back.
    {
        const auto folder = juce::File::createTempFile ("tempo");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        daw::Engine engine, loaded, old;
        for (auto* e : { &engine, &loaded, &old })
            e->deviceManager.removeAudioCallback (e);

        const TempoMap band ({ { 0.0, 96.84 }, { 16.0, 101.373 }, { 20.0, 99.5 } });
        engine.setTempoMap (band);
        juce::String text, error;
        juce::StringArray warnings;
        const auto file = folder.getChildFile ("Band.dawit");
        const bool ok = daw::project::save (engine, file, text, error) && daw::project::load (loaded, file, error, warnings);
        const auto& changes = loaded.getTempoMap().getChanges();
        check (ok && text.contains ("\"tempoChanges\"") && changes.size() == 3 && near (changes[1].beat, 16.0) && near (changes[1].bpm, 101.373, 1.0e-6)
                   && near (changes[2].bpm, 99.5, 1.0e-6),
               "a project saves its tempo changes and loads them back " + error);

        engine.setTempo (120.0);
        const bool steadyOk = daw::project::save (engine, file, text, error) && daw::project::load (old, file, error, warnings);
        check (steadyOk && ! text.contains ("tempoChanges") && old.getTempoMap() == TempoMap (120.0),
               "a song with one tempo saves only \"tempo\", as before");
    }
}

static void runChordFinderTests()
{
    using namespace daw;
    namespace cf = analysis::chordfinder;
    std::cout << "Detecting the chords" << std::endl;

    // The band from the tempo tests, Am F C G with drums, at 100 BPM from the first beat.
    {
        std::vector<double> beats;
        const auto band = makeBand (100.0, 0.0, 24.0, 44100.0, beats);
        const auto found = cf::analyse (cf::prepareInput (band, 44100.0), 0.0, 100.0);
        const auto text = cf::toText (found.bars);
        check (text.startsWith ("Am | F | C | G\nAm | F | C | G"), "a band's chords are found bar by bar, through the drums:\n" + text);
        check (found.key.tonic == 9 && found.key.scale == MusicalKey::Scale::minor,
               "Am F C G starting on Am is in A minor (" + MusicalKey::getTonicName (found.key.tonic) + " " + MusicalKey::getScaleName (found.key.scale) + ")");
    }

    // Chords played with overtones (as an instrument has) and a bass on the root, a bar each at 120 BPM.
    {
        const juce::StringArray names { "C", "G", "Am", "F", "Dm7", "G7", "Cmaj7", "Em" };
        const double rate = 22050.0, bar = 2.0;
        juce::AudioBuffer<float> audio (1, (int) (rate * bar * (names.size() + 1)));
        audio.clear();

        for (int b = 0; b < names.size(); ++b)
        {
            const auto chord = *Chord::fromName (names[b]);

            for (int i = 0; i < (int) (rate * bar); ++i)
            {
                const double t = i / rate;
                double v = 0.0;

                for (int pc : chord.getPitchClasses())
                {
                    const double hz = 440.0 * std::pow (2.0, (60 + pc - 69) / 12.0);   // around middle C
                    for (int h = 1; h <= 4; ++h)
                        v += 0.06 / h * std::sin (juce::MathConstants<double>::twoPi * hz * h * t);
                }

                v += 0.15 * std::sin (juce::MathConstants<double>::twoPi * 440.0 * std::pow (2.0, (36 + chord.root - 69) / 12.0) * t);
                audio.setSample (0, b * (int) (rate * bar) + i, (float) (v * juce::jmin (1.0, t * 40.0, (bar - t) * 40.0)));
            }
        }

        const auto found = cf::analyse (cf::prepareInput (audio, rate), 0.0, 120.0);
        const auto text = cf::toText (found.bars);
        check (text == "C | G | Am | F\nDm7 | G7 | Cmaj7 | Em\n-", "triads and sevenths are named, and the silent bar has none:\n" + text);
        check (found.key.tonic == 0 && found.key.scale == MusicalKey::Scale::major, "C G Am F starting on C is in C major");
    }

    const auto& vocabulary = cf::getVocabulary();
    check (vocabulary.size() == 60, "it names major, minor, 7, maj7 and m7 chords on every root");

    // A C major triad's notes fit C best; with an E in the bass, C still wins over Em, which has no C.
    cf::PitchClasses notes {}, bass {};
    notes[0] = notes[4] = notes[7] = 1.0f;
    bass[0] = 1.0f;
    check (cf::scoreChord (*Chord::fromName ("C"), notes, bass) > cf::scoreChord (*Chord::fromName ("Cmaj7"), notes, bass)
               && cf::scoreChord (*Chord::fromName ("C"), notes, bass) > cf::scoreChord (*Chord::fromName ("Am"), notes, bass) + 0.2,
           "a triad's notes fit the triad better than its seventh chord or a chord that shares two notes");
}

//==============================================================================
/** A song made of parts, each with its own chord loop (a chord a bar), loudness, brightness (how
    many overtones) and drums or not, at 120 BPM: two seconds a bar. */
struct SongPart
{
    int bars;
    juce::StringArray chords;
    double level;
    int overtones;
    bool drums;
};

static juce::AudioBuffer<float> makeSong (const std::vector<SongPart>& parts, double rate)
{
    using namespace daw;
    const double bar = 2.0, beat = bar / 4.0;
    int totalBars = 0;

    for (const auto& p : parts)
        totalBars += p.bars;

    juce::AudioBuffer<float> audio (1, (int) (rate * bar * totalBars));
    audio.clear();
    auto* out = audio.getWritePointer (0);
    juce::Random random (3);
    int barIndex = 0;

    for (const auto& p : parts)
    {
        for (int b = 0; b < p.bars; ++b, ++barIndex)
        {
            const auto chord = *Chord::fromName (p.chords[b % p.chords.size()]);
            const int offset = (int) (barIndex * bar * rate);

            for (int i = 0; i < (int) (bar * rate); ++i)
            {
                const double t = i / rate;
                double v = 0.0;

                for (int pc : chord.getPitchClasses())
                {
                    const double hz = 440.0 * std::pow (2.0, (60 + pc - 69) / 12.0);

                    for (int h = 1; h <= p.overtones; ++h)
                        v += 0.05 / h * std::sin (juce::MathConstants<double>::twoPi * hz * h * t);
                }

                v += 0.12 * std::sin (juce::MathConstants<double>::twoPi * 440.0 * std::pow (2.0, (36 + chord.root - 69) / 12.0) * t);

                if (p.drums)
                {
                    const int beatInBar = (int) (t / beat);
                    const double sinceBeat = t - beatInBar * beat, sinceEighth = std::fmod (t, beat / 2.0);

                    if (beatInBar % 2 == 0)
                        v += 0.6 * std::exp (-sinceBeat * 25.0) * std::sin (juce::MathConstants<double>::twoPi * 55.0 * sinceBeat);
                    else
                        v += 0.3 * std::exp (-sinceBeat * 30.0) * (random.nextFloat() * 2.0 - 1.0);

                    v += 0.1 * std::exp (-sinceEighth * 120.0) * (random.nextFloat() * 2.0 - 1.0);
                }

                out[offset + i] = (float) (p.level * v * juce::jmin (1.0, t * 40.0, (bar - t) * 40.0 + 1.0));
            }
        }
    }

    return audio;
}

static void runStructureFinderTests()
{
    using namespace daw;
    namespace sf = analysis::structurefinder;
    std::cout << "Detecting the song structure" << std::endl;

    // A pop song: each part its own chords and sound, the chorus loudest and brightest.
    {
        const SongPart intro { 4, { "Am", "Am", "F", "F" }, 0.3, 1, false }, verse { 8, { "Am", "F", "C", "G" }, 0.5, 2, true },
                       chorus { 8, { "F", "G", "C", "Am" }, 1.0, 5, true }, bridge { 8, { "Dm", "Em", "F", "G" }, 0.7, 3, true },
                       outro { 4, { "Am", "F", "Am", "Am" }, 0.3, 1, false };
        const double rate = 22050.0;
        const auto song = makeSong ({ intro, verse, chorus, verse, chorus, bridge, chorus, outro }, rate);
        const auto found = sf::analyse (sf::prepareInput (song, rate), 0.0, 120.0);
        check (sf::toText (found) == "Intro (4) | Verse 1 (8) | Chorus 1 (8) | Verse 2 (8) | Chorus 2 (8) | Bridge (8) | Chorus 3 (8) | Outro (4)",
               "a pop song's parts are found on the right bars and named:\n" + sf::toText (found));
        check (sf::getPattern (found) == "A B C B C D C E", "the verses and the choruses are each the same music (" + sf::getPattern (found) + ")");
        check (! found.empty() && found[2].timesHeard == 3 && found[2].reason.contains ("loudest"), "the chorus is heard 3 times and is the loudest");

        // The same song placed from bar 3 on.
        const auto later = sf::analyse (sf::prepareInput (song, rate), 8.0, 120.0);
        check (! later.empty() && later.front().section.startBar == 2 && later.back().section.getEndBar() == 2 + 56,
               "a recording placed later gets its sections on the bars it plays over");
    }

    // Verse and chorus on the same chords, the chorus louder and brighter, after an intro of 2 bars.
    {
        const SongPart intro { 2, { "Am", "F" }, 0.3, 1, false }, verse { 8, { "Am", "F", "C", "G" }, 0.45, 2, true },
                       chorus { 8, { "Am", "F", "C", "G" }, 1.0, 5, true }, outro { 4, { "Am", "F", "Am", "Am" }, 0.3, 1, false };
        const auto song = makeSong ({ intro, verse, chorus, verse, chorus, chorus, outro }, 22050.0);
        const auto found = sf::analyse (sf::prepareInput (song, 22050.0), 0.0, 120.0);
        check (sf::toText (found) == "Intro (2) | Verse 1 (8) | Chorus 1 (8) | Verse 2 (8) | Chorus 2 (16) | Outro (4)",
               "a chorus on the verse's chords is told apart by its sound, and a short intro keeps the bars after it in phrases:\n" + sf::toText (found));
    }

    // A song that's one loop all the way through is one part.
    {
        const auto loop = makeSong ({ { 16, { "Am", "F", "C", "G" }, 0.6, 3, true } }, 22050.0);
        const auto found = sf::analyse (sf::prepareInput (loop, 22050.0), 0.0, 120.0);
        check (sf::toText (found) == "Part A (16)", "a loop all the way through is one part: " + sf::toText (found));
    }

    // Cuts at the strong changes, at the usual lengths.
    {
        std::vector<double> novelty (28, 0.02);
        novelty[4] = 0.5;
        novelty[12] = 0.6;
        novelty[13] = 0.45;   // the change spreads over a bar or two
        novelty[20] = 0.5;
        const auto starts = sf::findBoundaries (novelty, 0, 28);
        check (starts == std::vector<int> { 0, 4, 12, 20 }, "parts start at the strong changes, 8 bars long rather than 9 and 7");
    }

    // Naming from the groups alone.
    {
        auto name = [] (std::initializer_list<std::tuple<int, int, double>> parts)
        {
            std::vector<sf::Segment> segments;
            int bar = 0;

            for (const auto& [group, bars, loudness] : parts)
            {
                segments.push_back ({ bar, bars, group, loudness });
                bar += bars;
            }

            return sf::toText (sf::nameSections (segments));
        };

        const auto withPre = name ({ { 0, 4, -20.0 }, { 1, 8, -14.0 }, { 2, 4, -11.0 }, { 3, 8, -8.0 }, { 1, 8, -14.0 }, { 2, 4, -11.0 },
                                     { 3, 8, -8.0 }, { 3, 8, -8.0 }, { 4, 4, -20.0 } });
        check (withPre == "Intro (4) | Verse 1 (8) | Pre-chorus 1 (4) | Chorus 1 (8) | Verse 2 (8) | Pre-chorus 2 (4) | Chorus 2 (16) | Outro (4)",
               "a part before each chorus is a pre-chorus, and a chorus played twice in a row is one long chorus:\n" + withPre);

        const auto quietMiddle = name ({ { 0, 8, -14.0 }, { 1, 8, -8.0 }, { 0, 8, -14.0 }, { 1, 8, -8.0 }, { 2, 8, -26.0 }, { 1, 8, -8.0 } });
        check (quietMiddle == "Verse 1 (8) | Chorus 1 (8) | Verse 2 (8) | Chorus 2 (8) | Breakdown (8) | Chorus 3 (8)",
               "a song can start on the verse, and a quiet part heard once is a breakdown:\n" + quietMiddle);

        const auto noRepeats = name ({ { 0, 4, -20.0 }, { 1, 8, -12.0 }, { 2, 8, -10.0 }, { 3, 4, -20.0 } });
        check (noRepeats == "Intro (4) | Part B (8) | Part C (8) | Outro (4)", "music that never comes back gets letters, not verse and chorus: " + noRepeats);
    }
}

//==============================================================================
static void runLyricsFinderTests()
{
    using namespace daw;
    namespace lf = analysis::lyricsfinder;
    std::cout << "Detecting lyrics (English)" << std::endl;

    // Syllables by English spelling, as a songbook hyphenates them.
    {
        const std::pair<const char*, const char*> expected[] = {
            { "together", "to-ge-ther" }, { "dancing,", "danc-ing," }, { "little", "lit-tle" }, { "time", "time" },
            { "loves", "loves" }, { "smiled", "smiled" }, { "tumbled", "tum-bled" }, { "wanted", "wan-ted" },
            { "kisses", "kis-ses" }, { "changes", "chang-es" }, { "baby", "ba-by" }, { "happy", "hap-py" },
            { "children", "chil-dren" }, { "table", "ta-ble" }, { "people", "peo-ple" }, { "tonight", "to-night" },
            { "forever", "for-e-ver" }, { "something", "some-thing" }, { "maybe", "may-be" }, { "lonely", "lone-ly" },
            { "going", "go-ing" }, { "being", "be-ing" }, { "running", "run-ning" }, { "singing", "sing-ing" },
            { "lion", "li-on" }, { "piano", "pi-a-no" }, { "nation", "na-tion" }, { "million", "mil-lion" },
            { "father", "fa-ther" }, { "beautiful", "beau-ti-ful" }, { "remember", "re-mem-ber" }, { "heaven", "hea-ven" },
            { "money", "mo-ney" }, { "beyond", "be-yond" }, { "queen", "queen" }, { "fire", "fire" }, { "eyes", "eyes" },
            { "Hello,", "Hel-lo," }, { "night,", "night," }, { "I'm", "I'm" }, { "ooh-ooh", "ooh-ooh" }, { "USA", "USA" },
            { "2am", "2am" }, { "caf\xc3\xa9", "caf\xc3\xa9" }
        };

        juce::StringArray wrong;

        for (const auto& [word, hyphenated] : expected)
        {
            const auto got = lyrics::splitEnglishWord (juce::String::fromUTF8 (word)).joinIntoString ("-");

            if (got != juce::String::fromUTF8 (hyphenated))
                wrong.add (juce::String::fromUTF8 (word) + " gave " + got);
        }

        check (wrong.isEmpty(), "English words split into their sung syllables " + wrong.joinIntoString (", "));
    }

    check (models::formatSize (83994361) == "84 MB" && models::formatSize (1533774781) == "1.5 GB",
           "download sizes read as MB and GB (" + models::formatSize (1533774781) + ")");
    check (lf::getModel (lf::ModelSize::small).size == 487614201 && juce::String (lf::getModel (lf::ModelSize::medium).fileName).endsWith (".en.bin")
               && juce::String (lf::getModel (lf::ModelSize::small).fileName).endsWith (".en.bin"),
           "the Whisper models are the English-only ones, at their known sizes");

    // Mono at 16 kHz: 1 s of stereo at 44.1 kHz becomes 16000 samples, the channels averaged.
    {
        juce::AudioBuffer<float> stereo (2, 44100);

        for (int i = 0; i < 44100; ++i)
        {
            const float v = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / 44100.0);
            stereo.setSample (0, i, v);
            stereo.setSample (1, i, v);
        }

        const auto mono = lf::prepareInput (stereo, 44100.0);
        check (mono.getNumChannels() == 1 && std::abs (mono.getNumSamples() - 16000) <= 1
                   && std::abs (mono.getRMSLevel (0, 1000, 14000) - 0.5f / std::sqrt (2.0f)) < 0.01f,
               "the audio goes to Whisper as mono at 16 kHz, at the same level");
    }

    // Where someone sings: 1 s of quiet, a phrase with a short breath in it, a pause, and a blip.
    {
        const double rate = lf::sampleRate;
        juce::AudioBuffer<float> mono (1, (int) (rate * 5.0));
        mono.clear();

        auto tone = [&] (double from, double to, float level)
        {
            for (int i = (int) (from * rate); i < (int) (to * rate); ++i)
                mono.setSample (0, i, level * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / rate));
        };

        tone (1.0, 2.0, 0.5f);
        tone (2.15, 3.0, 0.3f);    // after a 0.15 s breath
        tone (4.0, 4.03, 0.5f);    // a 30 ms click
        tone (0.0, 1.0, 0.002f);   // bleed of the band, 48 dB down

        const auto singing = lf::findSinging (mono, rate);
        check (singing.size() == 1 && std::abs (singing[0].getStart() - 1.0) < 0.03 && std::abs (singing[0].getEnd() - 3.0) < 0.05,
               "the singing is found from the level, breaths bridged, clicks and bleed left out ("
                   + juce::String (singing.size()) + " parts)");

        lf::muteOutside (mono, rate, singing);
        check (mono.getMagnitude (0, 0, (int) (0.9 * rate)) < 1.0e-6f && mono.getMagnitude (0, (int) (3.2 * rate), (int) (1.8 * rate)) < 1.0e-6f
                   && mono.getMagnitude (0, (int) (1.2 * rate), (int) (0.5 * rate)) > 0.45f,
               "everything outside the singing is muted before Whisper listens");
    }

    // Clean-up: sounds in brackets, signs, made-up phrases and words outside the singing go.
    {
        const std::vector<juce::Range<double>> singing { { 1.0, 3.0 }, { 5.0, 6.0 } };
        std::vector<lf::Word> heard {
            { "[Music]", 0.0, 0.9, 0 }, { "(upbeat", 0.9, 1.0, 0 }, { "music)", 1.0, 1.1, 0 },
            { "Hold", 1.0, 1.5, 1 }, { "on,", 1.4, 2.0, 1 }, { "(yeah)", 2.0, 2.4, 1 }, { "\xe2\x99\xaa", 2.4, 2.6, 1 },
            { "Thanks", 3.5, 3.8, 2 }, { "for", 3.8, 3.9, 2 }, { "watching!", 3.9, 4.5, 2 },
            { "you", 4.0, 4.4, 3 }, { "tonight", 4.8, 5.6, 4 }
        };

        for (auto& w : heard)
            w.text = juce::String::fromUTF8 (w.text.toRawUTF8());

        const auto kept = lf::cleanUp (heard, singing);
        juce::StringArray texts;

        for (const auto& w : kept)
            texts.add (w.text);

        check (texts.joinIntoString (" ") == "Hold on, yeah tonight", "only the sung words stay (" + texts.joinIntoString (" ") + ")");
        check (kept.size() == 4 && juce::exactlyEqual (kept[1].end, 2.0) && kept[0].end <= kept[1].start && juce::exactlyEqual (kept[3].start, 5.0),
               "words don't overlap, and start where the singing does");
        check (lf::isMadeUpPhrase ("Subtitles by the Amara.org community") && ! lf::isMadeUpPhrase ("Thanks for the memories"),
               "phrases Whisper makes up from silence are recognised, real lines aren't");
    }

    // Snapping words to the singing: six 0.3 s notes with known starts, and words that Whisper-like timing puts 80 ms off.
    {
        const double rate = lf::sampleRate;
        const double noteStarts[] = { 0.507, 1.0, 1.6, 2.2, 2.8, 3.6 };
        juce::AudioBuffer<float> mono (1, (int) (rate * 5.0));
        mono.clear();

        for (int n = 0; n < 6; ++n)
        {
            const double pitch = 180.0 + 35.0 * n;

            for (int i = 0; i < (int) (0.3 * rate); ++i)
            {
                const double t = i / rate;
                const double envelope = juce::jmin (1.0, t / 0.005) * juce::jmin (1.0, (0.3 - t) / 0.02);
                const double phase = juce::MathConstants<double>::twoPi * pitch * t;
                mono.setSample (0, (int) (noteStarts[n] * rate) + i,
                                (float) (envelope * (0.4 * std::sin (phase) + 0.2 * std::sin (3.0 * phase) + 0.1 * std::sin (5.0 * phase))));
            }
        }

        const auto found = analysis::onsets::findOnsets (mono, rate);
        double worst = 0.0;

        for (size_t n = 0; n < 6 && n < found.size(); ++n)
            worst = juce::jmax (worst, std::abs (found[n] - noteStarts[n]));

        check (found.size() == 6 && worst <= 0.01, "an onset is found at the start of each note, within one 10 ms hop (" + juce::String (found.size()) + " found, worst "
                                                       + juce::String (worst * 1000.0, 1) + " ms)");

        juce::AudioBuffer<float> silence (1, (int) (rate * 3.0));
        silence.clear();
        check (analysis::onsets::findOnsets (silence, rate).empty(), "silence has no onsets");

        std::vector<lf::Word> words {
            { "Hold", 0.587, 0.95, 0 },    // 80 ms late
            { "on", 0.92, 1.5, 0 },        // 80 ms early
            { "to", 1.68, 2.1, 0 },        // 80 ms late
            { "Maybe", 2.28, 3.4, 0 },     // 80 ms late, two syllables
            { "me", 3.9, 4.3, 0 },         // 0.3 s from the nearest onset: too far
        };

        lf::snapToOnsets (words, found);
        const double starts[] = { 0.507, 1.0, 1.6, 2.2 };
        double worstSnap = 0.0;

        for (size_t i = 0; i < 4; ++i)
        {
            worstSnap = juce::jmax (worstSnap, std::abs (words[i].start - starts[i]));
        }

        check (worstSnap <= 0.01, "words 80 ms off snap to the start of the note, within one hop (worst " + juce::String (worstSnap * 1000.0, 1) + " ms)");
        check (juce::exactlyEqual (words[4].start, 3.9) && juce::exactlyEqual (words[4].end, 4.3),
               "a word with no onset within 0.15 s stays where it was");

        bool ordered = true;

        for (size_t i = 0; i < words.size(); ++i)
            ordered = ordered && words[i].end >= words[i].start + 0.05 - 1.0e-9 && (i == 0 || words[i].start >= words[i - 1].start + 0.02 - 1.0e-9)
                      && (i + 1 == words.size() || words[i].end <= words[i + 1].start + 1.0e-9);

        check (ordered, "snapped words keep their order, 20 ms between starts, at least 50 ms long and no overlaps");

        // The syllables of "Maybe" (2.2 to 3.4): evenly the second would start at 2.8; the voice starts again at 2.8, so use a word whose halves differ.
        lf::Word maybe { "Maybe", 2.2, 3.5, 0, { 2.8 } };
        auto times = lf::getSyllableTimes (maybe, 2);
        check (times.size() == 2 && juce::exactlyEqual (times[0], 2.2) && juce::exactlyEqual (times[1], 2.8),
               "a syllable starts at the word's own onset when one is near its even split (2.85 to 2.8)");

        maybe.onsets = { 2.4 };
        times = lf::getSyllableTimes (maybe, 2);
        check (times.size() == 2 && std::abs (times[1] - 2.85) < 1.0e-9, "an onset too far from the even split is ignored, so the word is shared out evenly");

        maybe.onsets.clear();
        times = lf::getSyllableTimes (maybe, 3);
        check (times.size() == 3 && std::abs (times[1] - (2.2 + 1.3 / 3.0)) < 1.0e-9, "with no onsets inside, syllables are shared out evenly as before");

        // In the lyrics: 120 BPM, a second is two beats.
        lf::Word two { "Maybe", 1.0, 2.0, 0, { 1.6 } };
        const auto lyr = lf::toLyrics ({ two }, 0.0, daw::TempoMap (120.0), {});
        check (lyr.lines.size() == 1 && lyr.lines[0].syllables.size() == 2 && juce::exactlyEqual (lyr.lines[0].syllables[1].beat, 3.2),
               "a syllable lands on the voice's onset in the lyrics (beat 3.2, not 3.0)");
    }

    // To the lyrics track: at 120 BPM a second is two beats; the clip starts at bar 3 (beat 8).
    {
        SectionList sections { { 2, 2, SectionType::verse, {} }, { 4, 2, SectionType::chorus, {} } };
        std::vector<lf::Word> words {
            { "I", 0.0, 0.25, 0 }, { "walked", 0.25, 0.75, 0 }, { "alone.", 0.75, 1.5, 0 },
            { "The", 2.0, 2.25, 0 }, { "river", 2.25, 3.0, 0 }, { "ran,", 3.0, 3.5, 0 },
            { "Together", 4.0, 5.0, 1 }, { "tonight!", 5.0, 6.0, 1 }
        };

        const auto found = lf::toLyrics (words, 8.0, daw::TempoMap (120.0), sections);
        check (found.lines.size() == 3, "a pause of half a second starts a new line, and so does a section (" + juce::String (found.lines.size()) + " lines)");

        if (found.lines.size() == 3)
        {
            const auto& first = found.lines[0];
            const auto& chorus = found.lines[2];
            check (lyrics::toText (found) == "[Verse]\nI walked a-lone\nThe ri-ver ran\n\n[Chorus]\nTo-ge-ther to-night",
                   "the words become lyrics text with verses, headings and syllables:\n" + lyrics::toText (found));
            check (juce::exactlyEqual (first.startBeat, 8.0) && juce::exactlyEqual (first.syllables[1].beat, 8.5)
                       && juce::exactlyEqual (first.syllables[3].beat, 10.25) && juce::exactlyEqual (first.endBeat, 11.0),
                   "each syllable is placed where it's sung, a word's time shared out over its syllables");
            check (std::all_of (first.syllables.begin() + 1, first.syllables.end(), [] (const LyricSyllable& s) { return s.pinned; }),
                   "the found syllables are pinned, so spreading leaves them where they're sung");
            check (chorus.startsVerse && chorus.heading == "Chorus" && juce::exactlyEqual (chorus.startBeat, 16.0),
                   "the chorus's words start a verse headed Chorus at bar 5");
        }

        // A long line splits at its longest pause.
        std::vector<lf::Word> many;

        for (int i = 0; i < 16; ++i)
            many.push_back ({ "la", i * 0.3 + (i >= 9 ? 0.15 : 0.0), i * 0.3 + 0.2 + (i >= 9 ? 0.15 : 0.0), 0 });

        const auto split = lf::toLyrics (many, 0.0, daw::TempoMap (120.0), {});
        check (split.lines.size() == 2 && split.lines[0].syllables.size() == 9, "a line of 16 words splits at its longest pause");

        // Found lyrics replace the lines on the clip's bars and keep the others.
        Lyrics current = lyrics::parse ("before\n\nduring\n\nafter");
        const double starts[] = { 0.0, 10.0, 24.0 };

        for (size_t i = 0; i < current.lines.size(); ++i)
        {
            current.lines[i].startBeat = starts[i];
            current.lines[i].endBeat = starts[i] + 4.0;
        }

        lyrics::tidy (current);
        const auto merged = lf::replaceBetween (current, found, 8.0, 20.0);
        check (merged.lines.size() == 5 && merged.lines.front().syllables[0].text == "before" && merged.lines.back().syllables[0].text == "after",
               "found lyrics replace only the lines on the clip's bars (" + juce::String (merged.lines.size()) + " lines)");
    }
}

/** Runs Whisper on speech made with macOS's own voice, to check the words and their timing. */
static void runWhisperTest (const juce::File& modelFile)
{
    using namespace daw;
    namespace lf = analysis::lyricsfinder;
    std::cout << "Whisper with " << modelFile.getFullPathName() << std::endl;

   #if JUCE_MAC
    const auto speech = juce::File::createTempFile (".aiff");
    juce::ChildProcess say;
    say.start (juce::StringArray { "say", "-o", speech.getFullPathName(), "--data-format=BEI16@16000",
                                   "Hold on to me. [[slnc 2000]] The night is young and we are free." });
    say.waitForProcessToFinish (30000);

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (speech));
    check (reader != nullptr, "macOS made the speech to listen to");

    if (reader == nullptr)
        return;

    juce::AudioBuffer<float> audio ((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read (&audio, 0, (int) reader->lengthInSamples, 0, true, true);
    const double rate = reader->sampleRate;
    reader.reset();
    speech.deleteFile();

    auto mono = lf::prepareInput (audio, rate);
    const auto singing = lf::findSinging (mono, lf::sampleRate);
    lf::muteOutside (mono, lf::sampleRate, singing);

    lf::Transcriber transcriber;
    juce::String error;
    const auto size = modelFile.getFileName().contains ("medium") ? lf::ModelSize::medium : lf::ModelSize::small;
    check (transcriber.load (modelFile, size, error), "the model loads " + error);

    const auto started = juce::Time::getMillisecondCounterHiRes();
    const auto heard = transcriber.transcribe (mono, [] (double) { return true; }, error);
    const auto seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
    check (heard.has_value(), "Whisper listens to it " + error);

    if (! heard.has_value())
        return;

    juce::StringArray raw, ranges;

    for (const auto& w : *heard)
        raw.add (w.text + " " + juce::String (w.start, 2) + "-" + juce::String (w.end, 2));

    for (const auto& r : singing)
        ranges.add (juce::String (r.getStart(), 2) + "-" + juce::String (r.getEnd(), 2));

    std::cout << "    Whisper wrote: " << raw.joinIntoString (" | ") << "\n    singing at " << ranges.joinIntoString (", ") << std::endl;

    const auto words = lf::cleanUp (*heard, singing);
    juce::StringArray texts;

    for (const auto& w : words)
        texts.add (w.text + " " + juce::String (w.start, 2));

    std::cout << "    heard in " << seconds << " s: " << texts.joinIntoString (" | ") << std::endl;

    const auto text = texts.joinIntoString (" ").toLowerCase();
    check (text.contains ("hold") && text.contains ("night") && text.contains ("young") && text.contains ("free"), "it hears the words");

    // "The" comes after the two-second pause, at the start of the second phrase; "me" ends the first.
    const auto the = std::find_if (words.begin(), words.end(), [] (const lf::Word& w) { return w.text.equalsIgnoreCase ("the"); });
    const auto me = std::find_if (words.begin(), words.end(), [] (const lf::Word& w) { return w.text.startsWithIgnoreCase ("me"); });
    check (the != words.end() && singing.size() == 2 && std::abs (the->start - singing[1].getStart()) < 0.1,
           "a phrase's first word starts where it's spoken (within 0.1 s)");
    check (me != words.end() && me->end <= singing[0].getEnd() + 0.1, "the last word before a pause ends with the speech, not after the pause");

    const auto found = lf::toLyrics (words, 0.0, daw::TempoMap (120.0), {});
    check (found.lines.size() == 2, "the pause splits the words into two lines:\n" + lyrics::toText (found));
   #else
    juce::ignoreUnused (modelFile);
   #endif
}

//==============================================================================
static void runSingingGuideTests()
{
    using namespace daw;
    namespace sg = gen::singing;
    std::cout << "Singing guide" << std::endl;

    check (sg::getNoteName (40) == "E2" && sg::getNoteName (60) == "C4" && sg::getNoteName (69) == "A4" && sg::getNoteName (84) == "C6" && sg::getNoteName (70) == "A#4",
           "singing guide: notes are named with their octave, middle C is C4 and A4 is 69");
    check (std::abs (sg::getFrequency (40) - 82.41) < 0.01 && std::abs (sg::getFrequency (57) - 220.0) < 0.01
               && std::abs (sg::getFrequency (69) - 440.0) < 0.01 && std::abs (sg::getFrequency (84) - 1046.50) < 0.01,
           "singing guide: E2 is 82.41 Hz, A3 220, A4 440 and C6 1046.50");

    const auto& types = sg::getVoiceTypes();
    bool spans = types.size() == 6;

    for (auto& t : types)
        spans = spans && t.highest - t.lowest == 24;

    check (spans && types[0].name == "Bass" && types[2].name == "Tenor" && types[5].name == "Soprano" && types[0].lowest == 40 && types[5].highest == 84
               && sg::getNoteName (types[0].lowest) == "E2" && sg::getNoteName (types[2].lowest) == "C3" && sg::getNoteName (types[4].lowest) == "A3",
           "singing guide: six voice types from bass to soprano, each spanning 24 semitones");

    auto closest = [&] (int lo, int hi) { const int i = sg::getClosestType ({ lo, hi }); return i < 0 ? juce::String() : types[(size_t) i].name.toLowerCase(); };
    check (closest (43, 62) == "bass" && closest (48, 72) == "tenor" && closest (55, 74) == "alto" && closest (62, 79) == "mezzo-soprano" && closest (60, 84) == "soprano",
           "singing guide: the closest voice type to a range, a tie going to the lower one (" + closest (62, 79) + ")");

    const auto fits = sg::getTypesThatFit (60, 71);
    check (fits.size() == 4 && types[(size_t) fits[0]].name == "Tenor" && types[(size_t) fits[1]].name == "Alto"
               && types[(size_t) fits[2]].name == "Mezzo-soprano" && types[(size_t) fits[3]].name == "Soprano",
           "singing guide: a melody from C4 to B4 fits a tenor, an alto, a mezzo-soprano and a soprano");
    check (sg::getRangeLabel ({}) == "Not set" && sg::getRangeLabel ({ 48, 72 }) == "Tenor" && sg::getRangeLabel ({ 50, 72 }) == "Custom (closest: tenor)"
               && ! sg::Range { 60, 50 }.isSet() && ! sg::Range { 60, 64 }.isSet() && sg::Range { 60, 65 }.isSet(),
           "singing guide: the voice row's label is Not set, a type's name or Custom with the closest type, and a range needs 5 semitones");

    // The melody track to start on.
    {
        // The pop song's tracks: Kick, Snare, Hats, Bass, Keys, Lead vocal, BV 1, BV 2.
        std::vector<sg::TrackInfo> tracks { { "Kick", true, true }, { "Snare", true, true }, { "Hats", true, true }, { "Bass", true, false },
                                            { "Keys", true, false }, { "Lead vocal", true, false }, { "BV 1", true, false }, { "BV 2", true, false } };
        check (sg::chooseMelodyTrack (tracks, -1) == 5 && sg::chooseMelodyTrack (tracks, 3) == 3 && sg::chooseMelodyTrack (tracks, 6) == 6,
               "singing guide: the melody track is the selected one with notes, else the first with a vocal name (Lead vocal)");
        tracks[3].hasNotes = false;
        tracks[5].hasNotes = false;
        tracks[6].hasNotes = false;
        tracks[7].hasNotes = false;
        check (sg::chooseMelodyTrack (tracks, 3) == 4 && sg::chooseMelodyTrack (tracks, -1) == 4, "singing guide: without vocal names it takes the first track with notes that doesn't play drums");
        tracks[4].hasNotes = false;
        check (sg::chooseMelodyTrack (tracks, -1) == -1 && sg::hasVocalWord ("Lead vocal") && sg::hasVocalWord ("BV 2") && sg::hasVocalWord ("Backing vocals")
                   && sg::hasVocalWord ("Choir") && ! sg::hasVocalWord ("Bass") && ! sg::hasVocalWord ("Keys"),
               "singing guide: drums are never picked, and a track's name says whether it's a vocal");
    }

    // Texts.
    {
        bool everyTopic = std::size (sg::allTopics) == 13 && sg::allTopics[12] == sg::Topic::yours;
        bool spelling = true;
        sg::Context context;

        for (auto t : sg::allTopics)
        {
            const auto text = sg::explain (t, context);
            everyTopic = everyTopic && sg::getTopicName (t).isNotEmpty() && sg::getTopicSummary (t).isNotEmpty() && text.length() >= 400
                         && sg::getExampleSize (t, {}) > 0;
            spelling = spelling && ! text.containsIgnoreCase ("color") && ! text.containsIgnoreCase ("center") && ! text.containsIgnoreCase ("analyze");
        }

        check (everyTopic && spelling, "singing guide: 13 topics, each with a name, a summary, 400 characters of text and an example, in British spelling");

        context.range = { 48, 72 };
        const auto with = sg::explain (sg::Topic::warmup, context), without = sg::explain (sg::Topic::warmup, {});
        check (with.contains ("starts on F3, a fourth above your lowest note (C3)") && without.contains ("starts on C4 (middle C)") && ! with.contains ("{start}")
                   && sg::getWarmupExample ({ 48, 72 }).scale[0].pitch == 53 && sg::getWarmupExample ({}).scale[0].pitch == 60
                   && sg::getWarmupExample ({}).scale.size() == 9 && sg::getWarmupExample ({}).repeat[0].pitch == 61,
               "singing guide: the warm-up scale starts a fourth above the lowest note, or on middle C without a range");
        check (sg::explain (sg::Topic::care, {}).contains ("still hoarse after 2 weeks") && sg::explain (sg::Topic::care, {}).contains ("111 or 999"),
               "singing guide: the care topic says to see a doctor when still hoarse after 2 weeks");
    }

    // The pictures' numbers.
    {
        const auto breathing = sg::getBreathingExample();
        check (breathing.lines.size() == 3 && std::abs (breathing.getRestSeconds (0) - 1.25) < 0.001 && std::abs (breathing.getRestSeconds (1) - 0.3125) < 0.001
                   && std::abs (breathing.getLineSeconds (2) - 6.25) < 0.001,
               "singing guide: the breathing example at 96 BPM has rests of 1.25 s and 0.3125 s and a line of 6.25 s");

        const auto mic = sg::getMicDistances();
        const double expected[] { 12.04, 6.02, 2.50, 0.0, -3.52, -6.02, -9.54 };
        bool micOk = mic.size() == 7;

        for (size_t i = 0; micOk && i < mic.size(); ++i)
            micOk = std::abs (sg::getMicLevelDb (mic[i]) - expected[i]) < 0.01;

        check (micOk, "singing guide: the microphone curve is +12.04, +6.02, +2.50, 0, -3.52, -6.02 and -9.54 dB from 5 to 60 cm");

        const auto vibrato = sg::getVibratoExample();
        double peakThrough = 0.0, peakLate = 0.0;

        for (double t = 0.0; t < 2.0; t += 0.0005)
        {
            peakThrough = juce::jmax (peakThrough, std::abs (vibrato.curves[2].getCents (t)));
            peakLate = juce::jmax (peakLate, std::abs (vibrato.curves[1].getCents (t)));
        }

        check (std::abs (peakThrough - 60.0) < 0.5 && std::abs (peakLate - 30.0) < 0.5 && std::abs (vibrato.curves[2].getPeriodSeconds() - 0.1667) < 0.001
                   && std::abs (vibrato.curves[1].getPeriodSeconds() - 0.1818) < 0.001 && vibrato.curves[0].getCents (1.3) == 0.0 && vibrato.curves[1].getCents (0.5) == 0.0,
               "singing guide: vibrato all through swings 60 cents at 6 Hz, a late one 30 cents at 5.5 Hz from 1 s, a straight tone not at all");

        const auto tune = sg::getTuneExamples();
        check (tune.size() == 3 && std::abs (tune[0].getCents (0.0) + 150.0) < 1.0e-9 && tune[0].getCents (0.3) == 0.0 && tune[1].getCents (0.2) == 0.0
                   && std::abs (tune[2].getCents (1.5) + 30.0) < 1.0e-9 && tune[2].getCents (0.4) == 0.0,
               "singing guide: the in-tune examples slide in 150 cents low, swing around the note, and drift 30 cents flat");

        const auto harmony = sg::getHarmonyExample();
        juce::String thirds;

        for (size_t i = 0; i < harmony.lead.size(); ++i)
            thirds << (i > 0 ? " " : "") << (harmony.harmony[i].pitch - harmony.lead[i].pitch);

        check (thirds == "3 3 4 3 3 4" && std::abs (sg::getVoiceExample().fundamental - 220.0) < 1.0e-9 && sg::getVoiceExample().harmonics * 220 == 3960
                   && sg::VoiceExample::getEnvelope (sg::getVoiceExample().ahFormants, 730.0) > sg::VoiceExample::getEnvelope (sg::getVoiceExample().ahFormants, 3000.0),
               "singing guide: the harmony is a third above on every note (" + thirds + "), and the voice picture has 18 harmonics of 220 Hz");
    }

    // The range check on made-up melodies.
    {
        const MusicalKey aMinor { 9, MusicalKey::Scale::minor };
        const TempoMap tempo (96.0);
        auto melodyOf = [&] (std::vector<std::pair<int, double>> notes)
        {
            std::vector<gen::singthis::SourceNote> source;
            double beat = 0.0;

            for (auto& [pitch, length] : notes)
            {
                source.push_back ({ pitch, beat, length, 0.8f });
                beat += length;
            }

            return gen::singthis::buildMelody (source, tempo);
        };
        const SectionList sections { { 0, 1, SectionType::verse, {} }, { 1, 4, SectionType::chorus, {} } };
        auto check_ = [&] (const std::vector<std::pair<int, double>>& notes, sg::Range range)
        {
            return sg::analyse (melodyOf (notes), "Lead vocal", sections, aMinor, {}, 96.0, range);
        };
        using V = sg::Analysis::Verdict;

        const std::vector<std::pair<int, double>> m1 { { 60, 1 }, { 62, 1 }, { 64, 1 }, { 65, 1 }, { 67, 2 }, { 69, 2 }, { 71, 2 } };
        const auto high = check_ (m1, { 45, 69 });
        check (high.verdict == V::tooHigh && high.outside == 2 && high.shift == -2 && ! high.octave && high.newKeyName == "G minor" && high.newLowest == 58 && high.newHighest == 69
                   && high.paragraphs.size() == 1 && high.paragraphs[0].contains ("goes 2 semitones above your range") && high.paragraphs[0].contains ("its highest note is B4 (bar 3)")
                   && high.paragraphs[0].contains ("and your highest is A4") && high.paragraphs[0].contains ("in G minor instead of A minor")
                   && high.paragraphs[0].contains ("runs from A#3 to A4") && high.paragraphs[0].startsWith ("The chorus"),
               "singing guide: a melody 2 semitones too high is moved 2 down, to G minor, running from A#3 to A4 (" + high.paragraphs.joinIntoString (" ") + ")");

        const auto fine = check_ (m1, { 48, 72 });
        check (fine.verdict == V::fits && std::abs (fine.share - 0.2) < 0.001 && fine.sparesTop == 1 && fine.sparesBottom == 12 && fine.shift == 0
                   && fine.paragraphs[0].contains ("sits inside your range (C3 to C5), with 1 semitone to spare at the top and 12 at the bottom. A good key for you."),
               "singing guide: the same melody in 48 to 72 fits well (top share below 25%)");

        // The top share is 20% before the long note, 27% after it.
        std::vector<std::pair<int, double>> m2 { { 60, 1 }, { 62, 1 }, { 64, 1 }, { 65, 1 }, { 67, 2 }, { 69, 2 }, { 71, 3 } };
        const auto top = check_ (m2, { 48, 72 });
        check (top.verdict == V::nearTop && std::abs (top.share - 3.0 / 11.0) < 0.01 && top.shift == -2 && top.newKeyName == "G minor"
                   && top.paragraphs[0].contains ("27% of the sung time is in the top 3 semitones of your range, most of it in the chorus")
                   && top.paragraphs[0].contains ("Try it 2 semitones lower: in G minor instead of A minor."),
               "singing guide: a melody with a long top note is near the top (27%) and is moved 2 semitones down (" + top.paragraphs.joinIntoString (" ") + ")");
        check (check_ ({ { 60, 1 }, { 62, 1 }, { 64, 1 }, { 65, 1 }, { 67, 2 }, { 69, 2 }, { 71, 3 } }, { 59, 72 }).shift == -1
                   && check_ ({ { 60, 1 }, { 62, 1 }, { 64, 1 }, { 65, 1 }, { 67, 2 }, { 69, 2 }, { 71, 3 } }, { 60, 72 }).shift == 0
                   && check_ ({ { 60, 1 }, { 62, 1 }, { 64, 1 }, { 65, 1 }, { 67, 2 }, { 69, 2 }, { 71, 3 } }, { 60, 72 }).paragraphs[0].contains ("It already uses the bottom of your range"),
               "singing guide: near the top it moves a semitone when two don't fit, and says so when none does");

        const auto octaveDown = check_ ({ { 64, 1 }, { 67, 1 }, { 72, 1 }, { 76, 1 } }, { 45, 69 });
        check (octaveDown.verdict == V::tooHigh && octaveDown.octave && octaveDown.shift == -12 && octaveDown.newLowest == 52 && octaveDown.newHighest == 64
                   && octaveDown.paragraphs[0].contains ("Try singing it an octave lower: then it runs from E3 to E4, inside your range, and nothing in the song has to change."),
               "singing guide: when the closest key is 5 or more away (7) and an octave fits, it suggests the octave (E3 to E4)");

        const auto octaveUp = check_ ({ { 50, 1 }, { 54, 1 }, { 57, 1 }, { 62, 1 } }, { 57, 81 });
        check (octaveUp.verdict == V::tooLow && octaveUp.octave && octaveUp.shift == 12 && octaveUp.newLowest == 62 && octaveUp.newHighest == 74
                   && octaveUp.paragraphs[0].contains ("below your range") && octaveUp.paragraphs[0].contains ("an octave higher"),
               "singing guide: a melody too low, with the closest key 7 away, goes an octave up (62 to 74)");

        const auto wide = check_ ({ { 50, 1 }, { 80, 1 } }, { 48, 72 });
        check (wide.verdict == V::noKey && wide.paragraphs[0].contains ("spans 30 semitones (D3 to G#5), and your range spans 24 (C3 to C5), so no key fits all of it"),
               "singing guide: a melody wider than the range fits no key");

        const auto bottom = check_ ({ { 48, 2 }, { 50, 2 }, { 52, 1 }, { 55, 1 } }, { 48, 72 });
        check (bottom.verdict == V::nearBottom && std::abs (bottom.share - 4.0 / 6.0) < 0.01 && bottom.shift == 2 && bottom.newKeyName == "B minor"
                   && bottom.paragraphs[0].contains ("67% of the sung time is in the bottom 3 semitones of your range")
                   && bottom.paragraphs[0].contains ("in B minor instead of A minor"),
               "singing guide: a melody low in the range is near the bottom (67%) and goes 2 semitones up, to B minor");

        const auto none = check_ ({}, { 48, 72 });
        check (none.verdict == V::noMelody && none.paragraphs.size() == 1 && none.paragraphs[0].startsWith ("Your song has no melody to check yet.")
                   && none.paragraphs[0].contains ("Convert to MIDI..."),
               "singing guide: without a melody it says to write or record one");

        const auto unset = check_ ({ { 60, 1 }, { 71, 1 } }, {});
        check (unset.verdict == V::noRange && unset.paragraphs[0].startsWith ("The melody (Lead vocal) goes from C4 to B4, 11 semitones. In the usual ranges that fits a tenor, an alto, a mezzo-soprano or a soprano. Set your own range above")
                   && check_ ({ { 40, 1 }, { 84, 1 } }, {}).paragraphs[0].contains ("wider than any one voice type's usual range")
                   && check_ ({ { 60, 1 }, { 71, 1 } }, { 60, 50 }).verdict == V::noRange,
               "singing guide: without a range it lists the voice types that fit, and a range with the lowest above the highest counts as not set");

        // The longest line.
        Lyrics lyrics = lyrics::place (lyrics::parse ("[Verse]\nI walked a-long the ri-ver late"), {});

        if (! lyrics.lines.empty())
        {
            auto& line = lyrics.lines[0];
            line.endBeat = line.startBeat + 10.0;
            const auto longLine = sg::analyse (melodyOf ({ { 60, 1 }, { 62, 1 } }), "Lead", sections, aMinor, lyrics, 96.0, { 48, 72 });
            check (longLine.paragraphs.size() == 2 && longLine.paragraphs[1].startsWith ("Your longest line, \"I walked along the river late\", lasts 6.3 seconds: plan a deep breath before it"),
                   "singing guide: a 10-beat line at 96 BPM gets a reminder to breathe (" + longLine.paragraphs.joinIntoString (" | ") + ")");
            line.endBeat = line.startBeat + 4.0;
            check (sg::analyse (melodyOf ({ { 60, 1 }, { 62, 1 } }), "Lead", sections, aMinor, lyrics, 96.0, { 48, 72 }).paragraphs.size() == 1,
                   "singing guide: and nothing for a 4-beat line");
        }

        sg::Context context;
        context.analysis = high;
        const auto yours = sg::explain (sg::Topic::yours, context);
        check (yours.startsWith ("Your voice and your song checks the melody") && yours.contains ("in G minor instead of A minor") && yours.length() >= 400,
               "singing guide: Your voice and your song is its introduction followed by the advice");
    }
}

static void runTheoryGuidesTests()
{
    namespace tg = daw::gen::guides;
    const auto entries = tg::getEntries();

    check (entries.size() == std::size (tg::allGuides), "theory guides: every guide has an entry");

    juce::StringArray names;
    bool inOrder = true, complete = true;

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& e = entries[i];
        inOrder = inOrder && e.guide == tg::allGuides[i];
        complete = complete && e.name.isNotEmpty() && e.summary.endsWithChar ('.') && e.opensFrom.isNotEmpty() && ! e.keywords.isEmpty();
        names.addIfNotAlreadyThere (e.name);
    }

    check (inOrder, "theory guides: the entries follow allGuides");
    check (complete, "theory guides: every guide has a name, a summary, where it opens from and search words");
    check (names.size() == (int) entries.size(), "theory guides: the names are all different");

    bool everyAreaUsed = true;

    for (auto a : tg::allAreas)
    {
        everyAreaUsed = everyAreaUsed && tg::getAreaName (a).isNotEmpty() && tg::getAreaSummary (a).isNotEmpty()
                        && std::any_of (entries.begin(), entries.end(), [a] (auto& e) { return e.area == a; });
    }

    check (everyAreaUsed, "theory guides: every area has a name, a summary and at least one guide");

    check (tg::getEntry (tg::Guide::stereo).plugin == "DawIt Stereo", "Stereo guide opens the built-in effect's pages");
    for (const auto* query : { "goniometer", "mono", "vinyl", "correlation" })
    {
        const auto found = tg::search (entries, query);
        check (std::any_of (found.begin(), found.end(), [&] (const auto& result) { return entries[(size_t) result.entry].guide == tg::Guide::stereo; }), "Stereo guide is found by its search words");
    }

    // The topics come from the guides' own lists.
    check (tg::getEntry (tg::Guide::sound).topics.size() == (int) std::size (daw::soundguide::allTopics)
               && tg::getEntry (tg::Guide::sound).topics[0] == daw::soundguide::getTopicName (daw::soundguide::allTopics[0]),
           "theory guides: What is sound? lists its own topics");
    {
        namespace sg = daw::soundguide;
        const auto sound = tg::getEntry (tg::Guide::sound);
        const auto cancellation = sound.topics.indexOf (sg::getTopicName (sg::Topic::cancellation));
        bool found = true;

        for (const auto* query : { "adsr", "mono", "envelope", "attack release" })
        {
            const auto results = tg::search (entries, query);
            found = found && std::any_of (results.begin(), results.end(), [&] (const auto& r) { return entries[(size_t) r.entry].guide == tg::Guide::sound; });
        }

        check (sound.topics.size() == 10 && sound.topics[cancellation + 1] == "Checking in mono" && sound.topics[cancellation + 2] == "Taking the vocals out"
                   && found && sg::explain (sg::Topic::timbre).contains ("What is subtractive synthesis?")
                   && sound.summary.contains ("envelopes") && sound.summary.contains ("mono"),
               "theory guides: What is sound? has Checking in mono before the vocals and is found by adsr, mono and envelope");
    }
    check (tg::getEntry (tg::Guide::lyrics).topics.size() == (int) std::size (daw::gen::lyricsguide::allTopics),
           "theory guides: the lyrics guide lists its own topics");

    {
        // The singing guide: its own area, its topics, and the words it's found by.
        namespace sg = daw::gen::singing;
        const auto singing = tg::getEntry (tg::Guide::singing);
        const auto startPath = tg::getStartPath();
        check (singing.area == tg::Area::singing && tg::getAreaName (tg::Area::singing) == "Singing" && singing.name == "Singing techniques"
                   && singing.topics.size() == (int) std::size (sg::allTopics) && singing.topics[0] == "How the voice works" && singing.topics[12] == "Your voice and your song"
                   && singing.opensFrom.contains ("Tips...") && std::find (startPath.begin(), startPath.end(), tg::Guide::singing) == startPath.end(),
               "theory guides: the singing guide is in its own area, lists its 13 topics and isn't on the Start here path");

        bool allFound = true;

        for (auto* query : { "hoarse", "vibrato", "voice range", "pop filter" })
        {
            const auto found = tg::search (entries, query);
            const bool first = ! found.empty() && entries[(size_t) found[0].entry].guide == tg::Guide::singing;
            allFound = allFound && first;
        }

        const auto vibrato = tg::search (entries, "vibrato");
        check (allFound && ! vibrato.empty() && ! vibrato[0].topics.empty() && singing.topics[vibrato[0].topics[0]] == "Vibrato and straight tone",
               "theory guides: hoarse, vibrato, voice range and pop filter find the singing guide, vibrato at its Vibrato and straight tone topic");
    }
    check (tg::getEntry (tg::Guide::rhythm).topics.size() == (int) std::size (daw::gen::rhythm::allTopics)
               && tg::getEntry (tg::Guide::midi).topics.size() == (int) std::size (daw::midiguide::allTopics)
               && tg::getEntry (tg::Guide::gainStaging).topics.size() == (int) std::size (daw::gainstaging::allTopics),
           "theory guides: the rhythm, MIDI and gain staging guides list their own topics");

    // Every built-in effect and instrument explains itself, so each is in the hub, under its own name.
    auto plugins = daw::BuiltInFormat::getEffects();
    plugins.addArray (daw::BuiltInFormat::getInstruments());
    bool everyPlugin = true;

    for (auto& d : plugins)
        everyPlugin = everyPlugin && std::count_if (entries.begin(), entries.end(), [&d] (auto& e) { return e.plugin == d.fileOrIdentifier; }) == 1;

    check (everyPlugin, "theory guides: every built-in effect and instrument has one guide in the hub");
    check (std::count_if (entries.begin(), entries.end(), [] (auto& e) { return e.plugin.isNotEmpty(); }) == plugins.size(),
           "theory guides: no guide names a plugin that isn't built in");
    check (tg::getEntry (tg::Guide::physical).plugin == daw::BuiltInPhysical::identifier
               && tg::getEntry (tg::Guide::eq).plugin == daw::BuiltInEq::identifier,
           "theory guides: the plugin names are the built-ins' identifiers");

    const auto routing = tg::getEntry (tg::Guide::routing);
    check (routing.area == tg::Area::mixing && routing.name == "Buses, sends and routing" && routing.topics.size() == (int) std::size (daw::routingguide::allTopics)
               && routing.topics[0] == daw::routingguide::getTopicName (daw::routingguide::allTopics[0]),
           "theory guides: Buses, sends and routing is in Mixing, with its own topics");

    const auto path = tg::getStartPath();
    check (path.size() == 8 && path.front() == tg::Guide::sound && path[4] == tg::Guide::styles && path.back() == tg::Guide::compressor,
           "theory guides: the path for beginners starts with sound and ends with compression");

    // Searching.
    check (tg::getSearchWords ("What's a TRIPLET, and how do I count it?") == juce::StringArray ({ "triplet", "count" }),
           "theory guides: a question becomes its words, without the small ones");
    check (tg::search (entries, {}).size() == entries.size() && tg::search (entries, "  what is  ").size() == entries.size(),
           "theory guides: an empty search lists every guide");

    const auto first = [&entries] (const juce::String& query)
    {
        const auto found = tg::search (entries, query);
        return found.empty() ? juce::String() : entries[(size_t) found.front().entry].name;
    };

    check (first ("rhyme") == "How lyrics are written", "theory guides: \"rhyme\" finds the lyrics guide");
    check (first ("circle of fifths") == "Chord progressions", "theory guides: \"circle of fifths\" finds the chord guide");
    check (first ("muddy") == "What is EQ?", "theory guides: \"muddy\" finds the EQ guide");
    check (first ("muddy vocal") == "What is EQ?", "theory guides: the search field's example, \"muddy vocal\", finds the EQ guide");
    check (first ("Compress") == "What is compression?", "theory guides: a word's start finds a longer word, whatever its case");
    check (first ("triplets") == "Note lengths, beats and bars", "theory guides: a plural finds the word");
    check (first ("808 kick") == "What is drum synthesis?", "theory guides: every word must match");
    check (first ("dembow") == "Styles of music" && first ("new project style") == "Styles of music",
           "theory guides: dembow and new project style find Styles of music");
    check (first ("bus") == "Buses, sends and routing" && first ("folder") == "Buses, sends and routing" && first ("submix") == "Buses, sends and routing"
               && first ("group tracks") == "Buses, sends and routing",
           "theory guides: \"bus\", \"folder\", \"submix\" and \"group tracks\" find the buses guide");
    check (first ("mix bus") == "Buses, sends and routing" && first ("master bus") == "Buses, sends and routing" && first ("stereo bus") == "Buses, sends and routing"
               && first ("bus compressor") == "Buses, sends and routing" && first ("glue the mix") == "Buses, sends and routing",
           "theory guides: \"mix bus\", \"master bus\", \"stereo bus\", \"bus compressor\" and \"glue the mix\" find the buses guide");
    {
        const auto mixBus = tg::search (entries, "mix bus");
        const auto routingIndex = (int) std::distance (std::begin (tg::allGuides), std::find (std::begin (tg::allGuides), std::end (tg::allGuides), tg::Guide::routing));
        const auto topicIndex = entries[(size_t) routingIndex].topics.indexOf (daw::routingguide::getTopicName (daw::routingguide::Topic::mixBus));
        check (topicIndex >= 0 && std::any_of (mixBus.begin(), mixBus.end(), [&] (auto& m) { return m.entry == routingIndex; }),
               "theory guides: the buses guide lists the mix bus topic, taken from its own topic list");
    }
    check (first ("plate reverb") == "What is reverb?" && first ("spring reverb") == "What is reverb?"
               && first ("echo chamber") == "What is reverb?", "theory guides: plate reverb, spring reverb and echo chamber find the reverb guide first");
    check (first ("noise gate") == "What is a gate?" && first ("gated reverb") == "What is a gate?" && first ("expander") == "What is a gate?"
               && first ("bleed") == "What is a gate?" && first ("chatter") == "What is a gate?",
           "theory guides: \"noise gate\", \"gated reverb\", \"expander\", \"bleed\" and \"chatter\" find the gate guide first");
    {
        const auto gateEntry = tg::getEntry (tg::Guide::gate);
        const auto gatePath = tg::getStartPath();
        check (gateEntry.area == tg::Area::mixing && gateEntry.plugin == daw::BuiltInGate::identifier
                   && std::find (gatePath.begin(), gatePath.end(), tg::Guide::gate) == gatePath.end(),
               "theory guides: What is a gate? is in Mixing, belongs to DawIt Gate and is not on the path for beginners");
    }
    const auto startPath = tg::getStartPath();
    check (std::find (startPath.begin(), startPath.end(), tg::Guide::routing) == startPath.end(),
           "theory guides: the buses guide is not on the path for beginners");
    {
        const auto recording = tg::getEntry (tg::Guide::recording);
        check (recording.area == tg::Area::sound && recording.name == "How recording works" && recording.plugin.isEmpty()
                   && recording.topics.size() == (int) std::size (daw::recordingguide::allTopics)
                   && recording.topics[0] == daw::recordingguide::getTopicName (daw::recordingguide::Topic::air)
                   && recording.topics[recording.topics.size() - 1] == "Your session"
                   && std::find (startPath.begin(), startPath.end(), tg::Guide::recording) == startPath.end(),
               "theory guides: How recording works is in Sound with its nine topics and is not on the path for beginners");
        check (first ("phantom power") == "How recording works" && first ("latency") == "How recording works" && first ("comping") == "How recording works"
                   && first ("audio interface") == "How recording works" && first ("microphone howl") == "How recording works",
               "theory guides: \"phantom power\", \"latency\", \"comping\", \"audio interface\" and \"microphone howl\" find the recording guide first");
    }
    {
        const auto tuning = tg::getEntry (tg::Guide::tuning);
        check (tuning.area == tg::Area::sound && tuning.name == "How to tune a guitar" && tuning.plugin.isEmpty()
                   && tuning.topics.size() == (int) std::size (daw::gen::tuning::allTopics)
                   && tuning.topics[0] == "What a tuner shows"
                   && std::find (startPath.begin(), startPath.end(), tg::Guide::tuning) == startPath.end(),
               "theory guides: How to tune a guitar is in Sound with its ten topics and is not on the path for beginners");
        for (auto* query : { "guitar tuner", "dadgad", "ukulele", "432" })
            check (first (query) == "How to tune a guitar", "theory guides: \"" + juce::String (query) + "\" finds the tuning guide first");
        bool dropDFound = false;

        for (auto& m : tg::search (entries, "drop d"))
            dropDFound = dropDFound || entries[(size_t) m.entry].name == "How to tune a guitar";

        check (dropDFound, "theory guides: \"drop d\" finds the tuning guide among its matches");
    }
    {
        const auto triggers = tg::getEntry (tg::Guide::triggers);
        check (triggers.area == tg::Area::midi && triggers.name == "How drum triggers work" && triggers.plugin.isEmpty()
                   && triggers.topics.size() == (int) std::size (daw::triggerguide::allTopics)
                   && triggers.topics[triggers.topics.size() - 1] == "Your clip"
                   && std::find (startPath.begin(), startPath.end(), tg::Guide::triggers) == startPath.end(),
               "theory guides: How drum triggers work is in MIDI with its nine topics and is not on the path for beginners");
        for (auto* query : { "drum replacement", "transient", "drum trigger", "layering" })
            check (first (query) == "How drum triggers work", "theory guides: \"" + juce::String (query) + "\" finds the drum triggers guide first");
        bool bleedFound = false;

        for (auto& m : tg::search (entries, "bleed"))
            bleedFound = bleedFound || entries[(size_t) m.entry].name == "How drum triggers work";

        check (bleedFound, "theory guides: \"bleed\" finds the drum triggers guide among its matches");
    }
    {
        const auto found = tg::search (entries, "musical typing");
        check (! found.empty() && entries[(size_t) found[0].entry].guide == tg::Guide::midi, "theory guides: \"musical typing\" finds What is MIDI?");
    }
    check (tg::search (entries, "zebra").empty(), "theory guides: a word in no guide finds nothing");

    const auto rhymes = tg::search (entries, "rhyme");
    const auto lyrics = (int) std::distance (std::begin (tg::allGuides), std::find (std::begin (tg::allGuides), std::end (tg::allGuides), tg::Guide::lyrics));
    const auto rhymeTopic = entries[(size_t) lyrics].topics.indexOf (daw::gen::lyricsguide::getTopicName (daw::gen::lyricsguide::Topic::rhyme));
    check (! rhymes.empty() && std::find (rhymes.front().topics.begin(), rhymes.front().topics.end(), rhymeTopic) != rhymes.front().topics.end(),
           "theory guides: a search names the topic that matches");
    check (tg::search (entries, "physical modelling").front().topics.empty(),
           "theory guides: a guide that matches as a whole names no topic");
}

//==============================================================================
static void runEqAndCompressionGuideTests()
{
    // What is EQ?: the ranges, the problems and the numbers its text gives.
    const auto& regions = daw::eq::getRegions();
    bool joined = regions.size() == 7 && juce::exactlyEqual (regions.front().from, 20.0f) && juce::exactlyEqual (regions.back().to, 20000.0f);

    for (size_t i = 1; i < regions.size(); ++i)
        joined = joined && juce::exactlyEqual (regions[i].from, regions[i - 1].to);

    check (joined, "EQ guide: seven named ranges from 20 Hz to 20 kHz without gaps");

    const auto& problems = daw::eq::getProblems();
    bool problemsOk = problems.size() == 8;

    for (size_t i = 0; i < problems.size(); ++i)
        problemsOk = problemsOk && problems[i].from >= 20.0f && problems[i].to <= 20000.0f && problems[i].from < problems[i].to
                     && problems[i].fix.isNotEmpty() && (i == 0 || problems[i].from >= problems[i - 1].from);

    check (problemsOk, "EQ guide: the common problems are ranges inside 20 Hz to 20 kHz, low to high, each with a fix");
    check (std::abs (daw::eq::getBandwidthOctaves (1.41) - 1.0) < 0.02 && std::abs (daw::eq::getBandwidthOctaves (4.32) - 1.0 / 3.0) < 0.01
               && std::abs (daw::eq::getBandwidthOctaves (0.67) - 2.0) < 0.05,
           "EQ guide: Q 1.41 is an octave wide, Q 4.32 a third, Q 0.67 two octaves");

    const auto response = [] (int slope, float q, double hz)
    {
        return daw::eq::getResponseDb ({ true, daw::eq::BandType::lowCut, 100.0f, 0.0f, q, slope }, hz, 48000.0);
    };
    const double octave12 = response (12, daw::eq::flatQ, 25.0) - response (12, daw::eq::flatQ, 50.0);
    const double octave24 = response (24, daw::eq::flatQ, 25.0) - response (24, daw::eq::flatQ, 50.0);
    check (std::abs (response (12, daw::eq::flatQ, 100.0) + 3.0) < 0.2 && std::abs (octave12 + 12.0) < 0.5 && std::abs (octave24 + 24.0) < 1.0,
           "EQ guide: a 12 dB/oct low cut is -3 dB at its corner and falls 12 dB an octave below, a 24 dB/oct one 24 dB (measured "
               + juce::String (octave12, 1) + " and " + juce::String (octave24, 1) + ")");
    check (std::abs (response (12, 2.0f, 100.0) - 6.0) < 0.3, "EQ guide: a low cut with Q 2 bumps about +6 dB at its corner");

    // What is compression?: the curve's numbers its text gives.
    daw::comp::Settings hard;
    hard.thresholdDb = -18.0f;
    hard.ratio = 4.0f;
    hard.kneeDb = 0.0f;
    auto soft = hard;
    soft.kneeDb = 6.0f;
    check (std::abs (daw::comp::getReductionDb (hard, -10.0) - 6.0) < 1.0e-6, "compression guide: at 4:1, 8 dB over the threshold is turned down 6 dB");
    check (std::abs (daw::comp::getReductionDb (soft, -18.0) - 0.5625) < 0.01 && daw::comp::getReductionDb (hard, -18.0) < 1.0e-9,
           "compression guide: right at the threshold a 6 dB knee at 4:1 turns down about 0.6 dB, a hard knee nothing");
    check (std::abs (-6.0 - daw::comp::getReductionDb (hard, -6.0) + 15.0) < 1.0e-6 && daw::comp::getReductionDb (hard, -20.0) < 1.0e-9,
           "compression guide: with -18 dB and 4:1, a word at -6 dB comes out at -15 dB and one at -20 dB unchanged");

    double sumSquares = 0.0, peak = 0.0;

    for (int i = 0; i < 48000; ++i)
    {
        const double x = std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / 48000.0);
        sumSquares += x * x;
        peak = juce::jmax (peak, std::abs (x));
    }

    check (std::abs (juce::Decibels::gainToDecibels (std::sqrt (sumSquares / 48000.0) / peak) + 3.01) < 0.02,
           "compression guide: a steady sine measures 3 dB lower as RMS than as peak");
}

//==============================================================================
namespace
{
/** A song with buses, built through the engine: Drums (Kick, Snare, Hats), Vocals (Lead, Backing (BV 1, BV 2)) and Bass. */
struct BusTree
{
    BusTree()
    {
        using daw::Track;
        engine.deviceManager.removeAudioCallback (&engine);
        auto add = [this] (const char* name)
        {
            auto* t = engine.addTrack (Track::Type::midi);
            t->name = name;
            return t;
        };

        kick = add ("Kick");
        snare = add ("Snare");
        hats = add ("Hats");
        lead = add ("Lead");
        bv1 = add ("BV 1");
        bv2 = add ("BV 2");
        bass = add ("Bass");
        drums = engine.createBus ({ kick, snare, hats }, "Drums");
        backing = engine.createBus ({ bv1, bv2 }, "Backing");
        vocals = engine.createBus ({ lead, backing }, "Vocals");
    }

    juce::String order() const
    {
        juce::StringArray names;

        for (auto& t : engine.getTracks())
            names.add (t->name);

        return names.joinIntoString (", ");
    }

    static juce::String names (const std::vector<daw::Track*>& tracks)
    {
        juce::StringArray result;

        for (auto* t : tracks)
            result.add (t->name);

        return result.joinIntoString (", ");
    }

    bool tidy() const { return daw::routing::isTidy (engine.getTracks()); }

    daw::Engine engine;
    daw::Track *kick = nullptr, *snare = nullptr, *hats = nullptr, *lead = nullptr, *bv1 = nullptr, *bv2 = nullptr, *bass = nullptr,
               *drums = nullptr, *backing = nullptr, *vocals = nullptr;
};
} // namespace

static void runRoutingTests()
{
    using namespace daw;
    namespace r = daw::routing;
    std::cout << "Buses: routing" << std::endl;

    {
        BusTree t;
        const auto& tracks = t.engine.getTracks();

        check (t.order() == "Drums, Kick, Snare, Hats, Vocals, Lead, Backing, BV 1, BV 2, Bass" && t.tidy(),
               "buses: the tracks are listed with every bus followed by what is in it (" + t.order() + ")");
        const auto renderOrder = BusTree::names (r::getRenderOrder (tracks));
        std::cout << "  render order: " << renderOrder << std::endl;
        check (renderOrder == "Kick, Snare, Hats, Drums, Lead, BV 1, BV 2, Backing, Vocals, Bass", "buses: every track is rendered before the buses it plays into");

        {
            Engine flat;
            flat.deviceManager.removeAudioCallback (&flat);
            auto* a = flat.addTrack (Track::Type::audio);
            auto* b = flat.addTrack (Track::Type::midi);
            check (BusTree::names (r::getRenderOrder (flat.getTracks())) == a->name + ", " + b->name && r::isTidy (flat.getTracks()),
                   "buses: a song without buses renders in its edit order");
        }

        check (r::getDepth (*t.bv1) == 2 && r::getDepth (*t.vocals) == 0 && r::getDepth (*t.kick) == 1
                   && r::describePath (*t.bv1) == juce::String::fromUTF8 ("BV 1 \xe2\x86\x92 Backing \xe2\x86\x92 Vocals \xe2\x86\x92 Master")
                   && r::describePath (*t.bass) == juce::String::fromUTF8 ("Bass \xe2\x86\x92 Master"),
               "buses: depth and path (" + r::describePath (*t.bv1) + ")");
        check (r::playsInto (*t.bv1, *t.vocals) && ! r::playsInto (*t.kick, *t.vocals) && ! r::playsInto (*t.vocals, *t.vocals)
                   && BusTree::names (r::getMembers (*t.vocals, tracks)) == "Lead, Backing",
               "buses: playsInto follows the buses up, and getMembers lists a bus's own tracks");
        check (r::getBlockEnd (tracks, t.engine.indexOf (t.vocals)) == t.engine.indexOf (t.bass) && r::getBlockEnd (tracks, t.engine.indexOf (t.kick)) == t.engine.indexOf (t.snare),
               "buses: a bus's block runs to the next track outside it");

        // Folding.
        t.engine.setFolded (*t.backing, true);
        check (BusTree::names (r::getVisibleTracks (tracks)) == "Drums, Kick, Snare, Hats, Vocals, Lead, Backing, Bass", "buses: a folded bus hides its tracks");
        t.engine.setFolded (*t.vocals, true);
        check (BusTree::names (r::getVisibleTracks (tracks)) == "Drums, Kick, Snare, Hats, Vocals, Bass", "buses: a folded bus hides the buses in it, whatever their state");
        t.engine.setFolded (*t.vocals, false);
        t.engine.setFolded (*t.backing, false);
        std::cout << "  visible rows: " << BusTree::names (r::getVisibleTracks (tracks)) << std::endl;

        // Outermost and common bus.
        check (BusTree::names (r::getOutermost ({ t.drums, t.kick, t.bv1, t.bass }, tracks)) == "Drums, BV 1, Bass"
                   && r::findCommonBus ({ t.kick, t.hats }) == t.drums && r::findCommonBus ({ t.bv1, t.lead }) == t.vocals
                   && r::findCommonBus ({ t.kick, t.bass }) == nullptr && r::findCommonBus ({ t.drums, t.kick }) == nullptr,
               "buses: a selected bus takes its tracks along, and tracks have a closest common bus");
    }

    // Plugins, sounds and clips stay as they are when tracks are grouped.
    {
        BusTree t;
        juce::String error;
        auto* slot = t.engine.addPlugin (*t.kick, BuiltInEq::getDescription(), error);
        t.engine.setBuiltInSound (*t.snare, BuiltInSound::drums);
        t.engine.addMidiClip (*t.bass, 0.0, 4.0);
        const auto sound = t.snare->builtInSound.load();
        auto* made = t.engine.createBus ({ t.kick, t.snare, t.bass }, "Rhythm");
        check (made != nullptr && made->bus == nullptr && t.kick->plugins.size() == 1 && t.kick->plugins[0].get() == slot
                   && t.snare->builtInSound.load() == sound && t.bass->clips.size() == 1 && t.bass->bus == made && t.tidy(),
               "create bus: the tracks keep their plugins, sounds and clips");
        check (t.order() == "Rhythm, Kick, Snare, Bass, Drums, Hats, Vocals, Lead, Backing, BV 1, BV 2",
               "create bus: from tracks in different buses it goes to the top level (" + t.order() + ")");
    }

    // Create bus.
    {
        Engine flat;
        flat.deviceManager.removeAudioCallback (&flat);
        auto* kick = flat.addTrack (Track::Type::midi);
        auto* snare = flat.addTrack (Track::Type::midi);
        auto* hats = flat.addTrack (Track::Type::midi);
        kick->name = "Kick";
        snare->name = "Snare";
        hats->name = "Hats";
        auto* bus = flat.createBus ({ hats, kick });
        juce::StringArray names;

        for (auto& tr : flat.getTracks())
            names.add (tr->name);

        check (bus != nullptr && bus->name == "Bus 1" && names.joinIntoString (", ") == "Bus 1, Kick, Hats, Snare" && kick->bus == bus && hats->bus == bus
                   && snare->bus == nullptr && r::isTidy (flat.getTracks()),
               "create bus: the bus takes the first track's place and holds the tracks in order, and the next is called Bus 2 ("
                   + names.joinIntoString (", ") + ")");
        check (flat.createBus ({ snare })->name == "Bus 2" && flat.createBus ({}) == nullptr, "create bus: names count up, and no tracks make no bus");
    }

    {
        BusTree t;
        auto* made = t.engine.createBus ({ t.snare, t.bass });
        check (made != nullptr && made->name == "Bus 1" && made->bus == nullptr && t.order() == "Bus 1, Snare, Bass, Drums, Kick, Hats, Vocals, Lead, Backing, BV 1, BV 2"
                   && BusTree::names (r::getMembers (*t.drums, t.engine.getTracks())) == "Kick, Hats" && t.tidy(),
               "create bus: from a track inside Drums and one outside it, the bus goes before Drums at the top level (" + t.order() + ")");
    }

    {
        BusTree t;
        auto* made = t.engine.createBus ({ t.drums, t.kick });
        check (made != nullptr && t.drums->bus == made && t.kick->bus == t.drums && t.order().startsWith ("Bus 1, Drums, Kick, Snare, Hats, Vocals") && t.tidy(),
               "create bus: from a bus and a track in it, only the bus moves");
        auto* inside = t.engine.createBus ({ t.snare, t.hats });
        check (inside != nullptr && inside->bus == t.drums && t.order().startsWith ("Bus 1, Drums, Kick, Bus 2, Snare, Hats, Vocals") && t.tidy(),
               "create bus: from two tracks in one bus, the new bus goes inside that bus (" + t.order() + ")");
    }

    // Moving.
    {
        BusTree t;
        check (t.engine.moveTrack (*t.bv1, nullptr, t.bass) && t.order() == "Drums, Kick, Snare, Hats, Vocals, Lead, Backing, BV 2, BV 1, Bass" && t.tidy(),
               "move: a track goes out to the master, before another");
        check (t.engine.moveTrack (*t.bv1, t.drums) && t.order() == "Drums, Kick, Snare, Hats, BV 1, Vocals, Lead, Backing, BV 2, Bass" && t.bv1->bus == t.drums && t.tidy(),
               "move: then into Drums at the end");
        check (t.engine.moveTrack (*t.bv1, t.vocals, t.lead) && t.order() == "Drums, Kick, Snare, Hats, Vocals, BV 1, Lead, Backing, BV 2, Bass" && t.tidy(),
               "move: then into Vocals before Lead");
        const auto before = t.order();
        check (! t.engine.moveTrack (*t.vocals, t.backing) && ! t.engine.moveTrack (*t.vocals, t.vocals) && ! t.engine.moveTrack (*t.kick, t.kick)
                   && ! t.engine.moveTrack (*t.kick, t.drums, t.lead) && ! t.engine.moveTrack (*t.kick, t.bass) && t.order() == before && t.tidy(),
               "move: a bus can't go into itself or inside its own tracks, a track can't go before one of another bus, and nothing changes");
        check (t.engine.moveTrack (*t.vocals, nullptr, nullptr) && t.order() == "Drums, Kick, Snare, Hats, Bass, Vocals, BV 1, Lead, Backing, BV 2" && t.tidy(),
               "move: a bus takes everything in it along");
    }

    {
        BusTree t;
        check (t.engine.removeFromBus (*t.bv2) && t.bv2->bus == t.vocals && t.order() == "Drums, Kick, Snare, Hats, Vocals, Lead, Backing, BV 1, BV 2, Bass" && t.tidy()
                   && ! t.engine.removeFromBus (*t.bass),
               "remove from bus: BV 2 goes into Vocals, right below Backing (" + t.order() + ")");
    }

    {
        BusTree t;
        juce::String error;
        t.engine.addPlugin (*t.backing, BuiltInEq::getDescription(), error);
        t.engine.ungroup (*t.backing);
        check (t.order() == "Drums, Kick, Snare, Hats, Vocals, Lead, BV 1, BV 2, Bass" && t.bv1->bus == t.vocals && t.bv2->bus == t.vocals && t.tidy(),
               "ungroup: Backing's tracks take its place inside Vocals, and Backing and its EQ are gone");
        t.engine.ungroup (*t.drums);
        check (t.order() == "Kick, Snare, Hats, Vocals, Lead, BV 1, BV 2, Bass" && t.kick->bus == nullptr && t.tidy(), "ungroup: Drums's tracks go to the top level");
    }

    // addTrack's rule.
    {
        BusTree t;
        auto added = [&t] (int index)
        {
            auto* n = t.engine.addTrack (Track::Type::audio, index);
            n->name = "New";
            return n;
        };

        auto* a = added (t.engine.indexOf (t.kick) + 1);
        check (a->bus == t.drums && t.order() == "Drums, Kick, New, Snare, Hats, Vocals, Lead, Backing, BV 1, BV 2, Bass" && t.tidy(), "add track: below Kick, in Drums, right after Kick");
        t.engine.removeTrack (a);

        a = added (t.engine.indexOf (t.hats) + 1);
        check (a->bus == t.drums && t.order() == "Drums, Kick, Snare, Hats, New, Vocals, Lead, Backing, BV 1, BV 2, Bass" && t.tidy(), "add track: below the last track of Drums, in Drums");
        t.engine.removeTrack (a);

        a = added (t.engine.indexOf (t.drums) + 1);
        check (a->bus == nullptr && t.order() == "Drums, Kick, Snare, Hats, New, Vocals, Lead, Backing, BV 1, BV 2, Bass" && t.tidy(),
               "add track: below a bus is below everything in it, at its level");
        t.engine.removeTrack (a);

        a = added (t.engine.indexOf (t.bv2) + 1);
        check (a->bus == t.backing && t.order() == "Drums, Kick, Snare, Hats, Vocals, Lead, Backing, BV 1, BV 2, New, Bass" && t.tidy(), "add track: below BV 2, in Backing");
        t.engine.removeTrack (a);

        a = added (0);
        check (a->bus == nullptr && t.order().startsWith ("New, Drums") && t.tidy(), "add track: at 0 it is first");
        t.engine.removeTrack (a);
        a = added (-1);
        check (a->bus == nullptr && t.order().endsWith ("Bass, New") && t.tidy(), "add track: at -1 it is last at the top level");

        auto* bus = t.engine.addBus (t.engine.indexOf (t.kick) + 1);
        check (bus->isBus() && bus->bus == t.drums && bus->name == "Bus 1" && t.tidy(), "add bus: placed as a track is, and named Bus 1");
    }

    {
        BusTree t;
        t.engine.removeTrack (t.vocals);
        check (t.order() == "Drums, Kick, Snare, Hats, Bass" && t.tidy(), "remove: a bus takes everything in it with it");
        t.engine.removeTrack (t.kick);
        check (t.order() == "Drums, Snare, Hats, Bass" && t.tidy(), "remove: a track in a bus goes alone");
    }

    // Plugins.
    {
        BusTree t;
        juce::String error;
        auto* eq = t.engine.addPlugin (*t.drums, BuiltInEq::getDescription(), error);
        auto* synth = t.engine.addPlugin (*t.drums, BuiltInSubtractive::getDescription(), error);
        check (eq != nullptr && synth == nullptr && error.contains ("no instrument") && t.drums->plugins.size() == 1,
               "bus plugins: an effect goes on a bus, an instrument is refused (\"" + error + "\")");
    }

    // Solo and mute.
    {
        BusTree t;
        const auto& tracks = t.engine.getTracks();
        t.kick->soloed = true;
        check (r::isOnSoloPath (*t.kick, tracks) && r::isOnSoloPath (*t.drums, tracks) && ! r::isOnSoloPath (*t.snare, tracks) && ! r::isOnSoloPath (*t.bass, tracks)
                   && r::getSilence (*t.snare, tracks) == r::Silence::solo && r::getSilence (*t.kick, tracks) == r::Silence::none,
               "solo: a soloed track keeps its bus open, and the others are silent");
        t.kick->soloed = false;
        t.drums->soloed = true;
        check (r::isOnSoloPath (*t.kick, tracks) && r::isOnSoloPath (*t.hats, tracks) && ! r::isOnSoloPath (*t.lead, tracks),
               "solo: a soloed bus plays its tracks");
        t.drums->soloed = false;
        t.drums->muted = true;
        check (r::getSilence (*t.kick, tracks) == r::Silence::busMuted && r::getSilence (*t.drums, tracks) == r::Silence::muted && r::getSilence (*t.bass, tracks) == r::Silence::none,
               "mute: a muted bus silences its tracks");
    }

    // Dropping between rows.
    {
        BusTree t;
        auto rows = r::getVisibleTracks (t.engine.getTracks());
        auto place = [&rows] (int gap, int level) { return r::getPlace (rows, gap, level); };

        check (r::getGapLevels (rows, 4) == std::pair<int, int> { 0, 1 } && place (4, 1).bus == t.drums && place (4, 1).before == nullptr
                   && place (4, 0).bus == nullptr && place (4, 0).before == t.vocals,
               "drop: below Hats the level picks Drums or the master");
        check (r::getGapLevels (rows, 1) == std::pair<int, int> { 1, 1 } && place (1, 1).bus == t.drums && place (1, 1).before == t.kick, "drop: below the open Drums row it goes inside, first");
        check (r::getGapLevels (rows, 9) == std::pair<int, int> { 0, 2 } && place (9, 2).bus == t.backing && place (9, 2).before == nullptr
                   && place (9, 1).bus == t.vocals && place (9, 1).before == nullptr && place (9, 0).bus == nullptr && place (9, 0).before == t.bass,
               "drop: below BV 2 the level picks Backing, Vocals or the master");
        check (r::getGapLevels (rows, 10) == std::pair<int, int> { 0, 0 } && place (10, 0).bus == nullptr && place (10, 0).before == nullptr,
               "drop: below the last row it goes last at the master");

        t.engine.setFolded (*t.drums, true);
        rows = r::getVisibleTracks (t.engine.getTracks());
        check (r::getGapLevels (rows, 1) == std::pair<int, int> { 0, 0 } && place (1, 0).bus == nullptr && place (1, 0).before == t.vocals,
               "drop: below a folded bus there is only its own level");
    }
}

//==============================================================================
namespace
{
/** Audio tracks A and B (440 Hz) and C (660 Hz), each a sine peaking at 0.25, with D (440 Hz) outside the bus. */
struct BusSong
{
    explicit BusSong (bool startDevice = true)
    {
        using daw::Track;
        engine.deviceManager.removeAudioCallback (&engine);

        if (startDevice)
            engine.audioDeviceAboutToStart (&device);

        a = makeTrack ("A", 440.0f);
        b = makeTrack ("B", 440.0f);
        c = makeTrack ("C", 660.0f);
        d = makeTrack ("D", 440.0f);
        c->muted = true;
        d->muted = true;
        bus = engine.createBus ({ a, b }, "Group");
    }

    daw::Track* makeTrack (const char* name, float hz)
    {
        auto* t = engine.addTrack (daw::Track::Type::audio);
        t->name = name;
        juce::AudioBuffer<float> sine (1, (int) (testRate * 6));

        for (int i = 0; i < sine.getNumSamples(); ++i)
            sine.setSample (0, i, 0.25f * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) testRate));

        engine.addAudioClip (*t, std::move (sine), testRate, 0.0, name);
        return t;
    }

    /** Plays from the start and returns half a second of sound after the fades have settled. */
    juce::AudioBuffer<float> hear (double seconds = 0.5)
    {
        engine.stop();
        render (engine, 0.05);
        engine.setPositionBeats (0.0);
        engine.play();
        render (engine, 0.3);
        return render (engine, seconds);
    }

    static float peak (const juce::AudioBuffer<float>& b) { return b.getMagnitude (0, b.getNumSamples()); }

    daw::Engine engine;
    FakeDevice device;
    daw::Track *a = nullptr, *b = nullptr, *c = nullptr, *d = nullptr, *bus = nullptr;
};
} // namespace

static void runBusAudioTests()
{
    using namespace daw;
    std::cout << "Buses: sound" << std::endl;

    {
        BusSong s;
        s.bus->busInputPeak = 0.0f;
        auto out = s.hear();
        const float summed = BusSong::peak (out);
        check (std::abs (summed - 0.5f) < 0.01f && std::abs (s.bus->busInputPeak.load() - 0.5f) < 0.01f,
               "bus sound: two identical sines add to a peak of 0.50 (" + juce::String (summed, 3) + ") and the bus's input measures it");

        // A different sound: +3.01 dB, not +6.
        s.b->muted = true;
        s.engine.moveTrack (*s.c, s.bus);
        s.c->muted = false;
        out = s.hear (1.0);
        const float r = rms (out, 0);
        check (std::abs (r - 0.25f) < 0.005f, "bus sound: two different sines at RMS 0.177 add to RMS 0.250, +3.01 dB (" + juce::String (r, 4) + ")");
        s.c->muted = true;
        s.engine.moveTrack (*s.c, nullptr);
        s.b->muted = false;

        // The bus's fader moves the group; the tracks' meters are before it.
        s.bus->volumeDb = -6.0f;
        out = s.hear();
        check (std::abs (BusSong::peak (out) - 0.5f * 0.501f) < 0.01f && s.a->peakLeft.load() > 0.2f && s.a->peakLeft.load() < 0.26f,
               "bus sound: the bus at -6 dB brings the group to 0.251, and the tracks' own meters still read about 0.25 (" + juce::String (BusSong::peak (out), 3) + ")");
        s.bus->volumeDb = 0.0f;

        // Pan on the group's mix.
        s.bus->pan = -1.0f;
        out = s.hear();
        check (rms (out, 0) > 0.1f && rms (out, 1) < 1.0e-4f, "bus sound: a bus panned hard left leaves the right channel silent");
        s.bus->pan = 0.0f;

        // Mute.
        s.bus->muted = true;
        out = s.hear();
        check (rms (out, 0) < 1.0e-4f && s.a->peakLeft.load() > 0.2f, "bus sound: a muted bus is silent, while its tracks' meters still move");
        s.bus->muted = false;

        // Solo.
        s.d->muted = false;
        s.a->soloed = true;
        out = s.hear();
        check (std::abs (BusSong::peak (out) - 0.25f) < 0.01f, "bus sound: a soloed track in a bus is heard alone, through its bus (" + juce::String (BusSong::peak (out), 3) + ")");
        s.a->soloed = false;
        s.bus->soloed = true;
        out = s.hear();
        check (std::abs (BusSong::peak (out) - 0.5f) < 0.01f, "bus sound: a soloed bus plays all its tracks and not the track outside it (" + juce::String (BusSong::peak (out), 3) + ")");
        s.bus->soloed = false;
        s.d->muted = true;

        // An effect on the bus works on the sum.
        juce::String error;
        auto* slot = s.engine.addPlugin (*s.bus, BuiltInEq::getDescription(), error);
        auto* eqPlugin = slot != nullptr ? dynamic_cast<BuiltInEq*> (slot->instance.get()) : nullptr;

        if (eqPlugin != nullptr)
        {
            eqPlugin->setBand (2, { true, eq::BandType::bell, 440.0f, 6.0f, 1.0f, 12 });
            slot->inputPeak = 0.0f;
            out = s.hear();
            check (std::abs (BusSong::peak (out) - 1.0f) < 0.05f && std::abs (slot->inputPeak.load() - 0.5f) < 0.02f,
                   "bus sound: a +6 dB bell on the bus doubles the sum, and the effect's input reads 0.50 (" + juce::String (BusSong::peak (out), 3) + ", "
                       + juce::String (slot->inputPeak.load(), 3) + ")");
            s.engine.removePlugin (*s.bus, slot);
        }
        else
        {
            check (false, "bus sound: DawIt EQ goes on a bus " + error);
        }

        // Nesting.
        s.b->muted = true;
        auto* outer = s.engine.createBus ({ s.bus }, "Outer");
        s.bus->volumeDb = -6.0f;
        outer->volumeDb = -6.0f;
        out = s.hear();
        check (std::abs (BusSong::peak (out) - 0.25f * 0.501f * 0.501f) < 0.005f, "bus sound: a bus in a bus multiplies the faders: 0.063 (" + juce::String (BusSong::peak (out), 4) + ")");
        s.bus->volumeDb = 0.0f;
        s.engine.ungroup (*outer);
        s.b->muted = false;

        // Changing the routing while it plays.
        s.engine.setPositionBeats (0.0);
        s.engine.play();
        render (s.engine, 0.2);
        s.engine.moveTrack (*s.b, nullptr);
        render (s.engine, 0.1);
        s.engine.moveTrack (*s.b, s.bus);
        render (s.engine, 0.1);
        s.engine.moveTrack (*s.b, nullptr);
        s.bus->busInputPeak = 0.0f;
        render (s.engine, 0.3);
        out = render (s.engine, 0.3);
        check (std::abs (BusSong::peak (out) - 0.5f) < 0.01f && std::abs (s.bus->busInputPeak.load() - 0.25f) < 0.01f && routing::isTidy (s.engine.getTracks()),
               "bus sound: moving a track in and out while playing doesn't crash, and the peaks follow (0.25 from the bus, 0.50 in all)");
        s.engine.ungroup (*s.bus);
        render (s.engine, 0.2);
        out = render (s.engine, 0.3);
        check (std::abs (BusSong::peak (out) - 0.5f) < 0.01f && s.a->bus == nullptr, "bus sound: ungrouping while playing keeps the sound");

        // An empty bus costs nothing.
        auto* empty = s.engine.addBus();
        out = s.hear();
        check (std::abs (BusSong::peak (out) - 0.5f) < 0.01f && empty->busInputPeak.load() < 1.0e-6f && empty->peakLeft.load() < 1.0e-6f,
               "bus sound: an empty bus is silent and its meters stay at 0");
        s.engine.stop();
    }

    // Bouncing goes through the buses, too.
    {
        const auto folder = juce::File::createTempFile ("busbounce");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };

        BusSong s (false);
        s.bus->volumeDb = -6.0f;
        bounce::Settings settings;
        settings.startBeat = 0.0;
        settings.endBeat = 2.0;
        const auto outcome = bounce::bounceToFile (s.engine, settings, folder.getChildFile ("Bus.wav"));
        check (outcome.result.wasOk() && std::abs (outcome.peak - 0.5f * 0.501f) < 0.01f, "bus sound: a bounce with the bus at -6 dB peaks at 0.251 (" + juce::String (outcome.peak, 3) + ")");

        BusSong muted (false);
        muted.bus->muted = true;
        const auto silent = bounce::bounceToFile (muted.engine, settings, folder.getChildFile ("Muted.wav"));
        check (silent.result.wasOk() && silent.peak < 1.0e-5f, "bus sound: a bounce with the bus muted is silent");
    }

    // Plugin delay is not compensated yet; the number is on record.
    {
        BusSong s;
        juce::String error;
        auto* slot = s.engine.addPlugin (*s.d, BuiltInSaturation::getDescription(), error);
        const int latency = slot != nullptr ? slot->instance->getLatencySamples() : 0;
        std::cout << "  DawIt Saturation reports " << latency << " samples of latency (" << juce::String (1000.0 * latency / testRate, 3) << " ms): not compensated yet" << std::endl;
        check (latency > 0, "bus sound: DawIt Saturation reports a latency, which is not compensated yet");
    }
}

//==============================================================================
//==============================================================================
namespace
{
/** Audio tracks with sines: A and B (440 Hz) and C (660 Hz), and a Reverb return. */
struct SendSong
{
    explicit SendSong (float amplitude = 0.5f)
    {
        using daw::Track;
        engine.deviceManager.removeAudioCallback (&engine);
        engine.audioDeviceAboutToStart (&device);
        a = makeTrack ("A", 440.0f, amplitude);
        b = makeTrack ("B", 440.0f, amplitude);
        c = makeTrack ("C", 660.0f, amplitude);
        ret = engine.addReturn ("Reverb");
    }

    daw::Track* makeTrack (const char* name, float hz, float amplitude)
    {
        auto* t = engine.addTrack (daw::Track::Type::audio);
        t->name = name;
        juce::AudioBuffer<float> sine (1, (int) (testRate * 8));

        for (int i = 0; i < sine.getNumSamples(); ++i)
            sine.setSample (0, i, amplitude * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) testRate));

        engine.addAudioClip (*t, std::move (sine), testRate, 0.0, name);
        return t;
    }

    /** Plays from the start; the return's input peak is measured over half a second after the fades have settled. */
    juce::AudioBuffer<float> hear (double seconds = 0.5)
    {
        engine.stop();
        render (engine, 0.05);
        engine.setPositionBeats (0.0);
        engine.play();
        render (engine, 0.3);
        ret->busInputPeak = 0.0f;
        return render (engine, seconds);
    }

    static float peak (const juce::AudioBuffer<float>& buffer) { return buffer.getMagnitude (0, buffer.getNumSamples()); }
    float returnPeak() const { return ret->busInputPeak.load(); }

    daw::Engine engine;
    FakeDevice device;
    daw::Track *a = nullptr, *b = nullptr, *c = nullptr, *ret = nullptr;
};
} // namespace

static void runSendRoutingTests()
{
    using namespace daw;
    namespace r = daw::routing;
    std::cout << "Sends and returns: routing, engine and saving" << std::endl;

    // Routing rules.
    {
        BusTree t;
        auto* reverb = t.engine.addReturn ("Reverb");
        auto* bass2 = t.engine.addTrack (Track::Type::midi);
        bass2->name = "Bass 2";
        check (reverb != nullptr && reverb->isReturn() && reverb->hasInput() && ! reverb->isBus() && t.engine.getTracks().back().get() == reverb
                   && t.engine.indexOf (bass2) < t.engine.indexOf (reverb),
               "returns: a return goes last, and a track added later goes above it");
        check (! r::canMoveInto (*t.bass, reverb) && ! r::canMoveInto (*reverb, t.drums) && r::canMoveInto (*reverb, nullptr) && r::canMoveInto (*t.bass, t.drums),
               "returns: nothing goes into a return, and a return doesn't go into a bus");
        check (! t.engine.moveTrack (*t.bass, reverb) && ! t.engine.moveTrack (*reverb, t.drums), "returns: the engine refuses those moves");
        const auto order = BusTree::names (r::getRenderOrder (t.engine.getTracks()));
        check (order.endsWith ("Reverb") && order.startsWith ("Kick"), "returns: the render order ends with the returns (" + order + ")");

        t.engine.setSend (*t.kick, *reverb, -10.0f);
        t.engine.setSend (*t.bass, *reverb, -6.0f);
        t.engine.setSend (*t.kick, *reverb, -8.0f);
        check (t.kick->sends.size() == 1 && juce::exactlyEqual (t.kick->sends[0]->levelDb.load(), -8.0f) && r::isSendOn (*t.kick, *reverb)
                   && BusTree::names (r::getSenders (*reverb, t.engine.getTracks())) == "Kick, Bass",
               "sends: setSend twice keeps one send, and getSenders lists the tracks that send");
        t.bass->sends[0]->levelDb = sendOffDb;
        check (BusTree::names (r::getSenders (*reverb, t.engine.getTracks())) == "Kick" && r::findSend (*t.bass, *reverb) != nullptr && ! r::isSendOn (*t.bass, *reverb),
               "sends: a send at Off is ignored by getSenders");
        check (! t.engine.setSend (*reverb, *reverb, -5.0f) && ! t.engine.setSend (*t.kick, *t.drums, -5.0f), "sends: only a return can be sent to, and a return doesn't send");
        check (r::describePath (*t.kick) == juce::String::fromUTF8 ("Kick \xe2\x86\x92 Drums \xe2\x86\x92 Master (+ sends to Reverb)"),
               "sends: the path says where the sends go (" + r::describePath (*t.kick) + ")");

        // Solo and mute, as the headers and the guide read them.
        t.snare->soloed = true;
        check (r::getSilence (*reverb, t.engine.getTracks()) == r::Silence::solo, "solo: a return is silent while a track that doesn't send to it is soloed");
        t.engine.setSend (*t.snare, *reverb, -10.0f);
        check (r::getSilence (*reverb, t.engine.getTracks()) == r::Silence::none && r::isOnSoloPath (*reverb, t.engine.getTracks()),
               "solo: a return is heard while a soloed track sends to it");
        t.snare->soloed = false;
        reverb->soloed = true;
        check (r::getSilence (*t.kick, t.engine.getTracks()) == r::Silence::solo && r::getSilence (*reverb, t.engine.getTracks()) == r::Silence::none,
               "solo: soloing a return silences the tracks' own outputs");
        reverb->soloed = false;

        // Make a bus with a return in the selection.
        auto* made = t.engine.createBus ({ t.bass, reverb }, "Rhythm");
        check (made != nullptr && reverb->bus == nullptr && t.bass->bus == made && r::isTidy (t.engine.getTracks()) && t.engine.getTracks().back().get() == reverb,
               "create bus: a return in the selection stays at the top level");

        // Removing a return takes the sends to it.
        t.engine.removeTrack (reverb);
        check (t.kick->sends.empty() && t.bass->sends.empty() && t.snare->sends.empty() && r::getReturns (t.engine.getTracks()).empty(),
               "returns: removing a return removes the sends to it");
    }

    // An instrument is refused on a return.
    {
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        auto* ret = song.addReturn();
        juce::String error;
        check (song.addPlugin (*ret, BuiltInSubtractive::getDescription(), error) == nullptr && error.contains ("no instrument")
                   && ret->name == "Return 1" && song.addReturn ("Reverb")->name == "Reverb" && song.addReturn ("Reverb")->name == "Reverb 2",
               "returns: an instrument is refused, and names are made unique (Return 1, Reverb, Reverb 2)");
    }

    // The send level for a mix.
    {
        auto near = [] (float a, float b) { return std::abs (a - b) < 0.01f; };
        namespace rg = daw::routingguide;
        check (near (rg::getSendDbForMix (15.0f), -12.39f) && near (rg::getSendDbForMix (18.0f), -10.74f) && near (rg::getSendDbForMix (20.0f), -9.76f)
                   && near (rg::getSendDbForMix (22.0f), -8.87f) && near (rg::getSendDbForMix (25.0f), -7.66f) && near (rg::getSendDbForMix (35.0f), -4.25f)
                   && near (rg::getSendDbForMix (45.0f), -1.37f) && near (rg::getSendDbForMix (50.0f), 0.0f),
               "sends: the send level that keeps an insert's balance is 20 log10 (tan (mix x 90 degrees)): 20% is -9.76 dB");
        check (juce::exactlyEqual (rg::getSendDbForMix (0.0f), -60.0f) && juce::exactlyEqual (rg::getSendDbForMix (80.0f), 6.0f) && juce::exactlyEqual (rg::getSendDbForMix (100.0f), 6.0f),
               "sends: a mix of 0 is Off, and 80% and more is limited to +6 dB");
        check (near (rg::getDryChangeDb (20.0f), 0.44f) && near (rg::getDryChangeDb (45.0f), 2.38f) && std::abs (fx::getMixGains (100.0f).first) < 1.0e-6f,
               "sends: moving from a 20% insert to a send makes the dry sound 0.44 dB louder, and 100% mix has no dry sound");
    }

    // Starting points.
    {
        const auto& reverbs = verb::getPresets();
        const auto& delays = dly::getPresets();
        int numReverb = 0, numDelay = 0;
        bool filled = true;

        for (auto& p : reverbs)
            if (p.name.startsWith ("Return:"))
            {
                ++numReverb;
                filled = filled && juce::exactlyEqual (p.settings.mixPercent, 100.0f) && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& n) { return n.isNotEmpty(); }) && p.why.isNotEmpty();
            }

        for (auto& p : delays)
            if (p.name.startsWith ("Return:"))
            {
                ++numDelay;
                filled = filled && juce::exactlyEqual (p.settings.mixPercent, 100.0f) && std::all_of (p.notes.begin(), p.notes.end(), [] (auto& n) { return n.isNotEmpty(); }) && p.why.isNotEmpty();
            }

        check (numReverb == 3 && numDelay == 2 && filled, "return starting points: three reverbs and two delays, each at 100% mix with every note filled in");
        check (verb::getReturnPreset().name == "Return: plate" && dly::getReturnPreset().name == "Return: quarter echo" && std::abs (verb::getRoomMetres (25.0f) - 8.0) < 0.5,
               "return starting points: the defaults are Return: plate and Return: quarter echo, and 25% is about 8 m");
    }

    // Saving.
    {
        namespace project = daw::project;
        const auto folder = juce::File::createTempFile ("sends");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
        const auto file = folder.getChildFile ("Sends.dawit");

        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        auto* vocal = song.addTrack (Track::Type::midi);
        vocal->name = "Vocal";
        auto* guitar = song.addTrack (Track::Type::midi);
        guitar->name = "Guitar";
        auto* drum1 = song.addTrack (Track::Type::midi);
        auto* drum2 = song.addTrack (Track::Type::midi);
        auto* drums = song.createBus ({ drum1, drum2 }, "Drums");
        auto* reverb = song.addReturn ("Reverb");
        auto* delay = song.addReturn ("Delay");
        song.setSend (*vocal, *reverb, -12.0f);
        song.setSend (*guitar, *reverb, -9.5f);
        song.setSend (*guitar, *delay, -14.0f);
        song.setSend (*drums, *reverb, -7.0f);
        r::findSend (*guitar, *reverb)->preFader = true;
        song.setSend (*vocal, *delay, -10.0f);
        r::findSend (*vocal, *delay)->levelDb = sendOffDb;
        reverb->volumeDb = -3.0f;

        juce::String error, text;
        juce::String pluginError;
        song.addPlugin (*reverb, BuiltInReverb::getDescription(), pluginError);
        check (project::save (song, file, text, error), "a project with returns saves " + error);
        juce::var parsed;
        juce::JSON::parse (text, parsed);
        auto* top = parsed["tracks"].getArray();
        const auto& guitarJson = top != nullptr && top->size() >= 4 ? (*top)[1] : juce::var();
        const auto& reverbJson = top != nullptr && top->size() >= 4 ? (*top)[top->size() - 2] : juce::var();
        check (text.contains ("\"version\": 3") && reverbJson["type"].toString() == "return" && ! reverbJson.hasProperty ("clips")
                   && guitarJson["sends"].getArray() != nullptr && guitarJson["sends"].getArray()->size() == 2
                   && (*guitarJson["sends"].getArray())[0]["return"].operator int() == 0 && (bool) (*guitarJson["sends"].getArray())[0]["preFader"]
                   && ! (*guitarJson["sends"].getArray())[1].hasProperty ("preFader") && ! (*top)[2].hasProperty ("clips") && text.indexOf ("\"levelDb\": -60") < 0,
               "returns are saved as version 3, at the top level with no clips, and sends as the index of the return, the level and preFader when true; Off isn't saved");

        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        juce::StringArray warnings;
        const bool ok = project::load (loaded, file, error, warnings);
        const auto returnsLoaded = r::getReturns (loaded.getTracks());
        check (ok && warnings.isEmpty() && project::toText (loaded, file) == text && returnsLoaded.size() == 2 && returnsLoaded[0]->name == "Reverb"
                   && r::getSenders (*returnsLoaded[0], loaded.getTracks()).size() == 3 && r::getSenders (*returnsLoaded[1], loaded.getTracks()).size() == 1
                   && r::isTidy (loaded.getTracks()) && juce::exactlyEqual (returnsLoaded[0]->volumeDb.load(), -3.0f) && returnsLoaded[0]->plugins.size() == 1,
               "a project with returns loads with its sends, and saves to the same text " + error);

        // Version 2 with a bus only; version 1 with neither.
        song.removeTrack (reverb);
        song.removeTrack (delay);
        project::save (song, file, text, error);
        check (text.contains ("\"version\": 2") && ! text.contains ("\"sends\""), "a song with a bus but no return is still saved as version 2");
        song.removeTrack (drums);
        project::save (song, file, text, error);
        check (text.contains ("\"version\": 1"), "a song with neither is still saved as version 1");

        // A send to a return that isn't there.
        file.replaceWithText ("{ \"format\": \"DawIt project\", \"version\": 3, \"tracks\": ["
                              "{ \"type\": \"midi\", \"name\": \"Lead\", \"plugins\": [], \"clips\": [], \"sends\": [ { \"return\": 3, \"levelDb\": -6 }, { \"return\": 0, \"levelDb\": -6 } ] },"
                              "{ \"type\": \"return\", \"name\": \"Room\", \"plugins\": [] } ] }");
        warnings.clear();
        Engine odd;
        odd.deviceManager.removeAudioCallback (&odd);
        const bool oddOk = project::load (odd, file, error, warnings);
        check (oddOk && warnings.size() == 1 && warnings[0].contains ("Lead") && odd.getTracks().size() == 2 && odd.getTracks()[0]->sends.size() == 1,
               "a send to a missing return loads with a warning, and the other send stays");
    }
}

static void runSendAudioTests()
{
    using namespace daw;
    std::cout << "Sends and returns: sound" << std::endl;

    auto close = [] (float a, float b) { return std::abs (a - b) <= 0.01f * b; };

    {
        SendSong s;
        s.b->muted = s.c->muted = true;
        s.engine.setSend (*s.a, *s.ret, -6.0206f);
        s.b->muted = true;
        auto out = s.hear();
        check (close (s.returnPeak(), 0.25f) && close (SendSong::peak (out), 0.75f),
               "send sound: a sine at 0.5 sent at -6.02 dB gives the return 0.25, and the master track plus return, 0.75 ("
                   + juce::String (s.returnPeak(), 3) + ", " + juce::String (SendSong::peak (out), 3) + ")");

        // The fader.
        s.a->volumeDb = -6.0206f;
        out = s.hear();
        const float post = s.returnPeak();
        s.a->sends[0]->preFader = true;
        out = s.hear();
        const float pre = s.returnPeak();
        check (close (post, 0.125f) && close (pre, 0.25f), "send sound: with the fader at -6 dB, a post-fader send gives 0.125 and a pre-fader send 0.25 ("
                                                           + juce::String (post, 3) + ", " + juce::String (pre, 3) + ")");

        // Mute: pre or post, and a muted bus.
        s.a->muted = true;
        out = s.hear();
        const float mutedPre = s.returnPeak();
        s.a->sends[0]->preFader = false;
        out = s.hear();
        check (mutedPre < 1.0e-6f && s.returnPeak() < 1.0e-6f, "send sound: a muted track sends nothing, pre-fader or post-fader");
        s.a->muted = false;
        s.a->volumeDb = 0.0f;
        auto* group = s.engine.createBus ({ s.a }, "Group");
        group->muted = true;
        out = s.hear();
        check (s.returnPeak() < 1.0e-6f, "send sound: a track in a muted bus sends nothing");
        group->muted = false;
        out = s.hear();
        check (close (s.returnPeak(), 0.25f), "send sound: and sends again when the bus is not muted");

        // The pan is taken after the send.
        s.a->pan = -1.0f;
        out = s.hear();
        const float right = s.ret->busInput.getMagnitude (1, 0, testBlock);
        const float left = s.ret->busInput.getMagnitude (0, 0, testBlock);
        check (close (right, 0.25f) && close (left, 0.25f), "send sound: a track panned hard left still sends its whole sound to both sides of the return (" + juce::String (right, 3) + ")");
        s.a->pan = 0.0f;

        // Off.
        s.a->sends[0]->levelDb = -60.0f;
        out = s.hear();
        check (s.returnPeak() < 1.0e-6f, "send sound: a send at Off sends nothing");
    }

    {
        // Two different sounds add as powers, and the return's solo.
        SendSong s (0.25f);
        s.engine.setSend (*s.a, *s.ret, 0.0f);
        s.engine.setSend (*s.c, *s.ret, 0.0f);
        s.b->muted = true;
        s.ret->soloed = true;
        auto out = s.hear (1.0);
        const float r = rms (out, 0);
        check (close (r, 0.25f), "send sound: 440 and 660 Hz sines at RMS 0.177, both sent at 0 dB, make a return of RMS 0.250, +3.01 dB (" + juce::String (r, 4) + ")");
        s.ret->soloed = false;

        // Solo a track: the return carries only that track's send.
        s.engine.setSend (*s.a, *s.ret, 0.0f);
        s.engine.setSend (*s.c, *s.ret, -60.0f);
        s.engine.setSend (*s.b, *s.ret, 0.0f);
        s.b->muted = false;
        s.a->soloed = true;
        out = s.hear();
        const float soloPeak = s.returnPeak();
        check (close (soloPeak, 0.25f) && close (SendSong::peak (out), 0.5f),
               "send sound: with A soloed the return carries A only, and is heard with A (" + juce::String (soloPeak, 3) + ", " + juce::String (SendSong::peak (out), 3) + ")");
        s.a->soloed = false;
        s.ret->soloed = true;
        out = s.hear();
        check (close (s.returnPeak(), 0.5f) && close (SendSong::peak (out), 0.5f),
               "send sound: with the return soloed the tracks stay silent but keep sending, so the master is the return alone ("
                   + juce::String (s.returnPeak(), 3) + ", " + juce::String (SendSong::peak (out), 3) + ")");
        s.ret->soloed = false;

        // Muting the return takes the effect out and the tracks play on, dry.
        s.ret->muted = true;
        s.c->muted = true;
        out = s.hear();
        check (close (SendSong::peak (out), 0.5f), "send sound: a muted return leaves the tracks playing dry (" + juce::String (SendSong::peak (out), 3) + ")");
    }

    {
        // A bus that sends carries its tracks' sum.
        SendSong s (0.25f);
        auto* group = s.engine.createBus ({ s.a, s.b }, "Group");
        s.engine.setSend (*group, *s.ret, 0.0f);
        s.c->muted = true;
        s.hear();
        check (close (s.returnPeak(), 0.5f) && close (group->busInputPeak.load(), 0.5f), "send sound: a bus that sends gives the return the sum of its tracks (" + juce::String (s.returnPeak(), 3) + ")");

        // Removing the return while playing.
        s.engine.play();
        render (s.engine, 0.1);
        s.engine.removeTrack (s.ret);
        auto out = render (s.engine, 0.2);
        check (group->sends.empty() && SendSong::peak (out) > 0.4f, "send sound: removing a return while playing is safe, and the tracks play on");
    }
}

static void runSendGuideTests()
{
    using namespace daw;
    namespace rg = daw::routingguide;
    std::cout << "Sends and returns: guide and advice" << std::endl;

    check (std::size (rg::allTopics) == 18 && rg::explain (rg::Topic::insertOrSend).contains ("9.8 dB"), "sends guide: 18 topics, and Insert or send? says 9.8 dB");
    {
        const auto example = rg::getExample (rg::Topic::sends);
        int returns = 0, links = 0;

        for (auto& n : example)
        {
            returns += n.isReturn ? 1 : 0;
            links += (int) n.sends.size();
        }

        const auto points = rg::layOut (example);
        int returnIndex = -1;

        for (size_t i = 0; i < example.size(); ++i)
            if (example[i].isReturn)
                returnIndex = (int) i;

        check (returns == 1 && links == 3 && returnIndex >= 0 && points.size() == example.size() + 1 && std::abs (points.back().x - points[(size_t) returnIndex].x - 1.0f) < 1.0e-4f
                   && points[(size_t) returnIndex].y > points[0].y && rg::getExample (rg::Topic::depth).size() == 7,
               "sends guide: the sends example has a return and 3 links, the layout puts the return one column before the master and below the rows");
        check (rg::getExample (rg::Topic::typical).size() == 18, "sends guide: the pop example still has 18 boxes");
    }

    {
        namespace tg = daw::gen::guides;
        const auto entries = tg::getEntries();
        auto first = [&entries] (const juce::String& query)
        {
            const auto found = tg::search (entries, query);
            return found.empty() ? juce::String() : entries[(size_t) found.front().entry].name;
        };

        check (first ("aux send") == "Buses, sends and routing" && first ("return track") == "Buses, sends and routing" && first ("pre-fader") == "Buses, sends and routing"
                   && first ("insert or send") == "Buses, sends and routing",
               "sends guide: \"aux send\", \"return track\", \"pre-fader\" and \"insert or send\" find the buses, sends and routing guide");
    }

    // The advice on a made-up song.
    {
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        auto* vocal = song.addTrack (Track::Type::midi);
        vocal->name = "Vocal";
        auto* guitar = song.addTrack (Track::Type::midi);
        guitar->name = "Guitar";
        auto* bass = song.addTrack (Track::Type::midi);
        bass->name = "Bass";
        juce::String error;

        for (auto* t : { vocal, guitar })
        {
            auto* slot = song.addPlugin (*t, BuiltInReverb::getDescription(), error);
            auto* plugin = slot != nullptr ? dynamic_cast<BuiltInReverb*> (slot->instance.get()) : nullptr;

            if (plugin != nullptr)
            {
                auto settings = plugin->getSettings();
                settings.mixPercent = 20.0f;
                plugin->setSettings (settings);
            }
        }

        auto find = [] (const std::vector<rg::Advice>& list, rg::Advice::Action action)
        {
            return std::find_if (list.begin(), list.end(), [action] (auto& a) { return a.action == action; });
        };

        auto advice = rg::analyse (song.getTracks(), {});
        auto share = find (advice, rg::Advice::Action::shareEffect);
        check (share != advice.end() && share->text.contains ("-9.8 dB") && share->text.contains ("Vocal and Guitar each have their own DawIt Reverb")
                   && share->tracks.size() == 2 && share->effect == "DawIt Reverb" && share->bus == nullptr && share->topic == rg::Topic::insertOrSend,
               "sends advice: two tracks with their own reverb at 20% are offered one shared return (-9.8 dB)");

        // One track alone stays quiet; a bypassed reverb doesn't count.
        guitar->plugins[0]->bypassed = true;
        const auto alone = rg::analyse (song.getTracks(), {});
        check (find (alone, rg::Advice::Action::shareEffect) == alone.end(),
               "sends advice: a single track with a reverb, or a bypassed one, gets no advice to share");
        guitar->plugins[0]->bypassed = false;

        // An empty return.
        auto* ret = song.addReturn ("Reverb");
        advice = rg::analyse (song.getTracks(), {});
        const auto noSends = std::find_if (advice.begin(), advice.end(), [] (auto& a) { return a.text.contains ("has no sends yet"); });
        const auto noEffect = find (advice, rg::Advice::Action::addReturnEffect);
        check (noSends != advice.end() && noEffect != advice.end() && noEffect->effect == "DawIt Reverb" && noEffect->bus == ret,
               "sends advice: an empty return has no sends and no effect, and is offered DawIt Reverb");
        auto* echo = song.addReturn ("Delay");
        advice = rg::analyse (song.getTracks(), {});
        check (std::any_of (advice.begin(), advice.end(), [echo] (auto& a) { return a.action == rg::Advice::Action::addReturnEffect && a.bus == echo && a.effect == "DawIt Delay"; }),
               "sends advice: a return named Delay is offered DawIt Delay");
        song.removeTrack (echo);

        // An existing return with the effect: send to it instead.
        auto* slot = song.addPlugin (*ret, BuiltInReverb::getDescription(), error);
        auto* plugin = slot != nullptr ? dynamic_cast<BuiltInReverb*> (slot->instance.get()) : nullptr;

        if (plugin != nullptr)
        {
            auto settings = plugin->getSettings();
            settings.mixPercent = 25.0f;
            plugin->setSettings (settings);
        }

        advice = rg::analyse (song.getTracks(), {});
        const auto wet = find (advice, rg::Advice::Action::makeWet);
        share = find (advice, rg::Advice::Action::shareEffect);
        check (wet != advice.end() && wet->bus == ret && wet->text.contains ("DawIt Reverb on Reverb is at 25% mix") && share != advice.end() && share->bus == ret
                   && share->text.contains ("Send them to Reverb instead"),
               "sends advice: a reverb at 25% on a return is offered 100% wet, and tracks with their own reverb are offered the return");

        // Bass.
        song.setSend (*bass, *ret, -10.0f);
        advice = rg::analyse (song.getTracks(), {});
        const bool bassAdvice = std::any_of (advice.begin(), advice.end(), [] (auto& a) { return a.text.contains ("Bass sends to Reverb at -10.0 dB") && a.topic == rg::Topic::depth; });
        song.setSend (*bass, *ret, -25.0f);
        advice = rg::analyse (song.getTracks(), {});
        check (bassAdvice && std::none_of (advice.begin(), advice.end(), [] (auto& a) { return a.text.contains ("Bass sends"); }),
               "sends advice: bass sending above -20 dB to a reverb gets a warning, and at -25 dB none");

        // Pre-fader.
        song.setSend (*vocal, *ret, -10.0f);
        rg::Levels levels;
        advice = rg::analyse (song.getTracks(), levels);
        const bool noPre = std::none_of (advice.begin(), advice.end(), [] (auto& a) { return a.topic == rg::Topic::preFader; });
        vocal->sends[0]->preFader = true;
        advice = rg::analyse (song.getTracks(), levels);
        check (noPre && std::any_of (advice.begin(), advice.end(), [] (auto& a) { return a.topic == rg::Topic::preFader && a.text.contains ("pre-fader"); }),
               "sends advice: a pre-fader send is pointed out");

        // A hot return.
        levels.busInputDb[ret] = 1.2f;
        advice = rg::analyse (song.getTracks(), levels);
        const bool stopped = std::any_of (advice.begin(), advice.end(), [] (auto& a) { return a.text.contains ("add up to +1.2 dB"); });
        levels.playing = true;
        advice = rg::analyse (song.getTracks(), levels);
        const bool playing = std::any_of (advice.begin(), advice.end(), [] (auto& a) { return a.text.contains ("The sends into Reverb add up to +1.2 dB peaks"); });
        song.removePlugin (*ret, ret->plugins[0].get());
        advice = rg::analyse (song.getTracks(), levels);
        check (! stopped && playing && std::none_of (advice.begin(), advice.end(), [] (auto& a) { return a.text.contains ("add up to +1.2 dB"); }),
               "sends advice: a hot return with effects is pointed out while playing, and not without effects");

        const auto text = rg::explainSong (song.getTracks());
        check (text.contains ("3 tracks, no buses and 1 return") && text.contains ("Reverb is a return: Vocal (-10.0 dB, pre-fader) and Bass (-25.0 dB) send to it"),
               "sends advice: the song's explanation counts returns and says who sends (" + text + ")");
        song.removeTrack (ret);
        check (rg::explainSong (song.getTracks()).contains ("3 tracks and no buses."), "sends advice: without returns the explanation is as before");
    }

    // Gain staging.
    {
        using namespace daw::gainstaging;
        auto stop = [] (const char* name, StopKind kind, float in, float out)
        {
            Stop s;
            s.name = name;
            s.kind = kind;
            s.inDb = in;
            s.outDb = out;
            return s;
        };

        std::vector<Stop> chain { stop ("Instrument", StopKind::instrument, -100.0f, -12.0f), stop ("Fader", StopKind::fader, -12.0f, -12.0f), stop ("Master", StopKind::master, -12.0f, -12.0f) };
        chain[1].settingDb = 0.0f;
        const auto before = findHints (chain);
        chain.insert (chain.begin() + 2, stop ("Reverb", StopKind::send, -12.0f, -22.0f));
        check (findHints (chain).size() == before.size(), "gain staging: a send stop between the fader and the master doesn't change the hints");

        std::vector<Stop> returnChain { stop ("Reverb", StopKind::busInput, -100.0f, 1.2f), stop ("DawIt Reverb", StopKind::effect, 1.2f, 1.0f) };
        returnChain[0].isReturn = true;
        returnChain[0].numTracks = 3;
        const auto hints = findHints (returnChain);
        check (! hints.empty() && hints[0].text.startsWith ("The sends into Reverb add up to +1.2 dB peaks"), "gain staging: a return's hot input says \"The sends into\"");
    }
}

//==============================================================================
namespace
{
/** The made-up song of the signal flow tests: Kick and Snare (Gate) in Drums (Compressor), Bass (MIDI, EQ, Saturation
    bypassed), Vocal (EQ, Compressor, a -12 dB post-fader send to Reverb) and the Reverb return (DawIt Reverb). */
struct FlowSong
{
    FlowSong()
    {
        using daw::Track;
        engine.deviceManager.removeAudioCallback (&engine);
        engine.audioDeviceAboutToStart (&device);
        kick = engine.addTrack (Track::Type::audio);
        kick->name = "Kick";
        snare = engine.addTrack (Track::Type::audio);
        snare->name = "Snare";
        bass = engine.addTrack (Track::Type::midi);
        bass->name = "Bass";
        vocal = engine.addTrack (Track::Type::audio);
        vocal->name = "Vocal";
        drums = engine.createBus ({ kick, snare }, "Drums");
        reverb = engine.addReturn ("Reverb");

        juce::String error;
        engine.addPlugin (*snare, daw::BuiltInGate::getDescription(), error);
        engine.addPlugin (*drums, daw::BuiltInCompressor::getDescription(), error);
        engine.addPlugin (*bass, daw::BuiltInEq::getDescription(), error);
        saturation = engine.addPlugin (*bass, daw::BuiltInSaturation::getDescription(), error);
        saturation->bypassed = true;
        engine.addPlugin (*vocal, daw::BuiltInEq::getDescription(), error);
        engine.addPlugin (*vocal, daw::BuiltInCompressor::getDescription(), error);
        engine.addPlugin (*reverb, daw::BuiltInReverb::getDescription(), error);
        engine.setSend (*vocal, *reverb, -12.0f);
    }

    daw::signalflow::Graph build (const daw::Track* selected = nullptr) const
    {
        return daw::signalflow::build (engine.getTracks(), selected, {});
    }

    daw::Engine engine;
    FakeDevice device;
    daw::Track *kick = nullptr, *snare = nullptr, *bass = nullptr, *vocal = nullptr, *drums = nullptr, *reverb = nullptr;
    daw::PluginSlot* saturation = nullptr;
};
} // namespace

static void runSignalFlowTests()
{
    using namespace daw;
    namespace sf = daw::signalflow;
    std::cout << "Signal flow" << std::endl;

    auto near = [] (float a, float b, float tolerance = 0.01f) { return std::abs (a - b) <= tolerance; };

    // Levels that any number of views can read.
    {
        SendSong s;
        s.engine.setSend (*s.a, *s.ret, -6.0206f);
        juce::String error;
        auto* slot = s.engine.addPlugin (*s.a, BuiltInEq::getDescription(), error);
        s.hear();
        const float source = s.a->chainInputLevel.load(), out = slot->outputLevel.load(), send = s.a->sends[0]->level.load();
        check (source > 0.4f && out > 0.4f && send > 0.2f, "flow levels: the level into the chain, out of an effect and out of a send are above 0 while a track plays ("
                                                              + juce::String (source, 3) + ", " + juce::String (out, 3) + ", " + juce::String (send, 3) + ")");

        // What the device bar does when it reads the peaks.
        slot->outputPeak.exchange (0.0f);
        s.a->sends[0]->peak.exchange (0.0f);
        s.a->faderInputPeak.exchange (0.0f);
        check (juce::exactlyEqual (s.a->chainInputLevel.load(), source) && juce::exactlyEqual (slot->outputLevel.load(), out) && juce::exactlyEqual (s.a->sends[0]->level.load(), send),
               "flow levels: reading the peaks takes nothing from the falling levels");

        s.engine.stop();
        render (s.engine, 1.0);
        check (s.a->chainInputLevel.load() < 0.001f && slot->outputLevel.load() < 0.001f && s.a->sends[0]->level.load() < 0.001f,
               "flow levels: all three fall below 0.001 after a second of silence");
    }

    // The graph of the made-up song.
    {
        FlowSong song;
        auto g = song.build();
        int flowEdges = 0, sendEdges = 0;

        for (auto& e : g.edges)
            (e.isSend ? sendEdges : flowEdges)++;

        check (g.nodes.size() == 20 && flowEdges == 19 && sendEdges == 1, "flow graph: 20 nodes, 19 flow edges and 1 send edge ("
                                                                           + juce::String ((int) g.nodes.size()) + ", " + juce::String (flowEdges) + ", " + juce::String (sendEdges) + ")");

        const auto& send = *std::find_if (g.edges.begin(), g.edges.end(), [] (auto& e) { return e.isSend; });
        check (near (send.sendDb, -12.0f) && ! send.preFader && g.nodes[(size_t) send.from].kind == sf::NodeKind::fader
                   && g.tracks[(size_t) g.nodes[(size_t) send.from].trackIndex].name == "Vocal" && g.nodes[(size_t) send.to].kind == sf::NodeKind::sum
                   && g.tracks[(size_t) g.nodes[(size_t) send.to].trackIndex].name == "Reverb",
               "flow graph: the send is -12 dB, post-fader, from Vocal's fader to Reverb's sum");

        std::vector<int> outgoing (g.nodes.size(), 0);
        bool forward = true;

        for (auto& e : g.edges)
        {
            if (! e.isSend)
                ++outgoing[(size_t) e.from];

            forward = forward && g.nodes[(size_t) e.from].column < g.nodes[(size_t) e.to].column;
        }

        bool oneOut = true;

        for (size_t i = 0; i < g.nodes.size(); ++i)
            oneOut = oneOut && outgoing[i] == ((int) i == g.master ? 0 : 1);

        check (oneOut && forward, "flow graph: every box but the master has one line out, and every line goes to a higher column");

        // Columns and rows, printed for a look.
        auto column = [&g] (const juce::String& track, sf::NodeKind kind, int nth = 0)
        {
            for (size_t i = 0; i < g.nodes.size(); ++i)
                if (g.nodes[i].trackIndex >= 0 && g.tracks[(size_t) g.nodes[i].trackIndex].name == track && g.nodes[i].kind == kind && nth-- == 0)
                    return g.nodes[i].column;

            return -1;
        };
        auto row = [&g] (const juce::String& track)
        {
            for (auto& t : g.tracks)
                if (t.name == track)
                    return t.row;

            return -1.0f;
        };

        for (auto& n : g.nodes)
            std::cout << "    " << (n.trackIndex >= 0 ? g.tracks[(size_t) n.trackIndex].name : juce::String ("Master")) << " " << n.label << " column " << n.column << std::endl;

        check (column ("Kick", sf::NodeKind::source) == 0 && column ("Kick", sf::NodeKind::fader) == 1 && column ("Snare", sf::NodeKind::fader) == 2
                   && column ("Drums", sf::NodeKind::sum) == 3 && column ("Drums", sf::NodeKind::effect) == 4 && column ("Drums", sf::NodeKind::fader) == 5
                   && column ("Bass", sf::NodeKind::source) == 0 && column ("Bass", sf::NodeKind::fader) == 3 && column ("Vocal", sf::NodeKind::fader) == 3
                   && column ("Reverb", sf::NodeKind::sum) == 4 && column ("Reverb", sf::NodeKind::effect) == 5 && column ("Reverb", sf::NodeKind::fader) == 6
                   && g.nodes[(size_t) g.master].column == 7,
               "flow graph: the columns are the longest path from the left, and the master is one past the last");
        check (near (row ("Kick"), 0.0f) && near (row ("Snare"), 1.0f) && near (row ("Drums"), 0.5f) && near (row ("Bass"), 2.0f) && near (row ("Vocal"), 3.0f)
                   && near (row ("Reverb"), 4.0f) && near (g.masterRow, 2.0f),
               "flow graph: tracks one per row, the bus at the middle of its tracks, the return below, the master in the middle");

        int bypassed = 0;

        for (auto& n : g.nodes)
            if (n.bypassed)
            {
                ++bypassed;
                check (n.label == BuiltInSaturation::identifier && g.tracks[(size_t) n.trackIndex].name == "Bass", "flow graph: the bypassed box is Bass's Saturation");
            }

        check (bypassed == 1, "flow graph: only Bass's Saturation is bypassed");

        // The selected track's path.
        check (song.build (song.kick).getNumLit() == 6 && song.build (song.vocal).getNumLit() == 8 && song.build (song.drums).getNumLit() == 9
                   && g.getNumLit() == 20 && ! g.hasSelection,
               "flow graph: Kick lights 6 boxes, Vocal 8 and Drums 9; with no selection nothing is dimmed");

        // Mute and solo.
        auto silencedTracks = [] (const sf::Graph& graph)
        {
            juce::StringArray names;

            for (auto& t : graph.tracks)
                if (t.silence != routing::Silence::none)
                    names.add (t.name);

            return names.joinIntoString (",");
        };

        song.drums->muted = true;
        check (silencedTracks (song.build()) == "Drums,Kick,Snare", "flow graph: with Drums muted, the chains of Kick, Snare and Drums are silenced (" + silencedTracks (song.build()) + ")");
        song.drums->muted = false;
        song.vocal->soloed = true;
        check (silencedTracks (song.build()) == "Drums,Kick,Snare,Bass", "flow graph: with Vocal soloed, Kick, Snare, Drums and Bass are silenced, and Vocal and Reverb are not ("
                                                                         + silencedTracks (song.build()) + ")");
        song.vocal->soloed = false;

        // Levels.
        song.kick->chainInputLevel = 0.5f;
        song.kick->peakLeft = 0.25f;
        auto levels = song.build();
        check (near (levels.nodes[(size_t) levels.getFirstNode (1)].levelDb, -6.02f) && near (levels.nodes[(size_t) levels.getFader (1)].levelDb, -12.04f)
                   && near (sf::toDb (0.5f), -6.02f) && near (sf::toDb (0.25f), -12.04f),
               "flow graph: a level of 0.5 shows as -6.02 dB and 0.25 as -12.04 dB");

        // The example.
        auto example = sf::getExample();
        check (example.nodes.size() == 20 && near (example.nodes[(size_t) example.getSum (0)].levelDb, -9.875f) && near (example.nodes[(size_t) example.master].levelDb, -7.48f)
                   && example.hasSelection && example.getNumLit() == 8,
               "flow graph: the example has 20 boxes, Drums adds up to -9.88 dB and the master to -7.48 dB, with the Vocal's path lit");
        check (near (routingguide::sumDb ({ -12.0f, -14.0f }, false), -9.875f) && near (routingguide::sumDb ({ -12.0f, -13.0f, -12.0f, -26.0f }, false), -7.48f),
               "flow graph: sumDb gives the example's -9.88 dB for Drums and -7.48 dB for the master");
    }

    // Read-only: building doesn't change the song.
    {
        FlowSong song;
        song.drums->muted = true;
        song.vocal->soloed = true;
        song.vocal->sends[0]->levelDb = -7.0f;

        auto describe = [&song]
        {
            juce::String text;

            for (auto& t : song.engine.getTracks())
            {
                text << t->name << ":" << (t->bus != nullptr ? t->bus->name : juce::String ("-")) << ":" << (int) t->muted.load() << (int) t->soloed.load() << ":" << t->volumeDb.load() << ":";

                for (auto& slot : t->plugins)
                    text << slot->instance->getName() << (int) slot->bypassed.load() << ",";

                for (auto& send : t->sends)
                    text << "s" << send->levelDb.load() << (int) send->preFader.load() << ",";

                text << ";";
            }

            return text;
        };

        const auto before = describe();
        const auto count = song.engine.getTracks().size();
        auto g = song.build (song.vocal);
        (void) g;
        check (describe() == before && song.engine.getTracks().size() == count, "flow graph: building the graph changes nothing in the song (the tracks, chains, bypass, mute, solo and sends)");
    }

    // The order of effects (pure maths and two EQs).
    {
        comp::Settings st;
        st.thresholdDb = -30.0f;
        st.ratio = 4.0f;
        st.kneeDb = 6.0f;
        const double atMinus10 = comp::getReductionDb (st, -10.0), atMinus4 = comp::getReductionDb (st, -4.0);
        const double before = -10.0 + 6.0 - comp::getReductionDb (st, -4.0) ;
        const double after = -10.0 - atMinus10 + 6.0;
        check (std::abs (atMinus10 - 15.0) < 0.01 && std::abs (atMinus4 - 19.5) < 0.01 && std::abs ((before - after) - (-4.5)) < 0.01,
               "flow order: 6 dB before a 4:1 compressor with its threshold at -30 dB comes out at -23.5 dB, and 6 dB after it at -19 dB: 4.5 dB apart ("
                   + juce::String (before, 2) + ", " + juce::String (after, 2) + ")");

        auto makeEq = [] (bool lowCut)
        {
            auto plugin = std::make_unique<BuiltInEq>();

            for (int i = 0; i < eq::numBands; ++i)
                plugin->setBand (i, { false, eq::getDefaultBand (i).type, eq::getDefaultBand (i).freq, 0.0f, 1.0f, 12 });

            plugin->setBand (2, lowCut ? eq::Band { true, eq::BandType::lowCut, 100.0f, 0.0f, 0.71f, 12 }
                                       : eq::Band { true, eq::BandType::bell, 3000.0f, 6.0f, 1.0f, 12 });
            plugin->setMatchingLevel (false);
            plugin->prepareToPlay (testRate, testBlock);
            return plugin;
        };

        auto run = [] (BuiltInEq& first, BuiltInEq& second)
        {
            juce::Random random (7);
            juce::AudioBuffer<float> result (2, (int) (2 * testRate));
            juce::AudioBuffer<float> block (2, testBlock);
            juce::MidiBuffer midi;

            for (int pos = 0; pos + testBlock <= result.getNumSamples(); pos += testBlock)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < testBlock; ++i)
                        block.setSample (ch, i, 0.25f * (random.nextFloat() * 2.0f - 1.0f));

                first.processBlock (block, midi);
                second.processBlock (block, midi);

                for (int ch = 0; ch < 2; ++ch)
                    result.copyFrom (ch, pos, block, ch, 0, testBlock);
            }

            return result;
        };

        auto cut1 = makeEq (true), bell1 = makeEq (false), cut2 = makeEq (true), bell2 = makeEq (false);
        const auto one = run (*cut1, *bell1), two = run (*bell2, *cut2);
        float biggest = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = one.getNumSamples() / 2; i < one.getNumSamples(); ++i)   // after the filters have settled
                biggest = juce::jmax (biggest, std::abs (one.getSample (ch, i) - two.getSample (ch, i)));

        check (biggest < 1.0e-3f, "flow order: two EQs (a low cut and a bell) give the same sound in either order (largest difference " + juce::String (biggest, 6) + ")");
    }

    // The guide and its advice on the order of effects.
    {
        namespace rg = daw::routingguide;
        namespace tg = daw::gen::guides;

        bool topicsOk = true;

        for (auto topic : rg::allTopics)
            topicsOk = topicsOk && rg::getTopicName (topic).isNotEmpty() && rg::getTopicSummary (topic).isNotEmpty() && rg::explain (topic).length() > 200 && rg::getExample (topic).size() >= 3;

        const auto text = rg::explain (rg::Topic::signalFlow);
        check (std::size (rg::allTopics) == 18 && topicsOk && rg::getTopicName (rg::Topic::signalFlow) == "Signal flow" && text.contains ("10.7 ms") && text.contains ("4.5 dB")
                   && text.contains ("-23.5 dB") && text.contains ("-19 dB") && text.contains ("-9.9 dB"),
               "signal flow guide: 18 topics, and Signal flow has text with 10.7 ms, 4.5 dB, -23.5 dB, -19 dB and -9.9 dB");

        const auto entries = tg::getEntries();
        auto first = [&entries] (const juce::String& query)
        {
            const auto found = tg::search (entries, query);
            return found.empty() ? juce::String() : entries[(size_t) found.front().entry].name;
        };
        check (first ("signal chain") == "Buses, sends and routing" && first ("effect order") == "Buses, sends and routing" && first ("plugin order") == "Buses, sends and routing",
               "signal flow guide: \"signal chain\", \"effect order\" and \"plugin order\" find the buses, sends and routing guide");

        // A song with every rule.
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        auto* vocal = song.addTrack (Track::Type::audio);
        vocal->name = "Vocal";
        auto* snare = song.addTrack (Track::Type::audio);
        snare->name = "Snare";
        auto* guitar = song.addTrack (Track::Type::audio);
        guitar->name = "Guitar";
        auto* keys = song.addTrack (Track::Type::audio);
        keys->name = "Keys";
        auto* hall = song.addReturn ("Hall");
        juce::String error;
        auto add = [&] (Track& t, const juce::PluginDescription& d) { return song.addPlugin (t, d, error); };
        add (*vocal, BuiltInReverb::getDescription());
        add (*vocal, BuiltInCompressor::getDescription());
        add (*snare, BuiltInReverb::getDescription());
        add (*snare, BuiltInGate::getDescription());
        add (*guitar, BuiltInCompressor::getDescription());
        add (*guitar, BuiltInGate::getDescription());
        add (*keys, BuiltInDelay::getDescription());
        add (*keys, BuiltInDistortion::getDescription());
        add (*hall, BuiltInReverb::getDescription());
        song.setSend (*vocal, *hall, -12.0f);

        auto flowAdvice = [] (const Engine& e)
        {
            std::vector<rg::Advice> found;

            for (auto& a : rg::analyse (e.getTracks(), {}))
                if (a.topic == rg::Topic::signalFlow)
                    found.push_back (a);

            return found;
        };

        const auto found = flowAdvice (song);
        int moves = 0;
        juce::String texts;

        for (auto& a : found)
        {
            moves += a.action == rg::Advice::Action::moveEffect ? 1 : 0;
            texts << "\n      " << a.text << " [" << a.buttonText << "]";
        }

        std::cout << "    advice:" << texts << std::endl;
        check (found.size() == 5 && moves == 4, "signal flow advice: five rules fire once each, four with a move button (" + juce::String ((int) found.size()) + ", " + juce::String (moves) + ")");
        auto find = [&found] (const juce::String& start) -> const rg::Advice*
        {
            for (auto& a : found)
                if (a.text.startsWith (start))
                    return &a;

            return nullptr;
        };
        const auto* r1 = find ("On Vocal, DawIt Compressor comes after DawIt Reverb");
        const auto* r2 = find ("On Snare, DawIt Gate comes after DawIt Reverb");
        const auto* r3 = find ("On Guitar, DawIt Gate comes after DawIt Compressor");
        const auto* r4 = find ("On Keys, DawIt Distortion comes after DawIt Delay");
        const auto* r5 = find ("Vocal has its own DawIt Reverb and also sends to Hall");
        check (r1 != nullptr && r2 != nullptr && r3 != nullptr && r4 != nullptr && r5 != nullptr && r1->buttonText == "Put DawIt Compressor before DawIt Reverb"
                   && r2->buttonText == "Put DawIt Gate first" && r2->text.contains ("1980s") && r3->buttonText == "Put DawIt Gate first" && r3->text.contains ("harder to set")
                   && r4->buttonText == "Put DawIt Distortion before DawIt Delay" && r5->action == rg::Advice::Action::none,
               "signal flow advice: the texts and buttons follow the rules (compressor after reverb, gate after reverb, gate after compressor, distortion after delay, two reverbs)");

        // Doing rule 1 puts the compressor before the reverb and the advice goes.
        if (r1 != nullptr)
        {
            song.movePlugin (*r1->tracks[0], r1->slot, r1->toIndex);
            check (vocal->plugins[0]->instance->getName() == BuiltInCompressor::identifier && vocal->plugins[1]->instance->getName() == BuiltInReverb::identifier
                       && flowAdvice (song).size() == 4,
                   "signal flow advice: moving the compressor before the reverb gives Compressor, Reverb, and the advice is gone");
        }

        for (auto& a : found)
            if (a.action == rg::Advice::Action::moveEffect)
                song.movePlugin (*a.tracks[0], a.slot, a.toIndex);

        check (flowAdvice (song).size() == 1, "signal flow advice: after the four moves only the two reverbs advice is left");

        // Quiet cases.
        Engine quiet;
        quiet.deviceManager.removeAudioCallback (&quiet);
        auto* a = quiet.addTrack (Track::Type::audio);
        a->name = "A";
        auto* b = quiet.addTrack (Track::Type::audio);
        b->name = "B";
        auto* c = quiet.addTrack (Track::Type::audio);
        c->name = "C";
        auto* d = quiet.addTrack (Track::Type::audio);
        d->name = "D";
        auto* e2 = quiet.addTrack (Track::Type::audio);
        e2->name = "E";
        auto* room = quiet.addReturn ("Room");
        auto addQ = [&] (Track& t, const juce::PluginDescription& desc) { return quiet.addPlugin (t, desc, error); };
        addQ (*a, BuiltInCompressor::getDescription());
        addQ (*a, BuiltInReverb::getDescription());
        addQ (*room, BuiltInReverb::getDescription());
        addQ (*room, BuiltInCompressor::getDescription());
        auto* bypassedReverb = addQ (*b, BuiltInReverb::getDescription());
        bypassedReverb->bypassed = true;
        addQ (*b, BuiltInCompressor::getDescription());
        addQ (*c, BuiltInCompressor::getDescription());
        addQ (*c, BuiltInEq::getDescription());
        auto* dryVerb = addQ (*d, BuiltInReverb::getDescription());
        auto settings = static_cast<BuiltInReverb*> (dryVerb->instance.get())->getSettings();
        settings.mixPercent = 0.0f;
        static_cast<BuiltInReverb*> (dryVerb->instance.get())->setSettings (settings);
        addQ (*d, BuiltInCompressor::getDescription());
        addQ (*e2, BuiltInEq::getDescription());
        addQ (*e2, BuiltInCompressor::getDescription());
        check (flowAdvice (quiet).empty(), "signal flow advice: quiet for Compressor then Reverb, a return with Reverb then Compressor, a bypassed Reverb before a Compressor, EQ after Compressor and a reverb at 0% mix ("
                                              + juce::String ((int) flowAdvice (quiet).size()) + ")");
    }

    check (gainstaging::getZone (-30.0f) == gainstaging::Zone::quiet && gainstaging::getZone (-12.0f) == gainstaging::Zone::healthy
               && gainstaging::getZone (-3.0f) == gainstaging::Zone::hot && gainstaging::getZone (1.0f) == gainstaging::Zone::over,
           "flow levels: -30, -12, -3 and +1 dB are quiet, healthy, hot and over");
}

static void runExampleSongTests()
{
    using namespace daw;
    std::cout << "Example song" << std::endl;

    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* lead = examples::buildPopSong (engine);

    check (engine.getKey() == MusicalKey { 9, MusicalKey::Scale::minor } && std::abs (engine.getTempo() - 96.0) < 1.0e-9
               && engine.getSections().size() == 5 && engine.getSections().back().getEndBar() == 36,
           "the example song is in A minor at 96 BPM, with 5 sections over 36 bars");

    bool everyBar = engine.getChords().size() == 36;

    for (int bar = 0; bar < 36; ++bar)
        everyBar = everyBar && engine.getChord (bar) != nullptr;

    check (everyBar && engine.getChord (0)->getName() == "Am" && engine.getChord (12)->getName() == "F",
           "it has a chord on every bar: Am to start, F where the chorus starts");

    const auto& lines = engine.getLyrics().lines;
    size_t syllables = 0;
    bool sung = lines.size() == 16;

    for (auto& line : lines)
    {
        const auto* section = engine.getSectionAt ((int) (line.startBeat / 4.0));
        sung = sung && section != nullptr && (section->type == SectionType::verse || section->type == SectionType::chorus);
        syllables += line.syllables.size();
    }

    check (sung, "its lyrics are 16 lines on the verses and choruses (the chorus sung twice)");

    const auto& tracks = engine.getTracks();
    const auto find = [&tracks] (const juce::String& name) -> Track*
    {
        for (auto& t : tracks)
            if (t->name == name)
                return t.get();

        return nullptr;
    };

    auto* kick = find ("Kick");
    auto* bv1 = find ("BV 1");
    auto* drums = find ("Drums");
    check (tracks.size() == 11 && routing::isTidy (tracks) && kick != nullptr && kick->bus == drums && bv1 != nullptr
               && bv1->bus != nullptr && bv1->bus->name == "Backing vocals" && bv1->bus->bus != nullptr
               && bv1->bus->bus->name == "Vocals" && bv1->bus->bus->bus == nullptr && lead != nullptr && lead->name == "Lead vocal",
           "its 8 tracks sit in Drums, Vocals and Backing vocals (inside Vocals), in a tidy edit order");

    check (drums != nullptr && drums->plugins.size() == 1 && drums->plugins[0]->instance->getName() == "DawIt Compressor",
           "the Drums bus has DawIt Compressor on it");

    bool inClips = true;
    size_t leadNotes = 0;

    for (auto& t : tracks)
    {
        for (auto& c : t->clips)
        {
            if (auto* midi = dynamic_cast<MidiClip*> (c.get()))
            {
                for (auto& n : midi->notes)
                    inClips = inClips && n.start >= 0.0 && n.start < midi->lengthBeats && n.pitch >= 24 && n.pitch <= 84;

                if (t.get() == lead)
                    leadNotes += midi->notes.size();
            }
        }
    }

    check (inClips && leadNotes == syllables, "every note lies inside its clip, and the lead vocal sings a note per syllable ("
                                                  + juce::String ((int) leadNotes) + " of " + juce::String ((int) syllables) + ")");

    const auto folder = juce::File::createTempFile ("example");
    folder.createDirectory();
    const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
    Engine loaded;
    loaded.deviceManager.removeAudioCallback (&loaded);
    juce::String text, error;
    juce::StringArray warnings;
    const auto file = folder.getChildFile ("Song.dawit");
    const bool ok = project::save (engine, file, text, error) && project::load (loaded, file, error, warnings);
    check (ok && warnings.isEmpty() && loaded.getTracks().size() == 11 && loaded.getLyrics() == engine.getLyrics()
               && loaded.getChords() == engine.getChords() && loaded.getSections().size() == 5,
           "the example song saves and opens again with its tracks, chords, sections and lyrics " + error);
}

static void runSongTemplateTests()
{
    using namespace daw;
    namespace st = gen::templates;
    std::cout << "Song templates" << std::endl;

    const auto& styles = st::getStyles();
    check (styles.size() == 14 && styles.front().id == "pop-ballad" && styles.back().id == "film-score",
           "song templates: all 14 styles are present, from Pop ballad to Film score");

    bool dataFound = true;
    for (const auto& style : styles)
    {
        dataFound = dataFound && style.minimumBpm <= style.defaultBpm && style.defaultBpm <= style.maximumBpm
                    && st::findStructure (style) != nullptr && gen::styles::findStyle (style.chordStyle) != nullptr
                    && gen::styles::findStyle (style.bassStyle) != nullptr
                    && (style.padStyle.isEmpty() || gen::styles::findStyle (style.padStyle) != nullptr);

        for (const auto& choices : style.progressions)
            for (const auto& name : choices)
            {
                bool found = false;
                for (const auto& progression : gen::guide::getCommonProgressions ({ style.tonics[0], style.scale }))
                    found = found || progression.name == name;
                dataFound = dataFound && found;
            }

        bool kitFound = false, returnFound = false;
        for (const auto& preset : drums::getPresets()) kitFound = kitFound || preset.name == style.drumKit;
        for (const auto& preset : verb::getPresets()) returnFound = returnFound || preset.name == style.returnPreset;
        dataFound = dataFound && kitFound && returnFound;
    }
    check (dataFound, "song templates: every tempo, structure, progression, playing style, drum kit and reverb starting point exists");

    auto noteStarts = [] (const st::SongPlan& plan, const juce::String& partName)
    {
        std::vector<double> starts;
        for (const auto& part : plan.parts)
            if (part.name == partName && ! part.clips.empty())
                for (const auto& note : part.clips.front().notes)
                    starts.push_back (note.start);
        return starts;
    };
    auto hasStarts = [&] (const st::SongPlan& plan, const juce::String& partName, std::initializer_list<double> expected)
    {
        const auto starts = noteStarts (plan, partName);
        return std::all_of (expected.begin(), expected.end(), [&] (double beat)
        {
            return std::any_of (starts.begin(), starts.end(), [beat] (double actual) { return std::abs (actual - beat) < 1.0e-6; });
        });
    };

    const auto house = st::makePlan (*st::findStyle ("house"), std::nullopt, 124.0, 0, true);
    const auto reggaeton = st::makePlan (*st::findStyle ("reggaeton"), std::nullopt, 95.0, 0, true);
    const auto afrobeats = st::makePlan (*st::findStyle ("afrobeats"), std::nullopt, 108.0, 0, true);
    const auto dnb = st::makePlan (*st::findStyle ("drum-and-bass"), std::nullopt, 174.0, 0, true);
    check (hasStarts (house, "Kick", { 0.0, 1.0, 2.0, 3.0 })
               && hasStarts (reggaeton, "Snare", { 0.75, 1.5, 2.75, 3.5 })
               && hasStarts (afrobeats, "Rim", { 0.0, 0.75, 1.5, 2.5, 3.0 })
               && hasStarts (dnb, "Kick", { 0.0, 2.5 }) && hasStarts (dnb, "Snare", { 1.0, 3.0 }),
           "song templates: house, dembow, afrobeats and drum & bass use their documented drum steps");

    const auto trap = st::makePlan (*st::findStyle ("trap"), std::nullopt, 140.0, 0, true);
    bool trapClap = true, trapRoll = false;
    for (const auto& part : trap.parts)
        for (const auto& clip : part.clips)
            if (part.name == "Clap")
                for (const auto& note : clip.notes)
                    trapClap = trapClap && std::abs (std::fmod (note.start, 4.0) - 2.0) < 1.0e-6;
            else if (part.name == "Hats")
            {
                int hits = 0;
                for (const auto& note : clip.notes)
                    if (note.start >= 7.0 - 1.0e-6 && note.start <= 7.875 + 1.0e-6)
                        ++hits;
                trapRoll = trapRoll || hits == 8;
            }
    check (trapClap && trapRoll, "song templates: trap has one clap on beat 3 and eight 32nd-note hi-hats at the end of bar 2");

    const auto blues = st::makePlan (*st::findStyle ("blues"), std::nullopt, 100.0, 0, true);
    check (hasStarts (blues, "Hats", { 0.0, 2.0 / 3.0, 1.0, 1.0 + 2.0 / 3.0 }),
           "song templates: the blues hi-hat plays the first and third triplet of every beat");

    const auto trance = st::makePlan (*st::findStyle ("trance"), std::nullopt, 138.0, 0, true);
    bool rollRight = false;
    for (const auto& part : trance.parts)
        if (part.name == "Clap")
            for (const auto& clip : part.clips)
                if (juce::exactlyEqual (clip.lengthBeats, 32.0) && clip.notes.size() == 64)
                {
                    rollRight = std::abs (clip.notes.front().velocity - 0.4f) < 0.01f
                                && std::abs (clip.notes.back().velocity - 1.0f) < 0.01f;
                    for (size_t i = 1; i < clip.notes.size(); ++i)
                        rollRight = rollRight && clip.notes[i].velocity >= clip.notes[i - 1].velocity;
                }
    check (rollRight, "song templates: an eight-bar trance build-up has 64 clap hits rising from 0.4 to 1.0");

    bool plansRight = true, seedsVary = true, durationsRight = true;
    const std::map<juce::String, juce::String> durations {
        { "pop-ballad", "3:39" }, { "dance-pop", "2:08" }, { "rock", "1:52" }, { "hip-hop", "2:51" },
        { "trap", "1:50" }, { "house", "3:37" }, { "techno", "3:27" }, { "trance", "3:29" },
        { "drum-and-bass", "3:08" }, { "lo-fi", "2:00" }, { "blues", "2:43" }, { "reggaeton", "2:42" },
        { "afrobeats", "2:22" }, { "film-score", "3:12" }
    };

    for (const auto& style : styles)
    {
        const auto first = st::makePlan (style, std::nullopt, style.defaultBpm, 0, true);
        const auto again = st::makePlan (style, std::nullopt, style.defaultBpm, 0, true);
        std::set<int> keys;
        for (uint32_t seed = 0; seed < 20; ++seed)
            keys.insert (st::makePlan (style, std::nullopt, style.defaultBpm, seed, true).key.tonic);

        seedsVary = seedsVary && keys.size() >= 2;
        durationsRight = durationsRight && gen::structure::formatDuration (first.getTotalBars(), first.tempo) == durations.at (style.id);
        plansRight = plansRight && first.sections == again.sections && first.chords == again.chords
                     && first.sections == st::findStructure (style)->toSections (0)
                     && first.chords.size() == (size_t) first.getTotalBars()
                     && first.chords.at (first.getTotalBars() - 1) == gen::guide::buildChord (first.key, { 1 });

        for (const auto& [bar, chord] : first.chords)
            plansRight = plansRight && bar >= 0 && bar < first.getTotalBars() && first.key.contains (chord.root);

        for (const auto& part : first.parts)
            for (const auto& clip : part.clips)
            {
                const int sectionIndex = (int) std::distance (first.sections.begin(),
                    std::find_if (first.sections.begin(), first.sections.end(), [&] (const Section& section)
                    {
                        return std::abs (section.startBar * 4.0 - clip.startBeat) < 1.0e-9;
                    }));
                plansRight = plansRight && st::playsInSection (style, part.name, sectionIndex, first.sections)
                             && (part.name != "Melody" || clip.notes.empty());
            }
    }
    check (plansRight && seedsVary, "song templates: plans use the structure and key, end at home, repeat exactly and vary across seeds");
    check (durationsRight && std::abs (240.0 / 70.0 - 3.43) < 0.01 && std::abs (240.0 / 140.0 - 1.71) < 0.01,
           "song templates: all documented song lengths and bar lengths match their tempo");

    bool builtAll = true, routingRight = true, quietEnough = true;
    float worstPeak = 0.0f;
    juce::String worstStyle;
    for (const auto& style : styles)
    {
        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        juce::String error;
        const auto plan = st::makePlan (style, std::nullopt, style.defaultBpm, 0, true);
        builtAll = builtAll && st::build (engine, plan, error);

        Track* bus = nullptr;
        Track* ret = nullptr;
        int drumTracks = 0, inBus = 0, senders = 0;
        for (const auto& track : engine.getTracks())
        {
            if (track->name == "Drums") bus = track.get();
            if (track->name == "Reverb") ret = track.get();
            if (std::any_of (style.drums.begin(), style.drums.end(), [&] (const st::DrumPart& part) { return part.name == track->name; }))
            {
                ++drumTracks;
                if (track->bus != nullptr && track->bus->name == "Drums") ++inBus;
                routingRight = routingRight && ! track->plugins.empty() && dynamic_cast<BuiltInDrums*> (track->plugins[0]->instance.get()) != nullptr;
            }
            for (const auto& send : track->sends)
                if (send->isOn()) ++senders;
        }
        routingRight = routingRight && bus != nullptr && ret != nullptr && drumTracks == (int) style.drums.size() && inBus == drumTracks
                       && bus->plugins.size() == 1 && dynamic_cast<BuiltInCompressor*> (bus->plugins[0]->instance.get()) != nullptr
                       && ret->plugins.size() == 1 && dynamic_cast<BuiltInReverb*> (ret->plugins[0]->instance.get()) != nullptr && senders >= 2;

        const auto loud = std::find_if (plan.sections.begin(), plan.sections.end(), [] (const Section& section)
        {
            return section.type == SectionType::chorus || section.type == SectionType::drop;
        });
        if (loud != plan.sections.end())
        {
            engine.setPositionBeats (loud->startBar * 4.0);
            engine.play();
            const auto audio = render (engine, 240.0 / plan.tempo);
            const float peak = audio.getMagnitude (0, audio.getNumSamples());
            if (peak > worstPeak) { worstPeak = peak; worstStyle = style.name; }
            quietEnough = quietEnough && peak < juce::Decibels::decibelsToGain (-1.0f);
        }
    }
    check (builtAll && routingRight, "song templates: every style builds its tracks, DawIt Drums, Drums bus, effects, Reverb return and sends");
    check (quietEnough, "song templates: the loudest section of every style stays below -1 dBFS (worst: " + worstStyle + " at "
                        + juce::String (juce::Decibels::gainToDecibels (worstPeak), 2) + " dBFS)");

    {
        Engine empty;
        empty.deviceManager.removeAudioCallback (&empty);
        juce::String error;
        const auto plan = st::makePlan (*st::findStyle ("dance-pop"), std::nullopt, 120.0, 0, false);
        bool noNotes = st::build (empty, plan, error);
        for (const auto& track : empty.getTracks())
            for (const auto& clip : track->clips)
                noNotes = noNotes && static_cast<MidiClip&> (*clip).notes.empty();
        check (noNotes, "song templates: Write the notes off keeps all clips, routing and effects but leaves every clip empty");
    }

    {
        Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        juce::String error, text;
        st::build (engine, trap, error);
        const auto folder = juce::File::createTempFile ("song-template");
        folder.createDirectory();
        const juce::ScopeGuard cleanUp { [folder] { folder.deleteRecursively(); } };
        const auto file = folder.getChildFile ("Trap.dawit");
        Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        juce::StringArray warnings;
        const bool saved = project::save (engine, file, text, error) && project::load (loaded, file, error, warnings);
        const auto loadedText = project::toText (loaded, file);
        check (saved && warnings.isEmpty() && loadedText == text,
               "song templates: a built template saves and loads back to the same project text " + error);
    }
}

static void runStylesGuideTests()
{
    using namespace daw;
    namespace sg = gen::stylesguide;
    std::cout << "Styles of music guide" << std::endl;

    Engine empty;
    empty.deviceManager.removeAudioCallback (&empty);
    bool topicsRight = true;
    for (auto topic : sg::allTopics)
    {
        const auto example = sg::getExample (topic);
        topicsRight = topicsRight && sg::getTopicName (topic).isNotEmpty() && sg::getTopicSummary (topic).isNotEmpty()
                      && sg::explain (topic, empty).length() > 100 && ! example.sections.empty() && ! example.parts.empty();
    }
    check (std::size (sg::allTopics) == 18 && topicsRight,
           "styles guide: all 18 topics have a name, summary, beginner text and example song");

    empty.setTempo (96.0);
    check (sg::analyse (empty).contains ("usual range") && sg::analyse (empty).contains ("No sections yet"),
           "styles guide: a 96 BPM song names its matching styles and explains how to add sections");
    empty.setTempo (155.0);
    check (sg::analyse (empty).contains ("Half of it, 77.5") && sg::analyse (empty).contains ("half tempo"),
           "styles guide: a fast song with no direct match checks the tempo listeners may feel");

    Engine dance;
    dance.deviceManager.removeAudioCallback (&dance);
    juce::String error;
    const auto plan = gen::templates::makePlan (*gen::templates::findStyle ("dance-pop"), std::nullopt, 120.0, 0, true);
    const bool built = gen::templates::build (dance, plan, error);
    const auto danceAdvice = sg::analyse (dance);
    check (built && danceAdvice.contains ("Pop song structure")
               && (danceAdvice.contains ("same chords in another order") || danceAdvice.contains ("moves to other chords")),
           "styles guide: a template song recognises its structure and chord contrast " + error);

    Engine energy;
    energy.deviceManager.removeAudioCallback (&energy);
    energy.setSections ({ { 0, 1, SectionType::verse, {} }, { 1, 1, SectionType::chorus, {} } });
    auto* steady = energy.addTrack (Track::Type::midi);
    auto* lift = energy.addTrack (Track::Type::midi);
    energy.addMidiClip (*steady, 0.0, 8.0)->notes.push_back ({ 60, 0.0, 8.0, 0.8f });
    energy.addMidiClip (*lift, 4.0, 4.0)->notes.push_back ({ 67, 0.0, 4.0, 0.8f });
    check (sg::analyse (energy).contains ("chorus has 2 parts") && sg::analyse (energy).contains ("verse 1: it lifts"),
           "styles guide: a chorus with more sounding parts than the verse is recognised as a lift");

    Engine looseDrums;
    looseDrums.deviceManager.removeAudioCallback (&looseDrums);
    for (auto* name : { "Kick", "Snare", "Hats" })
        looseDrums.addTrack (Track::Type::midi)->name = name;
    check (sg::analyse (looseDrums).contains ("play straight into the master"),
           "styles guide: separate drum tracks without a bus get a concrete Drums bus suggestion");

    Engine dry;
    dry.deviceManager.removeAudioCallback (&dry);
    for (auto* name : { "Bass", "Chords", "Melody" })
    {
        auto* track = dry.addTrack (Track::Type::midi);
        track->name = name;
        dry.addMidiClip (*track, 0.0, 4.0)->notes.push_back ({ 60, 0.0, 1.0, 0.8f });
    }
    check (sg::analyse (dry).contains ("no shared reverb"),
           "styles guide: three sounding tracks without a return get a shared-reverb suggestion");

    for (const auto& track : dance.getTracks())
        for (const auto& send : track->sends)
            send->levelDb.store (sendOffDb);
    check (sg::analyse (dance).contains ("Nothing sends to Reverb yet"),
           "styles guide: a Reverb return with every send off says how to use it");
}

static void runRoutingGuideTests()
{
    using namespace daw;
    namespace rg = daw::routingguide;
    std::cout << "Buses, sends and routing guide" << std::endl;

    bool topicsOk = true, examplesOk = true;

    for (auto topic : rg::allTopics)
    {
        topicsOk = topicsOk && rg::getTopicName (topic).isNotEmpty() && rg::getTopicSummary (topic).isNotEmpty() && rg::explain (topic).length() > 200;
        examplesOk = examplesOk && rg::getExample (topic).size() >= 3;
    }

    check (topicsOk && examplesOk, "routing guide: every topic has a name, a summary, a text and an example of at least 3 boxes");
    check (rg::getExample (rg::Topic::yourSong).size() == rg::getExample (rg::Topic::typical).size() && rg::getExample (rg::Topic::yourSong).size() == 18,
           "routing guide: Your song's routing shows the pop song as its example");

    // Bouncing to audio, and Bounce in place.
    {
        namespace tg = daw::gen::guides;
        const auto bounceText = rg::explain (rg::Topic::bounce), inPlaceText = rg::explain (rg::Topic::inPlace);
        check (rg::getTopicName (rg::Topic::bounce) == "Bouncing to audio" && rg::getTopicName (rg::Topic::inPlace) == "Bounce in place"
                   && rg::getTopicSummary (rg::Topic::bounce) == "Turn the mix, or a few tracks, into a WAV file"
                   && rg::getTopicSummary (rg::Topic::inPlace) == "Print a track's instrument and effects to audio, and keep the original",
               "routing guide: the two bounce topics are named as planned");
        check (bounceText.contains ("Cmd+B") && bounceText.contains ("Selected tracks") && bounceText.contains ("Your solo buttons aren't touched")
                   && inPlaceText.contains ("Cmd+Shift+B") && inPlaceText.contains ("below -80 dB") && inPlaceText.contains ("+7.0 dB of clip gain")
                   && inPlaceText.contains ("up to 4 seconds"),
               "routing guide: the bounce texts hold their shortcuts and numbers");
        check (std::abs (juce::Decibels::decibelsToGain (-80.0f) - 0.0001f) < 1.0e-7f
                   && std::abs (20.0f * std::log10 (2.0f / 0.8913f) - 7.02f) < 0.01f,
               "routing guide: -80 dB is 0.0001 and a peak of 2.0 scaled to -1 dBFS needs +7.02 dB");

        const auto bounceExample = rg::getExample (rg::Topic::bounce);
        int lit = 0, dimmed = 0;
        juce::StringArray litNames;

        for (auto& node : bounceExample)
        {
            lit += node.onSelectedPath ? 1 : 0;
            dimmed += node.silent ? 1 : 0;

            if (node.onSelectedPath)
                litNames.add (node.name);
        }

        litNames.sort (false);
        check (bounceExample.size() == 18 && lit == 3 && dimmed == 15 && litNames.joinIntoString (",") == "Drums,Kick,Snare"
                   && rg::getExampleFileTag (rg::Topic::bounce) == "Kick + Snare.wav" && rg::getExampleFileTag (rg::Topic::solo).isEmpty(),
               "routing guide: the bounce picture lights Kick, Snare and Drums and dims the other 15, with a file tag");

        const auto place = rg::getExample (rg::Topic::inPlace);
        const bool placeOk = place.size() == 4 && place[1].name == "Lead" && place[1].muted && place[1].silent && place[2].name == "Lead (bounced)" && ! place[2].muted
                             && place[1].parent == 0 && place[2].parent == 0 && place[1].sends.size() == 1 && place[2].sends.size() == 1
                             && place[1].sends[0].target == place[2].sends[0].target && place[3].isReturn;
        check (placeOk, "routing guide: the bounce in place picture has Lead (muted) and Lead (bounced) in one bus, both sending to Reverb");

        const auto entries = tg::getEntries();
        const auto routingIndex = (int) std::distance (std::begin (tg::allGuides), std::find (std::begin (tg::allGuides), std::end (tg::allGuides), tg::Guide::routing));
        auto findsRouting = [&] (const juce::String& query)
        {
            const auto found = tg::search (entries, query);
            return ! found.empty() && found.front().entry == routingIndex;
        };
        check (findsRouting ("bounce in place") && findsRouting ("bounce") && findsRouting ("freeze") && findsRouting ("stems") && findsRouting ("multitracks"),
               "theory guides: bounce in place, bounce, freeze, stems and multitracks find the routing guide");
        check (entries[(size_t) routingIndex].topics.indexOf ("Bounce in place") >= 0 && entries[(size_t) routingIndex].topics.indexOf ("Bouncing to audio") >= 0
                   && entries[(size_t) routingIndex].topics.indexOf ("Signal flow") < entries[(size_t) routingIndex].topics.indexOf ("Bouncing to audio")
                   && entries[(size_t) routingIndex].topics.indexOf ("Bounce in place") < entries[(size_t) routingIndex].topics.indexOf ("Your song's routing"),
               "theory guides: the bounce topics come after Signal flow and before Your song's routing");
    }


    // The inspector topic and its two functions.
    {
        namespace tg = daw::gen::guides;
        const auto& all = rg::allTopics;
        check (std::size (all) == 18 && all[8] == rg::Topic::sendHowTo && all[9] == rg::Topic::inspector,
               "routing guide: 18 topics, The inspector directly after Returns in DawIt");
        const auto text = rg::explain (rg::Topic::inspector);
        check (rg::getTopicName (rg::Topic::inspector) == "The inspector"
                   && rg::getTopicSummary (rg::Topic::inspector) == "One track's effects, sends, pan and volume, beside the tracks"
                   && text.length() > 400 && text.contains ("View > Inspector") && text.contains ("Insert or send?"),
               "routing guide: the inspector topic has its name, summary and text");
        check (rg::explain (rg::Topic::sendHowTo).contains ("The inspector (View > Inspector, I) shows the same knobs")
                   && rg::explain (rg::Topic::preFader).contains ("in the mixer or the inspector, or a send in the device bar"),
               "routing guide: Returns in DawIt and Pre-fader and post-fader mention the inspector");
        const auto example = rg::getExample (rg::Topic::inspector);
        bool vocalLit = false, bassLit = true;

        for (auto& n : example)
        {
            if (n.name == "Vocal")
                vocalLit = n.onSelectedPath;

            if (n.name == "Bass")
                bassLit = n.onSelectedPath;
        }

        check (! example.empty() && vocalLit && ! bassLit, "routing guide: the inspector picture lights Vocal's path and not Bass");

        const auto entries = tg::getEntries();
        const auto routingIndex = (int) std::distance (std::begin (tg::allGuides), std::find (std::begin (tg::allGuides), std::end (tg::allGuides), tg::Guide::routing));
        const auto found = tg::search (entries, "inspector");
        check (! found.empty() && found.front().entry == routingIndex, "theory guides: searching for inspector finds Buses, sends and routing");

        // getSendTargets and describeSends.
        Tracks tracks;
        auto add = [&tracks] (Track::Type type, const char* name) -> Track*
        {
            tracks.push_back (std::make_unique<Track> (type, name, juce::Colours::red));
            return tracks.back().get();
        };
        check (rg::describeSends (nullptr, tracks).isEmpty(), "routing guide: describeSends is quiet for a song with no tracks");
        auto* lead = add (Track::Type::audio, "Lead vocal");
        auto* bus = add (Track::Type::bus, "Drums");
        check (rg::describeSends (lead, tracks) == "Your song has no return yet. Track > New Return Track > Reverb adds one, and every track gets a knob for it.",
               "routing guide: describeSends with no returns");
        std::vector<Track*> returns;

        for (int i = 0; i < 6; ++i)
            returns.push_back (add (Track::Type::fxReturn, i == 0 ? "Reverb" : i == 1 ? "Delay" : "Extra"));

        const auto targets = routing::getSendTargets (lead, tracks);
        check (targets.size() == 6 && routing::getSendTargets (bus, tracks).size() == 6 && routing::getSendTargets (returns[0], tracks).empty()
                   && routing::getSendTargets (nullptr, tracks).empty() && targets[0] == returns[0] && targets[5] == returns[5],
               "routing: getSendTargets gives every return for a track and a bus, in edit order, and none for a return or the master");
        check (rg::describeSends (lead, tracks) == "Lead vocal doesn't send to a return yet. Drag up a knob under Sends in the inspector to add some reverb.",
               "routing guide: describeSends with returns but no sends");

        lead->sends.push_back (std::make_unique<Send> (*returns[0]));
        lead->sends.back()->levelDb = -12.0f;
        lead->sends.push_back (std::make_unique<Send> (*returns[1]));
        lead->sends.back()->levelDb = -18.0f;
        lead->sends.back()->preFader = true;
        lead->sends.push_back (std::make_unique<Send> (*returns[2]));
        lead->sends.back()->levelDb = -60.0f;
        const auto line = rg::describeSends (lead, tracks);
        check (line == "Lead vocal sends to Reverb at -12.0 dB, after the fader, and to Delay at -18.0 dB, before the fader."
                   && ! line.contains ("Extra"),
               "routing guide: describeSends lists the sends that are on, with levels and where they are taken, and skips a send at Off");
        lead->sends.back()->levelDb = -6.0f;
        check (rg::describeSends (lead, tracks).contains ("Delay at -18.0 dB, before the fader, and to Extra at -6.0 dB, after the fader"),
               "routing guide: describeSends joins three sends with commas and 'and to'");

        bus->sends.push_back (std::make_unique<Send> (*returns[0]));
        bus->sends.back()->levelDb = -8.9f;
        const auto fed = rg::describeSends (returns[0], tracks);
        check (fed == "Reverb is fed by Lead vocal at -12.0 dB and Drums at -8.9 dB." && rg::describeSends (returns[3], tracks) == "Nothing sends to Extra yet."
                   && rg::describeSends (nullptr, tracks) == "The master gets every track, bus and return. It has no sends.",
               "routing guide: describeSends for a return with two senders, a return with none and the master");
    }

    // The sums.
    auto near = [] (float a, float b) { return std::abs (a - b) < 0.01f; };
    check (near (rg::sumDb ({ -6.0f, -6.0f }, false), -2.99f) && near (rg::sumDb ({ -6.0f, -6.0f }, true), 0.02f)
               && near (rg::sumDb ({ -10.0f, -10.0f, -10.0f, -10.0f }, false), -3.98f) && near (rg::sumDb ({ -10.0f, -10.0f, -10.0f, -10.0f }, true), 2.04f)
               && near (rg::sumDb ({ 0.0f }, false), 0.0f) && near (rg::sumDb ({ -6.0f, -100.0f }, false), -6.0f)
               && near (rg::sumDb ({ 0.0f, 0.0f }, false), 3.01f) && near (rg::sumDb ({ 0.0f, 0.0f }, true), 6.02f) && near (rg::sumDb ({ 0, 0, 0, 0, 0, 0, 0, 0 }, false), 9.03f),
           "routing guide: levels add up as powers (+3.01 dB for two) or amplitudes (+6.02 dB for two)");

    // Families and names.
    auto familyOf = [] (const char* name, bool midi = true)
    {
        Track t (midi ? Track::Type::midi : Track::Type::audio, name, juce::Colours::red);
        return rg::getFamily (t);
    };

    using F = rg::Family;
    check (familyOf ("Kick") == F::drums && familyOf ("Bass drum") == F::drums && familyOf ("Hi-hat") == F::drums && familyOf ("Overheads") == F::drums
               && familyOf ("Bass") == F::bass && familyOf ("Bass guitar") == F::bass && familyOf ("Sub") == F::bass
               && familyOf ("BV 2") == F::vocals && familyOf ("Lead Vox") == F::vocals && familyOf ("Gtr L") == F::guitars && familyOf ("Rhodes") == F::keys
               && familyOf ("Strings") == F::none && familyOf ("MIDI 3") == F::none,
           "routing guide: tracks are told apart by the words in their names");
    {
        Track t (Track::Type::midi, "MIDI 3", juce::Colours::red);
        t.builtInSound = BuiltInSound::drums;
        Track audio (Track::Type::audio, "Audio 1", juce::Colours::red);
        audio.builtInSound = BuiltInSound::drums;
        check (rg::getFamily (t) == F::drums && rg::getFamily (audio) == F::none, "routing guide: a MIDI track on the Drums sound is drums whatever its name");
    }

    {
        BusTree t;
        check (rg::suggestBusName ({ t.kick, t.snare, t.hats }, t.engine.getTracks()) == "Bus 1" && rg::suggestBusName ({ t.kick, t.bass }, t.engine.getTracks()) == "Bus 1",
               "routing guide: Drums is taken, so the suggestion is Bus 1");
        Engine flat;
        flat.deviceManager.removeAudioCallback (&flat);
        auto* k = flat.addTrack (Track::Type::midi);
        auto* s2 = flat.addTrack (Track::Type::midi);
        auto* b = flat.addTrack (Track::Type::midi);
        k->name = "Kick";
        s2->name = "Snare";
        b->name = "Bass";
        check (rg::suggestBusName ({ k, s2 }, flat.getTracks()) == "Drums" && rg::suggestBusName ({ k, b }, flat.getTracks()) == "Bus 1",
               "routing guide: Kick and Snare are suggested as Drums, Kick and Bass as Bus 1");
    }

    // The layout of the pop song.
    {
        const auto nodes = rg::getExample (rg::Topic::typical);
        const auto points = rg::layOut (nodes);
        bool busesRight = points.size() == nodes.size() + 1, middle = true, unique = true, masterRight = true;

        for (size_t i = 0; busesRight && i < nodes.size(); ++i)
        {
            masterRight = masterRight && points.back().x > points[i].x;

            for (size_t j = i + 1; j < points.size(); ++j)
                unique = unique && points[i] != points[j];

            if (nodes[i].parent >= 0)
                busesRight = busesRight && points[(size_t) nodes[i].parent].x > points[i].x;

            if (nodes[i].isBus)
            {
                float lo = 1.0e9f, hi = -1.0e9f;

                for (size_t k = 0; k < nodes.size(); ++k)
                    if (nodes[k].parent == (int) i)
                    {
                        lo = juce::jmin (lo, points[k].y);
                        hi = juce::jmax (hi, points[k].y);
                    }

                middle = middle && std::abs (points[i].y - (lo + hi) * 0.5f) < 1.0e-4f;
            }
        }

        std::set<float> leafRows;
        int leaves = 0;

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const bool leaf = std::none_of (nodes.begin(), nodes.end(), [i] (auto& n) { return n.parent == (int) i; });

            if (leaf)
            {
                ++leaves;
                leafRows.insert (points[i].y);
            }
        }

        check (busesRight && masterRight && middle && unique && (int) leafRows.size() == leaves,
               "routing guide: the layout puts buses right of their tracks, in the middle of their rows, the master last, with no two boxes together");
    }

    // The advice on a made-up song.
    {
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        std::map<juce::String, Track*> byName;

        for (auto* name : { "Kick", "Snare", "Hats", "Bass", "Lead Vox", "BV 1", "BV 2" })
        {
            auto* tr = song.addTrack (Track::Type::midi);
            tr->name = name;
            byName[name] = tr;
        }

        auto advice = rg::analyse (song.getTracks(), {});
        check (advice.size() == 2 && advice[0].action == rg::Advice::Action::createBus && advice[0].busName == "Drums"
                   && advice[0].text.contains ("Kick, Snare and Hats") && advice[1].action == rg::Advice::Action::createBus && advice[1].busName == "Vocals"
                   && advice[1].text.contains ("Lead Vox, BV 1 and BV 2") && advice[0].tracks.size() == 3,
               "routing guide: it suggests a Drums bus and a Vocals bus, and nothing for Bass alone");

        for (auto& a : advice)
            std::cout << "  advice: " << a.text << std::endl;

        song.createBus (advice[0].tracks, advice[0].busName);
        advice = rg::analyse (song.getTracks(), {});
        const auto glue = std::find_if (advice.begin(), advice.end(), [] (auto& a) { return a.action == rg::Advice::Action::addGlue; });
        check (glue != advice.end() && glue->text.contains ("2.0:1") && glue->text.contains ("30 ms") && glue->tracks.size() == 1 && glue->tracks[0]->name == "Drums"
                   && std::none_of (advice.begin(), advice.end(), [] (auto& a) { return a.action == rg::Advice::Action::createBus && a.busName == "Drums"; }),
               "routing guide: with the Drums bus made, it suggests a gentle compressor with the Drum bus settings");
        std::cout << "  advice: " << (glue != advice.end() ? glue->text : juce::String()) << std::endl;

        // Hot levels only while playing.
        rg::Levels levels;
        levels.busInputDb[glue->tracks[0]] = 2.1f;
        const auto has = [&] (const juce::String& s) { return std::any_of (advice.begin(), advice.end(), [&s] (auto& a) { return a.text.contains (s); }); };
        advice = rg::analyse (song.getTracks(), levels);
        const bool stopped = has ("+2.1 dB");
        levels.playing = true;
        advice = rg::analyse (song.getTracks(), levels);
        const bool quietWithoutEffects = ! has ("+2.1 dB");
        juce::String pluginError;
        song.addPlugin (*glue->tracks[0], BuiltInCompressor::getDescription(), pluginError);
        advice = rg::analyse (song.getTracks(), levels);
        check (! stopped && quietWithoutEffects && has ("+2.1 dB"),
               "routing guide: a hot bus with effects is pointed out while the song plays, not while it is stopped, and a bus without effects is left alone " + pluginError);

        // The explanation of the song.
        BusTree t;
        const auto text = rg::explainSong (t.engine.getTracks());
        check (text.contains ("7 tracks and 3 buses") && text.contains ("Drums takes 3") && text.contains ("Vocals takes 2") && text.contains ("Backing takes 2"),
               "routing guide: the song's explanation counts tracks and buses (" + text + ")");
    }

    {
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        auto* vox = song.addTrack (Track::Type::midi);
        vox->name = "Lead Vox";
        auto* only = song.createBus ({ vox }, "Lead");
        const auto alone = rg::analyse (song.getTracks(), {});
        check (alone.size() == 1 && alone[0].text.contains ("to itself"), "routing guide: a bus with one track is pointed out");
        song.removeTrack (vox);
        const auto none = rg::analyse (song.getTracks(), {});
        check (only != nullptr && none.size() == 1 && none[0].text.contains ("empty"), "routing guide: an empty bus is pointed out");

        Engine nested;
        nested.deviceManager.removeAudioCallback (&nested);
        auto* x = nested.addBus();
        auto* y = nested.addBus();
        auto* z = nested.addBus();
        nested.moveTrack (*y, x);
        nested.moveTrack (*z, y);
        auto* strings = nested.addTrack (Track::Type::midi);
        nested.moveTrack (*strings, z);
        const auto deep = rg::analyse (nested.getTracks(), {});
        check (strings->bus == z && std::any_of (deep.begin(), deep.end(), [] (auto& a) { return a.text.contains ("Two levels"); }),
               "routing guide: buses three levels deep are pointed out");
    }

    {
        Engine song;
        song.deviceManager.removeAudioCallback (&song);

        for (auto* name : { "Kick", "Bass", "Lead Vox", "Gtr", "Piano" })
            song.addTrack (Track::Type::midi)->name = name;

        const auto advice = rg::analyse (song.getTracks(), {});
        check (advice.size() == 1 && advice[0].action == rg::Advice::Action::none && advice[0].text.startsWith ("Every track goes straight to the master"),
               "routing guide: with one track of each kind and no buses, it says every track goes to the master");
    }

    // Drums playing into another kind of bus, and a second group when the kit's bus exists.
    {
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        std::map<juce::String, Track*> byName;

        for (auto* name : { "Lead Vox", "BV 1", "Kick", "Snare" })
        {
            byName[name] = song.addTrack (Track::Type::midi);
            byName[name]->name = name;
        }

        auto* vocals = song.createBus ({ byName["Lead Vox"], byName["BV 1"] }, "Vocals");
        song.moveTrack (*byName["Kick"], vocals);
        song.moveTrack (*byName["Snare"], vocals);
        auto* outer = song.addBus();
        song.moveTrack (*vocals, outer);
        outer->name = "Mix";

        auto advice = rg::analyse (song.getTracks(), {});
        const auto drums = std::find_if (advice.begin(), advice.end(), [] (auto& a) { return a.action == rg::Advice::Action::createBus; });
        check (drums != advice.end() && drums->busName == "Drums" && drums->bus == vocals && drums->text.contains ("next to Vocals rather than inside it"),
               "routing guide: drums in a Vocals bus get a Drums bus next to Vocals, and the advice says so");

        const auto made = song.createBusBeside (drums->tracks, *drums->bus, drums->busName);
        check (made != nullptr && made->name == "Drums" && made->bus == outer && byName["Kick"]->bus == made && byName["Snare"]->bus == made
                   && vocals->bus == outer && byName["Lead Vox"]->bus == vocals && song.indexOf (made) == routing::getBlockEnd (song.getTracks(), song.indexOf (vocals)),
               "routing guide: the action puts the Drums bus beside Vocals, in the bus Vocals is in, and takes the drums out of Vocals");
        const auto settled = rg::analyse (song.getTracks(), {});
        check (std::none_of (settled.begin(), settled.end(), [] (auto& a) { return a.action != rg::Advice::Action::none && a.action != rg::Advice::Action::addGlue; }),
               "routing guide: with the drums out of Vocals, no routing is suggested");

        // Drums beside Vocals at the top level: the new bus goes to the top level too.
        Engine flat;
        flat.deviceManager.removeAudioCallback (&flat);
        auto* v1 = flat.addTrack (Track::Type::midi);
        v1->name = "Lead Vox";
        auto* k1 = flat.addTrack (Track::Type::midi);
        k1->name = "Kick";
        auto* s1 = flat.addTrack (Track::Type::midi);
        s1->name = "Snare";
        auto* voc = flat.createBus ({ v1, k1, s1 }, "Vocals");
        const auto mixed = rg::analyse (flat.getTracks(), {});
        auto* top = flat.createBusBeside ({ k1, s1 }, *voc, "Drums");
        check (voc->bus == nullptr && top != nullptr && top->bus == nullptr && k1->bus == top && v1->bus == voc && flat.indexOf (top) == flat.indexOf (voc) + 2
                   && std::any_of (mixed.begin(), mixed.end(), [&] (auto& a) { return a.bus == voc && a.tracks.size() == 2; }),
               "routing guide: a Drums bus made beside a top-level Vocals bus is top-level too");

        // Tracks that sit in different places are advised separately (a group is one place).
        Engine split;
        split.deviceManager.removeAudioCallback (&split);
        auto* ka = split.addTrack (Track::Type::midi);
        ka->name = "Kick";
        auto* sa = split.addTrack (Track::Type::midi);
        sa->name = "Snare";
        auto* vb = split.addTrack (Track::Type::midi);
        vb->name = "Lead Vox";
        auto* ha = split.addTrack (Track::Type::midi);
        ha->name = "Hats";
        auto* tm = split.addTrack (Track::Type::midi);
        tm->name = "Tom";
        auto* band = split.createBus ({ vb, ha, tm }, "Vocals");
        const auto mixedPlaces = rg::analyse (split.getTracks(), {});
        const auto count = std::count_if (mixedPlaces.begin(), mixedPlaces.end(), [] (auto& a) { return a.action == rg::Advice::Action::createBus; });
        check (band != nullptr && count == 2 && std::count_if (mixedPlaces.begin(), mixedPlaces.end(), [&] (auto& a) { return a.action == rg::Advice::Action::createBus && a.bus == nullptr && a.tracks.size() == 2; }) == 1,
               "routing guide: drum tracks in the master and drum tracks in Vocals are two pieces of advice, each with its own place");

        // A Drums bus exists: more drum tracks are told to move into it.
        Engine second;
        second.deviceManager.removeAudioCallback (&second);
        auto* k = second.addTrack (Track::Type::midi);
        k->name = "Kick";
        auto* sn = second.addTrack (Track::Type::midi);
        sn->name = "Snare";
        auto* hh = second.addTrack (Track::Type::midi);
        hh->name = "Hats";
        auto* cl = second.addTrack (Track::Type::midi);
        cl->name = "Clap";
        auto* kit = second.createBus ({ k, sn }, "Drums");
        const auto join = rg::analyse (second.getTracks(), {});
        const auto move = std::find_if (join.begin(), join.end(), [] (auto& a) { return a.action == rg::Advice::Action::moveIntoBus; });
        check (move != join.end() && move->bus == kit && move->tracks.size() == 2 && move->text.contains ("Move them into Drums")
                   && std::none_of (join.begin(), join.end(), [] (auto& a) { return a.action == rg::Advice::Action::createBus; }),
               "routing guide: with a Drums bus, two more drum tracks are told to move into Drums, not to make Bus 1");

        for (auto* t : move->tracks)
            second.moveTrack (*t, move->bus);

        const auto after = rg::analyse (second.getTracks(), {});
        check (hh->bus == kit && cl->bus == kit && std::none_of (after.begin(), after.end(), [] (auto& a) { return a.action == rg::Advice::Action::moveIntoBus || a.action == rg::Advice::Action::createBus; }),
               "routing guide: after moving them into Drums, no routing is suggested");
    }

    // Text facts from the check by dawTheory.
    {
        using T = rg::Topic;
        check (rg::describeMixBusSettings().contains ("80 Hz"), "routing guide: the mix bus settings name the 80 Hz sidechain filter");
        check (rg::explain (T::mixBus).contains ("24-bit"), "routing guide: the mix bus text says a 24-bit bounce clips too");
        check (rg::explain (T::levels).contains ("doesn't make the mix any louder"), "routing guide: Levels add up says a bus doesn't make the mix louder");
        check (rg::explain (T::why).contains ("VCA") && rg::explain (T::why).contains ("stem"), "routing guide: What a bus is for tells a VCA group and a stem apart");
        check (rg::explain (T::solo).contains ("Mute always wins"), "routing guide: the solo text says mute always wins");
    }

    // The mix bus (master) topic.
    {
        using T = rg::Topic;
        const auto text = rg::explain (T::mixBus);
        const auto settings = rg::describeMixBusSettings();
        check (rg::getTopicName (T::mixBus) == "The mix bus (the master)" && rg::getTopicSummary (T::mixBus).isNotEmpty() && text.length() > 1500
                   && rg::getExample (T::mixBus).size() == 18 && std::find (std::begin (rg::allTopics), std::end (rg::allTopics), T::mixBus) != std::end (rg::allTopics),
               "routing guide: the mix bus topic has a name, a summary, a text and the pop song as its example");
        check (settings.contains ("1.5:1") && settings.contains ("30 ms") && settings.contains ("200 ms") && settings.contains ("1 to 2 dB") && text.contains (settings),
               "routing guide: the mix bus settings come from the Mix bus starting point (" + settings + ")");
        check (text.contains ("-6 dBFS") && text.contains ("no effects chain") && text.contains ("limiter") && text.contains ("1.5:1 to 2:1") && text.contains ("10 to 30 ms"),
               "routing guide: the mix bus text gives the -6 dB headroom, the compressor numbers, what not to do and what the master has today");
        check (juce::exactlyEqual (rg::getHeadroomDb (-6.0f), 6.0f) && juce::exactlyEqual (rg::getHeadroomDb (-0.5f), 0.5f) && rg::getHeadroomDb (1.0f) < 0.0f
                   && juce::exactlyEqual (rg::mixBusTargetPeakDb, -6.0f),
               "routing guide: headroom is the distance from the peak to 0 dBFS, 6 dB for a peak at -6 dB");
        check (rg::describeMasterLevel (-8.0f).contains ("8.0 dB of headroom") && rg::describeMasterLevel (-8.0f).contains ("enough")
                   && rg::describeMasterLevel (-3.0f).contains ("a bit hot") && rg::describeMasterLevel (1.2f).contains ("clips")
                   && rg::describeMasterLevel (-100.0f).contains ("Play the song"),
               "routing guide: the master's level is described as enough, a bit hot, clipping or silent");

        // The advice on the master's peak, only while playing.
        Engine song;
        song.deviceManager.removeAudioCallback (&song);
        song.addTrack (Track::Type::midi)->name = "Lead";
        rg::Levels levels;
        levels.masterPeakDb = 1.5f;
        const auto has = [&] (const juce::String& s)
        {
            const auto advice = rg::analyse (song.getTracks(), levels);
            return std::any_of (advice.begin(), advice.end(), [&s] (auto& a) { return a.text.contains (s) && a.topic == T::mixBus; });
        };
        const bool stopped = has ("master peaks");
        levels.playing = true;
        const bool clips = has ("it clips");
        levels.masterPeakDb = -3.0f;
        const bool hot = has ("3.0 dB of headroom");
        levels.masterPeakDb = -8.0f;
        check (! stopped && clips && hot && ! has ("master peaks"),
               "routing guide: the song's advice says when the master clips or has too little headroom, only while playing");
    }

    const auto glue = rg::describeGlueSettings();
    check (glue.contains ("2.0:1") && glue.contains ("30 ms") && glue.contains ("150 ms") && rg::explain (rg::Topic::glue).contains (glue),
           "routing guide: the glue settings come from the Drum bus starting point (" + glue + ")");
}


//==============================================================================
// Sing this: a computer voice sings a melody and lyrics.

namespace
{
/** A Singer that returns a sine at the pitch of every frame, so everything but the real models can be tested. */
struct FakeSinger final : public daw::sing::Singer
{
    std::optional<juce::AudioBuffer<float>> sing (const daw::sing::Phrase& p, int, const daw::sing::CancelFn&, juce::String&) override
    {
        ++calls;
        juce::AudioBuffer<float> b (1, p.getNumFrames() * daw::sing::hopSize);
        double phase = 0.0;

        for (int f = 0; f < p.getNumFrames(); ++f)
        {
            for (int s = 0; s < daw::sing::hopSize; ++s)
            {
                b.setSample (0, f * daw::sing::hopSize + s, 0.3f * (float) std::sin (phase));
                phase += juce::MathConstants<double>::twoPi * p.f0[(size_t) f] / daw::sing::sampleRate;
            }
        }

        return b;
    }

    int calls = 0;
};

daw::LyricLine makeLine (double start, double end, std::vector<std::tuple<const char*, double, bool>> syllables)
{
    daw::LyricLine line;
    line.startBeat = start;
    line.endBeat = end;

    for (auto& [text, beat, endsWord] : syllables)
        line.syllables.push_back ({ text, endsWord, true, beat });

    return line;
}
}

/** The timing report of the example song's lead melody up to bar 13, laid out with `voice` (null: the built-in
    dictionary) and `singer` (null: fixed consonant lengths). */
static daw::gen::singthis::Score scoreExampleSong (const daw::sing::Voice* voice, daw::sing::Singer* singer,
                                                   daw::gen::singthis::Melody& melodyOut)
{
    using namespace daw;
    namespace st = gen::singthis;
    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* lead = examples::buildPopSong (engine);
    std::vector<st::SourceNote> source;

    for (auto& c : lead->clips)
        if (c->kind == Clip::Kind::midi)
            for (auto& n : static_cast<MidiClip&> (*c).notes)
                if (c->startBeat + n.start < 52.0)
                    source.push_back ({ n.pitch, c->startBeat + n.start, n.length });

    melodyOut = st::buildMelody (source, engine.getTempoMap());
    const auto mode = st::chooseMode (engine.getLyrics(), melodyOut);
    return st::buildScore (melodyOut, st::pair (melodyOut, engine.getLyrics(), mode, voice), voice, {}, singer);
}

namespace
{
/** A Singer that only has a duration model: every consonant lasts `consonantSeconds`, as a voice that sings slowly would say. */
struct FakeDurationSinger final : public daw::sing::Singer
{
    explicit FakeDurationSinger (double c) : consonantSeconds (c) {}

    std::vector<double> predictDurations (const daw::sing::Phrase& p, const daw::sing::CancelFn&) override
    {
        std::vector<double> out;
        for (auto& ph : p.phonemes)
            out.push_back (ph.vowel || ph.symbol == "SP" || ph.symbol == "AP" ? 0.2 : consonantSeconds);
        return out;
    }

    std::optional<juce::AudioBuffer<float>> sing (const daw::sing::Phrase&, int, const daw::sing::CancelFn&, juce::String&) override { return std::nullopt; }

    double consonantSeconds;
};

/** The words of a made-up fast tune, one on each note of `noteBeats` beats at `bpm`, with consonant clusters. */
daw::gen::singthis::TimingReport measureFastTune (double bpm, double noteBeats, daw::sing::Singer* singer, int words = 16, double* endErrorMs = nullptr)
{
    using namespace daw;
    namespace st = gen::singthis;
    static const char* const tune[] = { "strengths", "next", "light", "street", "and", "glimpsed", "the", "night", "twelfths", "the", "stand", "ring", "past", "pink", "bright", "dreams" };
    const TempoMap tempo (bpm);
    std::vector<st::SourceNote> notes;
    std::vector<std::tuple<const char*, double, bool>> syllables;

    for (int i = 0; i < words; ++i)
    {
        notes.push_back ({ 60 + (i % 5), i * noteBeats, noteBeats });
        syllables.push_back ({ tune[i % 16], i * noteBeats, true });
    }

    Lyrics lyrics;
    lyrics.lines.push_back (makeLine (0.0, words * noteBeats, syllables));
    const auto melody = st::buildMelody (notes, tempo);
    const auto score = st::buildScore (melody, st::pair (melody, lyrics, st::Mode::asTimed, nullptr), nullptr, {}, singer);

    // How far after the last note the last sound ends (not counting the silence at the end).
    if (endErrorMs != nullptr)
    {
        const auto& phrase = score.phrases.back();
        const auto last = phrase.phonemes.size() - 2;
        *endErrorMs = (st::getPhonemeEndSeconds (phrase, last) - melody.notes.back().endSeconds) * 1000.0;
    }

    return st::measureTiming (score);
}
}

static void runSingThisTests()
{
    using namespace daw;
    namespace st = gen::singthis;
    namespace pron = gen::pronunciation;
    std::cout << "Sing this" << std::endl;

    const double frameTol = sing::frameSeconds * 1.01;
    const TempoMap tempo120 (120.0), tempo60 (60.0);
    auto melodyOf = [&] (std::vector<st::SourceNote> notes, const TempoMap& tempo = TempoMap (120.0)) { return st::buildMelody (std::move (notes), tempo); };
    auto adviceHas = [] (const std::vector<st::Advice>& advice, st::Rule rule, const juce::String& text = {})
    {
        for (auto& a : advice)
            if (a.rule == rule && (text.isEmpty() || a.text == text))
                return true;

        return false;
    };

    // The small YAML reader.
    {
        const auto r = yaml::parse ("# a comment\nphonemes: ph.json  # trailing\nname: 'it''s'\nspeakers:\n- a/b\n- \"c d\"\nlist: [1, 2, 'three']\n"
                                    "nested:\n  key: value\n  flow: {symbol: SP, type: vowel}\n  items:\n    - {symbol: SP, type: vowel}\n"
                                    "    - grapheme: x\n      phonemes: [a, b]\nflag: true\nnum: 44100\nratio: 0.6\n");
        check (r.ok() && r.root.getString ("phonemes") == "ph.json" && r.root.getString ("name") == "it's" && r.root.getInt ("num") == 44100
                   && r.root.getBool ("flag") && std::abs (r.root.getDouble ("ratio") - 0.6) < 1.0e-9,
               "yaml: maps, scalars, comments and quotes (" + r.error + ")");
        const auto* speakers = r.root.find ("speakers");
        const auto* list = r.root.find ("list");
        const auto* nested = r.root.find ("nested");
        check (speakers != nullptr && speakers->items.size() == 2 && speakers->items[1].scalar == "c d"
                   && list != nullptr && list->items.size() == 3 && list->items[2].scalar == "three"
                   && nested != nullptr && nested->getString ("key") == "value" && nested->find ("flow") != nullptr
                   && nested->find ("flow")->getString ("type") == "vowel",
               "yaml: block lists, flow lists and flow maps, nested maps");
        const auto* items = nested != nullptr ? nested->find ("items") : nullptr;
        check (items != nullptr && items->items.size() == 2 && items->items[1].getString ("grapheme") == "x"
                   && items->items[1].find ("phonemes") != nullptr && items->items[1].find ("phonemes")->items.size() == 2,
               "yaml: a list of maps, one in flow style and one in block style");
        check (! yaml::parse ("a:\n\tb: 1\n").ok() && yaml::parse ("a:\n\tb: 1\n").error.startsWith ("line 2")
                   && ! yaml::parse ("a: &x 1\nb: *x\n").ok() && ! yaml::parse ("a: |\n  text\n").ok()
                   && ! yaml::parse ("a: [1,\n  2]\n").ok() && ! yaml::parse ("a: 1\na: 2\n").ok(),
               "yaml: tabs, anchors, block scalars, multi-line lists and repeated keys fail with the line named");
    }

    // The numbers of the plan's Theory.
    {
        check (std::abs (st::midiToHz (69) - 440.0) < 0.01 && std::abs (st::midiToHz (60) - 261.63) < 0.01
                   && std::abs (st::midiToHz (62) - 293.66) < 0.01 && std::abs (st::midiToHz (64) - 329.63) < 0.01,
               "sing: f0 for MIDI 69, 60, 62 and 64 is 440.00, 261.63, 293.66 and 329.63 Hz");
        check (std::abs (sing::frameSeconds * 1000.0 - 11.61) < 0.01 && std::abs (1.0 / sing::frameSeconds - 86.13) < 0.01,
               "sing: a frame is 512 / 44,100 s = 11.61 ms, 86.13 a second");
    }

    // Pronunciation.
    {
        check (pron::getAllCmuPhonemes().size() == 39 && sing::getCmuVowels().size() == 15 && sing::getCmuConsonants().size() == 24
                   && std::abs (pron::getCmuEntryCount() - 135000) < 1000 && std::abs (pron::getCmuWordCount() - 126000) < 1000,
               "pronunciation: the dictionary has 39 sounds (15 vowels, 24 consonants) and about 135,000 entries for " + juce::String (pron::getCmuWordCount()) + " words");
        check (pron::lookupCmu ("night") == std::vector<juce::String> ({ "n", "ay", "t" }) && pron::lookupCmu ("Night,").size() == 3
                   && pron::lookupCmu ("though") == std::vector<juce::String> ({ "dh", "ow" })
                   && pron::lookupCmu ("through") == std::vector<juce::String> ({ "th", "r", "uw" })
                   && pron::lookupCmu ("tough") == std::vector<juce::String> ({ "t", "ah", "f" }),
               "pronunciation: night is n ay t, and though, through and tough end alike but sound different");
        auto countVowels = [] (const std::vector<juce::String>& sounds)
        {
            int n = 0;
            for (auto& s : sounds)
                n += pron::isCmuVowel (s) ? 1 : 0;
            return n;
        };
        const auto together = pron::lookupCmu ("together"), strengths = pron::lookupCmu ("strengths");
        check (countVowels (together) == 3 && countVowels (strengths) == 1 && (int) strengths.size() - countVowels (strengths) >= 5,
               "pronunciation: together has 3 vowels; strengths has 1 vowel and at least 5 consonants");
        const auto zorbly = pron::pronounce ("zorbly", nullptr), known = pron::pronounce ("night", nullptr);
        check (zorbly.source == pron::Source::guessed && ! zorbly.sounds.empty() && known.source == pron::Source::cmudict
                   && pron::pronounce ("--", nullptr).sounds.empty(),
               "pronunciation: an unknown word is guessed and flagged, a known one is not, a dash has no sounds");
    }

    // The voice's folder: a missing file is named, and a voice without English is refused.
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("dawit-voice-test");
        folder.deleteRecursively();
        const auto voiceDir = folder.getChildFile ("MyVoice");
        voiceDir.getChildFile ("dsvocoder/v").createDirectory();
        voiceDir.getChildFile ("dsconfig.yaml").replaceWithText ("phonemes: phonemes.json\nacoustic: acoustic.onnx\nvocoder: v\nsample_rate: 44100\nhop_size: 512\n");
        voiceDir.getChildFile ("phonemes.json").replaceWithText ("{\"SP\": 0, \"AP\": 1, \"ay\": 2, \"iy\": 3, \"t\": 4}");
        voiceDir.getChildFile ("dsvocoder/v/vocoder.yaml").replaceWithText ("name: v\nmodel: model.onnx\n");
        voiceDir.getChildFile ("dsvocoder/v/model.onnx").replaceWithText ("x");

        auto missing = sing::loadVoice (voiceDir, folder.getChildFile ("Vocoders"));
        check (! missing.voice.has_value() && missing.error.contains ("acoustic.onnx is missing"),
               "voice: a missing acoustic model is named (" + missing.error + ")");

        voiceDir.getChildFile ("acoustic.onnx").replaceWithText ("x");
        auto ok = sing::loadVoice (voiceDir, folder.getChildFile ("Vocoders"));
        check (ok.voice.has_value() && ok.voice->hasEnglish && ok.voice->mapPhoneme ("ay") == "ay" && ok.voice->mapPhoneme ("zh").isEmpty(),
               "voice: a complete voice with English loads, and a sound it lacks has no mapping (" + ok.error + ")");

        voiceDir.getChildFile ("phonemes.json").replaceWithText ("{\"SP\": 0, \"AP\": 1, \"a\": 2, \"i\": 3, \"k\": 4}");
        auto noEnglish = sing::loadVoice (voiceDir, folder.getChildFile ("Vocoders"));
        check (! noEnglish.voice.has_value() && noEnglish.error.contains ("no English"), "voice: a voice with no English is refused (" + noEnglish.error + ")");

        // Packed one folder deeper, as zips often are: still found.
        voiceDir.getChildFile ("phonemes.json").replaceWithText ("{\"SP\": 0, \"AP\": 1, \"ay\": 2, \"iy\": 3, \"t\": 4}");
        const auto packed = folder.getChildFile ("Packed");
        voiceDir.copyDirectoryTo (packed.getChildFile ("Inner"));
        check (sing::findVoiceRoot (packed) == packed.getChildFile ("Inner"), "voice: a voice inside one extra folder is found");

        // Installing from a folder copies it; a folder that isn't a voice is refused and leaves nothing behind.
        const auto voices = folder.getChildFile ("Voices");
        auto installed = sing::installVoice (packed, voices);
        const auto notVoice = folder.getChildFile ("Empty");
        notVoice.createDirectory();
        notVoice.getChildFile ("readme.txt").replaceWithText ("hello");
        auto refused = sing::installVoice (notVoice, voices);
        check (installed.error.isEmpty() && sing::listVoices (voices).size() == 1 && refused.error.contains ("isn't a DiffSinger voice") && sing::listVoices (voices).size() == 1,
               "voice: Add a voice copies a voice folder in and refuses one that isn't a voice (" + installed.error + refused.error + ")");
        folder.deleteRecursively();
    }

    // Melody and pairing.
    {
        // The top note wherever notes overlap.
        const auto overlapped = melodyOf ({ { 60, 0.0, 2.0 }, { 64, 1.0, 2.0 } });
        check (overlapped.overlaps == 1 && overlapped.notes.size() == 2 && overlapped.notes[0].pitch == 60 && std::abs (overlapped.notes[0].endBeat - 1.0) < 1.0e-9
                   && overlapped.notes[1].pitch == 64 && std::abs (overlapped.notes[1].endBeat - 3.0) < 1.0e-9,
               "melody: where notes overlap, the top one is sung");

        // A syllable 0.10 beat after a note is paired, 0.20 is not.
        auto pairedAt = [&] (double offset)
        {
            Lyrics l;
            l.lines.push_back (makeLine (0.0, 2.0, { { "la", offset, true } }));
            const auto m = melodyOf ({ { 60, 0.0, 1.0 } });
            return st::pair (m, l, st::Mode::asTimed, nullptr).notes[0].starts;
        };
        check (pairedAt (0.10) && ! pairedAt (0.20), "pairing: a syllable 0.10 beat after a note is paired, 0.20 beat after it is not");

        // As timed holds a syllable over the notes inside it (a melisma).
        Lyrics oh;
        oh.lines.push_back (makeLine (0.0, 4.0, { { "oh", 0.0, true }, { "yeah", 3.0, true } }));
        const auto run = melodyOf ({ { 60, 0.0, 1.0 }, { 62, 1.0, 1.0 }, { 64, 2.0, 1.0 }, { 65, 3.0, 1.0 } });
        const auto held = st::pair (run, oh, st::Mode::asTimed, nullptr);
        check (held.notes[0].starts && ! held.notes[1].starts && held.notes[1].syllable == 0 && held.notes[2].syllable == 0 && held.notes[3].starts
                   && held.notes[3].syllable == 1 && st::chooseMode (oh, run) == st::Mode::asTimed,
               "pairing: As timed holds a syllable over the notes inside it, and is the default when the words are timed");
        const auto heldAdvice = st::getAdvice (run, held);
        check (adviceHas (heldAdvice, st::Rule::melisma, "\"oh\" is held over 3 notes: the vowel slides from note to note.")
                   && adviceHas (heldAdvice, st::Rule::allGood, "Every note has a syllable and every syllable a note: ready to sing.")
                   && heldAdvice.front().rule == st::Rule::allGood,
               "advice: a melisma is mentioned, and all good comes first");

        // One syllable per note reports what is left over on both sides.
        auto la = [] (int n)
        {
            std::vector<std::tuple<const char*, double, bool>> s;
            for (int i = 0; i < n; ++i)
                s.emplace_back ("la", (double) i, true);
            return s;
        };
        auto notesOf = [] (int n) { std::vector<st::SourceNote> v; for (int i = 0; i < n; ++i) v.push_back ({ 60 + i % 5, (double) i, 1.0 }); return v; };

        Lyrics fourteen;
        fourteen.lines.push_back (makeLine (0.0, 14.0, { { "la", 0, true }, { "la", 1, true }, { "la", 2, true }, { "la", 3, true }, { "la", 4, true }, { "la", 5, true },
                                                         { "la", 6, true }, { "la", 7, true }, { "la", 8, true }, { "la", 9, true }, { "la", 10, true }, { "la", 11, true },
                                                         { "to", 12, false }, { "night", 13, true } }));
        const auto twelve = melodyOf (notesOf (12));
        const auto moreSyllables = st::pair (twelve, fourteen, st::Mode::onePerNote, nullptr);
        check (moreSyllables.leftOverSyllables == 2 && moreSyllables.leftOverNotes == 0
                   && adviceHas (st::getAdvice (twelve, moreSyllables), st::Rule::countsDiffer,
                                 "12 notes and 14 syllables: the last 2 syllables (\"to-night\") aren't sung."),
               "pairing: One syllable per note reports the syllables left over");
        Lyrics ten;
        ten.lines.push_back (makeLine (0.0, 10.0, la (10)));
        const auto moreNotes = st::pair (twelve, ten, st::Mode::onePerNote, nullptr);
        check (moreNotes.leftOverNotes == 2 && moreNotes.leftOverSyllables == 0
                   && adviceHas (st::getAdvice (twelve, moreNotes), st::Rule::countsDiffer,
                                 "12 notes and 10 syllables: the last 2 notes aren't sung. To hold a syllable over several notes, time the words and use As timed."),
               "pairing: One syllable per note reports the notes left over");

        // Not timed to the notes: the default switches to One syllable per note.
        Lyrics offbeat;
        offbeat.lines.push_back (makeLine (0.0, 4.0, { { "la", 0.5, true }, { "la", 1.5, true }, { "la", 2.5, true }, { "la", 3.5, true } }));
        check (st::chooseMode (offbeat, melodyOf (notesOf (4))) == st::Mode::onePerNote, "pairing: words that start between the notes switch the default to One syllable per note");

        // The advice rules, each on a made-up song.
        Lyrics none;
        const auto high = melodyOf ({ { 60, 16.0, 16.0 }, { 60, 32.0, 16.0 } });
        const auto noWords = st::getAdvice (high, st::pair (high, none, st::Mode::asTimed, nullptr));
        check (noWords.size() == 1 && noWords[0].blocksSinging
                   && noWords[0].text == "There are no words on the Lyrics row over bars 5" + juce::String::fromUTF8 ("\xe2\x80\x93") + "12. Type them with Edit... in the Lyrics header first.",
               "advice 1: no words in range says so and blocks singing (" + noWords[0].text + ")");

        Lyrics oneSyllable;
        oneSyllable.lines.push_back (makeLine (0.0, 1.0, { { "la", 0.0, true } }));
        const auto without = melodyOf ({ { 60, 0.0, 1.0 }, { 60, 16.0, 1.0 }, { 60, 17.0, 1.0 }, { 60, 20.0, 1.0 }, { 60, 21.0, 1.0 } });
        check (adviceHas (st::getAdvice (without, st::pair (without, oneSyllable, st::Mode::asTimed, nullptr)), st::Rule::notesWithoutWords,
                          "4 notes in bars 5" + juce::String::fromUTF8 ("\xe2\x80\x93") + "6 have no words, so they're left out. Add words there, or try One syllable per note."),
               "advice 2: notes without words are counted with their bars");

        Lyrics between;
        between.lines.push_back (makeLine (32.0, 36.0, { { "to", 32.0, false }, { "ge", 33.5, false }, { "ther", 35.0, true } }));
        const auto spread = melodyOf ({ { 60, 32.0, 1.0 }, { 60, 35.0, 1.0 } });
        const auto betweenPairing = st::pair (spread, between, st::Mode::asTimed, nullptr);
        check (betweenPairing.unpairedSyllables.size() == 1
                   && adviceHas (st::getAdvice (spread, betweenPairing), st::Rule::syllablesWithoutNote,
                                 "\"ge\" in bar 9 starts between notes, so it isn't sung. Drag it onto a note on the Lyrics row, or try One syllable per note."),
               "advice 3: a syllable between notes is named with its bar");

        Lyrics yeah;
        yeah.lines.push_back (makeLine (0.0, 2.0, { { "yeah", 0.0, false }, { "eh", 1.0, true } }));
        const auto two = melodyOf ({ { 60, 0.0, 1.0 }, { 62, 1.0, 1.0 } });
        check (adviceHas (st::getAdvice (two, st::pair (two, yeah, st::Mode::asTimed, nullptr)), st::Rule::guessedWords,
                          "\"yeah-eh\" isn't in the dictionary, so its sounds were guessed from the spelling. Listen to it, or spell it the way it sounds."),
               "advice 5: a word the dictionary doesn't have is named");

        const auto three = melodyOf ({ { 60, 0.0, 2.0 }, { 64, 1.0, 2.0 }, { 60, 6.0, 2.0 }, { 64, 7.0, 2.0 }, { 60, 12.0, 2.0 }, { 64, 13.0, 2.0 } });
        Lyrics overlapLyrics;
        overlapLyrics.lines.push_back (makeLine (0.0, 16.0, { { "la", 0.0, true }, { "la", 1.0, true }, { "la", 6.0, true }, { "la", 7.0, true }, { "la", 12.0, true }, { "la", 13.0, true } }));
        check (three.overlaps == 3 && adviceHas (st::getAdvice (three, st::pair (three, overlapLyrics, st::Mode::asTimed, nullptr)), st::Rule::overlap,
                                                 "Notes overlap in 3 places. A voice sings one note at a time, so the top one is sung."),
               "advice 6: overlapping notes are counted");

        const auto extremes = melodyOf ({ { 85, 0.0, 1.0 }, { 86, 1.0, 1.0 }, { 60, 2.0, 1.0 } });
        Lyrics extremeLyrics;
        extremeLyrics.lines.push_back (makeLine (0.0, 3.0, { { "la", 0.0, true }, { "la", 1.0, true }, { "la", 2.0, true } }));
        check (adviceHas (st::getAdvice (extremes, st::pair (extremes, extremeLyrics, st::Mode::asTimed, nullptr)), st::Rule::outOfRange,
                          "2 notes go above C6, higher than nearly every singer. Try them an octave lower."),
               "advice 7: notes above C6 are counted");

        for (auto [pitch, expected] : std::initializer_list<std::pair<int, bool>> { { 39, true }, { 40, false }, { 84, false }, { 85, true } })
        {
            const auto m = melodyOf ({ { pitch, 0.0, 1.0 } });
            Lyrics l;
            l.lines.push_back (makeLine (0.0, 1.0, { { "la", 0.0, true } }));
            check (adviceHas (st::getAdvice (m, st::pair (m, l, st::Mode::asTimed, nullptr)), st::Rule::outOfRange) == expected,
                   "range warning for MIDI " + juce::String (pitch) + ": " + (expected ? "yes" : "no"));
        }

        Lyrics strengths;
        strengths.lines.push_back (makeLine (8.0, 10.0, { { "strengths", 8.0, true } }));
        const auto quick = melodyOf ({ { 60, 8.0, 0.26 } });
        check (adviceHas (st::getAdvice (quick, st::pair (quick, strengths, st::Mode::asTimed, nullptr)), st::Rule::crowded,
                          "\"strengths\" in bar 3 has many consonants on a 0.13 s note and may sound rushed. Give it a longer note or a shorter word."),
               "advice 8: a word with many consonants on a very short note");

        Lyrics twelveLyrics;
        twelveLyrics.lines.push_back (makeLine (0.0, 12.0, la (12)));
        const auto plain = st::getAdvice (twelve, st::pair (twelve, twelveLyrics, st::Mode::asTimed, nullptr));
        check (plain.size() == 1 && plain[0].rule == st::Rule::allGood && ! plain[0].blocksSinging,
               "advice 10: a song with every note and syllable matched is ready to sing");
    }

    // The sung score: "feel the night" at 120 BPM.
    {
        Lyrics song;
        song.lines.push_back (makeLine (0.0, 4.0, { { "feel", 0.0, true }, { "the", 1.0, true }, { "night", 2.0, true } }));
        const auto melody = melodyOf ({ { 62, 0.0, 1.0 }, { 60, 1.0, 1.0 }, { 64, 2.0, 2.0 } });
        const auto pairing = st::pair (melody, song, st::Mode::asTimed, nullptr);
        const auto score = st::buildScore (melody, pairing, nullptr, {});

        check (score.phrases.size() == 1, "score: one phrase for feel the night");
        const auto& p = score.phrases[0];
        int iy = -1, ah = -1, ay = -1, f = -1, t = -1;

        for (size_t i = 0; i < p.phonemes.size(); ++i)
        {
            const auto& s = p.phonemes[i].symbol;
            iy = s == "iy" ? (int) i : iy;
            ah = s == "ah" ? (int) i : ah;
            ay = s == "ay" ? (int) i : ay;
            f = s == "f" ? (int) i : f;
            t = s == "t" ? (int) i : t;
        }

        check (iy >= 0 && ah >= 0 && ay >= 0 && std::abs (st::getPhonemeStartSeconds (p, (size_t) iy) - 0.0) < frameTol
                   && std::abs (st::getPhonemeStartSeconds (p, (size_t) ah) - 0.5) < frameTol
                   && std::abs (st::getPhonemeStartSeconds (p, (size_t) ay) - 1.0) < frameTol,
               "score: the vowels start on their notes: iy 0.00, ah 0.50, ay 1.00 s");
        check (f >= 0 && t >= 0 && std::abs (st::getPhonemeStartSeconds (p, (size_t) f) + 0.08) < frameTol
                   && std::abs (st::getPhonemeEndSeconds (p, (size_t) t) - 2.0) < frameTol,
               "score: the first sound starts at -0.08 s and the last ends at 2.00 s");

        double vowelSeconds = 0.0, allSeconds = 0.0;
        for (size_t i = 0; i < p.phonemes.size(); ++i)
        {
            if (p.phonemes[i].symbol == "SP")
                continue;

            allSeconds += p.phonemes[i].frames * sing::frameSeconds;
            vowelSeconds += p.phonemes[i].vowel ? p.phonemes[i].frames * sing::frameSeconds : 0.0;
        }
        check (std::abs (vowelSeconds / allSeconds - 0.808) < 0.02, "score: the vowels take about 80.8% of the sounds (" + juce::String (vowelSeconds / allSeconds * 100.0, 1) + "%)");

        // The pitch of each frame, with exact notes and a slide of 0.05 s.
        auto f0At = [&] (double seconds) { return p.f0[(size_t) juce::roundToInt ((seconds - p.startSeconds) / sing::frameSeconds)]; };
        check (p.f0.size() == (size_t) p.getNumFrames() && std::abs (f0At (0.2) - 293.66) < 1.0 && std::abs (f0At (0.7) - 261.63) < 1.0 && std::abs (f0At (1.5) - 329.63) < 1.0,
               "score: every frame has the pitch of its note (D4, C4, E4)");
        int notesFrames = 0;
        for (auto& n : p.notes)
            notesFrames += n.frames;
        check (notesFrames == p.getNumFrames(), "score: the notes tile the whole phrase");

        // A rest of half a second starts a new phrase; a shorter one is a silence inside the phrase.
        Lyrics two;
        two.lines.push_back (makeLine (0.0, 8.0, { { "la", 0.0, true }, { "la", 2.0, true } }));
        auto countSp = [] (const sing::Phrase& phrase) { int n = 0; for (auto& ph : phrase.phonemes) n += ph.symbol == "SP" ? 1 : 0; return n; };
        const auto longRest = melodyOf ({ { 60, 0.0, 1.0 }, { 60, 2.0, 1.0 } });   // a 1-beat (0.5 s) rest
        Lyrics shortLyrics;
        shortLyrics.lines.push_back (makeLine (0.0, 8.0, { { "la", 0.0, true }, { "la", 1.5, true } }));
        const auto shortRest = melodyOf ({ { 60, 0.0, 1.0 }, { 60, 1.5, 1.0 } });   // a 0.5-beat (0.25 s) rest
        const auto longScore = st::buildScore (longRest, st::pair (longRest, two, st::Mode::asTimed, nullptr), nullptr, {});
        const auto shortScore = st::buildScore (shortRest, st::pair (shortRest, shortLyrics, st::Mode::asTimed, nullptr), nullptr, {});
        check (longScore.phrases.size() == 2 && shortScore.phrases.size() == 1 && countSp (shortScore.phrases[0]) == 3,
               "score: a rest of 0.5 s starts a new phrase, one of 0.25 s is an SP in the same phrase");

        // A breath before the second phrase, when breaths are on.
        auto hasAp = [] (const sing::Phrase& phrase) { for (auto& ph : phrase.phonemes) if (ph.symbol == "AP") return true; return false; };
        sing::Voice withAp;
        withAp.phonemeIds = { { "SP", 0 }, { "AP", 1 } };
        const auto breathy = st::buildScore (longRest, st::pair (longRest, two, st::Mode::asTimed, nullptr), &withAp, { true, false });
        const auto breathless = st::buildScore (longRest, st::pair (longRest, two, st::Mode::asTimed, nullptr), &withAp, { false, false });
        check (breathy.phrases.size() == 2 && ! hasAp (breathy.phrases[0]) && hasAp (breathy.phrases[1]) && ! hasAp (breathless.phrases[1]),
               "score: a breath comes before a phrase that follows a rest, and only when Breaths is on");
    }

    // Rendering: the clip length, cancelling, and the melody track left alone.
    {
        auto buildScoreFor = [&] (const TempoMap& tempo, std::vector<st::SourceNote> notes, Lyrics lyrics)
        {
            const auto melody = st::buildMelody (std::move (notes), tempo);
            return std::pair (melody, st::buildScore (melody, st::pair (melody, lyrics, st::Mode::asTimed, nullptr), nullptr, {}));
        };

        Lyrics eight;
        eight.lines.push_back (makeLine (0.0, 8.0, { { "la", 0.0, true } }));
        FakeSinger singer;
        juce::String error;

        for (auto [tempo, expectedSeconds] : { std::pair (&tempo120, 5.0), std::pair (&tempo60, 9.0) })
        {
            auto [melody, score] = buildScoreFor (*tempo, { { 60, 0.0, 8.0 } }, eight);
            const auto rendering = sing::renderPhrases (singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, 20,
                                                        [] (double, const juce::String&) { return true; }, error);
            check (rendering.has_value() && std::abs (rendering->audio.getNumSamples() / (double) sing::sampleRate - expectedSeconds) < 0.02,
                   "render: a melody of 8 beats at " + juce::String (juce::roundToInt (tempo->getBpmAt (0.0))) + " BPM is a clip of " + juce::String (expectedSeconds, 1) + " s");
        }

        auto [melody, score] = buildScoreFor (tempo120, { { 60, 0.0, 2.0 }, { 62, 4.0, 2.0 } }, eight);
        const int callsBefore = singer.calls;
        const auto cancelled = sing::renderPhrases (singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, 20,
                                                    [] (double, const juce::String&) { return false; }, error);
        check (! cancelled.has_value() && singer.calls == callsBefore, "render: a cancelled render returns nothing and sings nothing");

        // The melody track keeps its notes and sound; the vocal is a new audio track right after it.
        Engine engine;
        auto* track = engine.addTrack (Track::Type::midi);
        track->name = "Lead";
        auto* clip = engine.addMidiClip (*track, 0.0, 8.0);
        clip->notes.push_back ({ 60, 0.0, 2.0, 0.9f });
        clip->notes.push_back ({ 62, 4.0, 2.0, 0.9f });
        const auto soundBefore = track->builtInSound.load();
        const auto notesBefore = clip->notes.size();
        auto rendering = sing::renderPhrases (singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, 20,
                                              [] (double, const juce::String&) { return true; }, error);
        const int tracksBefore = (int) engine.getTracks().size();
        auto* vocal = rendering.has_value() ? sing::addVocalTrack (engine, *track, std::move (*rendering), "Hanami", true) : nullptr;
        check (vocal != nullptr && (int) engine.getTracks().size() == tracksBefore + 1 && engine.indexOf (vocal) == engine.indexOf (track) + 1
                   && vocal->type == Track::Type::audio && vocal->name == "Hanami" + juce::String::fromUTF8 (" \xc2\xb7 ") + "Lead"
                   && track->muted.load() && track->type == Track::Type::midi && track->builtInSound.load() == soundBefore && clip->notes.size() == notesBefore
                   && clip->notes[1].pitch == 62,
               "render: the vocal is a new audio track after the melody's, the melody's notes and sound are unchanged, and it is muted on request");
    }

    // Words that overlap in time, and melodies with fewer or more notes than the lyrics have syllables: the words
    // sung are always the lyrics' own, in the order they are heard, and the advice says what is left out.
    {
        // Lines out of order in the list, the second one running over the first (a tap on a line's first word does this).
        Lyrics tangled;
        tangled.lines.push_back (makeLine (4.0, 8.0, { { "three", 4.0, true }, { "four", 5.0, true } }));
        tangled.lines.push_back (makeLine (0.0, 6.0, { { "one", 0.0, true }, { "two", 1.0, true } }));
        const auto m = melodyOf ({ { 60, 0.0, 1.0 }, { 62, 1.0, 1.0 }, { 64, 4.0, 1.0 }, { 65, 5.0, 1.0 } });
        const auto timed = st::pair (m, tangled, st::Mode::asTimed, nullptr);
        const auto each = st::pair (m, tangled, st::Mode::onePerNote, nullptr);
        auto sung = [] (const st::Pairing& p)
        {
            juce::StringArray out;
            for (auto& a : p.notes)
                out.add (a.syllable >= 0 && a.starts ? p.syllables[(size_t) a.syllable].text : juce::String ("-"));
            return out.joinIntoString (" ");
        };
        check (sung (timed) == "one two three four" && sung (each) == "one two three four",
               "sing this: lines out of order in the list are sung in the order they are heard (" + sung (timed) + " / " + sung (each) + ")");
        check (timed.syllables[1].endBeat <= 4.0 + 1.0e-9,
               "sing this: a line that runs over the next one stops its last word where the next line starts");

        // Fewer notes than syllables.
        Lyrics five;
        five.lines.push_back (makeLine (0.0, 5.0, { { "one", 0.0, true }, { "two", 1.0, true }, { "three", 2.0, true }, { "four", 3.0, true }, { "five", 4.0, true } }));
        const auto three = melodyOf ({ { 60, 0.0, 1.0 }, { 62, 2.0, 1.0 }, { 64, 4.0, 1.0 } });
        const auto asTimed = st::pair (three, five, st::Mode::asTimed, nullptr);
        const auto onePer = st::pair (three, five, st::Mode::onePerNote, nullptr);
        check (sung (asTimed) == "one three five" && asTimed.unpairedSyllables.size() == 2
                   && adviceHas (st::getAdvice (three, asTimed), st::Rule::syllablesWithoutNote),
               "sing this: with fewer notes than syllables, As timed sings the syllables that start on a note, leaves the others out and says so (" + sung (asTimed) + ")");
        check (sung (onePer) == "one two three" && onePer.leftOverSyllables == 2
                   && adviceHas (st::getAdvice (three, onePer), st::Rule::countsDiffer,
                                 "3 notes and 5 syllables: the last 2 syllables (\"four five\") aren't sung."),
               "sing this: with fewer notes than syllables, One syllable per note sings the first ones in order, never other words, and says which are left out (" + sung (onePer) + ")");

        // More notes than syllables.
        Lyrics two;
        two.lines.push_back (makeLine (0.0, 2.0, { { "one", 0.0, true }, { "two", 1.0, true } }));
        const auto four = melodyOf ({ { 60, 0.0, 1.0 }, { 62, 1.0, 1.0 }, { 64, 2.0, 1.0 }, { 65, 3.0, 1.0 } });
        const auto extraTimed = st::pair (four, two, st::Mode::asTimed, nullptr);
        const auto extraEach = st::pair (four, two, st::Mode::onePerNote, nullptr);
        check (sung (extraTimed) == "one two - -" && ! extraTimed.notesWithoutWords.empty()
                   && adviceHas (st::getAdvice (four, extraTimed), st::Rule::notesWithoutWords),
               "sing this: with more notes than syllables, the extra notes sing nothing and the advice says so (" + sung (extraTimed) + ")");
        check (sung (extraEach) == "one two - -" && extraEach.leftOverNotes == 2
                   && adviceHas (st::getAdvice (four, extraEach), st::Rule::countsDiffer),
               "sing this: One syllable per note with spare notes leaves them silent and says so (" + sung (extraEach) + ")");

        // Only the lyrics' own words reach the score: every sound comes from a word that was written.
        const auto score = st::buildScore (three, onePer, nullptr, {}, nullptr);
        int sungWords = 0;
        for (auto& ph : score.phrases)
            for (auto& p : ph.phonemes)
                sungWords = juce::jmax (sungWords, p.word + 1);
        check (sungWords <= 3, "sing this: the score has no more words than the notes that start one (" + juce::String (sungWords) + ")");
    }

    // Fast singing: every vowel starts on its note (to within half a frame, 5.8 ms) and keeps room to sound, however
    // short the notes and long the consonant clusters ("strengths next light street ..."). Before the fix the
    // vowels of 16th notes at 140 BPM were squeezed to one frame (11.6 ms) by the consonants around them.
    {
        const double halfFrameMs = sing::frameSeconds * 500.0 + 0.01;
        FakeDurationSinger model09 (0.09), model15 (0.15);
        double endError = 0.0;

        const auto sixteenths = measureFastTune (140.0, 0.25, nullptr, 16, &endError);
        check (sixteenths.count == 16 && sixteenths.maxAbsErrorMs < halfFrameMs && sixteenths.shortestVowelMs > 34.0 && sixteenths.smallestVowelShare > 0.32,
               "fast singing: 16th notes at 140 BPM (107 ms): all 16 vowels start within 5.8 ms of their notes and last at least 3 frames (34.8 ms), a third of the note ("
                   + juce::String (sixteenths.maxAbsErrorMs, 1) + " ms, " + juce::String (sixteenths.shortestVowelMs, 1) + " ms)");
        check (std::abs (endError) < sing::frameSeconds * 1000.0, "fast singing: the last sound still ends with the last note, not later (" + juce::String (endError, 1) + " ms)");

        for (auto* model : { &model09, &model15 })
        {
            const auto r = measureFastTune (140.0, 0.25, model);
            check (r.maxAbsErrorMs < halfFrameMs && r.shortestVowelMs > 34.0 && r.smallestVowelShare > 0.32,
                   "fast singing: with a duration model that says " + juce::String (model->consonantSeconds * 1000.0, 0) + " ms for each consonant, 16th notes keep their vowels on the beat and at 3 frames or more ("
                       + juce::String (r.maxAbsErrorMs, 1) + " ms, " + juce::String (r.shortestVowelMs, 1) + " ms)");
        }

        const auto eighths = measureFastTune (140.0, 0.5, nullptr);
        check (eighths.maxAbsErrorMs < halfFrameMs && eighths.shortestVowelMs > 80.0 && eighths.smallestVowelShare > 0.37,
               "fast singing: 8th notes at 140 BPM (214 ms): vowels on the beat and at least 7 frames (81 ms, " + juce::String (eighths.shortestVowelMs, 1) + ")");

        const auto slower = measureFastTune (120.0, 0.25, &model09);
        check (slower.maxAbsErrorMs < halfFrameMs && slower.shortestVowelMs > 46.0 && slower.smallestVowelShare > 0.37,
               "fast singing: 16th notes at 120 BPM (125 ms): vowels at least 4 frames (46 ms, " + juce::String (slower.shortestVowelMs, 1) + ")");

        // 32nd notes at 140 BPM (53 ms) can't hold "strengths": the vowels may be up to 26 ms off, but only where the
        // cluster is, and none is lost (every syllable still has a vowel of at least one frame).
        const auto thirtySeconds = measureFastTune (140.0, 0.125, nullptr);
        check (thirtySeconds.count == 16 && thirtySeconds.maxAbsErrorMs < 26.0 && thirtySeconds.shortestVowelMs > 11.0,
               "fast singing: 32nd notes at 140 BPM (53 ms) are too short for the clusters; the vowels stay within 26 ms of their notes and keep a frame ("
                   + juce::String (thirtySeconds.maxAbsErrorMs, 1) + " ms)");

        st::Melody exampleMelody;
        const auto example = st::measureTiming (scoreExampleSong (nullptr, nullptr, exampleMelody));
        const auto exampleModel = st::measureTiming (scoreExampleSong (nullptr, &model09, exampleMelody));
        check (example.count == 34 && example.maxAbsErrorMs < halfFrameMs && example.shortestVowelMs > 190.0
                   && exampleModel.count == 34 && exampleModel.maxAbsErrorMs < halfFrameMs && exampleModel.shortestVowelMs > 190.0,
               "fast singing: the example song's 34 syllables all start within 5.8 ms of their notes and last 190 ms or more ("
                   + juce::String (example.maxAbsErrorMs, 1) + " ms, " + juce::String (example.shortestVowelMs, 1) + " ms)");

        // Two quick notes in a row: the second word's cluster is squeezed, its vowel still starts on its note.
        Lyrics strengthsLyrics;
        strengthsLyrics.lines.push_back (makeLine (0.0, 4.0, { { "ah", 0.0, true }, { "strengths", 0.25, true } }));
        const auto tight = st::buildMelody ({ { 60, 0.0, 0.25 }, { 62, 0.25, 0.25 } }, TempoMap (140.0));
        const auto tightScore = st::buildScore (tight, st::pair (tight, strengthsLyrics, st::Mode::asTimed, nullptr), nullptr, {});
        const auto& tp = tightScore.phrases[0];
        int vowels = 0, frames = 0;
        for (auto& ph : tp.phonemes)
            if (ph.vowel && ph.symbol != "SP")
                ++vowels, frames += ph.frames;
        check (vowels == 2 && frames >= 6 && tightScore.syllables.size() == 2
                   && std::abs (tightScore.syllables[1].vowelStartSeconds - tight.notes[1].startSeconds) < sing::frameSeconds * 0.5 + 1.0e-6,
               "fast singing: \"ah strengths\" on two 16th notes: the second vowel starts on its note (" + juce::String (frames) + " vowel frames)");
    }

    // The guide's new topics.
    {
        namespace lg = gen::lyricsguide;
        const auto computer = lg::explain (lg::Topic::computer, {}, {}, 120.0);
        const auto voices = lg::explain (lg::Topic::voices, {}, {}, 120.0);
        const auto example = lg::getExample (lg::Topic::computer, {});
        check (computer.length() > 1500 && computer.contains ("39 sounds: 15 vowels and 24 consonants") && lg::getTopicName (lg::Topic::computer) == "How a computer sings"
                   && example.kind == lg::Kind::sounds && example.lines.size() == 1 && example.lines[0].tokens.size() == 3
                   && example.lines[0].tokens[2].sounds.size() == 3 && std::abs (example.lines[0].tokens[0].sounds[0].start + 0.16) < 1.0e-9,
               "lyrics guide: How a computer sings has its text and the feel the night example with sounds under each note");
        check (voices.contains ("Get a voice...") && voices.contains ("Add a voice...") && voices.contains ("Voices") && voices.contains ("dsconfig.yaml is missing")
                   && voices.contains ("credit") && voices.contains ("CC BY-NC-SA 4.0") && voices.contains ("Terms...")
                   && ! lg::getExample (lg::Topic::voices, {}).lines.empty(),
               "lyrics guide: Getting a voice explains where to get one, how to add it, the terms and what to do when it fails");
        const auto entries = gen::guides::getEntries();
        const auto found = gen::guides::search (entries, "voicebank"), phoneme = gen::guides::search (entries, "phoneme");
        check (gen::guides::getEntry (gen::guides::Guide::lyrics).topics.size() == (int) std::size (lg::allTopics)
                   && ! found.empty() && entries[(size_t) found[0].entry].guide == gen::guides::Guide::lyrics && ! phoneme.empty()
                   && entries[(size_t) phoneme[0].entry].guide == gen::guides::Guide::lyrics,
               "theory guides: voicebank and phoneme find the lyrics guide, which lists the new topics");
    }

    // Match the notes to the words.
    {
        std::cout << "  match the notes to the words" << std::endl;
        auto starts = [] (const st::MatchResult& r) { std::vector<double> v; for (auto& n : r.notes) v.push_back (n.startBeat); return v; };
        auto lengths = [] (const st::MatchResult& r) { std::vector<double> v; for (auto& n : r.notes) v.push_back (n.lengthBeats); return v; };
        auto pitches = [] (const st::MatchResult& r) { std::vector<int> v; for (auto& n : r.notes) v.push_back (n.pitch); return v; };
        auto contiguous = [] (std::initializer_list<std::pair<int, double>> notes)
        {
            std::vector<st::SourceNote> v;
            double beat = 0.0;

            for (auto& [pitch, length] : notes)
            {
                v.push_back ({ pitch, beat, length });
                beat += length;
            }

            return v;
        };

        // Too many notes: the shortest inner note is folded into the one before it, and the longer note's pitch wins.
        const auto many = melodyOf (contiguous ({ { 60, 2.0 }, { 62, 0.5 }, { 64, 1.0 }, { 65, 0.25 }, { 67, 1.0 }, { 69, 2.0 } }));
        const auto merged = st::matchNotesToSyllables (many, 4);
        check (merged.exact && merged.merged == 2 && merged.split == 0 && merged.notesBefore == 6 && merged.getNotesAfter() == 4
                   && starts (merged) == std::vector<double> { 0.0, 2.5, 3.75, 4.75 }
                   && lengths (merged) == std::vector<double> { 2.5, 1.25, 1.0, 2.0 }
                   && pitches (merged) == std::vector<int> { 60, 64, 67, 69 },
               "match: 6 notes to 4 merges the 0.25 and 0.5 beat notes into the notes before, keeping the longer pitch and the span");
        check (st::describeMatch (merged) == "Merged 2 notes into the one before: 4 notes now, 4 syllables."
                   && st::describeMatch (merged, false) == "6 notes, 4 syllables. Matching would merge 2 notes into the one before.",
               "match: the description says how many notes merged, before and after");

        // Too few notes: the longest is cut in half at the same pitch, the earliest first on a tie.
        const auto few = melodyOf (contiguous ({ { 60, 4.0 }, { 64, 1.0 } }));
        const auto splitUp = st::matchNotesToSyllables (few, 4);
        check (splitUp.exact && splitUp.split == 2 && splitUp.merged == 0 && starts (splitUp) == std::vector<double> { 0.0, 1.0, 2.0, 4.0 }
                   && lengths (splitUp) == std::vector<double> { 1.0, 1.0, 2.0, 1.0 } && pitches (splitUp) == std::vector<int> { 60, 60, 60, 64 },
               "match: 2 notes to 4 splits the 4 beat note twice, into 1, 1 and 2 beats at the same pitch");

        // Notes too short to split stop the match and say how many syllables have no note.
        const auto tiny = melodyOf (contiguous ({ { 60, 0.5 } }));
        const auto tooShort = st::matchNotesToSyllables (tiny, 5);
        check (! tooShort.exact && tooShort.getNotesAfter() == 4 && tooShort.remaining == -1 && tooShort.split == 3
                   && lengths (tooShort) == std::vector<double> { 0.125, 0.125, 0.125, 0.125 }
                   && st::matchNotesToSyllables (tiny, 4).exact
                   && st::describeMatch (tooShort).endsWith ("1 syllable still have no note: the notes left are too short to split."),
               "match: a half beat note splits into four 32nd notes and no further, and the shortfall is reported");

        // The first and last note of a phrase stay (a rest of half a second or more starts a phrase).
        const auto phrases = melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 1.0 }, { 62, 1.0, 0.25 }, { 64, 1.25, 1.0 },
                                                                      { 65, 4.25, 1.0 }, { 67, 5.25, 0.25 }, { 69, 5.5, 1.0 } });
        const auto kept = st::matchNotesToSyllables (phrases, 3);
        check (! kept.exact && kept.getNotesAfter() == 4 && kept.remaining == 1 && kept.merged == 2
                   && starts (kept) == std::vector<double> { 0.0, 1.25, 4.25, 5.5 } && lengths (kept) == std::vector<double> { 1.25, 1.0, 1.25, 1.0 },
               "match: phrase starts and ends are kept, so 6 notes in two phrases stop at 4 and 1 note too many is reported");
        check (st::matchNotesToSyllables (phrases, 6).notes.size() == 6 && st::matchNotesToSyllables (phrases, 6).merged == 0
                   && st::matchNotesToSyllables (phrases, 6).split == 0,
               "match: equal counts change nothing");

        // Match to the words' timing: one note on each syllable, at the syllable's beat.
        {
            auto syllablesAt = [] (std::initializer_list<double> beats)
            {
                std::vector<st::Syllable> v;

                for (auto b : beats)
                {
                    st::Syllable sy;
                    sy.text = "la";
                    sy.beat = b;
                    sy.endBeat = b + 1.0;
                    v.push_back (sy);
                }

                return v;
            };
            auto near = [] (const std::vector<double>& a, const std::vector<double>& b)
            {
                if (a.size() != b.size())
                    return false;

                for (size_t i = 0; i < a.size(); ++i)
                    if (std::abs (a[i] - b[i]) > 1.0e-6)
                        return false;

                return true;
            };
            auto velocities = [] (const st::MatchResult& r) { std::vector<float> v; for (auto& n : r.notes) v.push_back (n.velocity); return v; };

            // Notes 60 [0,2), 64 [2,4) at velocity 0.5, 67 [6,8) with a rest from 4 to 6.
            const auto melody = melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 2.0, 0.9f }, { 64, 2.0, 2.0, 0.5f }, { 67, 6.0, 2.0, 0.3f } });
            const auto timed = st::matchNotesToSyllableTimes (melody, syllablesAt ({ 0.0, 1.0, 2.5, 3.9, 4.5, 6.0 }), st::Method::syllableTimesCapped);
            check (timed.method == st::Method::syllableTimesCapped && timed.exact && timed.getNotesAfter() == 6 && timed.notesBefore == 3
                       && near (starts (timed), { 0.0, 1.0, 2.5, 3.9, 4.5, 6.0 })
                       && pitches (timed) == std::vector<int> { 60, 60, 64, 64, 64, 67 },
                   "match by timing, melody lengths: 3 notes over 6 syllables become 6 notes starting at the syllable beats, with the pitch of the note sounding there (the earlier note's in a rest)");
            check (near (lengths (timed), { 1.0, 1.0, 1.4, 0.125, 1.5, 2.0 }) && timed.tooShort == 1,
                   "match by timing, melody lengths: a note lasts to the next syllable but not past its source note's end or the end of the rest, and 0.1 beat is raised to 0.125");
            check (velocities (timed) == std::vector<float> { 0.9f, 0.9f, 0.5f, 0.5f, 0.5f, 0.3f } && timed.moved == 4,
                   "match by timing, melody lengths: the source note's velocity is kept, and 4 of the 6 starts moved by more than a 16th");

            // A syllable before the first note takes the first note's pitch and stops where the note starts.
            const auto late = melodyOf (std::vector<st::SourceNote> { { 62, 2.0, 2.0 }, { 64, 4.0, 2.0 } });
            const auto early = st::matchNotesToSyllableTimes (late, syllablesAt ({ 1.0, 2.0, 4.0 }), st::Method::syllableTimesCapped);
            check (near (starts (early), { 1.0, 2.0, 4.0 }) && near (lengths (early), { 1.0, 2.0, 2.0 }) && pitches (early) == std::vector<int> { 62, 62, 64 },
                   "match by timing, melody lengths: a syllable before the melody takes the first note's pitch and stops where that note starts");

            // Two syllables on one long note become two notes at the same pitch and velocity.
            const auto longNote = melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 4.0, 0.6f } });
            const auto two = st::matchNotesToSyllableTimes (longNote, syllablesAt ({ 0.0, 2.0 }), st::Method::syllableTimesCapped);
            check (starts (two) == std::vector<double> { 0.0, 2.0 } && lengths (two) == std::vector<double> { 2.0, 2.0 } && pitches (two) == std::vector<int> { 60, 60 }
                       && velocities (two) == std::vector<float> { 0.6f, 0.6f } && two.moved == 1,
                   "match by timing, melody lengths: two syllables on one 4 beat note become two 2 beat notes at the same pitch");

            // Equal counts are returned as they are; no syllables leaves the notes alone and reports it.
            const auto same = st::matchNotesToSyllableTimes (melody, syllablesAt ({ 0.5, 3.0, 7.0 }));
            check (same.exact && starts (same) == std::vector<double> { 0.0, 2.0, 6.0 } && lengths (same) == std::vector<double> { 2.0, 2.0, 2.0 } && same.moved == 0
                       && ! st::matchNotesToSyllableTimes (melody, {}).exact,
                   "match by timing, melody lengths: equal counts change nothing");

            // Legato (the default): notes run on to the next syllable, minus 0.125 beat.
            const auto legato = st::matchNotesToSyllableTimes (melody, syllablesAt ({ 0.0, 1.0, 2.5, 3.9, 4.5, 6.0 }));
            check (legato.method == st::Method::syllableTimes && near (starts (legato), { 0.0, 1.0, 2.5, 3.9, 4.5, 6.0 })
                       && near (lengths (legato), { 0.875, 1.375, 1.275, 0.475, 1.0, 2.0 }) && legato.tooShort == 0
                       && pitches (legato) == std::vector<int> { 60, 60, 64, 64, 64, 67 },
                   "match by timing: a note runs on to 0.125 beat before the next syllable, over the gap between notes; the last syllable of a phrase takes the longer of its note's rest and its own length, up to 2 beats, and stops before the next syllable");

            // A gap of half a beat between words is filled; the last syllable of the melody takes its own length (1 beat) as it is longer than its note's 0.5.
            const auto gapped = melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 1.0 }, { 62, 1.5, 1.0 }, { 64, 3.0, 1.0 } });
            const auto filled = st::matchNotesToSyllableTimes (gapped, syllablesAt ({ 0.0, 1.5, 3.0, 3.5 }));
            check (near (lengths (filled), { 1.375, 1.375, 0.375, 1.0 }) && pitches (filled) == std::vector<int> { 60, 62, 64, 64 },
                   "match by timing: a gap between words (0.5 beat, under a phrase rest) is filled by the note before");

            // A rest of 0.5 s or more ends a phrase: the last syllable stops at the next syllable's gap or the next phrase, never runs through the rest.
            auto phraseSyllables = syllablesAt ({ 0.0, 3.0, 4.5, 7.0 });
            phraseSyllables[2].endBeat = 9.0;
            const auto broken = melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 2.0 }, { 62, 2.0, 2.0 }, { 64, 6.0, 2.0 } });
            const auto phrased = st::matchNotesToSyllableTimes (broken, phraseSyllables);
            check (near (starts (phrased), { 0.0, 3.0, 4.5, 7.0 }) && near (lengths (phrased), { 2.875, 1.0, 1.5, 1.0 }) && pitches (phrased) == std::vector<int> { 60, 62, 62, 64 },
                   "match by timing: the last syllable of a phrase does not run past the start of the next phrase (a 9 beat word from beat 4.5 stops at 6)");

            auto longWord = syllablesAt ({ 0.0, 5.0 });
            longWord[0].endBeat = 4.0;
            const auto capTwo = st::matchNotesToSyllableTimes (melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 1.0 } }), longWord);
            check (near (lengths (capTwo), { 2.0, 1.0 }),
                   "match by timing: the last syllable of a phrase lasts at most 2 beats");

            const auto tight = st::matchNotesToSyllableTimes (melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 4.0 } }), syllablesAt ({ 0.0, 0.125, 2.0 }));
            check (near (lengths (tight), { 0.125, 1.75, 2.0 }) && tight.tooShort == 1,
                   "match by timing: a syllable 0.125 beat before the next gets the 0.125 minimum, and it is counted");

            check (st::describeMatch (timed, false) == "3 notes, 6 syllables. Matching would move 4 note starts by more than a 16th. 1 note is held at a 32nd note, the shortest allowed, though the next syllable or the end of its note comes sooner."
                       && st::describeMatch (two).startsWith ("Put a note on each syllable: 2 notes now, 2 syllables. 1 note start moved by more than a 16th."),
                   "match by timing, melody lengths: the description says how many note starts move");

            const auto sourceVelocity = melodyOf (std::vector<st::SourceNote> { { 60, 0.0, 1.0, 0.25f } });
            check (sourceVelocity.notes.size() == 1 && juce::exactlyEqual (sourceVelocity.notes[0].velocity, 0.25f)
                       && juce::exactlyEqual (st::matchNotesToSyllables (melodyOf (contiguous ({ { 60, 4.0 }, { 64, 1.0 } })), 3).notes[0].velocity, 0.8f),
                   "match: the melody keeps each note's velocity, and merging and splitting carry it");
        }

        // The count comes from the lyrics (One syllable per note), and the adjusted melody pairs one syllable on each note.
        Lyrics five;
        {
            LyricLine line;
            line.startBeat = 0.0;
            line.endBeat = 5.0;

            for (int i = 0; i < 5; ++i)
            {
                LyricSyllable s;
                s.text = "la";
                s.beat = (double) i;
                line.syllables.push_back (s);
            }

            five.lines.push_back (line);
        }
        std::vector<st::SourceNote> seven;
        for (int i = 0; i < 7; ++i)
            seven.push_back ({ 60 + i, (double) i, 1.0 });
        const auto sevenMelody = melodyOf (seven);
        const int target = st::countSyllablesToMatch (five, sevenMelody);
        const auto fitted = st::matchNotesToSyllables (sevenMelody, target);
        const auto refit = st::pair (melodyOf (fitted.notes), five, st::Mode::onePerNote, nullptr);
        check (target == 5 && fitted.exact && fitted.merged == 2 && starts (fitted) == std::vector<double> { 0.0, 3.0, 4.0, 5.0, 6.0 }
                   && refit.leftOverNotes == 0 && refit.leftOverSyllables == 0,
               "match: 7 notes over 5 syllables become 5 notes, and One syllable per note then leaves nothing over");
        check (st::countSyllablesToMatch (Lyrics(), sevenMelody) == 0, "match: with no words there is nothing to match to");
    }
}

/** Part of --voice: sings the first lines of the example song's lead melody with its lyrics, prints the pairing and
    what Whisper hears, to compare with the words. */
static void runVoiceSongTest (daw::sing::DiffSinger& singer, const daw::sing::Voice& voice, const juce::File& whisperModel)
{
    using namespace daw;
    namespace st = gen::singthis;
    Engine engine;
    engine.deviceManager.removeAudioCallback (&engine);
    auto* lead = examples::buildPopSong (engine);
    std::vector<st::SourceNote> source;

    for (auto& c : lead->clips)
        if (c->kind == Clip::Kind::midi)
            for (auto& n : static_cast<MidiClip&> (*c).notes)
                if (c->startBeat + n.start < 52.0)
                    source.push_back ({ n.pitch, c->startBeat + n.start, n.length });

    const auto melody = st::buildMelody (source, engine.getTempoMap());
    const auto mode = st::chooseMode (engine.getLyrics(), melody);
    const auto pairing = st::pair (melody, engine.getLyrics(), mode, &voice);
    std::cout << "  example song: " << melody.notes.size() << " notes, mode " << (mode == st::Mode::asTimed ? "as timed" : "one per note")
              << ", " << pairing.syllables.size() << " syllables, " << pairing.words.size() << " words\n";

    for (auto& w : pairing.words)
    {
        std::cout << "    " << w.text << " [" << (w.sounds.source == gen::pronunciation::Source::voiceDictionary ? "voice dict" : w.sounds.source == gen::pronunciation::Source::cmudict ? "cmudict" : "GUESSED") << "]:";

        for (auto& snd : w.sounds.sounds)
            std::cout << " " << snd.symbol;

        std::cout << "\n";
    }

    for (auto& a : st::getAdvice (melody, pairing))
        std::cout << "    advice: " << a.text << "\n";

    juce::String error;
    auto score = st::buildScore (melody, pairing, &voice, {}, &singer);
    {
        const auto r = st::measureTiming (score);
        std::cout << "  example song timing: " << r.count << " vowels, mean |error| " << juce::String (r.meanAbsErrorMs, 1) << " ms, max "
                  << juce::String (r.maxAbsErrorMs, 1) << " ms, shortest vowel " << juce::String (r.shortestVowelMs, 1) << " ms\n";
    }
    const auto rendering = sing::renderPhrases (singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, 10,
                                                [] (double, const juce::String&) { return true; }, error);

    if (! rendering.has_value() || whisperModel == juce::File())
        return;

    namespace lf = analysis::lyricsfinder;
    lf::Transcriber transcriber;
    juce::String werror;

    if (transcriber.load (whisperModel, lf::ModelSize::medium, werror))
    {
        auto mono = lf::prepareInput (rendering->audio, (double) sing::sampleRate);

        if (auto heard = transcriber.transcribe (mono, [] (double) { return true; }, werror))
        {
            juce::StringArray words;
            for (auto& w : *heard)
                words.add (w.text);

            std::cout << "  Whisper hears the song as: " << words.joinIntoString (" | ") << std::endl;
        }
    }
}

/** Part of --voice: sings the made-up fast tune (and the example song's quick bars) with the real voice, prints how far
    each vowel is from its note in the plan and, with Whisper, when each word is heard against when it was planned. */
static void runVoiceFastTest (daw::sing::DiffSinger& singer, const daw::sing::Voice& voice, const juce::File& whisperModel)
{
    using namespace daw;
    namespace st = gen::singthis;
    namespace lf = analysis::lyricsfinder;

    static const char* const tune[] = { "strengths", "next", "light", "street", "and", "glimpsed", "the", "night", "twelfths", "the", "stand", "ring", "past", "pink", "bright", "dreams" };

    for (auto [bpm, beats] : { std::pair (140.0, 0.5), std::pair (140.0, 0.25) })
    {
        const TempoMap tempo (bpm);
        std::vector<st::SourceNote> notes;
        std::vector<std::tuple<const char*, double, bool>> syllables;

        for (int i = 0; i < 16; ++i)
        {
            notes.push_back ({ 60 + (i % 5), i * beats, beats });
            syllables.push_back ({ tune[i], i * beats, true });
        }

        Lyrics lyrics;
        lyrics.lines.push_back (makeLine (0.0, 16 * beats, syllables));
        const auto melody = st::buildMelody (notes, tempo);
        const auto pairing = st::pair (melody, lyrics, st::Mode::asTimed, &voice);
        auto score = st::buildScore (melody, pairing, &voice, {}, &singer);
        const auto r = st::measureTiming (score);
        std::cout << "  fast tune " << bpm << " bpm, notes of " << beats << " beats: " << r.count << " vowels, mean |error| " << juce::String (r.meanAbsErrorMs, 1)
                  << " ms, max " << juce::String (r.maxAbsErrorMs, 1) << " ms, shortest vowel " << juce::String (r.shortestVowelMs, 1) << " ms ("
                  << juce::String (r.smallestVowelShare * 100.0, 0) << "% of its note)\n";

        for (auto& ph : score.phrases)
        {
            std::cout << "    frames:";
            for (auto& p : ph.phonemes)
                std::cout << " " << p.symbol << p.frames;
            std::cout << "\n";
        }

        juce::String error;
        const auto rendering = sing::renderPhrases (singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, 20,
                                                    [] (double, const juce::String&) { return true; }, error);

        if (! rendering.has_value() || whisperModel == juce::File())
            continue;

        lf::Transcriber transcriber;
        juce::String werror;

        if (! transcriber.load (whisperModel, lf::ModelSize::medium, werror))
            continue;

        auto mono = lf::prepareInput (rendering->audio, (double) sing::sampleRate);
        auto heard = transcriber.transcribe (mono, [] (double) { return true; }, werror);

        if (! heard.has_value())
            continue;

        // The planned start of each word: its first sound, in the song's time.
        std::vector<double> planned;
        for (size_t w = 0; w < pairing.words.size(); ++w)
            for (auto& ph : score.phrases)
            {
                bool found = false;
                for (size_t i = 0; i < ph.phonemes.size() && ! found; ++i)
                    if (ph.phonemes[i].word == (int) w)
                    {
                        planned.push_back (st::getPhonemeStartSeconds (ph, i));
                        found = true;
                    }
                if (found)
                    break;
            }

        std::cout << "    whisper heard " << heard->size() << " words for " << planned.size() << " planned:";
        double sum = 0.0, sumAbs = 0.0;
        const bool same = heard->size() == planned.size();

        for (size_t i = 0; i < heard->size(); ++i)
        {
            const double at = rendering->originSeconds + (*heard)[i].start;
            std::cout << " " << (*heard)[i].text << "@" << juce::String (at, 2);

            if (same)
            {
                sum += at - planned[i];
                sumAbs += std::abs (at - planned[i]);
                std::cout << "(" << juce::String ((at - planned[i]) * 1000.0, 0) << "ms)";
            }
        }

        if (same)
            std::cout << "\n    mean heard-minus-planned " << juce::String (sum / (double) planned.size() * 1000.0, 0) << " ms, mean |.| "
                      << juce::String (sumAbs / (double) planned.size() * 1000.0, 0) << " ms";

        std::cout << std::endl;
    }
}

/** An optional real run: sings "feel the night" with the voice in a folder and says how long it took. */
static void runVoiceTest (const juce::File& folder, const juce::File& whisperModel)
{
    using namespace daw;
    namespace st = gen::singthis;
    std::cout << "Sing this with the voice in " << folder.getFullPathName() << std::endl;

    auto loaded = sing::loadVoice (folder, sing::getVoicesFolder().getChildFile ("Vocoders"));

    if (! loaded.voice.has_value())
    {
        std::cout << "  the voice doesn't load: " << loaded.error << std::endl;
        return;
    }

    juce::String error;
    const auto loadStart = juce::Time::getMillisecondCounterHiRes();
    auto singer = sing::DiffSinger::create (*loaded.voice, error);

    if (singer == nullptr)
    {
        std::cout << "  the models don't load: " << error << std::endl;
        return;
    }

    std::cout << "  loaded in " << (juce::Time::getMillisecondCounterHiRes() - loadStart) / 1000.0 << " s" << std::endl;
    std::cout << singer->describeModels();

    for (auto& w : singer->getWarnings())
        std::cout << "  warning: " << w << std::endl;

    runVoiceSongTest (*singer, *loaded.voice, whisperModel);
    runVoiceFastTest (*singer, *loaded.voice, whisperModel);

    const TempoMap tempo (120.0);
    Lyrics song;
    song.lines.push_back (makeLine (0.0, 4.0, { { "feel", 0.0, true }, { "the", 1.0, true }, { "night", 2.0, true } }));
    const auto melody = st::buildMelody ({ { 62, 0.0, 1.0 }, { 60, 1.0, 1.0 }, { 64, 2.0, 2.0 } }, tempo);
    const auto pairing = st::pair (melody, song, st::Mode::asTimed, &*loaded.voice);
    const double sungSeconds = melody.notes.back().endSeconds - melody.notes.front().startSeconds;

    for (auto [name, steps] : { std::pair ("Draft", 10), std::pair ("Good", 20), std::pair ("Best", 50) })
    {
        const auto start = juce::Time::getMillisecondCounterHiRes();
        auto score = st::buildScore (melody, pairing, &*loaded.voice, {}, singer.get());

        if (steps == 10)
        {
            std::cout << "  language id for English: " << loaded.voice->englishLanguageId << std::endl;

            for (auto& ph : score.phrases)
            {
                std::cout << "  phrase tokens:";

                for (auto& p : ph.phonemes)
                    std::cout << " " << p.symbol << "(" << loaded.voice->phonemeIds.at (p.symbol) << "," << p.frames << ")";

                std::cout << std::endl;
            }
        }

        const auto rendering = sing::renderPhrases (*singer, score.phrases, score.spanStartSeconds, score.spanEndSeconds, steps,
                                                    [] (double, const juce::String&) { return true; }, error);
        const double took = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;

        if (! rendering.has_value())
        {
            std::cout << "  " << name << " (" << steps << " steps) failed: " << error << std::endl;
            continue;
        }

        const auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("dawit-sing-" + juce::String (name) + ".wav");
        wav.deleteFile();
        juce::WavAudioFormat format;

        std::unique_ptr<juce::OutputStream> stream (wav.createOutputStream());

        if (stream != nullptr)
        {
            auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate ((double) sing::sampleRate)
                                                                                           .withNumChannels (1).withBitsPerSample (16));

            if (writer != nullptr)
                writer->writeFromAudioSampleBuffer (rendering->audio, 0, rendering->audio.getNumSamples());
        }

        if (steps == 20 && whisperModel != juce::File())
        {
            namespace lf = analysis::lyricsfinder;
            lf::Transcriber transcriber;
            juce::String werror;

            if (transcriber.load (whisperModel, lf::ModelSize::medium, werror))
            {
                auto mono = lf::prepareInput (rendering->audio, (double) sing::sampleRate);

                if (auto heard = transcriber.transcribe (mono, [] (double) { return true; }, werror))
                {
                    juce::StringArray words;
                    for (auto& w : *heard)
                        words.add (w.text);

                    std::cout << "  Whisper hears the rendering as: " << words.joinIntoString (" | ") << std::endl;
                }
            }
        }

        std::cout << "  " << name << " (" << steps << " steps): " << took << " s for " << sungSeconds << " s sung (" << took / sungSeconds
                  << " x), written to " << wav.getFullPathName() << std::endl;
    }
}

#include "TuneTests.h"

static void runStereoTestSoundTests()
{
    constexpr int sampleRate = 44100;
    for (const auto sound : { daw::fx::Clap::Sound::clap, daw::fx::Clap::Sound::note, daw::fx::Clap::Sound::hitsInHiss, daw::fx::Clap::Sound::wideBass })
    {
        daw::fx::Clap clap;
        clap.prepare (sampleRate);
        clap.request (sound);
        juce::AudioBuffer<float> buffer (2, sampleRate * 3 / 2);
        buffer.clear();
        clap.addTo (buffer, 2);
        double difference = 0.0;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            difference += std::abs (buffer.getSample (0, i) - buffer.getSample (1, i));
        check (sound == daw::fx::Clap::Sound::wideBass ? difference > 1.0 : difference < 1.0e-12,
               "Test sounds are mono except Wide bass");
        if (sound == daw::fx::Clap::Sound::wideBass)
        {
            double ls = 0.0, lc = 0.0, rs = 0.0, rc = 0.0;
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const double phase = juce::MathConstants<double>::twoPi * 55.0 * i / sampleRate;
                ls += buffer.getSample (0, i) * std::sin (phase);
                lc += buffer.getSample (0, i) * std::cos (phase);
                rs += buffer.getSample (1, i) * std::sin (phase);
                rc += buffer.getSample (1, i) * std::cos (phase);
            }
            const double r = (ls * rs + lc * rc) / std::sqrt ((ls * ls + lc * lc) * (rs * rs + rc * rc));
            check (std::abs (r) < 0.02, "Wide bass low-frequency channels are uncorrelated");
        }
    }
}

static void runBuiltInStereoTests()
{
    using namespace daw;
    auto near = [] (double a, double b, double tolerance) { return std::abs (a - b) <= tolerance; };
    BuiltInStereo plugin;
    stereo::Settings settings { 0, 24, 100, 0, 0 };
    plugin.setSettings (settings); plugin.setMatchingLevel (false); plugin.prepareToPlay (48000, 512);
    juce::AudioBuffer<float> buffer (2, 512); juce::MidiBuffer midi;
    for (int i = 0; i < 512; ++i) { buffer.setSample (0, i, (float) std::sin (i * 0.1)); buffer.setSample (1, i, (float) std::cos (i * 0.1)); }
    auto original = buffer;
    plugin.processBlock (buffer, midi);
    bool same = true;
    for (int i = 0; i < 512; ++i) same = same && juce::exactlyEqual (buffer.getSample (0, i), original.getSample (0, i)) && juce::exactlyEqual (buffer.getSample (1, i), original.getSample (1, i));
    check (same, "Stereo off at 100 percent passes the original sound");
    settings.width = 0; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512); buffer = original; plugin.processBlock (buffer, midi);
    bool mono = true; for (int i = 0; i < 512; ++i) mono = mono && near (buffer.getSample (0, i), buffer.getSample (1, i), 1.0e-6);
    check (mono, "Stereo width zero produces mono");
    settings.width = 200; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512); buffer = original; plugin.processBlock (buffer, midi);
    bool sum = true; for (int i = 0; i < 512; ++i) sum = sum && near (buffer.getSample (0, i) + buffer.getSample (1, i), original.getSample (0, i) + original.getSample (1, i), 1.0e-6);
    check (sum, "Stereo width keeps the mono sum");
    bool doubledSide = true;
    for (int i = 0; i < 512; ++i)
        doubledSide = doubledSide && near (buffer.getSample (0, i) - buffer.getSample (1, i), 2.0 * (original.getSample (0, i) - original.getSample (1, i)), 1.0e-6);
    check (doubledSide, "Stereo width 200 percent doubles the side");
    settings.listen = 1; settings.width = 100; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512); buffer = original; plugin.processBlock (buffer, midi);
    bool monoListen = true;
    for (int i = 0; i < 512; ++i)
        monoListen = monoListen && near (buffer.getSample (0, i), (original.getSample (0, i) + original.getSample (1, i)) * 0.5, 1.0e-6)
                                && near (buffer.getSample (0, i), buffer.getSample (1, i), 1.0e-6);
    check (monoListen, "Stereo mono listen plays the input mid on both channels");
    settings.listen = 2; settings.width = 100; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512); buffer = original; plugin.processBlock (buffer, midi);
    check (near (buffer.getSample (0, 20), (original.getSample (0, 20) - original.getSample (1, 20)) * 0.5, 1.0e-6), "Stereo side listen plays the side");
    juce::AudioBuffer<float> one (1, 16); one.clear(); one.setSample (0, 3, 0.5f); plugin.processBlock (one, midi);
    check (near (one.getSample (0, 3), 0.5, 0), "Stereo leaves a mono buffer unchanged");
    for (const auto& preset : stereo::getPresets()) check (preset.name.isNotEmpty() && preset.why.isNotEmpty(), "Stereo starting point has a name and explanation");
    check (stereo::analyse ({}, {}, {}).joinIntoString (" ").contains ("couple of seconds"), "Stereo advice asks for enough sound");
    for (int slope : { 12, 24, 48 })
        for (double frequency : { 30.0, 60.0, 120.0, 240.0, 1000.0, 10000.0 })
        {
            settings = { 120, slope, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
            double inPower = 0, outPower = 0;
            for (int block = 0; block < 96; ++block)
            {
                for (int i = 0; i < 512; ++i) { const float x = (float) (0.3 * std::sin (2.0 * juce::MathConstants<double>::pi * frequency * (block * 512 + i) / 48000.0)); buffer.setSample (0, i, x); buffer.setSample (1, i, x); }
                plugin.processBlock (buffer, midi);
                if (block > 40) for (int i = 0; i < 512; ++i) { const double x = 0.3 * std::sin (2.0 * juce::MathConstants<double>::pi * frequency * (block * 512 + i) / 48000.0); inPower += x * x; outPower += buffer.getSample (0, i) * buffer.getSample (0, i); }
            }
            check (std::abs (10.0 * std::log10 (outPower / inPower)) < 0.1, "Stereo crossover sums flat across the spectrum");
        }
    for (const auto& test : std::array<std::pair<int, double>, 3> { { { 12, -13.98 }, { 24, -24.61 }, { 48, -48.20 } } })
    {
        settings = { 120, test.first, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
        double before = 0, after = 0;
        for (int block = 0; block < 96; ++block)
        {
            for (int i = 0; i < 512; ++i) { const float x = (float) (0.3 * std::sin (2.0 * juce::MathConstants<double>::pi * 60.0 * (block * 512 + i) / 48000.0)); buffer.setSample (0, i, x); buffer.setSample (1, i, -x); }
            if (block > 40) for (int i = 0; i < 512; ++i) before += buffer.getSample (0, i) * buffer.getSample (0, i);
            plugin.processBlock (buffer, midi);
            if (block > 40) for (int i = 0; i < 512; ++i) after += buffer.getSample (0, i) * buffer.getSample (0, i);
        }
        check (std::abs (10.0 * std::log10 (after / before) - test.second) < 0.5, "Stereo high-band attenuation one octave below crossover");
    }
    settings = { 0, 24, 0, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
    juce::Random widthRandom (332); double widthInput = 0, widthOutput = 0, midEnergy = 0;
    for (int block = 0; block < 100; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            const float l = widthRandom.nextFloat() - 0.5f, r = widthRandom.nextFloat() - 0.5f;
            buffer.setSample (0, i, l); buffer.setSample (1, i, r);
            widthInput += l * l + r * r; midEnergy += (double) (l + r) * (l + r) * 0.5;
        }
        plugin.processBlock (buffer, midi);
        for (int i = 0; i < 512; ++i) widthOutput += buffer.getSample (0, i) * buffer.getSample (0, i) + buffer.getSample (1, i) * buffer.getSample (1, i);
    }
    check (std::abs (10.0 * std::log10 (widthOutput / widthInput) - 10.0 * std::log10 (midEnergy / widthInput)) < 0.02,
           "Stereo width zero has the measured mid-only power on independent noise");
    for (int slope : { 12, 24, 48 })
    {
        settings = { 120, slope, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
        double originalPower = 0, highPower = 0;
        for (int block = 0; block < 96; ++block)
        {
            for (int i = 0; i < 512; ++i) { const float x = (float) (0.3 * std::sin (2.0 * juce::MathConstants<double>::pi * 120.0 * (block * 512 + i) / 48000.0)); buffer.setSample (0, i, x); buffer.setSample (1, i, -x); }
            if (block > 40) for (int i = 0; i < 512; ++i) originalPower += buffer.getSample (0, i) * buffer.getSample (0, i);
            plugin.processBlock (buffer, midi);
            if (block > 40) for (int i = 0; i < 512; ++i) highPower += buffer.getSample (0, i) * buffer.getSample (0, i);
        }
        double measuredLow = 0, measuredInput = 0;
        const auto recent = plugin.inputMeasurement.getBlocks();
        for (size_t i = recent.size() / 2; i < recent.size(); ++i) { measuredLow += recent[i].lowLeft; measuredInput += recent[i].left; }
        check (std::abs (10.0 * std::log10 (highPower / originalPower) + 6.02) < 0.1, "Stereo high band is minus 6 dB at crossover");
        check (std::abs (10.0 * std::log10 (measuredLow / measuredInput) + 6.02) < 0.1, "Stereo low band is minus 6 dB at crossover");
        double pointLow = 0, pointHigh = 0, pointRaw = 0;
        for (const auto& point : plugin.inputMeasurement.getPoints())
        {
            pointLow += point.lowLeft * point.lowLeft; pointHigh += point.highLeft * point.highLeft; pointRaw += point.left * point.left;
        }
        check (std::abs (10.0 * std::log10 (pointLow / pointRaw) + 6.02) < 0.1
                   && std::abs (10.0 * std::log10 (pointHigh / pointRaw) + 6.02) < 0.1,
               "Stereo scope samples contain the real low and high bands at crossover");
    }
    settings = { 120, 24, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
    juce::Random mixRandom (557); double mixIn = 0, mixOut = 0;
    for (int block = 0; block < 120; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi * 60.0 * (block * 512 + i) / 48000.0;
            const float l = (mixRandom.nextFloat() - 0.5f) * 0.5f + (float) (0.1 * std::sin (phase));
            const float r = (mixRandom.nextFloat() - 0.5f) * 0.5f + (float) (0.1 * std::cos (phase));
            buffer.setSample (0, i, l); buffer.setSample (1, i, r);
            if (block > 40) mixIn += l * l + r * r;
        }
        plugin.processBlock (buffer, midi);
        if (block > 40) for (int i = 0; i < 512; ++i) mixOut += buffer.getSample (0, i) * buffer.getSample (0, i) + buffer.getSample (1, i) * buffer.getSample (1, i);
    }
    check (std::abs (10.0 * std::log10 (mixOut / mixIn)) < 1.0, "Stereo mono below changes a wide mix's level by less than 1 dB");
    settings = { 250, 24, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
    double midPower = 0, sidePower = 0;
    for (int block = 0; block < 96; ++block)
    {
        for (int i = 0; i < 512; ++i) { const auto phase = 2.0 * juce::MathConstants<double>::pi * 50.0 * (block * 512 + i) / 48000.0; buffer.setSample (0, i, (float) (0.3 * std::sin (phase))); buffer.setSample (1, i, (float) (0.3 * std::cos (phase))); }
        plugin.processBlock (buffer, midi);
        if (block > 40) for (int i = 0; i < 512; ++i) { const auto m = stereo::midSide (buffer.getSample (0, i), buffer.getSample (1, i)); midPower += m.mid * m.mid; sidePower += m.side * m.side; }
    }
    check (stereo::sideToMidDb (midPower, sidePower) < -50, "Stereo crossover strongly narrows a 50 Hz bass");
    check ((midPower - sidePower) / (midPower + sidePower) > 0.999, "Stereo crossover gives the 50 Hz bass a correlation near one");
    {
        // The measured low side of the output must be far under the input's once the low end is made mono.
        settings = { 120, 24, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
        for (int block = 0; block < 200; ++block)
        {
            for (int i = 0; i < 512; ++i)
            {
                const double n = (double) (block * 512 + i), pi2 = 2.0 * juce::MathConstants<double>::pi;
                const double l = 0.2 * std::sin (pi2 * 60.0 * n / 48000.0) + 0.05 * std::sin (pi2 * 90.0 * n / 48000.0);
                const double r = 0.2 * std::sin (pi2 * 60.0 * n / 48000.0 + 1.2) + 0.05 * std::sin (pi2 * 90.0 * n / 48000.0 - 1.0);
                buffer.setSample (0, i, (float) l); buffer.setSample (1, i, (float) r);
            }
            plugin.processBlock (buffer, midi);
        }
        const auto inBlocks = plugin.inputMeasurement.getBlocks(), outBlocks = plugin.outputMeasurement.getBlocks();
        stereo::Block inSum, outSum;
        for (size_t i = inBlocks.size() - 40; i < inBlocks.size(); ++i) { inSum.lowMid += inBlocks[i].lowMid; inSum.lowSide += inBlocks[i].lowSide; outSum.lowMid += outBlocks[i].lowMid; outSum.lowSide += outBlocks[i].lowSide; }
        const auto inDb = stereo::sideToMidDb (inSum.lowMid, inSum.lowSide), outDb = stereo::sideToMidDb (outSum.lowMid, outSum.lowSide);
        check (inDb > -12, "Stereo wide test bass has a low side within 12 dB of its mid");
        check (outDb < inDb - 20, "Stereo output low side is at least 20 dB further under its mid than the input's");
        const auto lines = stereo::analyse (inBlocks, outBlocks, settings).joinIntoString (" ");
        check (lines.contains ("was " + juce::String (juce::roundToInt (-inDb)) + " dB under") && ! lines.contains ("is now " + juce::String (juce::roundToInt (-inDb)) + " dB"), "Stereo advice reports a different side level after Mono below");
    }
    settings = { 120, 24, 100, 0, 0 }; plugin.setSettings (settings); plugin.prepareToPlay (48000, 512);
    double leftPower = 0, rightPower = 0;
    for (int block = 0; block < 96; ++block)
    {
        for (int i = 0; i < 512; ++i) { buffer.setSample (0, i, (float) (0.3 * std::sin (2.0 * juce::MathConstants<double>::pi * 5000.0 * (block * 512 + i) / 48000.0))); buffer.setSample (1, i, 0); }
        plugin.processBlock (buffer, midi);
        if (block > 40) for (int i = 0; i < 512; ++i) { leftPower += buffer.getSample (0, i) * buffer.getSample (0, i); rightPower += buffer.getSample (1, i) * buffer.getSample (1, i); }
    }
    check (10.0 * std::log10 (std::max (rightPower / leftPower, 1.0e-20)) < -90, "Stereo preserves the position of a high sound on the left");
    std::vector<stereo::Block> measurements (40); for (auto& b : measurements) { b.peak = 0.5f; b.left = b.right = b.cross = b.mid = b.lowLeft = b.lowRight = b.lowCross = b.lowMid = 1.0; }
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("Your track is mono"), "Stereo advice recognises mono");
    for (auto& b : measurements) { b.side = 0.3; b.cross = 0.5; b.lowSide = 0.2; b.lowCross = 0.5; }
    auto narrowed = measurements; for (auto& b : narrowed) { b.lowSide = 0.002; b.lowCross = 0.99; }
    const auto madeMono = stereo::analyse (measurements, narrowed, settings).joinIntoString (" ");
    check (madeMono.contains ("makes the low end mono") && madeMono.contains ("was 7 dB under") && madeMono.contains ("is now 27 dB under"), "Stereo advice recognises wide low end and reports the narrower output");
    const auto barely = stereo::analyse (measurements, measurements, settings).joinIntoString (" ");
    check (barely.contains ("changes this low end little") && ! barely.contains ("makes the low end mono") && barely.contains ("Slope"), "Stereo advice does not claim a mono low end when the output side is unchanged");
    for (auto& b : measurements) b.lowCross = -0.4;
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("work against each other"), "Stereo advice recognises negative low correlation");
    for (auto& b : measurements) { b.lowCross = 0.5; b.lowLeft = b.lowRight = 0.0001; }
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("There's little below"), "Stereo advice recognises little low end");
    for (auto& b : measurements) { b.lowLeft = b.lowRight = 1; b.lowSide = 0.2; }
    settings.monoBelowHz = 0;
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("Try Mono below 120 Hz"), "Stereo advice suggests a crossover when off");
    auto outputMeasurements = measurements;
    for (auto& b : outputMeasurements) b.cross = -0.2;
    settings.width = 180;
    check (stereo::analyse (measurements, outputMeasurements, settings).joinIntoString (" ").contains ("Try 130% or less"), "Stereo advice catches excessive width");
    for (auto& b : measurements) { b.cross = -0.3; b.lowSide = 0.001; }
    settings.width = 100;
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("polarity switch"), "Stereo advice catches negative overall correlation");
    for (auto& b : measurements) { b.cross = 0.45; b.lowSide = 0.003162; }
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("close to mono already"), "Stereo advice recognises a narrow low end");
    settings.listen = 1;
    check (stereo::analyse (measurements, measurements, settings).joinIntoString (" ").contains ("Switch back to Stereo"), "Stereo advice warns about mono listen");
    check (stereo::analyse ({}, {}, settings).size() == 1, "Stereo advice asks for sound before any other advice");
    settings = { 0, 24, 200, 0, 0 }; plugin.setSettings (settings); plugin.setMatchingLevel (true); plugin.prepareToPlay (48000, 512);
    juce::Random random (91); double inputEnergy = 0, outputEnergy = 0;
    for (int block = 0; block < 240; ++block)
    {
        for (int i = 0; i < 512; ++i) { buffer.setSample (0, i, (random.nextFloat() - 0.5f) * 0.4f); buffer.setSample (1, i, (random.nextFloat() - 0.5f) * 0.4f); }
        if (block > 160) for (int i = 0; i < 512; ++i) inputEnergy += buffer.getSample (0, i) * buffer.getSample (0, i) + buffer.getSample (1, i) * buffer.getSample (1, i);
        plugin.processBlock (buffer, midi);
        if (block > 160) for (int i = 0; i < 512; ++i) outputEnergy += buffer.getSample (0, i) * buffer.getSample (0, i) + buffer.getSample (1, i) * buffer.getSample (1, i);
    }
    check (std::abs (10.0 * std::log10 (outputEnergy / inputEnergy)) < 0.5, "Stereo match level evens out wide noise");
    settings = { 0, 24, 100, 0, 0 }; plugin.setSettings (settings); plugin.setMatchingLevel (false); plugin.prepareToPlay (48000, 512);
    double largestStep = 0, previous = 0;
    for (int block = 0; block < 30; ++block)
    {
        if (block == 10) { settings.monoBelowHz = 120; plugin.setSettings (settings); }
        if (block == 20) { settings.slope = 48; plugin.setSettings (settings); }
        for (int i = 0; i < 512; ++i) { const float x = (float) (0.5 * std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * (block * 512 + i) / 48000.0)); buffer.setSample (0, i, x); buffer.setSample (1, i, x); }
        plugin.processBlock (buffer, midi);
        for (int i = 0; i < 512; ++i) { const double x = buffer.getSample (0, i); largestStep = std::max (largestStep, std::abs (x - previous)); previous = x; }
    }
    check (largestStep < 0.1, "Stereo crossover changes do not click");
    auto& monoParameter = *plugin.getKnobParameter (stereo::Knob::monoBelow);
    check (monoParameter.convertFrom0to1 (monoParameter.convertTo0to1 (5.0f)) < 20.0f, "Stereo frequency does not select a frequency below 20 Hz");
    checkThroughProject<BuiltInStereo> (BuiltInStereo::getDescription(), "DawIt Stereo",
        [] (BuiltInStereo& p) { p.setSettings ({ 200, 48, 150, 2, 3 }); p.setMatchingLevel (false); },
        [&] (BuiltInStereo& p) { const auto s = p.getSettings(); return near (s.monoBelowHz, 200, 1) && s.slope == 48 && near (s.width, 150, 0.1) && s.listen == 2 && near (s.outputDb, 3, 0.1) && ! p.isMatchingLevel(); });
}

static void runLoudnessMeterInitialTests()
{
    for (const double sampleRate : { 44100.0, 48000.0 })
    {
        daw::loudness::Meter meter;
        meter.prepare (sampleRate);
        check (! meter.integrated().has_value(), "Loudness silence has no integrated reading");
        for (int i = 0; i < (int) (sampleRate * 4.0); ++i)
        {
            const auto sample = (float) (0.1 * std::sin (2.0 * juce::MathConstants<double>::pi * 997.0 * i / sampleRate));
            meter.push (sample, sample, 2);
        }
        check (std::abs (meter.momentary().value_or (0.0) + 20.0) < 0.05, "Stereo momentary loudness at both sample rates");
        check (std::abs (meter.shortTerm().value_or (0.0) + 20.0) < 0.05, "Stereo short-term loudness at both sample rates");
        check (std::abs (meter.integrated().value_or (0.0) + 20.0) < 0.05, "Stereo integrated loudness at both sample rates");
        meter.reset();
        check (! meter.integrated().has_value(), "Loudness reset clears integrated reading");
        for (int i = 0; i < (int) (sampleRate * 4.0); ++i)
        {
            const auto sample = (float) (0.1 * std::sin (2.0 * juce::MathConstants<double>::pi * 997.0 * i / sampleRate));
            meter.push (sample, 0.0f, 1);
        }
        check (std::abs (meter.integrated().value_or (0.0) + 23.01) < 0.05, "Mono loudness is three LU below stereo");
    }
}

static void runStereoMeasureTests()
{
    using namespace daw::stereo;
    const auto ms = midSide (0.8, 0.4);
    check (std::abs (ms.mid - 0.6) < 1.0e-12 && std::abs (ms.side - 0.2) < 1.0e-12, "Stereo mid and side");
    check (! correlation (0.0, 0.0, 0.0).has_value(), "Stereo silence has no correlation");
    check (std::abs (correlation (1.0, 0.0, 0.0).value_or (-2.0)) < 1.0e-12, "One silent side reads zero correlation");
    check (std::abs (correlation (1.0, 1.0, -1.0).value_or (0.0) + 1.0) < 1.0e-12, "Opposite channels correlate at minus one");
    for (const int slope : { 12, 24, 48 })
        check (std::abs (20.0 * std::log10 (highMagnitude (1.0, slope)) + 6.0206) < 0.001, "Crossover band is minus 6 dB at cutoff");
    check (std::abs (20.0 * std::log10 (highMagnitude (0.5, 24)) + 24.61) < 0.02, "LR4 high band one octave below cutoff");
    check (std::abs (20.0 * std::log10 (highMagnitude (0.5, 12)) + 13.98) < 0.02
           && std::abs (20.0 * std::log10 (highMagnitude (0.5, 48)) + 48.20) < 0.02,
           "LR2 and LR8 high bands one octave below cutoff");
    check (std::abs (widthLevelDb (1.0, 1.0, 1.5) - 2.11) < 0.01, "Width level for noise at 150 percent");
    check (std::abs (widthCorrelation (4.0, 1.0, 1.5) - 0.28) < 0.01, "Width correlation for pad at 150 percent");
    check (std::abs (monoDropDb (-0.5) + 6.02) < 0.01, "Mono drop at negative correlation");
    double leftPower = 0, rightPower = 0, cross = 0;
    juce::Random random (924);
    for (int i = 0; i < 480000; ++i)
    {
        const double l = random.nextFloat() - 0.5, r = random.nextFloat() - 0.5;
        leftPower += l * l; rightPower += r * r; cross += l * r;
    }
    check (std::abs (correlation (leftPower, rightPower, cross).value_or (1.0)) < 0.03, "Independent noise has near-zero correlation");
    leftPower = rightPower = cross = 0;
    for (int i = 0; i < 48000; ++i)
    {
        const double phase = 2.0 * juce::MathConstants<double>::pi * 55.0 * i / 48000.0;
        const double l = std::sin (phase), r = std::cos (phase);
        leftPower += l * l; rightPower += r * r; cross += l * r;
    }
    check (std::abs (correlation (leftPower, rightPower, cross).value_or (1.0)) < 0.01, "A ninety-degree stereo sine has zero correlation");
    check (std::abs (widthLevelDb (1.0, 1.0, 2.0) - 3.98) < 0.01, "Width level for noise at 200 percent");
    Measurement measurement;
    measurement.prepare (1000.0);
    for (int i = 0; i < 1000; ++i) measurement.push (0.5f, 0.5f, 0.25f, 0.25f);
    check (measurement.getCorrelation (false).value_or (0.0) > 0.999, "Live measurement of mono correlation");
    check (measurement.getBlocks().size() == 20 && ! measurement.getPoints().empty(), "Stereo readers get measurement history");
}

//==============================================================================
namespace
{
/** An engine with a fake device that has input channels and reported latencies, and one audio track "Vocals". Takes go to
    a folder of its own. feed() plays blocks of audio through the device, with the input given by a function. */
struct RecordingRig
{
    explicit RecordingRig (int inputs, int inLatency = 0, int outLatency = 0)
    {
        engine.deviceManager.removeAudioCallback (&engine);
        device.numInputs = inputs;
        device.inputLatency = inLatency;
        device.outputLatency = outLatency;
        engine.audioDeviceAboutToStart (&device);
        folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                     .getChildFile ("dawit-rec-" + juce::String (juce::Random::getSystemRandom().nextInt64()));
        folder.createDirectory();
        engine.setRecordingFolder (audioFiles());
        track = engine.addTrack (daw::Track::Type::audio);
        track->name = "Vocals";
    }

    ~RecordingRig()
    {
        engine.stopRecording();
        folder.deleteRecursively();
    }

    juce::File audioFiles() const { return folder.getChildFile ("Audio Files"); }

    /** Plays `seconds` of blocks; the input is `input (channel, sample)`, the sample counted from the first block fed.
        Every two seconds the engine does what its 30 Hz timer does, so the FIFOs are drained as they are in the app. */
    juce::AudioBuffer<float> feed (double seconds, const std::function<float (int, juce::int64)>& input, bool pump = true)
    {
        const int numBlocks = (int) std::ceil (seconds * testRate / testBlock);
        juce::AudioBuffer<float> out (2, numBlocks * testBlock);
        juce::AudioBuffer<float> in (juce::jmax (1, device.numInputs), testBlock);

        for (int b = 0; b < numBlocks; ++b)
        {
            const float* inPointers[8] = {};
            float* outPointers[] = { out.getWritePointer (0, b * testBlock), out.getWritePointer (1, b * testBlock) };

            for (int ch = 0; ch < device.numInputs; ++ch)
            {
                for (int i = 0; i < testBlock; ++i)
                    in.setSample (ch, i, input ? input (ch, cursor + i) : 0.0f);

                inPointers[ch] = in.getReadPointer (ch);
            }

            engine.audioDeviceIOCallbackWithContext (inPointers, device.numInputs, outPointers, 2, testBlock, {});
            cursor += testBlock;

            if (pump && b % 375 == 374)
                engine.serviceRecording();
        }

        return out;
    }

    daw::Engine engine { true };   // scratch settings: a measured latency must never reach the user's own
    FakeDevice device;
    juce::File folder;
    daw::Track* track = nullptr;
    juce::int64 cursor = 0;
};

float sineAt (juce::int64 n, double hz, float amplitude)
{
    return amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * hz * (double) n / testRate);
}

/** The audio clips of a track, in the order they were added. */
std::vector<daw::AudioClip*> audioClipsOf (const daw::Track& track)
{
    std::vector<daw::AudioClip*> clips;

    for (auto& c : track.clips)
        if (c->kind == daw::Clip::Kind::audio)
            clips.push_back (static_cast<daw::AudioClip*> (c.get()));

    return clips;
}
} // namespace

static void runComputerKeyboardTests()
{
    using daw::ComputerKeyboard;
    std::cout << "Computer keyboard" << std::endl;

    const auto noteOf = [] (ComputerKeyboard& k, char c) -> int
    {
        const auto p = k.press (c);
        return p.noteOn ? p.noteOn->getNoteNumber() : -1;
    };

    {
        ComputerKeyboard k;
        check (noteOf (k, 'a') == 60 && noteOf (k, 'w') == 61 && noteOf (k, 's') == 62 && noteOf (k, 'k') == 72 && noteOf (k, 'l') == 74 && noteOf (k, 'p') == 75,
               "computer keyboard: a, w, s, k, l and p play 60, 61, 62, 72, 74 and 75");
        check (noteOf (k, 'r') == -1 && noteOf (k, 'i') == -1 && noteOf (k, 'q') == -1 && noteOf (k, 'm') == -1
                   && ! k.press ('r').handled && ! k.press ('q').handled && ! ComputerKeyboard::isMapped ('m'),
               "computer keyboard: r, i, q and m play nothing and are not taken");
    }
    {
        ComputerKeyboard k;
        check (noteOf (k, 'A') == 60, "computer keyboard: an upper-case A plays 60");
    }
    {
        ComputerKeyboard k;
        k.press ('x');
        check (noteOf (k, 'a') == 72, "computer keyboard: X then A plays 72");
    }
    {
        ComputerKeyboard k;
        for (int i = 0; i < 4; ++i)
            k.press ('z');
        const int low = noteOf (k, 'a');
        k.press ('z');
        check (low == 12 && k.getOctaveNote() == 12, "computer keyboard: four Zs put A on 12, and a fifth stays there");
    }
    {
        ComputerKeyboard k;
        for (int i = 0; i < 4; ++i)
            k.press ('x');
        const int high = noteOf (k, 'p');
        k.press ('x');
        check (high == 123 && k.getOctaveNote() == 108, "computer keyboard: four Xs then P plays 123, and a fifth X leaves A at 108");
    }
    {
        ComputerKeyboard k;
        check (k.getVelocity() == 100, "computer keyboard: velocity starts at 100");
        k.press ('c');
        check (k.getVelocity() == 80, "computer keyboard: C once gives 80");
        for (int i = 0; i < 5; ++i)
            k.press ('c');
        check (k.getVelocity() == 20, "computer keyboard: C five times stops at 20");
        k.setVelocity (100);
        k.press ('v');
        const int two = k.getVelocity();
        k.press ('v');
        const int three = k.getVelocity();
        k.press ('v');
        check (two == 120 && three == 127 && k.getVelocity() == 127, "computer keyboard: V from 100 gives 120, then 127, and stays");
        const auto on = k.press ('a').noteOn;
        check (on && on->getVelocity() == 127 && on->getChannel() == 1, "computer keyboard: the note carries the velocity on channel 1");
    }
    {
        ComputerKeyboard k;
        k.press ('a');
        k.press ('x');
        const auto off = k.release ('a');
        check (off && off->isNoteOff() && off->getNoteNumber() == 60, "computer keyboard: an octave change while A is held still ends note 60");
        check (! k.release ('a') && ! k.release ('d'), "computer keyboard: releasing a key that isn't held gives nothing");
    }
    {
        ComputerKeyboard k;
        const bool first = k.press ('a').noteOn.has_value();
        const auto second = k.press ('a');
        check (first && second.handled && ! second.noteOn, "computer keyboard: A pressed twice without a release gives one Note On");
    }
    {
        ComputerKeyboard k;
        k.press ('a');
        k.press ('e');
        k.press ('g');
        const auto offs = k.releaseAll();
        check (offs.size() == 3 && offs[0].getNoteNumber() == 60 && offs[1].getNoteNumber() == 63 && offs[2].getNoteNumber() == 67
                   && k.getHeldLetters().empty(),
               "computer keyboard: release all with A, E and G held gives Note Offs 60, 63 and 67");
    }
    {
        ComputerKeyboard k;
        const auto silentC = k.getSilencedLetters (0xAB5);   // pitch classes 0 2 4 5 7 9 11
        const auto silentG = k.getSilencedLetters (0xAD5);   // 7 9 11 0 2 4 6
        const auto sorted = [] (std::string s) { std::sort (s.begin(), s.end()); return s; };
        check (sorted (silentC) == "eoptuwy", "computer keyboard: in C major the black keys W E T Y U O P are silent");
        check (sorted (silentG) == sorted ("fweyuop"), "computer keyboard: in G major F is silent and T sounds");
    }
    {
        ComputerKeyboard k;
        k.setOctaveNote (200);
        const int top = k.getOctaveNote();
        k.setVelocity (90);
        check (top == 108 && k.getVelocity() == 100, "computer keyboard: an octave of 200 becomes 108 and a velocity of 90 snaps up to 100");
        k.setOctaveNote (65);
        check (k.getOctaveNote() == 60, "computer keyboard: an octave of 65 snaps to 60");
    }

    // The messages record like a MIDI keyboard's.
    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        auto* keys = engine.addTrack (daw::Track::Type::midi);
        engine.setArmed (*keys, true);
        engine.setPositionBeats (0.0);
        const auto tone = [] (int, juce::int64 n) { return sineAt (n, 440.0, 0.5f); };
        check (engine.startRecording(), "computer keyboard: a MIDI track armed starts recording");
        ComputerKeyboard k;
        rig.feed (0.25, tone);
        if (const auto on = k.press ('a').noteOn)
            engine.addMidiInput (*on);
        rig.feed (0.5, tone);
        if (const auto off = k.release ('a'))
            engine.addMidiInput (*off);
        rig.feed (0.25, tone);
        engine.stopRecording();
        const auto* clip = keys->clips.empty() ? nullptr : dynamic_cast<daw::MidiClip*> (keys->clips[0].get());
        check (clip != nullptr && clip->notes.size() == 1 && clip->notes[0].pitch == 60 && std::abs (clip->notes[0].velocity - 0.787f) < 0.01f,
               "computer keyboard: A is recorded as pitch 60 at velocity 0.787");
    }
}

static void runAudioRecordingTests()
{
    std::cout << "Audio recording" << std::endl;

    const auto tone = [] (int, juce::int64 n) { return sineAt (n, 440.0, 0.5f); };

    // Saving the input choice.
    {
        daw::Engine engine;
        engine.deviceManager.removeAudioCallback (&engine);
        auto* vocals = engine.addTrack (daw::Track::Type::audio);
        auto* keys = engine.addTrack (daw::Track::Type::audio);
        check (vocals->inputChannel.load() == 0 && ! vocals->inputStereo.load() && ! vocals->armed.load() && ! vocals->monitor.load(),
               "an audio track starts on Input 1, mono, not armed and not monitoring");
        check (! daw::project::toText (engine, {}).contains ("input"), "Input 1, mono, isn't written to the project");

        engine.setInput (*vocals, 2, false);
        engine.setInput (*keys, 0, true);
        engine.setArmed (*vocals, true);
        engine.setMonitor (*vocals, true);
        const auto text = daw::project::toText (engine, {});
        check (text.contains ("\"inputChannel\": 2") && text.contains ("\"inputStereo\": true"), "another input, and a stereo pair, are written");
        check (! text.contains ("armed") && ! text.contains ("monitor"), "arming and Monitor aren't saved");

        auto file = juce::File::createTempFile (".dawit");
        juce::String saved, error;
        juce::StringArray warnings;
        daw::Engine loaded;
        loaded.deviceManager.removeAudioCallback (&loaded);
        check (daw::project::save (engine, file, saved, error) && daw::project::load (loaded, file, error, warnings) && loaded.getTracks().size() == 2
                   && loaded.getTracks()[0]->inputChannel.load() == 2 && ! loaded.getTracks()[0]->inputStereo.load()
                   && loaded.getTracks()[1]->inputChannel.load() == 0 && loaded.getTracks()[1]->inputStereo.load()
                   && ! loaded.getTracks()[0]->armed.load(),
               "the input choice round trips through save and load, unarmed " + error);
        file.deleteFile();
        file.getSiblingFile ("Audio Files").deleteRecursively();

        engine.setInput (*vocals, 0, false);
        engine.setInput (*keys, 0, false);
        check (! daw::project::toText (engine, {}).contains ("inputChannel"), "a project without an input choice loads as Input 1, mono, and is written without one");
    }

    // Monitoring and the input level.
    {
        RecordingRig rig (2);
        auto& engine = rig.engine;
        auto* vocals = rig.track;
        auto* other = engine.addTrack (daw::Track::Type::audio);
        const auto minus12 = [] (int, juce::int64 n) { return sineAt (n, 440.0, 0.251f); };
        engine.play();

        engine.setArmed (*vocals, true);
        auto out = rig.feed (0.5, minus12);
        check (rms (out, 0) < 1e-5f, "armed with Monitor off, the input isn't heard");
        check (std::abs (juce::Decibels::gainToDecibels (vocals->inputMeterLevel.load()) - -12.0f) < 0.2f, "the input level of an armed track reads -12 dB for a -12 dB input");
        check (other->inputMeterLevel.load() == 0.0f, "a track that isn't armed has no input level");

        engine.setMonitor (*vocals, true);
        out = rig.feed (0.5, minus12);
        double dot = 0.0, a = 0.0, b = 0.0;

        for (int i = 4800; i < out.getNumSamples(); ++i)
        {
            const double x = out.getSample (0, i), y = sineAt (rig.cursor - out.getNumSamples() + i, 440.0, 0.251f);
            dot += x * y;
            a += x * x;
            b += y * y;
        }

        check (a > 0.0 && dot / std::sqrt (a * b) > 0.99, "Monitor on: the output follows the input");
        const float monitored = rms (out, 0, 4800);

        vocals->volumeDb = -6.0f;
        out = rig.feed (0.5, minus12);
        check (std::abs (juce::Decibels::gainToDecibels (rms (out, 0, 4800) / monitored) + 6.0f) < 0.1f, "the track's fader sets the monitored level: -6 dB is 6.0 dB lower");

        vocals->muted = true;
        out = rig.feed (0.5, minus12);
        check (rms (out, 0, 4800) < 1e-5f, "a muted track's monitoring is silent");
        vocals->muted = false;
        vocals->volumeDb = 0.0f;

        engine.setArmed (*vocals, false);
        out = rig.feed (0.5, minus12);
        check (rms (out, 0, 4800) < 1e-5f && vocals->inputMeterLevel.load() == 0.0f, "a track that isn't armed never hears the input, even with Monitor on");

        engine.setMonitor (*other, true);
        out = rig.feed (0.3, minus12);
        check (rms (out, 0) < 1e-5f, "Monitor does nothing without arming");
    }

    // The inputs open on demand, and a missing channel falls back to Input 1.
    {
        RecordingRig rig (2);
        auto& engine = rig.engine;
        auto* midi = engine.addTrack (daw::Track::Type::midi);
        check (! engine.areInputsWanted(), "no inputs are wanted at start");
        engine.setArmed (*rig.track, true);
        check (engine.areInputsWanted(), "arming an audio track wants the inputs");
        engine.setArmed (*midi, true);
        engine.setArmed (*rig.track, false);
        check (! engine.areInputsWanted(), "disarming the last audio track doesn't want them, whatever a MIDI track does");
        engine.setInputMeterWanted (true);
        check (engine.areInputsWanted(), "the settings panel's meter wants them");
        engine.setInputMeterWanted (false);
        check (! engine.areInputsWanted(), "and stops wanting them");
        engine.setArmed (*midi, false);

        const std::pair expected { 0, 0 };
        check (daw::resolveInputChannels (5, false, 2) == expected && daw::resolveInputChannels (1, true, 2) == std::pair (1, 1)
                   && daw::resolveInputChannels (0, true, 2) == std::pair (0, 1) && daw::resolveInputChannels (0, false, 0) == std::pair (-1, -1),
               "a channel the device doesn't have falls back to Input 1, a pair at the last channel repeats it, no inputs gives none");

        // A track on Input 2 hears the second channel.
        engine.setArmed (*rig.track, true);
        engine.setInput (*rig.track, 1, false);
        engine.play();
        rig.feed (0.2, [] (int ch, juce::int64 n) { return ch == 1 ? sineAt (n, 440.0, 0.1f) : 0.0f; });
        check (std::abs (juce::Decibels::gainToDecibels (rig.track->inputMeterLevel.load()) - -20.0f) < 0.3f && ! engine.isInputAllZero(),
               "an armed track meters the input channel it chose");
    }

    // The permission check: exactly zero from an armed input for a second.
    {
        RecordingRig rig (2);
        auto& engine = rig.engine;
        engine.setArmed (*rig.track, true);
        rig.feed (0.9, nullptr);
        check (! engine.isInputAllZero(), "under a second of zero input isn't reported");
        rig.feed (0.3, nullptr);
        check (engine.isInputAllZero(), "a second of exactly zero input from an armed track is reported");
        rig.feed (0.1, [] (int, juce::int64 n) { return n % 2 == 0 ? 1.0e-6f : 0.0f; });
        check (! engine.isInputAllZero(), "any sample that isn't zero clears it");
        engine.setArmed (*rig.track, false);
        rig.feed (1.2, nullptr);
        check (! engine.isInputAllZero(), "and nothing is reported while no track is armed");
    }

    // Recording: the latency shift, the capture beat and the channels.
    {
        RecordingRig rig (2, 200, 300);
        auto& engine = rig.engine;
        engine.setArmed (*rig.track, true);
        engine.setPositionBeats (4.0);
        check (engine.getLatencyShiftSamples() == 500, "the shift is the reported input and output latency");
        check (engine.startRecording() && engine.isRecording() && engine.isPlaying(), "Record with an audio track armed starts");
        rig.feed (1.0, tone);
        engine.serviceRecording();
        check (rig.track->clips.empty(), "the take isn't in the edit while recording");
        check (engine.getLiveTakes().size() == 1 && engine.getLiveTakes()[0].startBeat == 4.0 && engine.getLiveTakes()[0].seconds > 0.5,
               "the recorder shows the take growing, from the beat it started");
        engine.stopRecording();

        const auto clips = audioClipsOf (*rig.track);
        check (clips.size() == 1 && clips[0]->name == "Vocals take 1" && clips[0]->audio.getNumChannels() == 1 && ! clips[0]->muted,
               "stopping makes a mono clip named after the track and its take");
        bool exact = clips.size() == 1 && clips[0]->audio.getNumSamples() >= 47000;

        for (int i = 0; exact && i < 1000; ++i)
            exact = std::abs (clips[0]->audio.getSample (0, i) - tone (0, 500 + i)) < 1.2e-7f;

        check (exact, "sample 0 of the take is input sample 500: the take is moved back by the round trip");
        check (clips.size() == 1 && clips[0]->startBeat == 4.0, "the clip starts at the capture beat, not snapped to a bar");
        check (clips.size() == 1 && clips[0]->file.isAChildOf (rig.audioFiles()) && clips[0]->file.existsAsFile()
                   && rig.audioFiles().findChildFiles (juce::File::findFiles, false, "*.wav").size() == 1,
               "the take's file sits in the Audio Files folder, with no raw file left");
        check (engine.getRecordingReportVersion() == 1 && engine.getLastRecordingReport().size() == 1
                   && engine.getLastRecordingReport()[0].numTakes == 1 && std::abs (engine.getLastRecordingReport()[0].peakDb - -6.0) < 0.1
                   && engine.getLastRecordingReport()[0].lostSeconds == 0.0,
               "the report has the take's loudest peak and no lost samples");

        // A stereo pair gives a clip with each side matching its input.
        engine.setInput (*rig.track, 0, true);
        engine.setPositionBeats (20.0);
        rig.cursor = 0;
        engine.startRecording();
        rig.feed (0.5, [] (int ch, juce::int64 n) { return ch == 0 ? sineAt (n, 440.0, 0.5f) : sineAt (n, 660.0, 0.25f); });
        engine.stopRecording();
        const auto both = audioClipsOf (*rig.track);
        bool sides = both.size() == 2 && both[1]->audio.getNumChannels() == 2;

        for (int i = 0; sides && i < 1000; ++i)
            sides = std::abs (both[1]->audio.getSample (0, i) - sineAt (500 + i, 440.0, 0.5f)) < 1.2e-7f
                    && std::abs (both[1]->audio.getSample (1, i) - sineAt (500 + i, 660.0, 0.25f)) < 1.2e-7f;

        check (sides && both[1]->name == "Vocals take 2", "Inputs 1+2 give a clip with two channels, each side matching its input");

        // Input 2 alone.
        engine.setInput (*rig.track, 1, false);
        engine.setPositionBeats (40.0);
        rig.cursor = 0;
        engine.startRecording();
        rig.feed (0.3, [] (int ch, juce::int64 n) { return ch == 1 ? sineAt (n, 660.0, 0.25f) : sineAt (n, 440.0, 0.5f); });
        engine.stopRecording();
        const auto third = audioClipsOf (*rig.track);
        check (third.size() == 3 && third[2]->audio.getNumChannels() == 1 && std::abs (third[2]->audio.getSample (0, 100) - sineAt (600, 660.0, 0.25f)) < 1.2e-7f,
               "a track on Input 2 records the second channel");
    }

    // The count-in: nothing is recorded until it ends.
    {
        RecordingRig rig (1, 200, 300);
        auto& engine = rig.engine;
        engine.countInEnabled = true;
        engine.setArmed (*rig.track, true);
        engine.setPositionBeats (8.0);
        engine.startRecording();
        rig.feed (3.0, tone);
        engine.stopRecording();
        const auto clips = audioClipsOf (*rig.track);
        check (clips.size() == 1 && clips[0]->startBeat == 8.0 && std::abs (clips[0]->getLengthSeconds() - (1.0 - 500.0 / testRate)) < 0.01
                   && std::abs (clips[0]->audio.getSample (0, 0) - tone (0, 96000 + 500)) < 1.2e-7f,
               "a count-in of one bar records nothing until it ends: the take starts after it, at the playhead");
    }

    // Silence is removed; quiet takes stay; old clips under a take are muted.
    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        engine.setArmed (*rig.track, true);

        const auto record = [&] (double amplitude, double beat)
        {
            engine.setPositionBeats (beat);
            rig.cursor = 0;
            engine.startRecording();
            rig.feed (0.5, [amplitude] (int, juce::int64 n) { return sineAt (n, 440.0, (float) amplitude); });
            engine.stopRecording();
        };

        record (0.0, 0.0);
        check (audioClipsOf (*rig.track).empty() && rig.audioFiles().findChildFiles (juce::File::findFiles, false, "*.wav").isEmpty()
                   && engine.getLastRecordingReport().size() == 1 && engine.getLastRecordingReport()[0].removedAsSilent,
               "a take of silence leaves no clip and no file, and the report says so");
        record (juce::Decibels::decibelsToGain (-61.0), 0.0);
        check (audioClipsOf (*rig.track).empty() && rig.audioFiles().findChildFiles (juce::File::findFiles, false, "*.wav").isEmpty(),
               "a take peaking at -61 dB is removed with its file");
        record (juce::Decibels::decibelsToGain (-59.0), 0.0);
        check (audioClipsOf (*rig.track).size() == 1 && ! engine.getLastRecordingReport()[0].removedAsSilent, "one at -59 dB stays");

        // Recording over a clip mutes it.
        auto* old = engine.addAudioClip (*rig.track, juce::AudioBuffer<float> (1, 96000), testRate, 0.0, "Old");
        auto* far = engine.addAudioClip (*rig.track, juce::AudioBuffer<float> (1, 48000), testRate, 40.0, "Far");
        const auto* quiet = audioClipsOf (*rig.track)[0];
        record (0.5, 1.0);
        const auto clips = audioClipsOf (*rig.track);
        check (clips.size() == 4 && old->muted && quiet->muted && ! far->muted && ! clips[3]->muted && clips[3]->name == "Vocals take 2",
               "an old clip the take overlaps ends muted, a clip elsewhere keeps playing, the new take plays");
    }

    // Deleting the track while recording leaves no file; MIDI and audio record together; nothing armed, nothing to record.
    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        engine.setArmed (*rig.track, true);
        engine.startRecording();
        rig.feed (0.5, tone);
        engine.removeTrack (rig.track);
        rig.feed (0.5, tone);
        engine.stopRecording();
        check (rig.audioFiles().findChildFiles (juce::File::findFiles, true, "*").isEmpty() && engine.getTracks().empty(),
               "deleting the track while recording leaves no file");
        check (! engine.startRecording() && ! engine.isRecording(), "nothing armed and no MIDI track: Record says no");
    }

    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        auto* keys = engine.addTrack (daw::Track::Type::midi);
        engine.setArmed (*rig.track, true);
        engine.setArmed (*keys, true);
        engine.setPositionBeats (0.0);
        check (engine.startRecording(), "MIDI and audio armed together record");
        rig.feed (0.25, tone);
        engine.addMidiInput (juce::MidiMessage::noteOn (1, 60, 0.8f));
        rig.feed (0.5, tone);
        engine.addMidiInput (juce::MidiMessage::noteOff (1, 60));
        rig.feed (0.25, tone);
        engine.stopRecording();
        check (audioClipsOf (*rig.track).size() == 1 && keys->clips.size() == 1 && static_cast<daw::MidiClip&> (*keys->clips[0]).notes.size() == 1,
               "and give both takes");
    }

    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        auto* keys = engine.addTrack (daw::Track::Type::midi);
        check (! engine.startRecording(), "an audio track that isn't armed and no live MIDI track: nothing to record on");
        engine.setLiveMidiTrack (keys);
        check (engine.startRecording() && keys->armed.load(), "with a live MIDI track Record arms it, as before");
        engine.stopRecording();
    }

    // Samples lost when the message thread stalls: the take keeps its place, with silence.
    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        engine.setArmed (*rig.track, true);
        engine.startRecording();
        rig.feed (11.0, tone, false);
        engine.stopRecording();
        const auto& report = engine.getLastRecordingReport();
        check (report.size() == 1 && report[0].lostSeconds > 0.9 && report[0].lostSeconds < 1.1, "a stall of more than ten seconds is counted as lost samples");
        const auto clips = audioClipsOf (*rig.track);
        check (clips.size() == 1 && clips[0]->audio.getNumSamples() >= 11 * 48000 - 8, "the take is as long as the time recorded, the lost stretch silent");
    }

    // Loop recording: every pass is a take, the newest complete one plays.
    {
        RecordingRig rig (1);
        auto& engine = rig.engine;
        engine.setArmed (*rig.track, true);
        engine.loopStartBeat = 8.0;
        engine.loopEndBeat = 16.0;
        engine.loopEnabled = true;
        engine.setPositionBeats (12.0);
        engine.startRecording();
        rig.feed (12.0, tone);
        engine.stopRecording();
        engine.loopEnabled = false;

        const auto clips = audioClipsOf (*rig.track);
        check (clips.size() == 4, "recording across the loop end for 2.5 loops makes four takes");

        if (clips.size() == 4)
        {
            check (clips[0]->startBeat == 12.0 && clips[1]->startBeat == 8.0 && clips[2]->startBeat == 8.0 && clips[3]->startBeat == 8.0,
                   "the first pass starts where recording did, the others at the loop start");
            check (std::abs (clips[1]->audio.getNumSamples() - 192000) <= 1 && std::abs (clips[2]->audio.getNumSamples() - 192000) <= 1
                       && std::abs (clips[0]->audio.getNumSamples() - 96000) <= 1,
                   "passes that ran through the loop last 4.0 s, the first 2.0 s");
            check (clips[0]->muted && clips[1]->muted && ! clips[2]->muted && clips[3]->muted, "pass 3, the newest complete one, plays; the others are muted");
            check (clips[0]->name == "Vocals take 1" && clips[3]->name == "Vocals take 4", "the takes are numbered in order");

            engine.chooseTake (*rig.track, *clips[1]);
            check (! clips[1]->muted && clips[0]->muted && clips[2]->muted && clips[3]->muted, "choosing take 2 unmutes it and mutes the others there");
            engine.deleteOtherTakes (*rig.track, *clips[1]);
            check (audioClipsOf (*rig.track).size() == 1 && ! audioClipsOf (*rig.track)[0]->muted, "deleting the other takes leaves the one chosen, playing");
        }
    }

    // The pure helpers.
    {
        daw::takes::ClipList list;
        const auto add = [&] (double start, int samples)
        {
            auto clip = std::make_unique<daw::AudioClip>();
            clip->startBeat = start;
            clip->sourceSampleRate = testRate;
            clip->audio.setSize (1, samples);
            list.push_back (std::move (clip));
        };

        add (8.0, 96000);    // beats 8 to 12
        add (4.0, 96000);    // 4 to 8: only touches
        add (10.0, 48000);   // 10 to 12
        add (11.0, 96000);   // 11 to 15
        list.push_back (std::make_unique<daw::MidiClip>());
        const daw::TempoMap tempo (120.0);
        const auto first = static_cast<daw::AudioClip*> (list[0].get());
        const auto overlapping = daw::takes::findOverlapping (list, *first, tempo);
        check (overlapping.size() == 3 && overlapping[0] == first && overlapping[1]->startBeat == 10.0 && overlapping[2]->startBeat == 11.0,
               "the takes of a clip are the audio clips that overlap it, itself included, by start; clips that only touch aren't");
        const auto changed = daw::takes::choose (list, *static_cast<daw::AudioClip*> (list[2].get()), tempo);
        check (changed.size() == 2 && ! list[2]->muted && list[0]->muted && list[3]->muted && ! list[1]->muted,
               "choosing one unmutes it and mutes the others that overlap it, and no more");
        check (daw::takes::findOthers (list, *first, tempo).size() == 2, "the other takes leave out the clip itself");
    }
}

/** Plays the engine's latency measuring against a fake loop-back: the input is the output `delay` samples later,
    only for the clicks in `echoed` (their numbers from 0), and scaled by `gain`. Returns the engine's result. */
static daw::Engine::LatencyMeasurement measureWithEcho (RecordingRig& rig, int delay, const std::set<int>& echoed, float gain = 1.0f,
                                                        bool* started = nullptr)
{
    rig.feed (0.02, nullptr, false);   // a first block, so the engine knows the device has inputs
    const bool ok = rig.engine.startLatencyMeasurement();

    if (started != nullptr)
        *started = ok;

    std::vector<float> history;
    juce::AudioBuffer<float> in (1, testBlock), out (2, testBlock);
    const int spacing = (int) std::lround (daw::latency::spacingSeconds * testRate);
    const int lead = (int) std::lround (daw::latency::leadSeconds * testRate);

    for (int block = 0; block < 600 && ok && rig.engine.isMeasuringLatency(); ++block)
    {
        for (int i = 0; i < testBlock; ++i)
        {
            const int g = block * testBlock + i, p = g - delay;
            float v = 0.0f;

            if (p >= 0 && p < (int) history.size() && p >= lead)
                if (echoed.count ((p - lead) / spacing) > 0)
                    v = history[(size_t) p] * gain;

            in.setSample (0, i, v);
        }

        const float* inPointers[] = { in.getReadPointer (0) };
        float* outPointers[] = { out.getWritePointer (0), out.getWritePointer (1) };
        rig.engine.audioDeviceIOCallbackWithContext (inPointers, 1, outPointers, 2, testBlock, {});

        for (int i = 0; i < testBlock; ++i)
            history.push_back (out.getSample (0, i));
    }

    rig.engine.serviceLatencyMeasurement();
    return rig.engine.getLastLatencyMeasurement();
}

static void runLatencyMeasureTests()
{
    std::cout << "Measuring latency" << std::endl;

    const std::set<int> all { 0, 1, 2, 3, 4, 5, 6, 7 };

    {
        const auto click = daw::latency::makeClick (testRate);
        check (std::abs (click[0] - juce::Decibels::decibelsToGain (-12.0f)) < 1.0e-6f, "the click starts at -12 dBFS");
        float peak = 0.0f;

        for (auto v : click)
            peak = juce::jmax (peak, std::abs (v));

        check (peak <= juce::Decibels::decibelsToGain (-12.0f) + 1.0e-6f, "the click never goes above -12 dBFS");
        check (daw::latency::getClickStart (1, testRate) - daw::latency::getClickStart (0, testRate) == 12000, "the clicks are 0.25 s apart");
    }

    // The finder on its own.
    {
        const int total = daw::latency::getTotalSamples (testRate);
        const auto click = daw::latency::makeClick (testRate);
        auto heardAt = [&] (int delay, int numClicks)
        {
            std::vector<float> heard ((size_t) total, 0.0f);

            for (int i = 0; i < numClicks; ++i)
                for (size_t n = 0; n < click.size(); ++n)
                    heard[(size_t) (daw::latency::getClickStart (i, testRate) + delay) + n] = click[n] * 0.5f;

            return heard;
        };

        auto r = daw::latency::findRoundTrip (heardAt (437, 8), testRate);
        check (r.ok && r.clicksFound == 8 && std::abs (r.roundTripSamples - 437.0) <= 1.0, "eight clicks heard 437 samples late: round trip 437");
        r = daw::latency::findRoundTrip (heardAt (437, 5), testRate);
        check (r.ok && r.clicksFound == 5, "clicks found in 5 of 8 are accepted");
        r = daw::latency::findRoundTrip (heardAt (437, 4), testRate);
        check (! r.ok && r.clicksFound == 4, "clicks found in 4 of 8 are a failure");
        r = daw::latency::findRoundTrip (std::vector<float> ((size_t) total, 0.0f), testRate);
        check (! r.ok && r.clicksFound == 0, "silence is a failure");
        auto quiet = heardAt (437, 8);

        for (auto& v : quiet)
            v *= 0.002f;   // peaks at about -66 dB: under the -54 dB floor

        check (! daw::latency::findRoundTrip (quiet, testRate).ok, "clicks quieter than -54 dB are not heard");
        auto noisy = heardAt (437, 8);

        for (size_t i = 0; i < noisy.size(); ++i)
            noisy[i] += 0.0005f * (float) std::sin ((double) i * 0.37);

        r = daw::latency::findRoundTrip (noisy, testRate);
        check (r.ok && std::abs (r.roundTripSamples - 437.0) <= 1.0, "a little steady noise doesn't move the result");
    }

    // The engine routine: 437 samples through a loop-back, reporting 200 in and 200 out.
    {
        RecordingRig rig (1, 200, 200);
        bool started = false;
        const auto m = measureWithEcho (rig, 437, all, 0.8f, &started);
        check (started, "measuring starts on a device with inputs");
        check (m.ok && std::abs (m.roundTripSamples - 437.0) <= 1.0, "measured round trip 437 samples");
        check (std::abs (m.reportedSamples - 400.0) < 0.5 && std::abs (m.extraSamples - 37.0) <= 1.0, "extra = 437 - (200 + 200) = 37 samples");
        check (rig.engine.hasMeasuredLatency() && std::abs (rig.engine.getMeasuredLatencyExtraSamples() - 37) <= 1,
               "the extra is kept by the engine");
        check (std::abs (rig.engine.getLatencyShiftSamples() - 437) <= 1, "takes are now moved by the whole round trip");
        check (! rig.engine.isMeasuringLatency(), "measuring is over after the result");
    }

    {
        RecordingRig rig (1, 200, 200);
        // (A settings file left by an earlier run may already hold a value for the machine's device: only a change counts.)
        const bool measuredBefore = rig.engine.hasMeasuredLatency();
        const int extraBefore = rig.engine.getMeasuredLatencyExtraSamples();
        const auto m = measureWithEcho (rig, 437, {});
        check (! m.ok && m.problem.contains ("couldn't hear the clicks"), "no echo: the routine reports failure with advice");
        check (rig.engine.hasMeasuredLatency() == measuredBefore && rig.engine.getMeasuredLatencyExtraSamples() == extraBefore, "a failure changes nothing");
    }

    {
        RecordingRig rig (1, 200, 200);
        check (measureWithEcho (rig, 437, { 0, 2, 3, 5, 7 }).ok, "5 of 8 clicks echoed: accepted");
    }

    {
        RecordingRig rig (1, 200, 200);
        check (! measureWithEcho (rig, 437, { 1, 2, 6, 7 }).ok, "4 of 8 clicks echoed: failure");
    }

    {
        RecordingRig rig (0);
        rig.feed (0.02, nullptr, false);
        check (! rig.engine.startLatencyMeasurement(), "measuring needs an input");
    }

    {
        RecordingRig rig (1, 200, 200);
        rig.feed (0.02, nullptr, false);
        check (rig.engine.startLatencyMeasurement() && ! rig.engine.startLatencyMeasurement(), "a second start while measuring is refused");
        rig.engine.cancelLatencyMeasurement();
        check (! rig.engine.isMeasuringLatency() && ! rig.engine.serviceLatencyMeasurement(), "cancelling gives no result");
    }
}

static void runRecordingGuideTests()
{
    std::cout << "Recording guide" << std::endl;
    namespace rg = daw::recordingguide;

    auto near = [] (double a, double b, double tolerance) { return std::abs (a - b) <= tolerance; };

    // The numbers of the plan's table.
    check (near (rg::getBufferMs (64, 48000.0), 1.333, 0.001) && near (rg::getBufferMs (128, 48000.0), 2.667, 0.001)
               && near (rg::getBufferMs (256, 48000.0), 5.333, 0.001) && near (rg::getBufferMs (512, 44100.0), 11.610, 0.001)
               && near (rg::getBufferMs (1024, 48000.0), 21.333, 0.001),
           "buffer lengths: 1.333, 2.667, 5.333, 11.610 and 21.333 ms");
    check (near (rg::getMinimumRoundTripMs (128, 48000.0), 5.333, 0.001), "the least round trip at 128 samples and 48 kHz is 5.333 ms");
    check (near (rg::getDistanceMetres (rg::getMinimumRoundTripMs (128, 48000.0)), 1.83, 0.01), "5.333 ms of sound is 1.83 m");
    check (near (rg::getBitRangeDb (24), 144.5, 0.1) && near (rg::getBitRangeDb (16), 96.3, 0.1), "24 bits give 144.5 dB, 16 bits 96.3 dB");
    check (near (rg::getVoltsFromDbu (4.0), 1.228, 0.001) && near (rg::getVoltsFromDbv (-10.0), 0.316, 0.001), "+4 dBu is 1.228 V, -10 dBV 0.316 V");
    check (near (rg::getGainDb (0.0025, rg::getVoltsFromDbu (4.0)), 53.8, 0.1), "from a microphone to line level is 53.8 dB of gain");
    check (near (rg::getBytesPerMinute (48000.0, 24, 1) / 1.0e6, 8.64, 0.01) && rg::describeBytes (rg::getBytesPerMinute (48000.0, 24, 1)) == "8.64 MB"
               && near (rg::getBytesPerMinute (48000.0, 24, 2) / 1.0e6, 17.28, 0.01),
           "a minute of 24-bit 48 kHz WAV: 8.64 MB mono, 17.28 MB stereo");

    // Example data.
    {
        const auto table = rg::getBufferTable (48000.0, 1.0);
        check (table.size() == 5 && table[1].samples == 128 && near (table[1].roundTripMs, 5.333 + 1.0, 0.001), "the buffer table: 5 sizes, round trip = 2 buffers + extra");
        const auto levels = rg::getSignalLevels();
        check (levels.size() == 4 && ! levels[1].hasValue && near (levels[0].volts, 0.0025, 1.0e-9) && near (levels[3].volts, 1.228, 0.001),
               "the signal levels run from the microphone to studio line, the instrument without a figure");

        const auto examples = rg::getLevelExamples();
        check (examples.size() == 3 && near (examples[0].amplitude, std::pow (10.0, -36.0 / 20.0), 0.0001) && near (examples[1].amplitude, std::pow (10.0, -12.0 / 20.0), 0.0005),
               "the level examples' amplitudes match -36 and -12 dB");

        auto peakOf = [] (const std::vector<float>& wave) { float p = 0.0f; for (auto v : wave) p = juce::jmax (p, std::abs (v)); return p; };
        const auto quiet = rg::getPhrase (examples[0], 400), good = rg::getPhrase (examples[1], 400), clipped = rg::getPhrase (examples[2], 400);
        check (near (peakOf (quiet), examples[0].amplitude, examples[0].amplitude * 0.05) && near (peakOf (good), examples[1].amplitude, examples[1].amplitude * 0.05),
               "the phrase peaks where its example says");
        int flat = 0;

        for (auto v : clipped)
            flat += std::abs (v) >= 1.0f ? 1 : 0;

        check (peakOf (clipped) == 1.0f && flat >= 10, "the clipped phrase is cut flat at 1");

        const auto takes = rg::getTakesExample();
        check (takes.size() == 4 && takes[0].startBar == 10.0 && takes[3].cutShort && takes[3].endBar == 11.0 && takes[2].playing
                   && std::count_if (takes.begin(), takes.end(), [] (auto& t) { return t.playing; }) == 1,
               "the takes picture: pass 1 from bar 10, pass 4 cut at bar 11, pass 3 playing");
    }

    // Texts.
    for (auto topic : rg::allTopics)
    {
        check (rg::getTopicName (topic).isNotEmpty() && rg::getTopicSummary (topic).isNotEmpty() && rg::explain (topic).length() > 100,
               "recording guide topic has a name, summary and text: " + rg::getTopicName (topic));
        check (rg::explain (topic).containsNonWhitespaceChars() && ! rg::explain (topic).contains ("  "), "no double spaces in " + rg::getTopicName (topic));
    }

    check (rg::explain (rg::Topic::level).contains ("-18 to -10 dB") && rg::explain (rg::Topic::level).contains ("-6 dB"), "the level topic says -18 to -10 dB and under -6");
    check (rg::explain (rg::Topic::buffers).contains ("5.3 ms") && rg::explain (rg::Topic::buffers).contains ("1.8 m")
               && rg::explain (rg::Topic::buffers).contains ("11.6 ms") && rg::explain (rg::Topic::buffers).contains ("21.3 ms"),
           "the buffer topic's numbers match the formulas");
    check (rg::explain (rg::Topic::air).contains ("8.6 MB"), "the first topic's file size matches the formula");

    // The session.
    {
        rg::Snapshot s;
        s.inputDevice = s.outputDevice = "Scarlett 2i2 USB";
        s.sampleRate = 48000.0;
        s.bufferSamples = 128;
        s.numInputs = 2;
        s.reportedInputMs = 3.1;
        s.reportedOutputMs = 3.3;
        s.armed.push_back ({ "Vocals", 0, false, false, -20.0f });
        s.hasLastTake = true;
        s.lastTakeName = "Vocals take 3";
        s.lastTakePeakDb = -12.4;

        check (rg::describeSession (s)
                   == "Input: Scarlett 2i2 USB, 2 inputs. Output: Scarlett 2i2 USB. 48 kHz, buffer 128 samples (2.7 ms).\n"
                      "Round trip 6.4 ms: 3.1 ms in, 3.3 ms out, as the device reports.\n"
                      "Armed: Vocals on Input 1 (mono), Monitor off.\n"
                      "Last take: Vocals take 3, loudest peak -12.4 dB.",
               "the session text matches the plan's example");
        check (rg::describeLatency (s) == "Round trip 6.4 ms: 3.1 in, 3.3 out, reported", "the settings line before measuring");
        check (rg::describeInput (0, true) == "Inputs 1+2 (stereo)" && rg::describeInput (1, false) == "Input 2 (mono)", "input names");

        auto texts = [] (const rg::Snapshot& snap)
        {
            juce::StringArray out;

            for (auto& a : rg::analyse (snap))
                out.add (a.text);

            return out;
        };
        auto says = [&] (const rg::Snapshot& snap, const juce::String& part) { return texts (snap).joinIntoString ("|").contains (part); };

        // Not measured: the one note.
        check (says (s, "Takes are timed by the latency your device reports (6.4 ms)") && says (s, "a good level") && ! says (s, "Measuring found"),
               "an unmeasured device with a good take: the measuring note and the level");

        auto withPeak = [&] (double peak) { auto c = s; c.lastTakePeakDb = peak; return c; };
        check (says (withPeak (-0.1), "reached 0 dB") && ! says (withPeak (-0.11), "reached 0 dB") && says (withPeak (-0.11), "close to the top"), "peak rule at -0.1 dB");
        check (says (withPeak (-6.0), "close to the top") && says (withPeak (-6.01), "a good level") && ! says (withPeak (-6.01), "close to the top"), "peak rule at -6 dB");
        check (says (withPeak (-24.0), "a good level") && says (withPeak (-24.01), "quiet but not lost") && says (withPeak (-24.01), "-24.0 dB"), "peak rule at -24 dB");
        check (says (withPeak (-60.0), "quiet but not lost") && ! says (withPeak (-60.01), "Your last take"), "peak rule at -60 dB");
        auto noTake = s;
        noTake.hasLastTake = false;
        check (! says (noTake, "Your last take"), "no take, no level advice");

        auto monitored = s;
        monitored.armed[0].monitor = true;
        monitored.reportedInputMs = 7.0;
        monitored.reportedOutputMs = 7.2;
        check (says (monitored, "You hear yourself 14.2 ms late through DawIt"), "monitor rule at a slow round trip, with the figure");
        monitored.reportedInputMs = 5.0;
        monitored.reportedOutputMs = 7.0;
        check (says (monitored, "You hear yourself 12.0 ms late"), "monitor rule starts at 12 ms");
        monitored.reportedOutputMs = 6.9;
        check (! says (monitored, "You hear yourself"), "monitor rule quiet at 11.9 ms");
        auto slowButOff = s;
        slowButOff.reportedInputMs = 9.0;
        check (! says (slowButOff, "You hear yourself"), "monitor rule quiet when Monitor is off");

        auto big = s;
        big.bufferSamples = 512;
        check (says (big, "A buffer of 512 samples suits mixing"), "buffer rule at 512");
        big.bufferSamples = 511;
        check (! says (big, "suits mixing"), "buffer rule quiet at 511");
        big.bufferSamples = 1024;
        big.armed.clear();
        check (! says (big, "suits mixing"), "buffer rule quiet with nothing armed");

        auto silent = s;
        silent.inputAllZero = true;
        check (says (silent, "allow DawIt in System Settings") && silent.armed.size() == 1, "zero input while armed: the permission message");
        silent.armed.clear();
        check (! says (silent, "System Settings"), "zero input rule quiet when nothing is armed");

        auto noInput = s;
        noInput.numInputs = 0;
        check (says (noInput, "can't find an audio input") && ! says (s, "can't find an audio input"), "no input channels: the advice; quiet with inputs");
        auto noDevice = s;
        noDevice.inputDevice = {};
        check (! says (noDevice, "Takes are timed by"), "the measuring note is quiet without an input device");

        auto measured = s;
        measured.measured = true;
        measured.measuredExtraMs = 1.5;
        check (says (measured, "Measuring found 1.5 ms more than the device reports") && ! says (measured, "Takes are timed by"), "measured extra of 1.5 ms is mentioned");
        check (near (rg::getRoundTripMs (measured), 7.9, 0.001)
                   && rg::describeLatency (measured) == "Round trip 7.9 ms: 3.1 in, 3.3 out, measured, 1.5 ms more than reported",
               "the settings line after measuring");
        measured.measuredExtraMs = -1.0;
        check (says (measured, "1.0 ms less than the device reports"), "a negative extra of 1 ms is mentioned, as less");
        measured.measuredExtraMs = 0.99;
        check (! says (measured, "Measuring found"), "a measured extra under 1 ms is not mentioned");
    }
}

//==============================================================================
namespace
{
/** Feeds a made-up signal to a tuner listener in chunks, as the window's timer does. */
void feedListener (daw::tuner::Listener& listener, const std::vector<float>& signal, int chunk = 1600)
{
    for (size_t i = 0; i < signal.size(); i += (size_t) chunk)
        listener.feed (signal.data() + i, (int) juce::jmin ((size_t) chunk, signal.size() - i));
}

std::vector<float> makeSines (std::initializer_list<double> hzs, double rate, int n, double amplitude)
{
    std::vector<float> x ((size_t) n, 0.0f);

    for (const double hz : hzs)
        for (int i = 0; i < n; ++i)
            x[(size_t) i] += (float) (amplitude * std::sin (juce::MathConstants<double>::twoPi * hz * i / rate));

    return x;
}

double centsOf (double midi, double hz) { return (midi - daw::pitch::midiFromHz (hz)) * 100.0; }
}

static void runTunerTests()
{
    using namespace daw;
    std::cout << "Tuner: tracker, listener, input tap and mute" << std::endl;
    constexpr double rate = 48000.0;

    check (pitch::getTunerWindowSize (44100.0) == 4096 && pitch::getTunerWindowSize (48000.0) == 4096
               && pitch::getTunerWindowSize (88200.0) == 8192 && pitch::getTunerWindowSize (96000.0) == 8192,
           "the tuner's window is 4096 samples at 44.1 and 48 kHz and 8192 at 88.2 and 96 kHz (85 ms or more)");
    check (pitch::getWindowSize (48000.0) == 2048, "DawIt Tune's window is still 2048 at 48 kHz");

    // The tracker with the long window finds a bass's low notes; the default doesn't look below 50 Hz.
    for (const double hz : { 41.2034, 30.8677, 82.4069 })
    {
        pitch::Tracker tracker;
        const int w = pitch::getTunerWindowSize (rate);
        tracker.prepare (rate, w, 30.0);
        const auto x = makeSines ({ hz }, rate, w, 0.5);
        const auto r = tracker.analyse (x.data(), w);
        check (r.voiced && std::abs (centsOf (r.midi, hz)) < 2.0, "the long window finds " + juce::String (hz, 2) + " Hz within 2 cents");
    }

    {
        pitch::Tracker tracker;
        tracker.prepare (rate, 2048);
        const auto x = makeSines ({ 41.2034 }, rate, 2048, 0.5);
        check (! tracker.analyse (x.data(), 2048).voiced, "DawIt Tune's tracker still doesn't look below 50 Hz");
    }

    // The listener.
    {
        tuner::Listener listener;
        listener.prepare (rate);
        feedListener (listener, makeSines ({ 110.0 }, rate, 24000, 0.4));
        const auto& r = listener.getReading();
        check (r.hasNote && std::abs (centsOf (r.midi, 110.0)) < 1.0 && std::abs (r.midi - 45.0) < 0.05, "a 110 Hz sine reads A2, 0 cents, through the listener");
        check (! r.tooQuiet && r.levelDb > -15.0 && r.levelDb < -10.0, "its level is about -11 dB");
    }

    {
        tuner::Listener listener;
        listener.prepare (rate);
        feedListener (listener, makeSines ({ 440.0 }, rate, 24000, 0.4));
        check (std::abs (listener.getReading().midi - 69.0) < 0.01, "440 Hz reads A4");
    }

    for (const double hz : { 41.2034, 30.8677 })
    {
        tuner::Listener listener;
        listener.prepare (rate);
        feedListener (listener, makeSines ({ hz }, rate, 48000, 0.4));
        check (listener.getReading().hasNote && std::abs (centsOf (listener.getReading().midi, hz)) < 2.0,
               "the listener reads " + juce::String (hz, 2) + " Hz within 2 cents");
    }

    {
        // A plucked string: harmonics 1 to 8 at 1/n, decaying over 1.5 s, read after 150 ms.
        const double f0 = 82.4069;
        std::vector<float> pluck (7200);

        for (size_t i = 0; i < pluck.size(); ++i)
        {
            double v = 0.0;

            for (int k = 1; k <= 8; ++k)
                v += std::sin (juce::MathConstants<double>::twoPi * k * f0 * (double) i / rate) / k;

            pluck[i] = (float) (0.3 * v * std::exp (-(double) i / rate / 1.5));
        }

        tuner::Listener listener;
        listener.prepare (rate);
        feedListener (listener, pluck, 480);
        check (listener.getReading().hasNote && std::abs (centsOf (listener.getReading().midi, f0)) < 2.0, "a plucked E2 is read after 150 ms (within 2 cents)");
    }

    {
        juce::Random random (5);
        std::vector<float> noise (48000);

        for (auto& v : noise)
            v = (random.nextFloat() * 2.0f - 1.0f) * 0.173f;   // -20 dB rms

        const std::vector<std::pair<const char*, std::vector<float>>> cases {
            { "silence", std::vector<float> (48000, 0.0f) },
            { "white noise at -20 dB", noise },
            { "E2 and F2 together", makeSines ({ 82.4069, 87.3071 }, rate, 48000, 0.2) },
            { "E2 and A#2 together (a tritone)", makeSines ({ 82.4069, 116.541 }, rate, 48000, 0.2) } };

        for (const auto& [name, signal] : cases)
        {
            tuner::Listener listener;
            listener.prepare (rate);
            bool everHeard = false;

            for (size_t i = 0; i < signal.size(); i += 1600)
            {
                listener.feed (signal.data() + i, 1600);
                everHeard = everHeard || listener.getReading().hasNote;
            }

            // A semitone apart, two notes beat slowly enough for one window to look like a note: the first moments can
            // show one, but once the beating is seen it must go.
            const bool beating = juce::String (name).startsWith ("E2 and F2");
            check (beating ? ! listener.getReading().hasNote : ! everHeard, juce::String ("no note in ") + name);
        }
    }

    {
        // Status: a second of exact zeros, and a quiet signal.
        tuner::Listener listener;
        listener.prepare (rate);
        feedListener (listener, std::vector<float> (24000, 0.0f));
        check (! listener.getReading().exactlyZero, "half a second of zeros isn't reported");
        feedListener (listener, std::vector<float> (30000, 0.0f));
        check (listener.getReading().exactlyZero && listener.getReading().tooQuiet, "a second of exact zeros is reported, and is too quiet");
        feedListener (listener, makeSines ({ 110.0 }, rate, 24000, 0.001));
        check (! listener.getReading().exactlyZero && listener.getReading().tooQuiet && ! listener.getReading().hasNote, "a sine at -60 dB is too quiet for a note");
    }

    {
        // Hold: the note stays 2 s after the sound ends, fading, then goes.
        tuner::Listener listener;
        listener.prepare (rate);
        feedListener (listener, makeSines ({ 110.0 }, rate, 24000, 0.4));
        feedListener (listener, std::vector<float> (24000, 0.0f));
        check (listener.getReading().hasNote && listener.getReading().ageSeconds > 0.3 && listener.getReading().fade > 0.99f, "half a second after the sound the note is held");
        feedListener (listener, std::vector<float> (48000, 0.001f));
        check (listener.getReading().hasNote && listener.getReading().fade < 0.5f, "1.5 s after, it is fading");
        feedListener (listener, std::vector<float> (24000 + 14400, 0.001f));
        check (! listener.getReading().hasNote, "after 2 s it is gone");
    }

    {
        // A single slip never moves the needle: 4 steady readings, one of an octave up, and the median stays.
        tuner::Listener listener;
        listener.prepare (rate);
        const auto steady = makeSines ({ 110.0 }, rate, 48000, 0.4);
        feedListener (listener, steady, 1024);
        const double before = listener.getReading().midi;
        const auto slip = makeSines ({ 220.0 }, rate, 4096, 0.4);
        feedListener (listener, slip, 1024);
        check (std::abs (listener.getReading().midi - before) < 0.1, "a short burst a different note doesn't move the median of 5");
    }

    // The input tap, through the fake device.
    {
        RecordingRig rig (2);
        auto& engine = rig.engine;
        check (! engine.areInputsWanted(), "the inputs aren't wanted before the tuner listens");
        engine.setTunerInput (1, false);
        engine.setTunerListening (true);
        check (engine.areInputsWanted(), "the inputs are wanted while the tuner listens");

        tuner::Listener listener;
        listener.prepare (testRate);
        rig.feed (0.6, [] (int ch, juce::int64 n) { return ch == 1 ? sineAt (n, 110.0, 0.4f) : sineAt (n, 440.0, 0.4f); });
        check (listener.service (engine.getTunerTap()) >= 28000, "the listener drains what the audio thread copied");
        check (listener.getReading().hasNote && std::abs (listener.getReading().midi - 45.0) < 0.05, "an input sine at 110 Hz reads A2 on the chosen channel (Input 2)");

        engine.setTunerInput (0, true);
        listener.reset();
        rig.feed (0.5, [] (int, juce::int64 n) { return sineAt (n, 440.0, 0.4f); });
        listener.service (engine.getTunerTap());
        check (std::abs (listener.getReading().midi - 69.0) < 0.05, "a stereo pair is summed to mono");

        engine.setTunerListening (false);
        check (! engine.areInputsWanted(), "the inputs aren't wanted once it stops");
        rig.feed (0.2, [] (int, juce::int64 n) { return sineAt (n, 110.0, 0.4f); });
        float scrap[16];
        check (engine.getTunerTap().pull (scrap, 16) == 0, "nothing is copied while it isn't listening");
    }

    // Mute while tuning.
    {
        RecordingRig rig (2);
        auto& engine = rig.engine;
        engine.play();
        engine.setArmed (*rig.track, true);
        engine.setMonitor (*rig.track, true);
        const auto tone = [] (int, juce::int64 n) { return sineAt (n, 440.0, 0.5f); };

        auto out = rig.feed (0.3, tone);
        check (rms (out, 0, 4800) > 0.05f, "a monitored armed track is heard without the tuner");

        engine.setTunerMuteMonitoring (true);
        out = rig.feed (0.3, tone);
        check (rms (out, 0, 4800) > 0.05f, "the mute switch does nothing while the tuner isn't listening");

        engine.setTunerListening (true);
        out = rig.feed (0.3, tone);
        check (rms (out, 0, 4800) < 1e-5f, "with the tuner open and mute on, monitoring is silent");

        engine.setTunerMuteMonitoring (false);
        out = rig.feed (0.3, tone);
        check (rms (out, 0, 4800) > 0.05f, "with mute off it is heard again");

        engine.setTunerMuteMonitoring (true);
        engine.setPositionBeats (0.0);
        check (engine.startRecording(), "recording starts with the tuner open");
        rig.feed (1.0, tone);
        engine.stopRecording();
        const auto clips = audioClipsOf (*rig.track);
        float peak = 0.0f;

        if (clips.size() == 1)
            peak = clips[0]->audio.getMagnitude (0, clips[0]->audio.getNumSamples());

        check (clips.size() == 1 && peak > 0.45f, "recording still captures the input while monitoring is muted");
        engine.setTunerListening (false);
    }
}

static void runTuningGuideTests()
{
    using namespace daw::gen::tuning;
    std::cout << "Tuning guide: tunings, readings, words, rules and text" << std::endl;

    const auto standard = getStrings (Tuning::guitarStandard);
    const int openStrings[] { 40, 45, 50, 55, 59, 64 };   // ChordStyles.cpp
    bool same = standard.size() == 6;

    for (size_t i = 0; same && i < 6; ++i)
        same = standard[i].midi == openStrings[i];

    check (same, "the standard tuning is the chord shapes' open strings: 40 45 50 55 59 64");

    const double expectedHz[] { 82.4069, 110.000, 146.832, 195.998, 246.942, 329.628 };

    for (size_t i = 0; i < 6; ++i)
        check (std::abs (getHz (standard[i].midi) - expectedHz[i]) < 0.001, standard[i].note + " is " + juce::String (expectedHz[i], 3) + " Hz");

    check (std::abs (getHz (getStrings (Tuning::bass4)[0].midi) - 41.2034) < 0.001 && std::abs (getHz (getStrings (Tuning::bass5)[0].midi) - 30.8677) < 0.001
               && std::abs (getHz (getStrings (Tuning::ukulele)[0].midi) - 391.995) < 0.001,
           "bass E1 and B0 and the ukulele's G4 have their frequencies");
    check (getStrings (Tuning::chromatic).empty() && getStrings (Tuning::bass4).size() == 4 && getStrings (Tuning::bass5).size() == 5
               && getStrings (Tuning::ukulele).size() == 4, "the strings per tuning");
    check (standard[0].label == "low E" && standard[5].label == "high E" && standard[1].label == "A" && getStrings (Tuning::dropD)[0].label == "low D"
               && getStrings (Tuning::dropD)[2].label == "high D" && getStrings (Tuning::dadgad)[2].label == "D3" && getStrings (Tuning::dadgad)[0].label == "low D"
               && getStrings (Tuning::dadgad)[5].label == "high D",
           "a repeated note is named low, high or by its octave");
    check (getStrings (Tuning::halfStepDown)[2].note == "Db3" && getStrings (Tuning::dadgad)[4].note == "A3", "the other tunings have their notes");
    check (getTuningFromId (getTuningId (Tuning::dadgad)) == Tuning::dadgad && getTuningFromId ("nonsense") == Tuning::guitarStandard, "a tuning round trips through its saved id");

    // Readings.
    {
        const auto r = getReading (110.0, 440.0, Tuning::guitarStandard);
        check (r.note == "A2" && std::abs (r.cents) < 1.0 && r.stringIndex == 1 && r.zone == Zone::inTune && r.text == "A string: in tune", "110 Hz is the A string, in tune");
    }

    {
        const auto r = getReading (81.4604, 440.0, Tuning::guitarStandard);
        check (r.stringIndex == 0 && std::abs (r.cents + 20.0) < 1.0 && r.stringName == "low E string" && r.words.contains ("flat, tighten it") && ! r.words.contains ("a little")
                   && r.zone == Zone::out, "81.46 Hz is the low E string, 20 cents flat: flat, tighten it");
    }

    {
        const auto r = getReading (41.2, 440.0, Tuning::bass4);
        check (r.note == "E1" && r.stringIndex == 0 && std::abs (r.cents) < 2.0, "41.2 Hz on the bass is E1, the E string");
        const auto r5 = getReading (30.87, 440.0, Tuning::bass5);
        check (r5.note == "B0" && r5.stringIndex == 0, "30.87 Hz on the five-string bass is B0");
    }

    {
        const auto r = getReading (440.0, 442.0, Tuning::chromatic);
        check (r.note == "A4" && std::abs (r.cents + 7.85) < 0.05 && r.stringIndex == -1 && r.text == r.words, "440 Hz against a reference of 442 is A4, 7.85 cents flat");
        check (getReading (109.2390, 440.0, Tuning::guitarStandard).text == "A string: 12 cents flat, tighten it a little", "the guide's example reads as the plan says");
        check (getReading (112.0, 440.0, Tuning::guitarStandard).words == "31 cents sharp, loosen it", "past 15 cents there is no 'a little'");
    }

    {
        const int a = 1;   // the A string
        const auto locked = getReading (98.0, 440.0, Tuning::guitarStandard, a);
        check (locked.locked && locked.stringIndex == a && locked.words == "2 semitones low, tighten it" && locked.text == "A string: 2 semitones low, tighten it",
               "locked to the A string, 98 Hz is 2 semitones low");
        check (describeCents (100.0) == "1 semitone high, loosen it" && describeCents (1.0 + 3.0) == "4 cents sharp, loosen it a little" && describeCentsShort (-4.4) == "4 cents flat"
                   && describeCentsShort (-4.1) == "4 cents flat", "the words use 'semitone' and 'cents' correctly");
        check (describeCentsShort (-3.4).contains ("3 cents") && describeCents (-2.9) == "in tune", "within 3 cents is in tune");
        check (describeCents (-15.0) == "15 cents flat, tighten it a little" && describeCents (-16.0) == "16 cents flat, tighten it" && describeCents (-50.0) == "50 cents flat, tighten it",
               "up to 15 cents 'a little', up to 50 without");
        check (getReading (110.0 * std::pow (2.0, 1.0 / 1200.0), 440.0, Tuning::chromatic).words == "in tune", "1 cent is in tune");
    }

    check (getZone (2.9) == Zone::inTune && getZone (-3.1) == Zone::close && getZone (9.9) == Zone::close && getZone (10.1) == Zone::out, "zones: 2.9 in tune, 3.1 and 9.9 close, 10.1 out");
    check (getNearestString (44.5, Tuning::guitarStandard) == 1 && getNearestString (42.5, Tuning::guitarStandard) == 0 && getNearestString (32.0, Tuning::bass4) == 1
               && getNearestString (60.0, Tuning::chromatic) == -1, "the nearest string, the lower on a tie");

    // Formulas.
    {
        const auto errors = getHarmonicTuningErrors();
        check (errors.size() == 4 && std::abs (errors[0] + 1.955) < 0.01 && std::abs (errors[1] + 3.910) < 0.01 && std::abs (errors[2] + 5.865) < 0.01 && std::abs (errors[3] + 19.55) < 0.01,
               "matching harmonics leaves 1.955 cents flat a step, and the B 19.55 flat in all");
        check (std::abs (getCentsForTensionPercent (1.0) - 8.61) < 0.01, "1% more tension is 8.61 cents");
        check (std::abs (getBeatsPerSecond (440.0, 442.0) - 2.0) < 0.001 && std::abs (getHzForCents (110.0, 5.0) - 0.318) < 0.001
                   && std::abs (getHzForCents (440.0, 1.0) - 0.254) < 0.001 && std::abs (getHzForCents (82.4069, -10.0) + 0.47) < 0.005
                   && std::abs (getHzForCents (329.628, -10.0) + 1.90) < 0.005,
               "beats: 2 a second for 440 against 442; 5 cents at 110 Hz is 0.318 Hz");
        check (std::abs (getReferenceCents (432.0) + 31.77) < 0.01 && std::abs (getReferenceCents (442.0) - 7.85) < 0.01 && std::abs (getReferenceCents (415.3) + 100.0) < 0.1,
               "432 Hz is 31.77 cents below 440, 442 Hz is 7.85 above");
        check (std::abs (getCyclesInWindow (41.2, 4096 / 48.0) - 3.52) < 0.01 && std::abs (getCyclesInWindow (329.63, 4096 / 48.0) - 28.13) < 0.01,
               "the 85.3 ms window holds 3.5 vibrations of E1 and 28 of E4");
        check (std::abs (getBeatEnvelope (0.0) - 2.0) < 1e-9 && std::abs (getBeatEnvelope (0.5) - 2.0) < 1e-9 && getBeatEnvelope (0.25) < 1e-9 && getBeatEnvelope (1.25) < 1e-9,
               "the beat envelope swells twice a second");
        check (std::abs (getTuningPath (0.0) - 30.0) < 1e-9 && std::abs (getTuningPath (2.5) + 20.0) < 1e-9 && std::abs (getTuningPath (5.0)) < 1e-9 && std::abs (getTuningPath (2.0) - 5.0) < 1e-9, "the tuning path");
        check (std::abs (getDriftGlide (0.0) - 6.0) < 1e-9 && getDriftGlide (2.0) < 0.01, "the drift glide starts 6 cents sharp");
        check (getFifthFretSteps() == std::vector<int> { 5, 5, 5, 4, 5 } && getHarmonicHz (110.0) == std::vector<double> { 220.0, 330.0, 440.0 }, "the fifth-fret steps and the harmonics");
    }

    // The rules.
    {
        const daw::MusicalKey aMinor { 9, daw::MusicalKey::Scale::minor }, eFlat { 3, daw::MusicalKey::Scale::major };
        check (getKeyFit (Tuning::guitarStandard, aMinor).inKey.size() == 6 && getKeyFit (Tuning::guitarStandard, eFlat).inKey.size() == 2
                   && getBestCapoFret (Tuning::guitarStandard, eFlat) == 1 && getKeyFit (Tuning::guitarStandard, eFlat, 1).inKey.size() == 6,
               "key fit: 6 of 6 in A minor, 2 of 6 in Eb major, a capo on fret 1 makes 6");

        auto text = [] (const juce::StringArray& a) { return a.joinIntoString ("\n"); };
        const auto minor = text (analyse (Tuning::guitarStandard, 440.0, aMinor, {}, {}));
        check (minor.contains ("6 of the 6 open strings (E, A, D, G, B, E) are in A minor.") && minor.contains ("Open chords and ringing strings will fit.") && ! minor.contains ("capo")
                   && minor.contains ("Open the tuner (Cmd+7) and play each string once"),
               "in A minor all six strings fit, with no capo and the prompt to play each string");

        const auto flat = text (analyse (Tuning::guitarStandard, 440.0, eFlat, {}, {}));
        check (flat.contains ("2 of the 6 open strings (D, G) are in Eb major.") && flat.contains ("E, A, B and E aren't: let them ring only when you mean to.")
                   && flat.contains ("Try a capo on fret 1 and play the shapes of D major: the open strings become F, Bb, Eb, Ab, C, F, 6 of them in Eb major."),
               "in Eb major the advice suggests a capo on fret 1 and the shapes of D major");
        check (! text (analyse (Tuning::bass4, 440.0, eFlat, {}, {})).contains ("capo") && analyse (Tuning::chromatic, 440.0, eFlat, {}, {}).isEmpty(), "no capo for a bass, and nothing for Chromatic at 440");

        const auto low = text (analyse (Tuning::bass4, 440.0, aMinor, { { "Bass", 26 } }, {}));
        check (low.contains ("The Bass track goes down to D1 (36.7 Hz)") && low.contains ("E string") && low.contains ("2 semitones") && low.contains ("five-string bass"),
               "a Bass track down to D1 on a four-string bass: the advice names D1 and the E string");
        check (! text (analyse (Tuning::bass4, 440.0, aMinor, { { "Bass", 28 }, { "Piano", 20 } }, {})).contains ("goes down to"), "nothing lower than E1, and a piano isn't a bass: no advice");
        check (! text (analyse (Tuning::bass5, 440.0, aMinor, { { "Bass", 26 } }, {})).contains ("goes down to"), "a five-string bass reaches D1");

        const auto reference = text (analyse (Tuning::guitarStandard, 442.0, aMinor, {}, {}));
        check (reference.contains ("Your reference is A4 = 442 Hz, 7.9 cents above DawIt's instruments") && reference.contains ("sound sharp against the song"), "a 442 Hz reference is 7.9 cents above, and sounds sharp");
        check (text (analyse (Tuning::guitarStandard, 432.0, aMinor, {}, {})).contains ("31.8 cents below") && ! text (analyse (Tuning::guitarStandard, 440.0, aMinor, {}, {})).contains ("reference"),
               "432 Hz is 31.8 cents below; no note at 440");

        const auto heard = text (analyse (Tuning::guitarStandard, 440.0, aMinor, {}, { { 1, 0.4 }, { 0, -2.4 } }));
        check (heard.contains ("Last heard: low E 2 cents flat, A in tune.") && ! heard.contains ("Open the tuner"), "the last readings, in string order");
    }

    // Text.
    {
        bool complete = true;

        for (const auto topic : allTopics)
            complete = complete && getTopicName (topic).isNotEmpty() && getTopicSummary (topic).isNotEmpty() && explain (topic).length() > 400 && getExampleCaption (topic).isNotEmpty();

        check (complete, "every topic has a name, summary, text and example");
        check (explain (Topic::whatItShows).contains ("green within 3 cents") && explain (Topic::reference).contains ("31.8 cents")
                   && explain (Topic::harmonics).contains (juce::String::fromUTF8 ("4 \xc3\x97 82.41")) && explain (Topic::bassAndUkulele).contains ("24.3 milliseconds"),
               "the texts carry the numbers of the plan");
    }
}

/** The registry of built-ins (cmake/DawItBuiltIns.cmake, written as DawItBuiltInList.h) and what a plugin build
    relies on: codes, kinds, parameter texts and ids, bus layouts and the state, for every registered built-in. */
static void runPluginRegistryTests()
{
    using namespace daw;
    namespace bl = daw::builtinlist;

    const auto entries = std::vector<bl::Entry> (std::begin (bl::entries), std::end (bl::entries));

    // Codes: one capital first, then three lower case letters, each plugin's own.
    bool codesOk = true;
    juce::StringArray codes;

    for (auto& e : entries)
    {
        const juce::String code (e.code);
        codesOk = codesOk && code.length() == 4 && juce::CharacterFunctions::isUpperCase (code[0])
                  && juce::CharacterFunctions::isLowerCase (code[1]) && juce::CharacterFunctions::isLowerCase (code[2])
                  && juce::CharacterFunctions::isLowerCase (code[3]);
        codes.add (code);
    }

    codes.removeDuplicates (false);
    check (codesOk && codes.size() == (int) entries.size(), "plugin registry: every code is a capital and three lower case letters, and no two are the same");
    check (juce::String (bl::manufacturerCode) == "Dwit", "plugin registry: the manufacturer code is Dwit");

    // The registry against the built-in format: every effect and instrument once, with the same kind.
    auto all = BuiltInFormat::getEffects();
    all.addArray (BuiltInFormat::getInstruments());
    bool matches = (int) entries.size() == all.size();

    for (auto& d : all)
    {
        const auto count = std::count_if (entries.begin(), entries.end(), [&d] (auto& e) { return d.fileOrIdentifier == e.identifier; });
        const auto found = std::find_if (entries.begin(), entries.end(), [&d] (auto& e) { return d.fileOrIdentifier == e.identifier; });
        matches = matches && count == 1 && found->isInstrument == d.isInstrument;
    }

    check (matches, "plugin registry: every built-in effect and instrument is registered once, with the kind the format lists it under");

    const auto appOnly = std::count_if (entries.begin(), entries.end(), [] (auto& e) { return e.appOnly; });
    const auto tune = std::find_if (entries.begin(), entries.end(), [] (auto& e) { return e.appOnly; });
    check (appOnly == 1 && juce::String (tune->identifier) == BuiltInTune::identifier, "plugin registry: DawIt Tune is the only app-only built-in");

    BuiltInFormat builtInFormat;
    juce::AudioPluginFormat& format = builtInFormat;   // the public, synchronous createPluginInstance

    for (auto& e : entries)
    {
        const juce::String name (e.identifier);
        juce::PluginDescription description;

        for (auto& d : all)
            if (d.fileOrIdentifier == name)
                description = d;

        juce::String error;
        auto a = format.createInstanceFromDescription (description, 44100.0, 512, error);
        auto b = format.createInstanceFromDescription (description, 44100.0, 512, error);
        check (a != nullptr && b != nullptr, name + " plugin registry: the format creates it " + error);

        if (a == nullptr || b == nullptr)
            continue;

        // Parameter ids and texts.
        juce::StringArray ids;
        bool idsOk = ! a->getParameters().isEmpty(), textOk = true, versionOk = true;
        juce::String badText;

        for (auto* parameter : a->getParameters())
        {
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter))
            {
                ids.add (withId->paramID);
                versionOk = versionOk && withId->getVersionHint() >= 1;
            }
            else
                idsOk = false;

            for (float v : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                const auto text = parameter->getText (v, 1024);
                const auto back = parameter->getText (parameter->getValueForText (text), 1024);

                if (text.isEmpty() || back != text)
                {
                    textOk = false;
                    badText = parameter->getName (64) + " at " + juce::String (v) + ": \"" + text + "\" came back as \"" + back + "\"";
                }
            }
        }

        const int idCount = ids.size();
        ids.removeDuplicates (false);
        check (idsOk && versionOk && ids.size() == idCount, name + " plugin registry: parameter ids are unique and have a version hint of at least 1");
        check (textOk, name + " plugin registry: parameter texts are not empty and come back from their own text " + badText);

        // Layouts.
        auto layout = [] (int in, int out)
        {
            juce::AudioProcessor::BusesLayout l;
            if (in > 0)
                l.inputBuses.add (juce::AudioChannelSet::canonicalChannelSet (in));
            l.outputBuses.add (juce::AudioChannelSet::canonicalChannelSet (out));
            return l;
        };

        if (e.isInstrument)
        {
            auto stereoOut = a->getBusesLayout();
            stereoOut.inputBuses.clear();
            stereoOut.outputBuses.set (0, juce::AudioChannelSet::stereo());
            check (a->getTotalNumOutputChannels() == 2 && a->getTotalNumInputChannels() == 0 && a->acceptsMidi(),
                   name + " plugin registry: an instrument has stereo out, no input and takes MIDI");
        }
        else
        {
            check (a->checkBusesLayoutSupported (layout (1, 1)) && a->checkBusesLayoutSupported (layout (2, 2)),
                   name + " plugin registry: an effect accepts mono to mono and stereo to stereo");
        }

        // State: a changed blob loads into a fresh instance.
        int n = 0;

        for (auto* parameter : a->getParameters())
        {
            const float v = n++ % 2 == 0 ? 0.37f : 0.81f;

            // Stepped parameters (switches, choices) snap to a legal value first, as a host's value would.
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                parameter->setValueNotifyingHost (ranged->convertTo0to1 (ranged->convertFrom0to1 (v)));
            else
                parameter->setValueNotifyingHost (v);
        }

        juce::MemoryBlock blob;
        a->getStateInformation (blob);
        b->setStateInformation (blob.getData(), (int) blob.getSize());
        bool same = a->getParameters().size() == b->getParameters().size();

        juce::String differs;

        for (int i = 0; same && i < a->getParameters().size(); ++i)
        {
            same = std::abs (a->getParameters()[i]->getValue() - b->getParameters()[i]->getValue()) <= 1.0e-6f;

            if (! same)
                differs = " (" + a->getParameters()[i]->getName (64) + ": " + juce::String (a->getParameters()[i]->getValue(), 6) + " against "
                          + juce::String (b->getParameters()[i]->getValue(), 6) + ")";
        }

        check (same && blob.getSize() > 0, name + " plugin registry: its state loads into a fresh instance with every parameter the same" + differs);
    }
}

/** DawIt's own VST3 and AU copies are hidden from the add menus; the built-ins and others' plugins are not. */
static void runDawItPluginCopyTests()
{
    using namespace daw;
    juce::PluginDescription vst3, au, builtIn, other;
    vst3.name = au.name = builtIn.name = "DawIt Reverb";
    vst3.pluginFormatName = "VST3";
    au.pluginFormatName = "AudioUnit";
    vst3.manufacturerName = au.manufacturerName = "DawIt";
    builtIn = BuiltInReverb::getDescription();
    other.name = "Reverb";
    other.pluginFormatName = "VST3";
    other.manufacturerName = "Someone Else";

    check (plugins::isDawItPluginCopy (vst3) && plugins::isDawItPluginCopy (au), "DawIt copies: a VST3 or AU by DawIt is one of DawIt's own copies");
    check (! plugins::isDawItPluginCopy (builtIn) && ! plugins::isDawItPluginCopy (other),
           "DawIt copies: the built-in and another maker's VST3 are not");
}

static void runNotationTests()
{
    using namespace daw;
    namespace nt = gen::notation;
    std::cout << "Notation" << std::endl;

    const MusicalKey cMajor { 0, MusicalKey::Scale::major };
    const MusicalKey dMajor { 2, MusicalKey::Scale::major };
    const MusicalKey aMinor { 9, MusicalKey::Scale::minor };
    const MusicalKey dMinor { 2, MusicalKey::Scale::minor };
    const MusicalKey fSharpMajor { 6, MusicalKey::Scale::major };
    const Chord dChord { 2, { 0, 4, 7 } };

    // Spelling (R4).
    auto spelled = [] (int midi, const MusicalKey& key, const Chord* chord)
    {
        const auto p = nt::spell (midi, key, chord);
        return juce::String::charToString ("CDEFGAB"[p.letter]) + juce::String (p.alter > 0 ? "#" : p.alter < 0 ? "b" : "") + juce::String (p.octave);
    };
    check (spelled (66, dMajor, nullptr) == "F#4" && spelled (68, aMinor, nullptr) == "G#4"
               && spelled (61, dMinor, nullptr) == "C#4" && spelled (70, dMinor, nullptr) == "Bb4",
           "Notation: notes in the key or the raised 6th and 7th of a minor key are spelled F#4, G#4, C#4, Bb4");
    check (spelled (70, cMajor, nullptr) == "Bb4" && spelled (66, cMajor, &dChord) == "F#4" && spelled (66, cMajor, nullptr) == "Gb4",
           "Notation: outside the key a chord tone follows the chord, else flats: Bb4, F#4 over D, Gb4");
    {
        const auto p = nt::spell (65, fSharpMajor, nullptr);
        check (p.letter == 2 && p.alter == 1 && p.octave == 4 && nt::getStaffStep (nt::Clef::treble, p) == 0 && p.getMidi() == 65,
               "Notation: 65 in F# major is E#4, on the treble staff's bottom line");
    }

    // Staff steps and ledger lines (R1, R2).
    auto stepOf = [&] (nt::Clef clef, int midi) { return nt::getStaffStep (clef, nt::spell (midi, cMajor, nullptr)); };
    check (stepOf (nt::Clef::treble, 64) == 0 && stepOf (nt::Clef::treble, 71) == 4 && stepOf (nt::Clef::treble, 77) == 8
               && stepOf (nt::Clef::treble, 60) == -2,
           "Notation: treble staff steps of E4, B4, F5, C4 are 0, 4, 8, -2");
    check (stepOf (nt::Clef::bass, 43) == 0 && stepOf (nt::Clef::bass, 50) == 4 && stepOf (nt::Clef::bass, 57) == 8
               && stepOf (nt::Clef::bass, 60) == 10,
           "Notation: bass staff steps of G2, D3, A3, C4 are 0, 4, 8, 10");
    check (nt::getLedgerLines (stepOf (nt::Clef::treble, 60)) == 1 && nt::getLedgerLines (stepOf (nt::Clef::treble, 57)) == 2
               && nt::getLedgerLines (stepOf (nt::Clef::treble, 53)) == 3 && nt::getLedgerLines (stepOf (nt::Clef::treble, 84)) == 2
               && nt::getLedgerLines (stepOf (nt::Clef::bass, 64)) == 2 && nt::getLedgerLines (stepOf (nt::Clef::bass, 40)) == 1
               && nt::getLedgerLines (stepOf (nt::Clef::bass, 28)) == 4 && nt::getLedgerLines (4) == 0,
           "Notation: ledger lines: treble C4 1, A3 2, F3 3, C6 2; bass E4 2, E2 1, E1 4");

    // Auto clef (R3).
    auto range = [] (int lo, int hi) { std::vector<int> v; for (int i = lo; i <= hi; ++i) v.push_back (i); return v; };
    check (nt::chooseAutoClef (range (57, 72)) == nt::ClefChoice::treble && nt::chooseAutoClef (range (36, 60)) == nt::ClefChoice::bass
               && nt::chooseAutoClef (range (48, 76)) == nt::ClefChoice::grand && nt::chooseAutoClef ({ 55, 60, 62 }) == nt::ClefChoice::treble
               && nt::chooseAutoClef ({ 53, 55, 57 }) == nt::ClefChoice::bass && nt::chooseAutoClef ({}) == nt::ClefChoice::treble,
           "Notation: Auto picks treble for 57-72, bass for 36-60, grand for 48-76, the median when both fit, treble for nothing");

    // Key signatures (R5).
    auto steps = [] (const MusicalKey& key, nt::Clef clef)
    {
        juce::String s;

        for (auto& m : nt::getKeySignature (gen::guide::getCirclePosition (key), clef))
            s << m.step << " ";

        return s.trim();
    };
    check (steps (dMajor, nt::Clef::treble) == "8 5" && steps ({ 10, MusicalKey::Scale::major }, nt::Clef::treble) == "4 7"
               && steps (dMajor, nt::Clef::bass) == "6 3" && steps (cMajor, nt::Clef::treble).isEmpty(),
           "Notation: D major's sharps are on steps 8 and 5 (treble), 6 and 3 (bass); B-flat major's flats on 4 and 7");

    // Rhythm helpers.
    auto build = [&] (std::vector<MidiNote> notes, nt::ClefChoice clef = nt::ClefChoice::treble, ChordMap chords = {}, MusicalKey key = {})
    {
        nt::Input in;
        in.notes = std::move (notes);
        in.clef = clef;
        in.chords = std::move (chords);
        in.key = key;
        return nt::build (in);
    };
    auto note = [] (int pitch, double start, double length) { return MidiNote { pitch, start, length, 0.8f }; };
    auto written = [] (const nt::Score& score, bool rests = false)
    {
        // "start:length" in units, ~ for a tie, r for a rest, bars separated by |.
        juce::String s;

        for (auto& bar : score.staves[0].bars)
        {
            for (auto& e : bar.events)
                if (rests || ! e.isRest)
                    s << e.start << ":" << e.length << (e.isRest ? "r " : e.tiedToNext ? "~ " : " ");

            s << "| ";
        }

        return s.trim();
    };

    // Splitting and ties (R9).
    check (written (build ({ note (67, 1, 2) })).startsWith ("12:12~ 24:12 |"), "Notation: beats 2 to 4 are a quarter tied to a quarter at beat 3");
    {
        const auto dotted = build ({ note (67, 0, 3) });
        const auto& e = dotted.staves[0].bars[0].events[0];
        check (e.value == nt::Value::half && e.dotted && e.length == 36 && ! e.tiedToNext, "Notation: beats 1 to 3 are one dotted half");
    }
    check (written (build ({ note (67, 0.5, 1) })).startsWith ("6:6 12:6 ") || written (build ({ note (67, 0.5, 1) })).startsWith ("6:6~ 12:6 |"),
           "Notation: an eighth-beat start held a beat is two eighths tied at beat 2");
    check (written (build ({ note (67, 3, 2) })) == "36:12~ | 0:12 |", "Notation: a note over the bar line is tied over it");
    check (written (build ({ note (67, 0, 1.75) })) == "0:18~ 18:3 |", "Notation: 1.75 beats is a dotted quarter tied to a sixteenth");
    check (written (build ({ note (67, 2, 2) })) == "24:24 |" && written (build ({ note (67, 0, 4) })) == "0:48 |",
           "Notation: a half note on beat 3 and a whole note stay whole");

    // Rests (R10).
    check (written (build ({ note (67, 0, 0.5) }), true) == "0:6~ 6:6r 12:12r 24:24r |" || written (build ({ note (67, 0, 0.5) }), true) == "0:6 6:6r 12:12r 24:24r |",
           "Notation: after an eighth note the bar is filled with an eighth rest, a quarter rest and a half rest");
    {
        const auto empty = build ({});
        const auto& e = empty.staves[0].bars[0].events;
        check (e.size() == 1 && e[0].isRest && e[0].length == 48, "Notation: an empty bar is one whole rest");
    }
    check (written (build ({ note (67, 0, 1), note (67, 2.5, 0.5) }), true) == "0:12 12:12r 24:6r 30:6 36:12r |",
           "Notation: rests between notes: a quarter, an eighth, then a quarter after the second note");

    // Triplets and the display grid (R7).
    {
        const auto third = 1.0 / 3.0;
        const auto s1 = build ({ note (67, 0, third), note (69, third, third), note (71, 2 * third, third) });
        const auto& bar = s1.staves[0].bars[0];
        check (bar.events[0].triplet && bar.events[0].length == 4 && bar.events[1].triplet && bar.events[2].triplet
                   && bar.triplets.size() == 1 && bar.triplets[0].first == 0 && bar.triplets[0].last == 2,
               "Notation: three thirds of a beat are three eighth triplets under one bracket");
        const auto s2 = build ({ note (67, 0, 2 * third), note (69, 2 * third, third) });
        const auto& b2 = s2.staves[0].bars[0];
        check (b2.events[0].triplet && b2.events[0].value == nt::Value::quarter && b2.events[0].length == 8
                   && b2.events[1].triplet && b2.events[1].value == nt::Value::eighth && b2.events[1].length == 4 && b2.triplets.size() == 1,
               "Notation: two thirds then one third is a quarter triplet and an eighth triplet");
        const auto s3 = build ({ note (67, 0, 0.25), note (69, 0.25, 0.25), note (71, 0.5, 0.25), note (72, 0.75, 0.25) });
        const auto& b3 = s3.staves[0].bars[0];
        check (! b3.events[0].triplet && b3.events[0].value == nt::Value::sixteenth && b3.triplets.empty() && b3.beams.size() == 1
                   && b3.beams[0].events.size() == 4,
               "Notation: four sixteenths stay sixteenths under one beam");
    }
    {
        const auto fit = nt::fitBeat ({ 0.03, 0.49, 0.52, 0.93, 0.98 });
        check (std::abs (fit.sixteenthError - 0.15) < 0.005 && std::abs (fit.tripletError - 0.42) < 0.005 && ! fit.triplets,
               "Notation: the played beat fits sixteenths (0.15) better than triplets (0.42)");

        const auto played = build ({ note (64, 0.03, 0.46), note (67, 0.52, 0.41), note (69, 0.98, 0.95), note (67, 2.04, 0.47), note (64, 2.49, 1.40) });
        check (written (played) == "0:6 6:6 12:12 24:6 30:6~ 36:12 |", "Notation: the played example is written eighth, eighth, quarter, eighth, eighth tied to a quarter");
        double total = 0.0;

        for (auto& e : played.staves[0].bars[0].events)
            total += e.getBeats();

        check (std::abs (total - 4.0) < 1e-9, "Notation: the played example fills 4.0 beats");
    }
    {
        const auto tied = build ({ note (69, 0, 0.5), note (72, 0.5, 1.0), note (71, 1.5, 0.5), note (69, 2.0, 1.5), note (67, 3.5, 1.0) });
        check (written (tied, true) == "0:6 6:6~ 12:6 18:6 24:18 42:6~ | 0:6 6:6r 12:12r 24:24r |",
               "Notation: the ties example is written with ties at beat 2 and over the bar line, then rests");
    }

    // One voice (R8): a note still held when the next starts is written up to it; notes starting together make a chord.
    {
        const auto s = build ({ note (60, 0, 4), note (64, 1, 1), note (67, 1, 1) });
        check (written (s) == "0:12 12:12 |" && s.staves[0].bars[0].events[1].pitches.size() == 2,
               "Notation: a held note ends where the next chord starts, and simultaneous notes form a chord");
    }

    // Accidentals (R6).
    {
        ChordMap chords;
        chords[0] = dChord;
        const auto s = build ({ note (65, 0, 1), note (66, 1, 1), note (66, 2, 1), note (65, 3, 1), note (70, 4, 1), note (71, 5, 1), note (65, 6, 2) },
                             nt::ClefChoice::treble, chords);
        juce::String shown;

        for (auto& bar : s.staves[0].bars)
            for (auto& e : bar.events)
                if (! e.isRest)
                    shown << (e.pitches[0].showAccidental ? (e.pitches[0].alter > 0 ? "#" : e.pitches[0].alter < 0 ? "b" : "n") : "-");

        check (shown == "-#-nbn-", "Notation: accidentals last to the bar line: bar 1 none, sharp, none, natural; bar 2 flat, natural, none (" + shown + ")");
    }

    // Stems (R12) and beams (R11).
    auto stemOf = [&] (std::vector<MidiNote> notes) { return build (std::move (notes)).staves[0].bars[0].events[0].stemUp; };
    check (stemOf ({ note (67, 0, 1) }) && ! stemOf ({ note (74, 0, 1) }) && ! stemOf ({ note (71, 0, 1) }),
           "Notation: stems go up below the middle line (G4) and down on or above it (D5, B4)");
    {
        const auto low = build ({ note (72, 0, 0.5), note (71, 0.5, 0.5) });
        const auto wide = build ({ note (64, 0, 0.5), note (74, 0.5, 0.5) });
        check (! low.staves[0].bars[0].events[0].stemUp && ! low.staves[0].bars[0].events[1].stemUp
                   && wide.staves[0].bars[0].events[0].stemUp && wide.staves[0].bars[0].events[1].stemUp,
               "Notation: a beamed group follows its furthest note: C5 B4 down, E4 D5 up");
    }
    {
        const auto four = build ({ note (67, 0, 0.5), note (67, 0.5, 0.5), note (67, 1, 0.5), note (67, 1.5, 0.5) });
        const auto flag = build ({ note (67, 0, 0.5) });
        check (four.staves[0].bars[0].beams.size() == 2 && four.staves[0].bars[0].beams[0].events.size() == 2,
               "Notation: four eighths over two beats are two beam groups");
        check (flag.staves[0].bars[0].beams.empty() && flag.staves[0].bars[0].events[0].beamGroup == -1,
               "Notation: an eighth followed by a rest has a flag and no beam");
    }

    // Clefs: guitar is written an octave up; a grand staff splits at middle C.
    {
        const auto guitar = build ({ note (52, 0, 1) }, nt::ClefChoice::guitar);
        const auto& p = guitar.staves[0].bars[0].events[0].pitches[0];
        check (guitar.staves.size() == 1 && p.octave == 3 + 1 && p.midi == 52 && guitar.staves[0].writtenShift == 12,
               "Notation: guitar writes E3 as E4 on the treble staff");
        const auto grand = build ({ note (48, 0, 1), note (60, 0, 1), note (72, 1, 1) }, nt::ClefChoice::grand);
        check (grand.isGrand() && grand.staves[0].bars[0].events[0].pitches.size() == 1 && grand.staves[0].bars[0].events[0].pitches[0].midi == 60
                   && grand.staves[1].bars[0].events[0].pitches[0].midi == 48,
               "Notation: a grand staff puts middle C and up in the treble staff, the rest in the bass");
    }

    // Tempo marks (R13), chord symbols and sections (R14).
    {
        nt::Input in;
        in.notes = { note (67, 0, 40) };
        in.tempo = TempoMap (std::vector<TempoChange> { { 0.0, 120.0 }, { 8.0, 121.0 }, { 16.0, 125.0 } });
        in.chords[0] = Chord { 9, { 0, 3, 7 } };
        in.chords[1] = Chord { 9, { 0, 3, 7 } };
        in.chords[2] = Chord { 5, { 0, 4, 7 } };
        in.sections = { Section { 0, 4, SectionType::verse, "" }, Section { 4, 4, SectionType::chorus, "Chorus" } };
        in.key = aMinor;
        const auto s = nt::build (in);
        check (s.tempoMarks.size() == 2 && s.tempoMarks[0].bar == 0 && s.tempoMarks[0].bpm == 120 && s.tempoMarks[1].bar == 4
                   && s.tempoMarks[1].bpm == 125,
               "Notation: tempo marks for 120, 121, 125: one at bar 1 and one at the 125 change");
        check (s.chordSymbols.size() == 2 && s.chordSymbols[0].name == "Am" && s.chordSymbols[0].bar == 0 && s.chordSymbols[1].name == "F"
                   && s.chordSymbols[1].bar == 2,
               "Notation: chord symbols appear where the chord changes (Am, then F)");
        check (s.sectionMarks.size() == 2 && s.sectionMarks[1].bar == 4 && s.sectionMarks[1].name == "Chorus",
               "Notation: sections are marked at the bar they start");
    }

    // 200 seeded random parts keep the invariants.
    {
        juce::Random random (20260);
        bool barsFull = true, tiesEqual = true, xGrows = true, linesOk = true, barsInOrder = true, unchanged = true;

        for (int part = 0; part < 200; ++part)
        {
            nt::Input in;
            in.key = { random.nextInt (12), (MusicalKey::Scale) random.nextInt (MusicalKey::numScales) };
            in.clef = (nt::ClefChoice) random.nextInt (6);

            for (int i = random.nextInt (40); --i >= 0;)
                in.notes.push_back (note (24 + random.nextInt (76), random.nextDouble() * 16.0, 0.01 + random.nextDouble() * 6.0));

            for (int bar = 0; bar < 6; ++bar)
                if (random.nextBool())
                    in.chords[bar] = Chord { random.nextInt (12), random.nextBool() ? juce::Array<int> { 0, 4, 7 } : juce::Array<int> { 0, 3, 7 } };

            const auto before = in.notes;
            const auto score = nt::build (in);
            unchanged = unchanged && before.size() == in.notes.size();

            for (size_t i = 0; i < before.size() && unchanged; ++i)
                unchanged = before[i].pitch == in.notes[i].pitch && juce::exactlyEqual (before[i].start, in.notes[i].start);

            for (auto& staff : score.staves)
            {
                barsFull = barsFull && (int) staff.bars.size() == score.numBars;

                for (size_t b = 0; b < staff.bars.size(); ++b)
                {
                    double total = 0.0;
                    int lastStart = -1;

                    for (auto& e : staff.bars[b].events)
                    {
                        total += e.getBeats();
                        xGrows = xGrows && e.start >= lastStart;
                        lastStart = e.start;

                        if (e.tiedToNext)
                        {
                            const nt::Event* next = nullptr;
                            const auto& events = staff.bars[b].events;
                            const size_t index = (size_t) (&e - events.data());

                            if (index + 1 < events.size())
                                next = &events[index + 1];
                            else if (b + 1 < staff.bars.size())
                                next = &staff.bars[b + 1].events.front();

                            bool equal = next != nullptr && ! next->isRest && ! e.isRest && next->pitches.size() == e.pitches.size() && next->tiedFromPrevious;

                            for (size_t k = 0; equal && k < e.pitches.size(); ++k)
                                equal = static_cast<const nt::Pitch&> (e.pitches[k]) == static_cast<const nt::Pitch&> (next->pitches[k]);

                            tiesEqual = tiesEqual && equal;
                        }
                    }

                    barsFull = barsFull && std::abs (total - 4.0) < 1e-9;
                }
            }

            const double width = 120.0;
            const auto lines = nt::layoutLines (score, width);
            int expected = 0;

            for (auto& line : lines)
            {
                barsInOrder = barsInOrder && line.firstBar == expected && line.numBars > 0;
                expected += line.numBars;
                linesOk = linesOk && (line.width <= width + 1e-9 || line.numBars == 1);
                double lastX = line.headerWidth - 1.0;

                for (int b = 0; b < line.numBars; ++b)
                    for (auto& e : score.staves[0].bars[(size_t) (line.firstBar + b)].events)
                    {
                        const double x = nt::getEventX (line, b, e);
                        xGrows = xGrows && x >= lastX - 1e-9;
                        lastX = x;
                    }
            }

            barsInOrder = barsInOrder && expected == score.numBars;
        }

        check (barsFull, "Notation: 200 random parts: every bar of every staff adds up to exactly 4 beats");
        check (tiesEqual, "Notation: 200 random parts: a tie only joins equal pitches");
        check (xGrows, "Notation: 200 random parts: x grows with the beat along a line");
        check (barsInOrder && linesOk, "Notation: 200 random parts: lines hold whole bars in order and none is wider than the page");
        check (unchanged, "Notation: writing a score never changes the MIDI notes");
    }

    check (std::abs (0.0625 * 60.0 / 120.0 * 1000.0 - 31.25) < 1e-9, "Notation: a quarter of a sixteenth is 31.25 ms at 120 BPM");
}

static void runChatMusicianRangeTests()
{
    using namespace daw::gen::chatmusician;
    const daw::MusicalKey minor { 9, daw::MusicalKey::Scale::minor };
    const auto diatonic = chordVocabulary (minor, false, false, false);
    const auto extended = chordVocabulary (minor, true, false, true);
    check (diatonic.contains ("Am") && ! diatonic.contains ("E7") && extended.contains ("E7"),
           "ChatMusician chord vocabulary keeps the major dominant and sevenths explicit");
    std::vector<juce::StringArray> allowed { { "Am" }, extended, extended, { "E7" } };
    const auto schema = buildChordSchema (allowed);
    check (schema["additionalProperties"] == juce::var (false)
           && schema["required"].size() == 4 && schema["properties"]["bar1"]["enum"].size() == 1,
           "ChatMusician chord schema requires every bar and fixes the first chord");
    juce::String chordError;
    std::vector<daw::Chord> progression;
    check (parseChords (R"({"bar1":"Am","bar2":"Dm","bar3":"G","bar4":"E7"})", allowed, {}, progression, chordError)
           && progression.size() == 4, "ChatMusician validates allowed chords and fixed endpoints");
    std::vector<daw::MidiNote> voiced;
    check (voiceChords (progression, 8.0, voiced) && ! voiced.empty()
           && juce::exactlyEqual (voiced.front().start, 8.0) && juce::exactlyEqual (voiced.back().getEnd(), 24.0),
           "ChatMusician voices each chord for one bar at the selected range");
    for (const auto* bad : { R"({"bar1":"Am","bar2":"Dm","bar3":"Dm","bar4":"E7"})",
                             R"({"bar1":"Am","bar2":"Dm","bar3":"G","bar4":"Em"})",
                             R"({"bar1":"Am","bar2":"Dm","bar3":"G")" })
        check (! parseChords (bad, allowed, {}, progression, chordError),
               "ChatMusician rejects repeated, disallowed or incomplete chords");
    int calls = 0;
    const auto retry = [&calls] (const juce::String&, const juce::var&, int, juce::String&) -> juce::String
    {
        return ++calls == 1 ? R"({"bar1":"Am","bar2":"Dm","bar3":"Dm","bar4":"E7"})"
                            : R"({"bar1":"Am","bar2":"Dm","bar3":"G","bar4":"E7"})";
    };
    check (generateChords (minor, allowed, retry, {}, progression, chordError) && calls == 2,
           "ChatMusician retries an invalid progression and accepts a valid one");
    calls = 0;
    check (! generateChords (minor, allowed, retry, [] { return true; }, progression, chordError) && calls == 0,
           "ChatMusician cancellation leaves a staged progression unchanged");
    const std::vector<juce::StringArray> fiveBars { { "Am" }, { "Dm" }, { "G" }, { "E7" }, { "E7" } };
    int chunkCalls = 0;
    const auto repeatingBoundary = [&chunkCalls] (const juce::String&, const juce::var&, int, juce::String&) -> juce::String
    {
        return ++chunkCalls == 1 ? R"({"bar1":"Am","bar2":"Dm","bar3":"G","bar4":"E7"})" : R"({"bar1":"E7"})";
    };
    const auto original = progression;
    check (! generateChords (minor, fiveBars, repeatingBoundary, {}, progression, chordError)
           && chunkCalls == 4 && progression == original,
           "ChatMusician rejects repeated chords across chunks after bounded retries without changing the take");
    juce::String error;
    std::vector<daw::MidiNote> parsed;
    const auto valid = R"({"bar1":"C D E G","bar2":"A B c c'"})";
    check (parseReply (valid, 2, 8.0, parsed, error) && parsed.size() == 8
           && parsed[0].pitch == 60 && parsed[4].pitch == 69 && parsed[7].pitch == 84
           && juce::exactlyEqual (parsed[0].start, 8.0) && juce::exactlyEqual (parsed[7].getEnd(), 16.0),
           "ChatMusician validates pitches and places exactly two bars at the selected offset");
    std::vector<daw::Chord> longChords (6, daw::Chord {});
    int requests = 0;
    const auto fakeMelody = [&requests] (const juce::String&, const juce::var&, juce::String&) -> juce::String
    {
        return ++requests == 1 ? R"({"bar1":"C D E G","bar2":"C D E G","bar3":"C D E G","bar4":"C D E G"})"
                               : R"({"bar1":"C D E G","bar2":"C D E G"})";
    };
    std::vector<daw::MidiNote> generated;
    check (generateMelody (minor, longChords, fakeMelody, {}, generated, error) && requests == 2
           && generated.size() == 24 && juce::exactlyEqual (generated.back().getEnd(), 24.0),
           "ChatMusician stitches a six-bar melody without trimming or padding");
    for (const auto* bad : { R"({"bar1":"C D E G"})", R"({"bar1":"C D E G","bar2":"A B c c'","bar3":"C D E F"})",
                             R"({"bar1":"C D E","bar2":"A B c d"})", R"({"bar1":"C# D E G","bar2":"A B c d"})",
                             R"({"bar1":"C D E G","bar2":)", R"({"bar1":"C  D E G","bar2":"A B c d"})",
                             R"({"bar1":"C D E G","bar1":"A B c d"})", R"({"bar1":"C D E G","bar2":"A B c d"} trailing)" })
    {
        const auto oldSize = parsed.size();
        error.clear();
        check (! parseReply (bad, 2, 0.0, parsed, error) && parsed.size() == oldSize && error.isNotEmpty(),
               "ChatMusician rejects malformed or incomplete bars without changing the take");
    }
    const auto chunks = splitRange (40.0);
    check (chunks.size() == 3 && juce::exactlyEqual (chunks[0].start, 0.0) && juce::exactlyEqual (chunks[1].start, 16.0)
           && juce::exactlyEqual (chunks[2].start, 32.0) && juce::exactlyEqual (chunks[2].length, 8.0),
           "ChatMusician splits long clips at bar boundaries");
    check (splitRange (0.0).empty() && splitRange (37.5).empty() && splitRange (std::numeric_limits<double>::infinity()).empty(),
           "ChatMusician rejects an invalid clip length");

    if (chunks.size() != 3)
        return;

    std::vector<daw::MidiNote> result;
    const std::vector<daw::MidiNote> notes { { 60, 0.0, 1.0, 0.8f }, { 64, 5.0, 2.0, 0.8f } };
    check (appendChunk (result, notes, chunks[2], 40.0) && result.size() == 2
           && juce::exactlyEqual (result[0].start, 32.0) && juce::exactlyEqual (result[1].start, 37.0)
           && juce::exactlyEqual (result[1].length, 2.0),
           "ChatMusician offsets notes in the last chunk");
    const std::vector<daw::MidiNote> invalid { { 60, 0.0, 1.0, 0.8f }, { 128, 1.0, 1.0, 0.8f } };
    check (! appendChunk (result, invalid, chunks[0], 40.0) && result.size() == 2,
           "ChatMusician rejects a whole invalid chunk without modifying the take");
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    runChatMusicianRangeTests();

    juce::StringArray pluginPaths;
    juce::String ollamaModel;
    juce::File demucsModel, whisperModel, voiceFolder;

    for (int i = 1; i < argc; ++i)
    {
        const juce::String arg (argv[i]);

        if (arg == "--ollama" && i + 1 < argc)
            ollamaModel = argv[++i];
        else if (arg == "--demucs" && i + 1 < argc)
            demucsModel = juce::File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
        else if (arg == "--voice" && i + 1 < argc)
            voiceFolder = juce::File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
        else if (arg == "--whisper" && i + 1 < argc)
            whisperModel = juce::File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
        else
            pluginPaths.add (arg);
    }

    runTuneTrackerTests();
    runTuneRulesTests();
    runPsolaTests();
    runBuiltInTuneTests();
    runTuneEngineTests();
    runTuneAnalysisTests();
    runTuneGuideTests();
    runStereoTestSoundTests();
    runStereoMeasureTests();
    runLoudnessMeterInitialTests();
    runBuiltInStereoTests();
    runGeneratorTests();
    runGainStagingTests();
    runAudioRecordingTests();
    runComputerKeyboardTests();
    runLatencyMeasureTests();
    runRecordingGuideTests();
    runTunerTests();
    runTuningGuideTests();
    runSoundGuideTests();
    runPluginRegistryTests();
    runDawItPluginCopyTests();
    runTakeTests();
    runSplitJoinTests();
    runStemTests();
    runAudioToMidiTests();
    runHitFinderTests();
    runHitsToMidiTests();
    runAuditionTests();
    runDrumTriggerGuideTests();
    runChordStyleTests();
    runPlayingStyleGeneratorTests();
    runSectionTests();
    runRhythmTests();
    runChordEditingTests();
    runChordDiagramTests();
    runPlayAlongTests();
    runSingAlongTests();
    runMidiGuideTests();
    runChordSheetTests();
    runNotationTests();
    runReadingGuideTests();
    runPdfTests();
    runGuideBookletTests();
    runLyricsTests();
    runRhymeTests();
    runLyricsAiTests();
    runLyricsGuideTests();
    runSingingGuideTests();
    runSingThisTests();
    runLyricsFinderTests();
    runTempoFinderTests();
    runTempoMapTests();
    runChordFinderTests();
    runStructureFinderTests();
    runBounceTests();
    runBounceFocusTests();
    runBounceInPlaceTests();
    runChordGuideTests();
    runPitchRowsTests();
    runMetronomeTests();
    runRoutingTests();
    runBusAudioTests();
    runRoutingGuideTests();
    runSendRoutingTests();
    runSendAudioTests();
    runSendGuideTests();
    runSignalFlowTests();
    runExampleSongTests();
    runSongTemplateTests();
    runStylesGuideTests();
    runProjectFileTests();
    runBuiltInSynthTests();
    runBuiltInEqTests();
    runBuiltInCompressorTests();
    runBuiltInReverbTests();
    runReverbKindsTests();
    runReverbNetworkTests();
    runBuiltInDelayTests();
    runBuiltInSaturationTests();
    runBuiltInDistortionTests();
    runBuiltInGateTests();
    runBuiltInSubtractiveTests();
    runBuiltInAdditiveTests();
    runBuiltInFmTests();
    runBuiltInDrumsTests();
    runBuiltInPhysicalTests();
    runTheoryGuidesTests();
    runEqAndCompressionGuideTests();

    if (ollamaModel.isNotEmpty())
        runOllamaTests (ollamaModel);

    if (demucsModel != juce::File())
        runDemucsTest (demucsModel);

    if (whisperModel != juce::File())
        runWhisperTest (whisperModel);

    if (voiceFolder != juce::File())
        runVoiceTest (voiceFolder, whisperModel);

    daw::Engine engine;
    engine.deviceManager.removeAudioCallback (&engine); // we drive the callback ourselves
    FakeDevice device;
    engine.audioDeviceAboutToStart (&device);

    std::cout << "MIDI track with the built-in synth" << std::endl;
    auto* midiTrack = engine.addTrack (daw::Track::Type::midi);
    auto* clip = engine.addMidiClip (*midiTrack, 0.0, 4.0);
    clip->notes.push_back ({ 60, 0.0, 2.0, 0.9f });

    auto out = render (engine, 0.1);
    check (rms (out, 0) == 0.0f, "stopped transport is silent");

    // The sound guide's tone: heard while stopped, a copy adds up or cancels, and off is silent.
    {
        auto peakOf = [] (const juce::AudioBuffer<float>& b) { return b.getMagnitude (0, 0, b.getNumSamples()); };
        daw::Engine::Tone tone;
        tone.on = true;
        tone.levelDb = -6.0f;
        engine.setTone (tone);
        render (engine, 0.1);
        out = render (engine, 0.2);
        check (std::abs (peakOf (out) - 0.5f) < 0.01f && std::abs (rms (out, 1) - 0.5f / std::sqrt (2.0f)) < 0.01f,
               "the sound guide's tone plays at its level (peak " + juce::String (peakOf (out), 3) + ")");

        tone.withCopy = true;
        tone.phaseDegrees = 0.0f;
        engine.setTone (tone);
        render (engine, 0.1);
        out = render (engine, 0.2);
        check (std::abs (peakOf (out) - 1.0f) < 0.02f, "a copy in step doubles it (peak " + juce::String (peakOf (out), 3) + ")");

        tone.phaseDegrees = 180.0f;
        tone.shape = daw::soundguide::Shape::square;
        engine.setTone (tone);
        render (engine, 0.1);
        out = render (engine, 0.2);
        check (rms (out, 0) < 0.002f, "a copy half a cycle later cancels it (" + juce::String (rms (out, 0), 4) + ")");

        tone.phaseDegrees = 0.0f;
        tone.flipCopy = true;
        engine.setTone (tone);
        render (engine, 0.1);
        out = render (engine, 0.2);
        const float flippedRms = rms (out, 0);
        tone.phaseDegrees = 180.0f + 360.0f * 3.0f;
        engine.setTone (tone);
        render (engine, 0.1);
        out = render (engine, 0.2);
        check (flippedRms < 0.002f && std::abs (peakOf (out) - 1.0f) < 0.02f,
               "a flipped copy in step cancels it, and a flipped copy three and a half cycles later doubles it");
        tone.flipCopy = false;

        // The tone is in the middle, so left minus right cancels it; switched off, it's back.
        tone.withCopy = false;
        tone.phaseDegrees = 0.0f;
        tone.shape = daw::soundguide::Shape::sine;
        engine.setTone (tone);
        engine.setLeftMinusRight (true);
        render (engine, 0.1);
        out = render (engine, 0.2);
        const float cancelledRms = rms (out, 0) + rms (out, 1);
        engine.setLeftMinusRight (false);
        render (engine, 0.1);
        out = render (engine, 0.2);
        check (cancelledRms < 1.0e-4f && std::abs (rms (out, 0) - 0.5f / std::sqrt (2.0f)) < 0.01f && ! engine.isLeftMinusRight(),
               "hearing left minus right cancels a sound in the middle, and switching it off brings it back");

        tone.on = false;
        engine.setTone (tone);
        render (engine, 0.05);
        out = render (engine, 0.1);
        check (rms (out, 0) == 0.0f, "the tone stops");
    }

    // The sound guide's mono check: a sound on the left only plays on both speakers at half level, and only one speaker mode is on.
    {
        daw::Engine monoEngine;
        monoEngine.deviceManager.removeAudioCallback (&monoEngine);
        FakeDevice monoDevice;
        monoEngine.audioDeviceAboutToStart (&monoDevice);

        auto* t = monoEngine.addTrack (daw::Track::Type::audio);
        t->pan = -1.0f;   // hard left: whatever the pan law, the right speaker stays (almost) silent
        juce::AudioBuffer<float> sine (1, (int) (4.0 * testRate));
        for (int i = 0; i < sine.getNumSamples(); ++i)
            sine.setSample (0, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / (float) testRate));
        monoEngine.addAudioClip (*t, std::move (sine), testRate, 0.0, "Left");
        monoEngine.play();
        render (monoEngine, 0.2);
        auto plain = render (monoEngine, 0.2);
        const float leftOnly = rms (plain, 0), rightOnly = rms (plain, 1);

        monoEngine.setMono (true);
        render (monoEngine, 0.1);
        auto mono = render (monoEngine, 0.2);
        const float expected = 0.5f * (leftOnly + rightOnly);
        check (monoEngine.isMono() && leftOnly > 0.1f && std::abs (rms (mono, 0) - expected) < 0.01f && std::abs (rms (mono, 1) - expected) < 0.01f,
               "hearing in mono plays left and right added and halved on both speakers (" + juce::String (rms (mono, 0), 3) + ", " + juce::String (rms (mono, 1), 3) + ")");

        monoEngine.setLeftMinusRight (true);
        check (monoEngine.isLeftMinusRight() && ! monoEngine.isMono(), "switching left minus right on switches mono off");
        monoEngine.setMono (true);
        check (monoEngine.isMono() && ! monoEngine.isLeftMinusRight(), "switching mono on switches left minus right off");
        monoEngine.setLeftMinusRight (false);
        check (monoEngine.isMono(), "switching left minus right off leaves mono as it was");

        // A bounce is the plain mix, with mono on or off.
        monoEngine.setPositionBeats (0.0);
        juce::AudioBuffer<float> withMono (2, testBlock), without (2, testBlock);
        monoEngine.beginOffline (testRate, testBlock);
        float* a[] = { withMono.getWritePointer (0), withMono.getWritePointer (1) };
        monoEngine.renderOffline (a, 2, testBlock);
        monoEngine.endOffline();
        monoEngine.setMono (false);
        monoEngine.setPositionBeats (0.0);
        monoEngine.beginOffline (testRate, testBlock);
        float* b[] = { without.getWritePointer (0), without.getWritePointer (1) };
        monoEngine.renderOffline (b, 2, testBlock);
        monoEngine.endOffline();
        bool same = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < testBlock; ++i)
                same = same && juce::exactlyEqual (withMono.getSample (ch, i), without.getSample (ch, i));
        check (same, "a bounce is the plain mix with mono on");
        monoEngine.audioDeviceStopped();
    }

    engine.play();
    out = render (engine, 0.5);
    check (rms (out, 0) > 0.01f && rms (out, 1) > 0.01f, "note plays on both channels");
    check (midiTrack->builtInSynthPeak.exchange (0.0f) > 0.05f, "built-in synth level is metered");

    {
        daw::MidiClip octaves;
        octaves.notes = { { 60, 0.0, 1.0, 0.8f }, { 115, 1.0, 1.0, 0.8f } };
        check (engine.transposeClip (octaves, 12) && octaves.notes[0].pitch == 72 && octaves.notes[1].pitch == 127,
               "clip transposes up an octave");
        check (! engine.transposeClip (octaves, 12) && octaves.notes[1].pitch == 127 && engine.transposeClip (octaves, -12)
                   && octaves.notes[0].pitch == 60,
               "transposing that would leave the MIDI range does nothing");
        check (! daw::Engine::canTranspose (daw::MidiClip(), 12), "an empty clip can't be transposed");
    }
    {
        // Velocity mode: a drag changes the selected notes by the same number of steps.
        const auto v = [] (int n) { return (float) n / 127.0f; };
        daw::MidiClip c;
        c.notes = { { 60, 0.0, 1.0, v (40) }, { 62, 1.0, 1.0, v (63) }, { 64, 2.0, 1.0, v (127) }, { 65, 3.0, 1.0, v (10) } };
        const auto original = c.notes;
        juce::SortedSet<int> three;
        three.add (0);
        three.add (1);
        three.add (2);
        const auto steps = [&c] (int i) { return juce::roundToInt (c.notes[(size_t) i].velocity * 127.0f); };
        check (engine.setNoteVelocities (c, original, three, 20) && steps (0) == 60 && steps (1) == 83 && steps (2) == 127,
               "raising selected velocities by 20 keeps the differences and stops at 127");
        check (steps (3) == 10, "a note that is not selected keeps its velocity");
        check (! engine.setNoteVelocities (c, original, three, 20), "the same velocities again change nothing and send no change");
        check (engine.setNoteVelocities (c, original, three, -200) && steps (0) == 1 && steps (1) == 1 && steps (2) == 1,
               "velocity never goes below 1");
        check (engine.setNoteVelocities (c, original, three, 0) && steps (0) == 40 && steps (2) == 127,
               "a drag back to its start restores the original velocities");
        juce::SortedSet<int> top;
        top.add (2);
        check (! engine.setNoteVelocities (c, original, top, 5), "a note already at 127 raised further changes nothing");
    }
    check (std::abs (engine.getPositionBeats() - out.getNumSamples() / testRate * 2.0) < 1e-6, "transport advances at 120 BPM");

    check (! midiTrack->scaleFilter, "a new track shows every note");
    engine.setScaleFilter (*midiTrack, true);
    out = render (engine, 0.1);
    check (midiTrack->scaleFilter && rms (out, 0) > 0.01f, "the track's scale filter doesn't change how clips play");
    engine.setScaleFilter (*midiTrack, false);

    engine.setPositionBeats (0.0); // start the note again: the transpose checks released it
    midiTrack->pan = -1.0f;
    out = render (engine, 0.2);
    const int half = out.getNumSamples() / 2;
    check (rms (out, 0, half) > 0.01f && rms (out, 1, half) < 1e-4f, "hard-left pan silences the right channel");

    midiTrack->pan = 0.0f;
    midiTrack->volumeDb = -60.0f;
    out = render (engine, 0.2);
    check (rms (out, 0, out.getNumSamples() / 2) < 1e-4f, "volume at -inf is silent");
    midiTrack->volumeDb = 0.0f;

    midiTrack->muted = true;
    out = render (engine, 0.2);
    check (rms (out, 0, out.getNumSamples() / 2) < 1e-4f, "mute is silent");
    midiTrack->muted = false;

    engine.setClipMuted (*clip, true);   // releases the note the synth still held: let it fade out
    render (engine, 0.3);
    engine.setPositionBeats (0.0);
    out = render (engine, 0.2);
    check (rms (out, 0) < 1e-4f && ! midiTrack->muted.load(), "a muted MIDI clip is silent while its track plays");
    engine.setClipMuted (*clip, false);
    engine.setPositionBeats (0.0);
    out = render (engine, 0.2);
    check (rms (out, 0, out.getNumSamples() / 2) > 0.01f, "unmuted, it plays again");

    std::cout << "Note release and looping" << std::endl;
    engine.setPositionBeats (2.5); // after the note ended
    out = render (engine, 0.6);
    check (rms (out, 0, out.getNumSamples() / 2) < 1e-4f, "note is released after it ends");

    engine.loopStartBeat = 0.0;
    engine.loopEndBeat = 1.0;
    engine.loopEnabled = true;
    engine.setPositionBeats (0.0);
    out = render (engine, 2.0);
    check (engine.getPositionBeats() < 1.0, "loop keeps the playhead inside the loop range");
    check (rms (out, 0, out.getNumSamples() - 4800) > 0.01f, "looped note re-triggers");
    engine.loopEnabled = false;

    std::cout << "MIDI recording" << std::endl;
    {
        using Message = juce::MidiMessage;
        const auto near = [] (double a, double b) { return std::abs (a - b) < 0.05; };
        const auto findNote = [] (const daw::MidiClip& c, int pitch) -> const daw::MidiNote*
        {
            for (auto& n : c.notes)
                if (n.pitch == pitch)
                    return &n;

            return nullptr;
        };

        engine.setLiveMidiTrack (midiTrack);
        engine.setPositionBeats (8.0);
        const auto clipsBefore = midiTrack->clips.size();

        check (engine.startRecording() && engine.isRecording() && engine.isPlaying() && midiTrack->armed.load()
                   && midiTrack->clips.size() == clipsBefore + 1 && midiTrack->clips.back()->startBeat == 8.0,
               "Record arms the selected track and starts a take at the playhead's bar");
        auto& take = static_cast<daw::MidiClip&> (*midiTrack->clips.back());

        render (engine, 0.25);                                          // beat 8.5
        engine.addMidiInput (Message::noteOn (1, 64, 0.5f));
        out = render (engine, 0.5);                                     // 9.5
        check (rms (out, 0) > 0.01f, "the armed track plays the keyboard while recording");
        engine.addMidiInput (Message::noteOff (1, 64));
        engine.addMidiInput (Message::noteOn (1, 67, 0.8f));
        engine.addMidiInput (Message::controllerEvent (1, 64, 127));    // sustain pedal down
        render (engine, 0.25);                                          // 10
        engine.addMidiInput (Message::noteOff (1, 67));                 // released, but the pedal holds it
        render (engine, 0.25);                                          // 10.5
        engine.addMidiInput (Message::controllerEvent (1, 64, 0));
        engine.addMidiInput (Message::noteOn (1, 72, 0.8f));            // still held when recording stops
        render (engine, 2.5);                                           // 15.5
        engine.stopRecording();

        const auto* n64 = findNote (take, 64);
        const auto* n67 = findNote (take, 67);
        const auto* n72 = findNote (take, 72);
        check (take.notes.size() == 3 && n64 != nullptr && near (n64->start, 0.5) && near (n64->length, 1.0)
                   && std::abs (n64->velocity - 0.5f) < 0.02f,
               "a played note is recorded at its position, with its length and velocity");
        check (n67 != nullptr && near (n67->start, 1.5) && near (n67->length, 1.0), "the sustain pedal holds a released note until it comes up");
        check (n72 != nullptr && near (n72->start, 2.5) && near (n72->length, 5.0), "a key still held when recording stops ends there");
        check (! engine.isRecording() && engine.isPlaying() && take.startBeat == 8.0 && take.lengthBeats == 8.0,
               "the take grows bar by bar, and playback goes on after recording");

        engine.setPositionBeats (40.0);
        const auto clipsAfterTake = midiTrack->clips.size();
        engine.startRecording();
        engine.previewNote (60, 0.8f);
        render (engine, 0.5);
        engine.stopRecording();
        check (midiTrack->clips.size() == clipsAfterTake, "note previews aren't recorded, and a take with nothing played is removed");

        // Loop recording: a note held across the wrap ends at the loop end, and a note before the
        // take's first bar moves the take's start back.
        engine.loopStartBeat = 22.0;
        engine.loopEndBeat = 28.0;
        engine.loopEnabled = true;
        engine.setPositionBeats (26.0);
        engine.startRecording();
        auto& loopTake = static_cast<daw::MidiClip&> (*midiTrack->clips.back());
        render (engine, 0.75);                                          // 27.5
        engine.addMidiInput (Message::noteOn (1, 60, 0.8f));
        render (engine, 0.5);                                           // wraps at 28, now 22.5
        engine.addMidiInput (Message::noteOff (1, 60));
        engine.addMidiInput (Message::noteOn (1, 62, 0.8f));
        render (engine, 0.25);                                          // 23
        engine.addMidiInput (Message::noteOff (1, 62));
        render (engine, 0.05);
        engine.stopRecording();
        engine.loopEnabled = false;

        const auto* n60 = findNote (loopTake, 60);
        const auto* n62 = findNote (loopTake, 62);
        check (loopTake.startBeat == 20.0 && n60 != nullptr && near (n60->start, 7.5) && near (n60->length, 0.5)
                   && n62 != nullptr && near (n62->start, 2.5) && near (n62->length, 0.5),
               "loop recording keeps notes from every pass");

        engine.removeClip (*midiTrack, &take);
        engine.removeClip (*midiTrack, &loopTake);

        // With the scale filter on, keys outside the song key neither sound nor record.
        engine.setKey ({ 0, daw::MusicalKey::Scale::major });
        engine.setScaleFilter (*midiTrack, true);
        engine.stop();
        engine.setPositionBeats (60.0);
        render (engine, 0.3);
        engine.addMidiInput (Message::noteOn (1, 61, 0.8f));            // C#: not in C major
        out = render (engine, 0.2);
        engine.addMidiInput (Message::noteOff (1, 61));
        engine.addMidiInput (Message::noteOn (1, 60, 0.8f));
        const auto inKey = render (engine, 0.2);
        engine.addMidiInput (Message::noteOff (1, 60));
        render (engine, 0.3);
        check (rms (out, 0) < 1e-4f && rms (inKey, 0) > 0.01f, "the scale filter silences keys outside the song key");

        engine.startRecording();
        auto& filteredTake = static_cast<daw::MidiClip&> (*midiTrack->clips.back());
        engine.addMidiInput (Message::noteOn (1, 61, 0.8f));
        engine.addMidiInput (Message::noteOn (1, 64, 0.8f));
        render (engine, 0.25);
        engine.addMidiInput (Message::noteOff (1, 61));
        engine.addMidiInput (Message::noteOff (1, 64));
        render (engine, 0.25);
        engine.stopRecording();
        check (filteredTake.notes.size() == 1 && filteredTake.notes[0].pitch == 64, "and only notes in the key are recorded");

        engine.removeClip (*midiTrack, &filteredTake);
        engine.setScaleFilter (*midiTrack, false);
        engine.setArmed (*midiTrack, false);
        render (engine, 0.05);
    }

    std::cout << "Audio track" << std::endl;
    {
        // One second of a 440 Hz sine at 44.1 kHz, placed at beat 2 (= 1 s at 120 BPM).
        auto file = juce::File::createTempFile (".wav");
        {
            juce::AudioBuffer<float> sine (1, 44100);
            for (int i = 0; i < sine.getNumSamples(); ++i)
                sine.setSample (0, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / 44100.0f));

            juce::WavAudioFormat wav;
            std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
            auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (44100.0)
                                                                                      .withNumChannels (1)
                                                                                      .withBitsPerSample (16));
            writer->writeFromAudioSampleBuffer (sine, 0, sine.getNumSamples());
        }

        midiTrack->muted = true;
        auto* audioTrack = engine.addTrack (daw::Track::Type::audio);
        juce::String error;
        auto* audioClip = engine.addAudioClip (*audioTrack, file, 2.0, error);
        check (audioClip != nullptr, "wav file loads " + error);
        check (audioClip != nullptr && std::abs (audioClip->getLengthBeats (daw::TempoMap (120.0)) - 2.0) < 1e-3, "1 s clip is 2 beats at 120 BPM");

        engine.setPositionBeats (0.0);
        out = render (engine, 1.5);
        const int oneSecond = (int) testRate;
        check (rms (out, 0, 2400, oneSecond - 2500) < 1e-4f, "silent before the clip starts"); // after the mute fade
        check (rms (out, 0, oneSecond + 100, 20000) > 0.2f, "clip plays from its start position (resampled 44.1k -> 48k)");
        check (audioTrack->peakLeft.load() > 0.45f && audioTrack->peakLeft.load() <= 0.5f && engine.masterPeakLeft.load() > 0.3f,
               "the track and master meters show the level (" + juce::String (audioTrack->peakLeft.load(), 3) + ")");

        // Count zero crossings to verify playback speed: 440 Hz over 0.25 s = ~220 crossings.
        int crossings = 0;
        for (int i = oneSecond + 1; i < oneSecond + 12000; ++i)
            if ((out.getSample (0, i - 1) < 0.0f) != (out.getSample (0, i) < 0.0f))
                ++crossings;
        check (std::abs (crossings - 220) <= 2, "pitch is preserved (" + juce::String (crossings) + " zero crossings, expected 220)");

        engine.setClipGain (*audioClip, -6.0f);
        engine.setPositionBeats (2.0);
        render (engine, 0.05);   // past the fade after the jump
        out = render (engine, 0.25);
        const float quieter = rms (out, 0);
        engine.setClipGain (*audioClip, 0.0f);
        engine.setPositionBeats (2.0);
        render (engine, 0.05);
        out = render (engine, 0.25);
        check (std::abs (quieter / rms (out, 0) - juce::Decibels::decibelsToGain (-6.0f)) < 0.01f,
               "clip gain -6 dB plays the clip at half the level (" + juce::String (quieter / rms (out, 0), 3) + ")");

        // Gain staging: the levels into and out of the fader and the master, and how much an effect
        // changes the loudness.
        {
            audioTrack->volumeDb = -6.0f;
            engine.setPositionBeats (2.0);
            render (engine, 0.1);   // past the fade after the jump and the fader's smoothing
            audioTrack->faderInputPeak = 0.0f;
            audioTrack->faderOutputPeak = 0.0f;
            engine.masterInputPeak = 0.0f;
            engine.masterOutputPeak = 0.0f;
            render (engine, 0.2);
            const float faderIn = audioTrack->faderInputPeak.load(), faderOut = audioTrack->faderOutputPeak.load();
            const float masterIn = engine.masterInputPeak.load(), masterOut = engine.masterOutputPeak.load();
            check (std::abs (faderIn - 0.5f) < 0.03f && std::abs (faderOut - 0.5f * juce::Decibels::decibelsToGain (-6.0f)) < 0.02f
                       && std::abs (masterIn - faderOut) < 0.01f && std::abs (masterOut - masterIn) < 0.01f,
                   "the fader and the master measure what goes in and comes out (fader " + juce::String (faderIn, 3) + " -> "
                       + juce::String (faderOut, 3) + ", master " + juce::String (masterIn, 3) + " -> " + juce::String (masterOut, 3) + ")");
            audioTrack->volumeDb = 0.0f;

            juce::String eqError;
            auto* eqSlot = engine.addPlugin (*audioTrack, daw::BuiltInEq::getDescription(), eqError);
            auto* eqPlugin = eqSlot != nullptr ? dynamic_cast<daw::BuiltInEq*> (eqSlot->instance.get()) : nullptr;
            check (eqPlugin != nullptr, "DawIt EQ goes on the audio track " + eqError);

            if (eqPlugin != nullptr)
            {
                auto changeDb = [&]
                {
                    eqSlot->inputEnergy = 0.0f;
                    eqSlot->outputEnergy = 0.0f;
                    render (engine, 0.15);
                    return 10.0f * std::log10 (eqSlot->outputEnergy.load() / juce::jmax (1.0e-12f, eqSlot->inputEnergy.load()));
                };

                eqPlugin->setBand (2, { true, daw::eq::BandType::bell, 440.0f, 6.0f, 1.0f, 12 });
                engine.setPositionBeats (2.0);
                render (engine, 0.1);
                const float louder = changeDb();
                check (std::abs (louder - 6.0f) < 0.2f, "an effect's energy in and out tells how much louder it makes the sound ("
                                                            + juce::String (louder, 2) + " dB, expected 6)");

                eqPlugin->setMatchingLevel (true);
                render (engine, 0.4);
                const float matched = changeDb();
                check (eqPlugin->isMatchingLevel() && std::abs (matched) < 0.5f,
                       "with match level on, it is as loud coming out as going in (" + juce::String (matched, 2) + " dB)");

                engine.setPluginBypassed (*eqSlot, true);
                engine.setPositionBeats (2.0);
                render (engine, 0.1);
                const float bypassed = changeDb();
                check (std::abs (bypassed) < 0.01f, "a bypassed effect doesn't change the loudness (" + juce::String (bypassed, 3) + " dB)");
                engine.removePlugin (*audioTrack, eqSlot);
            }
        }

        engine.setClipMuted (*audioClip, true);
        engine.setPositionBeats (2.0);
        out = render (engine, 0.5);
        check (rms (out, 0) < 1e-4f, "a muted audio clip is silent");
        check (audioTrack->peakLeft.load() < 0.05f, "in silence the meter falls back by itself, without a view resetting it (" + juce::String (audioTrack->peakLeft.load(), 4) + ")");

        engine.removeTrack (audioTrack);
        file.deleteFile();
        midiTrack->muted = false;
    }

    std::cout << "Song key" << std::endl;
    {
        check (engine.getKey() == daw::MusicalKey {}, "default key is C major (" + engine.getKey().getName() + ")");
        check (engine.getKey().contains (64) && ! engine.getKey().contains (61), "E is in C major, C# is not");

        engine.setKey ({ 9, daw::MusicalKey::Scale::minor });
        const auto key = engine.getKey();
        check (key.getName() == "A minor", "key can be set to " + key.getName());
        check (key.contains (69) && key.contains (72) && key.contains (79) && ! key.contains (68),
               "A minor contains A, C, G but not G#");
        check (daw::MusicalKey { 9, daw::MusicalKey::Scale::harmonicMinor }.contains (68), "A harmonic minor contains G#");

        for (int i = 0; i < daw::MusicalKey::numScales; ++i)
        {
            const auto intervals = daw::MusicalKey::getIntervals ((daw::MusicalKey::Scale) i);
            if (intervals.size() != 7 || intervals[0] != 0)
                check (false, "scale " + juce::String (i) + " has 7 degrees starting at the tonic");
        }

        engine.setKey ({});
    }

    if (pluginPaths.size() > 0)
    {
        std::cout << "Plugin instrument: " << pluginPaths[0] << std::endl;
        auto description = findPlugin (engine, pluginPaths[0]);
        check (description.name.isNotEmpty(), "found plugin \"" + description.name + "\" (" + description.pluginFormatName + ")");

        juce::String error;
        auto* slot = description.name.isNotEmpty() ? engine.addPlugin (*midiTrack, description, error) : nullptr;
        check (slot != nullptr, "instantiated " + error);
        check (midiTrack->hasInstrument() == description.isInstrument, "instrument flag is " + juce::String (description.isInstrument ? "true" : "false"));

        if (slot != nullptr)
            std::cout << "  (" << slot->instance->getTotalNumInputChannels() << " in, "
                      << slot->instance->getTotalNumOutputChannels() << " out)" << std::endl;

        if (slot != nullptr)
        {
            engine.setPositionBeats (0.0);
            out = render (engine, 1.0);
            check (rms (out, 0) > 1e-4f || rms (out, 1) > 1e-4f,
                   "plugin produces audio for the clip (rms " + juce::String (rms (out, 0), 5) + ")");
        }
    }

    if (pluginPaths.size() > 1)
    {
        std::cout << "Plugin effect: " << pluginPaths[1] << std::endl;
        auto description = findPlugin (engine, pluginPaths[1]);
        juce::String error;
        auto* slot = description.name.isNotEmpty() ? engine.addPlugin (*midiTrack, description, error) : nullptr;
        check (slot != nullptr, "instantiated \"" + description.name + "\" " + error);

        if (slot != nullptr)
        {
            engine.setPositionBeats (0.0);
            out = render (engine, 1.0);
            check (rms (out, 0) > 1e-4f || rms (out, 1) > 1e-4f, "audio passes through the effect chain");
            check (slot->inputPeak.exchange (0.0f) > 1e-4f && slot->outputPeak.exchange (0.0f) > 1e-4f,
                   "effect input and output levels are metered");
            check (std::isfinite (out.getMagnitude (0, out.getNumSamples())), "output is finite");

            engine.movePlugin (*midiTrack, slot, 0);
            check (midiTrack->plugins.front().get() == slot, "effect moves to the front of the chain");
            engine.movePlugin (*midiTrack, slot, 99);
            check (midiTrack->plugins.back().get() == slot, "out-of-range move clamps to the end");

            engine.setPluginBypassed (*slot, true);
            check (slot->bypassed.load(), "effect can be bypassed");

            engine.setPositionBeats (0.0);
            slot->inputPeak = 0.0f;
            slot->outputPeak = 0.0f;
            render (engine, 0.3);
            check (slot->inputPeak.load() > 1e-4f && juce::exactlyEqual (slot->inputPeak.load(), slot->outputPeak.load()),
                   "a bypassed effect's output level equals its input");
            engine.setPluginBypassed (*slot, false);

            juce::String insertError;
            auto* inserted = engine.addPlugin (*midiTrack, description, insertError, 0);
            check (inserted != nullptr && midiTrack->plugins.front().get() == inserted, "plugin can be inserted at the front");

            if (inserted != nullptr)
                engine.removePlugin (*midiTrack, inserted);

            check (midiTrack->plugins.size() == 2, "removing leaves the rest of the chain");
        }
    }

    engine.stop();
    std::cout << (failures == 0 ? "All checks passed" : juce::String (failures) + " check(s) FAILED") << std::endl;

    // Leave without running static destructors: ONNX Runtime's telemetry thread can still be handling an
    // upload response while the library tears down its own statics, and then aborts with "recursive_mutex
    // lock failed" after the last check (about 1 run in 8). The result is already printed and flushed.
    std::cout.flush();
    std::_Exit (failures == 0 ? 0 : 1);
}
