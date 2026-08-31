; ============================================================================
; Rone Stucker — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{DEDF18E3-6C8E-4090-B461-655FE6048BA6}
AppName=Rone Stucker
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneStucker_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=Rone Stucker (RONE)
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
; Files — VST3 + Standalone
; ============================================================================
[Files]
; VST3 plugin bundle
Source: "..\RoneStucker\build-ci\RoneStucker_artefacts\Release\VST3\Rone Stucker.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\Rone Stucker.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RoneStucker\build-ci\RoneStucker_artefacts\Release\Standalone\Rone Stucker.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; The WebView2 loader has to sit beside the standalone executable: JUCE loads
; it with LoadLibrary at runtime, and without it the UI silently falls back
; to the legacy IE control and shows an error page instead of the interface.
; CI downloads the NuGet package to <workspace>\webview2 before compiling.
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: onlyifdoesntexist uninsneveruninstall

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneStucker"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
