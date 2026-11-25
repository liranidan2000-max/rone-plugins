#pragma once

#include <JuceHeader.h>

#if JUCE_MAC
 #include <sys/wait.h>
 #include <cstdlib>
#endif

// ============================================================================
// MacAdmin - the one place that runs osascript for "with administrator
// privileges" (installs, uninstalls, the move to the user's folders, the
// Center's own swap).
//
// Support #206 (Center 2.1.51, macOS 14.6.1): osascript answered "can't open
// default scripting component" - the Mac could not open its DEFAULT scripting
// language, so nothing ran, no password was asked, and the customer read those
// words. The same line is run once more with the language named
// (-l AppleScript), which does not go through the default; if that fails too,
// the customer is told what to do. Only after this one answer, so every Mac
// that works today runs exactly what it ran before.
// ============================================================================
namespace MacAdmin
{
    inline bool scriptingUnavailable (const juce::String& err)
    {
        return err.containsIgnoreCase ("scripting component");
    }

    // MainComponent's isEnvironmentalStatus() knows this text.
    inline juce::String scriptingUnavailableText()
    {
        return "This Mac could not open its password window";
    }

    inline juce::String scriptingUnavailableHelp()
    {
        return "Restart the Mac and try again. If it happens again, write to us from the SUPPORT page.";
    }

   #if JUCE_MAC
    struct Ran
    {
        int status = -1;    // std::system's own answer
        int code   = -1;    // the exit code, -1 when osascript did not exit by itself
        juce::String err;   // what osascript said on stderr
    };

    // args: what follows "osascript", quoted for sh. errFile: where stderr goes
    // (juce::File() = it is not kept, and then nothing is tried twice).
    // std::system rather than a ChildProcess: a ChildProcess cannot bring up the
    // macOS password dialog from a background thread.
    inline Ran run (const juce::String& args, const juce::File& errFile)
    {
        const bool capture = errFile != juce::File();

        auto once = [&] (const juce::String& options)
        {
            Ran r;
            const auto cmd = "osascript " + options + args
                           + (capture ? " 2>'" + errFile.getFullPathName() + "'" : juce::String());
            r.status = std::system (cmd.toRawUTF8());
            r.code   = WIFEXITED (r.status) ? WEXITSTATUS (r.status) : -1;
            if (capture)
            {
                r.err = errFile.loadFileAsString().trim();
                errFile.deleteFile();
            }
            return r;
        };

        auto r = once ({});
        if (r.code != 0 && scriptingUnavailable (r.err))
            r = once ("-l AppleScript ");
        return r;
    }
   #endif
}
