# Work notes — fixes Liran finds while producing

Liran produces his own tracks with RONE plugins and dictates what he finds
(bugs, missing presets, workflow wishes) straight into a Claude session.
Claude collects every note during the session and, at the end, files each one
here — one file per plugin (`<PluginFolder>.md`, same name as the monorepo
folder) — then fixes them in the plugin or asks which to fix first.

Format of each file:

- `## Open` — newest first: `- [ ] DD.MM.YYYY — the note in his words (Hebrew ok) → what it means technically`
- `## Done` — moved here when fixed: `- [x] DD.MM.YYYY — note → version / commit`
