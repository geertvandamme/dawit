#pragma once

#include <vector>
#include <juce_graphics/juce_graphics.h>

#include "Bounce.h"

namespace daw
{
/**
    The "Sending stems to an engineer" guide: stems, multitracks and the stereo mix, what a mixing and a mastering
    engineer each want, preparing the files (same start and length, 24-bit, the project's rate, no dither, no
    normalising), headroom, printed or dry effects, checking that the files add up, what to send with them,
    revisions, and a look at the user's own song as the files Bounce to WAV would make. The content only: the window
    is HandoverExplainer. The sizes, the turn-down and the advice rules are checked in the tests.
*/
namespace handoverguide
{
    enum class Topic { stems, mixOrMaster, preparing, headroom, effects, checking, sendWith, working, yourSong };
    inline constexpr Topic allTopics[] { Topic::stems, Topic::mixOrMaster, Topic::preparing, Topic::headroom, Topic::effects,
                                         Topic::checking, Topic::sendWith, Topic::working, Topic::yourSong };

    juce::String getTopicName (Topic);
    juce::String getTopicSummary (Topic);

    /** The general text of a topic. For "your song" this is the part that doesn't depend on the song. */
    juce::String explain (Topic);

    // ---- The numbers ------------------------------------------------------------------------------------------

    /** Bytes of a stereo WAV file of `seconds`: whole frames (rate x seconds, rounded) x 2 channels x bytes per sample
        (16-bit 2, 24-bit 3, 32 means 32-bit float: 4). 48 kHz, 24-bit, 214 s: 61,632,000. */
    juce::int64 getBytesPerFile (double sampleRate, int bits, double seconds);

    /** How far to turn the master volume down so a mix peaking at `peakDb` peaks at -6 dBFS: peakDb + 6. -1.2: 4.8. */
    float getTurnDownDb (float peakDb);

    /** True for the names DawIt gives a new track or bus and nobody changed: "Audio 3", "MIDI 12", "Bus 2" (the word,
        a space, and digits only). "Audio Lead", "Audio 3b" and "Bass" are not. */
    bool isGenericName (const juce::String&);

    /** "3:34" for 214 seconds (rounded to the second). */
    juce::String describeSeconds (double seconds);

    /** "48 kHz", "44.1 kHz". */
    juce::String describeRate (double sampleRate);

    // ---- Example data for the pictures ------------------------------------------------------------------------

    /** One stem of the example song: the tracks in it and where it plays, in bars as the ruler shows them. */
    struct ExampleStem
    {
        juce::String name;
        juce::StringArray tracks;                       // the tracks in it; empty for a lone track
        std::vector<juce::Range<int>> bars;             // first and last bar of each stretch, both included
        juce::String fileName;                          // "01 Drums.wav"
    };

    /** The pop song of the routing guide as 5 stems (Drums, Bass, Guitars, Keys, Vocals) of 13 tracks, 105 bars. */
    std::vector<ExampleStem> getExampleStems();
    inline constexpr int exampleNumBars = 105, exampleNumTracks = 13;

    /** The example's seconds per file: 105 bars of 4/4 at 120 BPM is 210 s, plus the 4 s tail: 214. */
    double getExampleSecondsPerFile();

    // ---- Your song --------------------------------------------------------------------------------------------

    /** What the page learns from outside the engine: the Bounce window's remembered choices and the last
        Measure Loudness. */
    struct Inputs
    {
        double bounceRate = 48000.0;        // export.bounceRate
        int bounceBits = 24;                // export.bounceBits
        bool hasPeak = false;               // a Measure Loudness result exists
        float peakDb = 0.0f;                // its sample peak, dBFS
        juce::String peakScope;             // where it measured, in plain words: "bar 1 to bar 105"
    };

    /** One stem of the song as a lane. */
    struct Lane
    {
        juce::String name;
        juce::Colour colour;
        juce::StringArray tracks;                       // the tracks inside a bus; empty for a lone track
        std::vector<juce::Range<double>> ranges;        // beats where an unmuted clip plays, merged when they touch
        juce::String fileName;                          // "01 Drums.wav"
        const Track* track = nullptr;                   // the bus or track
    };

    /** The song as the files Bounce to WAV would make, and what to do about them. */
    struct SongView
    {
        std::vector<Lane> lanes;                        // the stems, in edit order
        std::vector<bounce::SkippedItem> skipped;       // tracks and buses left out, and why
        int numStems = 0, numMultitracks = 0;
        double songBeats = 0.0;                         // the song range, in beats
        double secondsPerFile = 0.0;                    // the song range in seconds plus bounce::filesTailSeconds
        juce::int64 bytesPerFile = 0, bytesStems = 0, bytesMultitracks = 0;
        juce::StringArray advice;                       // the ten rules, in order, only those that apply
    };

    /** Works out the view on the message thread, with `bounce::planFiles` over `bounce::getSongRange` so it always
        agrees with Bounce to WAV. An empty song gives one line: "Your song has no clips to bounce yet." */
    SongView analyse (const Engine&, const Inputs&);
} // namespace handoverguide
} // namespace daw
