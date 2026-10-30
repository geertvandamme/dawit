# Sources of the theory guides

Where the facts in DawIt's theory guides come from, grouped by guide. **Pages read** were opened and read while planning or checking a guide. **Searched for** are searches whose results shaped a fact without a page of its own being opened. Facts that are derived (a calculation) are marked so in the plan's *Theory* section, in `plans/`.

Keep this file current: whenever a guide's text or numbers are added or changed, add the sources used under that guide (see `.claude/agents/dawPlanFeature.md` and `dawTheory.md`).

The first list was rebuilt from the agents' web lookups in the Claude Code chat history (22 to 28 October 2026) and the source tables in `plans/`. Guides built before that have no recorded sources yet: Rhythm, Chords, Chord shapes, Structure, Lyrics, Tune, Gain staging, EQ, Compressor, Delay, Saturation, Distortion, Subtractive, Additive, FM, Drums, Physical. Add theirs when those guides are next changed.


## Sound: "What is sound?" (envelope, mono check)

**Pages read**

- <https://en.wikipedia.org/wiki/Perceptual_attack_time>
- <https://knutsacoustics.com/files/Onset-transient-times-Rev.2.pdf>
- <https://lecdem.physics.umd.edu/h/h5/h5-22.html>
- <https://uol.de/f/6/dept/mediphysik/ag/musik-wahrnehmung/siedenburg2019transients.pdf>
- <https://www.soundonsound.com/sound-advice/10-essential-mix-checks>
- <https://www.uni-oldenburg.de/f/6/dept/mediphysik/ag/musik-wahrnehmung/siedenburg2019transients.pdf>

**Searched for**

- Saldanha Corso 1964 timbre cues identification musical instruments attack transients removed
- iPhone 7 first iPhone with stereo speakers
- mono compatibility mix why check smartphone single speaker Bluetooth speaker club mono sum engineers
- removing the attack of a piano or instrument note makes it hard to identify instrument timbre experiment Berger 1964 wind instruments
- rise time defined 10% to 90% of final value
- typical attack time milliseconds plucked guitar piano violin bowed flute onset rise time instruments table

## Recording: "How recording works"

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

**Pages read**

- <http://www.speech.cs.cmu.edu/cgi-bin/cmudict>
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
- passaggio notes by voice type tenor D4 G4 baritone B3 E4 soprano E4 F#5 Richard Miller primo secondo passaggio
- singing vibrato rate 5.5 to 7.5 Hz extent cents Sundberg Prame
- vocal warm-up duration minutes research singers lip trill semi-occluded vocal tract Titze benefits evidence
- voice type ranges soprano C4 C6 mezzo A3 A5 alto F3 F5 tenor C3 C5 baritone A2 A4 bass E2 E4

## Pitch correction: the effect and its guide

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

**Pages read**

- <https://en.wikipedia.org/wiki/Afrobeats>
- <https://en.wikipedia.org/wiki/Boom_bap>
- <https://en.wikipedia.org/wiki/Dembow_beat>
- <https://en.wikipedia.org/wiki/Drum_and_bass>
- <https://en.wikipedia.org/wiki/Four_on_the_floor_(music)>

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


## MIDI: "What is MIDI?" (the computer keyboard)

**Pages read**

- <https://www.ableton.com/en/manual/routing-and-i-o/>
- <https://www.ableton.com/en/manual/midi-and-key-remote-control/>
- <https://help.apple.com/logicpro/mac/10.4/en.lproj/lgcpb19cbd34.html>
- <https://en.wikipedia.org/wiki/Key_rollover>
- <https://www.deskthority.net/wiki/NKRO>

**Searched for**

- Logic Pro Musical Typing window keys octave Z X velocity C V (results: <https://dummies.com/article/how-to-record-midi-in-logic-pro-x-with-musical-typing-147775>)
- keyboard ghosting key rollover explained how many keys at once membrane keyboard matrix

## Drum triggers: "How drum triggers work" (Hits to MIDI)

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

**Searched for**

- notation rule 4/4 "middle of the bar" show beat 3 syncopation tie Gould Behind Bars (results: <https://musescore.org/en/node/351959>, Gould's *Behind Bars* p. 167 as quoted there)
- treble clef lines mnemonic "Every Good Boy Deserves Football" bass clef "Good Boys Deserve Fudge Always" "All Cows Eat Grass" (results: <https://music-theory-practice.com/reference/clef-mnemonics>, <https://www.dummies.com/article/academics-the-arts/music/instruments/piano/size-up-the-staff-how-to-read-treble-and-bass-clef-155363>)
- key signature written at the start of every staff line, time signature only at the beginning (results: <https://human.libretexts.org/Bookshelves/Music/Music_Theory/Understanding_Basic_Music_Theory_(Schmidt-Jones)/02%3A_Notation_-_Time/2.03%3A_Time_Signature>)
- whole rest hangs from the fourth line half rest sits on the middle line staff (results: <https://www.dummies.com/article/academics-the-arts/music/general-music/examining-rests-in-musical-notation-200164/>, <https://hellomusictheory.com/learn/rests/>)
- bass guitar notation sounds an octave lower than written bass clef transposing instrument (results: <https://www.dummies.com/article/academics-the-arts/music/instruments/bass-guitar/how-to-read-notation-to-play-the-bass-guitar-154804/>)
- bass clef lines mnemonic "Good Boys Deserve Fudge Always" (results: <https://music-theory-practice.com/reference/clef-mnemonics>, <https://barrypopik.com/blog/good_boys_do_fine_always>: "Good Boys Do Fine Always" is the more common form)
- order of sharps mnemonic "Father Charles Goes Down And Ends Battle" (results: <https://openbooks.macewan.ca/introtomusictheory/chapter/major-key-signatures/>, <https://music-theory-practice.com/reference/order-of-sharps-and-flats>)
- "Every Good Boy Deserves Football" treble clef (results: <https://en.wikipedia.org/wiki/Clef>, <https://teoria.com/en/tutorials/reading/12-notes.php>)

## Not for a guide (research for features, libraries and models)

These were looked up for features such as bounce and stems, the PDF export, aux sends and the AI and analysis models. They are listed so nothing is lost; they back no guide text.

- <https://arxiv.org/html/2511.03942>
- <https://bottosson.github.io/posts/oklab/>
- <https://code.claude.com/docs/en/skills>
- <https://code.claude.com/docs/en/sub-agents>
- <https://github.com/CPJKU/beat_this>
- <https://github.com/LAION-AI/CLAP>
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
