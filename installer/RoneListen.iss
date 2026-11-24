; ============================================================================
; Signed with Authenticode by CI since 24.11.2026, the Pro Tools plug-in with the same certificate (docs/CODE-SIGNING.md).
; RONE Listen - Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
;
; VST3 only on Windows: no standalone (LISTEN lives on a DAW's master).
; The AppId is the one the DEV installer (RoneListen\installer\RoneListen.iss)
; has used since the first dev build, so a public install takes a dev one over.
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
AppId={{32349622-2EEB-4C4C-836C-4134BF9E8AD2}
AppName=RONE Listen
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneListen_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Listen (RONE)
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
; Files - VST3 (no standalone)
; ============================================================================
[Files]
; AAX for Pro Tools (Windows). Built and signed with PACE on the iLok PC (scripts/aax), then
; uploaded to the "AAX-signed" release; CI unpacks it into ..\aax\ before this compile and checks
; that its FileVersion matches CMakeLists. No signed bundle for this version = no AAX in this
; installer (skipifsourcedoesntexist), never a stale or unsigned one.
Source: "..\aax\RONE Listen.aaxplugin\*"; \
  DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\RONE\RONE Listen.aaxplugin"; \
  Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist overwritereadonly uninsremovereadonly
; VST3 plugin bundle
Source: "..\RoneListen\build-ci\RoneListen_artefacts\Release\VST3\RONE Listen.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Listen.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Not needed by this build: the plugin links WebView2's loader statically
; (JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 in RoneListen/CMakeLists.txt).
; Installed like every other RONE plugin for the older builds that share {app}
; and load it at runtime; never uninstalled for the same reason (audit R247).
; CI downloads the NuGet package to <workspace>\webview2 before compiling.
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: onlyifdoesntexist uninsneveruninstall

; The user manual (docs/manuals = the private submodule rone-manuals). The Center's
; "Manual" menu entry opens it from here. skipifsourcedoesntexist as in RoneHook.iss:
; a docs/manuals pointer without the PDF yet ships the plugin without it.
Source: "..\docs\manuals\RONE Listen - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion skipifsourcedoesntexist

; ============================================================================
; Registry - write installed version for Center detection
; ============================================================================
[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneListen"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

; ============================================================================
; A top-level copy (JUCE's default dev-build target, or a hand-copied bundle)
; shadows the installed one in \RONE\ - see RoneThrow.iss. A moduleinfo.json
; from an older build describes that build: these builds ship none.
; ============================================================================
[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE Listen.vst3"
Type: files; Name: "{commoncf}\VST3\RONE\RONE Listen.vst3\Contents\Resources\moduleinfo.json"
