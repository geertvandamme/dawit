param([Parameter(Mandatory = $true)][string] $Version)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Invalid version.' }
$source = Join-Path (Get-Location) 'build/DawIt_artefacts/Release'
$stage = Join-Path $env:RUNNER_TEMP 'dawit-release'
$output = Join-Path (Get-Location) 'dist'
if (-not (Test-Path "$source/DawIt.exe") -or -not (Test-Path "$source/onnxruntime.dll")) { throw 'App or ONNX DLL missing.' }
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage, $output | Out-Null
Copy-Item "$source/*" $stage -Recurse
Copy-Item LICENSE "$stage/LICENSE.txt"
Copy-Item licenses "$stage/licenses" -Recurse
$compiler = Join-Path $env:RUNNER_TEMP 'inno-6.4.3/ISCC.exe'
if (-not (Test-Path $compiler)) { throw 'Pinned Inno Setup compiler is missing.' }
& $compiler "/DAppVersion=$Version" "/DSourceDir=$stage" "/O$output" 'tools/windows-installer.iss'
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed: $LASTEXITCODE" }
if (-not (Test-Path "$output/DawIt-$Version-windows-x64-setup.exe")) { throw 'Installer was not created.' }
