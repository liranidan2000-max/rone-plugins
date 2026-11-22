// Pure data (no window): node tests read it, test/heroSlides.test.mjs.
// Dev mode: the console's Center banners as they went live on 22.11.2026 (roneaudio.com/admin#banners).
const MEDIA = 'https://roneaudio.com/api/v1/media/'
const POSTER = 'https://roneaudio.com/graphics/center-banners/'
const DAY = 86400000
export const mockBanners = [
  { id: 'deal:bf2026', kind: 'deal', audiences: ['guest', 'free', 'buyer'], product: null, show_if: 'any', priority: 90,
    eyebrow: 'Black Friday · 23.11 – 1.12', title: 'All 13 plugins for life', body: '', price: '$129', was: '$538', accent: '#FFD02B',
    cta: 'url:/black-friday', cta_label: 'Get all 13 · $129', cta2: 'url:/black-friday#plugins', cta2_label: 'Half price on one',
    opens_at: Date.parse('2026-11-22T22:00:00Z'), ends_at: Date.parse('2026-12-01T22:00:00Z') },
  { id: 'film:hook', kind: 'film', audiences: ['guest', 'free', 'buyer', 'pass'], product: 'RoneHook', show_if: 'missing', priority: 70,
    eyebrow: 'New plugin · free with 3 RONE plugins', title: 'RONE Hook', body: 'Search a word, hear every moment it is said, clean it and play it from your keys.',
    video: MEDIA + 'banner-film-hook-b06f6c9d6f1b.mp4', poster: POSTER + 'film-hook.jpg', accent: '#FF8A7A', cta: 'plugin', cta2: 'video', cta2_label: 'Watch it work' },
  { id: 'film:reverb', kind: 'film', audiences: ['guest', 'free', 'buyer', 'pass'], product: 'RoneReverb', show_if: 'missing', priority: 65,
    eyebrow: 'New drop', title: 'RONE Reverb', body: 'A light, clean reverb with a picture of what it does: MIX, DECAY up to 50 s, and RONE Control inside.',
    video: MEDIA + 'banner-film-reverb-9c462d4a15a8.mp4', poster: POSTER + 'film-reverb.jpg', accent: '#FFAFC8', cta: 'plugin', cta2: 'video', cta2_label: 'Watch it work' },
  { id: 'action:sidechain', kind: 'action', audiences: ['guest', 'free', 'buyer'], product: 'RoneSidechain', show_if: 'missing', priority: 62,
    eyebrow: 'Free · RONE Sidechain', title: 'Draw your own duck curve', body: 'Drag a flat line or a step and the bass breathes differently at once. Every kick restarts the curve.',
    video: MEDIA + 'banner-act-sidechain-v2-23ca26216491.mp4', poster: MEDIA + 'banner-act-sidechain-v2-23ca26216491.jpg', accent: '#7B8CFF', cta: 'plugin', cta2: 'video', cta2_label: 'Watch the guide' },
  { id: 'action:stutter', kind: 'action', audiences: ['guest', 'free', 'buyer'], product: 'RoneStutter', show_if: 'locked', priority: 55,
    eyebrow: 'See it work · RONE Stutter', title: 'A filter sweep inside every fill', body: 'The arrows choose up or down, FILTER CURVE shapes the sweep, and it is saved with your project.',
    video: MEDIA + 'banner-act-stutter-v2-53b9f2263e43.mp4', poster: MEDIA + 'banner-act-stutter-v2-53b9f2263e43.jpg', accent: '#FFD02B', cta: 'plugin', cta2: 'video', cta2_label: 'Watch the guide' },
  { id: 'tip:1176-scope', kind: 'tip', audiences: ['buyer', 'pass'], product: 'Rone1176', show_if: 'owned', priority: 50,
    eyebrow: 'Tip of the week · RONE 1176', title: 'SCOPE shows what the compressor did', body: 'BARS sets how much of the song you see. HOLD freezes the picture, so you can read every hit.',
    video: MEDIA + 'banner-act-1176-v2-9acb16d99d76.mp4', poster: MEDIA + 'banner-act-1176-v2-9acb16d99d76.jpg', accent: '#FF8C2E', cta: 'video', cta_label: 'Watch the guide', cta2: 'manual', cta2_label: 'Open the manual',
    ends_at: Date.now() + 7 * DAY },
  { id: 'tip:stutter-filter', kind: 'tip', audiences: ['buyer', 'pass'], product: 'RoneStutter', show_if: 'owned', priority: 48,
    eyebrow: 'Did you know · RONE Stutter', title: 'A filter sweep inside every fill', body: 'The arrows choose up or down, FILTER CURVE shapes the sweep, and it is saved with your project.',
    video: MEDIA + 'banner-act-stutter-v2-53b9f2263e43.mp4', poster: MEDIA + 'banner-act-stutter-v2-53b9f2263e43.jpg', accent: '#FFD02B', cta: 'video', cta_label: 'Watch the guide', cta2: 'manual', cta2_label: 'Open the manual' },
  { id: 'soon:bench', kind: 'soon', audiences: ['guest', 'free', 'buyer', 'pass'], product: null, show_if: 'any', priority: 20,
    eyebrow: 'On the bench', title: 'Three new units are coming', body: 'RONE EQ, RONE Filter and RONE Panorama. Join the list and hear the day each one ships.',
    word: 'RONE EQ', accent: '#B07CFF', cta: 'optin', cta_label: 'Tell me first' },
]
