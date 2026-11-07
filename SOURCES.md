# Sources of the theory guides

Where the facts in DawIt's theory guides come from, grouped by guide. **Pages read** were opened and read while planning or checking a guide. **Searched for** are searches whose results shaped a fact without a page of its own being opened. Facts that are derived (a calculation) are marked so in the plan's *Theory* section, in `plans/`.

Keep this file current: whenever a guide's text or numbers are added or changed, add the sources used under that guide (see `.claude/agents/dawPlanFeature.md` and `dawTheory.md`).

**How the app uses this file.** DawIt embeds it and shows each guide's list as the last topic of that guide, *Sources*, with every link opening in the browser. So:

- A heading that is for a guide has a line `Guide key: <key>` straight under it, with the key `--screenshots` uses for the guide (`sound`, `readMusic`, `stepFilter`, ...). Every guide has one heading; a new guide needs its heading and key here. A heading without that line (*Loudness*, whose guide isn't built yet, and *Not for a guide*) is not shown anywhere.
- Any text under the heading before **Pages read** is shown to readers as a note, so write it for them. A guide with nothing recorded says "No sources recorded yet."
- Under **Pages read**, one line per page: `- <https://...>`, with an optional note in brackets after it. Every link starts with `https://`. Under **Searched for** (or **Searches**), one line per search, with the result links it used as `(results: <https://...>)`.

The first lists were rebuilt from the agents' web lookups in the Claude Code chat history (22 to 28 October 2026) and the source tables in `plans/`.


## Sound: "What is sound?" (envelope, mono check)

Guide key: sound

**Pages read**

- <https://en.wikipedia.org/wiki/Perceptual_attack_time>
- <https://knutsacoustics.com/files/Onset-transient-times-Rev.2.pdf>
- <https://lecdem.physics.umd.edu/h/h5/h5-22.html>
- <https://uol.de/f/6/dept/mediphysik/ag/musik-wahrnehmung/siedenburg2019transients.pdf>
- <https://www.soundonsound.com/sound-advice/10-essential-mix-checks>
- <https://www.uni-oldenburg.de/f/6/dept/mediphysik/ag/musik-wahrnehmung/siedenburg2019transients.pdf>

**Searched for**

- aupreset file location ~/Library/Audio/Presets manufacturer plugin name Audio Unit user presets
- Saldanha Corso 1964 timbre cues identification musical instruments attack transients removed
- iPhone 7 first iPhone with stereo speakers
- mono compatibility mix why check smartphone single speaker Bluetooth speaker club mono sum engineers
- removing the attack of a piano or instrument note makes it hard to identify instrument timbre experiment Berger 1964 wind instruments
- rise time defined 10% to 90% of final value
- typical attack time milliseconds plucked guitar piano violin bowed flute onset rise time instruments table

## Recording: "How recording works"

Guide key: recording

**Pages read**

- <https://aes.org/publications/elibrary-page/?id=14256>
- <https://aes2.org/publications/elibrary-page/?id=14256>
- <https://www.soundonsound.com/sound-advice/q-should-gain-stage-6dbfs-or-18dbfs>

**Searched for**

- "latency" "live sound monitoring" Lester Boley vocalists in-ear "1.4 ms" OR "42 ms" acceptable
- Focusrite buffer size recording low 64 128 mixing higher 512 1024 why CPU dropouts clicks
- Lester Boley "The Effects of Latency on Live Sound Monitoring" AES 2007 vocal in-ear threshold ms
- Lester Boley "The Effects of Latency on Live Sound Monitoring" AES 7198 abstract 42 ms 1.4 ms
- Sound On Sound recording levels 24-bit peaks -10 dBFS average -18 dBFS how loud should I record
- macOS microphone permission denied CoreAudio input returns silence zeros NSMicrophoneUsageDescription Privacy & Security Microphone
- microphone level line level instrument level dBu typical values -60 dBu +4 dBu -10 dBV Hi-Z guitar input explained
- phantom power P48 IEC 61938 48 volts condenser microphone dynamic microphone does not need
- round trip latency USB audio interface 48 kHz 64 samples 128 samples measured milliseconds macOS RTL Utility

## Tuning: "How to tune a guitar"

Guide key: tuning

**Pages read**

- <https://blog.gretschguitars.com/2014/01/gretschtech-reentrant-tuning/>
- <https://en.wikibooks.org/wiki/Guitar/Tuning_the_Guitar>
- <https://en.wikipedia.org/wiki/A440_(pitch_standard)>
- <https://en.wikipedia.org/wiki/Drop_D_tuning>
- <https://home.mit.bme.hu/~bank/publist/dafx09/index.html>
- <https://thesis.unipd.it/bitstream/20.500.12608/59910/1/Marmolaro%20Mattia.pdf>
- <https://www.fender.com/articles/how-to/why-do-guitars-go-out-of-tune>
- <https://www.guitarplayer.com/lessons/dadgad-for-dummies-you-already-know-50-percent-of-this-tuning>
- <https://www.sweetwater.com/insync/guitar-tuning-tip/>

**Searched for**

- 432 Hz tuning claims evidence study scientific no difference 440 Hz
- D'Addario why new guitar strings go out of tune stretch temperature humidity strings go flat warm
- DADGAD tuning D2 A2 D3 G3 A3 D4 drop D tuning lowers sixth string a whole step
- Fender support why does my guitar go out of tune temperature humidity new strings stretch
- concert pitch A440 ISO 16 orchestras tune 442 443 Hz baroque 415 Hz
- guitar string pitch sharp right after pluck tension modulation pitch glide tuner read after attack
- steel guitar strings warm up expand tension drops pitch goes flat stage lights thermal expansion
- ukulele standard tuning G4 C4 E4 A4 re-entrant high G low G tuning
- why tune up to pitch guitar from below gear slack tuning machine Fender how to tune
- orchestras Germany Austria tune A 443 Hz Berlin Philharmonic Vienna Philharmonic pitch

## Singing: singing techniques and "Sing this" (voice synthesis, diction)

Guide key: singing

**Pages read**

- <https://aadl.org/files/catalog_guides/SM58_guide_en-US.pdf>
- <https://deepwiki.com/stakira/OpenUtau/4.2-diffsinger-integration>
- <https://diffsinger.miraheze.org/wiki/Azure_Cobalt>
- <https://diffsinger.miraheze.org/wiki/Canary>
- <https://diffsinger.miraheze.org/wiki/Category:DiffSinger_voicebanks>
- <https://diffsinger.miraheze.org/wiki/Category:Voicebanks_supporting_English>
- <https://diffsinger.miraheze.org/wiki/English>
- <https://diffsinger.miraheze.org/wiki/Hanami_Hoshino>
- <https://diffsinger.miraheze.org/wiki/Laru_Mine>
- <https://diffsinger.miraheze.org/wiki/Main_Page>
- <https://diffsinger.miraheze.org/wiki/Miyo>
- <https://diffsinger.miraheze.org/wiki/Peiton>
- <https://diffsinger.miraheze.org/wiki/TIGER>
- <https://doi.org/10.1121/1.419735>
- <https://en.wikipedia.org/wiki/Passaggio>
- <https://en.wikipedia.org/wiki/Vibrato>
- <https://en.wikipedia.org/wiki/Vocal_cords>
- <https://en.wikipedia.org/wiki/Vocal_fry_register>
- <https://en.wikipedia.org/wiki/Vocal_range>
- <https://en.wikipedia.org/wiki/Voice_frequency>
- <https://en.wikipedia.org/wiki/Voice_type>
- <https://github.com/agentasteriski/azurecobaltsynth>
- <https://github.com/cmusphinx/cmudict>
- <https://github.com/lottev1991/LotteV-Voicebank-Update-Repo>
- <https://github.com/lottev1991/LotteV-Voicebank-Update-Repo/blob/main/ToU_EN.md>
- <https://github.com/lottev1991/LotteV-Voicebank-Update-Repo/releases>
- <https://github.com/lottev1991/Project-AIdol-Public-English-Dataset>
- <https://github.com/openutau/OpenUtau/wiki/DiffSinger-support>
- <https://github.com/openvpi/DiffSinger>
- <https://github.com/openvpi/DiffSinger/blob/main/docs/GettingStarted.md>
- <https://github.com/openvpi/vocoders/issues/4>
- <https://github.com/openvpi/vocoders/releases>
- <https://github.com/stakira/OpenUtau/tree/master/OpenUtau.Core/DiffSinger>
- <https://github.com/stakira/OpenUtau/tree/master/OpenUtau.Core/DiffSinger/Phonemizers>
- <https://labs.sonicfield.org/library/i-know-it-when-i-hear-it>
- <https://lottev.moe/category/vocal-synth/diffsinger/>
- <https://lottev.moe/hoshino-hanami-ai%e2%9d%a4dol-for-diffsinger-v1-0-is-out/>
- <https://openvpi.github.io/vocoders/>
- <https://periodicos.sbu.unicamp.br/ojs/index.php/cel/article/download/8674196/35048>
- <https://pmc.ncbi.nlm.nih.gov/articles/PMC9510150>
- <https://pubmed.ncbi.nlm.nih.gov/9228821/>
- <https://pubmed.ncbi.nlm.nih.gov/9228822/>
- <https://pubs.aip.org/jasa/article/102/1/616/557516/Vibrato-extent-and-intonation-in-professional>
- <https://pubs.aip.org/jasa/article/96/4/1979/564542/Measurements-of-the-vibrato-rate-of-ten-singers>
- <https://raw.githubusercontent.com/lottev1991/LotteV-Voicebank-Update-Repo/main/LICENSE-vocoder>
- <https://raw.githubusercontent.com/lottev1991/LotteV-Voicebank-Update-Repo/main/LICENSE-voicebank>
- <https://raw.githubusercontent.com/lottev1991/LotteV-Voicebank-Update-Repo/master/LICENSE-vocoder>
- <https://raw.githubusercontent.com/lottev1991/LotteV-Voicebank-Update-Repo/master/LICENSE-voicebank>
- <https://raw.githubusercontent.com/openutau/OpenUtau/master/LICENSE.txt>
- <https://raw.githubusercontent.com/openutau/OpenUtau/master/OpenUtau.Core/DiffSinger/DiffSingerBasePhonemizer.cs>
- <https://raw.githubusercontent.com/openutau/OpenUtau/master/OpenUtau.Core/DiffSinger/DiffSingerRenderer.cs>
- <https://raw.githubusercontent.com/stakira/OpenUtau/master/OpenUtau.Core/DiffSinger/DiffSingerConfig.cs>
- <https://raw.githubusercontent.com/stakira/OpenUtau/master/OpenUtau.Core/DiffSinger/Phonemizers/DiffSingerEnglishPhonemizer.cs>
- <https://raw.githubusercontent.com/stakira/OpenUtau/master/OpenUtau.Core/G2p/ArpabetG2p.cs>
- <https://singingcarrots.com/blog/how-to-find-your-vocal-range-step-by-step-guide/>
- <https://utaurepository.miraheze.org/wiki/Peiton>
- <https://vocalsynth.fandom.com/wiki/DiffSinger>
- <https://www.mcgill.ca/library/files/library/sm58-user-guide.pdf>
- <https://www.nhs.uk/conditions/laryngitis/>
- <https://www.soundonsound.com/sound-advice/q-what-distance-my-mic-should-record-my-voice>
- <https://www2.spsc.tugraz.at/add_material/audiotechnik/manuals/50_Mikrofone/Shure/shure_sm58_manual.pdf>

**Searched for**

- CMU Pronouncing Dictionary 134,000 words 39 phonemes stress 0 1 2 BSD license
- DiffSinger English voicebank free commercial use OpenUtau download terms of use
- DiffSinger English voicebank release github OpenUtau "English" singer Kikyo OR Tigerlily OR "Hoshino Hanami" OR Peiton OR Chloe
- DiffSinger OpenUtau vocoder commercial use allowed license "vocoder" hifigan trained from scratch 44.1k 128 mel
- DiffSinger inference real-time factor RTF CPU acoustic model shallow diffusion speedup steps benchmark
- English DiffSinger voicebank OpenUtau download
- English DiffSinger voicebank free download terms of use ARPAbet DIFFS EN github release
- OpenUtau DiffSinger render speed Mac M1 M2 CPU slow seconds per minute CoreML
- OpenUtau DiffSinger voicebank structure dsconfig.yaml dsdur dspitch dsvariance linguistic.onnx dsdict.yaml
- choir diction place consonants before the beat so the vowel lands on the beat
- free English DiffSinger voicebank CC license OpenUtau huggingface OR github "DiffSinger" English singer open license
- openvpi DiffSinger license Apache ONNX export variance acoustic
- openvpi pc-nsf-hifigan vocoder license DiffSinger release
- singing vowels occupy percent of time compared with speech consonants Sundberg
- voice type ranges soprano C4 C6 mezzo A3 A5 contralto F3 F5 tenor C3 C5 baritone A2 A4 bass E2 E4
- "I know it when I hear it" Larrouy-Maestri 2018 pitch accuracy tolerance "quarter tone" cents Music Perception
- "Larrouy-Maestri" "half of a quarter tone" mistuning sung melodies
- Larrouy-Maestri 2018 "I know it when I hear it" out of tune tolerance quarter tone Music & Science
- Larrouy-Maestri tolerance threshold out of tune singing cents listeners perceive mistuning
- Pfordresher Brown 2007 poor-pitch singing absence of tone deafness Music Perception percentage inaccurate singers
- Prame "vibrato extent" "71 cents" OR "±71 cents" singers Ave Maria J Acoust Soc Am 102 616
- Prame 1994 vibrato rate ten singers 5.5 6.7 Hz extent cents Schubert Ave Maria
- Prame 1997 "Vibrato extent and intonation in professional Western lyric singing" cents mean extent
- SM58 user guide "proximity effect" "6 to 10 dB at 100 Hz"
- Shure SM58 user guide proximity effect "6 to 10 dB" 100 Hz 6 mm
- Shure vocal recording microphone distance inches proximity effect pop filter distance from microphone
- Sound On Sound recording vocals condenser mic distance cm pop shield singer
- congenital amusia prevalence 1.5% Peretz Vuvan 2017 tone deafness most poor singers can hear pitch Pfordresher
- drinks never touch the vocal folds hydration systemic steam surface hydration singers voice clinic
- hoarseness lasting more than 2 weeks see doctor singer NIDCD OR NHS OR AAO-HNS
- how to find your vocal range test sing lowest highest comfortable note hold "lip trill" OR siren warm up voice teacher
- passaggio notes by voice type tenor D4 G4 baritone B3 E4 soprano E4 F#5 Richard Miller primo secondo passaggio
- singing vibrato rate 5.5 to 7.5 Hz extent cents Sundberg Prame
- vocal warm-up duration minutes research singers lip trill semi-occluded vocal tract Titze benefits evidence
- voice type ranges soprano C4 C6 mezzo A3 A5 alto F3 F5 tenor C3 C5 baritone A2 A4 bass E2 E4

## Pitch correction: the effect and its guide

Guide key: tune

**Pages read**

- <https://help.antarestech.com/hc/en-us/articles/42855736822932>
- <https://media.uaudio.com/support/manuals/dd/Auto-Tune%20Realtime%20Advanced%20Manual.pdf>
- <https://www.fullcompass.com/common/files/87680-AUTOTUNEACC10Manual.pdf>
- <https://www.sweetwater.com/sweetcare/articles/auto-tune-retune-speed-setting-work/>

**Searched for**

- Antares Auto-Tune retune speed natural correction milliseconds range typical
- Auto-Tune Andy Hildebrand 1997 Antares Cher Believe 1998 retune speed zero autocorrelation seismic
- Auto-Tune Andy Hildebrand released 1997 Antares history
- Auto-Tune retune speed typical settings milliseconds natural correction hard tune 0 ms recommended range
- Sound On Sound pitch correction retune speed natural vocal milliseconds formant correction vibrato tips
- frequency difference limen pure tone 1 kHz percent cents trained listeners 0.2%

## Gate: "What is a gate?"

Guide key: gate

**Pages read**

- <https://en.wikipedia.org/wiki/Gated_reverb>
- <https://en.wikipedia.org/wiki/Noise_gate>
- <https://mastering.com/noise-gate/>
- <https://nailthemix.com/sam-pura-tempo-synced-kick-snare-gates>
- <https://support.apple.com/guide/logicpro-ipad/lpipbafe55dc/ipados>
- <https://support.apple.com/guide/logicpro/noise-gate-lgcef1ea6f51/mac>
- <https://www.fabfilter.com/help/pro-g/using/dynamiccontrols>
- <https://www.fabfilter.com/help/pro-g/using/timecontrols>
- <https://www.prosoundweb.com/signal-processing-fundamentals-dynamic-controllers/2/>
- <https://www.shure.com/en-US/insights/recording-drums-part-3-gates>
- <https://www.soundonsound.com/techniques/advanced-gating-techniques-part-1>

**Searched for**

- Drawmer DS201 noise gate specifications attack hold decay range key filter
- FabFilter Pro-G help hysteresis range ratio expander knob description
- Logic Pro Noise Gate parameters hysteresis "Reduction" "Lookahead" high cut low cut sidechain "Monitor" ranges
- Sound On Sound noise gate hysteresis hold release settings drums toms explained
- downward expander vs noise gate ratio 2:1 vocals breath room noise typical settings range dB
- gating drums kick snare toms settings hold ms release ms range dB sidechain high-pass snare key filter Hz

## Stereo: the stereo tool and its guide

Guide key: stereo

**Pages read**

- <https://en.wikipedia.org/wiki/Phonograph_record>
- <https://en.wikipedia.org/wiki/Subwoofer>

**Searched for**

- Linkwitz-Riley crossover LR2 inverted polarity LR4 in phase LR8 -6 dB at crossover allpass sum Rane note
- Sound On Sound elliptical EQ vinyl cutting lateral vertical modulation out-of-phase bass
- phase correlation meter definition +1 0 -1 integration time IEC 60268-18 correlation meter
- vinyl mastering low frequencies mono below Hz vertical groove out of phase bass stylus jump elliptical EQ
- why keep bass mono in a mix low frequencies hard to localise club PA subwoofers mono summed Sound On Sound

## Routing: buses, aux sends, signal flow

Guide key: routing

**Pages read**

- <https://audient.com/tutorial/the-beginners-guide-to-headroom>
- <https://blog.native-instruments.com/glue-compressor/>
- <https://emastered.com/blog/glue-compressor>
- <https://en.wikipedia.org/wiki/Audio_bus>
- <https://en.wikipedia.org/wiki/Stem_mixing_and_mastering>
- <https://help.uaudio.com/hc/en-us/articles/30847649785748-SSL-4000-G-Bus-Compressor-Manual>
- <https://musicradar.com/how-to/compress-mix-bus>
- <https://pmc.ncbi.nlm.nih.gov/articles/PMC3563070>
- <https://sengpielaudio.com/calculator-coherentsources.htm>
- <https://www.SengpielAudio.com/AddingAmplitudesAndLevels.pdf>
- <https://www.ableton.com/en/manual/mixing/>
- <https://www.izotope.com/en/learn/signal-chain-order-of-operations.html>
- <https://www.izotope.com/en/learn/the-difference-between-vca-and-sub-groups-in-mixing.html>
- <https://www.masteringbox.com/learn/headroom-for-mastering>
- <https://www.musicguymixing.com/bus-compression/>
- <https://www.soundonsound.com/reviews/ssl-xlogic-g-series-compressor>
- <https://www.soundonsound.com/sound-advice/q-what-order-should-compress-and-eq>
- <https://www.soundonsound.com/techniques/making-most-your-mixer>
- <https://www.waves.com/how-loud-mix-before-mastering>

**Searched for**

- Ableton Live manual return tracks sends pre post fader dry/wet 100%
- Logic Pro send "Post Pan" "Post Fader" "Pre Fader" send position user guide
- Mike Senior Mixing Secrets reverb blend size tone spread reverbs sends
- SSL G-series bus compressor attack 0.1 0.3 1 3 10 30 ms release 0.1 0.3 0.6 1.2 auto ratio 2 4 10
- SSL G-series bus compressor attack 0.1 0.3 1 3 10 30 ms release 0.1 0.3 0.6 1.2 auto ratio 2:1 4:1 10:1
- Sound On Sound aux sends pre-fader post-fader reverb return 100% wet insert effects explained
- difference between bus, submix, group track, VCA group and stem in mixing
- direct-to-reverberant energy ratio auditory distance cue Zahorik 2002
- drum bus glue compression settings 2-4 dB gain reduction 2:1 or 4:1 slow attack 10-30 ms sound on sound
- how much headroom to leave for mastering peak -6 dBFS -3 dBFS mix no limiter on master bus
- mix bus compression typical settings 1-2 dB gain reduction ratio 2:1 attack 30 ms release auto sidechain high pass filter
- sengpielaudio adding decibel levels incoherent sources 3 dB coherent 6 dB
- EQ before or after compression order plugin chain Sound On Sound
- typical insert effect order gate EQ compressor saturation delay reverb end of chain why compressor after reverb
- Logic Pro user guide inspector show hide key command I channel strips track inspector
- Logic Pro for Mac user guide "inspector" "two channel strips" selected track output channel strip
- Cubase operation manual Inspector left zone selected track sends inserts

**Pages read (the inspector)**

- <https://support.apple.com/en-gu/guide/logicpro/lgcp0182c4e5/mac>

## Reverb: the kinds of reverb

Guide key: reverb

**Pages read**

- <https://acris.aalto.fi/ws/portalfiles/portal/13004391/art_10.1155_2011_646134.pdf>
- <https://asp-eurasipjournals.springeropen.com/articles/10.1155/2011/646134>
- <https://ccrma.stanford.edu/~bilbao/dafx10/electro.pdf>
- <https://link.springer.com/article/10.1155/2011/646134>
- <https://uaudio.com/webzine/2006/april/text/content2.html>
- <https://www.barryrudolph.com/recall/manuals/emt140ts.pdf>
- <https://www.soundonsound.com/reviews/waves-abbey-road-reverb-plates>
- <https://www.soundonsound.com/techniques/golden-age-reverb-chambers>
- <https://www.soundonsound.com/techniques/inside-abbey-road-reverb-chamber>
- <https://www.vintagedigital.com.au/?p=144376>

**Searched for**

- Abbey Road Studios echo chamber Studio Two history built year size reverb Beatles
- Accutronics spring reverb tank delay time decay time specifications type 4 spring transit delay ms
- Beranek concert halls reverberation time occupied Musikverein 2.0 s Boston Symphony Hall 1.9 s initial time delay gap 20 ms best halls
- Capitol Studios echo chambers Les Paul 1956 eight chambers underground concrete walls reverb time
- EMT 140 plate dimensions "2 m" "1 m" thickness 0.5 mm steel plate reverberation model Bilbao OR Arcas
- EMT 140 plate reverb 1957 steel plate 1 x 2 metres 0.5 mm thick weight decay time damping
- EMT 140 plate reverb history Walter Kuhl 1957 reverberation time adjustable seconds
- EMT 250 1976 first commercial digital reverb Lexicon 224 1978 history
- Schroeder artificial reverberation echo density "1000 echoes per second" natural sounding
- Välimäki Parker Abel "Parametric spring reverberation effect" chirp delay ms transition frequency stretched allpass
- spring reverb history Hammond organ patent Laurens Hammond Fender reverb unit 1961 6G15 first
- spring reverb impulse response echoes repeat every ms measured Accutronics tank "ms" round trip delay spring length chirp spacing

## Styles: song templates and drum patterns

Guide key: styles

**Pages read**

- <https://en.wikipedia.org/wiki/Afrobeats>
- <https://en.wikipedia.org/wiki/Boom_bap>
- <https://en.wikipedia.org/wiki/Dembow_beat>
- <https://en.wikipedia.org/wiki/Drum_and_bass>
- <https://en.wikipedia.org/wiki/Four_on_the_floor_(music)>
- <https://en.wikipedia.org/wiki/Pachelbel%27s_Canon> (the eight chords of the Canon, used in the pop ballad's bridge)

**Searched for**

- afrobeats drum pattern kick snare rim shaker typical groove programming
- afrobeats typical BPM range tempo
- blues shuffle typical tempo BPM range slow blues 12-bar
- dembow drum pattern sixteenth steps kick every beat snare "3+3+2" tresillo reggaeton
- drum and bass "two-step" beat kick on 1 and the "and" of 3 snare on 2 and 4
- house music drum pattern open hi-hat off-beat "four on the floor" clap on 2 and 4 909
- lo-fi hip hop typical tempo BPM range
- reggaeton typical tempo BPM dembow rhythm
- trap music tempo 130-150 BPM half-time hi-hat rolls typical

## Loudness: "What is loudness? LUFS explained" (DawIt Meter)

**Pages read**

- <https://tech.ebu.ch/docs/tech/tech3341.pdf>
- <https://tech.ebu.ch/docs/tech/tech3342.pdf>
- <https://tech.ebu.ch/docs/r/r128.pdf>
- <https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-5-202311-I!!PDF-E.pdf>
- <https://raw.githubusercontent.com/jiixyj/libebur128/master/ebur128/ebur128.c>
- <https://support.spotify.com/us/artists/article/loudness-normalization/>
- <https://www.meterplugs.com/blog/2022/03/23/apple-switch-to-lufs.html>

**Searched for**

- ITU-R BS.1770-5 K-weighting filter coefficients 48 kHz -0.691 true-peak oversampling 4x
- Apple Music Sound Check -16 LUFS normalization Apple Digital Masters true peak; YouTube -14 LUFS loudness normalization Tidal Amazon Music
- EBU R 128 -23 LUFS tolerance ±0.5 LU maximum true peak -1 dBTP; ATSC A/85 -24 LKFS true peak -2 dBTP (results: <https://doc.flux.audio/mira/Metering_Loudness.html>, <https://docs.lawo.com/audio-production-mc-mixing-consoles/mc96/mc96-operation/mc96-user-configuration/mc96-system-settings/mc96-system-settings-loudness-metering>)
- AES TD1008 recommendations distribution loudness internet audio streaming podcast -16 LUFS -18 speech music (results: <https://docs.telosalliance.com/docs/understanding-loudness-for-streaming-audio>, <https://www.production-expert.com/production-expert-1/aes-update-loudness-recommendations-for-audio-only-streaming>)
- typical integrated LUFS of commercial pop and EDM masters -8 -6 LUFS loudness war measured crest factor PLR (results: <https://www.masteringthemix.com/pages/how-loud-should-you-master>, <https://ask.audio/articles/how-loudness-standards-relate-to-edm-and-club-music-production>, <https://violetrecording.com/how-loud-should-my-master-be/>)
- peak to loudness ratio PLR typical values masters Ian Shepherd dynamic recommended PLR 8 dB (results: <https://www.soundonsound.com/reviews/meterplugs-dynameter>)

## Lyrics: "How lyrics are written" (finding a rhyme, writing with AI)

Guide key: lyrics

**Pages read**

- <https://en.wikipedia.org/wiki/Perfect_and_imperfect_rhymes>
- <https://www.izotope.com/en/learn/5-types-of-rhymes-you-can-use-in-your-song>
- <https://tunedly.com/?p=5007>
- <https://github.com/cmusphinx/cmudict>
- <https://arxiv.org/html/2404.02456v1>
- <https://www.copyright.gov/newsnet/2025/1060.html>
- <https://docs.ollama.com/capabilities/structured-outputs>
- <https://www.williamfry.com/knowledge/gema-v-openai-europes-direction-on-ai-infringement/>

**Searched for**

- Pat Pattison rhyme types family rhyme additive subtractive assonance consonance songwriting
- large language models syllable counting rhyme tokenization poetry constraint failure study
- US Copyright Office Copyright and Artificial Intelligence Part 2 copyrightability January 2025 prompts not sufficient human modifications (results: <https://blogs.loc.gov/copyright/2025/02/inside-the-copyright-offices-report-copyright-and-artificial-intelligence-part-2-copyrightability/>)
- GEMA v OpenAI Munich court ruling November 2025 song lyrics memorised ChatGPT (results: <https://www.gema.de/en/w/landmark-ruling-gema-against-openai>, <https://cm.twobirds.com/en/insights/2025/landmark-ruling-of-the-munich-regional-court-(gema-v-openai)-on-copyright-and-ai-training>)
- Ollama modelfile parameter temperature default 0.8 seed (results: <https://docs.ollama.com/modelfile>)
- Pat Pattison consonant families plosives fricatives nasals l r companions family rhyme (results: <https://americansongwriter.com/songwriter-u-surviving-rhyme-meeting-the-family>)
- GEMA OpenAI Munich ruling appeal 2026 lyrics (results: <https://www.juve-patent.com/cases/open-ai-must-pay-gema-licence-fee-for-chatgpt/>)
- OpenAI Berufung GEMA Urteil Landgericht München 42 O 14139/24 (results: <https://www.wbs.legal/urheberrecht/gema-gewinnt-gegen-openai-chatgpt-verletzt-urheberrechte-von-songwritern-84612/>, <https://hifi.de/news/openai-kassiert-schlappe-vor-gericht-die-gema-schlaegt-zurueck-249987>)


## MIDI: "What is MIDI?" (the computer keyboard, transforming notes, pedals, pitch bend and controllers)

Guide key: midi

**Pages read**

- <https://en.wikipedia.org/wiki/Retrograde_(music)>
- <https://en.wikipedia.org/wiki/Inversion_(music)>
- <https://en.wikipedia.org/wiki/Musikalisches_W%C3%BCrfelspiel>
- <https://www.ableton.com/en/manual/editing-midi/>
- <https://www.ableton.com/en/manual/routing-and-i-o/>
- <https://www.ableton.com/en/manual/midi-and-key-remote-control/>
- <https://help.apple.com/logicpro/mac/10.4/en.lproj/lgcpb19cbd34.html>
- <https://en.wikipedia.org/wiki/Key_rollover>
- <https://www.deskthority.net/wiki/NKRO>
- <https://midi.org/community/getting-started-with-midi-1/help-understanding-midi-volume-commands> (the GM2 and GM1 volume and expression curve, 40 log10 (value / 127) dB each)
- <https://midi.org/community/midi-specifications/list-of-default-values-for-controllers-and-rpns> (volume 100, expression 127, bend range 2 semitones)
- <https://hub.yamaha.com/music-educators/instruments/piano/piano-pedagogy-pedaling>
- <https://en.wikipedia.org/wiki/Sustain_pedal>
- <https://en.wikipedia.org/wiki/Pitch_wheel>
- <https://orchestraltools.helpscoutdocs.com/article/85-common-midi-ccs>
- <https://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midifile/tempo.htm> (the tempo event FF 51 03 gives the length of a quarter note in microseconds: 500,000 is 120 BPM, the default when a file has none)
- <https://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midifile/time.htm> (the time signature event FF 58 04 nn dd cc bb)

**Searched for**

- Logic Pro Musical Typing window keys octave Z X velocity C V (results: <https://dummies.com/article/how-to-record-midi-in-logic-pro-x-with-musical-typing-147775>)
- keyboard ghosting key rollover explained how many keys at once membrane keyboard matrix
- Ableton Live MIDI note editor "Reverse" notes retrograde how note positions are reversed time selection (results: ableton.com/en/manual/editing-midi, forum.ableton.com)
- Dowling 1972 "Recognition of melodic transformations: Inversion, retrograde, and retrograde inversion" Perception & Psychophysics results (results: labs.utdallas.edu/mpac/files/2021/03/1972.pdf, experimental-psychology.de MongovenCarbon-inpress.pdf, mdpi.com/2073-8994/9/3/39)
- General MIDI Level 1 default pitch bend sensitivity 2 semitones RPN 0 Reset All Controllers RP-015 default values (results: <https://www.wikipedia.com/wiki/Pitch_bend>, <https://www.manualslib.com/manual/196691/Yamaha-Db50xg.html?page=10>, <https://jazzdup.nfshost.com/tutorial/tech/midispec/rpn.htm>)
- General MIDI volume CC7 expression CC11 attenuation 40 log10 curve recommended practice RP-036 default volume 100
- "General MIDI 2" initial value channel volume 100 expression 127 pan 64 power-on defaults (results: <https://dtm.noyu.me/wiki/GM>, <https://git.ardour.org/ardour/ardour/commit/d42381078461857b26b12c7a87a7c0e254ce6c5f>)
- half pedaling MIDI CC64 continuous values digital piano sample library supports half pedal (results: <https://discuss.cakewalk.com/topic/5801-how-to-input-sustain-pedal-in-prv>, <https://forums.steinberg.net/t/help-with-sustain-pedal-and-cc64/623771>)
- legato pedaling technique piano change pedal after the new chord lift press syncopated pedaling (results: the Yamaha page above, <https://livingpianos.com/?p=4332>)
- MIDI pitch bend message E0 LSB first then MSB 0x2000 centre 14-bit specification midi.org (results: <https://www.recordingblogs.com/node/358>)
- guitar string bend half step whole step most common bends lesson (results: <https://www.guitarplayer.com/lessons/string-bending-masterclass-how-to-make-your-guitar-wail-and-sing-like-the-pros>, <https://www.fretjam.com/string-bending.html>)
- CC11 expression vs CC7 volume use expression for swells crescendo set volume once for mix level orchestral MIDI mockup (results: <https://forums.musicplayer.com/topic/182261-cc7-volume-vs-cc11-expression>, <https://forum.vsl.co.at/post/199007>)
- modulation wheel CC1 vibrato General MIDI default; orchestral sample libraries CC1 dynamics crossfade (results: <https://www.cmtext.com/MIDI/chapter3_controller_change.php>, the Orchestral Tools page above)
- Standard MIDI File meta event FF 58 04 nn dd cc bb time signature MIDI clocks per metronome click 6/8 36 tempo microseconds per quarter note (results: <https://midi.org/community/midi-specifications/time-signature-understanding>; the 6/8 example FF 58 04 06 03 24 08, 36 clocks per dotted quarter; the tempo part was checked on the tempo page above)

## Drum triggers: "How drum triggers work" (Hits to MIDI)

Guide key: triggers

**Pages read**

- <https://en.wikipedia.org/wiki/Onset_(audio)>
- <https://github.com/openjdk/jdk/blob/master/src/java.desktop/share/classes/com/sun/media/sound/ModelStandardTransform.java> (the Java sound synthesizer's DLS-style concave transform, 20 log10 (127² / value²) dB: the same curve as 40 log10 (127 / velocity))
- <https://help.apple.com/logicpro/mac/10.1/en.lproj/lgcpc6df7141.html>
- <https://midi.org/community/midi-specifications/how-do-controllers-calculate-velocity>
- <https://static.roland.com/manuals/SPD-SX_PRO_reference_v110/eng/83759072.html>
- <https://www.soundonsound.com/techniques/how-replace-reinforce-recorded-drums>

**Searched for**

- Sound On Sound drum replacement triggering techniques phase layering bleed retrigger (results: <https://www.soundonsound.com/reviews/steven-slate-trigger>, <https://recording.org/comment/drum-replacement-how-do-pros-do-it>)
- Slate Trigger 2 manual threshold retrigger sensitivity detail parameters ms (results: <https://www.mixonline.com/?p=60177>)
- DLS General MIDI velocity to attenuation 40 log10 velocity curve
- DLS level 1 velocity to attenuation concave transform 40 log10(127/velocity) dB (no relevant results)
- Downloadable Sounds MIDI velocity attenuation "40 log" 127 velocity curve GM2 (results: <https://midi.org/community/midi-specifications/how-do-controllers-calculate-velocity>, <https://www.soundonsound.com/sound-advice/q-which-midi-velocity-curve-should-use-my-controller-keyboard>)
- CONN_TRN_CONCAVE DLS velocity to gain concave transform 96 dB log value squared range squared (results: the OpenJDK `ModelStandardTransform` above)
- MIDI 1.0 specification "application of velocity to volume should be an exponential function" (results: <https://www.lim.di.unimi.it/IEEE/MIDI/SOT1.HTM>, the midi.org thread above): MIDI 1.0 leaves velocity to the receiver and only prefers an exponential velocity-to-volume curve
- Bello Daudet 2005 "A tutorial on onset detection in music signals" spectral flux peak picking threshold transient definition
- Logic Pro quantize Q-Strength percentage parameter explained (results: <https://help.apple.com/logicpro/mac/10.4/en.lproj/lgcpf7c0af60.html>)
- drum replacer "retrigger" time ms setting snare roll double trigger flam typical value
- General MIDI Level 1 percussion key map 35 Acoustic Bass Drum 36 Bass Drum 1 38 Acoustic Snare 42 Closed Hi-Hat channel 10 (results: <https://en.wikipedia.org/wiki/General_MIDI>)

## Reading music: "How to read music" (the score view and sheet music PDF)

Guide key: readMusic

- <https://abcnotation.com/wiki/abc%3Astandard%3Av2.1> (ABC notes, chord labels, ties and tuplets; search: abc notation 2.1 standard accidentals ties tuplets)

**Pages read**

- <https://en.wikipedia.org/wiki/Clef>
- <https://en.wikipedia.org/wiki/Key_signature>
- <https://en.wikipedia.org/wiki/Accidental_(music)>
- <https://en.wikipedia.org/wiki/Stem_(music)>
- <https://en.wikipedia.org/wiki/Beam_(music)>
- <https://en.wikipedia.org/wiki/Rest_(music)>
- <https://human.libretexts.org/Bookshelves/Music/Music_Theory/Open_Music_Theory_2e_(Gotham_et_al.)/01%3A_Fundamentals/1.03%3A_Reading_Clefs>
- <https://human.libretexts.org/Bookshelves/Music/Music_Theory/Open_Music_Theory_2e_(Gotham_et_al.)/01%3A_Fundamentals/1.04%3A_The_Keyboard_and_the_Grand_Staff>
- <https://online.berklee.edu/takenote/imaginary-barlines/>
- <https://music-theory-practice.com/reference/clef-mnemonics>
- <https://openbooks.macewan.ca/introtomusictheory/chapter/major-key-signatures/>
- <https://en.wikipedia.org/wiki/Time_signature> (cut time; for time signatures other than 4/4, see the Rhythm heading)
- <https://www.open.edu/openlearn/history-the-arts/music/an-introduction-music-theory/content-section-3.9> (beaming in threes in 6/8 and 12/8)

**Searched for**

- notation rule 4/4 "middle of the bar" show beat 3 syncopation tie Gould Behind Bars (results: <https://musescore.org/en/node/351959>, Gould's *Behind Bars* p. 167 as quoted there)
- treble clef lines mnemonic "Every Good Boy Deserves Football" bass clef "Good Boys Deserve Fudge Always" "All Cows Eat Grass" (results: <https://music-theory-practice.com/reference/clef-mnemonics>, <https://www.dummies.com/article/academics-the-arts/music/instruments/piano/size-up-the-staff-how-to-read-treble-and-bass-clef-155363>)
- key signature written at the start of every staff line, time signature only at the beginning (results: <https://human.libretexts.org/Bookshelves/Music/Music_Theory/Understanding_Basic_Music_Theory_(Schmidt-Jones)/02%3A_Notation_-_Time/2.03%3A_Time_Signature>)
- whole rest hangs from the fourth line half rest sits on the middle line staff (results: <https://www.dummies.com/article/academics-the-arts/music/general-music/examining-rests-in-musical-notation-200164/>, <https://hellomusictheory.com/learn/rests/>)
- bass guitar notation sounds an octave lower than written bass clef transposing instrument (results: <https://www.dummies.com/article/academics-the-arts/music/instruments/bass-guitar/how-to-read-notation-to-play-the-bass-guitar-154804/>)
- bass clef lines mnemonic "Good Boys Deserve Fudge Always" (results: <https://music-theory-practice.com/reference/clef-mnemonics>, <https://barrypopik.com/blog/good_boys_do_fine_always>: "Good Boys Do Fine Always" is the more common form)
- order of sharps mnemonic "Father Charles Goes Down And Ends Battle" (results: <https://openbooks.macewan.ca/introtomusictheory/chapter/major-key-signatures/>, <https://music-theory-practice.com/reference/order-of-sharps-and-flats>)
- "Every Good Boy Deserves Football" treble clef (results: <https://en.wikipedia.org/wiki/Clef>, <https://teoria.com/en/tutorials/reading/12-notes.php>)

## Slicer: "What is a slicer?"

Guide key: slicer

**Pages read**

- <https://www.ableton.com/en/manual/live-audio-effect-reference/> (Beat Repeat)
- <https://www.soundonsound.com/techniques/removing-clicks-crossfades-sonar>
- <https://www.soundbridge.io/use-the-trance-gate-effect-to-alter-the-rhythm>

**Searched for**

- editing audio click discontinuity zero crossing short fade few milliseconds avoid clicks
- Beatles "Rain" 1966 first backwards vocals reverse tape "I'm Only Sleeping" backwards guitar (results: beatlesbible.com, en.wikipedia.org/wiki/Backmasking)
- trance gate effect explained rhythmic gating pads sixteenth notes step pattern
- trap hi-hat rolls 1/32 notes triplets ratchet explained (results: emastered.com, unison.audio)

## Step filter: "What is a step filter?"

Guide key: stepFilter

**Pages read**

- <https://musictech.com/tutorials/get-to-grips-with-step-fx-in-logic-pro-x/>
- <https://www.soundonsound.com/node/4919135> (Step FX In Logic Pro X, 2018)
- <https://static.roland.com/manuals/sh-4d_v200/en-US/8685095586235403.html> (Roland SH-4d Step Filter)

**Searched for**

- Logic Pro Step FX plug-in filter step sequencer cutoff
- sample and hold filter cutoff random stepped synth sound explained (results: sweetwater.com, docs.cherryaudio.com)
- zipper noise parameter smoothing filter cutoff abrupt change click
- step filter effect rhythmic filter sequencer plugin pads bass how to use cutoff per step (results: steinberg.help StepFilter reference, kvraudio.com THE_PATTERN)
- filter house French house low-pass filter sweep disco samples Daft Punk technique (results: en.wikipedia.org/wiki/French_house, splice.com)

## Key finding: "Finding the key and chords" (Detect key and chords of a MIDI clip)

Guide key: keyFinding

**Pages read**

- <https://en.wikipedia.org/wiki/Mode_(music)>
- <https://en.wikipedia.org/wiki/Minor_scale>
- <https://en.wikipedia.org/wiki/Blues_scale>
- <https://en.wikipedia.org/wiki/Pentatonic_scale>
- <https://en.wikipedia.org/wiki/Slash_chord>
- <https://en.wikipedia.org/wiki/Inversion_(music)>
- <https://en.wikipedia.org/wiki/Nonchord_tone>
- <https://en.wikipedia.org/wiki/Suspended_chord>
- <https://en.wikipedia.org/wiki/Diminished_seventh_chord>
- <https://www.mtosmt.org/issues/mto.04.10.2/mto.04.10.2.aarden_hippel_6.2.1.html> (Aarden and von Hippel, on the Krumhansl and Kessler profiles)

**Searched for**

- Krumhansl Kessler key profiles major 6.35 2.23 3.48 minor 6.33 2.68 3.52 (results: fastled.io key profiles, partitura key_identification, dolmetsch.com)
- MIREX key detection evaluation weighted score relative key 0.3 fifth 0.5 parallel 0.2 (results: arxiv.org/pdf/1706.02921, mir-eval.readthedocs.io key)
- Krumhansl-Schmuckler key-finding algorithm input vector durations of each pitch class correlation 24 keys Temperley (results: dept-info.labri.fr robine-RRH.ICMC08, music.informatics.indiana.edu temperley.pdf, essentia.upf.edu std_Key)

## Spectral freeze: "What is a spectral freeze?"

Guide key: spectralFreeze

**Pages read**

- <https://cycling74.com/tutorials/the-phase-vocoder-part-ii> (Dudas and Lippe: freezing, random phase deviation)
- <https://ccrma.stanford.edu/~jos/sasp/Resolving_Sinusoids.html> (Smith, Spectral Audio Signal Processing: Hann main lobe)
- <https://en.wikipedia.org/wiki/Spectral_flatness>
- <https://en.wikipedia.org/wiki/Phase_vocoder>
- <https://www.ableton.com/en/manual/live-audio-effect-reference/> (Ableton Live manual, Spectral Time: the Freezer section with Manual and Retrigger modes, Onsets and Sync)

**Searched for**

- Harris 1978 window functions Hann 3 dB bandwidth 1.44 bins 6 dB bandwidth 2.00 highest sidelobe -31.5 dB (results: metricgate.com windowing-function, apps.dtic.mil ADA085032)
- spectral freeze phase vocoder tutorial Charles Computer Music Journal 2008 freezing a sound (results: cycling74.com phase vocoder parts I and II, Charles spectral tutorials)
- Paulstretch algorithm randomizes phases of each frequency bin extreme time stretch Nasca (results: csound.com paulstretch manual)
- Laroche Dolson 1999 improved phase vocoder time-scale modification phasiness (results: ee.columbia.edu LaroD99-pvoc.pdf, LaroD97-phasiness.pdf)
- Julius Smith Spectral Audio Signal Processing Hann window main lobe width 4 bins resolve sinusoids (results: ccrma.stanford.edu jos/sasp)
- Ableton Live Spectral Time device Freeze section (results: ableton.com blog Freeze, Delay and Deconstruct)
- phase vocoder true frequency of a bin from phase difference between frames hop size (results: cycling74.com the phase vocoder part I)
- sampler loop points click zero crossing crossfade loop seam (results: muzines.co.uk The Art of Looping, forum.hise.audio)
- Ableton Live manual Spectral Time Freeze section Retrigger Sync (results: ableton.com blog Freeze, Delay and Deconstruct, musicradar.com Spectral Time pads)

## Arpeggiator: "What is an arpeggiator?"

Guide key: arpeggiator

**Pages read**

- <https://en.wikipedia.org/wiki/Arpeggio>
- <https://en.wikipedia.org/wiki/Arpeggiator> (redirects to Synthesizer: speed, range and mode; the 1/16 trance lead)
- <https://en.wikipedia.org/wiki/Alberti_bass>
- <https://en.wikipedia.org/wiki/Tresillo_(rhythm)>
- <https://en.wikipedia.org/wiki/Syncopation>
- <https://en.wikipedia.org/wiki/Polymeter>
- <https://www.ableton.com/en/manual/live-midi-effect-reference/> (Arpeggiator: Play Order, Chord Trigger, Gate above 100%, Hold)
- <https://brettworks.com/2013/07/23/roger-linn-on-drum-machine-groove-and-j-dillas-off-beat-sound/> (Roger Linn on swing: 50%, 54%, 66%)
- <https://en.wikipedia.org/wiki/Roland_Juno-60> (released September 1982)
- <https://www.musicradar.com/news/overused-production-technique-trance-gate> (a gate chops a sustained sound into a stutter)
- <https://www.andyguitar.co.uk/videos/02-int-elec-l2-the-charleston-rhythm-aka-the-cant-buy-me-love-beat> (Charleston: hits on beat 1 and the "and" of 2)
- <https://www.guitarworld.com/lessons/how-fingerpick-merle-travis> (Travis picking: steady alternating thumb bass, fingers higher)
- <https://en.wikipedia.org/wiki/Charleston_(1923_song)> (the Charleston rhythm, a dotted quarter and an eighth, named after James P. Johnson's 1923 tune; widely used in jazz comping)

**Searched for**

- classic arpeggiator patterns trance synthwave house pop gated rhythms chord stabs bass and chord pattern (results: musicradar.com trance gate, rekkerd.org, loopcloud.com)
- Charleston rhythm beat 1 and the and of 2; dotted eighth note delay three sixteenths (results: andyguitar.co.uk, nationalguitaracademy.com, musicradar.com The Edge)
- Travis picking alternating bass thumb pattern (results: guitarworld.com, fender.com)
- Travis picking alternating bass thumb pattern; waltz oom-pah-pah bass chord chord (results: <https://www.fender.com/articles/how-to/travis-picking-on-guitar>: the thumb alternates between bass strings while the fingers play higher; <https://www.soundbrenner.com/blogs/articles/waltz-rhythm>, <https://accordionlifeacademy.com/articles/7-bass-patterns-to-play-on-your-accordion>: a waltz is bass on beat 1 and the chord on 2 and 3)
- Charleston rhythm dotted quarter eighth named after 1923 song James P. Johnson comping (results: en.wikipedia.org Charleston (1923 song) and Charleston (dance), jazzguitarlessons.net comping rhythms, moderndrummer.com)
- Roland Juno-60 arpeggiator modes up down up/down range 1 2 3 octaves hold rate (results: support.roland.com Juno-60 technical specifications, Roland JU-06A manual)
- Roger Linn swing percentage MPC 50% straight 66% triplet sixteenth notes explained (results: attackmagazine.com swing in DAWs, melodiefabriek.com MPC swing)
- Roland Juno-60 1982 arpeggiator up down up&down range 1 2 3 hold (results: support.roland.com Juno-60 technical specifications: MODE UP / UP & DOWN / DOWN, RANGE 1 / 2 / 3, RATE 1.5 to 50 Hz; Roland JU-06A manual)

## Harmoniser: "What is a harmoniser?"

Guide key: harmoniser

**Pages read**

- <https://www.mixonline.com/technology/1975-eventide-h910-harmonizer-377972> (Eventide H910 Harmonizer, 1975, ±1 octave, up to 112.5 ms delay)
- <https://www.soundonsound.com/reviews/eventide-micropitch> (doubling from two small opposing shifts of a few cents and a delay of a few tens of milliseconds)
- <https://cdn.eventideaudio.com/manuals/h9000/2.0/content/appendix/algorithms/93_PitchFactor.html> (diatonic shift by key and scale, adjusted so the result is in key; monophonic tracking, best on single notes)
- <https://human.libretexts.org/Bookshelves/Music/Music_Theory/Open_Music_Theory_2e_(Gotham_et_al.)/02%3A_Counterpoint_and_Galant_Schemas/2.01%3A_Introduction_to_Species_Counterpoint> (perfect and imperfect consonances; parallel fifths and octaves avoided)
- <https://en.wikipedia.org/wiki/Precedence_effect> (a reflection 5 to 30 ms late fuses with the sound; echo from about 50 ms for speech)
- <https://en.wikipedia.org/wiki/Automatic_double_tracking> (Ken Townsend, Abbey Road, 1966, *Revolver*)
- <https://support.roland.com/hc/en-us/articles/4514667135003-PS-6-Harmonist-Perfect-5th-vs-Diatonic-5th-Pitch-Shift-vs-MAJOR-MINOR> (could not be read: 403; found by search: a diatonic 5th gives a tritone on one scale degree)

**Searched for**

- Eventide H910 Harmonizer 1975 first trademark "Harmonizer" (results: en.wikipedia.org Eventide, mixonline.com, sweetwater.com; trademark filed 10 November 1976)
- "Harmonizer is a registered trademark of Eventide" (results: Eventide manuals, trademarks.justia.com, sweetwater.com)
- vocal doubling micro pitch shift detune cents delay milliseconds Sound on Sound (results: soundonsound.com MicroPitch review; vintagedigital.com.au: about ±4 to 9 cents and 8 to 20 ms for vocals)
- diatonic harmony pitch shifter scale mode how notes outside the key are handled (results: Eventide PitchFactor, Lexicon MPX G2 carries the last interval, Digitech SmartShift: E in C major gives G and B, Roland PS-6)
- open music theory consonance dissonance perfect imperfect consonances parallel fifths octaves (results: Open Music Theory via LibreTexts)
- TD-PSOLA pitch modification factor range quality degrades large shifts (results: TAMU speech lecture: up to a factor of two; arXiv 2512.16519: most stable within ±6 semitones)
- precedence effect echo threshold delay under 30 ms fuses (results: en.wikipedia.org Precedence effect, Litovsky et al. 1999)
- doubling delay time ms vocal 20 to 50 ms short delay double tracking (results: sonarworks.com vocal doubling tips: keep it under about 30 ms or it sounds like an echo; prosoundweb.com thickening up tracks with doubling effects: start at 25 ms; lynda.com double-tracking: 20 or 30 ms)

## Delay: "What is delay?"

Guide key: delay

The history (tape echo, the Echoplex, the Echorec's magnetic drum, the Space Echo, bucket-brigade chips and digital delays), the echo threshold and the Haas effect come from the pages below. The Echoplex was first made in 1959 and in shops from 1961. The numbers are worked out: an echo from a wall d metres away comes after 2 × d / 343 seconds (17 m gives 0.1 s); a beat lasts 60,000 / BPM ms; at 50% feedback each echo is 20 × log10 0.5 = -6 dB under the last, so it takes about 10 echoes to fade by 60 dB; the first notch of a comb filter is at 1 / (2 × the delay). The times for slapback, doubling, widening, flanging and chorus are common values that differ between sources: slapback is given anywhere from 60 to 250 ms, doubling from 20 to 50 ms.

**Pages read**

- <https://en.wikipedia.org/wiki/Precedence_effect> (a copy fuses with the sound below about 30 to 40 ms; an echo from about 50 ms for speech; Helmut Haas described it in his 1949 thesis)
- <https://en.wikipedia.org/wiki/Automatic_double_tracking> (Ken Townsend, Abbey Road, 1966: a short tape delay instead of singing a part twice)
- <https://www.izotope.com/en/learn/basic-vocal-doubling-techniques> (below about 30 ms the ear fuses two near-identical sounds into one; beyond that the copy starts to sound like an echo)
- <https://www.premierguitar.com/articles/22230-electric-etudes-u2s-the-edge> (The Edge: eighth notes into a dotted-eighth delay sound like a stream of sixteenths)
- <https://www.sweetwater.com/insync/bpm-delay-times-cheat-sheet/> (a dotted eighth is three sixteenths)
- <https://en.wikipedia.org/wiki/Delay_(audio_effect)> (slapback: Sam Phillips with Elvis Presley in 1954, on two tape machines, 60-250 ms with little or no feedback; the bucket-brigade chip from Philips in 1969, the Memory Man pedal in 1976; digital delays in studio racks from 1971, the first digital delay pedal in 1984)
- <https://en.wikipedia.org/wiki/Binson_Echorec> (it stores the sound on a magnetic drum, not a tape loop)
- <https://en.wikipedia.org/wiki/Roland_RE-201> (the Space Echo of 1974: one recording head and three playback heads)
- <https://en.wikipedia.org/wiki/Bucket-brigade_device> (invented at Philips in 1969; audio chips with 512 to 4096 stages)
- <https://en.wikipedia.org/wiki/Flanging> (a short delay, usually under 20 ms, slowly changing: a swept comb filter)
- <https://en.wikipedia.org/wiki/Chorus_effect> (like a flanger, with longer delays)
- <https://en.wikipedia.org/wiki/Speed_of_sound> (343 m/s in air at 20 °C)

**Searches**

- doubling delay time ms vocal 20 to 50 ms short delay double tracking (results: sonarworks.com vocal doubling tips: keep it under about 30 ms or it sounds like an echo; prosoundweb.com thickening up tracks with doubling effects: start at 25 ms; lynda.com double-tracking: 20 or 30 ms)
- precedence effect echo threshold delay under 30 ms fuses into one sound doubling ADT (results: <https://en.wikipedia.org/wiki/Precedence_effect>, <https://www.cmu.edu/dietrich/psychology/shinn/publications/pdfs/2001/2001jasa_litovsky.pdf>: below the echo threshold a copy fuses into one sound; the threshold depends on the sound, a few ms for clicks and tens of ms for speech and music)
- Charleston rhythm beat 1 and "and" of 2 syncopated pattern; dotted eighth note delay three sixteenths pattern The Edge (results: <https://www.premierguitar.com/articles/22230-electric-etudes-u2s-the-edge>, <https://musicradar.com/news/the-edges-guitar-tip-for-using-delay-can-make-you-a-tighter-player>, <https://articles.boss.info/patch-work-where-the-streets-have-no-name-by-u2/>)
- artificial double tracking ADT Ken Townsend 1966 Abbey Road short tape delay instead of singing a second take (results: <https://en.wikipedia.org/wiki/Automatic_double_tracking>, <https://en.wikipedia.org/wiki/Ken_Townsend>)
- Echoplex tape delay first made 1959 Mike Battle Market Electronics (results: <https://en.wikipedia.org/wiki/Echoplex>, <https://loopersdelight.com/musings/Mike_Battle/Mike_Battle.html>: first made in 1959; <https://www.musicradar.com/news/a-brief-history-of-tape-delay-and-4-great-plugin-emulations>: in shops from 1961)
- first analog delay pedal bucket brigade Electro-Harmonix Memory Man 1976 (results: <https://www.musicradar.com/news/a-brief-history-of-bucket-brigade-delays-and-4-great-plugin-emulations>, <https://www.guitarworld.com/gear/guitar-pedals/ehx-deluxe-memory-man>: bucket-brigade chips became affordable in the mid-1970s, and many analog delay pedals came out around 1976)

## Rhythm: "Note lengths, beats and bars" (time signatures)

Guide key: rhythm

Note names, dots, triplets and counting are standard music notation, from the pages below. The swing ratio is a rule of thumb: players swing wider at slow tempos and nearer even at fast ones. The tempo ranges of the styles are common values that agree with the Styles guide; sources differ by a few BPM at the edges. The lengths in seconds are worked out (at 120 BPM a beat lasts 60 / 120 = 0.5 seconds).

**Pages read**

- <https://en.wikipedia.org/wiki/Time_signature> (top and bottom numbers, simple and compound metres, common and cut time, odd groupings, examples: Take Five 5/4, Paranoid Android 7/8, Blue Rondo à la Turk 9/8 as 2+2+2+3, slow blues 12/8, waltzes 3/4, jigs 6/8)
- <https://www.open.edu/openlearn/history-the-arts/music/an-introduction-music-theory/content-section-3.9> (compound time: the dotted-crotchet beat, quavers beamed in threes, beaming in twos suggests 3/4)
- <https://www.manualsdir.com/manuals/547962/apple-logic-pro-9.html?page=176> (Logic Pro displays tempo as quarter notes per minute)
- <https://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midifile/time.htm> (SMF time signature FF 58 04 nn dd cc bb; no event means 4/4)
- <https://en.wikipedia.org/wiki/Note_value> (whole to thirty-second notes and their British names, semibreve to demisemiquaver; a dot adds half the value)
- <https://en.wikipedia.org/wiki/Counting_(music)> (1 & 2 &, 1 e & a, triplets as 1 trip let or 1 la li, 6/8 counted in two)
- <https://en.wikipedia.org/wiki/Swing_(jazz_performance_style)> (swung eighths long-short, about 2:1 like a triplet, wider at slow tempos and narrower at fast ones)
- <https://learningmusic.ableton.com/make-beats/tempo-and-genre.html> (tempo ranges by style: hip-hop 60 to 100, house 115 to 130, techno and trance 120 to 140, dubstep 135 to 145, drum and bass 160 to 180)

**Searched for**

- Standard MIDI File meta event FF 58 04 nn dd cc bb time signature MIDI clocks per metronome click 6/8 36 (results: the SMF 1.0 example FF 58 04 06 03 24 08, 36 clocks per dotted quarter; <https://midi.org/community/midi-specifications/time-signature-understanding>)
- Logic Pro tempo always quarter notes per minute 6/8 time signature BPM dotted quarter (results: <https://discussions.apple.com/thread/2380832>, <https://logic-users-group.com/threads/how-do-i-create-a-click-that-counts-the-dotted-quarter-note.3300/>)
- Open Music Theory simple and compound meter beaming eighth notes groups of three 6/8 dotted quarter beat (results: <https://pressbooks.nebraska.edu/openmusictheory/?p=155>)
- openlearn "Grouping rests in compound time" dotted crotchet rest whole beat 6/8 (results: OpenLearn 3.8: 6/4 is compound duple, six crotchets in two groups of three; <https://musescore.org/node/4867>: little agreement on rests in compound metre)
- shuffle rhythm notated 12/8 or 4/4 with triplets swung eighths slow blues (results: <https://bluesharmonica.com/time_signature_my_blues>, <https://www.bluesharmonica.com/counting_rhythms_shuffle_grooves>: slow blues often written in 12/8, sounding the same as a 4/4 shuffle)
- cut time 2/2 notation show half-bar beat tie syncopation beaming eighths in fours (theory check: results <https://en.wikipedia.org/wiki/Alla_breve>, <https://human.libretexts.org/Bookshelves/Music/Music_Theory/Music_Theory_for_the_21st-Century_Classroom_(Hutchinson)/04%3A_Basics_of_Rhythm/4.06%3A_Common_Rhythmic_Notation_Errors>: 2/2 has two half-note beats and notation shows the beat, so DawIt's tie at the half bar in 2/2 follows the same rule as beat 3 in 4/4; no source found that names eighths beamed in fours in 2/2 directly)
- typical tempo BPM techno, trance, dubstep 140 half-time 70, house 120-128 (results: <https://www.izotope.com/en/learn/using-different-tempos-to-make-beats-for-different-genres>, <https://digitaldjpool.com/blog/dj-bpm-guide/>: dubstep at 140 felt as 70; house, techno and trance ranges overlap and differ by source)

## Chords: "Chord progressions"

Guide key: chords

The chords of a key, the Roman numerals, the circle of fifths and the key signatures follow from the scales themselves, and the guide's tests check them (C major's neighbours are G and F; E major has 4 sharps, F#, C#, G# and D#). The names of the progressions, the cadences and the claims about where they are heard come from the pages below. "Home, away and tension" are DawIt's plain names for what theory books call tonic, subdominant (or predominant) and dominant chords.

**Pages read**

- <https://en.wikipedia.org/wiki/Cadence> (five to one, the authentic cadence, is generally considered the strongest cadence; four to one is the plagal or "amen" cadence; five to six is the deceptive cadence; a phrase that stops on five, a half cadence, calls for more)
- <https://en.wikipedia.org/wiki/I%E2%80%93V%E2%80%93vi%E2%80%93IV_progression> (I–V–vi–IV is a common progression across several genres; the article doesn't call it the most common, so the guide says "one of the most used"; vi–IV–I–V is the same loop started on the minor chord)
- <https://en.wikipedia.org/wiki/%2750s_progression> (I–vi–IV–V, common in the 1950s and early 1960s and tied to doo-wop)
- <https://en.wikipedia.org/wiki/Pachelbel%27s_Canon> (the eight chords I–V–vi–iii–IV–I–IV–V over a repeating bass, and pop songs that borrow them)
- <https://en.wikipedia.org/wiki/Romanesca> (the Canon's pattern: a bass that moves by fourths under a falling tune; so the guide no longer says the bass walks down step by step)
- <https://en.wikipedia.org/wiki/Twelve-bar_blues> (I I I I, IV IV I I, V IV I, with V in bar 12 as the turnaround; seventh chords are common throughout)
- <https://en.wikipedia.org/wiki/Andalusian_cadence> (i–VII–VI–V with a stepwise falling bass and a major five chord; heard in flamenco and in rock and pop)
- <https://en.wikipedia.org/wiki/Circle_of_fifths> (sharps clockwise, flats anticlockwise, F# and Gb at the bottom, relative minors on the inner ring, neighbours are closely related keys, chords often move down by fifths)

**Searched for**

- Pachelbel Canon ground bass D A B F# G D G A upper voice descends stepwise (results: <https://wikipedia.com/wiki/Canon_in_D>: the bass repeats D A B F# G D G A)
- perfect authentic cadence V-I strongest cadence; plagal "amen" cadence; deceptive cadence V-vi (results: <https://www.musicnotes.com/blog/cadences-in-music-theory-the-4-types-explained>, <https://flypaper.soundfly.com/write/using-cadences-in-composition/>)

## Chord shapes: "Piano and guitar chords"

Guide key: chordShapes

DawIt works out the piano voicings and the guitar shapes itself, from the chord's notes and the guitar's standard tuning, so every shape holds only the chord's notes, with the root lowest. The tests check that the common open chords come out as guitarists play them (C x32010, Am x02210, Em 022000, D xx0232, E 022100, A x02220, F as the full barre 133211). The tunings, the shapes and the way chord boxes are read come from the pages below.

**Pages read**

- <https://en.wikipedia.org/wiki/Guitar_chord> (standard tuning E A D G B E; the open shapes of C, A, G, E, D, Am, Em and Dm, the F barre 133211 as the E shape one fret up; barre shapes are movable; extended chords often leave out the fifth)

**Searched for**

- how to read a guitar chord diagram low E string on the left, o open string, x don't play, finger numbers (results: <https://www.fender.com/articles/chords/how-to-read-a-chord-chart>, <https://www.dummies.com/article/academics-the-arts/music/instruments/guitar/how-to-read-a-guitar-chord-diagram-198024/>: strings left to right from the low E, o open, x not played, fingers 1 index to 4 little)
- first open chords beginners learn guitar C A G E D Am Em Dm (results: <https://justinguitar.com/guitar-lessons/the-8-essential-beginner-chord-grips-b1-701>, <https://www.musicradar.com/how-to/guitar-basics-getting-started-with-open-chords>: these eight are the usual first chords)

## Structure: "Song structure"

Guide key: structure

Section lengths are typical values, not rules: songwriters and producers mostly work in 4, 8 and 16 bars, and DJs mix in phrases of 8, 16 and 32 bars, but real songs vary. The time to the first chorus is a measured average for one year of hits. The bar counts and durations of each structure are worked out from its sections and tempo (the pop song, 64 bars at 110 BPM: 64 × 4 × 60 / 110 = 140 seconds, about 2:20).

**Pages read**

- <https://en.wikipedia.org/wiki/Verse%E2%80%93chorus_form> (verse–chorus has been the main form of rock since the 1960s, after AABA; it goes back to the 1840s)
- <https://en.wikipedia.org/wiki/Thirty-two-bar_form> (AABA: four 8-bar sections, the B also called the bridge or middle eight; the main form of American popular song from about 1925 into the 1950s; many show tunes that became jazz standards use it; the title often on the first or last line of the A)
- <https://en.wikipedia.org/wiki/Twelve-bar_blues> (the 12-bar chord pattern, the same as in Chord progressions)
- <https://reports.hitsongsdeconstructed.com/articles/hip-hop-vs-pop-which-hooks-you-faster/> (top-10 hits of 2017: pop reached the first chorus after 39 seconds on average, hip-hop after 30)
- <https://en.wikipedia.org/wiki/Drum_and_bass> (the tempo, as in Styles)

**Searched for**

- rap verse 16 bars hip hop song structure hook 8 bars (results: <https://emastered.com/blog/rap-song-structure>, <https://hiphopmakers.com/how-to-make-beats-song-arrangements>: 16-bar verses and 4- or 8-bar hooks are the convention, not a rule)
- "hot 16" rap slang sixteen-bar verse (results: <https://www.rhymebook.com/dictionary/16>, <https://www.dictionary.com/e/slang/bars/>: "a 16" means a rap verse)
- drum and bass arrangement 32 bar intro DJ mix 32 bar drop (results: <https://www.redbull.com/gb-en/how-to-remix-drum-n-bass>: sections mostly 16 or 32 bars; <https://mixedinkey.com/captain-plugins/wiki/how-to-arrange-a-dance-music-track/>)
- DJ phrasing 8 16 32 bars dance music intro outro drums only (results: <https://www.mixgraph.io/glossary/intro>, <https://www.mixgraph.io/glossary/outro>: intros and outros of mostly drums, so DJs can mix in and out)
- drum and bass tempo 170 to 180 BPM typically 174 (results: <https://www.edmprod.com/what-is-drum-and-bass/>: 170 to 180 BPM, 174 the most common)
- Hit Songs Deconstructed time to first chorus average pop hits (results: <https://reports.hitsongsdeconstructed.com/articles/the-hit-songs-deconstructed-trend-report/>: averages between about 30 and 42 seconds in the 2010s)

## Gain staging: "Gain staging"

Guide key: gainStaging

The sums in *Through a bus* are worked out, as in Buses, sends and routing: different sounds add by power, 10 × log10 2 = 3.01 dB for two and 10 × log10 4 = 6.02 dB for four; copies of one sound in step add by amplitude, 20 × log10 2 = 6.02 dB for two and 20 × log10 4 = 12.04 dB for four. So four drum tracks with peaks at -10 dB reach -10 + 12.04 = +2.04 dB when their hits line up, and about -10 + 6.02 = -3.98 dB on average. The colours (quiet below -24 dB, hot above -6 dB) are DawIt's own choice. The typical levels (peaks of -18 to -6 dB into effects, -6 to -3 dB on the master while mixing, faders within about 10 dB of 0 dB) are common studio practice from the pages below, not a standard; engineers differ by a few dB, and some aim for an average, not a peak, near -18 dB. That even about 1 dB louder can sway a comparison is widely given advice; no study found pins down the exact amount.

**Pages read**

- <https://en.wikipedia.org/wiki/Gain_stage> (gain staging is managing the level at each step of the signal flow, so each one works in the best part of its range)
- <https://www.soundonsound.com/node/4926883> (Sound On Sound, gain staging in Logic Pro: some engineers keep each channel's peaks at or below -18 dBFS, a level plugins modelled on analog gear are made for; check each plugin's manual; leave the final mix several dB below 0 dBFS)
- <https://en.wikipedia.org/wiki/Clipping_(audio)> (a digital signal is cut off at the largest value its format can hold; hard clipping adds harsh high harmonics)
- <https://www.production-expert.com/production-expert-1/32-bit-floating-point-audio-the-case-for-using-it> (32-bit float holds about 770 dB above 0 dBFS; peaks over 0 dBFS come back clean when turned down; a 16- or 24-bit copy clips unless the peaks are brought down first)
- <https://www.soundonsound.com/reviews/meterplugs-perception> (louder sounds better and hides small faults, so compare processing at matched loudness)
- <https://audient.com/tutorial/the-beginners-guide-to-headroom> (headroom, as in Buses, sends and routing)
- <https://www.waves.com/how-loud-mix-before-mastering> (master peaks of about -6 to -3 dB before mastering, as in Buses, sends and routing)
- <https://www.masteringbox.com/learn/headroom-for-mastering> (the same)
- <https://sengpielaudio.com/calculator-coherentsources.htm> (different sounds add 3 dB, copies in step 6 dB, as in Buses, sends and routing)

**Searched for**

- gain staging DAW plugins -18 dBFS 0 VU analog modelled plugins peaks (results: <https://slatedigital.com/what-is-gain-staging-and-why-your-mix-sounds-muddy/>, <https://borisfx.com/blog/gain-staging-cheat-sheet/>: an average near -18 dBFS with peaks around -12 to -6 dBFS is a common aim; -18 dBFS = 0 VU is a convention, not a standard)
- 32-bit floating point audio headroom above 0 dBFS, 24-bit fixed point file clips (results: <https://www.masteringbox.com/learn/bit-depth>, <https://slooply.com/blog/how-to-recover-clipped-audio-using-32-bit-float/>: float keeps levels over 0 dBFS inside the DAW; 16- and 24-bit files stop at 0 dBFS)
- louder perceived as better, level matching 1 dB A/B comparison (results: <https://steinberg.help/wavelab_pro/v12/en/wavelab/topics/audio_montage/a_b_comparisons_c.html>, <https://violetrecording.com/how-to-a-b-your-mix/>: louder versions tend to be judged better; matching within about 0.5 dB is the usual advice)
- fader resolution near 0 dB, keep faders within about 10 dB of unity (results: <https://dfdeboer.github.io/HOW_S/fader.html>: with good gain staging most faders end up between -10 and 0 dB, and the bottom of the travel is touchy)

## EQ: "What is EQ?"

Guide key: eq

The names and edges of the frequency ranges differ a little between sources (some split 2 to 6 kHz into upper mids and presence, or call all of 6 to 20 kHz brilliance); DawIt uses one common version. The instrument ranges and problem frequencies are typical starting points from engineers' guides, not exact values: every voice, instrument and recording differs. Note frequencies are worked out from A4 = 440 Hz (A0 = 27.5 Hz, C8 = 4186 Hz, E1 = 41.2 Hz, E2 = 82.4 Hz). The Q widths and the cut slopes are worked out from DawIt EQ's own filters, which follow the Audio EQ Cookbook; Q 0.71 (1/√2) is the Butterworth shape, the flattest without a bump. Daniel Flickinger built a parametric equaliser in 1971; Massenburg's 1972 paper gave it its name.

**Pages read**

- <https://en.wikipedia.org/wiki/Equalization_(audio)> (equalisation began on telephone lines, making the response equal again; low cut, high cut, shelves, peaking bands and notches; Q and bandwidth; Flickinger 1971 and Massenburg's 1972 paper)
- <https://www.w3.org/TR/audio-eq-cookbook/> (the biquad uses the newest input, the two before it and the last two outputs, with five numbers after normalising; a peaking band's width is measured where the gain is half its dB; shelves are halfway at their frequency)
- <https://en.wikipedia.org/wiki/Hearing_range> (20 Hz to 20 kHz; the threshold rises sharply around 15 kHz in adults and the top falls with age; most sensitive between 2 and 5 kHz)
- <https://en.wikipedia.org/wiki/Utility_frequency> (mains at 50 Hz in Europe and large parts of the world, typically 60 Hz in the Americas, so the guide says "most of the Americas")
- <https://en.wikipedia.org/wiki/De-essing> (sibilance lies anywhere from 2 to 10 kHz depending on the voice, so the guide's 5 to 8 kHz is a place to start looking)
- <https://www.soundonsound.com/reviews/meterplugs-perception> (louder sounds better, so compare at matched loudness, as in Gain staging)

**Searched for**

- history of audio equaliser telephone lines, long cables lost high frequencies, Zobel (results: <https://en.wikipedia.org/wiki/Zobel_network>, <https://en.wikipedia.org/wiki/Equalization_(communications)>: early telephone equalisers made up for the highs long cables lost, from Campbell's 1915 wave filter and Zobel's work in the early 1920s)
- just noticeable difference in sound level about 1 dB (results: <https://hyperphysics.gsu.edu/hbase/Sound/db.html>, <https://arxiv.org/pdf/2307.00084>: about 1 dB or less at normal listening levels)
- audio frequency spectrum ranges sub-bass, bass, low mids, mids, presence, brilliance (results: <https://www.teachmeaudio.com/mixing/techniques/audio-spectrum>, <https://www.gearank.com/audio-frequencies/>: sub-bass 20-60 Hz, bass 60-250 Hz, low mids 250-500 Hz, mids 500 Hz-2 kHz; the upper bands differ by source)
- EQ cheat sheet kick beater click, boxy, bass growl, sibilance, vocal low cut (results: <https://cheatography.com/fredv/cheat-sheets/eq-tips>, <https://blog.sonicbids.com/the-ultimate-eq-cheat-sheet-for-every-common-instrument>: the beater click at 2-4 kHz, the cardboard sound at 350-400 Hz, bass growl at about 700 Hz to 1 kHz; numbers vary between guides)
- George Massenburg 1972 AES paper "Parametric Equalization" (results: <https://aes2.org/publications/elibrary-page/?id=16171>: paper K-2 at the 42nd AES Convention, May 1972; <https://en.wikipedia.org/wiki/George_Massenburg>, <https://en.wikipedia.org/wiki/Burgess_Macneal>: designed with Burgess Macneal, first shown in 1971)

## Compressor: "What is compression?"

Guide key: compressor

The history (radio limiters in the 1930s, the LA-2A and the 1176) and the soft knee come from the pages below. Some sources date the LA-2A to 1965 rather than 1962. Compressors define their attack and release times in different ways; DawIt Compressor's are time constants, so after the attack time it has done 1 - 1/e = 63% of its turning down, and after three times as long 95%. The numbers in the examples are worked out from the curve (8 dB over at 4:1 comes out 8 / 4 = 2 dB over; a steady sine wave's RMS is 20 × log10 (1/√2) = -3 dB under its peak). The typical attack, release, ratio and amount for each instrument are common studio practice: engineers differ widely, so take them as starting points.

**Pages read**

- <https://en.wikipedia.org/wiki/Dynamic_range_compression> (10:1 and more is generally called limiting; a soft knee raises the ratio gradually; RMS detection is closer to how loud we hear; parallel or New York compression; no industry standard for attack and release times)
- <https://www.soundonsound.com/music-business/40-ideas-changed-music-part-4-4> (the first commercial radio limiters in 1931; compressors were first used for music in cutting records)
- <https://en.wikipedia.org/wiki/LA-2A_Leveling_Amplifier> (released in 1962; an electroluminescent panel and a photocell do the turning down)
- <https://de.mathworks.com/help/audio/ug/dynamic-range-control.html> (the soft-knee curve of Giannoulis, Massberg and Reiss, which DawIt Compressor uses; MathWorks itself measures attack and release from 10% to 90%)

**Searched for**

- Teletronix LA-2A introduced year Jim Lawrence electro-optical T4 cell history (results: <https://en.wikipedia.org/wiki/LA-2A_Leveling_Amplifier>, <https://www.uaudio.com/blog/la-2a-analog-obsession>: first made in 1962; some sources say 1965)
- Universal Audio 1176 FET limiter introduced 1967 Bill Putnam (results: <https://en.wikipedia.org/wiki/1176_Peak_Limiter>: designed by Bill Putnam, introduced in 1967, an early solid-state limiter with a field-effect transistor and an attack as fast as 20 microseconds)
- history of audio compressor limiter 1930s radio broadcasting Western Electric (results: <https://www.vintagedigital.com.au/1930s-1960s-studio-gear-timeline/>, <https://www.thebdr.net/wp-content/uploads/PDF/Audio/Processing/a-history-of-audio-processing-part-2.pdf>: the Western Electric 110A and RCA 96-A broadcast limiters of the 1930s)
- Giannoulis Massberg Reiss Digital Dynamic Range Compressor Design, a Tutorial and Analysis (results: <https://www.aes.org/e-lib/browse.cfm?elib=16354>: Journal of the Audio Engineering Society, volume 60, issue 6, pages 399-408, June 2012)
- compression settings cheat sheet attack release kick snare vocals bass (results: <https://www.musicguymixing.com/audio-compressor-settings-chart/>, <https://logicstudiotraining.com/essential-compression-settings/>, <https://mastering.com/wp-content/uploads/2017/07/COMPRESSION-CHEAT-SHEET.pdf>: kick attacks from 1 to 30 ms depending on the aim, vocal attacks of about 5-30 ms and releases of about 50-250 ms, 3-6 dB of reduction; guides differ widely)

## Saturation: "What is saturation?"

Guide key: saturation

The history of fuzz and the even and odd harmonics come from the pages below. The intervals of the harmonics, the intermodulation tones (A at 110 Hz with E at 165 Hz leaves new tones every 55 Hz; with C# at 137.5 Hz every 27.5 Hz; with Bb at 117.3 Hz every 7.3 Hz) and the aliasing example (the 5th harmonic of 9.1 kHz is 45.5 kHz, which folds back to 48 - 45.5 = 2.5 kHz at a 48 kHz sample rate) are worked out. A note clipped all the way into a square wave has a THD of 48%, worked out from the square wave's harmonics. How much THD you hear is a rough guide only: it depends on the sound and on which harmonics they are. The pictures, the oversampling filters' delay and the harmonics shown are measured by DawIt itself.

**Pages read**

- <https://en.wikipedia.org/wiki/Distortion_(music)> (Link Wray's 'Rumble', 1958; Dave Davies slashing his speaker for 'You Really Got Me', 1964; the Maestro FZ-1 Fuzz-Tone, 1962, on 'Satisfaction', May 1965)
- <https://en.wikipedia.org/wiki/Fuzz_Face> (released by Arbiter in 1966; Jimi Hendrix its best-known user)
- <https://en.wikipedia.org/wiki/Hearing_range> (the ear is most sensitive between 2 and 5 kHz, as in the EQ guide)

**Searched for**

- magnetic tape maximum output level 3% third harmonic distortion (results: <https://en.wikipedia.org/wiki/Compact_Cassette_tape_types_and_formulations>, <https://www.sweetwater.com/insync/third-harmonic-distortion/>: tape's main distortion is the 3rd harmonic, the odd one a symmetric curve makes, and its limit was measured at 3% of it; the 2nd harmonic is hard to hear because it blends with the note)
- tape saturation odd, single-ended tube even, push-pull cancels even harmonics (results: <https://stereophile.com/content/boulder-500ae-power-amplifier-complementary-symmetry>: the strongest harmonic a single tube makes is the 2nd; a balanced push-pull circuit cancels the even ones)
- clarinet low register odd harmonics (results: <https://phys.unsw.edu.au/music/clarinet/E3.html>, <https://arxiv.org/pdf/physics/0008053>: in the clarinet's low register the odd harmonics are clearly the strongest; the even ones are there, but much weaker)
- total harmonic distortion audibility threshold (results: <https://milbert.com/articles/THD_Power_PerceivedLoudness>: in hi-fi, 1% THD is often called just noticeable and 3% just objectionable; guitar amplifiers are driven far beyond that)
- when digital recording replaced analog tape in studios (results: <https://thebroadcastbridge.com/content/entry/8815/in-the-1990s-audio-recording-changed-forever>, <https://reverb.com/news/how-the-1990s-changed-recording-and-music-production-forever>: digital recording spread through the 1990s, from Pro Tools in 1991 and ADAT the same year)
- Celestion Vintage 30 frequency range (results: <https://celestion.com/productpdf.php?id=888>: a 12-inch guitar speaker rated from 70 Hz to 5 kHz, so the harmonics above about 5 kHz are hardly played)

## Distortion: "What is distortion?"

Guide key: distortion

The dates of the pedals and the people who played them come from the pages below. Overdrive pedals clip with diodes in the feedback of an amplifier, distortion pedals with diodes to ground. The Tube Screamer's 720 Hz is worked out from its clipping stage (1 / (2π × 4.7 kΩ × 47 nF) = 720 Hz): below it, the lows are turned up less on their way into the diodes. A guitar speaker's range comes from a speaker maker's data; the cone's ring in the upper mids differs from speaker to speaker. The THD of a note squared off completely is 48%, worked out from the square wave's harmonics; the THD of DawIt's own curves, the plucked note in the pictures (falling about 20 dB a second) and the power chord's difference tone (E2 at 82.4 Hz and B2 at 123.5 Hz leave 41.2 Hz, an octave below the E) are worked out or measured by DawIt.

**Pages read**

- <https://en.wikipedia.org/wiki/Distortion_(music)> (the Maestro FZ-1 Fuzz-Tone of 1962; the Tube Screamer, RAT and DS-1 named as classic pedals; guitarists turning small amps up in the 1950s)
- <https://en.wikipedia.org/wiki/Ibanez_Tube_Screamer> (the TS808 released in 1979; two diodes in the feedback of an op-amp; it boosts the mids and cuts the bass to keep the sound from getting muddy)
- <https://en.wikipedia.org/wiki/Fuzz_Face> (released by Arbiter in 1966; Jimi Hendrix its best-known user)
- <https://en.wikipedia.org/wiki/Big_Muff> (released in 1969; its tone control blends a low-pass and a high-pass filter, which dips the mids in the middle position)
- <https://en.wikipedia.org/wiki/Octavia_(effects_pedal)> (made by Roger Mayer in early 1967; Jimi Hendrix played it on the solo of 'Purple Haze'; it doubles the note an octave up)

**Searched for**

- MXR Distortion+ introduced year, ProCo RAT 1978, Ibanez Tube Screamer TS808 1979 (results: <https://en.wikipedia.org/wiki/Pro_Co_RAT>: a dozen or so made in 1977 and 1978, in the catalogue from 1979; no clear year for the TS808)
- MXR Distortion+ M104 history released 1973 or 1974 (results: <https://en.wikipedia.org/wiki/MXR_Distortion_%2B>: released in 1974 with the Phase 90's follow-ups)
- Ibanez Tube Screamer TS808 introduced year (results: <https://www.guitarworld.com/features/classic-gear-maxon-overdrive-and-ibanez-tube-screamer>, <https://www.chicagomusicexchange.com/blogs/news/a-brief-history-of-the-tubescreamer>: designed by Susumu Tamura and first made in 1979, sold by Maxon in Japan as the OD-808 and abroad by Ibanez as the TS808)
- Tube Screamer clipping stage high-pass 720 Hz, 4.7 kΩ and 0.047 µF (results: <https://cushychicken.github.io/ltspice-tube-screamer/>, <https://sandelinos.com/diy/tube-screamer/>: the clipping stage gives the full gain only above about 720 Hz, less and less below it)
- Buchla 259 complex waveform generator wavefolding (results: <https://www.buchlaarchives.com/blog/timbre-digital-nonlinear-waveshaping>, <https://dafx.de/paper-archive/details/zmNQ8s5sZVhM_ObMB_D8tg>: wavefolding began in Buchla's 259 oscillator of the 1970s and was taken up by Serge; no exact year found, so the guide says "the 1970s")
- electric guitar string decay rate in dB per second (no measured figure for electric guitars found, so the guide gives the rate of the note in its own picture)
- Celestion Vintage 30 frequency range (results: <https://celestion.com/productpdf.php?id=888>: a 12-inch guitar speaker rated from 70 Hz to 5 kHz, with its resonance at 75 Hz)

## Subtractive: "What is subtractive synthesis?"

Guide key: subtractive

The history (Moog, Switched-On Bach, the Minimoog, the Prophet-5, the Juno and the TB-303) comes from the pages below. A filter's slope is 6 dB per octave for each pole, so 2 poles give 12 dB and 4 poles 24 dB; the 24 dB in DawIt Subtractive is two 2-pole stages in a row, about 24.6 dB down an octave past the cutoff. The harmonics of the waves (the nth at 1/n for a saw and a square, 1/n² for a triangle: the 3rd at -9.5 dB in a square, -19 dB in a triangle), the beating of two detuned oscillators (A3 at 7 cents: 220 × (2^(7/1200) - 1) = 0.89 times a second) and the LFO's depths are worked out or are DawIt's own settings. Vibrato's 5-7 times a second agrees with the singing guide and its sources. The typical knob settings are common synth-programming practice; programmers differ widely.

**Pages read**

- <https://en.wikipedia.org/wiki/Subtractive_synthesis> (until digital synths came, subtractive synthesis was nearly the only electronic method; not the oldest: the additive Telharmonium came first)
- <https://en.wikipedia.org/wiki/Moog_synthesizer> (first shown in 1964, at the AES convention that October; Wendy Carlos's Switched-On Bach of 1968 brought it into the mainstream; a 24 dB low-pass filter)
- <https://en.wikipedia.org/wiki/Minimoog> (released in 1970; monophonic, a 4-pole, 24 dB low-pass)
- <https://en.wikipedia.org/wiki/Prophet-5> (shown in January 1978 and shipped that year; the first polyphonic synth with fully programmable memory, five voices)
- <https://en.wikipedia.org/wiki/Roland_Juno-60> (the Juno-6 and Juno-60 of 1982, six voices; pulse-width modulation; its chorus is a large part of its thick sound)
- <https://en.wikipedia.org/wiki/Roland_TB-303> (released in 1981; a 24 dB resonant low-pass; the squelch of acid house from Phuture's 'Acid Tracks', 1987)
- <https://en.wikipedia.org/wiki/Oberheim_SEM> (a two-pole multimode filter: low-pass, band-pass, high-pass and notch; 1974)

**Searched for**

- clarinet low register odd harmonics (results: <https://phys.unsw.edu.au/music/clarinet/E3.html>, <https://arxiv.org/pdf/physics/0008053>: in the clarinet's low register the odd harmonics are clearly the strongest; the even ones are there, but much weaker)

## Additive: "What is additive synthesis?"

Guide key: additive

The history (Fourier, Ohm and Helmholtz, the Telharmonium, the Hammond, the Synclavier, the Kawai K5, Schroeder's phases) comes from the pages below. The harmonic series is worked out: the nth harmonic of A3 is n × 220 Hz, its distance from a piano key is 1200 × log2 of the ratio (the 7th 31 cents flat, the 11th 49 cents flat, the 13th 41 cents sharp), and neighbouring harmonics are less than a semitone apart from the 17th up (18/17 is 99 cents, 17/16 is 105). So are the recipes of the waves (a saw's nth harmonic at 1/n, -6 dB for each doubling), the Gibbs overshoot of about 9% at a square's corners, the stiff-string formula and the swell, fade, brightness and stretch numbers of DawIt Additive. The ear is far less sensitive to how the harmonics line up than to how loud they are, but researchers have found exceptions. How much a Hammond drawbar step changes the level is usually given as 3 dB; measurements find the steps near the bottom larger.

**Pages read**

- <https://en.wikipedia.org/wiki/Telharmonium> (patented in 1897, US 580,035; played in public in New York in 1906; tonewheels, with extra ones for the harmonics, mixed by controls and stops)
- <https://en.wikipedia.org/wiki/Hammond_organ> (first made in 1935; the 8' drawbar is the note, 16' an octave below, 4', 2' and 1' one, two and three octaves up; registrations written as nine numbers)
- <https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/instruments/chapter_7_section_14.html> (Logic's tonewheel organ: the 5 1/3' drawbar is a fifth above the 8'; the 16' and 5 1/3' are the brown ones, the octaves white)
- <https://www.stefanv.com/electronics/hammond_drawbar_science.html> (each drawbar step about 3 dB, the common figure; a reader's measurements find larger steps near the bottom)
- <https://www.b3world.com/hammond-technical-information-02.html> (the vibrato scanner turns at 412 revolutions a minute, about 6.9 a second)
- <https://en.wikipedia.org/wiki/Tonewheel> (the Telharmonium first used tonewheels; a fundamental mixed with harmonics through switches and resistors)
- <https://en.wikipedia.org/wiki/Synclavier> (the Synclavier I of 1977-78 and II of 1980, built on additive synthesis with digital sine oscillators)
- <https://en.wikipedia.org/wiki/Ohm%27s_acoustic_law> (Ohm in 1843: the ear hears a sound as its pure tones; Helmholtz: the quality depends on the partials' strengths, not their phases; later studies disputed how far this holds)

**Searched for**

- clarinet low register odd harmonics (results: <https://phys.unsw.edu.au/music/clarinet/E3.html>, <https://arxiv.org/pdf/physics/0008053>: in the clarinet's low register the odd harmonics are clearly the strongest; the even ones are there, but much weaker)
- Thaddeus Cahill Telharmonium patent 1897 first built 1906 (results: <https://en.wikipedia.org/wiki/Telharmonium>, <https://en.wikipedia.org/wiki/Thaddeus_Cahill>: patented in 1897, US 580,035, first shown in public in 1906)
- Hammond drawbar 5 1/3' fifth above the fundamental, drawbar colours, dB per step (results: <https://en.wikipedia.org/wiki/Hammond_organ>, <https://www.stefanv.com/electronics/hammond_drawbar_science.html>: no single page lists every colour; the 3 dB a step is common wisdom)
- Hammond B-3 91 tonewheels, near-sine (results: <https://en.wikipedia.org/wiki/List_of_Hammond_organs>, <https://www.soundonsound.com/techniques/synthesizing-tonewheel-organs-part-1>: 91 tonewheels make tones on a B-3, each designed to be close to a sine)
- Hammond B-3 scanner vibrato rate (results: <https://www.synthtopia.com/content/2017/05/13/martinic-scanner-vibrato-recreates-electromechanical-organ-effects/>, <https://www.b3world.com/hammond-technical-information-02.html>: a fixed rate of about 6.9 Hz, from the 412 rpm motor)
- Kawai K5 additive synthesiser 1987, 126 harmonics (results: <https://www.muzines.co.uk/articles/kawai-k5/2552>, <https://soundprogramming.net/synthesizers/kawai/kawai-k5>: released in 1987 with up to 126 harmonics, 63 per source)
- Schroeder 1970, low-peak-factor signals (results: <https://ieeexplore.ieee.org/document/1054411>: IEEE Transactions on Information Theory, January 1970, phases that keep a signal's peaks low)
- Fourier 1822, Théorie analytique de la chaleur (results: <https://en.wikipedia.org/wiki/Fourier_series>: begun in 1807, published in 1822; any periodic signal as a sum of sines and cosines)
- Helmholtz 1863, On the Sensations of Tone (results: <https://cmtext.com/acoustics/chapter1_timbre.php>: published in German in 1863; timbre from the relative strengths of the partials)
- Risset trumpet tones, the high harmonics coming in later in the attack (results: <https://en.wikipedia.org/wiki/Jean-Claude_Risset>, <https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/4018121>: Risset's analysis found only the lower harmonics strong at first, the higher ones growing after; no time found, so the guide says "a moment later")
- church bell partials hum, prime, tierce, quint, nominal (results: <https://www.hibberts.co.uk/?p=665>, <https://en.wikipedia.org/wiki/Strike_tone>: the hum an octave below the prime, a minor third and a fifth above it, the nominal an octave up)
- piano inharmonicity, the stiff-string formula and the Railsback curve (results: <https://en.wikipedia.org/wiki/Piano_acoustics>: a stiff string's partials run sharp, f_n = n f0 √(1 + B n²); tuners stretch the octaves, the Railsback curve of 1938)
- the formants of "ah" (results: <https://ccrma.stanford.edu/~kglee/m220c/formant.html>: Peterson and Barney's averages, about 730, 1090 and 2440 Hz for men and 590, 1220 and 2810 Hz for women)

## FM: "What is FM synthesis?"

Guide key: fm

The history (Chowning's find at Stanford in 1967, his 1973 paper, the licence to Yamaha, the DX7 of 1983 and Yamaha's FM chips in sound cards and game consoles) and the classic sounds (Chowning's bell at 1 : 1.4 with an index of 10, his brass with the index following the loudness, the electric piano's tine at 14 times the note) come from the pages below. Chowning himself dates his first FM experiments to 1966-67. The sidebands are worked out from Bessel functions: at an index of 1 the first pair are each at 44% of the unmodulated carrier and the carrier at 77% (J1(1) = 0.44, J0(1) = 0.77); the carrier vanishes at an index of 2.405, the first zero of J0; Carson's rule gives 2 × (2 + 1) × 220 = 1,320 Hz for an index of 2 at 220 Hz; and the 1 : 1, 1 : 2 and 1 : 3 ratios' harmonics follow from the sum and difference frequencies, folding below 0 Hz. So are the numbers of DawIt FM's starting points (at 1 : 1 and an index of 2, the 5th harmonic is 27 dB under the 2nd). The typical settings are DawIt's own suggestions.

**Pages read**

- <https://en.wikipedia.org/wiki/Frequency_modulation_synthesis> (developed at Stanford from 1967; Chowning's paper in the Journal of the AES, 1973; Yamaha's version is phase modulation, with the same result; the OPL chips of the AdLib and Sound Blaster, the OPN2 of the Mega Drive)
- <https://en.wikipedia.org/wiki/John_Chowning> (FM found in 1967 by speeding up a vibrato until it made sidebands; licensed to Yamaha in 1974; the DX7 of 1983)
- <https://en.wikipedia.org/wiki/Yamaha_DX7> (released in 1983, over 200,000 sold, one of the best-selling synthesisers; six operators and 32 algorithms; its electric piano on many 1980s hits)
- <https://homepages.abdn.ac.uk/d.j.benson/pages/html/Chowning.html> (Chowning's own account: FM explored in 1966-67; Risset's trumpet analysis, the bandwidth growing with the loudness in the attack; brass from one envelope on both loudness and index; bells whose index falls with the loudness; the Yamaha licence around 1975)
- <https://patents.google.com/patent/US4018121A/en> (Chowning's patent: the bell at 200 and 280 Hz, 1 : 1.4, index up to 10, a 15-second exponential decay; the brass at 1 : 1 with the index following the loudness)
- <https://en.wikipedia.org/wiki/Jean-Claude_Risset> (Risset's analysis of trumpet tones at Bell Labs from 1964)

**Searched for**

- Yamaha DX100 DX21 TX81Z four operators, eight algorithms, feedback (results: <https://en.wikipedia.org/wiki/Yamaha_DX21>, <https://www.musicradar.com/how-to/blast-from-the-past-yamaha-tx81z>: four operators and eight algorithms)
- DX21 DX27 DX100 algorithms, the feedback loop on operator 4 (results: <https://auto.manualsonline.com/manuals/mfg/yamaha/dx27.html?p=29>, <https://willpirkle.com/synthlab/docs/html/synth_lab_d_m_d_x.html>: the DX27 manual names the feedback loop on operator 4; the DX synths are phase modulation synths, operator 4 the only one that modulates itself)
- Chowning 1973 paper, bell tones at 1 : 1.4, brass and Risset's trumpet (results: <https://homepages.abdn.ac.uk/d.j.benson/pages/html/Chowning.html>: the paper in the Journal of the AES in 1973, later reprinted in the Computer Music Journal)
- Chowning's discovery in 1967, vibrato sped up into sidebands (results: <https://www.soundonsound.com/techniques/introduction-frequency-modulation>, <https://yamahasynth.com/learn/synth%20-programming/fm101-discovering-digital-fm-john-chowning-remembers/>: two sine oscillators, one making the other's vibrato, sped up until the vibrato became a change of timbre)
- DX7 E.PIANO 1, an operator at a ratio of 14 for the tine (results: <https://www.muzines.co.uk/articles/yamaha-dx7/4733>: algorithm 5, a modulator at 14.00 for the high, bright presence)
- marimba bar tuning, the first overtone two octaves up (results: <https://ccrma.stanford.edu/CCRMA/Courses/150/percussion.html>, <https://hyperphysics.gsu.edu/hbase/Music/xylo.html>: a marimba's bars are carved so the first overtone is at 4 times the note, a xylophone's at 3 times)

## Drums: "What is drum synthesis?"

Guide key: drums

The history (the Sideman of 1959, the CR-78 of 1978, the TR-808 of 1980 and its poor sales, the LinnDrum of 1982, the TR-909 of 1983 with its recorded hi-hats and cymbals, the Syndrum and Simmons drums) and the 808's circuits (the six square waves of its cymbals and hi-hats, the cowbell's 540 and 800 Hz, the clap's repeated bursts and tail, one accent for the whole kit) come from the pages below. A real TR-808 kick bends only slightly, about a semitone; the big bends of DawIt's 808-style kits are a synthesis choice, as in many software kicks. Worked out: a drumhead's second mode at 1.59 times its first (the Bessel zeros 3.832 / 2.405), the cowbell's interval (1200 × log2 (800 / 540) = 680 cents, about 20 cents short of a fifth), -20 dB at a third of the time to -60 dB, the note frequencies of the low notes and the General MIDI drum notes. The typical tunings, decays and levels, the clap's band around 1 kHz and the recipes are common starting points and DawIt's own choices; producers differ.

**Pages read**

- <https://en.wikipedia.org/wiki/Roland_TR-808> (launched in 1980, analog synthesis rather than samples, a commercial failure with fewer than 12,000 sold; the CR-78 of 1978; the long, tuned kick used as a bass line)
- <https://en.wikipedia.org/wiki/Roland_TR-909> (released in 1983; samples for the hi-hats, crash and ride, the other sounds analog; taken up by Chicago house and Detroit techno; the same year as in *Styles*)
- <https://en.wikipedia.org/wiki/Drum_machine> (the Wurlitzer Side Man of 1959; the CR-78 of 1978; the Linn LM-1 of 1980, the first with samples, and the LinnDrum of 1982)
- <https://en.wikipedia.org/wiki/Vibrations_of_a_circular_membrane> (the modes of an ideal drumhead: the second at 3.832 / 2.405, about 1.59 times the first, none a whole-number multiple)

**Searched for**

- Wurlitzer Sideman 1959, how it made its sounds (results: <https://www.thehenryford.org/collections/explore/artifact/142279>, <https://www.musicradar.com/news/video-the-inside-of-this-1950s-drum-machine-is-a-sight-to-behold>: released in 1959 for organ players, patterns from a motor-driven disc of contacts; some call the Chamberlin Rhythmate earlier)
- TR-808 bass drum pitch glide (results: <https://arxiv.org/pdf/2502.08131>, <https://www.pressreader.com/australia/computer-music/20190220/282381220809560>: the 808 kick's pitch falls about a semitone before it settles; a bridged-T circuit that rings when triggered)
- TR-808 cymbal oscillators 205.3, 304.4, 369.6, 522.7, 540 and 800 Hz (results: <https://speech.di.uoa.gr/ICMC-SMC-2014/images/VOL_2/1453.pdf>, <https://docs.cherryaudio.com/misfit-audio/transistor-808-cymbal>: Werner, Abel and Smith's model of the 808 cymbal, six square-wave oscillators at these frequencies)
- TR-808 cowbell, two square waves at 540 and 800 Hz (results: <https://hackaday.com/2022/05/12/how-the-roland-808-cowbell-worked>, <https://www.soundonsound.com/techniques/synthesizing-cowbells-claves>: two square waves through a band-pass, with a fast decay)
- TR-808 handclap, noise, band-pass, repeated bursts and a tail (results: <https://www.muzines.co.uk/articles/the-synclap/6177>, <https://www.kvraudio.com/forum/viewtopic.php?p=4765735>: noise through a band-pass, an envelope fired several times in quick succession for many hands, and a longer decay for the room; no source gives the band's exact frequency)
- TR-808 accent (results: <https://www.polynominal.com/sample-packs/TIPTOP-bd808/Tiptop_Audio_BD808_user-manual.pdf>, <https://support.roland.com/hc/en-us/articles/201963539-TR-808-Technical-Specifications>: the original 808 has one accent knob for all its sounds, and an accent applies to every instrument on that step)
- Syndrum 1976 and Simmons SDS-V 1981 (results: <https://cherryaudio.com/news/the-syndrum-saga>, <https://en.wikipedia.org/wiki/Simmons_SDS-V>: the Syndrum from 1976, a sine whose pitch falls as it dies away; the Simmons SDS-V of 1981)

## Physical: "What is physical modelling?"

Guide key: physical

The history (Karplus and Strong's plucked string of 1983, Julius O. Smith's digital waveguides, Yamaha's VL1 of 1994, Korg's Prophecy of 1995 and Z1 of 1997, Logic's Sculpture of 2004, Pianoteq of 2006) and the physics of the bow (Helmholtz's sharp corner running round the string, seen through his vibration microscope and described in 1863; Schelleng's 1973 limits of bow pressure; McIntyre, Schumacher and Woodhouse's 1983 model) come from the pages below. Some sources date the VL1 to 1993, when it was announced. Where a piano's hammers hit comes from Harold Conklin: a little under 1/8 of the string in the bass, nearer the end in the treble; the 1/7 to 1/9 of older books, chosen to quieten the 7th and 9th harmonics, doesn't hold for modern pianos. Losses in real strings grow with the frequency; making them grow with its square, as DawIt Physical does, is a common modelling choice rather than a measured law. Vibrato widths differ a lot between players. Worked out: A3's period (1 / 220 = 4.55 ms, 218.2 samples at 48 kHz), the harmonics a playing position leaves out (the nth is missing where sin (n × π × position) is 0: at 1/5, the 5th, 10th and 15th), a closed tube's odd harmonics and its octave below an open one (a quarter wave instead of a half), a touch of T seconds hardly making partials above 1/T, and every starting point's numbers from its settings (the Init's 2 kHz partial rings about 0.53 s; the harp's 2nd harmonic starts at 31% and its 4th at 59%). The typical settings are DawIt's own suggestions.

**Pages read**

- <https://en.wikipedia.org/wiki/Physical_modelling_synthesis> (Karplus-Strong before digital waveguides; Smith and others; Yamaha and Stanford working together from 1989; the VL1 of 1994, the first waveguide synthesiser on sale)
- <https://en.wikipedia.org/wiki/Karplus%E2%80%93Strong_string_synthesis> (Computer Music Journal, 1983; a burst of white noise in a delay line, each sample averaged with the next; Smith showed it is a special case of a digital waveguide; Jaffe and Smith's extensions, 1983)
- <https://www.speech.kth.se/music/5_lectures/conklin/whereshould.html> (Harold Conklin, "Where should the hammer hit the string": many books say 1/7 to 1/9; in the bass a little under 1/8, in the treble 1/12 to 1/17; striking at 1/7 nearly removes the 7th partial, but he doesn't think any partial should be removed on purpose)
- <https://dsprelated.com/freebooks/pasp/Frequency_Dependent_Losses.html> (Julius O. Smith, Physical Audio Signal Processing: in nearly all natural waves the losses grow with the frequency; air drag and losses inside the string do too)

**Searched for**

- Yamaha VL1 1994, Korg Prophecy and Z1 (results: <https://en.wikipedia.org/wiki/Korg_Z1>, <https://musictech.com/features/opinion-analysis/vintage-rewind-korg-z1/>: the Prophecy of 1995, Korg's first MOSS synth; the Z1 of 1997, its polyphonic successor; the VL1 dated 1994, by some 1993)
- Logic Pro 7 and Sculpture, Pianoteq's first release (results: <https://www.apple.com/newsroom/2004/09/29Apple-Introduces-Logic-Pro-7-Logic-Express-7/>, <https://modartt.com/data/download/press/press_release_2006_08_26.pdf>: Sculpture came with Logic Pro 7 in 2004; Pianoteq 1.0 in August 2006)
- violin vibrato extent in cents (results: <https://physics.byu.edu/docs/publication/1499>, <https://journals.sagepub.com/doi/abs/10.2307/3345438>: Fletcher and Sanders, 1967: a typical extent of about 30 cents, from 10 to 60)
- piano hammer striking point 1/7 to 1/9 (results: <https://www.speech.kth.se/music/5_lectures/conklin/whereshould.html>, <https://hyperphysics.gsu.edu/hbase/Music/pianof.html>: the rule of thumb and its reason, the 7th harmonic; Conklin's measured positions differ)
- decay times of guitar and piano notes (results: <https://ccrma.stanford.edu/~jos/realsimple/piano/CommutedPiano/StringP.h>, <https://www.speech.kth.se/music/5_lectures/wogram/decay.html>: a piano model after Fletcher and Rossing rings A0 for tens of seconds, the top notes for a fraction of one; decay times differ widely between notes)
- Schelleng 1973 and McIntyre, Schumacher and Woodhouse 1983 (results: <https://www.repository.cam.ac.uk/handle/1810/263508>, <https://knutsacoustics.com/files/schelleng-in-retrospect.pdf>: Schelleng's minimum and maximum bow force, both rising nearer the bridge; the 1983 paper's model of the bowed string)
- Helmholtz's vibration microscope and the bowed string (results: <https://knutsacoustics.com/files/The-Helmholtz-motion.pdf>, <https://plus.maths.org/why-violin-so-hard-play-0>: a dot on the string watched through a microscope on a tuning fork; two straight parts meeting at a corner that runs round the string once a period; *On the Sensations of Tone*, 1863)
- string damping and frequency (results: <https://dsprelated.com/freebooks/pasp/Frequency_Dependent_Losses.html>, <https://core.ac.uk/download/77046161.pdf>: air viscosity dominates the losses at low frequencies, friction inside the string at high ones)
- clarinet register key, a twelfth, a closed tube's odd harmonics (results: <https://newt.phys.unsw.edu.au/jw/inharmonic-resonances.html>, <https://en.wikipedia.org/wiki/Overblowing>: an ideal cylinder closed at one end resonates at f = c / 4L, 3f, 5f...; the register key makes the clarinet jump a twelfth, where a flute jumps an octave; a real clarinet's higher resonances are not quite odd harmonics)
- clarinet low register odd harmonics (results: <https://phys.unsw.edu.au/music/clarinet/E3.html>, <https://arxiv.org/pdf/physics/0008053>: in the clarinet's low register the odd harmonics are clearly the strongest; the even ones are there, but much weaker)
- harpists' plucking point (results: <https://andrewhugill.com/OrchestraManual/harp_techniques.html>: près de la table, plucking near the soundboard, is a special effect; no source found for the usual point, so the guide no longer says where harpists pluck)

## Not for a guide (research for features, libraries and models)

These were looked up for features such as bounce and stems, the PDF export, aux sends and the AI and analysis models. They are listed so nothing is lost; they back no guide text.

- <https://arxiv.org/html/2511.03942>
- <https://bottosson.github.io/posts/oklab/>
- <https://code.claude.com/docs/en/skills>
- <https://code.claude.com/docs/en/sub-agents>
- <https://github.com/CPJKU/beat_this>
- <https://github.com/LAION-AI/CLAP>
- <https://developer.apple.com/library/archive/technotes/tn2157/_index.html> (AU preset files and folders, for user plugin presets)
- <https://github.com/steinbergmedia/bravura> (the Bravura music font, SIL OFL 1.1, the SMuFL reference font, for the score view)
- <https://github.com/bmcfee/crema>
- <https://github.com/ggml-org/whisper.cpp>
- <https://github.com/jayg996/BTC-ISMIR19>
- <https://github.com/marl/crepe>
- <https://github.com/mir-aidj/all-in-one>
- <https://github.com/unicode-org/cldr/blob/main/common/supplemental/supplementalData.xml>
- <https://help.apple.com/logicpro/mac/10.1/en.lproj/lgcp8aa3b784.html>
- <https://magenta.withgoogle.com/plugins>
- <https://ollama.com/library>
- <https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Preset+Locations.html> (VST3 preset folders, for user plugin presets)
- <https://opensource.adobe.com/dc-acrobat-sdk-docs/pdfstandards/PDF32000_2008.pdf>
- <https://raw.githubusercontent.com/unicode-org/cldr/main/common/supplemental/supplementalData.xml>
- <https://www.izotope.com/en/learn/stems-and-multitracks>
- <https://www.kaggle.com/models/google/yamnet>
- <https://www.sonarworks.com/blog/learn/how-to-prepare-your-session-for-a-mixer>
- <https://www.w3.org/TR/WCAG21/#dfn-contrast-ratio>
- <https://www.w3.org/TR/WCAG21/#non-text-contrast>
- <https://www.w3.org/TR/png/#9Filters>

**Searched for**

- LLM MIDI generation model open weights 2025 2026 symbolic music text-to-MIDI
- best small local LLM ollama 2026 structured JSON output reasoning 8B 14B qwen3 gemma
- ollama model music generation MIDI ABC notation GGUF ChatMusician NotaGen
- Ableton Live manual export "All Individual Tracks" return tracks master effects rendered
- Logic Pro user guide bounce in place regions tracks options include effect plug-ins volume pan
- preparing files for mixing engineer checklist tempo bpm sample rate bit depth all files start same point consolidate
- what are stems vs multitracks mixing delivery start at same point bar 1 export
- CLDR supplementalData paperSize type="US-Letter" territories BZ CA CL CO CR GT MX NI PA PH PR SV US VE
- PDF FlateDecode DecodeParms Predictor 15 PNG predictors Columns Colors ISO 32000 table 8
- WCAG 2.1 contrast ratio definition relative luminance 4.5:1 7:1
- countries that use US Letter paper size instead of A4 list
- <https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html>
- Logic Pro supports Audio Units plug-ins only not VST Apple support
- VST 3.8 SDK MIT license JUCE update VST3 SDK 3.8
- JUCE 8.0.11 VST3 SDK 3.8 MIT license (results: <https://www.kvraudio.com/news/juce-8-0-11-now-available-65579>, <https://www.soundonsound.com/news/steinberg-adopt-mit-license-vst3>; checked locally: `external/JUCE` is 8.0.9 with VST 3.7.12, whose `VST3_SDK/LICENSE.txt` is Steinberg's licence or GPLv3, and `AU/AudioUnitSDK/LICENSE.txt` is Apache 2.0)
