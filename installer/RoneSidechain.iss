; ============================================================================
; RONE Sidechain — Silent Plugin Installer
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
; The same AppId as the dev builds of "RONE Lock" (its first name, 2026-10-20): this
; installer takes that install over instead of leaving a second entry behind.
AppId={{1697BE9A-F028-4267-87B6-0DB1CA940FF4}
AppName=RONE Sidechain
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneSidechain_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Sidechain (RONE)
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
Source: "..\RoneSidechain\build-ci\RoneSidechain_artefacts\Release\VST3\RONE Sidechain.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Sidechain.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RoneSidechain\build-ci\RoneSidechain_artefacts\Release\Standalone\RONE Sidechain.exe"; \
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
Source: "..\docs\manuals\RONE Sidechain - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion skipifsourcedoesntexist

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneSidechain"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey
; the first dev builds were "RONE Lock": its key goes, so the Center no longer sees it installed
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneLock"; ValueType: none; Flags: deletekey

; A copy of this plugin at the TOP of Common Files\VST3 (JUCE's default dev-build
; target, or a hand-copied bundle) shadows the installed one in \RONE\ - a DAW
; that sees both opens whichever it scanned first, usually the stale one.
[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE Sidechain.vst3"
; A moduleinfo.json from an older build (VST3_AUTO_MANIFEST was on until 2026-09) describes
; that build, not this one - RONE Reverse Reverb's said "Instrument|Synth", so FL Studio kept
; listing 1.1.7, an effect, as a synth. These builds ship no manifest: drop the old one first.
Type: files; Name: "{commoncf}\VST3\RONE\RONE Sidechain.vst3\Contents\Resources\moduleinfo.json"
; The first dev builds' files ("RONE Lock", 1.0.0.x): replaced by this plugin
Type: filesandordirs; Name: "{commoncf}\VST3\RONE\RONE Lock.vst3"
Type: filesandordirs; Name: "{commoncf}\VST3\RONE Lock.vst3"
Type: files; Name: "{app}\RONE Lock.exe"
