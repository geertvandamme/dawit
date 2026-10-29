#pragma once

#include "engine/Engine.h"
#include "GuideButton.h"
#include "engine/Voice.h"
#include "engine/VoiceInstall.h"
#include "generate/LyricsGuide.h"
#include "generate/SingMatch.h"
#include "generate/SingPairing.h"
#include "generate/SingScore.h"
#include "Theme.h"

namespace daw
{
/** Everything the singing job needs, copied out of the window: it runs on another thread, so nothing in it points
    into the edit except the track the vocal is added after. */
struct SingRequest
{
    Track* track = nullptr;                         // the melody's track
    sing::Voice voice;
    gen::singthis::Melody melody;
    gen::singthis::Pairing pairing;
    gen::singthis::ScoreSettings settings;
    int steps = 20;                                 // Draft 10, Good 20, Best 50
    bool muteMelody = true;
};

/**
    The Sing window: a computer voice sings a MIDI melody and the words on the Lyrics row. It shows the voice to
    use (with the maker's credit, terms and a way to add or find one), the melody, how the words are put on the
    notes, the pitch, breaths and quality, a picture of the notes with their syllables and sounds, and advice
    about the words and notes. Without a voice it says how to get one, step by step. Sing hands a SingRequest to
    its owner.
*/
class SingWindow final : public juce::Component,
                         private juce::ChangeListener
{
public:
    /** `track` is the melody to sing, and `clip` one of its clips (null: all of the track). */
    SingWindow (Engine&, Track* track, MidiClip* clip);
    ~SingWindow() override;

    /** Called when the user presses Sing. */
    std::function<void (SingRequest)> onSing;

    /** Called to open the lyrics guide at a topic (the ? button, and the voice help). */
    std::function<void (gen::lyricsguide::Topic)> onOpenGuide;

    /** Picks another melody (a track and optionally one of its clips) for an open window. */
    void setSource (Track*, MidiClip*);

    /** The voices in the Voices folder, for the screenshots to pick one. */
    void selectVoice (const juce::String& folderName);

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int width = 960, height = 640;

private:
    friend class ScreenshotRunner;   // the --screenshots developer flag can press Sing

    class Picture;
    class AdviceView;
    class HelpView;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void refreshTracks();
    void refreshVoice();
    void refreshMelody (bool chooseModeAgain);
    void refreshAdvice();
    void showVoiceMenu();
    void showAddMenu();
    void showGetMenu();
    void addVoice (const juce::File&);
    void finishAddingVoice (const sing::InstallResult&, bool cancelled);
    void removeCurrentVoice();
    void openGuide (gen::lyricsguide::Topic);
    void updateEnabled();
    void makeMatchedTrack();
    bool isStillThere() const;
    juce::String getTitleText() const;

    std::vector<gen::singthis::SourceNote> collectNotes() const;

    Engine& engine;
    Track* track = nullptr;
    MidiClip* clip = nullptr;

    // The voices and the one chosen.
    std::vector<juce::File> voiceFolders;
    juce::String voiceName;                     // the folder's name, remembered in the settings
    std::optional<sing::Voice> voice;
    juce::String voiceError;

    // The melody and how the words go on it.
    gen::singthis::Melody melody;
    gen::singthis::Pairing pairing;
    std::vector<gen::singthis::Advice> advice;
    std::vector<Track*> melodyTracks;
    bool matched = false;                       // sing from the notes matched to the words (a copy; the clip is untouched)
    bool canMatch = false;                      // the note count differs from the syllable count
    gen::singthis::MatchResult matchResult;     // what matching does to the melody's own notes

    juce::Label title, voiceLabel, melodyLabel, wordsLabel, pitchLabel, qualityLabel, credit;
    juce::TextButton voiceButton, addButton { "Add a voice..." }, getButton { "Get a voice..." }, termsButton { "Terms..." };
    GuideButton helpButton { "?" };
    juce::ComboBox melodyBox, wordsBox, pitchBox, qualityBox, matchMethodBox;
    juce::ToggleButton thisClip { "this clip" }, wholeTrack { "the whole track" }, breaths { "Breaths" }, muteMelody { "Mute the melody track afterwards" };
    juce::TextButton matchButton { "Match the notes to the words" }, makeTrackButton { "Make a new track with them" }, useMineButton { "Use my notes" };
    juce::Label matchLabel;
    juce::TextButton singButton { "Sing" }, cancelButton { "Cancel" }, stepsButton { "Step by step..." };
    std::unique_ptr<Picture> picture;
    juce::Viewport pictureViewport, adviceViewport;
    std::unique_ptr<AdviceView> adviceView;
    std::unique_ptr<HelpView> helpView;
    juce::Label hint;
    std::unique_ptr<juce::ThreadWithProgressWindow> installJob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SingWindow)
};
} // namespace daw
