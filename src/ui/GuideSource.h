#pragma once

#include "engine/GuideBooklet.h"

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace daw::guideprint
{
/** A guide window that can be saved as a booklet: it shows its topics one by one, and says what its pictures show. */
class Source
{
public:
    virtual ~Source() = default;

    /** "What is reverb?" */
    virtual juce::String getPrintTitle() const = 0;

    /** What the pictures are made from: "the settings in Vocals - DawIt Reverb", "the clip Lead in the piano roll". */
    virtual juce::String getPrintSource() const = 0;

    virtual int getNumPrintTopics() const = 0;

    /** Shows topic `index` as a click in its list would (nothing plays). Returns its name, summary and text, and
        the picture's size on screen; the picture itself stays empty. */
    virtual booklet::Topic showPrintTopic (int index) = 0;

    /** The picture of the topic shown, `scale` times its size on screen. */
    virtual juce::Image paintPrintPicture (float scale) = 0;

    /** Back to what the window showed before the export. */
    virtual void endPrinting() = 0;
};

/** Makes the "Export PDF..." button for a guide of a built-in effect or synth, or nothing. The app installs it
    (`MainComponent`); a plugin build has none, so its guides have no Export button. */
using ExportButtonFactory = std::function<std::unique_ptr<juce::Component> (Source&, const juce::String& pluginIdentifier)>;

/** The hook: set by the app while it exists; empty in a plugin. */
inline ExportButtonFactory& exportButtonFactory()
{
    static ExportButtonFactory factory;
    return factory;
}
} // namespace daw::guideprint
