; ============================================================================
; Signed with Authenticode by CI since 24.11.2026 (docs/CODE-SIGNING.md).
; RONE Hook — Silent Plugin Installer
; Designed to be invoked by RONE Plugins Center with /VERYSILENT flag
;
; Besides the VST3 + standalone, HOOK needs its own runtime (everything it runs
; is installed here, nothing has to be on the machine before):
;   C:\ProgramData\RONE Plugins\Hook\bin     RoneHookEngine.exe + onnxruntime / DirectML (the voice
;                                            separation, out of the DAW's process), yt-dlp (YouTube),
;                                            node (the JavaScript runtime YouTube needs; qjs = QuickJS fallback),
;                                            ffmpeg (LGPL build: decoding / cutting)
;   C:\ProgramData\RONE Plugins\Hook\models  Kim_Vocal_2.onnx (UVR MDX-Net, MIT) - no download on first open
; yt-dlp keeps itself current: HOOK copies it to %APPDATA%\RonePlugins\Hook\bin and runs `yt-dlp -U`
; once a day and again whenever a link fails (LinkLoader::freshenYtdlp).
; The payload (not in git): CI runs RoneHook\tools\fetch_runtime.py (the tested runtime, pinned by
; sha256), then RoneHook\tools\stage_payload.py build-ci (this build's engine), into RoneHook\payload.
; ============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif

[Setup]
#ifdef SIGN
SignTool=rone
SignedUninstaller=yes
SignToolRunMinimized=yes
SignToolRetryCount=3
#endif
AppId={{B812F406-735A-4DA6-8E7B-69E0051B8AA9}
AppName=RONE Hook
AppVersion={#MyAppVersion}
AppPublisher=Liran Rone Kalifa
DefaultDirName={commonpf}\RONE Plugins
DisableProgramGroupPage=yes
OutputBaseFilename=RoneHook_Installer_v{#MyAppVersion}
OutputDir=..\build-output
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=RONE Hook (RONE)
DisableWelcomePage=yes
DisableDirPage=yes
DisableReadyPage=yes
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; VST3 plugin bundle
Source: "..\RoneHook\build-ci\RoneHook_artefacts\Release\VST3\RONE Hook.vst3\*"; \
  DestDir: "{commoncf}\VST3\RONE\RONE Hook.vst3"; \
  Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\RoneHook\build-ci\RoneHook_artefacts\Release\Standalone\RONE Hook.exe"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

; The WebView2 loader beside the standalone (JUCE loads it at runtime)
Source: "..\webview2\Microsoft.Web.WebView2.1.0.1901.177\build\native\x64\WebView2Loader.dll"; \
  DestDir: "{app}"; \
  Flags: onlyifdoesntexist uninsneveruninstall

; The user manual (the Center's "Manual" entry opens it from here)
Source: "..\docs\manuals\RONE Hook - User Manual.pdf"; \
  DestDir: "{app}\Manuals"; \
  Flags: ignoreversion skipifsourcedoesntexist

; HOOK's runtime: the separation engine + its DLLs, yt-dlp, Node + QuickJS, ffmpeg
Source: "..\RoneHook\payload\win\bin\*"; \
  DestDir: "{commonappdata}\RONE Plugins\Hook\bin"; Flags: ignoreversion
; the embeddable Python (python.org, PSF licence) that keeps yt-dlp loaded while HOOK is open (bin\hookworker.py
; + bin\yt-dlp.pyz): a video's words in ~2.5 s instead of 5-9 s of starting yt-dlp.exe for every video
Source: "..\RoneHook\payload\win\py\*";   DestDir: "{commonappdata}\RONE Plugins\Hook\py"; Flags: ignoreversion recursesubdirs createallsubdirs
; the vocal model (67 MB): never re-copied when it is already there and whole
Source: "..\RoneHook\payload\models\Kim_Vocal_2.onnx"; \
  DestDir: "{commonappdata}\RONE Plugins\Hook\models"; Flags: onlyifdoesntexist
; MEANING: the sentence model (all-MiniLM-L6-v2, int8, 23 MB, Apache-2.0) + its vocabulary
Source: "..\RoneHook\payload\models\minilm-l6-int8.onnx"; DestDir: "{commonappdata}\RONE Plugins\Hook\models"; Flags: ignoreversion
Source: "..\RoneHook\payload\models\minilm-vocab.txt";    DestDir: "{commonappdata}\RONE Plugins\Hook\models"; Flags: ignoreversion
; third-party licences (ffmpeg is the LGPL build, yt-dlp public domain, QuickJS-ng MIT, Node MIT, Python PSF,
; all-MiniLM-L6-v2 Apache-2.0)
Source: "..\RoneHook\payload\licenses\*"; \
  DestDir: "{commonappdata}\RONE Plugins\Hook\licenses"; Flags: ignoreversion
; No RONE Control hands block: HOOK has no rack since 1.1.1 (Liran 22.11.2026: Control only in effects).

[Registry]
Root: HKCU; Subkey: "Software\RONE\Plugins\RoneHook"; ValueType: string; \
  ValueName: "InstalledVersion"; ValueData: "{#MyAppVersion}"; Flags: uninsdeletekey

[InstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\RONE Hook.vst3"
Type: files; Name: "{commoncf}\VST3\RONE\RONE Hook.vst3\Contents\Resources\moduleinfo.json"

[UninstallDelete]
Type: filesandordirs; Name: "{commonappdata}\RONE Plugins\Hook"

[Code]
// HOOK's own helpers - its yt-dlp workers on the embedded Python, Node, ffmpeg, the voice engine - run from
// {commonappdata}\RONE Plugins\Hook. The Center installs only once no DAW and no standalone holds the plug-in, but it
// does not know these: one left behind (a DAW that crashed in the middle of a download or of making a voice) holds
// its own file, and the silent install stops with code 5 and rolls back.
// They are found BY THEIR PATH: another program's python.exe / node.exe / ffmpeg.exe is never touched.
procedure EndHookHelpers(const Dir: String);
var
  Locator, Service, Procs, P: Variant;
  I, N: Integer;
  Q: String;
begin
  try
    Q := AddBackslash(Dir);
    StringChangeEx(Q, '\', '\\', True);
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Service := Locator.ConnectServer('.', 'root\CIMV2');
    Procs := Service.ExecQuery('SELECT * FROM Win32_Process WHERE ExecutablePath LIKE ''' + Q + '%''');
    N := Procs.Count;
    for I := 0 to N - 1 do
    begin
      try
        P := Procs.ItemIndex(I);
        Log('RONE Hook: ending its helper ' + P.Name);
        P.Terminate(0);
      except
      end;
    end;
    if N > 0 then Sleep(500);             // Windows lets go of the files a moment after the process ends
  except
    Log('RONE Hook: could not look for running helpers - ' + GetExceptionMessage);
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  EndHookHelpers(ExpandConstant('{commonappdata}\RONE Plugins\Hook'));
  Result := '';
end;

function InitializeUninstall(): Boolean;
begin
  EndHookHelpers(ExpandConstant('{commonappdata}\RONE Plugins\Hook'));
  Result := True;
end;
