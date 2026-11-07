#!/bin/sh
# Tests for scripts/hooks/pre-commit (run: sh scripts/hooks/pre-commit.test.sh).
# Builds a throwaway monorepo and a fake plugin repo (bare "remote" with main,
# release and rack-dev) under $TMPDIR, copies the hook to scripts/hooks there
# (core.hooksPath = scripts/hooks, as in the real monorepo) and commits through it.
# Nothing leaves the machine.
hook="$(cd "$(dirname "$0")" && pwd)/pre-commit"
tmp="${TMPDIR:-/tmp}/pre-commit-test.$$"
fail=0
unset RONE_ALLOW_DEV_POINTER

g() { git -c protocol.file.allow=always "$@"; }   # local-path submodules (git >= 2.38.1)
cfg() { git config user.name test; git config user.email test@example.invalid; git config commit.gpgsign false; git config core.autocrlf false; }

try() { # $1 = expected exit (0 committed / 1 refused), $2 = name; the commit output lands in $out
  out=$(git commit -q -m "$2" 2>&1); rc=$?
  if [ "$rc" -eq "$1" ]; then echo "ok   $2"; else echo "FAIL $2 (exit $rc, want $1)"; printf '%s\n' "$out" | sed 's/^/     /'; fail=1; fi
}
says() { # $1 = text the last output must contain, $2 = name
  case "$out" in *"$1"*) echo "ok   $2" ;; *) echo "FAIL $2 (output lacks '$1')"; printf '%s\n' "$out" | sed 's/^/     /'; fail=1 ;; esac
}
at() { # $1 = submodule path, $2 = ref on its origin: check that commit out, stage the pointer
  git -C "$1" fetch -q origin "$2" && git -C "$1" checkout -q FETCH_HEAD && git add "$1"
}

mkdir -p "$tmp" || exit 1
cd "$tmp" || exit 1

# --- the plugin repo: main A-M, release R off A, rack-dev D off A (not on main) ---
git init -q -b main plugin-src
( cd plugin-src && cfg
  echo 1 > f && git add f && git commit -qm "1.0.0"
  git checkout -qb release && echo r > f && git add f && git commit -qm "1.0.1 release"
  git checkout -q main && echo 2 > f && git add f && git commit -qm "1.0.2"
  git checkout -qb rack-dev main~1 && echo d > f && git add f && git commit -qm "1.1.0 rack"
  git checkout -q main )
git clone -q --bare plugin-src plugin.git
git -C plugin-src remote add origin "$tmp/plugin.git"

# --- the monorepo, with the real hook at scripts/hooks/pre-commit ---
git init -q -b main mono
cd mono || exit 1
cfg
git config core.hooksPath scripts/hooks
mkdir -p scripts/ci scripts/hooks
cp "$hook" scripts/hooks/pre-commit
printf '# folder<TAB>branch\n' > scripts/ci/release-lines.txt
git add scripts
try 0 "no gitlink staged: fast pass"

g submodule add -q "$tmp/plugin.git" Plugin
g submodule add -q "$tmp/plugin.git" "Plugin Two"
git -C Plugin config protocol.file.allow always
git -C "Plugin Two" config protocol.file.allow always
git add .gitmodules Plugin "Plugin Two"
try 0 "two new submodules at the tip of main"
says "Plugin Two -> " "names the folder with a space"

( cd "$tmp/plugin-src" && echo 3 > f && git add f && git commit -qm "1.0.3" && git push -q origin main )
at Plugin main
try 0 "pointer moved to a newer commit on main"

at Plugin rack-dev
try 1 "pointer on rack-dev is refused"
says "NOT on its release line 'main'" "refusal names the line"
says "RONE_ALLOW_DEV_POINTER=1" "refusal names the override"

out=$(RONE_ALLOW_DEV_POINTER=1 git commit -q -m "exception" 2>&1); rc=$?
if [ "$rc" -eq 0 ]; then echo "ok   RONE_ALLOW_DEV_POINTER=1 lets it through"; else echo "FAIL RONE_ALLOW_DEV_POINTER=1 lets it through (exit $rc)"; printf '%s\n' "$out" | sed 's/^/     /'; fail=1; fi
says "SKIPPED" "the exception says so"

at Plugin main
try 0 "back to main after the exception"

printf 'Plugin\trelease\n' >> scripts/ci/release-lines.txt
git add scripts/ci/release-lines.txt
at Plugin release
try 0 "line = release: pointer on release passes"

at Plugin main
try 1 "line = release: pointer on main is refused"
says "release line 'release'" "refusal names the release line"
git reset -q -- Plugin

at "Plugin Two" rack-dev
try 1 "folder with a space: rack-dev refused"
says "Plugin Two -> " "refusal names the folder with a space"
git reset -q -- "Plugin Two"

git -C "Plugin Two" remote set-url origin "$tmp/missing.git"
git -C "Plugin Two" checkout -q HEAD~1 && git add "Plugin Two"   # an older main commit: the pointer changes
try 1 "fetch failure fails closed"
says "could not fetch origin/main" "fail-closed names the fetch"
says "RONE_ALLOW_DEV_POINTER=1" "fail-closed names the override"
git -C "Plugin Two" remote set-url origin "$tmp/plugin.git"
git reset -q -- "Plugin Two"

git update-index --add --cacheinfo "160000,$(git -C "$tmp/plugin-src" rev-parse rack-dev),Ghost"
try 1 "uninitialized submodule fails closed"
says "not initialized here" "fail-closed names the missing checkout"
git reset -q -- Ghost

echo plain > note.txt && git add note.txt
try 0 "plain file after all that: fast pass"

cd "$tmp/.." && rm -rf "$tmp"
[ "$fail" -eq 0 ] && echo "all passed"
exit "$fail"
