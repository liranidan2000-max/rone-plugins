#!/bin/bash
# One-time: stores the iLok account password in wraptool's keychain, so sign-aax-mac.sh can
# sign without asking. The macOS twin of ../store-pace-password.ps1. Run it in a Terminal YOU
# type in: the password is read hidden, handed straight to "wraptool sync", never written to
# disk or printed. To forget it later: wraptool remove-pswd --account lirankalifa
ACCOUNT="${ACCOUNT:-lirankalifa}"
WT="${WRAPTOOL:-$(ls /Applications/PACEAntiPiracy/Eden/Fusion/Versions/*/bin/wraptool 2>/dev/null | tail -1)}"
[ -x "$WT" ] || { echo "wraptool not found - install PACECodeSigningForAAXSDK (macOS) first" >&2; exit 2; }
echo ""
echo "iLok account: $ACCOUNT  (the iLok USB must be plugged in)"
printf "iLok password (hidden): "
IFS= read -rs PW </dev/tty; echo
"$WT" sync --account "$ACCOUNT" --password "$PW"
code=$?
PW=""; unset PW
echo ""
if [ $code -eq 0 ]; then
  echo "OK - the password is stored in wraptool's keychain. Next: scripts/aax/mac/sign-aax-mac.sh"
else
  echo "wraptool sync failed (exit $code). Check the password and that the iLok is plugged in, then run this again."
fi
exit $code
