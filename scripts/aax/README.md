# RONE plugins for Pro Tools (AAX)

Pro Tools loads only AAX plugins signed with PACE's tools, and PACE signing needs the
physical iLok. So AAX is built and signed **on this PC**, not in CI:

```
build-aax.ps1   ->  D:\RONE PLUGINS\_build\aax\unsigned\*.aaxplugin   (any time, no iLok)
sign-aax.ps1    ->  D:\RONE PLUGINS\_build\aax\signed\*.aaxplugin     (iLok + PACE tools)
RONE_AAX.iss    ->  D:\RONE PLUGINS\_build\aax\installer\RONE_AAX_Installer_v<date>.exe
```

`aax-products.json` lists the products (folder, JUCE target, bundle name, Pro Tools
category, PACE wcguid).

## Build (no iLok needed)

```
& "D:\RONE PLUGINS\rone-plugins\scripts\aax\build-aax.ps1"
& "D:\RONE PLUGINS\rone-plugins\scripts\aax\build-aax.ps1" -Only RoneClipper,RoneDelay
```

It builds the **released** code: for each product it exports the commit the monorepo points
at (`git ls-tree HEAD <folder>`, the commit CI releases), plus `Shared/` from the same commit,
into `_build\aax\src`, appends `AAX` to the `juce_add_plugin` FORMATS line and sets
`AAX_CATEGORY` there, and builds only `<target>_AAX`. No plugin repo or working copy is
touched, so DEV branches (rack-dev) never leak into the AAX build. JUCE 8.0.4 and 8.0.12 both
carry the AAX SDK, so no SDK download is needed. Logs: `_build\aax\logs\<id>.log`.

## One-time PACE setup (needs the iLok USB)

1. iLok License Manager, signed in as `lirankalifa`, iLok plugged in: drag **PACE Central
   Access** from the right onto the iLok. Sign out, sign in again: a **PACE CENTRAL** button
   appears.
2. PACE Central > DEVELOPER > SDK Download: `PACECodeSigningForAAXSDK` for Windows (and Mac,
   for later). Install it; `sign-aax.ps1` finds `wraptool.exe` under Program Files or
   Downloads, or pass `-WrapTool`.
3. PACE Central: create one product (wrap configuration) per plugin and paste each
   **wcguid** into `aax-products.json`.
4. Certificate: the SSL.com code-signing certificate once it arrives
   (`-CertThumbprint <thumbprint>`), or `-SelfSigned` now.

## Sign and pack

```
& "D:\RONE PLUGINS\rone-plugins\scripts\aax\sign-aax.ps1" -SelfSigned
& "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe" "D:\RONE PLUGINS\rone-plugins\installer\RONE_AAX.iss"
```

The first `wraptool sign` asks for the iLok password. `/DUNSIGNED` packs the unsigned bundles
instead (for the Pro Tools Developer build only; ordinary Pro Tools refuses them).

## Not done yet

- **Mac AAX.** Same flow on a Mac (Xcode build, `wraptool sign --signid "Developer ID
  Application: ..."`, then notarize). The iLok has to be on the Mac for that.
- **Pro Tools test.** Nothing has been opened in Pro Tools yet. The Pro Tools Developer
  build (devauth@avid.com) has not answered since 29.10; with signed bundles an ordinary
  Pro Tools (trial) is enough.
- **Center / site.** The installer is standalone. Showing AAX in the Center, the manifest and
  the site's format chips comes after the first Pro Tools test passes.
