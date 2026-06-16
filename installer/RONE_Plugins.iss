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
#define MyAppURL      "https://github.com/liranronekalifa"

[Setup]
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

; ============================================================================
; WebView2 detection — skip the install if the runtime is already present
; ============================================================================
[Code]
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
const
  CLIENT = 'Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';
begin
  Result := not (
    WV2Installed(HKLM, 'SOFTWARE\WOW6432Node\' + CLIENT) or
    WV2Installed(HKLM, 'SOFTWARE\' + CLIENT) or
    WV2Installed(HKCU, 'SOFTWARE\' + CLIENT)
  );
end;
