# Version-regression guard for .github/workflows/main.yml (Release + Update versions.json).
# Sourced, not executed:   source scripts/ci/release-guard.sh
#
# A product is published only when BOTH hold:
#  (a) its folder in this run's commit ($GITHUB_SHA) is the folder on CURRENT origin/main
#      (submodule gitlink, or tree hash for in-repo folders). Otherwise this is an older
#      run finishing last, or a re-run of an old run - publishing it is a downgrade.
#  (b) the version being published (<base>.<run>) is strictly greater than the version
#      origin/main's versions.json carries for it (a pointer moved backwards, or the
#      same run published twice).
# A failed check prints ::error:: and returns 1 - the caller skips that product only.
#
# Release runs guard_check BEFORE it uploads anything. Update versions.json runs
# guard_manifest_check instead: by then the asset on <Tag>-latest is already
# this run's build, so the manifest must follow it whatever main looks like now
# (holding it back would pair the new installer with the old sha256). It skips
# only when the manifest already carries this version or newer, or a newer
# build has since replaced the asset.
#
# Needs: git (a checkout of this repo), jq, GITHUB_SHA.
# GUARD_FETCH_ARGS: extra args for the origin/main fetch ("--depth 1" on a fresh clone;
# empty on a clone that later rebases onto main, so its history stays connected).

guard_fetch_main() {
  # shellcheck disable=SC2086
  if ! git fetch --no-tags ${GUARD_FETCH_ARGS:-} origin "+refs/heads/main:refs/remotes/origin/main"; then
    echo "::error::release guard: could not fetch origin/main - nothing is published"
    return 1
  fi
  # GUARD_NO_RUN_COMMIT=1: the caller never reads this run's tree (guard_manifest_check).
  if [ "${GUARD_NO_RUN_COMMIT:-0}" != 1 ] && ! git cat-file -e "${GITHUB_SHA}^{commit}" 2>/dev/null; then
    if ! git fetch --no-tags --depth 1 origin "$GITHUB_SHA"; then
      echo "::error::release guard: could not fetch this run's commit $GITHUB_SHA - nothing is published"
      return 1
    fi
  fi
  if ! GUARD_LIVE_MANIFEST=$(git show origin/main:versions.json) \
     || ! jq -e . >/dev/null 2>&1 <<<"$GUARD_LIVE_MANIFEST"; then
    echo "::error::release guard: could not read versions.json on origin/main - nothing is published"
    return 1
  fi
  echo "release guard: run commit $GITHUB_SHA, origin/main $(git rev-parse origin/main)"
}

# $1 = manifest id (__center__ for the Center). Prints "" when the product is not listed yet.
# GUARD_PLATFORM=macos: a plugin's live Mac build - version_mac, written by the macOS
# pass since Windows goes out first (2026-10-26); an entry without it has one version
# for both. The Center always goes out on both platforms together: one version.
guard_live_version() {
  if [ "$1" = "__center__" ]; then
    jq -r '.center_installer.version // ""' <<<"$GUARD_LIVE_MANIFEST"
  elif [ "${GUARD_PLATFORM:-}" = macos ]; then
    jq -r --arg id "$1" '[.plugins[]? | select(.id == $id) | (.version_mac // .version)][0] // ""' <<<"$GUARD_LIVE_MANIFEST"
  else
    jq -r --arg id "$1" '[.plugins[]? | select(.id == $id) | .version][0] // ""' <<<"$GUARD_LIVE_MANIFEST"
  fi
}

# 0 when dotted version $1 > $2 (numeric per part, a missing part counts as 0).
guard_version_gt() {
  local re='^[0-9]+(\.[0-9]+)*$' i n x y
  [[ "$1" =~ $re && "$2" =~ $re ]] || return 1
  local -a a b
  IFS=. read -r -a a <<<"$1"
  IFS=. read -r -a b <<<"$2"
  n=${#a[@]}; [ ${#b[@]} -gt "$n" ] && n=${#b[@]}
  for ((i = 0; i < n; i++)); do
    x=$((10#${a[i]:-0})); y=$((10#${b[i]:-0}))
    [ "$x" -gt "$y" ] && return 0
    [ "$x" -lt "$y" ] && return 1
  done
  return 1
}

# guard_check <product id> <folder> <manifest id> <version to publish>
guard_check() {
  local id=$1 folder=$2 jid=$3 ver=$4 at_run at_main live
  at_run=$(git ls-tree "$GITHUB_SHA" -- "$folder") \
    || { echo "::error::$id: cannot read '$folder' at $GITHUB_SHA - not published"; return 1; }
  at_main=$(git ls-tree origin/main -- "$folder") \
    || { echo "::error::$id: cannot read '$folder' on origin/main - not published"; return 1; }
  if [ "$at_run" != "$at_main" ]; then
    echo "::error::$id: '$folder' in this run (${at_run%%$'\t'*}) is not the one on origin/main (${at_main%%$'\t'*}) - an older run or a re-run of one; not published (the newer run publishes it)"
    return 1
  fi
  live=$(guard_live_version "$jid")
  if [ -z "$live" ]; then
    echo "release guard: $id $ver - not in versions.json yet, ok"
    return 0
  fi
  if ! [[ "$ver" =~ ^[0-9]+(\.[0-9]+)+$ ]]; then
    echo "::error::$id: version '$ver' is not a version - not published"
    return 1
  fi
  if ! guard_version_gt "$ver" "$live"; then
    echo "::error::$id: $ver is not newer than the live $live - not published (a submodule pointer moved backwards, or this version is already out)"
    return 1
  fi
  echo "release guard: $id $ver > live $live, folder matches origin/main - ok"
}

# guard_manifest_check <product id> <manifest id> <version> <release tag> <run number> <repo>
# For Update versions.json, AFTER Release published this product. Returns
#   0 - write the entry (default: the manifest follows what Release uploaded)
#   1 - leave it: the live manifest is NEWER, or a newer build owns the asset (::error::)
#   2 - leave it: the live manifest already carries exactly this version (re-run)
# Needs gh (GH_TOKEN) for the release notes; reading them failing only warns.
guard_manifest_check() {
  local id=$1 jid=$2 ver=$3 tag=$4 run=$5 repo=$6 live body cur
  live=$(guard_live_version "$jid")
  if [ -n "$live" ]; then
    if [ "$ver" = "$live" ]; then
      echo "release guard: $id $ver is already the live version - manifest left as is"
      return 2
    fi
    if guard_version_gt "$live" "$ver"; then
      echo "::error::$id: versions.json on origin/main already carries $live, newer than this run's $ver - manifest left alone (the newer run owns it)"
      return 1
    fi
  fi
  if body=$(gh release view "$tag" -R "$repo" --json body -q .body 2>/dev/null); then
    cur=$(grep -oE 'Build #[0-9]+' <<<"$body" | head -1 | tr -dc '0-9' || true)
    if [ -n "$cur" ] && [ "$cur" -gt "$run" ]; then
      echo "::error::$id: $tag now carries build #$cur, newer than this run #$run - manifest left alone (that run writes it)"
      return 1
    fi
  else
    echo "::warning::$id: could not read the notes of $tag - writing the manifest for what this run uploaded"
  fi
  echo "release guard: $id $ver - manifest follows this run's upload to $tag"
  return 0
}
