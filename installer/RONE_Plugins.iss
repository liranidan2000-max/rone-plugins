; ============================================================================
; RONE Plugins Center — Lightweight Hub Installer
; Inno Setup Script
;
; Installs ONLY the RONE Plugins Center hub application.
; Individual plugins are installed/updated through the Center itself.
;
;   RONE Plugins Center → C:\Program Files\RONE Plugins\
;   Desktop + Start Menu shortcuts
;   Registry root for RONE plugin version tracking
;
; Build from repo root:
;   iscc installer\RONE_Plugins.iss
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

#define MyAppName     "RONE Plugins Center"
#define MyAppPublisher "Liran Rone Kalifa"
#define MyAppURL      "https://roneaudio.com"

[Setup]
#ifdef SIGN
; CI passes /DSIGN + /Srone=... (scripts/ci/sign-windows.ps1): Setup.exe and the
; uninstaller get an Authenticode signature. Without /DSIGN nothing changes.
SignTool=rone
SignedUninstaller=yes
SignToolRunMinimized=yes
SignToolRetryCount=3
#endif
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf}\RONE Plugins
DefaultGroupName=RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RONE_Plugins_Center_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; ============================================================================
; Files — the Center application + WebView2 Runtime bootstrapper
; ============================================================================
[Files]
Source: "..\RonePluginsCenter\build-ci\RonePluginsCenter_artefacts\Release\RONE Plugins Center.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; The WebView2 loader has to sit beside the standalone executable: JUCE loads
; it with LoadLibrary at runtime, and without it the UI silently falls back
; to the legacy IE control and shows an error page instead of the interface.
; CI downloads the NuGet package to <workspace>\webview2 before compiling.
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: ignoreversion uninsneveruninstall

; Microsoft Edge WebView2 Runtime evergreen bootstrapper.
; Only extracted/run when WebView2 is not already present (see NeedsWebView2).
Source: "MicrosoftEdgeWebview2Setup.exe"; \
  DestDir: "{tmp}"; \
  Flags: deleteafterinstall; \
  Check: NeedsWebView2

; ============================================================================
; Start Menu + Desktop shortcuts
; ============================================================================
[Icons]
Name: "{group}\RONE Plugins Center"; Filename: "{app}\RONE Plugins Center.exe"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\RONE Plugins Center"; Filename: "{app}\RONE Plugins Center.exe"

; ============================================================================
; Registry — create the RONE root key for plugin version tracking
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins"; Flags: uninsdeletekeyifempty
; The Center's own installed version - read back by the app so it can offer
; (and apply) its own updates exactly like it does for plugins.
Root: HKCU; Subkey: "Software\RONE\Plugins\__center__"; ValueType: string;   ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; ============================================================================
; Uninstall - the folder the elevated install batch stages installers in
; (Center 2.1.4, audit R048), only when it is empty
; ============================================================================
[UninstallDelete]
Type: dirifempty; Name: "{app}\InstallCache"

; ============================================================================
; Post-install: install WebView2 Runtime (if needed), then launch the Center
; ============================================================================
[Run]
; Install the WebView2 Runtime first so the Center's UI can render.
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; \
  Parameters: "/silent /install"; \
  StatusMsg: "Installing Microsoft WebView2 Runtime..."; \
  Check: NeedsWebView2; \
  Flags: waituntilterminated

Filename: "{app}\RONE Plugins Center.exe"; \
  Description: "Launch RONE Plugins Center"; \
  Flags: nowait postinstall skipifsilent

; Center 2.0 self-update (MainComponent::applyCenterUpdate): the running Center
; opens this installer with /VERYSILENT /RELAUNCH=1. Once setup is running
; elevated it waits for that Center to exit (InitializeSetup below), installs,
; and starts the new Center here - as the user, not elevated.
Filename: "{app}\RONE Plugins Center.exe"; \
  Parameters: "--updated"; \
  Flags: nowait runasoriginaluser; \
  Check: WantRelaunch

; ============================================================================
; WebView2 detection — skip the install if the runtime is already present
; ============================================================================
[Code]
// ---- Center self-update handshake ----
// The Center (Source/Main.cpp) holds the mutex "RonePluginsCenterRunning" for
// as long as it runs. With /RELAUNCH=1, setup creates "RoneCenterUpdateGo" -
// the Center's sign that permission was given and it should quit - then waits
// up to 30 s for the Center's mutex to go before it replaces the exe. An older
// Center (1.6.x) waits for its own exit before it starts setup and passes no
// /RELAUNCH, so nothing here changes for it.
function WantRelaunch: Boolean;
begin
  Result := ExpandConstant('{param:RELAUNCH|0}') = '1';
end;

function InitializeSetup(): Boolean;
var
  Waited: Integer;
begin
  Result := True;
  if WantRelaunch then
  begin
    CreateMutex('RoneCenterUpdateGo');
    Waited := 0;
    while CheckForMutexes('RonePluginsCenterRunning') and (Waited < 150) do
    begin
      Sleep(200);
      Waited := Waited + 1;
    end;
  end;
end;

function WV2Installed(const RootKey: Integer; const SubKey: String): Boolean;
var
  Version: String;
begin
  Result := False;
  if RegQueryStringValue(RootKey, SubKey, 'pv', Version) then
    if (Version <> '') and (Version <> '0.0.0.0') then
      Result := True;
end;

function NeedsWebView2: Boolean;
var
  Client: String;
begin
  Client := 'Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';
  Result := not (
    WV2Installed(HKLM, 'SOFTWARE\WOW6432Node\' + Client) or
    WV2Installed(HKLM, 'SOFTWARE\' + Client) or
    WV2Installed(HKCU, 'SOFTWARE\' + Client)
  );
end;


// ---- Uninstall cleanup of the per-user entries the Center writes itself ----
// The Center registers its login autostart (HKCU Run) and the ronecenter://
// handler from inside the app. Remove them only when they point at THIS
// install ({app}), so a dev build's entries are left alone.
function PointsToThisInstall(const Cmd: String): Boolean;
begin
  Result := Pos(Lowercase(ExpandConstant('{app}')), Lowercase(Cmd)) > 0;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Cmd: String;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run',
        'RONE Plugins Center', Cmd) then
      if PointsToThisInstall(Cmd) then
        RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run',
          'RONE Plugins Center');
    if RegQueryStringValue(HKCU, 'Software\Classes\ronecenter\shell\open\command',
        '', Cmd) then
      if PointsToThisInstall(Cmd) then
        RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\ronecenter');
  end;
end;
