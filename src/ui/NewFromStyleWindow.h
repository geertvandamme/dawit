#pragma once

#include "ProjectManager.h"
#include "GuideButton.h"
#include "StylePictures.h"
#include "generate/StylesGuide.h"

namespace daw
{
/** File > New Project from Style: chooses a style, key and tempo, then makes an ordinary untitled DawIt project. */
class NewFromStyleWindow final : public juce::Component, private juce::ListBoxModel
{
public:
    NewFromStyleWindow (Engine&, ProjectManager&);

    /** Selects a style by its stable id; an unknown id leaves the remembered style selected. */
    void showStyle (const juce::String& styleId);

    /** Opens the guide at the style's page or at Your song. */
    std::function<void (gen::stylesguide::Topic)> onShowGuide;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int getNumRows() override { return (int) gen::templates::getStyles().size(); }
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override;
    juce::String getTooltipForRow (int) override;
    void selectedRowsChanged (int) override;
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override;
    const gen::templates::Style* getStyle() const;
    void updateStyle();
    void create (bool shuffle);

    Engine& engine;
    ProjectManager& project;
    juce::Label title, summary, keyLabel, tempoLabel, status;
    juce::ListBox list;
    StylePicture picture;
    juce::ComboBox keyBox;
    juce::Slider tempo;
    juce::ToggleButton writeNotes { "Write the notes" };
    juce::TextButton explainButton { "Explain this song" }, shuffleButton { "Shuffle" }, createButton { "Create" };
    GuideButton whatButton;
    uint32_t seed = 0;
    juce::String lastMadeText;

    static constexpr int width = 860, height = 560;
};
} // namespace daw
