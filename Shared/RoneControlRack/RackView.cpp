#include "RackView.h"

namespace RoneRack
{
namespace
{
    juce::WebBrowserComponent::Options options (View* self, Engine& engine, const juce::String& pluginId,
                                                std::function<std::optional<juce::WebBrowserComponent::Resource> (const juce::String&)> res)
    {
        auto o = juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                    .getChildFile ("RonePlugins").getChildFile (pluginId + "_Rack_WebView2")))
            .withNativeIntegrationEnabled()
            .withResourceProvider (std::move (res));
        // page -> C++: everything but rackReady / collapse goes to the engine as it is
        for (auto* name : { "slotConfig", "slotOn", "slotParam", "addSlot", "removeSlot", "reset", "setLive", "arm",
                            "setSource", "hands", "newCode", "camera", "view", "rec" })
        {
            const juce::String n (name);
            o = o.withEventListener (n, [&engine, n] (const juce::var& p) { engine.pageEvent (n, p); });
        }
        juce::ignoreUnused (self);
        return o;
    }
}

View::View (Engine& e, const juce::String& pluginId, std::function<void()> onCollapse)
    : engine (e), collapse (std::move (onCollapse)),
      web (options (this, e, pluginId, [] (const juce::String& url) { return resource (url); })
               .withEventListener ("rackReady", [this] (const juce::var&) { pageReady = true; })
               .withEventListener ("collapse",  [this] (const juce::var&) { if (collapse) collapse(); }))
{
    setOpaque (true);
    addAndMakeVisible (web);
    web.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (30);
}

View::~View()
{
    stopTimer();
}

void View::resized()
{
    web.setBounds (getLocalBounds());
}

void View::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14161a));
}

void View::timerCallback()
{
    if (! pageReady || ! isShowing()) return;
    web.emitEventIfBrowserIsVisible ("rackState", engine.uiState (true));
}

std::optional<juce::WebBrowserComponent::Resource> View::resource (const juce::String& url)
{
    auto serve = [] (const char* name, const char* mime) -> std::optional<juce::WebBrowserComponent::Resource>
    {
        int size = 0;
        if (const auto* data = BinaryData::getNamedResource (name, size); data != nullptr && size > 0)
        {
            const auto* bytes = reinterpret_cast<const std::byte*> (data);
            return juce::WebBrowserComponent::Resource { std::vector<std::byte> (bytes, bytes + size), juce::String (mime) };
        }
        return std::nullopt;
    };
    if (url == "/" || url.isEmpty() || url == "/index.html" || url == "/rack.html") return serve ("rack_html", "text/html");
    if (url == "/rack.css") return serve ("rack_css", "text/css");
    if (url == "/rack.js")  return serve ("rack_js", "text/javascript");
    if (url == "/qrcode.min.js") return serve ("qrcode_min_js", "text/javascript");   // the QR for the phone, drawn offline
    return std::nullopt;
}
} // namespace RoneRack
