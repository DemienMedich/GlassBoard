#ifndef AppVersion
  #error AppVersion must come from CMakeLists.txt
#endif
#ifndef PackageDir
  #error PackageDir must contain a verified standalone package
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif
#ifndef AppIdentity
  #define AppIdentity "CBA09359-8F14-4A2D-8B13-6EF568933B14"
#endif

[Setup]
AppId={{{#AppIdentity}}
AppName=GlassBoard
AppVersion={#AppVersion}
AppVerName=GlassBoard {#AppVersion}
AppPublisher=Pharos
DefaultDirName={localappdata}\Programs\GlassBoard
DefaultGroupName=GlassBoard
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UsePreviousAppDir=yes
OutputDir={#OutputDir}
OutputBaseFilename=GlassBoardSetup_{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\GlassBoard.exe
VersionInfoVersion={#AppVersion}
VersionInfoProductVersion={#AppVersion}
CloseApplications=yes
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык на рабочем столе"; Flags: unchecked

[Files]
Source: "{#PackageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "smoke.png,smoke-result.txt"

[Icons]
#ifndef TestBuild
Name: "{autoprograms}\GlassBoard"; Filename: "{app}\GlassBoard.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\GlassBoard"; Filename: "{app}\GlassBoard.exe"; WorkingDir: "{app}"; Tasks: desktopicon
#endif

[Run]
Filename: "{app}\GlassBoard.exe"; Description: "Запустить GlassBoard"; Flags: nowait postinstall skipifsilent
