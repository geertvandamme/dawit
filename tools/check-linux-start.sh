#!/usr/bin/env bash
# Checks that the Linux archive starts on a fresh system with only the libraries the README lists.
# Run as root inside a clean container (ubuntu, debian or fedora), from a checkout, after the release archive is built:
#
#   tools/check-linux-start.sh DawIt-<version>-linux-x64.tar.gz "sudo apt install libcurl4 "
#
# The second argument is the start of the README's install line for that system. The script installs exactly that line,
# checks that every library DawIt and ONNX Runtime link, and every X11 library JUCE opens by name, is found, and only then
# adds what any desktop has anyway (an X server, here a virtual one, and a font) and starts DawIt with --screenshot.
set -euo pipefail
archive=${1:?Pass the Linux archive}
marker=${2:?Pass the start of the README install line}
work=$(mktemp -d)

line=$(grep -m1 -F -- "$marker" README.md | sed -e 's/^ *//' -e 's/^sudo //')
if [[ -z "$line" ]]; then echo "No README line starts with: $marker" >&2; exit 1; fi
echo "README: $line"

if command -v apt-get >/dev/null; then
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -q
    read -r -a packages <<< "${line#apt install }"
    apt-get install -y -q --no-install-recommends "${packages[@]}"
    desktop=(apt-get install -y -q --no-install-recommends xvfb xauth fonts-dejavu-core)
else
    read -r -a packages <<< "${line#dnf install }"
    dnf install -y -q --setopt=install_weak_deps=False tar gzip "${packages[@]}"
    desktop=(dnf install -y -q --setopt=install_weak_deps=False xorg-x11-server-Xvfb xorg-x11-xauth util-linux dejavu-sans-fonts)
fi

# DAWIT_UNPACKED: an archive unpacked outside the container, for runs under x86 emulation, where newer tar
# versions can fail with "Function not implemented".
if [[ -n "${DAWIT_UNPACKED:-}" ]]; then app=$DAWIT_UNPACKED/DawIt
else tar xzf "$archive" -C "$work"; app=$work/DawIt
fi

for binary in "$app/DawIt" "$app/libonnxruntime.so.1"; do
    missing=$(ldd "$binary" | grep 'not found' || true)
    if [[ -n "$missing" ]]; then echo "$binary needs libraries the README doesn't install:" >&2; echo "$missing" >&2; exit 1; fi
done

# JUCE opens these itself, so ldd can't see them (docs/release-bundle-checks.md).
for library in libX11.so.6 libXext.so.6 libXcursor.so.1 libXinerama.so.1 libXrandr.so.2 libXss.so.1; do
    if ! ldconfig -p | grep -qF "$library"; then echo "$library isn't installed by the README line" >&2; exit 1; fi
done
echo "Every library is found."

"${desktop[@]}"
shot=${DAWIT_START_SHOT:-$PWD/start.png}
timeout 300 xvfb-run -a -s '-screen 0 1600x1000x24' "$app/DawIt" --screenshot "$shot"
test -s "$shot"
echo "DawIt started and drew its window: $shot"
