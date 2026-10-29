#include "Tuner.h"
#include "PitchTracker.h"

namespace daw::tuner
{
InputTap::InputTap()
{
    storage.assign ((size_t) capacity, 0.0f);
    mix.assign (16384, 0.0f);
}

void InputTap::setListening (bool on)
{
    if (on)
        fifo.reset();   // safe: the audio thread isn't writing while listening is off

    listening = on;
}

void InputTap::setInput (int firstChannel, bool isStereo) noexcept
{
    channel = juce::jmax (0, firstChannel);
    stereo = isStereo;
}

void InputTap::push (const float* const* inputs, int numInputs, int numSamples) noexcept
{
    if (! listening.load() || inputs == nullptr || numInputs <= 0)
        return;

    const int left = channel.load() < numInputs ? channel.load() : 0;
    const int right = stereo.load() && left + 1 < numInputs ? left + 1 : left;

    if (inputs[left] == nullptr || inputs[right] == nullptr)
        return;

    for (int done = 0; done < numSamples;)
    {
        const int n = juce::jmin ((int) mix.size(), numSamples - done);

        for (int i = 0; i < n; ++i)
            mix[(size_t) i] = right == left ? inputs[left][done + i] : 0.5f * (inputs[left][done + i] + inputs[right][done + i]);

        const auto scope = fifo.write (n);
        const int first = scope.blockSize1, second = scope.blockSize2;

        if (first + second < n)
            return;   // full: the window isn't draining; drop the rest

        std::copy_n (mix.begin(), first, storage.begin() + scope.startIndex1);
        std::copy_n (mix.begin() + first, second, storage.begin() + scope.startIndex2);
        done += n;
    }
}

int InputTap::pull (float* destination, int maxSamples) noexcept
{
    const auto scope = fifo.read (juce::jmin (maxSamples, fifo.getNumReady()));
    std::copy_n (storage.begin() + scope.startIndex1, scope.blockSize1, destination);
    std::copy_n (storage.begin() + scope.startIndex2, scope.blockSize2, destination + scope.blockSize1);
    return scope.blockSize1 + scope.blockSize2;
}

//==============================================================================
Listener::Listener() : tracker (std::make_unique<pitch::Tracker>()) {}
Listener::~Listener() = default;

void Listener::prepare (double sampleRate)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    windowSize = pitch::getTunerWindowSize (rate);
    hop = 1024;
    ring.assign ((size_t) windowSize, 0.0f);
    ordered.assign ((size_t) windowSize, 0.0f);
    pullBuffer.assign (8192, 0.0f);
    tracker->prepare (rate, windowSize, lowestHz);
    reset();
}

void Listener::reset()
{
    std::fill (ring.begin(), ring.end(), 0.0f);
    filled = writePos = sinceAnalysis = 0;
    samplesSinceVoiced = zeroRun = 0;
    numRecent = nextRecent = 0;
    numBlocks = nextBlock = 0;
    tracker->reset();
    reading = {};
}

int Listener::service (InputTap& tap)
{
    int total = 0;

    for (;;)
    {
        const int n = tap.pull (pullBuffer.data(), (int) pullBuffer.size());

        if (n <= 0)
            break;

        feed (pullBuffer.data(), n);
        total += n;
    }

    return total;
}

void Listener::feed (const float* samples, int numSamples)
{
    if (windowSize == 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        ring[(size_t) writePos] = samples[i];
        writePos = (writePos + 1) % windowSize;
        filled = juce::jmin (windowSize, filled + 1);
        zeroRun = samples[i] == 0.0f ? zeroRun + 1 : 0;
        ++samplesSinceVoiced;

        if (++sinceAnalysis >= hop)
        {
            sinceAnalysis = 0;
            noteBlockLevel();

            if (filled == windowSize)
                analyseWindow();
        }
    }

    reading.exactlyZero = (double) zeroRun >= rate;

    // The age of a held note follows the samples, so a test needs no clock.
    if (reading.hasNote && samplesSinceVoiced > hop)
    {
        reading.ageSeconds = (double) samplesSinceVoiced / rate;

        if (reading.ageSeconds >= holdSeconds)
        {
            reading.hasNote = false;
            numRecent = nextRecent = 0;
        }
        else
            reading.fade = (float) juce::jlimit (0.0, 1.0, (holdSeconds - reading.ageSeconds) / (holdSeconds - 0.5));
    }
}

void Listener::noteBlockLevel()
{
    double energy = 0.0;

    for (int i = 1; i <= hop; ++i)
    {
        const float v = ring[(size_t) ((writePos - i + windowSize) % windowSize)];
        energy += (double) v * v;
    }

    blockDb[(size_t) nextBlock] = juce::Decibels::gainToDecibels ((float) std::sqrt (energy / hop), -100.0f);
    nextBlock = (nextBlock + 1) % (int) blockDb.size();
    numBlocks = juce::jmin ((int) blockDb.size(), numBlocks + 1);
}

/** Two notes a semitone or so apart make a sound that one window takes for a note in between, but it swells and fades
    a few times a second. A dip of 6 dB or more that is short (3 hops at most) and comes back up is such a beat. */
bool Listener::isBeating() const
{
    const int size = (int) blockDb.size();
    const auto at = [&] (int i) { return blockDb[(size_t) ((nextBlock - numBlocks + i + 2 * size) % size)]; };
    float before = -200.0f;

    for (int i = 0; i < numBlocks; ++i)
    {
        if (at (i) <= before - 6.0f)
        {
            for (int width = 1; width <= 3 && i + width < numBlocks; ++width)
                if (at (i + width) >= at (i) + 6.0f && at (i + width) >= before - 6.0f)
                {
                    bool deep = true;

                    for (int k = i; k < i + width; ++k)
                        deep = deep && at (k) <= before - 6.0f;

                    if (deep)
                        return true;
                }
        }

        before = juce::jmax (before, at (i));
    }

    return false;
}

void Listener::analyseWindow()
{
    const int tail = windowSize - writePos;
    std::copy_n (ring.begin() + writePos, tail, ordered.begin());
    std::copy_n (ring.begin(), writePos, ordered.begin() + tail);

    const auto r = tracker->analyse (ordered.data(), windowSize, pitch::Tracker::minClarity, true);
    reading.levelDb = r.rmsDb;
    reading.tooQuiet = r.rmsDb < pitch::Tracker::minRmsDb;

    if (r.voiced && isBeating())
    {
        // Seen beating: not a note. What was held goes too.
        reading.hasNote = false;
        numRecent = nextRecent = 0;
        return;
    }

    if (! r.voiced)
        return;

    recent[(size_t) nextRecent] = r.midi;
    nextRecent = (nextRecent + 1) % (int) recent.size();
    numRecent = juce::jmin ((int) recent.size(), numRecent + 1);

    auto sorted = recent;
    std::sort (sorted.begin(), sorted.begin() + numRecent);
    reading.midi = (double) sorted[(size_t) (numRecent / 2)];
    reading.hz = pitch::hzFromMidi (reading.midi);
    reading.hasNote = true;
    reading.ageSeconds = 0.0;
    reading.fade = 1.0f;
    samplesSinceVoiced = 0;
}
}
