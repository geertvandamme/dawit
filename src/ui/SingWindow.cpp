#include "SingWindow.h"
#include "LyricsLane.h"
#include "SectionLane.h"
#include "engine/VoiceInstall.h"

namespace daw
{
namespace st = gen::singthis;

namespace
{
    const char* const hanamiPage = "https://lottev.moe/hoshino-hanami-ai%e2%9d%a4dol-for-diffsinger-v1-0-is-out/";
    const char* const voiceWiki = "https://diffsinger.miraheze.org/wiki/Category:Voicebanks_supporting_English";

    juce::String bare (const juce::String& symbol)
    {
        return symbol.fromLastOccurrenceOf ("/", false, false);
    }

    juce::String dot() { return juce::String::fromUTF8 (" \xc2\xb7 "); }

    /** A line of wrapped text, measured for the width. */
    juce::TextLayout layoutText (const juce::String& text, float width, float fontSize, juce::Colour colour, bool bold = false)
    {
        juce::AttributedString s;
        s.append (text, juce::Font (juce::FontOptions (fontSize, bold ? juce::Font::bold : juce::Font::plain)), colour);
        s.setJustification (juce::Justification::topLeft);
        juce::TextLayout layout;
        layout.createLayout (s, width);
        return layout;
    }

    /** Runs the unpacking of a voice behind a progress window with a Cancel button. */
    class InstallJob final : public juce::ThreadWithProgressWindow
    {
    public:
        InstallJob (const juce::File& s, juce::Component* parent, std::function<void (InstallJob&, bool)> done)
            : juce::ThreadWithProgressWindow ("Adding a voice", true, true, 30000, "Cancel", parent), source (s), onDone (std::move (done))
        {
            setStatusMessage ("Unpacking " + s.getFileName() + "...\nA voice is a few hundred MB, so this can take a minute.");
        }

        void run() override
        {
            result = sing::installVoice (source, sing::getVoicesFolder(), [this] (double done)
            {
                setProgress (done);
                return ! threadShouldExit();
            });
        }

        void threadComplete (bool cancelled) override { onDone (*this, cancelled); }

        juce::File source;
        sing::InstallResult result;
        std::function<void (InstallJob&, bool)> onDone;
    };
}

//==============================================================================
/** The notes of the melody, each with the syllable it sings and its sounds under it. */
class SingWindow::Picture final : public juce::Component,
                                  public juce::TooltipClient
{
public:
    explicit Picture (Engine& e) : engine (e) {}

    void setData (const st::Melody* m, const st::Pairing* p)
    {
        melody = m;
        pairing = p;
        repaint();
    }

    int getContentWidth (int viewWidth) const
    {
        const double beats = melody != nullptr && ! melody->notes.empty() ? melody->getEndBeat() - melody->getStartBeat() : 8.0;
        return juce::jmax (viewWidth, (int) (beats * pixelsPerBeat) + 2 * margin);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::panelLight);
        g.fillRect (getLocalBounds());

        if (melody == nullptr || melody->notes.empty())
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (14.0f));
            g.drawText ("This melody has no notes to sing.", getLocalBounds(), juce::Justification::centred);
            return;
        }

        const double first = melody->getStartBeat(), last = melody->getEndBeat();

        // Bars and beats.
        g.setFont (juce::FontOptions (11.0f));

        for (int b = (int) std::floor (first / 4.0) * 4; b <= (int) std::ceil (last); ++b)
        {
            const float x = xOf ((double) b);
            const bool bar = b % 4 == 0;
            g.setColour (bar ? theme::gridBar : theme::grid);
            g.fillRect (x, (float) topStrip, 1.0f, (float) getHeight() - (float) topStrip);

            if (bar)
            {
                g.setColour (theme::textDim);
                g.drawText ("Bar " + juce::String (st::getBar ((double) b)), juce::Rectangle<float> (x + 3.0f, (float) topStrip + 1.0f, 60.0f, 14.0f),
                            juce::Justification::centredLeft, false);
            }
        }

        int low = 127, high = 0;

        for (auto& n : melody->notes)
        {
            low = juce::jmin (low, n.pitch);
            high = juce::jmax (high, n.pitch);
        }

        const float step = juce::jlimit (4.0f, 10.0f, (float) notesHeight / (float) (high - low + 1));
        const float notesTop = (float) topStrip + 18.0f;

        for (size_t i = 0; i < melody->notes.size(); ++i)
        {
            const auto& n = melody->notes[i];
            const auto& a = pairing->notes[i];
            const auto box = juce::Rectangle<float> (xOf (n.startBeat) + 1.0f, notesTop + (float) (high - n.pitch) * step,
                                                     juce::jmax (4.0f, xOf (n.endBeat) - xOf (n.startBeat) - 2.0f), step);
            g.setColour (a.syllable < 0 ? theme::textDim.withAlpha (0.45f) : a.starts ? theme::accent : theme::accent.withAlpha (0.55f));
            g.fillRoundedRectangle (box, 2.0f);

            const float textY = notesTop + (float) notesHeight + 14.0f;
            const float nextX = i + 1 < melody->notes.size() ? xOf (melody->notes[i + 1].startBeat) : box.getRight() + 40.0f;
            const float room = juce::jmax (8.0f, nextX - box.getX() - 3.0f);

            if (a.syllable < 0)
                continue;

            if (! a.starts)
            {
                // Held: a line from the syllable's note on.
                g.setColour (theme::textDim);
                g.fillRect (box.getX(), textY + 9.0f, box.getWidth(), 1.5f);
                continue;
            }

            g.setColour (theme::text);
            g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
            g.drawText (pairing->syllables[(size_t) a.syllable].text, juce::Rectangle<float> (box.getX(), textY, room, 18.0f),
                        juce::Justification::centredLeft, true);

            // Its sounds below: vowels bright, consonants dim.
            const auto& sounds = pairing->sounds[(size_t) a.syllable];
            float x = box.getX();
            g.setFont (juce::FontOptions (12.0f));

            for (auto* group : { &sounds.onset, &sounds.nucleus, &sounds.coda })
            {
                for (auto& s : *group)
                {
                    const auto name = bare (s.symbol);
                    const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), name) + 6.0f;

                    if (x + w > box.getX() + room)
                        break;

                    g.setColour (s.vowel ? SectionLane::getColour (SectionType::chorus).withAlpha (0.9f) : theme::textDim.withAlpha (0.35f));
                    g.fillRoundedRectangle (x, textY + 20.0f, w - 1.0f, 15.0f, 2.0f);
                    g.setColour (theme::text);
                    g.drawText (name, juce::Rectangle<float> (x, textY + 20.0f, w - 1.0f, 15.0f), juce::Justification::centred, false);
                    x += w;
                }
            }
        }

        // Syllables that are left out, red above the grid.
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.setColour (theme::meterRed);

        for (int s : pairing->unpairedSyllables)
            g.drawText (pairing->syllables[(size_t) s].text, juce::Rectangle<float> (xOf (pairing->syllables[(size_t) s].beat), 2.0f, 80.0f, 16.0f),
                        juce::Justification::centredLeft, false);

        if (pairing->mode == st::Mode::onePerNote && pairing->leftOverSyllables > 0)
        {
            g.drawText ("+" + juce::String (pairing->leftOverSyllables) + " syllables not sung", juce::Rectangle<float> (xOf (last) + 6.0f, 2.0f, 200.0f, 16.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = findNote (e.x);

        if (i >= 0)
            engine.previewNote (melody->notes[(size_t) i].pitch, 0.8f, 450);
    }

    juce::String getTooltip() override
    {
        const int i = findNote (getMouseXYRelative().x);
        return i >= 0 ? st::describeNote (*melody, *pairing, (size_t) i)
                      : juce::String ("The melody with the syllable each note sings and its sounds below. Click a note to hear it.");
    }

    static constexpr int topStrip = 20, notesHeight = 96;
    static constexpr double pixelsPerBeat = 56.0;
    static constexpr int margin = 24;

private:
    float xOf (double beat) const { return (float) margin + (float) ((beat - melody->getStartBeat()) * pixelsPerBeat); }

    int findNote (int x) const
    {
        if (melody == nullptr)
            return -1;

        for (size_t i = 0; i < melody->notes.size(); ++i)
            if ((float) x >= xOf (melody->notes[i].startBeat) && (float) x < xOf (melody->notes[i].endBeat))
                return (int) i;

        return -1;
    }

    Engine& engine;
    const st::Melody* melody = nullptr;
    const st::Pairing* pairing = nullptr;
};

//==============================================================================
/** The advice, one line per rule, each with a coloured dot. */
class SingWindow::AdviceView final : public juce::Component
{
public:
    void setAdvice (const std::vector<st::Advice>& a, int viewWidth)
    {
        advice = a;
        setSize (viewWidth, getHeightFor (viewWidth));
        repaint();
    }

    int getHeightFor (int w) const
    {
        int h = 6;

        for (auto& a : advice)
            h += (int) std::ceil (layoutText (a.text, (float) w - 40.0f, 13.5f, theme::text).getHeight()) + 8;

        return juce::jmax (30, h);
    }

    void paint (juce::Graphics& g) override
    {
        int y = 6;

        for (auto& a : advice)
        {
            const auto colour = a.blocksSinging ? theme::meterRed : a.rule == st::Rule::allGood ? theme::meterGreen
                                                 : a.rule == st::Rule::melisma ? theme::accent : theme::meterYellow;
            auto layout = layoutText (a.text, (float) getWidth() - 40.0f, 13.5f, theme::text);
            g.setColour (colour);
            g.fillEllipse (12.0f, (float) y + 4.0f, 9.0f, 9.0f);
            layout.draw (g, juce::Rectangle<float> (30.0f, (float) y, (float) getWidth() - 40.0f, layout.getHeight() + 2.0f));
            y += (int) std::ceil (layout.getHeight()) + 8;
        }
    }

private:
    std::vector<st::Advice> advice;
};

//==============================================================================
/** What to do without a voice, or when the chosen one can't be used: the steps, with this computer's folder. */
class SingWindow::HelpView final : public juce::Component
{
public:
    /** `error` is why the voice can't be used; empty when there is no voice at all. */
    void setState (const juce::String& error)
    {
        problem = error;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (theme::panelLight);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (problem.isEmpty() ? theme::accent : theme::meterRed);
        g.fillRoundedRectangle (r.removeFromLeft (4.0f), 2.0f);
        r.reduce (12.0f, 8.0f);

        auto head = layoutText (problem.isEmpty() ? "Add a voice to sing this" : "This voice can't be used yet", r.getWidth(), 15.0f, theme::text, true);
        head.draw (g, r.removeFromTop (head.getHeight() + 4.0f));

        juce::String body;

        if (problem.isNotEmpty())
            body << problem << "\n\nWhat to do: ";

        body << "1. Get a voice: click Get a voice... It offers the page of Hoshino Hanami, the voice DawIt suggests (a .zip file, about 570 MB unpacked), "
                "and a list of other English voices.\n"
                "2. Add it: click Add a voice... and pick the .zip (or the folder you unpacked). DawIt copies it into its Voices folder, "
             << sing::getVoicesFolder().getFullPathName() << ", and checks it. You can also put the folder there yourself.\n"
                "3. Check: the voice's name shows on the Voice button and Sing lights up. If a file is missing, DawIt names it.\n"
                "Each voice has its own terms: read them with Terms... and credit the maker.";

        auto layout = layoutText (body, r.getWidth(), 13.0f, theme::textDim);
        layout.draw (g, r);
    }

private:
    juce::String problem;
};

//==============================================================================
SingWindow::SingWindow (Engine& e, Track* t, MidiClip* c) : engine (e), track (t), clip (c)
{
    auto* settings = engine.properties.getUserSettings();

    for (auto* label : { &voiceLabel, &melodyLabel, &wordsLabel, &pitchLabel, &qualityLabel })
    {
        label->setFont (juce::FontOptions (13.0f));
        label->setColour (juce::Label::textColourId, theme::textDim);
        addAndMakeVisible (*label);
    }

    voiceLabel.setText ("Voice", juce::dontSendNotification);
    melodyLabel.setText ("Melody", juce::dontSendNotification);
    wordsLabel.setText ("Words", juce::dontSendNotification);
    pitchLabel.setText ("Pitch", juce::dontSendNotification);
    qualityLabel.setText ("Quality", juce::dontSendNotification);

    title.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    addAndMakeVisible (title);

    credit.setFont (juce::FontOptions (12.5f));
    credit.setColour (juce::Label::textColourId, theme::textDim);
    credit.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (credit);

    hint.setFont (juce::FontOptions (13.0f));
    hint.setColour (juce::Label::textColourId, theme::meterYellow);
    addAndMakeVisible (hint);

    for (auto* b : std::initializer_list<juce::TextButton*> { &helpButton, &voiceButton, &addButton, &getButton, &termsButton, &singButton, &cancelButton, &stepsButton, &matchButton, &makeTrackButton, &useMineButton })
        addAndMakeVisible (*b);

    matchLabel.setFont (juce::FontOptions (13.0f));
    matchLabel.setColour (juce::Label::textColourId, theme::text);
    matchLabel.setMinimumHorizontalScale (0.8f);
    addAndMakeVisible (matchLabel);

    for (auto* box : { &melodyBox, &wordsBox, &pitchBox, &qualityBox })
        addAndMakeVisible (*box);

    for (auto* tb : { &thisClip, &wholeTrack, &breaths, &muteMelody })
        addAndMakeVisible (*tb);

    thisClip.setRadioGroupId (41);
    wholeTrack.setRadioGroupId (41);

    matchMethodBox.addItem ("Match to the words' timing", 1);
    matchMethodBox.addItem ("Match by merging and splitting", 2);
    matchMethodBox.addItem ("Notes as long as the melody's", 3);
    matchMethodBox.setSelectedId (juce::jlimit (1, 3, engine.properties.getUserSettings()->getIntValue ("sing.matchMethod", 0) + 1), juce::dontSendNotification);
    addChildComponent (matchMethodBox);
    wordsBox.addItem ("As timed", 1);
    wordsBox.addItem ("One syllable per note", 2);
    pitchBox.addItem ("Natural", 1);
    pitchBox.addItem ("Exact notes", 2);
    qualityBox.addItem ("Draft", 1);
    qualityBox.addItem ("Good", 2);
    qualityBox.addItem ("Best", 3);
    pitchBox.setSelectedId (juce::jlimit (1, 2, settings->getIntValue ("sing.pitch", 0) + 1), juce::dontSendNotification);
    qualityBox.setSelectedId (juce::jlimit (1, 3, settings->getIntValue ("sing.quality", 1) + 1), juce::dontSendNotification);
    breaths.setToggleState (settings->getBoolValue ("sing.breaths", true), juce::dontSendNotification);
    muteMelody.setToggleState (settings->getBoolValue ("sing.muteMelody", true), juce::dontSendNotification);
    voiceName = settings->getValue ("sing.voice");

    // Tooltips.
    voiceButton.setTooltip ("The voice that sings: a model trained on recordings of one singer. Add voices with Add a voice...");
    addButton.setTooltip ("Add a DiffSinger voicebank you downloaded (a .zip or a folder), or a vocoder (.oudep) that a voice needs.");
    getButton.setTooltip ("Where to get a voice: the page of the one DawIt suggests, a list of English DiffSinger voices, or the steps to add one.");
    termsButton.setTooltip ("The voice maker's terms. Many voices ask for credit; some ask permission or a fee before you release music.");
    credit.setTooltip ("Who made the voice. Credit them when you release a song with it; the vocoder's licence is shown too when it has one.");
    melodyBox.setTooltip ("The MIDI track whose notes are sung, one note at a time.");
    thisClip.setTooltip ("Sing only the clip you picked, or every clip on the melody track.");
    wholeTrack.setTooltip ("Sing only the clip you picked, or every clip on the melody track.");
    wordsBox.setTooltip ("As timed: each note sings the syllable that starts on it, and a note inside a syllable holds it. "
                         "One syllable per note: the syllables in order, one on each note, whatever their timing.");
    pitchBox.setTooltip ("Natural: the voice adds its own slides and vibrato. Exact notes: the pitch stays on each note, with a short slide between notes.");
    breaths.setTooltip ("Adds a short breath before a phrase that follows a rest of half a second or more.");
    qualityBox.setTooltip ("How many steps the voice takes to paint its sound. Draft is quickest; Best is slowest and can sound cleaner.");
    muteMelody.setTooltip ("Mutes the melody's own instrument so you hear the voice alone. The notes stay.");
    matchMethodBox.setTooltip ("How the notes are matched to the words. Match to the words' timing: one note starts on each syllable, with the pitch of the melody note sounding there, "
                               "and lasts until just before the next syllable (legato, so the voice sings smoothly through the words), but not across a rest of half a second or more. "
                               "Match by merging and splitting: the shortest notes are merged into the note before them, or the longest are split in two, so the melody keeps its own "
                               "rhythm, but new note starts can land away from the words' times. Notes as long as the melody's: the same note starts as the first choice, but each note stops "
                               "where its melody note stops (staccato, with gaps where the melody had them), so notes can be much shorter than the words.");
    matchButton.setTooltip ("Makes the number of notes the same as the number of syllables, so each note sings one, using the way chosen next to it. "
                            "Your clip is not changed; press Sing to sing the adjusted notes.");
    matchLabel.setTooltip ("What matching the notes to the words does to the melody.");
    makeTrackButton.setTooltip ("Puts the adjusted notes in a new MIDI clip on a new track after the melody's track, so you can keep and edit them. Your own clip stays as it is.");
    useMineButton.setTooltip ("Goes back to singing your own notes as they are.");
    singButton.setTooltip ("Renders the vocal to a new audio track, on this computer.");
    cancelButton.setTooltip ("Closes this window without singing.");
    helpButton.setTooltip ("How a computer sings: phonemes, notes and voices.");
    stepsButton.setTooltip ("Where to get a voice, how to add it, what its terms mean and what to do when it doesn't work.");

    picture = std::make_unique<Picture> (engine);
    pictureViewport.setViewedComponent (picture.get(), false);
    pictureViewport.setScrollBarsShown (false, true);
    addAndMakeVisible (pictureViewport);

    adviceView = std::make_unique<AdviceView>();
    adviceViewport.setViewedComponent (adviceView.get(), false);
    adviceViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (adviceViewport);

    helpView = std::make_unique<HelpView>();
    addChildComponent (*helpView);

    helpButton.onClick = [this] { openGuide (gen::lyricsguide::Topic::computer); };
    stepsButton.onClick = [this] { openGuide (gen::lyricsguide::Topic::voices); };
    voiceButton.onClick = [this] { showVoiceMenu(); };
    addButton.onClick = [this] { showAddMenu(); };
    getButton.onClick = [this] { showGetMenu(); };
    termsButton.onClick = [this]
    {
        if (voice.has_value() && voice->files.terms.existsAsFile())
            voice->files.terms.startAsProcess();
    };

    melodyBox.onChange = [this]
    {
        const int index = melodyBox.getSelectedId() - 1;

        if (index >= 0 && index < (int) melodyTracks.size() && melodyTracks[(size_t) index] != track)
        {
            track = melodyTracks[(size_t) index];
            clip = nullptr;
            matched = false;
            wholeTrack.setToggleState (true, juce::dontSendNotification);
            refreshMelody (true);
        }
    };

    matchMethodBox.onChange = [this]
    {
        engine.properties.getUserSettings()->setValue ("sing.matchMethod", matchMethodBox.getSelectedId() - 1);
        refreshMelody (false);
    };

    matchButton.onClick = [this]
    {
        matched = true;
        wordsBox.setSelectedId (2, juce::dontSendNotification);   // one syllable on each note
        refreshMelody (false);
    };
    useMineButton.onClick = [this] { matched = false; refreshMelody (false); };
    makeTrackButton.onClick = [this] { makeMatchedTrack(); };

    thisClip.onClick = [this] { matched = false; refreshMelody (true); };
    wholeTrack.onClick = [this] { matched = false; refreshMelody (true); };
    wordsBox.onChange = [this] { refreshMelody (false); };

    pitchBox.onChange = [this] { engine.properties.getUserSettings()->setValue ("sing.pitch", pitchBox.getSelectedId() - 1); };
    qualityBox.onChange = [this] { engine.properties.getUserSettings()->setValue ("sing.quality", qualityBox.getSelectedId() - 1); };
    breaths.onClick = [this] { engine.properties.getUserSettings()->setValue ("sing.breaths", breaths.getToggleState()); };
    muteMelody.onClick = [this] { engine.properties.getUserSettings()->setValue ("sing.muteMelody", muteMelody.getToggleState()); };

    cancelButton.onClick = [this]
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SingWindow> (this)]
        {
            if (safe != nullptr)
                if (auto* w = safe->findParentComponentOfClass<juce::DocumentWindow>())
                    w->closeButtonPressed();
        });
    };

    singButton.onClick = [this]
    {
        if (! voice.has_value() || ! isStillThere())
            return;

        SingRequest request;
        request.track = track;
        request.voice = *voice;
        request.melody = melody;
        request.pairing = pairing;
        request.settings.breaths = breaths.getToggleState();
        request.settings.exactPitch = pitchBox.getSelectedId() == 2;
        request.steps = qualityBox.getSelectedId() == 1 ? 10 : qualityBox.getSelectedId() == 3 ? 50 : 20;
        request.muteMelody = muteMelody.getToggleState();

        if (onSing)
            onSing (std::move (request));

        cancelButton.onClick();
    };

    engine.addChangeListener (this);
    setSize (width, height);
    refreshTracks();
    refreshVoice();
    refreshMelody (true);
}

SingWindow::~SingWindow()
{
    engine.removeChangeListener (this);
}

bool SingWindow::isStillThere() const
{
    return track != nullptr && engine.indexOf (track) >= 0;
}

void SingWindow::openGuide (gen::lyricsguide::Topic topic)
{
    if (onOpenGuide)
        onOpenGuide (topic);
}

juce::String SingWindow::getTitleText() const
{
    return isStillThere() ? "Sing \"" + track->name + "\" with a computer voice" : juce::String ("Sing a melody with a computer voice");
}

void SingWindow::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // The lyrics, the notes or the tracks changed: look again.
    refreshTracks();
    refreshMelody (false);
}

void SingWindow::setSource (Track* t, MidiClip* c)
{
    track = t;
    clip = c;
    matched = false;
    refreshTracks();
    refreshMelody (true);
}

void SingWindow::selectVoice (const juce::String& folderName)
{
    voiceName = folderName;
    refreshVoice();
    refreshMelody (false);
}

//==============================================================================
void SingWindow::refreshTracks()
{
    melodyTracks.clear();
    melodyBox.clear (juce::dontSendNotification);
    int selected = 0;

    for (auto& t : engine.getTracks())
    {
        if (t->type != Track::Type::midi)
            continue;

        melodyTracks.push_back (t.get());
        melodyBox.addItem (t->name, (int) melodyTracks.size());

        if (t.get() == track)
            selected = (int) melodyTracks.size();
    }

    melodyBox.setSelectedId (selected, juce::dontSendNotification);

    // The clip must still be on the track.
    if (clip != nullptr)
    {
        bool found = false;

        if (isStillThere())
            for (auto& c : track->clips)
                found = found || c.get() == clip;

        if (! found)
            clip = nullptr;
    }

    thisClip.setEnabled (clip != nullptr);
    thisClip.setToggleState (clip != nullptr && ! wholeTrack.getToggleState(), juce::dontSendNotification);

    if (clip == nullptr)
        wholeTrack.setToggleState (true, juce::dontSendNotification);
    else if (! wholeTrack.getToggleState())
        thisClip.setToggleState (true, juce::dontSendNotification);

    title.setText (getTitleText(), juce::dontSendNotification);
}

void SingWindow::refreshVoice()
{
    voiceFolders = sing::listVoices (sing::getVoicesFolder());
    voice.reset();
    voiceError = {};

    bool found = false;

    for (auto& f : voiceFolders)
        found = found || f.getFileName() == voiceName;

    if (! found)
        voiceName = voiceFolders.empty() ? juce::String() : voiceFolders.front().getFileName();

    if (voiceName.isNotEmpty())
    {
        auto check = sing::loadVoice (sing::getVoicesFolder().getChildFile (voiceName), sing::getVocodersFolder (sing::getVoicesFolder()));

        if (check.voice.has_value())
            voice = std::move (check.voice);
        else
            voiceError = check.error;

        engine.properties.getUserSettings()->setValue ("sing.voice", voiceName);
    }

    voiceButton.setButtonText (voice.has_value() ? voice->name + juce::String::fromUTF8 (" \xe2\x96\xbe")
                                                 : voiceName.isNotEmpty() ? voiceName + " (problem)" : juce::String ("No voice yet"));
    voiceButton.setTooltip (voice.has_value() ? "The voice that sings: " + voice->name + ". Click for the other voices, to show it in Finder or to remove it."
                                              : "The voice that sings: a model trained on recordings of one singer. Add voices with Add a voice...");

    juce::String creditText;

    if (voice.has_value())
    {
        creditText = voice->author.isNotEmpty() ? "by " + voice->author : juce::String();

        if (voice->vocoderLicence.isNotEmpty())
            creditText << (creditText.isEmpty() ? "" : dot()) << "vocoder " << voice->vocoderLicence << " (non-commercial)";
    }

    credit.setText (creditText, juce::dontSendNotification);
    termsButton.setEnabled (voice.has_value() && voice->files.terms.existsAsFile());
    helpView->setState (voiceError);
    helpView->setVisible (! voice.has_value());
    stepsButton.setVisible (! voice.has_value());
    resized();
}

std::vector<st::SourceNote> SingWindow::collectNotes() const
{
    std::vector<st::SourceNote> notes;

    if (! isStillThere())
        return notes;

    for (auto& c : track->clips)
    {
        if (c->kind != Clip::Kind::midi || (thisClip.getToggleState() && clip != nullptr && c.get() != clip))
            continue;

        for (auto& n : static_cast<MidiClip&> (*c).notes)
            notes.push_back ({ n.pitch, c->startBeat + n.start, n.length, n.velocity });
    }

    return notes;
}

void SingWindow::refreshMelody (bool chooseModeAgain)
{
    melody = st::buildMelody (collectNotes(), engine.getTempoMap());

    // Matching: what it would do to the melody's own notes, and the adjusted copy when the user asked for it.
    const int target = st::countSyllablesToMatch (engine.getLyrics(), melody);
    canMatch = target > 0 && ! melody.notes.empty() && (int) melody.notes.size() != target;
    if (! canMatch)
        matchResult = st::MatchResult();
    else if (matchMethodBox.getSelectedId() != 2)
        matchResult = st::matchNotesToSyllableTimes (melody, st::findSyllables (engine.getLyrics(), melody, st::Mode::onePerNote),
                                                     matchMethodBox.getSelectedId() == 3 ? st::Method::syllableTimesCapped : st::Method::syllableTimes);
    else
        matchResult = st::matchNotesToSyllables (melody, target);

    matched = matched && canMatch;

    if (matched)
        melody = st::buildMelody (matchResult.notes, engine.getTempoMap());

    if (chooseModeAgain && ! matched)
        wordsBox.setSelectedId (st::chooseMode (engine.getLyrics(), melody) == st::Mode::asTimed ? 1 : 2, juce::dontSendNotification);

    pairing = st::pair (melody, engine.getLyrics(), wordsBox.getSelectedId() == 2 ? st::Mode::onePerNote : st::Mode::asTimed, voice.has_value() ? &*voice : nullptr);
    advice = st::getAdvice (melody, pairing);
    picture->setData (&melody, &pairing);
    picture->setSize (picture->getContentWidth (pictureViewport.getWidth() - 2), pictureViewport.getHeight() - 20);
    refreshAdvice();
    updateEnabled();
    title.setText (getTitleText(), juce::dontSendNotification);

    const bool showMatch = canMatch || matched;
    matchLabel.setText (matched ? "Singing from adjusted notes; your clip is unchanged. " + st::describeMatch (matchResult)
                                : canMatch ? st::describeMatch (matchResult, false) : juce::String(), juce::dontSendNotification);
    matchLabel.setVisible (showMatch);
    matchButton.setVisible (showMatch && ! matched);
    matchMethodBox.setVisible (showMatch && ! matched);
    makeTrackButton.setVisible (matched);
    useMineButton.setVisible (matched);
    resized();
    repaint();
}

void SingWindow::makeMatchedTrack()
{
    if (! matched || ! isStillThere() || matchResult.notes.empty())
        return;

    // A new MIDI track after the melody's, with one clip holding the adjusted notes. The melody's own track and clips are not touched.
    auto* source = track;
    auto* newTrack = engine.addTrack (Track::Type::midi, engine.indexOf (source) + 1);

    if (newTrack == nullptr)
        return;

    newTrack->name = source->name + " (matched)";
    engine.setBuiltInSound (*newTrack, source->builtInSound.load());

    const double first = matchResult.notes.front().startBeat;
    double last = first;

    for (auto& n : matchResult.notes)
        last = juce::jmax (last, n.startBeat + n.lengthBeats);

    auto* newClip = engine.addMidiClip (*newTrack, first, last - first);
    {
        const juce::ScopedLock sl (engine.getLock());
        newClip->name = newTrack->name;
        newClip->lengthBeats = last - first;

        for (auto& n : matchResult.notes)
            newClip->notes.push_back ({ n.pitch, n.startBeat - first, n.lengthBeats, n.velocity });
    }

    engine.notesChanged();

    // Sing the new track from now on.
    track = newTrack;
    clip = nullptr;
    matched = false;
    wholeTrack.setToggleState (true, juce::dontSendNotification);
    refreshTracks();
    refreshMelody (false);
}

void SingWindow::refreshAdvice()
{
    adviceView->setAdvice (advice, juce::jmax (100, adviceViewport.getWidth() - adviceViewport.getScrollBarThickness()));
}

void SingWindow::updateEnabled()
{
    bool blocked = false;

    for (auto& a : advice)
        blocked = blocked || a.blocksSinging;

    const bool ok = voice.has_value() && isStillThere() && ! blocked && st::canSing (pairing);
    singButton.setEnabled (ok);

    if (! voice.has_value())
        hint.setText ("Add a voice to sing this: Get a voice... shows where", juce::dontSendNotification);
    else if (blocked || ! st::canSing (pairing))
        hint.setText ("Nothing to sing yet: see the advice", juce::dontSendNotification);
    else
        hint.setText ({}, juce::dontSendNotification);
}

//==============================================================================
void SingWindow::showVoiceMenu()
{
    juce::PopupMenu menu;
    int id = 1;

    for (auto& f : voiceFolders)
        menu.addItem (id++, f.getFileName(), true, f.getFileName() == voiceName);

    if (voiceFolders.empty())
        menu.addItem (1000, "No voices yet", false);

    menu.addSeparator();
    menu.addItem (1001, "Show in Finder");
    menu.addItem (1002, "Remove this voice...", voice.has_value() || voiceName.isNotEmpty());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&voiceButton), [safe = juce::Component::SafePointer<SingWindow> (this)] (int result)
    {
        auto* self = safe.getComponent();

        if (self == nullptr || result == 0)
            return;

        if (result == 1001)
            (self->voiceName.isNotEmpty() ? sing::getVoicesFolder().getChildFile (self->voiceName) : sing::getVoicesFolder()).revealToUser();
        else if (result == 1002)
            self->removeCurrentVoice();
        else if (result >= 1 && result <= (int) self->voiceFolders.size())
            self->selectVoice (self->voiceFolders[(size_t) result - 1].getFileName());
    });
}

void SingWindow::showAddMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "A .zip or .oudep file...");
    menu.addItem (2, "A folder...");
    menu.addSeparator();
    menu.addItem (3, "Open the Voices folder");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton), [safe = juce::Component::SafePointer<SingWindow> (this)] (int result)
    {
        auto* self = safe.getComponent();

        if (self == nullptr || result == 0)
            return;

        if (result == 3)
        {
            sing::getVoicesFolder().createDirectory();
            sing::getVoicesFolder().revealToUser();
            return;
        }

        auto chooser = std::make_shared<juce::FileChooser> (result == 1 ? "Pick a voice or vocoder (.zip or .oudep)" : "Pick the voice's folder",
                                                            juce::File::getSpecialLocation (juce::File::userDesktopDirectory),
                                                            result == 1 ? "*.zip;*.oudep" : "");
        const int flags = result == 1 ? juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                                      : juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync (flags, [safe, chooser] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();

            if (safe != nullptr && file != juce::File())
                safe->addVoice (file);
        });
    });
}

void SingWindow::showGetMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Hoshino Hanami by Lotte V (suggested): her page and download");
    menu.addItem (2, "More English voices: the DiffSinger wiki");
    menu.addSeparator();
    menu.addItem (3, "Step by step: getting and adding a voice...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&getButton), [safe = juce::Component::SafePointer<SingWindow> (this)] (int result)
    {
        auto* self = safe.getComponent();

        if (self == nullptr || result == 0)
            return;

        if (result == 1)
            juce::URL (hanamiPage).launchInDefaultBrowser();
        else if (result == 2)
            juce::URL (voiceWiki).launchInDefaultBrowser();
        else
            self->openGuide (gen::lyricsguide::Topic::voices);
    });
}

void SingWindow::addVoice (const juce::File& source)
{
    if (installJob != nullptr)
        return;

    installJob = std::make_unique<InstallJob> (source, this, [safe = juce::Component::SafePointer<SingWindow> (this)] (InstallJob& job, bool cancelled)
    {
        // The job is deleted when this returns (the progress window allows it), so take what is needed first.
        auto* self = safe.getComponent();

        if (self == nullptr)
            return;

        const auto result = job.result;
        juce::MessageManager::callAsync ([safe, result, cancelled]
        {
            if (auto* window = safe.getComponent())
                window->finishAddingVoice (result, cancelled);
        });
    });
    installJob->launchThread();
}

void SingWindow::finishAddingVoice (const sing::InstallResult& r, bool cancelled)
{
    installJob.reset();

    if (cancelled || r.cancelled)
        return;

    if (r.error.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't add the voice",
                                                r.error + "\n\nThe ? button and Step by step... explain what a voice folder holds and what to do.");
        return;
    }

    if (r.wasVocoder)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Vocoder added",
                                                "The vocoder " + r.installed.getFileName() + " is in the Vocoders folder. Voices that need it can use it now.");
        refreshVoice();
        refreshMelody (false);
        return;
    }

    voiceName = r.installed.getFileName();
    refreshVoice();
    refreshMelody (false);

    if (r.notice.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Voice added, one thing is missing", r.notice);
    else if (voice.has_value())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Voice added",
                                                voice->name + " is ready. Read its terms with Terms... and credit "
                                                + (voice->author.isNotEmpty() ? voice->author : juce::String ("its maker")) + " when you release a song with it.");
}

void SingWindow::removeCurrentVoice()
{
    if (voiceName.isEmpty())
        return;

    const auto folder = sing::getVoicesFolder().getChildFile (voiceName);
    auto options = juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::QuestionIcon, "Remove this voice",
                                                                 "Move \"" + voiceName + "\" from the Voices folder to the bin? Songs that were sung with it keep their audio.",
                                                                 "Remove", "Cancel", this);
    juce::AlertWindow::showAsync (options, [safe = juce::Component::SafePointer<SingWindow> (this), folder] (int answer)
    {
        if (answer == 1 && sing::removeVoice (folder) && safe != nullptr)
        {
            safe->voiceName = {};
            safe->refreshVoice();
            safe->refreshMelody (false);
        }
    });
}

//==============================================================================
void SingWindow::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
}

void SingWindow::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    auto top = r.removeFromTop (30);
    helpButton.setBounds (top.removeFromRight (30));
    title.setBounds (top);
    r.removeFromTop (8);

    auto rowA = r.removeFromTop (28);
    voiceLabel.setBounds (rowA.removeFromLeft (52));
    voiceButton.setBounds (rowA.removeFromLeft (250));
    rowA.removeFromLeft (8);
    addButton.setBounds (rowA.removeFromLeft (120));
    rowA.removeFromLeft (6);
    getButton.setBounds (rowA.removeFromLeft (120));
    rowA.removeFromLeft (10);
    termsButton.setBounds (rowA.removeFromRight (84));
    credit.setBounds (rowA);
    r.removeFromTop (8);

    auto rowB = r.removeFromTop (28);
    melodyLabel.setBounds (rowB.removeFromLeft (52));
    melodyBox.setBounds (rowB.removeFromLeft (220));
    rowB.removeFromLeft (14);
    thisClip.setBounds (rowB.removeFromLeft (100));
    wholeTrack.setBounds (rowB.removeFromLeft (140));
    rowB.removeFromLeft (14);
    wordsLabel.setBounds (rowB.removeFromLeft (46));
    wordsBox.setBounds (rowB.removeFromLeft (200));
    r.removeFromTop (8);

    auto rowC = r.removeFromTop (28);
    pitchLabel.setBounds (rowC.removeFromLeft (52));
    pitchBox.setBounds (rowC.removeFromLeft (130));
    rowC.removeFromLeft (14);
    breaths.setBounds (rowC.removeFromLeft (90));
    qualityLabel.setBounds (rowC.removeFromLeft (56));
    qualityBox.setBounds (rowC.removeFromLeft (110));
    rowC.removeFromLeft (14);
    muteMelody.setBounds (rowC);
    r.removeFromTop (10);

    auto bottom = r.removeFromBottom (32);
    singButton.setBounds (bottom.removeFromRight (90));
    bottom.removeFromRight (8);
    cancelButton.setBounds (bottom.removeFromRight (90));
    bottom.removeFromRight (8);
    stepsButton.setBounds (bottom.removeFromRight (130));
    hint.setBounds (bottom);
    r.removeFromBottom (8);

    pictureViewport.setBounds (r.removeFromTop (236));
    r.removeFromTop (8);

    if (matchLabel.isVisible())
    {
        auto strip = r.removeFromTop (28);

        if (matchButton.isVisible())
        {
            matchButton.setBounds (strip.removeFromLeft (190));
            strip.removeFromLeft (6);
            matchMethodBox.setBounds (strip.removeFromLeft (230));
        }

        if (makeTrackButton.isVisible())
        {
            useMineButton.setBounds (strip.removeFromRight (100));
            strip.removeFromRight (6);
            makeTrackButton.setBounds (strip.removeFromRight (170));
        }

        strip.removeFromLeft (8);
        matchLabel.setBounds (strip);
        r.removeFromTop (6);
    }

    if (helpView->isVisible())
    {
        helpView->setBounds (r.removeFromTop (juce::jmin (r.getHeight() - 60, 152)));
        r.removeFromTop (8);
    }

    adviceViewport.setBounds (r);
    picture->setSize (picture->getContentWidth (pictureViewport.getWidth() - 2), pictureViewport.getHeight() - 20);
    refreshAdvice();
}
} // namespace daw
