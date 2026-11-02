# RONE VST MANAGER — work notes

## Open

### Roadmap ticked 02.11.2026 (mockup https://claude.ai/artifact/WAzDHqXasEZn2aSStC9qSJ, db `picks`)
Everything ticked except the privacy switch (f-privacy).
- Round 2: f-search (fuzzy + role + Hebrew) · f-tags (tags + note) · f-bulk (multi-select + undo) · f-disk · f-stats · f-own · f-hub
- Round 3: f-proj (project report FL/Cubase/REAPER/Bitwig) · f-open ("will it open?") · f-upd (update checker) · f-backup (RONE account sync) · f-move (new-PC pack) · f-remote (phone + Stream Deck racks/favs/actions) · f-midi
- Round 4: f-presets (preset browser across plugins) · f-anydaw (RONE RACK plugin) · f-share (.ronerack) · f-daws (Studio One + Pro Tools Win)

## Done

- [x] 02.11.2026 — "בבריאות, כשאני מסיר פלאגין ולוחץ, הדף קופץ לשנייה, חוזר, ורק אחרי כמה זמן השורה נעלמת" → row leaves at once, scroll kept, one check-up instead of two → 0.9.13 b1d6d7c (RELEASED 2.11)
- [x] 02.11.2026 — "בעמודה הצדדית: אם אני מרחיב, שיתחילו שתי שורות של ריבועים" → strip ≥360 px = tile grid (container query) → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "כפתור שנותן את הרשימה בצורה מאוזנת, שיזכיר RACK של אבלטון — הריבועים למטה" → strip button / Setup "Bottom": docked along the bottom of the screen, every section a row of devices → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "ב-RACK: ללחוץ על הפלאגין ולערוך אותו בתוך ה-RACK עצמו; שהתמונה תהיה כמו הפריסט" → click the slot picture: `--edit` child opens with the slot's state, Save = a user preset in the slot (group = rack name), picture = its shot → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "הלוגו מוכפל" + "זה אמור להיות הלוגו הרשמי ולא ציור" → title bar draws Resources/RoneMark.png cut from RONE_Logo_2025_Final-09 → 0.9.13 b1d6d7c
- [x] 02.11.2026 — Round 1 of the roadmap: b-remote, b-daw, b-mac, b-setup, b-speed, b-err, f-ver, f-compat, f-recent, f-tour → 0.9.13 b1d6d7c
