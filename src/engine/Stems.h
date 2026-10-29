#pragma once

#include "Engine.h"

#include <array>
#include <optional>

namespace demucscpp { struct demucs_model; }

namespace daw::stems
{
/**
    Splitting a mix into stems with Demucs v4 (htdemucs, through demucs.cpp), offline and on the CPU.

    The model is downloaded once (about 84 MB) into DawIt's application-support folder. A separation
    takes roughly as long as the audio on an M1 Max and needs about 2.5 GB of memory per piece
    that runs at the same time.
*/
constexpr int numStems = 4;

/** The stems in the model's order. */
inline const std::array<juce::String, numStems> names { "Drums", "Bass", "Other", "Vocals" };

/** The index of the vocals in the stems. */
constexpr int vocalsStem = 3;

/** Demucs works on stereo audio at this rate. */
constexpr double sampleRate = 44100.0;

using Stems = std::array<juce::AudioBuffer<float>, numStems>;

/** Reports progress from 0 to 1; returning false cancels. */
using ProgressFn = std::function<bool (double)>;

//==============================================================================
juce::File getModelFile();
bool isModelInstalled();
juce::int64 getModelDownloadSize();

/** Downloads the model to getModelFile() and checks it. Returns an error message, or an empty
    string on success or when cancelled. Blocks; call it from a background thread. */
juce::String downloadModel (const ProgressFn&);

//==============================================================================
/** The audio as stereo at 44.1 kHz, as Demucs wants it. */
juce::AudioBuffer<float> prepareInput (const juce::AudioBuffer<float>&, double sourceSampleRate);

/** Separates one piece of 44.1 kHz stereo audio. Reports progress from 0 to 1; returns nullopt
    when told to stop. May be called from several threads at once. */
using SeparateFn = std::function<std::optional<Stems> (const juce::AudioBuffer<float>&,
                                                       const std::function<bool (double)>& progressAndKeepGoing)>;

/** Cuts the audio into overlapping pieces, separates them at the same time on their own threads
    and cross-fades the results back together. Returns nullopt when cancelled or when a piece fails. */
std::optional<Stems> separateInPieces (const juce::AudioBuffer<float>&, int numPieces, const SeparateFn&, const ProgressFn&);

/** How many pieces of this much audio to separate at the same time: enough that a short clip
    takes one run of the model per piece, but no more than one per CPU core (up to 8) and as
    memory allows. */
int choosePieceCount (int numSamples);

/** True when the stem is more than 30 dB quieter than the loudest one: then it holds nothing
    but leftovers of the others (a drum loop has no vocals). */
bool isSilent (const Stems&, int index);

//==============================================================================
/** The Demucs model, loaded from the model file. */
class Separator
{
public:
    Separator();
    ~Separator();

    bool load (const juce::File&, juce::String& error);

    /** Splits 44.1 kHz stereo audio into the stems, using several threads. Blocks; returns nullopt
        when cancelled. */
    std::optional<Stems> separate (const juce::AudioBuffer<float>&, const ProgressFn&) const;

private:
    std::optional<Stems> separatePiece (const juce::AudioBuffer<float>&, const std::function<bool (double)>&) const;

    std::unique_ptr<demucscpp::demucs_model> model;

    JUCE_DECLARE_NON_COPYABLE (Separator)
};

//==============================================================================
/** Puts each stem that isn't silent on a new audio track right below the source track, at the
    clip's position, and mutes the clip, so the stems replace it; the rest of its track plays on.
    Returns the new tracks. Message thread. */
std::vector<Track*> addStemTracks (Engine&, Track& source, AudioClip&, Stems);

/** Puts one stem (44.1 kHz stereo) on a new audio track at `trackIndex`, named after the stem, in
    a clip at the source clip's position and gain. Message thread. */
Track* addStemTrack (Engine&, int trackIndex, const AudioClip& source, int stem, juce::AudioBuffer<float>);

//==============================================================================
// Karaoke: the song without its vocals

/** The stems added together, those with `which` set. Demucs's stems add up to the mix, so all but
    the vocals give the mix without them. */
juce::AudioBuffer<float> mixStems (const Stems&, const std::array<bool, numStems>& which);

/** How loud the vocals stay in a karaoke version: muted, or quietly under the band as a guide to
    sing along with. */
enum class GuideVocals { off, quiet, softer };

inline constexpr GuideVocals allGuideLevels[] = { GuideVocals::off, GuideVocals::quiet, GuideVocals::softer };

/** The Vocals track's volume: 0 dB when off (the track is muted, so unmuting brings the vocals back
    as they were), -12 dB when quiet and -6 dB when softer. */
float getGuideDb (GuideVocals);

/** "Off: the Vocals track is muted", "Quiet: the vocals at -12 dB"... for the dialog. */
juce::String getGuideName (GuideVocals);

/** What the karaoke version is made of. */
struct KaraokeChoices
{
    bool asStems = false;                    // drums, bass and other sounds on tracks of their own
    GuideVocals guide = GuideVocals::off;
};

/** The tracks a karaoke version added. */
struct Karaoke
{
    std::vector<Track*> instrumental;        // the Karaoke track, or the stems
    Track* vocals = nullptr;
};

/** Makes a karaoke version of the clip from its stems: right below the source track, the song
    without its vocals on a Karaoke track (or as the stems that aren't silent), then the vocals on a
    Vocals track at the guide level, muted when the guide is off. The clip is muted, so nothing plays
    twice; the rest of its track plays on. Adds nothing when the vocals are silent (the clip has no
    singing) or the whole clip is. Message thread. */
Karaoke addKaraokeTracks (Engine&, Track& source, AudioClip&, Stems, const KaraokeChoices&);
} // namespace daw::stems
