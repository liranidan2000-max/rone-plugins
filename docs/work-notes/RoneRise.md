# RONE Rise — work notes

## Open

## Done
- [x] 29.10.2026 — "תוסיף פריסטים שווים וטובים, עם אפשרות לשמירה על הMACROS שלמטה, שזה ישמור לכל משתמש תמיד מה שהוא שמר, ושיהיה בהגדרות כפתור של RESET TO FACTORY MACROS" → a preset bar in the header: 17 factory presets + the user's own; SAVE = a named preset or SAVE AS <MODE> MACROS (the card's own); the gear = RESET TO FACTORY MACROS + DELETE. Per user, every project: `<app data>/RonePlugins/RoneRise/user-presets.json` → 1.0.15, RoneRise 99fada2
- [x] 29.10.2026 — "KILL LOW צריך להגיע ב100 אחוז KNOB ל חיתוך של 1500 לפחות" → KILL LOW sets its own reach: 100 % = 3 kHz in every mode (LIFTOFF stopped at ~330 Hz); the modes' factory KILL and old projects carry the old scale, so nothing already made changes sound (risesim killscale) → 1.0.15, RoneRise 99fada2
- [x] 29.10.2026 — "כאן תחליף את השמות הם כמו של ENDLESS SMILE פלאגין אחר אז שלא יהיה שקוף" → SUNRISE / HORIZON / UPLIFT / DEEP DIVE / SUPERNOVA → LIFTOFF / ORBIT / PULSAR / UNDERTOW / MELTDOWN (CLEAN / ECHO / GATED / FILTER / EXTREME labels kept). Still to follow: the manual (docs/manuals/tools/content_rise.py) and the product page on roneaudio.com → 1.0.15, RoneRise 99fada2
- [x] 29.10.2026 — "TAIL ... צריך להיות לו כפתור חיצוני יותר מהיר לשליטה לא רק בADVANCED" → a CUT / 1 / 2 / 3 / 4 segment in the header next to BYPASS → 1.0.15, RoneRise 99fada2
- [x] 29.10.2026 — "TAIL צריך להיות CUT בברירת מחדל" → default TAIL = Cut (was 1 bar) → 1.0.15, RoneRise 99fada2
