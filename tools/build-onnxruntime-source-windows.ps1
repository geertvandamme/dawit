$ErrorActionPreference = 'Stop'
$revision = 'f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7'
# GitHub's tarball of that commit, as kept in the complete source archive (tools/make-complete-source.py).
$sourceArchiveSha256 = '1b4ec55fa26ee8fe622ea256894dfe9381bdfa99d41489994a1961720f556c48'
$mirror = if ($env:DAWIT_MIRROR_DIR) { $env:DAWIT_MIRROR_DIR } else { Join-Path $PSScriptRoot '../third_party/mirror' }
$sourceArchive = Join-Path $mirror "github.com/microsoft/onnxruntime/archive/$revision.tar.gz"
$work = $env:DAWIT_ONNX_WORK_DIR
if (-not $work) { throw 'Set DAWIT_ONNX_WORK_DIR to an empty build directory.' }
if (Test-Path $work) { throw "ONNX build directory already exists: $work" }
$source = Join-Path $work 'source'
$build = Join-Path $work 'build'
New-Item -ItemType Directory -Force $source | Out-Null
function Check-Exit($message) { if ($LASTEXITCODE -ne 0) { throw $message } }
if (Test-Path $sourceArchive) {
    # Building from the complete source archive, without a network: the pinned tarball, unpacked as is.
    if ((Get-FileHash $sourceArchive -Algorithm SHA256).Hash -ne $sourceArchiveSha256) { throw 'The ONNX source tarball does not match its pinned hash.' }
    tar -xzf $sourceArchive -C $source --strip-components=1
    Check-Exit 'Could not unpack the ONNX source tarball.'
    git init -q $source
    Check-Exit 'Could not initialise ONNX checkout.'
    git -C $source config core.autocrlf false
    Set-Content (Join-Path $source '.git/info/attributes') '* -text -filter'   # byte for byte: no line-ending or LFS filters
    git -C $source add -A
    Check-Exit 'Could not record the ONNX source.'
    git -C $source -c user.name=DawIt -c user.email=dawit@localhost commit -q -m "ONNX Runtime $revision from tarball $sourceArchiveSha256"
    Check-Exit 'Could not record the ONNX source.'
} else {
    git init -q $source
    Check-Exit 'Could not initialise ONNX checkout.'
    git -C $source config core.autocrlf false
    Check-Exit 'Could not configure line endings.'
    git -C $source remote add origin https://github.com/microsoft/onnxruntime.git
    Check-Exit 'Could not set ONNX remote.'
    git -C $source fetch --depth=1 origin $revision
    Check-Exit 'Could not fetch ONNX source.'
    $env:GIT_LFS_SKIP_SMUDGE = '1'
    git -C $source checkout --detach FETCH_HEAD
    Check-Exit 'Could not check out ONNX source.'
    if ((git -C $source rev-parse HEAD).Trim() -ne $revision) { throw 'ONNX revision mismatch.' }
}
git -C $source apply "$PSScriptRoot/patches/onnxruntime-1.30.0-unique-array.patch"
Check-Exit 'Could not apply ONNX patch.'
python -m pip install 'packaging==25.0'
Check-Exit 'Could not install Python build helper.'
Push-Location $source
try {
    python tools/ci_build/build.py --build_dir $build --config Release --build_shared_lib --skip_tests --skip_submodule_sync --no_telemetry --cmake_deps_mirror_dir $mirror --parallel 2 --cmake_generator 'Visual Studio 18 2026' --cmake_extra_defines onnxruntime_BUILD_UNIT_TESTS=OFF CMAKE_POLICY_VERSION_MINIMUM=3.5
    Check-Exit 'ONNX build failed.'
} finally { Pop-Location }
$release = Join-Path $build 'Release'
$cache = Get-Content "$release/CMakeCache.txt" -Raw
foreach ($entry in @('onnxruntime_USE_TELEMETRY:BOOL=OFF', 'onnxruntime_BUILD_UNIT_TESTS:BOOL=OFF', 'CMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026', 'CMAKE_GENERATOR_PLATFORM:INTERNAL=x64')) {
    if ($cache -notmatch "(?m)^$([regex]::Escape($entry))\r?$") { throw "Missing ONNX build setting: $entry" }
}
foreach ($name in @('onnxruntime.dll', 'onnxruntime.lib')) {
    $files = @(Get-ChildItem $release -Filter $name -File -Recurse | Where-Object { $_.FullName -notmatch '[\\/]_deps[\\/]|[\\/]CMakeFiles[\\/]' })
    if ($files.Count -ne 1) { throw "Expected one $name; found $($files.Count)." }
    $target = Join-Path $release $name
    if ($files[0].FullName -ne $target) { Copy-Item $files[0].FullName $target }
}
$deps = Join-Path $release '_deps'
$inputs = @(Get-ChildItem $deps -Recurse -File | Where-Object { $_.FullName -match '-populate-prefix[\\/]src[\\/][^\\/]+\.(zip|tar\.gz|tgz)$' } | ForEach-Object {
    [pscustomobject]@{ file = [IO.Path]::GetRelativePath($deps, $_.FullName); sha1 = (Get-FileHash $_.FullName -Algorithm SHA1).Hash; sha256 = (Get-FileHash $_.FullName -Algorithm SHA256).Hash }
})
if ($inputs.Count -eq 0 -and (Test-Path $sourceArchive)) {
    # FetchContent leaves no copy of a file read from the mirror: record the mirror's copies of ONNX's listed dependencies.
    $inputs = @(Get-Content "$source/cmake/deps.txt" | Where-Object { $_ -match '^[^#][^;]*;https://[^;]+;[0-9a-f]{40}$' } | ForEach-Object {
        $url = $_.Split(';')[1]
        Join-Path $mirror $url.Substring('https://'.Length)
    } | Where-Object { Test-Path $_ } | ForEach-Object {
        [pscustomobject]@{ file = $_; sha1 = (Get-FileHash $_ -Algorithm SHA1).Hash; sha256 = (Get-FileHash $_ -Algorithm SHA256).Hash }
    })
}
if ($inputs.Count -eq 0) { throw 'No ONNX dependency archives found.' }
[pscustomobject]@{ onnx_commit = $revision; patch_sha256 = (Get-FileHash "$PSScriptRoot/patches/onnxruntime-1.30.0-unique-array.patch" -Algorithm SHA256).Hash; runtime_sha256 = (Get-FileHash "$release/onnxruntime.dll" -Algorithm SHA256).Hash; archives = $inputs } | ConvertTo-Json -Depth 5 | Set-Content "$work/input-manifest.json"
