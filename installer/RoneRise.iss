; ============================================================================
; RONE Rise — Silent Plugin Installer
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
AppId={{E4F73C6B-63EB-4D37-9E02-0C6E91F7E9CE}
AppName=RONE Rise
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneRise_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Rise (RONE)
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
Source: "..\RoneRise\build-ci\RoneRise_artefacts\Release\VST3\RONE Rise.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Rise.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RoneRise\build-ci\RoneRise_artefacts\Release\Standalone\RONE Rise.exe"; \
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
Source: "..\docs\manuals\RONE Rise - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion skipifsourcedoesntexist

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneRise"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; A copy of this plugin at the TOP of Common Files\VST3 (JUCE's default dev-build
; target, or a hand-copied bundle) shadows the installed one in \RONE\ - a DAW
; that sees both opens whichever it scanned first, usually the stale one.
[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE Rise.vst3"
