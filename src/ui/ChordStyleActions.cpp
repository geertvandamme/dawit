#include "ChordStyleActions.h"
#include "generate/ChordStyles.h"
#include "Theme.h"

namespace daw
{
namespace styles = gen::styles;

/** The questions: instrument, style (with what it sounds like) and bars. */
class ChordStylePlayer::Dialog final : public juce::AlertWindow
{
public:
    Dialog (juce::Component& parentComponent, const juce::String& selectedStyleId, const std::vector<BarRange>& ranges)
        : juce::AlertWindow ("Play chords as...",
                             "Plays the chords of the chord track in an instrument's style, on a new MIDI track. "
                             "The notes follow the chords, so change a chord and play again to try another progression.",
                             juce::MessageBoxIconType::NoIcon, &parentComponent)
    {
        juce::StringArray instruments;
        for (auto i : styles::allInstruments)
            instruments.add (styles::getInstrumentName (i));

        addComboBox ("instrument", instruments, "Instrument");
        addComboBox ("style", {}, "Style");

        description.setSize (440, 40);
        description.setJustificationType (juce::Justification::topLeft);
        description.setColour (juce::Label::textColourId, theme::textDim);
        description.setFont (juce::FontOptions (13.0f));
        description.setMinimumHorizontalScale (1.0f);
        addCustomComponent (&description);

        juce::StringArray bars;
        for (auto& range : ranges)
            bars.add (range.label);

        addComboBox ("bars", bars, "Bars");
        getComboBoxComponent ("bars")->setSelectedItemIndex (0);

        addButton ("Play chords", 1, juce::KeyPress (juce::KeyPress::returnKey));
        addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        auto* instrumentBox = getComboBoxComponent ("instrument");
        auto* styleBox = getComboBoxComponent ("style");
        instrumentBox->onChange = [this] { showStyles ({}); };
        styleBox->onChange = [this] { updateDescription(); };

        const auto* selected = styles::findStyle (selectedStyleId);
        const auto instrument = selected != nullptr ? selected->instrument : styles::Instrument::piano;
        instrumentBox->setSelectedItemIndex ((int) std::distance (styles::allInstruments.begin(),
                                                                  std::find (styles::allInstruments.begin(), styles::allInstruments.end(), instrument)),
                                             juce::dontSendNotification);
        showStyles (selectedStyleId);
    }

    juce::String getStyleId() const
    {
        const int index = getComboBoxComponent ("style")->getSelectedItemIndex();
        return index >= 0 && index < (int) shown.size() ? shown[(size_t) index]->id : juce::String();
    }

    int getBarRangeIndex() const { return getComboBoxComponent ("bars")->getSelectedItemIndex(); }

private:
    void showStyles (const juce::String& selectId)
    {
        const int index = juce::jlimit (0, (int) styles::allInstruments.size() - 1, getComboBoxComponent ("instrument")->getSelectedItemIndex());
        shown = styles::getStylesFor (styles::allInstruments[(size_t) index]);

        auto* box = getComboBoxComponent ("style");
        box->clear (juce::dontSendNotification);
        int selected = 0;

        for (size_t i = 0; i < shown.size(); ++i)
        {
            box->addItem (shown[i]->name, (int) i + 1);

            if (shown[i]->id == selectId)
                selected = (int) i;
        }

        box->setSelectedItemIndex (selected, juce::dontSendNotification);
        updateDescription();
    }

    void updateDescription()
    {
        const int index = getComboBoxComponent ("style")->getSelectedItemIndex();
        description.setText (index >= 0 && index < (int) shown.size() ? shown[(size_t) index]->description : juce::String(),
                             juce::dontSendNotification);
    }

    juce::Label description;
    std::vector<const styles::Style*> shown;
};

//==============================================================================
ChordStylePlayer::ChordStylePlayer (Engine& e, juce::Component& p) : engine (e), parent (p) {}

std::vector<ChordStylePlayer::BarRange> ChordStylePlayer::getBarRanges (int clickedBar) const
{
    const auto& chords = engine.getChords();

    if (chords.empty())
        return {};

    auto describe = [] (const juce::String& what, int first, int end)
    {
        return what + " (bar" + (end - first > 1 ? "s " + juce::String (first + 1) + "-" + juce::String (end) : " " + juce::String (first + 1)) + ")";
    };

    const int first = chords.begin()->first, last = chords.rbegin()->first;
    std::vector<BarRange> ranges { { describe ("All chords", first, last + 1), first, last + 1 } };

    if (engine.loopEnabled)
    {
        const int loopFirst = (int) std::floor (engine.loopStartBeat / 4.0 + 1.0e-6);
        const int loopEnd = (int) std::ceil (engine.loopEndBeat / 4.0 - 1.0e-6);

        if (! styles::spansFromChordTrack (chords, loopFirst, loopEnd).empty())
            ranges.push_back ({ describe ("Loop range", loopFirst, loopEnd), loopFirst, loopEnd });
    }

    if (clickedBar > first && clickedBar <= last)
        ranges.push_back ({ describe ("From bar " + juce::String (clickedBar + 1) + " to the last chord", clickedBar, last + 1), clickedBar, last + 1 });

    return ranges;
}

void ChordStylePlayer::show (int clickedBar, int insertIndex)
{
    const auto ranges = getBarRanges (clickedBar);

    if (ranges.empty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Play chords as...",
                                                "The chord track has no chords yet. Click a bar on the chord track to set one, "
                                                "or let the Generate panel write a progression.");
        return;
    }

    auto* dialog = new Dialog (parent, lastStyleId, ranges);

    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, dialog, ranges, insertIndex] (int result)
    {
        const int range = dialog->getBarRangeIndex();

        if (result == 1 && range >= 0 && range < (int) ranges.size())
            play (dialog->getStyleId(), ranges[(size_t) range], insertIndex);
    }), true);
}

void ChordStylePlayer::play (const juce::String& styleId, const BarRange& range, int insertIndex)
{
    const auto* style = styles::findStyle (styleId);

    if (style == nullptr)
        return;

    lastStyleId = styleId;
    const double lengthBeats = (range.endBar - range.firstBar) * 4.0;
    auto notes = styles::play (*style, styles::spansFromChordTrack (engine.getChords(), range.firstBar, range.endBar),
                               lengthBeats, engine.getTempo());

    if (notes.empty())
        return;

    auto* clip = styles::addTrack (engine, *style, std::move (notes), range.firstBar * 4.0, lengthBeats, insertIndex);

    if (auto* track = engine.findTrackFor (clip); track != nullptr && onPlayed)
        onPlayed (*track, *clip);
}
} // namespace daw
