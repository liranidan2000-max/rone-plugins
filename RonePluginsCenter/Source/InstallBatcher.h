#pragma once

#include <JuceHeader.h>
#include "VersionChecker.h"
#include "PluginInUse.h"
#include "NetworkManager.h"
#include "OldVersionCleaner.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
 #pragma comment (lib, "shell32.lib")
#endif

#if JUCE_MAC
 #include <sys/wait.h>
 #include <cstdio>
 #include <cerrno>
 #include <cstring>
#endif

// ============================================================================
// InstallBatcher - installs what was downloaded, all of it behind ONE
// permission prompt (Center 2.0).
//
// Every RONE installer needs admin rights. Before 2.0 each finished download
// launched its own installer thread: "Update All" meant one UAC prompt per
// plugin, installers that could overlap, and a 120-second limit that counted
// the time the prompt sat open - a user who read it first got "Install timed
// out" while setup went on in the background.
//
// Now a finished download waits here as "ready". Once nothing the user asked
// for is still downloading, everything ready goes in one batch:
//   Windows: the Center runs itself once, elevated, with --install-batch
//            <job file> <job hash>. That copy moves each installer into a folder
//            only an administrator can write (Program Files\RONE Plugins\
//            InstallCache - audit R048: the ProgramData folder used before 2.1.4
//            let every user write), hashes the copy against the manifest - the
//            file that runs is the file that was checked - and runs the
//            installers one after another, writing a result per plugin as it
//            goes. An hour per installer at most (R280).
//   macOS:   a plugin that is not on the Mac system-wide yet needs no password
//            at all (Center 2.1): its .pkg is hashed and unpacked as the user
//            and the bundles go into ~/Library/Audio/Plug-Ins and ~/Applications
//            (installUserDomain below). The rest - plugins a .pkg put in /Library
//            before, or whose user folder the user may not write to (2.1.1) -
//            go through one shell script run with administrator
//            privileges (one password prompt), the same copy - hash - install
//            for each .pkg.
// A plugin a DAW still has loaded (Windows) waits as "waiting for FL Studio"
// and joins the next batch once the DAW lets go, exactly as before.
// ============================================================================
class InstallBatcher : private juce::Thread
{
public:
    static constexpr const char* kFlag = "--install-batch";

    struct Item
    {
        juce::String id, name, registryKey, version, sha256;
        juce::String vst3Bundle, auBundle, standaloneExe;
        juce::File   installer;
    };

    struct Result
    {
        juce::String id;
        bool ok = false;
        bool rebootNeeded = false;     // unused since 2.1.4: Inno's exit 8 means nothing was installed (R195)
        bool declined = false;         // the permission prompt was refused
        int  exitCode = 0;
        juce::String code;             // for the page: "installed", "in_use", "declined", "hash", "failed",
                                       // "restart_needed", "install_aborted"
        juce::String message;          // English, for the page's fallback and the error report
        juce::String reason;           // what the installer itself said (its log / stderr), may be empty
        juce::String integrityNote;    // audit R024 stage 1: the job file did not match its hash (logged only)
    };

    // All three are called on the message thread.
    std::function<void (const juce::String& id, PluginStatus, const juce::String& waitingFor)> onStatus;
    std::function<void (const Result&)> onResult;
    std::function<void()> onIdle;                       // nothing ready, waiting or installing any more

    // Asked from the batch thread: is something the user wants still downloading?
    std::function<bool()> downloadsPending;

    InstallBatcher() : juce::Thread ("RONE-Install") { startThread(); }

    ~InstallBatcher() override
    {
        alive->store (false);
        signalThreadShouldExit();
        wake.signal();
        stopThread (8000);
    }

    // Message thread: a verified installer is here.
    void add (const Item& item)
    {
        {
            const juce::ScopedLock sl (lock);
            for (auto& e : entries)
                if (e.item.id == item.id)
                    return;
            entries.add ({ item });
        }
        lastAddMs.store (juce::Time::getMillisecondCounter());
        wake.signal();
    }

    // Message thread: "nothing more is downloading" may have just become true.
    void poke() { wake.signal(); }

    // Message thread. Only before its batch starts; false once it is installing.
    bool cancel (const juce::String& id)
    {
        const juce::ScopedLock sl (lock);
        for (int i = 0; i < entries.size(); ++i)
            if (entries.getReference (i).item.id == id)
            {
                if (entries.getReference (i).state == State::installing)
                    return false;
                entries.remove (i);
                return true;
            }
        return false;
    }

    bool isBusy() const
    {
        const juce::ScopedLock sl (lock);
        return ! entries.isEmpty();
    }

    bool contains (const juce::String& id) const
    {
        const juce::ScopedLock sl (lock);
        for (auto& e : entries)
            if (e.item.id == id)
                return true;
        return false;
    }

    // ---- the elevated side (Windows) --------------------------------------
    static bool isCommandLineMode (const juce::String& commandLine) { return commandLine.contains (kFlag); }

    static bool runCommandLine (const juce::String& commandLine)
    {
        auto tokens = juce::StringArray::fromTokens (commandLine, true);
        const int at = tokens.indexOf (kFlag);
        if (at < 0) return false;
        if (at + 1 >= tokens.size()) return true;

       #if JUCE_WINDOWS
        const juce::File jobFile (tokens[at + 1].unquoted());

        // Audit R024, stage 1 (log only): the job sits in the user's %TEMP%, so it is
        // read ONCE, as bytes, and the same bytes are hashed and parsed - no second
        // read for a swap to slip into. The Center that wrote it passes the SHA256 of
        // what it wrote; a job that does not match is marked in every result
        // (jobHash) and reported by that Center, and - in this stage - still runs.
        // Stage 2 (a later release, once no false alarm was seen) refuses it.
        juce::MemoryBlock jobBytes;
        jobFile.loadFileAsData (jobBytes);
        const auto expectedJobHash = at + 2 < tokens.size() ? tokens[at + 2].unquoted().trim() : juce::String();
        const auto actualJobHash   = juce::SHA256 (jobBytes.getData(), jobBytes.getSize()).toHexString();
        const juce::String jobHashState = ! isSha256Hex (expectedJobHash)                 ? "absent"
                                        : actualJobHash.equalsIgnoreCase (expectedJobHash) ? "ok"
                                                                                           : "mismatch";

        const auto job = juce::JSON::parse (juce::String::fromUTF8 ((const char*) jobBytes.getData(), (int) jobBytes.getSize()));
        const juce::File resultFile (job.getProperty ("results", "").toString());
        auto* items = job.getProperty ("items", {}).getArray();
        if (items == nullptr || resultFile.getParentDirectory() != jobFile.getParentDirectory())
            return true;

        // Audit R048: the installers are staged in the Center's own folder under
        // Program Files, which standard users may only read. ProgramData\RONE
        // (before 2.1.4) let any user write - and swap - what was about to run
        // elevated. Only if that folder cannot be made does the old one serve,
        // so an install never fails over where it waits. A reparse point there
        // (made by an administrator) is reported, not refused, in this release.
        const auto legacyCacheDir = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory)
                                        .getChildFile ("RONE").getChildFile ("InstallCache");
        auto cacheDir = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
                            .getChildFile (RONE_INSTALL_DIR).getChildFile ("InstallCache");
        juce::String cacheNote;
        if (! cacheDir.createDirectory().wasOk())
        {
            cacheNote = "InstallCache could not be made in Program Files - used ProgramData";
            cacheDir = legacyCacheDir;
            cacheDir.createDirectory();
        }
        else if (isReparsePoint (cacheDir) || isReparsePoint (cacheDir.getParentDirectory()))
            cacheNote = "InstallCache or its parent is a reparse point";

        juce::Array<juce::var> results;
        auto writeResults = [&]
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("results", results);
            const auto tmp = resultFile.withFileExtension (".tmp");
            tmp.replaceWithText (juce::JSON::toString (juce::var (o)));
            tmp.moveFileTo (resultFile);
        };

        bool hung = false;
        for (auto& it : *items)
        {
            const auto id = it.getProperty ("id", "").toString();
            const juce::File source (it.getProperty ("path", "").toString());
            const auto sha = it.getProperty ("sha256", "").toString();

            auto* r = new juce::DynamicObject();
            r->setProperty ("id", id);
            if (jobHashState != "ok")    r->setProperty ("jobHash", jobHashState);
            if (cacheNote.isNotEmpty())  r->setProperty ("cacheNote", cacheNote);

            // Audit R280: the installer before this one hung and may still be running -
            // a second Inno setup must not start beside it.
            if (hung)
            {
                r->setProperty ("exitCode", -6);
                results.add (juce::var (r));
                writeResults();
                continue;
            }

            // Only an installer the Center downloaded beside this job file.
            const bool sane = id.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
                           && source.getParentDirectory() == jobFile.getParentDirectory()
                           && source.getFileName().endsWithIgnoreCase ("_Installer.exe")
                           && source.existsAsFile() && sha.length() == 64;

            auto stageDir = cacheDir;
            auto copy = stageDir.getChildFile (id + "_Installer.exe");
            copy.deleteFile();
            bool copied = sane && source.copyFileTo (copy);

            // Audit R048 (review): staging in Program Files must never be why an install
            // fails where 2.1.3, which staged in ProgramData, would have run it - an
            // antivirus that will not let an unsigned .exe be written there, say. One more
            // try in the old folder, reported like the other staging notes.
            if (sane && ! copied && stageDir != legacyCacheDir)
            {
                copy.deleteFile();
                stageDir = legacyCacheDir;
                stageDir.createDirectory();
                copy = stageDir.getChildFile (id + "_Installer.exe");
                copy.deleteFile();
                copied = source.copyFileTo (copy);
                r->setProperty ("cacheNote", (cacheNote.isNotEmpty() ? cacheNote + "; " : juce::String())
                                                + "the copy into Program Files failed - used ProgramData");
            }

            // The log gets a fresh name every run: a hung installer from an earlier
            // batch may still hold the old one open, and Inno aborts Setup (installs
            // nothing) when it cannot create its /LOG file.
            const auto log = stageDir.getChildFile (id + "_" + juce::Uuid().toString().substring (0, 8) + "_Installer.log");

            if (! copied)
            {
                r->setProperty ("exitCode", -3);
            }
            else
            {
                juce::FileInputStream fis (copy);
                const bool matches = fis.openedOk() && juce::SHA256 (fis).toHexString().equalsIgnoreCase (sha);

                if (! matches)
                    r->setProperty ("exitCode", -2);
                else
                {
                    // /LOG (audit R195): what Inno itself says comes back with a failure.
                    // Passed only when the Center could make that file itself, so the log
                    // can never be the reason an install does not run.
                    juce::StringArray args { copy.getFullPathName(), "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/SP-" };
                    if (log.create().wasOk())
                        args.add ("/LOG=" + log.getFullPathName());

                    juce::ChildProcess setup;
                    if (setup.start (args, 0))
                    {
                        // No time limit while setup runs; an hour is a hung installer, not a slow one.
                        for (int waited = 0; setup.isRunning() && waited < kInstallerHangMs; waited += 250)
                            juce::Thread::sleep (250);

                        if (setup.isRunning())
                        {
                            // R280: end it, and install nothing more in this batch. (This ends
                            // Inno's outer stub; a job object that ends its whole tree is stage 2.)
                            setup.kill();
                            hung = true;
                            r->setProperty ("exitCode", -4);
                        }
                        else
                            r->setProperty ("exitCode", (int) setup.getExitCode());

                        const auto detail = installerLogDetail (log);
                        if (detail.isNotEmpty())
                            r->setProperty ("detail", detail);
                    }
                    else
                        r->setProperty ("exitCode", -5);   // R202: Windows would not start it - usually antivirus
                }
            }

            copy.deleteFile();
            log.deleteFile();
            results.add (juce::var (r));
            writeResults();
        }
       #endif
        return true;
    }

private:
    enum class State { ready, waiting, installing };

    struct Entry
    {
        Item item;
        State state = State::ready;
        bool inUseChecked = false;
        juce::Array<juce::uint32> holderPids;
        juce::String waitingFor;
        juce::uint32 waitStartMs = 0, lastCheckMs = 0;
        bool forced = false;              // waited half a day and goes in regardless (not re-checked)
    };

    void post (std::function<void()> fn)
    {
        juce::MessageManager::callAsync ([flag = alive, fn = std::move (fn)] { if (flag->load()) fn(); });
    }

    void postStatus (const juce::String& id, PluginStatus s, const juce::String& waitingFor = {})
    {
        post ([this, id, s, waitingFor] { if (onStatus) onStatus (id, s, waitingFor); });
    }

    void postResult (const Result& r)
    {
        post ([this, r] { if (onResult) onResult (r); });
    }

    static bool isSha256Hex (const juce::String& s)
    {
        return s.length() == 64 && s.containsOnly ("0123456789abcdefABCDEF");
    }

   #if JUCE_WINDOWS
    static constexpr int kInstallerHangMs = 3600000;   // an hour: a hung installer, not a slow one

    static bool isReparsePoint (const juce::File& f)
    {
        const auto attrs = GetFileAttributesW (f.getFullPathName().toWideCharPointer());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    }

    // The lines of an Inno /LOG that say why it stopped (audit R195), newest last,
    // without their time stamps. Empty when it says nothing of the kind.
    static juce::String installerLogDetail (const juce::File& log)
    {
        juce::StringArray why;
        for (auto line : juce::StringArray::fromLines (log.loadFileAsString()))
        {
            line = line.trim();
            // "2026-10-13 12:00:00.000   Message" -> "Message"
            if (line.length() > 24 && juce::CharacterFunctions::isDigit (line[0]) && line[4] == '-' && line[10] == ' ')
                line = line.substring (23).trim();
            if (line.containsIgnoreCase ("error") || line.containsIgnoreCase ("abort") || line.containsIgnoreCase ("failed"))
                why.add (line);
        }
        while (why.size() > 3)
            why.remove (0);
        return why.joinIntoString (" | ").substring (0, 400);
    }

    static void holderFiles (const Item& item, juce::File& bundle, juce::File& exe)
    {
        // The Analyzer is an app, but its installer also replaces the VST3
        // bridge a DAW keeps on its master bus - outside the RONE folder.
        bundle = item.vst3Bundle.isNotEmpty() ? VersionChecker::getVst3InstallDir().getChildFile (item.vst3Bundle)
               : item.id == "RONEAnalyzer"    ? VersionChecker::getVst3InstallDir().getParentDirectory()
                                                    .getChildFile ("RONE Analyzer Bridge.vst3")
                                              : juce::File();
        exe = item.standaloneExe.isNotEmpty() ? VersionChecker::getStandaloneInstallDir().getChildFile (item.standaloneExe)
                                              : juce::File();
    }

    // Exit 5 with a program still holding the files = "close the DAW"; without
    // one it was something else (audit R195).
    static bool isHeldNow (const Item& item)
    {
        juce::File bundle, exe;
        holderFiles (item, bundle, exe);
        return ! PluginInUse::find (bundle, exe).isEmpty();
    }
   #endif

    // Ready items that a DAW holds become "waiting"; waiting ones are re-checked
    // every few seconds and come back once nothing holds them (Windows only).
    void checkInUse()
    {
       #if JUCE_WINDOWS
        juce::Array<Entry> snapshot;
        {
            const juce::ScopedLock sl (lock);
            snapshot = entries;
        }

        const auto now = juce::Time::getMillisecondCounter();

        for (auto& e : snapshot)
        {
            if (e.state == State::installing || threadShouldExit())
                continue;
            if (e.state == State::ready && e.inUseChecked)
                continue;
            if (e.state == State::waiting && now - e.lastCheckMs < 4000)
                continue;

            juce::File bundle, exe;
            holderFiles (e.item, bundle, exe);

            // Cheap: only the processes that held the files. Once they all let
            // go, one full scan - another program may have loaded it meanwhile.
            auto holders = e.state == State::waiting ? PluginInUse::find (bundle, exe, &e.holderPids)
                                                     : PluginInUse::find (bundle, exe);
            if (e.state == State::waiting && holders.isEmpty())
                holders = PluginInUse::find (bundle, exe);

            // Half a day: try anyway, the installer reports the lock.
            const bool giveUp = e.state == State::waiting && now - e.waitStartMs > 12u * 3600u * 1000u;

            juce::String announce;
            bool nowReady = false;
            {
                const juce::ScopedLock sl (lock);
                for (auto& live : entries)
                {
                    if (live.item.id != e.item.id || live.state == State::installing)
                        continue;

                    live.inUseChecked = true;
                    live.lastCheckMs = now;

                    if (holders.isEmpty() || giveUp)
                    {
                        nowReady = live.state == State::waiting;
                        live.forced = live.forced || (giveUp && ! holders.isEmpty());
                        live.state = State::ready;
                        live.holderPids.clear();
                        live.waitingFor.clear();
                    }
                    else
                    {
                        auto hosts = holders.hosts;
                        const auto waitingFor = hosts.joinIntoString (", ");   // empty = its own window
                        if (live.state != State::waiting || live.waitingFor != waitingFor)
                            announce = waitingFor.isEmpty() ? juce::String ("\x01") : waitingFor;
                        if (live.state != State::waiting)
                            live.waitStartMs = now;
                        live.state = State::waiting;
                        live.holderPids = holders.pids;
                        live.waitingFor = waitingFor;
                    }
                    break;
                }
            }

            if (announce.isNotEmpty())
                postStatus (e.item.id, PluginStatus::WaitingForHost, announce == "\x01" ? juce::String() : announce);
            else if (nowReady)
                postStatus (e.item.id, PluginStatus::ReadyToInstall);
        }
       #endif
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            wake.wait (1000);
            if (threadShouldExit())
                break;

            checkInUse();

            if (downloadsPending && downloadsPending())
                continue;   // one batch, one prompt: wait for the rest of what the user asked for

            // 2.0.6: and a few quiet seconds after the last plugin joined. Clicked
            // one after another (already downloaded, or small), each became its
            // own batch and asked for the password again (Liran's Mac, 2026-10-07).
            if (juce::Time::getMillisecondCounter() - lastAddMs.load() < kGatherMs)
                continue;

           #if JUCE_WINDOWS
            // Audit R118: a ready plugin was checked once, maybe minutes ago, while
            // the rest of the batch downloaded. A DAW or the standalone opened since
            // would make its installer roll back (exit 5, "Failed"). One fresh look
            // now: what is held waits for its program again, and stays out of this
            // batch. One that already waited half a day goes in regardless.
            {
                const juce::ScopedLock sl (lock);
                const auto now = juce::Time::getMillisecondCounter();
                for (auto& e : entries)
                    if (e.state == State::ready && e.inUseChecked && ! e.forced && now - e.lastCheckMs > 3000)
                        e.inUseChecked = false;
            }
            checkInUse();
           #endif

            juce::Array<Item> batch;
            {
                const juce::ScopedLock sl (lock);
                for (auto& e : entries)
                {
                   #if JUCE_WINDOWS
                    if (! e.inUseChecked) continue;
                   #endif
                    if (e.state == State::ready)
                    {
                        e.state = State::installing;
                        batch.add (e.item);
                    }
                }
            }

            if (batch.isEmpty())
                continue;

            for (auto& i : batch)
                postStatus (i.id, PluginStatus::Installing);

            runBatch (batch);

            bool idle = false;
            {
                const juce::ScopedLock sl (lock);
                for (auto& i : batch)
                    for (int k = entries.size(); --k >= 0;)
                        if (entries.getReference (k).item.id == i.id)
                            entries.remove (k);
                idle = entries.isEmpty();
            }

            if (idle)
                post ([this] { if (onIdle) onIdle(); });
        }
    }

    // ---- one batch ----------------------------------------------------------
    // heldNow: for exit 5, whether a program still holds the plugin's files right
    // after the batch (Windows; audit R195).
    static Result describe (const Item& item, int exitCode, bool heldNow = false)
    {
        Result r;
        r.id = item.id;
        r.exitCode = exitCode;

       #if JUCE_WINDOWS
        // Inno Setup exit codes: 0 = installed; 5 = stopped while installing (a file
        // in use with /SUPPRESSMSGBOXES answers Abort) and everything rolled back;
        // 8 = the Preparing to Install step found Windows must restart first - and
        // installed NOTHING (before 2.1.4 it counted as installed, R195); 2 =
        // cancelled before installing. Below zero: the Center's own codes.
        r.ok = exitCode == 0;
        if (r.ok)                 { r.code = "installed";
                                    r.message = item.name + " installed successfully!"; }
        else if (exitCode == 8)   { r.code = "restart_needed";
                                    r.message = item.name + ": Windows needs a restart before this can be installed. Restart, then update again."; }
        else if (exitCode == 5 && heldNow)
                                  { r.code = "in_use";
                                    r.message = "Install cancelled - a file was in use. Close the DAW or standalone that has "
                                                + item.name + " open, then update again."; }
        else if (exitCode == 5)   { r.code = "install_aborted";
                                    r.message = item.name + ": the installer stopped before it finished and nothing was changed."
                                                " Often antivirus, its manual PDF open in a reader, or a full disk."; }
        else if (exitCode == -2)  { r.code = "hash";
                                    r.message = item.name + ": the installer changed after it was checked - nothing was installed. Try again."; }
        else if (exitCode == -3)  { r.code = "failed";
                                    r.message = item.name + ": the installer could not be copied into Program Files\\RONE Plugins\\InstallCache"
                                                " (antivirus?) - nothing was installed. Try again."; }
        else if (exitCode == -4)  { r.code = "failed";
                                    r.message = item.name + ": the installer stopped responding."; }
        else if (exitCode == -5)  { r.code = "failed";
                                    r.message = item.name + ": Windows could not start the installer (often antivirus). Try again,"
                                                " or download it from roneaudio.com/account."; }
        else if (exitCode == -6)  { r.code = "failed";
                                    r.message = item.name + " was not installed - the installer before it stopped responding."
                                                " Restart the computer and update again."; }
        else if (exitCode == -1)  { r.code = "failed";
                                    r.message = item.name + ": the installer did not finish."; }
        else                      { r.code = "failed";
                                    r.message = "Install failed (installer code " + juce::String (exitCode) + ")."; }
       #else
        juce::ignoreUnused (item, heldNow);
       #endif
        return r;
    }

    static bool verifyOnDisk (const Item& item)
    {
        return (item.vst3Bundle.isEmpty()    || VersionChecker::isVst3Installed (item.vst3Bundle))
            && (item.auBundle.isEmpty()      || VersionChecker::isAUInstalled (item.auBundle))
            && (item.standaloneExe.isEmpty() || VersionChecker::isStandaloneInstalled (item.standaloneExe));
    }

    void finishItem (const Item& item, Result r)
    {
        if (r.ok)
        {
            VersionChecker::setInstalledVersion (item.registryKey, item.version);
            // Audit R050: which installer this was. The same file offered again under
            // another label (a manifest version that differs from the exe's own) is
            // then not an update - it was offered and installed forever before.
            VersionChecker::setInstalledSha (item.registryKey, item.sha256);
        }
        postResult (r);
    }

   #if JUCE_MAC
    // ---- Center 2.1: installing without a password ---------------------------
    // Liran's Mac, 2026-10-07: too many approvals. A plugin that is not on the
    // Mac system-wide goes into the user's own folders, which every DAW scans
    // and which need no administrator. One that a .pkg once put in /Library
    // stays there - a second copy in ~/Library would list the plugin twice -
    // and still goes through the password batch.
    //
    // Anything unexpected before a bundle is swapped in (a pkgutil that cannot
    // unpack, a payload holding a file this does not know where to put, a user
    // folder that cannot be written) hands the plugin to the password batch,
    // which installs the .pkg as before.
    enum class Route { done, system };

    struct Move { juce::File from, to; };

    static int runTool (const juce::StringArray& args, int timeoutMs, juce::String* output = nullptr)
    {
        juce::ChildProcess p;
        if (! p.start (args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
            return -1;
        // Wait first (with its limit), then read: these tools say a line or two.
        if (! p.waitForProcessToFinish (timeoutMs))
        {
            p.kill();
            return -1;
        }
        if (output != nullptr)
            *output = p.readAllProcessOutput().trim();
        return (int) p.getExitCode();
    }

    // Where each bundle of an unpacked .pkg goes in the user's folders. False
    // when the payload holds anything else.
    static bool planUserMoves (const juce::File& payload, juce::Array<Move>& moves)
    {
        struct Place { const char* in; const char* pattern; juce::File userDir; };
        const Place places[] {
            { "Library/Audio/Plug-Ins/VST3",       "*.vst3",      VersionChecker::getUserVst3Dir() },
            { "Library/Audio/Plug-Ins/Components", "*.component", VersionChecker::getUserAUDir() },
            { "Applications",                      "*.app",       VersionChecker::getUserAppsDir() },
            { "Users/Shared/RONE Plugins/Manuals", "*.pdf",       VersionChecker::getUserManualsDir() },
        };
        for (const auto& pl : places)
            for (const auto& f : payload.getChildFile (pl.in).findChildFiles (juce::File::findFilesAndDirectories, false, pl.pattern))
                moves.add ({ f, pl.userDir.getChildFile (f.getFileName()) });

        for (const auto& e : juce::RangedDirectoryIterator (payload, true, "*", juce::File::findFiles,
                                                             juce::File::FollowSymlinks::no))
        {
            bool known = false;
            for (const auto& m : moves)
                if (e.getFile() == m.from || e.getFile().isAChildOf (m.from)) { known = true; break; }
            if (! known)
                return false;
        }
        return ! moves.isEmpty();
    }

    // The names a bundle had before the "RONE <Name>" rename (build #155); names
    // that differ only in case are the same file on a Mac and are left out.
    static juce::StringArray legacyFileNames (const juce::File& bundle)
    {
        juce::StringArray names;
        const auto legacy = OldVersionCleaner::legacyNames();
        for (int k = 0; k < legacy.size(); ++k)
            if (legacy.getAllValues()[k] == bundle.getFileNameWithoutExtension()
                && ! legacy.getAllKeys()[k].equalsIgnoreCase (legacy.getAllValues()[k]))
                names.add (legacy.getAllKeys()[k] + bundle.getFileExtension());
        return names;
    }

    // A copy a .pkg put system-wide, under today's name or the old one: only the
    // .pkg (with the password) replaces it - its preinstall also clears the old name.
    static bool hasSystemCopy (const juce::Array<Move>& moves)
    {
        for (const auto& m : moves)
        {
            const auto ext = m.to.getFileExtension();
            juce::Array<juce::File> dirs;
            if (ext == ".vst3")           dirs.add (VersionChecker::getVst3InstallDir());
            else if (ext == ".component") dirs.add (VersionChecker::getAUInstallDir());
            else if (ext == ".app")       { dirs.add (juce::File ("/Applications")); dirs.add (juce::File ("/Applications/RONE Plugins")); }
            else continue;   // a manual: the newer one in the user's folder is the one the Center opens

            auto names = legacyFileNames (m.to);
            names.add (m.to.getFileName());
            for (const auto& d : dirs)
                for (const auto& n : names)
                    if (d.getChildFile (n).exists())
                        return true;
        }
        return false;
    }

    static bool hasUserCopy (const Item& i)
    {
        const auto app = i.standaloneExe.replace (".exe", "") + ".app";
        return (i.vst3Bundle.isNotEmpty()    && VersionChecker::getUserVst3Dir().getChildFile (i.vst3Bundle).exists())
            || (i.auBundle.isNotEmpty()      && VersionChecker::getUserAUDir().getChildFile (i.auBundle).exists())
            || (i.standaloneExe.isNotEmpty() && VersionChecker::getUserAppsDir().getChildFile (app).exists());
    }

    // Every format of this plugin sits in the system folders (the .pkg just put it there).
    static bool systemCopiesInPlace (const Item& i)
    {
        const auto app = i.standaloneExe.replace (".exe", "") + ".app";
        return (i.vst3Bundle.isEmpty() || VersionChecker::getVst3InstallDir().getChildFile (i.vst3Bundle).exists())
            && (i.auBundle.isEmpty()   || VersionChecker::getAUInstallDir().getChildFile (i.auBundle).exists())
            && (i.standaloneExe.isEmpty() || juce::File ("/Applications").getChildFile (app).exists()
                                          || juce::File ("/Applications/RONE Plugins").getChildFile (app).exists());
    }

    // Audit R196: the user-folder copies of a plugin the .pkg has just installed
    // system-wide. They are the user's own files, so no password; called only
    // after every system copy was found on disk, never before.
    static void removeUserCopies (const Item& i)
    {
        const auto app = i.standaloneExe.replace (".exe", "") + ".app";
        if (i.vst3Bundle.isNotEmpty())    VersionChecker::getUserVst3Dir().getChildFile (i.vst3Bundle).deleteRecursively();
        if (i.auBundle.isNotEmpty())      VersionChecker::getUserAUDir().getChildFile (i.auBundle).deleteRecursively();
        if (i.standaloneExe.isNotEmpty()) VersionChecker::getUserAppsDir().getChildFile (app).deleteRecursively();
    }

    // Every bundle is copied beside its place first (hidden), then swapped in by
    // renaming; a rename that fails puts back what was there. Empty = installed.
    // A copy that cannot even be written beside its place names that folder in
    // `unwritable`: then nothing in the user's folders was replaced.
    static juce::String installUserDomain (const juce::Array<Move>& moves, const juce::String& tag, juce::File& unwritable)
    {
        struct Swap { juce::File to, staged, old; bool movedOld = false, placed = false; };
        juce::Array<Swap> swaps;
        juce::String error;

        for (const auto& m : moves)
        {
            Swap s;
            s.to     = m.to;
            s.staged = m.to.getSiblingFile ("." + m.to.getFileName() + ".rone-new-" + tag);
            s.old    = m.to.getSiblingFile ("." + m.to.getFileName() + ".rone-old-" + tag);
            swaps.add (s);

            juce::String out;
            if (! m.to.getParentDirectory().createDirectory()
                || runTool (juce::StringArray { "/usr/bin/ditto", m.from.getFullPathName(), s.staged.getFullPathName() }, 300000, &out) != 0)
            {
                unwritable = m.to.getParentDirectory();
                error = "could not write to " + unwritable.getFullPathName()
                      + (out.isNotEmpty() ? " (" + out + ")" : juce::String());
                break;
            }
        }

        auto rename = [&error] (const juce::File& from, const juce::File& to, const juce::File& bundle)
        {
            if (std::rename (from.getFullPathName().toRawUTF8(), to.getFullPathName().toRawUTF8()) == 0)
                return true;
            const int e = errno;
            if (error.isEmpty())
                error = "macOS did not let the Center replace " + bundle.getFileName()
                      + " (" + juce::String (juce::CharPointer_UTF8 (std::strerror (e))) + ")"
                      // EPERM is macOS protecting an app (App Management, macOS 13+), not the folder.
                      // An app can also simply be moved to the Trash in Finder - Finder is always
                      // allowed - and the update then places a fresh one (audit R229, stage 1).
                      + (e == EPERM ? juce::String (" - allow RONE Plugins Center in System Settings > Privacy & Security"
                                                    " > App Management")
                                        + (bundle.hasFileExtension ("app") ? ", or move " + bundle.getFileName()
                                                                               + " to the Trash in Finder"
                                                                           : juce::String())
                                        + ", then update again"
                                    : juce::String());
            return false;
        };

        if (error.isEmpty())
            for (auto& s : swaps)
            {
                if (s.to.exists())
                {
                    if (! rename (s.to, s.old, s.to)) break;
                    s.movedOld = true;
                }
                if (! rename (s.staged, s.to, s.to)) break;
                s.placed = true;
            }

        if (error.isNotEmpty())
        {
            // Back as it was, last swap first. A copy that cannot go back stays
            // beside its place (hidden) rather than being deleted.
            for (int k = swaps.size(); --k >= 0;)
            {
                auto& s = swaps.getReference (k);
                if (s.placed)
                    std::rename (s.to.getFullPathName().toRawUTF8(), s.staged.getFullPathName().toRawUTF8());
                if (s.movedOld)
                    std::rename (s.old.getFullPathName().toRawUTF8(), s.to.getFullPathName().toRawUTF8());
            }
            for (auto& s : swaps)
                s.staged.deleteRecursively();
            return error;
        }

        bool anyAU = false;
        juce::StringArray placed (juce::StringArray { "/usr/bin/xattr", "-dr", "com.apple.quarantine" });
        for (auto& s : swaps)
        {
            s.old.deleteRecursively();
            placed.add (s.to.getFullPathName());
            anyAU = anyAU || s.to.hasFileExtension ("component");
            for (const auto& n : legacyFileNames (s.to))       // a pre-rename copy in the same folder
                s.to.getSiblingFile (n).deleteRecursively();
        }
        runTool (placed, 30000);     // nothing the Center downloads is quarantined - only in case
        if (anyAU)
            runTool (juce::StringArray { "/usr/bin/killall", "-9", "AudioComponentRegistrar" }, 10000);   // hosts see the new AU now
        return {};
    }

    // How the user takes back a folder of theirs that someone else's installer
    // made root's. Empty when the failure was not about permissions (a full disk).
    static juce::String unwritableFolderFix (juce::File dir, const juce::String& error)
    {
        while (! dir.isDirectory() && dir.getParentDirectory() != dir)   // the folder that stopped createDirectory
            dir = dir.getParentDirectory();
        if (dir.hasWriteAccess() && ! error.containsIgnoreCase ("permission denied"))
            return {};
        const auto path = dir.getFullPathName();
        return "To fix it: in Finder, Get Info on the folder " + path + " > Sharing & Permissions > give your user"
               " Read & Write - or in Terminal run: sudo chown -R \"$USER\" \"" + path + "\" - then install again.";
    }

    // Done (installed or failed, reported) or handed to the password batch.
    Route installWithoutPassword (const Item& i, const juce::File& work, const juce::String& tag)
    {
        auto fail = [this, &i] (const juce::String& why, const juce::String& fix = {})
        {
            Result r;
            r.id = i.id;
            r.code = "failed";
            r.message = "Install failed - " + why + ". Nothing was changed." + (fix.isNotEmpty() ? " " + fix : juce::String());
            finishItem (i, r);
            return Route::done;
        };

        {
            juce::FileInputStream in (i.installer);
            if (! in.openedOk() || ! juce::SHA256 (in).toHexString().equalsIgnoreCase (i.sha256))
            {
                Result r;
                r.id = i.id;
                r.exitCode = -2;
                r.code = "hash";
                r.message = i.name + ": the installer changed after it was checked - nothing was installed. Try again.";
                finishItem (i, r);
                return Route::done;
            }
        }

        // pkgutil wants a folder that does not exist yet.
        const auto unpacked = work.getChildFile (i.id);
        work.createDirectory();
        unpacked.deleteRecursively();
        juce::String out;
        const bool expanded = runTool (juce::StringArray { "/usr/sbin/pkgutil", "--expand-full", i.installer.getFullPathName(),
                                         unpacked.getFullPathName() }, 300000, &out) == 0;

        auto payload = unpacked.getChildFile ("Payload");
        if (expanded && ! payload.isDirectory())
            for (const auto& sub : unpacked.findChildFiles (juce::File::findDirectories, false, "*.pkg"))
                if (sub.getChildFile ("Payload").isDirectory())
                    payload = sub.getChildFile ("Payload");

        juce::Array<Move> moves;
        const bool planned = expanded && payload.isDirectory() && planUserMoves (payload, moves);

        if (planned && hasSystemCopy (moves))
            return Route::system;

        if (! planned)
        {
            // The .pkg would put it in /Library - next to the copy already in the
            // user's folders, and the DAW would list it twice.
            if (hasUserCopy (i))
                return fail (! expanded ? "the installer could not be unpacked" + (out.isNotEmpty() ? " (" + out + ")" : juce::String())
                                        : juce::String ("the installer holds files outside the plugin folders"));
            return Route::system;
        }

        juce::File unwritable;
        const auto error = installUserDomain (moves, tag, unwritable);
        unpacked.deleteRecursively();
        if (error.isNotEmpty() && unwritable != juce::File())
        {
            // A user folder the Center may not write to - typically one another
            // vendor's installer made as root (crash reports #114-#122, Center
            // 2.1.0: ~/Library/Audio/Plug-Ins/VST3, "Permission denied" every try).
            // Nothing was placed, so the .pkg installs it system-wide with the
            // password, as before 2.1 - unless a copy already sits in the user's
            // folders, which the DAW would then list twice.
            if (! hasUserCopy (i))
                return Route::system;
            return fail (error, unwritableFolderFix (unwritable, error));
        }
        if (error.isNotEmpty())
            return fail (error);

        Result r;
        r.id = i.id;
        r.ok = verifyOnDisk (i);
        r.code = r.ok ? "installed" : "failed";
        r.message = r.ok ? i.name + " installed successfully!"
                         : juce::String ("Install verification failed - components not found.");
        finishItem (i, r);
        return Route::done;
    }
   #endif

    void runBatch (const juce::Array<Item>& batch)
    {
        const auto dir = NetworkManager::getDownloadDir();
        dir.createDirectory();
        const auto tag = juce::Uuid().toString().substring (0, 12);

       #if JUCE_WINDOWS
        const auto jobFile = dir.getChildFile ("batch-" + tag + ".json");
        const auto resultFile = dir.getChildFile ("batch-" + tag + ".result.json");
        resultFile.deleteFile();

        juce::Array<juce::var> items;
        for (auto& i : batch)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("id", i.id);
            o->setProperty ("path", i.installer.getFullPathName());
            o->setProperty ("sha256", i.sha256);
            items.add (juce::var (o));
        }
        auto* job = new juce::DynamicObject();
        job->setProperty ("items", items);
        job->setProperty ("results", resultFile.getFullPathName());

        // Audit R024 stage 1: written as exact bytes (replaceWithText would turn
        // "\n" into "\r\n"), and the hash of those bytes travels on the command
        // line, which the user cannot change once the elevated copy runs.
        const auto jobText = juce::JSON::toString (juce::var (job));
        const juce::MemoryBlock jobBytes (jobText.toRawUTF8(), jobText.getNumBytesAsUTF8());
        jobFile.replaceWithData (jobBytes.getData(), jobBytes.getSize());
        const auto jobHash = juce::SHA256 (jobBytes.getData(), jobBytes.getSize()).toHexString();

        const auto self = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName();
        const auto params = juce::String (kFlag) + " \"" + jobFile.getFullPathName() + "\" " + jobHash;

        SHELLEXECUTEINFOW info {};
        info.cbSize = sizeof (info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        info.lpVerb = L"runas";
        info.lpFile = self.toWideCharPointer();
        info.lpParameters = params.toWideCharPointer();
        info.nShow = SW_HIDE;

        if (! ShellExecuteExW (&info) || info.hProcess == nullptr)
        {
            const bool declined = GetLastError() == ERROR_CANCELLED;
            for (auto& i : batch)
            {
                Result r;
                r.id = i.id;
                r.declined = declined;
                r.code = declined ? "declined" : "failed";
                r.message = declined ? juce::String ("Windows did not give permission - nothing was installed.")
                                     : juce::String ("Could not start the installer.");
                postResult (r);
            }
            jobFile.deleteFile();
            return;
        }

        juce::StringArray reported;
        auto collect = [&]
        {
            const auto parsed = juce::JSON::parse (resultFile.loadFileAsString());
            if (auto* arr = parsed.getProperty ("results", {}).getArray())
                for (auto& r : *arr)
                {
                    const auto id = r.getProperty ("id", "").toString();
                    if (id.isEmpty() || reported.contains (id))
                        continue;
                    for (auto& i : batch)
                        if (i.id == id)
                        {
                            reported.add (id);
                            const int exitCode = (int) r.getProperty ("exitCode", -1);
                            auto res = describe (i, exitCode, exitCode == 5 && isHeldNow (i));
                            if (! res.ok)
                                res.reason = r.getProperty ("detail", "").toString().trim();

                            const auto jobHashState = r.getProperty ("jobHash", "ok").toString();
                            const auto cacheNote    = r.getProperty ("cacheNote", "").toString();
                            if (jobHashState != "ok")
                                res.integrityNote = "install job hash " + jobHashState;
                            if (cacheNote.isNotEmpty())
                                res.integrityNote << (res.integrityNote.isNotEmpty() ? "; " : "") << cacheNote;

                            finishItem (i, res);
                        }
                }
        };

        // The elevated copy runs the installers one after another; each result
        // shows on its card as soon as it is written.
        for (;;)
        {
            const auto w = WaitForSingleObject (info.hProcess, 400);
            collect();
            if (w != WAIT_TIMEOUT || threadShouldExit())
                break;
        }
        CloseHandle (info.hProcess);

        if (threadShouldExit())
            return;   // the Center is quitting; the elevated copy finishes on its own

        collect();
        for (auto& i : batch)
            if (! reported.contains (i.id))
                finishItem (i, describe (i, -1));

        jobFile.deleteFile();
        resultFile.deleteFile();

       #elif JUCE_MAC
        // Center 2.1: what can go in without a password goes in first; the rest
        // waits for the one password below.
        juce::Array<Item> adminBatch;
        {
            const auto work = dir.getChildFile ("unpack-" + tag);
            for (auto& i : batch)
                if (installWithoutPassword (i, work, tag) == Route::system)
                    adminBatch.add (i);
            work.deleteRecursively();
        }
        if (adminBatch.isEmpty())
            return;

        // Record what was already on disk, to tell a fresh install from leftovers.
        const auto resultFile = dir.getChildFile ("batch-" + tag + ".result.txt");
        const auto script = dir.getChildFile ("batch-" + tag + ".sh");
        resultFile.replaceWithText ({});

        auto q = [] (const juce::String& s) { return "'" + s + "'"; };   // paths here never hold a quote (checked below)

        juce::String sh;
        sh << "#!/bin/sh\n"
           << "R=" << q (resultFile.getFullPathName()) << "\n"
           << "W=$(/usr/bin/mktemp -d /tmp/rone-install.XXXXXX) || exit 1\n";
        for (auto& i : adminBatch)
        {
            const auto pkg = i.installer.getFullPathName();
            if (pkg.containsAnyOf ("'\"\\$`") || ! i.id.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
                || ! i.sha256.containsOnly ("0123456789abcdefABCDEF"))
                continue;
            sh << "/bin/cp " << q (pkg) << " \"$W/" << i.id << ".pkg\" && "
               << "A=$(/usr/bin/shasum -a 256 \"$W/" << i.id << ".pkg\" | /usr/bin/awk '{print $1}')\n"
               << "if [ \"$A\" = \"" << i.sha256.toLowerCase() << "\" ]; then "
               // Audit R281: installer's last words go on the result line, after the code
               // ("<id> <code> <reason>"): before 2.1.4 a failed .pkg said only its number.
               // Both streams: it says why on either, depending on the macOS version.
               << "/usr/sbin/installer -pkg \"$W/" << i.id << ".pkg\" -target / >\"$W/" << i.id << ".err\" 2>&1; "
               << "C=$?; echo \"" << i.id << " $C $(/usr/bin/tail -n 2 \"$W/" << i.id << ".err\" | /usr/bin/tr '\\n\\r' '  ')\" >> \"$R\"; "
               << "else echo \"" << i.id << " -2\" >> \"$R\"; fi\n";
        }
        sh << "/bin/rm -rf \"$W\"\n";
        // "\n", not JUCE's default "\r\n": the Mac's sh read "fi\r" as a word, the
        // if never closed, and every install died on "line 7: syntax error:
        // unexpected end of file" (Zanon, a user in Korea; 2.0.2 - 2.0.4).
        script.replaceWithText (sh, false, false, "\n");

        bool declined = false;
        juce::String failure;
        if (script.getFullPathName().containsAnyOf ("'\"\\$`"))
            failure = "the download folder's path holds a quote or a $";
        else
        {
            // std::system, not ChildProcess: a ChildProcess cannot show the macOS
            // administrator password dialog from a background thread.
            //
            // The script sits in ~/Library/Caches/RONE Plugins Center - a path with
            // spaces. 2.0.0 passed it unquoted, sh ran ".../Caches/RONE" and every
            // Mac install failed as "password not given" (Marvin, 2026-10-06). It is
            // quoted inside the AppleScript string now, as 1.6 quoted the .pkg, and
            // osascript's own error is kept: -128 alone means the user cancelled.
            const auto errFile = dir.getChildFile ("batch-" + tag + ".err.txt");
            const auto cmd = "osascript -e 'do shell script \"/bin/sh \\\"" + script.getFullPathName()
                           + "\\\"\" with administrator privileges' 2>'" + errFile.getFullPathName() + "'";
            const int status = std::system (cmd.toRawUTF8());
            const int code = WIFEXITED (status) ? WEXITSTATUS (status) : -1;
            const auto err = errFile.loadFileAsString().trim();
            errFile.deleteFile();

            if (code != 0 && resultFile.loadFileAsString().trim().isEmpty())
            {
                declined = err.contains ("-128") || err.containsIgnoreCase ("cancel");
                if (! declined)
                    failure = err.isNotEmpty() ? err.fromLastOccurrenceOf ("execution error:", false, false).trim()
                                               : "the installer could not start (code " + juce::String (code) + ")";
            }
        }

        juce::StringArray reported;
        const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName();
        for (auto& line : juce::StringArray::fromLines (resultFile.loadFileAsString()))
        {
            const auto id   = line.upToFirstOccurrenceOf (" ", false, false).trim();
            const auto rest = line.fromFirstOccurrenceOf (" ", false, false).trim();
            const int code  = rest.getIntValue();
            for (auto& i : adminBatch)
                if (i.id == id && ! reported.contains (id))
                {
                    reported.add (id);
                    Result r;
                    r.id = id;
                    r.exitCode = code;
                    r.ok = code == 0 && verifyOnDisk (i);
                    r.code = r.ok ? "installed" : code == -2 ? "hash" : "failed";
                    r.message = r.ok ? i.name + " installed successfully!"
                              : code == -2 ? i.name + ": the installer changed after it was checked - nothing was installed. Try again."
                              : code == 0 ? juce::String ("Install verification failed - components not found.")
                                          : "Install failed (installer code " + juce::String (code) + ").";
                    if (! r.ok && code != -2)
                        r.reason = rest.fromFirstOccurrenceOf (" ", false, false).trim().replace (home, "~");

                    // Audit R196: the .pkg put the new version in /Library; a copy an
                    // earlier Center placed in the user's folders is the old one now.
                    if (r.ok && hasUserCopy (i) && systemCopiesInPlace (i))
                        removeUserCopies (i);

                    finishItem (i, r);
                }
        }

        for (auto& i : adminBatch)
            if (! reported.contains (i.id))
            {
                Result r;
                r.id = i.id;
                r.declined = declined;
                r.code = declined ? "declined" : "failed";
                r.message = declined ? juce::String ("The administrator password was not given - nothing was installed.")
                          : failure.isNotEmpty() ? "Install failed - " + failure + ". Nothing was installed."
                                                 : juce::String ("Install cancelled or failed. Enter your password when prompted.");
                postResult (r);
            }

        script.deleteFile();
        resultFile.deleteFile();
       #else
        for (auto& i : batch)
        {
            Result r;
            r.id = i.id;
            r.code = "failed";
            r.message = "Installing is not supported on this system.";
            postResult (r);
        }
       #endif
    }

    static constexpr juce::uint32 kGatherMs = 4000;
    std::atomic<juce::uint32> lastAddMs { 0 };

    mutable juce::CriticalSection lock;
    juce::Array<Entry> entries;
    juce::WaitableEvent wake;
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstallBatcher)
};
