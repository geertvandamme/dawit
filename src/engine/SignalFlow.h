#pragma once

#include "Routing.h"

namespace daw
{
/**
    The signal flow of a whole song as plain data: every track's source, its effects and fader, the buses and
    returns (each starting with a sum), the sends and the master, with the live levels at every step and where each
    box sits in a left-to-right layout. The Signal Flow window and the routing guide's picture draw it; the tests check it.

    Building a graph only reads the song: it takes the tracks as `const` and reads atomics, so a view that draws it
    can never change the edit. Everything here runs on the message thread.
*/
namespace signalflow
{
    enum class NodeKind { source, effect, fader, sum, master };

    /** What a source box stands for: a track's audio clips, DawIt Synth, or an instrument plugin. */
    enum class SourceKind { audio, builtInSynth, instrument };

    /** One track, bus or return of the song (the master isn't one). */
    struct TrackInfo
    {
        juce::String name;
        juce::Colour colour;
        bool isBus = false, isReturn = false;
        int parent = -1;                       // index of its bus in `tracks`, -1: the master
        const Track* track = nullptr;          // the song's track (null in the example); only for identifying it
        routing::Silence silence = routing::Silence::none;
        bool muted = false;
        bool lit = true;                       // on the selected track's path (always true without a selection)
        float row = 0.0f;                      // from layOut: the row in grid units
    };

    /** One box. */
    struct Node
    {
        NodeKind kind = NodeKind::source;
        int trackIndex = -1;                   // index in `tracks`; -1 for the master
        int slot = -1;                         // the plugin slot of an effect or an instrument source; -1 otherwise
        SourceKind sourceKind = SourceKind::audio;
        juce::String label;                    // an effect's or an instrument's name ("DawIt Compressor", "DawIt Synth")
        juce::String detail;                   // DawIt Synth: the name of its sound ("Bass")
        juce::String format;                   // a plugin's format ("VST3", "AudioUnit"); empty for DawIt's own
        bool bypassed = false;
        bool silenced = false;                 // the sound isn't heard after this track's fader
        bool lit = true;
        float levelDb = -100.0f;               // the level coming out of the box, from the meters
        float inputDb = -100.0f;               // an effect's input level (the level of the box before it)
        float volumeDb = 0.0f, pan = 0.0f;     // a fader's settings
        bool muted = false;                    // a fader: its track is muted
        juce::StringArray feeders;             // a sum: the names of the tracks (or senders) that add up here
        float levelLeft = 0.0f, levelRight = 0.0f;   // the master: its meter levels (linear)
        float masterVolumeDb = 0.0f;           // the master: its volume
        int column = 0;                        // from layOut
    };

    /** One line: the flow from a box to the next, or a send. */
    struct Edge
    {
        int from = 0, to = 0;
        bool isSend = false;
        float levelDb = -100.0f;               // the level on the line (a send: what it carries now)
        float sendDb = 0.0f;                   // a send: its level setting
        bool preFader = false;                 // a send: taken before the fader
        bool silenced = false;                 // dark: muted, or not heard while something else is soloed
        bool lit = true;
    };

    struct Graph
    {
        std::vector<TrackInfo> tracks;
        std::vector<Node> nodes;
        std::vector<Edge> edges;
        int master = -1;                       // index of the master node
        bool hasSelection = false;
        float masterRow = 0.0f;

        /** How many boxes are on the selected track's path. */
        int getNumLit() const;

        /** The first box of the track (its source, or the sum of a bus or a return); -1 when it has none. */
        int getFirstNode (int trackIndex) const;

        /** The track's fader node, or -1. */
        int getFader (int trackIndex) const;

        /** The track's sum node (a bus or a return), or -1. */
        int getSum (int trackIndex) const;
    };

    /** The master's meters, read by the caller from the engine. */
    struct MasterState
    {
        float peakLeft = 0.0f, peakRight = 0.0f;   // linear, after the master volume
        float volumeDb = 0.0f;
    };

    /** A linear level as dB, -100 for silence. */
    float toDb (float linear);

    /** Builds the graph of the song, with the levels from the meters, the selected track's path lit (null: none)
        and the layout done. Reads only: nothing in the song changes. */
    Graph build (const Tracks&, const Track* selected, const MasterState&);

    /** The routing guide's example: Kick and Snare in the Drums bus, Bass, and Vocal sending to a Reverb return,
        with the Vocal selected and made-up levels. */
    Graph getExample();

    /** Gives every node its column (the longest path from the left; the master one past the last) and every track
        its row (tracks one per row in order, a bus at the middle of its members, returns below, the master in the middle). */
    void layOut (Graph&);
} // namespace signalflow
} // namespace daw
