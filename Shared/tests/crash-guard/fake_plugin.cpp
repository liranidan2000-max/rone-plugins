// A plugin that misbehaves on request. Volatile everywhere so the optimiser keeps every crash.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdexcept>

#define EXPORT extern "C" __declspec(dllexport)

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
bool asked (const char* name)
{
    char value[8];
    return GetEnvironmentVariableA (name, value, sizeof (value)) > 0;
}

void fault()
{
    volatile int* p = nullptr;
    *p = 1;
}

struct AtLoad
{
    AtLoad()  { if (asked ("GUARD_CTOR_FAULT")) fault(); }
    ~AtLoad() { if (asked ("GUARD_DTOR_FAULT")) fault(); }
} atLoad;

// built on first use, so the CRT destroys it before everything built at load
struct OnFirstUse
{
    ~OnFirstUse() { if (asked ("GUARD_LATE_DTOR_FAULT")) fault(); }
};

int staticData[16];

// sends itself the message it just got, as JUCE 8.0.4 does with WM_IME_SETCONTEXT: most of each turn is
// user32, so the stack runs out in system code with our frames just above it
LRESULT CALLBACK sendsItself (HWND h, UINT message, WPARAM w, LPARAM l)
{
    if (message == WM_APP)
        return SendMessageW (h, WM_APP, w, l) + 1;

    return DefWindowProcW (h, message, w, l);
}
}

EXPORT void touch_late_static()
{
    static OnFirstUse late;
    (void) late;
}

EXPORT void* plugin_data() { return staticData; }

EXPORT __declspec(noinline) int crash_null()
{
    volatile int* p = nullptr;
    return *p;
}

EXPORT __declspec(noinline) void crash_write()
{
    volatile int* p = reinterpret_cast<int*> (0x10);
    *p = 1;
}

EXPORT __declspec(noinline) int crash_divide (int by)
{
    volatile int a = 7;
    return a / by;
}

// Rip is 0 at the fault
EXPORT __declspec(noinline) int crash_null_call (int (*volatile callback)())
{
    return callback() + 1;
}

#pragma warning (disable: 4717)
EXPORT __declspec(noinline) int crash_recursion (int n)
{
    volatile char pad[4096];
    pad[0] = (char) n;
    return crash_recursion (n + 1) + pad[0];
}

EXPORT __declspec(noinline) void crash_through_user32()
{
    WNDCLASSW type {};
    type.lpfnWndProc = sendsItself;
    type.hInstance = reinterpret_cast<HINSTANCE> (&__ImageBase);
    type.lpszClassName = L"RoneGuardTest";
    RegisterClassW (&type);
    const auto window = CreateWindowExW (0, type.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, type.hInstance, nullptr);
    SendMessageW (window, WM_APP, 0, 0);
}

EXPORT __declspec(noinline) int call_host (int (*callback)())
{
    return callback() + 1;
}

EXPORT __declspec(noinline) void throw_cpp()
{
    throw std::runtime_error ("control flow, not a crash");
}

extern "C" void asm_bad_frame();
EXPORT void crash_bad_frame() { asm_bad_frame(); }
