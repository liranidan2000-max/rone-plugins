# reopen.ps1 -Vst3 <bundle> [-Cycles 4] [-Hold 6] [-GapMs 300] [-TrackProcs] [-Label x] [-CopyProfile <EBWebView parent>]
# One window of one plugin, closed and opened again <Cycles> times with <GapMs> between, on a hidden
# desktop with its own WebView2 profile. Prints, for every open, how long until its page existed.
# -CopyProfile copies a real plugin profile folder (e.g. %APPDATA%\RonePlugins\RoneIron_WebView2) first,
# to test with what the customer's machine really carries. Never points at the live profile itself.
param(
    [Parameter(Mandatory = $true)][string] $Vst3,
    [int] $Cycles = 4,
    [double] $Hold = 6,
    [int] $GapMs = 300,
    [switch] $Stress,                # one fixed debug port for every browser: a closed window's browser
                                     # then lingers 20-40 s, as Liran's did in FL - the case the fix is for
    [int] $LingerMs = 0,             # suspend each closed window's browser this long (slow shutdown, made certain)
    [switch] $TrackProcs,            # also log browser process start/exit (CIM polling: adds load)
    [string] $Label = "run",
    [string] $CopyProfile = "",
    [string] $Root = (Join-Path $env:TEMP "wv2t")
)
$ErrorActionPreference = "Stop"
$node = "D:\RONE PLUGINS\tools\node-v24.20.0-win-x64\node.exe"
$hostExe = Join-Path $Root "host\ReopenEditorHost_artefacts\Release\ReopenEditorHost.exe"
$hidden  = Join-Path $Root "host\Release\HiddenDesktopRun.exe"
if (-not (Test-Path $hostExe)) { throw "build the host first (README.md): $hostExe" }

$udf = Join-Path $Root ("udf-reopen-" + $Label + "-" + [DateTime]::Now.ToString("HHmmssfff"))
if ($CopyProfile) { robocopy $CopyProfile $udf /E /NFL /NDL /NJH /NJS /R:0 /W:0 /XF lockfile | Out-Null }
New-Item -ItemType Directory -Force $udf | Out-Null
# random port per browser, written to <folder>\EBWebView\DevToolsActivePort (reopen_watch.mjs reads them)
$env:WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS = $(if ($Stress) { "--remote-debugging-port=9482" } else { "--remote-debugging-port=0" })
# A build with Shared/RoneWebViewFolder.h picks its own folders: give it a test root instead of
# WEBVIEW2_USER_DATA_FOLDER (that variable overrides the folder and would switch the fix off).
# A build without it gets the old single forced folder - never the profile a DAW is using.
$dll = Get-ChildItem (Join-Path $Vst3 "Contents\x86_64-win") -Filter *.vst3 -File | Select-Object -First 1
$fixed = [System.Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($dll.FullName)).Contains("RONE_WEBVIEW_TEST_ROOT")
if ($fixed) { $env:RONE_WEBVIEW_TEST_ROOT = $udf } else { $env:WEBVIEW2_USER_DATA_FOLDER = $udf }
Write-Output ("profile " + $(if ($fixed) { "per-window folders (RoneWebViewFolder build)" } else { "one forced folder (build without RoneWebViewFolder)" }))

$total = [int]($Cycles * ($Hold + $GapMs / 1000.0) + $LingerMs / 1000.0 + 25)
$out = Join-Path $Root ("reopen-" + $Label + ".txt")
$watch = Start-Process -FilePath $node -ArgumentList @("`"$(Join-Path $PSScriptRoot 'reopen_watch.mjs')`"", "`"$udf`"", $total) `
                       -NoNewWindow -PassThru -RedirectStandardOutput "$out.cdp"
# browser processes of this profile: when each starts and when it is really gone (the port closes earlier)
$procLog = "$out.proc"
$procJob = if ($TrackProcs) { Start-Job -ArgumentList $udf, $procLog, $total -ScriptBlock {
    param($udf, $log, $total)
    $known = @{}; $end = (Get-Date).AddSeconds($total + 20); Set-Content $log ""
    $ms = { [DateTimeOffset]::Now.ToUnixTimeMilliseconds() }
    while ((Get-Date) -lt $end) {
        $now = @{}
        Get-CimInstance Win32_Process -Filter "Name='msedgewebview2.exe'" | Where-Object { $_.CommandLine -like "*$udf*" -and $_.CommandLine -notmatch '--type=' } |
            ForEach-Object { $now[[int]$_.ProcessId] = 1 }
        foreach ($p in $now.Keys) { if (-not $known.ContainsKey($p)) { Add-Content $log "PROC START $p $(& $ms)" } }
        foreach ($p in @($known.Keys)) { if (-not $now.ContainsKey($p)) { Add-Content $log "PROC EXIT $p $(& $ms)" } }
        $known = $now
        Start-Sleep -Milliseconds 400
    }
} }
Start-Sleep -Milliseconds 800
$proc = Start-Process -FilePath $hidden -ArgumentList @([string]$total, "`"$hostExe`"", "`"$Vst3`"", $Cycles, $Hold, $GapMs, $LingerMs) `
                      -NoNewWindow -PassThru -RedirectStandardOutput $out -RedirectStandardError "$out.err"
$proc.WaitForExit()
Start-Sleep -Seconds 15          # keep watching: does the last browser process really go away?
Stop-Process -Id $watch.Id -ErrorAction SilentlyContinue
if ($procJob) { Stop-Job $procJob -ErrorAction SilentlyContinue; Remove-Job $procJob -Force -ErrorAction SilentlyContinue }
Remove-Item Env:WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS, Env:WEBVIEW2_USER_DATA_FOLDER, Env:RONE_WEBVIEW_TEST_ROOT -ErrorAction SilentlyContinue

$ev = @()
foreach ($l in (Get-Content $out) + (Get-Content "$out.cdp") + (Get-Content $procLog -ErrorAction SilentlyContinue)) {
    $p = $l -split ' '
    if ($p.Count -ge 2 -and $p[-1] -match '^\d{13}$') { $ev += [pscustomobject]@{ T = [int64]$p[-1]; E = ($p[0..($p.Count - 2)] -join ' ') } }
    elseif ($l) { Write-Output $l }
}
$ev = $ev | Sort-Object T
$t0 = $ev[0].T
foreach ($e in $ev) { "{0,8:N2} s  {1}" -f (($e.T - $t0) / 1000.0), $e.E }
Write-Output "---- $Label : gap $GapMs ms"
$opens = $ev | Where-Object { $_.E -like 'OPEN *' }
$white = 0; $slow = 0; $worst = 0.0
foreach ($o in $opens) {
    $c = $o.E.Split(' ')[1]
    $pg = $ev | Where-Object { $_.E -eq "PAGE $c" } | Select-Object -First 1
    if ($pg) {
        $s = ($pg.T - $o.T) / 1000.0; $worst = [Math]::Max($worst, $s); if ($s -gt 3) { $slow++ }
        "{0}: page after {1:N2} s" -f $o.E, $s
    } else { $white++; "{0}: WHITE for all {1} s" -f $o.E, $Hold }
}
$verdict = if ($white -eq 0 -and $slow -eq 0) { "PASS" } else { "FAIL" }
"==== {0} : {1}  ({2} opens, {3} white, {4} slower than 3 s, worst {5:N2} s)" -f $Label, $verdict, $opens.Count, $white, $slow, $worst
exit $(if ($verdict -eq "PASS") { 0 } else { 1 })
