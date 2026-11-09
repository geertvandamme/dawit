# v0.1.2 source and notice audit

Checked 7 November 2026 against tag `v0.1.2`, commit
`9bbaf5b34093f16caa00de4134df07fe810941c3`, and GitHub Actions run
[42414524524](https://github.com/geertvandamme/dawit/actions/runs/42414524524).
This is an evidence record, not a legal opinion or a declaration that the draft
is ready to distribute. The working-tree notice additions described below are
not part of the tag or its already-built packages.

## Candidate packages

The downloaded DMG, Windows installer and Linux archive match the draft's
`SHA256SUMS.txt`. The Linux archive contains the root licence and all 28
notice files that were in the tag. Its ONNX library is byte-for-byte equal to
the library in the hash-checked Linux upstream package. The Mac DMG has the
root licence and all 28 tagged notice files next to the app, each byte-for-byte
equal to the tag; the app bundle itself does not
carry the notice directory when copied out of the DMG. On 8 November 2026,
the Windows installer was independently integrity-tested and extracted with
an experimental Inno Setup 6.4-capable extractor (innoextract pull request
214, commit `4bbe109f18419a3c68faeb013d7ae67330a4296c`). Its 28 tagged
notice files and root licence match the tag after normalising line endings;
its ONNX DLL is byte-for-byte identical to the hash-checked upstream Windows
package. This is an extraction check, not an independent installation test.
The ten notice additions below are absent from all three tagged candidates.

## Source inputs

The tag contains DawIt's source, licence inventory, tests, CMake files,
workflow and packaging scripts. It does not contain the downloaded
dependencies. The release configuration specifies:

The accompanying [direct-input manifest](release-v0.1.2-inputs.json) records
the pinned URLs and hashes. It is not a complete transitive dependency list.

| Input | Version or revision | Evidence and remaining work |
|---|---|---|
| JUCE | 8.0.9; tag resolves to `f72bad64d29715216226685810c5196bd0d79d77` | Local fetched checkout matches that commit. The clean CI checkout uses the fallback fetch; its logs do not independently record the resolved JUCE SHA. Preserve the full source, including the bundled VST3 SDK. |
| demucs.cpp | `f1206e9adeea103aef4a636b9e62297cf1f8e34e` | Local downloaded source archive matches the SHA-256 recorded in the tagged CMake file. |
| Eigen | `dd8c71e62852b2fe429edb6682ac91fd1c578a26` | Local source archive matches the recorded SHA-256; builds define `EIGEN_MPL2_ONLY`. Preserve applicable MPL source and notices. |
| whisper.cpp and ggml | v1.9.5, tag currently resolves to commit `d1be6fde11ac6e0407606b4e42fe72d34add8037` | Local source archive matches the recorded SHA-256. Its ggml code includes YaRN attribution; see below. |
| ONNX Runtime | 1.30.0, commit `f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7` | All three upstream binary packages match the tagged SHA-256 values and report this commit in `GIT_COMMIT_ID`. The runtime's source tree contains submodules and a separate dependency manifest. Determine the applicable sources, patches and build configuration for each upstream binary; source completeness is not yet verified. |
| Basic Pitch | v0.4.0 | The local `nmp.onnx` matches the tagged SHA-256. Preserve the model, its source/provenance and upstream licence and NOTICE. |
| CMU dictionary | `74790861f652b15e4ac49015a90074ad62a27690` | The local dictionary matches the tagged SHA-256; retain its full notice. |
| Bravura | `37b194378b710cc40e406ab6c4b07608bb9548ae` | The local font matches the tagged SHA-256; retain the OFL notice and relevant source material. |

The hash checks above establish that the local downloads match the values in
the release configuration. They do not independently prove the CI runner's
actual bytes except where the configuration checks the hash or the package
itself was compared. The build currently downloads some inputs unconditionally:
placing copies in a source archive alone will not make an offline rebuild use
them. Document or implement a source-archive build and test it in a fresh
environment.

The three ONNX Runtime packages' licence and third-party notice texts match
after normalising Windows line endings. The local snapshots match the Mac and
Linux package bytes. ONNX Runtime's own MIT licence does not itself require
distributing its source; assess the corresponding-source and notice obligations
of the combined DawIt distribution and of its applicable bundled components.
See the [ONNX source and dependency documentation](https://onnxruntime.ai/docs/build/dependencies.html).
At that ONNX commit the source tree records three submodules: ONNX at
`2bb50465112feca9003e1ed654d77f01ff1415ca`, libprotobuf-mutator at
`7a2ed51a6b682a83e345ff49fc4cfd7ca47550db`, and emsdk at
`c0bb220cb6e6f4e0fabb6f6db9efd53390ef5e56`. Their presence does not establish
that all three were used in these binaries. ONNX Runtime's pinned
[`cmake/deps.txt`](https://github.com/microsoft/onnxruntime/blob/f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7/cmake/deps.txt)
lists further possible downloads; it is not a per-binary bill of materials.
The ONNX source archive downloaded by commit on 8 November 2026 had SHA-256
`1b4ec55fa26ee8fe622ea256894dfe9381bdfa99d41489994a1961720f556c48`.
It contains the vcpkg manifests and ONNX Runtime licence and notice files,
but its three submodule paths are empty: the archive alone is not a complete
recursive source checkout. This download remains a temporary audit copy, not
a published or independently rebuilt source package.

The upstream release pipeline at this commit supplies more build clues, not
proof of the exact historical pipeline run. Its C API packaging templates use
vcpkg with Microsoft's internal asset cache; the registry configuration pins
`18a4723aeb7adbbae84bcff0edf510883800f32f` and declares overlay ports. The
Linux x64 template builds a shared library in Release with LTO. The Windows
x64 template uses Visual Studio 2022, RelWithDebInfo, LTO and a shared library.
The Mac ARM64 template enables CoreML and WebGPU and sets a macOS 14.0
deployment target. Preserve those templates, the pinned registry and overlay
ports, and resolve their selected dependencies and patches for each platform.
The exact upstream pipeline run and internal-cache contents remain unverified.

The pinned vcpkg configuration also makes clear that ONNX Runtime's Eigen is
not DawIt's directly fetched Eigen: the ONNX overlay selects Eigen commit
`1d8b82b0740839c0de7f1242a3585e3390ff5f33`. Other overlay inputs include
ONNX `v1.22.0` and protobuf `v33.6`, with patches in the ONNX Runtime tree;
the CPU packaging templates enable vcpkg. These are declared build inputs,
not proof that every listed port was compiled into every binary. Determine
the resolved triplets, features and actual components before calling a
per-platform source inventory complete.

The hash-checked Windows ONNX Runtime package also contains an
`onnxruntime.pdb` debug file. Its strings include build-object paths for
Abseil, ONNX, protobuf, RE2 and cpuinfo using the `x64-windows-static-md-rel`
triplet, and FlatBuffers symbols. The PDB's GUID and age match the DLL's RSDS
debug identifier; the DLL in the installer is byte-identical to the upstream
package. The PDB also records static-library paths under that vcpkg triplet,
including `onnx.lib`, `libprotobuf-lite.lib`, `re2.lib`, `cpuinfo.lib` and
multiple Abseil libraries. These records identify linked build inputs, but do
not prove their exact resolved revisions or constitute an exhaustive bill of
materials.
The PDB records source paths `abseil/src/20250814.0-...`,
`onnx/src/v1.22.0-...`, `protobuf/src/v33.6-...`, and a cpuinfo path ending
`58f6d8f512`, matching the ONNX vcpkg overlay's pinned cpuinfo commit
`66ee79c038d70dad9f08705b2c9b3e58f6d8f512`. The PDB's RE2 source path ends
`86bb745f55`, matching the pinned registry port's commit
`927f5d53caf8111721e734cf24724686bb745f55`. This is different from the RE2
version in ONNX Runtime's non-vcpkg `cmake/deps.txt`; do not substitute that
file for the actual vcpkg source and patches.
The ONNX third-party notice text contains the licence texts checked against
ONNX v1.22.0, protobuf v33.6, Abseil 20250814.0, cpuinfo at
`66ee79c038d70dad9f08705b2c9b3e58f6d8f512`, and RE2 at
`927f5d53caf8111721e734cf24724686bb745f55`, after normalising whitespace;
RE2's source-file comment markers were also removed for the comparison. This
checks these five upstream licence texts, not all transitive notices or
platform-specific build options.

The upstream Mac ONNX package's dSYM UUID matches its dylib. Its debug paths
include ONNX v1.22.0, protobuf v33.6, Abseil 20250814.0, RE2, zlib v1.3.2,
SQLite and Microsoft `cpp-client-telemetry` 3.10.173.1. The ONNX third-party
notice file contains sections for zlib, SQLite and cpp-client-telemetry. Its
zlib copyright dates and its nlohmann/json notice predate the versions found
in the debug data; the working tree preserves their exact-version notices as
described below.
Both the Mac and Linux ONNX binaries contain POSIX telemetry symbols and the
`ORT_DISABLE_TELEMETRY` environment-variable name. At the identified ONNX
commit, the POSIX telemetry provider defaults to enabled and can initialise an
uploader and log process and device metadata when an ONNX environment is
created. In v0.1.2, DawIt's `onnx::getEnvironment()` does not explicitly opt out. The
upstream POSIX code suppresses this provider in CI, so passing CI is not evidence of
ordinary-user network behaviour. No transmission was observed during this
audit and the evidence does not show music being sent. Verify the actual
candidate's network and file behaviour and resolve the privacy implications
before release. Debug paths are evidence of build inputs, not an exhaustive
source or licence inventory.
Relevant upstream source at the identified commit:
[POSIX telemetry](https://github.com/microsoft/onnxruntime/blob/f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7/onnxruntime/core/platform/posix/telemetry.cc),
[environment checks](https://github.com/microsoft/onnxruntime/blob/f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7/onnxruntime/core/platform/telemetry_environment.h),
and [environment startup](https://github.com/microsoft/onnxruntime/blob/f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7/onnxruntime/core/session/environment.cc).

The POSIX source configures Microsoft's `mobile.events.data.microsoft.com`
OneCollector endpoint and includes a hashed persistent device identifier,
OS and hardware information in process events. Session events can include
model filename, graph and metadata, hashes, and execution-provider details;
the source obtains the model filename without its directory. The source also
describes a local telemetry database. Audio or music samples were not found
in the reviewed event fields. These are code-path observations, not a packet
capture or proof of what a particular user's machine transmitted.

In the v0.1.2 tag, Audio-to-MIDI and DiffSinger create ONNX sessions through DawIt's shared
environment. On POSIX, `ORT_DISABLE_TELEMETRY=1` must be present before ONNX
initialisation to prevent creating the uploader and persisted identifier.
Calling the ONNX disable-events API after creating the environment is not an
equivalent full suppression: upstream explicitly permits the process event to
fire with runtime API suppression. On Windows the ONNX source uses ETW and
does not consult this environment variable; that is a separate control path,
not evidence of the same network uploader.

After v0.1.2, DawIt sets the POSIX opt-out before creating the shared ONNX
environment. For Windows, the pinned 1.30.0 source also has an internal
`ORT_RUNNING_UNIT_TESTS` hard-suppression signal: setting it before the platform
environment is created prevents ETW provider registration, unlike the public
disable-events API. DawIt sets that signal on Windows and calls the API on all
platforms as well. The Windows signal is an internal upstream detail, so check
it again whenever ONNX Runtime changes. These changes are not in the v0.1.2
draft binaries. Tests check the signal and shared environment, but do not
observe network traffic or ETW; verify those on clean release candidates.

Inspection of the downloaded upstream binaries found only system library
dependencies in the Linux ELF's recorded `NEEDED` entries. The Windows DLL
imports the Visual C++ runtime, including `MSVCP140.dll`,
`VCRUNTIME140.dll` and `VCRUNTIME140_1.dll`; those runtime requirements need
clean-machine validation. The Mac ONNX library in the actual draft DMG declares
macOS 14.0 as its minimum, despite the tagged build's CMake deployment target of 11.0.
This does not establish a tested minimum for the complete app; do not claim
support for macOS versions earlier than 14 based on CMake's setting.
The actual Linux app executable records dependencies on ONNX Runtime, curl,
fontconfig, FreeType, ALSA and standard C/C++ runtime libraries; the latter
system libraries are not bundled in the Linux candidate. The extracted
Windows app executable also imports ONNX Runtime and Microsoft C/C++ runtime
DLLs. These static import-table checks do not rule out additional libraries
loaded dynamically at runtime.

Demucs and Whisper weights are downloaded on first use, rather than included in
the candidate installers. On 8 November 2026, the Hugging Face API at each
pinned revision reported the three expected filenames, sizes and LFS SHA-256
values exactly matching the tag; both repository cards report MIT. The model
files themselves were not downloaded or inspected. Those records do not by
themselves prove the conversions' provenance or that every applicable notice
is supplied, so those questions remain open.

The pinned Demucs model card identifies the original weights provider and lists
`955717e8-8726e21a.th` among its source weights. The tagged demucs.cpp
conversion script selects that filename for its default four-stem model, and
its README describes producing the same named ggml file. The pinned Whisper
model card identifies the weights as OpenAI models converted for whisper.cpp;
the v1.9.5 whisper.cpp model documentation describes its conversion script
and lists the same SHA-1 values as the card for `small.en` and `medium.en`.
These are provider and upstream claims, not independently reproduced
conversions or verified hashes of the original weights.

## Notices checked and gaps found

The repository's JUCE, VST3 SDK root, AudioUnitSDK, FLAC, Ogg/Vorbis, JPEG,
libpng, zlib, HarfBuzz, SheenBidi, demucs.cpp, Eigen and whisper.cpp licence
snapshots match the checked local dependency files byte for byte. Basic Pitch
and CMUdict licence snapshots match their pinned upstream files. The Inno Setup
licence matches its upstream content after normalising line endings and final
newline.

Ten upstream notice files were absent from the tagged notice set:

1. Spotify Basic Pitch v0.4.0 has a `NOTICE` file. The working tree now
   preserves it as `licenses/Basic-Pitch-NOTICE.txt`.
2. JUCE's VST3 hosting source includes files in the VST3 SDK `base` and
   `public.sdk` directories. Their identical BSD-style licence text is
   distinct from the SDK's root licence. The working tree preserves it as
   `licenses/VST3-SDK-base-public-LICENSE.txt`.
3. ggml's CPU rotary embedding code and Mac Metal shader attribute code to
   YaRN under MIT terms. The working tree preserves its upstream licence as
   `licenses/YaRN-LICENSE.txt`. The pinned YaRN licence is from the
   introduction of that upstream repository; the exact source revision used
   for the adaptation has not been established.
4. JUCE compiles HarfBuzz's `hb-ucd.cc` and `hb-ot-layout-base-table.hh`.
   Their headers carry copyright and permission notices absent from the
   aggregate HarfBuzz notice. Their verbatim headers are now preserved in
   `licenses/JUCE-HarfBuzz-hb-ucd-LICENSE.txt` and
   `licenses/JUCE-HarfBuzz-base-table-LICENSE.txt`. A conservative traversal
   of locally reachable HarfBuzz includes found 265 files and these two
   additional copyright holders; this is not a complete platform audit.
5. ggml's `ggml-cpu/vec.h` attributes ARM and x86 vector routines to Arm. A related
   upstream Arm optimized-routines source has the same exponential coefficients
   and advertises MIT OR Apache-2.0 WITH LLVM-exception, with a 2019–2024 Arm
   copyright. The upstream licence is preserved as
   `licenses/Arm-optimized-routines-LICENSE.txt` as a conservative addition.
   The precise revision and attribution obligations of the adapted code still
   need confirmation; no conclusion is drawn from the later related file alone.
6. JUCE's compiled HarfBuzz includes Unicode 16.0.0 character and emoji data.
   Its generated emoji table preserves a 2024 Unicode copyright and terms
   reference. The working tree now preserves that header and the Unicode
   License V3 from the Unicode tools repository's Unicode 16 data commit as
   `licenses/Unicode-16-emoji-data-NOTICE.txt` and
   `licenses/Unicode-3.0-LICENSE.txt`, respectively. The data header matches
   the upstream Unicode 16 emoji file. Confirm any other separately attributed
   data incorporated into the generated tables during the wider audit.
7. The ONNX Mac debug information identifies zlib v1.3.2 and nlohmann/json
   v3.12.0. Microsoft's aggregate notices carry older copyright dates for
   those components. The working tree preserves the exact upstream version
   notices as `licenses/ONNX-zlib-1.3.2-LICENSE.txt` and
   `licenses/ONNX-nlohmann-json-3.12.0-LICENSE.txt`; the original ONNX
   notice file is unchanged.

The release packaging scripts copy `licenses/`, so a future rebuild would
include these additions in the external notice set. The working-tree Mac
packaging script also copies the licence and notice directory inside the app
before signing. On 8 November 2026, a temporary DMG made with a local
pre-existing build contained all 38 current notice files and the root
licence inside the app; its ad-hoc signature passed
`codesign --verify --deep --strict`. That package was only a packaging
test, not a v0.1.2 release candidate or a clean-machine installation test.
None of these changes is in the existing v0.1.2 candidate assets.
The working-tree Windows release workflow also compares each installed notice
and the root licence against the checked-out files after installation. It has
not yet run on a Windows CI runner.

## Work still required

- Audit enabled transitive code and per-file licences for each platform,
  including the ONNX binaries, JUCE's platform backends, ggml and Windows
  installer output; check all required attributions and notices. Do not assume
  the additions above exhaust the audit.
- Independently install on Windows and verify that all required notices remain
  available after installation; extraction alone does not prove this.
- Complete the direct-input manifest with actual platform-specific transitive
  dependencies, modifications and the upstream binary build details that can
  be established. Preserve the source and instructions for each released
  binary; test that recipients can rebuild and install a modified app.
- Finish the dedicated complete-source archive required by the project release
  checklist, or explicitly revise that project policy after establishing an
  equivalent reliable source-distribution method. A dedicated archive is not
  a universal AGPL requirement.
- Make the source and licence materials easy to find beside the downloads;
  verify that all shipped notices remain available after installation.
- Verify model conversion provenance and downloaded-model notices, and perform
  the clean-machine and installation tests in `DISTRIBUTION_TODO.md`.
- Verify telemetry suppression on clean release candidates on each platform,
  including startup network activity and Windows ETW events. The existing
  v0.1.2 binaries do not include the subsequent suppression changes.
- Fix DawIt compiler warnings and review third-party warnings: the successful
  v0.1.2 Windows CI build logged warnings that need assessment under the
  current project rule.
