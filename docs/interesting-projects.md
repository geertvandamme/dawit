# Interesting projects and libraries

Projects and libraries worth a look for DawIt: for ideas, for learning how something is done, or as a possible dependency. A list to browse, not a plan. A project that becomes a dependency follows the rules in [architecture.md](architecture.md#libraries-and-models) (pinned with a hash, licence checked, named in the README and on *How it's made*).

Add an entry as: name and link, one sentence on what it does, what it could mean for DawIt, and the licence once checked. "Licence: not checked" means nobody has looked yet.

## Music AI from Google Magenta

Source: [magenta.withgoogle.com/plugins](https://magenta.withgoogle.com/plugins). The page lists these projects and links each one's code on GitHub; it doesn't state their licences.

- **[Magenta RealTime 2](https://magenta.withgoogle.com/plugins):** a real-time neural synthesis engine and open model for building and playing AI instruments on a laptop. *For DawIt:* a possible local, on-device sound source. Licence: not checked.
- **[The Infinite Crate](https://magenta.withgoogle.com/plugins):** a prototype DAW plugin that connects the Lyria RealTime API to music software. *For DawIt:* it goes through an online API, so it doesn't fit the rule that nothing leaves the user's machine. Interesting for how it fits into a DAW. Licence: not checked.
- **[DDSP-VST](https://magenta.withgoogle.com/plugins):** a plugin that morphs audio from one instrument to another in real time and can be played with MIDI. *For DawIt:* tone transfer, and a way to learn what *DDSP* (differentiable digital signal processing) does. Licence: not checked.
- **[Magenta Studio](https://magenta.withgoogle.com/plugins):** music plugins for Ableton Live, also available as standalone apps (generating and continuing melodies and drums, and humanising). *For DawIt:* compare with the generate features and *Humanize MIDI with AI*. Licence: not checked.
- **[NSynth Max for Live device](https://magenta.withgoogle.com/plugins):** explores NSynth sounds through a grid. *For DawIt:* an idea for a sound-morphing view. Licence: not checked.
- **[A.I. Jam (NIPS 2016 demo)](https://magenta.withgoogle.com/plugins):** an interactive jam session with an AI that runs in a browser. *For DawIt:* an old but clear example of a call-and-response player. Licence: not checked.

## Singing voice synthesis

Not LLMs, and not served by Ollama: models that turn a melody and lyrics into sung audio. Details below are from memory and not yet checked against the projects' pages.

- **[DiffSinger](https://github.com/openvpi/DiffSinger):** open-source singing voice synthesis; it takes lyrics (as phonemes) and a melody of timed notes and produces a sung vocal. ONNX exports exist. *For DawIt:* the best fit for a *Sing this* action on a MIDI melody and the lyrics track, through the ONNX Runtime already built in. Voicebanks have their own licences, to check before bundling or offering a download. Licence: not checked.
- **[OpenUtau](https://github.com/stakira/OpenUtau):** an open-source editor for singing voice synthesis that runs DiffSinger voicebanks locally. *For DawIt:* a working example of a note-and-lyrics editor driving a local voice, and of how voicebanks are shared. Licence: not checked.

### English voicebanks

DiffSinger voicebanks that sing English, from the pages of the [DiffSinger wiki](https://diffsinger.miraheze.org/wiki/Category:Voicebanks_supporting_English), [UTAU Repository](https://utaurepository.miraheze.org/wiki/Peiton) and [lottev.moe](https://lottev.moe/category/vocal-synth/diffsinger/), read in October 2026. Nothing was downloaded. Whether each is a standalone ONNX export for OpenUtau was not checked, but all are described as DiffSinger voicebanks for OpenUtau (OpenUtau needs the voicebank's own vocoder, usually included). The wiki is community-written, so check each maker's own terms before offering a download.

- **[Hoshino Hanami ~AI❤dol~](https://diffsinger.miraheze.org/wiki/Hanami_Hoshino)** (Lotte V): feminine, about F3 to A5, three vocal modes; English, Japanese, German, Korean, Spanish, Mandarin, French and Latin. Custom vocoder included. Download: MediaFire (v1.0 zip, [link](https://lottev.moe/hoshino-hanami-ai%e2%9d%a4dol-for-diffsinger-v1-0-is-out/)). Licence: the wiki says commercial use of the voicebank and character is allowed without permission, and the terms also bind derived works; the exact text is in the maker's [multi-licence repo](https://github.com/lottev1991/LotteV-Voicebank-Update-Repo) (LICENSE files not read). Redistribution and credit: not checked. The most open English voice found.
- **[Azure Cobalt Δ](https://diffsinger.miraheze.org/wiki/Azure_Cobalt)** (asteriski): feminine to androgynous, seven vocal modes; English, Japanese, Mandarin, Korean. Download: GitHub repo [agentasteriski/azurecobaltsynth](https://github.com/agentasteriski/azurecobaltsynth), but its Releases list was empty when read. Licence: no licence named; commercial use of the voicebank "varies by release" and of the character needs permission; no explicit sexual content. Vocoder: unconfirmed.
- **[Canary](https://diffsinger.miraheze.org/wiki/Canary)** (tigermeat, JAE VOCAL PROJECT; voice Mina Moonrise): male baritone, modes Arc, Spark, Voltage; English, Japanese, Mandarin, Spanish primary. Custom vocoder, duration and pitch models included. Download: tigermeat.xyz. Licence: commercial use needs permission; derivatives follow the same terms; full terms on the makers' sites (not read).
- **[Miyo](https://diffsinger.miraheze.org/wiki/Miyo)** (tigermeat, Shi Wei Migi, JAE VOCAL PROJECT): mezzo-soprano, nonbinary; English, Japanese, Mandarin. Custom vocoder included. Download: [utaufrance.com/voicebank/miyo](https://utaufrance.com/voicebank/miyo/) and tigermeat.xyz. Licence: the base voicebank is free; commercial use of the voicebank needs a purchased licence; credit required; derived voicebanks need permission. Full terms not read.
- **[TIGER](https://diffsinger.miraheze.org/wiki/TIGER)** (tigermeat): male, A2 to C#5, seven vocal modes; English, Japanese, Mandarin, Spanish, French and more. Custom vocoder included. Download: tigermeat.xyz. Licence: commercial use by purchased licence (Ko-fi); derivatives forbidden; terms of service on tigermeat.xyz (not read). The free download itself: unconfirmed.
- **[Laru Mine](https://diffsinger.miraheze.org/wiki/Laru_Mine)** (Minato): masculine, A2 to C4, six vocal modes; Japanese, French, English, Spanish (slight accent). Custom vocoder and duration model, no pitch model. Download: Bowlroll (needs a Bowlroll account; unconfirmed). Licence: commercial use needs permission; credit Minato; **no redistribution**; no religious or ideological use.
- **[Peiton DS](https://utaurepository.miraheze.org/wiki/Peiton)** (NebulaMeadow): tenor, androgynous; English (ARPAbet, "DIFFS EN" phonemizer in OpenUtau) and Japanese, seven vocal modes. Download: [nebulameadow.com/peitonds.html](https://nebulameadow.com/peitonds.html). Licence: the DiffSinger wiki says terms are "WIP"; UTAU Repository says commercial use needs a purchased licence, no use with AI voice changers or blockchain, full terms in a Google Doc (not read). Custom vocoder: per the DiffSinger wiki.

Other things read:

- The [Project AI❤dol Public English Dataset](https://github.com/lottev1991/Project-AIdol-Public-English-Dataset) is training data, not a voicebank: CC-BY-SA-4.0 per its page, commercial use allowed with credit to Lotte V, no public release of models with that voice without permission.
- The [nsf_hifigan vocoder](https://github.com/xunmengshe/OpenUtau/releases/download/0.0.0.0/nsf_hifigan.oudep) is what voicebanks without their own vocoder need (from the OpenUtau wiki); its licence: not checked.
- Opencpop, ROI, Ritsu Namine, Ruko Yokune and TORIKO are on the wiki but are not in its English category.
- Not found: any English voice under a plain open licence (CC0, CC-BY).
