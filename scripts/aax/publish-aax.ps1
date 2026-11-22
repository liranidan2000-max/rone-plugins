<#
.SYNOPSIS
  Uploads the PACE-signed AAX bundles to the "AAX-signed" GitHub release, where CI's
  "Fetch signed AAX (Pro Tools)" step picks each one up and packs it into that plugin's
  Windows installer. The Center then installs AAX like any other part of the plugin.

.DESCRIPTION
  Order of a release with AAX:
    1. build-aax.ps1  - builds the commit the monorepo points at (what CI will build)
    2. sign-aax.ps1   - PACE signature + verify (iLok plugged in)
    3. publish-aax.ps1 (this) - zips <signed>\<bundle>.aaxplugin as <Id>.aaxplugin.zip and
       uploads it with --clobber
    4. push the release commit; CI packs the AAX only when its FileVersion equals the
       plugin's CMake version, so a bundle from older code is never shipped.
  The release is a pre-release so it never shows as the repository's "Latest".

.EXAMPLE
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\publish-aax.ps1"
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\publish-aax.ps1" -Only RoneClipper -DryRun
#>
param(
    [string[]] $Only,
    [string]   $Out  = "D:\RONE PLUGINS\_build\aax",
    [string]   $Tag  = "AAX-signed",
    [string]   $Repo = "liranidan2000-max/rone-plugins",
    [switch]   $DryRun
)
$ErrorActionPreference = 'Stop'
$cfg = Get-Content (Join-Path $PSScriptRoot "aax-products.json") -Raw | ConvertFrom-Json
$products = $cfg.products
if ($Only) { $products = $products | Where-Object { $Only -contains $_.id } }
$signed = Join-Path $Out "signed"
$src    = Join-Path $Out "src"
$pub    = Join-Path $Out "publish"
New-Item -ItemType Directory -Force $pub | Out-Null

$results = @()
foreach ($p in $products) {
    $row = [ordered]@{ id = $p.id; version = ""; result = ""; note = "" }
    $bundle = Join-Path $signed "$($p.bundle).aaxplugin"
    $bin    = Join-Path $bundle "Contents\x64\$($p.bundle).aaxplugin"
    if (-not (Test-Path $bin)) { $row.result = "SKIPPED"; $row.note = "not signed (run sign-aax.ps1)"; $results += [pscustomobject]$row; continue }
    $fv = (Get-Item $bin).VersionInfo.FileVersion
    $row.version = $fv
    # the version CI will compare against: the exported (released) CMakeLists
    $cm = Join-Path $src "$($p.folder)\CMakeLists.txt"
    $want = if ((Test-Path $cm) -and ([IO.File]::ReadAllText($cm) -match 'project\(\s*\S+\s+VERSION\s+([0-9.]+)')) { $Matches[1] } else { "" }
    if ($want -and $fv -ne $want) {
        $row.result = "SKIPPED"; $row.note = "binary says $fv, source says $want (rebuild: build-aax.ps1)"
        $results += [pscustomobject]$row; continue
    }
    $zip = Join-Path $pub "$($p.id).aaxplugin.zip"
    if (Test-Path $zip) { Remove-Item -Force $zip }
    Compress-Archive -Path $bundle -DestinationPath $zip -CompressionLevel Optimal
    $row.result = "ZIPPED"
    $results += [pscustomobject]$row
}

$zips = Get-ChildItem $pub -Filter *.aaxplugin.zip | Where-Object { ($results | Where-Object result -eq "ZIPPED").id -contains ($_.Name -replace '\.aaxplugin\.zip$','') }
if (-not $DryRun -and $zips) {
    $ErrorActionPreference = 'Continue'
    & gh release view $Tag --repo $Repo *> $null
    if ($LASTEXITCODE -ne 0) {
        & gh release create $Tag --repo $Repo --prerelease --title "AAX signed bundles (CI input)" `
            --notes "PACE-signed AAX bundles for Pro Tools, one zip per plugin. Uploaded by scripts/aax/publish-aax.ps1 from the iLok PC; CI packs each into that plugin's Windows installer when the versions match. Not a download for customers."
        if ($LASTEXITCODE -ne 0) { throw "could not create release $Tag" }
    }
    & gh release upload $Tag --repo $Repo --clobber @($zips.FullName)
    if ($LASTEXITCODE -ne 0) { throw "upload to $Tag failed" }
    $ErrorActionPreference = 'Stop'
    foreach ($r in $results) { if ($r.result -eq "ZIPPED") { $r.result = "UPLOADED" } }
}

$results | Format-Table -AutoSize | Out-String | Write-Host
if ($DryRun) { Write-Host "Dry run: zips in $pub, nothing uploaded." }
