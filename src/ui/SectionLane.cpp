#include "GuideButton.h"
#include "SectionLane.h"
#include "generate/StructureGuide.h"

namespace daw
{
juce::Colour SectionLane::getColour (SectionType t)
{
    // Calm sections in cool colours, high points in warm ones.
    switch (t)
    {
        case SectionType::intro:      return juce::Colour::fromHSV (0.60f, 0.22f, 0.55f, 1.0f);
        case SectionType::verse:      return juce::Colour::fromHSV (0.60f, 0.50f, 0.58f, 1.0f);
        case SectionType::preChorus:  return juce::Colour::fromHSV (0.76f, 0.42f, 0.58f, 1.0f);
        case SectionType::chorus:     return juce::Colour::fromHSV (0.03f, 0.55f, 0.66f, 1.0f);
        case SectionType::postChorus: return juce::Colour::fromHSV (0.08f, 0.50f, 0.62f, 1.0f);
        case SectionType::bridge:     return juce::Colour::fromHSV (0.36f, 0.45f, 0.52f, 1.0f);
        case SectionType::solo:       return juce::Colour::fromHSV (0.13f, 0.55f, 0.60f, 1.0f);
        case SectionType::interlude:  return juce::Colour::fromHSV (0.48f, 0.35f, 0.50f, 1.0f);
        case SectionType::phrase:     return juce::Colour::fromHSV (0.54f, 0.45f, 0.55f, 1.0f);
        case SectionType::breakdown:  return juce::Colour::fromHSV (0.47f, 0.50f, 0.45f, 1.0f);
        case SectionType::buildUp:    return juce::Colour::fromHSV (0.86f, 0.45f, 0.60f, 1.0f);
        case SectionType::drop:       return juce::Colour::fromHSV (0.98f, 0.62f, 0.68f, 1.0f);
        case SectionType::outro:      return juce::Colour::fromHSV (0.60f, 0.12f, 0.45f, 1.0f);
    }

    return theme::panelLight;
}

SectionLane::SectionLane (Engine& e, TimelineView& t) : engine (e), timeline (t)
{
    setTooltip ("Sections: drag across bars to mark an intro, verse, chorus, drop... Drag a section's edge to resize it, "
                "click it to change, loop or delete it. The song structure guide explains the usual structures.");
    engine.addChangeListener (this);
}

SectionLane::~SectionLane()
{
    engine.removeChangeListener (this);
}

void SectionLane::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

int SectionLane::barAt (float x) const
{
    return (int) std::floor (timeline.xToBeat (x) / 4.0);
}

int SectionLane::barEdgeAt (float x) const
{
    return juce::jmax (0, juce::roundToInt (timeline.xToBeat (x) / 4.0));
}

int SectionLane::sectionIndexAt (int bar) const
{
    const auto& sections = engine.getSections();

    for (size_t i = 0; i < sections.size(); ++i)
        if (bar >= sections[i].startBar && bar < sections[i].getEndBar())
            return (int) i;

    return -1;
}

SectionLane::Drag SectionLane::edgeAt (float x, int& sectionIndex) const
{
    const auto& sections = engine.getSections();

    for (size_t i = 0; i < sections.size(); ++i)
    {
        const float x1 = timeline.beatToX (sections[i].startBar * 4.0), x2 = timeline.beatToX (sections[i].getEndBar() * 4.0);
        sectionIndex = (int) i;

        if (std::abs (x - x2) <= 5.0f)
            return Drag::resizeEnd;

        if (std::abs (x - x1) <= 5.0f)
            return Drag::resizeStart;
    }

    sectionIndex = -1;
    return Drag::none;
}

std::pair<int, int> SectionLane::getFreeRange (int bar) const
{
    int first = 0, end = std::numeric_limits<int>::max();

    for (auto& s : engine.getSections())
    {
        if (s.getEndBar() <= bar)
            first = juce::jmax (first, s.getEndBar());
        else if (s.startBar > bar)
            end = juce::jmin (end, s.startBar);
    }

    return { first, end };
}

void SectionLane::updatePlayhead()
{
    const float x = timeline.beatToX (engine.getPositionBeats());

    if (std::abs (x - playheadX) < 0.5f)
        return;

    repaint ((int) playheadX - 2, 0, 5, getHeight());
    playheadX = x;
    repaint ((int) x - 2, 0, 5, getHeight());
}

//==============================================================================
void SectionLane::paint (juce::Graphics& g)
{
    g.fillAll (theme::laneAlt);

    const float barWidth = (float) (timeline.pixelsPerBeat * 4.0);
    const int firstBar = juce::jmax (0, barAt (0.0f));
    const int lastBar = barAt ((float) getWidth());

    // Bar lines, thinned out when zoomed far out (like the ruler).
    int barStep = 1;
    while (barStep * barWidth < 44.0f)
        barStep *= 2;

    g.setColour (theme::grid);
    for (int bar = firstBar - firstBar % barStep; bar <= lastBar; bar += barStep)
        g.fillRect (timeline.beatToX (bar * 4.0), 0.0f, 1.0f, (float) getHeight());

    if (hoverBar >= 0 && drag == Drag::none && ! preview.has_value() && sectionIndexAt (hoverBar) < 0)
    {
        const auto r = juce::Rectangle<float> (timeline.beatToX (hoverBar * 4.0), 0.0f, barWidth, (float) getHeight()).reduced (1.0f, 3.0f);
        g.setColour (theme::text.withAlpha (0.06f));
        g.fillRoundedRectangle (r, 3.0f);

        if (r.getWidth() > 14.0f)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (12.0f));
            g.drawText ("+", r.withTrimmedLeft (5.0f), juce::Justification::centredLeft);
        }
    }

    auto drawSection = [&] (const Section& s, bool highlighted, bool draft)
    {
        const float x1 = timeline.beatToX (s.startBar * 4.0), x2 = timeline.beatToX (s.getEndBar() * 4.0);

        if (x2 < 0.0f || x1 > (float) getWidth())
            return;

        const auto r = juce::Rectangle<float> (x1, 0.0f, x2 - x1, (float) getHeight()).reduced (1.0f, 3.0f);
        const auto colour = getColour (s.type);

        g.setColour (draft ? colour.withAlpha (0.45f) : colour.withAlpha (highlighted ? 1.0f : 0.85f));
        g.fillRoundedRectangle (r, 3.0f);

        if (draft)
        {
            g.setColour (theme::text.withAlpha (0.8f));
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
        }

        if (r.getWidth() > 12.0f)
        {
            // Keep the name visible when the section starts left of the view.
            const auto text = r.withLeft (juce::jmax (r.getX(), 0.0f)).reduced (5.0f, 0.0f);
            const auto label = draft ? juce::String (s.bars) + (s.bars == 1 ? " bar" : " bars")
                                     : s.getDisplayName() + (r.getWidth() > 140.0f ? juce::String::fromUTF8 ("  \xc2\xb7  ") + juce::String (s.bars) + " bars"
                                                                                   : juce::String());
            g.setColour (theme::text);
            g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
            g.drawFittedText (label, text.toNearestInt(), juce::Justification::centredLeft, 1, 0.7f);
        }
    };

    const auto& sections = engine.getSections();

    for (size_t i = 0; i < sections.size(); ++i)
    {
        if ((drag == Drag::resizeStart || drag == Drag::resizeEnd) && (int) i == dragIndex && preview.has_value())
            continue;

        drawSection (sections[i], hoverBar >= sections[i].startBar && hoverBar < sections[i].getEndBar(), false);
    }

    if (preview.has_value())
        drawSection (*preview, true, drag == Drag::create || drag == Drag::none);

    g.setColour (theme::playhead); // where updatePlayhead() last put it, as in TrackLanes
    g.fillRect (playheadX, 0.0f, 1.0f, (float) getHeight());

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

//==============================================================================
void SectionLane::mouseMove (const juce::MouseEvent& e)
{
    int index = -1;
    setMouseCursor (edgeAt (e.position.x, index) != Drag::none ? juce::MouseCursor::LeftRightResizeCursor
                                                               : juce::MouseCursor::NormalCursor);

    const int bar = juce::jmax (-1, barAt (e.position.x));

    if (bar != hoverBar)
    {
        hoverBar = bar;
        repaint();
    }
}

void SectionLane::mouseExit (const juce::MouseEvent&)
{
    hoverBar = -1;
    repaint();
}

void SectionLane::mouseDown (const juce::MouseEvent& e)
{
    const int bar = barAt (e.position.x);
    int index = -1;
    drag = e.mods.isPopupMenu() ? Drag::none : edgeAt (e.position.x, index);
    preview.reset();

    if (drag != Drag::none)
    {
        dragIndex = index;
        preview = engine.getSections()[(size_t) index];
    }
    else if (! e.mods.isPopupMenu() && bar >= 0 && sectionIndexAt (bar) < 0)
    {
        drag = Drag::create;
        dragAnchorBar = bar;
    }
}

void SectionLane::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::none || ! e.mouseWasDraggedSinceMouseDown())
        return;

    const auto& sections = engine.getSections();

    if (drag == Drag::create)
    {
        // Across free bars only: it stops at the sections on either side.
        const auto [free, freeEnd] = getFreeRange (dragAnchorBar);
        const int bar = juce::jmax (0, barAt (e.position.x));
        const int first = juce::jmax (free, juce::jmin (dragAnchorBar, bar));
        const int last = juce::jmin (freeEnd - 1, juce::jmax (dragAnchorBar, bar));
        preview = Section { first, last - first + 1, SectionType::verse, {} };
    }
    else if (dragIndex >= 0 && dragIndex < (int) sections.size())
    {
        const auto& s = sections[(size_t) dragIndex];
        const int edge = barEdgeAt (e.position.x);
        Section changed = s;

        if (drag == Drag::resizeStart)
        {
            const int earliest = dragIndex > 0 ? sections[(size_t) dragIndex - 1].getEndBar() : 0;
            changed.startBar = juce::jlimit (earliest, s.getEndBar() - 1, edge);
            changed.bars = s.getEndBar() - changed.startBar;
        }
        else
        {
            const int latest = dragIndex + 1 < (int) sections.size() ? sections[(size_t) dragIndex + 1].startBar : std::numeric_limits<int>::max();
            changed.bars = juce::jlimit (s.startBar + 1, latest, edge) - s.startBar;
        }

        preview = changed;
    }

    repaint();
}

void SectionLane::mouseUp (const juce::MouseEvent& e)
{
    const auto finished = drag;
    const bool dragged = e.mouseWasDraggedSinceMouseDown() && preview.has_value();
    drag = Drag::none;

    if (finished == Drag::create)
    {
        if (dragged)
        {
            showAddMenu (preview->startBar, preview->getEndBar());
            return;
        }

        // A click on a free bar: a section of the usual length, up to the next one.
        const int bar = barAt (e.position.x);
        const int end = juce::jmin (bar + defaultBars, getFreeRange (bar).second);
        preview = Section { bar, end - bar, SectionType::verse, {} };
        repaint();
        showAddMenu (bar, end);
        return;
    }

    if ((finished == Drag::resizeStart || finished == Drag::resizeEnd) && dragged)
    {
        auto sections = engine.getSections();

        if (dragIndex >= 0 && dragIndex < (int) sections.size())
        {
            sections[(size_t) dragIndex] = *preview;
            engine.setSections (std::move (sections));
        }

        preview.reset();
        repaint();
        return;
    }

    preview.reset();
    repaint();

    if (e.mouseWasClicked())
        if (const int index = sectionIndexAt (barAt (e.position.x)); index >= 0)
            showSectionMenu (index);
}

//==============================================================================
void SectionLane::showAddMenu (int firstBar, int endBar)
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("New section: bar" + (endBar - firstBar > 1 ? "s " + juce::String (firstBar + 1) + "-" + juce::String (endBar)
                                                                     : " " + juce::String (firstBar + 1)));

    for (int i = 0; i < (int) sectionTypes::all.size(); ++i)
    {
        juce::PopupMenu::Item item (sectionTypes::getName (sectionTypes::all[(size_t) i]));
        item.itemID = i + 1;
        item.shortcutKeyDescription = gen::structure::getRole (sectionTypes::all[(size_t) i]);
        item.colour = getColour (sectionTypes::all[(size_t) i]).brighter (0.6f);
        menu.addItem (item);
    }

    constexpr int guideId = 100;

    if (onShowGuide != nullptr)
    {
        menu.addSeparator();
        guideicon::addItem (menu, guideId, "Song structure guide...");
    }

    const auto area = juce::Rectangle<float> (timeline.beatToX (firstBar * 4.0), 0.0f,
                                              (float) (timeline.pixelsPerBeat * 4.0 * (endBar - firstBar)), (float) getHeight());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (area.toNearestInt())),
                        [safeThis = juce::Component::SafePointer<SectionLane> (this), firstBar, endBar] (int result)
                        {
                            auto* self = safeThis.getComponent();

                            if (self == nullptr)
                                return;

                            self->preview.reset();
                            self->repaint();

                            if (result >= 1 && result <= (int) sectionTypes::all.size())
                                self->addSection ({ firstBar, endBar - firstBar, sectionTypes::all[(size_t) result - 1], {} });
                            else if (result == guideId && self->onShowGuide != nullptr)
                                self->onShowGuide (firstBar);
                        });
}

void SectionLane::addSection (Section section)
{
    auto sections = engine.getSections();

    // Number it when the type is already there: Chorus 2.
    const auto count = std::count_if (sections.begin(), sections.end(), [&] (const Section& s) { return s.type == section.type; });

    if (count > 0)
        section.name = sectionTypes::getName (section.type) + " " + juce::String ((int) count + 1);

    sections.push_back (section);
    engine.setSections (std::move (sections));
}

void SectionLane::showSectionMenu (int index)
{
    const auto section = engine.getSections()[(size_t) index];

    juce::PopupMenu menu, types;
    menu.addSectionHeader (section.getDisplayName() + ": bar" + (section.bars > 1 ? "s " + juce::String (section.startBar + 1) + "-" + juce::String (section.getEndBar())
                                                                                  : " " + juce::String (section.startBar + 1))
                           + " (" + gen::structure::getRole (section.type) + ")");

    for (int i = 0; i < (int) sectionTypes::all.size(); ++i)
    {
        juce::PopupMenu::Item item (sectionTypes::getName (sectionTypes::all[(size_t) i]));
        item.itemID = i + 1;
        item.isTicked = sectionTypes::all[(size_t) i] == section.type;
        item.shortcutKeyDescription = gen::structure::getRole (sectionTypes::all[(size_t) i]);
        types.addItem (item);
    }

    enum { renameId = 100, loopId, playheadId, deleteId, clearId, guideId };
    menu.addSubMenu ("Type", types);
    menu.addItem (renameId, "Rename...");
    menu.addSeparator();
    menu.addItem (loopId, "Loop this section");
    menu.addItem (playheadId, "Move the playhead here");
    menu.addSeparator();
    menu.addItem (deleteId, "Delete section");
    menu.addItem (clearId, "Delete all sections");

    if (onShowGuide != nullptr)
    {
        menu.addSeparator();
        guideicon::addItem (menu, guideId, "Song structure guide...");
    }

    const auto area = juce::Rectangle<float> (timeline.beatToX (section.startBar * 4.0), 0.0f,
                                              (float) (timeline.pixelsPerBeat * 4.0 * section.bars), (float) getHeight());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (area.toNearestInt())),
                        [safeThis = juce::Component::SafePointer<SectionLane> (this), section] (int result)
                        {
                            auto* self = safeThis.getComponent();

                            if (self == nullptr || result == 0)
                                return;

                            auto& edit = self->engine;
                            auto sections = edit.getSections();
                            const auto it = std::find (sections.begin(), sections.end(), section);   // it may have changed meanwhile

                            if (result == guideId && self->onShowGuide != nullptr)
                                self->onShowGuide (section.startBar);
                            else if (result == clearId)
                                edit.setSections ({});
                            else if (it == sections.end())
                                return;
                            else if (result >= 1 && result <= (int) sectionTypes::all.size())
                            {
                                const auto oldType = it->type;
                                it->type = sectionTypes::all[(size_t) result - 1];

                                // A name that only said what it was ("Verse 2") follows the new type.
                                if (it->name.isEmpty() || it->name.upToLastOccurrenceOf (" ", false, false) == sectionTypes::getName (oldType)
                                    || it->name == sectionTypes::getName (oldType))
                                    it->name = {};

                                edit.setSections (std::move (sections));
                            }
                            else if (result == renameId)
                                self->rename ((int) std::distance (sections.begin(), it));
                            else if (result == loopId)
                            {
                                edit.loopStartBeat = section.startBar * 4.0;
                                edit.loopEndBeat = section.getEndBar() * 4.0;
                                edit.loopEnabled = true;
                                edit.sendChangeMessage();
                            }
                            else if (result == playheadId)
                                edit.setPositionBeats (section.startBar * 4.0);
                            else if (result == deleteId)
                            {
                                sections.erase (it);
                                edit.setSections (std::move (sections));
                            }
                        });
}

void SectionLane::rename (int index)
{
    const auto section = engine.getSections()[(size_t) index];
    auto* window = new juce::AlertWindow ("Rename section", "A name for bars " + juce::String (section.startBar + 1) + "-"
                                                                + juce::String (section.getEndBar()) + ", such as Chorus 2 or Hook. "
                                                                "Leave it empty to show the type.",
                                          juce::MessageBoxIconType::NoIcon, this);
    window->addTextEditor ("name", section.name);
    window->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    window->enterModalState (true, juce::ModalCallbackFunction::create ([safeThis = juce::Component::SafePointer<SectionLane> (this), window, section] (int result)
    {
        auto* self = safeThis.getComponent();

        if (self == nullptr || result != 1)
            return;

        auto sections = self->engine.getSections();

        if (auto it = std::find (sections.begin(), sections.end(), section); it != sections.end())
        {
            it->name = window->getTextEditorContents ("name").trim();
            self->engine.setSections (std::move (sections));
        }
    }), true);
}
} // namespace daw
