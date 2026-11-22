# Mac AAX (Pro Tools for macOS) - brief for the Claude session on Liran's MacBook

Written 21.11.2026 by the Claude session on the Windows iLok PC. Your memory on this Mac does
not have the PC's notes, so **this file is the memory**. When a step finishes, add a dated line
under "Log" at the bottom, then commit and push it (see "Repo rules"), so the PC knows too.

## How to work with Liran
- **Write to him in Hebrew, every message**, short progress notes included. Code, paths and
  commands stay in English.
- He is the owner and developer of RONE PLUGINS. He likes to know where things stand: a short
  list of what is done and what is left.
- Passwords, 2FA codes, the iLok password, the Apple ID, the 1Password unlock: **his hands only**.
  Never type or read them; tell him exactly where to click and wait.
- Anything that ships to customers (a release, a pushed commit that triggers CI, a site change,
  a mail) needs his explicit "yes" in chat first.

## Where things stand (from the PC)
- Windows AAX is **LIVE** since 20.11: 16 plugins ship AAX inside their own Windows installer
  (release #432), Pro Tools Intro on Windows loaded all of them. The site says
  "AAX on Windows. AAX for macOS is on the way."
- The iLok 3 (`iLok_322CC4`, iLok account `lirankalifa`) holds **PACE Central Access + PACE
  Tools + Eden Tools** (valid to 01.12.2027). The licences live on the dongle itself, so moving
  it to this Mac brings them along.
- PACE Central: product "RONE PLUGINS AAX", ONE wrap configuration "RONE AAX Signing" (signing
  only). Its **wcguid `D7FE9D20-E4D9-11F1-BA7B-005056928F3B` is used for every plugin** (see
  `scripts/aax/aax-products.json`).
- Apple: Developer ID Application + Installer certificates, Team ID `AN9FQN75QW`. CI already
  signs + notarizes every Mac pkg (`scripts/ci/sign-macos.sh`). The p12 files are in 1Password,
  vault RONE, item "Apple Code Signing" (attachments `apple-devid-app.p12`,
  `apple-devid-installer.p12`; the p12 password is in the same item).
- The Windows pipeline to copy: `scripts/aax/README.md`, `build-aax.ps1`, `sign-aax.ps1`,
  `publish-aax.ps1`, and the CI step "Fetch signed AAX (Pro Tools)" in
  `.github/workflows/main.yml` (Windows job). Read all of them first.

## The goal
Every product in `aax-products.json` (the Analyzer since 22.11: its Bridge and Channel rows) built as a **universal
(arm64 + x86_64) macOS .aaxplugin**, PACE-signed with the Developer ID Application identity,
loaded in Pro Tools on this Mac, and then delivered inside each plugin's Mac .pkg by CI, the
same way Windows works.

## Steps

0. **Read** `~/RONE/mac-aax-status.txt` (written by `bootstrap-mac.sh`) and finish whatever it
   marks TODO, together with Liran.

1. **iLok License Manager**: Liran signs in as `lirankalifa` with the iLok plugged in. Check the
   three PACE licences show on the iLok (not on "Cloud" or the computer).

2. **wraptool for macOS**: in iLok License Manager the **PACE CENTRAL** button opens a one-time
   token page in the browser. There: DEVELOPER > SDK Download > `PACECodeSigningForAAXSDK`
   (macOS). Install it. Find `wraptool` (usually under
   `/Applications/PACEAntiPiracy/Eden/Fusion/Versions/<n>/bin/`). Docs:
   https://docs.paceap.com/fusion-protection/getting-started/aax-dsig/
   (if you drive a browser: never close a tab you did not open; on the PC closing one tab once
   deleted Liran's whole tab group).

3. **Developer ID Application identity** into the login keychain: Liran opens the 1Password item
   and double-clicks `apple-devid-app.p12` (he types its password). Check with
   `security find-identity -v -p codesigning` -> "Developer ID Application: ... (AN9FQN75QW)".
   Only the Application identity is needed here; the Installer one stays a CI matter.

4. **Port the build**: write `scripts/aax/mac/build-aax-mac.sh`, the macOS twin of
   `build-aax.ps1`. Keep its rules:
   - build the **released** code: `git ls-tree HEAD <folder>` gives the submodule commit;
     `git archive` that commit (plus the monorepo's `Shared/` from the same commit) into
     `~/RONE/_build/aax/src`. Never build a plugin's working copy (they can be on DEV branches).
     The vendored `RONE AFTER SPACE` comes from the monorepo itself.
   - in the staging CMakeLists only: append `AAX` to the `juce_add_plugin(<target> ...)` FORMATS
     line, insert `AAX_CATEGORY <category>`; for RONEAnalyzer the line is
     `set(RONE_BRIDGE_FORMATS VST3)`.
   - Flanger vendors JUCE without `extras/Build`: copy it from a JUCE clone of the same version
     (8.0.12), like CI does.
   - JUCE: the tag from the plugin's `FetchContent_Declare(JUCE ... GIT_TAG ...)` (default
     8.0.4), cloned once into a cache, passed with `-DFETCHCONTENT_SOURCE_DIR_JUCE=`. JUCE 8
     already contains the AAX SDK.
   - configure like CI's Mac job: `-DCMAKE_BUILD_TYPE=Release
     -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15`
     plus `-DCMAKE_PROJECT_INCLUDE=<src>/Shared/RoneBuildHooks.cmake`; Ninja or Xcode generator,
     your choice. Build only `<target>_AAX`.
   - output: `~/RONE/_build/aax/unsigned/<bundle>.aaxplugin`, a log per product, a summary
     table, `build-info.json` (product, commit, version). Check `lipo -info` shows both archs.
   - Start with **RoneClipper** alone (free, small), get it all the way through steps 5-6, then
     run the rest.

5. **Sign**: `scripts/aax/mac/sign-aax-mac.sh`, twin of `sign-aax.ps1`:
   `wraptool sign --verbose --account lirankalifa --wcguid <wcguid>
   --signid "Developer ID Application: ... (AN9FQN75QW)" --in <unsigned> --out <signed>`, then
   `wraptool verify --verbose --in <signed>` and `codesign --verify --deep --strict`.
   The first sign asks for the iLok password - Liran types it in the Terminal himself (on the PC
   it was then stored in wraptool's keychain; check `wraptool help` for the Mac way).
   Check in PACE's docs whether hardened runtime + timestamp are needed for notarization
   (wraptool's `--extrasigningoptions`, e.g. `"--options runtime --timestamp"`); a notarized pkg
   needs every Mach-O inside it hardened and timestamped.
   Trap seen on the PC: `CouldNotFindSignerCredentials` = the iLok has no signing cert yet ->
   iLok License Manager, right-click the iLok > Synchronize (first try may say "unknown error";
   restart LM and retry).

6. **Pro Tools test**: install the signed bundles into
   `/Library/Application Support/Avid/Audio/Plug-Ins/` (sudo, Liran's password), open Pro Tools
   Intro (Avid Link, the same Avid account he used on the PC), check they are listed
   (stereo + mono) and open Clipper, Throw and Reverb with their full UI. Screenshot for Liran.
   Then stop and show him before any delivery work.

7. **Delivery through CI** (after his yes), mirroring Windows:
   - `scripts/aax/mac/publish-aax-mac.sh`: zip each signed bundle with `ditto -c -k --keepParent`
     (keeps the signature; plain zip can break it) as `<Id>.mac.aaxplugin.zip` and
     `gh release upload AAX-signed --clobber` (same pre-release as Windows; skip a bundle whose
     version differs from the exported CMakeLists, like publish-aax.ps1 does).
   - CI Mac job (`main.yml`, the step that fills `staging/Library/...` before `pkgbuild`): new
     step "Fetch signed AAX (Pro Tools, Mac)" downloads `<Id>.mac.aaxplugin.zip`, unzips with
     `ditto -x -k`, compares `CFBundleShortVersionString`/`CFBundleVersion` in its Info.plist
     with the build's CMake version, and copies it to
     `staging/Library/Application Support/Avid/Audio/Plug-Ins/`. Missing or stale = a
     `::warning::` and the pkg ships without AAX. **It must be copied AFTER
     `sign-macos.sh sign` and never be codesigned again by CI** - re-signing breaks the PACE
     signature. The pkg's productsign + notarization then cover it.
   - Plugins Center on Mac: `RonePluginsCenter/Source/DawDetector.h` has "protools" since
     Center 2.1.41 (Windows paths). Check the Mac side: Pro Tools detection, the AAX path in
     holderFiles / uninstall (`PluginUninstaller::removeFiles`).
   - Site: the "AAX for macOS is on the way" line in the WEBSITE RONE AUDIO repo (that repo is
     on the PC, not here) - leave a note in the Log; the PC session updates it.
   - Note: a plugin version bump ships WITHOUT AAX until someone re-runs build + sign + publish
     on the machine with the iLok. That is how Windows works too.

## Repo rules
- Only ONE Claude session works in the monorepo at a time. The PC session is stopped while
  this Mac works; `git pull --ff-only` before you start and before each commit.
- A plugin's own code: commit inside its submodule first (push to its repo), then the pointer
  bump in the monorepo. For this task you should not need to touch plugin code at all.
- Monorepo commits that must not trigger a release build carry `[skip ci]` in the subject.
  Scripts and this file = `[skip ci]`. The CI change in step 7 is pushed only on Liran's yes.
- Push every commit right after making it.
- Do not put passwords, addresses, phone numbers or e-mail addresses in this repo - it is public.

## Log
- 21.11 (PC): brief + `bootstrap-mac.sh` written. Mac work not started.
- 21.11 (Mac): `build-aax-mac.sh` written (Ninja + Command Line Tools, no Xcode needed).
  RoneClipper 1.1.4 (9fc500e) built universal x86_64+arm64 -> `~/RONE/_build/aax/unsigned`.
  Waiting on Liran: iLok on USB, LM sign-in, wraptool, Developer ID p12, Pro Tools Intro.
- 21.11 (PC session, driving the Mac over SSH via Tailscale `100.125.44.59`, key `~/.ssh/rone_mac_ed25519`):
  16 products built universal + PACE-signed + Developer ID (hardened runtime, timestamp) on the
  MacBook; all verify (codesign + wraptool). Zips `<Id>.mac.aaxplugin.zip` uploaded to the
  `AAX-signed` release (from the PC: gh on the Mac has no token over SSH). RONEAnalyzer: configure
  fails (not needed yet).
  TRAPS: wraptool on the Mac never prompts -> `store-pace-password-mac.sh` once; signing works only
  in Liran's own Terminal (over SSH the keychain is locked: errSecInternalComponent); SSH cannot
  read ~/Downloads (macOS privacy); Apple's Developer ID G2 intermediate had to be added to the
  login keychain; iLok shows in `system_profiler SPUSBHostDataType` on macOS 26.
  Center (6420d22, unreleased): a .pkg carrying an AAX installs the AAX only on a Mac with Pro
  Tools; no Pro Tools = no password. CI step "Fetch signed AAX (Pro Tools, Mac)" written in
  main.yml, NOT pushed (waits for: Pro Tools test on the Mac, a notarization dry run, Liran's yes,
  and the Center release first). Left on release day: NetworkManager.cpp's Mac AAX filter,
  DawDetector "protools" on Mac, site line "AAX for macOS is on the way".
- 22.11 (PC session): RONE Analyzer AAX - two rows (RONEAnalyzer = Bridge, RONEAnalyzerChannel = Channel), category
  SoundField. Mac build needed `-DJUCE_PATH` (the Analyzer prefers a local ~/JUCE, an old one here that the macOS 15
  SDK refuses) and its Mac formats line is `set(RONE_BRIDGE_FORMATS VST3 AU)`. Signed by Liran in his Terminal
  (`sign-aax-mac.sh "RONE Analyzer Bridge" "RONE Analyzer Channel"`), zipped with `DRY_RUN=1 publish-aax-mac.sh`,
  uploaded from the PC. CI packs every row of a folder.
