# RONE crash & error reporting

> Activated 2026-09-01: private repo + crash token are live (verified
> end-to-end — test issue #1 created and closed via the token). Since Center
> 2.0 the token is the website Worker's `GITHUB_CRASH_TOKEN` secret, and the
> repo it files into is `GITHUB_CRASH_REPO` (a var in the site's wrangler.toml).

One pipeline for the whole bundle: every product captures its own crashes and
errors, everything lands in one private issue tracker, and fixes ship back
through the Plugins Center.

```
 plugin / standalone / Center
   │  crash or error
   ▼
 %APPDATA%\RonePlugins\CrashReports\pending\*.json     (local queue, offline-safe)
   │  next time the Center runs (it is the ONLY uploader; it holds no token)
   ▼
 roneaudio.com/api/v1/crash                            (website Worker: checks, rate-limits, files)
   │  with the Worker's GITHUB_CRASH_TOKEN secret
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
    ≥60s between passes. Each report is POSTed to roneaudio.com/api/v1/crash,
    and the Worker files it as one GitHub issue. What the answer does to the
    queued file:
    - 200 / 201: filed - the file is deleted.
    - 400 / 413 / 422: the server will never take it - the file is dropped.
    - 429 / 5xx (or no answer): the file is kept for a later pass; 429 ends
      the pass. From Center 2.1.4 on, a report answered 5xx moves to the back
      of the queue (oldest first; two 5xx in a row end the pass), so one report
      the server keeps failing on never blocks the ones behind it.
  - The Center also auto-reports every user-visible error status it shows
    (the `emitStatusMessage(..., "error")` hook) — download failures, sha
    mismatches, install failures etc. arrive without the tester doing anything.

- **Token**: no binary holds one. The website Worker (WEBSITE RONE AUDIO,
  worker/api/v1/crash.js) files the issues with its `GITHUB_CRASH_TOKEN`
  secret into `GITHUB_CRASH_REPO`; Centers 2.0+ only POST the report to it.
  Without the secret the Worker answers 503 and the reports stay queued on
  each machine until it is set. (Historical, before Center 2.0 only: the CI
  secret `CRASH_REPORT_TOKEN` was compiled into the Center through the CMake
  option `RONE_CRASH_TOKEN`. Neither is used any more.)

## One-time setup (Liran, ~3 minutes)

1. Create a **private** repo: github.com/new → name `rone-crash-reports` →
   Private → Create.
2. Create a fine-grained token: github.com/settings/personal-access-tokens →
   Generate new token → Resource owner: you → Only select repositories:
   `rone-crash-reports` → Repository permissions: **Issues: Read and write**
   (nothing else) → no expiry (or 1 year) → Generate, copy it.
3. Save it to `D:\RONE PLUGINS\rone-plugins\.claude\crash-report-token.txt`
   (`.claude/` is in the root `.gitignore`; never commit this file). Claude
   reads and triages the reports locally with it.
4. Put it on the Worker: double-click `scripts\crash-secret.cmd` in
   `D:\RONE PLUGINS\WEBSITE RONE AUDIO`. It reads the file from step 3 and sets
   it as the Worker secret `GITHUB_CRASH_TOKEN` (never printed; the log is
   `scripts\crash-secret.log`). Nothing goes into CI.
5. Tell Claude "done".

## Security honesty

Since Center 2.0 the token no longer ships inside the Center binary: it lives
only on the server (the website Worker's `GITHUB_CRASH_TOKEN` secret), which
checks and rate-limits what it files. Builds made before 2.0 did embed it, so
the old token should be rotated (it can ONLY read/create issues in the crash
repo).

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
- No dedup at intake - every report is its own issue. The admin console's
  Crashes tab groups them (same product + code + message) and triages each
  group; Claude dedupes the rest during triage.
