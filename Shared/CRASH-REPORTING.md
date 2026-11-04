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
    Crashes inside a DAW have their own route, see "Crashes inside a DAW".
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

## Crashes inside a DAW

Two routes, because neither alone is enough:

- **`Shared/RoneDawCrashGuard.cpp`** (Windows VST3). Compiled into every plugin by
  `Shared/RoneBuildHooks.cmake`, which CI passes with `-DCMAKE_PROJECT_INCLUDE`, so
  no plugin's CMakeLists changes. A vectored handler that only watches: it always
  passes the exception on, ignores C++ exceptions, and reports only when our module
  is at the crash or on the crashing stack. It writes one `DAW_CRASH` report per
  process, once in 12 hours per crash, from a helper thread the crash waits for at
  most 1.5 s. The hook also moves the DLL's entry point to the guard
  (`/ENTRY:roneGuardEntry`), so the handler is gone before any destructor can fault.
  Its header comment lists every rule. The proof is `Shared/tests/crash-guard` (a
  fake plugin and a host that catches everything, like FL Studio does), one process
  per case, and it takes a real plugin as an argument too. Every defence in the guard
  has a case that fails without it, checked by taking them out one at a time.
- **`RonePluginsCenter/Source/OsCrashScanner.cpp`** (Windows and macOS). Reads what
  the OS recorded (Application Error events, `.ips` files) and files `OS_CRASH`
  reports for crashes whose faulting module is ours. Needs nothing in the plugin, so
  it covers versions already installed, and it is the only Mac route: a signal
  handler in a plugin would fight the DAW's own. `Shared/tests/os-scan` runs it on
  the local machine and sends nothing. Crashes inside the VST Manager count only
  when the faulting module is ours, since it hosts other people's plugins on purpose.

The Support Center groups both by module+offset, so one crash that arrives by both
routes is one issue.

## From an offset to a line

`RoneBuildHooks.cmake` also gives every Windows binary a PDB (`/Z7`, `/DEBUG:FULL`,
with `/OPT:REF,ICF` kept so the code does not change). This repo is public, so CI
uploads each PDB to the `symbols` release of the private tracker repo, as
`<pdb id>.zip`, which is the id a `DAW_CRASH` report carries on its `build:` line.
That needs the `SYMBOLS_TOKEN` secret (contents: write on rone-crash-reports), and
that repo needs at least one commit, since GitHub refuses a release on an empty repo.
Without them the step skips or fails on its own and the release goes on. The Support
Center fetches the zip and resolves the stack with `tools\symbolize`.

## Known gaps

- macOS stacks stay module+offset: dSYMs are not stored yet.
- Only `DAW_CRASH` reports carry a PDB id. An `OS_CRASH` report stays module+offset,
  and a Standalone or Center crash keeps JUCE's plain backtrace.
- A crash the OS never logs, such as a host killed by a watchdog, leaves no trace.
- A fault in a plugin's own static constructor or destructor is not reported: the
  Windows loader swallows it and the guard is not armed at that moment.
- An offset moves with every build, so the same bug opens a new group per version.
  Grouping by function after symbols are resolved would close that.
- Reports wait for the Center to run. A customer who removed it sends nothing.
- No dedup at intake - every report is its own issue. The admin console's
  Crashes tab groups them (same product + code + message) and triages each
  group; the Support Center groups them further.
