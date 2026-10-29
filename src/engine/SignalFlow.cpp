#include "SignalFlow.h"

#include "RoutingGuide.h"

namespace daw::signalflow
{
float toDb (float linear)
{
    return juce::Decibels::gainToDecibels (linear, -100.0f);
}

int Graph::getNumLit() const
{
    return (int) std::count_if (nodes.begin(), nodes.end(), [] (const Node& n) { return n.lit; });
}

int Graph::getFirstNode (int trackIndex) const
{
    for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes[i].trackIndex == trackIndex)
            return (int) i;

    return -1;
}

int Graph::getFader (int trackIndex) const
{
    for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes[i].trackIndex == trackIndex && nodes[i].kind == NodeKind::fader)
            return (int) i;

    return -1;
}

int Graph::getSum (int trackIndex) const
{
    for (size_t i = 0; i < nodes.size(); ++i)
        if (nodes[i].trackIndex == trackIndex && nodes[i].kind == NodeKind::sum)
            return (int) i;

    return -1;
}

//==============================================================================
void layOut (Graph& g)
{
    const int numNodes = (int) g.nodes.size();

    // Columns: the longest path from the left. Every edge goes from a lower column to a higher one, so this settles
    // after at most as many passes as there are nodes.
    for (auto& n : g.nodes)
        n.column = 0;

    for (int pass = 0; pass < numNodes; ++pass)
    {
        bool changed = false;

        for (auto& e : g.edges)
        {
            if (g.nodes[(size_t) e.to].kind == NodeKind::master)
                continue;

            const int wanted = g.nodes[(size_t) e.from].column + 1;

            if (g.nodes[(size_t) e.to].column < wanted)
            {
                g.nodes[(size_t) e.to].column = wanted;
                changed = true;
            }
        }

        if (! changed)
            break;
    }

    if (g.master >= 0)
    {
        int highest = 0;

        for (int i = 0; i < numNodes; ++i)
            if (i != g.master)
                highest = juce::jmax (highest, g.nodes[(size_t) i].column);

        g.nodes[(size_t) g.master].column = highest + 1;
    }

    // Rows: tracks one per row in edit order, a bus in the middle of its members, returns after all the others.
    const int numTracks = (int) g.tracks.size();
    std::vector<std::vector<int>> members ((size_t) numTracks);

    for (int i = 0; i < numTracks; ++i)
        if (g.tracks[(size_t) i].parent >= 0)
            members[(size_t) g.tracks[(size_t) i].parent].push_back (i);

    std::vector<bool> done ((size_t) numTracks, false);
    float nextRow = 0.0f;

    for (int i = 0; i < numTracks; ++i)
    {
        auto& t = g.tracks[(size_t) i];

        if (t.isReturn || (t.isBus && ! members[(size_t) i].empty()))
            continue;

        t.row = nextRow++;
        done[(size_t) i] = true;
    }

    // Buses, deepest first: a bus's members are always before it in edit order, so walking backwards from the end
    // would see the bus first; repeat until every bus has its members' rows.
    for (int pass = 0; pass < numTracks; ++pass)
    {
        bool changed = false;

        for (int i = 0; i < numTracks; ++i)
        {
            auto& t = g.tracks[(size_t) i];

            if (done[(size_t) i] || t.isReturn)
                continue;

            bool ready = true;
            float sum = 0.0f;

            for (int m : members[(size_t) i])
            {
                ready = ready && done[(size_t) m];
                sum += g.tracks[(size_t) m].row;
            }

            if (ready)
            {
                t.row = sum / (float) members[(size_t) i].size();
                done[(size_t) i] = true;
                changed = true;
            }
        }

        if (! changed)
            break;
    }

    for (int i = 0; i < numTracks; ++i)
        if (g.tracks[(size_t) i].isReturn)
            g.tracks[(size_t) i].row = nextRow++;

    if (numTracks > 0)
    {
        float lowest = g.tracks[0].row, highest = g.tracks[0].row;

        for (auto& t : g.tracks)
        {
            lowest = juce::jmin (lowest, t.row);
            highest = juce::jmax (highest, t.row);
        }

        g.masterRow = (lowest + highest) * 0.5f;
    }
    else
    {
        g.masterRow = 0.0f;
    }
}

//==============================================================================
namespace
{
/** Adds the boxes and lines of tracks one at a time; shared by the song and the example. */
struct Builder
{
    Graph g;

    int addTrack (const juce::String& name, juce::Colour colour, bool isBus, bool isReturn, int parent, const Track* track)
    {
        TrackInfo t;
        t.name = name;
        t.colour = colour;
        t.isBus = isBus;
        t.isReturn = isReturn;
        t.parent = parent;
        t.track = track;
        g.tracks.push_back (t);
        faders.push_back (-1);
        sums.push_back (-1);
        previous.push_back (-1);
        return (int) g.tracks.size() - 1;
    }

    /** Adds a box at the end of the track's chain, with the flow line from the box before it. */
    int addNode (int trackIndex, Node n)
    {
        auto& t = g.tracks[(size_t) trackIndex];
        n.trackIndex = trackIndex;
        n.silenced = t.silence != routing::Silence::none;
        const int index = (int) g.nodes.size();
        const int before = previous[(size_t) trackIndex];

        if (before >= 0)
        {
            n.inputDb = g.nodes[(size_t) before].levelDb;
            addEdge (before, index, g.nodes[(size_t) before].levelDb, g.nodes[(size_t) before].kind == NodeKind::fader && n.silenced);
        }

        g.nodes.push_back (std::move (n));

        if (g.nodes.back().kind == NodeKind::fader)
            faders[(size_t) trackIndex] = index;

        if (g.nodes.back().kind == NodeKind::sum)
            sums[(size_t) trackIndex] = index;

        previous[(size_t) trackIndex] = index;
        return index;
    }

    void addEdge (int from, int to, float levelDb, bool silenced)
    {
        Edge e;
        e.from = from;
        e.to = to;
        e.levelDb = levelDb;
        e.silenced = silenced;
        g.edges.push_back (e);
    }

    /** The line from a track's fader into its bus (or the master). */
    void connectToParent (int trackIndex)
    {
        const int fader = faders[(size_t) trackIndex];
        const int parent = g.tracks[(size_t) trackIndex].parent;
        const int target = parent >= 0 ? sums[(size_t) parent] : g.master;

        if (fader >= 0 && target >= 0)
            addEdge (fader, target, g.nodes[(size_t) fader].levelDb, g.nodes[(size_t) fader].silenced);
    }

    void addSend (int trackIndex, int returnIndex, float sendDb, float levelDb, bool preFader, bool silenced)
    {
        const int fader = faders[(size_t) trackIndex];
        const int target = sums[(size_t) returnIndex];

        if (fader < 0 || target < 0)
            return;

        Edge e;
        e.from = fader;
        e.to = target;
        e.isSend = true;
        e.sendDb = sendDb;
        e.levelDb = levelDb;
        e.preFader = preFader;
        e.silenced = silenced;
        g.edges.push_back (e);
    }

    /** Works out which boxes are on the path of the selected track. */
    void finish (int selectedIndex)
    {
        g.hasSelection = selectedIndex >= 0;

        if (g.hasSelection)
        {
            for (auto& t : g.tracks)
                t.lit = false;

            // The track, the buses it plays into, and the returns it sends to.
            auto lightUp = [this] (int i) { g.tracks[(size_t) i].lit = true; };
            lightUp (selectedIndex);

            for (int p = g.tracks[(size_t) selectedIndex].parent; p >= 0; p = g.tracks[(size_t) p].parent)
                lightUp (p);

            for (auto& e : g.edges)
                if (e.isSend && g.nodes[(size_t) e.from].trackIndex == selectedIndex)
                    lightUp (g.nodes[(size_t) e.to].trackIndex);

            // A bus or a return: also the tracks that feed it.
            if (g.tracks[(size_t) selectedIndex].isBus)
            {
                for (size_t i = 0; i < g.tracks.size(); ++i)
                    for (int p = g.tracks[i].parent; p >= 0; p = g.tracks[(size_t) p].parent)
                        if (p == selectedIndex)
                            lightUp ((int) i);
            }
            else if (g.tracks[(size_t) selectedIndex].isReturn)
            {
                for (auto& e : g.edges)
                    if (e.isSend && g.nodes[(size_t) e.to].trackIndex == selectedIndex)
                        lightUp (g.nodes[(size_t) e.from].trackIndex);
            }
        }

        for (auto& n : g.nodes)
            n.lit = n.trackIndex < 0 || g.tracks[(size_t) n.trackIndex].lit;

        for (auto& e : g.edges)
            e.lit = g.nodes[(size_t) e.from].lit && g.nodes[(size_t) e.to].lit;

        layOut (g);
    }

    std::vector<int> faders, sums, previous;
};
} // namespace

Graph build (const Tracks& tracks, const Track* selected, const MasterState& master)
{
    Builder b;
    std::map<const Track*, int> indexOf;

    for (size_t i = 0; i < tracks.size(); ++i)
        indexOf[tracks[i].get()] = (int) i;

    auto find = [&indexOf] (const Track* t) { auto it = indexOf.find (t); return it == indexOf.end() ? -1 : it->second; };

    for (auto& t : tracks)
    {
        const int parent = t->bus != nullptr ? find (t->bus) : -1;
        const int i = b.addTrack (t->name, t->colour, t->isBus(), t->isReturn(), parent, t.get());
        auto& info = b.g.tracks[(size_t) i];
        info.silence = routing::getSilence (*t, tracks);
        info.muted = t->muted.load();
    }

    // The boxes of each track, in edit order; the master comes last.
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto& t = *tracks[i];
        const int ti = (int) i;

        if (t.hasInput())
        {
            Node sum;
            sum.kind = NodeKind::sum;
            sum.levelDb = toDb (t.busInputLevel.load());

            if (t.isBus())
                for (auto* m : routing::getMembers (t, tracks))
                    sum.feeders.add (m->name);
            else
                for (auto* s : routing::getSenders (t, tracks))
                    sum.feeders.add (s->name);

            b.addNode (ti, std::move (sum));
        }
        else
        {
            const PluginSlot* instrument = nullptr;

            for (auto& slot : t.plugins)
                if (slot->isInstrument && slot->instance != nullptr)
                {
                    instrument = slot.get();
                    break;
                }

            Node source;
            source.kind = NodeKind::source;

            if (instrument != nullptr)
            {
                source.sourceKind = SourceKind::instrument;
                source.label = instrument->instance->getName();
                source.levelDb = toDb (instrument->outputLevel.load());
                source.bypassed = false;

                for (size_t s = 0; s < t.plugins.size(); ++s)
                    if (t.plugins[s].get() == instrument)
                        source.slot = (int) s;

                if (instrument->instance->getName().startsWith ("DawIt"))
                    source.format.clear();
                else
                    source.format = instrument->instance->getPluginDescription().pluginFormatName;
            }
            else
            {
                source.sourceKind = t.isMidi() ? SourceKind::builtInSynth : SourceKind::audio;
                source.label = t.isMidi() ? "DawIt Synth" : juce::String();
                source.detail = t.isMidi() ? builtInSounds::getName (t.builtInSound.load()) : juce::String();
                source.levelDb = toDb (t.chainInputLevel.load());
            }

            b.addNode (ti, std::move (source));
        }

        for (size_t s = 0; s < t.plugins.size(); ++s)
        {
            auto& slot = *t.plugins[s];

            if (slot.isInstrument || slot.instance == nullptr)
                continue;

            Node effect;
            effect.kind = NodeKind::effect;
            effect.slot = (int) s;
            effect.label = slot.instance->getName();

            if (! effect.label.startsWith ("DawIt"))
                effect.format = slot.instance->getPluginDescription().pluginFormatName;

            effect.bypassed = slot.bypassed.load();
            effect.levelDb = toDb (slot.outputLevel.load());
            b.addNode (ti, std::move (effect));
        }

        Node fader;
        fader.kind = NodeKind::fader;
        fader.volumeDb = t.volumeDb.load();
        fader.pan = t.pan.load();
        fader.muted = t.muted.load();
        fader.levelDb = toDb (juce::jmax (t.peakLeft.load(), t.peakRight.load()));
        b.addNode (ti, std::move (fader));
    }

    Node masterNode;
    masterNode.kind = NodeKind::master;
    masterNode.levelLeft = master.peakLeft;
    masterNode.levelRight = master.peakRight;
    masterNode.masterVolumeDb = master.volumeDb;
    masterNode.levelDb = toDb (juce::jmax (master.peakLeft, master.peakRight));
    b.g.master = (int) b.g.nodes.size();
    b.g.nodes.push_back (std::move (masterNode));

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        b.connectToParent ((int) i);
        const auto& t = *tracks[i];
        const auto silence = b.g.tracks[i].silence;

        for (auto& send : t.sends)
        {
            const int target = find (send->target);

            if (! send->isOn() || target < 0 || ! tracks[(size_t) target]->isReturn())
                continue;

            const bool dark = silence == routing::Silence::muted || silence == routing::Silence::busMuted
                              || (silence == routing::Silence::solo && ! send->target->soloed.load());
            b.addSend ((int) i, target, send->levelDb.load(), toDb (send->level.load()), send->preFader.load(), dark);
        }
    }

    b.finish (selected != nullptr ? find (selected) : -1);
    return std::move (b.g);
}

Graph getExample()
{
    Builder b;
    const juce::Colour red (0xffe0645c), orange (0xfff0a04b), green (0xff5fd38d), blue (0xff4fa3ff), purple (0xffb07be8);

    const int drums = b.addTrack ("Drums", orange, true, false, -1, nullptr);
    const int kick = b.addTrack ("Kick", red, false, false, drums, nullptr);
    const int snare = b.addTrack ("Snare", orange, false, false, drums, nullptr);
    const int bass = b.addTrack ("Bass", green, false, false, -1, nullptr);
    const int vocal = b.addTrack ("Vocal", blue, false, false, -1, nullptr);
    const int reverb = b.addTrack ("Reverb", purple, false, true, -1, nullptr);

    auto source = [&b] (int t, SourceKind kind, const juce::String& label, float db)
    {
        Node n;
        n.kind = NodeKind::source;
        n.sourceKind = kind;
        n.label = label;
        n.levelDb = db;
        return b.addNode (t, n);
    };

    auto effect = [&b] (int t, int slot, const juce::String& label, float db, bool bypassed = false)
    {
        Node n;
        n.kind = NodeKind::effect;
        n.slot = slot;
        n.label = label;
        n.levelDb = db;
        n.bypassed = bypassed;
        return b.addNode (t, n);
    };

    auto fader = [&b] (int t, float volumeDb, float db)
    {
        Node n;
        n.kind = NodeKind::fader;
        n.volumeDb = volumeDb;
        n.levelDb = db;
        return b.addNode (t, n);
    };

    // The sums come before the members' boxes in the list, as in a song (a bus is followed by everything in it).
    const float drumsSum = routingguide::sumDb ({ -12.0f, -14.0f }, false);
    {
        Node n;
        n.kind = NodeKind::sum;
        n.levelDb = drumsSum;
        n.feeders = { "Kick", "Snare" };
        b.addNode (drums, n);
    }
    effect (drums, 0, "DawIt Compressor", -12.0f);
    fader (drums, 0.0f, -12.0f);

    source (kick, SourceKind::audio, {}, -12.0f);
    fader (kick, 0.0f, -12.0f);

    source (snare, SourceKind::audio, {}, -14.0f);
    effect (snare, 0, "DawIt Gate", -14.0f);
    fader (snare, 0.0f, -14.0f);

    source (bass, SourceKind::builtInSynth, "DawIt Synth", -10.0f);
    effect (bass, 0, "DawIt EQ", -11.0f);
    effect (bass, 1, "DawIt Saturation", -11.0f, true);
    fader (bass, -2.0f, -13.0f);

    source (vocal, SourceKind::audio, {}, -8.0f);
    effect (vocal, 0, "DawIt EQ", -9.0f);
    effect (vocal, 1, "DawIt Compressor", -12.0f);
    fader (vocal, 0.0f, -12.0f);

    {
        Node n;
        n.kind = NodeKind::sum;
        n.levelDb = -24.0f;
        n.feeders = { "Vocal" };
        b.addNode (reverb, n);
    }
    effect (reverb, 0, "DawIt Reverb", -26.0f);
    fader (reverb, 0.0f, -26.0f);

    Node masterNode;
    masterNode.kind = NodeKind::master;
    masterNode.levelDb = routingguide::sumDb ({ -12.0f, -13.0f, -12.0f, -26.0f }, false);
    masterNode.levelLeft = masterNode.levelRight = juce::Decibels::decibelsToGain (masterNode.levelDb);
    b.g.master = (int) b.g.nodes.size();
    b.g.nodes.push_back (masterNode);

    for (int t : { drums, kick, snare, bass, vocal, reverb })
        b.connectToParent (t);

    b.addSend (vocal, reverb, -12.0f, -24.0f, false, false);
    b.finish (vocal);
    return std::move (b.g);
}
} // namespace daw::signalflow
