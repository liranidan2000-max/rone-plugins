# run_twin.ps1 -Vst3 <bundle> -Event <event the editor's timer pushes> [-Instances 2] [-Seconds 14] [-Port 9481] [-Label x]
# N editors of one VST3 built in one message-loop turn, on a hidden desktop, with their own
# WebView2 profile; then CDP: does every editor have a page, and does each page still receive
# the timer's pushes? Exit code = the CDP verdict (0 = PASS). See README.md.
param(
    [Parameter(Mandatory = $true)][string] $Vst3,
    [Parameter(Mandatory = $true)][string] $Event,
    [int] $Instances = 2,
    [double] $Seconds = 14,
    [int] $Port = 9481,              # never 9333 (RONE Choir's DevTools) or 9477 (RONE Control's tests)
    [string] $Label = "run",
    [string] $Root = (Join-Path $env:TEMP "wv2t")
)
$ErrorActionPreference = "Stop"
$node = "D:\RONE PLUGINS\tools\node-v24.20.0-win-x64\node.exe"
if (-not (Test-Path $node)) { $node = "node" }
$hostExe = Join-Path $Root "host\TwinEditorHost_artefacts\Release\TwinEditorHost.exe"
$hidden  = Join-Path $Root "host\Release\HiddenDesktopRun.exe"
if (-not (Test-Path $hostExe)) { throw "build the host first (README.md): $hostExe" }

if (Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue) { throw "port $Port is busy" }

# Its own WebView2 profile: never the one a plugin open in a DAW is using
$udf = Join-Path $Root ("udf-" + $Label + "-" + [DateTime]::Now.ToString("HHmmssfff"))
$env:WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS = "--remote-debugging-port=$Port"
$env:WEBVIEW2_USER_DATA_FOLDER = $udf

$out = Join-Path $Root ("host-" + $Label + ".txt")
$proc = Start-Process -FilePath $hidden -ArgumentList @([string]($Seconds + 30), "`"$hostExe`"", "`"$Vst3`"", $Instances, $Seconds) `
                      -NoNewWindow -PassThru -RedirectStandardOutput $out -RedirectStandardError "$out.err"
$null = $proc.Handle   # keeps ExitCode readable after the process ends
& $node (Join-Path $PSScriptRoot "cdp_check.mjs") $Port $Instances $Event ([int](($Seconds - 3) * 1000))
$cdpExit = $LASTEXITCODE
$proc.WaitForExit()
Get-Content $out
if ((Test-Path "$out.err") -and (Get-Item "$out.err").Length -gt 0) { Get-Content "$out.err" }
Remove-Item Env:WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS, Env:WEBVIEW2_USER_DATA_FOLDER
$verdict = if ($cdpExit -eq 0) { "PASS" } else { "FAIL" }
Write-Output "==== $Label : $verdict (cdp exit $cdpExit, host exit $($proc.ExitCode))"
exit $cdpExit
