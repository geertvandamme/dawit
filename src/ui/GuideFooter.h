#pragma once

#include "GuideNavigator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw
{
/**
    The row under a guide window's explanation, 32 px high: *See also* chips to related pages in other guides (at most
    three, the rest under *+N*), *Back* to where a link came from, a *Read* toggle and *Next page*. It is owned by each
    guide window like `GuideSourcesTopic`; the window tells it which page shows with `setPage`, lays it out under the
    text, and calls `keyPressed` from its own for Cmd+[ (Back) and Cmd+] (Next page).

    A page counts as read after 10 seconds on screen in the front window, or when you leave it with *Next page*; the
    *Read* toggle ticks or unticks it by hand. The footer is hidden when the app has no `GuideNavigator` (in a plugin)
    and is not printed.
*/
class GuideFooter final : public juce::Component, public juce::TooltipClient, private juce::ChangeListener, private juce::Timer
{
public:
    GuideFooter();
    ~GuideFooter() override;

    static constexpr int height = 32;

    /** True when the app has a navigator, so the footer can show. */
    static bool isAvailable() noexcept;

    /** True while the footer is showing a page: the window gives it `height` pixels under the text. */
    bool isActive() const noexcept { return active; }

    /** Shows the footer for a page of a guide (the topic by name; "Sources" and "Your ..." pages included). */
    void setPage (gen::guides::Guide, const juce::String& topic);

    /** Hides the footer, for a window that shows something with no page (or nothing). */
    void clearPage();

    /** Shows the page (or hides the footer when the app has no navigator). True when the footer appeared or went away,
        so the window must lay out again. */
    bool update (gen::guides::Guide, const juce::String& topic);

    /** Called from the window's `resized()` before it places its text: when the footer is showing, puts it at the bottom
        of `area` and takes its space off. */
    void takeSpace (juce::Rectangle<int>& area);

    /** Called when the read ticks changed, so the window can repaint its list. */
    std::function<void()> onReadChanged;

    /** Cmd+[ goes Back, Cmd+] goes to Next page. True when the key was used. */
    bool keyPressed (const juce::KeyPress&) override;

    /** Draws the small dim tick at the right of a topic row when the page is read. */
    static void paintTick (juce::Graphics&, int rowWidth, int rowHeight, gen::guides::Guide, const juce::String& topic);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    enum class Kind { label, chip, more, back, read, next };

    struct Part
    {
        Kind kind = Kind::label;
        int index = 0;
        juce::Rectangle<float> area;
        juce::String text, tooltip;
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void rebuild();
    int getPartAt (juce::Point<float>) const;
    void showMoreMenu();
    void openLink (int index);

    GuideNavigator* navigator = nullptr;
    bool active = false;
    gen::guides::Guide guide = gen::guides::Guide::sound;
    juce::String topic;
    std::vector<GuideNavigator::Link> links;
    std::vector<Part> parts;
    int shownLinks = 0;
    int hovered = -1;
    int secondsOnPage = 0;
};
} // namespace daw
