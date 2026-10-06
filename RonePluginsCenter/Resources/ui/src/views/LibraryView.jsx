import React from 'react'
import Hero from '../components/Hero'
import PluginCard from '../components/PluginCard'
import { Icon } from '../components/ui'
import { CATEGORY_KEYS, categoriesOf, matchesSearch, shortName, sortPlugins, updatable } from '../catalog'

function Skeleton () {
  return (
    <div className="rounded-[14px] border border-rone-border bg-rone-card p-3.5 flex flex-col gap-3" aria-hidden="true">
      <div className="flex gap-3"><div className="skeleton w-14 h-14 rounded-[14px]" /><div className="flex-1 flex flex-col gap-2 pt-1"><div className="skeleton h-3.5 w-2/3 rounded" /><div className="skeleton h-3 w-1/3 rounded" /></div></div>
      <div className="skeleton h-3 w-full rounded" /><div className="skeleton h-3 w-4/5 rounded" />
      <div className="skeleton h-8 w-full rounded-[9px] mt-2" />
    </div>
  )
}

export default function LibraryView ({ t, plugins, loading, access, account, license, preview, tips, reduced, query, setQuery,
                                       cat, setCat, sort, setSort, catLabel, onNavigate, onUpdateAll, onRefresh, handlers }) {
  const ups = updatable(plugins, access)
  const filtered = sortPlugins(plugins.filter(p => (cat === 'all' || categoriesOf(p).includes(cat)) && matchesSearch(p, query, catLabel)), access, sort)
  const count = (c) => c === 'all' ? plugins.length : plugins.filter(p => categoriesOf(p).includes(c)).length

  return (
    <div className="flex flex-col">
      {!query && (
        <Hero t={t} plugins={plugins} access={access} account={account} license={license} preview={preview} tips={tips} reduced={reduced}
              onInstall={handlers.onInstall} onDetail={handlers.onDetail} onAccount={() => onNavigate('account')} />
      )}

      {ups.length > 0 && !query && (
        <div className="mt-3.5 flex items-center gap-3 h-[46px] pl-4 pr-2 rounded-[12px] border border-[rgba(255,208,43,.28)] bg-[rgba(255,208,43,.05)]">
          <span className="w-2 h-2 rounded-full bg-rone-amber" style={{ boxShadow: '0 0 8px rgba(255,208,43,.6)' }} />
          <span className="text-[13.5px] font-bold truncate">
            {ups.length === 1 ? t('upd.one') : t('upd.many', { n: ups.length })}
            <span className="text-rone-text-dim font-semibold"> · {ups.map(p => shortName(p) + ' ' + (p.remoteVersion || '').split('.').slice(0, 3).join('.')).join(', ')}</span>
          </span>
          <span className="flex-1" />
          <button className="btn btn-ghost btn-sm" onClick={() => onNavigate('updates')}>{t('upd.details')}</button>
          <button className="btn btn-pri btn-sm" style={{ '--acc': '#FFD02B' }} onClick={onUpdateAll}>
            <Icon.download className="w-3 h-3" />{t('upd.button', { n: ups.length })}
          </button>
        </div>
      )}

      <div className="flex items-center gap-2 mt-4 mb-3 flex-wrap">
        <div className="flex items-center gap-1.5 flex-wrap" role="tablist" aria-label="Categories">
          {['all', ...CATEGORY_KEYS].map(c => (
            <button key={c} role="tab" aria-selected={cat === c} onClick={() => setCat(c)}
                    className={`h-[32px] px-3 rounded-[9px] border text-[12.5px] font-bold inline-flex items-center gap-1.5 whitespace-nowrap
                                ${cat === c ? 'bg-rone-purple border-rone-purple text-rone-neon-dark' : 'border-rone-border text-rone-text-dim hover:text-rone-text-primary hover:border-rone-border-3'}`}>
              {catLabel(c)} <b className="font-mono text-[11px] opacity-75">{count(c)}</b>
            </button>
          ))}
        </div>
        <span className="flex-1" />
        <label className="flex items-center gap-2 text-[12px] text-rone-text-faint font-semibold">
          {t('lib.sort')}
          <select value={sort} onChange={(e) => setSort(e.target.value)}
                  className="h-[30px] px-2 rounded-[8px] bg-rone-card border border-rone-border text-[12px] font-bold text-rone-text-secondary">
            <option value="yours">{t('lib.sortYours')}</option>
            <option value="new">{t('lib.sortNew')}</option>
            <option value="name">{t('lib.sortName')}</option>
          </select>
        </label>
        <span className="inline-flex border border-rone-border rounded-[9px] p-[2px]">
          <button className="h-[26px] px-2.5 rounded-[7px] text-[12px] font-bold inline-flex items-center gap-1.5 bg-rone-drawer text-rone-text-primary" aria-pressed="true">
            <Icon.grid className="w-3.5 h-3.5" />{t('lib.grid')}
          </button>
          <button className="h-[26px] px-2.5 rounded-[7px] text-[12px] font-bold inline-flex items-center gap-1.5 text-rone-text-dim hover:text-rone-text-primary"
                  onClick={() => onNavigate('rack')}>
            <Icon.rack className="w-3.5 h-3.5" />{t('lib.rack')}
          </button>
        </span>
      </div>

      {/* Three cards a row at the Center's opening size, up to five on a wide screen */}
      <div className="grid gap-3.5" style={{ gridTemplateColumns: 'repeat(auto-fill, minmax(286px, 1fr))' }}>
        {loading && plugins.length === 0
          ? [...Array(6)].map((_, i) => <Skeleton key={i} />)
          : filtered.map(p => (
              <PluginCard key={p.id} t={t} plugin={p} access={access} preview={preview} catLabel={catLabel} {...handlers} />
            ))}
      </div>

      {!loading && plugins.length > 0 && filtered.length === 0 && (
        <div className="mt-6 p-10 rounded-[14px] border border-dashed border-rone-border-2 text-center text-rone-text-dim">
          {t('lib.noResults', { q: query })}
          <div className="mt-3"><button className="btn btn-out btn-sm" onClick={() => { setQuery(''); setCat('all') }}>{t('lib.clear')}</button></div>
        </div>
      )}
      {!loading && plugins.length === 0 && (
        <div className="mt-6 p-10 rounded-[14px] border border-dashed border-rone-border-2 text-center text-rone-text-dim">
          {t('lib.empty')}
          <div className="mt-3"><button className="btn btn-out btn-sm" onClick={onRefresh}>{t('lib.retry')}</button></div>
        </div>
      )}
    </div>
  )
}
