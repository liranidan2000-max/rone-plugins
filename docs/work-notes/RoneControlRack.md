# RONE Control Rack (Shared/RoneControlRack, in every plugin) — work notes

Notes from the DEV rack fleet test, 28.10.2026 (FL Studio, Windows). Rack-wide notes live here; a note about one
plugin's own map also names the plugin.

## Open
- [ ] 28.10.2026 — "צריך להיות כמו פריסטים כאלה בתוך הקונטרול המובנה ... וגם Save Preset as Default ... כשהוא פותח את הפלנג'ר שלו, תמיד יהיה לו את הקונטרול לפי הפרמטרים שהוא יגדיר" → rack map presets (save / load a slot map per plugin) + "Save as default" so a fresh instance opens with the user's map. New feature: mockup (הדמיה) first.
- [ ] 28.10.2026 — "הנוב המרכזי צריך להיות הדבר הראשון של A, שזה Pinch, Lift, Stays" → [fixed, DEV .343: Iron A = IRON (c622323); the rest already had the main knob on A; Delay's hero TIME is a stepped choice, stays off the hand] every plugin's rack.json: slot A = the plugin's main knob. Not on A today: Rise (INTENSITY, not RISE), Stutter (FADE OUT), 1176 (INPUT), 2A, Clipper, Sidechain, Gater, Grain, Delay to check. Moment plugins (Throw, Stucker, RR ARM, Iron) keep A = return by design (recommended, Liran to confirm).
- [ ] 28.10.2026 — "כשאני מחליף בין פלאגינים ופותח את הקונטרול של פלאגין אחר, הוא מבקש ממני ברקוד, ואם אני לוחץ על קאמרה וחוזר לפון פתאום זה מסתדר ... המצלמה כבר פתוחה באייפון" → a rack opened while the phone is already streaming shows the QR; switching CAMERA→PHONE re-reads the link. The rack page must ask for the phone/link state when it opens (and on pairing events) instead of waiting for a source switch. [fix in DEV .343 (mono a37ca01): cause = every plug-in module became its own link owner in one FL (link.log: "link started" at each rack); claim now per process + peer phone counts as connected. WAITS for Liran's FL check]
- [ ] 28.10.2026 — "כשפותחים את הרק, הפלאגין נהיה קטן. זה קורה רק בפעם הראשונה" (Flanger, Throw, AFTERSPACE screenshots; "כולם יש את הבעיה הזאת חוץ מהרייז") → FL Studio Windows: the first rack open shrinks the plugin's face to the left (Throw: a black unpainted band between face and rack); close + reopen is fine. Every rack plugin except Rise. [fix in DEV .343 (mono a37ca01): the grow check gave up after 180 ms while FL's first open builds the rack WebView2; now ~2 s + late grant back to GROW. WAITS for Liran's FL check]

## Done
