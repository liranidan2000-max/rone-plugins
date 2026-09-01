# RONE crash & error reporting

One pipeline for the whole bundle: every product captures its own crashes and
errors, everything lands in one private issue tracker, and fixes ship back
through the Plugins Center.

```
 plugin / standalone / Center
   │  crash or error
   ▼
 %APPDATA%\RonePlugins\CrashReports\pending\*.json     (local queue, offline-safe)
   │  next time the Center runs (it is the ONLY uploader / token holder)
   ▼
 github.com/liranidan2000-max/rone-crash-reports       (private repo, one issue per report)
   │  Claude triages, Liran approves, fix ships via CI
   ▼
 tester clicks Update in the Center
```

## Pieces

- **`Shared/RoneCrashReporter.h`** (queue side, no secrets)
  - `installCrashHandler(product, version, wrapper)` — SEH/signal handler via
    `juce::SystemStats::setApplicationCrashHandler`; writes a JSON report with
    a stack backtrace. Installed ONLY in processes we own: every standalone
    (guarded by `wrapperType == wrapperType_Standalone`) and the Center.
    Never in a DAW — the host process's crash handling is not ours to take.
  - `reportError(product, version, wrapper, code, message, details)` — for
    non-fatal "why didn't it work" events. Throttled: identical code+message
    at most once per 12h per machine.
  - Reports carry: product, version, wrapper, OS/arch/locale, an anonymous
    random install id (no personal data), timestamp, message/details/stack.
  - RoneStutter keeps a local snapshot (`RoneStutter/Source/RoneCrashReporter.h`),
    same as its other Shared copies — keep in sync.

- **`RonePluginsCenter/Source/CrashReportUploader.h`** (upload side)
  - Drains the queue on Center launch and after any error, ≤20 reports/pass,
    ≥60s between passes. Each report becomes one GitHub issue; the file is
    deleted only on HTTP 201.
  - The Center also auto-reports every user-visible error status it shows
    (the `emitStatusMessage(..., "error")` hook) — download failures, sha
    mismatches, install failures etc. arrive without the tester doing anything.

- **Token**: CMake option `RONE_CRASH_TOKEN`, injected in CI from the
  `CRASH_REPORT_TOKEN` Actions secret, Center target only — plugins never
  hold it. Empty token (local builds / secret missing) = uploads silently
  disabled, reports stay queued and upload retroactively once a token-carrying
  Center runs.

## One-time setup (Liran, ~3 minutes)

1. Create a **private** repo: github.com/new → name `rone-crash-reports` →
   Private → Create.
2. Create a fine-grained token: github.com/settings/personal-access-tokens →
   Generate new token → Resource owner: you → Only select repositories:
   `rone-crash-reports` → Repository permissions: **Issues: Read and write**
   (nothing else) → no expiry (or 1 year) → Generate, copy it.
3. Add it to CI: repo `rone-plugins` → Settings → Secrets and variables →
   Actions → New repository secret → name `CRASH_REPORT_TOKEN`, value = the
   token.
4. So Claude can read/triage the reports locally, also save it to
   `D:\RONE PLUGINS\rone-plugins\.claude\crash-report-token.txt`
   (that folder is gitignored).
5. Tell Claude "done" — the next release build bakes the token into the
   Center and uploading goes live.

## Security honesty

The token ships inside the Center binary, so a determined person can extract
it. It can ONLY read/create issues in the crash repo — worst case is spam
there; revoke + rebuild rotates it. Fine for a private beta; before a public
release move the upload behind a tiny serverless relay (open item, same
bucket as code signing).

## Triage flow

- Claude checks the repo for new issues (scheduled daily check + on demand),
  groups duplicates, briefs Liran with a proposed fix.
- Liran approves → fix lands → release via CI → issue closed with
  "fixed in build #N" → testers just press Update.

## Not in v1 (known gaps)

- Crashes of the VST3 **inside a DAW** are not captured (would require
  out-of-process crashpad-style tooling; the host owns the process).
- Stacks are module+offset only (Release builds without PDBs). Claude maps
  offsets by rebuilding the exact tag locally; shipping PDB artifacts from CI
  is a future improvement.
- No server-side dedup — Claude dedupes during triage.
