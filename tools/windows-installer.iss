#ifndef AppVersion
  #error AppVersion must be supplied by the packaging script
#endif
#ifndef SourceDir
  #error SourceDir must be supplied by the packaging script
#endif

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
