#pragma once

// ============================================================================
// OsCrashScanner: crashes the operating system recorded, for plugins and apps
// that never got to say so themselves.
//
// Windows keeps an "Application Error" event for every crash, with the faulting
// module and offset, whatever host it happened in. macOS keeps an .ips file.
// Neither needs anything in the plugin, so this covers every version already
// installed, and it is the only Mac route for a crash inside a DAW: a signal
// handler in a plugin would fight the DAW's own (see Shared/CRASH-REPORTING.md).
//
// Read-only on the OS side. A crash is ours only when a RONE module is the one
// that faulted or, on a Mac, is on the crashing thread. Reports carry file names,
// never paths, and join the same queue as everything else.
// ============================================================================

#include <juce_core/juce_core.h>
#include <functional>

namespace OsCrashScanner
{

struct Crash
{
    juce::String product, version, wrapper, message, details, stack;
};

// Hands each RONE crash recorded since the last pass to sink. stateFile remembers
// where the last pass stopped.
void scan (const juce::File& stateFile, const std::function<void (const Crash&)>& sink);

// One pass on a background thread, into the crash queue.
void scanAsync();

} // namespace OsCrashScanner
