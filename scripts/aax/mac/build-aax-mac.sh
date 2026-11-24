#!/bin/bash
# Builds the macOS AAX (Pro Tools) version of RONE plugins, from the RELEASED code.
# The macOS twin of ../build-aax.ps1 - same rules:
#   - never touches a plugin's working copy (they can be on DEV branches): for each product it
#     exports the commit the monorepo points at (git ls-tree <ref> <folder>) with git archive,
#     plus the monorepo's Shared/ from the same commit, into a staging folder;
#   - in the staging CMakeLists only: AAX appended to the juce_add_plugin FORMATS line and
#     AAX_CATEGORY inserted (RONEAnalyzer: set(RONE_BRIDGE_FORMATS VST3 AU AAX));
#   - builds only <target>_AAX, universal (x86_64 + arm64), deployment target 10.15, like CI.
# Output: ~/RONE/_build/aax/unsigned/<bundle>.aaxplugin + build-info.json, logs per product.
#
#   scripts/aax/mac/build-aax-mac.sh                      # every product
#   scripts/aax/mac/build-aax-mac.sh RoneClipper RoneDelay
#   REF=origin/main CLEAN=1 scripts/aax/mac/build-aax-mac.sh RoneClipper
set -uo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../../.." && pwd)"
CFG="$HERE/../aax-products.json"
REF="${REF:-HEAD}"
OUT="${OUT:-$HOME/RONE/_build/aax}"
CLEAN="${CLEAN:-0}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu)}"

SRC="$OUT/src"; BLD="$OUT/build"; LOGS="$OUT/logs"; UNSIGNED="$OUT/unsigned"; CACHE="$OUT/_cache"
mkdir -p "$SRC" "$BLD" "$LOGS" "$UNSIGNED" "$CACHE"

for t in git jq cmake ninja perl lipo; do
  command -v "$t" >/dev/null || { echo "missing tool: $t" >&2; exit 2; }
done

ALL_IDS=$(jq -r '.products[].id' "$CFG")
if [ $# -gt 0 ]; then
  for id in "$@"; do
    echo "$ALL_IDS" | grep -qx "$id" || { echo "Unknown product id: $id" >&2; exit 2; }
  done
  IDS="$*"
else
  IDS=$(echo $ALL_IDS)
fi

get_juce() {  # $1 = tag -> prints the cached clone's path
  local dir="$CACHE/juce-$1"
  if [ ! -f "$dir/CMakeLists.txt" ]; then
    echo "Fetching JUCE $1..." >&2
    rm -rf "$dir"
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$1" https://github.com/juce-framework/JUCE.git "$dir" >&2 || return 1
  fi
  echo "$dir"
}

# ---- Shared/ from the same monorepo commit ---------------------------------------------
rm -rf "$SRC/Shared"
git -C "$REPO" archive --format=tar "$REF" -- Shared | tar -xf - -C "$SRC" || { echo "export of Shared/ failed" >&2; exit 1; }
REF_SHORT=$(git -C "$REPO" rev-parse --short "$REF")

RESULTS="$CACHE/results.tsv"; : > "$RESULTS"

build_one() {  # $1 = id ; writes one line to $RESULTS
  local id="$1" row folder target bundle category fline log dst sha commit version tag
  row=$(jq -c --arg id "$id" '.products[] | select(.id == $id)' "$CFG")
  folder=$(echo "$row" | jq -r .folder); target=$(echo "$row" | jq -r .target)
  bundle=$(echo "$row" | jq -r .bundle); category=$(echo "$row" | jq -r .category)
  fline=$(echo "$row" | jq -r '.formats_line // ""')
  log="$LOGS/$id.log"; echo "=== $id  ($(date '+%Y-%m-%dT%H:%M:%S'))" > "$log"
  commit=""; version=""; local note=""

  fail() { note="$note $1"; echo "FAILED  $id: $1 (log: $log)"; printf '%s\t%s\t%s\t%s\t%s\n' "$id" "$commit" "$version" FAILED "${note# }" >> "$RESULTS"; }

  echo; echo "=== $id ==="
  # 1. export the released source
  local entry; entry=$(git -C "$REPO" ls-tree "$REF" -- "$folder")
  [ -n "$entry" ] || { fail "$folder is not in $REF"; return; }
  local kind; kind=$(echo "$entry" | awk '{print $2}'); sha=$(echo "$entry" | awk '{print $3}')
  dst="$SRC/$folder"; rm -rf "$dst"
  if [ "$kind" = commit ]; then
    commit=${sha:0:7}
    local gitdir="$REPO/$folder"
    if [ ! -e "$gitdir/.git" ]; then
      local url; url=$(git -C "$REPO" config -f .gitmodules --get "submodule.$folder.url")
      gitdir="$CACHE/repos/$folder.git"
      [ -d "$gitdir" ] || git clone --quiet --bare "$url" "$gitdir" >>"$log" 2>&1 || { fail "clone of $url failed"; return; }
    fi
    git -C "$gitdir" cat-file -e "$sha^{commit}" 2>/dev/null || git -C "$gitdir" fetch --quiet origin "$sha" >>"$log" 2>&1 || { fail "fetch $sha failed"; return; }
    git -C "$gitdir" archive --format=tar --prefix="$folder/" "$sha" | tar -xf - -C "$SRC" || { fail "archive failed"; return; }
  else
    commit="vendored@$REF_SHORT"
    git -C "$REPO" archive --format=tar "$REF" -- "$folder" | tar -xf - -C "$SRC" || { fail "archive failed"; return; }
  fi
  [ -f "$dst/CMakeLists.txt" ] || { fail "export of $folder has no CMakeLists.txt"; return; }

  # Another product's sources it compiles (HOOK builds RONE IRON's flatten): the released commit too.
  local need nsha ngit nurl
  for need in $(echo "$row" | jq -r '.needs[]?'); do
    nsha=$(git -C "$REPO" ls-tree "$REF" -- "$need" | awk '$2 == "commit" {print $3}')
    [ -n "$nsha" ] || { fail "$need is not a submodule in $REF"; return; }
    ngit="$REPO/$need"
    if [ ! -e "$ngit/.git" ]; then
      nurl=$(git -C "$REPO" config -f .gitmodules --get "submodule.$need.url")
      ngit="$CACHE/repos/$need.git"
      [ -d "$ngit" ] || git clone --quiet --bare "$nurl" "$ngit" >>"$log" 2>&1 || { fail "clone of $nurl failed"; return; }
    fi
    git -C "$ngit" cat-file -e "$nsha^{commit}" 2>/dev/null || git -C "$ngit" fetch --quiet origin "$nsha" >>"$log" 2>&1 || { fail "fetch $nsha failed"; return; }
    rm -rf "$SRC/$need"
    git -C "$ngit" archive --format=tar --prefix="$need/" "$nsha" | tar -xf - -C "$SRC" || { fail "archive of $need failed"; return; }
  done

  # Flanger vendors JUCE without extras/Build (its .gitignore drops Build/); CI adds it from JUCE.
  if [ -f "$dst/JUCE/CMakeLists.txt" ] && [ ! -d "$dst/JUCE/extras/Build" ]; then
    local vt; vt=$(perl -0ne 'print $1 if /project\(JUCE VERSION ([0-9.]+)/' "$dst/JUCE/CMakeLists.txt")
    [ -n "$vt" ] || { fail "vendored JUCE without a version"; return; }
    local vj; vj=$(get_juce "$vt") || { fail "JUCE $vt clone failed"; return; }
    cp -R "$vj/extras/Build" "$dst/JUCE/extras/Build"
  fi

  # 2. add AAX to the staging copy's CMakeLists
  local cm="$dst/CMakeLists.txt"
  version=$(perl -0ne 'print $1 if /project\(\s*\S+\s+VERSION\s+([0-9.]+)/' "$cm")
  tag=$(perl -0ne 'print $1 if /FetchContent_Declare\(\s*JUCE.*?GIT_TAG\s+(\S+)/s' "$cm"); tag=${tag:-8.0.4}
  cp "$cm" "$cm.orig"
  if [ "$fline" = bridge ]; then
    # On a Mac the line that counts is the APPLE one, set(RONE_BRIDGE_FORMATS VST3 AU).
    perl -0pi -e 's/set\(RONE_BRIDGE_FORMATS VST3 AU\)/set(RONE_BRIDGE_FORMATS VST3 AU AAX)/' "$cm"
  else
    T="$target" perl -0pi -e '
      my $t = quotemeta $ENV{T};
      s{(juce_add_plugin\(\s*$t\b.*?\n[ \t]*FORMATS[ \t]+)([^\r\n#]+)}{
        my ($h, $f) = ($1, $2); my @f = grep { $_ ne "AAX" } split " ", $f; $h . join(" ", @f, "AAX") }se;' "$cm"
  fi
  cmp -s "$cm" "$cm.orig" && { fail "could not find the FORMATS line of juce_add_plugin($target)"; return; }
  T="$target" C="$category" perl -0pi -e 'my $t = quotemeta $ENV{T}; s{juce_add_plugin\(\s*$t\b}{juce_add_plugin($ENV{T}\n    AAX_CATEGORY $ENV{C}}' "$cm"
  rm -f "$cm.orig"
  local pn; pn=$(T="$target" perl -0ne 'my $t = quotemeta $ENV{T}; print $1 if /juce_add_plugin\(\s*$t\b.*?PRODUCT_NAME\s+"([^"]+)"/s' "$cm")
  if [ -n "$pn" ] && [ "$pn" != "$bundle" ]; then note="PRODUCT_NAME is '$pn', json says '$bundle'"; bundle="$pn"; fi

  # 3. configure + build only the AAX target
  local b="$BLD/$id"
  [ "$CLEAN" = 1 ] && rm -rf "$b"
  local juce; juce=$(get_juce "$tag") || { fail "JUCE $tag clone failed"; return; }
  local flags=(-S "$dst" -B "$b" -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    "-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64"
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
    "-DFETCHCONTENT_SOURCE_DIR_JUCE=$juce"
    "-DCMAKE_PROJECT_INCLUDE=$SRC/Shared/RoneBuildHooks.cmake"
    -Wno-dev)
  # The Analyzer takes a local ~/JUCE before FetchContent (an old one on this Mac that the
  # macOS 15 SDK refuses: CGWindowListCreateImage); point it at the same JUCE as CI.
  [ "$folder" = RONEAnalyzer ] && flags+=("-DJUCE_PATH=$juce")
  echo "cmake ${flags[*]}" >> "$log"
  cmake "${flags[@]}" >>"$log" 2>&1 || { fail "configure failed"; return; }
  echo "  building ${target}_AAX (JUCE $tag, v$version)..."
  cmake --build "$b" --config Release --target "${target}_AAX" --parallel "$JOBS" >>"$log" 2>&1 || { fail "build failed"; return; }

  # 4. collect
  local built="$b/${target}_artefacts/Release/AAX/$bundle.aaxplugin"
  [ -d "$built" ] || built="$b/${target}_artefacts/AAX/$bundle.aaxplugin"
  [ -d "$built" ] || { fail "no bundle under $b/${target}_artefacts"; return; }
  local bin="$built/Contents/MacOS/$bundle"
  local archs; archs=$(lipo -archs "$bin" 2>/dev/null)
  case " $archs " in *" x86_64 "*) ;; *) fail "not universal: [$archs]"; return;; esac
  case " $archs " in *" arm64 "*) ;; *) fail "not universal: [$archs]"; return;; esac
  local ob="$UNSIGNED/$bundle.aaxplugin"
  rm -rf "$ob"; ditto "$built" "$ob"
  echo "OK  $ob  [$archs]"
  printf '%s\t%s\t%s\t%s\t%s\n' "$id" "$commit" "$version" OK "$note" >> "$RESULTS"
}

for id in $IDS; do build_one "$id"; done

# ---- summary + build-info.json (merged with earlier runs) --------------------------------
STAMP="$UNSIGNED/build-info.json"
[ -f "$STAMP" ] || echo '{"products":[]}' > "$STAMP"
NOW=$(date '+%Y-%m-%dT%H:%M:%S')
jq -R -s --arg ref "$REF_SHORT" --arg now "$NOW" --slurpfile prev "$STAMP" '
  (split("\n") | map(select(length > 0) | split("\t")
    | {id: .[0], commit: .[1], version: .[2], result: .[3], note: (.[4] // ""), built: $now})) as $new
  | {ref: $ref,
     products: ((($prev[0].products // []) | map(select(.id as $i | ($new | map(.id) | index($i)) | not))) + $new
                | sort_by(.id))}' "$RESULTS" > "$STAMP.tmp" && mv "$STAMP.tmp" "$STAMP"

echo
{ printf 'ID\tCOMMIT\tVERSION\tRESULT\tNOTE\n'; cat "$RESULTS"; } | column -t -s $'\t'
echo "Unsigned bundles: $UNSIGNED"
FAILED=$(awk -F'\t' '$4 != "OK"' "$RESULTS" | wc -l | tr -d ' ')
[ "$FAILED" = 0 ] || { echo "$FAILED product(s) failed - logs in $LOGS" >&2; exit 1; }
