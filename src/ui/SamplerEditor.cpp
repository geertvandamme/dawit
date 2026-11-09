#include "SamplerEditor.h"
#include "GuideButton.h"
#include "SamplerActions.h"
#include "SamplerGuide.h"
#include "engine/BuiltInSynth.h"
#include "engine/SamplerGuide.h"

namespace daw
{
using namespace fxui;
using sampler::Knob;

namespace
{
    /** The sampler never changes the sound it is asked to play "alone", so its window has no such switch. */
    std::atomic<bool> noListen { false };

    void drawHeading (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText (text, r.removeFromTop (16.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    }

    constexpr const char* zoneTooltip = "Key zones of this preset: each colour is one recording and the dot its root note. Hover a key for its zone; click to hear it.";
    constexpr const char* missingTooltip = "This SoundFont isn't on this computer. The DawIt Synth plays instead; the project remembers the file and the preset, so the sound comes back once you add it.";

    constexpr const char* levelText = "How loud the sampler plays, from -24 to +6 dB. Presets differ in loudness: even them out here, then mix with the fader.";
    constexpr const char* transposeText = "Moves every note by -24 to +24 semitones before the zone is chosen, so other recordings play, not just other pitches.";
    constexpr const char* fineTuneText = "Detunes by -100 to +100 cents (100 cents is a semitone), to match a recording or instrument slightly off A = 440 Hz.";
    constexpr const char* bendText = "How far the pitch wheel bends, 1 to 24 semitones each way. General MIDI starts at 2.";
}

//==============================================================================
/** The key-zone picture with the keyboard under it. */
class BuiltInSamplerEditor::ZoneView final : public juce::Component, public juce::TooltipClient
{
public:
    explicit ZoneView (BuiltInSampler& s) : processor (s)
    {
        setTopic (*this, zonesTopic);
    }

    /** Opens the guide: the last item of the right-click menu. */
    std::function<void()> onOpenGuide;

    juce::String getTooltip() override { return hoverText.isNotEmpty() ? hoverText : juce::String (zoneTooltip); }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        auto heading = r.removeFromTop (16.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText ("KEY ZONES: ONE COLOUR PER RECORDING, A DOT ON ITS ROOT", heading.removeFromLeft (heading.getWidth() * 0.5f).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.8f);

        g.setColour (theme::text);
        g.drawFittedText (hoverText.isNotEmpty() ? hoverText : juce::String ("HOVER A KEY FOR ITS ZONE, CLICK TO HEAR IT"), heading.toNearestInt(),
                          juce::Justification::centredRight, 1, 0.8f);

        const auto distinct = samplerpics::getDistinctZones (processor.getZones());
        const int lit = processor.lastNote.load();
        r.removeFromTop (4.0f);
        auto keys = r.removeFromBottom (34.0f);
        r.removeFromBottom (4.0f);
        samplerpics::paintZoneMap (g, r, distinct, hoverKey, lit);
        samplerpics::paintKeyboard (g, keys, hoverKey, lit);

        // The velocity axis: soft at the bottom, hard at the top.
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (9.5f));
        g.drawText ("velocity 127", r.reduced (4.0f, 2.0f).removeFromTop (12.0f), juce::Justification::topRight, false);
        g.drawText ("velocity 1", r.reduced (4.0f, 2.0f).removeFromBottom (12.0f), juce::Justification::bottomRight, false);
    }

    void mouseMove (const juce::MouseEvent& e) override { updateHover (e); }
    void mouseExit (const juce::MouseEvent&) override
    {
        hoverKey = -1;
        hoverText = {};
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            showMenu();
            return;
        }

        updateHover (e);

        if (hoverKey >= 0)
        {
            lastPlayed = hoverKey;
            processor.playKey (hoverKey);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        updateHover (e);

        if (hoverKey >= 0 && hoverKey != lastPlayed && ! e.mods.isPopupMenu())
        {
            lastPlayed = hoverKey;
            processor.playKey (hoverKey);
        }
    }

private:
    juce::Rectangle<float> getMapArea() const
    {
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (20.0f);
        r.removeFromBottom (38.0f);
        return r;
    }

    juce::Rectangle<float> getKeyArea() const
    {
        return getLocalBounds().toFloat().reduced (10.0f, 8.0f).removeFromBottom (34.0f);
    }

    void updateHover (const juce::MouseEvent& e)
    {
        const auto map = getMapArea(), keys = getKeyArea();
        const auto p = e.position;
        int key = -1, velocity = -1;

        if (map.contains (p))
        {
            key = samplerpics::getKeyAt (map, p.x);
            velocity = juce::jlimit (1, 127, juce::roundToInt ((map.getBottom() - p.y) / map.getHeight() * 127.0f));
        }
        else if (keys.contains (p))
        {
            key = samplerpics::getKeyAt (keys, p.x);
        }

        juce::String text;

        if (key >= 0)
            text = samplerpics::describeKey (samplerpics::getDistinctZones (processor.getZones()), key, velocity);

        if (key != hoverKey || text != hoverText)
        {
            hoverKey = key;
            hoverText = text;
            repaint();
        }
    }

    void showMenu()
    {
        juce::PopupMenu menu;
        const auto name = processor.getPresetName();
        menu.addItem ("Copy preset name", name.isNotEmpty(), false, [name] { juce::SystemClipboard::copyTextToClipboard (name); });
        menu.addSeparator();
        guideicon::addItem (menu, "What is a sampler?", [this] { if (onOpenGuide) onOpenGuide(); });
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
    }

    BuiltInSampler& processor;
    int hoverKey = -1, lastPlayed = -1;
    juce::String hoverText;
};

//==============================================================================
/** The recordings that play the last key, side by side along the velocity axis. */
class BuiltInSamplerEditor::LayersView final : public juce::Component
{
public:
    explicit LayersView (BuiltInSampler& s) : processor (s)
    {
        setTopic (*this, layersTopic);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        const int last = processor.lastNote.load();
        const int key = last >= 0 ? last : (processor.getBank() == soundfonts::drumBank ? sampler::testDrumNote : sampler::testNote);
        const auto distinct = samplerpics::getDistinctZones (processor.getZones());
        const auto here = samplerpics::findZonesAt (distinct, key, -1);
        drawHeading (g, r, "VELOCITY LAYERS OF " + samplerguide::getNoteName (key).toUpperCase() + (last >= 0 ? " (THE LAST NOTE)" : " (PLAY A NOTE TO FOLLOW IT)"));

        auto axis = r.removeFromBottom (14.0f);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (9.5f));
        g.drawText ("soft", axis, juce::Justification::centredLeft, false);
        g.drawText ("velocity", axis, juce::Justification::centred, false);
        g.drawText ("hard", axis, juce::Justification::centredRight, false);

        if (here.empty())
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.0f));
            g.drawFittedText (distinct.empty() ? juce::String ("No zones to show until a SoundFont is playing.")
                                               : "No zone plays " + samplerguide::getNoteName (key) + ": that note is silent.",
                              r.toNearestInt(), juce::Justification::centred, 2);
            return;
        }

        const float rowHeight = juce::jlimit (8.0f, 22.0f, r.getHeight() / (float) here.size());
        auto rows = r.withHeight (rowHeight * (float) here.size());
        auto xFor = [&rows] (int velocity) { return rows.getX() + rows.getWidth() * (float) velocity / 128.0f; };

        for (size_t i = 0; i < here.size(); ++i)
        {
            auto& z = distinct[(size_t) here[i]];
            const auto bar = juce::Rectangle<float> (xFor (z.lowVelocity - 1), rows.getY() + rowHeight * (float) i, xFor (z.highVelocity) - xFor (z.lowVelocity - 1), rowHeight);
            g.setColour (samplerpics::getZoneColour (here[i]).withAlpha (0.7f));
            g.fillRect (bar.reduced (0.5f));
            g.setColour (samplerpics::getZoneColour (here[i]));
            g.drawRect (bar.reduced (0.5f), 1.0f);
        }

        if (last >= 0)
        {
            const float x = xFor (processor.lastVelocity.load());
            g.setColour (theme::text);
            g.fillRect (x - 1.0f, rows.getY(), 2.0f, rows.getHeight());
        }
    }

private:
    BuiltInSampler& processor;
};

//==============================================================================
class BuiltInSamplerEditor::FamilyModel final : public juce::ListBoxModel
{
public:
    explicit FamilyModel (BuiltInSamplerEditor& o) : owner (o) {}

    int getNumRows() override { return (int) owner.families.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (! juce::isPositiveAndBelow (row, (int) owner.families.size()))
            return;

        if (selected)
        {
            g.setColour (samplerpics::colour.withAlpha (0.3f));
            g.fillRect (0, 0, width, height);
        }

        g.setColour (selected ? theme::text : theme::text.withAlpha (0.85f));
        g.setFont (juce::FontOptions (12.5f, selected ? juce::Font::bold : juce::Font::plain));
        g.drawText (owner.families[(size_t) row].name, 8, 0, width - 12, height, juce::Justification::centredLeft, true);
    }

    void selectedRowsChanged (int row) override
    {
        if (! owner.refreshing && row >= 0)
            owner.showFamily (row);
    }

private:
    BuiltInSamplerEditor& owner;
};

class BuiltInSamplerEditor::PresetModel final : public juce::ListBoxModel
{
public:
    explicit PresetModel (BuiltInSamplerEditor& o) : owner (o) {}

    int getNumRows() override { return (int) owner.shownPresets.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (! juce::isPositiveAndBelow (row, (int) owner.shownPresets.size()))
            return;

        if (selected)
        {
            g.setColour (samplerpics::colour.withAlpha (0.3f));
            g.fillRect (0, 0, width, height);
        }

        auto& p = owner.shownPresets[(size_t) row];
        g.setColour (selected ? theme::text : theme::text.withAlpha (0.85f));
        g.setFont (juce::FontOptions (12.5f, selected ? juce::Font::bold : juce::Font::plain));
        g.drawText (p.getLabel(), 8, 0, width - 12, height, juce::Justification::centredLeft, true);
    }

    void selectedRowsChanged (int row) override
    {
        if (! owner.refreshing && row >= 0)
            owner.chooseRow (row);
    }

private:
    BuiltInSamplerEditor& owner;
};

//==============================================================================
/** Covers the pictures while the SoundFont isn't there, with what plays instead and what to do about it. */
class BuiltInSamplerEditor::Banner final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Banner (BuiltInSamplerEditor& o) : owner (o)
    {
        setTopic (*this, bannerTopic);
        setTooltip (missingTooltip);
        addButton.setTooltip ("Choose a .sf2 or .sf3 file from this computer.");
        downloadButton.setTooltip ("Download MuseScore General, DawIt Sampler's free General MIDI set (about 40 MB, MIT licence), once.");

        addButton.onClick = [this]
        {
            sampleractions::addSoundFont (owner.engine, [safe = juce::Component::SafePointer<BuiltInSamplerEditor> (&owner)]
                                          (const soundfonts::Reference& reference, int bank, int program)
                                          {
                                              if (safe != nullptr)
                                                  safe->choose (reference, bank, program);
                                          });
        };

        downloadButton.onClick = [this]
        {
            owner.choose (soundfonts::getMuseScoreGeneralReference(), 0, 0);
        };

        addAndMakeVisible (addButton);
        addAndMakeVisible (downloadButton);
    }

    /** Sets what it says and which buttons it offers. */
    void update (const juce::String& newText, bool offerDownload)
    {
        text = newText;
        downloadButton.setVisible (offerDownload);
        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panel.withAlpha (0.94f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (samplerpics::colour.withAlpha (0.7f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (14.0f));
        g.drawFittedText (text, getLocalBounds().reduced (24, 16).withTrimmedBottom (50), juce::Justification::centred, 4, 0.9f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (24, 14).removeFromBottom (30);
        const int width = downloadButton.isVisible() ? 190 + 10 + 150 : 150;
        r = r.withSizeKeepingCentre (width, 28);

        if (downloadButton.isVisible())
        {
            downloadButton.setBounds (r.removeFromRight (190));
            r.removeFromRight (10);
        }

        addButton.setBounds (r);
    }

private:
    BuiltInSamplerEditor& owner;
    juce::String text;
    juce::TextButton addButton { "Add SoundFont..." }, downloadButton { "Download MuseScore General" };
};

//==============================================================================
BuiltInSamplerEditor::BuiltInSamplerEditor (BuiltInSampler& p, Engine& e)
    : EffectEditor (p, "DawIt Sampler", "What is a sampler?", "", noListen, p.clap, samplerpics::colour, fx::Clap::Sound::note),
      synth (p), engine (e),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "the sound", samplerpics::colour, "the recording", {})
{
    listenButton.setVisible (false);   // it never changes the sound: nothing to hear alone

    familyModel = std::make_unique<FamilyModel> (*this);
    presetModel = std::make_unique<PresetModel> (*this);
    zoneView = std::make_unique<ZoneView> (synth);
    layersView = std::make_unique<LayersView> (synth);
    banner = std::make_unique<Banner> (*this);
    zoneView->onOpenGuide = [this] { guideButton.triggerClick(); };

    fontButton.setTooltip ("Which SoundFont this track plays. MuseScore General is DawIt's free General MIDI set; add your own .sf2 or .sf3 with Add SoundFont...");
    fontButton.setColour (juce::TextButton::buttonColourId, samplerpics::colour.withAlpha (0.25f));
    fontButton.onClick = [this] { showFontMenu(); };
    fontButton.setWantsKeyboardFocus (false);
    setTopic (fontButton, fontBoxTopic);

    familyList.setModel (familyModel.get());
    familyList.setRowHeight (24);
    familyList.setColour (juce::ListBox::backgroundColourId, theme::panel);
    familyList.setColour (juce::ListBox::outlineColourId, theme::gridBar);
    familyList.setOutlineThickness (1);
    familyList.setTooltip ("The families of General MIDI (Piano, Bass, Strings ...) with the sounds of the SoundFont in each. Click one to see its sounds.");
    setTopic (familyList, presetListTopic);

    presetList.setModel (presetModel.get());
    presetList.setRowHeight (24);
    presetList.setColour (juce::ListBox::backgroundColourId, theme::panel);
    presetList.setColour (juce::ListBox::outlineColourId, theme::gridBar);
    presetList.setOutlineThickness (1);
    presetList.setTooltip ("The sounds in this SoundFont, by bank and program (programs shown 1 to 128, as in General MIDI). Click to choose; Up and Down step through them.");
    setTopic (presetList, presetListTopic);

    addAndMakeVisible (fontButton);
    addAndMakeVisible (familyList);
    addAndMakeVisible (presetList);
    addAndMakeVisible (*zoneView);
    addAndMakeVisible (*layersView);
    addAndMakeVisible (history);
    addAndMakeVisible (*banner);

    for (int k = 0; k < sampler::numKnobs; ++k)
    {
        const auto knob = (Knob) k;
        knobs[knob] = std::make_unique<EffectKnob> (synth.getParameters(), BuiltInSampler::getParameterId (knob), sampler::getKnobName (knob), k, samplerpics::colour);
    }

    setGroups ({ { "PLAYING", { knobs.at (Knob::level).get(), knobs.at (Knob::transpose).get(), knobs.at (Knob::fineTune).get(), knobs.at (Knob::bendRange).get() }, 0 } });

    juce::StringArray names;

    for (auto& point : samplerguide::getStartingPoints())
        names.add (point.name);

    setPresetNames (names);
    setWantsKeyboardFocus (true);
    synth.addChangeListener (this);
    refresh();
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

BuiltInSamplerEditor::~BuiltInSamplerEditor()
{
    synth.removeChangeListener (this);
    familyList.setModel (nullptr);
    presetList.setModel (nullptr);
}

//==============================================================================
int BuiltInSamplerEditor::getFamilyKey (int bank, int program) const
{
    if (bank == soundfonts::drumBank)
        return drumKits;

    if (bank != 0)
        return moreBanks;

    const auto inBankZero = std::count_if (allPresets.begin(), allPresets.end(), [] (auto& p) { return p.bank == 0; });
    return inBankZero > 16 ? sampler::getFamily (program) : allSounds;
}

void BuiltInSamplerEditor::rebuildFamilies()
{
    families.clear();
    const auto inBankZero = std::count_if (allPresets.begin(), allPresets.end(), [] (auto& p) { return p.bank == 0; });

    if (inBankZero > 16)
    {
        for (int family = 0; family < sampler::numFamilies; ++family)
            if (std::any_of (allPresets.begin(), allPresets.end(), [family] (auto& p) { return p.bank == 0 && sampler::getFamily (p.program) == family; }))
                families.push_back ({ samplerguide::getFamilyName (family), family });
    }
    else if (inBankZero > 0)
    {
        families.push_back ({ "Sounds", allSounds });
    }

    if (std::any_of (allPresets.begin(), allPresets.end(), [] (auto& p) { return p.bank != 0 && p.bank != soundfonts::drumBank; }))
        families.push_back ({ "More banks", moreBanks });

    if (std::any_of (allPresets.begin(), allPresets.end(), [] (auto& p) { return p.bank == soundfonts::drumBank; }))
        families.push_back ({ "Drum kits", drumKits });
}

void BuiltInSamplerEditor::rebuildPresets()
{
    shownPresets.clear();

    for (auto& p : allPresets)
        if (getFamilyKey (p.bank, p.program) == shownFamilyKey)
            shownPresets.push_back (p);
}

void BuiltInSamplerEditor::refresh()
{
    const juce::ScopedValueSetter<bool> guard (refreshing, true);
    const auto font = synth.getFont();
    const auto reference = synth.getFontReference();
    const int status = (int) synth.getStatus();
    const auto fontKey = reference.fileName + "|" + juce::String (reference.size);
    const bool moved = fontKey != shownFontKey || status != shownStatus || synth.getBank() != shownBank || synth.getProgram() != shownProgram;

    allPresets = font != nullptr && font->getState() == soundfonts::Font::State::ready ? font->getPresets() : std::vector<soundfonts::PresetInfo>();
    rebuildFamilies();

    // The lists follow the sound unless you are only looking at another family.
    if (moved)
        shownFamilyKey = getFamilyKey (synth.getBank(), synth.getProgram());

    if (std::none_of (families.begin(), families.end(), [this] (auto& f) { return f.key == shownFamilyKey; }) && ! families.empty())
        shownFamilyKey = families.front().key;

    rebuildPresets();
    familyList.updateContent();
    presetList.updateContent();

    for (int row = 0; row < (int) families.size(); ++row)
        if (families[(size_t) row].key == shownFamilyKey)
            familyList.selectRow (row, true, true);

    int presetRow = -1;

    for (int row = 0; row < (int) shownPresets.size(); ++row)
        if (shownPresets[(size_t) row].bank == synth.getBank() && shownPresets[(size_t) row].program == synth.getProgram())
            presetRow = row;

    if (presetRow >= 0)
        presetList.selectRow (presetRow, false, true);
    else
        presetList.deselectAllRows();

    familyList.repaint();
    presetList.repaint();

    // The SoundFont box, and the banner over the pictures.
    juce::String boxText = reference.isEmpty() ? juce::String ("No SoundFont chosen") : reference.fileName;
    juce::String bannerText;
    bool offerDownload = false;
    const auto fallbackName = builtInSounds::getName (synth.getFallbackSound());
    const auto quoted = "\"" + reference.fileName + "\"";
    const bool museMissing = ! soundfonts::isMuseScoreGeneralInstalled();

    switch (synth.getStatus())
    {
        case BuiltInSampler::Status::noFont:
            bannerText = "No SoundFont is chosen yet: the DawIt Synth's " + fallbackName + " plays until you choose one. Click the SoundFont box, or add a file.";
            offerDownload = museMissing;
            break;

        case BuiltInSampler::Status::missing:
            boxText += "  (missing)";
            bannerText = quoted + " isn't on this computer: the DawIt Synth's " + fallbackName + " plays instead. The project remembers the file and the sound, "
                                  "so it comes back once you add the file.";
            offerDownload = reference.fileName == soundfonts::museScoreGeneralFileName && museMissing;
            break;

        case BuiltInSampler::Status::loading:
            boxText += "  (loading)";
            bannerText = "Loading " + quoted + "... the DawIt Synth's " + fallbackName + " plays until it's ready.";
            break;

        case BuiltInSampler::Status::failed:
            boxText += "  (unreadable)";
            bannerText = quoted + " couldn't be read: " + synth.getFailure() + " The DawIt Synth's " + fallbackName + " plays instead.";
            break;

        case BuiltInSampler::Status::noPreset:
            bannerText = "This SoundFont has no sound with this bank and program: the DawIt Synth's " + fallbackName + " plays instead. Pick another from the list.";
            break;

        case BuiltInSampler::Status::ready:
            break;
    }

    fontButton.setButtonText (boxText);
    banner->setVisible (bannerText.isNotEmpty());
    banner->update (bannerText, offerDownload);

    shownFontKey = fontKey;
    shownStatus = status;
    shownBank = synth.getBank();
    shownProgram = synth.getProgram();
}

void BuiltInSamplerEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
    showTopic (getTopic());
}

void BuiltInSamplerEditor::showFamilyOf (int bank, int program)
{
    shownFamilyKey = getFamilyKey (bank, program);
    refresh();
}

void BuiltInSamplerEditor::showFamily (int row)
{
    if (! juce::isPositiveAndBelow (row, (int) families.size()) || families[(size_t) row].key == shownFamilyKey)
        return;

    shownFamilyKey = families[(size_t) row].key;
    rebuildPresets();
    presetList.updateContent();
    presetList.deselectAllRows();

    for (int i = 0; i < (int) shownPresets.size(); ++i)
        if (shownPresets[(size_t) i].bank == synth.getBank() && shownPresets[(size_t) i].program == synth.getProgram())
            presetList.selectRow (i, false, true);

    presetList.repaint();
}

void BuiltInSamplerEditor::chooseRow (int row)
{
    if (! juce::isPositiveAndBelow (row, (int) shownPresets.size()))
        return;

    synth.setPreset (shownPresets[(size_t) row].bank, shownPresets[(size_t) row].program);
    synth.clap.request();   // the test note, so a pick is heard
}

void BuiltInSamplerEditor::stepPreset (int step)
{
    if (allPresets.empty())
        return;

    int at = 0;

    for (int i = 0; i < (int) allPresets.size(); ++i)
        if (allPresets[(size_t) i].bank == synth.getBank() && allPresets[(size_t) i].program == synth.getProgram())
            at = i;

    const int next = juce::jlimit (0, (int) allPresets.size() - 1, at + step);
    synth.setPreset (allPresets[(size_t) next].bank, allPresets[(size_t) next].program);
    synth.clap.request();
}

bool BuiltInSamplerEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey)
    {
        stepPreset (key == juce::KeyPress::upKey ? -1 : 1);
        return true;
    }

    return false;
}

void BuiltInSamplerEditor::choose (const soundfonts::Reference& reference, int bank, int program)
{
    auto apply = [safe = juce::Component::SafePointer<BuiltInSamplerEditor> (this), reference, bank, program]
    {
        if (safe == nullptr)
            return;

        auto& s = safe->synth;

        if (! (s.getFontReference() == reference) || s.getStatus() == BuiltInSampler::Status::missing)
            s.setFont (reference);

        s.setPreset (bank, program);
        s.clap.request();
        safe->showFamilyOf (bank, program);
    };

    if (sampleractions::needsDownload (reference))
        sampleractions::whenMuseScoreGeneralIsHere (engine, apply);
    else
        apply();
}

void BuiltInSamplerEditor::showFontMenu()
{
    juce::PopupMenu menu;
    const auto current = synth.getFontReference();
    const auto muse = soundfonts::getMuseScoreGeneralReference();
    const bool museHere = soundfonts::isMuseScoreGeneralInstalled();

    auto firstOf = [] (const soundfonts::Reference& ref)
    {
        int bank = 0, program = 0;
        sampleractions::getFirstPreset (ref, bank, program);
        return std::pair<int, int> (bank, program);
    };

    menu.addItem (museHere ? juce::String ("MuseScore General") : "MuseScore General (not downloaded)", true, current.fileName == muse.fileName,
                  [this, muse] { choose (muse, 0, 0); });

    for (auto& font : soundfonts::getAddedFonts (engine.properties.getUserSettings()))
        menu.addItem (font.fileName, true, current == font, [this, font, firstOf] { const auto [bank, program] = firstOf (font); choose (font, bank, program); });

    menu.addSeparator();
    menu.addItem ("Add SoundFont...", [this]
                  {
                      sampleractions::addSoundFont (engine, [safe = juce::Component::SafePointer<BuiltInSamplerEditor> (this)]
                                                    (const soundfonts::Reference& reference, int bank, int program)
                                                    {
                                                        if (safe != nullptr)
                                                            safe->choose (reference, bank, program);
                                                    });
                  });
    menu.addItem ("About this sound set...", ! current.isEmpty(), false, [this] { showAbout(); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&fontButton));
}

void BuiltInSamplerEditor::showAbout()
{
    const auto reference = synth.getFontReference();
    juce::String text;
    juce::String dialogTitle = "About " + reference.fileName;

    if (reference.fileName == soundfonts::museScoreGeneralFileName)
    {
        dialogTitle = "About MuseScore General";
        text = "MuseScore General 0.2.0, a free sound set (all 128 General MIDI sounds, drum kits and more) made for MuseScore, under the MIT licence. "
               "DawIt downloads it once to its SoundFonts folder, beside this licence file.\n\n";
        const auto licence = soundfonts::getMuseScoreGeneralLicenceFile();
        text += licence.existsAsFile() ? licence.loadFileAsString() : juce::String ("The licence file isn't here yet: it is saved beside the set when the set is downloaded.");
    }
    else
    {
        const auto file = reference.find();
        const auto font = synth.getFont();
        text = "A SoundFont you added.\n\nFile: " + (file == juce::File() ? reference.fileName + " (not found)" : file.getFullPathName())
               + "\nSize: " + models::formatSize (file == juce::File() ? reference.size : file.getSize())
               + (font != nullptr && font->getState() == soundfonts::Font::State::ready
                      ? "\nSounds: " + juce::String ((int) font->getPresets().size()) + "\nHeld in memory while it plays: about "
                            + models::formatSize (font->getMemoryBytes())
                      : juce::String())
               + "\n\nIts own licence is the one that came with the file.";
    }

    auto* box = new juce::TextEditor();
    box->setMultiLine (true, true);
    box->setReadOnly (true);
    box->setCaretVisible (false);
    box->setText (text, false);
    box->setSize (580, 420);

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = dialogTitle;
    options.content.setOwned (box);
    options.componentToCentreAround = this;
    options.dialogBackgroundColour = theme::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.launchAsync();
}

//==============================================================================
void BuiltInSamplerEditor::applyPreset (int index)
{
    const auto& points = samplerguide::getStartingPoints();

    if (! juce::isPositiveAndBelow (index, (int) points.size()))
        return;

    const auto& point = points[(size_t) index];
    presetIndex = index;
    showPresetName (point.name);
    choose (soundfonts::getMuseScoreGeneralReference(), point.bank, point.shownProgram - 1);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInSamplerEditor::createGuidePages()
{
    return samplertext::makeGuide (synth);
}

void BuiltInSamplerEditor::layoutPictures (juce::Rectangle<int> r)
{
    auto left = r.removeFromLeft (330);
    r.removeFromLeft (10);
    fontButton.setBounds (left.removeFromTop (30));
    left.removeFromTop (8);
    familyList.setBounds (left.removeFromLeft (128));
    left.removeFromLeft (8);
    presetList.setBounds (left);

    auto top = r.removeFromTop (r.getHeight() * 60 / 100);
    zoneView->setBounds (top);
    banner->setBounds (top.reduced (2));
    r.removeFromTop (10);
    layersView->setBounds (r.removeFromLeft ((r.getWidth() - 10) / 2));
    r.removeFromLeft (10);
    history.setBounds (r);
}

void BuiltInSamplerEditor::tick()
{
    if ((int) synth.getStatus() != shownStatus || synth.getBank() != shownBank || synth.getProgram() != shownProgram)
        refresh();

    history.update();
    zoneView->repaint();
    layersView->repaint();
}

juce::AttributedString BuiltInSamplerEditor::buildText (int topicIndex)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto& points = samplerguide::getStartingPoints();
    const auto s = synth.getSettings();
    const auto distinct = samplerpics::getDistinctZones (synth.getZones());

    if (topicIndex == CommonTopic::presets && juce::isPositiveAndBelow (presetIndex, (int) points.size()))
    {
        const auto& point = points[(size_t) presetIndex];
        addTitle (text, point.name, samplerpics::colour);
        addLead (text, point.why);
        addSection (text, {}, "Starting points pick a sound of MuseScore General (bank " + juce::String (point.bank) + ", program " + juce::String (point.shownProgram)
                                  + "). If it isn't downloaded yet, DawIt asks first and the track plays the DawIt Synth until it is. Your Level, Transpose, "
                                    "Fine tune and Bend range stay as they are.");
        return text;
    }

    if (topicIndex >= 0 && topicIndex < sampler::numKnobs)
    {
        const auto k = (Knob) topicIndex;
        addTitle (text, sampler::getKnobName (k), samplerpics::colour);
        const float value = k == Knob::level ? s.levelDb : k == Knob::transpose ? s.transpose : k == Knob::fineTune ? s.fineTuneCents : s.bendRange;
        addLead (text, sampler::getKnobName (k) + ": " + sampler::formatKnob (k, value));
        addSection (text, {}, k == Knob::level ? levelText : k == Knob::transpose ? transposeText : k == Knob::fineTune ? fineTuneText : bendText);
        return text;
    }

    addTitle (text, "DawIt Sampler", samplerpics::colour);

    switch (topicIndex)
    {
        case CommonTopic::overview:
            addLead (text, "A sampler plays recordings. Someone recorded a real piano note by note and at several strengths, and when you press middle C the "
                           "sampler plays the recording of middle C. That is why a sampled piano sounds like a piano: it is one, recorded.");
            addSection (text, "Where the sounds come from", "A SoundFont (.sf2 or .sf3) holds many sounds, each made of zones: a recording, the keys and velocities it plays "
                                                             "for, and the note it was recorded at. MuseScore General is a free set with all 128 General MIDI sounds, drum kits and more.");
            addSection (text, "Reading the window", "Left, the SoundFont box over the families and sounds: click a sound to choose and hear it, or step with Up and Down. "
                                                    "Middle, the key zones with a keyboard under them: hover for a zone, click to hear a key. Below, the recordings that play "
                                                    "the last note, and the last 4 seconds.");
            addSection (text, "Getting started", "Pick a starting point, or a sound from the list, then click Play a note or play the track's notes. While the SoundFont is "
                                                 "missing or loading, the DawIt Synth plays instead, so the track is never silent.");
            break;

        case CommonTopic::clap:
            addLead (text, "Plays a note for under a second on the chosen sound: middle C, or the kick for a drum kit.");
            break;

        case CommonTopic::history:
        {
            const auto [before, after] = history.getLoudest();
            addLead (text, after > -70.0f ? "The last 4 seconds: the recording up to " + juce::String (before, 1) + " dB, the sound after Level up to " + juce::String (after, 1) + " dB."
                                          : juce::String ("Play a note to see it here."));
            break;
        }

        case fontBoxTopic:
            addLead (text, synth.getFontReference().isEmpty() ? juce::String ("No SoundFont is chosen.") : synth.getFontReference().fileName);
            addSection (text, {}, "Which SoundFont this track plays. MuseScore General is DawIt's free General MIDI set; add your own .sf2 or .sf3 with Add SoundFont... "
                                  "Several tracks on one file share it, so they cost the memory of one.");
            break;

        case presetListTopic:
            addLead (text, juce::String ((int) allPresets.size()) + " sounds in this SoundFont.");
            addSection (text, {}, "The sounds in this SoundFont, by bank and program (programs shown 1 to 128, as in General MIDI). Click to choose; Up and Down step through them. "
                                  "Banks other than 0 sit under More banks, and bank 128 holds the drum kits.");
            break;

        case zonesTopic:
            addLead (text, distinct.empty() ? juce::String ("No zones to show until a SoundFont is playing.")
                                            : juce::String (distinct.size()) + " key and velocity ranges, each with its own recording.");
            addSection (text, {}, zoneTooltip);
            addSection (text, "Why zones", "A recording stretches only so far before it sounds thin or unnatural, so a sound is made of several, each covering a few keys: "
                                           "a sample often covers its root note and about five semitones either side. Velocity layers do the same for how hard a note is played.");
            break;

        case layersTopic:
            addLead (text, "The recordings that play the last note you played, side by side from soft to hard.");
            addSection (text, {}, "A sound with several layers changes its character with velocity, not only its loudness: a soft note uses a soft recording. "
                                  "A line marks the velocity of the last note.");
            break;

        case bannerTopic:
            addLead (text, "The SoundFont isn't playing yet.");
            addSection (text, {}, missingTooltip);
            break;

        default:
            break;
    }

    return text;
}
} // namespace daw
