# Third-party licence texts

The root [LICENSE](../LICENSE) contains DawIt's `AGPL-3.0-only` declaration, the component inventory and the complete unmodified AGPLv3 text. These files preserve upstream copyright, licence and notice texts separately; they do not relicense the components.

## Provenance

Captured on 29 October 2026. Local snapshots below were copied byte for byte from the dependency sources/packages used by the current build. For fetched URLs without an immutable revision, this directory retains the downloaded notice; verify it against the exact model provenance before bundling those weights.

| Notice file | Source |
|---|---|
| `JUCE-LICENSE.md` | `external/JUCE/LICENSE.md`, JUCE 8.0.9 |
| `VST3-SDK-LICENSE.txt` | JUCE 8.0.9 `modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt`; DawIt chooses GPLv3 |
| `GPL-3.0.txt` | [GNU's complete GPLv3 text](https://www.gnu.org/licenses/gpl-3.0.txt) |
| `AudioUnitSDK-LICENSE.txt` | JUCE 8.0.9 `modules/juce_audio_plugin_client/AU/AudioUnitSDK/LICENSE.txt` |
| `JUCE-FLAC-LICENSE.txt`, `JUCE-Ogg-Vorbis-LICENSE.txt`, `JUCE-libvorbis-COPYING.txt` | JUCE 8.0.9 `modules/juce_audio_formats/codecs/` |
| `JUCE-JPEG-README.txt`, `JUCE-libpng-LICENSE.txt` | JUCE 8.0.9 `modules/juce_graphics/image_formats/`; JPEG's full README preserves its licensing context |
| `JUCE-zlib-LICENSE.txt` | JUCE 8.0.9 `modules/juce_core/zip/zlib/LICENSE` |
| `JUCE-HarfBuzz-COPYING.txt`, `JUCE-SheenBidi-LICENSE.txt` | JUCE 8.0.9 `modules/juce_graphics/fonts/harfbuzz/COPYING` and `unicode/sheenbidi/LICENSE` |
| `demucs.cpp-LICENSE.txt` | `demucscpp-src/LICENSE`, revision `f1206e9adeea103aef4a636b9e62297cf1f8e34e` |
| `Eigen/COPYING*` | `eigen-src/COPYING*`, revision `dd8c71e62852b2fe429edb6682ac91fd1c578a26`; includes the upstream explanatory file and component/alternative terms |
| `ONNX-Runtime-LICENSE.txt`, `ONNX-Runtime-ThirdPartyNotices.txt` | `onnxruntime-osx-arm64-1.30.0.tgz`, matching the package hash in CMake |
| `Basic-Pitch-LICENSE.txt` | [Spotify Basic Pitch v0.4.0 LICENSE](https://raw.githubusercontent.com/spotify/basic-pitch/v0.4.0/LICENSE) |
| `whisper.cpp-ggml-LICENSE.txt` | `whispercpp-src/LICENSE`, v1.9.5 |
| `Demucs-weights-LICENSE.txt` | [Demucs upstream LICENSE](https://raw.githubusercontent.com/facebookresearch/demucs/main/LICENSE) |
| `Whisper-weights-LICENSE.txt` | [Whisper upstream LICENSE](https://raw.githubusercontent.com/openai/whisper/main/LICENSE) |
| `Bravura-LICENSE.txt` | Hash-checked licence downloaded by CMake from Bravura revision `37b194378b710cc40e406ab6c4b07608bb9548ae` |
| `CMUdict-LICENSE.txt` | [Dictionary LICENSE at the pinned revision](https://raw.githubusercontent.com/cmusphinx/cmudict/74790861f652b15e4ac49015a90074ad62a27690/LICENSE) |

The AGPL text in the root file was downloaded from [GNU](https://www.gnu.org/licenses/agpl-3.0.txt), unmodified. Inspect the inventory without reading the whole standard licence when maintaining component entries.

## Updating and distributing

Follow `CLAUDE.md` and `docs/architecture.md` (*Libraries and models*): refresh the root inventory, exact upstream notices, this provenance record, applicable README text and distributed copies together. Include build/packaging tools when their terms require notices or their code/output is distributed; do not add a merely proposed tool as an adopted dependency. Keep each upstream notice unmodified, and put project explanations in the inventory or this file.

These snapshots are a starting set for the current direct dependencies and identified embedded components. They are not a completed per-platform transitive audit. Compare ONNX notices across the Windows/Linux/other Mac packages, check enabled JUCE and whisper/ggml backends and any file-level notices, and retain the relevant source notices in the complete source archive. Source-availability duties remain separate from including licence texts.

The existing CMake build only copies Bravura's licence into the app. Copying the app licence and the applicable texts/index from this directory into binary distributions, collecting complete corresponding source and checking converted model provenance remain release tasks in [DISTRIBUTION_TODO.md](../DISTRIBUTION_TODO.md). No installer or release is made by adding these files.
