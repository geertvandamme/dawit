# Third-party licence texts

The root [LICENSE](../LICENSE) contains DawIt's `AGPL-3.0-only` declaration, the component inventory and the complete unmodified AGPLv3 text. These files preserve upstream copyright, licence and notice texts separately; they do not relicense the components.

## Provenance

Captured on 29 October 2026. Local snapshots below were copied byte for byte from the dependency sources/packages used by the current build. For fetched URLs without an immutable revision, this directory retains the downloaded notice; verify it against the exact model provenance before bundling those weights.

| Notice file | Source |
|---|---|
| `JUCE-LICENSE.md` | `external/JUCE/LICENSE.md`, JUCE 8.0.9 |
| `VST3-SDK-LICENSE.txt` | JUCE 8.0.9 `modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt`; DawIt chooses GPLv3 |
| `VST3-SDK-base-public-LICENSE.txt` | JUCE 8.0.9 `VST3_SDK/base/LICENSE.txt`; identical to `public.sdk/LICENSE.txt` and retained for their BSD-style terms |
| `GPL-3.0.txt` | [GNU's complete GPLv3 text](https://www.gnu.org/licenses/gpl-3.0.txt) |
| `AudioUnitSDK-LICENSE.txt` | JUCE 8.0.9 `modules/juce_audio_plugin_client/AU/AudioUnitSDK/LICENSE.txt` |
| `JUCE-FLAC-LICENSE.txt`, `JUCE-Ogg-Vorbis-LICENSE.txt`, `JUCE-libvorbis-COPYING.txt` | JUCE 8.0.9 `modules/juce_audio_formats/codecs/` |
| `JUCE-JPEG-README.txt`, `JUCE-libpng-LICENSE.txt` | JUCE 8.0.9 `modules/juce_graphics/image_formats/`; JPEG's full README preserves its licensing context |
| `JUCE-zlib-LICENSE.txt` | JUCE 8.0.9 `modules/juce_core/zip/zlib/LICENSE` |
| `JUCE-HarfBuzz-COPYING.txt`, `JUCE-SheenBidi-LICENSE.txt` | JUCE 8.0.9 `modules/juce_graphics/fonts/harfbuzz/COPYING` and `unicode/sheenbidi/LICENSE` |
| `JUCE-HarfBuzz-hb-ucd-LICENSE.txt`, `JUCE-HarfBuzz-base-table-LICENSE.txt` | Verbatim licence headers from JUCE 8.0.9 HarfBuzz's compiled `hb-ucd.cc` and `hb-ot-layout-base-table.hh`; their authors are absent from HarfBuzz's aggregate `COPYING` |
| `Unicode-3.0-LICENSE.txt`, `Unicode-16-emoji-data-NOTICE.txt` | [Unicode License V3 from the Unicode tools repository's Unicode 16 data commit](https://github.com/unicode-org/unicodetools/blob/8e5668798448fab87bab62416f2b18186c40dff5/LICENSE), and the verbatim emoji-data header embedded in JUCE 8.0.9 HarfBuzz's `hb-unicode-emoji-table.hh`. The UCD table also records Unicode 16.0.0. |
| `demucs.cpp-LICENSE.txt` | `demucscpp-src/LICENSE`, revision `f1206e9adeea103aef4a636b9e62297cf1f8e34e` |
| `Eigen/COPYING*` | `eigen-src/COPYING*`, revision `dd8c71e62852b2fe429edb6682ac91fd1c578a26`; includes the upstream explanatory file and component/alternative terms |
| `ONNX-Runtime-LICENSE.txt`, `ONNX-Runtime-ThirdPartyNotices.txt` | `onnxruntime-osx-arm64-1.30.0.tgz`, matching the package hash in CMake. Release jobs build ONNX Runtime from source; its platform-specific dependencies and notices still require audit before public distribution. |
| `ONNX-zlib-1.3.2-LICENSE.txt`, `ONNX-nlohmann-json-3.12.0-LICENSE.txt` | [zlib v1.3.2](https://github.com/madler/zlib/blob/v1.3.2/LICENSE) and [nlohmann/json v3.12.0](https://github.com/nlohmann/json/blob/v3.12.0/LICENSE.MIT). ONNX Runtime's Mac debug data identifies these versions; the upstream ONNX aggregate notice has older copyright dates. |
| `Basic-Pitch-LICENSE.txt` | [Spotify Basic Pitch v0.4.0 LICENSE](https://raw.githubusercontent.com/spotify/basic-pitch/v0.4.0/LICENSE) |
| `Basic-Pitch-NOTICE.txt` | [Spotify Basic Pitch v0.4.0 NOTICE](https://raw.githubusercontent.com/spotify/basic-pitch/v0.4.0/NOTICE); retained in full for the embedded model |
| `whisper.cpp-ggml-LICENSE.txt` | `whispercpp-src/LICENSE`, v1.9.5 |
| `YaRN-LICENSE.txt` | [YaRN LICENSE at its 2023 introduction](https://github.com/jquesnelle/yarn/blob/a18e54988639030e706f6ff9db224cf74e08a65a/LICENSE); ggml v1.9.5 attributes its rotary embedding code to YaRN |
| `Arm-optimized-routines-LICENSE.txt` | [Arm optimized routines LICENSE at `0a6ab6d1`](https://github.com/ARM-software/optimized-routines/blob/0a6ab6d1f600a2fba6509440f455300a606024e6/LICENSE). ggml attributes vector routines to Arm; this related upstream version offers MIT or Apache-2.0 WITH LLVM-exception. The exact adaptation revision remains to be established. |
| `TinySoundFont-LICENSE.txt` | `LICENSE` of [TinySoundFont at commit `853a0a17`](https://github.com/schellingb/TinySoundFont/blob/853a0a171759f1ddba0de1442133a75912bbeffa/LICENSE) (v0.9), byte for byte, with its SFZero attribution line; the source archive's SHA-256 is pinned in CMake. The header of `tsf.h` states 2017-2025 where this file says 2017-2023. |
| `stb_vorbis-LICENSE.txt` | The MIT alternative ("ALTERNATIVE A") from the end of `stb_vorbis.c` v1.22 in [nothings/stb at commit `1ee679ca`](https://github.com/nothings/stb/blob/1ee679ca2ef753a528db5ba6801e1067b40481b8/stb_vorbis.c), copied unchanged; the file also offers the Unlicense, which DawIt does not use. The source archive's SHA-256 is pinned in CMake. |
| `Demucs-weights-LICENSE.txt` | [Demucs upstream LICENSE](https://raw.githubusercontent.com/facebookresearch/demucs/main/LICENSE) |
| `Whisper-weights-LICENSE.txt` | [Whisper upstream LICENSE](https://raw.githubusercontent.com/openai/whisper/main/LICENSE) |
| `MuseScore-General-LICENSE.md` | `MuseScore_General_License.md` of MuseScore General 0.2.0 from [ftp.osuosl.org](https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General_License.md), byte for byte (SHA-256 `5ad8d737e13c7f01f5b9674872a82a92b4ba253603e8ed14b9db12293550b4b9`, 3,419 bytes). It holds the MIT terms and the copyright notices that must accompany derivative work. The SoundFont itself is downloaded by the user's app after asking and never shipped; DawIt saves this file beside it |
| `Bravura-LICENSE.txt` | Hash-checked licence downloaded by CMake from Bravura revision `37b194378b710cc40e406ab6c4b07608bb9548ae` |
| `CMUdict-LICENSE.txt` | [Dictionary LICENSE at the pinned revision](https://raw.githubusercontent.com/cmusphinx/cmudict/74790861f652b15e4ac49015a90074ad62a27690/LICENSE) |
| `Inno-Setup-LICENSE.txt` | [Inno Setup 6.4.3 licence](https://github.com/jrsoftware/issrc/blob/is-6_4_3/license.txt) |

The AGPL text in the root file was downloaded from [GNU](https://www.gnu.org/licenses/agpl-3.0.txt), unmodified. Inspect the inventory without reading the whole standard licence when maintaining component entries.

## Updating and distributing

Follow `CLAUDE.md` and `docs/architecture.md` (*Libraries and models*): refresh the root inventory, exact upstream notices, this provenance record, applicable README text and distributed copies together. Include build/packaging tools when their terms require notices or their code/output is distributed; do not add a merely proposed tool as an adopted dependency. Keep each upstream notice unmodified, and put project explanations in the inventory or this file.

These snapshots are a starting set for the current direct dependencies and identified embedded components. They are not a completed per-platform transitive audit. Compare ONNX notices across the Windows/Linux/other Mac packages, check enabled JUCE and whisper/ggml backends and any file-level notices, and retain the relevant source notices in the complete source archive. Source-availability duties remain separate from including licence texts.

The existing CMake build only copies Bravura's licence into the app. Draft packages include these licence snapshots, but checking their completeness, collecting corresponding source and checking converted model provenance remain release tasks in [DISTRIBUTION_TODO.md](../DISTRIBUTION_TODO.md).
