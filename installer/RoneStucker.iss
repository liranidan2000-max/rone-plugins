; ============================================================================
; Rone Stucker — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
#ifdef SIGN
; CI passes /DSIGN + /Srone=... (scripts/ci/sign-windows.ps1): Setup.exe and the
; uninstaller get an Authenticode signature. Without /DSIGN nothing changes.
SignTool=rone
SignedUninstaller=yes
SignToolRunMinimized=yes
SignToolRetryCount=3
#endif
AppId={{DEDF18E3-6C8E-4090-B461-655FE6048BA6}
AppName=RONE Stucker
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
UninstallDisplayName=RONE Stucker (RONE)
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
Source: "..\RoneStucker\build-ci\RoneStucker_artefacts\Release\VST3\RONE Stucker.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Stucker.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RoneStucker\build-ci\RoneStucker_artefacts\Release\Standalone\RONE Stucker.exe"; \
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
Source: "..\docs\manuals\RONE Stucker - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneStucker"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; ============================================================================
; The plugin was called "Rone Stucker" before this build. Left behind, a DAW
; rescan lists it twice - one plugin code, two names - and half the time the
; user loads the stale copy. The plugin code is unchanged, so projects that
; saved the old name load the renamed plugin without a remap.
; ============================================================================
[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE\Rone Stucker.vst3"
Type: files; Name: "{app}\Rone Stucker.exe"
