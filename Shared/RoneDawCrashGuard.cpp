// ============================================================================
// RoneDawCrashGuard: tells us when one of our plugins takes a customer's DAW
// down, so we know before they write in.
//
// Windows x64, VST3 only. Shared/RoneBuildHooks.cmake compiles this file into
// every plugin's VST3 target. RoneCrashReporter.h says never to install a crash
// handler inside a DAW, and that stays true: a process-wide filter would take
// the DAW's own crash handling away from it. This does not. What keeps it safe
// inside someone else's process:
//
//   - It handles nothing. The handler always returns CONTINUE_SEARCH, so the
//     DAW sees every exception exactly as if this file did not exist.
//   - It looks at fatal hardware exceptions only. C++ exceptions are not on the
//     list, so a plugin or host using them for control flow costs nothing.
//   - It reports only if our module is at the crash address or on the crashing
//     stack. Another plugin's crash is not ours to report.
//   - It stands last among the vectored handlers, so a fault the host repairs in
//     its own handler never gets this far.
//   - One report per process, static buffers, no heap: the heap may be what broke.
//   - The file is written by a helper thread that the crashing thread waits for
//     1.5 seconds at most, so a lock held at the moment of the crash can delay
//     the DAW's crash dialog by that long and can never hang it.
//   - A module that reported stays mapped until the process ends: the writer may
//     outlive that wait, and the host may unload the plugin once it has caught
//     the crash.
//   - The handler goes away before the DLL's destructors run, and comes in after
//     its constructors. A fault in either is swallowed by the Windows loader and
//     stops the CRT from going on, so a handler removed by a destructor would stay
//     behind, and the next exception in the DAW would jump into unmapped code.
//     That is why the build links with /ENTRY:roneGuardEntry: the loader comes
//     here first. A target built without it simply never arms.
//   - File names only, never paths: a path can hold the customer's name.
//
// The report lands in the same queue as every other report and the Center
// uploads it. See Shared/CRASH-REPORTING.md.
// ============================================================================

#if defined (_WIN32) && defined (_M_X64) && defined (_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>

extern "C" IMAGE_DOS_HEADER __ImageBase;

#ifndef JucePlugin_Name
 #define JucePlugin_Name "RONE plugin"
#endif
#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString ""
#endif

namespace
{
constexpr int   maxFrames       = 32;
constexpr DWORD writerTimeoutMs = 1500;
constexpr int   throttleHours   = 12;   // same crash, same machine: once, like reportError()

std::atomic<bool>  reported { false };
std::atomic<DWORD> busyThread { 0 };
std::atomic<int>   inHandler { 0 };
PVOID handler = nullptr;

const BYTE* moduleStart = nullptr;
const BYTE* moduleEnd   = nullptr;

// Everything the writer needs is gathered at load, when calling Windows is safe.
wchar_t reportsParent[MAX_PATH], reportsRoot[MAX_PATH];
char moduleName[128], hostName[128], buildId[80], osName[24], localeName[24];

struct
{
    DWORD     code;
    bool      hasAccess;
    ULONG_PTR accessKind, accessAddress;
    DWORD     thread;
    int       frames;
    DWORD64   pcs[maxFrames];
} crash;

CONTEXT walkContext;
char report[32 * 1024];

bool isFatal (DWORD code)
{
    switch (code)
    {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_STACK_OVERFLOW:
            return true;
        default:
            return false;
    }
}

const char* exceptionWords (DWORD code)
{
    switch (code)
    {
        case EXCEPTION_ACCESS_VIOLATION:    return "access violation";
        case EXCEPTION_IN_PAGE_ERROR:       return "page error";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
        case EXCEPTION_PRIV_INSTRUCTION:    return "privileged instruction";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:  return "divide by zero";
        case EXCEPTION_STACK_OVERFLOW:      return "stack overflow";
        default:                            return "exception";
    }
}

bool inModule (DWORD64 pc)
{
    const auto* p = reinterpret_cast<const BYTE*> (pc);
    return p >= moduleStart && p < moduleEnd;
}

// The walk stays inside this thread's stack. A fault in the unwinder would land in
// the __except below, but only after the host's own handlers had seen it, and the
// host must see nothing this file did. A frame counts as ours only if it has unwind
// data: with none the walk reads raw stack, where a pointer to our data looks like
// a return address. Rip is 0 on the first frame after a call through a null pointer.
int walkStack (const CONTEXT& start, bool& ours)
{
    const auto* tib = reinterpret_cast<const NT_TIB*> (NtCurrentTeb());
    const auto stackLow = reinterpret_cast<DWORD64> (tib->StackLimit);
    const auto stackHigh = reinterpret_cast<DWORD64> (tib->StackBase);

    int n = 0;
    walkContext = start;

    __try
    {
        while (n < maxFrames && (walkContext.Rip != 0 || n == 0))
        {
            const DWORD64 pc = walkContext.Rip;
            crash.pcs[n++] = pc;

            DWORD64 imageBase = 0;
            auto* fn = RtlLookupFunctionEntry (pc, &imageBase, nullptr);
            ours = ours || (fn != nullptr && inModule (pc));

            if (walkContext.Rsp < stackLow || walkContext.Rsp + 8 > stackHigh)
                break;

            if (fn != nullptr)
            {
                // UNWIND_INFO's fourth byte: the frame register, 0 for none
                const auto frameRegister = reinterpret_cast<const BYTE*> (imageBase + fn->UnwindData)[3] & 15;
                const auto frameBase = frameRegister != 0 ? (&walkContext.Rax)[frameRegister] : walkContext.Rsp;
                if (frameBase < stackLow || frameBase + 8 > stackHigh)
                    break;

                PVOID data = nullptr;
                DWORD64 frame = 0;
                RtlVirtualUnwind (UNW_FLAG_NHANDLER, imageBase, pc, fn, &walkContext, &data, &frame, nullptr);
            }
            else
            {
                walkContext.Rip = *reinterpret_cast<const DWORD64*> (walkContext.Rsp);
                walkContext.Rsp += 8;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return n;
}

// ---- a report built by hand: no heap, no JUCE -------------------------------

char* out;
char* const outEnd = report + sizeof (report) - 1;

void put (const char* s) { while (*s != 0 && out < outEnd) *out++ = *s++; }

void putEscaped (const char* s)
{
    for (; *s != 0 && out + 6 < outEnd; ++s)
    {
        const auto c = static_cast<unsigned char> (*s);
        if (c == '"' || c == '\\') { *out++ = '\\'; *out++ = (char) c; }
        else if (c == '\n')        { *out++ = '\\'; *out++ = 'n'; }
        else if (c >= 0x20)        { *out++ = (char) c; }
    }
}

void toHex (DWORD64 v, char* buffer, int digits)
{
    static const char hex[] = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; --i, v >>= 4)
        buffer[i] = hex[v & 15];
    buffer[digits] = 0;
}

void toDecimal (DWORD64 v, char* buffer, int digits)
{
    for (int i = digits - 1; i >= 0; --i, v /= 10)
        buffer[i] = (char) ('0' + v % 10);
    buffer[digits] = 0;
}

void addHex (char*& dst, DWORD64 v)
{
    char b[17];
    toHex (v, b, 16);
    const char* p = b;
    while (*p == '0' && p[1] != 0) ++p;
    while (*p != 0) *dst++ = *p++;
}

void addText (char*& dst, const char* s, const char* end)
{
    while (*s != 0 && dst < end) *dst++ = *s++;
}

void fileNameOf (const wchar_t* path, char* utf8, int size)
{
    const wchar_t* name = path;
    for (const wchar_t* p = path; *p != 0; ++p)
        if (*p == L'\\' || *p == L'/') name = p + 1;

    if (WideCharToMultiByte (CP_UTF8, 0, name, -1, utf8, size, nullptr, nullptr) == 0)
        utf8[0] = 0;
}

// "RONE Clipper.vst3+0x1A2B3", or "ntdll.dll+0x1234" for code that is not ours.
void describe (DWORD64 pc, char* text, int size)
{
    char* dst = text;
    const char* end = text + size - 24;

    if (inModule (pc))
    {
        addText (dst, moduleName, end);
        addText (dst, "+0x", end);
        addHex (dst, pc - reinterpret_cast<DWORD64> (moduleStart));
    }
    else
    {
        HMODULE other = nullptr;
        wchar_t path[MAX_PATH];
        char name[128];

        if (GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR> (pc), &other)
            && GetModuleFileNameW (other, path, MAX_PATH) != 0)
        {
            fileNameOf (path, name, sizeof (name));
            addText (dst, name, end);
            addText (dst, "+0x", end);
            addHex (dst, pc - reinterpret_cast<DWORD64> (other));
        }
        else
        {
            addText (dst, "0x", end);
            addHex (dst, pc);
        }
    }

    *dst = 0;
}

void childPath (wchar_t* path, const wchar_t* child, const wchar_t* extra = nullptr)
{
    lstrcpynW (path, reportsRoot, MAX_PATH);
    lstrcatW (path, child);
    if (extra != nullptr)
        lstrcatW (path, extra);
}

void stampPath (DWORD hash, wchar_t* path)
{
    wchar_t name[16];
    char hex[9];
    toHex (hash, hex, 8);
    for (int i = 0; i < 9; ++i) name[i] = (wchar_t) hex[i];
    childPath (path, L"\\throttle\\g");
    lstrcatW (path, name);
    lstrcatW (path, L".t");
}

bool sentLately (const wchar_t* stampFile)
{
    WIN32_FILE_ATTRIBUTE_DATA info;
    FILETIME now;
    GetSystemTimeAsFileTime (&now);

    if (! GetFileAttributesExW (stampFile, GetFileExInfoStandard, &info))
        return false;

    const auto age = (ULARGE_INTEGER { now.dwLowDateTime, now.dwHighDateTime }).QuadPart
                   - (ULARGE_INTEGER { info.ftLastWriteTime.dwLowDateTime, info.ftLastWriteTime.dwHighDateTime }).QuadPart;
    return age < ULONGLONG (throttleHours) * 3600 * 10000000;
}

void markSent (const wchar_t* stampFile)
{
    const HANDLE stamp = CreateFileW (stampFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (stamp != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteFile (stamp, "1", 1, &written, nullptr);
        CloseHandle (stamp);
    }
}

void readInstallId (char* id)
{
    wchar_t path[MAX_PATH];
    childPath (path, L"\\install-id.txt");
    id[0] = 0;

    const HANDLE f = CreateFileW (path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
        return;

    DWORD got = 0;
    ReadFile (f, id, 36, &got, nullptr);
    id[got == 36 ? 36 : 0] = 0;
    CloseHandle (f);
}

DWORD WINAPI writeReport (void*)
{
    char first[256], at[256], line[300], hex[17];

    // the first of our frames names the crash, whatever system code it died in
    int ourFrame = 0;
    while (ourFrame < crash.frames && ! inModule (crash.pcs[ourFrame]))
        ++ourFrame;

    if (ourFrame == crash.frames)
        ourFrame = 0;

    describe (crash.pcs[ourFrame], first, sizeof (first));

    DWORD hash = 2166136261u;
    for (const char* p = first; *p != 0; ++p)
        hash = (hash ^ (BYTE) *p) * 16777619u;

    CreateDirectoryW (reportsParent, nullptr);
    CreateDirectoryW (reportsRoot, nullptr);
    wchar_t dir[MAX_PATH];
    childPath (dir, L"\\pending");
    CreateDirectoryW (dir, nullptr);
    childPath (dir, L"\\throttle");
    CreateDirectoryW (dir, nullptr);

    wchar_t stamp[MAX_PATH];
    stampPath (hash, stamp);
    if (sentLately (stamp))
        return 0;

    SYSTEMTIME t;
    GetSystemTime (&t);
    char date[24], part[8];
    char* d = date;
    toDecimal (t.wYear, part, 4);   addText (d, part, date + 23); *d++ = '-';
    toDecimal (t.wMonth, part, 2);  addText (d, part, date + 23); *d++ = '-';
    toDecimal (t.wDay, part, 2);    addText (d, part, date + 23); *d++ = 'T';
    toDecimal (t.wHour, part, 2);   addText (d, part, date + 23); *d++ = ':';
    toDecimal (t.wMinute, part, 2); addText (d, part, date + 23); *d++ = ':';
    toDecimal (t.wSecond, part, 2); addText (d, part, date + 23); *d++ = 'Z';
    *d = 0;

    char installId[40];
    readInstallId (installId);

    out = report;
    put ("{\"type\":\"crash\",\"product\":\""); putEscaped (JucePlugin_Name);
    put ("\",\"version\":\"");                  putEscaped (JucePlugin_VersionString);
    put ("\",\"wrapper\":\"VST3\",\"code\":\"DAW_CRASH\",\"message\":\"");
    putEscaped (first); put (" "); putEscaped (exceptionWords (crash.code)); put (" in "); putEscaped (hostName);

    put ("\",\"details\":\"host: ");            putEscaped (hostName);
    toHex (crash.code, hex, 8);
    put ("\\nexception: 0x");                   put (hex);
    put (" ");                                  putEscaped (exceptionWords (crash.code));

    if (crash.hasAccess)
    {
        put (crash.accessKind == 0 ? " reading 0x" : crash.accessKind == 1 ? " writing 0x" : " running 0x");
        char* a = hex;
        addHex (a, crash.accessAddress);
        *a = 0;
        put (hex);
    }

    describe (crash.pcs[0], at, sizeof (at));
    put ("\\ncrashed at: ");                    putEscaped (at);
    put ("\\nbuild: ");                         putEscaped (buildId);

    put ("\",\"stack\":\"");
    for (int i = 0; i < crash.frames; ++i)
    {
        describe (crash.pcs[i], line, sizeof (line));
        putEscaped (line);
        put ("\\n");
    }

    put ("\",\"os\":\"");                       putEscaped (osName);
    put ("\",\"arch\":\"x64\",\"locale\":\"");  putEscaped (localeName);
    put ("\",\"install_id\":\"");               putEscaped (installId);
    put ("\",\"time\":\"");                     put (date);
    put ("\"}");
    *out = 0;

    // written under another name first: the Center must never read half a report
    FILETIME now;
    GetSystemTimeAsFileTime (&now);
    toHex ((DWORD64 (now.dwHighDateTime) << 32) | now.dwLowDateTime, hex, 16);
    wchar_t name[32], tmp[MAX_PATH], final[MAX_PATH];
    name[0] = L'g';
    for (int i = 0; i < 17; ++i) name[i + 1] = (wchar_t) hex[i];
    childPath (tmp, L"\\pending\\", name);
    lstrcatW (tmp, L".tmp");
    childPath (final, L"\\pending\\", name);
    lstrcatW (final, L".json");

    const HANDLE f = CreateFileW (tmp, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        const auto length = static_cast<DWORD> (out - report);
        const auto ok = WriteFile (f, report, length, &written, nullptr) && written == length;
        CloseHandle (f);
        if (ok && MoveFileW (tmp, final))
            markSent (stamp);
        else
            DeleteFileW (tmp);
    }

    return 0;
}

bool pinModule()
{
    HMODULE self = nullptr;
    return GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                               reinterpret_cast<LPCWSTR> (&__ImageBase), &self) != FALSE;
}

void watch (EXCEPTION_POINTERS* info)
{
    const auto* record = info->ExceptionRecord;

    if (! isFatal (record->ExceptionCode) || reported.load() || IsDebuggerPresent())
        return;

    DWORD idle = 0;
    if (! busyThread.compare_exchange_strong (idle, GetCurrentThreadId()))
        return;     // this thread's walk faulted, or another thread is in here

    const auto pc = reinterpret_cast<DWORD64> (record->ExceptionAddress);
    bool ours = inModule (pc);

    crash.code = record->ExceptionCode;
    crash.thread = GetCurrentThreadId();
    crash.hasAccess = (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2);
    crash.accessKind = crash.hasAccess ? record->ExceptionInformation[0] : 0;
    crash.accessAddress = crash.hasAccess ? record->ExceptionInformation[1] : 0;

    if (record->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
    {
        crash.pcs[0] = pc;      // almost no stack left to walk with
        crash.frames = 1;
    }
    else
    {
        crash.frames = walkStack (*info->ContextRecord, ours);
    }

    if (ours && ! reported.exchange (true) && pinModule())
    {
        if (const HANDLE writer = CreateThread (nullptr, 128 * 1024, writeReport, nullptr, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr))
        {
            WaitForSingleObject (writer, writerTimeoutMs);
            CloseHandle (writer);
        }
    }

    busyThread.store (0);
}

LONG CALLBACK onException (EXCEPTION_POINTERS* info)
{
    inHandler.fetch_add (1);
    watch (info);
    inHandler.fetch_sub (1);
    return EXCEPTION_CONTINUE_SEARCH;
}

void readBuildId()
{
    const auto* base = reinterpret_cast<const BYTE*> (&__ImageBase);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*> (base + __ImageBase.e_lfanew);

    moduleStart = base;
    moduleEnd = base + nt->OptionalHeader.SizeOfImage;

    char* dst = buildId;
    const char* end = buildId + sizeof (buildId) - 1;
    char hex[33];
    toHex (nt->FileHeader.TimeDateStamp, hex, 8);
    addText (dst, hex, end);
    *dst = 0;

    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    const auto* entries = reinterpret_cast<const IMAGE_DEBUG_DIRECTORY*> (base + dir.VirtualAddress);

    for (DWORD i = 0; i < dir.Size / sizeof (IMAGE_DEBUG_DIRECTORY); ++i)
    {
        if (entries[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW || entries[i].AddressOfRawData == 0)
            continue;

        struct Rsds { DWORD signature; GUID guid; DWORD age; };
        const auto* cv = reinterpret_cast<const Rsds*> (base + entries[i].AddressOfRawData);

        if (cv->signature != 0x53445352)    // "RSDS"
            continue;

        // the form dbghelp and symbol stores use: GUID with no dashes, then the age
        addText (dst, " pdb ", end);
        toHex (cv->guid.Data1, hex, 8); addText (dst, hex, end);
        toHex (cv->guid.Data2, hex, 4); addText (dst, hex, end);
        toHex (cv->guid.Data3, hex, 4); addText (dst, hex, end);
        for (int b = 0; b < 8; ++b) { toHex (cv->guid.Data4[b], hex, 2); addText (dst, hex, end); }
        addText (dst, "-", end);
        addHex (dst, cv->age);
        *dst = 0;
        break;
    }
}

void readOsName()
{
    lstrcpynA (osName, "Windows", sizeof (osName));

    using RtlGetVersionFn = LONG (WINAPI*) (OSVERSIONINFOW*);
    if (const auto ntdll = GetModuleHandleW (L"ntdll.dll"))
    {
        if (const auto get = reinterpret_cast<RtlGetVersionFn> (reinterpret_cast<void*> (GetProcAddress (ntdll, "RtlGetVersion"))))
        {
            OSVERSIONINFOW v {};
            v.dwOSVersionInfoSize = sizeof (v);
            if (get (&v) == 0)
                lstrcpynA (osName, v.dwBuildNumber >= 22000 ? "Windows 11" : v.dwMajorVersion == 10 ? "Windows 10" : "Windows", sizeof (osName));
        }
    }
}

// Both run from the entry below, around the CRT, so neither may be inlined into it: the
// stack cookie their buffers check is only set inside _DllMainCRTStartup.
__declspec(noinline) void arm()
{
    wchar_t path[MAX_PATH];

    const auto length = GetEnvironmentVariableW (L"APPDATA", reportsParent, MAX_PATH - 80);
    if (length == 0 || length >= MAX_PATH - 80)
        return;

    lstrcatW (reportsParent, L"\\RonePlugins");
    lstrcpyW (reportsRoot, reportsParent);
    lstrcatW (reportsRoot, L"\\CrashReports");

    if (GetModuleFileNameW (reinterpret_cast<HMODULE> (&__ImageBase), path, MAX_PATH) == 0)
        return;
    fileNameOf (path, moduleName, sizeof (moduleName));

    if (GetModuleFileNameW (nullptr, path, MAX_PATH) == 0)
        return;
    fileNameOf (path, hostName, sizeof (hostName));

    wchar_t locale[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName (locale, LOCALE_NAME_MAX_LENGTH) != 0)
        WideCharToMultiByte (CP_UTF8, 0, locale, -1, localeName, sizeof (localeName), nullptr, nullptr);

    readBuildId();
    readOsName();

    handler = AddVectoredExceptionHandler (0, onException);
}

__declspec(noinline) void disarm (bool unloading)
{
    if (handler == nullptr)
        return;

    RemoveVectoredExceptionHandler (handler);
    handler = nullptr;

    if (! unloading)
        return;     // the process is ending, nothing gets unmapped

    // Removal does not wait for a thread already inside the handler, and this code is
    // about to go. Two quiet looks, for one that was just on its way in. Bounded, since
    // this runs under the loader lock: a thread in there waits for the writer at most.
    const auto start = GetTickCount64();

    for (int quiet = 0; quiet < 2 && GetTickCount64() - start < writerTimeoutMs + 500;)
    {
        quiet = inHandler.load() == 0 ? quiet + 1 : 0;
        Sleep (1);
    }
}
}

extern "C" BOOL WINAPI _DllMainCRTStartup (HINSTANCE, DWORD, LPVOID);

extern "C" __declspec(safebuffers) BOOL WINAPI roneGuardEntry (HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_DETACH)
        disarm (reserved == nullptr);

    const auto result = _DllMainCRTStartup (instance, reason, reserved);

    if (reason == DLL_PROCESS_ATTACH && result)
        arm();

    return result;
}

#endif
