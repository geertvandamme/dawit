#pragma once

/**
    The C side of DawIt Sampler's SoundFont player: TinySoundFont's own functions (tsf.h) and a few
    read-only accessors for what its `struct tsf` keeps private, compiled in SoundFontCore.cpp. Only
    SoundFonts.cpp and BuiltInSampler.cpp include this; everything else uses `soundfonts::` and
    `BuiltInSampler`.
*/

#include <tsf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** One zone (a "region" in TinySoundFont) of a preset after its generators are merged: the keys and
    velocities it answers to, its recording's root note, loop and envelope. */
typedef struct dawit_tsf_zone
{
    int lowKey, highKey, lowVel, highVel;
    int rootKey;                      /* the note the recording was made at */
    int loopMode;                     /* 0 plays once, 1 loops while the note sounds, 2 loops while the key is held */
    unsigned sampleStart, sampleEnd;  /* in decoded samples, in the font's sample memory */
    unsigned loopStart, loopEnd;
    unsigned sampleRate;
    int tuneCents, transposeSemitones;
    float attack, hold, decay, sustain, release;   /* seconds; sustain is a level, 0 to 1 */
} dawit_tsf_zone;

/** How many zones the preset (by index) has; 0 when the index is out of range. */
int dawit_tsf_zone_count (const tsf* font, int presetIndex);

/** Fills `out` with a zone of a preset; returns 0 when either index is out of range. */
int dawit_tsf_get_zone (const tsf* font, int presetIndex, int zoneIndex, dawit_tsf_zone* out);

/** The bank and program (0 to 127) of a preset by index; returns 0 when the index is out of range. */
int dawit_tsf_get_preset_numbers (const tsf* font, int presetIndex, int* bank, int* program);

/** The decoded samples (32-bit floats), shared by every copy of the font; null when the font is null. */
const float* dawit_tsf_get_samples (const tsf* font);

/** The highest sample position any zone ends at: the number of decoded samples the presets use. */
unsigned dawit_tsf_get_used_sample_count (const tsf* font);

#ifdef __cplusplus
}
#endif
