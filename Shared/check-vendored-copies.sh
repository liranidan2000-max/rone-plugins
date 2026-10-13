#!/usr/bin/env bash
# Shared headers that other repos carry must not drift from Shared (audit R117).
#   - RONEAnalyzer builds with byte copies in Source/Core: each one that exists
#     must equal Shared (line endings aside).
#   - RoneStutter keeps one-line shims in Source/: each one that exists must
#     include ../../Shared/<same name> instead of holding its own copy.
# A drifted copy ships an old licence chain (Analyzer 1.3.11-1.3.13 shipped
# without R182). Run from CI after the submodule is checked out:
#   bash Shared/check-vendored-copies.sh [--warn] [repo root]
# --warn prints ::warning:: lines and always exits 0 (the first release);
# without it a drift is an ::error:: and exit code 1.
set -u
mode=fail
if [ "${1:-}" = "--warn" ]; then mode=warn; shift; fi
root="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
bad=0
note() {
  if [ "$mode" = warn ]; then echo "::warning::$1"; else echo "::error::$1"; fi
  bad=1
}

core="$root/RONEAnalyzer/Source/Core"
for f in BundleLicenseChecker RemoteLicenseGate RoneEntitlement RoneUpdatePrompt RoneCrashReporter RoneWebCompat; do
  [ -f "$core/$f.h" ] || continue
  if ! cmp -s <(tr -d '\r' < "$root/Shared/$f.h") <(tr -d '\r' < "$core/$f.h"); then
    note "RONEAnalyzer/Source/Core/$f.h differs from Shared/$f.h - copy Shared/$f.h over it unchanged"
  fi
done

src="$root/RoneStutter/Source"
for f in BundleLicenseChecker RemoteLicenseGate RoneCrashReporter RoneExportFolder RoneAboutOverlay; do
  [ -f "$src/$f.h" ] || continue
  if ! grep -q "#include \"../../Shared/$f.h\"" "$src/$f.h"; then
    note "RoneStutter/Source/$f.h is a copy - make it a one-line shim: #include \"../../Shared/$f.h\""
  fi
done

if [ "$bad" = 0 ]; then echo "vendored copies match Shared"; fi
if [ "$mode" = warn ]; then exit 0; fi
exit "$bad"
