#pragma once

#include "engine/Engine.h"

/**
    The chord sheet of a key as a PDF, for piano or guitar: the key's seven chords with their
    numerals and families, its sevenths, the chords songs in it often borrow, and its well-known
    progressions, each drawn as a keyboard (with the inversions spelled out) or a guitar chord box
    (with the fingers, the tab and how hard it is, and the capo that makes a hard key easy).
    A sheet to keep on the music stand.
*/
namespace daw::chordsheet
{
enum class Instrument { piano, guitar };

/** The sheet's A4 pages, drawn at this many pixels per point (200 dpi by default). */
std::vector<juce::Image> renderPages (const MusicalKey&, Instrument, float pixelsPerPoint = 200.0f / 72.0f);

/** Writes the sheet for piano, guitar or both (piano first) into one PDF. */
juce::Result exportPdf (const MusicalKey&, bool piano, bool guitar, const juce::File&);

/** "Chords in A minor (guitar)": the sheet's title and file name. */
juce::String getTitle (const MusicalKey&, bool piano, bool guitar);

/** Asks for the key (starting at `key`) and the instrument, then where to save the PDF. */
void showDialog (Engine&, const MusicalKey& key);

//==============================================================================
/** A song's chord track and what the sheet says about it. */
struct SongChords
{
    juce::String title;
    MusicalKey key;
    double bpm = 120.0;
    ChordMap chords;
    SectionList sections;
};

/** The song's chord sheet: the chords it uses, each once, then bar by bar under its sections, each
    chord voiced near the one before as on the chord track. */
std::vector<juce::Image> renderSongPages (const SongChords&, Instrument, float pixelsPerPoint = 200.0f / 72.0f);

juce::Result exportSongPdf (const SongChords&, Instrument, const juce::File&);

/** The open chord track's "PDF...": asks where to save the song's chords, for the instrument the
    track shows, and writes them; says so when the chord track is empty. */
void exportSong (Engine&, const juce::String& projectName, Instrument);
} // namespace daw::chordsheet
