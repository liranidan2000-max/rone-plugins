# Release, one platform: publishes this run's installers of PLATFORM on each
# product's <Tag>-latest release. Sourced by the Release (Windows) and Release
# (macOS) jobs of .github/workflows/main.yml, which set:
#   PLATFORM  windows | macos      REPO  owner/repo
#   RUN       github.run_number    ATTEMPT  github.run_attempt
#   changed   detect-changes' JSON list of product ids
# Windows goes out first (Liran, 2026-10-26: "Windows released first, before
# the Mac"): its job needs only the Windows builds, so a slow Apple notarization
# no longer holds the Windows installers back.
# Writes published / guard_skipped / upload_failed / alias_failed / suffix to
# GITHUB_OUTPUT. Needs gh (GH_TOKEN), jq, git; artifacts/ and buildinfo/ hold
# the downloaded installer-* and buildinfo-* artefacts.

published='[]'; guard_skipped=''; upload_failed=''; alias_failed=''

# Every installer also goes up under a name of its own (audit R028):
# <stem>-<run>.<attempt>.<ext>, e.g. RoneStutter_Installer-272.1.exe.
# Nothing ever replaces the bytes behind such a name, so a manifest
# that points at it can never pair a new file with an old sha256. A
# re-run of this job is a new attempt, so it never meets its own names.
SUFFIX="$RUN.$ATTEMPT"
echo "suffix=$SUFFIX" >> $GITHUB_OUTPUT
VDIR="${RUNNER_TEMP:-/tmp}/versioned"
versioned_name() { local n=$1; printf '%s-%s.%s' "${n%.*}" "$SUFFIX" "${n##*.}"; }

# Version-regression guard (scripts/ci/release-guard.sh). Cannot read
# origin/main -> fail here, before anything is uploaded.
GUARD_FETCH_ARGS="--depth 1"
GUARD_PLATFORM=$PLATFORM
source scripts/ci/release-guard.sh
guard_fetch_main || exit 1

# One file at a time, three tries each. --clobber deletes the old asset
# before the upload; when GitHub answers 500 to that delete (a stuck
# asset, #254 on 2026-10-06) the old one is renamed aside - GitHub
# accepts that - and the upload goes again. Never several files in one
# call: #254's second try deleted the .exe, failed on the .dmg and
# uploaded neither, so the Center could not be downloaded at all.
# On a versioned name the only thing --clobber can meet is this same
# attempt's own half-finished try, which nothing points at yet.
#
# The Center's alias (audit R090 part 2): the site's /download/center/*
# and older Centers fetch it by name, so that name must never 404 - and
# --clobber deletes the old file before the new one lands. On the
# Center tag an alias that is already there is swapped instead: the new
# file goes up as tmp-<run>-<name>, the old asset is renamed
# stale-<id>-<name> (the next run's stale- cleanup deletes it), then the
# tmp asset takes the name. Every other upload is unchanged.
asset_id() {
  gh api "repos/$REPO/releases/tags/$1" --jq ".assets[] | select(.name == \"$2\") | .id" 2>/dev/null | head -1 || true
}
swap_one() {
  local tag=$1 f=$2 name tmp tdir oid tid
  name=$(basename "$f"); tmp="tmp-$RUN-$name"
  tdir="${RUNNER_TEMP:-/tmp}/swap"
  mkdir -p "$tdir" && cp "$f" "$tdir/$tmp" || return 1
  if ! gh release upload "$tag" "$tdir/$tmp" --clobber -R "$REPO"; then
    # a half-finished tmp goes aside, so the next try never meets it
    tid=$(asset_id "$tag" "$tmp")
    if [ -n "$tid" ]; then gh api -X PATCH "repos/$REPO/releases/assets/$tid" -f name="stale-$tid-$tmp" >/dev/null 2>&1 || true; fi
    return 1
  fi
  tid=$(asset_id "$tag" "$tmp")
  if [ -z "$tid" ]; then return 1; fi
  oid=$(asset_id "$tag" "$name")
  if [ -n "$oid" ] && ! gh api -X PATCH "repos/$REPO/releases/assets/$oid" -f name="stale-$oid-$name" >/dev/null; then
    gh api -X PATCH "repos/$REPO/releases/assets/$tid" -f name="stale-$tid-$tmp" >/dev/null 2>&1 || true
    return 1
  fi
  if ! gh api -X PATCH "repos/$REPO/releases/assets/$tid" -f name="$name" >/dev/null; then
    # the previous file takes its name back
    if [ -n "$oid" ]; then gh api -X PATCH "repos/$REPO/releases/assets/$oid" -f name="$name" >/dev/null 2>&1 || true; fi
    gh api -X PATCH "repos/$REPO/releases/assets/$tid" -f name="stale-$tid-$tmp" >/dev/null 2>&1 || true
    return 1
  fi
  echo "swapped $name on $tag (the previous file is stale-$oid-$name until the next run)"
}
upload_one() {
  local tag=$1 f=$2 name i sid
  name=$(basename "$f")
  if [ "$tag" = "PluginsCenter-latest" ] && [ -n "$(asset_id "$tag" "$name")" ]; then
    for i in 1 2 3; do
      if swap_one "$tag" "$f"; then return 0; fi
      echo "::warning::swap of $name on $tag failed (try $i)"
      sleep $((i * 15))
    done
    echo "::error::$name could not be replaced on $tag - the previous file keeps the name"
    return 1
  fi
  for i in 1 2 3; do
    if gh release upload "$tag" "$f" --clobber -R "$REPO"; then return 0; fi
    echo "::warning::upload of $name to $tag failed (try $i)"
    sid=$(gh api "repos/$REPO/releases/tags/$tag" --jq ".assets[] | select(.name == \"$name\") | .id" 2>/dev/null || true)
    if [ -n "$sid" ]; then
      gh api -X PATCH "repos/$REPO/releases/assets/$sid" -f name="stale-$sid-$name" >/dev/null \
        && echo "moved a stuck $name aside (asset $sid)"
    fi
    sleep $((i * 15))
  done
  echo "::error::$name could not be uploaded to $tag"
  return 1
}

for id in $(echo "$changed" | jq -r '.[]'); do
  case "$id" in
    reverse_reverb) tag=ReverseReverb-latest;    title="RONE Reverse Reverb (Latest)";  folder=ReverseReverbVST;   jid=ReverseReverb ;;
    stems_fixer)    tag=RoneStemsFixer-latest;    title="RONE Stems Fixer (Latest)";    folder=RoneStemsFixer;     jid=RoneStemsFixer ;;
    stutter)        tag=RoneStutter-latest;       title="RONE Stutter (Latest)";        folder=RoneStutter;        jid=RoneStutter ;;
    stucker)        tag=RoneStucker-latest;       title="RONE Stucker (Latest)";        folder=RoneStucker;        jid=RoneStucker ;;
    throw)          tag=RoneThrow-latest;         title="RONE Throw (Latest)";          folder=RoneThrow;          jid=RoneThrow ;;
    clipper)        tag=RoneClipper-latest;       title="RONE Clipper (Latest)";        folder=RoneClipper;        jid=RoneClipper ;;
    rise)           tag=RoneRise-latest;          title="RONE Rise (Latest)";           folder=RoneRise;           jid=RoneRise ;;
    iron)           tag=RoneIron-latest;          title="RONE Iron (Latest)";           folder=RoneIron;           jid=RoneIron ;;
    rone1176)       tag=Rone1176-latest;          title="RONE 1176 (Latest)";           folder=Rone1176;           jid=Rone1176 ;;
    rone2a)         tag=Rone2A-latest;            title="RONE 2A (Latest)";             folder=Rone2A;             jid=Rone2A ;;
    sidechain)      tag=RoneSidechain-latest;     title="RONE Sidechain (Latest)";      folder=RoneSidechain;      jid=RoneSidechain ;;
    flanger)        tag=RoneFlanger-latest;       title="RONE Flanger (Latest)";        folder=rone-flanger-;      jid=RoneFlanger ;;
    sync_verb)      tag=RoneSyncVerb-latest;      title="RONE Sync Verb (Latest)";      folder="rone sync verb";   jid=RoneSyncVerb ;;
    afterspace)     tag=RoneAfterspace-latest;    title="RONE AFTERSPACE (Latest)";     folder="RONE AFTER SPACE"; jid=RoneAfterspace ;;
    plugins_center) tag=PluginsCenter-latest;     title="RONE Plugins Center (Latest)"; folder=RonePluginsCenter;  jid=__center__ ;;
    analyzer)       tag=RONEAnalyzer-latest;      title="RONE Analyzer (Latest)";       folder=RONEAnalyzer;       jid=RONEAnalyzer ;;
    *) echo "::error::$id: unknown product - skipped"; continue ;;
  esac

  # This platform's installer files for this plugin
  files=()
  for dir in artifacts/installer-${id}-${PLATFORM}/; do
    [ -d "$dir" ] || continue
    for f in "$dir"*; do
      [ -f "$f" ] && files+=("$f")
    done
  done

  if [ ${#files[@]} -eq 0 ]; then
    echo "⚠ No $PLATFORM artifacts for $id — skipping"
    continue
  fi

  # Windows goes out first (Liran, 2026-10-26): each platform publishes on its own,
  # as soon as its own builds are done. This used to be "every platform or nothing"
  # (#253), because ONE version field covered both; versions.json now has
  # version_mac for the Mac build, so the Windows pass never relabels the old .pkg.

  # Version-regression guard, before any upload: the version this platform's
  # leg stamped (buildinfo-*), newer than live, built from the folder that is
  # on main now.
  # One line per file: version.txt has no trailing newline, so a plain cat glued
  # the Win and Mac stamps into one string ("1.3.10.2671.3.10.267", run #267).
  vers=$(for f in buildinfo/buildinfo-${id}-${PLATFORM}/version.txt; do [ -f "$f" ] && { tr -d '\r\n' < "$f"; echo; }; done | sort -u)
  if [ -z "$vers" ] || [ "$(printf '%s\n' "$vers" | wc -l)" -ne 1 ] || ! printf '%s' "$vers" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "::error::$id: $PLATFORM build version missing or not a version ('$(echo $vers)') - not published"
    guard_skipped="$guard_skipped $id"; continue
  fi
  if ! guard_check "$id" "$folder" "$jid" "$vers"; then
    guard_skipped="$guard_skipped $id"; continue
  fi
  # A newer run already put its build on this release (its versions.json
  # commit may still be on the way): never overwrite it with an older one.
  cur=$(gh release view "$tag" -R "$REPO" --json body -q .body 2>/dev/null | grep -oE 'Build #[0-9]+' | head -1 | tr -dc '0-9' || true)
  if [ -n "$cur" ] && [ "$cur" -gt "$RUN" ]; then
    echo "::error::$id: $tag already carries build #$cur, newer than this run #$RUN - not published"
    guard_skipped="$guard_skipped $id"; continue
  fi

  NOTES="Build #$RUN · $(date -u +%Y-%m-%d)"
  if gh release view "$tag" -R "$REPO" >/dev/null 2>&1; then
    # Leftovers nothing points at, best effort: what a stuck upload moved
    # aside (stale-*), and a hand-parked copy like the Center's
    # RONE_Plugins_Center-stuck-<id>.dmg (audit R289).
    for sid in $(gh api "repos/$REPO/releases/tags/$tag" --jq '.assets[] | select((.name | startswith("stale-")) or (.name | test("-stuck-"))) | .id' 2>/dev/null || true); do
      gh api -X DELETE "repos/$REPO/releases/assets/$sid" >/dev/null 2>&1 || true
    done
  elif ! gh release create "$tag" --title "$title" --notes "$NOTES" --latest=false -R "$REPO" \
       && ! gh release view "$tag" -R "$REPO" >/dev/null 2>&1; then
    # Created empty and filled one file at a time below, like every upload.
    # The other platform's job may have created it a moment before: then it is there.
    echo "::error::$id: could not create the release $tag - nothing of it is published"
    upload_failed="$upload_failed $id"; continue
  fi

  # 1. The versioned files, the ones versions.json will name. All of them
  #    or the product is not published - and the loop goes on to the next
  #    product instead of ending the step (audit R030): a failure used to
  #    exit here, so every product uploaded before it kept the old sha256.
  ok=1; vfiles=()
  mkdir -p "$VDIR/$id"
  for f in "${files[@]}"; do
    v="$VDIR/$id/$(versioned_name "$(basename "$f")")"
    cp "$f" "$v" && vfiles+=("$v") || ok=0
  done
  if [ "$ok" = 1 ]; then
    for v in "${vfiles[@]}"; do
      upload_one "$tag" "$v" || { ok=0; break; }
    done
  fi
  if [ "$ok" != 1 ]; then
    echo "::error::$id: not every installer reached $tag - not published, versions.json keeps the previous build"
    upload_failed="$upload_failed $id"; continue
  fi

  # 2. The unversioned alias (RoneStutter_Installer.exe, ...): the site's
  #    download buttons and older Centers' built-in catalogue use it,
  #    versions.json does not. A failure warns and the Verdict turns the
  #    run red, but the product is published - its versioned files are up.
  for f in "${files[@]}"; do
    if ! upload_one "$tag" "$f"; then
      echo "::warning::$id: the alias $(basename "$f") on $tag was not refreshed (versions.json does not use it)"
      alias_failed="$alias_failed $id:$(basename "$f")"
    fi
  done
  gh release edit "$tag" --title "$title" --notes "$NOTES" -R "$REPO" || echo "::warning::release notes of $tag not updated"

  echo "✓ $tag — ${#files[@]} $PLATFORM file(s), versioned -$SUFFIX"
  published=$(echo "$published" | jq -c --arg id "$id" '. + [$id]')
done

# update-manifest bumps only what was really published here.
echo "published=$published" >> $GITHUB_OUTPUT
echo "guard_skipped=${guard_skipped# }" >> $GITHUB_OUTPUT
echo "upload_failed=${upload_failed# }" >> $GITHUB_OUTPUT
echo "alias_failed=${alias_failed# }" >> $GITHUB_OUTPUT
if [ -n "$guard_skipped" ]; then
  echo "### Release guard skipped:$guard_skipped" >> $GITHUB_STEP_SUMMARY
fi
if [ -n "$upload_failed" ]; then
  echo "### Upload failed, not published:$upload_failed" >> $GITHUB_STEP_SUMMARY
fi
if [ -n "$alias_failed" ]; then
  echo "### Alias not refreshed:$alias_failed" >> $GITHUB_STEP_SUMMARY
fi
