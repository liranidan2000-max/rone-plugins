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
// ============================================================================
namespace PluginInUse
{
    struct Holders
    {
        juce::StringArray hosts;            // other programs with the files loaded (a DAW): "FL Studio"
        bool ownApp = false;                // the standalone itself is running
        juce::Array<juce::uint32> pids;     // every holding process, for the cheap re-check

        bool isEmpty() const noexcept { return pids.isEmpty(); }
    };

   #if JUCE_WINDOWS
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
        if (exePath.isNotEmpty() && mainModule.toLowerCase() == exePath)
            result.ownApp = true;
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
}
