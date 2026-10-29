#include "GuideButton.h"
#include "TuneEditor.h"

namespace daw
{
using namespace fxui;

namespace
{
    constexpr int topicTuner = 50, topicTrace = 51, topicLevel = 52;

    juce::String latencyText (const BuiltInTune& p)
    {
        const double rate = p.getSampleRate() > 0.0 ? p.getSampleRate() : 44100.0;
        return juce::String (juce::roundToInt (p.getLatency() / rate * 1000.0)) + " ms";
    }
}

//==============================================================================
TuneView::TuneView (BuiltInTune& p, std::function<void (int)> open, std::function<void (int)> hover)
    : processor (p), guide (std::move (open)), hoverTopic (std::move (hover))
{
    setTopic (*this, topicTrace);
}

juce::Rectangle<float> TuneView::getTunerArea() const { return getLocalBounds().toFloat().reduced (5.0f).removeFromLeft (tunerWidth); }

juce::Rectangle<float> TuneView::getTraceArea() const
{
    auto r = getLocalBounds().toFloat().reduced (5.0f);
    r.removeFromLeft (tunerWidth + 6.0f);
    return r.withTrimmedBottom (levelHeight + 6.0f);
}

juce::Rectangle<float> TuneView::getLevelArea() const
{
    auto r = getLocalBounds().toFloat().reduced (5.0f);
    r.removeFromLeft (tunerWidth + 6.0f);
    return r.removeFromBottom (levelHeight);
}

double TuneView::getSeconds() const
{
    if (auto* settings = tunetext::getSettings())
        return juce::jlimit (4.0, 16.0, settings->getDoubleValue ("view.tuneTraceSeconds", 8.0));

    return 8.0;
}

tunepics::Trace TuneView::makeTrace() const
{
    tunepics::Trace t;
    t.frames = &processor.getRecentFrames();
    t.framesPerSecond = processor.getFramesPerSecond();
    t.seconds = getSeconds();
    t.keyMask = processor.getKeyMask();
    t.showCorrected = processor.getSettings().mode == tune::Mode::correct && ! processor.hearOriginal.load();
    return t;
}

tunepics::KeyInfo TuneView::getKeyInfo() const
{
    return { tunetext::getSong().key.getName(), processor.getKeyMask() };
}

void TuneView::paint (juce::Graphics& g)
{
    auto tuner = getTunerArea();
    drawPanel (g, tuner);
    tunepics::paintTuner (g, tuner.reduced (6.0f), processor.liveSung.load(), getKeyInfo());

    tunepics::paintTrace (g, getTraceArea(), makeTrace());

    // The level of the sound over the same time.
    auto level = getLevelArea();
    drawPanel (g, level);
    const auto& frames = processor.getRecentFrames();
    const int shown = juce::jmax (2, juce::roundToInt (getSeconds() * processor.getFramesPerSecond()));
    const int first = juce::jmax (0, (int) frames.size() - shown);
    auto plot = level.reduced (8.0f, 6.0f);
    juce::Path path;
    bool started = false;

    for (int i = first; i < (int) frames.size(); ++i)
    {
        const float x = plot.getRight() - (float) ((int) frames.size() - 1 - i) * plot.getWidth() / (float) shown;
        const float v = juce::jlimit (0.0f, 1.0f, (frames[(size_t) i].rmsDb + 70.0f) / 70.0f);
        const juce::Point<float> p { x, plot.getBottom() - v * plot.getHeight() };
        started ? path.lineTo (p) : path.startNewSubPath (p);
        started = true;
    }

    g.setColour (theme::textDim);
    g.strokePath (path, juce::PathStrokeType (1.2f));
    g.setFont (juce::FontOptions (10.5f));
    g.drawText ("level of the sound, last " + juce::String (juce::roundToInt (getSeconds())) + " s", level.reduced (8.0f, 2.0f), juce::Justification::topLeft);
}

void TuneView::mouseMove (const juce::MouseEvent& e)
{
    if (hoverTopic == nullptr)
        return;

    hoverTopic (getTunerArea().contains (e.position) ? topicTuner : getLevelArea().contains (e.position) ? topicLevel : topicTrace);
}

void TuneView::mouseDown (const juce::MouseEvent& e)
{
    const auto trace = makeTrace();
    const auto hit = tunepics::hitTrace (getTraceArea(), trace, e.position);

    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        const double seconds = getSeconds();

        for (const int s : { 4, 8, 16 })
            menu.addItem (s, "Show " + juce::String (s) + " seconds", true, std::abs (seconds - s) < 0.5);

        menu.addSeparator();
        guideicon::addItem (menu, 100, "What is pitch correction?");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<TuneView> (this)] (int choice)
        {
            if (safe == nullptr || choice == 0)
                return;

            if (choice == 100)
            {
                if (safe->guide)
                    safe->guide (0);
            }
            else if (auto* settings = tunetext::getSettings())
            {
                settings->setValue ("view.tuneTraceSeconds", (double) choice);
                safe->repaint();
            }
        });
        return;
    }

    if (hit.valid && hit.beat >= 0.0)
        tunetext::seekToBeat (hit.beat);
}

juce::String TuneView::getTooltip()
{
    const auto mouse = getMouseXYRelative().toFloat();

    if (getTunerArea().contains (mouse))
        return "The nearest note to what is sung now, how many cents sharp or flat, and whether it is in the song's key.";

    if (getLevelArea().contains (mouse))
        return "How loud the sound was over the same seconds as the trace.";

    const auto hit = tunepics::hitTrace (getTraceArea(), makeTrace(), mouse);

    if (! hit.valid)
        return "The pitch of the last seconds on the piano roll's rows. Click to move the playhead; right-click to show more or fewer seconds.";

    return tunepics::describeHit (hit, getKeyInfo());
}

//==============================================================================
BuiltInTuneEditor::BuiltInTuneEditor (BuiltInTune& p)
    : EffectEditor (p, "DawIt Tune", "What is pitch correction?", "Hear the original", p.hearOriginal, p.clap, tunepics::colour, fx::Clap::Sound::offNote),
      tuner (p), view (p, [this] (int page) { openGuide (page); }, [this] (int topic) { hover (topic); })
{
    addAndMakeVisible (view);

    const std::pair<tune::Knob, const char*> names[] = { { tune::Knob::speed, "Speed" }, { tune::Knob::amount, "Amount" }, { tune::Knob::tolerance, "Tolerance" } };

    for (const auto& [knob, name] : names)
        knobs.add (new EffectKnob (p.getParameters(), BuiltInTune::getParameterId (knob), name, (int) knob, tunepics::colour));

    knobs[0]->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    knobs[0]->slider.setDoubleClickReturnValue (true, 50.0);
    knobs[1]->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + "%"; };
    knobs[1]->slider.setDoubleClickReturnValue (true, 100.0);
    knobs[2]->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " cents"; };
    knobs[2]->slider.setDoubleClickReturnValue (true, 10.0);

    for (auto* k : knobs)
        k->slider.updateText();

    knobs[0]->slider.setTooltip ("Time to get 95% of the way to the target note. Double-click for 50 ms.");
    knobs[1]->slider.setTooltip ("How much of the error is removed.");
    knobs[2]->slider.setTooltip ("Notes closer than this to their target are left alone.");

    mode.button.setTooltip ("Check: only measure. Correct: also pull notes into tune.");
    target.button.setTooltip ("Which notes a sung note can be pulled to.");
    keepVibrato.button.setTooltip ("On: correct the middle of the note and keep its vibrato.");
    mode.button.onClick = [this] { auto s = tuner.getSettings(); s.mode = s.mode == tune::Mode::check ? tune::Mode::correct : tune::Mode::check; tuner.setSettings (s); };
    target.button.onClick = [this]
    {
        auto s = tuner.getSettings();
        s.target = s.target == tune::Target::key ? tune::Target::chords : (s.target == tune::Target::chords ? tune::Target::chromatic : tune::Target::key);
        tuner.setSettings (s);
    };
    keepVibrato.button.onClick = [this] { auto s = tuner.getSettings(); s.keepVibrato = ! s.keepVibrato; tuner.setSettings (s); };
    listenButton.setTooltip ("Hold the correction back to compare (not saved).");

    setGroups ({ { "MODE", { &mode }, 0 }, { "TARGET", { &target }, 0 }, { "MOVE NOTES", { knobs[0], knobs[1], knobs[2] }, 0 }, { "HUMAN TOUCH", { &keepVibrato }, 0 } });

    juce::StringArray presetNames;

    for (const auto& preset : tune::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    clapButton.setButtonText ("Play an off note");
    clapButton.setTooltip ("Play a slightly out-of-tune sung note through DawIt Tune.");

    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 640);
}

void BuiltInTuneEditor::applyPreset (int index)
{
    const auto& list = tune::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    tuner.applyPreset (list[(size_t) index]);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInTuneEditor::createGuidePages() { return tunetext::makeGuide (tuner); }

void BuiltInTuneEditor::layoutPictures (juce::Rectangle<int> r) { view.setBounds (r); }

void BuiltInTuneEditor::tick()
{
    tuner.drainFrames();
    const auto s = tuner.getSettings();
    mode.button.setButtonText (s.mode == tune::Mode::check ? "Check" : "Correct");
    mode.button.setToggleState (s.mode == tune::Mode::correct, juce::dontSendNotification);
    target.button.setButtonText (s.target == tune::Target::key ? "Key" : (s.target == tune::Target::chords ? "Chords" : "Chromatic"));
    keepVibrato.button.setButtonText (s.keepVibrato ? "On" : "Off");
    keepVibrato.button.setToggleState (s.keepVibrato, juce::dontSendNotification);

    // In Check mode the correction controls are dimmed, but still work.
    const float alpha = s.mode == tune::Mode::check ? 0.5f : 1.0f;

    for (juce::Component* c : std::initializer_list<juce::Component*> { &target, &keepVibrato, knobs[0], knobs[1], knobs[2] })
        c->setAlpha (alpha);

    view.repaint();
}

juce::AttributedString BuiltInTuneEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto colour = tunepics::colour;

    struct Control { const char* title; const char* description; const char* typical; };
    static const Control controls[] = {
        { "Mode", "Check only measures: you hear the track as it is. Correct also moves each note towards its target. Both make the track late inside, which DawIt makes up for on an audio track, so switching never moves the track in time.",
          "Check first, Correct only if the take needs it." },
        { "Target", "Key: the notes of the song's key (C major: C, D, E, F, G, A, B). Chords: the notes of the chord in that bar; bars without a chord use the key. Chromatic: every semitone, nothing moves more than half a semitone.",
          "Key; Chromatic when the song borrows notes." },
        { "Speed", "How fast a note reaches its target: the time to get 95% of the way, from 0 ms (instant, the hard-tuned sound) to 400 ms (only long notes get there).",
          "About 50 ms for a natural fix, 100 to 150 ms for a gentle polish, 0 for hard tune." },
        { "Amount", "How much of the error is removed. 100%: all of it; 60%: a note 30 cents flat ends 12 cents flat.",
          "100% for a fix, 50 to 70% to keep it human." },
        { "Tolerance", "A band around each target, in cents, where nothing is touched: with 10, a note 8 cents flat is left as it was.",
          "5 to 15 cents; 0 for hard tune." },
        { "Keep vibrato", "Moves the middle of a note, not its swing, so vibrato, scoops and bends stay.",
          "On, except for hard tune." } };

    if (juce::isPositiveAndBelow (topic, 6))
    {
        addTitle (text, controls[topic].title, colour);
        addLead (text, controls[topic].description);
        addSection (text, "Typical", controls[topic].typical);
    }
    else if (topic == CommonTopic::presets && juce::isPositiveAndBelow (presetIndex, (int) tune::getPresets().size()))
    {
        const auto& p = tune::getPresets()[(size_t) presetIndex];
        addTitle (text, p.name, colour);
        addLead (text, p.why);
    }
    else if (topic == CommonTopic::listen)
    {
        addTitle (text, "Hear the original", colour);
        addLead (text, "The track without correction, at the same moment and level, to compare. Not saved with the project.");
    }
    else if (topic == CommonTopic::clap)
    {
        addTitle (text, "Play an off note", colour);
        addLead (text, "A sung \"ah\" on A3 that starts 150 cents low, slides up to 30 cents flat and holds there with a gentle vibrato, through the effect.");
    }
    else if (topic == topicTuner)
    {
        addTitle (text, "Tuner", colour);
        addLead (text, "The nearest note to what's sung now, how many cents sharp or flat, and whether it's in the song's key. Green within 10 cents, yellow within 25, red beyond. A dash means no clear pitch: silence, a consonant, a breath or several notes at once.");
    }
    else if (topic == topicTrace)
    {
        addTitle (text, "Trace", colour);
        addLead (text, "The pitch of the last seconds on the piano roll's rows: the key's notes shaded, the sung line coloured by how far out it is, the corrected line in blue. Hover for the note; click to move the playhead there; right-click to show more or fewer seconds.");
    }
    else if (topic == topicLevel)
    {
        addTitle (text, "Level", colour);
        addLead (text, "How loud the sound was over the same seconds. A gap in the trace while the level is up means no clear pitch: a consonant, a breath or several notes at once.");
    }
    else
    {
        const auto host = getHostTrack (tuner);
        addTitle (text, "DawIt Tune", colour);

        if (host.isBus || host.isReturn)
            addLead (text, juce::String ("On a ") + (host.isReturn ? "return" : "bus") + ", DawIt Tune makes everything in it " + latencyText (tuner) + " late. Put it on the vocal track itself.");
        else if (tuner.getSettings().mode == tune::Mode::check)
            addLead (text, "Only measuring: switch Mode to Correct to pull notes into tune.");
        else
            addLead (text, "Pulling notes towards the nearest note the Target allows. Turn on Hear the original to compare.");

        addSection (text, "", "Put it first on a single voice, on the vocal track: the tuner and the trace show every sung note, and Correct moves the ones that are out.");
    }

    return text;
}
}
