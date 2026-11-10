# Package contents and system libraries

Checked 10 November 2026 on the v0.1.4 draft packages
([release workflow](https://github.com/geertvandamme/dawit/actions/runs/42677288854)).
From the next release on, each release job records the same facts for its own
binaries (`tools/record-build-info.py`), and they are attached with the release
in `DawIt-<version>-dependencies.json`.

## macOS (DawIt-0.1.4-macos-arm64.dmg)

- **What's in it:** `DawIt.app`, an `Applications` link, `LICENSE.txt` and
  `licenses/`. The app holds two binaries, `Contents/MacOS/DawIt` and
  `Contents/Frameworks/libonnxruntime.1.dylib`, and in `Contents/Resources` the
  licence texts, Bravura's licence and JUCE's `RecentFilesMenuTemplate.nib`.
  No caches, settings, songs, test programs or debug files.
- **Architectures (`lipo -info`):** both binaries are arm64 only.
- **Linked libraries (`otool -L`):** DawIt links ONNX Runtime by
  `@rpath/libonnxruntime.1.dylib`, found through its one run path,
  `@executable_path/../Frameworks`, and otherwise only system frameworks and
  `/usr/lib` libraries. Metal and MetalKit are linked weakly. ONNX Runtime
  links Foundation, CoreFoundation, `libiconv`, `libc++` and `libSystem`.
- **Minimum macOS:** both binaries say 14.0 (`minos`), built with the 15.5 SDK;
  `LSMinimumSystemVersion` is 14.0. It isn't tested on 14 yet.
- **ggml's Metal code:** `GGML_METAL_EMBED_LIBRARY` is on, so the shader source
  is inside the DawIt binary (75 copies of `metal_stdlib` among its strings) and
  Metal compiles it on the user's Mac. No `default.metallib` or other resource
  file is needed.
- **Build-machine paths:** the binaries contain paths such as
  `/Users/runner/work/dawit/dawit/build/_deps/whispercpp-src/...` in assertion
  and log messages. They are text only; nothing is loaded from them.

## Linux (DawIt-0.1.4-linux-x64.tar.gz)

- **What's in it:** `DawIt/DawIt`, `DawIt/libonnxruntime.so.1`, `LICENSE.txt`,
  `Bravura-LICENSE.txt` and `licenses/`. Both binaries find each other through
  `RUNPATH $ORIGIN`.
- **Linked libraries (`NEEDED`):**

  | Library | Debian/Ubuntu package | Fedora package |
  |---|---|---|
  | `libcurl.so.4` | `libcurl4` (`libcurl4t64` on Ubuntu 24.04) | `libcurl` |
  | `libfontconfig.so.1` | `libfontconfig1` | `fontconfig` |
  | `libfreetype.so.6` | `libfreetype6` | `freetype` |
  | `libasound.so.2` | `libasound2` (`libasound2t64` on Ubuntu 24.04) | `alsa-lib` |
  | `libstdc++.so.6`, `libgcc_s.so.1` | `libstdc++6`, `libgcc-s1` | `libstdc++`, `libgcc` |
  | `libc.so.6`, `libm.so.6` | `libc6` | `glibc` |

- **Loaded when present:** JUCE opens the X11 libraries itself, by name:
  `libX11.so.6`, `libXext.so.6`, `libXcursor.so.1`, `libXinerama.so.1`,
  `libXrandr.so.2` and `libXss.so.1` (Debian/Ubuntu `libx11-6`, `libxext6`,
  `libxcursor1`, `libxinerama1`, `libxrandr2`, `libxss1`; Fedora `libX11`,
  `libXext`, `libXcursor`, `libXinerama`, `libXrandr`, `libXScrnSaver`). No JACK
  or OpenGL library is named in the binary, so neither is needed.
- **glibc baseline:** the newest symbol versions used are `GLIBC_2.35` and
  `GLIBCXX_3.4.30`: Ubuntu 22.04 or a distribution at least as new.

## Windows (DawIt-0.1.4-windows-x64-setup.exe)

The installer can't be unpacked here (Inno Setup 6.4.3 is too new for
innoextract 1.9). The release job installs it and checks the installed files.
From a rebuild of the same ONNX Runtime
([notice audit](release-v0.1.4-onnx-notice-audit.md#windows-x64)),
`onnxruntime.dll` needs only Windows system DLLs and the Visual C++ runtime:
`MSVCP140.dll`, `MSVCP140_1.dll`, `VCRUNTIME140.dll` and `VCRUNTIME140_1.dll`.
`DawIt.exe`'s list comes with the next release's build record.

## CPU features

- **x64 (Windows and Linux): ggml needs AVX2, FMA and F16C.** With
  `GGML_NATIVE` off, ggml 1.9.5 turns these on by default, and its CPU code is
  built with them (`/arch:AVX2` with MSVC). In the Linux binary every function
  using 256-bit registers belongs to ggml's CPU backend; the start-up
  constructors use none. So DawIt starts on any x64 CPU, but finding lyrics in
  a recording would stop the app with an illegal instruction on a CPU without
  AVX2, such as Intel processors before 2013 (Haswell) and some later Pentium
  and Celeron models.
- **ONNX Runtime** chooses its kernels when it runs, by the CPU's features.
- **Eigen and the rest of DawIt** are built for the plain x64 baseline.
- **macOS arm64:** ggml is built without extra `-mcpu` flags, for every Apple
  Silicon Mac.

## Still open

- Lyrics from audio on x64 CPUs without AVX2: check the CPU in DawIt before
  running ggml and say so, or build ggml for a lower baseline.
- `DawIt.exe`'s DLL list, from the next release's build record.
- Testing the minimum macOS, Windows and Linux versions on real systems.
