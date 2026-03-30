; ============================================================================
; ReverseReverb — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{B2C3D4E5-F6A7-8901-BCDE-F12345678901}
AppName=ReverseReverb
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=ReverseReverb_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=ReverseReverb (RONE)
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
Source: "..\ReverseReverbVST\build-ci\ReverseReverb_artefacts\Release\VST3\ReverseReverb.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\ReverseReverb.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\ReverseReverbVST\build-ci\ReverseReverb_artefacts\Release\Standalone\ReverseReverb.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\ReverseReverb"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
