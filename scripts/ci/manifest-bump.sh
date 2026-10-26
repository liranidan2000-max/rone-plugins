# Update versions.json, one platform: plans the entries of what Release
# (PLATFORM) published. Sourced by the Update versions.json (Windows) / (macOS)
# jobs of .github/workflows/main.yml, which set:
#   PLATFORM  windows | macos     REPO, RUN    SUFFIX  that Release's asset suffix
#   published     JSON list of what that Release published
#   published_win (macOS pass) JSON list of what the Windows Release published
#   matrix_win / matrix_mac  detect-changes' matrices
# The Windows pass moves a plugin's version / download_url / sha256 / size; the
# macOS pass its version_mac / download_url_mac / sha256_mac / size_mac
# (manifest-plan.sh, MANIFEST_PLATFORM). The Center goes in the macOS pass with
# both platforms, as before.
# Writes the plan to $RUNNER_TEMP/bump-plan.txt and guard_skipped to GITHUB_OUTPUT.

# Only what Release really published (a product the release guard
# skipped kept its old assets, so its entry must stay too).
guard_skipped=''

# Release already ran the full guard BEFORE uploading; every id in
# 'published' is now this run's build on <Tag>-latest, so its entry
# must follow it - never held back because main moved since (that
# would pair the new installer with the old sha256). Skipped only
# when origin/main's versions.json already has this version or newer,
# or a newer build has since replaced the asset (guard_manifest_check).
# A failed fetch fails this job loudly; re-run it to write what
# Release published.
GUARD_FETCH_ARGS=""
GUARD_NO_RUN_COMMIT=1
GUARD_PLATFORM=$PLATFORM
source scripts/ci/release-guard.sh
# This step only writes the PLAN; Commit & push applies it on top of
# whatever main is when it pushes (audit R207, scripts/ci/manifest-plan.sh).
source scripts/ci/manifest-plan.sh
PLAN="$RUNNER_TEMP/bump-plan.txt"
: > "$PLAN"
if ! [[ "$SUFFIX" =~ ^[0-9]+\.[0-9]+$ ]]; then
  echo "::error::Release reported no asset suffix ('$SUFFIX') - versions.json left alone; re-run the workflow"
  exit 1
fi
if ! guard_fetch_main; then
  {
    echo "### versions.json NOT updated"
    echo "Release published $published - those installers are live but the"
    echo "manifest still has the old version/sha256. Re-run this job."
  } >> $GITHUB_STEP_SUMMARY
  exit 1
fi

for id in $(echo "$published" | jq -r '.[]'); do
  case "$id" in
    reverse_reverb) folder=ReverseReverbVST;  jid=ReverseReverb;  tag=ReverseReverb-latest;  inst=ReverseReverb ;;
    stems_fixer)    folder=RoneStemsFixer;     jid=RoneStemsFixer;  tag=RoneStemsFixer-latest;  inst=RoneStemsFixer ;;
    stutter)        folder=RoneStutter;        jid=RoneStutter;     tag=RoneStutter-latest;     inst=RoneStutter ;;
    stucker)        folder=RoneStucker;        jid=RoneStucker;     tag=RoneStucker-latest;     inst=RoneStucker ;;
    throw)          folder=RoneThrow;          jid=RoneThrow;       tag=RoneThrow-latest;       inst=RoneThrow ;;
    clipper)        folder=RoneClipper;        jid=RoneClipper;     tag=RoneClipper-latest;     inst=RoneClipper ;;
    rise)           folder=RoneRise;           jid=RoneRise;        tag=RoneRise-latest;        inst=RoneRise ;;
    iron)           folder=RoneIron;           jid=RoneIron;        tag=RoneIron-latest;        inst=RoneIron ;;
    rone1176)       folder=Rone1176;           jid=Rone1176;        tag=Rone1176-latest;        inst=Rone1176 ;;
    rone2a)         folder=Rone2A;             jid=Rone2A;          tag=Rone2A-latest;          inst=Rone2A ;;
    sidechain)      folder=RoneSidechain;      jid=RoneSidechain;   tag=RoneSidechain-latest;   inst=RoneSidechain ;;
    flanger)        folder=rone-flanger-;      jid=RoneFlanger;     tag=RoneFlanger-latest;     inst=RoneFlanger ;;
    sync_verb)      folder="rone sync verb";   jid=RoneSyncVerb;   tag=RoneSyncVerb-latest;    inst=RoneSyncVerb ;;
    afterspace)     folder="RONE AFTER SPACE"; jid=RoneAfterspace; tag=RoneAfterspace-latest;  inst=RoneAfterspace ;;
    plugins_center) folder=RonePluginsCenter;  jid=__center__;     tag=PluginsCenter-latest;   inst=RONE_Plugins_Center ;;
    analyzer)       folder=RONEAnalyzer;       jid=RONEAnalyzer;   tag=RONEAnalyzer-latest;    inst=RONEAnalyzer ;;
  esac

  # The Center carries both platforms in one entry, so it goes out once both
  # are up: in the macOS pass, and only if the Windows Release published it too.
  if [ "$jid" = "__center__" ]; then
    if [ "$PLATFORM" != macos ]; then
      echo "  $jid: written by the macOS pass, with both platforms"
      continue
    fi
    if [ "$(echo "${published_win:-[]}" | jq -r --arg id "$id" 'any(.[]; . == $id)')" != true ]; then
      echo "::error::$id: the Windows Release did not publish it - the Center entry stays as it is"
      guard_skipped="$guard_skipped $id"; continue
    fi
  fi

  # The label is the version the installers carry (buildinfo, which
  # Release already checked); main's CMakeLists is only a cross-check.
  base=$(grep -oP 'VERSION\s+\K[0-9]+\.[0-9]+\.[0-9]+' "$folder/CMakeLists.txt" 2>/dev/null | head -1 || true)
  bi="$PLATFORM"; [ "$jid" = "__center__" ] && bi='*'
  built=$(for f in buildinfo/buildinfo-${id}-${bi}/version.txt; do [ -f "$f" ] && { tr -d '\r\n' < "$f"; echo; }; done | sort -u || true)
  if [ -n "$built" ] && [ "$(printf '%s\n' "$built" | wc -l)" -eq 1 ] && printf '%s' "$built" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$'; then
    ver="$built"
    if [ -n "$base" ] && [ "$ver" != "${base}.${RUN}" ]; then
      echo "::warning::$id: installers carry $ver, main's CMakeLists says ${base}.${RUN} - the manifest follows the installers"
    fi
  elif [ -n "$base" ]; then
    ver="${base}.${RUN}"
    echo "::warning::$id: no single build version in buildinfo ('$(echo $built)') - labelled $ver from CMakeLists"
  else
    echo "::error::$id: no build version and no CMakeLists version - manifest left alone although Release published it; re-run this job"
    guard_skipped="$guard_skipped $id"; continue
  fi

  # Compute SHA256 for whatever we built this run (per-platform).
  # A platform that did not build -> empty string, and its fields stay.
  sha_win=""; sha_mac=""
  if [ "$jid" = "__center__" ]; then
    win_file=$(find artifacts -type f -name "${inst}_Installer.exe" 2>/dev/null | head -1)
    mac_file=$(find artifacts -type f -name "${inst}.dmg"            2>/dev/null | head -1)
  else
    win_file=$(find artifacts -type f -name "${inst}_Installer.exe" 2>/dev/null | head -1)
    mac_file=$(find artifacts -type f -name "${inst}_Installer.pkg" 2>/dev/null | head -1)
  fi
  # A plugin's entry takes this pass's platform only; the Center takes both.
  if [ "$jid" != "__center__" ]; then
    if [ "$PLATFORM" = macos ]; then win_file=""; else mac_file=""; fi
  fi
  [ -n "$win_file" ] && sha_win=$(sha256sum "$win_file" | awk '{print $1}')
  [ -n "$mac_file" ] && sha_mac=$(sha256sum "$mac_file" | awk '{print $1}')
  size_win=0; size_mac=0
  [ -n "$win_file" ] && size_win=$(stat -c%s "$win_file")
  [ -n "$mac_file" ] && size_mac=$(stat -c%s "$mac_file")
  echo "  $jid: sha_win=${sha_win:0:12}... sha_mac=${sha_mac:0:12}..."

  # Nothing at all was produced for this plugin this run, which
  # means its build failed. Bumping the version anyway would point
  # the Center at the *previous* installer under a new number, with
  # a hash that does not match it - an update that can only fail.
  if [ -z "$win_file" ] && [ -z "$mac_file" ]; then
    echo "  $jid: no artefacts this run - manifest left alone"
    continue
  fi

  # The Center needs both platforms of this run (#253): one version for both.
  if [ "$jid" = "__center__" ] && { [ -z "$win_file" ] || [ -z "$mac_file" ]; }; then
    echo "::error::$jid: this run has not both its installers - Center entry left alone"
    guard_skipped="$guard_skipped $id"; continue
  fi

  # Leave the entry only when the live manifest already has this
  # version (rc 2, a re-run) or newer, or a newer build now owns
  # <Tag>-latest (rc 1, flagged). No folder-vs-main test here.
  rc=0; guard_manifest_check "$id" "$jid" "$ver" "$tag" "$RUN" "$REPO" || rc=$?
  if [ "$rc" -eq 2 ]; then continue; fi
  if [ "$rc" -ne 0 ]; then
    guard_skipped="$guard_skipped $id"; continue
  fi

  # The versioned files Release uploaded (audit R028/R205): nothing ever
  # replaces the bytes behind these URLs, so a Center can never pair
  # them with another build's hash. Only a platform built this run
  # gets a URL; the other keeps its fields.
  if [ "$jid" = "__center__" ]; then
    wname="${inst}_Installer-${SUFFIX}.exe"; mname="${inst}-${SUFFIX}.dmg"
  else
    wname="${inst}_Installer-${SUFFIX}.exe"; mname="${inst}_Installer-${SUFFIX}.pkg"
  fi
  url=""; url_mac=""
  [ -n "$sha_win" ] && url="https://github.com/$REPO/releases/download/$tag/$wname"
  [ -n "$sha_mac" ] && url_mac="https://github.com/$REPO/releases/download/$tag/$mname"

  # Never write a URL that does not answer, or whose bytes GitHub hashes
  # differently: the entry then stays on the previous build (which still
  # matches its own hash) and the run goes red.
  bad=""
  if [ -n "$url" ]; then
    plan_url_live "$url" || bad="$bad $wname (not downloadable)"
    plan_asset_digest_ok "$REPO" "$tag" "$wname" "$sha_win" || bad="$bad $wname (sha256 differs)"
  fi
  if [ -n "$url_mac" ]; then
    plan_url_live "$url_mac" || bad="$bad $mname (not downloadable)"
    plan_asset_digest_ok "$REPO" "$tag" "$mname" "$sha_mac" || bad="$bad $mname (sha256 differs)"
  fi
  if [ -n "$bad" ]; then
    echo "::error::$id:$bad - manifest left alone; re-run this job"
    guard_skipped="$guard_skipped $id"; continue
  fi

  plan_add "$PLAN" "$id" "$jid" "$ver" "$tag" "$url" "$sha_win" "$size_win" "$url_mac" "$sha_mac" "$size_mac"
  echo "  $jid: planned $ver"
done

echo "guard_skipped=${guard_skipped# }" >> $GITHUB_OUTPUT
if [ -n "$guard_skipped" ]; then
  echo "### Release guard left alone:$guard_skipped" >> $GITHUB_STEP_SUMMARY
fi
if [ -s "$PLAN" ]; then
  {
    echo "### Planned versions.json entries"
    echo '```'
    cat "$PLAN"
    echo '```'
  } >> $GITHUB_STEP_SUMMARY
else
  echo "nothing published this run - versions.json left untouched"
fi

# Audit R027: every installer URL a Center will follow - the ones this
# run planned and the ones main already carries, which Commit & push
# writes back - must be a file on this repository's releases. Anything
# else (another host, another repository, a dot segment) stops the
# manifest here, before anything is written.
url_re='^https://github\.com/liranidan2000-max/rone-plugins/releases/download/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$'
if ! main_urls=$(jq -r '.center_installer.url, .center_installer.url_mac, (.plugins[]? | .download_url, .download_url_mac) | select(. != null)' versions.json); then
  echo "::error::versions.json on main does not parse - nothing written"
  exit 1
fi
plan_urls=""
if [ -s "$PLAN" ]; then plan_urls=$(cut -d'|' -f5,8 "$PLAN" | tr '|' '\n'); fi
bad_urls=$(printf '%s\n%s\n' "$main_urls" "$plan_urls" | grep -v '^$' | grep -Ev "$url_re" || true)
dot_urls=$(printf '%s\n%s\n' "$main_urls" "$plan_urls" | grep -E '/\.\.?(/|$)' || true)
if [ -n "$bad_urls$dot_urls" ]; then
  echo "::error::versions.json would name an installer that is not a file on this repository's releases - nothing written (audit R027):"
  printf '%s\n%s\n' "$bad_urls" "$dot_urls" | grep -v '^$' | sed 's/^/  /' || true
  {
    echo "### versions.json NOT updated: a download URL failed the R027 check"
    printf '%s\n%s\n' "$bad_urls" "$dot_urls" | grep -v '^$' | sed 's/^/- /' || true
  } >> $GITHUB_STEP_SUMMARY
  exit 1
fi
echo "R027: $(printf '%s\n%s\n' "$main_urls" "$plan_urls" | grep -c . || true) installer URLs checked"
