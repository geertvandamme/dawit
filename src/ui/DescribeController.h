#pragma once

#include "EffectWindow.h"
#include "engine/DescribedSound.h"
#include "generate/SoundDescribe.h"

#include <functional>
#include <vector>

namespace daw
{
/** The name of the guide page the bar and the explainer open: "Describe a sound to the AI". */
inline const juce::String describePageName { "Describe a sound to the AI" };

/** What the bar of "Describe a sound" asks of the window it sits in. Implemented by `DescribeController`. */
class DescribeHost
{
public:
    virtual ~DescribeHost() = default;

    /** Opens or closes the bar; the pictures shrink or grow to fit. */
    virtual void setDescribeBarOpen (bool) = 0;

    /** Puts a ring round the controls with these knob indices (a walkthrough's step); none removes it. */
    virtual void setDescribeHighlight (const std::vector<int>&) = 0;

    /** Shows the last description's changes and reasons in the explainer; with `setupSteps` those lines (how to
        set up Ollama) instead. */
    virtual void showDescription (const juce::StringArray& setupSteps = {}) = 0;

    /** Opens the guide at the page about describing sounds. */
    virtual void openDescribeGuide() = 0;
};

//==============================================================================
/** Everything about one synth that the shared bar and window part need: its describe table, its sound as values and
    back, where its last description is kept, which component shows a knob, and its words. Built by the synth's editor. */
struct DescribeConfig
{
    const gen::sounddescribe::Table* table = nullptr;
    juce::String recentKey;                  // the settings key of the recent descriptions
    juce::Colour colour;                     // the synth's colour
    int topic = 130;                         // the explainer topic of the answer
    DescribedSound* last = nullptr;          // the synth's last description (memory only)

    std::function<gen::sounddescribe::Values()> getLive;               // the sound in the window now
    std::function<void (const gen::sounddescribe::Values&)> setLive;   // sets it on the synth (message thread)
    std::function<juce::Component* (int knob)> getComponent;           // the control to ring for a knob index, or null
    std::function<void (const std::vector<int>&)> onHighlight;         // optional: the knob indices now lit
    std::function<juce::String (int knob)> getTitle;                   // optional: the control's name as the window shows it (else the table's)
    std::function<void (int topic)> showTopic;                         // shows the explainer topic now
    std::function<void()> openGuide;                                   // opens the guide at describePageName

    // Words that differ between synths.
    juce::String describeTip, walkTip, setForMeTip, backTip;
    juce::String exampleWords;               // "a warm pad that swells in slowly"
    juce::String whatItSets = "knobs";       // "bars and knobs"
    juce::String stepUnit = "one knob at a time";
    juce::String allUnit = "every knob at once";
    juce::String hoverHint = "hover a knob for DawIt's own explanation.";
};

/** The bar of "Describe a sound", across the top of the pictures. Only the app has one (`DescribeBar`, made through
    `DescribeController::barFactory`), so the plugin builds don't compile the AI code. */
class DescribeBarBase : public juce::Component
{
public:
    /** The height the bar takes when open. */
    static constexpr int barHeight = 76;

    /** 30 times a second while the bar is open (the editor's tick): follows the knobs in a walkthrough. */
    virtual void tick() {}
    /** The bar was just opened: put the cursor in the text box and look for the models. */
    virtual void opened() {}
    /** The bar was just closed: end a walkthrough, cancel a request. */
    virtual void closed() {}
};

//==============================================================================
/**
    The part of a synth's window that belongs to "Describe a sound" and is the same for every synth: the header
    button, the bar across the top of the pictures, the ring round the controls of a walkthrough step, the answer
    in the explainer and the link to the guide under it. It lives in the plugin builds too, but there the factory
    is null and so it has no button or bar. The editor feeds it a `DescribeConfig` and calls `layout`, `layoutBar`,
    `tick`, `paintRing` and `buildText` from its own functions.
*/
class DescribeController final : public DescribeHost
{
public:
    /** Set by the app to make the bar; null in a plugin, so there is no button there. */
    static inline std::function<std::unique_ptr<DescribeBarBase> (DescribeHost&, const DescribeConfig&)> barFactory;

    /** `owner` is the editor; the button, the link and the bar become its children (only when `allowBar` and the factory is set). */
    DescribeController (juce::Component& owner, DescribeConfig, bool allowBar);

    bool hasBar() const noexcept { return bar != nullptr; }
    bool isOpen() const noexcept { return open; }
    DescribeBarBase* getBar() noexcept { return bar.get(); }

    void setDescribeBarOpen (bool) override;
    void setDescribeHighlight (const std::vector<int>&) override;
    void showDescription (const juce::StringArray& setupSteps = {}) override;
    void openDescribeGuide() override;

    /** Puts the button where the guide button was and the guide button right of it; the link under the answer. */
    void layout (juce::Button& guideButton, juce::Rectangle<int> explainerBounds);

    /** Takes the bar's space off the top of the pictures' area. */
    void layoutBar (juce::Rectangle<int>& pictures);

    /** The editor's tick: follows a walkthrough and shows the guide link only under the answer. */
    void tick (int currentTopic);

    /** The ring round the controls of the step, over the editor's children. */
    void paintRing (juce::Graphics&) const;

    /** The answer, the empty state or the Ollama set-up steps as the explainer's text for the topic. */
    void buildText (juce::AttributedString&) const;

    const std::vector<int>& getHighlight() const noexcept { return highlight; }

private:
    juce::Component& owner;
    DescribeConfig config;
    juce::TextButton button { "Describe a sound" };
    juce::TextButton guideLink { "Read about describing sounds" };   // under the answer in the explainer
    std::unique_ptr<DescribeBarBase> bar;
    bool open = false;
    std::vector<int> highlight;
    juce::StringArray setupSteps;   // not empty: the explainer shows how to set up Ollama
};
} // namespace daw
