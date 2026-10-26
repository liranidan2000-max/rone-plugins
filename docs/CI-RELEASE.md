# How a release goes out

`.github/workflows/main.yml`, one run per push to `main`:

1. **Detect changes** - the path filter picks the products whose folder (or
   `Shared/**` except `Shared/tests/**` and `Shared/check-vendored-copies.sh`,
   its `.iss`, its manual PDF, or the workflow itself) changed in this push.
   Every product builds on **both** platforms. The filter compares the push
   with the commit before it (`before`), so commits pushed together count as
   one change set: a `[skip ci]` commit pushed together with a release commit
   is part of that release's filter.
2. **Win / Mac legs** - build, sign (when the secrets exist), validate
   (warnings only), package. Each leg stamps `<CMake base>.<run>` into the
   installer and into a `buildinfo-*` artefact.
3. **Release (Windows)** / **Release (macOS)** - one job per platform, each
   waiting only for its own legs (`scripts/ci/release-publish.sh`): per
   product, after the release guard (`scripts/ci/release-guard.sh`), uploads
   that platform's installer twice to its `<Tag>-latest` release (see *Asset
   names*).
4. **Update versions.json (Windows)** / **(macOS)** - writes that platform's
   entries onto the newest `main` and pushes (`scripts/ci/manifest-bump.sh`,
   `scripts/ci/manifest-plan.sh`).
5. **Verdict** - red when anything was held back, failed or only half done,
   even though whatever could go out went out.

### Windows first (2026-10-26)

Apple's notarization queue can take an hour. Windows no longer waits for it:
Release (Windows) and the Windows pass of versions.json run as soon as the
Windows legs are done, and the Mac follows when its legs are.

| Field | Written by | Read by |
|---|---|---|
| `version`, `download_url`, `sha256`, `size` | Windows pass | every Center on Windows; Mac Centers before 2.1.11 |
| `version_mac`, `download_url_mac`, `sha256_mac`, `size_mac` | macOS pass | Mac Centers from 2.1.11 (`version_mac` over `version`) |

The Windows pass first copies the old `version` into `version_mac` when the
entry has none, so the Mac build keeps its own number. The guard compares a
Mac build with `version_mac` (or `version` when absent).

The **Center** keeps one entry for both platforms: its installers go up per
platform, but its `center_installer` entry is written once, by the macOS pass,
and only when the Windows Release published it too.

Between the two passes a Mac Center older than 2.1.11 sees the new `version`
with the previous `.pkg` behind `download_url_mac`. If it installs then, it
stamps the old bytes with the new number; Center 2.1.11 notices that the bytes
it last installed are not the ones `sha256_mac` names for that version and
offers the update again.

## Asset names (audit R028)

Every installer is on its release twice:

| Name | Example | Who uses it |
|---|---|---|
| versioned, never replaced | `RoneStutter_Installer-272.1.exe`, `RONE_Plugins_Center-272.1.dmg` | `versions.json` (every Center, every plugin's update notice) |
| alias, replaced each release | `RoneStutter_Installer.exe`, `RONE_Plugins_Center.dmg` | the site's download buttons and the smartlink (Center only), older Centers' built-in catalogue |

The suffix is `<run>.<attempt>`: a re-run of the Release job uploads under new
names and never meets its own earlier files. Because the bytes behind a
versioned URL never change, a Center that holds an older manifest always
downloads exactly the file its hash describes. Update versions.json writes a
URL only after it answers and GitHub's own digest of the asset (when it has
one) equals the hash.

Nothing is pruned yet. GitHub allows 1000 assets per release; at two files
per build that is years. When pruning comes: by age only (60 days or more),
keep the five newest, and never a file the current `versions.json` names.

### When the Verdict is red after a release

- *Not published, an upload failed*: that product kept its previous build in
  `versions.json`. **Re-run all jobs** of the run ("Re-run failed jobs" only
  repeats the Verdict); the new attempt uploads under new names.
- *The alias was not refreshed*: `versions.json` is right, only the unversioned
  file (the site's Center download button) is old or missing. Upload the
  installer from the run's artefacts under the alias name with
  `gh release upload <tag> <file> --clobber`, or let the product's next release
  refresh it. A re-run would be held back: that version is already live.

### Holding a release back or rolling it back

Restore a product's **version, download_url, sha256, size and the `_mac`
twins together** (`version_mac` included, when the entry has it; the Center: `version`, `url`, `url_mac`, `sha256`,
`sha256_mac`) from an earlier `versions.json` commit (edit `versions.json` by
hand, the CI skip token in the commit subject). Never change one of those
fields alone.

**Only a commit whose `download_url` / `download_url_mac` (Center: `url` /
`url_mac`) end in `-<run>.<attempt>.<ext>` is a rollback target**, e.g.
`.../RoneStutter_Installer-272.1.exe`. Those versioned files are never
replaced, so the older build is served again with matching hashes.

An older commit names the `-latest` alias (`.../RoneStutter_Installer.exe`),
which now holds a newer build. **Never restore it**: every Center (1.3.x to
2.1.x) would download the new file against the old hash, fail the SHA256
check, and refetching the manifest returns the same bad pair, so nobody can
install, update or reinstall that product until `versions.json` is fixed by
hand again. A build released before the switch to versioned names has no
rollback file - re-release instead: fix or revert the product's code but keep
its CMake version at least the live one (the release guard publishes only a
strictly newer `<base>.<run>`), and push.

Before committing a rollback, check each restored URL:

    curl -sL <url> | sha256sum

must print exactly the `sha256` (or `sha256_mac`) you restore, and
`curl -sL <url> | wc -c` the `size` (or `size_mac`).

## Dry run (audit R128)

Actions → *Smart Build & Release* → **Run workflow**: pick a branch and type
the products (`stutter,throw` or `all`). Both legs build, sign and package,
and the installers are kept as run artefacts for five days. Release and Update
versions.json never run for a dry run, so nothing reaches customers. Use it to
test a branch's `main.yml` before it reaches `main`.

## Commit subject tags

| Tag | Effect |
|---|---|
| the CI skip token (`[skip ci]`) | no run at all. **Subject line only**: anywhere in the body it silently cancels the run (`scripts/hooks/commit-msg` refuses that; activate once per clone with `git config core.hooksPath scripts/hooks`) |
| `[skip-validate]` | pluginval and auval do not run - for a hotfix once they block |
| `[win-only]`, `[mac-only]` | **retired** (audit R127): a warning, and both platforms build. One version field covers both platforms, so a one-platform build left the other on its old installer under the new number |

**Never push `v*` tags** (audit R132). They no longer start a run of the
current workflow, but a tag on a commit older than the release guard (8fded85)
runs that commit's old workflow, which re-releases everything.

## Checks that only warn today, and what makes them block

| Check | Where | Next step |
|---|---|---|
| pluginval, every `.vst3`, both platforms (R133) | *Run pluginval* | When the logs show the tests really run (Windows now takes tens of seconds per plugin, not 1-2 s) and pass for every product: drop `continue-on-error` and turn the `::warning::` lines into `::error::` + `exit 1`. `[skip-validate]` is the way past a flaky run |
| auval for every AU (R133) | *Run auval* (Mac) | Same, once every AU prints `AU VALIDATION SUCCEEDED` |
| architectures, minimum macOS, pkg payload, preinstall (R077) | *Check the Mac build* | Make single lines blocking after several green runs |
| binaries older than the job (R129) | *Stale binary check* | If one ever fires, or after a few clean runs: make it an error, or delete only `<proj>_artefacts/Release` before *CMake configure* - never the whole `<proj>_artefacts` (JuceLibraryCode lives there) |

## Integrity pins (audit R130)

| What | Today | To finish |
|---|---|---|
| WebView2 bootstrapper (shipped and run by every installer) | must carry a valid Microsoft signature, or the leg fails after three downloads | done |
| pluginval zip | sha256 printed in *Run pluginval* | copy the logged hash into `pinned` in both legs (Windows and Mac zip differ) |
| Inno Setup | version printed after *Install Inno Setup* | `choco install innosetup --version=<that version>`; test with a dry run first (an image that already has another version may need `choco upgrade --allow-downgrade`) |
| ASIO SDK (analyzer) | `common/` compared with SDK 2.3.4, warns on a difference | after a run shows them equal, make a difference fail the analyzer leg |
| WebView2 SDK via FetchContent | unhashed nuget.org download in the plugins' CMakeLists | optional: pass `-DFETCHCONTENT_SOURCE_DIR_WEBVIEW2=<the version-pinned nuget folder>` - only after checking that each CMakeLists asks for the same SDK version, and with a dry run |
| CodeSignTool | fetched from `releases/latest` | pin with the signing work (audit R008) |

## Guards from the wave-8 integration (2026-10-13)

| Guard | Where | What it does |
|---|---|---|
| GH_PAT only for the submodule (R027) | both build legs: checkout + *Init submodule* | The checkout uses the job token with `persist-credentials: false`. *Init submodule* sets the PAT's `insteadOf` rule, fetches the submodule (3 tries) and removes the rule on exit, before any third-party step. A folder that is a submodule and cannot be fetched now fails that step instead of failing later in *CMake configure* |
| Installer URLs pinned (R027) | end of *Bump versions.json* | Every `url` / `url_mac` / `download_url` / `download_url_mac` - this run's plan and what `main` already carries - must match `^https://github\.com/liranidan2000-max/rone-plugins/releases/download/<tag>/<file>$` with no dot segment. Otherwise nothing is written and the run is red |
| Center page tests (R032/R197) | Mac leg, Center only | `npm ci` (3 tries) and `npm test` in `RonePluginsCenter/Resources/ui`. A failure holds the Center back on both platforms |
| Center version warning (R122) | Mac leg, Center only | A warning when the Center's `project(VERSION)` equals the base of the live `center_installer.version` - a Mac Center does not take a same-base rebuild |
| Center alias swap (R090) | *Create per-plugin releases* | On `PluginsCenter-latest` an existing alias is never deleted first: the new file goes up as `tmp-<run>-<name>`, the old asset becomes `stale-<id>-<name>` (removed by the next run's cleanup), then tmp takes the name. If the swap fails 3 times the previous file keeps the name and the Verdict says `alias_failed` |

`[win-only]` / `[mac-only]` (R050): already retired by R127 above; *Bump
versions.json* also leaves an entry alone when a product built on only one of
its platforms.

## Tests

```
sh scripts/hooks/commit-msg.test.sh
python scripts/ci/tests/workflow_syntax_test.py   # bash -n / PowerShell parser on every run block
python scripts/ci/tests/ci_release_test.py        # the Release step against a fake gh
python scripts/ci/tests/manifest_plan_test.py     # Update versions.json against a scratch origin
```

They need Python 3 with PyYAML, git and bash (Git Bash on Windows). jq is used
when installed, a stand-in that knows exactly the filters these scripts use
otherwise. Nothing leaves the machine.
