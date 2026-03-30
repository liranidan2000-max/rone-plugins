; ============================================================================
; RONE Stems Fixer — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{C3D4E5F6-A7B8-9012-CDEF-123456789012}
AppName=RONE Stems Fixer
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneStemsFixer_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Stems Fixer (RONE)
; Streamlined for silent operation
DisableWelcomePage=yes
DisableDirPage=yes
DisableReadyPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; ============================================================================
; Files — Standalone only (no VST3)
; ============================================================================
[Files]
Source: "..\RoneStemsFixer\build-ci\RoneStemsFixer_artefacts\Release\RONE Stems Fixer.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneStemsFixer"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
