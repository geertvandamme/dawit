#pragma once

#include "engine/ReverbKinds.h"
#include "engine/TrackNameWords.h"

/** Checks the guide's planned numbers and advice on made-up named tracks. */
static void runReverbKindsTests()
{
    using namespace daw;
    using namespace daw::verb;
    const auto near = [] (double actual, double expected, double tolerance) { return std::abs (actual - expected) <= tolerance; };
    for (const auto type : { Type::digital, Type::room, Type::hall, Type::chamber, Type::plate, Type::spring })
    {
        check (kinds::getName (type).isNotEmpty() && kinds::getSummary (type).isNotEmpty() && kinds::explain (type).isNotEmpty(), "Each reverb kind has a name, summary and text");
        check (kinds::getShowcase (type).settings.type == type, "Each reverb kind's showcase is a preset of that kind");
    }
    check (kinds::explain (Type::plate).contains ("1957") && kinds::explain (Type::plate).contains ("EMT 140"), "Plate text explains the EMT 140 of 1957");
    check (kinds::explain (Type::chamber).contains ("1947") && kinds::explain (Type::spring).contains ("1961") && kinds::explain (Type::hall).contains ("1.9"), "Kind texts retain the planned chamber, spring and hall facts");
    check (near (kinds::reflectionRate (60.0, 0.01), 845.0, 1.0), "A 60 cubic metre room reaches 845 reflections per second at 10 ms");
    check (near (kinds::densityTimeMs (60.0), 10.9, 0.1) && near (kinds::densityTimeMs (15000.0), 172.0, 0.5)
           && near (kinds::densityTimeMs (56.6), 10.6, 0.1), "Room, hall and chamber density times match the guide");
    check (near (kinds::plateGroupVelocity (100.0), 43.8, 0.2) && near (kinds::plateGroupVelocity (1000.0), 138.5, 0.5)
           && near (kinds::plateGroupVelocity (10000.0), 438.1, 1.0), "Plate bending waves travel faster at higher frequencies");
    check (near (kinds::springRoundTripMs (0.0f), 30.0, 0.0) && near (kinds::springRoundTripMs (50.0f), 55.0, 0.0)
           && near (kinds::springRoundTripMs (100.0f), 80.0, 0.0), "Spring Size gives 30, 55 and 80 ms round trips");
    check (near (0.161 * 56.6 / 5.0, 1.82, 0.01), "Capitol chamber's Sabine absorption is about 1.82 square metres");
    check (kinds::getSource ("BV 1") == kinds::Source::vocal && kinds::getSource ("Vocals2") == kinds::Source::vocal
           && kinds::getSource ("Dovoxetail") == kinds::Source::unknown && kinds::getSource ("Choir") == kinds::Source::ensemble,
           "Source matcher recognises numbered whole words without matching short names inside words");

    Settings settings;
    settings.predelayMs = 0.0f;
    settings.mixPercent = 20.0f;
    settings.decaySeconds = 1.6f;
    auto advice = kinds::analyse ("Lead vocal", false, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("To try: Type Plate") && advice.contains ("30-100 ms"), "Digital vocal advice suggests Plate and enough pre-delay");
    check (advice.contains ("voice sounds close") && advice.contains ("words stay in front") && ! advice.contains ("snares"),
           "Vocal type advice explains the voice and words using the vocal starting point");
    settings.type = Type::spring;
    advice = kinds::analyse ("Lead vocal", false, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("Your reverb is a Spring") && advice.contains ("Plate is the usual choice"), "Spring vocal advice explains the usual alternative");
    settings.type = Type::room;
    settings.lowCutHz = 100.0f;
    settings.widthPercent = 100.0f;
    advice = kinds::analyse ("Bass", false, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("300-400 Hz") && advice.contains ("0-30%") && ! advice.contains ("150-300 Hz"), "Bass advice keeps lows dry and centred and replaces generic low-cut advice");
    settings.mixPercent = 60.0f;
    settings.lowCutHz = 50.0f;
    advice = kinds::analyse (trackname::pluginTrackName, false, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("doesn't know the name of the track") && advice.contains ("The mix is 60%") && advice.contains ("150-300 Hz")
               && ! advice.contains ("can't tell from the name"),
           "In a plugin the track is unknown: one sentence about it, and the settings advice still follows");
    settings.lowCutHz = 100.0f;
    settings.mixPercent = 50.0f;
    advice = kinds::analyse ("Reverb", true, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("set Mix to 100%") && ! advice.contains ("far away"), "Return advice checks fully wet mix without insert mix advice");
    settings.type = Type::hall;
    settings.decaySeconds = 3.2f;
    advice = kinds::analyse ("Strings", false, settings, 120.0).joinIntoString ("\n");
    check (! advice.contains ("The tail lasts"), "Strings do not get tempo-based decay advice");
    settings = kinds::getShowcase (Type::plate).settings;
    advice = kinds::analyse ("Snare", false, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("Nothing to change"), "Snare plate's starting point is a usual choice on snare");
    settings.type = Type::hall;
    settings.decaySeconds = 6.0f;
    advice = kinds::analyse ("Keys", false, settings, 120.0).joinIntoString ("\n");
    check (advice.contains ("3 bars") && advice.contains ("a bar is 2.0 s"), "Six-second tail is three bars at 120 BPM");
    check (kinds::analyse ("Track 3", false, settings, 120.0).joinIntoString ("\n").contains ("Name it Vocal"), "Unknown names get the source naming hint");
    check (kinds::analyse ({}, false, settings, 120.0).joinIntoString ("\n").contains ("isn't on a track"), "Stand-in reverbs explain how to get track advice");
    for (const auto* name : { "Lead vocal", "Bass", "Keys", "Track 3", "Snare", "Strings" })
        check (! kinds::analyse (name, false, settings, 120.0).joinIntoString ("\n").containsIgnoreCase ("wrong"), "Reverb advice never calls a choice wrong");
}
