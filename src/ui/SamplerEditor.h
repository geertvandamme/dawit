#pragma once

#include "EffectWindow.h"
#include "SamplerPictures.h"
#include "engine/BuiltInSampler.h"
#include "engine/Engine.h"

#include <map>

namespace daw
{
/**
    The window of DawIt Sampler: the SoundFont box over a list of General MIDI families and a list of its sounds (click one
    to choose and hear it; Up and Down step through them), the key-zone picture with a keyboard under it (hover for a
    zone, click to hear a key), the velocity layers of the last key and the last 4 seconds, the four knobs and the
    explainer. A banner covers the pictures while the SoundFont is missing, loading or unreadable, and says what plays
    instead.
*/
class BuiltInSamplerEditor final : public fxui::EffectEditor, private juce::ChangeListener
{
public:
    BuiltInSamplerEditor (BuiltInSampler&, Engine&);
    ~BuiltInSamplerEditor() override;

    bool keyPressed (const juce::KeyPress&) override;

    /** Chooses a SoundFont and one of its sounds on this sampler, asking to download MuseScore General first when
        that is what was chosen and it isn't here. Also used by the shot runner. */
    void choose (const soundfonts::Reference&, int bank, int program);

    /** Shows the lists on the family (a General MIDI family 0 to 15, or one of the other entries) that holds a sound. */
    void showFamilyOf (int bank, int program);

    /** A topic's number, for the shot runner. */
    enum Topic { zonesTopic = 50, layersTopic, presetListTopic, fontBoxTopic, bannerTopic };

private:
    class ZoneView;
    class LayersView;
    class FamilyModel;
    class PresetModel;
    class Banner;

    juce::AttributedString buildText (int topic) override;
    void layoutPictures (juce::Rectangle<int>) override;
    void applyPreset (int index) override;
    std::vector<fxui::Guide::Page> createGuidePages() override;
    void tick() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    struct Family
    {
        juce::String name;
        int key = 0;   // 0 to 15: a General MIDI family; otherwise one of the three below
    };

    static constexpr int allSounds = 100, moreBanks = 101, drumKits = 102;

    /** Which list entry a sound belongs under. */
    int getFamilyKey (int bank, int program) const;

    void refresh();
    void rebuildFamilies();
    void rebuildPresets();
    void showFamily (int row);
    void chooseRow (int row);
    void showFontMenu();
    void showAbout();
    void stepPreset (int step);

    BuiltInSampler& synth;
    Engine& engine;

    juce::TextButton fontButton;
    std::unique_ptr<FamilyModel> familyModel;
    std::unique_ptr<PresetModel> presetModel;
    juce::ListBox familyList, presetList;
    std::unique_ptr<ZoneView> zoneView;
    std::unique_ptr<LayersView> layersView;
    fxui::LevelHistoryView history;
    std::unique_ptr<Banner> banner;
    std::map<sampler::Knob, std::unique_ptr<fxui::EffectKnob>> knobs;

    std::vector<soundfonts::PresetInfo> allPresets, shownPresets;
    std::vector<Family> families;
    int shownFamilyKey = allSounds;
    bool refreshing = false;
    int shownStatus = -1;
    int presetIndex = -1;   // the starting point picked last
    juce::String shownFontKey;
    int shownBank = -1, shownProgram = -1;
};
} // namespace daw
