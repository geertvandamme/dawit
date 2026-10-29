#pragma once

#include "engine/Engine.h"
#include "generate/SingingGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    A guide to singing techniques for people who sing on their own songs: how the voice works, posture and
    breathing, warming up, voice types and range, registers, singing in tune, vowels, vibrato, loud singing,
    harmonies, looking after the voice and the microphone. Each topic has a picture in the app's visual language
    (keyboards, piano-roll notes, pitch curves, section colours). A voice row above the picture on Voice types and
    range and Your voice and your song sets the singer's range (a voice type, or a lowest and highest note found by
    ear on a keyboard that plays); Your voice and your song checks the song's melody against it and follows the
    song as it changes. The range is kept in the app settings (singing.lowest, singing.highest), the same for every
    song. The window never changes the song.
*/
class SingingExplainer final : public juce::Component,
                               public guideprint::Source,
                               private juce::ListBoxModel,
                               private juce::ChangeListener,
                               private juce::Timer
{
public:
    /** `getSelectedTrack` says which track is selected, so the melody starts on it. */
    SingingExplainer (Engine&, std::function<Track*()> getSelectedTrack);
    ~SingingExplainer() override;

    void showTopic (gen::singing::Topic);

    juce::String getPrintTitle() const override { return "Singing techniques"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    /** The singer's range from the app settings (not set when either is missing). */
    gen::singing::Range getRange() const;

    /** The app settings the range is kept in. */
    static constexpr const char* lowestKey = "singing.lowest";
    static constexpr const char* highestKey = "singing.highest";

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void show (int index);
    void setRange (gen::singing::Range);
    void setLowest (int note);
    void setHighest (int note);
    void updateVoiceRow();
    void updateMelodyChoices();
    void refreshAnalysis();
    bool isVoiceRowShown() const;
    int findLitNote() const;

    Engine& engine;
    std::function<Track*()> getSelectedTrack;
    int shownRow = 0;

    gen::singing::Analysis analysis;
    struct LitNote { int pitch = 0; double start = 0.0, end = 0.0; };
    std::vector<LitNote> melodyNotes;
    std::vector<const Track*> melodyTracks;
    const Track* melodyTrack = nullptr;
    bool melodyChosen = false;

    juce::Label title, voiceLabel { {}, "Your voice:" }, lowLabel { {}, "Lowest" }, highLabel { {}, "Highest" }, melodyLabel { {}, "Melody:" };
    juce::ComboBox typeBox, lowBox, highBox, melodyBox;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::singing };
    int rowBeforePrinting = -1;
    int printHeight = 0;

    static constexpr int width = 1000, height = 740;
};
} // namespace daw
