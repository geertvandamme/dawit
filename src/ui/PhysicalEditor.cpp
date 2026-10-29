#include "GuideButton.h"
#include "PhysicalEditor.h"

namespace daw
{
using phys::Exciter;
using phys::Knob;
using namespace fxui;

namespace
{
    void drawHeading (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText (text, r.removeFromTop (16.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    }

    /** How strongly the model moves: the newest note's level, or still when none plays. */
    float getShownLevel (const BuiltInPhysical& p)
    {
        return p.noteAge.load() >= 0.0f ? juce::jlimit (0.0f, 1.0f, p.shownLevel.load() * 6.0f) : 0.0f;
    }

    /** How long the pictures' note is held: as the newest note was, or as the test note is. */
    double getShownHold (const BuiltInPhysical& p, const phys::Settings& s)
    {
        if (p.releaseAge.load() >= 0.0f)
            return p.releaseAge.load();

        const bool sustained = s.exciter == Exciter::bow || s.exciter == Exciter::blow;
        return sustained ? BuiltInPhysical::testNoteSeconds : juce::jlimit (0.5, BuiltInPhysical::testNoteSeconds, (double) s.decaySeconds);
    }

    const char* describeExciter (Exciter e)
    {
        switch (e)
        {
            case Exciter::pluck:  return "a burst of noise, like a pick or a finger";
            case Exciter::strike: return "a hammer or a mallet";
            case Exciter::bow:    return "a bow that sticks and slips";
            case Exciter::blow:   return "breath through a reed, into a tube";
        }

        return "";
    }
}

//==============================================================================
PhysicalModelView::PhysicalModelView (BuiltInPhysical& p) : processor (p)
{
    setTopic (*this, phystext::model);
}

juce::Rectangle<float> PhysicalModelView::getPlot() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 8.0f).withTrimmedTop (22.0f);
}

void PhysicalModelView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    const auto s = processor.getSettings();
    const bool tube = phys::getResonator (s) == phys::Resonator::tube;
    const juce::String what = tube ? "THE TUBE" : "THE STRING";

    if (s.exciter == Exciter::blow)
        drawHeading (g, r, what + ": A REED AT ITS CLOSED END. RIGHT-CLICK FOR THE EXCITER");
    else
        drawHeading (g, r, what + ": DRAG ALONG IT TO MOVE WHERE IT'S " + juce::String (s.exciter == Exciter::pluck ? "PLUCKED" : s.exciter == Exciter::strike ? "STRUCK" : "BOWED")
                               + ". RIGHT-CLICK FOR MORE");

    // The string moves at a speed the eye can follow, a little over once a second for the fundamental.
    const double time = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    phase += juce::jlimit (0.0, 0.1, time - lastPaint) * 1.2;
    lastPaint = time;
    phystext::drawModel (g, getPlot(), s, phase, getShownLevel (processor), true);
}

void PhysicalModelView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    if (! phys::isUsed (processor.getSettings(), Knob::position))
        return;

    dragging = true;

    if (auto* p = processor.getKnobParameter (Knob::position))
        p->beginChangeGesture();

    mouseDrag (e);
}

void PhysicalModelView::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    processor.setKnob (Knob::position, phystext::getPositionAt (getPlot(), e.position.x));
    repaint();
}

void PhysicalModelView::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        if (auto* p = processor.getKnobParameter (Knob::position))
            p->endChangeGesture();

    dragging = false;
}

void PhysicalModelView::mouseMove (const juce::MouseEvent& e)
{
    const bool along = phys::isUsed (processor.getSettings(), Knob::position) && phystext::getResonatorArea (getPlot()).expanded (10.0f, 30.0f).contains (e.position);
    setMouseCursor (along ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void PhysicalModelView::showMenu()
{
    juce::PopupMenu menu;
    const auto s = processor.getSettings();

    for (int i = 0; i < phys::numExciters; ++i)
    {
        const auto e = (Exciter) i;
        menu.addItem (phys::getExciterName (e) + ": " + describeExciter (e), true, e == s.exciter, [this, i] { processor.setKnob (Knob::exciter, (float) i); repaint(); });
    }

    menu.addSeparator();
    menu.addItem ("String: every harmonic", s.exciter != Exciter::blow, phys::getResonator (s) == phys::Resonator::string,
                  [this] { processor.setKnob (Knob::resonator, 0.0f); repaint(); });
    menu.addItem ("Tube, closed at one end: only the odd harmonics", s.exciter != Exciter::blow, phys::getResonator (s) == phys::Resonator::tube,
                  [this] { processor.setKnob (Knob::resonator, 1.0f); repaint(); });
    menu.addSeparator();
    guideicon::addItem (menu, "What is physical modelling?", [this] { if (onOpenGuide) onOpenGuide(); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
}

//==============================================================================
/** The harmonics of the test note, measured from the sound: now while a note plays, otherwise at its start. */
class BuiltInPhysicalEditor::Harmonics final : public juce::Component
{
public:
    explicit Harmonics (BuiltInPhysical& p) : processor (p)
    {
        setTopic (*this, phystext::harmonics);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        const auto s = processor.getSettings();
        const double age = processor.noteAge.load();
        const double at = age >= 0.0 ? age : 0.1;
        drawHeading (g, r, "THE HARMONICS OF A3, " + juce::String (age >= 0.0 ? fxui::formatSeconds (at) + " IN" : juce::String ("AT ITS START"))
                               + ". RINGS: LEFT OUT BY THE POSITION");
        r.removeFromTop (4.0f);
        phystext::drawHarmonics (g, r, phys::measureHarmonics (cache.get (s, getShownHold (processor, s)), sub::testNoteHz, at, count), s, true);
    }

    static constexpr int count = 24;

private:
    BuiltInPhysical& processor;
    phystext::NoteCache cache;
};

//==============================================================================
/** How long each harmonic of the test note rings. */
class BuiltInPhysicalEditor::Rings final : public juce::Component
{
public:
    explicit Rings (BuiltInPhysical& p) : processor (p)
    {
        setTopic (*this, phystext::rings);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "HOW LONG EACH HARMONIC OF A3 RINGS");
        r.removeFromTop (4.0f);
        phystext::drawRingTimes (g, r, processor.getSettings(), 16, true);
    }

private:
    BuiltInPhysical& processor;
};

//==============================================================================
BuiltInPhysicalEditor::BuiltInPhysicalEditor (BuiltInPhysical& p)
    : EffectEditor (p, "DawIt Physical", "What is physical modelling?", "Hear the exciter alone", p.exciterOnly, p.clap, phystext::colour,
                    fx::Clap::Sound::note),
      synth (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "the whole sound", phystext::colour, "the exciter", {})
{
    harmonicsView = std::make_unique<Harmonics> (synth);
    ringsView = std::make_unique<Rings> (synth);
    modelView.onOpenGuide = [this] { guideButton.triggerClick(); };
    addAndMakeVisible (modelView);
    addAndMakeVisible (*harmonicsView);
    addAndMakeVisible (*ringsView);
    addAndMakeVisible (history);

    for (int k = 0; k < phys::numKnobs; ++k)
    {
        const auto knob = (Knob) k;

        switch (knob)
        {
            case Knob::exciter:
            case Knob::resonator:
            case Knob::mode:
                addSwitch (knob, phys::getKnobName (knob));
                break;

            case Knob::hardness:
            case Knob::position:
            case Knob::pressure:
            case Knob::noise:
            case Knob::attack:
            case Knob::decay:
            case Knob::damping:
            case Knob::release:
            case Knob::vibratoRate:
            case Knob::vibratoDepth:
            case Knob::glide:
            case Knob::velocity:
            case Knob::output:
            {
                const bool ofExciter = knob == Knob::hardness || knob == Knob::position || knob == Knob::pressure || knob == Knob::noise || knob == Knob::attack;
                const auto c = ofExciter ? phystext::exciterColour : knob == Knob::vibratoRate || knob == Knob::vibratoDepth ? phystext::vibratoColour : phystext::colour;
                knobs[knob] = std::make_unique<EffectKnob> (synth.getParameters(), BuiltInPhysical::getParameterId (knob), phys::getKnobName (knob), k, c);
                break;
            }
        }
    }

    switches.at (Knob::exciter)->button.onClick = [this]
    {
        juce::PopupMenu menu;
        const auto current = synth.getSettings().exciter;

        for (int i = 0; i < phys::numExciters; ++i)
        {
            const auto e = (Exciter) i;
            menu.addItem (phys::getExciterName (e) + ": " + describeExciter (e), true, e == current, [this, i] { synth.setKnob (Knob::exciter, (float) i); });
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&switches.at (Knob::exciter)->button));
    };

    auto knob = [this] (Knob k) -> juce::Component* { return knobs.at (k).get(); };
    auto toggle = [this] (Knob k) -> juce::Component* { return switches.at (k).get(); };

    setGroups ({ { "THE EXCITER: HOW IT'S PLAYED", { toggle (Knob::exciter), knob (Knob::hardness), knob (Knob::position), knob (Knob::pressure),
                                                     knob (Knob::noise), knob (Knob::attack) }, 0 },
                 { "THE RESONATOR: WHAT RINGS", { toggle (Knob::resonator), knob (Knob::decay), knob (Knob::damping), knob (Knob::release) }, 0 },
                 { "VIBRATO", { knob (Knob::vibratoRate), knob (Knob::vibratoDepth) }, 1 },
                 { "PLAYING", { knob (Knob::glide), toggle (Knob::mode), knob (Knob::velocity), knob (Knob::output) }, 1 } });

    juce::StringArray presetNames;

    for (auto& preset : phys::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    shownExciter = synth.getSettings().exciter;
    updateUsed();
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

BuiltInPhysicalEditor::Switch& BuiltInPhysicalEditor::addSwitch (Knob k, const juce::String& name)
{
    auto& s = *(switches[k] = std::make_unique<Switch> (name, (int) k));
    s.button.setColour (juce::TextButton::buttonColourId, (k == Knob::exciter ? phystext::exciterColour : phystext::colour).withAlpha (0.25f));
    s.button.onClick = [this, k]
    {
        if (auto* p = synth.getKnobParameter (k))
        {
            const int count = juce::jmax (2, (int) std::round (p->getNormalisableRange().end) + 1);
            const int next = (juce::roundToInt (p->convertFrom0to1 (p->getValue())) + 1) % count;
            synth.setKnob (k, (float) next);
        }
    };
    return s;
}

void BuiltInPhysicalEditor::updateUsed()
{
    const auto s = synth.getSettings();

    for (auto& [k, knob] : knobs)
    {
        const bool used = phys::isUsed (s, k);
        knob->setEnabled (used);
        knob->setAlpha (used ? 1.0f : 0.35f);
    }

    const bool resonatorUsed = phys::isUsed (s, Knob::resonator);
    switches.at (Knob::resonator)->setEnabled (resonatorUsed);
    switches.at (Knob::resonator)->setAlpha (resonatorUsed ? 1.0f : 0.35f);
}

void BuiltInPhysicalEditor::applyPreset (int index)
{
    const auto& list = phys::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    synth.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInPhysicalEditor::createGuidePages()
{
    return phystext::makeGuide (synth);
}

void BuiltInPhysicalEditor::layoutPictures (juce::Rectangle<int> r)
{
    modelView.setBounds (r.removeFromLeft (r.getWidth() * 42 / 100));
    r.removeFromLeft (10);
    harmonicsView->setBounds (r.removeFromTop ((r.getHeight() - 10) * 55 / 100));
    r.removeFromTop (10);
    ringsView->setBounds (r.removeFromLeft ((r.getWidth() - 10) / 2));
    r.removeFromLeft (10);
    history.setBounds (r);
}

void BuiltInPhysicalEditor::tick()
{
    const auto s = synth.getSettings();

    for (auto& [k, sw] : switches)
        sw->button.setButtonText (phys::formatKnob (k, s));

    // The knobs that matter change with the exciter.
    if (s.exciter != shownExciter)
    {
        shownExciter = s.exciter;
        updateUsed();

        if (getTopic() >= 0 && getTopic() < phys::numKnobs)
            showTopic (getTopic());
    }

    history.update();
    modelView.repaint();
    harmonicsView->repaint();
    ringsView->repaint();
}

juce::AttributedString BuiltInPhysicalEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = synth.getSettings();
    const auto& list = phys::getPresets();
    const phys::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, phystext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "Play a note, then change one knob at a time and listen. The knobs not named here are as in Init.");

        for (int k = 0; k < phys::numKnobs; ++k)
            if (preset->notes[(size_t) k].isNotEmpty())
                addSection (text, phystext::getTitle (k) + ": " + phys::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < phys::numKnobs)
    {
        const auto k = (Knob) topic;
        const bool ofExciter = k == Knob::hardness || k == Knob::position || k == Knob::pressure || k == Knob::noise || k == Knob::attack || k == Knob::exciter;
        addTitle (text, phystext::getTitle (topic), ofExciter ? phystext::exciterColour : phystext::colour);
        addLead (text, phystext::describeSettings (topic, s));

        if (! phys::isUsed (s, k))
            addSection (text, {}, "Greyed: " + phys::getExciterName (s.exciter).toLowerCase() + " doesn't use it.");

        addSection (text, "What it does", phystext::describe (topic));
        addSection (text, "How it's usually set", phystext::getTypical (topic));

        if (preset != nullptr && preset->notes[(size_t) topic].isNotEmpty())
            addSection (text, "In " + preset->name + ": " + phys::formatKnob (k, preset->settings), preset->notes[(size_t) topic]);
        else if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why the knobs are set the way they are for that sound.");

        return text;
    }

    addTitle (text, phystext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "A physical-modelling synthesiser: instead of building a sound from waves, it models the instrument. A string "
                           "or a tube is a loop a wave runs round, losing a little of its highs each trip, and an exciter sets it going: a "
                           "pluck, a strike, a bow or a breath. The sound comes out of the model, the way it comes out of a real guitar or clarinet.");
            addSection (text, "How the sound is made", "The exciter puts energy in where it plays the string. The wave runs to both ends and "
                                                       "back, and one trip round is one period: the loop's length is the pitch. Where it's "
                                                       "played decides which harmonics start; the decay and damping how long each rings. A "
                                                       "pluck or strike dies away; a bow or breath keeps it going while the key is held.");
            addSection (text, "Reading the window", "Left, the model: drag along the string to move where it's played, right-click for the "
                                                    "exciter or resonator. Right, the harmonics of the note as they are now, how long each "
                                                    "rings, and the last 4 seconds. Hover anything to read about it here.");
            addSection (text, "Getting started", "Pick a starting point; each tells why it sounds the way it does. Click Play a note, or play "
                                                 "the track's notes or a MIDI keyboard. Click Hear the exciter alone to hear what goes into "
                                                 "the string. Read What is physical modelling? for how it all works.");
            break;

        case CommonTopic::listen:
            addLead (text, synth.exciterOnly.load() ? "On: you hear only what the exciter puts in." : "Off: you hear the whole instrument.");
            addSection (text, {}, phystext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [before, after] = history.getLoudest();

            if (after > -70.0f)
                addLead (text, "The last 4 seconds: the exciter up to " + juce::String (before, 1) + " dB, the whole sound up to " + juce::String (after, 1) + " dB.");
            else
                addLead (text, "Play a note to see it here.");

            addSection (text, {}, phystext::describe (topic));
            break;
        }

        case phystext::model:
        {
            const auto missing = phystext::listMissing (s);
            addLead (text, phys::getExciterName (s.exciter) + " on a " + phys::getResonatorName (phys::getResonator (s)).toLowerCase()
                               + (s.exciter == Exciter::blow ? ", the reed at its closed end." : " at " + phys::formatKnob (Knob::position, s) + " of its length"
                                   + (missing.isEmpty() ? juce::String (".") : ": the " + missing + " harmonics are missing.")));
            addSection (text, {}, phystext::describe (topic));
            break;
        }

        case phystext::harmonics:
        {
            const auto levels = phys::getHarmonics (s, sub::testNoteHz, 0.1, Harmonics::count);
            int loudest = 0;

            for (int n = 1; n < (int) levels.size(); ++n)
                if (levels[(size_t) n] > levels[(size_t) loudest])
                    loudest = n;

            addLead (text, loudest == 0 ? juce::String ("At the start of A3 the loudest harmonic is the fundamental, 220 Hz.")
                                        : "At the start of A3 the loudest harmonic is number " + juce::String (loudest + 1) + ", at "
                                              + juce::String (juce::roundToInt ((loudest + 1) * sub::testNoteHz)) + " Hz.");
            addSection (text, {}, phystext::describe (topic));
            break;
        }

        case phystext::rings:
            addLead (text, phystext::describeSettings ((int) Knob::damping, s));
            addSection (text, {}, phystext::describe (topic));
            break;

        default:
            addSection (text, {}, phystext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
