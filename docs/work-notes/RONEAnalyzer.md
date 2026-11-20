# RONE Analyzer — work notes

## Open

## Done
- [x] 20.11.2026 — "שמתי Analyzer Channel על ערוץ אחר ... רואה אותו בכתום בספקטרום, מעולה ... בפלוס של הווקטורסקופ ... כרגע הוא מציג רק את הרפרנס ... שיהיה לי בחירה ... של הצ'אנלים או של הרפרנס עצמו" → the second vectorscope (+ REFERENCE) shows only a loaded reference: give it a source list = every connected Analyzer Channel or the reference. **Done in 1.3.55 (RONEAnalyzer a1c0477, NOT released): source picker in the second scope header - the reference file or any live Channel (colour, slot, idle), saved, falls back and returns. His photo had NO reference loaded: the orange REF chip was a Channel named REF. Also fixed: on every Mac since 1.3.44 the second scope was empty (JUCE 8 on macOS drops the ?query, the source now rides in the path). 4 live Channels end to end, 425 unit tests.**
