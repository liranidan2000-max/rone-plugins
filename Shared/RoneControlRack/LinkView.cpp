#include "LinkView.h"
#include "ControlCore.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <wrl.h>
 #include <WebView2.h>
 #include <WebView2EnvironmentOptions.h>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;

namespace RoneCtl
{
namespace
{
    juce::String hex (HRESULT hr) { return "0x" + juce::String::toHexString ((int) hr); }

    juce::String takeString (LPWSTR s)             // a string WebView2 allocated for us
    {
        juce::String r;
        if (s != nullptr)
        {
            r = juce::String (s);
            CoTaskMemFree (s);
        }
        return r;
    }

    bool fromUs (const juce::String& uri) { return uri.startsWith (juce::String (LinkView::kOrigin) + "/"); }
}

struct LinkView::Impl : std::enable_shared_from_this<Impl>
{
    HWND parent = nullptr;
    juce::File folder;
    Handler handler;
    ComPtr<ICoreWebView2Environment> env;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    EventRegistrationToken msgToken {}, permToken {};
    juce::String err;
    int width = 48, height = 48;
    bool closed = false;

    void create()
    {
        const auto userData = webViewFolder ("RoneControl_Link_WebView2");
        // WebRtcHideLocalIpsWithMdns off: the paired phone gets this computer's real LAN address instead of a
        // ".local" name it would have to resolve by multicast - many routers do not pass that, and the direct
        // path then never opens. (The address goes only to the phone that holds the pairing key.)
        juce::String args ("--disable-features=CalculateNativeWinOcclusion,WebRtcHideLocalIpsWithMdns --disable-background-timer-throttling "
                           "--disable-renderer-backgrounding --disable-backgrounding-occluded-windows");
        // tests only: a synthetic camera, a DevTools port (never set in a user's DAW)
        const auto extra = juce::SystemStats::getEnvironmentVariable ("RONE_CONTROL_LINK_ARGS", {});
        if (extra.isNotEmpty()) args << " " << extra;

        auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
        options->put_AdditionalBrowserArguments (args.toWideCharPointer());

        std::weak_ptr<Impl> weak = shared_from_this();
        const auto hr = CreateCoreWebView2EnvironmentWithOptions (nullptr, userData.getFullPathName().toWideCharPointer(), options.Get(),
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> ([weak] (HRESULT res, ICoreWebView2Environment* e) -> HRESULT
            {
                if (auto self = weak.lock()) self->onEnvironment (res, e);
                return S_OK;
            }).Get());
        if (FAILED (hr)) err = "no WebView2 (" + hex (hr) + ")";
    }

    void onEnvironment (HRESULT res, ICoreWebView2Environment* e)
    {
        if (closed) return;
        if (FAILED (res) || e == nullptr) { err = "WebView2 environment " + hex (res); return; }
        env = e;
        std::weak_ptr<Impl> weak = shared_from_this();
        const auto hr = env->CreateCoreWebView2Controller (parent,
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> ([weak] (HRESULT r, ICoreWebView2Controller* c) -> HRESULT
            {
                if (auto self = weak.lock()) self->onController (r, c);
                else if (c != nullptr) c->Close();
                return S_OK;
            }).Get());
        if (FAILED (hr)) err = "WebView2 controller " + hex (hr);
    }

    void onController (HRESULT res, ICoreWebView2Controller* c)
    {
        if (closed) { if (c != nullptr) c->Close(); return; }
        if (FAILED (res) || c == nullptr) { err = "WebView2 controller " + hex (res); return; }
        controller = c;
        if (FAILED (controller->get_CoreWebView2 (webview.ReleaseAndGetAddressOf())) || webview == nullptr) { err = "WebView2 view"; return; }

        ComPtr<ICoreWebView2Settings> settings;
        if (SUCCEEDED (webview->get_Settings (settings.GetAddressOf())))
        {
            settings->put_AreDefaultContextMenusEnabled (FALSE);
            settings->put_IsStatusBarEnabled (FALSE);
            settings->put_IsZoomControlEnabled (FALSE);
        }
        RECT r { 0, 0, width, height };
        controller->put_Bounds (r);
        controller->put_IsVisible (TRUE);

        ComPtr<ICoreWebView2_3> wv3;
        if (FAILED (webview.As (&wv3)) || wv3 == nullptr) { err = "WebView2 runtime too old"; return; }
        const juce::String host = juce::String (kOrigin).fromFirstOccurrenceOf ("https://", false, false);
        wv3->SetVirtualHostNameToFolderMapping (host.toWideCharPointer(), folder.getFullPathName().toWideCharPointer(),
                                                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);

        std::weak_ptr<Impl> weak = shared_from_this();
        webview->add_WebMessageReceived (Callback<ICoreWebView2WebMessageReceivedEventHandler> (
            [weak] (ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* a) -> HRESULT
            {
                auto self = weak.lock();
                if (self == nullptr || self->closed) return S_OK;
                LPWSTR src = nullptr;
                a->get_Source (&src);
                if (! fromUs (takeString (src))) return S_OK;
                LPWSTR json = nullptr;
                if (SUCCEEDED (a->get_WebMessageAsJson (&json)) && json != nullptr)
                {
                    const auto v = juce::JSON::parse (takeString (json));
                    if (self->handler != nullptr && v.isObject())
                        self->handler (v["n"].toString(), v["p"]);
                }
                return S_OK;
            }).Get(), &msgToken);

        // The camera for our own page, answered here (a prompt in an off-screen window could never be
        // clicked); the page only asks after the user picked CAMERA in the plugin. Everything else: no.
        webview->add_PermissionRequested (Callback<ICoreWebView2PermissionRequestedEventHandler> (
            [] (ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* a) -> HRESULT
            {
                COREWEBVIEW2_PERMISSION_KIND kind = COREWEBVIEW2_PERMISSION_KIND_UNKNOWN_PERMISSION;
                a->get_PermissionKind (&kind);
                LPWSTR uri = nullptr;
                a->get_Uri (&uri);
                const bool ok = kind == COREWEBVIEW2_PERMISSION_KIND_CAMERA && fromUs (takeString (uri));
                a->put_State (ok ? COREWEBVIEW2_PERMISSION_STATE_ALLOW : COREWEBVIEW2_PERMISSION_STATE_DENY);
                return S_OK;
            }).Get(), &permToken);

        webview->Navigate ((juce::String (kOrigin) + "/link.html").toWideCharPointer());
    }

    void post (const juce::String& name, const juce::var& payload)
    {
        if (closed || webview == nullptr) return;
        auto* o = new juce::DynamicObject();
        o->setProperty ("n", name);
        o->setProperty ("p", payload);
        webview->PostWebMessageAsJson (juce::JSON::toString (juce::var (o), true).toWideCharPointer());
    }

    void close()
    {
        closed = true;
        if (webview != nullptr)
        {
            webview->remove_WebMessageReceived (msgToken);
            webview->remove_PermissionRequested (permToken);
        }
        if (controller != nullptr) controller->Close();
        webview = nullptr;
        controller = nullptr;
        env = nullptr;
    }
};

LinkView::LinkView (void* parentHwnd, const juce::File& folder, Handler onMessage)
    : impl (std::make_shared<Impl>())
{
    impl->parent = static_cast<HWND> (parentHwnd);
    impl->folder = folder;
    impl->handler = std::move (onMessage);
    impl->create();
}

LinkView::~LinkView()
{
    impl->close();
}

void LinkView::post (const juce::String& name, const juce::var& payload) { impl->post (name, payload); }

void LinkView::setSize (int w, int h)
{
    impl->width = w;
    impl->height = h;
    if (impl->controller != nullptr)
    {
        RECT r { 0, 0, w, h };
        impl->controller->put_Bounds (r);
    }
}

juce::String LinkView::error() const { return impl->err; }

// ============================================================================
// The files the link page loads. The hand model and its runtime (Google's MediaPipe, Apache 2.0, see
// NOTICE.txt) are compiled into RONE Control as resources (link_assets.rc, made by CMake from
// lab/public/mp); they are written out once, then left alone while their size matches.
// THE MODEL LIVES ONCE: a plugin with a rack carries no resources of its own - it uses the files RONE
// Control already wrote into the shared link folder. Without them the camera cannot start; the rack then
// shows "install RONE Control from the Center" (the phone needs no model and keeps working).
// ============================================================================
namespace
{
    HMODULE ourModule()
    {
        HMODULE h = nullptr;
        GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR> (&ourModule), &h);
        return h;
    }

    bool writeIfChanged (const juce::File& dest, const void* data, size_t size)
    {
        if (dest.existsAsFile() && dest.getSize() == (juce::int64) size) return true;
        dest.getParentDirectory().createDirectory();
        const auto part = dest.getSiblingFile (dest.getFileName() + ".part");
        if (! part.replaceWithData (data, size)) return false;
        dest.deleteFile();
        return part.moveFileTo (dest);
    }
}

juce::String prepareLinkFolder (const juce::File& folder, const juce::String& linkHtml)
{
    struct Asset { const wchar_t* res; const char* path; };
    static const Asset assets[] = {
        { L"RCLINKBUNDLE", "mp/vision_bundle.mjs" },
        { L"RCLINKWASMJS", "mp/wasm/vision_wasm_internal.js" },
        { L"RCLINKWASM",   "mp/wasm/vision_wasm_internal.wasm" },
        { L"RCLINKMODEL",  "mp/hand_landmarker.task" },
        { L"RCLINKNOTICE", "mp/NOTICE.txt" },
    };
    const auto mod = ourModule();
    juce::StringArray failed, absent;
    for (const auto& a : assets)
    {
        const auto dest = folder.getChildFile (a.path);
        const auto r = FindResourceW (mod, a.res, MAKEINTRESOURCEW (10));      // RT_RCDATA
        if (r == nullptr)                                                       // not ours to write: RONE Control's file, if it is there
        {
            if (! (dest.existsAsFile() && dest.getSize() > 0)) absent.add (a.path);
            continue;
        }
        const auto g = LoadResource (mod, r);
        const void* data = g != nullptr ? LockResource (g) : nullptr;
        const auto size = (size_t) SizeofResource (mod, r);
        if (data == nullptr || size == 0 || ! writeIfChanged (dest, data, size))
            failed.add (a.path);
    }
    const auto page = folder.getChildFile ("link.html");
    if (page.loadFileAsString() != linkHtml && ! page.replaceWithText (linkHtml, false, false, "\n"))
        failed.add ("link.html");
    if (failed.size() > 0) return "could not write " + failed.joinIntoString (", ");
    if (absent.size() > 0) return kNoModelError;
    return {};
}
} // namespace RoneCtl
#endif
