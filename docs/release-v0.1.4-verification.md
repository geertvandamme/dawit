# v0.1.4 source, packages, notices and ONNX Runtime verification

Checked 9 November 2026 against tag `v0.1.4`, commit
`fd4292a1e6c6980ce09da01f8a5fc8844af5a9b9`, and the successful
[release workflow](https://github.com/geertvandamme/dawit/actions/runs/42677288854).
The release is a draft. This record says what was checked and what is still
open. It doesn't show that the release is ready to publish.

v0.1.4 is the first release whose ONNX Runtime is built from source, and the
first with the complete source archive.

## Packages

- Downloaded all five draft assets and checked each against the draft's
  `SHA256SUMS.txt`: all match. The checksums come from the same release, so
  they check the transfer, not an independent signature.
- The macOS app's ad-hoc signature passes `codesign --verify --deep --strict`.
  The app isn't notarised.

## Complete source archive

- `DawIt-0.1.4-complete-source.tar.gz` (419 MB, SHA-256
  `9c96a16acf626616e1fc6977789952ddd9ce41e4b1c61d4c762a700dc5ac93f8`) holds
  DawIt at the tag and 29 upstream files.
- Running `tools/make-complete-source.py --commit v0.1.4` on a Mac gave a
  byte-identical archive to the one the Linux CI job attached, so the archive
  can be reproduced from the tag and the pinned downloads alone.
- The packaged macOS app's source link (*Licences...*) is
  `https://github.com/geertvandamme/dawit/releases/download/v0.1.4/DawIt-0.1.4-complete-source.tar.gz`.
  It works only once the release is published: draft assets aren't public.
- The offline rebuild from such an archive was tested on macOS before the tag
  ([complete source](complete-source.md)); it wasn't repeated for this archive.

## ONNX Runtime

All three release jobs built ONNX Runtime from commit
`f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7` with
`tools/patches/onnxruntime-1.30.0-unique-array.patch`. Each job's
`verify-release-onnx.py` step checked that the app and the tests carry the
library it built.

| Platform | Built library (SHA-256, from the job's manifest) | In the package |
|---|---|---|
| Linux x64 | `cc9bab3c…a0b3` | Byte-identical |
| macOS arm64 | `7c041d84…f354` | Signed ad hoc, so the bytes differ; checked unsigned in CI |
| Windows x64 | `fdfe1804…1391` | Not extracted: no Inno Setup extractor on the checking Mac |

- The Windows manifest gives the patch a different hash
  (`56af36d9…f817`). That is the same patch with CRLF line endings, as the
  Windows checkout writes it; its LF bytes give the macOS and Linux hash.
- `onnxruntime_USE_TELEMETRY` is `OFF` in all three builds' `CMakeCache.txt`.
- The packaged macOS and Linux DawIt executables contain the
  `ORT_DISABLE_TELEMETRY` signal. The Windows tests, which check the Windows
  signal, passed in CI.
- **curl isn't in the source builds.** None of the three jobs fetched it, and
  the packaged macOS and Linux libraries link only system libraries (`otool -L`,
  `readelf -d`) and contain no curl code; the only "curl" strings are Unicode
  bracket names. The Windows DLL wasn't inspected, but its job fetched no curl
  either. The curl notice question came from Microsoft's prebuilt packages
  shipped in v0.1.3.

Dependencies each build fetched, from its input manifest:

- all three: Abseil, date, Eigen, flatbuffers, GSL, Boost.Mp11, nlohmann/json,
  ONNX, Protobuf, cpuinfo, RE2, SafeInt;
- macOS also KleidiAI and the prebuilt `protoc`; Windows also WIL.

`licenses/ONNX-Runtime-ThirdPartyNotices.txt` names each of these. The
[ONNX notice audit](release-v0.1.4-onnx-notice-audit.md) then checked which of
them are compiled into the macOS and Linux libraries and compared their
licence texts version by version. It found four notices missing from the
v0.1.4 packages (Protobuf's utf8_range, ONNX's NOTICE, nlohmann/json 3.11.3
and WIL's third-party notices); they are added for the next release.

## Notices

All 42 files of the tagged `LICENSE` and `licenses/` are present and identical
(after normalising line endings) in:

- the Linux archive;
- the DMG, beside the app;
- the app bundle's `Contents/Resources`.

The Windows installer wasn't extracted. The Windows CI job installs it and
compares the installed licence files with the tag; it passed.

## Still open

- The Windows installer's contents, checked independently of CI.
- The v0.1.4 packages lack four ONNX notices (see above), so this draft
  shouldn't be published as it is; a release built after the fix carries them.
- The Windows ONNX DLL itself, which the notice audit judged from its build.
- Telemetry and network behaviour observed on clean machines on all three
  platforms; this check read build settings and binaries only.
- Signing, notarisation, the third-party warning review and clean-machine
  tests, as listed in `DISTRIBUTION_TODO.md`.
