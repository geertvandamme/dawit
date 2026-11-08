// TinySoundFont (MIT, Bernhard Schelling, based on SFZero) and stb_vorbis (MIT, Sean Barrett) compiled
// together in one translation unit, so tsf.h can see stb_vorbis for .sf3 files. Pinned in CMakeLists.txt;
// warnings are silenced for this file there. See licenses/ for both notices.

#include "stb_vorbis.c"

#define TSF_IMPLEMENTATION
#include <tsf.h>

#include "SoundFontCore.h"

extern "C"
{
int dawit_tsf_zone_count (const tsf* font, int presetIndex)
{
    if (font == nullptr || presetIndex < 0 || presetIndex >= font->presetNum)
        return 0;

    return font->presets[presetIndex].regionNum;
}

int dawit_tsf_get_zone (const tsf* font, int presetIndex, int zoneIndex, dawit_tsf_zone* out)
{
    if (out == nullptr || zoneIndex < 0 || zoneIndex >= dawit_tsf_zone_count (font, presetIndex))
        return 0;

    const tsf_region& r = font->presets[presetIndex].regions[zoneIndex];
    out->lowKey = r.lokey;
    out->highKey = r.hikey;
    out->lowVel = r.lovel;
    out->highVel = r.hivel;
    out->rootKey = r.pitch_keycenter;
    out->loopMode = r.loop_mode == TSF_LOOPMODE_NONE ? 0 : (r.loop_mode == TSF_LOOPMODE_CONTINUOUS ? 1 : 2);
    out->sampleStart = r.offset;
    out->sampleEnd = r.end;
    out->loopStart = r.loop_start;
    out->loopEnd = r.loop_end;
    out->sampleRate = r.sample_rate;
    out->tuneCents = r.tune;
    out->transposeSemitones = r.transpose;
    out->attack = r.ampenv.attack;
    out->hold = r.ampenv.hold;
    out->decay = r.ampenv.decay;
    out->sustain = r.ampenv.sustain;
    out->release = r.ampenv.release;
    return 1;
}

int dawit_tsf_get_preset_numbers (const tsf* font, int presetIndex, int* bank, int* program)
{
    if (font == nullptr || presetIndex < 0 || presetIndex >= font->presetNum)
        return 0;

    if (bank != nullptr)
        *bank = font->presets[presetIndex].bank;

    if (program != nullptr)
        *program = font->presets[presetIndex].preset;

    return 1;
}

const float* dawit_tsf_get_samples (const tsf* font)
{
    return font != nullptr ? font->fontSamples : nullptr;
}

unsigned dawit_tsf_get_used_sample_count (const tsf* font)
{
    unsigned most = 0;

    if (font == nullptr)
        return 0;

    for (int p = 0; p < font->presetNum; ++p)
        for (int z = 0; z < font->presets[p].regionNum; ++z)
            if (font->presets[p].regions[z].end > most)
                most = font->presets[p].regions[z].end;

    return most;
}
}
