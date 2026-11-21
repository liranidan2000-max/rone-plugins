#!/bin/bash
# Uploads the PACE-signed macOS AAX bundles to the "AAX-signed" GitHub pre-release, where the
# CI Mac job's "Fetch signed AAX (Pro Tools, Mac)" step packs each one into that plugin's .pkg.
# The macOS twin of ../publish-aax.ps1 (Windows zips are <Id>.aaxplugin.zip, these are
# <Id>.mac.aaxplugin.zip - same release, no clash).
#
# A bundle is uploaded only when its version equals the exported (released) CMakeLists, so an
# AAX built from other code never reaches a customer. ditto keeps the code signature intact
# (a plain zip can drop the extended attributes / symlinks the signature covers).
#
#   scripts/aax/mac/publish-aax-mac.sh                 # every signed bundle
#   scripts/aax/mac/publish-aax-mac.sh RoneClipper     # by product id
#   DRY_RUN=1 scripts/aax/mac/publish-aax-mac.sh
set -uo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
CFG="$HERE/../aax-products.json"
OUT="${OUT:-$HOME/RONE/_build/aax}"
TAG="${TAG:-AAX-signed}"
REPO="${REPO:-liranidan2000-max/rone-plugins}"
DRY_RUN="${DRY_RUN:-0}"
SIGNED="$OUT/signed"; SRC="$OUT/src"; PUB="$OUT/publish-mac"
for b in /opt/homebrew/bin /usr/local/bin; do [ -d "$b" ] && PATH="$b:$PATH"; done
mkdir -p "$PUB"

IDS=$(jq -r '.products[] | select(.id != "RONEAnalyzer") | .id' "$CFG")
[ $# -gt 0 ] && IDS="$*"

zips=(); summary=""
for id in $IDS; do
  row=$(jq -c --arg i "$id" '.products[] | select(.id == $i)' "$CFG")
  [ -n "$row" ] || { summary+="$id  UNKNOWN id\n"; continue; }
  bundle=$(jq -r .bundle <<<"$row"); folder=$(jq -r .folder <<<"$row")
  b="$SIGNED/$bundle.aaxplugin"
  if [ ! -d "$b" ]; then summary+="$id  SKIPPED (not signed: sign-aax-mac.sh)\n"; continue; fi
  have=$(/usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" "$b/Contents/Info.plist" 2>/dev/null)
  want=$(grep -oE 'project\([^)]*VERSION +[0-9]+\.[0-9]+\.[0-9]+' "$SRC/$folder/CMakeLists.txt" 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+$' | head -1)
  if [ -n "$want" ] && [ "$have" != "$want" ]; then
    summary+="$id  SKIPPED (bundle $have, source $want: rebuild + re-sign)\n"; continue; fi
  if ! codesign --verify --deep --strict "$b" 2>/dev/null; then
    summary+="$id  SKIPPED (signature does not verify)\n"; continue; fi
  z="$PUB/$id.mac.aaxplugin.zip"; rm -f "$z"
  ditto -c -k --keepParent "$b" "$z"
  zips+=("$z"); summary+="$id  $have  ZIPPED\n"
done

if [ "$DRY_RUN" != "1" ] && [ ${#zips[@]} -gt 0 ]; then
  if ! gh release view "$TAG" --repo "$REPO" >/dev/null 2>&1; then
    gh release create "$TAG" --repo "$REPO" --prerelease --title "AAX signed bundles (CI input)" \
      --notes "PACE-signed AAX bundles for Pro Tools, one zip per plugin and OS. Uploaded from the iLok machine by scripts/aax; CI packs each into that plugin's installer when the versions match. Not a download for customers." \
      || { echo "could not create release $TAG" >&2; exit 1; }
  fi
  gh release upload "$TAG" --repo "$REPO" --clobber "${zips[@]}" || { echo "upload to $TAG failed" >&2; exit 1; }
  summary=$(printf "%b" "$summary" | sed 's/ZIPPED$/UPLOADED/'); summary+="\n"
fi

echo; printf "%b" "$summary"
[ "$DRY_RUN" = "1" ] && echo "Dry run: zips in $PUB, nothing uploaded."
exit 0
