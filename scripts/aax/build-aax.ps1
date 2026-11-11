<#
.SYNOPSIS
  Builds the Windows AAX (Pro Tools) version of every RONE plugin, from the RELEASED code.

.DESCRIPTION
  Pro Tools only loads AAX plugins signed with PACE's tools, and PACE signing needs the
  physical iLok, so AAX cannot be built and shipped by CI like VST3/AU. This script is the
  local half: it builds unsigned .aaxplugin bundles; sign-aax.ps1 signs them; RONE_AAX.iss
  packs the signed ones into one installer.

  It never touches a plugin's working copy (those are often on a DEV branch, rack-dev).
  For each product it exports the commit the monorepo points at (= what CI releases) with
  git archive, plus the monorepo's Shared/, into a staging folder, adds AAX to the
  juce_add_plugin FORMATS there, and builds only the <target>_AAX target.

.EXAMPLE
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\build-aax.ps1"
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\build-aax.ps1" -Only RoneClipper,RoneDelay
  & "D:\RONE PLUGINS\rone-plugins\scripts\aax\build-aax.ps1" -Ref origin/main
#>
param(
    [string[]] $Only,                                  # product ids from aax-products.json; empty = all
    [string]   $Ref = "HEAD",                          # monorepo commit whose submodule pointers are built
    [string]   $Out = "D:\RONE PLUGINS\_build\aax",    # staging, build trees, logs, unsigned output
    [switch]   $Clean                                  # wipe the product's build tree first
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$cfg  = Get-Content (Join-Path $PSScriptRoot "aax-products.json") -Raw | ConvertFrom-Json
$products = $cfg.products
if ($Only) {
    $products = $products | Where-Object { $Only -contains $_.id }
    $unknown = $Only | Where-Object { $products.id -notcontains $_ }
    if ($unknown) { throw "Unknown product id(s): $($unknown -join ', ')" }
}

$src      = Join-Path $Out "src"
$bld      = Join-Path $Out "build"
$logs     = Join-Path $Out "logs"
$unsigned = Join-Path $Out "unsigned"
$cache    = Join-Path $Out "_cache"
foreach ($d in $src, $bld, $logs, $unsigned, $cache) { New-Item -ItemType Directory -Force $d | Out-Null }

# Native tools write progress to stderr; under 'Stop' Windows PowerShell 5.1 would turn
# the first such line into a terminating error, so native calls run under 'Continue'.
function Invoke-Native { param([string]$Exe, [string[]]$NativeArgs, [string]$Log)
    $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    try {
        if ($Log) { & $Exe @NativeArgs *>> $Log } else { & $Exe @NativeArgs 2>&1 | ForEach-Object { "$_" } | Out-Null }
        return $LASTEXITCODE
    } finally { $ErrorActionPreference = $old }
}
function Invoke-Git { param([string[]]$GitArgs)
    $rc = Invoke-Native git $GitArgs
    if ($rc -ne 0) { throw "git $($GitArgs -join ' ') failed ($rc)" }
}

function Expand-Tar([string]$tarFile, [string]$dest) {
    New-Item -ItemType Directory -Force $dest | Out-Null
    # Windows' tar skips names it cannot map (AFTER SPACE has a Hebrew-named PNG) and then
    # exits non-zero; the caller checks that the CMakeLists arrived instead.
    $rc = Invoke-Native "$env:SystemRoot\System32\tar.exe" @("-xf", $tarFile, "-C", $dest)
    if ($rc -ne 0) { Write-Host "  (tar exit ${rc}: some file names were skipped)" -ForegroundColor DarkYellow }
    Remove-Item $tarFile -Force
}

# ---- shared inputs, fetched once ------------------------------------------------------
$wv2Ver  = "1.0.1901.177"
$wv2Root = Join-Path $cache "webview2"
$wv2Pkg  = Join-Path $wv2Root "Microsoft.Web.WebView2.$wv2Ver"
if (-not (Test-Path "$wv2Pkg\build\native\include\WebView2.h")) {
    Write-Host "Fetching WebView2 $wv2Ver (same package CI uses)..."
    New-Item -ItemType Directory -Force $wv2Pkg | Out-Null
    $zip = Join-Path $cache "webview2.zip"
    Invoke-WebRequest "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$wv2Ver" -OutFile $zip
    Expand-Archive $zip -DestinationPath $wv2Pkg -Force
    Remove-Item $zip
}

function Get-Juce([string]$tag) {
    $dir = Join-Path $cache "juce-$tag"
    if (-not (Test-Path "$dir\CMakeLists.txt")) {
        Write-Host "Fetching JUCE $tag..."
        Invoke-Git @("clone", "--quiet", "--depth", "1", "--branch", $tag, "https://github.com/juce-framework/JUCE.git", $dir)
    }
    return $dir
}

# Shared/ from the same monorepo commit
$sharedDst = Join-Path $src "Shared"
if (Test-Path $sharedDst) { Remove-Item -Recurse -Force $sharedDst }
$t = Join-Path $cache "shared.tar"
Invoke-Git @("-C", $repo, "archive", "--format=tar", "-o", $t, $Ref, "--", "Shared")
Expand-Tar $t $src

# ---- per product ------------------------------------------------------------------------
$results = @()
foreach ($p in $products) {
    $id = $p.id; $folder = $p.folder
    $log = Join-Path $logs "$id.log"
    "=== $id  ($(Get-Date -Format s))" | Out-File $log -Encoding utf8
    Write-Host ""
    Write-Host "=== $id ===" -ForegroundColor Cyan
    $row = [ordered]@{ id = $id; commit = ""; version = ""; result = "FAILED"; note = "" }
    try {
        # 1. export the released source
        $entry = (& git -C $repo ls-tree $Ref -- $folder) -split "\s+", 4
        if (-not $entry[0]) { throw "$folder is not in $Ref" }
        $dst = Join-Path $src $folder
        if (Test-Path $dst) { Remove-Item -Recurse -Force $dst }
        $tar = Join-Path $cache "$id.tar"
        if ($entry[1] -eq "commit") {
            $sha = $entry[2]; $row.commit = $sha.Substring(0, 7)
            $gitDir = Join-Path $repo $folder
            if (-not (Test-Path (Join-Path $gitDir ".git"))) {
                # uninitialised submodule: a private bare mirror, so the checkout stays as it is
                $url = (& git -C $repo config -f .gitmodules --get "submodule.$folder.url")
                $gitDir = Join-Path $cache "repos\$folder.git"
                if (-not (Test-Path $gitDir)) { Invoke-Git @("clone", "--quiet", "--bare", $url, $gitDir) }
            }
            if ((Invoke-Native git @("-C", $gitDir, "cat-file", "-e", "$sha^{commit}")) -ne 0) {
                Invoke-Git @("-C", $gitDir, "fetch", "--quiet", "origin", $sha) }
            Invoke-Git @("-C", $gitDir, "archive", "--format=tar", "--prefix=$folder/", "-o", $tar, $sha)
        } else {
            $row.commit = "vendored@" + (& git -C $repo rev-parse --short $Ref)
            Invoke-Git @("-C", $repo, "archive", "--format=tar", "-o", $tar, $Ref, "--", $folder)
        }
        Expand-Tar $tar $src
        if (-not (Test-Path (Join-Path $dst "CMakeLists.txt"))) { throw "export of $folder has no CMakeLists.txt" }

        # A plugin that vendors JUCE in its own repo (Flanger) has no extras/Build in git
        # (the repo's .gitignore drops Build/); CI adds it from JUCE, so do the same.
        $vj = Join-Path $dst "JUCE\CMakeLists.txt"
        if ((Test-Path $vj) -and -not (Test-Path (Join-Path $dst "JUCE\extras\Build"))) {
            $vt = if ([IO.File]::ReadAllText($vj) -match 'project\(JUCE VERSION ([0-9.]+)') { $Matches[1] } else { throw "vendored JUCE without a version" }
            Copy-Item -Recurse (Join-Path (Get-Juce $vt) "extras\Build") (Join-Path $dst "JUCE\extras\Build")
        }

        # 2. add AAX to the staging copy's CMakeLists
        $cm = Join-Path $dst "CMakeLists.txt"
        $text = [IO.File]::ReadAllText($cm)
        if ($text -match 'project\(\s*\S+\s+VERSION\s+([0-9.]+)') { $row.version = $Matches[1] }
        $tag = if ($text -match '(?s)FetchContent_Declare\(\s*JUCE.*?GIT_TAG\s+(\S+)') { $Matches[1] } else { "8.0.4" }
        if ($p.formats_line -eq "bridge") {
            $new = [regex]::Replace($text, 'set\(RONE_BRIDGE_FORMATS VST3\)', 'set(RONE_BRIDGE_FORMATS VST3 AAX)')
        } else {
            $rx = [regex]"(?s)(juce_add_plugin\(\s*$([regex]::Escape($p.target))\b.*?\n([ \t]*)FORMATS[ \t]+)([^\r\n#]+)"
            $new = $rx.Replace($text, { param($m)
                $fmts = ($m.Groups[3].Value.Trim() -split '\s+' | Where-Object { $_ -ne 'AAX' }) + 'AAX'
                $m.Groups[1].Value + ($fmts -join ' ') }, 1)
        }
        if ($new -eq $text) { throw "could not find the FORMATS line of juce_add_plugin($($p.target))" }
        # Pro Tools menu category, right after juce_add_plugin(<target>
        $new = ([regex]"juce_add_plugin\(\s*$([regex]::Escape($p.target))\b").Replace($new,
            "juce_add_plugin($($p.target)`n    AAX_CATEGORY $($p.category)", 1)
        [IO.File]::WriteAllText($cm, $new)
        $pn = ([regex]"(?s)juce_add_plugin\(\s*$([regex]::Escape($p.target))\b.*?PRODUCT_NAME\s+""([^""]+)""").Match($new)
        $bundle = if ($pn.Success) { $pn.Groups[1].Value } else { $p.bundle }
        if ($bundle -ne $p.bundle) { $row.note = "PRODUCT_NAME is '$bundle', json says '$($p.bundle)'" }

        # 3. configure + build only the AAX target
        $b = Join-Path $bld $id
        if ($Clean -and (Test-Path $b)) { Remove-Item -Recurse -Force $b }
        $juce = Get-Juce $tag
        $flags = @(
            "-S", $dst, "-B", $b, "-G", "Visual Studio 17 2022", "-A", "x64",
            "-DFETCHCONTENT_SOURCE_DIR_JUCE=$juce",
            "-DCMAKE_PROJECT_INCLUDE=$($src.Replace('\','/'))/Shared/RoneBuildHooks.cmake",
            "-DJUCE_WEBVIEW2_PACKAGE_LOCATION=$wv2Root",
            "-DFETCHCONTENT_SOURCE_DIR_WEBVIEW2=$wv2Pkg",

            "-Wno-dev")
        "cmake $($flags -join ' ')" | Out-File $log -Append -Encoding utf8
        if ((Invoke-Native cmake $flags $log) -ne 0) { throw "configure failed (see $log)" }
        if ((Invoke-Native cmake @("--build", $b, "--config", "Release", "--target", "$($p.target)_AAX", "--parallel") $log) -ne 0) {
            throw "build failed (see $log)" }

        # 4. collect
        $built = Join-Path $b "$($p.target)_artefacts\Release\AAX\$bundle.aaxplugin"
        if (-not (Test-Path $built)) { throw "no bundle at $built" }
        $outBundle = Join-Path $unsigned "$bundle.aaxplugin"
        if (Test-Path $outBundle) { Remove-Item -Recurse -Force $outBundle }
        Copy-Item -Recurse $built $outBundle
        $row.result = "OK"
        Write-Host "OK  $outBundle" -ForegroundColor Green
    } catch {
        $row.note = ($row.note + " " + $_.Exception.Message).Trim()
        Write-Host "FAILED  $($row.note)" -ForegroundColor Red
        $_ | Out-String | Out-File $log -Append -Encoding utf8
    }
    $results += [pscustomobject]$row
}

# ---- summary ------------------------------------------------------------------------------
$stamp = Join-Path $unsigned "build-info.json"
$prev = @{}
if (Test-Path $stamp) { (Get-Content $stamp -Raw | ConvertFrom-Json).products | ForEach-Object { $prev[$_.id] = $_ } }
foreach ($r in $results) { $prev[$r.id] = $r | Select-Object *, @{n='built'; e={ (Get-Date -Format s) }} }
[ordered]@{ ref = (& git -C $repo rev-parse --short $Ref); products = @($prev.Values | Sort-Object id) } |
    ConvertTo-Json -Depth 4 | Out-File $stamp -Encoding utf8

Write-Host ""
$results | Format-Table -AutoSize | Out-String | Write-Host
$failed = @($results | Where-Object result -ne "OK").Count
Write-Host "Unsigned bundles: $unsigned"
if ($failed) { Write-Host "$failed product(s) failed - logs in $logs" -ForegroundColor Red; exit 1 }
