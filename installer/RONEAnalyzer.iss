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

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; ============================================================================
; Files — Standalone only. The analyser is not a plugin: it attaches to the
; audio device beside the DAW rather than sitting in its signal path.
; ============================================================================
[Files]
Source: "..\RONEAnalyzer\build-ci\RONEAnalyzer_artefacts\Release\RONE Analyzer.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; The licence and the third-party notices travel with the binary. The JUCE
; Starter tier and the absent ASIO SDK are both licence conditions, and a
; condition nobody can read is a condition nobody can keep.
Source: "..\RONEAnalyzer\LICENSE";         DestDir: "{app}"; DestName: "RONE Analyzer LICENSE.txt"; Flags: ignoreversion
Source: "..\RONEAnalyzer\THIRD-PARTY.md";  DestDir: "{app}"; DestName: "RONE Analyzer THIRD-PARTY.txt"; Flags: ignoreversion

; The interface typefaces are embedded in the executable. The SIL Open Font
; License permits that; it requires the licence texts to travel with them.
Source: "..\RONEAnalyzer\Resources\fonts\OFL-Inter.txt"; DestDir: "{app}"; DestName: "Inter OFL.txt"; Flags: ignoreversion
Source: "..\RONEAnalyzer\Resources\fonts\OFL-JetBrainsMono.txt"; DestDir: "{app}"; DestName: "JetBrains Mono OFL.txt"; Flags: ignoreversion

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
