# versions.json updates as a PLAN, written again on top of whatever main holds
# when the push goes out (audit R207). Sourced, not executed, by
# .github/workflows/main.yml (Update versions.json):
#   source scripts/ci/release-guard.sh; source scripts/ci/manifest-plan.sh
#
# The old push loop rebased the bot commit onto a moved main. A rebase conflict
# (a hand edit of license_message sits next to last_updated) killed the step
# under bash -e, after Release had already uploaded the installers. Now the
# step resets to the new main and writes this run's entries again: only the
# version / url / sha256 / size fields of the products this run published, so
# a hand edit anywhere else always survives. Never '-X theirs' - that would
# silently undo a hand-edited license_message.
#
# Plan file: one line per product, '|'-separated (no field can hold a '|'):
#   id|jid|ver|tag|url|sha|size|url_mac|sha_mac|size_mac
# An empty sha (url, size) means that platform was not built: its fields stay.
#
# MANIFEST_PLATFORM (windows | macos | unset): which pass of Update versions.json
# writes the plan. Windows goes out first and moves 'version'; macOS writes
# 'version_mac' and leaves 'version' alone. The Center always carries both
# platforms in one entry, whatever the pass.
#
# Needs: jq, git, curl; gh (GH_TOKEN) for guard_manifest_check and the digest
# check; release-guard.sh sourced first.

# plan_add <plan file> <id> <jid> <ver> <tag> <url> <sha> <size> <url_mac> <sha_mac> <size_mac>
plan_add() {
  local f=$1; shift
  if [ $# -ne 10 ]; then echo "::error::plan_add: 10 fields wanted, got $#"; return 1; fi
  local IFS='|'
  printf '%s\n' "$*" >> "$f"
}

# One jq program over a file, in place. Fails without touching the file.
_plan_jq() {
  local f=$1; shift
  jq "$@" "$f" > "$f.tmp" && mv "$f.tmp" "$f"
}

# plan_apply_entry <versions.json> <the 10 plan fields>
# The same writes the workflow made inline before, on a copy that replaces the
# file only when every one of them worked. The Center's url_mac now goes with
# sha256_mac (the .dmg), since each platform has its own versioned file.
plan_apply_entry() {
  local m=$1 id=$2 jid=$3 ver=$4 tag=$5 url=$6 sha=$7 size=$8 url_mac=$9 sha_mac=${10} size_mac=${11}
  local w="$m.plan"
  cp "$m" "$w" || return 1
  if [ "$jid" = "__center__" ]; then
    _plan_jq "$w" --arg v "$ver" '.center_installer.version = $v' || return 1
    if [ -n "$sha" ]; then
      _plan_jq "$w" --arg u "$url" --arg sw "$sha" \
        '.center_installer.url = $u | .center_installer.sha256 = $sw' || return 1
    fi
    if [ -n "$sha_mac" ]; then
      _plan_jq "$w" --arg um "$url_mac" --arg sm "$sha_mac" \
        '.center_installer.url_mac = $um | .center_installer.sha256_mac = $sm' || return 1
    fi
  else
    if [ "${MANIFEST_PLATFORM:-}" = macos ]; then
      # Windows goes out first (2026-10-26): the Mac build has its own version_mac,
      # which Centers from 2.1.11 read on a Mac; 'version' stays the Windows one.
      _plan_jq "$w" --arg id "$jid" --arg v "$ver" \
        '(.plugins[]|select(.id==$id)).version_mac = $v' || return 1
    else
      if [ "${MANIFEST_PLATFORM:-}" = windows ]; then
        # Before 'version' moves on, the Mac build it described keeps its number.
        _plan_jq "$w" --arg id "$jid" \
          '(.plugins[]|select(.id==$id)) |= (if .version_mac == null and (.sha256_mac // "") != "" then .version_mac = .version else . end)' || return 1
      fi
      _plan_jq "$w" --arg id "$jid" --arg v "$ver" \
        '(.plugins[]|select(.id==$id)).version = $v' || return 1
    fi
    if [ -n "$sha" ]; then
      _plan_jq "$w" --arg id "$jid" --arg u "$url" --arg sw "$sha" \
        '(.plugins[]|select(.id==$id)).download_url = $u | (.plugins[]|select(.id==$id)).sha256 = $sw' || return 1
      _plan_jq "$w" --arg id "$jid" --argjson sz "${size:-0}" \
        '(.plugins[]|select(.id==$id)).size = $sz' || return 1
    fi
    if [ -n "$sha_mac" ]; then
      _plan_jq "$w" --arg id "$jid" --arg um "$url_mac" --arg sm "$sha_mac" \
        '(.plugins[]|select(.id==$id)).download_url_mac = $um | (.plugins[]|select(.id==$id)).sha256_mac = $sm' || return 1
      _plan_jq "$w" --arg id "$jid" --argjson sz "${size_mac:-0}" \
        '(.plugins[]|select(.id==$id)).size_mac = $sz' || return 1
    fi
  fi
  mv "$w" "$m"
}

# plan_apply_guarded <plan file> <versions.json> <run> <repo>
# Applies every entry the release guard still allows against GUARD_LIVE_MANIFEST
# (set it to the file's content first). rc 2 (already live) is skipped quietly;
# rc 1 (a newer version or build owns it) and a failed write are added to
# PLAN_SKIPPED, which the Verdict turns red.
plan_apply_guarded() {
  local plan=$1 m=$2 run=$3 repo=$4 line rc
  local id jid ver tag url sha size url_mac sha_mac size_mac
  while IFS= read -r line || [ -n "$line" ]; do
    [ -n "$line" ] || continue
    IFS='|' read -r id jid ver tag url sha size url_mac sha_mac size_mac <<<"$line"
    rc=0; guard_manifest_check "$id" "$jid" "$ver" "$tag" "$run" "$repo" || rc=$?
    if [ "$rc" -eq 2 ]; then continue; fi
    if [ "$rc" -ne 0 ]; then PLAN_SKIPPED="$PLAN_SKIPPED $id"; continue; fi
    if ! plan_apply_entry "$m" "$id" "$jid" "$ver" "$tag" "$url" "$sha" "$size" "$url_mac" "$sha_mac" "$size_mac"; then
      echo "::error::$id: could not write its entry into versions.json - left alone"
      rm -f "$m.plan" "$m.plan.tmp"
      PLAN_SKIPPED="$PLAN_SKIPPED $id"
    fi
  done < "$plan"
}

# plan_commit_push <plan file> <run> <repo> <commit message>
# Up to 3 attempts: fresh origin/main, reset onto it, write the entries, commit,
# push. Returns 0 when pushed (or main already had everything), 1 after 3 fails.
# Needs the checked-out branch to be main and user.name/email configured.
plan_commit_push() {
  local plan=$1 run=$2 repo=$3 msg=$4 attempt
  for attempt in 1 2 3; do
    [ "$attempt" -gt 1 ] && sleep 10
    git rebase --abort >/dev/null 2>&1 || true
    if ! git fetch --no-tags origin "+refs/heads/main:refs/remotes/origin/main"; then
      echo "::warning::could not fetch origin/main (attempt $attempt)"
      continue
    fi
    # artifacts/ and buildinfo/ are untracked and survive the reset
    if ! git reset -q --hard origin/main; then
      echo "::warning::could not reset onto origin/main (attempt $attempt)"
      continue
    fi
    GUARD_LIVE_MANIFEST=$(cat versions.json)
    PLAN_SKIPPED=''
    plan_apply_guarded "$plan" versions.json "$run" "$repo"
    if git diff --quiet -- versions.json; then
      echo "versions.json on main already carries this run's entries - nothing to commit"
      return 0
    fi
    # Timestamp only when an entry really changed.
    _plan_jq versions.json --arg t "$(date -u +%Y-%m-%dT%H:%M:%SZ)" '.last_updated=$t' || return 1
    git add versions.json && git commit -q -m "$msg" || return 1
    if git push origin HEAD:main; then
      return 0
    fi
    echo "push rejected (attempt $attempt) - writing the entries again on the new main"
  done
  return 1
}

# plan_url_live <url>: the published file answers (one byte, following GitHub's
# redirect), with retries for a transient error. A manifest URL that 404s is
# worse than a stale one: a 404 is not a hash mismatch, so no Center retries it.
plan_url_live() {
  curl -sfL -r 0-0 --retry 4 --retry-delay 5 --retry-all-errors \
       --connect-timeout 20 --max-time 120 -o /dev/null "$1"
}

# plan_asset_digest_ok <repo> <tag> <asset name> <sha256>
# GitHub's own sha256 of the uploaded asset (the 'digest' field) must equal the
# hash going into the manifest. Assets without a digest, or an API error, pass
# with a warning - the download check above already ran.
plan_asset_digest_ok() {
  local repo=$1 tag=$2 name=$3 sha=$4 listing d
  if ! listing=$(gh api "repos/$repo/releases/tags/$tag" --jq '.assets[] | .name + " " + (.digest // "null")' 2>/dev/null); then
    echo "::warning::could not read the assets of $tag - digest of $name not checked"
    return 0
  fi
  d=$(awk -v n="$name" '$1 == n { print $2; exit }' <<<"$listing")
  case "$d" in
    "sha256:$sha") return 0 ;;
    ""|null) echo "::warning::$tag/$name has no digest on GitHub - not checked"; return 0 ;;
    *) echo "::error::$tag/$name: GitHub has $d, this run hashed sha256:$sha"; return 1 ;;
  esac
}
