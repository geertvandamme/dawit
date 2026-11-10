DawIt (*DAW In Theory*) is a free, open-source music sketchpad for learning while you make music. You can write and record parts, try out chords and sounds, and explore music theory through playable examples and guides that follow your song. Read more at [dawit.studio](https://dawit.studio).

**This is an early preview (version {{VERSION}}).** The packages are built and tested automatically on GitHub's build machines, but they aren't signed by Apple or Microsoft, so your system will warn you the first time you open DawIt. The steps below show how to open it anyway. Please [report problems](https://github.com/{{REPO}}/issues) with your DawIt version (shown in *About DawIt*), your system and what you did.

## Downloads

| System | Download | Needs |
|---|---|---|
| macOS on Apple Silicon (M1 or later) | `DawIt-{{VERSION}}-macos-arm64.dmg` | macOS 14 Sonoma or later |
| Windows, 64-bit (x64) | `DawIt-{{VERSION}}-windows-x64-setup.exe` | Windows 10 or 11 on an x64 PC (not Arm) and the Microsoft Visual C++ runtime (see below) |
| Linux, 64-bit (x64) | `DawIt-{{VERSION}}-linux-x64.tar.gz` | Ubuntu 22.04 or later, or another distribution with glibc 2.35 or later, and the libraries below |

There is no build for Intel Macs or for Windows and Linux on Arm yet.

## Installing and first open

### macOS

1. Open the `.dmg` and drag **DawIt** to **Applications**.
2. Open DawIt from Applications. DawIt isn't notarised by Apple, so macOS says it can't check it for malicious software.
   - **macOS 15 or later:** click **Done**, open **System Settings > Privacy & Security**, scroll down to the message about DawIt, click **Open Anyway** and confirm.
   - **macOS 14:** Control-click DawIt in Applications, choose **Open**, then **Open** again.
3. macOS asks for permission the first time DawIt uses the microphone, for example when you record.

### Windows

1. Run `DawIt-{{VERSION}}-windows-x64-setup.exe`. The installer isn't signed, so Windows SmartScreen may say "Windows protected your PC": click **More info**, then **Run anyway**.
2. DawIt installs for your user account only, in `%LOCALAPPDATA%\Programs\DawIt`, without asking for administrator rights. It adds DawIt to the Start menu, and to the desktop if you tick the box.
3. If Windows says `MSVCP140.dll` or `VCRUNTIME140.dll` is missing, install Microsoft's [Visual C++ Redistributable for x64](https://aka.ms/vs/17/release/vc_redist.x64.exe) and open DawIt again.
4. To remove DawIt, use **Settings > Apps > Installed apps**.

### Linux

1. Unpack the archive and start DawIt:

   ```bash
   tar xzf DawIt-{{VERSION}}-linux-x64.tar.gz
   ./DawIt/DawIt
   ```

2. DawIt needs a few common libraries. Most desktop systems have them already; if DawIt doesn't start, install them:

   - **Ubuntu 22.04, Debian 12:**
     ```bash
     sudo apt install libcurl4 libfontconfig1 libfreetype6 libasound2 libx11-6 libxext6 libxcursor1 libxinerama1 libxrandr2 libxss1
     ```
   - **Ubuntu 24.04 and later:**
     ```bash
     sudo apt install libcurl4t64 libfontconfig1 libfreetype6 libasound2t64 libx11-6 libxext6 libxcursor1 libxinerama1 libxrandr2 libxss1
     ```
   - **Fedora:**
     ```bash
     sudo dnf install libcurl fontconfig freetype alsa-lib libX11 libXext libXcursor libXinerama libXrandr libXScrnSaver
     ```

   DawIt plays and records through ALSA, and runs under X11 (or XWayland on a Wayland desktop).

## What's included, and what downloads later

- **In the app:** the editor, built-in instruments and effects, every theory guide, and audio to MIDI with the Basic Pitch model (230 KB). These work without a network.
- **Downloaded when you first use them, after DawIt asks:**
  - separating a recording into stems, and finding its vocals: the Demucs model, 84 MB;
  - finding English lyrics in a recording: Whisper small.en (488 MB) or medium.en (1.5 GB), whichever you choose;
  - sampled instruments: the MuseScore General SoundFont, 40 MB.

  Each download is checked against its pinned SHA-256 hash before DawIt uses it.
- **Set up separately:** AI suggestions for parts and lyrics need [Ollama](https://ollama.com) running on your own computer with a model you install. Your music isn't sent anywhere: analysis and AI run on your computer.

## Known issues

- **Unsigned packages:** the security warnings above appear on first open. Signing and notarisation are planned.
- **Lyrics from a recording on older Windows and Linux PCs:** finding lyrics needs a CPU with AVX2, which most PCs made since 2013 have. On an older CPU, DawIt closes when it starts finding lyrics. Everything else works.
- **Not yet tested on fresh installs:** the minimum system versions in the table come from how DawIt is built, and haven't been tried on clean macOS 14, Windows 10 or Ubuntu 22.04 systems. Upgrading from an earlier version hasn't been tested either; keep a copy of your songs.
- **The time signature** is one for the whole song, and a bar holds one chord.
- **Plugins are scanned inside DawIt:** a plugin that crashes during a scan closes DawIt once, and is skipped on the next scan.

## Source code, licences and checksums

DawIt is free software under the GNU Affero General Public License, version 3 only (`AGPL-3.0-only`), with no warranty. Its licence and every third-party notice are in each package and in DawIt under *About DawIt > Licences...*.

- **Complete source:** `{{SOURCE}}` holds the source of DawIt and of every library, model and font these packages are built from, and rebuilds without a network ([how](https://github.com/{{REPO}}/blob/{{TAG}}/docs/complete-source.md)). The tagged files alone: https://github.com/{{REPO}}/tree/{{TAG}}
- **Third-party components:** `{{DEPENDENCIES}}` lists every component, its version, licence and source, where it ends up on each platform, and how each package was built.
- **Checksums:** `SHA256SUMS.txt` holds the SHA-256 of each file here. Check a download with `shasum -a 256 -c SHA256SUMS.txt --ignore-missing` (macOS), `sha256sum -c SHA256SUMS.txt --ignore-missing` (Linux), or `Get-FileHash` in PowerShell (Windows).
