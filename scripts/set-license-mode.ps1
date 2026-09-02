# ============================================================================
# RONE bundle remote kill-switch
#
#   .\scripts\set-license-mode.ps1 open
#       -> everything runs WITHOUT a license (current beta behavior)
#
#   .\scripts\set-license-mode.ps1 enforced
#   .\scripts\set-license-mode.ps1 enforced -Message "Beta ended - get a license at roneaudio.com"
#       -> every Center + plugin install with internet locks within a few
#          hours (or on next launch). Plugins bypass cleanly (dry through),
#          the Center blocks install/open and shows the message.
#
# The script edits license_mode/license_message in versions.json, commits
# ONLY that file, and pushes. [skip ci] keeps it from triggering builds.
# ============================================================================
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("open", "enforced")]
    [string]$Mode,

    [string]$Message = ""
)

$repo     = Split-Path $PSScriptRoot -Parent
$manifest = Join-Path $repo "versions.json"

if (-not (Test-Path $manifest)) {
    Write-Error "versions.json not found at $manifest"
    exit 1
}

$content = Get-Content $manifest -Raw

if ($content -notmatch '"license_mode"') {
    Write-Error "versions.json has no license_mode field - is this an old manifest?"
    exit 1
}

$escapedMsg = $Message -replace '\\', '\\\\' -replace '"', '\"'
$content = $content -replace '"license_mode":\s*"[^"]*"',    ('"license_mode": "' + $Mode + '"')
$content = $content -replace '"license_message":\s*"[^"]*"', ('"license_message": "' + $escapedMsg + '"')

# Windows PowerShell 5.1's -Encoding utf8 writes a BOM, and a BOM in front of
# '{' breaks strict JSON parsers (the Center, jq in CI). Write plain UTF-8.
[System.IO.File]::WriteAllText($manifest, $content, (New-Object System.Text.UTF8Encoding($false)))

git -C $repo add versions.json
git -C $repo commit -m "license: set mode to $Mode [skip ci]"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Nothing to commit (mode already '$Mode'?)"
    exit 0
}
git -C $repo push

Write-Host ""
Write-Host "license_mode is now '$Mode'." -ForegroundColor Green
if ($Mode -eq "enforced") {
    Write-Host "Every install refreshes its cache within ~6 hours (or on next launch)."
} else {
    Write-Host "All plugins run license-free again on their next refresh."
}
