#pragma once

// ============================================================================
// RoneUpdatePrompt - "RONE X 1.4.1 is available" inside every RONE plugin.
//
// Liran's decisions (2026-09-23 mockup): the bar appears when the plugin opens,
// LATER hides it for three days, and UPDATE opens the Plugins Center on that
// plugin. From then on the plugin is grey and passes its input through
// untouched, and it asks to be closed and reopened once the update is done -
// by name: the DAW ("close FL Studio"), since a DAW keeps a plugin's file loaded
// after its window closes and the Center can only install once it lets go.
//
// Nothing here touches the network: RemoteLicenseGate already fetches
// versions.json and caches each product's version (LatestVersions.xml).
//
// The bar and the grey overlay are drawn by ONE script that the editor adds to
// its WebView (JUCE withUserScript). It builds its own elements over whatever
// page the plugin has, so no plugin's HTML changes. Page and C++ talk through
// two events:
//     page -> C++   "roneUpdateAction"  { type: hello | update | later | openCenter | resume }
//     C++ -> page   "roneUpdate"        { available, updating, name, latest, current }
//
// Wiring in a plugin (four lines):
//     processor:  std::atomic<bool> roneUpdatePending { false };
//                 processBlock begins with   if (roneUpdatePending.load()) return;
//                 (a plugin whose output can be wider than its input fills the
//                 extra channels first; one that reports latency runs its own
//                 latency-matched bypass instead - Clipper)
//     editor:     RoneUpdatePrompt updatePrompt;            // declared BEFORE the WebView
//                 updatePrompt (id, name, version, accent, p.roneUpdatePending)
//                 webView (updatePrompt.addTo (juce::WebBrowserComponent::Options{} ...))
//                 updatePrompt.attachBrowser (webView);     // in the constructor body
// An instrument that makes its own sound (Reverse Reverb) goes silent instead of
// passing its input, and says so: pass RoneUpdatePrompt::WhilePending::silent.
// A standalone app (the Analyzer) has no host audio to talk about:
// WhilePending::app, and the card asks to close the app.
// ============================================================================

#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_gui_extra/juce_gui_extra.h>
#if defined (JucePlugin_Name)
 #include <juce_audio_processors/juce_audio_processors.h>   // PluginHostType: which DAW we are in
#endif
#include "RemoteLicenseGate.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

namespace RoneUpdates
{
    static constexpr int kLaterDays = 3;

    // Numeric compare of the first three parts: "1.4.1.230" is newer than "1.4.0",
    // "1.4.0.231" is NOT newer than "1.4.0" (a same-base rebuild is not an update).
    inline bool isNewer (const juce::String& latest, const juce::String& current)
    {
        auto a = juce::StringArray::fromTokens (latest,  ".", "");
        auto b = juce::StringArray::fromTokens (current, ".", "");
        for (int i = 0; i < 3; ++i)
        {
            const int x = i < a.size() ? a[i].getIntValue() : 0;
            const int y = i < b.size() ? b[i].getIntValue() : 0;
            if (x != y)
                return x > y;
        }
        return false;
    }

    // "1.4.1.230" -> "1.4.1"
    inline juce::String displayVersion (const juce::String& v)
    {
        auto parts = juce::StringArray::fromTokens (v, ".", "");
        while (parts.size() > 3)
            parts.remove (parts.size() - 1);
        return parts.joinIntoString (".");
    }

    inline juce::File promptFile()
    {
        return RemoteLicenseGate::getModeFile().getSiblingFile ("UpdatePrompt.xml");
    }

    // LATER: the bar stays away for kLaterDays for THAT version; a newer one shows at once.
    inline bool isSnoozed (const juce::String& id, const juce::String& version)
    {
        auto xml = juce::parseXML (promptFile());
        if (xml == nullptr)
            return false;
        for (auto* e : xml->getChildWithTagNameIterator ("Later"))
            if (e->getStringAttribute ("id") == id && e->getStringAttribute ("version") == version)
                return juce::Time::currentTimeMillis() < e->getStringAttribute ("until").getLargeIntValue();
        return false;
    }

    inline void snooze (const juce::String& id, const juce::String& version)
    {
        auto file = promptFile();
        auto xml = juce::parseXML (file);
        if (xml == nullptr || xml->getTagName() != "RoneUpdatePrompt")
            xml = std::make_unique<juce::XmlElement> ("RoneUpdatePrompt");

        juce::XmlElement* entry = nullptr;
        for (auto* e : xml->getChildWithTagNameIterator ("Later"))
            if (e->getStringAttribute ("id") == id)
                entry = e;
        if (entry == nullptr)
            entry = xml->createNewChildElement ("Later");

        entry->setAttribute ("id", id);
        entry->setAttribute ("version", version);
        entry->setAttribute ("until", juce::String (juce::Time::currentTimeMillis() + (juce::int64) kLaterDays * 86400000));
        file.getParentDirectory().createDirectory();
        xml->writeTo (file, {});
    }

   #if JUCE_WINDOWS
    inline juce::String readMachineString (const wchar_t* subKey, const wchar_t* value)
    {
        wchar_t buffer[1024] = {};
        DWORD size = sizeof (buffer);
        if (RegGetValueW (HKEY_LOCAL_MACHINE, subKey, value, RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
                          nullptr, buffer, &size) == ERROR_SUCCESS)
            return juce::String (buffer);
        return {};
    }
   #endif

    // Where the Plugins Center is: the installer's own record first (right even
    // when someone installed it elsewhere), then the default folder.
    inline juce::File findCenter()
    {
       #if JUCE_WINDOWS
        const auto location = readMachineString (L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}_is1",
                                                 L"InstallLocation");
        if (location.isNotEmpty())
        {
            auto exe = juce::File (location).getChildFile ("RONE Plugins Center.exe");
            if (exe.existsAsFile())
                return exe;
        }
        return juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
                   .getChildFile ("RONE Plugins").getChildFile ("RONE Plugins Center.exe");
       #elif JUCE_MAC
        juce::File app ("/Applications/RONE Plugins Center.app");
        if (! app.exists())
            app = juce::File ("/Applications/RONE Plugins/RONE Plugins Center.app");
        return app;
       #else
        return {};
       #endif
    }

    // Opens the Center on this product's update. False when the Center is not
    // installed - the caller then sends the user to the download page.
    inline bool launchCenterFor (const juce::String& productId)
    {
        auto center = findCenter();
        if (! center.exists())
            return false;
       #if JUCE_WINDOWS
        return center.startAsProcess ("--update " + productId);
       #else
        // macOS hands arguments only to a fresh launch; a running Center simply
        // comes forward and shows the update on the plugin's card.
        return center.startAsProcess ("--update " + productId);
       #endif
    }
}

class RoneUpdatePrompt
{
public:
    // What the host hears between UPDATE and the reopen - the grey card says it.
    enum class WhilePending { passesThrough, silent, app };

    RoneUpdatePrompt (juce::String productId, juce::String productName, juce::String currentVersion,
                      juce::String accentHex, std::atomic<bool>& pendingFlag,
                      WhilePending whilePending = WhilePending::passesThrough)
        : id (std::move (productId)), name (std::move (productName)), current (std::move (currentVersion)),
          accent (std::move (accentHex)), pending (pendingFlag),
          mode (whilePending == WhilePending::silent ? "silent" : whilePending == WhilePending::app ? "app" : "pass")
    {}

    // Adds the script and the event listener to the editor's WebView options.
    juce::WebBrowserComponent::Options addTo (juce::WebBrowserComponent::Options options)
    {
        return options
            .withUserScript (script())
            .withEventListener ("roneUpdateAction", [this] (const juce::var& payload) { onAction (payload); });
    }

    void attachBrowser (juce::WebBrowserComponent& b) { browser = &b; }

private:
    juce::String latest() const { return RemoteLicenseGate::latestVersionFor (id); }

    // The program that has the plugin loaded: "FL Studio", "Ableton Live"... A DAW
    // keeps a plugin's file loaded after its window closes (most until they quit),
    // and Windows cannot replace it until then, so the grey card names the DAW.
    // Empty in the plugin's own standalone window and in an app (the Analyzer).
    static juce::String hostName()
    {
       #if defined (JucePlugin_Name)
        if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone)
            return {};
        const juce::String host (juce::PluginHostType().getHostDescription());
        return host.isEmpty() || host == "Unknown" ? juce::String ("your DAW") : host;
       #else
        return {};
       #endif
    }

    juce::var state() const
    {
        const auto newest = latest();
        const bool updating = pending.load();
        const bool available = ! updating && newest.isNotEmpty()
                            && RoneUpdates::isNewer (newest, current)
                            && ! RoneUpdates::isSnoozed (id, newest);

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("available", available);
        obj->setProperty ("updating", updating);
        obj->setProperty ("name", name);
        obj->setProperty ("latest", RoneUpdates::displayVersion (newest));
        obj->setProperty ("current", RoneUpdates::displayVersion (current));
        obj->setProperty ("mode", mode);
        obj->setProperty ("host", hostName());
        return juce::var (obj);
    }

    void send()
    {
        if (browser != nullptr)
            browser->emitEventIfBrowserIsVisible ("roneUpdate", state());
    }

    void onAction (const juce::var& payload)
    {
        const auto type = payload.getProperty ("type", {}).toString();

        if (type == "update" || type == "openCenter")
        {
            pending.store (true);
            if (! RoneUpdates::launchCenterFor (id))
                juce::URL ("https://roneaudio.com/account/").launchInDefaultBrowser();
        }
        else if (type == "later")
        {
            const auto newest = latest();
            if (newest.isNotEmpty())
                RoneUpdates::snooze (id, newest);
        }
        else if (type == "resume")
        {
            pending.store (false);
        }

        send();   // "hello" and every action answer with the current state
    }

    juce::String script() const
    {
        return juce::String (kScript).replace ("%ACCENT%", accent);
    }

    const juce::String id, name, current, accent;
    std::atomic<bool>& pending;
    const juce::String mode;   // "pass" | "silent" | "app" - picks the grey card's sentence
    juce::WebBrowserComponent* browser = nullptr;

    // The page side. Plain ES5, one IIFE, its own prefixed ids and styles, no
    // dependency on the plugin's page. Kept well under MSVC's 16 KB literal limit.
    static constexpr const char* kScript = R"RONEJS(
(function () {
  if (window.__roneUpdatePrompt) return;
  window.__roneUpdatePrompt = true;
  var ACC = '%ACCENT%';
  var CSS = ''
    + '#rone-upd-bar{position:fixed;left:10px;right:10px;top:10px;z-index:2147483000;display:none;align-items:center;gap:12px;'
    + 'padding:10px 12px;border-radius:8px;background:rgba(18,19,23,.97);border:1px solid ' + ACC + ';'
    + 'box-shadow:0 10px 30px rgba(0,0,0,.55);font-family:Manrope,"Segoe UI",system-ui,sans-serif;color:#E8EAED;box-sizing:border-box}'
    + '#rone-upd-bar .ru-led{flex:none;width:8px;height:8px;border-radius:50%;background:' + ACC + ';box-shadow:0 0 10px ' + ACC + '}'
    + '#rone-upd-bar .ru-txt{flex:1;min-width:0;font:700 clamp(9px,2.3vw,12px)/1.3 Manrope,"Segoe UI",sans-serif;letter-spacing:.14em;text-transform:uppercase}'
    + '#rone-upd-bar .ru-txt small{display:block;margin-top:2px;font-weight:500;letter-spacing:.04em;text-transform:none;color:#9aa0a8;font-size:clamp(8px,2vw,11px)}'
    + '#rone-upd-bar button,#rone-upd-lock button{cursor:pointer;border-radius:5px;font:800 clamp(8px,2vw,11px) Manrope,"Segoe UI",sans-serif;letter-spacing:.14em;text-transform:uppercase;padding:8px 11px;white-space:nowrap}'
    + '#rone-upd-bar .ru-go{background:' + ACC + ';color:#0B0C0E;border:0}'
    + '#rone-upd-bar .ru-later{background:transparent;color:#9aa0a8;border:1px solid #30343b}'
    + '#rone-upd-lock{position:fixed;inset:0;z-index:2147483001;display:none;align-items:center;justify-content:center;padding:6%;'
    + 'background:rgba(10,11,13,.45);-webkit-backdrop-filter:grayscale(1) brightness(.5);backdrop-filter:grayscale(1) brightness(.5);box-sizing:border-box}'
    + '#rone-upd-lock .ru-card{max-width:440px;width:100%;text-align:center;background:rgba(16,17,21,.96);border:1px solid #30343b;border-radius:12px;'
    + 'padding:clamp(14px,4vw,26px);font-family:Manrope,"Segoe UI",system-ui,sans-serif;color:#E8EAED;box-sizing:border-box}'
    + '#rone-upd-lock .ru-ring{width:34px;height:34px;margin:0 auto 12px;border-radius:50%;border:3px solid rgba(255,255,255,.12);border-top-color:' + ACC + ';animation:ruSpin 1.1s linear infinite}'
    + '@keyframes ruSpin{to{transform:rotate(360deg)}}'
    + '@media (prefers-reduced-motion:reduce){#rone-upd-lock .ru-ring{animation:none}}'
    + '#rone-upd-lock h4{margin:0;font:800 clamp(10px,2.6vw,14px) Manrope,"Segoe UI",sans-serif;letter-spacing:.16em;text-transform:uppercase}'
    + '#rone-upd-lock p{margin:10px 0 0;font-size:clamp(10px,2.4vw,13px);line-height:1.55;color:#aab0b8}'
    + '#rone-upd-lock .ru-row{display:flex;flex-wrap:wrap;justify-content:center;gap:8px;margin-top:14px}'
    + '#rone-upd-lock .ru-open{background:transparent;color:#E8EAED;border:1px solid #3a3e46}'
    + '#rone-upd-lock .ru-keep{background:transparent;color:#8d939c;border:0;text-decoration:underline;text-underline-offset:3px}';

  var bar = null, lock = null, state = null;
  function backend() { return window.__JUCE__ && window.__JUCE__.backend; }
  function emit(type) { var b = backend(); if (b) b.emitEvent('roneUpdateAction', { type: type }); }
  function el(tag, cls, text) { var n = document.createElement(tag); if (cls) n.className = cls; if (text != null) n.textContent = text; return n; }

  function build() {
    var host = document.body || document.documentElement;
    var style = el('style'); style.textContent = CSS; (document.head || host).appendChild(style);

    bar = el('div'); bar.id = 'rone-upd-bar'; bar.setAttribute('role', 'status');
    bar.appendChild(el('span', 'ru-led'));
    var txt = el('span', 'ru-txt'); txt.appendChild(el('span', 'ru-title')); txt.appendChild(el('small', 'ru-sub'));
    bar.appendChild(txt);
    var go = el('button', 'ru-go', 'Update'); go.type = 'button';
    var later = el('button', 'ru-later', 'Later'); later.type = 'button';
    go.addEventListener('click', function () { emit('update'); });
    later.addEventListener('click', function () { bar.style.display = 'none'; emit('later'); });
    bar.appendChild(go); bar.appendChild(later);

    lock = el('div'); lock.id = 'rone-upd-lock'; lock.setAttribute('role', 'dialog'); lock.setAttribute('aria-modal', 'true');
    var card = el('div', 'ru-card');
    card.appendChild(el('div', 'ru-ring'));
    card.appendChild(el('h4', null, 'Updating in the RONE Plugins Center'));
    card.appendChild(el('p', 'ru-note'));
    var row = el('div', 'ru-row');
    var open = el('button', 'ru-open', 'Open the Center'); open.type = 'button';
    var keep = el('button', 'ru-keep', 'Keep using this version'); keep.type = 'button';
    open.addEventListener('click', function () { emit('openCenter'); });
    keep.addEventListener('click', function () { emit('resume'); });
    row.appendChild(open); row.appendChild(keep); card.appendChild(row); lock.appendChild(card);

    host.appendChild(bar); host.appendChild(lock);
  }

  function render() {
    if (!state) return;
    if (!bar) build();
    var titleBar = document.body && document.body.classList.contains('has-titlebar') ? 30 : 0;
    bar.style.top = (titleBar + 10) + 'px';
    lock.style.top = titleBar + 'px';
    bar.querySelector('.ru-title').textContent = state.name + ' ' + state.latest + ' is available';
    bar.querySelector('.ru-sub').textContent = 'You have ' + state.current + '. The Plugins Center updates it for you.';
    // The Center installs the moment nothing holds the plugin's files: our own
    // window, or the DAW - which keeps a plugin loaded until it quits.
    lock.querySelector('.ru-note').textContent =
        state.mode === 'app' ? 'Close ' + state.name + ' and the update installs by itself - if its Bridge is loaded in your DAW, close the DAW too. Your settings stay as they are.'
      : !state.host          ? 'Close this window and the update installs by itself, then open ' + state.name + ' again. Your settings stay as they are.'
      : 'Save your project and close ' + state.host + ': the update installs by itself, then open it again. '
        + (state.mode === 'silent' ? 'Until then ' + state.name + ' stays silent' : 'Until then your sound passes through untouched')
        + ', and your settings stay as they are.';
    bar.style.display = state.available ? 'flex' : 'none';
    lock.style.display = state.updating ? 'flex' : 'none';
  }

  function start() {
    var b = backend();
    if (!b) { setTimeout(start, 150); return; }
    b.addEventListener('roneUpdate', function (s) { state = s; render(); });
    var hello = function () { emit('hello'); };
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', hello, { once: true });
    else hello();
  }
  start();
})();
)RONEJS";

    JUCE_DECLARE_NON_COPYABLE (RoneUpdatePrompt)
};
