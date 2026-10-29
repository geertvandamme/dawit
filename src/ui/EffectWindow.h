#pragma once

#include "engine/EffectTools.h"
#include "GuideButton.h"
#include "GuideSource.h"
#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

/**
    What the windows of DawIt Reverb and DawIt Delay share: the explainer on the right that tells
    about whatever the mouse rests on, the knobs in groups, the graph of the last 4 seconds, the
    "What is ...?" guide in a window of its own, and the frame around it all.
*/
namespace daw::fxui
{
/** Topics every effect window has. The effect's knobs are 0 up, its own pictures from 50. */
enum CommonTopic { overview = 100, presets, clap, listen, guide, history };

/** True when the processor runs as a VST3 or AU plugin in a host, rather than inside DawIt. */
bool isPlugin (const juce::AudioProcessor&);

/** For an editor in a plugin: DawIt's look and feel and a tooltip window, so the window looks and explains itself as
    in DawIt. In DawIt it does nothing. Declare it as the editor's first member, so it outlives the controls. */
class PluginMode final
{
public:
    explicit PluginMode (juce::AudioProcessorEditor&);
    ~PluginMode();

private:
    juce::AudioProcessorEditor& editor;
    std::unique_ptr<DawLookAndFeel> lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltips;
};

/** The track holding a processor, or an empty name for a stand-in processor. In a plugin the lookup is not
    installed and the answer is "This track". */
struct HostTrack
{
    juce::String name;
    bool isReturn = false, isBus = false;
};

/** Installs the app lookup; it reads the edit under its lock on the message thread. */
void setHostTrackLookup (std::function<HostTrack (const juce::AudioProcessor&)>);
/** Finds the processor's track without keeping a pointer into the edit. */
HostTrack getHostTrack (const juce::AudioProcessor&);

/** Marks a control with what the explainer should tell about it when the mouse rests on it. */
void setTopic (juce::Component&, int topic);

void drawPanel (juce::Graphics&, juce::Rectangle<float>);

/** In a plugin, adds the "Part of DawIt" section to an overview; in DawIt it adds nothing. */
void addPluginParagraph (juce::AttributedString&, const juce::AudioProcessor&);

void addTitle (juce::AttributedString&, const juce::String&, juce::Colour = theme::text);
void addLead (juce::AttributedString&, const juce::String&);
void addSection (juce::AttributedString&, const juce::String& heading, const juce::String& text);

/** "1.80 s", "350 ms". */
juce::String formatSeconds (double seconds);

//==============================================================================
/** Explanations, scrolled with the wheel when they run long. */
class ExplainerPanel final : public juce::Component
{
public:
    /** Shows the text. A new `page` starts at the top; the same page keeps where it was scrolled to. */
    void show (const juce::AttributedString&, const juce::String& page);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::AttributedString text;
    juce::String shownText, shownPage;
    juce::TextLayout layout;
    float scroll = 0.0f;
};

//==============================================================================
/** One knob: its name, the dial and its value. */
class EffectKnob final : public juce::Component
{
public:
    EffectKnob (juce::AudioProcessorValueTreeState&, const juce::String& parameterId, const juce::String& name, int topic, juce::Colour);

    /** Makes the dial follow another parameter, as the delay's time does when it follows the tempo. */
    void attachTo (const juce::String& parameterId);

    void resized() override;

    juce::Label label;
    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };

private:
    juce::AudioProcessorValueTreeState& state;
    juce::String attachedId;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
/** The last 4 seconds, newest on the right: how loud the sound coming in was (grey) and what the
    effect added (in its colour), with the beats of the song. */
class LevelHistoryView final : public juce::Component
{
public:
    LevelHistoryView (fx::LevelHistory&, std::function<double()> getSampleRate, std::function<double()> getBpm,
                      const juce::String& effectName, juce::Colour, const juce::String& inputName = "the track",
                      const juce::String& effectSuffix = ", as loud as it is mixed");

    /** Takes the newest points. Called by the editor's timer. */
    void update();

    /** Over the 4 seconds: the loudest the sound and the effect were, in dB. */
    std::pair<float, float> getLoudest() const;

    void paint (juce::Graphics&) override;

private:
    double getPointsPerSecond() const;
    std::vector<fx::LevelPoint> getNewest (int count) const;

    fx::LevelHistory& history;
    std::function<double()> sampleRate, bpm;
    juce::String name, inputName, effectSuffix;
    juce::Colour colour;
    std::vector<fx::LevelPoint> ring, incoming;
    int ringPosition = 0, filled = 0;

    static constexpr double seconds = 4.0;
};

//==============================================================================
/**
    A guide for beginners, such as "What is reverb?": the topics on the left, a picture of the
    selected one, and its explanation. A button plays the effect's test sound (a clap or a note);
    pictures that move get the time since it was last played.
*/
class Guide final : public juce::Component, public guideprint::Source, private juce::ListBoxModel, private juce::Timer
{
public:
    struct Page
    {
        juce::String name, summary, text;

        /** Draws the picture; the last argument is the seconds since the clap (large when there was none). */
        std::function<void (juce::Graphics&, juce::Rectangle<float>, double)> paint;
        /** Optional test sound for this page; otherwise the guide's usual sound is used. */
        std::function<void()> ownClap {};
        juce::String clapText {}, clapTooltip {};
        /** Optional live explanation, refreshed as the song and settings change. */
        std::function<juce::String()> liveText {};
    };

    /** Without a `clap` function, the guide has no test-sound button. */
    Guide (const juce::String& title, std::vector<Page>, std::function<void()> clap, const juce::String& hint,
           const juce::String& clapText = "Clap");
    ~Guide() override;

    void showPage (int index);

    /** Plays the selected page's test sound, also used by screenshot scenes. */
    void playTestSound();

    /** Gives the guide its "Export PDF..." button and says what its pictures follow ("the settings in DawIt
        Reverb's window"); `plugin` is the effect's identifier, which finds its entry in the Theory Guides list. */
    void setPrintSource (const juce::String& text, const juce::String& plugin);

    juce::String getPrintTitle() const override;
    juce::String getPrintSource() const override { return printSource; }
    int getNumPrintTopics() const override { return (int) pages.size(); }
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void timerCallback() override;

    std::vector<Page> pages;
    std::function<void()> onClap;
    juce::String defaultClapText;
    int page = 0;
    double clapTime = -100.0;

    juce::Label title, hintLabel;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextButton clapButton { "Clap" };
    juce::TextEditor explanation;
    std::unique_ptr<juce::Component> exportButton;   // from guideprint::exportButtonFactory, if the app has one
    juce::String printSource;
    int pageBeforePrinting = -1;
    static constexpr double printSeconds = 1.0;   // the moment after the clap that a moving picture is printed at

    static constexpr int width = 1080, height = 800, pictureHeight = 360;
};

//==============================================================================
/** The window a "What is ...?" guide opens in, one per effect window. Closing it hides it, so it opens again on the
    page it showed. */
class GuideWindow final : public juce::DocumentWindow
{
public:
    GuideWindow (const juce::String& name, Guide*);

    /** Shows it at a page (-1 keeps the one it shows) and brings it to the front. */
    void show (int page);

    void closeButtonPressed() override { setVisible (false); }
};

//==============================================================================
/** Opens an editor's guide: in DawIt a window of its own (one per editor, hidden on close, so it reopens on its
    page); in a plugin an overlay that fills the editor's window, with a "Back to the controls" button top left,
    closed by Esc too, and kept so it reopens on the page it showed. Declare it after `PluginMode`. */
class GuideHost final : private juce::ComponentListener
{
public:
    explicit GuideHost (juce::AudioProcessorEditor&);
    ~GuideHost() override;

    /** Opens the guide at a page (-1 keeps the page it shows). `makeGuide` runs the first time only; `printText` says
        what the pictures follow, for the PDF. */
    void show (const juce::String& windowTitle, const std::function<Guide*()>& makeGuide, const juce::String& printText,
               int page);

    /** True while the plugin overlay covers the controls. */
    bool isOverlayShowing() const;

private:
    class Overlay;
    void componentMovedOrResized (juce::Component&, bool, bool) override;

    juce::AudioProcessorEditor& editor;
    std::unique_ptr<GuideWindow> window;
    std::unique_ptr<Overlay> overlay;
};

//==============================================================================
/**
    The frame of an effect's window: a header with the title, the starting points, the guide, a
    clap and "hear only the effect"; the knobs in named groups at the bottom; the explainer on the
    right. The effect puts its pictures in the rest and writes the explanations.
*/
class EffectEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    ~EffectEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;

    /** Opens the "What is ...?" guide, or brings it to the front, at a page (-1 keeps the one it shows). */
    void openGuide (int page = -1);

protected:
    EffectEditor (juce::AudioProcessor&, const juce::String& title, const juce::String& guideTitle, const juce::String& listenText,
                  std::atomic<bool>& listen, fx::Clap&, juce::Colour, fx::Clap::Sound = fx::Clap::Sound::clap);

    /** A named group of the knob area: knobs or other controls (each with a label above it), side by
        side, on the first row or, for a window with many controls, a row below it. */
    struct Group
    {
        juce::String name;
        std::vector<juce::Component*> cells;
        int row = 0;
    };

    void setGroups (std::vector<Group>);
    void setPresetNames (const juce::StringArray&);

    /** Shows the name of the starting point just picked, selecting nothing so it can be picked again. */
    void showPresetName (const juce::String&);

    /** Tells about this topic now (after a click, say) rather than when the mouse rests on it. */
    void showTopic (int topic);

    int getTopic() const noexcept { return topic; }

    /** The mouse rests on something with this topic; the explainer follows after a moment. */
    void hover (std::optional<int>);

    /** The explanation of a topic: the effect's knobs (0 up), its pictures (50 up) and the CommonTopics. */
    virtual juce::AttributedString buildText (int topic) = 0;

    /** Places the effect's pictures. */
    virtual void layoutPictures (juce::Rectangle<int>) = 0;

    virtual void applyPreset (int index) = 0;
    virtual std::vector<Guide::Page> createGuidePages() = 0;

    /** 30 times a second: repaint what moves. */
    virtual void tick() {}

    /** After a clap from the window or the guide, for pictures that show it. */
    virtual void clapped() {}

    /** A small switch with a label above it, for the knob area. */
    struct Switch final : public juce::Component
    {
        Switch (const juce::String& name, int topic);
        void resized() override;

        juce::Label label;
        juce::TextButton button;
    };

    const juce::Colour colour;
    juce::Label title;
    juce::ComboBox presets;
    juce::TextButton clapButton { "Clap" }, listenButton, helpButton { "?" };
    GuideButton guideButton;
    ExplainerPanel explainer;

private:
    void timerCallback() override;
    juce::AttributedString getText (int topic);

    PluginMode pluginMode { *this };
    std::atomic<bool>& listen;
    fx::Clap& clap;
    const fx::Clap::Sound clapSound;
    juce::String guideTitle;
    std::vector<Group> groups;
    std::vector<juce::Rectangle<int>> groupAreas;
    GuideHost guideHost { *this };

    int topic = overview;
    std::optional<int> pendingTopic;
    int pendingTicks = 0, ticks = 0;

    static constexpr int headerHeight = 44, knobsHeight = 150, explainerWidth = 360;
};
} // namespace daw::fxui
