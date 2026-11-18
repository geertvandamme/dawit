#include "VocoderEditor.h"

#include "GuideButton.h"
#include "VocoderGuide.h"
#include "generate/VocoderGuideText.h"

namespace daw
{
using namespace fxui;
namespace vg = gen::vocoderguide;

namespace
{
    constexpr int topicBands = 50, topicKeyboard = 51;
    constexpr float lowestDb = -60.0f;

    float toDb (float level) { return juce::Decibels::gainToDecibels (level, -100.0f); }

    vg::HostKind getHostKind (const HostTrack& host)
    {
        return host.isReturn ? vg::HostKind::returnTrack : host.isBus ? vg::HostKind::bus : host.isInstrument ? vg::HostKind::instrument : vg::HostKind::track;
    }

    /** The settings with one knob's slider value put in, to write that value as the knob does. */
    vocoder::Settings settingsWith (vocoder::Knob knob, double v)
    {
        using vocoder::Knob;
        vocoder::Settings s;
        const int index = juce::roundToInt (v);

        switch (knob)
        {
            case Knob::bands:      s.bands = vocoder::bandCounts[(size_t) juce::jlimit (0, 4, index)]; break;
            case Knob::smoothing:  s.smoothingMs = (float) v; break;
            case Knob::consonants: s.consonantsPercent = (float) v; break;
            case Knob::voice:      s.voiceDb = (float) v; break;
            case Knob::low:        s.lowHz = (float) v; break;
            case Knob::high:       s.highHz = (float) v; break;
            case Knob::source:     s.source = (vocoder::Source) juce::jlimit (0, 2, index); break;
            case Knob::wave:       s.wave = (vocoder::Wave) juce::jlimit (0, 3, index); break;
            case Knob::root:       s.root = juce::jlimit (0, 11, index); break;
            case Knob::shape:      s.shape = (vocoder::Shape) juce::jlimit (0, 3, index); break;
            case Knob::range:      s.range = juce::jlimit (1, 3, index); break;
            case Knob::carrier:    s.carrierDb = (float) v; break;
            case Knob::formant:    s.formantSemitones = (float) v; break;
            case Knob::mix:        s.mixPercent = (float) v; break;
        }

        return s;
    }

    bool isChoiceKnob (vocoder::Knob k)
    {
        using vocoder::Knob;
        return k == Knob::bands || k == Knob::source || k == Knob::wave || k == Knob::root || k == Knob::shape || k == Knob::range;
    }

    double getSampleRateOf (const BuiltInVocoder& p)
    {
        return p.getSampleRate() > 0.0 ? p.getSampleRate() : 48000.0;
    }
}

juce::String getVocoderLeadLine (const BuiltInVocoder& p)
{
    const auto host = getHostTrack (p);
    return vg::getLeadLine (p.getSettings().source, getHostKind (host), host.armedElsewhere, p.isUsingFallback());
}

//==============================================================================
VocoderBandsView::VocoderBandsView (BuiltInVocoder& p, std::function<void (int)> open, std::function<void (int)> hover)
    : processor (p), guide (std::move (open)), hoverTopic (std::move (hover))
{
    setTopic (*this, topicBands);
}

juce::Rectangle<float> VocoderBandsView::getBarArea() const
{
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    r.removeFromTop (18.0f);
    r.removeFromBottom (16.0f);
    return r.withTrimmedLeft (58.0f);
}

std::vector<double> VocoderBandsView::getEdges() const
{
    return vocoder::getBandEdges (processor.getBandsInForce(), processor.getLowInForce(), processor.getHighInForce(), getSampleRateOf (processor));
}

int VocoderBandsView::getBandAt (juce::Point<float> p) const
{
    const auto area = getBarArea();
    const int n = processor.getBandsInForce();

    if (! area.contains (p) || n <= 0)
        return -1;

    return juce::jlimit (0, n - 1, (int) ((p.x - area.getX()) / area.getWidth() * (float) n));
}

void VocoderBandsView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto inner = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    auto header = inner.removeFromTop (16.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("THE VOICE, THE CARRIER AND THE RESULT, BAND BY BAND", header, juce::Justification::centredLeft);

    const int n = juce::jlimit (1, vocoder::maxBands, processor.getBandsInForce());
    const int only = processor.getHearOnlyBand();

    if (only >= 0)
    {
        g.setColour (vocoderui::colour);
        g.drawText ("HEARING BAND " + juce::String (only + 1) + " ONLY: CLICK IT AGAIN FOR ALL", header, juce::Justification::centredRight);
    }

    auto axis = inner.removeFromBottom (16.0f).withTrimmedLeft (58.0f);
    const auto bars = getBarArea();
    const float rowGap = 6.0f, rowHeight = (bars.getHeight() - rowGap * 2.0f) / 3.0f;
    const float cell = bars.getWidth() / (float) n;

    struct Strip { const char* name; juce::Colour colour; float (BuiltInVocoder::*level) (int) const noexcept; };
    const Strip strips[] = { { "Voice", theme::text.withAlpha (0.85f), &BuiltInVocoder::getVoiceLevel },
                             { "Carrier", theme::accent, &BuiltInVocoder::getCarrierLevel },
                             { "Result", vocoderui::colour, &BuiltInVocoder::getResultLevel } };

    for (int s = 0; s < 3; ++s)
    {
        const auto row = juce::Rectangle<float> (bars.getX(), bars.getY() + (float) s * (rowHeight + rowGap), bars.getWidth(), rowHeight);
        g.setColour (theme::background.withAlpha (0.6f));
        g.fillRect (row);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (strips[s].name, juce::Rectangle<float> (inner.getX(), row.getY(), 56.0f, row.getHeight()), juce::Justification::centredLeft);

        for (float db = -12.0f; db > lowestDb; db -= 24.0f)
        {
            g.setColour (theme::grid);
            g.fillRect (row.getX(), row.getBottom() - (db - lowestDb) / -lowestDb * row.getHeight(), row.getWidth(), 1.0f);
        }

        for (int b = 0; b < n; ++b)
        {
            const float db = juce::jlimit (lowestDb, 0.0f, toDb ((processor.*strips[s].level) (b)));
            const float height = (db - lowestDb) / -lowestDb * row.getHeight();
            const bool dim = only >= 0 && only != b;
            g.setColour (strips[s].colour.withAlpha (dim ? 0.25f : 1.0f));
            g.fillRect (row.getX() + cell * (float) b + 1.0f, row.getBottom() - height, juce::jmax (1.0f, cell - 2.0f), height);

            if (b == hovered || b == only)
            {
                g.setColour (theme::text.withAlpha (b == only ? 0.9f : 0.5f));
                g.drawRect (juce::Rectangle<float> (row.getX() + cell * (float) b, row.getY(), cell, row.getHeight()), 1.0f);
            }
        }
    }

    // The edges of the range, and a mark every octave that falls inside it.
    const auto edges = getEdges();
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (theme::textDim);
    g.drawText (vg::formatHz (edges.front()), axis.removeFromLeft (80.0f), juce::Justification::centredLeft);
    g.drawText (vg::formatHz (edges.back()), axis.removeFromRight (80.0f), juce::Justification::centredRight);
    g.drawText ("log spaced: " + juce::String (n) + " bands, each " + juce::String (vocoder::getBandWidthSemitones (n, processor.getLowInForce(), processor.getHighInForce(), getSampleRateOf (processor)), 1) + " semitones wide",
                axis, juce::Justification::centred);
}

void VocoderBandsView::mouseMove (const juce::MouseEvent& e)
{
    const int band = getBandAt (e.position);

    if (band != hovered)
    {
        hovered = band;
        setMouseCursor (band >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }

    if (hoverTopic != nullptr)
        hoverTopic (topicBands);
}

void VocoderBandsView::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void VocoderBandsView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Hear all bands", processor.getHearOnlyBand() >= 0);
        menu.addSeparator();
        guideicon::addItem (menu, 2, "What is a vocoder?");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<VocoderBandsView> (this)] (int choice)
        {
            if (safe == nullptr)
                return;

            if (choice == 1)
                safe->processor.setHearOnlyBand (-1);
            else if (choice == 2 && safe->guide != nullptr)
                safe->guide (0);

            safe->repaint();
        });
        return;
    }

    const int band = getBandAt (e.position);

    if (band >= 0)
    {
        processor.setHearOnlyBand (processor.getHearOnlyBand() == band ? -1 : band);
        repaint();
    }
}

juce::String VocoderBandsView::getTooltip()
{
    const int band = getBandAt (getMouseXYRelative().toFloat());

    if (band < 0)
        return "The voice, the carrier and the result in each band, from -60 to 0 dB. Click a band to hear only that band; right-click for more.";

    const auto edges = getEdges();

    if (band + 1 >= (int) edges.size())
        return {};

    return vg::describeBand (band, edges[(size_t) band], edges[(size_t) band + 1], toDb (processor.getVoiceLevel (band)), toDb (processor.getCarrierLevel (band)))
           + ". Click to hear only this band.";
}

//==============================================================================
VocoderKeyboardView::VocoderKeyboardView (BuiltInVocoder& p, std::function<void (int)> hover) : processor (p), hoverTopic (std::move (hover))
{
    setTopic (*this, topicKeyboard);
}

void VocoderKeyboardView::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("WHAT THE CARRIER PLAYS", r.removeFromTop (16.0f), juce::Justification::centredLeft);

    const auto settings = processor.getSettings();
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (vg::getCarrierLabel (settings.source, processor.isUsingFallback(), processor.getCarrierPitchMask(), processor.getSongChordMask()),
                      r.removeFromTop (22.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    r.removeFromTop (4.0f);

    // One octave: white keys C to B, black keys over them. A lit key is a note the carrier plays; a tinted one is in the song's key.
    const auto keys = r.removeFromTop (juce::jmin (84.0f, r.getHeight() * 0.5f));
    const int mask = processor.getCarrierPitchMask(), keyMask = processor.getSongKeyMask();
    constexpr int whitePcs[] = { 0, 2, 4, 5, 7, 9, 11 };
    const float whiteWidth = keys.getWidth() / 7.0f;

    for (int i = 0; i < 7; ++i)
    {
        const int pc = whitePcs[i];
        const auto key = juce::Rectangle<float> (keys.getX() + whiteWidth * (float) i, keys.getY(), whiteWidth, keys.getHeight()).reduced (0.5f, 0.0f);
        g.setColour (theme::text.withAlpha (0.88f));
        g.fillRect (key);

        if ((keyMask & (1 << pc)) != 0)
        {
            g.setColour (vocoderui::colour.withAlpha (0.18f));
            g.fillRect (key);
        }

        if ((mask & (1 << pc)) != 0)
        {
            g.setColour (vocoderui::colour);
            g.fillRect (key.withTrimmedTop (key.getHeight() * 0.55f).reduced (3.0f, 3.0f));
        }
    }

    constexpr int blackPcs[] = { 1, 3, -1, 6, 8, 10 };

    for (int i = 0; i < 6; ++i)
    {
        if (blackPcs[i] < 0)
            continue;

        const auto key = juce::Rectangle<float> (keys.getX() + whiteWidth * ((float) i + 0.68f), keys.getY(), whiteWidth * 0.64f, keys.getHeight() * 0.6f);
        g.setColour ((mask & (1 << blackPcs[i])) != 0 ? vocoderui::colour : theme::background);
        g.fillRect (key);
    }

    r.removeFromTop (8.0f);
    const auto line = getVocoderLeadLine (processor);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText (line.isNotEmpty() ? line : juce::String ("The notes follow what you chose in Source."), r.toNearestInt(), juce::Justification::topLeft, 6);
}

void VocoderKeyboardView::mouseMove (const juce::MouseEvent&)
{
    if (hoverTopic != nullptr)
        hoverTopic (topicKeyboard);
}

juce::String VocoderKeyboardView::getTooltip()
{
    return "The notes the carrier plays now, lit in pink. The notes of the song's key are tinted.";
}

//==============================================================================
std::atomic<bool>& BuiltInVocoderEditor::unusedListen()
{
    static std::atomic<bool> unused { false };
    return unused;
}

fx::Clap& BuiltInVocoderEditor::unusedClap()
{
    static fx::Clap unused;   // the test voice goes through the processor itself, not through a clap
    return unused;
}

BuiltInVocoderEditor::BuiltInVocoderEditor (BuiltInVocoder& p)
    : EffectEditor (p, "DawIt Vocoder", "What is a vocoder?", {}, unusedListen(), unusedClap(), vocoderui::colour, fx::Clap::Sound::sungLine),
      vocoderProcessor (p),
      bands (p, [this] (int page) { openGuide (page); }, [this] (int topicIndex) { hover (topicIndex); }),
      keyboard (p, [this] (int topicIndex) { hover (topicIndex); }),
      history (p.levelHistory, [&p] { return p.getSampleRate(); }, [] { return 0.0; }, "DawIt Vocoder", vocoderui::colour, "your voice", ", as loud as it is mixed")
{
    addAndMakeVisible (bands);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (history);
    listenButton.setVisible (false);

    using vocoder::Knob;
    const char* const labels[] = { "Bands", "Smoothing", "Consonants", "Voice", "Low", "High", "Source", "Wave", "Root", "Shape", "Range", "Carrier", "Formant", "Mix" };

    for (int k = 0; k < vocoder::numKnobs; ++k)
    {
        const auto knob = (Knob) k;
        auto* k0 = knobs.add (new EffectKnob (p.getParameters(), BuiltInVocoder::getParameterId (knob), labels[k], k, vocoderui::colour));
        k0->slider.setTooltip (vg::getControlText ((vg::Control) k));
        k0->label.setTooltip (vg::getControlText ((vg::Control) k));
        k0->slider.textFromValueFunction = [knob] (double v) { return vg::formatKnob ((vg::Control) (int) knob, settingsWith (knob, v)); };

        if (isChoiceKnob (knob))
            k0->slider.setTextBoxIsEditable (false);

        k0->slider.updateText();
    }

    setGroups ({ { "VOICE", { knobs[0], knobs[1], knobs[2], knobs[3], knobs[4], knobs[5] }, 0 },
                 { "OUTPUT", { knobs[12], knobs[13] }, 0 },
                 { "CARRIER", { knobs[6], knobs[7], knobs[8], knobs[9], knobs[10], knobs[11] }, 1 } });

    juce::StringArray presetNames;

    for (const auto& preset : vocoder::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);

    clapButton.setButtonText ("Say \"ah ee s\"");
    clapButton.setTooltip ("Plays a made-up voice saying \"ah\", \"ee\", \"oo\" and \"s\" through the vocoder, with the settings in this window.");
    clapButton.onClick = [this]
    {
        vocoderProcessor.playTestVoice();
        showTopic (CommonTopic::clap);
    };

    tick();
    showTopic (CommonTopic::overview);
    setSize (1400, 800);
}

void BuiltInVocoderEditor::applyPreset (int index)
{
    const auto& list = vocoder::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    vocoderProcessor.applyPreset (list[(size_t) index]);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInVocoderEditor::createGuidePages()
{
    return voctext::makeGuide (vocoderProcessor);
}

void BuiltInVocoderEditor::layoutPictures (juce::Rectangle<int> r)
{
    r.reduce (10, 10);
    auto lower = r.removeFromBottom (juce::jmin (150, r.getHeight() / 3));
    r.removeFromBottom (10);
    keyboard.setBounds (r.removeFromRight (juce::jmin (260, r.getWidth() / 3)));
    r.removeFromRight (10);
    bands.setBounds (r);
    history.setBounds (lower);
}

void BuiltInVocoderEditor::tick()
{
    history.update();
    bands.repaint();
    keyboard.repaint();
}

juce::AttributedString BuiltInVocoderEditor::buildText (int topicIndex)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);
    const auto s = vocoderProcessor.getSettings();
    const double rate = getSampleRateOf (vocoderProcessor);

    if (juce::isPositiveAndBelow (topicIndex, vocoder::numKnobs))
    {
        const auto control = (vg::Control) topicIndex;
        addTitle (text, vg::getControlTitle (control), colour);

        juce::String lead = vg::formatKnob (control, s);

        if (control == vg::Control::bands || control == vg::Control::low || control == vg::Control::high)
            lead = juce::String (vocoderProcessor.getBandsInForce()) + " bands from " + vg::formatHz (vocoderProcessor.getLowInForce()) + " to " + vg::formatHz (vocoderProcessor.getHighInForce())
                   + ", each " + juce::String (vocoder::getBandWidthSemitones (vocoderProcessor.getBandsInForce(), vocoderProcessor.getLowInForce(), vocoderProcessor.getHighInForce(), rate), 1) + " semitones wide.";
        else if (control == vg::Control::source)
            lead = lead + ". " + getVocoderLeadLine (vocoderProcessor);

        addLead (text, lead.trim());
        addSection (text, {}, vg::getControlText (control));
        return text;
    }

    if (topicIndex == CommonTopic::presets && juce::isPositiveAndBelow (presetIndex, (int) vocoder::getPresets().size()))
    {
        const auto& preset = vocoder::getPresets()[(size_t) presetIndex];
        addTitle (text, preset.name, colour);
        addLead (text, preset.why);
        addSection (text, {}, "A start, not an answer: say something into the track, or press Say \"ah ee s\", and adjust by ear.");
        return text;
    }

    switch (topicIndex)
    {
        case CommonTopic::clap:
            addTitle (text, "Say \"ah ee s\"", colour);
            addLead (text, "A made-up voice says \"ah\", \"ee\", \"oo\" and \"s\" through the vocoder, with the settings in this window, even while the song is stopped.");
            addSection (text, {}, "It is added to what the track plays, so mute the track's own voice to hear it alone.");
            break;

        case CommonTopic::history:
            addTitle (text, "Level", colour);
            addLead (text, "How loud the voice was (grey) and how loud the result is (pink) over the last 4 seconds.");
            addSection (text, {}, "The result is usually quieter than the voice: raise Carrier, not the master.");
            break;

        case topicBands:
        {
            const int hear = vocoderProcessor.getHearOnlyBand();
            addTitle (text, "The bands", colour);
            addLead (text, hear >= 0 ? "Hearing band " + juce::String (hear + 1) + " only." : juce::String ("Hearing all bands."));
            addSection (text, {}, "Three strips across the same bands, from low to high. The top strip is what each band hears of your voice; the middle one is "
                                  "what the carrier holds there; the bottom one is the result, the carrier turned up and down by the voice. Hover a band to read its range and "
                                  "levels. Click it to hear only that band, and click it again to hear them all.");
            break;
        }

        case topicKeyboard:
        {
            addTitle (text, "What the carrier plays", colour);
            addLead (text, vg::getCarrierLabel (s.source, vocoderProcessor.isUsingFallback(), vocoderProcessor.getCarrierPitchMask(), vocoderProcessor.getSongChordMask()) + ".");
            const auto line = getVocoderLeadLine (vocoderProcessor);

            if (line.isNotEmpty())
                addSection (text, {}, line);

            addSection (text, {}, "One octave of keys: the notes the carrier plays are lit in pink and the notes of the song's key are tinted.");
            break;
        }

        default:
        {
            addTitle (text, "DawIt Vocoder", colour);
            const auto line = getVocoderLeadLine (vocoderProcessor);
            addLead (text, "Your voice shapes the colour of a synth chord: the track it sits on is the voice, and the effect plays the notes.");

            if (line.isNotEmpty())
                addSection (text, {}, line);

            addSection (text, "How it works", "The voice is cut into bands. Each band's loudness turns the same band of the carrier, a saw, pulse, choir or noise sound "
                                              "that plays the song's chord, notes you pick or keys you play, up and down.");
            addSection (text, "Getting started", "Pick a starting point, then talk or sing, or press Say \"ah ee s\". Read What is a vocoder? for how it works.");
            addSection (text, "Reading the window", "Left, the voice, the carrier and the result band by band, and the notes the carrier plays. Below, the level of the last "
                                                    "4 seconds. Hover anything to read about it here.");
            addPluginParagraph (text, vocoderProcessor);
            break;
        }
    }

    return text;
}
} // namespace daw
