#pragma once

#include "BuiltInSynth.h"
#include "SoundFonts.h"

#include <juce_core/juce_core.h>

#include <vector>

namespace daw
{
/**
    The "What is a sampler?" guide: what a sampler is, root notes and speed, key zones, velocity layers, loops,
    SoundFonts, General MIDI, memory, sampled against synthesised, and a look at the track's own notes against the
    zones of its preset. The content only: the window is `SamplerExplainer` in the UI. The numbers behind the pictures
    (speed factors, memory, the mappings to and from the DawIt Synth's sounds, the General MIDI families) and the
    advice rules are checked in the tests.
*/
namespace samplerguide
{
    /** The pages, in the order of the guide. `sources` has no text here: the guide shows SOURCES.md. */
    enum class Page { recordings, pitch, zones, velocity, loops, soundFont, generalMidi, memory, sampledOrSynth, yourTrack, sources };
    inline constexpr Page allPages[] { Page::recordings, Page::pitch,       Page::zones,          Page::velocity,       Page::loops,       Page::soundFont,
                                       Page::generalMidi, Page::memory,     Page::sampledOrSynth, Page::yourTrack,      Page::sources };

    juce::String getPageName (Page);
    juce::String getPageSummary (Page);

    // ---- Your track --------------------------------------------------------------------------------------------

    /** One note of the track. */
    struct Note
    {
        int key = 60;        // 0 to 127, after the sampler's Transpose
        int velocity = 100;  // 1 to 127
    };

    /** What the track's sampler is doing, and the notes it plays. */
    struct TrackView
    {
        enum class Font { ready, missing, loading };

        Font font = Font::ready;
        juce::String fontName;                               // the SoundFont's file name
        juce::String presetName;                             // "Acoustic Grand Piano"
        bool isDrumKit = false;                              // bank 128: layers and roots mean little there
        BuiltInSound fallbackSound = BuiltInSound::piano;    // what plays while the font is missing
        std::vector<soundfonts::Zone> zones;                 // of the preset (empty while the font isn't ready)
        std::vector<Note> notes;
    };

    /** The lines for the "Your track" page, each a paragraph. A missing font or a loading one gives one line and nothing
        else; no notes gives one line; otherwise a line for each of: notes outside every zone, every note in one
        velocity layer (8 or more notes, a preset with layers), a note 12 or more semitones from its zone's root;
        and when none of them applies, one line saying all is well. */
    juce::StringArray analyse (const TrackView&);

    /** The text of a page: its paragraphs, the last one saying what the picture shows. "Your track" is the lines of
        `analyse`, or with no track (null) how to get something to look at; "Sources" is empty. */
    juce::String explain (Page, const TrackView* = nullptr);

    // ---- General MIDI ------------------------------------------------------------------------------------------

    constexpr int numFamilies = 16;

    /** "Piano", "Chromatic Percussion", ... "Sound Effects": family 0 to 15. */
    juce::String getFamilyName (int family);

    /** The programs of a family as shown, 1 to 128: family 0 is 1 to 8, family 15 is 121 to 128. */
    int getFamilyFirstProgram (int family);
    int getFamilyLastProgram (int family);

    /** "Piano 1-8". */
    juce::String getFamilyRange (int family);

    /** The family of a program as shown (1 to 128): program 34 is family 4, Bass. */
    int getFamilyOfShownProgram (int shownProgram);

    // ---- Sound mappings ----------------------------------------------------------------------------------------

    /** The preset "Use a sampled sound" picks for a DawIt Synth sound, and the sound that plays while it isn't there. The
        second takes the first back: for every sound but percussion (which comes back as drums) it is the same sound. */
    struct SampledPreset
    {
        int bank = 0;
        int shownProgram = 1;   // 1 to 128
    };
    SampledPreset getSampledPreset (BuiltInSound);
    BuiltInSound getFallbackSound (const SampledPreset&);

    /** A starting point of the sampler's window, on MuseScore General. */
    struct StartingPoint
    {
        juce::String name;
        int bank = 0;
        int shownProgram = 1;   // 1 to 128; the program stored is one less
        juce::String why;
    };
    const std::vector<StartingPoint>& getStartingPoints();

    // ---- The numbers behind the pictures -----------------------------------------------------------------------

    /** How much faster a recording plays when moved `semitones` up: 2^(n/12). +12 is 2, -12 is 0.5. */
    double getSpeed (int semitones);

    /** How long it lasts compared with at its root: the inverse of the speed. */
    double getLengthFactor (int semitones);

    /** The intervals the speed picture offers: -12, -7, -5, +5, +7, +12. */
    const std::vector<int>& getPictureIntervals();

    /** Bytes of one minute of 16-bit mono audio at a sample rate: 44,100 x 2 x 60 = 5,292,000. */
    juce::int64 getBytesPerMinuteMono16 (int sampleRate = 44100);

    /** What DawIt Sampler holds in memory for an .sf2 of this many bytes: twice that, as it keeps 32-bit floats. */
    juce::int64 getMemoryBytes (juce::int64 sf2Bytes);

    /** The sizes of MuseScore General in MB (1,000,000 bytes, as DawIt shows sizes): 40 as .sf3, 216 as .sf2, and about 432 held in memory. */
    constexpr int museScoreSf3Mb = 40, museScoreSf2Mb = 216, museScoreMemoryMb = 2 * museScoreSf2Mb;

    /** "C4" for note 60 and "F#2" for note 42 (C4 is middle C). */
    juce::String getNoteName (int key);
} // namespace samplerguide
} // namespace daw
