; ============================================================================
; RONE Analyzer — Silent Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{E7F8A9B0-C1D2-3456-EF01-6789ABCDEF01}
AppName=RONE Analyzer
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RONEAnalyzer_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Analyzer (RONE)
; Streamlined for silent operation
DisableWelcomePage=yes
DisableDirPage=yes
DisableReadyPage=yes
; Never let a silent plugin install force-close the running Plugins Center
; (Restart Manager would kill the app that launched this installer)
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; ============================================================================
; Files — the standalone analyser plus its VST3 companion. The bridge sits
; on the DAW's master bus: it taps the master for the analyser to measure
; and renders band solo / mono inside the DAW's own signal path.
; ============================================================================
[Files]
Source: "..\RONEAnalyzer\build-ci\RONEAnalyzer_artefacts\Release\RONE Analyzer.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

Source: "..\RONEAnalyzer\build-ci\RONEBridge_artefacts\Release\VST3\RONE Analyzer Bridge.vst3\*"; \
  DestDir: "{commoncf64}\VST3\RONE Analyzer Bridge.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; The licence and the third-party notices travel with the binary. The JUCE
; Starter tier and the absent ASIO SDK are both licence conditions, and a
; condition nobody can read is a condition nobody can keep.
Source: "..\RONEAnalyzer\LICENSE";         DestDir: "{app}"; DestName: "RONE Analyzer LICENSE.txt"; Flags: ignoreversion
Source: "..\RONEAnalyzer\THIRD-PARTY.md";  DestDir: "{app}"; DestName: "RONE Analyzer THIRD-PARTY.txt"; Flags: ignoreversion

; The interface typefaces are embedded in the executable. The SIL Open Font
; License permits that; it requires the licence texts to travel with them.
Source: "..\RONEAnalyzer\Resources\fonts\OFL-Inter.txt"; DestDir: "{app}"; DestName: "Inter OFL.txt"; Flags: ignoreversion
Source: "..\RONEAnalyzer\Resources\fonts\OFL-JetBrainsMono.txt"; DestDir: "{app}"; DestName: "JetBrains Mono OFL.txt"; Flags: ignoreversion

; The user manual ships with the plugin (docs/manuals in the monorepo, built by
; docs/manuals/tools). The Center's "Manual" menu entry opens it from here.
Source: "..\docs\manuals\RONE Analyzer - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion

; ============================================================================
; Shortcuts — a standalone application, unlike the plugins, is something you
; actually launch.
; ============================================================================
[Icons]
Name: "{commonprograms}\RONE Analyzer"; Filename: "{app}\RONE Analyzer.exe"


; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RONEAnalyzer"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; ============================================================================
; The bridge was called "RONE Bridge" before this build. Left behind, a DAW
; rescan lists the plugin twice - one plugin code, two names - and half the
; time the user loads the stale copy.
; ============================================================================
[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\RONE Bridge.vst3"
