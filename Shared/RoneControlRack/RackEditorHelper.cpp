#include "RackEditorHelper.h"

namespace RoneRack
{
namespace
{
// The + handle inside the plugin's page: a grey hairline at rest, laser green only under the mouse (Liran 25.10). Plain ES5. Talks through two events:
//   page -> C++  "roneRack"       { type: "open" }
//   C++ -> page  "roneRackState"  { open }
const char* kHandleScript = R"RONERACK((function () {
  if (window.__roneRackHandle) return; window.__roneRackHandle = true;
  var NEON = '82,255,69';
  function build() {
    if (document.getElementById('rone-rack-handle')) return;
    var css = document.createElement('style');
    css.textContent =
      '#rone-rack-handle{position:fixed;right:0;top:50%;transform:translateY(-50%);height:84px;width:14px;z-index:2147483000;cursor:pointer;overflow:hidden;' +
      'border-radius:10px 0 0 10px;transition:width .18s ease,background .18s ease;background:transparent;display:flex;align-items:center;justify-content:center;font-family:Sora,Manrope,"Segoe UI",sans-serif}' +
      '#rone-rack-handle i{position:absolute;right:0;top:18px;bottom:18px;width:3px;border-radius:2px;background:rgba(255,255,255,.16);transition:opacity .18s,background .18s}' +
      '#rone-rack-handle b{font-size:22px;font-weight:800;color:rgb(' + NEON + ');opacity:0;transition:opacity .18s;text-shadow:0 0 14px rgba(' + NEON + ',.9);line-height:1}' +
      '#rone-rack-handle span{position:absolute;bottom:7px;left:0;right:0;text-align:center;font-size:7px;font-weight:800;letter-spacing:.22em;color:rgb(' + NEON + ');opacity:0;transition:opacity .18s}' +
      '#rone-rack-handle:hover{width:46px;background:rgba(16,18,22,.94);box-shadow:-8px 0 24px rgba(0,0,0,.5),inset 1px 0 0 #2A2E35}' +
      '#rone-rack-handle:hover i{opacity:0;background:rgb(' + NEON + ')}#rone-rack-handle:hover b,#rone-rack-handle:hover span{opacity:1}' +
      '#rone-rack-handle.rr-hidden{display:none}';
    document.head.appendChild(css);
    var h = document.createElement('div'); h.id = 'rone-rack-handle'; h.title = 'RONE Control';
    h.innerHTML = '<i></i><b>+</b><span>CONTROL</span>';
    h.addEventListener('click', function (e) { e.stopPropagation(); if (window.__JUCE__ && window.__JUCE__.backend) window.__JUCE__.backend.emitEvent('roneRack', { type: 'open' }); });
    h.addEventListener('mousedown', function (e) { e.stopPropagation(); });
    document.body.appendChild(h);
    function bridge() {
      if (!(window.__JUCE__ && window.__JUCE__.backend)) { setTimeout(bridge, 60); return; }
      window.__JUCE__.backend.addEventListener('roneRackState', function (s) { h.classList.toggle('rr-hidden', !!(s && s.open)); });
      window.__JUCE__.backend.emitEvent('roneRack', { type: 'hello' });
    }
    bridge();
  }
  if (document.body) build(); else document.addEventListener('DOMContentLoaded', build);
})();)RONERACK";
}

EditorHelper::EditorHelper (juce::AudioProcessorEditor& editor, Engine& engine, int baseW, int baseH)
    : ed (editor), eng (engine), W (baseW), H (baseH)
{
}

EditorHelper::~EditorHelper()
{
    stopTimer();
    eng.editorClosed();                                    // a closed window never moves a knob; it remembers to come back
    view = nullptr;
}

juce::WebBrowserComponent::Options EditorHelper::addTo (juce::WebBrowserComponent::Options options)
{
    return options
        .withUserScript (kHandleScript)
        .withEventListener ("roneRack", [this] (const juce::var& p)
        {
            const auto type = p["type"].toString();
            if (type == "open") setOpen (true);
            else if (type == "hello") tellPage();
        });
}

void EditorHelper::attach (juce::WebBrowserComponent& pluginView)
{
    plugin = &pluginView;
    if (eng.takeReopen())                                  // the rack was open when this window closed: it comes back,
    {                                                      // a moment after the plugin's own view (WebView2s one at a time)
        pendingReopen = true;
        startTimer (600);
    }
}

void EditorHelper::tellPage()
{
    if (plugin == nullptr) return;
    auto* o = new juce::DynamicObject();
    o->setProperty ("open", open);
    plugin->emitEventIfBrowserIsVisible ("roneRackState", juce::var (o));
}

double EditorHelper::widthFactor() const
{
    return grows() ? (double) (W + kRackW) / (double) W : 1.0;
}

int EditorHelper::heightForWidth (int contentWidth) const
{
    return juce::roundToInt ((double) contentWidth * H / (grows() ? (double) (W + kRackW) : (double) W));
}

void EditorHelper::layout (juce::Rectangle<int> bounds)
{
    if (! open || view == nullptr)
    {
        if (plugin != nullptr) plugin->setBounds (bounds);
        if (view != nullptr) view->setVisible (false);
        return;
    }
    int rp = rackPx (bounds.getHeight());
    if (shrink) rp = juce::jmin (rp, juce::roundToInt (bounds.getWidth() * 0.38));
    auto right = bounds.removeFromRight (rp);
    if (plugin != nullptr) plugin->setBounds (bounds);
    view->setBounds (right);
    view->setVisible (true);
}

void EditorHelper::setOpen (bool shouldOpen)
{
    if (open == shouldOpen) return;
    const int h = ed.getHeight(), w = ed.getWidth();
    if (shouldOpen)
    {
        if (view == nullptr)
        {
            view = std::make_unique<View> (eng, eng.pluginId(), [this] { setOpen (false); });
            ed.addChildComponent (*view);
        }
        open = true;
        eng.setOpen (true);
        eng.arm();
        if (! shrink)
        {
            if (onToggle) onToggle();                      // the limits widen FIRST, or the window clamps the new width
            expectW = w + rackPx (h);
            ed.setSize (expectW, h);                       // the host is asked for a wider window ...
            startTimer (180);                              // ... and checked a moment later
        }
        else if (onToggle) onToggle();
    }
    else
    {
        stopTimer();
        const bool grew = grows();
        open = false;
        eng.setOpen (false);
        // The limits narrow FIRST: the open limits' minimum width is kMinW * (W + 280) / W, which is wider than the
        // closed window (Throw: 504 > 440), and a constrainer still holding it clamps the setSize below to that
        // minimum - the window came back 64 px too wide (Phase 0, 26.10.2026: 560 -> 910 -> 640 real px).
        if (onToggle) onToggle();
        if (grew) ed.setSize (w - rackPx (h), h);
    }
    ed.resized();
    tellPage();
}

void EditorHelper::timerCallback()
{
    stopTimer();
    if (pendingReopen) { pendingReopen = false; setOpen (true); return; }
    if (! open || shrink) return;
    if (ed.getWidth() != expectW)                          // the host refused: the plugin makes room instead
    {
        shrink = true;
        if (onToggle) onToggle();
        ed.resized();
    }
}
} // namespace RoneRack
