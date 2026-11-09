#pragma once

#include "generate/TheoryGuides.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <optional>
#include <vector>

namespace daw
{
/**
    Moves the reader between the theory guides: follows a *See also* link or *Next page* by opening the guide at the
    page, keeps the *Back* history (the session's last 20 jumps, not saved), and keeps which pages are read
    (`guides.read` in the app settings, one "<guide key>/<topic name>" per line).

    `MainComponent` owns the one navigator and makes it the app-wide `getInstance()`; it is null in a plugin and in the
    tests, which is how a guide window knows to hide its footer. The navigator broadcasts a change when the read pages
    or the history change, so open windows and the Theory Guides window repaint their ticks.
*/
class GuideNavigator final : public juce::ChangeBroadcaster
{
public:
    using Entries = std::vector<gen::guides::Entry>;

    /** `makeEntries` gives the hub's entries (the built-ins' pages filled in); it runs once, when first needed.
        `open` opens a guide at a topic number of its entry, as `MainComponent::openGuide` does. */
    GuideNavigator (juce::PropertiesFile* settings, std::function<Entries()> makeEntries,
                    std::function<void (gen::guides::Guide, int)> open);
    ~GuideNavigator() override;

    /** The navigator of the running app; null in a plugin and in the tests. */
    static GuideNavigator* getInstance() noexcept;

    /** The entries every lookup uses. */
    const Entries& getEntries();

    /** The guide's entry; null if unknown. */
    const gen::guides::Entry* findEntry (gen::guides::Guide);

    /** "EQ", "Reverb": the guide's name without "What is" and the question mark, for a link chip. */
    static juce::String getShortName (const gen::guides::Entry&);

    //==============================================================================
    /** One *See also* chip: the page it opens, its text ("EQ · The frequency ranges") and its tooltip. */
    struct Link
    {
        gen::guides::PageRef to;
        juce::String text, tooltip;
    };

    /** The links of a page, from both directions of the table. */
    std::vector<Link> getLinks (gen::guides::Guide, const juce::String& topic);

    /** Opens the linked page; Back then returns to `from`. */
    void follow (gen::guides::Guide fromGuide, const juce::String& fromTopic, const gen::guides::PageRef& to);

    /** Follows the n-th (1-based) link of the page last shown; for the screenshot runner. False if there is none. */
    bool followNthLink (int n);

    //==============================================================================
    /** Where Back goes: a page and the label for the button. */
    struct BackTarget
    {
        gen::guides::Guide guide = gen::guides::Guide::sound;
        juce::String topic, label;
    };

    /** Back, for the window of `shown`: only the window a jump opened offers it. */
    std::optional<BackTarget> getBack (gen::guides::Guide shown);

    /** Opens the page Back names and drops it from the history. */
    void goBack();

    //==============================================================================
    /** Where Next page goes from a page, and the label for the button. */
    struct NextTarget
    {
        gen::guides::Guide guide = gen::guides::Guide::sound;
        int topic = 0;
        juce::String label;
        bool otherGuide = false;
    };

    /** Next page by the plan's rule (the next topic, then the next guide on Start here, skipping Sources). None when
        the page is not found in its entry or nothing follows. */
    std::optional<NextTarget> getNext (gen::guides::Guide, const juce::String& topic);

    /** Opens the next page and ticks this one read. Going to another guide is a jump: Back returns. */
    void goNext (gen::guides::Guide, const juce::String& topic);

    /** Follows Next page from the page last shown; for the screenshot runner. */
    bool goNextFromShown();

    //==============================================================================
    /** The pages read: ids "<guide key>/<topic name>". Read from the settings each time, so they are always current. */
    juce::StringArray getReadIds() const;

    bool isRead (gen::guides::Guide, const juce::String& topic) const;

    /** Ticks or unticks a page. Only pages that count (not Sources, not "Your ...") are kept. */
    void setRead (gen::guides::Guide, const juce::String& topic, bool read);

    /** Forgets every page read. */
    void markAllUnread();

    //==============================================================================
    /** Tells the navigator which page a window shows (the footer does), so the runner and Back know. */
    void setShown (gen::guides::Guide, const juce::String& topic);

    /** Opens the Theory Guides window; what the last item of a footer's right-click menu runs. */
    std::function<void()> onShowGuides;

    static constexpr int maxHistory = 20;

private:
    struct Place
    {
        gen::guides::Guide guide;
        juce::String topic;
    };

    void jumpTo (const Place& from, gen::guides::Guide, int topic);
    void saveReadIds (const juce::StringArray&);

    juce::PropertiesFile* settings;
    std::function<Entries()> makeEntries;
    std::function<void (gen::guides::Guide, int)> openGuide;
    std::optional<Entries> entries;
    std::vector<Place> history;
    gen::guides::Guide lastJumpGuide = gen::guides::Guide::sound;
    std::optional<Place> shown;
};
} // namespace daw
