#pragma once

#include "CompressorExplainer.h"
#include "EffectWindow.h"

/**
    "What is compression?", the guide to compressors for beginners, opened from DawIt Compressor's window and the Theory
    Guides window: what a compressor does, threshold and ratio, the knee, attack, release, makeup, peak and RMS, the
    sidechain filter, parallel compression, how much to use, too much, and how DawIt Compressor works. The pictures run
    made-up vocal and drum levels through the compressor's own curve and timing, with the settings in its window.
*/
namespace daw::comptext
{
/** A made-up vocal line, 4 seconds of syllables at different levels, as a level in dB per millisecond. */
std::vector<float> makeVocalLevels();

/** Made-up drum hits, one every half second, each falling from -3 dB, as a level in dB per millisecond. */
std::vector<float> makeDrumLevels (double seconds = 2.0);

/** What comes out for `levelsDb` (one per millisecond) with these settings, makeup and mix included, in dB. */
std::vector<float> getOutputLevels (const comp::Settings&, const std::vector<float>& levelsDb);

/** The pages of "What is compression?". */
std::vector<fxui::Guide::Page> makeGuide (BuiltInCompressor&);
} // namespace daw::comptext
