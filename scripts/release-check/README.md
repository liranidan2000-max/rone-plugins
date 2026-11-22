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
reporter · installer + AppId · hands block · CI paths-filter incl. `Shared/RoneControlRack/**` · Center icon +
VersionChecker row · test-bench row · factory presets (ledger).
site: manual PDF · Control chapter in the PDF · 3D unit (units.json + 4 graphics) · product page + buy button ·
Paddle price incl. `_REGULAR` · homepage card + fx.js showcase · living demo · internal lists (sitemap, llms.txt,
account.js, pricing + every list `site-counts.mjs --check` covers) · global: every count on the site.
media (ledger.json): YouTube tutorial (versions.json `video`) · preset video · Shorts · TikTok · launch reel ·
update carousel · studio trial reel.

Exempt: RONEAnalyzer has no Control by design (`NO_RACK`). Free / subscription-only products need no lifetime price.

## ledger.json
What no tool can see (YouTube, Instagram, TikTok, presets). Update the product's row the moment something goes up;
`release-media.mjs` opens the board cards for a new release.

## Nightly
Windows task **RONE Release Check**, 07:40 (`install-nightly.ps1`, `nightly.vbs`): Telegram to Liran through the
Kalifa bot (same path as the Test Bench) ONLY when a station turns red that was not red the run before. State in
`%LOCALAPPDATA%\RONE\release-check\` (`last.json`, `last-report.txt`, `notify-errors.log`).
