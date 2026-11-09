#include "GuideNavigator.h"

namespace daw
{
namespace guides = gen::guides;

namespace
{
    GuideNavigator* instance = nullptr;
    const char* const readKey = "guides.read";

    juce::String capitalise (const juce::String& s)
    {
        return s.isEmpty() ? s : s.substring (0, 1).toUpperCase() + s.substring (1);
    }
}

GuideNavigator::GuideNavigator (juce::PropertiesFile* s, std::function<Entries()> make, std::function<void (guides::Guide, int)> open)
    : settings (s), makeEntries (std::move (make)), openGuide (std::move (open))
{
    instance = this;
}

GuideNavigator::~GuideNavigator()
{
    if (instance == this)
        instance = nullptr;
}

GuideNavigator* GuideNavigator::getInstance() noexcept
{
    return instance;
}

const GuideNavigator::Entries& GuideNavigator::getEntries()
{
    if (! entries.has_value())
        entries = makeEntries != nullptr ? makeEntries() : guides::getEntries();

    return *entries;
}

const guides::Entry* GuideNavigator::findEntry (guides::Guide guide)
{
    for (auto& e : getEntries())
        if (e.guide == guide)
            return &e;

    return nullptr;
}

juce::String GuideNavigator::getShortName (const guides::Entry& entry)
{
    auto name = entry.name;

    if (name.contains ("?"))
        name = name.upToFirstOccurrenceOf ("?", false, false);

    for (auto prefix : { "What is ", "What are " })
        if (name.startsWith (prefix))
            name = name.fromFirstOccurrenceOf (prefix, false, false);

    for (auto article : { "a ", "an ", "the " })
        if (name.startsWith (article))
            name = name.fromFirstOccurrenceOf (article, false, false);

    return capitalise (name.trim());
}

std::vector<GuideNavigator::Link> GuideNavigator::getLinks (guides::Guide guide, const juce::String& topic)
{
    std::vector<Link> links;
    const auto* entry = findEntry (guide);
    const bool isFirst = entry != nullptr && ! entry->topics.isEmpty() && entry->topics[0] == topic;

    for (auto& ref : guides::getLinksFor (guide, topic, isFirst))
    {
        const auto* other = findEntry (ref.guide);

        if (other == nullptr)
            continue;

        const auto page = ref.topic.isEmpty() && ! other->topics.isEmpty() ? other->topics[0] : ref.topic;
        links.push_back ({ ref, getShortName (*other) + juce::String::fromUTF8 (" \xc2\xb7 ") + page,
                           "Opens " + other->name + " at " + page + ". Back there brings you here." });
    }

    return links;
}

void GuideNavigator::jumpTo (const Place& from, guides::Guide guide, int topic)
{
    history.push_back (from);

    if ((int) history.size() > maxHistory)
        history.erase (history.begin());

    lastJumpGuide = guide;

    if (openGuide != nullptr)
        openGuide (guide, topic);

    sendChangeMessage();
}

void GuideNavigator::follow (guides::Guide fromGuide, const juce::String& fromTopic, const guides::PageRef& to)
{
    const auto* entry = findEntry (to.guide);
    const int index = entry != nullptr ? juce::jmax (0, entry->topics.indexOf (to.topic)) : 0;
    jumpTo ({ fromGuide, fromTopic }, to.guide, to.topic.isEmpty() ? 0 : index);
}

bool GuideNavigator::followNthLink (int n)
{
    if (! shown.has_value())
        return false;

    const auto links = getLinks (shown->guide, shown->topic);

    if (! juce::isPositiveAndBelow (n - 1, (int) links.size()))
        return false;

    follow (shown->guide, shown->topic, links[(size_t) (n - 1)].to);
    return true;
}

std::optional<GuideNavigator::BackTarget> GuideNavigator::getBack (guides::Guide shownGuide)
{
    if (history.empty() || shownGuide != lastJumpGuide)
        return std::nullopt;

    const auto& place = history.back();
    BackTarget target { place.guide, place.topic, {} };

    if (const auto* e = findEntry (place.guide))
        target.label = "Back to " + e->name + juce::String::fromUTF8 (" \xc2\xb7 ") + place.topic;

    return target;
}

void GuideNavigator::goBack()
{
    if (history.empty())
        return;

    const auto place = history.back();
    history.pop_back();

    if (const auto* entry = findEntry (place.guide))
    {
        lastJumpGuide = place.guide;

        if (openGuide != nullptr)
            openGuide (place.guide, juce::jmax (0, entry->topics.indexOf (place.topic)));
    }

    sendChangeMessage();
}

std::optional<GuideNavigator::NextTarget> GuideNavigator::getNext (guides::Guide guide, const juce::String& topic)
{
    const auto* entry = findEntry (guide);

    if (entry == nullptr)
        return std::nullopt;

    const int index = entry->topics.indexOf (topic);

    if (index < 0)
        return std::nullopt;

    const auto next = guides::getNextPage (getEntries(), guide, index);

    if (! next.has_value())
        return std::nullopt;

    const auto* target = findEntry (next->guide);

    if (target == nullptr)
        return std::nullopt;

    NextTarget result { next->guide, next->topic, {}, next->guide != guide };
    result.label = result.otherGuide ? "Next: " + target->name
                                     : "Next page: " + (juce::isPositiveAndBelow (next->topic, target->topics.size()) ? target->topics[next->topic] : juce::String());
    return result;
}

void GuideNavigator::goNext (guides::Guide guide, const juce::String& topic)
{
    const auto next = getNext (guide, topic);

    if (! next.has_value())
        return;

    setRead (guide, topic, true);

    if (next->otherGuide)
    {
        jumpTo ({ guide, topic }, next->guide, next->topic);
    }
    else if (openGuide != nullptr)
    {
        openGuide (next->guide, next->topic);
        sendChangeMessage();
    }
}

bool GuideNavigator::goNextFromShown()
{
    if (! shown.has_value() || ! getNext (shown->guide, shown->topic).has_value())
        return false;

    goNext (shown->guide, shown->topic);
    return true;
}

juce::StringArray GuideNavigator::getReadIds() const
{
    juce::StringArray ids;

    if (settings != nullptr)
        ids.addLines (settings->getValue (readKey));

    ids.removeEmptyStrings();
    return ids;
}

bool GuideNavigator::isRead (guides::Guide guide, const juce::String& topic) const
{
    return getReadIds().contains (guides::getPageId (guide, topic));
}

void GuideNavigator::saveReadIds (const juce::StringArray& ids)
{
    if (settings == nullptr)
        return;

    settings->setValue (readKey, ids.joinIntoString ("\n"));
    sendChangeMessage();
}

void GuideNavigator::setRead (guides::Guide guide, const juce::String& topic, bool read)
{
    if (! guides::isCountedPage (topic))
        return;

    auto ids = getReadIds();
    const auto id = guides::getPageId (guide, topic);

    if (read == ids.contains (id))
        return;

    if (read)
        ids.add (id);
    else
        ids.removeString (id);

    saveReadIds (ids);
}

void GuideNavigator::markAllUnread()
{
    saveReadIds ({});
}

void GuideNavigator::setShown (guides::Guide guide, const juce::String& topic)
{
    shown = Place { guide, topic };
}
} // namespace daw
