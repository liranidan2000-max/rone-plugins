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
// OldVersionCleaner - Settings > DELETE OLD VERSIONS.
//
// Finds files that earlier RONE installs left behind and that make a DAW load
// the wrong thing, and deletes them. Liran, 2026-09-26, after RONE Reverse
// Reverb 1.1.7 kept showing up in FL Studio as a synth: the bundle still held a
// Contents/Resources/moduleinfo.json from a 2026-09-02 build that said
// "Instrument|Synth", and FL believes that file over the DLL. Every plugin now
// builds without that manifest, so no installer ever replaced or removed it.
//
// What counts (and only this - it never deletes the only copy of a plugin):
//   manifest   Contents/Resources/moduleinfo.json older than the bundle's own
//              binary: a description of a previous build.
//   duplicate  a bundle in the top VST3 folder that also sits in VST3\RONE
//              (an old dev build or the pre-RONE-folder install); DAWs load the
//              stale top-level copy (see the 2026-09-16 Throw incident).
//   legacy     a pre-2026-09-02 name (ReverseReverb.vst3, RONE Bridge.vst3, ...)
//              while the current name is installed too. Windows and macOS file
//              names ignore case, so "Rone Flanger" vs "RONE Flanger" in one
//              folder is the same file and is never touched.
//   leftover   a quarantined copy from an update that met a locked file:
//              *.locked-old*, *.shadow-old*, _RONE_shadow_quarantine_*.
//
// Deleting needs admin rights on most machines (Program Files, /Library), so
// the Center first tries as the user and then runs itself once, elevated, with
// --delete-old-versions; that instance scans again with the same rules and
// deletes only what it finds itself (no paths are passed to it). Anything a
// DAW still has loaded is skipped and reported by the DAW's name.
//
// --test-root "<dir>" (command line only, with --list/--delete): the roots become
// sub-folders of that folder - how the rules are tested on a fake tree. The
// elevated run is started with --delete-old-versions alone, so it cannot be steered.
// ============================================================================
namespace OldVersionCleaner
{
    constexpr const char* kDeleteFlag = "--delete-old-versions";
    constexpr const char* kListFlag   = "--list-old-versions";

    struct Roots
    {
        juce::File roneVst3;     // where the installers put bundles (Windows: ...\VST3\RONE)
        juce::File topVst3;      // Windows: ...\VST3 (the top-level folder); macOS: the same folder
        juce::File apps;         // standalone apps (Windows: Program Files\RONE Plugins)
        juce::File appsAlt;      // macOS: /Applications/RONE Plugins
        juce::File au;           // macOS: Components
    };

    inline Roots testRoots (const juce::File& base)
    {
        Roots r;
        r.topVst3  = base.getChildFile ("VST3");
        r.roneVst3 = r.topVst3.getChildFile ("RONE");
        r.apps     = base.getChildFile ("Apps");
        r.au       = base.getChildFile ("Components");
        return r;
    }

    inline Roots defaultRoots()
    {
        Roots r;
        r.roneVst3 = VersionChecker::getVst3InstallDir();
       #if JUCE_WINDOWS
        r.topVst3 = r.roneVst3.getParentDirectory();
        r.apps    = VersionChecker::getStandaloneInstallDir();
       #elif JUCE_MAC
        r.topVst3 = r.roneVst3;                          // no RONE sub-folder on macOS
        r.apps    = juce::File ("/Applications");
        r.appsAlt = juce::File ("/Applications/RONE Plugins");
        r.au      = VersionChecker::getAUInstallDir();
       #else
        r.topVst3 = r.roneVst3.getParentDirectory();
       #endif
        return r;
    }

    struct Item
    {
        juce::File file;
        juce::String kind;       // manifest | duplicate | legacy | leftover
        juce::String reason;     // one sentence for the confirm list
        juce::String status;     // after delete: deleted | in_use | failed
        juce::StringArray hosts; // in_use: who holds it ("FL Studio")
    };

    // Old name -> the name that replaced it (installer/*.iss [InstallDelete], 2026-09).
    // Names that differ only in case are the same file and are skipped below.
    inline juce::StringPairArray legacyNames()
    {
        juce::StringPairArray m;
        m.set ("ReverseReverb",  "RONE Reverse Reverb");
        m.set ("RONE Bridge",    "RONE Analyzer Bridge");
        m.set ("Rone Flanger",   "RONE Flanger");
        m.set ("Rone Stucker",   "RONE Stucker");
        m.set ("Rone Stutter",   "RONE Stutter");
        m.set ("Rone Sync Verb", "RONE Sync Verb");
        return m;
    }

    inline bool isRoneName (const juce::String& fileName)
    {
        return fileName.startsWith ("RONE ") || fileName.startsWith ("Rone ") || fileName.startsWithIgnoreCase ("ReverseReverb");
    }

    // The exact entry in `dir` whose name matches `name` ignoring case, or {}.
    inline juce::File findEntry (const juce::File& dir, const juce::String& name)
    {
        if (! dir.isDirectory()) return {};
        for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFilesAndDirectories))
            if (e.getFile().getFileName().equalsIgnoreCase (name))
                return e.getFile();
        return {};
    }

    inline juce::File bundleBinary (const juce::File& bundle)
    {
        const auto base = bundle.getFileNameWithoutExtension();
        auto win = bundle.getChildFile ("Contents/x86_64-win").getChildFile (base + ".vst3");
        if (win.existsAsFile()) return win;
        auto mac = bundle.getChildFile ("Contents/MacOS").getChildFile (base);
        if (mac.existsAsFile()) return mac;
        return {};
    }

    inline juce::String manifestVersion (const juce::File& json)
    {
        const auto parsed = juce::JSON::parse (json.loadFileAsString());
        if (auto* classes = parsed.getProperty ("Classes", {}).getArray())
            for (const auto& c : *classes)
                if (c.getProperty ("Version", {}).toString().isNotEmpty())
                    return c.getProperty ("Version", {}).toString();
        // The files VST3 SDK writes carry trailing commas that JUCE's parser refuses.
        const auto text = json.loadFileAsString();
        const auto at = text.indexOf ("\"Version\"");
        if (at < 0) return {};
        return text.substring (at).fromFirstOccurrenceOf (":", false, false)
                   .fromFirstOccurrenceOf ("\"", false, false).upToFirstOccurrenceOf ("\"", false, false);
    }

    inline juce::Array<Item> scan (const Roots& r = defaultRoots())
    {
        juce::Array<Item> items;
        auto add = [&items] (const juce::File& f, const juce::String& kind, const juce::String& reason)
        {
            for (const auto& i : items)
                if (i.file == f || f.isAChildOf (i.file)) return;   // already going, with its folder
            items.add ({ f, kind, reason, {}, {} });
        };

        const bool separateTop = r.topVst3 != r.roneVst3;
        const auto legacy = legacyNames();

        // leftover: quarantined copies from updates that met a locked file
        for (const auto& dir : { r.roneVst3, r.topVst3 })
        {
            if (! dir.isDirectory()) continue;
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFilesAndDirectories))
            {
                const auto f = e.getFile();
                const auto n = f.getFileName();
                if ((isRoneName (n) && (n.containsIgnoreCase (".locked-old") || n.containsIgnoreCase (".shadow-old")))
                    || n.startsWithIgnoreCase ("_RONE_shadow_quarantine_"))
                    add (f, "leftover", "A copy an earlier update set aside because a program was using it");
            }
        }

        // duplicate: a bundle in the top VST3 folder that the RONE folder has too
        if (separateTop && r.topVst3.isDirectory() && r.roneVst3.isDirectory())
        {
            for (const auto& e : juce::RangedDirectoryIterator (r.topVst3, false, "*.vst3", juce::File::findDirectories))
            {
                const auto f = e.getFile();
                const auto base = f.getFileNameWithoutExtension();
                if (! isRoneName (f.getFileName())) continue;

                auto current = findEntry (r.roneVst3, f.getFileName());
                if (current == juce::File() && legacy.containsKey (base))
                    current = findEntry (r.roneVst3, legacy[base] + ".vst3");
                if (current != juce::File() && current.isDirectory())
                    add (f, "duplicate", "A second copy outside the RONE folder - your DAW may load this old one instead of " + current.getFileName());
            }
        }

        // legacy: an old name next to its current name (never when they are the same file)
        const auto legacyKeys = legacy.getAllKeys();
        for (const auto& oldBase : legacyKeys)
        {
            const auto newBase = legacy[oldBase];
            if (oldBase.equalsIgnoreCase (newBase)) continue;             // same file on disk

            struct Place { juce::File dir; juce::String ext; };
            const Place places[] { { r.roneVst3, ".vst3" }, { r.apps, ".exe" }, { r.apps, ".app" },
                                   { r.appsAlt, ".app" }, { r.au, ".component" } };
            for (const auto& p : places)
            {
                if (p.dir == juce::File() || ! p.dir.isDirectory()) continue;
                const auto oldOne = findEntry (p.dir, oldBase + p.ext);
                const auto newOne = findEntry (p.dir, newBase + p.ext);
                if (oldOne != juce::File() && newOne != juce::File() && oldOne != newOne)
                    add (oldOne, "legacy", "The old name of " + newOne.getFileName() + ", which is installed next to it");
            }
        }

        // manifest: a moduleinfo.json older than the bundle's binary
        juce::Array<juce::File> vst3Dirs { r.roneVst3 };
        if (separateTop) vst3Dirs.add (r.topVst3);
        for (const auto& dir : vst3Dirs)
        {
            if (! dir.isDirectory()) continue;
            for (const auto& e : juce::RangedDirectoryIterator (dir, false, "*.vst3", juce::File::findDirectories))
            {
                const auto bundle = e.getFile();
                if (! isRoneName (bundle.getFileName())) continue;
                const auto json = bundle.getChildFile ("Contents/Resources/moduleinfo.json");
                const auto bin = bundleBinary (bundle);
                if (! json.existsAsFile() || bin == juce::File()) continue;

                const auto age = bin.getLastModificationTime() - json.getLastModificationTime();
                if (age.inMinutes() > 10.0)
                {
                    const auto v = manifestVersion (json);
                    add (json, "manifest", "An old description file" + (v.isNotEmpty() ? " (version " + v + ")" : juce::String())
                                         + " inside " + bundle.getFileName() + " - your DAW reads it instead of the plugin");
                }
            }
        }

        return items;
    }

    // Deletes as the current user. In-use items are skipped; failures stay "failed".
    inline void deleteItems (juce::Array<Item>& items)
    {
        for (auto& it : items)
        {
            if (! it.file.exists()) { it.status = "deleted"; continue; }

           #if JUCE_WINDOWS
            const bool isApp = it.file.hasFileExtension ("exe");
            auto holders = PluginInUse::find (isApp ? juce::File() : it.file, isApp ? it.file : juce::File());
            if (! holders.isEmpty())
            {
                it.status = "in_use";
                it.hosts = holders.hosts;
                if (holders.ownApp) it.hosts.addIfNotAlreadyThere (it.file.getFileNameWithoutExtension());
                continue;
            }
           #endif

            if (it.file.isDirectory()) it.file.deleteRecursively (false);
            else                       it.file.deleteFile();
            it.status = it.file.exists() ? "failed" : "deleted";
        }
    }

    // The same program, once, with admin rights and --delete-old-versions; waits for it.
    // Returns false when the user declined or the OS could not start it.
    inline bool runElevated()
    {
        const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
       #if JUCE_WINDOWS
        const auto file = exe.getFullPathName();
        const juce::String params (kDeleteFlag);
        SHELLEXECUTEINFOW info {};
        info.cbSize = sizeof (info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        info.lpVerb = L"runas";
        info.lpFile = file.toWideCharPointer();
        info.lpParameters = params.toWideCharPointer();
        info.nShow = SW_HIDE;
        if (! ShellExecuteExW (&info) || info.hProcess == nullptr)
            return false;
        WaitForSingleObject (info.hProcess, 120000);
        CloseHandle (info.hProcess);
        return true;
       #elif JUCE_MAC
        auto quoted = exe.getFullPathName().replace ("\\", "\\\\").replace ("\"", "\\\"");
        juce::StringArray cmd { "/usr/bin/osascript", "-e",
            "do shell script quoted form of \"" + quoted + "\" & \" " + juce::String (kDeleteFlag) + "\" with administrator privileges" };
        juce::ChildProcess p;
        if (! p.start (cmd)) return false;
        p.waitForProcessToFinish (120000);
        return p.getExitCode() == 0;
       #else
        juce::ignoreUnused (exe);
        return false;
       #endif
    }

    inline juce::var toVar (const Item& it)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("path", it.file.getFullPathName());
        o->setProperty ("name", it.kind == "manifest" ? it.file.getParentDirectory().getParentDirectory().getParentDirectory().getFileName()
                                                         + " / moduleinfo.json"
                                                       : it.file.getFileName());
        o->setProperty ("kind", it.kind);
        o->setProperty ("reason", it.reason);
        if (it.status.isNotEmpty()) o->setProperty ("status", it.status);
        if (! it.hosts.isEmpty()) o->setProperty ("hosts", it.hosts.joinIntoString (", "));
        return juce::var (o);
    }

    inline juce::var toVar (const juce::Array<Item>& items)
    {
        juce::Array<juce::var> arr;
        for (const auto& it : items) arr.add (toVar (it));
        return juce::var (arr);
    }

    // The whole button: scan, delete as the user, then once elevated for what is left.
    inline juce::var cleanNow()
    {
        auto items = scan();
        deleteItems (items);

        bool needsAdmin = false;
        for (const auto& it : items) needsAdmin = needsAdmin || it.status == "failed";

        bool elevationDeclined = false;
        if (needsAdmin)
        {
            elevationDeclined = ! runElevated();
            for (auto& it : items)
                if (it.status == "failed" && ! it.file.exists())
                    it.status = "deleted";
        }

        int deleted = 0, inUse = 0, failed = 0;
        for (const auto& it : items)
        {
            if (it.status == "deleted") ++deleted;
            else if (it.status == "in_use") ++inUse;
            else ++failed;
        }

        auto* o = new juce::DynamicObject();
        o->setProperty ("success", true);
        o->setProperty ("items", toVar (items));
        o->setProperty ("deleted", deleted);
        o->setProperty ("inUse", inUse);
        o->setProperty ("failed", failed);
        o->setProperty ("elevationDeclined", elevationDeclined);
        return juce::var (o);
    }

    // --delete-old-versions (the elevated instance) and --list-old-versions (a dry run):
    // the result goes to %APPDATA%\RonePlugins\old-versions.json for whoever started it.
    inline bool runCommandLine (const juce::String& commandLine)
    {
        const bool doDelete = commandLine.contains (kDeleteFlag);
        const bool doList = commandLine.contains (kListFlag);
        if (! doDelete && ! doList) return false;

        auto tokens = juce::StringArray::fromTokens (commandLine, true);
        const int at = tokens.indexOf ("--test-root");
        const auto roots = (at >= 0 && at + 1 < tokens.size()) ? testRoots (juce::File (tokens[at + 1].unquoted()))
                                                               : defaultRoots();
        auto items = scan (roots);
        if (doDelete) deleteItems (items);

        auto out = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("RonePlugins").getChildFile ("old-versions.json");
        out.getParentDirectory().createDirectory();
        out.replaceWithText (juce::JSON::toString (toVar (items)));
        return true;
    }

    inline bool isCommandLineMode (const juce::String& commandLine)
    {
        return commandLine.contains (kDeleteFlag) || commandLine.contains (kListFlag);
    }
}
