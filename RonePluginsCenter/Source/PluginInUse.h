#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <tlhelp32.h>
 #pragma comment (lib, "version.lib")
#endif

// ============================================================================
// PluginInUse - which running programs hold a plugin's files right now: a DAW
// with the VST3 loaded, or the plugin's own standalone.
//
// On Windows a loaded DLL cannot be replaced, and a silent Inno install that
// meets one rolls back with exit code 5. When a plugin's UPDATE button opens
// the Center, that plugin is by definition still loaded, so the Center asks
// this every few seconds and installs only once nothing holds the files.
// A DAW keeps a plugin loaded after its window closes (most until they quit),
// so the user is told which program to close - by its product name.
//
// A full scan reads the module list of every process: ~150 ms of CPU with 420
// processes (Liran's PC, 2026-09-23). The Center does one, then re-checks only
// the processes it found (a few ms every few seconds while the user works in
// the DAW), and scans in full once more before it installs.
//
// Programs running elevated cannot be inspected from the (non-elevated)
// Center and are not listed; their install still reports the lock as before.
//
// 2.1.6 (Yuval via Liran, 2026-10-20: "it says it waits for FL to close, but
// FL is closed, in Task Manager too"). FL Studio hosts plug-ins in helper
// programs as well - ilbridge.exe (bridged plug-ins), ILMinihostBridge64.exe,
// ILPluginScanner64.exe - and after a crash or a stuck shutdown one of them
// can stay alive with the plug-in loaded while FL itself is gone; FL64.exe can
// also linger without a window. Now:
//   * a holder is named for what it is: an FL helper as "FL Studio's plug-in
//     host (ilbridge.exe)", a program with no visible window with its file
//     name and "running in the background", so the user can find it;
//   * an FL helper that holds the files while no FL Studio is running at all is
//     an orphan - nothing can use it - and the Center ends it and installs.
// ============================================================================
namespace PluginInUse
{
    struct Holders
    {
        juce::StringArray hosts;            // other programs with the files loaded (a DAW): "FL Studio"
        bool ownApp = false;                // the standalone itself is running
        juce::Array<juce::uint32> pids;     // every holding process, for the cheap re-check
        juce::Array<juce::uint32> flHelpers;   // the holders that are FL Studio's helper programs

        bool isEmpty() const noexcept { return pids.isEmpty(); }
    };

   #if JUCE_WINDOWS
    // FL Studio's own helper programs, which load plug-ins outside FL64.exe.
    inline bool isFlHelper (const juce::String& exeFileName)
    {
        const auto n = exeFileName.toLowerCase();
        return n == "ilbridge.exe" || n == "ilminihostbridge64.exe" || n == "ilminihostbridge.exe"
            || n == "ilpluginscanner64.exe" || n == "ilpluginscanner.exe";
    }

    inline bool isFlMain (const juce::String& exeFileName)
    {
        const auto n = exeFileName.toLowerCase();
        return n == "fl64.exe" || n == "fl.exe" || n == "fl64 (scaled).exe" || n == "fl (scaled).exe";
    }

    // Has the process a window on screen? A DAW that is closing, or hung after a
    // crash, can stay in the process list without one.
    inline bool hasVisibleWindow (DWORD pid)
    {
        struct Search { DWORD pid; bool found; } s { pid, false };
        EnumWindows ([] (HWND w, LPARAM l) -> BOOL
        {
            auto* s = reinterpret_cast<Search*> (l);
            DWORD owner = 0;
            GetWindowThreadProcessId (w, &owner);
            if (owner == s->pid && IsWindowVisible (w) && GetWindow (w, GW_OWNER) == nullptr)
            {
                s->found = true;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM> (&s));
        return s.found;
    }

    // Is any FL Studio itself running (not just a helper)?
    inline bool flStudioRunning()
    {
        HANDLE processes = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
        if (processes == INVALID_HANDLE_VALUE)
            return true;   // cannot tell: assume it is, and end nothing

        PROCESSENTRY32W pe {};
        pe.dwSize = sizeof (pe);
        bool running = false;
        for (BOOL ok = Process32FirstW (processes, &pe); ok && ! running; ok = Process32NextW (processes, &pe))
            running = isFlMain (juce::String (pe.szExeFile));
        CloseHandle (processes);
        return running;
    }

    // "FL Studio" rather than "FL64": the program's own FileDescription, else its file name.
    inline juce::String productName (const juce::String& exePath)
    {
        const auto* path = exePath.toWideCharPointer();
        DWORD unused = 0;
        const DWORD size = GetFileVersionInfoSizeW (path, &unused);

        if (size > 0)
        {
            juce::HeapBlock<char> data (size);
            struct LangCodePage { WORD language, codePage; };
            LangCodePage* translations = nullptr;
            UINT bytes = 0;

            if (GetFileVersionInfoW (path, 0, size, data.getData())
                && VerQueryValueW (data.getData(), L"\\VarFileInfo\\Translation", (LPVOID*) &translations, &bytes)
                && bytes >= sizeof (LangCodePage))
            {
                wchar_t key[64] = {};
                swprintf_s (key, L"\\StringFileInfo\\%04x%04x\\FileDescription",
                            translations[0].language, translations[0].codePage);
                wchar_t* text = nullptr;
                UINT chars = 0;
                if (VerQueryValueW (data.getData(), key, (LPVOID*) &text, &chars) && chars > 1)
                {
                    const auto description = juce::String (text).trim();
                    if (description.isNotEmpty())
                        return description;
                }
            }
        }

        return juce::File (exePath).getFileNameWithoutExtension();
    }

    // Does this process have the bundle or the exe loaded? Adds it to `result` if so.
    inline void check (DWORD pid, const juce::String& bundlePath, const juce::String& exePath, Holders& result)
    {
        HANDLE modules = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (modules == INVALID_HANDLE_VALUE)
            return;   // gone, elevated, or a protected system process

        MODULEENTRY32W me {};
        me.dwSize = sizeof (me);
        juce::String mainModule;   // the first module of a snapshot is the program itself
        bool holds = false;

        for (BOOL m = Module32FirstW (modules, &me); m && ! holds; m = Module32NextW (modules, &me))
        {
            const juce::String path (me.szExePath);
            if (mainModule.isEmpty())
                mainModule = path;

            const auto lower = path.toLowerCase();
            holds = (bundlePath.isNotEmpty() && (lower == bundlePath || lower.startsWith (bundlePath + "\\")))
                 || (exePath.isNotEmpty() && lower == exePath);
        }

        CloseHandle (modules);

        if (! holds)
            return;

        result.pids.addIfNotAlreadyThere ((juce::uint32) pid);
        const auto fileName = juce::File (mainModule).getFileName();

        if (exePath.isNotEmpty() && mainModule.toLowerCase() == exePath)
            result.ownApp = true;
        else if (isFlHelper (fileName))
        {
            result.flHelpers.addIfNotAlreadyThere ((juce::uint32) pid);
            result.hosts.addIfNotAlreadyThere ("FL Studio's plug-in host (" + fileName + ")");
        }
        else if (! hasVisibleWindow (pid))
            result.hosts.addIfNotAlreadyThere (productName (mainModule) + " (" + fileName + ", still running in the background)");
        else
            result.hosts.addIfNotAlreadyThere (productName (mainModule));
    }
   #endif

    // Every process when `onlyThese` is null, else just those processes (the cheap re-check).
    inline Holders find (const juce::File& vst3Bundle, const juce::File& standaloneExe,
                         const juce::Array<juce::uint32>* onlyThese = nullptr)
    {
        Holders result;

       #if JUCE_WINDOWS
        const auto bundlePath = vst3Bundle == juce::File() ? juce::String()
                                                           : vst3Bundle.getFullPathName().toLowerCase();
        const auto exePath = standaloneExe == juce::File() ? juce::String()
                                                           : standaloneExe.getFullPathName().toLowerCase();
        if (bundlePath.isEmpty() && exePath.isEmpty())
            return result;

        if (onlyThese != nullptr)
        {
            for (auto pid : *onlyThese)
                check ((DWORD) pid, bundlePath, exePath, result);
            return result;
        }

        HANDLE processes = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
        if (processes == INVALID_HANDLE_VALUE)
            return result;

        const DWORD self = GetCurrentProcessId();
        PROCESSENTRY32W pe {};
        pe.dwSize = sizeof (pe);

        for (BOOL ok = Process32FirstW (processes, &pe); ok; ok = Process32NextW (processes, &pe))
            if (pe.th32ProcessID != 0 && pe.th32ProcessID != self)
                check (pe.th32ProcessID, bundlePath, exePath, result);

        CloseHandle (processes);
       #else
        juce::ignoreUnused (vst3Bundle, standaloneExe, onlyThese);
       #endif

        return result;
    }

    /** Ends FL Studio helper programs that hold the files while no FL Studio is
        running: orphans left by a crash or a stuck shutdown. Returns how many
        it ended. Anything else is left alone. */
    inline int endOrphanedFlHelpers (const Holders& holders)
    {
        int ended = 0;
       #if JUCE_WINDOWS
        if (holders.flHelpers.isEmpty() || flStudioRunning())
            return 0;

        for (auto pid : holders.flHelpers)
        {
            HANDLE process = OpenProcess (PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD) pid);
            if (process == nullptr)
                continue;

            // Still the same program: a pid can be reused.
            wchar_t path[MAX_PATH] = {};
            DWORD length = MAX_PATH;
            if (QueryFullProcessImageNameW (process, 0, path, &length)
                && isFlHelper (juce::File (juce::String (path)).getFileName())
                && TerminateProcess (process, 1))
            {
                WaitForSingleObject (process, 3000);
                ++ended;
            }
            CloseHandle (process);
        }
       #else
        juce::ignoreUnused (holders);
       #endif
        return ended;
    }
}
