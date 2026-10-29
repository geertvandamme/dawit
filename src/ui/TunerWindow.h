#pragma once

#include "Selection.h"
#include "GuideButton.h"
#include "TunePictures.h"
#include "Theme.h"
#include "engine/Engine.h"
#include "engine/Tuner.h"
#include "generate/TuningGuide.h"

namespace daw
{
/** What the Tuner heard of each string of a tuning since DawIt started (the last reading's cents), for the guide's "Your tuning" topic. */
std::vector<gen::tuning::LastReading> getTunerLastReadings (gen::tuning::Tuning);

/**
    The Tuner window (View > Tuner..., Cmd+7): listens to an instrument on an audio input and shows the note, the
    needle (green within 3 cents, yellow within 10, red beyond), how many cents sharp or flat it is in words, which
    string it is, and whether the note is in the song's key. A tuning menu (guitar, bass, ukulele, chromatic), the
    input to listen to, the reference pitch (A4, 415 to 466 Hz) and a switch that silences monitored inputs while
    the window is open. One button per string locks the tuner to that string and plays its note for tuning by ear
    (keys 1 to 6 do the same). The tuning, input, reference and switch are remembered in the app settings.
    Opening the window starts listening (which opens the device's inputs); closing it stops.
*/
class TunerWindow final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer, private juce::ChangeListener
{
public:
    TunerWindow (Engine&, Selection&);
    ~TunerWindow() override;

    /** Called by the ? button and the last menu item: opens "How to tune a guitar". The ? button is always shown, since the window is laid out before this is set. */
    std::function<void()> onOpenGuide;

    /** The colour of a zone, as DawIt Tune's tuner and the string buttons use it. */
    static juce::Colour getZoneColour (gen::tuning::Zone);

    /** For the screenshot runner: shows a made-up reading of this pitch instead of listening, so a picture needs no microphone. */
    void showExample (double hz);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void parentHierarchyChanged() override;

private:
    class StringButton;

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void setTuning (gen::tuning::Tuning);
    void rebuildStringButtons();
    void rebuildInputMenu();
    void applyInput();
    void lockString (int index);
    void playTone (int stringIndex);
    void stopTone();
    void showMenu();
    juce::String getStatusText (const tuner::Reading&) const;
    void saveSetting (const juce::String& key, const juce::var&);

    Engine& engine;
    Selection& selection;

    gen::tuning::Tuning tuning = gen::tuning::Tuning::guitarStandard;
    double referenceHz = 440.0;
    int lockedString = -1;
    bool inputFollowsTrack = true;        // the saved choice; the track's input is used only while an audio track is selected
    int deviceChannel = 0;
    bool deviceStereo = false;
    juce::String lastTrackLabel;
    int lastNumInputs = -1;
    double exampleHz = 0.0;               // above 0: showing this instead of listening
    juce::uint32 toneEndsAt = 0;
    int lastAppliedChannel = -1;
    bool lastAppliedStereo = false;

    tuner::Listener listener;
    std::vector<gen::tuning::TuningString> strings;
    gen::tuning::Reading current;
    bool hasCurrent = false;

    juce::ComboBox tuningBox, inputBox;
    juce::Label referenceLabel { {}, "A4 =" };
    juce::Slider referenceSlider;
    juce::ToggleButton muteToggle { "Mute while tuning" };
    GuideButton helpButton { "?" };
    juce::OwnedArray<StringButton> stringButtons;

    juce::Rectangle<int> pictureArea, levelArea, statusArea;
};
} // namespace daw
