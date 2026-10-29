#include "Routing.h"

namespace daw::routing
{
int getDepth (const Track& track)
{
    int depth = 0;

    for (auto* b = track.bus; b != nullptr; b = b->bus)
        ++depth;

    return depth;
}

std::vector<Track*> getMembers (const Track& bus, const Tracks& tracks)
{
    std::vector<Track*> members;

    for (auto& t : tracks)
        if (t->bus == &bus)
            members.push_back (t.get());

    return members;
}

bool playsInto (const Track& track, const Track& bus)
{
    for (auto* b = track.bus; b != nullptr; b = b->bus)
        if (b == &bus)
            return true;

    return false;
}

int getBlockEnd (const Tracks& tracks, int index)
{
    int end = index + 1;

    while (end < (int) tracks.size() && playsInto (*tracks[(size_t) end], *tracks[(size_t) index]))
        ++end;

    return end;
}

bool canMoveInto (const Track& track, const Track* bus)
{
    return bus == nullptr ? true : (bus->isBus() && ! track.isReturn() && bus != &track && ! playsInto (*bus, track));
}

std::vector<Track*> getReturns (const Tracks& tracks)
{
    std::vector<Track*> result;

    for (auto& t : tracks)
        if (t->isReturn())
            result.push_back (t.get());

    return result;
}

std::vector<Track*> getSendTargets (const Track* track, const Tracks& tracks)
{
    if (track == nullptr || track->isReturn())
        return {};

    return getReturns (tracks);
}

Send* findSend (const Track& track, const Track& returnTrack)
{
    for (auto& s : track.sends)
        if (s->target == &returnTrack)
            return s.get();

    return nullptr;
}

bool isSendOn (const Track& track, const Track& returnTrack)
{
    auto* send = findSend (track, returnTrack);
    return send != nullptr && send->isOn();
}

std::vector<Track*> getSenders (const Track& returnTrack, const Tracks& tracks)
{
    std::vector<Track*> result;

    for (auto& t : tracks)
        if (isSendOn (*t, returnTrack))
            result.push_back (t.get());

    return result;
}

bool isTidy (const Tracks& tracks)
{
    for (size_t j = 0; j < tracks.size(); ++j)
    {
        auto* bus = tracks[j]->bus;

        if (bus == nullptr)
            continue;

        size_t at = tracks.size();

        for (size_t i = 0; i < tracks.size(); ++i)
            if (tracks[i].get() == bus)
                at = i;

        if (! bus->isBus() || at >= j)
            return false;

        for (size_t k = at + 1; k < j; ++k)
            if (! playsInto (*tracks[k], *bus))
                return false;
    }

    return true;
}

std::vector<Track*> getRenderOrder (const Tracks& tracks)
{
    std::vector<Track*> order;
    order.reserve (tracks.size());

    auto isListed = [&tracks] (const Track* t)
    {
        return std::any_of (tracks.begin(), tracks.end(), [t] (auto& p) { return p.get() == t; });
    };

    // Post-order: a bus comes after everything in it.
    std::function<void (Track&)> visit = [&] (Track& t)
    {
        for (auto* member : getMembers (t, tracks))
            visit (*member);

        order.push_back (&t);
    };

    for (auto& t : tracks)
        if (! t->isReturn() && (t->bus == nullptr || ! isListed (t->bus)))
            visit (*t);

    for (auto& t : tracks)
        if (t->isReturn())
            order.push_back (t.get());

    return order;
}

std::vector<Track*> getVisibleTracks (const Tracks& tracks)
{
    std::vector<Track*> rows;

    for (auto& t : tracks)
    {
        bool hidden = false;

        for (auto* b = t->bus; b != nullptr; b = b->bus)
            hidden = hidden || b->folded;

        if (! hidden)
            rows.push_back (t.get());
    }

    return rows;
}

std::vector<Track*> getOutermost (const std::vector<Track*>& selected, const Tracks& tracks)
{
    std::vector<Track*> result;

    for (auto& t : tracks)
    {
        if (t->isReturn() || std::find (selected.begin(), selected.end(), t.get()) == selected.end())
            continue;

        bool inside = false;

        for (auto* b = t->bus; b != nullptr; b = b->bus)
            inside = inside || std::find (selected.begin(), selected.end(), b) != selected.end();

        if (! inside)
            result.push_back (t.get());
    }

    return result;
}

Track* findCommonBus (const std::vector<Track*>& tracks)
{
    if (tracks.empty())
        return nullptr;

    for (auto* candidate = tracks.front()->bus; candidate != nullptr; candidate = candidate->bus)
    {
        const bool all = std::all_of (tracks.begin(), tracks.end(), [candidate] (Track* t) { return playsInto (*t, *candidate); });

        if (all)
            return candidate;
    }

    return nullptr;
}

bool isOnSoloPath (const Track& track, const Tracks& tracks)
{
    if (track.soloed.load())
        return true;

    for (auto* b = track.bus; b != nullptr; b = b->bus)
        if (b->soloed.load())
            return true;

    for (auto& t : tracks)
        if (t->soloed.load() && playsInto (*t, track))
            return true;

    // A return is heard while a track that sends to it is on the solo path.
    if (track.isReturn())
        for (auto& t : tracks)
            if (isSendOn (*t, track) && isOnSoloPath (*t, tracks))
                return true;

    return false;
}

Silence getSilence (const Track& track, const Tracks& tracks)
{
    if (track.muted.load())
        return Silence::muted;

    for (auto* b = track.bus; b != nullptr; b = b->bus)
        if (b->muted.load())
            return Silence::busMuted;

    const bool anySolo = std::any_of (tracks.begin(), tracks.end(), [] (auto& t) { return t->soloed.load(); });

    if (anySolo && ! isOnSoloPath (track, tracks))
        return Silence::solo;

    return Silence::none;
}

juce::String describePath (const Track& track)
{
    const auto arrow = juce::String::fromUTF8 (" \xe2\x86\x92 ");
    juce::String text = track.name;

    for (auto* b = track.bus; b != nullptr; b = b->bus)
        text << arrow << b->name;

    juce::StringArray returns;

    for (auto& s : track.sends)
        if (s->isOn())
            returns.add (s->target->name);

    text << arrow << "Master";

    if (! returns.isEmpty())
        text << " (+ sends to " << returns.joinIntoString (", ") << ")";

    return text;
}

std::pair<int, int> getGapLevels (const std::vector<Track*>& rows, int gap)
{
    const int size = (int) rows.size();
    gap = juce::jlimit (0, size, gap);

    int deepest = 0;

    if (gap > 0)
    {
        auto* above = rows[(size_t) gap - 1];
        deepest = getDepth (*above) + (above->isBus() && ! above->folded ? 1 : 0);
    }

    const int shallowest = gap < size ? getDepth (*rows[(size_t) gap]) : 0;
    return { juce::jmin (shallowest, deepest), deepest };
}

Place getPlace (const std::vector<Track*>& rows, int gap, int level)
{
    const int size = (int) rows.size();
    gap = juce::jlimit (0, size, gap);
    auto* below = gap < size ? rows[(size_t) gap] : nullptr;

    if (gap == 0)
        return { nullptr, below };

    auto* above = rows[(size_t) gap - 1];

    // Into the open bus right above.
    if (above->isBus() && ! above->folded && level == getDepth (*above) + 1)
        return { above, below != nullptr && below->bus == above ? below : nullptr };

    // Beside the track above, or beside the bus it is in, at the chosen level.
    auto* sibling = above;

    while (sibling->bus != nullptr && getDepth (*sibling) > level)
        sibling = sibling->bus;

    Track* bus = sibling->bus;
    return { bus, below != nullptr && below->bus == bus ? below : nullptr };
}
} // namespace daw::routing
