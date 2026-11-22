# release-check — "what is missing for this plugin to be out everywhere"

One read-only command over the monorepo, the site, the Center, the manuals, the test bench and the media ledger.
Born 22.11.2026 from the release-machine audit (`docs/mockups/release-machine.html`): RONE Reverb, Hook and
Listen had shipped without RONE Control and nothing noticed.

```
scripts\release-check\release-check.cmd                      all products, all 25 stations
scripts\release-check\release-check.cmd --plugin RoneListen  one product (id or folder)
scripts\release-check\release-check.cmd --plugin RonePanorama --stage build   a plugin in development
scripts\release-check\release-check.cmd --stage site|build|media --json out.json
```

Exit code 1 = something selected is red. The "משחררים" skill (`~/.claude/skills/rone-release/SKILL.md`)
ends only when this is green for the plugin.

## Stations
build: versions.json entry · RONE Control (rack.json + CMake at the SHIPPED pointer) · 2+ factory maps · crash
reporter · lock-screen words ("14-day free trial", like the site - 22.11.2026) · installer + AppId · hands block · CI paths-filter incl. `Shared/RoneControlRack/**` · Center icon +
VersionChecker row · test-bench row · factory presets (ledger).
site: manual PDF · Control chapter in the PDF · 3D unit (units.json + 4 graphics) · product page + buy button ·
Paddle price incl. `_REGULAR` · homepage card + fx.js showcase · living demo · internal lists (sitemap, llms.txt,
account.js, pricing + every list `site-counts.mjs --check` covers) · global: every count on the site.
media (ledger.json): YouTube tutorial (versions.json `video`) · preset video · Shorts · TikTok · launch reel ·
update carousel · studio trial reel · 3D launch film 16:9 + 9:16 (read from its job.json, see below).

## launch-film.mjs (22.11.2026)
The Higgsfield film where the unit builds itself from its parts (Seedance 2.5, 15 s, 1080p, sound, real render as
image 1), in 16:9 and 9:16, on every release. Generation goes through the Higgsfield MCP connector of Liran's PLUS
account (no API key on a consumer plan), so Claude runs it inside the `rone-release` skill; the script does the rest:
`prepare` (folder `WEBSITE RONE AUDIO\videos\ai-launch-films\<slug>\`, both prompts, job.json) · `record` (writes the
paid job down; refuses PPU's ULTRA workspace and a 3rd try without `--force`) · `finish` (download, size/length/sound
check, `check-<aspect>.jpg` with a frame per shot) · `approve` / `reject` · `status`. Per-unit colour + screen words:
`launch-film.json`. Run it with node directly; the `.cmd` breaks on quoted paths under Git Bash.

Exempt: RONEAnalyzer has no Control by design (`NO_RACK`). Free / subscription-only products need no lifetime price.

## ledger.json
What no tool can see (YouTube, Instagram, TikTok, presets). Update the product's row the moment something goes up;
`release-media.mjs` opens the board cards for a new release.

## Nightly
Windows task **RONE Release Check**, 07:40 (`install-nightly.ps1`, `nightly.vbs`): Telegram to Liran through the
Kalifa bot (same path as the Test Bench) ONLY when a station turns red that was not red the run before. State in
`%LOCALAPPDATA%\RONE\release-check\` (`last.json`, `last-report.txt`, `notify-errors.log`).

## release-board.mjs (22.11.2026)
The same check drawn as one Hebrew page for Liran: a dot per station (green done, red missing - Claude completes it,
amber waits for him, grey not relevant). `node scripts/release-check/release-board.mjs [--plugin <Id>] [--out file.html]`.
Every release publishes it as an Artifact at the start and at the end (skill rone-release).
