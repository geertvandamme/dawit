# v0.1.3 source, notices and ONNX telemetry verification

Checked 8 November 2026 against tag `v0.1.3`, commit
`d5ddd5a89a3458c3f4501bbb027be549bddf8b21`, and the successful
[release workflow](https://github.com/geertvandamme/dawit/actions/runs/42485176151).
The release is a draft. This record states what was checked and what remains
open; it does not establish that the distribution is ready for publication.

## Source and packages

- Downloaded all three draft packages and verified each against the draft's
  `SHA256SUMS.txt`. The checksums are supplied by the same release, so they
  check transfer integrity, not an independent signature.
- Downloaded GitHub's generated source archive for `v0.1.3`. Its 804 files
  match the tagged Git blobs, with no missing or extra files. The downloaded
  archive's SHA-256 was
  `620f9b2fdb942dc128710920a7ab0986b993d235db8c8412dd405b5c0317f1d6`.
  GitHub-generated archive bytes can change; the tag's file contents are the
  meaningful comparison. Release notes link to the tagged source tree.
- The generated archive is DawIt's tagged source, not the dedicated complete
  source archive required by the project checklist. It does not contain all
  fetched dependency sources. The direct dependency versions and hashes are
  unchanged from v0.1.2; the earlier
  [source audit](release-v0.1.2-source-audit.md) and
  [input manifest](release-v0.1.2-inputs.json) remain relevant. Complete
  platform-specific ONNX transitive sources, build provenance and a clean
  rebuild from complete source remain unverified.
- The extracted Windows ONNX DLL and Linux ONNX shared library are
  byte-identical to the previously hash-checked upstream 1.30.0 packages.
  The macOS packaged dylib and upstream dylib have the same Mach-O UUID;
  their bytes differ because the packaged dylib is signed. UUID matching is
  supporting evidence, not a byte-for-byte comparison. The app's ad-hoc
  signature passes `codesign --verify --deep --strict`.

## Notices

Independently compared the actual package contents with the tagged `LICENSE`
and `licenses/` directory. All 38 tagged notice files and the root licence
matched after normalising line endings in each of these locations:

- the macOS image's external notice folder and the app bundle's resources;
- the Linux archive's app directory; and
- the Windows installer, extracted with the experimental Inno Setup 6.4
  extractor identified in the v0.1.2 audit.

The Windows CI job also installed the package twice, compared installed
licence files and the root licence with the tagged checkout, and uninstalled
it successfully. This is a CI installation check, not an independent clean
Windows machine test. Having every currently listed notice does not establish
that all applicable transitive notices have been identified; the unresolved
items in the prior audit still apply.

## ONNX telemetry

The tagged DawIt code sets `ORT_DISABLE_TELEMETRY=1` on macOS and Linux
before creating its shared ONNX environment. On Windows it sets
`ORT_RUNNING_UNIT_TESTS=1` before environment creation; in the pinned upstream
1.30.0 Windows implementation this internal signal prevents ETW provider
registration. It also calls the public disable-events API. This internal
Windows signal must be rechecked whenever ONNX Runtime changes. The packaged
macOS and Linux executables contain the POSIX signal, and the Windows
executable contains the Windows signal. All three CI test jobs passed their
shared-environment and signal checks. CI alone does not prove normal-user
behaviour, as upstream ONNX also suppresses telemetry in CI on POSIX.

An additional isolated macOS test compiled the tagged environment code into
a small harness, linked it to the actual packaged ONNX dylib, and created a
session using the Basic Pitch model. The process started with an empty
environment apart from `PATH` and a temporary `HOME`: CI and ONNX suppression
variables were absent. Networking was blocked with a process sandbox. Session
creation succeeded, the POSIX opt-out was set, and no files were created in
the temporary home. This checks the packaged runtime with the tagged code,
not a GUI run of the packaged app; blocking networking does not show whether
network access was attempted. It is not a packet capture.

Real release-candidate network and filesystem observation on macOS and Linux,
and Windows ETW observation outside CI, remain necessary to independently
verify actual behaviour. No equivalent Linux or Windows runtime observation was
performed in this audit.

## Remaining release checks

- Complete and preserve the required source archive and per-platform
  transitive dependency and notice audit; test rebuilding and installing a
  modified app from that material.
- Check telemetry behaviour on clean candidates on all three platforms, and
  complete independent installation and runtime checks.
- Resolve or explicitly account for compiler warnings in the successful
  platform build logs; success alone does not meet the project's warning-free
  build criterion. Review signing and macOS notarisation before publication.
