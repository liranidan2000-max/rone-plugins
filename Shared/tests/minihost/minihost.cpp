// minihost - opens a VST3's editor in a raw Win32 window, the way Ableton Live's "Auto-Scale Plug-In Window" does:
// the process is per-monitor DPI aware, the plug-in window is created DPI-UNAWARE (Windows stretches it), mixed
// hosting. No audio, no MIDI.   minihost.exe <path to the .vst3 DLL> [title] [aware]   (README.md)
#define INIT_CLASS_IID
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <string>
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/base/funknown.cpp"

using namespace Steinberg;


static HWND gWnd = nullptr;
static IPlugView* gView = nullptr;

struct Host : Vst::IHostApplication
{
    tresult PLUGIN_API getName (Vst::String128 name) override
    {
        const char16_t* n = u"Ableton Live"; int i = 0;
        for (; n[i]; ++i) name[i] = (Vst::TChar) n[i];
        name[i] = 0; return kResultOk;
    }
    tresult PLUGIN_API createInstance (TUID, TUID, void** obj) override { *obj = nullptr; return kNotImplemented; }
    tresult PLUGIN_API queryInterface (const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual (iid, FUnknown::iid) || FUnknownPrivate::iidEqual (iid, Vst::IHostApplication::iid)) { *obj = this; return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return 1; }
    uint32 PLUGIN_API release() override { return 1; }
};

struct Handler : Vst::IComponentHandler
{
    tresult PLUGIN_API beginEdit (Vst::ParamID) override { return kResultOk; }
    tresult PLUGIN_API performEdit (Vst::ParamID, Vst::ParamValue) override { return kResultOk; }
    tresult PLUGIN_API endEdit (Vst::ParamID) override { return kResultOk; }
    tresult PLUGIN_API restartComponent (int32) override { return kResultOk; }
    tresult PLUGIN_API queryInterface (const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual (iid, FUnknown::iid) || FUnknownPrivate::iidEqual (iid, Vst::IComponentHandler::iid)) { *obj = this; return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return 1; }
    uint32 PLUGIN_API release() override { return 1; }
};

struct Frame : IPlugFrame
{
    tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* r) override
    {
        // the window's own DPI context: sizes are the plug-in's (logical) pixels
        const auto old = SetThreadDpiAwarenessContext (GetWindowDpiAwarenessContext (gWnd));
        RECT rc { 0, 0, r->getWidth(), r->getHeight() };
        AdjustWindowRectEx (&rc, (DWORD) GetWindowLongW (gWnd, GWL_STYLE), FALSE, 0);
        SetWindowPos (gWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
        SetThreadDpiAwarenessContext (old);
        return view->onSize (r);
    }
    tresult PLUGIN_API queryInterface (const TUID iid, void** obj) override
    {
        if (FUnknownPrivate::iidEqual (iid, FUnknown::iid) || FUnknownPrivate::iidEqual (iid, IPlugFrame::iid)) { *obj = this; return kResultOk; }
        *obj = nullptr; return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return 1; }
    uint32 PLUGIN_API release() override { return 1; }
};

static Host gHost;
static Handler gHandler;
static Frame gFrame;

static LRESULT CALLBACK wndProc (HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_SIZE && gView != nullptr)
    {
        ViewRect r (0, 0, LOWORD (l), HIWORD (l));
        if (gView->checkSizeConstraint (&r) == kResultOk) {}
        gView->onSize (&r);
        return 0;
    }
    if (m == WM_CLOSE) { DestroyWindow (h); return 0; }
    if (m == WM_DESTROY) { PostQuitMessage (0); return 0; }
    return DefWindowProcW (h, m, w, l);
}

int wmain (int argc, wchar_t** argv)
{
    if (argc < 2) { fprintf (stderr, "minihost <plugin.vst3 dll> [title] [aware]\n"); return 1; }
    const std::wstring title = argc > 2 ? argv[2] : L"minihost";
    const bool aware = argc > 3 && std::wstring (argv[3]) == L"aware";

    SetProcessDpiAwarenessContext (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    OleInitialize (nullptr);

    HMODULE dll = LoadLibraryW (argv[1]);
    if (! dll) { fprintf (stderr, "LoadLibrary failed %lu\n", GetLastError()); return 2; }
    if (auto init = (bool (*)()) GetProcAddress (dll, "InitDll")) init();
    auto getFactory = (IPluginFactory* (PLUGIN_API*)()) GetProcAddress (dll, "GetPluginFactory");
    IPluginFactory* factory = getFactory ? getFactory() : nullptr;
    if (! factory) { fprintf (stderr, "no factory\n"); return 3; }

    Vst::IComponent* comp = nullptr;
    for (int32 i = 0; i < factory->countClasses(); ++i)
    {
        PClassInfo ci;
        factory->getClassInfo (i, &ci);
        if (strcmp (ci.category, "Audio Module Class") == 0)
        {
            factory->createInstance (ci.cid, Vst::IComponent::iid, (void**) &comp);
            break;
        }
    }
    if (! comp) { fprintf (stderr, "no component\n"); return 4; }
    comp->initialize (&gHost);

    Vst::IEditController* ctrl = nullptr;
    TUID ccid;
    if (comp->queryInterface (Vst::IEditController::iid, (void**) &ctrl) != kResultOk)
    {
        comp->getControllerClassId (ccid);
        factory->createInstance (ccid, Vst::IEditController::iid, (void**) &ctrl);
        if (! ctrl) { fprintf (stderr, "no controller\n"); return 5; }
        ctrl->initialize (&gHost);
    }
    ctrl->setComponentHandler (&gHandler);
    Vst::IConnectionPoint *cpA = nullptr, *cpB = nullptr;
    comp->queryInterface (Vst::IConnectionPoint::iid, (void**) &cpA);
    ctrl->queryInterface (Vst::IConnectionPoint::iid, (void**) &cpB);
    if (cpA && cpB) { cpA->connect (cpB); cpB->connect (cpA); }

    gView = ctrl->createView (Vst::ViewType::kEditor);
    if (! gView) { fprintf (stderr, "no view\n"); return 6; }

    WNDCLASSW wc {};
    wc.lpfnWndProc = wndProc; wc.hInstance = GetModuleHandleW (nullptr); wc.lpszClassName = L"MiniHostWnd";
    wc.hCursor = LoadCursor (nullptr, IDC_ARROW); wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    RegisterClassW (&wc);

    // Live's Auto-Scale Plug-In Window: the plug-in's window is DPI-unaware, in a DPI-aware process
    const auto oldCtx = SetThreadDpiAwarenessContext (aware ? DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 : DPI_AWARENESS_CONTEXT_UNAWARE);
    SetThreadDpiHostingBehavior (DPI_HOSTING_BEHAVIOR_MIXED);
    ViewRect vr;
    gView->getSize (&vr);
    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc { 0, 0, vr.getWidth(), vr.getHeight() };
    AdjustWindowRectEx (&rc, style, FALSE, 0);
    gWnd = CreateWindowExW (0, wc.lpszClassName, title.c_str(), style, 80, 80, rc.right - rc.left, rc.bottom - rc.top,
                            nullptr, nullptr, wc.hInstance, nullptr);
    gView->setFrame (&gFrame);
    if (gView->attached ((void*) gWnd, kPlatformTypeHWND) != kResultOk) { fprintf (stderr, "attach failed\n"); return 7; }
    SetThreadDpiAwarenessContext (oldCtx);
    ShowWindow (gWnd, SW_SHOW);
    printf ("open %d x %d\n", vr.getWidth(), vr.getHeight());
    fflush (stdout);

    MSG msg;
    while (GetMessageW (&msg, nullptr, 0, 0) > 0) { TranslateMessage (&msg); DispatchMessageW (&msg); }

    gView->removed();
    gView->release();
    if (cpA && cpB) { cpA->disconnect (cpB); cpB->disconnect (cpA); }
    ctrl->terminate();
    comp->terminate();
    if (auto exitDll = (bool (*)()) GetProcAddress (dll, "ExitDll")) exitDll();
    printf ("closed\n");
    return 0;
}
