#pragma once

#include <juce_core/juce_core.h>

#include <utility>
#include <vector>

namespace daw::yaml
{
/** A value read from a small YAML file: nothing, a piece of text, a list or a map (keys in the order written). */
struct Node
{
    enum class Kind { null, scalar, list, map };

    Kind kind = Kind::null;
    juce::String scalar;                                // for a scalar, without its quotes
    std::vector<Node> items;                            // for a list
    std::vector<std::pair<juce::String, Node>> entries; // for a map

    bool isScalar() const noexcept { return kind == Kind::scalar; }
    bool isList() const noexcept   { return kind == Kind::list; }
    bool isMap() const noexcept    { return kind == Kind::map; }

    /** The value of a map's key, or null when this isn't a map or has no such key. */
    const Node* find (const juce::String& key) const;

    /** A map's scalar as text, number or flag; the fallback when it's missing or isn't one. */
    juce::String getString (const juce::String& key, const juce::String& fallback = {}) const;
    int getInt (const juce::String& key, int fallback = 0) const;
    double getDouble (const juce::String& key, double fallback = 0.0) const;
    bool getBool (const juce::String& key, bool fallback = false) const;
};

struct Result
{
    Node root;
    juce::String error;   // empty on success; otherwise "line 12: ..."
    bool ok() const noexcept { return error.isEmpty(); }
};

/** Reads the small part of YAML that DiffSinger voices use (`dsconfig.yaml`, `dsdict-en.yaml`):
    block maps, block lists (also of maps), flow lists and maps on one line (`[a, b]`, `{k: v}`),
    plain, 'single' and "double" quoted scalars, and # comments. It fails with a message naming the
    line on what it doesn't read: tabs in the indentation, anchors, aliases, tags, block scalars (| and >),
    several documents, and flow lists over more than one line. */
Result parse (const juce::String& text);
} // namespace daw::yaml
