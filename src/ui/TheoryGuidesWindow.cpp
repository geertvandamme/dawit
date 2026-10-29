#include "TheoryGuidesWindow.h"
#include "AdditiveGuide.h"
#include "DelayGuide.h"
#include "DistortionGuide.h"
#include "GateGuide.h"
#include "StereoGuide.h"
#include "TuneGuide.h"
#include "DrumsGuide.h"
#include "FmGuide.h"
#include "PhysicalGuide.h"
#include "ReverbGuide.h"
#include "SaturationGuide.h"
#include "SubtractiveGuide.h"
#include "CompressorEditor.h"
#include "EqEditor.h"
#include "engine/BuiltInFormat.h"

namespace daw
{
namespace guides = gen::guides;

namespace
{
    juce::Array<juce::PluginDescription> getBuiltIns()
    {
        auto all = BuiltInFormat::getEffects();
        all.addArray (BuiltInFormat::getInstruments());
        return all;
    }

    std::unique_ptr<juce::AudioPluginInstance> createBuiltIn (const juce::String& identifier)
    {
        for (auto& d : getBuiltIns())
        {
            if (d.fileOrIdentifier == identifier)
            {
                BuiltInFormat format;
                juce::String error;
                return format.createInstanceFromDescription (d, 44100.0, 512, error);
            }
        }

        return {};
    }

    bool isBuiltIn (const juce::AudioPluginInstance& p, const juce::String& identifier)
    {
        const auto d = p.getPluginDescription();
        return d.pluginFormatName == BuiltInFormat::formatName && d.fileOrIdentifier == identifier;
    }
} // namespace

//==============================================================================
/** A built-in effect or synth on no track, playing straight to the output: silence through it, plus its clap or test note. */
class BuiltInGuides::StandIn final : public juce::AudioIODeviceCallback
{
public:
    StandIn (std::unique_ptr<juce::AudioPluginInstance> p, juce::AudioDeviceManager& d) : processor (std::move (p)), devices (d)
    {
        devices.addAudioCallback (this);
    }

    ~StandIn() override { devices.removeAudioCallback (this); }

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override
    {
        const int block = juce::jmax (512, device->getCurrentBufferSizeSamples());
        processor->setRateAndBufferSizeDetails (device->getCurrentSampleRate(), block);
        processor->prepareToPlay (device->getCurrentSampleRate(), block);
        buffer.setSize (2, block);
        prepared = true;
    }

    void audioDeviceStopped() override
    {
        prepared = false;
        processor->releaseResources();
    }

    void audioDeviceIOCallbackWithContext (const float* const*, int, float* const* outputs, int numOutputs, int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override
    {
        for (int ch = 0; ch < numOutputs; ++ch)
            if (outputs[ch] != nullptr)
                juce::FloatVectorOperations::clear (outputs[ch], numSamples);

        if (! prepared || numSamples > buffer.getNumSamples())
            return;

        // Silence in: what comes out is only the test sound through it (an effect never hears the microphone).
        juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), 2, numSamples);
        block.clear();
        midi.clear();
        processor->processBlock (block, midi);

        for (int ch = 0; ch < juce::jmin (2, numOutputs); ++ch)
            if (outputs[ch] != nullptr)
                juce::FloatVectorOperations::copy (outputs[ch], block.getReadPointer (ch), numSamples);
    }

    const std::unique_ptr<juce::AudioPluginInstance> processor;

private:
    juce::AudioDeviceManager& devices;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
    bool prepared = false;
};

BuiltInGuides::BuiltInGuides (Engine& e, Selection& s, PluginWindowManager& w) : engine (e), selection (s), pluginWindows (w)
{
}

BuiltInGuides::~BuiltInGuides()
{
    for (auto& s : standIns)
        pluginWindows.closeFor (*s->processor);
}

juce::AudioPluginInstance* BuiltInGuides::find (const juce::String& identifier, const Track** onTrack) const
{
    const auto findOn = [&identifier] (const Track& track) -> juce::AudioPluginInstance*
    {
        for (auto& slot : track.plugins)
            if (slot->instance != nullptr && isBuiltIn (*slot->instance, identifier))
                return slot->instance.get();

        return nullptr;
    };

    std::vector<const Track*> tracks;

    if (auto* selected = selection.getTrack())
        tracks.push_back (selected);

    for (auto& t : engine.getTracks())
        tracks.push_back (t.get());

    for (auto* t : tracks)
    {
        if (auto* p = findOn (*t))
        {
            if (onTrack != nullptr)
                *onTrack = t;

            return p;
        }
    }

    return nullptr;
}

juce::String BuiltInGuides::describeTarget (const juce::String& identifier) const
{
    const Track* track = nullptr;

    if (find (identifier, &track) != nullptr && track != nullptr)
        return "Opens the one on " + track->name;

    return "Opens one on no track, to try out";
}

void BuiltInGuides::open (const juce::String& identifier, int topic)
{
    const Track* track = nullptr;
    auto* processor = find (identifier, &track);
    juce::String title = track != nullptr ? track->name + " - " + identifier : identifier + " (on no track)";

    if (processor == nullptr)
    {
        for (auto& s : standIns)
            if (isBuiltIn (*s->processor, identifier))
                processor = s->processor.get();

        if (processor == nullptr)
        {
            auto made = createBuiltIn (identifier);

            if (made == nullptr)
                return;

            standIns.push_back (std::make_unique<StandIn> (std::move (made), engine.deviceManager));
            processor = standIns.back()->processor.get();
        }
    }

    pluginWindows.show (*processor, title);
    auto* editor = processor->getActiveEditor();

    if (auto* effect = dynamic_cast<fxui::EffectEditor*> (editor))
        effect->openGuide (juce::jmax (0, topic));
    else if (auto* eq = dynamic_cast<BuiltInEqEditor*> (editor))
        eq->openGuide (juce::jmax (0, topic));
    else if (auto* compressor = dynamic_cast<BuiltInCompressorEditor*> (editor))
        compressor->openGuide (juce::jmax (0, topic));
}

std::vector<fxui::Guide::Page> BuiltInGuides::makePages (juce::AudioProcessor& p)
{
    std::vector<fxui::Guide::Page> pages;
    auto* processor = &p;

    if (auto* eqProcessor = dynamic_cast<BuiltInEq*> (processor))
        pages = eqtext::makeGuide (*eqProcessor);
    else if (auto* compressor = dynamic_cast<BuiltInCompressor*> (processor))
        pages = comptext::makeGuide (*compressor);
    else if (auto* gateProcessor = dynamic_cast<BuiltInGate*> (processor))
        pages = gatetext::makeGuide (*gateProcessor);
    else if (auto* stereoProcessor = dynamic_cast<BuiltInStereo*> (processor))
        pages = stereotext::makeGuide (*stereoProcessor);
    else if (auto* tuneProcessor = dynamic_cast<BuiltInTune*> (processor))
        pages = tunetext::makeGuide (*tuneProcessor);
    else if (auto* reverb = dynamic_cast<BuiltInReverb*> (processor))
        pages = verbtext::makeGuide (*reverb);
    else if (auto* delay = dynamic_cast<BuiltInDelay*> (processor))
        pages = dlytext::makeGuide (*delay);
    else if (auto* saturation = dynamic_cast<BuiltInSaturation*> (processor))
        pages = sattext::makeGuide (*saturation);
    else if (auto* distortion = dynamic_cast<BuiltInDistortion*> (processor))
        pages = disttext::makeGuide (*distortion);
    else if (auto* subtractive = dynamic_cast<BuiltInSubtractive*> (processor))
        pages = subtext::makeGuide (*subtractive);
    else if (auto* additive = dynamic_cast<BuiltInAdditive*> (processor))
        pages = additext::makeGuide (*additive);
    else if (auto* fm = dynamic_cast<BuiltInFm*> (processor))
        pages = fmtext::makeGuide (*fm);
    else if (auto* drums = dynamic_cast<BuiltInDrums*> (processor))
        pages = drumtext::makeGuide (*drums);
    else if (auto* physical = dynamic_cast<BuiltInPhysical*> (processor))
        pages = phystext::makeGuide (*physical);

    return pages;
}

juce::StringArray BuiltInGuides::getPageNames (const juce::String& identifier)
{
    auto processor = createBuiltIn (identifier);
    const auto pages = processor != nullptr ? makePages (*processor) : std::vector<fxui::Guide::Page>();
    juce::StringArray names;

    for (auto& p : pages)
        names.add (p.name);

    return names;
}

guideprint::Item BuiltInGuides::getPrintGuide (const juce::String& identifier)
{
    guideprint::Item item;
    const auto entries = guides::getEntries();
    const auto found = std::find_if (entries.begin(), entries.end(), [&] (auto& e) { return e.plugin == identifier; });

    if (found == entries.end())
        return item;

    item.guide = found->guide;
    const Track* track = nullptr;
    auto* processor = find (identifier, &track);
    juce::String text;

    if (processor != nullptr)
    {
        text = identifier + (track != nullptr ? " on " + track->name : juce::String()) + ", as set in its window";
    }
    else
    {
        item.plugin = createBuiltIn (identifier);

        if (item.plugin == nullptr)
            return item;

        item.plugin->prepareToPlay (44100.0, 512);
        processor = item.plugin.get();
        text = "a " + identifier + " with its starting settings (none in the song)";
    }

    auto pages = makePages (*processor);
    auto guide = std::make_unique<fxui::Guide> (found->name, std::move (pages), nullptr, juce::String());
    guide->setPrintSource (text, identifier);
    item.sourceText = text;
    item.adopt (std::move (guide));
    return item;
}

std::vector<guides::Entry> BuiltInGuides::getEntries()
{
    auto entries = guides::getEntries();

    for (auto& e : entries)
        if (e.plugin.isNotEmpty() && e.topics.isEmpty())
            e.topics = getPageNames (e.plugin);

    return entries;
}

//==============================================================================
/** A guide: its name, what it covers, where else it opens from, and its topics as chips to click. */
class TheoryGuidesView::Card final : public juce::Component, public juce::TooltipClient
{
public:
    Card (const guides::Entry& e, int pathNumber, std::vector<int> matchedTopics, std::function<juce::String()> target)
        : entry (e), number (pathNumber), matched (std::move (matchedTopics)), getTarget (std::move (target)),
          colour (getColour (e.area))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    /** Opens the guide at a topic, or -1 at its start. */
    std::function<void (int)> onOpen;

    /** Saves just this guide as a PDF. */
    std::function<void()> onExport;

    /** Lays the card out for a width, and returns its height. */
    int layout (int w)
    {
        const float left = (float) (stripe + pad), right = (float) (w - pad);
        float y = (float) pad;

        nameArea = { left + (number > 0 ? 32.0f : 0.0f), y, right - left - (number > 0 ? 32.0f : 0.0f) - 70.0f, 22.0f };
        y += 26.0f;

        summary = makeLayout (entry.summary, 14.0f, theme::text, right - left);
        summaryArea = { left, y, right - left, summary.getHeight() };
        y += summary.getHeight() + 4.0f;

        opensFromArea = { left, y, right - left, 17.0f };
        y += 18.0f;

        if (entry.plugin.isNotEmpty())
        {
            targetArea = { left, y, right - left, 17.0f };
            y += 18.0f;
        }

        chips.clear();
        y += 6.0f;
        float x = left;
        const juce::Font chipFont (juce::FontOptions (12.5f));

        for (auto& topic : entry.topics)
        {
            const float chipWidth = juce::jmin (right - left, juce::GlyphArrangement::getStringWidth (chipFont, topic) + 18.0f);

            if (x > left && x + chipWidth > right)
            {
                x = left;
                y += chipHeight + 6.0f;
            }

            chips.push_back ({ x, y, chipWidth, chipHeight });
            x += chipWidth + 6.0f;
        }

        if (! chips.empty())
            y += chipHeight;

        return (int) std::ceil (y) + pad;
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        juce::Path shape;
        shape.addRoundedRectangle (r, 7.0f);
        g.setColour (hovered == none ? theme::panelLight : theme::panelLight.brighter (0.06f));
        g.fillPath (shape);

        {
            const juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (shape);
            g.setColour (colour);
            g.fillRect (r.withWidth ((float) stripe));
        }

        if (number > 0)
        {
            const auto circle = juce::Rectangle<float> (nameArea.getX() - 32.0f, nameArea.getY() - 1.0f, 24.0f, 24.0f);
            g.setColour (colour);
            g.fillEllipse (circle);
            g.setColour (theme::background);
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawText (juce::String (number), circle, juce::Justification::centred);
        }

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
        g.drawFittedText (entry.name, nameArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);

        g.setColour (hovered == none ? theme::textDim : theme::accent);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (juce::String::fromUTF8 ("Open \xe2\x80\xba"), juce::Rectangle<float> (nameArea.getRight(), nameArea.getY(), 70.0f, nameArea.getHeight()),
                    juce::Justification::centredRight);

        summary.draw (g, summaryArea);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.5f));
        g.drawFittedText ("Also opens from: " + entry.opensFrom, opensFromArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);

        if (entry.plugin.isNotEmpty() && getTarget != nullptr)
        {
            g.setColour (colour.withAlpha (0.9f));
            g.drawFittedText (getTarget() + (entry.plugin == entry.name ? juce::String() : ", in " + entry.plugin + "'s window"),
                              targetArea.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
        }

        g.setFont (juce::FontOptions (12.5f));

        for (int i = 0; i < (int) chips.size(); ++i)
        {
            const auto chip = chips[(size_t) i];
            const bool isMatch = std::find (matched.begin(), matched.end(), i) != matched.end();

            g.setColour (hovered == i ? theme::accent.withAlpha (0.45f) : isMatch ? colour.withAlpha (0.4f) : theme::panel);
            g.fillRoundedRectangle (chip, chipHeight / 2.0f);

            if (isMatch)
            {
                g.setColour (colour);
                g.drawRoundedRectangle (chip.reduced (0.5f), chipHeight / 2.0f, 1.0f);
            }

            g.setColour (theme::text);
            g.drawFittedText (entry.topics[i], chip.reduced (9.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override { setHovered (getPartAt (e.position)); }
    void mouseExit (const juce::MouseEvent&) override { setHovered (none); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            // Right-click: save this guide as a PDF; the last item opens it, as everywhere.
            juce::PopupMenu menu;
            menu.addItem (1, "Export \"" + entry.name + "\" as PDF...", onExport != nullptr);
            menu.addSeparator();
            menu.addItem (2, "Open \"" + entry.name + "\"");
            menu.showMenuAsync (juce::PopupMenu::Options(), [this, safe = juce::Component::SafePointer<Card> (this)] (int result)
            {
                if (safe == nullptr)
                    return;

                if (result == 1 && onExport != nullptr)
                    onExport();
                else if (result == 2 && onOpen != nullptr)
                    onOpen (-1);
            });
            return;
        }

        if (e.mouseWasDraggedSinceMouseDown())
            return;

        if (const int part = getPartAt (e.position); part != none && onOpen != nullptr)
            onOpen (part == card ? -1 : part);
    }

    juce::String getTooltip() override
    {
        const int part = getPartAt (getMouseXYRelative().toFloat());

        if (part >= 0)
            return "Open " + entry.name + " at \"" + entry.topics[part] + "\"";

        return part == card ? "Open " + entry.name + " at its start" : juce::String();
    }

private:
    static constexpr int none = -3, card = -1, stripe = 5, pad = 14;
    static constexpr float chipHeight = 24.0f;

    static juce::TextLayout makeLayout (const juce::String& text, float size, juce::Colour c, float w)
    {
        juce::AttributedString s;
        s.append (text, juce::FontOptions (size), c);
        s.setLineSpacing (2.0f);
        juce::TextLayout l;
        l.createLayout (s, w);
        return l;
    }

    int getPartAt (juce::Point<float> p) const
    {
        for (int i = 0; i < (int) chips.size(); ++i)
            if (chips[(size_t) i].contains (p))
                return i;

        return getLocalBounds().toFloat().contains (p) ? card : none;
    }

    void setHovered (int part)
    {
        if (hovered != part)
        {
            hovered = part;
            repaint();
        }
    }

    const guides::Entry entry;
    const int number;
    const std::vector<int> matched;
    const std::function<juce::String()> getTarget;
    const juce::Colour colour;

    juce::TextLayout summary;
    juce::Rectangle<float> nameArea, summaryArea, opensFromArea, targetArea;
    std::vector<juce::Rectangle<float>> chips;
    int hovered = none;
};

//==============================================================================
TheoryGuidesView::TheoryGuidesView (std::vector<guides::Entry> e) : entries (std::move (e))
{
    title.setText ("Theory Guides", juce::dontSendNotification);
    title.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    addAndMakeVisible (title);

    intro.setText ("Every guide in DawIt in one place. Each one also opens from where its theory applies, as its card says. "
                   "Export PDF... saves the guides shown as a booklet to print.",
                   juce::dontSendNotification);
    intro.setFont (juce::FontOptions (13.5f));
    intro.setMinimumHorizontalScale (1.0f);
    intro.setColour (juce::Label::textColourId, theme::textDim);
    addAndMakeVisible (intro);

    exportButton.setTooltip ("Save the guides shown here as one PDF booklet: Start here's seven guides, an area's guides, all of them, or the ones "
                             "your search found (Cmd+P)");
    exportButton.setWantsKeyboardFocus (false);
    exportButton.setMouseClickGrabsKeyboardFocus (false);
    exportButton.onClick = [this] { exportShown(); };
    addAndMakeVisible (exportButton);

    searchBox.setTextToShowWhenEmpty ("Search: a word or a question, such as \"rhyme\" or \"muddy vocal\"", theme::textDim);
    searchBox.setFont (juce::FontOptions (14.0f));
    searchBox.setIndents (10, 7);
    searchBox.setColour (juce::TextEditor::backgroundColourId, theme::panelLight);
    searchBox.setColour (juce::TextEditor::textColourId, theme::text);
    searchBox.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, theme::accent);
    searchBox.setTooltip ("Finds the guides and topics that have all the words you type. Esc clears it.");
    searchBox.onTextChange = [this] { refresh(); };
    searchBox.onEscapeKey = [this] { searchBox.clear(); refresh(); };
    addAndMakeVisible (searchBox);

    list.setModel (this);
    list.setRowHeight (50);
    list.setColour (juce::ListBox::backgroundColourId, theme::panelLight);
    list.setOutlineThickness (0);
    addAndMakeVisible (list);

    viewport.setViewedComponent (&cardHolder, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    list.updateContent();
    list.selectRow (startRow);
    setSize (width, height);
}

TheoryGuidesView::~TheoryGuidesView()
{
    list.setModel (nullptr);
}

juce::Colour TheoryGuidesView::getColour (guides::Area a)
{
    switch (a)
    {
        case guides::Area::sound:       return juce::Colour (0xff5cc8ff);
        case guides::Area::midi:        return juce::Colour (0xff7d8bff);
        case guides::Area::rhythm:      return juce::Colour (0xffb38cff);
        case guides::Area::readingMusic: return juce::Colour (0xffe8d36a);
        case guides::Area::harmony:     return juce::Colour (0xffff8fb1);
        case guides::Area::songwriting: return juce::Colour (0xffffa94d);
        case guides::Area::singing:     return juce::Colour (0xffc6e05a);
        case guides::Area::mixing:      return juce::Colour (0xff5fd38d);
        case guides::Area::effects:     return juce::Colour (0xff4fd1c5);
        case guides::Area::synthesis:   return juce::Colour (0xffe0b26b);
    }

    return theme::accent;
}

void TheoryGuidesView::showRow (int row)
{
    if (list.getSelectedRow() != row)
        list.selectRow (row);   // comes back through selectedRowsChanged
}

void TheoryGuidesView::search (const juce::String& query)
{
    searchBox.setText (query, true);
}

int TheoryGuidesView::getNumRows()
{
    return getAllRow() + 1;
}

int TheoryGuidesView::countGuides (int row) const
{
    if (row == startRow)
        return (int) guides::getStartPath().size();

    if (row == getAllRow())
        return (int) entries.size();

    const auto area = guides::allAreas[row - firstAreaRow];
    return (int) std::count_if (entries.begin(), entries.end(), [area] (auto& e) { return e.area == area; });
}

void TheoryGuidesView::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (selected)
    {
        g.setColour (theme::accent.withAlpha (0.35f));
        g.fillRect (0, 0, w, h);
    }

    juce::String name, summary;
    juce::Colour colour = theme::textDim;

    if (row == startRow)
    {
        name = "Start here";
        summary = "A path for beginners";
        colour = theme::accent;
    }
    else if (row == getAllRow())
    {
        name = "All guides";
        summary = "Every guide, area by area";
    }
    else if (juce::isPositiveAndBelow (row - firstAreaRow, (int) std::size (guides::allAreas)))
    {
        const auto area = guides::allAreas[row - firstAreaRow];
        name = guides::getAreaName (area);
        summary = guides::getAreaSummary (area);
        colour = getColour (area);
    }

    g.setColour (colour);
    g.fillEllipse (12.0f, 11.0f, 9.0f, 9.0f);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (name, 30, 5, w - 64, 20, juce::Justification::centredLeft, true);
    g.setColour (theme::textDim);
    g.drawText (juce::String (countGuides (row)), w - 34, 5, 24, 20, juce::Justification::centredRight);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText (summary, 30, 24, w - 40, 22, juce::Justification::topLeft, 2, 0.9f);
}

void TheoryGuidesView::selectedRowsChanged (int row)
{
    if (row < 0)
        return;

    shownRow = row;

    // Picking an area ends a search.
    if (searchBox.getText().isNotEmpty())
        searchBox.setText ({}, false);

    refresh();
}

void TheoryGuidesView::refresh()
{
    struct Shown
    {
        int entry, number;
        std::vector<int> topics;
    };

    std::vector<Shown> shown;
    const auto query = searchBox.getText().trim();
    const auto words = guides::getSearchWords (query);

    if (! words.isEmpty())
    {
        for (auto& m : guides::search (entries, query))
            shown.push_back ({ m.entry, 0, m.topics });

        const auto quoted = juce::String::fromUTF8 ("\xe2\x80\x9c") + query + juce::String::fromUTF8 ("\xe2\x80\x9d");
        heading = shown.empty() ? "Nothing found for " + quoted
                                : juce::String ((int) shown.size()) + (shown.size() == 1 ? " guide matches " : " guides match ") + quoted;
        subheading = shown.empty() ? "Try another word, or browse the areas on the left."
                                   : "Best first. The topics that match are outlined: click one to open the guide there.";
    }
    else if (shownRow == startRow)
    {
        int number = 0;

        for (auto g : guides::getStartPath())
            for (int i = 0; i < (int) entries.size(); ++i)
                if (entries[(size_t) i].guide == g)
                    shown.push_back ({ i, ++number, {} });

        heading = "Start here";
        subheading = "Seven guides in an order that builds up: what sound is, rhythm, chords, how songs are built, then levels, EQ and compression.";
    }
    else
    {
        const bool all = shownRow == getAllRow();
        const auto area = all ? guides::Area::sound : guides::allAreas[juce::jlimit (0, (int) std::size (guides::allAreas) - 1, shownRow - firstAreaRow)];

        for (int i = 0; i < (int) entries.size(); ++i)
            if (all || entries[(size_t) i].area == area)
                shown.push_back ({ i, 0, {} });

        heading = all ? juce::String ("All guides") : guides::getAreaName (area);
        subheading = all ? juce::String ((int) entries.size()) + " guides, area by area." : guides::getAreaSummary (area) + ".";
    }

    if (! words.isEmpty() && list.getSelectedRow() >= 0)
        list.deselectAllRows();

    shownGuides.clear();

    for (auto& s : shown)
        shownGuides.push_back (entries[(size_t) s.entry].guide);

    shownName = ! words.isEmpty() ? "Search for " + query : shownRow == startRow ? juce::String ("Start here")
                                                           : shownRow == getAllRow() ? juce::String ("All guides")
                                                                                     : guides::getAreaName (guides::allAreas[juce::jlimit (0, (int) std::size (guides::allAreas) - 1, shownRow - firstAreaRow)]);
    exportButton.setEnabled (! shownGuides.empty());

    cards.clear();

    for (auto& s : shown)
    {
        const auto& entry = entries[(size_t) s.entry];
        auto* card = cards.add (new Card (entry, s.number, s.topics, [this, entry]
        {
            return describeTarget != nullptr ? describeTarget (entry) : juce::String();
        }));

        card->onOpen = [this, g = entry.guide] (int topic)
        {
            if (onOpen != nullptr)
                onOpen (g, topic);
        };
        card->onExport = [g = entry.guide, name = entry.name] { guideprint::exportGuides ({ g }, name); };
        cardHolder.addAndMakeVisible (card);
    }

    layoutCards();
    viewport.setViewPosition (0, 0);
    repaint (headingArea);
}

void TheoryGuidesView::exportShown()
{
    guideprint::exportGuides (shownGuides, shownName);
}

bool TheoryGuidesView::keyPressed (const juce::KeyPress& key)
{
    if (! guideprint::isExportKey (key) || ! exportButton.isEnabled())
        return false;

    exportShown();
    return true;
}

void TheoryGuidesView::layoutCards()
{
    const int w = viewport.getMaximumVisibleWidth() - 4;
    int y = 0;

    for (auto* card : cards)
    {
        const int h = card->layout (w);
        card->setBounds (0, y, w, h);
        y += h + 10;
    }

    cardHolder.setSize (juce::jmax (1, w), juce::jmax (1, y));
}

void TheoryGuidesView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    auto r = headingArea;
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawFittedText (heading, r.removeFromTop (24), juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (subheading, r.withHeight (20), juce::Justification::centredLeft, 1, 0.8f);
}

void TheoryGuidesView::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto top = r.removeFromTop (64);
    searchBox.setBounds (top.removeFromRight (400).withSizeKeepingCentre (400, 32));
    top.removeFromRight (8);
    exportButton.setBounds (top.removeFromRight (120).withSizeKeepingCentre (120, 28));
    title.setBounds (top.removeFromTop (28));
    intro.setBounds (top);

    r.removeFromTop (8);
    list.setBounds (r.removeFromLeft (270));
    r.removeFromLeft (16);
    headingArea = r.removeFromTop (50);
    viewport.setBounds (r);
    layoutCards();
}
} // namespace daw
