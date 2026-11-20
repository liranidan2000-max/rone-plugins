<#
.SYNOPSIS
  Signs the unsigned AAX bundles from build-aax.ps1 with PACE's wraptool, so Pro Tools loads them.

.DESCRIPTION
  Needs, once:
    1. The iLok USB plugged in, with the "PACE Central Access" licence on it
       (iLok License Manager: drag it from the right onto the iLok).
    2. PACECodeSigningForAAXSDK for Windows from PACE Central (DEVELOPER > SDK Download),
       installed; wraptool.exe comes with it. Pass -WrapTool if it is not found.
    3. A wrap configuration GUID per product, created in PACE Central, written into
       aax-products.json ("wcguid"). A product without one is skipped, not failed.
    4. A code-signing certificate in Windows' certificate store (CurrentUser\My), given as
       -CertThumbprint. Without one, -SelfSigned makes a "RONE AUDIO AAX" code-signing
       certificate in CurrentUser\My and uses it (community reports say Pro Tools accepts a
       self-signed one on Windows; the real one, once bought, is the safer choice).

  The first signing asks for the iLok account password (wraptool prompts for it; it is
  never stored by this script). Signed bundles go to <Out>\signed, then each one is checked
  with "wraptool verify".

.EXAMPLE
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\sign-aax.ps1" -SelfSigned
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\sign-aax.ps1" -CertThumbprint 0123ABCD... -Only RoneClipper
#>
param(
    [string[]] $Only,
    [string]   $Account = "lirankalifa",               # iLok user id the PACE licence sits on
    [string]   $CertThumbprint,
    [switch]   $SelfSigned,
    [string]   $WrapTool,
    [string]   $Out = "D:\RONE PLUGINS\_build\aax"
)

$ErrorActionPreference = 'Stop'
$cfg = Get-Content (Join-Path $PSScriptRoot "aax-products.json") -Raw | ConvertFrom-Json
$products = $cfg.products
if ($Only) { $products = $products | Where-Object { $Only -contains $_.id } }
$unsigned = Join-Path $Out "unsigned"
$signed   = Join-Path $Out "signed"
New-Item -ItemType Directory -Force $signed | Out-Null

# ---- wraptool -----------------------------------------------------------------------------
if (-not $WrapTool) {
    $cands = @(Get-Command wraptool.exe -ErrorAction SilentlyContinue | ForEach-Object Source)
    $cands += "$env:ProgramFiles\PACEAntiPiracy\Eden\Fusion\Versions\6\bin\wraptool.exe" | Where-Object { Test-Path $_ }
    foreach ($root in "$env:ProgramFiles", "${env:ProgramFiles(x86)}", "$env:USERPROFILE\Downloads", "D:\RONE PLUGINS\_build\aax\_pace") {
        if (Test-Path $root) {
            $cands += Get-ChildItem $root -Recurse -Depth 7 -Filter wraptool.exe -ErrorAction SilentlyContinue | ForEach-Object FullName
        }
    }
    $WrapTool = $cands | Select-Object -First 1
}
if (-not $WrapTool -or -not (Test-Path $WrapTool)) {
    throw "wraptool.exe not found. Install PACECodeSigningForAAXSDK (PACE Central > DEVELOPER > SDK Download) or pass -WrapTool <path>."
}
Write-Host "wraptool: $WrapTool"
# The SDK installer sets PACE_FUSION_HOME machine-wide, but a shell opened before the install
# does not have it, and "wraptool verify" refuses to run without it.
if (-not $env:PACE_FUSION_HOME) {
    $env:PACE_FUSION_HOME = [Environment]::GetEnvironmentVariable('PACE_FUSION_HOME', 'Machine')
    if (-not $env:PACE_FUSION_HOME) { $env:PACE_FUSION_HOME = Split-Path (Split-Path $WrapTool) }
}

# ---- certificate --------------------------------------------------------------------------
if (-not $CertThumbprint) {
    $subject = "CN=RONE AUDIO AAX"
    $cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object Subject -eq $subject |
            Sort-Object NotAfter -Descending | Select-Object -First 1
    if (-not $cert -and $SelfSigned) {
        # wraptool 6 can only use a key held by a classic CryptoAPI provider. The default
        # New-SelfSignedCertificate key (CNG/KSP) fails with "Can't sign with the certificate
        # identified by the thumbprint" (found 20.11.2026).
        $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject $subject `
                  -CertStoreLocation Cert:\CurrentUser\My -KeyAlgorithm RSA -KeyLength 2048 `
                  -HashAlgorithm SHA256 -Provider "Microsoft Enhanced RSA and AES Cryptographic Provider" `
                  -KeySpec Signature -KeyExportPolicy Exportable -NotAfter (Get-Date).AddYears(10)
        Write-Host "Made a self-signed code-signing certificate $($cert.Thumbprint)"
    }
    if (-not $cert) { throw "No certificate. Pass -CertThumbprint <thumbprint> (CurrentUser\My) or -SelfSigned." }
    $CertThumbprint = $cert.Thumbprint
}
Write-Host "certificate: $CertThumbprint"

# ---- sign ---------------------------------------------------------------------------------
$results = @()
foreach ($p in $products) {
    $in = Join-Path $unsigned "$($p.bundle).aaxplugin"
    $row = [ordered]@{ id = $p.id; result = ""; note = "" }
    if (-not (Test-Path $in))   { $row.result = "SKIPPED"; $row.note = "not built (run build-aax.ps1)"; $results += [pscustomobject]$row; continue }
    if (-not $p.wcguid)         { $row.result = "SKIPPED"; $row.note = "no wcguid in aax-products.json"; $results += [pscustomobject]$row; continue }
    $outB = Join-Path $signed "$($p.bundle).aaxplugin"
    if (Test-Path $outB) { Remove-Item -Recurse -Force $outB }
    Write-Host ""
    Write-Host "=== $($p.id) ===" -ForegroundColor Cyan
    # wraptool cannot take paths with spaces everywhere (forum reports), so it works on a
    # space-free temp copy and the result is moved into place.
    $tmp = Join-Path $env:TEMP ("aaxsign_" + $p.id)
    if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
    New-Item -ItemType Directory $tmp | Out-Null
    # A bundle is signed in place: wraptool 6 refuses --out different from --in for a
    # directory bundle ("The specified output option should be the same as input option").
    $work = Join-Path $tmp "plugin.aaxplugin"
    Copy-Item -Recurse $in $work
    # wraptool writes progress to stderr; under 'Stop' PowerShell 5.1 would turn that into a
    # terminating error, so native calls run with 'Continue' and are judged by exit code.
    $ErrorActionPreference = 'Continue'
    & $WrapTool sign --verbose --account $Account --wcguid $p.wcguid --signid $CertThumbprint `
        --in $work --out $work 2>&1 | ForEach-Object { "$_" }
    $signExit = $LASTEXITCODE
    $verifyExit = -1
    if ($signExit -eq 0) {
        # On Windows "verify" takes the binary inside the bundle, not the bundle folder.
        & $WrapTool verify --verbose --in (Join-Path $work "Contents\x64\$($p.bundle).aaxplugin") 2>&1 | ForEach-Object { "$_" }
        $verifyExit = $LASTEXITCODE
    }
    $ErrorActionPreference = 'Stop'
    if ($signExit -ne 0)   { $row.result = "FAILED"; $row.note = "wraptool sign exit $signExit";     $results += [pscustomobject]$row; continue }
    if ($verifyExit -ne 0) { $row.result = "FAILED"; $row.note = "wraptool verify exit $verifyExit"; $results += [pscustomobject]$row; continue }
    Move-Item $work $outB
    Remove-Item -Recurse -Force $tmp
    $row.result = "SIGNED"
    $results += [pscustomobject]$row
}

Write-Host ""
$results | Format-Table -AutoSize | Out-String | Write-Host
Write-Host "Signed bundles: $signed"
Write-Host "Next: build the installer with"
Write-Host "  & `"$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe`" `"$((Resolve-Path (Join-Path $PSScriptRoot '..\..\installer')).Path)\RONE_AAX.iss`""
if ($results | Where-Object result -eq "FAILED") { exit 1 }
