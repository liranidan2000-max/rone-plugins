# RONE Plugins — Beta Test

שלום, ותודה שאתה עוזר לי לבדוק את הפלאגינים! 🎛️

זה בנייה מוקדמת (beta). **הפלאגינים עדיין לא חתומים דיגיטלית**, אז ה-OS יזהיר אותך בהתקנה הראשונה. זה נורמלי — ההוראות למטה מסבירות איך לעקוף. זה ייעלם ברגע שאוציא גרסה מסחרית עם חתימה.

---

## 📦 מה יש בחבילה

| פלאגין | סוג | פורמטים |
|---|---|---|
| **Rone Sync Verb** | BPM-synced reverb עם hard-cut ו-ducking | VST3 · AU · Standalone |
| **Reverse Reverb** | Real-time reverse reverb | VST3 · AU · Standalone |
| **Rone Stutter** | Glitch / stutter effect | VST3 · AU · Standalone |
| **Rone Flanger** | Manual flanger עם visualizer | VST3 · Standalone |
| **Rone Stems Fixer** | כלי לניתוח ותיקון stems | Standalone |

---

## 🪟 התקנה ב-Windows

1. הורד את ה-`.exe` של הפלאגין.
2. בהרצה, Windows יציג **"Windows protected your PC"** (SmartScreen).
3. לחץ **"More info"** → **"Run anyway"**.
4. עבור על שלבי ההתקנה (Next → Next → Install).

> ⚠️ זו לא תוכנה מזיקה — רק אין לי עדיין EV Code Signing Certificate ($300/שנה). אקנה כשנתחיל למכור.

---

## 🍎 התקנה ב-macOS

1. הורד את ה-`.pkg`.
2. בפתיחה, macOS יציג **"cannot be opened because the developer cannot be verified"**.
3. **לחץ Cancel** (כן, ממש Cancel).
4. פתח **System Settings → Privacy & Security**, גלול למטה, ותראה הודעה על ההתקנה החסומה. לחץ **"Open Anyway"**.
5. נסה להריץ את ה-`.pkg` שוב — עכשיו יופיע כפתור **Open**.

**אם זה עדיין לא עובד**, פתח Terminal ותריץ:

```bash
xattr -dr com.apple.quarantine ~/Downloads/RoneSyncVerb_Installer.pkg
```

(החלף את שם הקובץ לפי הצורך.)

> 🎧 לפעמים אחרי ההתקנה צריך להריץ rescan plugins ב-DAW. ב-Logic: Preferences → Plug-In Manager → Full Scan.

---

## 🧪 מה אני רוצה שתבדוק

**קריטי (דיווח דחוף):**
- [ ] ההתקנה עבדה בלי לשבור כלום ב-DAW
- [ ] הפלאגין נטען ב-DAW שלך (Logic / Cubase / FL / Ableton / Reaper / Pro Tools)
- [ ] אין crash כשמוסיפים / מסירים את הפלאגין מטראק
- [ ] Automation של פרמטרים עובד
- [ ] Save/Load של session משחזר את ה-state נכון

**חשוב (פידבק):**
- [ ] האם הצליל טוב? מה תשנה?
- [ ] ה-UI ברור? מה מבלבל?
- [ ] ביצועים: CPU סביר? latency?
- [ ] האם הפלאגין "מרגיש" מקצועי?

**בונוס:**
- [ ] איך תשווה את זה לפלאגינים מסחריים דומים?
- [ ] כמה היית מוכן לשלם על זה?
- [ ] למי עוד שווה לשלוח לבטא?

---

## 🐛 איך לדווח על באגים

**תבנית דיווח** (העתק למייל/וואטסאפ):

```
פלאגין: [Sync Verb / Reverse Reverb / ...]
גרסה:   [1.0.0.XX — רואים בתחתית ה-UI]
DAW:    [Logic 11.1 / Cubase 13 / FL 21 / ...]
OS:     [macOS 14.5 / Windows 11]
CPU:    [M2 Pro / i7-12700 / ...]

מה קרה:
[תיאור קצר]

איך לשחזר:
1.
2.
3.

מה ציפית שיקרה:
[...]

Screenshot/Video: [אם אפשר]
```

שלח אלי ב-**liranidan2000@gmail.com** או בוואטסאפ.

---

## 🙏 תודה

זה שלב קריטי בפרויקט. כל פידבק — אפילו "זה מרגיש זול" או "ה-knob הזה מפריע לי" — שווה זהב.

— Liran
