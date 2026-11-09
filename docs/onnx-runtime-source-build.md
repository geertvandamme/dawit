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
warnings, and the link step warned about duplicate static libraries. Review
them under the project's third-party warning rule. The script
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
count. It checks the ONNX checkout and patch, applies and checks the Linux
Protobuf bounds patch, reuses CMake outputs, and records hashes of downloaded
archives, both patches and the runtime in
`input-manifest.json`. Its SHA-256 values are observations, not yet an
independent lockfile. It checks the patched Protobuf file but does not check
every other already-extracted cached dependency. Use a clean directory to
establish release provenance.

On 8 November 2026 the pinned source was built in an Ubuntu 22.04.5 x86-64
container under emulation on Apple Silicon, using GCC 11.4 and CMake 4.1.2.
The cached Linux helper then applied the same patch and rebuilt successfully
with networking blocked, recording 12 downloaded dependency archives. The
helper's clean first-run path has not yet been separately verified. The
upstream configuration requested KleidiAI but disabled it as unsupported on
this target; the manifest records the requested setting, not an effective
KleidiAI build. The build emitted upstream compiler warnings, including in
protobuf; review them under the project's third-party warning rule.
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
integer conversion warnings; these need review under the third-party warning
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

## Protobuf GCC warning investigation

GCC 11.4 on Ubuntu 22.04 reproduces the Linux release's Protobuf memory-copy
warnings by compiling the pinned dependency's `upb/message/copy.c` in isolation
with `-O3 -D_FORTIFY_SOURCE=2 -DNDEBUG` and the Protobuf source directory on the
include path. It reports one possible 16-byte overread and four possible 16-byte
writes into eight-byte objects in that translation unit. Other translation units
also reported similar warnings in the release log.

The warning traces pass through `_upb_MiniTableField_DataCopy`, whose generic
switch includes a string-view copy. The map, array and tagged-message accessors
in those traces assert or assume that their fields use the native pointer-sized
representation. That explains why GCC may report a string-view branch it cannot
prove unreachable; it does not prove that all inputs satisfy those invariants or
that the warnings can safely be ignored. The investigation below led to a
Linux-only bounds patch; compiler warning settings remain enabled. Windows also
reports ONNX and Protobuf narrowing conversions; do not replace those with
casts without checking their bounds.

### Windows enum diagnostics

The Windows validation run logged 3,844 C5287 messages, predominantly from
Protobuf's generated descriptor headers. The run used MSVC 19.44.35229.0;
its build evidence identifies the same Protobuf 33.6 archive used on Linux
(SHA-1 `051fee744fcca04dab37eb009d49aa3e9546acda`). The reported source
already casts both enum operands to `int`.

A [small compiler comparison](https://github.com/geertvandamme/dawit/actions/runs/42606602661)
compiled an equivalent explicitly cast expression on two GitHub runners. The
Windows 2022 compiler still emitted C5287; the Windows 2025 Visual Studio 2026
compiler did not. This establishes a compiler-specific diagnostic for that
example, not that all Protobuf warnings are false positives. The complete
[Windows release validation](https://github.com/geertvandamme/dawit/actions/runs/42606754982)
using MSVC 19.51.36260.0 built ONNX and DawIt, passed the tests, packaged the
installer and passed installation and removal checks. It logged no C5287
warnings, compared with 3,844 in the earlier run. The release workflow now
uses the validated Visual Studio 2026 generator and Windows runner. The newer
build still logged other conversion and pointer warnings; those remain open.

The macOS validation log has 1,560 warning lines attributed to Whisper/ggml
sources during the DawIt build, versus 62 attributed to ONNX sources during
its source build. Earlier references to the numerous macOS warnings as ONNX
warnings were too broad. Whisper/ggml still needs its own warning review.

An isolated GCC-only experiment checked `__builtin_object_size` for both
arguments before the string-view copy and trapped when a known object was too
small. GCC 11 then compiled both `upb/message/copy.c` and
`upb/message/message.c` without the original overread/overflow diagnostics. A
small harness copied all four field representations and rejected a known
undersized destination. It changes failure behaviour for invalid inputs and
only detects bounds known to the compiler.

The full [isolated Linux validation](https://github.com/geertvandamme/dawit/actions/runs/42608205800)
passed the runtime build, DawIt tests and packaging; it logged none of the nine
previous `-Wstringop-overread` and `-Wstringop-overflow` warnings. The Linux
release helper now applies `tools/patches/protobuf-33.6-copy-bounds.patch`, checks
the complete modified file, tests valid and undersized copies and records the
patch and source hashes. Include this patch with the corresponding source. The
upstream Protobuf test suite has not been run; this is not a complete audit of
the dependency or its warnings.
Some ONNX unused-parameter warnings occur because the build defines
`__ONNX_NO_DOC_STRINGS`, leaving parameters such as `description` unused. This
needs an upstream-compatible fix or a deliberate build configuration decision;
it is independent of Protobuf's bounds and narrowing warnings.
For example, Protobuf 33.6 `src/google/protobuf/micro_string.h` stores
`LargeRep::size` as `uint32_t` while `SetExternalBuffer` accepts a
`string_view` with a `size_t` length. The representation documents a 2^32-byte
limit. Before changing that conversion, verify how oversized strings are
rejected by all callers and test the limit; a cast alone preserves possible
truncation and is not a fix.
