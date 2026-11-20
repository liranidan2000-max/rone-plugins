# RONE Hook — work notes

RONE Hook 1.0.0.404 was released 15.11.2026: free, Windows only, through the Center (monorepo c6e0929, CI #404,
site 3670b96 / Worker 3913c6f6). Liran (15.11): "תאסוף את כל המשימות שנותר לעשות לו ... ותשמור את זה בצד,
כי אני רוצה לעשות עוד כמה בדיקות אחרונות ... ורק אז נעשה את הכל". So: his last checks come first, then the
whole list below in one go.

## WHO GETS IT (RULE, Liran 15.11): a gift for people with 3 products

"שהפלאגין הזה יהיה במתנה רק למי שיש לו 3 פלאגינים ומעלה. 3 פלאגינים שנקנו בכסף, לא חינמים."
THE message everywhere (site, Center, plug-in, posts, reels, ads, mails): **Free plugin for people with 3 products.**
- Opens for: accounts with 3+ distinct RONE plugins bought with money (Paddle / Lemon Squeezy), and paying
  ALL ACCESS subscribers past the 14-day trial (his answer 15.11).
- Does NOT count / does not open: free plugins (Clipper, Sidechain), gifts, giveaway passes, comp passes,
  the 14-day trial, refunded or disabled rows, RONE VST MANAGER. Never sold on its own.
- Where it lives: Worker `worker/_lib/catalog.js` (giftForPaid: 3) + `auth.js` (paidPlugins, paidPass),
  tests/hook-gift.test.mjs; versions.json `gift_min_paid: 3`; Center 2.1.33 (owned only, never ALL ACCESS
  alone); HOOK 1.0.1 checks `isProductOwnedApart("RoneHook")` in its signed licence and shows a lock.
- Whoever installed 1.0.0 free on 15.11 is locked by the 1.0.1 update (his answer).
- Centers up to 2.1.32 still show the card open to any ALL ACCESS holder (comp/trial too); the plug-in itself
  stays locked for them.

## Liran's last checks (write down what he finds here before starting the list)

- [ ] (his notes from the last tests go here)

Things only he can check, if they are not already covered:
- FIT TO TRACK and ONE NOTE heard in a DAW (lands on the bar, in key, in tempo)
- Drag into FL Studio / Ableton / Cubase: the voice, a WORD CHOP, MARKS
- WORD CHOPS' MIDI map (C3 up) playing in a sampler
- His ears on the separated voice and on CLEAN / CHARACTER / REVERSE TAIL / STUTTER END
- Install from the Center on a clean machine: the 197 MB installer, SmartScreen, first open READY

## Open: the rest of the release (do all of it after his checks)

### Look (waits for his OK on coral #FF8A7A; the UI is already coral)
1. Re-render the 3D unit in coral: `WEBSITE RONE AUDIO/scripts/unit-render/render.py --unit rone-hook`
   (recipe in units.json + poses/rone-hook.js). Check `work/rone-hook-check.png` beside Flanger.
2. Then the files made from it: graphics/renders, cutouts (+600), og card, site icon, Center
   `RonePluginsCenter/Resources/units/unit_RoneHook.webp`.
3. The plugin icon `RoneHook/Resources/AppIcon.png` and the Center icon `RonePluginsCenter/Resources/RoneHook_icon.png`
   are still red.

### Manuals
4. Re-shoot the PDF manual: the window is 1000x720 now (shoot.py says 860x700), coral, and it has nothing added since
   1.0.0.5. Missing: SAID / MEANING / VIDEOS, IDEAS, the STANDS ALONE / LOW / HIGH VOICE chips, SORT, the
   arrows playing each moment, CLEAN, TAKE IT = only the moment + WHOLE VIDEO, FIT TO TRACK in the finder, local
   files / OPEN A FILE, the source card (COPY CREDIT / OPEN VIDEO / CC), the search meter, the "slow down" wait.
   `python docs/manuals/tools/pipeline.py --only hook`. The new PDF rebuilds and releases HOOK (CI path filter).
5. Web manual `/manuals/rone-hook/` (`WEBSITE RONE AUDIO/scripts/build-web-manuals.py`), the manuals index,
   sitemap, llms.txt manual line, and the USER MANUAL · ONLINE link on the product page.
6. Copy the new PDF to the site's `manuals/pdf/rone-hook.pdf`.

### Site
7. Homepage hero rail: `assets/fx.js`, the RENDERS list, a PLUGINS entry, PRICE/PADDLE as free, and an engine branch
   with a real-behaviour demo (a voice coming out of a mix?) (see the site-clips note in roneaudio-deploy-protocol).
8. Its phone clip `media/shorts/RoneHook.mp4` + `.jpg` (`קידום/youtube/_tools/site_shorts.py RoneHook`).
9. A real screenshot in a session for the product page (`graphics/screens/rone-hook-in-<daw>.webp`, like Clipper's).
10. Pricing page: decide whether the free plugins are listed there; if yes, add HOOK.
11. Admin console: the HOOK unit in `assets/admin-faces.js` (a new plugin = a CUT line).
12. `assets/account.js` PRODUCTS row, if the account page should show it.
13. `scripts/unit-render/site-og.html`: count + icon, then regenerate og-image.jpg.
14. Press kit (optional).
15. Deploy through `scripts/stage-site.ps1` (15.11 went straight from the site folder: 10 minutes, against the rule).

### Plugin
16. An update notice in the plugin (the Shared RoneUpdatePrompt like the other plugins). HOOK has none, so a user
    only hears about 1.0.1 from the Center.
17. Other machines: AMD and Intel graphics, no graphics card (CPU, about 4x slower), Windows 10.
18. YouTube "slow down" (13.11): watch `%APPDATA%\RonePlugins\Hook\youtube.log` from real users. If it repeats, a
    PO-token provider or a per-search budget. Cookies from the browser would put the user's Google account at risk,
    so ask first.
19. Ideas still open: drag the words' edges on the waveform, re-fit live when the key changes, level match to the
    track, drag a file onto the window, TikTok / Instagram links (not tested), our own caption index (words beyond
    20 videos).
20. Two copies of the installer script: `installer/RoneHook.iss` (CI) and `RoneHook/installer/RoneHook.iss` (local/dev).
    Keep them in step, or point the dev build at the monorepo one.

### Trust and distribution
21. Code signing: once the certificate exists, sign the payload too (RoneHookEngine.exe, node.exe, ffmpeg.exe,
    qjs.exe, yt-dlp.exe, python). CI's sign step only signs `RoneHook_artefacts`. Scan the built installer with
    Defender (not done yet).
22. The Chrome extension: a Chrome Web Store developer account + publishing (`RoneHook/extension/store/` is ready).
    The store may refuse it as a downloader.
23. A new runtime (yt-dlp, ffmpeg, a model): runtime-<N+1> as described in `RoneHook/tools/fetch_runtime.py`.
    yt-dlp updates itself every day on users' machines anyway.

### Mac
24. Mac version: a separate project. The engine is DirectML; the browser history uses winsqlite3; the installer
    uses WMI. Until then Center 2.1.32+ hides HOOK on a Mac. An older Mac Center shows the card with
    "being published".

### Launch and marketing
25. The decisions in the launch mockup are empty (https://claude.ai/artifact/DE6yJgeZEE8LtH3UQemAA6: channel,
    giveaway, upsell, mac, when, reel hook). Decide them.
26. Launch on Instagram / YouTube through the social-brand skill (mockup first), a guide video, a reel. RULES:
    HOOK is never sold through Paddle (its AUP bans streaming downloaders), and no ad ever says
    "download from YouTube". Every post, reel, ad and mail carries "Free plugin for people with 3 products."
    - it is a reason to buy a third plugin (warm audience / owners of 1-2), not a free giveaway to everyone.
    The launch mockup's "giveaway" decision is settled by this: no giveaway.
27. Update the Kalifa plugin board.

## Done
- [x] 20.11.2026 — "לא מצאתי כפתור ששומר את זה לבנקים ... אני עובד על איזה הוק עכשיו, אני רוצה לשמור אותו לבנק, ולהשתמש בו אחר כך" (friend's MacBook Pro 14 M4 Pro, macOS Sequoia 15.6.1 = WebKit 18.6; Liran's MacBook Air M4 on macOS 26 never showed it) → no way to save the hook being worked on (the selected region of the source) into the bank for later use: add SAVE TO BANK on the work screen. **Done in 1.0.6 (RoneHook e942b9e, RELEASED 20.11, run #430): SAVE TO BANK key under DRAG TO DAW (K), editable name, same file as DRAG TO DAW, no silent duplicates (IN THE BANK), RENAME in the bank. Also fixed: HookBank::load() read freed JSON, so the bank list came back empty on every reopen (his bank.json = [] beside 2 orphan wavs from 8.11). Tested end to end on the real standalone; Mac not run.**

- 15.11: the release itself: private repo, submodule, CI `hook` Windows-only, runtime prerelease RoneHook-runtime-1,
  versions.json, Center 2.1.32 (hides on Mac), product page + homepage card + Worker free entry.
- 15.11: turned into a gift for people with 3 products: Worker + site (site c5835d4, Worker f3794f17), HOOK 1.0.1
  lock (RoneHook 3a2376a), Center 2.1.33 + versions.json (monorepo 93cba37, CI #405).
