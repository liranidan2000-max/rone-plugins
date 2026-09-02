; ============================================================================
; Rone Sync Verb — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-SYNCVERB0001}
AppName=RONE Sync Verb
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneSyncVerb_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Sync Verb (RONE)
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
Source: "..\rone sync verb\build-ci\RoneSyncVerb_artefacts\Release\VST3\RONE Sync Verb.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Sync Verb.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\rone sync verb\build-ci\RoneSyncVerb_artefacts\Release\Standalone\RONE Sync Verb.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; The WebView2 loader has to sit beside the standalone executable: JUCE loads
; it with LoadLibrary at runtime, and without it the UI silently falls back
; to the legacy IE control and shows an error page instead of the interface.
; CI downloads the NuGet package to <workspace>\webview2 before compiling.
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: onlyifdoesntexist uninsneveruninstall


; The user manual ships with the plugin (docs/manuals in the monorepo, built by
; docs/manuals/tools). The Center's "Manual" menu entry opens it from here.
Source: "..\docs\manuals\RONE Sync Verb - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneSyncVerb"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; ============================================================================
; The plugin was called "Rone Sync Verb" before this build. Left behind, a DAW
; rescan lists it twice - one plugin code, two names - and half the time the
; user loads the stale copy. The plugin code is unchanged, so projects that
; saved the old name load the renamed plugin without a remap.
; ============================================================================
[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE\Rone Sync Verb.vst3"
Type: files; Name: "{app}\Rone Sync Verb.exe"
