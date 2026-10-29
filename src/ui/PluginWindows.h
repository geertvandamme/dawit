#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Theme.h"

namespace daw
{
/** Keeps one editor window per plugin instance open, and per built-in device (such as a track's
    built-in synth), keyed by whatever object the window belongs to. */
class PluginWindowManager
{
public:
    ~PluginWindowManager() { windows.clear(); }

    /** Attached to every editor window, so keys the plugin doesn't use (e.g. Space) still work.
        Must outlive the windows. */
    void setKeyListener (juce::KeyListener* listener) { keyListener = listener; }

    void show (juce::AudioProcessor& processor, const juce::String& title)
    {
        show (&processor, title, [&processor]() -> juce::Component*
        {
            if (auto* editor = processor.hasEditor() ? processor.createEditorIfNeeded() : nullptr)
                return editor;

            return new juce::GenericAudioProcessorEditor (processor);
        });
    }

    /** Brings the key's window to the front, or opens one holding a new createContent(). With a
        `minimumSize`, the window can be resized (and made full screen) down to that size. */
    void show (const void* key, const juce::String& title, const std::function<juce::Component*()>& createContent,
               juce::Point<int> minimumSize = {})
    {
        for (auto* w : windows)
        {
            if (w->key == key)
            {
                w->toFront (true);
                return;
            }
        }

        windows.add (new Window (*this, key, title, createContent(), keyListener, minimumSize));
    }

    void closeFor (const void* key)
    {
        for (int i = windows.size(); --i >= 0;)
            if (windows[i]->key == key)
                windows.remove (i);
    }

    void closeFor (const juce::AudioProcessor& processor) { closeFor (static_cast<const void*> (&processor)); }

    void closeAll() { windows.clear(); }

private:
    class Window final : public juce::DocumentWindow
    {
    public:
        Window (PluginWindowManager& o, const void* k, const juce::String& title, juce::Component* content, juce::KeyListener* keys,
                juce::Point<int> minimumSize)
            : juce::DocumentWindow (title, theme::panel, juce::DocumentWindow::closeButton),
              owner (o), key (k), keyListener (keys)
        {
            if (keyListener != nullptr)
                addKeyListener (keyListener);

            auto* editor = dynamic_cast<juce::AudioProcessorEditor*> (content);

            setUsingNativeTitleBar (true);
            setContentOwned (content, true);
            setResizable ((editor != nullptr && editor->isResizable()) || ! minimumSize.isOrigin(), false);

            if (! minimumSize.isOrigin())
                setResizeLimits (minimumSize.x, minimumSize.y, 8192, 8192);

            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        ~Window() override
        {
            if (keyListener != nullptr)
                removeKeyListener (keyListener);

            clearContentComponent();
        }

        void closeButtonPressed() override
        {
            juce::MessageManager::callAsync ([&o = owner, k = key] { o.closeFor (k); });
        }

        PluginWindowManager& owner;
        const void* key;
        juce::KeyListener* keyListener;
    };

    juce::OwnedArray<Window> windows;
    juce::KeyListener* keyListener = nullptr;
};
} // namespace daw
