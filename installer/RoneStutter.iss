; ============================================================================
; Rone Stutter — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{D4E5F6A7-B8C9-0123-DEFA-234567890123}
AppName=Rone Stutter
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneStutter_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=Rone Stutter (RONE)
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
Source: "..\RoneStutter\build-ci\RoneStutter_artefacts\Release\VST3\Rone Stutter.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\Rone Stutter.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RoneStutter\build-ci\RoneStutter_artefacts\Release\Standalone\Rone Stutter.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneStutter"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
