# build_plugin.ps1 -Src <plugin folder> -Short <build dir name> -Target <cmake target> [-Bundle <name>] [-FakeCopyDir]
# Configures (once) and builds one plugin target out of tree under <Root>, against a local JUCE
# 8.0.4 tree, at BELOW-NORMAL priority (MSBuild and cl.exe inherit it) so a live DAW keeps its
# CPU. Never installs anything: a plugin with COPY_PLUGIN_AFTER_BUILD TRUE (Sync Verb,
# AFTERSPACE, Choir, Motion) needs -FakeCopyDir, which points JUCE's VST3 copy folder into <Root>.
param(
    [Parameter(Mandatory = $true)][string] $Src,
    [Parameter(Mandatory = $true)][string] $Short,
    [Parameter(Mandatory = $true)][string] $Target,
    [string] $Bundle = "",
    [switch] $FakeCopyDir,
    [string] $Root = (Join-Path $env:TEMP "wv2t"),
    [string] $JuceDir = "D:/RONE PLUGINS/rone-plugins/RoneStucker/build-vs2022/_deps/juce-src",
    [string] $WebView2Dir = "D:/RONE PLUGINS/rone-plugins/RoneStucker/build-vs2022/_webview2_wrapper"
)
(Get-Process -Id $PID).PriorityClass = 'BelowNormal'
$b   = Join-Path $Root $Short
$log = Join-Path $Root ($Short + ".log")
New-Item -ItemType Directory -Force $Root | Out-Null
if ($FakeCopyDir) { $env:CommonProgramW6432 = (Join-Path $Root "fake-common") }   # JUCE reads it at configure time only
if (-not (Test-Path (Join-Path $b "CMakeCache.txt"))) {
    & cmake -S $Src -B $b -G "Visual Studio 17 2022" -A x64 "-DFETCHCONTENT_SOURCE_DIR_JUCE=$JuceDir" "-DJUCE_WEBVIEW2_PACKAGE_LOCATION=$WebView2Dir" *> $log
    if ($LASTEXITCODE -ne 0) { Write-Output "CONFIGURE FAILED ($Short)"; Get-Content $log -Tail 20; exit 1 }
}
$copyTo = Select-String -Path (Join-Path $b "*_VST3.vcxproj") -Pattern "-Ddest=([^ ]+)" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($copyTo -and ($copyTo.Matches[0].Groups[1].Value -notlike "$Root*")) {
    Write-Output "REFUSED: this target copies the plugin to $($copyTo.Matches[0].Groups[1].Value) after the build - rerun with -FakeCopyDir on a fresh -Short"
    exit 1
}
& cmake --build $b --config Release --target $Target -- "-maxCpuCount:8" *>> $log
$code = $LASTEXITCODE
Select-String -Path $log -Pattern " error " | Select-Object -First 15 | ForEach-Object { $_.Line }
if ($code -eq 0 -and $Bundle -ne "") {
    $vst = Get-ChildItem $b -Recurse -Directory -Filter "*.vst3" | Where-Object { $_.FullName -match "_artefacts\\Release\\VST3\\[^\\]+\.vst3$" } | Select-Object -First 1
    if ($vst) {
        $dest = Join-Path $Root ("bundles\" + $Bundle)
        New-Item -ItemType Directory -Force $dest | Out-Null
        Copy-Item -Recurse -Force $vst.FullName $dest
        Write-Output ("bundle   " + (Join-Path $dest $vst.Name))
    }
}
Write-Output "BUILD $Short $Target exit $code"
exit $code
