; ============================================================================
; RONE Plugins for Pro Tools (AAX) - one installer with every signed AAX plugin.
;
; Built LOCALLY, not by CI: AAX bundles must be signed with PACE's wraptool and the
; physical iLok (scripts/aax/build-aax.ps1 -> scripts/aax/sign-aax.ps1 -> this file).
;   ISCC.exe installer\RONE_AAX.iss                      signed bundles (the real one)
;   ISCC.exe /DUNSIGNED installer\RONE_AAX.iss           unsigned, for the Pro Tools Developer build only
; Each plugin still checks its own licence when it opens, exactly like the VST3.
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion GetDateTimeString('yyyy.mm.dd', '', '')
#endif
#ifndef AaxRoot
  #define AaxRoot "D:\RONE PLUGINS\_build\aax"
#endif
#ifdef UNSIGNED
  #define AaxDir AaxRoot + "\unsigned"
  #define Suffix "_UNSIGNED"
#else
  #define AaxDir AaxRoot + "\signed"
  #define Suffix ""
#endif

[Setup]
#ifdef SIGN
SignTool=rone
SignedUninstaller=yes
SignToolRunMinimized=yes
SignToolRetryCount=3
#endif
AppId={{6B0C4E2A-8F3D-4A7B-9C51-2E7D9A4F1B63}
AppName=RONE Plugins for Pro Tools (AAX)
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
AppPublisherURL=https://roneaudio.com
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
DisableDirPage=yes
OutputBaseFilename=RONE_AAX_Installer_v{#MyAppVersion}{#Suffix}
OutputDir={#AaxRoot}\installer
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Plugins for Pro Tools (AAX)
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Pro Tools scans Common Files\Avid\Audio\Plug-Ins and its sub-folders.
; Whole bundles (Contents\x64\..., desktop.ini, Plugin.ico), nothing else from the folder.
Source: "{#AaxDir}\*"; Excludes: "*.json,*.log"; \
  DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\RONE"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\AAX"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

[Messages]
FinishedLabelNoIcons=RONE plugins are installed for Pro Tools. Open Pro Tools and find them in the plug-in menu under their category (Dynamics, Reverb, Delay, Modulation, Effect).
