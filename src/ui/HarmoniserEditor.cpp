#include "GuideButton.h"
#include "HarmoniserEditor.h"
#include "TuneGuide.h"
#include "generate/HarmoniserGuideText.h"

namespace daw
{
using namespace fxui;

namespace
{
    using gen::harmoniserguide::Control;

    constexpr int topicNow = 50, topicTrace = 51, topicLevel = 52;

    juce::String dot() { return juce::String::fromUTF8 (" \xc2\xb7 "); }
    juce::String dash() { return juce::String::fromUTF8 ("\xe2\x80\x93"); }
    juce::String arrow() { return juce::String::fromUTF8 (" \xe2\x86\x92 "); }

    bool isBlack (int midi) { const int pc = ((midi % 12) + 12) % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
    bool inKey (int midi, int mask) { return (mask & (1 << (((midi % 12) + 12) % 12))) != 0; }

    /** "minor 3rd up", "unison": the size of the step from the lead's note to a voice's, in whole semitones. */
    juce::String getStepName (int semitones)
    {
        return semitones == 0 ? juce::String ("unison") : harm::getSizeName (semitones) + (semitones > 0 ? " up" : " down");
    }

    /** The lead's note and a voice's, as the now column and the hover write them. */
    juce::String describeVoice (float lead, float voice, harm::Mode mode, const harm::Voice& settings)
    {
        const int semitones = juce::roundToInt (voice - lead);
        auto text = tunepics::getNoteName (juce::roundToInt (voice)) + "  " + getStepName (semitones) + dot();

        if (mode == harm::Mode::key)
            text << (std::abs (settings.steps) + 1);
        else
            text << (semitones > 0 ? "+" : "") << semitones;

        return text;
    }

    bool songHasKey (const BuiltInHarmoniser& p)
    {
        return juce::BigInteger (p.getSongKeyMask()).countNumberOfSetBits() == 7;
    }

    /** The lines the editor and its guide write about the track: the key problem, or the latency problem. */
    juce::String getLeadLine (const BuiltInHarmoniser& p)
    {
        const auto host = getHostTrack (p);
        const auto s = p.getSettings();

        if (host.isBus || host.isReturn || host.isInstrument)
        {
            const double rate = p.getSampleRate();
            return harm::getLatencyLine (host.isReturn ? "return" : host.isBus ? "bus" : "instrument track",
                                         rate > 0.0 ? 1000.0 * BuiltInHarmoniser::getLatencyFor (rate) / rate : 0.0);
        }

        if (s.mode == harm::Mode::key && s.key == harm::keyFromSong && ! songHasKey (p))
            return "No song key here: choose one under Key.";

        return {};
    }
}

//==============================================================================
HarmoniserView::HarmoniserView (BuiltInHarmoniser& p, std::function<void (int)> open, std::function<void (int)> hover)
    : processor (p), guide (std::move (open)), hoverTopic (std::move (hover))
{
    setTopic (*this, topicTrace);
}

juce::Rectangle<float> HarmoniserView::getNowArea() const { return getLocalBounds().toFloat().reduced (5.0f).removeFromLeft (nowWidth); }

juce::Rectangle<float> HarmoniserView::getTraceArea() const
{
    auto r = getLocalBounds().toFloat().reduced (5.0f);
    r.removeFromLeft (nowWidth + 6.0f);
    return r.withTrimmedBottom (levelHeight + 6.0f);
}

juce::Rectangle<float> HarmoniserView::getLevelArea() const
{
    auto r = getLocalBounds().toFloat().reduced (5.0f);
    r.removeFromLeft (nowWidth + 6.0f);
    return r.removeFromBottom (levelHeight);
}

double HarmoniserView::getSeconds() const
{
    if (auto* settings = tunetext::getSettings())
        return juce::jlimit (4.0, 16.0, settings->getDoubleValue ("view.harmoniserTraceSeconds", 8.0));

    return 8.0;
}

tunepics::Trace HarmoniserView::makeTrace() const
{
    const auto& frames = processor.getRecentFrames();
    traceFrames.resize (frames.size());

    for (auto& v : traceVoices)
        v.resize (frames.size());

    for (size_t i = 0; i < frames.size(); ++i)
    {
        traceFrames[i].beat = frames[i].beat;
        traceFrames[i].sung = traceFrames[i].corrected = frames[i].lead;
        traceFrames[i].rmsDb = frames[i].rmsDb;

        for (size_t v = 0; v < traceVoices.size(); ++v)
            traceVoices[v][i] = frames[i].voice[v];
    }

    tunepics::Trace t;
    t.frames = &traceFrames;
    t.framesPerSecond = processor.getFramesPerSecond();
    t.seconds = getSeconds();
    t.keyMask = processor.getEffectiveKeyMask();
    t.showCorrected = false;
    t.barLength = timesig::getBarLength (tunetext::getSong().timeSignature);

    for (int v = 0; v < harm::numVoices; ++v)
        t.extraLines.push_back ({ &traceVoices[(size_t) v], tunepics::getVoiceColour (v) });

    return t;
}

void HarmoniserView::paintNow (juce::Graphics& g, juce::Rectangle<float> area)
{
    drawPanel (g, area);
    auto r = area.reduced (8.0f, 6.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("NOW", r.removeFromTop (14.0f), juce::Justification::centredLeft);
    r.removeFromTop (2.0f);

    const float lead = processor.liveLead.load();
    const auto settings = processor.getSettings();
    const int mask = processor.getEffectiveKeyMask();

    if (lead >= 0.0f)
        centreNote = juce::roundToInt (lead);

    constexpr int rows = 25;
    const int low = centreNote - 12;
    const float rowHeight = r.getHeight() / (float) rows;
    auto keys = r.removeFromLeft (64.0f);
    auto yFor = [&] (float pitch) { return keys.getBottom() - (pitch - (float) low + 0.5f) * rowHeight; };

    for (int note = low; note < low + rows; ++note)
    {
        const auto row = juce::Rectangle<float> (keys.getX(), yFor ((float) note) - rowHeight * 0.5f, keys.getWidth(), rowHeight);
        g.setColour (isBlack (note) ? theme::background.withAlpha (0.8f) : theme::textDim.withAlpha (0.4f));
        g.fillRect (row.reduced (0.0f, 0.5f));

        if (inKey (note, mask))
        {
            g.setColour (tunepics::harmoniserColour.withAlpha (0.3f));
            g.fillRect (row.reduced (0.0f, 0.5f));
        }

        if (note % 12 == 0)
        {
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (9.5f));
            g.drawText (tunepics::getNoteName (note), row.withWidth (26.0f).translated (2.0f, 0.0f), juce::Justification::centredLeft);
        }
    }

    if (lead < 0.0f)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
        g.drawText (dash(), r.withTrimmedLeft (30.0f).removeFromTop (60.0f), juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (12.5f));
        g.drawText ("no clear note", r.withTrimmedLeft (30.0f).withTrimmedTop (60.0f).removeFromTop (20.0f), juce::Justification::centredLeft);
        return;
    }

    // Each voice's note on the keyboard, an arrow from the lead to it, and its label.
    struct Label { float y; juce::String text; juce::Colour colour; bool bold; };
    std::vector<Label> labels;
    const float dotX = keys.getRight() - 14.0f;
    labels.push_back ({ yFor (lead), tunepics::getNoteName (juce::roundToInt (lead)), theme::text, true });

    for (int v = 0; v < harm::numVoices; ++v)
    {
        const float voice = processor.liveVoice[v].load();

        if (voice < 0.0f)
            continue;

        const auto colour = tunepics::getVoiceColour (v);
        const float x = keys.getRight() + 12.0f + (float) v * 9.0f;
        const float y0 = yFor (lead), y1 = yFor (voice);
        g.setColour (colour);

        if (std::abs (y1 - y0) > 2.0f)
        {
            g.drawLine (x, y0, x, y1, 1.6f);
            const float dir = y1 < y0 ? 1.0f : -1.0f;
            juce::Path head;
            head.addTriangle (x, y1, x - 3.5f, y1 + 6.0f * dir, x + 3.5f, y1 + 6.0f * dir);
            g.fillPath (head);
        }

        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ dotX, y1 }));
        labels.push_back ({ y1, describeVoice (lead, voice, settings.mode, settings.voices[(size_t) v]), colour, false });
    }

    g.setColour (theme::text);
    g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ dotX, yFor (lead) }));

    // Labels at their notes' heights, pushed apart so none covers another.
    std::sort (labels.begin(), labels.end(), [] (const Label& a, const Label& b) { return a.y < b.y; });

    for (size_t i = 1; i < labels.size(); ++i)
        labels[i].y = juce::jmax (labels[i].y, labels[i - 1].y + 16.0f);

    const float textX = keys.getRight() + 34.0f;

    for (const auto& l : labels)
    {
        g.setColour (l.colour);
        g.setFont (juce::FontOptions (l.bold ? 15.0f : 12.5f, l.bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (l.text, juce::Rectangle<float> (textX, juce::jlimit (r.getY(), r.getBottom() - 16.0f, l.y - 8.0f), area.getRight() - textX - 6.0f, 16.0f),
                    juce::Justification::centredLeft);
    }
}

void HarmoniserView::paint (juce::Graphics& g)
{
    paintNow (g, getNowArea());
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

    // The key of the lead line, over the trace.
    const auto lead = getLeadLine (processor);

    if (lead.isNotEmpty())
    {
        auto note = getTraceArea().reduced (8.0f, 6.0f).removeFromTop (32.0f);
        g.setColour (theme::background.withAlpha (0.75f));
        g.fillRoundedRectangle (note.withTrimmedLeft (tunepics::traceMargin), 4.0f);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (12.5f));
        g.drawFittedText (lead, note.withTrimmedLeft (tunepics::traceMargin + 8.0f).toNearestInt(), juce::Justification::centredLeft, 2);
    }
}

void HarmoniserView::mouseMove (const juce::MouseEvent& e)
{
    if (hoverTopic == nullptr)
        return;

    hoverTopic (getNowArea().contains (e.position) ? topicNow : getLevelArea().contains (e.position) ? topicLevel : topicTrace);
}

void HarmoniserView::mouseDown (const juce::MouseEvent& e)
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
        guideicon::addItem (menu, 100, "What is a harmoniser?");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<HarmoniserView> (this)] (int choice)
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
                settings->setValue ("view.harmoniserTraceSeconds", (double) choice);
                safe->repaint();
            }
        });
        return;
    }

    if (hit.valid && hit.beat >= 0.0)
        tunetext::seekToBeat (hit.beat);
}

juce::String HarmoniserView::describeFrame (int index) const
{
    const auto& frames = processor.getRecentFrames();

    if (! juce::isPositiveAndBelow (index, (int) frames.size()))
        return {};

    const auto& f = frames[(size_t) index];
    juce::String text;

    if (f.beat >= 0.0f)
        text << "Bar " << (int) std::floor (f.beat / (float) timesig::getBarLength (tunetext::getSong().timeSignature)) + 1 << dot();

    if (f.lead < 0.0f)
        return text + "no clear note here: a consonant, a breath, silence or several notes";

    const auto settings = processor.getSettings();
    text << tunepics::getNoteName (juce::roundToInt (f.lead));
    bool first = true;

    for (int v = 0; v < harm::numVoices; ++v)
    {
        if (f.voice[(size_t) v] < 0.0f)
            continue;

        const auto note = tunepics::getNoteName (juce::roundToInt (f.voice[(size_t) v]));
        const auto step = getStepName (juce::roundToInt (f.voice[(size_t) v] - f.lead));
        text << (first ? arrow() : dot()) << note << " (" << step << ")";
        first = false;
    }

    if (first)
        text << dot() << "no voice here";

    juce::ignoreUnused (settings);
    return text;
}

juce::String HarmoniserView::getTooltip()
{
    const auto mouse = getMouseXYRelative().toFloat();

    if (getNowArea().contains (mouse))
        return "The note sung now, each voice's note and the interval between them. The key's notes are shaded.";

    if (getLevelArea().contains (mouse))
        return "How loud the sound was over the same seconds as the trace.";

    const auto trace = makeTrace();
    const auto hit = tunepics::hitTrace (getTraceArea(), trace, mouse);

    if (! hit.valid)
        return "The lead and each voice over the last seconds, on the piano roll's rows. Click to move the playhead; right-click to show more or fewer seconds.";

    return describeFrame (hit.frame);
}

//==============================================================================
BuiltInHarmoniserEditor::BuiltInHarmoniserEditor (BuiltInHarmoniser& p)
    : EffectEditor (p, "DawIt Harmoniser", "What is a harmoniser?", {}, unusedListen(), p.clap, tunepics::harmoniserColour, fx::Clap::Sound::sungLine),
      harmoniser (p), view (p, [this] (int page) { openGuide (page); }, [this] (int topicIndex) { hover (topicIndex); })
{
    addAndMakeVisible (view);
    listenButton.setVisible (false);

    mix = std::make_unique<EffectKnob> (p.getParameters(), BuiltInHarmoniser::getParameterId (harm::Knob::mix), "Mix", (int) Control::mix, colour);
    mix->slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + "%"; };
    mix->slider.setTooltip (gen::harmoniserguide::getControlText (Control::mix));
    mix->slider.updateText();

    harmony.button.setTooltip (gen::harmoniserguide::getControlText (Control::harmony));
    keyChoice.button.setTooltip (gen::harmoniserguide::getControlText (Control::key));
    scale.button.setTooltip (gen::harmoniserguide::getControlText (Control::scale));
    setTopic (harmony, (int) Control::harmony);
    setTopic (keyChoice, (int) Control::key);
    setTopic (scale, (int) Control::scale);
    harmony.button.onClick = [this] { changeSettings ([] (harm::Settings& s) { s.mode = s.mode == harm::Mode::key ? harm::Mode::fixed : harm::Mode::key; }); };
    keyChoice.button.onClick = [this] { showKeyMenu(); };
    scale.button.onClick = [this] { changeSettings ([] (harm::Settings& s) { s.scale = s.scale == harm::Scale::major ? harm::Scale::minor : harm::Scale::major; }); };

    std::vector<Group> controlGroups { { "HARMONY", { &harmony, &keyChoice, &scale, mix.get() }, 0 } };

    for (int v = 0; v < harm::numVoices; ++v)
    {
        auto* on = voiceOn.add (new Switch ("On", (int) Control::on));
        on->button.setTooltip (gen::harmoniserguide::getControlText (Control::on));
        on->button.onClick = [this, v] { changeSettings ([v] (harm::Settings& s) { s.voices[(size_t) v].on = ! s.voices[(size_t) v].on; }); };

        auto make = [&] (int index, harm::VoiceKnob knob, const char* name, Control control)
        {
            auto k = std::make_unique<EffectKnob> (p.getParameters(), BuiltInHarmoniser::getParameterId (v, knob), name, (int) control, colour);
            k->slider.setTooltip (gen::harmoniserguide::getControlText (control));
            voiceKnobs[(size_t) v][(size_t) index] = std::move (k);
        };

        make (0, harm::VoiceKnob::steps, "Interval", Control::intervalKey);
        make (1, harm::VoiceKnob::level, "Level", Control::level);
        make (2, harm::VoiceKnob::pan, "Pan", Control::pan);
        make (3, harm::VoiceKnob::detune, "Detune", Control::detune);
        make (4, harm::VoiceKnob::delay, "Delay", Control::delay);

        auto& k = voiceKnobs[(size_t) v];
        k[1]->slider.textFromValueFunction = [] (double x)
        {
            return x <= (double) harm::minLevelDb + 0.05 ? juce::String ("off") : juce::String (x, 1) + " dB";
        };
        k[2]->slider.textFromValueFunction = [] (double x)
        {
            const int n = juce::roundToInt (x);
            return n == 0 ? juce::String ("centre") : juce::String (std::abs (n)) + (n < 0 ? "% left" : "% right");
        };
        k[3]->slider.textFromValueFunction = [] (double x) { const int n = juce::roundToInt (x); return (n > 0 ? "+" : "") + juce::String (n) + " cents"; };
        k[4]->slider.textFromValueFunction = [] (double x) { return juce::String (juce::roundToInt (x)) + " ms"; };

        for (size_t i = 1; i < k.size(); ++i)
            k[i]->slider.updateText();

        controlGroups.push_back ({ "VOICE " + juce::String (v + 1), { on, k[0].get(), k[1].get(), k[2].get(), k[3].get(), k[4].get() }, v == 0 ? 0 : 1 });
    }

    setGroups (controlGroups);

    juce::StringArray presetNames;

    for (const auto& preset : harm::getPresets())
        presetNames.add (preset.name);

    setPresetNames (presetNames);
    clapButton.setTooltip ("Play the singing guide's phrase as a made-up \"ah\" through DawIt Harmoniser.");

    tick();
    showTopic (CommonTopic::overview);
    setSize (1240, 640);
}

std::atomic<bool>& BuiltInHarmoniserEditor::unusedListen()
{
    static std::atomic<bool> unused { false };
    return unused;
}

void BuiltInHarmoniserEditor::changeSettings (const std::function<void (harm::Settings&)>& change)
{
    auto s = harmoniser.getSettings();
    change (s);
    harmoniser.setSettings (s);
}

void BuiltInHarmoniserEditor::showKeyMenu()
{
    juce::PopupMenu menu;
    const int current = harmoniser.getSettings().key;
    menu.addItem (1, "Song", true, current == harm::keyFromSong);
    menu.addSeparator();

    for (int k = 1; k <= 12; ++k)
        menu.addItem (k + 1, harm::getNoteName (k - 1), true, current == k);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&keyChoice.button), [safe = juce::Component::SafePointer<BuiltInHarmoniserEditor> (this)] (int choice)
    {
        if (safe != nullptr && choice > 0)
            safe->changeSettings ([choice] (harm::Settings& s) { s.key = choice - 1; });
    });
}

void BuiltInHarmoniserEditor::applyPreset (int index)
{
    const auto& list = harm::getPresets();

    if (! juce::isPositiveAndBelow (index, (int) list.size()))
        return;

    harmoniser.applyPreset (list[(size_t) index]);
    presetIndex = index;
    showPresetName (list[(size_t) index].name);
    showTopic (CommonTopic::presets);
}

std::vector<Guide::Page> BuiltInHarmoniserEditor::createGuidePages() { return harmtext::makeGuide (harmoniser); }

void BuiltInHarmoniserEditor::layoutPictures (juce::Rectangle<int> r) { view.setBounds (r); }

void BuiltInHarmoniserEditor::updateVoiceKnobs (harm::Mode mode)
{
    // The interval dial follows Semitones in Fixed and Steps in Key: two parameters, so each keeps its own value.
    for (int v = 0; v < harm::numVoices; ++v)
    {
        auto& knob = *voiceKnobs[(size_t) v][0];
        knob.attachTo (BuiltInHarmoniser::getParameterId (v, mode == harm::Mode::fixed ? harm::VoiceKnob::interval : harm::VoiceKnob::steps));
        setTopic (knob, (int) (mode == harm::Mode::fixed ? Control::intervalFixed : Control::intervalKey));
        knob.slider.setTooltip (gen::harmoniserguide::getControlText (mode == harm::Mode::fixed ? Control::intervalFixed : Control::intervalKey));
        knob.slider.textFromValueFunction = [mode] (double x)
        {
            const int n = juce::roundToInt (x);

            if (mode == harm::Mode::fixed)
                return (n > 0 ? "+" : "") + juce::String (n) + dot() + harm::getIntervalName (mode, n);

            return harm::getIntervalName (mode, n);
        };
        knob.slider.updateText();
    }
}

void BuiltInHarmoniserEditor::tick()
{
    harmoniser.drainFrames();
    const auto s = harmoniser.getSettings();

    if ((int) s.mode != shownMode)
    {
        shownMode = (int) s.mode;
        updateVoiceKnobs (s.mode);
    }

    harmony.button.setButtonText (s.mode == harm::Mode::fixed ? "Fixed" : "Key");
    harmony.button.setToggleState (s.mode == harm::Mode::key, juce::dontSendNotification);
    keyChoice.button.setButtonText (s.key == harm::keyFromSong ? "Song" : harm::getNoteName (s.key - 1));
    scale.button.setButtonText (s.scale == harm::Scale::major ? "Major" : "Minor");
    scale.setAlpha (s.key == harm::keyFromSong ? 0.5f : 1.0f);   // the song's own scale is used with Song
    keyChoice.setAlpha (s.mode == harm::Mode::key ? 1.0f : 0.5f);

    for (int v = 0; v < harm::numVoices; ++v)
    {
        const auto& voice = s.voices[(size_t) v];
        voiceOn[v]->button.setButtonText (voice.on ? "On" : "Off");
        voiceOn[v]->button.setToggleState (voice.on, juce::dontSendNotification);

        for (auto& k : voiceKnobs[(size_t) v])
            k->setAlpha (voice.on ? 1.0f : 0.5f);
    }

    view.repaint();
}

juce::AttributedString BuiltInHarmoniserEditor::buildText (int topicIndex)
{
    juce::AttributedString text;
    text.setWordWrap (juce::AttributedString::byWord);

    static const char* const titles[] = { "Harmony", "Key", "Scale", "Mix", "On", "Interval", "Interval", "Level", "Pan", "Detune", "Delay" };

    if (juce::isPositiveAndBelow (topicIndex, (int) std::size (titles)))
    {
        addTitle (text, titles[topicIndex], colour);
        addLead (text, gen::harmoniserguide::getControlText ((Control) topicIndex));
    }
    else if (topicIndex == CommonTopic::presets && juce::isPositiveAndBelow (presetIndex, (int) harm::getPresets().size()))
    {
        const auto& p = harm::getPresets()[(size_t) presetIndex];
        addTitle (text, p.name, colour);
        addLead (text, p.why);
    }
    else if (topicIndex == CommonTopic::clap)
    {
        addTitle (text, "Play a sung line", colour);
        addLead (text, "The singing guide's line \"hold on, hold on to-night\" as a made-up \"ah\" in C major, through the effect, with the settings in this window.");
    }
    else if (topicIndex == topicNow)
    {
        addTitle (text, "Now", colour);
        addLead (text, "The note sung now as a white dot on a keyboard of two octaves, with each voice's note in its colour and the interval between them, in semitones and in names. The key's notes are shaded. A dash means no clear note: silence, a consonant, a breath or several notes at once.");
    }
    else if (topicIndex == topicTrace)
    {
        addTitle (text, "Trace", colour);
        addLead (text, "The last seconds on the piano roll's rows: the lead in white and each voice in its colour, with gaps where there is no clear note. Hover for the notes and the intervals; click to move the playhead there; right-click to show more or fewer seconds.");
    }
    else if (topicIndex == topicLevel)
    {
        addTitle (text, "Level", colour);
        addLead (text, "How loud the sound was over the same seconds. A gap in the trace while the level is up means no clear note: a consonant, a breath or several notes at once.");
    }
    else
    {
        addTitle (text, "DawIt Harmoniser", colour);
        const auto line = getLeadLine (harmoniser);
        addLead (text, line.isNotEmpty() ? line : juce::String ("Adds two pitched voices to one voice or one single-note line: a harmony in the song's key, an octave, a fifth or a wide double."));
        addSection (text, "", "Put it on the vocal track, after DawIt Tune if the take needs tuning. The now column and the trace show the lead and each voice as the song plays.");
    }

    return text;
}
}
