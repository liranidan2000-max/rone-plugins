# RONE VST MANAGER — work notes

## Open

- f-daws (Studio One one click, Pro Tools on Windows): NOT built - neither DAW is installed on this PC, so it cannot be made or tested here. Studio One projects are read (Health, Will it open?).
- Deploy the site's /api/v1/vstm/backup (site f8cb7c2, committed, not deployed: the site checkout holds other sessions' uncommitted work) - backup/restore answer 404 until then.
- Not tested in a DAW: Live one-click MIDI (needs Live restarted once for the new script); RONE RACK only in REAPER.

## Done

- [x] 02.11.2026 — "בבריאות, כשאני מסיר פלאגין ולוחץ, הדף קופץ לשנייה, חוזר, ורק אחרי כמה זמן השורה נעלמת" → row leaves at once, scroll kept, one check-up instead of two → 0.9.13 b1d6d7c (RELEASED 2.11)
- [x] 02.11.2026 — "בעמודה הצדדית: אם אני מרחיב, שיתחילו שתי שורות של ריבועים" → strip ≥360 px = tile grid (container query) → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "כפתור שנותן את הרשימה בצורה מאוזנת, שיזכיר RACK של אבלטון — הריבועים למטה" → strip button / Setup "Bottom": docked along the bottom of the screen, every section a row of devices → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "ב-RACK: ללחוץ על הפלאגין ולערוך אותו בתוך ה-RACK עצמו; שהתמונה תהיה כמו הפריסט" → click the slot picture: `--edit` child opens with the slot's state, Save = a user preset in the slot (group = rack name), picture = its shot → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "הלוגו מוכפל" + "זה אמור להיות הלוגו הרשמי ולא ציור" → title bar draws Resources/RoneMark.png cut from RONE_Logo_2025_Final-09 → 0.9.13 b1d6d7c
- [x] 02.11.2026 — Round 1 of the roadmap: b-remote, b-daw, b-mac, b-setup, b-speed, b-err, f-ver, f-compat, f-recent, f-tour → 0.9.13 b1d6d7c
- [x] 02.11.2026 - Round 2 (f-search, f-tags, f-bulk, f-disk, f-stats, f-own, f-hub) -> RoneVstManager c11030e
- [x] 02.11.2026 - Round 3 (f-proj, f-open, f-upd, f-backup, f-move, f-remote, f-midi) -> 72ce9c8 (+ vstm/updates.json here, site f8cb7c2)
- [x] 02.11.2026 - Round 4 (f-presets, f-share, f-anydaw = RONE RACK) -> 0.9.14 08f6232 (not released)
