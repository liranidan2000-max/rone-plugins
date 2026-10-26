#!/usr/bin/env bash
# ============================================================================
# macOS code signing + notarization for the RONE CI (GitHub Actions).
#
#   bash scripts/ci/sign-macos.sh import            # .p12 -> temp keychain, exports MAC_APP_ID / MAC_INST_ID
#   bash scripts/ci/sign-macos.sh sign <folder>     # codesign every .vst3 / .component / .app under <folder>
#   bash scripts/ci/sign-macos.sh pkg  <file.pkg>   # productsign + notarize + staple (in place)
#   bash scripts/ci/sign-macos.sh dmg  <file.dmg>   # codesign + notarize + staple (in place)
#   bash scripts/ci/sign-macos.sh cleanup           # delete the temp keychain and the notary key
#
# Secrets (GitHub -> job env):
#   MAC_CERT_P12_BASE64      .p12 holding "Developer ID Application" + its key
#   MAC_INST_P12_BASE64      optional second .p12: "Developer ID Installer" + its key (Apple issues each
#                            Developer ID certificate against its own key pair - a CSR's public key cannot be
#                            reused, 2026-10-25), same password
#   MAC_CERT_PASSWORD        the password of both
#   APPLE_API_KEY_ID         App Store Connect API key (for notarytool)
#   APPLE_API_ISSUER_ID
#   APPLE_API_KEY_P8_BASE64  the .p8 file, base64
#
# Without MAC_CERT_P12_BASE64 every command is a no-op that prints a notice,
# so unsigned builds keep flowing until the secrets exist. docs/CODE-SIGNING.md
# ============================================================================
set -euo pipefail

TMP="${RUNNER_TEMP:-/tmp}"
KEYCHAIN="$TMP/rone-sign.keychain-db"
NOTARY_KEY="$TMP/rone-AuthKey.p8"
ENTITLEMENTS="$(cd "$(dirname "$0")" && pwd)/mac-app.entitlements"

cmd="${1:-}"; shift || true

notice() { echo "::notice::$*"; }
warn()   { echo "::warning::$*"; }
die()    { echo "::error::$*"; exit 1; }

notarize() {   # <file>  - uploads, waits, staples; only Apple's REJECTION fails the build
    # 26.10 (run #323): Apple's queue for a new developer account ran past 45 min (Throw, Sidechain: "Timeout of 2700
    # second(s) was reached") and once the status call itself timed out (Iron: NSURLErrorDomain -1001) - each killed
    # its leg, so no .pkg shipped at all. Waiting is not a verdict: on a timeout or a network error the signed .pkg
    # ships unstapled (Gatekeeper fetches the ticket online once Apple accepts it; the Center's installer run does not
    # need it), and only "Invalid"/"Rejected" fails the leg.
    local f="$1" name; name=$(basename "$1")
    if [ -z "${APPLE_API_KEY_ID:-}" ] || [ -z "${APPLE_API_ISSUER_ID:-}" ] || [ ! -f "$NOTARY_KEY" ]; then
        warn "$name is signed but NOT notarized (APPLE_API_KEY_ID / APPLE_API_ISSUER_ID / APPLE_API_KEY_P8_BASE64 missing). Gatekeeper will block it."
        return 0
    fi
    local auth=(--key "$NOTARY_KEY" --key-id "$APPLE_API_KEY_ID" --issuer "$APPLE_API_ISSUER_ID")
    local sub="$TMP/notary-submit-$name.json" out="$TMP/notary-$name.json" id="" status="" i
    jget() { python3 -c "import json,sys
try: print(json.load(open(sys.argv[1])).get(sys.argv[2],''))
except Exception: print('')" "$1" "$2"; }

    echo "Notarizing $name ..."
    for i in 1 2 3; do                                    # 1. the upload (a network error retries)
        if xcrun notarytool submit "$f" "${auth[@]}" --output-format json > "$sub" 2>&1; then id=$(jget "$sub" id); fi
        [ -n "$id" ] && break
        warn "notarytool submit try $i for $name failed: $(head -c 400 "$sub" 2>/dev/null)"
        sleep $((20 * i))
    done
    if [ -z "$id" ]; then
        warn "$name could not be uploaded to Apple's notary service - it ships signed, NOT notarized."
        return 0
    fi
    echo "notary submission $id"

    local deadline=$((SECONDS + 40 * 60))                 # 2. the wait, in 10-minute calls (one may time out)
    while :; do
        if xcrun notarytool wait "$id" "${auth[@]}" --timeout 10m --output-format json > "$out" 2>&1; then
            status=$(jget "$out" status)
        else
            status=$(jget "$out" status); warn "notarytool wait for $name: $(head -c 300 "$out" 2>/dev/null)"
        fi
        case "$status" in Accepted|Invalid|Rejected) break ;; esac
        [ "$SECONDS" -ge "$deadline" ] && break
        sleep 15
    done
    echo "notarization: ${status:-no answer} ($id)"

    case "$status" in
        Accepted)
            xcrun stapler staple "$f"
            xcrun stapler validate "$f" ;;
        Invalid|Rejected)
            xcrun notarytool log "$id" "${auth[@]}" || true
            die "Apple rejected $name - see the notarization log above" ;;
        *)
            warn "$name: Apple has not finished notarizing yet (submission $id, status '${status:-no answer}') - it ships signed and unstapled; check later with: xcrun notarytool info $id" ;;
    esac
}

case "$cmd" in
  import)
    if [ -z "${MAC_CERT_P12_BASE64:-}" ]; then
        notice "No macOS code-signing secrets configured - this build is UNSIGNED."
        exit 0
    fi
    P12="$TMP/rone-cert.p12"
    echo "$MAC_CERT_P12_BASE64" | base64 --decode > "$P12"
    KC_PASS=$(uuidgen)
    security create-keychain -p "$KC_PASS" "$KEYCHAIN"
    security set-keychain-settings -lut 21600 "$KEYCHAIN"
    security unlock-keychain -p "$KC_PASS" "$KEYCHAIN"
    # Apple's Developer ID intermediate, so the chain verifies on a bare runner.
    curl -fsSL -o "$TMP/DeveloperIDG2CA.cer" https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer \
        && security import "$TMP/DeveloperIDG2CA.cer" -k "$KEYCHAIN" -T /usr/bin/codesign -T /usr/bin/productsign >/dev/null 2>&1 || true
    security import "$P12" -k "$KEYCHAIN" -P "${MAC_CERT_PASSWORD:-}" -T /usr/bin/codesign -T /usr/bin/productsign -T /usr/bin/security
    if [ -n "${MAC_INST_P12_BASE64:-}" ]; then
        P12I="$TMP/rone-inst.p12"
        echo "$MAC_INST_P12_BASE64" | base64 --decode > "$P12I"
        security import "$P12I" -k "$KEYCHAIN" -P "${MAC_CERT_PASSWORD:-}" -T /usr/bin/codesign -T /usr/bin/productsign -T /usr/bin/security
        rm -f "$P12I"
    fi
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KC_PASS" "$KEYCHAIN" >/dev/null
    # shellcheck disable=SC2046
    security list-keychains -d user -s "$KEYCHAIN" $(security list-keychains -d user | tr -d '"')
    rm -f "$P12"

    APP_ID=$(security find-identity -v -p codesigning "$KEYCHAIN" | grep "Developer ID Application" | head -1 | sed -E 's/.*"(.*)".*/\1/' || true)
    INST_ID=$(security find-identity -v "$KEYCHAIN" | grep "Developer ID Installer" | head -1 | sed -E 's/.*"(.*)".*/\1/' || true)
    [ -n "$APP_ID" ] || die "The .p12 holds no 'Developer ID Application' identity (security find-identity found nothing)."
    [ -n "$INST_ID" ] || warn "No 'Developer ID Installer' identity in the .p12 - .pkg installers will stay unsigned."
    echo "Developer ID Application: $APP_ID"
    echo "Developer ID Installer:   ${INST_ID:-<none>}"
    { echo "MAC_APP_ID=$APP_ID"; echo "MAC_INST_ID=$INST_ID"; } >> "${GITHUB_ENV:-/dev/null}"

    if [ -n "${APPLE_API_KEY_P8_BASE64:-}" ]; then
        echo "$APPLE_API_KEY_P8_BASE64" | base64 --decode > "$NOTARY_KEY"
        chmod 600 "$NOTARY_KEY"
    else
        warn "APPLE_API_KEY_P8_BASE64 not set - builds will be signed but not notarized."
    fi
    ;;

  sign)
    [ -n "${MAC_APP_ID:-}" ] || { echo "unsigned build - nothing to sign"; exit 0; }
    root="${1:?folder}"
    n=0
    while IFS= read -r -d '' b; do
        case "$b" in
          *.app) codesign --force --options runtime --timestamp --entitlements "$ENTITLEMENTS" --sign "$MAC_APP_ID" "$b" ;;
          *)     codesign --force --options runtime --timestamp --sign "$MAC_APP_ID" "$b" ;;
        esac
        codesign --verify --deep --strict --verbose=2 "$b"
        echo "signed: $b"; n=$((n + 1))
    done < <(find "$root" \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0)
    [ "$n" -gt 0 ] || warn "No bundles found under $root"
    ;;

  pkg)
    f="${1:?file.pkg}"
    [ -n "${MAC_APP_ID:-}" ] || { echo "unsigned build - pkg left as is"; exit 0; }
    [ -n "${MAC_INST_ID:-}" ] || { warn "No Developer ID Installer identity - $(basename "$f") stays unsigned."; exit 0; }
    productsign --sign "$MAC_INST_ID" --timestamp "$f" "$f.signed"
    mv -f "$f.signed" "$f"
    pkgutil --check-signature "$f"
    notarize "$f"
    ;;

  dmg)
    f="${1:?file.dmg}"
    [ -n "${MAC_APP_ID:-}" ] || { echo "unsigned build - dmg left as is"; exit 0; }
    codesign --force --timestamp --sign "$MAC_APP_ID" "$f"
    codesign --verify --verbose=2 "$f"
    notarize "$f"
    ;;

  cleanup)
    security delete-keychain "$KEYCHAIN" 2>/dev/null || true
    rm -f "$NOTARY_KEY"
    ;;

  *)
    echo "usage: $0 import | sign <folder> | pkg <file> | dmg <file> | cleanup" >&2
    exit 2
    ;;
esac
