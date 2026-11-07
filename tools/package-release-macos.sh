#!/usr/bin/env bash
set -euo pipefail

version=${1:?Pass the project version}
app=${DAWIT_BUILD_DIR:-build}/DawIt_artefacts/Release/DawIt.app
stage=$(mktemp -d)
output=${DAWIT_DIST_DIR:-dist}
trap 'rm -rf "$stage"' EXIT

test -d "$app"
test -f "$app/Contents/Frameworks/libonnxruntime.1.dylib"
mkdir -p "$output"
ditto "$app" "$stage/DawIt.app"
codesign --force --sign - "$stage/DawIt.app/Contents/Frameworks/libonnxruntime.1.dylib"
codesign --force --sign - "$stage/DawIt.app"
cp LICENSE "$stage/LICENSE.txt"
cp -R licenses "$stage/licenses"
ln -s /Applications "$stage/Applications"
codesign --verify --deep --strict --verbose=2 "$stage/DawIt.app"
hdiutil create -volname DawIt -srcfolder "$stage" -format UDZO "$output/DawIt-${version}-macos-arm64.dmg"
