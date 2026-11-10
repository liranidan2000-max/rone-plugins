// ============================================================================
// ReopenEditorHost - close a plugin window and open it again at once.
//
// Liran 10.11.2026: DMT and Iron in FL opened fine, he closed the window, opened
// it again from the Channel rack, and it stayed white for 46 s to minutes. The
// guess: the closed window's WebView2 browser process is still shutting down and
// the new window waits for it. This host does exactly that, with one instance:
//
//   ReopenEditorHost <plugin.vst3> <cycles> <hold seconds> <gap ms> [linger ms]
//
// Every cycle: editor + window, hosted <hold> s, window and editor deleted,
// <gap> ms of message loop, again. It prints wall-clock times (ms since epoch)
// for every open and close, so reopen_watch.mjs (CDP, outside) can line them up
// with the moment each window's page really appeared.
// Run it like the twin test: hidden desktop, its own WEBVIEW2_USER_DATA_FOLDER.
// ============================================================================
#include <JuceHeader.h>

#include <windows.h>
#include <tlhelp32.h>
#include <thread>
#include <set>

// --- slow-shutdown injection ------------------------------------------------
// The browser process of a closed window lingered 20-40 s on Liran's PC (FL, 10.11.2026)
// and sometimes in this test, depending on load. <linger ms> makes it happen every time:
// at each close the WebView2 browser processes this host started are suspended for that
// long (they cannot finish exiting and keep their profile's lockfile), then resumed.
using NtProc = LONG (NTAPI*) (HANDLE);

static std::vector<DWORD> ourBrowsers()
{
    std::vector<DWORD> out;
    const DWORD self = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    PROCESSENTRY32W e { sizeof (e) };
    for (BOOL ok = Process32FirstW (snap, &e); ok; ok = Process32NextW (snap, &e))
        if (e.th32ParentProcessID == self && _wcsicmp (e.szExeFile, L"msedgewebview2.exe") == 0)
            out.push_back (e.th32ProcessID);       // the browser is our direct child, its helpers are its children
    CloseHandle (snap);
    return out;
}

static std::vector<std::thread> resumers;

// each browser is frozen once, when its window closes; every one is resumed after <ms>
static void lingerBrowsers (int ms, const std::vector<DWORD>& pids)
{
    if (ms <= 0) return;
    static std::set<DWORD> done;
    static auto suspend = (NtProc) GetProcAddress (GetModuleHandleW (L"ntdll.dll"), "NtSuspendProcess");
    static auto resume  = (NtProc) GetProcAddress (GetModuleHandleW (L"ntdll.dll"), "NtResumeProcess");
    for (auto pid : pids)
    {
        if (! done.insert (pid).second) continue;
        HANDLE h = OpenProcess (PROCESS_SUSPEND_RESUME, FALSE, pid);
        if (h == nullptr) continue;
        suspend (h);
        std::cout << "LINGER " << pid << " " << juce::Time::currentTimeMillis() << std::endl;
        resumers.emplace_back ([h, ms, resume = resume] { Sleep ((DWORD) ms); resume (h); CloseHandle (h); });
    }
}

static void resumeAll()
{
    for (auto& t : resumers) t.join();
    resumers.clear();
}

// A page is on screen once Chromium's render window sits inside the plug-in window
// (the WebView2 child HWNDs belong to msedgewebview2.exe, EnumChildWindows sees them).
static BOOL CALLBACK findRender (HWND h, LPARAM found)
{
    wchar_t cls[64] {};
    GetClassNameW (h, cls, 64);
    if (wcscmp (cls, L"Chrome_RenderWidgetHostHWND") == 0) { *(bool*) found = true; return FALSE; }
    return TRUE;
}

static bool hasPage (juce::Component* w)
{
    if (w == nullptr || w->getPeer() == nullptr) return false;
    bool found = false;
    EnumChildWindows ((HWND) w->getPeer()->getNativeHandle(), findRender, (LPARAM) &found);
    return found;
}

// hosts for <seconds>; prints PAGE <cycle> <ms> the first time the window has a page
static void pump (double seconds, juce::AudioPluginInstance& p, juce::AudioBuffer<float>& buffer,
                  juce::Component* window = nullptr, int cycle = 0)
{
    const double start = juce::Time::getMillisecondCounterHiRes() * 0.001;
    bool seen = false;
    do
    {
        juce::MidiBuffer midi;
        buffer.clear();
        p.processBlock (buffer, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (16);
        if (window != nullptr && ! seen && hasPage (window))
        {
            seen = true;
            std::cout << "PAGE " << cycle << " " << juce::Time::currentTimeMillis() << std::endl;
        }
    }
    while (juce::Time::getMillisecondCounterHiRes() * 0.001 - start < seconds);

    if (window != nullptr && ! seen)
        std::cout << "WHITE " << cycle << " " << juce::Time::currentTimeMillis() << std::endl;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 5)
    {
        std::cout << "usage: ReopenEditorHost <plugin.vst3> <cycles> <hold seconds> <gap ms>" << std::endl;
        return 2;
    }

    const juce::String path (argv[1]);
    const int cycles    = juce::jlimit (1, 20, juce::String (argv[2]).getIntValue());
    const double hold   = juce::String (argv[3]).getDoubleValue();
    const double gapSec = juce::String (argv[4]).getDoubleValue() * 0.001;
    const int lingerMs  = argc > 5 ? juce::String (argv[5]).getIntValue() : 0;

    juce::AudioPluginFormatManager formats;
    formats.addDefaultFormats();
    juce::VST3PluginFormat vst3;
    juce::OwnedArray<juce::PluginDescription> descs;
    vst3.findAllTypesForFile (descs, path);
    if (descs.isEmpty()) { std::cout << "no plugin found in " << path << std::endl; return 2; }
    std::cout << "plugin " << descs[0]->name << " " << descs[0]->version << std::endl;

    juce::String err;
    auto plug = formats.createPluginInstance (*descs[0], 48000.0, 256, err);
    if (plug == nullptr) { std::cout << "load failed: " << err << std::endl; return 2; }
    plug->prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buffer (2, 256);

    for (int c = 1; c <= cycles; ++c)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (plug->createEditorIfNeeded());
        if (editor == nullptr) { std::cout << "no editor" << std::endl; return 2; }
        auto w = std::make_unique<juce::DocumentWindow> ("ReopenEditorHost", juce::Colours::black,
                                                         juce::DocumentWindow::closeButton);
        w->setUsingNativeTitleBar (true);
        w->setContentNonOwned (editor.get(), true);
        w->setTopLeftPosition (60, 60);
        w->setVisible (true);
        std::cout << "OPEN " << c << " " << juce::Time::currentTimeMillis() << std::endl;

        pump (hold, *plug, buffer, w.get(), c);

        auto browsers = ourBrowsers();
        w = nullptr;
        plug->editorBeingDeleted (editor.get());
        editor = nullptr;
        lingerBrowsers (lingerMs, browsers);   // after the close (it talks to the browser): now it is exiting
        std::cout << "CLOSE " << c << " " << juce::Time::currentTimeMillis() << std::endl;

        pump (gapSec, *plug, buffer);
    }

    plug = nullptr;
    resumeAll();                     // never leave a frozen browser behind
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    std::cout << "END " << juce::Time::currentTimeMillis() << std::endl;
    return 0;
}
