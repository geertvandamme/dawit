#include "GuideButton.h"
#include "FmEditor.h"

namespace daw
{
using fm::Knob;
using fm::numOperators;
using namespace fxui;

namespace
{
    void drawHeading (juce::Graphics& g, juce::Rectangle<float>& r, const juce::String& text)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText (text, r.removeFromTop (16.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    }

    juce::String formatHz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    /** The newest note's envelopes, or nothing while no note plays. */
    std::optional<std::array<float, numOperators>> getShownEnvelopes (const BuiltInFm& p)
    {
        if (p.noteAge.load() < 0.0f)
            return std::nullopt;

        std::array<float, numOperators> envelopes;

        for (size_t i = 0; i < envelopes.size(); ++i)
            envelopes[i] = p.shownEnvelopes[i].load();

        return envelopes;
    }

    std::array<float, numOperators> getEnvelopesOrFull (const BuiltInFm& p)
    {
        if (auto shown = getShownEnvelopes (p))
            return *shown;

        std::array<float, numOperators> full;
        full.fill (1.0f);
        return full;
    }
}

//==============================================================================
FmOperatorsView::FmOperatorsView (BuiltInFm& p) : processor (p)
{
    setTopic (*this, fmtext::operators);
}

void FmOperatorsView::setSelected (int op)
{
    selected = op;
    repaint();
}

juce::Rectangle<float> FmOperatorsView::getPlot() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 8.0f).withTrimmedTop (22.0f);
}

void FmOperatorsView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    const auto s = processor.getSettings();

    if (hovered >= 0)
        drawHeading (g, r, fmtext::describeOperator (s, hovered).toUpperCase());
    else
        drawHeading (g, r, "THE OPERATORS: " + fm::getAlgorithm (s.algorithm).name.toUpperCase() + ". CLICK ONE, DRAG FOR ITS LEVEL");

    const auto envelopes = getShownEnvelopes (processor);
    fmtext::drawAlgorithm (g, getPlot(), s, selected, envelopes ? &*envelopes : nullptr, true);
}

void FmOperatorsView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    const int op = fmtext::getOperatorAt (getPlot(), processor.getSettings().algorithm, e.position);

    if (op < 0)
        return;

    if (onSelect)
        onSelect (op);

    dragging = op;
    dragFrom = processor.getSettings().ops[(size_t) op].levelPercent;

    if (auto* p = processor.getKnobParameter (Knob::level, op))
        p->beginChangeGesture();
}

void FmOperatorsView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0)
        return;

    // Half a percent a pixel: 200 pixels from off to full. Alt for fine steps.
    const float perPixel = e.mods.isAltDown() ? 0.1f : 0.5f;
    processor.setKnob (Knob::level, juce::jlimit (0.0f, 100.0f, dragFrom - (float) e.getDistanceFromDragStartY() * perPixel), dragging);
    repaint();
}

void FmOperatorsView::mouseUp (const juce::MouseEvent&)
{
    if (dragging >= 0)
        if (auto* p = processor.getKnobParameter (Knob::level, dragging))
            p->endChangeGesture();

    dragging = -1;
}

void FmOperatorsView::mouseMove (const juce::MouseEvent& e)
{
    const int op = fmtext::getOperatorAt (getPlot(), processor.getSettings().algorithm, e.position);
    setMouseCursor (op >= 0 ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);

    if (op != hovered)
    {
        hovered = op;
        repaint();
    }
}

void FmOperatorsView::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void FmOperatorsView::showMenu()
{
    juce::PopupMenu menu;
    const int current = processor.getSettings().algorithm;

    for (int a = 0; a < fm::numAlgorithms; ++a)
    {
        const auto& algorithm = fm::getAlgorithm (a);
        menu.addItem (juce::String (a + 1) + ". " + algorithm.name + ": " + algorithm.wiring, true, a == current,
                      [this, a] { processor.setKnob (Knob::algorithm, (float) a); repaint(); });
    }

    menu.addSeparator();
    guideicon::addItem (menu, "What is FM synthesis?", [this] { if (onOpenGuide) onOpenGuide(); });
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
}

//==============================================================================
/** The partials of the test note now, measured from the sound. */
class BuiltInFmEditor::Spectrum final : public juce::Component
{
public:
    explicit Spectrum (BuiltInFm& p) : processor (p)
    {
        setTopic (*this, fmtext::spectrum);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        const auto s = processor.getSettings();
        const float velocity = processor.noteAge.load() >= 0.0f ? processor.shownVelocity.load() : 1.0f;
        const auto peaks = fm::getSpectrum (s, sub::testNoteHz, getEnvelopesOrFull (processor), velocity, sub::testNoteHz * maxHarmonic);
        int heard = 0;
        double highest = 0.0;

        for (auto& p : peaks)
        {
            if (p.amplitude > 0.001)
            {
                ++heard;
                highest = juce::jmax (highest, p.hz);
            }
        }

        drawHeading (g, r, heard == 0 ? juce::String ("THE PARTIALS OF A3: NONE, AS EVERY CARRIER IS AT 0")
                                      : "THE PARTIALS OF A3" + juce::String (processor.noteAge.load() >= 0.0f ? " NOW" : "") + ": " + juce::String (heard)
                                            + " LOUDER THAN -60 dB, UP TO " + formatHz (highest).toUpperCase() + ". NUMBERS: HARMONICS");
        r.removeFromTop (4.0f);
        fmtext::drawSpectrum (g, r, peaks, sub::testNoteHz, maxHarmonic, &s);
    }

    static constexpr double maxHarmonic = 24.0;

private:
    BuiltInFm& processor;
};

//==============================================================================
/** A few cycles of the test note: the sound, and the carriers unmodulated. */
class BuiltInFmEditor::Wave final : public juce::Component
{
public:
    explicit Wave (BuiltInFm& p) : processor (p)
    {
        setTopic (*this, fmtext::wave);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
        drawHeading (g, r, "THE WAVE, AND THE CARRIERS UNMODULATED");
        fmtext::drawWave (g, r, processor.getSettings(), sub::testNoteHz, getEnvelopesOrFull (processor), 2.0);
    }

private:
    BuiltInFm& processor;
};

//==============================================================================
BuiltInFmEditor::BuiltInFmEditor (BuiltInFm& p)
    : EffectEditor (p, "DawIt FM", "What is FM synthesis?", "Hear the carriers alone", p.withoutModulators, p.clap, fmtext::colour,
                    fx::Clap::Sound::note),
      synth (p),
      history (p.history, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "the whole sound", fmtext::colour, "the carriers, unmodulated", {})
{
    spectrumView = std::make_unique<Spectrum> (synth);
    wave = std::make_unique<Wave> (synth);
    operatorsView.onSelect = [this] (int op) { select (op); };
    operatorsView.onOpenGuide = [this] { guideButton.triggerClick(); };
    addAndMakeVisible (operatorsView);
    addAndMakeVisible (*spectrumView);
    addAndMakeVisible (*wave);
    addAndMakeVisible (history);

    for (int k = 0; k < fm::numKnobs; ++k)
    {
        const auto knob = (Knob) k;

        switch (knob)
        {
            case Knob::algorithm:
            case Knob::lfoTarget:
            case Knob::mode:
                addSwitch (knob, fm::getKnobName (knob));
                break;

            case Knob::coarse:
            case Knob::fine:
            case Knob::level:
            case Knob::attack:
            case Knob::decay:
            case Knob::sustain:
            case Knob::release:
            case Knob::feedback:
            case Knob::lfoRate:
            case Knob::lfoDepth:
            case Knob::glide:
            case Knob::velocity:
            case Knob::output:
            {
                const auto c = knob == Knob::feedback ? fmtext::getOperatorColour (3)
                             : knob == Knob::lfoRate || knob == Knob::lfoDepth ? fmtext::lfoColour
                             : fmtext::colour;
                knobs[knob] = std::make_unique<EffectKnob> (synth.getParameters(), BuiltInFm::getParameterId (knob, 0), fm::getKnobName (knob), k, c);
                break;
            }
        }
    }

    operatorSwitch = std::make_unique<Switch> ("Operator", fmtext::selectOperator);
    operatorSwitch->button.onClick = [this] { select ((selected + 1) % numOperators); };

    switches.at (Knob::algorithm)->button.onClick = [this]
    {
        juce::PopupMenu menu;
        const int current = synth.getSettings().algorithm;

        for (int a = 0; a < fm::numAlgorithms; ++a)
        {
            const auto& algorithm = fm::getAlgorithm (a);
            menu.addItem (juce::String (a + 1) + ". " + algorithm.name + ": " + algorithm.wiring, true, a == current,
                          [this, a] { synth.setKnob (Knob::algorithm, (float) a); });
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&switches.at (Knob::algorithm)->button));
    };

    juce::StringArray presetNames;

    for (auto& preset : fm::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    select (selected);
    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 780);
}

BuiltInFmEditor::Switch& BuiltInFmEditor::addSwitch (Knob k, const juce::String& name)
{
    auto& s = *(switches[k] = std::make_unique<Switch> (name, (int) k));
    s.button.setColour (juce::TextButton::buttonColourId, fmtext::colour.withAlpha (0.25f));
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

void BuiltInFmEditor::select (int op)
{
    selected = juce::jlimit (0, numOperators - 1, op);
    const auto c = fmtext::getOperatorColour (selected);

    for (int k = 0; k < fm::numOperatorKnobs; ++k)
    {
        auto& knob = *knobs.at ((Knob) k);
        knob.attachTo (BuiltInFm::getParameterId ((Knob) k, selected));
        knob.slider.setColour (juce::Slider::rotarySliderFillColourId, c);
        knob.repaint();
    }

    operatorSwitch->button.setButtonText (juce::String (selected + 1));
    operatorSwitch->button.setColour (juce::TextButton::buttonColourId, c.withAlpha (0.35f));
    operatorsView.setSelected (selected);
    updateGroups();

    if ((getTopic() >= 0 && getTopic() < fm::numOperatorKnobs) || getTopic() == fmtext::selectOperator)
        showTopic (getTopic());
}

void BuiltInFmEditor::updateGroups()
{
    auto knob = [this] (Knob k) -> juce::Component* { return knobs.at (k).get(); };
    auto toggle = [this] (Knob k) -> juce::Component* { return switches.at (k).get(); };
    const auto s = synth.getSettings();
    shownAsCarrier = fm::isCarrier (s, selected);
    const juce::String name = "OPERATOR " + juce::String (selected + 1) + (shownAsCarrier ? ": A CARRIER" : ": A MODULATOR");

    setGroups ({ { name, { operatorSwitch.get(), knob (Knob::coarse), knob (Knob::fine), knob (Knob::level) }, 0 },
                 { "ITS ENVELOPE", { knob (Knob::attack), knob (Knob::decay), knob (Knob::sustain), knob (Knob::release) }, 0 },
                 { "WIRING", { toggle (Knob::algorithm), knob (Knob::feedback) }, 0 },
                 { "LFO", { knob (Knob::lfoRate), knob (Knob::lfoDepth), toggle (Knob::lfoTarget) }, 1 },
                 { "PLAYING", { knob (Knob::glide), toggle (Knob::mode), knob (Knob::velocity), knob (Knob::output) }, 1 } });
    resized();
    repaint();
}

void BuiltInFmEditor::applyPreset (int index)
{
    const auto& list = fm::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    synth.setSettings (list[(size_t) index].settings);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    updateGroups();
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInFmEditor::createGuidePages()
{
    return fmtext::makeGuide (synth);
}

void BuiltInFmEditor::layoutPictures (juce::Rectangle<int> r)
{
    operatorsView.setBounds (r.removeFromLeft (r.getWidth() * 38 / 100));
    r.removeFromLeft (10);
    spectrumView->setBounds (r.removeFromTop ((r.getHeight() - 10) * 55 / 100));
    r.removeFromTop (10);
    wave->setBounds (r.removeFromLeft ((r.getWidth() - 10) / 2));
    r.removeFromLeft (10);
    history.setBounds (r);
}

void BuiltInFmEditor::tick()
{
    const auto s = synth.getSettings();

    for (auto& [k, sw] : switches)
        sw->button.setButtonText (fm::formatKnob (k, s));

    // The operator group says whether it's a carrier or a modulator, which changes with the algorithm.
    const bool carrier = fm::isCarrier (s, selected);

    if (carrier != shownAsCarrier)
    {
        shownAsCarrier = carrier;
        updateGroups();
    }

    history.update();
    operatorsView.repaint();
    spectrumView->repaint();
    wave->repaint();
}

juce::AttributedString BuiltInFmEditor::buildText (int topic)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = synth.getSettings();
    const auto& list = fm::getPresets();
    const fm::Preset* preset = juce::isPositiveAndBelow (presetIndex, (int) list.size()) ? &list[(size_t) presetIndex] : nullptr;
    const juce::String who = "Operator " + juce::String (selected + 1);

    auto addOperatorNotes = [&] (int op)
    {
        if (preset != nullptr && preset->operators[(size_t) op].isNotEmpty())
            addSection (text, "In " + preset->name + ": operator " + juce::String (op + 1), preset->operators[(size_t) op]);
    };

    if (topic == CommonTopic::presets && preset != nullptr)
    {
        addTitle (text, preset->name, fmtext::colour);
        addLead (text, preset->why);
        addSection (text, {}, "Play a note, then change one knob at a time and listen. The knobs not named here are as in Init.");
        addSection (text, "Algorithm: " + fm::formatKnob (Knob::algorithm, preset->settings), fm::getAlgorithm (preset->settings.algorithm).wiring);

        for (int op = 0; op < numOperators; ++op)
            if (preset->operators[(size_t) op].isNotEmpty())
                addSection (text, fmtext::describeOperator (preset->settings, op), preset->operators[(size_t) op]);

        for (int k = fm::numOperatorKnobs; k < fm::numKnobs; ++k)
            if (preset->notes[(size_t) k].isNotEmpty() && (Knob) k != Knob::algorithm)
                addSection (text, fmtext::getTitle (k) + ": " + fm::formatKnob ((Knob) k, preset->settings), preset->notes[(size_t) k]);

        return text;
    }

    if (topic >= 0 && topic < fm::numKnobs)
    {
        const bool ofOperator = fm::isOperatorKnob ((Knob) topic);
        addTitle (text, (ofOperator ? who + ": " : juce::String()) + fmtext::getTitle (topic), ofOperator ? fmtext::getOperatorColour (selected) : fmtext::colour);
        addLead (text, fmtext::describeSettings (topic, s, selected));
        addSection (text, "What it does", fmtext::describe (topic));
        addSection (text, "How it's usually set", fmtext::getTypical (topic));

        if (ofOperator)
            addOperatorNotes (selected);
        else if (preset != nullptr && preset->notes[(size_t) topic].isNotEmpty())
            addSection (text, "In " + preset->name + ": " + fm::formatKnob ((Knob) topic, preset->settings), preset->notes[(size_t) topic]);

        if (preset == nullptr)
            addSection (text, {}, "Pick a starting point to read why each operator is set the way it is for that sound.");

        return text;
    }

    addTitle (text, fmtext::getTitle (topic));

    switch (topic)
    {
        case CommonTopic::overview:
            addLead (text, "An FM synthesiser: each note is made of four sines, the operators, and some of them bend others at audio "
                           "speed. Bending a sine that fast doesn't make a wobble, it makes new partials, so two sines can sound as rich "
                           "as a saw or as clangy as a bell. It's how the Yamaha DX7 made the electric pianos, basses and bells of the 1980s.");
            addSection (text, "How the sound is made", "The algorithm wires the operators: the modulators (above) bend the carriers "
                                                       "(below), and the carriers are heard. Each operator has a ratio to the note, a level "
                                                       "and an envelope; a modulator's level is how bright it makes the sound, so its "
                                                       "envelope moves the brightness through the note. Operator 4 can modulate itself "
                                                       "(feedback). The LFO moves the pitch, the brightness or the volume.");
            addSection (text, "Reading the window", "Left, the operators: click one to show its knobs below, drag up or down on it for its "
                                                    "level, right-click for the algorithm. Right, the partials of the note as they are now, "
                                                    "the wave, and the last 4 seconds. Hover anything to read about it here.");
            addSection (text, "Getting started", "Pick a starting point; each tells what every operator does. Click Play a note, or play "
                                                 "the track's notes or a MIDI keyboard. Click Hear the carriers alone to hear what the "
                                                 "modulation adds. Read What is FM synthesis? for how it works and how to build a sound.");
            break;

        case CommonTopic::listen:
            addLead (text, synth.withoutModulators.load() ? "On: you hear the carriers as plain sines." : "Off: you hear the whole sound.");
            addSection (text, {}, fmtext::describe (topic));
            break;

        case CommonTopic::history:
        {
            const auto [before, after] = history.getLoudest();

            if (after > -70.0f)
                addLead (text, "The last 4 seconds: the carriers up to " + juce::String (before, 1) + " dB unmodulated, the whole sound up to "
                                   + juce::String (after, 1) + " dB.");
            else
                addLead (text, "Play a note to see it here.");

            addSection (text, {}, fmtext::describe (topic));
            break;
        }

        case fmtext::operators:
        {
            juce::StringArray lines;

            for (int op = 0; op < numOperators; ++op)
                lines.add (fmtext::describeOperator (s, op) + ".");

            addLead (text, fm::getAlgorithm (s.algorithm).name + ": " + fm::getAlgorithm (s.algorithm).wiring + ".");
            addSection (text, {}, lines.joinIntoString ("\n"));
            addSection (text, {}, fmtext::describe (topic));

            for (int op = 0; op < numOperators; ++op)
                addOperatorNotes (op);

            break;
        }

        case fmtext::spectrum:
        {
            const auto peaks = fm::getSpectrum (s, sub::testNoteHz, getEnvelopesOrFull (synth), 1.0f, sub::testNoteHz * Spectrum::maxHarmonic);
            int harmonic = 0, between = 0;

            for (auto& p : peaks)
            {
                if (p.amplitude < 0.001)
                    continue;

                const double n = p.hz / sub::testNoteHz;
                (std::abs (n - std::round (n)) < 0.02 ? harmonic : between)++;
            }

            addLead (text, juce::String (harmonic) + (harmonic == 1 ? " partial is a harmonic" : " partials are harmonics") + " of A3"
                               + (between > 0 ? ", " + juce::String (between) + " fall between them: it rings like metal." : ": it has a clear pitch."));
            addSection (text, {}, fmtext::describe (topic));
            break;
        }

        case fmtext::wave:
            addLead (text, "A3 peaks at about " + juce::String (fmtext::getPeakDb (s), 1) + " dB at full velocity.");
            addSection (text, {}, fmtext::describe (topic));
            break;

        case fmtext::selectOperator:
            addLead (text, fmtext::describeOperator (s, selected) + ".");
            addSection (text, {}, fmtext::describe (topic));
            addOperatorNotes (selected);
            break;

        default:
            addSection (text, {}, fmtext::describe (topic));
            break;
    }

    return text;
}
} // namespace daw
