#pragma once

#include <JuceHeader.h>
#include "VersionChecker.h"
#include "PluginInUse.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
 #pragma comment (lib, "shell32.lib")
#endif

// ============================================================================
// PluginUninstaller - UNINSTALL in a plugin card's menu (Liran, 2026-09-27:
// "REINSTALL and UNINSTALL in every plugin's menu"). It came out of RONE
// Reverse Reverb 1.1.7 staying a synth in FL Studio: FL keeps what it once
// scanned for as long as the file it sees has the same date and size, so a
// reinstall of the same build changes nothing there. Uninstall, let the DAW
// scan (it forgets the plugin), install again, scan: the DAW meets it new.
//
// What it removes: the plugin's own uninstaller (Windows: Inno, the Uninstall
// entry of its AppId) runs first, then whatever it left of the plugin's files
// goes too - a bundle folder that still holds a file the installer never put
// there, a copy at the top of the VST3 folder. Nothing outside the RONE names
// in the plugin folders is ever touched; presets and settings stay.
//
// Both need admin rights, so the Center runs itself once, elevated, with
// --uninstall-plugin <registry key> <files...> (one permission prompt), and
// then reads the result off the machine itself: the uninstall entry gone and
// the files gone. macOS has no uninstaller for a .pkg: the files are removed
// with the administrator password, the same way the .pkg was installed - and
// without one when they are in the user's own folders (Center 2.1).
// A plugin loaded in a DAW is never uninstalled: the user is told what to close.
// ============================================================================
namespace PluginUninstaller
{
    static constexpr const char* kFlag = "--uninstall-plugin";

    struct Result
    {
        bool ok = false;
        juce::String error;             // for the user, when ! ok
        juce::StringArray hosts;        // programs that have it loaded - nothing was touched
        bool declined = false;          // the permission prompt was refused
        juce::StringArray leftovers;    // still on disk afterwards
    };

    // Every place an install of this plugin puts something a DAW or the user sees.
    inline juce::Array<juce::File> filesOf (const PluginInfo& p)
    {
        juce::Array<juce::File> files;
        auto add = [&files] (const juce::File& f) { files.addIfNotAlreadyThere (f); };

        const auto vst3Dir = VersionChecker::getVst3InstallDir();
        if (p.vst3Bundle.isNotEmpty())
        {
            add (vst3Dir.getChildFile (p.vst3Bundle));
           #if JUCE_WINDOWS
            add (vst3Dir.getParentDirectory().getChildFile (p.vst3Bundle));   // a top-level copy loads first
           #endif
        }
        if (p.id == "RONEAnalyzer")
        {
           #if JUCE_WINDOWS
            add (vst3Dir.getParentDirectory().getChildFile ("RONE Analyzer Bridge.vst3"));   // its installer puts it there
           #else
            add (vst3Dir.getChildFile ("RONE Analyzer Bridge.vst3"));
           #endif
        }

       #if JUCE_MAC
        if (p.auBundle.isNotEmpty())
        {
            add (VersionChecker::getAUInstallDir().getChildFile (p.auBundle));
            add (juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                     .getChildFile ("Library/Audio/Plug-Ins/Components").getChildFile (p.auBundle));
        }
        if (p.standaloneExe.isNotEmpty())
        {
            const auto app = p.standaloneExe.replace (".exe", "") + ".app";
            add (juce::File ("/Applications").getChildFile (app));
            add (juce::File ("/Applications/RONE Plugins").getChildFile (app));
            add (VersionChecker::getUserAppsDir().getChildFile (app));
        }
        if (p.manualPdf.isNotEmpty())
        {
            add (juce::File ("/Users/Shared/RONE Plugins/Manuals").getChildFile (p.manualPdf));
            add (VersionChecker::getUserManualsDir().getChildFile (p.manualPdf));
        }

        // Center 2.1 installs what was not system-wide into the user's folders.
        if (p.vst3Bundle.isNotEmpty())
            add (VersionChecker::getUserVst3Dir().getChildFile (p.vst3Bundle));
        if (p.id == "RONEAnalyzer")
        {
            add (VersionChecker::getUserVst3Dir().getChildFile ("RONE Analyzer Bridge.vst3"));
            add (VersionChecker::getAUInstallDir().getChildFile ("RONE Analyzer Bridge.component"));
            add (VersionChecker::getUserAUDir().getChildFile ("RONE Analyzer Bridge.component"));
        }
       #else
        const auto appDir = VersionChecker::getStandaloneInstallDir();
        if (p.standaloneExe.isNotEmpty())
            add (appDir.getChildFile (p.standaloneExe));
        if (p.manualPdf.isNotEmpty())
            add (appDir.getChildFile ("Manuals").getChildFile (p.manualPdf));
       #endif
        return files;
    }

    // The elevated side deletes only this: a RONE-named entry inside the plugin folders.
    inline bool isRemovable (const juce::File& f)
    {
        const auto n = f.getFileName();
        if (! (n.startsWith ("RONE ") || n.startsWith ("Rone ") || n.startsWithIgnoreCase ("ReverseReverb")))
            return false;

       #if JUCE_WINDOWS
        const juce::File roots[] { VersionChecker::getVst3InstallDir().getParentDirectory(),   // ...\Common Files\VST3
                                   VersionChecker::getStandaloneInstallDir() };                 // ...\RONE Plugins
       #elif JUCE_MAC
        const juce::File roots[] { VersionChecker::getVst3InstallDir(), VersionChecker::getAUInstallDir(),
                                   VersionChecker::getUserVst3Dir(), VersionChecker::getUserAUDir(),
                                   juce::File ("/Applications"), VersionChecker::getUserAppsDir(),
                                   juce::File ("/Users/Shared/RONE Plugins/Manuals"), VersionChecker::getUserManualsDir() };
       #else
        const juce::File roots[] { VersionChecker::getVst3InstallDir() };
       #endif
        for (const auto& r : roots)
            if (f.isAChildOf (r))
                return true;
        return false;
    }

    inline void removeFiles (const juce::StringArray& paths)
    {
        for (const auto& path : paths)
        {
            const juce::File f (path);
            if (! f.exists() || ! isRemovable (f)) continue;
            if (f.isDirectory()) f.deleteRecursively (false);
            else                 f.deleteFile();
        }
    }

   #if JUCE_WINDOWS
    // "C:\...\unins000.exe" (quoted or not) -> the exe.
    inline juce::File uninstallerExe (const juce::String& registryKey)
    {
        const auto cmd = VersionChecker::getUninstallCommand (registryKey).trim();
        if (cmd.isEmpty()) return {};
        const auto path = cmd.startsWithChar ('"') ? cmd.substring (1).upToFirstOccurrenceOf ("\"", false, false)
                                                   : cmd.upToFirstOccurrenceOf (" /", false, false);
        const juce::File exe (path.trim());
        return exe.existsAsFile() ? exe : juce::File();
    }
   #endif

    // The elevated instance: run the uninstaller, wait for it, remove what is left, quit.
    inline bool runCommandLine (const juce::String& commandLine)
    {
        auto tokens = juce::StringArray::fromTokens (commandLine, true);
        const int at = tokens.indexOf (kFlag);
        if (at < 0) return false;
        if (at + 1 >= tokens.size()) return true;

        const auto registryKey = tokens[at + 1].unquoted();
        juce::StringArray paths;
        for (int i = at + 2; i < tokens.size(); ++i)
            paths.add (tokens[i].unquoted());

       #if JUCE_WINDOWS
        const auto exe = uninstallerExe (registryKey);
        if (exe != juce::File())
        {
            juce::ChildProcess p;
            if (p.start (juce::StringArray { exe.getFullPathName(), "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART" }))
                p.waitForProcessToFinish (180000);

            // Inno's first phase hands over to a copy in %TEMP% and may return before
            // that one is done; its last act is removing the Uninstall entry.
            for (int waited = 0; waited < 120000 && VersionChecker::getUninstallCommand (registryKey).isNotEmpty(); waited += 250)
                juce::Thread::sleep (250);
            juce::Thread::sleep (500);
        }
       #endif

        removeFiles (paths);
        return true;
    }

    inline bool isCommandLineMode (const juce::String& commandLine)
    {
        return commandLine.contains (kFlag);
    }

   #if JUCE_WINDOWS
    inline juce::String quoteArg (const juce::String& s)   // Windows paths never hold a quote
    {
        return "\"" + s + "\"";
    }
   #endif

    // Blocking - call from a background thread.
    inline Result uninstall (const PluginInfo& p)
    {
        Result r;
        const auto files = filesOf (p);

       #if JUCE_WINDOWS
        // A DAW with the VST3 loaded holds the DLL: the uninstaller could not remove
        // it and the plugin would be half there. Say what to close instead.
        {
            const auto bundle = p.vst3Bundle.isNotEmpty() ? VersionChecker::getVst3InstallDir().getChildFile (p.vst3Bundle)
                              : p.id == "RONEAnalyzer" ? VersionChecker::getVst3InstallDir().getParentDirectory()
                                                                          .getChildFile ("RONE Analyzer Bridge.vst3")
                                                       : juce::File();
            const auto exe = p.standaloneExe.isNotEmpty() ? VersionChecker::getStandaloneInstallDir().getChildFile (p.standaloneExe)
                                                          : juce::File();
            const auto holders = PluginInUse::find (bundle, exe);
            if (! holders.isEmpty())
            {
                r.hosts = holders.hosts;
                if (holders.ownApp) r.hosts.addIfNotAlreadyThere (p.name);
                r.error = p.name + " is open in " + r.hosts.joinIntoString (", ") + ". Close it, then uninstall.";
                return r;
            }
        }

        juce::String params = juce::String (kFlag) + " " + quoteArg (p.registryKey);
        for (const auto& f : files)
            if (f.exists())
                params << " " << quoteArg (f.getFullPathName());

        const auto self = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName();
        SHELLEXECUTEINFOW info {};
        info.cbSize = sizeof (info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        info.lpVerb = L"runas";
        info.lpFile = self.toWideCharPointer();
        info.lpParameters = params.toWideCharPointer();
        info.nShow = SW_HIDE;
        if (! ShellExecuteExW (&info) || info.hProcess == nullptr)
        {
            r.declined = GetLastError() == ERROR_CANCELLED;
            r.error = r.declined ? juce::String ("Windows did not give permission - nothing was removed.")
                                 : juce::String ("Could not start the uninstaller.");
            return r;
        }
        WaitForSingleObject (info.hProcess, 300000);
        CloseHandle (info.hProcess);

        const bool entryGone = VersionChecker::getUninstallCommand (p.registryKey).isEmpty();
       #elif JUCE_MAC
        // What the Center 2.1 put in the user's folders goes without a password;
        // the password is asked for only when a .pkg's system-wide copy is left.
        const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        for (const auto& f : files)
            if (f.exists() && isRemovable (f) && f.isAChildOf (home))
            {
                if (f.isDirectory()) f.deleteRecursively (false);
                else                 f.deleteFile();
            }

        juce::String quoted;
        for (const auto& f : files)
        {
            const auto path = f.getFullPathName();
            if (! f.exists() || ! isRemovable (f)) continue;
            if (path.containsAnyOf ("'\"\\")) { r.error = "Unexpected characters in " + path; return r; }
            quoted << " \\\"" << path << "\\\"";   // \"path\" inside the AppleScript string
        }
        if (quoted.isNotEmpty())
        {
            // Same as the install: std::system, since a ChildProcess cannot show the
            // password dialog from a background thread.
            const auto cmd = juce::String ("osascript -e 'do shell script \"/bin/rm -rf")
                           + quoted + "\" with administrator privileges'";
            if (std::system (cmd.toRawUTF8()) != 0)
            {
                r.declined = true;
                r.error = "The administrator password was not given - nothing was removed.";
                return r;
            }
        }
        const bool entryGone = true;
       #else
        removeFiles ([&files] { juce::StringArray s; for (const auto& f : files) s.add (f.getFullPathName()); return s; }());
        const bool entryGone = true;
       #endif

        for (const auto& f : files)
            if (f.exists())
                r.leftovers.add (f.getFullPathName());

        if (entryGone)
            VersionChecker::clearInstalledVersion (p.registryKey);

        r.ok = entryGone && r.leftovers.isEmpty();
        if (! r.ok && r.error.isEmpty())
            r.error = ! entryGone ? juce::String ("The uninstaller did not finish. Restart the computer and try again.")
                                  : "Some files could not be removed: " + r.leftovers.joinIntoString (", ");
        return r;
    }
}
