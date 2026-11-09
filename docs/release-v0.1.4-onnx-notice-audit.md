# v0.1.4 ONNX Runtime notice audit

Checked 9 November 2026 against the v0.1.4 draft release
([release workflow](https://github.com/geertvandamme/dawit/actions/runs/42677288854)).
The question: which third-party code is compiled into the ONNX Runtime library
each release package ships, and does `licenses/` carry the notice each piece
needs, at the version built? This covers ONNX Runtime only; JUCE's platform
code, ggml and the rest of the per-platform check are still open in
`DISTRIBUTION_TODO.md`.

## What was compared

- **The built libraries.** `libonnxruntime.1.dylib` from the macOS DMG and
  `libonnxruntime.so.1` from the Linux archive. Both keep their full symbol
  tables (71,620 and 48,396 entries), so `nm` shows which projects' code was
  linked in; the source paths that assertion messages keep (`_deps/*-src`)
  confirm Abseil, ONNX, Protobuf and RE2.
- **The Windows DLL wasn't inspected.** The installer is built with Inno Setup
  6.4.3, which innoextract 1.9 can't read, and the CI artifact is the same
  installer. Windows is judged by its build: the same source commit, the same
  settings and the same downloads as Linux, plus WIL.
- **The build evidence.** Each job's `input-manifest.json` (what it downloaded)
  and `CMakeCache.txt` (its settings): in all three, CoreML, XNNPACK, mimalloc,
  WebGPU, the extensions and telemetry are off, and cpuinfo is on. KleidiAI is
  on everywhere but only fetched and compiled for Arm, so only macOS has it.
- **The upstream texts.** Each project's licence and notice files, from the
  exact archives in `DawIt-0.1.4-complete-source.tar.gz`, compared with the
  shipped notices word for word (whitespace and `//` comment markers
  ignored).

## What is compiled in

Defined symbols per project (0: not present):

| Project and version | macOS arm64 | Linux x64 | Notice |
|---|---|---|---|
| Abseil 20250814.0 | 2,126 | 2,267 | In the aggregate notices |
| Protobuf 33.6 | 682 | 830 | In the aggregate notices |
| utf8_range (inside Protobuf 33.6) | 1 | 2 | **Was missing**: added |
| RE2 2024-07-02 | 623 | 765 | In the aggregate notices |
| flatbuffers 23.5.26 | 207 | 180 | In the aggregate notices |
| cpuinfo at `66ee79c0` | 34 | 58 | In the aggregate notices; its `deps/clog` isn't compiled in |
| ONNX 1.22.0 | 11,918 | 7,550 | Licence in the aggregate notices; **its NOTICE was missing**: added |
| Eigen at `1d8b82b0` | 475 | 185 | `licenses/Eigen`, identical COPYING files |
| nlohmann/json 3.11.3 | 382 | 301 | **Only 3.12.0's text was shipped**: 3.11.3's added |
| date 3.0.1 | 6 | 2 | In the aggregate notices |
| SafeInt 3.0.28 | 10 | 25 | In the aggregate notices |
| Microsoft GSL 4.2.1 | 4,811 | 2,390 | In the aggregate notices; its own third-party notice is for tests |
| Boost.Mp11 1.82.0 | 232 | in template names | Boost licence in the aggregate notices |
| KleidiAI 1.20.0 | 360 | 0 | In the aggregate notices; its BSD-3-Clause file is for tests |
| WIL 1.0.250325.1 | 0 | 0 | Windows only: **its third-party notices were missing**: added |
| zlib, curl, XNNPACK, pthreadpool, mimalloc | 0 | 0 | Not compiled in |

`licenses/ONNX-Runtime-ThirdPartyNotices.txt` is identical, apart from line
endings, to `ThirdPartyNotices.txt` at the built commit `f2c39fe2` on all three
platforms. Upstream licence texts found in it in full: Abseil, cpuinfo, date,
flatbuffers, GSL, the Apache-2.0 texts of nlohmann/json, KleidiAI and ONNX,
ONNX's `LICENSE`, Protobuf, RE2, SafeInt and WIL's `LICENSE`.

## Gaps found and fixed

1. **utf8_range.** Protobuf 33.6 bundles Google's UTF-8 checker
   (`third_party/utf8_range`, MIT, "Copyright (c) 2019 Yibo Cai" and
   "Copyright 2022 Google LLC"). It's compiled in on macOS and Linux and isn't
   named in ONNX Runtime's notices. Added as
   `licenses/ONNX-Protobuf-utf8_range-LICENSE.txt`.
2. **ONNX's NOTICE.** ONNX 1.22.0 is Apache-2.0 and has a NOTICE file; the
   licence asks for it to go with derived works. Added verbatim as
   `licenses/ONNX-onnx-1.22.0-NOTICE.txt`. It also credits nanobind, which is
   only in ONNX's Python wheels, not in ONNX Runtime.
3. **nlohmann/json's version.** The source builds use 3.11.3 ("Copyright (c)
   2013-2022 Niels Lohmann"); the aggregate notices say 2013-2019 and the shipped
   version-specific file was 3.12.0's (2013-2025), taken from Microsoft's prebuilt
   library. Added 3.11.3's `LICENSE.MIT` as
   `licenses/ONNX-nlohmann-json-3.11.3-LICENSE.txt`. Its single header also
   credits Bjoern Hoehrmann (UTF-8 decoding, 2008-2009) and Florian Loitsch
   (Grisu2 number printing, 2009) under the same MIT terms; both pieces are
   compiled in (`serializer::decode` on macOS, `dtoa_impl::grisu2` on both), so
   `LICENSE` names them.
4. **WIL's notices.** WIL's `ThirdPartyNotices.txt` credits libc++, from which
   its `wistd_*` headers derive, as well as Catch2 and Detours, which are for its
   tests. Added verbatim as `licenses/ONNX-WIL-ThirdPartyNotices.txt`; it applies
   to the Windows build only.

The zlib 1.3.2 and nlohmann/json 3.12.0 texts stay: they cover Microsoft's
prebuilt library, which local builds may still download. The source builds
contain no zlib.

## Still open

- Inspecting the Windows DLL itself (it needs an extractor for Inno Setup 6.4,
  or the DLL kept as a CI artifact).
- The rest of the per-platform notice check: JUCE's enabled platform code,
  ggml and the Windows installer output.
- ONNX Runtime's own source files may carry file-level notices beyond its
  `ThirdPartyNotices.txt`; this audit relies on that file for ONNX Runtime's
  own code.
