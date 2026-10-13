#pragma once

#include <JuceHeader.h>
#include "VersionChecker.h"
#include "OldVersionCleaner.h"

#if JUCE_MAC
 #include <unistd.h>
 #include <sys/wait.h>
#endif

// ============================================================================
// UserFolderMove - Settings > MOVE PLUGINS TO MY FOLDER (macOS, Center 2.1.3).
//
// Since 2.1 the Center installs a plugin the Mac does not have yet into the
// user's own folders (~/Library/Audio/Plug-Ins, ~/Applications) with no
// password. A plugin a .pkg once put in /Library or /Applications stays there,
// and every update of it went through the administrator password - Liran,
// 2026-10-13: "every update the Center offers asks for my password, can we
// skip it?". This moves every RONE bundle from the system folders into the
// user's, once, behind one last password; after that hasSystemCopy() in
// InstallBatcher finds nothing and every update installs with no password.
//
// A move, not a copy: rename() keeps the file, so a Dock item that points at
// /Applications/RONE Analyzer.app still finds it in ~/Applications. Each moved
// bundle is then given to the user (chown -R), because a .pkg installed it as
// root and the user could otherwise not replace or delete it later.
//
// What counts (and only this):
//   move     a RONE bundle (OldVersionCleaner::isRoneName) in /Library/Audio/
//            Plug-Ins/VST3 or Components, /Applications or /Applications/RONE
//            Plugins, not yet in the user's matching folder. The Center itself
//            is never moved: it updates itself.
//   remove   the same bundle when the user's folder already has it (a DAW
//            would list it twice), or a pre-rename name whose current name is
//            installed (OldVersionCleaner::legacyNames).
//
// The elevated half is a short /bin/sh script run once through osascript with
// administrator privileges - the way every Mac install already runs (see
// InstallBatcher::runBatch): mv each bundle (ditto + rm across volumes),
// chown -R it to the user, rm what is only a duplicate. Every path in it came
// from scan() and is refused if it holds a quote.
//
// --test-root "<dir>" (command line only, with --list-user-folder-move or
// --move-to-user-folders): the folders become sub-folders of that folder and
// the moves are made in-process with nothing given to anyone - how the rules
// are tested on a fake tree, on any OS.
// ============================================================================
namespace UserFolderMove
{
    constexpr const char* kMoveFlag = "--move-to-user-folders";
    constexpr const char* kListFlag = "--list-user-folder-move";

    struct Roots
    {
        juce::File sysVst3, sysAU, sysApps, sysAppsAlt;
        juce::File userVst3, userAU, userApps;
    };

    inline Roots testRoots (const juce::File& base)
    {
        Roots r;
        r.sysVst3    = base.getChildFile ("System/VST3");
        r.sysAU      = base.getChildFile ("System/Components");
        r.sysApps    = base.getChildFile ("System/Applications");
        r.sysAppsAlt = r.sysApps.getChildFile ("RONE Plugins");
        r.userVst3   = base.getChildFile ("User/VST3");
        r.userAU     = base.getChildFile ("User/Components");
        r.userApps   = base.getChildFile ("User/Applications");
        return r;
    }

    inline Roots rootsForHome (const juce::File& home)
    {
        Roots r;
        r.sysVst3    = juce::File ("/Library/Audio/Plug-Ins/VST3");
        r.sysAU      = juce::File ("/Library/Audio/Plug-Ins/Components");
        r.sysApps    = juce::File ("/Applications");
        r.sysAppsAlt = juce::File ("/Applications/RONE Plugins");
        r.userVst3   = home.getChildFile ("Library/Audio/Plug-Ins/VST3");
        r.userAU     = home.getChildFile ("Library/Audio/Plug-Ins/Components");
        r.userApps   = home.getChildFile ("Applications");
        return r;
    }

    /** Nothing to move anywhere but on a Mac. */
    inline bool isSupported()
    {
       #if JUCE_MAC
        return true;
       #else
        return false;
       #endif
    }

    inline Roots defaultRoots()
    {
        return isSupported() ? rootsForHome (juce::File::getSpecialLocation (juce::File::userHomeDirectory))
                             : Roots {};
    }

    struct Item
    {
        juce::File from, to;
        juce::String kind;       // move | remove
        juce::String status;     // after the run: moved | removed | failed
    };

    // The current name a pre-rename bundle name stands for, or "" when it is not
    // one. Names that differ only in case are the same file on a Mac: not legacy.
    inline juce::String currentNameFor (const juce::String& nameWithoutExtension)
    {
        const auto legacy = OldVersionCleaner::legacyNames();
        const auto current = legacy.getValue (nameWithoutExtension, {});
        return current.equalsIgnoreCase (nameWithoutExtension) ? juce::String() : current;
    }

    inline juce::Array<Item> scan (const Roots& r = defaultRoots())
    {
        juce::Array<Item> items;

        struct Pair { juce::File sys, user; const char* pattern; };
        const Pair pairs[] {
            { r.sysVst3,    r.userVst3, "*.vst3" },
            { r.sysAU,      r.userAU,   "*.component" },
            { r.sysApps,    r.userApps, "*.app" },
            { r.sysAppsAlt, r.userApps, "*.app" },
        };

        for (const auto& p : pairs)
        {
            if (p.sys == juce::File() || ! p.sys.isDirectory())
                continue;

            for (const auto& f : p.sys.findChildFiles (juce::File::findDirectories, false, p.pattern))
            {
                const auto name = f.getFileName();
                if (! OldVersionCleaner::isRoneName (name) || name.equalsIgnoreCase ("RONE Plugins Center.app"))
                    continue;

                Item it;
                it.from = f;
                it.to   = p.user.getChildFile (name);
                it.kind = it.to.exists() ? "remove" : "move";

                // A pre-rename copy whose current name is installed is only stale.
                const auto current = currentNameFor (f.getFileNameWithoutExtension());
                if (current.isNotEmpty())
                {
                    const auto currentFile = current + f.getFileExtension();
                    if (p.sys.getChildFile (currentFile).exists() || p.user.getChildFile (currentFile).exists())
                        it.kind = "remove";
                }

                items.add (it);
            }
        }
        return items;
    }

   #if JUCE_MAC
    inline int runTool (const juce::StringArray& args, int timeoutMs = 120000)
    {
        juce::ChildProcess p;
        if (! p.start (args, 0))
            return -1;
        if (! p.waitForProcessToFinish (timeoutMs))
        {
            p.kill();
            return -1;
        }
        return (int) p.getExitCode();
    }
   #endif

    /** The same moves, in-process and as whoever runs this - the --test-root
        run on a fake tree. The real one is moveScript() under the password. */
    inline void moveItems (juce::Array<Item>& items, const Roots& r)
    {
        for (const auto& dir : { r.userVst3, r.userAU, r.userApps })
            if (dir != juce::File())
                dir.createDirectory();

        for (auto& it : items)
        {
            if (it.kind == "remove")
                it.from.deleteRecursively (false);
            else if (! it.to.exists())
                it.from.moveFileTo (it.to);

            const bool gone = ! it.from.exists();
            it.status = it.kind == "remove" ? (gone ? "removed" : "failed")
                                            : (gone && it.to.exists() ? "moved" : "failed");
        }
    }

    /** The script the password runs, for the user `owner` ("uid:gid"). Empty
        when any path holds a character it cannot quote. Written with "\n" line
        ends: a "\r" left the Mac's sh reading "fi\r" (InstallBatcher, 2.0.5). */
    inline juce::String moveScript (const juce::Array<Item>& items, const Roots& r, const juce::String& owner)
    {
        auto bad = [] (const juce::File& f) { return f.getFullPathName().containsAnyOf ("'\n\r"); };
        auto q   = [] (const juce::File& f) { return "'" + f.getFullPathName() + "'"; };

        juce::String sh;
        sh << "#!/bin/sh\n"
           << "O=" << owner << "\n";

        // The user's folders, made if missing and given to the user: a folder
        // root just made - or one another vendor's installer left root's
        // (Center 2.1.1, crash reports #114-#122) - would refuse the user's own
        // updates later. The folder itself, not what other vendors put in it.
        for (const auto& dir : { r.userVst3, r.userAU, r.userApps })
        {
            if (bad (dir)) return {};
            sh << "/bin/mkdir -p " << q (dir) << " && /usr/sbin/chown \"$O\" " << q (dir) << "\n";
        }

        for (const auto& it : items)
        {
            if (bad (it.from) || bad (it.to)) return {};

            if (it.kind == "remove")
            {
                sh << "/bin/rm -rf " << q (it.from) << "\n";
                continue;
            }

            // mv keeps the file (and a Dock item that points at it). Across
            // volumes it is a copy, and the original goes only once the copy is
            // complete. Then the bundle is the user's, so the Center can replace
            // it later with no password.
            sh << "if [ ! -e " << q (it.to) << " ]; then /bin/mv " << q (it.from) << " " << q (it.to)
               << " 2>/dev/null || { /usr/bin/ditto " << q (it.from) << " " << q (it.to)
               << " && /bin/rm -rf " << q (it.from) << "; }; fi\n"
               << "[ -e " << q (it.to) << " ] && /usr/sbin/chown -R \"$O\" " << q (it.to) << "\n";
        }
        sh << "exit 0\n";
        return sh;
    }

    inline juce::var toVar (const Item& it)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", it.from.getFileName());
        o->setProperty ("from", it.from.getParentDirectory().getFullPathName());
        o->setProperty ("to", it.to.getParentDirectory().getFullPathName());
        o->setProperty ("kind", it.kind);
        if (it.status.isNotEmpty()) o->setProperty ("status", it.status);
        return juce::var (o);
    }

    inline juce::var toVar (const juce::Array<Item>& items)
    {
        juce::Array<juce::var> arr;
        for (const auto& it : items) arr.add (toVar (it));
        return juce::var (arr);
    }

    // The script, once, with administrator rights. std::system rather than a
    // ChildProcess, as InstallBatcher does it: a ChildProcess cannot bring up
    // the macOS password dialog from a background thread. False when the user
    // cancelled; `failure` holds osascript's own words otherwise.
    inline bool runElevated (const juce::Array<Item>& items, const Roots& r, juce::String& failure)
    {
       #if JUCE_MAC
        const auto dir     = juce::File::getSpecialLocation (juce::File::tempDirectory);
        const auto script  = dir.getChildFile ("rone-user-folder-move.sh");
        const auto errFile = dir.getChildFile ("rone-user-folder-move.err.txt");

        const auto sh = moveScript (items, r, juce::String ((int) getuid()) + ":" + juce::String ((int) getgid()));
        if (sh.isEmpty() || script.getFullPathName().containsAnyOf ("'\"\\$`") || errFile.getFullPathName().containsAnyOf ("'"))
        {
            failure = "a folder's path holds a quote";
            return true;
        }
        script.replaceWithText (sh, false, false, "\n");

        const auto cmd = "osascript -e 'do shell script \"/bin/sh \\\"" + script.getFullPathName()
                       + "\\\"\" with administrator privileges' 2>'" + errFile.getFullPathName() + "'";
        const int status = std::system (cmd.toRawUTF8());
        script.deleteFile();
        const int code = WIFEXITED (status) ? WEXITSTATUS (status) : -1;
        const auto err = errFile.loadFileAsString().trim();
        errFile.deleteFile();

        if (code == 0)
            return true;
        if (err.contains ("-128") || err.containsIgnoreCase ("cancel"))
            return false;
        failure = err.isNotEmpty() ? err.fromLastOccurrenceOf ("execution error:", false, false).trim()
                                   : "could not start (code " + juce::String (code) + ")";
        return true;
       #else
        juce::ignoreUnused (items, r);
        failure = "only on a Mac";
        return true;
       #endif
    }

    // The whole button: scan, move once elevated, then read the result off the disk.
    inline juce::var moveNow()
    {
        const auto roots = defaultRoots();
        auto items = scan (roots);

        bool declined = false;
        juce::String failure;
        if (! items.isEmpty())
        {
            // Made as the user where possible, so they are the user's from the start.
            for (const auto& dir : { roots.userVst3, roots.userAU, roots.userApps })
                if (dir != juce::File())
                    dir.createDirectory();

            declined = ! runElevated (items, roots, failure);
        }

        int moved = 0, failed = 0;
        bool anyAU = false;
        for (auto& it : items)
        {
            const bool gone = ! it.from.exists();
            it.status = it.kind == "remove" ? (gone ? "removed" : "failed")
                                            : (gone && it.to.exists() ? "moved" : "failed");
            if (it.status == "failed") ++failed; else ++moved;
            anyAU = anyAU || (gone && it.from.hasFileExtension ("component"));
        }

       #if JUCE_MAC
        if (anyAU)
            runTool ({ "/usr/bin/killall", "-9", "AudioComponentRegistrar" }, 10000);   // hosts see the AUs where they are now
       #endif

        auto* o = new juce::DynamicObject();
        o->setProperty ("success", ! declined && failed == 0);
        o->setProperty ("declined", declined);
        o->setProperty ("moved", moved);
        o->setProperty ("failed", failed);
        if (failure.isNotEmpty()) o->setProperty ("error", failure);
        o->setProperty ("items", toVar (items));
        return juce::var (o);
    }

    inline bool isCommandLineMode (const juce::String& commandLine)
    {
        return commandLine.contains (kMoveFlag) || commandLine.contains (kListFlag);
    }

    // Either flag with --test-root <dir>: a run on a fake tree, written to
    // RonePlugins/user-folder-move.json, plus the script the password would run
    // for it (user-folder-move.sh) so its text can be checked too. Without
    // --test-root the flags do nothing: the real move is the button.
    inline bool runCommandLine (const juce::String& commandLine)
    {
        if (! isCommandLineMode (commandLine))
            return false;

        auto tokens = juce::StringArray::fromTokens (commandLine, true);
        const int at = tokens.indexOf ("--test-root");
        if (at < 0 || at + 1 >= tokens.size())
            return true;

        const auto roots = testRoots (juce::File (tokens[at + 1].unquoted()));
        auto items = scan (roots);

        const auto outDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("RonePlugins");
        outDir.createDirectory();
        outDir.getChildFile ("user-folder-move.sh").replaceWithText (moveScript (items, roots, "501:20"), false, false, "\n");

        if (tokens.contains (kMoveFlag))
            moveItems (items, roots);

        outDir.getChildFile ("user-folder-move.json").replaceWithText (juce::JSON::toString (toVar (items)));
        return true;
    }
}
