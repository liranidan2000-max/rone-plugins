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
#   .\scripts\set-license-mode.ps1 -SignedLicences required
#   .\scripts\set-license-mode.ps1 -SignedLicences off
#       -> licence cut-over R004 (docs/LICENCE-CUTOVER.md, Stage 1d / 3):
#          "required" adds the top-level "signed_licences": "required", after
#          which a plugin accepts ONLY a server-signed licence; "off" removes
#          it again (the rollback). Flip it only when every line of the
#          console's licence card is green, together with the Worker var
#          SIGNED_LICENCES_REQUIRED=1 - never on its own.
#
# The script edits versions.json, commits ONLY that file, and pushes.
# [skip ci] keeps it from triggering builds.
# ============================================================================
param(
    [Parameter(Position = 0)]
    [ValidateSet("open", "enforced")]
    [string]$Mode = "",

    [string]$Message = "",

    [ValidateSet("required", "off")]
    [string]$SignedLicences = ""
)

if (-not $Mode -and -not $SignedLicences) {
    Write-Error "Say what to set: a mode (open | enforced) or -SignedLicences required | off"
    exit 1
}

$repo     = Split-Path $PSScriptRoot -Parent
$manifest = Join-Path $repo "versions.json"

if (-not (Test-Path $manifest)) {
    Write-Error "versions.json not found at $manifest"
    exit 1
}

# Read as UTF-8 explicitly: Windows PowerShell 5.1's Get-Content reads a file without a BOM in the ANSI code page,
# which turned every accented letter of the i18n descriptions into mojibake on the way back out (found 25.10.2026).
$content = [System.IO.File]::ReadAllText($manifest, (New-Object System.Text.UTF8Encoding($false)))

if ($content -notmatch '"license_mode"') {
    Write-Error "versions.json has no license_mode field - is this an old manifest?"
    exit 1
}

$what = @()
if ($Mode) {
    $escapedMsg = $Message -replace '\\', '\\\\' -replace '"', '\"'
    $content = $content -replace '"license_mode":\s*"[^"]*"',    ('"license_mode": "' + $Mode + '"')
    $content = $content -replace '"license_message":\s*"[^"]*"', ('"license_message": "' + $escapedMsg + '"')
    $what += "mode $Mode"
}

if ($SignedLicences -eq "required") {
    if ($content -match '"signed_licences"') {
        $content = $content -replace '"signed_licences":\s*"[^"]*"', '"signed_licences": "required"'
    } else {
        # Right after license_mode, at the top level, where the plugins and the Center read it.
        $content = $content -replace '("license_mode":\s*"[^"]*",)', ('$1' + "`n  " + '"signed_licences": "required",')
    }
    if ($content -notmatch '"signed_licences": "required"') {
        Write-Error "could not add signed_licences to versions.json - nothing written"
        exit 1
    }
    $what += "signed licences required"
} elseif ($SignedLicences -eq "off") {
    $content = $content -replace '\r?\n\s*"signed_licences":\s*"[^"]*",', ''
    $what += "signed licences off"
}

# Windows PowerShell 5.1's -Encoding utf8 writes a BOM, and a BOM in front of
# '{' breaks strict JSON parsers (the Center, jq in CI). Write plain UTF-8.
try { $null = $content | ConvertFrom-Json } catch {
    Write-Error "the edited versions.json is not valid JSON - nothing written"
    exit 1
}
[System.IO.File]::WriteAllText($manifest, $content, (New-Object System.Text.UTF8Encoding($false)))

$summary = $what -join ", "
git -C $repo add versions.json
git -C $repo commit -m "license: $summary [skip ci]"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Nothing to commit (already '$summary'?)"
    exit 0
}
git -C $repo push

Write-Host ""
Write-Host "versions.json: $summary." -ForegroundColor Green
if ($Mode -eq "enforced") {
    Write-Host "Every install refreshes its cache within ~6 hours (or on next launch)."
} elseif ($Mode -eq "open") {
    Write-Host "All plugins run license-free again on their next refresh."
}
if ($SignedLicences -eq "required") {
    Write-Host "Now set the Worker var SIGNED_LICENCES_REQUIRED=1, and watch the support inbox for 7 days." -ForegroundColor Yellow
} elseif ($SignedLicences -eq "off") {
    Write-Host "Unset the Worker var SIGNED_LICENCES_REQUIRED too. Online machines relax within ~6 hours."
}
