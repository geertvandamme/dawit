#pragma once

#include <map>

#include "Track.h"

namespace daw
{
/**
    The "Buses, sends and routing" guide: where each track's sound goes, what a bus is for, why levels add up in a bus,
    glue compression, the mix bus (the master), sends to return tracks (one shared reverb, insert or send, depth,
    pre- and post-fader), solo and mute, and a look at the user's own song. The content only: the window is
    RoutingExplainer. Facts with numbers (the sums in dB, the send levels, the Drum bus starting point) are checked
    in the tests.
*/
namespace routingguide
{
    enum class Topic { what, why, howTo, levels, glue, mixBus, typical, sends, sendHowTo, inspector, insertOrSend, depth, preFader, solo, signalFlow, bounce, inPlace, yourSong };
    inline constexpr Topic allTopics[] { Topic::what, Topic::why, Topic::howTo, Topic::levels, Topic::glue, Topic::mixBus, Topic::typical,
                                         Topic::sends, Topic::sendHowTo, Topic::inspector, Topic::insertOrSend, Topic::depth, Topic::preFader,
                                         Topic::solo, Topic::signalFlow, Topic::bounce, Topic::inPlace, Topic::yourSong };

    juce::String getTopicName (Topic);
    juce::String getTopicSummary (Topic);

    /** The general text of a topic. For "your song" this is the part that doesn't depend on the song. */
    juce::String explain (Topic);

    /** The sum of levels in dB when tracks are added: powers for different sounds (incoherent, 10 log10 N for equal
        levels), amplitudes for the same sound in step (coherent, 20 log10 N, which is also the worst case for peaks). */
    float sumDb (const std::vector<float>& levelsDb, bool sameSound);

    /** The kind of sound a track carries, for suggesting bus names and advice. */
    enum class Family { none, drums, bass, guitars, keys, vocals };

    /** From the words in the track's name, then from its sound (a MIDI track playing drums). A bus is judged by its name. */
    Family getFamily (const Track&);

    /** "Drums", "Bass", "Guitars", "Keys", "Vocals"; empty for none. */
    juce::String getFamilyName (Family);

    /** The family's name when every track is of that family and no bus has that name yet; otherwise "Bus N". */
    juce::String suggestBusName (const std::vector<Track*>&, const Tracks&);

    /** The send level that gives the same reverb-to-dry balance as an insert effect at `mixPercent`:
        20 log10 (tan (mix x 90 degrees)), limited to Off (-60) .. +6 dB. 20% gives -9.76 dB. */
    float getSendDbForMix (float mixPercent);

    /** How much louder the dry sound gets when an insert effect at `mixPercent` becomes a send: -20 log10 (cos (mix x 90 degrees)). */
    float getDryChangeDb (float mixPercent);

    /** A send drawn as a dashed line from a box to a return. */
    struct Link
    {
        int target = -1;                 // index of the return in the list
        float levelDb = -10.0f;
        bool preFader = false;
    };

    /** A box of the picture, as plain data: the song's tracks or an example. */
    struct Node
    {
        juce::String name;
        juce::Colour colour;
        bool isReturn = false;           // a return track: it hears sends and plays into the master
        std::vector<Link> sends;         // the sends that are on
        bool isBus = false, folded = false, muted = false, soloed = false, silent = false, onSelectedPath = false;
        int parent = -1;                 // index of its bus in the list, -1: the master
        int numInside = 0;               // a folded bus: how many tracks it holds
        float levelDb = -100.0f, inputDb = -100.0f;   // live levels; a bus's input is its sum
        const Track* track = nullptr;    // the song's track, for clicks
    };

    /** The example tree a topic shows, in edit order (a bus before its tracks). */
    std::vector<Node> getExample (Topic);

    /** A file name the picture draws by the master, for the topics about files ("Kick + Snare.wav"); empty for the others. */
    juce::String getExampleFileTag (Topic);

    /** The song's visible tracks as boxes (a folded bus is one box); levels from the meters. */
    std::vector<Node> fromSong (const Tracks&, const Track* selected);

    /** Positions in units: x = column (0 left, the master in the last), y = row (tracks one per row, a bus at the
        middle of its tracks). Returns sit in the column before the master, below the other rows. Index nodes.size() is
        the master. */
    std::vector<juce::Point<float>> layOut (const std::vector<Node>&);

    /** One thing the "your song" topic suggests, with the action that does it. */
    struct Advice
    {
        juce::String text;
        Topic topic = Topic::yourSong;
        enum class Action { none, createBus, moveIntoBus, addGlue, shareEffect, addReturnEffect, makeWet, moveEffect } action = Action::none;
        std::vector<Track*> tracks;      // createBus, moveIntoBus: the tracks; addGlue: the bus; shareEffect: the tracks with their own effect
        juce::String busName;
        Track* bus = nullptr;            // createBus: when set, the new bus goes next to this bus (the tracks' own, of another kind);
                                         // moveIntoBus: the existing bus they join; shareEffect: the return that has the effect
                                         // already (null: make one); addReturnEffect and makeWet: the return
        juce::String effect = {};        // shareEffect, addReturnEffect, makeWet: the effect's identifier ("DawIt Reverb")
        const PluginSlot* slot = nullptr;   // moveEffect: the effect to move (on tracks[0]) ...
        int toIndex = -1;                   // ... and the place in the chain it goes to (Engine::movePlugin's newIndex)
        juce::String buttonText = {};       // moveEffect: the button, "Put DawIt Compressor before DawIt Reverb"
    };

    /** Live levels for the advice: what comes into each bus and each return, and whether the song is playing. */
    struct Levels
    {
        std::map<const Track*, float> busInputDb;
        float masterPeakDb = -100.0f;    // the master's peak level now (-100: none)
        bool playing = false;
    };

    /** One sentence on where `track` sends (the "In your song" line of the inspector topic): its sends that are on,
        with levels and where they are taken; for a return, the tracks that send to it; for null (the master), what
        reaches it. Empty when the song has no tracks. */
    juce::String describeSends (const Track* track, const Tracks&);

    /** "The song has 7 tracks, 2 buses and 1 return. ..." */
    juce::String explainSong (const Tracks&);

    /** What to try, in the order to try it. */
    std::vector<Advice> analyse (const Tracks&, const Levels&);

    /** Where the master's peaks should stay before mastering, in dBFS: about 6 dB of headroom, as in the Gain staging guide. */
    inline constexpr float mixBusTargetPeakDb = -6.0f;

    /** Headroom: how far a peak at `peakDb` is below 0 dBFS, in dB (negative when it is over). */
    float getHeadroomDb (float peakDb);

    /** "Peak -8.0 dB: 8.0 dB of headroom, enough." / "... a bit hot ..." / "... over 0 dB: it clips." */
    juce::String describeMasterLevel (float peakDb);

    /** The Mix bus topic's numbers, from DawIt Compressor's Mix bus starting point. */
    juce::String describeMixBusSettings();

    /** The Glue topic's numbers, from DawIt Compressor's Drum bus starting point. */
    juce::String describeGlueSettings();
} // namespace routingguide
} // namespace daw
