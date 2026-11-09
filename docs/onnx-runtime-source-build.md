# Experimental ONNX Runtime source build

DawIt's normal development builds use the hash-pinned Microsoft ONNX Runtime 1.30.0 packages. The draft release workflow builds the runtime from pinned source on all three release platforms. The experiments below build
replacements for macOS Apple Silicon, Linux x86-64 and Windows x64; they are not yet
wired into either build and are not a completed source or notice audit for
a public release.

## macOS experiment

`tools/build-onnxruntime-source-macos.sh` checks out ONNX Runtime commit
`f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7`, applies the checked patch and
builds the Release shared library with the CPU provider, a macOS 14.0
deployment target and ONNX Runtime's POSIX telemetry disabled. It requires
Apple Silicon macOS, the Apple command-line developer tools, Python 3, Git,
CMake and Ninja. The first run needs network access to fetch the source and
its pinned dependencies. Run from the project root:

```sh
tools/build-onnxruntime-source-macos.sh
```

The default working directory is `build/onnx-runtime-source-v1.30.0/`.
`DAWIT_ONNX_WORK_DIR` selects a different directory and `DAWIT_ONNX_JOBS`
sets parallel build jobs. The script refuses a checkout at the wrong commit
or containing unexpected changes. It reuses CMake's existing source and
build files when they remain valid. It also writes an `input-manifest.json`
listing downloaded archives, their upstream SHA-1 and calculated SHA-256
hashes, the runtime hash, key build options and toolchain versions. The
manifest covers downloaded archives; it is not an exhaustive inventory of
system libraries, build tools or required notices. In
particular, this build uses a downloaded, prebuilt `protoc` as a host tool.
The generated SHA-256 values record the observed files; they are not yet an
independently pinned lockfile. The script does not verify the contents of
already-extracted dependency directories on a cached rerun. Use a clean
working directory when establishing release provenance.

The [source patch](../tools/patches/onnxruntime-1.30.0-unique-array.patch)
replaces a C++20 allocation function unavailable in the tested Apple
command-line toolchain with `std::make_unique`. Both paths produce a zeroed
buffer because the following upstream `memset` is retained. Preserve the
patch with the matching source if distributing a binary built this way.

On 8 November 2026 the local build succeeded using Apple clang 17 and the
macOS 26.1 SDK. The resulting library reports the pinned upstream commit
and macOS 14.0 minimum. The clean build also emitted upstream compiler
warnings, and the link step warned about duplicate static libraries. These
need investigation under the project's warning-free build rule. The script
also completed a fresh first run, fetching and recording 13 pinned archives,
and was subsequently rerun with networking blocked using its existing
checkout and cached dependencies.

The tagged v0.1.3 DawIt engine test executable was built in a separate source
copy and run with the new library and networking blocked: all checks passed,
including real Basic Pitch inference. A separate synthetic-tone comparison
produced identical Basic Pitch output values to the upstream macOS runtime
for that input. These checks do not establish equivalence for every model or
verify DiffSinger with a real voice.

The local `build/onnx-runtime-source-v1.30.0/` directory also contains an
explicitly incomplete source-evidence archive, the downloaded inputs and
candidate notice texts. The archive is a review aid, not a release source
package; it remains in the ignored build directory and can be removed by a
clean. It has not been uploaded to the v0.1.3 draft.

## Linux experiment

`tools/build-onnxruntime-source-linux.sh` uses the same pinned ONNX commit
and compatibility patch as the macOS helper. It builds a Release shared
library for Linux x86-64 with the CPU provider and POSIX telemetry disabled.
It requires Git, Python 3, CMake, Ninja and a C++ toolchain. Its first source
and dependency downloads need network access. On a Linux x86-64 machine,
run from the project root:

```sh
tools/build-onnxruntime-source-linux.sh
```

The default working directory is `build/onnx-runtime-source-linux-x64-v1.30.0/`.
`DAWIT_ONNX_WORK_DIR` and `DAWIT_ONNX_JOBS` select another directory and job
count. Like the macOS helper, it checks the ONNX checkout and patch, reuses
CMake outputs, and records hashes of downloaded archives and the runtime in
`input-manifest.json`. Its SHA-256 values are observations, not yet an
independent lockfile, and it does not check already-extracted cached
dependencies. Use a clean directory to establish release provenance.

On 8 November 2026 the pinned source was built in an Ubuntu 22.04.5 x86-64
container under emulation on Apple Silicon, using GCC 11.4 and CMake 4.1.2.
The cached Linux helper then applied the same patch and rebuilt successfully
with networking blocked, recording 12 downloaded dependency archives. The
helper's clean first-run path has not yet been separately verified. The
upstream configuration requested KleidiAI but disabled it as unsupported on
this target; the manifest records the requested setting, not an effective
KleidiAI build. The build emitted upstream compiler warnings, including in
protobuf; review them under the project's warning-free build rule.
The resulting ELF identifies itself as x86-64 with the expected
`libonnxruntime.so.1` soname. Its recorded dynamic dependencies are the
standard C and C++ runtime libraries; this does not rule out libraries
loaded dynamically at runtime.

The tagged v0.1.3 DawIt engine tests passed against the source-built library
with container networking disabled, including Basic Pitch inference. A
synthetic-tone comparison produced identical Basic Pitch output values to
the existing upstream Linux runtime for that input. This is an emulated
container test, not a native clean-machine installation or a real DiffSinger
voice test.

## Windows experiment

On 8 November 2026, the isolated `codex/onnx-windows-validation` branch built
the same pinned ONNX Runtime source and compatibility patch on a GitHub
Actions Windows Server 2022 runner. The CPU shared library was built with
`--no_telemetry`. The tagged v0.1.3 DawIt engine tests then passed with that
DLL, including real Basic Pitch inference. The DLL's SHA-256 was
`a2a4b116b14afe471b841191a33439dbdedcdf15862a039957fd8ab8bbd22699`.
The run and its evidence are at
https://github.com/geertvandamme/dawit/actions/runs/42520813865.

The build produced upstream compiler warnings, including repeated Protobuf
integer conversion warnings; these need review under the warning-free build
rule. The test checks that DawIt's Windows telemetry suppression signal is
set, but does not capture ETW events or establish that no telemetry is
emitted. This was a CI test, not a clean-machine installer or real DiffSinger
voice test. The evidence is not a completed source and notice audit.

## Before public distribution

- Test the chosen configuration on native Linux hardware and validate Windows
  installation on a clean machine. In the pinned
  ONNX source, the Windows ETW provider has a separate control path; do not
  assume the POSIX `--no_telemetry` build option suppresses Windows ETW.
- Choose the intended execution providers and measure inference behaviour and
  performance against the existing release packages. Verify real DiffSinger
  models as well as Basic Pitch.
- Audit the actual linked components, licences, file-level notices and source
  for each platform. This configuration fetches inputs including KleidiAI,
  and some dependency versions differ from Microsoft's prebuilt packages.
  Update `LICENSE`, `licenses/`, shipped notices, source materials and build
  instructions before public distribution; the experiment's local notice candidates
  are not a completed audit.
- Verify the integrated release builds on each GitHub runner, plus recipients'
  ability to build and install modifications. Development builds still use the
  prebuilt packages; the release jobs currently build from scratch without a cache.
- A replacement runtime changes the binaries. Do not alter the existing
  v0.1.3 draft assets or move its tag; prepare a new candidate when authorised.

## Release integration

The draft workflow builds a fresh runtime before configuring DawIt and sets `DAWIT_ORT_SOURCE_BUILD` to its working directory. CMake then links and bundles that runtime and uses headers from the same checkout. The workflow retains build settings, source dependency hashes and upstream notices as run artifacts. Those artifacts are evidence for review, not a complete corresponding-source archive or an exhaustive notice audit. The existing v0.1.3 draft is unchanged. Windows ETW suppression and clean-machine testing still need verification.

## Integrated release validation, 9 November 2026

The isolated `codex/onnx-release-integration-validation` branch exercised the
release build, test and packaging steps without creating a tag or release.
The runs used the pinned ONNX source and compatibility patch and verified that
the app and tests contained the source-built runtime. The GitHub Actions run
artifacts include the candidate and ONNX input evidence; they are not a
complete-source archive or a completed licence audit.

- [Linux x64](https://github.com/geertvandamme/dawit/actions/runs/42526634593)
  at `319379436368b2c44b25f75d3d4417441e175b97`: source build, DawIt
  build, engine tests and tarball packaging passed. The build emitted upstream
  compiler warnings, including Protobuf memory-copy overread and overflow
  diagnostics; investigate rather than treating them as harmless.
- [macOS arm64](https://github.com/geertvandamme/dawit/actions/runs/42531059087)
  at `61680491964799fe25fb17cf3bee66a77961cdfd`: source build, DawIt build,
  engine tests and DMG packaging passed. The source build emitted numerous
  upstream warnings.
- [Windows x64](https://github.com/geertvandamme/dawit/actions/runs/42531668807)
  at `fd98a035fc6f4127b57fb86689772b66ae0fedaf`: source build, DawIt build,
  engine tests, installer packaging, two installations and removal passed.
  The first Windows run found that cancellation attempted to delete a partial
  download while its output stream was open; the corrected run closes it
  before removal and passes the cancellation test. The source build emitted
  numerous upstream warnings.

The jobs used separate validation commits; these runs do not verify one final
release commit. Compiler warnings, source and notices, actual telemetry,
clean-machine checks, and final-candidate verification remain open.
