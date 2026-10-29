#pragma once

#include "Track.h"

namespace daw::routing
{
/**
    The tree of tracks and buses, as plain functions for the engine, the views and the tests.

    Where a track plays is `Track::bus` (null: the master). `Engine::getTracks()` stays one flat list in the
    order the rows show, with one rule: a bus is followed by everything in it (its own tracks and their tracks),
    so code that walks the list keeps working.
*/

/** 0 at the top level (plays into the master), 1 inside a bus, 2 inside a bus in a bus, and so on. */
int getDepth (const Track&);

/** The tracks that play straight into the bus, in order. */
std::vector<Track*> getMembers (const Track& bus, const Tracks&);

/** The index after the track at `index` and everything in it (the next index for a track that holds nothing). */
int getBlockEnd (const Tracks&, int index);

/** True when the track plays into the bus, directly or through other buses. */
bool playsInto (const Track& track, const Track& bus);

/** True when the track can go into `bus` (null: the master): it must be a bus that isn't the track itself or inside it.
    A return stays at the top level, and nothing goes into a return. */
bool canMoveInto (const Track& track, const Track* bus);

/** The return tracks, in edit order. */
std::vector<Track*> getReturns (const Tracks&);

/** The returns `track` can send to, in edit order: all of them for a track or a bus, none for a return or the master (null). */
std::vector<Track*> getSendTargets (const Track* track, const Tracks&);

/** The tracks (and buses) with a send that is on to the return, in edit order. */
std::vector<Track*> getSenders (const Track& returnTrack, const Tracks&);

/** The track's send to the return, or null when it has none (it may be off). */
Send* findSend (const Track&, const Track& returnTrack);

/** True when the track has a send to the return that is on. */
bool isSendOn (const Track&, const Track& returnTrack);

/** True when every bus is followed by everything in it, and nothing else. */
bool isTidy (const Tracks&);

/** Every track before the buses it plays into, otherwise in edit order, then the returns: the order the engine
    renders in (a return hears the sends of everything before it). */
std::vector<Track*> getRenderOrder (const Tracks&);

/** The rows the arrangement and the mixer show: everything but the tracks inside folded buses. */
std::vector<Track*> getVisibleTracks (const Tracks&);

/** The selected tracks without those inside another selected bus, and without returns, in edit order. */
std::vector<Track*> getOutermost (const std::vector<Track*>& selected, const Tracks&);

/** The closest bus every one of them plays into (null: the master). */
Track* findCommonBus (const std::vector<Track*>&);

/** True when the track, a bus it plays into, or a track that plays into it is soloed: the path that stays open
    while anything is soloed. A return is also on it when a track on the path sends to it. */
bool isOnSoloPath (const Track&, const Tracks&);

/** Why a track can't be heard, for the headers and strips. `solo` means another track is soloed. */
enum class Silence { none, muted, busMuted, solo };
Silence getSilence (const Track&, const Tracks&);

/** "Kick → Drums → Master", with " (+ sends to Reverb)" when it has sends that are on. */
juce::String describePath (const Track&);

/** Where a dragged track lands: in `bus` (null: the master), before `before` (null: at the end). */
struct Place
{
    Track* bus = nullptr;
    const Track* before = nullptr;
};

/** The levels (depths) a track can land at in the gap above visible row `gap` (rows.size(): below the last row):
    the deepest is the row above's level (one deeper when it is an open bus), the shallowest the row below's level
    (0 below the last row). Returned as { shallowest, deepest }. */
std::pair<int, int> getGapLevels (const std::vector<Track*>& rows, int gap);

/** Where a track dropped in that gap at that level lands. */
Place getPlace (const std::vector<Track*>& rows, int gap, int level);
} // namespace daw::routing
