#!/usr/bin/env bash
set -euo pipefail
version=${1:?Pass the project version}
source_dir=${DAWIT_BUILD_DIR:-build}/DawIt_artefacts/Release
output=${DAWIT_DIST_DIR:-dist}
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
test -f "$source_dir/DawIt"
test -f "$source_dir/libonnxruntime.so.1"
mkdir -p "$stage/DawIt" "$output"
cp -a "$source_dir/." "$stage/DawIt/"
cp LICENSE "$stage/DawIt/LICENSE.txt"
cp -R licenses "$stage/DawIt/licenses"

# The app icon for the application menu: hicolor icons, a desktop entry and a script that installs both for the
# user (a desktop entry needs the absolute path of DawIt, which only the user's chosen folder knows).
for size in 16 32 48 64 128 256 512; do
    mkdir -p "$stage/DawIt/icons/hicolor/${size}x${size}/apps"
    cp "resources/icons/app-icon-${size}.png" "$stage/DawIt/icons/hicolor/${size}x${size}/apps/dawit.png"
done
cat > "$stage/DawIt/dawit.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=DawIt
Comment=Explore music theory while you make music
Exec=DawIt
Icon=dawit
Terminal=false
Categories=AudioVideo;Audio;Midi;
DESKTOP
cat > "$stage/DawIt/install-desktop.sh" <<'INSTALL'
#!/usr/bin/env bash
# Adds DawIt to your application menu: run this in the folder where you unpacked DawIt.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
data=${XDG_DATA_HOME:-$HOME/.local/share}
mkdir -p "$data/applications"
cp -R "$here/icons" "$data/"
sed -e "s|^Exec=.*|Exec=\"$here/DawIt\"|" "$here/dawit.desktop" > "$data/applications/dawit.desktop"
command -v update-desktop-database >/dev/null && update-desktop-database "$data/applications" || true
command -v gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -q -t "$data/icons/hicolor" || true
echo "DawIt is in your application menu."
INSTALL
chmod +x "$stage/DawIt/install-desktop.sh"
tar -czf "$output/DawIt-${version}-linux-x64.tar.gz" -C "$stage" DawIt
