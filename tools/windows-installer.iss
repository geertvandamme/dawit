#ifndef AppVersion
  #error AppVersion must be supplied by the packaging script
#endif
#ifndef SourceDir
  #error SourceDir must be supplied by the packaging script
#endif
#ifndef MinRuntimeMajor
  #error MinRuntimeMajor must be supplied by the packaging script
#endif
#ifndef MinRuntimeMinor
  #error MinRuntimeMinor must be supplied by the packaging script
#endif
; Microsoft's own download of the latest Visual C++ v14 runtime for x64. DawIt doesn't redistribute the runtime.
#define RuntimeDownload "https://aka.ms/vc14/vc_redist.x64.exe"

[Setup]
AppId={{DB11331D-CEEA-4EEB-A8F5-CB120E6B3CB0}
AppName=DawIt
AppVersion={#AppVersion}
AppPublisher=DawIt contributors
AppPublisherURL=https://github.com/geertvandamme/dawit
AppSupportURL=https://github.com/geertvandamme/dawit/issues
DefaultDirName={localappdata}\Programs\DawIt
DefaultGroupName=DawIt
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible and not arm64
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=DawIt-{#AppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\DawIt.exe

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Icons]
Name: "{group}\DawIt"; Filename: "{app}\DawIt.exe"
Name: "{autodesktop}\DawIt"; Filename: "{app}\DawIt.exe"; Tasks: desktopicon

[Code]
const
  RuntimeKey = 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';

{ DawIt needs Microsoft's Visual C++ runtime at least as new as the toolset it was built with. }
function IsNewEnough(Major, Minor: Cardinal): Boolean;
begin
  Result := (Major > {#MinRuntimeMajor}) or ((Major = {#MinRuntimeMajor}) and (Minor >= {#MinRuntimeMinor}));
end;

{ Microsoft's redistributable records itself here; which registry view depends on the installer, so both are read. }
function RegistryHasRuntime(RootKey: Integer): Boolean;
var
  Installed, Major, Minor: Cardinal;
begin
  Result := RegQueryDWordValue(RootKey, RuntimeKey, 'Installed', Installed) and (Installed = 1)
    and RegQueryDWordValue(RootKey, RuntimeKey, 'Major', Major)
    and RegQueryDWordValue(RootKey, RuntimeKey, 'Minor', Minor)
    and IsNewEnough(Major, Minor);
end;

function FileIsNewEnough(const Name: String): Boolean;
var
  VersionMS, VersionLS: Cardinal;
begin
  Result := GetVersionNumbers(ExpandConstant('{sys}\') + Name, VersionMS, VersionLS)
    and IsNewEnough(VersionMS shr 16, VersionMS and $FFFF);
end;

function HasRuntime(): Boolean;
begin
  Result := RegistryHasRuntime(HKLM64) or RegistryHasRuntime(HKLM32)
    or (FileIsNewEnough('vcruntime140.dll') and FileIsNewEnough('vcruntime140_1.dll')
        and FileIsNewEnough('msvcp140.dll') and FileIsNewEnough('msvcp140_1.dll'));
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ErrorCode: Integer;
begin
  if (CurStep = ssPostInstall) and not HasRuntime() then
  begin
    Log('The Visual C++ runtime {#MinRuntimeMajor}.{#MinRuntimeMinor} or later was not found.');
    { Silent installs answer No, so nothing opens. }
    if SuppressibleMsgBox('DawIt needs Microsoft''s Visual C++ Redistributable for x64, version {#MinRuntimeMajor}.{#MinRuntimeMinor} or later. '
        + 'It isn''t on this PC yet, and DawIt can''t start without it.' + #13#10#13#10
        + 'Download it from Microsoft now? Run the file you get, then open DawIt.',
        mbConfirmation, MB_YESNO, IDNO) = IDYES then
    begin
      if not ShellExec('open', '{#RuntimeDownload}', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode) then
        Log('Could not open {#RuntimeDownload}: ' + SysErrorMessage(ErrorCode));
    end;
  end;
end;
