// The host side of the proof. It catches every exception the way FL Studio does, so a case that
// reaches its end shows the guard never swallowed one. Each case is its own process with its own
// temporary APPDATA, so nothing reaches the real queue: a plugin that reported stays mapped until
// the process ends, and a guard that kills the host must only take that one case with it.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

extern "C" void asm_foreign_crash (void*);

namespace
{
constexpr DWORD diedCode = 0xDEAD0001;

std::wstring root, plugin;
int failures = 0;

using IntFn = int (*)();
using VoidFn = void (*)();
using DivideFn = int (*) (int);
using CallbackFn = int (*) (IntFn);
using RecurseFn = int (*) (int);
using DataFn = void* (*)();

void check (bool ok, const char* what)
{
    printf ("  %s %s\n", ok ? "PASS" : "FAIL", what);
    if (! ok) ++failures;
}

std::vector<std::string> reports()
{
    std::vector<std::string> out;
    WIN32_FIND_DATAW d;
    const auto dir = root + L"\\RonePlugins\\CrashReports\\pending\\";
    const auto h = FindFirstFileW ((dir + L"*.json").c_str(), &d);
    if (h == INVALID_HANDLE_VALUE) return out;
    do
    {
        const auto f = CreateFileW ((dir + d.cFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        std::string text (d.nFileSizeLow, 0);
        DWORD got = 0;
        ReadFile (f, text.data(), d.nFileSizeLow, &got, nullptr);
        CloseHandle (f);
        out.push_back (text);
    } while (FindNextFileW (h, &d));
    FindClose (h);
    return out;
}

// waits for the writer: a case that held it back lets it go just before calling this
std::vector<std::string> reportsAfter (DWORD ms)
{
    const auto t0 = GetTickCount64();
    while (reports().empty() && GetTickCount64() - t0 < ms)
        Sleep (20);
    return reports();
}

// Strict enough to catch a bad escape, a truncated report or a stray control character.
struct Json
{
    const char* p;

    void space() { while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') ++p; }
    bool string()
    {
        if (*p++ != '"') return false;
        for (;; ++p)
        {
            if (*p == 0 || (unsigned char) *p < 0x20) return false;
            if (*p == '"') { ++p; return true; }
            if (*p == '\\' && strchr ("\"\\/bfnrt", *++p) == nullptr) return false;
        }
    }
    bool object()
    {
        ++p;
        space();
        if (*p == '}') { ++p; return true; }
        for (;;)
        {
            space();
            if (! string()) return false;
            space();
            if (*p++ != ':') return false;
            space();
            if (! string()) return false;
            space();
            if (*p == '}') { ++p; return true; }
            if (*p++ != ',') return false;
        }
    }
};

bool validJson (const std::string& text)
{
    Json j { text.c_str() };
    j.space();
    if (*j.p != '{' || ! j.object()) return false;
    j.space();
    return *j.p == 0;
}

// returns the exception code the host saw, 0 if none reached it
template <typename F> DWORD hostCall (F&& f)
{
    __try { f(); return 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}

__declspec(noinline) int hostCrashes()
{
    volatile int* p = nullptr;
    return *p;
}

bool hostSurvivesAnotherException()
{
    Sleep (200);
    return hostCall ([] { hostCrashes(); }) == EXCEPTION_ACCESS_VIOLATION
        && hostCall ([] { RaiseException (0xE0000001, 0, 0, nullptr); }) == 0xE0000001;
}

FARPROC fn (HMODULE m, const char* name) { return GetProcAddress (m, name); }

struct Loaded
{
    HMODULE m = LoadLibraryW (plugin.c_str());
    ~Loaded() { if (m != nullptr) FreeLibrary (m); }
};

using LockFn = LONG (NTAPI*) (ULONG, ULONG*, void**);
using UnlockFn = LONG (NTAPI*) (ULONG, void*);

struct LoaderLock
{
    void* cookie = nullptr;
    LoaderLock()  { ((LockFn) GetProcAddress (GetModuleHandleW (L"ntdll.dll"), "LdrLockLoaderLock")) (0, nullptr, &cookie); }
    ~LoaderLock() { ((UnlockFn) GetProcAddress (GetModuleHandleW (L"ntdll.dll"), "LdrUnlockLoaderLock")) (0, cookie); }
};

// another thread sits on the loader lock until told to let go, so the writer cannot start
struct LoaderLockElsewhere
{
    HANDLE held = CreateEventW (nullptr, TRUE, FALSE, nullptr), letGo = CreateEventW (nullptr, TRUE, FALSE, nullptr);
    std::thread thread { [this] { LoaderLock lock; SetEvent (held); WaitForSingleObject (letGo, INFINITE); } };

    LoaderLockElsewhere() { WaitForSingleObject (held, INFINITE); }
    ~LoaderLockElsewhere() { release(); }
    void release() { SetEvent (letGo); if (thread.joinable()) thread.join(); }
};

std::atomic<int> avSeen { 0 };

LONG CALLBACK countFaults (EXCEPTION_POINTERS* e)
{
    if (e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        ++avSeen;
    return EXCEPTION_CONTINUE_SEARCH;
}

// ---- the cases --------------------------------------------------------------------------------

void crashInPlugin()
{
    Loaded p;
    check (p.m != nullptr, "plugin loads");
    const auto code = hostCall ([&] { ((IntFn) fn (p.m, "crash_null")) (); });
    check (code == EXCEPTION_ACCESS_VIOLATION, "the host still got the exception");
    auto r = reports();
    check (r.size() == 1, "one report in the queue");
    if (! r.empty())
    {
        puts (r[0].c_str());
        check (validJson (r[0]), "valid JSON");
        check (r[0].find ("\"code\":\"DAW_CRASH\"") != std::string::npos, "marked DAW_CRASH");
        check (r[0].find ("\"product\":\"RONE GuardTest\"") != std::string::npos, "names the product");
        check (r[0].find ("\"version\":\"9.9.9\"") != std::string::npos, "names the version");
        check (r[0].find ("RONE GuardTest.vst3+0x") != std::string::npos, "names module and offset");
        check (r[0].find ("reading 0x0") != std::string::npos, "says what was accessed");
        check (r[0].find ("guard_proof.exe") != std::string::npos, "names the host");
        check (r[0].find (" pdb ") != std::string::npos, "carries the build id");
        check (r[0].find (":\\\\") == std::string::npos && r[0].find ("Users") == std::string::npos, "holds no path");
    }
    hostCall ([&] { ((VoidFn) fn (p.m, "crash_write")) (); });
    check (reports().size() == 1, "a second crash in the same process adds nothing");
}

void crashInHost()
{
    Loaded p;
    check (hostCall ([] { hostCrashes(); }) == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    check (reports().empty(), "no report");
}

void cppException()
{
    Loaded p;
    const auto throwIt = (VoidFn) fn (p.m, "throw_cpp");
    bool caught = false;
    try { throwIt(); } catch (const std::exception&) { caught = true; }
    check (caught, "the C++ exception arrived normally");
    check (reports().empty(), "no report");
}

void crashInCallback()
{
    Loaded p;
    const auto code = hostCall ([&] { ((CallbackFn) fn (p.m, "call_host")) (hostCrashes); });
    check (code == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    auto r = reports();
    check (r.size() == 1, "reported: the plugin is on the stack");
    if (! r.empty())
        check (r[0].find ("crashed at: guard_proof.exe+0x") != std::string::npos, "says the crash was in the host");
}

void divideByZero()
{
    Loaded p;
    check (hostCall ([&] { ((DivideFn) fn (p.m, "crash_divide")) (0); }) == EXCEPTION_INT_DIVIDE_BY_ZERO, "the host got it");
    check (reports().size() == 1, "reported");
}

void sameCrashAgain()
{
    Loaded p;
    hostCall ([&] { ((IntFn) fn (p.m, "crash_null")) (); });
    check (reports().size() == 1, "one report, however many runs share this queue");
}

DWORD WINAPI recurseInPlugin (void* m)
{
    const auto recurse = (RecurseFn) fn ((HMODULE) m, "crash_recursion");
    return hostCall ([&] { recurse (0); });
}

DWORD onItsOwnThread (LPTHREAD_START_ROUTINE body, void* argument)
{
    DWORD code = 0;
    const auto thread = CreateThread (nullptr, 0, body, argument, 0, nullptr);
    WaitForSingleObject (thread, 10000);
    GetExitCodeThread (thread, &code);
    CloseHandle (thread);
    return code;
}

void stackOverflow()
{
    Loaded p;
    check (onItsOwnThread (recurseInPlugin, p.m) == EXCEPTION_STACK_OVERFLOW, "the host got it");
    check (reports().size() == 1, "reported");
}

void overflowInSystemCode()
{
    Loaded p;
    const auto code = onItsOwnThread ([] (void* m) -> DWORD
    {
        const auto send = (VoidFn) fn ((HMODULE) m, "crash_through_user32");
        return hostCall ([&] { send(); });
    }, p.m);
    check (code == EXCEPTION_STACK_OVERFLOW, "the host got it");
    auto r = reports();
    check (r.size() == 1, "reported, though the fault is outside the plugin");
    if (! r.empty())
    {
        puts (r[0].c_str());
        check (validJson (r[0]), "valid JSON");
        check (r[0].find ("\"message\":\"RONE GuardTest.vst3+0x") != std::string::npos, "the message names our frame");
        check (r[0].find ("stack overflow") != std::string::npos, "and says what happened");
        check (r[0].find ("crashed at: RONE GuardTest") == std::string::npos, "the fault really was in system code");
    }
}

#pragma warning (disable: 4717)
__declspec(noinline) int recurseInHost (int n)
{
    volatile char pad[4096];
    pad[0] = (char) n;
    return recurseInHost (n + 1) + pad[0];
}

void overflowInTheHost()
{
    Loaded p;
    const auto code = onItsOwnThread ([] (void*) -> DWORD { return hostCall ([] { recurseInHost (0); }); }, nullptr);
    check (code == EXCEPTION_STACK_OVERFLOW, "the host got it");
    Sleep (200);
    check (reports().empty(), "no report: our frames are not on that stack");
    check (hostSurvivesAnotherException(), "the host lives on");
}

// A thread can neither start nor end while the loader lock is elsewhere, so this one starts first,
// and says what it saw before it tries to end.
HANDLE overflowReady = nullptr, overflowGo = nullptr, overflowSeen = nullptr;
std::atomic<DWORD> overflowCode { 0 };

DWORD WINAPI recurseAndSay (void* m)
{
    SetEvent (overflowReady);
    WaitForSingleObject (overflowGo, INFINITE);
    overflowCode = recurseInPlugin (m);
    SetEvent (overflowSeen);
    return 0;
}

void hostUnloadsAfterOverflow()
{
    overflowReady = CreateEventW (nullptr, TRUE, FALSE, nullptr);
    overflowGo = CreateEventW (nullptr, TRUE, FALSE, nullptr);
    overflowSeen = CreateEventW (nullptr, TRUE, FALSE, nullptr);
    const auto m = LoadLibraryW (plugin.c_str());
    CloseHandle (CreateThread (nullptr, 0, recurseAndSay, m, 0, nullptr));
    WaitForSingleObject (overflowReady, 10000);
    {
        LoaderLockElsewhere lock;   // the helper cannot start, so the crashing thread stops waiting for it
        SetEvent (overflowGo);
        WaitForSingleObject (overflowSeen, 10000);
    }
    check (overflowCode == EXCEPTION_STACK_OVERFLOW, "the host got it");
    FreeLibrary (m);
    check (GetModuleHandleW (L"RONE GuardTest.vst3") != nullptr, "the plugin stays mapped for the helper");
    Sleep (300);
    check (reports().empty(), "the helper gave up: the stack it would walk is gone");
    check (hostSurvivesAnotherException(), "the host lives on");
}

void crashHoldingLoaderLock()
{
    Loaded p;
    DWORD code;
    ULONGLONG ms;
    {
        LoaderLock lock;
        const auto t0 = GetTickCount64();
        code = hostCall ([&] { ((IntFn) fn (p.m, "crash_null")) (); });
        ms = GetTickCount64() - t0;
    }
    check (code == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    printf ("  waited %llu ms\n", (unsigned long long) ms);
    check (ms < 2500, "the wait is bounded");
    check (reportsAfter (3000).size() == 1, "the report is written once the lock is free");
}

void crashWhileLoaderLockIsElsewhere()
{
    Loaded p;
    LoaderLockElsewhere lock;
    const auto t0 = GetTickCount64();
    const auto code = hostCall ([&] { ((IntFn) fn (p.m, "crash_null")) (); });
    const auto ms = GetTickCount64() - t0;
    check (code == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    printf ("  waited %llu ms\n", (unsigned long long) ms);
    check (ms < 2500, "the wait is bounded");
    lock.release();
    check (reportsAfter (3000).size() == 1, "the report is written once the lock is free");
}

void hostUnloadsWhatItCaught()
{
    const auto m = LoadLibraryW (plugin.c_str());
    {
        LoaderLockElsewhere lock;
        check (hostCall ([&] { ((IntFn) fn (m, "crash_null")) (); }) == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    }
    FreeLibrary (m);    // the writer has not run a single instruction yet
    check (GetModuleHandleW (L"RONE GuardTest.vst3") != nullptr, "the plugin stays mapped for the writer");
    check (reportsAfter (3000).size() == 1, "the report still arrives");
    check (hostSurvivesAnotherException(), "the host lives on");
}

void unloadedThenHostCrash()
{
    { Loaded p; }
    check (GetModuleHandleW (L"RONE GuardTest.vst3") == nullptr, "really unloaded");
    check (hostSurvivesAnotherException(), "no dangling handler: the host got it and lived");
    check (reports().empty(), "no report");
}

void nullCall()
{
    Loaded p;
    const auto call = (CallbackFn) fn (p.m, "crash_null_call");
    check (hostCall ([&] { call (nullptr); }) == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    auto r = reports();
    check (r.size() == 1, "reported");
    if (! r.empty())
    {
        check (r[0].find ("crashed at: 0x0") != std::string::npos, "says it died at address 0");
        check (r[0].find ("\"message\":\"RONE GuardTest.vst3+0x") != std::string::npos, "and names our frame");
    }
}

void faultInStaticConstructor()
{
    const auto m = LoadLibraryW (plugin.c_str());
    check (m == nullptr, "the load fails, as it does with no guard");
    check (GetModuleHandleW (L"RONE GuardTest.vst3") == nullptr, "nothing stays mapped");
    check (hostSurvivesAnotherException(), "the host lives through its next exception");
}

void faultInStaticDestructor()
{
    const auto m = LoadLibraryW (plugin.c_str());
    check (FreeLibrary (m) != FALSE, "the unload returns");
    check (GetModuleHandleW (L"RONE GuardTest.vst3") == nullptr, "really unloaded");
    check (hostSurvivesAnotherException(), "the host lives through its next exception");
}

void faultInLateDestructor()
{
    const auto m = LoadLibraryW (plugin.c_str());
    ((VoidFn) fn (m, "touch_late_static")) ();
    check (FreeLibrary (m) != FALSE, "the unload returns");
    check (GetModuleHandleW (L"RONE GuardTest.vst3") == nullptr, "really unloaded");
    check (hostSurvivesAnotherException(), "the host lives through its next exception");
}

void unloadWhileOtherThreadsTakeExceptions()
{
    std::atomic<bool> stop { false };
    const auto raise = [&] { while (! stop) hostCall ([] { RaiseException (0xE0000001, 0, 0, nullptr); }); };
    const auto fault = [&] { while (! stop) hostCall ([] { hostCrashes(); }); };
    std::thread threads[] = { std::thread (raise), std::thread (raise), std::thread (fault), std::thread (fault) };

    int cycles = 0;
    for (const auto t0 = GetTickCount64(); GetTickCount64() - t0 < 4000; ++cycles)
        FreeLibrary (LoadLibraryW (plugin.c_str()));

    stop = true;
    for (auto& t : threads) t.join();
    printf ("  %d load and unload cycles\n", cycles);
    check (cycles > 50, "enough of them to mean something");
    check (reports().empty(), "no report");
}

// a fault a host's handler repairs and resumes from, as copy protection does
volatile int repaired = 0;
volatile int* volatile repairable = nullptr;

LONG CALLBACK repairFault (EXCEPTION_POINTERS* e)
{
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || repairable != nullptr)
        return EXCEPTION_CONTINUE_SEARCH;
    repairable = &repaired;
    e->ContextRecord->Rax = e->ContextRecord->Rcx = e->ContextRecord->Rdx = e->ContextRecord->R8 = reinterpret_cast<DWORD64> (&repaired);
    return EXCEPTION_CONTINUE_EXECUTION;
}

__declspec(noinline) int readRepairable() { return *repairable; }

void faultTheHostRepairs()
{
    AddVectoredExceptionHandler (0, repairFault);   // set up before the plugin loads, and last in line
    Loaded p;
    check (hostCall ([&] { ((CallbackFn) fn (p.m, "call_host")) (readRepairable); }) == 0, "nothing reached the host");
    Sleep (200);
    check (reports().empty(), "no report: a host that repairs a fault has no crash");
}

void foreignCrashWithOurDataOnTheStack()
{
    Loaded p;
    const auto data = ((DataFn) fn (p.m, "plugin_data")) ();
    check (hostCall ([&] { asm_foreign_crash (data); }) == EXCEPTION_ACCESS_VIOLATION, "the host got it");
    check (reports().empty(), "no report: a pointer to our data is not our frame");
}

// Windows cannot unwind a broken frame either, so no __except past it ever runs: a handler behind
// the guards' is the last anyone hears of this crash. It looks at what they did and ends the case.
LONG CALLBACK lastLook (EXCEPTION_POINTERS*)
{
    printf ("  the host's handlers saw %d access violation(s)\n", avSeen.load());
    check (avSeen == 1, "just the real fault, nothing from the walk");
    auto r = reports();
    check (r.size() == 1, "one report, from the plugin that crashed");
    if (! r.empty())
        check (r[0].find ("reading 0x0") != std::string::npos && r[0].find ("crashed at: RONE GuardTest.vst3+0x") != std::string::npos, "and it describes the real fault");
    TerminateProcess (GetCurrentProcess(), failures);
    return EXCEPTION_CONTINUE_SEARCH;
}

void brokenFrame()
{
    AddVectoredExceptionHandler (1, countFaults);
    Loaded p;
    AddVectoredExceptionHandler (0, lastLook);
    hostCall ([&] { ((VoidFn) fn (p.m, "crash_bad_frame")) (); });
    check (false, "never gets here");
}

void twoPluginsLoaded()
{
    const auto second = plugin.substr (0, plugin.size() - 5) + L" Two.vst3";
    CopyFileW (plugin.c_str(), second.c_str(), FALSE);
    AddVectoredExceptionHandler (1, countFaults);
    Loaded p;
    check (LoadLibraryW (second.c_str()) != nullptr, "both load");
    AddVectoredExceptionHandler (0, lastLook);
    hostCall ([&] { ((VoidFn) fn (p.m, "crash_bad_frame")) (); });
    check (false, "never gets here");
}

struct Case
{
    const char* name;
    void (*run)();
    const char* fault = nullptr;    // the variable that makes the plugin fault
    bool sameQueue = false;         // keeps the previous case's queue
};

const Case cases[] =
{
    { "a crash inside the plugin", crashInPlugin },
    { "a crash in the host, plugin loaded but not involved", crashInHost },
    { "a C++ exception through the plugin", cppException },
    { "a crash in host code the plugin called back into", crashInCallback },
    { "a divide by zero in the plugin", divideByZero },
    { "a crash, for the throttle", sameCrashAgain },
    { "the same crash in a new process", sameCrashAgain, nullptr, true },
    { "stack overflow in the plugin", stackOverflow },
    { "a crash while this thread holds the loader lock", crashHoldingLoaderLock },
    { "a crash while another thread holds the loader lock", crashWhileLoaderLockIsElsewhere },
    { "the host unloads the plugin it caught, before the writer ran", hostUnloadsWhatItCaught },
    { "the plugin unloaded, then a crash in the host", unloadedThenHostCrash },
    { "a call through a null pointer", nullCall },
    { "a fault in a static constructor", faultInStaticConstructor, "GUARD_CTOR_FAULT" },
    { "a fault in a static destructor", faultInStaticDestructor, "GUARD_DTOR_FAULT" },
    { "a fault in the destructor of a static built after load", faultInLateDestructor, "GUARD_LATE_DTOR_FAULT" },
    { "unloading while other threads take exceptions", unloadWhileOtherThreadsTakeExceptions },
    { "a fault the host repairs", faultTheHostRepairs },
    { "a foreign crash with a pointer to our data on the stack", foreignCrashWithOurDataOnTheStack },
    { "a broken frame pointer", brokenFrame },
    { "a broken frame pointer with two plugins loaded", twoPluginsLoaded },
    { "a stack overflow with the fault in system code", overflowInSystemCode },
    { "a stack overflow in the host, plugin loaded", overflowInTheHost },
    { "the host unloads the plugin after a stack overflow, before the helper ran", hostUnloadsAfterOverflow },
};

LONG WINAPI lastResort (EXCEPTION_POINTERS* e)
{
    printf ("  DIED: unhandled 0x%08lX at %p\n", e->ExceptionRecord->ExceptionCode, e->ExceptionRecord->ExceptionAddress);
    fflush (stdout);
    TerminateProcess (GetCurrentProcess(), diedCode);
    return EXCEPTION_EXECUTE_HANDLER;
}

void emptyQueue()
{
    for (const wchar_t* sub : { L"\\RonePlugins\\CrashReports\\pending\\", L"\\RonePlugins\\CrashReports\\throttle\\" })
    {
        const auto dir = root + sub;
        WIN32_FIND_DATAW d;
        const auto h = FindFirstFileW ((dir + L"*").c_str(), &d);
        if (h == INVALID_HANDLE_VALUE) continue;
        do { DeleteFileW ((dir + d.cFileName).c_str()); } while (FindNextFileW (h, &d));
        FindClose (h);
    }
}

// A real plugin, in the same fake host: its factory is called with a null class id, which it reads.
int realPlugin (const wchar_t* path)
{
    emptyQueue();
    using FactoryFn = void** (*)();
    using CreateFn = long (__stdcall*) (void*, const char*, const char*, void**);

    const auto m = LoadLibraryW (path);
    if (m == nullptr) { printf ("cannot load, error %lu\n", GetLastError()); return 1; }

    const auto factory = ((FactoryFn) GetProcAddress (m, "GetPluginFactory")) ();
    const auto create = reinterpret_cast<CreateFn> ((*reinterpret_cast<void***> (factory))[6]);
    const auto code = hostCall ([&] { void* object = nullptr; create (factory, nullptr, nullptr, &object); });
    printf ("the host saw 0x%08lX\n", code);

    auto r = reportsAfter (3000);
    printf ("%zu report(s)\n", r.size());
    for (const auto& text : r) puts (text.c_str());
    FreeLibrary (m);
    return r.size() == 1 ? 0 : 1;
}

int runCase (const std::wstring& exe, int index)
{
    if (cases[index].fault != nullptr)
        SetEnvironmentVariableA (cases[index].fault, "1");

    auto command = L"\"" + exe + L"\" --case " + std::to_wstring (index);
    STARTUPINFOW si { sizeof (si) };
    PROCESS_INFORMATION pi {};
    const auto started = CreateProcessW (exe.c_str(), command.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);

    if (cases[index].fault != nullptr)
        SetEnvironmentVariableA (cases[index].fault, nullptr);

    if (! started)
        return -1;

    DWORD code = STILL_ACTIVE;
    if (WaitForSingleObject (pi.hProcess, 30000) == WAIT_TIMEOUT)
    {
        puts ("  FAIL hung");
        TerminateProcess (pi.hProcess, 1);
    }
    GetExitCodeProcess (pi.hProcess, &code);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);
    return (int) code;
}
}

int wmain (int argc, wchar_t* argv[])
{
    wchar_t tmp[MAX_PATH], exe[MAX_PATH];
    GetTempPathW (MAX_PATH, tmp);
    root = std::wstring (tmp) + L"rone-guard-proof";
    CreateDirectoryW (root.c_str(), nullptr);
    SetEnvironmentVariableW (L"APPDATA", root.c_str());
    setvbuf (stdout, nullptr, _IONBF, 0);

    GetModuleFileNameW (nullptr, exe, MAX_PATH);
    plugin = exe;
    plugin = plugin.substr (0, plugin.find_last_of (L'\\')) + L"\\RONE GuardTest.vst3";

    if (argc == 3 && std::wstring (argv[1]) == L"--case")
    {
        SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        SetUnhandledExceptionFilter (lastResort);
        cases[_wtoi (argv[2])].run();
        return failures;
    }

    if (argc == 2)
        return realPlugin (argv[1]);

    int failed = 0;
    for (int i = 0; i < (int) std::size (cases); ++i)
    {
        printf ("%d. %s\n", i + 1, cases[i].name);
        if (! cases[i].sameQueue)
            emptyQueue();
        const auto code = runCase (exe, i);
        if (code != 0)
        {
            ++failed;
            printf ("  ended with 0x%08X\n", (unsigned) code);
        }
    }

    printf (failed ? "\n%d CASE(S) FAILED\n" : "\nALL PASSED\n", failed);
    return failed;
}
