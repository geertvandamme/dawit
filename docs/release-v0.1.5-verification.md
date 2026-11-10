# v0.1.5 packages, source, notices and dependency manifest verification

Checked 10 November 2026 against tag `v0.1.5`, commit
`5bbfcb0e2013931e63b241852cd7cc16dd497071`, and the successful
[release workflow](https://github.com/geertvandamme/dawit/actions/runs/42783169716).
The release is a draft. This record says what was checked and what is still
open. It doesn't show that the release is ready to publish.

v0.1.5 is the first release with the four ONNX notices the
[v0.1.4 notice audit](release-v0.1.4-onnx-notice-audit.md) found missing, and
the first with a dependency manifest and build records.

## Packages

- Downloaded all six draft assets and checked each against the draft's
  `SHA256SUMS.txt`: all match, `DawIt-0.1.5-dependencies.json` included. The
  checksums come from the same release, so they check the transfer, not an
  independent signature.
- The macOS app's ad-hoc signature passes `codesign --verify --deep --strict`.
  The app isn't notarised.
- The release notes name the dependency manifest and link the complete source.

## Complete source archive

- `DawIt-0.1.5-complete-source.tar.gz` (419 MB, SHA-256
  `fee38ed0ec431a4a03e7eed15a3bd3e244115903ebfa82d584852a1436dfde6e`) holds
  DawIt at the tag and 29 upstream files.
- Running `tools/make-complete-source.py --commit v0.1.5` on a Mac gave a
  byte-identical archive to the one the Linux CI job attached.
- The packaged macOS app's source link is
  `https://github.com/geertvandamme/dawit/releases/download/v0.1.5/DawIt-0.1.5-complete-source.tar.gz`.
  It works only once the release is published: draft assets aren't public.

## Dependency manifest and build records

- `DawIt-0.1.5-dependencies.json` lists 30 components, 5 app downloads and a
  build record for each platform. Running `tools/make-dependency-manifest.py`
  on the downloaded source archive gives the same components, app downloads
  and system requirements.
- The Linux package's `DawIt` and `libonnxruntime.so.1` match the hashes their
  build record took as built, byte for byte. The macOS binaries are signed ad
  hoc while packaging, so their bytes differ from the record; the Windows
  binaries are inside the installer.

| Platform | Runner image | Compiler | SDK |
|---|---|---|---|
| macOS arm64 | macos15 20261104.0514.1 | AppleClang 17.0.0 | macOS 15.5 |
| Windows x64 | win25-vs2026 20261103.298.1 | MSVC 19.51.36260 | Windows SDK 10.0.26100.0 |
| Linux x64 | ubuntu22 20261101.347.1 | GCC 11.4.0 | glibc of Ubuntu 22.04 |

- **`DawIt.exe`'s DLLs**, from the Windows record: `onnxruntime.dll`, Windows
  system DLLs (among them Direct2D, Direct3D 11, DXGI, DirectComposition,
  WinINet and Winsock), and the Visual C++ runtime: `MSVCP140`,
  `VCRUNTIME140`, `VCRUNTIME140_1` and the `api-ms-win-crt-*` libraries of the
  Universal CRT. `onnxruntime.dll` also needs `MSVCP140_1`, `dbghelp`,
  `SETUPAPI` and DXGI.

## ONNX Runtime

The as-built hashes of the macOS (`7c041d84…`) and Linux (`cc9bab3c…`)
libraries are the same as v0.1.4's: both builds are reproducible from the same
inputs, so the [v0.1.4 notice audit](release-v0.1.4-onnx-notice-audit.md)
covers these exact libraries. The Windows DLL differs (`d76f369b…`), as MSVC
builds always do. Each job's `verify-release-onnx.py` step passed, and the
Windows job installed and removed the installer.

## Notices

All 46 files of the tagged `LICENSE` and `licenses/` are present and identical
(after normalising line endings) in:

- the Linux archive;
- the DMG, beside the app;
- the app bundle's `Contents/Resources`.

Among them are the four added since v0.1.4:
`ONNX-Protobuf-utf8_range-LICENSE.txt`, `ONNX-onnx-1.22.0-NOTICE.txt`,
`ONNX-nlohmann-json-3.11.3-LICENSE.txt` and `ONNX-WIL-ThirdPartyNotices.txt`.
The Windows installer wasn't extracted; the Windows CI job installs it and
compares the installed licence files with the tag, and it passed.

## Still open

- The Windows installer's contents, checked independently of CI.
- Lyrics from audio on x64 CPUs without AVX2
  ([package checks](release-bundle-checks.md#cpu-features)).
- Signing, notarisation, the Visual C++ runtime decision, final release notes
  and clean-machine tests, as listed in `DISTRIBUTION_TODO.md`.
