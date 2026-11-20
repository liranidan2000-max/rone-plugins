# RONE Plugins Center — work notes

## Open

## Done
- [x] 20.11.2026 — "לא ראו בסנטר אצלו את הפלאגינים ב-3D ... המודעה היא בלי כלום" (friend's MacBook Pro 14 M4 Pro, macOS Sequoia 15.6.1 = WebKit 18.6; Liran's MacBook Air M4 on macOS 26 never showed it) → Library cards show no 3D unit and the TIP OF THE WEEK banner's picture side is empty (grey glow only). **Done in 2.1.40 (monorepo 6fea41e, RELEASED 20.11, run #430): the card units DID draw on his Mac (faint at 20 % by design, visible in the photo); only the banner unit was missing. Inferred cause: the Mac Core Animation compositor of WebKit 18 with preserve-3d + fill-both entrance + drop-shadow + mix-blend sheen on one image; the banner is now three plain layers. Also all platforms: the card unit never came up under the mouse (fill both overrode :hover) - fixed. Verified Chromium + WebKit 18.2 (Windows); macOS 15 itself not run.**
