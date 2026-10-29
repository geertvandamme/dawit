#pragma once

#include "MainComponent.h"

namespace daw
{
/**
    The developer flag --screenshots <shots.json>: takes screenshots of the real app from a list, without
    changing any code. It sets up a song, then for each shot puts the app in the state the shot asks for
    (a track selected, a panel or a guide open, zoomed to some bars), waits for it to draw, saves a PNG and
    goes on to the next. Used to check a feature in the real app and for the website's screenshots.

    The list is JSON:

        {
          "song": "pop",                     the example pop song (default), "sends" (the pop song with Reverb and Delay
                                             returns and sends), "flow" (the sends song with EQ and Compressor on Lead
                                             vocal, EQ and a switched-off Saturation on Bass, and Delay then Compressor on
                                             Keys, for the Signal Flow window), "style:<id>", "demo", "empty" or a .dawit file
          "window": [1500, 940],             the main window's size (optional)
          "folder": "website/assets/screenshots",   where the PNGs go (default: the current folder)
          "scale": 2,                        pixels per point (default 2)
          "shots": [
            { "file": "arrangement.png", "select": "Kick", "beat": 32, "height": 520 },
            { "file": "mixer.png", "panel": "mixer", "capture": "bottomPanel" },
            { "file": "routing-guide.png", "guide": "routing", "topic": "Your song's routing" },
            { "file": "compressor.png", "plugin": "DawIt Compressor", "track": "Drums" }
          ]
        }

    A shot's settings, applied in this order (all optional except "file"); what a shot changes in the song or the
    views stays for the shots after it:
    - "settings": an object of app-setting keys and values (numbers, text, true or false; null removes one), written to
      the scratch settings first, such as { "singing.lowest": 45, "singing.highest": 69 } for the singing guide's range;
    - "fold" / "unfold": bus names to fold or open; "mute" / "unmute": track names to mute or unmute;
      "select": a track name, or a list of them (all selected, as with Cmd-click), or [] for nothing selected; "scaleFilter": a track name or a list of them, whose scale filter is switched on; "beat": the playhead;
      "play": true plays from there (audibly), otherwise the transport stops;
    - "sends": a track name: does what the Sends button on its header does (selects it, shows the mixer on its strip);
    - "sendsChoice": "Track:Return name" or "Track:new reverb|delay|empty": chooses that item in the Sends button's menu
      (the return is selected and the mixer opens on it);
    - "panel": "mixer", "clipEditor" or "closed"; "inspector": true or false shows or hides the inspector; "songRowsFolded": true or false;
      "chordsOpen", "lyricsOpen": open or close those rows; "chordDiagrams": "piano" or "guitar";
    - "clipEditorMode": "score" or "pianoRoll" switches the clip editor; "scoreClef": "auto", "treble", "bass", "grand", "guitar" or
      "bassGuitar" sets the selected MIDI track's score clef; "sheetMusic": "a4" or "letter" writes the selected MIDI track's sheet
      music PDF and <file stem>-page<n>.png next to the shot (give it "area": [0,0,10,10]);
    - "zoom": pixels per beat; "viewStart": the first beat in view;
    - "guide": a guide by its key ("lyrics", "routing", "reverb", ... as `getGuideKey` names them) or its
      window title, with "topic" as an index or a topic's name; "track" opens a built-in's guide on that track when present;
    - "addPlugins" with "track": adds the named built-in plugins to that track if missing;
    - "plugin" with "track": opens that plugin's window on that track; "preset" selects a DawIt Stereo or DawIt Tune starting point,
      and "clap": true plays its test sound;
    - "scene": one of `getSceneNames()`, for windows that open in other ways (the Theory Guides window,
      play along, sing along, the lyrics' text, the Sing window with "voices" and "voice" to choose the voices folder,
      "signalFlow": the Signal Flow window, "newFromStyle" (with an optional "style" id), "bounce" (the Bounce to WAV window with the "select" tracks selected and *Selected tracks* picked), "bounceInPlace" (bounces the shot's "track" in place and selects the new track), "soundClip" (adds an "Example clip" audio track with the sound guide's made-up example clip, open in the wave editor zoomed to its first two seconds so What is sound? looks at 1.0 s), "tune" (adds a "Sung take" track with DawIt Tune on it, played through once offline so its tuner has heard the song), fitted or with "flowZoom" as a percent such as 150 or "fit", scrolled to
      the selected track);
    - "wait": milliseconds before the picture (default 700).

    What's photographed ("capture"): "main", "arrangement", "deviceBar", "transport", "bottomPanel", "inspector", "window"
    (the window the shot opened; the default when it opened one, otherwise "main") or a window's title. "area"
    [x, y, w, h] or "height" crops it, in points. Windows a shot opened are closed before the next one,
    unless it says "keepWindows": true.

    Runs on its own settings (see Engine's `scratchSettings`), so the user's are never read or changed.
*/
class ScreenshotRunner final
{
public:
    /** Reads the list and starts taking the shots; `done` gets whether every one was taken. */
    static void start (MainComponent&, const juce::File& shotList, std::function<void (bool ok)> done);

    /** The key a shot names a guide by: "lyrics", "chordShapes", "gainStaging"... */
    static juce::String getGuideKey (gen::guides::Guide);

    /** The scenes a shot can ask for by name. */
    static juce::StringArray getSceneNames();

private:
    struct State;
    static void takeShot (std::shared_ptr<State>, int index);
    static bool apply (State&, const juce::var& shot);
    static juce::Component* findTarget (State&, const juce::var& shot);
};
} // namespace daw
