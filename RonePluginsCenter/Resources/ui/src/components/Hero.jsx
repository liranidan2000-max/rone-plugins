import React, { useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react'
import { Icon } from './ui'
import { openExternal } from '../bridge'
import { buildSlides, clockOf, durationOf, hideSlide, readHidden } from '../heroSlides'

// The banner: the Center's own slides (heroSlides.js) and the console's (roneaudio.com/admin#banners) - launch films,
// the plugin working in a short clip, deals with a clock, tips in video, the rack this person owns. Liran 22.11.2026:
// "ads that do not bother, a level up". So: a slide's time fills its dot; the mouse on the banner holds it; a video
// is muted, loads only when its slide is up and stops when the window is away; "Hide for 14 days" on every ad;
// reduced motion = still pictures and no rotation. The unit turns slowly with a light sweeping across it (index.css .hero-*).

const EVENTS = 'https://roneaudio.com/api/v1/banner'
const seen = new Set()   // a console banner counts one view per Center session
function sendEvent (id, type) {
  try {
    // A "simple" request (text/plain, no-cors): no preflight from the WebView, nothing to read back.
    fetch(EVENTS, { method: 'POST', mode: 'no-cors', keepalive: true, headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify({ id, type }) }).catch(() => {})
  } catch {}
}

const BtnIcon = { download: Icon.download, play: Icon.play }

function Clock ({ t, slide }) {
  const [now, setNow] = useState(Date.now())
  useEffect(() => { const id = setInterval(() => setNow(Date.now()), 1000); return () => clearInterval(id) }, [])
  const c = clockOf(slide, now)
  if (!c) return null
  const labels = [t('hero.days'), t('hero.hrs'), t('hero.min'), t('hero.sec')]
  return (
    <div className="flex items-center gap-1.5" aria-label={c.before ? t('hero.opensIn') : t('hero.endsIn')}>
      <span className="text-[10px] font-extrabold tracking-[0.16em] uppercase mr-1.5" style={{ color: slide.color }}>{c.before ? t('hero.opensIn') : t('hero.endsIn')}</span>
      {c.parts.map((v, k) => (k === 0 && v === 0 ? null : (   // no "00 days" on the last day
        <div key={k} className="hero-clock-box">
          <b>{String(v).padStart(2, '0')}</b><i>{labels[k]}</i>
        </div>
      )))}
    </div>
  )
}

/** The slide's moving picture: a muted loop that plays only while it is the slide on screen and the window is here. */
function Clip ({ src, poster, playing, onFail, onShape, className }) {
  const ref = useRef(null)
  useEffect(() => {
    const v = ref.current
    if (!v) return
    if (playing) { const p = v.play(); if (p && p.catch) p.catch(() => {}) } else v.pause()
  }, [playing, src])
  return <video ref={ref} className={className} src={src} poster={poster || undefined} muted loop playsInline preload="metadata"
                disablePictureInPicture onError={onFail} aria-hidden="true"
                onLoadedMetadata={(e) => { const v = e.currentTarget; if (v.videoWidth && v.videoHeight) onShape?.(v.videoWidth / v.videoHeight) }} />
}

export default function Hero ({ t, plugins, app, access, account, license, personal, optinAsk, onBuy, tips, reduced, onInstall, onDetail, onAccount, onTryAll,
                                banners, ups, onUpdateAll, onNavigate, onManual, onOptin, optinOn }) {
  const [hidden, setHidden] = useState(() => readHidden())
  const slides = useMemo(() => buildSlides({
    t, plugins, app, access, account, license, personal, optinAsk, tips, banners, hidden, ups, hasTrial: !!onTryAll, optinOn,
  }), [t, plugins, app, access, account, license, personal, optinAsk, tips, banners, hidden, ups, onTryAll, optinOn])

  const [i, setI] = useState(0)
  const [hover, setHover] = useState(false)
  const [away, setAway] = useState(() => typeof document !== 'undefined' && document.hidden)
  const [broken, setBroken] = useState({})
  const [shape, setShape] = useState({})   // video url -> its width / height, so the screen is the clip's own shape
  useEffect(() => { if (i >= slides.length) setI(0) }, [slides.length])

  // The words always fit the banner (Liran 22.11.2026: a two-line title was squeezed under the body and the
  // body cut mid-word). After every layout the copy is measured; while something does not fit - the column runs
  // past the banner, or the title or the body is clamped - it steps down one size: 30 px title, 26, 23 (smaller
  // body and padding), 21 with no body. A new slide, a resize or the fonts arriving start again from the top.
  const sectionRef = useRef(null)
  const copyRef = useRef(null)
  const [tick, setTick] = useState(0)
  const [bw, setBw] = useState(0)   // the banner's width
  const hasSlides = slides.length > 0
  useEffect(() => {
    const el = sectionRef.current
    if (!el) return
    let w = 0
    let ro = null
    if (typeof ResizeObserver !== 'undefined') {
      ro = new ResizeObserver((es) => { const nw = Math.round(es[0].contentRect.width); if (nw !== w) { w = nw; setBw(nw); setTick(x => x + 1) } })
      ro.observe(el)
    }
    let live = true
    try { document.fonts?.ready?.then(() => { if (live) setTick(x => x + 1) }) } catch {}
    return () => { live = false; ro?.disconnect() }
  }, [hasSlides])
  const [fit, setFit] = useState({ k: '', n: 0 })

  // A window that was away for a long time (overnight, minimised, behind the DAW) can come back with the unit
  // never painted again (22.11.2026, RONE Listen): after a minute away the pictures are mounted afresh and a
  // picture that failed meanwhile gets another try. While away, videos stop and the slide holds.
  const [wake, setWake] = useState(0)
  useEffect(() => {
    let since = document.hidden ? Date.now() : 0
    const leave = () => { if (!since) since = Date.now(); setAway(true) }
    const back = () => {
      if (since && Date.now() - since > 60000) { setWake(w => w + 1); setBroken({}) }
      since = 0
      setAway(false)
    }
    const onVisibility = () => (document.hidden ? leave() : back())
    document.addEventListener('visibilitychange', onVisibility)
    window.addEventListener('blur', leave)
    window.addEventListener('focus', back)
    return () => {
      document.removeEventListener('visibilitychange', onVisibility)
      window.removeEventListener('blur', leave)
      window.removeEventListener('focus', back)
    }
  }, [])

  const s = slides.length ? slides[Math.min(i, slides.length - 1)] : null
  useEffect(() => { if (s?.server && !seen.has(s.bannerId)) { seen.add(s.bannerId); sendEvent(s.bannerId, 'view') } }, [s?.key])
  const clipShape = s?.video ? shape[s.video] || 0 : 0
  const fitKey = `${s?.key}|${tick}|${clipShape}`
  const level = fit.k === fitKey ? fit.n : 0
  useLayoutEffect(() => {
    const el = copyRef.current
    if (!el || level >= 3) return
    // Layout boxes only (offsetTop / clientHeight): the entrance animation's transform must not count as overflow.
    const last = el.lastElementChild
    const room = el.clientHeight - (parseFloat(getComputedStyle(el).paddingBottom) || 0)
    const runsOver = last && last.offsetTop + last.offsetHeight > room + 1
    // A clamped line is a whole line; the font's own ascent and descent can poke a few px past a tight leading.
    const clamped = (x) => x && x.scrollHeight - x.clientHeight > (parseFloat(getComputedStyle(x).lineHeight) || 20) / 2
    if (runsOver || clamped(el.querySelector('h2')) || clamped(el.querySelector('p'))) setFit({ k: fitKey, n: level + 1 })
  })

  if (!s) return null
  const go = (n) => setI(((n % slides.length) + slides.length) % slides.length)
  const next = slides.length > 1 ? slides[(i + 1) % slides.length] : null
  const rotate = !reduced && slides.length > 1
  // The window being away holds the slide too (it is not "blur" alone: a click in the DAW should not count as watching).
  const holding = hover || away
  const playing = !reduced && !holding

  const run = (a) => {
    if (!a || !a.act) return
    if (a.server && s.server) sendEvent(s.bannerId, 'click')
    const x = a.act
    const plugin = x.id ? plugins.find(p => p.id === x.id) : null
    if (x.type === 'install') onInstall?.(x.id)
    else if (x.type === 'detail') onDetail?.(x.id)
    else if (x.type === 'buy') { if (plugin) onBuy?.(plugin, x.src || 'banner') }
    else if (x.type === 'url') openExternal(x.url)
    else if (x.type === 'manual') { if (plugin) onManual?.(plugin) }
    else if (x.type === 'optin') onOptin?.(true)
    else if (x.type === 'optinAnswer') optinAsk?.(x.yes)
    else if (x.type === 'trial') onTryAll?.()
    else if (x.type === 'updates') onNavigate?.('updates')
    else if (x.type === 'updateAll') onUpdateAll?.()
    else if (x.type === 'account') onAccount?.()
  }
  const hide = () => {
    if (s.server) sendEvent(s.bannerId, 'hide')
    setHidden(hideSlide(s.key))
  }

  const video = s.video && !broken[s.video] ? s.video : null
  const unit = s.unit && !broken[s.unit] ? s.unit : null
  const fail = (u) => () => setBroken(b => ({ ...b, [u]: true }))
  const film = s.kind === 'film' && video
  const screen = (s.kind === 'action' || s.kind === 'tip') && video
  const showUnit = !film && !screen && unit && (s.kind === 'unit' || s.kind === 'film' || s.kind === 'action' || s.kind === 'tip')
  const wide = !film && !screen && !showUnit && !s.rack && s.kind !== 'deal' && s.kind !== 'coll'
  // The clip's screen: 160 px high, as wide as the clip's own shape; the words stop before it.
  const screenW = screen ? Math.round(160 * (shape[video] || 1.65)) : 0
  const copyMax = screen ? `calc(100% - ${screenW + 60}px)` : undefined
  // A narrow banner (the smallest Center window, 22.11.2026): the price row and the clock did not fit next to a
  // 44 % picture and the buttons fell under the banner. There the words get 64 % and the picture 34 %, 3 units.
  const narrow = bw > 0 && bw < 860
  const roomy = narrow && (s.kind === 'deal' || showUnit)
  const fan = s.kind === 'deal' ? plugins.filter(p => !p.free && p.kind !== 'app' && p.unitUrl && !broken[p.unitUrl]).slice(0, narrow ? 3 : 5) : []
  // left %, top px, turn; each unit is at most --mw wide, so the last one ends inside the banner (22.11.2026: it was cut).
  const fanAt = narrow ? [[0, 40, -6], [27, 18, 0], [55, 46, 7]] : [[0, 40, -8], [16, 16, -3], [32, 52, 2], [48, 20, 6], [64, 46, 10]]

  return (
    <section ref={sectionRef} className={`hero relative h-[220px] rounded-[18px] overflow-hidden border border-rone-border-2 flex-shrink-0 ${holding ? 'held' : ''}`}
             style={{ background: 'linear-gradient(160deg, #1B1E23, #0D0F12)', '--c': s.color }} aria-roledescription="carousel"
             onMouseEnter={() => setHover(true)} onMouseLeave={() => setHover(false)}>
      <div className="hero-glow absolute inset-0" style={{ background: `radial-gradient(55% 110% at 78% 50%, ${s.color} 0%, transparent 62%)` }} />
      <div className="absolute inset-0" style={{ background: 'radial-gradient(90% 160% at 85% 50%, rgba(255,255,255,.06) 0%, transparent 55%)' }} />

      {film && (
        <div key={s.key + wake} className={`hero-film ${reduced ? '' : 'hero-media-in'}`} aria-hidden="true">
          <Clip src={video} poster={s.poster} playing={playing} onFail={fail(video)} />
        </div>
      )}
      {/* The words over a bright film were hard to read (22.11.2026): a dark veil under the words only. */}
      {film && <div className="hero-scrim" aria-hidden="true" />}
      {screen && (
        <div key={s.key + wake} className={`hero-screen ${reduced ? '' : 'hero-media-in'}`} style={{ '--c': s.color, width: screenW }} aria-hidden="true">
          <div className="hero-glass">
            <Clip src={video} poster={s.poster} playing={playing} onFail={fail(video)}
                  onShape={(r) => setShape(m => (Math.abs((m[video] || 0) - r) < 0.01 ? m : { ...m, [video]: r }))} />
          </div>
          {/* A tip already says so above its title; a clip of a plugin you do not have gets the tag. */}
          {s.kind === 'action' && <span className="hero-tag">{t('hero.liveTag')}</span>}
        </div>
      )}
      {s.rack && (
        <div key={s.key + wake} className="absolute right-0 top-0 bottom-0 w-[42%] pointer-events-none overflow-hidden" aria-hidden="true">
          <div className={reduced ? 'hero-rack still' : 'hero-rack'}>
            {[...s.rack, ...(reduced ? [] : s.rack)].map((u, k) => (
              <div key={u.id + k} className="hero-rack-cell" style={{ '--d': `${(k % s.rack.length) * -0.55}s`, '--c': u.color }}>
                <img src={u.url} alt="" className="hero-rack-unit" onError={(e) => { e.currentTarget.parentElement.style.display = 'none' }} />
              </div>
            ))}
          </div>
          <div className="absolute inset-0" style={{ background: 'linear-gradient(90deg, #15171B 0%, rgba(21,23,27,0) 22%, rgba(13,15,18,0) 85%, #0D0F12 100%)' }} />
        </div>
      )}
      {s.kind === 'deal' && fan.length > 0 && (
        <div key={s.key + wake} className={`hero-fan ${reduced ? 'still' : 'hero-media-in'}`} style={narrow ? { width: '34%', '--mw': '45%' } : { '--mw': '36%' }} aria-hidden="true">
          {fan.map((p, k) => (
            <img key={p.id} src={p.unitUrl} alt="" style={{ left: `${fanAt[k][0]}%`, top: fanAt[k][1], '--r': `${fanAt[k][2]}deg`, animationDelay: `${-k * 0.8}s`, zIndex: k === (narrow ? 1 : 2) ? 3 : 1 }}
                 onError={fail(p.unitUrl)} />
          ))}
        </div>
      )}
      {s.kind === 'coll' && (
        <div key={s.key + wake} className={`hero-coll ${reduced ? '' : 'hero-media-in'}`} aria-hidden="true">
          <div className="hero-coll-grid">
            {s.units.map(u => <img key={u.id} src={u.url} alt="" className={u.lit ? 'lit' : ''} style={{ '--g': u.color }} onError={(e) => { e.currentTarget.style.visibility = 'hidden' }} />)}
          </div>
          <div className="hero-coll-bar"><i style={{ width: `${Math.round(s.lit / s.units.length * 100)}%` }} /></div>
          <div className="hero-coll-count"><span>{t('hero.collLit', { n: s.lit, total: s.units.length })}</span><span>{t('hero.collLeft', { n: s.units.length - s.lit })}</span></div>
        </div>
      )}
      {s.kind === 'soon' && s.word && <div className="hero-word" aria-hidden="true">{s.word}</div>}
      {showUnit && (
        <div className={`absolute right-[3%] top-0 bottom-0 ${roomy ? 'w-[34%]' : 'w-[44%]'} flex items-center justify-center pointer-events-none`} aria-hidden="true">
          {/* Entrance, turn and picture are three elements on purpose (index.css .hero-*): the old one-element
              stack drew nothing on macOS 15's WebKit. */}
          <div key={s.key + unit + wake} className={reduced ? '' : 'hero-unit-in'}>
            <div className={reduced ? 'relative' : 'hero-unit-wrap'} style={{ '--unit-mask': `url("${unit}")` }}>
              <img src={unit} alt="" className="hero-unit max-h-[190px] w-auto" style={bw ? { maxWidth: Math.round(bw * (roomy ? 0.34 : 0.44)) } : undefined} onError={fail(unit)} />
              {!reduced && <span className="hero-sheen" />}
              <span className="hero-reflect" style={{ backgroundImage: `url("${unit}")` }} />
            </div>
          </div>
          {s.badge && <span className="hero-badge">{s.badge}</span>}
        </div>
      )}

      <div key={'copy' + s.key} ref={copyRef} style={copyMax ? { maxWidth: copyMax } : undefined}
           className={`relative z-[3] h-full px-8 flex flex-col ${level >= 2 ? 'pt-6 pb-7 gap-1.5' : 'pt-7 pb-8 gap-2'} ${reduced ? '' : 'hero-copy-in'} ${wide ? 'max-w-[86%]' : roomy ? (s.kind === 'deal' ? 'max-w-[70%]' : 'max-w-[64%]') : 'max-w-[56%]'}`} aria-live="polite">
        <div className="shrink-0 text-[11px] font-extrabold tracking-[0.24em] uppercase truncate" style={{ color: s.color, textShadow: `0 0 14px ${s.color}` }}>{s.eyebrow}</div>
        {s.title && <h2 className={`shrink-0 m-0 font-display font-extrabold ${['text-[30px]', 'text-[26px]', 'text-[23px]', 'text-[21px]'][level]} leading-[1.08] tracking-[-0.01em] text-white text-balance line-clamp-2`}>{s.title}</h2>}
        {s.kind === 'deal'
          ? (<div className="shrink-0 flex items-center gap-5 flex-wrap">
               {s.price && <div className="flex items-baseline gap-2 font-display"><b className="text-[30px] font-extrabold text-white">{s.price}</b>{s.was && <s className="text-[17px] font-bold text-rone-text-dim">{s.was}</s>}</div>}
               <Clock t={t} slide={s} />
             </div>)
          : s.body && !(s.title && level >= 3) && <p className={`shrink-0 m-0 text-rone-text-secondary ${s.title ? `${level >= 2 ? 'text-[12.5px] line-clamp-3' : 'text-[13.5px] line-clamp-2'} leading-[1.5]` : 'text-[16px] font-semibold leading-snug text-rone-text-primary line-clamp-3'}`}>{s.body}</p>}
        <div className="shrink-0 mt-auto flex items-center gap-2">
          {s.acts.map(a => {
            const I = a.icon && BtnIcon[a.icon]
            if (a.done) return <span key={a.label} className="text-[12.5px] font-bold text-rone-text-secondary px-2">✓ {a.label}</span>
            return (
              <button key={a.label} onClick={() => run(a)} className={`btn ${a.primary ? 'btn-pri btn-white h-[38px] px-5' : a.ghost ? 'btn-ghost' : 'btn-out'}`}>
                {I && <I className="w-3.5 h-3.5" />}{a.label}
              </button>
            )
          })}
        </div>
      </div>

      {slides.length > 1 && (
        <>
          {/* Bottom right, side by side: on the left edge the arrow covered the first letters (22.11.2026). */}
          <div className="hero-arrows">
            <button className="hero-arrow" onClick={() => go(i - 1)} aria-label={t('hero.prev')}>‹</button>
            <button className="hero-arrow" onClick={() => go(i + 1)} aria-label={t('hero.next')}>›</button>
          </div>
          <div className="absolute left-8 bottom-3 flex gap-1.5 z-[5]">
            {slides.map((x, k) => (
              <button key={x.key} onClick={() => go(k)} aria-label={t('hero.slide', { n: k + 1 })} aria-current={k === i}
                      className="hero-dot" style={{ width: k === i ? 34 : 16, '--c': x.color }}>
                {k === i && rotate && (
                  <i key={s.key + wake} className="hero-dot-fill" style={{ animationDuration: `${durationOf(s)}ms` }} onAnimationEnd={() => go(i + 1)} />
                )}
                {k === i && !rotate && <i className="hero-dot-fill done" />}
                {k < i && <i className="hero-dot-fill past" />}
              </button>
            ))}
          </div>
          {next && <div className="hero-next">{t('hero.upNext')} · <b>{next.title || next.eyebrow}</b></div>}
        </>
      )}
      {!s.fixed && s.key !== 'optin' && (
        <button className="hero-hide" onClick={hide}>{t('hero.hide')} ×</button>
      )}
    </section>
  )
}
