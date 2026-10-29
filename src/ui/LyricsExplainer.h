#pragma once

#include "engine/Engine.h"
#include "generate/LyricsGuide.h"
#include "GuidePrint.h"
#include "Theme.h"

namespace daw
{
/**
    A guide to writing lyrics for beginning songwriters, opened from the ? in the Lyrics row's header
    and the row's menu: what each section's words do, lines and phrases, rhyme, stress on the beat,
    putting words on a melody, prosody, the hook, writing tips and song forms. Each topic has a
    picture: lines with their rhymes coloured, syllables on a grid of beats with the strong beats
    shaded, or syllables under the notes of a melody. The last topic looks at the song's own lyrics
    and follows them as they change.
*/
class LyricsExplainer final : public juce::Component,
                              public guideprint::Source,
                              private juce::ListBoxModel,
                              private juce::ChangeListener
{
public:
    explicit LyricsExplainer (Engine&);
    ~LyricsExplainer() override;

    void showTopic (gen::lyricsguide::Topic);

    /** Set by the owner: the *Singing techniques...* button under the picture of Singing along calls it. */
    std::function<void()> onOpenSingingGuide;

    juce::String getPrintTitle() const override { return "How lyrics are written"; }
    juce::String getPrintSource() const override;
    int getNumPrintTopics() const override;
    booklet::Topic showPrintTopic (int index) override;
    juce::Image paintPrintPicture (float scale) override;
    void endPrinting() override;

    /** The colour of a rhyme (A, B, C...), as the pictures draw it. */
    static juce::Colour getRhymeColour (int rhyme);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Picture;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void show (int index);

    Engine& engine;
    int shownRow = 0;

    juce::Label title;
    juce::ListBox list;
    std::unique_ptr<Picture> picture;
    juce::TextEditor explanation;
    juce::TextButton singingButton { "Singing techniques..." };
    guideprint::ExportButton exportButton { *this, gen::guides::Guide::lyrics };
    int rowBeforePrinting = -1;
    int printHeight = 0;

    static constexpr int width = 1000, height = 740;
};
} // namespace daw
