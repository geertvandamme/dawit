#include "BuiltInSynthEditor.h"

namespace daw
{
namespace
{
    constexpr int margin = 16, headerHeight = 34, displayHeight = 78, tileHeight = 82, tileGap = 8, footerHeight = 30;

    /** A small picture of each sound, drawn in `colour` inside `area`. */
    void drawIcon (juce::Graphics& g, BuiltInSound sound, juce::Rectangle<float> area, juce::Colour colour)
    {
        g.setColour (colour);
        const float x = area.getX(), y = area.getY(), w = area.getWidth(), h = area.getHeight();
        const juce::PathStrokeType stroke (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path p;

        auto wave = [&] (float amplitude, float cycles, float phase)
        {
            juce::Path wavePath;

            for (int i = 0; i <= 48; ++i)
            {
                const float fx = (float) i / 48.0f;
                const float fy = y + h * 0.5f - amplitude * h * 0.5f
                                 * std::sin (juce::MathConstants<float>::twoPi * (cycles * fx + phase));
                i == 0 ? wavePath.startNewSubPath (x + fx * w, fy) : wavePath.lineTo (x + fx * w, fy);
            }

            return wavePath;
        };

        switch (sound)
        {
            case BuiltInSound::piano:
            {
                const float keyW = w / 4.0f;

                for (int i = 0; i < 4; ++i)
                    g.drawRoundedRectangle (x + i * keyW, y, keyW, h, 1.5f, 1.4f);

                for (int i = 1; i < 3; ++i) // C# and D# over C D E F
                    g.fillRoundedRectangle (x + i * keyW - keyW * 0.3f, y, keyW * 0.6f, h * 0.6f, 1.0f);

                return;
            }

            case BuiltInSound::organ:
            {
                const float heights[] = { 0.55f, 0.9f, 0.7f, 0.4f, 0.6f };
                const float barW = w / 5.0f;

                for (int i = 0; i < 5; ++i)
                {
                    const float bx = x + i * barW + barW * 0.3f;
                    g.fillRoundedRectangle (bx, y, barW * 0.4f, h * heights[i], 1.5f);
                    g.fillEllipse (bx - barW * 0.1f, y + h * heights[i] - barW * 0.3f, barW * 0.6f, barW * 0.6f);
                }

                return;
            }

            case BuiltInSound::bass:
                g.strokePath (wave (0.85f, 1.0f, 0.0f), juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                return;

            case BuiltInSound::lead:
                p.startNewSubPath (x, y + h);

                for (int i = 0; i < 3; ++i)
                {
                    p.lineTo (x + w * (float) (i + 1) / 3.0f, y);
                    p.lineTo (x + w * (float) (i + 1) / 3.0f, y + h);
                }

                g.strokePath (p, stroke);
                return;

            case BuiltInSound::pluck:
                for (int i = 0; i < 5; ++i)
                {
                    const float px = x + w * (0.08f + 0.2f * (float) i);
                    const float height = h * std::exp (-0.45f * (float) i);
                    g.drawLine (px, y + h, px, y + h - height, 2.0f);
                }

                return;

            case BuiltInSound::pad:
                g.strokePath (wave (0.55f, 1.5f, 0.0f), stroke);
                g.setColour (colour.withAlpha (0.55f));
                g.strokePath (wave (0.4f, 1.5f, 0.25f), stroke);
                return;

            case BuiltInSound::drums:
            {
                const auto top = juce::Rectangle<float> (x + w * 0.1f, y + h * 0.35f, w * 0.8f, h * 0.3f);
                g.drawEllipse (top, 1.6f);
                p.startNewSubPath (top.getX(), top.getCentreY());
                p.lineTo (top.getX(), y + h * 0.88f);
                p.quadraticTo (x + w * 0.5f, y + h * 1.08f, top.getRight(), y + h * 0.88f);
                p.lineTo (top.getRight(), top.getCentreY());
                g.strokePath (p, stroke);
                g.drawLine (x + w * 0.2f, y, x + w * 0.5f, y + h * 0.42f, 1.8f);
                g.drawLine (x + w * 0.8f, y, x + w * 0.55f, y + h * 0.42f, 1.8f);
                return;
            }

            case BuiltInSound::percussion:
            {
                // A conga and a maraca.
                const auto top = juce::Rectangle<float> (x + w * 0.05f, y + h * 0.05f, w * 0.5f, h * 0.22f);
                g.drawEllipse (top, 1.6f);
                p.startNewSubPath (top.getX(), top.getCentreY());
                p.lineTo (x + w * 0.15f, y + h);
                p.lineTo (x + w * 0.45f, y + h);
                p.lineTo (top.getRight(), top.getCentreY());
                g.strokePath (p, stroke);
                g.fillEllipse (x + w * 0.66f, y + h * 0.1f, w * 0.3f, h * 0.38f);
                g.drawLine (x + w * 0.81f, y + h * 0.45f, x + w * 0.81f, y + h, 2.0f);
                return;
            }
        }
    }

    /** What a click on a sound plays, so you hear it straight away. */
    std::vector<int> getPreviewNotes (BuiltInSound sound)
    {
        switch (sound)
        {
            case BuiltInSound::piano:
            case BuiltInSound::organ:
            case BuiltInSound::pad:   return { 60, 64, 67 };
            case BuiltInSound::bass:  return { 36 };
            case BuiltInSound::lead:
            case BuiltInSound::pluck: return { 72 };
            case BuiltInSound::drums: return { 36, 42 };
            case BuiltInSound::percussion: return { 63, 70 };
        }

        return {};
    }
} // namespace

//==============================================================================
class BuiltInSynthEditor::SoundButton final : public juce::Component
{
public:
    SoundButton (BuiltInSynthEditor& o, BuiltInSound s) : owner (o), sound (s)
    {
        setTitle (builtInSounds::getName (sound));
        setDescription (builtInSounds::getDescription (sound));
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const bool selected = owner.shownSound == sound;
        const bool hovered = isMouseOver();
        const auto colour = owner.track.colour;
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (selected ? colour.withAlpha (0.35f) : (hovered ? theme::panelLight.brighter (0.08f) : theme::panelLight));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (selected ? colour : theme::gridBar);
        g.drawRoundedRectangle (bounds, 6.0f, selected ? 1.5f : 1.0f);

        auto area = getLocalBounds().reduced (8, 10);
        const auto name = area.removeFromBottom (18);
        drawIcon (g, sound, area.withSizeKeepingCentre (34, 24).toFloat(), selected ? colour.brighter (0.6f) : theme::text.withAlpha (0.8f));

        g.setColour (selected ? theme::text : theme::text.withAlpha (0.85f));
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (builtInSounds::getName (sound), name, juce::Justification::centred, true);
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        owner.hoveredSound = sound;
        owner.repaint();
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (owner.hoveredSound == sound)
            owner.hoveredSound.reset();

        owner.repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()))
            owner.pick (sound);
    }

private:
    BuiltInSynthEditor& owner;
    const BuiltInSound sound;
};

//==============================================================================
BuiltInSynthEditor::BuiltInSynthEditor (Engine& e, PluginWindowManager& w, Track& t)
    : engine (e), windows (w), track (t), shownSound (t.builtInSound.load())
{
    for (auto sound : builtInSounds::all)
        addAndMakeVisible (buttons.add (new SoundButton (*this, sound)));

    setWantsKeyboardFocus (true);
    engine.addChangeListener (this);

    const int numSounds = (int) builtInSounds::all.size();
    setSize (margin * 2 + numSounds * 84 + (numSounds - 1) * tileGap,
             headerHeight + 6 + displayHeight + 12 + tileHeight + footerHeight + margin * 2);

    juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<BuiltInSynthEditor> (this)]
    {
        if (safeThis != nullptr && safeThis->isShowing())
            safeThis->grabKeyboardFocus();
    });
}

BuiltInSynthEditor::~BuiltInSynthEditor()
{
    engine.removeChangeListener (this);
}

void BuiltInSynthEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Nothing to pick once the track is gone or plays an instrument plugin.
    if (engine.indexOf (&track) < 0 || track.hasInstrument())
    {
        juce::MessageManager::callAsync ([&w = windows, key = static_cast<const void*> (&track)] { w.closeFor (key); });
        return;
    }

    if (const auto sound = track.builtInSound.load(); sound != shownSound)
    {
        shownSound = sound;
        repaint();

        for (auto* b : buttons)
            b->repaint();
    }
}

void BuiltInSynthEditor::pick (BuiltInSound sound)
{
    engine.setBuiltInSound (track, sound);
    shownSound = sound;
    repaint();

    for (auto* b : buttons)
        b->repaint();

    // Previews play on the selected track, which is normally the one this window belongs to.
    if (engine.getLiveMidiTrack() == &track)
        for (int note : getPreviewNotes (sound))
            engine.previewNote (note, 0.8f, sound == BuiltInSound::pad ? 1200 : 500);
}

bool BuiltInSynthEditor::keyPressed (const juce::KeyPress& key)
{
    const int step = key == juce::KeyPress::leftKey ? -1 : (key == juce::KeyPress::rightKey ? 1 : 0);

    if (step == 0)
        return false;

    const auto& all = builtInSounds::all;
    const auto index = std::distance (all.begin(), std::find (all.begin(), all.end(), shownSound));
    const auto count = (std::ptrdiff_t) all.size();
    pick (all[(size_t) ((index + step + count) % count)]);
    return true;
}

void BuiltInSynthEditor::resized()
{
    auto r = getLocalBounds().reduced (margin);
    headerArea = r.removeFromTop (headerHeight);
    r.removeFromTop (6);
    displayArea = r.removeFromTop (displayHeight);
    r.removeFromTop (12);

    auto row = r.removeFromTop (tileHeight);
    footerArea = r.removeFromTop (footerHeight);
    const int tileWidth = (row.getWidth() - tileGap * (buttons.size() - 1)) / buttons.size();

    for (auto* b : buttons)
    {
        b->setBounds (row.removeFromLeft (tileWidth));
        row.removeFromLeft (tileGap);
    }
}

void BuiltInSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);

    // Header: the synth's name, and the track it plays on.
    auto header = headerArea;
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText ("DawIt Synth", header.removeFromLeft (200), juce::Justification::centredLeft, false);

    g.setFont (juce::FontOptions (12.0f));
    const auto trackName = track.name;
    const int nameWidth = juce::GlyphArrangement::getStringWidthInt (juce::Font (juce::FontOptions (12.0f)), trackName) + 2;
    auto trackLabel = header.removeFromRight (nameWidth);
    g.drawText (trackName, trackLabel, juce::Justification::centredRight, false);
    header.removeFromRight (8);
    g.setColour (track.colour);
    g.fillEllipse (header.removeFromRight (9).withSizeKeepingCentre (9, 9).toFloat());
    header.removeFromRight (8);
    g.setColour (theme::textDim);
    g.drawText ("Built-in instrument on", header, juce::Justification::centredRight, false);

    // The display, like a hardware synth's screen.
    const auto display = displayArea.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (display, 7.0f);
    g.setColour (theme::gridBar);
    g.drawRoundedRectangle (display.reduced (0.5f), 7.0f, 1.0f);

    auto text = displayArea.reduced (16, 10);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("SOUND  " + juce::String ((int) shownSound + 1) + " / " + juce::String ((int) builtInSounds::all.size()),
                text.removeFromTop (14), juce::Justification::centredLeft, false);

    g.setColour (track.colour.brighter (0.5f));
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText (builtInSounds::getName (shownSound), text.removeFromTop (32), juce::Justification::centredLeft, false);

    g.setColour (theme::text.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (builtInSounds::getDescription (shownSound), text, juce::Justification::centredLeft, true);

    // Footer: what the hovered sound is, or how to use the window.
    auto footer = footerArea.withTrimmedTop (4);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (hoveredSound.has_value() && *hoveredSound != shownSound
                    ? builtInSounds::getName (*hoveredSound) + ": " + builtInSounds::getDescription (*hoveredSound)
                    : juce::String ("Click a sound to pick it and hear it. Left and right arrow keys step through them."),
                footer, juce::Justification::centredLeft, true);
}
} // namespace daw
