<#
.SYNOPSIS
  One-time: stores the iLok account password in wraptool's own keychain, so sign-aax.ps1
  can sign without asking. Run it in a terminal YOU type in; the password is read hidden,
  handed straight to "wraptool sync" and never written to disk or printed.
  To forget it later: wraptool remove-pswd --account lirankalifa
#>
param(
    [string] $Account  = "lirankalifa",
    [string] $WrapTool = "C:\Program Files\PACEAntiPiracy\Eden\Fusion\Versions\6\bin\wraptool.exe"
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $WrapTool)) { throw "wraptool.exe not found at $WrapTool" }
Write-Host ""
Write-Host "iLok account: $Account  (the iLok USB must be plugged in)" -ForegroundColor Cyan
$sec  = Read-Host "iLok password (hidden)" -AsSecureString
$bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec)
try {
    $plain = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    & $WrapTool sync --account $Account --password $plain
    $code = $LASTEXITCODE
} finally {
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
    $plain = $null
}
if ($code -eq 0) {
    Write-Host ""
    Write-Host "OK - the password is stored in wraptool's keychain. You can close this tab." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "wraptool sync failed (exit $code). Check the password and that the iLok is plugged in, then run this again." -ForegroundColor Red
}
