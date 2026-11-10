#!/usr/bin/env python3
"""Writes build-info-<platform>.json for one release job: the runner image, compiler, CMake and SDK the package
was built with, the hash of each binary it ships, as built (before packaging signs it) and the libraries each binary needs from the system.

tools/make-dependency-manifest.py puts these records in the release's dependency manifest, so the toolchain is
kept with the release and not only in the CI logs. Run it after the build, from the checkout.

Usage: tools/record-build-info.py PLATFORM BUILD_DIR ONNX_WORK_DIR [--out DIR]
"""

import argparse
import hashlib
import json
import os
import pathlib
import platform
import re
import shutil
import subprocess
import sys


def run(*command):
    """The command's output, or None when it isn't there or fails."""
    try:
        return subprocess.run(command, capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def first_line(text):
    return text.splitlines()[0].strip() if text else None


def compiler(build):
    for path in sorted(build.glob('CMakeFiles/*/CMakeCXXCompiler.cmake')):
        text = path.read_text(encoding='utf-8', errors='replace')
        found = dict(re.findall(r'^set\((CMAKE_CXX_COMPILER_(?:ID|VERSION))\s+"([^"]*)"\)', text, re.M))
        if found:
            return f'{found.get("CMAKE_CXX_COMPILER_ID", "?")} {found.get("CMAKE_CXX_COMPILER_VERSION", "?")}'
    return None


def cache_value(cache, key):
    if not cache.is_file():
        return None
    match = re.search(rf'^{re.escape(key)}:[A-Z]+=(.*)$', cache.read_text(encoding='utf-8', errors='replace'), re.M)
    return match.group(1).strip() if match else None


def sha256(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as data:
        while block := data.read(1 << 20):
            digest.update(block)
    return digest.hexdigest()


def needs(path, kind):
    """The libraries a binary links, as the platform's own tool lists them."""
    if kind == 'macos':
        out = run('otool', '-L', str(path)) or ''
        return [line.split(' (')[0].strip() for line in out.splitlines()[1:] if line.strip()]
    if kind == 'linux':
        out = run('readelf', '-d', str(path)) or ''
        return re.findall(r'\(NEEDED\)\s+Shared library: \[([^\]]+)\]', out)
    dumpbin = shutil.which('dumpbin') or windows_dumpbin()
    out = run(dumpbin, '/nologo', '/dependents', str(path)) if dumpbin else ''
    return re.findall(r'^\s{4}(\S+\.dll)\s*$', out or '', re.M | re.I)


def windows_dumpbin():
    vswhere = pathlib.Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'),
                           'Microsoft Visual Studio/Installer/vswhere.exe')
    vs = run(str(vswhere), '-latest', '-products', '*', '-property', 'installationPath') if vswhere.is_file() else None
    if not vs:
        return None
    found = sorted(pathlib.Path(vs, 'VC/Tools/MSVC').glob('*/bin/Hostx64/x64/dumpbin.exe'))
    return str(found[-1]) if found else None


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('platform', choices=['macos-arm64', 'windows-x64', 'linux-x64'])
    parser.add_argument('build', help="DawIt's CMake build directory")
    parser.add_argument('onnx_work', help='the ONNX Runtime work directory (DAWIT_ONNX_WORK_DIR)')
    parser.add_argument('--out', default='dist', help='where to write the record (default dist/)')
    args = parser.parse_args()

    build = pathlib.Path(args.build)
    onnx_cache = pathlib.Path(args.onnx_work, 'build/Release/CMakeCache.txt')
    kind = args.platform.split('-')[0]
    artefacts = build / 'DawIt_artefacts/Release'
    binaries = {
        'macos': ['DawIt.app/Contents/MacOS/DawIt', 'DawIt.app/Contents/Frameworks/libonnxruntime.1.dylib'],
        'windows': ['DawIt.exe', 'onnxruntime.dll'],
        'linux': ['DawIt', 'libonnxruntime.so.1'],
    }[kind]

    info = {
        'platform': args.platform,
        'runner_image': ' '.join(filter(None, [os.environ.get('ImageOS'), os.environ.get('ImageVersion')])) or None,
        'host': f'{platform.system()} {platform.release()} {platform.machine()}',
        'cmake': first_line(run('cmake', '--version')),
        'compiler': compiler(build),
        'onnx_runtime_compiler': compiler(pathlib.Path(args.onnx_work, 'build/Release')),
        'onnx_runtime_generator': cache_value(onnx_cache, 'CMAKE_GENERATOR'),
        'python': platform.python_version(),
    }
    if kind == 'macos':
        info['xcode'] = ' '.join((run('xcodebuild', '-version') or '').split()) or None
        info['sdk'] = f'macOS {run("xcrun", "--show-sdk-version")}'
        info['deployment_target'] = cache_value(build / 'CMakeCache.txt', 'CMAKE_OSX_DEPLOYMENT_TARGET')
    elif kind == 'windows':
        info['visual_studio'] = cache_value(build / 'CMakeCache.txt', 'CMAKE_GENERATOR')
        project = build / 'DawIt.vcxproj'
        found = re.search(r'<WindowsTargetPlatformVersion>([^<]+)<', project.read_text(encoding='utf-8-sig')) \
            if project.is_file() else None
        info['sdk'] = f'Windows SDK {found.group(1)}' if found else None
    else:
        info['glibc'] = first_line(run('ldd', '--version'))
        info['distribution'] = next((line.split('=', 1)[1].strip('"') for line in
                                     pathlib.Path('/etc/os-release').read_text().splitlines()
                                     if line.startswith('PRETTY_NAME=')), None)
    info['binaries'] = [{'file': name, 'sha256_as_built': sha256(artefacts / name), 'needs': needs(artefacts / name, kind)}
                        for name in binaries]

    out = pathlib.Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    target = out / f'build-info-{args.platform}.json'
    target.write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(info, indent=2))


if __name__ == '__main__':
    sys.exit(main())
