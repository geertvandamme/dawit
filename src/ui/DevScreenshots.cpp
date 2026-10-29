#include "DevScreenshots.h"
#include "SheetMusicExport.h"
#include "generate/ChordSheet.h"

#include "engine/BuiltInCompressor.h"
#include "engine/BuiltInDelay.h"
#include "engine/BuiltInEq.h"
#include "engine/BuiltInFormat.h"
#include "engine/BuiltInReverb.h"
#include "engine/BuiltInSaturation.h"
#include "engine/BuiltInStereo.h"
#include "engine/BuiltInTune.h"
#include "BounceDialog.h"
#include "engine/Bounce.h"
#include "engine/ExampleSong.h"
#include "engine/HitsToMidi.h"
#include "HitsToMidiWindow.h"
#include "engine/Routing.h"
#include "generate/SongTemplates.h"
#include "PluginActions.h"
#include "EffectWindow.h"
#include "EqEditor.h"
#include "CompressorEditor.h"

#include <iostream>

namespace daw
{
struct ScreenshotRunner::State
{
    MainComponent& main;
    std::function<void (bool)> done;
    juce::Array<juce::var> shots;
    juce::File folder;
    float scale = 2.0f;
    bool ok = true;
    juce::Array<juce::Component::SafePointer<juce::TopLevelWindow>> before, opened;
    juce::OwnedArray<juce::Component> pluginEditorWindows;   // windows of the "pluginEditor" scene, kept until the run ends

    State (MainComponent& m, std::function<void (bool)> d) : main (m), done (std::move (d)) {}

    void fail (const juce::String& file, const juce::String& message)
    {
        std::cout << "screenshots: " << file << ": " << message << std::endl;
        ok = false;
    }
};

namespace
{
    /** A plain window holding a built-in's editor as a plugin host would, for the "pluginEditor" scene. It owns the
        processor, which it was created as with a plugin wrapper type. */
    class PluginEditorWindow final : public juce::DocumentWindow
    {
    public:
        PluginEditorWindow (std::unique_ptr<juce::AudioProcessor> p)
            : juce::DocumentWindow (p->getName(), theme::panel, juce::DocumentWindow::closeButton), processor (std::move (p))
        {
            setUsingNativeTitleBar (true);
            setContentOwned (processor->createEditor(), true);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        ~PluginEditorWindow() override { clearContentComponent(); }

        void closeButtonPressed() override { setVisible (false); }

        /** Opens the editor's guide overlay, at a page. */
        bool openGuide (int page)
        {
            auto* content = getContentComponent();

            if (auto* e = dynamic_cast<fxui::EffectEditor*> (content))      { e->openGuide (page); return true; }
            if (auto* q = dynamic_cast<BuiltInEqEditor*> (content))         { q->openGuide (page); return true; }
            if (auto* k = dynamic_cast<BuiltInCompressorEditor*> (content)) { k->openGuide (page); return true; }

            return false;
        }

    private:
        std::unique_ptr<juce::AudioProcessor> processor;
    };

    juce::Array<juce::TopLevelWindow*> getVisibleWindows()
    {
        juce::Array<juce::TopLevelWindow*> windows;

        for (int i = 0; i < juce::TopLevelWindow::getNumTopLevelWindows(); ++i)
            if (auto* w = juce::TopLevelWindow::getTopLevelWindow (i); w != nullptr && w->isVisible())
                windows.add (w);

        return windows;
    }

    /** What to photograph of a window: its content, without the title bar. */
    juce::Component* getContent (juce::TopLevelWindow& window)
    {
        if (auto* document = dynamic_cast<juce::ResizableWindow*> (&window))
            if (auto* content = document->getContentComponent())
                return content;

        return &window;
    }

    Track* findTrack (Engine& engine, const juce::String& name)
    {
        for (auto& track : engine.getTracks())
            if (track->name.equalsIgnoreCase (name))
                return track.get();

        return nullptr;
    }

    /** The "tune" scene: a sung take of the lead vocal's melody on a track of its own, with DawIt Tune (Natural fix) on it, played through
        once offline so what the tuner heard is there to show. The verse is 20 cents flat, three notes of the first chorus are F sharps
        (outside A minor) and the last note of each line sinks. Returns false with a reason when it can't be set up. */
    bool setUpTuneScene (Engine& engine, juce::String& problem)
    {
        auto* lead = findTrack (engine, "Lead vocal");

        if (lead == nullptr)
        {
            problem = "the tune scene needs a track called Lead vocal (the pop song has one)";
            return false;
        }

        if (findTrack (engine, "Sung take") != nullptr)
            return true;

        struct Melody { double startBeat, endBeat; int midi; };
        std::vector<Melody> melody;

        for (auto& clip : lead->clips)
            if (clip->kind == Clip::Kind::midi)
                for (auto& note : static_cast<MidiClip&> (*clip).notes)
                    melody.push_back ({ clip->startBeat + note.start, clip->startBeat + note.start + note.length, note.pitch });

        std::sort (melody.begin(), melody.end(), [] (const Melody& a, const Melody& b) { return a.startBeat < b.startBeat; });
        const auto& tempo = engine.getTempoMap();
        const auto& sections = engine.getSections();
        std::vector<tune::TakeNote> notes;
        int chorusNotes = 0, wrong = 0;

        for (size_t i = 0; i < melody.size(); ++i)
        {
            const auto& m = melody[i];
            const int bar = (int) std::floor (m.startBeat / 4.0);
            SectionType type = SectionType::intro;

            for (auto& s : sections)
                if (bar >= s.startBar && bar < s.getEndBar())
                    type = s.type;

            tune::TakeNote n;
            n.midi = m.midi;
            n.start = tempo.beatToSeconds (m.startBeat);
            n.end = juce::jmax (n.start + 0.12, tempo.beatToSeconds (m.endBeat) - 0.02);
            n.level = 1.4;
            const bool endOfLine = i + 1 >= melody.size() || melody[i + 1].startBeat - m.endBeat > 1.5;
            double offset = type == SectionType::verse ? -20.0 : ((int) i % 3 - 1) * 5.0;

            if (type == SectionType::chorus && ++chorusNotes % 5 == 3 && wrong < 3)
            {
                n.midi = 66;   // an F sharp in A minor
                offset = 4.0;
                ++wrong;
            }

            if (endOfLine)
                offset -= 26.0;

            const bool vibrato = n.end - n.start > 0.45;
            n.cents = [offset, vibrato] (double t)
            {
                return offset + (vibrato && t > 0.25 ? 14.0 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * (t - 0.25)) : 0.0);
            };
            notes.push_back (n);
        }

        const double seconds = notes.empty() ? 1.0 : notes.back().end + 1.0;
        auto* track = engine.addTrack (Track::Type::audio);
        track->name = "Sung take";
        engine.addAudioClip (*track, tune::makeTake (notes, 48000.0, seconds), 48000.0, 0.0, "Sung take");
        juce::String why;
        auto* slot = engine.addPlugin (*track, BuiltInTune::getDescription(), why);
        auto* tuner = slot != nullptr ? dynamic_cast<BuiltInTune*> (slot->instance.get()) : nullptr;

        if (tuner == nullptr)
        {
            problem = "could not add DawIt Tune: " + why;
            return false;
        }

        tuner->applyPreset (tune::getPresets()[1]);

        // One pass through the song, as a bounce, so the take log fills; the tuner's frames are read as they come.
        bounce::Settings settings;
        settings.startBeat = 0.0;
        settings.endBeat = juce::jmax (4.0, engine.getSongEndBar() * 4.0);
        const auto file = juce::File::createTempFile ("tune-scene.wav");

        {
            bounce::Renderer renderer (engine, settings);
            renderer.render (file, [tuner] (double) { tuner->drainFrames(); return true; });
        }

        file.deleteFile();
        tuner->drainFrames();
        tuner->clearRecentFrames();
        return true;
    }

    /** The "recording" scene: a "Voice" audio track armed on Input 1 with a stack of three takes of the same four seconds (a sung-like
        tone, a bit different each time), the newest playing and the older ones muted. No input device is opened. */
    bool setUpRecordingScene (Engine& engine, juce::String& problem)
    {
        if (findTrack (engine, "Voice") != nullptr)
            return true;

        auto* track = engine.addTrack (Track::Type::audio);

        if (track == nullptr)
        {
            problem = "could not add the Voice track";
            return false;
        }

        track->name = "Voice";
        const double rate = 48000.0;
        const int length = (int) (4.0 * rate);
        std::vector<AudioClip*> takes;

        for (int take = 0; take < 3; ++take)
        {
            juce::AudioBuffer<float> buffer (1, length);
            const double base = 220.0 * std::pow (2.0, take * 2.0 / 12.0);

            for (int i = 0; i < length; ++i)
            {
                const double t = i / rate;
                const double envelope = std::sin (juce::MathConstants<double>::pi * t / 4.0);
                const double wobble = 1.0 + 0.01 * std::sin (juce::MathConstants<double>::twoPi * 5.5 * t);
                buffer.setSample (0, i, (float) (0.25 * (0.4 + 0.3 * take) * envelope * std::sin (juce::MathConstants<double>::twoPi * base * wobble * t)));
            }

            takes.push_back (engine.addAudioClip (*track, std::move (buffer), rate, 8.0 * 4.0, "Voice take " + juce::String (take + 1)));
        }

        for (size_t i = 0; i < takes.size(); ++i)
            if (takes[i] != nullptr && i + 1 < takes.size())
                engine.setClipMuted (*takes[i], true);

        engine.setArmed (*track, true);
        return true;
    }

    /** The "soundClip" scene's track: "Example clip" with the sound guide's made-up example clip; returns its audio clip. */
    AudioClip* addSoundClipTrack (Engine& engine, juce::String& problem)
    {
        auto* track = findTrack (engine, "Example clip");

        if (track == nullptr)
        {
            track = engine.addTrack (Track::Type::audio);

            if (track == nullptr)
            {
                problem = "could not add the Example clip track";
                return nullptr;
            }

            track->name = "Example clip";
            engine.addAudioClip (*track, soundguide::makeExampleClip (44100.0), 44100.0, 0.0, "Example clip");
        }

        for (auto& c : track->clips)
            if (c->kind == Clip::Kind::audio)
                return static_cast<AudioClip*> (c.get());

        problem = "the Example clip track has no audio clip";
        return nullptr;
    }

    juce::StringArray toStrings (const juce::var& value)
    {
        juce::StringArray strings;

        if (auto* list = value.getArray())
            for (auto& item : *list)
                strings.add (item.toString());
        else if (! value.isVoid())
            strings.add (value.toString());

        return strings;
    }
} // namespace

juce::String ScreenshotRunner::getGuideKey (gen::guides::Guide guide)
{
    using G = gen::guides::Guide;

    switch (guide)
    {
        case G::readMusic:    return "readMusic";
        case G::sound:        return "sound";
        case G::midi:         return "midi";
        case G::rhythm:       return "rhythm";
        case G::chords:       return "chords";
        case G::chordShapes:  return "chordShapes";
        case G::structure:    return "structure";
        case G::styles:       return "styles";
        case G::lyrics:       return "lyrics";
        case G::singing:      return "singing";
        case G::tune:         return "tune";
        case G::gainStaging:  return "gainStaging";
        case G::eq:           return "eq";
        case G::compressor:   return "compressor";
        case G::gate:         return "gate";
        case G::stereo:       return "stereo";
        case G::routing:      return "routing";
        case G::reverb:       return "reverb";
        case G::delay:        return "delay";
        case G::saturation:   return "saturation";
        case G::distortion:   return "distortion";
        case G::subtractive:  return "subtractive";
        case G::additive:     return "additive";
        case G::fm:           return "fm";
        case G::drums:        return "drums";
        case G::physical:     return "physical";
        case G::recording:    return "recording";
        case G::tuning:       return "tuning";
        case G::triggers:     return "triggers";
    }

    return {};
}

juce::StringArray ScreenshotRunner::getSceneNames()
{
    return { "bounce", "bounceInPlace", "theoryGuides", "playAlong", "singAlong", "lyricsText", "lyricsEditor", "sing", "signalFlow", "newFromStyle", "tune", "tuner", "recording", "drumLoop", "audioSettings", "soundClip", "pluginEditor" };
}

void ScreenshotRunner::start (MainComponent& main, const juce::File& shotList, std::function<void (bool ok)> done)
{
    auto state = std::make_shared<State> (main, std::move (done));
    const auto list = juce::JSON::parse (shotList.loadFileAsString());

    if (! list.isObject() || list["shots"].getArray() == nullptr)
    {
        state->fail (shotList.getFullPathName(), "not a shot list: it needs a JSON object with a \"shots\" array");
        state->done (false);
        return;
    }

    state->shots = *list["shots"].getArray();
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    state->folder = list.hasProperty ("folder") ? cwd.getChildFile (list["folder"].toString()) : cwd;
    state->folder.createDirectory();
    state->scale = list.hasProperty ("scale") ? (float) (double) list["scale"] : 2.0f;

    if (auto* size = list["window"].getArray(); size != nullptr && size->size() == 2)
        if (auto* window = main.findParentComponentOfClass<juce::ResizableWindow>())
            window->setContentComponentSize ((int) (*size)[0], (int) (*size)[1]);

    const auto song = list.hasProperty ("song") ? list["song"].toString() : juce::String ("pop");

    if (song == "pop")
    {
        if (auto* lead = examples::buildPopSong (main.engine))
            main.selection.select (lead);
    }
    else if (song == "sends" || song == "flow")
    {
        // The pop song with a Reverb and a Delay return: Lead vocal, Keys and Snare (pre-fader) send to the reverb, Hats and a little of Lead vocal to the
        // delay, and BV 1 and BV 2 each have their own DawIt Reverb at 20% (which the guide offers to share).
        auto* lead = examples::buildPopSong (main.engine);
        auto& engine = main.engine;
        auto& actions = main.arrangement.getTrackActions();
        auto* reverb = actions.addReturn (TrackActions::ReturnKind::reverb);
        auto* delay = actions.addReturn (TrackActions::ReturnKind::delay);

        if (auto* t = findTrack (engine, "Lead vocal"))
        {
            engine.setSend (*t, *reverb, -12.4f);
            engine.setSend (*t, *delay, -16.0f);
        }

        if (auto* t = findTrack (engine, "Keys"))
            engine.setSend (*t, *reverb, -8.9f);

        if (auto* t = findTrack (engine, "Snare"))
        {
            engine.setSend (*t, *reverb, -10.0f);

            if (auto* send = routing::findSend (*t, *reverb))
                send->preFader = true;
        }

        if (auto* t = findTrack (engine, "Hats"))
            engine.setSend (*t, *delay, -14.0f);

        for (auto* name : { "BV 1", "BV 2" })
        {
            juce::String error;

            if (auto* t = findTrack (engine, name))
                if (auto* slot = engine.addPlugin (*t, BuiltInReverb::getDescription(), error))
                    if (auto* plugin = dynamic_cast<BuiltInReverb*> (slot->instance.get()))
                    {
                        auto settings = plugin->getSettings();
                        settings.mixPercent = 20.0f;
                        plugin->setSettings (settings);
                    }
        }

        // The "flow" song also has effects in different orders, for the Signal Flow window: DawIt EQ then DawIt Compressor on Lead vocal,
        // DawIt EQ and a switched-off DawIt Saturation on Bass, and DawIt Delay (20%) then DawIt Compressor on Keys (which the guide says to reorder).
        if (song == "flow")
        {
            juce::String error;
            auto add = [&] (const char* trackName, const juce::PluginDescription& description) -> PluginSlot*
            {
                if (auto* t = findTrack (engine, trackName))
                    return engine.addPlugin (*t, description, error);

                return nullptr;
            };

            add ("Lead vocal", BuiltInEq::getDescription());
            add ("Lead vocal", BuiltInCompressor::getDescription());
            add ("Bass", BuiltInEq::getDescription());

            if (auto* saturation = add ("Bass", BuiltInSaturation::getDescription()))
                saturation->bypassed = true;

            if (auto* slot = add ("Keys", BuiltInDelay::getDescription()))
                if (auto* plugin = dynamic_cast<BuiltInDelay*> (slot->instance.get()))
                {
                    auto settings = plugin->getSettings();
                    settings.mixPercent = 20.0f;
                    plugin->setSettings (settings);
                }

            add ("Keys", BuiltInCompressor::getDescription());
        }

        if (lead != nullptr)
            main.selection.select (lead);
    }
    else if (song.startsWith ("style:"))
    {
        const auto id = song.fromFirstOccurrenceOf (":", false, false);
        if (const auto* style = gen::templates::findStyle (id))
        {
            juce::String error;
            if (! gen::templates::build (main.engine, gen::templates::makePlan (*style, std::nullopt, style->defaultBpm, 0, true), error))
                state->fail (song, error);
        }
        else
        {
            state->fail (song, "no such style");
        }
    }
    else if (song == "demo")
    {
        main.createDemoContent();
    }
    else if (song != "empty")
    {
        auto file = cwd.getChildFile (song);

        if (! file.existsAsFile())
            file = shotList.getParentDirectory().getChildFile (song);

        if (file.existsAsFile())
            main.openProject (file);
        else
            state->fail (song, "no such song or project file");
    }

    juce::Timer::callAfterDelay (1000, [state] { takeShot (state, 0); });
}

void ScreenshotRunner::takeShot (std::shared_ptr<State> state, int index)
{
    // Close what the previous shot opened, newest first. Windows close asynchronously, so the next shot waits a
    // moment: otherwise it would find a closing window still there and bring it to the front instead of a new one.
    bool closed = false;

    for (int i = state->opened.size(); --i >= 0;)
    {
        if (auto* document = dynamic_cast<juce::DocumentWindow*> (state->opened[i].getComponent()))
        {
            document->closeButtonPressed();
            closed = true;
        }
    }

    state->opened.clear();

    if (closed)
    {
        juce::Timer::callAfterDelay (300, [state, index] { takeShot (state, index); });
        return;
    }

    if (index >= state->shots.size())
    {
        state->main.engine.stop();
        state->done (state->ok);
        return;
    }

    const auto shot = state->shots[index];
    const auto fileName = shot["file"].toString();

    if (fileName.isEmpty())
    {
        state->fail ("shot " + juce::String (index + 1), "it has no \"file\"");
        takeShot (state, index + 1);
        return;
    }

    state->before.clear();

    for (auto* w : getVisibleWindows())
        state->before.add (w);

    const bool applied = apply (*state, shot);
    const int wait = shot.hasProperty ("wait") ? (int) shot["wait"] : 700;

    juce::Timer::callAfterDelay (wait, [state, index, shot, fileName, applied]
    {
        for (auto* w : getVisibleWindows())
            if (! state->before.contains (w))
                state->opened.add (w);

        if (shot["keepWindows"])
            state->opened.clear();   // left open for the next shots; otherwise closed when the next one starts

        if (applied)
        {
            if (auto* target = findTarget (*state, shot))
            {
                auto area = target->getLocalBounds();

                if (auto* crop = shot["area"].getArray(); crop != nullptr && crop->size() == 4)
                    area = area.getIntersection ({ (int) (*crop)[0], (int) (*crop)[1], (int) (*crop)[2], (int) (*crop)[3] });
                else if (shot.hasProperty ("height"))
                    area = area.withHeight (juce::jmin (area.getHeight(), (int) shot["height"]));

                const float scale = shot.hasProperty ("scale") ? (float) (double) shot["scale"] : state->scale;
                const auto image = target->createComponentSnapshot (area, true, scale);
                const auto file = state->folder.getChildFile (fileName);
                file.deleteFile();

                if (auto stream = file.createOutputStream(); stream != nullptr && juce::PNGImageFormat().writeImageToStream (image, *stream))
                    std::cout << "screenshots: " << file.getFullPathName() << " (" << image.getWidth() << " x " << image.getHeight() << ")" << std::endl;
                else
                    state->fail (fileName, "couldn't write the file");
            }
        }

        takeShot (state, index + 1);
    });
}

bool ScreenshotRunner::apply (State& state, const juce::var& shot)
{
    auto& main = state.main;
    auto& engine = main.engine;
    auto& arrangement = main.arrangement;
    const auto file = shot["file"].toString();
    bool ok = true;

    const auto error = [&] (const juce::String& message)
    {
        state.fail (file, message);
        ok = false;
    };

    // App settings, written before anything else so a window that opens in this shot reads them (the scratch settings, never the user's).
    if (auto* object = shot["settings"].getDynamicObject())
    {
        auto* settings = engine.properties.getUserSettings();

        for (auto& property : object->getProperties())
        {
            const auto key = property.name.toString();

            if (property.value.isVoid())
                settings->removeValue (key);
            else if (property.value.isBool())
                settings->setValue (key, (bool) property.value);
            else if (property.value.isInt() || property.value.isInt64())
                settings->setValue (key, (int) property.value);
            else if (property.value.isDouble())
                settings->setValue (key, (double) property.value);
            else
                settings->setValue (key, property.value.toString());
        }
    }

    if (auto* object = shot["settings"].getDynamicObject(); object != nullptr && object->hasProperty ("typing.on"))
        main.typing.reloadSettings();   // the computer keyboard reads its settings at start-up

    for (auto [key, folded] : { std::pair { "fold", true }, std::pair { "unfold", false } })
    {
        for (auto& name : toStrings (shot[key]))
        {
            if (auto* bus = findTrack (engine, name); bus != nullptr && bus->isBus())
                engine.setFolded (*bus, folded);
            else
                error ("no bus called \"" + name + "\"");
        }
    }

    if (shot.hasProperty ("select"))
    {
        // One name, or a list: the first is selected, the others added (as Cmd-click does).
        bool first = true;
        const auto names = toStrings (shot["select"]);

        if (names.isEmpty())
            main.selection.select (nullptr);   // an empty list: nothing selected (the inspector then shows the master)

        for (auto& name : names)
        {
            if (auto* track = findTrack (engine, name))
            {
                if (first)
                    main.selection.select (track);
                else
                    main.selection.toggle (track);

                first = false;
            }
            else
                error ("no track called \"" + name + "\"");
        }
    }

    // "scaleFilter": a track name or a list, whose scale filter is switched on (the computer keyboard strip greys the keys it silences).
    for (auto& name : toStrings (shot["scaleFilter"]))
    {
        if (auto* track = findTrack (engine, name))
            track->scaleFilter = true;
        else
            error ("no track called \"" + name + "\" for the scale filter");
    }

    if (shot.hasProperty ("addPlugins"))
    {
        auto* track = findTrack (engine, shot["track"].toString());
        if (track == nullptr)
            error ("no track called \"" + shot["track"].toString() + "\"");
        else
        {
            auto descriptions = BuiltInFormat::getEffects();
            descriptions.addArray (BuiltInFormat::getInstruments());
            for (const auto& name : toStrings (shot["addPlugins"]))
            {
                bool exists = false, known = false;
                for (const auto& slot : track->plugins)
                    if (slot->instance != nullptr && slot->instance->getName().equalsIgnoreCase (name))
                        exists = true;
                for (const auto& description : descriptions)
                    if (description.name.equalsIgnoreCase (name))
                    {
                        known = true;
                        if (! exists)
                        {
                            juce::String why;
                            if (engine.addPlugin (*track, description, why) == nullptr)
                                error ("could not add " + name + ": " + why);
                        }
                        break;
                    }
                if (! known)
                    error ("no built-in plugin called \"" + name + "\"");
            }
        }
    }

    for (auto [key, muted] : { std::pair { "mute", true }, std::pair { "unmute", false } })
    {
        for (auto& name : toStrings (shot[key]))
        {
            if (auto* track = findTrack (engine, name))
                track->muted = muted;
            else
                error ("no track called \"" + name + "\"");
        }
    }

    if (shot.hasProperty ("sends"))
    {
        // As clicking the Sends button on that track's header.
        if (auto* track = findTrack (engine, shot["sends"].toString()); track != nullptr && arrangement.getTrackActions().onShowSends != nullptr)
            arrangement.getTrackActions().onShowSends (*track);
        else
            error ("no track called \"" + shot["sends"].toString() + "\"");
    }

    if (shot.hasProperty ("sendsChoice"))
    {
        // "Track:Return name" or "Track:new reverb|delay|empty": as choosing that item in the Sends button's menu.
        const auto text = shot["sendsChoice"].toString();
        const auto trackName = text.upToFirstOccurrenceOf (":", false, false);
        const auto what = text.fromFirstOccurrenceOf (":", false, false).trim();
        auto* track = findTrack (engine, trackName);
        int id = 0;

        if (what.equalsIgnoreCase ("new reverb")) id = 101;
        else if (what.equalsIgnoreCase ("new delay")) id = 102;
        else if (what.equalsIgnoreCase ("new empty")) id = 103;
        else
        {
            const auto returns = routing::getReturns (engine.getTracks());

            for (size_t i = 0; i < returns.size(); ++i)
                if (returns[i]->name == what)
                    id = (int) i + 1;
        }

        if (track != nullptr && id != 0)
            arrangement.getTrackActions().sendsButtonChoice (*track, id);
        else
            error ("bad sendsChoice \"" + text + "\"");
    }

    if (shot.hasProperty ("beat"))
        engine.setPositionBeats ((double) shot["beat"]);

    if (shot["play"])
        engine.play();
    else
        engine.stop();

    if (shot.hasProperty ("panel"))
    {
        const auto panel = shot["panel"].toString();

        if (panel == "mixer")
            main.showBottomPanel (MainComponent::BottomPanel::mixer);
        else if (panel == "clipEditor")
            main.showBottomPanel (MainComponent::BottomPanel::clipEditor);
        else if (panel == "closed")
            main.showBottomPanel (MainComponent::BottomPanel::closed);
        else
            error ("\"panel\" is mixer, clipEditor or closed, not \"" + panel + "\"");
    }

    if (shot.hasProperty ("clipEditorMode"))
    {
        const auto mode = shot["clipEditorMode"].toString();

        if (mode == "score" || mode == "pianoRoll")
            main.pianoRoll.setScoreMode (mode == "score");
        else
            error ("\"clipEditorMode\" is pianoRoll or score, not \"" + mode + "\"");
    }

    if (shot.hasProperty ("scoreClef"))
    {
        // The selected MIDI track's score clef, as the Clef menu sets it.
        static const juce::StringArray names { "auto", "treble", "bass", "grand", "guitar", "bassGuitar" };
        const auto index = names.indexOf (shot["scoreClef"].toString());
        auto* track = main.selection.getTrack();

        if (index < 0)
            error ("\"scoreClef\" is auto, treble, bass, grand, guitar or bassGuitar");
        else if (track == nullptr || ! track->isMidi())
            error ("\"scoreClef\" needs a selected MIDI track");
        else
            engine.setScoreClef (*track, (ScoreClef) index);
    }

    if (shot.hasProperty ("sheetMusic"))
    {
        // The selected MIDI track as sheet music: the PDF and its first page as a PNG, next to the shot's file.
        const auto paper = shot["sheetMusic"].toString() == "letter" ? sheetmusic::Paper::letter : sheetmusic::Paper::a4;
        auto* track = main.selection.getTrack();

        if (track == nullptr || ! track->isMidi())
        {
            error ("\"sheetMusic\" needs a selected MIDI track");
        }
        else
        {
            const auto part = sheetmusic::getPart (engine, *track, 0.0);
            const sheetmusic::Header header { "Example song", track->name, gen::sheet::getKeyName (part.input.key),
                                              juce::roundToInt (part.input.tempo.getBpmAt (0.0)) };
            const auto stem = state.folder.getChildFile (file).getFileNameWithoutExtension();
            const auto pdfFile = state.folder.getChildFile (stem + ".pdf");

            if (const auto result = sheetmusic::exportPdf (part, header, paper, pdfFile); result.failed())
            {
                error (result.getErrorMessage());
            }
            else
            {
                std::cout << "screenshots: " << pdfFile.getFullPathName() << std::endl;
                const auto pages = sheetmusic::renderPages (part, header, paper, 1.5f);

                for (size_t i = 0; i < pages.size(); ++i)
                {
                    const auto png = state.folder.getChildFile (stem + "-page" + juce::String (i + 1) + ".png");
                    png.deleteFile();

                    if (auto stream = png.createOutputStream())
                        juce::PNGImageFormat().writeImageToStream (pages[i], *stream);

                    std::cout << "screenshots: " << png.getFullPathName() << std::endl;
                }
            }
        }
    }

    if (shot.hasProperty ("inspector"))
        main.setInspectorOpen ((bool) shot["inspector"]);

    if (shot.hasProperty ("songRowsFolded"))
        arrangement.setSongRowsFolded ((bool) shot["songRowsFolded"]);

    if (shot.hasProperty ("chordsOpen"))
        arrangement.chordLane.setExpanded ((bool) shot["chordsOpen"]);

    if (shot.hasProperty ("lyricsOpen"))
        arrangement.lyricsLane.setExpanded ((bool) shot["lyricsOpen"]);

    if (shot.hasProperty ("chordDiagrams"))
        arrangement.chordLane.setInstrument (shot["chordDiagrams"].toString() == "guitar" ? ChordLane::Instrument::guitar
                                                                                         : ChordLane::Instrument::piano);

    if (shot.hasProperty ("zoom"))
        arrangement.timeline.pixelsPerBeat = juce::jlimit (2.0, 400.0, (double) shot["zoom"]);

    if (shot.hasProperty ("zoom") || shot.hasProperty ("viewStart"))
        arrangement.setViewStart (shot.hasProperty ("viewStart") ? (double) shot["viewStart"] : arrangement.timeline.viewStartBeat);

    if (shot.hasProperty ("guide"))
    {
        const auto wanted = shot["guide"].toString();
        std::optional<gen::guides::Guide> found;

        for (auto guide : gen::guides::allGuides)
            if (getGuideKey (guide).equalsIgnoreCase (wanted) || gen::guides::getEntry (guide).name.equalsIgnoreCase (wanted))
                found = guide;

        if (found.has_value())
        {
            const auto entry = gen::guides::getEntry (*found);
            const auto topics = entry.topics.isEmpty() && entry.plugin.isNotEmpty() ? BuiltInGuides::getPageNames (entry.plugin) : entry.topics;
            int topic = -1;

            if (shot["topic"].isString())
            {
                topic = topics.indexOf (shot["topic"].toString(), true);

                if (topic < 0)
                    error ("the " + entry.name + " guide has no topic \"" + shot["topic"].toString() + "\": " + topics.joinIntoString (", "));
            }
            else if (shot.hasProperty ("topic"))
            {
                topic = (int) shot["topic"];
            }

            bool openedOnTrack = false;
            if (shot.hasProperty ("track") && entry.plugin.isNotEmpty())
                if (auto* track = findTrack (engine, shot["track"].toString()))
                    for (auto& slot : track->plugins)
                        if (slot->instance != nullptr && slot->instance->getName() == entry.plugin)
                        {
                            plugins::open (main.pluginWindows, *track, *slot);
                            if (auto* editor = dynamic_cast<fxui::EffectEditor*> (slot->instance->getActiveEditor()))
                            {
                                editor->openGuide (topic);
                                openedOnTrack = true;
                            }
                            break;
                        }
            if (! openedOnTrack)
                main.openGuide (*found, topic);
            if ((bool) shot["clap"])
                for (int windowIndex = 0; windowIndex < juce::TopLevelWindow::getNumTopLevelWindows(); ++windowIndex)
                    if (auto* window = dynamic_cast<juce::DocumentWindow*> (juce::TopLevelWindow::getTopLevelWindow (windowIndex)))
                        if (window->isVisible() && window->getName() == entry.name)
                            if (auto* effectGuide = dynamic_cast<fxui::Guide*> (window->getContentComponent()))
                                effectGuide->playTestSound();
        }
        else
        {
            juce::StringArray keys;

            for (auto guide : gen::guides::allGuides)
                keys.add (getGuideKey (guide));

            error ("no guide \"" + wanted + "\": " + keys.joinIntoString (", "));
        }
    }

    if (shot.hasProperty ("plugin"))
    {
        auto* track = findTrack (engine, shot["track"].toString());
        PluginSlot* slot = nullptr;

        if (track != nullptr)
            for (auto& s : track->plugins)
                if (s->instance != nullptr && s->instance->getName().equalsIgnoreCase (shot["plugin"].toString()))
                    slot = s.get();

        if (slot != nullptr)
        {
            if (auto* tuner = dynamic_cast<BuiltInTune*> (slot->instance.get()))
            {
                if (shot.hasProperty ("preset"))
                {
                    bool foundPreset = false;

                    for (const auto& preset : tune::getPresets())
                        if (preset.name.equalsIgnoreCase (shot["preset"].toString()))
                        {
                            tuner->applyPreset (preset);
                            foundPreset = true;
                            break;
                        }

                    if (! foundPreset)
                        error ("no starting point called \"" + shot["preset"].toString() + "\"");
                }

                if ((bool) shot["clap"])
                    tuner->clap.request (fx::Clap::Sound::offNote);
            }

            if (auto* stereoPlugin = dynamic_cast<BuiltInStereo*> (slot->instance.get()))
            {
                if (shot.hasProperty ("preset"))
                {
                    bool foundPreset = false;
                    for (const auto& preset : stereo::getPresets())
                        if (preset.name.equalsIgnoreCase (shot["preset"].toString()))
                        {
                            stereoPlugin->applyPreset (preset);
                            foundPreset = true;
                            break;
                        }
                    if (! foundPreset)
                        error ("no starting point called \"" + shot["preset"].toString() + "\"");
                }
                if ((bool) shot["clap"])
                    stereoPlugin->clap.request (fx::Clap::Sound::wideBass);
            }
            plugins::open (main.pluginWindows, *track, *slot);
        }
        else
            error ("no " + shot["plugin"].toString() + " on a track called \"" + shot["track"].toString() + "\"");
    }

    if (shot.hasProperty ("scene"))
    {
        const auto scene = shot["scene"].toString();

        if (scene == "theoryGuides")
            main.showTheoryGuides();
        else if (scene == "bounce")
        {
            // The Bounce to WAV window with the shot's "select" tracks selected and *Selected tracks* picked.
            bounceui::showDialog (engine, main.project.getProjectName(), main.selection.getTracks(), true);
        }
        else if (scene == "bounceInPlace")
        {
            // Bounces the shot's "track" in place (waiting for it, with no progress window) and selects the new track.
            auto* track = findTrack (engine, shot["track"].toString());

            if (track == nullptr)
                error ("no track called \"" + shot["track"].toString() + "\" to bounce in place");
            else if (const auto result = bounce::bounceInPlace (engine, *track); result.result.failed())
                error (result.result.getErrorMessage());
            else
                main.selection.select (result.track, result.clip);
        }
        else if (scene == "tune")
        {
            juce::String problem;

            if (! setUpTuneScene (engine, problem))
                error (problem);
        }
        else if (scene == "pluginEditor")
        {
            // The shot's "builtIn" (a built-in's name, such as "DawIt Reverb") opened as a plugin host would, in a plain
            // window, with no track around it. "overlay": true adds its guide overlay, at the shot's "page" (default: as it opens).
            juce::PluginDescription found;
            auto all = BuiltInFormat::getEffects();
            all.addArray (BuiltInFormat::getInstruments());

            for (const auto& d : all)
                if (d.name.equalsIgnoreCase (shot["builtIn"].toString()) || d.fileOrIdentifier.equalsIgnoreCase (shot["builtIn"].toString()))
                    found = d;

            std::unique_ptr<juce::AudioProcessor> processor;
            BuiltInFormat format;
            juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_VST3);

            if (found.name.isNotEmpty())
                {
                juce::String message;
                processor = format.createInstanceFromDescription (found, 44100.0, 512, message);
            }

            juce::AudioProcessor::setTypeOfNextNewPlugin (juce::AudioProcessor::wrapperType_Undefined);

            if (processor == nullptr)
                error ("no built-in called \"" + shot["builtIn"].toString() + "\"");
            else
            {
                // A plugin has no Export PDF button, so the app's hook is off while the guide is made.
                auto exportHook = std::move (guideprint::exportButtonFactory());
                guideprint::exportButtonFactory() = {};

                auto* window = new PluginEditorWindow (std::move (processor));
                state.pluginEditorWindows.add (window);

                if ((bool) shot["overlay"] && ! window->openGuide (shot.hasProperty ("page") ? (int) shot["page"] : -1))
                    error (shot["builtIn"].toString() + " has no guide to open");

                guideprint::exportButtonFactory() = std::move (exportHook);
            }
        }
        else if (scene == "tuner")
        {
            // The Tuner window with a made-up reading, no microphone needed: "hz" is the pitch it shows (default 109.24, an A string
            // 12 cents flat); the tuning and reference come from the shot's "settings" (tuner.tuning, tuner.referenceHz).
            main.showTuner();

            if (auto* window = main.tunerWindow.getComponent())
                window->showExample (shot.hasProperty ("hz") ? (double) shot["hz"] : 109.24);
        }
        else if (scene == "recording")
        {
            // A Voice audio track armed on Input 1 with a stack of three takes, the newest playing. Add "audioSettings": true
            // for the Audio / MIDI Settings panel too. The tracks stay as they are for the shots after it.
            juce::String problem;

            if (! setUpRecordingScene (engine, problem))
                error (problem);
            else if (auto* vocals = findTrack (engine, "Voice"))
                main.selection.select (vocals, nullptr);
        }
        else if (scene == "drumLoop")
        {
            // An audio track "Drum loop" with the drum trigger guide's example loop (2 bars, 120 BPM), and the Hits to MIDI window
            // open on it with the candidates found at once. "makeTrack": true then presses Make MIDI track (the new track "Drum loop
            // hits" sits below it). Sensitivity and the rest come from the shot's "settings" (hits.sensitivityDb and so on).
            auto* track = findTrack (engine, "Drum loop");
            AudioClip* clip = nullptr;

            if (track == nullptr)
            {
                // The loop is 2 bars at 120 BPM, whatever the song's tempo.
                engine.setTempo (120.0);
                track = engine.addTrack (Track::Type::audio);

                if (track != nullptr)
                {
                    track->name = "Drum loop";
                    clip = engine.addAudioClip (*track, hitstomidi::renderExampleLoop (44100.0), 44100.0, 0.0, "Drum loop");
                }
            }
            else
            {
                for (auto& c : track->clips)
                    if (c->kind == Clip::Kind::audio)
                        clip = static_cast<AudioClip*> (c.get());
            }

            if (track == nullptr || clip == nullptr)
                error ("could not make the Drum loop track");
            else
            {
                main.selection.select (track, clip);

                if (auto* window = arrangement.getHitsToMidi().openWith (*track, *clip, analysis::hits::findCandidates (clip->audio, clip->sourceSampleRate)))
                {
                    if ((bool) shot["makeTrack"])
                        window->makeTrack();
                }
                else
                    error ("the Hits to MIDI window did not open");
            }
        }
        else if (scene == "audioSettings")
            main.showAudioSettings();
        else if (scene == "soundClip")
        {
            // The sound guide's example clip on an audio track, open in the wave editor at 1.0 s, so What is sound? measures it.
            juce::String problem;

            if (auto* clip = addSoundClipTrack (engine, problem))
            {
                main.selection.select (findTrack (engine, "Example clip"), clip);
                main.selection.dispatchPendingMessages();   // so the wave editor has the clip before it is zoomed
                main.showBottomPanel (MainComponent::BottomPanel::clipEditor);
                main.waveEditor.showSeconds (0.0, 2.0);     // its middle is 1.0 s, inside the first held note
            }
            else
                error (problem);
        }
        else if (scene == "newFromStyle")
            main.showNewFromStyle (shot["style"].toString());
        else if (scene == "playAlong")
            arrangement.showPlayAlong();
        else if (scene == "singAlong")
            arrangement.showSingAlong();
        else if (scene == "lyricsText")
            arrangement.lyricsLane.editText();
        else if (scene == "lyricsEditor")
        {
            // The lyrics window with a made-up answer (no model): "lyricsAi" is rewrite, song, rhymes or noOllama.
            arrangement.showLyricsEditor();

            if (auto* window = arrangement.lyricsEditor.getComponent())
                window->showCanned (shot.hasProperty ("lyricsAi") ? shot["lyricsAi"].toString() : juce::String ("rewrite"));
            else
                error ("the lyrics window did not open");
        }
        else if (scene == "signalFlow")
        {
            // The Signal Flow window, fitted unless "flowZoom" says a percent ("flowZoom": 150) or "fit"; it scrolls to the selected track.
            main.showSignalFlow();

            if (auto* view = main.signalFlow.getComponent())
            {
                const auto zoom = shot["flowZoom"].toString();

                if (zoom.isEmpty() || zoom == "fit")
                    view->fit();
                else
                    view->setZoom (zoom.getFloatValue() / 100.0f);

                if (auto* selected = main.selection.getTrack(); selected != nullptr && ! view->isFitted())
                    view->showTrack (*selected);
            }
        }
        else if (scene == "sing")
        {
            // The Sing window on Lead vocal's Melody clip. "voices": a folder to use as the Voices folder, or "none"
            // for an empty one (the window without a voice); "voice": which voice in it to pick.
            if (shot.hasProperty ("voices"))
            {
                const auto voices = shot["voices"].toString();
                sing::overrideVoicesFolder (voices == "none" ? juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("dawit-no-voices")
                                                             : juce::File::getCurrentWorkingDirectory().getChildFile (voices));
            }

            auto* track = findTrack (engine, shot.hasProperty ("track") ? shot["track"].toString() : juce::String ("Lead vocal"));
            MidiClip* clip = nullptr;

            if (track != nullptr)
                for (auto& c : track->clips)
                    if (clip == nullptr && c->kind == Clip::Kind::midi)
                        clip = static_cast<MidiClip*> (c.get());

            if (track == nullptr)
                error ("no track called \"" + shot["track"].toString() + "\" for the Sing window");
            else
            {
                // "thinNotes": N removes every Nth note of the clip first, so the notes no longer match the words.
                if (clip != nullptr && (int) shot["thinNotes"] > 1)
                {
                    const int every = (int) shot["thinNotes"];
                    const juce::ScopedLock sl (engine.getLock());

                    for (int i = (int) clip->notes.size() - 1; i >= 0; --i)
                        if (i % every == every - 1)
                            clip->notes.erase (clip->notes.begin() + i);

                    engine.sendChangeMessage();
                }

                arrangement.singActions.open (*track, clip);

                if (shot.hasProperty ("voice") && arrangement.singActions.getWindow() != nullptr)
                    arrangement.singActions.getWindow()->selectVoice (shot["voice"].toString());

                // "thinNotes": 5 first removes every 5th note; "match": true presses Match the notes to the words; "makeTrack": true then makes the new track.
                if (auto* w = arrangement.singActions.getWindow())
                {
                    if (static_cast<bool> (shot["match"]) && w->matchButton.isVisible())
                        w->matchButton.onClick();

                    if (static_cast<bool> (shot["makeTrack"]) && w->makeTrackButton.isVisible())
                        w->makeTrackButton.onClick();
                }

                // "sing": true presses Sing; give the shot a long "wait", the voice takes a few seconds.
                if (static_cast<bool> (shot["sing"]) && arrangement.singActions.getWindow() != nullptr)
                    arrangement.singActions.getWindow()->singButton.onClick();
            }
        }
        else
            error ("no scene \"" + scene + "\": " + getSceneNames().joinIntoString (", "));
    }

    return ok;
}

juce::Component* ScreenshotRunner::findTarget (State& state, const juce::var& shot)
{
    auto& main = state.main;
    const auto file = shot["file"].toString();
    auto capture = shot["capture"].toString();

    if (capture.isEmpty())
        capture = state.opened.isEmpty() ? "main" : "window";

    if (capture == "main")         return &main;
    if (capture == "arrangement")  return &main.arrangement;
    if (capture == "deviceBar")    return &main.deviceBar;
    if (capture == "transport")    return &main.transport;
    if (capture == "bottomPanel")  return &main.getBottomPanel();
    if (capture == "inspector")    return &main.inspector;

    if (capture == "window")
    {
        if (auto* window = state.opened.isEmpty() ? nullptr : state.opened.getLast().getComponent())
            return getContent (*window);

        state.fail (file, "the shot opened no window to capture");
        return nullptr;
    }

    for (auto* window : getVisibleWindows())
        if (window->getName() == capture)
            return getContent (*window);

    juce::StringArray names;

    for (auto* window : getVisibleWindows())
        names.add (window->getName());

    state.fail (file, "no window \"" + capture + "\" to capture; open: " + names.joinIntoString (", "));
    return nullptr;
}
} // namespace daw
