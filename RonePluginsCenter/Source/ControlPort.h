#pragma once

#include <JuceHeader.h>
#include "VersionChecker.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <tlhelp32.h>
 #include "WinMidiLoopback.h"
#endif

// ============================================================================
// ControlPort — the MIDI port "RONE Control", held open from login for RONE Control (the plugin).
//
// Why the Center: FL Studio attaches a controller script only to a MIDI device that is there when FL starts (or that
// it already met in that run). A port the plugin makes itself appears later, and FL takes it for a "generic
// controller" - no script, so no LINK (seen on Liran's FL 2026, 2026-10-05). The Center starts with the machine,
// so FL always meets "RONE Control" like a hardware controller and attaches RONE Control's script by itself.
//
// - A loopback, like a loopMIDI port: whatever is sent INTO the port (the plugin opens "RONE Control" as a MIDI
//   output) comes OUT of it (the DAW's MIDI input "RONE Control").
// - It answers a MIDI identity request itself (FL asks a device it has not met; the script's supportedHardwareIds).
// - It puts RONE Control's FL script (shipped in the plugin's bundle, Contents/Resources/fl) into FL's Hardware
//   folder, so even the first FL start finds it. The plugin keeps it up to date from then on.
// - FL does NOT pick the script by itself (supportedDevices / supportedHardwareIds did nothing on a fresh FL 2026,
//   2026-10-05): it uses what its MIDI settings saved for the device. So, while FL is closed (FL reads them when it
//   starts and writes them back when it quits), the Center saves "RONE Control" -> RONE Control's script, enabled,
//   in every FL version's settings (HKCU\Software\Image-Line\FL Studio NN\Devices\MIDI input\RONE Control:
//   Type -3 = a user script, ScriptFolder = its folder - what FL itself writes for a scripted controller).
// - Only while RONE Control is installed. The exact name or nothing: a port of that name that is already there
//   (the plugin made its own, or a loopMIDI port) is left alone and tried again later.
//
// Windows 11 with the new MIDI stack (service midisrv): a port pair from Windows itself (WinMidiLoopback.h).
//   A = "RONE Control" (the plugin and the DAW open it), B = "RONE Control Link" (only the Center opens it). The
//   Center echoes what reaches B back out of B, so "RONE Control" stays one loopback, exactly as before: every
//   plugin version (the ones out there open the output "RONE Control") and every DAW script keep working, no
//   plugin release needed. loopMIDI's ports pass NO data on that stack until midisrv restarts (2026-10-30).
//   The pair goes away when the Center closes (a port nobody echoes would swallow the plugin's CCs) and is made
//   again when midisrv restarts (a restart wipes every pair).
// Windows otherwise, or when the pair cannot be made: loopMIDI's driver (teVirtualMIDI - its SDK needs Tobias
//   Erichsen's licence for a commercial product).
// macOS: a CoreMIDI destination and source of the same name.
// ============================================================================
class ControlPort : private juce::Timer
{
public:
    ControlPort()  { startTimer (2000); }             // a moment after the Center starts
    ~ControlPort() override { stopTimer(); close(); }

   #if JUCE_WINDOWS
    bool isOpen() const noexcept { return port != nullptr || (pair != nullptr && pair->state == Pair::open); }
   #else
    bool isOpen() const noexcept { return port != nullptr; }
   #endif

    static juce::File pluginBundle()
    {
       #if JUCE_WINDOWS
        return juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
                   .getChildFile ("Common Files").getChildFile ("VST3").getChildFile ("RONE").getChildFile ("RONE Control.vst3");
       #elif JUCE_MAC
        // /Library from a .pkg, ~/Library when the Center 2.1 installed it without a password
        const auto found = VersionChecker::findVst3 ("RONE Control.vst3");
        return found != juce::File() ? found : juce::File ("/Library/Audio/Plug-Ins/VST3/RONE Control.vst3");
       #else
        return juce::File ("/Library/Audio/Plug-Ins/VST3/RONE Control.vst3");
       #endif
    }

    // FL's user data folder (Options > File settings) is not always Documents: FL keeps it in the registry
    static juce::File flHardwareFolder()
    {
       #if JUCE_WINDOWS
        const auto shared = juce::WindowsRegistry::getValue ("HKEY_CURRENT_USER\\Software\\Image-Line\\Shared\\Paths\\Shared data");
        if (shared.isNotEmpty() && juce::File::isAbsolutePath (shared))
            return juce::File (shared).getChildFile ("FL Studio").getChildFile ("Settings").getChildFile ("Hardware");
       #endif
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Image-Line").getChildFile ("FL Studio").getChildFile ("Settings").getChildFile ("Hardware");
    }

private:
    static constexpr const char* kName = "RONE Control";
    // 7D (non-commercial), family 'R' 'C', model 1, version 1 - the same ID the plugin answers with, fixed forever
    static constexpr juce::uint8 kIdentity[] = { 0xF0, 0x7E, 0x7F, 0x06, 0x02, 0x7D, 0x52, 0x43, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF7 };

    static bool isIdentityRequest (const juce::uint8* d, int n)
    {
        return n == 6 && d[0] == 0xF0 && d[1] == 0x7E && d[3] == 0x06 && d[4] == 0x01 && d[5] == 0xF7;
    }

    void timerCallback() override
    {
        startTimer (15000);                           // then every 15 s: installed, removed, the name freed
        if (! pluginBundle().isDirectory())
        {
            close();
            return;
        }
        installScript();
        if (! isOpen())
            open();
        ensureFlDevice();
    }

    void installScript()
    {
        const auto src = pluginBundle().getChildFile ("Contents").getChildFile ("Resources").getChildFile ("fl");
        if (! src.isDirectory()) return;
        const auto dst = flHardwareFolder().getChildFile ("RONE Control");
        for (const auto& f : src.findChildFiles (juce::File::findFiles, false, "*.py"))
        {
            const auto target = dst.getChildFile (f.getFileName());
            if (target.existsAsFile() && target.loadFileAsString().removeCharacters ("\r") == f.loadFileAsString().removeCharacters ("\r")) continue;
            if (dst.createDirectory()) f.copyFileTo (target);
        }
        // The script answers through replies/*.syx. FL's Python sandbox lets it write there but not make the folder
        // or delete a file: the folder is made here, and answers no plugin read (none open in FL) are cleared.
        const auto replies = dst.getChildFile ("replies");
        replies.createDirectory();
        const auto now = juce::Time::getCurrentTime();
        for (const auto& f : replies.findChildFiles (juce::File::findFiles, false, "*.syx"))
            if ((now - f.getLastModificationTime()).inSeconds() > 60.0) f.deleteFile();
    }

   #if JUCE_WINDOWS
    static bool flRunning()
    {
        const HANDLE snap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return true;          // cannot tell: leave FL's settings alone
        PROCESSENTRY32W e {};
        e.dwSize = sizeof (e);
        bool found = false;
        for (BOOL ok = Process32FirstW (snap, &e); ok && ! found; ok = Process32NextW (snap, &e))
            found = _wcsicmp (e.szExeFile, L"FL64.exe") == 0 || _wcsicmp (e.szExeFile, L"FL.exe") == 0;
        CloseHandle (snap);
        return found;
    }

    void ensureFlDevice()
    {
        if (flRunning()) return;
        HKEY root = nullptr;
        if (RegOpenKeyExW (HKEY_CURRENT_USER, L"Software\\Image-Line", 0, KEY_READ, &root) != ERROR_SUCCESS) return;
        juce::StringArray versions;
        wchar_t name[256];
        for (DWORD i = 0;; ++i)
        {
            DWORD len = 256;
            if (RegEnumKeyExW (root, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
            const juce::String k (name);
            if (k.startsWith ("FL Studio ") && k.fromFirstOccurrenceOf ("FL Studio ", false, false).getIntValue() >= 20)
                versions.add (k);
        }
        RegCloseKey (root);
        for (const auto& v : versions)
        {
            const auto devices = "HKEY_CURRENT_USER\\Software\\Image-Line\\" + v + "\\Devices\\MIDI input\\";
            if (! juce::WindowsRegistry::keyExists (devices.dropLastCharacters (1))) continue;   // that FL never ran
            const auto key = devices + kName + "\\";
            if (juce::WindowsRegistry::getValue (key + "Type") == "-3"
                && juce::WindowsRegistry::getValue (key + "ScriptFolder") == kName
                && juce::WindowsRegistry::getValue (key + "Enabled") == "1") continue;
            juce::WindowsRegistry::setValue (key + "Type", juce::String ("-3"));
            juce::WindowsRegistry::setValue (key + "ScriptFolder", juce::String (kName));
            juce::WindowsRegistry::setValue (key + "Enabled", juce::String ("1"));
            for (const auto& [value, fallback] : { std::pair<const char*, const char*> { "Port", "-1" }, { "IDString", "-" }, { "ConnectionCounter", "0" } })
                if (! juce::WindowsRegistry::valueExists (key + value))
                    juce::WindowsRegistry::setValue (key + value, juce::String (fallback));
        }
    }

    using Port     = void*;
    using DataCb   = void (CALLBACK*) (Port, LPBYTE, DWORD, DWORD_PTR);
    using CreateFn = Port (CALLBACK*) (LPCWSTR, DataCb, DWORD_PTR, DWORD, DWORD);
    using CloseFn  = void (CALLBACK*) (Port);
    using SendFn   = BOOL (CALLBACK*) (Port, LPBYTE, DWORD);
    // TE_VM_FLAGS_PARSE_RX | TE_VM_FLAGS_PARSE_TX | TE_VM_FLAGS_INSTANTIATE_BOTH: whole messages each way
    static constexpr DWORD kFlags = 1 | 2 | 12, kMaxSysex = 65535;

    // the driver's thread: what came in goes straight back out (a null buffer = the driver shut the port)
    static void CALLBACK onData (Port p, LPBYTE data, DWORD length, DWORD_PTR instance)
    {
        auto* self = reinterpret_cast<ControlPort*> (instance);
        if (self == nullptr || data == nullptr || length == 0) return;
        if (isIdentityRequest (data, (int) length))
            self->sendFn (p, const_cast<LPBYTE> (kIdentity), (DWORD) sizeof (kIdentity));
        else
            self->sendFn (p, data, length);
    }

    static void log (const juce::String& line)
    {
        const auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("RonePluginsCenter").getChildFile ("control-port.log");
        if (f.getSize() > 100000) f.deleteFile();
        f.getParentDirectory().createDirectory();
        f.appendText (juce::Time::getCurrentTime().toString (true, true, true, true) + " " + line + "\n");
    }

    // The new MIDI stack's pair, on its own thread: the MIDI service is out of process and a wedged one blocks
    // every call into it (windows-midi-service-hang) - the Center's window must never wait on it.
    struct Pair : juce::Thread, juce::MidiInputCallback
    {
        static constexpr const char* kLinkName = "RONE Control Link";
        // RONE Control's pair, the same on every computer (lower case: the service looks removals up as written)
        static constexpr const char* kAssociation = "{5ce1a0b2-7d3e-4c52-9b43-524f4e45434c}";
        enum State { trying, open, failed };
        std::atomic<int> state { trying };

        Pair() : juce::Thread ("RONE Control pair") { startThread(); }
        ~Pair() override
        {
            signalThreadShouldExit();
            notify();
            stopThread (6000);                        // run() closes the ports and removes the pair on its way out
        }

        void run() override
        {
            DWORD madeFor = 0;                        // the service process the pair lives in
            while (! threadShouldExit())
            {
                const auto pid = rone::winmidi::serviceProcessId();
                if (state != open || (pid != 0 && pid != madeFor))
                {
                    if (state == open) log ("MIDI service restarted: making the pair again");
                    closePorts();
                    if (! makeAndOpen()) { state = failed; break; }
                    madeFor = rone::winmidi::serviceProcessId();
                    state = open;
                }
                wait (15000);
            }
            closePorts();
            if (made)
            {
                juce::String reply;
                const bool gone = rone::winmidi::removeLoopback (kAssociation, reply);
                log ("pair " + juce::String (gone ? "removed" : "not removed") + ": " + reply.substring (0, 300));
            }
        }

        bool makeAndOpen()
        {
            juce::String reply;
            made = rone::winmidi::createLoopback (kAssociation, "RONECONTROL",
                                                  kName, "RONE Control: the port the RONE Control plugin and your DAW open",
                                                  kLinkName, "RONE Control: the side the RONE Plugins Center keeps open",
                                                  reply);
            log ("Windows MIDI pair " + juce::String (made ? "made" : "not available") + ": " + reply.substring (0, 300));
            if (! made) return false;
            // the service publishes the ports a moment after it says yes
            for (int i = 0; i < 50 && ! threadShouldExit() && (out == nullptr || in == nullptr); ++i)
            {
                if (out == nullptr)
                    for (const auto& d : juce::MidiOutput::getAvailableDevices())
                        if (d.name == kLinkName) { out = juce::MidiOutput::openDevice (d.identifier); break; }
                if (out != nullptr && in == nullptr)  // the echo needs the output before anything comes in
                    for (const auto& d : juce::MidiInput::getAvailableDevices())
                        if (d.name == kLinkName)
                        {
                            if ((in = juce::MidiInput::openDevice (d.identifier, this)) != nullptr) in->start();
                            break;
                        }
                if (out == nullptr || in == nullptr) wait (100);
            }
            if (out != nullptr && in != nullptr)
            {
                log ("\"" + juce::String (kName) + "\" open through \"" + kLinkName + "\"");
                return true;
            }
            log ("pair made, but \"" + juce::String (kLinkName) + "\" did not open");
            closePorts();
            return false;
        }

        void closePorts()
        {
            if (in != nullptr) in->stop();            // no callback runs after this
            in.reset();
            out.reset();
        }

        // what the plugin (or a DAW) sent into "RONE Control" comes out of "RONE Control" again
        void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m) override
        {
            if (out == nullptr) return;
            if (isIdentityRequest (m.getRawData(), m.getRawDataSize()))
                out->sendMessageNow (juce::MidiMessage (kIdentity, (int) sizeof (kIdentity)));
            else
                out->sendMessageNow (m);
        }

        std::unique_ptr<juce::MidiInput> in;
        std::unique_ptr<juce::MidiOutput> out;
        bool made = false;
    };

    void open()
    {
        // the new MIDI stack: the pair first; while it is being made (or is open) nothing else
        if (pair == nullptr && rone::winmidi::serviceInstalled())
        {
            pair = std::make_unique<Pair>();
            return;
        }
        if (pair != nullptr && pair->state != Pair::failed) return;
        if (port != nullptr) return;
        if (dll == nullptr)
            dll = LoadLibraryExW (L"teVirtualMIDI64.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (dll == nullptr) return;                   // loopMIDI's driver is not installed (yet)
        create  = reinterpret_cast<CreateFn> (GetProcAddress (dll, "virtualMIDICreatePortEx2"));
        closeFn = reinterpret_cast<CloseFn>  (GetProcAddress (dll, "virtualMIDIClosePort"));
        sendFn  = reinterpret_cast<SendFn>   (GetProcAddress (dll, "virtualMIDISendData"));
        if (create == nullptr || closeFn == nullptr || sendFn == nullptr) return;
        port = create (juce::String (kName).toWideCharPointer(), &onData, reinterpret_cast<DWORD_PTR> (this), kMaxSysex, kFlags);
    }

    void close()
    {
        pair.reset();                                 // the plugin was removed / the Center quits: the pair goes
        if (port != nullptr && closeFn != nullptr) closeFn (port);   // no callback runs after this
        port = nullptr;
        if (dll != nullptr) { FreeLibrary (dll); dll = nullptr; }
    }

    HMODULE dll = nullptr;
    CreateFn create = nullptr;
    CloseFn closeFn = nullptr;
    SendFn sendFn = nullptr;
    Port port = nullptr;
    std::unique_ptr<Pair> pair;
   #else
    void ensureFlDevice() {}                          // macOS: FL keeps its settings elsewhere - to do

    struct Loop : juce::MidiInputCallback
    {
        std::unique_ptr<juce::MidiOutput> out;
        void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m) override
        {
            if (out == nullptr) return;
            if (isIdentityRequest (m.getRawData(), m.getRawDataSize()))
                out->sendMessageNow (juce::MidiMessage (kIdentity, (int) sizeof (kIdentity)));
            else
                out->sendMessageNow (m);
        }
    };

    void open()
    {
        for (const auto& d : juce::MidiOutput::getAvailableDevices())
            if (d.name == kName) return;              // the plugin made its own: leave it
        auto loop = std::make_unique<Loop>();
        loop->out = juce::MidiOutput::createNewDevice (kName);
        if (loop->out == nullptr) return;
        auto in = juce::MidiInput::createNewDevice (kName, loop.get());
        if (in == nullptr) return;
        in->start();
        loopback = std::move (loop);
        input = std::move (in);
        port = input.get();
    }

    void close()
    {
        if (input != nullptr) input->stop();
        input = nullptr;
        loopback = nullptr;
        port = nullptr;
    }

    std::unique_ptr<Loop> loopback;
    std::unique_ptr<juce::MidiInput> input;
    void* port = nullptr;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPort)
};
