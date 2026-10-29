#include "BuiltInEq.h"
#include "BuiltInFormat.h"

#include <complex>

namespace daw
{
namespace eq
{
    juce::String getTypeName (BandType t)
    {
        switch (t)
        {
            case BandType::lowCut:    return "Low cut";
            case BandType::lowShelf:  return "Low shelf";
            case BandType::bell:      return "Bell";
            case BandType::notch:     return "Notch";
            case BandType::highShelf: return "High shelf";
            case BandType::highCut:   return "High cut";
        }

        return {};
    }

    juce::String getTypeAlias (BandType t)
    {
        switch (t)
        {
            case BandType::lowCut:    return "high-pass";
            case BandType::lowShelf:  return "bass shelf";
            case BandType::bell:      return "peak";
            case BandType::notch:     return "band-stop";
            case BandType::highShelf: return "treble shelf";
            case BandType::highCut:   return "low-pass";
        }

        return {};
    }

    Band getDefaultBand (int index)
    {
        switch (index)
        {
            case 0:  return { false, BandType::lowCut, 80.0f, 0.0f, flatQ, 12 };
            case 1:  return { true, BandType::lowShelf, 150.0f, 0.0f, flatQ, 12 };
            case 2:  return { true, BandType::bell, 400.0f, 0.0f, 1.0f, 12 };
            case 3:  return { true, BandType::bell, 2500.0f, 0.0f, 1.0f, 12 };
            case 4:  return { true, BandType::highShelf, 8000.0f, 0.0f, flatQ, 12 };
            default: return { false, BandType::highCut, 16000.0f, 0.0f, flatQ, 12 };
        }
    }

    double Biquad::getMagnitude (double freq, double sampleRate) const
    {
        const auto z = std::polar (1.0, -juce::MathConstants<double>::twoPi * freq / sampleRate);   // z^-1
        const auto numerator = b0 + (b1 + b2 * z) * z;
        const auto denominator = 1.0 + (a1 + a2 * z) * z;
        return std::abs (numerator / denominator);
    }

    double Stages::getMagnitude (double freq, double sampleRate) const
    {
        double m = 1.0;

        for (int i = 0; i < num; ++i)
            m *= filters[(size_t) i].getMagnitude (freq, sampleRate);

        return m;
    }

    namespace
    {
        enum class Shape { lowPass, highPass, bandPass, peak, notch, lowShelf, highShelf };

        Biquad makeBiquad (Shape shape, double freq, double q, double gainDb, double sampleRate)
        {
            // The Audio EQ Cookbook: w0 is the frequency as an angle, alpha sets the width from Q,
            // and A is the gain (as the square root of the level, since it's applied twice).
            const double w0 = juce::MathConstants<double>::twoPi * juce::jlimit (1.0, sampleRate * 0.49, freq) / sampleRate;
            const double cosW = std::cos (w0), alpha = std::sin (w0) / (2.0 * juce::jmax (0.01, q));
            const double A = std::pow (10.0, gainDb / 40.0), sqrtA2alpha = 2.0 * std::sqrt (A) * alpha;
            double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

            switch (shape)
            {
                case Shape::lowPass:
                    b0 = b2 = (1.0 - cosW) / 2.0; b1 = 1.0 - cosW;
                    a0 = 1.0 + alpha; a1 = -2.0 * cosW; a2 = 1.0 - alpha;
                    break;

                case Shape::highPass:
                    b0 = b2 = (1.0 + cosW) / 2.0; b1 = -(1.0 + cosW);
                    a0 = 1.0 + alpha; a1 = -2.0 * cosW; a2 = 1.0 - alpha;
                    break;

                case Shape::bandPass:   // 0 dB at the centre
                    b0 = alpha; b1 = 0.0; b2 = -alpha;
                    a0 = 1.0 + alpha; a1 = -2.0 * cosW; a2 = 1.0 - alpha;
                    break;

                case Shape::peak:
                    b0 = 1.0 + alpha * A; b1 = -2.0 * cosW; b2 = 1.0 - alpha * A;
                    a0 = 1.0 + alpha / A; a1 = -2.0 * cosW; a2 = 1.0 - alpha / A;
                    break;

                case Shape::notch:
                    b0 = 1.0; b1 = -2.0 * cosW; b2 = 1.0;
                    a0 = 1.0 + alpha; a1 = -2.0 * cosW; a2 = 1.0 - alpha;
                    break;

                case Shape::lowShelf:
                    b0 = A * ((A + 1) - (A - 1) * cosW + sqrtA2alpha);
                    b1 = 2 * A * ((A - 1) - (A + 1) * cosW);
                    b2 = A * ((A + 1) - (A - 1) * cosW - sqrtA2alpha);
                    a0 = (A + 1) + (A - 1) * cosW + sqrtA2alpha;
                    a1 = -2 * ((A - 1) + (A + 1) * cosW);
                    a2 = (A + 1) + (A - 1) * cosW - sqrtA2alpha;
                    break;

                case Shape::highShelf:
                    b0 = A * ((A + 1) + (A - 1) * cosW + sqrtA2alpha);
                    b1 = -2 * A * ((A - 1) + (A + 1) * cosW);
                    b2 = A * ((A + 1) + (A - 1) * cosW - sqrtA2alpha);
                    a0 = (A + 1) - (A - 1) * cosW + sqrtA2alpha;
                    a1 = 2 * ((A - 1) - (A + 1) * cosW);
                    a2 = (A + 1) - (A - 1) * cosW - sqrtA2alpha;
                    break;
            }

            return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
        }

        /** A 12 or 24 dB/oct cut. 24 is two filters in a row with the Qs of a 4th-order
            Butterworth (0.54 and 1.31), so at Q 0.71 it is just as flat; the band's Q scales the
            second one, which sets the bump. */
        Stages makeCut (Shape shape, const Band& band, double sampleRate)
        {
            Stages s;

            if (band.slope >= 24)
            {
                s.num = 2;
                s.filters[0] = makeBiquad (shape, band.freq, 0.5412, 0.0, sampleRate);
                s.filters[1] = makeBiquad (shape, band.freq, 1.3066 * band.q / flatQ, 0.0, sampleRate);
            }
            else
            {
                s.filters[0] = makeBiquad (shape, band.freq, band.q, 0.0, sampleRate);
            }

            return s;
        }
    }

    Stages makeStages (const Band& band, double sampleRate)
    {
        Stages s;

        switch (band.type)
        {
            case BandType::lowCut:    return makeCut (Shape::highPass, band, sampleRate);
            case BandType::highCut:   return makeCut (Shape::lowPass, band, sampleRate);
            case BandType::lowShelf:  s.filters[0] = makeBiquad (Shape::lowShelf, band.freq, band.q, band.gainDb, sampleRate); break;
            case BandType::highShelf: s.filters[0] = makeBiquad (Shape::highShelf, band.freq, band.q, band.gainDb, sampleRate); break;
            case BandType::bell:      s.filters[0] = makeBiquad (Shape::peak, band.freq, band.q, band.gainDb, sampleRate); break;
            case BandType::notch:     s.filters[0] = makeBiquad (Shape::notch, band.freq, band.q, 0.0, sampleRate); break;
        }

        return s;
    }

    Stages makeSoloStages (const Band& band, double sampleRate)
    {
        auto passBand = band;
        passBand.q = flatQ;

        switch (band.type)
        {
            case BandType::lowCut:
            case BandType::lowShelf:  return makeCut (Shape::lowPass, passBand, sampleRate);
            case BandType::highCut:
            case BandType::highShelf: return makeCut (Shape::highPass, passBand, sampleRate);
            case BandType::bell:
            case BandType::notch:     break;
        }

        Stages s;
        s.filters[0] = makeBiquad (Shape::bandPass, band.freq, band.q, 0.0, sampleRate);
        return s;
    }

    double getResponseDb (const Band& band, double freq, double sampleRate)
    {
        if (! band.on)
            return 0.0;

        return juce::Decibels::gainToDecibels (makeStages (band, sampleRate).getMagnitude (freq, sampleRate), -200.0);
    }

    double getBandwidthOctaves (double q)
    {
        return 2.0 / std::log (2.0) * std::asinh (1.0 / (2.0 * juce::jmax (0.01, q)));
    }

    const std::vector<Region>& getRegions()
    {
        static const std::vector<Region> regions
        {
            { 20.0f, 60.0f, "sub-bass", "felt more than heard: the rumble under a kick or a synth bass" },
            { 60.0f, 250.0f, "bass", "the weight of kick and bass, the low notes of piano and guitar" },
            { 250.0f, 500.0f, "low mids", "body and warmth, where many instruments pile up" },
            { 500.0f, 2000.0f, "mids", "the core of most instruments and voices" },
            { 2000.0f, 6000.0f, "presence", "clarity and attack, where the ear is most sensitive" },
            { 6000.0f, 10000.0f, "brilliance", "the s and t sounds, cymbals, string and breath noise" },
            { 10000.0f, 20000.0f, "air", "openness and sparkle" }
        };

        return regions;
    }

    const std::vector<Problem>& getProblems()
    {
        static const std::vector<Problem> problems
        {
            { "boomy", 80.0f, 200.0f, true, "a low cut on what doesn't need the lows, or a cut of 2-4 dB" },
            { "thin", 100.0f, 250.0f, false, "too little body: a low shelf or a wide bell of 2-3 dB, or less of a low cut" },
            { "muddy", 200.0f, 500.0f, true, "a cut of 2-3 dB with a Q around 1-2, on the instruments that pile up there" },
            { "boxy", 300.0f, 800.0f, true, "a narrow cut of 3-6 dB at the frequency that sounds worst" },
            { "nasal", 800.0f, 1500.0f, true, "a narrow cut of 2-4 dB: it sounds like a telephone" },
            { "harsh", 2000.0f, 5000.0f, true, "a small, narrow cut of 2-3 dB; the ear is most sensitive here" },
            { "sibilant", 5000.0f, 8000.0f, true, "a narrow cut, or a de-esser that cuts only while an s sounds" },
            { "dull", 8000.0f, 16000.0f, false, "too little top: a high shelf of 2-3 dB" }
        };

        return problems;
    }

    const std::vector<Preset>& getPresets()
    {
        static const auto presets = []
        {
            // Each preset starts from the default bands and changes some.
            auto make = [] (juce::String name, juce::String why, std::vector<std::tuple<int, Band, juce::String>> changes)
            {
                Preset p { std::move (name), std::move (why), {}, {} };

                for (int i = 0; i < numBands; ++i)
                {
                    p.bands[(size_t) i] = getDefaultBand (i);
                    p.bands[(size_t) i].on = false;
                }

                for (auto& [index, band, note] : changes)
                {
                    p.bands[(size_t) index] = band;
                    p.notes[(size_t) index] = note;
                }

                return p;
            };

            using T = BandType;
            std::vector<Preset> list;

            auto flat = make ("Flat", "Every band at 0 dB and the cuts off: the EQ changes nothing. Start here to learn by ear.", {});
            for (int i = 0; i < numBands; ++i)
                flat.bands[(size_t) i] = getDefaultBand (i);
            list.push_back (flat);

            list.push_back (make ("Vocal", "A clear, present voice that sits on top of the mix.",
            {
                { 0, { true, T::lowCut, 90.0f, 0.0f, flatQ, 12 }, "Removes rumble, handling noise and the boom of breathing on the mic: a voice has almost nothing useful below 80-100 Hz." },
                { 2, { true, T::bell, 300.0f, -3.0f, 1.4f, 12 }, "Takes out mud: the boxy, closed-in sound of a small room or a close mic." },
                { 3, { true, T::bell, 4000.0f, 2.5f, 1.0f, 12 }, "Presence: the words get easier to follow and the voice comes forward. Too much sounds harsh and sharpens the s sounds." },
                { 4, { true, T::highShelf, 12000.0f, 2.0f, flatQ, 12 }, "Air: a little openness and sparkle on top." },
            }));

            list.push_back (make ("Kick drum", "Weight and a clear click, without the cardboard-box sound.",
            {
                { 0, { true, T::lowCut, 30.0f, 0.0f, flatQ, 24 }, "Removes sub rumble below what speakers play, which only uses up headroom." },
                { 1, { true, T::bell, 60.0f, 3.0f, 1.2f, 12 }, "Weight: the low thump you feel more than hear." },
                { 2, { true, T::bell, 400.0f, -4.0f, 2.0f, 12 }, "Boxiness, the cardboard sound of the shell: cutting it makes the kick sound bigger and tighter." },
                { 3, { true, T::bell, 4000.0f, 3.0f, 1.5f, 12 }, "The click of the beater, which lets the kick be heard on small speakers." },
                { 5, { true, T::highCut, 12000.0f, 0.0f, flatQ, 12 }, "Removes hiss and cymbal spill above the kick's range." },
            }));

            list.push_back (make ("Snare drum", "A snare with body and crack.",
            {
                { 0, { true, T::lowCut, 80.0f, 0.0f, flatQ, 12 }, "Removes kick spill and rumble." },
                { 1, { true, T::bell, 200.0f, 2.0f, 1.2f, 12 }, "Body: the fat part of the hit." },
                { 2, { true, T::bell, 800.0f, -3.0f, 2.0f, 12 }, "The boxy, honky ring of the shell." },
                { 3, { true, T::bell, 5000.0f, 2.0f, 1.0f, 12 }, "Crack, and the rattle of the snare wires." },
            }));

            list.push_back (make ("Bass", "Full, but leaving room for the kick, and heard on small speakers.",
            {
                { 0, { true, T::lowCut, 35.0f, 0.0f, flatQ, 24 }, "Below 35-40 Hz is mostly rumble you can't hear, which makes the mix less loud." },
                { 1, { true, T::bell, 100.0f, 2.0f, 1.0f, 12 }, "Fullness: the fundamental of most bass notes." },
                { 2, { true, T::bell, 250.0f, -3.0f, 1.4f, 12 }, "Mud: bass, guitars and keys pile up here." },
                { 3, { true, T::bell, 800.0f, 2.0f, 1.2f, 12 }, "Growl and definition: the overtones that make the bass heard on laptop and phone speakers." },
                { 5, { true, T::highCut, 5000.0f, 0.0f, flatQ, 12 }, "String and finger noise, and hiss above what a bass needs." },
            }));

            list.push_back (make ("Acoustic guitar", "A clear strummed guitar that doesn't boom.",
            {
                { 0, { true, T::lowCut, 100.0f, 0.0f, flatQ, 12 }, "The body of the guitar booms at 80-120 Hz, more so close to the mic." },
                { 2, { true, T::bell, 200.0f, -3.0f, 1.2f, 12 }, "Woolliness from the sound hole." },
                { 3, { true, T::bell, 3000.0f, 2.0f, 1.0f, 12 }, "Pick attack and string definition." },
                { 4, { true, T::highShelf, 10000.0f, 2.0f, flatQ, 12 }, "Sparkle and air." },
            }));

            list.push_back (make ("Piano", "Keeps its warmth without clashing with bass and vocal.",
            {
                { 0, { true, T::lowCut, 40.0f, 0.0f, flatQ, 12 }, "Rumble and pedal thumps." },
                { 2, { true, T::bell, 300.0f, -2.0f, 1.0f, 12 }, "Mud, where piano, bass and guitars overlap." },
                { 3, { true, T::bell, 2500.0f, 1.5f, 0.8f, 12 }, "Clarity of the hammers." },
                { 4, { true, T::highShelf, 10000.0f, 1.5f, flatQ, 12 }, "Brilliance." },
            }));

            list.push_back (make ("Drum bus", "Gentle shaping of all drums together.",
            {
                { 0, { true, T::lowCut, 25.0f, 0.0f, flatQ, 24 }, "Sub rumble." },
                { 1, { true, T::lowShelf, 80.0f, 2.0f, flatQ, 12 }, "Weight of the kick and toms together." },
                { 2, { true, T::bell, 400.0f, -2.0f, 1.0f, 12 }, "Boxiness of the whole kit." },
                { 4, { true, T::highShelf, 10000.0f, 2.0f, flatQ, 12 }, "Cymbal shimmer and room air." },
            }));

            list.push_back (make ("Make room for a vocal", "For guitars, keys or pads under a voice: leaves the lows to bass and kick, and the presence range to the vocal.",
            {
                { 0, { true, T::lowCut, 150.0f, 0.0f, flatQ, 12 }, "Leaves the lows to bass and kick." },
                { 3, { true, T::bell, 3000.0f, -3.0f, 1.0f, 12 }, "A dip where the voice is most understandable, so the words come through." },
                { 5, { true, T::highCut, 12000.0f, 0.0f, flatQ, 12 }, "Leaves the air to the vocal and cymbals." },
            }));

            list.push_back (make ("Remove hum", "Mains hum is 50 Hz in Europe and much of the world, 60 Hz in the Americas, with copies at 2x and 3x. Set the notches to 60, 120 and 180 Hz for 60 Hz hum.",
            {
                { 1, { true, T::notch, 50.0f, 0.0f, 10.0f, 12 }, "The hum itself." },
                { 2, { true, T::notch, 100.0f, 0.0f, 10.0f, 12 }, "Its first overtone, twice the frequency." },
                { 3, { true, T::notch, 150.0f, 0.0f, 10.0f, 12 }, "Its second overtone, three times the frequency." },
            }));

            return list;
        }();

        return presets;
    }

    const std::vector<Instrument>& getInstruments()
    {
        static const std::vector<Instrument> instruments
        {
            { "Kick drum", "Most of its energy is below 100 Hz, but what tells you it's a kick on a phone is the click of the beater at 2-5 kHz.",
              false, 40.0f, 100.0f, 8000.0f, 30.0f,
              { { 50.0f, 80.0f, "thump", "The weight you feel. A broad boost of 2-3 dB makes it fuller. Kick and bass both want this spot: let one of them own it.", false },
                { 250.0f, 500.0f, "boxy", "Sounds like hitting a cardboard box. A cut of 3-6 dB with Q 2-4, often around 300-400 Hz, is the most common kick fix.", true },
                { 2000.0f, 5000.0f, "click", "The beater hitting the head: what makes the kick heard on small speakers and in a busy mix. Boost 2-4 dB.", false } } },

            { "Snare drum", "A body of 150-250 Hz with a crack on top and the rattle of its wires reaching far up.",
              false, 150.0f, 250.0f, 12000.0f, 80.0f,
              { { 150.0f, 250.0f, "body", "The weight of the hit. Boost 2-3 dB for a fatter snare.", false },
                { 400.0f, 800.0f, "boxy", "A hollow, cardboard sound; a ringing overtone often hides here too. Cut 2-4 dB, narrow.", true },
                { 2000.0f, 5000.0f, "crack", "The stick on the head: snap and attack. Boost to make it cut through.", false },
                { 6000.0f, 10000.0f, "wires", "The rattle of the snare wires. A high shelf brings out the sizzle; cut a little if it's harsh.", false } } },

            { "Toms", "Tuned drums: floor toms sit around 70-120 Hz, rack toms higher. Between hits their mics mostly pick up the rest of the kit.",
              false, 70.0f, 300.0f, 8000.0f, 50.0f,
              { { 80.0f, 200.0f, "body", "The note of the tom. Find it by sweeping a narrow boost, then boost it a little for a fuller tom.", false },
                { 300.0f, 800.0f, "boxy", "Cardboard sound and the ring of the shell. Cut 2-4 dB.", true },
                { 3000.0f, 5000.0f, "attack", "The stick on the head. Boost for punch.", false } } },

            { "Hi-hat and cymbals", "Almost all of a cymbal is high: what it has below a few hundred hertz is mostly the other drums bleeding in.",
              false, 300.0f, 1000.0f, 18000.0f, 250.0f,
              { { 300.0f, 700.0f, "clang", "A gong-like ring, mostly from big cymbals. Cut if they sound clangy.", true },
                { 2000.0f, 5000.0f, "harsh", "Where cymbals hurt. A small cut softens them without making them dull.", true },
                { 8000.0f, 12000.0f, "sizzle", "The shimmer. Boost a little for brightness; cut if the hi-hat sounds like hiss.", false },
                { 12000.0f, 18000.0f, "air", "Openness. Overheads often get a gentle high shelf here.", false } } },

            { "Bass guitar", "Its notes run from E1 (41 Hz) up, but you hear a bass on small speakers through its overtones: the growl around 1 kHz.",
              true, 41.0f, 330.0f, 5000.0f, 30.0f,
              { { 60.0f, 100.0f, "weight", "The low end of the mix. Share it with the kick: if the kick owns 50-60 Hz, give the bass 80-100 Hz, or the other way round.", false },
                { 200.0f, 300.0f, "mud", "Too much makes the mix woolly. A cut of 2-3 dB clears room for guitars and keys.", true },
                { 700.0f, 1500.0f, "growl", "The character of the notes, and what lets you hear the bass on a phone. Boost to bring it forward.", false },
                { 2000.0f, 4000.0f, "fingers", "String noise, pick attack and fret buzz. Boost for definition; cut if it clicks.", false } } },

            { "Synth bass and 808", "Often lower than a bass guitar, with few overtones: the lowest-reaching part of many songs.",
              true, 30.0f, 200.0f, 3000.0f, 25.0f,
              { { 30.0f, 60.0f, "sub", "Felt more than heard. At any moment only one of kick and bass should be loudest down here.", false },
                { 100.0f, 400.0f, "overtones", "What small speakers play: phones and laptops can't reproduce the sub. A boost or some saturation here keeps the bass audible.", false } } },

            { "Electric guitar", "From E2 (82 Hz) to around E6, through an amp that hardly plays above 6 kHz. Distorted guitars fill the mids wall to wall.",
              true, 82.0f, 1320.0f, 10000.0f, 80.0f,
              { { 200.0f, 400.0f, "mud", "Guitars pile up here with bass and keys. Cut 2-3 dB for a clearer mix.", true },
                { 2000.0f, 4000.0f, "bite", "The pick attack and the edge of the sound. Boost to bring it forward; too much is harsh.", false },
                { 6000.0f, 10000.0f, "fizz", "The buzzy top of distortion. A high cut at 8-10 kHz cleans it up.", true } } },

            { "Acoustic guitar", "The same notes as an electric guitar, with a body that booms when the mic is close and strings that sparkle up high.",
              true, 82.0f, 1320.0f, 15000.0f, 80.0f,
              { { 80.0f, 200.0f, "boom", "The body of the guitar close to the mic. A low cut or a low shelf cut keeps it from booming.", true },
                { 200.0f, 400.0f, "boxy", "Cut 2-3 dB if it sounds like it's played in a box.", true },
                { 2000.0f, 5000.0f, "strings", "The pick and the strings: clarity. Boost 2-3 dB for a brighter strum.", false },
                { 10000.0f, 15000.0f, "air", "Shimmer and sparkle. A high shelf of 2-3 dB.", false } } },

            { "Piano", "The widest range of any instrument here: A0 (27.5 Hz) to C8 (4.2 kHz). In a band mix its lowest notes often get a low cut to leave room for the bass.",
              true, 27.5f, 4186.0f, 12000.0f, 0.0f,
              { { 100.0f, 250.0f, "warmth", "The fullness of the left hand. Cut a little if the piano fights the bass.", false },
                { 300.0f, 600.0f, "mud", "Boxy and muddy, especially from close mics in a small room. Cut 2-3 dB.", true },
                { 2000.0f, 5000.0f, "attack", "The hammers on the strings: presence and definition. Boost to make it cut through.", false } } },

            { "Strings", "Violins, violas and cellos together: from C2 (65 Hz) on the cello to around E7 on the violin.",
              true, 65.0f, 2640.0f, 16000.0f, 60.0f,
              { { 150.0f, 300.0f, "warmth", "The body of violas and cellos. Boost for a richer section; cut if it gets thick.", false },
                { 2000.0f, 5000.0f, "harsh", "Where close-miked violins get piercing. A small cut smooths them.", true },
                { 7000.0f, 10000.0f, "scratch", "Bow noise and rosin. A little is realism, too much is scratchy.", true } } },

            { "Brass", "Trombones from E2 (82 Hz), trumpets up to about C6. Loud, bright and full of overtones.",
              true, 82.0f, 1050.0f, 10000.0f, 70.0f,
              { { 150.0f, 400.0f, "body", "Fullness, mostly from trombones and horns. Cut a little if a section sounds thick.", false },
                { 1000.0f, 3000.0f, "brassy", "The bright blare that makes brass brass. Boost to cut through; too much is harsh.", false } } },

            { "Male voice", "Sung notes from about F2 to C5, with speech low in that range (about 90 to 155 Hz). The words, though, are understood at 2-5 kHz, far above the notes.",
              true, 87.0f, 520.0f, 12000.0f, 80.0f,
              { { 100.0f, 250.0f, "chest", "Warmth and weight. Too much gets boomy, especially close to the mic (the proximity effect).", false },
                { 250.0f, 500.0f, "mud", "Boxy and muffled. A cut of 2-3 dB often makes a voice clearer at once.", true },
                { 800.0f, 1500.0f, "nasal", "Honky, like a telephone. Cut narrow if it sticks out.", true },
                { 2000.0f, 5000.0f, "presence", "The clarity of the words: what brings a voice forward in a mix. Boost 2-3 dB.", false },
                { 5000.0f, 8000.0f, "sibilance", "Sibilance: the s and t sounds. Too loud hurts; a de-esser or a narrow cut tames it.", true },
                { 10000.0f, 16000.0f, "air", "Breath and openness. A high shelf of 2-3 dB.", false } } },

            { "Female voice", "Sung notes from about E3 to C6, an octave above a male voice; presence and sibilance sit a little higher too.",
              true, 165.0f, 1050.0f, 16000.0f, 120.0f,
              { { 250.0f, 500.0f, "mud", "Boxy and muffled. Cut 2-3 dB.", true },
                { 2000.0f, 5000.0f, "presence", "The clarity of the words. Boost 2-3 dB to bring the voice forward.", false },
                { 6000.0f, 9000.0f, "sibilance", "Sibilance: the s and t sounds. A de-esser or a narrow cut tames it.", true },
                { 10000.0f, 16000.0f, "air", "Breath and openness. A high shelf of 2-3 dB.", false } } },
        };

        return instruments;
    }
} // namespace eq

//==============================================================================
namespace
{
    juce::NormalisableRange<float> logRange (float start, float end)
    {
        return { start, end,
                 [] (float s, float e, float x) { return s * std::pow (e / s, x); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    juce::String parameterId (int band, const juce::String& what) { return "b" + juce::String (band + 1) + what; }

    // How the parameters read in the editor and to hosts: "320 Hz", "2.50 kHz", "+3.0 dB", "0.71".
    juce::AudioParameterFloatAttributes frequencyText()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz" : juce::String (v / 1000.0f, v < 10000.0f ? 2 : 1) + " kHz";
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                const float v = text.retainCharacters ("0123456789.").getFloatValue();
                return text.containsIgnoreCase ("k") ? v * 1000.0f : v;
            });
    }

    juce::AudioParameterFloatAttributes gainText()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return (v >= 0.05f ? "+" : "") + juce::String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB"; })
            .withValueFromStringFunction ([] (const juce::String& text) { return text.retainCharacters ("-0123456789.").getFloatValue(); });
    }

    // How fast the settings glide to a new value; long enough to never click, short enough to feel direct.
    constexpr double glideSeconds = 0.03;
    constexpr int chunkSize = 32;
}

juce::AudioProcessorValueTreeState::ParameterLayout BuiltInEq::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    juce::StringArray typeNames;

    for (int t = 0; t < eq::numTypes; ++t)
        typeNames.add (eq::getTypeName ((eq::BandType) t));

    for (int i = 0; i < eq::numBands; ++i)
    {
        const auto band = eq::getDefaultBand (i);
        const auto name = "Band " + juce::String (i + 1) + " ";
        auto id = [i] (const char* what) { return juce::ParameterID { parameterId (i, what), 1 }; };

        layout.add (std::make_unique<juce::AudioParameterBool> (id ("on"), name + "on", band.on));
        layout.add (std::make_unique<juce::AudioParameterChoice> (id ("type"), name + "type", typeNames, (int) band.type));
        layout.add (std::make_unique<juce::AudioParameterFloat> (id ("freq"), name + "frequency", logRange (eq::minFreq, eq::maxFreq), band.freq,
                                                                 frequencyText()));
        layout.add (std::make_unique<juce::AudioParameterFloat> (id ("gain"), name + "gain", juce::NormalisableRange<float> (-eq::maxGainDb, eq::maxGainDb, 0.1f),
                                                                 band.gainDb, gainText()));
        layout.add (std::make_unique<juce::AudioParameterFloat> (id ("q"), name + "Q", logRange (eq::minQ, eq::maxQ), band.q,
                                                                 juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); })));
        layout.add (std::make_unique<juce::AudioParameterChoice> (id ("slope"), name + "slope", juce::StringArray { "12 dB/oct", "24 dB/oct" }, 0));
    }

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> (-eq::maxGainDb, eq::maxGainDb, 0.1f),
                                                             0.0f, gainText()));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "match", 1 }, "Match level", false));
    return layout;
}

BuiltInEq::BuiltInEq()
    : juce::AudioPluginInstance (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                                  .withOutput ("Output", juce::AudioChannelSet::stereo())),
      parameters (*this, nullptr, "DawItEQ", createLayout())
{
    for (int i = 0; i < eq::numBands; ++i)
    {
        auto& p = bandParameters[(size_t) i];
        p.on = parameters.getRawParameterValue (parameterId (i, "on"));
        p.type = parameters.getRawParameterValue (parameterId (i, "type"));
        p.freq = parameters.getRawParameterValue (parameterId (i, "freq"));
        p.gain = parameters.getRawParameterValue (parameterId (i, "gain"));
        p.q = parameters.getRawParameterValue (parameterId (i, "q"));
        p.slope = parameters.getRawParameterValue (parameterId (i, "slope"));
    }

    outputDb = parameters.getRawParameterValue ("output");
    matchLevel = parameters.getRawParameterValue ("match");
    analyzerBuffer.clear();
}

void BuiltInEq::setMatchingLevel (bool on)
{
    if (auto* p = parameters.getParameter ("match"))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

juce::PluginDescription BuiltInEq::getDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = identifier;
    d.pluginFormatName = BuiltInFormat::formatName;
    d.category = "EQ";
    d.manufacturerName = "DawIt";
    d.version = "1.0";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x44457131;   // "DEq1"
    d.isInstrument = false;
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void BuiltInEq::fillInPluginDescription (juce::PluginDescription& d) const
{
    d = getDescription();
}

juce::RangedAudioParameter* BuiltInEq::getBandParameter (int index, const juce::String& what) const
{
    return parameters.getParameter (parameterId (index, what));
}

eq::Band BuiltInEq::readTarget (int index) const
{
    const auto& p = bandParameters[(size_t) index];
    eq::Band b;
    b.on = p.on->load() >= 0.5f;
    b.type = (eq::BandType) juce::jlimit (0, eq::numTypes - 1, juce::roundToInt (p.type->load()));
    b.freq = p.freq->load();
    b.gainDb = p.gain->load();
    b.q = p.q->load();
    b.slope = p.slope->load() >= 0.5f ? 24 : 12;
    return b;
}

eq::Band BuiltInEq::getBand (int index) const
{
    return readTarget (index);
}

void BuiltInEq::setBand (int index, const eq::Band& band)
{
    auto set = [this, index] (const char* what, float value)
    {
        if (auto* p = getBandParameter (index, what))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    set ("on", band.on ? 1.0f : 0.0f);
    set ("type", (float) band.type);
    set ("freq", band.freq);
    set ("gain", band.gainDb);
    set ("q", band.q);
    set ("slope", band.slope >= 24 ? 1.0f : 0.0f);
}

void BuiltInEq::applyPreset (const eq::Preset& preset)
{
    for (int i = 0; i < eq::numBands; ++i)
        setBand (i, preset.bands[(size_t) i]);
}

//==============================================================================
bool BuiltInEq::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && layout.getMainInputChannelSet() == out;
}

void BuiltInEq::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;

    for (int i = 0; i < eq::numBands; ++i)
    {
        auto& s = bands[(size_t) i];

        for (auto* v : { &s.freq, &s.q })
            v->reset (rate, glideSeconds);

        s.gainDb.reset (rate, glideSeconds);
        s.amount.reset (rate, glideSeconds);
        std::fill_n (&s.memory[0][0][0], 8, 0.0);
        updateBand (i, 0, true);
    }

    std::fill_n (&soloMemory[0][0][0], 8, 0.0);
    soloShown = -1;
    powerIn = powerOut = 0.0;
    outputGain.reset (rate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputDb->load()));
    matchGain.reset (rate, 0.1);
    matchGain.setCurrentAndTargetValue (1.0f);
}

void BuiltInEq::updateBand (int index, int numSamples, bool snap)
{
    const auto target = readTarget (index);
    auto& s = bands[(size_t) index];
    bool changed = snap;

    if (target.type != s.type || target.slope != s.slope)
    {
        // A different filter: its memory means nothing to the new one.
        s.type = target.type;
        s.slope = target.slope;
        std::fill_n (&s.memory[0][0][0], 8, 0.0);
        changed = true;
    }

    if (snap)
    {
        s.freq.setCurrentAndTargetValue (target.freq);
        s.q.setCurrentAndTargetValue (target.q);
        s.gainDb.setCurrentAndTargetValue (target.gainDb);
        s.amount.setCurrentAndTargetValue (target.on ? 1.0f : 0.0f);
    }
    else
    {
        s.freq.setTargetValue (target.freq);
        s.q.setTargetValue (target.q);
        s.gainDb.setTargetValue (target.gainDb);
        s.amount.setTargetValue (target.on ? 1.0f : 0.0f);
    }

    changed = changed || s.freq.isSmoothing() || s.q.isSmoothing() || s.gainDb.isSmoothing();

    if (! changed)
        return;

    eq::Band now = target;
    now.freq = s.freq.skip (numSamples);
    now.q = s.q.skip (numSamples);
    now.gainDb = s.gainDb.skip (numSamples);
    s.stages = eq::makeStages (now, rate);
}

namespace
{
    /** Runs one sample through a band's filters (transposed direct form II). */
    inline double runStages (const eq::Stages& stages, double (&memory)[2][2], double x)
    {
        for (int st = 0; st < stages.num; ++st)
        {
            const auto& f = stages.filters[(size_t) st];
            auto& z = memory[st];
            const double y = f.b0 * x + z[0];
            z[0] = f.b1 * x - f.a1 * y + z[1];
            z[1] = f.b2 * x - f.a2 * y;
            x = y;
        }

        return x;
    }
}

void BuiltInEq::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int total = buffer.getNumSamples();

    for (int ch = numChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    if (numChannels == 0)
        return;

    const int solo = soloBand.load();

    if (solo != soloShown)
    {
        soloShown = solo;
        std::fill_n (&soloMemory[0][0][0], 8, 0.0);
    }

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputDb->load()));
    const double powerCoefficient = 1.0 - std::exp (-chunkSize / (0.5 * rate));   // about half a second

    for (int start = 0; start < total; start += chunkSize)
    {
        const int n = juce::jmin (chunkSize, total - start);
        float before[chunkSize], after[chunkSize];
        double sumIn = 0.0, sumOut = 0.0;

        for (int i = 0; i < n; ++i)
        {
            float mono = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float x = buffer.getSample (ch, start + i);
                mono += x;
                sumIn += (double) x * x;
            }

            before[i] = mono / (float) numChannels;
        }

        for (int b = 0; b < eq::numBands; ++b)
            updateBand (b, n, false);

        if (juce::isPositiveAndBelow (solo, eq::numBands))
        {
            soloStages = eq::makeSoloStages (readTarget (solo), rate);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch, start);

                for (int i = 0; i < n; ++i)
                    data[i] = (float) runStages (soloStages, soloMemory[ch], data[i]);
            }
        }
        else
        {
            for (auto& band : bands)
            {
                const float from = band.amount.getCurrentValue(), to = band.amount.skip (n);

                if (from <= 0.0f && to <= 0.0f)
                {
                    std::fill_n (&band.memory[0][0][0], 8, 0.0);
                    continue;
                }

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch, start);

                    for (int i = 0; i < n; ++i)
                    {
                        const double x = data[i], y = runStages (band.stages, band.memory[ch], x);
                        const double amount = from + (to - from) * (i + 1) / n;
                        data[i] = (float) (x + amount * (y - x));
                    }
                }
            }

            for (int ch = 0; ch < numChannels; ++ch)
                for (int i = 0; i < n; ++i)
                    sumOut += (double) buffer.getSample (ch, start + i) * buffer.getSample (ch, start + i);

            // Match level: compare the loudness (power over the last half second) after the EQ with
            // before it, and make up the difference. Silence keeps the last gain.
            powerIn += powerCoefficient * (sumIn / (n * numChannels) - powerIn);
            powerOut += powerCoefficient * (sumOut / (n * numChannels) - powerOut);

            if (powerIn > 1.0e-8 && powerOut > 1.0e-12)
            {
                const float db = juce::jlimit (-24.0f, 24.0f, (float) (10.0 * std::log10 (powerIn / powerOut)));
                matchGainDb = db;
            }
        }

        matchGain.setTargetValue (isMatchingLevel() && solo < 0 ? juce::Decibels::decibelsToGain (matchGainDb.load()) : 1.0f);

        for (int i = 0; i < n; ++i)
        {
            const float gain = outputGain.getNextValue() * matchGain.getNextValue();
            float mono = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* data = buffer.getWritePointer (ch, start + i);
                *data *= gain;
                mono += *data;
            }

            after[i] = mono / (float) numChannels;
        }

        if (numChannels == 1 && buffer.getNumChannels() > 1)
            buffer.copyFrom (1, start, buffer, 0, start, n);

        // For the spectrum: drop what doesn't fit while nobody reads.
        int start1, size1, start2, size2;
        analyzerFifo.prepareToWrite (n, start1, size1, start2, size2);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* source = ch == 0 ? before : after;

            if (size1 > 0)
                analyzerBuffer.copyFrom (ch, start1, source, size1);

            if (size2 > 0)
                analyzerBuffer.copyFrom (ch, start2, source + size1, size2);
        }

        analyzerFifo.finishedWrite (size1 + size2);

        // The preview ring: always the newest samples, whoever reads.
        const auto written = previewWritten.load (std::memory_order_relaxed);

        for (int i = 0; i < n; ++i)
            previewRing[(size_t) ((written + i) % previewSize)].store (after[i], std::memory_order_relaxed);

        previewWritten.store (written + n, std::memory_order_release);
    }
}

juce::int64 BuiltInEq::readPreview (float* dest, int count) const
{
    count = juce::jlimit (0, previewSize, count);
    const auto written = previewWritten.load (std::memory_order_acquire);

    for (int i = 0; i < count; ++i)
        dest[i] = previewRing[(size_t) ((written - count + i + previewSize) % previewSize)].load (std::memory_order_relaxed);

    return written;
}

int BuiltInEq::readAnalyzer (float* before, float* after, int max)
{
    int ready = analyzerFifo.getNumReady();

    if (ready > max)
    {
        analyzerFifo.finishedRead (ready - max);
        ready = max;
    }

    int start1, size1, start2, size2;
    analyzerFifo.prepareToRead (ready, start1, size1, start2, size2);

    for (int ch = 0; ch < 2; ++ch)
    {
        float* dest = ch == 0 ? before : after;

        if (size1 > 0)
            juce::FloatVectorOperations::copy (dest, analyzerBuffer.getReadPointer (ch, start1), size1);

        if (size2 > 0)
            juce::FloatVectorOperations::copy (dest + size1, analyzerBuffer.getReadPointer (ch, start2), size2);
    }

    analyzerFifo.finishedRead (size1 + size2);
    return size1 + size2;
}

juce::AudioProcessorEditor* BuiltInEq::createEditor()
{
    return createEditorFunction != nullptr ? createEditorFunction (*this) : nullptr;
}

void BuiltInEq::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void BuiltInEq::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
} // namespace daw
