# RONE VST MANAGER — work notes

## Open

- f-daws (Studio One one click, Pro Tools on Windows): NOT built - neither DAW is installed on this PC, so it cannot be made or tested here. Studio One projects are read (Health, Will it open?).
- Deploy the site's /api/v1/vstm/backup (site f8cb7c2, committed, not deployed: the site checkout holds other sessions' uncommitted work) - backup/restore answer 404 until then.
- Not tested in a DAW: Live one-click MIDI (needs Live restarted once for the new script); RONE RACK only in REAPER.

## Done
- [x] 05.11.2026 — "כשיש הודעה כזאת הכפתור שמוריד אותה 'הבנתי' לא מוצג הוא למטה" (screenshot: "RONE 1176 did not load") → in a low bottom strip the dialog was taller than the window. **Done in 0.9.21 b8fe851: every dialog stops at the window height, the buttons stay on screen, Enter/Esc closes, the steps are translated. 6 window sizes tested.**
- [x] 04.11.2026 — "בטריי, בווינדוז, התפריט שלו נראה כמו ג'וס רגיל, צריך להיות מעוצב, יפה. אותו דבר גם לגבי ארוני סנטר עצמו" → the tray menu is a stock JUCE popup; give it the brand look in VST MANAGER and in the Center. **Done: RoneMenuLook.h in both apps; VST MANAGER 0.9.20, Center 2.1.24.**
- [x] 04.11.2026 — "עוד כפתור בסייד פאנל, כפתור שמחליט אם הפאנל נשאר אחרי לחיצה על פלאגין או שהוא נסגר אוטומטית" → strip button: stay open / auto close after a load. **Done: pin button in the strip (settings.stripAutoClose) → 0.9.20.**
- [x] 04.11.2026 — "כשעושים זום, זה רק מקטין או מגדיל את התמונות ... כשזה נהיה יותר קטן, זה אמור לעשות כבר כמה שורות" → strip zoom must reflow: smaller cards = more rows in the same height. **Done: small tiles = more rows, in the rack and by the side (tiles, tiny) → 0.9.20.**
- [x] 04.11.2026 — "פלאגינים של סטוק של FL Studio שהוא לא מצא לי כמו Frequency Shifter. וגם פלאגין חיצוני של דאדה לייף ... תבדוק גם מה פוספס פה ... שלא יפספס להם דברים" → scan misses FL stock plugins and a dada life plugin; find what is missed, for every user. **Done: FL Plugin database + FL stock as format FL + Live/REAPER folders + VST folders on every drive; 381 more entries, Endless Smile + Frequency Shifter found → 0.9.20. NOT tested: FL stock one click in real FL.**
- [x] 04.11.2026 — "ריבוע מסומן בצהוב, מקיף את הפלאגין הראשון שעולה ... לוחץ אינטר, הוא פותח אותו ... מחצים במקלדת ... אני לא באמת מזיז את העכבר" → strip keyboard flow: first result highlighted in yellow, Enter loads, arrows move by orientation. **Done: moveCursor + setCursor, best match first → 0.9.20.**
- [x] 04.11.2026 — "אם אני הקלדתי משהו ולחצתי על משהו, אני רוצה שימחק לי ההקלדה בפעם הבאה שהוא ייפתח" → after a load the strip search is cleared for the next open. **Done: stripOpens clears after a load → 0.9.20.**
- [x] 04.11.2026 — "הרמתי, הגדלתי, מתחתי ... זה הפך לשתי שורות, ואז קטנתי ... שיחקתי עם הזום, וזה נהיה ככה" (screenshot: cards cut at the top, chips clipped) → bottom strip layout breaks across resize + zoom; must hold in every combination. **Done: fitDockRows caps the tile to the strip height, left column gives way → 0.9.20 (30 size + zoom combinations, 0 cut).**
- [x] 04.11.2026 — "לפעמים שאני לוחץ על פלאגין, הוא כותב לי את ההודעה הזאת ... וגם יצא מצב ... הוא הגביר לי שם במיקסר את הווליום ... העכבר הולך לשם, ניסה לפתוח, אבל בטעות הוא הגביר בכלל את הווליום" (screenshots: "RONE Clipper did not load", FL mixer EQ band at 16.0k +18) → one click in FL fails at times, and the mouse landed on the mixer EQ and moved a fader. Never click a spot it has not verified. **Done in 0.9.19, RoneVstManager aa83c55, released on 4.11 (Windows). The one click presses on a window of ours over the Mixer EQ; a moving hand sent the press to FL and the load failed. Fake mixer + moving cursor: old 8 of 8 presses reached it, new 0. Quiet hand: 6 of 6 loaded on both. NOT tested in real FL.**

- [x] 02.11.2026 — "בבריאות, כשאני מסיר פלאגין ולוחץ, הדף קופץ לשנייה, חוזר, ורק אחרי כמה זמן השורה נעלמת" → row leaves at once, scroll kept, one check-up instead of two → 0.9.13 b1d6d7c (RELEASED 2.11)
- [x] 02.11.2026 — "בעמודה הצדדית: אם אני מרחיב, שיתחילו שתי שורות של ריבועים" → strip ≥360 px = tile grid (container query) → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "כפתור שנותן את הרשימה בצורה מאוזנת, שיזכיר RACK של אבלטון — הריבועים למטה" → strip button / Setup "Bottom": docked along the bottom of the screen, every section a row of devices → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "ב-RACK: ללחוץ על הפלאגין ולערוך אותו בתוך ה-RACK עצמו; שהתמונה תהיה כמו הפריסט" → click the slot picture: `--edit` child opens with the slot's state, Save = a user preset in the slot (group = rack name), picture = its shot → 0.9.13 b1d6d7c
- [x] 02.11.2026 — "הלוגו מוכפל" + "זה אמור להיות הלוגו הרשמי ולא ציור" → title bar draws Resources/RoneMark.png cut from RONE_Logo_2025_Final-09 → 0.9.13 b1d6d7c
- [x] 02.11.2026 — Round 1 of the roadmap: b-remote, b-daw, b-mac, b-setup, b-speed, b-err, f-ver, f-compat, f-recent, f-tour → 0.9.13 b1d6d7c
- [x] 02.11.2026 - Round 2 (f-search, f-tags, f-bulk, f-disk, f-stats, f-own, f-hub) -> RoneVstManager c11030e
- [x] 02.11.2026 - Round 3 (f-proj, f-open, f-upd, f-backup, f-move, f-remote, f-midi) -> 72ce9c8 (+ vstm/updates.json here, site f8cb7c2)
- [x] 02.11.2026 - Round 4 (f-presets, f-share, f-anydaw = RONE RACK) -> 0.9.14 08f6232 (not released)
