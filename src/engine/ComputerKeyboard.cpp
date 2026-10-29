#include "ComputerKeyboard.h"

#include <cctype>

namespace daw
{
namespace
{
    constexpr const char* letters = "awsedftgyhujkolp";

    char lower (char c) noexcept     { return (char) std::tolower ((unsigned char) c); }
}

char ComputerKeyboard::getLetter (int index)
{
    return juce::isPositiveAndBelow (index, numKeys) ? letters[index] : '\0';
}

int ComputerKeyboard::getKeyIndex (char letter)
{
    const auto c = lower (letter);

    for (int i = 0; i < numKeys; ++i)
        if (letters[i] == c)
            return i;

    return -1;
}

bool ComputerKeyboard::isBlackKey (int index)
{
    const int pitchClass = index % 12;
    return pitchClass == 1 || pitchClass == 3 || pitchClass == 6 || pitchClass == 8 || pitchClass == 10;
}

bool ComputerKeyboard::isMapped (char letter)
{
    const auto c = lower (letter);
    return getKeyIndex (c) >= 0 || c == 'z' || c == 'x' || c == 'c' || c == 'v';
}

ComputerKeyboard::Press ComputerKeyboard::press (char letter)
{
    const auto c = lower (letter);

    switch (c)
    {
        case 'z': moveOctave (-1); return { true, std::nullopt };
        case 'x': moveOctave (1);  return { true, std::nullopt };
        case 'c': moveVelocity (-1); return { true, std::nullopt };
        case 'v': moveVelocity (1);  return { true, std::nullopt };
        default: break;
    }

    const int index = getKeyIndex (c);

    if (index < 0)
        return {};

    const int note = getNote (index);

    if (note < 0 || held.count (c) > 0)
        return { true, std::nullopt };

    held[c] = note;
    return { true, juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity) };
}

std::optional<juce::MidiMessage> ComputerKeyboard::release (char letter)
{
    const auto it = held.find (lower (letter));

    if (it == held.end())
        return std::nullopt;

    const auto message = juce::MidiMessage::noteOff (1, it->second);
    held.erase (it);
    return message;
}

std::vector<juce::MidiMessage> ComputerKeyboard::releaseAll()
{
    std::vector<juce::MidiMessage> messages;

    // In key order, so the result doesn't depend on the order the keys went down.
    for (int i = 0; i < numKeys; ++i)
        if (const auto message = release (letters[i]))
            messages.push_back (*message);

    held.clear();
    return messages;
}

bool ComputerKeyboard::isHeld (char letter) const      { return held.count (lower (letter)) > 0; }
bool ComputerKeyboard::isKeyHeld (int index) const     { return isHeld (getLetter (index)); }

std::string ComputerKeyboard::getHeldLetters() const
{
    std::string result;

    for (int i = 0; i < numKeys; ++i)
        if (isKeyHeld (i))
            result += letters[i];

    return result;
}

void ComputerKeyboard::setOctaveNote (int note) noexcept
{
    const int clamped = juce::jlimit (lowestOctaveNote, highestOctaveNote, note);
    octaveNote = ((clamped + 6) / 12) * 12;
}

bool ComputerKeyboard::moveOctave (int steps) noexcept
{
    const int before = octaveNote;
    setOctaveNote (octaveNote + 12 * steps);
    return octaveNote != before;
}

void ComputerKeyboard::setVelocity (int newVelocity) noexcept
{
    int best = velocitySteps[0];

    for (const int step : velocitySteps)
        if (std::abs (step - newVelocity) <= std::abs (best - newVelocity))   // <= : a tie goes up
            best = step;

    velocity = best;
}

bool ComputerKeyboard::moveVelocity (int steps) noexcept
{
    int position = 0;

    for (int i = 0; i < (int) velocitySteps.size(); ++i)
        if (velocitySteps[(size_t) i] == velocity)
            position = i;

    const int target = juce::jlimit (0, (int) velocitySteps.size() - 1, position + steps);
    const bool moved = target != position;
    velocity = velocitySteps[(size_t) target];
    return moved;
}

int ComputerKeyboard::getNote (int index) const noexcept
{
    const int note = octaveNote + index;
    return juce::isPositiveAndBelow (index, numKeys) && note <= 127 ? note : -1;
}

std::string ComputerKeyboard::getSilencedLetters (int keyPitchClasses) const
{
    std::string result;

    for (int i = 0; i < numKeys; ++i)
    {
        const int note = getNote (i);

        if (note >= 0 && (keyPitchClasses & (1 << (note % 12))) == 0)
            result += letters[i];
    }

    return result;
}
} // namespace daw
