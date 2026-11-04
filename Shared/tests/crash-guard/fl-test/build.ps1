# Builds the two plugins for the crash guard's test inside a real DAW, in a throwaway folder:
#   RONE CrashTest Guard   Clipper as a release gets it, with two crash buttons above its window
#   RONE CrashTest Plain   the same without the guard, to compare with if the DAW dies
# Nothing here can reach a release: the buttons exist only in the copy this script makes, the copy is
# of the commit this branch points to, and CI never builds this folder. See README.md.
#   powershell -File build.ps1 -Out <folder> [-Juce <JUCE source>] [-WebView2 <package location>]
param(
  [Parameter(Mandatory = $true)] [string] $Out,
  [string] $Juce = '',
  [string] $WebView2 = ''
)
$ErrorActionPreference = 'Continue'
(Get-Process -Id $PID).PriorityClass = 'BelowNormal'

$repo = (Resolve-Path "$PSScriptRoot\..\..\..\..").Path
$main = ((git -C $repo worktree list --porcelain | Select-Object -First 1) -replace '^worktree ', '').Replace('/', '\')
$commit = ((git -C $repo ls-tree HEAD RoneClipper) -split '\s+')[2]
$src = "$Out\src"
$tar = "$env:SystemRoot\System32\tar.exe"
"Clipper at $commit"

New-Item -ItemType Directory -Force $Out | Out-Null
Remove-Item -Recurse -Force $src -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$src\RoneClipper" | Out-Null
git -C "$main\RoneClipper" archive -o "$Out\clipper.tar" $commit
& $tar -xf "$Out\clipper.tar" -C "$src\RoneClipper"
git -C $repo archive -o "$Out\shared.tar" HEAD Shared
& $tar -xf "$Out\shared.tar" -C $src
Remove-Item "$Out\clipper.tar", "$Out\shared.tar"
Copy-Item "$PSScriptRoot\CrashTest.h", "$PSScriptRoot\CrashTest.cpp" "$src\RoneClipper\Source\"

function Patch($file, $pairs) {
  $path = "$src\RoneClipper\$file"
  $text = [IO.File]::ReadAllText($path).Replace("`r`n", "`n")
  foreach ($p in $pairs) {
    if (-not $text.Contains($p[0])) { throw "$file no longer has: $($p[0])" }
    $text = $text.Replace($p[0], $p[1])
  }
  [IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding $false))
}

$strip = 'CrashTest::Strip::height'
Patch 'CMakeLists.txt' @(
  @('PLUGIN_CODE            RnCl', 'PLUGIN_CODE            ${RONE_CRASH_CODE}'),
  @('FORMATS                VST3 Standalone AU', 'FORMATS                VST3'),
  @('PRODUCT_NAME           "RONE Clipper"', 'PRODUCT_NAME           "RONE CrashTest ${RONE_CRASH_KIND}"'),
  @('        Source/PluginEditor.cpp', "        Source/PluginEditor.cpp`n        Source/CrashTest.cpp")
)
Patch 'Source\PluginProcessor.cpp' @(
  @("juce::MidiBuffer&)`n{`n    juce::ScopedNoDenormals noDenormals;", "juce::MidiBuffer&)`n{`n    CrashTest::checkInAudio();`n    juce::ScopedNoDenormals noDenormals;"),
  @('#include "PluginEditor.h"', '#include "PluginEditor.h"')
)
Patch 'Source\PluginEditor.h' @(
  @('#include "PluginProcessor.h"', "#include `"PluginProcessor.h`"`n#include `"CrashTest.h`""),
  @('    juce::WebBrowserComponent webView;', "    CrashTest::Strip crashStrip;`n    juce::WebBrowserComponent webView;")
)
Patch 'Source\PluginEditor.cpp' @(
  @('if (isStandalone) h += kTitleBarHeight;', "h += $strip;"),
  @('const int tb = htmlTitleBar ? kTitleBarHeight : 0;', "const int tb = $strip;"),
  @('const int tb = isStandalone ? kTitleBarHeight : 0;', "const int tb = $strip;"),
  @('    addAndMakeVisible (webView);', "    addAndMakeVisible (crashStrip);`n    addAndMakeVisible (webView);"),
  @('    webView.setBounds (getLocalBounds());', "    auto area = getLocalBounds();`n    crashStrip.setBounds (area.removeFromTop ($strip));`n    webView.setBounds (area);")
)

$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
foreach ($kind in 'Guard', 'Plain') {
  $b = "$Out\$kind"
  $flags = @('-G', 'Visual Studio 17 2022', '-A', 'x64', "-DRONE_CRASH_KIND=$kind", "-DRONE_CRASH_CODE=RnC$($kind[0])")
  if ($kind -eq 'Guard') { $flags += "-DCMAKE_PROJECT_INCLUDE=$($src.Replace('\', '/'))/Shared/RoneBuildHooks.cmake" }
  if ($Juce) { $flags += "-DFETCHCONTENT_SOURCE_DIR_JUCE=$Juce" }
  if ($WebView2) { $flags += "-DJUCE_WEBVIEW2_PACKAGE_LOCATION=$WebView2" }
  & $cmake -S "$src\RoneClipper" -B $b @flags
  if ($LASTEXITCODE -ne 0) { throw "configure failed: $kind" }
  & $cmake --build $b --config Release --target RoneClipper_VST3 --parallel 4 -- /p:CL_MPCount=4
  if ($LASTEXITCODE -ne 0) { throw "build failed: $kind" }
  "built: $b\RoneClipper_artefacts\Release\VST3\RONE CrashTest $kind.vst3"
}
