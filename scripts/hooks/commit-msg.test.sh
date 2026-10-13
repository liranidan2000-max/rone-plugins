#!/bin/sh
# Tests for scripts/hooks/commit-msg (run: sh scripts/hooks/commit-msg.test.sh).
# Feeds message files straight to the hook; no repo needed.
hook="$(dirname "$0")/commit-msg"
tmp="${TMPDIR:-/tmp}/commit-msg-test.$$"
fail=0

check() { # $1 = expected exit (0 allow / 1 refuse), $2 = name, message on stdin
  cat > "$tmp"
  sh "$hook" "$tmp" 2>/dev/null; rc=$?
  if [ "$rc" -eq "$1" ]; then echo "ok   $2"; else echo "FAIL $2 (exit $rc, want $1)"; fail=1; fi
}

printf 'ci: auto-update versions.json (build #9) [skip ci]\n' | check 0 "bot subject"
printf 'license: set mode to free [skip ci]\n' | check 0 "kill-switch subject"
printf 'ci: tweak [skip ci]\n\nplain body\n' | check 0 "token in subject, clean body"
printf 'center 2.1.3: fix\n\nthe skip-ci token is not a token\n' | check 0 "token described in words"
printf 'center 2.1.3: fix\n\nreverted 85faa90 ([skip ci]) by mistake\n' | check 1 "token in body"
printf 'center 2.1.3: fix\n\nsee [CI SKIP] earlier\n' | check 1 "variant, upper case"
printf 'x\n\n[no ci]\n' | check 1 "[no ci]"
printf 'x\n\n[skip actions]\n' | check 1 "[skip actions]"
printf 'x\n\n[actions skip]\n' | check 1 "[actions skip]"
printf 'x\n\nskip-checks: true\n' | check 1 "skip-checks trailer"
printf 'x\r\n\r\nbody [skip ci]\r\n' | check 1 "CRLF message"
printf '\n\nsubject [skip ci]\nbody\n' | check 0 "leading blank lines before subject"
printf 'x\n\nbody\n# Please enter... [skip ci]\n' | check 0 "token only in a comment line"
printf 'x\n\nbody\n# ------------------------ >8 ------------------------\n+ echo "[skip ci]"\n' | check 0 "token only in the -v diff"
printf '' | check 0 "empty message"

rm -f "$tmp"
[ "$fail" -eq 0 ] && echo "all passed"
exit "$fail"
