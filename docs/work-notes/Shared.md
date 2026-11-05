# Shared (all plugins) — work notes

## Open

## Done
- [x] 04.11.2026 — "כשפותחים שתיים ... יש מסך שחור לשנייה ואז חוזר ... ה-UI נעלם ... פתאום מסך שחור, פתאום מסך לבן, בכל הפלאגינים כמעט" → almost every WebView plugin: the UI blanks (black or white) and returns, probably with two instances open. Seen in Stutter and RR. **Cause found and measured: RoneWebViewGuard (28.10) hid the whole WebView while the mouse was over any window overlapping the plug-in, so two overlapping Stutter windows blacked one out (webview children visible 0/2). Fixed in Shared/RoneWebViewGuard.h: a partly covered plug-in stays on screen (3/3) and only the invisible overlays pass clicks; a fully covered one still hides (0/2, the FL mixer fix stays). The WebView also starts dark instead of white. Released to all 14 plugins in run #369 on 4.11. NOT tested inside FL, and the overlay path could not be exercised outside FL.**
