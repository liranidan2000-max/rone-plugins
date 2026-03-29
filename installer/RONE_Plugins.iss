; ============================================================================
; RONE Plugins - Combined Windows Installer
; Inno Setup Script
;
; Installs:
;   VST3 plugins  → C:\Program Files\Common Files\VST3\RONE\
;   Standalone apps → C:\Program Files\RONE Plugins\
;   Desktop shortcuts for all 4 standalone apps
;
; Build from repo root:
;   iscc installer\RONE_Plugins.iss
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

#define MyAppName     "RONE Plugins"
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
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=RONE_Plugins_Installer_v{#MyAppVersion}
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
; Installation components — let the user pick what to install
; ============================================================================
[Types]
Name: "full";    Description: "Full installation (VST3 + Standalone apps)"
Name: "vst3";    Description: "VST3 plugins only"
Name: "standalone"; Description: "Standalone applications only"
Name: "custom";  Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 Plugins";          Types: full vst3 custom
Name: "standalone"; Description: "Standalone Applications"; Types: full standalone custom

; ============================================================================
; Optional tasks (checkboxes on the final wizard page)
; ============================================================================
[Tasks]
Name: "desktopicons"; Description: "Create desktop shortcuts for all standalone apps"; GroupDescription: "Additional shortcuts:"; Components: standalone

; ============================================================================
; Files
; ============================================================================
[Files]

; --------------------------------------------------------------------------
; ReverseReverb — VST3
; Source path is relative to this .iss file location (installer\ folder)
; --------------------------------------------------------------------------
Source: "..\ReverseReverbVST\build-ci\ReverseReverb_artefacts\Release\VST3\ReverseReverb.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\ReverseReverb.vst3"; \
  Components: vst3; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; ReverseReverb — Standalone
Source: "..\ReverseReverbVST\build-ci\ReverseReverb_artefacts\Release\Standalone\ReverseReverb.exe"; \
  DestDir: "{app}"; \
  Components: standalone; \
  Flags: ignoreversion

; --------------------------------------------------------------------------
; RONE Stems Fixer — Standalone only (GUI app, no VST3)
; --------------------------------------------------------------------------
Source: "..\RoneStemsFixer\build-ci\RoneStemsFixer_artefacts\Release\RONE Stems Fixer.exe"; \
  DestDir: "{app}"; \
  Components: standalone; \
  Flags: ignoreversion

; --------------------------------------------------------------------------
; Rone Stutter — VST3
; --------------------------------------------------------------------------
Source: "..\RoneStutter\build-ci\RoneStutter_artefacts\Release\VST3\Rone Stutter.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\Rone Stutter.vst3"; \
  Components: vst3; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Rone Stutter — Standalone
Source: "..\RoneStutter\build-ci\RoneStutter_artefacts\Release\Standalone\Rone Stutter.exe"; \
  DestDir: "{app}"; \
  Components: standalone; \
  Flags: ignoreversion

; --------------------------------------------------------------------------
; Rone Flanger — VST3
; --------------------------------------------------------------------------
Source: "..\rone-flanger-\build-ci\ManualFlanger_artefacts\Release\VST3\Rone Flanger.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\Rone Flanger.vst3"; \
  Components: vst3; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Rone Flanger — Standalone
Source: "..\rone-flanger-\build-ci\ManualFlanger_artefacts\Release\Standalone\Rone Flanger.exe"; \
  DestDir: "{app}"; \
  Components: standalone; \
  Flags: ignoreversion

; ============================================================================
; Start Menu + Desktop shortcuts
; ============================================================================
[Icons]
; --- Start Menu group ---
Name: "{group}\ReverseReverb";    Filename: "{app}\ReverseReverb.exe";         Components: standalone
Name: "{group}\RONE Stems Fixer"; Filename: "{app}\RONE Stems Fixer.exe";      Components: standalone
Name: "{group}\Rone Stutter";     Filename: "{app}\Rone Stutter.exe";          Components: standalone
Name: "{group}\Rone Flanger";     Filename: "{app}\Rone Flanger.exe";          Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

; --- Desktop shortcuts (only if user ticked the checkbox) ---
Name: "{commondesktop}\ReverseReverb";    Filename: "{app}\ReverseReverb.exe";    Components: standalone; Tasks: desktopicons
Name: "{commondesktop}\RONE Stems Fixer"; Filename: "{app}\RONE Stems Fixer.exe"; Components: standalone; Tasks: desktopicons
Name: "{commondesktop}\Rone Stutter";     Filename: "{app}\Rone Stutter.exe";     Components: standalone; Tasks: desktopicons
Name: "{commondesktop}\Rone Flanger";     Filename: "{app}\Rone Flanger.exe";     Components: standalone; Tasks: desktopicons

; ============================================================================
; Post-install: offer to launch one of the apps
; ============================================================================
[Run]
Filename: "{app}\Rone Stutter.exe"; \
  Description: "Launch Rone Stutter"; \
  Flags: nowait postinstall skipifsilent unchecked; \
  Components: standalone
