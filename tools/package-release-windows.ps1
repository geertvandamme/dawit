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

# DawIt doesn't ship Microsoft's Visual C++ runtime. The installer checks that it is on the PC, at least as new as
# the toolset that built DawIt, and otherwise offers Microsoft's own download (windows-installer.iss).
$runtime = @('msvcp140.dll', 'msvcp140_1.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio with the C++ tools was not found.' }
$toolset = Get-ChildItem (Join-Path $vs 'VC/Tools/MSVC') -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } | Sort-Object { [version] $_.Name } -Descending | Select-Object -First 1
if (-not $toolset) { throw 'The MSVC toolset was not found.' }
$needed = [version] $toolset.Name
Write-Host "The installer will ask for the Visual C++ runtime $($needed.Major).$($needed.Minor) or later."
# Every Visual C++ runtime DLL the binaries link, as the build record lists them, must be one the installer checks.
$record = 'build-info/build-info-windows-x64.json'
if (-not (Test-Path $record)) { throw "Build record missing: $record" }
$linked = (Get-Content $record -Raw | ConvertFrom-Json).binaries.needs |
    Where-Object { $_ -match '^(msvcp|vcruntime|concrt|vcomp|vccorlib)' } | ForEach-Object { $_.ToLowerInvariant() } | Sort-Object -Unique
$missing = $linked | Where-Object { $runtime -notcontains $_ }
if ($missing) { throw "Visual C++ runtime DLLs linked but not checked by the installer: $($missing -join ', ')" }
Copy-Item LICENSE "$stage/LICENSE.txt"
Copy-Item licenses "$stage/licenses" -Recurse
$compiler = Join-Path $env:RUNNER_TEMP 'inno-6.4.3/ISCC.exe'
if (-not (Test-Path $compiler)) { throw 'Pinned Inno Setup compiler is missing.' }
& $compiler "/DAppVersion=$Version" "/DSourceDir=$stage" "/DMinRuntimeMajor=$($needed.Major)" "/DMinRuntimeMinor=$($needed.Minor)" "/O$output" 'tools/windows-installer.iss'
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed: $LASTEXITCODE" }
if (-not (Test-Path "$output/DawIt-$Version-windows-x64-setup.exe")) { throw 'Installer was not created.' }
