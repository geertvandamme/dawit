#include "SoundFonts.h"

#include "ModelDownload.h"
#include "SoundFontCore.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

#include <algorithm>
#include <cmath>
#include <map>

namespace daw::soundfonts
{
namespace
{
    std::mutex folderMutex;
    juce::File folderOverride;

    /** tsf_copy and tsf_close share a reference count that isn't thread safe: one lock for every font. */
    std::mutex& getCopyMutex()
    {
        static std::mutex m;
        return m;
    }

    std::atomic<int> openCopies { 0 };

    constexpr juce::int64 maxFileSize = 2000000000;   // TinySoundFont reads sizes as 32-bit numbers

    //==============================================================================
    // Reading the file's structure.
    struct Span
    {
        juce::int64 start = 0, size = 0;
        bool found = false;
    };

    struct Layout
    {
        Span phdr, pbag, pmod, pgen, inst, ibag, imod, igen, shdr, smpl;
        bool hasSmpo = false;
        juce::String problem;
    };

    constexpr int phdrSize = 38, pbagSize = 4, pmodSize = 10, pgenSize = 4, instSize = 22, ibagSize = 4, imodSize = 10,
                  igenSize = 4, shdrSize = 46;

    juce::String readFourCC (juce::InputStream& in)
    {
        char id[5] = {};
        in.read (id, 4);
        return juce::String (id, 4);
    }

    /** Walks the chunks as TinySoundFont does (no padding bytes) and records where the tables and the recordings are. */
    Layout scan (juce::FileInputStream& in)
    {
        Layout layout;
        const auto total = in.getTotalLength();

        if (total == 0)
        {
            layout.problem = "The file is empty.";
            return layout;
        }

        if (total > maxFileSize)
        {
            layout.problem = "The file is bigger than 2 GB, which DawIt Sampler can't load.";
            return layout;
        }

        if (total < 12)
        {
            layout.problem = "This isn't a SoundFont: the file is too short.";
            return layout;
        }

        in.setPosition (0);
        const auto riff = readFourCC (in);
        const juce::int64 riffSize = (juce::uint32) in.readInt();
        const auto form = readFourCC (in);

        if (riff != "RIFF" || form != "sfbk")
        {
            layout.problem = "This isn't a SoundFont: it doesn't start like an .sf2 or .sf3 file.";
            return layout;
        }

        const auto riffEnd = 8 + riffSize;

        if (riffEnd > total)
        {
            layout.problem = "The file is cut short: it ends before the SoundFont does.";
            return layout;
        }

        auto damaged = [&layout] { layout.problem = "The SoundFont is damaged: its parts don't add up."; };
        juce::int64 pos = 12;

        while (pos + 8 <= riffEnd)
        {
            in.setPosition (pos);
            const auto id = readFourCC (in);
            const juce::int64 size = (juce::uint32) in.readInt();
            const auto dataStart = pos + 8, dataEnd = dataStart + size;

            if (dataEnd > riffEnd)
            {
                layout.problem = "The file is cut short: a part of the SoundFont runs past its end.";
                return layout;
            }

            if (id == "LIST")
            {
                if (size < 4 || size % 2 != 0)
                {
                    damaged();
                    return layout;
                }

                const auto type = readFourCC (in);

                if (type == "pdta" || type == "sdta")
                {
                    juce::int64 sub = dataStart + 4;

                    while (sub + 8 <= dataEnd)
                    {
                        in.setPosition (sub);
                        const auto subId = readFourCC (in);
                        const juce::int64 subSize = (juce::uint32) in.readInt();
                        const auto subStart = sub + 8, subEnd = subStart + subSize;

                        if (subEnd > dataEnd)
                        {
                            layout.problem = "The file is cut short: a table of the SoundFont runs past its end.";
                            return layout;
                        }

                        auto take = [&] (Span& s, int recordSize)
                        {
                            if (s.found)
                                return true;

                            if (recordSize > 0 && subSize % recordSize != 0)
                            {
                                layout.problem = "The SoundFont is damaged: its \"" + subId + "\" table has a part record.";
                                return false;
                            }

                            s = { subStart, subSize, true };
                            return true;
                        };

                        bool good = true;

                        if (type == "pdta")
                        {
                            if (subId == "phdr") good = take (layout.phdr, phdrSize);
                            else if (subId == "pbag") good = take (layout.pbag, pbagSize);
                            else if (subId == "pmod") good = take (layout.pmod, pmodSize);
                            else if (subId == "pgen") good = take (layout.pgen, pgenSize);
                            else if (subId == "inst") good = take (layout.inst, instSize);
                            else if (subId == "ibag") good = take (layout.ibag, ibagSize);
                            else if (subId == "imod") good = take (layout.imod, imodSize);
                            else if (subId == "igen") good = take (layout.igen, igenSize);
                            else if (subId == "shdr") good = take (layout.shdr, shdrSize);
                        }
                        else if (subId == "smpl")
                            good = take (layout.smpl, 0);
                        else if (subId == "smpo")
                            layout.hasSmpo = true;

                        if (! good)
                            return layout;

                        if (subSize % 2 != 0)
                        {
                            damaged();
                            return layout;
                        }

                        sub = subEnd;
                    }
                }
            }
            else if (size % 2 != 0)
            {
                damaged();
                return layout;
            }

            pos = dataEnd;
        }

        if (! layout.phdr.found || ! layout.pbag.found || ! layout.pmod.found || ! layout.pgen.found || ! layout.inst.found
            || ! layout.ibag.found || ! layout.imod.found || ! layout.igen.found || ! layout.shdr.found)
            layout.problem = "The SoundFont is incomplete: it lacks some of the tables that say which sounds it holds.";
        else if (layout.hasSmpo && ! layout.smpl.found)
            layout.problem = "This SoundFont keeps its recordings in a form DawIt Sampler doesn't read.";
        else if (! layout.smpl.found || layout.smpl.size < 2)
            layout.problem = "The SoundFont has no recordings in it.";

        return layout;
    }

    juce::MemoryBlock readSpan (juce::FileInputStream& in, const Span& span)
    {
        juce::MemoryBlock block;

        if (! span.found || span.size <= 0)
            return block;

        in.setPosition (span.start);
        block.setSize ((size_t) span.size);
        size_t done = 0;

        while (done < block.getSize())
        {
            const int n = in.read (static_cast<char*> (block.getData()) + done, (int) juce::jmin ((size_t) 1 << 24, block.getSize() - done));

            if (n <= 0)
                break;

            done += (size_t) n;
        }

        if (done < block.getSize())
            block.reset();

        return block;
    }

    juce::uint32 u16 (const juce::MemoryBlock& b, size_t offset)
    {
        const auto* p = static_cast<const juce::uint8*> (b.getData());
        return (juce::uint32) p[offset] | ((juce::uint32) p[offset + 1] << 8);
    }

    juce::uint32 u32 (const juce::MemoryBlock& b, size_t offset)
    {
        return u16 (b, offset) | (u16 (b, offset + 2) << 16);
    }

    juce::String readName (const juce::MemoryBlock& b, size_t offset)
    {
        const auto* p = static_cast<const char*> (b.getData()) + offset;
        char name[21] = {};

        for (int i = 0; i < 20 && p[i] != 0; ++i)
            name[i] = p[i];

        return juce::String::fromUTF8 (name).trim();
    }

    std::vector<PresetInfo> presetsFrom (const juce::MemoryBlock& phdr)
    {
        std::vector<PresetInfo> list;
        const size_t count = phdr.getSize() / phdrSize;

        for (size_t i = 0; i + 1 < count; ++i)   // the last record ends the list
            list.push_back ({ readName (phdr, i * phdrSize), (int) u16 (phdr, i * phdrSize + 22), (int) u16 (phdr, i * phdrSize + 20) });

        std::stable_sort (list.begin(), list.end(), [] (const PresetInfo& a, const PresetInfo& b)
                          { return a.bank != b.bank ? a.bank < b.bank : a.program < b.program; });
        return list;
    }

    /** The whole structure check: the layout, then every index and sample position of the tables. */
    CheckResult checkFile (const juce::File& file, juce::int64* smplBytesOut)
    {
        CheckResult result;

        if (! file.existsAsFile())
        {
            result.problem = "The file isn't there.";
            return result;
        }

        juce::FileInputStream in (file);

        if (in.failedToOpen())
        {
            result.problem = "The file can't be opened.";
            return result;
        }

        const auto layout = scan (in);

        if (layout.problem.isNotEmpty())
        {
            result.problem = layout.problem;
            return result;
        }

        const auto phdr = readSpan (in, layout.phdr), pbag = readSpan (in, layout.pbag), pgen = readSpan (in, layout.pgen),
                   inst = readSpan (in, layout.inst), ibag = readSpan (in, layout.ibag), igen = readSpan (in, layout.igen),
                   shdr = readSpan (in, layout.shdr);

        if ((phdr.isEmpty() && layout.phdr.size > 0) || (shdr.isEmpty() && layout.shdr.size > 0))
        {
            result.problem = "The file is cut short: the tables can't be read to their end.";
            return result;
        }

        const juce::int64 nPhdr = (juce::int64) phdr.getSize() / phdrSize, nPbag = (juce::int64) pbag.getSize() / pbagSize,
                          nPgen = (juce::int64) pgen.getSize() / pgenSize, nInst = (juce::int64) inst.getSize() / instSize,
                          nIbag = (juce::int64) ibag.getSize() / ibagSize, nIgen = (juce::int64) igen.getSize() / igenSize,
                          nShdr = (juce::int64) shdr.getSize() / shdrSize;
        const auto bad = [&result] (const char* what) { result.problem = what; return result; };

        if (nPhdr < 2)
            return bad ("The SoundFont has no sounds in it.");

        if (nPbag < 1 || nPgen < 1 || nInst < 1 || nIbag < 1 || nIgen < 1 || nShdr < 1)
            return bad ("The SoundFont is incomplete: one of its tables is empty.");

        // Every table points into the next: indexes never go back and never leave it. The end record of each
        // table closes the one before, so the last real entry may point at it but nothing may point past it.
        juce::int64 last = 0;

        for (juce::int64 i = 0; i < nPhdr; ++i)
        {
            const juce::int64 v = u16 (phdr, (size_t) (i * phdrSize + 24));

            if (v < last || v > nPbag - 1)
                return bad ("The SoundFont is damaged: a sound points at a part that isn't there.");

            last = v;
        }

        last = 0;

        for (juce::int64 i = 0; i < nPbag; ++i)
        {
            const juce::int64 v = u16 (pbag, (size_t) (i * pbagSize));

            if (v < last || v > nPgen)
                return bad ("The SoundFont is damaged: a sound's settings point past the end of the table.");

            last = v;
        }

        last = 0;

        for (juce::int64 i = 0; i < nInst; ++i)
        {
            const juce::int64 v = u16 (inst, (size_t) (i * instSize + 20));

            if (v < last || v > nIbag - 1)
                return bad ("The SoundFont is damaged: an instrument points at a part that isn't there.");

            last = v;
        }

        last = 0;

        for (juce::int64 i = 0; i < nIbag; ++i)
        {
            const juce::int64 v = u16 (ibag, (size_t) (i * ibagSize));

            if (v < last || v > nIgen)
                return bad ("The SoundFont is damaged: an instrument's settings point past the end of the table.");

            last = v;
        }

        for (juce::int64 i = 0; i < nPgen; ++i)
            if (u16 (pgen, (size_t) (i * pgenSize)) == 41 && (juce::int64) u16 (pgen, (size_t) (i * pgenSize + 2)) >= nInst - 1)
                return bad ("The SoundFont is damaged: a sound uses an instrument that isn't there.");

        for (juce::int64 i = 0; i < nIgen; ++i)
            if (u16 (igen, (size_t) (i * igenSize)) == 53 && (juce::int64) u16 (igen, (size_t) (i * igenSize + 2)) >= nShdr - 1)
                return bad ("The SoundFont is damaged: a zone uses a recording that isn't there.");

        // The recordings: plain 16-bit samples count in samples, Ogg Vorbis ones (.sf3) in bytes of the data.
        const juce::int64 smplBytes = layout.smpl.size, smplSamples = smplBytes / 2;
        bool anyCompressed = false, anyPlain = false;

        for (juce::int64 i = 0; i < nShdr; ++i)
        {
            const auto base = (size_t) (i * shdrSize);
            const juce::int64 start = u32 (shdr, base + 20), end = u32 (shdr, base + 24);
            const bool compressed = (u16 (shdr, base + 44) & 0x30) != 0, isEnd = i == nShdr - 1;

            if (compressed)
            {
                anyCompressed = true;

                if (start > end || end > smplBytes)
                    return bad ("The SoundFont is damaged: a recording lies outside the file's sample data.");
            }
            else if (! isEnd)
            {
                if (end > start)
                    anyPlain = true;

                if (start > end || end > smplSamples)
                    return bad ("The SoundFont is damaged: a recording lies outside the file's sample data.");
            }
        }

        if (anyCompressed && anyPlain)
            return bad ("This SoundFont mixes compressed and plain recordings, which DawIt Sampler doesn't read.");

        if (smplBytesOut != nullptr)
            *smplBytesOut = anyCompressed ? -smplBytes : smplBytes;   // negative: an .sf3

        result.ok = true;
        return result;
    }

    //==============================================================================
    // TinySoundFont reads through these.
    struct StreamState
    {
        juce::FileInputStream* in = nullptr;
        juce::int64 length = 0;
    };

    int streamRead (void* data, void* buffer, unsigned int size)
    {
        auto* s = static_cast<StreamState*> (data);
        unsigned int done = 0;

        while (done < size)
        {
            const int n = s->in->read (static_cast<char*> (buffer) + done, (int) juce::jmin (size - done, (unsigned int) (1 << 24)));

            if (n <= 0)
                break;

            done += (unsigned int) n;
        }

        return (int) done;
    }

    int streamSkip (void* data, unsigned int count)
    {
        auto* s = static_cast<StreamState*> (data);
        const auto target = s->in->getPosition() + (juce::int64) count;

        if (target > s->length)
            return 0;

        return s->in->setPosition (target) ? 1 : 0;
    }
} // namespace

//==============================================================================
juce::File getFolder()
{
    {
        const std::lock_guard<std::mutex> lock (folderMutex);

        if (folderOverride != juce::File())
            return folderOverride;
    }

    return models::getFolder().getSiblingFile ("SoundFonts");
}

void setFolderOverride (const juce::File& folder)
{
    const std::lock_guard<std::mutex> lock (folderMutex);
    folderOverride = folder;
}

Reference Reference::forFile (const juce::File& file, bool linked)
{
    Reference r;
    r.fileName = file.getFileName();
    r.size = file.getSize();

    if (linked)
        r.linkedPath = file.getFullPathName();

    return r;
}

juce::File Reference::find() const
{
    if (fileName.isEmpty())
        return {};

    const auto own = getFolder().getChildFile (fileName);

    if (own.existsAsFile() && (size <= 0 || own.getSize() == size))
        return own;

    if (linkedPath.isNotEmpty())
    {
        const juce::File linked (linkedPath);

        if (linked.existsAsFile() && (size <= 0 || linked.getSize() == size))
            return linked;
    }

    return {};
}

//==============================================================================
const models::Model& getMuseScoreGeneral()
{
    static const models::Model model { museScoreGeneralFileName,
                                       "https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General.sf3",
                                       "5b85b6c2c61d10b2b91cddd41efcce7b25cd31c8271d511c73afafbef20b6fa3",
                                       39900972,
                                       &getFolder,
                                       "the MuseScore download server" };
    return model;
}

const models::Model& getMuseScoreGeneralLicence()
{
    static const models::Model model { museScoreGeneralLicenceFileName,
                                       "https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General_License.md",
                                       "5ad8d737e13c7f01f5b9674872a82a92b4ba253603e8ed14b9db12293550b4b9",
                                       3419,
                                       &getFolder,
                                       "the MuseScore download server" };
    return model;
}

Reference getMuseScoreGeneralReference()
{
    Reference r;
    r.fileName = museScoreGeneralFileName;
    r.size = getMuseScoreGeneral().size;
    return r;
}

bool isMuseScoreGeneralInstalled()
{
    return models::isInstalled (getMuseScoreGeneral());
}

juce::File getMuseScoreGeneralLicenceFile()
{
    return models::getFile (getMuseScoreGeneralLicence());
}

juce::String downloadMuseScoreGeneral (const models::ProgressFn& progress)
{
    // The licence first: the notices must be beside the set, so a set without them is never left behind.
    if (! models::isInstalled (getMuseScoreGeneralLicence()))
        if (auto problem = models::download (getMuseScoreGeneralLicence(), [] (double) { return true; }); problem.isNotEmpty())
            return problem;

    if (models::isInstalled (getMuseScoreGeneral()))
        return {};

    return models::download (getMuseScoreGeneral(), progress);
}

//==============================================================================
bool isSoundFontFile (const juce::File& file)
{
    return file.hasFileExtension ("sf2;sf3");
}

AddResult addFile (const juce::File& chosen, bool copy)
{
    AddResult result;

    if (chosen.hasFileExtension ("sfz"))
    {
        result.problem = "DawIt Sampler plays SoundFonts (.sf2, .sf3); SFZ libraries aren't supported yet.";
        return result;
    }

    if (! isSoundFontFile (chosen))
    {
        result.problem = "DawIt Sampler plays SoundFonts (.sf2, .sf3); this isn't one.";
        return result;
    }

    if (const auto checked = check (chosen); ! checked.ok)
    {
        result.problem = checked.problem;
        return result;
    }

    const auto folder = getFolder();

    if (! copy)
    {
        // A file that already sits in the SoundFonts folder is found by name, so it isn't a link.
        const bool inFolder = chosen.getParentDirectory() == folder;
        result.reference = Reference::forFile (chosen, ! inFolder);
        result.ok = true;
        return result;
    }

    if (chosen.getParentDirectory() == folder)
    {
        result.reference = Reference::forFile (chosen, false);
        result.ok = true;
        return result;
    }

    if (! folder.createDirectory())
    {
        result.problem = "Couldn't create the folder " + folder.getFullPathName() + ".";
        return result;
    }

    auto target = folder.getChildFile (chosen.getFileName());

    // The same file already copied is used again; a different one of the same name gets a number.
    for (int n = 2; target.existsAsFile() && target.getSize() != chosen.getSize() && n < 1000; ++n)
        target = folder.getChildFile (chosen.getFileNameWithoutExtension() + " " + juce::String (n) + chosen.getFileExtension());

    if (! target.existsAsFile() && ! chosen.copyFileTo (target))
    {
        target.deleteFile();
        result.problem = "Couldn't copy the file into " + folder.getFullPathName() + ". Is the disk full?";
        return result;
    }

    result.reference = Reference::forFile (target, false);
    result.ok = true;
    return result;
}

juce::StringArray getLinked (const juce::PropertiesFile& settings)
{
    juce::StringArray paths;
    paths.addLines (settings.getValue ("soundfonts.linked"));
    paths.removeEmptyStrings();
    paths.removeDuplicates (false);
    return paths;
}

void addLinked (juce::PropertiesFile& settings, const juce::File& file)
{
    auto paths = getLinked (settings);
    paths.addIfNotAlreadyThere (file.getFullPathName());
    settings.setValue ("soundfonts.linked", paths.joinIntoString ("\n"));
}

void removeLinked (juce::PropertiesFile& settings, const juce::String& path)
{
    auto paths = getLinked (settings);
    paths.removeString (path);

    if (paths.isEmpty())
        settings.removeValue ("soundfonts.linked");
    else
        settings.setValue ("soundfonts.linked", paths.joinIntoString ("\n"));
}

std::vector<Reference> getAddedFonts (const juce::PropertiesFile* settings)
{
    std::vector<Reference> fonts;
    auto files = getFolder().findChildFiles (juce::File::findFiles, false, "*");
    files.sort();

    for (auto& file : files)
        if (isSoundFontFile (file) && file.getFileName() != museScoreGeneralFileName)
            fonts.push_back (Reference::forFile (file, false));

    if (settings != nullptr)
        for (auto& path : getLinked (*settings))
            if (const juce::File file (path); file.existsAsFile() && isSoundFontFile (file))
                fonts.push_back (Reference::forFile (file, true));

    return fonts;
}

juce::String PresetInfo::getLabel() const
{
    return juce::String (program + 1).paddedLeft ('0', 3) + " " + name;
}

CheckResult check (const juce::File& file)
{
    return checkFile (file, nullptr);
}

std::vector<PresetInfo> readPresets (const juce::File& file)
{
    juce::FileInputStream in (file);

    if (in.failedToOpen())
        return {};

    const auto layout = scan (in);

    if (layout.problem.isNotEmpty() || ! layout.phdr.found)
        return {};

    return presetsFrom (readSpan (in, layout.phdr));
}

//==============================================================================
Font::Font (const juce::File& f) : file (f), fileSize (f.getSize()) {}

Font::~Font()
{
    if (master != nullptr)
    {
        const std::lock_guard<std::mutex> lock (getCopyMutex());
        tsf_close (master);
    }
}

bool Font::waitUntilLoaded (int timeoutMs) const
{
    const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) juce::jmax (0, timeoutMs);

    while (getState() == State::loading && juce::Time::getMillisecondCounter() < end)
        juce::Thread::sleep (5);

    return getState() == State::ready;
}

juce::String Font::getProblem() const
{
    return getState() == State::failed ? problem : juce::String();
}

int Font::getPresetIndex (int bank, int program) const
{
    return getState() == State::ready ? tsf_get_presetindex (master, bank, program) : -1;
}

std::vector<Zone> Font::getZones (int bank, int program) const
{
    std::vector<Zone> zones;
    const int index = getPresetIndex (bank, program);

    for (int z = 0, n = dawit_tsf_zone_count (master, index); z < n && index >= 0; ++z)
    {
        dawit_tsf_zone c {};

        if (! dawit_tsf_get_zone (master, index, z, &c))
            continue;

        Zone zone;
        zone.lowKey = c.lowKey;
        zone.highKey = c.highKey;
        zone.lowVelocity = juce::jmax (1, c.lowVel);
        zone.highVelocity = c.highVel;
        zone.rootKey = c.rootKey;
        zone.loop = c.loopMode == 0 ? LoopMode::none : (c.loopMode == 1 ? LoopMode::continuous : LoopMode::untilRelease);
        zone.sampleStart = (int) c.sampleStart;
        zone.sampleEnd = (int) c.sampleEnd;
        zone.loopStart = (int) c.loopStart;
        zone.loopEnd = (int) c.loopEnd;
        zone.sampleRate = (int) c.sampleRate;
        zone.tuneCents = c.tuneCents;
        zone.transposeSemitones = c.transposeSemitones;
        zone.attack = c.attack;
        zone.hold = c.hold;
        zone.decay = c.decay;
        zone.sustain = c.sustain;
        zone.release = c.release;
        zones.push_back (zone);
    }

    return zones;
}

const float* Font::getSamples() const
{
    return getState() == State::ready ? dawit_tsf_get_samples (master) : nullptr;
}

::tsf* Font::makeCopy (double sampleRate, int maxVoices) const
{
    if (getState() != State::ready)
        return nullptr;

    const std::lock_guard<std::mutex> lock (getCopyMutex());
    ::tsf* copy = tsf_copy (master);

    if (copy == nullptr)
        return nullptr;

    // Voices and channel 0 are made now, so the audio thread never allocates.
    if (! tsf_set_max_voices (copy, maxVoices) || ! tsf_channel_set_pitchrange (copy, 0, 2.0f))
    {
        tsf_close (copy);
        return nullptr;
    }

    tsf_set_output (copy, TSF_STEREO_INTERLEAVED, juce::roundToInt (sampleRate > 0.0 ? sampleRate : 44100.0), 0.0f);
    ++openCopies;
    return copy;
}

void Font::closeCopy (::tsf* copy)
{
    const std::lock_guard<std::mutex> lock (getCopyMutex());
    tsf_close (copy);
    --openCopies;
}

int Font::getOpenCopyCount() noexcept
{
    return openCopies.load();
}

void Font::load()
{
    juce::int64 smplBytes = 0;
    const auto checked = checkFile (file, &smplBytes);

    if (! checked.ok)
    {
        problem = checked.problem;
        state.store ((int) State::failed, std::memory_order_release);
        return;
    }

    juce::FileInputStream in (file);
    StreamState stream { &in, in.getTotalLength() };
    tsf_stream callbacks { &stream, streamRead, streamSkip };
    ::tsf* loaded = tsf_load (&callbacks);

    if (loaded == nullptr)
    {
        problem = "DawIt couldn't read this SoundFont: it may be damaged, or there isn't enough memory to hold it.";
        state.store ((int) State::failed, std::memory_order_release);
        return;
    }

    if (smplBytes < 0)
        sampleCount = (juce::int64) dawit_tsf_get_used_sample_count (loaded);
    else
        sampleCount = smplBytes / 2;

    juce::StringArray names;

    for (int i = 0, n = tsf_get_presetcount (loaded); i < n; ++i)
    {
        PresetInfo info;
        info.name = juce::String::fromUTF8 (tsf_get_presetname (loaded, i)).trim();
        dawit_tsf_get_preset_numbers (loaded, i, &info.bank, &info.program);
        presets.push_back (info);
    }

    master = loaded;
    state.store ((int) State::ready, std::memory_order_release);
}

//==============================================================================
struct Library::Impl
{
    struct LoadJob final : public juce::ThreadPoolJob
    {
        explicit LoadJob (std::shared_ptr<Font> f) : juce::ThreadPoolJob ("Loading a SoundFont"), font (std::move (f)) {}

        JobStatus runJob() override
        {
            font->load();
            font.reset();   // this share goes now; the last one frees the font on the message thread
            return jobHasFinished;
        }

        std::shared_ptr<Font> font;
    };

    Impl() : pool (1) {}

    std::mutex mutex;
    std::map<juce::String, std::weak_ptr<Font>> fonts;
    juce::ThreadPool pool;
};

Library::Library() : impl (std::make_unique<Impl>()) {}

Library::~Library()
{
    impl->pool.removeAllJobs (true, 60000);
}

Library& Library::get()
{
    static Library library;
    return library;
}

std::shared_ptr<Font> Library::acquire (const juce::File& file)
{
    const auto key = file.getFullPathName() + "|" + juce::String (file.getSize()) + "|" + juce::String (file.getLastModificationTime().toMilliseconds());
    const std::lock_guard<std::mutex> lock (impl->mutex);

    for (auto it = impl->fonts.begin(); it != impl->fonts.end();)
    {
        if (auto existing = it->second.lock())
        {
            if (it->first == key)
                return existing;

            ++it;
        }
        else
            it = impl->fonts.erase (it);
    }

    // Freed on the message thread, whichever thread lets go of the last share.
    auto deleter = [] (Font* f)
    {
        auto* manager = juce::MessageManager::getInstanceWithoutCreating();

        if (manager == nullptr || manager->isThisTheMessageThread())
            delete f;
        else
            juce::MessageManager::callAsync ([f] { delete f; });
    };

    std::shared_ptr<Font> font (new Font (file), deleter);
    impl->fonts[key] = font;
    ++loads;
    impl->pool.addJob (new Impl::LoadJob (font), true);
    return font;
}

//==============================================================================
namespace testfont
{
    namespace
    {
        struct Sample
        {
            std::vector<float> data;
            int rootKey = 60;
            int loopStart = 0, loopEnd = 0;   // relative to the sample's start; 0 and 0: none
        };

        constexpr double rate = 44100.0;

        std::vector<float> makeSine (int length, int periodSamples, double amplitude, double secondHarmonic = 0.0)
        {
            std::vector<float> s ((size_t) length);

            for (int i = 0; i < length; ++i)
            {
                const double phase = juce::MathConstants<double>::twoPi * i / periodSamples;
                s[(size_t) i] = (float) (amplitude * std::sin (phase) + secondHarmonic * std::sin (2.0 * phase));
            }

            return s;
        }

        std::vector<Sample> makeSamples()
        {
            std::vector<Sample> samples;
            constexpr int half = 22000;   // 0.5 s: 220 periods of the 441 Hz sine, 440 of the 882 Hz one

            samples.push_back ({ makeSine (half, 100, 0.5), 69, 10000, 20000 });    // 0: 441 Hz, loops over 100 periods
            samples.push_back ({ makeSine (half, 50, 0.5), 81, 10000, 15000 });     // 1: 882 Hz
            samples.push_back ({ makeSine (half, 100, 0.4), 69, 0, 0 });            // 2: the soft layer: a pure sine
            samples.push_back ({ makeSine (half, 100, 0.4, 0.2), 69, 0, 0 });       // 3: the loud layer: its 2nd harmonic at -6 dB

            Sample kick;
            kick.rootKey = 36;

            for (int i = 0; i < 13230; ++i)
            {
                const double t = i / rate;
                kick.data.push_back ((float) (0.8 * std::exp (-12.0 * t) * std::sin (juce::MathConstants<double>::twoPi * 80.0 * t)));
            }

            samples.push_back (kick);                                               // 4: the kick

            Sample snare;
            snare.rootKey = 38;
            juce::uint32 seed = 12345;

            for (int i = 0; i < 8820; ++i)
            {
                seed = seed * 1664525u + 1013904223u;
                const double noise = ((seed >> 8) & 0xffff) / 32768.0 - 1.0;
                snare.data.push_back ((float) (0.6 * std::exp (-20.0 * (i / rate)) * noise));
            }

            samples.push_back (snare);                                              // 5: the snare
            return samples;
        }

        void writeName (juce::MemoryOutputStream& out, const juce::String& name)
        {
            char field[20] = {};
            name.copyToUTF8 (field, 19);
            out.write (field, 20);
        }

        void writeChunk (juce::MemoryOutputStream& out, const char* id, const juce::MemoryBlock& data)
        {
            out.write (id, 4);
            out.writeInt ((int) data.getSize());
            out.write (data.getData(), data.getSize());

            if (data.getSize() % 2 != 0)
                out.writeByte (0);
        }

        juce::MemoryBlock list (const char* type, const juce::MemoryBlock& content)
        {
            juce::MemoryOutputStream out;
            out.write (type, 4);
            out.write (content.getData(), content.getSize());
            return out.getMemoryBlock();
        }

        juce::MemoryBlock encodeVorbis (const std::vector<float>& data)
        {
            juce::MemoryBlock encoded;
            juce::OggVorbisAudioFormat format;
            std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (encoded, false);

            if (auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions {}
                                                                 .withSampleRate (rate)
                                                                 .withNumChannels (1)
                                                                 .withBitsPerSample (16)
                                                                 .withQualityOptionIndex (format.getQualityOptions().size() - 1)))
            {
                juce::AudioBuffer<float> buffer (1, (int) data.size());
                buffer.copyFrom (0, 0, data.data(), (int) data.size());
                writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
            }

            return encoded;
        }

        /** The generators of one zone: a key range, perhaps a velocity range and a loop mode, then the sample. */
        struct ZoneSpec
        {
            int lowKey, highKey, lowVelocity, highVelocity, sample, sampleModes;
        };

        struct InstrumentSpec
        {
            const char* name;
            std::vector<ZoneSpec> zones;
        };

        struct PresetSpec
        {
            const char* name;
            int bank, program, instrument;
        };
    } // namespace

    juce::MemoryBlock make (bool compressed)
    {
        const auto samples = makeSamples();

        const std::vector<InstrumentSpec> instruments {
            { "Test Piano", { { 0, 71, 0, 127, 0, 0 }, { 72, 127, 0, 127, 1, 0 } } },
            { "Test Layers", { { 0, 127, 0, 63, 2, 0 }, { 0, 127, 64, 127, 3, 0 } } },
            { "Test Organ", { { 0, 127, 0, 127, 0, 3 } } },
            { "Test Pluck", { { 0, 127, 0, 127, 0, 0 } } },
            { "Test Kit", { { 36, 36, 0, 127, 4, 0 }, { 38, 38, 0, 127, 5, 0 } } }
        };

        const std::vector<PresetSpec> presets {
            { "Test Piano", 0, pianoProgram, 0 }, { "Test Layers", 0, layersProgram, 1 }, { "Test Organ", 0, organProgram, 2 },
            { "Test Pluck", 0, pluckProgram, 3 }, { "Test Kit", drumBank, kitProgram, 4 }
        };

        // The recordings, and where each one starts and ends in the sample data.
        juce::MemoryOutputStream smpl;
        std::vector<juce::uint32> starts, ends;

        for (auto& s : samples)
        {
            if (compressed)
            {
                const auto bytes = encodeVorbis (s.data);
                starts.push_back ((juce::uint32) smpl.getDataSize());
                smpl.write (bytes.getData(), bytes.getSize());
                ends.push_back ((juce::uint32) smpl.getDataSize());
            }
            else
            {
                starts.push_back ((juce::uint32) (smpl.getDataSize() / 2));

                for (float v : s.data)
                    smpl.writeShort ((short) juce::roundToInt (v * 32767.0f));

                ends.push_back ((juce::uint32) (smpl.getDataSize() / 2));

                for (int i = 0; i < 46; ++i)   // the format asks for 46 silent samples after each recording
                    smpl.writeShort (0);
            }
        }

        if (smpl.getDataSize() % 2 != 0)
            smpl.writeByte (0);

        // The tables.
        juce::MemoryOutputStream phdr, pbag, pmod, pgen, inst, ibag, imod, igen, shdr;
        int presetGens = 0, instrumentBags = 0, instrumentGens = 0;

        for (size_t p = 0; p < presets.size(); ++p)
        {
            writeName (phdr, presets[p].name);
            phdr.writeShort ((short) presets[p].program);
            phdr.writeShort ((short) presets[p].bank);
            phdr.writeShort ((short) p);   // its one bag
            phdr.writeInt (0), phdr.writeInt (0), phdr.writeInt (0);

            pbag.writeShort ((short) presetGens);
            pbag.writeShort (0);
            pgen.writeShort (41);   // instrument
            pgen.writeShort ((short) presets[p].instrument);
            ++presetGens;
        }

        writeName (phdr, "EOP");
        phdr.writeShort (0), phdr.writeShort (0), phdr.writeShort ((short) presets.size());
        phdr.writeInt (0), phdr.writeInt (0), phdr.writeInt (0);
        pbag.writeShort ((short) presetGens);
        pbag.writeShort (0);
        pgen.writeShort (0), pgen.writeShort (0);
        pmod.writeRepeatedByte (0, pmodSize);

        for (auto& instrument : instruments)
        {
            writeName (inst, instrument.name);
            inst.writeShort ((short) instrumentBags);

            for (auto& z : instrument.zones)
            {
                ibag.writeShort ((short) instrumentGens);
                ibag.writeShort (0);
                ++instrumentBags;

                auto generator = [&] (int op, int amount) { igen.writeShort ((short) op), igen.writeShort ((short) amount), ++instrumentGens; };
                generator (43, z.lowKey | (z.highKey << 8));                                  // key range, always first
                generator (44, z.lowVelocity | (z.highVelocity << 8));                        // then the velocity range
                generator (54, z.sampleModes);                                                // 3: loop while the key is held
                generator (53, z.sample);                                                     // the sample, always last
            }
        }

        writeName (inst, "EOI");
        inst.writeShort ((short) instrumentBags);
        ibag.writeShort ((short) instrumentGens);
        ibag.writeShort (0);
        igen.writeShort (0), igen.writeShort (0);
        imod.writeRepeatedByte (0, imodSize);

        for (size_t i = 0; i < samples.size(); ++i)
        {
            auto& s = samples[i];
            writeName (shdr, "Sample " + juce::String (i));
            shdr.writeInt ((int) starts[i]);
            shdr.writeInt ((int) ends[i]);
            // The loop points are positions in the sample data for an .sf2, and relative to the recording for an .sf3.
            shdr.writeInt ((int) (compressed ? (juce::uint32) s.loopStart : starts[i] + (juce::uint32) s.loopStart));
            shdr.writeInt ((int) (compressed ? (juce::uint32) s.loopEnd : starts[i] + (juce::uint32) s.loopEnd));
            shdr.writeInt ((int) rate);
            shdr.writeByte ((char) s.rootKey);
            shdr.writeByte (0);
            shdr.writeShort (0);                       // no linked sample
            shdr.writeShort (compressed ? 0x11 : 1);   // mono, and Ogg Vorbis for an .sf3
        }

        writeName (shdr, "EOS");
        shdr.writeRepeatedByte (0, shdrSize - 20);

        juce::MemoryOutputStream info;
        juce::MemoryOutputStream version;
        version.writeShort (compressed ? 3 : 2), version.writeShort (compressed ? 0 : 1);
        writeChunk (info, "ifil", version.getMemoryBlock());
        juce::MemoryBlock engineName ("EMU8000", 8);
        writeChunk (info, "isng", engineName);
        juce::MemoryBlock fontName ("DawIt test font", 16);
        writeChunk (info, "INAM", fontName);

        juce::MemoryOutputStream sdta, pdta, content;
        writeChunk (sdta, "smpl", smpl.getMemoryBlock());
        writeChunk (pdta, "phdr", phdr.getMemoryBlock());
        writeChunk (pdta, "pbag", pbag.getMemoryBlock());
        writeChunk (pdta, "pmod", pmod.getMemoryBlock());
        writeChunk (pdta, "pgen", pgen.getMemoryBlock());
        writeChunk (pdta, "inst", inst.getMemoryBlock());
        writeChunk (pdta, "ibag", ibag.getMemoryBlock());
        writeChunk (pdta, "imod", imod.getMemoryBlock());
        writeChunk (pdta, "igen", igen.getMemoryBlock());
        writeChunk (pdta, "shdr", shdr.getMemoryBlock());

        content.write ("sfbk", 4);
        writeChunk (content, "LIST", list ("INFO", info.getMemoryBlock()));
        writeChunk (content, "LIST", list ("sdta", sdta.getMemoryBlock()));
        writeChunk (content, "LIST", list ("pdta", pdta.getMemoryBlock()));

        juce::MemoryOutputStream file;
        writeChunk (file, "RIFF", content.getMemoryBlock());
        return file.getMemoryBlock();
    }

    bool write (const juce::File& file, bool compressed)
    {
        const auto bytes = make (compressed);
        file.getParentDirectory().createDirectory();
        return file.replaceWithData (bytes.getData(), bytes.getSize());
    }
} // namespace testfont
} // namespace daw::soundfonts
