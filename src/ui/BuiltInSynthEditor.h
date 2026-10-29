#pragma once

#include "engine/Engine.h"
#include "PluginWindows.h"
#include "Theme.h"

namespace daw
{
/**
    The window of a track's built-in synth, laid out like a plugin editor: a display with the
    current sound and a row of sounds to pick from. Clicking a sound picks it and plays a short
    preview; the left and right arrow keys step through the sounds.
*/
class BuiltInSynthEditor final : public juce::Component, private juce::ChangeListener
{
public:
    BuiltInSynthEditor (Engine&, PluginWindowManager&, Track&);
    ~BuiltInSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    class SoundButton;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void pick (BuiltInSound);

    Engine& engine;
    PluginWindowManager& windows;
    Track& track;

    juce::OwnedArray<SoundButton> buttons;
    BuiltInSound shownSound;
    std::optional<BuiltInSound> hoveredSound;

    juce::Rectangle<int> headerArea, displayArea, footerArea;
};
} // namespace daw
