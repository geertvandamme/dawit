#!/usr/bin/env python3
"""Verify the app and tests contain the library built for this release job."""
import hashlib
import os
import pathlib
import sys

work = pathlib.Path(sys.argv[1])
platform = sys.argv[2]
release = work / 'build' / 'Release'
build = pathlib.Path(os.environ.get('DAWIT_BUILD_DIR', 'build'))
if platform == 'macos-arm64':
    original = release / 'libonnxruntime.1.30.0.dylib'
    app = build / pathlib.Path('DawIt_artefacts/Release/DawIt.app/Contents/Frameworks/libonnxruntime.1.dylib')
    test = build / pathlib.Path('DawItTests_artefacts/Release/libonnxruntime.1.dylib')
elif platform == 'windows-x64':
    original = release / 'onnxruntime.dll'
    app = build / pathlib.Path('DawIt_artefacts/Release/onnxruntime.dll')
    test = build / pathlib.Path('DawItTests_artefacts/Release/onnxruntime.dll')
elif platform == 'linux-x64':
    original = release / 'libonnxruntime.so.1.30.0'
    app = build / pathlib.Path('DawIt_artefacts/Release/libonnxruntime.so.1')
    test = build / pathlib.Path('DawItTests_artefacts/Release/libonnxruntime.so.1')
else:
    raise SystemExit(f'Unsupported release platform: {platform}')

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

expected = digest(original)
for path in (app, test):
    if digest(path) != expected:
        raise SystemExit(f'Bundled runtime differs from source build: {path}')
print(f'App and tests use source-built ONNX Runtime: {expected}')
