#!/bin/bash
# Builds the Plugins Center .dmg (Center 2.0.6): the classic window - the app,
# an arrow, a link to Applications - instead of a folder that held only the
# app, which left people unsure how to install it (Liran on his Mac, 2026-10-07).
#
#   bash scripts/ci/make-center-dmg.sh "<path>/RONE Plugins Center.app" build-output/RONE_Plugins_Center.dmg
#
# The window art is installer/mac/dmg-background(@2x).png (make_dmg_background.py
# draws it; the icon points below match it). When installer/mac/How to install.pdf
# exists it goes into the window too. create-dmg (Homebrew) lays the window out
# through Finder; if that fails on a runner, a plain image with the Applications
# link is built instead, so a release never goes out without a .dmg.
#
# The Center's own self-update mounts this image and takes the one *.app at its
# root - the Applications link, .background and the PDF do not match that.

APP="$1"
OUT="$2"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MAC="$ROOT/installer/mac"
NAME="$(basename "$APP")"
WORK="$(mktemp -d)"
STAGE="$WORK/stage"
mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/" || exit 1
[ -f "$MAC/How to install.pdf" ] && cp "$MAC/How to install.pdf" "$STAGE/"
rm -f "$OUT"

# One background holding both resolutions, so Retina screens get the sharp one
tiffutil -cathidpicheck "$MAC/dmg-background.png" "$MAC/dmg-background@2x.png" -out "$WORK/background.tiff" \
  || cp "$MAC/dmg-background.png" "$WORK/background.tiff"

if command -v create-dmg >/dev/null 2>&1 || brew install create-dmg >/dev/null 2>&1; then
  set -- --volname "RONE Plugins Center" --background "$WORK/background.tiff" \
         --window-pos 200 120 --window-size 640 400 --icon-size 112 --text-size 13 \
         --icon "$NAME" 170 190 --hide-extension "$NAME" --app-drop-link 470 190
  [ -f "$STAGE/How to install.pdf" ] && set -- "$@" --icon "How to install.pdf" 320 290
  if create-dmg "$@" "$OUT" "$STAGE"; then
    echo "dmg: laid out by create-dmg"
    rm -rf "$WORK"
    exit 0
  fi
  echo "::warning::create-dmg failed - building a plain .dmg with an Applications link"
  rm -f "$OUT"
fi

ln -s /Applications "$STAGE/Applications"
hdiutil create -volname "RONE Plugins Center" -srcfolder "$STAGE" -ov -format UDZO "$OUT"
status=$?
rm -rf "$WORK"
exit $status
