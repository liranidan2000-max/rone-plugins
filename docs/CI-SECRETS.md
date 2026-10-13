# CI secrets and tokens

What `.github/workflows/main.yml` authenticates with, where each credential is
used, and how to replace it. Names only: no value ever goes into this file, the
repository, a log or a chat. Repository secrets live under **GitHub →
rone-plugins → Settings → Secrets and variables → Actions**; only Liran can see
or change them.

## Tokens

| Name | Kind | Used by | What for | Scope / expiry |
|---|---|---|---|---|
| `GH_PAT` | personal access token (repository secret) | build-windows, build-macos: only *Init submodule* (its `insteadOf` rule is removed when the step ends; the checkout uses the job token with `persist-credentials: false`, audit R027); update-manifest: the checkout and *Init changed submodules* | cloning the 11 private plugin repos (submodules); pushing the `versions.json` commit to `main` | **not recorded - to fill in:** classic or fine-grained, its scopes, its expiry date |
| `GITHUB_TOKEN` | automatic, per run | release (`contents: write`: `gh release upload / edit / create`); update-manifest (`gh release view`, read-only); build-windows: only *Sign binaries* and *Compile installer* (sign-windows.ps1 reads the CodeSignTool release with it) | everything that talks to this repository's releases | expires with the run; the workflow default is `contents: read` |

Since audit R051 (2026-10) `GITHUB_TOKEN` is no longer set for the whole
Windows build job, only for the two steps above.

Signing secrets (`AZURE_*`, `ACS_*`, `ESIGNER_*`, `WIN_SIGN_PFX_*`,
`MAC_CERT_*`, `APPLE_API_*`) are described in [CODE-SIGNING.md](CODE-SIGNING.md).
They are job-level today and stay dormant until they exist.

## One token, two jobs

`GH_PAT` both clones the private submodules and pushes `versions.json` to
`main`. The split below gives the clone a read-only token of its own, so the
build jobs no longer carry a credential that can write to this repository.

## Planned split (audit R051, needs Liran first)

1. Look up `GH_PAT` in GitHub → Settings → Developer settings and write its
   kind, scopes and expiry date into the table above.
2. Create a **fine-grained** token with *Contents: read* on the 11 submodule
   repositories only, and store it as the secret `GH_SUBMODULES_RO`.
3. Decide on a ruleset for `main`: only Liran and the Actions bot push, no
   force-push, no deletion; optionally block creating `v*` tags (a tag would
   run an old workflow, audit R132).

Then, in one commit with the CI skip token in its subject and the next normal
single-plugin release as the canary (rehearse it first with a dry run, Actions
→ Run workflow, see [CI-RELEASE.md](CI-RELEASE.md)):

- build-windows / build-macos: **done with `GH_PAT` itself** in the wave-8
  integration (audit R027): *Configure git PAT* is gone, the checkout uses the
  default token with `persist-credentials: false`, and *Init submodule* sets
  the `insteadOf` rule, fetches (3 tries, failing loudly for a real
  submodule) and removes the rule again. What is left is swapping
  `secrets.GH_PAT` for `secrets.GH_SUBMODULES_RO` in that step's `env`.
- update-manifest *Init changed submodules*: the same read-only token, step-only.
- The `versions.json` push keeps `GH_PAT` through the checkout token. Later
  option: push with `github.token` and `permissions: contents: write` on
  update-manifest only (works while `main` is unprotected; a push made with
  `GITHUB_TOKEN` starts no new run).
- After two green releases: narrow or revoke `GH_PAT`'s extra scopes.

A token with the wrong scope fails *Init submodule* before anything is built or
published, so customers never see the mistake.

## New plugin checklist line

A new plugin repository must be added to the read-only submodule token
(`GH_SUBMODULES_RO`, once it exists) - otherwise its first CI build cannot
clone it.

## Rotation

- `GH_PAT`: create the new token with the same scopes, replace the secret,
  start a dry run (Actions → Run workflow, any product): *Init submodule* must
  pass. Then revoke the old token.
- Signing secrets: see CODE-SIGNING.md.
