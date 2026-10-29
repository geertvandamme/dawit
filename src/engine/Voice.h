#pragma once

#include <juce_core/juce_core.h>

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "YamlLite.h"

namespace daw::sing
{
/** The 15 vowels and 24 consonants of the CMU Pronouncing Dictionary, lower case and without stress
    (the sounds an English word is made of in Sing this). */
const std::vector<juce::String>& getCmuVowels();
const std::vector<juce::String>& getCmuConsonants();

/** The voices folder: ~/Library/Application Support/DawIt/Voices on macOS, next to the Models folder. */
juce::File getVoicesFolder();

/** Uses another folder as the voices folder (the screenshot runner shows the window with no voice or a test one);
    an empty File goes back to the usual one. */
void overrideVoicesFolder (const juce::File&);

/** The files of a DiffSinger voice in OpenUtau's layout: the root `dsconfig.yaml` names the acoustic model,
    the phonemes and the vocoder; the sub-folders `dsdur` and `dspitch` hold the optional models that decide
    how long each sound lasts and draw the pitch curve. */
struct VoiceFiles
{
    juce::File root;
    juce::File acoustic, phonemes, languages;
    juce::File vocoderModel, vocoderConfig;
    juce::File durLinguistic, dur, pitchLinguistic, pitch;   // optional; not set when the voice lacks them
    juce::File durSpeaker, pitchSpeaker;                     // the .emb file of the first speaker of those models, when they have speakers
    juce::File dictionary;                                   // dsdict-en.yaml or dsdict.yaml; optional
    juce::File terms;                                        // the readme or licence to show; optional
};

/** A DiffSinger voice, read and checked: every file the config names is there. Nothing is run yet. */
struct Voice
{
    juce::String name, author;
    VoiceFiles files;
    yaml::Node config;

    int sampleRate = 44100, hopSize = 512, numMelBins = 128;
    std::map<juce::String, int> phonemeIds;      // "en/ay" -> 12, "SP", "AP"
    std::map<juce::String, int> languageIds;     // "en" -> 1
    juce::String languagePrefix;                 // "en/" when the phonemes carry one, otherwise empty
    int englishLanguageId = -1;                  // the id to pass with each phoneme, -1 when the voice has no languages
    int numSpeakers = 0;                         // the speakers in dsconfig (a voice with 0 or 1 needs no embedding)
    std::map<juce::String, std::vector<juce::String>> dictionary;   // word -> its sounds in this voice's own spelling
    std::set<juce::String> dictionaryVowels;     // symbols the voice's dictionary calls vowels
    bool hasEnglish = false;
    juce::String vocoderLicence;                 // "CC BY-NC-SA 4.0" when a licence file next to the vocoder says so

    /** Our sound for an English phoneme ("ay"): the voice's spelling of it ("en/ay"), or empty when the voice has none. */
    juce::String mapPhoneme (const juce::String& cmuPhoneme) const;

    /** True when a symbol in this voice's spelling is a vowel (what carries the note). */
    bool isVowel (const juce::String& symbol) const;

    /** The lower-case name of a symbol without its language prefix. */
    juce::String stripPrefix (const juce::String& symbol) const;
};

struct VoiceCheck
{
    std::optional<Voice> voice;
    juce::String error;   // names the file that is missing or wrong; empty when the voice is fine
    bool vocoderMissing = false;   // the only thing wrong is the vocoder, which a .oudep file can add
};

/** Where a voice's root is: the folder itself when it has a dsconfig.yaml, or the one folder inside (up to
    two levels down, as zips are often packed). Empty when there is none. */
juce::File findVoiceRoot (const juce::File& folder);

/** Reads a voice folder and checks it: the config, the acoustic model, the phonemes, a vocoder (in the voice or
    in `vocodersFolder`) and English. The error says what is missing ("acoustic.onnx is missing from ...") and
    a voice with no English phonemes is refused. */
VoiceCheck loadVoice (const juce::File& folder, const juce::File& vocodersFolder);

/** Lists the folders in the voices folder that hold a voice, by name. */
std::vector<juce::File> listVoices (const juce::File& voicesFolder);
} // namespace daw::sing
