; ============================================================================
; Rone Flanger — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{E5F6A7B8-C9D0-1234-EFAB-345678901234}
AppName=Rone Flanger
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneFlanger_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=Rone Flanger (RONE)
; Streamlined for silent operation
DisableWelcomePage=yes
DisableDirPage=yes
DisableReadyPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; ============================================================================
; Files — VST3 + Standalone
; ============================================================================
[Files]
; VST3 plugin bundle
Source: "..\rone-flanger-\build-ci\ManualFlanger_artefacts\Release\VST3\Rone Flanger.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\Rone Flanger.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\rone-flanger-\build-ci\ManualFlanger_artefacts\Release\Standalone\Rone Flanger.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneFlanger"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
