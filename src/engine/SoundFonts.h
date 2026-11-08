#pragma once

#include "ModelDownload.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

struct tsf;   // TinySoundFont's handle (see SoundFontCore.h); only SoundFonts.cpp and BuiltInSampler.cpp use its functions

namespace daw::soundfonts
{
/**
    SoundFonts for DawIt Sampler: finding and checking the files, listing the sounds in one without loading
    its recordings, the map of key and velocity zones of a sound, and a shared library that loads each file
    once, on a background thread, for every sampler that plays it. No UI and no `Engine.h`.

    A SoundFont (.sf2, or .sf3 with compressed recordings) holds presets (what you choose, such as "Acoustic
    Grand Piano", numbered by bank and program), made of zones with a key range, a velocity range and a recording.
    Programs are stored 0 to 127 here and shown 1 to 128, as General MIDI does. Bank 128 holds drum kits.
*/

/** The bank that holds drum kits. */
constexpr int drumBank = 128;

/** DawIt's own SoundFonts folder, next to the Models folder
    (~/Library/Application Support/DawIt/SoundFonts on macOS), or the folder set by setFolderOverride. */
juce::File getFolder();

/** Makes getFolder return this folder instead (for tests and the screenshot runner); an empty File puts it back. */
void setFolderOverride (const juce::File&);

/** Which SoundFont a sampler plays, as a project remembers it: the file's name and size, and for a file the user
    linked instead of copying, its full path. */
struct Reference
{
    juce::String fileName;
    juce::int64 size = 0;
    juce::String linkedPath;

    bool isEmpty() const noexcept { return fileName.isEmpty(); }
    bool operator== (const Reference& other) const { return fileName == other.fileName && size == other.size && linkedPath == other.linkedPath; }

    /** A reference to this file: by name in the SoundFonts folder when it is there (`linked` false), or by its full path. */
    static Reference forFile (const juce::File&, bool linked);

    /** Finds the file: in the SoundFonts folder by name and size, then at the linked path. An empty File when it isn't there. */
    juce::File find() const;
};

//==============================================================================
// MuseScore General: the free General MIDI sound set DawIt offers to download.

/** The file name of the set in the SoundFonts folder, and of its licence file beside it. */
constexpr const char* museScoreGeneralFileName = "MuseScore_General.sf3";
constexpr const char* museScoreGeneralLicenceFileName = "MuseScore_General_License.md";

/** MuseScore General 0.2.0 (MIT licence, 39,900,972 bytes) at its pinned address, with its size and SHA-256, for
    `models::download`. It goes to the SoundFonts folder. */
const models::Model& getMuseScoreGeneral();

/** Its licence file, which must go with the set (the copyright notices are part of the licence). */
const models::Model& getMuseScoreGeneralLicence();

/** The reference a sampler keeps for MuseScore General: its name and size, so it is found in the SoundFonts folder. */
Reference getMuseScoreGeneralReference();

/** True when the set is in the SoundFonts folder at its full size. */
bool isMuseScoreGeneralInstalled();

/** Where the licence file is (or will be) saved, beside the set. It may not exist. */
juce::File getMuseScoreGeneralLicenceFile();

/** Downloads the set and, beside it, its licence file (fetched first: it is small), each checked against its size and
    SHA-256. Returns an error in plain words, or an empty string on success or when cancelled. Blocks; call it from a
    background thread, as `models::download`. */
juce::String downloadMuseScoreGeneral (const models::ProgressFn&);

//==============================================================================
// SoundFonts the user adds.

/** What adding a file came to. */
struct AddResult
{
    bool ok = false;
    Reference reference;     // what a sampler keeps, when it worked
    juce::String problem;    // plain words, when it didn't
};

/** True for a file name that DawIt Sampler plays: .sf2 or .sf3. */
bool isSoundFontFile (const juce::File&);

/** Checks a file the user chose and makes it available. It must end in .sf2 or .sf3 (an .sfz gets one sentence of
    explanation: SFZ libraries aren't supported yet) and pass `check`. With `copy`, the file is copied into the SoundFonts
    folder (a file that is already there is used as it is, and a different file of the same name gets a numbered name),
    and the reference is by name; without it the file stays where it is and the reference carries its path, which
    `addLinked` should record. Message thread or a background thread. */
AddResult addFile (const juce::File& chosen, bool copy);

/** The paths of linked SoundFonts, from the app settings (`soundfonts.linked`, one path to a line). */
juce::StringArray getLinked (const juce::PropertiesFile&);

/** Remembers a linked file's path, once. */
void addLinked (juce::PropertiesFile&, const juce::File&);

/** Forgets a linked path. */
void removeLinked (juce::PropertiesFile&, const juce::String& path);

/** The SoundFonts the user added, for the menus: the files in the SoundFonts folder other than MuseScore General, by
    name, then the linked files that still exist. */
std::vector<Reference> getAddedFonts (const juce::PropertiesFile* settings);

//==============================================================================
/** One sound of a SoundFont, read from its header alone. */
struct PresetInfo
{
    juce::String name;
    int bank = 0;
    int program = 0;   // 0 to 127

    /** "001 Acoustic Grand Piano": the program as shown, 1 to 128, and the name. */
    juce::String getLabel() const;
};

/** What the structure check found. */
struct CheckResult
{
    bool ok = false;
    juce::String problem;   // plain words, when it isn't ok: "The file is cut short ..."
};

/** Checks a file before anything decodes it: it is a SoundFont, every chunk and size is inside the file, and every
    index and sample position in its tables is in range, so the player never reads past a table or the samples.
    Works from a stream, so a bad file can't crash it. */
CheckResult check (const juce::File&);

/** The presets of a SoundFont, sorted by bank and then program, read from its header alone (the recordings are
    skipped, so a menu can list a 200 MB file instantly). Empty when the file can't be read. */
std::vector<PresetInfo> readPresets (const juce::File&);

/** How a zone's recording keeps sounding. */
enum class LoopMode { none, continuous, untilRelease };

/** One zone of a preset after its settings are merged. Samples are positions in the decoded sample memory. */
struct Zone
{
    int lowKey = 0, highKey = 127;
    int lowVelocity = 1, highVelocity = 127;
    int rootKey = 60;               // the note the recording was made at
    LoopMode loop = LoopMode::none;
    int sampleStart = 0, sampleEnd = 0, loopStart = 0, loopEnd = 0;
    int sampleRate = 44100;         // of the recording
    int tuneCents = 0, transposeSemitones = 0;
    double attack = 0.0, hold = 0.0, decay = 0.0, sustain = 1.0, release = 0.0;   // the volume envelope: seconds; sustain a level 0 to 1

    bool containsKey (int key) const noexcept { return key >= lowKey && key <= highKey; }
    bool containsVelocity (int velocity) const noexcept { return velocity >= lowVelocity && velocity <= highVelocity; }
};

/**
    A SoundFont loaded into memory, shared by every sampler that plays the file (see Library). Its recordings are
    held as 32-bit floats, about twice the size of an .sf2. It is loaded on a background thread: until
    `getState()` is `ready`, ask nothing else of it. The font is freed on the message thread when its last share goes.
*/
class Font
{
public:
    enum class State { loading, ready, failed };

    ~Font();

    State getState() const noexcept { return (State) state.load (std::memory_order_acquire); }

    /** Waits (polling) until the font is no longer loading, or the time is up. Returns true if it is ready. */
    bool waitUntilLoaded (int timeoutMs) const;

    /** Why it failed, in plain words, when `getState()` is `failed`. */
    juce::String getProblem() const;

    const juce::File& getFile() const noexcept { return file; }
    juce::String getName() const { return file.getFileName(); }

    /** The presets of the loaded font (empty until it's ready), sorted by bank and program. */
    const std::vector<PresetInfo>& getPresets() const noexcept { return presets; }

    /** The index TinySoundFont knows a preset by, or -1 when this font has no such preset. */
    int getPresetIndex (int bank, int program) const;

    /** The zones of a preset (empty when the font lacks it or isn't ready). */
    std::vector<Zone> getZones (int bank, int program) const;

    /** The recordings, decoded: 32-bit floats shared by every sampler of this font, or null until it's ready. */
    const float* getSamples() const;

    /** How many decoded samples the font holds, as far as it is known: exact for an .sf2 (its file holds 16-bit
        samples), the highest position any zone reaches for an .sf3. */
    juce::int64 getSampleCount() const noexcept { return sampleCount; }

    /** The memory the decoded samples take: the sample count times 4 bytes. */
    juce::int64 getMemoryBytes() const noexcept { return sampleCount * 4; }

    /** The size of the file, as it was when loading started. */
    juce::int64 getFileSize() const noexcept { return fileSize; }

    /** A copy for one sampler: its own voices (up to `maxVoices`, allocated now) and channel 0, the shared
        recordings, set to render stereo at this sample rate. Null while the font isn't ready. Message thread;
        close it with closeCopy. */
    ::tsf* makeCopy (double sampleRate, int maxVoices) const;

    /** Closes a copy made by makeCopy (the recordings stay while another copy or the font uses them). Message thread. */
    static void closeCopy (::tsf*);

    /** How many copies made by makeCopy are open now, in the whole process: a sampler's current copy, one waiting to be
        handed over, and any retired but not yet closed. For tests. */
    static int getOpenCopyCount() noexcept;

private:
    friend class Library;
    explicit Font (const juce::File&);
    void load();   // on the background thread

    const juce::File file;
    const juce::int64 fileSize;
    std::atomic<int> state { (int) State::loading };
    ::tsf* master = nullptr;
    std::vector<PresetInfo> presets;
    juce::int64 sampleCount = 0;
    juce::String problem;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Font)
};

/**
    One per process. Hands out a share of the loaded font for a file, starting its load on a background thread the
    first time and sharing it after that, so ten tracks on one SoundFont cost the memory of one. Loads run one at a time.
    The font is freed on the message thread when no sampler holds a share any more.
*/
class Library
{
public:
    static Library& get();

    /** The font for this file, shared with everyone else who asked for it. Starts loading it if nobody has.
        The check of the file's structure runs first, on the background thread; a file that fails it never reaches the
        decoder. Message thread. */
    std::shared_ptr<Font> acquire (const juce::File&);

    /** How many loads have started in this process. Two samplers on one file make one. */
    int getLoadCount() const noexcept { return loads.load(); }

    ~Library();

private:
    Library();

    struct Impl;
    std::unique_ptr<Impl> impl;
    std::atomic<int> loads { 0 };
};

//==============================================================================
/** The tiny SoundFonts that tests and screenshots use: no real instrument, but sines and noise at known pitches. */
namespace testfont
{
    /** The presets, by (bank, program):
        - (0, 0) "Test Piano": zone 1 keys 0-71, root 69, a 441 Hz sine (100 samples a period at 44.1 kHz); zone 2 keys 72-127,
          root 81, an 882 Hz sine; both play once (0.5 s).
        - (0, 1) "Test Layers": keys 0-127, root 69: velocity 0-63 a pure 441 Hz sine, 64-127 the sine plus its 2nd harmonic at -6 dB.
        - (0, 2) "Test Organ": the 441 Hz sine, looped while the key is held.
        - (0, 3) "Test Pluck": the same sine, played once (0.5 s).
        - (128, 0) "Test Kit": note 36 a kick, note 38 a snare. */
    constexpr int pianoProgram = 0, layersProgram = 1, organProgram = 2, pluckProgram = 3, kitProgram = 0;
    constexpr int sinePeriodSamples = 100;          // at 44.1 kHz: 441 Hz
    constexpr double sineHz = 441.0, upperSineHz = 882.0;
    constexpr int numPresets = 5;

    /** The font as the bytes of a file: an .sf2 with 16-bit recordings, or an .sf3 with each recording compressed
        with Ogg Vorbis (written with JUCE's encoder). */
    juce::MemoryBlock make (bool compressed);

    /** Writes it to a file. */
    bool write (const juce::File&, bool compressed);
} // namespace testfont
} // namespace daw::soundfonts
