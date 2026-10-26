#include "RackTransport.h"
#include "ControlCore.h"

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wshadow-field-in-constructor", "-Wnon-virtual-dtor", "-Wzero-as-null-pointer-constant")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4100)
#include <pluginterfaces/base/funknown.h>

namespace rackreaper
{
    using namespace Steinberg;

    // reaper_vst3_interfaces.h (Cockos, REAPER v5.02+) - the same declaration RONE Control's ReaperBridge uses
    class IReaperHostApplication : public FUnknown
    {
    public:
        virtual void* PLUGIN_API getReaperApi (CStringA funcname) = 0;
        virtual void* PLUGIN_API getReaperParent (uint32 w) = 0;
        virtual void* PLUGIN_API reaperExtended (uint32 call, void* parm1, void* parm2, void* parm3) = 0;
        static const FUID iid;
    };

    DECLARE_CLASS_IID (IReaperHostApplication, 0x79655E36, 0x77EE4267, 0xA573FEF7, 0x4912C27C)
    DEF_CLASS_IID (IReaperHostApplication)
}
JUCE_END_IGNORE_WARNINGS_MSVC
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

namespace RoneRack
{
namespace
{
    constexpr int kCmdTransportRecord = 1013;          // REAPER: Transport: Record (toggles; keeps playing when off)
    constexpr int kCmdTransportStop   = 1016;          // REAPER: Transport: Stop (the second press stops, like FL/Live)
    constexpr int kPlayStateRecording = 4;
    constexpr juce::int64 kStateFreshMs = 3000;        // the script writes at least once a second
    constexpr juce::int64 kCommandOldMs = 5000;        // a command nobody took in 5 s is no one's
    constexpr double kPollMs = 250.0;
}

Transport::~Transport()
{
    if (host != nullptr)
        static_cast<rackreaper::IReaperHostApplication*> (host)->release();
}

void Transport::attachHost (Steinberg::FUnknown* hostApplication)
{
    if (hostApplication == nullptr || host != nullptr) return;
    void* obj = nullptr;
    if (hostApplication->queryInterface (rackreaper::IReaperHostApplication::iid, &obj) != Steinberg::kResultOk || obj == nullptr)
        return;                                                     // not REAPER
    host = obj;
    auto* h = static_cast<rackreaper::IReaperHostApplication*> (host);
    api.Main_OnCommand = reinterpret_cast<void (*) (int, int)> (h->getReaperApi ("Main_OnCommand"));
    api.GetPlayState   = reinterpret_cast<int (*)()> (h->getReaperApi ("GetPlayState"));
    api.ok = api.Main_OnCommand != nullptr && api.GetPlayState != nullptr;
}

juce::String Transport::hostName() const
{
    if (standalone) return "Standalone";
    juce::PluginHostType t;
    if (t.isFruityLoops()) return "FL Studio";
    if (t.isAbletonLive()) return "Ableton Live";
    if (t.isBitwigStudio()) return "Bitwig Studio";
    if (t.isReaper()) return "REAPER";
    return juce::String (t.getHostDescription());
}

juce::String Transport::how() const
{
    if (api.ok) return "reaper";
    juce::PluginHostType t;
    if (! standalone && (t.isFruityLoops() || t.isAbletonLive() || t.isBitwigStudio())) return "script";
    return {};
}

// ---------------------------------------------------------------- the script channel
// FL's Hardware folder, found as RONE Control's FlBridge finds it: FL's user data folder from the registry
// ("Shared data", not always Documents), RONE_CONTROL_FL_DIR for the lab.
juce::File Transport::flScriptFolder()
{
    const auto test = juce::SystemStats::getEnvironmentVariable ("RONE_CONTROL_FL_DIR", {});
    if (test.isNotEmpty()) return juce::File (test).getChildFile ("RONE Control");
   #if JUCE_WINDOWS
    const auto shared = juce::WindowsRegistry::getValue ("HKEY_CURRENT_USER\\Software\\Image-Line\\Shared\\Paths\\Shared data");
    if (shared.isNotEmpty() && juce::File::isAbsolutePath (shared))
        return juce::File (shared).getChildFile ("FL Studio").getChildFile ("Settings").getChildFile ("Hardware").getChildFile ("RONE Control");
   #endif
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Image-Line").getChildFile ("FL Studio").getChildFile ("Settings").getChildFile ("Hardware").getChildFile ("RONE Control");
}

juce::File Transport::commandFile() const
{
    if (juce::PluginHostType().isFruityLoops()) return flScriptFolder().getChildFile ("transport_cmd.json");
    return RoneCtl::controlDataFolder().getChildFile ("transport").getChildFile ("cmd.json");
}

juce::File Transport::stateFile() const
{
    if (juce::PluginHostType().isFruityLoops()) return flScriptFolder().getChildFile ("transport_state.json");
    return RoneCtl::controlDataFolder().getChildFile ("transport").getChildFile ("state.json");
}

void Transport::poll() const
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (st.readAt > 0.0 && now - st.readAt < kPollMs) return;
    st.readAt = now;
    st.fresh = false;
    const auto f = stateFile();
    const auto v = f.existsAsFile() ? juce::JSON::parse (f.loadFileAsString()) : juce::var();   // kept: the object lives in it
    if (auto* o = v.getDynamicObject())
        {
            const auto at = (juce::int64) o->getProperty ("at");
            st.fresh = std::abs (juce::Time::currentTimeMillis() - at) < kStateFreshMs;
            st.recording = (bool) o->getProperty ("recording");
            st.playing = (bool) o->getProperty ("playing");
            st.ack = (juce::int64) o->getProperty ("ack");
        }
    if (cmdAt != 0)                                                 // our command: gone once the script took it
    {
        const auto c = commandFile();
        if (! c.existsAsFile()) cmdAt = 0;
        else if ((st.fresh && st.ack == cmdAt) || juce::Time::currentTimeMillis() - cmdAt > kCommandOldMs)
        {
            c.deleteFile();                                         // FL's script cannot delete; Live's and Bitwig's did
            cmdAt = 0;
        }
    }
}

// ---------------------------------------------------------------- the button
bool Transport::hostRecording() const
{
    if (api.ok) return (api.GetPlayState() & kPlayStateRecording) != 0;
    if (how() == "script")
    {
        poll();
        if (st.fresh) return st.recording;
    }
    return recording.load (std::memory_order_relaxed);
}

bool Transport::canRecord() const
{
    if (api.ok) return true;
    if (how() != "script") return false;
    poll();
    return st.fresh;
}

juce::String Transport::why() const
{
    if (canRecord()) return {};
    if (standalone) return "Recording needs a DAW";
    // Rack-first (Liran, 26.10.2026): the racks ship before RONE Control; in FL, Live and Bitwig REC is RONE Control's
    // script, so until it is installed and has run once the button says where REC comes from.
    if (how() == "script") return hostName() + ": REC comes with RONE Control";
    return hostName() + ": no record control from a plugin yet";
}

bool Transport::record (bool on)
{
    if (api.ok)
    {
        const bool now = hostRecording();
        if (on && ! now) api.Main_OnCommand (kCmdTransportRecord, 0);
        else if (! on && now) api.Main_OnCommand (kCmdTransportStop, 0);   // 1013 alone would leave REAPER playing
        return true;
    }
    if (how() == "script")
    {
        const auto at = juce::Time::currentTimeMillis();
        auto* o = new juce::DynamicObject();
        o->setProperty ("seq", ++seq);
        o->setProperty ("op", "record");
        o->setProperty ("on", on);
        o->setProperty ("at", at);
        const auto f = commandFile();
        f.getParentDirectory().createDirectory();
        if (! f.replaceWithText (juce::JSON::toString (juce::var (o)))) return false;
        cmdAt = at;
        st.readAt = 0.0;                                            // read the answer as soon as it comes
        return canRecord();
    }
    return false;
}
} // namespace RoneRack
