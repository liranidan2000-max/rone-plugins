; 2026-10-26: rebuilt for the macOS .pkg - run #323 lost it to Apple notarization timeouts (fixed in b9db578)
; ============================================================================
; RONE AFTERSPACE — Silent Plugin Installer
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
AppId={{A1B2C3D4-E5F6-7890-ABCD-AFTERSPACE01}
AppName=RONE AFTERSPACE
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneAfterspace_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE AFTERSPACE (RONE)
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
Source: "..\RONE AFTER SPACE\build-ci\RoneAfterspace_artefacts\Release\VST3\RONE AFTERSPACE.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE AFTERSPACE.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RONE AFTER SPACE\build-ci\RoneAfterspace_artefacts\Release\Standalone\RONE AFTERSPACE.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; Not needed by this build: the plugin and the standalone link WebView2's
; loader statically (JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 in the
; product's CMakeLists.txt). The DLL is still installed for older builds on
; disk that predate static linking and load it at runtime, and it is never
; uninstalled because {app} is shared with them (audit R247).
; CI downloads the NuGet package to <workspace>\webview2 before compiling.
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: onlyifdoesntexist uninsneveruninstall


; The user manual ships with the plugin (docs/manuals in the monorepo, built by
; docs/manuals/tools). The Center's "Manual" menu entry opens it from here.
Source: "..\docs\manuals\RONE AFTERSPACE - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion restartreplace

; RONE Control's hands (the rack): the hand model and its runtime (Google MediaPipe,
; Apache 2.0, NOTICE.txt beside them), fetched by CI with scripts/ci/fetch_hands.py when
; this plugin has a rack (Source/rack.json). One folder shared by every RONE plugin with a
; rack, so it stays when this plugin is uninstalled. Without the files the installer just
; skips them (a build before the rack, a local dev build).
Source: "..\hands\mp\*"; \
  DestDir: "{app}\Hands"; \
  Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist uninsneveruninstall

; ============================================================================
; Registry — write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneAfterspace"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

[InstallDelete]
; A copy of this plugin at the TOP of Common Files\VST3 (JUCE's default dev-build
; target, or a hand-copied bundle) shadows the installed one in \RONE\ - a DAW
; that sees both opens whichever it scanned first, usually the stale one.
Type: filesandordirs; Name: "{commoncf}\VST3\RONE AFTERSPACE.vst3"
; A moduleinfo.json from an older build (VST3_AUTO_MANIFEST was on until 2026-09) describes
; that build, not this one - RONE Reverse Reverb's said "Instrument|Synth", so FL Studio kept
; listing 1.1.7, an effect, as a synth. These builds ship no manifest: drop the old one first.
Type: files; Name: "{commoncf}\VST3\RONE\RONE AFTERSPACE.vst3\Contents\Resources\moduleinfo.json"
