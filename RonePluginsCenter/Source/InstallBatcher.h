#pragma once

#include <JuceHeader.h>
#include "VersionChecker.h"
#include "PluginInUse.h"
#include "NetworkManager.h"

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
//            <job file>. That copy moves each installer into a folder only an
//            administrator can write (ProgramData\RONE\InstallCache), hashes the
//            copy against the manifest - the file that runs is the file that
//            was checked - and runs the installers one after another, writing a
//            result per plugin as it goes. No time limit while one is running.
//   macOS:   one shell script run with administrator privileges (one password
//            prompt), the same copy - hash - install for each .pkg.
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
        bool rebootNeeded = false;     // Inno exit 8: a file in use is replaced at the next restart
        bool declined = false;         // the permission prompt was refused
        int  exitCode = 0;
        juce::String code;             // for the page: "installed", "in_use", "declined", "hash", "failed"
        juce::String message;          // English, for the page's fallback and the error report
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
        const auto job = juce::JSON::parse (jobFile.loadFileAsString());
        const juce::File resultFile (job.getProperty ("results", "").toString());
        auto* items = job.getProperty ("items", {}).getArray();
        if (items == nullptr || resultFile.getParentDirectory() != jobFile.getParentDirectory())
            return true;

        const auto cacheDir = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory)
                                  .getChildFile ("RONE").getChildFile ("InstallCache");
        cacheDir.createDirectory();

        juce::Array<juce::var> results;
        auto writeResults = [&]
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("results", results);
            const auto tmp = resultFile.withFileExtension (".tmp");
            tmp.replaceWithText (juce::JSON::toString (juce::var (o)));
            tmp.moveFileTo (resultFile);
        };

        for (auto& it : *items)
        {
            const auto id = it.getProperty ("id", "").toString();
            const juce::File source (it.getProperty ("path", "").toString());
            const auto sha = it.getProperty ("sha256", "").toString();

            auto* r = new juce::DynamicObject();
            r->setProperty ("id", id);

            // Only an installer the Center downloaded beside this job file.
            const bool sane = id.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
                           && source.getParentDirectory() == jobFile.getParentDirectory()
                           && source.getFileName().endsWithIgnoreCase ("_Installer.exe")
                           && source.existsAsFile() && sha.length() == 64;

            const auto copy = cacheDir.getChildFile (id + "_Installer.exe");
            copy.deleteFile();

            if (! sane || ! source.copyFileTo (copy))
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
                    juce::ChildProcess setup;
                    if (setup.start (juce::StringArray { copy.getFullPathName(), "/VERYSILENT", "/SUPPRESSMSGBOXES",
                                                         "/NORESTART", "/SP-" }, 0))
                    {
                        // No time limit while setup runs; an hour is a hung installer, not a slow one.
                        for (int waited = 0; setup.isRunning() && waited < 3600000; waited += 250)
                            juce::Thread::sleep (250);
                        r->setProperty ("exitCode", setup.isRunning() ? -4 : (int) setup.getExitCode());
                    }
                    else
                        r->setProperty ("exitCode", -3);
                }
            }

            copy.deleteFile();
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

   #if JUCE_WINDOWS
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
    static Result describe (const Item& item, int exitCode)
    {
        Result r;
        r.id = item.id;
        r.exitCode = exitCode;

       #if JUCE_WINDOWS
        // Inno Setup exit codes: 0 = installed; 8 = installed, but a file in use is
        // replaced at the next reboot (/NORESTART); 5 = a plugin file was open in a
        // DAW and setup rolled everything back; 2 = cancelled before installing.
        r.ok = exitCode == 0 || exitCode == 8;
        r.rebootNeeded = exitCode == 8;
        if (r.ok)                 { r.code = r.rebootNeeded ? "installed_reboot" : "installed";
                                    r.message = item.name + (r.rebootNeeded ? " installed - restart Windows to finish replacing a file that was in use."
                                                                            : " installed successfully!"); }
        else if (exitCode == 5)   { r.code = "in_use";
                                    r.message = "Install cancelled - a file was in use. Close the DAW or standalone that has "
                                                + item.name + " open, then update again."; }
        else if (exitCode == -2)  { r.code = "hash";
                                    r.message = item.name + ": the installer changed after it was checked - nothing was installed. Try again."; }
        else if (exitCode == -4)  { r.code = "failed";
                                    r.message = item.name + ": the installer stopped responding."; }
        else if (exitCode == -1)  { r.code = "failed";
                                    r.message = item.name + ": the installer did not finish."; }
        else                      { r.code = "failed";
                                    r.message = "Install failed (installer code " + juce::String (exitCode) + ")."; }
       #else
        juce::ignoreUnused (item);
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
            VersionChecker::setInstalledVersion (item.registryKey, item.version);
        postResult (r);
    }

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
        jobFile.replaceWithText (juce::JSON::toString (juce::var (job)));

        const auto self = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName();
        const auto params = juce::String (kFlag) + " \"" + jobFile.getFullPathName() + "\"";

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
                            finishItem (i, describe (i, (int) r.getProperty ("exitCode", -1)));
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
        // Record what was already on disk, to tell a fresh install from leftovers.
        const auto resultFile = dir.getChildFile ("batch-" + tag + ".result.txt");
        const auto script = dir.getChildFile ("batch-" + tag + ".sh");
        resultFile.replaceWithText ({});

        auto q = [] (const juce::String& s) { return "'" + s + "'"; };   // paths here never hold a quote (checked below)

        juce::String sh;
        sh << "#!/bin/sh\n"
           << "R=" << q (resultFile.getFullPathName()) << "\n"
           << "W=$(/usr/bin/mktemp -d /tmp/rone-install.XXXXXX) || exit 1\n";
        for (auto& i : batch)
        {
            const auto pkg = i.installer.getFullPathName();
            if (pkg.containsAnyOf ("'\"\\$`") || ! i.id.containsOnly ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")
                || ! i.sha256.containsOnly ("0123456789abcdefABCDEF"))
                continue;
            sh << "/bin/cp " << q (pkg) << " \"$W/" << i.id << ".pkg\" && "
               << "A=$(/usr/bin/shasum -a 256 \"$W/" << i.id << ".pkg\" | /usr/bin/awk '{print $1}')\n"
               << "if [ \"$A\" = \"" << i.sha256.toLowerCase() << "\" ]; then "
               << "/usr/sbin/installer -pkg \"$W/" << i.id << ".pkg\" -target / >/dev/null 2>&1; "
               << "echo \"" << i.id << " $?\" >> \"$R\"; "
               << "else echo \"" << i.id << " -2\" >> \"$R\"; fi\n";
        }
        sh << "/bin/rm -rf \"$W\"\n";
        script.replaceWithText (sh);

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
        for (auto& line : juce::StringArray::fromLines (resultFile.loadFileAsString()))
        {
            const auto id = line.upToFirstOccurrenceOf (" ", false, false).trim();
            const int code = line.fromFirstOccurrenceOf (" ", false, false).trim().getIntValue();
            for (auto& i : batch)
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
                    finishItem (i, r);
                }
        }

        for (auto& i : batch)
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

    mutable juce::CriticalSection lock;
    juce::Array<Entry> entries;
    juce::WaitableEvent wake;
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstallBatcher)
};
