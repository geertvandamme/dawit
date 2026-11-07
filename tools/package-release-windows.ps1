param([Parameter(Mandatory = $true)][string] $Version)
$ErrorActionPreference = 'Stop'
$source = 'build/DawIt_artefacts/Release'
$stage = Join-Path $env:RUNNER_TEMP 'dawit-release'
if (-not (Test-Path "$source/DawIt.exe") -or -not (Test-Path "$source/onnxruntime.dll")) {
    throw 'The app or ONNX Runtime DLL is missing.'
}
New-Item -ItemType Directory -Force -Path $stage, 'dist' | Out-Null
Copy-Item "$source/*" $stage -Recurse
Copy-Item LICENSE "$stage/LICENSE.txt"
Copy-Item licenses "$stage/licenses" -Recurse
Compress-Archive -Path "$stage/*" -DestinationPath "dist/DawIt-$Version-windows-x64.zip"
