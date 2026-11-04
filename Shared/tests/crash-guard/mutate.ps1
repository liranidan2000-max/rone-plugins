# Does each case have teeth? Builds the fake plugin against a guard with one defence taken out, and
# shows which cases notice. A case that passes with its defence gone proves nothing, so run this after
# any change to RoneDawCrashGuard.cpp. A pattern that no longer matches is reported, not skipped.
#   powershell -File mutate.ps1 <the build folder of this CMake project>
#
# Expected, 2026-11-04:
#   no-wait-on-unload  17
#   no-pin             11
#   first-in-line      18
#   any-frame-is-ours  19
#   no-stack-bounds    20, 21
#   rip-zero-stops     13
#   stays-armed        12, 15, 16, 17
# Case 17 is a race, so judge it on repeat runs.
param([Parameter(Mandatory = $true)] [string] $Build)

$ErrorActionPreference = 'Continue'
(Get-Process -Id $PID).PriorityClass = 'BelowNormal'
$Shared = (Resolve-Path "$PSScriptRoot\..\..").Path
$Build = (Resolve-Path $Build).Path
$out = "$Build\mutants"
Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $out | Out-Null
$guard = [IO.File]::ReadAllText("$Shared\RoneDawCrashGuard.cpp")

$mutants = [ordered]@{
  'no-wait-on-unload'   = @("quiet < 2 && GetTickCount64() - start < writerTimeoutMs + 500", "quiet < 0")
  'no-pin'              = @(" && pinModule())", ")")
  'first-in-line'       = @("AddVectoredExceptionHandler (0, onException)", "AddVectoredExceptionHandler (1, onException)")
  'any-frame-is-ours'   = @("ours = ours || (fn != nullptr && inModule (pc));", "ours = ours || inModule (pc);")
  'no-stack-bounds'     = @("if (frameBase < stackLow || frameBase + 8 > stackHigh)", "if (false)")
  'rip-zero-stops'      = @("(walkContext.Rip != 0 || n == 0)", "walkContext.Rip != 0")
  'stays-armed'         = @("        disarm (reserved == nullptr);", "        (void) reserved;")
}
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc = "$vs\VC\Auxiliary\Build\vcvars64.bat"
$fp = "$PSScriptRoot\fake_plugin.cpp"
$fa = "$PSScriptRoot\fake_plugin_asm.asm"
$CF = '/nologo /std:c++17 /W1 /EHsc /Ox /GL /MD /Z7 /DWIN32 /D_WINDOWS /DNDEBUG'

function Build($name, $source) {
  $d = "$out\$name"
  New-Item -ItemType Directory -Force $d | Out-Null
  Copy-Item "$Build\Release\guard_proof.exe" $d
  $link = 'link /nologo /DLL /LTCG /DEBUG:FULL /OPT:REF /OPT:ICF /ENTRY:roneGuardEntry "/OUT:RONE GuardTest.vst3" guard.obj fake_plugin.obj fake_plugin_asm.obj kernel32.lib user32.lib'
  $cmd = @"
@echo off
call "$vc" >nul 2>nul
cd /d "$d"
cl $CF /c /Foguard.obj "$source" "/DJucePlugin_Name=\"RONE GuardTest\"" "/DJucePlugin_VersionString=\"9.9.9\"" >build.log 2>&1 || exit /b 1
cl $CF /c /Fofake_plugin.obj "$fp" >>build.log 2>&1 || exit /b 1
ml64 /nologo /c /Fofake_plugin_asm.obj "$fa" >>build.log 2>&1 || exit /b 1
$link >>build.log 2>&1 || exit /b 1
"@
  Set-Content "$d\build.cmd" $cmd -Encoding ASCII
  & cmd /c "$d\build.cmd"
  if ($LASTEXITCODE -ne 0) { "$name : BUILD FAILED"; Get-Content "$d\build.log" | Select-Object -Last 6; return }
  $lines = & "$d\guard_proof.exe" 2>&1 | ForEach-Object { $_.ToString() }
  $case = ''; $failed = New-Object System.Collections.ArrayList
  foreach ($l in $lines) {
    if ($l -match '^(\d+)\. ') { $case = $Matches[1] }
    elseif ($l -cmatch '^\s+FAIL|ended with|DIED') { if (-not $failed.Contains($case)) { [void]$failed.Add($case) } }
  }
  "{0,-22} cases that noticed: {1}" -f $name, ($failed -join ', ')
}

foreach ($m in $mutants.Keys) {
  $from, $to = $mutants[$m]
  if (-not $guard.Contains($from)) { "$m : pattern not found"; continue }
  $src = "$out\$m.cpp"
  [IO.File]::WriteAllText($src, $guard.Replace($from, $to), (New-Object Text.UTF8Encoding $false))
  Build $m $src
}
