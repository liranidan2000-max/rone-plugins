#!/bin/bash
# PACE-signs the macOS AAX bundles built by build-aax-mac.sh. The macOS twin of ../sign-aax.ps1.
#   unsigned: ~/RONE/_build/aax/unsigned/<bundle>.aaxplugin
#   signed:   ~/RONE/_build/aax/signed/<bundle>.aaxplugin
# Needs: the iLok (with the PACE licences) on USB, wraptool (PACECodeSigningForAAXSDK for macOS),
# and the "Developer ID Application (AN9FQN75QW)" identity in the login keychain.
# The first sign asks for the iLok account password: run it in Terminal yourself and type it.
#
#   scripts/aax/mac/sign-aax-mac.sh                    # every unsigned bundle
#   scripts/aax/mac/sign-aax-mac.sh "RONE Clipper"     # by bundle name
set -uo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
CFG="$HERE/../aax-products.json"
OUT="${OUT:-$HOME/RONE/_build/aax}"
ACCOUNT="${ACCOUNT:-lirankalifa}"
TEAM_ID="AN9FQN75QW"
UNSIGNED="$OUT/unsigned"; SIGNED="$OUT/signed"; LOGS="$OUT/logs"
mkdir -p "$SIGNED" "$LOGS"
command -v jq >/dev/null || { for b in /opt/homebrew/bin /usr/local/bin; do [ -x "$b/jq" ] && PATH="$b:$PATH"; done; }

WT="${WRAPTOOL:-}"
if [ -z "$WT" ]; then
  for c in /Applications/PACEAntiPiracy/Eden/Fusion/Versions/*/bin/wraptool /usr/local/bin/wraptool "$HOME"/RONE/*/wraptool "$HOME"/RONE/*/*/wraptool "$HOME"/RONE/*/*/*/wraptool; do
    [ -x "$c" ] && WT="$c"
  done
fi
[ -n "$WT" ] || { echo "wraptool not found - install PACECodeSigningForAAXSDK (macOS) or set WRAPTOOL=" >&2; exit 2; }

IDENT=$(security find-identity -v -p codesigning | sed -n "s/.*\"\(Developer ID Application: .*($TEAM_ID)\)\".*/\1/p" | head -1)
[ -n "$IDENT" ] || { echo "no valid 'Developer ID Application ($TEAM_ID)' in the keychain" >&2; exit 2; }
echo "wraptool: $WT"
echo "identity: $IDENT"

if [ $# -gt 0 ]; then BUNDLES=("$@"); else
  BUNDLES=(); while IFS= read -r b; do BUNDLES+=("$b"); done < <(cd "$UNSIGNED" && ls -d *.aaxplugin | sed 's/\.aaxplugin$//')
fi

ok=0; failed=0; summary=""
for name in "${BUNDLES[@]}"; do
  in="$UNSIGNED/$name.aaxplugin"; out="$SIGNED/$name.aaxplugin"; log="$LOGS/sign-$name.log"
  guid=$(jq -r --arg b "$name" '.products[] | select(.bundle == $b) | .wcguid' "$CFG")
  if [ ! -d "$in" ] || [ -z "$guid" ] || [ "$guid" = "null" ]; then
    summary+="$name  SKIPPED (no bundle or no wcguid)\n"; failed=$((failed+1)); continue; fi
  rm -rf "$out"
  echo ""; echo "=== $name ==="
  # Hardened runtime + secure timestamp: the pkg that carries the bundle is notarized, and
  # notarization rejects any Mach-O without them.
  if "$WT" sign --verbose --account "$ACCOUNT" --wcguid "$guid" --signid "$IDENT" \
        --extrasigningoptions "--options runtime --timestamp" \
        --in "$in" --out "$out" 2>&1 | tee "$log" \
     && "$WT" verify --verbose --in "$out" >>"$log" 2>&1 \
     && codesign --verify --deep --strict "$out" >>"$log" 2>&1; then
    ver=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$out/Contents/Info.plist" 2>/dev/null)
    summary+="$name  $ver  SIGNED\n"; ok=$((ok+1))
  else
    summary+="$name  FAILED (see $log)\n"; failed=$((failed+1))
  fi
done

echo ""; printf "%b" "$summary"
echo "Signed: $ok   Failed/skipped: $failed   ->  $SIGNED"
[ $failed -eq 0 ]
