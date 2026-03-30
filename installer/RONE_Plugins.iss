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
; Files — only the Center application
; ============================================================================
[Files]
Source: "..\RonePluginsCenter\build-ci\RonePluginsCenter_artefacts\Release\RONE Plugins Center.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

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
; Post-install: launch the Center
; ============================================================================
[Run]
Filename: "{app}\RONE Plugins Center.exe"; \
  Description: "Launch RONE Plugins Center"; \
  Flags: nowait postinstall skipifsilent
