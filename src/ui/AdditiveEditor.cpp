#include "GuideButton.h"
#include "AdditiveEditor.h"

namespace daw
{
using additive::Knob;
using additive::numPartials;
using namespace fxui;

namespace
{
    void drawHeading (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (text, r.removeFromTop (16.0f), juce::Justification::centredLeft);
    }

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    /** The newest note's swell and fade, or nothing while no note plays. */
    std::optional<std::array<float, numPartials>> getShownEnvelope (const BuiltInAdditive& p)
    {
        if (p.noteAge.load() < 0.0f)
            return std::nullopt;

        std::array<float, numPartials> envelope;

        for (size_t i = 0; i < envelope.size(); ++i)
            envelope[i] = p.shownPartials[i].load();

        return envelope;
    }
}

//==============================================================================
AdditivePartialsView::AdditivePartialsView (BuiltInAdditive& p) : processor (p)
{
    setTopic (*this, additext::partials);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> AdditivePartialsView::getPlot() const
{
    return additext::getPartialsPlot (getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedTop (20.0f));
}

void AdditivePartialsView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    const auto s = processor.getSettings();

    if (hovered > 0)
    {
        const double hz = sub::testNoteHz * additive::getRatio (hovered, s.stretchPercent);
        drawHeading (g, r, "THE " + additive::getOrdinal (hovered).toUpperCase() + " HARMONIC: " + formatHz (hz).toUpperCase() + " AT A3, DRAWN "
                               + additive::formatLevel (s.levelsDb[(size_t) hovered - 1]).toUpperCase());
    }
    else
    {
        drawHeading (g, r, "THE HARMONICS OF A3: DRAG TO DRAW, RIGHT-CLICK TO FILL");
    }

    r.removeFromTop (4.0f);
    const auto envelope = getShownEnvelope (processor);
    const bool lfoShows = s.lfoTarget != additive::LfoTarget::pitch && s.lfoDepthPercent > 0.5f;
    additext::drawPartials (g, r, s, envelope ? &*envelope : nullptr, envelope ? processor.shownVelocity.load() : 1.0f,
                            lfoShows ? processor.lfoValue.load() : 0.0f, [this] (int n) { return n == hovered; });
}

void AdditivePartialsView::beginGestures()
{
    for (int n = 1; n <= numPartials; ++n)
        if (auto* p = processor.getPartialParameter (n))
            p->beginChangeGesture();
}

void AdditivePartialsView::endGestures()
{
    for (int n = 1; n <= numPartials; ++n)
        if (auto* p = processor.getPartialParameter (n))
            p->endChangeGesture();
}

void AdditivePartialsView::draw (juce::Point<float> at)
{
    const auto plot = getPlot();
    const int n = additext::getPartialAt (plot, at.x);
    const float db = additext::getDbAt (plot, at.y);

    // A quick sweep skips bars between two mouse events, so draw a line through them.
    if (lastPartial > 0 && std::abs (n - lastPartial) > 1)
    {
        const int step = n > lastPartial ? 1 : -1;

        for (int m = lastPartial + step; m != n; m += step)
        {
            const float t = (float) (m - lastPartial) / (float) (n - lastPartial);
            const float between = lastDb + (db - lastDb) * t;
            processor.setPartialDb (m, between < additive::offDb + 1.0f ? additive::offDb : between);
        }
    }

    processor.setPartialDb (n, db);
    lastPartial = n;
    lastDb = db;
    hovered = n;
    repaint();
}

void AdditivePartialsView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    beginGestures();
    drawing = true;
    lastPartial = 0;
    draw (e.position);
}

void AdditivePartialsView::mouseDrag (const juce::MouseEvent& e)
{
    if (drawing)
        draw (e.position);
}

void AdditivePartialsView::mouseUp (const juce::MouseEvent&)
{
    if (drawing)
        endGestures();

    drawing = false;
    lastPartial = 0;
}

void AdditivePartialsView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;

    const int n = additext::getPartialAt (getPlot(), e.position.x);
    beginGestures();
    processor.setPartialDb (n, additive::offDb);
    endGestures();
    repaint();
}

void AdditivePartialsView::mouseMove (const juce::MouseEvent& e)
{
    const int n = getPlot().withTrimmedBottom (-18.0f).contains (e.position) ? additext::getPartialAt (getPlot(), e.position.x) : 0;

    if (n != hovered)
    {
        hovered = n;
        repaint();
    }
}

void AdditivePartialsView::mouseExit (const juce::MouseEvent&)
{
    hovered = 0;
    repaint();
}

void AdditivePartialsView::setAll (const std::array<float, numPartials>& levels)
{
    beginGestures();

    for (int n = 1; n <= numPartials; ++n)
        processor.setPartialDb (n, levels[(size_t) n - 1]);

    endGestures();
    repaint();
}

void AdditivePartialsView::showMenu()
{
    juce::PopupMenu menu;
    const std::pair<additive::Shape, const char*> shapes[] = {
        { additive::Shape::sine, "Fill as a sine: the 1st harmonic alone" },
        { additive::Shape::saw, "Fill as a saw: every harmonic at 1/n" },
        { additive::Shape::square, "Fill as a square: the odd ones at 1/n" },
        { additive::Shape::triangle, "Fill as a triangle: the odd ones at 1/n squared" },
        { additive::Shape::organ, "Fill as an organ: the 1st to 4th, as four drawbars" } };

    for (auto& [shape, name] : shapes)
        menu.addItem (name, [this, filled = shape] { setAll (additive::getShapeLevels (filled)); });

    menu.addItem ("Turn off all but the 1st", [this]
    {
        auto levels = processor.getSettings().levelsDb;
        std::fill (levels.begin() + 1, levels.end(), additive::offDb);
        setAll (levels);
    });

    menu.addSeparator();
    guideicon::addItem (menu, "What is additive synthesis?", [this] { if (onOpenGuide) onOpenGuide(); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
}

//==============================================================================
/** A few cycles of the test note: the harmonics added up, and the 1st alone. */
class BuiltInAdditiveEditor::Wave final : public juce::Component
{
public:
    explicit Wave (BuiltInAdditive& p) : processor (p)
    {
        setTopic (*this, additext::wave);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "THE WAVE: THE HARMONICS ADDED UP, AND THE 1ST ALONE");
        additext::drawWave (g, r, processor.getSettings(), sub::testNoteHz, processor.noteAge.load(), 3.0);
    }

private:
    BuiltInAdditive& processor;
};

//==============================================================================
/** A note over time, each harmonic a row, with the newest note on it. */
class BuiltInAdditiveEditor::OverTime final : public juce::Component
{
public:
    explicit OverTime (BuiltInAdditive& p) : processor (p)
    {
        setTopic (*this, additext::overTime);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "A NOTE OVER TIME: EACH HARMONIC A ROW, THE AMP ENVELOPE IN GREEN");
        const double age = processor.noteAge.load();
        additext::drawOverTime (g, r, processor.getSettings(), age, age >= 0.0 ? (double) processor.releaseAge.load() : -1.0);
    }

private:
    BuiltInAdditive& processor;
};

//==============================================================================
BuiltInAdditiveEditor::BuiltInAdditiveEditor (BuiltInAdditive& p)
    : EffectEditor (p, "DawIt Additive", "What is additive synthesis?", "Hear only the 1st harmonic", p.onlyFundamental, p.clap,
                    additext::colour, fx::Clap::Sound::note),
      synth (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "all the harmonics", additext::colour, "the 1st harmonic alone", {})
{
    wave = std::make_unique<Wave> (synth);
    overTime = std::make_unique<OverTime> (synth);
    partialsView.onOpenGuide = [this] { guideButton.triggerClick(); };
    addAndMakeVisible (partialsView);
    addAndMakeVisible (*wave);
    addAndMakeVisible (*overTime);
    addAndMakeVisible (history);

    for (int k = 0; k < additive::numKnobs; ++k)
    {
        const auto knob = (Knob) k;

        switch (knob)
        {
            case Knob::phases:
            case Knob::lfoTarget:
            case Knob::mode:
                addSwitch (knob, additive::getKnobName (knob));
                break;

            case Knob::brightness:
            case Knob::even:
            case Knob::stretch:
            case Knob::swell:
            case Knob::fade:
            case Knob::ampAttack:
            case Knob::ampDecay:
            case Knob::ampSustain:
            case Knob::ampRelease:
            case Knob::lfoRate:
            case Knob::lfoDepth:
            case Knob::glide:
            case Knob::velocity:
            case Knob::output:
            {
                const auto colour = k >= (int) Knob::swell && k <= (int) Knob::fade ? additext::moveColour
                                  : k >= (int) Knob::ampAttack && k <= (int) Knob::ampRelease ? additext::ampColour
                                  : k >= (int) Knob::lfoRate && k <= (int) Knob::lfoTarget ? additext::lfoColour
                                  : additext::colour;
                knobs[knob] = std::make_unique<EffectKnob> (synth.getParameters(), BuiltInAdditive::getParameterId (knob),
                                                            additive::getKnobName (knob), k, colour);
                break;
            }
        }
    }

    auto knob = [this] (Knob k) -> juce::Component* { return knobs.at (k).get(); };
    auto toggle = [this] (Knob k) -> juce::Component* { return switches.at (k).get(); };

    setGroups ({ { "HARMONICS", { knob (Knob::brightness), knob (Knob::even), knob (Knob::stretch), toggle (Knob::phases) }, 0 },
                 { "MOVEMENT", { knob (Knob::swell), knob (Knob::fade) }, 0 },
                 { "LFO", { knob (Knob::lfoRate), knob (Knob::lfoDepth), toggle (Knob::lfoTarget) }, 0 },
                 { "AMP ENVELOPE", { knob (Knob::ampAttack), knob (Knob::ampDecay), knob (Knob::ampSustain), knob (Knob::ampRelease) }, 1 },
                 { "PLAYING", { knob (Knob::glide), toggle (Knob::mode), knob (Knob::velocity), knob (Knob::output) }, 1 } });

    juce::StringArray presetNames;

    for (auto& preset : additive::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

BuiltInAdditiveEditor::Switch& BuiltInAdditiveEditor::addSwitch (Knob k, const juce::String& name)
{
    auto& s = *(switches[k] = std::make_unique<Switch> (name, (int) k));
    s.button.setColour (juce::TextButton::buttonColourId, additext::colour.withAlpha (0.25f));
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

void BuiltInAdditiveEditor::applyPreset (int index)
{
    const auto& list = additive::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    synth.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInAdditiveEditor::createGuidePages()
{
    return additext::makeGuide (synth);
}

void BuiltInAdditiveEditor::layoutPictures (juce::Rectangle<int> r)
{
    auto right = r.removeFromRight (r.getWidth() * 42 / 100);
    r.removeFromRight (10);
    partialsView.setBounds (r);
    const int third = (right.getHeight() - 20) / 3;
    wave->setBounds (right.removeFromTop (third));
    right.removeFromTop (10);
    overTime->setBounds (right.removeFromTop (third));
    right.removeFromTop (10);
    history.setBounds (right);
}

void BuiltInAdditiveEditor::tick()
{
    const auto s = synth.getSettings();

    for (auto& [k, sw] : switches)
        sw->button.setButtonText (additive::formatKnob (k, s));

    history.update();
    partialsView.repaint();
    wave->repaint();
    overTime->repaint();
}

juce::AttributedString BuiltInAdditiveEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = synth.getSettings();
    const auto& list = additive::getPresets();
    const additive::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, additext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "Play a note, then change one bar or knob at a time and listen. The knobs not named here are as in Init.");
        addSection (text, "The harmonics", preset->partials);

        for (int k = 0; k < additive::numKnobs; ++k)
            if (preset->notes[(size_t) k].isNotEmpty())
                addSection (text, additext::getTitle (k) + ": " + additive::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < additive::numKnobs)
    {
        addTitle (text, additext::getTitle (topic), additext::colour);
        addLead (text, additext::describeSettings (topic, s));
        addSection (text, "What it does", additext::describe (topic));
        addSection (text, "How it's usually set", additext::getTypical (topic));

        if (preset != nullptr && preset->notes[(size_t) topic].isNotEmpty())
            addSection (text, "In " + preset->name + ": " + additive::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);
        else if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why each knob is set the way it is for that sound.");

        return text;
    }

    addTitle (text, additext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "An additive synthesiser: it builds each note from sines, one at each harmonic of the note, every one at "
                           "its own level. Where a subtractive synth starts rich and takes away, this one starts from nothing and adds: "
                           "the way a Hammond organ's drawbars work, and how any sound can be taken apart.");
            addSection (text, "How the sound is made", "The bars set the level of each of the 32 harmonics. Brightness, Even and Stretch "
                                                       "change them all at once; Swell and Fade move them during a note, the high ones "
                                                       "coming in later or dying sooner; the amp envelope shapes each note's level; the "
                                                       "LFO moves the pitch, the brightness or the even harmonics.");
            addSection (text, "Reading the window", "Left, the harmonics as bars, the 1st (the note itself) on the left: drag over them "
                                                    "to draw, right-click to fill them with a shape. Right, the wave they add up to, a "
                                                    "note over time with a row per harmonic, and the last 4 seconds. Hover anything to "
                                                    "read about it here.");
            addSection (text, "Getting started", "Pick a starting point; each tells why it's made the way it is. Click Play a note, or "
                                                 "play the track's notes or a MIDI keyboard. Click Hear only the 1st harmonic to hear "
                                                 "what the others add. Read What is additive synthesis? for how it works and how to "
                                                 "build a sound step by step.");
            break;

        case CommonTopic::listen:
            addLead (text, synth.onlyFundamental.load() ? "On: you hear only the 1st harmonic." : "Off: you hear all the harmonics.");
            addSection (text, {}, additext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [before, after] = history.getLoudest();

            if (after > -70.0f)
                addLead (text, "The last 4 seconds: the 1st harmonic up to " + juce::String (before, 1) + " dB, the whole sound up to "
                                   + juce::String (after, 1) + " dB.");
            else
                addLead (text, "Play a note to see it here.");

            addSection (text, {}, additext::describe (topic));
            break;
        }

        case additext::partials:
        {
            int on = 0, highest = 0;

            for (int n = 1; n <= numPartials; ++n)
            {
                if (s.levelsDb[(size_t) n - 1] > additive::offDb + 0.05f)
                {
                    ++on;
                    highest = n;
                }
            }

            addLead (text, on == 0 ? juce::String ("Every harmonic is off: draw some to hear a sound.")
                                   : juce::String (on) + (on == 1 ? " harmonic is on" : " harmonics are on") + ", the highest the "
                                         + additive::getOrdinal (highest) + " (" + formatHz (sub::testNoteHz * additive::getRatio (highest, s.stretchPercent))
                                         + " at A3).");
            addSection (text, {}, additext::describe (topic));

            if (preset != nullptr)
                addSection (text, "In " + preset->name, preset->partials);

            break;
        }

        case additext::wave:
            addLead (text, "A3 peaks at about " + juce::String (additext::getPeakDb (s), 1) + " dB at full velocity.");
            addSection (text, {}, additext::describe (topic));
            break;

        case additext::overTime:
            addLead (text, additext::describeSettings ((int) Knob::fade, s) + " " + additext::describeSettings ((int) Knob::swell, s));
            addSection (text, {}, additext::describe (topic));
            break;

        default:
            addSection (text, {}, additext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
