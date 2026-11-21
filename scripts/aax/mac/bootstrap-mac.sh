#!/bin/bash
# RONE - Mac AAX (Pro Tools) workstation set-up.
#
# Run it in Terminal on the Mac (it is safe to run again; every step checks first):
#
#   bash -c "$(curl -fsSL https://raw.githubusercontent.com/liranidan2000-max/rone-plugins/main/scripts/aax/mac/bootstrap-mac.sh)"
#
# What it does: Xcode command-line tools, Homebrew, git/cmake/ninja/gh/jq, GitHub sign-in,
# the monorepo + its submodules in ~/RONE/rone-plugins, iLok License Manager, Claude Code.
# What it only CHECKS (they need Liran's own hands/passwords): the iLok on USB, the PACE
# wraptool, the Apple "Developer ID Application" identity in the keychain, Pro Tools.
# At the end it prints a checklist and the one command that starts Claude on the hand-off
# brief (scripts/aax/mac/HANDOFF.md).
#
# Written for the bash 3.2 that ships with macOS (no associative arrays, no mapfile).

set -u
ROOT="$HOME/RONE"
REPO="$ROOT/rone-plugins"
GH_REPO="liranidan2000-max/rone-plugins"
TEAM_ID="AN9FQN75QW"
STATUS="$ROOT/mac-aax-status.txt"
mkdir -p "$ROOT"

ok()   { printf '  \033[32m[OK]\033[0m   %s\n' "$*"; echo "OK    $*" >> "$STATUS"; }
todo() { printf '  \033[33m[TODO]\033[0m %s\n' "$*"; echo "TODO  $*" >> "$STATUS"; }
bad()  { printf '  \033[31m[FAIL]\033[0m %s\n' "$*"; echo "FAIL  $*" >> "$STATUS"; }
step() { printf '\n\033[36m== %s\033[0m\n' "$*"; }
pause(){ printf '%s ' "$*"; read -r _ </dev/tty; }

echo "RONE Mac AAX set-up  $(date '+%Y-%m-%d %H:%M')" > "$STATUS"
echo "macOS $(sw_vers -productVersion)  $(uname -m)" >> "$STATUS"

# ---------------------------------------------------------------------------------------------
step "1. Xcode command-line tools"
if xcode-select -p >/dev/null 2>&1; then
  ok "command-line tools at $(xcode-select -p)"
else
  xcode-select --install >/dev/null 2>&1
  pause "A window asked to install the command-line tools. Click Install, wait until it ends, then press Enter here."
  if xcode-select -p >/dev/null 2>&1; then ok "command-line tools installed"; else bad "command-line tools still missing - run the script again"; exit 1; fi
fi

# ---------------------------------------------------------------------------------------------
step "2. Homebrew + build tools"
if ! command -v brew >/dev/null 2>&1; then
  for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do [ -x "$b" ] && eval "$("$b" shellenv)"; done
fi
if ! command -v brew >/dev/null 2>&1; then
  echo "  Installing Homebrew (it asks for the Mac's login password - type it, nothing is shown while typing)."
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" </dev/tty
  for b in /opt/homebrew/bin/brew /usr/local/bin/brew; do [ -x "$b" ] && eval "$("$b" shellenv)"; done
  # keep brew on PATH in new Terminal windows
  if [ -x /opt/homebrew/bin/brew ] && ! grep -q 'brew shellenv' "$HOME/.zprofile" 2>/dev/null; then
    echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> "$HOME/.zprofile"
  fi
fi
if command -v brew >/dev/null 2>&1; then
  ok "Homebrew $(brew --version | head -1 | awk '{print $2}')"
  for f in git cmake ninja gh jq; do
    if brew list --formula "$f" >/dev/null 2>&1; then ok "$f"; else
      brew install "$f" >/dev/null 2>&1 && ok "$f installed" || bad "$f could not be installed (brew install $f)"; fi
  done
else
  bad "Homebrew is missing - run the script again"; exit 1
fi

# ---------------------------------------------------------------------------------------------
step "3. GitHub sign-in (the private plugin repos need it)"
if gh auth status -h github.com >/dev/null 2>&1; then
  ok "gh signed in as $(gh api user --jq .login 2>/dev/null)"
else
  echo "  A browser opens: sign in to GitHub as liranidan2000-max and paste the code shown here."
  gh auth login --hostname github.com --git-protocol https --web </dev/tty
  gh auth status -h github.com >/dev/null 2>&1 && ok "gh signed in" || { bad "GitHub sign-in did not finish - run the script again"; exit 1; }
fi
gh auth setup-git >/dev/null 2>&1 && ok "git uses the gh sign-in"

# ---------------------------------------------------------------------------------------------
step "4. The monorepo in $REPO"
if [ -d "$REPO/.git" ]; then
  git -C "$REPO" pull --ff-only --quiet && ok "rone-plugins updated ($(git -C "$REPO" rev-parse --short HEAD))" \
    || todo "rone-plugins could not fast-forward (local changes?) - Claude will sort it out"
else
  gh repo clone "$GH_REPO" "$REPO" -- --quiet && ok "rone-plugins cloned ($(git -C "$REPO" rev-parse --short HEAD))" \
    || { bad "clone failed"; exit 1; }
fi
echo "  Fetching the plugin submodules (private repos, a few minutes the first time)..."
if git -C "$REPO" submodule update --init --jobs 4 >/dev/null 2>"$ROOT/submodules.log"; then
  ok "all submodules present"
else
  todo "some submodules did not come (see $ROOT/submodules.log) - Claude can fetch them one by one"
fi

# ---------------------------------------------------------------------------------------------
step "5. iLok"
if [ -d "/Applications/iLok License Manager.app" ]; then
  ok "iLok License Manager installed"
else
  if brew install --cask ilok-license-manager >/dev/null 2>&1; then ok "iLok License Manager installed (brew)"; else
    open "https://www.ilok.com/#!license-manager"
    todo "install iLok License Manager from the page that just opened (ilok.com), then run the script again"; fi
fi
if system_profiler SPUSBDataType SPUSBHostDataType 2>/dev/null | grep -qi "ilok"; then
  ok "an iLok is plugged in"
else
  todo "no iLok seen on USB - plug in the iLok 3 (the one from the PC)"
fi

# ---------------------------------------------------------------------------------------------
step "6. PACE wraptool (signs AAX)"
WT=""
for c in /Applications/PACEAntiPiracy/Eden/Fusion/Versions/*/bin/wraptool /usr/local/bin/wraptool "$HOME"/Downloads/*/wraptool; do
  [ -x "$c" ] && WT="$c"
done
if [ -n "$WT" ]; then ok "wraptool at $WT ($("$WT" version 2>/dev/null | head -1))"; else
  todo "wraptool missing: iLok License Manager > PACE CENTRAL > DEVELOPER > SDK Download > PACECodeSigningForAAXSDK (macOS). Claude walks you through it."; fi

# ---------------------------------------------------------------------------------------------
step "7. Apple signing identity (Developer ID Application)"
if security find-identity -v -p codesigning 2>/dev/null | grep -q "Developer ID Application:.*($TEAM_ID)"; then
  ok "$(security find-identity -v -p codesigning | grep "Developer ID Application:.*($TEAM_ID)" | head -1 | sed 's/^ *[0-9]*) [0-9A-F]* //')"
else
  todo "Developer ID Application ($TEAM_ID) not in the keychain: 1Password > vault RONE > 'Apple Code Signing' > attachment apple-devid-app.p12 > double-click it, password in the same item. Claude walks you through it."
fi

# ---------------------------------------------------------------------------------------------
step "8. Pro Tools (for the test)"
if ls -d /Applications/Pro\ Tools*.app >/dev/null 2>&1; then ok "$(ls -d /Applications/Pro\ Tools*.app | head -1)"; else
  todo "Pro Tools not installed: Avid Link (avid.com) > Pro Tools Intro (free), same Avid account as on the PC"; fi

# ---------------------------------------------------------------------------------------------
step "9. Claude Code"
if command -v claude >/dev/null 2>&1 || [ -x "$HOME/.local/bin/claude" ]; then
  ok "claude CLI present"
else
  curl -fsSL https://claude.ai/install.sh | bash >/dev/null 2>&1 && ok "claude CLI installed" || todo "claude CLI not installed - the desktop app's Code tab works too"
  case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) grep -q '.local/bin' "$HOME/.zprofile" 2>/dev/null || echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.zprofile";; esac
fi

# ---------------------------------------------------------------------------------------------
step "Done"
echo "  Status saved in $STATUS"
grep -c '^TODO' "$STATUS" | xargs -I{} echo "  {} step(s) left for your hands - Claude does them with you."
cat <<EOF

  Next: start Claude in the monorepo and give it the brief.
  Terminal (new window):
      cd "$REPO" && claude "Read scripts/aax/mac/HANDOFF.md and $STATUS, then do it"
  Or the Claude desktop app: Code tab > folder $REPO, and send the same sentence.

EOF
