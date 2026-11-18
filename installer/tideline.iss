; Inno Setup script for the Tideline installer. Build it with installer\build-installer.ps1, which prepares the
; "stage" folder (the launcher, the dashboard app and a private copy of Java) and then runs the Inno Setup compiler.
#define AppName "Tideline"
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif

[Setup]
; Keep this ID the same in every version: it is how Windows knows a new installer is an update of the same program.
AppId={{8F3B1C52-6A7E-4D0B-9C21-5E7A4B9D1F36}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Tideline
DefaultDirName={localappdata}\Programs\Tideline
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; installs for the current user only, so no administrator prompt, and updates can run without one too
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=output
OutputBaseFilename=Tideline-Setup-{#AppVersion}
SetupIconFile=..\icon\bolt.ico
UninstallDisplayIcon={app}\tideline.exe
LicenseFile=..\LICENCE
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
UsePreviousAppDir=yes

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "stage\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\tideline.exe"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\tideline.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\tideline.exe"; Description: "Start {#AppName}"; Flags: nowait postinstall skipifsilent
; the in-app updater passes /RELAUNCH=1 so Tideline reopens by itself after a silent update
Filename: "{app}\tideline.exe"; Flags: nowait; Check: ShouldRelaunch

[Code]
// The dashboard app runs hidden in the background, so make sure it (and Tideline) are not holding files before updating.
procedure StopRunningCopies();
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM tideline-web.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

function ShouldRelaunch(): Boolean;
begin
  Result := ExpandConstant('{param:RELAUNCH|0}') = '1';
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  StopRunningCopies();
  Result := '';
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usUninstall then
    Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM tideline-web.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;
