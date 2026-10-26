; ============================================================================
; RONE Control - Silent Plugin Installer
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
AppId={{7FDE9D3D-91AC-4CD5-9F45-48D4E9673B57}
AppName=RONE Control
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneControl_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Control (RONE)
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
; Files - VST3 only (no standalone: there is nothing for a standalone to control)
; ============================================================================
[Files]
; VST3 plugin bundle. Contents\Resources\fl inside it carries the FL Studio script
; the Center copies into FL's Hardware folder (RonePluginsCenter/Source/ControlPort.h);
; the hand model rides inside the DLL as resources and is written out at run time.
Source: "..\RoneControl\build-ci\RoneControl_artefacts\Release\VST3\RONE Control.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Control.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Not needed by this build: the plugin links WebView2's loader statically
; (JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 in the product's CMakeLists.txt).
; The DLL is still installed for older builds on disk that predate static
; linking and load it at runtime, and it is never uninstalled because {app} is
; shared with them (audit R247). CI downloads the NuGet package to
; <workspace>\webview2 before compiling.
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: onlyifdoesntexist uninsneveruninstall

; The user manual ships with the plugin (docs/manuals in the monorepo, built by
; docs/manuals/tools). The Center's "Manual" menu entry opens it from here.
Source: "..\docs\manuals\RONE Control - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion restartreplace

; ============================================================================
; Registry - write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneControl"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; A copy of this plugin at the TOP of Common Files\VST3 (JUCE's default dev-build
; target, or a hand-copied bundle) shadows the installed one in \RONE\ - a DAW
; that sees both opens whichever it scanned first, usually the stale one.
[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE Control.vst3"
; A moduleinfo.json from an older build (VST3_AUTO_MANIFEST was on until 2026-09) describes
; that build, not this one. These builds ship no manifest: drop the old one first.
Type: files; Name: "{commoncf}\VST3\RONE\RONE Control.vst3\Contents\Resources\moduleinfo.json"
; Pre-release dev builds replaced a DLL FL Studio held by renaming it (*.locked-old*, *.old*,
; *.shared*, *.rack*): never part of the plugin.
Type: files; Name: "{commoncf}\VST3\RONE\RONE Control.vst3\Contents\x86_64-win\*.locked-old*"
Type: files; Name: "{commoncf}\VST3\RONE\RONE Control.vst3\Contents\x86_64-win\*.old*"
Type: files; Name: "{commoncf}\VST3\RONE\RONE Control.vst3\Contents\x86_64-win\*.shared*"
Type: files; Name: "{commoncf}\VST3\RONE\RONE Control.vst3\Contents\x86_64-win\*.rack*"
