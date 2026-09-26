// HiddenDesktopRun - runs a command on a private, never-shown desktop, so a test
// that opens plugin windows cannot take the foreground or reach the screen (a DAW
// may be recording). stdout/stderr are inherited; the exit code is the child's.
//   HiddenDesktopRun <timeout seconds> <command line...>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <string>

int wmain (int argc, wchar_t* argv[])
{
    if (argc < 3)
    {
        fwprintf (stderr, L"usage: HiddenDesktopRun <timeout seconds> <command line...>\n");
        return 2;
    }

    const DWORD timeoutMs = (DWORD) (_wtof (argv[1]) * 1000.0);

    std::wstring cmd;
    for (int i = 2; i < argc; ++i)
    {
        if (i > 2) cmd += L' ';
        const std::wstring a (argv[i]);
        const bool quote = a.find_first_of (L" \t") != std::wstring::npos;
        if (quote) cmd += L'"';
        cmd += a;
        if (quote) cmd += L'"';
    }

    wchar_t deskName[64];
    swprintf_s (deskName, L"RoneTwinTest_%lu", GetCurrentProcessId());
    HDESK desk = CreateDesktopW (deskName, nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    if (desk == nullptr)
    {
        fwprintf (stderr, L"CreateDesktop failed: %lu\n", GetLastError());
        return 2;
    }

    std::wstring deskPath = std::wstring (L"WinSta0\\") + deskName;
    STARTUPINFOW si {};
    si.cb = sizeof (si);
    si.lpDesktop = deskPath.data();
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle (STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi {};
    std::wstring mutableCmd = cmd;
    if (! CreateProcessW (nullptr, mutableCmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi))
    {
        fwprintf (stderr, L"CreateProcess failed: %lu\n", GetLastError());
        CloseDesktop (desk);
        return 2;
    }

    DWORD code = 3;
    if (WaitForSingleObject (pi.hProcess, timeoutMs) == WAIT_TIMEOUT)
    {
        fwprintf (stderr, L"timeout - terminating the child\n");
        TerminateProcess (pi.hProcess, 3);
        WaitForSingleObject (pi.hProcess, 5000);
    }
    GetExitCodeProcess (pi.hProcess, &code);
    CloseHandle (pi.hThread);
    CloseHandle (pi.hProcess);
    CloseDesktop (desk);
    return (int) code;
}
