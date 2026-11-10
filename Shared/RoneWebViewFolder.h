#pragma once

// ============================================================================
// RoneWebViewFolder - a plug-in window that is closed and opened again comes up
// at once, never white.
//
// Found 2026-11-10 (Liran in FL: DMT, then Iron - closed the window, opened it
// again from the Channel rack, white for 46 s to minutes). Measured with
// scripts/webview-twin-test/reopen.ps1 and a logging copy of JUCE:
//   - when the last WebView of a profile folder closes, its msedgewebview2
//     browser process stays alive, shutting down, for 20-40 s;
//   - a new window on the SAME folder during that time gets no browser:
//     CreateCoreWebView2Controller fails after 10 s with E_ABORT (0x80004004);
//   - JUCE (8.0.4 and 8.0.12 alike) ignores that failure and never tries
//     again, so the window stays blank (white) for good.
// Gap 300 ms: Iron, DMT, Stucker and Flanger 2.3.0 each had windows that never
// got a page; gap 5 s: every window in about 1 s.
//
// So the folder is picked per window, at the moment its WebView is built:
//   1. a folder another window of this plug-in (same DLL) is using right now -
//      its browser is alive and healthy, the new window simply joins it;
//   2. else the first of <folder>, <folder>-2, -3, -4 whose browser is gone:
//      no msedgewebview2 browser process runs with it as --user-data-dir, and
//      its EBWebView\lockfile is not write-locked (the lock goes a moment
//      BEFORE the process does, so the lock alone is not enough);
//   3. else <folder> (four closes inside half a minute: rare; it waits as before).
// No RONE plug-in page keeps anything in the browser profile (no localStorage),
// so a second folder loses nothing.
//
// Wired once, in RoneUpdatePrompt::addTo - every RONE WebView plug-in passes its
// options through there, so no editor changes. The lease ends with the editor.
//
// Tests: RONE_WEBVIEW_TEST_ROOT=<dir> moves every folder under <dir> (the
// reopen test and the Test Bench never touch a profile a DAW is using). The
// string is also how the bench tells a build with this fix from one without.
// ============================================================================

#include <map>
#include <juce_core/juce_core.h>
#include <juce_gui_extra/juce_gui_extra.h>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <tlhelp32.h>
#endif

class RoneWebViewFolder
{
public:
    RoneWebViewFolder() = default;
    ~RoneWebViewFolder() { release(); }

    juce::WebBrowserComponent::Options apply (juce::WebBrowserComponent::Options options)
    {
       #if JUCE_WINDOWS
        auto wv = options.getWinWebView2BackendOptions();
        auto base = wv.getUserDataFolder();

        if (base == juce::File())
            return options;

        const auto testRoot = juce::SystemStats::getEnvironmentVariable ("RONE_WEBVIEW_TEST_ROOT", {});
        if (testRoot.isNotEmpty())
            base = juce::File (testRoot).getChildFile (base.getFileName());

        release();
        const juce::ScopedLock sl (lock());
        folder = pick (base);
        ++live()[folder.getFullPathName()];

        if (testRoot.isNotEmpty())
            juce::File (testRoot).getChildFile ("folder-picks.txt")
                .appendText (juce::String (juce::Time::currentTimeMillis()) + " " + folder.getFileName() + juce::newLine);
        return options.withWinWebView2Options (wv.withUserDataFolder (folder));
       #else
        return options;
       #endif
    }

    juce::File current() const { return folder; }

    // true while a browser process (alive or still shutting down) holds the folder
    static bool inUse (const juce::File& profile)
    {
       #if JUCE_WINDOWS
        return lockHeld (profile) || browserRunsOn (profile);
       #else
        juce::ignoreUnused (profile);
        return false;
       #endif
    }

private:
   #if JUCE_WINDOWS
    static bool lockHeld (const juce::File& profile)
    {
        const auto lockFile = profile.getChildFile ("EBWebView").getChildFile ("lockfile");
        if (! lockFile.existsAsFile())
            return false;

        // Chromium opens it GENERIC_WRITE, share READ only, delete-on-close:
        // asking for write access fails while that process holds it.
        auto h = CreateFileW (lockFile.getFullPathName().toWideCharPointer(), GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            return GetLastError() == ERROR_SHARING_VIOLATION;

        CloseHandle (h);
        return false;
    }

    // a WebView2 browser process (no --type=) whose --user-data-dir is this folder's EBWebView
    static bool browserRunsOn (const juce::File& profile)
    {
        using QueryFn = LONG (NTAPI*) (HANDLE, ULONG, PVOID, ULONG, PULONG);
        static auto query = (QueryFn) GetProcAddress (GetModuleHandleW (L"ntdll.dll"), "NtQueryInformationProcess");
        if (query == nullptr)
            return false;

        const auto want = profile.getChildFile ("EBWebView").getFullPathName().toLowerCase();
        bool found = false;
        HANDLE snap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return false;

        PROCESSENTRY32W e {};
        e.dwSize = sizeof (e);
        for (BOOL ok = Process32FirstW (snap, &e); ok && ! found; ok = Process32NextW (snap, &e))
        {
            if (_wcsicmp (e.szExeFile, L"msedgewebview2.exe") != 0)
                continue;

            HANDLE h = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, e.th32ProcessID);
            if (h == nullptr)
                continue;

            ULONG len = 0;
            query (h, 60 /* ProcessCommandLineInformation */, nullptr, 0, &len);
            if (len > 0 && len < 65536)
            {
                juce::HeapBlock<char> buf (len, true);
                if (query (h, 60, buf.getData(), len, &len) >= 0)
                {
                    struct CmdLine { USHORT Length, MaximumLength; PWSTR Buffer; };   // UNICODE_STRING
                    auto* us = reinterpret_cast<CmdLine*> (buf.getData());
                    const juce::String cmd (juce::CharPointer_UTF16 (us->Buffer), (size_t) (us->Length / sizeof (WCHAR)));
                    const auto lower = cmd.toLowerCase();
                    found = ! lower.contains ("--type=") && lower.contains (want);
                }
            }
            CloseHandle (h);
        }
        CloseHandle (snap);
        return found;
    }
   #endif

    static juce::File pick (const juce::File& base)
    {
        juce::Array<juce::File> candidates { base };
        for (int i = 2; i <= 4; ++i)
            candidates.add (base.getSiblingFile (base.getFileName() + "-" + juce::String (i)));

        for (auto& c : candidates)                               // 1. join a window of ours
            if (live()[c.getFullPathName()] > 0)
                return c;

        for (auto& c : candidates)                               // 2. a folder nobody holds
            if (! inUse (c))
                return c;

        return base;                                             // 3. all busy: as before
    }

    void release()
    {
        if (folder == juce::File())
            return;

        const juce::ScopedLock sl (lock());
        auto& n = live()[folder.getFullPathName()];
        n = juce::jmax (0, n - 1);
        folder = juce::File();
    }

    // per DLL: windows of THIS plug-in, open now, per folder
    static std::map<juce::String, int>& live()      { static std::map<juce::String, int> m; return m; }
    static juce::CriticalSection& lock()            { static juce::CriticalSection c; return c; }

    juce::File folder;
};
